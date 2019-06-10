#include "sslex.h"
#include "base64.h"
using namespace toollib;

// 3DES密钥长度
#define KEY_SIZE        24

bool toollib::Encrypt_RSA_KeyBuff(char *pPubkey, const std::string &strData, std::string &strres)
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

bool toollib::Decrypt_RSA_KeyBuff(const char *pPrikey, const std::string &strData, std::string &strres)
{
	int ret;
	unsigned int len;
	strres = "";
	//调用BASE64解码
	len = strData.length();
	std::string buff;
	ret = Decode_Base64(strData, buff);
	if (ret == false) {
		strres = "Base64 decode failed.";
		return false;
	}
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
	ret = RSA_private_decrypt(buff.length(), (const unsigned char*)buff.c_str(),
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
	hPriKeyFile = fopen(filename, "rb");
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
