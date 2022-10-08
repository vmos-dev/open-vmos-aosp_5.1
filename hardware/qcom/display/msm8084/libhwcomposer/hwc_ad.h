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

#ifndef HWC_AD_H
#define HWC_AD_H

#include <overlayUtils.h>
#include <hwc_utils.h>

struct hwc_context_t;

namespace qhwc {

class AssertiveDisplay {
public:
    AssertiveDisplay(hwc_context_t *ctx);
    void markDoable(hwc_context_t *ctx, const hwc_display_contents_1_t* list);
    bool prepare(hwc_context_t *ctx, const hwc_rect_t& crop,
            const overlay::utils::Whf& whf,
            const private_handle_t *hnd);
    bool draw(hwc_context_t *ctx, int fd, uint32_t offset);
    //Resets a few members on each draw round
    void reset() { mDoable = false;
            mDest = overlay::utils::OV_INVALID;
    }
    bool isDoable() const { return mDoable; }
    bool isModeOn() const { return (mWbFd >= 0); }
    int getDstFd() const;
    uint32_t getDstOffset() const;

private:
    //State of feature turned on and off
    int mWbFd;
    bool mDoable;
    //State of feature existence on certain devices and configs.
    bool mFeatureEnabled;
    overlay::utils::eDest mDest;
};

}
#endif
