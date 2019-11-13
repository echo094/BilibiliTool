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
	int ParseSTORMMSG(rapidjson::Value &alue, const unsigned room);
	int ParseNOTICEMSG(rapidjson::Value &doc, const unsigned room, const unsigned area);
	int ParseSYSMSG(rapidjson::Value &doc, const unsigned room, const unsigned area);
	int ParseGUARDMSG(rapidjson::Value &doc, const unsigned room, const unsigned area);
	int ParseGUARDLO(rapidjson::Value &doc, const unsigned room);
	int ParsePKLOTTERY(rapidjson::Value &doc, const unsigned room);
	int ParseROOMMSG(rapidjson::Value &doc, const unsigned room, const unsigned opt);

private:
	std::map<std::string, int> m_cmdid;
};
