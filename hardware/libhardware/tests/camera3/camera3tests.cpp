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

#include <gtest/gtest.h>
#include "camera3test_fixtures.h"

namespace tests {

TEST_F(Camera3Module, NumberOfCameras) {
    ASSERT_LT(0, num_cams()) << "No cameras found";
    ASSERT_GE(kMmaxCams, num_cams()) << "Too many cameras found";
}

TEST_F(Camera3Module, IsActiveArraySizeSubsetPixelArraySize) {
    for (int i = 0; i < num_cams(); ++i) {
        ASSERT_TRUE(NULL != cam_module()->get_camera_info)
            << "get_camera_info is not implemented";

        camera_info info;
        ASSERT_EQ(0, cam_module()->get_camera_info(i, &info))
                    << "Can't get camera info for" << i;

        camera_metadata_entry entry;
        ASSERT_EQ(0, find_camera_metadata_entry(
                    const_cast<camera_metadata_t*>(
                        info.static_camera_characteristics),
                        ANDROID_SENSOR_INFO_PIXEL_ARRAY_SIZE, &entry))
                            << "Can't find the sensor pixel array size.";
        int pixel_array_w = entry.data.i32[0];
        int pixel_array_h = entry.data.i32[1];

        ASSERT_EQ(0, find_camera_metadata_entry(
                    const_cast<camera_metadata_t*>(
                        info.static_camera_characteristics),
                        ANDROID_SENSOR_INFO_ACTIVE_ARRAY_SIZE, &entry))
                            << "Can't find the sensor active array size.";
        int active_array_w = entry.data.i32[0];
        int active_array_h = entry.data.i32[1];

        EXPECT_LE(active_array_h, pixel_array_h);
        EXPECT_LE(active_array_w, pixel_array_w);
    }
}

TEST_F(Camera3Device, DefaultSettingsStillCaptureHasAndroidControlMode) {
    ASSERT_TRUE(NULL != cam_device()->ops) << "Camera device ops are NULL";
    const camera_metadata_t *default_settings =
        cam_device()->ops->construct_default_request_settings(cam_device(),
            CAMERA3_TEMPLATE_STILL_CAPTURE);
    ASSERT_TRUE(NULL != default_settings) << "Camera default settings are NULL";
    camera_metadata_entry entry;
    ASSERT_EQ(0, find_camera_metadata_entry(
                const_cast<camera_metadata_t*>(default_settings),
                ANDROID_CONTROL_MODE, &entry))
                    << "Can't find ANDROID_CONTROL_MODE in default settings.";
}

}  // namespace tests
