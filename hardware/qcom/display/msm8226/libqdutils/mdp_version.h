/*
 * Copyright (c) 2012-2013, The Linux Foundation. All rights reserved.

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

#ifndef INCLUDE_LIBQCOMUTILS_MDPVER
#define INCLUDE_LIBQCOMUTILS_MDPVER

#include <stdint.h>
#include <utils/Singleton.h>
#include <cutils/properties.h>

/* This class gets the MSM type from the soc info
*/
using namespace android;
namespace qdutils {
// These panel definitions are available at mdss_mdp.h which is internal header
// file and is not available at <linux/mdss_mdp.h>.
// ToDo: once it is available at linux/mdss_mdp.h, these below definitions can
// be removed.
enum mdp_version {
    MDP_V_UNKNOWN = 0,
    MDP_V2_2    = 220,
    MDP_V3_0    = 300,
    MDP_V3_0_3  = 303,
    MDP_V3_0_4  = 304,
    MDP_V3_1    = 310,
    MDP_V4_0    = 400,
    MDP_V4_1    = 410,
    MDP_V4_2    = 420,
    MDP_V4_3    = 430,
    MDP_V4_4    = 440,
    MDSS_V5     = 500,
};

// chip variants have same major number and minor numbers usually vary
// for e.g., MDSS_MDP_HW_REV_101 is 0x10010000
//                                    1001       -  major number
//                                        0000   -  minor number
// 8x26 v1 minor number is 0000
//      v2 minor number is 0001 etc..

enum {
    MAX_DISPLAY_DIM = 2048,
};

#define NO_PANEL         '0'
#define MDDI_PANEL       '1'
#define EBI2_PANEL       '2'
#define LCDC_PANEL       '3'
#define EXT_MDDI_PANEL   '4'
#define TV_PANEL         '5'
#define DTV_PANEL        '7'
#define MIPI_VIDEO_PANEL '8'
#define MIPI_CMD_PANEL   '9'
#define WRITEBACK_PANEL  'a'
#define LVDS_PANEL       'b'
#define EDP_PANEL        'c'

class MDPVersion;

struct Split {
    int mLeft;
    int mRight;
    Split() : mLeft(0), mRight(0){}
    int left() { return mLeft; }
    int right() { return mRight; }
    friend class MDPVersion;
};

struct PanelInfo {
    char mType;                  // Smart or Dumb
    int mPartialUpdateEnable;    // Partial update feature
    int mLeftAlign;              // ROI left alignment restriction
    int mWidthAlign;             // ROI width alignment restriction
    int mTopAlign;               // ROI top alignment restriction
    int mHeightAlign;            // ROI height alignment restriction
    int mMinROIWidth;            // Min width needed for ROI
    int mMinROIHeight;           // Min height needed for ROI
    bool mNeedsROIMerge;         // Merge ROI's of both the DSI's
    PanelInfo() : mType(NO_PANEL), mPartialUpdateEnable(0),
    mLeftAlign(0), mWidthAlign(0), mTopAlign(0), mHeightAlign(0),
    mMinROIWidth(0), mMinROIHeight(0), mNeedsROIMerge(false){}
    friend class MDPVersion;
};

class MDPVersion : public Singleton <MDPVersion>
{
public:
    MDPVersion();
    ~MDPVersion();
    int getMDPVersion() {return mMDPVersion;}
    char getPanelType() {return mPanelInfo.mType;}
    bool hasOverlay() {return mHasOverlay;}
    uint8_t getTotalPipes() {
        return (uint8_t)(mRGBPipes + mVGPipes + mDMAPipes);
    }
    uint8_t getRGBPipes() { return mRGBPipes; }
    uint8_t getVGPipes() { return mVGPipes; }
    uint8_t getDMAPipes() { return mDMAPipes; }
    bool supportsDecimation();
    uint32_t getMaxMDPDownscale();
    uint32_t getMaxMDPUpscale();
    bool supportsBWC();
    bool supportsMacroTile();
    int getLeftSplit() { return mSplit.left(); }
    int getRightSplit() { return mSplit.right(); }
    bool isPartialUpdateEnabled() { return mPanelInfo.mPartialUpdateEnable; }
    int getLeftAlign() { return mPanelInfo.mLeftAlign; }
    int getWidthAlign() { return mPanelInfo.mWidthAlign; }
    int getTopAlign() { return mPanelInfo.mTopAlign; }
    int getHeightAlign() { return mPanelInfo.mHeightAlign; }
    int getMinROIWidth() { return mPanelInfo.mMinROIWidth; }
    int getMinROIHeight() { return mPanelInfo.mMinROIHeight; }
    bool needsROIMerge() { return mPanelInfo.mNeedsROIMerge; }
    unsigned long getLowBw() { return mLowBw; }
    unsigned long getHighBw() { return mHighBw; }
    bool isRotDownscaleEnabled() { return mRotDownscale; }
    bool isSrcSplit() const;
    bool isSrcSplitAlways() const;
    bool isRGBScalarSupported() const;
    bool is8x26();
    bool is8x74v2();
    bool is8084();
    bool is8092();
    bool is8994();
    bool is8x16();
    bool is8x39();

private:
    bool updateSysFsInfo();
    void updatePanelInfo();
    bool updateSplitInfo();
    int tokenizeParams(char *inputParams, const char *delim,
                        char* tokenStr[], int *idx);
    int mFd;
    int mMDPVersion;
    bool mHasOverlay;
    uint32_t mMdpRev;
    uint8_t mRGBPipes;
    uint8_t mVGPipes;
    uint8_t mDMAPipes;
    uint32_t mFeatures;
    uint32_t mMDPDownscale;
    uint32_t mMDPUpscale;
    bool mMacroTileEnabled;
    Split mSplit;
    PanelInfo mPanelInfo;
    unsigned long mLowBw; //kbps
    unsigned long mHighBw; //kbps
    bool mSourceSplit;
    //Additional property on top of source split
    bool mSourceSplitAlways;
    bool mRGBHasNoScalar;
    bool mRotDownscale;
};
}; //namespace qdutils
#endif //INCLUDE_LIBQCOMUTILS_MDPVER
