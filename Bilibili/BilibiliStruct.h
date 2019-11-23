#pragma once
#include <string>
#include <boost/shared_array.hpp>

#ifdef WIN32
const char DEF_CONFIG_USER[] = "\\ConfigUser.json";
const char DEF_CONFIG_SERVER[] = "\\ConfigServer.json";
const char DEF_CONFIG_GIFT[] = "\\ConfigGift.txt";
#else
const char DEF_CONFIG_USER[] = "/ConfigUser.json";
const char DEF_CONFIG_SERVER[] = "/ConfigServer.json";
const char DEF_CONFIG_GIFT[] = "/ConfigGift.txt";
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
	JOIN_AGAIN,

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

enum {
	MSG_NOTICE_GIFT = 1,
	MSG_NOTICE_GUARD,
	MSG_LOT_GIFT,
	MSG_LOT_GUARD,
	MSG_LOT_STORM,
	MSG_LOT_PK,
	MSG_LOT_DANMU,
	MSG_LOT_ANCHOR,
	MSG_CHANGEROOM1,
	MSG_CHANGEROOM2,
	MSG_CLOSEROOM
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

struct BILI_LOTTERYDATA {
	/**
	 * @brief 数据包计数
	 */
	static unsigned count;
	/**
	 * @brief 剩余数据包计数
	 */
	static unsigned remain;
	/**
	 * @brief 抽奖类型
	 */
	int cmd = 0;
	/**
	 * @brief 房间短ID
	 */
	int srid = 0;
	/**
	 * @brief 房间真实ID
	 */
	int rrid = 0;
	/**
	 * @brief 事件编号
	 */
	long long loid = 0;
	/**
	 * @brief 事件开始时间
	 */
	time_t time_start = 0;
	/**
	 * @brief 事件可参与时间
	 */
	time_t time_get = 0;
	/**
	 * @brief 事件结束时间
	 *
	 * 节奏风暴会提前结束
	 *
	 */
	time_t time_end = 0;
	/**
	 * @brief 事件类型
	 *
	 * - 风暴 storm
	 * - 礼物 礼物的type
	 * - 守护 guard
	 * - PK   pk
	 * - 弹幕 danmu
	 * - 天选 anchor
	 *
	 */
	std::string type;
	/**
	 * @brief 额外记录的数量数据
	 *
	 * - 守护 privilege_type
	 * - 弹幕 award_num
	 * - 天选 award_num
	 *
	 */
	// 
	unsigned exinfo = 0;
	/**
	 * @brief 额外记录的抽奖信息 名称或奖品
	 *
	 * - 礼物 thank_text
	 * - 弹幕 award_name
	 * - 天选 award_name
	 *
	 */
	std::string title;
	/**
	 * @brief 天选抽奖所需送的礼物编号
	 */
	unsigned gift_id = 0;
	/**
	 * @brief 天选抽奖所需送的礼物数量
	 */
	unsigned gift_num = 0;

	BILI_LOTTERYDATA();
	BILI_LOTTERYDATA(const BILI_LOTTERYDATA& item);
	~BILI_LOTTERYDATA();
};
