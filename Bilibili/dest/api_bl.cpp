#include "api_bl.h" 

#include <fstream>
#include <sstream>
#include "rapidjson/document.h"

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

int GetMD5Sign(const char *in, std::string &sign) {
	std::string str = in;
	str += APP_SECRET;
	sign = Encode_MD5(str);
	return 0;
}

BILIRET apibl::APIShowCPInfo(const std::shared_ptr<user_info>& user) {
	std::ostringstream oss;
	oss << "https://show.bilibili.com/api/activity/index/cp/info?&_="
		<< GetTimeStampM();
	user->httpweb->url = oss.str();
	user->httpweb->ClearHeader();
	user->httpweb->AddHeaderManual("Accept: application/json, text/plain, */*");
	user->httpweb->AddHeaderManual("Origin: https://mall.bilibili.com");
	user->httpweb->AddHeaderManual("Referer: https://mall.bilibili.com/activities/activity2019626.html");
	int ret = toollib::HttpGetEx(user->curlweb, user->httpweb);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	rapidjson::Document doc;
	doc.Parse(user->httpweb->recv_data.c_str());
	if (!doc.IsObject() || !doc.HasMember("errno") || !doc["errno"].IsInt() || doc["errno"].GetInt()) {
		BOOST_LOG_SEV(g_logger::get(), error) << "[User" << user->fileid << "] "
			<< "APIShowCPInfo: " << user->httpweb->recv_data;
		return BILIRET::JSON_ERROR;
	}
	user->ten_cp_id = doc["data"]["cpId"].GetUint64();
	user->ten_cp_token = doc["data"]["token"].GetString();
	// 获取见证者ID
	user->ten_team_list.clear();
	if (user->ten_cp_id) {
		rapidjson::Value &witlist = doc["data"]["fansMember"];
		for (unsigned i = 0; i < witlist.Size(); i++) {
			std::string sid = witlist[i]["id"].GetString();
			user->ten_team_list.push_back(atoi(sid.c_str()));
		}
	}
	// 重置账户级别
	user->ten_self_level = TEN_DEFAULT_LEVEL;

	BOOST_LOG_SEV(g_logger::get(), info) << "[User" << user->fileid << "] "
		<< "APIShowCPInfo cpid: " << user->ten_cp_id;

	return BILIRET::NOFAULT;
}

BILIRET apibl::APIShowCPAgree(const std::shared_ptr<user_info>& user, std::string token) {
	user->httpweb->url = "https://show.bilibili.com/api/activity/index/cp/agree";
	std::ostringstream oss;
	oss << "{\"token\":\"" << token << "\"}";
	user->httpweb->send_data = oss.str();
	user->httpweb->ClearHeader();
	user->httpweb->AddHeaderManual("Accept: application/json, text/plain, */*");
	user->httpweb->AddHeaderManual("Content-Type: application/json;charset=UTF-8");
	user->httpweb->AddHeaderManual("Origin: https://mall.bilibili.com");
	oss.str("");
	oss << "Referer: https://mall.bilibili.com/activities/invitation.html?token=" << token;
	user->httpweb->AddHeaderManual(oss.str().c_str());
	int ret = toollib::HttpPostEx(user->curlweb, user->httpweb);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	rapidjson::Document doc;
	doc.Parse(user->httpweb->recv_data.c_str());
	if (!doc.IsObject() || !doc.HasMember("errno") || !doc["errno"].IsInt()) {
		BOOST_LOG_SEV(g_logger::get(), error) << "[User" << user->fileid << "] "
			<< "APIShowCPAgree: " << user->httpweb->recv_data;
		return BILIRET::JSON_ERROR;
	}
	BOOST_LOG_SEV(g_logger::get(), info) << "[User" << user->fileid << "] "
		<< "APIShowCPAgree: " << doc["msg"].GetString();

	return BILIRET::NOFAULT;
}

