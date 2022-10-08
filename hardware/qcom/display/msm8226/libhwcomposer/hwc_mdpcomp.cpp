/*
 * Copyright (C) 2012-2014, The Linux Foundation. All rights reserved.
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

#include <math.h>
#include "hwc_mdpcomp.h"
#include <sys/ioctl.h>
#include "external.h"
#include "virtual.h"
#include "qdMetaData.h"
#include "mdp_version.h"
#include "hwc_fbupdate.h"
#include "hwc_ad.h"
#include <overlayRotator.h>
#include "hwc_copybit.h"

using namespace overlay;
using namespace qdutils;
using namespace overlay::utils;
namespace ovutils = overlay::utils;

namespace qhwc {

//==============MDPComp========================================================

IdleInvalidator *MDPComp::idleInvalidator = NULL;
bool MDPComp::sIdleFallBack = false;
bool MDPComp::sHandleTimeout = false;
bool MDPComp::sDebugLogs = false;
bool MDPComp::sEnabled = false;
bool MDPComp::sEnableMixedMode = true;
int MDPComp::sSimulationFlags = 0;
int MDPComp::sMaxPipesPerMixer = MAX_PIPES_PER_MIXER;
bool MDPComp::sEnable4k2kYUVSplit = false;
bool MDPComp::sSrcSplitEnabled = false;
MDPComp* MDPComp::getObject(hwc_context_t *ctx, const int& dpy) {
    if(qdutils::MDPVersion::getInstance().isSrcSplit()) {
        sSrcSplitEnabled = true;
        return new MDPCompSrcSplit(dpy);
    } else if(isDisplaySplit(ctx, dpy)) {
        return new MDPCompSplit(dpy);
    }
    return new MDPCompNonSplit(dpy);
}

MDPComp::MDPComp(int dpy):mDpy(dpy){};

void MDPComp::dump(android::String8& buf, hwc_context_t *ctx)
{
    if(mCurrentFrame.layerCount > MAX_NUM_APP_LAYERS)
        return;

    dumpsys_log(buf,"HWC Map for Dpy: %s \n",
                (mDpy == 0) ? "\"PRIMARY\"" :
                (mDpy == 1) ? "\"EXTERNAL\"" : "\"VIRTUAL\"");
    dumpsys_log(buf,"CURR_FRAME: layerCount:%2d mdpCount:%2d "
                "fbCount:%2d \n", mCurrentFrame.layerCount,
                mCurrentFrame.mdpCount, mCurrentFrame.fbCount);
    dumpsys_log(buf,"needsFBRedraw:%3s  pipesUsed:%2d  MaxPipesPerMixer: %d \n",
                (mCurrentFrame.needsRedraw? "YES" : "NO"),
                mCurrentFrame.mdpCount, sMaxPipesPerMixer);
    if(isDisplaySplit(ctx, mDpy)) {
        dumpsys_log(buf, "Programmed ROI's: Left: [%d, %d, %d, %d] "
                "Right: [%d, %d, %d, %d] \n",
                ctx->listStats[mDpy].lRoi.left, ctx->listStats[mDpy].lRoi.top,
                ctx->listStats[mDpy].lRoi.right,
                ctx->listStats[mDpy].lRoi.bottom,
                ctx->listStats[mDpy].rRoi.left,ctx->listStats[mDpy].rRoi.top,
                ctx->listStats[mDpy].rRoi.right,
                ctx->listStats[mDpy].rRoi.bottom);
    } else {
        dumpsys_log(buf, "Programmed ROI: [%d, %d, %d, %d] \n",
                ctx->listStats[mDpy].lRoi.left,ctx->listStats[mDpy].lRoi.top,
                ctx->listStats[mDpy].lRoi.right,
                ctx->listStats[mDpy].lRoi.bottom);
    }
    dumpsys_log(buf," ---------------------------------------------  \n");
    dumpsys_log(buf," listIdx | cached? | mdpIndex | comptype  |  Z  \n");
    dumpsys_log(buf," ---------------------------------------------  \n");
    for(int index = 0; index < mCurrentFrame.layerCount; index++ )
        dumpsys_log(buf," %7d | %7s | %8d | %9s | %2d \n",
                    index,
                    (mCurrentFrame.isFBComposed[index] ? "YES" : "NO"),
                     mCurrentFrame.layerToMDP[index],
                    (mCurrentFrame.isFBComposed[index] ?
                    (mCurrentFrame.drop[index] ? "DROP" :
                    (mCurrentFrame.needsRedraw ? "GLES" : "CACHE")) : "MDP"),
                    (mCurrentFrame.isFBComposed[index] ? mCurrentFrame.fbZ :
    mCurrentFrame.mdpToLayer[mCurrentFrame.layerToMDP[index]].pipeInfo->zOrder));
    dumpsys_log(buf,"\n");
}

bool MDPComp::init(hwc_context_t *ctx) {

    if(!ctx) {
        ALOGE("%s: Invalid hwc context!!",__FUNCTION__);
        return false;
    }

    char property[PROPERTY_VALUE_MAX];

    sEnabled = false;
    if((property_get("persist.hwc.mdpcomp.enable", property, NULL) > 0) &&
       (!strncmp(property, "1", PROPERTY_VALUE_MAX ) ||
        (!strncasecmp(property,"true", PROPERTY_VALUE_MAX )))) {
        sEnabled = true;
    }

    sEnableMixedMode = true;
    if((property_get("debug.mdpcomp.mixedmode.disable", property, NULL) > 0) &&
       (!strncmp(property, "1", PROPERTY_VALUE_MAX ) ||
        (!strncasecmp(property,"true", PROPERTY_VALUE_MAX )))) {
        sEnableMixedMode = false;
    }

    sMaxPipesPerMixer = MAX_PIPES_PER_MIXER;
    if(property_get("debug.mdpcomp.maxpermixer", property, "-1") > 0) {
        int val = atoi(property);
        if(val >= 0)
            sMaxPipesPerMixer = min(val, MAX_PIPES_PER_MIXER);
    }

    if(ctx->mMDP.panel != MIPI_CMD_PANEL) {
        // Idle invalidation is not necessary on command mode panels
        long idle_timeout = DEFAULT_IDLE_TIME;
        if(property_get("debug.mdpcomp.idletime", property, NULL) > 0) {
            if(atoi(property) != 0)
                idle_timeout = atoi(property);
        }

        //create Idle Invalidator only when not disabled through property
        if(idle_timeout != -1)
            idleInvalidator = IdleInvalidator::getInstance();

        if(idleInvalidator == NULL) {
            ALOGE("%s: failed to instantiate idleInvalidator object",
                  __FUNCTION__);
        } else {
            idleInvalidator->init(timeout_handler, ctx,
                                  (unsigned int)idle_timeout);
        }
    }

    if(!qdutils::MDPVersion::getInstance().isSrcSplit() &&
            property_get("persist.mdpcomp.4k2kSplit", property, "0") > 0 &&
            (!strncmp(property, "1", PROPERTY_VALUE_MAX) ||
            !strncasecmp(property,"true", PROPERTY_VALUE_MAX))) {
        sEnable4k2kYUVSplit = true;
    }

    if ((property_get("persist.hwc.ptor.enable", property, NULL) > 0) &&
            ((!strncasecmp(property, "true", PROPERTY_VALUE_MAX )) ||
             (!strncmp(property, "1", PROPERTY_VALUE_MAX )))) {
        ctx->mCopyBit[HWC_DISPLAY_PRIMARY] = new CopyBit(ctx,
                                                    HWC_DISPLAY_PRIMARY);
    }

    return true;
}

void MDPComp::reset(hwc_context_t *ctx) {
    const int numLayers = ctx->listStats[mDpy].numAppLayers;
    mCurrentFrame.reset(numLayers);
    ctx->mOverlay->clear(mDpy);
    ctx->mLayerRotMap[mDpy]->clear();
}

void MDPComp::reset() {
    sHandleTimeout = false;
    mModeOn = false;
}

void MDPComp::timeout_handler(void *udata) {
    struct hwc_context_t* ctx = (struct hwc_context_t*)(udata);

    if(!ctx) {
        ALOGE("%s: received empty data in timer callback", __FUNCTION__);
        return;
    }
    Locker::Autolock _l(ctx->mDrawLock);
    // Handle timeout event only if the previous composition is MDP or MIXED.
    if(!sHandleTimeout) {
        ALOGD_IF(isDebug(), "%s:Do not handle this timeout", __FUNCTION__);
        return;
    }
    if(!ctx->proc) {
        ALOGE("%s: HWC proc not registered", __FUNCTION__);
        return;
    }
    sIdleFallBack = true;
    /* Trigger SF to redraw the current frame */
    ctx->proc->invalidate(ctx->proc);
}

void MDPComp::setMDPCompLayerFlags(hwc_context_t *ctx,
                                   hwc_display_contents_1_t* list) {
    LayerProp *layerProp = ctx->layerProp[mDpy];

    for(int index = 0; index < ctx->listStats[mDpy].numAppLayers; index++) {
        hwc_layer_1_t* layer = &(list->hwLayers[index]);
        if(!mCurrentFrame.isFBComposed[index]) {
            layerProp[index].mFlags |= HWC_MDPCOMP;
            layer->compositionType = HWC_OVERLAY;
            layer->hints |= HWC_HINT_CLEAR_FB;
        } else {
            /* Drop the layer when its already present in FB OR when it lies
             * outside frame's ROI */
            if(!mCurrentFrame.needsRedraw || mCurrentFrame.drop[index]) {
                layer->compositionType = HWC_OVERLAY;
            }
        }
    }
}

void MDPComp::setRedraw(hwc_context_t *ctx,
        hwc_display_contents_1_t* list) {
    mCurrentFrame.needsRedraw = false;
    if(!mCachedFrame.isSameFrame(mCurrentFrame, list) ||
            (list->flags & HWC_GEOMETRY_CHANGED) ||
            isSkipPresent(ctx, mDpy)) {
        mCurrentFrame.needsRedraw = true;
    }
}

MDPComp::FrameInfo::FrameInfo() {
    memset(&mdpToLayer, 0, sizeof(mdpToLayer));
    reset(0);
}

void MDPComp::FrameInfo::reset(const int& numLayers) {
    for(int i = 0 ; i < MAX_PIPES_PER_MIXER; i++ ) {
        if(mdpToLayer[i].pipeInfo) {
            delete mdpToLayer[i].pipeInfo;
            mdpToLayer[i].pipeInfo = NULL;
            //We dont own the rotator
            mdpToLayer[i].rot = NULL;
        }
    }

    memset(&mdpToLayer, 0, sizeof(mdpToLayer));
    memset(&layerToMDP, -1, sizeof(layerToMDP));
    memset(&isFBComposed, 1, sizeof(isFBComposed));

    layerCount = numLayers;
    fbCount = numLayers;
    mdpCount = 0;
    needsRedraw = true;
    fbZ = -1;
}

void MDPComp::FrameInfo::map() {
    // populate layer and MDP maps
    int mdpIdx = 0;
    for(int idx = 0; idx < layerCount; idx++) {
        if(!isFBComposed[idx]) {
            mdpToLayer[mdpIdx].listIndex = idx;
            layerToMDP[idx] = mdpIdx++;
        }
    }
}

MDPComp::LayerCache::LayerCache() {
    reset();
}

void MDPComp::LayerCache::reset() {
    memset(&hnd, 0, sizeof(hnd));
    memset(&isFBComposed, true, sizeof(isFBComposed));
    memset(&drop, false, sizeof(drop));
    layerCount = 0;
}

void MDPComp::LayerCache::cacheAll(hwc_display_contents_1_t* list) {
    const int numAppLayers = (int)list->numHwLayers - 1;
    for(int i = 0; i < numAppLayers; i++) {
        hnd[i] = list->hwLayers[i].handle;
    }
}

void MDPComp::LayerCache::updateCounts(const FrameInfo& curFrame) {
    layerCount = curFrame.layerCount;
    memcpy(&isFBComposed, &curFrame.isFBComposed, sizeof(isFBComposed));
    memcpy(&drop, &curFrame.drop, sizeof(drop));
}

bool MDPComp::LayerCache::isSameFrame(const FrameInfo& curFrame,
                                      hwc_display_contents_1_t* list) {
    if(layerCount != curFrame.layerCount)
        return false;
    for(int i = 0; i < curFrame.layerCount; i++) {
        if((curFrame.isFBComposed[i] != isFBComposed[i]) ||
                (curFrame.drop[i] != drop[i])) {
            return false;
        }
        if(curFrame.isFBComposed[i] &&
           (hnd[i] != list->hwLayers[i].handle)){
            return false;
        }
    }
    return true;
}

