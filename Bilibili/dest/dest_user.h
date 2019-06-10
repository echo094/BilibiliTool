#pragma once
#include <atomic>
#include <memory>
#include <vector>
#include <boost/thread/thread.hpp>
#include "BilibiliUserInfo.h"

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

class dest_user
{
public:
	explicit dest_user();
	~dest_user();

public:
	// CMD  1 导入用户
	int ImportUserList();
	// CMD  2 导出用户
	int ExportUserList();
	// CMD  3 显示用户列表
	int ShowUserList();
	// CMD  4 添加用户
	int AddUser(std::string username, std::string password);
	// CMD  5 删除用户
	int DeleteUser(std::string username);
	// CMD  6 刷新用户Cookie
	int ReloginAll();
	// CMD  7 检查用户账号状态
	int CheckUserStatusALL();
	// CMD  8 查询用户持有道具
	int GetUserInfoALL();

private:
	// 清除所有用户
	void _ClearUserList();
	// 查找用户
	bool _IsExistUser(std::string username);

public:
	//设置父级线程ID
	int SetNotifyThread(unsigned long id);
	// 检测并等待抽奖线程停止
	int WaitActThreadStop();
	// 经验心跳
	int HeartExp(int firsttime = 0);
	// 参与抽奖
	int JoinLotteryALL(std::shared_ptr<BILI_LOTTERYDATA> data);
	// 参与亲密度
	int JoinGuardALL(std::shared_ptr<BILI_LOTTERYDATA> data);
	// 参与风暴
	int JoinSpecialGiftALL(std::shared_ptr<BILI_LOTTERYDATA> data);

private:
	// 抽奖线程
	void Thread_ActLottery(PTHARED_DATAEX pdata);
	// 舰队低保领取线程
	void Thread_ActGuard(PTHARED_DATAEX pdata);
	// 节奏领取线程
	void Thread_ActStorm(PTHARED_DATAEX pdata);
	// 取随机数
	int _GetRand(int start, int len);

private:
	unsigned long _parentthread;//上级消息线程
	// 正在运行的线程计数;
	std::atomic<int> _threadcount;
	// 线程互斥量 过渡方案
	boost::shared_mutex rwmutex_;

private:
	char _cfgfile[MAX_PATH];
	unsigned _heartcount;
	unsigned _usercount;
	std::vector<std::shared_ptr<CBilibiliUserInfo> > _user_list;
	std::string pubkey, prikey;//ini文件中密码的加密解密key
};
