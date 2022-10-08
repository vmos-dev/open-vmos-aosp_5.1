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
#define ATRACE_TAG (ATRACE_TAG_GRAPHICS | ATRACE_TAG_HAL)
#include <fcntl.h>
#include <errno.h>

#include <cutils/log.h>
#include <cutils/atomic.h>
#include <EGL/egl.h>
#include <utils/Trace.h>
#include <sys/ioctl.h>
#include <overlay.h>
#include <overlayRotator.h>
#include <overlayWriteback.h>
#include <mdp_version.h>
#include "hwc_utils.h"
#include "hwc_fbupdate.h"
#include "hwc_mdpcomp.h"
#include "external.h"
#include "hwc_copybit.h"
#include "hwc_ad.h"
#include "profiler.h"

using namespace qhwc;
using namespace overlay;

#define VSYNC_DEBUG 0
#define BLANK_DEBUG 1

static int hwc_device_open(const struct hw_module_t* module,
                           const char* name,
                           struct hw_device_t** device);

static struct hw_module_methods_t hwc_module_methods = {
    open: hwc_device_open
};

hwc_module_t HAL_MODULE_INFO_SYM = {
    common: {
        tag: HARDWARE_MODULE_TAG,
        version_major: 2,
        version_minor: 0,
        id: HWC_HARDWARE_MODULE_ID,
        name: "Qualcomm Hardware Composer Module",
        author: "CodeAurora Forum",
        methods: &hwc_module_methods,
        dso: 0,
        reserved: {0},
    }
};

/*
 * Save callback functions registered to HWC
 */
static void hwc_registerProcs(struct hwc_composer_device_1* dev,
                              hwc_procs_t const* procs)
{
    ALOGI("%s", __FUNCTION__);
    hwc_context_t* ctx = (hwc_context_t*)(dev);
    if(!ctx) {
        ALOGE("%s: Invalid context", __FUNCTION__);
        return;
    }
    ctx->proc = procs;

    // Now that we have the functions needed, kick off
    // the uevent & vsync threads
    init_uevent_thread(ctx);
    init_vsync_thread(ctx);
}

//Helper
static void reset(hwc_context_t *ctx, int numDisplays,
                  hwc_display_contents_1_t** displays) {
    for(int i = 0; i < MAX_DISPLAYS; i++) {
        hwc_display_contents_1_t *list = displays[i];
        // XXX:SurfaceFlinger no longer guarantees that this
        // value is reset on every prepare. However, for the layer
        // cache we need to reset it.
        // We can probably rethink that later on
        if (LIKELY(list && list->numHwLayers > 1)) {
            for(uint32_t j = 0; j < list->numHwLayers; j++) {
                if(list->hwLayers[j].compositionType != HWC_FRAMEBUFFER_TARGET)
                    list->hwLayers[j].compositionType = HWC_FRAMEBUFFER;
            }
        }

        if(ctx->mFBUpdate[i])
            ctx->mFBUpdate[i]->reset();
        if(ctx->mMDPComp[i])
            ctx->mMDPComp[i]->reset();
        if(ctx->mCopyBit[i])
            ctx->mCopyBit[i]->reset();
        if(ctx->mLayerRotMap[i])
            ctx->mLayerRotMap[i]->reset();
    }

    ctx->mAD->reset();
}

//clear prev layer prop flags and realloc for current frame
static void reset_layer_prop(hwc_context_t* ctx, int dpy, int numAppLayers) {
    if(ctx->layerProp[dpy]) {
       delete[] ctx->layerProp[dpy];
       ctx->layerProp[dpy] = NULL;
    }
    ctx->layerProp[dpy] = new LayerProp[numAppLayers];
}

static void handleGeomChange(hwc_context_t *ctx, int dpy,
        hwc_display_contents_1_t *list) {
    if(list->flags & HWC_GEOMETRY_CHANGED) {
        ctx->mOverlay->forceSet(dpy);
    }
}

