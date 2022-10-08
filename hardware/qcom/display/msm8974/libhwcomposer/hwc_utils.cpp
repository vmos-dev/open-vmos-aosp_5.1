/*
 * Copyright (C) 2010 The Android Open Source Project
 * Copyright (C) 2012-2013, The Linux Foundation All rights reserved.
 *
 * Not a Contribution, Apache license notifications and license are retained
 * for attribution purposes only.
 *
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
#define ATRACE_TAG (ATRACE_TAG_GRAPHICS | ATRACE_TAG_HAL)
#define HWC_UTILS_DEBUG 0
#include <math.h>
#include <sys/ioctl.h>
#include <binder/IServiceManager.h>
#include <EGL/egl.h>
#include <cutils/properties.h>
#include <utils/Trace.h>
#include <gralloc_priv.h>
#include <overlay.h>
#include <overlayRotator.h>
#include <overlayWriteback.h>
#include "hwc_utils.h"
#include "hwc_mdpcomp.h"
#include "hwc_fbupdate.h"
#include "hwc_ad.h"
#include "mdp_version.h"
#include "hwc_copybit.h"
#include "external.h"
#include "hwc_qclient.h"
#include "QService.h"
#include "comptype.h"

using namespace qClient;
using namespace qService;
using namespace android;
using namespace overlay;
using namespace overlay::utils;
namespace ovutils = overlay::utils;

namespace qhwc {

static int openFramebufferDevice(hwc_context_t *ctx)
{
    struct fb_fix_screeninfo finfo;
    struct fb_var_screeninfo info;

    int fb_fd = openFb(HWC_DISPLAY_PRIMARY);
    if(fb_fd < 0) {
        ALOGE("%s: Error Opening FB : %s", __FUNCTION__, strerror(errno));
        return -errno;
    }

    if (ioctl(fb_fd, FBIOGET_VSCREENINFO, &info) == -1) {
        ALOGE("%s:Error in ioctl FBIOGET_VSCREENINFO: %s", __FUNCTION__,
                                                       strerror(errno));
        close(fb_fd);
        return -errno;
    }

    if (int(info.width) <= 0 || int(info.height) <= 0) {
        // the driver doesn't return that information
        // default to 160 dpi
        info.width  = ((info.xres * 25.4f)/160.0f + 0.5f);
        info.height = ((info.yres * 25.4f)/160.0f + 0.5f);
    }

    float xdpi = (info.xres * 25.4f) / info.width;
    float ydpi = (info.yres * 25.4f) / info.height;

#ifdef MSMFB_METADATA_GET
    struct msmfb_metadata metadata;
    memset(&metadata, 0 , sizeof(metadata));
    metadata.op = metadata_op_frame_rate;

    if (ioctl(fb_fd, MSMFB_METADATA_GET, &metadata) == -1) {
        ALOGE("%s:Error retrieving panel frame rate: %s", __FUNCTION__,
                                                      strerror(errno));
        close(fb_fd);
        return -errno;
    }

    float fps  = metadata.data.panel_frame_rate;
#else
    //XXX: Remove reserved field usage on all baselines
    //The reserved[3] field is used to store FPS by the driver.
    float fps  = info.reserved[3] & 0xFF;
#endif

    if (ioctl(fb_fd, FBIOGET_FSCREENINFO, &finfo) == -1) {
        ALOGE("%s:Error in ioctl FBIOGET_FSCREENINFO: %s", __FUNCTION__,
                                                       strerror(errno));
        close(fb_fd);
        return -errno;
    }

    ctx->dpyAttr[HWC_DISPLAY_PRIMARY].fd = fb_fd;
    //xres, yres may not be 32 aligned
    ctx->dpyAttr[HWC_DISPLAY_PRIMARY].stride = finfo.line_length /(info.xres/8);
    ctx->dpyAttr[HWC_DISPLAY_PRIMARY].xres = info.xres;
    ctx->dpyAttr[HWC_DISPLAY_PRIMARY].yres = info.yres;
    ctx->dpyAttr[HWC_DISPLAY_PRIMARY].xdpi = xdpi;
    ctx->dpyAttr[HWC_DISPLAY_PRIMARY].ydpi = ydpi;
    ctx->dpyAttr[HWC_DISPLAY_PRIMARY].vsync_period = 1000000000l / fps;

    //Unblank primary on first boot
    if(ioctl(fb_fd, FBIOBLANK,FB_BLANK_UNBLANK) < 0) {
        ALOGE("%s: Failed to unblank display", __FUNCTION__);
        return -errno;
    }
    ctx->dpyAttr[HWC_DISPLAY_PRIMARY].isActive = true;

    return 0;
}

void initContext(hwc_context_t *ctx)
{
    memset(&ctx->dpyAttr, 0, sizeof(ctx->dpyAttr));
    if(openFramebufferDevice(ctx) < 0) {
        ALOGE("%s: failed to open framebuffer!!", __FUNCTION__);
    }

    overlay::Overlay::initOverlay();
    ctx->mOverlay = overlay::Overlay::getInstance();
    ctx->mRotMgr = new RotMgr();
    ctx->mMDP.version = qdutils::MDPVersion::getInstance().getMDPVersion();
    ctx->mMDP.hasOverlay = qdutils::MDPVersion::getInstance().hasOverlay();
    ctx->mMDP.panel = qdutils::MDPVersion::getInstance().getPanelType();
    const int rightSplit = qdutils::MDPVersion::getInstance().getRightSplit();
    overlay::Overlay::initOverlay();
    ctx->mOverlay = overlay::Overlay::getInstance();
    ctx->mRotMgr = new RotMgr();

    //Is created and destroyed only once for primary
    //For external it could get created and destroyed multiple times depending
    //on what external we connect to.
    ctx->mFBUpdate[HWC_DISPLAY_PRIMARY] =
        IFBUpdate::getObject(ctx->dpyAttr[HWC_DISPLAY_PRIMARY].xres,
                rightSplit, HWC_DISPLAY_PRIMARY);

    // Check if the target supports copybit compostion (dyn/mdp/c2d) to
    // decide if we need to open the copybit module.
    int compositionType =
        qdutils::QCCompositionType::getInstance().getCompositionType();

    if (compositionType & (qdutils::COMPOSITION_TYPE_DYN |
                           qdutils::COMPOSITION_TYPE_MDP |
                           qdutils::COMPOSITION_TYPE_C2D)) {
            ctx->mCopyBit[HWC_DISPLAY_PRIMARY] = new CopyBit();
    }

    ctx->mExtDisplay = new ExternalDisplay(ctx);

    for (uint32_t i = 0; i < MAX_DISPLAYS; i++) {
        ctx->mLayerRotMap[i] = new LayerRotMap();
    }

    ctx->mMDPComp[HWC_DISPLAY_PRIMARY] =
         MDPComp::getObject(ctx->dpyAttr[HWC_DISPLAY_PRIMARY].xres,
                rightSplit, HWC_DISPLAY_PRIMARY);

    MDPComp::init(ctx);
    ctx->mAD = new AssertiveDisplay();

    ctx->vstate.enable = false;
    ctx->vstate.fakevsync = false;

    //Right now hwc starts the service but anybody could do it, or it could be
    //independent process as well.
    QService::init();
    sp<IQClient> client = new QClient(ctx);
    interface_cast<IQService>(
            defaultServiceManager()->getService(
            String16("display.qservice")))->connect(client);

    ALOGI("Initializing Qualcomm Hardware Composer");
    ALOGI("MDP version: %d", ctx->mMDP.version);
}

void closeContext(hwc_context_t *ctx)
{
    if(ctx->mOverlay) {
        delete ctx->mOverlay;
        ctx->mOverlay = NULL;
    }

    if(ctx->mRotMgr) {
        delete ctx->mRotMgr;
        ctx->mRotMgr = NULL;
    }

    for(int i = 0; i < MAX_DISPLAYS; i++) {
        if(ctx->mCopyBit[i]) {
            delete ctx->mCopyBit[i];
            ctx->mCopyBit[i] = NULL;
        }
    }

    if(ctx->dpyAttr[HWC_DISPLAY_PRIMARY].fd) {
        close(ctx->dpyAttr[HWC_DISPLAY_PRIMARY].fd);
        ctx->dpyAttr[HWC_DISPLAY_PRIMARY].fd = -1;
    }

    if(ctx->mExtDisplay) {
        delete ctx->mExtDisplay;
        ctx->mExtDisplay = NULL;
    }

    for(int i = 0; i < MAX_DISPLAYS; i++) {
        if(ctx->mFBUpdate[i]) {
            delete ctx->mFBUpdate[i];
            ctx->mFBUpdate[i] = NULL;
        }
        if(ctx->mMDPComp[i]) {
            delete ctx->mMDPComp[i];
            ctx->mMDPComp[i] = NULL;
        }
        if(ctx->mLayerRotMap[i]) {
            delete ctx->mLayerRotMap[i];
            ctx->mLayerRotMap[i] = NULL;
        }
    }

    if(ctx->mAD) {
        delete ctx->mAD;
        ctx->mAD = NULL;
    }
}


void dumpsys_log(android::String8& buf, const char* fmt, ...)
{
    va_list varargs;
    va_start(varargs, fmt);
    buf.appendFormatV(fmt, varargs);
    va_end(varargs);
}

/* Calculates the destination position based on the action safe rectangle */
void getActionSafePosition(hwc_context_t *ctx, int dpy, uint32_t& x,
                           uint32_t& y, uint32_t& w, uint32_t& h) {

    // if external supports underscan, do nothing
    // it will be taken care in the driver
    if(ctx->mExtDisplay->isCEUnderscanSupported())
        return;

    char value[PROPERTY_VALUE_MAX];
    // Read action safe properties
    property_get("persist.sys.actionsafe.width", value, "0");
    int asWidthRatio = atoi(value);
    property_get("persist.sys.actionsafe.height", value, "0");
    int asHeightRatio = atoi(value);

    if(!asWidthRatio && !asHeightRatio) {
        //No action safe ratio set, return
        return;
    }

    float wRatio = 1.0;
    float hRatio = 1.0;
    float xRatio = 1.0;
    float yRatio = 1.0;

    float fbWidth = ctx->dpyAttr[dpy].xres;
    float fbHeight = ctx->dpyAttr[dpy].yres;

    float asX = 0;
    float asY = 0;
    float asW = fbWidth;
    float asH= fbHeight;

    // based on the action safe ratio, get the Action safe rectangle
    asW = fbWidth * (1.0f -  asWidthRatio / 100.0f);
    asH = fbHeight * (1.0f -  asHeightRatio / 100.0f);
    asX = (fbWidth - asW) / 2;
    asY = (fbHeight - asH) / 2;

    // calculate the position ratio
    xRatio = (float)x/fbWidth;
    yRatio = (float)y/fbHeight;
    wRatio = (float)w/fbWidth;
    hRatio = (float)h/fbHeight;

    //Calculate the position...
    x = (xRatio * asW) + asX;
    y = (yRatio * asH) + asY;
    w = (wRatio * asW);
    h = (hRatio * asH);

    return;
}

