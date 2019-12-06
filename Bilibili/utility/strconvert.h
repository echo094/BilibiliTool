#pragma once
#ifndef _TOOLLIB_STRCONVERT_
#define _TOOLLIB_STRCONVERT_

#include <string>
#include <vector>

namespace toollib {

	// 获取当前时间
	long long GetTimeStamp();
	// 生成毫秒时间
	long long GetTimeStampM();
	std::string GetTimeString();

	/**
	 * @brief URL编码 W3C标准
	 *
	 * 更新日期 12/06/2019 <br>
	 *
	 * @param str      原字符串
	 *
	 * @return
	 *   返回编码后的字符串
	 */
	std::string UrlEncode(const std::string& str);

	/**
	 * @brief URL编码 RFC 2396标准
	 *
	 * 更新日期 12/06/2019 <br>
	 *
	 * @param str      原字符串
	 *
	 * @return
	 *   返回编码后的字符串
	 */
	std::string UrlEncodeAnd(const std::string& str);
	bool UTF8ToUTF16(const std::string &in, std::wstring &out);
	std::wstring UTF8ToUTF16(const std::string &in);
	bool UTF16ToUTF8(const std::wstring &in, std::string &out);

}

#endif
