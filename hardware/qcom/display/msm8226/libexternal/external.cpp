/*
 * Copyright (C) 2010 The Android Open Source Project
 * Copyright (C) 2012-2013, The Linux Foundation. All rights reserved.
 *
 * Not a Contribution, Apache license notifications and license are
 * retained for attribution purposes only.
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

#define DEBUG 0
#include <ctype.h>
#include <fcntl.h>
#include <utils/threads.h>
#include <utils/Errors.h>
#include <utils/Log.h>

#include <linux/msm_mdp.h>
#include <video/msm_hdmi_modes.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <cutils/properties.h>
#include "hwc_utils.h"
#include "external.h"
#include "overlayUtils.h"
#include "overlay.h"
#include "qd_utils.h"

using namespace android;

namespace qhwc {
#define MAX_SYSFS_FILE_PATH             255
#define UNKNOWN_STRING                  "unknown"
#define SPD_NAME_LENGTH                 16
/* Max. resolution assignable to when downscale */
#define SUPPORTED_DOWNSCALE_EXT_AREA    (1920*1080)


int ExternalDisplay::configure() {
    if(!openFrameBuffer()) {
        ALOGE("%s: Failed to open FB: %d", __FUNCTION__, mFbNum);
        return -1;
    }
    readCEUnderscanInfo();
    readResolution();
    // TODO: Move this to activate
    /* Used for changing the resolution
     * getUserMode will get the preferred
     * mode set thru adb shell */
    int mode = getUserMode();
    if (mode == -1) {
        //Get the best mode and set
        mode = getBestMode();
    }
    setResolution(mode);
    setAttributes();
    // set system property
    property_set("hw.hdmiON", "1");
    return 0;
}

void ExternalDisplay::getAttributes(int& width, int& height) {
    int fps = 0;
    getAttrForMode(width, height, fps);
}

int ExternalDisplay::teardown() {
    closeFrameBuffer();
    resetInfo();
    // unset system property
    property_set("hw.hdmiON", "0");
    return 0;
}

ExternalDisplay::ExternalDisplay(hwc_context_t* ctx):mFd(-1),
    mCurrentMode(-1), mModeCount(0),
    mUnderscanSupported(false), mHwcContext(ctx)
{
    memset(&mVInfo, 0, sizeof(mVInfo));
    mFbNum = overlay::Overlay::getInstance()->getFbForDpy(HWC_DISPLAY_EXTERNAL);
    // disable HPD at start, it will be enabled later
    // when the display powers on
    // This helps for framework reboot or adb shell stop/start
    writeHPDOption(0);

    // for HDMI - retreive all the modes supported by the driver
    if(mFbNum != -1) {
        supported_video_mode_lut =
                        new msm_hdmi_mode_timing_info[HDMI_VFRMT_MAX];
        // Populate the mode table for supported modes
        MSM_HDMI_MODES_INIT_TIMINGS(supported_video_mode_lut);
        MSM_HDMI_MODES_SET_SUPP_TIMINGS(supported_video_mode_lut,
                                        MSM_HDMI_MODES_ALL);
        // Update the Source Product Information
        // Vendor Name
        setSPDInfo("vendor_name", "ro.product.manufacturer");
        // Product Description
        setSPDInfo("product_description", "ro.product.name");
    }
}
/* gets the product manufacturer and product name and writes it
 * to the sysfs node, so that the driver can get that information
 * Used to show QCOM 8974 instead of Input 1 for example
 */
