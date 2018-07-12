#pragma once
// #define WITH_TLS
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

class connection_metadata {
public:
	typedef websocketpp::lib::shared_ptr<connection_metadata> ptr;

	connection_metadata(int id, websocketpp::connection_hdl hdl, std::string uri)
		: m_id(id)
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

	void record_sent_message(std::string message);
	void record_recv_message(client::message_ptr msg);

	friend std::ostream & operator<< (std::ostream & out, connection_metadata const & data);
private:
	int m_id;
	websocketpp::connection_hdl m_hdl;
	std::string m_status;
	std::string m_uri;
	std::string m_server;
	std::string m_error_reason;
	std::vector<std::string> m_messages;
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
	con_list m_connection_list;
	client m_endpoint;
	websocketpp::lib::shared_ptr<websocketpp::lib::thread> m_thread;
	client::timer_ptr m_timer;
};
