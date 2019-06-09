#include "stdafx.h"
#include <ctime>
#include <iostream>
#include <list>
#include "BilibiliDanmuWS.h"
#include "proto_bl.h"
#include "log.h"

// const char DM_WSSERVER[] = "ws://broadcastlv.chat.bilibili.com:2244/sub";
const char DM_WSSSERVER[] = "wss://broadcastlv.chat.bilibili.com:443/sub";

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
		int type = protobl::CheckMessage(precv + pos);
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
		info.ver = msg[pos + 7];
		info.len = ireclen - 16;
		if (ireclen > 16) {
			info.buff.reset(new char[info.len + 1]);
			memcpy(info.buff.get(), msg.c_str() + pos + 16, info.len);
			info.buff.get()[info.len] = 0;
		}
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
	printf("IO count: %ld \n", m_connection_list.size());
}

int CWSDanmu::SendConnectionInfo(connection_metadata *it) {
	unsigned char cmdstr[128];
	int len = protobl::MakeWebConnectionInfo(cmdstr, 128, it->get_id());
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
	int len = protobl::MakeWebHeartInfo(cmdstr, 128);
	websocketpp::lib::error_code ec;
	m_client.send(it.get_hdl(), cmdstr, len, websocketpp::frame::opcode::binary, ec);
	if (ec) {
		BOOST_LOG_SEV(g_logger::get(), error) << "[DMWS] Error sending heart message: " << ec.message();
		return -1;
	}

	return 0;
}
