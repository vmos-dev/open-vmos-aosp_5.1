/*
 * Copyright (C) 2010 The Android Open Source Project
 * Copyright (C) 2012-2014, The Linux Foundation. All rights reserved.
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

#define NUM_RENDER_BUFFERS 3
//These scaling factors are specific for MDP3. Normally scaling factor
//is only 4, but copybit will create temp buffer to let it run through
//twice
#define MAX_SCALE_FACTOR 16
#define MIN_SCALE_FACTOR 0.0625
#define MAX_LAYERS_FOR_ABC 2
namespace qhwc {

class CopyBit {
public:
    CopyBit(hwc_context_t *ctx, const int& dpy);
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

    void setReleaseFdSync(int fd);

    bool prepareOverlap(hwc_context_t *ctx, hwc_display_contents_1_t *list);

    int drawOverlap(hwc_context_t *ctx, hwc_display_contents_1_t *list);

private:
    /* cached data */
    struct LayerCache {
      int layerCount;
      buffer_handle_t hnd[MAX_NUM_APP_LAYERS];
      /* c'tor */
      LayerCache();
      /* clear caching info*/
      void reset();
      void updateCounts(hwc_context_t *ctx, hwc_display_contents_1_t *list,
              int dpy);
    };
    /* framebuffer cache*/
    struct FbCache {
      hwc_rect_t  FbdirtyRect[NUM_RENDER_BUFFERS];
      int FbIndex;
      FbCache();
      void reset();
      void insertAndUpdateFbCache(hwc_rect_t dirtyRect);
      int getUnchangedFbDRCount(hwc_rect_t dirtyRect);
    };

    // holds the copybit device
    struct copybit_device_t *mEngine;
    bool drawUsingAppBufferComposition(hwc_context_t *ctx,
                                hwc_display_contents_1_t *list,
                                int dpy, int *fd);
    // Helper functions for copybit composition
    int  drawLayerUsingCopybit(hwc_context_t *dev, hwc_layer_1_t *layer,
                          private_handle_t *renderBuffer, bool isFG);
    // Helper function to draw copybit layer for PTOR comp
    int drawRectUsingCopybit(hwc_context_t *dev, hwc_layer_1_t *layer,
                          private_handle_t *renderBuffer, hwc_rect_t overlap,
                          hwc_rect_t destRect);
    int fillColorUsingCopybit(hwc_layer_1_t *layer,
                          private_handle_t *renderBuffer);
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

    // Release FDs of the intermediate render buffer
    int mRelFd[NUM_RENDER_BUFFERS];

    //Dynamic composition threshold for deciding copybit usage.
    double mDynThreshold;
    bool mSwapRectEnable;
    int mAlignedWidth;
    int mAlignedHeight;
    int mDirtyLayerIndex;
    LayerCache mLayerCache;
    FbCache mFbCache;
    int getLayersChanging(hwc_context_t *ctx, hwc_display_contents_1_t *list,
                  int dpy);
    int checkDirtyRect(hwc_context_t *ctx, hwc_display_contents_1_t *list,
                  int dpy);
};

}; //namespace qhwc

#endif //HWC_COPYBIT_H
