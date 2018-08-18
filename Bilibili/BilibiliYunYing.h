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
	unique_ptr<CHTTPPack> _httppack;
	int _curid;
	std::list<PBILI_LOTTERYDATA> _bili_lotteryactive;

public:
	// 查询抽奖入口函数
	int CheckLottery(CURL *pcurl, int room);
	// 获取最近的一条抽奖信息
	int GetNextLottery(BILI_LOTTERYDATA &pla);
protected:
	// 需要在类中实例化的查询API
	virtual BILIRET _GetLotteryID(CURL *pcurl, int srid, int rrid) = 0;
	// 检测房间号以及房间状态
	BILIRET _CheckRoom(CURL *pcurl, int srid, int &rrid);
	// 更新待抽奖列表
	void _UpdateLotteryList(rapidjson::Value &infoArray, int srid, int rrid);

};


class CBilibiliYunYing: public CBilibiliLotteryBase
{
protected:
	BILIRET _GetLotteryID(CURL *pcurl, int srid, int rrid) override;
};

class CBilibiliSmallTV : public CBilibiliLotteryBase
{
protected:
	BILIRET _GetLotteryID(CURL *pcurl, int srid, int rrid) override;
};

// 其它API
class CBilibiliLive
{
protected:
	CTools _tool;
	CStrConvert _strcoding;
	unique_ptr<CHTTPPack> _httppack;
public:
	CBilibiliLive() {
		_httppack = std::make_unique<CHTTPPack>();
	}
	~CBilibiliLive() {
		_httppack = nullptr;
	}
	BILIRET ApiSearchUser(CURL *pcurl, const char *user, int &rrid);
	BILIRET ApiCheckGuard(CURL *pcurl, int rrid, int &loid);
};
