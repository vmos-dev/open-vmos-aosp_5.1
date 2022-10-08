/*
 *  Copyright (c) 2012-2013, The Linux Foundation. All rights reserved.
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

#ifndef ANDROID_QSERVICE_H
#define ANDROID_QSERVICE_H

#include <utils/Errors.h>
#include <sys/types.h>
#include <cutils/log.h>
#include <binder/IServiceManager.h>
#include <IQService.h>
#include <IQClient.h>

struct hwc_context_t;

namespace qService {
// ----------------------------------------------------------------------------

class QService : public BnQService {
public:
    virtual ~QService();
    virtual void connect(const android::sp<qClient::IQClient>& client);
    virtual void connect(const android::sp<qClient::IQHDMIClient>& client);
    virtual android::status_t dispatch(uint32_t command,
            const android::Parcel* data,
            android::Parcel* reply);
    void onHdmiHotplug(int connected);
    void onCECMessageReceived(char *msg, ssize_t len);
    static void init();
private:
    QService();
    android::sp<qClient::IQClient> mClient;
    android::sp<qClient::IQHDMIClient> mHDMIClient;
    static QService *sQService;
};
}; // namespace qService
#endif // ANDROID_QSERVICE_H
