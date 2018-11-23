#pragma once
#include <map>

#define DM_ROOM_AREA(x) (x & 0xf)

class DanmuAPI {
public:
	DanmuAPI():
		parentthreadid(0) {
		InitCMD();
	}
	~DanmuAPI() {}

public:
	// 设置父级线程ID
	int SetNotifyThread(DWORD id);
	void ProcessData(MSG_INFO *data);

protected:
	void InitCMD();
	int ParseJSON(MSG_INFO *data);
	int ParseSTORMMSG(rapidjson::Document &doc, const unsigned room);
	int ParseNOTICEMSG(rapidjson::Document &doc, const unsigned room, const unsigned area);
	int ParseSYSMSG(rapidjson::Document &doc, const unsigned room, const unsigned area);
	int ParseGUARDMSG(rapidjson::Document &doc, const unsigned room, const unsigned area);
	int ParseGUARDLO(rapidjson::Document &doc, const unsigned room);

protected:
	// 上级消息线程
	DWORD parentthreadid;
	// 指令列表
	std::map<std::string, int> m_cmdid;

	CStrConvert _strcoding;
};
