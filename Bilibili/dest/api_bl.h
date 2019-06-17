#pragma once
#include <string>
#include "BilibiliStruct.h"
#include "user_info.h"

namespace apibl {

	/* 网页端 API*/

	/* 网页端 非直播站 API*/

	// 获取登录硬币
	BILIRET APIWebGetCoin(const std::shared_ptr<user_info> &user);
	// 获取移动端登录验证码
	BILIRET APIWebGETLoginCaptcha(const std::shared_ptr<user_info> &user);
	// 获取必要的临时id
	BILIRET APIWebGetCaptchaKey(const std::shared_ptr<user_info> &user);

	/* 网页端 无版本 API*/

	// 直播经验心跳Web
	BILIRET APIWebOnlineHeart(const std::shared_ptr<user_info> &user);
	// 获取每日任务信息 并领取奖励
	BILIRET APIWebTaskInfo(const std::shared_ptr<user_info> &user);
	// 直播站签到
	BILIRET APIWebSign(const std::shared_ptr<user_info> &user);
	// 获取直播站主要信息
	BILIRET APIWebGetUserInfo(const std::shared_ptr<user_info> &user);

	/* 网页端 v1 API*/

	// 直播服务器信息获取
	BILIRET APIWebv1DanmuConf(
		CURL *pcurl,
		unsigned room,
		const std::string player,
		std::string &key
	);
	// 直播经验心跳日志1
	BILIRET APIWebv1HeartBeat(const std::shared_ptr<user_info> &user);
	// 领取双端观看奖励
	BILIRET APIWebv1TaskAward(const std::shared_ptr<user_info> &user);
	// 银瓜子换硬币新API
	BILIRET APIWebv1Silver2Coin(const std::shared_ptr<user_info> &user);
	// 进入房间历史记录
	BILIRET APIWebv1RoomEntry(const std::shared_ptr<user_info> &user, unsigned room);
	// 领取风暴
	BILIRET APIWebv1StormJoin(
		std::shared_ptr<user_info> &user, 
		std::shared_ptr<BILI_LOTTERYDATA> data, 
		std::string code, 
		std::string token
	);
	// 大乱斗抽奖
	BILIRET APIWebv1PKJOIN(
		std::shared_ptr<user_info> &user,
		std::shared_ptr<BILI_LOTTERYDATA> data
	);
	// 查询扭蛋币数量
	BILIRET APIWebv1CapsuleCheck(const std::shared_ptr<user_info> &user);

	/* 网页端 v2 API*/

	// 领取每日礼物
	BILIRET APIWebv2GiftDaily(const std::shared_ptr<user_info> &user);
	// 上船低保领取
	BILIRET APIWebv2LotteryJoin(
		std::shared_ptr<user_info> &user, 
		std::shared_ptr<BILI_LOTTERYDATA> data
	);
	// 查询背包道具
	BILIRET APIWebv2GiftBag(const std::shared_ptr<user_info> &user);

	/* 网页端 v3 API*/

	// 通告礼物抽奖
	BILIRET APIWebv3SmallTV(
		std::shared_ptr<user_info> &user,
		std::shared_ptr<BILI_LOTTERYDATA> data
	);

	/* 安卓端 API*/

	/* 安卓端 无版本 API*/

	// 移动端加密密钥
	BILIRET APIAndGetKey(const std::shared_ptr<user_info> &user, std::string &psd);

	/* 安卓端 v1 API*/

	// 进入房间历史记录
	BILIRET APIAndv1RoomEntry(const std::shared_ptr<user_info> &user, unsigned room);
	// 客户端经验心跳
	BILIRET APIAndv1Heart(const std::shared_ptr<user_info> &user);
	// 获取当前宝箱领取情况
	BILIRET APIAndv1SilverTask(std::shared_ptr<user_info> &user);
	// 领取银瓜子
	BILIRET APIAndv1SilverAward(std::shared_ptr<user_info> &user);
	// 领取风暴
	BILIRET APIAndv1StormJoin(
		std::shared_ptr<user_info> &user,
		std::shared_ptr<BILI_LOTTERYDATA> data
	);
	// 大乱斗抽奖
	BILIRET APIAndv1PKJOIN(
		std::shared_ptr<user_info> &user,
		std::shared_ptr<BILI_LOTTERYDATA> data
	);

	/* 安卓端 v2 API*/

	// 上船低保领取
	BILIRET APIAndv2LotteryJoin(
		std::shared_ptr<user_info> &user,
		std::shared_ptr<BILI_LOTTERYDATA> data
	);
	// 移动端登录接口
	BILIRET APIAndv2Login(
		std::shared_ptr<user_info> &user, 
		std::string username, 
		std::string password, 
		std::string captcha
	);

	/* 安卓端 v4 API*/

	// 通告礼物抽奖
	BILIRET APIAndv4SmallTV(
		std::shared_ptr<user_info> &user,
		std::shared_ptr<BILI_LOTTERYDATA> data
	);

}
