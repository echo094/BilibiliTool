﻿#pragma once
#include <string>
#include "BilibiliStruct.h"
#include "user_info.h"

namespace apibl {

	/* 网页端 API*/

	/* 网页端 非直播站 API*/

	// 获取登录硬币
	BILIRET APIWebGetCoin(const std::shared_ptr<user_info> &user);
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

	/* 网页端 常规 API*/

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
	// 查询扭蛋币数量
	BILIRET APIWebv1CapsuleCheck(const std::shared_ptr<user_info> &user);

	// 领取每日礼物
	BILIRET APIWebv2GiftDaily(const std::shared_ptr<user_info> &user);
    /**
     * @brief 查询背包道具
     *
     * 更新日期 02/12/2020
     *
     * @param user  用户IO
     * @param flag  是否输出
     *
     * @return
     *   返回抽奖参与情况
     */
    BILIRET APIWebv1GiftBag(
       const std::shared_ptr<user_info>& user,
       unsigned flag = 0
    );
    /**
     * @brief 赠送背包中的礼物礼物
     *
     * 更新日期 02/12/2020
     *
     * @param user           用户IO
     * @param ruid           主播uid
     * @param room           主播原始房间号
     * @param gift_id    礼物编号
     * @param gift_num  礼物数量
     * @param bag_id      礼物包裹号
     *
     * @return
     *   返回抽奖参与情况
     */
    BILIRET APIWebv2GiftSend(
        const std::shared_ptr<user_info>& user,
        unsigned ruid,
        unsigned room,
        unsigned gift_id,
        unsigned gift_num,
        unsigned bag_id
    );
	/**
	 * @brief 进入直播间 留下历史记录
	 *
	 * 更新日期 11/17/2019
	 *
	 * @param user  用户IO
	 * @param room  直播间编号
	 *
	 * @return
	 *   返回抽奖参与情况
	 */
	BILIRET APIWebv1RoomEntry(const user_info *user, unsigned room);
	/**
	 * @brief 参加风暴抽奖
	 *
	 * 更新日期 11/17/2019
	 *
	 * 在未被拉黑时 一个抽奖可参与多次 直到抽奖结束或中奖
	 * 领取成功时返回码为 0
	 * 需要验证码时返回码为 429 待测试
	 * 领取失败时返回码为 400 有多种情况
	 * - 你错过了奖励，下次要更快一点哦~
	 * - 节奏风暴抽奖过期
	 * - 访问被拒绝
	 *
	 * @param user  用户IO
	 * @param data  抽奖信息
	 * @param code  验证码答案
	 * @param token 验证码ID
	 *
	 * @return
	 *   返回抽奖参与情况
	 */
	BILIRET APIWebv1StormJoin(
		user_info *user,
		std::shared_ptr<BILI_LOTTERYDATA> data,
		std::string code,
		std::string token
	);
	/**
	 * @brief 参加礼物抽奖
	 *
	 * 更新日期 11/17/2019 <br>
	 * <br>
	 * 返回值 <br>
	 * -     0: 成功
	 * -  -401: 请先登录哦
	 * -  -403: 您已参加抽奖~
	 * -  -405: 奖品都被领完啦！下次下手快点哦~
	 * -  -500: 系统繁忙，请稍后再试
	 * -   503: 请求太多啦!
	 * -  -509: 请求过于频繁，请稍后再试
	 *
	 * @param user  用户IO
	 * @param data  抽奖信息
	 *
	 * @return
	 *   返回抽奖参与情况
	 */
	BILIRET APIWebv5SmalltvJoin(
		user_info *user,
		std::shared_ptr<BILI_LOTTERYDATA> data
	);
	/**
	 * @brief 参加守护抽奖
	 *
	 * 更新日期 11/17/2019
	 *
	 * @param user  用户IO
	 * @param data  抽奖信息
	 *
	 * @return
	 *   返回抽奖参与情况
	 */
	BILIRET APIWebv3GuardJoin(
		user_info *user,
		std::shared_ptr<BILI_LOTTERYDATA> data
	);
	/**
	 * @brief 参加PK抽奖
	 *
	 * 更新日期 11/17/2019
	 *
	 * @param user  用户IO
	 * @param data  抽奖信息
	 *
	 * @return
	 *   返回抽奖参与情况
	 */
	BILIRET APIWebv2PKJoin(
		user_info *user,
		std::shared_ptr<BILI_LOTTERYDATA> data
	);
	/**
	 * @brief 参加弹幕抽奖
	 *
	 * 更新日期 11/17/2019
	 *
	 * @param user  用户IO
	 * @param data  抽奖信息
	 *
	 * @return
	 *   返回抽奖参与情况
	 */
	BILIRET APIWebv1DanmuJoin(
		user_info *user,
		std::shared_ptr<BILI_LOTTERYDATA> data
	);
	/**
	 * @brief 参加天选抽奖
	 *
	 * 更新日期 11/17/2019
	 *
	 * 有两种形式：
	 * - 关注 参数有 follow （目前参数无效）
	 * - 送礼 参数有 gift_id|gift_num
	 *
	 * @param user  用户IO
	 * @param data  抽奖信息
	 *
	 * @return
	 *   返回抽奖参与情况
	 */
	BILIRET APIWebv1AnchorJoin(
		user_info *user,
		std::shared_ptr<BILI_LOTTERYDATA> data
	);

	/* 安卓端 API*/

	/**
	 * @brief 获取RSA密钥
	 *
	 * 更新日期 12/06/2019 <br>
	 * <br>
	 * 收到密钥后将传入的密码加密 <br>
	 *
	 * @param user  用户IO
	 * @param psd   密码
	 *
	 * @return
	 *   返回码
	 */
	BILIRET APIAndGetKey(
		const std::shared_ptr<user_info> &user, 
		std::string &psd
	);

	/**
	 * @brief 登录v3
	 *
	 * 更新日期 12/06/2019 <br>
	 * <br>
	 * 返回值 <br>
	 * -     0: 成功
	 * -    -3: API sign invalid
	 * -  -105: 验证码错误
	 * -  -629: 账号或密码错误
	 * - -2001: 校验失败,请重新提交
	 *
	 *
	 * @param user       用户IO
	 * @param username   登录名
	 * @param password   加密后的密码
	 * @param challenge  验证码ID  不需要时留空
	 * @param validate   验证码 不需要时留空
	 *
	 * @return
	 *   返回码
	 */
	BILIRET APIAndv3Login(
		std::shared_ptr<user_info>& user,
		std::string username,
		std::string password,
		std::string challenge,
		std::string validate
	);

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

	/* 安卓端 v4 API*/

	// 通告礼物抽奖
	BILIRET APIAndv4SmallTV(
		std::shared_ptr<user_info> &user,
		std::shared_ptr<BILI_LOTTERYDATA> data
	);

}
