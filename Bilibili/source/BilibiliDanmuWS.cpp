#include "stdafx.h"
#include "BilibiliDanmuWS.h"
#include "log.h"
#include <ctime>
#include <iostream>
#include <list>

CWSDanmu::CWSDanmu() {
	BOOST_LOG_SEV(g_logger::get(), debug) << "[DMWS] Create.";
	_isworking = false;
}

CWSDanmu::~CWSDanmu() {
	stop();
	BOOST_LOG_SEV(g_logger::get(), debug) << "[DMWS] Destroy.";
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
		this->add_context(rid, get_info(rid));
		recon_list.pop_front();
	}

	// Start the timer for next heart
	this->set_timer(30000);
}

void CWSDanmu::on_open(connection_metadata *it) {
	BOOST_LOG_SEV(g_logger::get(), info) << "[DMWS] WebSocket Open: " << it->get_id();
	SendConnectionInfo(it);
}

void CWSDanmu::on_fail(connection_metadata *it) {
	BOOST_LOG_SEV(g_logger::get(), warning) << "[DMWS] WebSocket Error: " << it->get_id() << " " << it->get_error_reason();
}

void CWSDanmu::on_close(connection_metadata *it) {
	BOOST_LOG_SEV(g_logger::get(), info) << "[DMWS] WebSocket Close: " << it->get_id() << " " << it->get_error_reason();
}

void CWSDanmu::on_message(connection_metadata *it, std::string &msg, int len) {
	int label = it->get_id();
	int pos = 0, ireclen;
	while (pos < len) {
		const unsigned char *precv = (const unsigned char *)msg.c_str();
		if (len < pos + 16) {
			BOOST_LOG_SEV(g_logger::get(), warning) << "[DMWS] Header missing len: " << len;
			if (pos) {
				msg = msg.substr(pos, len - pos);
			}
			return;
		}
		ireclen = precv[pos + 1] << 16 | precv[pos + 2] << 8 | precv[pos + 3];
		if (ireclen < 16 || ireclen > 5000) {
			// The length of datapack is abnormal
			BOOST_LOG_SEV(g_logger::get(), error) << "[DMWS] Data pack is oversize: " << ireclen;
			msg.clear();
			return;
		}
		if (len < pos + ireclen) {
			BOOST_LOG_SEV(g_logger::get(), warning) << "[DMWS] Data pack missing: " << len - pos << ":" << ireclen;
			if (pos) {
				msg = msg.substr(pos, len - pos);
			}
			return;
		}
		int type = CheckMessage(precv + pos);
		if (type == -1) {
			// The header is wrong
			BOOST_LOG_SEV(g_logger::get(), error) << "[DMWS] Data pack check failed!";
			msg.clear();
			return;
		}
		MSG_INFO info;
		info.id = label;
		info.opt = get_info(label).opt;
		info.type = type;
		info.msg = msg.substr(pos + 16, ireclen - 16);
		info.msg.append(1, 0);
		handler_msg(&info);
		pos += ireclen;
	}
	msg.clear();
}

int CWSDanmu::start() {
	if (_isworking)
		return -1;

	this->set_timer(30000);

	_isworking = true;
	return 0;
}

int CWSDanmu::stop() {
	if (!_isworking)
		return 0;

	this->cancel_timer();
	// 这里会触发事件但不会进入继承的 on_close 函数
	this->closeall();
	// 清理列表
	source_base::stop();

	_isworking = false;
	return 0;
}

int CWSDanmu::add_context(const unsigned id, const ROOM_INFO& info) {
	int ret;
	std::string url = DM_WSSSERVER;
	ret = this->connect(id, url);
	if (ret) {
		BOOST_LOG_SEV(g_logger::get(), error) << "[DMWS] Connect to " << id << " failed!";
		return -1;
	}
	source_base::do_info_add(id, info);
	source_base::do_list_add(id);

	return 0;
}

int CWSDanmu::del_context(const unsigned id) {
	if (!source_base::is_exist(id)) {
		return -1;
	}
	this->close(id, websocketpp::close::status::going_away, "");
	// 从列表清除该房间
	source_base::do_list_del(id);

	return 0;
}

int CWSDanmu::update_context(std::set<unsigned> &nlist, const unsigned opt) {
	return 0;
}

void CWSDanmu::show_stat() {
	source_base::show_stat();
	printf("IO count: %d \n", m_connection_list.size());
}

int CWSDanmu::CheckMessage(const unsigned char *str) {
	int i;
	if (str[4])
		return -1;
	if (str[5] - 16)
		return -1;
	for (i = 8; i < 11; i++) {
		if (str[i])
			return -1;
	}
	for (i = 12; i < 15; i++) {
		if (str[i])
			return -1;
	}
	return str[11];
}

int CWSDanmu::MakeConnectionInfo(unsigned char* str, int len, int room) {
	memset(str, 0, len);
	int buflen;
	//构造发送的字符串
	buflen = sprintf_s((char*)str + 16, len - 16, "{\"uid\":0,\"roomid\":%d,\"protover\":1,\"platform\":\"web\",\"clientver\":\"1.5.10\"}", room);
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
		BOOST_LOG_SEV(g_logger::get(), error) << "[DMWS] Error sending connection message: " << ec.message();
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
		BOOST_LOG_SEV(g_logger::get(), error) << "[DMWS] Error sending heart message: " << ec.message();
		return -1;
	}

	return 0;
}