void ExternalDisplay::setSPDInfo(const char* node, const char* property) {
    ssize_t err = -1;
    char info[PROPERTY_VALUE_MAX];
    char sysFsSPDFilePath[MAX_SYSFS_FILE_PATH];
    memset(sysFsSPDFilePath, 0, sizeof(sysFsSPDFilePath));
    snprintf(sysFsSPDFilePath , sizeof(sysFsSPDFilePath),
                 "/sys/devices/virtual/graphics/fb%d/%s",
                 mFbNum, node);
    int spdFile = open(sysFsSPDFilePath, O_RDWR, 0);
    if (spdFile < 0) {
        ALOGE("%s: file '%s' not found : ret = %d"
              "err str: %s",  __FUNCTION__, sysFsSPDFilePath,
              spdFile, strerror(errno));
    } else {
        memset(info, 0, sizeof(info));
        property_get(property, info, UNKNOWN_STRING);
        ALOGD_IF(DEBUG, "In %s: %s = %s", __FUNCTION__, property, info);
        if (strncmp(info, UNKNOWN_STRING, SPD_NAME_LENGTH)) {
            err = write(spdFile, info, strlen(info));
            if (err <= 0) {
                ALOGE("%s: file write failed for '%s'"
                      "err no = %d", __FUNCTION__, sysFsSPDFilePath, errno);
            }
        } else {
            ALOGD_IF(DEBUG, "%s: property_get failed for SPD %s",
                         __FUNCTION__, node);
        }
        close(spdFile);
    }
}

void ExternalDisplay::setHPD(uint32_t startEnd) {
    ALOGD_IF(DEBUG,"HPD enabled=%d", startEnd);
    writeHPDOption(startEnd);
}

void ExternalDisplay::setActionSafeDimension(int w, int h) {
    ALOGD_IF(DEBUG,"ActionSafe w=%d h=%d", w, h);
    char actionsafeWidth[PROPERTY_VALUE_MAX];
    char actionsafeHeight[PROPERTY_VALUE_MAX];
    snprintf(actionsafeWidth, sizeof(actionsafeWidth), "%d", w);
    property_set("persist.sys.actionsafe.width", actionsafeWidth);
    snprintf(actionsafeHeight, sizeof(actionsafeHeight), "%d", h);
    property_set("persist.sys.actionsafe.height", actionsafeHeight);
}

int ExternalDisplay::getModeCount() const {
    ALOGD_IF(DEBUG,"HPD mModeCount=%d", mModeCount);
    return mModeCount;
}

void ExternalDisplay::getEDIDModes(int *out) const {
    for(int i = 0;i < mModeCount;i++) {
        out[i] = mEDIDModes[i];
    }
}

void ExternalDisplay::readCEUnderscanInfo()
{
    int hdmiScanInfoFile = -1;
    ssize_t len = -1;
    char scanInfo[17];
    char *ce_info_str = NULL;
    char *save_ptr;
    const char token[] = ", \n";
    int ce_info = -1;
    char sysFsScanInfoFilePath[MAX_SYSFS_FILE_PATH];
    snprintf(sysFsScanInfoFilePath, sizeof(sysFsScanInfoFilePath),
            "/sys/devices/virtual/graphics/fb%d/"
                                   "scan_info", mFbNum);

    memset(scanInfo, 0, sizeof(scanInfo));
    hdmiScanInfoFile = open(sysFsScanInfoFilePath, O_RDONLY, 0);
    if (hdmiScanInfoFile < 0) {
        ALOGD_IF(DEBUG, "%s: scan_info file '%s' not found",
                                __FUNCTION__, sysFsScanInfoFilePath);
        return;
    } else {
        len = read(hdmiScanInfoFile, scanInfo, sizeof(scanInfo)-1);
        ALOGD("%s: Scan Info string: %s length = %zd",
                 __FUNCTION__, scanInfo, len);
        if (len <= 0) {
            close(hdmiScanInfoFile);
            ALOGE("%s: Scan Info file empty '%s'",
                                __FUNCTION__, sysFsScanInfoFilePath);
            return;
        }
        scanInfo[len] = '\0';  /* null terminate the string */
        close(hdmiScanInfoFile);
    }

    /*
     * The scan_info contains the three fields
     * PT - preferred video format
     * IT - video format
     * CE video format - containing the underscan support information
     */

    /* PT */
    ce_info_str = strtok_r(scanInfo, token, &save_ptr);
    if (ce_info_str) {
        /* IT */
        ce_info_str = strtok_r(NULL, token, &save_ptr);
        if (ce_info_str) {
            /* CE */
            ce_info_str = strtok_r(NULL, token, &save_ptr);
            if (ce_info_str)
                ce_info = atoi(ce_info_str);
        }
    }

    if (ce_info_str) {
        // ce_info contains the underscan information
        if (ce_info == EXT_SCAN_ALWAYS_UNDERSCANED ||
            ce_info == EXT_SCAN_BOTH_SUPPORTED)
            // if TV supported underscan, then driver will always underscan
            // hence no need to apply action safe rectangle
            mUnderscanSupported = true;
    } else {
        ALOGE("%s: scan_info string error", __FUNCTION__);
    }

    // Store underscan support info in a system property
    const char* prop = (mUnderscanSupported) ? "1" : "0";
    property_set("hw.underscan_supported", prop);
    return;
}

