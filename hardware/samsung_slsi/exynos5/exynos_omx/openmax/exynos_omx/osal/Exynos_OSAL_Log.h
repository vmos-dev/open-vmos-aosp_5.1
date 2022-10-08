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
 * @file        Exynos_OSAL_Log.h
 * @brief
 * @author      Yunji Kim (yunji.kim@samsung.com)
 * @version     2.0.0
 * @history
 *   2012.02.20 : Create
 *   2012.8.27 : Add trace function
 */

#ifndef EXYNOS_OSAL_LOG
#define EXYNOS_OSAL_LOG

#ifdef __cplusplus
extern "C" {
#endif

#ifndef EXYNOS_LOG_OFF
#define EXYNOS_LOG
#endif

#ifndef EXYNOS_LOG_TAG
#define EXYNOS_LOG_TAG    "EXYNOS_LOG"
#endif

#ifdef EXYNOS_TRACE_ON
#define EXYNOS_TRACE
#endif

typedef enum _LOG_LEVEL
{
    EXYNOS_LOG_TRACE,
    EXYNOS_LOG_INFO,
    EXYNOS_LOG_WARNING,
    EXYNOS_LOG_ERROR
} EXYNOS_LOG_LEVEL;

#ifdef EXYNOS_LOG
#define Exynos_OSAL_Log(a, ...)    ((void)_Exynos_OSAL_Log(a, EXYNOS_LOG_TAG, __VA_ARGS__))
#else
#define Exynos_OSAL_Log(a, ...)                                                \
    do {                                                                \
        if (a == EXYNOS_LOG_ERROR)                                     \
            ((void)_Exynos_OSAL_Log(a, EXYNOS_LOG_TAG, __VA_ARGS__)); \
    } while (0)
#endif

#ifdef EXYNOS_TRACE
#define FunctionIn() _Exynos_OSAL_Log(EXYNOS_LOG_TRACE, EXYNOS_LOG_TAG, "%s In , Line: %d", __FUNCTION__, __LINE__)
#define FunctionOut() _Exynos_OSAL_Log(EXYNOS_LOG_TRACE, EXYNOS_LOG_TAG, "%s Out , Line: %d", __FUNCTION__, __LINE__)
#else
#define FunctionIn() ((void *)0)
#define FunctionOut() ((void *)0)
#endif

extern void _Exynos_OSAL_Log(EXYNOS_LOG_LEVEL logLevel, const char *tag, const char *msg, ...);

#ifdef __cplusplus
}
#endif

#endif
