/*
 * Copyright (C) 2010 The Android Open Source Project
 * Copyright (C) 2012-2014, The Linux Foundation. All rights reserved.
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

#include <fcntl.h>
#include <stdint.h>
#include <sys/types.h>
#include <binder/Parcel.h>
#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <binder/IPCThreadState.h>
#include <utils/Errors.h>
#include <private/android_filesystem_config.h>
#include <IQService.h>

#define QSERVICE_DEBUG 0

using namespace android;
using namespace qClient;

// ---------------------------------------------------------------------------

namespace qService {

class BpQService : public BpInterface<IQService>
{
public:
    BpQService(const sp<IBinder>& impl)
        : BpInterface<IQService>(impl) {}

    virtual void connect(const sp<IQClient>& client) {
        ALOGD_IF(QSERVICE_DEBUG, "%s: connect HWC client", __FUNCTION__);
        Parcel data, reply;
        data.writeInterfaceToken(IQService::getInterfaceDescriptor());
        data.writeStrongBinder(client->asBinder());
        remote()->transact(CONNECT_HWC_CLIENT, data, &reply);
    }

    virtual void connect(const sp<IQHDMIClient>& client) {
        ALOGD_IF(QSERVICE_DEBUG, "%s: connect HDMI client", __FUNCTION__);
        Parcel data, reply;
        data.writeInterfaceToken(IQService::getInterfaceDescriptor());
        data.writeStrongBinder(client->asBinder());
        remote()->transact(CONNECT_HDMI_CLIENT, data, &reply);
    }


    virtual android::status_t dispatch(uint32_t command, const Parcel* inParcel,
            Parcel* outParcel) {
        ALOGD_IF(QSERVICE_DEBUG, "%s: dispatch in:%p", __FUNCTION__, inParcel);
        status_t err = (status_t) android::FAILED_TRANSACTION;
        Parcel data;
        Parcel *reply = outParcel;
        data.writeInterfaceToken(IQService::getInterfaceDescriptor());
        if (inParcel && inParcel->dataSize() > 0)
            data.appendFrom(inParcel, 0, inParcel->dataSize());
        err = remote()->transact(command, data, reply);
        return err;
    }
};

IMPLEMENT_META_INTERFACE(QService, "android.display.IQService");

// ----------------------------------------------------------------------

static void getProcName(int pid, char *buf, int size);

status_t BnQService::onTransact(
    uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags)
{
    ALOGD_IF(QSERVICE_DEBUG, "%s: code: %d", __FUNCTION__, code);
    // IPC should be from certain processes only
    IPCThreadState* ipc = IPCThreadState::self();
    const int callerPid = ipc->getCallingPid();
    const int callerUid = ipc->getCallingUid();
    const int MAX_BUF_SIZE = 1024;
    char callingProcName[MAX_BUF_SIZE] = {0};

    getProcName(callerPid, callingProcName, MAX_BUF_SIZE);

    const bool permission = (callerUid == AID_MEDIA ||
            callerUid == AID_GRAPHICS ||
            callerUid == AID_ROOT ||
            callerUid == AID_SYSTEM);

    if (code == CONNECT_HWC_CLIENT) {
        CHECK_INTERFACE(IQService, data, reply);
        if(callerUid != AID_GRAPHICS) {
            ALOGE("display.qservice CONNECT_HWC_CLIENT access denied: \
                    pid=%d uid=%d process=%s",
                    callerPid, callerUid, callingProcName);
            return PERMISSION_DENIED;
        }
        sp<IQClient> client =
                interface_cast<IQClient>(data.readStrongBinder());
        connect(client);
        return NO_ERROR;
    } else if(code == CONNECT_HDMI_CLIENT) {
        CHECK_INTERFACE(IQService, data, reply);
        if(callerUid != AID_SYSTEM && callerUid != AID_ROOT) {
            ALOGE("display.qservice CONNECT_HDMI_CLIENT access denied: \
                    pid=%d uid=%d process=%s",
                    callerPid, callerUid, callingProcName);
            return PERMISSION_DENIED;
        }
        sp<IQHDMIClient> client =
                interface_cast<IQHDMIClient>(data.readStrongBinder());
        connect(client);
        return NO_ERROR;
    } else if (code > COMMAND_LIST_START && code < COMMAND_LIST_END) {
        if(!permission) {
            ALOGE("display.qservice access denied: command=%d\
                  pid=%d uid=%d process=%s", code, callerPid,
                  callerUid, callingProcName);
            return PERMISSION_DENIED;
        }
        CHECK_INTERFACE(IQService, data, reply);
        dispatch(code, &data, reply);
        return NO_ERROR;
    } else {
        return BBinder::onTransact(code, data, reply, flags);
    }
}

//Helper
static void getProcName(int pid, char *buf, int size) {
    int fd = -1;
    snprintf(buf, size, "/proc/%d/cmdline", pid);
    fd = open(buf, O_RDONLY);
    if (fd < 0) {
        strlcpy(buf, "Unknown", size);
    } else {
        ssize_t len = read(fd, buf, size - 1);
        if (len >= 0)
           buf[len] = 0;

        close(fd);
    }
}

}; // namespace qService
