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
 * @file       Exynos_OMX_Core.c
 * @brief      Exynos OpenMAX IL Core
 * @author     SeungBeom Kim (sbcrux.kim@samsung.com)
 *             HyeYeon Chung (hyeon.chung@samsung.com)
 *             Yunji Kim (yunji.kim@samsung.com)
 * @version    2.0.0
 * @history
 *    2012.02.20 : Create
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Exynos_OMX_Core.h"
#include "Exynos_OMX_Component_Register.h"
#include "Exynos_OSAL_Memory.h"
#include "Exynos_OSAL_Mutex.h"
#include "Exynos_OSAL_ETC.h"
#include "Exynos_OMX_Resourcemanager.h"

#undef  EXYNOS_LOG_TAG
#define EXYNOS_LOG_TAG    "EXYNOS_OMX_CORE"
#define EXYNOS_LOG_OFF
#include "Exynos_OSAL_Log.h"


static int gInitialized = 0;
static OMX_U32 gComponentNum = 0;

static EXYNOS_OMX_COMPONENT_REGLIST *gComponentList = NULL;
static EXYNOS_OMX_COMPONENT *gLoadComponentList = NULL;
static OMX_HANDLETYPE ghLoadComponentListMutex = NULL;


OMX_API OMX_ERRORTYPE OMX_APIENTRY Exynos_OMX_Init(void)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    FunctionIn();

    if (gInitialized == 0) {
        if (Exynos_OMX_Component_Register(&gComponentList, &gComponentNum)) {
            ret = OMX_ErrorInsufficientResources;
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Exynos_OMX_Init : %s", "OMX_ErrorInsufficientResources");
            goto EXIT;
        }

        ret = Exynos_OMX_ResourceManager_Init();
        if (OMX_ErrorNone != ret) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Exynos_OMX_Init : Exynos_OMX_ResourceManager_Init failed");
            goto EXIT;
        }

        ret = Exynos_OSAL_MutexCreate(&ghLoadComponentListMutex);
        if (OMX_ErrorNone != ret) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Exynos_OMX_Init : Exynos_OSAL_MutexCreate(&ghLoadComponentListMutex) failed");
            goto EXIT;
        }

        gInitialized = 1;
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "Exynos_OMX_Init : %s", "OMX_ErrorNone");
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_API OMX_ERRORTYPE OMX_APIENTRY Exynos_OMX_Deinit(void)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    FunctionIn();

    Exynos_OSAL_MutexTerminate(ghLoadComponentListMutex);
    ghLoadComponentListMutex = NULL;

    Exynos_OMX_ResourceManager_Deinit();

    if (OMX_ErrorNone != Exynos_OMX_Component_Unregister(gComponentList)) {
        ret = OMX_ErrorUndefined;
        goto EXIT;
    }
    gComponentList = NULL;
    gComponentNum = 0;
    gInitialized = 0;

EXIT:
    FunctionOut();

    return ret;
}

OMX_API OMX_ERRORTYPE OMX_APIENTRY Exynos_OMX_ComponentNameEnum(
    OMX_OUT OMX_STRING cComponentName,
    OMX_IN  OMX_U32 nNameLength,
    OMX_IN  OMX_U32 nIndex)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    FunctionIn();

    if (nIndex >= gComponentNum) {
        ret = OMX_ErrorNoMore;
        goto EXIT;
    }

    snprintf(cComponentName, nNameLength, "%s", gComponentList[nIndex].component.componentName);
    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_API OMX_ERRORTYPE OMX_APIENTRY Exynos_OMX_GetHandle(
    OMX_OUT OMX_HANDLETYPE *pHandle,
    OMX_IN  OMX_STRING cComponentName,
    OMX_IN  OMX_PTR pAppData,
    OMX_IN  OMX_CALLBACKTYPE *pCallBacks)
{
    OMX_ERRORTYPE         ret = OMX_ErrorNone;
    EXYNOS_OMX_COMPONENT *loadComponent;
    EXYNOS_OMX_COMPONENT *currentComponent;
    unsigned int i = 0;

    FunctionIn();

    if (gInitialized != 1) {
        ret = OMX_ErrorNotReady;
        goto EXIT;
    }

    if ((pHandle == NULL) || (cComponentName == NULL) || (pCallBacks == NULL)) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "ComponentName : %s", cComponentName);

    for (i = 0; i < gComponentNum; i++) {
        if (Exynos_OSAL_Strcmp(cComponentName, gComponentList[i].component.componentName) == 0) {
            loadComponent = Exynos_OSAL_Malloc(sizeof(EXYNOS_OMX_COMPONENT));
            Exynos_OSAL_Memset(loadComponent, 0, sizeof(EXYNOS_OMX_COMPONENT));

            Exynos_OSAL_Strcpy(loadComponent->libName, gComponentList[i].libName);
            Exynos_OSAL_Strcpy(loadComponent->componentName, gComponentList[i].component.componentName);
            ret = Exynos_OMX_ComponentLoad(loadComponent);
            if (ret != OMX_ErrorNone) {
                Exynos_OSAL_Free(loadComponent);
                Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_Error, Line:%d", __LINE__);
                goto EXIT;
            }

            ret = loadComponent->pOMXComponent->SetCallbacks(loadComponent->pOMXComponent, pCallBacks, pAppData);
            if (ret != OMX_ErrorNone) {
                Exynos_OMX_ComponentUnload(loadComponent);
                Exynos_OSAL_Free(loadComponent);
                Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_Error, Line:%d", __LINE__);
                goto EXIT;
            }

            Exynos_OSAL_MutexLock(ghLoadComponentListMutex);
            if (gLoadComponentList == NULL) {
                gLoadComponentList = loadComponent;
            } else {
                currentComponent = gLoadComponentList;
                while (currentComponent->nextOMXComp != NULL) {
                    currentComponent = currentComponent->nextOMXComp;
                }
                currentComponent->nextOMXComp = loadComponent;
            }
            Exynos_OSAL_MutexUnlock(ghLoadComponentListMutex);

            *pHandle = loadComponent->pOMXComponent;
            ret = OMX_ErrorNone;
            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "Exynos_OMX_GetHandle : %s", "OMX_ErrorNone");
            goto EXIT;
        }
    }

    ret = OMX_ErrorComponentNotFound;

