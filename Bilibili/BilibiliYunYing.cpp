﻿#include "BilibiliYunYing.h"
#include <sstream>
#include <iostream>
#include "logger/log.h"
#include "utility/strconvert.h"

bool sortbiliyunyingdata(
	const std::shared_ptr<BILI_LOTTERYDATA>& item1, 
	const std::shared_ptr<BILI_LOTTERYDATA>& item2
) {
	return item1->loid < item2->loid;
}

event_list_base::event_list_base() :
	m_httppack(new toollib::CHTTPPack()) {
}

event_list_base::~event_list_base() {
	m_httppack = nullptr;
	BOOST_LOG_SEV(g_logger::get(), debug) << "[Lottery] Stop.";
}

int event_list_base::CheckLottery(CURL *pcurl, std::shared_ptr<BILI_LOTTERYDATA> data) {
	BILIRET ret;
	// 检测房间
	// 旧API失效 目前无法检测
	ret = _CheckRoom(pcurl, data);
	// 房间无效则返回0 停止查询
	if (ret == BILIRET::ROOM_BLOCK) {
		return 0;
	}
	if (ret != BILIRET::NOFAULT) {
		// 查询失败返回-1
		return -1;
	}
	ret = this->_GetLotteryID(pcurl, data);
	if (ret != BILIRET::NOFAULT) {
		return -1;
	}
	return 0;
}

std::shared_ptr<BILI_LOTTERYDATA> event_list_base::GetNextLottery() {
	if (m_lotteryactive.empty()) {
		return nullptr;
	}
	std::shared_ptr<BILI_LOTTERYDATA> pdata = m_lotteryactive.front();
	m_lotteryactive.pop_front();
	return pdata;
}

BILIRET event_list_base::_CheckRoom(CURL* pcurl, std::shared_ptr<BILI_LOTTERYDATA> &data) {
	if (!data->srid) {
		data->srid = data->rrid;
	}
	return BILIRET::NOFAULT;
}

BILIRET lottery_list::_GetLotteryID(CURL *pcurl, std::shared_ptr<BILI_LOTTERYDATA> &data) {
	std::ostringstream oss;
	oss << URL_LIVEAPI_HEAD << "/xlive/lottery-interface/v1/lottery/Check?roomid=" << data->rrid;
	m_httppack->url = oss.str();
	m_httppack->ClearHeader();
	int ret = toollib::HttpGetEx(pcurl, m_httppack);
	if (ret) {
		BOOST_LOG_SEV(g_logger::get(), error) << "[Lottery] HTTP GET Failed!";
		return BILIRET::HTTP_ERROR;
	}

	//开始处理小电视信息
	rapidjson::Document doc;
	doc.Parse(m_httppack->recv_data.c_str());
	if (!doc.IsObject() || !doc.HasMember("code") || !doc["code"].IsInt() || doc["code"].GetInt()
		|| !doc.HasMember("data") || !doc["data"].IsObject() || !doc["data"].HasMember("gift") || !doc["data"]["gift"].IsArray()) {
		BOOST_LOG_SEV(g_logger::get(), error) << "[Lottery] ERROR:" << doc["code"].GetInt();
		return BILIRET::JSON_ERROR;
	}
	// 处理并添加新的抽奖信息
	this->_UpdateLotteryList(doc["data"]["gift"], data);

	return BILIRET::NOFAULT;
}

