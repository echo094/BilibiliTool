#pragma once
#include <map>
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
	}
	~event_dmmsg() {
	}

private:
	void InitCMD();

public:
	void process_data(MSG_INFO *data) override;

private:
	int ParseJSON(MSG_INFO *data);
	int ParseSTORMMSG(rapidjson::Document &doc, const unsigned room);
	int ParseNOTICEMSG(rapidjson::Document &doc, const unsigned room, const unsigned area);
	int ParseSYSMSG(rapidjson::Document &doc, const unsigned room, const unsigned area);
	int ParseGUARDMSG(rapidjson::Document &doc, const unsigned room, const unsigned area);
	int ParseGUARDLO(rapidjson::Document &doc, const unsigned room);

private:
	std::map<std::string, int> m_cmdid;
};
