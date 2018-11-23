#include "stdafx.h"
#include "source_base.h"

int source_base::stop() {
	con_list_.clear();
	con_info_.clear();
	return 0;
}

void source_base::show_stat() {
	printf("Map count: %d List count: %d \n", con_info_.size(), con_list_.size());
}

bool source_base::is_exist(const unsigned id) {
	if (con_list_.count(id)) {
		return true;
	}
	return false;
}

void source_base::do_info_add(const unsigned id, const ROOM_INFO & info) {
	con_info_[id] = info;
	con_info_[id].needclose = false;
}

void source_base::do_list_add(const unsigned id) {
	EnterCriticalSection(&cslist_);
	con_list_.insert(id);
	LeaveCriticalSection(&cslist_);
}

void source_base::do_list_del(const unsigned id) {
	EnterCriticalSection(&cslist_);
	con_list_.erase(id);
	LeaveCriticalSection(&cslist_);
}
