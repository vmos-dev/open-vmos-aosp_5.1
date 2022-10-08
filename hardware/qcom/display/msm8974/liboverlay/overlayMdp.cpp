/*
* Copyright (C) 2008 The Android Open Source Project
* Copyright (c) 2010-2013, The Linux Foundation. All rights reserved.
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
#include <mdp_version.h>
#include "overlayUtils.h"
#include "overlayMdp.h"
#include "mdp_version.h"

#define HSIC_SETTINGS_DEBUG 0

using namespace qdutils;

static inline bool isEqual(float f1, float f2) {
        return ((int)(f1*100) == (int)(f2*100)) ? true : false;
}

namespace ovutils = overlay::utils;
namespace overlay {

bool MdpCtrl::init(uint32_t fbnum) {
    // FD init
    if(!utils::openDev(mFd, fbnum,
                Res::fbPath, O_RDWR)){
        ALOGE("Ctrl failed to init fbnum=%d", fbnum);
        return false;
    }
    return true;
}

void MdpCtrl::reset() {
    utils::memset0(mOVInfo);
    utils::memset0(mLkgo);
    mOVInfo.id = MSMFB_NEW_REQUEST;
    mLkgo.id = MSMFB_NEW_REQUEST;
    mOrientation = utils::OVERLAY_TRANSFORM_0;
    mDownscale = 0;
    mForceSet = false;
#ifdef USES_POST_PROCESSING
    mPPChanged = false;
    memset(&mParams, 0, sizeof(struct compute_params));
    mParams.params.conv_params.order = hsic_order_hsc_i;
    mParams.params.conv_params.interface = interface_rec601;
    mParams.params.conv_params.cc_matrix[0][0] = 1;
    mParams.params.conv_params.cc_matrix[1][1] = 1;
    mParams.params.conv_params.cc_matrix[2][2] = 1;
#endif
}

bool MdpCtrl::close() {
    bool result = true;
    if(MSMFB_NEW_REQUEST != static_cast<int>(mOVInfo.id)) {
        if(!mdp_wrapper::unsetOverlay(mFd.getFD(), mOVInfo.id)) {
            ALOGE("MdpCtrl close error in unset");
            result = false;
        }
    }
#ifdef USES_POST_PROCESSING
    /* free allocated memory in PP */
    if (mOVInfo.overlay_pp_cfg.igc_cfg.c0_c1_data)
            free(mOVInfo.overlay_pp_cfg.igc_cfg.c0_c1_data);
#endif
    reset();

    if(!mFd.close()) {
        result = false;
    }

    return result;
}

void MdpCtrl::setSource(const utils::PipeArgs& args) {
    setSrcWhf(args.whf);

    //TODO These are hardcoded. Can be moved out of setSource.
    mOVInfo.transp_mask = 0xffffffff;

    //TODO These calls should ideally be a part of setPipeParams API
    setFlags(args.mdpFlags);
    setZ(args.zorder);
    setIsFg(args.isFg);
    setPlaneAlpha(args.planeAlpha);
    setBlending(args.blending);
}

void MdpCtrl::setCrop(const utils::Dim& d) {
    setSrcRectDim(d);
}

void MdpCtrl::setPosition(const overlay::utils::Dim& d) {
    setDstRectDim(d);
}

void MdpCtrl::setTransform(const utils::eTransform& orient) {
    int rot = utils::getMdpOrient(orient);
    setUserData(rot);
    mOrientation = static_cast<utils::eTransform>(rot);
}

void MdpCtrl::doTransform() {
    setRotationFlags();
    utils::Whf whf = getSrcWhf();
    utils::Dim dim = getSrcRectDim();
    utils::preRotateSource(mOrientation, whf, dim);
    setSrcWhf(whf);
    setSrcRectDim(dim);
}

void MdpCtrl::doDownscale() {
    int mdpVersion = MDPVersion::getInstance().getMDPVersion();
    if(mdpVersion < MDSS_V5) {
        mOVInfo.src_rect.x >>= mDownscale;
        mOVInfo.src_rect.y >>= mDownscale;
        mOVInfo.src_rect.w >>= mDownscale;
        mOVInfo.src_rect.h >>= mDownscale;
    } else if(MDPVersion::getInstance().supportsDecimation()) {
        utils::getDecimationFactor(mOVInfo.src_rect.w, mOVInfo.src_rect.h,
                mOVInfo.dst_rect.w, mOVInfo.dst_rect.h, mOVInfo.horz_deci,
                mOVInfo.vert_deci);
    }
}

