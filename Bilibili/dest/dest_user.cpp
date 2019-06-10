#include "stdafx.h"
#include "dest_user.h"
#include <fstream>
#include <functional>
#include <thread>
#include "logger/log.h"

dest_user::dest_user() :
	_parentthread(0),
	_threadcount(0),
	_heartcount(0),
	_usercount(0) {

	GetDir(_cfgfile, sizeof(_cfgfile));
	strcat(_cfgfile, DEF_CONFIGGILE_NAME);

	pubkey = "-----BEGIN PUBLIC KEY-----\
\nMIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQDqDG9LqVmteJ3xsWv6u/lAt6cU\
\n3KTF4BDNZsSs+zmARQnBnHkaV4nJgRl9IK8b8tCMK6xbFrWa6a9RUENL8vWbclo4\
\nvuqG1/qLtZgo+eFzXbT3tg/XLUrLsdKhU5+w1YJWdw4TQUMbsR0z1F0yOZDwAvRC\
\n3dc9IxriHo2DKIFEqwIDAQAB\
\n-----END PUBLIC KEY-----";
	prikey = "-----BEGIN RSA PRIVATE KEY-----\
\nMIICXQIBAAKBgQDqDG9LqVmteJ3xsWv6u/lAt6cU3KTF4BDNZsSs+zmARQnBnHka\
\nV4nJgRl9IK8b8tCMK6xbFrWa6a9RUENL8vWbclo4vuqG1/qLtZgo+eFzXbT3tg/X\
\nLUrLsdKhU5+w1YJWdw4TQUMbsR0z1F0yOZDwAvRC3dc9IxriHo2DKIFEqwIDAQAB\
\nAoGACggYaRzMHDRUSLy7DRcresukXK+MXHLbJYKnIWbvMwFChsrnIerom/ttlUBm\
\nYQNKTwe8LndNt2MWwZx4FfRG9Jq5KUJJz16Yk+i1JTffFjOmALijyqHdLbc7SZ6p\
\nl82ChNdD8X7k305qULu86itrMMSQ3L6s3IBHoQGypQpf5vECQQD3I1OIzP3rPIo9\
\nzwOXU8LT/vADydW0ttdReesUR2wT2uMDosl5mxh8X36u/Oe72MjW0a/sfLZr5oRa\
\n1ectZMEDAkEA8nDzS+P35BbRI9hIA9X0F/SD8mLe9kVvGUKrriV2sH3FIMdbYkep\
\n9UEFP9ZpQ7lpc0Eq0SCjl8A7vbvqoRMZOQJBAMNDuSu8c9+aTMu7NeYp+yTPKEqF\
\n/YE0efnZL4EtUVp6tqVXyIJ5paYXOZv/HQWRqlX5BVv/yY6FawvuOCLomYsCQQDc\
\n2iH4TzqBsHticOLhg6TxsZAFXSYJKCVV2JM2d/BQRLIv8wt/UxMzVMDob3TC+gNi\
\nt8m+akI8uiRx6d6KTzCZAkBVZQV47puvjcLD75yUhQN5cUO7iqdeFQTFMYcv72DM\
\naIzBAtlfQDwItQM7Ylkquj+Ns2MbYotX5RxWlLmKE15u\
\n-----END RSA PRIVATE KEY-----";
}

dest_user::~dest_user() {
	_ClearUserList();
	BOOST_LOG_SEV(g_logger::get(), debug) << "[UserList] Stop.";
}

int dest_user::ImportUserList() {
	using std::ios;
	using namespace rapidjson;

	_ClearUserList();

	std::ifstream infile;
	infile.open(_cfgfile, std::ios::in | std::ios::binary);
	if (!infile.is_open()) {
		BOOST_LOG_SEV(g_logger::get(), info) << "[UserList] File does not exist. ";
		return 0;
	}
	infile.seekg(0, ios::end);
	auto inlen = infile.tellg();
	infile.seekg(0, ios::beg);
	char *buff = new char[int(inlen) + 1];
	infile.read(buff, inlen);
	buff[int(inlen)] = 0;
	infile.close();
	rapidjson::Document data;
	data.Parse(buff);
	delete[] buff;
	if (data.GetParseError() != ParseErrorCode::kParseErrorNone) {
		ParseErrorCode ret = data.GetParseError();
		BOOST_LOG_SEV(g_logger::get(), info) << "[UserList] File error: " << ret;
		return 0;
	}
	try {
		if (!data.IsArray()) {
			throw "Data is not array";
		}
		_user_list.reserve(data.Size());
		for (unsigned i = 0; i < data.Size(); i++) {
			std::shared_ptr<CBilibiliUserInfo> new_user(new CBilibiliUserInfo);
			new_user->ReadFileAccount(prikey, data[i], i + 1);
			// 无异常则读取成功
			_usercount++;
			_user_list.push_back(new_user);
			BOOST_LOG_SEV(g_logger::get(), info) << "[UserList] Import user " << i + 1 << " success.";
		}
	}
	catch (const char* msg) {
		BOOST_LOG_SEV(g_logger::get(), info) << "[UserList] Import error: " << msg;
		return 0;
	}

	return 0;
}