static int display_commit(hwc_context_t *ctx, int dpy) {
    int fbFd = ctx->dpyAttr[dpy].fd;
    if(fbFd == -1) {
        ALOGE("%s: Invalid FB fd for display: %d", __FUNCTION__, dpy);
        return -1;
    }

    struct mdp_display_commit commit_info;
    memset(&commit_info, 0, sizeof(struct mdp_display_commit));
    commit_info.flags = MDP_DISPLAY_COMMIT_OVERLAY;
    if(ioctl(fbFd, MSMFB_DISPLAY_COMMIT, &commit_info) == -1) {
       ALOGE("%s: MSMFB_DISPLAY_COMMIT for primary failed", __FUNCTION__);
       return -errno;
    }
    return 0;
}

static int hwc_prepare_primary(hwc_composer_device_1 *dev,
        hwc_display_contents_1_t *list) {
    ATRACE_CALL();
    hwc_context_t* ctx = (hwc_context_t*)(dev);
    const int dpy = HWC_DISPLAY_PRIMARY;
    if (LIKELY(list && list->numHwLayers > 1) &&
            ctx->dpyAttr[dpy].isActive) {
        reset_layer_prop(ctx, dpy, list->numHwLayers - 1);
        handleGeomChange(ctx, dpy, list);
        setListStats(ctx, list, dpy);
        if(ctx->mMDPComp[dpy]->prepare(ctx, list) < 0) {
            const int fbZ = 0;
            ctx->mFBUpdate[dpy]->prepare(ctx, list, fbZ);
        }
        if (ctx->mMDP.version < qdutils::MDP_V4_0) {
            if(ctx->mCopyBit[dpy])
                ctx->mCopyBit[dpy]->prepare(ctx, list, dpy);
        }
    }
    return 0;
}

static int hwc_prepare_external(hwc_composer_device_1 *dev,
        hwc_display_contents_1_t *list) {

    ATRACE_CALL();
    hwc_context_t* ctx = (hwc_context_t*)(dev);
    const int dpy = HWC_DISPLAY_EXTERNAL;

    if (LIKELY(list && list->numHwLayers > 1) &&
            ctx->dpyAttr[dpy].isActive &&
            ctx->dpyAttr[dpy].connected) {
        reset_layer_prop(ctx, dpy, list->numHwLayers - 1);
        handleGeomChange(ctx, dpy, list);
        if(!ctx->dpyAttr[dpy].isPause) {
            ctx->dpyAttr[dpy].isConfiguring = false;
            setListStats(ctx, list, dpy);
            if(ctx->mMDPComp[dpy]->prepare(ctx, list) < 0) {
                const int fbZ = 0;
                ctx->mFBUpdate[dpy]->prepare(ctx, list, fbZ);
            }

            /* Temporarily commenting out C2D until we support partial
               copybit composition for mixed mode MDP

               if((fbZOrder >= 0) && ctx->mCopyBit[dpy])
               ctx->mCopyBit[dpy]->prepare(ctx, list, dpy);
             */
        } else {
            // External Display is in Pause state.
            // ToDo:
            // Mark all application layers as OVERLAY so that
            // GPU will not compose. This is done for power
            // optimization
        }
    }
    return 0;
}

