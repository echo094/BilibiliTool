﻿#include "stdafx.h"
#include "BilibiliUserInfo.h"
#include <iostream>
#include <sstream> 
#include <fstream>  
using namespace rapidjson;

CBilibiliUserInfo::CBilibiliUserInfo()
{
	_useropt.fileid = 0;
	_useropt.islogin = false;
	_useropt.conf = 0;
	_urlapi = URL_LIVEAPI_HEAD;
	_key_3des = "08E1CAAD409B840208E1CAAD";
	curlweb = curl_easy_init();
	curlapp = curl_easy_init();
	_httppackweb = std::make_unique<CHTTPPack>();
	_httppackweb->AddDefHeader("Accept-Language: zh-CN,zh;q=0.8");
	_httppackweb->AddDefHeader("Connection: keep-alive");
	_httppackweb->AddDefHeader("DNT: 1");
	_httppackapp = std::make_unique<CHTTPPack>("Mozilla/5.0 BiliDroid/5.23.2 (bbcallen@gmail.com)");
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
#ifdef _DEBUG
	printf("[User] Stop. \n");
#endif
}

// 新用户登录
LOGINRET CBilibiliUserInfo::Login(int index, std::string username, std::string password) {
	BILIRET bret;
	int ret;
	_useropt.account = username;
	_useropt.password = password;

	// 将账号转码
	username = _strcoding.UrlEncode(_useropt.account);

	// 为避免异常情况必须有两次登录
	// 无验证码登录
	bret = GETPicCaptcha();
	if (bret != BILIRET::NOFAULT) {
		return LOGINRET::NOTLOGIN;
	}
	bret = GETEncodePsd(password);
	if (bret != BILIRET::NOFAULT) {
		return LOGINRET::NOTLOGIN;
	}
	POSTLogin(username, password);

	// 移动端登录
	password = _useropt.password;
	bret = _APIAndv2GetKey(password);
	if (bret != BILIRET::NOFAULT) {
		return LOGINRET::NOTLOGIN;
	}
	bret = _APIAndv2Login(username, password);
	if (bret != BILIRET::NOFAULT) {
		return LOGINRET::NOTLOGIN;
	}

	// 网页端有验证码登录
	password = _useropt.password;
	bret = GETPicCaptcha();
	if (bret != BILIRET::NOFAULT) {
		return LOGINRET::NOTLOGIN;
	}
	std::string tmp_chcode;
	printf("[User] Enter the pic validate code: ");
	std::cin >> tmp_chcode;
	bret = GETEncodePsd(password);
	if (bret != BILIRET::NOFAULT) {
		return LOGINRET::NOTLOGIN;
	}
	bret =  POSTLogin(username, password, tmp_chcode);
	if (bret != BILIRET::NOFAULT) {
		return LOGINRET::NOTLOGIN;
	}

	// 登录成功则获取必要的临时id
	bret = _GetCaptchaKey();
	if (bret != BILIRET::NOFAULT) {
		_useropt.islogin = false;
		return LOGINRET::NOTLOGIN;
	}
	// 获取必要的cookie字段
	GETLoginJct(2);
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
	std::string ckname, ckvalue;
	ckname = "DedeUserID";
	_httpcookie.UpdateCookies(curlweb);
	if (_httpcookie.GetCookie(ckname, ckvalue) == 0)
		_useropt.uid = ckvalue;
	else
		_useropt.uid = "";
	// 默认开启所有功能
	if (_useropt.conf == 0) {
		_useropt.conf = 0b1001111;
	}
	_useropt.islogin = true;

	return LOGINRET::NOFAULT;
}

// 重新登录
LOGINRET CBilibiliUserInfo::Relogin() {
	int ret = 0;
	LOGINRET lret;
	ret = GetExpiredTime();
	long long rtime;
	rtime = ret - _tool.GetTimeStamp();
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
	GETLoginJct(2);
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
	printf("[User%d]\n", _useropt.fileid);
	_APIv2GiftBag();
	_APIv1CapsuleCheck();
	return 0;
}

// 从文件导入指定账户
int CBilibiliUserInfo::ReadFileAccount(std::string key, int index, char *addr) {
	int ret;
	int ilenck;
	char *tmpch;
	std::string tmps, tmpcookie, enstr;

	tmps = "User" + std::to_string(index);
	ilenck = ::GetPrivateProfileIntA(tmps.c_str(), "CookieLength", -1, addr);
	if (ilenck < 0) {
		return -1;
	}

	// 如果用户存在
	_useropt.fileid = index;
	if (ilenck < 256)
		tmpch = new char[256];
	else
		tmpch = new char[ilenck + 10];
	ret = ::GetPrivateProfileStringA(tmps.c_str(), "UserID", "", tmpch, 30, addr);
	_useropt.uid = tmpch;
	ret = ::GetPrivateProfileStringA(tmps.c_str(), "Username", "", tmpch, 30, addr);
	_useropt.account = tmpch;

	// 解密密码
	ret = ::GetPrivateProfileStringA(tmps.c_str(), "Password", "", tmpch, 256, addr);
	enstr = tmpch;
	if (enstr == "") {
		_useropt.password = "";
	}
	else {
		ret = Decrypt_RSA_KeyBuff((char*)key.c_str(), enstr, _useropt.password);
		if (!ret)
			_useropt.password = "";
	}

	// 解密Cookie
	ret = ::GetPrivateProfileStringA(tmps.c_str(), "Cookie", "", tmpch, ilenck+5, addr);
	enstr = tmpch;
	if (enstr == "") {
		tmpcookie = "";
	}
	else {
		ret = Decrypt_3DES_BASE64(_key_3des, enstr, tmpcookie);
		if (ret <= 0)
			tmpcookie = "";
		else
			_httpcookie.ImportCookies(tmpcookie, curlweb);
	}

	// 移动端Token 需要加密
	ret = ::GetPrivateProfileStringA(tmps.c_str(), "AccessToken", "", tmpch, 40, addr);
	_useropt.tokena = tmpch;
	ret = ::GetPrivateProfileStringA(tmps.c_str(), "RefreshToken", "", tmpch, 40, addr);
	_useropt.tokenr = tmpch;

	// 获取配置信息 各种活动的参与参数
	_useropt.conf = ::GetPrivateProfileIntA(tmps.c_str(), "Conf", 0, addr);

	delete[] tmpch;
	return 0;
}

