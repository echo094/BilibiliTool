#include "source_notice.h"
#include <fstream>
#include <rapidjson/document.h>
#include "logger/log.h"
#include "utility/platform.h"

source_notice::source_notice() :
	asioclient_(8),
	is_stop_(false) {
	BOOST_LOG_SEV(g_logger::get(), debug) << "[NOTICE] Create.";
}

source_notice::~source_notice() {
	BOOST_LOG_SEV(g_logger::get(), debug) << "[NOTICE] Destroy.";
}

int source_notice::start() {
	using std::placeholders::_1;
	using std::placeholders::_2;
	using rapidjson::ParseErrorCode;

	asioclient_.set_error_handler(
		std::bind(&source_notice::on_error, this, _1, _2)
	);
	asioclient_.set_open_handler(
		std::bind(&source_notice::on_open, this, _1)
	);
	asioclient_.set_header_handler(
		std::bind(&source_notice::on_header, this, _1, _2)
	);
	asioclient_.set_payload_handler(
		std::bind(&source_notice::on_payload, this, _1, _2)
	);

	asioclient_.init("localhost", "8000", 30);
	is_stop_ = false;

	// 导入服务器并连接
	char file[MAX_PATH];
	GetDir(file, sizeof(file));
	strcat(file, DEF_CONFIG_SERVER);
	std::ifstream infile;
	infile.open(file, std::ios::in | std::ios::binary);
	if (!infile.is_open()) {
		BOOST_LOG_SEV(g_logger::get(), info) << "[NOTICE] Config file does not exist. ";
		return 0;
	}
	infile.seekg(0, std::ios::end);
	auto inlen = infile.tellg();
	infile.seekg(0, std::ios::beg);
	char *buff = new char[int(inlen) + 1];
	infile.read(buff, inlen);
	buff[int(inlen)] = 0;
	infile.close();
	rapidjson::Document data;
	data.Parse(buff);
	delete[] buff;
	if (data.GetParseError() != ParseErrorCode::kParseErrorNone) {
		ParseErrorCode ret = data.GetParseError();
		BOOST_LOG_SEV(g_logger::get(), info) << "[NOTICE] Config file error: " << ret;
		return 0;
	}
	for (size_t i = 0; i < data.Size(); i++) {
		std::string host = data[i]["host"].GetString();
		std::string port = data[i]["port"].GetString();
		asioclient_.connect(host, port, i, "");
		BOOST_LOG_SEV(g_logger::get(), info) << "[NOTICE] Connecting " << i << ' ' 
			<< host << ':' << port;
	}

	return 0;
}

int source_notice::stop() {
	is_stop_ = true;
	asioclient_.deinit();
	return 0;
}

void source_notice::show_stat() {
	printf("IO count: %ld \n", asioclient_.get_ionum());
}

void source_notice::on_error(const unsigned id, const boost::system::error_code ec) {
	if (is_stop_) {
		BOOST_LOG_SEV(g_logger::get(), info) << "[NOTICE] Final Close: " << id
			<< " code: " << ec;
	}
	else {
		BOOST_LOG_SEV(g_logger::get(), warning) << "[NOTICE] Abnormal Close: " << id
			<< " code: " << ec;
	}
}

void source_notice::on_open(context_info * c) {
	BOOST_LOG_SEV(g_logger::get(), info) << "[NOTICE] Open: " << c->label_;
}

size_t source_notice::on_header(context_info * c, const int len) {
	unsigned char *data = (unsigned char *)c->buff_header_;
	size_t bufflen = data[0] << 24 | data[1] << 16 | data[2] << 8 | data[3];
	c->opt_ = data[7];
	return bufflen;
}

size_t source_notice::on_payload(context_info * c, const int len) {
	char *buff = new char[len + 1];
	memcpy(buff, c->buff_payload_, len);
	buff[len] = 0;
	rapidjson::Document doc;
	doc.Parse(buff);

	std::shared_ptr<BILI_LOTTERYDATA> data(new BILI_LOTTERYDATA());
	data->cmd = doc["cmd"].GetInt();
	data->srid = doc["srid"].GetInt();
	data->rrid = doc["rrid"].GetInt();
	data->loid = doc["loid"].GetInt64();
	data->time_end = doc["time_end"].GetInt();
	data->time_start = doc["time_start"].GetInt();
	data->time_get = doc["time_get"].GetInt();
	data->type = doc["type"].GetString();
	data->title = doc["title"].GetString();
	data->exinfo = doc["exinfo"].GetUint();
	data->gift_id = doc["gift_id"].GetUint();
	data->gift_num = doc["gift_num"].GetUint();
	BOOST_LOG_SEV(g_logger::get(), info) << "[NOTICE] server:" << c->label_ 
		<< " lottery room: " << data->rrid << " id: " << data->loid;
	if (event_act_) {
		event_act_(data);
	}
	return 0;
}
