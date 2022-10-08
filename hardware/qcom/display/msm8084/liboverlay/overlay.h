/*
* Copyright (c) 2011-2013, The Linux Foundation. All rights reserved.
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

#ifndef OVERLAY_H
#define OVERLAY_H

#include "overlayUtils.h"
#include "mdp_version.h"
#include "utils/threads.h"

struct MetaData_t;

namespace overlay {
class GenericPipe;

class Overlay : utils::NoCopy {
public:
    enum { DMA_BLOCK_MODE, DMA_LINE_MODE };
    //Abstract Display types. Each backed by a LayerMixer,
    //represented by a fb node.
    //High res panels can be backed by 2 layer mixers and a single fb node.
    enum { DPY_PRIMARY, DPY_EXTERNAL, DPY_WRITEBACK, DPY_UNUSED };
    enum { DPY_MAX = DPY_UNUSED };
    enum { MIXER_LEFT, MIXER_RIGHT, MIXER_UNUSED };
    enum { MIXER_DEFAULT = MIXER_LEFT, MIXER_MAX = MIXER_UNUSED };
    enum { MAX_FB_DEVICES = DPY_MAX };
    enum { FORMAT_YUV, FORMAT_RGB };

    struct PipeSpecs {
        PipeSpecs() : formatClass(FORMAT_RGB), needsScaling(false), fb(false),
                dpy(DPY_PRIMARY), mixer(MIXER_DEFAULT) {}
        int formatClass;
        bool needsScaling;
        bool fb;
        int dpy;
        int mixer;
    };

    /* dtor close */
    ~Overlay();

    /* Marks the beginning of a drawing round, resets usage bits on pipes
     * Should be called when drawing begins before any pipe config is done.
     */
    void configBegin();

    /* Marks the end of config for this drawing round
     * Will do garbage collection of pipe objects and thus calling UNSETs,
     * closing FDs, removing rotator objects and memory, if allocated.
     * Should be called after all pipe configs are done.
     */
    void configDone();

    /* Get a pipe that supported the specified format class (yuv, rgb) and has
     * scaling capabilities.
     */
    utils::eDest getPipe(const PipeSpecs& pipeSpecs);
    /* Returns the eDest corresponding to an already allocated pipeid.
     * Useful for the reservation case, when libvpu reserves the pipe at its
     * end, and expect the overlay to allocate a given pipe for a layer.
     */
    utils::eDest reservePipe(int pipeid);
    /* getting dest for the given pipeid */
    utils::eDest getDest(int pipeid);
    /* getting overlay.pipeid for the given dest */
    int getPipeId(utils::eDest dest);

    void setSource(const utils::PipeArgs args, utils::eDest dest);
    void setCrop(const utils::Dim& d, utils::eDest dest);
    void setColor(const uint32_t color, utils::eDest dest);
    void setTransform(const int orientation, utils::eDest dest);
    void setPosition(const utils::Dim& dim, utils::eDest dest);
    void setVisualParams(const MetaData_t& data, utils::eDest dest);
    bool commit(utils::eDest dest);
    bool queueBuffer(int fd, uint32_t offset, utils::eDest dest);

    /* pipe reservation session is running */
    bool sessionInProgress(utils::eDest dest);
    /* pipe reservation session has ended*/
    bool isSessionEnded(utils::eDest dest);
    /* start session for the pipe reservation */
    void startSession(utils::eDest dest);
    /* end all started sesisons */
    void endAllSessions();
    /* Returns available ("unallocated") pipes for a display's mixer */
    int availablePipes(int dpy, int mixer);
    /* Returns available ("unallocated") pipes for a display */
    int availablePipes(int dpy);
    /* Returns available ("unallocated") pipe of given type for a display */
    int availablePipes(int dpy, utils::eMdpPipeType type);
    /* Returns if any of the requested pipe type is attached to any of the
     * displays
     */
    bool isPipeTypeAttached(utils::eMdpPipeType type);
    /* Compare pipe priorities and return
     * 1 if 1st pipe has a higher priority
     * 0 if both have the same priority
     *-1 if 2nd pipe has a higher priority
     */
    int comparePipePriority(utils::eDest pipe1Index, utils::eDest pipe2Index);
    /* Returns pipe dump. Expects a NULL terminated buffer of big enough size
     * to populate.
     */
    void getDump(char *buf, size_t len);
    /* Reset usage and allocation bits on all pipes for given display */
    void clear(int dpy);
    /* Validate the set of pipes for a display and set them in driver */
    bool validateAndSet(const int& dpy, const int& fbFd);

    /* Closes open pipes, called during startup */
    static int initOverlay();
    /* Returns the singleton instance of overlay */
    static Overlay* getInstance();
    static void setDMAMode(const int& mode);
    static int getDMAMode();
    /* Returns the framebuffer node backing up the display */
    static int getFbForDpy(const int& dpy);

    static bool displayCommit(const int& fd);
    /* Overloads display commit with ROI's of each halves.
     * Single interface panels will only update left ROI. */
    static bool displayCommit(const int& fd, const utils::Dim& lRoi,
                              const utils::Dim& rRoi);

