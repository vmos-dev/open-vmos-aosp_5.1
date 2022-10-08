/*
 * Copyright (C) 2007 The Android Open Source Project
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

//
#ifndef ANDROID_HARDWARE_IMOUNTSERVICE_H
#define ANDROID_HARDWARE_IMOUNTSERVICE_H

#include <binder/IInterface.h>
#include <utils/String16.h>

namespace android {

// ----------------------------------------------------------------------

class IMountService : public IInterface
{
public:
    static const int OperationSucceeded               = 0;
    static const int OperationFailedInternalError     = -1;
    static const int OperationFailedNoMedia           = -2;
    static const int OperationFailedMediaBlank        = -3;
    static const int OperationFailedMediaCorrupt      = -4;
    static const int OperationFailedVolumeNotMounted  = -5;


public:
    DECLARE_META_INTERFACE(MountService);

    virtual void getShareMethodList() = 0;
    virtual bool getShareMethodAvailable(String16 method) = 0;
    virtual int shareVolume(String16 path, String16 method) = 0;
    virtual int unshareVolume(String16 path, String16 method) = 0;
    virtual bool getVolumeShared(String16 path, String16 method) = 0;
    virtual int mountVolume(String16 path) = 0;
    virtual int unmountVolume(String16 path) = 0;
    virtual int formatVolume(String16 path) = 0;
    virtual String16 getVolumeState(String16 mountPoint) = 0;
    virtual int createSecureContainer(String16 id, int sizeMb, String16 fstype, String16 key, int ownerUid) = 0;
    virtual int finalizeSecureContainer(String16 id) = 0;
    virtual int destroySecureContainer(String16 id) = 0;
    virtual int mountSecureContainer(String16 id, String16 key, int ownerUid) = 0;
    virtual int unmountSecureContainer(String16 id) = 0;
    virtual int renameSecureContainer(String16 oldId, String16 newId) = 0;
    virtual String16 getSecureContainerPath(String16 id) = 0;
    virtual void getSecureContainerList() = 0;
    virtual void shutdown() = 0;
};

// ----------------------------------------------------------------------

}; // namespace android

#endif // ANDROID_HARDWARE_IMOUNTSERVICE_H
