#include "stdafx.h"
#include "strconvert.h"
using namespace toollib;
#include <locale.h>
#include <codecvt>
#include <fstream>
#include <sstream>
using namespace std;


namespace toollib {

	long long GetTimeStamp() {
		time_t time;
		std::time(&time);
		return time;
	}

	std::string GetTimeString() {
		time_t time;
		std::tm tm;
		std::time(&time);
		localtime_s(&tm, &time);
		std::string str;
		char buff[25];
		sprintf_s(buff, sizeof(buff), "[%04d%02d%02d-%02d-%02d-%02d]", 1900 + tm.tm_year, 1 + tm.tm_mon, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
		str = buff;
		return str;
	}

}


// 需包含locale、string头文件、使用setlocale函数。
/*http://www.cnblogs.com/SunboyL/archive/2013/03/31/stringandwstring.html*/
wstring CStrConvert::StringToWstring(const string str)
{// string转wstring
	size_t convertedChars = 0;
	std::string curLocale = setlocale(LC_ALL, NULL);   //curLocale="C"
	setlocale(LC_ALL, "chs");
	//setlocale(LC_CTYPE, "");     //必须调用此函数
	size_t charNum = strlen(str.c_str()) + 1;
	wchar_t* dest = new wchar_t[charNum];
	mbstowcs_s(&convertedChars, dest, charNum, str.c_str(), _TRUNCATE);
	//mbstowcs(dest, str.c_str(), charNum);// 转换
	std::wstring result = dest;
	delete[] dest;
	setlocale(LC_ALL, curLocale.c_str());
	return result;
}

string CStrConvert::WstringToString(const wstring str)
{// wstring转string
	size_t len = str.size() * 4;
	setlocale(LC_CTYPE, "");
	char *p = new char[len];
	size_t lenout;
	wcstombs_s(&lenout, p, len, str.c_str(), len);
	//wcstombs(p, str.c_str(), len);
	std::string str1(p);
	delete[] p;
	return str1;
}

//GBK编码转换到UTF8编码
int CStrConvert::GBKToUTF8(unsigned char * lpGBKStr, unsigned char * lpUTF8Str, int nUTF8StrLen)
{
	wchar_t * lpUnicodeStr = NULL;
	int nRetLen = 0;

	if (!lpGBKStr)  //如果GBK字符串为NULL则出错退出
		return 0;

	nRetLen = ::MultiByteToWideChar(CP_ACP, 0, (char *)lpGBKStr, -1, NULL, NULL);  //获取转换到Unicode编码后所需要的字符空间长度
	lpUnicodeStr = new WCHAR[nRetLen + 1];  //为Unicode字符串空间
	nRetLen = ::MultiByteToWideChar(CP_ACP, 0, (char *)lpGBKStr, -1, lpUnicodeStr, nRetLen);  //转换到Unicode编码
	if (!nRetLen)  //转换失败则出错退出
		return 0;

	nRetLen = ::WideCharToMultiByte(CP_UTF8, 0, lpUnicodeStr, -1, NULL, 0, NULL, NULL);  //获取转换到UTF8编码后所需要的字符空间长度

	if (!lpUTF8Str)  //输出缓冲区为空则返回转换后需要的空间大小
	{
		if (lpUnicodeStr)
			delete[]lpUnicodeStr;
		return nRetLen;
	}

	if (nUTF8StrLen < nRetLen)  //如果输出缓冲区长度不够则退出
	{
		if (lpUnicodeStr)
			delete[]lpUnicodeStr;
		return 0;
	}

	nRetLen = ::WideCharToMultiByte(CP_UTF8, 0, lpUnicodeStr, -1, (char *)lpUTF8Str, nUTF8StrLen, NULL, NULL);  //转换到UTF8编码

	if (lpUnicodeStr)
		delete[]lpUnicodeStr;

	return nRetLen;
}

// UTF8编码转换到GBK编码
int CStrConvert::UTF8ToGBK(unsigned char * lpUTF8Str, unsigned char * lpGBKStr, int nGBKStrLen)
{
	wchar_t * lpUnicodeStr = NULL;
	int nRetLen = 0;

	if (!lpUTF8Str)  //如果UTF8字符串为NULL则出错退出
		return 0;

	nRetLen = ::MultiByteToWideChar(CP_UTF8, 0, (char *)lpUTF8Str, -1, NULL, NULL);  //获取转换到Unicode编码后所需要的字符空间长度
	lpUnicodeStr = new WCHAR[nRetLen + 1];  //为Unicode字符串空间
	nRetLen = ::MultiByteToWideChar(CP_UTF8, 0, (char *)lpUTF8Str, -1, lpUnicodeStr, nRetLen);  //转换到Unicode编码
	if (!nRetLen)  //转换失败则出错退出
		return 0;

	nRetLen = ::WideCharToMultiByte(CP_ACP, 0, lpUnicodeStr, -1, NULL, NULL, NULL, NULL);  //获取转换到GBK编码后所需要的字符空间长度

	if (!lpGBKStr)  //输出缓冲区为空则返回转换后需要的空间大小
	{
		if (lpUnicodeStr)
			delete[]lpUnicodeStr;
		return nRetLen;
	}

	if (nGBKStrLen < nRetLen)  //如果输出缓冲区长度不够则退出
	{
		if (lpUnicodeStr)
			delete[]lpUnicodeStr;
		return 0;
	}

	nRetLen = ::WideCharToMultiByte(CP_ACP, 0, lpUnicodeStr, -1, (char *)lpGBKStr, nRetLen, NULL, NULL);  //转换到GBK编码

	if (lpUnicodeStr)
		delete[]lpUnicodeStr;

	return nRetLen;
}


//UTF_8 转gb2312
void CStrConvert::UTF_8ToGB2312(string &pOut, char *pText, int pLen)
{
	char buf[4];
	char* rst = new char[pLen + (pLen >> 2) + 2];
	memset(buf, 0, 4);
	memset(rst, 0, pLen + (pLen >> 2) + 2);

	int i = 0;
	int j = 0;

	while (i < pLen)
	{
		if (*(pText + i) >= 0)
		{

			rst[j++] = pText[i++];
		}
		else
		{
			WCHAR Wtemp;


			UTF_8ToUnicode(&Wtemp, pText + i);

			UnicodeToGB2312(buf, Wtemp);

			unsigned short int tmp = 0;
			tmp = rst[j] = buf[0];
			tmp = rst[j + 1] = buf[1];
			tmp = rst[j + 2] = buf[2];

			//newBuf[j] = Ctemp[0];
			//newBuf[j + 1] = Ctemp[1];

			i += 3;
			j += 2;
		}

	}
	rst[j] = '\0';
	pOut = rst;
	delete[]rst;
}

//GB2312 转为 UTF-8
void CStrConvert::GB2312ToUTF_8(string& pOut, const char *pText, int pLen) const
{
	char buf[4];
	memset(buf, 0, 4);

	pOut.clear();

	int i = 0;
	while (i < pLen)
	{
		//如果是英文直接复制就可以
		if (pText[i] >= 0)
		{
			char asciistr[2] = { 0 };
			asciistr[0] = (pText[i++]);
			pOut.append(asciistr);
		}
		else
		{
			WCHAR pbuffer;
			Gb2312ToUnicode(&pbuffer, pText + i);

			UnicodeToUTF_8(buf, &pbuffer);

			pOut.append(buf);

			i += 2;
		}
	}

	return;
}
//把str编码为网页中的 GB2312 url encode ,英文不变，汉字双字节 如%3D%AE%88
string CStrConvert::UrlGB2312(char * str)
{
	string dd;
	size_t len = strlen(str);
	for (size_t i = 0; i<len; i++)
	{
		if (isalnum((BYTE)str[i]))
		{
			char tempbuff[2];
			sprintf_s(tempbuff, "%c", str[i]);
			dd.append(tempbuff);
		}
		else if (isspace((BYTE)str[i]))
		{
			dd.append("+");
		}
		else
		{
			char tempbuff[4];
			sprintf_s(tempbuff, "%%%X%X", ((BYTE*)str)[i] >> 4, ((BYTE*)str)[i] % 16);
			dd.append(tempbuff);
		}

	}
	return dd;
}

//把str编码为网页中的 UTF-8 url encode ,英文不变，汉字三字节 如%3D%AE%88
string CStrConvert::UrlUTF8(const char * str) const
{
	string tt;
	string dd;
	GB2312ToUTF_8(tt, str, (int)strlen(str));

	size_t len = tt.length();
	for (size_t i = 0; i<len; i++)
	{
		if (isalnum((BYTE)tt.at(i)))
		{
			char tempbuff[2] = { 0 };
			sprintf_s(tempbuff, "%c", (BYTE)tt.at(i));
			dd.append(tempbuff);
		}
		else if (isspace((BYTE)tt.at(i)))
		{
			dd.append("+");
		}
		else
		{
			char tempbuff[4];
			sprintf_s(tempbuff, "%%%X%X", ((BYTE)tt.at(i)) >> 4, ((BYTE)tt.at(i)) % 16);
			dd.append(tempbuff);
		}

	}
	return dd;
}

//把url GB2312解码
string CStrConvert::UrlGB2312Decode(string str)
{
	string output = "";
	char tmp[2];
	int i = 0, idx = 0, len = str.length();

	while (i<len) {
		if (str[i] == '%') {
			tmp[0] = str[i + 1];
			tmp[1] = str[i + 2];
			output += StrToBin(tmp);
			i = i + 3;
		}
		else if (str[i] == '+') {
			output += ' ';
			i++;
		}
		else {
			output += str[i];
			i++;
		}
	}

	return output;
}

//把url utf8解码
string CStrConvert::UrlUTF8Decode(string str)
{
	string output = "";

	string temp = UrlGB2312Decode(str);//

	UTF_8ToGB2312(output, (char *)temp.data(), strlen(temp.data()));

	return output;

}

std::wstring CStrConvert::UTF_8ToWString(const char* szU8)
{
	//预转换，得到所需空间的大小;
	int wcsLen = ::MultiByteToWideChar(CP_UTF8, NULL, szU8, strlen(szU8), NULL, 0);

	//分配空间要给'\0'留个空间，MultiByteToWideChar不会给'\0'空间
	wchar_t* wszString = new wchar_t[wcsLen + 1];

	//转换
	::MultiByteToWideChar(CP_UTF8, NULL, szU8, strlen(szU8), wszString, wcsLen);

	//最后加上'\0'
	wszString[wcsLen] = '\0';

	//CString unicodeString(wszString);
	wstring unicodeString = wszString;

	delete[] wszString;
	wszString = NULL;

	return unicodeString;
}

std::string CStrConvert::UTF_8ToString(const char* szU8) const
{
	//预转换，得到所需空间的大小;
	int nwLen = ::MultiByteToWideChar(CP_UTF8, NULL, szU8, strlen(szU8), NULL, 0);
	//一定要加1，不然会出现尾巴 
	wchar_t * pwBuf = new wchar_t[nwLen + 1];
	memset(pwBuf, 0, nwLen * 2 + 2);
	//转换
	::MultiByteToWideChar(CP_UTF8, NULL, szU8, strlen(szU8), pwBuf, nwLen);
	//预转换，得到所需空间的大小;
	int nLen = WideCharToMultiByte(CP_ACP, 0, pwBuf, -1, NULL, NULL, NULL, NULL);
	char * pBuf = new char[nLen + 1];
	memset(pBuf, 0, nLen + 1);
	WideCharToMultiByte(CP_ACP, 0, pwBuf, nwLen, pBuf, nLen, NULL, NULL);
	std::string retStr = pBuf;

	delete[]pBuf;
	delete[]pwBuf;
	pBuf = NULL;
	pwBuf = NULL;

	return retStr;
}

std::wstring CStrConvert::UTF_16ToWString(const char * str)
{
	wstring rst;
	bool escape = false;
	int len = strlen(str);
	int intHex;
	char tmp[5];
	memset(tmp, 0, 5);
	for (int i = 0; i < len; i++)
	{
		char c = str[i];
		switch (c)
		{
		case'//':
		case'%':
		case'\\':
			escape = true;
			break;
		case'u':
		case'U':
			if (escape)
			{
				memcpy(tmp, str + i + 1, 4);
				sscanf_s(tmp, "%x", &intHex); //把16进制字符转换为数字
				rst.push_back(intHex);
				i += 4;
				escape = false;
			}
			else {
				rst.push_back(c);
			}
			break;
		default:
			rst.push_back(c);
			break;
		}
	}
	return rst;
}

std::string CStrConvert::UTF_16ToString(const char * str)
{
	wstring rstw = UTF_16ToWString(str);
	string rst = WstringToString(rstw);
	return rst;
}

unsigned char ToHex(unsigned char x)
{
	return  x > 9 ? x + 55 : x + 48;
}

std::string CStrConvert::UrlEncode(const std::string& str)
{
	std::string strTemp = "";
	size_t length = str.length();
	for (size_t i = 0; i < length; i++)
	{
		if (isalnum((unsigned char)str[i]) ||
			(str[i] == '-') ||
			(str[i] == '_') ||
			(str[i] == '.') ||
			(str[i] == '~'))
			strTemp += str[i];
		else if (str[i] == ' ')
			strTemp += "+";
		else
		{
			strTemp += '%';
			strTemp += ToHex((unsigned char)str[i] >> 4);
			strTemp += ToHex((unsigned char)str[i] % 16);
		}
	}
	return strTemp;
}

//做为解Url使用
char CStrConvert::CharToInt(char ch) {
	if (ch >= '0' && ch <= '9')return (char)(ch - '0');
	if (ch >= 'a' && ch <= 'f')return (char)(ch - 'a' + 10);
	if (ch >= 'A' && ch <= 'F')return (char)(ch - 'A' + 10);
	return -1;
}

char CStrConvert::StrToBin(char *str) {
	char tempWord[2];
	char chn;

	tempWord[0] = CharToInt(str[0]);                         //make the B to 11 -- 00001011
	tempWord[1] = CharToInt(str[1]);                         //make the 0 to 0 -- 00000000

	chn = (tempWord[0] << 4) | tempWord[1];                //to change the BO to 10110000

	return chn;
}

void CStrConvert::Gb2312ToUnicode(WCHAR* pOut, const char *gbBuffer) const
{
	::MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, gbBuffer, 2, pOut, 1);
	return;
}

void CStrConvert::UTF_8ToUnicode(WCHAR* pOut, char *pText)
{
	char* uchar = (char *)pOut;

	uchar[1] = ((pText[0] & 0x0F) << 4) + ((pText[1] >> 2) & 0x0F);
	uchar[0] = ((pText[1] & 0x03) << 6) + (pText[2] & 0x3F);

	return;
}

void CStrConvert::UnicodeToUTF_8(char* pOut, WCHAR* pText) const
{
	// 注意 WCHAR高低字的顺序,低字节在前，高字节在后
	char* pchar = (char *)pText;

	pOut[0] = (0xE0 | ((pchar[1] & 0xF0) >> 4));
	pOut[1] = (0x80 | ((pchar[1] & 0x0F) << 2)) + ((pchar[0] & 0xC0) >> 6);
	pOut[2] = (0x80 | (pchar[0] & 0x3F));

	return;
}

void CStrConvert::UnicodeToGB2312(char* pOut, WCHAR uData)
{
	WideCharToMultiByte(CP_ACP, NULL, &uData, 1, pOut, sizeof(WCHAR), NULL, NULL);
	return;
}