int dest_user::ExportUserList() {
	using namespace rapidjson;

	if (_usercount == 0) {
		BOOST_LOG_SEV(g_logger::get(), info) << "[UserList] No user.";
		return 0;
	}

	try {
		Document data;
		data.SetArray();
		for (auto itor = _user_list.begin(); itor != _user_list.end(); itor++) {
			(*itor)->WriteFileAccount(pubkey, data);
		}
		StringBuffer buffer;
		Writer<StringBuffer> writer(buffer);
		data.Accept(writer);
		std::ofstream outfile(_cfgfile);
		outfile << buffer.GetString();
	}
	catch (const char* msg) {
		BOOST_LOG_SEV(g_logger::get(), info) << "[UserList] Export error: " << msg;
		return 0;
	}

	BOOST_LOG_SEV(g_logger::get(), info) << "[UserList] Export end.";

	return 0;
}

int dest_user::ShowUserList() {
	printf("[UserList] Count: %d \n", _usercount);
	std::list<CBilibiliUserInfo*>::iterator itor;
	for (auto itor = _user_list.begin(); itor != _user_list.end(); itor++) {
		printf("%d %s \n", (*itor)->GetFileSN(), (*itor)->GetUsername().c_str());
	}
	return 0;
}

int  dest_user::AddUser(std::string username, std::string password) {
	if (_IsExistUser(username)) {
		BOOST_LOG_SEV(g_logger::get(), info) << "[UserList] " << username << " is already in the list.";
		return -1;
	}
	std::shared_ptr<CBilibiliUserInfo> new_user(new CBilibiliUserInfo);
	LOGINRET lret = new_user->Login(_usercount + 1, username, password);
	if (lret == LOGINRET::NOFAULT) {
		_usercount++;
		_user_list.push_back(new_user);
		BOOST_LOG_SEV(g_logger::get(), info) << "[UserList] " << username << " logged in successfully.";
		return 0;
	}
	if (lret == LOGINRET::NOTVALID) {
		BOOST_LOG_SEV(g_logger::get(), info) << "[UserList] " << username << " is not valid.";
	}
	else {
		BOOST_LOG_SEV(g_logger::get(), info) << "[UserList] " << username << " logged in failed.";
	}

	return -1;
}

int  dest_user::DeleteUser(std::string username) {
	for (auto itor = _user_list.begin(); itor != _user_list.end(); itor++) {
		if ((*itor)->GetUsername() != username) {
			continue;
		}
		// 删除该用户
		itor = _user_list.erase(itor);
		BOOST_LOG_SEV(g_logger::get(), info) << "[UserList] " << username << " deleted.";
		return 0;
	}
	BOOST_LOG_SEV(g_logger::get(), info) << "[UserList] " << username << " does not exist.";
	return -1;
}

int dest_user::ReloginAll() {
	for (auto itor = _user_list.begin(); itor != _user_list.end(); itor++) {
		switch ((*itor)->Relogin()) {
		case LOGINRET::NOFAULT:
			printf("[UserList] Account %s Logged in. \n", (*itor)->GetUsername().c_str());
			break;
		case LOGINRET::NOTVALID:
			printf("[UserList] Account %s Not valid. \n", (*itor)->GetUsername().c_str());
			break;
		default:
			printf("[UserList] Account %s Login failed. \n", (*itor)->GetUsername().c_str());
			break;
		}
	}
	return 0;
}

int dest_user::CheckUserStatusALL() {
	for (auto itor = _user_list.begin(); itor != _user_list.end(); itor++) {
		switch ((*itor)->CheckLogin()) {
		case LOGINRET::NOFAULT:
			printf("[UserList] Account %s Logged in. \n", (*itor)->GetUsername().c_str());
			break;
		case LOGINRET::NOTVALID:
			printf("[UserList] Account %s Not valid. \n", (*itor)->GetUsername().c_str());
			break;
		default:
			printf("[UserList] Account %s Logged out. \n", (*itor)->GetUsername().c_str());
			break;
		}
	}
	return 0;
}

int dest_user::GetUserInfoALL() {
	for (auto itor = _user_list.begin(); itor != _user_list.end(); itor++) {
		if (!(*itor)->getLoginStatus()) {
			continue;
		}
		(*itor)->FreshUserInfo();
	}
	return 0;
}

void dest_user::_ClearUserList() {
	_user_list.clear();
}

bool dest_user::_IsExistUser(std::string username) {
	for (auto itor = _user_list.begin(); itor != _user_list.end(); itor++) {
		if ((*itor)->GetUsername() == username) {
			return true;
		}
	}
	return false;
}



int dest_user::SetNotifyThread(unsigned long id) {
	if (id >= 0)
		_parentthread = id;
	return 0;
}

int dest_user::WaitActThreadStop() {
	while (_threadcount)
		Sleep(100);
	return 0;
}

