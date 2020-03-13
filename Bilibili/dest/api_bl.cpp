#include "api_bl.h" 

#include <fstream>
#include <sstream>
#include <rapidjson/document.h>

#include "logger/log.h"
#include "utility/md5.h"
#include "utility/platform.h"
#include "utility/sslex.h"
#include "utility/strconvert.h"
using namespace apibl;
using namespace toollib;

#ifdef GetObject
#undef GetObject
#endif

static const char UALogin[] = "Mozilla/5.0 BiliDroid/5.51.1 (bbcallen@gmail.com) os/android model/HUAWEI EVR-AN00 mobi_app/android build/5511400 channel/yingyongbao innerVer/5511400 osVer/5.1.1 network/2";

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
    auto &obj = doc["data"];
	if (!obj.HasMember("double_watch_info") || !obj["double_watch_info"].IsObject()) {
		return BILIRET::JSON_ERROR;
	}
	obj = obj["double_watch_info"];
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

	// 0签到成功 -1已签到
	rapidjson::Document doc;
	doc.Parse(user->httpweb->recv_data.c_str());
	if (!doc.IsObject() || !doc.HasMember("code") || !doc["code"].IsInt() ||
		!doc.HasMember("message") || !doc["message"].IsString()) {
		return BILIRET::JSON_ERROR;
	}
	if (doc["code"].GetInt()) {
		BOOST_LOG_SEV(g_logger::get(), info) << "[User" << user->fileid << "] "
			<< "APIWebSign: " << doc["message"].GetString();
		return BILIRET::NOFAULT;
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
	oss << URL_LIVEAPI_HEAD << "/relation/v1/feed/heartBeat?_=" << GetTimeStampM();
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

BILIRET apibl::APIWebv1GiftBag(
   const std::shared_ptr<user_info>& user,
   unsigned flag
) {
    std::ostringstream oss;
    oss << URL_LIVEAPI_HEAD
        << "/xlive/web-room/v1/gift/bag_list?"
        << "t=" << GetTimeStampM()
        << "&room_id=23058";
	user->httpweb->url = oss.str();
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
			<< "APIWebv1GiftBag: Get bag info failed.";
		return BILIRET::JSON_ERROR;
	}
	printf("  Current bag info: \n");
	int curtime, expiretime;
	std::string giftname;
	curtime = GetTimeStamp();
	rapidjson::Value &datalist = doc["data"]["list"];
	unsigned int i, si = 0;
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
        if (flag) {
            unsigned gift_id = datalist[i]["gift_id"].GetUint();
            unsigned gift_num = datalist[i]["gift_num"].GetUint();
            unsigned bag_id = datalist[i]["bag_id"].GetUint();
            if (gift_id != 1 || gift_num != 3 || ++si > 50) {
                continue;
            }
            apibl::APIWebv2GiftSend(
                user,
                11153765,
                23058,
                gift_id,
                gift_num,
                bag_id
            );
        }
	}

	return BILIRET::NOFAULT;
}

BILIRET apibl::APIWebv2GiftSend(
   const std::shared_ptr<user_info>& user,
   unsigned ruid,
   unsigned room,
   unsigned gift_id,
   unsigned gift_num,
   unsigned bag_id
) {
    user->httpweb->url = URL_LIVEAPI_HEAD + "/gift/v2/live/bag_send";
    std::ostringstream oss;
    oss << "uid=" << user->uid
        << "&gift_id=" << gift_id
        << "&ruid=" << ruid
        << "&send_ruid=" << 0
        << "&gift_num=" << gift_num
        << "&bag_id=" << bag_id
        << "&platform=pc"
        << "&biz_code=live"
        << "&biz_id=" << room
        << "&rnd=" << GetTimeStamp()
        << "&storm_beat_id=" << 0
        << "&metadata="
        << "&price=" << 0
        << "&csrf_token=" << user->tokenjct
        << "&csrf=" << user->tokenjct
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

    rapidjson::Document doc;
    doc.Parse(user->httpweb->recv_data.c_str());
    BOOST_LOG_SEV(g_logger::get(), warning) << "[User" << user->fileid << "] "
        << "APIWebv2GiftSend: " << doc["code"].GetInt()
        << " " << doc["msg"].GetString();

    return BILIRET::NOFAULT;
}