void lottery_list::_UpdateLotteryList(rapidjson::Value &infoArray, std::shared_ptr<BILI_LOTTERYDATA> &data)
{
	long long curtime = toollib::GetTimeStamp();
	std::string tid;
	int tmpid;
	std::list<std::shared_ptr<BILI_LOTTERYDATA> > tlist;

	// 将所有抽奖ID放入临时列表
	for (unsigned i = 0; i < infoArray.Size(); i++) {
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
		std::shared_ptr<BILI_LOTTERYDATA> pdata(new BILI_LOTTERYDATA);
		pdata->cmd = MSG_LOT_GIFT;
		pdata->srid = data->srid;
		pdata->rrid = data->rrid;
		pdata->loid = tmpid;
		pdata->time_end = curtime + infoArray[i]["time"].GetInt();
		pdata->time_start = pdata->time_end - infoArray[i]["max_time"].GetInt();
		pdata->time_get = curtime + infoArray[i]["time_wait"].GetInt();
		pdata->type = infoArray[i]["type"].GetString();
		pdata->title = infoArray[i]["thank_text"].GetString();
		tlist.push_back(pdata);
	}
	// 排序
	tlist.sort(sortbiliyunyingdata);
	// 添加到正式列表
	int flag = 0;
	for (auto itor = tlist.begin(); itor != tlist.end();) {
		if (_CheckLoid((*itor)->loid)) {
			flag++;
			m_lotteryactive.push_back(*itor);
			BOOST_LOG_SEV(g_logger::get(), info) << "[Lottery] Type: " << (*itor)->type << " Id: " << (*itor)->loid;
		}
		itor = tlist.erase(itor);
	}
	if (!flag) {
		BOOST_LOG_SEV(g_logger::get(), info) << "[Lottery] No New ID.";
	}
}

void lottery_list::ClearLotteryCache() {
    m_cacheid.clear();
}

bool lottery_list::_CheckLoid(const long long id) {
    if (m_cacheid.count(id)) {
        return false;
    }
    m_cacheid.insert(id);
    return true;
}

BILIRET guard_list::_GetLotteryID(CURL *pcurl, std::shared_ptr<BILI_LOTTERYDATA> &data) {
	std::ostringstream oss;
	oss << URL_LIVEAPI_HEAD << "/xlive/lottery-interface/v1/lottery/Check?roomid=" << data->rrid;
	m_httppack->url = oss.str();
	m_httppack->ClearHeader();
	int ret = toollib::HttpGetEx(pcurl, m_httppack);
	if (ret) {
		BOOST_LOG_SEV(g_logger::get(), error) << "[Guard] HTTP GET Failed!";
		return BILIRET::HTTP_ERROR;
	}

	// 开始处理上船信息
	rapidjson::Document doc;
	doc.Parse(m_httppack->recv_data.c_str());
	if (!doc.IsObject() || !doc.HasMember("code") || !doc["code"].IsInt() || doc["code"].GetInt()
		|| !doc.HasMember("data") || !doc["data"].IsObject() || !doc["data"].HasMember("guard") || !doc["data"]["guard"].IsArray()) {
		BOOST_LOG_SEV(g_logger::get(), error) << "[Guard] ERROR:" << doc["code"].GetInt();
		return BILIRET::JSON_ERROR;
	}
	// 处理并添加新的上船信息
	this->_UpdateLotteryList(doc["data"]["guard"], data);

	return BILIRET::NOFAULT;
}

void guard_list::_UpdateLotteryList(rapidjson::Value &infoArray, std::shared_ptr<BILI_LOTTERYDATA> &data)
{
	long long curtime = toollib::GetTimeStamp();
	std::string tid;
	int tloid, ttype;
	std::list<std::shared_ptr<BILI_LOTTERYDATA> > tlist;

	// 将所有上船ID放入临时列表
	for (unsigned i = 0; i < infoArray.Size(); i++) {
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
		std::shared_ptr<BILI_LOTTERYDATA> pdata(new BILI_LOTTERYDATA);
		pdata->cmd = MSG_LOT_GUARD;
		pdata->srid = data->srid;
		pdata->rrid = data->rrid;
		pdata->loid = tloid;
		pdata->time_start = curtime;
		pdata->time_end = curtime + infoArray[i]["time"].GetInt();
		pdata->time_get = curtime + infoArray[i]["time_wait"].GetInt();
		pdata->type = infoArray[i]["keyword"].GetString();
		pdata->exinfo = ttype;
		tlist.push_back(pdata);
	}
	// 排序
	tlist.sort(sortbiliyunyingdata);
	// 添加到正式列表
	int flag = 0;
	for (auto itor = tlist.begin(); itor != tlist.end();) {
		if ((*itor)->loid > m_curid) {
			flag++;
			m_curid = (*itor)->loid;
			m_lotteryactive.push_back(*itor);
			BOOST_LOG_SEV(g_logger::get(), info) << "[Guard] Type: " << (*itor)->type << " Id: " << (*itor)->loid
				<< " GType: " << (*itor)->exinfo;
		}
		itor = tlist.erase(itor);
	}
	if (!flag) {
		BOOST_LOG_SEV(g_logger::get(), info) << "[Guard] No New ID.";
	}
}

