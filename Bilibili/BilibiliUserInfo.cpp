#include "stdafx.h"
#include "BilibiliUserInfo.h"
#include <cmath>
#include <iostream>
#include <sstream> 
#include <fstream>  
#include "log.h"
using namespace rapidjson;

const char PARAM_BUILD[] = "5230002";

CBilibiliUserInfo::CBilibiliUserInfo() :
	_httppackweb(new CHTTPPack()),
	_httppackapp(new CHTTPPack("Mozilla/5.0 BiliDroid/5.23.2 (bbcallen@gmail.com)"))
{
	_useropt.fileid = 0;
	_useropt.islogin = false;
	_useropt.conf_coin = 0;
	_useropt.conf_lottery = 0;
	_useropt.conf_storm = 0;
	_useropt.conf_guard = 0;
	_urlapi = URL_LIVEAPI_HEAD;
	_key_3des = "08E1CAAD409B840208E1CAAD";
	curlweb = curl_easy_init();
	curlapp = curl_easy_init();
	_httppackweb->AddDefHeader("Accept-Language: zh-CN,zh;q=0.8");
	_httppackweb->AddDefHeader("Connection: keep-alive");
	_httppackweb->AddDefHeader("DNT: 1");
	_httppackapp->AddDefHeader("Buvid: A59813F7-2A50-42C5-A246-AF93A96374E320912infoc");
	_httppackapp->AddDefHeader("Device-ID: KRkhFHdFd0J0RXBFOUU5Cj8KPQg_Dz4OOQkxBDUA");
	_httppackapp->AddDefHeader("Display-ID: 759639-1523371597");
	_httppackapp->AddDefHeader("Connection: keep-alive");
}

CBilibiliUserInfo::~CBilibiliUserInfo()
{
	curl_easy_cleanup(curlweb);
	curlweb = nullptr;
	curl_easy_cleanup(curlapp);
	curlapp = nullptr;
	_httppackweb = nullptr;
	_httppackapp = nullptr;
	BOOST_LOG_SEV(g_logger::get(), debug) << "[User] Stop.";
}

// 新用户登录
LOGINRET CBilibiliUserInfo::Login(int index, std::string username, std::string password) {
	BILIRET bret;
	int ret;
	_useropt.account = username;
	_useropt.password = password;

	// 将账号转码
	username = CStrConvert::UrlEncode(_useropt.account);

	// 移动端登录
	password = _useropt.password;
	bret = _APIAndv2GetKey(password);
	if (bret != BILIRET::NOFAULT) {
		return LOGINRET::NOTLOGIN;
	}
	bret = _APIAndv2Login(username, password, "");
	if (bret == BILIRET::LOGIN_NEEDVERIFY) {
		// 获取验证码
		bret = GETLoginCaptcha();
		if (bret != BILIRET::NOFAULT) {
			return LOGINRET::NOTLOGIN;
		}
		std::string tmp_chcode;
		printf("Enter the pic validate code: ");
		std::cin >> tmp_chcode;
		// 移动端使用验证码登录
		password = _useropt.password;
		bret = _APIAndv2GetKey(password);
		if (bret != BILIRET::NOFAULT) {
			return LOGINRET::NOTLOGIN;
		}
		bret = _APIAndv2Login(username, password, tmp_chcode);
	}
	if (bret != BILIRET::NOFAULT) {
		return LOGINRET::NOTLOGIN;
	}

	// 登录成功则获取必要的临时id
	bret = _GetCaptchaKey();
	if (bret != BILIRET::NOFAULT) {
		_useropt.islogin = false;
		return LOGINRET::NOTLOGIN;
	}
	// 生成访问ID
	_GetVisitID(_useropt.visitid);
	// 验证账户有效性
	ret = AccountVerify();
	if (ret) {
		return LOGINRET::NOTVALID;
	}
	if (GetToken(_useropt)) {
		_useropt.islogin = false;
		return LOGINRET::NOTLOGIN;
	}
	_useropt.fileid = index;
	_useropt.islogin = true;

	return LOGINRET::NOFAULT;
}

// 重新登录
LOGINRET CBilibiliUserInfo::Relogin() {
	int ret = 0;
	LOGINRET lret;
	ret = GetExpiredTime();
	long long rtime;
	rtime = ret - GetTimeStamp();
	// 有效期小于一周或Token不存在则重新登录
	if ((rtime < 604800) || (_useropt.tokena.empty())) {
		lret = Login(_useropt.fileid, _useropt.account, _useropt.password);
		return lret;
	}
	return LOGINRET::NOFAULT;
}

// 导入用户验证
LOGINRET CBilibiliUserInfo::CheckLogin() {
	BILIRET bret;
	bret = GetUserInfoLive(_useropt);
	if (bret != BILIRET::NOFAULT) {
		_useropt.islogin = false;
		return LOGINRET::NOTLOGIN;
	}
	bret = _GetCaptchaKey();
	if (bret != BILIRET::NOFAULT) {
		_useropt.islogin = false;
		return LOGINRET::NOTLOGIN;
	}
	// 生成访问ID
	_GetVisitID(_useropt.visitid);
	if (GetToken(_useropt)) {
		_useropt.islogin = false;
		return LOGINRET::NOTLOGIN;
	}
	if (AccountVerify()) {
		_useropt.islogin = false;
		return LOGINRET::NOTVALID;
	}
	_useropt.islogin = true;
	return LOGINRET::NOFAULT;
}

// 获取用户信息
int CBilibiliUserInfo::FreshUserInfo() {
	BOOST_LOG_SEV(g_logger::get(), info) << "[User" << _useropt.fileid << "] ";
	_APIv2GiftBag();
	_APIv1CapsuleCheck();
	return 0;
}

