/*
 *  Copyright (c) 2012-2014, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
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

#include <QService.h>

#define QSERVICE_DEBUG 0

using namespace android;

namespace qService {

QService* QService::sQService = NULL;
// ----------------------------------------------------------------------------
QService::QService()
{
    ALOGD_IF(QSERVICE_DEBUG, "QService Constructor invoked");
}

QService::~QService()
{
    ALOGD_IF(QSERVICE_DEBUG,"QService Destructor invoked");
}

void QService::connect(const sp<qClient::IQClient>& client) {
    ALOGD_IF(QSERVICE_DEBUG,"HWC client connected");
    mClient = client;
}

void QService::connect(const sp<qClient::IQHDMIClient>& client) {
    ALOGD_IF(QSERVICE_DEBUG,"HWC client connected");
    mHDMIClient = client;
}

status_t QService::dispatch(uint32_t command, const Parcel* inParcel,
        Parcel* outParcel) {
    status_t err = (status_t) FAILED_TRANSACTION;
    if (mClient.get()) {
        ALOGD_IF(QSERVICE_DEBUG, "Dispatching command: %d", command);
        err = mClient->notifyCallback(command, inParcel, outParcel);
    }
    return err;
}

void QService::onHdmiHotplug(int connected) {
    if(mHDMIClient.get()) {
        ALOGD_IF(QSERVICE_DEBUG, "%s: HDMI hotplug", __FUNCTION__);
        mHDMIClient->onHdmiHotplug(connected);
    } else {
        ALOGE("%s: Failed to get a valid HDMI client", __FUNCTION__);
    }
}

void QService::onCECMessageReceived(char *msg, ssize_t len) {
    if(mHDMIClient.get()) {
        ALOGD_IF(QSERVICE_DEBUG, "%s: CEC message received", __FUNCTION__);
        mHDMIClient->onCECMessageRecieved(msg, len);
    } else {
        ALOGE("%s: Failed to get a valid HDMI client", __FUNCTION__);
    }
}


void QService::init()
{
    if(!sQService) {
        sQService = new QService();
        sp<IServiceManager> sm = defaultServiceManager();
        sm->addService(String16("display.qservice"), sQService);
        if(sm->checkService(String16("display.qservice")) != NULL)
            ALOGD_IF(QSERVICE_DEBUG, "adding display.qservice succeeded");
        else
            ALOGD_IF(QSERVICE_DEBUG, "adding display.qservice failed");
    }
}

}
