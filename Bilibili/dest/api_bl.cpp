#include "api_bl.h" 

#include <fstream>
#include <sstream>
#include "rapidjson/document.h"

#include "logger/log.h"
#include "utility/md5.h"
#include "utility/sslex.h"
#include "utility/strconvert.h"
using namespace apibl;
using namespace toollib;

#ifdef GetObject
#undef GetObject
#endif

int GetMD5Sign(const char *in, std::string &sign) {
	std::string str = in;
	str += APP_SECRET;
	sign = Encode_MD5(str);
	return 0;
}

BILIRET apibl::APIWebGetCoin(const std::shared_ptr<user_info>& user) {
	user->httpweb->url = "https://account.bilibili.com/site/getCoin";
	user->httpweb->ClearHeader();
	user->httpweb->AddHeaderManual("Accept: application/json, text/plain, */*");
	int ret = toollib::HttpGetEx(user->curlweb, user->httpweb);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	rapidjson::Document doc;
	doc.Parse(user->httpweb->recv_data.c_str());
	if (!doc.IsObject() || !doc.HasMember("code") || !doc["code"].IsInt() || doc["code"].GetInt()) {
		return BILIRET::JSON_ERROR;
	}
	double money = doc["data"]["money"].GetDouble();
	BOOST_LOG_SEV(g_logger::get(), info) << "[User" << user->fileid << "] "
		<< "APIWebGetCoin: Current Coin " << money;

	return BILIRET::NOFAULT;
}

BILIRET apibl::APIWebGETLoginCaptcha(const std::shared_ptr<user_info>& user) {
	BOOST_LOG_SEV(g_logger::get(), info) << "[User] Get captcha...";
	user->httpapp->url = "https://passport.bilibili.com/captcha?rnd=569";
	user->httpapp->ClearHeader();
	int ret = toollib::HttpGetEx(user->curlapp, user->httpapp);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	std::ofstream out;
	out.open("Captcha.jpg", std::ios::binary);
	out.write(user->httpapp->recv_data.c_str(), user->httpapp->recv_data.size());
	out.close();

	return BILIRET::NOFAULT;
}

BILIRET apibl::APIWebGetCaptchaKey(const std::shared_ptr<user_info>& user) {
	// 新增Cookie验证
	int ret;
	user->httpweb->url = "https://www.bilibili.com/plus/widget/ajaxGetCaptchaKey.php?js";
	user->httpweb->ClearHeader();
	ret = toollib::HttpGetEx(user->curlweb, user->httpweb);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}
	ret = user->httpweb->recv_data.find("captcha_key");
	if (ret == -1) {
		return BILIRET::HTMLTEXT_ERROR;
	}
	// finger生成于如下脚本中
	// https://s1.hdslb.com/bfs/seed/log/report/log-reporter.js
	// 目前该Cookie已被删除

	return BILIRET::NOFAULT;
}

BILIRET apibl::APIWebOnlineHeart(const std::shared_ptr<user_info>& user) {
	// -403 非法心跳
	user->httpweb->url = URL_LIVEAPI_HEAD + "/User/userOnlineHeart";
	std::ostringstream oss;
	oss << "&csrf_token=" << user->tokenjct
		<< "&csrf=" << user->tokenjct
		<< "&visit_id=";
	user->httpweb->send_data = oss.str();
	user->httpweb->ClearHeader();
	user->httpweb->AddHeaderManual("Accept:application/json, text/javascript, */*; q=0.01");
	user->httpweb->AddHeaderManual(URL_DEFAULT_ORIGIN);
	user->httpweb->AddHeaderManual(URL_DEFAULT_REFERER);
	int ret = toollib::HttpPostEx(user->curlweb, user->httpweb);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	rapidjson::Document doc;
	doc.Parse(user->httpweb->recv_data.c_str());
	if (!doc.IsObject() || !doc.HasMember("code") || !doc["code"].IsInt()) {
		return BILIRET::JSON_ERROR;
	}
	ret = doc["code"].GetInt();
	if (ret) {
		BOOST_LOG_SEV(g_logger::get(), warning) << "[User" << user->fileid << "] "
			<< "APIWebOnlineHeart: " << user->httpweb->recv_data;
		return BILIRET::JSON_ERROR;
	}
	BOOST_LOG_SEV(g_logger::get(), info) << "[User" << user->fileid << "] "
		<< "APIWebOnlineHeart: OK";

	return BILIRET::NOFAULT;
}

