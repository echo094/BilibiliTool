#include "stdafx.h"
#include "asioclient.h"
#include <cstring>
#include <boost/bind.hpp>
#include <boost/ref.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

void asioclient::init(
	const std::string& host, 
	const std::string& service, 
	const std::size_t interval)
{
	// Resolve ip address
	tcp::resolver resolver(io_context_);
	endpoints_ = resolver.resolve(host, service);
	// Create the worker threads
	pwork_ = std::make_shared< boost::asio::io_context::work>(io_context_);
	for (size_t i = 0; i < prosessor_num_; i++) {
		io_threads_.push_back(std::make_shared< std::thread>(
			boost::bind(&boost::asio::io_service::run, &io_context_)
		));
	}
	// Set heart timer
	heart_interval_ = interval;
	start_timer();
}

void asioclient::deinit()
{
	// Cancel the timer
	heart_timer_.cancel();
	if (heart_thread_) {
		heart_thread_->join();
	}
	// Close sockets
	boost::asio::post(
		io_context_, 
		[this]() { do_deinit(); }
	);
	// Wait for threads to exit
	for (size_t i = 0; i < prosessor_num_; i++) {
		io_threads_[i]->join();
	}
	io_threads_.clear();
	// Clean the context list
	{
		boost::unique_lock<boost::shared_mutex> m(rwmutex_);
		for (auto it = socket_list_.begin(); it != socket_list_.end(); it++) {
			delete it->second;
		}
		socket_list_.clear();
	}
}

void asioclient::connect(const unsigned id)
{
	bool exist = false;
	context_info *pitem;
	{
		boost::unique_lock<boost::shared_mutex> m(rwmutex_);
		if (socket_list_.count(id)) {
			exist = true;
		}
		else {
			pitem = new context_info(io_context_, id);
			socket_list_[id] = pitem;
		}
	}
	if (exist) {
		return;
	}

	boost::asio::async_connect(
		pitem->socket_,
		endpoints_,
		boost::bind(
			&asioclient::on_connect, 
			this, 
			pitem,
			boost::asio::placeholders::error,
			boost::asio::placeholders::endpoint
		)
	);
}

int asioclient::disconnect(const unsigned id)
{
	int ret = -1;
	{
		boost::shared_lock<boost::shared_mutex> m(rwmutex_);
		if (socket_list_.count(id)) {
			socket_list_[id]->socket_.close();
			ret = 0;
		}
	}
	return ret;
}

void asioclient::post_write(context_info* context, const char *msg, const size_t len)
{
	if (context->stat_ != ioconst::IO_READY) {
		return;
	}
	context->stat_ = ioconst::IO_SENDING;
	context->len_send_ = len;
	memcpy(context->buff_send_, msg, len);
	boost::asio::post(
		io_context_,
		boost::bind(
			&asioclient::do_write,
			this, 
			context
		)
	);
}

void asioclient::post_read(context_info* context)
{
	boost::asio::post(
		io_context_,
		boost::bind(
			&asioclient::do_read_header,
			this,
			context
		)
	);
}

size_t asioclient::get_ionum() {
	size_t num = 0;
	{
		boost::shared_lock<boost::shared_mutex> m(rwmutex_);
		num = socket_list_.size();
	}
	return num;
}

void asioclient::start_timer()
{
	heart_timer_.expires_from_now(boost::posix_time::seconds(heart_interval_));
	heart_timer_.async_wait(
		boost::bind(
			&asioclient::on_timer,
			this,
			boost::asio::placeholders::error
		)
	);
}

void asioclient::thread_timer()
{
	if (!timer_handler_) {
		return;
	}
	{
		boost::shared_lock<boost::shared_mutex> m(rwmutex_);
		for (auto it = socket_list_.begin(); it != socket_list_.end(); it++) {
			timer_handler_(it->second);
		}
	}
}

