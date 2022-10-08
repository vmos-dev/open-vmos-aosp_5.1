/*
 *  Copyright (c) 2013-14, The Linux Foundation. All rights reserved.
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
 * SUBSTITUTE GOODS OR CLIENTS; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <hwc_qclient.h>
#include <IQService.h>
#include <hwc_utils.h>
#include <mdp_version.h>

#define QCLIENT_DEBUG 0

using namespace android;
using namespace qService;
using namespace qhwc;

namespace qClient {

// ----------------------------------------------------------------------------
QClient::QClient(hwc_context_t *ctx) : mHwcContext(ctx),
        mMPDeathNotifier(new MPDeathNotifier(ctx))
{
    ALOGD_IF(QCLIENT_DEBUG, "QClient Constructor invoked");
}

QClient::~QClient()
{
    ALOGD_IF(QCLIENT_DEBUG,"QClient Destructor invoked");
}

static void securing(hwc_context_t *ctx, uint32_t startEnd) {
    Locker::Autolock _sl(ctx->mDrawLock);
    //The only way to make this class in this process subscribe to media
    //player's death.
    IMediaDeathNotifier::getMediaPlayerService();

    ctx->mSecuring = startEnd;
    //We're done securing
    if(startEnd == IQService::END)
        ctx->mSecureMode = true;
    if(ctx->proc)
        ctx->proc->invalidate(ctx->proc);
}

static void unsecuring(hwc_context_t *ctx, uint32_t startEnd) {
    Locker::Autolock _sl(ctx->mDrawLock);
    ctx->mSecuring = startEnd;
    //We're done unsecuring
    if(startEnd == IQService::END)
        ctx->mSecureMode = false;
    if(ctx->proc)
        ctx->proc->invalidate(ctx->proc);
}

void QClient::MPDeathNotifier::died() {
    Locker::Autolock _sl(mHwcContext->mDrawLock);
    ALOGD_IF(QCLIENT_DEBUG, "Media Player died");
    mHwcContext->mSecuring = false;
    mHwcContext->mSecureMode = false;
    if(mHwcContext->proc)
        mHwcContext->proc->invalidate(mHwcContext->proc);
}

static android::status_t screenRefresh(hwc_context_t *ctx) {
    status_t result = NO_INIT;
    if(ctx->proc) {
        ctx->proc->invalidate(ctx->proc);
        result = NO_ERROR;
    }
    return result;
}

static void setExtOrientation(hwc_context_t *ctx, uint32_t orientation) {
    ctx->mExtOrientation = orientation;
}

static void isExternalConnected(hwc_context_t* ctx, Parcel* outParcel) {
    int connected;
    connected = ctx->dpyAttr[HWC_DISPLAY_EXTERNAL].connected ? 1 : 0;
    outParcel->writeInt32(connected);
}

static void getDisplayAttributes(hwc_context_t* ctx, const Parcel* inParcel,
        Parcel* outParcel) {
    int dpy = inParcel->readInt32();
    outParcel->writeInt32(ctx->dpyAttr[dpy].vsync_period);
    outParcel->writeInt32(ctx->dpyAttr[dpy].xres);
    outParcel->writeInt32(ctx->dpyAttr[dpy].yres);
    outParcel->writeFloat(ctx->dpyAttr[dpy].xdpi);
    outParcel->writeFloat(ctx->dpyAttr[dpy].ydpi);
    //XXX: Need to check what to return for HDMI
    outParcel->writeInt32(ctx->mMDP.panel);
}
static void setHSIC(const Parcel* inParcel) {
    int dpy = inParcel->readInt32();
    ALOGD_IF(0, "In %s: dpy = %d", __FUNCTION__, dpy);
    HSICData_t hsic_data;
    hsic_data.hue = inParcel->readInt32();
    hsic_data.saturation = inParcel->readFloat();
    hsic_data.intensity = inParcel->readInt32();
    hsic_data.contrast = inParcel->readFloat();
    //XXX: Actually set the HSIC data through ABL lib
}


static void setBufferMirrorMode(hwc_context_t *ctx, uint32_t enable) {
    ctx->mBufferMirrorMode = enable;
}

static status_t getDisplayVisibleRegion(hwc_context_t* ctx, int dpy,
                                Parcel* outParcel) {
    // Get the info only if the dpy is valid
    if(dpy >= HWC_DISPLAY_PRIMARY && dpy <= HWC_DISPLAY_VIRTUAL) {
        Locker::Autolock _sl(ctx->mDrawLock);
        if(dpy && (ctx->mExtOrientation || ctx->mBufferMirrorMode)) {
            // Return the destRect on external, if external orienation
            // is enabled
            outParcel->writeInt32(ctx->dpyAttr[dpy].mDstRect.left);
            outParcel->writeInt32(ctx->dpyAttr[dpy].mDstRect.top);
            outParcel->writeInt32(ctx->dpyAttr[dpy].mDstRect.right);
            outParcel->writeInt32(ctx->dpyAttr[dpy].mDstRect.bottom);
        } else {
            outParcel->writeInt32(ctx->mViewFrame[dpy].left);
            outParcel->writeInt32(ctx->mViewFrame[dpy].top);
            outParcel->writeInt32(ctx->mViewFrame[dpy].right);
            outParcel->writeInt32(ctx->mViewFrame[dpy].bottom);
        }
        return NO_ERROR;
    } else {
        ALOGE("In %s: invalid dpy index %d", __FUNCTION__, dpy);
        return BAD_VALUE;
    }
}

static void pauseWFD(hwc_context_t *ctx, uint32_t pause) {
    /* TODO: Will remove pauseWFD once all the clients start using
     * setWfdStatus to indicate the status of WFD display
     */
    int dpy = HWC_DISPLAY_VIRTUAL;
    if(pause) {
        //WFD Pause
        handle_pause(ctx, dpy);
    } else {
        //WFD Resume
        handle_resume(ctx, dpy);
    }
}

