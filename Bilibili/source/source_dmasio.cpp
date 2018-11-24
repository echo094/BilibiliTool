#include "stdafx.h"
#include "source/source_dmasio.h"
#include "log.h"
#include <iomanip>
#include <iostream>
#include <sstream>

const std::string Char2HexString(unsigned char * data, int nLen) {
	using namespace std;
	ostringstream oss;
	oss << hex << setfill('0');
	for (int i = 0; i < nLen; i++) {
		oss << setw(2) << static_cast<unsigned int>(data[i]) << ' ';
	}
	return oss.str();
}

source_dmasio::source_dmasio() :
	asioclient_(16),
	is_stop_(false) {
	BOOST_LOG_SEV(g_logger::get(), debug) << "[DMAS] Create.";
}

source_dmasio::~source_dmasio() {
	BOOST_LOG_SEV(g_logger::get(), debug) << "[DMAS] Destroy.";
}

int source_dmasio::start() {
	using std::placeholders::_1;
	using std::placeholders::_2;
	asioclient_.set_error_handler(
		std::bind(&source_dmasio::on_error, this, _1, _2)
	);
	asioclient_.set_open_handler(
		std::bind(&source_dmasio::on_open, this, _1)
	);
	asioclient_.set_timer_handler(
		std::bind(&source_dmasio::on_heart, this, _1)
	);
	asioclient_.set_header_handler(
		std::bind(&source_dmasio::on_header, this, _1, _2)
	);
	asioclient_.set_payload_handler(
		std::bind(&source_dmasio::on_payload, this, _1, _2)
	);

	asioclient_.init(DM_TCPSERVER, DM_TCPPORTSTR, 30);
	is_stop_ = false;
	return 0;
}

int source_dmasio::stop() {
	is_stop_ = true;
	asioclient_.deinit();
	source_base::stop();
	return 0;
}

int source_dmasio::add_context(const unsigned id, const ROOM_INFO & info) {
	if (source_base::is_exist(id)) {
		return -1;
	}
	asioclient_.connect(id);
	source_base::do_info_add(id, info);
	return 0;
}

int source_dmasio::del_context(const unsigned id) {
	if (!source_base::is_exist(id)) {
		return -1;
	}
	list_closing_.insert(id);
	asioclient_.disconnect(id);
	return 0;
}

int source_dmasio::update_context(std::set<unsigned>& nlist, const unsigned opt) {
	using namespace std;
	BOOST_LOG_SEV(g_logger::get(), debug) << "[DMAS] Update start. ";
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
	BOOST_LOG_SEV(g_logger::get(), info) << "[DMAS] Status:"
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
	BOOST_LOG_SEV(g_logger::get(), debug) << "[DMAS] Update finish. ";
	return 0;
}

void source_dmasio::show_stat() {
	source_base::show_stat();
	printf("IO count: %d \n", asioclient_.get_ionum());
}

void source_dmasio::on_error(const unsigned id, const boost::system::error_code ec) {
	source_base::do_list_del(id);
	if (is_stop_) {
		BOOST_LOG_SEV(g_logger::get(), info) << "[DMAS] Final Close: " << id
			<< " code: " << ec;
	}
	else if (list_closing_.count(id)) {
		BOOST_LOG_SEV(g_logger::get(), info) << "[DMAS] Normal Close: " << id
			<< " code: " << ec;
		list_closing_.erase(id);
	}
	else {
		BOOST_LOG_SEV(g_logger::get(), warning) << "[DMAS] Abnormal Close: " << id
			<< " code: " << ec;
		asioclient_.connect(id);
	}
}

void source_dmasio::on_open(context_info * c) {
	BOOST_LOG_SEV(g_logger::get(), info) << "[DMAS] Open: " << c->label_;
	source_base::do_list_add(c->label_);
	char cmdstr[128];
	int len = MakeConnectionInfo((unsigned char *)cmdstr, 128, c->label_);
	asioclient_.post_write(c, cmdstr, len);
}

void source_dmasio::on_heart(context_info * c) {
	char cmdstr[128];
	int len = MakeHeartInfo((unsigned char *)cmdstr, 128, c->label_);
	asioclient_.post_write(c, cmdstr, len);
}

size_t source_dmasio::on_header(context_info * c, const int len) {
	unsigned char *data = (unsigned char *)c->buff_header_;
	size_t bufflen = data[0] << 24 | data[1] << 16 | data[2] << 8 | data[3];
	int type = CheckMessage(data);
	if ((bufflen > 5000) || (type == -1)) {
		BOOST_LOG_SEV(g_logger::get(), error) << "[DMAS] Recv: " << c->label_ 
			<< " bytes: " << Char2HexString(data, len);
		return 0;
	}
	c->opt_ = type;
	if (bufflen == 16) {
		MSG_INFO info;
		info.id = c->label_;
		info.opt = get_info(c->label_).opt;
		info.type = c->opt_;
		info.msg = "";
		handler_msg(&info);
	}
	return bufflen;
}

size_t source_dmasio::on_payload(context_info * c, const int len) {
	unsigned char *data = (unsigned char *)c->buff_payload_;
	MSG_INFO info;
	info.id = c->label_;
	info.opt = get_info(c->label_).opt;
	info.type = c->opt_;
	info.msg = "";
	info.msg.append(c->buff_payload_, len);
	info.msg.append(1, 0);
	handler_msg(&info);
	return 0;
}

long long source_dmasio::GetRUID() {
	srand(unsigned(time(0)));
	double val = 100000000000000.0 + 200000000000000.0*(rand() / (RAND_MAX + 1.0));
	return static_cast <long long> (val);
}

int source_dmasio::CheckMessage(const unsigned char *str) {
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

int source_dmasio::MakeConnectionInfo(unsigned char* str, int len, int room) {
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

int source_dmasio::MakeHeartInfo(unsigned char* str, int len, int room) {
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
