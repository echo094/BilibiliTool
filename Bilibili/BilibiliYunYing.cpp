#include "stdafx.h"
#include "BilibiliYunYing.h"
#include "log.h"
#include <sstream>
#include <iostream>
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
	BOOST_LOG_SEV(g_logger::get(), debug) << "[Lottery] Stop.";
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
		BOOST_LOG_SEV(g_logger::get(), error) << "[Lottery] Get RealRoomID Failed!";
		return BILIRET::HTTP_ERROR;
	}

	rapidjson::Document doc;
	doc.Parse(_httppack->strrecdata);
	if (!doc.IsObject() || !doc.HasMember("code") || !doc["code"].IsInt() || doc["code"].GetInt()
		|| !doc.HasMember("data") || !doc["data"].IsObject()
		|| !doc["data"].HasMember("room_id") || !doc["data"]["room_id"].IsInt()) {
		BOOST_LOG_SEV(g_logger::get(), error) << "[Lottery] Get RealRoomID Failed!";
		return BILIRET::JSON_ERROR;
	}
	if (doc["data"]["is_hidden"].GetBool() || doc["data"]["is_locked"].GetBool() || doc["data"]["encrypted"].GetBool()) {
		BOOST_LOG_SEV(g_logger::get(), warning) << "[Lottery] Invalid Room!";
		return BILIRET::ROOM_BLOCK;
	}
	rrid = doc["data"]["room_id"].GetInt();
	BOOST_LOG_SEV(g_logger::get(), info) << "[Lottery] RealRoomID: " << rrid;

	return BILIRET::NOFAULT;
}

void CBilibiliLotteryBase::_UpdateLotteryList(rapidjson::Value &infoArray, int srid, int rrid)
{
	long long curtime = GetTimeStamp();
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
			BOOST_LOG_SEV(g_logger::get(), info) << "[Lottery] Type: " << (*itor)->type << " Id: " << _curid;
		}
		itor = tlist.erase(itor);
	}
	if (!flag) {
		BOOST_LOG_SEV(g_logger::get(), info) << "[Lottery] No New ID.";
	}
}

BILIRET CBilibiliSmallTV::_GetLotteryID(CURL *pcurl, int srid, int rrid)
{
	int ret;
	_httppack->url = URL_LIVEAPI_HEAD;
	_httppack->url += "/gift/v3/smalltv/check?roomid=" + std::to_string(rrid);
	_httppack->ClearHeader();
	ret = toollib::HttpGetEx(pcurl, _httppack);
	if (ret) {
		BOOST_LOG_SEV(g_logger::get(), error) << "[SmallTV] HTTP GET Failed!";
		return BILIRET::HTTP_ERROR;
	}

	//开始处理小电视信息
	rapidjson::Document doc;
	doc.Parse(_httppack->strrecdata);
	if (!doc.IsObject() || !doc.HasMember("code") || !doc["code"].IsInt() || doc["code"].GetInt()
		|| !doc.HasMember("data") || !doc["data"].IsObject() || !doc["data"].HasMember("list") || !doc["data"]["list"].IsArray()) {
		BOOST_LOG_SEV(g_logger::get(), error) << "[SmallTV] ERROR:" << doc["code"].GetInt();
		return BILIRET::JSON_ERROR;
	}
	// 处理并添加新的抽奖信息
	this->_UpdateLotteryList(doc["data"]["list"], srid, rrid);

	return BILIRET::NOFAULT;
}