BILIRET apibl::APIWebTaskInfo(const std::shared_ptr<user_info>& user) {
	user->httpweb->url = URL_LIVEAPI_HEAD + "/i/api/taskInfo";
	user->httpweb->ClearHeader();
	user->httpweb->AddHeaderManual("Accept: application/json, text/plain, */*");
	user->httpweb->AddHeaderManual("Origin: https://link.bilibili.com");
	user->httpweb->AddHeaderManual("Referer: https://link.bilibili.com/p/center/index");
	int ret = toollib::HttpGetEx(user->curlweb, user->httpweb);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	rapidjson::Document doc;
	doc.Parse(user->httpweb->recv_data.c_str());
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
	BOOST_LOG_SEV(g_logger::get(), info) << "[User" << user->fileid << "] "
		<< "APIWebTaskInfo: " << ret;
	if (ret == 1) {
		return APIWebv1TaskAward(user);
	}

	return BILIRET::NOFAULT;
}

BILIRET apibl::APIWebSign(const std::shared_ptr<user_info>& user) {
	user->httpweb->url = URL_LIVEAPI_HEAD + "/sign/doSign";
	user->httpweb->ClearHeader();
	int ret = toollib::HttpGetEx(user->curlweb, user->httpweb);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	// 0签到成功 -500已签到
	rapidjson::Document doc;
	doc.Parse(user->httpweb->recv_data.c_str());
	if (!doc.IsObject() || !doc.HasMember("code") || !doc["code"].IsInt()) {
		return BILIRET::JSON_ERROR;
	}
	ret = doc["code"].GetInt();
	if (ret == -500) {
		BOOST_LOG_SEV(g_logger::get(), info) << "[User" << user->fileid << "] "
			<< "APIWebSign: Signed.";
		return BILIRET::NOFAULT;
	}
	if (ret) {
		BOOST_LOG_SEV(g_logger::get(), warning) << "[User" << user->fileid << "] "
			<< "APIWebSign: " << user->httpweb->recv_data;
		return BILIRET::JSON_ERROR;
	}
	BOOST_LOG_SEV(g_logger::get(), info) << "[User" << user->fileid << "] "
		<< "APIWebSign: Success.";

	return BILIRET::NOFAULT;
}

