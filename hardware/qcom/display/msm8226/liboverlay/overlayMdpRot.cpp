/*
 * Copyright (C) 2008 The Android Open Source Project
 * Copyright (c) 2010-2014, The Linux Foundation. All rights reserved.
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

#include "overlayUtils.h"
#include "overlayRotator.h"
#include "gr.h"

namespace ovutils = overlay::utils;

namespace overlay {

MdpRot::MdpRot() {
    reset();
    init();
}

MdpRot::~MdpRot() { close(); }

bool MdpRot::enabled() const { return mRotImgInfo.enable; }

void MdpRot::setRotations(uint32_t r) { mRotImgInfo.rotations = (uint8_t)r; }

int MdpRot::getDstMemId() const {
    return mRotDataInfo.dst.memory_id;
}

uint32_t MdpRot::getDstOffset() const {
    return mRotDataInfo.dst.offset;
}

uint32_t MdpRot::getDstFormat() const {
    return mRotImgInfo.dst.format;
}

//Added for completeness. Not expected to be called.
utils::Whf MdpRot::getDstWhf() const {
    int alW = 0, alH = 0;
    int halFormat = ovutils::getHALFormat(mRotImgInfo.dst.format);
    getBufferSizeAndDimensions(mRotImgInfo.dst.width, mRotImgInfo.dst.height,
            halFormat, alW, alH);
    return utils::Whf(alW, alH, mRotImgInfo.dst.format);
}

//Added for completeness. Not expected to be called.
utils::Dim MdpRot::getDstDimensions() const {
    int alW = 0, alH = 0;
    int halFormat = ovutils::getHALFormat(mRotImgInfo.dst.format);
    getBufferSizeAndDimensions(mRotImgInfo.dst.width, mRotImgInfo.dst.height,
            halFormat, alW, alH);
    return utils::Dim(0, 0, alW, alH);
}

uint32_t MdpRot::getSessId() const { return mRotImgInfo.session_id; }

void MdpRot::setDownscale(int ds) {
    if ((utils::ROT_DS_EIGHTH == ds) && (mRotImgInfo.src_rect.h & 0xF)) {
        // Ensure src_rect.h is a multiple of 16 for 1/8 downscaling.
        // This is an undocumented MDP Rotator constraint.
        mRotImgInfo.src_rect.h = utils::aligndown(mRotImgInfo.src_rect.h, 16);
    }
    mRotImgInfo.downscale_ratio = ds;
}

void MdpRot::save() {
    mLSRotImgInfo = mRotImgInfo;
}

bool MdpRot::rotConfChanged() const {
    // 0 means same
    if(0 == ::memcmp(&mRotImgInfo, &mLSRotImgInfo,
                sizeof (msm_rotator_img_info))) {
        return false;
    }
    return true;
}

bool MdpRot::init()
{
    if(!mFd.open(Res::rotPath, O_RDWR)){
        ALOGE("MdpRot failed to init %s", Res::rotPath);
        return false;
    }
    return true;
}

void MdpRot::setSource(const overlay::utils::Whf& awhf) {
    utils::Whf whf(awhf);
    mRotImgInfo.src.format = whf.format;

    mRotImgInfo.src.width = whf.w;
    mRotImgInfo.src.height = whf.h;

    mRotImgInfo.src_rect.w = whf.w;
    mRotImgInfo.src_rect.h = whf.h;

    mRotImgInfo.dst.width = whf.w;
    mRotImgInfo.dst.height = whf.h;
}

void MdpRot::setCrop(const utils::Dim& /*crop*/) {
    // NO-OP for non-mdss rotator due to possible h/w limitations
}

void MdpRot::setFlags(const utils::eMdpFlags& flags) {
    mRotImgInfo.secure = 0;
    if(flags & utils::OV_MDP_SECURE_OVERLAY_SESSION)
        mRotImgInfo.secure = 1;
}

void MdpRot::setTransform(const utils::eTransform& rot)
{
    int r = utils::getMdpOrient(rot);
    setRotations(r);
    mOrientation = static_cast<utils::eTransform>(r);
    ALOGE_IF(DEBUG_OVERLAY, "%s: r=%d", __FUNCTION__, r);
}

void MdpRot::doTransform() {
    if(mOrientation & utils::OVERLAY_TRANSFORM_ROT_90)
        utils::swap(mRotImgInfo.dst.width, mRotImgInfo.dst.height);
}

