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
 * @file        library_register.c
 * @brief
 * @author      Yunji Kim (yunji.kim@samsung.com)
 * @version     2.0.0
 * @history
 *   2012.02.20 : Create
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

#include "Exynos_OSAL_Memory.h"
#include "Exynos_OSAL_ETC.h"
#include "library_register.h"

#undef  EXYNOS_LOG_TAG
#define EXYNOS_LOG_TAG    "EXYNOS_MPEG4_ENC"
#define EXYNOS_LOG_OFF
#include "Exynos_OSAL_Log.h"

OSCL_EXPORT_REF int Exynos_OMX_COMPONENT_Library_Register(ExynosRegisterComponentType **ppExynosComponent)
{
    FunctionIn();

    if (ppExynosComponent == NULL)
        goto EXIT;

    /* component 1 - video encoder MPEG4 */
    Exynos_OSAL_Strcpy(ppExynosComponent[0]->componentName, EXYNOS_OMX_COMPONENT_MPEG4_ENC);
    Exynos_OSAL_Strcpy(ppExynosComponent[0]->roles[0], EXYNOS_OMX_COMPONENT_MPEG4_ENC_ROLE);
    ppExynosComponent[0]->totalRoleNum = MAX_COMPONENT_ROLE_NUM;

    /* component 2 - video encoder H.263 */
    Exynos_OSAL_Strcpy(ppExynosComponent[1]->componentName, EXYNOS_OMX_COMPONENT_H263_ENC);
    Exynos_OSAL_Strcpy(ppExynosComponent[1]->roles[0], EXYNOS_OMX_COMPONENT_H263_ENC_ROLE);
    ppExynosComponent[1]->totalRoleNum = MAX_COMPONENT_ROLE_NUM;

EXIT:
    FunctionOut();

    return MAX_COMPONENT_NUM;
}