// 将账户信息导出到文件
int CBilibiliUserInfo::WriteFileAccount(std::string key, char *addr) {
	int ret;
	std::string tmps, tmpcookie, enstr;
	char tcfg[10] = "";

	// 序号错误则退出
	if (_useropt.fileid == 0) {
		return -1;
	}
	tmps = "User" + std::to_string(_useropt.fileid);

	ret = ::WritePrivateProfileStringA(tmps.c_str(), "UserID", _useropt.uid.c_str(), addr);
	ret = ::WritePrivateProfileStringA(tmps.c_str(), "Username", _useropt.account.c_str(), addr);

	// 加密密码
	ret = Encrypt_RSA_KeyBuff((char*)key.c_str(), _useropt.password, enstr);
	if (!ret)
		enstr = "";
	ret = ::WritePrivateProfileStringA(tmps.c_str(), "Password", enstr.c_str(), addr);

	// 加密Cookie
	_httpcookie.ExportCookies(tmpcookie, curlweb);
	ret = Encrypt_3DES_BASE64(_key_3des, tmpcookie, enstr);
	if (ret < 0)
		enstr = "";
	ret = ::WritePrivateProfileStringA(tmps.c_str(), "Cookie", enstr.c_str(), addr);
	ret = ::WritePrivateProfileStringA(tmps.c_str(), "CookieLength", std::to_string(enstr.length()).c_str(), addr);

	// 移动端Token
	ret = ::WritePrivateProfileStringA(tmps.c_str(), "AccessToken", _useropt.tokena.c_str(), addr);
	ret = ::WritePrivateProfileStringA(tmps.c_str(), "RefreshToken", _useropt.tokenr.c_str(), addr);

	// 设置项
	sprintf_s(tcfg, sizeof(tcfg), "%d", _useropt.conf);
	ret = ::WritePrivateProfileStringA(tmps.c_str(), "Conf", tcfg, addr);

	return 0;
}

// 开启经验心跳
int CBilibiliUserInfo::ActStartHeart() {
	// 常规心跳
	_APIv1HeartBeat();
	// 活动礼物信息获取
	_heartopt.freegift = false;
	_APIv2CheckHeartGift();
	if (_heartopt.freegift) {
		_APIv2GetHeartGift();
	}
	// 心跳计时标签
	_heartopt.timercount = 0;
	// 银瓜子领取信息获取
	if (_useropt.conf & 0x02) {
		_APIv1SilverCurrentTask();
		_APIv1SilverCaptcha();
	}
	else {
		_heartopt.silvercount = -1;
	}
	// 签到
	GetSign();
	// 每日礼物
	_APIv2GiftDaily();
	// 兑换硬币
	if (_useropt.conf & 0x01) {
		PostSilver2Coin();
		Sleep(1000);
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
		if (_heartopt.freegift) {
			_APIv2GetHeartGift();
		}
		PostOnlineHeart();
	}
	if (_heartopt.silvercount != -1) {
		_heartopt.silvercount--;
		if (_heartopt.silvercount == 0) {
			// _APIv1SilverAward();
			_APIv1SilverCurrentTask();
		}
	}
	return 0;
}
int CBilibiliUserInfo::ActSmallTV(int rrid, int loid)
{
	BILIRET bret;
	int count;
	if (_useropt.conf & 0x40) {
		// 产生访问记录
		_APIv1RoomEntry(rrid);
		// 网页端最多尝试三次
		count = 2;
		bret = this->_APIv3SmallTV(rrid, loid);
		while ((bret != BILIRET::NOFAULT) &&count) {
			Sleep(1000);
			bret = this->_APIv3SmallTV(rrid, loid);
			count--;
		}
	}

	return 0;
}

int CBilibiliUserInfo::ActStorm(int roid, long long cid) {
	// 风暴只领取一次 不管成功与否
	if (_useropt.conf & 0x10) {
		// 网页端API
		_APIv1StormJoin(roid, cid, "", "");
		return 0;
	}
	if (_useropt.conf & 0x20) {
		// 调用客户端API领取
		_APIAndv1StormJoin(cid);
		return 0;
	}
	return 0;
}

int CBilibiliUserInfo::ActYunYing(std::string eventname, int rid, int raffleId)
{
	BILIRET ret;
	int count;
	if (_useropt.conf & 0x08) {
		// 手机端只操作一次
		ret = this->_APIAndv1YunYing(eventname, rid, raffleId);
	}
	if (_useropt.conf & 0x04) {
		// 网页端最多尝试三次
		count = 2;
		ret = this->_APIv1YunYing(rid, raffleId);
		while ((ret!= BILIRET::NOFAULT)&&count) {
			Sleep(1000);
			ret = this->_APIv1YunYing(rid, raffleId);
			count--;
		}
	}

	return 0;
}

// 获取LIVE的一些Cookie
BILIRET CBilibiliUserInfo::GETLoginJct(int area) const
{
	printf("[User] Get initial cookie... \n");
	int ret;
	if (area == 1)
	{
		_httppackweb->url = DEF_URLLogin;
		_httppackweb->ClearHeader();
		ret = toollib::HttpGetEx(curlweb, _httppackweb);
		_httppackweb->url = "https://api.bilibili.com/nav?callback=jQuery1491890066391&type=jsonp";
		_httppackweb->ClearHeader();
		ret = toollib::HttpGetEx(curlweb, _httppackweb);
	}
	else {
		_httppackweb->url = "http://live.bilibili.com/23058";
		_httppackweb->ClearHeader();
		ret = toollib::HttpGetEx(curlweb, _httppackweb);
	}
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	return BILIRET::NOFAULT;
}

// 获取验证码图片
BILIRET CBilibiliUserInfo::GETPicCaptcha() const {
	printf("[User] Get captcha... \n");
	_httppackweb->url = DEF_URLLoginCaptcha + std::to_string((long long)_tool.GetTimeStamp());
	_httppackweb->ClearHeader();
	_httppackweb->AddHeaderManual("Referer: https://passport.bilibili.com/login/");
	int ret = toollib::HttpGetEx(curlweb, _httppackweb);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	FILE *ValidateJPG;
	ret = fopen_s(&ValidateJPG, "Captcha.jpg", "wb");
	if (ret) {
		return BILIRET::FILE_ERROR;
	}
	fwrite((char*)_httppackweb->strrecdata, 1, _httppackweb->i_lenrecdata, ValidateJPG);
	fclose(ValidateJPG);
	
	return BILIRET::NOFAULT;
}

// 获取RSA公钥加密密码
BILIRET CBilibiliUserInfo::GETEncodePsd(std::string &psd) const {
	printf("[User] Get RSA key... \n");
	_httppackweb->url = DEF_URLLoginGweKey;
	_httppackweb->url += "&_=" + std::to_string((long long)_tool.GetTimeStamp());
	_httppackweb->ClearHeader();
	_httppackweb->AddHeaderManual("Accept: application/json, text/javascript, */*; q=0.01");
	_httppackweb->AddHeaderManual("Referer: https://passport.bilibili.com/ajax/miniLogin/minilogin");
	_httppackweb->AddHeaderManual("X-Requested-With: XMLHttpRequest");
	int ret = toollib::HttpGetEx(curlweb, _httppackweb);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	rapidjson::Document doc;
	doc.Parse(_httppackweb->strrecdata);
	if (!doc.IsObject() || !doc.HasMember("hash") || !doc["hash"].IsString()
		|| !doc.HasMember("key") || !doc["key"].IsString()) {
		return BILIRET::JSON_ERROR;
	}
	std::string tmp_strhash = doc["hash"].GetString();
	std::string tmp_strpubkey = doc["key"].GetString();
	tmp_strhash += psd;
	ret = Encrypt_RSA_KeyBuff((char*)tmp_strpubkey.c_str(), tmp_strhash, psd);
	if (!ret) {
		return BILIRET::OPENSSL_ERROR;
	}
	return BILIRET::NOFAULT;
}

