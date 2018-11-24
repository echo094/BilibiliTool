#include "stdafx.h"
#include "event/event_base.h"
#include "log.h"

void event_base::set_event_handler(event_handler h) {
	event_handler_ = h;
}

void event_base::post_lottery_msg(unsigned rrid) {
	BOOST_LOG_SEV(g_logger::get(), info) << "[EVENT] lottery room: " << rrid;
	if (event_handler_) {
		event_handler_(MSG_NEWSMALLTV, WPARAM(rrid), 0);
	}
}

void event_base::post_storm_msg(BILI_ROOMEVENT * pinfo) {
	BOOST_LOG_SEV(g_logger::get(), info) << "[EVENT] storm room: " << pinfo->rid
		<< " id: " << pinfo->loidl;
	if (event_handler_) {
		event_handler_(MSG_NEWSPECIALGIFT, WPARAM(pinfo), 0);
	}
}

void event_base::post_guard1_msg(unsigned rrid) {
	BOOST_LOG_SEV(g_logger::get(), info) << "[EVENT] guard room: " << rrid;
	if (event_handler_) {
		event_handler_(MSG_NEWGUARD1, WPARAM(rrid), 0);
	}
}

void event_base::post_guard23_msg(BILI_LOTTERYDATA * pinfo) {
	BOOST_LOG_SEV(g_logger::get(), info) << "[EVENT] guard room: " << pinfo->rrid
		<< " id: " << pinfo->loid << " type: " << pinfo->exinfo;
	if (event_handler_) {
		event_handler_(MSG_NEWGUARD0, WPARAM(pinfo), 0);
	}
}

void event_base::post_close_msg(unsigned rrid, unsigned area) {
	BOOST_LOG_SEV(g_logger::get(), trace) << "[EVENT] close room: " << rrid;
	if (event_handler_) {
		event_handler_(MSG_CHANGEROOM1, WPARAM(rrid), LPARAM(area));
	}
}

void event_base::post_open_msg(unsigned rrid, unsigned area) {
	BOOST_LOG_SEV(g_logger::get(), trace) << "[EVENT] open room: " << rrid;
	if (event_handler_) {
		event_handler_(MSG_CHANGEROOM2, WPARAM(rrid), LPARAM(area));
	}
}
