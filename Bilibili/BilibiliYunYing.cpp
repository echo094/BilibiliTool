#include "stdafx.h"
#include "BilibiliYunYing.h"
using namespace rapidjson;

/*
{
	"code": 0,
	"msg": "OK",
	"message": "OK",
	"data": [{
		"raffleId": 28800,
		"type": "small_tv",
		"from": "鏃鸿€佸笀涓婄嚎",
		"from_user": {
			"uname": "鏃鸿€佸笀涓婄嚎",
			"face": "https://i1.hdslb.com/bfs/face/aa4ee339e405701d1d79ee6334cdc9b482a80268.jpg"
		},
		"time": 180,
		"status": 1
	}]
}
{
	"code":0,
	"msg":"success",
	"message":"success",
	"data":[
		{
			"raffleId":26065,
			"type":"flower_rain",
			"from":"椋庝箣寰姺",
			"from_user": {
				"uname":"椋庝箣寰姺",
				"face":"https://i1.hdslb.com/bfs/face/3e8d6513393cfa30027c9719ab85a4e73459276d.jpg"
			},
			"time":59,
			"status":1
		}
	]
}
*/

bool sortbiliyunyingdata(const PBILI_LOTTERYDATA & item1, const PBILI_LOTTERYDATA & item2)
{
	return item1->loid < item2->loid;
}

CBilibiliLotteryBase::CBilibiliLotteryBase() {
	_httppack = std::make_unique<CHTTPPack>();
	_curid = 0;
}

CBilibiliLotteryBase::~CBilibiliLotteryBase() {
	_httppack = nullptr;
#ifdef _DEBUG
	printf("[Lottery]Stop. \n");
#endif
}

int CBilibiliLotteryBase::CheckLottery(CURL *pcurl, int room)
{
	int rrid;
	BILIRET ret;
	ret = this->_CheckRoom(pcurl, room, rrid);
	// 房间无效则返回0 停止查询
	if (ret == BILIRET::ROOM_BLOCK) {
		return 0;
	}
	if (ret != BILIRET::NOFAULT) {
		// 查询失败返回-1
		return -1;
	}
	ret = this->_GetLotteryID(pcurl, room, rrid);
	if (ret != BILIRET::NOFAULT) {
		return -1;
	}
	return 0;
}

int CBilibiliLotteryBase::GetNextLottery(BILI_LOTTERYDATA &plo)
{
	if (_bili_lotteryactive.empty()) {
		return -1;
	}
	PBILI_LOTTERYDATA pdata = _bili_lotteryactive.front();
	plo = *pdata;
	delete pdata;
	_bili_lotteryactive.pop_front();
	return 0;
}

BILIRET CBilibiliLotteryBase::_CheckRoom(CURL *pcurl, int srid, int &rrid) {
	int ret;
	_httppack->url = URL_LIVEAPI_HEAD;
	_httppack->url += "/room/v1/Room/room_init?id=" + std::to_string(srid);
	_httppack->ClearHeader();
	_httppack->AddHeaderManual("Accept: application/json, text/plain, */*");
	ret = toollib::HttpGetEx(pcurl, _httppack);
	if (ret) {
		printf("%s[Lottery] Get RealRoomID Failed! \n", _tool.GetTimeString().c_str());
		return BILIRET::HTTP_ERROR;
	}

	rapidjson::Document doc;
	doc.Parse(_httppack->strrecdata);
	if (!doc.IsObject() || !doc.HasMember("code") || !doc["code"].IsInt() || doc["code"].GetInt()
		|| !doc.HasMember("data") || !doc["data"].IsObject()
		|| !doc["data"].HasMember("room_id") || !doc["data"]["room_id"].IsInt()) {
		printf("%s[Lottery] Get RealRoomID Failed! \n", _tool.GetTimeString().c_str());
		return BILIRET::JSON_ERROR;
	}
	if (doc["data"]["is_hidden"].GetBool() || doc["data"]["is_locked"].GetBool() || doc["data"]["encrypted"].GetBool()) {
		printf("%s[Lottery] Invalid Room! \n", _tool.GetTimeString().c_str());
		return BILIRET::ROOM_BLOCK;
	}
	rrid = doc["data"]["room_id"].GetInt();
	printf("%s[Lottery] RealRoomID: %d \n", _tool.GetTimeString().c_str(), rrid);

	return BILIRET::NOFAULT;
}

