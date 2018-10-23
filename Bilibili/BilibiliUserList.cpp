#include "stdafx.h"
#include "BilibiliUserList.h"
#include "log.h"

CBilibiliUserList::CBilibiliUserList()
{
	_parentthread = 0;
	_msgthread = 0;
	memset(_isworking, 0, sizeof(_isworking));
	m_workmode = TOOL_EVENT::STOP;
	_threadcount = 0;
	// 初始化线程互斥量
	InitializeCriticalSection(&_csthread);

	GetCurrentDirectoryA(sizeof(_cfgfile), _cfgfile);
	strcat_s(_cfgfile, DEF_CONFIGGILE_NAME);
	_usercount = 0;

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

CBilibiliUserList::~CBilibiliUserList()
{
	ClearUserList();
	//删除客户端列表的互斥量
	DeleteCriticalSection(&_csthread);
	BOOST_LOG_SEV(g_logger::get(), debug) << "[UserList] Stop.";
}

int  CBilibiliUserList::AddUser(std::string username, std::string password) {
	if (m_workmode != TOOL_EVENT::STOP)
		return -1;
	if (SearchUser(username)) {
		BOOST_LOG_SEV(g_logger::get(), info) << "[UserList] " << username << " is already in the list.";
		return -1;
	}
	CBilibiliUserInfo *puser = new CBilibiliUserInfo;
	LOGINRET lret;
	lret = puser->Login(_usercount + 1, username, password);
	if (lret == LOGINRET::NOFAULT) {
		_usercount++;
		_userlist.push_back(puser);
		BOOST_LOG_SEV(g_logger::get(), info) << "[UserList] " << username << " logged in successfully.";
		return 0;
	}
	else {
		delete puser;
		if (lret == LOGINRET::NOTVALID) {
			BOOST_LOG_SEV(g_logger::get(), info) << "[UserList] " << username << " is not valid.";
		}
		else {
			BOOST_LOG_SEV(g_logger::get(), info) << "[UserList] " << username <<  " logged in failed.";
		}
	}
	return -1;
}

int  CBilibiliUserList::DeleteUser(std::string username)
{
	if (m_workmode != TOOL_EVENT::STOP)
		return -1;
	std::list<CBilibiliUserInfo*>::iterator itor;
	for (itor = _userlist.begin(); itor != _userlist.end(); itor++) {
		if ((*itor)->GetUsername() == username) {
			break;
		}
	}
	if (itor == _userlist.end()) {
		BOOST_LOG_SEV(g_logger::get(), info) << "[UserList] " << username << " does not exist.";
		return -1;
	}
	// 删除该用户
	int isn = (*itor)->GetFileSN();
	BOOST_LOG_SEV(g_logger::get(), info) << "[UserList] " << username << " deleted.";
	delete *itor;
	itor = _userlist.erase(itor);
	if (isn == _usercount) {
		_usercount--;
		return 0;
	}
	// 如果该用户不是编号最大的 需要修改编号
	for (itor = _userlist.begin(); itor != _userlist.end(); itor++) {
		if ((*itor)->GetFileSN() == _usercount) {
			(*itor)->SetFileSN(isn);
			break;
		}
	}
	_usercount--;
	return 0;
}

int CBilibiliUserList::ClearUserList()
{
	if (m_workmode != TOOL_EVENT::STOP)
		return -1;
	std::list<CBilibiliUserInfo*>::iterator itor;
	for (itor = _userlist.begin(); itor != _userlist.end();) {
		delete *itor;
		itor = _userlist.erase(itor);
	}
	_usercount = 0;
	return 0;
}

int CBilibiliUserList::ShowUserList()
{
	printf("[UserList] Count: %d \n", _usercount);
	std::list<CBilibiliUserInfo*>::iterator itor;
	for (itor = _userlist.begin(); itor != _userlist.end(); itor++) {
		printf("%d %s \n", (*itor)->GetFileSN(), (*itor)->GetUsername().c_str());
	}
	return 0;
}

int CBilibiliUserList::ImportUserList()
{
	if (m_workmode != TOOL_EVENT::STOP)
		return -1;
	ClearUserList();
	int ret, tmpi, ii;
	tmpi = ::GetPrivateProfileIntA("Global", "Accountnum", -1, _cfgfile);
	if (tmpi == -1)
		return -1;
	// 如果有数据
	CBilibiliUserInfo *puser;
	for (ii = 0; ii < tmpi; ii++) {
		puser = new CBilibiliUserInfo;
		ret = puser->ReadFileAccount(prikey, ii + 1, _cfgfile);
		if (!ret) {
			// 读取成功
			_usercount++;
			_userlist.push_back(puser);
			BOOST_LOG_SEV(g_logger::get(), info) << "[UserList] Import user " << ii + 1 << " success.";
		}
		else {
			delete puser;
			BOOST_LOG_SEV(g_logger::get(), warning) << "[UserList] Import user " << ii + 1 << " unsuccess.";
		}
	}
	// 导入后立即检测账户有效性
	// BOOST_LOG_SEV(g_logger::get(), info) << "[UserList] Start checking...";
	// this->CheckUserStatusALL();
	BOOST_LOG_SEV(g_logger::get(), info) << "[UserList] Import " << _usercount << " user(s).";

	return 0;
}

int CBilibiliUserList::ExportUserList()
{
	if (m_workmode != TOOL_EVENT::STOP)
		return -1;
	int ret;
	//如果有数据
	ret = ::WritePrivateProfileStringA("Global", "Accountnum", std::to_string(_usercount).c_str(), _cfgfile);
	std::list<CBilibiliUserInfo*>::iterator itor;
	for (itor = _userlist.begin(); itor != _userlist.end(); itor++) {
		ret = (*itor)->WriteFileAccount(pubkey, _cfgfile);
	}
	BOOST_LOG_SEV(g_logger::get(), info) << "[UserList] Export " << _usercount << " user(s).";

	return ret;
}

int CBilibiliUserList::ReloginAll()
{
	std::list<CBilibiliUserInfo*>::iterator itor;
	for (itor = _userlist.begin(); itor != _userlist.end(); itor++) {
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

int CBilibiliUserList::CheckUserStatusALL()
{
	std::list<CBilibiliUserInfo*>::iterator itor;
	for (itor = _userlist.begin(); itor != _userlist.end(); itor++) {
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

int CBilibiliUserList::GetUserInfoALL()
{
	std::list<CBilibiliUserInfo*>::iterator itor;
	for (itor = _userlist.begin(); itor != _userlist.end(); itor++) {
		if (!(*itor)->getLoginStatus())
			continue;

		(*itor)->FreshUserInfo();
	}
	return 0;
}



int CBilibiliUserList::SetNotifyThread(DWORD id)
{
	if (id >= 0)
		_parentthread = id;
	return 0;
}

int CBilibiliUserList::StartUserHeart()
{
	if (!_isworking[0])
	{
		m_workmode = TOOL_EVENT::ONLINE;
		_lphandle[0] = CreateThread(NULL, 0, Thread_UserListMSG, this, 0, &_msgthread);
		if (_lphandle[0] == INVALID_HANDLE_VALUE) {
			m_workmode = TOOL_EVENT::STOP;
			return -1;
		}
		//确认线程已经启动
		while (!_isworking[0])
			Sleep(100);
		//进行初次心跳
		_HeartExp(1);
	}
	return 0;
}

int CBilibiliUserList::StopUserHeart()
{
	//退出主线程
	if (_isworking[0])
	{
		CloseHandle(_lphandle[0]);
		PostThreadMessage(_msgthread, MSG_BILI_THREADCLOSE, WPARAM(0), LPARAM(0));
		while (_isworking[0])
			Sleep(100);
		_msgthread = 0;
		m_workmode = TOOL_EVENT::STOP;
	}
	return 0;
}

int CBilibiliUserList::WaitActThreadStop()
{
	while (_threadcount)
		Sleep(100);
	return 0;
}

int CBilibiliUserList::JoinTVALL(BILI_LOTTERYDATA *data)
{
	BOOST_LOG_SEV(g_logger::get(), info) << "[UserList] Gift Room: " << data->rrid << " ID: " << data->loid;
	// 当前没有用户则不领取
	if (!_usercount)
		return 0;

	HANDLE lphandle;
	PTHARED_DATAEX pdata = new THARED_DATAEX;
	pdata->ptr = this;
	pdata->id1 = data->rrid;
	pdata->id2 = data->loid;
	// 配置节奏ID并创建领取线程
	EnterCriticalSection(&_csthread);
	_threadcount++;
	LeaveCriticalSection(&_csthread);
	lphandle = CreateThread(NULL, 0, Thread_ActTV, pdata, 0, 0);
	CloseHandle(lphandle);

	return 0;
}

int CBilibiliUserList::JoinGuardALL(BILI_LOTTERYDATA &data)
{
	BOOST_LOG_SEV(g_logger::get(), info) << "[UserList] Guard Room: " << data.rrid << " ID: " << data.loid;
	// 当前没有用户则不领取
	if (!_usercount)
		return 0;

	HANDLE lphandle;
	PTHARED_DATAEX pdata = new THARED_DATAEX;
	pdata->ptr = this;
	pdata->id1 = data.rrid;
	pdata->id2 = data.loid;
	pdata->str = data.type;
	// 配置节奏ID并创建领取线程
	EnterCriticalSection(&_csthread);
	_threadcount++;
	LeaveCriticalSection(&_csthread);
	lphandle = CreateThread(NULL, 0, Thread_ActGuard, pdata, 0, 0);
	CloseHandle(lphandle);

	return 0;
}

int CBilibiliUserList::JoinSpecialGiftALL(int roomID, long long cid)
{
	BOOST_LOG_SEV(g_logger::get(), info) << "[UserList] SpecialGift Room: " << roomID;
	// 当前没有用户则不领取
	if (!_usercount)
		return 0;

	HANDLE lphandle;
	PTHARED_DATAEX pdata = new THARED_DATAEX;
	pdata->ptr = this;
	pdata->id1 = roomID;
	pdata->id3 = cid;
	// 配置节奏ID并创建领取线程
	EnterCriticalSection(&_csthread);
	_threadcount++;
	LeaveCriticalSection(&_csthread);
	lphandle = CreateThread(NULL, 0, Thread_ActStorm, pdata, 0, 0);
	CloseHandle(lphandle);

	return 0;
}

//给指定房间发送弹幕需完善
int CBilibiliUserList::SendDanmuku(int index, int roomID, std::string msg)
{
	int ret = 0;
	if (index <= 0 || index > _usercount)
		return -1;
	int count = index;
	BOOST_LOG_SEV(g_logger::get(), info) << "[UserList] Danmu Room: " << roomID;
	std::list<CBilibiliUserInfo*>::iterator itor;
	for (itor = _userlist.begin(); itor != _userlist.end(); itor++) {
		if (!(*itor)->GetFileSN() != index)
			continue;
		if (!(*itor)->getLoginStatus())
			break;
		(*itor)->SendDanmuku(roomID, msg);
		break;
	}
	return 0;
}

CBilibiliUserInfo*  CBilibiliUserList::SearchUser(std::string username)
{
	std::list<CBilibiliUserInfo*>::iterator itor;
	for (itor = _userlist.begin(); itor != _userlist.end(); itor++) {
		if ((*itor)->GetUsername() == username)
			return *itor;
	}
	return NULL;
}

int CBilibiliUserList::_HeartExp(int firsttime)
{
	_isworking[1] = true;
	if (_userlist.empty()) {
		_isworking[1] = false;
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

	std::list<CBilibiliUserInfo*>::iterator itor;
	for (itor = _userlist.begin(); itor != _userlist.end(); itor++) {
		if (!(*itor)->getLoginStatus())
			continue;
		if (_heartcount == 0) {
			(*itor)->ActStartHeart();
		}
		else {
			(*itor)->ActHeart();
		}
	}
	_isworking[1] = false;
	return 0;
}

DWORD CBilibiliUserList::Thread_ActTV(PVOID lpParameter)
{
	PTHARED_DATAEX pdata = (PTHARED_DATAEX)lpParameter;
	CBilibiliUserList *pclass = pdata->ptr;
	// 等待领取
	BOOST_LOG_SEV(g_logger::get(), trace) << "[UserList] Thread join gift: " << pdata->id2;
	Sleep(pclass->_GetRand(5000, 5000));
	// 领取为防止冲突 同一时间只能有一个用户在领取
	// 在同一抽奖的两次抽奖之间增加间隔
	std::list<CBilibiliUserInfo*>::iterator itor;
	for (itor = pclass->_userlist.begin(); itor != pclass->_userlist.end(); itor++) {
		if (!(*itor)->getLoginStatus())
			continue;
		Sleep(pclass->_GetRand(1000, 1500));
		EnterCriticalSection(&pclass->_csthread);
		(*itor)->ActSmallTV(pdata->id1, pdata->id2);
		LeaveCriticalSection(&pclass->_csthread);
	}
	delete pdata;
	// 退出线程
	EnterCriticalSection(&pclass->_csthread);
	pclass->_threadcount--;
	LeaveCriticalSection(&pclass->_csthread);

	return 0;
}

DWORD CBilibiliUserList::Thread_ActGuard(PVOID lpParameter)
{
	PTHARED_DATAEX pdata = (PTHARED_DATAEX)lpParameter;
	CBilibiliUserList *pclass = pdata->ptr;
	// 等待领取
	BOOST_LOG_SEV(g_logger::get(), trace) << "[UserList] Thread join guard: " << pdata->id2;
	Sleep(pclass->_GetRand(5000, 5000));
	std::list<CBilibiliUserInfo*>::iterator itor;
	for (itor = pclass->_userlist.begin(); itor != pclass->_userlist.end(); itor++) {
		if (!(*itor)->getLoginStatus())
			continue;
		Sleep(pclass->_GetRand(1000, 1500));
		EnterCriticalSection(&pclass->_csthread);
		(*itor)->ActGuard(pdata->str, pdata->id1, pdata->id2);
		LeaveCriticalSection(&pclass->_csthread);
	}
	delete pdata;
	// 退出线程
	EnterCriticalSection(&pclass->_csthread);
	pclass->_threadcount--;
	LeaveCriticalSection(&pclass->_csthread);
	return 0;
}

DWORD CBilibiliUserList::Thread_ActStorm(PVOID lpParameter)
{
	PTHARED_DATAEX pdata = (PTHARED_DATAEX)lpParameter;
	CBilibiliUserList *pclass = pdata->ptr;
	// 等待领取
	BOOST_LOG_SEV(g_logger::get(), trace) << "[UserList] Thread join guard: " << pdata->id3;
	// Sleep(pclass->_GetRand(8000, 4000));
	// 领取为防止冲突 同一时间只能有一个用户在领取
	// 在同一抽奖的两次抽奖之间增加间隔
	std::list<CBilibiliUserInfo*>::iterator itor;
	for (itor = pclass->_userlist.begin(); itor != pclass->_userlist.end(); itor++) {
		if (!(*itor)->getLoginStatus())
			continue;
		Sleep(pclass->_GetRand(1000, 1500));
		EnterCriticalSection(&pclass->_csthread);
		(*itor)->ActStorm(pdata->id1, pdata->id3);
		LeaveCriticalSection(&pclass->_csthread);
	}
	delete pdata;
	// 退出线程
	EnterCriticalSection(&pclass->_csthread);
	pclass->_threadcount--;
	LeaveCriticalSection(&pclass->_csthread);
	return 0;
}

DWORD CBilibiliUserList::Thread_UserListMSG(PVOID lpParameter)
{
	BOOST_LOG_SEV(g_logger::get(), debug) << "[UserList] Thread Start!";
	CBilibiliUserList *pclass = (CBilibiliUserList *)lpParameter;
	pclass->_isworking[0] = true;
	int bRet = 0;
	MSG msg;
	//开启心跳定时器
	DWORD hearttimer;
	if (pclass->m_workmode == TOOL_EVENT::ONLINE)
		hearttimer = SetTimer(NULL, 1, 60000, NULL);
	while (pclass->_isworking[0])
	{
		if ((bRet = GetMessage(&msg, NULL, 0, 0)) != 0)
		{
			if (msg.message == MSG_BILI_THREADCLOSE)
			{
				KillTimer(NULL, hearttimer);
				hearttimer = 0;
				while (pclass->_isworking[1])
					Sleep(100);
				pclass->_isworking[0] = false;
				BOOST_LOG_SEV(g_logger::get(), debug) << "[UserList] Thread Stop!";
				return 0;
			}
			if (msg.message == WM_TIMER)
			{
				if (msg.wParam == hearttimer) {
					if (pclass->m_workmode == TOOL_EVENT::ONLINE)
						pclass->_HeartExp();
				}
			}
		}
		Sleep(0);
	}
	return 0;
}

int CBilibiliUserList::_GetRand(int start, int len)
{
	srand((unsigned int)time(0));
	return rand() % len + start;
}
