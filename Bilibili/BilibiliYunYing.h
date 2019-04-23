#pragma once
#include <string>
#include <list>
#include <set>

// 目前不支持多线程操作
class event_list_base
{
public:
	event_list_base();
	virtual ~event_list_base();

public:
	// 查询抽奖入口函数
	int CheckLottery(CURL *pcurl, int room);
	// 获取最近的一条抽奖信息
	int GetNextLottery(BILI_LOTTERYDATA &pla);
	// 显示漏掉的抽奖事件
	void ShowMissingLottery();
	// 清空漏掉的抽奖事件
	void ClearMissingLottery();

protected:
	// 需要在类中实例化的查询API
	virtual BILIRET _GetLotteryID(CURL *pcurl, int srid, int rrid) = 0;
	// 检测房间号以及房间状态 待修改
	BILIRET _CheckRoom(CURL* pcurl, int srid, int& rrid);
	// 检测房间号以及房间状态 废除
	BILIRET _CheckRoomOld(CURL *pcurl, int srid, int &rrid);
	// 更新待抽奖列表
	virtual void _UpdateLotteryList(rapidjson::Value &infoArray, int srid, int rrid) = 0;

protected:
	unique_ptr<CHTTPPack> m_httppack;
	int m_curid;
	std::list<PBILI_LOTTERYDATA> m_lotteryactive;
	std::set<int> m_missingid;
};

class lottery_list : public event_list_base
{
protected:
	BILIRET _GetLotteryID(CURL *pcurl, int srid, int rrid) override;
	// 更新待抽奖列表
	void _UpdateLotteryList(rapidjson::Value &infoArray, int srid, int rrid) override;

private:
	bool _CheckLoid(const int id);
};

class guard_list : public event_list_base
{
protected:
	BILIRET _GetLotteryID(CURL *pcurl, int srid, int rrid) override;
	// 更新待领取列表
	void _UpdateLotteryList(rapidjson::Value &infoArray, int srid, int rrid) override;
};

// 其它API
class CBilibiliLive
{
protected:
	unique_ptr<CHTTPPack> _httppack;
public:
	CBilibiliLive() {
		_httppack = std::make_unique<CHTTPPack>();
	}
	~CBilibiliLive() {
		_httppack = nullptr;
	}
	// 获取分区数量
	BILIRET GetAreaNum(CURL *pcurl, unsigned &num) const;
	// 获取人气满足一定条件的房间列表
	BILIRET GetLiveList(CURL *pcurl, std::set<unsigned> &rlist, const unsigned minpop) const;
	// 获取指定分区的一个房间
	BILIRET PickOneRoom(CURL *pcurl, unsigned &nrid, const unsigned orid, const unsigned area) const;

private:
	BILIRET _ApiLiveList(CURL *pcurl, rapidjson::Document &doc, int pid) const;
	BILIRET _ApiRoomArea(CURL *pcurl, rapidjson::Document &doc, const unsigned a1, const unsigned a2) const;
};
