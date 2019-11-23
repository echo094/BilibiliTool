#pragma once
#include <map>
#include <set>
#include "event/event_base.h"
#include "rapidjson/document.h"

#define DM_ROOM_AREA(x) (x & 0xf)

const unsigned DM_PUBEVENT = 0x10;
const unsigned DM_HIDDENEVENT = 0x20;

class event_dmmsg :
	public event_base {
public:
	event_dmmsg() {
		InitCMD();
		InitGift();
	}
	~event_dmmsg() {
	}

private:
	/**
	 * @brief 初始化消息类型列表
	 */
	void InitCMD();
	/**
	 * @brief 初始化全区礼物列表
	 */
	void InitGift();

public:
	void process_data(MSG_INFO *data) override;

private:
	/**
	 * @brief 处理常规消息
	 *
	 * @param data  消息数据包
	 *
	 * @return
	 *   数据包正确返回0 错误返回-1
	 */
	int ParseJSON(MSG_INFO *data);
	/**
	 * @brief 当前直播间的频道切换消息
	 *
	 * @param doc   消息数据包
	 * @param room  订阅的直播间号
	 * @param opt   该IO的配置参数
	 *
	 * @return
	 *   返回0
	 */
	int ParseMsgRoomChange(rapidjson::Value &doc, const unsigned room, const unsigned opt);
	/**
	 * @brief 广播的通知消息
	 *
	 * @param doc   消息数据包
	 * @param room  订阅的直播间号
	 * @param area  订阅的直播间分区
	 *
	 * @return
	 *   数据包正确返回0 错误返回-1
	 */
	int ParseMsgNotice(rapidjson::Value &doc, const unsigned room, const unsigned area);
	/**
	 * @brief 广播的礼物抽奖消息
	 *
	 * @param doc   消息数据包
	 * @param room  订阅的直播间号
	 * @param area  订阅的直播间分区
	 *
	 * @return
	 *   数据包正确返回0 错误返回-1
	 */
	int ParseNoticeGift(rapidjson::Value &doc, const unsigned room, const unsigned area);
	/**
	 * @brief 广播或当前直播间的守护抽奖消息 只处理总督消息
	 *
	 * @param doc   消息数据包
	 * @param room  订阅的直播间号
	 * @param area  订阅的直播间分区
	 *
	 * @return
	 *   数据包正确返回0 错误返回-1
	 */
	int ParseNoticeGuard(rapidjson::Value &doc, const unsigned room, const unsigned area);
	/**
	 * @brief 当前直播间的节奏风暴抽奖消息
	 *
	 * @param doc   消息数据包
	 * @param room  订阅的直播间号
	 *
	 * @return
	 *   数据包正确返回0 错误返回-1
	 */
	int ParseLotStorm(rapidjson::Value &doc, const unsigned room);
	/**
	 * @brief 当前直播间的礼物抽奖消息
	 *
	 * 目前只处理 30405|30406 这两个房间内礼物
	 *
	 * @param doc   消息数据包
	 * @param room  订阅的直播间号
	 *
	 * @return
	 *   数据包正确返回0 错误返回-1
	 */
	int ParseLotGift(rapidjson::Value &doc, const unsigned room);
	/**
	 * @brief 当前直播间的守护抽奖消息 只处理舰长以及提督消息
	 *
	 * @param doc   消息数据包
	 * @param room  订阅的直播间号
	 *
	 * @return
	 *   数据包正确返回0 错误返回-1
	 */
	int ParseLotGuard(rapidjson::Value &doc, const unsigned room);
	/**
	 * @brief 当前直播间的PK抽奖消息
	 *
	 * @param doc   消息数据包
	 * @param room  订阅的直播间号
	 *
	 * @return
	 *   数据包正确返回0 错误返回-1
	 */
	int ParseLotPK(rapidjson::Value &doc, const unsigned room);
	/**
	 * @brief 当前直播间的弹幕抽奖消息
	 *
	 * @param doc   消息数据包
	 * @param room  订阅的直播间号
	 *
	 * @return
	 *   数据包正确返回0 错误返回-1
	 */
	int ParseLotDanmu(rapidjson::Value &doc, const unsigned room);
	/**
	 * @brief 当前直播间的天选时刻抽奖消息
	 *
	 * @param doc   消息数据包
	 * @param room  订阅的直播间号
	 *
	 * @return
	 *   数据包正确返回0 错误返回-1
	 */
	int ParseLotAnchor(rapidjson::Value &doc, const unsigned room);

private:
	/**
	 * @brief 指令列表
	 */
	std::map<std::string, int> m_cmdid;
	/**
	 * @brief 全区礼物列表
	 */
	std::set<unsigned> m_gift;
};
