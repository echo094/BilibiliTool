#include "stdafx.h"
#include "BilibiliMain.h"
#include "log.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <time.h>
#include <conio.h> //getch()

int GetPassword(std::string &psd) {
	using namespace std;

	int ret = 0;
	char ch;
	unsigned int ich;
	psd = "";

	while (1) {
		ich = _getch();
		if (!ich) {
			continue;
		}
		//case cursor move
		if (ich == 224) {
			ch = _getch();
			continue;
		}
		ch = ich;
		//case enter
		if (ch == 13) {
			if (psd.size() > 0) {
				cout << '\n';
				return 0;
			}
			cout << "\nPassword is empty. Please reenter. \n";
			continue;
		}
		//case backspace
		if (ch == 8)
		{
			if (psd.size() == 0)
				continue;
			psd.erase(psd.end() - 1);
			cout << "\b \b";
			continue;
		}
		//noral case
		psd += ch;
		cout << "*";
	}

	return 0;
}

CBilibiliMain::CBilibiliMain(){
	curmode = TOOL_EVENT::STOP;
	m_curl = curl_easy_init();
	m_threadstat = false;

	SaveLogFile();

	_tcpdanmu = nullptr;
	_wsdanmu = nullptr;
	_lotterytv = std::make_unique<CBilibiliSmallTV>();
	_lotterygu = std::make_unique<CBilibiliGuard>();
	_apilive = std::make_unique<CBilibiliLive>();
	_userlist = std::make_unique<CBilibiliUserList>();
}

CBilibiliMain::~CBilibiliMain() {
	_logfile.close();
	_tcpdanmu = nullptr;
	_wsdanmu = nullptr;
	_lotterytv = nullptr;
	_lotterygu = nullptr;
	_apilive = nullptr;
	_userlist = nullptr;
	curl_easy_cleanup(m_curl);
	BOOST_LOG_SEV(g_logger::get(), debug) << "[Main] Stop.";
}

void CBilibiliMain::PrintHelp() {
	printf("\n Bilibili Tool \n");
	printf("\n\
  > 1 userimport  \t 导入用户列表         \n\
  > 2 userexport  \t 导出用户列表         \n\
  > 3 userlist    \t 显示用户列表         \n\
  > 4 useradd     \t 添加新用户           \n\
  > 5 userdel     \t 删除用户             \n\
  > 6 userre      \t 重新登录             \n\
  > 7 userlogin   \t 检测账户Cookie有效性 \n\
  > 8 usergetinfo \t 获取账户信息         \n\
  >10 stopall     \t 关闭所有领取         \n\
  >11 userexp     \t 开启领取经验         \n\
  >12 startlp     \t 开启广播类事件监控   \n\
  >13 startlh     \t 开启非广播类事件监控 \n\
  >21 savelog     \t 更新日志文件         \n\
  >   help        \t 目录                \n\
  >   exit        \t 退出                \n");
}

int CBilibiliMain::Run() {
	using namespace std;
;
	// 辅助线程
	m_threadhandle = CreateThread(NULL, 0, ThreadEntry, this, 0, &m_threadid);
	if (!m_threadhandle) {
		m_threadhandle = INVALID_HANDLE_VALUE;
		return -1;
	}
	PrintHelp();

	string command;
	int ret = 1;
	while (ret) {
		printf("> ");
		while (getline(cin, command) && !command.size());
		ret = ProcessCommand(command);
	}
	BOOST_LOG_SEV(g_logger::get(), info) << "[Main] Waiting to exit...";
	while (m_threadstat) {
		Sleep(100);
	}

	return 0;
}

DWORD CBilibiliMain::ThreadEntry(PVOID lpParameter) {
	CBilibiliMain *self = (CBilibiliMain *)lpParameter;
	self->m_threadstat = true;
	BOOST_LOG_SEV(g_logger::get(), debug) << "[Main] Thread Start.";
	self->ThreadHandler();
	BOOST_LOG_SEV(g_logger::get(), debug) << "[Main] Thread Stop.";
	self->m_threadstat = false;
	return 0;
}

