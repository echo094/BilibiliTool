#pragma once
#ifndef _TOOLLIB_STRCONVERT_
#define _TOOLLIB_STRCONVERT_

#include <string>
#include <vector>

namespace toollib {

	//获取当前时间
	long long GetTimeStamp();
	std::string GetTimeString();

	std::string UrlEncode(const std::string& str);
	bool UTF8ToUTF16(const std::string &in, std::wstring &out);
	std::wstring UTF8ToUTF16(const std::string &in);
	bool UTF16ToUTF8(const std::wstring &in, std::string &out);

}

#endif
