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

#define LOG_TAG "drm_vendor_api"

#include <log/log.h>
#include <dlfcn.h>
#include <string.h>

#include "drm_vendor_api.h"

#define LIB_DRM_VENDOR "libsepdrm_cc54.so"

uint32_t drm_vendor_api_init(struct drm_vendor_api *api)
{
    uint32_t got_error = 0;

    if (api == NULL) {
        ALOGE("%s: NULL parameter provided", __func__);
        return 1;
    }

    api->handle = dlopen(LIB_DRM_VENDOR, RTLD_NOW);
    if (api->handle == NULL) {
        ALOGE("%s: couldn't dlopen %s : %s", __func__, LIB_DRM_VENDOR, dlerror());
        drm_vendor_api_deinit(api);
        return 1;
    } else {
        api->drm_keep_alive = (drm_keep_alive_t) dlsym(api->handle, "drm_keep_alive");
        if (api->drm_keep_alive == NULL) {
            ALOGE("%s: couldn't dlsym drm_keep_alive in %s : %s",
                    __func__, LIB_DRM_VENDOR, dlerror());
            got_error = 1;
        }
        api->drm_start_playback = (drm_start_playback_t) dlsym(api->handle, "drm_start_playback");
        if (api->drm_start_playback == NULL) {
            ALOGE("%s: couldn't dlsym drm_start_playback in %s : %s",
                    __func__, LIB_DRM_VENDOR, dlerror());
            got_error = 1;
        }
        api->drm_stop_playback = (drm_stop_playback_t) dlsym(api->handle, "drm_stop_playback");
        if (api->drm_stop_playback == NULL) {
            ALOGE("%s: couldn't dlsym drm_stop_playback in %s : %s",
                    __func__, LIB_DRM_VENDOR, dlerror());
            got_error = 1;
        }
        api->drm_pr_return_naluheaders = (drm_pr_return_naluheaders_t) dlsym(api->handle,
                "drm_pr_return_naluheaders");
        if (api->drm_pr_return_naluheaders == NULL) {
            ALOGE("%s: couldn't dlsym drm_pr_return_naluheaders in %s : %s",
                    __func__, LIB_DRM_VENDOR, dlerror());
            got_error = 1;
        }
        api->drm_wv_return_naluheaders =
                (drm_wv_return_naluheaders_t) dlsym(api->handle, "drm_wv_return_naluheaders");
        if (api->drm_wv_return_naluheaders == NULL) {
            ALOGE("%s: couldn't dlsym drm_wv_return_naluheaders in %s : %s",
                    __func__, LIB_DRM_VENDOR, dlerror());
            got_error = 1;
        }
        if (got_error) {
            drm_vendor_api_deinit(api);
            return 1;
        }
    }
    return 0;
}

uint32_t drm_vendor_api_deinit(struct drm_vendor_api *api)
{
    if (api == NULL) {
        ALOGE("%s: NULL parameter provided", __func__);
        return 1;
    }
    if(api->handle) {
        if (dlclose(api->handle)) {
            ALOGE("%s: couldn't dlcose %s : %s", __func__, LIB_DRM_VENDOR, dlerror());
            return 1;
        }
    }
    memset(api, 0, sizeof(struct drm_vendor_api));
    return 0;
}