bool needsScaling(hwc_context_t* ctx, hwc_layer_1_t const* layer,
        const int& dpy) {
    int dst_w, dst_h, src_w, src_h;

    hwc_rect_t displayFrame  = layer->displayFrame;
    hwc_rect_t sourceCrop = integerizeSourceCrop(layer->sourceCropf);
    trimLayer(ctx, dpy, layer->transform, sourceCrop, displayFrame);

    dst_w = displayFrame.right - displayFrame.left;
    dst_h = displayFrame.bottom - displayFrame.top;
    src_w = sourceCrop.right - sourceCrop.left;
    src_h = sourceCrop.bottom - sourceCrop.top;

    if(((src_w != dst_w) || (src_h != dst_h)))
        return true;

    return false;
}

bool isAlphaScaled(hwc_context_t* ctx, hwc_layer_1_t const* layer,
        const int& dpy) {
    if(needsScaling(ctx, layer, dpy) && isAlphaPresent(layer)) {
        return true;
    }
    return false;
}

bool isAlphaPresent(hwc_layer_1_t const* layer) {
    private_handle_t *hnd = (private_handle_t *)layer->handle;
    if(hnd) {
        int format = hnd->format;
        switch(format) {
        case HAL_PIXEL_FORMAT_RGBA_8888:
        case HAL_PIXEL_FORMAT_BGRA_8888:
            // In any more formats with Alpha go here..
            return true;
        default : return false;
        }
    }
    return false;
}

