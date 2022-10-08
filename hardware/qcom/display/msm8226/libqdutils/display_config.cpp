/*
* Copyright (c) 2013-2014 The Linux Foundation. All rights reserved.
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

#include <display_config.h>
#include <QServiceUtils.h>

using namespace android;
using namespace qService;

namespace qdutils {

int isExternalConnected(void) {
    int ret;
    status_t err = (status_t) FAILED_TRANSACTION;
    sp<IQService> binder = getBinder();
    Parcel inParcel, outParcel;
    if(binder != NULL) {
        err = binder->dispatch(IQService::CHECK_EXTERNAL_STATUS,
                &inParcel , &outParcel);
    }
    if(err) {
        ALOGE("%s: Failed to get external status err=%d", __FUNCTION__, err);
        ret = err;
    } else {
        ret = outParcel.readInt32();
    }
    return ret;
}

int getDisplayAttributes(int dpy, DisplayAttributes_t& dpyattr) {
    status_t err = (status_t) FAILED_TRANSACTION;
    sp<IQService> binder = getBinder();
    Parcel inParcel, outParcel;
    inParcel.writeInt32(dpy);
    if(binder != NULL) {
        err = binder->dispatch(IQService::GET_DISPLAY_ATTRIBUTES,
                &inParcel, &outParcel);
    }
    if(!err) {
        dpyattr.vsync_period = outParcel.readInt32();
        dpyattr.xres = outParcel.readInt32();
        dpyattr.yres = outParcel.readInt32();
        dpyattr.xdpi = outParcel.readFloat();
        dpyattr.ydpi = outParcel.readFloat();
        dpyattr.panel_type = (char) outParcel.readInt32();
    } else {
        ALOGE("%s: Failed to get display attributes err=%d", __FUNCTION__, err);
    }
    return err;
}

int setHSIC(int dpy, const HSICData_t& hsic_data) {
    status_t err = (status_t) FAILED_TRANSACTION;
    sp<IQService> binder = getBinder();
    Parcel inParcel, outParcel;
    inParcel.writeInt32(dpy);
    inParcel.writeInt32(hsic_data.hue);
    inParcel.writeFloat(hsic_data.saturation);
    inParcel.writeInt32(hsic_data.intensity);
    inParcel.writeFloat(hsic_data.contrast);
    if(binder != NULL) {
        err = binder->dispatch(IQService::SET_HSIC_DATA, &inParcel, &outParcel);
    }
    if(err)
        ALOGE("%s: Failed to get external status err=%d", __FUNCTION__, err);
    return err;
}

int getDisplayVisibleRegion(int dpy, hwc_rect_t &rect) {
    status_t err = (status_t) FAILED_TRANSACTION;
    sp<IQService> binder = getBinder();
    Parcel inParcel, outParcel;
    inParcel.writeInt32(dpy);
    if(binder != NULL) {
        err = binder->dispatch(IQService::GET_DISPLAY_VISIBLE_REGION,
                &inParcel, &outParcel);
    }
    if(!err) {
        rect.left = outParcel.readInt32();
        rect.top = outParcel.readInt32();
        rect.right = outParcel.readInt32();
        rect.bottom = outParcel.readInt32();
    } else {
        ALOGE("%s: Failed to getVisibleRegion for dpy =%d: err = %d",
              __FUNCTION__, dpy, err);
    }
    return err;
}

int setViewFrame(int dpy, int l, int t, int r, int b) {
    status_t err = (status_t) FAILED_TRANSACTION;
    sp<IQService> binder = getBinder();
    Parcel inParcel, outParcel;
    inParcel.writeInt32(dpy);
    inParcel.writeInt32(l);
    inParcel.writeInt32(t);
    inParcel.writeInt32(r);
    inParcel.writeInt32(b);

    if(binder != NULL) {
        err = binder->dispatch(IQService::SET_VIEW_FRAME,
                &inParcel, &outParcel);
    }
    if(err)
        ALOGE("%s: Failed to set view frame for dpy %d err=%d",
                            __FUNCTION__, dpy, err);

    return err;
}

}; //namespace
