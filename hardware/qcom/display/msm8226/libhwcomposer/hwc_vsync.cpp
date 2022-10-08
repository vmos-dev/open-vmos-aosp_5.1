/*
 * Copyright (C) 2010 The Android Open Source Project
 * Copyright (C) 2012-2014, The Linux Foundation. All rights reserved.
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

#include <cutils/properties.h>
#include <utils/Log.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/msm_mdp.h>
#include <sys/resource.h>
#include <sys/prctl.h>
#include <poll.h>
#include "hwc_utils.h"
#include "string.h"
#include "external.h"
#include "overlay.h"
#define __STDC_FORMAT_MACROS 1
#include <inttypes.h>

namespace qhwc {

#define HWC_VSYNC_THREAD_NAME "hwcVsyncThread"
#define MAX_SYSFS_FILE_PATH             255
#define PANEL_ON_STR "panel_power_on ="
#define ARRAY_LENGTH(array) (sizeof((array))/sizeof((array)[0]))
const int MAX_DATA = 64;

int hwc_vsync_control(hwc_context_t* ctx, int dpy, int enable)
{
    int ret = 0;
    if(!ctx->vstate.fakevsync &&
       ioctl(ctx->dpyAttr[dpy].fd, MSMFB_OVERLAY_VSYNC_CTRL,
             &enable) < 0) {
        ALOGE("%s: vsync control failed. Dpy=%d, enable=%d : %s",
              __FUNCTION__, dpy, enable, strerror(errno));
        ret = -errno;
    }
    return ret;
}

static void handle_vsync_event(hwc_context_t* ctx, int dpy, char *data)
{
    // extract timestamp
    uint64_t timestamp = 0;
    if (!strncmp(data, "VSYNC=", strlen("VSYNC="))) {
        timestamp = strtoull(data + strlen("VSYNC="), NULL, 0);
    }
    // send timestamp to SurfaceFlinger
    ALOGD_IF (ctx->vstate.debug, "%s: timestamp %"PRIu64" sent to SF for dpy=%d",
            __FUNCTION__, timestamp, dpy);
    ctx->proc->vsync(ctx->proc, dpy, timestamp);
}

static void handle_blank_event(hwc_context_t* ctx, int dpy, char *data)
{
    if (!strncmp(data, PANEL_ON_STR, strlen(PANEL_ON_STR))) {
        unsigned long int poweron = strtoul(data + strlen(PANEL_ON_STR), NULL, 0);
        ALOGI("%s: dpy:%d panel power state: %ld", __FUNCTION__, dpy, poweron);
        ctx->dpyAttr[dpy].isActive = poweron ? true: false;
    }
}

struct event {
    const char* name;
    void (*callback)(hwc_context_t* ctx, int dpy, char *data);
};

struct event event_list[] =  {
    { "vsync_event", handle_vsync_event },
    { "show_blank_event", handle_blank_event },
};

#define num_events ARRAY_LENGTH(event_list)
#define VSYNC_FAILURE_THRESHOLD 2

static void *vsync_loop(void *param)
{
    hwc_context_t * ctx = reinterpret_cast<hwc_context_t *>(param);

    char thread_name[64] = HWC_VSYNC_THREAD_NAME;
    prctl(PR_SET_NAME, (unsigned long) &thread_name, 0, 0, 0);
    setpriority(PRIO_PROCESS, 0, HAL_PRIORITY_URGENT_DISPLAY +
                android::PRIORITY_MORE_FAVORABLE);

    char vdata[MAX_DATA];
    //Number of physical displays
    //We poll on all the nodes.
    int num_displays = HWC_NUM_DISPLAY_TYPES - 1;
    struct pollfd pfd[num_displays][num_events];

    char property[PROPERTY_VALUE_MAX];
    if(property_get("debug.hwc.fakevsync", property, NULL) > 0) {
        if(atoi(property) == 1)
            ctx->vstate.fakevsync = true;
    }

    char node_path[MAX_SYSFS_FILE_PATH];
#ifdef VSYNC_FAILURE_FALLBACK
    int fake_vsync_switch_cnt = 0;
#endif

    for (int dpy = HWC_DISPLAY_PRIMARY; dpy < num_displays; dpy++) {
        for(size_t ev = 0; ev < num_events; ev++) {
            snprintf(node_path, sizeof(node_path),
                    "/sys/class/graphics/fb%d/%s",
                    dpy == HWC_DISPLAY_PRIMARY ? 0 :
                    overlay::Overlay::getInstance()->
                    getFbForDpy(HWC_DISPLAY_EXTERNAL),
                    event_list[ev].name);

            ALOGI("%s: Reading event %zu for dpy %d from %s", __FUNCTION__,
                    ev, dpy, node_path);
            pfd[dpy][ev].fd = open(node_path, O_RDONLY);

            if (dpy == HWC_DISPLAY_PRIMARY && pfd[dpy][ev].fd < 0) {
                // Make sure fb device is opened before starting
                // this thread so this never happens.
                ALOGE ("%s:unable to open event node for dpy=%d event=%zu, %s",
                        __FUNCTION__, dpy, ev, strerror(errno));
                if (ev == 0) {
                    ctx->vstate.fakevsync = true;
                    //XXX: Blank events don't work with fake vsync,
                    //but we shouldn't be running on fake vsync anyway.
                    break;
                }
            }

            pread(pfd[dpy][ev].fd, vdata , MAX_DATA, 0);
            if (pfd[dpy][ev].fd >= 0)
                pfd[dpy][ev].events = POLLPRI | POLLERR;
        }
    }

    if (LIKELY(!ctx->vstate.fakevsync)) {
        do {
            int err = poll(*pfd, (int)(num_displays * num_events), -1);
            if(err > 0) {
                for (int dpy = HWC_DISPLAY_PRIMARY; dpy < num_displays; dpy++) {
                    for(size_t ev = 0; ev < num_events; ev++) {
                        if (pfd[dpy][ev].revents & POLLPRI) {
                            ssize_t len = pread(pfd[dpy][ev].fd, vdata, MAX_DATA, 0);
                            if (UNLIKELY(len < 0)) {
                                // If the read was just interrupted - it is not
                                // a fatal error. Just continue in this case
                                ALOGE ("%s: Unable to read event:%zu for \
                                        dpy=%d : %s",
                                        __FUNCTION__, ev, dpy, strerror(errno));
#ifdef VSYNC_FAILURE_FALLBACK
                                fake_vsync_switch_cnt++;
                                if (fake_vsync_switch_cnt >
                                        VSYNC_FAILURE_THRESHOLD) {
                                    ALOGE("%s: Too many errors, falling back to fake vsync ",
                                            __FUNCTION__);
                                    ctx->vstate.fakevsync = true;
                                    goto vsync_failed;
                                }
#endif
                                continue;
                            }
                            event_list[ev].callback(ctx, dpy, vdata);
                        }
                    }
                }
            } else {
                ALOGE("%s: poll failed errno: %s", __FUNCTION__,
                        strerror(errno));
                continue;
            }
        } while (true);

    } else {

#ifdef VSYNC_FAILURE_FALLBACK
vsync_failed:
#endif
        //Fake vsync is used only when set explicitly through a property or when
        //the vsync timestamp node cannot be opened at bootup. There is no
        //fallback to fake vsync from the true vsync loop, ever, as the
        //condition can easily escape detection.
        //Also, fake vsync is delivered only for the primary display.
        do {
            usleep(16666);
            uint64_t timestamp = systemTime();
            ctx->proc->vsync(ctx->proc, HWC_DISPLAY_PRIMARY, timestamp);

        } while (true);
    }

    for (int dpy = HWC_DISPLAY_PRIMARY; dpy <= HWC_DISPLAY_EXTERNAL; dpy++ ) {
        for( size_t event = 0; event < num_events; event++) {
            if(pfd[dpy][event].fd >= 0)
                close (pfd[dpy][event].fd);
        }
    }

    return NULL;
}

void init_vsync_thread(hwc_context_t* ctx)
{
    int ret;
    pthread_t vsync_thread;
    ALOGI("Initializing VSYNC Thread");
    ret = pthread_create(&vsync_thread, NULL, vsync_loop, (void*) ctx);
    if (ret) {
        ALOGE("%s: failed to create %s: %s", __FUNCTION__,
              HWC_VSYNC_THREAD_NAME, strerror(ret));
    }
}

}; //namespace
