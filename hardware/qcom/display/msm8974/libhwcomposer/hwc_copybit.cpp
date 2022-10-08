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

#define DEBUG_COPYBIT 0
#include <sync/sync.h>
#include <copybit.h>
#include <utils/Timers.h>
#include <mdp_version.h>
#include "hwc_copybit.h"
#include "comptype.h"
#include "gr.h"

namespace qhwc {

struct range {
    int current;
    int end;
};
struct region_iterator : public copybit_region_t {

    region_iterator(hwc_region_t region) {
        mRegion = region;
        r.end = region.numRects;
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
    size_t fbLayerIndex = ctx->listStats[dpy].fbLayerIndex;
    hwc_layer_1_t *fbLayer = &list->hwLayers[fbLayerIndex];
    private_handle_t *fbHnd = (private_handle_t *)fbLayer->handle;


    // Following are MDP3 limitations for which we
    // need to fallback to GPU composition:
    // 1. HW issues with mdp3 and rotation.
    // 2. Plane alpha is not supported by MDP3.
    if (qdutils::MDPVersion::getInstance().getMDPVersion() < 400) {
        for (int i = ctx->listStats[dpy].numAppLayers-1; i >= 0 ; i--) {
            hwc_layer_1_t *layer = (hwc_layer_1_t *) &list->hwLayers[i];
            if ((layer->transform & (HAL_TRANSFORM_FLIP_H |
                   HAL_TRANSFORM_FLIP_V | HAL_TRANSFORM_ROT_90)) &&
                   ((layer->displayFrame.bottom - layer->displayFrame.top) % 16 ||
                   (layer->displayFrame.right - layer->displayFrame.left) % 16))
                return true;
            if (layer->planeAlpha != 0xFF)
                return true;
        }
    }

    //Allocate render buffers if they're not allocated
    if (useCopybitForYUV || useCopybitForRGB) {
        int ret = allocRenderBuffers(fbHnd->width,
                                     fbHnd->height,
                                     fbHnd->format);
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

    // numAppLayers-1, as we iterate till 0th layer index
    for (int i = ctx->listStats[dpy].numAppLayers-1; i >= 0 ; i--) {
        private_handle_t *hnd = (private_handle_t *)list->hwLayers[i].handle;

        if ((hnd->bufferType == BUFFER_TYPE_VIDEO && useCopybitForYUV) ||
            (hnd->bufferType == BUFFER_TYPE_UI && useCopybitForRGB)) {
            layerProp[i].mFlags |= HWC_COPYBIT;
            list->hwLayers[i].compositionType = HWC_OVERLAY;
            mCopyBitDraw = true;
        } else {
            // We currently cannot mix copybit layers with layers marked to
            // be drawn on the framebuffer or that are on the layer cache.
            mCopyBitDraw = false;
            //There is no need to reset layer properties here as we return in
            //draw if mCopyBitDraw is false
            break;
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

bool CopyBit::draw(hwc_context_t *ctx, hwc_display_contents_1_t *list,
                                                        int dpy, int32_t *fd) {
    // draw layers marked for COPYBIT
    int retVal = true;
    int copybitLayerCount = 0;
    LayerProp *layerProp = ctx->layerProp[dpy];

    if(mCopyBitDraw == false) // there is no layer marked for copybit
        return false ;

    //render buffer
    private_handle_t *renderBuffer = getCurrentRenderBuffer();
    if (!renderBuffer) {
        ALOGE("%s: Render buffer layer handle is NULL", __FUNCTION__);
        return false;
    }

    //Wait for the previous frame to complete before rendering onto it
    if(mRelFd[0] >=0) {
        sync_wait(mRelFd[0], 1000);
        close(mRelFd[0]);
        mRelFd[0] = -1;
    }

    if (ctx->mMDP.version >= qdutils::MDP_V4_0) {
        //Clear the visible region on the render buffer
        //XXX: Do this only when needed.
        hwc_rect_t clearRegion;
        getNonWormholeRegion(list, clearRegion);
        clear(renderBuffer, clearRegion);
    }
    // numAppLayers-1, as we iterate from 0th layer index with HWC_COPYBIT flag
    for (int i = 0; i <= (ctx->listStats[dpy].numAppLayers-1); i++) {
        hwc_layer_1_t *layer = &list->hwLayers[i];
        if(!(layerProp[i].mFlags & HWC_COPYBIT)) {
            ALOGD_IF(DEBUG_COPYBIT, "%s: Not Marked for copybit", __FUNCTION__);
            continue;
        }
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
                                                    renderBuffer, dpy, !i);
        copybitLayerCount++;
        if(retVal < 0) {
            ALOGE("%s : drawLayerUsingCopybit failed", __FUNCTION__);
        }
    }

    if (copybitLayerCount) {
        copybit_device_t *copybit = getCopyBitDevice();
        // Async mode
        copybit->flush_get_fence(copybit, fd);
    }
    return true;
}

int  CopyBit::drawLayerUsingCopybit(hwc_context_t *dev, hwc_layer_1_t *layer,
                          private_handle_t *renderBuffer, int dpy, bool isFG)
{
    hwc_context_t* ctx = (hwc_context_t*)(dev);
    int err = 0, acquireFd;
    if(!ctx) {
         ALOGE("%s: null context ", __FUNCTION__);
         return -1;
    }

    private_handle_t *hnd = (private_handle_t *)layer->handle;
    if(!hnd) {
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
    src.base = (void *)hnd->base;
    src.handle = (native_handle_t *)layer->handle;
    src.horiz_padding = src.w - getWidth(hnd);
    // Initialize vertical padding to zero for now,
    // this needs to change to accomodate vertical stride
    // if needed in the future
    src.vert_padding = 0;

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

    if((layer->transform == HWC_TRANSFORM_ROT_90) ||
                           (layer->transform == HWC_TRANSFORM_ROT_270)) {
        //swap screen width and height
        int tmp = screen_w;
        screen_w  = screen_h;
        screen_h = tmp;
    }
    private_handle_t *tmpHnd = NULL;

    if(screen_w <=0 || screen_h<=0 ||src_crop_width<=0 || src_crop_height<=0 ) {
        ALOGE("%s: wrong params for display screen_w=%d src_crop_width=%d \
        screen_w=%d src_crop_width=%d", __FUNCTION__, screen_w,
                                src_crop_width,screen_w,src_crop_width);
        return -1;
    }

    float dsdx = (float)screen_w/src_crop_width;
    float dtdy = (float)screen_h/src_crop_height;

    float scaleLimitMax = copybitsMaxScale * copybitsMaxScale;
    float scaleLimitMin = copybitsMinScale * copybitsMinScale;
    if(dsdx > scaleLimitMax ||
        dtdy > scaleLimitMax ||
        dsdx < 1/scaleLimitMin ||
        dtdy < 1/scaleLimitMin) {
        ALOGE("%s: greater than max supported size dsdx=%f dtdy=%f \
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

       //Driver makes width and height as even
       //that may cause wrong calculation of the ratio
       //in display and crop.Hence we make
       //crop width and height as even.
       src_crop_width  = (src_crop_width/2)*2;
       src_crop_height = (src_crop_height/2)*2;

       int tmp_w =  src_crop_width;
       int tmp_h =  src_crop_height;

       if (dsdx > copybitsMaxScale || dtdy > copybitsMaxScale ){
         tmp_w = src_crop_width*copybitsMaxScale;
         tmp_h = src_crop_height*copybitsMaxScale;
       }else if (dsdx < 1/copybitsMinScale ||dtdy < 1/copybitsMinScale ){
         tmp_w = src_crop_width/copybitsMinScale;
         tmp_h = src_crop_height/copybitsMinScale;
         tmp_w  = (tmp_w/2)*2;
         tmp_h = (tmp_h/2)*2;
       }
       ALOGD("%s:%d::tmp_w = %d,tmp_h = %d",__FUNCTION__,__LINE__,tmp_w,tmp_h);

       int usage = GRALLOC_USAGE_PRIVATE_IOMMU_HEAP;
       int format = fbHandle->format;

       // We do not want copybit to generate alpha values from nothing
       if (format == HAL_PIXEL_FORMAT_RGBA_8888 &&
               src.format != HAL_PIXEL_FORMAT_RGBA_8888) {
           format = HAL_PIXEL_FORMAT_RGBX_8888;
       }
       if (0 == alloc_buffer(&tmpHnd, tmp_w, tmp_h, format, usage)){
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
                                              layer->transform);
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
            free_buffer(mRenderBuffer[i]);
            mRenderBuffer[i] = NULL;
        }
    }
}

private_handle_t * CopyBit::getCurrentRenderBuffer() {
    return mRenderBuffer[mCurRenderBufferIndex];
}

void CopyBit::setReleaseFd(int fd) {
    if(mRelFd[0] >=0)
        close(mRelFd[0]);
    mRelFd[0] = mRelFd[1];
    mRelFd[1] = dup(fd);
}

struct copybit_device_t* CopyBit::getCopyBitDevice() {
    return mEngine;
}

CopyBit::CopyBit():mIsModeOn(false), mCopyBitDraw(false),
    mCurRenderBufferIndex(0){
    hw_module_t const *module;
    for (int i = 0; i < NUM_RENDER_BUFFERS; i++)
        mRenderBuffer[i] = NULL;
    mRelFd[0] = -1;
    mRelFd[1] = -1;

    char value[PROPERTY_VALUE_MAX];
    property_get("debug.hwc.dynThreshold", value, "2");
    mDynThreshold = atof(value);

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
    if(mRelFd[0] >=0)
        close(mRelFd[0]);
    if(mRelFd[1] >=0)
        close(mRelFd[1]);
    if(mEngine)
    {
        copybit_close(mEngine);
        mEngine = NULL;
    }
}
}; //namespace qhwc