int dest_user::HeartExp(int firsttime) {
	if (_user_list.empty()) {
		return 0;
	}

	if (firsttime) {
		//初次启动
		_heartcount = 0;
	}
	else {
		//暂定4小时一个周期
		_heartcount++;
		if (_heartcount >= 240) {
			_heartcount = 0;
		}
	}

	for (auto itor = _user_list.begin(); itor != _user_list.end(); itor++) {
		if (!(*itor)->getLoginStatus())
			continue;
		if (_heartcount == 0) {
			(*itor)->ActStartHeart();
		}
		else {
			(*itor)->ActHeart();
		}
	}
	return 0;
}

int dest_user::JoinLotteryALL(std::shared_ptr<BILI_LOTTERYDATA> data) {
	BOOST_LOG_SEV(g_logger::get(), info) << "[UserList] Gift Room: " << data->rrid << " ID: " << data->loid;
	// 当前没有用户则不领取
	if (!_usercount)
		return 0;

	PTHARED_DATAEX pdata = new THARED_DATAEX;
	pdata->rrid = data->rrid;
	pdata->loid = data->loid;
	// 配置节奏ID并创建领取线程
	_threadcount++;
	auto task = std::thread(std::bind(
		&dest_user::Thread_ActLottery,
		this,
		pdata)
	);
	task.detach();

	return 0;
}

int dest_user::JoinGuardALL(std::shared_ptr<BILI_LOTTERYDATA> data) {
	BOOST_LOG_SEV(g_logger::get(), info) << "[UserList] Guard Room: " << data->rrid << " ID: " << data->loid;
	// 当前没有用户则不领取
	if (!_usercount)
		return 0;

	PTHARED_DATAEX pdata = new THARED_DATAEX;
	pdata->rrid = data->rrid;
	pdata->loid = data->loid;
	pdata->str = data->type;
	// 配置节奏ID并创建领取线程
	_threadcount++;
	auto task = std::thread(std::bind(
		&dest_user::Thread_ActGuard,
		this,
		pdata)
	);
	task.detach();

	return 0;
}

int dest_user::JoinSpecialGiftALL(std::shared_ptr<BILI_LOTTERYDATA> data) {
	BOOST_LOG_SEV(g_logger::get(), info) << "[UserList] SpecialGift Room: " << data->rrid;
	// 当前没有用户则不领取
	if (!_usercount)
		return 0;

	PTHARED_DATAEX pdata = new THARED_DATAEX;
	pdata->rrid = data->rrid;
	pdata->loid = data->loid;
	// 配置节奏ID并创建领取线程
	_threadcount++;
	auto task = std::thread(std::bind(
		&dest_user::Thread_ActStorm,
		this,
		pdata)
	);
	task.detach();

	return 0;
}

void dest_user::Thread_ActLottery(PTHARED_DATAEX pdata) {
	// 等待领取
	BOOST_LOG_SEV(g_logger::get(), trace) << "[UserList] Thread join gift: " << pdata->loid;
	Sleep(_GetRand(5000, 5000));
	// 领取为防止冲突 同一时间只能有一个用户在领取
	// 在同一抽奖的两次抽奖之间增加间隔
	for (auto itor = _user_list.begin(); itor != _user_list.end(); itor++) {
		if (!(*itor)->getLoginStatus())
			continue;
		Sleep(_GetRand(1000, 1500));
		{
			boost::unique_lock<boost::shared_mutex> m(rwmutex_);
			(*itor)->ActLottery(pdata->rrid, pdata->loid);
		}
	}
	delete pdata;
	// 退出线程
	_threadcount--;
}

void dest_user::Thread_ActGuard(PTHARED_DATAEX pdata) {
	// 等待领取
	BOOST_LOG_SEV(g_logger::get(), trace) << "[UserList] Thread join guard: " << pdata->loid;
	Sleep(_GetRand(5000, 5000));
	for (auto itor = _user_list.begin(); itor != _user_list.end(); itor++) {
		if (!(*itor)->getLoginStatus())
			continue;
		Sleep(_GetRand(1000, 1500));
		{
			boost::unique_lock<boost::shared_mutex> m(rwmutex_);
			(*itor)->ActGuard(pdata->str, pdata->rrid, pdata->loid);
		}
	}
	delete pdata;
	// 退出线程
	_threadcount--;
}

void dest_user::Thread_ActStorm(PTHARED_DATAEX pdata) {
	// 等待领取
	BOOST_LOG_SEV(g_logger::get(), trace) << "[UserList] Thread join guard: " << pdata->loid;
	// Sleep(_GetRand(8000, 4000));
	// 领取为防止冲突 同一时间只能有一个用户在领取
	// 在同一抽奖的两次抽奖之间增加间隔
	for (auto itor = _user_list.begin(); itor != _user_list.end(); itor++) {
		if (!(*itor)->getLoginStatus())
			continue;
		Sleep(_GetRand(1000, 1500));
		{
			boost::unique_lock<boost::shared_mutex> m(rwmutex_);
			(*itor)->ActStorm(pdata->rrid, pdata->loid);
		}
	}
	delete pdata;
	// 退出线程
	_threadcount--;
}

int dest_user::_GetRand(int start, int len)
{
	srand((unsigned int)time(0));
	return rand() % len + start;
}
