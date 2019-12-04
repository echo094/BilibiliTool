#include "httpex.h"
#include <iostream>
#include <sstream>
#include <boost/algorithm/string.hpp>
using namespace toollib;

CHTTPPack::CHTTPPack(const char *ua) :
	header_num_inf(0) {
	strcpy(useragent, ua);
	url.empty();
	recv_data = "";
}

void CHTTPPack::AddHeaderInf(const char *str) {
	send_header.insert(send_header.begin() + header_num_inf, str);
	header_num_inf++;
}

void CHTTPPack::AddHeaderManual(const char *str) {
	send_header.push_back(str);
}

void CHTTPPack::ClearHeader() {
	send_header.resize(header_num_inf);
}

void CHTTPPack::ClearRec() {
	recv_data = "";
}

int toollib::HttpImportCookie(CURL * pcurl, const std::string & str) {
	size_t pl = 0, pr = str.find('\n', 0);
	while (pr != std::string::npos) {
		CURLcode ret = curl_easy_setopt(pcurl, CURLOPT_COOKIELIST, str.c_str() + pl);
		if (ret != CURLE_OK) {
			return ret;
		}
		pl = pr + 1;
		pr = str.find('\n', pl);
	}
	return 0;
}

int toollib::HttpExportCookie(CURL * pcurl, std::string & str) {
	struct curl_slist *pcookies = NULL, *cur = NULL;
	str = "";
	CURLcode ret = curl_easy_getinfo(pcurl, CURLINFO_COOKIELIST, &pcookies);
	if (ret != CURLE_OK) {
		curl_slist_free_all(pcookies);
		return ret;
	}
	cur = pcookies;
	while (cur) {
		str += cur->data;
		str += "\n";
		cur = cur->next;
	}
	curl_slist_free_all(pcookies);
	return 0;
}

std::vector<std::string> toollib::HttpGetCookieData(CURL *pcurl, const char* ckname) {
	struct curl_slist *pcookies = NULL, *cur = NULL;
	CURLcode ret = curl_easy_getinfo(pcurl, CURLINFO_COOKIELIST, &pcookies);
	if (ret != CURLE_OK) {
		curl_slist_free_all(pcookies);
		return std::vector<std::string>();
	}
	cur = pcookies;
	while (cur) {
		std::vector<std::string> vec;
		boost::split(vec, cur->data, boost::is_any_of("\t"), boost::token_compress_off);
		if (vec[5] == ckname) {
			curl_slist_free_all(pcookies);
			return vec;
		}
		cur = cur->next;
	}
	curl_slist_free_all(pcookies);
	return std::vector<std::string>();
}

int toollib::HttpGetCookieVal(CURL *pcurl, const char* ckname, std::string &val) {
	std::vector<std::string> vec(toollib::HttpGetCookieData(pcurl, ckname));
	if (vec.size() == 7) {
		val = vec[6];
		return 0;
	}
	return -1;
}

int toollib::HttpGetCookieTime(CURL *pcurl, const char* ckname, int &val) {
	std::vector<std::string> vec(toollib::HttpGetCookieData(pcurl, ckname));
	if (vec.size() == 7) {
		val = atoi(vec[4].c_str());
		return 0;
	}
	return -1;
}

static size_t write_data_callback(void *ptr, size_t size, size_t nmemb, void *stream) {
	size_t realsize = size * nmemb;
	if (nullptr == ptr) {
		return 0;
	}
	if (nullptr == stream) {
		return realsize;
	}
	char* contents = (char*)ptr;
	std::string* buff = (std::string *)stream;
	buff->append(contents, realsize);
	return realsize;
}

CURLcode http_perform(CURL *pcurl, const unique_ptr<CHTTPPack> &pHTTPPack) {
	CURLcode ret;
	// 存放HTTP表头
	struct curl_slist *slist = NULL;
	// 设定URL
	curl_easy_setopt(pcurl, CURLOPT_URL, pHTTPPack->url.c_str());
	// 自定义Header内容
	for (unsigned i = 0; i < pHTTPPack->send_header.size(); i++) {
		slist = curl_slist_append(slist, pHTTPPack->send_header[i].c_str());
	}
	// 将自定义Header添加到curl包
	if (slist != NULL) {
		curl_easy_setopt(pcurl, CURLOPT_HTTPHEADER, slist);
	}
	// 设置支持302重定向
	curl_easy_setopt(pcurl, CURLOPT_FOLLOWLOCATION, 1);
	// 启动CURL内部的Cookie引擎
	curl_easy_setopt(pcurl, CURLOPT_COOKIEFILE, "");
	// 设置接受的 Cookies
	// curl_easy_setopt(curl, CURLOPT_COOKIEJAR, cookieFilePath);
	// 设置UA
	curl_easy_setopt(pcurl, CURLOPT_USERAGENT, pHTTPPack->useragent);
	// 自动解压Header
	curl_easy_setopt(pcurl, CURLOPT_ACCEPT_ENCODING, "gzip");
	// 不检查证书
	curl_easy_setopt(pcurl, CURLOPT_SSL_VERIFYPEER, false);
	// 设置代理
	// curl_easy_setopt(pcurl, CURLOPT_PROXY, "127.0.0.1:8888");
	// 设置超时时间
	curl_easy_setopt(pcurl, CURLOPT_TIMEOUT, 10L);
	// 阻塞方式执行
	ret = curl_easy_perform(pcurl);
	curl_easy_reset(pcurl);
	if (slist != NULL) {
		curl_slist_free_all(slist);
	}
	if (ret != CURLE_OK) {
		printf("curl_easy_perform() failed: %s\n", curl_easy_strerror(ret));
	}
	return ret;
}

int toollib::HttpGetEx(CURL *pcurl, const unique_ptr<CHTTPPack> &pHTTPPack) {
	if (pcurl == NULL) {
		return -1;
	}

	//对返回的数据进行操作的函数地址
	pHTTPPack->ClearRec();
	curl_easy_setopt(pcurl, CURLOPT_WRITEDATA, (void *)&pHTTPPack->recv_data);
	curl_easy_setopt(pcurl, CURLOPT_WRITEFUNCTION, write_data_callback);
	//执行
	CURLcode res = http_perform(pcurl, pHTTPPack);

	return res;
}

int toollib::HttpPostEx(CURL *pcurl, const unique_ptr<CHTTPPack> &pHTTPPack) {
	if (pcurl == NULL) {
		return -1;
	}

	//标记为POST
	curl_easy_setopt(pcurl, CURLOPT_POST, 1L);
	//POST数据格式为char*否则无法正常读取和发送
	curl_easy_setopt(pcurl, CURLOPT_POSTFIELDS, pHTTPPack->send_data.c_str());
	//对返回的数据进行操作的函数地址
	pHTTPPack->ClearRec();
	curl_easy_setopt(pcurl, CURLOPT_WRITEDATA, (void *)&pHTTPPack->recv_data);
	curl_easy_setopt(pcurl, CURLOPT_WRITEFUNCTION, write_data_callback);
	//执行
	CURLcode res = http_perform(pcurl, pHTTPPack);

	return res;
}

int toollib::HttpHeadEx(CURL *pcurl, const unique_ptr<CHTTPPack> &pHTTPPack) {
	if (pcurl == NULL) {
		return -1;
	}

	// 标记为HEAD
	curl_easy_setopt(pcurl, CURLOPT_NOBODY, 1L);
	// 执行
	CURLcode res = http_perform(pcurl, pHTTPPack);

	return res;
}