BILIRET apibl::APIWebv1RoomEntry(const user_info *user, unsigned room) {
	user->httpweb->url = URL_LIVEAPI_HEAD + "/room/v1/Room/room_entry_action";
	std::ostringstream oss;
	oss << "room_id=" << room
		<< "&platform=pc"
		<< "&csrf_token=" << user->tokenjct
		<< "&csrf=" << user->tokenjct
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
	user_info *user,
	std::shared_ptr<BILI_LOTTERYDATA> data,
	std::string code,
	std::string token
) {
	user->httpweb->url = URL_LIVEAPI_HEAD + "/xlive/lottery-interface/v1/storm/Join";
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
	if (ret) {
		if (doc.HasMember("msg")) {
			msg = doc["msg"].GetString();
		}
		else if (doc.HasMember("message")) {
			msg = doc["message"].GetString();
		}
		else {
			msg = user->httpweb->recv_data;
		}
		BOOST_LOG_SEV(g_logger::get(), info) << "[User" << user->fileid << "] "
			<< "APIWebv1StormJoin: " << ret << " " << msg;
		if (user->CheckBanned(msg)) {
			// 检查是否被封禁
			return BILIRET::NOFAULT;
		}
		if (msg.find(u8"你错过了奖励") != std::string::npos) {
			// 未抽中 可多次参与抽奖
			return BILIRET::JOIN_AGAIN;
		}
		// 抽奖过期或其它未知情况
		return BILIRET::NOFAULT;
	}
	// 抽中的返回值为0
	msg = doc["data"]["mobile_content"].GetString();
	BOOST_LOG_SEV(g_logger::get(), info) << "[User" << user->fileid << "] "
		<< "APIWebv1StormJoin: " << msg;

	return BILIRET::NOFAULT;
}

BILIRET apibl::APIWebv5SmalltvJoin(
	user_info* user,
	std::shared_ptr<BILI_LOTTERYDATA> data
) {
	user->httpweb->url = URL_LIVEAPI_HEAD + "/xlive/lottery-interface/v5/smalltv/join";
	std::ostringstream oss;
	oss << "id=" << data->loid
		<< "&roomid=" << data->rrid
		<< "&type=" << data->type
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
	/*
	{
		"code": 0,
		"data": {
			"id": 540620,
			"award_id": 1,
			"award_type": 0,
			"award_num": 5,
			"award_image": "http://i0.hdslb.com/bfs/live/ .png",
			"award_name": "辣条",
			"award_text": "",
			"award_ex_time": 1577721600
		},
		"message": "",
		"msg": ""
	}
	*/
	rapidjson::Document doc;
	doc.Parse(user->httpweb->recv_data.c_str());
	if (!doc.IsObject() || !doc.HasMember("code") || !doc["code"].IsInt()) {
		return BILIRET::JSON_ERROR;
	}

	int icode = doc["code"].GetInt();
	if (icode) {
		std::string msg;
		if (doc.HasMember("msg")) {
			msg = doc["msg"].GetString();
		}
		else if (doc.HasMember("message")) {
			msg = doc["message"].GetString();
		}
		else {
			msg = user->httpweb->recv_data;
		}
		// 检查是否被封禁
		user->CheckBanned(msg);
		BOOST_LOG_SEV(g_logger::get(), info) << "[User" << user->fileid << "] "
			<< "APIWebv5SmalltvJoin: " << icode << ' ' << msg;
		return BILIRET::NOFAULT;
	}
	BOOST_LOG_SEV(g_logger::get(), info) << "[User" << user->fileid << "] "
		<< "APIWebv5SmalltvJoin: " << data->loid << ' ' << doc["data"]["award_name"].GetString()
		<< u8"×" << doc["data"]["award_num"].GetInt();

	return BILIRET::NOFAULT;
}

BILIRET apibl::APIWebv3GuardJoin(
	user_info *user,
	std::shared_ptr<BILI_LOTTERYDATA> data
) {
	user->httpweb->url = URL_LIVEAPI_HEAD + "/xlive/lottery-interface/v3/guard/join";
	std::ostringstream oss;
	oss << "id=" << data->loid
		<< "&roomid=" << data->rrid
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

	int icode = doc["code"].GetInt();
	std::string msg;
	if (doc.HasMember("msg")) {
		msg = doc["msg"].GetString();
	}
	else if (doc.HasMember("message")) {
		msg = doc["message"].GetString();
	}
	else {
		msg = user->httpweb->recv_data;
	}
	if (icode) {
		// 检查是否被封禁
		user->CheckBanned(msg);
	}
	BOOST_LOG_SEV(g_logger::get(), info) << "[User" << user->fileid << "] "
		<< "APIWebv3GuardJoin: " << icode << ' ' << msg;

	return BILIRET::NOFAULT;
}