BILIRET CBilibiliGuard::_GetLotteryID(CURL *pcurl, int srid, int rrid)
{
	int ret;
	std::ostringstream oss;
	oss << URL_LIVEAPI_HEAD << "/lottery/v1/Lottery/check_guard?roomid=" << rrid;
	_httppack->url = oss.str();
	_httppack->ClearHeader();
	ret = toollib::HttpGetEx(pcurl, _httppack);
	if (ret) {
		BOOST_LOG_SEV(g_logger::get(), error) << "[Guard] HTTP GET Failed!";
		return BILIRET::HTTP_ERROR;
	}

	// 开始处理上船信息
	rapidjson::Document doc;
	doc.Parse(_httppack->strrecdata);
	if (!doc.IsObject() || !doc.HasMember("code") || !doc["code"].IsInt() || doc["code"].GetInt()
		|| !doc.HasMember("data") || !doc["data"].IsArray()) {
		BOOST_LOG_SEV(g_logger::get(), error) << "[SmallTV] ERROR:" << doc["code"].GetInt();
		return BILIRET::JSON_ERROR;
	}
	// 处理并添加新的上船信息
	this->_UpdateLotteryList(doc["data"], srid, rrid);

	return BILIRET::NOFAULT;
}

void CBilibiliGuard::_UpdateLotteryList(rapidjson::Value &infoArray, int srid, int rrid)
{
	long long curtime = GetTimeStamp();
	unsigned int i;
	std::string tid;
	int tloid, ttype;
	std::list<PBILI_LOTTERYDATA> tlist;
	PBILI_LOTTERYDATA pdata;

	// 将所有上船ID放入临时列表
	for (i = 0; i < infoArray.Size(); i++) {
		if (infoArray[i]["id"].IsInt()) {
			tloid = infoArray[i]["id"].GetInt();
		}
		else {
			continue;
		}
		ttype = infoArray[i]["privilege_type"].GetInt();
		if (ttype != 1) {
			continue;
		}
		pdata = new BILI_LOTTERYDATA;
		pdata->srid = srid;
		pdata->rrid = rrid;
		pdata->loid = tloid;
		pdata->type = infoArray[i]["keyword"].GetString();
		pdata->time = curtime + infoArray[i]["time"].GetInt();
		pdata->exinfo = ttype;
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
			BOOST_LOG_SEV(g_logger::get(), info) << "[Guard] Type: " << (*itor)->type << " Id: " << _curid
				<< " GType: " << (*itor)->exinfo;
		}
		itor = tlist.erase(itor);
	}
	if (!flag) {
		BOOST_LOG_SEV(g_logger::get(), info) << "[Guard] No New ID.";
	}
}

BILIRET CBilibiliLive::ApiCheckGuard(CURL *pcurl, int rrid, int &loid) const
{
	int ret;
	_httppack->url = URL_LIVEAPI_HEAD;
	_httppack->url += "/lottery/v1/lottery/check?roomid=" + std::to_string(rrid);
	_httppack->ClearHeader();
	_httppack->AddHeaderManual("Accept: application/json, text/plain, */*");
	ret = toollib::HttpGetEx(pcurl, _httppack);
	if (ret) {
		BOOST_LOG_SEV(g_logger::get(), error) << "[Live] Check Guard Failed!";
		return BILIRET::HTTP_ERROR;
	}

	rapidjson::Document doc;
	doc.Parse(_httppack->strrecdata);
	if (!doc.IsObject() || !doc.HasMember("code") || !doc["code"].IsInt() || doc["code"].GetInt()
		|| !doc.HasMember("data") || !doc["data"].IsObject() || !doc["data"].HasMember("guard") || !doc["data"]["guard"].IsArray()) {
		BOOST_LOG_SEV(g_logger::get(), error) << "[Live] Check Guard Failed!";
		return BILIRET::JSON_ERROR;
	}
	rapidjson::Value &infoArray = doc["data"]["guard"];
	if (infoArray.Size() == 0) {
		BOOST_LOG_SEV(g_logger::get(), warning) << "[Live] Guard not found!";
		return BILIRET::NORESULT;
	}
	loid = infoArray[0]["id"].GetInt();
	BOOST_LOG_SEV(g_logger::get(), info) << "[Live] GuardID: " << loid;

	return BILIRET::NOFAULT;
}

