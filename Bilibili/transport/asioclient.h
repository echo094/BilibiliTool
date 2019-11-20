// /*
// boost::asio::error
// 远程关闭时的返回码
// eof 2
// 使用不同方式关闭时的返回码
// shutdown    2 eof
// cancel    995 ERROR_OPERATION_ABORTED operation_aborted
// close    1236 ERROR_CONNECTION_ABORTED
// 用close关起来最快 shutdown更符合规范
// */
#pragma once
#include <functional>
#include <map>
#include <memory>
#include <vector>
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>

using boost::asio::ip::tcp;

namespace ioconst {
	const size_t MAX_SEND_BUFF = 8192;
	const size_t MAX_HEADER_BUFF = 128;
	const size_t MAX_PAYLOAD_BUFF = 8192;

	enum {
		IO_CONNECTING = 0,
		IO_READY = 0x1,
		IO_SENDING = 0x2
	};
}

struct context_info {
	int stat_;
	int label_;
	int opt_;
	int len_send_;
	char buff_send_[ioconst::MAX_SEND_BUFF];
	char buff_header_[ioconst::MAX_HEADER_BUFF];
	char buff_payload_[ioconst::MAX_PAYLOAD_BUFF];
	tcp::socket socket_;
	std::string key_;

	context_info(
		boost::asio::io_context &io_context, 
		const int id,
		const std::string &key
	):
		stat_(0),
		label_(id),
		opt_(0),
		len_send_(0),
		socket_(io_context),
		key_(key) {
		buff_send_[0] = 0;
		buff_header_[0] = 0;
		buff_payload_[0] = 0;
	}
};

class asioclient
{
private:
	typedef std::map<unsigned, context_info* > socket_list;
	typedef std::function<void(const unsigned, const boost::system::error_code)> error_handler;
	typedef std::function<void(context_info*)> open_handler;
	typedef std::function<void(context_info*)> timer_handler;
	typedef std::function<size_t(context_info*, const int)> header_handler;
	typedef std::function<size_t(context_info*, const int)> payload_handler;
public:
	asioclient(size_t len): 
		prosessor_num_(1),
		io_context_(),
		heart_timer_(io_context_),
		heart_interval_(30),
		heart_thread_(nullptr),
		header_len_(len),
		error_handler_(nullptr),
		open_handler_(nullptr),
		timer_handler_(nullptr),
		header_handler_(nullptr),
		payload_handler_(nullptr) {
	}

public:
	void init(const std::string& host, const std::string& service, const std::size_t interval);
	void deinit();
	void connect(const std::string& host, const std::string& service, const unsigned id, const std::string &key);
	void connect(const unsigned id, const std::string &key);
	int disconnect(const unsigned id);
	void post_write(context_info* context, const char *msg, const size_t len);
	void post_read(context_info* context);

public:
	void set_header_len(size_t len) {
		header_len_ = len;
	}
	size_t get_ionum();

public:
	void set_error_handler(error_handler h) {
		error_handler_ = h;
	}
	void set_open_handler(open_handler h) {
		open_handler_ = h;
	}
	void set_timer_handler(timer_handler h) {
		timer_handler_ = h;
	}
	void set_header_handler(header_handler h) {
		header_handler_ = h;
	}
	void set_payload_handler(payload_handler h) {
		payload_handler_ = h;
	}

private:
	void start_timer();
	void thread_timer();

// The following methods are called only by the io worker threads
private:
	void on_error(context_info* context, const boost::system::error_code ec);
	void do_deinit();
	void on_connect(context_info* context, boost::system::error_code ec, tcp::endpoint);
	void on_timer(boost::system::error_code ec);
	void do_write(context_info* context, const char *msg, const size_t len);
	void do_write(context_info* context);
	void on_write(context_info* context, boost::system::error_code ec, std::size_t length);
	void do_read_header(context_info* context);
	void on_read_header(context_info* context, boost::system::error_code ec, std::size_t length);
	void do_read_payload(context_info* context, std::size_t length);
	void on_read_payload(context_info* context, boost::system::error_code ec, std::size_t length);
	void do_read_some(context_info* context);
	void on_read_some(context_info* context, boost::system::error_code ec, std::size_t length);

private:
	size_t prosessor_num_;
	// IO serverce
	boost::asio::io_context io_context_;
	// Heart Timer
	boost::asio::deadline_timer heart_timer_;
	std::size_t heart_interval_;
	std::shared_ptr<std::thread> heart_thread_;
	// Client
	tcp::resolver::results_type endpoints_;
	// Maintenance worker
	std::shared_ptr<boost::asio::io_context::work> pwork_;
	// Worker thread
	std::vector<std::shared_ptr<std::thread> > io_threads_;
	// Thread lock
	boost::shared_mutex rwmutex_;
	// Socket info
	socket_list socket_list_;
	// Header length
	size_t header_len_;

private:
	error_handler error_handler_;
	open_handler open_handler_;
	timer_handler timer_handler_;
	header_handler header_handler_;
	payload_handler payload_handler_;
};