BILIRET apibl::APIWebv2PKJoin(
	user_info *user,
	std::shared_ptr<BILI_LOTTERYDATA> data
) {
	user->httpweb->url = URL_LIVEAPI_HEAD + "/xlive/lottery-interface/v2/pk/join";
	std::ostringstream oss;
	oss << "id=" << data->loid
		<< "&roomid=" << data->rrid
		<< "&type=" << data->type
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

	int icode = doc["code"].GetInt();
	if (icode) {
		std::string msg;
		if (doc.HasMember("msg")) {
			msg = doc["msg"].GetString();
		}
		else if (doc.HasMember("message")) {
			msg = doc["message"].GetString();
		}
		else {
			msg = user->httpweb->recv_data;
		}
		// 检查是否被封禁
		user->CheckBanned(msg);
		BOOST_LOG_SEV(g_logger::get(), info) << "[User" << user->fileid << "] "
			<< "APIWebv2PKJoin: " << icode << ' ' << msg;
		return BILIRET::NOFAULT;
	}
	BOOST_LOG_SEV(g_logger::get(), info) << "[User" << user->fileid << "] "
		<< "APIWebv2PKJoin award: " << doc["data"]["award_text"].GetString();

	return BILIRET::NOFAULT;
}

BILIRET apibl::APIWebv1DanmuJoin(
	user_info *user,
	std::shared_ptr<BILI_LOTTERYDATA> data
) {
	user->httpweb->url = URL_LIVEAPI_HEAD + "/xlive/lottery-interface/v1/Danmu/Join";
	std::ostringstream oss;
	oss << "id=" << data->loid
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

	/*
	{
		"code": 0,
		"data": {
			"code": 0,
			"message": ""
		},
		"message": "发送成功",
		"msg": "发送成功"
	}
	*/

	rapidjson::Document doc;
	doc.Parse(user->httpweb->recv_data.c_str());
	if (!doc.IsObject() || !doc.HasMember("code") || !doc["code"].IsInt()) {
		return BILIRET::JSON_ERROR;
	}

	std::string msg;
	if (doc.HasMember("message")) {
		msg = doc["message"].GetString();
	}
	else {
		msg = user->httpweb->recv_data;
	}
	BOOST_LOG_SEV(g_logger::get(), info) << "[User" << user->fileid << "] "
		<< "APIWebv1DanmuJoin: " << doc["code"].GetInt()
		<< ' ' << msg;

	return BILIRET::NOFAULT;
}

BILIRET apibl::APIWebv1AnchorJoin(
	user_info *user,
	std::shared_ptr<BILI_LOTTERYDATA> data
) {
	user->httpweb->url = URL_LIVEAPI_HEAD + "/xlive/lottery-interface/v1/Anchor/Join";
	std::ostringstream oss;
	oss << "id=" << data->loid;
	if (data->gift_id) {
		oss << "&gift_id=" << data->gift_id
			<< "&gift_num=" << data->gift_num;
	}
	oss << "&platform=pc"
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

	int icode = doc["code"].GetInt();
	if (icode) {
		std::string msg;
		if (doc.HasMember("msg")) {
			msg = doc["msg"].GetString();
		}
		else if (doc.HasMember("message")) {
			msg = doc["message"].GetString();
		}
		else {
			msg = user->httpweb->recv_data;
		}
		// 检查是否被封禁
		user->CheckBanned(msg);
		BOOST_LOG_SEV(g_logger::get(), info) << "[User" << user->fileid << "] "
			<< "APIWebv1AnchorJoin: " << icode << ' ' << msg;
		return BILIRET::NOFAULT;
	}
	BOOST_LOG_SEV(g_logger::get(), info) << "[User" << user->fileid << "] "
		<< "APIWebv1AnchorJoin award: OK";

	return BILIRET::NOFAULT;
}

