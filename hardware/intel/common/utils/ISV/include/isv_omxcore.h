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
#ifndef ISV_OMXCORE_H_

#define ISV_OMXCORE_H_

#define ISV_CORE_DEBUG 0

struct ISVOMXCore {
    typedef OMX_ERRORTYPE (*InitFunc)();
    typedef OMX_ERRORTYPE (*DeinitFunc)();
    typedef OMX_ERRORTYPE (*ComponentNameEnumFunc)(
            OMX_STRING, OMX_U32, OMX_U32);

    typedef OMX_ERRORTYPE (*GetHandleFunc)(
            OMX_HANDLETYPE *, OMX_STRING, OMX_PTR, OMX_CALLBACKTYPE *);

    typedef OMX_ERRORTYPE (*FreeHandleFunc)(OMX_HANDLETYPE);

    typedef OMX_ERRORTYPE (*GetRolesOfComponentFunc)(
            OMX_STRING, OMX_U32 *, OMX_U8 **);

    void *mLibHandle;

    InitFunc mInit;
    DeinitFunc mDeinit;
    ComponentNameEnumFunc mComponentNameEnum;
    GetHandleFunc mGetHandle;
    FreeHandleFunc mFreeHandle;
    GetRolesOfComponentFunc mGetRolesOfComponentHandle;

    OMX_U32 mNumComponents;
};

#endif  //ISV_OMXCORE_H_