BILIRET CBilibiliLive::GetLiveList(CURL *pcurl, std::set<unsigned> &rlist, const unsigned minpop) const {
	BILIRET ret = BILIRET::NOFAULT;
	int page = 0, err = 10;
	while (1) {
		rapidjson::Document doc;
		page++;
		ret = _ApiLiveList(pcurl, doc, page);
		if (ret != BILIRET::NOFAULT) {
			return ret;
		}
		rapidjson::Value &infoArray = doc["data"];
		unsigned pop;
		for (auto it = infoArray.Begin(); it != infoArray.End(); it++) {
			pop = (*it)["online"].GetUint();
			if (pop < minpop) {
				// 可能会有人气小的房间跑到前面去的情况
				err--;
				if (!err) {
					return BILIRET::NOFAULT;
				}
			}
			rlist.insert((*it)["roomid"].GetUint());
		}
	}
}

BILIRET CBilibiliLive::PickOneRoom(CURL *pcurl, unsigned &nrid, const unsigned orid, const unsigned area) const {
	if (!area) {
		return BILIRET::NORESULT;
	}
	unsigned sa = 0;
	if (area == 1) {
		sa = 34;
	}
	rapidjson::Document doc;
	BILIRET ret = _ApiRoomArea(pcurl, doc, area, sa);
	if (ret != BILIRET::NOFAULT) {
		return ret;
	}
	nrid = doc["data"][0]["roomid"].GetUint();
	if (nrid == orid) {
		nrid = doc["data"][1]["roomid"].GetUint();
	}
	return BILIRET::NOFAULT;
}

BILIRET CBilibiliLive::_ApiLiveList(CURL *pcurl, rapidjson::Document &doc, int pid) const {
	int ret;
	std::ostringstream oss;
	oss << URL_LIVEAPI_HEAD << "/area/liveList?area=all&order=online&page=" << pid;
	_httppack->url = oss.str();
	_httppack->ClearHeader();
	_httppack->AddHeaderManual("Accept: application/json, text/plain, */*");
	ret = toollib::HttpGetEx(pcurl, _httppack);
	if (ret) {
		BOOST_LOG_SEV(g_logger::get(), error) << "[Live] Get Livelist Failed!";
		return BILIRET::HTTP_ERROR;
	}

	doc.Parse(_httppack->strrecdata);
	if (!doc.IsObject() || !doc.HasMember("code") || !doc["code"].IsInt() || doc["code"].GetInt()
		|| !doc.HasMember("data") || !doc["data"].IsArray()) {
		BOOST_LOG_SEV(g_logger::get(), error) << "[Live] Get Livelist Failed!";
		return BILIRET::JSON_ERROR;
	}

	return BILIRET::NOFAULT;
}

BILIRET CBilibiliLive::_ApiRoomArea(CURL *pcurl, rapidjson::Document &doc, const unsigned a1, const unsigned a2) const {
	int ret;
	std::ostringstream oss;
	oss << URL_LIVEAPI_HEAD << "/room/v1/area/getRoomList?platform=web&parent_area_id=" << a1
		<< "&cate_id=0&area_id=" << a2 
		<< "&sort_type=online&page=1&page_size=30";
	_httppack->url = oss.str();
	_httppack->ClearHeader();
	_httppack->AddHeaderManual("Accept: application/json, text/plain, */*");
	ret = toollib::HttpGetEx(pcurl, _httppack);
	if (ret) {
		BOOST_LOG_SEV(g_logger::get(), error) << "[Live] Get RoomArea Failed!";
		return BILIRET::HTTP_ERROR;
	}

	doc.Parse(_httppack->strrecdata);
	if (!doc.IsObject() || !doc.HasMember("code") || !doc["code"].IsInt() || doc["code"].GetInt()
		|| !doc.HasMember("data") || !doc["data"].IsArray() || (doc["data"].Size() < 2)) {
		BOOST_LOG_SEV(g_logger::get(), error) << "[Live] Get RoomArea Failed!";
		return BILIRET::JSON_ERROR;
	}

	return BILIRET::NOFAULT;
}