// 移动端登录接口
BILIRET CBilibiliUserInfo::POSTLogin(std::string username, std::string password, std::string strver) const {
	printf("[User] Logging in by mobile api... \n");
	_httppackweb->url = DEF_URLLoginMini; 
	std::string postdata = _strcoding.UrlUTF8((char*)password.c_str());
	_httppackweb->strsenddata = "userid=" + username + "&pwd=" + postdata + "&captcha=" + strver  + "&keep=1&cType=2&vcType=1";
	_httppackweb->ClearHeader();
	_httppackweb->AddHeaderManual("Accept: application/json, text/javascript, */*; q=0.01");
	_httppackweb->AddHeaderManual("Content-Type: application/x-www-form-urlencoded; charset=UTF-8");
	_httppackweb->AddHeaderManual("Origin: https://passport.bilibili.com");
	_httppackweb->AddHeaderManual("Referer: https://passport.bilibili.com/ajax/miniLogin/minilogin");
	_httppackweb->AddHeaderManual("X-Requested-With:XMLHttpRequest");
	int ret = toollib::HttpPostEx(curlweb, _httppackweb);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	rapidjson::Document doc;
	doc.Parse(_httppackweb->strrecdata);
	if (!doc.IsObject() || !doc.HasMember("status") || !doc["status"].IsBool()) {
		return BILIRET::JSON_ERROR;
	}
	if (doc["status"].GetBool()) {
		return BILIRET::NOFAULT;
	}
	// 登录失败
	if (!doc.HasMember("message") || !doc["message"]["code"].IsInt()) {
		return BILIRET::JSON_ERROR;
	}

	ret = doc["message"]["code"].GetInt();
	if (ret == -626)
		return BILIRET::LOGIN_NAMEWRONG;
	if (ret == -627)
		return BILIRET::LOGIN_PASSWORDWRONG;
	if (ret == -105)
		return BILIRET::LOGIN_NEEDVERIFY;

	return BILIRET::JSON_ERROR;
}

// 获取直播站主要信息
BILIRET CBilibiliUserInfo::GetUserInfoLive(BILIUSEROPT &pinfo) const {
	printf("[User] Get user live info... \n");
	_httppackweb->url = _urlapi + "/User/getUserInfo";
	_httppackweb->ClearHeader();
	int ret = toollib::HttpGetEx(curlweb, _httppackweb);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	rapidjson::Document doc;
	doc.Parse(_httppackweb->strrecdata);
	if (!doc.IsObject()) {
		return BILIRET::JSON_ERROR;
	}
	bool isvalid = false;
	if (doc.HasMember("code") && doc["code"].IsInt() && !doc["code"].GetInt()) {
		isvalid = true;
	}
	if (doc.HasMember("code") && doc["code"].IsString() && strcmp("REPONSE_OK", doc["code"].GetString()) == 0) {
		isvalid = true;
	}
	if (!isvalid) {
		return BILIRET::JSON_ERROR;
	}
	
	return BILIRET::NOFAULT;
}

// 直播经验心跳Web
BILIRET CBilibiliUserInfo::PostOnlineHeart() const {
	// -403 非法心跳
	_httppackweb->url = _urlapi + "/User/userOnlineHeart";
	_httppackweb->strsenddata = "";
	_httppackweb->ClearHeader();
	_httppackweb->AddHeaderManual("Accept:application/json, text/javascript, */*; q=0.01");
	_httppackweb->AddHeaderManual(URL_DEFAULT_ORIGIN);
	_httppackweb->AddHeaderManual(URL_DEFAULT_REFERER);
	int ret = toollib::HttpPostEx(curlweb, _httppackweb);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	rapidjson::Document doc;
	doc.Parse(_httppackweb->strrecdata);
	if (!doc.IsObject() || !doc.HasMember("code") || !doc["code"].IsInt()) {
		return BILIRET::JSON_ERROR;
	}
	ret = doc["code"].GetInt();
	if (ret) {
		printf("%s[User%d] OnlineHeart: %s \n", _tool.GetTimeString().c_str(), _useropt.fileid, _httppackweb->strrecdata);
		return BILIRET::JSON_ERROR;
	}
	printf("%s[User%d] OnlineHeart: OK \n", _tool.GetTimeString().c_str(), _useropt.fileid);

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
	doc.Parse(_httppackweb->strrecdata);
	if (!doc.IsObject() || !doc.HasMember("code") || !doc["code"].IsInt()) {
		return BILIRET::JSON_ERROR;
	}
	ret = doc["code"].GetInt();
	if (ret == -500) {
		printf("%s[User%d] Sign: Signed. \n", _tool.GetTimeString().c_str(), _useropt.fileid);
		return BILIRET::NOFAULT;
	}
	if (ret) {
		printf("%s[User%d] Sign: %s \n", _tool.GetTimeString().c_str(), _useropt.fileid, _httppackweb->strrecdata);
		return BILIRET::JSON_ERROR;
	}
	printf("%s[User%d] Sign: Success. \n", _tool.GetTimeString().c_str(), _useropt.fileid);

	return BILIRET::NOFAULT;
}

// 银瓜子换硬币
BILIRET CBilibiliUserInfo::PostSilver2Coin() const {
	_httppackweb->url = _urlapi + "/exchange/silver2coin";
	_httppackweb->ClearHeader();
	_httppackweb->AddHeaderManual("Accept: application/json, text/javascript, */*; q=0.01");
	_httppackweb->AddHeaderManual("Content-Type: application/x-www-form-urlencoded; charset=UTF-8");
	int ret = toollib::HttpGetEx(curlweb, _httppackweb);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	// 0兑换成功 -403已兑换
	rapidjson::Document doc;
	doc.Parse(_httppackweb->strrecdata);
	if (!doc.IsObject() || !doc.HasMember("code") || !doc["code"].IsInt()) {
		return BILIRET::JSON_ERROR;
	}
	ret = doc["code"].GetInt();
	std::string msg = _strcoding.UTF_8ToString(doc["msg"].GetString());
	printf("%s[User%d] Silver2Coin: %s. \n", _tool.GetTimeString().c_str(), _useropt.fileid, msg.c_str());

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
	doc.Parse(_httppackweb->strrecdata);
	if (!doc.IsObject() || !doc.HasMember("code") || !doc["code"].IsInt() || doc["code"].GetInt()) {
		return BILIRET::JSON_ERROR;
	}
	double money = doc["data"]["money"].GetDouble();
	printf("%s[User%d] Current Coin: %.1lf. \n", _tool.GetTimeString().c_str(), _useropt.fileid, money);

	return BILIRET::NOFAULT;
}

// 发送弹幕
BILIRET  CBilibiliUserInfo::SendDanmuku(int roomID, std::string msg) const {
	_httppackweb->url = _urlapi + "/msg/send";
	std::string postdata;
	postdata = "color=16777215&fontsize=25&mode=1&msg=" + _strcoding.UrlUTF8(msg.c_str()) +
		"&rnd=" + std::to_string(_tool.GetTimeStamp()) + "&roomid=" + std::to_string(roomID);
	_httppackweb->strsenddata = postdata;
	_httppackweb->ClearHeader();
	_httppackweb->AddHeaderManual("Accept: application/json, text/javascript, */*; q=0.01");
	_httppackweb->AddHeaderManual("Content-Type: application/x-www-form-urlencoded; charset=UTF-8");
	_httppackweb->AddHeaderManual(URL_DEFAULT_ORIGIN);
	std::string strreffer(URL_DEFAULT_REFERERBASE);
	strreffer += std::to_string(roomID);
	_httppackweb->AddHeaderManual(strreffer.c_str());
	int ret = toollib::HttpPostEx(curlweb, _httppackweb);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	rapidjson::Document doc;
	doc.Parse(_httppackweb->strrecdata);
	if (!doc.IsObject() || !doc.HasMember("code") 
		|| !doc["code"].IsInt() || doc["code"].GetInt()) {
		return BILIRET::JSON_ERROR;
	}

	printf("%s[User%d] SendDanmuku Success \n", _tool.GetTimeString().c_str(), _useropt.fileid);

	return BILIRET::NOFAULT;
}

