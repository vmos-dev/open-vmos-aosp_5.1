/*
 * Copyright (C) 2014 Intel Corporation. All rights reserved.
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

#ifndef __DRM_COMMON_API_H__
#define __DRM_COMMON_API_H__

#include <inttypes.h>

/*
 * Maximum number of bytes for an audio or video data DMA.
 */
#define MAX_DMA_DATA_SIZE_IN_BYTES (4 * 1024 * 1024)

/*
 * The size of an AES Initialization Vector counter in bytes.
 */
#define AES_IV_COUNTER_SIZE_IN_BYTES 16

#define DRM_PLATCAP_IED         0x01
#define DRM_PLATCAP_IMR         0x02
#define DRM_PLATCAP_EPID        0x04
#define DRM_PLATCAP_HDCP        0x08

// Secure clock transaction ID (TID) size in bytes.
#define DRM_TID_SIZE 16

#define MAX_RNG_SIZE_IN_BYTES (4 * 1024)

//
// Secure clock time of day data type.
// day_of_month: starts with 1.
// month: 0 = January.
// year: Epoch is 70 (i.e., 1970). Maximum value is 138 (i.e., 2038).
//
struct time_of_day
{
    uint8_t sec;
    uint8_t min;
    uint8_t hour;
    uint8_t day_of_week;
    uint8_t day_of_month;
    uint8_t month;
    uint8_t year;
    uint8_t padding;
};


//
// Secure clock server response data type.
//
struct drm_secureclock_server_response
{
    uint8_t tid[DRM_TID_SIZE];
    struct time_of_day current_time;
    struct time_of_day refresh_time;
    uint8_t signature[256];
};


/*
 * DRM Schemes
 */
/*
   typedef enum {
        DRM_SCHEME_Netflix,
        DRM_SCHEME_Widevine,
        DRM_SCHEME_WidevineHLS,
   } drm_scheme_t;
 */


struct drm_platform_caps
{
    uint32_t imr_size;
    uint32_t reserved[15];
};


/*
 * DRM Library Initialization
 * Description:
 *  Initializes the security engine driver for DRM library use.
 */
uint32_t drm_library_init(void);

/*
 * @brief Writes random bytes into buffer
 */
uint32_t drm_get_random(
    uint8_t *rand_num_buf,
    uint32_t buf_size);

/*!
 * Create a DRM session
 */
uint32_t drm_create_session(
    uint32_t drm_scheme,
    uint32_t *sessionid_ptr);

/*!
 * Destroy the specified DRM session
 */
uint32_t drm_destroy_session(
    uint32_t session_id);


/*
 * Keeps an active DRM session from timing out
 */
uint32_t drm_keep_alive(
    uint32_t session_id,
    uint32_t *timeout);

/*
 * Query secure platform capabilities
 */
uint32_t drm_query_platformcapabilities(
    uint32_t *plat_cap,
    struct drm_platform_caps *cap_array);


/*
 * @brief Pauses the playback of a video decryption session.
 * @param session_id The ID number of the session to pause playback.
 * @return DRM_SUCCESSFUL The video decryption session was paused.
 */
uint32_t drm_playback_pause(
    uint32_t session_id);


/*
 * @brief Resumes the playback of a paused video decryption session.
 * @param session_id The ID number of the session to resume playback.
 * @return DRM_SUCCESSFUL The video decryption session was resumed.
 */
uint32_t drm_playback_resume(
    uint32_t session_id);


/*!
 * @brief Enables protected video path for DRM playback
 */
uint32_t drm_start_playback(void);


/*!
 * @brief - Disables protected video path for DRM
 */
uint32_t drm_stop_playback(void);

#endif
