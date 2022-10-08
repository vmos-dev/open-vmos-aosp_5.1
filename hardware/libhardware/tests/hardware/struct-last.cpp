/*
 * Copyright (C) 2013 The Android Open Source Project
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

#include <cstddef>
#include <system/window.h>
#include <hardware/hardware.h>
#include <hardware/sensors.h>
#include <hardware/fb.h>
#include <hardware/hwcomposer.h>
#include <hardware/gralloc.h>
#include <hardware/consumerir.h>
#include <hardware/camera_common.h>
#include <hardware/camera3.h>

#define GET_PADDING(align, size) (((align) - ((size) % (align))) % (align))

#define CHECK_LAST_MEMBER(type, member) \
do { \
static constexpr size_t calc_size = offsetof(type, member) + sizeof(((type *)0)->member); \
static_assert(sizeof(type) == calc_size + GET_PADDING(alignof(type), calc_size), \
"" #member " is not the last element of " #type); \
} while (0)

void CheckSizes(void) {
    //Types defined in hardware.h
    CHECK_LAST_MEMBER(hw_module_t, reserved);
    CHECK_LAST_MEMBER(hw_device_t, close);

    //Types defined in sensors.h
    CHECK_LAST_MEMBER(sensors_vec_t, reserved);
    CHECK_LAST_MEMBER(sensors_event_t, reserved1);
    CHECK_LAST_MEMBER(struct sensor_t, reserved);
    CHECK_LAST_MEMBER(sensors_poll_device_1_t, reserved_procs);

    //Types defined in fb.h
    CHECK_LAST_MEMBER(framebuffer_device_t, reserved_proc);

    //Types defined in hwcomposer.h
    CHECK_LAST_MEMBER(hwc_layer_1_t, reserved);
    CHECK_LAST_MEMBER(hwc_composer_device_1_t, reserved_proc);

    //Types defined in gralloc.h
    CHECK_LAST_MEMBER(gralloc_module_t, reserved_proc);
    CHECK_LAST_MEMBER(alloc_device_t, reserved_proc);

    //Types defined in consumerir.h
    CHECK_LAST_MEMBER(consumerir_device_t, reserved);

    //Types defined in camera_common.h
    CHECK_LAST_MEMBER(vendor_tag_ops_t, reserved);
    CHECK_LAST_MEMBER(camera_module_t, reserved);

    //Types defined in camera3.h
    CHECK_LAST_MEMBER(camera3_device_ops_t, reserved);
}