BILIRET apibl::APIShowWitDeital(const std::shared_ptr<user_info>& user) {
	std::ostringstream oss;
	oss << "https://show.bilibili.com/api/activity/index/witness/detail?_="
		<< GetTimeStampM();
	user->httpweb->url = oss.str();
	user->httpweb->ClearHeader();
	user->httpweb->AddHeaderManual("Accept: application/json, text/plain, */*");
	user->httpweb->AddHeaderManual("Origin: https://mall.bilibili.com");
	user->httpweb->AddHeaderManual("Referer: https://mall.bilibili.com/activities/friends.html");
	int ret = toollib::HttpGetEx(user->curlweb, user->httpweb);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	rapidjson::Document doc;
	doc.Parse(user->httpweb->recv_data.c_str());
	if (!doc.IsObject() || !doc.HasMember("errno") || !doc["errno"].IsInt() || doc["errno"].GetInt()) {
		BOOST_LOG_SEV(g_logger::get(), error) << "[User" << user->fileid << "] "
			<< "APIShowWitDeital: " << user->httpweb->recv_data;
		return BILIRET::JSON_ERROR;
	}
	user->ten_team_id = doc["data"]["teamId"].GetUint64();
	user->ten_team_empty_num = 0;
	user->ten_team_list.clear();
	rapidjson::Value &witlist = doc["data"]["witnessList"];
	for (unsigned i = 0; i < witlist.Size(); i++) {
		if (witlist[i]["isLocked"].IsTrue()) {
			// 未开启
			continue;
		}
		if (witlist[i]["rebate"].HasMember("num")) {
			// 领取提成
			apibl::APIShowReward(
				user,
				witlist[i]["rebate"]["assocId"].GetString(),
				witlist[i]["rebate"]["taskId"].GetString(),
				witlist[i]["rebate"]["type"].GetUint(),
				TEN_REFERER_FRIEND
			);
		}
		if (witlist[i]["member"].HasMember("id")) {
			// 添加到列表
			user->ten_team_list.push_back(witlist[i]["member"]["id"].GetUint());
		}
		else {
			// 有空余坑位
			user->ten_team_empty_num++;
		}
	}
	BOOST_LOG_SEV(g_logger::get(), info) << "[User" << user->fileid << "] "
		<< "APIShowWitDeital teamId: " << user->ten_team_id << " empty num: " << user->ten_team_empty_num;

	return BILIRET::NOFAULT;
}

BILIRET apibl::APIShowWitJoin(const std::shared_ptr<user_info>& user, long long teamId) {
	user->httpweb->url = "https://show.bilibili.com/api/activity/index/witness/join";
	std::ostringstream oss;
	oss << "{\"teamId\":\"" << teamId << "\"}";
	user->httpweb->send_data = oss.str();
	user->httpweb->ClearHeader();
	user->httpweb->AddHeaderManual("Accept: application/json, text/plain, */*");
	user->httpweb->AddHeaderManual("Content-Type: application/json;charset=UTF-8");
	user->httpweb->AddHeaderManual("Origin: https://mall.bilibili.com");
	oss.str("");
	oss << "Referer: https://mall.bilibili.com/activities/cp.html?teamId=" << teamId;
	user->httpweb->AddHeaderManual(oss.str().c_str());
	int ret = toollib::HttpPostEx(user->curlweb, user->httpweb);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	rapidjson::Document doc;
	doc.Parse(user->httpweb->recv_data.c_str());
	if (!doc.IsObject() || !doc.HasMember("errno") || !doc["errno"].IsInt()) {
		BOOST_LOG_SEV(g_logger::get(), error) << "[User" << user->fileid << "] "
			<< "APIShowWitJoin: " << user->httpweb->recv_data;
		return BILIRET::JSON_ERROR;
	}
	// 600015 活动期间只能参加一个见证团
	// 600020 见证团已经满员
	//        自己的见证团就不用加入啦
	ret = doc["errno"].GetInt();
	if (ret == 600015) {
		user->ten_team_hasjoin = true;
		BOOST_LOG_SEV(g_logger::get(), info) << "[User" << user->fileid << "] "
			<< "APIShowWitJoin: " << doc["msg"].GetString();
		return BILIRET::TEN_TEAM_HASJOIN;
	}
	if (ret == 600020) {
		BOOST_LOG_SEV(g_logger::get(), info) << "[User" << user->fileid << "] "
			<< "APIShowWitJoin: " << doc["msg"].GetString();
		return BILIRET::TEN_TEAM_FULL;
	}
	if (ret) {
		BOOST_LOG_SEV(g_logger::get(), info) << "[User" << user->fileid << "] "
			<< "APIShowWitJoin: " << doc["msg"].GetString();
		return BILIRET::TEN_TEAM_HASJOIN;
	}
	BOOST_LOG_SEV(g_logger::get(), info) << "[User" << user->fileid << "] "
		<< "APIShowWitJoin: " << doc["msg"].GetString();

	return BILIRET::NOFAULT;
}

