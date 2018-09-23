#include "stdafx.h"
#include "BilibiliDanmu.h"
#include <ctime>
#include <algorithm>
#include <iostream>

CBilibiliDanmu::CBilibiliDanmu() {
	_isworking = false;
	_roomcount = 0;
}

CBilibiliDanmu::~CBilibiliDanmu() {
	Deinit();
}

// 函数退出后相关IO被自动清理
int CBilibiliDanmu::OnClose(PER_SOCKET_CONTEXT* pSocketContext) {
	int room = pSocketContext->label;
	printf("[Danmu] Worker Close Room:%d \n", room);
	// 如果房间在列表中将其标志为需要重连
	if (m_rinfo.find(room) == m_rinfo.end()) {
		return 0;
	}
	m_rinfo[room].needconnect = true;

	return 0;
}

int CBilibiliDanmu::OnReceive(PER_SOCKET_CONTEXT* pSocketContext, PER_IO_CONTEXT* pIoContext, int byteslen) {
	int type;
	int room = pSocketContext->label;
	unsigned char *str = (unsigned char *)pIoContext->m_szBuffer;
	int reclen = byteslen + pIoContext->m_occupy;
	int pos = 0, i;
	while (pos < reclen) {
		// 通过前四个字节的数据计算当前数据包长度
		if (pos + 4 > reclen) {
			// 无法获得数据包长度
			printf("[Danmu] Warning: %d Recv bytes last:sum:remain:need %d:%d:%d:%d \n",
				room, pIoContext->m_occupy, reclen, reclen - pos, 4);
			pIoContext->m_occupy = reclen - pos;
			if (!pos)
				return 0;
			for (i = 0; i < pIoContext->m_occupy; i++) {
				pIoContext->m_szBuffer[i] = pIoContext->m_szBuffer[pos + i];
			}
			return 0;
		}

		// 计算数据包长度
		unsigned int ireclen;
		ireclen = str[pos + 1] << 16 | str[pos + 2] << 8 | str[pos + 3];
		// 协议头长度为16字节因此数据包至少为16字节
		// 当消息类型为SEND_GIFT时在同时赠送5个小电视的情况下数据包长度会大于3500个字节
		if (ireclen < 16 || ireclen > 5000) {
			// 数据包错误
			printf("[Danmu] Error: %d Recv bytes last:sum:remain:need %d:%d:%d:%d \n",
				room, pIoContext->m_occupy, reclen, reclen - pos, ireclen);
			pIoContext->m_occupy = 0;
			return -1;
		}
		// 如果剩余数据大于16字节则校验协议头的正确性
		if (pos + 16 <= reclen) {
			type = CheckMessage(str + pos);
			if (type == -1) {
				// 数据包校验失败
				printf("[Danmu] Error: %d Recv bytes last:sum:remain:need %d:%d:%d:%d \n",
					room, pIoContext->m_occupy, reclen, reclen - pos, ireclen);
				pIoContext->m_occupy = 0;
				return -1;
			}
		}
		// 数据包正确但不完整
		if (pos + int(ireclen) > reclen ) {
#ifdef _DEBUG
			printf("[Danmu] Leave: %d Recv bytes last:sum:remain:need %d:%d:%d:%d \n", 
				room, pIoContext->m_occupy, reclen, reclen - pos, ireclen);
#endif
			pIoContext->m_occupy = reclen - pos;
			for (i = 0; i < pIoContext->m_occupy; i++) {
				pIoContext->m_szBuffer[i] = pIoContext->m_szBuffer[pos + i];
			}
			return 0;
		}

		// 转发给其它函数处理
		str[pos + ireclen] = 0;
		ProcessData(str + (pos + 16), ireclen - 16, room, type);

		// 移动指针
		pos += ireclen;
	}

	// 当数据包有错误或者数据包不完整时会直接在while语句内退出本方法
	// 当指针正好指向末尾时才会跳出while语句执行到此处
	// 遗留数据处理完毕
	if (pIoContext->m_occupy) {
#ifdef _DEBUG
		printf("[Danmu] Clear: %d Recv bytes last:sum:remain %d:%d:%d \n", 
			room, pIoContext->m_occupy, reclen, pos - reclen);
#endif
		pIoContext->m_occupy = 0;
	}

	return 0;
}

int CBilibiliDanmu::OnHeart() {
	//所有在线连接发送心跳包
	int ret, len, room;
	char cmdstr[60];
	std::list<toollib::PER_SOCKET_CONTEXT*>::iterator m_itor;
	bool needclean = false;

	// 向所有活动连接发送心跳包
	EnterCriticalSection(&m_csContextList);
	for (m_itor = m_arrayClientContext.begin(); m_itor != m_arrayClientContext.end();) {
		room = (*m_itor)->label;
		len = this->MakeHeartInfo((unsigned char *)cmdstr, 60, room);
		ret = _PostSend(*m_itor, cmdstr, len);
		if (ret) {
			//发送失败则断开连接 等待重连 必须保证没有Worker线程正在处理该连接的数据
			printf("[Danmu] Heart %d failed code:%d \n", room, ret);
			this->_DisConnectSocketHard(m_itor);
			m_rinfo[room].needconnect = true;
			needclean = true;
		}
		m_itor++;
	}
	LeaveCriticalSection(&m_csContextList);

	// 清理发送失败的连接
	if (needclean) {
		this->_CleanContextList();
	}

	// 扫描并重连断开的房间
	std::map<unsigned, ROOM_INFO>::const_iterator it;
	for (it = m_rinfo.begin(); it != m_rinfo.end(); ++it) {
		if (it->second.needconnect) {
			this->SendConnectionInfo(it->first);
			Sleep(0);
		}
	}

	return 0;
}