// 获取播主账户ID（亦作 RUID）
BILIRET CBilibiliUserInfo::_APIv1MasterID(int liveRoomID, int &uid) const {
	_httppackweb->url = _urlapi + "/room/v1/Room/getRoomInfoMain?roomid=" + std::to_string(liveRoomID);
	_httppackweb->ClearHeader();
	int ret = toollib::HttpGetEx(curlweb, _httppackweb);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	rapidjson::Document doc;
	doc.Parse(_httppackweb->strrecdata);
	if (!doc.IsObject() || !doc["data"].HasMember("MASTERID")) {
		return BILIRET::JSON_ERROR;
	}
	uid = doc["data"]["MASTERID"].GetInt();

	return BILIRET::NOFAULT;
}

// 直播经验心跳日志
BILIRET CBilibiliUserInfo::_APIv1HeartBeat() const {
	int ret;
	std::string thetime = std::to_string(_tool.GetTimeStamp());
	_httppackweb->url = _urlapi + "/relation/v1/feed/heartBeat?_=" + thetime + "378";
	_httppackweb->ClearHeader();
	_httppackweb->AddHeaderManual("Accept:*/*");
	_httppackweb->AddHeaderManual(URL_DEFAULT_REFERER);
	ret = toollib::HttpGetEx(curlweb, _httppackweb);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	rapidjson::Document doc;
	doc.Parse(_httppackweb->strrecdata);
	if (!doc.IsObject() || !doc.HasMember("code") || !doc["code"].IsInt()) {
		return BILIRET::JSON_ERROR;
	}
	ret = doc["code"].GetInt();
	if (ret) {
		printf("%s[User%d] HeartBeat: %d \n", _tool.GetTimeString().c_str(), _useropt.fileid, ret);
	}
	else {
		printf("%s[User%d] HeartBeat: OK \n", _tool.GetTimeString().c_str(), _useropt.fileid);
	}

	return BILIRET::NOFAULT;
}

// 获取指定勋章排名
BILIRET CBilibiliUserInfo::_APIv1MedalRankList(int roomid, int uid, int &rank) const {
	_httppackweb->url = _urlapi + "/rankdb/v1/RoomRank/webMedalRank?";
	char datastr[100] = "";
	sprintf_s(datastr, sizeof(datastr), "roomid=%d&ruid=%d", roomid, uid);
	_httppackweb->url += datastr;
	_httppackweb->ClearHeader();
	_httppackweb->AddHeaderManual("Accept: application/json, text/javascript, */*; q=0.01");
	int ret = toollib::HttpGetEx(curlweb, _httppackweb);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	rapidjson::Document doc;
	doc.Parse(_httppackweb->strrecdata);
	if (!doc.IsObject() || !doc.HasMember("code")|| !doc["code"].IsInt()
		|| doc["code"].GetInt() || !doc.HasMember("data")
		|| !doc["data"].HasMember("own") || !doc["data"]["own"].HasMember("rank")) {
		return BILIRET::JSON_ERROR;
	}

	rank = -1;
	rank = doc["data"]["own"]["rank"].GetInt();

	return BILIRET::NOFAULT;
}

// 获取节奏风暴验证码
BILIRET CBilibiliUserInfo::_APIv1Captcha(std::string &img, std::string &token) const {
	int ret;
	_httppackweb->url = _urlapi + "/captcha/v1/Captcha/create?_=" + std::to_string(_tool.GetTimeStamp()) + "324&width=112&height=32";
	_httppackweb->ClearHeader();
	_httppackweb->AddHeaderManual(URL_DEFAULT_ORIGIN);
	ret = toollib::HttpGetEx(curlweb, _httppackweb);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	rapidjson::Document doc;
	doc.Parse(_httppackweb->strrecdata);
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
	char datastr[400] = "";
	sprintf_s(datastr, sizeof(datastr), "id=%I64d&color=16777215&captcha_token=%s&captcha_phrase=%s&roomid=%d&csrf_token=%s&visit_id=%s",
		cid, token.c_str(), code.c_str(), roomID, _useropt.tokenjct.c_str(), _useropt.visitid.c_str());
	_httppackweb->strsenddata = datastr;
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
	doc.Parse(_httppackweb->strrecdata);
	if (!doc.IsObject() || !doc.HasMember("code") || !doc["code"].IsInt()) {
		return BILIRET::JSON_ERROR;
	}
	ret = doc["code"].GetInt();
	std::string msg;
	// 429需要验证码 400未抽中或已过期
	if (ret) {
		msg = _strcoding.UTF_8ToString(doc["msg"].GetString());
		printf("%s[User%d] Storm %d %s \n", _tool.GetTimeString().c_str(), _useropt.fileid, ret, msg.c_str());
		// 检查是否被封禁
		if (ret == 400) {
			if (msg.find("访问被拒绝") != -1) {
				this->SetBanned();
				return BILIRET::NOFAULT;
			}
		}
		return BILIRET::JOINEVENT_FAILED;
	}
	// 抽中的返回值为0
	msg = _strcoding.UTF_8ToString(doc["data"]["mobile_content"].GetString());
	printf("%s[User%d] Storm %s \n", _tool.GetTimeString().c_str(), _useropt.fileid, msg.c_str());

	return BILIRET::NOFAULT;
}

// 运营活动抽奖
BILIRET CBilibiliUserInfo::_APIv1YunYing(int rid, int raffleId) {
	int ret;
	_httppackweb->url = _urlapi + "/activity/v1/Raffle/join";
	std::string postData;
	postData = "roomid=" + std::to_string(rid) + "&raffleId=" + std::to_string(raffleId);
	_httppackweb->strsenddata = postData;
	_httppackweb->ClearHeader();
	_httppackweb->AddHeaderManual("Accept: application/json, text/javascript, */*; q=0.01");
	_httppackweb->AddHeaderManual("Content-Type: application/x-www-form-urlencoded; charset=UTF-8");
	_httppackweb->AddHeaderManual(URL_DEFAULT_ORIGIN);
	_httppackweb->AddHeaderManual(URL_DEFAULT_REFERER);
	ret = toollib::HttpPostEx(curlweb, _httppackweb);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}
#ifdef _DEBUG
	printf("%s[User%d] ActYunYing %s \n", _tool.GetTimeString().c_str(), _useropt.fileid, _httppackweb->strrecdata);