bool MdpCtrl::set() {
    int mdpVersion = MDPVersion::getInstance().getMDPVersion();
    //deferred calcs, so APIs could be called in any order.
    doTransform();
    utils::Whf whf = getSrcWhf();
    if(utils::isYuv(whf.format)) {
        utils::normalizeCrop(mOVInfo.src_rect.x, mOVInfo.src_rect.w);
        utils::normalizeCrop(mOVInfo.src_rect.y, mOVInfo.src_rect.h);
        if(mdpVersion < MDSS_V5) {
            utils::even_floor(mOVInfo.dst_rect.w);
            utils::even_floor(mOVInfo.dst_rect.h);
        } else if (mOVInfo.flags & MDP_DEINTERLACE) {
            // For interlaced, crop.h should be 4-aligned
            if (!(mOVInfo.flags & MDP_SOURCE_ROTATED_90) &&
                (mOVInfo.src_rect.h % 4))
                mOVInfo.src_rect.h = utils::aligndown(mOVInfo.src_rect.h, 4);
        }
    } else {
        if (mdpVersion >= MDSS_V5) {
            // Check for 1-pixel down-scaling
            if (mOVInfo.src_rect.w - mOVInfo.dst_rect.w == 1)
                mOVInfo.src_rect.w -= 1;
            if (mOVInfo.src_rect.h - mOVInfo.dst_rect.h == 1)
                mOVInfo.src_rect.h -= 1;
        }
    }

    doDownscale();

    if(this->ovChanged() || mForceSet) {
        mForceSet = false;
        if(!mdp_wrapper::setOverlay(mFd.getFD(), mOVInfo)) {
            ALOGE("MdpCtrl failed to setOverlay, restoring last known "
                  "good ov info");
            mdp_wrapper::dump("== Bad OVInfo is: ", mOVInfo);
            mdp_wrapper::dump("== Last good known OVInfo is: ", mLkgo);
            this->restore();
            return false;
        }
        this->save();
    }

    return true;
}

bool MdpCtrl::get() {
    mdp_overlay ov;
    ov.id = mOVInfo.id;
    if (!mdp_wrapper::getOverlay(mFd.getFD(), ov)) {
        ALOGE("MdpCtrl get failed");
        return false;
    }
    mOVInfo = ov;
    return true;
}

//Update src format based on rotator's destination format.
void MdpCtrl::updateSrcFormat(const uint32_t& rotDestFmt) {
    utils::Whf whf = getSrcWhf();
    whf.format =  rotDestFmt;
    setSrcWhf(whf);
}

void MdpCtrl::dump() const {
    ALOGE("== Dump MdpCtrl start ==");
    mFd.dump();
    mdp_wrapper::dump("mOVInfo", mOVInfo);
    ALOGE("== Dump MdpCtrl end ==");
}

void MdpCtrl::getDump(char *buf, size_t len) {
    ovutils::getDump(buf, len, "Ctrl", mOVInfo);
}

void MdpData::dump() const {
    ALOGE("== Dump MdpData start ==");
    mFd.dump();
    mdp_wrapper::dump("mOvData", mOvData);
    ALOGE("== Dump MdpData end ==");
}

void MdpData::getDump(char *buf, size_t len) {
    ovutils::getDump(buf, len, "Data", mOvData);
}

void MdpCtrl3D::dump() const {
    ALOGE("== Dump MdpCtrl start ==");
    mFd.dump();
    ALOGE("== Dump MdpCtrl end ==");
}