void CBilibiliMain::ThreadHandler() {
	m_timer = 0;
	MSG msg;
	// 用于线程退出循环的标志位
	int runflag = 0;
	while (!runflag) {
		//Peek不阻塞但占用内存,如果使用GetMessage会阻塞
		if (GetMessage(&msg, NULL, 0, 0) == 0) {
			continue;
		}
		TranslateMessage(&msg);
		if (msg.message == WM_TIMER) {
			if (msg.wParam == m_timer) {
				UpdateLiveRoom();
			}
		}
		else if (msg.message == ON_USER_COMMAND) {
			TOOL_EVENT opt = static_cast<TOOL_EVENT>(msg.wParam);
			runflag = ProcessUserMSG(opt);
		}
		else {
			runflag = ProcessModuleMSG(msg);
		}
		Sleep(0);
	}
}

int CBilibiliMain::ProcessUserMSG(TOOL_EVENT &msg) {
	int ret = 0;
	if (msg == TOOL_EVENT::EXIT) {
		ret = -1;
		if (m_timer) {
			KillTimer(NULL, m_timer);
			m_timer = 0;
		}
		StopMonitorALL();
	}
	else if (msg == TOOL_EVENT::STOP) {
		if (m_timer) {
			KillTimer(NULL, m_timer);
			m_timer = 0;
		}
		StopMonitorALL();
	}
	else if (msg == TOOL_EVENT::ONLINE) {
		if (curmode != TOOL_EVENT::STOP) {
			printf("Another task is working. \n");
		}
		else {
			StartUserHeart();
		}
	}
	else if (msg == TOOL_EVENT::GET_SYSMSG_GIFT) {
		if (curmode != TOOL_EVENT::STOP) {
			printf("Another task is working. \n");
		}
		else {
			StartMonitorPubEvent(m_threadid);
		}
	}
	else if (msg == TOOL_EVENT::GET_HIDEN_GIFT) {
		if (curmode != TOOL_EVENT::STOP) {
			printf("Another task is working. \n");
		}
		else {
			// 每5分钟刷新房间
			m_timer = SetTimer(NULL, 1, 300000, NULL);
			StartMonitorHiddenEvent(m_threadid);
		}
	}
	else if (msg == TOOL_EVENT::DEBUG1) {
		Debugfun(1);
	}
	else if (msg == TOOL_EVENT::DEBUG2) {
		Debugfun(2);
	}
	return ret;
}

int CBilibiliMain::ProcessModuleMSG(MSG &msg) {
	switch (msg.message) {
	case MSG_NEWSMALLTV: {
		JoinTV(msg.wParam);
		break;
	}
	case MSG_NEWGUARD0: {
		// 房间上船事件
		BILI_LOTTERYDATA *pinfo = (BILI_LOTTERYDATA *)msg.wParam;
		JoinGuardGift(*pinfo);
		delete pinfo;
		break;
	}
	case MSG_NEWGUARD1: {
		// 广播上船事件
		JoinGuardGift(msg.wParam);
		break;
	}
	case MSG_NEWSPECIALGIFT: {
		BILI_ROOMEVENT *pinfo = (BILI_ROOMEVENT *)msg.wParam;
		JoinSpecialGift(pinfo->rid, pinfo->loidl);
		delete pinfo;
		break;
	}
	case MSG_CHANGEROOM: {
		// 房间下播
		UpdateAreaRoom(msg.wParam, msg.lParam);
		break;
	}
	}
	return 0;
}

