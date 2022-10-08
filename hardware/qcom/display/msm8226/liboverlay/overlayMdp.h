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

#ifndef OVERLAY_MDP_H
#define OVERLAY_MDP_H

#include <linux/msm_mdp.h>

#include "overlayUtils.h"
#include "mdpWrapper.h"
#include "qdMetaData.h"
#ifdef USES_POST_PROCESSING
#include "lib-postproc.h"
#endif

namespace overlay{

/*
* Mdp Ctrl holds corresponding fd and MDP related struct.
* It is simple wrapper to MDP services
* */
class MdpCtrl {
public:
    /* ctor reset */
    explicit MdpCtrl(const int& dpy);
    /* dtor close */
    ~MdpCtrl();
    /* init underlying device using fbnum for dpy */
    bool init(const int& dpy);
    /* unset overlay, reset and close fd */
    bool close();
    /* reset and set ov id to -1 / MSMFB_NEW_REQUEST */
    void reset();
    /* calls overlay set
     * Set would always consult last good known ov instance.
     * Only if it is different, set would actually exectue ioctl.
     * On a sucess ioctl. last good known ov instance is updated */
    bool set();
    /* Sets the source total width, height, format */
    void setSource(const utils::PipeArgs& pargs);
    /*
     * Sets ROI, the unpadded region, for source buffer.
     * Dim - ROI dimensions.
     */
    void setCrop(const utils::Dim& d);
    /* set color for mdp pipe */
    void setColor(const uint32_t color);
    void setTransform(const utils::eTransform& orient);
    /* given a dim and w/h, set overlay dim */
    void setPosition(const utils::Dim& dim);
    /* using user_data, sets/unsets roationvalue in mdp flags */
    void setRotationFlags();
    /* Update the src format with rotator's dest*/
    void updateSrcFormat(const uint32_t& rotDstFormat);
    /* dump state of the object */
    void dump() const;
    /* Return the dump in the specified buffer */
    void getDump(char *buf, size_t len);
    /* returns session id */
    int getPipeId() const;
    /* returns the fd associated to ctrl*/
    int getFd() const;
    /* returns a copy ro dst rect dim */
    utils::Dim getDstRectDim() const;
    /* returns a copy to src rect dim */
    utils::Dim getSrcRectDim() const;
    /* return pipe priority */
    uint8_t getPriority() const;
    /* setVisualParam */
    bool setVisualParams(const MetaData_t& data);
    /* sets pipe type RGB/DMA/VG */
    void setPipeType(const utils::eMdpPipeType& pType);

    static bool validateAndSet(MdpCtrl* mdpCtrlArray[], const int& count,
            const int& fbFd);
private:
    /* Perform transformation calculations */
    void doTransform();
    void doDownscale();
    /* get orient / user_data[0] */
    int getOrient() const;
    /* returns flags from mdp structure */
    int getFlags() const;
    /* set flags to mdp structure */
    void setFlags(int f);
    /* set z order */
    void setZ(utils::eZorder z);
    /* set isFg flag */
    void setIsFg(utils::eIsFg isFg);
    /* return a copy of src whf*/
    utils::Whf getSrcWhf() const;
    /* set plane alpha */
    void setPlaneAlpha(int planeAlpha);
    /* set blending method */
    void setBlending(overlay::utils::eBlending blending);

    /* set src whf */
    void setSrcWhf(const utils::Whf& whf);
    /* set src/dst rect dim */
    void setSrcRectDim(const utils::Dim d);
    void setDstRectDim(const utils::Dim d);
    /* returns user_data[0]*/
    int getUserData() const;
    /* sets user_data[0] */
    void setUserData(int v);

    utils::eTransform mOrientation; //Holds requested orientation
    /* Actual overlay mdp structure */
    mdp_overlay   mOVInfo;
    /* FD for the mdp fbnum */
    OvFD          mFd;
    int mDpy;

#ifdef USES_POST_PROCESSING
    /* PP Compute Params */
    struct compute_params mParams;
#endif
};

/* MDP data */
class MdpData {
public:
    /* ctor reset data */
    explicit MdpData(const int& dpy);
    /* dtor close*/
    ~MdpData();
    /* init FD */
    bool init(const int& dpy);
    /* memset0 the underlying mdp object */
    void reset();
    /* close fd, and reset */
    bool close();
    /* set id of mdp data */
    void setPipeId(int id);
    /* return ses id of data */
    int getPipeId() const;
    /* get underlying fd*/
    int getFd() const;
    /* get memory_id */
    int getSrcMemoryId() const;
    /* calls wrapper play */
    bool play(int fd, uint32_t offset);
    /* dump state of the object */
    void dump() const;
    /* Return the dump in the specified buffer */
    void getDump(char *buf, size_t len);

private:

    /* actual overlay mdp data */
    msmfb_overlay_data mOvData;
    /* fd to mdp fbnum */
    OvFD mFd;
};

//--------------Inlines---------------------------------

/////   MdpCtrl  //////

inline MdpCtrl::MdpCtrl(const int& dpy) {
    reset();
    init(dpy);
}

inline MdpCtrl::~MdpCtrl() {
    close();
}

inline int MdpCtrl::getOrient() const {
    return getUserData();
}

inline int MdpCtrl::getPipeId() const {
    return mOVInfo.id;
}

inline int MdpCtrl::getFd() const {
    return mFd.getFD();
}

inline int MdpCtrl::getFlags() const {
    return mOVInfo.flags;
}

inline void MdpCtrl::setFlags(int f) {
    mOVInfo.flags = f;
}

inline void MdpCtrl::setZ(overlay::utils::eZorder z) {
    mOVInfo.z_order = z;
}

inline void MdpCtrl::setIsFg(overlay::utils::eIsFg isFg) {
    mOVInfo.is_fg = isFg;
}

inline void MdpCtrl::setPlaneAlpha(int planeAlpha) {
    mOVInfo.alpha = planeAlpha;
}

inline void MdpCtrl::setBlending(overlay::utils::eBlending blending) {
    switch((int) blending) {
    case utils::OVERLAY_BLENDING_OPAQUE:
        mOVInfo.blend_op = BLEND_OP_OPAQUE;
        break;
    case utils::OVERLAY_BLENDING_PREMULT:
        mOVInfo.blend_op = BLEND_OP_PREMULTIPLIED;
        break;
    case utils::OVERLAY_BLENDING_COVERAGE:
    default:
        mOVInfo.blend_op = BLEND_OP_COVERAGE;
    }
}

inline overlay::utils::Whf MdpCtrl::getSrcWhf() const {
    return utils::Whf(  mOVInfo.src.width,
                        mOVInfo.src.height,
                        mOVInfo.src.format);
}

inline void MdpCtrl::setSrcWhf(const overlay::utils::Whf& whf) {
    mOVInfo.src.width  = whf.w;
    mOVInfo.src.height = whf.h;
    mOVInfo.src.format = whf.format;
}

inline overlay::utils::Dim MdpCtrl::getSrcRectDim() const {
    return utils::Dim(  mOVInfo.src_rect.x,
                        mOVInfo.src_rect.y,
                        mOVInfo.src_rect.w,
                        mOVInfo.src_rect.h);
}

inline void MdpCtrl::setSrcRectDim(const overlay::utils::Dim d) {
    mOVInfo.src_rect.x = d.x;
    mOVInfo.src_rect.y = d.y;
    mOVInfo.src_rect.w = d.w;
    mOVInfo.src_rect.h = d.h;
}

inline overlay::utils::Dim MdpCtrl::getDstRectDim() const {
    return utils::Dim(  mOVInfo.dst_rect.x,
                        mOVInfo.dst_rect.y,
                        mOVInfo.dst_rect.w,
                        mOVInfo.dst_rect.h);
}

inline void MdpCtrl::setDstRectDim(const overlay::utils::Dim d) {
    mOVInfo.dst_rect.x = d.x;
    mOVInfo.dst_rect.y = d.y;
    mOVInfo.dst_rect.w = d.w;
    mOVInfo.dst_rect.h = d.h;
}

inline int MdpCtrl::getUserData() const { return mOVInfo.user_data[0]; }

inline void MdpCtrl::setUserData(int v) { mOVInfo.user_data[0] = v; }

inline void MdpCtrl::setRotationFlags() {
    const int u = getUserData();
    if (u & MDP_ROT_90)
        mOVInfo.flags |= MDP_SOURCE_ROTATED_90;
}

inline uint8_t MdpCtrl::getPriority() const {
    return mOVInfo.priority;
}

///////    MdpData   //////

inline MdpData::MdpData(const int& dpy) {
    reset();
    init(dpy);
}

inline MdpData::~MdpData() { close(); }

inline void MdpData::reset() {
    overlay::utils::memset0(mOvData);
    mOvData.data.memory_id = -1;
}

inline bool MdpData::close() {
    reset();
    return mFd.close();
}

inline int MdpData::getSrcMemoryId() const { return mOvData.data.memory_id; }

inline void MdpData::setPipeId(int id) { mOvData.id = id; }

inline int MdpData::getPipeId() const { return mOvData.id; }

inline int MdpData::getFd() const { return mFd.getFD(); }

inline bool MdpData::play(int fd, uint32_t offset) {
    mOvData.data.memory_id = fd;
    mOvData.data.offset = offset;
    if(!mdp_wrapper::play(mFd.getFD(), mOvData)){
        ALOGE("MdpData failed to play");
        dump();
        return false;
    }
    return true;
}

} // overlay

#endif // OVERLAY_MDP_H