// Let CABL know we have a YUV layer
static void setYUVProp(int yuvCount) {
    static char property[PROPERTY_VALUE_MAX];
    if(yuvCount > 0) {
        if (property_get("hw.cabl.yuv", property, NULL) > 0) {
            if (atoi(property) != 1) {
                property_set("hw.cabl.yuv", "1");
            }
        }
    } else {
        if (property_get("hw.cabl.yuv", property, NULL) > 0) {
            if (atoi(property) != 0) {
                property_set("hw.cabl.yuv", "0");
            }
        }
    }
}

void setListStats(hwc_context_t *ctx,
        const hwc_display_contents_1_t *list, int dpy) {
    const int prevYuvCount = ctx->listStats[dpy].yuvCount;
    memset(&ctx->listStats[dpy], 0, sizeof(ListStats));
    ctx->listStats[dpy].numAppLayers = list->numHwLayers - 1;
    ctx->listStats[dpy].fbLayerIndex = list->numHwLayers - 1;
    ctx->listStats[dpy].skipCount = 0;
    ctx->listStats[dpy].needsAlphaScale = false;
    ctx->listStats[dpy].preMultipliedAlpha = false;
    ctx->listStats[dpy].planeAlpha = false;
    ctx->listStats[dpy].isSecurePresent = false;
    ctx->listStats[dpy].yuvCount = 0;

    for (size_t i = 0; i < (size_t)ctx->listStats[dpy].numAppLayers; i++) {
        hwc_layer_1_t const* layer = &list->hwLayers[i];
        private_handle_t *hnd = (private_handle_t *)layer->handle;

        // continue if number of app layers exceeds MAX_NUM_APP_LAYERS
        if(ctx->listStats[dpy].numAppLayers > MAX_NUM_APP_LAYERS)
            continue;

        //reset yuv indices
        ctx->listStats[dpy].yuvIndices[i] = -1;

        if (isSecureBuffer(hnd)) {
            ctx->listStats[dpy].isSecurePresent = true;
        }

        if (isSkipLayer(&list->hwLayers[i])) {
            ctx->listStats[dpy].skipCount++;
        } else if (UNLIKELY(isYuvBuffer(hnd))) {
            int& yuvCount = ctx->listStats[dpy].yuvCount;
            ctx->listStats[dpy].yuvIndices[yuvCount] = i;
            yuvCount++;

            if((layer->transform & HWC_TRANSFORM_ROT_90) &&
                    canUseRotator(ctx, dpy)) {
                if( (dpy == HWC_DISPLAY_PRIMARY) &&
                        ctx->mOverlay->isPipeTypeAttached(OV_MDP_PIPE_DMA)) {
                    ctx->isPaddingRound = true;
                }
                Overlay::setDMAMode(Overlay::DMA_BLOCK_MODE);
            }
        }
        if(layer->blending == HWC_BLENDING_PREMULT)
            ctx->listStats[dpy].preMultipliedAlpha = true;
        if(layer->planeAlpha < 0xFF)
            ctx->listStats[dpy].planeAlpha = true;
        if(!ctx->listStats[dpy].needsAlphaScale)
            ctx->listStats[dpy].needsAlphaScale =
                    isAlphaScaled(ctx, layer, dpy);
    }
    setYUVProp(ctx->listStats[dpy].yuvCount);
    //The marking of video begin/end is useful on some targets where we need
    //to have a padding round to be able to shift pipes across mixers.
    if(prevYuvCount != ctx->listStats[dpy].yuvCount) {
        ctx->mVideoTransFlag = true;
    }
    if(dpy == HWC_DISPLAY_PRIMARY) {
        ctx->mAD->markDoable(ctx, list);
    }
}


static void calc_cut(double& leftCutRatio, double& topCutRatio,
        double& rightCutRatio, double& bottomCutRatio, int orient) {
    if(orient & HAL_TRANSFORM_FLIP_H) {
        swap(leftCutRatio, rightCutRatio);
    }
    if(orient & HAL_TRANSFORM_FLIP_V) {
        swap(topCutRatio, bottomCutRatio);
    }
    if(orient & HAL_TRANSFORM_ROT_90) {
        //Anti clock swapping
        double tmpCutRatio = leftCutRatio;
        leftCutRatio = topCutRatio;
        topCutRatio = rightCutRatio;
        rightCutRatio = bottomCutRatio;
        bottomCutRatio = tmpCutRatio;
    }
}

bool isSecuring(hwc_context_t* ctx, hwc_layer_1_t const* layer) {
    if((ctx->mMDP.version < qdutils::MDSS_V5) &&
       (ctx->mMDP.version > qdutils::MDP_V3_0) &&
        ctx->mSecuring) {
        return true;
    }
    //  On A-Family, Secure policy is applied system wide and not on
    //  buffers.
    if (isSecureModePolicy(ctx->mMDP.version)) {
        private_handle_t *hnd = (private_handle_t *)layer->handle;
        if(ctx->mSecureMode) {
            if (! isSecureBuffer(hnd)) {
                // This code path executes for the following usecase:
                // Some Apps in which first few seconds, framework
                // sends non-secure buffer and with out destroying
                // surfaces, switches to secure buffer thereby exposing
                // vulnerability on A-family devices. Catch this situation
                // and handle it gracefully by allowing it to be composed by
                // GPU.
                ALOGD_IF(HWC_UTILS_DEBUG, "%s: Handle non-secure video layer"
                         "during secure playback gracefully", __FUNCTION__);
                return true;
            }
        } else {
            if (isSecureBuffer(hnd)) {
                // This code path executes for the following usecase:
                // For some Apps, when User terminates playback, Framework
                // doesnt destroy video surface and video surface still
                // comes to Display HAL. This exposes vulnerability on
                // A-family. Catch this situation and handle it gracefully
                // by allowing it to be composed by GPU.
                ALOGD_IF(HWC_UTILS_DEBUG, "%s: Handle secure video layer"
                         "during non-secure playback gracefully", __FUNCTION__);
                return true;
            }
        }
    }
    return false;
}

