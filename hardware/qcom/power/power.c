/*
 * Copyright (C) 2012 The Android Open Source Project
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
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <dlfcn.h>

#define LOG_TAG "PowerHAL"
#include <utils/Log.h>

#include <hardware/hardware.h>
#include <hardware/power.h>

#define STATE_ON "state=1"
#define STATE_OFF "state=0"
#define STATE_HDR_ON "state=2"
#define STATE_HDR_OFF "state=3"

#define MAX_LENGTH         50
#define BOOST_SOCKET       "/dev/socket/pb"

static int client_sockfd;
static struct sockaddr_un client_addr;
static int last_state = -1;

static void socket_init()
{
    if (!client_sockfd) {
        client_sockfd = socket(PF_UNIX, SOCK_DGRAM, 0);
        if (client_sockfd < 0) {
            ALOGE("%s: failed to open: %s", __func__, strerror(errno));
            return;
        }
        memset(&client_addr, 0, sizeof(struct sockaddr_un));
        client_addr.sun_family = AF_UNIX;
        snprintf(client_addr.sun_path, UNIX_PATH_MAX, BOOST_SOCKET);
    }
}

static void power_init(struct power_module *module)
{
    ALOGI("%s", __func__);
    socket_init();
}

static void sync_thread(int off)
{
    int rc;
    pid_t client;
    char data[MAX_LENGTH];

    if (client_sockfd < 0) {
        ALOGE("%s: boost socket not created", __func__);
        return;
    }

    client = getpid();

    if (!off) {
        snprintf(data, MAX_LENGTH, "2:%d", client);
        rc = sendto(client_sockfd, data, strlen(data), 0, (const struct sockaddr *)&client_addr, sizeof(struct sockaddr_un));
    } else {
        snprintf(data, MAX_LENGTH, "3:%d", client);
        rc = sendto(client_sockfd, data, strlen(data), 0, (const struct sockaddr *)&client_addr, sizeof(struct sockaddr_un));
    }

    if (rc < 0) {
        ALOGE("%s: failed to send: %s", __func__, strerror(errno));
    }
}

static void enc_boost(int off)
{
    int rc;
    pid_t client;
    char data[MAX_LENGTH];

    if (client_sockfd < 0) {
        ALOGE("%s: boost socket not created", __func__);
        return;
    }

    client = getpid();

    if (!off) {
        snprintf(data, MAX_LENGTH, "5:%d", client);
        rc = sendto(client_sockfd, data, strlen(data), 0, (const struct sockaddr *)&client_addr, sizeof(struct sockaddr_un));
    } else {
        snprintf(data, MAX_LENGTH, "6:%d", client);
        rc = sendto(client_sockfd, data, strlen(data), 0, (const struct sockaddr *)&client_addr, sizeof(struct sockaddr_un));
    }

    if (rc < 0) {
        ALOGE("%s: failed to send: %s", __func__, strerror(errno));
    }
}

static void process_video_encode_hint(void *metadata)
{

    socket_init();

    if (client_sockfd < 0) {
        ALOGE("%s: boost socket not created", __func__);
        return;
    }

    if (metadata) {
        if (!strncmp(metadata, STATE_ON, sizeof(STATE_ON))) {
            /* Video encode started */
            sync_thread(1);
            enc_boost(1);
        } else if (!strncmp(metadata, STATE_OFF, sizeof(STATE_OFF))) {
            /* Video encode stopped */
            sync_thread(0);
            enc_boost(0);
        }  else if (!strncmp(metadata, STATE_HDR_ON, sizeof(STATE_HDR_ON))) {
            /* HDR usecase started */
        } else if (!strncmp(metadata, STATE_HDR_OFF, sizeof(STATE_HDR_OFF))) {
            /* HDR usecase stopped */
        }else
            return;
    } else {
        return;
    }
}


static void touch_boost()
{
    int rc;
    pid_t client;
    char data[MAX_LENGTH];

    if (client_sockfd < 0) {
        ALOGE("%s: boost socket not created", __func__);
        return;
    }

    client = getpid();

    snprintf(data, MAX_LENGTH, "1:%d", client);
    rc = sendto(client_sockfd, data, strlen(data), 0, (const struct sockaddr *)&client_addr, sizeof(struct sockaddr_un));
    if (rc < 0) {
        ALOGE("%s: failed to send: %s", __func__, strerror(errno));
    }
}

static void power_set_interactive(struct power_module *module, int on)
{
    if (last_state == -1) {
        last_state = on;
    } else {
        if (last_state == on)
            return;
        else
            last_state = on;
    }

    ALOGV("%s %s", __func__, (on ? "ON" : "OFF"));
    if (on) {
        sync_thread(0);
        touch_boost();
    } else {
        sync_thread(1);
    }
}

static void power_hint(struct power_module *module, power_hint_t hint,
                       void *data) {
    switch (hint) {
        case POWER_HINT_INTERACTION:
            ALOGV("POWER_HINT_INTERACTION");
            touch_boost();
            break;
#if 0
        case POWER_HINT_VSYNC:
            ALOGV("POWER_HINT_VSYNC %s", (data ? "ON" : "OFF"));
            break;
#endif
        case POWER_HINT_VIDEO_ENCODE:
            process_video_encode_hint(data);
            break;
        default:
             break;
    }
}

static struct hw_module_methods_t power_module_methods = {
    .open = NULL,
};

struct power_module HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .module_api_version = POWER_MODULE_API_VERSION_0_2,
        .hal_api_version = HARDWARE_HAL_API_VERSION,
        .id = POWER_HARDWARE_MODULE_ID,
        .name = "Qualcomm Power HAL",
        .author = "The Android Open Source Project",
        .methods = &power_module_methods,
    },

    .init = power_init,
    .setInteractive = power_set_interactive,
    .powerHint = power_hint,
};
