
/*
https://github.com/lzghzr/bilive_client
http://www.lyyyuna.com/2016/03/14/bilibili-danmu01/
*/

#pragma once
#include <functional>
#include <memory>
#include <boost/asio.hpp>
#include "BilibiliUserList.h"
#include "event/BilibiliDanmuAPI.h"
#include "source/BilibiliDanmuWS.h"
#include "source/source_dmasio.h"
#include "BilibiliYunYing.h"

#include <memory>
using std::unique_ptr;
using std::shared_ptr;

class CBilibiliMain {
public:
	explicit CBilibiliMain();
	~CBilibiliMain();

public:
	// 帮助菜单
	void PrintHelp();
	// 初始化
	int Run();
	// 消息回调
	void post_msg(unsigned msg, WPARAM wp, LPARAM lp);

private:
	// Stert heart timer
	void start_timer(unsigned sec);
	// Heart timer
	void on_timer(boost::system::error_code ec);

private:
	// 处理模块事件
	int ProcessModuleMSG(unsigned msg, WPARAM wp, LPARAM lp);
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
	int JoinLottery(int room);
	// 上船消息通告只含有房间号
	int JoinGuardGift(int room);
	// 上船抽奖事件通告含有完整抽奖信息
	int JoinGuardGift(BILI_LOTTERYDATA &pdata);
	// 节奏风暴
	int JoinSpecialGift(int room, long long cid);

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
	// 账户列表类
	unique_ptr<CBilibiliUserList> _userlist;
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
};
