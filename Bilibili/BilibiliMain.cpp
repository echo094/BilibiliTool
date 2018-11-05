#include "stdafx.h"
#include "BilibiliMain.h"
#include "log.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <time.h>

CBilibiliMain::CBilibiliMain(CURL *pcurl){
	curmode = TOOL_EVENT::STOP;
	curl = pcurl;

	char name[50];
	sprintf_s(name, "[BILIMAIN]%sActHis.log", GetTimeString().c_str());
	_logfile.open(name, std::ios::out);
	_logfile.close();
	_logfile.open(name, std::ios::in | std::ios::out);

	_tcpdanmu = nullptr;
	_wsdanmu = nullptr;
	_lotterytv = std::make_unique<CBilibiliSmallTV>();
	_lotterygu = std::make_unique<CBilibiliGuard>();
	_apilive = std::make_unique<CBilibiliLive>();
	_userlist = std::make_unique<CBilibiliUserList>();
}

CBilibiliMain::~CBilibiliMain() {
	_logfile.close();
	curl = NULL;
	_tcpdanmu = nullptr;
	_wsdanmu = nullptr;
	_lotterytv = nullptr;
	_lotterygu = nullptr;
	_apilive = nullptr;
	_userlist = nullptr;
	BOOST_LOG_SEV(g_logger::get(), debug) << "[Main] Stop.";
}

int CBilibiliMain::SetCURLHandle(CURL *pcurl) {
	curl = pcurl;
	return 0;
}

unique_ptr<CBilibiliUserList> &CBilibiliMain::GetUserList(int index) {
	return _userlist;
}

int CBilibiliMain::SaveLogFile() {
	int ret = 0;
	char name[50];
	sprintf_s(name, "[BILIMAIN]%sActHis.log", GetTimeString().c_str());
	_logfile.close();
	_logfile.open(name, std::ios::out);
	_logfile.close();
	_logfile.open(name, std::ios::in | std::ios::out);

	return ret;
}

// 在1点和10点之间不参加抽奖
bool CBilibiliMain::isSkip() {
	time_t time;
	std::time(&time);
	// 需要考虑时区
	auto sec = (time + 28800) % 86400;
	if ((sec > 3600) && (sec < 36000)) {
		return true;
	}
	return false;
}

int CBilibiliMain::StopMonitorALL() {
	if (curmode == TOOL_EVENT::STOP)
		return 0;
	int ret = -1;

	if (curmode == TOOL_EVENT::ONLINE) {
		ret = _userlist->StopUserHeart();
		BOOST_LOG_SEV(g_logger::get(), info) << "[Main] Heart stopped.";
	}
	if (curmode == TOOL_EVENT::GET_SYSMSG_GIFT) {
		BOOST_LOG_SEV(g_logger::get(), info) << "[Main] Closing ws threads...";

		_wsdanmu->SetNotifyThread(0);
		ret = _wsdanmu->Deinit();
		_wsdanmu = nullptr;

		BOOST_LOG_SEV(g_logger::get(), info) << "[Main] Monitor stopped.";
		ret = _userlist->WaitActThreadStop();
		BOOST_LOG_SEV(g_logger::get(), info) << "[Main] User Thread Clear.";
	}
	if (curmode == TOOL_EVENT::GET_HIDEN_GIFT) {
		BOOST_LOG_SEV(g_logger::get(), info) << "[Main] Closing socket threads...";

		_tcpdanmu->SetNotifyThread(0);
		ret = _tcpdanmu->Deinit();
		_tcpdanmu = nullptr;

		BOOST_LOG_SEV(g_logger::get(), info) << "[Main] Monitor stopped.";
		ret = _userlist->WaitActThreadStop();
		BOOST_LOG_SEV(g_logger::get(), info) << "[Main] User Thread Clear.";
	}
	curmode = TOOL_EVENT::STOP;

	return ret;
}

int CBilibiliMain::StartUserHeart() {
	int ret = 0;
	curmode = TOOL_EVENT::ONLINE;
	ret = _userlist->StartUserHeart();

	return ret;
}

int CBilibiliMain::StartMonitorPubEvent(int pthreadid) {
	curmode = TOOL_EVENT::GET_SYSMSG_GIFT;

	if (_wsdanmu == nullptr) {
		_wsdanmu = std::make_unique<CWSDanmu>();
		_wsdanmu->SetNotifyThread(pthreadid);
		_wsdanmu->Init();
	}

	// 清空错过抽奖列表
	_lotterytv->ClearMissingLottery();

	// 获取需要连接的房间
	unsigned roomid;
	for (unsigned int i = 1; i < 5; i++) {
		if (_apilive->PickOneRoom(curl, roomid, 0, i) == BILIRET::NOFAULT) {
			_wsdanmu->ConnectToRoom(roomid, i, DANMU_FLAG::MSG_PUBEVENT);
		}
	}

	return 0;
}

