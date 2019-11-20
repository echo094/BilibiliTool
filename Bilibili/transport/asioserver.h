#pragma once
#include <memory>
#include <set>
#include <string>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

class tcp_connection
	: public std::enable_shared_from_this<tcp_connection>
{
public:
	typedef std::shared_ptr<tcp_connection> pointer;

	/**
	 * @brief 构造函数 将套接字移动给成员变量
	 */
	tcp_connection(tcp::socket socket):
		socket_(std::move(socket)),
		addr(socket_.remote_endpoint().address().to_string()),
		port(socket_.remote_endpoint().port()) {
	}

public:
	/**
	 * @brief socket套接字
	 */
	tcp::socket socket_;
	/**
	 * @brief 客户端IP
	 */
	std::string addr;
	/**
	 * @brief 客户端端口
	 */
	unsigned port;
	/**
	 * @brief 接收缓冲区长度
	 */
	enum { max_length = 1024 };
	/**
	 * @brief 接收缓冲区
	 */
	char data_[max_length];
	/**
	 * @brief 待发送的数据
	 */
	std::shared_ptr<std::string> msg;
};

class asioserver
{
public:
	asioserver(unsigned port):
		io_context_(),
		acceptor_(io_context_, tcp::endpoint(tcp::v4(), port)) {
	}

public:
	void init();
	void deinit();
	void broadcast(std::shared_ptr<std::string> msg);

private:
	void do_accept();
	void do_deinit();
	void do_read(tcp_connection::pointer s);
	void do_broadcast(std::shared_ptr<std::string> msg);

private:
	// IO serverce
	boost::asio::io_context io_context_;
	// Accepter
	tcp::acceptor acceptor_;
	// Maintenance worker
	std::shared_ptr<boost::asio::io_context::work> pwork_;
	// Worker thread
	std::shared_ptr<std::thread> io_thread_;
	// Sessions
	std::set<tcp_connection::pointer> sessions_;
};
