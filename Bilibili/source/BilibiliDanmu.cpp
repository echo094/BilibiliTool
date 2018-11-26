#include "stdafx.h"
#include <algorithm>
#include "BilibiliDanmu.h"
#include "proto_bl.h"
#include "log.h"

const char DM_TCPSERVER[] = "broadcastlv.chat.bilibili.com";
const int DM_TCPPORT = 2243;

CBilibiliDanmu::CBilibiliDanmu() {
	BOOST_LOG_SEV(g_logger::get(), debug) << "[DMWIN] Create.";
	_isworking = false;
}

CBilibiliDanmu::~CBilibiliDanmu() {
	stop();
	BOOST_LOG_SEV(g_logger::get(), debug) << "[DMWIN] Destroy.";
}

// 函数退出后相关IO被自动清理
int CBilibiliDanmu::OnClose(PER_SOCKET_CONTEXT* pSocketContext) {
	int room = pSocketContext->label;
	if (!source_base::is_exist(room)) {
		BOOST_LOG_SEV(g_logger::get(), warning) << "[DMWIN] Worker Close Room: " << room;
		return 0;
	}
	source_base::do_list_del(room);
	// 房间在列表中
	if (m_list_closing.count(room)) {
		BOOST_LOG_SEV(g_logger::get(), info) << "[DMWIN] Normal Close Room: " << room;
		m_list_closing.erase(room);
	}
	else {
		// 意外关闭 将其标志为需要重连
		BOOST_LOG_SEV(g_logger::get(), warning) << "[DMWIN] Abnormal Close Room: " << room;
		m_list_reconnect.push_back(room);
	}

	return 0;
}

int CBilibiliDanmu::OnReceive(PER_SOCKET_CONTEXT* pSocketContext, PER_IO_CONTEXT* pIoContext, int byteslen) {
	int type = 0;
	int room = pSocketContext->label;
	const unsigned char *str = (unsigned char *)pIoContext->m_szBuffer;
	int reclen = byteslen + pIoContext->m_occupy;
	int pos = 0, i;
	while (pos < reclen) {
		// 通过前四个字节的数据计算当前数据包长度
		if (pos + 4 > reclen) {
			// 无法获得数据包长度
			BOOST_LOG_SEV(g_logger::get(), warning) << "[DMWIN] " << room 
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
			BOOST_LOG_SEV(g_logger::get(), error) << "[DMWIN] " << room
				<< " Recv bytes last:" << pIoContext->m_occupy << " sum:" << reclen
				<< " remain:" << reclen - pos << " need:" << ireclen;
			pIoContext->m_occupy = 0;
			return -1;
		}
		// 如果剩余数据大于16字节则校验协议头的正确性
		if (pos + 16 <= reclen) {
			type = protobl::CheckMessage(str + pos);
			if (type == -1) {
				// 数据包校验失败
				BOOST_LOG_SEV(g_logger::get(), error) << "[DMWIN] " << room
					<< " Recv bytes last:" << pIoContext->m_occupy << " sum:" << reclen
					<< " remain:" << reclen - pos << " need:" << ireclen;
				pIoContext->m_occupy = 0;
				return -1;
			}
		}
		// 数据包正确但不完整
		if (pos + int(ireclen) > reclen ) {
			BOOST_LOG_SEV(g_logger::get(), debug) << "[DMWIN] " << room
				<< " Leave Recv bytes last:" << pIoContext->m_occupy << " sum:" << reclen
				<< " remain:" << reclen - pos << " need:" << ireclen;
			pIoContext->m_occupy = reclen - pos;
			for (i = 0; i < pIoContext->m_occupy; i++) {
				pIoContext->m_szBuffer[i] = pIoContext->m_szBuffer[pos + i];
			}
			return 0;
		}

		// 转发给其它函数处理
		MSG_INFO info;
		info.id = room;
		info.opt = get_info(room).opt;
		info.type = type;
		info.ver = pIoContext->m_szBuffer[pos + 7];
		info.len = ireclen - 16;
		if (ireclen > 16) {
			info.buff.reset(new char[info.len + 1]);
			memcpy_s(info.buff.get(), info.len, pIoContext->m_szBuffer + (pos + 16), info.len);
			info.buff.get()[info.len] = 0;
		}
		handler_msg(&info);

		// 移动指针
		pos += ireclen;
	}

	// 当数据包有错误或者数据包不完整时会直接在while语句内退出本方法
	// 当指针正好指向末尾时才会跳出while语句执行到此处
	// 遗留数据处理完毕
	if (pIoContext->m_occupy) {
		BOOST_LOG_SEV(g_logger::get(), debug) << "[DMWIN] " << room
			<< " Clear Recv bytes last:" << pIoContext->m_occupy << " sum:" << reclen
			<< " remain:" << pos - reclen;
		pIoContext->m_occupy = 0;
	}

	return 0;
}

