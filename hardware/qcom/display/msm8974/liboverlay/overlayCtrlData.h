/*
* Copyright (c) 2012-2013, The Linux Foundation. All rights reserved.
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
*    * Neither the name of The Linux Foundation nor the names of its
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

#ifndef OVERLAY_CTRLDATA_H
#define OVERLAY_CTRLDATA_H

#include "overlayUtils.h"
#include "overlayMdp.h"
#include "gralloc_priv.h" // INTERLACE_MASK

namespace ovutils = overlay::utils;

namespace overlay {

/*
* Sequence to use:
* init
* start
* setXXX
* close
* */
class Ctrl : utils::NoCopy {
public:

    /* ctor */
    explicit Ctrl();
    /* dtor close */
    ~Ctrl();
    /* init fd etc*/
    bool init(uint32_t fbnum);
    /* close underlying mdp */
    bool close();

    /* set source using whf, orient and wait flag */
    void setSource(const utils::PipeArgs& args);
    /* set crop info and pass it down to mdp */
    void setCrop(const utils::Dim& d);
    /* set orientation */
    void setTransform(const utils::eTransform& p);
    /* set mdp position using dim */
    void setPosition(const utils::Dim& dim);
    /* set mdp visual params using metadata */
    bool setVisualParams(const MetaData_t &metadata);
    /* mdp set overlay/commit changes */
    bool commit();

    /* ctrl id */
    int  getPipeId() const;
    /* ctrl fd */
    int  getFd() const;
    /* retrieve crop data */
    utils::Dim getCrop() const;
    utils::Dim getPosition() const;
    /* Set downscale */
    void setDownscale(int dscale_factor);
    /* Update the src format based on rotator's dest */
    void updateSrcFormat(const uint32_t& rotDstFormat);
    /* dump the state of the object */
    void dump() const;
    /* Return the dump in the specified buffer */
    void getDump(char *buf, size_t len);
    void forceSet();

private:
    // mdp ctrl struct(info e.g.)
    MdpCtrl mMdp;
};


class Data : utils::NoCopy {
public:
    /* init, reset */
    explicit Data();
    /* calls close */
    ~Data();
    /* init fd etc */
    bool init(uint32_t fbnum);
    /* calls underlying mdp close */
    bool close();
    /* set overlay pipe id in the mdp struct */
    void setPipeId(int id);
    /* get overlay id in the mdp struct */
    int getPipeId() const;
    /* queue buffer to the overlay */
    bool queueBuffer(int fd, uint32_t offset);
    /* sump the state of the obj */
    void dump() const;
    /* Return the dump in the specified buffer */
    void getDump(char *buf, size_t len);

private:
    // mdp data struct
    MdpData mMdp;
};

/* This class just creates a Ctrl Data pair to be used by a pipe.
 * Although this was legacy design, this separation still makes sense, since we
 * need to use the Ctrl channel in hwc_prepare (i.e config stage) and Data
 * channel in hwc_set (i.e draw stage)
 */
struct CtrlData {
    Ctrl ctrl;
    Data data;
};

//-------------Inlines-------------------------------

inline Ctrl::Ctrl() {
    mMdp.reset();
}

inline Ctrl::~Ctrl() {
    close();
}

inline bool Ctrl::close() {
    if(!mMdp.close())
        return false;
    return true;
}

inline bool Ctrl::init(uint32_t fbnum) {
    // MDP/FD init
    if(!mMdp.init(fbnum)) {
        ALOGE("Ctrl failed to init fbnum=%d", fbnum);
        return false;
    }
    return true;
}

inline void Ctrl::setSource(const utils::PipeArgs& args)
{
    mMdp.setSource(args);
}

inline void Ctrl::setPosition(const utils::Dim& dim)
{
    mMdp.setPosition(dim);
}

inline void Ctrl::setTransform(const utils::eTransform& orient)
{
    mMdp.setTransform(orient);
}

inline void Ctrl::setCrop(const utils::Dim& d)
{
    mMdp.setCrop(d);
}

inline bool Ctrl::setVisualParams(const MetaData_t &metadata)
{
    if (!mMdp.setVisualParams(metadata)) {
        ALOGE("Ctrl setVisualParams failed in MDP setVisualParams");
        return false;
    }
    return true;
}

inline void Ctrl::dump() const {
    ALOGE("== Dump Ctrl start ==");
    mMdp.dump();
    ALOGE("== Dump Ctrl end ==");
}

inline bool Ctrl::commit() {
    if(!mMdp.set()) {
        ALOGE("Ctrl commit failed set overlay");
        return false;
    }
    return true;
}

inline int Ctrl::getPipeId() const {
    return mMdp.getPipeId();
}

inline int Ctrl::getFd() const {
    return mMdp.getFd();
}

inline void Ctrl::updateSrcFormat(const uint32_t& rotDstFmt) {
    mMdp.updateSrcFormat(rotDstFmt);
}

inline utils::Dim Ctrl::getCrop() const {
    return mMdp.getSrcRectDim();
}

inline utils::Dim Ctrl::getPosition() const {
    return mMdp.getDstRectDim();
}

inline void Ctrl::setDownscale(int dscale_factor) {
    mMdp.setDownscale(dscale_factor);
}

inline void Ctrl::getDump(char *buf, size_t len) {
    mMdp.getDump(buf, len);
}

inline void Ctrl::forceSet() {
    mMdp.forceSet();
}

inline Data::Data() {
    mMdp.reset();
}

inline Data::~Data() { close(); }

inline void Data::setPipeId(int id) { mMdp.setPipeId(id); }

inline int Data::getPipeId() const { return mMdp.getPipeId(); }

inline bool Data::init(uint32_t fbnum) {
    if(!mMdp.init(fbnum)) {
        ALOGE("Data cannot init mdp");
        return false;
    }
    return true;
}

inline bool Data::close() {
    if(!mMdp.close()) {
        ALOGE("Data close failed");
        return false;
    }
    return true;
}

inline bool Data::queueBuffer(int fd, uint32_t offset) {
    return mMdp.play(fd, offset);
}

inline void Data::dump() const {
    ALOGE("== Dump Data MDP start ==");
    mMdp.dump();
    ALOGE("== Dump Data MDP end ==");
}

inline void Data::getDump(char *buf, size_t len) {
    mMdp.getDump(buf, len);
}

} // overlay

#endif
