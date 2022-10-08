/*
 *
 * Copyright 2010 Samsung Electronics S.LSI Co. LTD
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
 * @file        Exynos_log.h
 * @brief
 * @author      Yongbae, Song(yb.songsamsung.com)
 * @version     1.0.0
 * @history
 *   2012.4.02 : Create
 */

#ifndef EXYNOS_LOG
#define EXYNOS_LOG

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    EXYNOS_DEV_LOG_DEBUG,
    EXYNOS_DEV_LOG_INFO,
    EXYNOS_DEV_LOG_WARNING,
    EXYNOS_DEV_LOG_ERROR
} EXYNOS_DEV_LOG_LEVEL;

void Exynos_Log(EXYNOS_DEV_LOG_LEVEL logLevel, const char *tag, const char *msg, ...);

#ifdef __cplusplus
}
#endif

#endif
