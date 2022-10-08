/*
* Copyright (c) 2009-2011 Intel Corporation.  All rights reserved.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

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

#include "WrsOMXPlugin.h"

#include <dlfcn.h>

#include <HardwareAPI.h>
#include <media/stagefright/foundation/ADebug.h>

namespace android {

OMXPluginBase *createOMXPlugin() {
    return new WrsOMXPlugin;
}

WrsOMXPlugin::WrsOMXPlugin()
{
#ifdef TARGET_HAS_ISV
   AddCore("libisv_omx_core.so");
#else
   AddCore("libwrs_omxil_core_pvwrapped.so");
#if defined(USE_MEDIASDK)
   AddCore("libmfx_omx_core.so");
#endif
#endif
}

OMX_ERRORTYPE WrsOMXPlugin::AddCore(const char* coreName)
{
   void* libHandle = dlopen(coreName, RTLD_NOW);

   if (libHandle != NULL) {
        WrsOMXCore* core = (WrsOMXCore*)calloc(1,sizeof(WrsOMXCore));

        if (!core) {
            dlclose(libHandle);
            return OMX_ErrorUndefined;
        }
        // set plugin lib handle and methods
        core->mLibHandle = libHandle;
        core->mInit = (WrsOMXCore::InitFunc)dlsym(libHandle, "OMX_Init");
        core->mDeinit = (WrsOMXCore::DeinitFunc)dlsym(libHandle, "OMX_Deinit");

        core->mComponentNameEnum =
            (WrsOMXCore::ComponentNameEnumFunc)dlsym(libHandle, "OMX_ComponentNameEnum");

        core->mGetHandle = (WrsOMXCore::GetHandleFunc)dlsym(libHandle, "OMX_GetHandle");
        core->mFreeHandle = (WrsOMXCore::FreeHandleFunc)dlsym(libHandle, "OMX_FreeHandle");

        core->mGetRolesOfComponentHandle =
            (WrsOMXCore::GetRolesOfComponentFunc)dlsym(
                    libHandle, "OMX_GetRolesOfComponent");
        if (core->mInit != NULL) {
            (*(core->mInit))();
        }
        if (core->mComponentNameEnum != NULL) {
            // calculating number of components registered inside given OMX core
            char tmpComponentName[OMX_MAX_STRINGNAME_SIZE] = { 0 };
            OMX_U32 tmpIndex = 0;
            while (OMX_ErrorNone == ((*(core->mComponentNameEnum))(tmpComponentName, OMX_MAX_STRINGNAME_SIZE, tmpIndex))) {
                tmpIndex++;
            ALOGI("OMX IL core %s: declares component %s", coreName, tmpComponentName);
            }
            core->mNumComponents = tmpIndex;
            ALOGI("OMX IL core %s: contains %ld components", coreName, core->mNumComponents);
        }
        // add plugin to the vector
        mCores.push_back(core);
    }
    else {
        ALOGW("OMX IL core %s not found", coreName);
        return OMX_ErrorUndefined; // Do we need to return error message
    }
    return OMX_ErrorNone;
}

WrsOMXPlugin::~WrsOMXPlugin() {
    for (OMX_U32 i = 0; i < mCores.size(); i++) {
       if (mCores[i] != NULL && mCores[i]->mLibHandle != NULL) {
          (*(mCores[i]->mDeinit))();

          dlclose(mCores[i]->mLibHandle);
          free(mCores[i]);
       }
    }
}

OMX_ERRORTYPE WrsOMXPlugin::makeComponentInstance(
        const char *name,
        const OMX_CALLBACKTYPE *callbacks,
        OMX_PTR appData,
        OMX_COMPONENTTYPE **component) {
    for (OMX_U32 i = 0; i < mCores.size(); i++) {
        if (mCores[i] != NULL) {
            if (mCores[i]->mLibHandle == NULL) {
                continue;
            }

            OMX_ERRORTYPE omx_res = (*(mCores[i]->mGetHandle))(
                reinterpret_cast<OMX_HANDLETYPE *>(component),
                const_cast<char *>(name),
                appData, const_cast<OMX_CALLBACKTYPE *>(callbacks));
            if(omx_res == OMX_ErrorNone) {
                Mutex::Autolock autoLock(mMutex);
                WrsOMXComponent comp;

                comp.mComponent = *component;
                comp.mCore = mCores[i];

                mComponents.push_back(comp);
                return OMX_ErrorNone;
            }
        }
    }
    return OMX_ErrorInvalidComponentName;
}

OMX_ERRORTYPE WrsOMXPlugin::destroyComponentInstance(
        OMX_COMPONENTTYPE *component) {
    Mutex::Autolock autoLock(mMutex);
    for (OMX_U32 i = 0; i < mComponents.size(); i++) {
        if (mComponents[i].mComponent == component) {
            if (mComponents[i].mCore == NULL || mComponents[i].mCore->mLibHandle == NULL) {
                return OMX_ErrorUndefined;
            }
            OMX_ERRORTYPE omx_res = (*(mComponents[i].mCore->mFreeHandle))(reinterpret_cast<OMX_HANDLETYPE *>(component));
            mComponents.erase(mComponents.begin() + i);
            return omx_res;
        }
    }
    return OMX_ErrorInvalidComponent;
}

OMX_ERRORTYPE WrsOMXPlugin::enumerateComponents(
        OMX_STRING name,
        size_t size,
        OMX_U32 index) {
    // returning components
    OMX_U32 relativeIndex = index;
    for (OMX_U32 i = 0; i < mCores.size(); i++) {
        if (mCores[i]->mLibHandle == NULL) {
           continue;
        }
        if (relativeIndex < mCores[i]->mNumComponents) return ((*(mCores[i]->mComponentNameEnum))(name, size, relativeIndex));
        else relativeIndex -= mCores[i]->mNumComponents;
    }
    return OMX_ErrorNoMore;
}

OMX_ERRORTYPE WrsOMXPlugin::getRolesOfComponent(
        const char *name,
        Vector<String8> *roles) {
    roles->clear();
    for (OMX_U32 j = 0; j < mCores.size(); j++) {
        if (mCores[j]->mLibHandle == NULL) {
           continue;
        }

        OMX_U32 numRoles;
        OMX_ERRORTYPE err = (*(mCores[j]->mGetRolesOfComponentHandle))(
                const_cast<OMX_STRING>(name), &numRoles, NULL);

        if (err != OMX_ErrorNone) {
            continue;
        }

        if (numRoles > 0) {
            OMX_U8 **array = new OMX_U8 *[numRoles];
            for (OMX_U32 i = 0; i < numRoles; ++i) {
                array[i] = new OMX_U8[OMX_MAX_STRINGNAME_SIZE];
            }

            OMX_U32 numRoles2 = numRoles;
            err = (*(mCores[j]->mGetRolesOfComponentHandle))(
                    const_cast<OMX_STRING>(name), &numRoles2, array);

            CHECK_EQ(err, OMX_ErrorNone);
            CHECK_EQ(numRoles, numRoles2);

            for (OMX_U32 i = 0; i < numRoles; ++i) {
                String8 s((const char *)array[i]);
                roles->push(s);

                delete[] array[i];
                array[i] = NULL;
            }

            delete[] array;
            array = NULL;
        }
        return OMX_ErrorNone;
    }
    return OMX_ErrorInvalidComponent;
}

}  // namespace android
