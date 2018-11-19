#include "stdafx.h"
#include "BilibiliDanmu.h"
#include "log.h"
#include <ctime>
#include <algorithm>
#include <iostream>

CBilibiliDanmu::CBilibiliDanmu() {
	InitializeCriticalSection(&m_cslist);
	_isworking = false;
}

CBilibiliDanmu::~CBilibiliDanmu() {
	Deinit();
	DeleteCriticalSection(&m_cslist);
}

// 函数退出后相关IO被自动清理
int CBilibiliDanmu::OnClose(PER_SOCKET_CONTEXT* pSocketContext) {
	int room = pSocketContext->label;
	if (m_rlist.find(room) == m_rlist.end()) {
		BOOST_LOG_SEV(g_logger::get(), warning) << "[Danmu] Worker Close Room: " << room;
		return 0;
	}
	EnterCriticalSection(&m_cslist);
	m_rlist.erase(room);
	LeaveCriticalSection(&m_cslist);
	// 房间在列表中
	if (!m_rinfo[room].needclear) {
		// 意外关闭 将其标志为需要重连
		BOOST_LOG_SEV(g_logger::get(), warning) << "[Danmu] Abnormal Close Room: " << room;
		m_listre.push_back(room);
	}
	BOOST_LOG_SEV(g_logger::get(), info) << "[Danmu] Normal Close Room: " << room;

	return 0;
}

int CBilibiliDanmu::OnReceive(PER_SOCKET_CONTEXT* pSocketContext, PER_IO_CONTEXT* pIoContext, int byteslen) {
	int type;
	int room = pSocketContext->label;
	const unsigned char *str = (unsigned char *)pIoContext->m_szBuffer;
	int reclen = byteslen + pIoContext->m_occupy;
	int pos = 0, i;
	while (pos < reclen) {
		// 通过前四个字节的数据计算当前数据包长度
		if (pos + 4 > reclen) {
			// 无法获得数据包长度
			BOOST_LOG_SEV(g_logger::get(), warning) << "[Danmu] " << room 
				<< " Recv bytes last:" << pIoContext->m_occupy << " sum:" << reclen
				<< " remain:" << reclen - pos << " need:" << 4;
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
			BOOST_LOG_SEV(g_logger::get(), error) << "[Danmu] " << room
				<< " Recv bytes last:" << pIoContext->m_occupy << " sum:" << reclen
				<< " remain:" << reclen - pos << " need:" << ireclen;
			pIoContext->m_occupy = 0;
			return -1;
		}
		// 如果剩余数据大于16字节则校验协议头的正确性
		if (pos + 16 <= reclen) {
			type = CheckMessage(str + pos);
			if (type == -1) {
				// 数据包校验失败
				BOOST_LOG_SEV(g_logger::get(), error) << "[Danmu] " << room
					<< " Recv bytes last:" << pIoContext->m_occupy << " sum:" << reclen
					<< " remain:" << reclen - pos << " need:" << ireclen;
				pIoContext->m_occupy = 0;
				return -1;
			}
		}
		// 数据包正确但不完整
		if (pos + int(ireclen) > reclen ) {
			BOOST_LOG_SEV(g_logger::get(), debug) << "[Danmu] " << room
				<< " Leave Recv bytes last:" << pIoContext->m_occupy << " sum:" << reclen
				<< " remain:" << reclen - pos << " need:" << ireclen;
			pIoContext->m_occupy = reclen - pos;
			for (i = 0; i < pIoContext->m_occupy; i++) {
				pIoContext->m_szBuffer[i] = pIoContext->m_szBuffer[pos + i];
			}
			return 0;
		}

		// 转发给其它函数处理
		pIoContext->m_szBuffer[pos + ireclen] = 0;
		ProcessData(pIoContext->m_szBuffer + (pos + 16), ireclen - 16, room, type);

		// 移动指针
		pos += ireclen;
	}

	// 当数据包有错误或者数据包不完整时会直接在while语句内退出本方法
	// 当指针正好指向末尾时才会跳出while语句执行到此处
	// 遗留数据处理完毕
	if (pIoContext->m_occupy) {
		BOOST_LOG_SEV(g_logger::get(), debug) << "[Danmu] " << room
			<< " Clear Recv bytes last:" << pIoContext->m_occupy << " sum:" << reclen
			<< " remain:" << pos - reclen;
		pIoContext->m_occupy = 0;
	}

	return 0;
}

