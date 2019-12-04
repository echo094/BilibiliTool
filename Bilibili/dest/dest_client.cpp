#include "dest_client.h"
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

dest_client::dest_client(unsigned port) :
	server(port) {
	server.init();
}

dest_client::~dest_client() {
	server.deinit();
}

void dest_client::post_lottery(std::shared_ptr<BILI_LOTTERYDATA> data)
{
	std::shared_ptr<std::string> msg(new std::string());
	rapidjson::Document doc;
	rapidjson::Value val;
	doc.SetObject();
	doc.AddMember("cmd", data->cmd, doc.GetAllocator());
	doc.AddMember("srid", data->srid, doc.GetAllocator());
	doc.AddMember("rrid", data->rrid, doc.GetAllocator());
    val.SetInt64(data->loid);
	doc.AddMember("loid", val, doc.GetAllocator());
    val.SetInt64(data->time_start);
	doc.AddMember("time_start", val, doc.GetAllocator());
    val.SetInt64(data->time_get);
	doc.AddMember("time_get", val, doc.GetAllocator());
    val.SetInt64(data->time_end);
	doc.AddMember("time_end", val, doc.GetAllocator());
	val.SetString(data->type.c_str(), data->type.size(), doc.GetAllocator());
	doc.AddMember("type", val, doc.GetAllocator());
	doc.AddMember("exinfo", data->exinfo, doc.GetAllocator());
	val.SetString(data->title.c_str(), data->title.size(), doc.GetAllocator());
	doc.AddMember("title", val, doc.GetAllocator());
	doc.AddMember("join_type", data->join_type, doc.GetAllocator());
	doc.AddMember("require_type", data->require_type, doc.GetAllocator());
	doc.AddMember("require_value", data->require_value, doc.GetAllocator());
	doc.AddMember("gift_id", data->gift_id, doc.GetAllocator());
	doc.AddMember("gift_num", data->gift_num, doc.GetAllocator());
	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	doc.Accept(writer);

	auto bufflen = buffer.GetSize();
	auto pkglen = 8 + bufflen;
	(*msg).reserve(pkglen + 10);
	(*msg).resize(8);
	(*msg)[0] = (pkglen >> 24) & 0xff;
	(*msg)[1] = (pkglen >> 16) & 0xff;
	(*msg)[2] = (pkglen >> 8) & 0xff;
	(*msg)[3] = (pkglen) & 0xff;
	(*msg)[7] = 1;
	(*msg).append(buffer.GetString(), bufflen);

	server.broadcast(msg);
}
