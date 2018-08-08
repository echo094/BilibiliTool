#pragma once
#include <string>

#define DEF_CONFIGGILE_NAME "\\BiliConfig.ini"
#define MSG_BILI_THREADCLOSE WM_USER + 611

const char DM_TCPSERVER[] = "livecmt-2.bilibili.com";
const int DM_TCPPORT = 2243;
const char DM_WSSERVER[] = "ws://broadcastlv.chat.bilibili.com:2244/sub";
const char DM_WSSSERVER[] = "wss://broadcastlv.chat.bilibili.com:443/sub";

const char APP_KEY[20] = "1d8b6e7d45233436";
const char APP_SECRET[40] = "560c52ccd288fed045859ed18bffd973";

#define URL_DEFAULT_ORIGIN "Origin: https://live.bilibili.com"
#define URL_DEFAULT_REFERERBASE "Referer: https://live.bilibili.com/"
#define URL_DEFAULT_REFERER "Referer: https://live.bilibili.com/3"
#define URL_LIVEAPI_HEAD "https://api.live.bilibili.com"

#define DEF_MAX_ROOM 1000
#define MSG_NEWSMALLTV WM_USER + 612
#define MSG_NEWSPECIALGIFT WM_USER + 613
#define MSG_NEWYUNYING WM_USER + 615
#define MSG_NEWYUNYINGDAILY WM_USER + 616
#define MSG_NEWGUARD WM_USER + 617

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

struct tagSPECIALGIFT
{
	int num = 0;
	int rtime = 0;
	long long id; 
	std::string content;
};

typedef struct _BILI_LOTTERYDATA
{
	time_t time = 0;
	int srid = 0, rrid = 0;
	int loid = 0;
	std::string type;
}BILI_LOTTERYDATA, *PBILI_LOTTERYDATA;
