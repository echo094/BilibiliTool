#include "stdafx.h"
#include "BilibiliDanmuAPI.h"

struct tagDANMUMSGDANMU
{
	time_t time;
	std::string strmsg, struname;
	std::wstring wstrmsg;
};

struct tagDANMUMSGWELCOME
{
	int itype;
	int iuid, iguard_level;
	std::wstring struname;
};

struct tagDANMUMSGSYS
{
	int itype;
	time_t time;
	int iuid, iround;
	int iroomid, igiftid;//SYSGIFT
	std::string msg;
	std::wstring wmsg;
	std::string url;
};

int DanmuAPI::SetNotifyThread(DWORD id) {
	if (id >= 0) {
		parentthreadid = id;
	}
	return 0;
}

int DanmuAPI::CheckMessage(const unsigned char *str) {
	int i;
	if (str[4])
		return -1;
	if (str[5] - 16)
		return -1;
	for (i = 8; i < 11; i++) {
		if (str[i])
			return -1;
	}
	for (i = 12; i < 15; i++) {
		if (str[i])
			return -1;
	}
	return str[11];
}

int DanmuAPI::ProcessData(const unsigned char* str, int len, int room, int type)
{
	int ret;
	switch (type) {
	case 0x03: {
		// int value = str[1] << 16 | str[2] << 8 | str[3];
		// printf("[DanmuAPI] Roomid: %d Popular:%d \n", room, value);
		break;
	}
	case 0x05: {
		ret = ParseJSON((char *)str, room);
		if (ret) {
			printf("[DanmuAPI] ERROR: %s \n", (char *)str);
		}
		break;
	}
	case 0x08: {
		printf("[DanmuAPI] Roomid: %d Link start... \n", room);
		break;
	}
	default: {
		printf("[DanmuAPI] ERROR: Unknown Type %d \n", type);
		break;
	}
	}
	return 0;
}

int DanmuAPI::ParseJSON(char *str, int room) {
	rapidjson::Document doc;
	doc.Parse(str);

	if (!doc.IsObject() || !doc.HasMember("cmd") || !doc["cmd"].IsString()) {
		printf("[DanmuAPI] Error: %d JSON Wrong. \n", room);
		return -1;
	}
	std::string strtype = doc["cmd"].GetString();

	if (strtype == "DANMU_MSG") {
		if (!bdanmukuon)
			return 0;
		return this->ParseDANMUMSG(doc, room);
	}

	// 节奏风暴
	if (strtype == "SPECIAL_GIFT") {
		if (room_list[room].flag != DANMU_FLAG::MSG_SPECIALGIFT)
			return 0;
		return this->ParseSTORMMSG(doc, room);
	}

	if (strtype == "SEND_GIFT") {
		return 0;
	}

	//运营活动使用SYS_GIFT消息
	//新增活动时监控指定礼物ID即可
	if (strtype == "SYS_GIFT") {
		return 0;
		if (room_list[room].flag != DANMU_FLAG::MSG_PUBEVENT)
			return 0;
		return this->ParseSYSGIFT(doc, room);
	}

	if (strtype == "SYS_MSG") {
		if (room_list[room].flag != DANMU_FLAG::MSG_PUBEVENT)
			return 0;
		return this->ParseSYSMSG(doc, room);
	}

	if (strtype == "GUARD_MSG") {
		if (room_list[room].flag != DANMU_FLAG::MSG_PUBEVENT)
			return 0;
		return this->ParseGUARDMSG(doc, room);
	}

	//Exit if it is not in debug mode. 
	if (!bmodedebug)
		return 0;

	if (strtype == "WELCOME") {
		tagDANMUMSGWELCOME m_tmpwelcome;
		m_tmpwelcome.itype = 1;
		m_tmpwelcome.iuid = doc["data"]["uid"].GetInt();
		m_tmpwelcome.struname = _strcoding.UTF_8ToWString(doc["data"]["uname"].GetString());
		printf("[DanmuAPI] WELCOME: %d \n", m_tmpwelcome.iuid);
	}
	else if (strtype == "WELCOME_GUARD") {
		tagDANMUMSGWELCOME m_tmpwelcome;
		m_tmpwelcome.itype = 2;
		m_tmpwelcome.iuid = doc["data"]["uid"].GetInt();
		m_tmpwelcome.iguard_level = doc["data"]["guard_level"].GetInt();
		m_tmpwelcome.struname = _strcoding.UTF_8ToWString(doc["data"]["username"].GetString());
		printf("[DanmuAPI] WELCOME_GUARD: %d \n", m_tmpwelcome.iuid);
	}
	else if (strtype == "PREPARING") {
		tagDANMUMSGSYS m_tmpsysmsg;
		m_tmpsysmsg.itype = 1;
		m_tmpsysmsg.iroomid = doc["roomid"].GetInt();
		printf("[DanmuAPI] PREPARING: %d \n", m_tmpsysmsg.iroomid);
	}
	else if (strtype == "CUT_OFF") {
		tagDANMUMSGSYS m_tmpsysmsg;
		m_tmpsysmsg.itype = 2;
		m_tmpsysmsg.iroomid = doc["roomid"].GetInt();
		m_tmpsysmsg.msg = _strcoding.UTF_8ToString(doc["msg"].GetString());
		printf("[DanmuAPI] CUT_OFF: %d %s \n", m_tmpsysmsg.iroomid, m_tmpsysmsg.msg.c_str());
	}
	else if (strtype == "TV_START") {
		// printf("[DanmuAPI] TV_START: %d \n", roomid);
	}
	else if (strtype == "TV_END") {
		// printf("[DanmuAPI] TV_END: %d \n", roomid);
	}
	else if (strtype == "EVENT_CMD") {
		printf("[DanmuAPI] EVENT_CMD: %s \n", str);
	}

	return 0;
}

