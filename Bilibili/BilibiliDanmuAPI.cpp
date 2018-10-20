#include "stdafx.h"
#include "BilibiliDanmuAPI.h"

struct tagDANMUMSGDANMU
{
	time_t time;
	std::string strmsg, struname;
	std::wstring wstrmsg;
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

int DanmuAPI::ProcessData(const char* str, int len, int room, int type)
{
	int ret;
	switch (type) {
	case 0x03: {
		// int value = str[1] << 16 | str[2] << 8 | str[3];
		// printf("[DanmuAPI] Roomid: %d Popular:%d \n", room, value);
		break;
	}
	case 0x05: {
		ret = ParseJSON(str, room);
		if (ret) {
			printf("[DanmuAPI] ERROR: %s \n", str);
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

int DanmuAPI::ParseJSON(const char *str, int room) {
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
		if (m_rinfo[room].flag != DANMU_FLAG::MSG_SPECIALGIFT)
			return 0;
		return this->ParseSTORMMSG(doc, room);
	}

	if (strtype == "SEND_GIFT") {
		return 0;
	}

	// 旧的运营活动使用SYS_GIFT消息 已作废
	if (strtype == "SYS_GIFT") {
		return 0;
	}

	if (strtype == "SYS_MSG") {
		if (m_rinfo[room].flag != DANMU_FLAG::MSG_PUBEVENT)
			return 0;
		return this->ParseSYSMSG(doc, room);
	}

	if (strtype == "GUARD_MSG") {
		if (m_rinfo[room].flag != DANMU_FLAG::MSG_PUBEVENT) {
			return 0;
		}
		return this->ParseGUARDMSG(doc, room);
	}

	if (strtype == "GUARD_LOTTERY_START") {
		if (m_rinfo[room].flag != DANMU_FLAG::MSG_SPECIALGIFT) {
			return 0;
		}
		return this->ParseGUARDLO(doc, room);
	}

	if ((strtype == "PREPARING") || (strtype == "CUT_OFF")) {
		// 标记为需关闭
		m_rinfo[room].needclose = true;
		printf("%s[DanmuAPI] Roomid: %d Notice Close \n", _tool.GetTimeString().c_str(), room);
		if (m_rinfo[room].flag == DANMU_FLAG::MSG_PUBEVENT) {
			// 监测广播事件的房间需要立即更换
			if (parentthreadid) {
				PostThreadMessage(parentthreadid, MSG_CHANGEROOM, WPARAM(room), LPARAM(m_rinfo[room].area));
			}
		}
		return 0;
	}

	if (strtype == "LIVE") {
		if (m_rinfo[room].flag != DANMU_FLAG::MSG_SPECIALGIFT) {
			return 0;
		}
		// 取消需要关闭的标记
		m_rinfo[room].needclose = false;
		printf("%s[DanmuAPI] Roomid: %d Notice Open \n", _tool.GetTimeString().c_str(), room);
		return 0;
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
		BILI_ROOMEVENT *pinfo = new BILI_ROOMEVENT;
		pinfo->rid = room;
		pinfo->loidl = id;
		int num;
		std::string content;
		if (doc["data"]["39"].HasMember("num") && doc["data"]["39"]["num"].IsInt()) {
			num = doc["data"]["39"]["num"].GetInt();
		}
		if (doc["data"]["39"].HasMember("content") && doc["data"]["39"]["content"].IsString()) {
			tmpstr = doc["data"]["39"]["content"].GetString();
			content = _strcoding.UTF_8ToString(tmpstr.c_str());
		}
		printf("%s[DanmuAPI] Storm Room:%d ID: %I64d num: %d content: %s \n", _tool.GetTimeString().c_str(), room,
			id, num, content.c_str());
		if (parentthreadid) {
			PostThreadMessage(parentthreadid, MSG_NEWSPECIALGIFT, WPARAM(pinfo), 0);
		}
		return 0;
	}
	if (tmpstr == "end") {
		printf("%s[DanmuAPI] Storm Room:%d End \n", _tool.GetTimeString().c_str(), room);
		return 0;
	}
	return -1;
}

int DanmuAPI::ParseSYSMSG(rapidjson::Document &doc, int room) {
	// 如果消息不含有房间ID 则说明不是抽奖信息
	if (!doc.HasMember("real_roomid") || !doc["real_roomid"].IsInt()) {
		return 0;
	}
	int rrid = doc["real_roomid"].GetInt();

	// 检测是否为分区广播
	// 如果是全区广播需要过滤重复消息
	std::string tstr;
	tstr = doc["msg"].GetString();
	std::wstring wmsg = _strcoding.UTF_8ToWString(tstr.c_str());
	if ((wmsg.find(L"摩天大楼") == -1) && (wmsg.find(L"小金人") == -1)) {
		if (m_rinfo[room].area != 1) {
			return 0;
		}
	}

	// 有房间号就进行抽奖
	if (parentthreadid) {
		PostThreadMessage(parentthreadid, MSG_NEWSMALLTV, WPARAM(rrid), LPARAM(0));
	}

	return 0;
}

// 处理广播事件总督上船消息
// {"cmd":"GUARD_MSG","msg":"用户 :?[A]:? 在本房间开通了舰长","buy_type":3}
// {"cmd":"GUARD_MSG","msg":"用户 :?[A]:? 在主播 [B] 的直播间开通了总督","msg_new":"","url":"","roomid":10116204,"buy_type":1,"broadcast_type":0}
int DanmuAPI::ParseGUARDMSG(rapidjson::Document &doc, int room) {
	if (!doc.HasMember("buy_type") || !doc["buy_type"].IsInt()) {
		// 非开通消息
		return 0;
	}
	int btype = doc["buy_type"].GetInt();
	if (btype == 1) {
		int rid = doc["roomid"].GetInt();
		printf("%s[DanmuAPI] GUARD_MSG Roomid: %d Type: %d \n", _tool.GetTimeString().c_str(), rid, btype);
		if (parentthreadid) {
			PostThreadMessage(parentthreadid, MSG_NEWGUARD1, WPARAM(rid), 0);
		}
	}
	return 0;
}

// 处理房间事件非总督上船消息
int DanmuAPI::ParseGUARDLO(rapidjson::Document &doc, int room) {
	int btype = doc["data"]["privilege_type"].GetInt();
	if (btype != 1) {
		BILI_LOTTERYDATA *pinfo = new BILI_LOTTERYDATA;
		pinfo->rrid = room;
		pinfo->loid = doc["data"]["id"].GetInt();
		pinfo->exinfo = btype;
		pinfo->type = doc["data"]["lottery"]["keyword"].GetString();
		printf("%s[DanmuAPI] GUARD_LOTTERY Roomid: %d Type: %d \n", _tool.GetTimeString().c_str(), room, btype);
		if (parentthreadid) {
			PostThreadMessage(parentthreadid, MSG_NEWGUARD0, WPARAM(pinfo), 0);
		}
	}
	return 0;
}
