#include "dest_user.h"
#include <fstream>
#include <functional>
#include <iostream>
#include <thread>
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "utility/platform.h"
#include "utility/strconvert.h"
#include "logger/log.h"
#include "api_bl.h" 

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
			std::shared_ptr<user_info> new_user(new user_info);
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
	std::list<user_info*>::iterator itor;
	for (auto itor = _user_list.begin(); itor != _user_list.end(); itor++) {
		printf("%d %s \n", (*itor)->fileid, (*itor)->account.c_str());
	}
	return 0;
}

int  dest_user::AddUser(std::string username, std::string password) {
	if (_IsExistUser(username)) {
		BOOST_LOG_SEV(g_logger::get(), info) << "[UserList] " << username << " is already in the list.";
		return -1;
	}
	std::shared_ptr<user_info> new_user(new user_info);
	LOGINRET lret = _ActLogin(new_user, _usercount + 1, username, password);
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
		if ((*itor)->account != username) {
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
		switch (_ActRelogin(*itor)) {
		case LOGINRET::NOFAULT:
			printf("[UserList] Account %s Logged in. \n", (*itor)->account.c_str());
			break;
		case LOGINRET::NOTVALID:
			printf("[UserList] Account %s Not valid. \n", (*itor)->account.c_str());
			break;
		default:
			printf("[UserList] Account %s Login failed. \n", (*itor)->account.c_str());
			break;
		}
	}
	return 0;
}

int dest_user::CheckUserStatusALL() {
	for (auto itor = _user_list.begin(); itor != _user_list.end(); itor++) {
		switch (_ActCheckLogin(*itor)) {
		case LOGINRET::NOFAULT:
			printf("[UserList] Account %s Logged in. \n", (*itor)->account.c_str());
			break;
		case LOGINRET::NOTVALID:
			printf("[UserList] Account %s Not valid. \n", (*itor)->account.c_str());
			break;
		default:
			printf("[UserList] Account %s Logged out. \n", (*itor)->account.c_str());
			break;
		}
	}
	return 0;
}

int dest_user::GetUserInfoALL() {
	for (auto itor = _user_list.begin(); itor != _user_list.end(); itor++) {
		if (!(*itor)->islogin) {
			continue;
		}
		_ActGetUserInfo(*itor);
	}
	return 0;
}

void dest_user::_ClearUserList() {
	_user_list.clear();
}