BILIRET apibl::APIShowSignStatus(const std::shared_ptr<user_info> &user) {
	std::ostringstream oss;
	oss << "https://show.bilibili.com/api/activity/index/sign/detail?taskId=act626-sign&_="
		<< GetTimeStampM();
	user->httpweb->url = oss.str();
	user->httpweb->ClearHeader();
	user->httpweb->AddHeaderManual("Accept: application/json, text/plain, */*");
	user->httpweb->AddHeaderManual("Origin: https://mall.bilibili.com");
	user->httpweb->AddHeaderManual("Referer: https://mall.bilibili.com/activities/activity2019626.html");
	int ret = toollib::HttpGetEx(user->curlweb, user->httpweb);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	rapidjson::Document doc;
	doc.Parse(user->httpweb->recv_data.c_str());
	if (!doc.IsObject() || !doc.HasMember("errno") || !doc["errno"].IsInt() || doc["errno"].GetInt()) {
		BOOST_LOG_SEV(g_logger::get(), error) << "[User" << user->fileid << "] "
			<< "APIShowSignStatus: " << user->httpweb->recv_data;
		return BILIRET::JSON_ERROR;
	}
	user->ten_sign_status = doc["data"]["isSigned"].GetBool();
	if (user->ten_sign_status) {
		BOOST_LOG_SEV(g_logger::get(), info) << "[User" << user->fileid << "] "
			<< "APIShowSignStatus " << "Signed";
	}
	else {
		BOOST_LOG_SEV(g_logger::get(), info) << "[User" << user->fileid << "] "
			<< "APIShowSignStatus " << "Unsigned";
	}

	return BILIRET::NOFAULT;
}

BILIRET apibl::APIShowSignDo(const std::shared_ptr<user_info> &user) {
	user->httpweb->url = "https://show.bilibili.com/api/activity/index/sign/do";
	user->httpweb->send_data = "{\"taskId\":\"act626-sign\"}";
	user->httpweb->ClearHeader();
	user->httpweb->AddHeaderManual("Accept: application/json, text/plain, */*");
	user->httpweb->AddHeaderManual("Content-Type: application/json;charset=UTF-8");
	user->httpweb->AddHeaderManual("Origin: https://mall.bilibili.com");
	user->httpweb->AddHeaderManual("Referer: https://mall.bilibili.com/activities/activity2019626.html");
	int ret = toollib::HttpPostEx(user->curlweb, user->httpweb);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	rapidjson::Document doc;
	doc.Parse(user->httpweb->recv_data.c_str());
	if (!doc.IsObject() || !doc.HasMember("errno") || !doc["errno"].IsInt() || doc["errno"].GetInt()) {
		BOOST_LOG_SEV(g_logger::get(), error) << "[User" << user->fileid << "] "
			<< "APIShowSignDo: " << user->httpweb->recv_data;
		return BILIRET::JSON_ERROR;
	}
	BOOST_LOG_SEV(g_logger::get(), info) << "[User" << user->fileid << "] "
		<< "APIShowSignDo: OK";

	return BILIRET::NOFAULT;
}

