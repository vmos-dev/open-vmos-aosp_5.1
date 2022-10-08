/*
 * Copyright (C) 2010 The Android Open Source Project
 * Copyright (C) 2012-2013, The Linux Foundation. All rights reserved.
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

namespace qhwc {

#define HWC_VSYNC_THREAD_NAME "hwcVsyncThread"
#define MAX_SYSFS_FILE_PATH             255

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

static void *vsync_loop(void *param)
{
    hwc_context_t * ctx = reinterpret_cast<hwc_context_t *>(param);

    char thread_name[64] = HWC_VSYNC_THREAD_NAME;
    prctl(PR_SET_NAME, (unsigned long) &thread_name, 0, 0, 0);
    setpriority(PRIO_PROCESS, 0, HAL_PRIORITY_URGENT_DISPLAY +
                android::PRIORITY_MORE_FAVORABLE);

    const int MAX_DATA = 64;
    char vdata[MAX_DATA];
    bool logvsync = false;

    struct pollfd pfd[2];
    int fb_fd[2];
    uint64_t timestamp[2];
    int num_displays;

    char property[PROPERTY_VALUE_MAX];
    if(property_get("debug.hwc.fakevsync", property, NULL) > 0) {
        if(atoi(property) == 1)
            ctx->vstate.fakevsync = true;
    }

    if(property_get("debug.hwc.logvsync", property, 0) > 0) {
        if(atoi(property) == 1)
            logvsync = true;
    }

    if (ctx->mExtDisplay->getHDMIIndex() > 0)
        num_displays = 2;
    else
        num_displays = 1;

    char vsync_node_path[MAX_SYSFS_FILE_PATH];
    for (int dpy = HWC_DISPLAY_PRIMARY; dpy < num_displays; dpy++) {
        snprintf(vsync_node_path, sizeof(vsync_node_path),
                "/sys/class/graphics/fb%d/vsync_event",
                dpy == HWC_DISPLAY_PRIMARY ? 0 :
                ctx->mExtDisplay->getHDMIIndex());
        ALOGI("%s: Reading vsync for dpy=%d from %s", __FUNCTION__, dpy,
                vsync_node_path);
        fb_fd[dpy] = open(vsync_node_path, O_RDONLY);

        if (fb_fd[dpy] < 0) {
            // Make sure fb device is opened before starting this thread so this
            // never happens.
            ALOGE ("%s:not able to open vsync node for dpy=%d, %s",
                    __FUNCTION__, dpy, strerror(errno));
            if (dpy == HWC_DISPLAY_PRIMARY) {
                ctx->vstate.fakevsync = true;
                break;
            }
        }
        // Read once from the fds to clear the first notify
        pread(fb_fd[dpy], vdata , MAX_DATA, 0);

        pfd[dpy].fd = fb_fd[dpy];
        if (pfd[dpy].fd >= 0)
            pfd[dpy].events = POLLPRI | POLLERR;
    }

    if (LIKELY(!ctx->vstate.fakevsync)) {
        do {
            int err = poll(pfd, num_displays, -1);
            if(err > 0) {
                for (int dpy = HWC_DISPLAY_PRIMARY; dpy < num_displays; dpy++) {
                    if (pfd[dpy].revents & POLLPRI) {
                        int len = pread(pfd[dpy].fd, vdata, MAX_DATA, 0);
                        if (UNLIKELY(len < 0)) {
                            // If the read was just interrupted - it is not a
                            // fatal error. Just continue in this case
                            ALOGE ("%s: Unable to read vsync for dpy=%d : %s",
                                    __FUNCTION__, dpy, strerror(errno));
                            continue;
                        }
                        // extract timestamp
                        if (!strncmp(vdata, "VSYNC=", strlen("VSYNC="))) {
                            timestamp[dpy] = strtoull(vdata + strlen("VSYNC="),
                                    NULL, 0);
                        }
                        // send timestamp to SurfaceFlinger
                        ALOGD_IF (logvsync,
                                "%s: timestamp %llu sent to SF for dpy=%d",
                                __FUNCTION__, timestamp[dpy], dpy);
                        ctx->proc->vsync(ctx->proc, dpy, timestamp[dpy]);
                    }
                }

            } else {
                ALOGE("%s: vsync poll failed errno: %s", __FUNCTION__,
                        strerror(errno));
                continue;
            }
        } while (true);

    } else {

        //Fake vsync is used only when set explicitly through a property or when
        //the vsync timestamp node cannot be opened at bootup. There is no
        //fallback to fake vsync from the true vsync loop, ever, as the
        //condition can easily escape detection.
        //Also, fake vsync is delivered only for the primary display.
        do {
            usleep(16666);
            timestamp[HWC_DISPLAY_PRIMARY] = systemTime();
            ctx->proc->vsync(ctx->proc, HWC_DISPLAY_PRIMARY,
                    timestamp[HWC_DISPLAY_PRIMARY]);

        } while (true);
    }

    for (int dpy = HWC_DISPLAY_PRIMARY; dpy <= HWC_DISPLAY_EXTERNAL; dpy++ ) {
        if(fb_fd[dpy] >= 0)
            close (fb_fd[dpy]);
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