void CBilibiliLotteryBase::_UpdateLotteryList(rapidjson::Value &infoArray, int srid, int rrid)
{
	long long curtime = _tool.GetTimeStamp();
	unsigned int i;
	std::string tid;
	int tmpid;
	std::list<PBILI_LOTTERYDATA> tlist;
	PBILI_LOTTERYDATA pdata;

	// 将所有抽奖ID放入临时列表
	for (i = 0; i < infoArray.Size(); i++) {
		if (infoArray[i]["raffleId"].IsString()) {
			tid = infoArray[i]["raffleId"].GetString();
			tmpid = atoi(tid.c_str());
		}
		else if (infoArray[i]["raffleId"].IsInt()) {
			tmpid = infoArray[i]["raffleId"].GetInt();
		}
		else {
			continue;
		}
		pdata = new BILI_LOTTERYDATA;
		pdata->srid = srid;
		pdata->rrid = rrid;
		pdata->loid = tmpid;
		pdata->type = infoArray[i]["type"].GetString();
		pdata->time = curtime + infoArray[i]["time"].GetInt();
		tlist.push_back(pdata);
	}
	// 排序
	tlist.sort(sortbiliyunyingdata);
	// 添加到正式列表
	int flag = 0;
	std::list<PBILI_LOTTERYDATA>::iterator itor;
	for (itor = tlist.begin(); itor != tlist.end();) {
		if ((*itor)->loid <= _curid) {
			delete (*itor);
		}
		else {
			flag++;
			_curid = (*itor)->loid;
			_bili_lotteryactive.push_back(*itor);
			printf("%s[Lottery] Type: %s Id: %d \n", _tool.GetTimeString().c_str(), (*itor)->type.c_str(), _curid);
		}
		itor = tlist.erase(itor);
	}
	if (!flag) {
		printf("%s[Lottery] No New ID \n", _tool.GetTimeString().c_str());
	}
}

BILIRET CBilibiliYunYing::_GetLotteryID(CURL *pcurl, int srid, int rrid)
{
	int ret;
	_httppack->url = URL_LIVEAPI_HEAD;
	_httppack->url += "/activity/v1/Raffle/check?roomid=" + std::to_string(rrid);
	_httppack->ClearHeader();
	ret = toollib::HttpGetEx(pcurl, _httppack, 1);
	if (ret) {
		printf("%s[Raffle]HTTP GET Failed! \n", _tool.GetTimeString().c_str());
		return BILIRET::HTTP_ERROR;
	}

	rapidjson::Document doc;
	doc.Parse(_httppack->strrecdata);
	if (!doc.IsObject() || !doc.HasMember("code") || !doc["code"].IsInt() || !doc.HasMember("data") || !doc["data"].IsArray()) {
		printf("[Raffle] ERROR: %s\n", _httppack->strrecdata);
		return BILIRET::JSON_ERROR;
	}
	ret = doc["code"].GetInt();
	if (ret) {
		std::string msg = _strcoding.UTF_8ToString(doc["msg"].GetString());
		printf("[Raffle] ERROR: %d %s\n", ret, msg.c_str());
		return BILIRET::JSON_ERROR;
	}
	// 处理并添加新的抽奖信息
	this->_UpdateLotteryList(doc["data"], srid, rrid);

	return BILIRET::NOFAULT;
}