BILIRET apibl::APIShowShareStatus(const std::shared_ptr<user_info> &user) {
	std::ostringstream oss;
	oss << "https://show.bilibili.com/api/activity/index/share/sharetaskstatus?_="
		<< GetTimeStampM();
	user->httpweb->url = oss.str();
	user->httpweb->ClearHeader();
	user->httpweb->AddHeaderManual("Accept: application/json, text/plain, */*");
	user->httpweb->AddHeaderManual("Origin: https://mall.bilibili.com");
	user->httpweb->AddHeaderManual("Referer: https://mall.bilibili.com/activities/activity2019626.html");
	int ret = toollib::HttpGetEx(user->curlweb, user->httpweb);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	rapidjson::Document doc;
	doc.Parse(user->httpweb->recv_data.c_str());
	if (!doc.IsObject() || !doc.HasMember("errno") || !doc["errno"].IsInt() || doc["errno"].GetInt()) {
		BOOST_LOG_SEV(g_logger::get(), error) << "[User" << user->fileid << "] "
			<< "APIShowShareStatus: " << user->httpweb->recv_data;
		return BILIRET::JSON_ERROR;
	}
	if (!doc["data"]["egg"].IsNull()) {
		user->ten_egg_status = doc["data"]["egg"]["status"].GetInt();
		user->ten_egg_taskid = doc["data"]["egg"]["taskId"].GetString();
		user->ten_egg_assocId = doc["data"]["egg"]["assocId"].GetUint();
		user->ten_egg_type = doc["data"]["egg"]["type"].GetUint();
	}
	else {
		user->ten_egg_status = 2;
	}
	if (!doc["data"]["publish"].IsNull()) {
		user->ten_pub_status = doc["data"]["publish"]["status"].GetInt();
		user->ten_pub_taskid = doc["data"]["publish"]["taskId"].GetString();
		user->ten_pub_assocId = doc["data"]["publish"]["assocId"].GetUint();
		user->ten_pub_type = doc["data"]["publish"]["type"].GetUint();
	}
	else {
		user->ten_pub_status = 2;
	}
	BOOST_LOG_SEV(g_logger::get(), info) << "[User" << user->fileid << "] "
		<< "APIShowShareStatus" 
		<< " egg status: " << user->ten_egg_status
		<< " publish status: " << user->ten_pub_status;

	return BILIRET::NOFAULT;
}

BILIRET apibl::APIShowReward(
	const std::shared_ptr<user_info> &user,
	unsigned assocId,
	std::string taskId,
	unsigned type,
	const unsigned referer
) {
	return apibl::APIShowReward(
		user, 
		std::to_string(assocId),
		taskId,
		type,
		referer
	);
}

