#pragma once
#include <string>
#include <boost/shared_array.hpp>

#ifdef WIN32
const char DEF_CONFIGGILE_NAME[] = "\\BiliConfig.json";
#else
const char DEF_CONFIGGILE_NAME[] = "/BiliConfig.json";
#endif

const char APP_KEY[] = "1d8b6e7d45233436";
const char APP_SECRET[] = "560c52ccd288fed045859ed18bffd973";
const char PARAM_BUILD[] = "5430400";

const char URL_DEFAULT_ORIGIN[] = "Origin: https://live.bilibili.com";
const char URL_DEFAULT_REFERERBASE[] = "Referer: https://live.bilibili.com/";
const char URL_DEFAULT_REFERER[] = "Referer: https://live.bilibili.com/3";
const std::string URL_LIVEAPI_HEAD = "https://api.live.bilibili.com";

enum class BILIRET {
	NOFAULT = 0,
	// 查询结果为0
	NORESULT,
	ROOM_BLOCK,
	JOINEVENT_FAILED,

	LOGIN_NAMEWRONG = 12,
	LOGIN_PASSWORDWRONG,
	LOGIN_NEEDVERIFY,

	HTTP_ERROR = 101,
	FILE_ERROR,
	OPENSSL_ERROR,
	RESCODE_ERROR,
	JSON_ERROR,
	HTMLTEXT_ERROR,
	// 十周年
	TEN_TASK_NOTOPEN,
	TEN_TASK_FINISH,
	TEN_LIKE_FAILED,
	TEN_RECV_FAILED,
	TEN_SHARE_FAILED,
	TEN_TEAM_FULL,
	TEN_TEAM_HASJOIN
};

typedef struct _MSG_INFO{
	unsigned id = 0;
	// 参数设置
	unsigned opt = 0;
	// 消息类型
	unsigned type = 0;
	// 数据包版本
	unsigned ver = 0;
	// 消息长度
	unsigned len = 0;
	// 消息内容
	boost::shared_array<char> buff;

}MSG_INFO;

typedef struct _BILI_LOTTERYDATA {
	// 房间短ID
	int srid = 0;
	// 房间真实ID
	int rrid = 0;
	// 事件编号
	long long loid = 0;
	// 开始时间
	time_t time_start = 0;
	// 失效时间
	time_t time_end = 0;
	// 事件类型
	// 抽奖为礼物编号 网页API统一为 Gift
	// 亲密为 guard
	// 风暴为 storm 实际没用
	// 大乱斗为 pk 实际没用
	std::string type;
	// Guard等级
	int exinfo = 0;
	// 抽奖的名称
	std::string title;
}BILI_LOTTERYDATA;