EXIT:
    FunctionOut();

    return ret;
}

OMX_API OMX_ERRORTYPE OMX_APIENTRY Exynos_OMX_FreeHandle(OMX_IN OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE         ret = OMX_ErrorNone;
    EXYNOS_OMX_COMPONENT *currentComponent;
    EXYNOS_OMX_COMPONENT *deleteComponent;

    FunctionIn();

    if (gInitialized != 1) {
        ret = OMX_ErrorNotReady;
        goto EXIT;
    }

    if (!hComponent) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    Exynos_OSAL_MutexLock(ghLoadComponentListMutex);
    currentComponent = gLoadComponentList;
    if (gLoadComponentList->pOMXComponent == hComponent) {
        deleteComponent = gLoadComponentList;
        gLoadComponentList = gLoadComponentList->nextOMXComp;
    } else {
        while ((currentComponent != NULL) && (((EXYNOS_OMX_COMPONENT *)(currentComponent->nextOMXComp))->pOMXComponent != hComponent))
            currentComponent = currentComponent->nextOMXComp;

        if (((EXYNOS_OMX_COMPONENT *)(currentComponent->nextOMXComp))->pOMXComponent == hComponent) {
            deleteComponent = currentComponent->nextOMXComp;
            currentComponent->nextOMXComp = deleteComponent->nextOMXComp;
        } else if (currentComponent == NULL) {
            ret = OMX_ErrorComponentNotFound;
            Exynos_OSAL_MutexUnlock(ghLoadComponentListMutex);
            goto EXIT;
        }
    }
    Exynos_OSAL_MutexUnlock(ghLoadComponentListMutex);

    Exynos_OMX_ComponentUnload(deleteComponent);
    Exynos_OSAL_Free(deleteComponent);

EXIT:
    FunctionOut();

    return ret;
}

OMX_API OMX_ERRORTYPE OMX_APIENTRY Exynos_OMX_SetupTunnel(
    OMX_IN OMX_HANDLETYPE hOutput,
    OMX_IN OMX_U32 nPortOutput,
    OMX_IN OMX_HANDLETYPE hInput,
    OMX_IN OMX_U32 nPortInput)
{
    OMX_ERRORTYPE ret = OMX_ErrorNotImplemented;

EXIT:
    return ret;
}

OMX_API OMX_ERRORTYPE Exynos_OMX_GetContentPipe(
    OMX_OUT OMX_HANDLETYPE *hPipe,
    OMX_IN  OMX_STRING szURI)
{
    OMX_ERRORTYPE ret = OMX_ErrorNotImplemented;

EXIT:
    return ret;
}

OMX_API OMX_ERRORTYPE Exynos_OMX_GetComponentsOfRole (
    OMX_IN    OMX_STRING role,
    OMX_INOUT OMX_U32 *pNumComps,
    OMX_INOUT OMX_U8  **compNames)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    int           max_role_num = 0;
    OMX_STRING    RoleString[MAX_OMX_COMPONENT_ROLE_SIZE];
    int i = 0, j = 0;

    FunctionIn();

    if (gInitialized != 1) {
        ret = OMX_ErrorNotReady;
        goto EXIT;
    }

    *pNumComps = 0;

    for (i = 0; i < MAX_OMX_COMPONENT_NUM; i++) {
        max_role_num = gComponentList[i].component.totalRoleNum;

        for (j = 0; j < max_role_num; j++) {
            if (Exynos_OSAL_Strcmp(gComponentList[i].component.roles[j], role) == 0) {
                if (compNames != NULL) {
                    Exynos_OSAL_Strcpy((OMX_STRING)compNames[*pNumComps], gComponentList[i].component.componentName);
                }
                *pNumComps = (*pNumComps + 1);
            }
        }
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_API OMX_ERRORTYPE Exynos_OMX_GetRolesOfComponent (
    OMX_IN    OMX_STRING compName,
    OMX_INOUT OMX_U32 *pNumRoles,
    OMX_OUT   OMX_U8 **roles)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_BOOL      detectComp = OMX_FALSE;
    int           compNum = 0, totalRoleNum = 0;
    int i = 0;

    FunctionIn();

    if (gInitialized != 1) {
        ret = OMX_ErrorNotReady;
        goto EXIT;
    }

    for (i = 0; i < MAX_OMX_COMPONENT_NUM; i++) {
        if (gComponentList != NULL) {
            if (Exynos_OSAL_Strcmp(gComponentList[i].component.componentName, compName) == 0) {
                *pNumRoles = totalRoleNum = gComponentList[i].component.totalRoleNum;
                compNum = i;
                detectComp = OMX_TRUE;
                break;
            }
        } else {
            ret = OMX_ErrorUndefined;
            goto EXIT;
        }
    }

    if (detectComp == OMX_FALSE) {
        *pNumRoles = 0;
        ret = OMX_ErrorComponentNotFound;
        goto EXIT;
    }

    if (roles != NULL) {
        for (i = 0; i < totalRoleNum; i++) {
            Exynos_OSAL_Strcpy(roles[i], gComponentList[compNum].component.roles[i]);
        }
    }

EXIT:
    FunctionOut();

    return ret;
}