BILIRET apibl::APIShowReward(
	const std::shared_ptr<user_info>& user, 
	std::string assocId, 
	std::string taskId, 
	unsigned type,
	const unsigned referer
) {
	user->httpweb->url = "https://show.bilibili.com/api/activity/index/reward/receive";
	std::ostringstream oss;
	oss << "{\"assocId\":\"" << assocId
		<< "\",\"taskId\":\"" << taskId
		<< "\",\"type\":" << type
		<< "}";
	user->httpweb->send_data = oss.str();
	user->httpweb->ClearHeader();
	user->httpweb->AddHeaderManual("Accept: application/json, text/plain, */*");
	user->httpweb->AddHeaderManual("Content-Type: application/json;charset=UTF-8");
	user->httpweb->AddHeaderManual("Origin: https://mall.bilibili.com");
	if (referer == TEN_REFERER_MAIN) {
		user->httpweb->AddHeaderManual("Referer: https://mall.bilibili.com/activities/activity2019626.html");
	}
	if (referer == TEN_REFERER_FRIEND) {
		user->httpweb->AddHeaderManual("Referer: https://mall.bilibili.com/activities/friends.html");
	}
	if (referer == TEN_REFERER_PRODUCT) {
		user->httpweb->AddHeaderManual("Referer: https://mall.bilibili.com/activities/productList.html");
	}
	int ret = toollib::HttpPostEx(user->curlweb, user->httpweb);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	rapidjson::Document doc;
	doc.Parse(user->httpweb->recv_data.c_str());
	if (!doc.IsObject() || !doc.HasMember("errno") || !doc["errno"].IsInt()) {
		BOOST_LOG_SEV(g_logger::get(), error) << "[User" << user->fileid << "] "
			<< "APIShowReward: " << taskId << " " << user->httpweb->recv_data;
		return BILIRET::JSON_ERROR;
	}
	ret = doc["errno"].GetInt();
	if (ret) {
		std::string msg = doc["msg"].GetString();
		// 其它错误
		BOOST_LOG_SEV(g_logger::get(), info) << "[User" << user->fileid << "] "
			<< "APIShowReward: " << taskId << " " << doc["msg"].GetString();
		if (msg.find(u8"慢一点") != -1) {
			Sleep(20000);
		}
		return BILIRET::TEN_RECV_FAILED;
	}
	BOOST_LOG_SEV(g_logger::get(), info) << "[User" << user->fileid << "] "
		<< "APIShowReward: " << taskId << " " << doc["data"]["num"].GetInt();
	// 领取成功需要延时
	Sleep(10000);

	return BILIRET::NOFAULT;
}

BILIRET apibl::APIShowCallback(
	const std::shared_ptr<user_info> &user,
	std::string taskId,
	unsigned targetId,
	std::string eventId
) {
	user->httpweb->url = "https://show.bilibili.com/api/activity/index/share/callbackshare";
	std::ostringstream oss;
	oss << "{\"taskId\":\"" << taskId
		<< "\",\"targetId\":\"" << targetId
		<< "\",\"eventId\":\"" << eventId
		<< "\",\"eventPage\":\"" << "self_task"
		<< "\"}";
	user->httpweb->send_data = oss.str();
	user->httpweb->ClearHeader();
	user->httpweb->AddHeaderManual("Accept: application/json, text/plain, */*");
	user->httpweb->AddHeaderManual("Content-Type: application/json;charset=UTF-8");
	user->httpweb->AddHeaderManual("Origin: https://mall.bilibili.com");
	int ret = toollib::HttpPostEx(user->curlweb, user->httpweb);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	rapidjson::Document doc;
	doc.Parse(user->httpweb->recv_data.c_str());
	if (!doc.IsObject() || !doc.HasMember("errno") || !doc["errno"].IsInt()) {
		BOOST_LOG_SEV(g_logger::get(), error) << "[User" << user->fileid << "] "
			<< "APIShowCallback: " << taskId << " " << user->httpweb->recv_data;
		return BILIRET::JSON_ERROR;
	}
	ret = doc["errno"].GetInt();
	if (ret == 6000036) {
		// 点过了
		BOOST_LOG_SEV(g_logger::get(), info) << "[User" << user->fileid << "] "
			<< "APIShowCallback: " << taskId << " " << doc["msg"].GetString();
		return BILIRET::NOFAULT;
	}
	if (ret == 6000031) {
		// 分享过了
		BOOST_LOG_SEV(g_logger::get(), info) << "[User" << user->fileid << "] "
			<< "APIShowCallback: " << taskId << " " << doc["msg"].GetString();
		return BILIRET::NOFAULT;
	}
	if (ret) {
		// 其它错误
		BOOST_LOG_SEV(g_logger::get(), info) << "[User" << user->fileid << "] "
			<< "APIShowCallback: " << taskId << " " << doc["msg"].GetString();
		return BILIRET::TEN_SHARE_FAILED;
	}
	if (!doc.HasMember("data") || !doc["data"].IsBool() || !doc["data"].IsTrue()) {
		// 未知错误
		BOOST_LOG_SEV(g_logger::get(), info) << "[User" << user->fileid << "] "
			<< "APIShowCallback: " << taskId << " " << doc["msg"].GetString();
		return BILIRET::TEN_SHARE_FAILED;
	}
	BOOST_LOG_SEV(g_logger::get(), info) << "[User" << user->fileid << "] "
		<< "APIShowCallback: " << taskId << " " << "OK";

	return BILIRET::NOFAULT;
}