bool MdpCtrl::setVisualParams(const MetaData_t& data) {
    bool needUpdate = false;
#ifdef USES_POST_PROCESSING
    /* calculate the data */
    if (data.operation & PP_PARAM_HSIC) {
        if (mParams.params.pa_params.hue != data.hsicData.hue) {
            ALOGD_IF(HSIC_SETTINGS_DEBUG,
                "Hue has changed from %d to %d",
                mParams.params.pa_params.hue,data.hsicData.hue);
            needUpdate = true;
        }

        if (!isEqual(mParams.params.pa_params.sat,
            data.hsicData.saturation)) {
            ALOGD_IF(HSIC_SETTINGS_DEBUG,
                "Saturation has changed from %f to %f",
                mParams.params.pa_params.sat,
                data.hsicData.saturation);
            needUpdate = true;
        }

        if (mParams.params.pa_params.intensity != data.hsicData.intensity) {
            ALOGD_IF(HSIC_SETTINGS_DEBUG,
                "Intensity has changed from %d to %d",
                mParams.params.pa_params.intensity,
                data.hsicData.intensity);
            needUpdate = true;
        }

        if (!isEqual(mParams.params.pa_params.contrast,
            data.hsicData.contrast)) {
            ALOGD_IF(HSIC_SETTINGS_DEBUG,
                "Contrast has changed from %f to %f",
                mParams.params.pa_params.contrast,
                data.hsicData.contrast);
            needUpdate = true;
        }

        if (needUpdate) {
            mParams.params.pa_params.hue = data.hsicData.hue;
            mParams.params.pa_params.sat = data.hsicData.saturation;
            mParams.params.pa_params.intensity = data.hsicData.intensity;
            mParams.params.pa_params.contrast = data.hsicData.contrast;
            mParams.params.pa_params.ops = MDP_PP_OPS_WRITE | MDP_PP_OPS_ENABLE;
            mParams.operation |= PP_OP_PA;
        }
    }

    if (data.operation & PP_PARAM_SHARP2) {
        if (mParams.params.sharp_params.strength != data.Sharp2Data.strength) {
            needUpdate = true;
        }
        if (mParams.params.sharp_params.edge_thr != data.Sharp2Data.edge_thr) {
            needUpdate = true;
        }
        if (mParams.params.sharp_params.smooth_thr !=
                data.Sharp2Data.smooth_thr) {
            needUpdate = true;
        }
        if (mParams.params.sharp_params.noise_thr !=
                data.Sharp2Data.noise_thr) {
            needUpdate = true;
        }

        if (needUpdate) {
            mParams.params.sharp_params.strength = data.Sharp2Data.strength;
            mParams.params.sharp_params.edge_thr = data.Sharp2Data.edge_thr;
            mParams.params.sharp_params.smooth_thr =
                data.Sharp2Data.smooth_thr;
            mParams.params.sharp_params.noise_thr = data.Sharp2Data.noise_thr;
            mParams.params.sharp_params.ops =
                MDP_PP_OPS_WRITE | MDP_PP_OPS_ENABLE;
            mParams.operation |= PP_OP_SHARP;
        }
    }

    if (data.operation & PP_PARAM_IGC) {
        if (mOVInfo.overlay_pp_cfg.igc_cfg.c0_c1_data == NULL){
            uint32_t *igcData
                = (uint32_t *)malloc(2 * MAX_IGC_LUT_ENTRIES * sizeof(uint32_t));
            if (!igcData) {
                ALOGE("IGC storage allocated failed");
                return false;
            }
            mOVInfo.overlay_pp_cfg.igc_cfg.c0_c1_data = igcData;
            mOVInfo.overlay_pp_cfg.igc_cfg.c2_data
                = igcData + MAX_IGC_LUT_ENTRIES;
        }

        memcpy(mParams.params.igc_lut_params.c0,
            data.igcData.c0, sizeof(uint16_t) * MAX_IGC_LUT_ENTRIES);
        memcpy(mParams.params.igc_lut_params.c1,
            data.igcData.c1, sizeof(uint16_t) * MAX_IGC_LUT_ENTRIES);
        memcpy(mParams.params.igc_lut_params.c2,
            data.igcData.c2, sizeof(uint16_t) * MAX_IGC_LUT_ENTRIES);

        mParams.params.igc_lut_params.ops
            = MDP_PP_OPS_WRITE | MDP_PP_OPS_ENABLE;
        mParams.operation |= PP_OP_IGC;
        needUpdate = true;
    }

    if (data.operation & PP_PARAM_VID_INTFC) {
        mParams.params.conv_params.interface =
            (interface_type) data.video_interface;
        needUpdate = true;
    }

    if (needUpdate) {
        display_pp_compute_params(&mParams, &mOVInfo.overlay_pp_cfg);
        mPPChanged = true;
    }
#endif
    return true;
}

} // overlay
