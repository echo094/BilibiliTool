﻿// Bilibili.cpp : 定义控制台应用程序的入口点。
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
CBilibiliMain *m_BilibiliMain;
//用于查询的共享CURL句柄
CURL* curl;

bool threadflag = false;//主线程运行状态
DWORD threadid;//主线程序号
HANDLE threadhandle;//主线程句柄

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
	//用于线程退出循环的标志位
	bool runflag = true;
	//是否有功能模块正在运行
	bool isrunning = false;

	int bRet = 0, ret;
	MSG msg;
	while (runflag)
	{
		//Peek不阻塞但占用内存,如果使用GetMessage会阻塞
		if ((bRet = GetMessage(&msg, NULL, 0, 0)) != 0)
		{
			TranslateMessage(&msg);
			if (msg.message == ON_USER_COMMAND)
			{
				TOOL_EVENT opt = static_cast<TOOL_EVENT>(msg.wParam);
				if (msg.wParam == 0)
					runflag = false;
				else if (opt == TOOL_EVENT::STOP) {
					ret = m_BilibiliMain->StopMonitorALL();
					isrunning = false;
				}
				else if (opt == TOOL_EVENT::ONLINE) {
					if (isrunning) {
						printf("Another task is working. \n");
					}
					else {
						isrunning = true;
						ret = m_BilibiliMain->StartUserHeart();
					}
				}
				else if (opt == TOOL_EVENT::GET_SYSMSG_GIFT) {
					if (isrunning) {
						printf("Another task is working. \n");
					}
					else {
						isrunning = true;
						ret = m_BilibiliMain->StartMonitorPubEvent(threadid);
					}
				}
				else if (opt == TOOL_EVENT::DEBUG1) {
					m_BilibiliMain->Debugfun(1);
				}
				else if (opt == TOOL_EVENT::DEBUG2) {
					m_BilibiliMain->Debugfun(2);
				}
			}
			if (msg.message == MSG_NEWSMALLTV)
			{
				m_BilibiliMain->JoinTV(msg.wParam);
			}
			if (msg.message == MSG_NEWYUNYING)
			{
				m_BilibiliMain->JoinYunYing(msg.wParam);
			}
			if (msg.message == MSG_NEWYUNYINGDAILY)
			{
				m_BilibiliMain->JoinYunYingGift(msg.wParam);
			}
			if (msg.message == MSG_NEWGUARD)
			{
				std::string *puser = (std::string *)msg.wParam;
				m_BilibiliMain->JoinGuardGift(puser->c_str());
				delete puser;
			}
			if (msg.message == MSG_NEWSPECIALGIFT)
			{
				tagSPECIALGIFT *tspecialgift = (tagSPECIALGIFT *)msg.lParam;
				m_BilibiliMain->JoinSpecialGift(msg.wParam, tspecialgift->id, tspecialgift->content);
				delete tspecialgift;
			}
			if (msg.message == WM_TIMER)
			{
				DispatchMessage(&msg);
			}
		}
		//todo something
		Sleep(0);
	}
	//开始清理过程
	if (isrunning)
		ret = m_BilibiliMain->StopMonitorALL();
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
	m_BilibiliMain = new CBilibiliMain(curl);

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
	m_BilibiliMain->SetCURLHandle(NULL);
	delete m_BilibiliMain;
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
  >12 lotterystart\t 开启领取小电视等道具 \n\
  >21 savelog     \t 更新日志文件         \n\
  >22 danmukuon   \t 显示弹幕消息         \n\
  >23 danmukuoff  \t 隐藏弹幕消息         \n\
  >   help        \t 目录                 \n\
  >   exit        \t 退出                 \n");
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
		m_BilibiliMain->GetUserList()->ImportUserList();
	}
	else if (!str.compare("2") || !str.compare("userexport")) {
		m_BilibiliMain->GetUserList()->ExportUserList();
	}
	else if (!str.compare("3") || !str.compare("userlist")) {
		m_BilibiliMain->GetUserList()->ShowUserList();
	}
	else if (!str.compare("4") || !str.compare("useradd")) {
		std::string name, psd;
		cout << "Account: ";
		getline(cin, name);
		cout << "Password: ";
		GetPassword(psd);
		m_BilibiliMain->GetUserList()->AddUser(name, psd);
	}
	else if (!str.compare("5") || !str.compare("userdel")) {
		char tstr[30];
		cout << "Account: ";
		cin >> tstr;
		m_BilibiliMain->GetUserList()->DeleteUser(tstr);
	}
	else if (!str.compare("6") || !str.compare("userre")) {
		m_BilibiliMain->GetUserList()->ReloginAll();
	}
	else if (!str.compare("7") || !str.compare("userlogin")) {
		m_BilibiliMain->GetUserList()->CheckUserStatusALL();
	}
	else if (!str.compare("8") || !str.compare("usergetinfo")) {
		m_BilibiliMain->GetUserList()->GetUserInfoALL();
	}
	else if (!str.compare("10") || !str.compare("stopall")) {
		PostThreadMessage(threadid, ON_USER_COMMAND, WPARAM(TOOL_EVENT::STOP), LPARAM(0));
	}
	else if (!str.compare("11") || !str.compare("userexp")) {
		PostThreadMessage(threadid, ON_USER_COMMAND, WPARAM(TOOL_EVENT::ONLINE), LPARAM(0));
	}
	else if (!str.compare("12") || !str.compare("lotterystart")) {
		PostThreadMessage(threadid, ON_USER_COMMAND, WPARAM(TOOL_EVENT::GET_SYSMSG_GIFT), LPARAM(0));
	}
	else if (!str.compare("21")) {
		printf("Saving lottery history... \n");
		m_BilibiliMain->SaveLogFile();
	}
	else if (!str.compare("22")) {
		printf("Danmuku Print. \n");
		m_BilibiliMain->SetDanmukuShow();
	}
	else if (!str.compare("23")) {
		printf("Danmuku Drop. \n");
		m_BilibiliMain->SetDanmukuHide();
	}
	else if (!str.compare("90")) {
		PostThreadMessage(threadid, ON_USER_COMMAND, WPARAM(TOOL_EVENT::DEBUG1), LPARAM(0));
	}
	else {
		printf("未知命令\n");
	}
	return 1;
}