bool isSecureModePolicy(int mdpVersion) {
    if (mdpVersion < qdutils::MDSS_V5)
        return true;
    else
        return false;
}

int getBlending(int blending) {
    switch(blending) {
    case HWC_BLENDING_NONE:
        return overlay::utils::OVERLAY_BLENDING_OPAQUE;
    case HWC_BLENDING_PREMULT:
        return overlay::utils::OVERLAY_BLENDING_PREMULT;
    case HWC_BLENDING_COVERAGE :
    default:
        return overlay::utils::OVERLAY_BLENDING_COVERAGE;
    }
}

//Crops source buffer against destination and FB boundaries
void calculate_crop_rects(hwc_rect_t& crop, hwc_rect_t& dst,
                          const hwc_rect_t& scissor, int orient) {

    int& crop_l = crop.left;
    int& crop_t = crop.top;
    int& crop_r = crop.right;
    int& crop_b = crop.bottom;
    int crop_w = crop.right - crop.left;
    int crop_h = crop.bottom - crop.top;

    int& dst_l = dst.left;
    int& dst_t = dst.top;
    int& dst_r = dst.right;
    int& dst_b = dst.bottom;
    int dst_w = abs(dst.right - dst.left);
    int dst_h = abs(dst.bottom - dst.top);

    const int& sci_l = scissor.left;
    const int& sci_t = scissor.top;
    const int& sci_r = scissor.right;
    const int& sci_b = scissor.bottom;
    int sci_w = abs(sci_r - sci_l);
    int sci_h = abs(sci_b - sci_t);

    double leftCutRatio = 0.0, rightCutRatio = 0.0, topCutRatio = 0.0,
            bottomCutRatio = 0.0;

    if(dst_l < sci_l) {
        leftCutRatio = (double)(sci_l - dst_l) / (double)dst_w;
        dst_l = sci_l;
    }

    if(dst_r > sci_r) {
        rightCutRatio = (double)(dst_r - sci_r) / (double)dst_w;
        dst_r = sci_r;
    }

    if(dst_t < sci_t) {
        topCutRatio = (double)(sci_t - dst_t) / (double)dst_h;
        dst_t = sci_t;
    }

    if(dst_b > sci_b) {
        bottomCutRatio = (double)(dst_b - sci_b) / (double)dst_h;
        dst_b = sci_b;
    }

    calc_cut(leftCutRatio, topCutRatio, rightCutRatio, bottomCutRatio, orient);
    crop_l += crop_w * leftCutRatio;
    crop_t += crop_h * topCutRatio;
    crop_r -= crop_w * rightCutRatio;
    crop_b -= crop_h * bottomCutRatio;
}

void getNonWormholeRegion(hwc_display_contents_1_t* list,
                              hwc_rect_t& nwr)
{
    uint32_t last = list->numHwLayers - 1;
    hwc_rect_t fbDisplayFrame = list->hwLayers[last].displayFrame;
    //Initiliaze nwr to first frame
    nwr.left =  list->hwLayers[0].displayFrame.left;
    nwr.top =  list->hwLayers[0].displayFrame.top;
    nwr.right =  list->hwLayers[0].displayFrame.right;
    nwr.bottom =  list->hwLayers[0].displayFrame.bottom;

    for (uint32_t i = 1; i < last; i++) {
        hwc_rect_t displayFrame = list->hwLayers[i].displayFrame;
        nwr.left   = min(nwr.left, displayFrame.left);
        nwr.top    = min(nwr.top, displayFrame.top);
        nwr.right  = max(nwr.right, displayFrame.right);
        nwr.bottom = max(nwr.bottom, displayFrame.bottom);
    }

    //Intersect with the framebuffer
    nwr.left   = max(nwr.left, fbDisplayFrame.left);
    nwr.top    = max(nwr.top, fbDisplayFrame.top);
    nwr.right  = min(nwr.right, fbDisplayFrame.right);
    nwr.bottom = min(nwr.bottom, fbDisplayFrame.bottom);

}

void closeAcquireFds(hwc_display_contents_1_t* list, int dpy) {
    if(LIKELY(list)) {
        for(uint32_t i = 0; i < list->numHwLayers; i++) {
            //Close the acquireFenceFds
            //HWC_FRAMEBUFFER are -1 already by SF, rest we close.
            if(list->hwLayers[i].acquireFenceFd >= 0) {
                close(list->hwLayers[i].acquireFenceFd);
                list->hwLayers[i].acquireFenceFd = -1;
            }
        }

        //Writeback
        if(dpy > HWC_DISPLAY_EXTERNAL && list->outbufAcquireFenceFd >= 0) {
            close(list->outbufAcquireFenceFd);
            list->outbufAcquireFenceFd = -1;
        }
    }
}

