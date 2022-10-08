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
 * @file        Exynos_OSAL_SharedMemory.h
 * @brief
 * @author      SeungBeom Kim (sbcrux.kim@samsung.com)
 *              Taehwan Kim (t_h.kim@samsung.com)
 * @version     2.0.0
 * @history
 *   2012.02.20 : Create
 */

#ifndef EXYNOS_OSAL_SHAREDMEMORY
#define EXYNOS_OSAL_SHAREDMEMORY

#include "OMX_Types.h"

typedef enum _MEMORY_TYPE
{
    NORMAL_MEMORY = 0x00,
    SECURE_MEMORY = 0x01,
    SYSTEM_MEMORY = 0x02
} MEMORY_TYPE;

#ifdef __cplusplus
extern "C" {
#endif

OMX_HANDLETYPE Exynos_OSAL_SharedMemory_Open();
void Exynos_OSAL_SharedMemory_Close(OMX_HANDLETYPE handle);
OMX_PTR Exynos_OSAL_SharedMemory_Alloc(OMX_HANDLETYPE handle, OMX_U32 size, MEMORY_TYPE memoryType);
void Exynos_OSAL_SharedMemory_Free(OMX_HANDLETYPE handle, OMX_PTR pBuffer);
int Exynos_OSAL_SharedMemory_VirtToION(OMX_HANDLETYPE handle, OMX_PTR pBuffer);
OMX_PTR Exynos_OSAL_SharedMemory_IONToVirt(OMX_HANDLETYPE handle, int ion_addr);

#ifdef __cplusplus
}
#endif

#endif

