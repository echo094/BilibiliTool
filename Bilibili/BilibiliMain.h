
/*
https://github.com/lzghzr/bilive_client
http://www.lyyyuna.com/2016/03/14/bilibili-danmu01/
*/

#pragma once
#include "BilibiliUserList.h"
#include "BilibiliDanmu.h"
#include "BilibiliDanmuWS.h"
#include "BilibiliYunYing.h"

class CBilibiliMain
{
private:
	CTools _tool;//实用函数类
	std::fstream _logfile;//日志文件句柄
	char _logbuff[100];//日志缓冲区

	TOOL_EVENT curmode;//当前模式
	CBilibiliUserList *_userlist;//账户列表类

	int _roomcount;//当前连接的房间数
	CBilibiliSmallTV *_lotterytv;//小电视信息处理类
	CBilibiliYunYing *_lotteryyy;//季节性活动信息处理类
	CBilibiliLive *_apilive; // 其它API
	CBilibiliDanmu *_tcpdanmu;
	CWSDanmu *_wsdanmu;

public:
	CBilibiliMain(CURL *pcurl = NULL);
	~CBilibiliMain();

//HTTP数据收发
protected:
	CURL *curl;
public:
	int SetCURLHandle(CURL *pcurl);

public:
	CBilibiliUserList* GetUserList(int index = 0);
	int SaveLogFile();

	int StopMonitorALL();
	int StartUserHeart();
	int StartMonitorPubEvent(int pthreadid);
	void SetDanmukuShow();
	void SetDanmukuHide();
	int JoinTV(int room);
	int JoinYunYing(int room);
	int JoinYunYingGift(int room);
	int JoinGuardGift(const char *user);
	int JoinSpecialGift(int room, long long cid, std::string str);
	int Debugfun(int index);
};