ExternalDisplay::~ExternalDisplay()
{
    delete [] supported_video_mode_lut;
    closeFrameBuffer();
}

/*
 * sets the fb_var_screeninfo from the hdmi_mode_timing_info
 */
void setDisplayTiming(struct fb_var_screeninfo &info,
                                const msm_hdmi_mode_timing_info* mode)
{
    info.reserved[0] = 0;
    info.reserved[1] = 0;
    info.reserved[2] = 0;
#ifndef FB_METADATA_VIDEO_INFO_CODE_SUPPORT
    info.reserved[3] = (info.reserved[3] & 0xFFFF) |
              (mode->video_format << 16);
#endif
    info.xoffset = 0;
    info.yoffset = 0;
    info.xres = mode->active_h;
    info.yres = mode->active_v;

    info.pixclock = (mode->pixel_freq)*1000;
    info.vmode = mode->interlaced ?
                    FB_VMODE_INTERLACED : FB_VMODE_NONINTERLACED;

    info.right_margin = mode->front_porch_h;
    info.hsync_len = mode->pulse_width_h;
    info.left_margin = mode->back_porch_h;
    info.lower_margin = mode->front_porch_v;
    info.vsync_len = mode->pulse_width_v;
    info.upper_margin = mode->back_porch_v;
}

int ExternalDisplay::parseResolution(char* edidStr, int* edidModes)
{
    char delim = ',';
    int count = 0;
    char *start, *end;
    // EDIDs are string delimited by ','
    // Ex: 16,4,5,3,32,34,1
    // Parse this string to get mode(int)
    start = (char*) edidStr;
    end = &delim;
    while(*end == delim) {
        edidModes[count] = (int) strtol(start, &end, 10);
        start = end+1;
        count++;
    }
    ALOGD_IF(DEBUG, "In %s: count = %d", __FUNCTION__, count);
    for (int i = 0; i < count; i++)
        ALOGD_IF(DEBUG, "Mode[%d] = %d", i, edidModes[i]);
    return count;
}

bool ExternalDisplay::readResolution()
{
    char sysFsEDIDFilePath[MAX_SYSFS_FILE_PATH];
    snprintf(sysFsEDIDFilePath , sizeof(sysFsEDIDFilePath),
            "/sys/devices/virtual/graphics/fb%d/edid_modes", mFbNum);

    int hdmiEDIDFile = open(sysFsEDIDFilePath, O_RDONLY, 0);
    ssize_t len = -1;
    char edidStr[128] = {'\0'};

    if (hdmiEDIDFile < 0) {
        ALOGE("%s: edid_modes file '%s' not found",
                 __FUNCTION__, sysFsEDIDFilePath);
        return false;
    } else {
        len = read(hdmiEDIDFile, edidStr, sizeof(edidStr)-1);
        ALOGD_IF(DEBUG, "%s: EDID string: %s length = %zd",
                 __FUNCTION__, edidStr, len);
        if ( len <= 0) {
            ALOGE("%s: edid_modes file empty '%s'",
                     __FUNCTION__, sysFsEDIDFilePath);
            edidStr[0] = '\0';
        }
        else {
            while (len > 1 && isspace(edidStr[len-1])) {
                --len;
            }
            edidStr[len] = '\0';
        }
        close(hdmiEDIDFile);
    }
    if(len > 0) {
        // Get EDID modes from the EDID strings
        mModeCount = parseResolution(edidStr, mEDIDModes);
        ALOGD_IF(DEBUG, "%s: mModeCount = %d", __FUNCTION__,
                 mModeCount);
    }

    return (len > 0);
}

