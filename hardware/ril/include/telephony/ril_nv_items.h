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

#ifndef ANDROID_RIL_NV_ITEMS_H
#define ANDROID_RIL_NV_ITEMS_H 1

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Must match the values in RadioNVItems.java in frameworks/opt/telephony. */
typedef enum {

    // CDMA radio and account information (items 1-10)
    RIL_NV_CDMA_MEID = 1,                   // CDMA MEID (hex)
    RIL_NV_CDMA_MIN = 2,                    // CDMA MIN (MSID)
    RIL_NV_CDMA_MDN = 3,                    // CDMA MDN
    RIL_NV_CDMA_ACCOLC = 4,                 // CDMA access overload control

    // Carrier device provisioning (items 11-30)
    RIL_NV_DEVICE_MSL = 11,                 // device MSL
    RIL_NV_RTN_RECONDITIONED_STATUS = 12,   // RTN reconditioned status
    RIL_NV_RTN_ACTIVATION_DATE = 13,        // RTN activation date
    RIL_NV_RTN_LIFE_TIMER = 14,             // RTN life timer
    RIL_NV_RTN_LIFE_CALLS = 15,             // RTN life calls
    RIL_NV_RTN_LIFE_DATA_TX = 16,           // RTN life data TX
    RIL_NV_RTN_LIFE_DATA_RX = 17,           // RTN life data RX
    RIL_NV_OMADM_HFA_LEVEL = 18,            // HFA in progress

    // Mobile IP profile information (items 31-50)
    RIL_NV_MIP_PROFILE_NAI = 31,            // NAI realm
    RIL_NV_MIP_PROFILE_HOME_ADDRESS = 32,   // MIP home address
    RIL_NV_MIP_PROFILE_AAA_AUTH = 33,       // AAA auth
    RIL_NV_MIP_PROFILE_HA_AUTH = 34,        // HA auth
    RIL_NV_MIP_PROFILE_PRI_HA_ADDR = 35,    // primary HA address
    RIL_NV_MIP_PROFILE_SEC_HA_ADDR = 36,    // secondary HA address
    RIL_NV_MIP_PROFILE_REV_TUN_PREF = 37,   // reverse TUN preference
    RIL_NV_MIP_PROFILE_HA_SPI = 38,         // HA SPI
    RIL_NV_MIP_PROFILE_AAA_SPI = 39,        // AAA SPI
    RIL_NV_MIP_PROFILE_MN_HA_SS = 40,       // HA shared secret
    RIL_NV_MIP_PROFILE_MN_AAA_SS = 41,      // AAA shared secret

    // CDMA network and band config (items 51-70)
    RIL_NV_CDMA_PRL_VERSION = 51,           // CDMA PRL version
    RIL_NV_CDMA_BC10 = 52,                  // CDMA band class 10
    RIL_NV_CDMA_BC14 = 53,                  // CDMA band class 14
    RIL_NV_CDMA_SO68 = 54,                  // CDMA SO68
    RIL_NV_CDMA_SO73_COP0 = 55,             // CDMA SO73 COP0
    RIL_NV_CDMA_SO73_COP1TO7 = 56,          // CDMA SO73 COP1-7
    RIL_NV_CDMA_1X_ADVANCED_ENABLED = 57,   // CDMA 1X Advanced enabled
    RIL_NV_CDMA_EHRPD_ENABLED = 58,         // CDMA eHRPD enabled
    RIL_NV_CDMA_EHRPD_FORCED = 59,          // CDMA eHRPD forced

    // LTE network and band config (items 71-90)
    RIL_NV_LTE_BAND_ENABLE_25 = 71,         // LTE band 25 enable
    RIL_NV_LTE_BAND_ENABLE_26 = 72,         // LTE band 26 enable
    RIL_NV_LTE_BAND_ENABLE_41 = 73,         // LTE band 41 enable

    RIL_NV_LTE_SCAN_PRIORITY_25 = 74,       // LTE band 25 scan priority
    RIL_NV_LTE_SCAN_PRIORITY_26 = 75,       // LTE band 26 scan priority
    RIL_NV_LTE_SCAN_PRIORITY_41 = 76,       // LTE band 41 scan priority

    RIL_NV_LTE_HIDDEN_BAND_PRIORITY_25 = 77,    // LTE hidden band 25 priority
    RIL_NV_LTE_HIDDEN_BAND_PRIORITY_26 = 78,    // LTE hidden band 26 priority
    RIL_NV_LTE_HIDDEN_BAND_PRIORITY_41 = 79,    // LTE hidden band 41 priority

} RIL_NV_Item;

#ifdef __cplusplus
}
#endif

#endif /* ANDROID_RIL_NV_ITEMS_H */