// 从文件导入指定账户
void CBilibiliUserInfo::ReadFileAccount(const std::string &key, const rapidjson::Value& data, int index) {
	std::string enstr;
	// 编号
	_useropt.fileid = index;
	// UID
	if (!data.HasMember("UserID") || !data["UserID"].IsUint()) {
		throw "Key UserID error";
	}
	_useropt.uid = data["UserID"].GetUint();
	// 获取配置信息 各种活动的参与参数
	if (!data.HasMember("Conf") || !data["Conf"].IsObject()) {
		throw "Key Conf error";
	}
	_useropt.conf_coin = data["Conf"]["CoinExchange"].GetUint();
	_useropt.conf_lottery = data["Conf"]["Lottery"].GetUint();
	_useropt.conf_storm = data["Conf"]["Storm"].GetUint();
	_useropt.conf_guard = data["Conf"]["Guard"].GetUint();
	// 账号
	if (!data.HasMember("Username") || !data["Username"].IsString()) {
		throw "Key Username error";
	}
	_useropt.account = data["Username"].GetString();
	// 密码
	if (!data.HasMember("Password") || !data["Password"].IsString()) {
		throw "Key Password error";
	}
	std::string buff = data["Password"].GetString();
	bool ret = Decrypt_RSA_KeyBuff((char*)key.c_str(), buff, _useropt.password);
	if (!ret) {
		throw "Key Password data error";
	}
	// Cookie
	if (!data.HasMember("Cookie") || !data["Cookie"].IsString()) {
		throw "Key Cookie error";
	}
	buff = data["Cookie"].GetString();
	std::string cookie;
	ret = Decode_Base64(buff, cookie);
	if (!ret) {
		throw "Key Password data error";
	}
	_httpcookie.ImportCookies(cookie, curlweb);
	// 移动端AccessToken 需要加密
	if (!data.HasMember("AccessToken") || !data["AccessToken"].IsString()) {
		throw "Key AccessToken error";
	}
	_useropt.tokena = data["AccessToken"].GetString();
	// 移动端RefreshToken 需要加密
	if (!data.HasMember("RefreshToken") || !data["RefreshToken"].IsString()) {
		throw "Key RefreshToken error";
	}
	_useropt.tokenr = data["RefreshToken"].GetString();
}

// 将账户信息导出到文件
void CBilibiliUserInfo::WriteFileAccount(const std::string key, rapidjson::Document& doc) {
	using namespace rapidjson;
	Document::AllocatorType& allocator = doc.GetAllocator();
	Value data(kObjectType);

	// UID
	data.AddMember("UserID", _useropt.uid, allocator);
	// 配置信息
	Value conf(kObjectType);
	conf.AddMember("CoinExchange", _useropt.conf_coin, allocator);
	conf.AddMember("Lottery", _useropt.conf_lottery, allocator);
	conf.AddMember("Storm", _useropt.conf_storm, allocator);
	conf.AddMember("Guard", _useropt.conf_guard, allocator);
	data.AddMember("Conf", conf.Move(), allocator);
	// 账号
	data.AddMember(
		"Username",
		Value(_useropt.account.c_str(), doc.GetAllocator()).Move(),
		allocator
	);
	// 密码
	std::string buff;
	bool ret = Encrypt_RSA_KeyBuff((char*)key.c_str(), _useropt.password, buff);
	if (!ret) {
		throw "Key Password encrypt error";
	}
	data.AddMember(
		"Password",
		Value(buff.c_str(), doc.GetAllocator()).Move(),
		allocator
	);
	// Cookie
	std::string cookie;
	_httpcookie.ExportCookies(cookie, curlweb);
	ret = Encode_Base64((const unsigned char *)cookie.c_str(), cookie.size(), buff);
	data.AddMember(
		"Cookie",
		Value(buff.c_str(), doc.GetAllocator()).Move(),
		allocator
	);
	// 移动端AccessToken
	data.AddMember(
		"AccessToken",
		Value(_useropt.tokena.c_str(), doc.GetAllocator()).Move(),
		allocator
	);
	// 移动端RefreshToken
	data.AddMember(
		"RefreshToken",
		Value(_useropt.tokenr.c_str(), doc.GetAllocator()).Move(),
		allocator
	);
	doc.PushBack(data.Move(), allocator);
}

// 开启经验心跳
int CBilibiliUserInfo::ActStartHeart() {
	// 常规心跳
	_APIv1HeartBeat();
	// 心跳计时标签
	_heartopt.timercount = 0;
	// 双端观看奖励
	_GetTaskInfo();
	// 银瓜子领取信息获取
	_APIAndSilverCurrentTask();
	// 签到
	GetSign();
	// 每日礼物
	_APIv2GiftDaily();
	// 兑换硬币
	if (_useropt.conf_coin == 1) {
		_APIv1Silver2Coin();
	}
	// 登录硬币
	GetCoin();

	return 0;
}

// 经验心跳
int CBilibiliUserInfo::ActHeart() {
	_heartopt.timercount++;
	if (_heartopt.timercount == 5) {
		_heartopt.timercount = 0;
		_APIv1HeartBeat();
		_APIAndHeart();
		PostOnlineHeart();
	}
	if (_heartopt.silvercount != -1) {
		_heartopt.silvercount--;
		if (_heartopt.silvercount == 0) {
			_APIAndSilverAward();
			_APIAndSilverCurrentTask();
		}
	}
	return 0;
}

int CBilibiliUserInfo::ActStorm(int rrid, long long loid) {
	// 风暴只领取一次 不管成功与否
	if (_useropt.conf_storm == 1) {
		// 产生访问记录
		_APIv1RoomEntry(rrid);
		// 网页端API
		_APIv1StormJoin(rrid, loid, "", "");
		return 0;
	}
	if (_useropt.conf_storm == 2) {
		// 调用客户端API领取
		_APIAndv1StormJoin(loid);
		return 0;
	}
	return 0;
}

int CBilibiliUserInfo::ActLottery(int rrid, long long loid)
{
	BILIRET bret;
	int count;
	if (_useropt.conf_lottery == 1) {
		// 产生访问记录
		_APIv1RoomEntry(rrid);
		// 网页端最多尝试三次
		count = 2;
		bret = this->_APIv3SmallTV(rrid, loid);
		while ((bret != BILIRET::NOFAULT) && count) {
			Sleep(1000);
			bret = this->_APIv3SmallTV(rrid, loid);
			count--;
		}
	}

	return 0;
}

int CBilibiliUserInfo::ActGuard(const std::string &type, const int rrid, const long long loid) {
	if (_useropt.conf_guard == 1) {
		// 产生访问记录
		_APIv1RoomEntry(rrid);
		// 网页端API
		_APIv2LotteryJoin(type, rrid, loid);
		return 0;
	}
	return 0;
}

// 获取直播站主要信息
BILIRET CBilibiliUserInfo::GetUserInfoLive(BILIUSEROPT &pinfo) const {
	BOOST_LOG_SEV(g_logger::get(), info) << "[User] Get user live info...";
	_httppackweb->url = _urlapi + "/xlive/web-ucenter/user/get_user_info";
	_httppackweb->ClearHeader();
	int ret = toollib::HttpGetEx(curlweb, _httppackweb);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	rapidjson::Document doc;
	doc.Parse(_httppackweb->recv_data.c_str());
	if (!doc.IsObject()) {
		return BILIRET::JSON_ERROR;
	}
	bool isvalid = false;
	if (doc.HasMember("code") && doc["code"].IsInt() && !doc["code"].GetInt()) {
		isvalid = true;
	}
	if (!isvalid) {
		return BILIRET::JSON_ERROR;
	}
	
	return BILIRET::NOFAULT;
}

