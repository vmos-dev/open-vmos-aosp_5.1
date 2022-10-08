/*
 * Copyright (C) 2010 The Android Open Source Project
 * Copyright (C) 2012-2013, The Linux Foundation. All rights reserved.
 *
 * Not a Contribution, Apache license notifications and license are
 * retained for attribution purposes only.

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

#ifndef ANDROID_IQSERVICE_H
#define ANDROID_IQSERVICE_H

#include <stdint.h>
#include <sys/types.h>
#include <utils/Errors.h>
#include <utils/RefBase.h>
#include <binder/IInterface.h>
#include <binder/IBinder.h>
#include <IQClient.h>

namespace qService {
// ----------------------------------------------------------------------------
class IQService : public android::IInterface
{
public:
    DECLARE_META_INTERFACE(QService);
    enum {
        // Hardware securing start/end notification
        SECURING = android::IBinder::FIRST_CALL_TRANSACTION,
        UNSECURING, // Hardware unsecuring start/end notification
        CONNECT,
        SCREEN_REFRESH,
    };
    enum {
        END = 0,
        START,
    };
    virtual void securing(uint32_t startEnd) = 0;
    virtual void unsecuring(uint32_t startEnd) = 0;
    virtual void connect(const android::sp<qClient::IQClient>& client) = 0;
    virtual android::status_t screenRefresh() = 0;
};

// ----------------------------------------------------------------------------

class BnQService : public android::BnInterface<IQService>
{
public:
    virtual android::status_t onTransact( uint32_t code,
                                          const android::Parcel& data,
                                          android::Parcel* reply,
                                          uint32_t flags = 0);
};

// ----------------------------------------------------------------------------
}; // namespace qService

#endif // ANDROID_IQSERVICE_H