BILIRET apibl::APIWebGetUserInfo(const std::shared_ptr<user_info>& user) {
	BOOST_LOG_SEV(g_logger::get(), info) << "[User] Get user live info...";
	user->httpweb->url = URL_LIVEAPI_HEAD + "/xlive/web-ucenter/user/get_user_info";
	user->httpweb->ClearHeader();
	int ret = toollib::HttpGetEx(user->curlweb, user->httpweb);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	rapidjson::Document doc;
	doc.Parse(user->httpweb->recv_data.c_str());
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

BILIRET apibl::APIWebv1DanmuConf(
	CURL * pcurl,
	unsigned room,
	const std::string player, 
	std::string & key
) {
	std::unique_ptr<toollib::CHTTPPack> httpdata(new toollib::CHTTPPack());
	std::ostringstream oss;
	oss << URL_LIVEAPI_HEAD << "/room/v1/Danmu/getConf"
		<< "?room_id=" << room
		<< "&platform=pc"
		<< "&player=" << player;
	httpdata->url = oss.str();
	httpdata->AddHeaderManual("Accept: application/json, text/plain, */*");
	int ret = toollib::HttpGetEx(pcurl, httpdata);
	if (ret) {
		BOOST_LOG_SEV(g_logger::get(), error) << "[APIbl] APIWebv1DanmuConf HTTP error: " << ret;
		return BILIRET::HTTP_ERROR;
	}

	rapidjson::Document doc;
	doc.Parse(httpdata->recv_data.c_str());
	if (!doc.IsObject() || !doc.HasMember("code") || !doc["code"].IsInt() || doc["code"].GetInt()
		|| !doc.HasMember("data") || !doc["data"].IsObject()) {
		BOOST_LOG_SEV(g_logger::get(), error) << "[APIbl] APIWebv1DanmuConf JSON error!";
		return BILIRET::JSON_ERROR;
	}
	key = doc["data"]["token"].GetString();

	return BILIRET::NOFAULT;
}

// 直播经验心跳日志
BILIRET apibl::APIWebv1HeartBeat(const std::shared_ptr<user_info> &user) {
	int ret;
	std::ostringstream oss;
	oss << URL_LIVEAPI_HEAD << "/relation/v1/feed/heartBeat?_=" << GetTimeStamp() << "479";
	user->httpweb->url = oss.str();
	user->httpweb->ClearHeader();
	user->httpweb->AddHeaderManual("Accept:*/*");
	user->httpweb->AddHeaderManual(URL_DEFAULT_REFERER);
	ret = toollib::HttpGetEx(user->curlweb, user->httpweb);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	rapidjson::Document doc;
	doc.Parse(user->httpweb->recv_data.c_str());
	if (!doc.IsObject() || !doc.HasMember("code") || !doc["code"].IsInt()) {
		return BILIRET::JSON_ERROR;
	}
	ret = doc["code"].GetInt();
	if (ret) {
		BOOST_LOG_SEV(g_logger::get(), info) << "[User" << user->fileid << "] "
			<< "APIWebv1HeartBeat: " << ret;
	}
	else {
		BOOST_LOG_SEV(g_logger::get(), info) << "[User" << user->fileid << "] "
			<< "APIWebv1HeartBeat: OK";
	}

	return BILIRET::NOFAULT;
}

BILIRET apibl::APIWebv1TaskAward(const std::shared_ptr<user_info>& user) {
	user->httpweb->url = URL_LIVEAPI_HEAD + "/activity/v1/task/receive_award";
	std::ostringstream oss;
	oss << "task_id=double_watch_task"
		<< "&csrf_token=" << user->tokenjct
		<< "&csrf=" << user->tokenjct;
	user->httpweb->send_data = oss.str();
	user->httpweb->ClearHeader();
	user->httpweb->AddHeaderManual("Accept: application/json, text/plain, */*");
	user->httpweb->AddHeaderManual("Content-Type: application/x-www-form-urlencoded");
	user->httpweb->AddHeaderManual("Origin: https://link.bilibili.com");
	user->httpweb->AddHeaderManual("Referer: https://link.bilibili.com/p/center/index");
	int ret = toollib::HttpPostEx(user->curlweb, user->httpweb);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	rapidjson::Document doc;
	doc.Parse(user->httpweb->recv_data.c_str());
	if (!doc.IsObject() || !doc.HasMember("code") || !doc["code"].IsInt()) {
		BOOST_LOG_SEV(g_logger::get(), warning) << "[User" << user->fileid << "] "
			<< "APIWebv1TaskAward： JSON error.";
		return BILIRET::JSON_ERROR;
	}
	BOOST_LOG_SEV(g_logger::get(), info) << "[User" << user->fileid << "] "
		<< "APIWebv1TaskAward: " << doc["code"].GetInt();

	return BILIRET::NOFAULT;
}

BILIRET apibl::APIWebv1Silver2Coin(const std::shared_ptr<user_info>& user) {
	user->httpweb->url = URL_LIVEAPI_HEAD + "/pay/v1/Exchange/silver2coin";
	user->httpweb->send_data = "platform=pc&csrf_token=" + user->tokenjct;
	user->httpweb->ClearHeader();
	user->httpweb->AddHeaderManual("Accept: application/json, text/javascript, */*; q=0.01");
	user->httpweb->AddHeaderManual("Content-Type: application/x-www-form-urlencoded; charset=UTF-8");
	user->httpweb->AddHeaderManual(URL_DEFAULT_ORIGIN);
	user->httpweb->AddHeaderManual("Referer: https://live.bilibili.com/exchange");
	int ret = toollib::HttpPostEx(user->curlweb, user->httpweb);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	// 0兑换成功 403已兑换
	rapidjson::Document doc;
	doc.Parse(user->httpweb->recv_data.c_str());
	if (!doc.IsObject() || !doc.HasMember("code") || !doc["code"].IsInt()) {
		return BILIRET::JSON_ERROR;
	}
	ret = doc["code"].GetInt();
	std::string msg = doc["msg"].GetString();
	BOOST_LOG_SEV(g_logger::get(), info) << "[User" << user->fileid << "] "
		<< "APIWebv1Silver2Coin: " << msg;

	return BILIRET::NOFAULT;
}

BILIRET apibl::APIWebv1RoomEntry(const std::shared_ptr<user_info>& user, unsigned room) {
	user->httpweb->url = URL_LIVEAPI_HEAD + "/room/v1/Room/room_entry_action";
	std::ostringstream oss;
	oss << "room_id=" << room
		<< "&platform=pc&csrf_token=" << user->tokenjct
		<< "&visit_id=" << user->visitid;
	user->httpweb->send_data = oss.str();
	user->httpweb->ClearHeader();
	user->httpweb->AddHeaderManual("Accept: application/json, text/plain, */*");
	user->httpweb->AddHeaderManual("Content-Type: application/x-www-form-urlencoded");
	user->httpweb->AddHeaderManual(URL_DEFAULT_ORIGIN);
	std::string strreffer(URL_DEFAULT_REFERERBASE);
	strreffer += std::to_string(room);
	user->httpweb->AddHeaderManual(strreffer.c_str());
	int ret = toollib::HttpPostEx(user->curlweb, user->httpweb);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	return BILIRET::NOFAULT;
}

BILIRET apibl::APIWebv1StormJoin(
	std::shared_ptr<user_info> &user,
	std::shared_ptr<BILI_LOTTERYDATA> data,
	std::string code,
	std::string token
) {
	user->httpweb->url = URL_LIVEAPI_HEAD + "/lottery/v1/Storm/join";
	std::ostringstream oss;
	oss << "id=" << data->loid
		<< "&color=16777215"
		<< "&captcha_token=" << token
		<< "&captcha_phrase=" << code
		<< "&roomid=" << data->rrid
		<< "&csrf_token=" << user->tokenjct
		<< "&csrf=" << user->tokenjct
		<< "&visit_id=" << user->visitid;
	user->httpweb->send_data = oss.str();
	user->httpweb->ClearHeader();
	user->httpweb->AddHeaderManual("Accept: application/json, text/plain, */*");
	user->httpweb->AddHeaderManual("Content-Type: application/x-www-form-urlencoded");
	user->httpweb->AddHeaderManual(URL_DEFAULT_ORIGIN);
	std::string strreffer(URL_DEFAULT_REFERERBASE);
	strreffer += std::to_string(data->srid);
	user->httpweb->AddHeaderManual(strreffer.c_str());
	int ret = toollib::HttpPostEx(user->curlweb, user->httpweb);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	rapidjson::Document doc;
	doc.Parse(user->httpweb->recv_data.c_str());
	if (!doc.IsObject() || !doc.HasMember("code") || !doc["code"].IsInt()) {
		return BILIRET::JSON_ERROR;
	}
	ret = doc["code"].GetInt();
	std::string msg;
	// 429需要验证码 400未抽中或已过期
	if (ret) {
		msg = doc["msg"].GetString();
		BOOST_LOG_SEV(g_logger::get(), warning) << "[User" << user->fileid << "] "
			<< "APIWebv1StormJoin: " << ret << " " << msg;
		// 检查是否被封禁
		if (ret == 400) {
			if (user->CheckBanned(msg)) {
				return BILIRET::NOFAULT;
			}
		}
		return BILIRET::JOINEVENT_FAILED;
	}
	// 抽中的返回值为0
	msg = doc["data"]["mobile_content"].GetString();
	BOOST_LOG_SEV(g_logger::get(), info) << "[User" << user->fileid << "] "
		<< "APIWebv1StormJoin: " << msg;

	return BILIRET::NOFAULT;
}

BILIRET apibl::APIWebv1PKJOIN(
	std::shared_ptr<user_info> &user,
	std::shared_ptr<BILI_LOTTERYDATA> data
) {
	user->httpweb->url = URL_LIVEAPI_HEAD + "/xlive/lottery-interface/v1/pk/join";
	std::ostringstream oss;
	oss << "roomid=" << data->rrid
		<< "&id=" << data->loid
		<< "&csrf_token=" << user->tokenjct
		<< "&csrf=" << user->tokenjct
		<< "&visit_id=" << user->visitid;
	user->httpweb->send_data = oss.str();
	user->httpweb->ClearHeader();
	user->httpweb->AddHeaderManual("Accept: application/json, text/plain, */*");
	user->httpweb->AddHeaderManual("Content-Type: application/x-www-form-urlencoded");
	user->httpweb->AddHeaderManual(URL_DEFAULT_ORIGIN);
	oss.str("");
	oss << URL_DEFAULT_REFERERBASE << data->srid;
	user->httpweb->AddHeaderManual(oss.str().c_str());
	int ret = toollib::HttpPostEx(user->curlweb, user->httpweb);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	rapidjson::Document doc;
	doc.Parse(user->httpweb->recv_data.c_str());
	if (!doc.IsObject() || !doc.HasMember("code") || !doc["code"].IsInt()) {
		return BILIRET::JSON_ERROR;
	}

	// 0成功 -400已领取 -500系统繁忙
	int icode = doc["code"].GetInt();
	if (icode) {
		// 检查是否被封禁
		if (icode == 400) {
			user->SetBanned();
		}
		std::string tmpstr;
		if (doc.HasMember("message") && doc["message"].IsString()) {
			tmpstr = doc["message"].GetString();
		}
		BOOST_LOG_SEV(g_logger::get(), info) << "[User" << user->fileid << "] "
			<< "APIWebv1PKJOIN: " << tmpstr;
		return BILIRET::NOFAULT;
	}
	BOOST_LOG_SEV(g_logger::get(), info) << "[User" << user->fileid << "] "
		<< "APIWebv1PKJOIN award: " << doc["data"]["award_text"].GetString();

	return BILIRET::NOFAULT;
}

BILIRET apibl::APIWebv1CapsuleCheck(const std::shared_ptr<user_info>& user) {
	user->httpweb->url = URL_LIVEAPI_HEAD + "/xlive/web-ucenter/v1/capsule/get_detail?from=room";
	user->httpweb->ClearHeader();
	user->httpweb->AddHeaderManual("Accept: application/json, text/javascript, */*; q=0.01");
	user->httpweb->AddHeaderManual(URL_DEFAULT_ORIGIN);
	user->httpweb->AddHeaderManual(URL_DEFAULT_REFERER);
	int ret = toollib::HttpGetEx(user->curlweb, user->httpweb);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	rapidjson::Document doc;
	doc.Parse(user->httpweb->recv_data.c_str());
	if (!doc.IsObject() || !doc.HasMember("code")
		|| !doc["code"].IsInt() || doc["code"].GetInt()
		|| !doc.HasMember("data") || !doc["data"].IsObject()) {
		return BILIRET::JSON_ERROR;
	}
	printf("  Capsule info:\n");
	if (doc["data"].HasMember("normal") && doc["data"]["normal"].IsObject()
		&& doc["data"]["normal"].HasMember("coin") && doc["data"]["normal"]["coin"].IsInt()) {
		BOOST_LOG_SEV(g_logger::get(), info) << "[User" << user->fileid << "] "
			<< "Normal: " << doc["data"]["normal"]["coin"].GetInt();
	}
	if (doc["data"].HasMember("colorful") && doc["data"]["colorful"].IsObject()
		&& doc["data"]["colorful"].HasMember("coin") && doc["data"]["colorful"]["coin"].IsInt()) {
		BOOST_LOG_SEV(g_logger::get(), info) << "[User" << user->fileid << "] "
			<< "Colorful: " << doc["data"]["colorful"]["coin"].GetInt();
	}

	return BILIRET::NOFAULT;
}

BILIRET apibl::APIWebv2GiftDaily(const std::shared_ptr<user_info>& user) {
	user->httpweb->url = URL_LIVEAPI_HEAD + "/gift/v2/live/receive_daily_bag";
	user->httpweb->ClearHeader();
	user->httpweb->AddHeaderManual("Accept: application/json, text/plain, */*");
	user->httpweb->AddHeaderManual(URL_DEFAULT_ORIGIN);
	user->httpweb->AddHeaderManual(URL_DEFAULT_REFERER);
	int ret = toollib::HttpGetEx(user->curlweb, user->httpweb);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	rapidjson::Document doc;
	doc.Parse(user->httpweb->recv_data.c_str());
	if (!doc.IsObject() || !doc.HasMember("code") || !doc["code"].IsInt() || doc["code"].GetInt()) {
		BOOST_LOG_SEV(g_logger::get(), warning) << "[User" << user->fileid << "] "
			<< "APIWebv2GiftDaily: failed.";
		return BILIRET::JSON_ERROR;
	}
	ret = doc["data"]["bag_status"].GetInt();
	if (ret == 1) {
		BOOST_LOG_SEV(g_logger::get(), info) << "[User" << user->fileid << "] "
			<< u8"APIWebv2GiftDaily: 每日礼物领取成功";
	}
	else if (ret == 2) {
		BOOST_LOG_SEV(g_logger::get(), info) << "[User" << user->fileid << "] "
			<< u8"APIWebv2GiftDaily: 每日礼物已领取";
	}
	else {
		BOOST_LOG_SEV(g_logger::get(), warning) << "[User" << user->fileid << "] "
			<< "APIWebv2GiftDaily: " << ret;
	}

	return BILIRET::NOFAULT;
}

BILIRET apibl::APIWebv2LotteryJoin(std::shared_ptr<user_info>& user, std::shared_ptr<BILI_LOTTERYDATA> data) {
	user->httpweb->url = URL_LIVEAPI_HEAD + "/lottery/v2/Lottery/join";
	std::ostringstream oss;
	oss << "roomid=" << data->rrid
		<< "&id=" << data->loid
		<< "&type=" << data->type
		<< "&csrf_token=" << user->tokenjct
		<< "&csrf=" << user->tokenjct
		<< "&visit_id=" << user->visitid;
	user->httpweb->send_data = oss.str();
	user->httpweb->ClearHeader();
	user->httpweb->AddHeaderManual("Accept: application/json, text/plain, */*");
	user->httpweb->AddHeaderManual("Content-Type: application/x-www-form-urlencoded");
	user->httpweb->AddHeaderManual(URL_DEFAULT_ORIGIN);
	std::string strreffer(URL_DEFAULT_REFERERBASE);
	strreffer += std::to_string(data->srid);
	user->httpweb->AddHeaderManual(strreffer.c_str());
	int ret = toollib::HttpPostEx(user->curlweb, user->httpweb);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	rapidjson::Document doc;
	doc.Parse(user->httpweb->recv_data.c_str());
	if (!doc.IsObject() || !doc.HasMember("code") || !doc["code"].IsInt()) {
		return BILIRET::JSON_ERROR;
	}

	std::string tmpstr;
	int icode = doc["code"].GetInt();
	if (icode) {
		// 检查是否被封禁
		if (icode == 400) {
			user->SetBanned();
		}
		if (doc.HasMember("message") && doc["message"].IsString()) {
			tmpstr = doc["message"].GetString();
		}
	}
	else {
		tmpstr = doc["data"]["message"].GetString();
	}
	BOOST_LOG_SEV(g_logger::get(), info) << "[User" << user->fileid << "] "
		<< "APIWebv2LotteryJoin: " << tmpstr;

	return BILIRET::NOFAULT;
}

BILIRET apibl::APIWebv2GiftBag(const std::shared_ptr<user_info>& user) {
	user->httpweb->url = URL_LIVEAPI_HEAD + "/gift/v2/gift/bag_list";
	user->httpweb->ClearHeader();
	user->httpweb->AddHeaderManual("Accept: application/json, text/plain, */*");
	user->httpweb->AddHeaderManual(URL_DEFAULT_ORIGIN);
	user->httpweb->AddHeaderManual(URL_DEFAULT_REFERER);
	int ret = toollib::HttpGetEx(user->curlweb, user->httpweb);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	rapidjson::Document doc;
	doc.Parse(user->httpweb->recv_data.c_str());
	if (!doc.IsObject() || !doc.HasMember("code") || !doc["code"].IsInt() || doc["code"].GetInt()) {
		BOOST_LOG_SEV(g_logger::get(), warning) << "[User" << user->fileid << "] "
			<< "APIWebv2GiftBag: Get bag info failed.";
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
			printf("Expire:%6.2f Day\n", expiretime / 86400.0);
		}
		else {
			printf("Expire: Infinite\n");
		}
	}

	return BILIRET::NOFAULT;
}

