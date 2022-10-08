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
#include "utils/threads.h"

struct MetaData_t;

namespace overlay {
class GenericPipe;

class Overlay : utils::NoCopy {
public:
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

    /* Returns an available pipe based on the type of pipe requested. When ANY
     * is requested, the first available VG or RGB is returned. If no pipe is
     * available for the display "dpy" then INV is returned. Note: If a pipe is
     * assigned to a certain display, then it cannot be assigned to another
     * display without being garbage-collected once */
    utils::eDest nextPipe(utils::eMdpPipeType, int dpy);

    void setSource(const utils::PipeArgs args, utils::eDest dest);
    void setCrop(const utils::Dim& d, utils::eDest dest);
    void setTransform(const int orientation, utils::eDest dest);
    void setPosition(const utils::Dim& dim, utils::eDest dest);
    void setVisualParams(const MetaData_t& data, utils::eDest dest);
    bool commit(utils::eDest dest);
    bool queueBuffer(int fd, uint32_t offset, utils::eDest dest);

    /* Closes open pipes, called during startup */
    static int initOverlay();
    /* Returns the singleton instance of overlay */
    static Overlay* getInstance();
    /* Returns available ("unallocated") pipes for a display */
    int availablePipes(int dpy);
    /* set the framebuffer index for external display */
    void setExtFbNum(int fbNum);
    /* Returns framebuffer index of the current external display */
    int getExtFbNum();
    /* Returns pipe dump. Expects a NULL terminated buffer of big enough size
     * to populate.
     */
    void getDump(char *buf, size_t len);

private:
    /* Ctor setup */
    explicit Overlay();
    /*Validate index range, abort if invalid */
    void validate(int index);
    void dump() const;

    /* Just like a Facebook for pipes, but much less profile info */
    struct PipeBook {
        enum { DPY_PRIMARY, DPY_EXTERNAL, DPY_UNUSED };

        void init();
        void destroy();
        /* Check if pipe exists and return true, false otherwise */
        bool valid();

        /* Hardware pipe wrapper */
        GenericPipe *mPipe;
        /* Display using this pipe. Refer to enums above */
        int mDisplay;

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
    char mDumpStr[256];

    /* Singleton Instance*/
    static Overlay *sInstance;
    static int sExtFbIndex;
};

inline void Overlay::validate(int index) {
    OVASSERT(index >=0 && index < PipeBook::NUM_PIPES, \
        "%s, Index out of bounds: %d", __FUNCTION__, index);
    OVASSERT(mPipeBook[index].valid(), "Pipe does not exist %s",
            PipeBook::getDestStr((utils::eDest)index));
}

inline int Overlay::availablePipes(int dpy) {
     int avail = 0;
     for(int i = 0; i < PipeBook::NUM_PIPES; i++) {
       if((mPipeBook[i].mDisplay == PipeBook::DPY_UNUSED ||
           mPipeBook[i].mDisplay == dpy) && PipeBook::isNotAllocated(i)) {
                avail++;
        }
    }
    return avail;
}

inline void Overlay::setExtFbNum(int fbNum) {
    sExtFbIndex = fbNum;
}

inline int Overlay::getExtFbNum() {
    return sExtFbIndex;
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