bool MDPComp::isSupportedForMDPComp(hwc_context_t *ctx, hwc_layer_1_t* layer) {
    private_handle_t *hnd = (private_handle_t *)layer->handle;
    if((has90Transform(layer) and (not isRotationDoable(ctx, hnd))) ||
        (not isValidDimension(ctx,layer))
        //More conditions here, SKIP, sRGB+Blend etc
        ) {
        return false;
    }
    return true;
}

bool MDPComp::isValidDimension(hwc_context_t *ctx, hwc_layer_1_t *layer) {
    private_handle_t *hnd = (private_handle_t *)layer->handle;

    if(!hnd) {
        if (layer->flags & HWC_COLOR_FILL) {
            // Color layer
            return true;
        }
        ALOGE("%s: layer handle is NULL", __FUNCTION__);
        return false;
    }

    //XXX: Investigate doing this with pixel phase on MDSS
    if(!isSecureBuffer(hnd) && isNonIntegralSourceCrop(layer->sourceCropf))
        return false;

    hwc_rect_t crop = integerizeSourceCrop(layer->sourceCropf);
    hwc_rect_t dst = layer->displayFrame;
    bool rotated90 = (bool)(layer->transform & HAL_TRANSFORM_ROT_90);
    int crop_w = rotated90 ? crop.bottom - crop.top : crop.right - crop.left;
    int crop_h = rotated90 ? crop.right - crop.left : crop.bottom - crop.top;
    int dst_w = dst.right - dst.left;
    int dst_h = dst.bottom - dst.top;
    float w_scale = ((float)crop_w / (float)dst_w);
    float h_scale = ((float)crop_h / (float)dst_h);

    /* Workaround for MDP HW limitation in DSI command mode panels where
     * FPS will not go beyond 30 if buffers on RGB pipes are of width or height
     * less than 5 pixels
     * There also is a HW limilation in MDP, minimum block size is 2x2
     * Fallback to GPU if height is less than 2.
     */
    if((crop_w < 5)||(crop_h < 5))
        return false;

    if((w_scale > 1.0f) || (h_scale > 1.0f)) {
        const uint32_t maxMDPDownscale =
            qdutils::MDPVersion::getInstance().getMaxMDPDownscale();
        const float w_dscale = w_scale;
        const float h_dscale = h_scale;

        if(ctx->mMDP.version >= qdutils::MDSS_V5) {

            if(!qdutils::MDPVersion::getInstance().supportsDecimation()) {
                /* On targets that doesnt support Decimation (eg.,8x26)
                 * maximum downscale support is overlay pipe downscale.
                 */
                if(crop_w > MAX_DISPLAY_DIM || w_dscale > maxMDPDownscale ||
                        h_dscale > maxMDPDownscale)
                    return false;
            } else {
                // Decimation on macrotile format layers is not supported.
                if(isTileRendered(hnd)) {
                    /* MDP can read maximum MAX_DISPLAY_DIM width.
                     * Bail out if
                     *      1. Src crop > MAX_DISPLAY_DIM on nonsplit MDPComp
                     *      2. exceeds maximum downscale limit
                     */
                    if(((crop_w > MAX_DISPLAY_DIM) && !sSrcSplitEnabled) ||
                            w_dscale > maxMDPDownscale ||
                            h_dscale > maxMDPDownscale) {
                        return false;
                    }
                } else if(w_dscale > 64 || h_dscale > 64)
                    return false;
            }
        } else { //A-family
            if(w_dscale > maxMDPDownscale || h_dscale > maxMDPDownscale)
                return false;
        }
    }

    if((w_scale < 1.0f) || (h_scale < 1.0f)) {
        const uint32_t upscale =
            qdutils::MDPVersion::getInstance().getMaxMDPUpscale();
        const float w_uscale = 1.0f / w_scale;
        const float h_uscale = 1.0f / h_scale;

        if(w_uscale > upscale || h_uscale > upscale)
            return false;
    }

    return true;
}

bool MDPComp::isFrameDoable(hwc_context_t *ctx) {
    bool ret = true;

    if(!isEnabled()) {
        ALOGD_IF(isDebug(),"%s: MDP Comp. not enabled.", __FUNCTION__);
        ret = false;
    } else if((qdutils::MDPVersion::getInstance().is8x26() ||
               qdutils::MDPVersion::getInstance().is8x16() ||
               qdutils::MDPVersion::getInstance().is8x39()) &&
            ctx->mVideoTransFlag &&
            isSecondaryConnected(ctx)) {
        //1 Padding round to shift pipes across mixers
        ALOGD_IF(isDebug(),"%s: MDP Comp. video transition padding round",
                __FUNCTION__);
        ret = false;
    } else if(isSecondaryConfiguring(ctx)) {
        ALOGD_IF( isDebug(),"%s: External Display connection is pending",
                  __FUNCTION__);
        ret = false;
    } else if(ctx->isPaddingRound) {
        ALOGD_IF(isDebug(), "%s: padding round invoked for dpy %d",
                 __FUNCTION__,mDpy);
        ret = false;
    }
    return ret;
}

void MDPCompNonSplit::trimAgainstROI(hwc_context_t *ctx, hwc_rect_t& fbRect) {
    hwc_rect_t roi = ctx->listStats[mDpy].lRoi;
    fbRect = getIntersection(fbRect, roi);
}

/* 1) Identify layers that are not visible or lying outside the updating ROI and
 *    drop them from composition.
 * 2) If we have a scaling layer which needs cropping against generated
 *    ROI, reset ROI to full resolution. */
bool MDPCompNonSplit::validateAndApplyROI(hwc_context_t *ctx,
        hwc_display_contents_1_t* list) {
    int numAppLayers = ctx->listStats[mDpy].numAppLayers;
    hwc_rect_t visibleRect = ctx->listStats[mDpy].lRoi;

    for(int i = numAppLayers - 1; i >= 0; i--){
        if(!isValidRect(visibleRect)) {
            mCurrentFrame.drop[i] = true;
            mCurrentFrame.dropCount++;
            continue;
        }

        const hwc_layer_1_t* layer =  &list->hwLayers[i];
        hwc_rect_t dstRect = layer->displayFrame;
        hwc_rect_t res  = getIntersection(visibleRect, dstRect);

        if(!isValidRect(res)) {
            mCurrentFrame.drop[i] = true;
            mCurrentFrame.dropCount++;
        } else {
            /* Reset frame ROI when any layer which needs scaling also needs ROI
             * cropping */
            if(!isSameRect(res, dstRect) && needsScaling (layer)) {
                ALOGI("%s: Resetting ROI due to scaling", __FUNCTION__);
                memset(&mCurrentFrame.drop, 0, sizeof(mCurrentFrame.drop));
                mCurrentFrame.dropCount = 0;
                return false;
            }

            /* deduct any opaque region from visibleRect */
            if (layer->blending == HWC_BLENDING_NONE)
                visibleRect = deductRect(visibleRect, res);
        }
    }
    return true;
}

/* Calculate ROI for the frame by accounting all the layer's dispalyFrame which
 * are updating. If DirtyRegion is applicable, calculate it by accounting all
 * the changing layer's dirtyRegion. */
void MDPCompNonSplit::generateROI(hwc_context_t *ctx,
        hwc_display_contents_1_t* list) {
    int numAppLayers = ctx->listStats[mDpy].numAppLayers;
    if(!canPartialUpdate(ctx, list))
        return;

    struct hwc_rect roi = (struct hwc_rect){0, 0, 0, 0};
    hwc_rect fullFrame = (struct hwc_rect) {0, 0,(int)ctx->dpyAttr[mDpy].xres,
        (int)ctx->dpyAttr[mDpy].yres};

    for(int index = 0; index < numAppLayers; index++ ) {
        hwc_layer_1_t* layer = &list->hwLayers[index];
        if ((mCachedFrame.hnd[index] != layer->handle) ||
                isYuvBuffer((private_handle_t *)layer->handle)) {
            hwc_rect_t dst = layer->displayFrame;
            hwc_rect_t updatingRect = dst;

#ifdef QCOM_BSP
            if(!needsScaling(layer) && !layer->transform)
            {
                hwc_rect_t src = integerizeSourceCrop(layer->sourceCropf);
                int x_off = dst.left - src.left;
                int y_off = dst.top - src.top;
                updatingRect = moveRect(layer->dirtyRect, x_off, y_off);
            }
#endif

            roi = getUnion(roi, updatingRect);
        }
    }

    /* No layer is updating. Still SF wants a refresh.*/
    if(!isValidRect(roi))
        return;

    // Align ROI coordinates to panel restrictions
    roi = getSanitizeROI(roi, fullFrame);

    ctx->listStats[mDpy].lRoi = roi;
    if(!validateAndApplyROI(ctx, list))
        resetROI(ctx, mDpy);

    ALOGD_IF(isDebug(),"%s: generated ROI: [%d, %d, %d, %d]", __FUNCTION__,
            ctx->listStats[mDpy].lRoi.left, ctx->listStats[mDpy].lRoi.top,
            ctx->listStats[mDpy].lRoi.right, ctx->listStats[mDpy].lRoi.bottom);
}

void MDPCompSplit::trimAgainstROI(hwc_context_t *ctx, hwc_rect_t& fbRect) {
    hwc_rect l_roi = ctx->listStats[mDpy].lRoi;
    hwc_rect r_roi = ctx->listStats[mDpy].rRoi;

    hwc_rect_t l_fbRect = getIntersection(fbRect, l_roi);
    hwc_rect_t r_fbRect = getIntersection(fbRect, r_roi);
    fbRect = getUnion(l_fbRect, r_fbRect);
}
/* 1) Identify layers that are not visible or lying outside BOTH the updating
 *    ROI's and drop them from composition. If a layer is spanning across both
 *    the halves of the screen but needed by only ROI, the non-contributing
 *    half will not be programmed for MDP.
 * 2) If we have a scaling layer which needs cropping against generated
 *    ROI, reset ROI to full resolution. */
bool MDPCompSplit::validateAndApplyROI(hwc_context_t *ctx,
        hwc_display_contents_1_t* list) {

    int numAppLayers = ctx->listStats[mDpy].numAppLayers;

    hwc_rect_t visibleRectL = ctx->listStats[mDpy].lRoi;
    hwc_rect_t visibleRectR = ctx->listStats[mDpy].rRoi;

    for(int i = numAppLayers - 1; i >= 0; i--){
        if(!isValidRect(visibleRectL) && !isValidRect(visibleRectR))
        {
            mCurrentFrame.drop[i] = true;
            mCurrentFrame.dropCount++;
            continue;
        }

        const hwc_layer_1_t* layer =  &list->hwLayers[i];
        hwc_rect_t dstRect = layer->displayFrame;

        hwc_rect_t l_res  = getIntersection(visibleRectL, dstRect);
        hwc_rect_t r_res  = getIntersection(visibleRectR, dstRect);
        hwc_rect_t res = getUnion(l_res, r_res);

        if(!isValidRect(l_res) && !isValidRect(r_res)) {
            mCurrentFrame.drop[i] = true;
            mCurrentFrame.dropCount++;
        } else {
            /* Reset frame ROI when any layer which needs scaling also needs ROI
             * cropping */
            if(!isSameRect(res, dstRect) && needsScaling (layer)) {
                memset(&mCurrentFrame.drop, 0, sizeof(mCurrentFrame.drop));
                mCurrentFrame.dropCount = 0;
                return false;
            }

            if (layer->blending == HWC_BLENDING_NONE) {
                visibleRectL = deductRect(visibleRectL, l_res);
                visibleRectR = deductRect(visibleRectR, r_res);
            }
        }
    }
    return true;
}
/* Calculate ROI for the frame by accounting all the layer's dispalyFrame which
 * are updating. If DirtyRegion is applicable, calculate it by accounting all
 * the changing layer's dirtyRegion. */
