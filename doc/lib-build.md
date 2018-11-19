# 开源库说明

### boost

版本 1.67.0

[主页](https://www.boost.org/)  



### curl(libcurl)  

版本 7.62.0

[主页](https://curl.haxx.se/libcurl/)

设置见 winbuild/BUILD.WINDOWS.txt

**注意事项**  

- Zlib

  需要以C风格导出

  即在编译时需要去除 ZLIB_WINAPI 宏
  VS导出符号查看命令为 dumpbin /LINKERMEMBER [].lib  

- Openssl

**编译说明**

RTLIBCFG决定CRT为MD或MT 文件中不建议使用MT编译 但最好与包含的附带库的编译选项相一致。

若使用编译为static 在使用时需要添加CURL_STATICLIB预编译宏。

编译指令示例：

```bash
nmake /f Makefile.vc VC=15 mode=dll WITH_SSL=dll WITH_ZLIB=static DEBUG=no RTLIBCFG=static
```

生成的文件在builds文件夹下。



### Openssl

版本 1.1.1

[主页](https://www.openssl.org/)

参考文件**INSTALL**以及**NOTES.WIN**。编译需要使用perl以及NASM，perl建议使用ActiveState，NASM的话32位64位都可。

使用以下语句编译并安装（32位版）：

```bash
perl Configure VC-WIN32
nmake
nmake test
nmake install
```

会将文件安装到默认路径：C:\Program Files (x86)\OpenSSL

这样得到的是使用VC运行时库的DLL。



### RapidJSON

版本 1.1.0

[主页](http://rapidjson.org/)  



### Websocketpp

版本 0.8.1

[主页](https://www.zaphoyd.com/websocketpp)  



### Zlib

版本 1.2.11

[主页](http://www.zlib.net/)

**编译说明**

打开contrib\vstudio目录下的相应工程进行编译。

如果需要以C风格导出需要去除 ZLIB_WINAPI 预定义宏。

Release版本使用ReleaseWithoutAsm配置项编译。  