//开启主监视线程 初始化IOCP类
int CBilibiliDanmu::Init(DANMU_MODE mode) {
	if (_isworking)
		return -1;
	// 开启心跳模块
	this->SetHeart(true, 30000);
	if (mode == DANMU_MODE::SINGLE_ROOM) {
		this->Initialize(2);
	}
	else {
		this->Initialize(0);
	}
	_isworking = true;

	return 0;
}

//退出所有线程 断开Socket 释放所有IOCP资源
int CBilibiliDanmu::Deinit() {
	if (!_isworking)
		return 0;
	// 关闭IOCP客户端
	this->Destory();
	// 清理列表
	m_rinfo.clear();
	m_rlist.clear();
	_roomcount = 0;

	_isworking = false;

	return 0;
}

int CBilibiliDanmu::ConnectToRoom(const unsigned room, DANMU_FLAG flag) {
	if (m_rlist.count(room)) {
		return -1;
	}
	// 添加房间至列表
	_roomcount++;
	ROOM_INFO info;
	info.flag = flag;
	m_rinfo[room] = info;
	m_rlist.insert(room);
	this->SendConnectionInfo(room);

	return 0;
}

int CBilibiliDanmu::DisconnectFromRoom(const unsigned room) {
	if (!m_rlist.count(room)) {
		return -1;
	}
	this->CloseConnectionByLabel(room);
	// 从列表清除该房间
	m_rinfo.erase(room);
	m_rlist.erase(room);
	_roomcount--;
	return 0;
}

long long CBilibiliDanmu::GetRUID() {
	srand(unsigned(time(0)));
	double val = 100000000000000.0 + 200000000000000.0*(rand() / (RAND_MAX + 1.0));
	return static_cast <long long> (val);
}

int CBilibiliDanmu::ShowCount() {
	printf("Map count: %d IO count: %d \n", m_rinfo.size(), m_arrayClientContext.size());
	return m_arrayClientContext.size();
}

int CBilibiliDanmu::UpdateRoom(std::set<unsigned> &nlist, DANMU_FLAG flag) {
	using namespace std;
	set<unsigned> dlist, ilist;
	// 断开失效房间
	set_difference(m_rlist.begin(), m_rlist.end(), nlist.begin(), nlist.end(), inserter(dlist, dlist.end()));
	for (auto it = dlist.begin(); it != dlist.end(); it++) {
		DisconnectFromRoom(*it);
	}
	// 连接新增房间
	set_difference(nlist.begin(), nlist.end(), m_rlist.begin(), m_rlist.end(), inserter(ilist, ilist.end()));
	for (auto it = ilist.begin(); it != ilist.end(); it++) {
		ConnectToRoom(*it, flag);
	}
	// 输出操作概要
	cout << "New list count: " << nlist.size() << endl;
	cout << "Remove room count: " << dlist.size() << endl;
	cout << "Add room count: " << ilist.size() << endl;
	cout << "Current room count: " << m_rlist.size() << endl;
	return m_rlist.size();
}

int CBilibiliDanmu::MakeConnectionInfo(unsigned char* str, int len, int room) {
	memset(str, 0, len);
	int buflen;
	buflen = sprintf_s((char*)str + 16, len - 16, "{\"roomid\":%d,\"uid\":%I64d}", room, GetRUID());
	if (buflen == -1) {
		return -1;
	}
	buflen = 16 + buflen;
	str[3] = buflen;
	str[5] = 0x10;
	str[7] = 0x01;
	str[11] = 0x07;
	str[15] = 0x01;
	return buflen;
}

int CBilibiliDanmu::MakeHeartInfo(unsigned char* str, int len, int room) {
	memset(str, 0, len);
	int buflen;
	buflen = sprintf_s((char*)str + 16, len - 16, "{\"roomid\":%d,\"uid\":%I64d}", room, GetRUID());
	if (buflen == -1) {
		return -1;
	}
	buflen = 16 + buflen;
	str[3] = buflen;
	str[5] = 0x10;
	str[7] = 0x01;
	str[11] = 0x02;
	str[15] = 0x01;
	return buflen;
}

int CBilibiliDanmu::SendConnectionInfo(int room) {
	//构造发送的字符串
	int ret, len;
	char cmdstr[60];
	len = this->MakeConnectionInfo((unsigned char *)cmdstr, 60, room);
	// 在添加连接信息至列表时会加线程锁
	ret = this->Connect(2243, "livecmt-2.bilibili.com", room, cmdstr, len);
	if (ret) {
		//连接失败 在下一次心跳时重新连接
		m_rinfo[room].needconnect = true;
		return 0;
	}
	m_rinfo[room].needconnect = false;

	return 0;
}