BILIRET apibl::APIWebv3SmallTV(
	std::shared_ptr<user_info> &user,
	std::shared_ptr<BILI_LOTTERYDATA> data
) {
	user->httpweb->url = URL_LIVEAPI_HEAD + "/xlive/lottery-interface/v3/smalltv/Join";
	std::ostringstream oss;
	oss << "roomid=" << data->rrid
		<< "&raffleId=" << data->loid
		<< "&type=Gift"
		<< "&csrf_token=" << user->tokenjct
		<< "&csrf=" << user->tokenjct
		<< "&visit_id=" << user->visitid;
	user->httpweb->send_data = oss.str();
	user->httpweb->ClearHeader();
	user->httpweb->AddHeaderManual("Accept: application/json, text/plain, */*");
	user->httpweb->AddHeaderManual("Content-Type: application/x-www-form-urlencoded");
	user->httpweb->AddHeaderManual(URL_DEFAULT_ORIGIN);
	oss.str("");
	oss << URL_DEFAULT_REFERERBASE << data->srid;
	user->httpweb->AddHeaderManual(oss.str().c_str());
	int ret = toollib::HttpPostEx(user->curlweb, user->httpweb);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	rapidjson::Document doc;
	doc.Parse(user->httpweb->recv_data.c_str());
	if (!doc.IsObject() || !doc.HasMember("code") || !doc["code"].IsInt()) {
		return BILIRET::JSON_ERROR;
	}

	// 0成功 -400已领取 -500系统繁忙
	int icode = doc["code"].GetInt();
	if (icode) {
		// 检查是否被封禁
		if (icode == 400) {
			user->SetBanned();
		}
		std::string tmpstr;
		if (doc.HasMember("message") && doc["message"].IsString()) {
			tmpstr = doc["message"].GetString();
		}
		BOOST_LOG_SEV(g_logger::get(), info) << "[User" << user->fileid << "] "
			<< "APIWebv3SmallTV: " << tmpstr;
		return BILIRET::NOFAULT;
	}
	BOOST_LOG_SEV(g_logger::get(), info) << "[User" << user->fileid << "] "
		<< "APIWebv3SmallTV: Success ";

	return BILIRET::NOFAULT;
}

