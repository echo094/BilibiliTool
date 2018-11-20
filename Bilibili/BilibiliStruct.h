﻿#pragma once
#include <string>

const char DEF_CONFIGGILE_NAME[] = "\\BiliConfig.ini";
const unsigned MSG_BILI_THREADCLOSE = WM_USER + 611;

// 服务器地址与WS数据处理
// https://api.live.bilibili.com/room/v1/Danmu/getConf?room_id=23058&platform=pc&player=web
// https://s1.hdslb.com/bfs/static/blive/live-assets/player/decorator.min.js?

const char DM_TCPSERVER[] = "broadcastlv.chat.bilibili.com";
const int DM_TCPPORT = 2243;
const char DM_WSSERVER[] = "ws://broadcastlv.chat.bilibili.com:2244/sub";
const char DM_WSSSERVER[] = "wss://broadcastlv.chat.bilibili.com:443/sub";
const char DM_UA[] = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/70.0.3538.102 Safari/537.36";

const char APP_KEY[] = "1d8b6e7d45233436";
const char APP_SECRET[] = "560c52ccd288fed045859ed18bffd973";

const char URL_DEFAULT_ORIGIN[] = "Origin: https://live.bilibili.com";
const char URL_DEFAULT_REFERERBASE[] = "Referer: https://live.bilibili.com/";
const char URL_DEFAULT_REFERER[] = "Referer: https://live.bilibili.com/3";
const char URL_LIVEAPI_HEAD[] = "https://api.live.bilibili.com";

const unsigned MSG_NEWSMALLTV = WM_USER + 612;
const unsigned MSG_NEWSPECIALGIFT = WM_USER + 613;
const unsigned MSG_NEWGUARD1 = WM_USER + 614;
const unsigned MSG_NEWGUARD0 = WM_USER + 615;
const unsigned MSG_CHANGEROOM = WM_USER + 616;

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

typedef struct _BILI_ROOMEVENT
{
	int rid = 0;
	int loids = 0;
	int exinfo;
	long long loidl; 
}BILI_ROOMEVENT;

typedef struct _BILI_LOTTERYDATA
{
	time_t time = 0;
	int srid = 0, rrid = 0;
	int loid = 0;
	int exinfo;
	std::string type;
}BILI_LOTTERYDATA, *PBILI_LOTTERYDATA;
