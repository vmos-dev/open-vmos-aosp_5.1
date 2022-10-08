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

enum mdp_rev {
    MDSS_MDP_HW_REV_100 = 0x10000000,
    MDSS_MDP_HW_REV_101 = 0x10010000, //8x26
    MDSS_MDP_HW_REV_102 = 0x10020000,
};

enum {
    MAX_DISPLAY_DIM = 2048,
};

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

class MDPVersion;

struct Split {
    int mLeft;
    int mRight;
    Split() : mLeft(0), mRight(0){}
    int left() { return mLeft; }
    int right() { return mRight; }
    friend class MDPVersion;
};

class MDPVersion : public Singleton <MDPVersion>
{
public:
    MDPVersion();
    ~MDPVersion();
    int getMDPVersion() {return mMDPVersion;}
    char getPanelType() {return mPanelType;}
    bool hasOverlay() {return mHasOverlay;}
    uint8_t getTotalPipes() { return (mRGBPipes + mVGPipes + mDMAPipes);}
    uint8_t getRGBPipes() { return mRGBPipes; }
    uint8_t getVGPipes() { return mVGPipes; }
    uint8_t getDMAPipes() { return mDMAPipes; }
    bool supportsDecimation();
    uint32_t getMaxMDPDownscale();
    bool supportsBWC();
    bool is8x26();
    int getLeftSplit() { return mSplit.left(); }
    int getRightSplit() { return mSplit.right(); }
private:
    int mFd;
    int mMDPVersion;
    char mPanelType;
    bool mHasOverlay;
    uint32_t mMdpRev;
    uint8_t mRGBPipes;
    uint8_t mVGPipes;
    uint8_t mDMAPipes;
    uint32_t mFeatures;
    uint32_t mMDPDownscale;
    Split mSplit;
};
}; //namespace qdutils
#endif //INCLUDE_LIBQCOMUTILS_MDPVER