// 客户端经验心跳
BILIRET apibl::APIAndHeart(const std::shared_ptr<user_info> &user) {
	user->httpapp->url = URL_LIVEAPI_HEAD + "/mobile/userOnlineHeart?";
	std::ostringstream oss;
	oss << "access_key=" << user->tokena
		<< "&actionKey=appkey&appkey=" << APP_KEY
		<< "&build=" << PARAM_BUILD
		<< "&device=android&mobi_app=android&platform=android&ts=" << GetTimeStamp();
	std::string sign;
	GetMD5Sign(oss.str().c_str(), sign);
	oss << "&sign=" << sign;
	user->httpapp->url += oss.str();
	user->httpapp->send_data = "{'roomid': 23058, 'scale': 'xhdpi'}";
	user->httpapp->ClearHeader();
	int ret = toollib::HttpPostEx(user->curlapp, user->httpapp);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	rapidjson::Document doc;
	doc.Parse(user->httpapp->recv_data.c_str());
	if (!doc.IsObject() || !doc.HasMember("code") || !doc["code"].IsInt()) {
		return BILIRET::JSON_ERROR;
	}
	ret = doc["code"].GetInt();
	if (ret) {
		BOOST_LOG_SEV(g_logger::get(), warning) << "[User" << user->fileid << "] "
			<< "APIAndHeart: " << user->httpweb->recv_data;
		return BILIRET::JSON_ERROR;
	}
	BOOST_LOG_SEV(g_logger::get(), info) << "[User" << user->fileid << "] "
		<< "APIAndHeart: OK";

	return BILIRET::NOFAULT;
}

