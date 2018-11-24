#pragma once
#include "source/source_base.h"
#include "tcpio/asioclient.h"
#include <set>

class source_dmasio :
	public source_base {

public:
	source_dmasio();
	~source_dmasio();

public:
	// 开启心跳定时器
	int start() override;
	// 关闭心跳定时器 断开所有连接
	int stop() override;
	// 连接一个房间
	int add_context(const unsigned id, const ROOM_INFO& info) override;
	// 断开特定房间
	int del_context(const unsigned id) override;
	// 更新房间
	int update_context(std::set<unsigned> &nlist, const unsigned opt) override;
	// 显示当前连接数
	void show_stat() override;

public:
	void on_error(const unsigned id, const boost::system::error_code ec);
	void on_open(context_info* c);
	void on_heart(context_info* c);
	size_t on_header(context_info* c, const int len);
	size_t on_payload(context_info* c, const int len);

private:
	long long GetRUID();
	int CheckMessage(const unsigned char *str);
	int MakeConnectionInfo(unsigned char* str, int len, int room);
	int MakeHeartInfo(unsigned char* str, int len, int room);

private:
	asioclient asioclient_;
	// 运行状态
	bool is_stop_;
	// 正在主动关闭的房间
	std::set<unsigned> list_closing_;
};
