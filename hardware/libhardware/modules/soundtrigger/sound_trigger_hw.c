/*
 * Copyright (C) 2011 The Android Open Source Project
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

#define LOG_TAG "sound_trigger_hw_default"
/*#define LOG_NDEBUG 0*/

#include <errno.h>
#include <pthread.h>
#include <sys/prctl.h>
#include <cutils/log.h>

#include <hardware/hardware.h>
#include <system/sound_trigger.h>
#include <hardware/sound_trigger.h>

static const struct sound_trigger_properties hw_properties = {
        "The Android Open Source Project", // implementor
        "Sound Trigger stub HAL", // description
        1, // version
        { 0xed7a7d60, 0xc65e, 0x11e3, 0x9be4, { 0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b } }, // uuid
        1, // max_sound_models
        1, // max_key_phrases
        1, // max_users
        RECOGNITION_MODE_VOICE_TRIGGER, // recognition_modes
        false, // capture_transition
        0, // max_buffer_ms
        false, // concurrent_capture
        false, // trigger_in_event
        0 // power_consumption_mw
};

struct stub_sound_trigger_device {
    struct sound_trigger_hw_device device;
    sound_model_handle_t model_handle;
    recognition_callback_t recognition_callback;
    void *recognition_cookie;
    sound_model_callback_t sound_model_callback;
    void *sound_model_cookie;
    pthread_t callback_thread;
    pthread_mutex_t lock;
    pthread_cond_t  cond;
};


static void *callback_thread_loop(void *context)
{
    struct stub_sound_trigger_device *stdev = (struct stub_sound_trigger_device *)context;
    ALOGI("%s", __func__);

    prctl(PR_SET_NAME, (unsigned long)"sound trigger callback", 0, 0, 0);

    pthread_mutex_lock(&stdev->lock);
    if (stdev->recognition_callback == NULL) {
        goto exit;
    }
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += 3;
    ALOGI("%s wait 3 sec", __func__);
    int rc = pthread_cond_timedwait(&stdev->cond, &stdev->lock, &ts);
    if (rc == ETIMEDOUT && stdev->recognition_callback != NULL) {
        char *data = (char *)calloc(1, sizeof(struct sound_trigger_phrase_recognition_event) + 1);
        struct sound_trigger_phrase_recognition_event *event =
                (struct sound_trigger_phrase_recognition_event *)data;
        event->common.status = RECOGNITION_STATUS_SUCCESS;
        event->common.type = SOUND_MODEL_TYPE_KEYPHRASE;
        event->common.model = stdev->model_handle;
        event->num_phrases = 1;
        event->phrase_extras[0].recognition_modes = RECOGNITION_MODE_VOICE_TRIGGER;
        event->phrase_extras[0].confidence_level = 100;
        event->phrase_extras[0].num_levels = 1;
        event->phrase_extras[0].levels[0].level = 100;
        event->phrase_extras[0].levels[0].user_id = 0;
        event->common.data_offset = sizeof(struct sound_trigger_phrase_recognition_event);
        event->common.data_size = 1;
        data[event->common.data_offset] = 8;
        ALOGI("%s send callback model %d", __func__, stdev->model_handle);
        stdev->recognition_callback(&event->common, stdev->recognition_cookie);
        free(data);
    } else {
        ALOGI("%s abort recognition model %d", __func__, stdev->model_handle);
    }
    stdev->recognition_callback = NULL;

exit:
    pthread_mutex_unlock(&stdev->lock);

    return NULL;
}

static int stdev_get_properties(const struct sound_trigger_hw_device *dev,
                                struct sound_trigger_properties *properties)
{
    struct stub_sound_trigger_device *stdev = (struct stub_sound_trigger_device *)dev;

    ALOGI("%s", __func__);
    if (properties == NULL)
        return -EINVAL;
    memcpy(properties, &hw_properties, sizeof(struct sound_trigger_properties));
    return 0;
}

static int stdev_load_sound_model(const struct sound_trigger_hw_device *dev,
                                  struct sound_trigger_sound_model *sound_model,
                                  sound_model_callback_t callback,
                                  void *cookie,
                                  sound_model_handle_t *handle)
{
    struct stub_sound_trigger_device *stdev = (struct stub_sound_trigger_device *)dev;
    int status = 0;

    ALOGI("%s stdev %p", __func__, stdev);
    pthread_mutex_lock(&stdev->lock);
    if (handle == NULL || sound_model == NULL) {
        status = -EINVAL;
        goto exit;
    }
    if (sound_model->data_size == 0 ||
            sound_model->data_offset < sizeof(struct sound_trigger_sound_model)) {
        status = -EINVAL;
        goto exit;
    }

    if (stdev->model_handle == 1) {
        status = -ENOSYS;
        goto exit;
    }
    char *data = (char *)sound_model + sound_model->data_offset;
    ALOGI("%s data size %d data %d - %d", __func__,
          sound_model->data_size, data[0], data[sound_model->data_size - 1]);
    stdev->model_handle = 1;
    stdev->sound_model_callback = callback;
    stdev->sound_model_cookie = cookie;

    *handle = stdev->model_handle;

exit:
    pthread_mutex_unlock(&stdev->lock);
    return status;
}

