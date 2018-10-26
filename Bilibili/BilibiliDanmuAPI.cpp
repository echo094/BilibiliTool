#include "stdafx.h"
#include "BilibiliDanmuAPI.h"
#include "log.h"

enum {
	DM_NONE = 0,
	DM_CHANGE_ROOM_INFO,
	DM_COMBO_SEND,
	DM_CUT_OFF,
	DM_WARNING,
	DM_DANMU_MSG,
	DM_GUARD_BUY,
	DM_LIVE,
	DM_NOTICE_MSG,
	DM_GUARD_LOTTERY_START,
	DM_PK_MATCH,
	DM_PK_PRE,
	DM_PK_START,
	DM_PK_PROCESS,
	DM_PK_END,
	DM_PK_SETTLE,
	DM_PK_AGAIN,
	DM_PK_MIC_END,
	DM_PREPARING,
	DM_RAFFLE_START,
	DM_RAFFLE_END,
	DM_ROOM_REFRESH,
	DM_TV_START,
	DM_TV_END,
	DM_ROOM_BLOCK_INTO,
	DM_ROOM_BLOCK_MSG,
	DM_ROOM_KICKOUT,
	DM_ROOM_LOCK,
	DM_ROOM_LIMIT,
	DM_ROOM_SILENT_ON,
	DM_ROOM_SILENT_OFF,
	DM_SEND_GIFT,
	DM_SEND_TOP,
	DM_SPECIAL_GIFT,
	DM_WELCOME,
	DM_WELCOME_GUARD,
	DM_WELCOME_ACTIVITY,
	DM_WIN_ACTIVITY,
	DM_WISH_BOTTLE,
	DM_ROOM_RANK,
	DM_LOL_ACTIVITY,
	DM_ACTIVITY_EVENT,

	DM_ROOM_SHIELD,
	DM_ROOM_ADMINS,
	DM_PK_INVITE_INIT,
	DM_PK_INVITE_FAIL,
	DM_PK_INVITE_CANCEL,
	DM_PK_INVITE_SWITCH_OPEN,
	DM_PK_INVITE_SWITCH_CLOSE,
	DM_PK_INVITE_REFUSE,
	DM_PK_CLICK_AGAIN,

	DM_COMBO_END,
	DM_ENTRY_EFFECT,
	DM_GUARD_MSG,
	DM_LOTTERY_START,
	DM_SYS_GIFT,
	DM_SYS_MSG
};

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

