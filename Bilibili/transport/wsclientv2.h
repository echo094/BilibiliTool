#pragma once
#include <set>
#include <memory>
#include <string>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket/ssl.hpp>

namespace beast = boost::beast;

#define USE_WSS

class session_ws
	: public std::enable_shared_from_this<session_ws>
{
public:

#ifdef USE_WSS
	session_ws(
		boost::asio::io_context& ioc,
		boost::asio::ssl::context& ctx,
		char const* host,
		char const* param,
		unsigned label,
		std::string key
	): ws_(ioc, ctx)
		, resolver_(ioc)
		, host_(host)
		, param_(param)
		, label_(label)
		, key_(key) {
		ws_.binary(true);
	}
#else
	session_ws(
		boost::asio::io_context& ioc,
		char const* host,
		char const* param,
		unsigned label,
		std::string key
	) : ws_(ioc)
		, resolver_(ioc)
		, host_(host)
		, param_(param)
		, label_(label)
		, key_(key) {
		ws_.binary(true);
	}
#endif

public:
	/**
	 * @brief WSS socket
	 */
#ifdef USE_WSS
	beast::websocket::stream<beast::ssl_stream<
		beast::tcp_stream> > ws_;
#else
	beast::websocket::stream<
		beast::tcp_stream> ws_;
#endif
	/**
	 * @brief 域名解析
	 */
	boost::asio::ip::tcp::resolver resolver_;
	/**
	 * @brief 服务器host 用于SSL握手
	 */
	std::string host_;
	/**
	 * @brief 服务器index 用于SSL握手
	 */
	std::string param_;
	/**
	 * @brief 接收数据缓存
	 */
	boost::beast::flat_buffer buffer_;
	/**
	 * @brief 发送数据缓存
	 */
	std::shared_ptr<std::string> msg_;
	/**
	 * @brief 标签
	 */
	unsigned label_;
	/**
	 * @brief 连接房间时的key
	 */
	std::string key_;
};

/**
 * @brief 基于Beast的WSS客户端
 */
class WsClientV2
	: public std::enable_shared_from_this<WsClientV2> 
{
public:
	explicit WsClientV2();
	virtual ~WsClientV2();

public:

	/**
	 * @brief 投递到IO线程 连接服务器
	 */
	void connect(
		unsigned id,
		char const* host,
		char const* port,
		char const* param,
		const std::string key);

	/**
	 * @brief 投递到IO线程 关闭指定连接
	 */
	void close_spec(unsigned id);

	/**
	 * @brief 投递到IO线程 关闭所有连接
	 */
	void close_all();

protected:
	/**
	 * @brief IO线程 域名解析的回调 进行TCP连接
	 *
	 * @param c       当前会话实例
	 * @param results 域名解析结果
	 *
	 */
	void on_resolve(
		std::shared_ptr<session_ws> c,
		boost::asio::ip::tcp::resolver::results_type results);

	/**
	 * @brief IO线程 建立TCP连接的回调 进行WS握手
	 *
	 * SSL在连接后需要进行SSL握手
	 *
	 * @param c     当前会话实例
	 * @param ec    错误码
	 *
	 */
	void on_connect(
		std::shared_ptr<session_ws> c,
		boost::system::error_code ec,
		boost::asio::ip::tcp::resolver::results_type::endpoint_type);

	/**
	 * @brief IO线程 进行WS握手
	 *
	 * 连接成功后将session添加到map，
	 * 投递read请求，并调用on_open函数。
	 *
	 * @param c     当前会话实例
	 *
	 */
	void do_handshake(
		std::shared_ptr<session_ws> c);

	/**
	 * @brief 投递到IO线程 投递发送请求
	 *
	 * @param c     当前会话实例
	 * @param msg   待发送的数据 二进制模式
	 *
	 */
	void do_write(
		std::shared_ptr<session_ws> c,
		std::shared_ptr<std::string> msg);

	/**
	 * @brief 投递到IO线程 投递读取请求
	 *
	 * @param c     当前会话实例
	 *
	 */
	void do_read(
		std::shared_ptr<session_ws> c);

	/**
	 * @brief IO线程 读取数据
	 *
	 * @param c     当前会话实例
	 * @param ec    错误码
	 * @param bytes_transferred 收到的数据长度
	 *
	 */
	void on_read(
		std::shared_ptr<session_ws> c,
		boost::system::error_code ec,
		std::size_t bytes_transferred);

protected:
	/**
	 * @brief IO线程 已成功建立WS连接
	 *
	 * @param c     当前会话实例
	 *
	 */
	virtual void on_open(std::shared_ptr<session_ws> c) {}
	/**
	 * @brief IO线程 意外断开
	 *
	 * @param label 当前会话标签
	 *
	 */
	virtual void on_fail(unsigned label) {}
	/**
	 * @brief IO线程 主动断开
	 *
	 * @param label 当前会话标签
	 *
	 */
	virtual void on_close(unsigned label) {}
	/**
	 * @brief IO线程 收到数据
	 *
	 * @param label 当前会话标签
	 * @param len   数据长度
	 *
	 */
	virtual void on_message(
		std::shared_ptr<session_ws> c,
		size_t len) {}

protected:
	/**
	 * @brief I/O context
	 */
	boost::asio::io_context io_ctx_;
#ifdef USE_WSS
	/**
	 * @brief SSL context
	 */
	boost::asio::ssl::context ssl_ctx_;
#endif
	/**
	 * @brief Maintenance worker
	 */
	std::shared_ptr<boost::asio::io_context::work> pwork_;
	/**
	 * @brief Worker thread
	 */
	std::shared_ptr<std::thread> io_thread_;
	/**
	 * @brief 连接集合
	 */
	std::set<std::shared_ptr<session_ws>> sessions_;
};
