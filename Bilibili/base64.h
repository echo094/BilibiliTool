
#ifndef _TOOLLIB_BASE64_
#define _TOOLLIB_BASE64_

#include <string>

namespace toollib {

	bool Encode_Base64(const unsigned char *pIn, unsigned long uInLen, std::string& strOut);
	bool Encode_Base64(const unsigned char *pIn, unsigned long uInLen, unsigned char *pOut, unsigned int *uOutLen);
	bool Decode_Base64(const std::string& strIn, unsigned char *pOut, unsigned int *uOutLen);
	bool Decode_Base64(const unsigned char *pIn, unsigned long uInLen, unsigned char *pOut, unsigned int *uOutLen);

}

#endif 
 
 