static int hwc_prepare_virtual(hwc_composer_device_1 *dev,
        hwc_display_contents_1_t *list) {
    ATRACE_CALL();
    hwc_context_t* ctx = (hwc_context_t*)(dev);
    const int dpy = HWC_DISPLAY_VIRTUAL;

    if (list && list->outbuf && list->numHwLayers > 0) {
        reset_layer_prop(ctx, dpy, list->numHwLayers - 1);
        uint32_t last = list->numHwLayers - 1;
        hwc_layer_1_t *fbLayer = &list->hwLayers[last];
        int fbWidth = 0, fbHeight = 0;
        getLayerResolution(fbLayer, fbWidth, fbHeight);
        ctx->dpyAttr[dpy].xres = fbWidth;
        ctx->dpyAttr[dpy].yres = fbHeight;

        if(ctx->dpyAttr[dpy].connected == false) {
            ctx->dpyAttr[dpy].connected = true;
            setupSecondaryObjs(ctx, dpy);
        }

        ctx->dpyAttr[dpy].fd = Writeback::getInstance()->getFbFd();
        private_handle_t *ohnd = (private_handle_t *)list->outbuf;
        Writeback::getInstance()->configureDpyInfo(ohnd->width, ohnd->height);
        setListStats(ctx, list, dpy);

        if(ctx->mMDPComp[dpy]->prepare(ctx, list) < 0) {
            const int fbZ = 0;
            ctx->mFBUpdate[dpy]->prepare(ctx, list, fbZ);
        }
    }
    return 0;
}

static int hwc_prepare(hwc_composer_device_1 *dev, size_t numDisplays,
                       hwc_display_contents_1_t** displays)
{
    int ret = 0;
    hwc_context_t* ctx = (hwc_context_t*)(dev);
    //Will be unlocked at the end of set
    ctx->mDrawLock.lock();
    reset(ctx, numDisplays, displays);

    ctx->mOverlay->configBegin();
    ctx->mRotMgr->configBegin();
    overlay::Writeback::configBegin();

    Overlay::setDMAMode(Overlay::DMA_LINE_MODE);

    //Cleanup virtual display objs, since there is no explicit disconnect
    if(ctx->dpyAttr[HWC_DISPLAY_VIRTUAL].connected &&
        (numDisplays <= HWC_NUM_PHYSICAL_DISPLAY_TYPES ||
        displays[HWC_DISPLAY_VIRTUAL] == NULL)) {
        ctx->dpyAttr[HWC_DISPLAY_VIRTUAL].connected = false;
        clearSecondaryObjs(ctx, HWC_DISPLAY_VIRTUAL);
    }

    for (int32_t i = numDisplays - 1; i >= 0; i--) {
        hwc_display_contents_1_t *list = displays[i];
        switch(i) {
            case HWC_DISPLAY_PRIMARY:
                ret = hwc_prepare_primary(dev, list);
                break;
            case HWC_DISPLAY_EXTERNAL:
                ret = hwc_prepare_external(dev, list);
                break;
            case HWC_DISPLAY_VIRTUAL:
                ret = hwc_prepare_virtual(dev, list);
                break;
            default:
                ret = -EINVAL;
        }
    }

    ctx->mOverlay->configDone();
    ctx->mRotMgr->configDone();
    overlay::Writeback::configDone();

    return ret;
}

static int hwc_eventControl(struct hwc_composer_device_1* dev, int dpy,
                             int event, int enable)
{
    ATRACE_CALL();
    int ret = 0;
    hwc_context_t* ctx = (hwc_context_t*)(dev);
    switch(event) {
        case HWC_EVENT_VSYNC:
            if(!ctx->dpyAttr[dpy].isActive) {
                ALOGE("Display is blanked - Cannot %s vsync",
                        enable ? "enable" : "disable");
                return -EINVAL;
            }

            if (ctx->vstate.enable == enable)
                break;
            ret = hwc_vsync_control(ctx, dpy, enable);
            if(ret == 0)
                ctx->vstate.enable = !!enable;
            ALOGD_IF (VSYNC_DEBUG, "VSYNC state changed to %s",
                      (enable)?"ENABLED":"DISABLED");
            break;
        default:
            ret = -EINVAL;
    }
    return ret;
}