int hwc_sync(hwc_context_t *ctx, hwc_display_contents_1_t* list, int dpy,
        int fd) {
    ATRACE_CALL();
    int ret = 0;
    int acquireFd[MAX_NUM_APP_LAYERS];
    int count = 0;
    int releaseFd = -1;
    int retireFd = -1;
    int fbFd = -1;
    bool swapzero = false;
    int mdpVersion = qdutils::MDPVersion::getInstance().getMDPVersion();

    struct mdp_buf_sync data;
    memset(&data, 0, sizeof(data));
    data.acq_fen_fd = acquireFd;
    data.rel_fen_fd = &releaseFd;
    data.retire_fen_fd = &retireFd;

    char property[PROPERTY_VALUE_MAX];
    if(property_get("debug.egl.swapinterval", property, "1") > 0) {
        if(atoi(property) == 0)
            swapzero = true;
    }

    for(uint32_t i = 0; i < ctx->mLayerRotMap[dpy]->getCount(); i++) {
        int rotFd = ctx->mRotMgr->getRotDevFd();
        int rotReleaseFd = -1;
        int rotRetireFd = -1;
        struct mdp_buf_sync rotData;
        memset(&rotData, 0, sizeof(rotData));
        rotData.acq_fen_fd =
                &ctx->mLayerRotMap[dpy]->getLayer(i)->acquireFenceFd;
        rotData.rel_fen_fd = &rotReleaseFd; //driver to populate this
        rotData.retire_fen_fd = &rotRetireFd;
        rotData.session_id = ctx->mLayerRotMap[dpy]->getRot(i)->getSessId();
        int ret = 0;
        ret = ioctl(rotFd, MSMFB_BUFFER_SYNC, &rotData);
        if(ret < 0) {
            ALOGE("%s: ioctl MSMFB_BUFFER_SYNC failed for rot sync, err=%s",
                    __FUNCTION__, strerror(errno));
        } else {
            close(ctx->mLayerRotMap[dpy]->getLayer(i)->acquireFenceFd);
            //For MDP to wait on.
            ctx->mLayerRotMap[dpy]->getLayer(i)->acquireFenceFd =
                    dup(rotReleaseFd);
            //A buffer is free to be used by producer as soon as its copied to
            //rotator
            ctx->mLayerRotMap[dpy]->getLayer(i)->releaseFenceFd =
                    rotReleaseFd;
            //Not used for rotator
            close(rotRetireFd);
        }
    }

    //Accumulate acquireFenceFds for MDP
    if(dpy > HWC_DISPLAY_EXTERNAL && list->outbufAcquireFenceFd >= 0) {
        //Writeback output buffer
        acquireFd[count++] = list->outbufAcquireFenceFd;
    }

    for(uint32_t i = 0; i < list->numHwLayers; i++) {
        if(list->hwLayers[i].compositionType == HWC_OVERLAY &&
                        list->hwLayers[i].acquireFenceFd >= 0) {
            if(UNLIKELY(swapzero))
                acquireFd[count++] = -1;
            else
                acquireFd[count++] = list->hwLayers[i].acquireFenceFd;
        }
        if(list->hwLayers[i].compositionType == HWC_FRAMEBUFFER_TARGET) {
            if(UNLIKELY(swapzero))
                acquireFd[count++] = -1;
            else if(fd >= 0) {
                //set the acquireFD from fd - which is coming from c2d
                acquireFd[count++] = fd;
                // Buffer sync IOCTL should be async when using c2d fence is
                // used
                data.flags &= ~MDP_BUF_SYNC_FLAG_WAIT;
            } else if(list->hwLayers[i].acquireFenceFd >= 0)
                acquireFd[count++] = list->hwLayers[i].acquireFenceFd;
        }
    }

    data.acq_fen_fd_cnt = count;
    fbFd = ctx->dpyAttr[dpy].fd;

    //Waits for acquire fences, returns a release fence
    if(LIKELY(!swapzero)) {
        uint64_t start = systemTime();
        ret = ioctl(fbFd, MSMFB_BUFFER_SYNC, &data);
        ALOGD_IF(HWC_UTILS_DEBUG, "%s: time taken for MSMFB_BUFFER_SYNC IOCTL = %d",
                            __FUNCTION__, (size_t) ns2ms(systemTime() - start));
    }

    if(ret < 0) {
        ALOGE("%s: ioctl MSMFB_BUFFER_SYNC failed, err=%s",
                  __FUNCTION__, strerror(errno));
        ALOGE("%s: acq_fen_fd_cnt=%d flags=%d fd=%d dpy=%d numHwLayers=%d",
              __FUNCTION__, data.acq_fen_fd_cnt, data.flags, fbFd,
              dpy, list->numHwLayers);
    }

    for(uint32_t i = 0; i < list->numHwLayers; i++) {
        if(list->hwLayers[i].compositionType == HWC_OVERLAY ||
           list->hwLayers[i].compositionType == HWC_FRAMEBUFFER_TARGET) {
            //Populate releaseFenceFds.
            if(UNLIKELY(swapzero)) {
                list->hwLayers[i].releaseFenceFd = -1;
            } else if(list->hwLayers[i].releaseFenceFd < 0) {
                //If rotator has not already populated this field.
                list->hwLayers[i].releaseFenceFd = dup(releaseFd);
            }
        }
    }

    if(fd >= 0) {
        close(fd);
        fd = -1;
    }

    if (ctx->mCopyBit[dpy])
        ctx->mCopyBit[dpy]->setReleaseFd(releaseFd);

    //Signals when MDP finishes reading rotator buffers.
    ctx->mLayerRotMap[dpy]->setReleaseFd(releaseFd);
    close(releaseFd);

    if(UNLIKELY(swapzero))
        list->retireFenceFd = -1;
    else
        list->retireFenceFd = retireFd;
    return ret;
}

void trimLayer(hwc_context_t *ctx, const int& dpy, const int& transform,
        hwc_rect_t& crop, hwc_rect_t& dst) {
    int hw_w = ctx->dpyAttr[dpy].xres;
    int hw_h = ctx->dpyAttr[dpy].yres;
    if(dst.left < 0 || dst.top < 0 ||
            dst.right > hw_w || dst.bottom > hw_h) {
        hwc_rect_t scissor = {0, 0, hw_w, hw_h };
        qhwc::calculate_crop_rects(crop, dst, scissor, transform);
    }
}