#endif

	rapidjson::Document doc;
	doc.Parse(_httppackweb->strrecdata);
	if (!doc.IsObject() || !doc.HasMember("code") || !doc["code"].IsInt()) {
		return BILIRET::JSON_ERROR;
	}

	// 0成功 -400已领取或被封禁 -500系统繁忙
	int icode = doc["code"].GetInt();
	std::string tmpstr;
	if (doc.HasMember("message") && doc["message"].IsString())
		tmpstr = _strcoding.UTF_8ToString(doc["message"].GetString());
	printf("%s[User%d] Raffle %s \n", _tool.GetTimeString().c_str(), _useropt.fileid, tmpstr.c_str());
	// 检查是否被封禁
	if (icode == 400) {
		if (tmpstr.find("访问被拒绝") != -1) {
			this->SetBanned();
			return BILIRET::NOFAULT;
		}
	}
	if (icode) {
		return BILIRET::JOINEVENT_FAILED;
	}
	return BILIRET::NOFAULT;
}

// 银瓜子验证码
BILIRET CBilibiliUserInfo::_APIv1SilverCaptcha() const {
	_httppackweb->url = _urlapi + "/lottery/v1/SilverBox/getCaptcha?ts=" + std::to_string((long long)_tool.GetTimeStamp()) + "537";
	_httppackweb->ClearHeader();
	_httppackweb->AddHeaderManual("Accept:application/json, text/plain, */*");
	_httppackweb->AddHeaderManual(URL_DEFAULT_ORIGIN);
	_httppackweb->AddHeaderManual(URL_DEFAULT_REFERER);
	int ret = toollib::HttpGetEx(curlweb, _httppackweb);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	rapidjson::Document doc;
	doc.Parse(_httppackweb->strrecdata);
	if (!doc.IsObject() || !doc.HasMember("code") || !doc["code"].IsInt()) {
		return BILIRET::JSON_ERROR;
	}
	if (doc["code"].GetInt()) {
		printf("%s[User%d] ERROR: Get captcha. \n", _tool.GetTimeString().c_str(), _useropt.fileid);
		return BILIRET::JSON_ERROR;
	}
	// 获取验证码BASE64字符串
	std::string img;
	img = doc["data"]["img"].GetString();
	ret = img.find("base64") + 7;
	img = img.substr(ret, img.length() - ret);
	// 打开文档
	FILE *ValidateJPG;
	ret = fopen_s(&ValidateJPG, "CaptchaSilver.jpg", "wb");
	if (ret) {
		return BILIRET::FILE_ERROR;
	}
	// 解码并写入文件
	int len = img.length();
	BYTE *imgstr = new BYTE[len];
	Decode_Base64(img, imgstr, (unsigned int *)&len);
	fwrite((char*)imgstr, 1, len, ValidateJPG);
	fclose(ValidateJPG);
	delete[]imgstr;

	return BILIRET::NOFAULT;
}

//获取当前宝箱领取情况
BILIRET CBilibiliUserInfo::_APIv1SilverCurrentTask() {
	_httppackweb->url = _urlapi + "/lottery/v1/SilverBox/getCurrentTask";
	_httppackweb->ClearHeader();
	_httppackweb->AddHeaderManual("Accept:application/json, text/plain, */*");
	_httppackweb->AddHeaderManual(URL_DEFAULT_ORIGIN);
	_httppackweb->AddHeaderManual(URL_DEFAULT_REFERER);
	int ret = toollib::HttpGetEx(curlweb, _httppackweb);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}
	// 领完为-10017

	rapidjson::Document doc;
	doc.Parse(_httppackweb->strrecdata);
	if (!doc.IsObject() || !doc.HasMember("code")) {
		return BILIRET::JSON_ERROR;
	}
	ret = doc["code"].GetInt();
	if (ret != 0) {
		printf("%s[User%d] GetFreeSilver: No new task. \n", _tool.GetTimeString().c_str(), _useropt.fileid);
		_heartopt.silvercount = -1;
		return BILIRET::NOFAULT;
	}
	_heartopt.silvercount = doc["data"]["minute"].GetInt();
	_useropt.silver_minute = doc["data"]["minute"].GetInt();
	_useropt.silver_count = doc["data"]["silver"].GetInt();
	_useropt.silver_start = doc["data"]["time_start"].GetInt();
	_useropt.silver_end = doc["data"]["time_end"].GetInt();
	printf("%s[User%d] GetFreeSilver: Wait:%d Amount:%d \n", _tool.GetTimeString().c_str(), _useropt.fileid, _useropt.silver_minute, _useropt.silver_count);

	return BILIRET::NOFAULT;
}

// 领取银瓜子 失效
BILIRET CBilibiliUserInfo::_APIv1SilverAward() {
	int ret;
	int iCaptcha = -1;
	_httppackweb->url = _urlapi + "/lottery/v1/SilverBox/getAward?";
	char datastr[100] = "";
	sprintf_s(datastr, sizeof(datastr), "time_start=%I64d&end_time=%I64d&captcha=%d", _useropt.silver_start, _useropt.silver_end, iCaptcha);
	_httppackweb->url += datastr;
	_httppackweb->ClearHeader();
	_httppackweb->AddHeaderManual(URL_DEFAULT_REFERER);
	ret = toollib::HttpGetEx(curlweb, _httppackweb);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}
	
	rapidjson::Document doc;
	doc.Parse(_httppackweb->strrecdata);
	if (!doc.IsObject() || !doc.HasMember("code")) {
		return BILIRET::JSON_ERROR;
	}
	ret = doc["code"].GetInt();
	if (ret == -99) {
		printf("%s[User%d] GetFreeSilver: Overdue. \n", _tool.GetTimeString().c_str(), _useropt.fileid);
		_heartopt.silvercount = 0;
		return BILIRET::NOFAULT;
	}
	if (strcmp(doc["msg"].GetString(), "ok")) {
		return BILIRET::JSON_ERROR;
	}

	if (doc["data"]["isEnd"].GetInt()) {
		printf("%s[User%d] GetFreeSilver: Finish. \n", _tool.GetTimeString().c_str(), _useropt.fileid);
		_heartopt.silvercount = -1;
	}
	else {
		printf("%s[User%d] GetFreeSilver: Get Success. \n", _tool.GetTimeString().c_str(), _useropt.fileid);
		_heartopt.silvercount = 0;
	}
	return BILIRET::NOFAULT;
}

// 银瓜子换硬币
BILIRET CBilibiliUserInfo::_APIv1Silver2Coin() const {
	_httppackweb->url = _urlapi + "/pay/v1/Exchange/silver2coin";
	_httppackweb->strsenddata = "platform=pc&csrf_token=" + _useropt.tokenjct;
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
	doc.Parse(_httppackweb->strrecdata);
	if (!doc.IsObject() || !doc.HasMember("code") || !doc["code"].IsInt()) {
		return BILIRET::JSON_ERROR;
	}
	ret = doc["code"].GetInt();
	std::string msg = _strcoding.UTF_8ToString(doc["msg"].GetString());
	printf("%s[User%d] v1Silver2Coin: %s. \n", _tool.GetTimeString().c_str(), _useropt.fileid, msg.c_str());

	return BILIRET::NOFAULT;
}

