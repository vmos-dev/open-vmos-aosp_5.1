/*
 * Copyright (C) 2012 Intel Corporation.  All rights reserved.
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
 *
 */

#include <OMX_Core.h>
#include <OMX_Component.h>
#include <dlfcn.h>

#include "isv_omxcore.h"
#include "isv_omxcomponent.h"
#include "isv_profile.h"

//#define LOG_NDEBUG 0
#undef LOG_TAG
#define LOG_TAG "isv-omxil"

#define WRS_CORE_NAME "libwrs_omxil_core_pvwrapped.so"
#define CORE_NUMBER 1
#ifdef USE_MEDIASDK
#define MSDK_CORE_NAME "libmfx_omx_core.so"
#undef CORE_NUMBER
#define CORE_NUMBER 2
#endif


using namespace android;

static unsigned int g_initialized = 0;
static unsigned int g_nr_instances = 0;
static unsigned int g_nr_comp = 0;

static pthread_mutex_t g_module_lock = PTHREAD_MUTEX_INITIALIZER;
static ISVOMXCore g_cores[CORE_NUMBER];
static Vector<ISVComponent*> g_isv_components;

/**********************************************************************************
 * core entry
 */

OMX_API OMX_ERRORTYPE OMX_APIENTRY OMX_Init(void)
{
    int ret;
    ALOGD_IF(ISV_CORE_DEBUG, "%s: enter", __func__);

    pthread_mutex_lock(&g_module_lock);
    if (!g_initialized) {
        for (OMX_U32 i = 0; i < CORE_NUMBER; i++) {

            void* libHandle = NULL;
            if (i == 0)
                libHandle = dlopen(WRS_CORE_NAME, RTLD_LAZY);
#ifdef USE_MEDIASDK
            else
                libHandle = dlopen(MSDK_CORE_NAME, RTLD_LAZY);
#endif
            if (libHandle != NULL) {
                g_cores[i].mLibHandle = libHandle;
                g_cores[i].mInit = (ISVOMXCore::InitFunc)dlsym(libHandle, "OMX_Init");
                g_cores[i].mDeinit = (ISVOMXCore::DeinitFunc)dlsym(libHandle, "OMX_Deinit");

                g_cores[i].mComponentNameEnum =
                    (ISVOMXCore::ComponentNameEnumFunc)dlsym(libHandle, "OMX_ComponentNameEnum");

                g_cores[i].mGetHandle = (ISVOMXCore::GetHandleFunc)dlsym(libHandle, "OMX_GetHandle");
                g_cores[i].mFreeHandle = (ISVOMXCore::FreeHandleFunc)dlsym(libHandle, "OMX_FreeHandle");

                g_cores[i].mGetRolesOfComponentHandle =
                    (ISVOMXCore::GetRolesOfComponentFunc)dlsym(
                            libHandle, "OMX_GetRolesOfComponent");
                if (g_cores[i].mInit != NULL) {
                    (*(g_cores[i].mInit))();
                }
                if (g_cores[i].mComponentNameEnum != NULL) {
                    // calculating number of components registered inside given OMX core
                    char tmpComponentName[OMX_MAX_STRINGNAME_SIZE] = { 0 };
                    OMX_U32 tmpIndex = 0;
                    while (OMX_ErrorNone == ((*(g_cores[i].mComponentNameEnum))(tmpComponentName, OMX_MAX_STRINGNAME_SIZE, tmpIndex))) {
                        tmpIndex++;
                        ALOGD_IF(ISV_CORE_DEBUG, "OMX IL core: declares component %s", tmpComponentName);
                    }
                    g_cores[i].mNumComponents = tmpIndex;
                    g_nr_comp += g_cores[i].mNumComponents;
                    ALOGD_IF(ISV_CORE_DEBUG, "OMX IL core: contains %ld components", g_cores[i].mNumComponents);
                }
            } else {
                pthread_mutex_unlock(&g_module_lock);
                ALOGW("OMX IL core not found");
            }
        }
        g_initialized = 1;
    }
    pthread_mutex_unlock(&g_module_lock);

    ALOGD_IF(ISV_CORE_DEBUG, "%s: exit done", __func__);
    return OMX_ErrorNone;
}

