/*
// Copyright (c) 2014 Intel Corporation 
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/
#include <common/utils/HwcTrace.h>
#include <common/base/Drm.h>
#include <Hwcomposer.h>
#include <DisplayPlane.h>
#include <IDisplayDevice.h>
#include <common/base/HwcLayerList.h>
#include <ips/tangier/TngDisplayContext.h>


namespace android {
namespace intel {

TngDisplayContext::TngDisplayContext()
    : mIMGDisplayDevice(0),
      mInitialized(false),
      mCount(0)
{
    CTRACE();
}

TngDisplayContext::~TngDisplayContext()
{
    WARN_IF_NOT_DEINIT();
}

bool TngDisplayContext::initialize()
{
    CTRACE();

    // open frame buffer device
    hw_module_t const* module;
    int err = hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &module);
    if (err) {
        ELOGTRACE("failed to load gralloc module, error = %d", err);
        return false;
    }

    // init IMG display device
    mIMGDisplayDevice = (((IMG_gralloc_module_public_t *)module)->getDisplayDevice((IMG_gralloc_module_public_t *)module));
    if (!mIMGDisplayDevice) {
        ELOGTRACE("failed to get display device");
        return false;
    }

    mCount = 0;
    mInitialized = true;
    return true;
}

bool TngDisplayContext::commitBegin(size_t /* numDisplays */,
        hwc_display_contents_1_t ** /* displays */)
{
    RETURN_FALSE_IF_NOT_INIT();
    mCount = 0;
    return true;
}

bool TngDisplayContext::commitContents(hwc_display_contents_1_t *display, HwcLayerList *layerList)
{
    bool ret;

    RETURN_FALSE_IF_NOT_INIT();

    if (!display || !layerList) {
        ELOGTRACE("invalid parameters");
        return false;
    }

    IMG_hwc_layer_t *imgLayerList = (IMG_hwc_layer_t*)mImgLayers;

    for (size_t i = 0; i < display->numHwLayers; i++) {
        if (mCount >= MAXIMUM_LAYER_NUMBER) {
            ELOGTRACE("layer count exceeds the limit");
            return false;
        }

        // check layer parameters
        if (!display->hwLayers[i].handle) {
            continue;
        }

        DisplayPlane* plane = layerList->getPlane(i);
        if (!plane) {
            continue;
        }

        ret = plane->flip(NULL);
        if (ret == false) {
            VLOGTRACE("failed to flip plane %d", i);
            continue;
        }

        IMG_hwc_layer_t *imgLayer = &imgLayerList[mCount++];
        // update IMG layer
        imgLayer->psLayer = &display->hwLayers[i];
        imgLayer->custom = (uint32_t)plane->getContext();
        struct intel_dc_plane_ctx *ctx =
            (struct intel_dc_plane_ctx *)imgLayer->custom;
        // update z order
        Hwcomposer& hwc = Hwcomposer::getInstance();
        DisplayPlaneManager *pm = hwc.getPlaneManager();
        void *config = pm->getZOrderConfig();
        if (config) {
            memcpy(&ctx->zorder, config, sizeof(ctx->zorder));
        } else {
            memset(&ctx->zorder, 0, sizeof(ctx->zorder));
        }

        VLOGTRACE("count %d, handle %#x, trans %#x, blending %#x"
              " sourceCrop %f,%f - %fx%f, dst %d,%d - %dx%d, custom %#x",
              mCount,
              (uint32_t)imgLayer->psLayer->handle,
              imgLayer->psLayer->transform,
              imgLayer->psLayer->blending,
              imgLayer->psLayer->sourceCropf.left,
              imgLayer->psLayer->sourceCropf.top,
              imgLayer->psLayer->sourceCropf.right - imgLayer->psLayer->sourceCropf.left,
              imgLayer->psLayer->sourceCropf.bottom - imgLayer->psLayer->sourceCropf.top,
              imgLayer->psLayer->displayFrame.left,
              imgLayer->psLayer->displayFrame.top,
              imgLayer->psLayer->displayFrame.right - imgLayer->psLayer->displayFrame.left,
              imgLayer->psLayer->displayFrame.bottom - imgLayer->psLayer->displayFrame.top,
              imgLayer->custom);
    }

    layerList->postFlip();
    return true;
}