static void setWfdStatus(hwc_context_t *ctx, uint32_t wfdStatus) {

    ALOGD_IF(HWC_WFDDISPSYNC_LOG,
             "%s: Received a binder call that WFD state is %s",
             __FUNCTION__,getExternalDisplayState(wfdStatus));
    int dpy = HWC_DISPLAY_VIRTUAL;

    if(wfdStatus == EXTERNAL_OFFLINE) {
        ctx->mWfdSyncLock.lock();
        ctx->mWfdSyncLock.signal();
        ctx->mWfdSyncLock.unlock();
    } else if(wfdStatus == EXTERNAL_PAUSE) {
        handle_pause(ctx, dpy);
    } else if(wfdStatus == EXTERNAL_RESUME) {
        handle_resume(ctx, dpy);
    }
}

status_t QClient::notifyCallback(uint32_t command, const Parcel* inParcel,
        Parcel* outParcel) {
    status_t ret = NO_ERROR;

    switch(command) {
        case IQService::SECURING:
            securing(mHwcContext, inParcel->readInt32());
            break;
        case IQService::UNSECURING:
            unsecuring(mHwcContext, inParcel->readInt32());
            break;
        case IQService::SCREEN_REFRESH:
            return screenRefresh(mHwcContext);
            break;
        case IQService::EXTERNAL_ORIENTATION:
            setExtOrientation(mHwcContext, inParcel->readInt32());
            break;
        case IQService::BUFFER_MIRRORMODE:
            setBufferMirrorMode(mHwcContext, inParcel->readInt32());
            break;
        case IQService::GET_DISPLAY_VISIBLE_REGION:
            ret = getDisplayVisibleRegion(mHwcContext, inParcel->readInt32(),
                                    outParcel);
            break;
        case IQService::CHECK_EXTERNAL_STATUS:
            isExternalConnected(mHwcContext, outParcel);
            break;
        case IQService::GET_DISPLAY_ATTRIBUTES:
            getDisplayAttributes(mHwcContext, inParcel, outParcel);
            break;
        case IQService::SET_HSIC_DATA:
            setHSIC(inParcel);
            break;
        case IQService::PAUSE_WFD:
            pauseWFD(mHwcContext, inParcel->readInt32());
            break;
        case IQService::SET_WFD_STATUS:
            setWfdStatus(mHwcContext,inParcel->readInt32());
            break;
        default:
            ret = NO_ERROR;
    }
    return ret;
}

}
