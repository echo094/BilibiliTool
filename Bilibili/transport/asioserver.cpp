#include "asioserver.h"
#include <string>
#include <boost/bind.hpp>
#include "logger/log.h"

void asioserver::init()
{
	// Create the worker
	pwork_ = std::make_shared<boost::asio::io_context::work>(io_context_);
	// Create IO thread
	io_thread_.reset(new std::thread(
		boost::bind(&boost::asio::io_service::run, &io_context_)
	));
	// Start listening
	do_accept();
}

void asioserver::deinit()
{
	// Stop listening
	acceptor_.cancel();
	// Close sockets
	boost::asio::post(
		io_context_, 
		[this]() { do_deinit(); }
	);
	// Stop worker
	pwork_.reset();
	// Wait for threads to exit
	io_thread_->join();
	io_thread_.reset();
}

void asioserver::broadcast(std::shared_ptr<std::string> msg)
{
	boost::asio::post(
		io_context_,
		[this, msg]() { do_broadcast(msg); }
	);
}

void asioserver::do_accept()
{
	acceptor_.async_accept(
		[this](boost::system::error_code ec, tcp::socket socket)
	{
		if (ec) {
			BOOST_LOG_SEV(g_logger::get(), info) << "[TCPSEV] listen code:" << ec;
			return;
		}
		auto s = std::make_shared<tcp_connection>(std::move(socket));
		sessions_.insert(s);
		BOOST_LOG_SEV(g_logger::get(), info) << "[TCPSEV] accept "
			<< s->addr << ':' << s->port;
		do_read(s);
		do_accept();
	});
}

void asioserver::do_deinit()
{
	for (auto it = sessions_.begin(); it != sessions_.end(); it++) {
		(*it)->socket_.close();
	}
}

void asioserver::do_read(tcp_connection::pointer s)
{
	s->socket_.async_read_some(boost::asio::buffer(s->data_, tcp_connection::max_length),
		[this, s](boost::system::error_code ec, std::size_t length)
	{
		if (ec){
			BOOST_LOG_SEV(g_logger::get(), info) << "[TCPSEV] close "
				<< s->addr << ':' << s->port << " code: " << ec;
			sessions_.erase(s);
			return;
		}
		do_read(s);
	});
}

void asioserver::do_broadcast(std::shared_ptr<std::string> msg)
{
	BOOST_LOG_SEV(g_logger::get(), info) << "[TCPSEV] broadcast length:" << msg->size();
	for (auto it = sessions_.begin(); it != sessions_.end(); it++) {
		tcp_connection::pointer s = *it;
		s->msg = msg;
		boost::asio::async_write(s->socket_, boost::asio::buffer(*(s->msg)),
			[this, s](boost::system::error_code ec, std::size_t /*length*/)
		{
			if (ec) {
				sessions_.erase(s);
			}
		});
	}
}
