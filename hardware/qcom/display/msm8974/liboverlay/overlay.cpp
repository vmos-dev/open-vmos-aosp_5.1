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

#include "overlay.h"
#include "pipes/overlayGenPipe.h"
#include "mdp_version.h"
#include "qdMetaData.h"

#define PIPE_DEBUG 0

namespace overlay {
using namespace utils;

Overlay::Overlay() {
    PipeBook::NUM_PIPES = qdutils::MDPVersion::getInstance().getTotalPipes();
    for(int i = 0; i < PipeBook::NUM_PIPES; i++) {
        mPipeBook[i].init();
    }

    mDumpStr[0] = '\0';
}

Overlay::~Overlay() {
    for(int i = 0; i < PipeBook::NUM_PIPES; i++) {
        mPipeBook[i].destroy();
    }
}

void Overlay::configBegin() {
    for(int i = 0; i < PipeBook::NUM_PIPES; i++) {
        //Mark as available for this round.
        PipeBook::resetUse(i);
        PipeBook::resetAllocation(i);
    }
    sForceSetBitmap = 0;
    mDumpStr[0] = '\0';
}

void Overlay::configDone() {
    for(int i = 0; i < PipeBook::NUM_PIPES; i++) {
        if(PipeBook::isNotUsed(i)) {
            //Forces UNSET on pipes, flushes rotator memory and session, closes
            //fds
            if(mPipeBook[i].valid()) {
                char str[32];
                sprintf(str, "Unset=%s dpy=%d mix=%d; ",
                        PipeBook::getDestStr((eDest)i),
                        mPipeBook[i].mDisplay, mPipeBook[i].mMixer);
#if PIPE_DEBUG
                strncat(mDumpStr, str, strlen(str));
#endif
            }
            mPipeBook[i].destroy();
        }
    }
    dump();
    PipeBook::save();
}

eDest Overlay::nextPipe(eMdpPipeType type, int dpy, int mixer) {
    eDest dest = OV_INVALID;

    for(int i = 0; i < PipeBook::NUM_PIPES; i++) {
        if( (type == OV_MDP_PIPE_ANY || //Pipe type match
             type == PipeBook::getPipeType((eDest)i)) &&
            (mPipeBook[i].mDisplay == DPY_UNUSED || //Free or same display
             mPipeBook[i].mDisplay == dpy) &&
            (mPipeBook[i].mMixer == MIXER_UNUSED || //Free or same mixer
             mPipeBook[i].mMixer == mixer) &&
            PipeBook::isNotAllocated(i) && //Free pipe
            !(sDMAMode == DMA_BLOCK_MODE && //DMA pipe in Line mode
               PipeBook::getPipeType((eDest)i) == OV_MDP_PIPE_DMA)) {
            dest = (eDest)i;
            PipeBook::setAllocation(i);
            break;
        }
    }

    if(dest != OV_INVALID) {
        int index = (int)dest;
        mPipeBook[index].mDisplay = dpy;
        mPipeBook[index].mMixer = mixer;
        if(not mPipeBook[index].valid()) {
            mPipeBook[index].mPipe = new GenericPipe(dpy);
            char str[32];
            snprintf(str, 32, "Set=%s dpy=%d mix=%d; ",
                     PipeBook::getDestStr(dest), dpy, mixer);
#if PIPE_DEBUG
            strncat(mDumpStr, str, strlen(str));
#endif
        }
    } else {
        ALOGD_IF(PIPE_DEBUG, "Pipe unavailable type=%d display=%d mixer=%d",
                (int)type, dpy, mixer);
    }

    return dest;
}

bool Overlay::isPipeTypeAttached(eMdpPipeType type) {
    for(int i = 0; i < PipeBook::NUM_PIPES; i++) {
        if(type == PipeBook::getPipeType((eDest)i) &&
                mPipeBook[i].mDisplay != DPY_UNUSED) {
            return true;
        }
    }
    return false;
}

bool Overlay::commit(utils::eDest dest) {
    bool ret = false;
    int index = (int)dest;
    validate(index);

    if(mPipeBook[index].mPipe->commit()) {
        ret = true;
        PipeBook::setUse((int)dest);
        if(sForceSetBitmap & (1 << mPipeBook[index].mDisplay)) {
            mPipeBook[index].mPipe->forceSet();
        }
    } else {
        int dpy = mPipeBook[index].mDisplay;
        for(int i = 0; i < PipeBook::NUM_PIPES; i++)
            if (mPipeBook[i].mDisplay == dpy) {
                PipeBook::resetAllocation(i);
                PipeBook::resetUse(i);
                if(mPipeBook[i].valid()) {
                    mPipeBook[i].mPipe->forceSet();
                }
            }
    }
    return ret;
}

bool Overlay::queueBuffer(int fd, uint32_t offset,
        utils::eDest dest) {
    int index = (int)dest;
    bool ret = false;
    validate(index);
    //Queue only if commit() has succeeded (and the bit set)
    if(PipeBook::isUsed((int)dest)) {
        ret = mPipeBook[index].mPipe->queueBuffer(fd, offset);
    }
    return ret;
}

void Overlay::setCrop(const utils::Dim& d,
        utils::eDest dest) {
    int index = (int)dest;
    validate(index);
    mPipeBook[index].mPipe->setCrop(d);
}

void Overlay::setPosition(const utils::Dim& d,
        utils::eDest dest) {
    int index = (int)dest;
    validate(index);
    mPipeBook[index].mPipe->setPosition(d);
}

void Overlay::setTransform(const int orient,
        utils::eDest dest) {
    int index = (int)dest;
    validate(index);

    utils::eTransform transform =
            static_cast<utils::eTransform>(orient);
    mPipeBook[index].mPipe->setTransform(transform);

}

void Overlay::setSource(const utils::PipeArgs args,
        utils::eDest dest) {
    int index = (int)dest;
    validate(index);

    PipeArgs newArgs(args);
    if(PipeBook::getPipeType(dest) == OV_MDP_PIPE_VG) {
        setMdpFlags(newArgs.mdpFlags, OV_MDP_PIPE_SHARE);
    } else {
        clearMdpFlags(newArgs.mdpFlags, OV_MDP_PIPE_SHARE);
    }

    if(PipeBook::getPipeType(dest) == OV_MDP_PIPE_DMA) {
        setMdpFlags(newArgs.mdpFlags, OV_MDP_PIPE_FORCE_DMA);
    } else {
        clearMdpFlags(newArgs.mdpFlags, OV_MDP_PIPE_FORCE_DMA);
    }

    mPipeBook[index].mPipe->setSource(newArgs);
}

void Overlay::setVisualParams(const MetaData_t& metadata, utils::eDest dest) {
    int index = (int)dest;
    validate(index);
    mPipeBook[index].mPipe->setVisualParams(metadata);
}

Overlay* Overlay::getInstance() {
    if(sInstance == NULL) {
        sInstance = new Overlay();
    }
    return sInstance;
}

// Clears any VG pipes allocated to the fb devices
// Generates a LUT for pipe types.
int Overlay::initOverlay() {
    int mdpVersion = qdutils::MDPVersion::getInstance().getMDPVersion();
    int numPipesXType[OV_MDP_PIPE_ANY] = {0};
    numPipesXType[OV_MDP_PIPE_RGB] =
            qdutils::MDPVersion::getInstance().getRGBPipes();
    numPipesXType[OV_MDP_PIPE_VG] =
            qdutils::MDPVersion::getInstance().getVGPipes();
    numPipesXType[OV_MDP_PIPE_DMA] =
            qdutils::MDPVersion::getInstance().getDMAPipes();

    int index = 0;
    for(int X = 0; X < (int)OV_MDP_PIPE_ANY; X++) { //iterate over types
        for(int j = 0; j < numPipesXType[X]; j++) { //iterate over num
            PipeBook::pipeTypeLUT[index] = (utils::eMdpPipeType)X;
            index++;
        }
    }

    if (mdpVersion < qdutils::MDSS_V5 && mdpVersion != qdutils::MDP_V3_0_4) {
        msmfb_mixer_info_req  req;
        mdp_mixer_info *minfo = NULL;
        char name[64];
        int fd = -1;
        for(int i = 0; i < MAX_FB_DEVICES; i++) {
            snprintf(name, 64, FB_DEVICE_TEMPLATE, i);
            ALOGD("initoverlay:: opening the device:: %s", name);
            fd = ::open(name, O_RDWR, 0);
            if(fd < 0) {
                ALOGE("cannot open framebuffer(%d)", i);
                return -1;
            }
            //Get the mixer configuration */
            req.mixer_num = i;
            if (ioctl(fd, MSMFB_MIXER_INFO, &req) == -1) {
                ALOGE("ERROR: MSMFB_MIXER_INFO ioctl failed");
                close(fd);
                return -1;
            }
            minfo = req.info;
            for (int j = 0; j < req.cnt; j++) {
                ALOGD("ndx=%d num=%d z_order=%d", minfo->pndx, minfo->pnum,
                      minfo->z_order);
                // clear any pipe connected to mixer including base pipe.
                int index = minfo->pndx;
                ALOGD("Unset overlay with index: %d at mixer %d", index, i);
                if(ioctl(fd, MSMFB_OVERLAY_UNSET, &index) == -1) {
                    ALOGE("ERROR: MSMFB_OVERLAY_UNSET failed");
                    close(fd);
                    return -1;
                }
                minfo++;
            }
            close(fd);
            fd = -1;
        }
    }

    FILE *displayDeviceFP = NULL;
    const int MAX_FRAME_BUFFER_NAME_SIZE = 128;
    char fbType[MAX_FRAME_BUFFER_NAME_SIZE];
    char msmFbTypePath[MAX_FRAME_BUFFER_NAME_SIZE];
    const char *strDtvPanel = "dtv panel";
    const char *strWbPanel = "writeback panel";

    for(int num = 1; num < MAX_FB_DEVICES; num++) {
        snprintf (msmFbTypePath, sizeof(msmFbTypePath),
                "/sys/class/graphics/fb%d/msm_fb_type", num);
        displayDeviceFP = fopen(msmFbTypePath, "r");

        if(displayDeviceFP){
            fread(fbType, sizeof(char), MAX_FRAME_BUFFER_NAME_SIZE,
                    displayDeviceFP);

            if(strncmp(fbType, strDtvPanel, strlen(strDtvPanel)) == 0) {
                sDpyFbMap[DPY_EXTERNAL] = num;
            } else if(strncmp(fbType, strWbPanel, strlen(strWbPanel)) == 0) {
                sDpyFbMap[DPY_WRITEBACK] = num;
            }

            fclose(displayDeviceFP);
        }
    }

    return 0;
}

bool Overlay::displayCommit(const int& fd) {
    //Commit
    struct mdp_display_commit info;
    memset(&info, 0, sizeof(struct mdp_display_commit));
    info.flags = MDP_DISPLAY_COMMIT_OVERLAY;
    if(!mdp_wrapper::displayCommit(fd, info)) {
       ALOGE("%s: commit failed", __func__);
       return false;
    }
    return true;
}

void Overlay::dump() const {
#if PIPE_DEBUG
    if(strlen(mDumpStr)) { //dump only on state change
        ALOGD("%s\n", mDumpStr);
    }
#endif
}

void Overlay::getDump(char *buf, size_t len) {
    int totalPipes = 0;
    const char *str = "\nOverlay State\n\n";
    strncat(buf, str, strlen(str));
    for(int i = 0; i < PipeBook::NUM_PIPES; i++) {
        if(mPipeBook[i].valid()) {
            mPipeBook[i].mPipe->getDump(buf, len);
            char str[64] = {'\0'};
            snprintf(str, 64, "Display=%d\n\n", mPipeBook[i].mDisplay);
            strncat(buf, str, strlen(str));
            totalPipes++;
        }
    }
    char str_pipes[64] = {'\0'};
    snprintf(str_pipes, 64, "Pipes=%d\n\n", totalPipes);
    strncat(buf, str_pipes, strlen(str_pipes));
}

void Overlay::clear(int dpy) {
    for(int i = 0; i < PipeBook::NUM_PIPES; i++) {
        if (mPipeBook[i].mDisplay == dpy) {
            // Mark as available for this round
            PipeBook::resetUse(i);
            PipeBook::resetAllocation(i);
            if(mPipeBook[i].valid()) {
                mPipeBook[i].mPipe->forceSet();
            }
        }
    }
}

void Overlay::PipeBook::init() {
    mPipe = NULL;
    mDisplay = DPY_UNUSED;
    mMixer = MIXER_UNUSED;
}

void Overlay::PipeBook::destroy() {
    if(mPipe) {
        delete mPipe;
        mPipe = NULL;
    }
    mDisplay = DPY_UNUSED;
    mMixer = MIXER_UNUSED;
}

Overlay* Overlay::sInstance = 0;
int Overlay::sDpyFbMap[DPY_MAX] = {0, -1, -1};
int Overlay::sDMAMode = DMA_LINE_MODE;
int Overlay::sForceSetBitmap = 0;
int Overlay::PipeBook::NUM_PIPES = 0;
int Overlay::PipeBook::sPipeUsageBitmap = 0;
int Overlay::PipeBook::sLastUsageBitmap = 0;
int Overlay::PipeBook::sAllocatedBitmap = 0;
utils::eMdpPipeType Overlay::PipeBook::pipeTypeLUT[utils::OV_MAX] =
    {utils::OV_MDP_PIPE_ANY};

}; // namespace overlay
