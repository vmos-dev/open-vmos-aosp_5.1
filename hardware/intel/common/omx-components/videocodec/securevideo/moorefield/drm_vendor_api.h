/*
 * Copyright (c) 2014 Intel Corporation.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __DRM_VENDOR_API__
#define __DRM_VENDOR_API__

#include <sepdrm.h>
#include <pr_drm_api.h>
#include <fcntl.h>
#include <linux/psb_drm.h>
#include "xf86drm.h"
#include "xf86drmMode.h"

typedef uint32_t (*drm_keep_alive_t)(uint32_t session_id, uint32_t *timeout);
typedef uint32_t (*drm_start_playback_t)(void);
typedef uint32_t (*drm_stop_playback_t)(void);
typedef uint32_t (*drm_pr_return_naluheaders_t)(uint32_t session_id, struct drm_nalu_headers *nalu_info);
typedef uint32_t (*drm_wv_return_naluheaders_t)(uint32_t session_id, struct drm_wv_nalu_headers *nalu_info);

struct drm_vendor_api {
    void *handle;
    drm_keep_alive_t drm_keep_alive;
    drm_start_playback_t drm_start_playback;
    drm_stop_playback_t drm_stop_playback;
    drm_pr_return_naluheaders_t drm_pr_return_naluheaders;
    drm_wv_return_naluheaders_t drm_wv_return_naluheaders;
};

/* Initialize the function pointers structure opening the vendor library
   Returns 0 on case of success, 1 in case of error */
uint32_t drm_vendor_api_init(struct drm_vendor_api *api);

/* Close the function pointers structure
   Returns 0 on case of success, 1 in case of error */
uint32_t drm_vendor_api_deinit(struct drm_vendor_api *api);

#endif  /* #ifndef __DRM_VENDOR_API__ */
