#pragma once
#include <set>
#include "transport/asioclient.h"
#include "BilibiliStruct.h"

class source_notice {
private:
	typedef std::function<void(std::shared_ptr<BILI_LOTTERYDATA>)> event_act;

public:
	source_notice();
	~source_notice();

public:
	/**
	 * @brief 连接消息服务器
	 *
	 * 服务器信息由文件导入
	 *
	 * @return
	 *   返回0
	 */
	int start();
	/**
	 * @brief 断开消息服务器
	 *
	 * @return
	 *   返回0
	 */
	int stop();
	/**
	 * @brief 显示当前连接的消息服务器的数量
	 */
	void show_stat();
	/**
	 * @brief 设置消息回调
	 *
	 * @param h     消息回调句柄
	 *
	 */
	void set_event_act(event_act h) {
		event_act_ = h;
	}

private:
	/**
	 * @brief asio/tcp的错误消息 一般由于连接断开
	 *
	 * @param id    当前连接的标签
	 * @param ec    错误码
	 *
	 */
	void on_error(const unsigned id, const boost::system::error_code ec);
	/**
	 * @brief asio/tcp的连接成功消息 目前不需要发送数据包
	 *
	 * @param c     当前连接的session
	 *
	 */
	void on_open(context_info* c);
    /**
     * @brief asio/tcp的连接心跳
     *
     * @param c     当前连接的session
     *
     */
    void on_heart(context_info* c);
	/**
	 * @brief asio/tcp的协议头处理回调
	 *
	 * @param c     当前连接的session
	 * @param len   本次接收数据的长度
	 *
	 * @return
	 *   返回整个数据包的长度 包含协议头
	 */
	size_t on_header(context_info* c, const int len);
	/**
	 * @brief asio/tcp的载荷处理回调
	 *
	 * @param c     当前连接的session
	 * @param len   本次接收数据的长度 不包含协议头长度
	 *
	 * @return
	 *   返回值随意
	 */
	size_t on_payload(context_info* c, const int len);

private:
	/**
	 * @brief asio/tcp实例
	 */
	asioclient asioclient_;
	/**
	 * @brief 消息回调函数指针
	 */
	event_act event_act_;
	/**
	 * @brief 运行状态
	 */
	bool is_stop_;
};
