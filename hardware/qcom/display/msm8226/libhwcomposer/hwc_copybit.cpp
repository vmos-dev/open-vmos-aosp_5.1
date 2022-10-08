/*
 * Copyright (C) 2010 The Android Open Source Project
 * Copyright (C) 2012-2014, The Linux Foundation. All rights reserved.
 *
 * Not a Contribution.
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

#define DEBUG_COPYBIT 0
#include <copybit.h>
#include <utils/Timers.h>
#include <mdp_version.h>
#include "hwc_copybit.h"
#include "comptype.h"
#include "gr.h"
#include "cb_utils.h"
#include "cb_swap_rect.h"
#include "math.h"
#include <sync/sync.h>

using namespace qdutils;
namespace qhwc {

struct range {
    int current;
    int end;
};
struct region_iterator : public copybit_region_t {

    region_iterator(hwc_region_t region) {
        mRegion = region;
        r.end = (int)region.numRects;
        r.current = 0;
        this->next = iterate;
    }

private:
    static int iterate(copybit_region_t const * self, copybit_rect_t* rect){
        if (!self || !rect) {
            ALOGE("iterate invalid parameters");
            return 0;
        }

        region_iterator const* me =
                                  static_cast<region_iterator const*>(self);
        if (me->r.current != me->r.end) {
            rect->l = me->mRegion.rects[me->r.current].left;
            rect->t = me->mRegion.rects[me->r.current].top;
            rect->r = me->mRegion.rects[me->r.current].right;
            rect->b = me->mRegion.rects[me->r.current].bottom;
            me->r.current++;
            return 1;
        }
        return 0;
    }

    hwc_region_t mRegion;
    mutable range r;
};

void CopyBit::reset() {
    mIsModeOn = false;
    mCopyBitDraw = false;
}

bool CopyBit::canUseCopybitForYUV(hwc_context_t *ctx) {
    // return true for non-overlay targets
    if(ctx->mMDP.hasOverlay && ctx->mMDP.version >= qdutils::MDP_V4_0) {
       return false;
    }
    return true;
}

bool CopyBit::canUseCopybitForRGB(hwc_context_t *ctx,
                                        hwc_display_contents_1_t *list,
                                        int dpy) {
    int compositionType = qdutils::QCCompositionType::
                                    getInstance().getCompositionType();

    if (compositionType & qdutils::COMPOSITION_TYPE_DYN) {
        // DYN Composition:
        // use copybit, if (TotalRGBRenderArea < threashold * FB Area)
        // this is done based on perf inputs in ICS
        // TODO: Above condition needs to be re-evaluated in JB
        int fbWidth =  ctx->dpyAttr[dpy].xres;
        int fbHeight =  ctx->dpyAttr[dpy].yres;
        unsigned int fbArea = (fbWidth * fbHeight);
        unsigned int renderArea = getRGBRenderingArea(list);
            ALOGD_IF (DEBUG_COPYBIT, "%s:renderArea %u, fbArea %u",
                                  __FUNCTION__, renderArea, fbArea);
        if (renderArea < (mDynThreshold * fbArea)) {
            return true;
        }
    } else if ((compositionType & qdutils::COMPOSITION_TYPE_MDP)) {
      // MDP composition, use COPYBIT always
      return true;
    } else if ((compositionType & qdutils::COMPOSITION_TYPE_C2D)) {
      // C2D composition, use COPYBIT
      return true;
    }
    return false;
}

unsigned int CopyBit::getRGBRenderingArea
                                    (const hwc_display_contents_1_t *list) {
    //Calculates total rendering area for RGB layers
    unsigned int renderArea = 0;
    unsigned int w=0, h=0;
    // Skipping last layer since FrameBuffer layer should not affect
    // which composition to choose
    for (unsigned int i=0; i<list->numHwLayers -1; i++) {
         private_handle_t *hnd = (private_handle_t *)list->hwLayers[i].handle;
         if (hnd) {
             if (BUFFER_TYPE_UI == hnd->bufferType) {
                 getLayerResolution(&list->hwLayers[i], w, h);
                 renderArea += (w*h);
             }
         }
    }
    return renderArea;
}

int CopyBit::getLayersChanging(hwc_context_t *ctx,
                      hwc_display_contents_1_t *list,
                      int dpy){

   int changingLayerIndex = -1;
   if(mLayerCache.layerCount != ctx->listStats[dpy].numAppLayers) {
        mLayerCache.reset();
        mFbCache.reset();
        mLayerCache.updateCounts(ctx,list,dpy);
        return -1;
    }

    int updatingLayerCount = 0;
    for (int k = ctx->listStats[dpy].numAppLayers-1; k >= 0 ; k--){
       //swap rect will kick in only for single updating layer
       if(mLayerCache.hnd[k] != list->hwLayers[k].handle){
           updatingLayerCount ++;
           if(updatingLayerCount == 1)
             changingLayerIndex = k;
       }
    }
    //since we are using more than one framebuffers,we have to
    //kick in swap rect only if we are getting continuous same
    //dirty rect for same layer at least equal of number of
    //framebuffers

    if ( updatingLayerCount ==  1 ) {
       hwc_rect_t dirtyRect = list->hwLayers[changingLayerIndex].displayFrame;
#ifdef QCOM_BSP
       dirtyRect = list->hwLayers[changingLayerIndex].dirtyRect;
#endif

       for (int k = ctx->listStats[dpy].numAppLayers-1; k >= 0 ; k--){
            //disable swap rect for overlapping visible layer(s)
            hwc_rect_t displayFrame = list->hwLayers[k].displayFrame;
            hwc_rect_t result = getIntersection(displayFrame,dirtyRect);
            if((k != changingLayerIndex) && isValidRect(result)){
              return -1;
           }
       }
       mFbCache.insertAndUpdateFbCache(dirtyRect);
       if(mFbCache.getUnchangedFbDRCount(dirtyRect) <
                                             NUM_RENDER_BUFFERS)
              changingLayerIndex =  -1;
    }else {
       mFbCache.reset();
       changingLayerIndex =  -1;
    }
    mLayerCache.updateCounts(ctx,list,dpy);
    return changingLayerIndex;
}

int CopyBit::checkDirtyRect(hwc_context_t *ctx,
                           hwc_display_contents_1_t *list,
                           int dpy) {

   //dirty rect will enable only if
   //1.Only single layer is updating.
   //2.No overlapping
   //3.No scaling
   //4.No video layer
   if(mSwapRectEnable == false)
      return -1;
   int changingLayerIndex =  getLayersChanging(ctx, list, dpy);
   //swap rect will kick in only for single updating layer
   if(changingLayerIndex == -1){
      return -1;
   }
   if(!needsScaling(&list->hwLayers[changingLayerIndex])){
     private_handle_t *hnd =
         (private_handle_t *)list->hwLayers[changingLayerIndex].handle;
      if( hnd && !isYuvBuffer(hnd))
           return  changingLayerIndex;
   }
   return -1;
}

bool CopyBit::prepareOverlap(hwc_context_t *ctx,
                             hwc_display_contents_1_t *list) {

    if (ctx->mMDP.version < qdutils::MDP_V4_0) {
        ALOGE("%s: Invalid request", __FUNCTION__);
        return false;
    }

    if (mEngine == NULL || !(validateParams(ctx, list))) {
        ALOGE("%s: Invalid Params", __FUNCTION__);
        return false;
    }
    PtorInfo* ptorInfo = &(ctx->mPtorInfo);

    // Allocate render buffers if they're not allocated
    int alignW = 0, alignH = 0;
    int finalW = 0, finalH = 0;
    for (int i = 0; i < ptorInfo->count; i++) {
        int ovlapIndex = ptorInfo->layerIndex[i];
        hwc_rect_t overlap = list->hwLayers[ovlapIndex].displayFrame;
        // render buffer width will be the max of two layers
        // Align Widht and height to 32, Mdp would be configured
        // with Aligned overlap w/h
        finalW = max(finalW, ALIGN((overlap.right - overlap.left), 32));
        finalH += ALIGN((overlap.bottom - overlap.top), 32);
        if(finalH > ALIGN((overlap.bottom - overlap.top), 32)) {
            // Calculate the offset for RGBA(4BPP)
            ptorInfo->mRenderBuffOffset[i] = finalW *
                (finalH - ALIGN((overlap.bottom - overlap.top), 32)) * 4;
            // Calculate the dest top, left will always be zero
            ptorInfo->displayFrame[i].top = (finalH -
                                (ALIGN((overlap.bottom - overlap.top), 32)));
        }
        // calculate the right and bottom values
        ptorInfo->displayFrame[i].right =  ptorInfo->displayFrame[i].left +
                                            (overlap.right - overlap.left);
        ptorInfo->displayFrame[i].bottom = ptorInfo->displayFrame[i].top +
                                            (overlap.bottom - overlap.top);
    }

    getBufferSizeAndDimensions(finalW, finalH, HAL_PIXEL_FORMAT_RGBA_8888,
                               alignW, alignH);

    if ((mAlignedWidth != alignW) || (mAlignedHeight != alignH)) {
        // Overlap rect has changed, so free render buffers
        freeRenderBuffers();
    }

    int ret = allocRenderBuffers(alignW, alignH, HAL_PIXEL_FORMAT_RGBA_8888);

    if (ret < 0) {
        ALOGE("%s: Render buffer allocation failed", __FUNCTION__);
        return false;
    }

    mAlignedWidth = alignW;
    mAlignedHeight = alignH;
    mCurRenderBufferIndex = (mCurRenderBufferIndex + 1) % NUM_RENDER_BUFFERS;
    return true;
}

bool CopyBit::prepare(hwc_context_t *ctx, hwc_display_contents_1_t *list,
                                                            int dpy) {

    if(mEngine == NULL) {
        // No copybit device found - cannot use copybit
        return false;
    }
    int compositionType = qdutils::QCCompositionType::
                                    getInstance().getCompositionType();

    if ((compositionType == qdutils::COMPOSITION_TYPE_GPU) ||
        (compositionType == qdutils::COMPOSITION_TYPE_CPU))   {
        //GPU/CPU composition, don't change layer composition type
        return true;
    }

    if(!(validateParams(ctx, list))) {
        ALOGE("%s:Invalid Params", __FUNCTION__);
        return false;
    }

    if(ctx->listStats[dpy].skipCount) {
        //GPU will be anyways used
        return false;
    }

    if (ctx->listStats[dpy].numAppLayers > MAX_NUM_APP_LAYERS) {
        // Reached max layers supported by HWC.
        return false;
    }

    bool useCopybitForYUV = canUseCopybitForYUV(ctx);
    bool useCopybitForRGB = canUseCopybitForRGB(ctx, list, dpy);
    LayerProp *layerProp = ctx->layerProp[dpy];

    // Following are MDP3 limitations for which we
    // need to fallback to GPU composition:
    // 1. Plane alpha is not supported by MDP3.
    // 2. Scaling is within range
    if (qdutils::MDPVersion::getInstance().getMDPVersion() < 400) {
        for (int i = ctx->listStats[dpy].numAppLayers-1; i >= 0 ; i--) {
            int dst_h, dst_w, src_h, src_w;
            float dx, dy;
            hwc_layer_1_t *layer = (hwc_layer_1_t *) &list->hwLayers[i];
            if (layer->planeAlpha != 0xFF)
                return true;
            hwc_rect_t sourceCrop = integerizeSourceCrop(layer->sourceCropf);

            if (layer->transform & HAL_TRANSFORM_ROT_90) {
                src_h = sourceCrop.right - sourceCrop.left;
                src_w = sourceCrop.bottom - sourceCrop.top;
            } else {
                src_h = sourceCrop.bottom - sourceCrop.top;
                src_w = sourceCrop.right - sourceCrop.left;
            }
            dst_h = layer->displayFrame.bottom - layer->displayFrame.top;
            dst_w = layer->displayFrame.right - layer->displayFrame.left;

            if(src_w <=0 || src_h<=0 ||dst_w<=0 || dst_h<=0 ) {
              ALOGE("%s: wrong params for display screen_w=%d \
                         src_crop_width=%d screen_h=%d src_crop_height=%d",
                         __FUNCTION__, dst_w,src_w,dst_h,src_h);
              return false;
            }
            dx = (float)dst_w/(float)src_w;
            dy = (float)dst_h/(float)src_h;

            if (dx > MAX_SCALE_FACTOR || dx < MIN_SCALE_FACTOR)
                return false;

            if (dy > MAX_SCALE_FACTOR || dy < MIN_SCALE_FACTOR)
                return false;
        }
    }

    //Allocate render buffers if they're not allocated
    if (ctx->mMDP.version != qdutils::MDP_V3_0_4 &&
            (useCopybitForYUV || useCopybitForRGB)) {
        int ret = allocRenderBuffers(mAlignedWidth,
                                     mAlignedHeight,
                                     HAL_PIXEL_FORMAT_RGBA_8888);
        if (ret < 0) {
            return false;
        } else {
            mCurRenderBufferIndex = (mCurRenderBufferIndex + 1) %
                NUM_RENDER_BUFFERS;
        }
    }

    // We cannot mix copybit layer with layers marked to be drawn on FB
    if (!useCopybitForYUV && ctx->listStats[dpy].yuvCount)
        return true;

    mCopyBitDraw = false;
    if (useCopybitForRGB &&
        (useCopybitForYUV || !ctx->listStats[dpy].yuvCount)) {
        mCopyBitDraw =  true;
        // numAppLayers-1, as we iterate till 0th layer index
        // Mark all layers to be drawn by copybit
        for (int i = ctx->listStats[dpy].numAppLayers-1; i >= 0 ; i--) {
            layerProp[i].mFlags |= HWC_COPYBIT;
#ifdef QCOM_BSP
            if (ctx->mMDP.version == qdutils::MDP_V3_0_4)
                list->hwLayers[i].compositionType = HWC_BLIT;
            else
#endif
                list->hwLayers[i].compositionType = HWC_OVERLAY;
        }
    }

    return true;
}

int CopyBit::clear (private_handle_t* hnd, hwc_rect_t& rect)
{
    int ret = 0;
    copybit_rect_t clear_rect = {rect.left, rect.top,
        rect.right,
        rect.bottom};

    copybit_image_t buf;
    buf.w = ALIGN(getWidth(hnd),32);
    buf.h = getHeight(hnd);
    buf.format = hnd->format;
    buf.base = (void *)hnd->base;
    buf.handle = (native_handle_t *)hnd;

    copybit_device_t *copybit = mEngine;
    ret = copybit->clear(copybit, &buf, &clear_rect);
    return ret;
}

bool CopyBit::drawUsingAppBufferComposition(hwc_context_t *ctx,
                                      hwc_display_contents_1_t *list,
                                      int dpy, int *copybitFd) {
     int layerCount = 0;
     uint32_t last = (uint32_t)list->numHwLayers - 1;
     hwc_layer_1_t *fbLayer = &list->hwLayers[last];
     private_handle_t *fbhnd = (private_handle_t *)fbLayer->handle;

    if(ctx->enableABC == false)
       return false;

    if(ctx->listStats[dpy].numAppLayers > MAX_LAYERS_FOR_ABC )
       return false;

    layerCount = ctx->listStats[dpy].numAppLayers;
    //bottom most layer should
    //equal to FB
    hwc_layer_1_t *tmpLayer = &list->hwLayers[0];
    private_handle_t *hnd = (private_handle_t *)tmpLayer->handle;
    if(hnd && fbhnd && (hnd->size == fbhnd->size) &&
    (hnd->width == fbhnd->width) && (hnd->height == fbhnd->height)){
       if(tmpLayer->transform  ||
       (!(hnd->format == HAL_PIXEL_FORMAT_RGBA_8888 ||
       hnd->format == HAL_PIXEL_FORMAT_RGBX_8888))  ||
                   (needsScaling(tmpLayer) == true)) {
          return false;
       }else {
          ctx->listStats[dpy].renderBufIndexforABC = 0;
       }
    }

    if(ctx->listStats[dpy].renderBufIndexforABC == 0) {
       if(layerCount == 1)
          return true;

       if(layerCount ==  MAX_LAYERS_FOR_ABC) {
          int abcRenderBufIdx = ctx->listStats[dpy].renderBufIndexforABC;
          //enable ABC only for non intersecting layers.
          hwc_rect_t displayFrame =
                  list->hwLayers[abcRenderBufIdx].displayFrame;
          for (int i = abcRenderBufIdx + 1; i < layerCount; i++) {
             hwc_rect_t tmpDisplayFrame = list->hwLayers[i].displayFrame;
             hwc_rect_t result = getIntersection(displayFrame,tmpDisplayFrame);
             if (isValidRect(result)) {
                ctx->listStats[dpy].renderBufIndexforABC = -1;
                return false;
             }
          }
          // Pass the Acquire Fence FD to driver for base layer
          private_handle_t *renderBuffer =
          (private_handle_t *)list->hwLayers[abcRenderBufIdx].handle;
          copybit_device_t *copybit = getCopyBitDevice();
          if(list->hwLayers[abcRenderBufIdx].acquireFenceFd >=0){
             copybit->set_sync(copybit,
             list->hwLayers[abcRenderBufIdx].acquireFenceFd);
          }
          for(int i = abcRenderBufIdx + 1; i < layerCount; i++){
             int retVal = drawLayerUsingCopybit(ctx,
               &(list->hwLayers[i]),renderBuffer, 0);
             if(retVal < 0) {
                ALOGE("%s : Copybit failed", __FUNCTION__);
             }
          }
          // Get Release Fence FD of copybit for the App layer(s)
          copybit->flush_get_fence(copybit, copybitFd);
          close(list->hwLayers[abcRenderBufIdx].acquireFenceFd);
          list->hwLayers[abcRenderBufIdx].acquireFenceFd = -1;
          return true;
       }
    }
    return false;
}

bool  CopyBit::draw(hwc_context_t *ctx, hwc_display_contents_1_t *list,
                                                          int dpy, int32_t *fd) {
    // draw layers marked for COPYBIT
    int retVal = true;
    int copybitLayerCount = 0;
    uint32_t last = 0;
    LayerProp *layerProp = ctx->layerProp[dpy];
    private_handle_t *renderBuffer;

    if(mCopyBitDraw == false){
       mFbCache.reset(); // there is no layer marked for copybit
       return false ;
    }

    if(drawUsingAppBufferComposition(ctx, list, dpy, fd)) {
       return true;
    }
    //render buffer
    if (ctx->mMDP.version == qdutils::MDP_V3_0_4) {
        last = (uint32_t)list->numHwLayers - 1;
        renderBuffer = (private_handle_t *)list->hwLayers[last].handle;
    } else {
        renderBuffer = getCurrentRenderBuffer();
    }
    if (!renderBuffer) {
        ALOGE("%s: Render buffer layer handle is NULL", __FUNCTION__);
        return false;
    }

    if (ctx->mMDP.version >= qdutils::MDP_V4_0) {
        //Wait for the previous frame to complete before rendering onto it
        if(mRelFd[mCurRenderBufferIndex] >=0) {
            sync_wait(mRelFd[mCurRenderBufferIndex], 1000);
            close(mRelFd[mCurRenderBufferIndex]);
            mRelFd[mCurRenderBufferIndex] = -1;
        }
    } else {
        if(list->hwLayers[last].acquireFenceFd >=0) {
            copybit_device_t *copybit = getCopyBitDevice();
            copybit->set_sync(copybit, list->hwLayers[last].acquireFenceFd);
        }
    }

    mDirtyLayerIndex =  checkDirtyRect(ctx, list, dpy);
    if( mDirtyLayerIndex != -1){
          hwc_layer_1_t *layer = &list->hwLayers[mDirtyLayerIndex];
#ifdef QCOM_BSP
          clear(renderBuffer,layer->dirtyRect);
#else
          clear(renderBuffer,layer->displayFrame);
#endif
    } else {
          hwc_rect_t clearRegion = {0,0,0,0};
          if(CBUtils::getuiClearRegion(list, clearRegion, layerProp))
             clear(renderBuffer, clearRegion);
    }

    // numAppLayers-1, as we iterate from 0th layer index with HWC_COPYBIT flag
    for (int i = 0; i <= (ctx->listStats[dpy].numAppLayers-1); i++) {
        if(!(layerProp[i].mFlags & HWC_COPYBIT)) {
            ALOGD_IF(DEBUG_COPYBIT, "%s: Not Marked for copybit", __FUNCTION__);
            continue;
        }
        //skip non updating layers
        if((mDirtyLayerIndex != -1) && (mDirtyLayerIndex != i) )
            continue;
        int ret = -1;
        if (list->hwLayers[i].acquireFenceFd != -1
                && ctx->mMDP.version >= qdutils::MDP_V4_0) {
            // Wait for acquire Fence on the App buffers.
            ret = sync_wait(list->hwLayers[i].acquireFenceFd, 1000);
            if(ret < 0) {
                ALOGE("%s: sync_wait error!! error no = %d err str = %s",
                                    __FUNCTION__, errno, strerror(errno));
            }
            close(list->hwLayers[i].acquireFenceFd);
            list->hwLayers[i].acquireFenceFd = -1;
        }
        retVal = drawLayerUsingCopybit(ctx, &(list->hwLayers[i]),
                                          renderBuffer, !i);
        copybitLayerCount++;
        if(retVal < 0) {
            ALOGE("%s : drawLayerUsingCopybit failed", __FUNCTION__);
        }
    }

    if (copybitLayerCount) {
        copybit_device_t *copybit = getCopyBitDevice();
        // Async mode
        copybit->flush_get_fence(copybit, fd);
        if(ctx->mMDP.version == qdutils::MDP_V3_0_4 &&
                list->hwLayers[last].acquireFenceFd >= 0) {
            close(list->hwLayers[last].acquireFenceFd);
            list->hwLayers[last].acquireFenceFd = -1;
        }
    }
    return true;
}

int CopyBit::drawOverlap(hwc_context_t *ctx, hwc_display_contents_1_t *list) {
    int fd = -1;
    PtorInfo* ptorInfo = &(ctx->mPtorInfo);

    if (ctx->mMDP.version < qdutils::MDP_V4_0) {
        ALOGE("%s: Invalid request", __FUNCTION__);
        return fd;
    }

    private_handle_t *renderBuffer = getCurrentRenderBuffer();

    if (!renderBuffer) {
        ALOGE("%s: Render buffer layer handle is NULL", __FUNCTION__);
        return fd;
    }

    int copybitLayerCount = 0;
    for(int j = 0; j < ptorInfo->count; j++) {
        int ovlapIndex = ptorInfo->layerIndex[j];
        hwc_rect_t overlap = list->hwLayers[ovlapIndex].displayFrame;

        // Draw overlapped content of layers on render buffer
        for (int i = 0; i <= ovlapIndex; i++) {
            hwc_layer_1_t *layer = &list->hwLayers[i];
            if(!isValidRect(getIntersection(layer->displayFrame,
                                               overlap))) {
                continue;
            }
            if ((list->hwLayers[i].acquireFenceFd != -1)) {
                // Wait for acquire fence on the App buffers.
                if(sync_wait(list->hwLayers[i].acquireFenceFd, 1000) < 0) {
                    ALOGE("%s: sync_wait error!! error no = %d err str = %s",
                          __FUNCTION__, errno, strerror(errno));
                }
                close(list->hwLayers[i].acquireFenceFd);
                list->hwLayers[i].acquireFenceFd = -1;
            }

            int retVal = drawRectUsingCopybit(ctx, layer, renderBuffer, overlap,
                                                ptorInfo->displayFrame[j]);
            copybitLayerCount++;
            if(retVal < 0) {
                ALOGE("%s: drawRectUsingCopybit failed", __FUNCTION__);
                copybitLayerCount = 0;
            }
        }
    }

    if (copybitLayerCount) {
        copybit_device_t *copybit = getCopyBitDevice();
        copybit->flush_get_fence(copybit, &fd);
    }

    ALOGD_IF(DEBUG_COPYBIT, "%s: done! copybitLayerCount = %d", __FUNCTION__,
             copybitLayerCount);
    return fd;
}

int CopyBit::drawRectUsingCopybit(hwc_context_t *dev, hwc_layer_1_t *layer,
                        private_handle_t *renderBuffer, hwc_rect_t overlap,
                        hwc_rect_t destRect)
{
    hwc_context_t* ctx = (hwc_context_t*)(dev);
    if (!ctx) {
        ALOGE("%s: null context ", __FUNCTION__);
        return -1;
    }

    private_handle_t *hnd = (private_handle_t *)layer->handle;
    if (!hnd) {
        ALOGE("%s: invalid handle", __FUNCTION__);
        return -1;
    }

    private_handle_t *dstHandle = (private_handle_t *)renderBuffer;
    if (!dstHandle) {
        ALOGE("%s: RenderBuffer handle is NULL", __FUNCTION__);
        return -1;
    }

    // Set the Copybit Source
    copybit_image_t src;
    src.handle = (native_handle_t *)layer->handle;
    src.w = hnd->width;
    src.h = hnd->height;
    src.base = (void *)hnd->base;
    src.format = hnd->format;
    src.horiz_padding = 0;
    src.vert_padding = 0;


    hwc_rect_t dispFrame = layer->displayFrame;
    hwc_rect_t iRect = getIntersection(dispFrame, overlap);
    hwc_rect_t crop = integerizeSourceCrop(layer->sourceCropf);
    qhwc::calculate_crop_rects(crop, dispFrame, iRect,
                               layer->transform);

    // Copybit source rect
    copybit_rect_t srcRect = {crop.left, crop.top, crop.right,
        crop.bottom};

    // Copybit destination rect
    copybit_rect_t dstRect = {destRect.left, destRect.top, destRect.right,
        destRect.bottom};

    // Copybit dst
    copybit_image_t dst;
    dst.handle = (native_handle_t *)dstHandle;
    dst.w = ALIGN(dstHandle->width, 32);
    dst.h = dstHandle->height;
    dst.base = (void *)dstHandle->base;
    dst.format = dstHandle->format;

    copybit_device_t *copybit = mEngine;

    // Copybit region is the destRect
    hwc_rect_t regRect = {dstRect.l,dstRect.t, dstRect.r, dstRect.b};
    hwc_region_t region;
    region.numRects = 1;
    region.rects  = &regRect;
    region_iterator copybitRegion(region);
    int acquireFd = layer->acquireFenceFd;

    copybit->set_parameter(copybit, COPYBIT_FRAMEBUFFER_WIDTH,
                           renderBuffer->width);
    copybit->set_parameter(copybit, COPYBIT_FRAMEBUFFER_HEIGHT,
                           renderBuffer->height);
    copybit->set_parameter(copybit, COPYBIT_TRANSFORM, layer->transform);
    copybit->set_parameter(copybit, COPYBIT_PLANE_ALPHA, layer->planeAlpha);
    copybit->set_parameter(copybit, COPYBIT_BLEND_MODE, layer->blending);
    copybit->set_parameter(copybit, COPYBIT_DITHER,
        (dst.format == HAL_PIXEL_FORMAT_RGB_565) ? COPYBIT_ENABLE :
        COPYBIT_DISABLE);
    copybit->set_sync(copybit, acquireFd);
    int err = copybit->stretch(copybit, &dst, &src, &dstRect, &srcRect,
                               &copybitRegion);

    if (err < 0)
        ALOGE("%s: copybit stretch failed",__FUNCTION__);

    return err;
}

int  CopyBit::drawLayerUsingCopybit(hwc_context_t *dev, hwc_layer_1_t *layer,
                          private_handle_t *renderBuffer, bool isFG)
{
    hwc_context_t* ctx = (hwc_context_t*)(dev);
    int err = 0, acquireFd;
    if(!ctx) {
         ALOGE("%s: null context ", __FUNCTION__);
         return -1;
    }

    private_handle_t *hnd = (private_handle_t *)layer->handle;
    if(!hnd) {
        if (layer->flags & HWC_COLOR_FILL) { // Color layer
            return fillColorUsingCopybit(layer, renderBuffer);
        }
        ALOGE("%s: invalid handle", __FUNCTION__);
        return -1;
    }

    private_handle_t *fbHandle = (private_handle_t *)renderBuffer;
    if(!fbHandle) {
        ALOGE("%s: Framebuffer handle is NULL", __FUNCTION__);
        return -1;
    }

    // Set the copybit source:
    copybit_image_t src;
    src.w = getWidth(hnd);
    src.h = getHeight(hnd);
    src.format = hnd->format;

    // Handle R/B swap
    if ((layer->flags & HWC_FORMAT_RB_SWAP)) {
        if (src.format == HAL_PIXEL_FORMAT_RGBA_8888) {
            src.format = HAL_PIXEL_FORMAT_BGRA_8888;
        } else if (src.format == HAL_PIXEL_FORMAT_RGBX_8888) {
            src.format = HAL_PIXEL_FORMAT_BGRX_8888;
        }
    }

    src.base = (void *)hnd->base;
    src.handle = (native_handle_t *)layer->handle;
    src.horiz_padding = src.w - getWidth(hnd);
    // Initialize vertical padding to zero for now,
    // this needs to change to accomodate vertical stride
    // if needed in the future
    src.vert_padding = 0;

    int layerTransform = layer->transform ;
    // When flip and rotation(90) are present alter the flip,
    // as GPU is doing the flip and rotation in opposite order
    // to that of MDP3.0
    // For 270 degrees, we get 90 + (H+V) which is same as doing
    // flip first and then rotation (H+V) + 90
    if (qdutils::MDPVersion::getInstance().getMDPVersion() < 400) {
                if (((layer->transform& HAL_TRANSFORM_FLIP_H) ||
                (layer->transform & HAL_TRANSFORM_FLIP_V)) &&
                (layer->transform &  HAL_TRANSFORM_ROT_90) &&
                !(layer->transform ==  HAL_TRANSFORM_ROT_270)){
                      if(layer->transform & HAL_TRANSFORM_FLIP_H){
                                 layerTransform ^= HAL_TRANSFORM_FLIP_H;
                                 layerTransform |= HAL_TRANSFORM_FLIP_V;
                      }
                      if(layer->transform & HAL_TRANSFORM_FLIP_V){
                                 layerTransform ^= HAL_TRANSFORM_FLIP_V;
                                 layerTransform |= HAL_TRANSFORM_FLIP_H;
                      }
               }
    }
    // Copybit source rect
    hwc_rect_t sourceCrop = integerizeSourceCrop(layer->sourceCropf);
    copybit_rect_t srcRect = {sourceCrop.left, sourceCrop.top,
                              sourceCrop.right,
                              sourceCrop.bottom};

    // Copybit destination rect
    hwc_rect_t displayFrame = layer->displayFrame;
    copybit_rect_t dstRect = {displayFrame.left, displayFrame.top,
                              displayFrame.right,
                              displayFrame.bottom};
#ifdef QCOM_BSP
    //change src and dst with dirtyRect
    if(mDirtyLayerIndex != -1) {
      srcRect.l = layer->dirtyRect.left;
      srcRect.t = layer->dirtyRect.top;
      srcRect.r = layer->dirtyRect.right;
      srcRect.b = layer->dirtyRect.bottom;
      dstRect = srcRect;
    }
#endif
    // Copybit dst
    copybit_image_t dst;
    dst.w = ALIGN(fbHandle->width,32);
    dst.h = fbHandle->height;
    dst.format = fbHandle->format;
    dst.base = (void *)fbHandle->base;
    dst.handle = (native_handle_t *)fbHandle;

    copybit_device_t *copybit = mEngine;

    int32_t screen_w        = displayFrame.right - displayFrame.left;
    int32_t screen_h        = displayFrame.bottom - displayFrame.top;
    int32_t src_crop_width  = sourceCrop.right - sourceCrop.left;
    int32_t src_crop_height = sourceCrop.bottom -sourceCrop.top;

    // Copybit dst
    float copybitsMaxScale =
                      (float)copybit->get(copybit,COPYBIT_MAGNIFICATION_LIMIT);
    float copybitsMinScale =
                       (float)copybit->get(copybit,COPYBIT_MINIFICATION_LIMIT);

    if (layer->transform & HWC_TRANSFORM_ROT_90) {
        //swap screen width and height
        int tmp = screen_w;
        screen_w  = screen_h;
        screen_h = tmp;
    }
    private_handle_t *tmpHnd = NULL;

    if(screen_w <=0 || screen_h<=0 ||src_crop_width<=0 || src_crop_height<=0 ) {
        ALOGE("%s: wrong params for display screen_w=%d src_crop_width=%d \
        screen_h=%d src_crop_height=%d", __FUNCTION__, screen_w,
                                src_crop_width,screen_h,src_crop_height);
        return -1;
    }

    float dsdx = (float)screen_w/(float)src_crop_width;
    float dtdy = (float)screen_h/(float)src_crop_height;

    float scaleLimitMax = copybitsMaxScale * copybitsMaxScale;
    float scaleLimitMin = copybitsMinScale * copybitsMinScale;
    if(dsdx > scaleLimitMax ||
        dtdy > scaleLimitMax ||
        dsdx < 1/scaleLimitMin ||
        dtdy < 1/scaleLimitMin) {
        ALOGW("%s: greater than max supported size dsdx=%f dtdy=%f \
              scaleLimitMax=%f scaleLimitMin=%f", __FUNCTION__,dsdx,dtdy,
                                          scaleLimitMax,1/scaleLimitMin);
        return -1;
    }
    acquireFd = layer->acquireFenceFd;
    if(dsdx > copybitsMaxScale ||
        dtdy > copybitsMaxScale ||
        dsdx < 1/copybitsMinScale ||
        dtdy < 1/copybitsMinScale){
        // The requested scale is out of the range the hardware
        // can support.
       ALOGD("%s:%d::Need to scale twice dsdx=%f, dtdy=%f,copybitsMaxScale=%f,\
                                 copybitsMinScale=%f,screen_w=%d,screen_h=%d \
                  src_crop_width=%d src_crop_height=%d",__FUNCTION__,__LINE__,
              dsdx,dtdy,copybitsMaxScale,1/copybitsMinScale,screen_w,screen_h,
                                              src_crop_width,src_crop_height);


       int tmp_w =  src_crop_width;
       int tmp_h =  src_crop_height;

       if (dsdx > copybitsMaxScale || dtdy > copybitsMaxScale ){
         tmp_w = (int)((float)src_crop_width*copybitsMaxScale);
         tmp_h = (int)((float)src_crop_height*copybitsMaxScale);
       }else if (dsdx < 1/copybitsMinScale ||dtdy < 1/copybitsMinScale ){
         // ceil the tmp_w and tmp_h value to maintain proper ratio
         // b/w src and dst (should not cross the desired scale limit
         // due to float -> int )
         tmp_w = (int)ceil((float)src_crop_width/copybitsMinScale);
         tmp_h = (int)ceil((float)src_crop_height/copybitsMinScale);
       }
       ALOGD("%s:%d::tmp_w = %d,tmp_h = %d",__FUNCTION__,__LINE__,tmp_w,tmp_h);

       int usage = GRALLOC_USAGE_PRIVATE_IOMMU_HEAP;
       int format = fbHandle->format;

       // We do not want copybit to generate alpha values from nothing
       if (format == HAL_PIXEL_FORMAT_RGBA_8888 &&
               src.format != HAL_PIXEL_FORMAT_RGBA_8888) {
           format = HAL_PIXEL_FORMAT_RGBX_8888;
       }
       if (0 == alloc_buffer(&tmpHnd, tmp_w, tmp_h, format, usage) && tmpHnd) {
            copybit_image_t tmp_dst;
            copybit_rect_t tmp_rect;
            tmp_dst.w = tmp_w;
            tmp_dst.h = tmp_h;
            tmp_dst.format = tmpHnd->format;
            tmp_dst.handle = tmpHnd;
            tmp_dst.horiz_padding = src.horiz_padding;
            tmp_dst.vert_padding = src.vert_padding;
            tmp_rect.l = 0;
            tmp_rect.t = 0;
            tmp_rect.r = tmp_dst.w;
            tmp_rect.b = tmp_dst.h;
            //create one clip region
            hwc_rect tmp_hwc_rect = {0,0,tmp_rect.r,tmp_rect.b};
            hwc_region_t tmp_hwc_reg = {1,(hwc_rect_t const*)&tmp_hwc_rect};
            region_iterator tmp_it(tmp_hwc_reg);
            copybit->set_parameter(copybit,COPYBIT_TRANSFORM,0);
            //TODO: once, we are able to read layer alpha, update this
            copybit->set_parameter(copybit, COPYBIT_PLANE_ALPHA, 255);
            copybit->set_sync(copybit, acquireFd);
            err = copybit->stretch(copybit,&tmp_dst, &src, &tmp_rect,
                                                           &srcRect, &tmp_it);
            if(err < 0){
                ALOGE("%s:%d::tmp copybit stretch failed",__FUNCTION__,
                                                             __LINE__);
                if(tmpHnd)
                    free_buffer(tmpHnd);
                return err;
            }
            // use release fence as aquire fd for next stretch
            if (ctx->mMDP.version < qdutils::MDP_V4_0) {
                copybit->flush_get_fence(copybit, &acquireFd);
                close(acquireFd);
                acquireFd = -1;
            }
            // copy new src and src rect crop
            src = tmp_dst;
            srcRect = tmp_rect;
      }
    }
    // Copybit region
    hwc_region_t region = layer->visibleRegionScreen;
    region_iterator copybitRegion(region);

    copybit->set_parameter(copybit, COPYBIT_FRAMEBUFFER_WIDTH,
                                          renderBuffer->width);
    copybit->set_parameter(copybit, COPYBIT_FRAMEBUFFER_HEIGHT,
                                          renderBuffer->height);
    copybit->set_parameter(copybit, COPYBIT_TRANSFORM,
                                              layerTransform);
    //TODO: once, we are able to read layer alpha, update this
    copybit->set_parameter(copybit, COPYBIT_PLANE_ALPHA, 255);
    copybit->set_parameter(copybit, COPYBIT_BLEND_MODE,
                                              layer->blending);
    copybit->set_parameter(copybit, COPYBIT_DITHER,
                             (dst.format == HAL_PIXEL_FORMAT_RGB_565)?
                                             COPYBIT_ENABLE : COPYBIT_DISABLE);
    copybit->set_parameter(copybit, COPYBIT_FG_LAYER, isFG ?
                                             COPYBIT_ENABLE : COPYBIT_DISABLE);

    copybit->set_parameter(copybit, COPYBIT_BLIT_TO_FRAMEBUFFER,
                                                COPYBIT_ENABLE);
    copybit->set_sync(copybit, acquireFd);
    err = copybit->stretch(copybit, &dst, &src, &dstRect, &srcRect,
                                                   &copybitRegion);
    copybit->set_parameter(copybit, COPYBIT_BLIT_TO_FRAMEBUFFER,
                                               COPYBIT_DISABLE);

    if(tmpHnd) {
        if (ctx->mMDP.version < qdutils::MDP_V4_0){
            int ret = -1, releaseFd;
            // we need to wait for the buffer before freeing
            copybit->flush_get_fence(copybit, &releaseFd);
            ret = sync_wait(releaseFd, 1000);
            if(ret < 0) {
                ALOGE("%s: sync_wait error!! error no = %d err str = %s",
                    __FUNCTION__, errno, strerror(errno));
            }
            close(releaseFd);
        }
        free_buffer(tmpHnd);
    }

    if(err < 0)
        ALOGE("%s: copybit stretch failed",__FUNCTION__);
    return err;
}

int CopyBit::fillColorUsingCopybit(hwc_layer_1_t *layer,
                          private_handle_t *renderBuffer)
{
    if (!renderBuffer) {
        ALOGE("%s: Render Buffer is NULL", __FUNCTION__);
        return -1;
    }

    // Copybit dst
    copybit_image_t dst;
    dst.w = ALIGN(renderBuffer->width, 32);
    dst.h = renderBuffer->height;
    dst.format = renderBuffer->format;
    dst.base = (void *)renderBuffer->base;
    dst.handle = (native_handle_t *)renderBuffer;

    // Copybit dst rect
    hwc_rect_t displayFrame = layer->displayFrame;
    copybit_rect_t dstRect = {displayFrame.left, displayFrame.top,
                              displayFrame.right, displayFrame.bottom};

    uint32_t color = layer->transform;
    copybit_device_t *copybit = mEngine;
    copybit->set_parameter(copybit, COPYBIT_FRAMEBUFFER_WIDTH,
                           renderBuffer->width);
    copybit->set_parameter(copybit, COPYBIT_FRAMEBUFFER_HEIGHT,
                           renderBuffer->height);
    copybit->set_parameter(copybit, COPYBIT_DITHER,
                           (dst.format == HAL_PIXEL_FORMAT_RGB_565) ?
                           COPYBIT_ENABLE : COPYBIT_DISABLE);
    copybit->set_parameter(copybit, COPYBIT_TRANSFORM, 0);
    copybit->set_parameter(copybit, COPYBIT_BLEND_MODE, layer->blending);
    copybit->set_parameter(copybit, COPYBIT_PLANE_ALPHA, layer->planeAlpha);
    copybit->set_parameter(copybit, COPYBIT_BLIT_TO_FRAMEBUFFER,COPYBIT_ENABLE);
    int res = copybit->fill_color(copybit, &dst, &dstRect, color);
    copybit->set_parameter(copybit,COPYBIT_BLIT_TO_FRAMEBUFFER,COPYBIT_DISABLE);
    return res;
}

void CopyBit::getLayerResolution(const hwc_layer_1_t* layer,
                                 unsigned int& width, unsigned int& height)
{
    hwc_rect_t displayFrame  = layer->displayFrame;

    width = displayFrame.right - displayFrame.left;
    height = displayFrame.bottom - displayFrame.top;
}

bool CopyBit::validateParams(hwc_context_t *ctx,
                                        const hwc_display_contents_1_t *list) {
    //Validate parameters
    if (!ctx) {
        ALOGE("%s:Invalid HWC context", __FUNCTION__);
        return false;
    } else if (!list) {
        ALOGE("%s:Invalid HWC layer list", __FUNCTION__);
        return false;
    }
    return true;
}


int CopyBit::allocRenderBuffers(int w, int h, int f)
{
    int ret = 0;
    for (int i = 0; i < NUM_RENDER_BUFFERS; i++) {
        if (mRenderBuffer[i] == NULL) {
            ret = alloc_buffer(&mRenderBuffer[i],
                               w, h, f,
                               GRALLOC_USAGE_PRIVATE_IOMMU_HEAP);
        }
        if(ret < 0) {
            freeRenderBuffers();
            break;
        }
    }
    return ret;
}

void CopyBit::freeRenderBuffers()
{
    for (int i = 0; i < NUM_RENDER_BUFFERS; i++) {
        if(mRenderBuffer[i]) {
            //Since we are freeing buffer close the fence if it has a valid one.
            if(mRelFd[i] >= 0) {
                close(mRelFd[i]);
                mRelFd[i] = -1;
            }
            free_buffer(mRenderBuffer[i]);
            mRenderBuffer[i] = NULL;
        }
    }
}

private_handle_t * CopyBit::getCurrentRenderBuffer() {
    return mRenderBuffer[mCurRenderBufferIndex];
}

void CopyBit::setReleaseFd(int fd) {
    if(mRelFd[mCurRenderBufferIndex] >=0)
        close(mRelFd[mCurRenderBufferIndex]);
    mRelFd[mCurRenderBufferIndex] = dup(fd);
}

void CopyBit::setReleaseFdSync(int fd) {
    if (mRelFd[mCurRenderBufferIndex] >=0) {
        int ret = -1;
        ret = sync_wait(mRelFd[mCurRenderBufferIndex], 1000);
        if (ret < 0)
            ALOGE("%s: sync_wait error! errno = %d, err str = %s",
                  __FUNCTION__, errno, strerror(errno));
        close(mRelFd[mCurRenderBufferIndex]);
    }
    mRelFd[mCurRenderBufferIndex] = dup(fd);
}

struct copybit_device_t* CopyBit::getCopyBitDevice() {
    return mEngine;
}

CopyBit::CopyBit(hwc_context_t *ctx, const int& dpy) :  mEngine(0),
    mIsModeOn(false), mCopyBitDraw(false), mCurRenderBufferIndex(0) {

    getBufferSizeAndDimensions(ctx->dpyAttr[dpy].xres,
            ctx->dpyAttr[dpy].yres,
            HAL_PIXEL_FORMAT_RGBA_8888,
            mAlignedWidth,
            mAlignedHeight);

    hw_module_t const *module;
    for (int i = 0; i < NUM_RENDER_BUFFERS; i++) {
        mRenderBuffer[i] = NULL;
        mRelFd[i] = -1;
    }

    char value[PROPERTY_VALUE_MAX];
    property_get("debug.hwc.dynThreshold", value, "2");
    mDynThreshold = atof(value);

    property_get("debug.sf.swaprect", value, "0");
    mSwapRectEnable = atoi(value) ? true:false ;
    mDirtyLayerIndex = -1;
    if (hw_get_module(COPYBIT_HARDWARE_MODULE_ID, &module) == 0) {
        if(copybit_open(module, &mEngine) < 0) {
            ALOGE("FATAL ERROR: copybit open failed.");
        }
    } else {
        ALOGE("FATAL ERROR: copybit hw module not found");
    }
}

CopyBit::~CopyBit()
{
    freeRenderBuffers();
    if(mEngine)
    {
        copybit_close(mEngine);
        mEngine = NULL;
    }
}
CopyBit::LayerCache::LayerCache() {
    reset();
}
void CopyBit::LayerCache::reset() {
    memset(&hnd, 0, sizeof(hnd));
    layerCount = 0;
}
void CopyBit::LayerCache::updateCounts(hwc_context_t *ctx,
              hwc_display_contents_1_t *list, int dpy)
{
   layerCount = ctx->listStats[dpy].numAppLayers;
   for (int i=0; i<ctx->listStats[dpy].numAppLayers; i++){
      hnd[i] = list->hwLayers[i].handle;
   }
}

CopyBit::FbCache::FbCache() {
     reset();
}
void CopyBit::FbCache::reset() {
     memset(&FbdirtyRect, 0, sizeof(FbdirtyRect));
     FbIndex =0;
}

void CopyBit::FbCache::insertAndUpdateFbCache(hwc_rect_t dirtyRect) {
   FbIndex =  FbIndex % NUM_RENDER_BUFFERS;
   FbdirtyRect[FbIndex] = dirtyRect;
   FbIndex++;
}

int CopyBit::FbCache::getUnchangedFbDRCount(hwc_rect_t dirtyRect){
    int sameDirtyCount = 0;
    for (int i = 0 ; i < NUM_RENDER_BUFFERS ; i++ ){
      if( FbdirtyRect[i] == dirtyRect)
           sameDirtyCount++;
   }
   return sameDirtyCount;
}

}; //namespace qhwc
