/*
* Copyright (c) 2013 The Linux Foundation. All rights reserved.
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
#ifndef OVERLAY_WRITEBACK_H
#define OVERLAY_WRITEBACK_H

#include "overlayMem.h"

namespace overlay {

class WritebackMgr;

class WritebackMem {
public:
    explicit WritebackMem() : mCurrOffsetIndex(0) {
        memset(&mOffsets, 0, sizeof(mOffsets));
    }
    ~WritebackMem() { dealloc(); }
    bool manageMem(uint32_t size, bool isSecure);
    void useNextBuffer() {
            mCurrOffsetIndex = (mCurrOffsetIndex + 1) % NUM_BUFS;
    }
    uint32_t getOffset() const { return mOffsets[mCurrOffsetIndex]; }
    int getDstFd() const { return mBuf.getFD(); }
private:
    bool alloc(uint32_t size, bool isSecure);
    bool dealloc();
    enum { NUM_BUFS = 2 };
    OvMem mBuf;
    uint32_t mOffsets[NUM_BUFS];
    uint32_t mCurrOffsetIndex;
};

//Abstracts the WB2 interface of MDP
//Has modes to either manage memory or work with memory allocated elsewhere
class Writeback {
public:
    ~Writeback();
    bool configureDpyInfo(int xres, int yres);
    bool configureMemory(uint32_t size, bool isSecure);
    /* Blocking write. (queue, commit, dequeue)
     * This class will do writeback memory management.
     * This class will call display-commit on writeback mixer.
     */
    bool writeSync();
    /* Blocking write. (queue, commit, dequeue)
     * Client must do writeback memory management.
     * Client must not call display-commit on writeback mixer.
     */
    bool writeSync(int opFd, uint32_t opOffset);
    /* Async queue. (Does not write)
     * Client must do writeback memory management.
     * Client must call display-commit on their own.
     * Client must use sync mechanism e.g sync pt.
     */
    bool queueBuffer(int opFd, uint32_t opOffset);
    uint32_t getOffset() const { return mWbMem.getOffset(); }
    int getDstFd() const { return mWbMem.getDstFd(); }
    /* Subject to GC if writeback isnt used for a drawing round.
     * Get always if caching the value.
     */
    int getFbFd() const { return mFd.getFD(); }
    int getOutputFormat();
    bool setOutputFormat(int mdpFormat);

    static Writeback* getInstance();
    static void configBegin() { sUsed = false; }
    static void configDone();
    static void clear();
    //Will take a dump of data structure only if there is an instance existing
    //Returns true if dump is added to the input buffer, false otherwise
    static bool getDump(char *buf, size_t len);

private:
    explicit Writeback();
    bool startSession();
    bool stopSession();
    //Actually block_until_write_done for the usage here.
    bool dequeueBuffer();
    OvFD mFd;
    WritebackMem mWbMem;
    struct msmfb_data mFbData;
    int mXres;
    int mYres;
    int mOpFmt;

    static bool sUsed;
    static Writeback *sWb;
};

}

#endif
