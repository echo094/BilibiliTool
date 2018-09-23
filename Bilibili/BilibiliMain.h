
/*
https://github.com/lzghzr/bilive_client
http://www.lyyyuna.com/2016/03/14/bilibili-danmu01/
*/

#pragma once
#include "BilibiliUserList.h"
#include "BilibiliDanmu.h"
#include "BilibiliDanmuWS.h"
#include "BilibiliYunYing.h"

class CBilibiliMain {
public:
	explicit CBilibiliMain(CURL *pcurl = NULL);
	~CBilibiliMain();

public:
	int SetCURLHandle(CURL *pcurl);
	unique_ptr<CBilibiliUserList> &GetUserList(int index = 0);
	int SaveLogFile();

	int StopMonitorALL();
	int StartUserHeart();
	int StartMonitorPubEvent(int pthreadid);
	int StartMonitorHiddenEvent(int pthreadid);
	void SetDanmukuShow();
	void SetDanmukuHide();
	// 非广播模式下更新监控的房间
	int UpdateLiveRoom();
	int JoinTV(int room);
	int JoinYunYing(int room);
	int JoinYunYingGift(int room);
	int JoinGuardGift(int user);
	int JoinGuardGift(BILI_LOTTERYDATA &pdata);
	int JoinSpecialGift(int room, long long cid);
	int Debugfun(int index);

private:
	// HTTP数据收发
	CURL *curl;
	// 实用函数类
	CTools _tool;
	// 日志文件句柄
	std::fstream _logfile;

	// 当前模式
	TOOL_EVENT curmode;
	// 当前连接的房间数
	int _roomcount;
	// 账户列表类
	unique_ptr<CBilibiliUserList> _userlist;
	// 小电视信息处理类
	unique_ptr<CBilibiliSmallTV> _lotterytv;
	// 季节性活动信息处理类
	unique_ptr<CBilibiliYunYing> _lotteryyy;
	// 上船信息处理类
	unique_ptr<CBilibiliGuard> _lotterygu;
	// 其它API
	unique_ptr<CBilibiliLive> _apilive;
	// TCP弹幕连接
	unique_ptr<CBilibiliDanmu> _tcpdanmu;
	// WS弹幕连接
	unique_ptr<CWSDanmu> _wsdanmu;
};
