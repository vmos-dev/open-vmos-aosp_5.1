/*
 * Copyright (C) 2010 The Android Open Source Project
 * Copyright (C) 2012, The Linux Foundation. All rights reserved.
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

#include <utils/threads.h>
#include <linux/fb.h>

struct hwc_context_t;

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
    int getModeCount() const;
    void getEDIDModes(int *out) const;
    bool isCEUnderscanSupported() { return mUnderscanSupported; }
    void setExternalDisplay(bool connected, int extFbNum = 0);
    bool isExternalConnected() { return mConnected;};
    void  setExtDpyNum(int extDpyNum) { mExtDpyNum = extDpyNum;};
    void setHPD(uint32_t startEnd);
    void setEDIDMode(int resMode);
    void setActionSafeDimension(int w, int h);
    void processUEventOnline(const char *str);
    void processUEventOffline(const char *str);

private:
    void readCEUnderscanInfo();
    bool readResolution();
    int  parseResolution(char* edidStr, int* edidModes);
    void setResolution(int ID);
    bool openFrameBuffer(int fbNum);
    bool closeFrameBuffer();
    bool writeHPDOption(int userOption) const;
    bool isValidMode(int ID);
    void handleUEvent(char* str, int len);
    int  getModeOrder(int mode);
    int  getUserMode();
    int  getBestMode();
    bool isInterlacedMode(int mode);
    void resetInfo();
    void setDpyHdmiAttr();
    void setDpyWfdAttr();
    void getAttrForMode(int& width, int& height, int& fps);
    void updateExtDispDevFbIndex();
    int  configureHDMIDisplay();
    int  configureWFDDisplay();
    int  teardownHDMIDisplay();
    int  teardownWFDDisplay();
    int  getExtFbNum(int &fbNum);

    mutable android::Mutex mExtDispLock;
    int mFd;
    int mCurrentMode;
    int mConnected;
    int mConnectedFbNum;
    int mResolutionMode;
    char mEDIDs[128];
    int mEDIDModes[64];
    int mModeCount;
    bool mUnderscanSupported;
    hwc_context_t *mHwcContext;
    fb_var_screeninfo mVInfo;
    int mHdmiFbNum;
    int mWfdFbNum;
    int mExtDpyNum;
};

}; //qhwc
// ---------------------------------------------------------------------------
#endif //HWC_EXTERNAL_DISPLAY_H