int CBilibiliDanmu::OnHeart() {
	BOOST_LOG_SEV(g_logger::get(), debug) << "[DMWIN] Heart start. ";
	int ret, len, room;
	// 向所有活动连接发送心跳包
	EnterCriticalSection(&m_csContextList);
	char cmdstr[60];
	std::list<PER_SOCKET_CONTEXT*>::iterator m_itor;
	for (m_itor = m_arrayClientContext.begin(); m_itor != m_arrayClientContext.end();) {
		if ((*m_itor)->isdroped) {
			// 跳过关闭中的
			m_itor++;
			continue;
		}
		room = (*m_itor)->label;
		len = protobl::MakeFlashHeartInfo((unsigned char *)cmdstr, 60, room);
		ret = _PostSend(*m_itor, cmdstr, len);
		if (ret) {
			// 发送失败则断开连接 必须保证没有Worker线程正在处理该连接的数据
			BOOST_LOG_SEV(g_logger::get(), warning) << "[DMWIN] " << room
				<< " Heart failed code:" << ret;
			// 清理当前连接
			this->_DisConnectSocketHard(m_itor);
			delete (*m_itor);
			m_itor = m_arrayClientContext.erase(m_itor);
			// 标记为需要重连
			source_base::do_list_del(room);
			m_list_reconnect.push_back(room);
			continue;
		}
		m_itor++;
	}
	LeaveCriticalSection(&m_csContextList);
	// 重连断开的房间
	// 一次心跳最多连接300个
	int maxcount = 300;
	while (maxcount && !m_list_reconnect.empty()) {
		room = m_list_reconnect.front();
		m_list_reconnect.pop_front();
		if (source_base::is_exist(room)) {
			continue;
		}
		maxcount--;
		SendConnectionInfo(room);
	}
	BOOST_LOG_SEV(g_logger::get(), debug) << "[DMWIN] Heart finish. ";

	return 0;
}

//开启主监视线程 初始化IOCP类
int CBilibiliDanmu::start() {
	if (_isworking)
		return -1;
	// 开启心跳模块
	this->SetHeart(true, 30000);
	this->Initialize(0);

	_isworking = true;

	return 0;
}

//退出所有线程 断开Socket 释放所有IOCP资源
int CBilibiliDanmu::stop() {
	if (!_isworking)
		return 0;
	// 更新标识符
	EnterCriticalSection(&m_csContextList);
	for (auto it = m_arrayClientContext.begin(); it != m_arrayClientContext.end(); it++) {
		m_list_closing.insert((*it)->label);
	}
	LeaveCriticalSection(&m_csContextList);
	// 关闭IOCP客户端
	this->Destory();
	// 清理列表
	source_base::stop();

	_isworking = false;

	return 0;
}

int CBilibiliDanmu::add_context(const unsigned id, const ROOM_INFO& info) {
	// 添加房间至列表
	source_base::do_info_add(id, info);
	m_list_reconnect.push_back(id);

	return 0;
}

int CBilibiliDanmu::del_context(const unsigned id) {
	if (!source_base::is_exist(id)) {
		return -1;
	}
	m_list_closing.insert(id);
	this->CloseConnectionByLabel(id);

	return 0;
}

int CBilibiliDanmu::update_context(std::set<unsigned> &nlist, const unsigned opt) {
	using namespace std;
	BOOST_LOG_SEV(g_logger::get(), debug) << "[DMWIN] Update start. ";
	set<unsigned> dlist, ilist;
	EnterCriticalSection(&cslist_);
	// 房间下播后还会在列表存在一段时间
	// 生成新增房间列表
	set_difference(nlist.begin(), nlist.end(), con_list_.begin(), con_list_.end(), inserter(ilist, ilist.end()));
	// 生成关播房间列表
	for (auto it = con_list_.begin(); it != con_list_.end(); it++) {
		if (con_info_[(*it)].needclose) {
			dlist.insert(*it);
		}
	}
	LeaveCriticalSection(&cslist_);
	// 输出操作概要
	BOOST_LOG_SEV(g_logger::get(), info) << "[DMWIN] Status:"
		<< " Current: " << con_list_.size()
		<< " New: " << nlist.size()
		<< " Add: " << ilist.size()
		<< " Remove: " << dlist.size();
	// 断开关播房间
	for (auto it = dlist.begin(); it != dlist.end(); it++) {
		del_context(*it);
	}
	// 连接新增房间
	for (auto it = ilist.begin(); it != ilist.end(); it++) {
		ROOM_INFO info;
		info.id = *it;
		info.opt = opt;
		add_context(*it, info);
	}
	BOOST_LOG_SEV(g_logger::get(), debug) << "[DMWIN] Update finish. ";
	return 0;
}

void CBilibiliDanmu::show_stat() {
	source_base::show_stat();
	printf("IO count: %d \n", m_arrayClientContext.size());
}

int CBilibiliDanmu::SendConnectionInfo(int room) {
	if (source_base::is_exist(room)) {
		return 0;
	}
	//构造发送的字符串
	int ret, len;
	char cmdstr[60];
	len = protobl::MakeFlashConnectionInfo((unsigned char *)cmdstr, 60, room);
	// 在添加连接信息至列表时会加线程锁
	ret = this->Connect(DM_TCPPORT, DM_TCPSERVER, room, cmdstr, len);
	if (ret) {
		//连接失败 在下一次心跳时重新连接
		m_list_reconnect.push_back(room);
		return 0;
	}
	// 连接成功
	source_base::do_list_add(room);

	return 0;
}