bool ExternalDisplay::openFrameBuffer()
{
    if (mFd == -1) {
        char strDevPath[MAX_SYSFS_FILE_PATH];
        snprintf(strDevPath, MAX_SYSFS_FILE_PATH, "/dev/graphics/fb%d", mFbNum);
        mFd = open(strDevPath, O_RDWR);
        if (mFd < 0)
            ALOGE("%s: %s is not available", __FUNCTION__, strDevPath);
        mHwcContext->dpyAttr[HWC_DISPLAY_EXTERNAL].fd = mFd;
    }
    return (mFd > 0);
}

bool ExternalDisplay::closeFrameBuffer()
{
    int ret = 0;
    if(mFd >= 0) {
        ret = close(mFd);
        mFd = -1;
    }
    mHwcContext->dpyAttr[HWC_DISPLAY_EXTERNAL].fd = mFd;
    return (ret == 0);
}

// clears the vinfo, edid, best modes
void ExternalDisplay::resetInfo()
{
    memset(&mVInfo, 0, sizeof(mVInfo));
    memset(mEDIDModes, 0, sizeof(mEDIDModes));
    mModeCount = 0;
    mCurrentMode = -1;
    mUnderscanSupported = false;
    // Reset the underscan supported system property
    const char* prop = "0";
    property_set("hw.underscan_supported", prop);
}

int ExternalDisplay::getModeOrder(int mode)
{
    // XXX: We dont support interlaced modes but having
    // it here for future
    switch (mode) {
        default:
        case HDMI_VFRMT_1440x480i60_4_3:
            return 1; // 480i 4:3
        case HDMI_VFRMT_1440x480i60_16_9:
            return 2; // 480i 16:9
        case HDMI_VFRMT_1440x576i50_4_3:
            return 3; // i576i 4:3
        case HDMI_VFRMT_1440x576i50_16_9:
            return 4; // 576i 16:9
        case HDMI_VFRMT_1920x1080i60_16_9:
            return 5; // 1080i 16:9
        case HDMI_VFRMT_640x480p60_4_3:
            return 6; // 640x480 4:3
        case HDMI_VFRMT_720x480p60_4_3:
            return 7; // 480p 4:3
        case HDMI_VFRMT_720x480p60_16_9:
            return 8; // 480p 16:9
        case HDMI_VFRMT_720x576p50_4_3:
            return 9; // 576p 4:3
        case HDMI_VFRMT_720x576p50_16_9:
            return 10; // 576p 16:9
        case HDMI_VFRMT_1024x768p60_4_3:
            return 11; // 768p 4:3 Vesa format
        case HDMI_VFRMT_1280x1024p60_5_4:
            return 12; // 1024p Vesa format
        case HDMI_VFRMT_1280x720p50_16_9:
            return 13; // 720p@50Hz
        case HDMI_VFRMT_1280x720p60_16_9:
            return 14; // 720p@60Hz
        case HDMI_VFRMT_1920x1080p24_16_9:
            return 15; //1080p@24Hz
        case HDMI_VFRMT_1920x1080p25_16_9:
            return 16; //108-p@25Hz
        case HDMI_VFRMT_1920x1080p30_16_9:
            return 17; //1080p@30Hz
        case HDMI_VFRMT_1920x1080p50_16_9:
            return 18; //1080p@50Hz
        case HDMI_VFRMT_1920x1080p60_16_9:
            return 19; //1080p@60Hz
        case HDMI_VFRMT_2560x1600p60_16_9:
            return 20; //WQXGA@60Hz541
        case HDMI_VFRMT_3840x2160p24_16_9:
            return 21;//2160@24Hz
        case HDMI_VFRMT_3840x2160p25_16_9:
            return 22;//2160@25Hz
        case HDMI_VFRMT_3840x2160p30_16_9:
            return 23; //2160@30Hz
        case HDMI_VFRMT_4096x2160p24_16_9:
            return 24; //4kx2k@24Hz
    }
}

