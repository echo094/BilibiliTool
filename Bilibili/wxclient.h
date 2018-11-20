#pragma once
#define WITH_TLS
#ifdef WITH_TLS
#include <websocketpp/config/asio_client.hpp>
#else
#include <websocketpp/config/asio_no_tls.hpp>
#endif
#include <websocketpp/client.hpp>

#include <websocketpp/common/thread.hpp>
#include <websocketpp/common/memory.hpp>

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <sstream>

#ifdef WITH_TLS
typedef websocketpp::client<websocketpp::config::asio_tls_client> client;
typedef websocketpp::lib::shared_ptr<boost::asio::ssl::context> context_ptr;
#else
typedef websocketpp::client<websocketpp::config::asio> client;
#endif

// 前置声明
class websocket_endpoint;

class connection_metadata {
public:
	typedef websocketpp::lib::shared_ptr<connection_metadata> ptr;

	connection_metadata(websocket_endpoint *ep, int id, websocketpp::connection_hdl hdl, std::string uri)
		: m_endpoint(ep)
		, m_id(id)
		, m_hdl(hdl)
		, m_status("Connecting")
		, m_uri(uri)
		, m_server("N/A") {
		// std::cout << "> Init metadata " << m_id << std::endl;
	}

	~connection_metadata() {
		// std::cout << "> Deinit metadata " << m_id << std::endl;
	}

	void on_open(client * c, websocketpp::connection_hdl hdl);
	void on_fail(client * c, websocketpp::connection_hdl hdl);
	void on_close(client * c, websocketpp::connection_hdl hdl);
	void on_message(websocketpp::connection_hdl hdl, client::message_ptr msg);

	websocketpp::connection_hdl get_hdl() const {
		return m_hdl;
	}

	int get_id() const {
		return m_id;
	}

	std::string get_status() const {
		return m_status;
	}

	std::string get_error_reason() const {
		return m_error_reason;
	}

	friend std::ostream & operator<< (std::ostream & out, connection_metadata const & data);
private:
	// 连接终端对象 在事件中回调
	websocket_endpoint *m_endpoint;
	// 标签
	int m_id;
	// 连接句柄
	websocketpp::connection_hdl m_hdl;
	// 连接状态
	std::string m_status;
	// 服务器链接
	std::string m_uri;
	// 服务器名称
	std::string m_server;
	// 连接关闭的原因
	std::string m_error_reason;
	// 接收数据缓存
	std::string m_recvbuff;
};

class websocket_endpoint {
public:
	websocket_endpoint();
	virtual ~websocket_endpoint();

protected:
	void set_timer(int interval);
	void cancel_timer();
	virtual void on_timer(websocketpp::lib::error_code const & ec) {}
#ifdef WITH_TLS
	context_ptr on_tls_init(websocketpp::connection_hdl);
#endif

public:
	int connect(int label, std::string const & uri);
	void close(int id, websocketpp::close::status::value code, std::string reason);
	void closeall();
	void send(int id, unsigned char *message, int len);
	connection_metadata::ptr get_metadata(int id);

	virtual void on_open(connection_metadata *it) {}
	virtual void on_fail(connection_metadata *it) {}
	virtual void on_close(connection_metadata *it) {}
	virtual void on_message(connection_metadata *it, std::string &msg, int len) {}

protected:
	typedef std::map<int, connection_metadata::ptr> con_list;
	// 标签-连接指针的map
	con_list m_connection_list;
	// 连接终端
	client m_client;
	// 终端 m_client 的运行线程
	websocketpp::lib::shared_ptr<websocketpp::lib::thread> m_thread;
	// 心跳定时器
	client::timer_ptr m_timer;
};