int DanmuAPI::ParseDANMUMSG(rapidjson::Document &doc, int room) {
	std::string tmpstr;
	tagDANMUMSGDANMU m_tmpdanmu;
	m_tmpdanmu.time = doc["info"][0][4].GetInt64();
	tmpstr = doc["info"][1].GetString();
	m_tmpdanmu.strmsg = _strcoding.UTF_8ToString(tmpstr.c_str());
	tmpstr = doc["info"][2][1].GetString();
	m_tmpdanmu.struname = _strcoding.UTF_8ToString(tmpstr.c_str());
	printf("[DanmuAPI] DANMU_MSG %d Roomid: %d %s:%s\n", (int)m_tmpdanmu.time, room, m_tmpdanmu.struname.c_str(), m_tmpdanmu.strmsg.c_str());
	return 0;
}

int DanmuAPI::ParseSTORMMSG(rapidjson::Document &doc, int room) {
	if (!doc.HasMember("data") || !doc["data"].IsObject() || !doc["data"].HasMember("39")
		|| !doc["data"]["39"].IsObject() || !doc["data"]["39"].HasMember("action")) {
		return -1;
	}
	std::string tmpstr;
	tmpstr = doc["data"]["39"]["action"].GetString();
	if (tmpstr == "start") {
		if (!doc["data"]["39"].HasMember("id")) {
			return -1;
		}
		long long id = 0;
		if (doc["data"]["39"]["id"].IsString()) {
			id = _atoi64(doc["data"]["39"]["id"].GetString());
		}
		if (doc["data"]["39"]["id"].IsInt64()) {
			id = doc["data"]["39"]["id"].GetInt64();
		}
		if (!id) {
			return -1;
		}
		tagSPECIALGIFT *tspecialgift = new tagSPECIALGIFT;
		tspecialgift->id = id;
		if (doc["data"]["39"].HasMember("num") && doc["data"]["39"]["num"].IsInt()) {
			tspecialgift->num = doc["data"]["39"]["num"].GetInt();
		}
		if (doc["data"]["39"].HasMember("time") && doc["data"]["39"]["time"].IsInt()) {
			tspecialgift->rtime = doc["data"]["39"]["time"].GetInt();
		}
		if (doc["data"]["39"].HasMember("content") && doc["data"]["39"]["content"].IsString()) {
			tmpstr = doc["data"]["39"]["content"].GetString();
			tspecialgift->content = _strcoding.UTF_8ToString(tmpstr.c_str());
		}
		printf("%s[DanmuAPI] Storm Room:%d ID: %I64d num: %d content: %s \n", _tool.GetTimeString().c_str(), room,
			tspecialgift->id, tspecialgift->num, tspecialgift->content.c_str());
		if (parentthreadid)
			PostThreadMessage(parentthreadid, MSG_NEWSPECIALGIFT, WPARAM((UINT)room), LPARAM(tspecialgift));
		return 0;
	}
	if (tmpstr == "end") {
		printf("%s[DanmuAPI] Storm Room:%d End \n", _tool.GetTimeString().c_str(), room);
		return 0;
	}
	return -1;
}