BILIRET apibl::APIAndGetKey(
	const std::shared_ptr<user_info>& user, 
	std::string & psd)
{
	BOOST_LOG_SEV(g_logger::get(), info) << "[User] Get APP RSA key...";
	user->httpapp->url = "https://passport.bilibili.com/api/oauth2/getKey";
	std::string statistics = "{\"appId\":1,\"platform\":3,\"version\":\"5.51.1\",\"abtest\":\"\"}";
	std::ostringstream oss;
	oss << "appkey=" << APP_KEY
		<< "&build=" << "5511400"
		<< "&channel=yingyongbao"
		<< "&mobi_app=android"
		<< "&platform=android"
		<< "&statistics=" << toollib::UrlEncodeAnd(statistics)
		<< "&ts=" << GetTimeStamp();
	std::string sign;
	GetMD5Sign(oss.str().c_str(), sign);
	oss << "&sign=" << sign;
	user->httpapp->send_data = oss.str();
	user->httpapp->ClearHeader();
	user->httpapp->AddHeaderManual("Content-Type: application/x-www-form-urlencoded; charset=utf-8");
	int ret = toollib::HttpPostEx(user->curlapp, user->httpapp, UALogin);
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

BILIRET apibl::APIAndv3Login(
	std::shared_ptr<user_info>& user, 
	std::string username, 
	std::string password,
	std::string challenge,
	std::string validate)
{
	BOOST_LOG_SEV(g_logger::get(), info) << "[User] Logging in by app api...";
	user->httpapp->url = "https://passport.bilibili.com/api/v3/oauth2/login";
	std::string statistics = "{\"appId\":1,\"platform\":3,\"version\":\"5.51.1\",\"abtest\":\"\"}";
	std::ostringstream oss;
	oss << "appkey=" << APP_KEY
		<< "&bili_local_id=" << user->phoneDeviceID
		<< "&build=" << "5511400"
		<< "&buvid=" << user->phoneBuvid;
	if (challenge != "") {
		oss << "&challenge=" << challenge;
	}
	oss << "&channel=yingyongbao"
		<< "&device_id=" << user->phoneDeviceID
		<< "&device_name=" << user->phoneDeviceName
		<< "&device_platform=" << user->phoneDevicePlatform
		<< "&local_id=" << user->phoneBuvid
		<< "&mobi_app=android"
		<< "&password=" << toollib::UrlEncodeAnd(password.c_str())
		<< "&platform=android";
	if (challenge != "") {
		oss << "&seccode=" << validate << toollib::UrlEncodeAnd("|jordan");
	}
	oss << "&statistics=" << toollib::UrlEncodeAnd(statistics)
		<< "&ts=" << GetTimeStamp()
		<< "&username=" << username;
	if (challenge != "") {
		oss << "&validate=" << validate;
	}
	std::string sign;
	GetMD5Sign(oss.str().c_str(), sign);
	oss << "&sign=" << sign;
	user->httpapp->send_data = oss.str();
	user->httpapp->ClearHeader();
	user->httpapp->AddHeaderManual("Content-Type: application/x-www-form-urlencoded; charset=utf-8");
	int ret = toollib::HttpPostEx(user->curlapp, user->httpapp, UALogin);
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
		std::string url = doc["data"]["url"].GetString();
		BOOST_LOG_SEV(g_logger::get(), info) << "[User" << user->fileid << "] "
			<< "APIAndv3Login: " << url;
		return BILIRET::LOGIN_NEEDVERIFY;
	}
	if (ret) {
		BOOST_LOG_SEV(g_logger::get(), info) << "[User" << user->fileid << "] "
			<< "APIAndv3Login: " << ret << ' ' << doc["message"].GetString();
		return BILIRET::LOGIN_PASSWORDWRONG;
	}

	// 登录成功
	user->uid = doc["data"]["token_info"]["mid"].GetInt();
	user->tokena = doc["data"]["token_info"]["access_token"].GetString();
	user->tokenr = doc["data"]["token_info"]["refresh_token"].GetString();
	oss.str("");
	rapidjson::Value& cookie_list = doc["data"]["cookie_info"]["cookies"];
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

BILIRET apibl::APIAndv1RoomEntry(const std::shared_ptr<user_info>& user, unsigned room) {
	user->httpapp->url = URL_LIVEAPI_HEAD + "/room/v1/Room/room_entry_action";
	std::ostringstream oss;
	oss << "access_key=" << user->tokena
		<< "&actionKey=appkey"
		<< "&appkey=" << APP_KEY
		<< "&build=" << PARAM_BUILD
		<< "&device=android"
		<< "&jumpFrom=" << 28000
		<< "&mobi_app=android"
		<< "&platform=android"
		<< "&room_id=" << room
		<< "&ts=" << GetTimeStamp();
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

	return BILIRET::NOFAULT;
}

// 客户端经验心跳
BILIRET apibl::APIAndv1Heart(const std::shared_ptr<user_info> &user) {
	user->httpapp->url = URL_LIVEAPI_HEAD + "/heartbeat/v1/OnLine/mobileOnline?";
	std::ostringstream oss;
	oss << "access_key=" << user->tokena
		<< "&actionKey=appkey"
		<< "&appkey=" << APP_KEY
		<< "&build=" << PARAM_BUILD
		<< "&device=android"
		<< "&mobi_app=android"
		<< "&platform=android"
		<< "&ts=" << GetTimeStamp();
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
			<< "APIAndv1Heart: " << user->httpweb->recv_data;
		return BILIRET::JSON_ERROR;
	}
	BOOST_LOG_SEV(g_logger::get(), info) << "[User" << user->fileid << "] "
		<< "APIAndv1Heart: OK";

	return BILIRET::NOFAULT;
}