int CBilibiliMain::ProcessCommand(std::string str) {
	using namespace std;

	if (str == "") {
		return 1;
	}
	if (!str.compare("exit")) {
		CloseHandle(m_threadhandle);
		PostThreadMessage(m_threadid, ON_USER_COMMAND, WPARAM(0), LPARAM(0));
		return 0;
	}
	if (!str.compare("help")) {
		PrintHelp();
	}
	else if (!str.compare("1") || !str.compare("userimport")) {
		_userlist->ImportUserList();
	}
	else if (!str.compare("2") || !str.compare("userexport")) {
		_userlist->ExportUserList();
	}
	else if (!str.compare("3") || !str.compare("userlist")) {
		_userlist->ShowUserList();
	}
	else if (!str.compare("4") || !str.compare("useradd")) {
		std::string name, psd;
		cout << "Account: ";
		getline(cin, name);
		cout << "Password: ";
		GetPassword(psd);
		_userlist->AddUser(name, psd);
	}
	else if (!str.compare("5") || !str.compare("userdel")) {
		char tstr[30];
		cout << "Account: ";
		cin >> tstr;
		_userlist->DeleteUser(tstr);
	}
	else if (!str.compare("6") || !str.compare("userre")) {
		_userlist->ReloginAll();
	}
	else if (!str.compare("7") || !str.compare("userlogin")) {
		_userlist->CheckUserStatusALL();
	}
	else if (!str.compare("8") || !str.compare("usergetinfo")) {
		_userlist->GetUserInfoALL();
	}
	else if (!str.compare("10") || !str.compare("stopall")) {
		PostThreadMessage(m_threadid, ON_USER_COMMAND, WPARAM(TOOL_EVENT::STOP), LPARAM(0));
	}
	else if (!str.compare("11") || !str.compare("userexp")) {
		PostThreadMessage(m_threadid, ON_USER_COMMAND, WPARAM(TOOL_EVENT::ONLINE), LPARAM(0));
	}
	else if (!str.compare("12") || !str.compare("startlp")) {
		PostThreadMessage(m_threadid, ON_USER_COMMAND, WPARAM(TOOL_EVENT::GET_SYSMSG_GIFT), LPARAM(0));
	}
	else if (!str.compare("13") || !str.compare("startlh")) {
		PostThreadMessage(m_threadid, ON_USER_COMMAND, WPARAM(TOOL_EVENT::GET_HIDEN_GIFT), LPARAM(0));
	}
	else if (!str.compare("21")) {
		printf("Saving lottery history... \n");
		SaveLogFile();
	}
	else if (!str.compare("90")) {
		PostThreadMessage(m_threadid, ON_USER_COMMAND, WPARAM(TOOL_EVENT::DEBUG1), LPARAM(0));
	}
	else if (!str.compare("91")) {
		PostThreadMessage(m_threadid, ON_USER_COMMAND, WPARAM(TOOL_EVENT::DEBUG2), LPARAM(0));
	}
	else {
		printf("未知命令\n");
	}
	return 1;
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
		if (_apilive->PickOneRoom(m_curl, roomid, 0, i) == BILIRET::NOFAULT) {
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

int CBilibiliMain::Debugfun(int index) {
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

int CBilibiliMain::UpdateAreaRoom(const unsigned rid, const unsigned area) {
	if (curmode != TOOL_EVENT::GET_SYSMSG_GIFT) {
		return 0;
	}
	_wsdanmu->DisconnectFromRoom(rid);
	unsigned nrid;
	if (_apilive->PickOneRoom(m_curl, nrid, rid, area) == BILIRET::NOFAULT) {
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
	_apilive->GetLiveList(m_curl, nlist, 400);
	_tcpdanmu->UpdateRoom(nlist, DANMU_FLAG::MSG_SPECIALGIFT);

	return 0;
}

int CBilibiliMain::JoinTV(int room)
{
	int ret = -1, count = 2;
	ret = _lotterytv->CheckLottery(m_curl, room);
	while (ret&&count) {
		Sleep(1000);
		ret = _lotterytv->CheckLottery(m_curl, room);
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
	ret = _lotterygu->CheckLottery(m_curl, room);
	while (ret&&count) {
		Sleep(1000);
		ret = _lotterygu->CheckLottery(m_curl, room);
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

int CBilibiliMain::SaveLogFile() {
	int ret = 0;
	char name[50];
	sprintf_s(name, "[BILIMAIN]%sActHis.log", GetTimeString().c_str());
	if (_logfile.is_open()) {
		_logfile.close();
	}
	_logfile.open(name, std::ios::out);
	_logfile.close();
	_logfile.open(name, std::ios::in | std::ios::out);

	return ret;
}