BILIRET CBilibiliSmallTV::_GetLotteryID(CURL *pcurl, int srid, int rrid)
{
	int ret;
	_httppack->url = URL_LIVEAPI_HEAD;
	_httppack->url += "/gift/v3/smalltv/check?roomid=" + std::to_string(rrid);
	_httppack->ClearHeader();
	ret = toollib::HttpGetEx(pcurl, _httppack);
	if (ret) {
		printf("%s[SmallTV] HTTP GET Failed! \n", _tool.GetTimeString().c_str());
		return BILIRET::HTTP_ERROR;
	}

	//开始处理小电视信息
	rapidjson::Document doc;
	doc.Parse(_httppack->strrecdata);
	if (!doc.IsObject() || !doc.HasMember("code") || !doc["code"].IsInt() || doc["code"].GetInt()
		|| !doc.HasMember("data") || !doc["data"].IsObject() || !doc["data"].HasMember("list") || !doc["data"]["list"].IsArray()) {
		printf("[SmallTV]ERROR: %d! \n", doc["code"].IsInt());
		return BILIRET::JSON_ERROR;
	}
	// 处理并添加新的抽奖信息
	this->_UpdateLotteryList(doc["data"]["list"], srid, rrid);

	return BILIRET::NOFAULT;
}

BILIRET CBilibiliLive::ApiSearchUser(CURL *pcurl, const char *user, int &rrid)
{
	int ret;
	_httppack->url = "https://search.bilibili.com/api/search?search_type=live_user&keyword=";
	_httppack->url += user;
	_httppack->ClearHeader();
	_httppack->AddHeaderManual("Accept: application/json, text/plain, */*");
	ret = toollib::HttpGetEx(pcurl, _httppack);
	if (ret) {
		printf("%s[Live] Search User Failed! \n", _tool.GetTimeString().c_str());
		return BILIRET::HTTP_ERROR;
	}

	rapidjson::Document doc;
	doc.Parse(_httppack->strrecdata);
	if (!doc.IsObject() || !doc.HasMember("code") || !doc["code"].IsInt() || doc["code"].GetInt()
		|| !doc.HasMember("result") || !doc["result"].IsArray()) {
		printf("%s[Live] Search User Failed! \n", _tool.GetTimeString().c_str());
		return BILIRET::JSON_ERROR;
	}
	rapidjson::Value &infoArray = doc["result"];
	if (infoArray.Size() == 0) {
		printf("%s[Live] User not found! \n", _tool.GetTimeString().c_str());
		return BILIRET::NORESULT;
	}
	rrid = doc["result"][0]["roomid"].GetInt();
	printf("%s[API Search] RoomID: %d \n", _tool.GetTimeString().c_str(), rrid);

	return BILIRET::NOFAULT;
}

BILIRET CBilibiliLive::ApiCheckGuard(CURL *pcurl, int rrid, int &loid)
{
	int ret;
	_httppack->url = URL_LIVEAPI_HEAD;
	_httppack->url += "/lottery/v1/lottery/check?roomid=" + std::to_string(rrid);
	_httppack->ClearHeader();
	_httppack->AddHeaderManual("Accept: application/json, text/plain, */*");
	ret = toollib::HttpGetEx(pcurl, _httppack);
	if (ret) {
		printf("%s[Live] Check Guard Failed! \n", _tool.GetTimeString().c_str());
		return BILIRET::HTTP_ERROR;
	}

	rapidjson::Document doc;
	doc.Parse(_httppack->strrecdata);
	if (!doc.IsObject() || !doc.HasMember("code") || !doc["code"].IsInt() || doc["code"].GetInt()
		|| !doc.HasMember("data") || !doc["data"].IsObject() || !doc["data"].HasMember("guard") || !doc["data"]["guard"].IsArray()) {
		printf("%s[Live] Check Guard Failed! \n", _tool.GetTimeString().c_str());
		return BILIRET::JSON_ERROR;
	}
	rapidjson::Value &infoArray = doc["data"]["guard"];
	if (infoArray.Size() == 0) {
		printf("%s[Live] Guard not found! \n", _tool.GetTimeString().c_str());
		return BILIRET::NORESULT;
	}
	loid = infoArray[0]["id"].GetInt();
	printf("%s[API Search] GuardID: %d \n", _tool.GetTimeString().c_str(), loid);

	return BILIRET::NOFAULT;
}
