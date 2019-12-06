#include "BilibiliMain.h"
#include <fstream>
#include <iostream>
#include <time.h>
#include "logger/log.h"
#include "dest/api_bl.h"
#include "event/event_dmmsg.h"
#include "source/source_dmws.h"
#include "source/source_dmasio.h"
#include "utility/platform.h"
#include "utility/strconvert.h"

const int HEART_INTERVAL = 300; 

CBilibiliMain::CBilibiliMain() :
	io_context_(),
	heart_timer_(io_context_),
	heart_flag_(false),
	_lotterytv(new lottery_list()),
	_lotterygu(new guard_list()),
	_apilive(new CBilibiliLive()),
	_apidm(new event_dmmsg()),
	_dmsource(nullptr),
	_userlist(new dest_user()) {

	curmode = TOOL_EVENT::STOP;
	curl_main_ = curl_easy_init();
	curl_heart_ = curl_easy_init();

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
	curl_easy_cleanup(curl_main_);
	curl_easy_cleanup(curl_heart_);
	BOOST_LOG_SEV(g_logger::get(), debug) << "[Main] Stop.";
}

void CBilibiliMain::PrintHelp() {
	printf("\n Bilibili Tool \n");
	printf(u8"\n\
  >  账号相关 \n\
  > 1 导入用户列表         \n\
  > 2 导出用户列表         \n\
  > 3 显示用户列表         \n\
  > 4 添加新用户           \n\
  > 5 删除用户             \n\
  > 6 重新登录             \n\
  > 7 检测账户Cookie有效性 \n\
  > 8 获取账户信息         \n\
  >  挂机模块 \n\
  >10 关闭所有领取         \n\
  >11 开启领取经验         \n\
  >12 开启广播类事件监控   \n\
  >13 开启非广播类事件监控 \n\
  >14 开启账户抽奖         \n\
  >21 更新日志文件         \n\
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

	_apidm->set_event_act(
		std::bind(
			&CBilibiliMain::post_msg_act,
			this,
			std::placeholders::_1
		));

	_apidm->set_event_room(
		std::bind(
			&CBilibiliMain::post_msg_room,
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

void CBilibiliMain::post_msg_room(unsigned msg, unsigned rrid, unsigned opt) {
	boost::asio::post(
		io_context_,
		boost::bind(
			&CBilibiliMain::ProcessMSGRoom,
			this,
			msg,
			rrid,
			opt
		)
	);
}

void CBilibiliMain::post_msg_act(std::shared_ptr<BILI_LOTTERYDATA> data) {
	boost::asio::post(
		io_context_,
		boost::bind(
			&CBilibiliMain::ProcessMSGAct,
			this,
			data->cmd,
			data
		)
	);
}

void CBilibiliMain::set_timer_refresh(unsigned sec) {
	heart_timer_.expires_from_now(boost::posix_time::seconds(sec));
	heart_timer_.async_wait(
		boost::bind(
			&CBilibiliMain::on_timer_refresh,
			this,
			boost::asio::placeholders::error
		)
	);
}

void CBilibiliMain::on_timer_refresh(boost::system::error_code ec) {
	if (ec) {
		BOOST_LOG_SEV(g_logger::get(), info) << "[Main] Timer refresh: " << ec;
		return;
	}
	if (thread_heart_ && thread_heart_->joinable()) {
		thread_heart_->join();
	}
	if (heart_flag_) {
		// 在停止时定时器会延迟收到取消信号
		// 若两操作同时发生 可能会有问题
		return;
	}
	set_timer_refresh(HEART_INTERVAL);
	thread_heart_.reset(new std::thread(
		&CBilibiliMain::UpdateLiveRoom, this
	));
}

void CBilibiliMain::set_timer_userheart(unsigned sec, unsigned type) {
	heart_timer_.expires_from_now(boost::posix_time::seconds(sec));
	heart_timer_.async_wait(
		boost::bind(
			&CBilibiliMain::on_timer_userheart,
			this,
			boost::asio::placeholders::error,
			type
		)
	);
}

void CBilibiliMain::on_timer_userheart(boost::system::error_code ec, unsigned type) {
	if (ec) {
		BOOST_LOG_SEV(g_logger::get(), info) << "[Main] Timer userheart: " << ec;
		return;
	}
	if (thread_heart_ && thread_heart_->joinable()) {
		thread_heart_->join();
	}
	if (heart_flag_) {
		// 在停止时定时器会延迟收到取消信号
		// 若两操作同时发生 可能会有问题
		return;
	}
	set_timer_userheart(60, 0);
	thread_heart_.reset(new std::thread(
		&CBilibiliMain::HeartExp, this, type
	));
}

int CBilibiliMain::ProcessMSGRoom(unsigned msg, unsigned rrid, unsigned opt) {
	switch (msg) {
	case MSG_CLOSEROOM:
	case MSG_CHANGEROOM1: {
		// 房间下播
		UpdateAreaRoom(rrid, DM_ROOM_AREA(opt), true);
		break;
	}
	case MSG_CHANGEROOM2: {
		// 房间上播
		UpdateAreaRoom(rrid, DM_ROOM_AREA(opt), false);
		break;
	}
	}
	return 0;
}

int CBilibiliMain::ProcessMSGAct(unsigned msg, std::shared_ptr<BILI_LOTTERYDATA> data) {
	switch (msg) {
	case MSG_NOTICE_GIFT: {
		CheckLotGift(data);
		break;
	}
	case MSG_NOTICE_GUARD: {
		CheckLotGuard(data);
		break;
	}
	case MSG_LOT_STORM: {
		JoinLotStorm(data);
		break;
	}
	case MSG_LOT_GIFT: {
		JoinLotGift(data);
		break;
	}
	case MSG_LOT_GUARD: {
		JoinLotGuard(data);
		break;
	}
	case MSG_LOT_PK: {
		JoinLotPK(data);
		break;
	}
	case MSG_LOT_DANMU: {
		JoinLotDanmu(data);
		break;
	}
	case MSG_LOT_ANCHOR: {
		JoinLotAnchor(data);
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
	else if (!str.compare("1")) {
		boost::asio::post(
			io_context_,
			[this] {
				this->_userlist->ImportUserList();
		});
	}
	else if (!str.compare("2")) {
		boost::asio::post(
			io_context_,
			[this] {
			this->_userlist->ExportUserList();
		});
	}
	else if (!str.compare("3")) {
		boost::asio::post(
			io_context_,
			[this] {
			this->_userlist->ShowUserList();
		});
	}
	else if (!str.compare("4")) {
		std::string name, psd;
		cout << "Account: ";
		getline(cin, name);
		cout << "Password: ";
		GetPassword(psd);
		// 由于可能需要输入验证码 必须在主线程执行
		this->_userlist->AddUser(name, psd);
	}
	else if (!str.compare("5")) {
		char tstr[30];
		cout << "Account: ";
		cin >> tstr;
		boost::asio::post(
			io_context_,
			[this, tstr] {
			this->_userlist->DeleteUser(tstr);
		});
	}
	else if (!str.compare("6")) {
		// 由于可能需要输入验证码 必须在主线程执行
		this->_userlist->ReloginAll();
	}
	else if (!str.compare("7")) {
		boost::asio::post(
			io_context_,
			[this] {
			this->_userlist->CheckUserStatusALL();
		});
	}
	else if (!str.compare("8")) {
		boost::asio::post(
			io_context_,
			[this] {
			this->_userlist->GetUserInfoALL();
		});
	}
	else if (!str.compare("10")) {
		StopMonitorALL();
	}
	else if (!str.compare("11")) {
		StartUserHeart();
	}
	else if (!str.compare("12")) {
		unsigned port;
		cout << "Port: ";
		cin >> port;
		StartMonitorPubEvent(port);
	}
	else if (!str.compare("13")) {
		unsigned port;
		cout << "Port: ";
		cin >> port;
		StartMonitorHiddenEvent(port);
	}
	else if (!str.compare("14")) {
		StartJoinLottery();
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
		printf(u8"未知命令\n");
	}
	return 1;
}

int CBilibiliMain::StopMonitorALL() {
	if (curmode == TOOL_EVENT::STOP)
		return 0;
	int ret = -1;

	if (curmode == TOOL_EVENT::ONLINE) {
		heart_flag_ = true;
		heart_timer_.cancel();
		if (thread_heart_ && thread_heart_->joinable()) {
			thread_heart_->join();
		}
		BOOST_LOG_SEV(g_logger::get(), info) << "[Main] Heart stopped.";
	}
	if (curmode == TOOL_EVENT::GET_SYSMSG_GIFT) {
		BOOST_LOG_SEV(g_logger::get(), info) << "[Main] Closing ws threads...";

		ret = _dmsource->stop();
		_dmsource = nullptr;

		dest_notice_ = nullptr;

		BOOST_LOG_SEV(g_logger::get(), info) << "[Main] Monitor stopped.";
	}
	if (curmode == TOOL_EVENT::GET_HIDEN_GIFT) {
		BOOST_LOG_SEV(g_logger::get(), info) << "[Main] Closing socket threads...";

		heart_flag_ = true;
		heart_timer_.cancel();
		if (thread_heart_ && thread_heart_->joinable()) {
			thread_heart_->join();
		}
		ret = _dmsource->stop();
		_dmsource = nullptr;

		dest_notice_ = nullptr;

		BOOST_LOG_SEV(g_logger::get(), info) << "[Main] Monitor stopped.";
	}
	if (curmode == TOOL_EVENT::JOIN_LOTTERY) {
		src_notice_->stop();
		src_notice_ = nullptr;

		BOOST_LOG_SEV(g_logger::get(), info) << "[Main] Monitor stopped.";
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

	heart_flag_ = false;
	set_timer_userheart(1, 1);

	return 0;
}

int CBilibiliMain::StartMonitorPubEvent(unsigned port) {
	if (curmode != TOOL_EVENT::STOP) {
		printf("Another task is working. \n");
		return 0;
	}
	curmode = TOOL_EVENT::GET_SYSMSG_GIFT;

	try
	{
		if (dest_notice_ == nullptr) {
			dest_notice_.reset(new dest_client(port));
		}

		if (_dmsource == nullptr) {
			_dmsource.reset(new source_dmws());
			_dmsource->set_msg_handler(
				std::bind(&event_base::process_data, _apidm, std::placeholders::_1)
			);
			_dmsource->set_close_handler(
				std::bind(
					&event_base::connection_close,
					_apidm,
					std::placeholders::_1,
					std::placeholders::_2)
			);
			_dmsource->start();
		}

		// 清空错过抽奖列表
		_lotterytv->ClearMissingLottery();

		// 获取key
		std::string key;
		if (apibl::APIWebv1DanmuConf(curl_main_, 23058, "web", key) != BILIRET::NOFAULT) {
			printf("Start failed! \n");
			return -1;
		}
		// 获取需要连接的房间
		unsigned num = 0;
		if (_apilive->GetAreaNum(curl_main_, num) != BILIRET::NOFAULT) {
			printf("Start failed! \n");
			return -1;
		}
		unsigned roomid;
		for (unsigned int i = 1; i <= num; i++) {
			if (_apilive->PickOneRoom(curl_main_, roomid, 0, i) == BILIRET::NOFAULT) {
				ROOM_INFO info;
				info.id = roomid;
				info.opt = i | DM_PUBEVENT;
				info.key = key;
				_dmsource->add_context(roomid, info);
			}
		}
	}
	catch (std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << "\n";
	}

	return 0;
}

int CBilibiliMain::StartMonitorHiddenEvent(unsigned port) {
	if (curmode != TOOL_EVENT::STOP) {
		printf("Another task is working. \n");
		return 0;
	}
	curmode = TOOL_EVENT::GET_HIDEN_GIFT;

	try
	{
		heart_flag_ = false;

		if (dest_notice_ == nullptr) {
			dest_notice_.reset(new dest_client(port));
		}

		if (_dmsource == nullptr) {
			_dmsource.reset(new source_dmasio());
			_dmsource->set_msg_handler(
				std::bind(&event_base::process_data, _apidm, std::placeholders::_1)
			);
			_dmsource->set_close_handler(
				std::bind(
					&event_base::connection_close,
					_apidm,
					std::placeholders::_1,
					std::placeholders::_2)
			);
			_dmsource->start();
		}

		// 连接符合人气条件的开播房间
		// 在IO线程中开始操作
		set_timer_refresh(1);
	}
	catch (std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << "\n";
	}

	return 0;
}

int CBilibiliMain::StartJoinLottery()
{
	if (curmode != TOOL_EVENT::STOP) {
		printf("Another task is working. \n");
		return 0;
	}
	curmode = TOOL_EVENT::JOIN_LOTTERY;

	if (src_notice_ == nullptr) {
		src_notice_.reset(new source_notice());
		src_notice_->start();
		src_notice_->set_event_act(
			std::bind(
				&CBilibiliMain::post_msg_act,
				this,
				std::placeholders::_1
			));
	}

	return 0;
}

int CBilibiliMain::Debugfun(int index) {
	if (index == 1) {
		if (_dmsource) {
			_dmsource->show_stat();
		}
		if (src_notice_) {
			src_notice_->show_stat();
		}
	}
	if (index == 2) {
		std::shared_ptr<BILI_LOTTERYDATA> data(new BILI_LOTTERYDATA());
		data->cmd = MSG_LOT_GIFT;
		data->rrid = 1000;
		data->loid = 500000;
		PostLottery(data);
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
		if (_apilive->PickOneRoom(curl_main_, nrid, rid, area) != BILIRET::NOFAULT) {
			return -1;
		}
		ROOM_INFO info;
		info.id = rid;
		info.opt = area | DM_PUBEVENT;
		if (apibl::APIWebv1DanmuConf(curl_main_, rid, "web", info.key) != BILIRET::NOFAULT) {
			return -1;
		}
		_dmsource->add_context(nrid, info);
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
	// 获取新列表
	std::set<unsigned> nlist;
	_apilive->GetLiveList(curl_heart_, nlist, 400);
	// 清理现有房间
	_dmsource->clean_context(nlist);
	// 获取key
	std::string key;
	if (apibl::APIWebv1DanmuConf(curl_main_, 23058, "flash", key) != BILIRET::NOFAULT) {
		return -1;
	}
	// 连接新增房间
	for (auto it = nlist.begin(); it != nlist.end(); it++) {
		ROOM_INFO info;
		info.id = *it;
		info.opt = DM_HIDDENEVENT;
		info.key = key;
		_dmsource->add_context(*it, info);
	}

	return 0;
}

// 抽奖事件
// 输入含有 rrid
int CBilibiliMain::CheckLotGift(std::shared_ptr<BILI_LOTTERYDATA> data)
{
	int ret = -1, count = 2;
	ret = _lotterytv->CheckLottery(curl_main_, data);
	while (ret&&count) {
		Sleep(1000);
		ret = _lotterytv->CheckLottery(curl_main_, data);
		count--;
	}
	if (ret != 0) {
		return -1;
	}
	while (1) {
		data = _lotterytv->GetNextLottery();
		if (data == nullptr) {
			return 0;
		}
		JoinLotGift(data);
	}
	return 0;
}

// 广播上船事件
// 含有 rrid
int CBilibiliMain::CheckLotGuard(std::shared_ptr<BILI_LOTTERYDATA> data)
{
	int ret = -1, count = 2;
	ret = _lotterygu->CheckLottery(curl_main_, data);
	while (ret&&count) {
		Sleep(1000);
		ret = _lotterygu->CheckLottery(curl_main_, data);
		count--;
	}
	if (ret != 0) {
		return -1;
	}
	while (1) {
		data = _lotterygu->GetNextLottery();
		if (data == nullptr) {
			return 0;
		}
		JoinLotGuard(data);
	}

	return 0;
}

int CBilibiliMain::JoinLotGift(std::shared_ptr<BILI_LOTTERYDATA> data)
{
	_logfile << "{time:" << data->time_start
		<< ",type:'" << data->type
		<< "',ruid:" << data->rrid
		<< ",loid:" << data->loid
		<< ",title:'" << data->title
		<< "'}," << std::endl;

	PostLottery(data);

	return 0;
}

int CBilibiliMain::JoinLotGuard(std::shared_ptr<BILI_LOTTERYDATA> data)
{
	_logfile << "{time:" << data->time_start
		<< ",type:'" << data->type << '_' << data->exinfo
		<< "',ruid:" << data->rrid
		<< ",loid:" << data->loid
		<< "}," << std::endl;

	PostLottery(data);

	return 0;
}

int CBilibiliMain::JoinLotStorm(std::shared_ptr<BILI_LOTTERYDATA> data)
{
	_logfile << "{time:" << data->time_start
		<< ",type:'" << data->type
		<< "',ruid:" << data->rrid
		<< ",loid:" << data->loid
		<< "}," << std::endl;

	PostLottery(data);

	return 0;
}

int CBilibiliMain::JoinLotPK(std::shared_ptr<BILI_LOTTERYDATA> data)
{
	_logfile << "{time:" << data->time_start
		<< ",type:'" << data->type
		<< "',ruid:" << data->rrid
		<< ",loid:" << data->loid
		<< "}," << std::endl;
	
	PostLottery(data);

	return 0;
}

int CBilibiliMain::JoinLotDanmu(std::shared_ptr<BILI_LOTTERYDATA> data)
{
	_logfile << "{time:" << data->time_start
		<< ",type:'" << data->type
		<< "',ruid:" << data->rrid
		<< ",loid:" << data->loid
		<< ",award_name:'" << data->title
		<< "',award_num:" << data->exinfo
		<< "}," << std::endl;

	PostLottery(data);

	return 0;
}

int CBilibiliMain::JoinLotAnchor(std::shared_ptr<BILI_LOTTERYDATA> data)
{
	_logfile << "{time:" << data->time_start
		<< ",type:'" << data->type
		<< "',ruid:" << data->rrid
		<< ",loid:" << data->loid
		<< ",award_name:'" << data->title
		<< "',award_num:" << data->exinfo
		<< ",join_type:" << data->join_type
		<< ",require_type:" << data->require_type
		<< ",require_value:" << data->require_value
		<< ",gift_id:" << data->gift_id
		<< ",gift_num:" << data->gift_num
		<< "}," << std::endl;

	// 需要送礼的不领取
	if (data->join_type) {
		return 0;
	}

	PostLottery(data);

	return 0;
}

void CBilibiliMain::PostLottery(std::shared_ptr<BILI_LOTTERYDATA> data)
{
	if (curmode == TOOL_EVENT::JOIN_LOTTERY) {
		if (isSkip()) {
			return;
		}
		_userlist->JoinLottery(data);
	}
	else {
		dest_notice_->post_lottery(data);
	}
}

int CBilibiliMain::HeartExp(unsigned type) {
	_userlist->HeartExp(type);
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
	sprintf(name, "[BILIMAIN]%sActHis.log", toollib::GetTimeString().c_str());
	if (_logfile.is_open()) {
		_logfile.close();
	}
	_logfile.open(name, std::ios::out);
	_logfile.close();
	_logfile.open(name, std::ios::in | std::ios::out);

	return ret;
}
