// stdafx.h : 标准系统包含文件的包含文件，
// 或是经常使用但不常更改的
// 特定于项目的包含文件
//

#pragma once

#include <stdio.h>

#include "BilibiliStruct.h"

#include "base64.h"
#include "httpex.h"
#include "sslex.h"
#include "utility/md5.h"
#include "utility/strconvert.h"
#include "utility/platform.h"
using namespace toollib;

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/encodings.h"
#include "rapidjson/filereadstream.h"   // FileReadStream
#include "rapidjson/encodedstream.h"    // AutoUTFInputStream
