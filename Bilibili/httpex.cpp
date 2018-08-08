#include "stdafx.h"
#include "httpex.h"
using namespace toollib;
#include<iostream>
#include <sstream>

CHTTPPack::CHTTPPack(const char *ua)
{
	strcpy_s(useragent, ua);
	url.empty();
	//所有HTTP包都需要的头
	_defheadernum = 0;
	i_numsendheader = 0;
	strrecheader = NULL;
	strrecdata = NULL;
	sstrrecheader = "";
	sstrrecdata = "";
}

CHTTPPack::~CHTTPPack()
{
	if (strrecheader != NULL)
		free(strrecheader);
	if (strrecdata != NULL)
		free(strrecdata);
}

int CHTTPPack::AddDefHeader(const char *str)
{
	_defheadernum++;
	strsendheader[_defheadernum] = str;
	i_numsendheader = _defheadernum;
	return 0;
}

bool CHTTPPack::ClearHeader()
{
	i_numsendheader = _defheadernum;
	return true;
}

bool CHTTPPack::ClearRec()
{
	sstrrecheader = "";
	sstrrecdata = "";
	if (strrecheader != NULL)
		delete[] strrecheader;
	strrecheader = NULL;
	if (strrecdata != NULL)
		delete[] strrecdata;
	strrecdata = NULL;
	i_lenrecheader = 0;
	i_lenrecdata = 0;
	return true;
}

bool CHTTPPack::AddHeaderManual(const char *tsheader)
{
	strsendheader[i_numsendheader] = tsheader;
	i_numsendheader++;
	return true;
}



int CCookiePack::ImportCookies(std::string &str, CURL *pcurl)
{
	cookie = str;
	if (pcurl == NULL)
		return 0;
	ApplyCookies(pcurl);

	return 0;
}

int CCookiePack::ExportCookies(std::string &str, CURL *pcurl)
{
	if (pcurl) {
		UpdateCookies(pcurl);
	}
	str = cookie;

	return 0;
}