// 查询扭蛋币数量
BILIRET CBilibiliUserInfo::_APIv1CapsuleCheck() const {
	_httppackweb->url = _urlapi + "/lottery/v1/Capsule/getUserInfo";
	_httppackweb->ClearHeader();
	_httppackweb->AddHeaderManual("Accept: application/json, text/javascript, */*; q=0.01");
	_httppackweb->AddHeaderManual(URL_DEFAULT_ORIGIN);
	_httppackweb->AddHeaderManual(URL_DEFAULT_REFERER);
	int ret = toollib::HttpGetEx(curlweb, _httppackweb);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	rapidjson::Document doc;
	doc.Parse(_httppackweb->strrecdata);
	if (!doc.IsObject() || !doc.HasMember("code") 
		|| !doc["code"].IsInt() || doc["code"].GetInt()
		|| !doc.HasMember("data") || !doc["data"].IsObject()) {
		return BILIRET::JSON_ERROR;
	}
	printf("  Capsule info:\n");
	if (doc["data"].HasMember("normal") && doc["data"]["normal"].IsObject() 
		&& doc["data"]["normal"].HasMember("coin") && doc["data"]["normal"]["coin"].IsInt()) {
		printf("Normal: %d\n", doc["data"]["normal"]["coin"].GetInt());
	}
	if (doc["data"].HasMember("colorful") && doc["data"]["colorful"].IsObject()
		&& doc["data"]["colorful"].HasMember("coin") && doc["data"]["colorful"]["coin"].IsInt()) {
		printf("Colorful: %d\n", doc["data"]["colorful"]["coin"].GetInt());
	}

	return BILIRET::NOFAULT;
}

BILIRET CBilibiliUserInfo::_APIv1RoomEntry(int room) const {
	int ret;
	_httppackweb->url = _urlapi + "/room/v1/Room/room_entry_action";
	char datastr[400] = "";
	sprintf_s(datastr, sizeof(datastr), "room_id=%d&platform=pc&csrf_token=%s&visit_id=%s",
		room, _useropt.tokenjct.c_str(), _useropt.visitid.c_str());
	_httppackweb->strsenddata = datastr;
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

BILIRET CBilibiliUserInfo::APIv1YunYingGift(int rid) const {
	int ret;
	_httppackweb->url = _urlapi + "/activity/v1/Common/getReceiveGift?roomid=" + std::to_string(rid);
	_httppackweb->ClearHeader();
	_httppackweb->AddHeaderManual("Accept: application/json, text/javascript, */*; q=0.01");
	_httppackweb->AddHeaderManual("Content-Type: application/x-www-form-urlencoded; charset=UTF-8");
	_httppackweb->AddHeaderManual(URL_DEFAULT_ORIGIN);
	_httppackweb->AddHeaderManual(URL_DEFAULT_REFERER);
	ret = toollib::HttpGetEx(curlweb, _httppackweb);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}
#ifdef _DEBUG
	printf("%s[User%d] ActYunYing Daily Gift %s \n", _tool.GetTimeString().c_str(), _useropt.fileid, _httppackweb->strrecdata);
#endif

	rapidjson::Document doc;
	doc.Parse(_httppackweb->strrecdata);
	if (!doc.IsObject() || !doc.HasMember("code") || !doc["code"].IsInt()
		|| !doc.HasMember("message") || !doc["message"].IsString()) {
		return BILIRET::JSON_ERROR;
	}

	std::string tmpstr;
	int icode = doc["code"].GetInt();
	tmpstr = doc["message"].GetString();
	tmpstr = _strcoding.UTF_8ToString(tmpstr.c_str());
	printf("%s[User%d] YunYing Daily Gift %s \n", _tool.GetTimeString().c_str(), _useropt.fileid, tmpstr.c_str());

	return BILIRET::NOFAULT;
}

BILIRET CBilibiliUserInfo::APIv1LotteryJoin(BILI_LOTTERYDATA &pdata) {
	int ret;
	_httppackweb->url = _urlapi + "/lottery/v1/lottery/join";
	char datastr[400] = "";
	sprintf_s(datastr, sizeof(datastr), "roomid=%d&id=%d&type=%s&csrf_token=%s&visit_id=%s",
		pdata.rrid, pdata.loid, pdata.type.c_str(), _useropt.tokenjct.c_str(), _useropt.visitid.c_str());
	_httppackweb->strsenddata = datastr;
	_httppackweb->ClearHeader();
	_httppackweb->AddHeaderManual("Accept: application/json, text/plain, */*");
	_httppackweb->AddHeaderManual("Content-Type: application/x-www-form-urlencoded");
	_httppackweb->AddHeaderManual(URL_DEFAULT_ORIGIN);
	std::string strreffer(URL_DEFAULT_REFERERBASE);
	strreffer += std::to_string(pdata.rrid);
	_httppackweb->AddHeaderManual(strreffer.c_str());
	ret = toollib::HttpPostEx(curlweb, _httppackweb);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	rapidjson::Document doc;
	doc.Parse(_httppackweb->strrecdata);
	if (!doc.IsObject() || !doc.HasMember("code") || !doc["code"].IsInt()) {
		return BILIRET::JSON_ERROR;
	}
	std::string tmpstr;
	if (doc["code"].GetInt()) {
		tmpstr = doc["message"].GetString();
	}
	else {
		tmpstr = doc["data"]["message"].GetString();
	}
	tmpstr = _strcoding.UTF_8ToString(tmpstr.c_str());
	printf("%s[User%d] Lottery %s \n", _tool.GetTimeString().c_str(), _useropt.fileid, tmpstr.c_str());

	return BILIRET::NOFAULT;
}

// 免费礼物领取状态查询
BILIRET CBilibiliUserInfo::_APIv2CheckHeartGift() {
	int ret;
	_httppackweb->url = _urlapi + "/gift/v2/live/heart_gift_status?roomid=23058&area_v2_id=32";
	_httppackweb->ClearHeader();
	_httppackweb->AddHeaderManual("Accept:application/json, text/javascript, */*; q=0.01");
	_httppackweb->AddHeaderManual(URL_DEFAULT_ORIGIN);
	_httppackweb->AddHeaderManual(URL_DEFAULT_REFERER);
	ret = toollib::HttpGetEx(curlweb, _httppackweb);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	rapidjson::Document doc;
	doc.Parse(_httppackweb->strrecdata);
	if (!doc.IsObject() || !doc.HasMember("code") || !doc["code"].IsInt() || doc["code"].GetInt()) {
		printf("%s[User%d] FreeGiftInfo: %s \n", _tool.GetTimeString().c_str(), _useropt.fileid, _httppackweb->strrecdata);
		return BILIRET::JSON_ERROR;
	}
	if (doc["data"]["heart_status"].GetInt()) {
		_heartopt.freegift = 3;
		printf("%s[User%d] FreeGift: Start getting. \n", _tool.GetTimeString().c_str(), _useropt.fileid);
	}
	else {
		printf("%s[User%d] FreeGift: No available gift. \n", _tool.GetTimeString().c_str(), _useropt.fileid);
	}

	return BILIRET::NOFAULT;
}

