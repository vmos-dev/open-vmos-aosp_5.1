/*
 * Copyright (C) 2010 The Android Open Source Project
 * Copyright (C) 2012-2013, The Linux Foundation. All rights reserved.
 *
 * Not a Contribution, Apache license notifications and license are
 * retained for attribution purposes only.

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

#ifndef HWC_EXTERNAL_DISPLAY_H
#define HWC_EXTERNAL_DISPLAY_H

#include <linux/fb.h>

struct hwc_context_t;
struct msm_hdmi_mode_timing_info;

namespace qhwc {

//Type of scanning of EDID(Video Capability Data Block)
enum external_scansupport_type {
    EXT_SCAN_NOT_SUPPORTED      = 0,
    EXT_SCAN_ALWAYS_OVERSCANED  = 1,
    EXT_SCAN_ALWAYS_UNDERSCANED = 2,
    EXT_SCAN_BOTH_SUPPORTED     = 3
};

class ExternalDisplay
{
public:
    ExternalDisplay(hwc_context_t* ctx);
    ~ExternalDisplay();
    void setHPD(uint32_t startEnd);
    void setActionSafeDimension(int w, int h);
    bool isCEUnderscanSupported() { return mUnderscanSupported; }
    int configure();
    void getAttributes(int& width, int& height);
    int teardown();
    bool isConnected() {
        return  mHwcContext->dpyAttr[HWC_DISPLAY_EXTERNAL].connected;
    }

private:
    int getModeCount() const;
    void getEDIDModes(int *out) const;
    void setEDIDMode(int resMode);
    void setSPDInfo(const char* node, const char* property);
    void readCEUnderscanInfo();
    bool readResolution();
    int  parseResolution(char* edidStr, int* edidModes);
    void setResolution(int ID);
    bool openFrameBuffer();
    bool closeFrameBuffer();
    bool writeHPDOption(int userOption) const;
    bool isValidMode(int ID);
    int  getModeOrder(int mode);
    int  getUserMode();
    int  getBestMode();
    bool isInterlacedMode(int mode);
    void resetInfo();
    void setAttributes();
    void getAttrForMode(int& width, int& height, int& fps);

    int mFd;
    int mFbNum;
    int mCurrentMode;
    int mEDIDModes[64];
    int mModeCount;
    bool mUnderscanSupported;
    hwc_context_t *mHwcContext;
    fb_var_screeninfo mVInfo;
    // Holds all the HDMI modes and timing info supported by driver
    msm_hdmi_mode_timing_info* supported_video_mode_lut;
};

}; //qhwc
// ---------------------------------------------------------------------------
#endif //HWC_EXTERNAL_DISPLAY_H
