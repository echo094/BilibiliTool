#include "strconvert.h"
#include <ctime>
#include "rapidjson/encodings.h"
#include "rapidjson/stringbuffer.h"

unsigned char ToHex(unsigned char x)
{
	return  x > 9 ? x + 55 : x + 48;
}

namespace toollib {

	long long GetTimeStamp() {
		time_t time;
		std::time(&time);
		return time;
	}

	std::string GetTimeString() {
		time_t time;
		std::tm* tm;
		std::time(&time);
		tm = localtime(&time);
		std::string str;
		char buff[25];
		sprintf(buff, "[%04d%02d%02d-%02d-%02d-%02d]", 1900 + tm->tm_year, 1 + tm->tm_mon, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
		str = buff;
		return str;
	}

	std::string UrlEncode(const std::string& str)
	{
		std::string strTemp = "";
		size_t length = str.length();
		for (size_t i = 0; i < length; i++)
		{
			if (isalnum((unsigned char)str[i]) ||
				(str[i] == '-') ||
				(str[i] == '_') ||
				(str[i] == '.') ||
				(str[i] == '~')) {
				strTemp += str[i];
			}
			else if (isspace((unsigned char)str[i])) {
				strTemp += "+";
			}
			else {
				strTemp += '%';
				strTemp += ToHex((unsigned char)str[i] >> 4);
				strTemp += ToHex((unsigned char)str[i] % 16);
			}
		}
		return strTemp;
	}

	bool UTF8ToUTF16(const std::string &in, std::wstring &out) {
		using namespace rapidjson;
		StringStream source(in.c_str());
		GenericStringBuffer<UTF16<> > target;
		bool hasError = false;
		while (source.Peek() != '\0') {
			if (!Transcoder<UTF8<>, UTF16<> >::Transcode(source, target)) {
				hasError = true;
				break;
			}
		}
		if (!hasError) {
			out = target.GetString();
			return true;
		}
		return false;
	}

	std::wstring toollib::UTF8ToUTF16(const std::string & in) {
		std::wstring out;
		UTF8ToUTF16(in, out);
		return out;
	}

	bool UTF16ToUTF8(const std::wstring &in, std::string &out) {
		using namespace rapidjson;
		GenericStringStream<UTF16<> > source(in.c_str());
		GenericStringBuffer<UTF8<> > target;
		bool hasError = false;
		while (source.Peek() != '\0') {
			if (!Transcoder<UTF16<>, UTF8<> >::Transcode(source, target)) {
				hasError = true;
				break;
			}
		}
		if (!hasError) {
			out = target.GetString();
			return true;
		}
		return false;
	}

}
