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

#include <sys/types.h>
#include <binder/Parcel.h>
#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <utils/Errors.h>
#include <IQClient.h>

using namespace android;

// ---------------------------------------------------------------------------

namespace qClient {

enum {
    NOTIFY_CALLBACK = IBinder::FIRST_CALL_TRANSACTION,
};

class BpQClient : public BpInterface<IQClient>
{
public:
    BpQClient(const sp<IBinder>& impl)
        : BpInterface<IQClient>(impl) {}

    virtual status_t notifyCallback(uint32_t msg, uint32_t value) {
        Parcel data, reply;
        data.writeInterfaceToken(IQClient::getInterfaceDescriptor());
        data.writeInt32(msg);
        data.writeInt32(value);
        remote()->transact(NOTIFY_CALLBACK, data, &reply);
        status_t result = reply.readInt32();
        return result;
    }
};

IMPLEMENT_META_INTERFACE(QClient, "android.display.IQClient");

// ----------------------------------------------------------------------

status_t BnQClient::onTransact(
    uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags)
{
    switch(code) {
        case NOTIFY_CALLBACK: {
            CHECK_INTERFACE(IQClient, data, reply);
            uint32_t msg = data.readInt32();
            uint32_t value = data.readInt32();
            notifyCallback(msg, value);
            return NO_ERROR;
        } break;
        default:
            return BBinder::onTransact(code, data, reply, flags);
    }
}

}; // namespace qClient