void DanmuAPI::InitCMD() {
	m_cmdid["CHANGE_ROOM_INFO"] = DM_CHANGE_ROOM_INFO;
	m_cmdid["CUT_OFF"] = DM_CUT_OFF;
	m_cmdid["LIVE"] = DM_LIVE;
	m_cmdid["PREPARING"] = DM_PREPARING;
	m_cmdid["WARNING"] = DM_WARNING;
	// m_cmdid["ROOM_BLOCK_INTO"] = DM_ROOM_BLOCK_INTO;
	m_cmdid["ROOM_BLOCK_MSG"] = DM_ROOM_BLOCK_MSG;
	// m_cmdid["ROOM_KICKOUT"] = DM_ROOM_KICKOUT;
	m_cmdid["ROOM_LOCK"] = DM_ROOM_LOCK;
	// m_cmdid["ROOM_LIMIT"] = DM_ROOM_LIMIT;
	m_cmdid["ROOM_RANK"] = DM_ROOM_RANK;
	// m_cmdid["ROOM_REFRESH"] = DM_ROOM_REFRESH;
	m_cmdid["ROOM_SILENT_ON"] = DM_ROOM_SILENT_ON;
	m_cmdid["ROOM_SILENT_OFF"] = DM_ROOM_SILENT_OFF;
	m_cmdid["ROOM_SHIELD"] = DM_ROOM_SHIELD;
	m_cmdid["ROOM_ADMINS"] = DM_ROOM_ADMINS;

	m_cmdid["COMBO_SEND"] = DM_COMBO_SEND;
	m_cmdid["DANMU_MSG"] = DM_DANMU_MSG;
	m_cmdid["GUARD_BUY"] = DM_GUARD_BUY;
	m_cmdid["NOTICE_MSG"] = DM_NOTICE_MSG;
	m_cmdid["SEND_GIFT"] = DM_SEND_GIFT;
	// m_cmdid["SEND_TOP"] = DM_SEND_TOP;
	m_cmdid["WELCOME"] = DM_WELCOME;
	m_cmdid["WELCOME_GUARD"] = DM_WELCOME_GUARD;
	m_cmdid["WELCOME_ACTIVITY"] = DM_WELCOME_ACTIVITY;

	m_cmdid["PK_INVITE_INIT"] = DM_PK_INVITE_INIT;
	m_cmdid["PK_INVITE_FAIL"] = DM_PK_INVITE_FAIL;
	m_cmdid["PK_INVITE_CANCEL"] = DM_PK_INVITE_CANCEL;
	m_cmdid["PK_INVITE_SWITCH_OPEN"] = DM_PK_INVITE_SWITCH_OPEN;
	m_cmdid["PK_INVITE_SWITCH_CLOSE"] = DM_PK_INVITE_SWITCH_CLOSE;
	m_cmdid["PK_INVITE_REFUSE"] = DM_PK_INVITE_REFUSE;
	m_cmdid["PK_MATCH"] = DM_PK_MATCH;
	m_cmdid["PK_PRE"] = DM_PK_PRE;
	m_cmdid["PK_START"] = DM_PK_START;
	m_cmdid["PK_PROCESS"] = DM_PK_PROCESS;
	m_cmdid["PK_END"] = DM_PK_END;
	m_cmdid["PK_SETTLE"] = DM_PK_SETTLE;
	m_cmdid["PK_AGAIN"] = DM_PK_AGAIN;
	m_cmdid["PK_MIC_END"] = DM_PK_MIC_END;
	m_cmdid["PK_CLICK_AGAIN"] = DM_PK_CLICK_AGAIN;

	m_cmdid["GUARD_LOTTERY_START"] = DM_GUARD_LOTTERY_START;
	m_cmdid["RAFFLE_START"] = DM_RAFFLE_START;
	m_cmdid["RAFFLE_END"] = DM_RAFFLE_END;
	m_cmdid["TV_START"] = DM_TV_START;
	m_cmdid["TV_END"] = DM_TV_END;
	m_cmdid["SPECIAL_GIFT"] = DM_SPECIAL_GIFT;
	m_cmdid["WIN_ACTIVITY"] = DM_WIN_ACTIVITY;
	m_cmdid["WISH_BOTTLE"] = DM_WISH_BOTTLE;
	// m_cmdid["LOL_ACTIVITY"] = DM_LOL_ACTIVITY;
	m_cmdid["ACTIVITY_EVENT"] = DM_ACTIVITY_EVENT;

	m_cmdid["COMBO_END"] = DM_COMBO_END;
	m_cmdid["ENTRY_EFFECT"] = DM_ENTRY_EFFECT;
	m_cmdid["GUARD_MSG"] = DM_GUARD_MSG;
	m_cmdid["LOTTERY_START"] = DM_LOTTERY_START;
	m_cmdid["SYS_GIFT"] = DM_SYS_GIFT;
	m_cmdid["SYS_MSG"] = DM_SYS_MSG;
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
		// BOOST_LOG_SEV(g_logger::get(), trace) << "[DanmuAPI] " << room << " Popular:" << value;
		break;
	}
	case 0x05: {
		ret = ParseJSON(str, room);
		if (ret) {
			BOOST_LOG_SEV(g_logger::get(), debug) << "[DanmuAPI] " << room << " DMNEW: " << str;
		}
		break;
	}
	case 0x08: {
		BOOST_LOG_SEV(g_logger::get(), info) << "[DanmuAPI] " << room << " Link start...";
		break;
	}
	default: {
		BOOST_LOG_SEV(g_logger::get(), error) << "[DanmuAPI] " << room << " Unknown msg type: " << type;
		break;
	}
	}
	return 0;
}

