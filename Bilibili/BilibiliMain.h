
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
	// 是否跳过此次操作
	bool isSkip();

	int StopMonitorALL();
	int StartUserHeart();
	int StartMonitorPubEvent(int pthreadid);
	int StartMonitorHiddenEvent(int pthreadid);
	void SetDanmukuShow();
	void SetDanmukuHide();
	// 广播模式下更新监控的房间
	int UpdateAreaRoom(const unsigned rid, const unsigned area);
	// 非广播模式下更新监控的房间
	int UpdateLiveRoom();
	int JoinTV(int room);
	// 上船消息通告只含有房间号
	int JoinGuardGift(int room);
	// 上船抽奖事件通告含有完整抽奖信息
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
	// 上船信息处理类
	unique_ptr<CBilibiliGuard> _lotterygu;
	// 其它API
	unique_ptr<CBilibiliLive> _apilive;
	// TCP弹幕连接
	unique_ptr<CBilibiliDanmu> _tcpdanmu;
	// WS弹幕连接
	unique_ptr<CWSDanmu> _wsdanmu;
};