/// Returns the user mode set(if any) using adb shell
int ExternalDisplay::getUserMode() {
    /* Based on the property set the resolution */
    char property_value[PROPERTY_VALUE_MAX];
    property_get("hw.hdmi.resolution", property_value, "-1");
    int mode = atoi(property_value);
    // We dont support interlaced modes
    if(isValidMode(mode) && !isInterlacedMode(mode)) {
        ALOGD_IF(DEBUG, "%s: setting the HDMI mode = %d", __FUNCTION__, mode);
        return mode;
    }
    return -1;
}

// Get the best mode for the current HD TV
int ExternalDisplay::getBestMode() {
    int bestOrder = 0;
    int bestMode = HDMI_VFRMT_640x480p60_4_3;
    // for all the edid read, get the best mode
    for(int i = 0; i < mModeCount; i++) {
        int mode = mEDIDModes[i];
        int order = getModeOrder(mode);
        if (order > bestOrder) {
            bestOrder = order;
            bestMode = mode;
        }
    }
    return bestMode;
}

inline bool ExternalDisplay::isValidMode(int ID)
{
    bool valid = false;
    for (int i = 0; i < mModeCount; i++) {
        if(ID == mEDIDModes[i]) {
            valid = true;
            break;
        }
    }
    return valid;
}

// returns true if the mode(ID) is interlaced mode format
bool ExternalDisplay::isInterlacedMode(int ID) {
    bool interlaced = false;
    switch(ID) {
        case HDMI_VFRMT_1440x480i60_4_3:
        case HDMI_VFRMT_1440x480i60_16_9:
        case HDMI_VFRMT_1440x576i50_4_3:
        case HDMI_VFRMT_1440x576i50_16_9:
        case HDMI_VFRMT_1920x1080i60_16_9:
            interlaced = true;
            break;
        default:
            interlaced = false;
            break;
    }
    return interlaced;
}

void ExternalDisplay::setResolution(int ID)
{
    int ret = 0;
    ret = ioctl(mFd, FBIOGET_VSCREENINFO, &mVInfo);
    if(ret < 0) {
        ALOGD("In %s: FBIOGET_VSCREENINFO failed Err Str = %s", __FUNCTION__,
                                                            strerror(errno));
    }
    ALOGD_IF(DEBUG, "%s: GET Info<ID=%d %dx%d (%d,%d,%d),"
            "(%d,%d,%d) %dMHz>", __FUNCTION__,
            mVInfo.reserved[3], mVInfo.xres, mVInfo.yres,
            mVInfo.right_margin, mVInfo.hsync_len, mVInfo.left_margin,
            mVInfo.lower_margin, mVInfo.vsync_len, mVInfo.upper_margin,
            mVInfo.pixclock/1000/1000);
    //If its a new ID - update var_screeninfo
    if ((isValidMode(ID)) && mCurrentMode != ID) {
        const struct msm_hdmi_mode_timing_info *mode =
            &supported_video_mode_lut[0];
        for (unsigned int i = 0; i < HDMI_VFRMT_MAX; ++i) {
            const struct msm_hdmi_mode_timing_info *cur =
                                        &supported_video_mode_lut[i];
            if (cur->video_format == (uint32_t)ID) {
                mode = cur;
                break;
            }
        }
        setDisplayTiming(mVInfo, mode);
        ALOGD_IF(DEBUG, "%s: SET Info<ID=%d => Info<ID=%d %dx %d"
                 "(%d,%d,%d), (%d,%d,%d) %dMHz>", __FUNCTION__, ID,
                 mode->video_format, mVInfo.xres, mVInfo.yres,
                 mVInfo.right_margin, mVInfo.hsync_len, mVInfo.left_margin,
                 mVInfo.lower_margin, mVInfo.vsync_len, mVInfo.upper_margin,
                 mVInfo.pixclock/1000/1000);
#ifdef FB_METADATA_VIDEO_INFO_CODE_SUPPORT
        struct msmfb_metadata metadata;
        memset(&metadata, 0 , sizeof(metadata));
        metadata.op = metadata_op_vic;
        metadata.data.video_info_code = mode->video_format;
        if (ioctl(mFd, MSMFB_METADATA_SET, &metadata) == -1) {
            ALOGD("In %s: MSMFB_METADATA_SET failed Err Str = %s",
                                                 __FUNCTION__, strerror(errno));
        }
#endif
        mVInfo.activate = FB_ACTIVATE_NOW | FB_ACTIVATE_ALL | FB_ACTIVATE_FORCE;
        ret = ioctl(mFd, FBIOPUT_VSCREENINFO, &mVInfo);
        if(ret < 0) {
            ALOGD("In %s: FBIOPUT_VSCREENINFO failed Err Str = %s",
                                                 __FUNCTION__, strerror(errno));
        }
        mCurrentMode = ID;
    }
}

