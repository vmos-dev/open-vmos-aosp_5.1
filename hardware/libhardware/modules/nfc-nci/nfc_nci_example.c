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

#include <cutils/log.h>
#include <hardware/hardware.h>
#include <hardware/nfc.h>


/*
 * NCI HAL method implementations. These must be overriden
 */
static int hal_open(const struct nfc_nci_device *dev,
        nfc_stack_callback_t *p_cback, nfc_stack_data_callback_t *p_data_cback) {
    ALOGE("NFC-NCI HAL: %s", __FUNCTION__);
    return 0;
}

static int hal_write(const struct nfc_nci_device *dev,
        uint16_t data_len, const uint8_t *p_data) {
    ALOGE("NFC-NCI HAL: %s", __FUNCTION__);
    return 0;
}

static int hal_core_initialized(const struct nfc_nci_device *dev,
        uint8_t* p_core_init_rsp_params) {
    ALOGE("NFC-NCI HAL: %s", __FUNCTION__);
    return 0;
}

static int hal_pre_discover(const struct nfc_nci_device *dev) {
    ALOGE("NFC-NCI HAL: %s", __FUNCTION__);
    return 0;
}

static int hal_close(const struct nfc_nci_device *dev) {
    ALOGE("NFC-NCI HAL: %s", __FUNCTION__);
    return 0;
}

static int hal_control_granted (const struct nfc_nci_device *p_dev)
{
    ALOGE("NFC-NCI HAL: %s", __FUNCTION__);
    return 0;
}


static int hal_power_cycle (const struct nfc_nci_device *p_dev)
{
    ALOGE("NFC-NCI HAL: %s", __FUNCTION__);
    return 0;
}

/*
 * Generic device handling below - can generally be left unchanged.
 */
/* Close an opened nfc device instance */
static int nfc_close(hw_device_t *dev) {
    free(dev);
    return 0;
}

static int nfc_open(const hw_module_t* module, const char* name,
        hw_device_t** device) {
    if (strcmp(name, NFC_NCI_CONTROLLER) == 0) {
        nfc_nci_device_t *dev = calloc(1, sizeof(nfc_nci_device_t));

        dev->common.tag = HARDWARE_DEVICE_TAG;
        dev->common.version = 0x00010000; // [31:16] major, [15:0] minor
        dev->common.module = (struct hw_module_t*) module;
        dev->common.close = nfc_close;

        // NCI HAL method pointers
        dev->open = hal_open;
        dev->write = hal_write;
        dev->core_initialized = hal_core_initialized;
        dev->pre_discover = hal_pre_discover;
        dev->close = hal_close;
        dev->control_granted = hal_control_granted;
        dev->power_cycle = hal_power_cycle;

        *device = (hw_device_t*) dev;

        return 0;
    } else {
        return -EINVAL;
    }
}


static struct hw_module_methods_t nfc_module_methods = {
    .open = nfc_open,
};

struct nfc_nci_module_t HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .module_api_version = 0x0100, // [15:8] major, [7:0] minor (1.0)
        .hal_api_version = 0x00, // 0 is only valid value
        .id = NFC_NCI_HARDWARE_MODULE_ID,
        .name = "Default NFC NCI HW HAL",
        .author = "The Android Open Source Project",
        .methods = &nfc_module_methods,
    },
};
