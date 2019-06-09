#pragma once
#ifndef _TOOLLIB_STRCONVERT_
#define _TOOLLIB_STRCONVERT_

#include <string>
#include <vector>

namespace toollib {

	//获取当前时间
	long long GetTimeStamp();
	std::string GetTimeString();

	class CStrConvert
	{
	public:
		static std::string UrlEncode(const std::string& str);
		static bool UTF8ToUTF16(const std::string &in, std::wstring &out);
		static std::wstring UTF8ToUTF16(const std::string &in);
		static bool UTF16ToUTF8(const std::wstring &in, std::string &out);
	};

}

#endif