bool TngDisplayContext::commitEnd(size_t numDisplays, hwc_display_contents_1_t **displays)
{
    int releaseFenceFd = -1;

    VLOGTRACE("count = %d", mCount);

    if (mIMGDisplayDevice && mCount) {
        int err = mIMGDisplayDevice->post(mIMGDisplayDevice,
                                          mImgLayers,
                                          mCount,
                                          &releaseFenceFd);
        if (err) {
            ELOGTRACE("post failed, err = %d", err);
            return false;
        }
    }

    // close acquire fence
    for (size_t i = 0; i < numDisplays; i++) {
        // Wait and close HWC_OVERLAY typed layer's acquire fence
        hwc_display_contents_1_t* display = displays[i];
        if (!display) {
            continue;
        }

        for (size_t j = 0; j < display->numHwLayers-1; j++) {
            hwc_layer_1_t& layer = display->hwLayers[j];
            if (layer.compositionType == HWC_OVERLAY) {
                if (layer.acquireFenceFd != -1) {
                    // sync_wait(layer.acquireFenceFd, 16ms);
                    close(layer.acquireFenceFd);
                    layer.acquireFenceFd = -1;
                }
            }
        }

        // Wait and close framebuffer target layer's acquire fence
        hwc_layer_1_t& fbt = display->hwLayers[display->numHwLayers-1];
        if (fbt.acquireFenceFd != -1) {
            // sync_wait(fbt.acquireFencdFd, 16ms);
            close(fbt.acquireFenceFd);
            fbt.acquireFenceFd = -1;
        }

        // Wait and close outbuf's acquire fence
        if (display->outbufAcquireFenceFd != -1) {
            // sync_wait(display->outbufAcquireFenceFd, 16ms);
            close(display->outbufAcquireFenceFd);
            display->outbufAcquireFenceFd = -1;
        }
    }

    // update release fence and retire fence
    if (mCount > 0) {
        // For physical displays, dup the releaseFenceFd only for
        // HWC layers which successfully flipped to display planes
        IMG_hwc_layer_t *imgLayerList = (IMG_hwc_layer_t*)mImgLayers;

        for (size_t i = 0; i < mCount; i++) {
            IMG_hwc_layer_t *imgLayer = &imgLayerList[i];
            imgLayer->psLayer->releaseFenceFd =
                (releaseFenceFd != -1) ? dup(releaseFenceFd) : -1;
        }
    }

    for (size_t i = 0; i < numDisplays; i++) {
        if (!displays[i]) {
            continue;
        }

        // log for layer fence status
        for (size_t j = 0; j < displays[i]->numHwLayers; j++) {
            VLOGTRACE("handle %#x, acquiredFD %d, releaseFD %d",
                 (uint32_t)displays[i]->hwLayers[j].handle,
                 displays[i]->hwLayers[j].acquireFenceFd,
                 displays[i]->hwLayers[j].releaseFenceFd);
        }

        // retireFence is used for SurfaceFlinger to do DispSync;
        // dup releaseFenceFd for physical displays and ignore virtual
        // display; we don't distinguish between release and retire, and all
        // physical displays are using a single releaseFence; for virtual
        // display, fencing is handled by the VirtualDisplay class
        if (i < IDisplayDevice::DEVICE_VIRTUAL) {
            displays[i]->retireFenceFd =
                (releaseFenceFd != -1) ? dup(releaseFenceFd) : -1;
        }
    }

    // close original release fence fd
    if (releaseFenceFd != -1) {
        close(releaseFenceFd);
    }
    return true;
}

bool TngDisplayContext::compositionComplete()
{
    return true;
}

bool TngDisplayContext::setCursorPosition(int disp, int x, int y)
{
    DLOGTRACE("setCursorPosition");
    struct intel_dc_cursor_ctx ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.pipe = disp;
    if (x < 0) {
        ctx.pos |= 1 << 15;
        x = -x;
    }
    if (y < 0) {
        ctx.pos |= 1 << 31;
        y = -y;
    }
    ctx.pos |= (y & 0xfff) << 16 | (x & 0xfff);
    Drm *drm = Hwcomposer::getInstance().getDrm();
    return drm->writeIoctl(DRM_PSB_UPDATE_CURSOR_POS, &ctx, sizeof(ctx));
}


void TngDisplayContext::deinitialize()
{
    mIMGDisplayDevice = 0;

    mCount = 0;
    mInitialized = false;
}


} // namespace intel
} // namespace android
