
/*
https://github.com/lzghzr/bilive_client
http://www.lyyyuna.com/2016/03/14/bilibili-danmu01/
*/

#pragma once
#include <fstream>
#include <functional>
#include <memory>
#include <boost/asio.hpp>
#include "event/event_base.h"
#include "source/source_base.h"
#include "dest/dest_user.h"
#include "BilibiliYunYing.h"

using std::unique_ptr;
using std::shared_ptr;

//功能模块编号
enum class TOOL_EVENT {
	EXIT = 0,
	STOP = 10,
	ONLINE,
	GET_SYSMSG_GIFT,
	GET_HIDEN_GIFT,
	DEBUG1,
	DEBUG2,
};

class CBilibiliMain {
public:
	explicit CBilibiliMain();
	~CBilibiliMain();

public:
	// 帮助菜单
	void PrintHelp();
	// 初始化
	int Run();
	// 房间消息回调
	void post_msg_room(unsigned msg, unsigned rrid, unsigned opt);
	// 活动消息回调
	void post_msg_act(std::shared_ptr<BILI_LOTTERYDATA> data);

private:
	// Start heart timer
	void set_timer_refresh(unsigned sec);
	// Heart timer
	void on_timer_refresh(boost::system::error_code ec);
	// 用户心跳定时器
	void set_timer_userheart(unsigned sec, unsigned type);
	// 用户心跳操作
	void on_timer_userheart(boost::system::error_code ec, unsigned type);

private:
	// 处理房间消息
	int ProcessMSGRoom(unsigned msg, unsigned rrid, unsigned opt);
	// 处理活动消息
	int ProcessMSGAct(unsigned msg, std::shared_ptr<BILI_LOTTERYDATA> data);
	// 处理用户指令
	int ProcessCommand(std::string str);

	int StopMonitorALL();
	int StartUserHeart();
	int StartMonitorPubEvent();
	int StartMonitorHiddenEvent();
	// 调试函数
	int Debugfun(int index);

	// 广播模式下更新监控的房间
	int UpdateAreaRoom(const unsigned rid, const unsigned area, const bool opt);
	// 非广播模式下更新监控的房间
	int UpdateLiveRoom();
	// 抽奖消息
	int CheckLotGift(std::shared_ptr<BILI_LOTTERYDATA> data);
	// 上船消息通告只含有房间号
	int CheckLotGuard(std::shared_ptr<BILI_LOTTERYDATA> data);
	// 上船抽奖事件通告含有完整抽奖信息
	int JoinLotGuard(std::shared_ptr<BILI_LOTTERYDATA> data);
	// 节奏风暴
	int JoinLotStorm(std::shared_ptr<BILI_LOTTERYDATA> data);
	// 大乱斗抽奖
	int JoinLotPK (std::shared_ptr<BILI_LOTTERYDATA> data);
	// 弹幕抽奖
	int JoinLotDanmu(std::shared_ptr<BILI_LOTTERYDATA> data);
	// 天选时刻抽奖
	int JoinLotAnchor(std::shared_ptr<BILI_LOTTERYDATA> data);
	// 用户心跳
	int HeartExp(unsigned type);

	// 是否跳过此次操作
	bool isSkip();
	// 创建新文件
	int SaveLogFile();

private:
	// IO serverce
	boost::asio::io_context io_context_;
	// Heart Timer
	boost::asio::deadline_timer heart_timer_;
	// Maintenance worker
	std::shared_ptr<boost::asio::io_context::work> pwork_;
	// Threads
	std::shared_ptr<std::thread> thread_main_, thread_heart_;
	// 辅助运行标志
	bool heart_flag_;
	// 当前模式
	TOOL_EVENT curmode;
	// HTTP数据收发
	CURL *curl_main_, *curl_heart_;
	// 日志文件句柄
	std::fstream _logfile;
	// 小电视信息处理类
	unique_ptr<lottery_list> _lotterytv;
	// 上船信息处理类
	unique_ptr<guard_list> _lotterygu;
	// 其它API
	unique_ptr<CBilibiliLive> _apilive;
	// 弹幕API解释模块
	shared_ptr<event_base> _apidm;
	// 弹幕连接
	unique_ptr<source_base> _dmsource;
	// 账户列表类
	unique_ptr<dest_user> _userlist;
};
