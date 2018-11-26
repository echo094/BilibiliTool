#pragma once

namespace protobl {
	int CheckMessage(const unsigned char *str);
	int MakeFlashConnectionInfo(unsigned char* str, int len, int room);
	int MakeFlashHeartInfo(unsigned char* str, int len, int room);
	int MakeWebConnectionInfo(unsigned char* str, int len, int room);
	int MakeWebHeartInfo(unsigned char* str, int len);
}
