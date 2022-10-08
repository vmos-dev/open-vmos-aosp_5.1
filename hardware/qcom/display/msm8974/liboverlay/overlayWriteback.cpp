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

#include "overlay.h"
#include "overlayWriteback.h"
#include "mdpWrapper.h"

namespace overlay {

//=========== class WritebackMem ==============================================
bool WritebackMem::manageMem(uint32_t size, bool isSecure) {
    if(mBuf.bufSz() == size) {
        return true;
    }
    if(mBuf.valid()) {
        if(!mBuf.close()) {
            ALOGE("%s error closing mem", __func__);
            return false;
        }
    }
    return alloc(size, isSecure);
}

bool WritebackMem::alloc(uint32_t size, bool isSecure) {
    if(!mBuf.open(NUM_BUFS, size, isSecure)){
        ALOGE("%s: Failed to open", __func__);
        mBuf.close();
        return false;
    }

    OVASSERT(MAP_FAILED != mBuf.addr(), "MAP failed");
    OVASSERT(mBuf.getFD() != -1, "getFd is -1");

    mCurrOffsetIndex = 0;
    for (uint32_t i = 0; i < NUM_BUFS; i++) {
        mOffsets[i] = i * size;
    }
    return true;
}

bool WritebackMem::dealloc() {
    bool ret = true;
    if(mBuf.valid()) {
        ret = mBuf.close();
    }
    return ret;
}

//=========== class Writeback =================================================
Writeback::Writeback() : mXres(0), mYres(0), mOpFmt(-1) {
    int fbNum = Overlay::getFbForDpy(Overlay::DPY_WRITEBACK);
    if(!utils::openDev(mFd, fbNum, Res::fbPath, O_RDWR)) {
        ALOGE("%s failed to init %s", __func__, Res::fbPath);
        return;
    }
    startSession();
}

Writeback::~Writeback() {
    stopSession();
    if (!mFd.close()) {
        ALOGE("%s error closing fd", __func__);
    }
}

bool Writeback::startSession() {
    if(!mdp_wrapper::wbInitStart(mFd.getFD())) {
        ALOGE("%s failed", __func__);
        return false;
    }
    return true;
}

bool Writeback::stopSession() {
    if(mFd.valid()) {
        if(!mdp_wrapper::wbStopTerminate(mFd.getFD())) {
            ALOGE("%s failed", __func__);
            return false;
        }
    } else {
        ALOGE("%s Invalid fd", __func__);
        return false;
    }
    return true;
}

bool Writeback::configureDpyInfo(int xres, int yres) {
    if(mXres != xres || mYres != yres) {
        fb_var_screeninfo vinfo;
        memset(&vinfo, 0, sizeof(fb_var_screeninfo));
        if(!mdp_wrapper::getVScreenInfo(mFd.getFD(), vinfo)) {
            ALOGE("%s failed", __func__);
            return false;
        }
        vinfo.xres = xres;
        vinfo.yres = yres;
        vinfo.xres_virtual = xres;
        vinfo.yres_virtual = yres;
        vinfo.xoffset = 0;
        vinfo.yoffset = 0;
        if(!mdp_wrapper::setVScreenInfo(mFd.getFD(), vinfo)) {
            ALOGE("%s failed", __func__);
            return false;
        }
        mXres = xres;
        mYres = yres;
    }
    return true;
}

bool Writeback::configureMemory(uint32_t size, bool isSecure) {
    if(!mWbMem.manageMem(size, isSecure)) {
        ALOGE("%s failed, memory failure", __func__);
        return false;
    }
    return true;
}

bool Writeback::queueBuffer(int opFd, uint32_t opOffset) {
    memset(&mFbData, 0, sizeof(struct msmfb_data));
    //Queue
    mFbData.offset = opOffset;
    mFbData.memory_id = opFd;
    mFbData.id = 0;
    mFbData.flags = 0;
    if(!mdp_wrapper::wbQueueBuffer(mFd.getFD(), mFbData)) {
        ALOGE("%s: queuebuffer failed", __func__);
        return false;
    }
    return true;
}

bool Writeback::dequeueBuffer() {
    //Dequeue
    mFbData.flags = MSMFB_WRITEBACK_DEQUEUE_BLOCKING;
    if(!mdp_wrapper::wbDequeueBuffer(mFd.getFD(), mFbData)) {
        ALOGE("%s: dequeuebuffer failed", __func__);
        return false;
    }
    return true;
}

bool Writeback::writeSync(int opFd, uint32_t opOffset) {
    if(!queueBuffer(opFd, opOffset)) {
        return false;
    }
    if(!Overlay::displayCommit(mFd.getFD())) {
        return false;
    }
    if(!dequeueBuffer()) {
        return false;
    }
    return true;
}

bool Writeback::writeSync() {
    mWbMem.useNextBuffer();
    return writeSync(mWbMem.getDstFd(), mWbMem.getOffset());
}

bool Writeback::setOutputFormat(int mdpFormat) {
    if(mdpFormat != mOpFmt) {
        struct msmfb_metadata metadata;
        memset(&metadata, 0 , sizeof(metadata));
        metadata.op = metadata_op_wb_format;
        metadata.data.mixer_cfg.writeback_format = mdpFormat;
        if (ioctl(mFd.getFD(), MSMFB_METADATA_SET, &metadata) < 0) {
            ALOGE("Error setting MDP Writeback format");
            return false;
        }
        mOpFmt = mdpFormat;
    }
    return true;
}

int Writeback::getOutputFormat() {
    if(mOpFmt < 0) {
        struct msmfb_metadata metadata;
        memset(&metadata, 0 , sizeof(metadata));
        metadata.op = metadata_op_wb_format;
        if (ioctl(mFd.getFD(), MSMFB_METADATA_GET, &metadata) < 0) {
            ALOGE("Error retrieving MDP Writeback format");
            return -1;
        }
        mOpFmt =  metadata.data.mixer_cfg.writeback_format;
    }
    return mOpFmt;
}

//static

Writeback *Writeback::getInstance() {
    if(sWb == NULL) {
        sWb = new Writeback();
    }
    sUsed = true;
    return sWb;
}

void Writeback::configDone() {
    if(sUsed == false && sWb) {
        delete sWb;
        sWb = NULL;
    }
}

void Writeback::clear() {
    sUsed = false;
    if(sWb) {
        delete sWb;
        sWb = NULL;
    }
}

bool Writeback::getDump(char *buf, size_t len) {
    if(sWb) {
        utils::getDump(buf, len, "WBData", sWb->mFbData);
        char str[4] = {'\0'};
        snprintf(str, 4, "\n");
        strncat(buf, str, strlen(str));
        return true;
    }
    return false;
}

Writeback *Writeback::sWb = 0;
bool Writeback::sUsed = false;

} //namespace overlay
