#include "stdafx.h"
#include "httpex.h"
using namespace toollib;
#include<iostream>
#include <sstream>

CHTTPPack::CHTTPPack(const char *ua) {
	strcpy_s(useragent, ua);
	//所有HTTP包都需要的头
	header_num_def = 0;
	url.empty();
	recv_data = "";
}

void CHTTPPack::AddDefHeader(const char *str) {
	header_num_def++;
	send_header.push_back(str);
}

void CHTTPPack::ClearHeader() {
	send_header.resize(header_num_def);
}

void CHTTPPack::ClearRec() {
	recv_data = "";
}

void CHTTPPack::AddHeaderManual(const char *str) {
	send_header.push_back(str);
}


int CCookiePack::ImportCookies(std::string &str, CURL *pcurl) {
	cookie = str;
	if (pcurl == NULL)
		return 0;
	ApplyCookies(pcurl);

	return 0;
}

int CCookiePack::ExportCookies(std::string &str, CURL *pcurl) {
	if (pcurl) {
		UpdateCookies(pcurl);
	}
	str = cookie;

	return 0;
}

int CCookiePack::ApplyCookies(CURL *pcurl) {
	if (pcurl == NULL)
		return 0;
	int pl = 0, pr = cookie.find('\n', 0);
	while (pr != -1) {
		curl_easy_setopt(pcurl, CURLOPT_COOKIELIST, cookie.c_str() + pl);
		pl = pr + 1;
		pr = cookie.find('\n', pl);
	}

	return 0;
}

int CCookiePack::UpdateCookies(CURL *pcurl) {
	struct curl_slist *pcookies = NULL, *cur = NULL;
	if (pcurl) {
		cookie = "";
		curl_easy_getinfo(pcurl, CURLINFO_COOKIELIST, &pcookies);
		cur = pcookies;
		while (cur) {
			cookie += cur->data;
			cookie += "\n";
			cur = cur->next;
		}
		curl_slist_free_all(pcookies);
		pcookies = NULL;
	}

	return 0;
}

int CCookiePack::GetCookie(std::string &name, std::string &value) {
	std::string str = "\t" + name + "\t";
	int pl = cookie.find(str, 0);
	if (pl == -1)
		return -1;
	pl += str.length();
	int  pr = cookie.find('\n', pl);
	if (pr == -1)
		return -1;
	value = cookie.substr(pl, pr - pl);

	return 0;
}

int CCookiePack::GetCookieTime(std::string &name, int &value) const {
	std::string str = "\t" + name + "\t";
	int pr = cookie.find(str, 0);
	if (pr == -1)
		return -1;
	int  pl = cookie.find_last_of('\t', pr - 1);
	if (pl == -1)
		return -1;
	str = cookie.substr(pl + 1, pr - pl - 1);
	value = atoi(str.c_str());

	return 0;
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
	// 启动CURL内部的Cookie引擎
	curl_easy_setopt(pcurl, CURLOPT_COOKIEFILE, "");
	// 设置接受的 Cookies
	// curl_easy_setopt(curl, CURLOPT_COOKIEJAR, cookieFilePath);
	// 设置UA
	curl_easy_setopt(pcurl, CURLOPT_USERAGENT, pHTTPPack->useragent);
	// 自动解压Header
	curl_easy_setopt(pcurl, CURLOPT_ACCEPT_ENCODING, "gzip");
	// 不检查证书
	curl_easy_setopt(pcurl, CURLOPT_SSL_VERIFYPEER, FALSE);
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
