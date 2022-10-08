/*
* Copyright (c) 2013-14 The Linux Foundation. All rights reserved.
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

#ifndef QSERVICEUTILS_H
#define QSERVICEUTILS_H
#include <binder/Parcel.h>
#include <binder/IServiceManager.h>
#include <utils/RefBase.h>
#include <IQService.h>

// ----------------------------------------------------------------------------
// Helpers
// ----------------------------------------------------------------------------
inline android::sp<qService::IQService> getBinder() {
    android::sp<android::IServiceManager> sm = android::defaultServiceManager();
    android::sp<qService::IQService> binder =
            android::interface_cast<qService::IQService>
            (sm->getService(android::String16("display.qservice")));
    if (binder == NULL) {
        ALOGE("%s: invalid binder object", __FUNCTION__);
    }
    return binder;
}

inline android::status_t sendSingleParam(uint32_t command, uint32_t value) {
    android::status_t err = (android::status_t) android::FAILED_TRANSACTION;
    android::sp<qService::IQService> binder = getBinder();
    android::Parcel inParcel, outParcel;
    inParcel.writeInt32(value);
    if(binder != NULL) {
        err = binder->dispatch(command, &inParcel , &outParcel);
    }
    return err;
}

// ----------------------------------------------------------------------------
// Convenience wrappers that clients can call
// ----------------------------------------------------------------------------
inline android::status_t securing(uint32_t startEnd) {
    return sendSingleParam(qService::IQService::SECURING, startEnd);
}

inline android::status_t unsecuring(uint32_t startEnd) {
    return sendSingleParam(qService::IQService::UNSECURING, startEnd);
}

inline android::status_t screenRefresh() {
    return sendSingleParam(qService::IQService::SCREEN_REFRESH, 1);
}

inline android::status_t setExtOrientation(uint32_t orientation) {
    return sendSingleParam(qService::IQService::EXTERNAL_ORIENTATION,
            orientation);
}

inline android::status_t setBufferMirrorMode(uint32_t enable) {
    return sendSingleParam(qService::IQService::BUFFER_MIRRORMODE, enable);
}

inline android::status_t pauseWFD(uint32_t pause) {
    return sendSingleParam(qService::IQService::PAUSE_WFD, pause);
}

inline android::status_t setWfdStatus(uint32_t wfdStatus) {
    return sendSingleParam(qService::IQService::SET_WFD_STATUS, wfdStatus);
}
#endif /* end of include guard: QSERVICEUTILS_H */