int CBilibiliMain::StartMonitorHiddenEvent(int pthreadid) {
	curmode = TOOL_EVENT::GET_HIDEN_GIFT;

	if (_tcpdanmu == nullptr) {
		_tcpdanmu = std::make_unique<CBilibiliDanmu>();
		_tcpdanmu->SetNotifyThread(pthreadid);
		_tcpdanmu->Init(DANMU_MODE::MULTI_ROOM);
	}

	// 连接符合人气条件的开播房间
	UpdateLiveRoom();

	return 0;
}

void CBilibiliMain::SetDanmukuShow()
{
	if (curmode == TOOL_EVENT::GET_SYSMSG_GIFT) {
		_wsdanmu->SetDanmukuOn();
	}
	if (curmode == TOOL_EVENT::GET_HIDEN_GIFT) {
		_tcpdanmu->SetDanmukuOn();
	}
}

void CBilibiliMain::SetDanmukuHide()
{
	if (curmode == TOOL_EVENT::GET_SYSMSG_GIFT) {
		_wsdanmu->SetDanmukuOff();
	}
	if (curmode == TOOL_EVENT::GET_HIDEN_GIFT) {
		_tcpdanmu->SetDanmukuOff();
	}
}

int CBilibiliMain::UpdateAreaRoom(const unsigned rid, const unsigned area) {
	if (curmode != TOOL_EVENT::GET_SYSMSG_GIFT) {
		return 0;
	}
	_wsdanmu->DisconnectFromRoom(rid);
	unsigned nrid;
	if (_apilive->PickOneRoom(curl, nrid, rid, area) == BILIRET::NOFAULT) {
		_wsdanmu->ConnectToRoom(nrid, area, DANMU_FLAG::MSG_PUBEVENT);
		return 0;
	}

	return -1;
}

int CBilibiliMain::UpdateLiveRoom() {
	if (curmode != TOOL_EVENT::GET_HIDEN_GIFT) {
		return 0;
	}
	std::set<unsigned> nlist;
	_apilive->GetLiveList(curl, nlist, 400);
	_tcpdanmu->UpdateRoom(nlist, DANMU_FLAG::MSG_SPECIALGIFT);

	return 0;
}

int CBilibiliMain::JoinTV(int room)
{
	int ret = -1, count = 2;
	ret = _lotterytv->CheckLottery(curl, room);
	while (ret&&count) {
		Sleep(1000);
		ret = _lotterytv->CheckLottery(curl, room);
		count--;
	}
	if (ret != 0)
		return -1;
	BILI_LOTTERYDATA pdata;
	while (_lotterytv->GetNextLottery(pdata) == 0) {
		_logfile << "{time:" << GetTimeStamp()
			<< ",type:'" << pdata.type
			<< "',ruid:" << pdata.rrid
			<< ",loid:" << pdata.loid
			<< "}," << std::endl;
		
		if (isSkip()) {
			continue;
		}
		_userlist->JoinTVALL(&pdata);
	}
	return 0;
}

int CBilibiliMain::JoinGuardGift(int room)
{
	int ret = -1, count = 2;
	ret = _lotterygu->CheckLottery(curl, room);
	while (ret&&count) {
		Sleep(1000);
		ret = _lotterygu->CheckLottery(curl, room);
		count--;
	}
	if (ret != 0)
		return -1;
	BILI_LOTTERYDATA pdata;
	while (_lotterygu->GetNextLottery(pdata) == 0) {
		_logfile << "{time:" << GetTimeStamp()
			<< ",type:'" << pdata.type << '_' << pdata.exinfo
			<< "',ruid:" << pdata.rrid
			<< ",loid:" << pdata.loid
			<< "}," << std::endl;

		if (isSkip()) {
			continue;
		}
		_userlist->JoinGuardALL(pdata);
	}

	return 0;
}

int CBilibiliMain::JoinGuardGift(BILI_LOTTERYDATA &pdata)
{
	_logfile << "{time:" << GetTimeStamp()
		<< ",type:'" << pdata.type << '_' << pdata.exinfo
		<< "',ruid:" << pdata.rrid
		<< ",loid:" << pdata.loid
		<< "}," << std::endl;

	if (isSkip()) {
		return 0;
	}
	_userlist->JoinGuardALL(pdata);

	return 0;
}

int CBilibiliMain::JoinSpecialGift(int room, long long cid)
{
	_logfile << "{time:" << GetTimeStamp()
		<< ",type:'" << "storm"
		<< "',ruid:" << room
		<< ",loid:" << cid
		<< "}," << std::endl;

	if (isSkip()) {
		return 0;
	}
	_userlist->JoinSpecialGiftALL(room, cid);

	return 0;
}

int CBilibiliMain::Debugfun(int index)
{
	if (index == 1) {
		if (curmode == TOOL_EVENT::GET_SYSMSG_GIFT) {
			_wsdanmu->ShowCount();
		}
		if (curmode == TOOL_EVENT::GET_HIDEN_GIFT) {
			_tcpdanmu->ShowCount();
		}
	}
	if (index == 2) {
		if (curmode == TOOL_EVENT::GET_SYSMSG_GIFT) {
			_lotterytv->ShowMissingLottery();
		}
		if (curmode == TOOL_EVENT::GET_HIDEN_GIFT) {
			UpdateLiveRoom();
		}
	}
	return 0;
}
