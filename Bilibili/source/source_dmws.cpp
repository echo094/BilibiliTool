#include "source_dmws.h"
#include <ctime>
#include <iostream>
#include <list>
#include <zlib.h>
#include <boost/bind.hpp>
#include "logger/log.h"
#include "proto_bl.h"

const char DM_WS_HOST[] = "broadcastlv.chat.bilibili.com";
const char DM_WS_PARAM[] = "/sub";
#ifdef USE_WSS
const char DM_WS_PORT[] = "443";
#else
const char DM_WS_PORT[] = "2244";
#endif

source_dmws::source_dmws():
	source_base(),
	WsClientV2(),
	heart_timer_(io_ctx_),
	_isworking(false)
{
	BOOST_LOG_SEV(g_logger::get(), debug) << "[DMWS] Create.";
}

source_dmws::~source_dmws() {
	stop();
	BOOST_LOG_SEV(g_logger::get(), debug) << "[DMWS] Destroy.";
}

void source_dmws::start_timer()
{
	heart_timer_.expires_from_now(boost::posix_time::seconds(30));
	heart_timer_.async_wait(
		boost::bind(
			&source_dmws::on_timer,
			this,
			boost::asio::placeholders::error
		)
	);
}

void source_dmws::on_timer(boost::system::error_code ec) {
	if (ec) {
		return;
	}

	for (auto it = sessions_.begin(); it != sessions_.end(); ++it) {
		// Send heart data
		SendHeartInfo(*it);
	}

	// Start the timer for next heart
	start_timer();
}

void source_dmws::on_open(std::shared_ptr<session_ws> c) {
	BOOST_LOG_SEV(g_logger::get(), info) << "[DMWS] WebSocket Open: " << c->label_;
	source_base::do_list_add(c->label_);
	SendConnectionInfo(c);
}

void source_dmws::on_fail(unsigned label) {
	BOOST_LOG_SEV(g_logger::get(), warning) << "[DMWS] WebSocket Error: " << label;
	// 从列表清除该房间
	source_base::do_list_del(label);
	if (_isworking) {
		// 重连
		add_context(label, get_info(label));
	}
}

void source_dmws::on_close(unsigned label) {
	BOOST_LOG_SEV(g_logger::get(), info) << "[DMWS] WebSocket Close: " << label;
	// 从列表清除该房间
	source_base::do_list_del(label);
}

void source_dmws::on_message(std::shared_ptr<session_ws> c, size_t len) {
	int label = c->label_;
	size_t pos = 0, ireclen;
	std::string msg = beast::buffers_to_string(c->buffer_.data());
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
		process_data(
			msg.c_str() + pos,
			ireclen,
			label,
			get_info(label).opt,
			type
		);
		pos += ireclen;
	}
	msg.clear();
}

void source_dmws::process_data(const char *buff,
	const unsigned ilen,
	const unsigned id,
	const unsigned opt,
	const unsigned type
) {
	if (buff[7] != 2) {
		// 数据包未压缩
		MSG_INFO info;
		info.id = id;
		info.opt = opt;
		info.type = type;
		info.ver = buff[7];
		info.len = ilen - 16;
		if (ilen > 16) {
			info.buff.reset(new char[info.len + 1]);
			memcpy(info.buff.get(), buff + 16, info.len);
			info.buff.get()[info.len] = 0;
		}
		handler_msg(&info);
		return;
	}

	// 需要解压数据包
	// 初始化zlib
	z_stream strm;
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = 0;
	strm.next_in = Z_NULL;
	if (inflateInit(&strm) != Z_OK) {
		BOOST_LOG_SEV(g_logger::get(), error) << "[DMAS] Recv: " << id
			<< " Zlib init failed! ";
		return;
	}
	strm.avail_in = ilen - 16;
	strm.next_in = (unsigned char *)buff + 16;
	int ret = 0;
	bool success = true;
	do {
		// 解压header
		unsigned char head[17] = {};
		strm.avail_out = 16;
		strm.next_out = head;
		ret = inflate(&strm, Z_NO_FLUSH);
		if (ret != Z_OK && ret != Z_STREAM_END) {
			// 解压出错
			success = false;
			break;
		}
		if (strm.avail_out) {
			// header未完整获取
			success = false;
			break;
		}
		// 获取header信息
		MSG_INFO info;
		info.id = id;
		info.opt = opt;
		info.type = protobl::CheckMessage(head);
		info.ver = head[7];
		info.len = head[0] << 24 | head[1] << 16 | head[2] << 8 | head[3];
		info.len -= 16;
		info.buff.reset(new char[info.len + 1]);
		if (info.type == -1) {
			success = false;
			break;
		}
		// 解压payload
		strm.avail_out = info.len;
		strm.next_out = (unsigned char *)info.buff.get();
		ret = inflate(&strm, Z_NO_FLUSH);
		if (ret != Z_OK && ret != Z_STREAM_END) {
			// 解压出错
			success = false;
			break;
		}
		if (strm.avail_out) {
			// payload未完整获取
			success = false;
			break;
		}
		info.buff.get()[info.len] = 0;
		handler_msg(&info);
	} while (ret == Z_OK);

	if (!success) {
		BOOST_LOG_SEV(g_logger::get(), error) << "[DMAS] Recv: " << id
			<< " compress bytes: " << char2hexstring((unsigned char*)buff, ilen);
	}

	// clean up and return
	(void)inflateEnd(&strm);
	return;
}

int source_dmws::start() {
	if (_isworking) {
		return -1;
	}
	_isworking = true;

	start_timer();

	return 0;
}

int source_dmws::stop() {
	if (!_isworking) {
		return 0;
	}
	_isworking = false;

	heart_timer_.cancel();
	// 这里会触发事件但不会进入继承的 on_close 函数
	close_all();
	// 清理列表
	source_base::stop();

	return 0;
}

int source_dmws::add_context(const unsigned id, const ROOM_INFO& info) 
{
	connect(id, DM_WS_HOST, DM_WS_PORT, DM_WS_PARAM, info.key);
	source_base::do_info_add(id, info);

	return 0;
}

int source_dmws::del_context(const unsigned id) {
	if (!source_base::is_exist(id)) {
		return -1;
	}
	close_spec(id);

	return 0;
}

int source_dmws::clean_context(std::set<unsigned> &nlist) {
	return 0;
}

void source_dmws::show_stat() {
	source_base::show_stat();
	printf("IO count: %ld \n", sessions_.size());
}

int source_dmws::SendConnectionInfo(std::shared_ptr<session_ws> c) {
	unsigned char cmdstr[512];
	int len = protobl::MakeWebConnectionInfo(
		cmdstr, 
		sizeof(cmdstr), 
		c->label_, 
		c->key_.c_str()
	);
	std::shared_ptr<std::string> buff(new std::string((const char*)cmdstr, len));
	do_write(c, buff);

	return 0;
}

int source_dmws::SendHeartInfo(std::shared_ptr<session_ws> c) {
	unsigned char cmdstr[128];
	int len = protobl::MakeWebHeartInfo(cmdstr, 128);
	std::shared_ptr<std::string> buff(new std::string((const char*)cmdstr, len));
	do_write(c, buff);

	return 0;
}
