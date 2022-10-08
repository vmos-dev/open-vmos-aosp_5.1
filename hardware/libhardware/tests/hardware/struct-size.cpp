/*
 * Copyright (C) 2012 The Android Open Source Project
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


#include <system/window.h>
#include <hardware/hardware.h>
#include <hardware/sensors.h>
#include <hardware/fb.h>
#include <hardware/hwcomposer.h>
#include <hardware/gralloc.h>
#include <hardware/consumerir.h>
#include <hardware/camera_common.h>
#include <hardware/camera3.h>

template<size_t> static constexpr size_t CheckSizeHelper(size_t, size_t);

template<> constexpr size_t CheckSizeHelper<4>(size_t size32, size_t size64) {
    return size32;
}

template<> constexpr size_t CheckSizeHelper<8>(size_t size32, size_t size64) {
    return size64;
}

template<typename T, size_t size32, size_t size64> static void CheckTypeSize() {
    const size_t mySize = CheckSizeHelper<sizeof(void *)>(size32, size64);

    static_assert(sizeof(T) == mySize, "struct is the wrong size");
}

void CheckSizes(void) {
    //Types defined in hardware.h
    CheckTypeSize<hw_module_t, 128, 248>();
    CheckTypeSize<hw_device_t, 64, 120>();

    //Types defined in sensors.h
    CheckTypeSize<sensors_vec_t, 16, 16>();
    CheckTypeSize<sensors_event_t, 104, 104>();
    CheckTypeSize<struct sensor_t, 68, 104>();
    CheckTypeSize<sensors_poll_device_1_t, 116, 224>();

    //Types defined in fb.h
    CheckTypeSize<framebuffer_device_t, 184, 288>();

    //Types defined in hwcomposer.h
    CheckTypeSize<hwc_layer_1_t, 96, 120>();
    CheckTypeSize<hwc_composer_device_1_t, 116, 224>();

    //Types defined in gralloc.h
    CheckTypeSize<gralloc_module_t, 176, 344>();
    CheckTypeSize<alloc_device_t, 104, 200>();

    //Types defined in consumerir.h
    CheckTypeSize<consumerir_device_t, 96, 184>();

    //Types defined in camera_common.h
    CheckTypeSize<vendor_tag_ops_t, 52, 104>();
    CheckTypeSize<camera_module_t, 176, 344>();

    //Types defined in camera3.h
    CheckTypeSize<camera3_device_ops_t, 64, 128>();
}

