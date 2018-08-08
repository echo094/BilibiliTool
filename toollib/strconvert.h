#pragma once
#ifndef _TOOLLIB_STRCONVERT_
#define _TOOLLIB_STRCONVERT_

#define WIN32_LEAN_AND_MEAN
#include <windows.h> 
#include <string>
#include <vector>

namespace toollib {

	class CTools {
	public:
		//获取当前时间
		__int64 GetTimeStamp() const;
		std::string GetTimeString() const;
		//查找并截取字符串
		std::wstring findText(const std::wstring origin, const std::wstring find1, const std::wstring find2);
		//字符串分割函数
		std::vector<std::string> split(std::string str, std::string pattern);
		//文本读取
		std::wstring readFileWstring(const char* filename);
		std::string readFileString(const char* filename);
	};


	class CStrConvert
	{
	public:
		//String2Wstring
		std::wstring StringToWstring(const std::string str);
		std::string WstringToString(const std::wstring str);
		//UTF-8与GBK编码互转
		int GBKToUTF8(unsigned char * lpGBKStr, unsigned char * lpUTF8Str, int nUTF8StrLen);
		int UTF8ToGBK(unsigned char * lpUTF8Str, unsigned char * lpGBKStr, int nGBKStrLen);
		void UTF_8ToGB2312(std::string &pOut, char *pText, int pLen);//utf_8转为gb2312
		void GB2312ToUTF_8(std::string& pOut, const char *pText, int pLen) const; //gb2312 转utf_8
		std::string UrlGB2312(char * str);                           //urlgb2312编码
		std::string UrlUTF8(const char * str) const;                 //urlutf8 编码
		std::string UrlUTF8Decode(std::string str);                  //urlutf8解码
		std::string UrlGB2312Decode(std::string str);                //urlgb2312解码
		std::wstring UTF_8ToWString(const char* szU8);
		std::string UTF_8ToString(const char* szU8) const;
		std::wstring UTF_16ToWString(const char * str);
		std::string UTF_16ToString(const char * str);
		std::string UrlEncode(const std::string& str);
	private:
		char CharToInt(char ch);
		char StrToBin(char *str);
		void Gb2312ToUnicode(WCHAR* pOut, const char *gbBuffer) const;
		void UTF_8ToUnicode(WCHAR* pOut, char *pText);
		void UnicodeToUTF_8(char* pOut, WCHAR* pText) const;
		void UnicodeToGB2312(char* pOut, WCHAR uData);

	};

}

#endif
