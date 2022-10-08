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

#ifndef OVERLAY_GENERIC_PIPE_H
#define OVERLAY_GENERIC_PIPE_H

#include "overlayUtils.h"
#include "overlayRotator.h"
#include "overlayCtrlData.h"

namespace overlay {

class GenericPipe : utils::NoCopy {
public:
    /* ctor */
    explicit GenericPipe(int dpy);
    /* dtor */
    ~GenericPipe();
    bool init();
    bool close();
    /* Control APIs */
    /* set source using whf, orient and wait flag */
    void setSource(const utils::PipeArgs& args);
    /* set crop a.k.a the region of interest */
    void setCrop(const utils::Dim& d);
    /* set orientation*/
    void setTransform(const utils::eTransform& param);
    /* set mdp posision using dim */
    void setPosition(const utils::Dim& dim);
    /* set visual param */
    bool setVisualParams(const MetaData_t &metadata);
    /* commit changes to the overlay "set"*/
    bool commit();
    /* Data APIs */
    /* queue buffer to the overlay */
    bool queueBuffer(int fd, uint32_t offset);
    /* return cached startup args */
    const utils::PipeArgs& getArgs() const;
    /* retrieve cached crop data */
    utils::Dim getCrop() const;
    /* is closed */
    bool isClosed() const;
    /* is open */
    bool isOpen() const;
    /* return Ctrl fd. Used for S3D */
    int getCtrlFd() const;
    /* dump the state of the object */
    void dump() const;
    /* Return the dump in the specified buffer */
    void getDump(char *buf, size_t len);

private:
    /* set Closed pipe */
    bool setClosed();

    int mFbNum;
    /* Ctrl/Data aggregator */
    CtrlData mCtrlData;
    Rotator* mRot;
    //Whether rotator is used for 0-rot or otherwise
    bool mRotUsed;
    //Whether we will do downscale opt. This is just a request. If the frame is
    //not a candidate, we might not do it.
    bool mRotDownscaleOpt;
    //Whether the source is prerotated.
    bool mPreRotated;
    /* Pipe open or closed */
    enum ePipeState {
        CLOSED,
        OPEN
    };
    ePipeState pipeState;
};

} //namespace overlay

#endif // OVERLAY_GENERIC_PIPE_H