// 获取验证码图片
BILIRET CBilibiliUserInfo::GETLoginCaptcha() const {
	BOOST_LOG_SEV(g_logger::get(), info) << "[User] Get captcha...";
	_httppackapp->url = "https://passport.bilibili.com/captcha?rnd=569";
	_httppackapp->ClearHeader();
	int ret = toollib::HttpGetEx(curlapp, _httppackapp);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	std::ofstream out;
	out.open("Captcha.jpg", std::ios::binary);
	out.write(_httppackapp->recv_data.c_str(), _httppackapp->recv_data.size());
	out.close();

	return BILIRET::NOFAULT;
}

// 直播经验心跳Web
BILIRET CBilibiliUserInfo::PostOnlineHeart() const {
	// -403 非法心跳
	_httppackweb->url = _urlapi + "/User/userOnlineHeart";
	std::ostringstream oss;
	oss << "&csrf_token=" << _useropt.tokenjct
		<< "&csrf=" << _useropt.tokenjct
		<< "&visit_id=";
	_httppackweb->send_data = oss.str();
	_httppackweb->ClearHeader();
	_httppackweb->AddHeaderManual("Accept:application/json, text/javascript, */*; q=0.01");
	_httppackweb->AddHeaderManual(URL_DEFAULT_ORIGIN);
	_httppackweb->AddHeaderManual(URL_DEFAULT_REFERER);
	int ret = toollib::HttpPostEx(curlweb, _httppackweb);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	rapidjson::Document doc;
	doc.Parse(_httppackweb->recv_data.c_str());
	if (!doc.IsObject() || !doc.HasMember("code") || !doc["code"].IsInt()) {
		return BILIRET::JSON_ERROR;
	}
	ret = doc["code"].GetInt();
	if (ret) {
		BOOST_LOG_SEV(g_logger::get(), warning) << "[User" << _useropt.fileid << "] "
			<< "OnlineHeart web: " << _httppackweb->recv_data;
		return BILIRET::JSON_ERROR;
	}
	BOOST_LOG_SEV(g_logger::get(), info) << "[User" << _useropt.fileid << "] "
		<< "OnlineHeart web: OK";

	return BILIRET::NOFAULT;
}

// 直播站签到
BILIRET CBilibiliUserInfo::GetSign() const {
	_httppackweb->url = _urlapi + "/sign/doSign";
	_httppackweb->ClearHeader();
	int ret = toollib::HttpGetEx(curlweb, _httppackweb);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	// 0签到成功 -500已签到
	rapidjson::Document doc;
	doc.Parse(_httppackweb->recv_data.c_str());
	if (!doc.IsObject() || !doc.HasMember("code") || !doc["code"].IsInt()) {
		return BILIRET::JSON_ERROR;
	}
	ret = doc["code"].GetInt();
	if (ret == -500) {
		BOOST_LOG_SEV(g_logger::get(), info) << "[User" << _useropt.fileid << "] "
			<< "Sign: Signed.";
		return BILIRET::NOFAULT;
	}
	if (ret) {
		BOOST_LOG_SEV(g_logger::get(), warning) << "[User" << _useropt.fileid << "] "
			<< "Sign: " << _httppackweb->recv_data;
		return BILIRET::JSON_ERROR;
	}
	BOOST_LOG_SEV(g_logger::get(), info) << "[User" << _useropt.fileid << "] "
		<< "Sign: Success.";

	return BILIRET::NOFAULT;
}

// 获取登录硬币
BILIRET CBilibiliUserInfo::GetCoin() const {
	_httppackweb->url = "https://account.bilibili.com/site/getCoin";
	_httppackweb->ClearHeader();
	_httppackweb->AddHeaderManual("Accept: application/json, text/plain, */*");
	int ret = toollib::HttpGetEx(curlweb, _httppackweb);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	rapidjson::Document doc;
	doc.Parse(_httppackweb->recv_data.c_str());
	if (!doc.IsObject() || !doc.HasMember("code") || !doc["code"].IsInt() || doc["code"].GetInt()) {
		return BILIRET::JSON_ERROR;
	}
	double money = doc["data"]["money"].GetDouble();
	BOOST_LOG_SEV(g_logger::get(), info) << "[User" << _useropt.fileid << "] "
		<< "Current Coin: " << money;

	return BILIRET::NOFAULT;
}

BILIRET CBilibiliUserInfo::_GetTaskInfo() const {
	_httppackweb->url = _urlapi + "/i/api/taskInfo";
	_httppackweb->ClearHeader();
	_httppackweb->AddHeaderManual("Accept: application/json, text/plain, */*");
	_httppackweb->AddHeaderManual("Origin: https://link.bilibili.com");
	_httppackweb->AddHeaderManual("Referer: https://link.bilibili.com/p/center/index");
	int ret = toollib::HttpGetEx(curlweb, _httppackweb);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	rapidjson::Document doc;
	doc.Parse(_httppackweb->recv_data.c_str());
	if (!doc.IsObject() || !doc.HasMember("code") || !doc["code"].IsInt() || doc["code"].GetInt()
		|| !doc.HasMember("data") || !doc["data"].IsObject()) {
		return BILIRET::JSON_ERROR;
	}
	auto obj = doc["data"].GetObject();
	if (!obj.HasMember("double_watch_info") || !obj["double_watch_info"].IsObject()) {
		return BILIRET::JSON_ERROR;
	}
	obj = obj["double_watch_info"].GetObject();
	if (!obj.HasMember("status") || !obj["status"].IsInt()) {
		return BILIRET::JSON_ERROR;
	}
	ret = obj["status"].GetInt();
	BOOST_LOG_SEV(g_logger::get(), info) << "[User" << _useropt.fileid << "] "
		<< "Get taskinfo: " << ret;
	if (ret == 1) {
		return _APIv1TaskAward();
	}

	return BILIRET::NOFAULT;
}

