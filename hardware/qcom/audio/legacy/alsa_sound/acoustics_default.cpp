/* acoustics_default.cpp
 **
 ** Copyright 2009 Wind River Systems
 **
 ** Licensed under the Apache License, Version 2.0 (the "License");
 ** you may not use this file except in compliance with the License.
 ** You may obtain a copy of the License at
 **
 **     http://www.apache.org/licenses/LICENSE-2.0
 **
 ** Unless required by applicable law or agreed to in writing, software
 ** distributed under the License is distributed on an "AS IS" BASIS,
 ** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 ** See the License for the specific language governing permissions and
 ** limitations under the License.
 */

#define LOG_TAG "AcousticsModule"
#include <utils/Log.h>

#include "AudioHardwareALSA.h"

namespace android
{

static int s_device_open(const hw_module_t*, const char*, hw_device_t**);
static int s_device_close(hw_device_t*);

static status_t s_use_handle(acoustic_device_t *, alsa_handle_t *);
static status_t s_cleanup(acoustic_device_t *);
static status_t s_set_params(acoustic_device_t *,
        AudioSystem::audio_in_acoustics, void *params);

static hw_module_methods_t s_module_methods = {
    open            : s_device_open
};

extern "C" hw_module_t HAL_MODULE_INFO_SYM = {
    tag             : HARDWARE_MODULE_TAG,
    version_major   : 1,
    version_minor   : 0,
    id              : ACOUSTICS_HARDWARE_MODULE_ID,
    name            : "ALSA acoustics module",
    author          : "Wind River",
    methods         : &s_module_methods,
    dso             : 0,
    reserved        : { 0, },
};

static int s_device_open(const hw_module_t* module, const char* name,
        hw_device_t** device)
{
    acoustic_device_t *dev;
    dev = (acoustic_device_t *) malloc(sizeof(*dev));
    if (!dev) return -ENOMEM;

    memset(dev, 0, sizeof(*dev));

    /* initialize the procs */
    dev->common.tag = HARDWARE_DEVICE_TAG;
    dev->common.version = 0;
    dev->common.module = (hw_module_t *) module;
    dev->common.close = s_device_close;

    // Required methods...
    dev->use_handle = s_use_handle;
    dev->cleanup = s_cleanup;
    dev->set_params = s_set_params;

    // read, write, and recover are optional methods...

    *device = &dev->common;
    return 0;
}

static int s_device_close(hw_device_t* device)
{
    free(device);
    return 0;
}

static status_t s_use_handle(acoustic_device_t *dev, alsa_handle_t *h)
{
    return NO_ERROR;
}

static status_t s_cleanup(acoustic_device_t *dev)
{
    ALOGD("Acoustics close stub called.");
    return NO_ERROR;
}

static status_t s_set_params(acoustic_device_t *dev,
        AudioSystem::audio_in_acoustics acoustics, void *params)
{
    ALOGD("Acoustics set_params stub called with %d.", (int)acoustics);
    return NO_ERROR;
}
}
