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
#ifdef CURL_STATICLIB
#pragma comment( lib, "normaliz.lib")
#pragma comment( lib, "wldap32.lib")
#pragma comment( lib, "ws2_32.lib")
#pragma comment( lib, "libcurl_a.lib")
#else
#pragma comment( lib, "libcurl.lib")
#endif

#include <string>
#include <vector>
#include <memory>
using std::unique_ptr;

#define MAX_COOKIE_NUM 30U
#define MAX_COOKIE_NAMELEN 50U
#define DEF_UA_FF "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/69.0.3497.100 Safari/537.36"

namespace toollib {

	class CHTTPPack
	{

	public:
		CHTTPPack(const char *ua = DEF_UA_FF);
		void AddDefHeader(const char *str);
		void ClearHeader();
		void ClearRec();
		void AddHeaderManual(const char *str);

	private:
		// 不会更改的HTTP头数量
		unsigned header_num_def;
	public:
		char useragent[255];
		std::string url, send_data;
		std::vector<std::string> send_header;
		std::string recv_data;
	};

	class CCookiePack
	{
	private:
		std::string cookie;
		//Cookie相关
	public:
		int ImportCookies(std::string &str, CURL *pcurl = NULL);
		int ExportCookies(std::string &str, CURL *pcurl = NULL);
		//将Cookie应用到CURL句柄
		int ApplyCookies(CURL *pcurl);
		//将CURL句柄中的cookie更新到类变量中
		int UpdateCookies(CURL *pcurl);
		//查找特定名称的Cookie值
		int GetCookie(std::string &name, std::string &value);
		int GetCookieTime(std::string &name, int &value) const;
	};

	int HttpGetEx(CURL *pcurl, const unique_ptr<CHTTPPack> &pHTTPPack);
	int HttpPostEx(CURL *pcurl, const unique_ptr<CHTTPPack> &pHTTPPack);
	int HttpHeadEx(CURL *pcurl, const unique_ptr<CHTTPPack> &pHTTPPack);

}

#endif
