
#ifndef _TOOLLIB_BASE64_
#define _TOOLLIB_BASE64_

#include <string>

namespace toollib {

	bool Encode_Base64(const unsigned char *pIn, unsigned long uInLen, std::string& strOut);
	bool Decode_Base64(const std::string& strIn, std::string& strOut);

}

#endif 
 
 