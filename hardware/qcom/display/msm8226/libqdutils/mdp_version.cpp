/*
 * Copyright (c) 2012-2014, The Linux Foundation. All rights reserved.

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of The Linux Foundation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
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
#include <cutils/log.h>
#include <linux/msm_mdp.h>
#include "mdp_version.h"

#define DEBUG 0

ANDROID_SINGLETON_STATIC_INSTANCE(qdutils::MDPVersion);
namespace qdutils {

#define TOKEN_PARAMS_DELIM  "="

#ifndef MDSS_MDP_HW_REV_100
#define MDSS_MDP_HW_REV_100 0x10000000 //8974 v1
#endif
#ifndef MDSS_MDP_HW_REV_101
#define MDSS_MDP_HW_REV_101 0x10010000 //8x26
#endif
#ifndef MDSS_MDP_HW_REV_102
#define MDSS_MDP_HW_REV_102 0x10020000 //8974 v2
#endif
#ifndef MDSS_MDP_HW_REV_103
#define MDSS_MDP_HW_REV_103 0x10030000 //8084
#endif
#ifndef MDSS_MDP_HW_REV_104
#define MDSS_MDP_HW_REV_104 0x10040000 //Next version
#endif
#ifndef MDSS_MDP_HW_REV_105
#define MDSS_MDP_HW_REV_105 0x10050000 //8994
#endif
#ifndef MDSS_MDP_HW_REV_106
#define MDSS_MDP_HW_REV_106 0x10060000 //8x16
#endif
#ifndef MDSS_MDP_HW_REV_107
#define MDSS_MDP_HW_REV_107 0x10070000 //Next version
#endif
#ifndef MDSS_MDP_HW_REV_108
#define MDSS_MDP_HW_REV_108 0x10080000 //8x39 & 8x36
#endif
#ifndef MDSS_MDP_HW_REV_109
#define MDSS_MDP_HW_REV_109 0x10090000 //Next version
#endif
#ifndef MDSS_MDP_HW_REV_200
#define MDSS_MDP_HW_REV_200 0x20000000 //8092
#endif
#ifndef MDSS_MDP_HW_REV_206
#define MDSS_MDP_HW_REV_206 0x20060000 //Future
#endif

MDPVersion::MDPVersion()
{
    mMDPVersion = MDSS_V5;
    mMdpRev = 0;
    mRGBPipes = 0;
    mVGPipes = 0;
    mDMAPipes = 0;
    mFeatures = 0;
    mMDPUpscale = 0;
    mMDPDownscale = 0;
    mMacroTileEnabled = false;
    mLowBw = 0;
    mHighBw = 0;
    mSourceSplit = false;
    mSourceSplitAlways = false;
    mRGBHasNoScalar = false;
    mRotDownscale = false;

    updatePanelInfo();

    if(!updateSysFsInfo()) {
        ALOGE("Unable to read display sysfs node");
    }
    if (mMdpRev == MDP_V3_0_4){
        mMDPVersion = MDP_V3_0_4;
    }

    mHasOverlay = false;
    if((mMDPVersion >= MDP_V4_0) ||
       (mMDPVersion == MDP_V_UNKNOWN) ||
       (mMDPVersion == MDP_V3_0_4))
        mHasOverlay = true;
    if(!updateSplitInfo()) {
        ALOGE("Unable to read display split node");
    }
}

MDPVersion::~MDPVersion() {
    close(mFd);
}

int MDPVersion::tokenizeParams(char *inputParams, const char *delim,
                                char* tokenStr[], int *idx) {
    char *tmp_token = NULL;
    char *temp_ptr;
    int index = 0;
    if (!inputParams) {
        return -1;
    }
    tmp_token = strtok_r(inputParams, delim, &temp_ptr);
    while (tmp_token != NULL) {
        tokenStr[index++] = tmp_token;
        tmp_token = strtok_r(NULL, " ", &temp_ptr);
    }
    *idx = index;
    return 0;
}
// This function reads the sysfs node to read the primary panel type
// and updates information accordingly
void  MDPVersion::updatePanelInfo() {
    FILE *displayDeviceFP = NULL;
    FILE *panelInfoNodeFP = NULL;
    const int MAX_FRAME_BUFFER_NAME_SIZE = 128;
    char fbType[MAX_FRAME_BUFFER_NAME_SIZE];
    const char *strCmdPanel = "mipi dsi cmd panel";
    const char *strVideoPanel = "mipi dsi video panel";
    const char *strLVDSPanel = "lvds panel";
    const char *strEDPPanel = "edp panel";

    displayDeviceFP = fopen("/sys/class/graphics/fb0/msm_fb_type", "r");
    if(displayDeviceFP){
        fread(fbType, sizeof(char), MAX_FRAME_BUFFER_NAME_SIZE,
                displayDeviceFP);
        if(strncmp(fbType, strCmdPanel, strlen(strCmdPanel)) == 0) {
            mPanelInfo.mType = MIPI_CMD_PANEL;
        }
        else if(strncmp(fbType, strVideoPanel, strlen(strVideoPanel)) == 0) {
            mPanelInfo.mType = MIPI_VIDEO_PANEL;
        }
        else if(strncmp(fbType, strLVDSPanel, strlen(strLVDSPanel)) == 0) {
            mPanelInfo.mType = LVDS_PANEL;
        }
        else if(strncmp(fbType, strEDPPanel, strlen(strEDPPanel)) == 0) {
            mPanelInfo.mType = EDP_PANEL;
        }
        fclose(displayDeviceFP);
    } else {
        ALOGE("Unable to read Primary Panel Information");
    }

    panelInfoNodeFP = fopen("/sys/class/graphics/fb0/msm_fb_panel_info", "r");
    if(panelInfoNodeFP){
        size_t len = PAGE_SIZE;
        ssize_t read;
        char *readLine = (char *) malloc (len);
        while((read = getline((char **)&readLine, &len,
                              panelInfoNodeFP)) != -1) {
            int token_ct=0;
            char *tokens[10];
            memset(tokens, 0, sizeof(tokens));

            if(!tokenizeParams(readLine, TOKEN_PARAMS_DELIM, tokens,
                               &token_ct)) {
                if(!strncmp(tokens[0], "pu_en", strlen("pu_en"))) {
                    mPanelInfo.mPartialUpdateEnable = atoi(tokens[1]);
                    ALOGI("PartialUpdate status: %s",
                          mPanelInfo.mPartialUpdateEnable? "Enabled" :
                          "Disabled");
                }
                if(!strncmp(tokens[0], "xstart", strlen("xstart"))) {
                    mPanelInfo.mLeftAlign = atoi(tokens[1]);
                    ALOGI("Left Align: %d", mPanelInfo.mLeftAlign);
                }
                if(!strncmp(tokens[0], "walign", strlen("walign"))) {
                    mPanelInfo.mWidthAlign = atoi(tokens[1]);
                    ALOGI("Width Align: %d", mPanelInfo.mWidthAlign);
                }
                if(!strncmp(tokens[0], "ystart", strlen("ystart"))) {
                    mPanelInfo.mTopAlign = atoi(tokens[1]);
                    ALOGI("Top Align: %d", mPanelInfo.mTopAlign);
                }
                if(!strncmp(tokens[0], "halign", strlen("halign"))) {
                    mPanelInfo.mHeightAlign = atoi(tokens[1]);
                    ALOGI("Height Align: %d", mPanelInfo.mHeightAlign);
                }
                if(!strncmp(tokens[0], "min_w", strlen("min_w"))) {
                    mPanelInfo.mMinROIWidth = atoi(tokens[1]);
                    ALOGI("Min ROI Width: %d", mPanelInfo.mMinROIWidth);
                }
                if(!strncmp(tokens[0], "min_h", strlen("min_h"))) {
                    mPanelInfo.mMinROIHeight = atoi(tokens[1]);
                    ALOGI("Min ROI Height: %d", mPanelInfo.mMinROIHeight);
                }
                if(!strncmp(tokens[0], "roi_merge", strlen("roi_merge"))) {
                    mPanelInfo.mNeedsROIMerge = atoi(tokens[1]);
                    ALOGI("Needs ROI Merge: %d", mPanelInfo.mNeedsROIMerge);
                }
            }
        }
        fclose(panelInfoNodeFP);
    } else {
        ALOGE("Failed to open msm_fb_panel_info node");
    }
}

// This function reads the sysfs node to read MDP capabilities
// and parses and updates information accordingly.
bool MDPVersion::updateSysFsInfo() {
    FILE *sysfsFd;
    size_t len = PAGE_SIZE;
    ssize_t read;
    char *line = NULL;
    char sysfsPath[255];
    memset(sysfsPath, 0, sizeof(sysfsPath));
    snprintf(sysfsPath , sizeof(sysfsPath),
            "/sys/class/graphics/fb0/mdp/caps");
    char property[PROPERTY_VALUE_MAX];
    bool enableMacroTile = false;

    if((property_get("persist.hwc.macro_tile_enable", property, NULL) > 0) &&
       (!strncmp(property, "1", PROPERTY_VALUE_MAX ) ||
        (!strncasecmp(property,"true", PROPERTY_VALUE_MAX )))) {
        enableMacroTile = true;
    }

    sysfsFd = fopen(sysfsPath, "rb");

    if (sysfsFd == NULL) {
        ALOGE("%s: sysFsFile file '%s' not found",
                __FUNCTION__, sysfsPath);
        return false;
    } else {
        line = (char *) malloc(len);
        while((read = getline(&line, &len, sysfsFd)) != -1) {
            int index=0;
            char *tokens[10];
            memset(tokens, 0, sizeof(tokens));

            // parse the line and update information accordingly
            if(!tokenizeParams(line, TOKEN_PARAMS_DELIM, tokens, &index)) {
                if(!strncmp(tokens[0], "hw_rev", strlen("hw_rev"))) {
                    mMdpRev = atoi(tokens[1]);
                }
                else if(!strncmp(tokens[0], "rgb_pipes", strlen("rgb_pipes"))) {
                    mRGBPipes = (uint8_t)atoi(tokens[1]);
                }
                else if(!strncmp(tokens[0], "vig_pipes", strlen("vig_pipes"))) {
                    mVGPipes = (uint8_t)atoi(tokens[1]);
                }
                else if(!strncmp(tokens[0], "dma_pipes", strlen("dma_pipes"))) {
                    mDMAPipes = (uint8_t)atoi(tokens[1]);
                }
                else if(!strncmp(tokens[0], "max_downscale_ratio",
                                strlen("max_downscale_ratio"))) {
                    mMDPDownscale = atoi(tokens[1]);
                }
                else if(!strncmp(tokens[0], "max_upscale_ratio",
                                strlen("max_upscale_ratio"))) {
                    mMDPUpscale = atoi(tokens[1]);
                } else if(!strncmp(tokens[0], "max_bandwidth_low",
                        strlen("max_bandwidth_low"))) {
                    mLowBw = atol(tokens[1]);
                } else if(!strncmp(tokens[0], "max_bandwidth_high",
                        strlen("max_bandwidth_high"))) {
                    mHighBw = atol(tokens[1]);
                } else if(!strncmp(tokens[0], "features", strlen("features"))) {
                    for(int i=1; i<index;i++) {
                        if(!strncmp(tokens[i], "bwc", strlen("bwc"))) {
                           mFeatures |= MDP_BWC_EN;
                        } else if(!strncmp(tokens[i], "decimation",
                                    strlen("decimation"))) {
                           mFeatures |= MDP_DECIMATION_EN;
                        } else if(!strncmp(tokens[i], "tile_format",
                                    strlen("tile_format"))) {
                           if(enableMacroTile)
                               mMacroTileEnabled = true;
                        } else if(!strncmp(tokens[i], "src_split",
                                    strlen("src_split"))) {
                            mSourceSplit = true;
                        } else if(!strncmp(tokens[i], "non_scalar_rgb",
                                    strlen("non_scalar_rgb"))) {
                            mRGBHasNoScalar = true;
                        } else if(!strncmp(tokens[i], "rotator_downscale",
                                    strlen("rotator_downscale"))) {
                            mRotDownscale = true;
                        }
                    }
                }
            }
        }
        free(line);
        fclose(sysfsFd);
    }

    if(mSourceSplit) {
        memset(sysfsPath, 0, sizeof(sysfsPath));
        snprintf(sysfsPath , sizeof(sysfsPath),
                "/sys/class/graphics/fb0/msm_fb_src_split_info");

        sysfsFd = fopen(sysfsPath, "rb");
        if (sysfsFd == NULL) {
            ALOGE("%s: Opening file %s failed with error %s", __FUNCTION__,
                    sysfsPath, strerror(errno));
            return false;
        } else {
            line = (char *) malloc(len);
            if((read = getline(&line, &len, sysfsFd)) != -1) {
                if(!strncmp(line, "src_split_always",
                        strlen("src_split_always"))) {
                    mSourceSplitAlways = true;
                }
            }
            free(line);
            fclose(sysfsFd);
        }
    }

    ALOGD_IF(DEBUG, "%s: mMDPVersion: %d mMdpRev: %x mRGBPipes:%d,"
                    "mVGPipes:%d", __FUNCTION__, mMDPVersion, mMdpRev,
                    mRGBPipes, mVGPipes);
    ALOGD_IF(DEBUG, "%s:mDMAPipes:%d \t mMDPDownscale:%d, mFeatures:%d",
                     __FUNCTION__,  mDMAPipes, mMDPDownscale, mFeatures);
    ALOGD_IF(DEBUG, "%s:mLowBw: %lu mHighBw: %lu", __FUNCTION__,  mLowBw,
            mHighBw);

    return true;
}

// This function reads the sysfs node to read MDP capabilities
// and parses and updates information accordingly.
bool MDPVersion::updateSplitInfo() {
    if(mMDPVersion >= MDSS_V5) {
        char split[64] = {0};
        FILE* fp = fopen("/sys/class/graphics/fb0/msm_fb_split", "r");
        if(fp){
            //Format "left right" space as delimiter
            if(fread(split, sizeof(char), 64, fp)) {
                split[sizeof(split) - 1] = '\0';
                mSplit.mLeft = atoi(split);
                ALOGI_IF(mSplit.mLeft, "Left Split=%d", mSplit.mLeft);
                char *rght = strpbrk(split, " ");
                if(rght)
                    mSplit.mRight = atoi(rght + 1);
                ALOGI_IF(mSplit.mRight, "Right Split=%d", mSplit.mRight);
            }
        } else {
            ALOGE("Failed to open mdss_fb_split node");
            return false;
        }
        if(fp)
            fclose(fp);
    }
    return true;
}


bool MDPVersion::supportsDecimation() {
    return mFeatures & MDP_DECIMATION_EN;
}

uint32_t MDPVersion::getMaxMDPDownscale() {
    return mMDPDownscale;
}

uint32_t MDPVersion::getMaxMDPUpscale() {
    return mMDPUpscale;
}

bool MDPVersion::supportsBWC() {
    // BWC - Bandwidth Compression
    return (mFeatures & MDP_BWC_EN);
}

bool MDPVersion::supportsMacroTile() {
    // MACRO TILE support
    return mMacroTileEnabled;
}

bool MDPVersion::isSrcSplit() const {
    return mSourceSplit;
}

bool MDPVersion::isSrcSplitAlways() const {
    return mSourceSplitAlways;
}

bool MDPVersion::isRGBScalarSupported() const {
    return (!mRGBHasNoScalar);
}

bool MDPVersion::is8x26() {
    return (mMdpRev >= MDSS_MDP_HW_REV_101 and
            mMdpRev < MDSS_MDP_HW_REV_102);
}

bool MDPVersion::is8x74v2() {
    return (mMdpRev >= MDSS_MDP_HW_REV_102 and
            mMdpRev < MDSS_MDP_HW_REV_103);
}

bool MDPVersion::is8084() {
    return (mMdpRev >= MDSS_MDP_HW_REV_103 and
            mMdpRev < MDSS_MDP_HW_REV_104);
}

bool MDPVersion::is8092() {
    return (mMdpRev >= MDSS_MDP_HW_REV_200 and
            mMdpRev < MDSS_MDP_HW_REV_206);
}

bool MDPVersion::is8994() {
    return (mMdpRev >= MDSS_MDP_HW_REV_105 and
            mMdpRev < MDSS_MDP_HW_REV_106);
}

bool MDPVersion::is8x16() {
    return (mMdpRev >= MDSS_MDP_HW_REV_106 and
            mMdpRev < MDSS_MDP_HW_REV_107);
}

bool MDPVersion::is8x39() {
    return (mMdpRev >= MDSS_MDP_HW_REV_108 and
            mMdpRev < MDSS_MDP_HW_REV_109);
}


}; //namespace qdutils

