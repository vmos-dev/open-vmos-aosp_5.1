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
 * @file       Exynos_OMX_Component_Register.c
 * @brief      Exynos OpenMAX IL Component Register
 * @author     SeungBeom Kim (sbcrux.kim@samsung.com)
 * @version    2.0.0
 * @history
 *    2012.02.20 : Create
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <assert.h>
#include <dirent.h>

#include "OMX_Component.h"
#include "Exynos_OSAL_Memory.h"
#include "Exynos_OSAL_ETC.h"
#include "Exynos_OSAL_Library.h"
#include "Exynos_OMX_Component_Register.h"
#include "Exynos_OMX_Macros.h"

#undef  EXYNOS_LOG_TAG
#define EXYNOS_LOG_TAG    "EXYNOS_COMP_REGS"
#define EXYNOS_LOG_OFF
#include "Exynos_OSAL_Log.h"

OMX_ERRORTYPE Exynos_OMX_Component_Register(EXYNOS_OMX_COMPONENT_REGLIST **compList, OMX_U32 *compNum)
{
    OMX_ERRORTYPE  ret = OMX_ErrorNone;
    int            componentNum = 0, roleNum = 0, totalCompNum = 0;
    int            read;
    char          *libName;
    size_t         len;
    const char    *errorMsg;
    DIR           *dir;
    struct dirent *d;

    int (*Exynos_OMX_COMPONENT_Library_Register)(ExynosRegisterComponentType **exynosComponents);
    ExynosRegisterComponentType **exynosComponentsTemp;
    EXYNOS_OMX_COMPONENT_REGLIST *componentList;

    FunctionIn();

    dir = opendir(EXYNOS_OMX_INSTALL_PATH);
    if (dir == NULL) {
        ret = OMX_ErrorUndefined;
        goto EXIT;
    }

    componentList = (EXYNOS_OMX_COMPONENT_REGLIST *)Exynos_OSAL_Malloc(sizeof(EXYNOS_OMX_COMPONENT_REGLIST) * MAX_OMX_COMPONENT_NUM);
    Exynos_OSAL_Memset(componentList, 0, sizeof(EXYNOS_OMX_COMPONENT_REGLIST) * MAX_OMX_COMPONENT_NUM);
    libName = Exynos_OSAL_Malloc(MAX_OMX_COMPONENT_LIBNAME_SIZE);

    while ((d = readdir(dir)) != NULL) {
        OMX_HANDLETYPE soHandle;
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "%s", d->d_name);

        if (Exynos_OSAL_Strncmp(d->d_name, "libOMX.Exynos.", Exynos_OSAL_Strlen("libOMX.Exynos.")) == 0) {
            Exynos_OSAL_Memset(libName, 0, MAX_OMX_COMPONENT_LIBNAME_SIZE);
            Exynos_OSAL_Strcpy(libName, EXYNOS_OMX_INSTALL_PATH);
            Exynos_OSAL_Strcat(libName, d->d_name);
            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "Path & libName : %s", libName);
            if ((soHandle = Exynos_OSAL_dlopen(libName, RTLD_NOW)) != NULL) {
                Exynos_OSAL_dlerror();    /* clear error*/
                if ((Exynos_OMX_COMPONENT_Library_Register = Exynos_OSAL_dlsym(soHandle, "Exynos_OMX_COMPONENT_Library_Register")) != NULL) {
                    int i = 0;
                    unsigned int j = 0;

                    componentNum = (*Exynos_OMX_COMPONENT_Library_Register)(NULL);
                    exynosComponentsTemp = (ExynosRegisterComponentType **)Exynos_OSAL_Malloc(sizeof(ExynosRegisterComponentType*) * componentNum);
                    for (i = 0; i < componentNum; i++) {
                        exynosComponentsTemp[i] = Exynos_OSAL_Malloc(sizeof(ExynosRegisterComponentType));
                        Exynos_OSAL_Memset(exynosComponentsTemp[i], 0, sizeof(ExynosRegisterComponentType));
                    }
                    (*Exynos_OMX_COMPONENT_Library_Register)(exynosComponentsTemp);

                    for (i = 0; i < componentNum; i++) {
                        Exynos_OSAL_Strcpy(componentList[totalCompNum].component.componentName, exynosComponentsTemp[i]->componentName);
                        for (j = 0; j < exynosComponentsTemp[i]->totalRoleNum; j++)
                            Exynos_OSAL_Strcpy(componentList[totalCompNum].component.roles[j], exynosComponentsTemp[i]->roles[j]);
                        componentList[totalCompNum].component.totalRoleNum = exynosComponentsTemp[i]->totalRoleNum;

                        Exynos_OSAL_Strcpy(componentList[totalCompNum].libName, libName);

                        totalCompNum++;
                    }
                    for (i = 0; i < componentNum; i++) {
                        Exynos_OSAL_Free(exynosComponentsTemp[i]);
                    }

                    Exynos_OSAL_Free(exynosComponentsTemp);
                } else {
                    if ((errorMsg = Exynos_OSAL_dlerror()) != NULL)
                        Exynos_OSAL_Log(EXYNOS_LOG_WARNING, "dlsym failed: %s", errorMsg);
                }
                Exynos_OSAL_dlclose(soHandle);
            } else {
                Exynos_OSAL_Log(EXYNOS_LOG_WARNING, "dlopen failed: %s", Exynos_OSAL_dlerror());
            }
        } else {
            /* not a component name line. skip */
            continue;
        }
    }

    Exynos_OSAL_Free(libName);

    closedir(dir);

    *compList = componentList;
    *compNum = totalCompNum;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_Component_Unregister(EXYNOS_OMX_COMPONENT_REGLIST *componentList)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    Exynos_OSAL_Free(componentList);

