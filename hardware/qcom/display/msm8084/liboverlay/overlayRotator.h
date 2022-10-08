/*
* Copyright (c) 2011,2013 The Linux Foundation. All rights reserved.
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

#ifndef OVERlAY_ROTATOR_H
#define OVERlAY_ROTATOR_H

#include <stdlib.h>

#include "mdpWrapper.h"
#include "overlayUtils.h"
#include "overlayMem.h"
#include "sync/sync.h"

namespace overlay {

/*
   Manages the case where new rotator memory needs to be
   allocated, before previous is freed, due to resolution change etc. If we make
   rotator memory to be always max size, irrespctive of source resolution then
   we don't need this RotMem wrapper. The inner class is sufficient.
*/
struct RotMem {
    // Max rotator buffers
    enum { ROT_NUM_BUFS = 2 };
    RotMem();
    ~RotMem();
    bool close();
    bool valid() { return mem.valid(); }
    uint32_t size() const { return mem.bufSz(); }
    void setReleaseFd(const int& fence);

    // rotator data info dst offset
    uint32_t mRotOffset[ROT_NUM_BUFS];
    int mRelFence[ROT_NUM_BUFS];
    // current slot being used
    uint32_t mCurrIndex;
    OvMem mem;
};

class Rotator
{
public:
    enum { TYPE_MDP, TYPE_MDSS };
    virtual ~Rotator();
    virtual void setSource(const utils::Whf& wfh) = 0;
    virtual void setCrop(const utils::Dim& crop) = 0;
    virtual void setFlags(const utils::eMdpFlags& flags) = 0;
    virtual void setTransform(const utils::eTransform& rot) = 0;
    virtual bool commit() = 0;
    virtual void setDownscale(int ds) = 0;
    virtual int getDstMemId() const = 0;
    virtual uint32_t getDstOffset() const = 0;
    virtual uint32_t getDstFormat() const = 0;
    virtual uint32_t getSessId() const = 0;
    virtual bool queueBuffer(int fd, uint32_t offset) = 0;
    virtual void dump() const = 0;
    virtual void getDump(char *buf, size_t len) const = 0;
    void setReleaseFd(const int& fence) { mMem.setReleaseFd(fence); }
    static Rotator *getRotator();

protected:
    /* Rotator memory manager */
    RotMem mMem;
    explicit Rotator() {}
    static uint32_t calcOutputBufSize(const utils::Whf& destWhf);

private:
    /*Returns rotator h/w type */
    static int getRotatorHwType();
    friend class RotMgr;
};

/*
* MDP rot holds MDP's rotation related structures.
*
* */
class MdpRot : public Rotator {
public:
    virtual ~MdpRot();
    virtual void setSource(const utils::Whf& wfh);
    virtual void setCrop(const utils::Dim& crop);
    virtual void setFlags(const utils::eMdpFlags& flags);
    virtual void setTransform(const utils::eTransform& rot);
    virtual bool commit();
    virtual void setDownscale(int ds);
    virtual int getDstMemId() const;
    virtual uint32_t getDstOffset() const;
    virtual uint32_t getDstFormat() const;
    virtual uint32_t getSessId() const;
    virtual bool queueBuffer(int fd, uint32_t offset);
    virtual void dump() const;
    virtual void getDump(char *buf, size_t len) const;

private:
    explicit MdpRot();
    bool init();
    bool close();
    void setRotations(uint32_t r);
    bool enabled () const;
    /* remap rot buffers */
    bool remap(uint32_t numbufs);
    bool open_i(uint32_t numbufs, uint32_t bufsz);
    /* Deferred transform calculations */
    void doTransform();
    /* reset underlying data, basically memset 0 */
    void reset();
    /* return true if current rotator config is different
     * than last known config */
    bool rotConfChanged() const;
    /* save mRotImgInfo to be last known good config*/
    void save();
    /* Calculates the rotator's o/p buffer size post the transform calcs and
     * knowing the o/p format depending on whether fastYuv is enabled or not */
    uint32_t calcOutputBufSize();

    /* rot info*/
    msm_rotator_img_info mRotImgInfo;
    /* Last saved rot info*/
    msm_rotator_img_info mLSRotImgInfo;
    /* rot data */
    msm_rotator_data_info mRotDataInfo;
    /* Orientation */
    utils::eTransform mOrientation;
    /* rotator fd */
    OvFD mFd;

    friend Rotator* Rotator::getRotator();
};

/*
+* MDSS Rot holds MDSS's rotation related structures.
+*
+* */
class MdssRot : public Rotator {
public:
    virtual ~MdssRot();
    virtual void setSource(const utils::Whf& wfh);
    virtual void setCrop(const utils::Dim& crop);
    virtual void setFlags(const utils::eMdpFlags& flags);
    virtual void setTransform(const utils::eTransform& rot);
    virtual bool commit();
    virtual void setDownscale(int ds);
    virtual int getDstMemId() const;
    virtual uint32_t getDstOffset() const;
    virtual uint32_t getDstFormat() const;
    virtual uint32_t getSessId() const;
    virtual bool queueBuffer(int fd, uint32_t offset);
    virtual void dump() const;
    virtual void getDump(char *buf, size_t len) const;

private:
    explicit MdssRot();
    bool init();
    bool close();
    void setRotations(uint32_t r);
    bool enabled () const;
    /* remap rot buffers */
    bool remap(uint32_t numbufs);
    bool open_i(uint32_t numbufs, uint32_t bufsz);
    /* Deferred transform calculations */
    void doTransform();
    /* reset underlying data, basically memset 0 */
    void reset();
    /* Calculates the rotator's o/p buffer size post the transform calcs and
     * knowing the o/p format depending on whether fastYuv is enabled or not */
    uint32_t calcOutputBufSize();
    // Calculate the compressed o/p buffer size for BWC
    uint32_t calcCompressedBufSize(const utils::Whf& destWhf);

    /* MdssRot info structure */
    mdp_overlay   mRotInfo;
    /* MdssRot data structure */
    msmfb_overlay_data mRotData;
    /* Orientation */
    utils::eTransform mOrientation;
    /* rotator fd */
    OvFD mFd;
    /* Enable/Disable Mdss Rot*/
    bool mEnabled;

    friend Rotator* Rotator::getRotator();
};

// Holder of rotator objects. Manages lifetimes
class RotMgr {
public:
    enum { MAX_ROT_SESS = 4 };

    ~RotMgr();
    void configBegin();
    void configDone();
    overlay::Rotator *getNext();
    void clear(); //Removes all instances
    //Resets the usage of top count objects, making them available for reuse
    void markUnusedTop(const uint32_t& count) { mUseCount -= count; }
    /* Returns rot dump.
     * Expects a NULL terminated buffer of big enough size.
     */
    void getDump(char *buf, size_t len);
    int getRotDevFd();
    int getNumActiveSessions() { return mUseCount; }

    static RotMgr *getInstance();

private:
    RotMgr();
    static RotMgr *sRotMgr;

    overlay::Rotator *mRot[MAX_ROT_SESS];
    uint32_t mUseCount;
    int mRotDevFd;
};


} // overlay

#endif // OVERlAY_ROTATOR_H
