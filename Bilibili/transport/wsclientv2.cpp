#include "wsclientv2.h"
#include <functional>
#include <boost/bind.hpp>
#include "logger/log.h"

namespace ssl = boost::asio::ssl;

const char DM_UA[] = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/78.0.3904.108 Safari/537.36";

// Report a failure
static void
fail(boost::system::error_code ec, char const* what)
{
	BOOST_LOG_SEV(g_logger::get(), info) << "[BEAST] "
		<< what << ": " << ec.message();
}

#ifdef USE_WSS
WsClientV2::WsClientV2() :
	io_ctx_(),
	ssl_ctx_{ ssl::context::tlsv12 }
{
	boost::system::error_code ec;
	ssl_ctx_.set_verify_mode(boost::asio::ssl::verify_none, ec);

	pwork_ = std::make_shared< boost::asio::io_context::work>(io_ctx_);
	io_thread_.reset(new std::thread(
		boost::bind(&boost::asio::io_service::run, &io_ctx_)
	));
}
#else
WsClientV2::WsClientV2() :
	io_ctx_()
{
	pwork_ = std::make_shared< boost::asio::io_context::work>(io_ctx_);
	io_thread_.reset(new std::thread(
		boost::bind(&boost::asio::io_service::run, &io_ctx_)
	));
}
#endif

WsClientV2::~WsClientV2() {
	// Close sockets
	close_all();
	// Stop worker
	pwork_.reset();
	// Wait for threads to exit
	io_thread_->join();
	io_thread_.reset();
}

void WsClientV2::connect(
	unsigned id,
	char const* host,
	char const* port,
	char const* param,
	const std::string key)
{
	// Create a session
#ifdef USE_WSS
	std::shared_ptr<session_ws> c(new session_ws(
		io_ctx_, ssl_ctx_, host, param, id, key
	));
#else
	std::shared_ptr<session_ws> c(new session_ws(
		io_ctx_, host, param, id, key
	));
#endif
	// Look up the domain name
	c->resolver_.async_resolve(
		host,
		port,
		[this, c](
			boost::system::error_code ec,
			boost::asio::ip::tcp::resolver::results_type results)
	{
		if (ec) {
			return fail(ec, "resolve");
		}
		on_resolve(c, results);
	});
}

void WsClientV2::close_spec(unsigned id)
{
	boost::asio::post(
		io_ctx_,
		[this, id]()
	{
		for (auto it = sessions_.begin(); it != sessions_.end(); it++) {
			if ((*it)->label_ == id) {
				auto p = *it;
				sessions_.erase(it);
				p->ws_.async_close(
					beast::websocket::close_code::normal,
					[this, p](boost::system::error_code ec)
				{
					on_close(p->label_);
				});
				break;
			}
		}
	});
}

void WsClientV2::close_all() {
	boost::asio::post(
		io_ctx_,
		[this]()
	{
		auto it = sessions_.begin();
		while (it != sessions_.end()) {
			auto p = *it;
			it = sessions_.erase(it);
			p->ws_.async_close(
				beast::websocket::close_code::normal,
				[this, p](boost::system::error_code ec)
			{
				on_close(p->label_);
			});
		}
	});
}

void WsClientV2::on_resolve(
	std::shared_ptr<session_ws> c,
	boost::asio::ip::tcp::resolver::results_type results)
{
	// Make the connection on the IP address we get from a lookup
	beast::get_lowest_layer(c->ws_).async_connect(
		results,
		std::bind(
			&WsClientV2::on_connect,
			this,
			c,
			std::placeholders::_1,
			std::placeholders::_2));
}

#ifdef USE_WSS
void WsClientV2::on_connect(
	std::shared_ptr<session_ws> c,
	boost::system::error_code ec,
	boost::asio::ip::tcp::resolver::results_type::endpoint_type)
{
	if (ec) {
		return fail(ec, "connect");
	}
	// Perform the SSL handshake
	c->ws_.next_layer().async_handshake(
		ssl::stream_base::client,
		[this, c](
			boost::system::error_code ec)
	{
		if (ec) {
			return fail(ec, "ssl_handshake");
		}
		do_handshake(c);
	});
}
#else
void WsClientV2::on_connect(
	std::shared_ptr<session_ws> c,
	boost::system::error_code ec,
	boost::asio::ip::tcp::resolver::results_type::endpoint_type)
{
	if (ec) {
		return fail(ec, "connect");
	}
	do_handshake(c);
}
#endif

void WsClientV2::do_handshake(
	std::shared_ptr<session_ws> c)
{
	// Set a decorator to change the User-Agent of the handshake
	c->ws_.set_option(beast::websocket::stream_base::decorator(
		[](beast::websocket::request_type& req)
	{
		req.set(beast::http::field::user_agent, DM_UA);
	}));
	// Perform the websocket handshake
	c->ws_.async_handshake(
		c->host_,
		c->param_,
		[this, c](boost::system::error_code ec)
	{
		if (ec) {
			return fail(ec, "handshake");
		}
		sessions_.insert(c);
		do_read(c);
		on_open(c);
	});
}

void WsClientV2::do_write(
	std::shared_ptr<session_ws> c,
	std::shared_ptr<std::string> msg)
{
	c->msg_ = msg;
	// Send the message
	c->ws_.binary(true);
	c->ws_.async_write(
		boost::asio::buffer(c->msg_->data(), c->msg_->size()),
		[this, c](
			boost::system::error_code ec,
			std::size_t bytes_transferred)
	{
		if (ec) {
			if (sessions_.count(c)) {
				sessions_.erase(c);
				on_fail(c->label_);
			}
			return fail(ec, "write");
		}
	});
}

void WsClientV2::do_read(
	std::shared_ptr<session_ws> c)
{
	// Read a message into our buffer
	c->ws_.async_read(
		c->buffer_,
		std::bind(
			&WsClientV2::on_read,
			this,
			c,
			std::placeholders::_1,
			std::placeholders::_2));
}

void WsClientV2::on_read(
	std::shared_ptr<session_ws> c,
	boost::system::error_code ec,
	std::size_t bytes_transferred)
{
	if (ec) {
		if (sessions_.count(c)) {
			sessions_.erase(c);
			on_fail(c->label_);
		}
		return fail(ec, "read");
	}
	on_message(c, bytes_transferred);
	// Clear the buffer
	c->buffer_.consume(c->buffer_.size());
	do_read(c);
}
