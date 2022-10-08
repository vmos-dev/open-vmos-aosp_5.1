/*--------------------------------------------------------------------------
Copyright (c) 2013 - 2016, The Linux Foundation. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of The Linux Foundation nor
      the names of its contributors may be used to endorse or promote
      products derived from this software without specific prior written
      permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
--------------------------------------------------------------------------*/

#ifndef __VIDC_DEBUG_H__
#define __VIDC_DEBUG_H__

#ifdef _ANDROID_
#include <cstdio>
#include <pthread.h>

enum {
   PRIO_ERROR=0x1,
   PRIO_INFO=0x1,
   PRIO_HIGH=0x2,
   PRIO_LOW=0x4
};

extern int debug_level;

#undef DEBUG_PRINT_ERROR
#define DEBUG_PRINT_ERROR(fmt, args...) \
      if (debug_level & PRIO_ERROR) \
          ALOGE(fmt,##args)
#undef DEBUG_PRINT_INFO
#define DEBUG_PRINT_INFO(fmt, args...) \
      if (debug_level & PRIO_INFO) \
          ALOGI(fmt,##args)
#undef DEBUG_PRINT_LOW
#define DEBUG_PRINT_LOW(fmt, args...) \
      if (debug_level & PRIO_LOW) \
          ALOGD(fmt,##args)
#undef DEBUG_PRINT_HIGH
#define DEBUG_PRINT_HIGH(fmt, args...) \
      if (debug_level & PRIO_HIGH) \
          ALOGD(fmt,##args)
#else
#define DEBUG_PRINT_ERROR printf
#define DEBUG_PRINT_INFO printf
#define DEBUG_PRINT_LOW printf
#define DEBUG_PRINT_HIGH printf
#endif

#define VALIDATE_OMX_PARAM_DATA(ptr, paramType)                                \
    {                                                                          \
        if (ptr == NULL) { return OMX_ErrorBadParameter; }                     \
        paramType *p = reinterpret_cast<paramType *>(ptr);                     \
        if (p->nSize < sizeof(paramType)) {                                    \
            ALOGE("Insufficient object size(%u) v/s expected(%zu) for type %s",\
                    (unsigned int)p->nSize, sizeof(paramType), #paramType);    \
            return OMX_ErrorBadParameter;                                      \
        }                                                                      \
    }                                                                          \

class auto_lock {
    public:
        auto_lock(pthread_mutex_t &lock)
            : mLock(lock) {
                pthread_mutex_lock(&mLock);
            }
        ~auto_lock() {
            pthread_mutex_unlock(&mLock);
        }
    private:
        pthread_mutex_t &mLock;
};

#endif
