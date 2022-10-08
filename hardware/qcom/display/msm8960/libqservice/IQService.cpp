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

using namespace android;
using namespace qClient;

// ---------------------------------------------------------------------------

namespace qService {

class BpQService : public BpInterface<IQService>
{
public:
    BpQService(const sp<IBinder>& impl)
        : BpInterface<IQService>(impl) {}

    virtual void securing(uint32_t startEnd) {
        Parcel data, reply;
        data.writeInterfaceToken(IQService::getInterfaceDescriptor());
        data.writeInt32(startEnd);
        remote()->transact(SECURING, data, &reply);
    }

    virtual void unsecuring(uint32_t startEnd) {
        Parcel data, reply;
        data.writeInterfaceToken(IQService::getInterfaceDescriptor());
        data.writeInt32(startEnd);
        remote()->transact(UNSECURING, data, &reply);
    }

    virtual void connect(const sp<IQClient>& client) {
        Parcel data, reply;
        data.writeInterfaceToken(IQService::getInterfaceDescriptor());
        data.writeStrongBinder(client->asBinder());
        remote()->transact(CONNECT, data, &reply);
    }

    virtual status_t screenRefresh() {
        Parcel data, reply;
        data.writeInterfaceToken(IQService::getInterfaceDescriptor());
        remote()->transact(SCREEN_REFRESH, data, &reply);
        status_t result = reply.readInt32();
        return result;
    }
};

IMPLEMENT_META_INTERFACE(QService, "android.display.IQService");

// ----------------------------------------------------------------------

static void getProcName(int pid, char *buf, int size);

status_t BnQService::onTransact(
    uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags)
{
    // IPC should be from mediaserver only
    IPCThreadState* ipc = IPCThreadState::self();
    const int callerPid = ipc->getCallingPid();
    const int callerUid = ipc->getCallingUid();
    const size_t MAX_BUF_SIZE = 1024;
    char callingProcName[MAX_BUF_SIZE] = {0};

    getProcName(callerPid, callingProcName, MAX_BUF_SIZE);

    const bool permission = (callerUid == AID_MEDIA);

    switch(code) {
        case SECURING: {
            if(!permission) {
                ALOGE("display.qservice SECURING access denied: \
                      pid=%d uid=%d process=%s",
                      callerPid, callerUid, callingProcName);
                return PERMISSION_DENIED;
            }
            CHECK_INTERFACE(IQService, data, reply);
            uint32_t startEnd = data.readInt32();
            securing(startEnd);
            return NO_ERROR;
        } break;
        case UNSECURING: {
            if(!permission) {
                ALOGE("display.qservice UNSECURING access denied: \
                      pid=%d uid=%d process=%s",
                      callerPid, callerUid, callingProcName);
                return PERMISSION_DENIED;
            }
            CHECK_INTERFACE(IQService, data, reply);
            uint32_t startEnd = data.readInt32();
            unsecuring(startEnd);
            return NO_ERROR;
        } break;
        case CONNECT: {
            CHECK_INTERFACE(IQService, data, reply);
            if(callerUid != AID_GRAPHICS) {
                ALOGE("display.qservice CONNECT access denied: \
                      pid=%d uid=%d process=%s",
                      callerPid, callerUid, callingProcName);
                return PERMISSION_DENIED;
            }
            sp<IQClient> client =
                interface_cast<IQClient>(data.readStrongBinder());
            connect(client);
            return NO_ERROR;
        } break;
        case SCREEN_REFRESH: {
            CHECK_INTERFACE(IQService, data, reply);
            if(callerUid != AID_SYSTEM) {
                ALOGE("display.qservice SCREEN_REFRESH access denied: \
                      pid=%d uid=%d process=%s",callerPid,
                      callerUid, callingProcName);
                return PERMISSION_DENIED;
            }
            return screenRefresh();
        } break;
        default:
            return BBinder::onTransact(code, data, reply, flags);
    }
}

//Helper
static void getProcName(int pid, char *buf, int size) {
    int fd = -1;
    snprintf(buf, size, "/proc/%d/cmdline", pid);
    fd = open(buf, O_RDONLY);
    if (fd < 0) {
        strcpy(buf, "Unknown");
    } else {
        int len = read(fd, buf, size - 1);
        buf[len] = 0;
        close(fd);
    }
}

}; // namespace qService