void setMdpFlags(hwc_layer_1_t *layer,
        ovutils::eMdpFlags &mdpFlags,
        int rotDownscale) {
    private_handle_t *hnd = (private_handle_t *)layer->handle;
    MetaData_t *metadata = (MetaData_t *)hnd->base_metadata;
    const int& transform = layer->transform;

    if(layer->blending == HWC_BLENDING_PREMULT) {
        ovutils::setMdpFlags(mdpFlags,
                ovutils::OV_MDP_BLEND_FG_PREMULT);
    }

    if(isYuvBuffer(hnd)) {
        if(isSecureBuffer(hnd)) {
            ovutils::setMdpFlags(mdpFlags,
                    ovutils::OV_MDP_SECURE_OVERLAY_SESSION);
        }
        if(metadata && (metadata->operation & PP_PARAM_INTERLACED) &&
                metadata->interlaced) {
            ovutils::setMdpFlags(mdpFlags,
                    ovutils::OV_MDP_DEINTERLACE);
        }
        //Pre-rotation will be used using rotator.
        if(transform & HWC_TRANSFORM_ROT_90) {
            ovutils::setMdpFlags(mdpFlags,
                    ovutils::OV_MDP_SOURCE_ROTATED_90);
        }
    }

    //No 90 component and no rot-downscale then flips done by MDP
    //If we use rot then it might as well do flips
    if(!(layer->transform & HWC_TRANSFORM_ROT_90) && !rotDownscale) {
        if(layer->transform & HWC_TRANSFORM_FLIP_H) {
            ovutils::setMdpFlags(mdpFlags, ovutils::OV_MDP_FLIP_H);
        }

        if(layer->transform & HWC_TRANSFORM_FLIP_V) {
            ovutils::setMdpFlags(mdpFlags,  ovutils::OV_MDP_FLIP_V);
        }
    }

    if(metadata &&
        ((metadata->operation & PP_PARAM_HSIC)
        || (metadata->operation & PP_PARAM_IGC)
        || (metadata->operation & PP_PARAM_SHARP2))) {
        ovutils::setMdpFlags(mdpFlags, ovutils::OV_MDP_PP_EN);
    }
}

int configRotator(Rotator *rot, const Whf& whf,
        hwc_rect_t& crop, const eMdpFlags& mdpFlags,
        const eTransform& orient, const int& downscale) {
    rot->setSource(whf);

    if (qdutils::MDPVersion::getInstance().getMDPVersion() >=
        qdutils::MDSS_V5) {
        uint32_t crop_w = (crop.right - crop.left);
        uint32_t crop_h = (crop.bottom - crop.top);
        if (ovutils::isYuv(whf.format)) {
            ovutils::normalizeCrop((uint32_t&)crop.left, crop_w);
            ovutils::normalizeCrop((uint32_t&)crop.top, crop_h);
            // For interlaced, crop.h should be 4-aligned
            if ((mdpFlags & ovutils::OV_MDP_DEINTERLACE) && (crop_h % 4))
                crop_h = ovutils::aligndown(crop_h, 4);
            crop.right = crop.left + crop_w;
            crop.bottom = crop.top + crop_h;
        }
        Dim rotCrop(crop.left, crop.top, crop_w, crop_h);
        rot->setCrop(rotCrop);
    }

    rot->setFlags(mdpFlags);
    rot->setTransform(orient);
    rot->setDownscale(downscale);
    if(!rot->commit()) return -1;
    return 0;
}

int configMdp(Overlay *ov, const PipeArgs& parg,
        const eTransform& orient, const hwc_rect_t& crop,
        const hwc_rect_t& pos, const MetaData_t *metadata,
        const eDest& dest) {
    ov->setSource(parg, dest);
    ov->setTransform(orient, dest);

    int crop_w = crop.right - crop.left;
    int crop_h = crop.bottom - crop.top;
    Dim dcrop(crop.left, crop.top, crop_w, crop_h);
    ov->setCrop(dcrop, dest);

    int posW = pos.right - pos.left;
    int posH = pos.bottom - pos.top;
    Dim position(pos.left, pos.top, posW, posH);
    ov->setPosition(position, dest);

    if (metadata)
        ov->setVisualParams(*metadata, dest);

    if (!ov->commit(dest)) {
        return -1;
    }
    return 0;
}

void updateSource(eTransform& orient, Whf& whf,
        hwc_rect_t& crop) {
    Dim srcCrop(crop.left, crop.top,
            crop.right - crop.left,
            crop.bottom - crop.top);
    orient = static_cast<eTransform>(ovutils::getMdpOrient(orient));
    preRotateSource(orient, whf, srcCrop);
    if (qdutils::MDPVersion::getInstance().getMDPVersion() >=
        qdutils::MDSS_V5) {
        // Source for overlay will be the cropped (and rotated)
        crop.left = 0;
        crop.top = 0;
        crop.right = srcCrop.w;
        crop.bottom = srcCrop.h;
        // Set width & height equal to sourceCrop w & h
        whf.w = srcCrop.w;
        whf.h = srcCrop.h;
    } else {
        crop.left = srcCrop.x;
        crop.top = srcCrop.y;
        crop.right = srcCrop.x + srcCrop.w;
        crop.bottom = srcCrop.y + srcCrop.h;
    }
}

