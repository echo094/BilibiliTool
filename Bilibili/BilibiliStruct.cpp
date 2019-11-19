#include "BilibiliStruct.h"
#include "logger/log.h"

unsigned BILI_LOTTERYDATA::count = 0;
unsigned BILI_LOTTERYDATA::remain = 0;

BILI_LOTTERYDATA::BILI_LOTTERYDATA() {
	++count;
	++remain;
	BOOST_LOG_SEV(g_logger::get(), trace) << "[ITEM] new count: " << count << " remain: " << remain;
}

BILI_LOTTERYDATA::BILI_LOTTERYDATA(const BILI_LOTTERYDATA & item)
{
	++remain;
	BOOST_LOG_SEV(g_logger::get(), trace) << "[ITEM] copy count: " << count << " remain: " << remain;
	*this = item;
}

BILI_LOTTERYDATA::~BILI_LOTTERYDATA() {
	--remain;
	BOOST_LOG_SEV(g_logger::get(), trace) << "[ITEM] del count: " << count << " remain: " << remain;
}