static int hwc_blank(struct hwc_composer_device_1* dev, int dpy, int blank)
{
    ATRACE_CALL();
    hwc_context_t* ctx = (hwc_context_t*)(dev);

    Locker::Autolock _l(ctx->mDrawLock);
    int ret = 0;
    ALOGD_IF(BLANK_DEBUG, "%s: %s display: %d", __FUNCTION__,
          blank==1 ? "Blanking":"Unblanking", dpy);
    if (blank || (dpy == HWC_DISPLAY_PRIMARY &&
        !ctx->dpyAttr[dpy].isActive )) {
        // free up all the overlay pipes in use
        // when we get a blank for either display
        // makes sure that all pipes are freed
        ctx->mOverlay->configBegin();
        ctx->mOverlay->configDone();
        ctx->mRotMgr->clear();
        overlay::Writeback::clear();
    }
    switch(dpy) {
        case HWC_DISPLAY_PRIMARY:
            if(blank) {
                ret = ioctl(ctx->dpyAttr[dpy].fd, FBIOBLANK,
                            FB_BLANK_POWERDOWN);
            } else {
                ret = ioctl(ctx->dpyAttr[dpy].fd, FBIOBLANK,FB_BLANK_UNBLANK);
            }
            break;
        case HWC_DISPLAY_EXTERNAL:
        case HWC_DISPLAY_VIRTUAL:
            if(blank) {
                // call external framebuffer commit on blank,
                // so that any pipe unsets gets committed
                if (display_commit(ctx, dpy) < 0) {
                    ret = -1;
                    ALOGE("%s:post failed for dpy %d",
                          __FUNCTION__, dpy);
                }
            } else {
            }
            break;
        default:
            return -EINVAL;
    }
    if(ret == 0){
        ctx->dpyAttr[dpy].isActive = !blank;
    } else {
        ALOGE("%s: Failed in %s display: %d error:%s", __FUNCTION__,
              blank==1 ? "blanking":"unblanking", dpy, strerror(errno));
        return ret;
    }

    ALOGD_IF(BLANK_DEBUG, "%s: Done %s display: %d", __FUNCTION__,
          blank==1 ? "blanking":"unblanking", dpy);
    return 0;
}

static int hwc_query(struct hwc_composer_device_1* dev,
                     int param, int* value)
{
    hwc_context_t* ctx = (hwc_context_t*)(dev);
    int supported = HWC_DISPLAY_PRIMARY_BIT;

    switch (param) {
    case HWC_BACKGROUND_LAYER_SUPPORTED:
        // Not supported for now
        value[0] = 0;
        break;
    case HWC_DISPLAY_TYPES_SUPPORTED: //Unused by f/w
        if(ctx->mMDP.hasOverlay) {
            supported |= HWC_DISPLAY_VIRTUAL_BIT;
            if(!qdutils::MDPVersion::getInstance().is8x26())
                supported |= HWC_DISPLAY_EXTERNAL_BIT;
        }
        value[0] = supported;
        break;
    default:
        return -EINVAL;
    }
    return 0;

}


static int hwc_set_primary(hwc_context_t *ctx, hwc_display_contents_1_t* list) {
    ATRACE_CALL();
    int ret = 0;
    const int dpy = HWC_DISPLAY_PRIMARY;

    if (LIKELY(list) && ctx->dpyAttr[dpy].isActive) {
        uint32_t last = list->numHwLayers - 1;
        hwc_layer_1_t *fbLayer = &list->hwLayers[last];
        int fd = -1; //FenceFD from the Copybit(valid in async mode)
        bool copybitDone = false;
        if(ctx->mCopyBit[dpy])
            copybitDone = ctx->mCopyBit[dpy]->draw(ctx, list, dpy, &fd);
        if(list->numHwLayers > 1)
            hwc_sync(ctx, list, dpy, fd);

        if (!ctx->mMDPComp[dpy]->draw(ctx, list)) {
            ALOGE("%s: MDPComp draw failed", __FUNCTION__);
            ret = -1;
        }

        //TODO We dont check for SKIP flag on this layer because we need PAN
        //always. Last layer is always FB
        private_handle_t *hnd = (private_handle_t *)fbLayer->handle;
        if(copybitDone) {
            hnd = ctx->mCopyBit[dpy]->getCurrentRenderBuffer();
        }

        if(hnd) {
            if (!ctx->mFBUpdate[dpy]->draw(ctx, hnd)) {
                ALOGE("%s: FBUpdate draw failed", __FUNCTION__);
                ret = -1;
            }
        }

        if (display_commit(ctx, dpy) < 0) {
            ALOGE("%s: display commit fail!", __FUNCTION__);
            ret = -1;
        }
    }

    closeAcquireFds(list, dpy);
    return ret;
}

