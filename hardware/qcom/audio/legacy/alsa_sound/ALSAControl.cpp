/* ALSAControl.cpp
 **
 ** Copyright 2008-2009 Wind River Systems
 ** Copyright (c) 2011-2012, Code Aurora Forum. All rights reserved.
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

#include <errno.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>

#define LOG_TAG "ALSAControl"
//#define LOG_NDEBUG 0
#define LOG_NDDEBUG 0
#include <utils/Log.h>
#include <utils/String8.h>

#include <cutils/properties.h>
#include <media/AudioRecord.h>
#include <hardware_legacy/power.h>

#include "AudioHardwareALSA.h"

namespace android_audio_legacy
{

ALSAControl::ALSAControl(const char *device)
{
    ALOGD("ALSAControl: ctor device %s", device);
    mHandle = mixer_open(device);
    ALOGV("ALSAControl: ctor mixer %p", mHandle);
}

ALSAControl::~ALSAControl()
{
    if (mHandle) mixer_close(mHandle);
}

status_t ALSAControl::get(const char *name, unsigned int &value, int index)
{
    struct mixer_ctl *ctl;

    if (!mHandle) {
        ALOGE("Control not initialized");
        return NO_INIT;
    }

    ctl =  mixer_get_control(mHandle, name, index);
    if (!ctl)
        return BAD_VALUE;

    mixer_ctl_get(ctl, &value);
    return NO_ERROR;
}

status_t ALSAControl::set(const char *name, unsigned int value, int index)
{
    struct mixer_ctl *ctl;
    int ret = 0;
    ALOGD("set:: name %s value %d index %d", name, value, index);
    if (!mHandle) {
        ALOGE("Control not initialized");
        return NO_INIT;
    }

    // ToDo: Do we need to send index here? Right now it works with 0
    ctl = mixer_get_control(mHandle, name, 0);
    if(ctl == NULL) {
        ALOGE("Could not get the mixer control");
        return BAD_VALUE;
    }
    ret = mixer_ctl_set(ctl, value);
    return (ret < 0) ? BAD_VALUE : NO_ERROR;
}

status_t ALSAControl::set(const char *name, const char *value)
{
    struct mixer_ctl *ctl;
    int ret = 0;
    ALOGD("set:: name %s value %s", name, value);

    if (!mHandle) {
        ALOGE("Control not initialized");
        return NO_INIT;
    }

    ctl = mixer_get_control(mHandle, name, 0);
    if(ctl == NULL) {
        ALOGE("Could not get the mixer control");
        return BAD_VALUE;
    }
    ret = mixer_ctl_select(ctl, value);
    return (ret < 0) ? BAD_VALUE : NO_ERROR;
}

status_t ALSAControl::setext(const char *name, int count, char **setValues)
{
    struct mixer_ctl *ctl;
    int ret = 0;
    ALOGD("setext:: name %s count %d", name, count);
    if (!mHandle) {
        ALOGE("Control not initialized");
        return NO_INIT;
    }

    // ToDo: Do we need to send index here? Right now it works with 0
    ctl = mixer_get_control(mHandle, name, 0);
    if(ctl == NULL) {
        ALOGE("Could not get the mixer control");
        return BAD_VALUE;
    }
    ret = mixer_ctl_set_value(ctl, count, setValues);
    return (ret < 0) ? BAD_VALUE : NO_ERROR;
}

};        // namespace android