// 免费礼物领取心跳包
BILIRET CBilibiliUserInfo::_APIv2GetHeartGift() {
	int ret;
	_httppackweb->url = _urlapi + "/gift/v2/live/heart_gift_receive?roomid=23058&area_v2_id=32";
	_httppackweb->ClearHeader();
	_httppackweb->AddHeaderManual("Accept:application/json, text/javascript, */*; q=0.01");
	_httppackweb->AddHeaderManual(URL_DEFAULT_ORIGIN);
	_httppackweb->AddHeaderManual(URL_DEFAULT_REFERER);
	ret = toollib::HttpGetEx(curlweb, _httppackweb);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	rapidjson::Document doc;
	doc.Parse(_httppackweb->strrecdata);
	_heartopt.freegift--;
	if (!doc.IsObject() || !doc.HasMember("code") || !doc["code"].IsInt()) {
		return BILIRET::JSON_ERROR;
	}
	if (doc["code"].GetInt()) {
		printf("%s[User%d] EventRoomHeart ERROR: %s \n", _tool.GetTimeString().c_str(), _useropt.fileid, _httppackweb->strrecdata);
		return BILIRET::JSON_ERROR;
	}
	if (!doc.HasMember("data") || !doc["data"].IsObject() || !doc["data"].HasMember("gift_list")) {
		return BILIRET::JSON_ERROR;
	}
	if (doc["data"]["gift_list"].IsNull()) {
		printf("%s[User%d] EventRoomHeart: Heart success. \n", _tool.GetTimeString().c_str(), _useropt.fileid);
		_heartopt.freegift = 3;
	}
	else if (doc["data"]["gift_list"].IsObject()) {
		printf("%s[User%d] EventRoomHeart: Receive one freegift. \n", _tool.GetTimeString().c_str(), _useropt.fileid);
		_heartopt.freegift = 3;
	}
	if (doc["data"].HasMember("heart_status") || doc["data"]["heart_status"].IsInt()) {
		return BILIRET::JSON_ERROR;
	}
	if (doc["data"]["heart_status"].GetInt() == 0) {
		printf("%s[User%d] EventRoomHeart: Done. Heart stop. \n", _tool.GetTimeString().c_str(), _useropt.fileid);
		_heartopt.freegift = 0;
	}

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
	doc.Parse(_httppackweb->strrecdata);
	if (!doc.IsObject() || !doc.HasMember("code") || !doc["code"].IsInt() || doc["code"].GetInt()) {
		printf("%s[User%d] Get bag info failed. \n", _tool.GetTimeString().c_str(), _useropt.fileid);
		return BILIRET::JSON_ERROR;
	}
	printf("  Current bag info: \n");
	int curtime, expiretime;
	std::string giftname;
	curtime = doc["data"]["time"].GetInt();
	rapidjson::Value &datalist = doc["data"]["list"];
	unsigned int i;
	for (i = 0; i < datalist.Size(); i++) {
		giftname = _strcoding.UTF_8ToString(datalist[i]["gift_name"].GetString());
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
	doc.Parse(_httppackweb->strrecdata);
	if (!doc.IsObject() || !doc.HasMember("code") || !doc["code"].IsInt() || doc["code"].GetInt()) {
		printf("%s[User%d] Receive daily gift failed. \n", _tool.GetTimeString().c_str(), _useropt.fileid);
		return BILIRET::JSON_ERROR;
	}
	ret = doc["data"]["bag_status"].GetInt();
	if (ret == 1) {
		printf("%s[User%d] 每日礼物领取成功 \n", _tool.GetTimeString().c_str(), _useropt.fileid);
	}
	else if (ret == 2) {
		printf("%s[User%d] 每日礼物已领取 \n", _tool.GetTimeString().c_str(), _useropt.fileid);
	}
	else {
		printf("%s[User%d] Receive daily gift Code: %d \n", _tool.GetTimeString().c_str(), _useropt.fileid, ret);
	}

	return BILIRET::NOFAULT;
}

// 赠送礼物
BILIRET CBilibiliUserInfo::APIv2SendGift(int giftID, int roomID, int num, bool coinType, int bagID) const {
	_httppackweb->url = _urlapi + "/gift/v2/live/bag_send";
	char datastr[300] = "";
	int uid;
	_APIv1MasterID(roomID, uid);
	sprintf_s(datastr, sizeof(datastr), "uid=%s&giftId=%d&ruid=%d&gift_num=%d&bag_id=%d&platform=pc&biz_code=live&biz_id=%d&rnd=%I64d&storm_beat_id=0&metadata=&price=0&csrf_token=%s",
		_useropt.uid.c_str(), giftID, uid, num, bagID, roomID, _tool.GetTimeStamp() - 10, _useropt.tokenlive.c_str());
	_httppackweb->strsenddata = datastr;
	_httppackweb->ClearHeader();
	int ret = toollib::HttpPostEx(curlweb, _httppackweb);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}
	printf("%s[User] SendGift: %d: %s\n", _tool.GetTimeString().c_str(), _useropt.fileid, _httppackweb->strrecdata);

	rapidjson::Document doc;
	doc.Parse(_httppackweb->strrecdata);
	if (!doc.IsObject() || !doc.HasMember("msg") 
		|| strcmp(doc["msg"].GetString(), "ok")) {
		return BILIRET::JSON_ERROR;
	}

	return BILIRET::NOFAULT;
}

// 通告礼物抽奖
BILIRET CBilibiliUserInfo::_APIv3SmallTV(int rrid, int loid)
{
	int ret;
	char datastr[400] = "";
	_httppackweb->url = _urlapi + "/gift/v3/smalltv/join";
	sprintf_s(datastr, sizeof(datastr), "roomid=%d&raffleId=%d&type=Gift&csrf_token=%s&visit_id=%s",
		rrid, loid, _useropt.tokenjct.c_str(), _useropt.visitid.c_str());
	_httppackweb->strsenddata = datastr;
	_httppackweb->ClearHeader();
	_httppackweb->AddHeaderManual("Accept: application/json, text/plain, */*");
	_httppackweb->AddHeaderManual("Content-Type: application/x-www-form-urlencoded");
	_httppackweb->AddHeaderManual(URL_DEFAULT_ORIGIN);
	sprintf_s(datastr, sizeof(datastr), "%s%d", URL_DEFAULT_REFERERBASE, rrid);
	_httppackweb->AddHeaderManual(datastr);
	ret = toollib::HttpPostEx(curlweb, _httppackweb);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	rapidjson::Document doc;
	doc.Parse(_httppackweb->strrecdata);
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
		if (doc.HasMember("message") && doc["message"].IsString())
			tmpstr = _strcoding.UTF_8ToString(doc["message"].GetString());
		printf("%s[User%d] Gift Error %s \n", _tool.GetTimeString().c_str(), _useropt.fileid, tmpstr.c_str());
		return BILIRET::NOFAULT;
	}
	printf("%s[User%d] Gift Success \n", _tool.GetTimeString().c_str(), _useropt.fileid);
	return BILIRET::NOFAULT;
}

