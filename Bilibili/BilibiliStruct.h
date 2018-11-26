#pragma once
#include <string>
#include <boost/shared_array.hpp>

const char DEF_CONFIGGILE_NAME[] = "\\BiliConfig.ini";
const unsigned MSG_BILI_THREADCLOSE = WM_USER + 611;

const char APP_KEY[] = "1d8b6e7d45233436";
const char APP_SECRET[] = "560c52ccd288fed045859ed18bffd973";

const char URL_DEFAULT_ORIGIN[] = "Origin: https://live.bilibili.com";
const char URL_DEFAULT_REFERERBASE[] = "Referer: https://live.bilibili.com/";
const char URL_DEFAULT_REFERER[] = "Referer: https://live.bilibili.com/3";
const char URL_LIVEAPI_HEAD[] = "https://api.live.bilibili.com";

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
	HTMLTEXT_ERROR
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

typedef struct _BILI_ROOMEVENT
{
	int rid = 0;
	int loids = 0;
	int exinfo = 0;
	long long loidl = 0; 
}BILI_ROOMEVENT;

typedef struct _BILI_LOTTERYDATA
{
	time_t time = 0;
	int srid = 0, rrid = 0;
	int loid = 0;
	int exinfo = 0;
	std::string type;
}BILI_LOTTERYDATA, *PBILI_LOTTERYDATA;
