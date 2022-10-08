/*
 * Copyright (c) 2011 Intel Corporation. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Zhaohan Ren  <zhaohan.ren@intel.com>
 *    Jiang Fei  <jiang.fei@intel.com>
 *    Binglin Chen <binglin.chen@intel.com>
 *
 */

#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <gui/Surface.h>
#include <gui/SurfaceComposerClient.h>
#include <gui/ISurfaceComposer.h>
#include <binder/MemoryHeapBase.h>
#include "psb_android_glue.h"
#include "psb_output_android.h"
#include <cutils/log.h>
#include <ui/Rect.h>
#include <system/window.h>
#include <system/graphics.h>
#ifdef TARGET_HAS_MULTIPLE_DISPLAY
#include "psb_mds.h"
#endif

#ifdef  LOG_TAG
#undef  LOG_TAG
#endif

#define LOG_TAG "pvr_drv_video"

using namespace android;
#ifdef TARGET_HAS_MULTIPLE_DISPLAY
using namespace android::intel;
#endif

#ifdef TARGET_HAS_MULTIPLE_DISPLAY

void init_mds_listener(void* output) {
    psb_android_output_p aoutput = (psb_android_output_p)output;
    if (aoutput == NULL) {
        ALOGE("Invalid input parameter");
        return;
    }
    if (aoutput->mds == NULL)
        aoutput->mds = new psbMultiDisplayListener();
}

void deinit_mds_listener(void* output) {
    psb_android_output_p aoutput = (psb_android_output_p)output;
    if (aoutput == NULL) {
        ALOGE("Invalid input parameter");
        return;
    }
    if (aoutput->mds != NULL) {
        delete (psbMultiDisplayListener*)(aoutput->mds);
        aoutput->mds = NULL;
    }
}

int psb_android_get_mds_mode(void* output) {
    if (output == NULL)
        return MDS_INIT_VALUE;
    psb_android_output_p aoutput = (psb_android_output_p)output;
    if (aoutput->mds == NULL)
        init_mds_listener(aoutput);
    psbMultiDisplayListener* mds =
        static_cast<psbMultiDisplayListener*>(aoutput->mds);
    if (mds == NULL)
        return MDS_INIT_VALUE;
    return mds->getMode();
}

int psb_android_get_mds_decoder_output_resolution(void* output,
        int* width, int* height,
        int* offX, int* offY,
        int* bufW, int* bufH) {
    if (output == NULL ||
            width == NULL || height == NULL ||
            offX  == NULL || offY == NULL ||
            bufW  == NULL || bufH == NULL)
        return 0;
    psb_android_output_p aoutput = (psb_android_output_p)output;
    if (aoutput->mds == NULL)
        init_mds_listener(aoutput);
    psbMultiDisplayListener* mds =
        static_cast<psbMultiDisplayListener*>(aoutput->mds);
    if (mds == NULL)
        return 0;
    bool ret = mds->getDecoderOutputResolution(width, height, offX, offY, bufW, bufH);
    return (ret ? 1 : 0);
}

int psb_android_get_mds_vpp_state(void* output) {
    bool ret = false;
    if (output == NULL) {
        sp<IServiceManager> sm = defaultServiceManager();
        if (sm == NULL)
            return 0;
        sp<IMDService> imds = interface_cast<IMDService>(
                sm->getService(String16(INTEL_MDS_SERVICE_NAME)));
        if (imds == NULL)
            return 0;
        sp<IMultiDisplayInfoProvider> mds = imds->getInfoProvider();
        if (mds != NULL) {
            ret = mds->getVppState();
        }
        mds = NULL;
        return (ret ? 1 : 0);
    }
    psb_android_output_p aoutput = (psb_android_output_p)output;
    if (aoutput->mds == NULL)
        init_mds_listener(aoutput);
    psbMultiDisplayListener* mds =
        static_cast<psbMultiDisplayListener*>(aoutput->mds);
    ret = mds->getVppState();
    if (mds == NULL)
        return 0;
    return (ret ? 1 : 0);
}

#else //TARGET_HAS_MULTIPLE_DISPLAY

#ifdef PSBVIDEO_MRFL_VPP
/* VPP is always enabled. It disables decoder rotate.
 * TODO: remove the dependency the on libVPP. Get it form ISB configure
 */
int psb_android_get_vpp_state() {
    return 1;
}

#endif
#endif

unsigned int update_forced;

int psb_android_surfaceflinger_rotate(void* native_window, int *rotation)
{
    sp<ANativeWindow> mNativeWindow = static_cast<ANativeWindow*>(native_window);
    int err, transform_hint;

    if (mNativeWindow.get()) {
        err = mNativeWindow->query(mNativeWindow.get(), NATIVE_WINDOW_TRANSFORM_HINT, &transform_hint);
        if (err != 0) {
            ALOGE("%s: NATIVE_WINDOW_TRANSFORM_HINT query failed", __func__);
            return -1;
        }
        switch (transform_hint) {
        case HAL_TRANSFORM_ROT_90:
            *rotation = 1;
            break;
        case HAL_TRANSFORM_ROT_180:
            *rotation = 2;
            break;
        case HAL_TRANSFORM_ROT_270:
            *rotation = 3;
            break;
        default:
            *rotation = 0;
        }
    }
    return 0;
}
