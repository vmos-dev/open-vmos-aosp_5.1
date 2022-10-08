/*
 * Copyright (C) 2010 The Android Open Source Project
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

/*
 *
 * @author Yongbae, Song(yb.song@samsung.com)
 *
 * @date   2012-04-02
 *
 */

#include <utils/Log.h>

#include "Exynos_log.h"

void Exynos_Log(EXYNOS_DEV_LOG_LEVEL logLevel, const char *tag, const char *msg, ...)
{
    va_list argptr;

    va_start(argptr, msg);

    switch (logLevel) {
    case EXYNOS_DEV_LOG_DEBUG:
        __android_log_vprint(ANDROID_LOG_DEBUG, tag, msg, argptr);
        break;
    case EXYNOS_DEV_LOG_WARNING:
        __android_log_vprint(ANDROID_LOG_WARN, tag, msg, argptr);
        break;
    case EXYNOS_DEV_LOG_ERROR:
        __android_log_vprint(ANDROID_LOG_ERROR, tag, msg, argptr);
        break;
    default:
        __android_log_vprint(ANDROID_LOG_VERBOSE, tag, msg, argptr);
    }

    va_end(argptr);
}
