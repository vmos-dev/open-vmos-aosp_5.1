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

#ifndef __DRM_ERROR_H__
#define __DRM_ERROR_H__


enum
{
    DRM_SUCCESSFUL  = 0,

    /*! DRM middleware and firmware errors  */
    DRM_ERR_BASE                            = 0x50000000,
    DRM_FAIL_INVALID_PARAM,
    DRM_FAIL_NULL_PARAM,
    DRM_FAIL_BUFFER_TOO_SMALL,
    DRM_FAIL_UNSUPPORTED,
    DRM_FAIL_INVALID_IMR_SIZE,
    DRM_FAIL_INVALID_SESSION,
    DRM_FAIL_FW_SESSION,
    DRM_FAIL_AUDIO_DATA_NOT_VALID,
    DRM_FAIL_INVALID_TEE_PARAM,
    DRM_FAIL_KEYBOX_INVALID_BAD_MAGIC,
    DRM_FAIL_KEYBOX_INVALID_BAD_CRC,
    DRM_FAIL_KEYBOX_INVALID_BAD_PROVISIONING,
    DRM_FAIL_KEYBOX_INVALID_NOT_PROVISIONED,
    DRM_FAIL_INPUT_BUFFER_TOO_SHORT,
    DRM_FAIL_AES_DECRYPT,                                     // 0x5000000F
    DRM_FAIL_AES_FAILURE,
    DRM_FAIL_WV_NO_ASSET_KEY,
    DRM_FAIL_WV_NO_CEK,
    DRM_FAIL_SCHEDULER,
    DRM_FAIL_SESSION_NOT_INITIALIZED,
    DRM_FAIL_REPROVISION_IED_KEY,
    DRM_FAIL_NALU_DATA_EXCEEDS_BUFFER_SIZE,
    DRM_FAIL_WV_SESSION_NALU_PARSE_FAILURE,
    DRM_FAIL_SESSION_KEY_GEN,
    DRM_FAIL_INVALID_PLATFORM_ID,
    DRM_FAIL_INVALID_MAJOR_VERSION,
    DRM_FAIL_NO_PREV_PARTIAL_BLOCK,
    DRM_FAIL_WRITE_KEYBOX,
    DRM_FAIL_INSUFFICENT_RESOURCES,

    /*! Middleware specific errors */
    DRM_FAIL_GENERATE_RANDOM_NUMBER_FAILURE = 0x50001000,
    DRM_FAIL_DX_CCLIB_INIT_FAILURE,
    DRM_FAILURE                             = 0x5FFFFFFF
};


#endif // drm_error.h