int configureLowRes(hwc_context_t *ctx, hwc_layer_1_t *layer,
        const int& dpy, eMdpFlags& mdpFlags, const eZorder& z,
        const eIsFg& isFg, const eDest& dest, Rotator **rot) {

    private_handle_t *hnd = (private_handle_t *)layer->handle;
    if(!hnd) {
        ALOGE("%s: layer handle is NULL", __FUNCTION__);
        ctx->mLayerRotMap[dpy]->reset();
        return -1;
    }

    MetaData_t *metadata = (MetaData_t *)hnd->base_metadata;

    hwc_rect_t crop = integerizeSourceCrop(layer->sourceCropf);
    hwc_rect_t dst = layer->displayFrame;
    int transform = layer->transform;
    eTransform orient = static_cast<eTransform>(transform);
    int downscale = 0;
    int rotFlags = ovutils::ROT_FLAGS_NONE;
    Whf whf(getWidth(hnd), getHeight(hnd),
            getMdpFormat(hnd->format), hnd->size);

    uint32_t x = dst.left, y  = dst.top;
    uint32_t w = dst.right - dst.left;
    uint32_t h = dst.bottom - dst.top;

    if(dpy) {
        // Calculate the actionsafe dimensions for External(dpy = 1 or 2)
        getActionSafePosition(ctx, dpy, x, y, w, h);
        // Convert position to hwc_rect_t
        dst.left = x;
        dst.top = y;
        dst.right = w + dst.left;
        dst.bottom = h + dst.top;
    }

    if(isYuvBuffer(hnd) && ctx->mMDP.version >= qdutils::MDP_V4_2 &&
       ctx->mMDP.version < qdutils::MDSS_V5) {
        downscale =  getDownscaleFactor(
            crop.right - crop.left,
            crop.bottom - crop.top,
            dst.right - dst.left,
            dst.bottom - dst.top);
        if(downscale) {
            rotFlags = ROT_DOWNSCALE_ENABLED;
        }
    }

    setMdpFlags(layer, mdpFlags, downscale);
    trimLayer(ctx, dpy, transform, crop, dst);

    //Will do something only if feature enabled and conditions suitable
    //hollow call otherwise
    if(ctx->mAD->prepare(ctx, crop, whf, hnd)) {
        overlay::Writeback *wb = overlay::Writeback::getInstance();
        whf.format = wb->getOutputFormat();
    }

    if(isYuvBuffer(hnd) && //if 90 component or downscale, use rot
            ((transform & HWC_TRANSFORM_ROT_90) || downscale)) {
        *rot = ctx->mRotMgr->getNext();
        if(*rot == NULL) return -1;
        BwcPM::setBwc(ctx, crop, dst, transform, mdpFlags);
        //Configure rotator for pre-rotation
        if(configRotator(*rot, whf, crop, mdpFlags, orient, downscale) < 0) {
            ALOGE("%s: configRotator failed!", __FUNCTION__);
            ctx->mOverlay->clear(dpy);
            return -1;
        }
        ctx->mLayerRotMap[dpy]->add(layer, *rot);
        whf.format = (*rot)->getDstFormat();
        updateSource(orient, whf, crop);
        rotFlags |= ovutils::ROT_PREROTATED;
    }

    //For the mdp, since either we are pre-rotating or MDP does flips
    orient = OVERLAY_TRANSFORM_0;
    transform = 0;

    PipeArgs parg(mdpFlags, whf, z, isFg,
                  static_cast<eRotFlags>(rotFlags), layer->planeAlpha,
                  (ovutils::eBlending) getBlending(layer->blending));

    if(configMdp(ctx->mOverlay, parg, orient, crop, dst, metadata, dest) < 0) {
        ALOGE("%s: commit failed for low res panel", __FUNCTION__);
        ctx->mLayerRotMap[dpy]->reset();
        return -1;
    }
    return 0;
}

int configureHighRes(hwc_context_t *ctx, hwc_layer_1_t *layer,
        const int& dpy, eMdpFlags& mdpFlagsL, const eZorder& z,
        const eIsFg& isFg, const eDest& lDest, const eDest& rDest,
        Rotator **rot) {
    private_handle_t *hnd = (private_handle_t *)layer->handle;
    if(!hnd) {
        ALOGE("%s: layer handle is NULL", __FUNCTION__);
        return -1;
    }

    MetaData_t *metadata = (MetaData_t *)hnd->base_metadata;

    int hw_w = ctx->dpyAttr[dpy].xres;
    int hw_h = ctx->dpyAttr[dpy].yres;
    hwc_rect_t crop = integerizeSourceCrop(layer->sourceCropf);
    hwc_rect_t dst = layer->displayFrame;
    int transform = layer->transform;
    eTransform orient = static_cast<eTransform>(transform);
    const int downscale = 0;
    int rotFlags = ROT_FLAGS_NONE;

    Whf whf(getWidth(hnd), getHeight(hnd),
            getMdpFormat(hnd->format), hnd->size);

    setMdpFlags(layer, mdpFlagsL, 0);
    trimLayer(ctx, dpy, transform, crop, dst);

    //Will do something only if feature enabled and conditions suitable
    //hollow call otherwise
    if(ctx->mAD->prepare(ctx, crop, whf, hnd)) {
        overlay::Writeback *wb = overlay::Writeback::getInstance();
        whf.format = wb->getOutputFormat();
    }

    if(isYuvBuffer(hnd) && (transform & HWC_TRANSFORM_ROT_90)) {
        (*rot) = ctx->mRotMgr->getNext();
        if((*rot) == NULL) return -1;
        //Configure rotator for pre-rotation
        if(configRotator(*rot, whf, crop, mdpFlagsL, orient, downscale) < 0) {
            ALOGE("%s: configRotator failed!", __FUNCTION__);
            ctx->mOverlay->clear(dpy);
            return -1;
        }
        ctx->mLayerRotMap[dpy]->add(layer, *rot);
        whf.format = (*rot)->getDstFormat();
        updateSource(orient, whf, crop);
        rotFlags |= ROT_PREROTATED;
    }

    eMdpFlags mdpFlagsR = mdpFlagsL;
    setMdpFlags(mdpFlagsR, OV_MDSS_MDP_RIGHT_MIXER);

    hwc_rect_t tmp_cropL, tmp_dstL;
    hwc_rect_t tmp_cropR, tmp_dstR;

    const int lSplit = getLeftSplit(ctx, dpy);

    if(lDest != OV_INVALID) {
        tmp_cropL = crop;
        tmp_dstL = dst;
        hwc_rect_t scissor = {0, 0, lSplit, hw_h };
        qhwc::calculate_crop_rects(tmp_cropL, tmp_dstL, scissor, 0);
    }
    if(rDest != OV_INVALID) {
        tmp_cropR = crop;
        tmp_dstR = dst;
        hwc_rect_t scissor = {lSplit, 0, hw_w, hw_h };
        qhwc::calculate_crop_rects(tmp_cropR, tmp_dstR, scissor, 0);
    }

    //When buffer is H-flipped, contents of mixer config also needs to swapped
    //Not needed if the layer is confined to one half of the screen.
    //If rotator has been used then it has also done the flips, so ignore them.
    if((orient & OVERLAY_TRANSFORM_FLIP_H) && lDest != OV_INVALID
            && rDest != OV_INVALID && (*rot) == NULL) {
        hwc_rect_t new_cropR;
        new_cropR.left = tmp_cropL.left;
        new_cropR.right = new_cropR.left + (tmp_cropR.right - tmp_cropR.left);

        hwc_rect_t new_cropL;
        new_cropL.left  = new_cropR.right;
        new_cropL.right = tmp_cropR.right;

        tmp_cropL.left =  new_cropL.left;
        tmp_cropL.right =  new_cropL.right;

        tmp_cropR.left = new_cropR.left;
        tmp_cropR.right =  new_cropR.right;

    }

    //For the mdp, since either we are pre-rotating or MDP does flips
    orient = OVERLAY_TRANSFORM_0;
    transform = 0;

    //configure left mixer
    if(lDest != OV_INVALID) {
        PipeArgs pargL(mdpFlagsL, whf, z, isFg,
                       static_cast<eRotFlags>(rotFlags), layer->planeAlpha,
                       (ovutils::eBlending) getBlending(layer->blending));

        if(configMdp(ctx->mOverlay, pargL, orient,
                tmp_cropL, tmp_dstL, metadata, lDest) < 0) {
            ALOGE("%s: commit failed for left mixer config", __FUNCTION__);
            return -1;
        }
    }

    //configure right mixer
    if(rDest != OV_INVALID) {
        PipeArgs pargR(mdpFlagsR, whf, z, isFg,
                static_cast<eRotFlags>(rotFlags), layer->planeAlpha,
                (ovutils::eBlending) getBlending(layer->blending));

        tmp_dstR.right = tmp_dstR.right - lSplit;
        tmp_dstR.left = tmp_dstR.left - lSplit;
        if(configMdp(ctx->mOverlay, pargR, orient,
                tmp_cropR, tmp_dstR, metadata, rDest) < 0) {
            ALOGE("%s: commit failed for right mixer config", __FUNCTION__);
            return -1;
        }
    }

    return 0;
}

