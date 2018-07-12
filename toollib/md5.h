#pragma once
/*
*******************************************************
* brief: md5 encryption
* author: Monkey.Knight
*******************************************************
*/
#ifndef _TOOLLIB_MD5ENCODE_
#define _TOOLLIB_MD5ENCODE_

// std
#include <string>

namespace toollib {

	std::string Encode_MD5(std::string src_info);

}

#endif