BILIRET apibl::APIShowTaskList(const std::shared_ptr<user_info>& user, unsigned id) {
	std::ostringstream oss;
	oss << "https://show.bilibili.com/api/activity/index/chelp/list?taskId=act626-help-"
		<< id << "&_="
		<< GetTimeStampM();
	user->httpweb->url = oss.str();
	user->httpweb->ClearHeader();
	user->httpweb->AddHeaderManual("Accept: application/json, text/plain, */*");
	user->httpweb->AddHeaderManual("Origin: https://mall.bilibili.com");
	user->httpweb->AddHeaderManual("Referer: https://mall.bilibili.com/activities/productList.html");
	int ret = toollib::HttpGetEx(user->curlweb, user->httpweb);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	rapidjson::Document doc;
	doc.Parse(user->httpweb->recv_data.c_str());
	if (!doc.IsObject() || !doc.HasMember("errno") || !doc["errno"].IsInt() || doc["errno"].GetInt()) {
		BOOST_LOG_SEV(g_logger::get(), error) << "[User" << user->fileid << "] "
			<< "APIShowTaskList: " << user->httpweb->recv_data;
		return BILIRET::JSON_ERROR;
	}
	if (doc["data"]["isOnGoing"].IsFalse()) {
		// 活动未开始
		return BILIRET::TEN_TASK_NOTOPEN;
	}
	rapidjson::Value &tasklist = doc["data"]["list"];
	for (unsigned i = 0; i < tasklist.Size(); i++) {
		unsigned assocId = tasklist[i]["assocId"].GetUint();
		if (assocId) {
			// 已生成
			unsigned num = tasklist[i]["num"].GetUint();
			if (num != 3) {
				user->ten_task_list.insert(assocId);
				BOOST_LOG_SEV(g_logger::get(), info) << "[User" << user->fileid << "] "
					<< "APIShowTaskList assocId: " << assocId;
			}
			else {
				unsigned status = tasklist[i]["status"].GetUint();
				if (status == 0) {
					// 领取碎片
					apibl::APIShowReward(
						user,
						assocId,
						tasklist[i]["taskId"].GetString(),
						tasklist[i]["type"].GetUint(),
						TEN_REFERER_PRODUCT
					);
				}
			}
		}
		else {
			// 未生成
			unsigned configId = tasklist[i]["configId"].GetUint();
			apibl::APIShowTaskCreate(user, configId);
		}
	}

	return BILIRET::NOFAULT;
}

