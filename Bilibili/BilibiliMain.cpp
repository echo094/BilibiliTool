#include "stdafx.h"
#include "BilibiliMain.h"
#include "log.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <time.h>
#include <conio.h> //getch()

const int HEART_INTERVAL = 300;

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

CBilibiliMain::CBilibiliMain():
	io_context_(),
	heart_timer_(io_context_),
	_dmsource(nullptr),
	_apidm(new DanmuAPI()),
	_lotterytv(new CBilibiliSmallTV()),
	_lotterygu(new CBilibiliGuard()),
	_apilive(new CBilibiliLive()),
	_userlist(new CBilibiliUserList()) {

	curmode = TOOL_EVENT::STOP;
	m_curl = curl_easy_init();

	SaveLogFile();
}

CBilibiliMain::~CBilibiliMain() {
	_logfile.close();
	_dmsource.reset();
	_lotterytv.reset();
	_lotterygu.reset();
	_apilive.reset();
	_apidm.reset();
	_userlist.reset();
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
	
	// Create the worker threads
	pwork_ = std::make_shared< boost::asio::io_context::work>(io_context_);
	// Create thread
	thread_main_ = std::make_shared< std::thread>(
		boost::bind(&boost::asio::io_service::run, &io_context_)
		);

	_apidm->set_event_handler(
		std::bind(
			&CBilibiliMain::post_msg,
			this,
			std::placeholders::_1,
			std::placeholders::_2,
			std::placeholders::_3
		));

	PrintHelp();

	string command;
	int ret = 1;
	while (ret) {
		while (getline(cin, command) && !command.size());
		ret = ProcessCommand(command);
	}
	BOOST_LOG_SEV(g_logger::get(), info) << "[Main] Waiting to exit...";

	// Stop
	pwork_.reset();
	thread_main_->join();
	thread_main_.reset();

	return 0;
}

void CBilibiliMain::post_msg(unsigned msg, WPARAM wp, LPARAM lp) {
	boost::asio::post(
		io_context_,
		boost::bind(
			&CBilibiliMain::ProcessModuleMSG,
			this,
			msg,
			wp,
			lp
		)
	);
}

void CBilibiliMain::start_timer(unsigned sec) {
	heart_timer_.expires_from_now(boost::posix_time::seconds(sec));
	heart_timer_.async_wait(
		boost::bind(
			&CBilibiliMain::on_timer,
			this,
			boost::asio::placeholders::error
		)
	);
}

void CBilibiliMain::on_timer(boost::system::error_code ec) {
	if (ec) {
		BOOST_LOG_SEV(g_logger::get(), info) << "[Main] Timer: " << ec;
		return;
	}
	start_timer(HEART_INTERVAL);
	UpdateLiveRoom();
}

