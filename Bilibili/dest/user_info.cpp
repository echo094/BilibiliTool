#include "user_info.h"
#include <cstdlib>
#include <random>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "api_bl.h" 
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
	conf_gift(0),
	conf_storm(0),
	conf_guard(0),
	conf_pk(0),
	conf_danmu(0),
	conf_anchor(0),
	httpweb(new CHTTPPack("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/78.0.3904.108 Safari/537.36")),
	httpapp(new CHTTPPack("Mozilla/5.0 BiliDroid/5.43.0 (bbcallen@gmail.com)")),
	ioc_(),
	timer_(ioc_)
{
	curlweb = curl_easy_init();
	curlapp = curl_easy_init();
	httpweb->AddHeaderInf("Accept-Language: zh-CN,zh;q=0.8");
	httpweb->AddHeaderInf("Connection: keep-alive");
	httpweb->AddHeaderInf("DNT: 1");
	httpapp->AddHeaderInf("APP-KEY: android");
	httpapp->AddHeaderInf("Buvid: XZ843B78BAAB10F554BBD584271300E8DE86A");
	httpapp->AddHeaderInf("Device-ID: HCwUIUJwQndBcEVwDD4MPgw1V2EAZF0_CnYKOAo_CTgNOAo_Cz8IOAA1Vg");
	httpapp->AddHeaderInf("Display-ID: XZ843B78BAAB10F554BBD584271300E8DE86A-1560783638");
	httpapp->AddHeaderInf("env: prod");
	httpapp->AddHeaderInf("Connection: keep-alive");
	// 启动用户IO
	worker_ = std::make_shared< boost::asio::io_context::work>(ioc_);
	start_timer();
	thread_ = std::make_shared< std::thread>(
		boost::bind(&boost::asio::io_service::run, &ioc_)
		);
}

user_info::~user_info() {
	// 停止用户IO
	timer_.cancel();
	clear_task();
	worker_.reset();
	thread_->join();
	thread_.reset();
	// 清理其它内容
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
	if (data["Conf"].HasMember("CoinExchange")) {
		conf_coin = data["Conf"]["CoinExchange"].GetUint();
	}
	if (data["Conf"].HasMember("Gift")) {
		conf_gift = data["Conf"]["Gift"].GetUint();
	}
	if (data["Conf"].HasMember("Storm")) {
		conf_storm = data["Conf"]["Storm"].GetUint();
	}
	if (data["Conf"].HasMember("Guard")) {
		conf_guard = data["Conf"]["Guard"].GetUint();
	}
	if (data["Conf"].HasMember("PK")) {
		conf_pk = data["Conf"]["PK"].GetUint();
	}
	if (data["Conf"].HasMember("Danmu")) {
		conf_danmu = data["Conf"]["Danmu"].GetUint();
	}
	if (data["Conf"].HasMember("Anchor")) {
		conf_anchor = data["Conf"]["Anchor"].GetUint();
	}
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
	conf.AddMember("Gift", conf_gift, allocator);
	conf.AddMember("Storm", conf_storm, allocator);
	conf.AddMember("Guard", conf_guard, allocator);
	conf.AddMember("PK", conf_pk, allocator);
	conf.AddMember("Danmu", conf_danmu, allocator);
	conf.AddMember("Anchor", conf_anchor, allocator);
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
	// 添加到 doc
	doc.PushBack(data.Move(), allocator);
}

bool user_info::CheckBanned(const std::string &msg) {
	if (msg.find(u8"访问被拒绝") != -1) {
		this->SetBanned();
		return true;
	}
	return false;
}

