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

#ifndef __ANDROID_HAL_CAMERA2_TESTS_MODULE_FIXTURE__
#define __ANDROID_HAL_CAMERA2_TESTS_MODULE_FIXTURE__

#include <gtest/gtest.h>

#include "hardware/hardware.h"
#include "hardware/camera2.h"

#include <device2/Camera2Device.h>
#include <device3/Camera3Device.h>

#include "camera2_utils.h"
#include "TestExtensions.h"

namespace android {
namespace camera2 {
namespace tests {

template <bool InfoQuirk = false>
struct CameraModuleFixture {

    CameraModuleFixture(int CameraID = -1) {
        TEST_EXTENSION_FORKING_CONSTRUCTOR;

        mCameraID = CameraID;
    }

    ~CameraModuleFixture() {
        TEST_EXTENSION_FORKING_DESTRUCTOR;
    }

    camera_metadata_ro_entry GetStaticEntry(uint32_t tag) const {
        const CameraMetadata& staticInfo = mDevice->info();
        camera_metadata_ro_entry entry = staticInfo.find(tag);
        return entry;
    }

    void SetUp() {
        TEST_EXTENSION_FORKING_SET_UP;

        ASSERT_LE(0, hw_get_module(CAMERA_HARDWARE_MODULE_ID,
            (const hw_module_t **)&mModule)) << "Could not load camera module";
        ASSERT_NE((void*)0, mModule);

        mNumberOfCameras = mModule->get_number_of_cameras();
        ASSERT_LE(0, mNumberOfCameras);

        ASSERT_LE(
            CAMERA_MODULE_API_VERSION_2_0, mModule->common.module_api_version)
            << "Wrong module API version";

        /* For using this fixture in other tests only */
        SetUpMixin();
    }

    void TearDown() {
        TEST_EXTENSION_FORKING_TEAR_DOWN;

        TearDownMixin();

        /* important: device must be destructed before closing module,
           since it calls back into HAL */
        mDevice.clear();

        if (!TEST_EXTENSION_FORKING_ENABLED) {
            ASSERT_EQ(0, HWModuleHelpers::closeModule(&mModule->common))
                << "Failed to close camera HAL module";
        }
    }

    void CreateCamera(int cameraID, /*out*/ sp<CameraDeviceBase> *device) {
        struct camera_info info;
        ASSERT_EQ(OK, mModule->get_camera_info(cameraID, &info));

        ASSERT_GE((int)info.device_version, CAMERA_DEVICE_API_VERSION_2_0) <<
                "Device version too old for camera " << cameraID << ". Version: " <<
                info.device_version;
        switch(info.device_version) {
            case CAMERA_DEVICE_API_VERSION_2_0:
            case CAMERA_DEVICE_API_VERSION_2_1:
                *device = new Camera2Device(cameraID);
                break;
            case CAMERA_DEVICE_API_VERSION_3_0:
            case CAMERA_DEVICE_API_VERSION_3_1:
            case CAMERA_DEVICE_API_VERSION_3_2:
                *device = new Camera3Device(cameraID);
                break;
            default:
                device->clear();
                FAIL() << "Device version unknown for camera " << cameraID << ". Version: " <<
                       info.device_version;
        }

    }

    int getDeviceVersion() {
        return getDeviceVersion(mCameraID);
    }

    int getDeviceVersion(int cameraId, status_t* status = NULL) {
        camera_info info;
        status_t res;
        res = mModule->get_camera_info(cameraId, &info);
        if (status != NULL) *status = res;

        return info.device_version;
    }

private:

    void SetUpMixin() {
        /* For using this fixture in other tests only */
        if (mCameraID != -1) {
            EXPECT_LE(0, mCameraID);
            EXPECT_LT(mCameraID, mNumberOfCameras);

            /* HALBUG (Exynos5); crashes if we skip calling get_camera_info
               before initializing. Need info anyway now. */

            CreateCamera(mCameraID, &mDevice);

            ASSERT_TRUE(mDevice != NULL) << "Failed to open device " << mCameraID;
            ASSERT_EQ(OK, mDevice->initialize(mModule))
                << "Failed to initialize device " << mCameraID;
        }
    }

    void TearDownMixin() {

    }

protected:
    int mNumberOfCameras;
    camera_module_t *mModule;
    sp<CameraDeviceBase> mDevice;

private:
    int mCameraID;
};


}
}
}

#endif