static int hwc_set_external(hwc_context_t *ctx,
        hwc_display_contents_1_t* list)
{
    ATRACE_CALL();
    int ret = 0;
    const int dpy = HWC_DISPLAY_EXTERNAL;

    if (LIKELY(list) && ctx->dpyAttr[dpy].isActive &&
        !ctx->dpyAttr[dpy].isPause &&
        ctx->dpyAttr[dpy].connected) {
        uint32_t last = list->numHwLayers - 1;
        hwc_layer_1_t *fbLayer = &list->hwLayers[last];
        int fd = -1; //FenceFD from the Copybit(valid in async mode)
        bool copybitDone = false;
        if(ctx->mCopyBit[dpy])
            copybitDone = ctx->mCopyBit[dpy]->draw(ctx, list, dpy, &fd);

        if(list->numHwLayers > 1)
            hwc_sync(ctx, list, dpy, fd);

        if (!ctx->mMDPComp[dpy]->draw(ctx, list)) {
            ALOGE("%s: MDPComp draw failed", __FUNCTION__);
            ret = -1;
        }

        private_handle_t *hnd = (private_handle_t *)fbLayer->handle;
        if(copybitDone) {
            hnd = ctx->mCopyBit[dpy]->getCurrentRenderBuffer();
        }

        if(hnd) {
            if (!ctx->mFBUpdate[dpy]->draw(ctx, hnd)) {
                ALOGE("%s: FBUpdate::draw fail!", __FUNCTION__);
                ret = -1;
            }
        }

        if (display_commit(ctx, dpy) < 0) {
            ALOGE("%s: display commit fail!", __FUNCTION__);
            ret = -1;
        }
    }

    closeAcquireFds(list, dpy);
    return ret;
}

static int hwc_set_virtual(hwc_context_t *ctx,
        hwc_display_contents_1_t* list) {
    ATRACE_CALL();
    int ret = 0;
    const int dpy = HWC_DISPLAY_VIRTUAL;

    if (list && list->outbuf && list->numHwLayers > 0) {
        uint32_t last = list->numHwLayers - 1;
        hwc_layer_1_t *fbLayer = &list->hwLayers[last];

        if(fbLayer->handle && ctx->dpyAttr[dpy].connected
#ifndef FORCE_HWC_FOR_VIRTUAL_DISPLAYS
                //XXX: If we're not forcing virtual via HWC,
                //full GLES compositions will not be routed through here.
                && !isGLESOnlyComp(ctx, dpy)
#endif
        ) {

            private_handle_t *ohnd = (private_handle_t *)list->outbuf;
            int format = ohnd->format;
            if (format == HAL_PIXEL_FORMAT_RGBA_8888)
                format = HAL_PIXEL_FORMAT_RGBX_8888;
            Writeback::getInstance()->setOutputFormat(
                    utils::getMdpFormat(format));

            int fd = -1; //FenceFD from the Copybit
            hwc_sync(ctx, list, dpy, fd);

            if (!ctx->mMDPComp[dpy]->draw(ctx, list)) {
                ALOGE("%s: MDPComp draw failed", __FUNCTION__);
                ret = -1;
            }

            if (!ctx->mFBUpdate[dpy]->draw(ctx,
                        (private_handle_t *)fbLayer->handle)) {
                ALOGE("%s: FBUpdate::draw fail!", __FUNCTION__);
                ret = -1;
            }

            Writeback::getInstance()->queueBuffer(ohnd->fd, ohnd->offset);
            if (display_commit(ctx, dpy) < 0) {
                ALOGE("%s: display commit fail!", __FUNCTION__);
                ret = -1;
            }
        } else if(list->outbufAcquireFenceFd >= 0) {
            //If we dont handle the frame, set retireFenceFd to outbufFenceFd,
            //which will make sure, the framework waits on it and closes it.
            //The other way is to wait on outbufFenceFd ourselves, close it and
            //set retireFenceFd to -1. Since we want hwc to be async, choosing
            //the former.
            //Also dup because, the closeAcquireFds() will close the outbufFence
            list->retireFenceFd = dup(list->outbufAcquireFenceFd);
        }
    }

    closeAcquireFds(list, dpy);
    return ret;
}


