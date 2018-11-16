#include "stdafx.h"
#include <iostream>
#include "BilibiliMain.h"
#include "log.h"

int main() {
	setlocale(LC_ALL, "chs");
	int ret = 1;
	// 初始化log
	boost_log_init();
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

	// 创建Bili助手主模块
	unique_ptr<CBilibiliMain> g_BilibiliMain;
	g_BilibiliMain = std::make_unique<CBilibiliMain>();
	g_BilibiliMain->Run();
	printf("Do some cleaning... \n");
	g_BilibiliMain = nullptr;

	// 清理libcurl库
	printf("Cleaning CURL handles... \n");
	curl_global_cleanup();
	// 释放SOCKET库
	WSACleanup();
	// 清空log的sinks
	boost_log_deinit();

	printf("Success. \n");
	system("pause");
    return 0;
}
