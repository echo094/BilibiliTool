#pragma once

namespace protobl {
	unsigned CheckMessage(const unsigned char *str);
	int MakeFlashConnectionInfo(unsigned char* str, int len, unsigned room, const char *key);
	int MakeFlashHeartInfo(unsigned char* str, int len, int room);
	int MakeWebConnectionInfo(unsigned char* str, int len, unsigned room, const char *key);
	int MakeWebHeartInfo(unsigned char* str, int len);
}
