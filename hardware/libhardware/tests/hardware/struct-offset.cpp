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

//Ideally this would print type.member instead we need to rely on the line number from the output
template <size_t actual, size_t expected> void check_member(void) {
    static_assert(actual == expected, "");
}

#ifdef __LP64__
#define CHECK_MEMBER_AT(type, member, off32, off64) \
    check_member<offsetof(type, member), off64>()
#else
#define CHECK_MEMBER_AT(type, member, off32, off64) \
    check_member<offsetof(type, member), off32>()
#endif

void CheckOffsets(void) {
    //Types defined in hardware.h
    CHECK_MEMBER_AT(hw_module_t, tag, 0, 0);
    CHECK_MEMBER_AT(hw_module_t, module_api_version, 4, 4);
    CHECK_MEMBER_AT(hw_module_t, hal_api_version, 6, 6);
    CHECK_MEMBER_AT(hw_module_t, id, 8, 8);
    CHECK_MEMBER_AT(hw_module_t, name, 12, 16);
    CHECK_MEMBER_AT(hw_module_t, author, 16, 24);
    CHECK_MEMBER_AT(hw_module_t, methods, 20, 32);
    CHECK_MEMBER_AT(hw_module_t, dso, 24, 40);
    CHECK_MEMBER_AT(hw_module_t, reserved, 28, 48);

    CHECK_MEMBER_AT(hw_device_t, tag, 0, 0);
    CHECK_MEMBER_AT(hw_device_t, version, 4, 4);
    CHECK_MEMBER_AT(hw_device_t, module, 8, 8);
    CHECK_MEMBER_AT(hw_device_t, reserved, 12, 16);
    CHECK_MEMBER_AT(hw_device_t, close, 60, 112);

    //Types defined in sensors.h
    CHECK_MEMBER_AT(sensors_vec_t, v, 0, 0);
    CHECK_MEMBER_AT(sensors_vec_t, x, 0, 0);
    CHECK_MEMBER_AT(sensors_vec_t, y, 4, 4);
    CHECK_MEMBER_AT(sensors_vec_t, z, 8, 8);
    CHECK_MEMBER_AT(sensors_vec_t, azimuth, 0, 0);
    CHECK_MEMBER_AT(sensors_vec_t, pitch, 4, 4);
    CHECK_MEMBER_AT(sensors_vec_t, roll, 8, 8);
    CHECK_MEMBER_AT(sensors_vec_t, status, 12, 12);
    CHECK_MEMBER_AT(sensors_vec_t, reserved, 13, 13);

    CHECK_MEMBER_AT(sensors_event_t, version, 0, 0);
    CHECK_MEMBER_AT(sensors_event_t, sensor, 4, 4);
    CHECK_MEMBER_AT(sensors_event_t, type, 8, 8);
    CHECK_MEMBER_AT(sensors_event_t, reserved0, 12, 12);
    CHECK_MEMBER_AT(sensors_event_t, timestamp, 16, 16);
    CHECK_MEMBER_AT(sensors_event_t, data, 24, 24);
    CHECK_MEMBER_AT(sensors_event_t, acceleration, 24, 24);
    CHECK_MEMBER_AT(sensors_event_t, magnetic, 24, 24);
    CHECK_MEMBER_AT(sensors_event_t, orientation, 24, 24);
    CHECK_MEMBER_AT(sensors_event_t, gyro, 24, 24);
    CHECK_MEMBER_AT(sensors_event_t, temperature, 24, 24);
    CHECK_MEMBER_AT(sensors_event_t, distance, 24, 24);
    CHECK_MEMBER_AT(sensors_event_t, light, 24, 24);
    CHECK_MEMBER_AT(sensors_event_t, pressure, 24, 24);
    CHECK_MEMBER_AT(sensors_event_t, relative_humidity, 24, 24);
    CHECK_MEMBER_AT(sensors_event_t, uncalibrated_gyro, 24, 24);
    CHECK_MEMBER_AT(sensors_event_t, uncalibrated_magnetic, 24, 24);
    CHECK_MEMBER_AT(sensors_event_t, meta_data, 24, 24);
    CHECK_MEMBER_AT(sensors_event_t, u64, 24, 24);
    CHECK_MEMBER_AT(sensors_event_t, u64.data, 24, 24);
    CHECK_MEMBER_AT(sensors_event_t, u64.step_counter, 24, 24);
    CHECK_MEMBER_AT(sensors_event_t, flags, 88, 88);
    CHECK_MEMBER_AT(sensors_event_t, reserved1, 92, 92);

    CHECK_MEMBER_AT(struct sensor_t, name, 0, 0);
    CHECK_MEMBER_AT(struct sensor_t, vendor, 4, 8);
    CHECK_MEMBER_AT(struct sensor_t, version, 8, 16);
    CHECK_MEMBER_AT(struct sensor_t, handle, 12, 20);
    CHECK_MEMBER_AT(struct sensor_t, type, 16, 24);
    CHECK_MEMBER_AT(struct sensor_t, maxRange, 20, 28);
    CHECK_MEMBER_AT(struct sensor_t, resolution, 24, 32);
    CHECK_MEMBER_AT(struct sensor_t, power, 28, 36);
    CHECK_MEMBER_AT(struct sensor_t, minDelay, 32, 40);
    CHECK_MEMBER_AT(struct sensor_t, fifoReservedEventCount, 36, 44);
    CHECK_MEMBER_AT(struct sensor_t, fifoMaxEventCount, 40, 48);
    CHECK_MEMBER_AT(struct sensor_t, stringType, 44, 56);
    CHECK_MEMBER_AT(struct sensor_t, requiredPermission, 48, 64);
    CHECK_MEMBER_AT(struct sensor_t, maxDelay, 52, 72);
    CHECK_MEMBER_AT(struct sensor_t, flags, 56, 80);
    CHECK_MEMBER_AT(struct sensor_t, reserved, 60, 88);

    CHECK_MEMBER_AT(sensors_poll_device_1_t, v0, 0, 0);
    CHECK_MEMBER_AT(sensors_poll_device_1_t, common, 0, 0);
    CHECK_MEMBER_AT(sensors_poll_device_1_t, activate, 64, 120);
    CHECK_MEMBER_AT(sensors_poll_device_1_t, setDelay, 68, 128);
    CHECK_MEMBER_AT(sensors_poll_device_1_t, poll, 72, 136);
    CHECK_MEMBER_AT(sensors_poll_device_1_t, batch, 76, 144);
    CHECK_MEMBER_AT(sensors_poll_device_1_t, flush, 80, 152);
    CHECK_MEMBER_AT(sensors_poll_device_1_t, reserved_procs, 84, 160);

    //Types defined in fb.h
    CHECK_MEMBER_AT(framebuffer_device_t, common, 0, 0);
    CHECK_MEMBER_AT(framebuffer_device_t, flags, 64, 120);
    CHECK_MEMBER_AT(framebuffer_device_t, width, 68, 124);
    CHECK_MEMBER_AT(framebuffer_device_t, height, 72, 128);
    CHECK_MEMBER_AT(framebuffer_device_t, stride, 76, 132);
    CHECK_MEMBER_AT(framebuffer_device_t, format, 80, 136);
    CHECK_MEMBER_AT(framebuffer_device_t, xdpi, 84, 140);
    CHECK_MEMBER_AT(framebuffer_device_t, ydpi, 88, 144);
    CHECK_MEMBER_AT(framebuffer_device_t, fps, 92, 148);
    CHECK_MEMBER_AT(framebuffer_device_t, minSwapInterval, 96, 152);
    CHECK_MEMBER_AT(framebuffer_device_t, maxSwapInterval, 100, 156);
    CHECK_MEMBER_AT(framebuffer_device_t, numFramebuffers, 104, 160);
    CHECK_MEMBER_AT(framebuffer_device_t, reserved, 108, 164);
    CHECK_MEMBER_AT(framebuffer_device_t, setSwapInterval, 136, 192);
    CHECK_MEMBER_AT(framebuffer_device_t, setUpdateRect, 140, 200);
    CHECK_MEMBER_AT(framebuffer_device_t, post, 144, 208);
    CHECK_MEMBER_AT(framebuffer_device_t, compositionComplete, 148, 216);
    CHECK_MEMBER_AT(framebuffer_device_t, dump, 152, 224);
    CHECK_MEMBER_AT(framebuffer_device_t, enableScreen, 156, 232);
    CHECK_MEMBER_AT(framebuffer_device_t, reserved_proc, 160, 240);

    //Types defined in hwcomposer.h
    CHECK_MEMBER_AT(hwc_layer_1_t, compositionType, 0, 0);
    CHECK_MEMBER_AT(hwc_layer_1_t, hints, 4, 4);
    CHECK_MEMBER_AT(hwc_layer_1_t, flags, 8, 8);
    CHECK_MEMBER_AT(hwc_layer_1_t, backgroundColor, 12, 16);
    CHECK_MEMBER_AT(hwc_layer_1_t, handle, 12, 16);
    CHECK_MEMBER_AT(hwc_layer_1_t, transform, 16, 24);
    CHECK_MEMBER_AT(hwc_layer_1_t, blending, 20, 28);
    CHECK_MEMBER_AT(hwc_layer_1_t, sourceCropi, 24, 32);
    CHECK_MEMBER_AT(hwc_layer_1_t, sourceCrop, 24, 32);
    CHECK_MEMBER_AT(hwc_layer_1_t, sourceCropf, 24, 32);
    CHECK_MEMBER_AT(hwc_layer_1_t, displayFrame, 40, 48);
    CHECK_MEMBER_AT(hwc_layer_1_t, visibleRegionScreen, 56, 64);
    CHECK_MEMBER_AT(hwc_layer_1_t, acquireFenceFd, 64, 80);
    CHECK_MEMBER_AT(hwc_layer_1_t, releaseFenceFd, 68, 84);
    CHECK_MEMBER_AT(hwc_layer_1_t, planeAlpha, 72, 88);
    CHECK_MEMBER_AT(hwc_layer_1_t, _pad, 73, 89);

    CHECK_MEMBER_AT(hwc_composer_device_1_t, common, 0, 0);
    CHECK_MEMBER_AT(hwc_composer_device_1_t, prepare, 64, 120);
    CHECK_MEMBER_AT(hwc_composer_device_1_t, set, 68, 128);
    CHECK_MEMBER_AT(hwc_composer_device_1_t, eventControl, 72, 136);
    CHECK_MEMBER_AT(hwc_composer_device_1_t, blank, 76, 144);
    CHECK_MEMBER_AT(hwc_composer_device_1_t, query, 80, 152);
    CHECK_MEMBER_AT(hwc_composer_device_1_t, registerProcs, 84, 160);
    CHECK_MEMBER_AT(hwc_composer_device_1_t, dump, 88, 168);
    CHECK_MEMBER_AT(hwc_composer_device_1_t, getDisplayConfigs, 92, 176);
    CHECK_MEMBER_AT(hwc_composer_device_1_t, getDisplayAttributes, 96, 184);
    CHECK_MEMBER_AT(hwc_composer_device_1_t, getActiveConfig, 100, 192);
    CHECK_MEMBER_AT(hwc_composer_device_1_t, setActiveConfig, 104, 200);
    CHECK_MEMBER_AT(hwc_composer_device_1_t, setCursorPositionAsync, 108, 208);
    CHECK_MEMBER_AT(hwc_composer_device_1_t, reserved_proc, 112, 216);

    //Types defined in gralloc.h
    CHECK_MEMBER_AT(gralloc_module_t, common, 0, 0);
    CHECK_MEMBER_AT(gralloc_module_t, registerBuffer, 128, 248);
    CHECK_MEMBER_AT(gralloc_module_t, unregisterBuffer, 132, 256);
    CHECK_MEMBER_AT(gralloc_module_t, lock, 136, 264);
    CHECK_MEMBER_AT(gralloc_module_t, unlock, 140, 272);
    CHECK_MEMBER_AT(gralloc_module_t, perform, 144, 280);
    CHECK_MEMBER_AT(gralloc_module_t, lock_ycbcr, 148, 288);
    CHECK_MEMBER_AT(gralloc_module_t, lockAsync, 152, 296);
    CHECK_MEMBER_AT(gralloc_module_t, unlockAsync, 156, 304);
    CHECK_MEMBER_AT(gralloc_module_t, lockAsync_ycbcr, 160, 312);
    CHECK_MEMBER_AT(gralloc_module_t, reserved_proc, 164, 320);

    CHECK_MEMBER_AT(alloc_device_t, common, 0, 0);
    CHECK_MEMBER_AT(alloc_device_t, alloc, 64, 120);
    CHECK_MEMBER_AT(alloc_device_t, free, 68, 128);
    CHECK_MEMBER_AT(alloc_device_t, dump, 72, 136);
    CHECK_MEMBER_AT(alloc_device_t, reserved_proc, 76, 144);

    //Types defined in consumerir.h
    CHECK_MEMBER_AT(consumerir_device_t, common, 0, 0);
    CHECK_MEMBER_AT(consumerir_device_t, transmit, 64, 120);
    CHECK_MEMBER_AT(consumerir_device_t, get_num_carrier_freqs, 68, 128);
    CHECK_MEMBER_AT(consumerir_device_t, get_carrier_freqs, 72, 136);
    CHECK_MEMBER_AT(consumerir_device_t, reserved, 76, 144);

    //Types defined in camera_common.h
    CHECK_MEMBER_AT(vendor_tag_ops_t, get_tag_count, 0, 0);
    CHECK_MEMBER_AT(vendor_tag_ops_t, get_all_tags, 4, 8);
    CHECK_MEMBER_AT(vendor_tag_ops_t, get_section_name, 8, 16);
    CHECK_MEMBER_AT(vendor_tag_ops_t, get_tag_name, 12, 24);
    CHECK_MEMBER_AT(vendor_tag_ops_t, get_tag_type, 16, 32);
    CHECK_MEMBER_AT(vendor_tag_ops_t, reserved, 20, 40);

    CHECK_MEMBER_AT(camera_module_t, common, 0, 0);
    CHECK_MEMBER_AT(camera_module_t, get_number_of_cameras, 128, 248);
    CHECK_MEMBER_AT(camera_module_t, get_camera_info, 132, 256);
    CHECK_MEMBER_AT(camera_module_t, set_callbacks, 136, 264);
    CHECK_MEMBER_AT(camera_module_t, get_vendor_tag_ops, 140, 272);
    CHECK_MEMBER_AT(camera_module_t, open_legacy, 144, 280);
    CHECK_MEMBER_AT(camera_module_t, reserved, 148, 288);

    //Types defined in camera3.h
    CHECK_MEMBER_AT(camera3_device_ops_t, initialize, 0, 0);
    CHECK_MEMBER_AT(camera3_device_ops_t, configure_streams, 4, 8);
    CHECK_MEMBER_AT(camera3_device_ops_t, register_stream_buffers, 8, 16);
    CHECK_MEMBER_AT(camera3_device_ops_t, construct_default_request_settings, 12, 24);
    CHECK_MEMBER_AT(camera3_device_ops_t, process_capture_request, 16, 32);
    CHECK_MEMBER_AT(camera3_device_ops_t, get_metadata_vendor_tag_ops, 20, 40);
    CHECK_MEMBER_AT(camera3_device_ops_t, dump, 24, 48);
    CHECK_MEMBER_AT(camera3_device_ops_t, flush, 28, 56);
    CHECK_MEMBER_AT(camera3_device_ops_t, reserved, 32, 64);
}

