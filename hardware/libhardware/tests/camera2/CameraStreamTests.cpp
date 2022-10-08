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

#include <iostream>
#include <iomanip>
#include <gtest/gtest.h>

#define LOG_TAG "CameraStreamTest"
#define LOG_NDEBUG 0
#include <utils/Log.h>

#include "hardware/hardware.h"
#include "hardware/camera2.h"

#include <utils/StrongPointer.h>
#include <gui/CpuConsumer.h>
#include <gui/Surface.h>

#include <device2/Camera2Device.h>

#include "CameraStreamFixture.h"
#include "TestExtensions.h"

using namespace android;
using namespace android::camera2;

namespace android {
namespace camera2 {
namespace tests {

class CameraStreamTest
    : public ::testing::TestWithParam<CameraStreamParams>,
      public CameraStreamFixture {

public:
    CameraStreamTest() : CameraStreamFixture(GetParam()) {
        TEST_EXTENSION_FORKING_CONSTRUCTOR;
    }

    ~CameraStreamTest() {
        TEST_EXTENSION_FORKING_DESTRUCTOR;
    }

    virtual void SetUp() {
        TEST_EXTENSION_FORKING_SET_UP;
    }
    virtual void TearDown() {
        TEST_EXTENSION_FORKING_TEAR_DOWN;
    }

protected:

};

TEST_P(CameraStreamTest, CreateStream) {

    TEST_EXTENSION_FORKING_INIT;

    /** Make sure the format requested is supported. PASS this test if it's not
      * not supported.
      *
      * TODO: would be nice of not running this test in the first place
      *       somehow.
      */
    {
        camera_metadata_ro_entry availableFormats =
            GetStaticEntry(ANDROID_SCALER_AVAILABLE_FORMATS);

        bool hasFormat = false;
        for (size_t i = 0; i < availableFormats.count; ++i) {
            if (availableFormats.data.i32[i] == GetParam().mFormat) {
                hasFormat = true;
                break;
            }
        }

        if (!hasFormat) {
            const ::testing::TestInfo* const test_info =
                ::testing::UnitTest::GetInstance()->current_test_info();
            std::cerr << "Skipping test "
                      << test_info->test_case_name() << "."
                      << test_info->name()
                      << " because the format was not available: "
                      << GetParam() << std::endl;
            return;
        }
    }

    ASSERT_NO_FATAL_FAILURE(CreateStream());
    ASSERT_NO_FATAL_FAILURE(DeleteStream());
}

//TODO: use a combinatoric generator
static CameraStreamParams TestParameters[] = {
    {
        /*mFormat*/    HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED,
        /*mHeapCount*/ 1
    },
    {
        /*mFormat*/    HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED,
        /*mHeapCount*/ 2
    },
    {
        /*mFormat*/    HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED,
        /*mHeapCount*/ 3
    },
    {
        /*mFormat*/    HAL_PIXEL_FORMAT_YCrCb_420_SP, // NV21
        /*mHeapCount*/ 1
    },
    {
        /*mFormat*/    HAL_PIXEL_FORMAT_YCrCb_420_SP,
        /*mHeapCount*/ 2
    },
    {
        /*mFormat*/    HAL_PIXEL_FORMAT_YCrCb_420_SP,
        /*mHeapCount*/ 3
    },
    {
        /*mFormat*/    HAL_PIXEL_FORMAT_YV12,
        /*mHeapCount*/ 1
    },
    {
        /*mFormat*/    HAL_PIXEL_FORMAT_YV12,
        /*mHeapCount*/ 2
    },
    {
        /*mFormat*/    HAL_PIXEL_FORMAT_YV12,
        /*mHeapCount*/ 3
    },
    {
        /*mFormat*/    HAL_PIXEL_FORMAT_Y8,
        /*mHeapCount*/ 1
    },
    {
        /*mFormat*/    HAL_PIXEL_FORMAT_Y8,
        /*mHeapCount*/ 2
    },
    {
        /*mFormat*/    HAL_PIXEL_FORMAT_Y8,
        /*mHeapCount*/ 3
    },
    {
        /*mFormat*/    HAL_PIXEL_FORMAT_Y16,
        /*mHeapCount*/ 1
    },
    {
        /*mFormat*/    HAL_PIXEL_FORMAT_Y16,
        /*mHeapCount*/ 2
    },
    {
        /*mFormat*/    HAL_PIXEL_FORMAT_Y16,
        /*mHeapCount*/ 3
    },
    {
        /*mFormat*/    HAL_PIXEL_FORMAT_RAW_SENSOR,
        /*mHeapCount*/ 1
    },
    {
        /*mFormat*/    HAL_PIXEL_FORMAT_RAW_SENSOR,
        /*mHeapCount*/ 2
    },
    {
        /*mFormat*/    HAL_PIXEL_FORMAT_RAW_SENSOR,
        /*mHeapCount*/ 3
    },
};

INSTANTIATE_TEST_CASE_P(StreamParameterCombinations, CameraStreamTest,
    testing::ValuesIn(TestParameters));


}
}
}
