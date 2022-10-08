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
// XXX: Since qservice currently runs as part of hwc instead of a standalone
// process, the implementation below is overridden and the notifyCallback in
// hwc_qclient is directly called.

namespace qClient {

enum {
    NOTIFY_CALLBACK = IBinder::FIRST_CALL_TRANSACTION,
};

class BpQClient : public BpInterface<IQClient>
{
public:
    BpQClient(const sp<IBinder>& impl)
        : BpInterface<IQClient>(impl) {}

    virtual status_t notifyCallback(uint32_t command,
            const Parcel* inParcel,
            Parcel* outParcel) {
        Parcel data;
        Parcel *reply = outParcel;
        data.writeInterfaceToken(IQClient::getInterfaceDescriptor());
        data.writeInt32(command);
        if (inParcel->dataAvail())
            data.appendFrom(inParcel, inParcel->dataPosition(),
                    inParcel->dataAvail());
        status_t result = remote()->transact(NOTIFY_CALLBACK, data, reply);
        return result;
    }
};

IMPLEMENT_META_INTERFACE(QClient, "android.display.IQClient");

// ----------------------------------------------------------------------
//Stub implementation - nothing needed here
status_t BnQClient::onTransact(
    uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags)
{
    switch(code) {
        case NOTIFY_CALLBACK: {
            CHECK_INTERFACE(IQClient, data, reply);
            uint32_t command = data.readInt32();
            notifyCallback(command, &data, reply);
            return NO_ERROR;
        } break;
        default:
            return BBinder::onTransact(code, data, reply, flags);
    }

}

}; // namespace qClient
