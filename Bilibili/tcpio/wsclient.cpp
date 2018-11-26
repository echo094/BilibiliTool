#include "stdafx.h"
#include "wxclient.h"

const char DM_UA[] = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/70.0.3538.102 Safari/537.36";

std::ostream & operator<< (std::ostream & out, connection_metadata const & data) {
	out << "> URI: " << data.m_uri << "\n"
		<< "> Status: " << data.m_status << "\n"
		<< "> Remote Server: " << (data.m_server.empty() ? "None Specified" : data.m_server) << "\n"
		<< "> Error/close reason: " << (data.m_error_reason.empty() ? "N/A" : data.m_error_reason) << "\n";

	return out;
}

void connection_metadata::on_open(client * c, websocketpp::connection_hdl hdl) {
	m_status = "Open";
	client::connection_ptr con = c->get_con_from_hdl(hdl);
	m_server = con->get_response_header("Server");

	m_endpoint->on_open(this);
}

void connection_metadata::on_fail(client * c, websocketpp::connection_hdl hdl) {
	m_status = "Failed";

	client::connection_ptr con = c->get_con_from_hdl(hdl);
	m_server = con->get_response_header("Server");
	m_error_reason = con->get_ec().message();

	m_endpoint->on_fail(this);
}

void connection_metadata::on_close(client * c, websocketpp::connection_hdl hdl) {
	m_status = "Closed";
	client::connection_ptr con = c->get_con_from_hdl(hdl);
	std::stringstream s;
	s << "close code: " << con->get_remote_close_code() << " ("
		<< websocketpp::close::status::get_string(con->get_remote_close_code())
		<< "), close reason: " << con->get_remote_close_reason();
	m_error_reason = s.str();

	m_endpoint->on_close(this);
}

void connection_metadata::on_message(websocketpp::connection_hdl hdl, client::message_ptr msg) {
	// 将收到的数据添加到缓冲
	m_recvbuff.append(msg->get_payload(), 0, msg->get_payload().length());

	m_endpoint->on_message(this, m_recvbuff, m_recvbuff.length());
}

websocket_endpoint::websocket_endpoint():
	m_timer(NULL) {

	m_client.clear_access_channels(websocketpp::log::alevel::all);
	m_client.clear_error_channels(websocketpp::log::elevel::all);

	m_client.init_asio();
	m_client.set_user_agent(DM_UA);
#ifdef WITH_TLS
	m_client.set_tls_init_handler(bind(&websocket_endpoint::on_tls_init, this, websocketpp::lib::placeholders::_1));
#endif
	m_client.start_perpetual();
	// 在新线程中运行 m_client 的run函数
	m_thread = websocketpp::lib::make_shared<websocketpp::lib::thread>(&client::run, &m_client);
}

websocket_endpoint::~websocket_endpoint() {
	m_client.stop_perpetual();
	closeall();
	m_thread->join();
}

void websocket_endpoint::set_timer(int interval) {
	m_timer = m_client.set_timer(
		interval,
		websocketpp::lib::bind(
			&websocket_endpoint::on_timer,
			this,
			websocketpp::lib::placeholders::_1
		)
	);
}

void websocket_endpoint::cancel_timer() {
	if (m_timer) {
		m_timer->cancel();
		m_timer = NULL;
	}
}

#ifdef WITH_TLS
context_ptr websocket_endpoint::on_tls_init(websocketpp::connection_hdl) {

	context_ptr ctx = websocketpp::lib::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::tlsv12);
	
	try {
		ctx->set_options(boost::asio::ssl::context::default_workarounds |
			boost::asio::ssl::context::no_sslv2 |
			boost::asio::ssl::context::no_sslv3 |
			boost::asio::ssl::context::single_dh_use);

		ctx->set_verify_mode(boost::asio::ssl::verify_none);
	}
	catch (std::exception& e) {
		std::cout << e.what() << std::endl;
	}
	return ctx;
}
#endif

int websocket_endpoint::connect(int label, std::string const & uri) {
	// 如果该编号已存在则尝试断开连接。
	close(label, websocketpp::close::status::normal, "");

	websocketpp::lib::error_code ec;
	// 创建连接资源 connection_ptr
	client::connection_ptr con = m_client.get_connection(uri, ec);
	if (ec) {
		std::cout << "> Connect initialization error: " << ec.message() << std::endl;
		return -1;
	}
	// 创建连接的复合信息类 connection_metadata
	connection_metadata::ptr metadata_ptr = websocketpp::lib::make_shared<connection_metadata>(
		this, label, con->get_handle(), uri);
	// 添加到map中
	m_connection_list[label] = metadata_ptr;
	// 绑定各种回调函数
	con->set_open_handler(websocketpp::lib::bind(
		&connection_metadata::on_open, 
		metadata_ptr, 
		&m_client,
		websocketpp::lib::placeholders::_1
	));
	con->set_fail_handler(websocketpp::lib::bind(
		&connection_metadata::on_fail, 
		metadata_ptr, 
		&m_client,
		websocketpp::lib::placeholders::_1
	));
	con->set_close_handler(websocketpp::lib::bind(
		&connection_metadata::on_close, 
		metadata_ptr, 
		&m_client,
		websocketpp::lib::placeholders::_1
	));
	con->set_message_handler(websocketpp::lib::bind(
		&connection_metadata::on_message, 
		metadata_ptr, 
		websocketpp::lib::placeholders::_1, 
		websocketpp::lib::placeholders::_2
	));
	// 开始连接
	m_client.connect(con);

	return 0;
}

void websocket_endpoint::close(int id, websocketpp::close::status::value code, std::string reason) {
	websocketpp::lib::error_code ec;

	con_list::iterator metadata_it = m_connection_list.find(id);
	if (metadata_it == m_connection_list.end()) {
		// std::cout << "> No connection found with id " << id << std::endl;
		return;
	}

	if (metadata_it->second->get_status() == "Open") {
		std::cout << "> Closing connection " << id << std::endl;
		m_client.close(metadata_it->second->get_hdl(), code, reason, ec);
		if (ec) {
			std::cout << "> Error initiating close: " << ec.message() << std::endl;
		}
	}
	m_connection_list.erase(metadata_it);
}

void websocket_endpoint::closeall() {
	websocketpp::lib::error_code ec;
	for (con_list::const_iterator it = m_connection_list.begin(); it != m_connection_list.end(); ++it) {
		if (it->second->get_status() != "Open") {
			continue;
		}
		std::cout << "> Closing connection " << it->second->get_id() << std::endl;
		m_client.close(it->second->get_hdl(), websocketpp::close::status::going_away, "", ec);
		if (ec) {
			std::cout << "> Error closing connection " << it->second->get_id() << ": "
				<< ec.message() << std::endl;
		}
	}
	m_connection_list.clear();
}


void websocket_endpoint::send(int id, unsigned char *message, int len) {
	websocketpp::lib::error_code ec;

	con_list::iterator metadata_it = m_connection_list.find(id);
	if (metadata_it == m_connection_list.end()) {
		std::cout << "> No connection found with id " << id << std::endl;
		return;
	}

	m_client.send(metadata_it->second->get_hdl(), message, len, websocketpp::frame::opcode::binary, ec);
	if (ec) {
		std::cout << "> Error sending message: " << ec.message() << std::endl;
		return;
	}
}

connection_metadata::ptr websocket_endpoint::get_metadata(int id) {
	con_list::const_iterator metadata_it = m_connection_list.find(id);
	if (metadata_it == m_connection_list.end()) {
		return connection_metadata::ptr();
	}
	else {
		return metadata_it->second;
	}
}
