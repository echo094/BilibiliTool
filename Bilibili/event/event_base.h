#pragma once
#include <functional>
#include <map>

enum {
	MSG_NEWLOTTERY = 612,
	MSG_NEWSPECIALGIFT,
	MSG_NEWGUARD1,
	MSG_NEWGUARD0,
	MSG_CHANGEROOM1,
	MSG_CHANGEROOM2,
	MSG_CLOSEROOM
};

class event_base {
private:
	typedef std::function<void(unsigned, std::shared_ptr<BILI_LOTTERYDATA>)> event_act;
	typedef std::function<void(unsigned, unsigned, unsigned)> event_room;
public:
	event_base() :
		event_act_(nullptr),
		event_room_(nullptr) {
	}
	virtual ~event_base() {
	}

public:
	// 设置父级线程ID
	void set_event_act(event_act h);
	void set_event_room(event_room h);
	void connection_close(unsigned rrid, unsigned opt);
	virtual void process_data(MSG_INFO *data) = 0;

protected:
	void post_lottery_msg(unsigned rrid);
	void post_storm_msg(std::shared_ptr<BILI_LOTTERYDATA> data);
	void post_guard1_msg(unsigned rrid);
	void post_guard23_msg(std::shared_ptr<BILI_LOTTERYDATA> data);
	void post_close_event(unsigned rrid, unsigned opt);
	void post_close_msg(unsigned rrid, unsigned opt);
	void post_open_msg(unsigned rrid, unsigned opt);

private:
	event_act event_act_;
	event_room event_room_;
};