// 直播经验心跳日志
BILIRET CBilibiliUserInfo::_APIv1HeartBeat() const {
	int ret;
	std::ostringstream oss;
	oss << _urlapi << "/relation/v1/feed/heartBeat?_=" << GetTimeStamp() << "479";
	_httppackweb->url = oss.str();
	_httppackweb->ClearHeader();
	_httppackweb->AddHeaderManual("Accept:*/*");
	_httppackweb->AddHeaderManual(URL_DEFAULT_REFERER);
	ret = toollib::HttpGetEx(curlweb, _httppackweb);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	rapidjson::Document doc;
	doc.Parse(_httppackweb->recv_data.c_str());
	if (!doc.IsObject() || !doc.HasMember("code") || !doc["code"].IsInt()) {
		return BILIRET::JSON_ERROR;
	}
	ret = doc["code"].GetInt();
	if (ret) {
		BOOST_LOG_SEV(g_logger::get(), info) << "[User" << _useropt.fileid << "] "
			<< "HeartBeat: " << ret;
	}
	else {
		BOOST_LOG_SEV(g_logger::get(), info) << "[User" << _useropt.fileid << "] "
			<< "HeartBeat: OK";
	}

	return BILIRET::NOFAULT;
}

// 获取节奏风暴验证码
BILIRET CBilibiliUserInfo::_APIv1Captcha(std::string &img, std::string &token) const {
	int ret;
	std::ostringstream oss;
	oss << _urlapi << "/captcha/v1/Captcha/create?_=" << GetTimeStamp() << "324&width=112&height=32";
	_httppackweb->url = oss.str();
	_httppackweb->ClearHeader();
	_httppackweb->AddHeaderManual(URL_DEFAULT_ORIGIN);
	ret = toollib::HttpGetEx(curlweb, _httppackweb);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	rapidjson::Document doc;
	doc.Parse(_httppackweb->recv_data.c_str());
	if (!doc.IsObject() || !doc.HasMember("code") || !doc["code"].IsInt() || doc["code"].GetInt()
		|| !doc.HasMember("data") || !doc["data"].IsObject()) {
		return BILIRET::JSON_ERROR;
	}
	img = doc["data"]["image"].GetString();
	token = doc["data"]["token"].GetString();
	ret = img.find("base64") + 7;
	img = img.substr(ret, img.length() - ret);

	return BILIRET::NOFAULT;
}

// 获取节奏风暴
BILIRET CBilibiliUserInfo::_APIv1StormJoin(int roomID, long long cid, std::string code, std::string token) {
	int ret;
	_httppackweb->url = _urlapi + "/lottery/v1/Storm/join";
	std::ostringstream oss;
	oss << "id=" << cid
		<< "&color=16777215&captcha_token=" << token
		<< "&captcha_phrase=" << code
		<< "&roomid=" << roomID
		<< "&csrf_token=" << _useropt.tokenjct
		<< "&visit_id=" << _useropt.visitid;
	_httppackweb->send_data = oss.str();
	_httppackweb->ClearHeader();
	_httppackweb->AddHeaderManual("Accept: application/json, text/plain, */*");
	_httppackweb->AddHeaderManual("Content-Type: application/x-www-form-urlencoded");
	_httppackweb->AddHeaderManual(URL_DEFAULT_ORIGIN);
	std::string strreffer(URL_DEFAULT_REFERERBASE);
	strreffer += std::to_string(roomID);
	_httppackweb->AddHeaderManual(strreffer.c_str());
	ret = toollib::HttpPostEx(curlweb, _httppackweb);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	rapidjson::Document doc;
	doc.Parse(_httppackweb->recv_data.c_str());
	if (!doc.IsObject() || !doc.HasMember("code") || !doc["code"].IsInt()) {
		return BILIRET::JSON_ERROR;
	}
	ret = doc["code"].GetInt();
	std::string msg;
	// 429需要验证码 400未抽中或已过期
	if (ret) {
		msg = doc["msg"].GetString();
		BOOST_LOG_SEV(g_logger::get(), warning) << "[User" << _useropt.fileid << "] "
			<< "Storm " << ret << " " << msg;
		// 检查是否被封禁
		if (ret == 400) {
			if (CheckBanned(msg)) {
				return BILIRET::NOFAULT;
			}
		}
		return BILIRET::JOINEVENT_FAILED;
	}
	// 抽中的返回值为0
	msg = doc["data"]["mobile_content"].GetString();
	BOOST_LOG_SEV(g_logger::get(), info) << "[User" << _useropt.fileid << "] "
		<< "Storm " << msg;

	return BILIRET::NOFAULT;
}

// 银瓜子换硬币
BILIRET CBilibiliUserInfo::_APIv1Silver2Coin() const {
	_httppackweb->url = _urlapi + "/pay/v1/Exchange/silver2coin";
	_httppackweb->send_data = "platform=pc&csrf_token=" + _useropt.tokenjct;
	_httppackweb->ClearHeader();
	_httppackweb->AddHeaderManual("Accept: application/json, text/javascript, */*; q=0.01");
	_httppackweb->AddHeaderManual("Content-Type: application/x-www-form-urlencoded; charset=UTF-8");
	_httppackweb->AddHeaderManual(URL_DEFAULT_ORIGIN);
	_httppackweb->AddHeaderManual("Referer: https://live.bilibili.com/exchange");
	int ret = toollib::HttpPostEx(curlweb, _httppackweb);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	// 0兑换成功 403已兑换
	rapidjson::Document doc;
	doc.Parse(_httppackweb->recv_data.c_str());
	if (!doc.IsObject() || !doc.HasMember("code") || !doc["code"].IsInt()) {
		return BILIRET::JSON_ERROR;
	}
	ret = doc["code"].GetInt();
	std::string msg = doc["msg"].GetString();
	BOOST_LOG_SEV(g_logger::get(), info) << "[User" << _useropt.fileid << "] "
		<< "v1Silver2Coin: " << msg;

	return BILIRET::NOFAULT;
}

