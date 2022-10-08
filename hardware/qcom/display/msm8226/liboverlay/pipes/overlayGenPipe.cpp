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

GenericPipe::GenericPipe(const int& dpy) : mDpy(dpy),
    mCtrl(new Ctrl(dpy)), mData(new Data(dpy)) {
}

GenericPipe::~GenericPipe() {
    delete mCtrl;
    delete mData;
}

void GenericPipe::setSource(const utils::PipeArgs& args) {
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

void GenericPipe::setPipeType(const utils::eMdpPipeType& pType) {
    mCtrl->setPipeType(pType);
}

bool GenericPipe::commit() {
    return mCtrl->commit();
}

bool GenericPipe::queueBuffer(int fd, uint32_t offset) {
    int pipeId = mCtrl->getPipeId();
    OVASSERT(-1 != pipeId, "Ctrl ID should not be -1");
    // set pipe id from ctrl to data
    mData->setPipeId(pipeId);

    return mData->queueBuffer(fd, offset);
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
    mCtrl->dump();
    mData->dump();
    ALOGE("== Dump Generic pipe end ==");
}

void GenericPipe::getDump(char *buf, size_t len) {
    mCtrl->getDump(buf, len);
    mData->getDump(buf, len);
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
