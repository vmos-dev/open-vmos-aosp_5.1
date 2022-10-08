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
#include <errno.h>
#include <string.h>

#include <hardware/hardware.h>
#include <hardware/nfc.h>

/* Close an opened pn544 device instance */
static int pn544_close(hw_device_t *dev) {
    free(dev);
    return 0;
}

/*
 * Generic device handling
 */
static int nfc_open(const hw_module_t* module, const char* name,
        hw_device_t** device) {
    if (strcmp(name, NFC_PN544_CONTROLLER) == 0) {
        nfc_pn544_device_t *dev = calloc(1, sizeof(nfc_pn544_device_t));

        dev->common.tag = HARDWARE_DEVICE_TAG;
        dev->common.version = 0;
        dev->common.module = (struct hw_module_t*) module;
        dev->common.close = pn544_close;

        /* Example settings */
        dev->num_eeprom_settings = 0;
        dev->eeprom_settings = NULL;
        dev->linktype = PN544_LINK_TYPE_INVALID;
        dev->device_node = NULL;
        dev->enable_i2c_workaround = 0;
        dev->i2c_device_address = 0;

        *device = (hw_device_t*) dev;
        return 0;
    } else {
        return -EINVAL;
    }
}


static struct hw_module_methods_t nfc_module_methods = {
    .open = nfc_open,
};

struct nfc_module_t HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .version_major = 1,
        .version_minor = 0,
        .id = NFC_HARDWARE_MODULE_ID,
        .name = "Default NFC HW HAL",
        .author = "The Android Open Source Project",
        .methods = &nfc_module_methods,
    },
};