BILIRET apibl::APIAndSilverTask(std::shared_ptr<user_info>& user) {
	user->httpapp->url = URL_LIVEAPI_HEAD + "/mobile/freeSilverCurrentTask";
	std::ostringstream oss;
	oss << "access_key=" << user->tokena
		<< "&actionKey=appkey&appkey=" << APP_KEY
		<< "&build=" << PARAM_BUILD
		<< "&device=android&mobi_app=android&platform=android&ts=" << GetTimeStamp();
	std::string sign;
	GetMD5Sign(oss.str().c_str(), sign);
	oss << "&sign=" << sign;
	user->httpapp->send_data = oss.str();
	user->httpapp->ClearHeader();
	int ret = toollib::HttpPostEx(user->curlapp, user->httpapp);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}
	// 领完为-10017

	rapidjson::Document doc;
	doc.Parse(user->httpapp->recv_data.c_str());
	if (!doc.IsObject() || !doc.HasMember("code")) {
		return BILIRET::JSON_ERROR;
	}
	ret = doc["code"].GetInt();
	if (ret != 0) {
		BOOST_LOG_SEV(g_logger::get(), info) << "[User" << user->fileid << "] "
			<< "APIAndSilverTask: No new task.";
		user->silver_deadline = -1;
		return BILIRET::NOFAULT;
	}
	user->silver_minute = doc["data"]["minute"].GetInt();
	user->silver_amount = doc["data"]["silver"].GetInt();
	user->silver_start = doc["data"]["time_start"].GetInt();
	user->silver_end = doc["data"]["time_end"].GetInt();
	user->silver_deadline = user->silver_minute;
	BOOST_LOG_SEV(g_logger::get(), info) << "[User" << user->fileid << "] "
		<< "APIAndSilverTask: Wait:" << user->silver_minute << " Amount:" << user->silver_amount;

	return BILIRET::NOFAULT;
}