BILIRET apibl::APIShowTaskCreate(const std::shared_ptr<user_info>& user, unsigned configId) {
	user->httpweb->url = "https://show.bilibili.com/api/activity/index/chelp/create";
	std::ostringstream oss;
	oss << "{\"id\":" << configId << "}";
	user->httpweb->send_data = oss.str();
	user->httpweb->ClearHeader();
	user->httpweb->AddHeaderManual("Accept: application/json, text/plain, */*");
	user->httpweb->AddHeaderManual("Content-Type: application/json;charset=UTF-8");
	user->httpweb->AddHeaderManual("Origin: https://mall.bilibili.com");
	user->httpweb->AddHeaderManual("Referer: https://mall.bilibili.com/activities/productList.html");
	int ret = toollib::HttpPostEx(user->curlweb, user->httpweb);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	rapidjson::Document doc;
	doc.Parse(user->httpweb->recv_data.c_str());
	if (!doc.IsObject() || !doc.HasMember("errno") || !doc["errno"].IsInt() || doc["errno"].GetInt()) {
		BOOST_LOG_SEV(g_logger::get(), error) << "[User" << user->fileid << "] "
			<< "APIShowTaskCreate: " << user->httpweb->recv_data;
		return BILIRET::JSON_ERROR;
	}
	unsigned assocId = doc["data"].GetUint();
	user->ten_task_list.insert(assocId);
	BOOST_LOG_SEV(g_logger::get(), info) << "[User" << user->fileid << "] "
		<< "APIShowTaskCreate assocId: " << assocId;

	return BILIRET::NOFAULT;
}

BILIRET apibl::APIShowLike(const std::shared_ptr<user_info> &user, std::string id) {
	user->httpweb->url = "https://show.bilibili.com/api/activity/index/chelp/like";
	std::ostringstream oss;
	oss << "{\"id\":\"" << id << "\"}";
	user->httpweb->send_data = oss.str();
	user->httpweb->ClearHeader();
	user->httpweb->AddHeaderManual("Accept: application/json, text/plain, */*");
	user->httpweb->AddHeaderManual("Content-Type: application/json;charset=UTF-8");
	user->httpweb->AddHeaderManual("Origin: https://mall.bilibili.com");
	oss.str("");
	oss << "Referer: https://mall.bilibili.com/activities/sharedetail.html?id=" << id;
	user->httpweb->AddHeaderManual(oss.str().c_str());
	int ret = toollib::HttpPostEx(user->curlweb, user->httpweb);
	if (ret) {
		return BILIRET::HTTP_ERROR;
	}

	rapidjson::Document doc;
	doc.Parse(user->httpweb->recv_data.c_str());
	if (!doc.IsObject() || !doc.HasMember("errno") || !doc["errno"].IsInt()) {
		BOOST_LOG_SEV(g_logger::get(), error) << "[User" << user->fileid << "] "
			<< "APIShowLike: " << user->httpweb->recv_data;
		return BILIRET::JSON_ERROR;
	}
	ret = doc["errno"].GetInt();
	std::string msg = doc["msg"].GetString();
	if (ret == 3) {
		if (msg.find(u8"超过上限") != -1) {
			user->ten_task_full = true;
			BOOST_LOG_SEV(g_logger::get(), info) << "[User" << user->fileid << "] "
				<< "APIShowLike: " << msg;
			return BILIRET::TEN_LIKE_FAILED;
		}
		if (msg.find(u8"已经点过") != -1) {
			BOOST_LOG_SEV(g_logger::get(), info) << "[User" << user->fileid << "] "
				<< "APIShowLike: " << msg;
			return BILIRET::TEN_LIKE_FAILED;
		}
		if (msg.find(u8"给自己") != -1) {
			BOOST_LOG_SEV(g_logger::get(), info) << "[User" << user->fileid << "] "
				<< "APIShowLike: " << msg;
			return BILIRET::TEN_LIKE_FAILED;
		}
		BOOST_LOG_SEV(g_logger::get(), info) << "[User" << user->fileid << "] "
			<< "APIShowLike: " << msg;
		// 分享点赞数已满时需要延时
		Sleep(10000);
		return BILIRET::TEN_TASK_FINISH;
	}
	BOOST_LOG_SEV(g_logger::get(), info) << "[User" << user->fileid << "] "
		<< "APIShowLike: " << msg;
	// 领取成功需要延时
	if (msg.find(u8"慢一点") != -1) {
		Sleep(20000);
	}
	else {
		Sleep(10000);
	}

	return BILIRET::NOFAULT;
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
