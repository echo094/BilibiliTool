#pragma once
#include "BilibiliUserInfo.h"
#include <queue> 
#include <list>

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

// 传递给线程的数据结构体
class CBilibiliUserList;
typedef struct _THARED_DATAEX
{
	CBilibiliUserList *ptr;
	std::string str;
	int id1, id2;
	long long id3;
}THARED_DATAEX, *PTHARED_DATAEX;

class CBilibiliUserList
{
private:
	DWORD _parentthread;//上级消息线程
	bool _isworking[2]; //线程运行循环标志
	DWORD _msgthread;//当前主消息循环
	HANDLE _lphandle[2];//线程句柄列表
	TOOL_EVENT m_workmode;//工作模式

private:
	char _cfgfile[MAX_PATH];
	std::list<CBilibiliUserInfo*> _userlist;
	int _usercount;
	int _heartcount;
	std::string pubkey, prikey;//ini文件中密码的加密解密key

private:
	// 目前同一时间只能进行一次请求 不管是不是同一用户
	CRITICAL_SECTION _csthread;
	// 正在运行的线程计数;
	int _threadcount;

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
	int SetNotifyThread(DWORD id);
	int StartUserHeart();
	int StopUserHeart();
	// 检测并等待抽奖线程停止
	int WaitActThreadStop();
	int JoinTVALL(BILI_LOTTERYDATA *data);
	int JoinGuardALL(BILI_LOTTERYDATA &data);
	int JoinSpecialGiftALL(int roomID, long long cid);
	int SendDanmuku(int index, int roomID, std::string msg);

protected:
	// 查找用户
	CBilibiliUserInfo* SearchUser(std::string username);
	// 经验心跳
	int _HeartExp(int firsttime = 0);
	// 小电视领取线程
	static DWORD WINAPI Thread_ActTV(PVOID lpParameter);
	// 舰队低保领取线程
	static DWORD WINAPI Thread_ActGuard(PVOID lpParameter);
	// 节奏领取线程
	static DWORD WINAPI Thread_ActStorm(PVOID lpParameter);
	// 消息线程
	static DWORD WINAPI Thread_UserListMSG(PVOID lpParameter);
	// 取随机数
	int _GetRand(int start, int len);
};
