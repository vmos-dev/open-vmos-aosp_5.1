/*
 * Copyright (C) 2011 The Android Open Source Project
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

#define LOG_TAG "local_time_hw_default"
//#define LOG_NDEBUG 0

#include <errno.h>
#include <stdint.h>
#include <sys/time.h>
#include <linux/time.h>

#include <cutils/log.h>

#include <hardware/hardware.h>
#include <hardware/local_time_hal.h>

struct stub_local_time_device {
    struct local_time_hw_device device;
};

static int64_t ltdev_get_local_time(struct local_time_hw_device* dev)
{
    struct timespec ts;
    uint64_t now;
    int ret;

    ret = clock_gettime(CLOCK_MONOTONIC, &ts);
    if (ret < 0) {
        ALOGW("%s failed to fetch CLOCK_MONOTONIC value! (res = %d)",
                dev->common.module->name, ret);
        return 0;
    }

    now = (((uint64_t)ts.tv_sec) * 1000000000ull) +
           ((uint64_t)ts.tv_nsec);

    return (int64_t)now;
}

static uint64_t ltdev_get_local_freq(struct local_time_hw_device* dev)
{
    // For better or worse, linux clock_gettime routines normalize all clock
    // frequencies to 1GHz
    return 1000000000ull;
}

static int ltdev_close(hw_device_t *device)
{
    free(device);
    return 0;
}

static int ltdev_open(const hw_module_t* module, const char* name,
                     hw_device_t** device)
{
    struct stub_local_time_device *ltdev;
    struct timespec ts;
    int ret;

    if (strcmp(name, LOCAL_TIME_HARDWARE_INTERFACE) != 0)
        return -EINVAL;

    ltdev = calloc(1, sizeof(struct stub_local_time_device));
    if (!ltdev)
        return -ENOMEM;

    ltdev->device.common.tag = HARDWARE_DEVICE_TAG;
    ltdev->device.common.version = 0;
    ltdev->device.common.module = (struct hw_module_t *) module;
    ltdev->device.common.close = ltdev_close;

    ltdev->device.get_local_time = ltdev_get_local_time;
    ltdev->device.get_local_freq = ltdev_get_local_freq;
    ltdev->device.set_local_slew = NULL;
    ltdev->device.get_debug_log  = NULL;

    *device = &ltdev->device.common;

    return 0;
}

static struct hw_module_methods_t hal_module_methods = {
    .open = ltdev_open,
};

struct local_time_module HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .version_major = 1,
        .version_minor = 0,
        .id = LOCAL_TIME_HARDWARE_MODULE_ID,
        .name = "Default local_time HW HAL",
        .author = "The Android Open Source Project",
        .methods = &hal_module_methods,
    },
};
