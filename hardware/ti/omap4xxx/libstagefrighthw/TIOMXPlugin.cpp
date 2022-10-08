/*
 * Copyright (C) 2009 The Android Open Source Project
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

#include "TIOMXPlugin.h"

#include <dlfcn.h>

#include <HardwareAPI.h>

namespace android {

extern "C" OMXPluginBase *createOMXPlugin() {
    return new TIOMXPlugin;
}

extern "C" void destroyOMXPlugin(OMXPluginBase *plugin) {
    delete plugin;
}

#define LIBOMX "libOMX_Core.so"

TIOMXPlugin::TIOMXPlugin()
    : mLibHandle(dlopen(LIBOMX, RTLD_NOW)),
      mInit(NULL),
      mDeinit(NULL),
      mComponentNameEnum(NULL),
      mGetHandle(NULL),
      mFreeHandle(NULL),
      mGetRolesOfComponentHandle(NULL) {
    if (mLibHandle != NULL) {
        mInit = (InitFunc)dlsym(mLibHandle, "TIOMX_Init");
        mDeinit = (DeinitFunc)dlsym(mLibHandle, "TIOMX_Deinit");

        mComponentNameEnum =
            (ComponentNameEnumFunc)dlsym(mLibHandle, "TIOMX_ComponentNameEnum");

        mGetHandle = (GetHandleFunc)dlsym(mLibHandle, "TIOMX_GetHandle");
        mFreeHandle = (FreeHandleFunc)dlsym(mLibHandle, "TIOMX_FreeHandle");

        mGetRolesOfComponentHandle =
            (GetRolesOfComponentFunc)dlsym(
                    mLibHandle, "TIOMX_GetRolesOfComponent");

        (*mInit)();
    }
    else
        ALOGE("%s: failed to load %s", __func__, LIBOMX);
}

TIOMXPlugin::~TIOMXPlugin() {
    if (mLibHandle != NULL) {
        (*mDeinit)();

        dlclose(mLibHandle);
        mLibHandle = NULL;
    }
}

OMX_ERRORTYPE TIOMXPlugin::makeComponentInstance(
        const char *name,
        const OMX_CALLBACKTYPE *callbacks,
        OMX_PTR appData,
        OMX_COMPONENTTYPE **component) {
    if (mLibHandle == NULL) {
        return OMX_ErrorUndefined;
    }

    return (*mGetHandle)(
            reinterpret_cast<OMX_HANDLETYPE *>(component),
            const_cast<char *>(name),
            appData, const_cast<OMX_CALLBACKTYPE *>(callbacks));
}

OMX_ERRORTYPE TIOMXPlugin::destroyComponentInstance(
        OMX_COMPONENTTYPE *component) {
    if (mLibHandle == NULL) {
        return OMX_ErrorUndefined;
    }

    return (*mFreeHandle)(reinterpret_cast<OMX_HANDLETYPE *>(component));
}

OMX_ERRORTYPE TIOMXPlugin::enumerateComponents(
        OMX_STRING name,
        size_t size,
        OMX_U32 index) {
    if (mLibHandle == NULL) {
	ALOGE("mLibHandle is NULL!");
        return OMX_ErrorUndefined;
    }

    return (*mComponentNameEnum)(name, size, index);
}

OMX_ERRORTYPE TIOMXPlugin::getRolesOfComponent(
        const char *name,
        Vector<String8> *roles) {
    roles->clear();

    if (mLibHandle == NULL) {
        return OMX_ErrorUndefined;
    }

    OMX_U32 numRoles;
    OMX_ERRORTYPE err = (*mGetRolesOfComponentHandle)(
            const_cast<OMX_STRING>(name), &numRoles, NULL);

    if (err != OMX_ErrorNone) {
        return err;
    }

    if (numRoles > 0) {
        OMX_U8 **array = new OMX_U8 *[numRoles];
        for (OMX_U32 i = 0; i < numRoles; ++i) {
            array[i] = new OMX_U8[OMX_MAX_STRINGNAME_SIZE];
        }

        err = (*mGetRolesOfComponentHandle)(
                const_cast<OMX_STRING>(name), &numRoles, array);

        for (OMX_U32 i = 0; i < numRoles; ++i) {
            if (err == OMX_ErrorNone) {
                String8 s((const char *)array[i]);
                roles->push(s);
            }

            delete[] array[i];
            array[i] = NULL;
        }

        delete[] array;
        array = NULL;
    }

    return err;
}

}  // namespace android
