#pragma once
#include <atomic>
#include <memory>
#include <vector>
#include <boost/thread/thread.hpp>
#include "user_info.h"
#include "BilibiliStruct.h"

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

enum class LOGINRET {
	NOFAULT,
	NOTLOGIN,
	NOTVALID
};

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
	int JoinLotGift(std::shared_ptr<BILI_LOTTERYDATA> data);
	// 参与亲密度
	int JoinLotGuard(std::shared_ptr<BILI_LOTTERYDATA> data);
	// 参与风暴
	int JoinLotStorm(std::shared_ptr<BILI_LOTTERYDATA> data);
	// 参与大乱斗抽奖
	int JoinLotPk(std::shared_ptr<BILI_LOTTERYDATA> data);
	// 参与弹幕抽奖
	int JoinLotDanmu(std::shared_ptr<BILI_LOTTERYDATA> data);
	// 参与天选抽奖
	int JoinLotAnchor(std::shared_ptr<BILI_LOTTERYDATA> data);

private:
	// 新用户登录
	LOGINRET _ActLogin(std::shared_ptr<user_info> &user, int index, std::string username, std::string password);
	// 重新登录
	LOGINRET _ActRelogin(std::shared_ptr<user_info> &user);
	// 导入用户验证
	LOGINRET _ActCheckLogin(std::shared_ptr<user_info> &user);
	// 获取用户信息
	int _ActGetUserInfo(const std::shared_ptr<user_info> &user);
	// 首次经验心跳
	void _ActHeartFirst(std::shared_ptr<user_info> &user);
	// 持续经验心跳
	void _ActHeartContinue(std::shared_ptr<user_info> &user);
	// 通告礼物
	int _ActLottery(std::shared_ptr<user_info> &user, std::shared_ptr<BILI_LOTTERYDATA> data);
	// 上船低保
	int _ActGuard(std::shared_ptr<user_info> &user, std::shared_ptr<BILI_LOTTERYDATA> data);
	// 节奏风暴
	int _ActStorm(std::shared_ptr<user_info> &user, std::shared_ptr<BILI_LOTTERYDATA> data);
	// 大乱斗
	int _ActPK(std::shared_ptr<user_info> &user, std::shared_ptr<BILI_LOTTERYDATA> data);
	// 弹幕
	int _ActDanmu(std::shared_ptr<user_info> &user, std::shared_ptr<BILI_LOTTERYDATA> data);
	// 天选
	int _ActAnchor(std::shared_ptr<user_info> &user, std::shared_ptr<BILI_LOTTERYDATA> data);
	// 抽奖线程
	void Thread_ActLottery(std::shared_ptr<BILI_LOTTERYDATA> data);
	// 舰队低保领取线程
	void Thread_ActGuard(std::shared_ptr<BILI_LOTTERYDATA> data);
	// 节奏领取线程
	void Thread_ActStorm(std::shared_ptr<BILI_LOTTERYDATA> data);
	// 大乱斗领取线程
	void Thread_ActPK(std::shared_ptr<BILI_LOTTERYDATA> data);
	// 弹幕领取线程
	void Thread_ActDanmu(std::shared_ptr<BILI_LOTTERYDATA> data);
	// 天选领取线程
	void Thread_ActAnchor(std::shared_ptr<BILI_LOTTERYDATA> data);
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
	std::vector<std::shared_ptr<user_info> > _user_list;
	std::string pubkey, prikey;//ini文件中密码的加密解密key
};