BILIRET apibl::APIAndSilverAward(std::shared_ptr<user_info> &user) {
	user->httpapp->url = URL_LIVEAPI_HEAD + "/mobile/freeSilverAward";
	std::ostringstream oss;
	oss << "access_key=" << user->tokena
		<< "&actionKey=appkey&appkey=" << APP_KEY
		<< "&build=" << PARAM_BUILD
		<< "&device=android&mobi_app=android&platform=android&time_end=" << user->silver_end
		<< "&time_start=" << user->silver_start
		<< "&ts=" << GetTimeStamp();
	std::string sign;
	GetMD5Sign(oss.str().c_str(), sign);
	oss << "&sign=" << sign;
	user->httpapp->send_data = oss.str();
	user->httpapp->ClearHeader();
	int ret = toollib::HttpPostEx(user->curlapp, user->httpapp);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	rapidjson::Document doc;
	doc.Parse(user->httpapp->recv_data.c_str());
	if (!doc.IsObject() || !doc.HasMember("code")) {
		return BILIRET::JSON_ERROR;
	}
	ret = doc["code"].GetInt();
	if (ret) {
		BOOST_LOG_SEV(g_logger::get(), info) << "[User" << user->fileid << "] "
			<< "APIAndSilverAward: Error.";
		user->silver_deadline = 0;
		return BILIRET::NOFAULT;
	}

	if (doc["data"]["isEnd"].GetInt()) {
		BOOST_LOG_SEV(g_logger::get(), info) << "[User" << user->fileid << "] "
			<< "APIAndSilverAward: Finish.";
		user->silver_deadline = -1;
	}
	else {
		BOOST_LOG_SEV(g_logger::get(), info) << "[User" << user->fileid << "] "
			<< "APIAndSilverAward: Success.";
		user->silver_deadline = 0;
	}
	return BILIRET::NOFAULT;
}