OMX_API OMX_ERRORTYPE OMX_APIENTRY OMX_Deinit(void)
{
    ALOGD_IF(ISV_CORE_DEBUG, "%s: enter", __func__);
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    ALOGV("%s: enter", __func__);
    if (g_initialized == 0)
        return OMX_ErrorNone;

    pthread_mutex_lock(&g_module_lock);
    for (OMX_U32 i = 0; i < CORE_NUMBER; i++) {
        if (g_cores[i].mDeinit != NULL) {
            (*(g_cores[i].mDeinit))();
        }
    }
    pthread_mutex_unlock(&g_module_lock);

    g_initialized = 0;

    ALOGD_IF(ISV_CORE_DEBUG, "%s: exit %d", __func__, ret);
    return ret;
}

OMX_API OMX_ERRORTYPE OMX_APIENTRY OMX_ComponentNameEnum(
    OMX_OUT OMX_STRING cComponentName,
    OMX_IN OMX_U32 nNameLength,
    OMX_IN OMX_U32 nIndex)
{
    ALOGD_IF(ISV_CORE_DEBUG, "%s: enter", __func__);
    pthread_mutex_lock(&g_module_lock);
    OMX_U32 relativeIndex = nIndex;
    if (nIndex >= g_nr_comp) {
        pthread_mutex_unlock(&g_module_lock);
        ALOGD_IF(ISV_CORE_DEBUG, "%s: exit done", __func__);
        return OMX_ErrorNoMore;
    }

    for (OMX_U32 i = 0; i < CORE_NUMBER; i++) {
        if (g_cores[i].mLibHandle == NULL) {
           continue;
        }
        if (relativeIndex < g_cores[i].mNumComponents) {
            pthread_mutex_unlock(&g_module_lock);
            ALOGD_IF(ISV_CORE_DEBUG, "%s: found %luth component %s", __func__, nIndex, cComponentName);
            return ((*(g_cores[i].mComponentNameEnum))(cComponentName, nNameLength, relativeIndex));
        } else relativeIndex -= g_cores[i].mNumComponents;
    }
    pthread_mutex_unlock(&g_module_lock);
    ALOGD_IF(ISV_CORE_DEBUG, "%s: exit error!!!", __func__);
    return OMX_ErrorUndefined;
}

OMX_API OMX_ERRORTYPE OMX_APIENTRY OMX_GetHandle(
    OMX_OUT OMX_HANDLETYPE* pHandle,
    OMX_IN OMX_STRING cComponentName,
    OMX_IN OMX_PTR pAppData,
    OMX_IN OMX_CALLBACKTYPE *pCallBacks)
{
    struct list *entry;
    OMX_ERRORTYPE ret;
    OMX_HANDLETYPE tempHandle;
    ALOGD_IF(ISV_CORE_DEBUG, "%s: enter, try to get %s", __func__, cComponentName);
    pthread_mutex_lock(&g_module_lock);

    // create a isv component instant
    ISVComponent *pISVComponent = new ISVComponent(pAppData);
    if (!pISVComponent) {
        ALOGE("%s: failed to alloc isv omx component", __func__);
        pthread_mutex_unlock(&g_module_lock);
        return OMX_ErrorInsufficientResources;
    }

    OMX_CALLBACKTYPE *pISVCallBacks = pISVComponent->getCallBacks(pCallBacks);
    if (!pISVCallBacks) {
        ALOGE("%s: failed to get isv callback functions", __func__);
        pthread_mutex_unlock(&g_module_lock);
        return OMX_ErrorInsufficientResources;
    }

    /* find the real component*/
    for (OMX_U32 i = 0; i < CORE_NUMBER; i++) {
        if (g_cores[i].mLibHandle == NULL) {
            continue;
        }

        OMX_ERRORTYPE omx_res = (*(g_cores[i].mGetHandle))(
                &tempHandle,
                const_cast<char *>(cComponentName),
                pAppData, pISVCallBacks);
        if(omx_res == OMX_ErrorNone) {
            pISVComponent->setComponent(static_cast<OMX_COMPONENTTYPE*>(tempHandle), &g_cores[i]);
            g_isv_components.push_back(pISVComponent);
            *pHandle = pISVComponent->getBaseComponent();

            ALOGD_IF(ISV_CORE_DEBUG, "%s: found component %s, pHandle %p", __func__, cComponentName, *pHandle);
            pthread_mutex_unlock(&g_module_lock);
            return OMX_ErrorNone;
        }
    }
    pthread_mutex_unlock(&g_module_lock);

    delete pISVComponent;
    pISVComponent = NULL;
    ALOGD_IF(ISV_CORE_DEBUG, "%s(): exit failure, %s not found", __func__, cComponentName);
    return OMX_ErrorInvalidComponent;
}

