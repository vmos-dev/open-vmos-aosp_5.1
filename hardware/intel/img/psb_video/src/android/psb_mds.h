/*
 * Copyright (c) 2011 Intel Corporation. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    tianyang.zhu <tianyang.zhu@intel.com>
 */

#ifndef _PSB_MDS_H_
#define _PSB_MDS_H_

#ifdef TARGET_HAS_MULTIPLE_DISPLAY
#include <display/MultiDisplayService.h>
#ifdef PSBVIDEO_MRFL_VPP
#include <VPPSetting.h>
#endif
#endif

#ifdef TARGET_HAS_MULTIPLE_DISPLAY
enum {
    MDS_INIT_VALUE = 0,
    MDS_HDMI_VIDEO_ISPLAYING,
    MDS_WIDI_VIDEO_ISPLAYING,
};
#endif

namespace android {
namespace intel {

class psbMultiDisplayListener {
private:
    sp<IMDService>  mMds;
    sp<IMultiDisplayInfoProvider> mListener;
public:
    psbMultiDisplayListener();
    ~psbMultiDisplayListener();
    inline bool checkMode(int value, int bit) {
        return (value & bit) == bit ? true : false;
    }

    int  getMode();
    // only for WIDI video playback
    bool getDecoderOutputResolution(
            int32_t* width, int32_t* height,
            int32_t* offX, int32_t* offY,
            int32_t* bufW, int32_t* bufH);
    bool getVppState();
};

}; // namespace android
}; // namespace intel

#endif
