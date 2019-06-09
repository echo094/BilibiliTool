#include "stdafx.h"
#include "BilibiliMain.h"
#include "log.h"

int main() {
#ifdef WIN32
	// 设置控制台格式为UTF-8
	SetConsoleOutputCP(65001);
#endif
	// 初始化log
	boost_log_init();
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
	// 清空log的sinks
	boost_log_deinit();

    return 0;
}