bool ExternalDisplay::writeHPDOption(int userOption) const
{
    bool ret = true;
    if(mFbNum != -1) {
        char sysFsHPDFilePath[MAX_SYSFS_FILE_PATH];
        snprintf(sysFsHPDFilePath ,sizeof(sysFsHPDFilePath),
                 "/sys/devices/virtual/graphics/fb%d/hpd", mFbNum);
        int hdmiHPDFile = open(sysFsHPDFilePath,O_RDWR, 0);
        if (hdmiHPDFile < 0) {
            ALOGE("%s: state file '%s' not found : ret%d err str: %s",
                  __FUNCTION__, sysFsHPDFilePath, hdmiHPDFile, strerror(errno));
            ret = false;
        } else {
            ssize_t err = -1;
            ALOGD_IF(DEBUG, "%s: option = %d", __FUNCTION__, userOption);
            if(userOption)
                err = write(hdmiHPDFile, "1", 2);
            else
                err = write(hdmiHPDFile, "0" , 2);
            if (err <= 0) {
                ALOGE("%s: file write failed '%s'", __FUNCTION__,
                      sysFsHPDFilePath);
                ret = false;
            }
            close(hdmiHPDFile);
        }
    }
    return ret;
}


void ExternalDisplay::setAttributes() {
    int width = 0, height = 0, fps = 0;
    getAttrForMode(width, height, fps);
    ALOGD("ExtDisplay setting xres = %d, yres = %d", width, height);
    if(mHwcContext) {
        // Always set dpyAttr res to mVInfo res
        mHwcContext->dpyAttr[HWC_DISPLAY_EXTERNAL].xres = width;
        mHwcContext->dpyAttr[HWC_DISPLAY_EXTERNAL].yres = height;
        mHwcContext->dpyAttr[HWC_DISPLAY_EXTERNAL].mDownScaleMode = false;
        if(mHwcContext->mOverlay->isUIScalingOnExternalSupported()
                && mHwcContext->mMDPDownscaleEnabled) {
            int priW = mHwcContext->dpyAttr[HWC_DISPLAY_PRIMARY].xres;
            int priH = mHwcContext->dpyAttr[HWC_DISPLAY_PRIMARY].yres;
            // if primary resolution is more than the hdmi resolution
            // configure dpy attr to primary resolution and set
            // downscale mode
            // Restrict this upto 1080p resolution max
            if(((priW * priH) > (width * height)) &&
               ((priW * priH) <= SUPPORTED_DOWNSCALE_EXT_AREA)) {
                // tmpW and tmpH will hold the primary dimensions before we
                // update the aspect ratio if necessary.
                int tmpW = priW;
                int tmpH = priH;
                // HDMI is always in landscape, so always assign the higher
                // dimension to hdmi's xres
                if(priH > priW) {
                    tmpW = priH;
                    tmpH = priW;
                }
                // The aspect ratios of the external and primary displays
                // can be different. As a result, directly assigning primary
                // resolution could lead to an incorrect final image.
                // We get around this by calculating a new resolution by
                // keeping aspect ratio intact.
                hwc_rect r = {0, 0, 0, 0};
                getAspectRatioPosition(tmpW, tmpH, width, height, r);
                mHwcContext->dpyAttr[HWC_DISPLAY_EXTERNAL].xres =
                                                              r.right - r.left;
                mHwcContext->dpyAttr[HWC_DISPLAY_EXTERNAL].yres =
                                                              r.bottom - r.top;
                // Set External Display MDP Downscale mode indicator
                mHwcContext->dpyAttr[HWC_DISPLAY_EXTERNAL].mDownScaleMode =true;
            }
        }
        //Initialize the display viewFrame info
        mHwcContext->mViewFrame[HWC_DISPLAY_EXTERNAL].left = 0;
        mHwcContext->mViewFrame[HWC_DISPLAY_EXTERNAL].top = 0;
        mHwcContext->mViewFrame[HWC_DISPLAY_EXTERNAL].right =
            (int)mHwcContext->dpyAttr[HWC_DISPLAY_EXTERNAL].xres;
        mHwcContext->mViewFrame[HWC_DISPLAY_EXTERNAL].bottom =
            (int)mHwcContext->dpyAttr[HWC_DISPLAY_EXTERNAL].yres;
        mHwcContext->dpyAttr[HWC_DISPLAY_EXTERNAL].vsync_period =
                (int) 1000000000l / fps;
    }
}

