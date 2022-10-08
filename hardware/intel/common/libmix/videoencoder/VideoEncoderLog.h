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

#ifndef __VIDEO_ENCODER_LOG_H__
#define __VIDEO_ENCODER_LOG_H__

#define LOG_TAG "VideoEncoder"

#include <wrs_omxil_core/log.h>

#define LOG_V ALOGV
#define LOG_D ALOGD
#define LOG_I ALOGI
#define LOG_W ALOGW
#define LOG_E ALOGE

extern int32_t gLogLevel;
#define CHECK_VA_STATUS_RETURN(FUNC)\
    if (vaStatus != VA_STATUS_SUCCESS) {\
        LOG_E(FUNC" failed. vaStatus = %d\n", vaStatus);\
        return ENCODE_DRIVER_FAIL;\
    }

#define CHECK_VA_STATUS_GOTO_CLEANUP(FUNC)\
    if (vaStatus != VA_STATUS_SUCCESS) {\
        LOG_E(FUNC" failed. vaStatus = %d\n", vaStatus);\
        ret = ENCODE_DRIVER_FAIL; \
        goto CLEAN_UP;\
    }

#define CHECK_ENCODE_STATUS_RETURN(FUNC)\
    if (ret != ENCODE_SUCCESS) { \
        LOG_E(FUNC"Failed. ret = 0x%08x\n", ret); \
        return ret; \
    }

#define CHECK_ENCODE_STATUS_CLEANUP(FUNC)\
    if (ret != ENCODE_SUCCESS) { \
        LOG_E(FUNC"Failed, ret = 0x%08x\n", ret); \
        goto CLEAN_UP;\
    }

#define CHECK_NULL_RETURN_IFFAIL(POINTER)\
    if (POINTER == NULL) { \
        LOG_E("Invalid pointer\n"); \
        return ENCODE_NULL_PTR;\
    }
#endif /*  __VIDEO_ENCODER_LOG_H__ */
