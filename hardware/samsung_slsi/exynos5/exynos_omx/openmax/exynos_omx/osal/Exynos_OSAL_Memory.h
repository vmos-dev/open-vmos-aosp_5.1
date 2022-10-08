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
 * @file        Exynos_OSAL_Memory.h
 * @brief
 * @author      SeungBeom Kim (sbcrux.kim@samsung.com)
 * @version     2.0.0
 * @history
 *   2012.02.20 : Create
 */

#ifndef Exynos_OSAL_MEMORY
#define Exynos_OSAL_MEMORY

#include "OMX_Types.h"


#ifdef __cplusplus
extern "C" {
#endif

OMX_PTR Exynos_OSAL_Malloc(OMX_U32 size);
void    Exynos_OSAL_Free(OMX_PTR addr);
OMX_PTR Exynos_OSAL_Memset(OMX_PTR dest, OMX_S32 c, OMX_S32 n);
OMX_PTR Exynos_OSAL_Memcpy(OMX_PTR dest, OMX_PTR src, OMX_S32 n);
OMX_PTR Exynos_OSAL_Memmove(OMX_PTR dest, OMX_PTR src, OMX_S32 n);

#ifdef __cplusplus
}
#endif

#endif

