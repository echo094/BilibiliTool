#pragma once
#include "BilibiliUserInfo.h"
#include <atomic>
#include <queue> 
#include <list>
#include <boost/thread/thread.hpp>

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

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

// 传递给线程的数据结构体=
typedef struct _THARED_DATAEX
{
	std::string str;
	int rrid;
	long long loid;
}THARED_DATAEX, *PTHARED_DATAEX;

class CBilibiliUserList
{
private:
	unsigned long _parentthread;//上级消息线程

private:
	char _cfgfile[MAX_PATH];
	std::list<CBilibiliUserInfo*> _userlist;
	int _usercount;
	int _heartcount;
	std::string pubkey, prikey;//ini文件中密码的加密解密key

private:
	// 正在运行的线程计数;
	std::atomic<int> _threadcount;
	// 线程互斥量 过渡方案
	boost::shared_mutex rwmutex_;

public:
	explicit CBilibiliUserList();
	~CBilibiliUserList();

public:
	int AddUser(std::string username, std::string password);
	int DeleteUser(std::string username);
	int ClearUserList();
	int ShowUserList();
	int ImportUserList();
	int ExportUserList();
	int ReloginAll();
	int CheckUserStatusALL();
	int GetUserInfoALL();
public:
	//设置父级线程ID
	int SetNotifyThread(unsigned long id);
	// 检测并等待抽奖线程停止
	int WaitActThreadStop();
	int JoinLotteryALL(std::shared_ptr<BILI_LOTTERYDATA> data);
	int JoinGuardALL(std::shared_ptr<BILI_LOTTERYDATA> data);
	int JoinSpecialGiftALL(std::shared_ptr<BILI_LOTTERYDATA> data);
	// 经验心跳
	int HeartExp(int firsttime = 0);

protected:
	// 查找用户
	CBilibiliUserInfo* SearchUser(std::string username);
	// 小电视领取线程
	void Thread_ActLottery(PTHARED_DATAEX pdata);
	// 舰队低保领取线程
	void Thread_ActGuard(PTHARED_DATAEX pdata);
	// 节奏领取线程
	void Thread_ActStorm(PTHARED_DATAEX pdata);
	// 取随机数
	int _GetRand(int start, int len);
};
