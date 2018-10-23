#include "stdafx.h"
#include "BilibiliDanmuWS.h"
#include "log.h"
#include <ctime>
#include <iostream>
#include <list>

CWSDanmu::CWSDanmu() {
	_isworking = false;
}

CWSDanmu::~CWSDanmu() {
	Deinit();
}

void CWSDanmu::on_timer(websocketpp::lib::error_code const & ec) {
	if (ec) {
		// there was an error, stop telemetry
		m_client.get_alog().write(websocketpp::log::alevel::app,
			"Timer Error: " + ec.message());
		return;
	}

	std::list<int> recon_list;
	for (con_list::const_iterator it = m_connection_list.begin(); it != m_connection_list.end(); ++it) {
		if (it->second->get_status() != "Open") {
			// Add wrong connection to reconnect list
			recon_list.push_back(it->second->get_id());
			continue;
		}
		// Send heart data
		this->SendHeartInfo(*(it->second));
	}
	while (recon_list.size()) {
		// Connect to room again
		// This will erase current metadata first and then create new metadata
		// If it failed to connect, this room will lose. 
		int rid = recon_list.front();
		this->ConnectToRoom(rid, m_rinfo[rid].area, m_rinfo[rid].flag);
		recon_list.pop_front();
	}

	// Start the timer for next heart
	this->set_timer(30000);
}

void CWSDanmu::on_open(connection_metadata *it) {
	BOOST_LOG_SEV(g_logger::get(), info) << "[WSDanmu] WebSocket Open: " << it->get_id();
	SendConnectionInfo(it);
}

void CWSDanmu::on_fail(connection_metadata *it) {
	BOOST_LOG_SEV(g_logger::get(), warning) << "[WSDanmu] WebSocket Error: " << it->get_id() << " " << it->get_error_reason();
}

void CWSDanmu::on_close(connection_metadata *it) {
	BOOST_LOG_SEV(g_logger::get(), info) << "[WSDanmu] WebSocket Close: " << it->get_id() << " " << it->get_error_reason();
}

void CWSDanmu::on_message(connection_metadata *it, std::string &msg, int len) {
	int label = it->get_id();
	int pos = 0, ireclen;
	while (pos < len) {
		const unsigned char *precv = (const unsigned char *)msg.c_str();
		if (len < pos + 16) {
			BOOST_LOG_SEV(g_logger::get(), warning) << "[WSDanmu] Header missing len: " << len;
			if (pos) {
				msg = msg.substr(pos, len - pos);
			}
			return;
		}
		ireclen = precv[pos + 1] << 16 | precv[pos + 2] << 8 | precv[pos + 3];
		if (ireclen < 16 || ireclen > 5000) {
			// The length of datapack is abnormal
			BOOST_LOG_SEV(g_logger::get(), error) << "[WSDanmu] Data pack is oversize: " << ireclen;
			msg.clear();
			return;
		}
		if (len < pos + ireclen) {
			BOOST_LOG_SEV(g_logger::get(), warning) << "[WSDanmu] Data pack missing: " << len - pos << ":" << ireclen;
			if (pos) {
				msg = msg.substr(pos, len - pos);
			}
			return;
		}
		int type = CheckMessage(precv + pos);
		if (type == -1) {
			// The header is wrong
			BOOST_LOG_SEV(g_logger::get(), error) << "[WSDanmu] Data pack check failed!";
			msg.clear();
			return;
		}
		msg.append(1, 0);
		ProcessData(msg.c_str() + pos + 16, ireclen - 16, label, type);
		pos += ireclen;
	}
	msg.clear();
}

int CWSDanmu::Init() {
	if (_isworking)
		return -1;

	this->set_timer(30000);

	_isworking = true;
	return 0;
}

int CWSDanmu::Deinit() {
	if (!_isworking)
		return 0;

	this->cancel_timer();
	// 这里会触发事件但不会进入继承的 on_close 函数
	this->closeall();
	// 清理列表
	m_rinfo.clear();
	m_rlist.clear();

	_isworking = false;
	return 0;
}

int CWSDanmu::ConnectToRoom(const unsigned room, const unsigned area, const DANMU_FLAG flag) {
	int ret;
	std::string url = DM_WSSERVER;
	ret = this->connect(room, url);
	if (ret) {
		BOOST_LOG_SEV(g_logger::get(), error) << "[WSDanmu] Connect to " << room  << " failed!";
		return -1;
	}
	ROOM_INFO info;
	info.area = area;
	info.flag = flag;
	m_rinfo[room] = info;
	m_rlist.insert(room);

	return 0;
}

int CWSDanmu::DisconnectFromRoom(const unsigned room) {
	if (!m_rlist.count(room)) {
		return -1;
	}
	this->close(room, websocketpp::close::status::going_away, "");
	// 从列表清除该房间
	m_rinfo.erase(room);
	m_rlist.erase(room);

	return 0;
}

int CWSDanmu::ShowCount() {
	printf("Map count: %d IO count: %d \n", m_rinfo.size(), m_connection_list.size());
	return m_connection_list.size();
}

int CWSDanmu::MakeConnectionInfo(unsigned char* str, int len, int room) {
	memset(str, 0, len);
	int buflen;
	//构造发送的字符串
	buflen = sprintf_s((char*)str + 16, len - 16, "{\"uid\":0,\"roomid\":%d,\"protover\":1,\"platform\":\"web\",\"clientver\":\"1.4.6\"}", room);
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

int CWSDanmu::MakeHeartInfo(unsigned char* str, int len) {
	memset(str, 0, len);
	int buflen;
	//构造发送的字符串
	buflen = sprintf_s((char*)str + 16, len - 16, "[object Object]");
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

int CWSDanmu::SendConnectionInfo(connection_metadata *it) {
	unsigned char cmdstr[128];
	int len = MakeConnectionInfo(cmdstr, 128, it->get_id());
	websocketpp::lib::error_code ec;
	m_client.send(it->get_hdl(), cmdstr, len, websocketpp::frame::opcode::binary, ec);
	if (ec) {
		BOOST_LOG_SEV(g_logger::get(), error) << "[WSDanmu] Error sending connection message: " << ec.message();
		return -1;
	}

	return 0;
}

int CWSDanmu::SendHeartInfo(connection_metadata &it) {
	unsigned char cmdstr[128];
	int len = MakeHeartInfo(cmdstr, 128);
	websocketpp::lib::error_code ec;
	m_client.send(it.get_hdl(), cmdstr, len, websocketpp::frame::opcode::binary, ec);
	if (ec) {
		BOOST_LOG_SEV(g_logger::get(), error) << "[WSDanmu] Error sending heart message: " << ec.message();
		return -1;
	}

	return 0;
}