// 查询扭蛋币数量
BILIRET CBilibiliUserInfo::_APIv1CapsuleCheck() const {
	_httppackweb->url = _urlapi + "/xlive/web-ucenter/v1/capsule/get_detail?from=room";
	_httppackweb->ClearHeader();
	_httppackweb->AddHeaderManual("Accept: application/json, text/javascript, */*; q=0.01");
	_httppackweb->AddHeaderManual(URL_DEFAULT_ORIGIN);
	_httppackweb->AddHeaderManual(URL_DEFAULT_REFERER);
	int ret = toollib::HttpGetEx(curlweb, _httppackweb);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	rapidjson::Document doc;
	doc.Parse(_httppackweb->recv_data.c_str());
	if (!doc.IsObject() || !doc.HasMember("code") 
		|| !doc["code"].IsInt() || doc["code"].GetInt()
		|| !doc.HasMember("data") || !doc["data"].IsObject()) {
		return BILIRET::JSON_ERROR;
	}
	printf("  Capsule info:\n");
	if (doc["data"].HasMember("normal") && doc["data"]["normal"].IsObject() 
		&& doc["data"]["normal"].HasMember("coin") && doc["data"]["normal"]["coin"].IsInt()) {
		BOOST_LOG_SEV(g_logger::get(), info) << "[User" << _useropt.fileid << "] "
			<< "Normal: " << doc["data"]["normal"]["coin"].GetInt();
	}
	if (doc["data"].HasMember("colorful") && doc["data"]["colorful"].IsObject()
		&& doc["data"]["colorful"].HasMember("coin") && doc["data"]["colorful"]["coin"].IsInt()) {
		BOOST_LOG_SEV(g_logger::get(), info) << "[User" << _useropt.fileid << "] "
			<< "Colorful: " << doc["data"]["colorful"]["coin"].GetInt();
	}

	return BILIRET::NOFAULT;
}

BILIRET CBilibiliUserInfo::_APIv1RoomEntry(int room) const {
	int ret;
	_httppackweb->url = _urlapi + "/room/v1/Room/room_entry_action";
	std::ostringstream oss;
	oss << "room_id=" << room
		<< "&platform=pc&csrf_token=" << _useropt.tokenjct
		<< "&visit_id=" << _useropt.visitid;
	_httppackweb->send_data = oss.str();
	_httppackweb->ClearHeader();
	_httppackweb->AddHeaderManual("Accept: application/json, text/plain, */*");
	_httppackweb->AddHeaderManual("Content-Type: application/x-www-form-urlencoded");
	_httppackweb->AddHeaderManual(URL_DEFAULT_ORIGIN);
	std::string strreffer(URL_DEFAULT_REFERERBASE);
	strreffer += std::to_string(room);
	_httppackweb->AddHeaderManual(strreffer.c_str());
	ret = toollib::HttpPostEx(curlweb, _httppackweb);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	return BILIRET::NOFAULT;
}

BILIRET CBilibiliUserInfo::_APIv1TaskAward() const {
	_httppackweb->url = _urlapi + "/activity/v1/task/receive_award";
	std::ostringstream oss;
	oss << "task_id=double_watch_task" 
		<< "&csrf_token=" << _useropt.tokenjct
		<< "&csrf=" << _useropt.tokenjct;
	_httppackweb->send_data = oss.str();
	_httppackweb->ClearHeader();
	_httppackweb->AddHeaderManual("Accept: application/json, text/plain, */*");
	_httppackweb->AddHeaderManual("Content-Type: application/x-www-form-urlencoded");
	_httppackweb->AddHeaderManual("Origin: https://link.bilibili.com");
	_httppackweb->AddHeaderManual("Referer: https://link.bilibili.com/p/center/index");
	int ret = toollib::HttpPostEx(curlweb, _httppackweb);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	rapidjson::Document doc;
	doc.Parse(_httppackweb->recv_data.c_str());
	if (!doc.IsObject() || !doc.HasMember("code") || !doc["code"].IsInt()) {
		BOOST_LOG_SEV(g_logger::get(), warning) << "[User" << _useropt.fileid << "] "
			<< "Get double watch award failed.";
		return BILIRET::JSON_ERROR;
	}
	BOOST_LOG_SEV(g_logger::get(), info) << "[User" << _useropt.fileid << "] "
		<< "Get double watch award: " << doc["code"].GetInt();

	return BILIRET::NOFAULT;
}

// 查询背包道具
BILIRET CBilibiliUserInfo::_APIv2GiftBag() const {
	_httppackweb->url = _urlapi + "/gift/v2/gift/bag_list";
	_httppackweb->ClearHeader();
	_httppackweb->AddHeaderManual("Accept: application/json, text/plain, */*");
	_httppackweb->AddHeaderManual(URL_DEFAULT_ORIGIN);
	_httppackweb->AddHeaderManual(URL_DEFAULT_REFERER);
	int ret = toollib::HttpGetEx(curlweb, _httppackweb);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	rapidjson::Document doc;
	doc.Parse(_httppackweb->recv_data.c_str());
	if (!doc.IsObject() || !doc.HasMember("code") || !doc["code"].IsInt() || doc["code"].GetInt()) {
		BOOST_LOG_SEV(g_logger::get(), warning) << "[User" << _useropt.fileid << "] "
			<< "Get bag info failed.";
		return BILIRET::JSON_ERROR;
	}
	printf("  Current bag info: \n");
	int curtime, expiretime;
	std::string giftname;
	curtime = doc["data"]["time"].GetInt();
	rapidjson::Value &datalist = doc["data"]["list"];
	unsigned int i;
	for (i = 0; i < datalist.Size(); i++) {
		giftname = datalist[i]["gift_name"].GetString();
		printf("Gift:%8s   Num:%5d   ", giftname.c_str(), datalist[i]["gift_num"].GetInt());
		expiretime = datalist[i]["expire_at"].GetInt();
		if (expiretime) {
			expiretime -= curtime;
			printf("Expire:%6.2f Day\n", expiretime/86400.0);
		}
		else {
			printf("Expire: Infinite\n");
		}
	}

	return BILIRET::NOFAULT;
}

// 领取每日礼物
BILIRET CBilibiliUserInfo::_APIv2GiftDaily() const {
	_httppackweb->url = _urlapi + "/gift/v2/live/receive_daily_bag";
	_httppackweb->ClearHeader();
	_httppackweb->AddHeaderManual("Accept: application/json, text/plain, */*");
	_httppackweb->AddHeaderManual(URL_DEFAULT_ORIGIN);
	_httppackweb->AddHeaderManual(URL_DEFAULT_REFERER);
	int ret = toollib::HttpGetEx(curlweb, _httppackweb);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	rapidjson::Document doc;
	doc.Parse(_httppackweb->recv_data.c_str());
	if (!doc.IsObject() || !doc.HasMember("code") || !doc["code"].IsInt() || doc["code"].GetInt()) {
		BOOST_LOG_SEV(g_logger::get(), warning) << "[User" << _useropt.fileid << "] "
			<< "Receive daily gift failed.";
		return BILIRET::JSON_ERROR;
	}
	ret = doc["data"]["bag_status"].GetInt();
	if (ret == 1) {
		BOOST_LOG_SEV(g_logger::get(), info) << "[User" << _useropt.fileid << "] "
			<< u8"每日礼物领取成功";
	}
	else if (ret == 2) {
		BOOST_LOG_SEV(g_logger::get(), info) << "[User" << _useropt.fileid << "] "
			<< u8"每日礼物已领取";
	}
	else {
		BOOST_LOG_SEV(g_logger::get(), warning) << "[User" << _useropt.fileid << "] "
			<< "Receive daily gift Code: " << ret;
	}

	return BILIRET::NOFAULT;
}