int CBilibiliDanmu::OnHeart() {
	BOOST_LOG_SEV(g_logger::get(), debug) << "[Danmu] Heart start. ";
	int ret, len, room;
	// 向所有活动连接发送心跳包
	EnterCriticalSection(&m_csContextList);
	char cmdstr[60];
	std::list<toollib::PER_SOCKET_CONTEXT*>::iterator m_itor;
	for (m_itor = m_arrayClientContext.begin(); m_itor != m_arrayClientContext.end();) {
		if ((*m_itor)->isdroped) {
			// 跳过关闭中的
			m_itor++;
			continue;
		}
		room = (*m_itor)->label;
		len = this->MakeHeartInfo((unsigned char *)cmdstr, 60, room);
		ret = _PostSend(*m_itor, cmdstr, len);
		if (ret) {
			// 发送失败则断开连接 必须保证没有Worker线程正在处理该连接的数据
			BOOST_LOG_SEV(g_logger::get(), warning) << "[Danmu] " << room
				<< " Heart failed code:" << ret;
			// 清理当前连接
			this->_DisConnectSocketHard(m_itor);
			delete (*m_itor);
			m_itor = m_arrayClientContext.erase(m_itor);
			// 标记为需要重连
			EnterCriticalSection(&m_cslist);
			m_rlist.erase(room);
			LeaveCriticalSection(&m_cslist);
			m_listre.push_back(room);
			continue;
		}
		m_itor++;
	}
	LeaveCriticalSection(&m_csContextList);
	// 重连断开的房间
	// 一次心跳最多连接300个
	int maxcount = 300;
	while (maxcount && !m_listre.empty()) {
		room = m_listre.front();
		m_listre.pop_front();
		if (m_rlist.count(room)) {
			continue;
		}
		maxcount--;
		SendConnectionInfo(room);
	}
	BOOST_LOG_SEV(g_logger::get(), debug) << "[Danmu] Heart finish. ";

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
	// 更新标识符
	EnterCriticalSection(&m_cslist);
	for (auto it = m_rlist.begin(); it != m_rlist.end(); it++) {
		m_rinfo[(*it)].needclear = true;
	}
	LeaveCriticalSection(&m_cslist);
	// 关闭IOCP客户端
	this->Destory();
	// 清理列表
	m_rinfo.clear();
	m_rlist.clear();

	_isworking = false;

	return 0;
}

int CBilibiliDanmu::AddRoom(const unsigned room, const unsigned area, const DANMU_FLAG flag) {
	// 添加房间至列表
	ROOM_INFO info;
	info.area = area;
	info.flag = flag;
	m_rinfo[room] = info;
	m_listre.push_back(room);

	return 0;
}

int CBilibiliDanmu::DisconnectFromRoom(const unsigned room) {
	if (!m_rlist.count(room)) {
		return -1;
	}
	m_rinfo[room].needclear = true;
	this->CloseConnectionByLabel(room);

	return 0;
}

long long CBilibiliDanmu::GetRUID() {
	srand(unsigned(time(0)));
	double val = 100000000000000.0 + 200000000000000.0*(rand() / (RAND_MAX + 1.0));
	return static_cast <long long> (val);
}

void CBilibiliDanmu::UpdateRoom(std::set<unsigned> &nlist, DANMU_FLAG flag) {
	using namespace std;
	BOOST_LOG_SEV(g_logger::get(), debug) << "[Danmu] Update start. ";
	set<unsigned> dlist, ilist;
	EnterCriticalSection(&m_cslist);
	// 房间下播后还会在列表存在一段时间
	// 生成新增房间列表
	set_difference(nlist.begin(), nlist.end(), m_rlist.begin(), m_rlist.end(), inserter(ilist, ilist.end()));
	// 生成关播房间列表
	for (auto it = m_rlist.begin(); it != m_rlist.end(); it++) {
		if (m_rinfo[(*it)].needclose) {
			dlist.insert(*it);
		}
	}
	LeaveCriticalSection(&m_cslist);
	// 输出操作概要
	BOOST_LOG_SEV(g_logger::get(), info) << "[Danmu] Status:"
		<< " Current: " << m_rlist.size()
		<< " New: " << nlist.size()
		<< " Add: " << ilist.size()
		<< " Remove: " << dlist.size();
	// 断开关播房间
	for (auto it = dlist.begin(); it != dlist.end(); it++) {
		DisconnectFromRoom(*it);
	}
	// 连接新增房间
	for (auto it = ilist.begin(); it != ilist.end(); it++) {
		AddRoom(*it, 0, flag);
	}
	BOOST_LOG_SEV(g_logger::get(), debug) << "[Danmu] Update finish. ";
}

void CBilibiliDanmu::ShowCount() {
	printf("Map count: %d Active count: %d IO count: %d \n", m_rinfo.size(), m_rlist.size(), m_arrayClientContext.size());
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
	if (m_rlist.count(room)) {
		return 0;
	}
	//构造发送的字符串
	int ret, len;
	char cmdstr[60];
	len = this->MakeConnectionInfo((unsigned char *)cmdstr, 60, room);
	// 在添加连接信息至列表时会加线程锁
	ret = this->Connect(DM_TCPPORT, DM_TCPSERVER, room, cmdstr, len);
	if (ret) {
		//连接失败 在下一次心跳时重新连接
		m_listre.push_back(room);
		return 0;
	}
	// 连接成功
	EnterCriticalSection(&m_cslist);
	m_rlist.insert(room);
	LeaveCriticalSection(&m_cslist);

	return 0;
}