bool dest_user::_IsExistUser(std::string username) {
	for (auto itor = _user_list.begin(); itor != _user_list.end(); itor++) {
		if ((*itor)->account == username) {
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
		if (!(*itor)->islogin)
			continue;
		if (_heartcount == 0) {
			_ActHeartFirst(*itor);
		}
		else {
			_ActHeartContinue(*itor);
		}
	}
	return 0;
}

int dest_user::JoinLotteryALL(std::shared_ptr<BILI_LOTTERYDATA> data) {
	BOOST_LOG_SEV(g_logger::get(), info) << "[UserList] Gift Room: " << data->rrid << " ID: " << data->loid;
	// 当前没有用户则不领取
	if (!_usercount)
		return 0;

	_threadcount++;
	auto task = std::thread(std::bind(
		&dest_user::Thread_ActLottery,
		this,
		data)
	);
	task.detach();

	return 0;
}

int dest_user::JoinGuardALL(std::shared_ptr<BILI_LOTTERYDATA> data) {
	BOOST_LOG_SEV(g_logger::get(), info) << "[UserList] Guard Room: " << data->rrid << " ID: " << data->loid;
	// 当前没有用户则不领取
	if (!_usercount)
		return 0;

	_threadcount++;
	auto task = std::thread(std::bind(
		&dest_user::Thread_ActGuard,
		this,
		data)
	);
	task.detach();

	return 0;
}

int dest_user::JoinSpecialGiftALL(std::shared_ptr<BILI_LOTTERYDATA> data) {
	BOOST_LOG_SEV(g_logger::get(), info) << "[UserList] SpecialGift Room: " << data->rrid;
	// 当前没有用户则不领取
	if (!_usercount)
		return 0;

	_threadcount++;
	auto task = std::thread(std::bind(
		&dest_user::Thread_ActStorm,
		this,
		data)
	);
	task.detach();

	return 0;
}

int dest_user::JoinPKLotteryALL(std::shared_ptr<BILI_LOTTERYDATA> data) {
	BOOST_LOG_SEV(g_logger::get(), info) << "[UserList] PK Lottery Room: " << data->rrid;
	// 当前没有用户则不领取
	if (!_usercount)
		return 0;
	
	_threadcount++;
	auto task = std::thread(std::bind(
		&dest_user::Thread_ActPK,
		this,
		data)
	);
	task.detach();
	
	return 0;
}

void dest_user::ShowDailyTask(unsigned max_deep) {
	using namespace std;
	// 从后往前领取
	for (auto itor = _user_list.rbegin(); itor != _user_list.rend(); itor++) {
		cout << "User " << (*itor)->account << endl;
		if ((*itor)->ten_self_level <= max_deep) {
			_ActShowTask(*itor);
		}
	}
}

void dest_user::ShowCP() {
	using namespace std;
	std::string token = "";
	for (auto itor = _user_list.begin(); itor != _user_list.end(); itor++) {
		cout << "User " << (*itor)->account << endl;
		// 获取CP状态 获取见证者列表
		if (apibl::APIShowCPInfo(*itor) != BILIRET::NOFAULT) {
			continue;
		}
		if ((*itor)->ten_cp_id != 0) {
			// 有CP跳过
			continue;
		}
		if (token != "") {
			// 与上一个用户组CP
			if (apibl::APIShowCPAgree(*itor, token) != BILIRET::NOFAULT) {
				continue;
			}
			token = "";
			continue;
		}
		token = (*itor)->ten_cp_token;
	}
}

void dest_user::ShowList(const char * filename, unsigned max_deep) {
	using namespace std;
	std::ofstream outfile(filename);
	if (!outfile.is_open()) {
		return;
	}
	for (auto itor = _user_list.begin(); itor != _user_list.end(); itor++) {
		if ((*itor)->ten_self_level > max_deep) {
			continue;
		}
		for (unsigned i = 1; i <= 5; i++) {
			BILIRET ret = apibl::APIShowTaskList(*itor, i);
			if (ret == BILIRET::TEN_TASK_NOTOPEN) {
				break;
			}
			if (ret != BILIRET::NOFAULT) {
				continue;
			}
		}
		outfile << '#' << (*itor)->account << endl;
		for (auto it = (*itor)->ten_task_list.begin(); it != (*itor)->ten_task_list.end(); it++) {
			outfile << *it << endl;
		}
	}
	outfile << '#' << endl;
}

void dest_user::ShowLike(const char*filename, bool &haschange) {
	using namespace std;
	haschange = false;
	std::ifstream infile(filename);
	if (!infile.is_open()) {
		return;
	}
	std::deque<std::string> tasklist;
	std::string taskid;
	while (infile >> taskid) {
		if (taskid[0] == '#') {
			// 每个账号的任务分开来做
			_ActShowLike(tasklist, haschange);
			continue;
		}
		tasklist.push_back(taskid);
		printf("Add taskid: %s\n", taskid.c_str());
		if (tasklist.size() > 10) {
			// 积累了10个任务就点一次
			_ActShowLike(tasklist, haschange);
		}
	}
}

void dest_user::ShowJoinWitness(std::shared_ptr<user_info>& user) {
	using namespace std;
	for (auto itor = _user_list.begin(); itor != _user_list.end(); itor++) {
		if ((*itor)->ten_team_hasjoin) {
			// 跳过已加入的账号
			continue;
		}
		cout << "User " << (*itor)->account << endl;
		BILIRET ret = apibl::APIShowWitJoin(*itor, user->ten_team_id);
		if (ret == BILIRET::TEN_TEAM_FULL) {
			return;
		}
		if (ret == BILIRET::NOFAULT) {
			user->ten_team_list.push_back((*itor)->uid);
		}
	}
}

void dest_user::ShowJoinWitness(long long id) {
	using namespace std;
	for (auto itor = _user_list.begin(); itor != _user_list.end(); itor++) {
		if ((*itor)->ten_team_hasjoin) {
			// 跳过已加入的账号
			continue;
		}
		cout << "User " << (*itor)->account << endl;
		BILIRET ret = apibl::APIShowWitJoin(*itor, id);
		if (ret == BILIRET::TEN_TEAM_FULL) {
			return;
		}
	}
}

void dest_user::ShowAuto() {
	// 生成下线列表
	unsigned active_like_mem = 0;
	// 用户存储映射
	std::map<unsigned, unsigned> memmap;
	for (size_t i = 0; i < _user_list.size(); i++) {
		memmap[_user_list[i]->uid] = i;
		// 重置账户级别
		_user_list[i]->ten_self_level = TEN_DEFAULT_LEVEL;
		if (!_user_list[i]->ten_task_full) {
			active_like_mem++;
		}
	}
	// 下线列表
	std::vector<std::vector<unsigned> > member(3, std::vector<unsigned>());
	// 将前2个账号当作主账号 等级为1级
	_user_list[0]->ten_self_level = 1;
	member[0].push_back(_user_list[0]->uid);
	_user_list[1]->ten_self_level = 1;
	member[0].push_back(_user_list[1]->uid);
	// 将需要做任务涨积分的账号添加到树中
	// 仅前两代对主账号起作用
	for (size_t deep = 0; deep < 2; deep++) {
		for (size_t i = 0; i < member[deep].size(); i++) {
			unsigned uid = member[deep][i];
			unsigned idx1 = memmap[uid];
			if (_user_list[idx1]->ten_team_list.size() >= 10 && deep == 1) {
				// 子代见证者满员后 孙子代就不需要做任务帮子代升级了
				continue;
			}
			for (auto it = _user_list[idx1]->ten_team_list.begin();
				it != _user_list[idx1]->ten_team_list.end(); 
				it++) {
				if (!memmap.count(*it)) {
					// 见证者非自己的账号
					continue;
				}
				unsigned idx2 = memmap[*it];
				if (_user_list[idx2]->ten_self_level != TEN_DEFAULT_LEVEL) {
					// 见证者为该账号的上级 跳过
					continue;
				}
				// 更新级别
				_user_list[idx2]->ten_self_level = deep + 2;
				if (member.size() <= deep + 1) {
					member.push_back(std::vector<unsigned>());
				}
				member[deep + 1].push_back(*it);
			}
		}
	}
	// 有可用点赞账号
	if (active_like_mem) {
		// 下线等级2以内的用户领取任务
		ShowList("tmp", 2);
		// 点赞
		bool haschange = false;
		ShowLike("tmp", haschange);
		// 下线等级2以内的用户领取点赞奖励
		if (haschange) {
			ShowList("tmp", 2);
		}
	}
	// 所有下属做日常 更新见证者坑位
	// 更新后不会刷新缓存数据
	ShowDailyTask(TEN_DEFAULT_LEVEL - 1);
	// 添加见证者 最高下线为2级
	size_t max_deep = member.size();
	if (max_deep > 2) {
		max_deep = 2;
	}
	for (size_t deep = 0; deep < max_deep; deep++) {
		for (size_t i = 0; i < member[deep].size(); i++) {
			unsigned uid = member[deep][i];
			unsigned idx1 = memmap[uid];
			if (!_user_list[idx1]->ten_team_empty_num) {
				continue;
			}
			// 有剩余坑位 添加见证者
			ShowJoinWitness(_user_list[idx1]);
		}
	}
	// 导出账号信息
	ExportUserList();
}

LOGINRET dest_user::_ActLogin(std::shared_ptr<user_info>& user, int index, std::string username, std::string password) {
	BILIRET bret;
	user->account = username;
	user->password = password;

	// 将账号转码
	username = toollib::UrlEncode(user->account);

	// 移动端登录
	password = user->password;
	bret = apibl::APIAndGetKey(user, password);
	if (bret != BILIRET::NOFAULT) {
		return LOGINRET::NOTLOGIN;
	}
	bret = apibl::APIAndv2Login(user, username, password, "");
	if (bret == BILIRET::LOGIN_NEEDVERIFY) {
		// 获取验证码
		bret = apibl::APIWebGETLoginCaptcha(user);
		if (bret != BILIRET::NOFAULT) {
			return LOGINRET::NOTLOGIN;
		}
		std::string tmp_chcode;
		printf("Enter the pic validate code: ");
		std::cin >> tmp_chcode;
		// 移动端使用验证码登录
		password = user->password;
		bret = apibl::APIAndGetKey(user, password);
		if (bret != BILIRET::NOFAULT) {
			return LOGINRET::NOTLOGIN;
		}
		bret = apibl::APIAndv2Login(user, username, password, tmp_chcode);
	}
	if (bret != BILIRET::NOFAULT) {
		return LOGINRET::NOTLOGIN;
	}

	// 登录成功则获取必要的临时id
	bret = apibl::APIWebGetCaptchaKey(user);
	if (bret != BILIRET::NOFAULT) {
		user->islogin = false;
		return LOGINRET::NOTLOGIN;
	}
	// 生成访问ID
	user->GetVisitID();
	if (!user->GetToken()) {
		user->islogin = false;
		return LOGINRET::NOTLOGIN;
	}
	user->fileid = index;
	user->islogin = true;

	return LOGINRET::NOFAULT;
}

LOGINRET dest_user::_ActRelogin(std::shared_ptr<user_info>& user) {
	int ret = 0;
	LOGINRET lret;
	ret = user->GetExpiredTime();
	long long rtime;
	rtime = ret - toollib::GetTimeStamp();
	// 有效期小于一周或Token不存在则重新登录
	if ((rtime < 604800) || (user->tokena.empty())) {
		lret = _ActLogin(user, user->fileid, user->account, user->password);
		return lret;
	}
	return LOGINRET::NOFAULT;
}

LOGINRET dest_user::_ActCheckLogin(std::shared_ptr<user_info>& user) {
	BILIRET bret;
	bret = apibl::APIWebGetUserInfo(user);
	if (bret != BILIRET::NOFAULT) {
		user->islogin = false;
		return LOGINRET::NOTLOGIN;
	}
	bret = apibl::APIWebGetCaptchaKey(user);
	if (bret != BILIRET::NOFAULT) {
		user->islogin = false;
		return LOGINRET::NOTLOGIN;
	}
	// 生成访问ID
	user->GetVisitID();
	if (!user->GetToken()) {
		user->islogin = false;
		return LOGINRET::NOTLOGIN;
	}
	user->islogin = true;
	return LOGINRET::NOFAULT;
}

int dest_user::_ActGetUserInfo(const std::shared_ptr<user_info>& user) {
	BOOST_LOG_SEV(g_logger::get(), info) << "[User" << user->fileid << "] ";
	apibl::APIWebv2GiftBag(user);
	apibl::APIWebv1CapsuleCheck(user);
	return 0;
}

// 开启经验心跳
void dest_user::_ActHeartFirst(std::shared_ptr<user_info> &user) {
	// 常规心跳
	apibl::APIWebv1HeartBeat(user);
	// 心跳计时标签
	user->heart_deadline = 5;
	// 双端观看奖励
	apibl::APIWebTaskInfo(user);
	// 银瓜子领取信息获取
	apibl::APIAndv1SilverTask(user);
	// 签到
	apibl::APIWebSign(user);
	// 每日礼物
	apibl::APIWebv2GiftDaily(user);
	// 兑换硬币
	if (user->conf_coin == 1) {
		apibl::APIWebv1Silver2Coin(user);
	}
	// 登录硬币
	apibl::APIWebGetCoin(user);
}

// 经验心跳
void dest_user::_ActHeartContinue(std::shared_ptr<user_info> &user) {
	user->heart_deadline--;
	if (user->heart_deadline == 0) {
		user->heart_deadline = 5;
		apibl::APIWebv1HeartBeat(user);
		apibl::APIAndv1Heart(user);
		apibl::APIWebOnlineHeart(user);
	}
	if (user->silver_deadline != -1) {
		user->silver_deadline--;
		if (user->silver_deadline == 0) {
			apibl::APIAndv1SilverAward(user);
			apibl::APIAndv1SilverTask(user);
		}
	}
}

int dest_user::_ActLottery(std::shared_ptr<user_info>& user, std::shared_ptr<BILI_LOTTERYDATA> data) {
	BILIRET bret;
	int count;
	if (user->conf_lottery == 1) {
		// 产生访问记录
		apibl::APIWebv1RoomEntry(user, data->rrid);
		// 网页端最多尝试三次
		count = 2;
		bret = apibl::APIWebv3SmallTV(user, data);
		while ((bret != BILIRET::NOFAULT) && count) {
			Sleep(1000);
			bret = apibl::APIWebv3SmallTV(user, data);
			count--;
		}
	}
	// 手机端需要 time_wait 倒计时结束才能抽奖
	if (user->conf_lottery == 2) {
		// 产生访问记录
		apibl::APIAndv1RoomEntry(user, data->rrid);
		// 调用客户端API领取
		apibl::APIAndv4SmallTV(user, data);
		return 0;
	}
	return 0;
}

int dest_user::_ActGuard(std::shared_ptr<user_info>& user, std::shared_ptr<BILI_LOTTERYDATA> data)  {
	if (user->conf_guard == 1) {
		// 产生访问记录
		apibl::APIWebv1RoomEntry(user, data->rrid);
		// 网页端API
		apibl::APIWebv2LotteryJoin(user, data);
		return 0;
	}
	if (user->conf_guard == 2) {
		// 产生访问记录
		apibl::APIAndv1RoomEntry(user, data->rrid);
		// 调用客户端API领取
		apibl::APIAndv2LotteryJoin(user, data);
		return 0;
	}
	return 0;
}

int dest_user::_ActStorm(std::shared_ptr<user_info> &user, std::shared_ptr<BILI_LOTTERYDATA> data) {
	// 风暴只领取一次 不管成功与否
	if (user->conf_storm == 1) {
		// 产生访问记录
		apibl::APIWebv1RoomEntry(user, data->rrid);
		// 网页端API
		apibl::APIWebv1StormJoin(user, data, "", "");
		return 0;
	}
	if (user->conf_storm == 2) {
		// 产生访问记录
		apibl::APIAndv1RoomEntry(user, data->rrid);
		// 调用客户端API领取
		apibl::APIAndv1StormJoin(user, data);
		return 0;
	}
	return 0;
}

int dest_user::_ActPK(std::shared_ptr<user_info> &user, std::shared_ptr<BILI_LOTTERYDATA> data) {
	if (user->conf_pk == 1) {
		// 产生访问记录
		apibl::APIWebv1RoomEntry(user, data->rrid);
		// 网页端API
		apibl::APIWebv1PKJOIN(user, data);
		return 0;
	}
	if (user->conf_pk == 2) {
		// 产生访问记录
		apibl::APIAndv1RoomEntry(user, data->rrid);
		// 调用客户端API领取
		apibl::APIAndv1PKJOIN(user, data);
		return 0;
	}
	return 0;
}

int dest_user::_ActShowTask(std::shared_ptr<user_info>& user) {
	// 签到
	if (apibl::APIShowSignStatus(user) != BILIRET::NOFAULT) {
		return -1;
	}
	if (user->ten_sign_status == false) {
		if (apibl::APIShowSignDo(user) != BILIRET::NOFAULT) {
			return -1;
		}
	}
	// 查询任务状态
	if (apibl::APIShowShareStatus(user) != BILIRET::NOFAULT) {
		return -1;
	}
	// 找彩蛋
	if (user->ten_egg_status != 1) {
		if (apibl::APIShowCallback(
			user,
			"act626-egg",
			6261002,
			"egg_one"
		) != BILIRET::NOFAULT) {
			return -1;
		}
		if (apibl::APIShowCallback(
			user,
			"act626-egg",
			6261002,
			"egg_two"
		) != BILIRET::NOFAULT) {
			return -1;
		}
		if (apibl::APIShowCallback(
			user,
			"act626-egg",
			6261002,
			"egg_three"
		) != BILIRET::NOFAULT) {
			return -1;
		}
		if (apibl::APIShowShareStatus(user) != BILIRET::NOFAULT) {
			return -1;
		}
		if (apibl::APIShowReward(
			user,
			user->ten_egg_assocId,
			user->ten_egg_taskid,
			user->ten_egg_type,
			apibl::TEN_REFERER_MAIN
		) != BILIRET::NOFAULT) {
			return -1;
		}
	}
	// 发动态
	if (user->ten_pub_status != 1) {
		if (apibl::APIShowCallback(
			user,
			"act626-pub",
			6261001,
			"publish"
		) != BILIRET::NOFAULT) {
			return -1;
		}
		if (apibl::APIShowShareStatus(user) != BILIRET::NOFAULT) {
			return -1;
		}
		if (apibl::APIShowReward(
			user,
			user->ten_pub_assocId,
			user->ten_pub_taskid,
			user->ten_pub_type,
			apibl::TEN_REFERER_MAIN
		) != BILIRET::NOFAULT) {
			return -1;
		}
	}
	// 领碎片加成 获取队伍ID以及剩余坑位
	if (apibl::APIShowWitDeital(user) != BILIRET::NOFAULT) {
		return -1;
	}

	return 0;
}

void dest_user::_ActShowLike(std::deque<std::string>& tasklist, bool &haschange) {
	using namespace std;
	if (tasklist.empty()) {
		return;
	}
	cout << "Task size: " << tasklist.size() << endl;
	for (auto itor = _user_list.begin(); itor != _user_list.end(); itor++) {
		while (1) {
			if (tasklist.empty()) {
				// 没任务了
				return;
			}
			if ((*itor)->ten_task_full) {
				// 这个账号3次满了
				break;
			}
			std::string id = tasklist.front();
			if ((*itor)->ten_task_list.count(atoi(id.c_str()))) {
				// 是自己的任务
				break;
			}
			tasklist.pop_front();
			BILIRET ret = apibl::APIShowLike(*itor, id);
			if (ret == BILIRET::TEN_TASK_FINISH) {
				// 点满了
				haschange = true;
				continue;
			}
			if (ret == BILIRET::TEN_LIKE_FAILED) {
				// 账号不能点了 或者任务是自己的
				tasklist.push_back(id);
				break;
			}
			// 将任务放到队尾
			tasklist.push_back(id);
		}
	}
}

void dest_user::Thread_ActLottery(std::shared_ptr<BILI_LOTTERYDATA> data) {
	// 等待领取
	BOOST_LOG_SEV(g_logger::get(), trace) << "[UserList] Thread join gift: " << data->loid;
	Sleep(_GetRand(5000, 5000));
	// 领取为防止冲突 同一时间只能有一个用户在领取
	// 在同一抽奖的两次抽奖之间增加间隔
	for (auto itor = _user_list.begin(); itor != _user_list.end(); itor++) {
		if (!(*itor)->islogin)
			continue;
		Sleep(_GetRand(1000, 1500));
		{
			boost::unique_lock<boost::shared_mutex> m(rwmutex_);
			_ActLottery(*itor, data);
		}
	}
	// 退出线程
	_threadcount--;
}

void dest_user::Thread_ActGuard(std::shared_ptr<BILI_LOTTERYDATA> data) {
	// 等待领取
	BOOST_LOG_SEV(g_logger::get(), trace) << "[UserList] Thread join guard: " << data->loid;
	Sleep(_GetRand(5000, 5000));
	for (auto itor = _user_list.begin(); itor != _user_list.end(); itor++) {
		if (!(*itor)->islogin)
			continue;
		Sleep(_GetRand(1000, 1500));
		{
			boost::unique_lock<boost::shared_mutex> m(rwmutex_);
			_ActGuard(*itor, data);
		}
	}
	// 退出线程
	_threadcount--;
}

void dest_user::Thread_ActStorm(std::shared_ptr<BILI_LOTTERYDATA> data) {
	// 等待领取
	BOOST_LOG_SEV(g_logger::get(), trace) << "[UserList] Thread join storm: " << data->loid;
	// Sleep(_GetRand(8000, 4000));
	// 领取为防止冲突 同一时间只能有一个用户在领取
	// 在同一抽奖的两次抽奖之间增加间隔
	for (auto itor = _user_list.begin(); itor != _user_list.end(); itor++) {
		if (!(*itor)->islogin)
			continue;
		Sleep(_GetRand(1000, 1500));
		{
			boost::unique_lock<boost::shared_mutex> m(rwmutex_);
			_ActStorm(*itor, data);
		}
	}
	// 退出线程
	_threadcount--;
}

void dest_user::Thread_ActPK(std::shared_ptr<BILI_LOTTERYDATA> data) {
	// 等待领取
	BOOST_LOG_SEV(g_logger::get(), trace) << "[UserList] Thread join pk: " << data->loid;
	Sleep(_GetRand(5000, 5000));
	// 领取为防止冲突 同一时间只能有一个用户在领取
	// 在同一抽奖的两次抽奖之间增加间隔
	for (auto itor = _user_list.begin(); itor != _user_list.end(); itor++) {
		if (!(*itor)->islogin)
			continue;
		Sleep(_GetRand(1000, 1500));
		{
			boost::unique_lock<boost::shared_mutex> m(rwmutex_);
			_ActPK(*itor, data);
		}
	}
	// 退出线程
	_threadcount--;
}

int dest_user::_GetRand(int start, int len)
{
	srand((unsigned int)time(0));
	return rand() % len + start;
}
