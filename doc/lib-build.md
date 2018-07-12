# 开源库说明
### boost
版本 1.67.0  
[主页](https://www.boost.org/)  

### curl(libcurl)  
版本 7.60.0  
[主页](https://curl.haxx.se/libcurl/)
设置见 winbuild/BUILD.WINDOWS.txt  
**注意事项**  
* Zlib  
  需要以C风格导出  
  即在编译时需要去除 ZLIB_WINAPI 宏
  VS导出符号查看命令为 dumpbin /LINKERMEMBER [].lib  
* Openssl  
  如果为1.1.x的版本，需要将生成的libssl.lib重命名为libeay32.lib  

**编译说明**  
RTLIBCFG决定CRT为MD或MT 文件中不建议使用MT编译 但最好与包含的附带库的编译选项相一致  
若使用编译为static 在使用时需要添加CURL_STATICLIB预编译宏    
编译指令示例  
nmake /f Makefile.vc VC=15 mode=dll WITH_SSL=static WITH_ZLIB=static DEBUG=no RTLIBCFG=static  
nmake /f Makefile.vc VC=15 mode=static WITH_SSL=static WITH_ZLIB=static DEBUG=yes  
生成的文件在builds文件夹下  

### Openssl
版本 1.0.2o  
[主页](https://www.openssl.org/)  
编译需要使用 perl 以及 nasm (32位64位都可)  
编译选项参考 INSTALL.W32 文件  
编译命令参考  
1. 配置环境  
  perl Configure VC-WIN32 --prefix=win32-release --openssldir=win32-release\ssl  
  perl Configure debug-VC-WIN32 --prefix=win32-debug --openssldir=win32-debug\ssl  
2. 处理汇编码  
  ms\do_nasm  
3. 编译测试与安装  
  **nt 生成lib 使用MT选项**  
  **ntdll 生成dll 使用MD选项**  
  nmake -f ms\nt.mak  
  nmake -f ms\nt.mak test  
  nmake -f ms\nt.mak install  

### RapidJSON
版本 1.1.0  
[主页](http://rapidjson.org/)  

### Websocketpp
版本 0.7.0  
[主页](https://www.zaphoyd.com/websocketpp)  

### Zlib
版本 1.2.11  
[主页](http://www.zlib.net/)  
**编译说明**  
打开contrib\vstudio目录下的相应工程进行编译。  
如果需要以C风格导出需要去除 ZLIB_WINAPI 预定义宏。  
Release版本使用ReleaseWithoutAsm配置项编译。  
