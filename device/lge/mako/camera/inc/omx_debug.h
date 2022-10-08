/* Copyright (c) 2012, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef _OMXLOG_H
#define _OMXGLOG_H

#define OMX_DBG_ERROR_ENABLE 1
#define OMX_DBG_WARNG_ENABLE 0
#define OMX_DBG_HIGH_ENABLE 1
#define OMX_DBG_INFO_ENABLE  0

#ifdef ANDROID
  #define LOG_NIDEBUG 0
  #define LOG_TAG "mm-still-omx"
  #include <utils/Log.h>
  #ifdef NEW_LOG_API
    #define OMXDBG(fmt, args...) ALOGV(fmt, ##args)
  #else
    #define OMXDBG(fmt, args...) LOGV(fmt, ##args)
  #endif
#endif

#if(OMX_DBG_ERROR_ENABLE)
  #define OMX_DBG_ERROR(...)  OMXDBG(__VA_ARGS__)
#else
  #define OMX_DBG_ERROR(...)  do{}while(0)
#endif

#if(OMX_DBG_WARNG_ENABLE)
  #define OMX_DBG_WARNG(...)  OMXDBG(__VA_ARGS__)
#else
  #define OMX_DBG_WARNG(...)  do{}while(0)
#endif


#if(OMX_DBG_INFO_ENABLE)
  #define OMX_DBG_INFO(...)   OMXDBG(__VA_ARGS__)
#else
  #define OMX_DBG_INFO(...)  do{}while(0)
#endif

#if(OMX_DBG_HIGH_ENABLE)
  #define OMX_DBG_HIGH(...)   OMXDBG(__VA_ARGS__)
#else
  #define OMX_DBG_HIGH(...)  do{}while(0)
#endif


#endif /* _OMXGLOG_H */
