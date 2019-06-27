# 弹幕协议说明

弹幕协议可以参考这个代码：[bilive/bilive_server: bilibili直播房间弹幕监听](https://github.com/bilive/bilive_server)



## 总体说明

内容来源：

[弹幕服务器信息获取](https://api.live.bilibili.com/room/v1/Danmu/getConf?room_id=23058&platform=pc&player=web)

[协议数据处理(包含客户端版本信息)](https://s1.hdslb.com/bfs/static/player/live/loader/player-loader-1.7.4.min.js)

[JSON数据处理](https://s1.hdslb.com/bfs/static/blive/blfe-live-room/static/js/3.cd532a1d3ccf709a6d64.js)

处理函数：

flash通过调用 window._playerEventMap 对象处理事件。



所有以二进制形式表示数据，都是高位在前。

由于出现了100连礼物，一个未压缩的封包最大能够达到100k字节（平均一个礼物占用1k字节）。但考虑到该消息为房间内消息，不会影响正常的事件监控，因此目前直接丢弃这类过长的封包。

第2版协议对操作码为5的封包（包括协议头）进行了压缩，并且会智能地将多个封包合并到一个压缩包中。经过压缩的封包会将协议头中的版本置为2（解压后的各封包为0，与未压缩的一致）。



## 协议头(16字节)

协议头中的数据都是无符号整数。

| 起始字节 | 长度 | 内容                                      |
| -------- | ---- | ----------------------------------------- |
| 0        | 4    | 封包总大小                                |
| 4        | 2    | 头部长度                                  |
| 6        | 2    | 协议版本(版本0/1不压缩数据 版本2压缩数据) |
| 8        | 4    | 操作码（封包类型）                        |
| 12       | 4    | sequence 可以取常数1                      |



## 操作码

| 操作码 | 发送方 | 协议版本 | sequence | 内容                                   |
| ------ | ------ | -------- | -------- | -------------------------------------- |
| 2      | 客户端 | 1        | 1        | 心跳包                                 |
| 3      | 服务器 | 1        | 1        | 回应心跳包 内容为人气值                |
| 5      | 服务器 | 0        | 0        | 未经压缩的JSON数据包                   |
| 5      | 服务器 | 2        | 0        | 经过压缩的JSON数据包（包含一个或多个） |
| 7      | 客户端 | 1        | 1        | 认证并加入房间                         |
| 8      | 服务器 | 1        | 1        | 连接成功的回应包                       |



### 认证包结构

认证包中的key为API`/room/v1/Danmu/getConf`返回数据中的`data.token`值。

* websocket

```json
{
  "uid": 0表示未登录，否则为用户ID,
  "roomid": 房间ID,
  "protover": 2,
  "platform": "web",
  "clientver": "1.7.4",
  "type": 2,
  "key": "通过API获取"
}
```



* flash

```json
{
  "roomid": 房间ID,
  "platform": "flash",
  "key": "通过API获取",
  "clientver": "2.4.6-9e02b4f1",
  "uid": 随机数,
  "protover": 2
}
```



## 有效指令(78)



### 房间配置相关(18)

#### CHANGE_ROOM_INFO

房间背景更换。

```json
{
	"cmd": "CHANGE_ROOM_INFO",
	"background": "http:\/\/i0.hdslb.com\/bfs\/live\/6a9e9708f99a65198a45682569d30e129f8eaeea.jpg"
}
```



#### CUT_OFF

房间内被下播消息。



#### LIVE

房间内开播消息。



#### PREPARING

房间内下播消息。



#### WARNING

房间内管理员警告消息。

```json
{"cmd":"WARNING","msg":"禁止直播违禁游戏，请立即更换","roomid":776023}
```



#### ROOM_SKIN_MSG

更改皮肤。

```json
{
	"cmd": "ROOM_SKIN_MSG",
	"skin_id": 5,
	"status": 0,
	"end_time": 1552924800,
	"current_time": 1552924802
}
```



#### ROOM_BLOCK_INTO



#### ROOM_BLOCK_MSG

房间内用户被管理员封禁的消息。

```json
{
	"cmd": "ROOM_BLOCK_MSG",
	"uid": 33839631,
	"uname": "萌萌哒的玛丽",
	"data": {
		"uid": "30412231",
		"uname": "萌萌哒的玛丽",
		"operator": 1
	},
	"roomid": 13941486
}
```



#### ROOM_KICKOUT



#### ROOM_LOCK

房间被封禁。

```json
{
	"cmd": "ROOM_LOCK",
	"expire": "2018-11-22 16:47:51",
	"roomid": 14845433
}
```



#### ROOM_LIMIT



#### ROOM_RANK

房间内排行榜变动信息。

```json
{
	"cmd": "ROOM_RANK",
	"data": {
		"roomid": 3036985,
		"rank_desc": "小时榜 170",
		"color": "#FB7299",
		"h5_url": "https:\/\/live.bilibili.com\/p\/eden\/rank-h5-current?anchor_uid=872065&rank_type=master_realtime_hour_room",
		"web_url": "https:\/\/live.bilibili.com\/blackboard\/room-current-rank.html?rank_type=master_realtime_hour_room",
		"timestamp": 1540274042
	}
}
```



#### ROOM_REFRESH



#### ROOM_SILENT_ON

房间内开启禁言。

```json
{
	"cmd": "ROOM_SILENT_ON",
	"data": {
		"type": "level",
		"level": 1,
		"second": 1540279450
	},
	"roomid": 3515408
}
```



#### ROOM_SILENT_OFF

房间内关闭禁言。

```json
{
	"cmd": "ROOM_SILENT_OFF",
	"data": [],
	"roomid": 3515408
}
```



#### ROOM_REAL_TIME_MESSAGE_UPDATE

粉丝数更新。

```json
{
	"cmd": "ROOM_REAL_TIME_MESSAGE_UPDATE",
	"data": {
		"roomid": 10175194,
		"fans": 19606
	}
}
```



#### ROOM_SHIELD(无处理)

无需响应，设置屏蔽词汇以及用户。

```json
{
	"cmd": "ROOM_SHIELD",
	"type": 1,
	"user": "",
	"keyword": "",
	"roomid": 929493
}
```



#### ROOM_ADMINS(无处理)

无需响应，设置管理员。

```json
{
	"cmd": "ROOM_ADMINS",
	"uids": [160720921]
}
```



### 消息相关(11)

#### COMBO_SEND

房间内送礼连击。

```json
{
	"cmd": "COMBO_SEND",
	"data": {
		"uid": 51955965,
		"uname": "我是尔尔啊",
		"combo_num": 3,
		"gift_name": "吃瓜",
		"gift_id": 20004,
		"action": "赠送",
		"combo_id": "gift:combo_id:51955965:4801245:20004:1540273897.6808"
	}
}
```



#### DANMU_MSG

弹幕消息。



#### GUARD_BUY

房间内船员购买消息。

```json
{
	"cmd": "GUARD_BUY",
	"data": {
		"uid": 606066,
		"username": "病理酱Alter",
		"guard_level": 3,
		"num": 1,
		"price": 198000,
		"gift_id": 10003,
		"gift_name": "舰长",
		"start_time": 1540273959,
		"end_time": 1540273959
	}
}
```



#### LUCK_GIFT_AWARD_USER



#### MESSAGEBOX_USER_GAIN_MEDAL



#### NOTICE_MSG

新的系统消息，替换SYS_MSG、GUARD_MSG。

根据msg_type区分消息类型：



1 广播的公告。

```json
{
	"full": {
		"head_icon": "http:\/\/i0.hdslb.com\/bfs\/live\/b049ac07021f3e4269d22a79ca53e6e7815af9ba.png",
		"tail_icon": "http:\/\/i0.hdslb.com\/bfs\/live\/822da481fdaba986d738db5d8fd469ffa95a8fa1.webp",
		"head_icon_fa": "http:\/\/i0.hdslb.com\/bfs\/live\/b049ac07021f3e4269d22a79ca53e6e7815af9ba.png",
		"tail_icon_fa": "http:\/\/i0.hdslb.com\/bfs\/live\/38cb2a9f1209b16c0f15162b0b553e3b28d9f16f.png",
		"head_icon_fan": 1,
		"tail_icon_fan": 4,
		"background": "#FFE6BDFF",
		"color": "#9D5412FF",
		"highlight": "#FF6933FF",
		"time": 10
	},
	"half": {
		"head_icon": "http:\/\/i0.hdslb.com\/bfs\/live\/4db5bf9efcac5d5928b6040038831ffe85a91883.png",
		"tail_icon": "",
		"background": "#FFE6BDFF",
		"color": "#9D5412FF",
		"highlight": "#FF6933FF",
		"time": 8
	},
	"side": {
		"head_icon": "http:\/\/i0.hdslb.com\/bfs\/live\/fa323d24f448d670bcc3dc59996d17463860a6b3.png",
		"background": "#F5EBDDFF",
		"color": "#DA9F77FF",
		"highlight": "#C67137FF",
		"border": "#ECDDC0FF"
	},
	"msg_type": 1,
	"msg_common": "用沉船告诉你，如何成为最非主播。",
	"msg_self": "用沉船告诉你，如何成为最非主播。",
	"real_roomid": 451,
	"link_url": "https:\/\/live.bilibili.com\/451?from=28003&extra_jump_from=28003",
	"shield_uid": -1,
	"cmd": "NOTICE_MSG",
	"roomid": 0
}
```

2 广播的抽奖消息。

```json
{
	"cmd": "NOTICE_MSG",
	"full": {
		"head_icon": "http:\/\/i0.hdslb.com\/bfs\/live\/00f26756182b2e9d06c00af23001bc8e10da67d0.webp",
		"tail_icon": "http:\/\/i0.hdslb.com\/bfs\/live\/822da481fdaba986d738db5d8fd469ffa95a8fa1.webp",
		"head_icon_fa": "http:\/\/i0.hdslb.com\/bfs\/live\/77983005023dc3f31cd599b637c83a764c842f87.png",
		"tail_icon_fa": "http:\/\/i0.hdslb.com\/bfs\/live\/38cb2a9f1209b16c0f15162b0b553e3b28d9f16f.png",
		"head_icon_fan": 36,
		"tail_icon_fan": 4,
		"background": "#6098FFFF",
		"color": "#FFFFFFFF",
		"highlight": "#FDFF2FFF",
		"time": 20
	},
	"half": {
		"head_icon": "http:\/\/i0.hdslb.com\/bfs\/live\/358cc52e974b315e83eee429858de4fee97a1ef5.png",
		"tail_icon": "",
		"background": "#7BB6F2FF",
		"color": "#FFFFFFFF",
		"highlight": "#FDFF2FFF",
		"time": 15
	},
	"side": {
		"head_icon": "http:\/\/i0.hdslb.com\/bfs\/live\/7ef6f2b7fe01c550f93dd569ca4b573b80b9e6a5.png",
		"background": "#E6F4FFFF",
		"color": "#51A8FBFF",
		"highlight": "#0081F0FF",
		"border": "#BDD9FFFF"
	},
	"roomid": 11731666,
	"real_roomid": 11731666,
	"msg_common": "手游区广播: <%百茷襛%> 送给<% 安吉丽娜都蒙%> 1个摩天大楼，点击前往TA的房间去抽奖吧",
	"msg_self": "手游区广播: <%百茷襛%> 送给<% 安吉丽娜都蒙%> 1个摩天大楼，快来抽奖吧",
	"link_url": "http:\/\/live.bilibili.com\/11731666?live_lottery_type=1&broadcast_type=0&from=28003&extra_jump_from=28003",
	"msg_type": 2,
	"shield_uid": -1
}
```

3 包括当前房间（舰长以及提督）和全区广播（总督）

```
{
	"cmd": "NOTICE_MSG",
	"full": {
		"head_icon": "http:\/\/i0.hdslb.com\/bfs\/live\/72337e86020b8d0874d817f15c48a610894b94ff.png",
		"tail_icon": "http:\/\/i0.hdslb.com\/bfs\/live\/822da481fdaba986d738db5d8fd469ffa95a8fa1.webp",
		"head_icon_fa": "http:\/\/i0.hdslb.com\/bfs\/live\/72337e86020b8d0874d817f15c48a610894b94ff.png",
		"tail_icon_fa": "http:\/\/i0.hdslb.com\/bfs\/live\/38cb2a9f1209b16c0f15162b0b553e3b28d9f16f.png",
		"head_icon_fan": 1,
		"tail_icon_fan": 4,
		"background": "#FFB03CFF",
		"color": "#FFFFFFFF",
		"highlight": "#B25AC1FF",
		"time": 10
	},
	"half": {
		"head_icon": "",
		"tail_icon": "",
		"background": "",
		"color": "",
		"highlight": "",
		"time": 8
	},
	"side": {
		"head_icon": "http:\/\/i0.hdslb.com\/bfs\/live\/31566d8cd5d468c30de8c148c5d06b3b345d8333.png",
		"background": "#FFE9C8FF",
		"color": "#EF903AFF",
		"highlight": "#D54900FF",
		"border": "#FFCFA4FF"
	},
	"roomid": 10729306,
	"real_roomid": 10729306,
	"msg_common": "<%孤島の風%> 在本房间开通了舰长",
	"msg_self": "<%孤島の風%> 在本房间开通了舰长",
	"link_url": "https:\/\/live.bilibili.com\/10729306?live_lottery_type=2&broadcast_type=1&from=28003&extra_jump_from=28003",
	"msg_type": 3,
	"shield_uid": -1
}
{
	"cmd": "NOTICE_MSG",
	"full": {
		"head_icon": "http:\/\/i0.hdslb.com\/bfs\/live\/d63e78ade2319108390b1d6a59a81b2abe46925d.png",
		"tail_icon": "http:\/\/i0.hdslb.com\/bfs\/live\/822da481fdaba986d738db5d8fd469ffa95a8fa1.webp",
		"head_icon_fa": "http:\/\/i0.hdslb.com\/bfs\/live\/d63e78ade2319108390b1d6a59a81b2abe46925d.png",
		"tail_icon_fa": "http:\/\/i0.hdslb.com\/bfs\/live\/38cb2a9f1209b16c0f15162b0b553e3b28d9f16f.png",
		"head_icon_fan": 1,
		"tail_icon_fan": 4,
		"background": "#FFB03CFF",
		"color": "#FFFFFFFF",
		"highlight": "#B25AC1FF",
		"time": 10
	},
	"half": {
		"head_icon": "",
		"tail_icon": "",
		"background": "",
		"color": "",
		"highlight": "",
		"time": 8
	},
	"side": {
		"head_icon": "http:\/\/i0.hdslb.com\/bfs\/live\/17c5353140045345f31c7475432920df08351837.png",
		"background": "#FFE9C8FF",
		"color": "#EF903AFF",
		"highlight": "#D54900FF",
		"border": "#FFCFA4FF"
	},
	"roomid": 425315,
	"real_roomid": 425315,
	"msg_common": "<%太太太太太了%> 在 <%Dae-天涯ovo%> 的房间开通了总督并触发了抽奖，点击前往TA的房间去抽奖吧",
	"msg_self": "<%太太太太太了%> 在本房间开通了总督并触发了抽奖，快来抽奖吧",
	"link_url": "https:\/\/live.bilibili.com\/425315?live_lottery_type=2&broadcast_type=0&from=28003&extra_jump_from=28003",
	"msg_type": 3,
	"shield_uid": -1
}
```

4 房间内总督进房间欢迎消息。

```json
{
	"cmd": "NOTICE_MSG",
	"full": {
		"head_icon": "http:\/\/i0.hdslb.com\/bfs\/live\/d2a0c88d9da045c8d0677dea97cb63c985c76ecc.webp",
		"tail_icon": "",
		"head_icon_fa": "http:\/\/i0.hdslb.com\/bfs\/live\/4db954c31db24c469a87b3951b818d80de08501a.png",
		"tail_icon_fa": "",
		"head_icon_fan": 25,
		"tail_icon_fan": 0,
		"background": "#FFB64AFF",
		"color": "#FFFFFFFF",
		"highlight": "#B25AC1FF",
		"time": 10
	},
	"half": {
		"head_icon": "",
		"tail_icon": "",
		"background": "#FFB64AFF",
		"color": "#FFFFFFFF",
		"highlight": "#B25AC1FF",
		"time": 8
	},
	"side": {
		"head_icon": "",
		"background": "",
		"color": "",
		"highlight": "",
		"border": ""
	},
	"roomid": 3742025,
	"real_roomid": 3742025,
	"msg_common": "欢迎 <%总督 不再瞎逛的菜菜大佬%> 登船",
	"msg_self": "欢迎 <%总督 不再瞎逛的菜菜大佬%> 登船",
	"link_url": "",
	"msg_type": 4,
	"shield_uid": 229007783
}
```

5 房间内的获奖信息

```json
{
	"cmd": "NOTICE_MSG",
	"full": {
		"head_icon": "http:\/\/i0.hdslb.com\/bfs\/live\/bdd67e50e04fb4668e77294ace37ea59524ddb8c.webp",
		"tail_icon": "",
		"head_icon_fa": "http:\/\/i0.hdslb.com\/bfs\/live\/67dbb744cf794317e5a7df77e91a3d21fea4771f.png",
		"tail_icon_fa": "",
		"head_icon_fan": 27,
		"tail_icon_fan": 0,
		"background": "#A6714AFF",
		"color": "#FFFFFFFF",
		"highlight": "#FDFF2FFF",
		"time": 10
	},
	"half": {
		"head_icon": "http:\/\/i0.hdslb.com\/bfs\/live\/ad30c259cbc5cefb482585421d650a0288cbe03c.png",
		"tail_icon": "",
		"background": "#DE9C72FF",
		"color": "#FFFFFFFF",
		"highlight": "#FDFF2FFF",
		"time": 8
	},
	"side": {
		"head_icon": "",
		"background": "",
		"color": "",
		"highlight": "",
		"border": ""
	},
	"roomid": 11731666,
	"real_roomid": 0,
	"msg_common": "恭喜 <%她依旧思无邪%> 获得大奖 <%20x普通扭蛋币%>, 感谢 <%安吉丽娜都蒙%> 的赠送",
	"msg_self": "恭喜 <%她依旧思无邪%> 获得大奖 <%20x普通扭蛋币%>, 感谢 <%安吉丽娜都蒙%> 的赠送",
	"link_url": "",
	"msg_type": 5,
	"shield_uid": -1
}
```

6 广播的20倍节奏风暴。

```json
{
	"full": {
		"head_icon": "http:\/\/i0.hdslb.com\/bfs\/live\/6fb61c0b149b46571b7945ba4e7561b92929bd04.webp",
		"tail_icon": "http:\/\/i0.hdslb.com\/bfs\/live\/822da481fdaba986d738db5d8fd469ffa95a8fa1.webp",
		"head_icon_fa": "http:\/\/i0.hdslb.com\/bfs\/live\/88b6e816855fdaedb5664359d8b5a5ae7de39807.png",
		"tail_icon_fa": "http:\/\/i0.hdslb.com\/bfs\/live\/38cb2a9f1209b16c0f15162b0b553e3b28d9f16f.png",
		"head_icon_fan": 24,
		"tail_icon_fan": 4,
		"background": "#5E769FFF",
		"color": "#FFFFFFFF",
		"highlight": "#FFF77FFF",
		"time": 10
	},
	"half": {
		"head_icon": "http:\/\/i0.hdslb.com\/bfs\/live\/4db5bf9efcac5d5928b6040038831ffe85a91883.png",
		"tail_icon": "",
		"background": "#8DA3C7FF",
		"color": "#FFFFFFFF",
		"highlight": "#FFF77FFF",
		"time": 8
	},
	"msg_type": 6,
	"roomid": 5540554,
	"real_roomid": 5540554,
	"msg_common": "<%Eli大老师%> 在直播间 <%5540554%> 使用了 <%20%> 倍节奏风暴，大家快去跟风领取奖励吧！",
	"msg_self": "<%Eli大老师%> 在直播间 <%5540554%> 使用了 <%20%> 倍节奏风暴，大家快去跟风领取奖励吧！",
	"link_url": "http:\/\/live.bilibili.com\/5540554?broadcast_type=-1&from=28003&extra_jump_from=28003",
	"cmd": "NOTICE_MSG",
	"side": {
		"head_icon": "",
		"background": "",
		"color": "",
		"highlight": "",
		"border": ""
	},
	"shield_uid": -1
}
```

8 广播的抽奖消息，替换消息2，出现在11月1日16:58至19:13的时间段内。

```json
{
	"cmd": "NOTICE_MSG",
	"full": {
		"head_icon": "http://i0.hdslb.com/bfs/live/b29add66421580c3e680d784a827202e512a40a0.webp",
		"tail_icon": "http://i0.hdslb.com/bfs/live/822da481fdaba986d738db5d8fd469ffa95a8fa1.webp",
		"head_icon_fa": "http://i0.hdslb.com/bfs/live/49869a52d6225a3e70bbf1f4da63f199a95384b2.png",
		"tail_icon_fa": "http://i0.hdslb.com/bfs/live/38cb2a9f1209b16c0f15162b0b553e3b28d9f16f.png",
		"head_icon_fan": 24,
		"tail_icon_fan": 4,
		"background": "#66A74EFF",
		"color": "#FFFFFFFF",
		"highlight": "#FDFF2FFF",
		"time": 20
	},
	"half": {
		"head_icon": "http://i0.hdslb.com/bfs/live/ec9b374caec5bd84898f3780a10189be96b86d4e.png",
		"tail_icon": "",
		"background": "#85B971FF",
		"color": "#FFFFFFFF",
		"highlight": "#FDFF2FFF",
		"time": 15
	},
	"side": {
		"head_icon": "http://i0.hdslb.com/bfs/live/e41c7e12b1e08724d2ab2f369515132d30fe1ef7.png",
		"background": "#F4FDE8FF",
		"color": "#79B48EFF",
		"highlight": "#388726FF",
		"border": "#A9DA9FFF"
	},
	"roomid": 516984,
	"real_roomid": 516984,
	"msg_common": "全区广播：<%阿鲁迪巴1%>送给<%YJ-洛茵w%>1个小电视飞船，点击前往TA的房间去抽奖吧",
	"msg_self": "全区广播：<%阿鲁迪巴1%>送给<%YJ-洛茵w%>1个小电视飞船，快来抽奖吧",
	"link_url": "http://live.bilibili.com/516984?live_lottery_type=1&broadcast_type=0&from=28003&extra_jump_from=28003",
	"msg_type": 8,
	"shield_uid": -1
}
```

9 大乱斗晋级广播

```json
{
	"cmd": "NOTICE_MSG",
	"full": {
		"head_icon": "https://i0.hdslb.com/bfs/vc/a3b46a9ce8ae4142e14fcf39400cef5ea52cd2cb.png",
		"tail_icon": "",
		"head_icon_fa": "http://i0.hdslb.com/bfs/vc/9642e509f441f9c324649b71ee31304cfcb87345.png",
		"tail_icon_fa": "http://i0.hdslb.com/bfs/live/38cb2a9f1209b16c0f15162b0b553e3b28d9f16f.png",
		"head_icon_fan": 1,
		"tail_icon_fan": 4,
		"background": "#5423A8",
		"color": "#FFFFFF",
		"highlight": "#FDFF2F",
		"time": 10
	},
	"half": {
		"head_icon": "https://i0.hdslb.com/bfs/vc/20f8b8bc87a5e897f7e8ecbb7a9079d684cb1cb4.png",
		"tail_icon": "",
		"background": "#5423A8",
		"color": "#FFFFFF",
		"highlight": "#FDFF2F",
		"time": 8
	},
	"side": {
		"head_icon": "http://i0.hdslb.com/bfs/vc/7d8267591763bdb330b6e4d0d60e428db4aceff6.png",
		"background": "#F6F1FF",
		"color": "#D48FFF",
		"highlight": "#9D00FF",
		"border": "#E3B6FF",
		"time": 8
	},
	"roomid": 10729306,
	"real_roomid": 10729306,
	"msg_common": "恭喜主播 <%会飞的芽子%> 大乱斗 神圣大天使1星 达成～快去看看吧",
	"msg_self": "恭喜主播 <%会飞的芽子%> 大乱斗 神圣大天使1星 达成～",
	"link_url": "https://live.bilibili.com/10729306?from=28003&extra_jump_from=28003",
	"msg_type": 9,
	"shield_uid": 0
}
```



#### SEND_GIFT

房间内赠送礼物消息。



#### SEND_TOP



#### WELCOME

房间内姥爷进入欢迎消息。

```json
{"cmd":"WELCOME","data":{"uid":172281508,"uname":"_既望","is_admin":false,"vip":1}}
```



#### WELCOME_GUARD

房间内船员进入欢迎消息。

```json
{"cmd":"WELCOME_GUARD","data":{"uid":18910980,"username":"玲家的糖","guard_level":3}}
```



#### ENTRY_EFFECT

房间内用户进入特效。

```json
{
	"cmd": "ENTRY_EFFECT",
	"data": {
		"id": 4,
		"uid": 18910980,
		"target_id": 19738891,
		"show_avatar": 1,
		"copy_writing": "欢迎舰长 <%玲家的糖%> 进入直播间",
		"highlight_color": "#E6FF00",
		"basemap_url": "http:\/\/i0.hdslb.com\/bfs\/live\/1fa3cc06258e16c0ac4c209e2645fda3c2791894.png",
		"effective_time": 2,
		"priority": 70,
		"privilege_type": 3,
		"face": "http:\/\/i1.hdslb.com\/bfs\/face\/855cf5155c3122b15b5c57bd4ddd4c94c0841e12.jpg"
	}
}
```



### PK模式相关(15)

#### PK_INVITE_INIT(无处理)

无需响应，邀请玩家。

```json
{
	"cmd": "PK_INVITE_INIT",
	"pk_invite_status": 200,
	"invite_id": 6610,
	"face": "http:\/\/i0.hdslb.com\/bfs\/face\/97733dda698d97c69735f7a54452b7b4c0df07cd.jpg",
	"uname": "豆喵才不是矮子",
	"area_name": "视频聊天",
	"user_level": 3,
	"master_level": 12,
	"roomid": 880638
}
```



#### PK_INVITE_FAIL(无处理)

无需响应。

```json
{
	"cmd": "PK_INVITE_FAIL",
	"pk_invite_status": 1100,
	"invite_id": 6707,
	"roomid": "8260376"
}
```



#### PK_INVITE_CANCEL(无处理)

无需响应。

```json
{
	"cmd": "PK_INVITE_CANCEL",
	"pk_invite_status": 1200,
	"invite_id": 6715,
	"face": "http:\/\/i1.hdslb.com\/bfs\/face\/b53c77a7ef4fe2b015419b926cad420f1b0cbbfa.jpg",
	"uname": "王二壮阿",
	"area_name": "视频聊天",
	"user_level": 20,
	"master_level": 17,
	"roomid": "9758780"
}
```



#### PK_INVITE_SWITCH_OPEN(无处理)

无需响应。

```json
{
	"cmd": "PK_INVITE_SWITCH_OPEN",
	"roomid": 1542742
}
```



#### PK_INVITE_SWITCH_CLOSE(无处理)

无需响应。

```json
{
	"cmd": "PK_INVITE_SWITCH_CLOSE",
	"roomid": 81697
}
```



#### PK_INVITE_REFUSE(无处理)

无需响应。

```json
{
	"cmd": "PK_INVITE_REFUSE",
	"pk_invite_status": 1100,
	"invite_id": 6724,
	"roomid": "14347962"
}
```



#### PK_MATCH

比赛生成。

```json
{
	"cmd": "PK_MATCH",
	"pk_status": 100,
	"pk_id": 170781,
	"data": {
		"init_id": 1445045,
		"match_id": 14186707,
		"escape_all_time": 10,
		"escape_time": 9,
		"is_portrait": false,
		"uname": "Bunny兔子乖",
		"face": "http:\/\/i0.hdslb.com\/bfs\/face\/3013b7fc63ec48d2b8fe841c622c9b3d4fb8d8de.jpg",
		"uid": 376290374
	},
	"roomid": "1445045"
}
```



#### PK_PRE

比赛准备阶段。

```json
{
	"cmd": "PK_PRE",
	"pk_id": 170781,
	"pk_status": 200,
	"data": {
		"init_id": 1445045,
		"match_id": 14186707,
		"count_down": 5,
		"pk_topic": "唱首恋爱曲",
		"pk_pre_time": 1540287425,
		"pk_start_time": 1540287430,
		"pk_end_time": 1540287730,
		"end_time": 1540287850
	}
}
```



#### PK_START

比赛开始。

```json
{
	"cmd": "PK_START",
	"pk_id": 170781,
	"pk_status": 300,
	"data": {
		"init_id": 1445045,
		"match_id": 14186707,
		"pk_topic": "唱首恋爱曲"
	}
}
```



#### PK_PROCESS

比赛过程。

```json
{
	"cmd": "PK_PROCESS",
	"pk_id": 170781,
	"pk_status": 300,
	"data": {
		"uid": 351928542,
		"init_votes": 8,
		"match_votes": 0,
		"user_votes": 8
	},
	"roomid": 1445045
}
```



#### PK_END

比赛结束。

```json
{
	"cmd": "PK_END",
	"pk_id": 170781,
	"pk_status": 400,
	"data": {
		"init_id": 1445045,
		"match_id": 14186707,
		"punish_topic": "惩罚：闻自己的鞋子30秒"
	}
}
```



#### PK_SETTLE

比赛结果。

```json
{
	"cmd": "PK_SETTLE",
	"pk_id": 170781,
	"pk_status": 400,
	"data": {
		"pk_id": 170781,
		"init_info": {
			"uid": 3932788,
			"init_id": 1445045,
			"uname": "大腿儿想吃肉",
			"face": "http:\/\/i0.hdslb.com\/bfs\/face\/019ee58f2e438775d63e1269f3ad0c966ca685dc.jpg",
			"votes": 16,
			"is_winner": true,
			"vip_type": 0,
			"exp": {
				"color": 6406234,
				"user_level": 20,
				"master_level": {
					"level": 17,
					"color": 5805790
				}
			},
			"vip": {
				"vip": 0,
				"svip": 0
			},
			"face_frame": "",
			"badge": {
				"url": "",
				"desc": "",
				"position": 0
			}
		},
		"match_info": {
			"uid": 376290374,
			"match_id": 14186707,
			"uname": "Bunny兔子乖",
			"face": "http:\/\/i0.hdslb.com\/bfs\/face\/3013b7fc63ec48d2b8fe841c622c9b3d4fb8d8de.jpg",
			"votes": 0,
			"is_winner": false
		},
		"best_user": {
			"uid": 351928542,
			"uname": "血姐姐_",
			"face": "http:\/\/i2.hdslb.com\/bfs\/face\/7e089f8599081a3d2792598ecd56c342cec4d0bc.jpg",
			"vip_type": 2,
			"exp": {
				"color": 5805790,
				"user_level": 21,
				"master_level": {
					"level": 1,
					"color": 6406234
				}
			},
			"vip": {
				"vip": 1,
				"svip": 1
			},
			"privilege_type": 3,
			"face_frame": "http:\/\/i0.hdslb.com\/bfs\/live\/78e8a800e97403f1137c0c1b5029648c390be390.png",
			"badge": {
				"url": "http:\/\/i0.hdslb.com\/bfs\/live\/b5e9ebd5ddb979a482421ca4ea2f8c1cc593370b.png",
				"desc": "",
				"position": 3
			}
		},
		"punish_topic": "惩罚：闻自己的鞋子30秒"
	}
}
```



#### PK_AGAIN

重新比赛。

```json
{
	"cmd": "PK_AGAIN",
	"pk_id": 170781,
	"pk_status": 400,
	"data": {
		"new_pk_id": 170786,
		"init_id": 1445045,
		"match_id": 14186707,
		"escape_all_time": 10,
		"escape_time": 9,
		"is_portrait": false,
		"uname": "Bunny兔子乖",
		"face": "http:\/\/i0.hdslb.com\/bfs\/face\/3013b7fc63ec48d2b8fe841c622c9b3d4fb8d8de.jpg",
		"uid": 376290374
	},
	"roomid": 1445045
}
```



#### PK_MIC_END

比赛连麦终止。

```json
{
	"cmd": "PK_MIC_END",
	"pk_id": 170781,
	"pk_status": 1000,
	"data": {
		"pk_id": 170781,
		"type": 1,
		"pk_status": 1000,
		"new_pk_id": 170786,
		"init_id": 1445045,
		"match_id": 14186707
	}
}
```



#### PK_CLICK_AGAIN(无处理)

无需响应。

```json
{
	"pk_status": 400,
	"pk_id": 170781,
	"cmd": "PK_CLICK_AGAIN",
	"roomid": 1445045
}
```



### 大乱斗(13)

2019/06/17新增的模式：[大乱斗教程](https://live.bilibili.com/blackboard/activity-DLD.html)

#### PK_BATTLE_ENTRANCE(无处理)

```json
{
	"cmd": "PK_BATTLE_ENTRANCE",
	"timestamp": 1560510240,
	"data": {
		"is_open": true
	}
}
```



#### PK_BATTLE_MATCH_TIMEOUT(无处理)

超时

```json
{
	"cmd": "PK_BATTLE_MATCH_TIMEOUT",
	"data": []
}
```



#### PK_BATTLE_PRE

准备阶段

```json
{
	"cmd": "PK_BATTLE_PRE",
	"pk_status": 101,
	"pk_id": 332920,
	"timestamp": 1560704671,
	"data": {
		"uname": "糖酥小青菜",
		"face": "http://i0.hdslb.com/bfs/face/fc966f9b2c9ce667931c94cc16eca3477f988274.jpg",
		"uid": 57584675,
		"room_id": 12097384,
		"pre_timer": 10,
		"pk_votes_name": "魔法值"
	}
}
```



#### PK_BATTLE_START

大乱斗开始

```json
{
	"cmd": "PK_BATTLE_START",
	"pk_id": 332920,
	"pk_status": 201,
	"timestamp": 1560704681,
	"data": {
		"final_hit_votes": 10000,
		"pk_start_time": 1560704681,
		"pk_frozen_time": 1560705161,
		"pk_end_time": 1560705171,
		"pk_votes_type": 0,
		"pk_votes_add": 0,
		"pk_votes_name": "魔法值"
	}
}
```



#### PK_BATTLE_PROCESS

更新数据

```json
{
	"cmd": "PK_BATTLE_PROCESS",
	"pk_id": 332920,
	"pk_status": 301,
	"timestamp": 1560704685,
	"data": {
		"init_info": {
			"room_id": 12097384,
			"votes": 0,
			"best_uname": ""
		},
		"match_info": {
			"room_id": 982077,
			"votes": 120000,
			"best_uname": "SDNYNG"
		}
	}
}
```



#### PK_BATTLE_PRO_TYPE

更改形式

```json
{
	"cmd": "PK_BATTLE_PRO_TYPE",
	"pk_id": 332920,
	"pk_status": 301,
	"timestamp": 1560704685,
	"data": {
		"timer": 60,
		"final_hit_room_id": 982077,
		"be_final_hit_room_id": 12097384
	}
}
```



#### PK_BATTLE_GIFT

房间内消息，使用大乱斗道具。

```json
{
	"cmd": "PK_BATTLE_GIFT",
	"timestamp": 1560831881,
	"pk_id": 336604,
	"pk_status": 201,
	"data": {
		"room_id": 1355059,
		"gift_id": 30240,
		"gift_msg": "以太坊爱好...赠送了一个时光沙漏"
	}
}
```



#### PK_BATTLE_VOTES_ADD

房间内消息，大乱斗魔法值增加。

```json
{
	"cmd": "PK_BATTLE_VOTES_ADD",
	"timestamp": 1560859201,
	"data": {
		"type": 1,
		"pk_votes_add": 0.05,
		"pk_votes_name": "魔法值"
	}
}
```



#### PK_BATTLE_END

大乱斗结束

```json
{
	"cmd": "PK_BATTLE_END",
	"pk_id": "332920",
	"pk_status": 501,
	"timestamp": 1560704745,
	"data": {
		"timer": 10,
		"init_info": {
			"room_id": 12097384,
			"votes": 400,
			"winner_type": -1,
			"best_uname": "离久不成悲"
		},
		"match_info": {
			"room_id": 982077,
			"votes": 121440,
			"winner_type": 3,
			"best_uname": "SDNYNG"
		}
	}
}
```



#### PK_BATTLE_RANK_CHANGE(无处理)

等级变更

```json
{
	"cmd": "PK_BATTLE_RANK_CHANGE",
	"timestamp": 1560704745,
	"data": {
		"first_rank_img_url": "http://i0.hdslb.com/bfs/live/dfd84488f4cc342ca51819a9adcb61ba8bbccc07.png",
		"rank_name": "初阶守护者x1"
	}
}
```



#### PK_BATTLE_SETTLE_USER

设置结果？

```json
{
	"cmd": "PK_BATTLE_SETTLE_USER",
	"pk_id": 332920,
	"pk_status": 501,
	"settle_status": 1,
	"timestamp": 1560704745,
	"data": {
		"settle_status": 1,
		"result_type": 3,
		"winner": {
			"uid": 16692120,
			"uname": "绘梨衣是个小怪兽呐",
			"face": "http://i1.hdslb.com/bfs/face/5da5a7beb9cf4c9cfa3d519f2ef161aeb9dd8422.jpg",
			"face_frame": "https://i0.hdslb.com/bfs/vc/922eb74a4c5353812450718c93a4cdb624f1dba6.png",
			"exp": {
				"color": 10512625,
				"user_level": 39,
				"master_level": {
					"color": 16746162,
					"level": 31
				}
			},
			"best_user": {
				"uid": 11249043,
				"uname": "SDNYNG",
				"face": "http://i2.hdslb.com/bfs/face/7a579666caf946a553cf3f83b33bfe0e6371398e.jpg",
				"pk_votes": 121440,
				"pk_votes_name": "魔法值",
				"exp": {
					"color": 6406234,
					"level": 1
				},
				"face_frame": "http://i0.hdslb.com/bfs/live/9b3cfee134611c61b71e38776c58ad67b253c40a.png",
				"badge": "",
				"award_info": null
			}
		}
	}
}
```



#### PK_BATTLE_SETTLE(无处理)

结果类型？

```json
{
	"cmd": "PK_BATTLE_SETTLE",
	"pk_id": 332920,
	"pk_status": 501,
	"settle_status": 1,
	"timestamp": 1560704745,
	"data": {
		"result_type": 3
	}
}
```



#### PK_LOTTERY_START

房间内公告抽奖开始

```json
{
	"cmd": "PK_LOTTERY_START",
	"data": {
		"asset_animation_pic": "https://i0.hdslb.com/bfs/live/e1ab9f88b4af63fbf15197acea2dbb60bfc4434b.gif",
		"asset_icon": "https://i0.hdslb.com/bfs/vc/44c367b09a8271afa22853785849e65797e085a1.png",
		"id": 332920,
		"max_time": 120,
		"pk_id": 332920,
		"room_id": 982077,
		"time": 120,
		"title": "恭喜主播大乱斗胜利"
	}
}
```





### 活动相关(12)

#### GUARD_LOTTERY_START

房间内登船活动开启消息。



#### SCORE_CARD

分数卡。

```json
{
	"cmd": "SCORE_CARD",
	"data": {
		"start_time": 1552740607,
		"end_time": 1552740667,
		"now_time": 1552740607,
		"gift_id": 30081,
		"uid": 2232963,
		"ruid": 1577804,
		"id": 66
	}
}
```



#### RAFFLE_START

房间内活动抽奖开始消息，包括摩天大楼、周星、BLS道具。

```json
{
	"cmd": "RAFFLE_START",
	"data": {
		"id": "159459",
		"dtime": 180,
		"msg": {
			"cmd": "SYS_MSG",
			"msg": " :?主播:? 一颗奶糖不加糖:? 充满了欧气能量，充能炮发射！点击前往TA的房间去抽奖吧",
			"msg_text": " :?主播:? 一颗奶糖不加糖:? 充满了欧气能量，充能炮发射！点击前往TA的房间去抽奖吧",
			"msg_common": "全区广播：<% %>主播<% 一颗奶糖不加糖%> 充满了欧气能量，充能炮发射！点击前往TA的房间去抽奖吧",
			"msg_self": "全区广播：<% %>主播<% 一颗奶糖不加糖%> 充满了欧气能量，充能炮发射！快来抽奖吧",
			"rep": 1,
			"styleType": 2,
			"url": "http:\/\/live.bilibili.com\/418",
			"roomid": 418,
			"real_roomid": 3514612,
			"rnd": 49070210,
			"broadcast_type": 0
		},
		"raffleId": 159459,
		"payflow_id": 1,
		"title": "充能炮抽奖",
		"type": "GIFT_30033",
		"from": "一颗奶糖不加糖",
		"from_user": {
			"uname": "一颗奶糖不加糖",
			"face": "http:\/\/i0.hdslb.com\/bfs\/face\/a2ead6078e39d569b067453c06f21808df26d133.jpg"
		},
		"time": 180,
		"max_time": 180,
		"time_wait": 120,
		"asset_animation_pic": "http:\/\/i0.hdslb.com\/bfs\/live\/84a2e86014546892853656992f5ebfc8baf4288b.gif",
		"asset_tips_pic": "http:\/\/s1.hdslb.com\/bfs\/live\/1364c63088b4eb5e31d575e5b4095c904e48f017.png",
		"sender_type": 0
	}
}
```



#### RAFFLE_END

房间内活动抽奖结束消息。

```json
{
	"cmd": "RAFFLE_END",
	"data": {
		"id": "159459",
		"uname": "-迓-",
		"sname": "一颗奶糖不加糖",
		"giftName": "20x普通扭蛋币",
		"mobileTips": "恭喜 -迓- 获得20x普通扭蛋币",
		"raffleId": "159459",
		"type": "GIFT_30033",
		"from": "一颗奶糖不加糖",
		"fromFace": "http:\/\/i0.hdslb.com\/bfs\/face\/a2ead6078e39d569b067453c06f21808df26d133.jpg",
		"fromGiftId": 30033,
		"win": {
			"uname": "-迓-",
			"face": "http:\/\/i0.hdslb.com\/bfs\/face\/a8c50d9041683057acc097ba4dfaf17bce90de1e.jpg",
			"giftName": "普通扭蛋币",
			"giftId": "normal_capsule_coin",
			"giftNum": 20,
			"giftImage": "http:\/\/i0.hdslb.com\/bfs\/vc\/3985d5d504b7b6051dea5e264b6adf3466f37c01.png",
			"msg": "恭喜 <%-迓-%> 获得大奖 <%20x普通扭蛋币%>, 感谢 <%一颗奶糖不加糖%> 的赠送"
		}
	}
}
```



#### TV_START

房间内小电视抽奖开始消息。

```json
{
	"cmd": "TV_START",
	"data": {
		"id": "159458",
		"dtime": 180,
		"msg": {
			"cmd": "SYS_MSG",
			"msg": "正在精分的男子:? 送给:? 一颗奶糖不加糖:? 1个小电视飞船，点击前往TA的房间去抽奖吧",
			"msg_text": "正在精分的男子:? 送给:? 一颗奶糖不加糖:? 1个小电视飞船，点击前往TA的房间去抽奖吧",
			"msg_common": "全区广播：<%正在精分的男子%> 送给<% 一颗奶糖不加糖%> 1个小电视飞船，点击前往TA的房间去抽奖吧",
			"msg_self": "全区广播：<%正在精分的男子%> 送给<% 一颗奶糖不加糖%> 1个小电视飞船，快来抽奖吧",
			"rep": 1,
			"styleType": 2,
			"url": "http:\/\/live.bilibili.com\/418",
			"roomid": 418,
			"real_roomid": 3514612,
			"rnd": 1540274096,
			"broadcast_type": 0
		},
		"raffleId": 159458,
		"payflow_id": "1540274667111000001",
		"title": "小电视飞船抽奖",
		"type": "small_tv",
		"from": "正在精分的男子",
		"from_user": {
			"uname": "正在精分的男子",
			"face": "http:\/\/static.hdslb.com\/images\/member\/noface.gif"
		},
		"time": 180,
		"max_time": 180,
		"time_wait": 120,
		"asset_animation_pic": "http:\/\/i0.hdslb.com\/bfs\/live\/746a8db0702740ec63106581825667ae525bb11a.gif",
		"asset_tips_pic": "http:\/\/s1.hdslb.com\/bfs\/live\/ac43b069bec53d303a9a1e0c4e90ccd1213d1b6b.png",
		"sender_type": 0
	}
}
```



#### TV_END

房间内小电视抽奖结束消息。

```json
{
	"cmd": "TV_END",
	"data": {
		"id": "159458",
		"uname": "最完美历史酱",
		"sname": "正在精分的男子",
		"giftName": "100000x银瓜子",
		"mobileTips": "恭喜 最完美历史酱 获得100000x银瓜子",
		"raffleId": "159458",
		"type": "small_tv",
		"from": "正在精分的男子",
		"fromFace": "http:\/\/static.hdslb.com\/images\/member\/noface.gif",
		"fromGiftId": 25,
		"win": {
			"uname": "最完美历史酱",
			"face": "http:\/\/i1.hdslb.com\/bfs\/face\/688f96dffb077061f54e323efc12c103f9ad94cb.jpg",
			"giftName": "银瓜子",
			"giftId": "silver",
			"giftNum": 100000,
			"giftImage": "http:\/\/s1.hdslb.com\/bfs\/live\/00d768b444f1e1197312e57531325cde66bf0556.png",
			"msg": "恭喜 <%最完美历史酱%> 获得大奖 <%100000x银瓜子%>, 感谢 <%正在精分的男子%> 的赠送"
		}
	}
}
```



#### SPECIAL_GIFT

房间内节奏风暴开始与结束消息。



#### WIN_ACTIVITY

实物抽奖进度。

```json
{
	"cmd": "WIN_ACTIVITY",
	"number": 2
}
```



#### WISH_BOTTLE

房间内许愿瓶更新信息。

```json
{
	"cmd": "WISH_BOTTLE",
	"data": {
		"action": "update",
		"id": 49471,
		"wish": {
			"id": 49471,
			"uid": 14781725,
			"type": 1,
			"type_id": 30046,
			"wish_limit": 99999,
			"wish_progress": 10717,
			"status": 1,
			"content": "打鱼",
			"ctime": "2018-09-06 23:33:17",
			"count_map": [1,
			10,
			100]
		}
	}
}
```



#### LOL_ACTIVITY

房间内LOL竞猜（仅限房间7734200）。

```json
{
	"cmd": "LOL_ACTIVITY",
	"data": {
		"action": "vote_begin",
		"match_id": 4193,
		"timestamp": 1540627015,
		"guess_info": null
	}
}
{
	"cmd": "LOL_ACTIVITY",
	"data": {
		"action": "vote_end",
		"match_id": 4193,
		"timestamp": 1540630432,
		"guess_info": null
	}
}
```



#### HOUR_RANK_AWARDS

房间内小时榜榜首信息。

```json
{
	"cmd": "HOUR_RANK_AWARDS",
	"action": "all_top3",
	"data": {
		"roomid": 3742025,
		"ruid": 93841264,
		"uname": "姊﹂啋涓夌敓姊?",
		"face": "http:\/\/i2.hdslb.com\/bfs\/face\/b30b947c58c6b1011136621a3ab54c0031c920fe.jpg",
		"rank_desc": "http:\/\/i0.hdslb.com\/bfs\/vc\/fc85c1f7b815ea3422d4f8c0f3ed9e96c2a0a77a.png",
		"content": "鎭枩涓绘挱鑾峰緱\n瓒呬汉姘旀帹鑽愬鍔憋紒",
		"life_cycle": 3
	}
}
```



#### ACTIVITY_MATCH_GIFT

房间内LOL应援礼物消息（仅限房间7734200）。

```json
{
	"cmd": "ACTIVITY_MATCH_GIFT",
	"data": {
		"action": "match_ing",
		"status": 1,
		"detail": {
			"match_id": 1099,
			"home": {
				"team_name": "FW",
				"url": "http:\/\/i0.hdslb.com\/bfs\/vc\/6a2be591bc445d6f3c01005f1f3de7d6fba744a6.png",
				"ratio": "66.67",
				"score": 1,
				"gift_info": [{
						"gift_id": 30227,
						"gift_name": "FW加油！"
					}
				]
			},
			"visit": {
				"team_name": "VEGA",
				"url": "http:\/\/i0.hdslb.com\/bfs\/vc\/e75be97576144fe5f0f8c79e97bf9c390c71a143.png",
				"ratio": "33.33",
				"score": 1,
				"gift_info": [{
						"gift_id": 30219,
						"gift_name": "Vega加油！"
					}
				]
			}
		}
	}
}
```



### 其它无处理函数的消息(9)

#### USER_TOAST_MSG

房间内用户购买舰队服务时。

```json
{
	"cmd": "USER_TOAST_MSG",
	"data": {
		"op_type": 1,
		"uid": 24655455,
		"username": "到底有莫得感情",
		"guard_level": 3,
		"is_show": 0
	}
}
```



#### room_admin_entrance

房间内新增房管。

```json
{
	"cmd": "room_admin_entrance",
	"msg": "系统提示：你已被主播设为房管",
	"uid": 157642459
}
```



#### new_anchor_reward

```json
{
	"cmd": "new_anchor_reward",
	"reward_id": 1,
	"roomid": 10633444,
	"uid": 111977978
}
```



#### DANMU_MSG:4:0:2:2:2:0

特殊时期客户端丢弃弹幕消息的作用。

见公告：[弹幕系统技术升级通知](https://t.bilibili.com/258961230595770167)



#### BOX_ACTIVITY_START

添加日期：2019/06/19

房间内消息，实物抽奖开始。

```json
{
	"cmd": "BOX_ACTIVITY_START",
	"aid": 373
}
```



#### ROOM_CHANGE

添加日期：2019/06/20

房间内消息，在更改名字或分区后推送。

```json
{
	"cmd": "ROOM_CHANGE",
	"data": {
		"title": "小仙菇的日常肝书|自习室|北大|轻音乐",
		"area_id": 259,
		"parent_area_id": 7,
		"area_name": "考生加油站",
		"parent_area_name": "哔考"
	}
}
```



#### ACTIVITY_BANNER_UPDATE

添加日期：2019/06/20

房间内消息，大乱斗活动条更新。

```json
{
	"cmd": "ACTIVITY_BANNER_UPDATE",
	"data": {
		"id": 299,
		"title": "初阶守护者x2星",
		"cover": "http://i0.hdslb.com/bfs/live/dfd84488f4cc342ca51819a9adcb61ba8bbccc07.png",
		"background": "http://i0.hdslb.com/bfs/live/4807ff55e5b93980b4811ed6c0da69d8fc757a0c.png",
		"jump_url": "https://live.bilibili.com/p/html/live-app-battle/u-anchor.html?is_live_half_webview=1&hybrid_biz=live-app-battle-u-anchor&hybrid_half_ui=1,5,272,320,0,0,30,0,8;2,5,272,320,0,0,30,0,8;3,5,272,320,0,0,30,0,8;4,5,272,320,0,0,30,0,8;5,5,272,320,0,0,30,0,8;6,5,272,320,0,0,30,0,8;7,5,272,320,0,0,30,0,8;8,5,272,320,0,0,30,0,8&battleAnchorId=421470960",
		"title_color": "#ffffff",
		"closeable": 0,
		"banner_type": 1,
		"weight": 10,
		"web_text": "",
		"web_cover": ""
	}
}
```



#### ACTIVITY_BANNER_RED_NOTICE_CLOSE

添加日期：2019/06/25

房间内消息，大乱斗活动条关闭。

```json
{
	"cmd": "ACTIVITY_BANNER_RED_NOTICE_CLOSE",
	"data": {
		"id": 297,
		"type": "revenue_banner"
	}
}
```



#### DAILY_QUEST_NEWDAY

添加日期：2019/06/26

房间内消息，作用不明。

```json
{
	"cmd": "DAILY_QUEST_NEWDAY",
	"data": {}
}
```



## 失效指令(7)

在以上JS文件中找不到的指令。



### ACTIVITY_EVENT

房间内活动信息。目前为BLS预热活动。

```json
{
	"cmd": "ACTIVITY_EVENT",
	"data": {
		"keyword": "bls_winter_2018",
		"type": "charge",
		"limit": 500000,
		"progress": 48280
	}
}
```



### COMBO_END

```json
{
	"cmd": "COMBO_END",
	"data": {
		"uname": "进击的拓海",
		"r_uname": "一米八的坤儿",
		"combo_num": 1,
		"price": 100,
		"gift_name": "吃瓜",
		"gift_id": 20004,
		"start_time": 1540277390,
		"end_time": 1540277390,
		"guard_level": 0
	}
}
```



### LOTTERY_START

旧的房间内抽奖消息。

```json
{
	"cmd": "LOTTERY_START",
	"data": {
		"id": 531692,
		"roomid": 425315,
		"message": "澶お澶お澶簡 鍦ㄣ€?25315銆戣喘涔颁簡鎬荤潱锛岃鍓嶅線鎶藉",
		"type": "guard",
		"privilege_type": 1,
		"link": "https:\/\/live.bilibili.com\/425315",
		"payflow_id": "web_d0de2bfd7e7dde8b86_201810",
		"lottery": {
			"id": 531692,
			"sender": {
				"uid": 738417,
				"uname": "澶お澶お澶簡",
				"face": "http:\/\/i1.hdslb.com\/bfs\/face\/f8eeeaf0e46329359eb6cfb8960a92ada9f0ea6f.jpg"
			},
			"keyword": "guard",
			"privilege_type": 1,
			"time": 86400,
			"status": 1,
			"mobile_display_mode": 2,
			"mobile_static_asset": "",
			"mobile_animation_asset": ""
		}
	}
}
```



### SYS_GIFT

旧的全区礼物赠送消息。

```json
{
	"cmd": "SYS_GIFT",
	"msg": "Eli大老师:? 在直播间 :?5540554:? 使用了 20 倍节奏风暴，大家快去跟风领取奖励吧！",
	"tips": "【Eli大老师】在直播间【5540554】使用了 20 倍节奏风暴，大家快去跟风领取奖励吧！",
	"msg_text": "【Eli大老师】在直播间【5540554】使用了 20 倍节奏风暴，大家快去跟风领取奖励吧！",
	"giftId": 39,
	"msgTips": 1,
	"url": "http:\/\/live.bilibili.com\/5540554",
	"roomid": 5540554,
	"rnd": 1540374323
}
```



### SYS_MSG

广播的抽奖信息。



### GUARD_MSG

广播的总督上船消息。



### WELCOME_ACTIVITY

房间欢迎特效。

```json
{
	"cmd": "WELCOME_ACTIVITY",
	"data": {
		"uid": 12298306,
		"uname": "你的丸子_",
		"type": "comingForYou",
		"display_mode": 1
	}
}
```


