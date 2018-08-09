#include "stdafx.h"
#include "BilibiliMain.h"
#include <fstream>

CBilibiliMain::CBilibiliMain(CURL *pcurl):
	_tcpdanmu(NULL),
	_wsdanmu(NULL) {

	curmode = TOOL_EVENT::STOP;
	curl = pcurl;

	char name[50];
	sprintf_s(name, "[BILIMAIN]%sActHis.log", _tool.GetTimeString().c_str());
	_logfile.open(name, std::ios::out);
	_logfile.close();
	_logfile.open(name, std::ios::in | std::ios::out);

	_roomcount = 0;
	_lotterytv = new CBilibiliSmallTV;
	_lotteryyy = new CBilibiliYunYing;
	_apilive = new CBilibiliLive;
	_userlist = new CBilibiliUserList;
}

CBilibiliMain::~CBilibiliMain() {
	_logfile.close();
	curl = NULL;
	delete _tcpdanmu;
	delete _wsdanmu;
	delete _lotterytv;
	delete _lotteryyy;
	delete _apilive;
	delete _userlist;
#ifdef _DEBUG
	printf("[Main] Stop. \n");
#endif
}

int CBilibiliMain::SetCURLHandle(CURL *pcurl) {
	curl = pcurl;
	return 0;
}

CBilibiliUserList* CBilibiliMain::GetUserList(int index) {
	return _userlist;
}

int CBilibiliMain::SaveLogFile() {
	int ret = 0;
	char name[50];
	sprintf_s(name, "[BILIMAIN]%sActHis.log", _tool.GetTimeString().c_str());
	_logfile.close();
	_logfile.open(name, std::ios::out);
	_logfile.close();
	_logfile.open(name, std::ios::in | std::ios::out);

	return ret;
}


int CBilibiliMain::StopMonitorALL() {
	if (curmode == TOOL_EVENT::STOP)
		return 0;
	int ret = -1;

	if (curmode == TOOL_EVENT::ONLINE) {
		ret = _userlist->StopUserHeart();
		printf("[Main] Heart stopped.\n");
	}
	if (curmode == TOOL_EVENT::GET_SYSMSG_GIFT) {
		printf("[Main] Closing ws threads...\n");

		_wsdanmu->SetNotifyThread(0);
		ret = _wsdanmu->Deinit();
		delete _wsdanmu;
		_wsdanmu = NULL;

		_roomcount = 0;
		printf("[Main] Monitor stopped.\n");
		ret = _userlist->WaitActThreadStop();
		printf("[Main] User Thread Clear.\n");
	}
	if (curmode == TOOL_EVENT::GET_HIDEN_GIFT) {
		printf("[Main] Closing socket threads...\n");

		_tcpdanmu->SetNotifyThread(0);
		ret = _tcpdanmu->Deinit();
		delete _tcpdanmu;
		_tcpdanmu = NULL;

		_roomcount = 0;
		printf("[Main] Monitor stopped.\n");
		ret = _userlist->WaitActThreadStop();
		printf("[Main] User Thread Clear.\n");
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
	int ret = -1;
	curmode = TOOL_EVENT::GET_SYSMSG_GIFT;

	if (!_wsdanmu) {
		_wsdanmu = new CWSDanmu;
	}
	_roomcount++;
	if (_roomcount == 1) {
		ret = _wsdanmu->SetNotifyThread(pthreadid);
		_wsdanmu->Init();
	}

	// 获取需要连接的房间
	char filepath[MAX_PATH];
	GetCurrentDirectoryA(sizeof(filepath), filepath);
	strcat_s(filepath, DEF_CONFIGGILE_NAME);
	int troom = ::GetPrivateProfileIntA("Global", "Room", 23058, filepath);
	ret = _wsdanmu->ConnectToRoom(troom, DANMU_FLAG::MSG_PUBEVENT);

	printf("[Main] Curent Room Num:%d \n", _roomcount);
	return ret;
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
		sprintf_s(_logbuff, sizeof(_logbuff), "{time:%I64d,type:'%s',ruid:%d,loid:%d},\n", _tool.GetTimeStamp(), pdata.type.c_str(), pdata.rrid, pdata.loid);
		_logfile.write(_logbuff, strlen(_logbuff));

		_userlist->JoinTVALL(&pdata);
	}
	return 0;
}

int CBilibiliMain::JoinYunYing(int room)
{
	int ret = -1, count = 2;
	ret = _lotteryyy->CheckLottery(curl, room);
	while (ret&&count) {
		Sleep(1000);
		ret = _lotteryyy->CheckLottery(curl, room);
		count--;
	}
	if (ret != 0)
		return -1;
	BILI_LOTTERYDATA pdata;
	while (_lotteryyy->GetNextLottery(pdata) == 0) {
		sprintf_s(_logbuff, sizeof(_logbuff), "{time:%I64d,type:'Raffle',ruid:%d,loid:%d},\n", _tool.GetTimeStamp(), pdata.rrid, pdata.loid);
		_logfile.write(_logbuff, strlen(_logbuff));

		_userlist->JoinYunYingALL(pdata);
	}
	return 0;
}

int CBilibiliMain::JoinYunYingGift(int room)
{
	_userlist->JoinYunYingGiftALL(room);
	return 0;
}

int CBilibiliMain::JoinGuardGift(const char *user)
{
	BILIRET ret;
	BILI_LOTTERYDATA pdata;
	ret = _apilive->ApiSearchUser(curl, user, pdata.rrid);
	if (ret != BILIRET::NOFAULT) {
		return -1;
	}
	ret = _apilive->ApiCheckGuard(curl, pdata.rrid, pdata.loid);
	if (ret != BILIRET::NOFAULT) {
		return -1;
	}
	pdata.type = "guard";
	_userlist->JoinGuardALL(pdata);

	return 0;
}

int CBilibiliMain::JoinSpecialGift(int room, long long cid, std::string str)
{
	int ret;
	ret = _userlist->JoinSpecialGiftALL(room, cid, str);
	sprintf_s(_logbuff, sizeof(_logbuff), "{time:%I64d,type:'storm',ruid:%d,loid:%I64d},\n", _tool.GetTimeStamp(), room, cid);
	_logfile.write(_logbuff, strlen(_logbuff));
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
	}
	return 0;
}
