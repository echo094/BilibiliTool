#pragma once
#include <fstream>
#include "tcpio/wxclient.h"
#include "source/source_base.h"

class CWSDanmu: 
	public source_base,
	public websocket_endpoint {
public:
	CWSDanmu();
	~CWSDanmu();

protected:
	void on_timer(websocketpp::lib::error_code const & ec) override;

public:
	void on_open(connection_metadata *it) override;
	void on_fail(connection_metadata *it) override;
	void on_close(connection_metadata *it) override;
	void on_message(connection_metadata *it, std::string &msg, int len) override;

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

private:
	int SendConnectionInfo(connection_metadata *it);
	int SendHeartInfo(connection_metadata &it);

private:
	bool _isworking;
};