int CCookiePack::ApplyCookies(CURL *pcurl)
{
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

int CCookiePack::UpdateCookies(CURL *pcurl)
{
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

int CCookiePack::GetCookie(std::string &name, std::string &value)
{
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

int CCookiePack::GetCookieTime(std::string &name, int &value) const
{
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

/*
回调函数 curl_easy
pData是指向存储数据的指针
size是每个块的大小
nmemb是指块的数目
stream是用户参数
*/
static size_t write_header_callback(void *ptr, size_t size, size_t nmemb, void *stream)
{
	size_t realsize = size * nmemb;
	/*
	char*str = (char*)ptr;
	for (int i = 0; i<realsize; i++)
		std::cout << str[i];
	*/
	std::string* sstr = dynamic_cast<std::string*>((std::string *)stream);
	if (NULL == sstr || NULL == ptr)
		return -1;
	char* pData = (char*)ptr;
	sstr->append(pData, realsize);

	return nmemb;
}

static size_t write_data_callback(void *ptr, size_t size, size_t nmemb, void *stream)
{
	size_t realsize = size * nmemb;
	/*
	char*str = (char*)ptr;
	for (int i = 0; i<realsize; i++)
		std::cout << str[i];
	*/
	std::string* sstr = dynamic_cast<std::string*>((std::string *)stream);
	if (NULL == sstr || NULL == ptr)
		return -1;
	char* pData = (char*)ptr;
	sstr->append(pData, realsize);

	return nmemb;
}

int toollib::HttpGetEx(CURL *pcurl, CHTTPPack *pHTTPPack, int flag)
{
	CURLcode res;
	struct curl_slist *slist = NULL;//存放HTTP表头
	size_t i;

	if (pcurl == NULL)
		return -1;
	if (pcurl) {
		//设定URL
		curl_easy_setopt(pcurl, CURLOPT_URL, pHTTPPack->url.c_str());
		//不检查证书
		curl_easy_setopt(pcurl, CURLOPT_SSL_VERIFYPEER, FALSE);
		//设置UA
		curl_easy_setopt(pcurl, CURLOPT_USERAGENT, pHTTPPack->useragent);
		//自定义Header内容
		for (i=0;i<pHTTPPack->i_numsendheader;i++)
			slist = curl_slist_append(slist, pHTTPPack->strsendheader[i].c_str());
		//将自定义Header添加到curl包
		if (slist != NULL)
			curl_easy_setopt(pcurl, CURLOPT_HTTPHEADER, slist);
		//自动解压Header
		curl_easy_setopt(pcurl, CURLOPT_ACCEPT_ENCODING, "gzip");
		//设置代理
		//curl_easy_setopt(pcurl, CURLOPT_PROXY, "127.0.0.1:8888");
		//启动CURL内部的Cookie引擎
		curl_easy_setopt(pcurl, CURLOPT_COOKIEFILE, "");
		/* 设置接受的 Cookies */
		//curl_easy_setopt(curl, CURLOPT_COOKIEJAR, cookieFilePath);
		//对返回的数据进行操作的函数地址
		pHTTPPack->ClearRec();
		curl_easy_setopt(pcurl, CURLOPT_HEADERDATA, (void *)&pHTTPPack->sstrrecheader);
		curl_easy_setopt(pcurl, CURLOPT_HEADERFUNCTION, write_header_callback);
		curl_easy_setopt(pcurl, CURLOPT_WRITEDATA, (void *)&pHTTPPack->sstrrecdata);
		curl_easy_setopt(pcurl, CURLOPT_WRITEFUNCTION, write_data_callback);

		//执行
		res = curl_easy_perform(pcurl);
		if (res != CURLE_OK)
			printf("curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

		//转换字符串
		if (flag)
		{
			size_t tmplen;
			tmplen = pHTTPPack->sstrrecheader.size();
			pHTTPPack->strrecheader = new char[tmplen + 1];
			for (i = 0; i < tmplen; i++)
				pHTTPPack->strrecheader[i] = pHTTPPack->sstrrecheader[i];
			pHTTPPack->strrecheader[tmplen] = 0;
			pHTTPPack->i_lenrecheader = tmplen;

			tmplen = pHTTPPack->sstrrecdata.size();
			pHTTPPack->strrecdata = new char[tmplen + 1];
			for (i = 0; i < tmplen; i++)
				pHTTPPack->strrecdata[i] = pHTTPPack->sstrrecdata[i];
			pHTTPPack->strrecdata[tmplen] = 0;
			pHTTPPack->i_lenrecdata = tmplen;
		}

		curl_easy_reset(pcurl);
		/* free the list again */
		if (slist != NULL)
			curl_slist_free_all(slist); 
	}

	return res;
}

int toollib::HttpPostEx(CURL *pcurl, CHTTPPack *pHTTPPack, int flag)
{
	CURLcode res;
	struct curl_slist *slist = NULL;//存放HTTP表头
	size_t i;

	if (pcurl == NULL)
		return -1;
	if (pcurl) {
		//设定URL
		curl_easy_setopt(pcurl, CURLOPT_URL, pHTTPPack->url.c_str());
		//标记为POST
		curl_easy_setopt(pcurl, CURLOPT_POST, 1L);
		//POST数据格式为char*否则无法正常读取和发送
		curl_easy_setopt(pcurl, CURLOPT_POSTFIELDS, pHTTPPack->strsenddata.c_str());
		//不检查证书
		curl_easy_setopt(pcurl, CURLOPT_SSL_VERIFYPEER, FALSE);
		//设置UA
		curl_easy_setopt(pcurl, CURLOPT_USERAGENT, pHTTPPack->useragent);
		//自定义Header内容
		for (i = 0; i<pHTTPPack->i_numsendheader; i++)
			slist = curl_slist_append(slist, pHTTPPack->strsendheader[i].c_str());
		//将自定义Header添加到curl包
		if (slist != NULL)
			curl_easy_setopt(pcurl, CURLOPT_HTTPHEADER, slist);
		//自动解压Header
		curl_easy_setopt(pcurl, CURLOPT_ACCEPT_ENCODING, "gzip");
		//设置代理
		// curl_easy_setopt(pcurl, CURLOPT_PROXY, "127.0.0.1:8888");
		//启动CURL内部的Cookie引擎
		curl_easy_setopt(pcurl, CURLOPT_COOKIEFILE, "");
		//对返回的数据进行操作的函数地址
		pHTTPPack->ClearRec();
		curl_easy_setopt(pcurl, CURLOPT_HEADERDATA, (void *)&pHTTPPack->sstrrecheader);
		curl_easy_setopt(pcurl, CURLOPT_HEADERFUNCTION, write_header_callback);
		curl_easy_setopt(pcurl, CURLOPT_WRITEDATA, (void *)&pHTTPPack->sstrrecdata);
		curl_easy_setopt(pcurl, CURLOPT_WRITEFUNCTION, write_data_callback);
		
		//执行
		res = curl_easy_perform(pcurl);
		if (res != CURLE_OK)
			printf("curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
		
		//转换字符串
		if (flag)
		{
			size_t tmplen;
			tmplen = pHTTPPack->sstrrecheader.size();
			pHTTPPack->strrecheader = new char[tmplen + 1];
			for (i = 0; i < tmplen; i++)
				pHTTPPack->strrecheader[i] = pHTTPPack->sstrrecheader[i];
			pHTTPPack->strrecheader[tmplen] = 0;
			pHTTPPack->i_lenrecheader = tmplen;

			tmplen = pHTTPPack->sstrrecdata.size();
			pHTTPPack->strrecdata = new char[tmplen + 1];
			for (i = 0; i < tmplen; i++)
				pHTTPPack->strrecdata[i] = pHTTPPack->sstrrecdata[i];
			pHTTPPack->strrecdata[tmplen] = 0;
			pHTTPPack->i_lenrecdata = tmplen;
		}

		curl_easy_reset(pcurl);
		/* free the list again */
		if (slist != NULL)
			curl_slist_free_all(slist);
	}

	return res;
}

int toollib::HttpHeadEx(CURL *pcurl, CHTTPPack *pHTTPPack, int flag)
{
	CURLcode res;
	struct curl_slist *slist = NULL;//存放HTTP表头
	size_t i;

	if (pcurl == NULL)
		return -1;
	if (pcurl) {
		//设定URL
		curl_easy_setopt(pcurl, CURLOPT_URL, pHTTPPack->url.c_str());
		//标记为HEAD
		curl_easy_setopt(pcurl, CURLOPT_NOBODY, 1L);
		//不检查证书
		curl_easy_setopt(pcurl, CURLOPT_SSL_VERIFYPEER, FALSE);
		//设置UA
		curl_easy_setopt(pcurl, CURLOPT_USERAGENT, pHTTPPack->useragent);
		//自定义Header内容
		for (i = 0; i<pHTTPPack->i_numsendheader; i++)
			slist = curl_slist_append(slist, pHTTPPack->strsendheader[i].c_str());
		//将自定义Header添加到curl包
		if (slist != NULL)
			curl_easy_setopt(pcurl, CURLOPT_HTTPHEADER, slist);
		//自动解压Header
		curl_easy_setopt(pcurl, CURLOPT_ACCEPT_ENCODING, "gzip");
		//设置代理
		//curl_easy_setopt(pcurl, CURLOPT_PROXY, "127.0.0.1:8888");
		//执行
		res = curl_easy_perform(pcurl);
		if (res != CURLE_OK)
			printf("curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

		curl_easy_reset(pcurl);
		/* free the list again */
		if (slist != NULL)
			curl_slist_free_all(slist);
	}

	return res;
}
