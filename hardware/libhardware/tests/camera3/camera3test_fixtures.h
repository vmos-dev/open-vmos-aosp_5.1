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

#ifndef __ANDROID_HAL_CAMERA3_TEST_COMMON__
#define __ANDROID_HAL_CAMERA3_TEST_COMMON__

#include <gtest/gtest.h>
#include <hardware/hardware.h>
#include <hardware/camera3.h>

namespace tests {

static const int kMmaxCams = 2;
static const uint16_t kVersion3_0 = HARDWARE_MODULE_API_VERSION(3, 0);

class Camera3Module : public testing::Test {
 public:
    Camera3Module() :
        num_cams_(0),
        cam_module_(NULL) {}
    ~Camera3Module() {}
 protected:
    virtual void SetUp() {
        const hw_module_t *hw_module = NULL;
        ASSERT_EQ(0, hw_get_module(CAMERA_HARDWARE_MODULE_ID, &hw_module))
                    << "Can't get camera module";
        ASSERT_TRUE(NULL != hw_module)
                    << "hw_get_module didn't return a valid camera module";

        cam_module_ = reinterpret_cast<const camera_module_t*>(hw_module);
        ASSERT_TRUE(NULL != cam_module_->get_number_of_cameras)
                    << "get_number_of_cameras is not implemented";
        num_cams_ = cam_module_->get_number_of_cameras();
    }
    int num_cams() { return num_cams_; }
    const camera_module_t * cam_module() { return cam_module_; }
 private:
    int num_cams_;
    const camera_module_t *cam_module_;
};

class Camera3Device : public Camera3Module {
 public:
    Camera3Device() :
        cam_device_(NULL) {}
    ~Camera3Device() {}
 protected:
    virtual void SetUp() {
        Camera3Module::SetUp();
        hw_device_t *device = NULL;
        ASSERT_TRUE(NULL != cam_module()->common.methods->open)
                    << "Camera open() is unimplemented";
        ASSERT_EQ(0, cam_module()->common.methods->open(
            (const hw_module_t*)cam_module(), "0", &device))
                << "Can't open camera device";
        ASSERT_TRUE(NULL != device)
                    << "Camera open() returned a NULL device";
        ASSERT_LE(kVersion3_0, device->version)
                    << "The device does not support HAL3";
        cam_device_ = reinterpret_cast<camera3_device_t*>(device);
    }
    camera3_device_t* cam_device() { return cam_device_; }
 private:
    camera3_device *cam_device_;
};

}  // namespace tests

#endif  // __ANDROID_HAL_CAMERA3_TEST_COMMON__
