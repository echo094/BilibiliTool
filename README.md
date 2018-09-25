# BilibiliTool

Bilibili直播站小工具 

单个房间使用Websocket协议连接弹幕服务器

多个房间使用Socket协议连接弹幕服务器



说心里话，这样一个号一天捡捡低保也不会消耗贵站多少资源，我们领低保也很开心，然后出了活动还会怀着感恩的心氪一单两单。何况，比起进进出出各个房间领低保，专心看主播看内容的在线时长会更可观。我们有了喜欢的主播，也不会忍心让他们没饭吃。

以前进一次小黑屋一天就出来了，这样吃一天饿一天，我们忍忍也就过去了。后来慢慢变成3天，7天，吃一顿要饿好久。我们都快饿死了。

求求你了，给我们留点口粮，不要这么赶尽杀绝，拜谢。



## Features  

**程序**  

- Bilibili 为助手主程序  
- ToolConfig 为配置程序(只能配置已登录用户)  

**目前实现的功能**  

- 7 账户cookie有效性检测 **(在导入账号后必须执行)**  

- 8 信息查询

  背包道具查询

  扭蛋币查询  

- 11 挂机相关 

  经验

  签到

  ~~银瓜子任务~~

  每日礼物领取

  运营活动礼物领取

  银瓜子换硬币

  ~~主站登录奖励硬币(方案失效)~~

- 12 全区或分区广播事件

  小电视API抽奖(包括摩天大楼，小金人) 

  总督上船低保

- 13 无广播事件

  节奏风暴

  舰长以及提督上船低保



**相关说明**  

- 在添加账号时需要手动输入验证码，验证码图片在当前目录下，名称为Captcha.jpg。  
- 在导入账号后需要使用功能(7)查询账户有效性后该账户才能执行其它操作。  
- 添加账户后默认开启所有功能，可以通过配置程序修改。  
- 当检测到账号被封禁时会立即停止该账号的所有活动。  
- 挂机与参与抽奖两个功能目前不能同时开启，如有需要可以开启两份软件。  
- 在参与抽奖时设置有延时，多账号情况下两次领取间也存在1到2秒的延时。延时数值目前无法通过配置文件修改。  
- 参与活动抽奖时能够过滤不可进入的房间，避免钓鱼。  
- **目前活动相关需要更新代码**  



**其它资料库**

- 樱花助手上船数据查询：[点我](https://list.bilibili.wiki/)



## Build and Usage

#### 文件组织结构

> BilibiliTool  
>
> > build(output)
> >
> > doc 
> >
> > Bilibili
> >
> > ToolConfig 
> >
> > toollib  
>
> share  
>
> > boost  
> >
> > > boost(boost include) 
> > >
> > > lib(boost lib)  
> >
> > curl  
> >
> > openssl
> >
> > rapidjson
> >
> > websocketpp  
> >
> > > websocketpp(websocketpp include)  

#### 项目配置  

默认配置：Debug运行时为MDd，Release运行时为MT。  

#### 依赖库  

- boost  
- curl  
- openssl  
- rapidjson  
- websocketpp  
- zlib

依赖库的编译参考[这里](doc/lib-build.md)



## TODO

- 解决Websocket在WSS连接中无法发送数据的问题(与B站WSS服务器的通信 问题不容易找)
- 使用BOOST的asio重写Socket
- 添加验证码识别领取瓜子(识别部分就不用C写了 是个大工程 有生之年)
- 非广播模式下头部直播间下播后也应纳入监控



## License

本项目采用MIT开源协议  



## Thanks

感谢以下大佬贡献的代码： 

[Military-Doctor](https://github.com/Military-Doctor/Bilibili/) 

[Dawnnnnnn](https://github.com/Dawnnnnnn/bilibili-live-tools/)  

