# 开源库说明

### boost

版本 1.69.0

[主页](https://www.boost.org/)  

编译方法：

```shell
bootstrap
.\b2
```



### curl(libcurl)  

版本 7.65.1

[主页](https://curl.haxx.se/libcurl/)

设置见`winbuild/BUILD.WINDOWS.txt`

**注意事项**  

- Zlib

  需要以C风格导出

  VS导出符号查看命令为 `dumpbin /LINKERMEMBER zlib.lib  `

- Openssl

**编译说明**

RTLIBCFG决定CRT为MD或MT 文件中不建议使用MT编译 但最好与包含的附带库的编译选项相一致。

若使用编译为static 在使用时需要添加CURL_STATICLIB预编译宏。

编译指令示例：

```bash
# 生成DLL
nmake /f Makefile.vc VC=15 mode=dll WITH_SSL=dll WITH_ZLIB=dll
# 生成静态库
nmake /f Makefile.vc VC=15 mode=static WITH_SSL=static WITH_ZLIB=static DEBUG=no RTLIBCFG=static
```

生成的文件在builds文件夹下。



### Openssl

版本 1.1.1c

[主页](https://www.openssl.org/)

参考文件**INSTALL**以及**NOTES.WIN**。编译需要使用perl以及NASM，perl建议使用ActiveState，NASM的话32位64位都可。

使用以下语句编译并安装（32位版）：

```bash
# 使用DLL运行时 同时生成DLL和静态库
perl Configure VC-WIN32
# 使用静态运行时 只生成静态库
perl Configure VC-WIN32 -static
nmake
nmake test
# 一定要执行安装才能生成对应的头文件
nmake install
```

会将文件安装到默认路径：`C:\Program Files (x86)\OpenSSL`

如果不需要编译测试程序，可使用 no-tests 参数。



### RapidJSON

版本 1.1.0

[主页](http://rapidjson.org/)  



### Websocketpp

版本 0.8.1

[主页](https://www.zaphoyd.com/websocketpp)  



### zlib

版本 1.2.11

[主页](http://www.zlib.net/)

**编译说明**

在根目录使用Cmake生成项目，然后执行编译和安装。

生成的项目为C风格导出，无` ZLIB_WINAPI `预定义宏。

32位的版本安装路径为`C:\Program Files (x86)\zlib`

