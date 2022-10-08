/*
 * Copyright (C) 2014 The Android Open Source Project
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
#define LOG_TAG "FingerprintHal"

#include <errno.h>
#include <string.h>
#include <cutils/log.h>
#include <hardware/hardware.h>
#include <hardware/fingerprint.h>

static int fingerprint_close(hw_device_t *dev)
{
    if (dev) {
        free(dev);
        return 0;
    } else {
        return -1;
    }
}

static int fingerprint_enroll(struct fingerprint_device __unused *dev,
                                uint32_t __unused timeout_sec) {
    return FINGERPRINT_ERROR;
}

static int fingerprint_remove(struct fingerprint_device __unused *dev,
                                uint32_t __unused fingerprint_id) {
    return FINGERPRINT_ERROR;
}

static int set_notify_callback(struct fingerprint_device *dev,
                                fingerprint_notify_t notify) {
    /* Decorate with locks */
    dev->notify = notify;
    return FINGERPRINT_ERROR;
}

static int fingerprint_open(const hw_module_t* module, const char __unused *id,
                            hw_device_t** device)
{
    if (device == NULL) {
        ALOGE("NULL device on open");
        return -EINVAL;
    }

    fingerprint_device_t *dev = malloc(sizeof(fingerprint_device_t));
    memset(dev, 0, sizeof(fingerprint_device_t));

    dev->common.tag = HARDWARE_DEVICE_TAG;
    dev->common.version = HARDWARE_MODULE_API_VERSION(1, 0);
    dev->common.module = (struct hw_module_t*) module;
    dev->common.close = fingerprint_close;

    dev->enroll = fingerprint_enroll;
    dev->remove = fingerprint_remove;
    dev->set_notify = set_notify_callback;
    dev->notify = NULL;

    *device = (hw_device_t*) dev;
    return 0;
}

static struct hw_module_methods_t fingerprint_module_methods = {
    .open = fingerprint_open,
};

fingerprint_module_t HAL_MODULE_INFO_SYM = {
    .common = {
        .tag                = HARDWARE_MODULE_TAG,
        .module_api_version = FINGERPRINT_MODULE_API_VERSION_1_0,
        .hal_api_version    = HARDWARE_HAL_API_VERSION,
        .id                 = FINGERPRINT_HARDWARE_MODULE_ID,
        .name               = "Demo Fingerprint HAL",
        .author             = "The Android Open Source Project",
        .methods            = &fingerprint_module_methods,
    },
};