void MDPCompSplit::generateROI(hwc_context_t *ctx,
        hwc_display_contents_1_t* list) {
    if(!canPartialUpdate(ctx, list))
        return;

    int numAppLayers = ctx->listStats[mDpy].numAppLayers;
    int lSplit = getLeftSplit(ctx, mDpy);

    int hw_h = (int)ctx->dpyAttr[mDpy].yres;
    int hw_w = (int)ctx->dpyAttr[mDpy].xres;

    struct hwc_rect l_frame = (struct hwc_rect){0, 0, lSplit, hw_h};
    struct hwc_rect r_frame = (struct hwc_rect){lSplit, 0, hw_w, hw_h};

    struct hwc_rect l_roi = (struct hwc_rect){0, 0, 0, 0};
    struct hwc_rect r_roi = (struct hwc_rect){0, 0, 0, 0};

    for(int index = 0; index < numAppLayers; index++ ) {
        hwc_layer_1_t* layer = &list->hwLayers[index];
        private_handle_t *hnd = (private_handle_t *)layer->handle;
        if ((mCachedFrame.hnd[index] != layer->handle) ||
                isYuvBuffer(hnd)) {
            hwc_rect_t dst = layer->displayFrame;
            hwc_rect_t updatingRect = dst;

#ifdef QCOM_BSP
            if(!needsScaling(layer) && !layer->transform)
            {
                hwc_rect_t src = integerizeSourceCrop(layer->sourceCropf);
                int x_off = dst.left - src.left;
                int y_off = dst.top - src.top;
                updatingRect = moveRect(layer->dirtyRect, x_off, y_off);
            }
#endif

            hwc_rect_t l_dst  = getIntersection(l_frame, updatingRect);
            if(isValidRect(l_dst))
                l_roi = getUnion(l_roi, l_dst);

            hwc_rect_t r_dst  = getIntersection(r_frame, updatingRect);
            if(isValidRect(r_dst))
                r_roi = getUnion(r_roi, r_dst);
        }
    }

    /* For panels that cannot accept commands in both the interfaces, we cannot
     * send two ROI's (for each half). We merge them into single ROI and split
     * them across lSplit for MDP mixer use. The ROI's will be merged again
     * finally before udpating the panel in the driver. */
    if(qdutils::MDPVersion::getInstance().needsROIMerge()) {
        hwc_rect_t temp_roi = getUnion(l_roi, r_roi);
        l_roi = getIntersection(temp_roi, l_frame);
        r_roi = getIntersection(temp_roi, r_frame);
    }

    /* No layer is updating. Still SF wants a refresh. */
    if(!isValidRect(l_roi) && !isValidRect(r_roi))
        return;

    l_roi = getSanitizeROI(l_roi, l_frame);
    r_roi = getSanitizeROI(r_roi, r_frame);

    ctx->listStats[mDpy].lRoi = l_roi;
    ctx->listStats[mDpy].rRoi = r_roi;

    if(!validateAndApplyROI(ctx, list))
        resetROI(ctx, mDpy);

    ALOGD_IF(isDebug(),"%s: generated L_ROI: [%d, %d, %d, %d]"
            "R_ROI: [%d, %d, %d, %d]", __FUNCTION__,
            ctx->listStats[mDpy].lRoi.left, ctx->listStats[mDpy].lRoi.top,
            ctx->listStats[mDpy].lRoi.right, ctx->listStats[mDpy].lRoi.bottom,
            ctx->listStats[mDpy].rRoi.left, ctx->listStats[mDpy].rRoi.top,
            ctx->listStats[mDpy].rRoi.right, ctx->listStats[mDpy].rRoi.bottom);
}

/* Checks for conditions where all the layers marked for MDP comp cannot be
 * bypassed. On such conditions we try to bypass atleast YUV layers */
bool MDPComp::tryFullFrame(hwc_context_t *ctx,
                                hwc_display_contents_1_t* list){

    const int numAppLayers = ctx->listStats[mDpy].numAppLayers;
    int priDispW = ctx->dpyAttr[HWC_DISPLAY_PRIMARY].xres;

    if(sIdleFallBack && !ctx->listStats[mDpy].secureUI) {
        ALOGD_IF(isDebug(), "%s: Idle fallback dpy %d",__FUNCTION__, mDpy);
        return false;
    }

    if(isSkipPresent(ctx, mDpy)) {
        ALOGD_IF(isDebug(),"%s: SKIP present: %d",
                __FUNCTION__,
                isSkipPresent(ctx, mDpy));
        return false;
    }

    if(mDpy > HWC_DISPLAY_PRIMARY && (priDispW > MAX_DISPLAY_DIM) &&
                              (ctx->dpyAttr[mDpy].xres < MAX_DISPLAY_DIM)) {
        // Disable MDP comp on Secondary when the primary is highres panel and
        // the secondary is a normal 1080p, because, MDP comp on secondary under
        // in such usecase, decimation gets used for downscale and there will be
        // a quality mismatch when there will be a fallback to GPU comp
        ALOGD_IF(isDebug(), "%s: Disable MDP Compositon for Secondary Disp",
              __FUNCTION__);
        return false;
    }

    // check for action safe flag and downscale mode which requires scaling.
    if(ctx->dpyAttr[mDpy].mActionSafePresent
            || ctx->dpyAttr[mDpy].mDownScaleMode) {
        ALOGD_IF(isDebug(), "%s: Scaling needed for this frame",__FUNCTION__);
        return false;
    }

    for(int i = 0; i < numAppLayers; ++i) {
        hwc_layer_1_t* layer = &list->hwLayers[i];
        private_handle_t *hnd = (private_handle_t *)layer->handle;

        if(has90Transform(layer) && isRotationDoable(ctx, hnd)) {
            if(!canUseRotator(ctx, mDpy)) {
                ALOGD_IF(isDebug(), "%s: Can't use rotator for dpy %d",
                        __FUNCTION__, mDpy);
                return false;
            }
        }

        //For 8x26 with panel width>1k, if RGB layer needs HFLIP fail mdp comp
        // may not need it if Gfx pre-rotation can handle all flips & rotations
        if(qdutils::MDPVersion::getInstance().is8x26() &&
                                (ctx->dpyAttr[mDpy].xres > 1024) &&
                                (layer->transform & HWC_TRANSFORM_FLIP_H) &&
                                (!isYuvBuffer(hnd)))
                   return false;
    }

    if(ctx->mAD->isDoable()) {
        return false;
    }

    //If all above hard conditions are met we can do full or partial MDP comp.
    bool ret = false;
    if(fullMDPComp(ctx, list)) {
        ret = true;
    } else if(fullMDPCompWithPTOR(ctx, list)) {
        ret = true;
    } else if(partialMDPComp(ctx, list)) {
        ret = true;
    }

    return ret;
}

bool MDPComp::fullMDPComp(hwc_context_t *ctx, hwc_display_contents_1_t* list) {

    if(sSimulationFlags & MDPCOMP_AVOID_FULL_MDP)
        return false;

    //Will benefit presentation / secondary-only layer.
    if((mDpy > HWC_DISPLAY_PRIMARY) &&
            (list->numHwLayers - 1) > MAX_SEC_LAYERS) {
        ALOGD_IF(isDebug(), "%s: Exceeds max secondary pipes",__FUNCTION__);
        return false;
    }

    const int numAppLayers = ctx->listStats[mDpy].numAppLayers;
    for(int i = 0; i < numAppLayers; i++) {
        hwc_layer_1_t* layer = &list->hwLayers[i];
        if(not mCurrentFrame.drop[i] and
           not isSupportedForMDPComp(ctx, layer)) {
            ALOGD_IF(isDebug(), "%s: Unsupported layer in list",__FUNCTION__);
            return false;
        }
    }

    mCurrentFrame.fbCount = 0;
    memcpy(&mCurrentFrame.isFBComposed, &mCurrentFrame.drop,
           sizeof(mCurrentFrame.isFBComposed));
    mCurrentFrame.mdpCount = mCurrentFrame.layerCount - mCurrentFrame.fbCount -
        mCurrentFrame.dropCount;

    if(sEnable4k2kYUVSplit){
        adjustForSourceSplit(ctx, list);
    }

    if(!postHeuristicsHandling(ctx, list)) {
        ALOGD_IF(isDebug(), "post heuristic handling failed");
        reset(ctx);
        return false;
    }
    ALOGD_IF(sSimulationFlags,"%s: FULL_MDP_COMP SUCCEEDED",
             __FUNCTION__);
    return true;
}

/* Full MDP Composition with Peripheral Tiny Overlap Removal.
 * MDP bandwidth limitations can be avoided, if the overlap region
 * covered by the smallest layer at a higher z-order, gets composed
 * by Copybit on a render buffer, which can be queued to MDP.
 */
bool MDPComp::fullMDPCompWithPTOR(hwc_context_t *ctx,
    hwc_display_contents_1_t* list) {

    const int numAppLayers = ctx->listStats[mDpy].numAppLayers;
    const int stagesForMDP = min(sMaxPipesPerMixer,
            ctx->mOverlay->availablePipes(mDpy, Overlay::MIXER_DEFAULT));

    // Hard checks where we cannot use this mode
    if (mDpy || !ctx->mCopyBit[mDpy] || isDisplaySplit(ctx, mDpy)) {
        ALOGD_IF(isDebug(), "%s: Feature not supported!", __FUNCTION__);
        return false;
    }

    // Frame level checks
    if ((numAppLayers > stagesForMDP) || isSkipPresent(ctx, mDpy) ||
        isYuvPresent(ctx, mDpy) || mCurrentFrame.dropCount ||
        isSecurePresent(ctx, mDpy)) {
        ALOGD_IF(isDebug(), "%s: Frame not supported!", __FUNCTION__);
        return false;
    }
    // MDP comp checks
    for(int i = 0; i < numAppLayers; i++) {
        hwc_layer_1_t* layer = &list->hwLayers[i];
        if(not isSupportedForMDPComp(ctx, layer)) {
            ALOGD_IF(isDebug(), "%s: Unsupported layer in list",__FUNCTION__);
            return false;
        }
    }

    /* We cannot use this composition mode, if:
     1. A below layer needs scaling.
     2. Overlap is not peripheral to display.
     3. Overlap or a below layer has 90 degree transform.
     4. Overlap area > (1/3 * FrameBuffer) area, based on Perf inputs.
     */

    int minLayerIndex[MAX_PTOR_LAYERS] = { -1, -1};
    hwc_rect_t overlapRect[MAX_PTOR_LAYERS];
    memset(overlapRect, 0, sizeof(overlapRect));
    int layerPixelCount, minPixelCount = 0;
    int numPTORLayersFound = 0;
    for (int i = numAppLayers-1; (i >= 0 &&
                                  numPTORLayersFound < MAX_PTOR_LAYERS); i--) {
        hwc_layer_1_t* layer = &list->hwLayers[i];
        hwc_rect_t crop = integerizeSourceCrop(layer->sourceCropf);
        hwc_rect_t dispFrame = layer->displayFrame;
        layerPixelCount = (crop.right - crop.left) * (crop.bottom - crop.top);
        // PTOR layer should be peripheral and cannot have transform
        if (!isPeripheral(dispFrame, ctx->mViewFrame[mDpy]) ||
                                has90Transform(layer)) {
            continue;
        }
        if((3 * (layerPixelCount + minPixelCount)) >
                ((int)ctx->dpyAttr[mDpy].xres * (int)ctx->dpyAttr[mDpy].yres)) {
            // Overlap area > (1/3 * FrameBuffer) area, based on Perf inputs.
            continue;
        }
        // Found the PTOR layer
        bool found = true;
        for (int j = i-1; j >= 0; j--) {
            // Check if the layers below this layer qualifies for PTOR comp
            hwc_layer_1_t* layer = &list->hwLayers[j];
            hwc_rect_t disFrame = layer->displayFrame;
            //layer below PTOR is intersecting and has 90 degree transform or
            // needs scaling cannot be supported.
            if ((isValidRect(getIntersection(dispFrame, disFrame)))
                            && (has90Transform(layer) || needsScaling(layer))) {
                found = false;
                break;
            }
        }
        // Store the minLayer Index
        if(found) {
            minLayerIndex[numPTORLayersFound] = i;
            overlapRect[numPTORLayersFound] = list->hwLayers[i].displayFrame;
            minPixelCount += layerPixelCount;
            numPTORLayersFound++;
        }
    }

    if(isValidRect(getIntersection(overlapRect[0], overlapRect[1]))) {
        ALOGD_IF(isDebug(), "%s: Ignore Rect2 its intersects with Rect1",
                 __FUNCTION__);
        // reset second minLayerIndex[1];
        minLayerIndex[1] = -1;
        numPTORLayersFound--;
    }

    // No overlap layers
    if (!numPTORLayersFound)
        return false;

    ctx->mPtorInfo.count = numPTORLayersFound;
    for(int i = 0; i < MAX_PTOR_LAYERS; i++) {
        ctx->mPtorInfo.layerIndex[i] = minLayerIndex[i];
    }

    if (!ctx->mCopyBit[mDpy]->prepareOverlap(ctx, list)) {
        // reset PTOR
        ctx->mPtorInfo.count = 0;
        return false;
    }
    // Store the displayFrame and the sourceCrops of the layers
    hwc_rect_t displayFrame[numAppLayers];
    hwc_rect_t sourceCrop[numAppLayers];
    for(int i = 0; i < numAppLayers; i++) {
        hwc_layer_1_t* layer = &list->hwLayers[i];
        displayFrame[i] = layer->displayFrame;
        sourceCrop[i] = integerizeSourceCrop(layer->sourceCropf);
    }

    for(int j = 0; j < numPTORLayersFound; j++) {
        int index =  ctx->mPtorInfo.layerIndex[j];
        // Remove overlap from crop & displayFrame of below layers
        for (int i = 0; i < index && index !=-1; i++) {
            hwc_layer_1_t* layer = &list->hwLayers[i];
            if(!isValidRect(getIntersection(layer->displayFrame,
                                            overlapRect[j])))  {
                continue;
            }
            // Update layer attributes
            hwc_rect_t srcCrop = integerizeSourceCrop(layer->sourceCropf);
            hwc_rect_t destRect = deductRect(layer->displayFrame,
                                             overlapRect[j]);
            qhwc::calculate_crop_rects(srcCrop, layer->displayFrame, destRect,
                                       layer->transform);
            layer->sourceCropf.left = (float)srcCrop.left;
            layer->sourceCropf.top = (float)srcCrop.top;
            layer->sourceCropf.right = (float)srcCrop.right;
            layer->sourceCropf.bottom = (float)srcCrop.bottom;
        }
    }

    mCurrentFrame.mdpCount = numAppLayers;
    mCurrentFrame.fbCount = 0;
    mCurrentFrame.fbZ = -1;

    for (int j = 0; j < numAppLayers; j++)
        mCurrentFrame.isFBComposed[j] = false;

    bool result = postHeuristicsHandling(ctx, list);

    // Restore layer attributes
    for(int i = 0; i < numAppLayers; i++) {
        hwc_layer_1_t* layer = &list->hwLayers[i];
        layer->displayFrame = displayFrame[i];
        layer->sourceCropf.left = (float)sourceCrop[i].left;
        layer->sourceCropf.top = (float)sourceCrop[i].top;
        layer->sourceCropf.right = (float)sourceCrop[i].right;
        layer->sourceCropf.bottom = (float)sourceCrop[i].bottom;
    }

    if (!result) {
        // reset PTOR
        ctx->mPtorInfo.count = 0;
        reset(ctx);
    } else {
        ALOGD_IF(isDebug(), "%s: PTOR Indexes: %d and %d", __FUNCTION__,
                 ctx->mPtorInfo.layerIndex[0],  ctx->mPtorInfo.layerIndex[1]);
    }

    ALOGD_IF(isDebug(), "%s: Postheuristics %s!", __FUNCTION__,
             (result ? "successful" : "failed"));
    return result;
}

