#include "stdafx.h"
#include "wxclient.h"

websocket_endpoint *ptrmsg = NULL;
void OnEvent(int type, connection_metadata *it) {
	if (!ptrmsg) {
		return;
	}
	switch (type) {
	case 1: {
		ptrmsg->on_open(it);
		break;
	}
	case 2: {
		ptrmsg->on_fail(it);
		break;
	}
	case 3: {
		ptrmsg->on_close(it);
		break;
	}
	}
}
void OnMessage(int type, connection_metadata *it, std::string &msg, int len) {
	if (!ptrmsg) {
		return;
	}
	switch (type) {
	case 4: {
		ptrmsg->on_message(it, msg, len);
		break;
	}
	}
}

std::ostream & operator<< (std::ostream & out, connection_metadata const & data) {
	out << "> URI: " << data.m_uri << "\n"
		<< "> Status: " << data.m_status << "\n"
		<< "> Remote Server: " << (data.m_server.empty() ? "None Specified" : data.m_server) << "\n"
		<< "> Error/close reason: " << (data.m_error_reason.empty() ? "N/A" : data.m_error_reason) << "\n";
	out << "> Messages Processed: (" << data.m_messages.size() << ") \n";

	std::vector<std::string>::const_iterator it;
	for (it = data.m_messages.begin(); it != data.m_messages.end(); ++it) {
		out << *it << "\n";
	}

	return out;
}

void connection_metadata::on_open(client * c, websocketpp::connection_hdl hdl) {
	m_status = "Open";
	client::connection_ptr con = c->get_con_from_hdl(hdl);
	m_server = con->get_response_header("Server");

	OnEvent(1, this);
}

void connection_metadata::on_fail(client * c, websocketpp::connection_hdl hdl) {
	m_status = "Failed";

	client::connection_ptr con = c->get_con_from_hdl(hdl);
	m_server = con->get_response_header("Server");
	m_error_reason = con->get_ec().message();

	OnEvent(2, this);
}

void connection_metadata::on_close(client * c, websocketpp::connection_hdl hdl) {
	m_status = "Closed";
	client::connection_ptr con = c->get_con_from_hdl(hdl);
	std::stringstream s;
	s << "close code: " << con->get_remote_close_code() << " ("
		<< websocketpp::close::status::get_string(con->get_remote_close_code())
		<< "), close reason: " << con->get_remote_close_reason();
	m_error_reason = s.str();

	OnEvent(3, this);
}

void connection_metadata::on_message(websocketpp::connection_hdl hdl, client::message_ptr msg) {
	// this->record_recv_message(msg);
	m_recvbuff.append(msg->get_payload(), 0, msg->get_payload().length());
	OnMessage(4, this, m_recvbuff, msg->get_payload().length());
}

void connection_metadata::record_sent_message(std::string message) {
	m_messages.push_back(">> " + message);
}

void connection_metadata::record_recv_message(client::message_ptr msg) {
	if (msg->get_opcode() == websocketpp::frame::opcode::text) {
		m_messages.push_back("<< " + msg->get_payload());
	}
	else {
		m_messages.push_back("<< " + websocketpp::utility::to_hex(msg->get_payload()));
	}
}

websocket_endpoint::websocket_endpoint():
	m_timer(NULL) {

	ptrmsg = this;

	m_endpoint.clear_access_channels(websocketpp::log::alevel::all);
	m_endpoint.clear_error_channels(websocketpp::log::elevel::all);
	// m_endpoint.set_access_channels(websocketpp::log::alevel::all);

	m_endpoint.init_asio();
	m_endpoint.set_user_agent("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/67.0.3396.99 Safari/537.36");
#ifdef WITH_TLS
	m_endpoint.set_tls_init_handler(bind(&websocket_endpoint::on_tls_init, this, websocketpp::lib::placeholders::_1));
#endif
	m_endpoint.start_perpetual();

	m_thread = websocketpp::lib::make_shared<websocketpp::lib::thread>(&client::run, &m_endpoint);
}

websocket_endpoint::~websocket_endpoint() {
	m_endpoint.stop_perpetual();
	closeall();
	m_thread->join();
}

void websocket_endpoint::set_timer(int interval) {
	m_timer = m_endpoint.set_timer(
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
	std::cout << "> TLS init... " << std::endl;
	// m_tls_init = std::chrono::high_resolution_clock::now();
	context_ptr ctx = websocketpp::lib::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::tlsv12);

	try {
		ctx->set_options(boost::asio::ssl::context::default_workarounds |
			boost::asio::ssl::context::no_sslv2 |
			boost::asio::ssl::context::no_sslv3 |
			boost::asio::ssl::context::single_dh_use);
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

	client::connection_ptr con = m_endpoint.get_connection(uri, ec);

	if (ec) {
		std::cout << "> Connect initialization error: " << ec.message() << std::endl;
		return -1;
	}

	connection_metadata::ptr metadata_ptr = websocketpp::lib::make_shared<connection_metadata>(label, con->get_handle(), uri);
	m_connection_list[label] = metadata_ptr;
	con->set_open_handler(websocketpp::lib::bind(&connection_metadata::on_open, metadata_ptr, &m_endpoint, websocketpp::lib::placeholders::_1));
	con->set_fail_handler(websocketpp::lib::bind(&connection_metadata::on_fail, metadata_ptr, &m_endpoint, websocketpp::lib::placeholders::_1));
	con->set_close_handler(websocketpp::lib::bind(&connection_metadata::on_close, metadata_ptr, &m_endpoint, websocketpp::lib::placeholders::_1));
	con->set_message_handler(websocketpp::lib::bind(&connection_metadata::on_message, metadata_ptr, websocketpp::lib::placeholders::_1, websocketpp::lib::placeholders::_2));

	m_endpoint.connect(con);

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
		m_endpoint.close(metadata_it->second->get_hdl(), code, reason, ec);
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
		m_endpoint.close(it->second->get_hdl(), websocketpp::close::status::going_away, "", ec);
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

	m_endpoint.send(metadata_it->second->get_hdl(), message, len, websocketpp::frame::opcode::binary, ec);
	if (ec) {
		std::cout << "> Error sending message: " << ec.message() << std::endl;
		return;
	}

	// metadata_it->second->record_sent_message(message);
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