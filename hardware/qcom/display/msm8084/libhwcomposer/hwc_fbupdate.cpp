/*
 * Copyright (C) 2010 The Android Open Source Project
 * Copyright (C) 2012-2014, The Linux Foundation. All rights reserved.
 *
 * Not a Contribution, Apache license notifications and license are
 * retained for attribution purposes only.
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

#define DEBUG_FBUPDATE 0
#include <cutils/properties.h>
#include <gralloc_priv.h>
#include <overlay.h>
#include <overlayRotator.h>
#include "hwc_fbupdate.h"
#include "mdp_version.h"
#include "external.h"
#include "virtual.h"

using namespace qdutils;
using namespace overlay;
using overlay::Rotator;
using namespace overlay::utils;

namespace qhwc {

namespace ovutils = overlay::utils;

IFBUpdate* IFBUpdate::getObject(hwc_context_t *ctx, const int& dpy) {
    if(qdutils::MDPVersion::getInstance().isSrcSplit()) {
        return new FBSrcSplit(ctx, dpy);
    } else if(isDisplaySplit(ctx, dpy)) {
        return new FBUpdateSplit(ctx, dpy);
    }
    return new FBUpdateNonSplit(ctx, dpy);
}

IFBUpdate::IFBUpdate(hwc_context_t *ctx, const int& dpy) : mDpy(dpy) {
    size_t size = 0;
    getBufferAttributes(ctx->dpyAttr[mDpy].xres,
            ctx->dpyAttr[mDpy].yres,
            HAL_PIXEL_FORMAT_RGBA_8888,
            0,
            mAlignedFBWidth,
            mAlignedFBHeight,
            mTileEnabled, size);
}

void IFBUpdate::reset() {
    mModeOn = false;
    mRot = NULL;
}

bool IFBUpdate::prepareAndValidate(hwc_context_t *ctx,
            hwc_display_contents_1 *list, int fbZorder) {
    hwc_layer_1_t *layer = &list->hwLayers[list->numHwLayers - 1];
    mModeOn = prepare(ctx, list, layer->displayFrame, fbZorder) &&
            ctx->mOverlay->validateAndSet(mDpy, ctx->dpyAttr[mDpy].fd);
    return mModeOn;
}

//================= Low res====================================
FBUpdateNonSplit::FBUpdateNonSplit(hwc_context_t *ctx, const int& dpy):
        IFBUpdate(ctx, dpy) {}

void FBUpdateNonSplit::reset() {
    IFBUpdate::reset();
    mDest = ovutils::OV_INVALID;
}

bool FBUpdateNonSplit::preRotateExtDisplay(hwc_context_t *ctx,
                                            hwc_layer_1_t *layer,
                                            ovutils::Whf &info,
                                            hwc_rect_t& sourceCrop,
                                            ovutils::eMdpFlags& mdpFlags,
                                            int& rotFlags)
{
    int extOrient = getExtOrientation(ctx);
    ovutils::eTransform orient = static_cast<ovutils::eTransform >(extOrient);
    if(mDpy && (extOrient & HWC_TRANSFORM_ROT_90)) {
        mRot = ctx->mRotMgr->getNext();
        if(mRot == NULL) return false;
        ctx->mLayerRotMap[mDpy]->add(layer, mRot);
        // Composed FB content will have black bars, if the viewFrame of the
        // external is different from {0, 0, fbWidth, fbHeight}, so intersect
        // viewFrame with sourceCrop to avoid those black bars
        sourceCrop = getIntersection(sourceCrop, ctx->mViewFrame[mDpy]);
        //Configure rotator for pre-rotation
        if(configRotator(mRot, info, sourceCrop, mdpFlags, orient, 0) < 0) {
            ALOGE("%s: configRotator Failed!", __FUNCTION__);
            mRot = NULL;
            return false;
        }
        info.format = (mRot)->getDstFormat();
        updateSource(orient, info, sourceCrop);
        rotFlags |= ovutils::ROT_PREROTATED;
    }
    return true;
}

bool FBUpdateNonSplit::prepare(hwc_context_t *ctx, hwc_display_contents_1 *list,
                             hwc_rect_t fbUpdatingRect, int fbZorder) {
    if(!ctx->mMDP.hasOverlay) {
        ALOGD_IF(DEBUG_FBUPDATE, "%s, this hw doesnt support overlays",
                 __FUNCTION__);
        return false;
    }
    mModeOn = configure(ctx, list, fbUpdatingRect, fbZorder);
    return mModeOn;
}

// Configure
bool FBUpdateNonSplit::configure(hwc_context_t *ctx, hwc_display_contents_1 *list,
                               hwc_rect_t fbUpdatingRect, int fbZorder) {
    bool ret = false;
    hwc_layer_1_t *layer = &list->hwLayers[list->numHwLayers - 1];
    if (LIKELY(ctx->mOverlay)) {
        int extOnlyLayerIndex = ctx->listStats[mDpy].extOnlyLayerIndex;
        // ext only layer present..
        if(extOnlyLayerIndex != -1) {
            layer = &list->hwLayers[extOnlyLayerIndex];
            layer->compositionType = HWC_OVERLAY;
        }
        overlay::Overlay& ov = *(ctx->mOverlay);

        ovutils::Whf info(mAlignedFBWidth, mAlignedFBHeight,
                ovutils::getMdpFormat(HAL_PIXEL_FORMAT_RGBA_8888,
                    mTileEnabled));

        Overlay::PipeSpecs pipeSpecs;
        pipeSpecs.formatClass = Overlay::FORMAT_RGB;
        pipeSpecs.needsScaling = qhwc::needsScaling(layer);
        pipeSpecs.dpy = mDpy;
        pipeSpecs.mixer = Overlay::MIXER_DEFAULT;
        pipeSpecs.fb = true;

        ovutils::eDest dest = ov.getPipe(pipeSpecs);
        if(dest == ovutils::OV_INVALID) { //None available
            ALOGE("%s: No pipes available to configure fb for dpy %d",
                __FUNCTION__, mDpy);
            return false;
        }
        mDest = dest;

        if((mDpy && ctx->deviceOrientation) &&
            ctx->listStats[mDpy].isDisplayAnimating) {
            fbZorder = 0;
        }

        ovutils::eMdpFlags mdpFlags = ovutils::OV_MDP_BLEND_FG_PREMULT;
        ovutils::eIsFg isFg = ovutils::IS_FG_OFF;
        ovutils::eZorder zOrder = static_cast<ovutils::eZorder>(fbZorder);

        hwc_rect_t sourceCrop = integerizeSourceCrop(layer->sourceCropf);
        hwc_rect_t displayFrame = layer->displayFrame;

        // No FB update optimization on (1) Custom FB resolution,
        // (2) External Mirror mode, (3) External orientation
        if(!ctx->dpyAttr[mDpy].customFBSize && !ctx->mBufferMirrorMode
           && !ctx->mExtOrientation) {
            sourceCrop = fbUpdatingRect;
            displayFrame = fbUpdatingRect;
        }

        int transform = layer->transform;
        int rotFlags = ovutils::ROT_FLAGS_NONE;

        ovutils::eTransform orient =
                    static_cast<ovutils::eTransform>(transform);
        // use ext orientation if any
        int extOrient = getExtOrientation(ctx);

        // Do not use getNonWormholeRegion() function to calculate the
        // sourceCrop during animation on external display and
        // Dont do wormhole calculation when extorientation is set on External
        // Dont do wormhole calculation when extDownscale is enabled on External
        if(ctx->listStats[mDpy].isDisplayAnimating && mDpy) {
            sourceCrop = layer->displayFrame;
        } else if((!mDpy ||
                  (mDpy && !extOrient
                  && !ctx->dpyAttr[mDpy].mDownScaleMode))
                  && (extOnlyLayerIndex == -1)) {
            if(!qdutils::MDPVersion::getInstance().is8x26() &&
                !ctx->dpyAttr[mDpy].customFBSize) {
                getNonWormholeRegion(list, sourceCrop);
                displayFrame = sourceCrop;
            }
        }
        calcExtDisplayPosition(ctx, NULL, mDpy, sourceCrop, displayFrame,
                                   transform, orient);
        //Store the displayFrame, will be used in getDisplayViewFrame
        ctx->dpyAttr[mDpy].mDstRect = displayFrame;
        setMdpFlags(layer, mdpFlags, 0, transform);
        // For External use rotator if there is a rotation value set
        ret = preRotateExtDisplay(ctx, layer, info,
                sourceCrop, mdpFlags, rotFlags);
        if(!ret) {
            ALOGE("%s: preRotate for external Failed!", __FUNCTION__);
            return false;
        }
        //For the mdp, since either we are pre-rotating or MDP does flips
        orient = ovutils::OVERLAY_TRANSFORM_0;
        transform = 0;
        ovutils::PipeArgs parg(mdpFlags, info, zOrder, isFg,
                               static_cast<ovutils::eRotFlags>(rotFlags),
                               ovutils::DEFAULT_PLANE_ALPHA,
                               (ovutils::eBlending)
                               getBlending(layer->blending));
        ret = true;
        if(configMdp(ctx->mOverlay, parg, orient, sourceCrop, displayFrame,
                    NULL, mDest) < 0) {
            ALOGE("%s: configMdp failed for dpy %d", __FUNCTION__, mDpy);
            ret = false;
        }
    }
    return ret;
}

bool FBUpdateNonSplit::draw(hwc_context_t *ctx, private_handle_t *hnd)
{
    if(!mModeOn) {
        return true;
    }
    bool ret = true;
    overlay::Overlay& ov = *(ctx->mOverlay);
    ovutils::eDest dest = mDest;
    int fd = hnd->fd;
    uint32_t offset = (uint32_t)hnd->offset;
    if(mRot) {
        if(!mRot->queueBuffer(fd, offset))
            return false;
        fd = mRot->getDstMemId();
        offset = mRot->getDstOffset();
    }
    if (!ov.queueBuffer(fd, offset, dest)) {
        ALOGE("%s: queueBuffer failed for FBUpdate", __FUNCTION__);
        ret = false;
    }
    return ret;
}

//================= High res====================================
FBUpdateSplit::FBUpdateSplit(hwc_context_t *ctx, const int& dpy):
        IFBUpdate(ctx, dpy) {}

void FBUpdateSplit::reset() {
    IFBUpdate::reset();
    mDestLeft = ovutils::OV_INVALID;
    mDestRight = ovutils::OV_INVALID;
    mRot = NULL;
}

bool FBUpdateSplit::prepare(hwc_context_t *ctx, hwc_display_contents_1 *list,
                              hwc_rect_t fbUpdatingRect, int fbZorder) {
    if(!ctx->mMDP.hasOverlay) {
        ALOGD_IF(DEBUG_FBUPDATE, "%s, this hw doesnt support overlays",
                 __FUNCTION__);
        return false;
    }
    ALOGD_IF(DEBUG_FBUPDATE, "%s, mModeOn = %d", __FUNCTION__, mModeOn);
    mModeOn = configure(ctx, list, fbUpdatingRect, fbZorder);
    return mModeOn;
}

// Configure
bool FBUpdateSplit::configure(hwc_context_t *ctx,
        hwc_display_contents_1 *list, hwc_rect_t fbUpdatingRect, int fbZorder) {
    bool ret = false;
    hwc_layer_1_t *layer = &list->hwLayers[list->numHwLayers - 1];
    if (LIKELY(ctx->mOverlay)) {
        /*  External only layer present */
        int extOnlyLayerIndex = ctx->listStats[mDpy].extOnlyLayerIndex;
        if(extOnlyLayerIndex != -1) {
            layer = &list->hwLayers[extOnlyLayerIndex];
            layer->compositionType = HWC_OVERLAY;
        }
        ovutils::Whf info(mAlignedFBWidth, mAlignedFBHeight,
                          ovutils::getMdpFormat(HAL_PIXEL_FORMAT_RGBA_8888,
                                                mTileEnabled));

        overlay::Overlay& ov = *(ctx->mOverlay);
        ovutils::eMdpFlags mdpFlags = ovutils::OV_MDP_BLEND_FG_PREMULT;
        ovutils::eZorder zOrder = static_cast<ovutils::eZorder>(fbZorder);
        ovutils::eTransform orient =
            static_cast<ovutils::eTransform>(layer->transform);
        const int hw_w = ctx->dpyAttr[mDpy].xres;
        const int hw_h = ctx->dpyAttr[mDpy].yres;
        const int lSplit = getLeftSplit(ctx, mDpy);
        mDestLeft = ovutils::OV_INVALID;
        mDestRight = ovutils::OV_INVALID;

        hwc_rect_t sourceCrop = fbUpdatingRect;
        hwc_rect_t displayFrame = fbUpdatingRect;

        ret = true;
        Overlay::PipeSpecs pipeSpecs;
        pipeSpecs.formatClass = Overlay::FORMAT_RGB;
        pipeSpecs.needsScaling = qhwc::needsScaling(layer);
        pipeSpecs.dpy = mDpy;
        pipeSpecs.fb = true;

        /* Configure left pipe */
        if(displayFrame.left < lSplit) {
            pipeSpecs.mixer = Overlay::MIXER_LEFT;
            ovutils::eDest destL = ov.getPipe(pipeSpecs);
            if(destL == ovutils::OV_INVALID) { //None available
                ALOGE("%s: No pipes available to configure fb for dpy %d's left"
                      " mixer", __FUNCTION__, mDpy);
                return false;
            }

            mDestLeft = destL;

            //XXX: FB layer plane alpha is currently sent as zero from
            //surfaceflinger
            ovutils::PipeArgs pargL(mdpFlags,
                                    info,
                                    zOrder,
                                    ovutils::IS_FG_OFF,
                                    ovutils::ROT_FLAGS_NONE,
                                    ovutils::DEFAULT_PLANE_ALPHA,
                                    (ovutils::eBlending)
                                    getBlending(layer->blending));
            hwc_rect_t cropL = sourceCrop;
            hwc_rect_t dstL = displayFrame;
            hwc_rect_t scissorL = {0, 0, lSplit, hw_h };
            qhwc::calculate_crop_rects(cropL, dstL, scissorL, 0);

            if (configMdp(ctx->mOverlay, pargL, orient, cropL,
                           dstL, NULL, destL)< 0) {
                ALOGE("%s: configMdp fails for left FB", __FUNCTION__);
                ret = false;
            }
        }

        /* Configure right pipe */
        if(displayFrame.right > lSplit) {
            pipeSpecs.mixer = Overlay::MIXER_RIGHT;
            ovutils::eDest destR = ov.getPipe(pipeSpecs);
            if(destR == ovutils::OV_INVALID) { //None available
                ALOGE("%s: No pipes available to configure fb for dpy %d's"
                      " right mixer", __FUNCTION__, mDpy);
                return false;
            }

            mDestRight = destR;
            ovutils::eMdpFlags mdpFlagsR = mdpFlags;
            ovutils::setMdpFlags(mdpFlagsR, ovutils::OV_MDSS_MDP_RIGHT_MIXER);

            //XXX: FB layer plane alpha is currently sent as zero from
            //surfaceflinger
            ovutils::PipeArgs pargR(mdpFlagsR,
                                    info,
                                    zOrder,
                                    ovutils::IS_FG_OFF,
                                    ovutils::ROT_FLAGS_NONE,
                                    ovutils::DEFAULT_PLANE_ALPHA,
                                    (ovutils::eBlending)
                                    getBlending(layer->blending));

            hwc_rect_t cropR = sourceCrop;
            hwc_rect_t dstR = displayFrame;
            hwc_rect_t scissorR = {lSplit, 0, hw_w, hw_h };
            qhwc::calculate_crop_rects(cropR, dstR, scissorR, 0);

            dstR.left -= lSplit;
            dstR.right -= lSplit;

            if (configMdp(ctx->mOverlay, pargR, orient, cropR,
                           dstR, NULL, destR) < 0) {
                ALOGE("%s: configMdp fails for right FB", __FUNCTION__);
                ret = false;
            }
        }
    }
    return ret;
}

