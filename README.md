# BilibiliTool

Bilibili直播站小工具 

模式12使用wss协议(boost beast)连接弹幕服务器，各分区监控一个房间。

模式13使用tcp协议(boost asio)连接弹幕服务器，经测试，网络条件允许时，1800个房间没问题。



接下来一段时间要忙于写论文和完善相关的仿真模型，其它东西只能先放一放了。不过毕竟是认真写的第一个软件，随着技术的提高也会不时改改，也算是致敬B站“新手村”的头衔。



**备忘1**

新的活动道具需要使用客户端挂机才能获得，使用了全新的API：

头部为：`https://live-trace.bilibili.com/xlive/data-interface/v1/heartbeat/`

共有3个指令：

`mobileEntry`：获取`secret_key`等信息

`mobileHeartBeat`：心跳包，300秒1次

`mobileExit`：退出时的通知

在后两个指令中存在新的`client_sign`签名。看到[HubertYoung/AcFun](https://github.com/HubertYoung/AcFun)的代码，发现数据包由类`LiveWatchTimeBody`生成，然后调用类`LiveWatcherTimeJni`签名，签名时调用了库`spyder_core`，因此需要动态调试。



**别人家的资料库**

- 樱花助手上船数据查询：[点我](https://list.bilibili.wiki/)



## Features  

目前已将抽奖的检测和领取分开。

消息服务器目前没有验证以及心跳环节。



**目前实现的功能**  

* 账号配置及查询

  * 6 刷新cookie，正常情况下不需要输验证码

  * 7 账户cookie有效性检测 **(在导入账号后必须执行)**  

  * 8 信息查询

    背包道具查询

    扭蛋币查询  

- 功能模块

  * 11 挂机模块 

    经验

    签到

    任务（银瓜子，双端观看）

    每日礼物领取

    银瓜子换硬币

    ~~主站登录奖励硬币(方案失效)~~

  * 12 检测全区或分区广播事件

    礼物抽奖（全区和分区）

    守护抽奖（总督）

  * 13 检测房间内事件

    节奏风暴

    礼物抽奖（房间内广播的 目前需要手动更新礼物ID）
    
    守护抽奖（舰长和提督）
    
    大乱斗抽奖
    
    弹幕抽奖（待测试）
    
    天选时刻抽奖（仅参加发送弹幕的）
    
  * 14 连接消息服务器并参与抽奖



**相关说明**   

- **添加账户后默认关闭所有可配置功能，需要导出数据后修改配置项重新导入。**  
- 在导入账号后需要使用功能(7)查询账户有效性后该账户才能执行其它操作。  
- 当检测到账号被封禁时会立即停止该账号的所有活动。  
- 参与活动抽奖时能够过滤不可进入的房间，避免钓鱼。  



## Config

### 用户账户配置文件

文件名：`ConfigUser.json`

文件格式为`JSON数组`，在导出时未增加换行，在修改时可手动格式化。

数据为列表，每一个成员为一个用户，通过修改对象中的`Conf`对象开关对应的功能。

配置项如下：

| 名称         | 内容       | 选项                      |
| ------------ | ---------- | ------------------------- |
| CoinExchange | 兑换硬币   | 0 不兑换 1 兑换           |
| Gift         | 礼物抽奖   | 0 不参与 1 通过WebAPI参与 |
| Storm        | 风暴抽奖   | 0 不参与 1 通过WebAPI参与 |
| Guard        | 守护抽奖   | 0 不参与 1 通过WebAPI参与 |
| PK           | 大乱斗抽奖 | 0 不参与 1 通过WebAPI参与 |
| Danmu        | 弹幕抽奖   | 0 不参与 1 通过WebAPI参与 |
| Anchor       | 天选抽奖   | 0 不参与 1 通过WebAPI参与 |



### 消息服务器配置文件

文件名：`ConfigServer.json`

文件格式为`JSON数组`

每一个条目包含`host`和`port`两项内容，类型为字符串



### 全区广播礼物列表

文件名：`ConfigGift.txt`

每一行为一个礼物ID。



## Build and Usage

### 文件组织结构

```
source dir
├── README
├── CMakelists.txt (cmake配置文件)
├── cmake/ (cmake插件)
├── config/ (配置文件)
├── doc/
├── Bilibili/ (主程序)
└── LotteryHistory/ (历史记录) 

build dir
├── bin/ (生成目录)
└── install/ (安装目录) 
```



### 依赖库  

- boost (1.70.0及以上)
- curl  
- openssl  
- rapidjson  
- zlib

依赖库的编译参考[doc/lib-build.md](doc/lib-build.md)

`boost`1.70.0之前的版本存在WSS无法连接的问题。



### 项目配置  

使用Cmake配置依赖库，定义下述变量：

* `BOOST_ROOT`：boost库的根目录，静态链接
* `libcurl`库的相关变量，动态链接
  * `CURL_INCLUDE_DIR`：头文件所在文件夹
  * `CURL_LIBRARY_RELEASE`：库文件路径
  * `CURL_DLL`：DLL文件路径（仅Windows平台配置）
* `OPENSSL_ROOT_DIR`：OpenSSL安装目录，动态链接
* `RapidJSON_ROOT`：RapidJSON头文件所在文件夹
* `ZLIB_ROOT`：ZLIB安装目录，动态链接

输出目录可以选择其它文件夹，不建议使用当前文件夹。

按照提示一步步添加路径，然后生成解决方案。

先执行`ALL_BUILD`，然后执行`INSTALL`，这时程序以及所需的DLL会被拷贝至`install/bin`文件夹，如果库文件的路径没有问题。

**编译器**

Windows平台可使用VS2017及后续版本。

Mac平台可使用Xcode。



## TODO

- 添加验证码识别领取瓜子(识别部分就不用C写了 是个大工程 有生之年)
- 重构代码，适配`doxygen`注释规范。



## License

本项目采用MIT开源协议  



## Thanks

感谢以下大佬贡献的代码： 

[Military-Doctor](https://github.com/Military-Doctor/Bilibili/) 

[Dawnnnnnn](https://github.com/Dawnnnnnn/bilibili-live-tools/)  

