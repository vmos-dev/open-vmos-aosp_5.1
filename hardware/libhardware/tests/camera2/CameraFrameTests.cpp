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

#include <gtest/gtest.h>

#define LOG_TAG "CameraFrameTest"
//#define LOG_NDEBUG 0
#include <utils/Log.h>

#include "hardware/hardware.h"
#include "hardware/camera2.h"

#include <common/CameraDeviceBase.h>
#include <utils/StrongPointer.h>
#include <gui/CpuConsumer.h>
#include <gui/Surface.h>

#include <unistd.h>

#include "CameraStreamFixture.h"
#include "TestExtensions.h"

#define CAMERA_FRAME_TIMEOUT    1000000000 //nsecs (1 secs)
#define CAMERA_HEAP_COUNT       2 //HALBUG: 1 means registerBuffers fails
#define CAMERA_FRAME_DEBUGGING  0

using namespace android;
using namespace android::camera2;

namespace android {
namespace camera2 {
namespace tests {

static CameraStreamParams STREAM_PARAMETERS = {
    /*mFormat*/     CAMERA_STREAM_AUTO_CPU_FORMAT,
    /*mHeapCount*/  CAMERA_HEAP_COUNT
};

class CameraFrameTest
    : public ::testing::TestWithParam<int>,
      public CameraStreamFixture {

public:
    CameraFrameTest() : CameraStreamFixture(STREAM_PARAMETERS) {
        TEST_EXTENSION_FORKING_CONSTRUCTOR;

        if (!HasFatalFailure()) {
            CreateStream();
        }
    }

    ~CameraFrameTest() {
        TEST_EXTENSION_FORKING_DESTRUCTOR;

        if (mDevice.get()) {
            mDevice->waitUntilDrained();
        }
    }

    virtual void SetUp() {
        TEST_EXTENSION_FORKING_SET_UP;
    }
    virtual void TearDown() {
        TEST_EXTENSION_FORKING_TEAR_DOWN;
    }

protected:

};

TEST_P(CameraFrameTest, GetFrame) {

    TEST_EXTENSION_FORKING_INIT;

    /* Submit a PREVIEW type request, then wait until we get the frame back */
    CameraMetadata previewRequest;
    ASSERT_EQ(OK, mDevice->createDefaultRequest(CAMERA2_TEMPLATE_PREVIEW,
                                                &previewRequest));
    {
        Vector<int32_t> outputStreamIds;
        outputStreamIds.push(mStreamId);
        ASSERT_EQ(OK, previewRequest.update(ANDROID_REQUEST_OUTPUT_STREAMS,
                                            outputStreamIds));
        if (CAMERA_FRAME_DEBUGGING) {
            int frameCount = 0;
            ASSERT_EQ(OK, previewRequest.update(ANDROID_REQUEST_FRAME_COUNT,
                                                &frameCount, 1));
        }
    }

    if (CAMERA_FRAME_DEBUGGING) {
        previewRequest.dump(STDOUT_FILENO);
    }

    for (int i = 0; i < GetParam(); ++i) {
        ALOGV("Submitting capture request %d", i);
        CameraMetadata tmpRequest = previewRequest;
        ASSERT_EQ(OK, mDevice->capture(tmpRequest));
    }

    for (int i = 0; i < GetParam(); ++i) {
        ALOGV("Reading capture request %d", i);
        ASSERT_EQ(OK, mDevice->waitForNextFrame(CAMERA_FRAME_TIMEOUT));

        CaptureResult result;
        ASSERT_EQ(OK, mDevice->getNextResult(&result));

        // wait for buffer to be available
        ASSERT_EQ(OK, mFrameListener->waitForFrame(CAMERA_FRAME_TIMEOUT));
        ALOGV("We got the frame now");

        // mark buffer consumed so producer can re-dequeue it
        CpuConsumer::LockedBuffer imgBuffer;
        ASSERT_EQ(OK, mCpuConsumer->lockNextBuffer(&imgBuffer));
        ASSERT_EQ(OK, mCpuConsumer->unlockBuffer(imgBuffer));
    }

}

//FIXME: dont hardcode stream params, and also test multistream
INSTANTIATE_TEST_CASE_P(FrameParameterCombinations, CameraFrameTest,
    testing::Range(1, 10));


}
}
}