BILIRET apibl::APIAndGetKey(const std::shared_ptr<user_info>& user, std::string & psd) {
	BOOST_LOG_SEV(g_logger::get(), info) << "[User] Get APP RSA key...";
	user->httpapp->url = "https://passport.bilibili.com/api/oauth2/getKey";
	std::ostringstream oss;
	oss << "appkey=" << APP_KEY
		<< "&build=" << PARAM_BUILD
		<< "&mobi_app=android&platform=android&ts=" << GetTimeStamp();
	std::string sign;
	GetMD5Sign(oss.str().c_str(), sign);
	oss << "&sign=" << sign;
	user->httpapp->send_data = oss.str();
	user->httpapp->ClearHeader();
	user->httpapp->AddHeaderManual("Content-Type: application/x-www-form-urlencoded; charset=utf-8");
	int ret = toollib::HttpPostEx(user->curlapp, user->httpapp);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	rapidjson::Document doc;
	doc.Parse(user->httpapp->recv_data.c_str());
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

BILIRET apibl::APIAndv1StormJoin(
	std::shared_ptr<user_info> &user,
	std::shared_ptr<BILI_LOTTERYDATA> data
) {
	user->httpapp->url = URL_LIVEAPI_HEAD + "/lottery/v1/Storm/join";
	std::ostringstream oss;
	oss << "access_key=" << user->tokena
		<< "&actionKey=appkey&appkey=" << APP_KEY
		<< "&build=" << PARAM_BUILD
		<< "&device=android&id=" << data->rrid
		<< "&mobi_app=android&platform=android&ts=" << GetTimeStamp();
	std::string sign;
	GetMD5Sign(oss.str().c_str(), sign);
	oss << "&sign=" << sign;
	user->httpapp->send_data = oss.str();
	user->httpapp->ClearHeader();
	user->httpapp->AddHeaderManual("Content-Type: application/x-www-form-urlencoded; charset=utf-8");
	int ret = toollib::HttpPostEx(user->curlapp, user->httpapp);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	rapidjson::Document doc;
	doc.Parse(user->httpapp->recv_data.c_str());
	if (!doc.IsObject() || !doc.HasMember("code") || !doc["code"].IsInt()) {
		return BILIRET::JSON_ERROR;
	}
	ret = doc["code"].GetInt();
	std::string msg;
	if (ret) {
		msg = doc["msg"].GetString();
		BOOST_LOG_SEV(g_logger::get(), info) << "[User" << user->fileid << "] "
			<< "APIAndv1StormJoin: " << ret << " " << msg;
		// 检查是否被封禁
		if (ret == 400) {
			if (user->CheckBanned(msg)) {
				return BILIRET::NOFAULT;
			}
		}
		return BILIRET::JOINEVENT_FAILED;
	}
	msg = doc["data"]["mobile_content"].GetString();
	BOOST_LOG_SEV(g_logger::get(), info) << "[User" << user->fileid << "] "
		<< "APIAndv1StormJoin: " << msg;

	return BILIRET::NOFAULT;
}

BILIRET apibl::APIAndv2Login(std::shared_ptr<user_info>& user, std::string username, std::string password, std::string captcha) {
	BOOST_LOG_SEV(g_logger::get(), info) << "[User] Logging in by app api...";
	user->httpapp->url = "https://passport.bilibili.com/api/v2/oauth2/login";
	std::ostringstream oss;
	oss << "appkey=" << APP_KEY
		<< "&build=" << PARAM_BUILD;
	if (captcha != "") {
		oss << "&captcha=" << captcha;
	}
	oss << "&mobi_app=android&password=" << toollib::UrlEncode(password.c_str())
		<< "&platform=android&ts=" << GetTimeStamp()
		<< "&username=" << username;
	std::string sign;
	GetMD5Sign(oss.str().c_str(), sign);
	oss << "&sign=" << sign;
	user->httpapp->send_data = oss.str();
	user->httpapp->ClearHeader();
	user->httpapp->AddHeaderManual("Content-Type: application/x-www-form-urlencoded; charset=utf-8");
	int ret = toollib::HttpPostEx(user->curlapp, user->httpapp);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	rapidjson::Document doc;
	doc.Parse(user->httpapp->recv_data.c_str());
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
	user->uid = doc["data"]["token_info"]["mid"].GetInt();
	user->tokena = doc["data"]["token_info"]["access_token"].GetString();
	user->tokenr = doc["data"]["token_info"]["refresh_token"].GetString();
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
	toollib::HttpImportCookie(user->curlweb, oss.str());

	return BILIRET::NOFAULT;
}