bool MDPComp::partialMDPComp(hwc_context_t *ctx, hwc_display_contents_1_t* list)
{
    if(!sEnableMixedMode) {
        //Mixed mode is disabled. No need to even try caching.
        return false;
    }

    bool ret = false;
    if(list->flags & HWC_GEOMETRY_CHANGED) { //Try load based first
        ret =   loadBasedComp(ctx, list) or
                cacheBasedComp(ctx, list);
    } else {
        ret =   cacheBasedComp(ctx, list) or
                loadBasedComp(ctx, list);
    }

    return ret;
}

bool MDPComp::cacheBasedComp(hwc_context_t *ctx,
        hwc_display_contents_1_t* list) {
    if(sSimulationFlags & MDPCOMP_AVOID_CACHE_MDP)
        return false;

    int numAppLayers = ctx->listStats[mDpy].numAppLayers;
    mCurrentFrame.reset(numAppLayers);
    updateLayerCache(ctx, list);

    //If an MDP marked layer is unsupported cannot do partial MDP Comp
    for(int i = 0; i < numAppLayers; i++) {
        if(!mCurrentFrame.isFBComposed[i]) {
            hwc_layer_1_t* layer = &list->hwLayers[i];
            if(not isSupportedForMDPComp(ctx, layer)) {
                ALOGD_IF(isDebug(), "%s: Unsupported layer in list",
                        __FUNCTION__);
                reset(ctx);
                return false;
            }
        }
    }

    updateYUV(ctx, list, false /*secure only*/);
    bool ret = markLayersForCaching(ctx, list); //sets up fbZ also
    if(!ret) {
        ALOGD_IF(isDebug(),"%s: batching failed, dpy %d",__FUNCTION__, mDpy);
        reset(ctx);
        return false;
    }

    int mdpCount = mCurrentFrame.mdpCount;

    if(sEnable4k2kYUVSplit){
        adjustForSourceSplit(ctx, list);
    }

    //Will benefit cases where a video has non-updating background.
    if((mDpy > HWC_DISPLAY_PRIMARY) and
            (mdpCount > MAX_SEC_LAYERS)) {
        ALOGD_IF(isDebug(), "%s: Exceeds max secondary pipes",__FUNCTION__);
        reset(ctx);
        return false;
    }

    if(!postHeuristicsHandling(ctx, list)) {
        ALOGD_IF(isDebug(), "post heuristic handling failed");
        reset(ctx);
        return false;
    }
    ALOGD_IF(sSimulationFlags,"%s: CACHE_MDP_COMP SUCCEEDED",
             __FUNCTION__);

    return true;
}

bool MDPComp::loadBasedComp(hwc_context_t *ctx,
        hwc_display_contents_1_t* list) {
    if(sSimulationFlags & MDPCOMP_AVOID_LOAD_MDP)
        return false;

    if(not isLoadBasedCompDoable(ctx)) {
        return false;
    }

    const int numAppLayers = ctx->listStats[mDpy].numAppLayers;
    const int numNonDroppedLayers = numAppLayers - mCurrentFrame.dropCount;
    const int stagesForMDP = min(sMaxPipesPerMixer,
            ctx->mOverlay->availablePipes(mDpy, Overlay::MIXER_DEFAULT));

    int mdpBatchSize = stagesForMDP - 1; //1 stage for FB
    int fbBatchSize = numNonDroppedLayers - mdpBatchSize;
    int lastMDPSupportedIndex = numAppLayers;
    int dropCount = 0;

    //Find the minimum MDP batch size
    for(int i = 0; i < numAppLayers;i++) {
        if(mCurrentFrame.drop[i]) {
            dropCount++;
            continue;
        }
        hwc_layer_1_t* layer = &list->hwLayers[i];
        if(not isSupportedForMDPComp(ctx, layer)) {
            lastMDPSupportedIndex = i;
            mdpBatchSize = min(i - dropCount, stagesForMDP - 1);
            fbBatchSize = numNonDroppedLayers - mdpBatchSize;
            break;
        }
    }

    ALOGD_IF(isDebug(), "%s:Before optimizing fbBatch, mdpbatch %d, fbbatch %d "
            "dropped %d", __FUNCTION__, mdpBatchSize, fbBatchSize,
            mCurrentFrame.dropCount);

    //Start at a point where the fb batch should at least have 2 layers, for
    //this mode to be justified.
    while(fbBatchSize < 2) {
        ++fbBatchSize;
        --mdpBatchSize;
    }

    //If there are no layers for MDP, this mode doesnt make sense.
    if(mdpBatchSize < 1) {
        ALOGD_IF(isDebug(), "%s: No MDP layers after optimizing for fbBatch",
                __FUNCTION__);
        return false;
    }

    mCurrentFrame.reset(numAppLayers);

    //Try with successively smaller mdp batch sizes until we succeed or reach 1
    while(mdpBatchSize > 0) {
        //Mark layers for MDP comp
        int mdpBatchLeft = mdpBatchSize;
        for(int i = 0; i < lastMDPSupportedIndex and mdpBatchLeft; i++) {
            if(mCurrentFrame.drop[i]) {
                continue;
            }
            mCurrentFrame.isFBComposed[i] = false;
            --mdpBatchLeft;
        }

        mCurrentFrame.fbZ = mdpBatchSize;
        mCurrentFrame.fbCount = fbBatchSize;
        mCurrentFrame.mdpCount = mdpBatchSize;

        ALOGD_IF(isDebug(), "%s:Trying with: mdpbatch %d fbbatch %d dropped %d",
                __FUNCTION__, mdpBatchSize, fbBatchSize,
                mCurrentFrame.dropCount);

        if(postHeuristicsHandling(ctx, list)) {
            ALOGD_IF(isDebug(), "%s: Postheuristics handling succeeded",
                     __FUNCTION__);
            ALOGD_IF(sSimulationFlags,"%s: LOAD_MDP_COMP SUCCEEDED",
                     __FUNCTION__);
            return true;
        }

        reset(ctx);
        --mdpBatchSize;
        ++fbBatchSize;
    }

    return false;
}

bool MDPComp::isLoadBasedCompDoable(hwc_context_t *ctx) {
    if(mDpy or isSecurePresent(ctx, mDpy) or
            isYuvPresent(ctx, mDpy)) {
        return false;
    }
    return true;
}

bool MDPComp::canPartialUpdate(hwc_context_t *ctx,
        hwc_display_contents_1_t* list){
    if(!qdutils::MDPVersion::getInstance().isPartialUpdateEnabled() ||
            isSkipPresent(ctx, mDpy) || (list->flags & HWC_GEOMETRY_CHANGED) ||
            mDpy ) {
        return false;
    }
    if(ctx->listStats[mDpy].secureUI)
        return false;
    return true;
}

bool MDPComp::tryVideoOnly(hwc_context_t *ctx,
        hwc_display_contents_1_t* list) {
    const bool secureOnly = true;
    return videoOnlyComp(ctx, list, not secureOnly) or
            videoOnlyComp(ctx, list, secureOnly);
}

bool MDPComp::videoOnlyComp(hwc_context_t *ctx,
        hwc_display_contents_1_t* list, bool secureOnly) {
    if(sSimulationFlags & MDPCOMP_AVOID_VIDEO_ONLY)
        return false;
    int numAppLayers = ctx->listStats[mDpy].numAppLayers;

    mCurrentFrame.reset(numAppLayers);
    mCurrentFrame.fbCount -= mCurrentFrame.dropCount;
    updateYUV(ctx, list, secureOnly);
    int mdpCount = mCurrentFrame.mdpCount;

    if(!isYuvPresent(ctx, mDpy) or (mdpCount == 0)) {
        reset(ctx);
        return false;
    }

    /* Bail out if we are processing only secured video layers
     * and we dont have any */
    if(!isSecurePresent(ctx, mDpy) && secureOnly){
        reset(ctx);
        return false;
    }

    if(mCurrentFrame.fbCount)
        mCurrentFrame.fbZ = mCurrentFrame.mdpCount;

    if(sEnable4k2kYUVSplit){
        adjustForSourceSplit(ctx, list);
    }

    if(!postHeuristicsHandling(ctx, list)) {
        ALOGD_IF(isDebug(), "post heuristic handling failed");
        reset(ctx);
        return false;
    }

    ALOGD_IF(sSimulationFlags,"%s: VIDEO_ONLY_COMP SUCCEEDED",
             __FUNCTION__);
    return true;
}

/* Checks for conditions where YUV layers cannot be bypassed */
bool MDPComp::isYUVDoable(hwc_context_t* ctx, hwc_layer_1_t* layer) {
    if(isSkipLayer(layer)) {
        ALOGD_IF(isDebug(), "%s: Video marked SKIP dpy %d", __FUNCTION__, mDpy);
        return false;
    }

    if(has90Transform(layer) && !canUseRotator(ctx, mDpy)) {
        ALOGD_IF(isDebug(), "%s: no free DMA pipe",__FUNCTION__);
        return false;
    }

    if(isSecuring(ctx, layer)) {
        ALOGD_IF(isDebug(), "%s: MDP securing is active", __FUNCTION__);
        return false;
    }

    if(!isValidDimension(ctx, layer)) {
        ALOGD_IF(isDebug(), "%s: Buffer is of invalid width",
            __FUNCTION__);
        return false;
    }

    if(layer->planeAlpha < 0xFF) {
        ALOGD_IF(isDebug(), "%s: Cannot handle YUV layer with plane alpha\
                 in video only mode",
                 __FUNCTION__);
        return false;
    }

    return true;
}

