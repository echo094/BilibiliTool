﻿#include "proto_bl.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <boost/shared_array.hpp>

const int DM_FLASH_PROTO = 2;
const char DM_FLASH_VER[] = "2.4.6-9e02b4f1";
const int DM_WEB_PROTO = 2;
const char DM_WEB_VER[] = "1.8.11";

long long GetRUID() {
	srand(unsigned(time(0)));
	double val = 100000000000000.0 + 200000000000000.0*(rand() / (RAND_MAX + 1.0));
	return static_cast <long long> (val);
}

unsigned protobl::CheckMessage(const unsigned char *str) {
	int i;
	if (str[4]) {
		return 0;
	}
	if (str[5] - 16) {
		return 0;
	}
	for (i = 8; i < 11; i++) {
		if (str[i])
			return 0;
	}
	for (i = 12; i < 15; i++) {
		if (str[i])
			return 0;
	}
	return str[11];
}

int protobl::MakeFlashConnectionInfo(unsigned char* str, int len, unsigned room, const char *key) {
	memset(str, 0, len);
	int buflen;
	buflen = sprintf((char*)str + 16, "{\"roomid\":%d,\"platform\":\"%s\",\
\"key\":\"%s\",\"clientver\":\"%s\",\"uid\":%lld,\"protover\":%d}",
room, "flash", key, DM_FLASH_VER, GetRUID(), DM_FLASH_PROTO);
	if (buflen == -1) {
		return -1;
	}
	buflen = 16 + buflen;
	str[3] = buflen;
	str[5] = 0x10;
	str[7] = 0x01;
	str[11] = 0x07;
	str[15] = 0x01;
	return buflen;
}

int protobl::MakeFlashHeartInfo(unsigned char* str, int len, int room) {
	memset(str, 0, len);
	str[3] = 0x10;
	str[5] = 0x10;
	str[7] = 0x01;
	str[11] = 0x02;
	str[15] = 0x01;
	return 16;
}

int protobl::MakeWebConnectionInfo(unsigned char* str, int len, unsigned room, const char *key) {
	memset(str, 0, len);
	int buflen;
	//构造发送的字符串
	buflen = sprintf((char*)str + 16, "{\"uid\":0,\"roomid\":%d,\
\"protover\":%d,\"platform\":\"%s\",\"clientver\":\"%s\",\
\"type\":%d,\"key\":\"%s\"}",
room, DM_WEB_PROTO, "web", DM_WEB_VER, 2, key);
	if (buflen == -1) {
		return -1;
	}
	buflen = 16 + buflen;
	str[3] = buflen;
	str[5] = 0x10;
	str[7] = 0x01;
	str[11] = 0x07;
	str[15] = 0x01;
	return buflen;
}

int protobl::MakeWebHeartInfo(unsigned char* str, int len) {
	memset(str, 0, len);
	int buflen;
	//构造发送的字符串
	buflen = sprintf((char*)str + 16, "[object Object]");
	if (buflen == -1) {
		return -1;
	}
	buflen = 16 + buflen;
	str[3] = buflen;
	str[5] = 0x10;
	str[7] = 0x01;
	str[11] = 0x02;
	str[15] = 0x01;
	return buflen;
}
