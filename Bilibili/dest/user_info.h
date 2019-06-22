﻿/*
1 为每一个用户创建一个CURL句柄，并在登录成功或导入后将Cookie信息
  更新到CURL句柄，在之后的HTTP请求中无需再次导入Cookie。
2 导出账号信息时自动从CURL句柄获取最新的Cookie信息。

关于配置信息的说明:
conf_coin    硬币兑换
conf_lottery 各种抽奖
conf_storm   风暴
conf_guard   亲密度
conf_pk      大乱斗抽奖
*/
#pragma once
#include <memory>
#include <set>
#include <string>
#include "rapidjson/document.h"
#include "utility/httpex.h"

const unsigned TEN_DEFAULT_LEVEL = 9;

class user_info {
public:
	bool islogin;
	// 配置文件中的序号
	int fileid;
	unsigned uid;
	// 配置信息
	unsigned conf_coin;
	unsigned conf_lottery;
	unsigned conf_storm;
	unsigned conf_guard;
	unsigned conf_pk;
	std::string account;
	std::string password;
	std::string tokena, tokenr;

	std::string tokenjct;
	std::string visitid;

	// 心跳倒计时
	int heart_deadline;
	// 瓜子倒计时
	int silver_deadline;
	int silver_minute;
	int silver_amount;
	long long int silver_start;
	long long int silver_end;

// 10周年
public:
	// CP ID
	long long ten_cp_id = 0;
	// CP Token
	std::string ten_cp_token;
	// 签到状态
	bool ten_sign_status = false;
	// 菜单状态 1表示已完成
	int ten_egg_status = 0;
	std::string ten_egg_taskid;
	unsigned ten_egg_assocId = 0;
	unsigned ten_egg_type = 0;
	// 分享状态 1表示已完成 2未分享
	int ten_pub_status = 0;
	std::string ten_pub_taskid;
	unsigned ten_pub_assocId = 0;
	unsigned ten_pub_type = 0;
	// 点赞任务列表
	std::set<unsigned> ten_task_list;
	// 3次点赞已用完
	bool ten_task_full = false;
	// 见证者队伍ID
	long long ten_team_id = 0;
	// 见证者剩余坑位
	unsigned ten_team_empty_num = 0;
	// 见证者列表
	std::vector<unsigned> ten_team_list;
	// 是否加入见证团
	bool ten_team_hasjoin = false;
	// 该账号的下线等级 1为最高级 9为未成为见证者
	unsigned ten_self_level = TEN_DEFAULT_LEVEL;

public:
	// 两个连接 一个用于网页端 一个用于手机端
	CURL *curlweb, *curlapp;
	unique_ptr<toollib::CHTTPPack> httpweb, httpapp;
	
public:
	explicit user_info();
	~user_info();

public:
	// 从文件导入指定账户 出错时向外抛出异常 
	void ReadFileAccount(const std::string &key, const rapidjson::Value& data, int index);
	// 将账户信息导出到文件
	void WriteFileAccount(const std::string key, rapidjson::Document& doc);
	// 检测账户是否被封禁
	bool CheckBanned(const std::string &msg);
	// 账户被封禁
	void SetBanned();
	// 生成随机访问ID
	void GetVisitID();
	// 获取 Cookie bili_jct 的值
	bool GetToken();
	// 获取 Cookie bili_jct 的失效时间
	int GetExpiredTime();
};