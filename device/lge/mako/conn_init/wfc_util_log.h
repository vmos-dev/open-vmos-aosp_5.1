/*
 * Copyright 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __WFC_UTIL_LOG_H__
#define __WFC_UTIL_LOG_H__

#ifdef ANDROID
#define LOG_TAG "WifiUtil"
#include <cutils/log.h>

#define wfc_util_log_info(...)  ALOGI(__VA_ARGS__)
#define wfc_util_log_error(...) ALOGE(__VA_ARGS__)
#else  /* ANDROID */
#define wfc_util_log_info(...)  printf(__VA_ARGS__);printf("\n")
#define wfc_util_log_error(...) printf(__VA_ARGS__);printf("\n")
#endif /* ANDROID */

#endif

