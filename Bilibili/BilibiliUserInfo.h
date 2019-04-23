/*
1 为每一个用户创建一个CURL句柄，并在登录成功或导入后将Cookie信息
  更新到CURL句柄，在之后的HTTP请求中无需再次导入Cookie。
2 导出账号信息时自动从CURL句柄获取最新的Cookie信息。

关于配置信息的说明:
0x01 硬币兑换
0x02 银瓜子
0x04 Web端运营抽奖
0x08 客户端运营抽奖
0x10 Web端风暴抽奖
0x20 客户端风暴抽奖
0x40 Web端小电视抽奖
*/
#pragma once
#include <string>

enum class LOGINRET {
	NOFAULT,
	NOTLOGIN,
	NOTVALID
};

struct tagHeartInfo
{
	int timercount;// 心跳计时
	int silvercount;// 瓜子计时
};

// 用户参数
typedef struct _BILIUSEROPT
{
	// 配置文件中的序号
	int fileid;
	int conf; // 配置信息
	bool islogin;
	unsigned uid;
	std::string account;
	std::string password;
	std::string tokena,tokenr;
	std::string tokenjct;
	std::string visitid;

	int silver_minute;
	int silver_count;
	long long int silver_start;
	long long int silver_end;
}BILIUSEROPT;

class CBilibiliUserInfo
{
private:
	// 网页链接
	std::string _urlapi;
	// 3DES加密key
	std::string _key_3des;
	// 用户信息
	BILIUSEROPT _useropt;
	// 心跳包配置
	tagHeartInfo _heartopt;

// 工具成员
private:
	CStrConvert _strcoding;
	
public:
	explicit CBilibiliUserInfo();
	~CBilibiliUserInfo();
	bool getLoginStatus()const { return _useropt.islogin; };
	void setLoginStatus(bool status) { _useropt.islogin = status; };
	int GetFileSN()const { return _useropt.fileid; };
	void SetFileSN(int sn) { _useropt.fileid = sn; };
	std::string GetUsername()const { return _useropt.account; };

// HTTP数据收发
protected:
	// 两个连接 一个用于网页端 一个用于手机端
	CURL *curlweb, *curlapp;
	unique_ptr<CHTTPPack> _httppackweb, _httppackapp;
	CCookiePack _httpcookie;

public:
	// 新用户登录
	LOGINRET Login(int index, std::string username, std::string password);
	// 重新登录
	LOGINRET Relogin();
	// 导入用户验证
	LOGINRET CheckLogin();
	// 获取用户信息
	int FreshUserInfo();
	// 从文件导入指定账户
	int ReadFileAccount(std::string key, int index, char *addr);
	// 将账户信息导出到文件
	int WriteFileAccount(std::string key, char *addr);
public:
	// 开启经验心跳
	int ActStartHeart();
	// 经验心跳
	int ActHeart();
	// 节奏风暴
	int ActStorm(int roid, long long cid);
	// 通告礼物
	int ActLottery(int rrid, int loid);
	// 上船低保
	int ActGuard(const std::string &type, const int rrid, const int loid);

// 其它直播API
protected:
	// 获取直播站主要信息
	BILIRET GetUserInfoLive(BILIUSEROPT &pinfo) const;
	// 获取移动端登录验证码
	BILIRET GETLoginCaptcha() const;
	// 直播经验心跳Web
	BILIRET PostOnlineHeart() const;
	// 直播站签到
	BILIRET GetSign() const;
	// 获取登录硬币
	BILIRET GetCoin() const;

// APIv1
protected:
	// 直播经验心跳日志1
	BILIRET _APIv1HeartBeat() const;
	// 获取验证码图片
	BILIRET _APIv1Captcha(std::string &img, std::string &token) const;
	// 领取风暴
	BILIRET _APIv1StormJoin(int roomID, long long cid, std::string code, std::string token);
	// 银瓜子换硬币新API
	BILIRET _APIv1Silver2Coin() const;
	// 查询扭蛋币数量
	BILIRET _APIv1CapsuleCheck() const;
	// 进入房间历史记录
	BILIRET _APIv1RoomEntry(int room) const;

// APIv2
protected:
	// 查询背包道具
	BILIRET _APIv2GiftBag() const;
	// 领取每日礼物
	BILIRET _APIv2GiftDaily() const;
	// 上船低保领取
	BILIRET _APIv2LotteryJoin(const std::string &type, const int rrid, const int loid);

// APIv3
protected:
	// 通告礼物抽奖
	BILIRET _APIv3SmallTV(int rrid, int loid);

// 安卓端API 
private:
	BILIRET _APIAndv2GetKey(std::string &psd) const;
	BILIRET _APIAndv2Login(std::string username, std::string password, std::string captcha);
	// 获取当前宝箱领取情况
	BILIRET _APIAndSilverCurrentTask();
	// 领取银瓜子
	BILIRET _APIAndSilverAward();
	// 领取风暴
	BILIRET _APIAndv1StormJoin(long long cid);

// 其他
protected:
	int _GetMD5Sign(const char *in, std::string &sign) const;
	int GetExpiredTime();
	int GetToken(BILIUSEROPT &opt);
	// 账户权限验证
	int AccountVerify();
	// 账户被封禁
	int SetBanned();

	BILIRET _GetCaptchaKey();
	// 生成随机访问ID
	int _GetVisitID(std::string &sid) const;
	// 大写转小写
	int _ToLower(std::string &str) const;
};