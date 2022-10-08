/*
* Copyright (C) 2008 The Android Open Source Project
* Copyright (c) 2010-2012, The Linux Foundation. All rights reserved.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include <cutils/properties.h>
#include "overlayCtrlData.h"
#include "gralloc_priv.h" //for interlace

namespace overlay{

bool Ctrl::init(uint32_t fbnum) {
    // MDP/FD init
    if(!mMdp.init(fbnum)) {
        ALOGE("Ctrl failed to init fbnum=%d", fbnum);
        return false;
    }

    if(!getScreenInfo(mInfo)) {
        ALOGE("Ctrl failed to getScreenInfo");
        return false;
    }

    return true;
}

bool Ctrl::setSource(const utils::PipeArgs& args)
{
    return mMdp.setSource(args);
}

bool Ctrl::setPosition(const utils::Dim& dim)
{
    if(!dim.check(mInfo.mFBWidth, mInfo.mFBHeight)) {
        ALOGE("Ctrl setPosition error in dim");
        dim.dump();
        return false;
    }

    if(!mMdp.setPosition(dim, mInfo.mFBWidth, mInfo.mFBHeight)) {
        ALOGE("Ctrl failed MDP setPosition");
        return false;
    }
    return true;
}

bool Ctrl::setTransform(const utils::eTransform& orient)
{
    if(!mMdp.setTransform(orient)) {
        ALOGE("Ctrl setTransform failed for Mdp");
        return false;
    }
    return true;
}

void Ctrl::setRotatorUsed(const bool& rotUsed) {
    mMdp.setRotatorUsed(rotUsed);
}

bool Ctrl::setCrop(const utils::Dim& d)
{
    if(!mMdp.setCrop(d)) {
        ALOGE("Data setCrop failed in MDP setCrop");
        return false;
    }
    return true;
}

utils::FrameBufferInfo* utils::FrameBufferInfo::sFBInfoInstance = 0;

void Ctrl::dump() const {
    ALOGE("== Dump Ctrl start ==");
    mInfo.dump("mInfo");
    mMdp.dump();
    ALOGE("== Dump Ctrl end ==");
}

} // overlay