EXIT:
    return ret;
}

OMX_ERRORTYPE Exynos_OMX_ComponentAPICheck(OMX_COMPONENTTYPE *component)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    if ((NULL == component->GetComponentVersion)    ||
        (NULL == component->SendCommand)            ||
        (NULL == component->GetParameter)           ||
        (NULL == component->SetParameter)           ||
        (NULL == component->GetConfig)              ||
        (NULL == component->SetConfig)              ||
        (NULL == component->GetExtensionIndex)      ||
        (NULL == component->GetState)               ||
        (NULL == component->ComponentTunnelRequest) ||
        (NULL == component->UseBuffer)              ||
        (NULL == component->AllocateBuffer)         ||
        (NULL == component->FreeBuffer)             ||
        (NULL == component->EmptyThisBuffer)        ||
        (NULL == component->FillThisBuffer)         ||
        (NULL == component->SetCallbacks)           ||
        (NULL == component->ComponentDeInit)        ||
        (NULL == component->UseEGLImage)            ||
        (NULL == component->ComponentRoleEnum))
        ret = OMX_ErrorInvalidComponent;
    else
        ret = OMX_ErrorNone;

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_ComponentLoad(EXYNOS_OMX_COMPONENT *exynos_component)
{
    OMX_ERRORTYPE      ret = OMX_ErrorNone;
    OMX_HANDLETYPE     libHandle;
    OMX_COMPONENTTYPE *pOMXComponent;

    FunctionIn();

    OMX_ERRORTYPE (*Exynos_OMX_ComponentInit)(OMX_HANDLETYPE hComponent, OMX_STRING componentName);

    libHandle = Exynos_OSAL_dlopen((OMX_STRING)exynos_component->libName, RTLD_NOW);
    if (!libHandle) {
        ret = OMX_ErrorInvalidComponentName;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorInvalidComponentName, Line:%d", __LINE__);
        goto EXIT;
    }

    Exynos_OMX_ComponentInit = Exynos_OSAL_dlsym(libHandle, "Exynos_OMX_ComponentInit");
    if (!Exynos_OMX_ComponentInit) {
        Exynos_OSAL_dlclose(libHandle);
        ret = OMX_ErrorInvalidComponent;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorInvalidComponent, Line:%d", __LINE__);
        goto EXIT;
    }

    pOMXComponent = (OMX_COMPONENTTYPE *)Exynos_OSAL_Malloc(sizeof(OMX_COMPONENTTYPE));
    INIT_SET_SIZE_VERSION(pOMXComponent, OMX_COMPONENTTYPE);
    ret = (*Exynos_OMX_ComponentInit)((OMX_HANDLETYPE)pOMXComponent, (OMX_STRING)exynos_component->componentName);
    if (ret != OMX_ErrorNone) {
        Exynos_OSAL_Free(pOMXComponent);
        Exynos_OSAL_dlclose(libHandle);
        ret = OMX_ErrorInvalidComponent;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorInvalidComponent, Line:%d", __LINE__);
        goto EXIT;
    } else {
        if (Exynos_OMX_ComponentAPICheck(pOMXComponent) != OMX_ErrorNone) {
            if (NULL != pOMXComponent->ComponentDeInit)
                pOMXComponent->ComponentDeInit(pOMXComponent);
            Exynos_OSAL_Free(pOMXComponent);
            Exynos_OSAL_dlclose(libHandle);
            ret = OMX_ErrorInvalidComponent;
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorInvalidComponent, Line:%d", __LINE__);
            goto EXIT;
        }
        exynos_component->libHandle = libHandle;
        exynos_component->pOMXComponent = pOMXComponent;
        ret = OMX_ErrorNone;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_ComponentUnload(EXYNOS_OMX_COMPONENT *exynos_component)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pOMXComponent = NULL;

    FunctionIn();

    if (!exynos_component) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pOMXComponent = exynos_component->pOMXComponent;
    if (pOMXComponent != NULL) {
        pOMXComponent->ComponentDeInit(pOMXComponent);
        Exynos_OSAL_Free(pOMXComponent);
        exynos_component->pOMXComponent = NULL;
    }

    if (exynos_component->libHandle != NULL) {
        Exynos_OSAL_dlclose(exynos_component->libHandle);
        exynos_component->libHandle = NULL;
    }

EXIT:
    FunctionOut();

    return ret;
}

