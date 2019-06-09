#include "stdafx.h"
#include "base64.h"
#include <sstream>

static const char *g_pCodes =
 "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";
static const unsigned char g_pMap[256] =
{
 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
 255, 255, 255, 255, 255, 255, 255,  62, 255, 255, 255,  63,
  52,  53,  54,  55,  56,  57,  58,  59,  60,  61, 255, 255,
 255, 254, 255, 255, 255,   0,   1,   2,   3,   4,   5,   6,
   7,   8,   9,  10,  11,  12,  13,  14,  15,  16,  17,  18,
  19,  20,  21,  22,  23,  24,  25, 255, 255, 255, 255, 255,
 255,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,
  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,  48,
  49,  50,  51, 255, 255, 255, 255, 255, 255, 255, 255, 255,
 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
 255, 255, 255, 255
};

bool toollib::Encode_Base64(const unsigned char *pIn, unsigned long uInLen, std::string& strOut)
{
	unsigned long i, len2, leven;
	len2 = ((uInLen + 2) / 3) << 2;
	leven = 3 * (uInLen / 3);
	strOut = "";
	strOut.reserve(len2 + 1);
	for (i = 0; i < leven; i += 3)
	{
		strOut += g_pCodes[pIn[0] >> 2];
		strOut += g_pCodes[((pIn[0] & 3) << 4) + (pIn[1] >> 4)];
		strOut += g_pCodes[((pIn[1] & 0xf) << 2) + (pIn[2] >> 6)];
		strOut += g_pCodes[pIn[2] & 0x3f];
		pIn += 3;
	}
	if (i < uInLen)
	{
		unsigned char a = pIn[0];
		unsigned char b = ((i + 1) < uInLen) ? pIn[1] : 0;
		unsigned char c = 0;
		strOut += g_pCodes[a >> 2];
		strOut += g_pCodes[((a & 3) << 4) + (b >> 4)];
		strOut += ((i + 1) < uInLen) ? g_pCodes[((b & 0xf) << 2) + (c >> 6)] : '=';
		strOut += '=';
	}
	return true;
}

bool toollib::Decode_Base64(const std::string& strIn, std::string& strOut)
{
	unsigned long t, x, y, z;
	unsigned char c;
	unsigned long g = 3;
	unsigned long uInLen = strIn.length();
	strOut = "";
	strOut.reserve(uInLen);
	for (x = y = z = t = 0; x < uInLen; x++)
	{
		c = g_pMap[(unsigned char)strIn[x]];
		if (c == 255) continue;
		if (c == 254) { c = 0; g--; }
		t = (t << 6) | c;
		if (++y == 4)
		{
			strOut += (unsigned char)((t >> 16) & 255);
			if (g > 1) strOut += (unsigned char)((t >> 8) & 255);
			if (g > 2) strOut += (unsigned char)(t & 255);
			y = t = 0;
		}
	}
	return true;
}