int CBilibiliMain::ProcessModuleMSG(unsigned msg, WPARAM wp, LPARAM lp) {
	switch (msg) {
	case MSG_NEWSMALLTV: {
		JoinTV(wp);
		break;
	}
	case MSG_NEWGUARD0: {
		// 房间上船事件
		BILI_LOTTERYDATA *pinfo = (BILI_LOTTERYDATA *)wp;
		JoinGuardGift(*pinfo);
		delete pinfo;
		break;
	}
	case MSG_NEWGUARD1: {
		// 广播上船事件
		JoinGuardGift(wp);
		break;
	}
	case MSG_NEWSPECIALGIFT: {
		BILI_ROOMEVENT *pinfo = (BILI_ROOMEVENT *)wp;
		JoinSpecialGift(pinfo->rid, pinfo->loidl);
		delete pinfo;
		break;
	}
	case MSG_CHANGEROOM1: {
		// 房间下播
		UpdateAreaRoom(wp, lp, true);
		break;
	}
	case MSG_CHANGEROOM2: {
		// 房间上播
		UpdateAreaRoom(wp, lp, false);
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
		StopMonitorALL();
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
		StopMonitorALL();
	}
	else if (!str.compare("11") || !str.compare("userexp")) {
		StartUserHeart();
	}
	else if (!str.compare("12") || !str.compare("startlp")) {
		StartMonitorPubEvent();
	}
	else if (!str.compare("13") || !str.compare("startlh")) {
		StartMonitorHiddenEvent();
	}
	else if (!str.compare("21")) {
		printf("Saving lottery history... \n");
		SaveLogFile();
	}
	else if (!str.compare("90")) {
		Debugfun(1);
	}
	else if (!str.compare("91")) {
		Debugfun(2);
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

		ret = _dmsource->stop();
		_dmsource = nullptr;

		BOOST_LOG_SEV(g_logger::get(), info) << "[Main] Monitor stopped.";
		ret = _userlist->WaitActThreadStop();
		BOOST_LOG_SEV(g_logger::get(), info) << "[Main] User Thread Clear.";
	}
	if (curmode == TOOL_EVENT::GET_HIDEN_GIFT) {
		BOOST_LOG_SEV(g_logger::get(), info) << "[Main] Closing socket threads...";

		heart_timer_.cancel();
		ret = _dmsource->stop();
		_dmsource = nullptr;

		BOOST_LOG_SEV(g_logger::get(), info) << "[Main] Monitor stopped.";
		ret = _userlist->WaitActThreadStop();
		BOOST_LOG_SEV(g_logger::get(), info) << "[Main] User Thread Clear.";
	}
	curmode = TOOL_EVENT::STOP;

	return ret;
}

int CBilibiliMain::StartUserHeart() {
	if (curmode != TOOL_EVENT::STOP) {
		printf("Another task is working. \n");
		return 0;
	}
	curmode = TOOL_EVENT::ONLINE;

	int ret = 0;
	ret = _userlist->StartUserHeart();

	return ret;
}

int CBilibiliMain::StartMonitorPubEvent() {
	if (curmode != TOOL_EVENT::STOP) {
		printf("Another task is working. \n");
		return 0;
	}
	curmode = TOOL_EVENT::GET_SYSMSG_GIFT;

	if (_dmsource == nullptr) {
		_dmsource = std::make_unique<CWSDanmu>();
		_dmsource->set_msg_handler(
			std::bind(&event_base::process_data, _apidm, std::placeholders::_1)
		);
		_dmsource->start();
	}

	// 清空错过抽奖列表
	_lotterytv->ClearMissingLottery();

	// 获取需要连接的房间
	unsigned roomid;
	for (unsigned int i = 1; i < 6; i++) {
		if (_apilive->PickOneRoom(m_curl, roomid, 0, i) == BILIRET::NOFAULT) {
			ROOM_INFO info;
			info.id = roomid;
			info.opt = DM_ROOM_AREA(i) | DM_PUBEVENT;
			_dmsource->add_context(roomid, info);
		}
	}

	return 0;
}

int CBilibiliMain::StartMonitorHiddenEvent() {
	if (curmode != TOOL_EVENT::STOP) {
		printf("Another task is working. \n");
		return 0;
	}
	curmode = TOOL_EVENT::GET_HIDEN_GIFT;

	start_timer(HEART_INTERVAL);

	if (_dmsource == nullptr) {
		_dmsource = std::make_unique<source_dmasio>();
		_dmsource->set_msg_handler(
			std::bind(&event_base::process_data, _apidm, std::placeholders::_1)
		);
		_dmsource->start();
	}

	// 连接符合人气条件的开播房间
	UpdateLiveRoom();

	return 0;
}

int CBilibiliMain::Debugfun(int index) {
	if (index == 1) {
		if (_dmsource) {
			_dmsource->show_stat();
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

int CBilibiliMain::UpdateAreaRoom(const unsigned rid, const unsigned area, const bool opt) {
	if (curmode == TOOL_EVENT::GET_SYSMSG_GIFT) {
		_dmsource->set_con_stat(rid, opt);
		if (!opt) {
			return 0;
		}
		_dmsource->del_context(rid);
		unsigned nrid;
		if (_apilive->PickOneRoom(m_curl, nrid, rid, area) == BILIRET::NOFAULT) {
			ROOM_INFO info;
			info.id = rid;
			info.opt = DM_ROOM_AREA(area) | DM_PUBEVENT;
			_dmsource->add_context(nrid, info);
			return 0;
		}
		return 0;
	}
	if (curmode == TOOL_EVENT::GET_HIDEN_GIFT) {
		_dmsource->set_con_stat(rid, opt);
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
	_dmsource->update_context(nlist, DM_HIDDENEVENT);

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
