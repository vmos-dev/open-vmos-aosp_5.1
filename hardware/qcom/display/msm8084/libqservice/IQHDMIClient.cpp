/*
* Copyright (c) 2014 The Linux Foundation. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are
* met:
*    * Redistributions of source code must retain the above copyright
*      notice, this list of conditions and the following disclaimer.
*    * Redistributions in binary form must reproduce the above
*      copyright notice, this list of conditions and the following
*      disclaimer in the documentation and/or other materials provided
*      with the distribution.
*    * Neither the name of The Linux Foundation. nor the names of its
*      contributors may be used to endorse or promote products derived
*      from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
* ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
* BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
* OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
* IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include <utils/Log.h>
#include <binder/Parcel.h>
#include "IQHDMIClient.h"

using namespace android;
namespace qClient {

enum {
    HDMI_CONNECTED = IBinder::FIRST_CALL_TRANSACTION,
    CEC_MESSAGE_RECEIVED
};

class BpQHDMIClient : public BpInterface<IQHDMIClient>
{
public:
    BpQHDMIClient(const sp<IBinder>& impl)
            :BpInterface<IQHDMIClient>(impl)
    {
    }

    void onHdmiHotplug(int connected)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IQHDMIClient::getInterfaceDescriptor());
        data.writeInt32(connected);
        remote()->transact(HDMI_CONNECTED, data, &reply, IBinder::FLAG_ONEWAY);
    }

    void onCECMessageRecieved(char *msg, ssize_t len)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IQHDMIClient::getInterfaceDescriptor());
        data.writeInt32(len);
        void *buf = data.writeInplace(len);
        memcpy(buf, msg, len);
        remote()->transact(CEC_MESSAGE_RECEIVED, data, &reply,
                IBinder::FLAG_ONEWAY);
    }
};

IMPLEMENT_META_INTERFACE(QHDMIClient,
        "android.display.IQHDMIClient");

status_t BnQHDMIClient::onTransact(uint32_t code, const Parcel& data,
        Parcel* reply, uint32_t flags)
{
    switch(code) {
        case HDMI_CONNECTED: {
            CHECK_INTERFACE(IQHDMIClient, data, reply);
            int connected = data.readInt32();
            onHdmiHotplug(connected);
            return NO_ERROR;
        }
        case CEC_MESSAGE_RECEIVED: {
            CHECK_INTERFACE(IQHDMIClient, data, reply);
            ssize_t len = data.readInt32();
            const void* msg;
            if(len >= 0 && len <= (ssize_t) data.dataAvail()) {
                msg = data.readInplace(len);
            } else {
                msg = NULL;
                len = 0;
            }
            onCECMessageRecieved((char*) msg, len);
            return NO_ERROR;
        }
        default: {
            return BBinder::onTransact(code, data, reply, flags);
        }
    }
}

}; //namespace qClient
