/*
* Copyright (c) 2009-2011 Intel Corporation.  All rights reserved.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/



#include "VideoDecoderTrace.h"

#ifdef ENABLE_VIDEO_DECODER_TRACE

void TraceVideoDecoder(const char* cat, const char* fun, int line, const char* format, ...)
{
    if (NULL == cat || NULL == fun || NULL == format)
        return;

    printf("%s %s(#%d): ", cat, fun, line);
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("\n");
}

#endif