/* starts at fromIndex and check for each layer to find
 * if it it has overlapping with any Updating layer above it in zorder
 * till the end of the batch. returns true if it finds any intersection */
bool MDPComp::canPushBatchToTop(const hwc_display_contents_1_t* list,
        int fromIndex, int toIndex) {
    for(int i = fromIndex; i < toIndex; i++) {
        if(mCurrentFrame.isFBComposed[i] && !mCurrentFrame.drop[i]) {
            if(intersectingUpdatingLayers(list, i+1, toIndex, i)) {
                return false;
            }
        }
    }
    return true;
}

/* Checks if given layer at targetLayerIndex has any
 * intersection with all the updating layers in beween
 * fromIndex and toIndex. Returns true if it finds intersectiion */
bool MDPComp::intersectingUpdatingLayers(const hwc_display_contents_1_t* list,
        int fromIndex, int toIndex, int targetLayerIndex) {
    for(int i = fromIndex; i <= toIndex; i++) {
        if(!mCurrentFrame.isFBComposed[i]) {
            if(areLayersIntersecting(&list->hwLayers[i],
                        &list->hwLayers[targetLayerIndex]))  {
                return true;
            }
        }
    }
    return false;
}

int MDPComp::getBatch(hwc_display_contents_1_t* list,
        int& maxBatchStart, int& maxBatchEnd,
        int& maxBatchCount) {
    int i = 0;
    int fbZOrder =-1;
    int droppedLayerCt = 0;
    while (i < mCurrentFrame.layerCount) {
        int batchCount = 0;
        int batchStart = i;
        int batchEnd = i;
        /* Adjust batch Z order with the dropped layers so far */
        int fbZ = batchStart - droppedLayerCt;
        int firstZReverseIndex = -1;
        int updatingLayersAbove = 0;//Updating layer count in middle of batch
        while(i < mCurrentFrame.layerCount) {
            if(!mCurrentFrame.isFBComposed[i]) {
                if(!batchCount) {
                    i++;
                    break;
                }
                updatingLayersAbove++;
                i++;
                continue;
            } else {
                if(mCurrentFrame.drop[i]) {
                    i++;
                    droppedLayerCt++;
                    continue;
                } else if(updatingLayersAbove <= 0) {
                    batchCount++;
                    batchEnd = i;
                    i++;
                    continue;
                } else { //Layer is FBComposed, not a drop & updatingLayer > 0

                    // We have a valid updating layer already. If layer-i not
                    // have overlapping with all updating layers in between
                    // batch-start and i, then we can add layer i to batch.
                    if(!intersectingUpdatingLayers(list, batchStart, i-1, i)) {
                        batchCount++;
                        batchEnd = i;
                        i++;
                        continue;
                    } else if(canPushBatchToTop(list, batchStart, i)) {
                        //If All the non-updating layers with in this batch
                        //does not have intersection with the updating layers
                        //above in z-order, then we can safely move the batch to
                        //higher z-order. Increment fbZ as it is moving up.
                        if( firstZReverseIndex < 0) {
                            firstZReverseIndex = i;
                        }
                        batchCount++;
                        batchEnd = i;
                        fbZ += updatingLayersAbove;
                        i++;
                        updatingLayersAbove = 0;
                        continue;
                    } else {
                        //both failed.start the loop again from here.
                        if(firstZReverseIndex >= 0) {
                            i = firstZReverseIndex;
                        }
                        break;
                    }
                }
            }
        }
        if(batchCount > maxBatchCount) {
            maxBatchCount = batchCount;
            maxBatchStart = batchStart;
            maxBatchEnd = batchEnd;
            fbZOrder = fbZ;
        }
    }
    return fbZOrder;
}

bool  MDPComp::markLayersForCaching(hwc_context_t* ctx,
        hwc_display_contents_1_t* list) {
    /* Idea is to keep as many non-updating(cached) layers in FB and
     * send rest of them through MDP. This is done in 2 steps.
     *   1. Find the maximum contiguous batch of non-updating layers.
     *   2. See if we can improve this batch size for caching by adding
     *      opaque layers around the batch, if they don't have
     *      any overlapping with the updating layers in between.
     * NEVER mark an updating layer for caching.
     * But cached ones can be marked for MDP */

    int maxBatchStart = -1;
    int maxBatchEnd = -1;
    int maxBatchCount = 0;
    int fbZ = -1;

    /* Nothing is cached. No batching needed */
    if(mCurrentFrame.fbCount == 0) {
        return true;
    }

    /* No MDP comp layers, try to use other comp modes */
    if(mCurrentFrame.mdpCount == 0) {
        return false;
    }

    fbZ = getBatch(list, maxBatchStart, maxBatchEnd, maxBatchCount);

    /* reset rest of the layers lying inside ROI for MDP comp */
    for(int i = 0; i < mCurrentFrame.layerCount; i++) {
        hwc_layer_1_t* layer = &list->hwLayers[i];
        if((i < maxBatchStart || i > maxBatchEnd) &&
                mCurrentFrame.isFBComposed[i]){
            if(!mCurrentFrame.drop[i]){
                //If an unsupported layer is being attempted to
                //be pulled out we should fail
                if(not isSupportedForMDPComp(ctx, layer)) {
                    return false;
                }
                mCurrentFrame.isFBComposed[i] = false;
            }
        }
    }

    // update the frame data
    mCurrentFrame.fbZ = fbZ;
    mCurrentFrame.fbCount = maxBatchCount;
    mCurrentFrame.mdpCount = mCurrentFrame.layerCount -
            mCurrentFrame.fbCount - mCurrentFrame.dropCount;

    ALOGD_IF(isDebug(),"%s: cached count: %d",__FUNCTION__,
            mCurrentFrame.fbCount);

    return true;
}

void MDPComp::updateLayerCache(hwc_context_t* ctx,
        hwc_display_contents_1_t* list) {
    int numAppLayers = ctx->listStats[mDpy].numAppLayers;
    int fbCount = 0;

    for(int i = 0; i < numAppLayers; i++) {
        if (mCachedFrame.hnd[i] == list->hwLayers[i].handle) {
            if(!mCurrentFrame.drop[i])
                fbCount++;
            mCurrentFrame.isFBComposed[i] = true;
        } else {
            mCurrentFrame.isFBComposed[i] = false;
        }
    }

    mCurrentFrame.fbCount = fbCount;
    mCurrentFrame.mdpCount = mCurrentFrame.layerCount - mCurrentFrame.fbCount
                                                    - mCurrentFrame.dropCount;

    ALOGD_IF(isDebug(),"%s: MDP count: %d FB count %d drop count: %d"
             ,__FUNCTION__, mCurrentFrame.mdpCount, mCurrentFrame.fbCount,
            mCurrentFrame.dropCount);
}

void MDPComp::updateYUV(hwc_context_t* ctx, hwc_display_contents_1_t* list,
        bool secureOnly) {
    int nYuvCount = ctx->listStats[mDpy].yuvCount;
    for(int index = 0;index < nYuvCount; index++){
        int nYuvIndex = ctx->listStats[mDpy].yuvIndices[index];
        hwc_layer_1_t* layer = &list->hwLayers[nYuvIndex];

        if(!isYUVDoable(ctx, layer)) {
            if(!mCurrentFrame.isFBComposed[nYuvIndex]) {
                mCurrentFrame.isFBComposed[nYuvIndex] = true;
                mCurrentFrame.fbCount++;
            }
        } else {
            if(mCurrentFrame.isFBComposed[nYuvIndex]) {
                private_handle_t *hnd = (private_handle_t *)layer->handle;
                if(!secureOnly || isSecureBuffer(hnd)) {
                    mCurrentFrame.isFBComposed[nYuvIndex] = false;
                    mCurrentFrame.fbCount--;
                }
            }
        }
    }

    mCurrentFrame.mdpCount = mCurrentFrame.layerCount -
            mCurrentFrame.fbCount - mCurrentFrame.dropCount;
    ALOGD_IF(isDebug(),"%s: fb count: %d",__FUNCTION__,
             mCurrentFrame.fbCount);
}

hwc_rect_t MDPComp::getUpdatingFBRect(hwc_context_t *ctx,
        hwc_display_contents_1_t* list){
    hwc_rect_t fbRect = (struct hwc_rect){0, 0, 0, 0};

    /* Update only the region of FB needed for composition */
    for(int i = 0; i < mCurrentFrame.layerCount; i++ ) {
        if(mCurrentFrame.isFBComposed[i] && !mCurrentFrame.drop[i]) {
            hwc_layer_1_t* layer = &list->hwLayers[i];
            hwc_rect_t dst = layer->displayFrame;
            fbRect = getUnion(fbRect, dst);
        }
    }
    trimAgainstROI(ctx, fbRect);
    return fbRect;
}

bool MDPComp::postHeuristicsHandling(hwc_context_t *ctx,
        hwc_display_contents_1_t* list) {

    //Capability checks
    if(!resourceCheck(ctx, list)) {
        ALOGD_IF(isDebug(), "%s: resource check failed", __FUNCTION__);
        return false;
    }

    //Limitations checks
    if(!hwLimitationsCheck(ctx, list)) {
        ALOGD_IF(isDebug(), "%s: HW limitations",__FUNCTION__);
        return false;
    }

    //Configure framebuffer first if applicable
    if(mCurrentFrame.fbZ >= 0) {
        hwc_rect_t fbRect = getUpdatingFBRect(ctx, list);
        if(!ctx->mFBUpdate[mDpy]->prepare(ctx, list, fbRect, mCurrentFrame.fbZ))
        {
            ALOGD_IF(isDebug(), "%s configure framebuffer failed",
                    __FUNCTION__);
            return false;
        }
    }

    mCurrentFrame.map();

    if(!allocLayerPipes(ctx, list)) {
        ALOGD_IF(isDebug(), "%s: Unable to allocate MDP pipes", __FUNCTION__);
        return false;
    }

    for (int index = 0, mdpNextZOrder = 0; index < mCurrentFrame.layerCount;
            index++) {
        if(!mCurrentFrame.isFBComposed[index]) {
            int mdpIndex = mCurrentFrame.layerToMDP[index];
            hwc_layer_1_t* layer = &list->hwLayers[index];

            //Leave fbZ for framebuffer. CACHE/GLES layers go here.
            if(mdpNextZOrder == mCurrentFrame.fbZ) {
                mdpNextZOrder++;
            }
            MdpPipeInfo* cur_pipe = mCurrentFrame.mdpToLayer[mdpIndex].pipeInfo;
            cur_pipe->zOrder = mdpNextZOrder++;

            private_handle_t *hnd = (private_handle_t *)layer->handle;
            if(is4kx2kYuvBuffer(hnd) && sEnable4k2kYUVSplit){
                if(configure4k2kYuv(ctx, layer,
                            mCurrentFrame.mdpToLayer[mdpIndex])
                        != 0 ){
                    ALOGD_IF(isDebug(), "%s: Failed to configure split pipes \
                            for layer %d",__FUNCTION__, index);
                    return false;
                }
                else{
                    mdpNextZOrder++;
                }
                continue;
            }
            if(configure(ctx, layer, mCurrentFrame.mdpToLayer[mdpIndex]) != 0 ){
                ALOGD_IF(isDebug(), "%s: Failed to configure overlay for \
                        layer %d",__FUNCTION__, index);
                return false;
            }
        }
    }

    if(!ctx->mOverlay->validateAndSet(mDpy, ctx->dpyAttr[mDpy].fd)) {
        ALOGD_IF(isDebug(), "%s: Failed to validate and set overlay for dpy %d"
                ,__FUNCTION__, mDpy);
        return false;
    }

    setRedraw(ctx, list);
    return true;
}

