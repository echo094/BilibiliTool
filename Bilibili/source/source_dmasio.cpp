#include "stdafx.h"

#include <zlib.h>

#include "source_dmasio.h"
#include "proto_bl.h"
#include "log.h"

const char DM_TCPSERVER[] = "broadcastlv.chat.bilibili.com";
const char DM_TCPPORTSTR[] = "2243";

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
	source_base::do_info_add(id, info);
	asioclient_.connect(id);
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
	{
		boost::shared_lock<boost::shared_mutex> m(mutex_list_);
		// 房间下播后还会在列表存在一段时间
		// 生成新增房间列表
		set_difference(nlist.begin(), nlist.end(), con_list_.begin(), con_list_.end(), inserter(ilist, ilist.end()));
		// 生成关播房间列表
		for (auto it = con_list_.begin(); it != con_list_.end(); it++) {
			if (con_info_[(*it)].needclose) {
				dlist.insert(*it);
			}
		}
	}
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
	printf("IO count: %ld \n", asioclient_.get_ionum());
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
		// 通知意外断开事件
		handler_close(id, get_info(id).opt);
	}
}

void source_dmasio::on_open(context_info * c) {
	BOOST_LOG_SEV(g_logger::get(), info) << "[DMAS] Open: " << c->label_;
	source_base::do_list_add(c->label_);
	char cmdstr[128];
	int len = protobl::MakeFlashConnectionInfo((unsigned char *)cmdstr, 128, c->label_);
	asioclient_.post_write(c, cmdstr, len);
}

void source_dmasio::on_heart(context_info * c) {
	char cmdstr[128];
	int len = protobl::MakeFlashHeartInfo((unsigned char *)cmdstr, 128, c->label_);
	asioclient_.post_write(c, cmdstr, len);
}

size_t source_dmasio::on_header(context_info * c, const int len) {
	unsigned char *data = (unsigned char *)c->buff_header_;
	size_t bufflen = data[0] << 24 | data[1] << 16 | data[2] << 8 | data[3];
	int type = protobl::CheckMessage(data);
	if ((bufflen > 5000) || (type == -1)) {
		BOOST_LOG_SEV(g_logger::get(), error) << "[DMAS] Recv: " << c->label_ 
			<< " bytes: " << char2hexstring(data, len);
		return 0;
	}
	c->opt_ = type;
	if (bufflen == 16) {
		MSG_INFO info;
		info.id = c->label_;
		info.opt = get_info(c->label_).opt;
		info.type = c->opt_;
		handler_msg(&info);
	}
	return bufflen;
}

size_t source_dmasio::on_payload(context_info * c, const int len) {
	if (c->buff_header_[7] == 2) {
		uncompress_dmpack(c->buff_payload_, len, c->label_, get_info(c->label_).opt);
	}
	else {
		MSG_INFO info;
		info.id = c->label_;
		info.opt = get_info(c->label_).opt;
		info.type = c->opt_;
		info.ver = c->buff_header_[7];
		info.len = len;
		info.buff.reset(new char[len + 1]);
		memcpy(info.buff.get(), c->buff_payload_, len);
		info.buff.get()[len] = 0;
		handler_msg(&info);
	}
	return 0;
}

int source_dmasio::uncompress_dmpack(
	char *buf,
	const unsigned ilen,
	const unsigned id,
	const unsigned opt
) {
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
		return 0;
	}
	strm.avail_in = ilen;
	strm.next_in = (unsigned char *)buf;
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
			<< " compress bytes: " << char2hexstring((unsigned char*)buf, ilen);
	}

	// clean up and return
	(void)inflateEnd(&strm);
	return 0;
}
