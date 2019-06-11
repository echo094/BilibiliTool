/*
1 在主程序中须调用以下函数初始化和清理，这两个函数非线程安全，在一个
  进程中只需调用一次。
	curl_global_init(CURL_GLOBAL_ALL);
	curl_global_cleanup();
2 类CHTTPPack复用CURL指针以提高性能，在调用函数发送HTTP请求时传入
  外部CURL句柄，方便外部模块对句柄集中调配。
3 使用抓包工具须加入下列语句：
	curl_easy_setopt(pcurl, CURLOPT_PROXY, "127.0.0.1:8888");
4 使用curl_easy_reset()函数会保留CURL句柄中的Cookie信息、TCP连接信息等。
  在一系列调用中只需将Cookie信息导入一次，在有效期内就能够一直使用。
5 无法手动更换CURL句柄中已有Cookie条目的值，只有重新获取新的CURL句柄
  才能够写入具有同样名称的Cookie值。
*/

#pragma once
#ifndef _TOOLLIB_HTTPEX_
#define _TOOLLIB_HTTPEX_

// #define CURL_STATICLIB

#include <curl/curl.h>

#include <string>
#include <vector>
#include <memory>
using std::unique_ptr;

#define MAX_COOKIE_NUM 30U
#define MAX_COOKIE_NAMELEN 50U
#define DEF_UA_FF "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/73.0.3683.103 Safari/537.36"

namespace toollib {

	class CHTTPPack {
	public:
		CHTTPPack(const char *ua = DEF_UA_FF);
		void AddHeaderInf(const char *str);
		void AddHeaderManual(const char *str);
		void ClearHeader();
		void ClearRec();

	private:
		// 不会更改的HTTP头数量
		unsigned header_num_inf;
	public:
		char useragent[255];
		std::string url;
		std::vector<std::string> send_header;
		std::string send_data;
		std::string recv_data;
	};

	// /*
	// CURL的Cookie有7项内容 用制表符隔开
	// 第5项是时间
	// 第6项是名称
	// 第7项是数值
	// */

	// 从Cookie字符串导入 必须格式正确
	// 执行成功返回值为 0
	int HttpImportCookie(CURL *pcurl, const std::string &str);
	// 导出Cookie到字符串
	// 执行成功返回值为 0
	int HttpExportCookie(CURL *pcurl, std::string &str);
	// 获取指定名称的Cookie 成功时返回的数组长度为7
	std::vector<std::string> HttpGetCookieData(CURL *pcurl, const char* ckname);
	// 获取指定Cookie的数值
	// 成功时返回值为 0
	int HttpGetCookieVal(CURL *pcurl, const char* ckname, std::string &ckval);
	// 获取指定Cookie的数值
	// 成功时返回值为 0
	int HttpGetCookieTime(CURL *pcurl, const char* ckname, int &val);

	int HttpGetEx(CURL *pcurl, const unique_ptr<CHTTPPack> &pHTTPPack);
	int HttpPostEx(CURL *pcurl, const unique_ptr<CHTTPPack> &pHTTPPack);
	int HttpHeadEx(CURL *pcurl, const unique_ptr<CHTTPPack> &pHTTPPack);

}

#endif
