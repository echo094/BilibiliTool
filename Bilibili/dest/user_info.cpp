#include "user_info.h"
#include <cstdlib>
#include "utility/base64.h"
#include "utility/sslex.h"
#include "utility/strconvert.h"
#include "logger/log.h"
using namespace toollib;

user_info::user_info() :
	islogin(false),
	fileid(0),
	uid(0),
	conf_coin(0),
	conf_lottery(0),
	conf_storm(0),
	conf_guard(0),
	httpweb(new CHTTPPack()),
	httpapp(new CHTTPPack("Mozilla/5.0 BiliDroid/5.23.2 (bbcallen@gmail.com)")) {
	curlweb = curl_easy_init();
	curlapp = curl_easy_init();
	httpweb->AddHeaderInf("Accept-Language: zh-CN,zh;q=0.8");
	httpweb->AddHeaderInf("Connection: keep-alive");
	httpweb->AddHeaderInf("DNT: 1");
	httpapp->AddHeaderInf("Buvid: A59813F7-2A50-42C5-A246-AF93A96374E320912infoc");
	httpapp->AddHeaderInf("Device-ID: KRkhFHdFd0J0RXBFOUU5Cj8KPQg_Dz4OOQkxBDUA");
	httpapp->AddHeaderInf("Display-ID: 759639-1523371597");
	httpapp->AddHeaderInf("Connection: keep-alive");
}

user_info::~user_info() {
	curl_easy_cleanup(curlweb);
	curlweb = nullptr;
	curl_easy_cleanup(curlapp);
	curlapp = nullptr;
	httpweb = nullptr;
	httpapp = nullptr;
	BOOST_LOG_SEV(g_logger::get(), debug) << "[User] Stop.";
}

// 从文件导入指定账户
void user_info::ReadFileAccount(const std::string &key, const rapidjson::Value& data, int index) {
	std::string enstr;
	// 编号
	fileid = index;
	// UID
	if (!data.HasMember("UserID") || !data["UserID"].IsUint()) {
		throw "Key UserID error";
	}
	uid = data["UserID"].GetUint();
	// 获取配置信息 各种活动的参与参数
	if (!data.HasMember("Conf") || !data["Conf"].IsObject()) {
		throw "Key Conf error";
	}
	conf_coin = data["Conf"]["CoinExchange"].GetUint();
	conf_lottery = data["Conf"]["Lottery"].GetUint();
	conf_storm = data["Conf"]["Storm"].GetUint();
	conf_guard = data["Conf"]["Guard"].GetUint();
	// 账号
	if (!data.HasMember("Username") || !data["Username"].IsString()) {
		throw "Key Username error";
	}
	account = data["Username"].GetString();
	// 密码
	if (!data.HasMember("Password") || !data["Password"].IsString()) {
		throw "Key Password error";
	}
	std::string buff = data["Password"].GetString();
	bool ret = Decrypt_RSA_KeyBuff((char*)key.c_str(), buff, password);
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
	toollib::HttpImportCookie(curlweb, cookie);
	// 移动端AccessToken 需要加密
	if (!data.HasMember("AccessToken") || !data["AccessToken"].IsString()) {
		throw "Key AccessToken error";
	}
	tokena = data["AccessToken"].GetString();
	// 移动端RefreshToken 需要加密
	if (!data.HasMember("RefreshToken") || !data["RefreshToken"].IsString()) {
		throw "Key RefreshToken error";
	}
	tokenr = data["RefreshToken"].GetString();
}

// 将账户信息导出到文件
void user_info::WriteFileAccount(const std::string key, rapidjson::Document& doc) {
	using namespace rapidjson;
	Document::AllocatorType& allocator = doc.GetAllocator();
	Value data(kObjectType);

	// UID
	data.AddMember("UserID", uid, allocator);
	// 配置信息
	Value conf(kObjectType);
	conf.AddMember("CoinExchange", conf_coin, allocator);
	conf.AddMember("Lottery", conf_lottery, allocator);
	conf.AddMember("Storm", conf_storm, allocator);
	conf.AddMember("Guard", conf_guard, allocator);
	data.AddMember("Conf", conf.Move(), allocator);
	// 账号
	data.AddMember(
		"Username",
		Value(account.c_str(), doc.GetAllocator()).Move(),
		allocator
	);
	// 密码
	std::string buff;
	bool ret = Encrypt_RSA_KeyBuff((char*)key.c_str(), password, buff);
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
	toollib::HttpExportCookie(curlweb, cookie);
	ret = Encode_Base64((const unsigned char *)cookie.c_str(), cookie.size(), buff);
	data.AddMember(
		"Cookie",
		Value(buff.c_str(), doc.GetAllocator()).Move(),
		allocator
	);
	// 移动端AccessToken
	data.AddMember(
		"AccessToken",
		Value(tokena.c_str(), doc.GetAllocator()).Move(),
		allocator
	);
	// 移动端RefreshToken
	data.AddMember(
		"RefreshToken",
		Value(tokenr.c_str(), doc.GetAllocator()).Move(),
		allocator
	);
	doc.PushBack(data.Move(), allocator);
}

bool user_info::CheckBanned(const std::string &msg) {
	std::wstring wmsg;
	toollib::UTF8ToUTF16(msg, wmsg);
	if (wmsg.find(L"访问被拒绝") != -1) {
		this->SetBanned();
		return true;
	}
	return false;
}

void user_info::SetBanned() {
	// 账户被封禁时取消所有抽奖
	conf_lottery = 0;
	conf_storm = 0;
	conf_guard = 0;
}

// 生成随机访问ID
// ((new Date).getTime() * Math.ceil(1e6 * Math.random())).toString(36)
void user_info::GetVisitID() {
	std::string sid;
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
	visitid = sid;
}

bool user_info::GetToken() {
	if (toollib::HttpGetCookieVal(curlweb, "bili_jct", tokenjct)) {
		tokenjct = "";
		return false;
	}
	return true;
}

int user_info::GetExpiredTime() {
	int cktime;
	if (toollib::HttpGetCookieTime(curlweb, "bili_jct", cktime)) {
		return 0;
	}
	return cktime;
}