bool MDPComp::resourceCheck(hwc_context_t* ctx,
        hwc_display_contents_1_t* list) {
    const bool fbUsed = mCurrentFrame.fbCount;
    if(mCurrentFrame.mdpCount > sMaxPipesPerMixer - fbUsed) {
        ALOGD_IF(isDebug(), "%s: Exceeds MAX_PIPES_PER_MIXER",__FUNCTION__);
        return false;
    }
    // Init rotCount to number of rotate sessions used by other displays
    int rotCount = ctx->mRotMgr->getNumActiveSessions();
    // Count the number of rotator sessions required for current display
    for (int index = 0; index < mCurrentFrame.layerCount; index++) {
        if(!mCurrentFrame.isFBComposed[index]) {
            hwc_layer_1_t* layer = &list->hwLayers[index];
            private_handle_t *hnd = (private_handle_t *)layer->handle;
            if(has90Transform(layer) && isRotationDoable(ctx, hnd)) {
                rotCount++;
            }
        }
    }
    // if number of layers to rotate exceeds max rotator sessions, bail out.
    if(rotCount > RotMgr::MAX_ROT_SESS) {
        ALOGD_IF(isDebug(), "%s: Exceeds max rotator sessions  %d",
                                    __FUNCTION__, mDpy);
        return false;
    }
    return true;
}

bool MDPComp::hwLimitationsCheck(hwc_context_t* ctx,
        hwc_display_contents_1_t* list) {

    //A-family hw limitation:
    //If a layer need alpha scaling, MDP can not support.
    if(ctx->mMDP.version < qdutils::MDSS_V5) {
        for(int i = 0; i < mCurrentFrame.layerCount; ++i) {
            if(!mCurrentFrame.isFBComposed[i] &&
                    isAlphaScaled( &list->hwLayers[i])) {
                ALOGD_IF(isDebug(), "%s:frame needs alphaScaling",__FUNCTION__);
                return false;
            }
        }
    }

    // On 8x26 & 8974 hw, we have a limitation of downscaling+blending.
    //If multiple layers requires downscaling and also they are overlapping
    //fall back to GPU since MDSS can not handle it.
    if(qdutils::MDPVersion::getInstance().is8x74v2() ||
            qdutils::MDPVersion::getInstance().is8x26()) {
        for(int i = 0; i < mCurrentFrame.layerCount-1; ++i) {
            hwc_layer_1_t* botLayer = &list->hwLayers[i];
            if(!mCurrentFrame.isFBComposed[i] &&
                    isDownscaleRequired(botLayer)) {
                //if layer-i is marked for MDP and needs downscaling
                //check if any MDP layer on top of i & overlaps with layer-i
                for(int j = i+1; j < mCurrentFrame.layerCount; ++j) {
                    hwc_layer_1_t* topLayer = &list->hwLayers[j];
                    if(!mCurrentFrame.isFBComposed[j] &&
                            isDownscaleRequired(topLayer)) {
                        hwc_rect_t r = getIntersection(botLayer->displayFrame,
                                topLayer->displayFrame);
                        if(isValidRect(r))
                            return false;
                    }
                }
            }
        }
    }
    return true;
}

int MDPComp::prepare(hwc_context_t *ctx, hwc_display_contents_1_t* list) {
    int ret = 0;
    char property[PROPERTY_VALUE_MAX];

    if(!ctx || !list) {
        ALOGE("%s: Invalid context or list",__FUNCTION__);
        mCachedFrame.reset();
        return -1;
    }

    const int numLayers = ctx->listStats[mDpy].numAppLayers;

    if(property_get("debug.hwc.simulate", property, NULL) > 0) {
        int currentFlags = atoi(property);
        if(currentFlags != sSimulationFlags) {
            sSimulationFlags = currentFlags;
            ALOGE("%s: Simulation Flag read: 0x%x (%d)", __FUNCTION__,
                    sSimulationFlags, sSimulationFlags);
        }
    }
    // reset PTOR
    if(!mDpy)
        memset(&(ctx->mPtorInfo), 0, sizeof(ctx->mPtorInfo));

    //Do not cache the information for next draw cycle.
    if(numLayers > MAX_NUM_APP_LAYERS or (!numLayers)) {
        ALOGI("%s: Unsupported layer count for mdp composition",
                __FUNCTION__);
        mCachedFrame.reset();
        return -1;
    }

    //reset old data
    mCurrentFrame.reset(numLayers);
    memset(&mCurrentFrame.drop, 0, sizeof(mCurrentFrame.drop));
    mCurrentFrame.dropCount = 0;

    // Detect the start of animation and fall back to GPU only once to cache
    // all the layers in FB and display FB content untill animation completes.
    if(ctx->listStats[mDpy].isDisplayAnimating) {
        mCurrentFrame.needsRedraw = false;
        if(ctx->mAnimationState[mDpy] == ANIMATION_STOPPED) {
            mCurrentFrame.needsRedraw = true;
            ctx->mAnimationState[mDpy] = ANIMATION_STARTED;
        }
        setMDPCompLayerFlags(ctx, list);
        mCachedFrame.updateCounts(mCurrentFrame);
        ret = -1;
        return ret;
    } else {
        ctx->mAnimationState[mDpy] = ANIMATION_STOPPED;
    }

    //Hard conditions, if not met, cannot do MDP comp
    if(isFrameDoable(ctx)) {
        generateROI(ctx, list);

        mModeOn = tryFullFrame(ctx, list) || tryVideoOnly(ctx, list);
        if(mModeOn) {
            setMDPCompLayerFlags(ctx, list);
        } else {
            resetROI(ctx, mDpy);
            reset(ctx);
            memset(&mCurrentFrame.drop, 0, sizeof(mCurrentFrame.drop));
            mCurrentFrame.dropCount = 0;
            ret = -1;
        }
    } else {
        ALOGD_IF( isDebug(),"%s: MDP Comp not possible for this frame",
                __FUNCTION__);
        ret = -1;
    }

    if(isDebug()) {
        ALOGD("GEOMETRY change: %d",
                (list->flags & HWC_GEOMETRY_CHANGED));
        android::String8 sDump("");
        dump(sDump, ctx);
        ALOGD("%s",sDump.string());
    }

    mCachedFrame.cacheAll(list);
    mCachedFrame.updateCounts(mCurrentFrame);
    return ret;
}

bool MDPComp::allocSplitVGPipesfor4k2k(hwc_context_t *ctx, int index) {

    bool bRet = true;
    int mdpIndex = mCurrentFrame.layerToMDP[index];
    PipeLayerPair& info = mCurrentFrame.mdpToLayer[mdpIndex];
    info.pipeInfo = new MdpYUVPipeInfo;
    info.rot = NULL;
    MdpYUVPipeInfo& pipe_info = *(MdpYUVPipeInfo*)info.pipeInfo;

    pipe_info.lIndex = ovutils::OV_INVALID;
    pipe_info.rIndex = ovutils::OV_INVALID;

    Overlay::PipeSpecs pipeSpecs;
    pipeSpecs.formatClass = Overlay::FORMAT_YUV;
    pipeSpecs.needsScaling = true;
    pipeSpecs.dpy = mDpy;
    pipeSpecs.fb = false;

    pipe_info.lIndex = ctx->mOverlay->getPipe(pipeSpecs);
    if(pipe_info.lIndex == ovutils::OV_INVALID){
        bRet = false;
        ALOGD_IF(isDebug(),"%s: allocating first VG pipe failed",
                __FUNCTION__);
    }
    pipe_info.rIndex = ctx->mOverlay->getPipe(pipeSpecs);
    if(pipe_info.rIndex == ovutils::OV_INVALID){
        bRet = false;
        ALOGD_IF(isDebug(),"%s: allocating second VG pipe failed",
                __FUNCTION__);
    }
    return bRet;
}

int MDPComp::drawOverlap(hwc_context_t *ctx, hwc_display_contents_1_t* list) {
    int fd = -1;
    if (ctx->mPtorInfo.isActive()) {
        fd = ctx->mCopyBit[mDpy]->drawOverlap(ctx, list);
        if (fd < 0) {
            ALOGD_IF(isDebug(),"%s: failed", __FUNCTION__);
        }
    }
    return fd;
}
//=============MDPCompNonSplit==================================================

void MDPCompNonSplit::adjustForSourceSplit(hwc_context_t *ctx,
        hwc_display_contents_1_t* list) {
    //If 4k2k Yuv layer split is possible,  and if
    //fbz is above 4k2k layer, increment fb zorder by 1
    //as we split 4k2k layer and increment zorder for right half
    //of the layer
    if(!ctx)
        return;
    if(mCurrentFrame.fbZ >= 0) {
        for (int index = 0, mdpNextZOrder = 0; index < mCurrentFrame.layerCount;
                index++) {
            if(!mCurrentFrame.isFBComposed[index]) {
                if(mdpNextZOrder == mCurrentFrame.fbZ) {
                    mdpNextZOrder++;
                }
                mdpNextZOrder++;
                hwc_layer_1_t* layer = &list->hwLayers[index];
                private_handle_t *hnd = (private_handle_t *)layer->handle;
                if(is4kx2kYuvBuffer(hnd)) {
                    if(mdpNextZOrder <= mCurrentFrame.fbZ)
                        mCurrentFrame.fbZ += 1;
                    mdpNextZOrder++;
                    //As we split 4kx2k yuv layer and program to 2 VG pipes
                    //(if available) increase mdpcount by 1.
                    mCurrentFrame.mdpCount++;
                }
            }
        }
    }
}

/*
 * Configures pipe(s) for MDP composition
 */
int MDPCompNonSplit::configure(hwc_context_t *ctx, hwc_layer_1_t *layer,
                             PipeLayerPair& PipeLayerPair) {
    MdpPipeInfoNonSplit& mdp_info =
        *(static_cast<MdpPipeInfoNonSplit*>(PipeLayerPair.pipeInfo));
    eMdpFlags mdpFlags = OV_MDP_BACKEND_COMPOSITION;
    eZorder zOrder = static_cast<eZorder>(mdp_info.zOrder);
    eIsFg isFg = IS_FG_OFF;
    eDest dest = mdp_info.index;

    ALOGD_IF(isDebug(),"%s: configuring: layer: %p z_order: %d dest_pipe: %d",
             __FUNCTION__, layer, zOrder, dest);

    return configureNonSplit(ctx, layer, mDpy, mdpFlags, zOrder, isFg, dest,
                           &PipeLayerPair.rot);
}

bool MDPCompNonSplit::allocLayerPipes(hwc_context_t *ctx,
        hwc_display_contents_1_t* list) {
    for(int index = 0; index < mCurrentFrame.layerCount; index++) {

        if(mCurrentFrame.isFBComposed[index]) continue;

        hwc_layer_1_t* layer = &list->hwLayers[index];
        private_handle_t *hnd = (private_handle_t *)layer->handle;
        if(is4kx2kYuvBuffer(hnd) && sEnable4k2kYUVSplit){
            if(allocSplitVGPipesfor4k2k(ctx, index)){
                continue;
            }
        }

        int mdpIndex = mCurrentFrame.layerToMDP[index];
        PipeLayerPair& info = mCurrentFrame.mdpToLayer[mdpIndex];
        info.pipeInfo = new MdpPipeInfoNonSplit;
        info.rot = NULL;
        MdpPipeInfoNonSplit& pipe_info = *(MdpPipeInfoNonSplit*)info.pipeInfo;

        Overlay::PipeSpecs pipeSpecs;
        pipeSpecs.formatClass = isYuvBuffer(hnd) ?
                Overlay::FORMAT_YUV : Overlay::FORMAT_RGB;
        pipeSpecs.needsScaling = qhwc::needsScaling(layer) or
                (qdutils::MDPVersion::getInstance().is8x26() and
                ctx->dpyAttr[HWC_DISPLAY_PRIMARY].xres > 1024);
        pipeSpecs.dpy = mDpy;
        pipeSpecs.fb = false;
        pipeSpecs.numActiveDisplays = ctx->numActiveDisplays;

        pipe_info.index = ctx->mOverlay->getPipe(pipeSpecs);

        if(pipe_info.index == ovutils::OV_INVALID) {
            ALOGD_IF(isDebug(), "%s: Unable to get pipe", __FUNCTION__);
            return false;
        }
    }
    return true;
}

int MDPCompNonSplit::configure4k2kYuv(hwc_context_t *ctx, hwc_layer_1_t *layer,
        PipeLayerPair& PipeLayerPair) {
    MdpYUVPipeInfo& mdp_info =
            *(static_cast<MdpYUVPipeInfo*>(PipeLayerPair.pipeInfo));
    eZorder zOrder = static_cast<eZorder>(mdp_info.zOrder);
    eIsFg isFg = IS_FG_OFF;
    eMdpFlags mdpFlagsL = OV_MDP_BACKEND_COMPOSITION;
    eDest lDest = mdp_info.lIndex;
    eDest rDest = mdp_info.rIndex;

    return configureSourceSplit(ctx, layer, mDpy, mdpFlagsL, zOrder, isFg,
            lDest, rDest, &PipeLayerPair.rot);
}