void asioclient::on_error(context_info* context, const boost::system::error_code ec)
{
	unsigned id = context->label_;
	{
		boost::unique_lock<boost::shared_mutex> m(rwmutex_);
		for (auto it = socket_list_.begin(); it != socket_list_.end(); it++) {
			if (it->second == context) {
				delete it->second;
				socket_list_.erase(it);
				break;
			}
		}
	}
	if (error_handler_) {
		error_handler_(id, ec);
	}
}

void asioclient::do_deinit()
{
	{
		boost::shared_lock<boost::shared_mutex> m(rwmutex_);
		for (auto it = socket_list_.begin(); it != socket_list_.end(); it++) {
			it->second->socket_.close();
		}
	}
	pwork_ = nullptr;
}

void asioclient::on_connect(context_info* context, boost::system::error_code ec, tcp::endpoint)
{
	if (ec) {
		// 未连接成功时上层列表无记录 不需要更新列表
		// on_error(context, ec);
		return;
	}
	context->stat_ = ioconst::IO_READY;
	do_read_header(context);
	if (open_handler_) {
		open_handler_(context);
	}
}

void asioclient::on_timer(boost::system::error_code ec)
{
	if (ec) {
		return;
	}
	start_timer();
	if (heart_thread_) {
		heart_thread_->join();
	}
	heart_thread_.reset(new std::thread(
		&asioclient::thread_timer, this
	));
}

void asioclient::do_write(context_info* context, const char *msg, const size_t len)
{
	if (context->stat_ != ioconst::IO_READY) {
		return;
	}
	context->stat_ = ioconst::IO_SENDING;
	context->len_send_ = len;
	memcpy(context->buff_send_, msg, len);
	do_write(context);
}

void asioclient::do_write(context_info* context)
{
	boost::asio::async_write(
		context->socket_,
		boost::asio::buffer(
			context->buff_send_,
			context->len_send_
		),
		boost::bind(
			&asioclient::on_write,
			this,
			context,
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred
		)
	);
}

void asioclient::on_write(context_info* context, boost::system::error_code ec, std::size_t length)
{
	if (ec) {
		// 发送失败时 接收IO同样会产生错误 只需向上层通知一次
		// on_error(context, ec);
		return;
	}
	context->stat_ = ioconst::IO_READY;
}

void asioclient::do_read_header(context_info* context)
{
	boost::asio::async_read(
		context->socket_,
		boost::asio::buffer(
			context->buff_header_,
			header_len_
		),
		boost::bind(
			&asioclient::on_read_header,
			this,
			context,
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred
		)
	);
}

void asioclient::on_read_header(context_info* context, boost::system::error_code ec, std::size_t length)
{
	if (ec) {
		on_error(context, ec);
		return;
	}
	int ret = 0;
	if (header_handler_) {
		ret = header_handler_(context, length);
	}
	if (ret == header_len_) {
		do_read_header(context);
	}
	else if (ret == 0) {
		do_read_some(context);
	}
	else {
		do_read_payload(context, ret - 16);
	}
}

void asioclient::do_read_payload(context_info* context, std::size_t length)
{
	boost::asio::async_read(
		context->socket_,
		boost::asio::buffer(
			context->buff_payload_,
			length
		),
		boost::bind(
			&asioclient::on_read_payload,
			this,
			context,
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred
		)
	);
}

void asioclient::on_read_payload(context_info* context, boost::system::error_code ec, std::size_t length)
{
	if (ec) {
		on_error(context, ec);
		return;
	}
	if (payload_handler_) {
		payload_handler_(context, length);
	}
	do_read_header(context);
}

void asioclient::do_read_some(context_info* context)
{
	context->socket_.async_read_some(
		boost::asio::buffer(
			context->buff_payload_,
			ioconst::MAX_PAYLOAD_BUFF
		),
		boost::bind(
			&asioclient::on_read_some,
			this,
			context,
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred
		)
	);
}

void asioclient::on_read_some(context_info* context, boost::system::error_code ec, std::size_t length)
{
	if (ec) {
		on_error(context, ec);
		return;
	}
	do_read_header(context);
}
