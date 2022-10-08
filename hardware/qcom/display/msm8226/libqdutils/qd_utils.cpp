/*
 * Copyright (c) 2013, The Linux Foundation. All rights reserved.

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
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

#include "qd_utils.h"


#define MAX_FRAME_BUFFER_NAME_SIZE      (80)
#define QD_UTILS_DEBUG 0

int getHDMINode(void)
{
    FILE *displayDeviceFP = NULL;
    char fbType[MAX_FRAME_BUFFER_NAME_SIZE];
    char msmFbTypePath[MAX_FRAME_BUFFER_NAME_SIZE];
    int j = 0;

    for(j = 0; j < HWC_NUM_DISPLAY_TYPES; j++) {
        snprintf (msmFbTypePath, sizeof(msmFbTypePath),
                  "/sys/class/graphics/fb%d/msm_fb_type", j);
        displayDeviceFP = fopen(msmFbTypePath, "r");
        if(displayDeviceFP) {
            fread(fbType, sizeof(char), MAX_FRAME_BUFFER_NAME_SIZE,
                    displayDeviceFP);
            if(strncmp(fbType, "dtv panel", strlen("dtv panel")) == 0) {
                ALOGD("%s: HDMI is at fb%d", __func__, j);
                fclose(displayDeviceFP);
                break;
            }
            fclose(displayDeviceFP);
        } else {
            ALOGE("%s: Failed to open fb node %d", __func__, j);
        }
    }

    if (j < HWC_NUM_DISPLAY_TYPES)
        return j;
    else
        ALOGE("%s: Failed to find HDMI node", __func__);

    return -1;
}

int getEdidRawData(char *buffer)
{
    int size;
    int edidFile;
    char msmFbTypePath[MAX_FRAME_BUFFER_NAME_SIZE];
    int node_id = getHDMINode();

    if (node_id < 0) {
        ALOGE("%s no HDMI node found", __func__);
        return 0;
    }

    snprintf(msmFbTypePath, sizeof(msmFbTypePath),
                 "/sys/class/graphics/fb%d/edid_raw_data", node_id);

   edidFile = open(msmFbTypePath, O_RDONLY, 0);

    if (edidFile < 0) {
        ALOGE("%s no edid raw data found", __func__);
        return 0;
    }

    size = (int)read(edidFile, (char*)buffer, EDID_RAW_DATA_SIZE);
    close(edidFile);
    return size;
}

/* Calculates the aspect ratio for based on src & dest */
void getAspectRatioPosition(int destWidth, int destHeight, int srcWidth,
                                int srcHeight, hwc_rect_t& rect) {
   int x =0, y =0;

   if (srcWidth * destHeight > destWidth * srcHeight) {
        srcHeight = destWidth * srcHeight / srcWidth;
        srcWidth = destWidth;
    } else if (srcWidth * destHeight < destWidth * srcHeight) {
        srcWidth = destHeight * srcWidth / srcHeight;
        srcHeight = destHeight;
    } else {
        srcWidth = destWidth;
        srcHeight = destHeight;
    }
    if (srcWidth > destWidth) srcWidth = destWidth;
    if (srcHeight > destHeight) srcHeight = destHeight;
    x = (destWidth - srcWidth) / 2;
    y = (destHeight - srcHeight) / 2;
    ALOGD_IF(QD_UTILS_DEBUG, "%s: AS Position: x = %d, y = %d w = %d h = %d",
             __FUNCTION__, x, y, srcWidth , srcHeight);
    // Convert it back to hwc_rect_t
    rect.left = x;
    rect.top = y;
    rect.right = srcWidth + rect.left;
    rect.bottom = srcHeight + rect.top;
}

