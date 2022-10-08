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

#include "overlayGenPipe.h"
#include "mdp_version.h"

namespace overlay {

GenericPipe::GenericPipe(const int& dpy) : mDpy(dpy), mRotDownscaleOpt(false),
    pipeState(CLOSED), mCtrl(new Ctrl(dpy)), mData(new Data(dpy)) {
}

GenericPipe::~GenericPipe() {
    delete mCtrl;
    delete mData;
    setClosed();
}

void GenericPipe::setSource(const utils::PipeArgs& args) {
    mRotDownscaleOpt = args.rotFlags & utils::ROT_DOWNSCALE_ENABLED;
    mCtrl->setSource(args);
}

void GenericPipe::setCrop(const overlay::utils::Dim& d) {
    mCtrl->setCrop(d);
}

void GenericPipe::setColor(const uint32_t color) {
    mCtrl->setColor(color);
}

void GenericPipe::setTransform(const utils::eTransform& orient) {
    mCtrl->setTransform(orient);
}

void GenericPipe::setPosition(const utils::Dim& d) {
    mCtrl->setPosition(d);
}

bool GenericPipe::setVisualParams(const MetaData_t &metadata)
{
        return mCtrl->setVisualParams(metadata);
}

bool GenericPipe::commit() {
    bool ret = false;
    int downscale_factor = utils::ROT_DS_NONE;

    if(mRotDownscaleOpt) {
        ovutils::Dim src(mCtrl->getCrop());
        ovutils::Dim dst(mCtrl->getPosition());
        downscale_factor = ovutils::getDownscaleFactor(
                src.w, src.h, dst.w, dst.h);
    }

    mCtrl->setDownscale(downscale_factor);
    ret = mCtrl->commit();

    pipeState = ret ? OPEN : CLOSED;
    return ret;
}

bool GenericPipe::queueBuffer(int fd, uint32_t offset) {
    //TODO Move pipe-id transfer to CtrlData class. Make ctrl and data private.
    OVASSERT(isOpen(), "State is closed, cannot queueBuffer");
    int pipeId = mCtrl->getPipeId();
    OVASSERT(-1 != pipeId, "Ctrl ID should not be -1");
    // set pipe id from ctrl to data
    mData->setPipeId(pipeId);

    return mData->queueBuffer(fd, offset);
}

int GenericPipe::getCtrlFd() const {
    return mCtrl->getFd();
}

utils::Dim GenericPipe::getCrop() const
{
    return mCtrl->getCrop();
}

uint8_t GenericPipe::getPriority() const {
    return mCtrl->getPriority();
}

void GenericPipe::dump() const
{
    ALOGE("== Dump Generic pipe start ==");
    ALOGE("pipe state = %d", (int)pipeState);
    mCtrl->dump();
    mData->dump();
    ALOGE("== Dump Generic pipe end ==");
}

void GenericPipe::getDump(char *buf, size_t len) {
    mCtrl->getDump(buf, len);
    mData->getDump(buf, len);
}

bool GenericPipe::isClosed() const  {
    return (pipeState == CLOSED);
}

bool GenericPipe::isOpen() const  {
    return (pipeState == OPEN);
}

bool GenericPipe::setClosed() {
    pipeState = CLOSED;
    return true;
}

int GenericPipe::getPipeId() {
    return mCtrl->getPipeId();
}

bool GenericPipe::validateAndSet(GenericPipe* pipeArray[], const int& count,
        const int& fbFd) {
    Ctrl* ctrlArray[count];
    memset(&ctrlArray, 0, sizeof(ctrlArray));

    for(int i = 0; i < count; i++) {
        ctrlArray[i] = pipeArray[i]->mCtrl;
    }

    return Ctrl::validateAndSet(ctrlArray, count, fbFd);
}

} //namespace overlay
