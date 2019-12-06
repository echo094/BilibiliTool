/*
1 为每一个用户创建一个CURL句柄，并在登录成功或导入后将Cookie信息
  更新到CURL句柄，在之后的HTTP请求中无需再次导入Cookie。
2 导出账号信息时自动从CURL句柄获取最新的Cookie信息。

关于配置信息的说明:
conf_coin    硬币兑换
conf_lottery 各种抽奖
conf_storm   风暴
conf_guard   亲密度
conf_pk      大乱斗抽奖
*/
#pragma once
#include <map>
#include <memory>
#include <queue>
#include <string>
#include <boost/asio.hpp>
#include "rapidjson/document.h"
#include "utility/httpex.h"
#include "BilibiliStruct.h"


/**
 * @brief 任务的待执行队列 开始时间早的靠前
 */
struct task_cmp {
	bool operator ()(
		const std::shared_ptr<BILI_LOTTERYDATA> &a, 
		const std::shared_ptr<BILI_LOTTERYDATA> &b)
	{
		if (a->time_get == b->time_get) {
			return a->rrid > b->rrid;
		}
		return a->time_get > b->time_get;
	}
};

class user_info {
public:
	bool islogin;
	// 配置文件中的序号
	int fileid;
	unsigned uid;
	// 配置信息
	unsigned conf_coin;
	unsigned conf_gift;
	unsigned conf_storm;
	unsigned conf_guard;
	unsigned conf_pk;
	unsigned conf_danmu;
	unsigned conf_anchor;
	std::string account;
	std::string password;
	std::string tokena, tokenr;

	std::string tokenjct;
	std::string visitid;

	// 心跳倒计时
	int heart_deadline;
	// 瓜子倒计时
	int silver_deadline;
	int silver_minute;
	int silver_amount;
	long long int silver_start;
	long long int silver_end;

public:
	// 两个连接 一个用于网页端 一个用于手机端
	CURL *curlweb, *curlapp;
	unique_ptr<toollib::CHTTPPack> httpweb, httpapp;
	// 手机ID
	std::string phoneDeviceName;
	std::string phoneDevicePlatform;
	std::string phoneBuvid;
	std::string phoneDeviceID;
	std::string phoneDisplayID;
	
public:
	explicit user_info();
	~user_info();

public:
	// 从文件导入指定账户 出错时向外抛出异常 
	void ReadFileAccount(const std::string &key, const rapidjson::Value& data, int index);
	// 将账户信息导出到文件
	void WriteFileAccount(const std::string key, rapidjson::Document& doc);
	// 检测账户是否被封禁
	bool CheckBanned(const std::string &msg);
	// 账户被封禁
	void SetBanned();
	// 生成随机访问ID
	void GetVisitID();
	// 获取 Cookie bili_jct 的值
	bool GetToken();
	// 获取 Cookie bili_jct 的失效时间
	int GetExpiredTime();
	/**
	 * @brief 将抽奖任务通过IO线程投递到任务队列
	 *
	 * @param lot   抽奖信息
	 *
	 */
	void post_task(std::shared_ptr<BILI_LOTTERYDATA> lot);
	/**
	 * @brief 通过IO线程清空抽奖列表
	 */
	void clear_task();

private:
	/**
	 * @brief 设置定时器 等待时间1秒
	 */
	void start_timer();
	/**
	 * @brief 定时器回调函数
	 *
	 * @param ec    定时器错误码
	 *
	 */
	void on_timer(boost::system::error_code ec);
	/**
	 * @brief 执行抽奖
	 *
	 * @param data  抽奖信息
	 *
	 * @return 抽奖API的返回状态
	 *
	 */
	BILIRET do_task(const std::shared_ptr<BILI_LOTTERYDATA> &data, long long curtime);
	/**
	 * @brief 访问房间
	 *
	 * 180秒内不重复访问
	 *
	 * @param room      房间号
	 * @param curtime   当前时间 ms
	 */
	void do_visit(unsigned room, long long curtime);

private:
	/**
	 * @brief 当前用户的IO
	 */
	boost::asio::io_context ioc_;
	/**
	 * @brief 定时器 定期检查事件队列
	 */
	boost::asio::deadline_timer timer_;
	/**
	 * @brief Maintenance worker
	 */
	std::shared_ptr<boost::asio::io_context::work> worker_;
	/**
	 * @brief IO工作线程
	 */
	std::shared_ptr<std::thread> thread_;
	/**
	 * @brief 任务队列
	 */
	std::priority_queue<
		std::shared_ptr<BILI_LOTTERYDATA>,
		std::vector<std::shared_ptr<BILI_LOTTERYDATA>>,
		task_cmp
	> tasks_;
	/**
	 * @brief 直播间访问历史
	 */
	std::map<unsigned, long long> his_visit_;
};