bool MDPCompNonSplit::draw(hwc_context_t *ctx, hwc_display_contents_1_t* list) {

    if(!isEnabled() or !mModeOn) {
        ALOGD_IF(isDebug(),"%s: MDP Comp not enabled/configured", __FUNCTION__);
        return true;
    }

    // Set the Handle timeout to true for MDP or MIXED composition.
    if(idleInvalidator && !sIdleFallBack && mCurrentFrame.mdpCount) {
        sHandleTimeout = true;
    }

    overlay::Overlay& ov = *ctx->mOverlay;
    LayerProp *layerProp = ctx->layerProp[mDpy];

    int numHwLayers = ctx->listStats[mDpy].numAppLayers;
    for(int i = 0; i < numHwLayers && mCurrentFrame.mdpCount; i++ )
    {
        if(mCurrentFrame.isFBComposed[i]) continue;

        hwc_layer_1_t *layer = &list->hwLayers[i];
        private_handle_t *hnd = (private_handle_t *)layer->handle;
        if(!hnd) {
            if (!(layer->flags & HWC_COLOR_FILL)) {
                ALOGE("%s handle null", __FUNCTION__);
                return false;
            }
            // No PLAY for Color layer
            layerProp[i].mFlags &= ~HWC_MDPCOMP;
            continue;
        }

        int mdpIndex = mCurrentFrame.layerToMDP[i];

        if(is4kx2kYuvBuffer(hnd) && sEnable4k2kYUVSplit)
        {
            MdpYUVPipeInfo& pipe_info =
                *(MdpYUVPipeInfo*)mCurrentFrame.mdpToLayer[mdpIndex].pipeInfo;
            Rotator *rot = mCurrentFrame.mdpToLayer[mdpIndex].rot;
            ovutils::eDest indexL = pipe_info.lIndex;
            ovutils::eDest indexR = pipe_info.rIndex;
            int fd = hnd->fd;
            uint32_t offset = (uint32_t)hnd->offset;
            if(rot) {
                rot->queueBuffer(fd, offset);
                fd = rot->getDstMemId();
                offset = rot->getDstOffset();
            }
            if(indexL != ovutils::OV_INVALID) {
                ovutils::eDest destL = (ovutils::eDest)indexL;
                ALOGD_IF(isDebug(),"%s: MDP Comp: Drawing layer: %p hnd: %p \
                        using  pipe: %d", __FUNCTION__, layer, hnd, indexL );
                if (!ov.queueBuffer(fd, offset, destL)) {
                    ALOGE("%s: queueBuffer failed for display:%d",
                            __FUNCTION__, mDpy);
                    return false;
                }
            }

            if(indexR != ovutils::OV_INVALID) {
                ovutils::eDest destR = (ovutils::eDest)indexR;
                ALOGD_IF(isDebug(),"%s: MDP Comp: Drawing layer: %p hnd: %p \
                        using  pipe: %d", __FUNCTION__, layer, hnd, indexR );
                if (!ov.queueBuffer(fd, offset, destR)) {
                    ALOGE("%s: queueBuffer failed for display:%d",
                            __FUNCTION__, mDpy);
                    return false;
                }
            }
        }
        else{
            MdpPipeInfoNonSplit& pipe_info =
            *(MdpPipeInfoNonSplit*)mCurrentFrame.mdpToLayer[mdpIndex].pipeInfo;
            ovutils::eDest dest = pipe_info.index;
            if(dest == ovutils::OV_INVALID) {
                ALOGE("%s: Invalid pipe index (%d)", __FUNCTION__, dest);
                return false;
            }

            if(!(layerProp[i].mFlags & HWC_MDPCOMP)) {
                continue;
            }

            int fd = hnd->fd;
            uint32_t offset = (uint32_t)hnd->offset;
            int index = ctx->mPtorInfo.getPTORArrayIndex(i);
            if (!mDpy && (index != -1)) {
                hnd = ctx->mCopyBit[mDpy]->getCurrentRenderBuffer();
                fd = hnd->fd;
                // Use the offset of the RenderBuffer
                offset = ctx->mPtorInfo.mRenderBuffOffset[index];
            }

            ALOGD_IF(isDebug(),"%s: MDP Comp: Drawing layer: %p hnd: %p \
                    using  pipe: %d", __FUNCTION__, layer,
                    hnd, dest );

            Rotator *rot = mCurrentFrame.mdpToLayer[mdpIndex].rot;
            if(rot) {
                if(!rot->queueBuffer(fd, offset))
                    return false;
                fd = rot->getDstMemId();
                offset = rot->getDstOffset();
            }

            if (!ov.queueBuffer(fd, offset, dest)) {
                ALOGE("%s: queueBuffer failed for display:%d ",
                        __FUNCTION__, mDpy);
                return false;
            }
        }

        layerProp[i].mFlags &= ~HWC_MDPCOMP;
    }
    return true;
}

//=============MDPCompSplit===================================================

void MDPCompSplit::adjustForSourceSplit(hwc_context_t *ctx,
         hwc_display_contents_1_t* list){
    //if 4kx2k yuv layer is totally present in either in left half
    //or right half then try splitting the yuv layer to avoid decimation
    const int lSplit = getLeftSplit(ctx, mDpy);
    if(mCurrentFrame.fbZ >= 0) {
        for (int index = 0, mdpNextZOrder = 0; index < mCurrentFrame.layerCount;
                index++) {
            if(!mCurrentFrame.isFBComposed[index]) {
                if(mdpNextZOrder == mCurrentFrame.fbZ) {
                    mdpNextZOrder++;
                }
                mdpNextZOrder++;
                hwc_layer_1_t* layer = &list->hwLayers[index];
                private_handle_t *hnd = (private_handle_t *)layer->handle;
                if(is4kx2kYuvBuffer(hnd)) {
                    hwc_rect_t dst = layer->displayFrame;
                    if((dst.left > lSplit) || (dst.right < lSplit)) {
                        mCurrentFrame.mdpCount += 1;
                    }
                    if(mdpNextZOrder <= mCurrentFrame.fbZ)
                        mCurrentFrame.fbZ += 1;
                    mdpNextZOrder++;
                }
            }
        }
    }
}

bool MDPCompSplit::acquireMDPPipes(hwc_context_t *ctx, hwc_layer_1_t* layer,
        MdpPipeInfoSplit& pipe_info) {

    const int lSplit = getLeftSplit(ctx, mDpy);
    private_handle_t *hnd = (private_handle_t *)layer->handle;
    hwc_rect_t dst = layer->displayFrame;
    pipe_info.lIndex = ovutils::OV_INVALID;
    pipe_info.rIndex = ovutils::OV_INVALID;

    Overlay::PipeSpecs pipeSpecs;
    pipeSpecs.formatClass = isYuvBuffer(hnd) ?
            Overlay::FORMAT_YUV : Overlay::FORMAT_RGB;
    pipeSpecs.needsScaling = qhwc::needsScalingWithSplit(ctx, layer, mDpy);
    pipeSpecs.dpy = mDpy;
    pipeSpecs.mixer = Overlay::MIXER_LEFT;
    pipeSpecs.fb = false;

    // Acquire pipe only for the updating half
    hwc_rect_t l_roi = ctx->listStats[mDpy].lRoi;
    hwc_rect_t r_roi = ctx->listStats[mDpy].rRoi;

    if (dst.left < lSplit && isValidRect(getIntersection(dst, l_roi))) {
        pipe_info.lIndex = ctx->mOverlay->getPipe(pipeSpecs);
        if(pipe_info.lIndex == ovutils::OV_INVALID)
            return false;
    }

    if(dst.right > lSplit && isValidRect(getIntersection(dst, r_roi))) {
        pipeSpecs.mixer = Overlay::MIXER_RIGHT;
        pipe_info.rIndex = ctx->mOverlay->getPipe(pipeSpecs);
        if(pipe_info.rIndex == ovutils::OV_INVALID)
            return false;
    }

    return true;
}

bool MDPCompSplit::allocLayerPipes(hwc_context_t *ctx,
        hwc_display_contents_1_t* list) {
    for(int index = 0 ; index < mCurrentFrame.layerCount; index++) {

        if(mCurrentFrame.isFBComposed[index]) continue;

        hwc_layer_1_t* layer = &list->hwLayers[index];
        private_handle_t *hnd = (private_handle_t *)layer->handle;
        hwc_rect_t dst = layer->displayFrame;
        const int lSplit = getLeftSplit(ctx, mDpy);
        if(is4kx2kYuvBuffer(hnd) && sEnable4k2kYUVSplit){
            if((dst.left > lSplit)||(dst.right < lSplit)){
                if(allocSplitVGPipesfor4k2k(ctx, index)){
                    continue;
                }
            }
        }
        int mdpIndex = mCurrentFrame.layerToMDP[index];
        PipeLayerPair& info = mCurrentFrame.mdpToLayer[mdpIndex];
        info.pipeInfo = new MdpPipeInfoSplit;
        info.rot = NULL;
        MdpPipeInfoSplit& pipe_info = *(MdpPipeInfoSplit*)info.pipeInfo;

        if(!acquireMDPPipes(ctx, layer, pipe_info)) {
            ALOGD_IF(isDebug(), "%s: Unable to get pipe for type",
                    __FUNCTION__);
            return false;
        }
    }
    return true;
}

int MDPCompSplit::configure4k2kYuv(hwc_context_t *ctx, hwc_layer_1_t *layer,
        PipeLayerPair& PipeLayerPair) {
    const int lSplit = getLeftSplit(ctx, mDpy);
    hwc_rect_t dst = layer->displayFrame;
    if((dst.left > lSplit)||(dst.right < lSplit)){
        MdpYUVPipeInfo& mdp_info =
                *(static_cast<MdpYUVPipeInfo*>(PipeLayerPair.pipeInfo));
        eZorder zOrder = static_cast<eZorder>(mdp_info.zOrder);
        eIsFg isFg = IS_FG_OFF;
        eMdpFlags mdpFlagsL = OV_MDP_BACKEND_COMPOSITION;
        eDest lDest = mdp_info.lIndex;
        eDest rDest = mdp_info.rIndex;

        return configureSourceSplit(ctx, layer, mDpy, mdpFlagsL, zOrder, isFg,
                lDest, rDest, &PipeLayerPair.rot);
    }
    else{
        return configure(ctx, layer, PipeLayerPair);
    }
}

/*
 * Configures pipe(s) for MDP composition
 */
int MDPCompSplit::configure(hwc_context_t *ctx, hwc_layer_1_t *layer,
        PipeLayerPair& PipeLayerPair) {
    MdpPipeInfoSplit& mdp_info =
        *(static_cast<MdpPipeInfoSplit*>(PipeLayerPair.pipeInfo));
    eZorder zOrder = static_cast<eZorder>(mdp_info.zOrder);
    eIsFg isFg = IS_FG_OFF;
    eMdpFlags mdpFlagsL = OV_MDP_BACKEND_COMPOSITION;
    eDest lDest = mdp_info.lIndex;
    eDest rDest = mdp_info.rIndex;

    ALOGD_IF(isDebug(),"%s: configuring: layer: %p z_order: %d dest_pipeL: %d"
             "dest_pipeR: %d",__FUNCTION__, layer, zOrder, lDest, rDest);

    return configureSplit(ctx, layer, mDpy, mdpFlagsL, zOrder, isFg, lDest,
                            rDest, &PipeLayerPair.rot);
}

