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
 * @file       Exynos_OMX_Resourcemanager.h
 * @brief
 * @author     SeungBeom Kim (sbcrux.kim@samsung.com)
 * @version    2.0.0
 * @history
 *    2012.02.20 : Create
 */

#ifndef EXYNOS_OMX_RESOURCEMANAGER
#define EXYNOS_OMX_RESOURCEMANAGER


#include "Exynos_OMX_Def.h"
#include "OMX_Component.h"


struct EXYNOS_OMX_RM_COMPONENT_LIST;
typedef struct _EXYNOS_OMX_RM_COMPONENT_LIST
{
    OMX_COMPONENTTYPE         *pOMXStandComp;
    OMX_U32                    groupPriority;
    struct _EXYNOS_OMX_RM_COMPONENT_LIST *pNext;
} EXYNOS_OMX_RM_COMPONENT_LIST;


#ifdef __cplusplus
extern "C" {
#endif

OMX_ERRORTYPE Exynos_OMX_ResourceManager_Init();
OMX_ERRORTYPE Exynos_OMX_ResourceManager_Deinit();
OMX_ERRORTYPE Exynos_OMX_Get_Resource(OMX_COMPONENTTYPE *pOMXComponent);
OMX_ERRORTYPE Exynos_OMX_Release_Resource(OMX_COMPONENTTYPE *pOMXComponent);
OMX_ERRORTYPE Exynos_OMX_In_WaitForResource(OMX_COMPONENTTYPE *pOMXComponent);
OMX_ERRORTYPE Exynos_OMX_Out_WaitForResource(OMX_COMPONENTTYPE *pOMXComponent);

#ifdef __cplusplus
};
#endif

#endif
