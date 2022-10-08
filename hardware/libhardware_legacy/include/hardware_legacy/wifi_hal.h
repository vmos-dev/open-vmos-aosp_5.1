/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __WIFI_HAL_H__
#define __WIFI_HAL_H__

#include <stdint.h>

typedef enum {
    WIFI_SUCCESS = 0,
    WIFI_ERROR_NONE = 0,
    WIFI_ERROR_UNKNOWN = -1,
    WIFI_ERROR_UNINITIALIZED = -2,
    WIFI_ERROR_NOT_SUPPORTED = -3,
    WIFI_ERROR_NOT_AVAILABLE = -4,              // Not available right now, but try later
    WIFI_ERROR_INVALID_ARGS = -5,
    WIFI_ERROR_INVALID_REQUEST_ID = -6,
    WIFI_ERROR_TIMED_OUT = -7,
    WIFI_ERROR_TOO_MANY_REQUESTS = -8,          // Too many instances of this request
    WIFI_ERROR_OUT_OF_MEMORY = -9
} wifi_error;

typedef unsigned char byte;
typedef unsigned char u8;
typedef signed char s8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int wifi_request_id;
typedef int wifi_channel;                       // indicates channel frequency in MHz
typedef int wifi_rssi;
typedef byte mac_addr[6];
typedef byte oui[3];
typedef int64_t wifi_timestamp;                 // In microseconds (us)
typedef int64_t wifi_timespan;                  // In nanoseconds  (ns)

struct wifi_info;
typedef wifi_info *wifi_handle;
struct wifi_interface_info;
typedef wifi_interface_info *wifi_interface_handle;

/* Initialize/Cleanup */

wifi_error wifi_initialize(wifi_handle *handle);
typedef void (*wifi_cleaned_up_handler) (wifi_handle handle);
void wifi_cleanup(wifi_handle handle, wifi_cleaned_up_handler handler);
void wifi_event_loop(wifi_handle handle);

/* Error handling */
void wifi_get_error_info(wifi_error err, const char **msg); // return a pointer to a static string

/* Feature enums */
#define WIFI_FEATURE_INFRA              0x0001      // Basic infrastructure mode
#define WIFI_FEATURE_INFRA_5G           0x0002      // Support for 5 GHz Band
#define WIFI_FEATURE_HOTSPOT            0x0004      // Support for GAS/ANQP
#define WIFI_FEATURE_P2P                0x0008      // Wifi-Direct
#define WIFI_FEATURE_SOFT_AP            0x0010      // Soft AP
#define WIFI_FEATURE_GSCAN              0x0020      // Google-Scan APIs
#define WIFI_FEATURE_NAN                0x0040      // Neighbor Awareness Networking
#define WIFI_FEATURE_D2D_RTT            0x0080      // Device-to-device RTT
#define WIFI_FEATURE_D2AP_RTT           0x0100      // Device-to-AP RTT
#define WIFI_FEATURE_BATCH_SCAN         0x0200      // Batched Scan (legacy)
#define WIFI_FEATURE_PNO                0x0400      // Preferred network offload
#define WIFI_FEATURE_ADDITIONAL_STA     0x0800      // Support for two STAs
#define WIFI_FEATURE_TDLS               0x1000      // Tunnel directed link setup
#define WIFI_FEATURE_TDLS_OFFCHANNEL    0x2000      // Support for TDLS off channel
#define WIFI_FEATURE_EPR                0x4000      // Enhanced power reporting
#define WIFI_FEATURE_AP_STA             0x8000      // Support for AP STA Concurrency
#define WIFI_FEATURE_LINK_LAYER_STATS  0x10000      // Link layer stats collection
// Add more features here

typedef int feature_set;

#define IS_MASK_SET(mask, flags)        ((flags & mask) == mask)
#define IS_MASK_RESET(mask, flags)      ((flags & mask) == 0)

#define IS_SUPPORTED_FEATURE(feature, featureSet)       IS_MASK_SET(feature, fetureSet)
#define IS_UNSUPPORTED_FEATURE(feature, featureSet)     IS_MASK_RESET(feature, fetureSet)

/* Feature set */
wifi_error wifi_get_supported_feature_set(wifi_interface_handle handle, feature_set *set);

/*
 * Each row represents a valid feature combination;
 * all other combinations are invalid!
 */
wifi_error wifi_get_concurrency_matrix(wifi_interface_handle handle, int set_size_max,
       feature_set set[], int *set_size);

/* multiple interface support */

wifi_error wifi_get_ifaces(wifi_handle handle, int *num_ifaces, wifi_interface_handle **ifaces);
wifi_error wifi_get_iface_name(wifi_interface_handle iface, char *name, size_t size);

/* Configuration events */

typedef struct {
    void (*on_country_code_changed)(char code[2]);      // We can get this from supplicant too

    // More event handlers
} wifi_event_handler;

wifi_error wifi_set_iface_event_handler(wifi_request_id id, wifi_interface_handle iface, wifi_event_handler eh);
wifi_error wifi_reset_iface_event_handler(wifi_request_id id, wifi_interface_handle iface);

wifi_error wifi_set_nodfs_flag(wifi_interface_handle handle, u32 nodfs);

/* include various feature headers */

#include "gscan.h"
#include "link_layer_stats.h"
#include "rtt.h"
#include "tdls.h"

#endif