BILIRET CBilibiliUserInfo::_APIv2LotteryJoin(const std::string &type, const int rrid, const long long loid) {
	int ret;
	_httppackweb->url = _urlapi + "/lottery/v2/Lottery/join";
	std::ostringstream oss;
	oss << "roomid=" << rrid
		<< "&id=" << loid
		<< "&type=" << type
		<< "&csrf_token=" << _useropt.tokenjct
		<< "&visit_id=" << _useropt.visitid;
	_httppackweb->send_data = oss.str();
	_httppackweb->ClearHeader();
	_httppackweb->AddHeaderManual("Accept: application/json, text/plain, */*");
	_httppackweb->AddHeaderManual("Content-Type: application/x-www-form-urlencoded");
	_httppackweb->AddHeaderManual(URL_DEFAULT_ORIGIN);
	std::string strreffer(URL_DEFAULT_REFERERBASE);
	strreffer += std::to_string(rrid);
	_httppackweb->AddHeaderManual(strreffer.c_str());
	ret = toollib::HttpPostEx(curlweb, _httppackweb);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	rapidjson::Document doc;
	doc.Parse(_httppackweb->recv_data.c_str());
	if (!doc.IsObject() || !doc.HasMember("code") || !doc["code"].IsInt()) {
		return BILIRET::JSON_ERROR;
	}

	std::string tmpstr;
	int icode = doc["code"].GetInt();
	if (icode) {
		// 检查是否被封禁
		if (icode == 400) {
			this->SetBanned();
		}
		if (doc.HasMember("message") && doc["message"].IsString()) {
			tmpstr = doc["message"].GetString();
		}
	}
	else {
		tmpstr = doc["data"]["message"].GetString();
	}
	BOOST_LOG_SEV(g_logger::get(), info) << "[User" << _useropt.fileid << "] "
		<< "Lottery " << tmpstr;

	return BILIRET::NOFAULT;
}

// 通告礼物抽奖
BILIRET CBilibiliUserInfo::_APIv3SmallTV(int rrid, long long loid)
{
	int ret;
	_httppackweb->url = _urlapi + "/xlive/lottery-interface/v3/smalltv/Join";
	std::ostringstream oss;
	oss << "roomid=" << rrid
		<< "&raffleId=" << loid
		<< "&type=Gift&csrf_token=" << _useropt.tokenjct
		<< "&csrf=" << _useropt.tokenjct
		<< "&visit_id=" << _useropt.visitid;
	_httppackweb->send_data = oss.str();
	_httppackweb->ClearHeader();
	_httppackweb->AddHeaderManual("Accept: application/json, text/plain, */*");
	_httppackweb->AddHeaderManual("Content-Type: application/x-www-form-urlencoded");
	_httppackweb->AddHeaderManual(URL_DEFAULT_ORIGIN);
	oss.str("");
	oss << URL_DEFAULT_REFERERBASE << rrid;
	_httppackweb->AddHeaderManual(oss.str().c_str());
	ret = toollib::HttpPostEx(curlweb, _httppackweb);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	rapidjson::Document doc;
	doc.Parse(_httppackweb->recv_data.c_str());
	if (!doc.IsObject() || !doc.HasMember("code") || !doc["code"].IsInt()) {
		return BILIRET::JSON_ERROR;
	}

	// 0成功 -400已领取 -500系统繁忙
	int icode = doc["code"].GetInt();
	if (icode) {
		// 检查是否被封禁
		if (icode == 400) {
			this->SetBanned();
		}
		std::string tmpstr;
		if (doc.HasMember("message") && doc["message"].IsString()) {
			tmpstr = doc["message"].GetString();
		}
		BOOST_LOG_SEV(g_logger::get(), info) << "[User" << _useropt.fileid << "] "
			<< "Gift Error " << tmpstr;
		return BILIRET::NOFAULT;
	}
	BOOST_LOG_SEV(g_logger::get(), info) << "[User" << _useropt.fileid << "] "
		<< "Gift Success ";

	return BILIRET::NOFAULT;
}

// 移动端加密密钥
BILIRET CBilibiliUserInfo::_APIAndv2GetKey(std::string &psd) const
{
	BOOST_LOG_SEV(g_logger::get(), info) << "[User] Get APP RSA key...";
	_httppackapp->url = "https://passport.bilibili.com/api/oauth2/getKey";
	std::ostringstream oss;
	oss << "appkey=" << APP_KEY
		<< "&build=" << PARAM_BUILD
		<< "&mobi_app=android&platform=android&ts=" << GetTimeStamp();
	std::string sign;
	this->_GetMD5Sign(oss.str().c_str(), sign);
	oss << "&sign=" << sign;
	_httppackapp->send_data = oss.str();
	_httppackapp->ClearHeader();
	_httppackapp->AddHeaderManual("Content-Type: application/x-www-form-urlencoded; charset=utf-8");
	int ret = toollib::HttpPostEx(curlapp, _httppackapp);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	rapidjson::Document doc;
	doc.Parse(_httppackapp->recv_data.c_str());
	if (!doc.IsObject() || !doc.HasMember("code")
		|| !doc["code"].IsInt() || doc["code"].GetInt()) {
		return BILIRET::JSON_ERROR;
	}

	std::string tmp_strhash = doc["data"]["hash"].GetString();
	std::string tmp_strpubkey = doc["data"]["key"].GetString();
	tmp_strhash += psd;
	ret = Encrypt_RSA_KeyBuff((char*)tmp_strpubkey.c_str(), tmp_strhash, psd);
	if (ret) {
		return BILIRET::NOFAULT;
	}

	return BILIRET::OPENSSL_ERROR;
}

