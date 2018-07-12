#pragma once
#include <string>
#include <list>

class CBilibiliLotteryBase
{
public:
	CBilibiliLotteryBase();
	virtual ~CBilibiliLotteryBase();

protected:
	CTools _tool;
	CStrConvert _strcoding;
	CHTTPPack _httppack;
	int _curid;
	std::list<PBILI_LOTTERYDATA> _bili_lotteryactive;

public:
	// 查询抽奖入口函数
	int CheckLottery(CURL *pcurl, int room);
	// 获取最近的一条抽奖信息
	int GetNextLottery(BILI_LOTTERYDATA &pla);
protected:
	// 需要在类中实例化的查询API
	virtual int _GetLotteryID(CURL *pcurl, int srid, int rrid) = 0;
	// 检测房间号以及房间状态
	int _CheckRoom(CURL *pcurl, int srid, int &rrid);
	// 更新待抽奖列表
	int _UpdateLotteryList(rapidjson::Value &infoArray, int srid, int rrid);

};


class CBilibiliYunYing: public CBilibiliLotteryBase
{
protected:
	int _GetLotteryID(CURL *pcurl, int srid, int rrid);
};

class CBilibiliSmallTV : public CBilibiliLotteryBase
{
protected:
	int _GetLotteryID(CURL *pcurl, int srid, int rrid);
};

// 其它API
class CBilibiliLive
{
protected:
	CTools _tool;
	CStrConvert _strcoding;
	CHTTPPack _httppack;
public:
	int ApiSearchUser(CURL *pcurl, const char *user, int &rrid);
	int ApiCheckGuard(CURL *pcurl, int rrid, int &loid);
};