bool MDPCompSplit::draw(hwc_context_t *ctx, hwc_display_contents_1_t* list) {

    if(!isEnabled() or !mModeOn) {
        ALOGD_IF(isDebug(),"%s: MDP Comp not enabled/configured", __FUNCTION__);
        return true;
    }

    // Set the Handle timeout to true for MDP or MIXED composition.
    if(idleInvalidator && !sIdleFallBack && mCurrentFrame.mdpCount) {
        sHandleTimeout = true;
    }

    overlay::Overlay& ov = *ctx->mOverlay;
    LayerProp *layerProp = ctx->layerProp[mDpy];

    int numHwLayers = ctx->listStats[mDpy].numAppLayers;
    for(int i = 0; i < numHwLayers && mCurrentFrame.mdpCount; i++ )
    {
        if(mCurrentFrame.isFBComposed[i]) continue;

        hwc_layer_1_t *layer = &list->hwLayers[i];
        private_handle_t *hnd = (private_handle_t *)layer->handle;
        if(!hnd) {
            ALOGE("%s handle null", __FUNCTION__);
            return false;
        }

        if(!(layerProp[i].mFlags & HWC_MDPCOMP)) {
            continue;
        }

        int mdpIndex = mCurrentFrame.layerToMDP[i];

        if(is4kx2kYuvBuffer(hnd) && sEnable4k2kYUVSplit)
        {
            MdpYUVPipeInfo& pipe_info =
                *(MdpYUVPipeInfo*)mCurrentFrame.mdpToLayer[mdpIndex].pipeInfo;
            Rotator *rot = mCurrentFrame.mdpToLayer[mdpIndex].rot;
            ovutils::eDest indexL = pipe_info.lIndex;
            ovutils::eDest indexR = pipe_info.rIndex;
            int fd = hnd->fd;
            uint32_t offset = (uint32_t)hnd->offset;
            if(rot) {
                rot->queueBuffer(fd, offset);
                fd = rot->getDstMemId();
                offset = rot->getDstOffset();
            }
            if(indexL != ovutils::OV_INVALID) {
                ovutils::eDest destL = (ovutils::eDest)indexL;
                ALOGD_IF(isDebug(),"%s: MDP Comp: Drawing layer: %p hnd: %p \
                        using  pipe: %d", __FUNCTION__, layer, hnd, indexL );
                if (!ov.queueBuffer(fd, offset, destL)) {
                    ALOGE("%s: queueBuffer failed for display:%d",
                            __FUNCTION__, mDpy);
                    return false;
                }
            }

            if(indexR != ovutils::OV_INVALID) {
                ovutils::eDest destR = (ovutils::eDest)indexR;
                ALOGD_IF(isDebug(),"%s: MDP Comp: Drawing layer: %p hnd: %p \
                        using  pipe: %d", __FUNCTION__, layer, hnd, indexR );
                if (!ov.queueBuffer(fd, offset, destR)) {
                    ALOGE("%s: queueBuffer failed for display:%d",
                            __FUNCTION__, mDpy);
                    return false;
                }
            }
        }
        else{
            MdpPipeInfoSplit& pipe_info =
                *(MdpPipeInfoSplit*)mCurrentFrame.mdpToLayer[mdpIndex].pipeInfo;
            Rotator *rot = mCurrentFrame.mdpToLayer[mdpIndex].rot;

            ovutils::eDest indexL = pipe_info.lIndex;
            ovutils::eDest indexR = pipe_info.rIndex;

            int fd = hnd->fd;
            uint32_t offset = (uint32_t)hnd->offset;
            int index = ctx->mPtorInfo.getPTORArrayIndex(i);
            if (!mDpy && (index != -1)) {
                hnd = ctx->mCopyBit[mDpy]->getCurrentRenderBuffer();
                fd = hnd->fd;
                offset = ctx->mPtorInfo.mRenderBuffOffset[index];
            }

            if(ctx->mAD->draw(ctx, fd, offset)) {
                fd = ctx->mAD->getDstFd();
                offset = ctx->mAD->getDstOffset();
            }

            if(rot) {
                rot->queueBuffer(fd, offset);
                fd = rot->getDstMemId();
                offset = rot->getDstOffset();
            }

            //************* play left mixer **********
            if(indexL != ovutils::OV_INVALID) {
                ovutils::eDest destL = (ovutils::eDest)indexL;
                ALOGD_IF(isDebug(),"%s: MDP Comp: Drawing layer: %p hnd: %p \
                        using  pipe: %d", __FUNCTION__, layer, hnd, indexL );
                if (!ov.queueBuffer(fd, offset, destL)) {
                    ALOGE("%s: queueBuffer failed for left mixer",
                            __FUNCTION__);
                    return false;
                }
            }

            //************* play right mixer **********
            if(indexR != ovutils::OV_INVALID) {
                ovutils::eDest destR = (ovutils::eDest)indexR;
                ALOGD_IF(isDebug(),"%s: MDP Comp: Drawing layer: %p hnd: %p \
                        using  pipe: %d", __FUNCTION__, layer, hnd, indexR );
                if (!ov.queueBuffer(fd, offset, destR)) {
                    ALOGE("%s: queueBuffer failed for right mixer",
                            __FUNCTION__);
                    return false;
                }
            }
        }

        layerProp[i].mFlags &= ~HWC_MDPCOMP;
    }

    return true;
}

//================MDPCompSrcSplit==============================================
bool MDPCompSrcSplit::acquireMDPPipes(hwc_context_t *ctx, hwc_layer_1_t* layer,
        MdpPipeInfoSplit& pipe_info) {
    private_handle_t *hnd = (private_handle_t *)layer->handle;
    hwc_rect_t dst = layer->displayFrame;
    hwc_rect_t crop = integerizeSourceCrop(layer->sourceCropf);
    pipe_info.lIndex = ovutils::OV_INVALID;
    pipe_info.rIndex = ovutils::OV_INVALID;

    //If 2 pipes are staged on a single stage of a mixer, then the left pipe
    //should have a higher priority than the right one. Pipe priorities are
    //starting with VG0, VG1 ... , RGB0 ..., DMA1

    Overlay::PipeSpecs pipeSpecs;
    pipeSpecs.formatClass = isYuvBuffer(hnd) ?
            Overlay::FORMAT_YUV : Overlay::FORMAT_RGB;
    pipeSpecs.needsScaling = qhwc::needsScaling(layer);
    pipeSpecs.dpy = mDpy;
    pipeSpecs.fb = false;

    //1 pipe by default for a layer
    pipe_info.lIndex = ctx->mOverlay->getPipe(pipeSpecs);
    if(pipe_info.lIndex == ovutils::OV_INVALID) {
        return false;
    }

    /* Use 2 pipes IF
        a) Layer's crop width is > 2048 or
        b) Layer's dest width > 2048 or
        c) On primary, driver has indicated with caps to split always. This is
           based on an empirically derived value of panel height. Applied only
           if the layer's width is > mixer's width
    */

    bool primarySplitAlways = (mDpy == HWC_DISPLAY_PRIMARY) and
            qdutils::MDPVersion::getInstance().isSrcSplitAlways();
    int lSplit = getLeftSplit(ctx, mDpy);
    int dstWidth = dst.right - dst.left;
    int cropWidth = crop.right - crop.left;

    if(dstWidth > qdutils::MAX_DISPLAY_DIM or
            cropWidth > qdutils::MAX_DISPLAY_DIM or
            (primarySplitAlways and (cropWidth > lSplit))) {
        pipe_info.rIndex = ctx->mOverlay->getPipe(pipeSpecs);
        if(pipe_info.rIndex == ovutils::OV_INVALID) {
            return false;
        }

        // Return values
        // 1  Left pipe is higher priority, do nothing.
        // 0  Pipes of same priority.
        //-1  Right pipe is of higher priority, needs swap.
        if(ctx->mOverlay->comparePipePriority(pipe_info.lIndex,
                pipe_info.rIndex) == -1) {
            qhwc::swap(pipe_info.lIndex, pipe_info.rIndex);
        }
    }

    return true;
}

int MDPCompSrcSplit::configure(hwc_context_t *ctx, hwc_layer_1_t *layer,
        PipeLayerPair& PipeLayerPair) {
    private_handle_t *hnd = (private_handle_t *)layer->handle;
    if(!hnd) {
        ALOGE("%s: layer handle is NULL", __FUNCTION__);
        return -1;
    }
    MetaData_t *metadata = (MetaData_t *)hnd->base_metadata;
    MdpPipeInfoSplit& mdp_info =
        *(static_cast<MdpPipeInfoSplit*>(PipeLayerPair.pipeInfo));
    Rotator **rot = &PipeLayerPair.rot;
    eZorder z = static_cast<eZorder>(mdp_info.zOrder);
    eIsFg isFg = IS_FG_OFF;
    eDest lDest = mdp_info.lIndex;
    eDest rDest = mdp_info.rIndex;
    hwc_rect_t crop = integerizeSourceCrop(layer->sourceCropf);
    hwc_rect_t dst = layer->displayFrame;
    int transform = layer->transform;
    eTransform orient = static_cast<eTransform>(transform);
    const int downscale = 0;
    int rotFlags = ROT_FLAGS_NONE;
    uint32_t format = ovutils::getMdpFormat(hnd->format, isTileRendered(hnd));
    Whf whf(getWidth(hnd), getHeight(hnd), format, hnd->size);

    ALOGD_IF(isDebug(),"%s: configuring: layer: %p z_order: %d dest_pipeL: %d"
             "dest_pipeR: %d",__FUNCTION__, layer, z, lDest, rDest);

    // Handle R/B swap
    if (layer->flags & HWC_FORMAT_RB_SWAP) {
        if (hnd->format == HAL_PIXEL_FORMAT_RGBA_8888)
            whf.format = getMdpFormat(HAL_PIXEL_FORMAT_BGRA_8888);
        else if (hnd->format == HAL_PIXEL_FORMAT_RGBX_8888)
            whf.format = getMdpFormat(HAL_PIXEL_FORMAT_BGRX_8888);
    }

    eMdpFlags mdpFlags = OV_MDP_BACKEND_COMPOSITION;
    setMdpFlags(ctx, layer, mdpFlags, 0, transform);

    if(lDest != OV_INVALID && rDest != OV_INVALID) {
        //Enable overfetch
        setMdpFlags(mdpFlags, OV_MDSS_MDP_DUAL_PIPE);
    }

    if(has90Transform(layer) && isRotationDoable(ctx, hnd)) {
        (*rot) = ctx->mRotMgr->getNext();
        if((*rot) == NULL) return -1;
        ctx->mLayerRotMap[mDpy]->add(layer, *rot);
        //If the video is using a single pipe, enable BWC
        if(rDest == OV_INVALID) {
            BwcPM::setBwc(crop, dst, transform, mdpFlags);
        }
        //Configure rotator for pre-rotation
        if(configRotator(*rot, whf, crop, mdpFlags, orient, downscale) < 0) {
            ALOGE("%s: configRotator failed!", __FUNCTION__);
            return -1;
        }
        updateSource(orient, whf, crop, *rot);
        rotFlags |= ROT_PREROTATED;
    }

    //If 2 pipes being used, divide layer into half, crop and dst
    hwc_rect_t cropL = crop;
    hwc_rect_t cropR = crop;
    hwc_rect_t dstL = dst;
    hwc_rect_t dstR = dst;
    if(lDest != OV_INVALID && rDest != OV_INVALID) {
        cropL.right = (crop.right + crop.left) / 2;
        cropR.left = cropL.right;
        sanitizeSourceCrop(cropL, cropR, hnd);

        //Swap crops on H flip since 2 pipes are being used
        if((orient & OVERLAY_TRANSFORM_FLIP_H) && (*rot) == NULL) {
            hwc_rect_t tmp = cropL;
            cropL = cropR;
            cropR = tmp;
        }

        dstL.right = (dst.right + dst.left) / 2;
        dstR.left = dstL.right;
    }

    //For the mdp, since either we are pre-rotating or MDP does flips
    orient = OVERLAY_TRANSFORM_0;
    transform = 0;

    //configure left pipe
    if(lDest != OV_INVALID) {
        PipeArgs pargL(mdpFlags, whf, z, isFg,
                static_cast<eRotFlags>(rotFlags), layer->planeAlpha,
                (ovutils::eBlending) getBlending(layer->blending));

        if(configMdp(ctx->mOverlay, pargL, orient,
                    cropL, dstL, metadata, lDest) < 0) {
            ALOGE("%s: commit failed for left mixer config", __FUNCTION__);
            return -1;
        }
    }

    //configure right pipe
    if(rDest != OV_INVALID) {
        PipeArgs pargR(mdpFlags, whf, z, isFg,
                static_cast<eRotFlags>(rotFlags),
                layer->planeAlpha,
                (ovutils::eBlending) getBlending(layer->blending));
        if(configMdp(ctx->mOverlay, pargR, orient,
                    cropR, dstR, metadata, rDest) < 0) {
            ALOGE("%s: commit failed for right mixer config", __FUNCTION__);
            return -1;
        }
    }

    return 0;
}

}; //namespace

