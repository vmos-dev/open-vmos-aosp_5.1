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

void QService::securing(uint32_t startEnd) {
    if(mClient.get()) {
        mClient->notifyCallback(SECURING, startEnd);
    }
}

void QService::unsecuring(uint32_t startEnd) {
    if(mClient.get()) {
        mClient->notifyCallback(UNSECURING, startEnd);
    }
}

void QService::connect(const sp<qClient::IQClient>& client) {
    mClient = client;
}

android::status_t QService::screenRefresh() {
    status_t result = NO_ERROR;
    if(mClient.get()) {
        result = mClient->notifyCallback(SCREEN_REFRESH, 0);
    }
    return result;
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