OMX_API OMX_ERRORTYPE OMX_APIENTRY OMX_FreeHandle(
    OMX_IN OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE ret;

    ALOGD_IF(ISV_CORE_DEBUG, "%s: enter, try to free component hanle %p", __func__, hComponent);
    pthread_mutex_lock(&g_module_lock);

    for (OMX_U32 i = 0; i < g_isv_components.size(); i++) {
        ISVComponent *pComp = g_isv_components.itemAt(i);
        if (static_cast<OMX_HANDLETYPE>(pComp->getBaseComponent()) == hComponent) {
            OMX_ERRORTYPE omx_res = pComp->freeComponent();
            if (omx_res != OMX_ErrorNone) {
                ALOGE("%s: free OMX handle %p failed", __func__, hComponent);
                pthread_mutex_unlock(&g_module_lock);
                return omx_res;
            }
            delete pComp;
            g_isv_components.removeAt(i);
            ALOGD_IF(ISV_CORE_DEBUG, "%s: free component %p success", __func__, hComponent);
            pthread_mutex_unlock(&g_module_lock);
            return OMX_ErrorNone;
        }
    }
    pthread_mutex_unlock(&g_module_lock);
    ALOGE("%s(): exit failure, component %p not found", __func__, hComponent);
    return OMX_ErrorInvalidComponent;
}

OMX_API OMX_ERRORTYPE OMX_APIENTRY OMX_SetupTunnel(
    OMX_IN OMX_HANDLETYPE __maybe_unused hOutput,
    OMX_IN OMX_U32 __maybe_unused nPortOutput,
    OMX_IN OMX_HANDLETYPE __maybe_unused hInput,
    OMX_IN OMX_U32 __maybe_unused nPortInput)
{
    return OMX_ErrorNotImplemented;
}

OMX_API OMX_ERRORTYPE   OMX_GetContentPipe(
    OMX_OUT OMX_HANDLETYPE __maybe_unused *hPipe,
    OMX_IN OMX_STRING __maybe_unused szURI)
{
    return OMX_ErrorNotImplemented;
}

OMX_API OMX_ERRORTYPE OMX_GetComponentsOfRole (
    OMX_IN OMX_STRING __maybe_unused role,
    OMX_INOUT OMX_U32 __maybe_unused *pNumComps,
    OMX_INOUT OMX_U8  __maybe_unused **compNames)
{
    ALOGD_IF(ISV_CORE_DEBUG, "%s: enter", __func__);
    return OMX_ErrorNotImplemented;
}

OMX_API OMX_ERRORTYPE OMX_GetRolesOfComponent (
    OMX_IN OMX_STRING compName,
    OMX_INOUT OMX_U32 *pNumRoles,
    OMX_OUT OMX_U8 **roles)
{
    ALOGD_IF(ISV_CORE_DEBUG, "%s: enter", __func__);
    pthread_mutex_lock(&g_module_lock);
    for (OMX_U32 j = 0; j < CORE_NUMBER; j++) {
        if (g_cores[j].mLibHandle == NULL) {
           continue;
        }

        OMX_U32 numRoles;
        OMX_ERRORTYPE err = (*(g_cores[j].mGetRolesOfComponentHandle))(
                const_cast<OMX_STRING>(compName), &numRoles, NULL);

        if (err != OMX_ErrorNone) {
            continue;
        }

        if (numRoles > 0) {
            OMX_U8 **array = new OMX_U8 *[numRoles];
            for (OMX_U32 i = 0; i < numRoles; ++i) {
                array[i] = new OMX_U8[OMX_MAX_STRINGNAME_SIZE];
            }

            OMX_U32 numRoles2 = numRoles;
            err = (*(g_cores[j].mGetRolesOfComponentHandle))(
                    const_cast<OMX_STRING>(compName), &numRoles2, array);

            *pNumRoles = numRoles;
            for (OMX_U32 i = 0; i < numRoles; i++) {
                if (i < numRoles-1)
                    roles[i+1] = roles[i] + OMX_MAX_STRINGNAME_SIZE;

                strncpy((OMX_STRING)&roles[i][0],
                        (const OMX_STRING)&array[i][0], OMX_MAX_STRINGNAME_SIZE);
                delete[] array[i];
                array[i] = NULL;
            }

            delete[] array;
            array = NULL;
        }

        pthread_mutex_unlock(&g_module_lock);
        ALOGD_IF(ISV_CORE_DEBUG, "%s: exit done", __func__);
        return OMX_ErrorNone;
    }
    pthread_mutex_unlock(&g_module_lock);

    ALOGE("%s: invalid component", __func__);
    return OMX_ErrorInvalidComponent;
}

