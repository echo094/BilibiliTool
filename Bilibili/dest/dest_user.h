#pragma once
#include <atomic>
#include <memory>
#include <vector>
#include <boost/thread/thread.hpp>
#include "user_info.h"
#include "utility/platform.h"
#include "BilibiliStruct.h"

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
	// 经验心跳
	int HeartExp(int firsttime = 0);
	// 参与抽奖
	int JoinLottery(std::shared_ptr<BILI_LOTTERYDATA> data);

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
	// 取随机数
	int _GetRand(int start, int len);

private:
	char _cfgfile[MAX_PATH];
	unsigned _heartcount;
	unsigned _usercount;
	std::vector<std::shared_ptr<user_info> > _user_list;
	std::string pubkey, prikey;//ini文件中密码的加密解密key
};
