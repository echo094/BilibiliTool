#pragma once
#include <boost/log/common.hpp>
#include <boost/log/trivial.hpp>

using namespace boost::log::trivial;

typedef boost::log::sources::severity_logger_mt < boost::log::trivial::severity_level > level_type;
BOOST_LOG_INLINE_GLOBAL_LOGGER_DEFAULT(g_logger, level_type)

void boost_log_init();
void boost_log_deinit();

const std::string char2hexstring(unsigned char * data, int nLen);