BILIRET CBilibiliLive::GetAreaNum(CURL * pcurl, unsigned & num) const {
	int ret;
	std::ostringstream oss;
	oss << URL_LIVEAPI_HEAD << "/room/v1/Area/getList";
	_httppack->url = oss.str();
	_httppack->ClearHeader();
	_httppack->AddHeaderManual("Accept: application/json, text/plain, */*");
	ret = toollib::HttpGetEx(pcurl, _httppack);
	if (ret) {
		BOOST_LOG_SEV(g_logger::get(), error) << "[Live] Get area num failed code: " << ret;
		return BILIRET::HTTP_ERROR;
	}

	rapidjson::Document doc;
	doc.Parse(_httppack->recv_data.c_str());
	if (!doc.IsObject() || !doc.HasMember("code") || !doc["code"].IsInt() || doc["code"].GetInt()
		|| !doc.HasMember("data") || !doc["data"].IsArray()) {
		BOOST_LOG_SEV(g_logger::get(), error) << "[Live] Get area num failed!";
		return BILIRET::JSON_ERROR;
	}
	num = doc["data"].Size();

	return BILIRET::NOFAULT;
}

BILIRET CBilibiliLive::GetLiveList(CURL *pcurl, std::set<unsigned> &rlist, const unsigned minpop) const {
	BILIRET ret = BILIRET::NOFAULT;
	int page = 0, err = 10, filternum = 0;
	while (1) {
		rapidjson::Document doc;
		page++;
		ret = _ApiLiveList(pcurl, doc, page);
		if (ret != BILIRET::NOFAULT) {
			return ret;
		}
		rapidjson::Value &infoArray = doc["data"];
		if (infoArray.Size() == 0) {
			BOOST_LOG_SEV(g_logger::get(), error) << "[Live] List empty page: " << page;
			return BILIRET::NOFAULT;
		}
		unsigned pop;
		for (auto it = infoArray.Begin(); it != infoArray.End(); it++) {
			pop = (*it)["online"].GetUint();
			if (pop < minpop) {
				// 可能会有人气小的房间跑到前面去的情况
				err--;
				if (!err) {
                    BOOST_LOG_SEV(g_logger::get(), error) << "[Live] Filtered quantity: " << filternum;
					return BILIRET::NOFAULT;
				}
			}
            if ((*it)["area_v2_id"].GetInt() == 27) {
                // 过滤学习分区
                ++filternum;
                continue;
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
	oss << URL_LIVEAPI_HEAD << "/room/v1/Area/getListByAreaID?areaId=0&sort=online&pageSize="
		<< 100 << "&page=" << pid;
	_httppack->url = oss.str();
	_httppack->ClearHeader();
	_httppack->AddHeaderManual("Accept: application/json, text/plain, */*");
	ret = toollib::HttpGetEx(pcurl, _httppack);
	if (ret) {
		BOOST_LOG_SEV(g_logger::get(), error) << "[Live] Get Livelist Failed code: " << ret;
		return BILIRET::HTTP_ERROR;
	}

	doc.Parse(_httppack->recv_data.c_str());
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

	doc.Parse(_httppack->recv_data.c_str());
	if (!doc.IsObject() || !doc.HasMember("code") || !doc["code"].IsInt() || doc["code"].GetInt()
		|| !doc.HasMember("data") || !doc["data"].IsArray() || (doc["data"].Size() < 2)) {
		BOOST_LOG_SEV(g_logger::get(), error) << "[Live] Get RoomArea Failed!";
		return BILIRET::JSON_ERROR;
	}

	return BILIRET::NOFAULT;
}
