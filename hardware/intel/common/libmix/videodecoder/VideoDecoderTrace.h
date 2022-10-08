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


#ifndef VIDEO_DECODER_TRACE_H_
#define VIDEO_DECODER_TRACE_H_


#define ENABLE_VIDEO_DECODER_TRACE
//#define ANDROID


#ifdef ENABLE_VIDEO_DECODER_TRACE

#ifndef ANDROID

#include <stdio.h>
#include <stdarg.h>

extern void TraceVideoDecoder(const char* cat, const char* fun, int line, const char* format, ...);
#define VIDEO_DECODER_TRACE(cat, format, ...) \
TraceVideoDecoder(cat, __FUNCTION__, __LINE__, format,  ##__VA_ARGS__)

#define ETRACE(format, ...) VIDEO_DECODER_TRACE("ERROR:   ",  format, ##__VA_ARGS__)
#define WTRACE(format, ...) VIDEO_DECODER_TRACE("WARNING: ",  format, ##__VA_ARGS__)
#define ITRACE(format, ...) VIDEO_DECODER_TRACE("INFO:    ",  format, ##__VA_ARGS__)
#define VTRACE(format, ...) VIDEO_DECODER_TRACE("VERBOSE: ",  format, ##__VA_ARGS__)

#else
// for Android OS

//#define LOG_NDEBUG 0

#define LOG_TAG "VideoDecoder"

#include <wrs_omxil_core/log.h>
#define ETRACE(...) LOGE(__VA_ARGS__)
#define WTRACE(...) LOGW(__VA_ARGS__)
#define ITRACE(...) LOGI(__VA_ARGS__)
#define VTRACE(...) LOGV(__VA_ARGS__)

#endif


#else

#define ETRACE(format, ...)
#define WTRACE(format, ...)
#define ITRACE(format, ...)
#define VTRACE(format, ...)


#endif /* ENABLE_VIDEO_DECODER_TRACE*/


#define CHECK_STATUS(FUNC)\
    if (status != DECODE_SUCCESS) {\
        if (status > DECODE_SUCCESS) {\
            WTRACE(FUNC" failed. status = %d", status);\
        } else {\
            ETRACE(FUNC" failed. status = %d", status);\
        }\
        return status;\
     }

#define CHECK_VA_STATUS(FUNC)\
    if (vaStatus != VA_STATUS_SUCCESS) {\
        ETRACE(FUNC" failed. vaStatus = 0x%x", vaStatus);\
        return DECODE_DRIVER_FAIL;\
    }

#define CHECK_VBP_STATUS(FUNC)\
    if (vbpStatus != VBP_OK) {\
        ETRACE(FUNC" failed. vbpStatus = %d", (int)vbpStatus);\
        if (vbpStatus == VBP_ERROR) {\
            return DECODE_FAIL;\
        }\
        return DECODE_PARSER_FAIL;\
    }

#endif /*VIDEO_DECODER_TRACE_H_*/


