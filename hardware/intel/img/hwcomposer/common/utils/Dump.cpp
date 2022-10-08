/*
// Copyright (c) 2014 Intel Corporation 
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/
#include <stdarg.h>
#include <stdio.h>

#include <common/utils/Dump.h>

namespace android {
namespace intel {

Dump::Dump(char *buf, int len)
    : mBuf(buf),
      mLen(len)
{

}

Dump::~Dump()
{

}

void Dump::append(const char *fmt, ...)
{
    int len;

    if (!mBuf || !mLen)
        return;

    va_list ap;
    va_start(ap, fmt);
    len = vsnprintf(mBuf, mLen, fmt, ap);
    va_end(ap);

    mLen -= len;
    mBuf += len;
}

} // namespace intel
} // namespace android