static int hwc_set(hwc_composer_device_1 *dev,
                   size_t numDisplays,
                   hwc_display_contents_1_t** displays)
{
    int ret = 0;
    hwc_context_t* ctx = (hwc_context_t*)(dev);
    for (uint32_t i = 0; i < numDisplays; i++) {
        hwc_display_contents_1_t* list = displays[i];
        switch(i) {
            case HWC_DISPLAY_PRIMARY:
                ret = hwc_set_primary(ctx, list);
                break;
            case HWC_DISPLAY_EXTERNAL:
                ret = hwc_set_external(ctx, list);
                break;
            case HWC_DISPLAY_VIRTUAL:
                ret = hwc_set_virtual(ctx, list);
                break;
            default:
                ret = -EINVAL;
        }
    }
    // This is only indicative of how many times SurfaceFlinger posts
    // frames to the display.
    CALC_FPS();
    MDPComp::resetIdleFallBack();
    ctx->mVideoTransFlag = false;
    //Was locked at the beginning of prepare
    ctx->mDrawLock.unlock();
    return ret;
}

int hwc_getDisplayConfigs(struct hwc_composer_device_1* dev, int disp,
        uint32_t* configs, size_t* numConfigs) {
    int ret = 0;
    hwc_context_t* ctx = (hwc_context_t*)(dev);
    //in 1.1 there is no way to choose a config, report as config id # 0
    //This config is passed to getDisplayAttributes. Ignore for now.
    switch(disp) {
        case HWC_DISPLAY_PRIMARY:
            if(*numConfigs > 0) {
                configs[0] = 0;
                *numConfigs = 1;
            }
            ret = 0; //NO_ERROR
            break;
        case HWC_DISPLAY_EXTERNAL:
            ret = -1; //Not connected
            if(ctx->dpyAttr[HWC_DISPLAY_EXTERNAL].connected) {
                ret = 0; //NO_ERROR
                if(*numConfigs > 0) {
                    configs[0] = 0;
                    *numConfigs = 1;
                }
            }
            break;
    }
    return ret;
}

int hwc_getDisplayAttributes(struct hwc_composer_device_1* dev, int disp,
        uint32_t config, const uint32_t* attributes, int32_t* values) {

    hwc_context_t* ctx = (hwc_context_t*)(dev);
    //If hotpluggable displays are inactive return error
    if(disp == HWC_DISPLAY_EXTERNAL && !ctx->dpyAttr[disp].connected) {
        return -1;
    }

    //From HWComposer
    static const uint32_t DISPLAY_ATTRIBUTES[] = {
        HWC_DISPLAY_VSYNC_PERIOD,
        HWC_DISPLAY_WIDTH,
        HWC_DISPLAY_HEIGHT,
        HWC_DISPLAY_DPI_X,
        HWC_DISPLAY_DPI_Y,
        HWC_DISPLAY_NO_ATTRIBUTE,
    };

    const int NUM_DISPLAY_ATTRIBUTES = (sizeof(DISPLAY_ATTRIBUTES) /
            sizeof(DISPLAY_ATTRIBUTES)[0]);

    for (size_t i = 0; i < NUM_DISPLAY_ATTRIBUTES - 1; i++) {
        switch (attributes[i]) {
        case HWC_DISPLAY_VSYNC_PERIOD:
            values[i] = ctx->dpyAttr[disp].vsync_period;
            break;
        case HWC_DISPLAY_WIDTH:
            values[i] = ctx->dpyAttr[disp].xres;
            ALOGD("%s disp = %d, width = %d",__FUNCTION__, disp,
                    ctx->dpyAttr[disp].xres);
            break;
        case HWC_DISPLAY_HEIGHT:
            values[i] = ctx->dpyAttr[disp].yres;
            ALOGD("%s disp = %d, height = %d",__FUNCTION__, disp,
                    ctx->dpyAttr[disp].yres);
            break;
        case HWC_DISPLAY_DPI_X:
            values[i] = (int32_t) (ctx->dpyAttr[disp].xdpi*1000.0);
            break;
        case HWC_DISPLAY_DPI_Y:
            values[i] = (int32_t) (ctx->dpyAttr[disp].ydpi*1000.0);
            break;
        default:
            ALOGE("Unknown display attribute %d",
                    attributes[i]);
            return -EINVAL;
        }
    }
    return 0;
}

