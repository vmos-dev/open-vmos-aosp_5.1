/*
* Copyright (c) 2013 The Linux Foundation. All rights reserved.
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

#include <unistd.h>
#include <overlay.h>
#include <overlayUtils.h>
#include <overlayWriteback.h>
#include <mdp_version.h>
#include "hwc_ad.h"
#include "hwc_utils.h"

#define DEBUG 0
using namespace overlay;
using namespace overlay::utils;
namespace qhwc {

//Opens writeback framebuffer and returns fd.
static int openWbFb() {
    int wbFd = -1;
    //Check opening which FB would connect LM to WB
    const int wbFbNum = Overlay::getFbForDpy(Overlay::DPY_WRITEBACK);
    if(wbFbNum >= 0) {
        char wbFbPath[256];
        snprintf (wbFbPath, sizeof(wbFbPath),
                "/sys/class/graphics/fb%d", wbFbNum);
        //Opening writeback fb first time would create ad node if the device
        //supports adaptive display
        wbFd = open(wbFbPath, O_RDONLY);
        if(wbFd < 0) {
            ALOGE("%s: Failed to open /sys/class/graphics/fb%d with error %s",
                    __func__, wbFbNum, strerror(errno));
        }
    } else {
        ALOGD_IF(DEBUG, "%s: No writeback available", __func__);
    }
    return wbFd;
}

static inline void closeWbFb(int& fd) {
    if(fd >= 0) {
        close(fd);
        fd = -1;
    } else {
        ALOGE("%s: Invalid fd %d", __func__, fd);
    }
}

//Helper to write data to ad node
static void adWrite(const int& value) {
    const int wbFbNum = Overlay::getFbForDpy(Overlay::DPY_WRITEBACK);
    char wbFbPath[256];
    snprintf (wbFbPath, sizeof(wbFbPath),
            "/sys/class/graphics/fb%d/ad", wbFbNum);
    int adFd = open(wbFbPath, O_WRONLY);
    if(adFd >= 0) {
        char opStr[4] = "";
        snprintf(opStr, sizeof(opStr), "%d", value);
        int ret = write(adFd, opStr, strlen(opStr));
        if(ret < 0) {
            ALOGE("%s: Failed to write %d with error %s",
                    __func__, value, strerror(errno));
        } else if (ret == 0){
            ALOGE("%s Nothing written to ad", __func__);
        } else {
            ALOGD_IF(DEBUG, "%s: Wrote %d to ad", __func__, value);
        }
        close(adFd);
    } else {
        ALOGE("%s: Failed to open /sys/class/graphics/fb%d/ad with error %s",
                __func__, wbFbNum, strerror(errno));
    }
}

//Helper to read data from ad node
static int adRead() {
    const int wbFbNum = Overlay::getFbForDpy(Overlay::DPY_WRITEBACK);
    int ret = -1;
    char wbFbPath[256];
    snprintf (wbFbPath, sizeof(wbFbPath),
            "/sys/class/graphics/fb%d/ad", wbFbNum);
    int adFd = open(wbFbPath, O_RDONLY);
    if(adFd >= 0) {
        char opStr[4] = {'\0'};
        if(read(adFd, opStr, strlen(opStr)) >= 0) {
            //Should return -1, 0 or 1
            ret = atoi(opStr);
            ALOGD_IF(DEBUG, "%s: Read %d from ad", __func__, ret);
        } else {
            ALOGE("%s: Read from ad node failed with error %s", __func__,
                    strerror(errno));
        }
        close(adFd);
    } else {
        ALOGD("%s: /sys/class/graphics/fb%d/ad could not be opened : %s",
                __func__, wbFbNum, strerror(errno));
    }
    return ret;
}

AssertiveDisplay::AssertiveDisplay() :mWbFd(-1), mDoable(false),
        mFeatureEnabled(false), mDest(overlay::utils::OV_INVALID) {
    int fd = openWbFb();
    if(fd >= 0) {
        //-1 means feature is disabled on device
        // 0 means feature exists but turned off, will be turned on by hwc
        // 1 means feature is turned on by hwc
        if(adRead() >= 0) {
            ALOGD_IF(DEBUG, "Assertive display feature supported");
            mFeatureEnabled = true;
        }
        closeWbFb(fd);
    }
}

void AssertiveDisplay::markDoable(hwc_context_t *ctx,
        const hwc_display_contents_1_t* list) {
    mDoable = false;
    if(mFeatureEnabled &&
        !isSecondaryConnected(ctx) &&
        ctx->listStats[HWC_DISPLAY_PRIMARY].yuvCount == 1) {
        int nYuvIndex = ctx->listStats[HWC_DISPLAY_PRIMARY].yuvIndices[0];
        const hwc_layer_1_t* layer = &list->hwLayers[nYuvIndex];
        private_handle_t *hnd = (private_handle_t *)layer->handle;
        if(hnd && hnd->width <= qdutils::MAX_DISPLAY_DIM) {
            mDoable = true;
        }
    }
}

bool AssertiveDisplay::prepare(hwc_context_t *ctx,
        const hwc_rect_t& crop,
        const Whf& whf,
        const private_handle_t *hnd) {
    if(!isDoable()) {
        if(isModeOn()) {
            //Cleanup one time during this switch
            const int off = 0;
            adWrite(off);
            closeWbFb(mWbFd);
        }
        return false;
    }

    ovutils::eDest dest = ctx->mOverlay->nextPipe(ovutils::OV_MDP_PIPE_VG,
            overlay::Overlay::DPY_WRITEBACK, Overlay::MIXER_DEFAULT);
    if(dest == OV_INVALID) {
        ALOGE("%s failed: No VG pipe available", __func__);
        mDoable = false;
        return false;
    }

    overlay::Writeback *wb = overlay::Writeback::getInstance();

    if(!wb->configureDpyInfo(hnd->width, hnd->height)) {
        ALOGE("%s: config display failed", __func__);
        mDoable = false;
        return false;
    }

    int tmpW, tmpH, size;
    int format = ovutils::getHALFormat(wb->getOutputFormat());
    if(format < 0) {
        ALOGE("%s invalid format %d", __func__, format);
        mDoable = false;
        return false;
    }

    size = getBufferSizeAndDimensions(hnd->width, hnd->height,
                format, tmpW, tmpH);

    if(!wb->configureMemory(size, isSecureBuffer(hnd))) {
        ALOGE("%s: config memory failed", __func__);
        mDoable = false;
        return false;
    }

    eMdpFlags mdpFlags = OV_MDP_FLAGS_NONE;
    if(isSecureBuffer(hnd)) {
        ovutils::setMdpFlags(mdpFlags,
                ovutils::OV_MDP_SECURE_OVERLAY_SESSION);
    }

    PipeArgs parg(mdpFlags, whf, ZORDER_0, IS_FG_OFF,
            ROT_FLAGS_NONE,
            ovutils::DEFAULT_PLANE_ALPHA,
            ovutils::OVERLAY_BLENDING_OPAQUE);
    hwc_rect_t dst = crop; //input same as output

    if(configMdp(ctx->mOverlay, parg, OVERLAY_TRANSFORM_0, crop, dst, NULL,
                dest) < 0) {
        ALOGE("%s: configMdp failed", __func__);
        mDoable = false;
        return false;
    }

    mDest = dest;
    if(!isModeOn()) {
        mWbFd = openWbFb();
        if(mWbFd >= 0) {
            //write to sysfs, one time during this switch
            const int on = 1;
            adWrite(on);
        }
    }
    return true;
}

bool AssertiveDisplay::draw(hwc_context_t *ctx, int fd, uint32_t offset) {
    if(!isDoable() || !isModeOn()) {
        return false;
    }

    if (!ctx->mOverlay->queueBuffer(fd, offset, mDest)) {
        ALOGE("%s: queueBuffer failed", __func__);
        return false;
    }

    overlay::Writeback *wb = overlay::Writeback::getInstance();
    if(!wb->writeSync()) {
        return false;
    }

    return true;
}

int AssertiveDisplay::getDstFd(hwc_context_t *ctx) const {
    overlay::Writeback *wb = overlay::Writeback::getInstance();
    return wb->getDstFd();
}

uint32_t AssertiveDisplay::getDstOffset(hwc_context_t *ctx) const {
    overlay::Writeback *wb = overlay::Writeback::getInstance();
    return wb->getOffset();
}

}
