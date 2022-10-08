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

#define LOG_TAG "ext_speaker"
/*#define LOG_NDEBUG 0*/
#include <cutils/log.h>

#include <audio_hw.h>
#include <dlfcn.h>

#ifdef __LP64__
        #define LIB_SPEAKER_BUNDLE "/system/lib64/soundfx/libspeakerbundle.so"
#else
        #define LIB_SPEAKER_BUNDLE "/system/lib/soundfx/libspeakerbundle.so"
#endif

typedef void (*set_mode_t)(int);
typedef void (*set_speaker_on_t)(bool);
typedef void (*set_earpiece_on_t)(bool);
typedef void (*set_voice_vol_t)(float);

struct speaker_data {
    struct audio_device *adev;
    void *speaker_bundle;
    set_mode_t set_mode;
    set_speaker_on_t set_speaker_on;
    set_earpiece_on_t set_earpiece_on;
    set_voice_vol_t set_voice_vol;
};

static struct speaker_data* open_speaker_bundle()
{
    struct speaker_data *sd = calloc(1, sizeof(struct speaker_data));

    sd->speaker_bundle = dlopen(LIB_SPEAKER_BUNDLE, RTLD_NOW);
    if (sd->speaker_bundle == NULL) {
        ALOGE("%s: DLOPEN failed for %s", __func__, LIB_SPEAKER_BUNDLE);
        goto error;
    } else {
        ALOGV("%s: DLOPEN successful for %s", __func__, LIB_SPEAKER_BUNDLE);

        sd->set_mode = (set_mode_t)dlsym(sd->speaker_bundle,
                                             "set_mode");
        if (sd->set_mode == NULL) {
            ALOGE("%s: dlsym error %s for set_mode", __func__,
                  dlerror());
            goto error;
        }
        sd->set_speaker_on = (set_speaker_on_t)dlsym(sd->speaker_bundle,
                                             "set_speaker_on");
        if (sd->set_speaker_on == NULL) {
            ALOGE("%s: dlsym error %s for set_speaker_on", __func__,
                  dlerror());
            goto error;
        }
        sd->set_earpiece_on = (set_earpiece_on_t)dlsym(sd->speaker_bundle,
                                             "set_earpiece_on");
        if (sd->set_earpiece_on == NULL) {
            ALOGE("%s: dlsym error %s for set_earpiece_on", __func__,
                  dlerror());
            goto error;
        }
        sd->set_voice_vol = (set_voice_vol_t)dlsym(sd->speaker_bundle,
                                             "set_voice_volume");
        if (sd->set_voice_vol == NULL) {
            ALOGE("%s: dlsym error %s for set_voice_volume",
                  __func__, dlerror());
            goto error;
        }
    }
    return sd;

error:
    free(sd);
    return 0;
}

static void close_speaker_bundle(struct speaker_data *sd)
{
    if (sd != NULL) {
        dlclose(sd->speaker_bundle);
        free(sd);
        sd = NULL;
    }
}

void *audio_extn_extspk_init(struct audio_device *adev)
{
    struct speaker_data *data = open_speaker_bundle();

    if (data)
        data->adev = adev;

    return data;
}

void audio_extn_extspk_deinit(void *extn)
{
    struct speaker_data *data = (struct speaker_data*)extn;
    close_speaker_bundle(data);
}

void audio_extn_extspk_update(void* extn)
{
    struct speaker_data *data = (struct speaker_data*)extn;

    if (data) {
        bool speaker_on = false;
        bool earpiece_on = false;
        struct listnode *node;
        struct audio_usecase *usecase;
        list_for_each(node, &data->adev->usecase_list) {
            usecase = node_to_item(node, struct audio_usecase, list);
            if (usecase->devices & AUDIO_DEVICE_OUT_EARPIECE) {
                if(data->adev->snd_dev_ref_cnt[usecase->out_snd_device] != 0) {
                    earpiece_on = true;
                }
            }
            if (usecase->devices & AUDIO_DEVICE_OUT_SPEAKER) {
                if(data->adev->snd_dev_ref_cnt[usecase->out_snd_device] != 0) {
                    speaker_on = true;
                }
            }
        }
        data->set_earpiece_on(earpiece_on);
        data->set_speaker_on(speaker_on);
    }
}

void audio_extn_extspk_set_mode(void* extn, audio_mode_t mode)
{
    struct speaker_data *data = (struct speaker_data*)extn;

    if (data)
        data->set_mode(mode);
}

void audio_extn_extspk_set_voice_vol(void* extn, float vol)
{
    struct speaker_data *data = (struct speaker_data*)extn;

    if (data)
        data->set_voice_vol(vol);
}
