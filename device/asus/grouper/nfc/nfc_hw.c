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

static uint8_t pn544_eedata_settings[][4] = {
    // DIFFERENTIAL_ANTENNA

    // RF Settings
    {0x00,0x9B,0xD1,0x0D} // Tx consumption higher than 0x0D (average 50mA)
    ,{0x00,0x9B,0xD2,0x24} // GSP setting for this threshold
    ,{0x00,0x9B,0xD3,0x0A} // Tx consumption higher than 0x0A (average 40mA)
    ,{0x00,0x9B,0xD4,0x22} // GSP setting for this threshold
    ,{0x00,0x9B,0xD5,0x08} // Tx consumption higher than 0x08 (average 30mA)
    ,{0x00,0x9B,0xD6,0x1E} // GSP setting for this threshold
    ,{0x00,0x9B,0xDD,0x1C} // GSP setting for this threshold
    ,{0x00,0x9B,0x84,0x13} // ANACM2 setting
    ,{0x00,0x99,0x81,0x7F} // ANAVMID setting PCD
    ,{0x00,0x99,0x7A,0x3E} // ANATXMODGSPON
    ,{0x00,0x99,0x77,0xFF} // ANATXCWGSPON
    ,{0x00,0x99,0x31,0x70} // ANAVMID setting PICC
    ,{0x00,0x99,0x2A,0xF5} // ANATXMODGSN-TYPEB
#ifdef grouper
    ,{0x00,0x99,0x29,0xF5} // ANATXMODGSN-TYPEA
#else
    ,{0x00,0x99,0x29,0xFF} // ANATXMODGSN-TYPEA
#endif
    // For tegra we don't override load modulation settings.

    // Enable PBTF
    ,{0x00,0x98,0x00,0x3F} // SECURE_ELEMENT_CONFIGURATION - No Secure Element
    ,{0x00,0x9F,0x09,0x00} // SWP_PBTF_RFU
    ,{0x00,0x9F,0x0A,0x05} // SWP_PBTF_RFLD  --> RFLEVEL Detector for PBTF
    ,{0x00,0x9E,0xD1,0xA1} //

    // Change RF Level Detector ANARFLDWU
    ,{0x00,0x99,0x23,0x00} // Default Value is 0x01

    // Low-power polling
    ,{0x00,0x9E,0x74,0xB0} // Default Value is 0x00, bits 0->2: sensitivity (0==max,  6==min),
                           // bit 3: RFU,
                           // bits 4,5 hybrid low-power: # of low-power polls per regular poll
                           // bit 6: RFU
                           // bit 7: (0 -> disabled, 1 -> enabled)
    ,{0x00,0x9E,0x7D,0xB0} // bits 0->3: RFU,
                           // bits 4,5: # retries after low power detection
                           // 0=1 retry, 1=2 retry, 2=3 retry, 3=4 retry
                           // bit 6: RFU,
                           // bit 7: Enable or disable retry mechanism (0: disable, 1: enable)
    ,{0x00,0x9F,0x28,0x01} // bits 0->7: # of measurements per low-power poll

    // Polling Loop - Card Emulation Timeout
    ,{0x00,0x9F,0x35,0x14} // Time for which PN544 stays in Card Emulation mode after leaving RF field
    ,{0x00,0x9F,0x36,0x60} // Default value 0x0411 = 50 ms ---> New Value : 0x1460 = 250 ms

    //LLC Timer
    ,{0x00,0x9C,0x31,0x00} // Guard host time-out in ms (MSB)
    ,{0x00,0x9C,0x32,0xC8} // Guard host time-out in ms (LSB)
    ,{0x00,0x9C,0x19,0x40} // Max RX retry (PN544=>host?)
    ,{0x00,0x9C,0x1A,0x40} // Max TX retry (PN544=>host?)

    ,{0x00,0x9C,0x0C,0x00} //
    ,{0x00,0x9C,0x0D,0x00} //
    ,{0x00,0x9C,0x12,0x00} //
    ,{0x00,0x9C,0x13,0x00} //

    //WTX for LLCP communication
    ,{0x00,0x98,0xA2,0x0E} // Max value: 14 (default value: 09)

    //SE GPIO
    ,{0x00, 0x98, 0x93, 0x40}

    // Set NFCT ATQA
    ,{0x00, 0x98, 0x7D, 0x02}
    ,{0x00, 0x98, 0x7E, 0x00}

    // Enable CEA detection mechanism
    ,{0x00, 0x9F, 0xC8, 0x01}
    // Set NFC-F poll RC=0x00
    ,{0x00, 0x9F, 0x9A, 0x00}
    // Setting for EMD support for ISO 14443-4 Reader
    ,{0x00,0x9F,0x09,0x00} // 0x00 - Disable EMD support, 0x01 - Enable EMD support
};

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

        dev->num_eeprom_settings = sizeof(pn544_eedata_settings) / 4;
        dev->eeprom_settings = (uint8_t*)pn544_eedata_settings;
        dev->linktype = PN544_LINK_TYPE_I2C;
        dev->device_node = "/dev/pn544";
        dev->enable_i2c_workaround = 1;
        dev->i2c_device_address = 0x51;
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
        .name = "Tegra NFC HW HAL",
        .author = "The Android Open Source Project",
        .methods = &nfc_module_methods,
    },
};
