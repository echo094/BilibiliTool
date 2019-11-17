#include "event_dmmsg.h"
#include "logger/log.h"
#include "utility/platform.h"
#include "utility/strconvert.h"

enum {
	DM_NONE = 0,
	// 主文件
	DM_ACTIVITY_MATCH_GIFT,
	DM_ANIMATION,
	DM_CHANGE_ROOM_INFO,
	DM_COMBO_SEND,
	DM_CUT_OFF,
	DM_DANMU_LOTTERY_START,
	DM_DANMU_LOTTERY_END,
	// DM_DANMU_LOTTERY_AWARD,
	DM_WARNING,
	DM_DANMU_MSG,
	DM_GUARD_BUY,
	DM_GUARD_ACHIEVEMENT_ROOM,
	DM_LIVE,
	// DM_LUCK_GIFT_AWARD_USER,
	// DM_MESSAGEBOX_USER_GAIN_MEDAL,
	// DM_LITTLE_TIPS
	DM_NOTICE_MSG,
	DM_PK_MATCH,
	DM_PK_PRE,
	DM_PK_START,
	DM_PK_PROCESS,
	DM_PK_END,
	DM_PK_SETTLE,
	DM_PK_AGAIN,
	DM_PK_MIC_END,
	DM_PK_BATTLE_PRE,
	DM_PK_BATTLE_START,
	DM_PK_BATTLE_PROCESS,
	DM_PK_BATTLE_PRO_TYPE,
	DM_PK_BATTLE_GIFT,
	DM_PK_BATTLE_SPECIAL_GIFT,
	DM_PK_BATTLE_CRIT,
	DM_PK_BATTLE_VOTES_ADD,
	DM_PK_BATTLE_END,
	DM_PK_BATTLE_SETTLE_USER,
	DM_PK_BATTLE_RANK_CHANGE,
	DM_PREPARING,
	// DM_ROOM_REFRESH,
	DM_ROOM_SKIN_MSG,
	DM_ROOM_BOX_USER,
	DM_SCORE_CARD,
	DM_TV_START,
	DM_TV_END,
	DM_RAFFLE_END,
	DM_PK_LOTTERY_START,
	DM_GUARD_LOTTERY_START,
	// DM_ROOM_BLOCK_INTO,
	DM_ROOM_BLOCK_MSG,
	// DM_ROOM_KICKOUT,
	DM_ROOM_LOCK,
	DM_ROOM_LIMIT,
	DM_ROOM_SILENT_ON,
	DM_ROOM_SILENT_OFF,
	DM_SEND_GIFT,
	// DM_SEND_TOP,
	DM_SPECIAL_GIFT,
	DM_WEEK_STAR_CLOCK,
	DM_WELCOME,
	DM_WELCOME_GUARD,
	DM_ENTRY_EFFECT,
	DM_BOX_ACTIVITY_START,
	DM_WIN_ACTIVITY,
	// DM_WIN_ACTIVITY_USER,
	DM_WISH_BOTTLE,
	DM_ROOM_RANK,
	DM_HOUR_RANK_AWARDS,
	DM_LOL_ACTIVITY,
	DM_ROOM_REAL_TIME_MESSAGE_UPDATE,
	DM_ROOM_CHANGE,
	DM_SUPER_CHAT_MESSAGE,
	DM_SUPER_CHAT_MESSAGE_DELETE,
	DM_SUPER_CHAT_ENTRANCE,
	// 房间
	DM_ROOM_BOX_MASTER,
	DM_ROOM_SHIELD,
	DM_ROOM_ADMINS,
	// 通知
	DM_COMBO_END,
	DM_NOTICE_MSG_H5,
	// PK
	DM_PK_INVITE_INIT,
	DM_PK_INVITE_FAIL,
	DM_PK_INVITE_CANCEL,
	DM_PK_INVITE_SWITCH_OPEN,
	DM_PK_INVITE_SWITCH_CLOSE,
	DM_PK_INVITE_REFUSE,
	DM_PK_CLICK_AGAIN,
	// 大乱斗
	DM_PK_BATTLE_ENTRANCE,
	DM_PK_BATTLE_MATCH_TIMEOUT,
	DM_PK_BATTLE_SETTLE,
	// Spuer Chat
	DM_SUPER_CHAT_MESSAGE_JPN,
	// 活动
	DM_RAFFLE_START,
	// 大乱斗界面
	DM_ACTIVITY_BANNER_UPDATE,
	DM_ACTIVITY_BANNER_UPDATE_V2,
	DM_ACTIVITY_BANNER_RED_NOTICE_CLOSE,
	DM_ACTIVITY_BANNER_CLOSE,
	// 机甲活动
	DM_BOSS_INFO,
	DM_BOSS_INJURY,
	DM_BOSS_BATTLE,
	DM_BOSS_ENERGY,
	// Voice
	DM_VOICE_JOIN_STATUS,
	DM_VOICE_JOIN_LIST,
	DM_VOICE_JOIN_ROOM_COUNT_INFO,
	DM_VOICE_JOIN_SWITCH,
	// 播放器
	DM_SYS_GIFT,
	DM_SYS_MSG,
	DM_GUARD_MSG,
	DM_GUIARD_MSG,
	// 新增
	DM_DANMU_GIFT_LOTTERY_START,
	DM_DANMU_GIFT_LOTTERY_AWARD,
	DM_DANMU_GIFT_LOTTERY_END,
	DM_ANCHOR_LOT_CHECKSTATUS,
	DM_ANCHOR_LOT_START,
	DM_ANCHOR_LOT_AWARD,
	DM_ANCHOR_LOT_END,
	// 其它
	DM_DANMU_MSG_402220,
	DM_USER_TOAST_MSG,
	DM_LOTTERY_START,
	DM_room_admin_entrance,
	DM_new_anchor_reward,
	DM_DAILY_QUEST_NEWDAY
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

void event_dmmsg::InitCMD() {
	// 18 3
	m_cmdid["CHANGE_ROOM_INFO"] = DM_CHANGE_ROOM_INFO;
	m_cmdid["CUT_OFF"] = DM_CUT_OFF;
	m_cmdid["LIVE"] = DM_LIVE;
	m_cmdid["PREPARING"] = DM_PREPARING;
	m_cmdid["WARNING"] = DM_WARNING;
	m_cmdid["ROOM_SKIN_MSG"] = DM_ROOM_SKIN_MSG;
	m_cmdid["ROOM_BOX_USER"] = DM_ROOM_BOX_USER;
	m_cmdid["ROOM_BOX_MASTER"] = DM_ROOM_BOX_MASTER;
	m_cmdid["ROOM_BLOCK_MSG"] = DM_ROOM_BLOCK_MSG;
	m_cmdid["ROOM_LOCK"] = DM_ROOM_LOCK;
	m_cmdid["ROOM_LIMIT"] = DM_ROOM_LIMIT;
	m_cmdid["ROOM_RANK"] = DM_ROOM_RANK;
	m_cmdid["ROOM_SILENT_ON"] = DM_ROOM_SILENT_ON;
	m_cmdid["ROOM_SILENT_OFF"] = DM_ROOM_SILENT_OFF;
	m_cmdid["ROOM_REAL_TIME_MESSAGE_UPDATE"] = DM_ROOM_REAL_TIME_MESSAGE_UPDATE;
	m_cmdid["ROOM_SHIELD"] = DM_ROOM_SHIELD;
	m_cmdid["ROOM_ADMINS"] = DM_ROOM_ADMINS;
	m_cmdid["ROOM_CHANGE"] = DM_ROOM_CHANGE;
	// 11 4
	m_cmdid["COMBO_SEND"] = DM_COMBO_SEND;
	m_cmdid["COMBO_END"] = DM_COMBO_END;
	m_cmdid["DANMU_MSG"] = DM_DANMU_MSG;
	m_cmdid["GUARD_BUY"] = DM_GUARD_BUY;
	m_cmdid["GUARD_ACHIEVEMENT_ROOM"] = DM_GUARD_ACHIEVEMENT_ROOM;
	m_cmdid["NOTICE_MSG"] = DM_NOTICE_MSG;
	m_cmdid["NOTICE_MSG_H5"] = DM_NOTICE_MSG_H5;
	m_cmdid["SEND_GIFT"] = DM_SEND_GIFT;
	m_cmdid["WELCOME"] = DM_WELCOME;
	m_cmdid["WELCOME_GUARD"] = DM_WELCOME_GUARD;
	m_cmdid["ENTRY_EFFECT"] = DM_ENTRY_EFFECT;
	// 15
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
	// 15
	m_cmdid["PK_BATTLE_ENTRANCE"] = DM_PK_BATTLE_ENTRANCE;
	m_cmdid["PK_BATTLE_MATCH_TIMEOUT"] = DM_PK_BATTLE_MATCH_TIMEOUT;
	m_cmdid["PK_BATTLE_PRE"] = DM_PK_BATTLE_PRE;
	m_cmdid["PK_BATTLE_START"] = DM_PK_BATTLE_START;
	m_cmdid["PK_BATTLE_PROCESS"] = DM_PK_BATTLE_PROCESS;
	m_cmdid["PK_BATTLE_PRO_TYPE"] = DM_PK_BATTLE_PRO_TYPE;
	m_cmdid["PK_BATTLE_GIFT"] = DM_PK_BATTLE_GIFT;
	m_cmdid["PK_BATTLE_SPECIAL_GIFT"] = DM_PK_BATTLE_SPECIAL_GIFT; 
	m_cmdid["PK_BATTLE_CRIT"] = DM_PK_BATTLE_CRIT;
	m_cmdid["PK_BATTLE_VOTES_ADD"] = DM_PK_BATTLE_VOTES_ADD;
	m_cmdid["PK_BATTLE_END"] = DM_PK_BATTLE_END;
	m_cmdid["PK_BATTLE_RANK_CHANGE"] = DM_PK_BATTLE_RANK_CHANGE;
	m_cmdid["PK_BATTLE_SETTLE_USER"] = DM_PK_BATTLE_SETTLE_USER;
	m_cmdid["PK_BATTLE_SETTLE"] = DM_PK_BATTLE_SETTLE;
	m_cmdid["PK_LOTTERY_START"] = DM_PK_LOTTERY_START;
	// 4
	m_cmdid["SUPER_CHAT_MESSAGE"] = DM_SUPER_CHAT_MESSAGE;
	m_cmdid["SUPER_CHAT_MESSAGE_DELETE"] = DM_SUPER_CHAT_MESSAGE_DELETE;
	m_cmdid["SUPER_CHAT_ENTRANCE"] = DM_SUPER_CHAT_ENTRANCE;
	m_cmdid["SUPER_CHAT_MESSAGE_JPN"] = DM_SUPER_CHAT_MESSAGE_JPN;
	// 16 2
	m_cmdid["DANMU_LOTTERY_START"] = DM_DANMU_LOTTERY_START;
	m_cmdid["DANMU_LOTTERY_END"] = DM_DANMU_LOTTERY_END;
	m_cmdid["GUARD_LOTTERY_START"] = DM_GUARD_LOTTERY_START;
	m_cmdid["RAFFLE_START"] = DM_RAFFLE_START;
	m_cmdid["RAFFLE_END"] = DM_RAFFLE_END;
	m_cmdid["SCORE_CARD"] = DM_SCORE_CARD;
	m_cmdid["TV_START"] = DM_TV_START;
	m_cmdid["TV_END"] = DM_TV_END;
	m_cmdid["SPECIAL_GIFT"] = DM_SPECIAL_GIFT;
	m_cmdid["WEEK_STAR_CLOCK"] = DM_WEEK_STAR_CLOCK;
	m_cmdid["BOX_ACTIVITY_START"] = DM_BOX_ACTIVITY_START;
	m_cmdid["WIN_ACTIVITY"] = DM_WIN_ACTIVITY;
	m_cmdid["WISH_BOTTLE"] = DM_WISH_BOTTLE;
	m_cmdid["LOL_ACTIVITY"] = DM_LOL_ACTIVITY;
	m_cmdid["HOUR_RANK_AWARDS"] = DM_HOUR_RANK_AWARDS;
	m_cmdid["ACTIVITY_MATCH_GIFT"] = DM_ACTIVITY_MATCH_GIFT;
	// 4
	m_cmdid["ACTIVITY_BANNER_UPDATE"] = DM_ACTIVITY_BANNER_UPDATE;
	m_cmdid["ACTIVITY_BANNER_UPDATE_V2"] = DM_ACTIVITY_BANNER_UPDATE_V2;
	m_cmdid["ACTIVITY_BANNER_RED_NOTICE_CLOSE"] = DM_ACTIVITY_BANNER_RED_NOTICE_CLOSE;
	m_cmdid["ACTIVITY_BANNER_CLOSE"] = DM_ACTIVITY_BANNER_CLOSE;
	// 5
	m_cmdid["ANIMATION"] = DM_ANIMATION;
	m_cmdid["BOSS_INFO"] = DM_BOSS_INFO;
	m_cmdid["BOSS_INJURY"] = DM_BOSS_INJURY;
	m_cmdid["BOSS_BATTLE"] = DM_BOSS_BATTLE;
	m_cmdid["BOSS_ENERGY"] = DM_BOSS_ENERGY;
	// 4
	m_cmdid["VOICE_JOIN_STATUS"] = DM_VOICE_JOIN_STATUS;
	m_cmdid["VOICE_JOIN_LIST"] = DM_VOICE_JOIN_LIST;
	m_cmdid["VOICE_JOIN_ROOM_COUNT_INFO"] = DM_VOICE_JOIN_ROOM_COUNT_INFO;
	m_cmdid["VOICE_JOIN_SWITCH"] = DM_VOICE_JOIN_SWITCH;
	// 4
	m_cmdid["SYS_GIFT"] = DM_SYS_GIFT;
	m_cmdid["SYS_MSG"] = DM_SYS_MSG;
	m_cmdid["GUARD_MSG"] = DM_GUARD_MSG;
	m_cmdid["GUIARD_MSG"] = DM_GUIARD_MSG;
	// 7
	m_cmdid["DANMU_GIFT_LOTTERY_START"] = DM_DANMU_GIFT_LOTTERY_START;
	m_cmdid["DANMU_GIFT_LOTTERY_AWARD"] = DM_DANMU_GIFT_LOTTERY_AWARD;
	m_cmdid["DANMU_GIFT_LOTTERY_END"] = DM_DANMU_GIFT_LOTTERY_END;
	m_cmdid["ANCHOR_LOT_CHECKSTATUS"] = DM_ANCHOR_LOT_CHECKSTATUS;
	m_cmdid["ANCHOR_LOT_START"] = DM_ANCHOR_LOT_START;
	m_cmdid["ANCHOR_LOT_AWARD"] = DM_ANCHOR_LOT_AWARD;
	m_cmdid["ANCHOR_LOT_END"] = DM_ANCHOR_LOT_END;
	// 6
	m_cmdid["DANMU_MSG:4:0:2:2:2:0"] = DM_DANMU_MSG_402220;
	m_cmdid["USER_TOAST_MSG"] = DM_USER_TOAST_MSG;
	m_cmdid["LOTTERY_START"] = DM_LOTTERY_START;
	m_cmdid["room_admin_entrance"] = DM_room_admin_entrance;
	m_cmdid["new_anchor_reward"] = DM_new_anchor_reward;
	m_cmdid["DAILY_QUEST_NEWDAY"] = DM_DAILY_QUEST_NEWDAY;
}

void event_dmmsg::process_data(MSG_INFO *data)
{
	int ret;
	switch (data->type) {
	case 0x03: {
		// int value = str[1] << 16 | str[2] << 8 | str[3];
		// BOOST_LOG_SEV(g_logger::get(), trace) << "[DMMSG] " << room << " Popular:" << value;
		break;
	}
	case 0x05: {
		ret = ParseJSON(data);
		if (ret) {
			BOOST_LOG_SEV(g_logger::get(), debug) << "[DMMSG] " << data->id << " DMNEW: " << data->buff.get();
		}
		break;
	}
	case 0x08: {
		BOOST_LOG_SEV(g_logger::get(), info) << "[DMMSG] " << data->id << " Link start...";
		break;
	}
	default: {
		BOOST_LOG_SEV(g_logger::get(), error) << "[DMMSG] " << data->id << " Unknown msg type: " << data->type;
		break;
	}
	}
}

int event_dmmsg::ParseJSON(MSG_INFO *data) {
	rapidjson::Document doc;
	doc.Parse(data->buff.get());

	if (!doc.IsObject()) {
		BOOST_LOG_SEV(g_logger::get(), error) << "[DMMSG] " << data->id << " JSON Wrong.";
		return -1;
	}
	rapidjson::Value &value = doc;
	if (value.HasMember("cmd")) {
		if (!value["cmd"].IsString()) {
			BOOST_LOG_SEV(g_logger::get(), error) << "[DMMSG] " << data->id << " JSON Wrong.";
			return -1;
		}
	}
	else if (value.HasMember("msg") && value["msg"].IsObject() && value["msg"].HasMember("cmd")) {
		value = value["msg"];
		if (!value["cmd"].IsString()) {
			BOOST_LOG_SEV(g_logger::get(), error) << "[DMMSG] " << data->id << " JSON Wrong.";
			return -1;
		}
	}
	else {
		BOOST_LOG_SEV(g_logger::get(), error) << "[DMMSG] " << data->id << " JSON Wrong.";
		return -1;
	}
	std::string strtype = value["cmd"].GetString();
	if (!m_cmdid.count(strtype)) {
		// 新指令
		return -1;
	}
	int cmdid = m_cmdid[strtype];

	switch (cmdid) {
	case DM_DANMU_MSG: {
		return 0;
	}
	case DM_PREPARING:
	case DM_CUT_OFF: {
		event_base::post_close_msg(data->id, data->opt);
		return 0;
	}
	case DM_LIVE: {
		event_base::post_open_msg(data->id, data->opt);
		return 0;
	}
	case DM_ROOM_CHANGE: {
		if (data->opt & DM_PUBEVENT) {
			return ParseMsgRoomChange(value, data->id, data->opt);
		}
		return 0;
	}
	case DM_NOTICE_MSG: {
		if (data->opt & DM_PUBEVENT) {
			return ParseMsgNotice(value, data->id, DM_ROOM_AREA(data->opt));
		}
		return 0;
	}
	case DM_SPECIAL_GIFT: {
		if (data->opt & DM_HIDDENEVENT) {
			return ParseLotStorm(value, data->id);
		}
		return 0;
	}
	case DM_GUARD_LOTTERY_START: {
		if (data->opt & DM_HIDDENEVENT) {
			return ParseLotGuard(value, data->id);
		}
		return 0;
	}
	case DM_PK_LOTTERY_START: {
		if (data->opt & DM_HIDDENEVENT) {
			return ParseLotPK(value, data->id);
		}
		return 0;
	}
	case DM_DANMU_LOTTERY_START:
	case DM_DANMU_GIFT_LOTTERY_START: {
		if (data->opt & DM_HIDDENEVENT) {
			return ParseLotDanmu(value, data->id);
		}
		return 0;
	}
	case DM_ANCHOR_LOT_START: {
		if (data->opt & DM_HIDDENEVENT) {
			return ParseLotAnchor(value, data->id);
		}
		return 0;
	}
	}

	return 0;
}

int event_dmmsg::ParseMsgRoomChange(rapidjson::Value & doc, const unsigned room, const unsigned opt) {
	unsigned area_cur = DM_ROOM_AREA(opt);
	unsigned area_new = doc["data"]["parent_area_id"].GetUint();
	if (area_cur != area_new) {
		// 主分区发生更改
		BOOST_LOG_SEV(g_logger::get(), trace) << "[DMMSG] room area change " << room
			<< " from:" << area_cur << " to:" << area_new;
		event_base::post_close_msg(room, opt);
	}
	return 0;
}

int event_dmmsg::ParseMsgNotice(rapidjson::Value &doc, const unsigned room, const unsigned area) {
	// 如果消息不含有房间ID 则说明不是抽奖信息
	if (!doc.HasMember("msg_type") || !doc["msg_type"].IsInt()) {
		return -1;
	}
	int type = doc["msg_type"].GetInt();
	switch (type) {
	case 2:
	case 8: {
		return ParseNoticeGift(doc, room, area);
	}
	case 3: {
		return ParseNoticeGuard(doc, room, area);
	}
	case 1:
	case 4:
	case 5:
	case 6:
	case 9:
	case 10: {
		return 0;
	}
	}

	return -1;
}

int event_dmmsg::ParseNoticeGift(rapidjson::Value &doc, const unsigned room, const unsigned area) {
	// 如果消息不含有房间ID 则说明不是抽奖信息
	if (!doc.HasMember("real_roomid") || !doc["real_roomid"].IsInt()) {
		return -1;
	}
	int rrid = doc["real_roomid"].GetInt();

	// 如果是全区广播需要过滤重复消息
	std::string tstr = doc["msg_common"].GetString();
	std::wstring wmsg;
	toollib::UTF8ToUTF16(tstr, wmsg);
	if (wmsg.find(L"全区广播") != -1) {
		if (area != 1) {
			return 0;
		}
	}

	// 有房间号就进行抽奖
	event_base::post_lottery_pub(MSG_NOTICE_GIFT, rrid);

	return 0;
}

int event_dmmsg::ParseNoticeGuard(rapidjson::Value &doc, const unsigned room, const unsigned area) {
	// 过滤当前房间的开通信息
	std::string tstr = doc["msg_common"].GetString();
	if (tstr.find(u8"在本房间") != -1) {
		return 0;
	}
	// 全区广播只需通知一次
	if (area != 1) {
		return 0;
	}
	int rid = doc["real_roomid"].GetInt();
	event_base::post_lottery_pub(MSG_NOTICE_GUARD, rid);

	return 0;
}

int event_dmmsg::ParseLotStorm(rapidjson::Value &doc, const unsigned room) {
	if (!doc.HasMember("data") || !doc["data"].IsObject() || !doc["data"].HasMember("39")
		|| !doc["data"]["39"].IsObject() || !doc["data"]["39"].HasMember("action")) {
		return -1;
	}
	std::string tstr = doc["data"]["39"]["action"].GetString();
	if (tstr == "start") {
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
		std::shared_ptr<BILI_LOTTERYDATA> data(new BILI_LOTTERYDATA());
		auto curtime = toollib::GetTimeStamp();
		data->srid = room;
		data->rrid = room;
		data->loid = id;
		data->time_start = curtime;
		data->time_end = curtime + doc["data"]["39"]["time"].GetInt();
		data->time_get = curtime;
		data->type = "storm";
		if (doc["data"]["39"].HasMember("num") && doc["data"]["39"]["num"].IsInt()) {
			data->exinfo = doc["data"]["39"]["num"].GetInt();
		}
		if (doc["data"]["39"].HasMember("content") && doc["data"]["39"]["content"].IsString()) {
			data->title = doc["data"]["39"]["content"].GetString();
		}
		BOOST_LOG_SEV(g_logger::get(), trace) << "[DMMSG] storm " << room
			<< " num:" << data->exinfo << " content:" << data->title;
		event_base::post_lottery_hidden(MSG_LOT_STORM, data);
		return 0;
	}
	if (tstr == "end") {
		return 0;
	}
	return -1;
}

int event_dmmsg::ParseLotGuard(rapidjson::Value &doc, const unsigned room) {
	int btype = doc["data"]["privilege_type"].GetInt();
	if (btype != 1) {
		std::shared_ptr<BILI_LOTTERYDATA> data(new BILI_LOTTERYDATA());
		auto curtime = toollib::GetTimeStamp();
		data->srid = doc["data"]["room_id"].GetInt();
		data->rrid = room;
		data->loid = doc["data"]["id"].GetInt();
		data->time_end = curtime + doc["data"]["lottery"]["time"].GetInt();
		data->time_start = curtime;
		data->time_get = curtime + doc["data"]["lottery"]["time_wait"].GetInt();
		data->type = doc["data"]["lottery"]["keyword"].GetString();
		data->exinfo = btype;
		event_base::post_lottery_hidden(MSG_LOT_GUARD, data);
	}
	return 0;
}

int event_dmmsg::ParseLotPK(rapidjson::Value & doc, const unsigned room) {
	std::shared_ptr<BILI_LOTTERYDATA> data(new BILI_LOTTERYDATA());
	auto curtime = toollib::GetTimeStamp();
	data->srid = doc["data"]["room_id"].GetInt();
	data->rrid = room;
	data->loid = doc["data"]["pk_id"].GetInt();
	data->time_end = curtime + doc["data"]["time"].GetInt();
	data->time_start = data->time_end - doc["data"]["max_time"].GetInt();
	data->time_get = curtime;
	data->type = "pk";
	event_base::post_lottery_hidden(MSG_LOT_PK, data);
	return 0;
}

int event_dmmsg::ParseLotDanmu(rapidjson::Value & doc, const unsigned room) {
	std::shared_ptr<BILI_LOTTERYDATA> data(new BILI_LOTTERYDATA());
	auto curtime = toollib::GetTimeStamp();
	data->srid = doc["data"]["room_id"].GetInt();
	data->rrid = room;
	data->loid = doc["data"]["id"].GetInt();
	data->time_end = curtime + doc["data"]["time"].GetInt();
	data->time_start = data->time_end - doc["data"]["max_time"].GetInt();
	data->time_get = curtime;
	data->type = "danmu";
	data->title = doc["data"]["award_name"].GetString();
	data->exinfo = doc["data"]["award_num"].GetUint();
	event_base::post_lottery_hidden(MSG_LOT_DANMU, data);
	return 0;
}

int event_dmmsg::ParseLotAnchor(rapidjson::Value & doc, const unsigned room) {
	std::shared_ptr<BILI_LOTTERYDATA> data(new BILI_LOTTERYDATA());
	auto curtime = toollib::GetTimeStamp();
	data->srid = doc["data"]["room_id"].GetInt();
	data->rrid = room;
	data->loid = doc["data"]["id"].GetInt();
	data->time_end = curtime + doc["data"]["time"].GetInt();
	data->time_start = data->time_end - doc["data"]["max_time"].GetInt();
	data->time_get = curtime;
	data->type = "anchor";
	data->title = doc["data"]["award_name"].GetString();
	data->exinfo = doc["data"]["award_num"].GetUint();
	data->gift_id = doc["data"]["gift_id"].GetUint();
	data->gift_num = doc["data"]["gift_num"].GetUint();
	event_base::post_lottery_hidden(MSG_LOT_ANCHOR, data);
	return 0;
}
