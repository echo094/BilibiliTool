#pragma once
#include "transport/asioserver.h"
#include "BilibiliStruct.h"

class dest_client
{
public:
	explicit dest_client(unsigned port);
	~dest_client();

	void post_lottery(std::shared_ptr<BILI_LOTTERYDATA> data);

private:
	asioserver server;
};