private:
    /* Ctor setup */
    explicit Overlay();
    /*Validate index range, abort if invalid */
    void validate(int index);
    static void setDMAMultiplexingSupported();
    void dump() const;
    /* Returns an available pipe based on the type of pipe requested. When ANY
     * is requested, the first available VG or RGB is returned. If no pipe is
     * available for the display "dpy" then INV is returned. Note: If a pipe is
     * assigned to a certain display, then it cannot be assigned to another
     * display without being garbage-collected once. To add if a pipe is
     * asisgned to a mixer within a display it cannot be reused for another
     * mixer without being UNSET once*/
    utils::eDest nextPipe(utils::eMdpPipeType, int dpy, int mixer);
    /* Helpers that enfore target specific policies while returning pipes */
    utils::eDest getPipe_8x26(const PipeSpecs& pipeSpecs);
    utils::eDest getPipe_8x16(const PipeSpecs& pipeSpecs);

    /* Returns the handle to libscale.so's programScale function */
    static int (*getFnProgramScale())(struct mdp_overlay_list *);
    /* Creates a scalar object using libscale.so */
    static void initScalar();
    /* Destroys the scalar object using libscale.so */
    static void destroyScalar();

    /* Just like a Facebook for pipes, but much less profile info */
    struct PipeBook {
        void init();
        void destroy();
        /* Check if pipe exists and return true, false otherwise */
        bool valid();

        /* Hardware pipe wrapper */
        GenericPipe *mPipe;
        /* Display using this pipe. Refer to enums above */
        int mDisplay;
        /* Mixer within a split display this pipe is attached to */
        int mMixer;

        /* operations on bitmap */
        static bool pipeUsageUnchanged();
        static void setUse(int index);
        static void resetUse(int index);
        static bool isUsed(int index);
        static bool isNotUsed(int index);
        static void save();

        static void setAllocation(int index);
        static void resetAllocation(int index);
        static bool isAllocated(int index);
        static bool isNotAllocated(int index);

        static utils::eMdpPipeType getPipeType(utils::eDest dest);
        static const char* getDestStr(utils::eDest dest);

        static int NUM_PIPES;
        static utils::eMdpPipeType pipeTypeLUT[utils::OV_MAX];
        /* Session for reserved pipes */
        enum Session {
            NONE,
            START,
            END
        };
        Session mSession;

    private:
        //usage tracks if a successful commit happened. So a pipe could be
        //allocated to a display, but it may not end up using it for various
        //reasons. If one display actually uses a pipe then it amy not be
        //used by another display, without an UNSET in between.
        static int sPipeUsageBitmap;
        static int sLastUsageBitmap;
        //Tracks which pipe objects are allocated. This does not imply that they
        //will actually be used. For example, a display might choose to acquire
        //3 pipe objects in one shot and proceed with config only if it gets all
        //3. The bitmap helps allocate different pipe objects on each request.
        static int sAllocatedBitmap;
    };

    PipeBook mPipeBook[utils::OV_INVALID]; //Used as max

    /* Dump string */
    char mDumpStr[1024];

    /* Singleton Instance*/
    static Overlay *sInstance;
    static int sDpyFbMap[DPY_MAX];
    static int sDMAMode;
    static bool sDMAMultiplexingSupported;
    static void *sLibScaleHandle;
    static int (*sFnProgramScale)(struct mdp_overlay_list *);

    friend class MdpCtrl;
};

inline void Overlay::validate(int index) {
    OVASSERT(index >=0 && index < PipeBook::NUM_PIPES, \
        "%s, Index out of bounds: %d", __FUNCTION__, index);
    OVASSERT(mPipeBook[index].valid(), "Pipe does not exist %s",
            PipeBook::getDestStr((utils::eDest)index));
}

