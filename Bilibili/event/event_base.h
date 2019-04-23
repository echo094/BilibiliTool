#pragma once
#include <functional>
#include <map>

const unsigned MSG_NEWLOTTERY = WM_USER + 612;
const unsigned MSG_NEWSPECIALGIFT = WM_USER + 613;
const unsigned MSG_NEWGUARD1 = WM_USER + 614;
const unsigned MSG_NEWGUARD0 = WM_USER + 615;
const unsigned MSG_CHANGEROOM1 = WM_USER + 616;
const unsigned MSG_CHANGEROOM2 = WM_USER + 617;
const unsigned MSG_CLOSEROOM = WM_USER + 618;

class event_base {
private:
	typedef std::function<void(unsigned, WPARAM, LPARAM)> event_handler;
public:
	event_base():
		event_handler_(nullptr) {
	}
	virtual ~event_base() {
	}

public:
	// 设置父级线程ID
	void set_event_handler(event_handler h);
	void connection_close(unsigned rrid, unsigned opt);
	virtual void process_data(MSG_INFO *data) = 0;

protected:
	void post_lottery_msg(unsigned rrid);
	void post_storm_msg(BILI_ROOMEVENT *pinfo);
	void post_guard1_msg(unsigned rrid);
	void post_guard23_msg(BILI_LOTTERYDATA *pinfo);
	void post_close_event(unsigned rrid, unsigned opt);
	void post_close_msg(unsigned rrid, unsigned opt);
	void post_open_msg(unsigned rrid, unsigned opt);

private:
	event_handler event_handler_;
};
