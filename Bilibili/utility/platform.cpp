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
	using namespace std;

	int ret = 0;
	char ch;
	unsigned int ich;
	psd = "";

	while (1) {
		ich = _getch();
		if (!ich) {
			continue;
		}
		//case cursor move
		if (ich == 224) {
			ch = _getch();
			continue;
		}
		ch = ich;
		//case enter
		if (ch == 13) {
			if (psd.size() > 0) {
				cout << '\n';
				return 0;
			}
			cout << "\nPassword is empty. Please reenter. \n";
			continue;
		}
		//case backspace
		if (ch == 8)
		{
			if (psd.size() == 0)
				continue;
			psd.erase(psd.end() - 1);
			cout << "\b \b";
			continue;
		}
		//noral case
		psd += ch;
		cout << "*";
	}
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
