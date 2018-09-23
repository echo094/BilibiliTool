#pragma once
#include <map>
#include <set>

enum class DANMU_FLAG {
	MSG_NONE = 0,
	MSG_PUBEVENT = 1,
	MSG_SPECIALGIFT = 2,
};

typedef struct tagRoomInfo {
	DANMU_FLAG flag;
	bool needconnect;

	tagRoomInfo() :
		flag(DANMU_FLAG::MSG_NONE),
		needconnect(false) {}

}ROOM_INFO;

class DanmuAPI {
public:
	DanmuAPI():
		parentthreadid(0),
		bdanmukuon(false),
		bmodedebug(false) {}
	virtual ~DanmuAPI() {}

public:
	void SetDanmukuOn() { bdanmukuon = true; }
	void SetDanmukuOff() { bdanmukuon = false; }
	// 设置父级线程ID
	int SetNotifyThread(DWORD id);

protected:
	int CheckMessage(const unsigned char *str);
	int ProcessData(const unsigned char* str, int len, int room, int type);
	int ParseJSON(char *str, int room);
	int ParseDANMUMSG(rapidjson::Document &doc, int room);
	int ParseSTORMMSG(rapidjson::Document &doc, int room);
	int ParseSYSGIFT(rapidjson::Document &doc, int room);
	int ParseSYSMSG(rapidjson::Document &doc, int room);
	int ParseGUARDMSG(rapidjson::Document &doc, int room);

protected:
	DWORD parentthreadid;//上级消息线程
	bool bdanmukuon;
	bool bmodedebug;
	CTools _tool;
	CStrConvert _strcoding;
	std::set<unsigned> m_rlist;
	std::map<unsigned, ROOM_INFO> m_rinfo;
};