BILIRET apibl::APIAndv1SilverTask(std::shared_ptr<user_info>& user) {
	user->httpapp->url = URL_LIVEAPI_HEAD + "/lottery/v1/SilverBox/getCurrentTask?";
	std::ostringstream oss;
	oss << "access_key=" << user->tokena
		<< "&actionKey=appkey"
		<< "&appkey=" << APP_KEY
		<< "&build=" << PARAM_BUILD
		<< "&device=android"
		<< "&mobi_app=android"
		<< "&platform=android"
		<< "&ts=" << GetTimeStamp();
	std::string sign;
	GetMD5Sign(oss.str().c_str(), sign);
	oss << "&sign=" << sign;
	user->httpapp->url += oss.str();
	user->httpapp->ClearHeader();
	int ret = toollib::HttpGetEx(user->curlapp, user->httpapp);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	rapidjson::Document doc;
	doc.Parse(user->httpapp->recv_data.c_str());
	if (!doc.IsObject() || !doc.HasMember("code")) {
		return BILIRET::JSON_ERROR;
	}
	ret = doc["code"].GetInt();
	if (ret != 0) {
		BOOST_LOG_SEV(g_logger::get(), info) << "[User" << user->fileid << "] "
			<< "APIAndv1SilverTask: No new task.";
		user->silver_deadline = -1;
		return BILIRET::NOFAULT;
	}
	user->silver_minute = doc["data"]["minute"].GetInt();
	user->silver_amount = doc["data"]["silver"].GetInt();
	user->silver_start = doc["data"]["time_start"].GetInt();
	user->silver_end = doc["data"]["time_end"].GetInt();
	user->silver_deadline = user->silver_minute;
	BOOST_LOG_SEV(g_logger::get(), info) << "[User" << user->fileid << "] "
		<< "APIAndv1SilverTask: Wait:" << user->silver_minute << " Amount:" << user->silver_amount;

	return BILIRET::NOFAULT;
}

