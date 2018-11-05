# 弹幕指令说明



## 有效指令

采集自：https://s1.hdslb.com/bfs/static/blive/blfe-live-room/static/js/2.34e62d43c14ad56704c6.js



### 房间配置相关(16)

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



#### ROOM_SHIELD

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



#### ROOM_ADMINS

无需响应，设置管理员。

```json
{
	"cmd": "ROOM_ADMINS",
	"uids": [160720921]
}
```



### 消息相关(9)

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



#### WELCOME_ACTIVITY

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



### PK模式相关(15)

#### PK_INVITE_INIT

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



#### PK_INVITE_FAIL

无需响应。

```json
{
	"cmd": "PK_INVITE_FAIL",
	"pk_invite_status": 1100,
	"invite_id": 6707,
	"roomid": "8260376"
}
```



#### PK_INVITE_CANCEL

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



#### PK_INVITE_SWITCH_OPEN

无需响应。

```json
{
	"cmd": "PK_INVITE_SWITCH_OPEN",
	"roomid": 1542742
}
```



#### PK_INVITE_SWITCH_CLOSE

无需响应。

```json
{
	"cmd": "PK_INVITE_SWITCH_CLOSE",
	"roomid": 81697
}
```



#### PK_INVITE_REFUSE

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



#### PK_CLICK_AGAIN

无需响应。

```json
{
	"pk_status": 400,
	"pk_id": 170781,
	"cmd": "PK_CLICK_AGAIN",
	"roomid": 1445045
}
```



### 活动相关(10)

#### GUARD_LOTTERY_START

房间内登船活动开启消息。



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



#### ACTIVITY_EVENT

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



## 失效指令

在以上JS文件中找不到的指令。



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



### ENTRY_EFFECT

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

