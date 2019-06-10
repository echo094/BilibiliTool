#pragma once
#ifndef _TOOLLIB_SSLEX_
#define _TOOLLIB_SSLEX_

#include <string>

#include <openssl/rsa.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/evp.h>

namespace toollib {

	// RSA加密
	bool Encrypt_RSA_KeyBuff(char *pPubkey, const std::string &strData, std::string &strres);
	// RSA解密
	bool Decrypt_RSA_KeyBuff(const char *pPubkey, const std::string &strData, std::string &strres);
	// RSA解密 密钥从文件读取
	bool Decrypt_RSA_KeyFile(const char *filename, std::string&, std::string& strres);
}

#endif