bool FBUpdateSplit::draw(hwc_context_t *ctx, private_handle_t *hnd)
{
    if(!mModeOn) {
        return true;
    }
    bool ret = true;
    overlay::Overlay& ov = *(ctx->mOverlay);
    if(mDestLeft != ovutils::OV_INVALID) {
        if (!ov.queueBuffer(hnd->fd, (uint32_t)hnd->offset, mDestLeft)) {
            ALOGE("%s: queue failed for left of dpy = %d",
                  __FUNCTION__, mDpy);
            ret = false;
        }
    }
    if(mDestRight != ovutils::OV_INVALID) {
        if (!ov.queueBuffer(hnd->fd, (uint32_t)hnd->offset, mDestRight)) {
            ALOGE("%s: queue failed for right of dpy = %d",
                  __FUNCTION__, mDpy);
            ret = false;
        }
    }
    return ret;
}

//=================FBSrcSplit====================================
FBSrcSplit::FBSrcSplit(hwc_context_t *ctx, const int& dpy):
        FBUpdateSplit(ctx, dpy) {}

bool FBSrcSplit::configure(hwc_context_t *ctx, hwc_display_contents_1 *list,
        hwc_rect_t fbUpdatingRect, int fbZorder) {
    hwc_layer_1_t *layer = &list->hwLayers[list->numHwLayers - 1];
    int extOnlyLayerIndex = ctx->listStats[mDpy].extOnlyLayerIndex;
    // ext only layer present..
    if(extOnlyLayerIndex != -1) {
        layer = &list->hwLayers[extOnlyLayerIndex];
        layer->compositionType = HWC_OVERLAY;
    }

    overlay::Overlay& ov = *(ctx->mOverlay);

    ovutils::Whf info(mAlignedFBWidth,
            mAlignedFBHeight,
            ovutils::getMdpFormat(HAL_PIXEL_FORMAT_RGBA_8888,
                mTileEnabled));

    ovutils::eMdpFlags mdpFlags = OV_MDP_BLEND_FG_PREMULT;
    ovutils::eZorder zOrder = static_cast<ovutils::eZorder>(fbZorder);

    ovutils::PipeArgs parg(mdpFlags,
            info,
            zOrder,
            ovutils::IS_FG_OFF,
            ovutils::ROT_FLAGS_NONE,
            ovutils::DEFAULT_PLANE_ALPHA,
            (ovutils::eBlending)
            getBlending(layer->blending));

    int transform = layer->transform;
    ovutils::eTransform orient =
            static_cast<ovutils::eTransform>(transform);

    hwc_rect_t cropL = fbUpdatingRect;
    hwc_rect_t cropR = fbUpdatingRect;

    //Request left pipe (or 1 by default)
    Overlay::PipeSpecs pipeSpecs;
    pipeSpecs.formatClass = Overlay::FORMAT_RGB;
    pipeSpecs.needsScaling = qhwc::needsScaling(layer);
    pipeSpecs.dpy = mDpy;
    pipeSpecs.mixer = Overlay::MIXER_DEFAULT;
    pipeSpecs.fb = true;
    ovutils::eDest destL = ov.getPipe(pipeSpecs);
    if(destL == ovutils::OV_INVALID) {
        ALOGE("%s: No pipes available to configure fb for dpy %d's left"
                " mixer", __FUNCTION__, mDpy);
        return false;
    }

    ovutils::eDest destR = ovutils::OV_INVALID;

    /*  Use 2 pipes IF
        a) FB's width is > 2048 or
        b) On primary, driver has indicated with caps to split always. This is
           based on an empirically derived value of panel height.
    */

    bool primarySplitAlways = (mDpy == HWC_DISPLAY_PRIMARY) and
            qdutils::MDPVersion::getInstance().isSrcSplitAlways();
    int lSplit = getLeftSplit(ctx, mDpy);

    if(((fbUpdatingRect.right - fbUpdatingRect.left) >
            qdutils::MAX_DISPLAY_DIM) or
            ((fbUpdatingRect.right - fbUpdatingRect.left) > lSplit and
                primarySplitAlways)) {
        destR = ov.getPipe(pipeSpecs);
        if(destR == ovutils::OV_INVALID) {
            ALOGE("%s: No pipes available to configure fb for dpy %d's right"
                    " mixer", __FUNCTION__, mDpy);
            return false;
        }

        if(ctx->mOverlay->comparePipePriority(destL, destR) == -1) {
            qhwc::swap(destL, destR);
        }

        //Split crop equally when using 2 pipes
        cropL.right = (fbUpdatingRect.right + fbUpdatingRect.left) / 2;
        cropR.left = cropL.right;
    }

    mDestLeft = destL;
    mDestRight = destR;

    if(destL != OV_INVALID) {
        if(configMdp(ctx->mOverlay, parg, orient,
                    cropL, cropL, NULL /*metadata*/, destL) < 0) {
            ALOGE("%s: commit failed for left mixer config", __FUNCTION__);
            return false;
        }
    }

    //configure right pipe
    if(destR != OV_INVALID) {
        if(configMdp(ctx->mOverlay, parg, orient,
                    cropR, cropR, NULL /*metadata*/, destR) < 0) {
            ALOGE("%s: commit failed for right mixer config", __FUNCTION__);
            return false;
        }
    }

    return true;
}

//---------------------------------------------------------------------
}; //namespace qhwc