bool canUseRotator(hwc_context_t *ctx, int dpy) {
    if(qdutils::MDPVersion::getInstance().is8x26() &&
            isSecondaryConnected(ctx)) {
        return false;
    }
    if(ctx->mMDP.version == qdutils::MDP_V3_0_4)
        return false;
    return true;
}

int getLeftSplit(hwc_context_t *ctx, const int& dpy) {
    //Default even split for all displays with high res
    int lSplit = ctx->dpyAttr[dpy].xres / 2;
    if(dpy == HWC_DISPLAY_PRIMARY &&
            qdutils::MDPVersion::getInstance().getLeftSplit()) {
        //Override if split published by driver for primary
        lSplit = qdutils::MDPVersion::getInstance().getLeftSplit();
    }
    return lSplit;
}

void setupSecondaryObjs(hwc_context_t *ctx, const int& dpy) {
    const int rSplit = 0;
    ctx->mFBUpdate[dpy] =
            IFBUpdate::getObject(ctx->dpyAttr[dpy].xres, rSplit, dpy);
    ctx->mMDPComp[dpy] =  MDPComp::getObject(
            ctx->dpyAttr[dpy].xres, rSplit, dpy);

    int compositionType =
            qdutils::QCCompositionType::getInstance().getCompositionType();
    if (compositionType & (qdutils::COMPOSITION_TYPE_DYN |
                           qdutils::COMPOSITION_TYPE_MDP |
                           qdutils::COMPOSITION_TYPE_C2D)) {
        ctx->mCopyBit[dpy] = new CopyBit();
    }

    if(ctx->mFBUpdate[dpy])
        ctx->mFBUpdate[dpy]->reset();
    if(ctx->mMDPComp[dpy])
        ctx->mMDPComp[dpy]->reset();
    if(ctx->mCopyBit[dpy])
        ctx->mCopyBit[dpy]->reset();
}

void clearSecondaryObjs(hwc_context_t *ctx, const int& dpy) {
    if(ctx->mFBUpdate[dpy]) {
        delete ctx->mFBUpdate[dpy];
        ctx->mFBUpdate[dpy] = NULL;
    }
    if(ctx->mCopyBit[dpy]){
        delete ctx->mCopyBit[dpy];
        ctx->mCopyBit[dpy] = NULL;
    }
    if(ctx->mMDPComp[dpy]) {
        delete ctx->mMDPComp[dpy];
        ctx->mMDPComp[dpy] = NULL;
    }
}

bool isGLESOnlyComp(hwc_context_t *ctx, const int& dpy) {
    if(ctx->mMDPComp[dpy]) {
        return (ctx->mMDPComp[dpy]->getMDPCompCount() == 0);
    }
    return true;
}

void BwcPM::setBwc(hwc_context_t *ctx, const hwc_rect_t& crop,
            const hwc_rect_t& dst, const int& transform,
            ovutils::eMdpFlags& mdpFlags) {
    //Target doesnt support Bwc
    if(!qdutils::MDPVersion::getInstance().supportsBWC()) {
        return;
    }
    int src_w = crop.right - crop.left;
    int src_h = crop.bottom - crop.top;
    int dst_w = dst.right - dst.left;
    int dst_h = dst.bottom - dst.top;
    if(transform & HAL_TRANSFORM_ROT_90) {
        swap(src_w, src_h);
    }
    //src width > MAX mixer supported dim
    if(src_w > qdutils::MAX_DISPLAY_DIM) {
        return;
    }
    //Secondary display connected
    if(isSecondaryConnected(ctx)) {
        return;
    }
    //Decimation necessary, cannot use BWC. H/W requirement.
    if(qdutils::MDPVersion::getInstance().supportsDecimation()) {
        uint8_t horzDeci = 0;
        uint8_t vertDeci = 0;
        ovutils::getDecimationFactor(src_w, src_h, dst_w, dst_h, horzDeci,
                vertDeci);
        if(horzDeci || vertDeci) return;
    }
    //Property
    char value[PROPERTY_VALUE_MAX];
    property_get("debug.disable.bwc", value, "0");
     if(atoi(value)) return;

    ovutils::setMdpFlags(mdpFlags, ovutils::OV_MDSS_MDP_BWC_EN);
}

void LayerRotMap::add(hwc_layer_1_t* layer, Rotator *rot) {
    if(mCount >= MAX_SESS) return;
    mLayer[mCount] = layer;
    mRot[mCount] = rot;
    mCount++;
}

void LayerRotMap::reset() {
    for (int i = 0; i < MAX_SESS; i++) {
        mLayer[i] = 0;
        mRot[i] = 0;
    }
    mCount = 0;
}

void LayerRotMap::setReleaseFd(const int& fence) {
    for(uint32_t i = 0; i < mCount; i++) {
        mRot[i]->setReleaseFd(dup(fence));
    }
}

};//namespace qhwc
