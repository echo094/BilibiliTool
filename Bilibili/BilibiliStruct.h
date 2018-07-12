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

enum DANMUSOCKETFLAG
{
	MSG_PUBEVENT = 1,
	MSG_SPECIALGIFT = 2,
};

typedef enum _BILIRESCODE
{
	SUCCESS_G = 0,
	STATUS_NOTLOGIN,
	STATUS_NOTVALID,
	STATUS_NOTINVITED,
	STATUS_DRAWINVIALID,

	UNSUCCESS_G = 10,
	LOGIN_UNSUCCESS = UNSUCCESS_G + 1,
	LOGIN_NAMEWRONG,
	LOGIN_PASSWORDWRONG,
	LOGIN_NEEDVERIFY,

	ERROR_G = -100,
	HTTP_ERROR = ERROR_G - 1,
	FILE_ERROR = ERROR_G - 2,
	OPENSSL_ERROR = ERROR_G - 3,
	RESCODE_ERROR = ERROR_G - 4,
	ACCOUNT_IMPORT_FAILED = ERROR_G - 5,
	ACCOUNT_EXPORT_FAILED = ERROR_G - 6,

	JSON_ERROR = ERROR_G - 105,
	JSON_DATAMISSING,
	JSON_DATAERROR,

	DANMU_ERROR = ERROR_G - 310,
	DANMU_JSON_ERROR,
	DANMU_JSON_DATAMISSING,
	DANMU_JSON_DATAERROR,
}BILIRESCODE;

//功能模块编号
typedef enum _BILI_MODULES
{
	BILI_STOP = 10,
	BILI_ONLINE,
	BILI_GET_SYSMSG_GIFT,
	BILI_GET_HIDEN_GIFT,
	BILI_DEBUG1,
	BILI_DEBUG2,
}BILI_MODULES;

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
