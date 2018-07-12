// stdafx.h : 标准系统包含文件的包含文件，
// 或是经常使用但不常更改的
// 特定于项目的包含文件
//

#pragma once

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>



// TODO: 在此处引用程序需要的其他头文件

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include "windows.h"

#include "BilibiliStruct.h"

#include "base64.h"
#include "httpex.h"
#include "sslex.h"
#include "toollib/md5.h"
#include "toollib/socketsc.h"
#include "toollib/strconvert.h"
using namespace toollib;
#ifdef _DEBUG
#pragma comment(lib,"toollibd.lib")  
#else
#pragma comment(lib,"toollib.lib") 
#endif

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/encodings.h"
#include "rapidjson/filereadstream.h"   // FileReadStream
#include "rapidjson/encodedstream.h"    // AutoUTFInputStream

