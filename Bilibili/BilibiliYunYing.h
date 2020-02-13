#pragma once
#include <list>
#include <memory>
#include <set>
#include <string>
#include "BilibiliStruct.h"
#include "utility/httpex.h"
#include "rapidjson/document.h"

// 目前不支持多线程操作
class event_list_base
{
public:
	event_list_base();
	virtual ~event_list_base();

public:
	// 查询抽奖入口函数
	// 查询成功或房间被封禁返回0 失败返回-1
	int CheckLottery(CURL *pcurl, std::shared_ptr<BILI_LOTTERYDATA> data);
	// 获取最近的一条抽奖信息 无信息返回空
	std::shared_ptr<BILI_LOTTERYDATA> GetNextLottery();

protected:
	// 需要在类中实例化的查询API
	virtual BILIRET _GetLotteryID(CURL *pcurl, std::shared_ptr<BILI_LOTTERYDATA> &data) = 0;
	// 检测房间号以及房间状态 待修改
	BILIRET _CheckRoom(CURL* pcurl, std::shared_ptr<BILI_LOTTERYDATA> &data);
	// 更新待抽奖列表
	virtual void _UpdateLotteryList(rapidjson::Value &infoArray, std::shared_ptr<BILI_LOTTERYDATA> &data) = 0;

protected:
	unique_ptr<toollib::CHTTPPack> m_httppack;
	std::list<std::shared_ptr<BILI_LOTTERYDATA> > m_lotteryactive;
};

class lottery_list : public event_list_base
{
protected:
	BILIRET _GetLotteryID(CURL *pcurl, std::shared_ptr<BILI_LOTTERYDATA> &data) override;
	// 更新待抽奖列表
	void _UpdateLotteryList(rapidjson::Value &infoArray, std::shared_ptr<BILI_LOTTERYDATA> &data) override;
    
public:
    // 清空抽奖id缓存
    void ClearLotteryCache();

private:
	bool _CheckLoid(const long long id);
    
private:
    std::set<long long> m_cacheid;
};

class guard_list : public event_list_base
{
public:
    guard_list():
        m_curid(0) {}

protected:
	BILIRET _GetLotteryID(CURL *pcurl, std::shared_ptr<BILI_LOTTERYDATA> &data) override;
	// 更新待领取列表
	void _UpdateLotteryList(rapidjson::Value &infoArray, std::shared_ptr<BILI_LOTTERYDATA> &data) override;
        
private:
    long long m_curid;
};

// 其它API
class CBilibiliLive
{
protected:
	unique_ptr<toollib::CHTTPPack> _httppack;
public:
	CBilibiliLive() :
		_httppack(new toollib::CHTTPPack()) {
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
