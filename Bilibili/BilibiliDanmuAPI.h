#pragma once
#include <map>
#include <set>

enum class DANMU_FLAG {
	MSG_NONE = 0,
	MSG_PUBEVENT = 1,
	MSG_SPECIALGIFT = 2,
};

typedef struct _ROOM_INFO {
	// 房间监控类型
	DANMU_FLAG flag;
	// 区域
	int area;
	// 需要重连（心跳失败时）
	bool needconnect;
	// 需要关闭（下播时）
	bool needclose;

	_ROOM_INFO() :
		flag(DANMU_FLAG::MSG_NONE),
		area(0),
		needconnect(false),
		needclose(false) {}

}ROOM_INFO;

class DanmuAPI {
public:
	DanmuAPI():
		parentthreadid(0),
		bdanmukuon(false) {}
	virtual ~DanmuAPI() {}

public:
	void SetDanmukuOn() { bdanmukuon = true; }
	void SetDanmukuOff() { bdanmukuon = false; }
	// 设置父级线程ID
	int SetNotifyThread(DWORD id);

protected:
	int CheckMessage(const unsigned char *str);
	int ProcessData(const char* str, int len, int room, int type);
	int ParseJSON(const char *str, int room);
	int ParseDANMUMSG(rapidjson::Document &doc, int room);
	int ParseSTORMMSG(rapidjson::Document &doc, int room);
	int ParseSYSMSG(rapidjson::Document &doc, int room);
	int ParseGUARDMSG(rapidjson::Document &doc, int room);
	int ParseGUARDLO(rapidjson::Document &doc, int room);

protected:
	// 上级消息线程
	DWORD parentthreadid;
	// 连接的房间集合
	std::set<unsigned> m_rlist;
	// 房间信息map
	std::map<unsigned, ROOM_INFO> m_rinfo;

	bool bdanmukuon;
	CStrConvert _strcoding;
};
