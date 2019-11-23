#pragma once
#include <fstream>
#include "source/source_base.h"
#include "transport/wsclientv2.h"

class source_dmws: 
	public source_base,
	public WsClientV2 {
public:
	source_dmws();
	~source_dmws();

protected:
	/**
	 * @brief 开启定时器 倒计时30秒
	 */
	void start_timer();
	/**
	 * @brief IO线程 发送心跳数据
	 *
	 * @param ec    错误码
	 *
	 */
	void on_timer(boost::system::error_code ec);

protected:
	/**
	 * @brief IO线程 已成功建立WS连接
	 *
	 * 发送认证数据包
	 *
	 * @param c     当前会话实例
	 *
	 */
	void on_open(std::shared_ptr<session_ws> c) override;
	/**
	 * @brief IO线程 意外断开 需要重新连接
	 *
	 * @param label 当前会话标签
	 *
	 */
	void on_fail(unsigned label) override;
	/**
	 * @brief IO线程 主动断开 不需要重新连接
	 *
	 * @param label 当前会话标签
	 *
	 */
	void on_close(unsigned label) override;
	/**
	 * @brief IO线程 收到数据
	 *
	 * @param label 当前会话标签
	 * @param len   数据长度
	 *
	 */
	void on_message(std::shared_ptr<session_ws> c, size_t len) override;

protected:
	void process_data(const char *buff,
		const unsigned ilen,
		const unsigned id,
		const unsigned opt,
		const unsigned type
	);

public:
	// 开启心跳定时器
	int start() override;
	// 关闭心跳定时器 断开所有连接
	int stop() override;
	// 连接一个房间
	int add_context(const unsigned id, const ROOM_INFO& info) override;
	// 断开特定房间
	int del_context(const unsigned id) override;
	// 清理房间
	int clean_context(std::set<unsigned> &nlist) override;
	// 显示当前连接数
	void show_stat() override;

private:
	int SendConnectionInfo(std::shared_ptr<session_ws> c);
	int SendHeartInfo(std::shared_ptr<session_ws> c);

private:
	// Heart Timer
	boost::asio::deadline_timer heart_timer_;
	bool _isworking;
};