// 移动端加密密钥
BILIRET CBilibiliUserInfo::_APIAndv2GetKey(std::string &psd) const
{
	printf("[User] Get APP RSA key... \n");
	_httppackapp->url = "https://passport.bilibili.com/api/oauth2/getKey";
	char datastr[400] = "";
	sprintf_s(datastr, sizeof(datastr), "appkey=%s&build=5230002&mobi_app=android&platform=android&ts=%I64d", 
		APP_KEY, _tool.GetTimeStamp());
	std::string sign;
	this->_GetMD5Sign(datastr, sign);
	_httppackapp->strsenddata = datastr;
	_httppackapp->strsenddata += "&sign=" + sign;
	_httppackapp->ClearHeader();
	_httppackapp->AddHeaderManual("Content-Type: application/x-www-form-urlencoded; charset=utf-8");
	int ret = toollib::HttpPostEx(curlapp, _httppackapp);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	rapidjson::Document doc;
	doc.Parse(_httppackapp->strrecdata);
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
BILIRET CBilibiliUserInfo::_APIAndv2Login(std::string username, std::string password)
{
	printf("[User] Logging in by app api... \n");
	_httppackapp->url = "https://passport.bilibili.com/api/v2/oauth2/login";
	std::string psd = _strcoding.UrlUTF8((char*)password.c_str());
	char datastr[400] = "";
	sprintf_s(datastr, sizeof(datastr), "appkey=%s&build=5230002&mobi_app=android&password=%s&platform=android&ts=%I64d&username=%s", 
		APP_KEY, psd.c_str(), _tool.GetTimeStamp(), username.c_str());
	std::string sign;
	this->_GetMD5Sign(datastr, sign);
	_httppackapp->strsenddata = datastr;
	_httppackapp->strsenddata += "&sign=" + sign;
	_httppackapp->ClearHeader();
	_httppackapp->AddHeaderManual("Content-Type: application/x-www-form-urlencoded; charset=utf-8");
	int ret = toollib::HttpPostEx(curlapp, _httppackapp);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	rapidjson::Document doc;
	doc.Parse(_httppackapp->strrecdata);
	if (!doc.IsObject() || !doc.HasMember("code") || !doc["code"].IsInt()) {
		return BILIRET::JSON_ERROR;
	}
	if (doc["code"].GetInt())
		return BILIRET::LOGIN_PASSWORDWRONG;

	// 登录成功
	_useropt.tokena = doc["data"]["token_info"]["access_token"].GetString();
	_useropt.tokenr = doc["data"]["token_info"]["refresh_token"].GetString();

	return BILIRET::NOFAULT;
}

BILIRET CBilibiliUserInfo::_APIAndv1RoomInfo(int rid) const {
	_httppackapp->url = _urlapi + "/activity/v1/Common/mobileRoomInfo?";
	char datastr[400] = "";
	sprintf_s(datastr, sizeof(datastr), "access_key=%s&actionKey=appkey&appkey=%s&build=5230002&device=android&mobi_app=android&platform=android&roomid=%d&ts=%I64d", 
		_useropt.tokena.c_str(), APP_KEY, rid, _tool.GetTimeStamp());
	std::string sign;
	this->_GetMD5Sign(datastr, sign);
	_httppackapp->url += datastr;
	_httppackapp->url += "&sign=" + sign;
	_httppackapp->ClearHeader();
	int ret = toollib::HttpGetEx(curlapp, _httppackapp);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	rapidjson::Document doc;
	doc.Parse(_httppackapp->strrecdata);
	if (!doc.IsObject() || !doc.HasMember("code") || !doc["code"].IsInt()) {
		return BILIRET::JSON_ERROR;
	}
	ret = doc["code"].GetInt();

	return BILIRET::NOFAULT;
}

// 领取风暴
BILIRET CBilibiliUserInfo::_APIAndv1StormJoin(long long cid) {
	_httppackapp->url = _urlapi + "/lottery/v1/Storm/join";
	char datastr[400] = "";
	sprintf_s(datastr, sizeof(datastr), "access_key=%s&actionKey=appkey&appkey=%s&build=5230002&device=android&id=%I64d&mobi_app=android&platform=android&ts=%I64d", 
		_useropt.tokena.c_str(), APP_KEY, cid, _tool.GetTimeStamp());
	std::string sign;
	this->_GetMD5Sign(datastr, sign);
	_httppackapp->strsenddata = datastr;
	_httppackapp->strsenddata += "&sign=" + sign;
	_httppackapp->ClearHeader();
	_httppackapp->AddHeaderManual("Content-Type: application/x-www-form-urlencoded; charset=utf-8");
	int ret = toollib::HttpPostEx(curlapp, _httppackapp);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	rapidjson::Document doc;
	doc.Parse(_httppackapp->strrecdata);
	if (!doc.IsObject() || !doc.HasMember("code") || !doc["code"].IsInt()) {
		return BILIRET::JSON_ERROR;
	}
	ret = doc["code"].GetInt();
	std::string msg;
	if (ret) {
		msg = _strcoding.UTF_8ToString(doc["msg"].GetString());
		printf("%s[User%d] Storm %d %s \n", _tool.GetTimeString().c_str(), _useropt.fileid, ret, msg.c_str());

		// 检查是否被封禁
		if (ret == 400) {
			if (msg.find("访问被拒绝") != -1) {
				this->SetBanned();
				return BILIRET::NOFAULT;
			}
		}
		return BILIRET::JOINEVENT_FAILED;
	}
	msg = _strcoding.UTF_8ToString(doc["data"]["mobile_content"].GetString());
	printf("%s[User%d] Storm %s \n", _tool.GetTimeString().c_str(), _useropt.fileid, msg.c_str());

	return BILIRET::NOFAULT;
}

// 活动抽奖
BILIRET CBilibiliUserInfo::_APIAndv1YunYing(std::string eventname, int rid, int raffleId) {
	_httppackapp->url = _urlapi + "/YunYing/roomEvent?";
	char datastr[400] = "";
	sprintf_s(datastr, sizeof(datastr), "access_key=%s&actionKey=appkey&appkey=%s&build=5230002&device=android&event_type=%s-%d&mobi_app=android&platform=android&room_id=%d&ts=%I64d", 
		_useropt.tokena.c_str(), APP_KEY, eventname.c_str(), raffleId, rid, _tool.GetTimeStamp());
	std::string sign;
	this->_GetMD5Sign(datastr, sign);
	_httppackapp->url += datastr;
	_httppackapp->url += "&sign=" + sign;
	_httppackapp->ClearHeader();
	int ret = toollib::HttpGetEx(curlapp, _httppackapp);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	rapidjson::Document doc;
	doc.Parse(_httppackapp->strrecdata);
	if (!doc.IsObject() || !doc.HasMember("code") || !doc["code"].IsInt()) {
		return BILIRET::JSON_ERROR;
	}
	ret = doc["code"].GetInt();
	std::string msg;
	if (ret) {
		msg = _strcoding.UTF_8ToString(doc["msg"].GetString());
		printf("%s[User%d] YunYing %d %s \n", _tool.GetTimeString().c_str(), _useropt.fileid, ret, msg.c_str());

		// 检查是否被封禁
		if (ret == 400) {
			if (msg.find("访问被拒绝") != -1) {
				this->SetBanned();
				return BILIRET::NOFAULT;
			}
		}
		return BILIRET::JOINEVENT_FAILED;
	}
	msg = _strcoding.UTF_8ToString(doc["data"]["gift_desc"].GetString());
	printf("%s[User%d] YunYing %s \n", _tool.GetTimeString().c_str(), _useropt.fileid, msg.c_str());

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
	ckname = "LIVE_LOGIN_DATA";
	if (_httpcookie.GetCookie(ckname, opt.tokenlive)) {
		opt.tokenlive = "";
	}
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

int CBilibiliUserInfo::SetBanned() {
	// 账户被封禁时取消所有抽奖以及瓜子的领取
	_useropt.conf = _useropt.conf & 0x1;
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
	ret = _httppackweb->sstrrecdata.find("captcha_key");
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
	randid = _tool.GetTimeStamp() * 1000 + rand() % 1000;
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
