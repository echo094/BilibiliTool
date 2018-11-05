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

enum class DANMU_MODE {
	SINGLE_ROOM = 1,
	MULTI_ROOM
};

class CBilibiliDanmu:
	public DanmuAPI, 
	public toollib::CIOCPClient {
public:
	CBilibiliDanmu();
	~CBilibiliDanmu();

// 重载CIOCPClient中的函数
protected:
	int OnClose(PER_SOCKET_CONTEXT* pSocketContext) override;
	int OnReceive(PER_SOCKET_CONTEXT* pSocketContext, PER_IO_CONTEXT* pIoContext, int byteslen) override;
	int OnHeart() override;

public:
	// 开启主监视线程 初始化IOCP类
	int Init(DANMU_MODE mode = DANMU_MODE::MULTI_ROOM);
	// 退出所有线程 断开Socket 释放所有IOCP资源
	int Deinit();
	// 连接一个房间
	int ConnectToRoom(const unsigned room, const unsigned area, const DANMU_FLAG flag);
	// 断开特定房间
	int DisconnectFromRoom(const unsigned room);
	// 更新房间
	void UpdateRoom(std::set<unsigned> &nlist, DANMU_FLAG flag);
	// 显示当前连接数
	int ShowCount();

private:
	long long GetRUID();
	int MakeConnectionInfo(unsigned char* str, int len, int room);
	int MakeHeartInfo(unsigned char* str, int len, int room);
	int SendConnectionInfo(int room);

private:
	bool _isworking;
};
