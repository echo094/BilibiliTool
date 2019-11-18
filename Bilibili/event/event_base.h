#pragma once
#include <functional>
#include <map>
#include "BilibiliStruct.h"

class event_base {
private:
	typedef std::function<void(std::shared_ptr<BILI_LOTTERYDATA>)> event_act;
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
	/**
	 * @brief 传递广播抽奖消息
	 *
	 * 包括 礼物|总督
	 *
	 * @param type  消息类型
	 * @param rrid  需查询的直播间号
	 *
	 */
	void post_lottery_pub(unsigned type, unsigned rrid);
	/**
	 * @brief 传递房间内抽奖消息
	 *
	 * 包括 节奏风暴|舰长提督|PK
	 *
	 * @param data  抽奖信息
	 *
	 */
	void post_lottery_hidden(std::shared_ptr<BILI_LOTTERYDATA> data);
	void post_close_event(unsigned rrid, unsigned opt);
	void post_close_msg(unsigned rrid, unsigned opt);
	void post_open_msg(unsigned rrid, unsigned opt);

private:
	event_act event_act_;
	event_room event_room_;
};
