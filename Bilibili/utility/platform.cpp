#include "platform.h"
#include <iostream>
#ifdef WIN32
#include <windows.h>
#include <conio.h>
#else
#include <unistd.h>
#endif

void GetDir(char *path, unsigned len) {
#ifdef WIN32
	GetCurrentDirectoryA(len, path);
#else
	getcwd(path, len);
#endif
}

#ifdef WIN32
int GetPassword(std::string &psd) {
	std::cin >> psd;
	return 0;
}
#else
int GetPassword(std::string &psd) {
	system("stty -echo");
	std::cin >> psd;
	system("stty echo");
	return 0;
}
#endif