// 移动端登录接口
BILIRET CBilibiliUserInfo::_APIAndv2Login(std::string username, std::string password, std::string captcha)
{
	BOOST_LOG_SEV(g_logger::get(), info) << "[User] Logging in by app api...";
	_httppackapp->url = "https://passport.bilibili.com/api/v2/oauth2/login";
	std::ostringstream oss;
	oss << "appkey=" << APP_KEY
		<< "&build=" << PARAM_BUILD;
	if (captcha != "") {
		oss << "&captcha=" << captcha;
	}
	oss << "&mobi_app=android&password=" << _strcoding.UrlEncode(password.c_str())
		<< "&platform=android&ts=" << GetTimeStamp()
		<< "&username=" << username;
	std::string sign;
	this->_GetMD5Sign(oss.str().c_str(), sign);
	oss << "&sign=" << sign;
	_httppackapp->send_data = oss.str();
	_httppackapp->ClearHeader();
	_httppackapp->AddHeaderManual("Content-Type: application/x-www-form-urlencoded; charset=utf-8");
	int ret = toollib::HttpPostEx(curlapp, _httppackapp);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	rapidjson::Document doc;
	doc.Parse(_httppackapp->recv_data.c_str());
	if (!doc.IsObject() || !doc.HasMember("code") || !doc["code"].IsInt()) {
		return BILIRET::JSON_ERROR;
	}
	ret = doc["code"].GetInt();
	if (ret == -105) {
		return BILIRET::LOGIN_NEEDVERIFY;
	}
	if (ret) {
		return BILIRET::LOGIN_PASSWORDWRONG;
	}

	// 登录成功
	_useropt.uid = doc["data"]["token_info"]["mid"].GetInt();
	_useropt.tokena = doc["data"]["token_info"]["access_token"].GetString();
	_useropt.tokenr = doc["data"]["token_info"]["refresh_token"].GetString();
	oss.str("");
	rapidjson::Value &cookie_list = doc["data"]["cookie_info"]["cookies"];
	for (unsigned i = 0; i < cookie_list.Size(); i++) {
		if (cookie_list[i]["http_only"].GetInt()) {
			oss << "#HttpOnly_";
		}
		oss << ".bilibili.com\tTRUE\t/\tFALSE\t"
			<< cookie_list[i]["expires"].GetInt64() << "\t"
			<< cookie_list[i]["name"].GetString() << "\t"
			<< cookie_list[i]["value"].GetString() << "\n";
	}
	_httpcookie.ImportCookies(oss.str(), curlweb);

	return BILIRET::NOFAULT;
}

// 客户端经验心跳
BILIRET CBilibiliUserInfo::_APIAndHeart() {
	_httppackapp->url = _urlapi + "/mobile/userOnlineHeart?";
	std::ostringstream oss;
	oss << "access_key=" << _useropt.tokena
		<< "&actionKey=appkey&appkey=" << APP_KEY
		<< "&build=" << PARAM_BUILD
		<< "&device=android&mobi_app=android&platform=android&ts=" << GetTimeStamp();
	std::string sign;
	this->_GetMD5Sign(oss.str().c_str(), sign);
	oss << "&sign=" << sign;
	_httppackapp->url += oss.str();
	_httppackapp->send_data = "{'roomid': 23058, 'scale': 'xhdpi'}";
	_httppackapp->ClearHeader();
	int ret = toollib::HttpPostEx(curlapp, _httppackapp);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	rapidjson::Document doc;
	doc.Parse(_httppackapp->recv_data.c_str());
	if (!doc.IsObject() || !doc.HasMember("code") || !doc["code"].IsInt()) {
		return BILIRET::JSON_ERROR;
	}
	ret = doc["code"].GetInt();
	if (ret) {
		BOOST_LOG_SEV(g_logger::get(), warning) << "[User" << _useropt.fileid << "] "
			<< "OnlineHeart mobile: " << _httppackweb->recv_data;
		return BILIRET::JSON_ERROR;
	}
	BOOST_LOG_SEV(g_logger::get(), info) << "[User" << _useropt.fileid << "] "
		<< "OnlineHeart mobile: OK";

	return BILIRET::NOFAULT;
}

//获取当前宝箱领取情况
BILIRET CBilibiliUserInfo::_APIAndSilverCurrentTask() {
	_httppackapp->url = _urlapi + "/mobile/freeSilverCurrentTask";
	std::ostringstream oss;
	oss << "access_key=" << _useropt.tokena
		<< "&actionKey=appkey&appkey=" << APP_KEY
		<< "&build=" << PARAM_BUILD
		<< "&device=android&mobi_app=android&platform=android&ts=" << GetTimeStamp();
	std::string sign;
	this->_GetMD5Sign(oss.str().c_str(), sign);
	oss << "&sign=" << sign;
	_httppackapp->send_data = oss.str();
	_httppackapp->ClearHeader();
	int ret = toollib::HttpPostEx(curlapp, _httppackapp);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}
	// 领完为-10017

	rapidjson::Document doc;
	doc.Parse(_httppackapp->recv_data.c_str());
	if (!doc.IsObject() || !doc.HasMember("code")) {
		return BILIRET::JSON_ERROR;
	}
	ret = doc["code"].GetInt();
	if (ret != 0) {
		BOOST_LOG_SEV(g_logger::get(), info) << "[User" << _useropt.fileid << "] "
			<< "GetFreeSilver: No new task.";
		_heartopt.silvercount = -1;
		return BILIRET::NOFAULT;
	}
	_useropt.silver_minute = doc["data"]["minute"].GetInt();
	_useropt.silver_count = doc["data"]["silver"].GetInt();
	_useropt.silver_start = doc["data"]["time_start"].GetInt();
	_useropt.silver_end = doc["data"]["time_end"].GetInt();
	_heartopt.silvercount = _useropt.silver_minute;
	BOOST_LOG_SEV(g_logger::get(), info) << "[User" << _useropt.fileid << "] "
		<< "GetFreeSilver: Wait:" << _heartopt.silvercount << " Amount:" << _useropt.silver_count;

	return BILIRET::NOFAULT;
}

// 领取银瓜子
BILIRET CBilibiliUserInfo::_APIAndSilverAward() {
	_httppackapp->url = _urlapi + "/mobile/freeSilverAward";
	std::ostringstream oss;
	oss << "access_key=" << _useropt.tokena
		<< "&actionKey=appkey&appkey=" << APP_KEY
		<< "&build=" << PARAM_BUILD
		<< "&device=android&mobi_app=android&platform=android&time_end=" << _useropt.silver_end
		<< "&time_start=" << _useropt.silver_start
		<< "&ts=" << GetTimeStamp();
	std::string sign;
	this->_GetMD5Sign(oss.str().c_str(), sign);
	oss << "&sign=" << sign;
	_httppackapp->send_data = oss.str();
	_httppackapp->ClearHeader();
	int ret = toollib::HttpPostEx(curlapp, _httppackapp);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	rapidjson::Document doc;
	doc.Parse(_httppackapp->recv_data.c_str());
	if (!doc.IsObject() || !doc.HasMember("code")) {
		return BILIRET::JSON_ERROR;
	}
	ret = doc["code"].GetInt();
	if (ret) {
		BOOST_LOG_SEV(g_logger::get(), info) << "[User" << _useropt.fileid << "] "
			<< "GetFreeSilver: Error.";
		_heartopt.silvercount = 0;
		return BILIRET::NOFAULT;
	}

	if (doc["data"]["isEnd"].GetInt()) {
		BOOST_LOG_SEV(g_logger::get(), info) << "[User" << _useropt.fileid << "] "
			<< "GetFreeSilver: Finish.";
		_heartopt.silvercount = -1;
	}
	else {
		BOOST_LOG_SEV(g_logger::get(), info) << "[User" << _useropt.fileid << "] "
			<< "GetFreeSilver: Success.";
		_heartopt.silvercount = 0;
	}
	return BILIRET::NOFAULT;
}

