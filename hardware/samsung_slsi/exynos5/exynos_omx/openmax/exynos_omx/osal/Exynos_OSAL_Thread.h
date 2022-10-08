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
 * @file        Exynos_OSAL_Thread.h
 * @brief
 * @author      SeungBeom Kim (sbcrux.kim@samsung.com)
 * @version     2.0.0
 * @history
 *   2012.02.20 : Create
 */

#ifndef Exynos_OSAL_THREAD
#define Exynos_OSAL_THREAD

#include "OMX_Types.h"
#include "OMX_Core.h"


#ifdef __cplusplus
extern "C" {
#endif

OMX_ERRORTYPE Exynos_OSAL_ThreadCreate(OMX_HANDLETYPE *threadHandle, OMX_PTR function_name, OMX_PTR argument);
OMX_ERRORTYPE Exynos_OSAL_ThreadTerminate(OMX_HANDLETYPE threadHandle);
OMX_ERRORTYPE Exynos_OSAL_ThreadCancel(OMX_HANDLETYPE threadHandle);
void          Exynos_OSAL_ThreadExit(void *value_ptr);
void          Exynos_OSAL_SleepMillisec(OMX_U32 ms);

#ifdef __cplusplus
}
#endif

#endif
