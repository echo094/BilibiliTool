#include "event_base.h"
#include "logger/log.h"

void event_base::set_event_act(event_act h) {
	event_act_ = h;
}

void event_base::set_event_room(event_room h) {
	event_room_ = h;
}

void event_base::connection_close(unsigned rrid, unsigned opt) {
	post_close_event(rrid, opt);
}

void event_base::post_lottery_pub(unsigned type, unsigned rrid) {
	BOOST_LOG_SEV(g_logger::get(), info) << "[EVENT] lottery pub room: " << rrid;
	std::shared_ptr<BILI_LOTTERYDATA> data(new BILI_LOTTERYDATA());
	data->cmd = type;
	data->rrid = rrid;
	if (event_act_) {
		event_act_(data);
	}
}

void event_base::post_lottery_hidden(std::shared_ptr<BILI_LOTTERYDATA> data) {
	if (event_act_) {
		event_act_(data);
	}
}

void event_base::post_close_event(unsigned rrid, unsigned opt) {
	BOOST_LOG_SEV(g_logger::get(), trace) << "[EVENT] abnormal close room: " << rrid;
	if (event_room_) {
		event_room_(MSG_CLOSEROOM, rrid, opt);
	}
}

void event_base::post_close_msg(unsigned rrid, unsigned opt) {
	BOOST_LOG_SEV(g_logger::get(), trace) << "[EVENT] msg close room: " << rrid;
	if (event_room_) {
		event_room_(MSG_CHANGEROOM1, rrid, opt);
	}
}

void event_base::post_open_msg(unsigned rrid, unsigned opt) {
	BOOST_LOG_SEV(g_logger::get(), trace) << "[EVENT] msg open room: " << rrid;
	if (event_room_) {
		event_room_(MSG_CHANGEROOM2, rrid, opt);
	}
}