// 领取风暴
BILIRET CBilibiliUserInfo::_APIAndv1StormJoin(long long cid) {
	_httppackapp->url = _urlapi + "/lottery/v1/Storm/join";
	std::ostringstream oss;
	oss << "access_key=" << _useropt.tokena
		<< "&actionKey=appkey&appkey=" << APP_KEY
		<< "&build=" << PARAM_BUILD
		<< "&device=android&id=" << cid
		<< "&mobi_app=android&platform=android&ts=" << GetTimeStamp();
	std::string sign;
	this->_GetMD5Sign(oss.str().c_str(), sign);
	oss << "&sign=" << sign;
	_httppackapp->send_data = oss.str();
	_httppackapp->ClearHeader();
	_httppackapp->AddHeaderManual("Content-Type: application/x-www-form-urlencoded; charset=utf-8");
	int ret = toollib::HttpPostEx(curlapp, _httppackapp);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	rapidjson::Document doc;
	doc.Parse(_httppackapp->recv_data.c_str());
	if (!doc.IsObject() || !doc.HasMember("code") || !doc["code"].IsInt()) {
		return BILIRET::JSON_ERROR;
	}
	ret = doc["code"].GetInt();
	std::string msg;
	if (ret) {
		msg = doc["msg"].GetString();
		BOOST_LOG_SEV(g_logger::get(), info) << "[User" << _useropt.fileid << "] "
			<< "Storm " << ret << " " << msg;
		// 检查是否被封禁
		if (ret == 400) {
			if (CheckBanned(msg)) {
				return BILIRET::NOFAULT;
			}
		}
		return BILIRET::JOINEVENT_FAILED;
	}
	msg = doc["data"]["mobile_content"].GetString();
	BOOST_LOG_SEV(g_logger::get(), info) << "[User" << _useropt.fileid << "] "
		<< "Storm " << msg;

	return BILIRET::NOFAULT;
}

int CBilibiliUserInfo::_GetMD5Sign(const char *in, std::string &sign) const {
	std::string str = in;
	str += APP_SECRET;
	sign = Encode_MD5(str);
	return 0;
}

int CBilibiliUserInfo::GetExpiredTime() {
	std::string ckname;
	int cktime;
	ckname = "bili_jct";
	_httpcookie.UpdateCookies(curlweb);
	if (_httpcookie.GetCookieTime(ckname, cktime))
		return 0;
	return cktime;
}

int CBilibiliUserInfo::GetToken(BILIUSEROPT &opt) {
	std::string ckname;
	_httpcookie.UpdateCookies(curlweb);
	ckname = "bili_jct";
	if (_httpcookie.GetCookie(ckname, opt.tokenjct)) {
		opt.tokenjct = "";
		return -1;
	}
	return 0;
}

int CBilibiliUserInfo::AccountVerify() {
	return 0;
}

bool CBilibiliUserInfo::CheckBanned(const std::string &msg) {
	std::wstring wmsg;
	CStrConvert::UTF8ToUTF16(msg, wmsg);
	if (wmsg.find(L"访问被拒绝") != -1) {
		this->SetBanned();
		return true;
	}
	return false;
}

int CBilibiliUserInfo::SetBanned() {
	// 账户被封禁时取消所有抽奖
	_useropt.conf_lottery = 0;
	_useropt.conf_storm = 0;
	_useropt.conf_guard = 0;
	return 0;
}

BILIRET CBilibiliUserInfo::_GetCaptchaKey() {
	// 新增Cookie验证
	int ret;
	_httppackweb->url = "https://www.bilibili.com/plus/widget/ajaxGetCaptchaKey.php?js";
	_httppackweb->ClearHeader();
	ret = toollib::HttpGetEx(curlweb, _httppackweb);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}
	ret = _httppackweb->recv_data.find("captcha_key");
	if (ret == -1)
		return BILIRET::HTMLTEXT_ERROR;
	// finger生成于如下脚本中
	//https://s1.hdslb.com/bfs/seed/log/report/log-reporter.js
	std::string strcookie;
	_httpcookie.ExportCookies(strcookie, curlweb);
	if (strcookie.find("finger") != -1) {
		return BILIRET::NOFAULT;
	}
	strcookie += ".bilibili.com\tTRUE\t/\tFALSE\t0\tfinger\tedc6ecda\n";
	_httpcookie.ImportCookies(strcookie, curlweb);
	_httpcookie.ExportCookies(strcookie, curlweb);

	return BILIRET::NOFAULT;
}

// 生成随机访问ID
// ((new Date).getTime() * Math.ceil(1e6 * Math.random())).toString(36)
int CBilibiliUserInfo::_GetVisitID(std::string &sid) const {
	long long randid;
	char ch;
	srand((unsigned int)time(0)); 
	// 生成一个毫秒级的时间
	randid = GetTimeStamp() * 1000 + rand() % 1000;
	// 乘以一个随机数
	randid *= ((rand() & 0xfff) << 12 | (rand() & 0xfff)) % 1000000;
	// 转换为36进制字符串
	sid = "";
	while (randid) {
		ch = randid % 36;
		if (ch < 10) {
			ch += '0';
		}
		else {
			ch += 'a' - 10;
		}
		sid += ch;
		randid /= 36;
	}
	std::reverse(sid.begin(), sid.end());
	return 0;
}

// 大写转小写
int CBilibiliUserInfo::_ToLower(std::string &str) const {
	std::string tmpstr = "";
	char ch;
	for (unsigned int i = 0; i < str.length(); i++) {
		ch = str.at(i);
		if (ch >= 'A'&&ch <= 'Z') 
			ch = ch + 32;
		tmpstr += ch;
	}
	str = tmpstr;
	return 0;
}