void ExternalDisplay::getAttrForMode(int& width, int& height, int& fps) {
    switch (mCurrentMode) {
        case HDMI_VFRMT_640x480p60_4_3:
            width = 640;
            height = 480;
            fps = 60;
            break;
        case HDMI_VFRMT_720x480p60_4_3:
        case HDMI_VFRMT_720x480p60_16_9:
            width = 720;
            height = 480;
            fps = 60;
            break;
        case HDMI_VFRMT_720x576p50_4_3:
        case HDMI_VFRMT_720x576p50_16_9:
            width = 720;
            height = 576;
            fps = 50;
            break;
        case HDMI_VFRMT_1280x720p50_16_9:
            width = 1280;
            height = 720;
            fps = 50;
            break;
        case HDMI_VFRMT_1280x720p60_16_9:
            width = 1280;
            height = 720;
            fps = 60;
            break;
        case HDMI_VFRMT_1280x1024p60_5_4:
            width = 1280;
            height = 1024;
            fps = 60;
            break;
        case HDMI_VFRMT_1024x768p60_4_3:
            width = 1024;
            height = 768;
            fps = 60;
            break;
        case HDMI_VFRMT_1920x1080p24_16_9:
            width = 1920;
            height = 1080;
            fps = 24;
            break;
        case HDMI_VFRMT_1920x1080p25_16_9:
            width = 1920;
            height = 1080;
            fps = 25;
            break;
        case HDMI_VFRMT_1920x1080p30_16_9:
            width = 1920;
            height = 1080;
            fps = 30;
            break;
        case HDMI_VFRMT_1920x1080p50_16_9:
            width = 1920;
            height = 1080;
            fps = 50;
            break;
        case HDMI_VFRMT_1920x1080p60_16_9:
            width = 1920;
            height = 1080;
            fps = 60;
            break;
        case HDMI_VFRMT_2560x1600p60_16_9:
            width = 2560;
            height = 1600;
            fps = 60;
            break;
        case HDMI_VFRMT_3840x2160p24_16_9:
            width = 3840;
            height = 2160;
            fps = 24;
            break;
        case HDMI_VFRMT_3840x2160p25_16_9:
            width = 3840;
            height = 2160;
            fps = 25;
            break;
        case HDMI_VFRMT_3840x2160p30_16_9:
            width = 3840;
            height = 2160;
            fps = 30;
            break;
        case HDMI_VFRMT_4096x2160p24_16_9:
            width = 4096;
            height = 2160;
            fps = 24;
            break;

    }
}

};