int DanmuAPI::ParseSYSGIFT(rapidjson::Document &doc, int room) {
	tagDANMUMSGSYS m_tmpsysmsg;
	m_tmpsysmsg.itype = 5;
	if (doc.HasMember("giftId")) {
		m_tmpsysmsg.igiftid = doc["giftId"].GetInt();
		if (m_tmpsysmsg.igiftid == 116 || m_tmpsysmsg.igiftid == 117) {
			m_tmpsysmsg.iroomid = doc["real_roomid"].GetInt();
			printf("%s[DanmuAPI] Raffle RealRoomId:%d Gift: %d\n", _tool.GetTimeString().c_str(), m_tmpsysmsg.iroomid, m_tmpsysmsg.igiftid);
			if (parentthreadid)
				PostThreadMessage(parentthreadid, MSG_NEWYUNYING, WPARAM((UINT)m_tmpsysmsg.iroomid), LPARAM(0));
		}
	}
	return 0;
}

int DanmuAPI::ParseSYSMSG(rapidjson::Document &doc, int room) {
	// 如果消息不含有房间ID 则说明不是抽奖信息
	if (!doc.HasMember("real_roomid") || !doc["real_roomid"].IsInt()) {
		return 0;
	}
	int rrid = doc["real_roomid"].GetInt();

	std::string tmpstr;
	tagDANMUMSGSYS m_tmpsysmsg;
	m_tmpsysmsg.itype = 4;
	tmpstr = doc["msg"].GetString();
#ifdef _DEBUG
	std::string showstr = _strcoding.UTF_8ToString(tmpstr.c_str());
	printf("[DanmuAPI] Roomid: %d SYS_MSG %s \n", room, showstr.c_str());
#endif
	m_tmpsysmsg.wmsg = _strcoding.UTF_8ToWString(tmpstr.c_str());

	int ret = -1;
	// small_tv
	ret = m_tmpsysmsg.wmsg.find(L"小电视飞船");
	if (ret != -1) {
		printf("%s[DanmuAPI] SmallTV RoomID:%d \n", _tool.GetTimeString().c_str(), rrid);
		if (parentthreadid)
			PostThreadMessage(parentthreadid, MSG_NEWSMALLTV, WPARAM(rrid), LPARAM(0));
		return 0;
	}
	// GIFT_20003
	ret = m_tmpsysmsg.wmsg.find(L"摩天大楼");
	if (ret != -1) {
		printf("%s[DanmuAPI] Skyscraper RoomID:%d \n", _tool.GetTimeString().c_str(), rrid);
		if (parentthreadid)
			PostThreadMessage(parentthreadid, MSG_NEWSMALLTV, WPARAM(rrid), LPARAM(0));
		return 0;
	}
	// GIFT_30013
	ret = m_tmpsysmsg.wmsg.find(L"C位光环");
	if (ret != -1) {
		printf("%s[DanmuAPI] Center RoomID:%d \n", _tool.GetTimeString().c_str(), rrid);
		if (parentthreadid)
			PostThreadMessage(parentthreadid, MSG_NEWSMALLTV, WPARAM(rrid), LPARAM(0));
		return 0;
	}
	// GIFT_30014
	ret = m_tmpsysmsg.wmsg.find(L"盛夏么么茶");
	if (ret != -1) {
		printf("%s[DanmuAPI] Summer Tea RoomID:%d \n", _tool.GetTimeString().c_str(), rrid);
		if (parentthreadid)
			PostThreadMessage(parentthreadid, MSG_NEWSMALLTV, WPARAM(rrid), LPARAM(0));
		return 0;
	}

	return -1;
}

// {"cmd":"GUARD_MSG","msg":"用户 :?[A]:? 在主播 [B] 的直播间开通了总督","buy_type":1}
int DanmuAPI::ParseGUARDMSG(rapidjson::Document &doc, int room) {
	if (!doc.HasMember("msg") || !doc["msg"].IsString()) {
		return -1;
	}
	std::string str1, str2;
	str1 = _strcoding.UrlUTF8(":? 在主播 ");
	str2 = _strcoding.UrlUTF8(" 的直播间开通了总督");
	std::string msg = _strcoding.UTF_8ToString(doc["msg"].GetString());
	msg = _strcoding.UrlUTF8(msg.c_str());
	printf("[DanmuAPI] Roomid: %d GUARD_MSG %s \n", room, msg.c_str());
	int pos1, pos2;
	pos1 = msg.find(str1.c_str());
	if (pos1 == -1) {
		return -1;
	}
	pos1 += str1.length();
	pos2 = msg.find(str2, pos1);
	if (pos2 == -1) {
		return -1;
	}
	std::string *user = new std::string;
	*user = msg.substr(pos1, pos2 - pos1);
	msg = _strcoding.UrlUTF8Decode(*user);
	printf("[DanmuAPI] Roomid: %d GUARD_MSG %s \n", room, msg.c_str());
	if (parentthreadid)
		PostThreadMessage(parentthreadid, MSG_NEWGUARD, WPARAM(user), LPARAM(0));

	return 0;
}
