# BilibiliTool

Bilibili直播站小工具 

模式12使用wss协议(websocketpp)连接弹幕服务器，监控5个房间。

模式13使用tcp协议(boost asio)连接弹幕服务器，经测试，网络条件允许时，1800个房间没问题。



很久没抽奖了，每次抽不了多久就进去了。一般也就挂着监控，隔段时间看一下。因此有些变动无法及时发现。

用C++编的软件，在配置各种依赖包的时候可能会比较麻烦，会看文档的话都不是问题。

`Cmake`是刚弄的，配置文件还是有待改进。

反正也没几个人用，如果有人遇到问题麻烦提一下，能改的也会尽量改。



## Attention

最近B站程序员做了一些比较大的改动，看了一下至少包含以下3件。

**第1件**

新的活动道具`铃音`需要使用客户端挂机才能获得，看了一下使用了全新的API：

头部为：`https://live-trace.bilibili.com/xlive/data-interface/v1/heartbeat/`

共有3个指令：

`mobileEntry`：获取`secret_key`等信息

`mobileHeartBeat`：心跳包，300秒1次

`mobileExit`：退出时的通知

在后两个指令中存在新的`client_sign`签名，没做过安卓开发，只能等大腿搞定了。

**第2件**

目前抽奖API多出了一个`time_wait`的参数，比如小电视是120秒，必须等倒计时结束才能参与抽奖。抽奖结束的时间不变。

客户端已实装，网页端暂时没有实装。

**第3件**

弹幕协议的连接数据包增加了一个`key`字段，需要通过API`/room/v1/Danmu/getConf`获取，目前`key`可以复用，不保证以后变成一次性的。



## Features  



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

  * 12 全区或分区广播事件

    小电视API抽奖（全区和分区） 

    总督上船低保

  * 13 无广播事件

    节奏风暴

    舰长以及提督上船低保
    
    大乱斗抽奖事件



**相关说明**   

- **添加账户后默认关闭所有可配置功能，需要导出数据后修改配置项重新导入。**  
- 在导入账号后需要使用功能(7)查询账户有效性后该账户才能执行其它操作。  
- 当检测到账号被封禁时会立即停止该账号的所有活动。  
- 挂机与参与抽奖两个功能目前不能同时开启，如有需要可以开启两份软件。  
- 在参与抽奖时设置有延时，多账号情况下两次领取间也存在1到2秒的延时。延时数值目前无法通过配置文件修改。  
- 参与活动抽奖时能够过滤不可进入的房间，避免钓鱼。  



**配置文件**

文件格式为`JSON`，在导出时未增加换行，在修改时可手动格式化。

数据为列表，每一个成员为一个用户，通过修改对象中的`Conf`对象开关对应的功能。

配置项如下：

* `CoinExchange`：0 不兑换硬币 1 使用瓜子换硬币
* `Lottery`：0 不参与抽奖 1 通过WebAPI参与抽奖
* `Storm`：0 不领取风暴 1 通过WebAPI领取风暴 2 通过安卓端API领取风暴
* `Guard`：0 不领取亲密 1 通过WebAPI领取亲密
* `PK`：0 不参与大乱斗抽奖 1 通过WebAPI参与大乱斗抽奖



**别人家的资料库**

- 樱花助手上船数据查询：[点我](https://list.bilibili.wiki/)



## Build and Usage

### 文件组织结构

```
source dir
├── README
├── CMakelists.txt (cmake配置文件)
├── cmake/ (cmake插件)
├── doc/
├── Bilibili/ (主程序)
└── LotteryHistory/ (历史记录) 

build dir
├── bin/ (生成目录)
└── install/ (安装目录) 
```



### 依赖库  

- boost  
- curl  
- openssl  
- rapidjson  
- websocketpp(需要做一些修改)
- zlib

依赖库的编译参考[doc/lib-build.md](doc/lib-build.md)

**[重要]WSS相关内容参考[doc/notes-wss.md](doc/notes-wss.md)**



### 项目配置  

使用Cmake配置依赖库，定义下述变量：

* `BOOST_ROOT`：boost库的根目录，静态链接
* `libcurl`库的相关变量，动态链接
  * `CURL_INCLUDE_DIR`：头文件所在文件夹
  * `CURL_LIBRARY_RELEASE`：库文件路径
  * `CURL_DLL`：DLL文件路径（仅Windows平台配置）
* `OPENSSL_ROOT_DIR`：OpenSSL安装目录，动态链接
* `RapidJSON_ROOT`：RapidJSON头文件所在文件夹
* `WebsocketPP_ROOT`：WebsocketPP头文件所在文件夹
* `ZLIB_ROOT`：ZLIB安装目录，动态链接

输出目录可以选择其它文件夹，不建议使用当前文件夹。

按照提示一步步添加路径，然后生成解决方案。

先执行`ALL_BUILD`，然后执行`INSTALL`，这时程序以及所需的DLL会被拷贝至`install/bin`文件夹，如果库文件的路径没有问题。

**编译器**

Windows平台建议使用VS2017，由于插件`WebsocketPP`目前不支持`Boost 1.70.0`，目前无法使用VS2019。

Mac平台我用的Xcode，还需要测试。



## TODO

- 添加验证码识别领取瓜子(识别部分就不用C写了 是个大工程 有生之年)



## License

本项目采用MIT开源协议  



## Thanks

感谢以下大佬贡献的代码： 

[Military-Doctor](https://github.com/Military-Doctor/Bilibili/) 

[Dawnnnnnn](https://github.com/Dawnnnnnn/bilibili-live-tools/)  

