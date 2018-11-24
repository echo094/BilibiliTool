#include "stdafx.h"
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
		BOOST_LOG_SEV(g_logger::get(), error) << "[Main] Initialize SOCKET failed.";
		return -1;
	}
	// 初始化libcurl库
	curl_global_init(CURL_GLOBAL_ALL);

	// 创建Bili助手主模块
	unique_ptr<CBilibiliMain> g_BilibiliMain;
	g_BilibiliMain = std::make_unique<CBilibiliMain>();
	g_BilibiliMain->Run();
	g_BilibiliMain.reset();
	BOOST_LOG_SEV(g_logger::get(), info) << "[Main] Do some cleaning...";

	// 清理libcurl库
	curl_global_cleanup();
	// 释放SOCKET库
	WSACleanup();
	// 清空log的sinks
	boost_log_deinit();

    return 0;
}
