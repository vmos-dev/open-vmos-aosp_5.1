/*
 * log.h, logging helper
 *
 * Copyright (c) 2009-2010 Wind River Systems, Inc.
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

#ifndef __LOG_H
#define __LOG_H

#ifdef ANDROID
 #include <cutils/log.h>
 #define LOGV ALOGV
 #define LOGD ALOGD
 #define LOGI ALOGI
 #define LOGW ALOGW
 #define LOGE ALOGE
 #define LOGV_IF ALOGV_IF
 #define LOGD_IF ALOGD_IF
 #define LOGI_IF ALOGI_IF
 #define LOGW_IF ALOGW_IF
 #define LOGE_IF ALOGE_IF
#else
 #include <stdio.h>
 #define LOG(_p, ...) \
      fprintf(stderr, _p "/" LOG_TAG ": " __VA_ARGS__)
 #define LOGV(...)   LOG("V", __VA_ARGS__)
 #define LOGD(...)   LOG("D", __VA_ARGS__)
 #define LOGI(...)   LOG("I", __VA_ARGS__)
 #define LOGW(...)   LOG("W", __VA_ARGS__)
 #define LOGE(...)   LOG("E", __VA_ARGS__)
#endif

#endif /* __LOG_H */