void user_info::SetBanned() {
	// 账户被封禁时取消所有抽奖
	conf_gift = 0;
	conf_storm = 0;
	conf_guard = 0;
	conf_pk = 0;
	conf_danmu = 0;
	conf_anchor = 0;
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

void user_info::post_task(std::shared_ptr<BILI_LOTTERYDATA> lot)
{
	boost::asio::post(
		ioc_,
		[this, lot] {
		BOOST_LOG_SEV(g_logger::get(), trace) << "[User] Push task type: " << lot->cmd;
		if (lot->cmd == MSG_LOT_GIFT && conf_gift == 1) {
			// 送礼类别的提前进入房间
			this->do_visit(lot->rrid, toollib::GetTimeStamp() * 1000);
		}
		this->tasks_.push(lot);
	});
}

void user_info::clear_task()
{
	boost::asio::post(
		ioc_,
		[this] {
		BOOST_LOG_SEV(g_logger::get(), trace) << "[User] Clear task. ";
		while (this->tasks_.size()) {
			this->tasks_.pop();
		}
	});
}

void user_info::start_timer()
{
	timer_.expires_from_now(boost::posix_time::milliseconds(100));
	timer_.async_wait(
		boost::bind(
			&user_info::on_timer,
			this,
			boost::asio::placeholders::error
		)
	);
}

void user_info::on_timer(boost::system::error_code ec)
{
	static thread_local std::mt19937 gen;
	static thread_local boost::posix_time::ptime epoch(
		boost::gregorian::date(1970, boost::gregorian::Jan, 1));
	if (ec) {
		return;
	}
	// 检查任务列表
	boost::posix_time::time_duration now =
		boost::posix_time::microsec_clock::universal_time() - epoch;
	auto curtime = now.total_milliseconds();
	std::uniform_int_distribution<int> dis(100, 300);
	while (!tasks_.empty() && tasks_.top()->time_get < curtime) {
		auto p = tasks_.top();
		tasks_.pop();
		if (do_task(p, curtime) == BILIRET::JOIN_AGAIN
			&& (p->time_start + 22000 > curtime)) {
			// 节奏风暴没抽中且时间在22秒内
			p->time_get += dis(gen);
			tasks_.push(p);
		}
	}

	start_timer();
}

BILIRET user_info::do_task(
	const std::shared_ptr<BILI_LOTTERYDATA>& data, 
	long long curtime)
{
	BOOST_LOG_SEV(g_logger::get(), trace) << "[User] Do task type: " << data->cmd
		<< " id: " << data->loid;
	BILIRET ret = BILIRET::NOFAULT;
	switch (data->cmd) {
	case MSG_LOT_STORM: {
		if (conf_storm == 1) {
			do_visit(data->rrid, curtime);
			ret = apibl::APIWebv1StormJoin(this, data, "", "");
		}
		break;
	}
	case MSG_LOT_GIFT: {
		if (conf_gift == 1) {
			ret = apibl::APIWebv5SmalltvJoin(this, data);
		}
		break;
	}
	case MSG_LOT_GUARD: {
		if (conf_guard == 1) {
			do_visit(data->rrid, curtime);
			ret = apibl::APIWebv3GuardJoin(this, data);
		}
		break;
	}
	case MSG_LOT_PK: {
		if (conf_pk == 1) {
			do_visit(data->rrid, curtime);
			ret = apibl::APIWebv2PKJoin(this, data);
		}
		break;
	}
	case MSG_LOT_DANMU: {
		if (conf_danmu == 1) {
			do_visit(data->rrid, curtime);
			ret = apibl::APIWebv1DanmuJoin(this, data);
		}
		break;
	}
	case MSG_LOT_ANCHOR: {
		if (conf_anchor == 1) {
			do_visit(data->rrid, curtime);
			ret = apibl::APIWebv1AnchorJoin(this, data);
		}
		break;
	}
	}
	return ret;
}

void user_info::do_visit(unsigned room, long long curtime)
{
	if (his_visit_.count(room) &&
		his_visit_[room] > curtime) {
		return;
	}
	his_visit_[room] = curtime + 180000;
	apibl::APIWebv1RoomEntry(this, room);
}