bool MdpRot::commit() {
    doTransform();
    if(rotConfChanged()) {
        mRotImgInfo.enable = 1;
        if(!overlay::mdp_wrapper::startRotator(mFd.getFD(), mRotImgInfo)) {
            ALOGE("MdpRot commit failed");
            dump();
            mRotImgInfo.enable = 0;
            return false;
        }
        save();
        mRotDataInfo.session_id = mRotImgInfo.session_id;
    }
    return true;
}

uint32_t MdpRot::calcOutputBufSize() {
    ovutils::Whf destWhf(mRotImgInfo.dst.width,
            mRotImgInfo.dst.height, mRotImgInfo.dst.format);
    return Rotator::calcOutputBufSize(destWhf);
}

bool MdpRot::open_i(uint32_t numbufs, uint32_t bufsz)
{
    OvMem mem;

    OVASSERT(MAP_FAILED == mem.addr(), "MAP failed in open_i");

    if(!mem.open(numbufs, bufsz, mRotImgInfo.secure)){
        ALOGE("%s: Failed to open", __func__);
        mem.close();
        return false;
    }

    OVASSERT(MAP_FAILED != mem.addr(), "MAP failed");
    OVASSERT(mem.getFD() != -1, "getFd is -1");

    mRotDataInfo.dst.memory_id = mem.getFD();
    mRotDataInfo.dst.offset = 0;
    mMem.mem = mem;
    return true;
}

bool MdpRot::close() {
    bool success = true;
    if(mFd.valid() && (getSessId() != 0)) {
        if(!mdp_wrapper::endRotator(mFd.getFD(), getSessId())) {
            ALOGE("Mdp Rot error endRotator, fd=%d sessId=%u",
                    mFd.getFD(), getSessId());
            success = false;
        }
    }
    if (!mFd.close()) {
        ALOGE("Mdp Rot error closing fd");
        success = false;
    }
    if (!mMem.close()) {
        ALOGE("Mdp Rot error closing mem");
        success = false;
    }
    reset();
    return success;
}

bool MdpRot::remap(uint32_t numbufs) {
    // if current size changed, remap
    uint32_t opBufSize = calcOutputBufSize();
    if(opBufSize == mMem.size()) {
        ALOGE_IF(DEBUG_OVERLAY, "%s: same size %d", __FUNCTION__, opBufSize);
        return true;
    }

    if(!mMem.close()) {
        ALOGE("%s error in closing prev rot mem", __FUNCTION__);
        return false;
    }

    ALOGE_IF(DEBUG_OVERLAY, "%s: size changed - remapping", __FUNCTION__);

    if(!open_i(numbufs, opBufSize)) {
        ALOGE("%s Error could not open", __FUNCTION__);
        return false;
    }

    for (uint32_t i = 0; i < numbufs; ++i) {
        mMem.mRotOffset[i] = i * opBufSize;
    }

    return true;
}

void MdpRot::reset() {
    ovutils::memset0(mRotImgInfo);
    ovutils::memset0(mLSRotImgInfo);
    ovutils::memset0(mRotDataInfo);
    ovutils::memset0(mMem.mRotOffset);
    mMem.mCurrIndex = 0;
    mOrientation = utils::OVERLAY_TRANSFORM_0;
}

bool MdpRot::queueBuffer(int fd, uint32_t offset) {
    if(enabled()) {
        mRotDataInfo.src.memory_id = fd;
        mRotDataInfo.src.offset = offset;

        if(false == remap(RotMem::ROT_NUM_BUFS)) {
            ALOGE("%s Remap failed, not queueing", __FUNCTION__);
            return false;
        }

        mRotDataInfo.dst.offset =
                mMem.mRotOffset[mMem.mCurrIndex];
        mMem.mCurrIndex =
                (mMem.mCurrIndex + 1) % mMem.mem.numBufs();

        if(!overlay::mdp_wrapper::rotate(mFd.getFD(), mRotDataInfo)) {
            ALOGE("MdpRot failed rotate");
            dump();
            return false;
        }
    }
    return true;
}

void MdpRot::dump() const {
    ALOGE("== Dump MdpRot start ==");
    mFd.dump();
    mMem.mem.dump();
    mdp_wrapper::dump("mRotImgInfo", mRotImgInfo);
    mdp_wrapper::dump("mRotDataInfo", mRotDataInfo);
    ALOGE("== Dump MdpRot end ==");
}

void MdpRot::getDump(char *buf, size_t len) const {
    ovutils::getDump(buf, len, "MdpRotCtrl", mRotImgInfo);
    ovutils::getDump(buf, len, "MdpRotData", mRotDataInfo);
}

} // namespace overlay
