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


#ifndef HWC_VIRTUAL_DISPLAY_H
#define HWC_VIRTUAL_DISPLAY_H

#include <linux/fb.h>

struct hwc_context_t;

namespace qhwc {

class VirtualDisplay
{
public:
    VirtualDisplay(hwc_context_t* ctx);
    ~VirtualDisplay();
    int  configure();
    void getAttributes(int& width, int& height);
    int  teardown();
    bool isConnected() {
        return  mHwcContext->dpyAttr[HWC_DISPLAY_VIRTUAL].connected;
    }
private:
    bool openFrameBuffer();
    bool closeFrameBuffer();
    void setAttributes();
    void initResolution(uint32_t &extW, uint32_t &extH);
    void setToPrimary(uint32_t maxArea, uint32_t priW, uint32_t priH,
                      uint32_t &extW, uint32_t &extH);
    void setDownScaleMode(uint32_t maxArea);

    int mFd;
    hwc_context_t *mHwcContext;
    fb_var_screeninfo mVInfo;
};
}; //qhwc
// ---------------------------------------------------------------------------
#endif //HWC_VIRTUAL_DISPLAY_H
