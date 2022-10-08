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

#define LOG_NDEBUG 0
#define LOG_TAG "CameraMetadataTestFunctional"
#include "cutils/log.h"
#include "cutils/properties.h"
#include "utils/Errors.h"

#include "gtest/gtest.h"
#include "system/camera_metadata.h"
#include "hardware/hardware.h"
#include "hardware/camera2.h"

#include "common/CameraDeviceBase.h"
#include "utils/StrongPointer.h"

#include <gui/CpuConsumer.h>
#include <gui/Surface.h>

#include <string>

#include "CameraStreamFixture.h"
#include "TestExtensions.h"

namespace android {
namespace camera2 {
namespace tests {

//FIXME: dont hardcode
static CameraStreamParams METADATA_STREAM_PARAMETERS = {
    /*mFormat*/     HAL_PIXEL_FORMAT_YCrCb_420_SP,
    /*mHeapCount*/  2
};

class CameraMetadataTest
    : public ::testing::Test,
      public CameraStreamFixture {

public:
    CameraMetadataTest()
    : CameraStreamFixture(METADATA_STREAM_PARAMETERS) {
        TEST_EXTENSION_FORKING_CONSTRUCTOR;
    }

    ~CameraMetadataTest() {
        TEST_EXTENSION_FORKING_DESTRUCTOR;
    }

    int GetTypeFromTag(uint32_t tag) const {
        return get_camera_metadata_tag_type(tag);
    }

    int GetTypeFromStaticTag(uint32_t tag) const {
        const CameraMetadata& staticInfo = mDevice->info();
        camera_metadata_ro_entry entry = staticInfo.find(tag);
        return entry.type;
    }

    int GetEntryCountFromStaticTag(uint32_t tag) const {
        const CameraMetadata& staticInfo = mDevice->info();
        camera_metadata_ro_entry entry = staticInfo.find(tag);
        return entry.count;
    }

    bool HasElementInArrayFromStaticTag(uint32_t tag, int32_t element) const {
        const CameraMetadata& staticInfo = mDevice->info();
        camera_metadata_ro_entry entry = staticInfo.find(tag);
        for (size_t i = 0; i < entry.count; ++i) {
            if (entry.data.i32[i] == element)
                return true;
        }
        return false;
    }

protected:

};

TEST_F(CameraMetadataTest, types) {

    TEST_EXTENSION_FORKING_INIT;

    //FIXME: set this up in an external file of some sort (xml?)
    {
        char value[PROPERTY_VALUE_MAX];
        property_get("ro.build.id", value, "");
        std::string str_value(value);

        if (str_value == "manta")
        {
            EXPECT_EQ(TYPE_BYTE,
                GetTypeFromStaticTag(ANDROID_QUIRKS_TRIGGER_AF_WITH_AUTO));
            EXPECT_EQ(TYPE_BYTE,
                GetTypeFromStaticTag(ANDROID_QUIRKS_USE_ZSL_FORMAT));
            EXPECT_EQ(TYPE_BYTE,
                GetTypeFromStaticTag(ANDROID_QUIRKS_METERING_CROP_REGION));
        }
    }

    /*
    TODO:
    go through all static metadata and make sure all fields we expect
    that are there, ARE there.

    dont worry about the type as its enforced by the metadata api
    we can probably check the range validity though
    */

    if (0) {
        camera_metadata_ro_entry entry;
        EXPECT_EQ(TYPE_BYTE,     entry.type);
        EXPECT_EQ(TYPE_INT32,    entry.type);
        EXPECT_EQ(TYPE_FLOAT,    entry.type);
        EXPECT_EQ(TYPE_INT64,    entry.type);
        EXPECT_EQ(TYPE_DOUBLE,   entry.type);
        EXPECT_EQ(TYPE_RATIONAL, entry.type);
    }
}

TEST_F(CameraMetadataTest, RequiredFormats) {
    TEST_EXTENSION_FORKING_INIT;

    EXPECT_TRUE(
        HasElementInArrayFromStaticTag(ANDROID_SCALER_AVAILABLE_FORMATS,
                                       HAL_PIXEL_FORMAT_BLOB)); // JPEG

    if (getDeviceVersion() < CAMERA_DEVICE_API_VERSION_3_0) {
        // HAL2 can support either flexible YUV or YV12 + NV21
        if (!HasElementInArrayFromStaticTag(ANDROID_SCALER_AVAILABLE_FORMATS,
                        HAL_PIXEL_FORMAT_YCbCr_420_888)) {

            EXPECT_TRUE(
                HasElementInArrayFromStaticTag(ANDROID_SCALER_AVAILABLE_FORMATS,
                        HAL_PIXEL_FORMAT_YCrCb_420_SP)); // NV21

            EXPECT_TRUE(
                HasElementInArrayFromStaticTag(ANDROID_SCALER_AVAILABLE_FORMATS,
                        HAL_PIXEL_FORMAT_YV12));
        }
    } else {
        // HAL3 must support flexible YUV
        EXPECT_TRUE(HasElementInArrayFromStaticTag(ANDROID_SCALER_AVAILABLE_FORMATS,
                        HAL_PIXEL_FORMAT_YCbCr_420_888));
    }

}

TEST_F(CameraMetadataTest, SaneResolutions) {
    TEST_EXTENSION_FORKING_INIT;

    if (getDeviceVersion() < CAMERA_DEVICE_API_VERSION_3_2) {
        // Iff there are listed raw resolutions, the format should be available
        int rawResolutionsCount =
                GetEntryCountFromStaticTag(ANDROID_SCALER_AVAILABLE_RAW_SIZES);
        if (rawResolutionsCount > 0) {
            EXPECT_TRUE(
                HasElementInArrayFromStaticTag(ANDROID_SCALER_AVAILABLE_FORMATS,
                        HAL_PIXEL_FORMAT_RAW_SENSOR));
        }

        // Required processed sizes.
        int processedSizeCount =
               GetEntryCountFromStaticTag(ANDROID_SCALER_AVAILABLE_PROCESSED_SIZES);
        EXPECT_NE(0, processedSizeCount);
        EXPECT_EQ(0, processedSizeCount % 2); // multiple of 2 (w,h)

        // Required JPEG sizes
        int jpegSizeCount =
                GetEntryCountFromStaticTag(ANDROID_SCALER_AVAILABLE_JPEG_SIZES);
        EXPECT_NE(0, jpegSizeCount);
        EXPECT_EQ(0, jpegSizeCount % 2); // multiple of 2 (w,h)
    } else {
        int strmConfigCount =
                GetEntryCountFromStaticTag(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS);
        EXPECT_NE(0, strmConfigCount);
        EXPECT_EQ(0, strmConfigCount % 4); // multiple of 4 (format,w,h,output?)
    }

}

}
}
}
