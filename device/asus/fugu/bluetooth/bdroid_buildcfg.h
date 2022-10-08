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

#ifndef _BDROID_BUILDCFG_H
#define _BDROID_BUILDCFG_H

#define BTM_DEF_LOCAL_NAME "Fugu"
#define BTA_DM_COD {0x20, BTM_COD_MAJOR_AUDIO, BTM_COD_MINOR_SET_TOP_BOX}

#define BLE_VND_INCLUDED TRUE

// Turn off BLE_PRIVACY_SPT.  Remote reconnect fails on
// often if this is enabled.
#define BLE_PRIVACY_SPT FALSE

// Force connection interval to 13.75ms
#define BTM_BLE_CONN_INT_MIN_DEF 11 /* 13.75ms = 11 * 1.25 */
#define BTM_BLE_CONN_INT_MAX_DEF BTM_BLE_CONN_INT_MIN_DEF

// Allow better battery life
#define BTM_BLE_CONN_SLAVE_LATENCY_DEF 24

// Detect disconnects faster
#define BTM_BLE_CONN_TIMEOUT_DEF 300

// Increase background scanning to reduce reconnect time
#define BTM_BLE_SCAN_SLOW_INT_1    110    /* 68.75 ms   = 110 *0.625 */
#define BTM_BLE_SCAN_SLOW_WIN_1    8      /* 5 ms = 8 *0.625 */

// Disable HFP
#define BTIF_HF_SERVICES (BTA_HSP_SERVICE_MASK)
#define BTIF_HF_SERVICE_NAMES  { BTIF_HSAG_SERVICE_NAME, NULL }

// Disable compiling code in Bluedroid for profiles we don't support
#define BTA_PAN_INCLUDED FALSE
#define BNEP_INCLUDED FALSE
#define AVDT_INCLUDED FALSE
#define PAN_INCLUDED FALSE
#define AVCT_INCLUDED FALSE

/* We will support a remote +  4 game controllers.  To be able to
 * allocate sufficient bandwidth for all devices we will restrict the
 * Game Controllers to a sniff interval of 13.75ms.
 */
#define BTA_DM_PM_SNIFF4_MAX     22
#define BTA_DM_PM_SNIFF4_MIN     22
#define BTA_DM_PM_SNIFF4_ATTEMPT 1
#define BTA_DM_PM_SNIFF4_TIMEOUT 0

#define BTA_DM_PM_SNIFF_HH_OPEN_IDX BTA_DM_PM_SNIFF4
#define BTA_DM_PM_HH_OPEN_DELAY 0

#define BTA_DM_PM_SNIFF_HH_ACTIVE_IDX BTA_DM_PM_SNIFF4
#define BTA_DM_PM_HH_ACTIVE_DELAY 0

#define BTA_DM_PM_SNIFF_HH_IDLE_IDX BTA_DM_PM_SNIFF4
#define BTA_DM_PM_HH_IDLE_DELAY 0

// Change I/O capabilities to output only so pairing uses passkey instead of pin
#define BTM_LOCAL_IO_CAPS BTM_IO_CAP_OUT

#endif
