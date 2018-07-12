#pragma once
#ifndef _TOOLLIB_SSLEX_
#define _TOOLLIB_SSLEX_

#include <openssl/rsa.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/evp.h>
#pragma comment(lib,"libeay32.lib")

#include <string>

namespace toollib {

	// RSA加密
	bool Encrypt_RSA_KeyBuff(char *pPubkey, std::string &strData, std::string &strres);
	// RSA解密
	bool Decrypt_RSA_KeyBuff(char *pPubkey, std::string &strData, std::string &strres);
	// RSA解密 密钥从文件读取
	bool Decrypt_RSA_KeyFile(const char *filename, std::string&, std::string& strres);
	// 3DES_CBC加密
	int Encrypt_3DES(const char *key, const unsigned char * szInput, int nInLen, unsigned char *szOutput);
	// 3DES_CBC解密
	int Decrypt_3DES(const char *key, const unsigned char * szInput, int nInLen, unsigned char *szOutput);
	// 3DES_CBC加密 并编码为BASE64字符串
	int Encrypt_3DES_BASE64(std::string &key, std::string &strData, std::string &strres);
	// 对BASE64编码字符串 进行3DES_CBC解密
	int Decrypt_3DES_BASE64(std::string &key, std::string &strData, std::string &strres);

}

#endif