
/*
https://github.com/lzghzr/bilive_client
http://www.lyyyuna.com/2016/03/14/bilibili-danmu01/
*/

#pragma once
#include "BilibiliUserList.h"
#include "BilibiliDanmu.h"
#include "BilibiliDanmuWS.h"
#include "BilibiliYunYing.h"

const unsigned ON_USER_COMMAND = WM_USER + 610;

class CBilibiliMain {
public:
	explicit CBilibiliMain();
	~CBilibiliMain();

public:
	// 帮助菜单
	void PrintHelp();
	// 初始化
	int Run();
	// 消息线程
	static DWORD WINAPI ThreadEntry(PVOID lpParameter);
	// 消息处理
	void ThreadHandler();

private:
	// 处理用户指令
	int ProcessUserMSG(TOOL_EVENT &msg);
	// 处理模块事件
	int ProcessModuleMSG(MSG &msg);
	// 处理用户指令
	int ProcessCommand(std::string str);

	int StopMonitorALL();
	int StartUserHeart();
	int StartMonitorPubEvent(int pthreadid);
	int StartMonitorHiddenEvent(int pthreadid);
	// 调试函数
	int Debugfun(int index);

	// 广播模式下更新监控的房间
	int UpdateAreaRoom(const unsigned rid, const unsigned area);
	// 非广播模式下更新监控的房间
	int UpdateLiveRoom();
	// 抽奖消息
	int JoinTV(int room);
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
	// 日志文件句柄
	std::fstream _logfile;
	// 当前模式
	TOOL_EVENT curmode;
	// HTTP数据收发
	CURL *m_curl;
	// 主线程运行状态
	bool m_threadstat;
	//主线程序号
	DWORD m_threadid;
	//主线程句柄
	HANDLE m_threadhandle;
	// 主线程定时器
	DWORD m_timer;
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
