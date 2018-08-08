#include "stdafx.h"
#include "BilibiliDanmuWS.h"
#include <ctime>
using namespace rapidjson;
#include <iostream>
#include <list>
using namespace std;

CWSDanmu::CWSDanmu() {
	_isworking = false;
}

CWSDanmu::~CWSDanmu() {
	Deinit();
}

void CWSDanmu::on_timer(websocketpp::lib::error_code const & ec) {
	if (ec) {
		// there was an error, stop telemetry
		m_endpoint.get_alog().write(websocketpp::log::alevel::app,
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
		this->ConnectToRoom(recon_list.front(), room_list[recon_list.front()].flag);
		recon_list.pop_front();
	}

	// Start the timer for next heart
	this->set_timer(30000);
}

void CWSDanmu::on_open(connection_metadata *it) {
	printf("[WSDanmu] WebSocket Open: %d\n", it->get_id());
	SendConnectionInfo(it);
}

void CWSDanmu::on_fail(connection_metadata *it) {
	printf("[WSDanmu] WebSocket Error: %s\n", it->get_error_reason().c_str());
}

void CWSDanmu::on_close(connection_metadata *it) {
	printf("[WSDanmu] WebSocket Close: %s\n", it->get_error_reason().c_str());
}

void CWSDanmu::on_message(connection_metadata *it, std::string &msg, int len) {
	int label = it->get_id();
	const unsigned char *precv = (const unsigned char *)msg.c_str();
	int pos = 0, ireclen;
	while (pos < len) {
		if (len < pos + 16) {
			printf("[WSDanmu] Warning: Header missing len: %d \n", len);
			if (pos) {
				msg = msg.substr(pos, len - pos);
			}
			return;
		}
		ireclen = precv[pos + 1] << 16 | precv[pos + 2] << 8 | precv[pos + 3];
		if (ireclen < 16 || ireclen > 5000) {
			// The length of datapack is abnormal
			printf("[WSDanmu] Error: Data pack is oversize: %d \n", ireclen);
			msg.clear();
			return;
		}
		if (len < pos + ireclen) {
			printf("[WSDanmu] Warning: Data pack missing: %d:%d \n", len - pos, ireclen);
			if (pos) {
				msg = msg.substr(pos, len - pos);
			}
			return;
		}
		int type = CheckMessage(precv + pos);
		if (type == -1) {
			// The header is wrong
			printf("[WSDanmu] Error: Data pack check failed! \n");
			msg.clear();
			return;
		}
		msg.append(1, 0);
		ProcessData((const unsigned char *)msg.c_str() + pos + 16, ireclen - 16, label, type);
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
	this->closeall();

	_isworking = false;
	return 0;
}

int CWSDanmu::ConnectToRoom(int room, DANMU_FLAG flag) {
	int ret;
	std::string url = DM_WSSERVER;
	ret = this->connect(room, url);
	if (ret) {
		printf("[WSDanmu] Connect to %d failed\n", room);
		return -1;
	}
	ROOM_INFO info;
	info.flag = flag;
	room_list[room] = info;

	return 0;
}

int CWSDanmu::ShowCount() {
	printf("Map count: %d IO count: %d \n", room_list.size(), m_connection_list.size());
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
	m_endpoint.send(it->get_hdl(), cmdstr, len, websocketpp::frame::opcode::binary, ec);
	if (ec) {
		std::cout << "[WSDanmu] Error sending connection message: " << ec.message() << std::endl;
		return -1;
	}

	return 0;
}

int CWSDanmu::SendHeartInfo(connection_metadata &it) {
	unsigned char cmdstr[128];
	int len = MakeHeartInfo(cmdstr, 128);
	websocketpp::lib::error_code ec;
	m_endpoint.send(it.get_hdl(), cmdstr, len, websocketpp::frame::opcode::binary, ec);
	if (ec) {
		std::cout << "[WSDanmu] Error sending heart message: " << ec.message() << std::endl;
		return -1;
	}

	return 0;
}
