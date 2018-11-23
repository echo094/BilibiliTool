/*
弹幕协议说明
协议头一般为16位规则如下
 0- 3 总长度
 4- 5 头部长度(0x0010)
 6- 7 版本 客户端版本目前为1 
 8-11 数据类型 3为在线人数 5为JSON数据 8为连接成功
12-15 设备 socket为0 flash为1

操作码
2 客户端发送的心跳包
3 人气值，数据不是JSON，是4字节整数
5 命令，数据中['cmd']表示具体命令
7 认证并加入房间
8 服务器确认加入房间

WS说明
API中的map信息与WS中的连接map相独立
在连接时两个map都会添加对象
在连接意外断开时状态码会更新 两个map的连接信息保留
心跳时对断开的连接重连
关闭指定房间时清除两个map中的对应信息
*/

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
	int CheckMessage(const unsigned char *str);
	int MakeConnectionInfo(unsigned char* str, int len, int room);
	int MakeHeartInfo(unsigned char* str, int len);
	int SendConnectionInfo(connection_metadata *it);
	int SendHeartInfo(connection_metadata &it);

private:
	bool _isworking;
};