void hwc_dump(struct hwc_composer_device_1* dev, char *buff, int buff_len)
{
    hwc_context_t* ctx = (hwc_context_t*)(dev);
    Locker::Autolock _l(ctx->mDrawLock);
    android::String8 aBuf("");
    dumpsys_log(aBuf, "Qualcomm HWC state:\n");
    dumpsys_log(aBuf, "  MDPVersion=%d\n", ctx->mMDP.version);
    dumpsys_log(aBuf, "  DisplayPanel=%c\n", ctx->mMDP.panel);
    for(int dpy = 0; dpy < MAX_DISPLAYS; dpy++) {
        if(ctx->mMDPComp[dpy])
            ctx->mMDPComp[dpy]->dump(aBuf);
    }
    char ovDump[2048] = {'\0'};
    ctx->mOverlay->getDump(ovDump, 2048);
    dumpsys_log(aBuf, ovDump);
    ovDump[0] = '\0';
    ctx->mRotMgr->getDump(ovDump, 1024);
    dumpsys_log(aBuf, ovDump);
    ovDump[0] = '\0';
    if(Writeback::getDump(ovDump, 1024)) {
        dumpsys_log(aBuf, ovDump);
        ovDump[0] = '\0';
    }
    strlcpy(buff, aBuf.string(), buff_len);
}

static int hwc_device_close(struct hw_device_t *dev)
{
    if(!dev) {
        ALOGE("%s: NULL device pointer", __FUNCTION__);
        return -1;
    }
    closeContext((hwc_context_t*)dev);
    free(dev);

    return 0;
}

static int hwc_device_open(const struct hw_module_t* module, const char* name,
                           struct hw_device_t** device)
{
    int status = -EINVAL;

    if (!strcmp(name, HWC_HARDWARE_COMPOSER)) {
        struct hwc_context_t *dev;
        dev = (hwc_context_t*)malloc(sizeof(*dev));
        memset(dev, 0, sizeof(*dev));

        //Initialize hwc context
        initContext(dev);

        //Setup HWC methods
        dev->device.common.tag          = HARDWARE_DEVICE_TAG;
        dev->device.common.version      = HWC_DEVICE_API_VERSION_1_3;
        dev->device.common.module       = const_cast<hw_module_t*>(module);
        dev->device.common.close        = hwc_device_close;
        dev->device.prepare             = hwc_prepare;
        dev->device.set                 = hwc_set;
        dev->device.eventControl        = hwc_eventControl;
        dev->device.blank               = hwc_blank;
        dev->device.query               = hwc_query;
        dev->device.registerProcs       = hwc_registerProcs;
        dev->device.dump                = hwc_dump;
        dev->device.getDisplayConfigs   = hwc_getDisplayConfigs;
        dev->device.getDisplayAttributes = hwc_getDisplayAttributes;
        *device = &dev->device.common;
        status = 0;
    }
    return status;
}
