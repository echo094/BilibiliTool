/*
弹幕协议说明
协议头一般为16位规则如下
 0- 3 总长度
 4- 5 头部长度(0x0010)
 6- 7 版本 客户端版本目前为1 
 8-11 数据类型 3为在线人数 5为JSON数据 8为连接成功
12-15 设备 socket为0 flash为1
*/

#pragma once
#include <fstream>
#include "BilibiliDanmuAPI.h"

typedef enum _BILI_ROOMMONITOR_MODE
{
	SINGLE_ROOM = 1,
	MULTI_ROOM,
}BILI_ROOMMONITOR_MODE;

class CBilibiliDanmu:
	public DanmuAPI, 
	public toollib::CIOCPClient {
public:
	CBilibiliDanmu();
	~CBilibiliDanmu();

// 重载CIOCPClient中的函数
protected:
	int OnClose(PER_SOCKET_CONTEXT* pSocketContext);
	int OnReceive(PER_SOCKET_CONTEXT* pSocketContext, PER_IO_CONTEXT* pIoContext, int byteslen);
	virtual int OnHeart();

public:
	// 开启主监视线程 初始化IOCP类
	int Init(int mode = MULTI_ROOM);
	// 退出所有线程 断开Socket 释放所有IOCP资源
	int Deinit();
	// 连接一个房间
	int ConnectToRoom(int room, int flag);
	// 断开特定房间
	int DisconnectFromRoom(int room);
	// 显示当前连接数
	int ShowCount();

private:
	long long GetRUID();
	int MakeConnectionInfo(unsigned char* str, int len, int room);
	int MakeHeartInfo(unsigned char* str, int len, int room);
	int SendConnectionInfo(int room);

private:
	bool _isworking;
	int _roomcount;
};
