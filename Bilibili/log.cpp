#include "stdafx.h"
#include "log.h"

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <fstream>

#include <boost/core/null_deleter.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/smart_ptr/make_shared_object.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <boost/log/expressions.hpp>
#include <boost/log/sinks.hpp>
#include <boost/log/attributes.hpp>

#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/sources/logger.hpp>

void boost_log_init() {
	namespace logging = boost::log;
	namespace sinks = boost::log::sinks;
	namespace keywords = boost::log::keywords;
	namespace expr = boost::log::expressions;
	namespace attrs = boost::log::attributes;

	// Create a file backend
	boost::shared_ptr< sinks::text_file_backend > file_backend =
		boost::make_shared< sinks::text_file_backend >(
			keywords::file_name = "tool_%Y%m%d_%H%M%S_%2N.log",
			// keywords::file_name = "tool_%2N.log",
			keywords::rotation_size = 4 * 1024 * 1024
			);
	file_backend->auto_flush(true);
	// Wrap it into the frontend and register in the core.
	// The backend requires synchronization in the frontend.
	typedef sinks::synchronous_sink< sinks::text_file_backend > sink_file;
	boost::shared_ptr< sink_file > sink1(new sink_file(file_backend));
	sink1->set_formatter(
		expr::format("%1%\t[%2%]\t[%3%]\t%4%")
		% expr::format_date_time< boost::posix_time::ptime >("TimeStamp", "%Y%m%d-%H:%M:%S")
		% expr::attr< attrs::current_thread_id::value_type >("ThreadID")
		% logging::trivial::severity
		% expr::smessage
	);
	logging::core::get()->add_sink(sink1);

	// Create a backend and attach a couple of streams to it
	boost::shared_ptr< sinks::text_ostream_backend > stream_backend =
		boost::make_shared< sinks::text_ostream_backend >();
	stream_backend->add_stream(
		boost::shared_ptr< std::ostream >(&std::clog, boost::null_deleter()));
	// Construct the sink
	typedef sinks::synchronous_sink< sinks::text_ostream_backend > text_sink;
	boost::shared_ptr< text_sink > sink2 = boost::make_shared< text_sink >(stream_backend);
	sink2->set_formatter(
		expr::format("%1%[%2%]%3%")
		% expr::format_date_time< boost::posix_time::ptime >("TimeStamp", "%Y%m%d-%H:%M:%S")
		% logging::trivial::severity
		% expr::smessage
	);
	sink2->set_filter(
		logging::trivial::severity >= logging::trivial::info
	);
	// Register the sink in the logging core
	logging::core::get()->add_sink(sink2);
	// Add attributes to the logging system
	logging::add_common_attributes();
}

void boost_log_deinit() {
	boost::log::core::get()->remove_all_sinks();
}

const std::string char2hexstring(unsigned char * data, int nLen) {
	using namespace std;
	ostringstream oss;
	oss << hex << setfill('0');
	for (int i = 0; i < nLen; i++) {
		oss << setw(2) << static_cast<unsigned int>(data[i]) << ' ';
	}
	return oss.str();
}
