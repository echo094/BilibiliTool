// Bilibili.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <assert.h>
#include <string>
#include <conio.h> //getch()
#include <iostream>
using namespace std;
#include "BilibiliMain.h"

#define BILI_HEPLER_HEADER "\n Bilibili Tool \n"

//本软件主模块
static unique_ptr<CBilibiliMain> g_BilibiliMain;
//用于查询的共享CURL句柄
static CURL* curl;

static bool threadflag = false;//主线程运行状态
static DWORD threadid;//主线程序号
static HANDLE threadhandle;//主线程句柄

#define ON_USER_COMMAND WM_USER + 610 

// 窗口密码输入与获取
int GetPassword(std::string &psd);
// 帮助菜单
void printHelp();
// 指令处理函数
int ProcessCommand(std::string str);

static DWORD WINAPI Thread_BilibiliMain(PVOID lpParameter)
{
	threadflag = true;
	OutputDebugString(_T("BilibiliWIN32 -> Thread Start!\n"));
	// 用于线程退出循环的标志位
	bool runflag = true;
	// 是否有功能模块正在运行
	bool isrunning = false;
	// 主线程定时器
	DWORD hearttimer;

	int bRet = 0, ret;
	MSG msg;
	while (runflag)
	{
		//Peek不阻塞但占用内存,如果使用GetMessage会阻塞
		if ((bRet = GetMessage(&msg, NULL, 0, 0)) != 0)
		{
			TranslateMessage(&msg);
			if (msg.message == MSG_NEWSMALLTV)
			{
				g_BilibiliMain->JoinTV(msg.wParam);
			}
			else if (msg.message == MSG_NEWGUARD0)
			{
				// 房间上船事件
				BILI_LOTTERYDATA *pinfo = (BILI_LOTTERYDATA *)msg.wParam;
				g_BilibiliMain->JoinGuardGift(*pinfo);
				delete pinfo;
			}
			else if (msg.message == MSG_NEWGUARD1)
			{
				// 广播上船事件
				g_BilibiliMain->JoinGuardGift(msg.wParam);
			}
			else if (msg.message == MSG_NEWSPECIALGIFT)
			{
				BILI_ROOMEVENT *pinfo = (BILI_ROOMEVENT *)msg.wParam;
				g_BilibiliMain->JoinSpecialGift(pinfo->rid, pinfo->loidl);
				delete pinfo;
			}
			else if (msg.message == WM_TIMER)
			{
				if (msg.wParam == hearttimer) {
					g_BilibiliMain->UpdateLiveRoom();
				}
			}
			else if (msg.message == ON_USER_COMMAND)
			{
				TOOL_EVENT opt = static_cast<TOOL_EVENT>(msg.wParam);
				if (msg.wParam == 0)
					runflag = false;
				else if (opt == TOOL_EVENT::STOP) {
					KillTimer(NULL, hearttimer);
					ret = g_BilibiliMain->StopMonitorALL();
					isrunning = false;
				}
				else if (opt == TOOL_EVENT::ONLINE) {
					if (isrunning) {
						printf("Another task is working. \n");
					}
					else {
						isrunning = true;
						ret = g_BilibiliMain->StartUserHeart();
					}
				}
				else if (opt == TOOL_EVENT::GET_SYSMSG_GIFT) {
					if (isrunning) {
						printf("Another task is working. \n");
					}
					else {
						isrunning = true;
						ret = g_BilibiliMain->StartMonitorPubEvent(threadid);
					}
				}
				else if (opt == TOOL_EVENT::GET_HIDEN_GIFT) {
					if (isrunning) {
						printf("Another task is working. \n");
					}
					else {
						isrunning = true;
						// 每5分钟刷新房间
						hearttimer = SetTimer(NULL, 1, 300000, NULL);
						ret = g_BilibiliMain->StartMonitorHiddenEvent(threadid);
					}
				}
				else if (opt == TOOL_EVENT::DEBUG1) {
					g_BilibiliMain->Debugfun(1);
				}
				else if (opt == TOOL_EVENT::DEBUG2) {
					g_BilibiliMain->Debugfun(2);
				}
			}
		}
		//todo something
		Sleep(0);
	}
	//开始清理过程
	if (isrunning)
		ret = g_BilibiliMain->StopMonitorALL();
	OutputDebugString(_T("BilibiliWIN32 -> Thread Stop!\n"));
	threadflag = false;
	return 0;
}

