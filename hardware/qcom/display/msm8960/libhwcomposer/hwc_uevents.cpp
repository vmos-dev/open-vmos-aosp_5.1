
/*
 * Copyright (C) 2010 The Android Open Source Project
 * Copyright (C) 2012, The Linux Foundation. All rights reserved.
 *
 * Not a Contribution, Apache license notifications and license are
 * retained for attribution purposes only.

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
#define UEVENT_DEBUG 0
#include <hardware_legacy/uevent.h>
#include <utils/Log.h>
#include <sys/resource.h>
#include <sys/prctl.h>
#include <string.h>
#include <stdlib.h>
#include "hwc_utils.h"
#include "hwc_fbupdate.h"
#include "hwc_mdpcomp.h"
#include "hwc_copybit.h"
#include "comptype.h"
#include "external.h"

namespace qhwc {

#define HWC_UEVENT_THREAD_NAME "hwcUeventThread"

/* External Display states */
enum {
    EXTERNAL_OFFLINE = 0,
    EXTERNAL_ONLINE,
    EXTERNAL_PAUSE,
    EXTERNAL_RESUME
};

static bool isHDMI(const char* str)
{
    if(strcasestr("change@/devices/virtual/switch/hdmi", str))
        return true;
    return false;
}

static void handle_uevent(hwc_context_t* ctx, const char* udata, int len)
{
    int vsync = 0;
    int64_t timestamp = 0;
    const char *str = udata;
    bool usecopybit = false;
    int compositionType =
        qdutils::QCCompositionType::getInstance().getCompositionType();

    if (compositionType & (qdutils::COMPOSITION_TYPE_DYN |
                           qdutils::COMPOSITION_TYPE_MDP |
                           qdutils::COMPOSITION_TYPE_C2D)) {
        usecopybit = true;
    }

    if(!strcasestr("change@/devices/virtual/switch/hdmi", str) &&
       !strcasestr("change@/devices/virtual/switch/wfd", str)) {
        ALOGD_IF(UEVENT_DEBUG, "%s: Not Ext Disp Event ", __FUNCTION__);
        return;
    }
    int connected = -1; // initial value - will be set to  1/0 based on hotplug
    int extDpyNum = HWC_DISPLAY_EXTERNAL;
    char property[PROPERTY_VALUE_MAX];
    if((property_get("persist.sys.wfd.virtual", property, NULL) > 0) &&
            (!strncmp(property, "1", PROPERTY_VALUE_MAX ) ||
             (!strncasecmp(property,"true", PROPERTY_VALUE_MAX )))) {
        // This means we are using Google API to trigger WFD Display
        extDpyNum = HWC_DISPLAY_VIRTUAL;

    }

    int dpy = isHDMI(str) ? HWC_DISPLAY_EXTERNAL : extDpyNum;

    // update extDpyNum
    ctx->mExtDisplay->setExtDpyNum(dpy);

    // parse HDMI/WFD switch state for connect/disconnect
    // for HDMI:
    // The event will be of the form:
    // change@/devices/virtual/switch/hdmi ACTION=change
    // SWITCH_STATE=1 or SWITCH_STATE=0
    while(*str) {
        if (!strncmp(str, "SWITCH_STATE=", strlen("SWITCH_STATE="))) {
            connected = atoi(str + strlen("SWITCH_STATE="));
            //Disabled until SF calls unblank
            ctx->dpyAttr[HWC_DISPLAY_EXTERNAL].isActive = false;
            //Ignored for Virtual Displays
            //ToDo: we can do this in a much better way
            ctx->dpyAttr[HWC_DISPLAY_VIRTUAL].isActive = true;
            break;
        }
        str += strlen(str) + 1;
        if (str - udata >= len)
            break;
    }

    switch(connected) {
        case EXTERNAL_OFFLINE:
            {   // disconnect event
                ctx->mExtDisplay->processUEventOffline(udata);
                if(ctx->mFBUpdate[dpy]) {
                    Locker::Autolock _l(ctx->mExtSetLock);
                    delete ctx->mFBUpdate[dpy];
                    ctx->mFBUpdate[dpy] = NULL;
                }
                if(ctx->mCopyBit[dpy]){
                    Locker::Autolock _l(ctx->mExtSetLock);
                    delete ctx->mCopyBit[dpy];
                    ctx->mCopyBit[dpy] = NULL;
                }
                if(ctx->mMDPComp[dpy]) {
                    delete ctx->mMDPComp[dpy];
                    ctx->mMDPComp[dpy] = NULL;
                }
                ALOGD("%s sending hotplug: connected = %d and dpy:%d",
                      __FUNCTION__, connected, dpy);
                ctx->dpyAttr[dpy].connected = false;
                Locker::Autolock _l(ctx->mExtSetLock);
                //hwc comp could be on
                ctx->proc->hotplug(ctx->proc, dpy, connected);
                break;
            }
        case EXTERNAL_ONLINE:
            {   // connect case
                ctx->mExtDispConfiguring = true;
                ctx->mExtDisplay->processUEventOnline(udata);
                ctx->mFBUpdate[dpy] =
                        IFBUpdate::getObject(ctx->dpyAttr[dpy].xres, dpy);
                ctx->dpyAttr[dpy].isPause = false;
                if(usecopybit)
                    ctx->mCopyBit[dpy] = new CopyBit();
                ctx->mMDPComp[dpy] =  MDPComp::getObject(
                        ctx->dpyAttr[dpy].xres, dpy);
                ALOGD("%s sending hotplug: connected = %d", __FUNCTION__,
                        connected);
                ctx->dpyAttr[dpy].connected = true;
                Locker::Autolock _l(ctx->mExtSetLock); //hwc comp could be on
                ctx->proc->hotplug(ctx->proc, dpy, connected);
                break;
            }
        case EXTERNAL_PAUSE:
            {   // pause case
                ALOGD("%s Received Pause event",__FUNCTION__);
                // This is required to ensure that composition
                // fall back to FB, closing all MDP pipes.
                ctx->mExtDispConfiguring = true;
                ctx->dpyAttr[dpy].isActive = true;
                ctx->dpyAttr[dpy].isPause = true;
                break;
            }
        case EXTERNAL_RESUME:
            {  // resume case
                ALOGD("%s Received resume event",__FUNCTION__);
                ctx->dpyAttr[dpy].isActive = true;
                ctx->dpyAttr[dpy].isPause = false;
                break;
            }
        default:
            {
                ALOGE("ignore event and connected:%d",connected);
                break;
            }
    }
}

static void *uevent_loop(void *param)
{
    int len = 0;
    static char udata[PAGE_SIZE];
    hwc_context_t * ctx = reinterpret_cast<hwc_context_t *>(param);
    char thread_name[64] = HWC_UEVENT_THREAD_NAME;
    prctl(PR_SET_NAME, (unsigned long) &thread_name, 0, 0, 0);
    setpriority(PRIO_PROCESS, 0, HAL_PRIORITY_URGENT_DISPLAY);
    uevent_init();

    while(1) {
        len = uevent_next_event(udata, sizeof(udata) - 2);
        handle_uevent(ctx, udata, len);
    }

    return NULL;
}

void init_uevent_thread(hwc_context_t* ctx)
{
    pthread_t uevent_thread;
    int ret;

    ALOGI("Initializing UEVENT Thread");
    ret = pthread_create(&uevent_thread, NULL, uevent_loop, (void*) ctx);
    if (ret) {
        ALOGE("%s: failed to create %s: %s", __FUNCTION__,
            HWC_UEVENT_THREAD_NAME, strerror(ret));
    }
}

}; //namespace
