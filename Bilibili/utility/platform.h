#pragma once
#include <string>


#ifdef WIN32
#else
#define _atoi64(val)     strtoll(val, NULL, 10)
#define Sleep(x)     sleep(x)
#endif

// 获取当前目录
void GetDir(char *path, unsigned len);

// 隐藏密码
int GetPassword(std::string &psd);
