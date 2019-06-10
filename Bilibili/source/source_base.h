#pragma once
#include <functional>
#include <map>
#include <set>
#include <boost/thread/lock_types.hpp>
#include <boost/thread/shared_mutex.hpp>
#include "BilibiliStruct.h"

typedef struct _ROOM_INFO {
	// 编号
	unsigned id;
	// 参数设置
	unsigned opt;
	// 需要关闭（下播时）
	bool needclose;

	_ROOM_INFO() :
		id(0),
		opt(0),
		needclose(false) {}

}ROOM_INFO;

class source_base {
public:
	typedef std::function<void(MSG_INFO *)> msg_handler;
	typedef std::function<void(unsigned, unsigned)> close_handler;

public:
	source_base() :
		msg_handler_(nullptr),
		close_handler_(nullptr) {
	}
	virtual ~source_base() {
	}

public:
	virtual int start() = 0;
	virtual int stop();
	virtual int add_context(const unsigned id, const ROOM_INFO& info) = 0;
	virtual int del_context(const unsigned id) = 0;
	virtual int update_context(std::set<unsigned> &nlist, const unsigned opt) = 0;
	virtual void show_stat();

public:
	void set_msg_handler(msg_handler h) {
		msg_handler_ = h;
	}
	void set_close_handler(close_handler h) {
		close_handler_ = h;
	}
	void set_con_stat(const unsigned id, const bool stat) {
		con_info_[id].needclose = stat;
	}

protected:
	void handler_msg(MSG_INFO *info) {
		if (msg_handler_) {
			msg_handler_(info);
		}
	}
	void handler_close(unsigned rrid, unsigned opt) {
		if (close_handler_) {
			close_handler_(rrid, opt);
		}
	}
	ROOM_INFO get_info(const unsigned id) {
		return con_info_[id];
	}
	bool is_exist(const unsigned id);
	void do_info_add(const unsigned id, const ROOM_INFO& info);
	void do_list_add(const unsigned id);
	void do_list_del(const unsigned id);

protected:
	msg_handler msg_handler_;
	close_handler close_handler_;
	// Connection list
	std::set<unsigned> con_list_;
	// Connection info
	std::map<unsigned, ROOM_INFO> con_info_;
	// Lock of list
	boost::shared_mutex mutex_list_;
};