BILIRET apibl::APIAndv1SilverAward(std::shared_ptr<user_info> &user) {
	user->httpapp->url = URL_LIVEAPI_HEAD + "/lottery/v1/SilverBox/getAward?";
	std::ostringstream oss;
	oss << "access_key=" << user->tokena
		<< "&actionKey=appkey"
		<< "&appkey=" << APP_KEY
		<< "&build=" << PARAM_BUILD
		<< "&device=android"
		<< "&mobi_app=android"
		<< "&platform=android"
		<< "&time_end=" << user->silver_end
		<< "&time_start=" << user->silver_start
		<< "&ts=" << GetTimeStamp();
	std::string sign;
	GetMD5Sign(oss.str().c_str(), sign);
	oss << "&sign=" << sign;
	user->httpapp->url += oss.str();
	user->httpapp->ClearHeader();
	int ret = toollib::HttpGetEx(user->curlapp, user->httpapp);
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
			<< "APIAndv1SilverAward: Error.";
		user->silver_deadline = 0;
		return BILIRET::NOFAULT;
	}

	if (doc["data"]["isEnd"].GetInt()) {
		BOOST_LOG_SEV(g_logger::get(), info) << "[User" << user->fileid << "] "
			<< "APIAndv1SilverAward: Finish.";
		user->silver_deadline = -1;
	}
	else {
		BOOST_LOG_SEV(g_logger::get(), info) << "[User" << user->fileid << "] "
			<< "APIAndv1SilverAward: Success.";
		user->silver_deadline = 0;
	}
	return BILIRET::NOFAULT;
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
		if (ret == 400) {
			if (user->CheckBanned(msg)) {
				// 检查是否被封禁
				return BILIRET::NOFAULT;
			}
			if (msg.find(u8"你错过了奖励") != std::string::npos) {
				// 未抽中 可多次参与抽奖
				return BILIRET::JOIN_AGAIN;
			}
		}
		// 抽奖过期或其它未知情况
		return BILIRET::NOFAULT;
	}
	msg = doc["data"]["mobile_content"].GetString();
	BOOST_LOG_SEV(g_logger::get(), info) << "[User" << user->fileid << "] "
		<< "APIAndv1StormJoin: " << msg;

	return BILIRET::NOFAULT;
}

BILIRET apibl::APIAndv1PKJOIN(
	std::shared_ptr<user_info>& user, 
	std::shared_ptr<BILI_LOTTERYDATA> data
) {
	user->httpapp->url = URL_LIVEAPI_HEAD + "/xlive/lottery-interface/v1/pk/join";
	std::ostringstream oss;
	oss << "access_key=" << user->tokena
		<< "&actionKey=appkey"
		<< "&appkey=" << APP_KEY
		<< "&build=" << PARAM_BUILD
		<< "&device=android"
		<< "&id=" << data->loid
		<< "&mobi_app=android"
		<< "&platform=android"
		<< "&roomid=" << data->rrid
		<< "&ts=" << GetTimeStamp();
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
			<< "APIAndv1PKJOIN: " << tmpstr;
		return BILIRET::NOFAULT;
	}
	BOOST_LOG_SEV(g_logger::get(), info) << "[User" << user->fileid << "] "
		<< "APIAndv1PKJOIN award: " << doc["data"]["award_text"].GetString();

	return BILIRET::NOFAULT;
}

BILIRET apibl::APIAndv2LotteryJoin(
	std::shared_ptr<user_info>& user,
	std::shared_ptr<BILI_LOTTERYDATA> data
) {
	user->httpapp->url = URL_LIVEAPI_HEAD + "/lottery/v2/Lottery/join";
	std::ostringstream oss;
	oss << "access_key=" << user->tokena
		<< "&actionKey=appkey"
		<< "&appkey=" << APP_KEY
		<< "&build=" << PARAM_BUILD
		<< "&device=android"
		<< "&id=" << data->loid
		<< "&mobi_app=android"
		<< "&platform=android"
		<< "&roomid=" << data->rrid
		<< "&ts=" << GetTimeStamp()
		<< "&type=" << data->type;
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
		<< "APIAndv2LotteryJoin: " << tmpstr;

	return BILIRET::NOFAULT;
}

BILIRET apibl::APIAndv4SmallTV(
	std::shared_ptr<user_info>& user,
	std::shared_ptr<BILI_LOTTERYDATA> data
) {
	user->httpapp->url = URL_LIVEAPI_HEAD + "/gift/v4/smalltv/getAward";
	std::ostringstream oss;
	oss << "access_key=" << user->tokena
		<< "&actionKey=appkey"
		<< "&appkey=" << APP_KEY
		<< "&build=" << PARAM_BUILD
		<< "&device=android"
		<< "&mobi_app=android"
		<< "&platform=android"
		<< "&raffleId=" << data->loid
		<< "&roomid=" << data->rrid
		<< "&ts=" << GetTimeStamp()
		<< "&type=" << data->type;
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
			<< "APIAndv4SmallTV: " << tmpstr;
		return BILIRET::NOFAULT;
	}
	BOOST_LOG_SEV(g_logger::get(), info) << "[User" << user->fileid << "] "
		<< "APIAndv4SmallTV award: " << doc["data"]["gift_name"].GetString();

	return BILIRET::NOFAULT;
}