int main()
{
	int ret = 1;
	// 初始化Socket
	WSADATA wsaData;
	ret = WSAStartup(MAKEWORD(2, 2), &wsaData);//协议库的版本信息
	if (ret){
		printf("Initialize SOCKET failed. \n");
		system("pause");
		return -1;
	}
	// 初始化libcurl库
	curl_global_init(CURL_GLOBAL_ALL);
	curl = curl_easy_init();

	// 创建Bili助手主模块
	g_BilibiliMain = std::make_unique<CBilibiliMain>(curl);

	string command;
	setlocale(LC_ALL, "chs");
	threadhandle = CreateThread(NULL, 0, Thread_BilibiliMain, 0, 0, &threadid); //辅助线程
	assert(threadhandle);
	if (!threadhandle){
		threadhandle = INVALID_HANDLE_VALUE;
		return 1;
	}
	printHelp();
	ret = 1;
	while (ret)
	{
		printf("> ");
		while (getline(cin, command) && !command.size());
		ret = ProcessCommand(command);
	}
	printf("Waiting to exit... \n");
	while (threadflag)
		Sleep(100);

	printf("Do some cleaning... \n");
	g_BilibiliMain->SetCURLHandle(NULL);
	g_BilibiliMain = nullptr;
	// 清理libcurl库
	printf("Cleaning CURL handles... \n");
	curl_easy_cleanup(curl);
	curl = NULL;
	curl_global_cleanup();
	// 释放SOCKET库
	WSACleanup();
	printf("Success. \n");
	system("pause");
    return 0;
}

int GetPassword(std::string &psd)
{
	int ret = 0;
	char ch;
	unsigned int ich;
	psd = "";

	while (1) {
		ich = _getch();
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

void printHelp()
{
	printf(BILI_HEPLER_HEADER);
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
  >22 danmukuon   \t 显示弹幕消息         \n\
  >23 danmukuoff  \t 隐藏弹幕消息         \n\
  >   help        \t 目录                \n\
  >   exit        \t 退出                \n");
}

int ProcessCommand(std::string str)
{
	if (str == "")
		return 1;
	if (!str.compare("exit"))
	{
		CloseHandle(threadhandle);
		PostThreadMessage(threadid, ON_USER_COMMAND, WPARAM(0), LPARAM(0));
		return 0;
	}
	if (!str.compare("help"))
	{
		printHelp();
	}
	else if (!str.compare("1") || !str.compare("userimport")) {
		g_BilibiliMain->GetUserList()->ImportUserList();
	}
	else if (!str.compare("2") || !str.compare("userexport")) {
		g_BilibiliMain->GetUserList()->ExportUserList();
	}
	else if (!str.compare("3") || !str.compare("userlist")) {
		g_BilibiliMain->GetUserList()->ShowUserList();
	}
	else if (!str.compare("4") || !str.compare("useradd")) {
		std::string name, psd;
		cout << "Account: ";
		getline(cin, name);
		cout << "Password: ";
		GetPassword(psd);
		g_BilibiliMain->GetUserList()->AddUser(name, psd);
	}
	else if (!str.compare("5") || !str.compare("userdel")) {
		char tstr[30];
		cout << "Account: ";
		cin >> tstr;
		g_BilibiliMain->GetUserList()->DeleteUser(tstr);
	}
	else if (!str.compare("6") || !str.compare("userre")) {
		g_BilibiliMain->GetUserList()->ReloginAll();
	}
	else if (!str.compare("7") || !str.compare("userlogin")) {
		g_BilibiliMain->GetUserList()->CheckUserStatusALL();
	}
	else if (!str.compare("8") || !str.compare("usergetinfo")) {
		g_BilibiliMain->GetUserList()->GetUserInfoALL();
	}
	else if (!str.compare("10") || !str.compare("stopall")) {
		PostThreadMessage(threadid, ON_USER_COMMAND, WPARAM(TOOL_EVENT::STOP), LPARAM(0));
	}
	else if (!str.compare("11") || !str.compare("userexp")) {
		PostThreadMessage(threadid, ON_USER_COMMAND, WPARAM(TOOL_EVENT::ONLINE), LPARAM(0));
	}
	else if (!str.compare("12") || !str.compare("startlp")) {
		PostThreadMessage(threadid, ON_USER_COMMAND, WPARAM(TOOL_EVENT::GET_SYSMSG_GIFT), LPARAM(0));
	}
	else if (!str.compare("13") || !str.compare("startlh")) {
		PostThreadMessage(threadid, ON_USER_COMMAND, WPARAM(TOOL_EVENT::GET_HIDEN_GIFT), LPARAM(0));
	}
	else if (!str.compare("21")) {
		printf("Saving lottery history... \n");
		g_BilibiliMain->SaveLogFile();
	}
	else if (!str.compare("22")) {
		printf("Danmuku Print. \n");
		g_BilibiliMain->SetDanmukuShow();
	}
	else if (!str.compare("23")) {
		printf("Danmuku Drop. \n");
		g_BilibiliMain->SetDanmukuHide();
	}
	else if (!str.compare("90")) {
		PostThreadMessage(threadid, ON_USER_COMMAND, WPARAM(TOOL_EVENT::DEBUG1), LPARAM(0));
	}
	else if (!str.compare("91")) {
		PostThreadMessage(threadid, ON_USER_COMMAND, WPARAM(TOOL_EVENT::DEBUG2), LPARAM(0));
	}
	else {
		printf("未知命令\n");
	}
	return 1;
}