int DanmuAPI::ParseJSON(const char *str, int room) {
	rapidjson::Document doc;
	doc.Parse(str);

	if (!doc.IsObject() || !doc.HasMember("cmd") || !doc["cmd"].IsString()) {
		BOOST_LOG_SEV(g_logger::get(), error) << "[DanmuAPI] " << room << " JSON Wrong.";
		return -1;
	}
	std::string strtype = doc["cmd"].GetString();
	if (!m_cmdid.count(strtype)) {
		// 新指令
		return -1;
	}
	int cmdid = m_cmdid[strtype];

	switch (cmdid) {
	case DM_DANMU_MSG: {
		if (!bdanmukuon)
			return 0;
		return this->ParseDANMUMSG(doc, room);
	}
	case DM_SPECIAL_GIFT: {
		if (m_rinfo[room].flag != DANMU_FLAG::MSG_SPECIALGIFT)
			return 0;
		return this->ParseSTORMMSG(doc, room);
	}
	case DM_NOTICE_MSG: {
		if (m_rinfo[room].flag != DANMU_FLAG::MSG_PUBEVENT)
			return 0;
		return this->ParseNOTICEMSG(doc, room);
	}
	case DM_GUARD_LOTTERY_START: {
		if (m_rinfo[room].flag != DANMU_FLAG::MSG_SPECIALGIFT) {
			return 0;
		}
		return this->ParseGUARDLO(doc, room);
	}
	case DM_PREPARING:
	case DM_CUT_OFF: {
		// 标记为需关闭
		m_rinfo[room].needclose = true;
		BOOST_LOG_SEV(g_logger::get(), trace) << "[DanmuAPI] " << room << " Notice Close.";
		if (m_rinfo[room].flag == DANMU_FLAG::MSG_PUBEVENT) {
			// 监测广播事件的房间需要立即更换
			if (parentthreadid) {
				PostThreadMessage(parentthreadid, MSG_CHANGEROOM, WPARAM(room), LPARAM(m_rinfo[room].area));
			}
		}
		return 0;
	}
	case DM_LIVE: {
		if (m_rinfo[room].flag != DANMU_FLAG::MSG_SPECIALGIFT) {
			return 0;
		}
		// 取消需要关闭的标记
		m_rinfo[room].needclose = false;
		BOOST_LOG_SEV(g_logger::get(), trace) << "[DanmuAPI] " << room << " Notice Open.";
		return 0;
	}
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
	BOOST_LOG_SEV(g_logger::get(), info) << "[DanmuAPI] " << room 
		<< " DANMU_MSG: " << m_tmpdanmu.struname << ":" << m_tmpdanmu.strmsg;
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
		BOOST_LOG_SEV(g_logger::get(), info) << "[DanmuAPI] " << room
			<< " Storm: ID:" << id << " num:" << num << " content:" << content;
		if (parentthreadid) {
			PostThreadMessage(parentthreadid, MSG_NEWSPECIALGIFT, WPARAM(pinfo), 0);
		}
		return 0;
	}
	if (tmpstr == "end") {
		BOOST_LOG_SEV(g_logger::get(), info) << "[DanmuAPI] " << room
			<< " Storm: end ";
		return 0;
	}
	return -1;
}

int DanmuAPI::ParseNOTICEMSG(rapidjson::Document &doc, int room) {
	// 如果消息不含有房间ID 则说明不是抽奖信息
	if (!doc.HasMember("msg_type") || !doc["msg_type"].IsInt()) {
		return -1;
	}
	int type = doc["msg_type"].GetInt();
	switch (type) {
	case 2: {
		return ParseSYSMSG(doc, room);
	}
	case 3: {
		return ParseGUARDMSG(doc, room);
	}
	case 1:
	case 4:
	case 5:
	case 6: {
		return 0;
	}
	}

	return -1;
}

int DanmuAPI::ParseSYSMSG(rapidjson::Document &doc, int room) {
	// 如果消息不含有房间ID 则说明不是抽奖信息
	if (!doc.HasMember("real_roomid") || !doc["real_roomid"].IsInt()) {
		return -1;
	}
	int rrid = doc["real_roomid"].GetInt();

	// 如果是全区广播需要过滤重复消息
	std::string tstr;
	tstr = doc["msg_common"].GetString();
	std::wstring wmsg = _strcoding.UTF_8ToWString(tstr.c_str());
	if (wmsg.find(L"全区广播") != -1) {
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
int DanmuAPI::ParseGUARDMSG(rapidjson::Document &doc, int room) {
	// 过滤当前房间的开通信息
	std::string tstr;
	tstr = doc["msg_common"].GetString();
	std::wstring wmsg = _strcoding.UTF_8ToWString(tstr.c_str());
	if (wmsg.find(L"在本房间") != -1) {
		return 0;
	}
	// 全区广播只需通知一次
	if (m_rinfo[room].area != 1) {
		return 0;
	}
	int rid = doc["real_roomid"].GetInt();
	BOOST_LOG_SEV(g_logger::get(), info) << "[DanmuAPI] " << room
		<< " GUARD_MSG: " << " rid:" << rid;
	if (parentthreadid) {
		PostThreadMessage(parentthreadid, MSG_NEWGUARD1, WPARAM(rid), 0);
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
		BOOST_LOG_SEV(g_logger::get(), info) << "[DanmuAPI] " << room
			<< " GUARD_LOTTERY: Type: " << btype;
		if (parentthreadid) {
			PostThreadMessage(parentthreadid, MSG_NEWGUARD0, WPARAM(pinfo), 0);
		}
	}
	return 0;
}