static int stdev_unload_sound_model(const struct sound_trigger_hw_device *dev,
                                    sound_model_handle_t handle)
{
    struct stub_sound_trigger_device *stdev = (struct stub_sound_trigger_device *)dev;
    int status = 0;

    ALOGI("%s handle %d", __func__, handle);
    pthread_mutex_lock(&stdev->lock);
    if (handle != 1) {
        status = -EINVAL;
        goto exit;
    }
    if (stdev->model_handle == 0) {
        status = -ENOSYS;
        goto exit;
    }
    stdev->model_handle = 0;
    if (stdev->recognition_callback != NULL) {
        stdev->recognition_callback = NULL;
        pthread_cond_signal(&stdev->cond);
        pthread_mutex_unlock(&stdev->lock);
        pthread_join(stdev->callback_thread, (void **) NULL);
        pthread_mutex_lock(&stdev->lock);
    }

exit:
    pthread_mutex_unlock(&stdev->lock);
    return status;
}

static int stdev_start_recognition(const struct sound_trigger_hw_device *dev,
                                   sound_model_handle_t sound_model_handle,
                                   const struct sound_trigger_recognition_config *config,
                                   recognition_callback_t callback,
                                   void *cookie)
{
    struct stub_sound_trigger_device *stdev = (struct stub_sound_trigger_device *)dev;
    int status = 0;
    ALOGI("%s sound model %d", __func__, sound_model_handle);
    pthread_mutex_lock(&stdev->lock);
    if (stdev->model_handle != sound_model_handle) {
        status = -ENOSYS;
        goto exit;
    }
    if (stdev->recognition_callback != NULL) {
        status = -ENOSYS;
        goto exit;
    }
    if (config->data_size != 0) {
        char *data = (char *)config + config->data_offset;
        ALOGI("%s data size %d data %d - %d", __func__,
              config->data_size, data[0], data[config->data_size - 1]);
    }

    stdev->recognition_callback = callback;
    stdev->recognition_cookie = cookie;
    pthread_create(&stdev->callback_thread, (const pthread_attr_t *) NULL,
                        callback_thread_loop, stdev);
exit:
    pthread_mutex_unlock(&stdev->lock);
    return status;
}

static int stdev_stop_recognition(const struct sound_trigger_hw_device *dev,
                                 sound_model_handle_t sound_model_handle)
{
    struct stub_sound_trigger_device *stdev = (struct stub_sound_trigger_device *)dev;
    int status = 0;
    ALOGI("%s sound model %d", __func__, sound_model_handle);
    pthread_mutex_lock(&stdev->lock);
    if (stdev->model_handle != sound_model_handle) {
        status = -ENOSYS;
        goto exit;
    }
    if (stdev->recognition_callback == NULL) {
        status = -ENOSYS;
        goto exit;
    }
    stdev->recognition_callback = NULL;
    pthread_cond_signal(&stdev->cond);
    pthread_mutex_unlock(&stdev->lock);
    pthread_join(stdev->callback_thread, (void **) NULL);
    pthread_mutex_lock(&stdev->lock);

exit:
    pthread_mutex_unlock(&stdev->lock);
    return status;
}


static int stdev_close(hw_device_t *device)
{
    free(device);
    return 0;
}

static int stdev_open(const hw_module_t* module, const char* name,
                     hw_device_t** device)
{
    struct stub_sound_trigger_device *stdev;
    int ret;

    if (strcmp(name, SOUND_TRIGGER_HARDWARE_INTERFACE) != 0)
        return -EINVAL;

    stdev = calloc(1, sizeof(struct stub_sound_trigger_device));
    if (!stdev)
        return -ENOMEM;

    stdev->device.common.tag = HARDWARE_DEVICE_TAG;
    stdev->device.common.version = SOUND_TRIGGER_DEVICE_API_VERSION_1_0;
    stdev->device.common.module = (struct hw_module_t *) module;
    stdev->device.common.close = stdev_close;
    stdev->device.get_properties = stdev_get_properties;
    stdev->device.load_sound_model = stdev_load_sound_model;
    stdev->device.unload_sound_model = stdev_unload_sound_model;
    stdev->device.start_recognition = stdev_start_recognition;
    stdev->device.stop_recognition = stdev_stop_recognition;

    pthread_mutex_init(&stdev->lock, (const pthread_mutexattr_t *) NULL);
    pthread_cond_init(&stdev->cond, (const pthread_condattr_t *) NULL);

    *device = &stdev->device.common;

    return 0;
}

static struct hw_module_methods_t hal_module_methods = {
    .open = stdev_open,
};

struct sound_trigger_module HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .module_api_version = SOUND_TRIGGER_MODULE_API_VERSION_1_0,
        .hal_api_version = HARDWARE_HAL_API_VERSION,
        .id = SOUND_TRIGGER_HARDWARE_MODULE_ID,
        .name = "Default sound trigger HAL",
        .author = "The Android Open Source Project",
        .methods = &hal_module_methods,
    },
};
