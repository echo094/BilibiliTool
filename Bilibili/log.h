#pragma once
#include <boost/log/common.hpp>
#include <boost/log/trivial.hpp>

using namespace boost::log::trivial;

typedef boost::log::sources::severity_logger_mt < boost::log::trivial::severity_level > logger_type;
BOOST_LOG_INLINE_GLOBAL_LOGGER_DEFAULT(g_logger, logger_type)

void boost_log_init();
void boost_log_deinit();
