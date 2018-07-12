#include "stdafx.h"
#include "sslex.h"
#include "base64.h"
using namespace toollib;

// 3DES密钥长度
#define KEY_SIZE        24

bool toollib::Encrypt_RSA_KeyBuff(char *pPubkey, std::string &strData, std::string &strres)
{
	strres = "";
	BIO *bio = NULL;
	bio = BIO_new_mem_buf(pPubkey, -1);
	if (bio == NULL)
		return false;
	RSA* pRSAPublicKey;
	pRSAPublicKey = PEM_read_bio_RSA_PUBKEY(bio, NULL, NULL, NULL);
	if (pRSAPublicKey == NULL)
		return false;

	int nLen = RSA_size(pRSAPublicKey);
	char* pEncode = new char[nLen + 1];
	int ret = RSA_public_encrypt(strData.length(), (const unsigned char*)strData.c_str(),
		(unsigned char*)pEncode, pRSAPublicKey, RSA_PKCS1_PADDING);
	if (ret >= 0)
	{
		Encode_Base64((unsigned char *)pEncode, ret, strres);
		ret = strres.size();
	}
	else
		return false;

	delete[] pEncode;
	RSA_free(pRSAPublicKey);
	CRYPTO_cleanup_all_ex_data();
	return true;
}

bool toollib::Decrypt_RSA_KeyBuff(char *pPrikey, std::string &strData, std::string &strres)
{
	int ret;
	unsigned int len;
	strres = "";
	//调用BASE64解码
	len = strData.length();
	char * dstr = new char[len];
	ret = Decode_Base64(strData, (unsigned char *)dstr, &len);
	if (ret == 0) {
		delete[] dstr;
		strres = "Base64 decode failed.";
		return false;
	}
	strData = std::string((char*)dstr, len);
	delete[] dstr;
	//RSA解密
	BIO *bio = NULL;
	bio = BIO_new_mem_buf(pPrikey, -1);
	if (bio == NULL)
		return false;
	RSA* pRSAPriKey = NULL;
	pRSAPriKey = PEM_read_bio_RSAPrivateKey(bio, NULL, NULL, NULL);
	if (pRSAPriKey == NULL)
		return false;

	int nLen = RSA_size(pRSAPriKey);
	char* pDecode = new char[nLen + 1];
	ret = RSA_private_decrypt(strData.length(), (const unsigned char*)strData.c_str(),
		(unsigned char*)pDecode, pRSAPriKey, RSA_PKCS1_PADDING);
	if (ret >= 0)
	{
		strres = std::string((char*)pDecode, ret);
		ret = strres.size();
	}
	else
		return false;

	delete[] pDecode;
	RSA_free(pRSAPriKey);
	CRYPTO_cleanup_all_ex_data();
	return true;
}

bool toollib::Decrypt_RSA_KeyFile(const char *filename, std::string& strData, std::string& strres)
{
	if (strData.empty())
		return false;
	FILE* hPriKeyFile;
	fopen_s(&hPriKeyFile, filename, "rb");
	if (hPriKeyFile == NULL)
		return false;
	RSA* pRSAPriKey = RSA_new();
	if (PEM_read_RSAPrivateKey(hPriKeyFile, &pRSAPriKey, 0, 0) == NULL)
		return false;

	int nLen = RSA_size(pRSAPriKey);
	char* pDecode = new char[nLen + 1];
	int ret = RSA_private_decrypt(strData.length(), (const unsigned char*)strData.c_str(), (unsigned char*)pDecode, pRSAPriKey, RSA_PKCS1_PADDING);
	if (ret >= 0)
	{
		strres = std::string((char*)pDecode, ret);
	}
	else
		return false;

	delete[] pDecode;
	RSA_free(pRSAPriKey);
	fclose(hPriKeyFile);
	CRYPTO_cleanup_all_ex_data();
	return true;
}

