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
#include <cutils/log.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <linux/msm_mdp.h>
#include "mdp_version.h"

ANDROID_SINGLETON_STATIC_INSTANCE(qdutils::MDPVersion);
namespace qdutils {

MDPVersion::MDPVersion()
{
    int fb_fd = open("/dev/graphics/fb0", O_RDWR);
    int mdp_version = MDP_V_UNKNOWN;
    char panel_type = 0;
    struct fb_fix_screeninfo fb_finfo;

    mMdpRev = 0;
    mRGBPipes = mVGPipes = 0;
    mDMAPipes = 0;

    if (ioctl(fb_fd, FBIOGET_FSCREENINFO, &fb_finfo) < 0) {
        ALOGE("FBIOGET_FSCREENINFO failed");
        mdp_version =  MDP_V_UNKNOWN;
    } else {
        if(!strncmp(fb_finfo.id, "msmfb", 5)) {
            char str_ver[4] = { 0 };
            memcpy(str_ver, &fb_finfo.id[5], 3);
            str_ver[3] = '\0';
            mdp_version = atoi(str_ver);

            //Normalize MDP version to ease comparison.
            //This is needed only because
            //MDP 3.0.3 reports value as 303 which
            //is more than all the others
            if (mdp_version < 100)
                mdp_version *= 10;

            mRGBPipes = mVGPipes = 2;

        } else if (!strncmp(fb_finfo.id, "mdssfb", 6)) {
            mdp_version = MDSS_V5;
#ifdef MDSS_TARGET
            struct msmfb_metadata metadata;
            memset(&metadata, 0 , sizeof(metadata));
            metadata.op = metadata_op_get_caps;
            if (ioctl(fb_fd, MSMFB_METADATA_GET, &metadata) == -1) {
                ALOGE("Error retrieving MDP revision and pipes info");
                mdp_version = MDP_V_UNKNOWN;
            } else {
                mMdpRev = metadata.data.caps.mdp_rev;
                mRGBPipes = metadata.data.caps.rgb_pipes;
                mVGPipes = metadata.data.caps.vig_pipes;
                mDMAPipes = metadata.data.caps.dma_pipes;
            }
#endif
        } else {
            mdp_version = MDP_V_UNKNOWN;
        }
        int len = strlen("msmfbXX_");
        if (mdp_version == MDP_V3_0_3)
            len++;
        panel_type = fb_finfo.id[len];

    }
    close(fb_fd);
    mMDPVersion = mdp_version;
    mHasOverlay = false;
    if((mMDPVersion >= MDP_V4_0) || (mMDPVersion == MDP_V_UNKNOWN))
        mHasOverlay = true;
    mPanelType = panel_type;
}
}; //namespace qdutils

