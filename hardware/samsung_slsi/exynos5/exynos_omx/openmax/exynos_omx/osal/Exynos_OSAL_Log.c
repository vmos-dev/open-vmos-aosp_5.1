/*
 *
 * Copyright 2012 Samsung Electronics S.LSI Co. LTD
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
 * @file        Exynos_OSAL_Log.c
 * @brief
 * @author      Yunji Kim (yunji.kim@samsung.com)
 * @version     2.0.0
 * @history
 *   2012.02.20 : Create
 */

#include <utils/Log.h>

#include "Exynos_OSAL_Log.h"


void _Exynos_OSAL_Log(EXYNOS_LOG_LEVEL logLevel, const char *tag, const char *msg, ...)
{
    va_list argptr;

    va_start(argptr, msg);

    switch (logLevel) {
    case EXYNOS_LOG_TRACE:
        __android_log_vprint(ANDROID_LOG_DEBUG, tag, msg, argptr);
        break;
    case EXYNOS_LOG_INFO:
        __android_log_vprint(ANDROID_LOG_INFO, tag, msg, argptr);
        break;
    case EXYNOS_LOG_WARNING:
        __android_log_vprint(ANDROID_LOG_WARN, tag, msg, argptr);
        break;
    case EXYNOS_LOG_ERROR:
        __android_log_vprint(ANDROID_LOG_ERROR, tag, msg, argptr);
        break;
    default:
        __android_log_vprint(ANDROID_LOG_VERBOSE, tag, msg, argptr);
    }

    va_end(argptr);
}
