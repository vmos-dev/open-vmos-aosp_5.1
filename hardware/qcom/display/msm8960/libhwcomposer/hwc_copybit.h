/*
 * Copyright (C) 2010 The Android Open Source Project
 * Copyright (C) 2012-2013, The Linux Foundation. All rights reserved.
 *
 * Not a Contribution, Apache license notifications and license are retained
 * for attribution purposes only.
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
#ifndef HWC_COPYBIT_H
#define HWC_COPYBIT_H
#include "hwc_utils.h"

#define NUM_RENDER_BUFFERS 2

namespace qhwc {

class CopyBit {
public:
    CopyBit();
    ~CopyBit();
    // API to get copybit engine(non static)
    struct copybit_device_t *getCopyBitDevice();
    //Sets up members and prepares copybit if conditions are met
    bool prepare(hwc_context_t *ctx, hwc_display_contents_1_t *list,
                                                                   int dpy);
    //Draws layer if the layer is set for copybit in prepare
    bool draw(hwc_context_t *ctx, hwc_display_contents_1_t *list,
                                                        int dpy, int* fd);
    // resets the values
    void reset();

    private_handle_t * getCurrentRenderBuffer();

    void setReleaseFd(int fd);

private:
    // holds the copybit device
    struct copybit_device_t *mEngine;
    // Helper functions for copybit composition
    int  drawLayerUsingCopybit(hwc_context_t *dev, hwc_layer_1_t *layer,
                                       private_handle_t *renderBuffer, int dpy);
    bool canUseCopybitForYUV (hwc_context_t *ctx);
    bool canUseCopybitForRGB (hwc_context_t *ctx,
                                     hwc_display_contents_1_t *list, int dpy);
    bool validateParams (hwc_context_t *ctx,
                                const hwc_display_contents_1_t *list);
    //Flags if this feature is on.
    bool mIsModeOn;
    // flag that indicates whether CopyBit composition is enabled for this cycle
    bool mCopyBitDraw;

    unsigned int getRGBRenderingArea
                            (const hwc_display_contents_1_t *list);

    void getLayerResolution(const hwc_layer_1_t* layer,
                                   unsigned int &width, unsigned int& height);

    int allocRenderBuffers(int w, int h, int f);

    void freeRenderBuffers();

    int clear (private_handle_t* hnd, hwc_rect_t& rect);

    private_handle_t* mRenderBuffer[NUM_RENDER_BUFFERS];

    // Index of the current intermediate render buffer
    int mCurRenderBufferIndex;

    //These are the the release FDs of the T-2 and T-1 round
    //We wait on the T-2 fence
    int mRelFd[2];

    //Dynamic composition threshold for deciding copybit usage.
    double mDynThreshold;
};

}; //namespace qhwc

#endif //HWC_COPYBIT_H