int toollib::Encrypt_3DES(const char *key, const unsigned char * szInput, int nInLen, unsigned char *szOutput)
{
	int iOutLen = 0;
	int iTmpLen = 0;
	char iv[KEY_SIZE] = { 0 };
	EVP_CIPHER_CTX ctx;        //初始化,用到什么加密方式由EVP_des_ede3_ecb()决定的，如果改为其他加密方式，只要改这个就可以了。    
	EVP_CIPHER_CTX_init(&ctx);
	EVP_EncryptInit_ex(&ctx, EVP_des_ede3_cbc(), NULL, (const unsigned char *)key, (const unsigned char *)iv);        //加密    
	if (!EVP_EncryptUpdate(&ctx, szOutput, &iOutLen, szInput, nInLen))
	{
		EVP_CIPHER_CTX_cleanup(&ctx);
		return -1;
	}   //结束加密    
	if (!EVP_EncryptFinal_ex(&ctx, (unsigned char *)(szOutput + iOutLen), &iTmpLen))
	{
		EVP_CIPHER_CTX_cleanup(&ctx);
		return -1;
	}
	iOutLen += iTmpLen;
	EVP_CIPHER_CTX_cleanup(&ctx);
	return iOutLen;
}

int toollib::Decrypt_3DES(const char *key, const unsigned char * szInput, int nInLen, unsigned char *szOutput)
{
	int iOutLen = 0;
	int iTmpLen = 0;
	char iv[KEY_SIZE] = { 0 };
	EVP_CIPHER_CTX ctx;        //初始化,用到什么加密方式由EVP_des_ede3_ecb()决定的，如果改为其他加密方式，只要改这个就可以了。    
	EVP_CIPHER_CTX_init(&ctx);
	EVP_DecryptInit_ex(&ctx, EVP_des_ede3_cbc(), NULL, (const unsigned char *)key, (const unsigned char *)iv);        //加密    
	if (!EVP_DecryptUpdate(&ctx, szOutput, &iOutLen, szInput, nInLen))
	{
		EVP_CIPHER_CTX_cleanup(&ctx);
		return -1;
	}   //结束解密    
	if (!EVP_DecryptFinal_ex(&ctx, (unsigned char *)(szOutput + iOutLen), &iTmpLen))
	{
		EVP_CIPHER_CTX_cleanup(&ctx);
		return -1;
	}
	iOutLen += iTmpLen;
	EVP_CIPHER_CTX_cleanup(&ctx);
	return iOutLen;
}

int toollib::Encrypt_3DES_BASE64(std::string &key, std::string &strData, std::string &strres)
{
	int ret = 0, len;
	len = strData.length();
	char *str = new char[len * 3];
	ret = Encrypt_3DES(key.c_str(), (unsigned char *)strData.c_str(), len, (unsigned char *)str);
	if (ret == -1) {
		strres = "3DES encrypt failed.";
		delete[] str;
		return -1;
	}
	len = ret;
	ret = Encode_Base64((unsigned char *)str, len, strres);
	if (!ret) {
		strres = "Base64 encode failed.";
		delete[] str;
		return -1;
	}
	ret = strres.size();

	delete[] str;
	return ret;
}

int toollib::Decrypt_3DES_BASE64(std::string &key, std::string &strData, std::string &strres)
{
	int ret = 0;
	unsigned int len;
	len = strData.length();
	char *str = new char[len];

	ret = Decode_Base64(strData, (unsigned char *)str, &len);
	if (!ret) {
		strres = "Base64 decode failed.";
		delete[] str;
		return -1;
	}
	char *strdesout = new char[len + 1];
	ret = Decrypt_3DES(key.c_str(), (unsigned char *)str, len, (unsigned char *)strdesout);
	if (ret == -1) {
		strres = "3DES decrypt failed.";
		delete[] str;
		delete[] strdesout;
		return -1;
	}
	strres = std::string((char*)strdesout, ret);

	delete[] str;
	delete[] strdesout;
	return ret;
}