inline int Overlay::availablePipes(int dpy, int mixer) {
    int avail = 0;
    for(int i = 0; i < PipeBook::NUM_PIPES; i++) {
        if( (mPipeBook[i].mDisplay == DPY_UNUSED ||
             mPipeBook[i].mDisplay == dpy) &&
            (mPipeBook[i].mMixer == MIXER_UNUSED ||
             mPipeBook[i].mMixer == mixer) &&
            PipeBook::isNotAllocated(i) &&
            !(Overlay::getDMAMode() == Overlay::DMA_BLOCK_MODE &&
              PipeBook::getPipeType((utils::eDest)i) ==
              utils::OV_MDP_PIPE_DMA)) {
            avail++;
        }
    }
    return avail;
}

inline int Overlay::availablePipes(int dpy) {
    int avail = 0;
    for(int i = 0; i < PipeBook::NUM_PIPES; i++) {
        if( (mPipeBook[i].mDisplay == DPY_UNUSED ||
             mPipeBook[i].mDisplay == dpy) &&
            PipeBook::isNotAllocated(i) &&
            !(Overlay::getDMAMode() == Overlay::DMA_BLOCK_MODE &&
              PipeBook::getPipeType((utils::eDest)i) ==
              utils::OV_MDP_PIPE_DMA)) {
            avail++;
        }
    }
    return avail;
}

inline int Overlay::availablePipes(int dpy, utils::eMdpPipeType type) {
    int avail = 0;
    for(int i = 0; i < PipeBook::NUM_PIPES; i++) {
        if((mPipeBook[i].mDisplay == DPY_UNUSED ||
            mPipeBook[i].mDisplay == dpy) &&
            PipeBook::isNotAllocated(i) &&
            type == PipeBook::getPipeType((utils::eDest)i)) {
            avail++;
        }
    }
    return avail;
}

inline void Overlay::setDMAMode(const int& mode) {
    if(mode == DMA_LINE_MODE || mode == DMA_BLOCK_MODE)
        sDMAMode = mode;
}

inline void Overlay::setDMAMultiplexingSupported() {
    sDMAMultiplexingSupported = false;
    if(qdutils::MDPVersion::getInstance().is8x26())
        sDMAMultiplexingSupported = true;
}

inline int Overlay::getDMAMode() {
    return sDMAMode;
}

inline int Overlay::getFbForDpy(const int& dpy) {
    OVASSERT(dpy >= 0 && dpy < DPY_MAX, "Invalid dpy %d", dpy);
    return sDpyFbMap[dpy];
}

inline int (*Overlay::getFnProgramScale())(struct mdp_overlay_list *) {
    return sFnProgramScale;
}

inline bool Overlay::PipeBook::valid() {
    return (mPipe != NULL);
}

inline bool Overlay::PipeBook::pipeUsageUnchanged() {
    return (sPipeUsageBitmap == sLastUsageBitmap);
}

inline void Overlay::PipeBook::setUse(int index) {
    sPipeUsageBitmap |= (1 << index);
}

inline void Overlay::PipeBook::resetUse(int index) {
    sPipeUsageBitmap &= ~(1 << index);
}

inline bool Overlay::PipeBook::isUsed(int index) {
    return sPipeUsageBitmap & (1 << index);
}

inline bool Overlay::PipeBook::isNotUsed(int index) {
    return !isUsed(index);
}

inline void Overlay::PipeBook::save() {
    sLastUsageBitmap = sPipeUsageBitmap;
}

inline void Overlay::PipeBook::setAllocation(int index) {
    sAllocatedBitmap |= (1 << index);
}

inline void Overlay::PipeBook::resetAllocation(int index) {
    sAllocatedBitmap &= ~(1 << index);
}

inline bool Overlay::PipeBook::isAllocated(int index) {
    return sAllocatedBitmap & (1 << index);
}

inline bool Overlay::PipeBook::isNotAllocated(int index) {
    return !isAllocated(index);
}

inline utils::eMdpPipeType Overlay::PipeBook::getPipeType(utils::eDest dest) {
    return pipeTypeLUT[(int)dest];
}

inline void Overlay::startSession(utils::eDest dest) {
    mPipeBook[(int)dest].mSession = PipeBook::START;
}

inline bool Overlay::sessionInProgress(utils::eDest dest) {
    return (mPipeBook[(int)dest].mSession == PipeBook::START);
}

inline bool Overlay::isSessionEnded(utils::eDest dest) {
    return (mPipeBook[(int)dest].mSession == PipeBook::END);
}

inline const char* Overlay::PipeBook::getDestStr(utils::eDest dest) {
    switch(getPipeType(dest)) {
        case utils::OV_MDP_PIPE_RGB: return "RGB";
        case utils::OV_MDP_PIPE_VG: return "VG";
        case utils::OV_MDP_PIPE_DMA: return "DMA";
        default: return "Invalid";
    }
    return "Invalid";
}

}; // overlay

#endif // OVERLAY_H
