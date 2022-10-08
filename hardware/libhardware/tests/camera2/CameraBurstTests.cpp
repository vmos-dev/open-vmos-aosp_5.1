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
#include <inttypes.h>

#define LOG_TAG "CameraBurstTest"
//#define LOG_NDEBUG 0
#include <utils/Log.h>
#include <utils/Timers.h>

#include <cmath>

#include "CameraStreamFixture.h"
#include "TestExtensions.h"

#define CAMERA_FRAME_TIMEOUT    1000000000LL //nsecs (1 secs)
#define CAMERA_HEAP_COUNT       2 //HALBUG: 1 means registerBuffers fails
#define CAMERA_BURST_DEBUGGING  0
#define CAMERA_FRAME_BURST_COUNT 10

/* constants for the exposure test */
#define CAMERA_EXPOSURE_DOUBLE  2
#define CAMERA_EXPOSURE_DOUBLING_THRESHOLD 1.0f
#define CAMERA_EXPOSURE_DOUBLING_COUNT 4
#define CAMERA_EXPOSURE_FORMAT CAMERA_STREAM_AUTO_CPU_FORMAT
#define CAMERA_EXPOSURE_STARTING 100000 // 1/10ms, up to 51.2ms with 10 steps

#define USEC 1000LL        // in ns
#define MSEC 1000000LL     // in ns
#define SEC  1000000000LL  // in ns

#if CAMERA_BURST_DEBUGGING
#define dout std::cout
#else
#define dout if (0) std::cout
#endif

#define WARN_UNLESS(condition) (!(condition) ? (std::cerr) : (std::ostream(NULL)) << "Warning: ")
#define WARN_LE(exp, act) WARN_UNLESS((exp) <= (act))
#define WARN_LT(exp, act) WARN_UNLESS((exp) < (act))
#define WARN_GT(exp, act) WARN_UNLESS((exp) > (act))

using namespace android;
using namespace android::camera2;

namespace android {
namespace camera2 {
namespace tests {

static CameraStreamParams STREAM_PARAMETERS = {
    /*mFormat*/     CAMERA_EXPOSURE_FORMAT,
    /*mHeapCount*/  CAMERA_HEAP_COUNT
};

class CameraBurstTest
    : public ::testing::Test,
      public CameraStreamFixture {

public:
    CameraBurstTest() : CameraStreamFixture(STREAM_PARAMETERS) {
        TEST_EXTENSION_FORKING_CONSTRUCTOR;

        if (HasFatalFailure()) {
            return;
        }

        CreateStream();
    }

    ~CameraBurstTest() {
        TEST_EXTENSION_FORKING_DESTRUCTOR;

        if (mDevice.get()) {
            mDevice->waitUntilDrained();
        }
        DeleteStream();
    }

    virtual void SetUp() {
        TEST_EXTENSION_FORKING_SET_UP;
    }
    virtual void TearDown() {
        TEST_EXTENSION_FORKING_TEAR_DOWN;
    }

    /* this assumes the format is YUV420sp or flexible YUV */
    long long TotalBrightness(const CpuConsumer::LockedBuffer& imgBuffer,
                              int *underexposed,
                              int *overexposed) const {

        const uint8_t* buf = imgBuffer.data;
        size_t stride = imgBuffer.stride;

        /* iterate over the Y plane only */
        long long acc = 0;

        *underexposed = 0;
        *overexposed = 0;

        for (size_t y = 0; y < imgBuffer.height; ++y) {
            for (size_t x = 0; x < imgBuffer.width; ++x) {
                const uint8_t p = buf[y * stride + x];

                if (p == 0) {
                    if (underexposed) {
                        ++*underexposed;
                    }
                    continue;
                } else if (p == 255) {
                    if (overexposed) {
                        ++*overexposed;
                    }
                    continue;
                }

                acc += p;
            }
        }

        return acc;
    }

    // Parses a comma-separated string list into a Vector
    template<typename T>
    void ParseList(const char *src, Vector<T> &list) {
        std::istringstream s(src);
        while (!s.eof()) {
            char c = s.peek();
            if (c == ',' || c == ' ') {
                s.ignore(1, EOF);
                continue;
            }
            T val;
            s >> val;
            list.push_back(val);
        }
    }

};

TEST_F(CameraBurstTest, ManualExposureControl) {

    TEST_EXTENSION_FORKING_INIT;

    // Range of valid exposure times, in nanoseconds
    int64_t minExp, maxExp;
    {
        camera_metadata_ro_entry exposureTimeRange =
            GetStaticEntry(ANDROID_SENSOR_INFO_EXPOSURE_TIME_RANGE);

        ASSERT_EQ(2u, exposureTimeRange.count);
        minExp = exposureTimeRange.data.i64[0];
        maxExp = exposureTimeRange.data.i64[1];
    }

    dout << "Min exposure is " << minExp;
    dout << " max exposure is " << maxExp << std::endl;

    // Calculate some set of valid exposure times for each request
    int64_t exposures[CAMERA_FRAME_BURST_COUNT];
    exposures[0] = CAMERA_EXPOSURE_STARTING;
    for (int i = 1; i < CAMERA_FRAME_BURST_COUNT; ++i) {
        exposures[i] = exposures[i-1] * CAMERA_EXPOSURE_DOUBLE;
    }
    // Our calculated exposure times should be in [minExp, maxExp]
    EXPECT_LE(minExp, exposures[0])
        << "Minimum exposure range is too high, wanted at most "
        << exposures[0] << "ns";
    EXPECT_GE(maxExp, exposures[CAMERA_FRAME_BURST_COUNT-1])
        << "Maximum exposure range is too low, wanted at least "
        << exposures[CAMERA_FRAME_BURST_COUNT-1] << "ns";

    // Create a preview request, turning off all 3A
    CameraMetadata previewRequest;
    ASSERT_EQ(OK, mDevice->createDefaultRequest(CAMERA2_TEMPLATE_PREVIEW,
                                                &previewRequest));
    {
        Vector<int32_t> outputStreamIds;
        outputStreamIds.push(mStreamId);
        ASSERT_EQ(OK, previewRequest.update(ANDROID_REQUEST_OUTPUT_STREAMS,
                                            outputStreamIds));

        // Disable all 3A routines
        uint8_t cmOff = static_cast<uint8_t>(ANDROID_CONTROL_MODE_OFF);
        ASSERT_EQ(OK, previewRequest.update(ANDROID_CONTROL_MODE,
                                            &cmOff, 1));

        int requestId = 1;
        ASSERT_EQ(OK, previewRequest.update(ANDROID_REQUEST_ID,
                                            &requestId, 1));

        if (CAMERA_BURST_DEBUGGING) {
            int frameCount = 0;
            ASSERT_EQ(OK, previewRequest.update(ANDROID_REQUEST_FRAME_COUNT,
                                                &frameCount, 1));
        }
    }

    if (CAMERA_BURST_DEBUGGING) {
        previewRequest.dump(STDOUT_FILENO);
    }

    // Submit capture requests
    for (int i = 0; i < CAMERA_FRAME_BURST_COUNT; ++i) {
        CameraMetadata tmpRequest = previewRequest;
        ASSERT_EQ(OK, tmpRequest.update(ANDROID_SENSOR_EXPOSURE_TIME,
                                        &exposures[i], 1));
        ALOGV("Submitting capture request %d with exposure %"PRId64, i,
            exposures[i]);
        dout << "Capture request " << i << " exposure is "
             << (exposures[i]/1e6f) << std::endl;
        ASSERT_EQ(OK, mDevice->capture(tmpRequest));
    }

    dout << "Buffer dimensions " << mWidth << "x" << mHeight << std::endl;

    float brightnesses[CAMERA_FRAME_BURST_COUNT];
    // Get each frame (metadata) and then the buffer. Calculate brightness.
    for (int i = 0; i < CAMERA_FRAME_BURST_COUNT; ++i) {
        ALOGV("Reading capture request %d with exposure %"PRId64, i, exposures[i]);
        ASSERT_EQ(OK, mDevice->waitForNextFrame(CAMERA_FRAME_TIMEOUT));
        ALOGV("Reading capture request-1 %d", i);
        CaptureResult result;
        ASSERT_EQ(OK, mDevice->getNextResult(&result));
        ALOGV("Reading capture request-2 %d", i);

        ASSERT_EQ(OK, mFrameListener->waitForFrame(CAMERA_FRAME_TIMEOUT));
        ALOGV("We got the frame now");

        CpuConsumer::LockedBuffer imgBuffer;
        ASSERT_EQ(OK, mCpuConsumer->lockNextBuffer(&imgBuffer));

        int underexposed, overexposed;
        long long brightness = TotalBrightness(imgBuffer, &underexposed,
                                               &overexposed);
        float avgBrightness = brightness * 1.0f /
                              (mWidth * mHeight - (underexposed + overexposed));
        ALOGV("Total brightness for frame %d was %lld (underexposed %d, "
              "overexposed %d), avg %f", i, brightness, underexposed,
              overexposed, avgBrightness);
        dout << "Average brightness (frame " << i << ") was " << avgBrightness
             << " (underexposed " << underexposed << ", overexposed "
             << overexposed << ")" << std::endl;

        ASSERT_EQ(OK, mCpuConsumer->unlockBuffer(imgBuffer));

        brightnesses[i] = avgBrightness;
    }

    // Calculate max consecutive frame exposure doubling
    float prev = brightnesses[0];
    int doubling_count = 1;
    int max_doubling_count = 0;
    for (int i = 1; i < CAMERA_FRAME_BURST_COUNT; ++i) {
        if (fabs(brightnesses[i] - prev*CAMERA_EXPOSURE_DOUBLE)
            <= CAMERA_EXPOSURE_DOUBLING_THRESHOLD) {
            doubling_count++;
        }
        else {
            max_doubling_count = std::max(max_doubling_count, doubling_count);
            doubling_count = 1;
        }
        prev = brightnesses[i];
    }

    dout << "max doubling count: " << max_doubling_count << std::endl;

    /**
     * Make this check warning only, since the brightness calculation is not reliable
     * and we have separate test to cover this case. Plus it is pretty subtle to make
     * it right without complicating the test too much.
     */
    WARN_LE(CAMERA_EXPOSURE_DOUBLING_COUNT, max_doubling_count)
            << "average brightness should double at least "
            << CAMERA_EXPOSURE_DOUBLING_COUNT
            << " times over each consecutive frame as the exposure is doubled"
            << std::endl;
}

/**
 * This test varies exposure time, frame duration, and sensitivity for a
 * burst of captures. It picks values by default, but the selection can be
 * overridden with the environment variables
 *   CAMERA2_TEST_VARIABLE_BURST_EXPOSURE_TIMES
 *   CAMERA2_TEST_VARIABLE_BURST_FRAME_DURATIONS
 *   CAMERA2_TEST_VARIABLE_BURST_SENSITIVITIES
 * which must all be a list of comma-separated values, and each list must be
 * the same length.  In addition, if the environment variable
 *   CAMERA2_TEST_VARIABLE_BURST_DUMP_FRAMES
 * is set to 1, then the YUV buffers are dumped into files named
 *   "camera2_test_variable_burst_frame_NNN.yuv"
 *
 * For example:
 *   $ setenv CAMERA2_TEST_VARIABLE_BURST_EXPOSURE_TIMES 10000000,20000000
 *   $ setenv CAMERA2_TEST_VARIABLE_BURST_FRAME_DURATIONS 40000000,40000000
 *   $ setenv CAMERA2_TEST_VARIABLE_BURST_SENSITIVITIES 200,100
 *   $ setenv CAMERA2_TEST_VARIABLE_BURST_DUMP_FRAMES 1
 *   $ /data/nativetest/camera2_test/camera2_test --gtest_filter="*VariableBurst"
 */
TEST_F(CameraBurstTest, VariableBurst) {

    TEST_EXTENSION_FORKING_INIT;

    // Bounds for checking frame duration is within range
    const nsecs_t DURATION_UPPER_BOUND = 10 * MSEC;
    const nsecs_t DURATION_LOWER_BOUND = 20 * MSEC;

    // Threshold for considering two captures to have equivalent exposure value,
    // as a ratio of the smaller EV to the larger EV.
    const float   EV_MATCH_BOUND = 0.95;
    // Bound for two captures with equivalent exp values to have the same
    // measured brightness, in 0-255 luminance.
    const float   BRIGHTNESS_MATCH_BOUND = 5;

    // Environment variables to look for to override test settings
    const char *expEnv         = "CAMERA2_TEST_VARIABLE_BURST_EXPOSURE_TIMES";
    const char *durationEnv    = "CAMERA2_TEST_VARIABLE_BURST_FRAME_DURATIONS";
    const char *sensitivityEnv = "CAMERA2_TEST_VARIABLE_BURST_SENSITIVITIES";
    const char *dumpFrameEnv   = "CAMERA2_TEST_VARIABLE_BURST_DUMP_FRAMES";

    // Range of valid exposure times, in nanoseconds
    int64_t minExp = 0, maxExp = 0;
    // List of valid sensor sensitivities
    Vector<int32_t> sensitivities;
    // Range of valid frame durations, in nanoseconds
    int64_t minDuration = 0, maxDuration = 0;

    {
        camera_metadata_ro_entry exposureTimeRange =
            GetStaticEntry(ANDROID_SENSOR_INFO_EXPOSURE_TIME_RANGE);

        EXPECT_EQ(2u, exposureTimeRange.count) << "Bad exposure time range tag."
                "Using default values";
        if (exposureTimeRange.count == 2) {
            minExp = exposureTimeRange.data.i64[0];
            maxExp = exposureTimeRange.data.i64[1];
        }

        EXPECT_LT(0, minExp) << "Minimum exposure time is 0";
        EXPECT_LT(0, maxExp) << "Maximum exposure time is 0";
        EXPECT_LE(minExp, maxExp) << "Minimum exposure is greater than maximum";

        if (minExp == 0) {
            minExp = 1 * MSEC; // Fallback minimum exposure time
        }

        if (maxExp == 0) {
            maxExp = 10 * SEC; // Fallback maximum exposure time
        }
    }

    camera_metadata_ro_entry hardwareLevel =
        GetStaticEntry(ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL);
    ASSERT_EQ(1u, hardwareLevel.count);
    uint8_t level = hardwareLevel.data.u8[0];
    ASSERT_GE(level, ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_LIMITED);
    ASSERT_LE(level, ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_FULL);
    if (level == ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_LIMITED) {
        const ::testing::TestInfo* const test_info =
            ::testing::UnitTest::GetInstance()->current_test_info();
        std::cerr << "Skipping test "
                  << test_info->test_case_name() << "."
                  << test_info->name()
                  << " because HAL hardware supported level is limited "
                  << std::endl;
        return;
    }

    dout << "Stream size is " << mWidth << " x " << mHeight << std::endl;
    dout << "Valid exposure range is: " <<
            minExp << " - " << maxExp << " ns " << std::endl;

    {
        camera_metadata_ro_entry sensivityRange =
            GetStaticEntry(ANDROID_SENSOR_INFO_SENSITIVITY_RANGE);
        EXPECT_EQ(2u, sensivityRange.count) << "No sensitivity range listed."
                "Falling back to default set.";
        int32_t minSensitivity = 100;
        int32_t maxSensitivity = 800;
        if (sensivityRange.count == 2) {
            ASSERT_GT(sensivityRange.data.i32[0], 0);
            ASSERT_GT(sensivityRange.data.i32[1], 0);
            minSensitivity = sensivityRange.data.i32[0];
            maxSensitivity = sensivityRange.data.i32[1];
        }
        int32_t count = (maxSensitivity - minSensitivity + 99) / 100;
        sensitivities.push_back(minSensitivity);
        for (int i = 1; i < count; i++) {
            sensitivities.push_back(minSensitivity + i * 100);
        }
        sensitivities.push_back(maxSensitivity);
    }

    dout << "Available sensitivities: ";
    for (size_t i = 0; i < sensitivities.size(); i++) {
        dout << sensitivities[i] << " ";
    }
    dout << std::endl;

    {
        if (getDeviceVersion() < CAMERA_DEVICE_API_VERSION_3_2) {
            camera_metadata_ro_entry availableProcessedSizes =
                    GetStaticEntry(ANDROID_SCALER_AVAILABLE_PROCESSED_SIZES);

            camera_metadata_ro_entry availableProcessedMinFrameDurations =
                    GetStaticEntry(ANDROID_SCALER_AVAILABLE_PROCESSED_MIN_DURATIONS);

            EXPECT_EQ(availableProcessedSizes.count,
                    availableProcessedMinFrameDurations.count * 2) <<
                    "The number of minimum frame durations doesn't match the number of "
                    "available sizes. Using fallback values";

            if (availableProcessedSizes.count ==
                    availableProcessedMinFrameDurations.count * 2) {
                bool gotSize = false;
                for (size_t i = 0; i < availableProcessedSizes.count; i += 2) {
                    if (availableProcessedSizes.data.i32[i] == mWidth &&
                            availableProcessedSizes.data.i32[i+1] == mHeight) {
                        gotSize = true;
                        minDuration = availableProcessedMinFrameDurations.data.i64[i/2];
                    }
                }
                EXPECT_TRUE(gotSize) << "Can't find stream size in list of "
                        "available sizes: " << mWidth << ", " << mHeight;
            }
            if (minDuration == 0) {
                minDuration = 1 * SEC / 30; // Fall back to 30 fps as minimum duration
            }
        } else {
            minDuration = getMinFrameDurationFor(
                    HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED, mWidth, mHeight);
        }
        ASSERT_LT(0, minDuration);

        camera_metadata_ro_entry maxFrameDuration =
                GetStaticEntry(ANDROID_SENSOR_INFO_MAX_FRAME_DURATION);

        EXPECT_EQ(1u, maxFrameDuration.count) << "No valid maximum frame duration";

        if (maxFrameDuration.count == 1) {
            maxDuration = maxFrameDuration.data.i64[0];
        }

        EXPECT_GT(maxDuration, 0) << "Max duration is 0 or not given, using fallback";

        if (maxDuration == 0) {
            maxDuration = 10 * SEC; // Fall back to 10 seconds as max duration
        }

    }
    dout << "Available frame duration range for configured stream size: "
         << minDuration << " - " << maxDuration << " ns" << std::endl;

    // Get environment variables if set
    const char *expVal = getenv(expEnv);
    const char *durationVal = getenv(durationEnv);
    const char *sensitivityVal = getenv(sensitivityEnv);

    bool gotExp = (expVal != NULL);
    bool gotDuration = (durationVal != NULL);
    bool gotSensitivity = (sensitivityVal != NULL);

    // All or none must be provided if using override envs
    ASSERT_TRUE( (gotDuration && gotExp && gotSensitivity) ||
            (!gotDuration && !gotExp && !gotSensitivity) ) <<
            "Incomplete set of environment variable overrides provided";

    Vector<int64_t> expList, durationList;
    Vector<int32_t> sensitivityList;
    if (gotExp) {
        ParseList(expVal, expList);
        ParseList(durationVal, durationList);
        ParseList(sensitivityVal, sensitivityList);

        ASSERT_TRUE(
            (expList.size() == durationList.size()) &&
            (durationList.size() == sensitivityList.size())) <<
                "Mismatched sizes in env lists, or parse error";

        dout << "Using burst list from environment with " << expList.size() <<
                " captures" << std::endl;
    } else {
        // Create a default set of controls based on the available ranges

        int64_t e;
        int64_t d;
        int32_t s;

        // Exposure ramp

        e = minExp;
        d = minDuration;
        s = sensitivities[0];
        while (e < maxExp) {
            expList.push_back(e);
            durationList.push_back(d);
            sensitivityList.push_back(s);
            e = e * 2;
        }
        e = maxExp;
        expList.push_back(e);
        durationList.push_back(d);
        sensitivityList.push_back(s);

        // Duration ramp

        e = 30 * MSEC;
        d = minDuration;
        s = sensitivities[0];
        while (d < maxDuration) {
            // make sure exposure <= frame duration
            expList.push_back(e > d ? d : e);
            durationList.push_back(d);
            sensitivityList.push_back(s);
            d = d * 2;
        }

        // Sensitivity ramp

        e = 30 * MSEC;
        d = 30 * MSEC;
        d = d > minDuration ? d : minDuration;
        for (size_t i = 0; i < sensitivities.size(); i++) {
            expList.push_back(e);
            durationList.push_back(d);
            sensitivityList.push_back(sensitivities[i]);
        }

        // Constant-EV ramp, duration == exposure

        e = 30 * MSEC; // at ISO 100
        for (size_t i = 0; i < sensitivities.size(); i++) {
            int64_t e_adj = e * 100 / sensitivities[i];
            expList.push_back(e_adj);
            durationList.push_back(e_adj > minDuration ? e_adj : minDuration);
            sensitivityList.push_back(sensitivities[i]);
        }

        dout << "Default burst sequence created with " << expList.size() <<
                " entries" << std::endl;
    }

    // Validate the list, but warn only
    for (size_t i = 0; i < expList.size(); i++) {
        EXPECT_GE(maxExp, expList[i])
                << "Capture " << i << " exposure too long: " << expList[i];
        EXPECT_LE(minExp, expList[i])
                << "Capture " << i << " exposure too short: " << expList[i];
        EXPECT_GE(maxDuration, durationList[i])
                << "Capture " << i << " duration too long: " << durationList[i];
        EXPECT_LE(minDuration, durationList[i])
                 << "Capture " << i << " duration too short: "  << durationList[i];
        bool validSensitivity = false;
        for (size_t j = 0; j < sensitivities.size(); j++) {
            if (sensitivityList[i] == sensitivities[j]) {
                validSensitivity = true;
                break;
            }
        }
        EXPECT_TRUE(validSensitivity)
                << "Capture " << i << " sensitivity not in list: " << sensitivityList[i];
    }

    // Check if debug yuv dumps are requested

    bool dumpFrames = false;
    {
        const char *frameDumpVal = getenv(dumpFrameEnv);
        if (frameDumpVal != NULL) {
            if (frameDumpVal[0] == '1') dumpFrames = true;
        }
    }

    dout << "Dumping YUV frames " <<
            (dumpFrames ? "enabled, not checking timing" : "disabled") << std::endl;

    // Create a base preview request, turning off all 3A
    CameraMetadata previewRequest;
    ASSERT_EQ(OK, mDevice->createDefaultRequest(CAMERA2_TEMPLATE_PREVIEW,
                                                &previewRequest));
    {
        Vector<int32_t> outputStreamIds;
        outputStreamIds.push(mStreamId);
        ASSERT_EQ(OK, previewRequest.update(ANDROID_REQUEST_OUTPUT_STREAMS,
                                            outputStreamIds));

        // Disable all 3A routines
        uint8_t cmOff = static_cast<uint8_t>(ANDROID_CONTROL_MODE_OFF);
        ASSERT_EQ(OK, previewRequest.update(ANDROID_CONTROL_MODE,
                                            &cmOff, 1));

        int requestId = 1;
        ASSERT_EQ(OK, previewRequest.update(ANDROID_REQUEST_ID,
                                            &requestId, 1));
    }

    // Submit capture requests

    for (size_t i = 0; i < expList.size(); ++i) {
        CameraMetadata tmpRequest = previewRequest;
        ASSERT_EQ(OK, tmpRequest.update(ANDROID_SENSOR_EXPOSURE_TIME,
                                        &expList[i], 1));
        ASSERT_EQ(OK, tmpRequest.update(ANDROID_SENSOR_FRAME_DURATION,
                                        &durationList[i], 1));
        ASSERT_EQ(OK, tmpRequest.update(ANDROID_SENSOR_SENSITIVITY,
                                        &sensitivityList[i], 1));
        ALOGV("Submitting capture %zu with exposure %"PRId64", frame duration %"PRId64", sensitivity %d",
                i, expList[i], durationList[i], sensitivityList[i]);
        dout << "Capture request " << i <<
                ": exposure is " << (expList[i]/1e6f) << " ms" <<
                ", frame duration is " << (durationList[i]/1e6f) << " ms" <<
                ", sensitivity is " << sensitivityList[i] <<
                std::endl;
        ASSERT_EQ(OK, mDevice->capture(tmpRequest));
    }

    Vector<float> brightnesses;
    Vector<nsecs_t> captureTimes;
    brightnesses.setCapacity(expList.size());
    captureTimes.setCapacity(expList.size());

    // Get each frame (metadata) and then the buffer. Calculate brightness.
    for (size_t i = 0; i < expList.size(); ++i) {

        ALOGV("Reading request %zu", i);
        dout << "Waiting for capture " << i << ": " <<
                " exposure " << (expList[i]/1e6f) << " ms," <<
                " frame duration " << (durationList[i]/1e6f) << " ms," <<
                " sensitivity " << sensitivityList[i] <<
                std::endl;

        // Set wait limit based on expected frame duration, or minimum timeout
        int64_t waitLimit = CAMERA_FRAME_TIMEOUT;
        if (expList[i] * 2 > waitLimit) waitLimit = expList[i] * 2;
        if (durationList[i] * 2 > waitLimit) waitLimit = durationList[i] * 2;

        ASSERT_EQ(OK, mDevice->waitForNextFrame(waitLimit));
        ALOGV("Reading capture request-1 %zu", i);
        CaptureResult result;
        ASSERT_EQ(OK, mDevice->getNextResult(&result));
        ALOGV("Reading capture request-2 %zu", i);

        ASSERT_EQ(OK, mFrameListener->waitForFrame(CAMERA_FRAME_TIMEOUT));
        ALOGV("We got the frame now");

        captureTimes.push_back(systemTime());

        CpuConsumer::LockedBuffer imgBuffer;
        ASSERT_EQ(OK, mCpuConsumer->lockNextBuffer(&imgBuffer));

        int underexposed, overexposed;
        float avgBrightness = 0;
        long long brightness = TotalBrightness(imgBuffer, &underexposed,
                                               &overexposed);
        int numValidPixels = mWidth * mHeight - (underexposed + overexposed);
        if (numValidPixels != 0) {
            avgBrightness = brightness * 1.0f / numValidPixels;
        } else if (underexposed < overexposed) {
            avgBrightness = 255;
        }

        ALOGV("Total brightness for frame %zu was %lld (underexposed %d, "
              "overexposed %d), avg %f", i, brightness, underexposed,
              overexposed, avgBrightness);
        dout << "Average brightness (frame " << i << ") was " << avgBrightness
             << " (underexposed " << underexposed << ", overexposed "
             << overexposed << ")" << std::endl;
        brightnesses.push_back(avgBrightness);

        if (i != 0) {
            float prevEv = static_cast<float>(expList[i - 1]) * sensitivityList[i - 1];
            float currentEv = static_cast<float>(expList[i]) * sensitivityList[i];
            float evRatio = (prevEv > currentEv) ? (currentEv / prevEv) :
                    (prevEv / currentEv);
            if ( evRatio > EV_MATCH_BOUND ) {
                WARN_LT(fabs(brightnesses[i] - brightnesses[i - 1]),
                        BRIGHTNESS_MATCH_BOUND) <<
                        "Capture brightness different from previous, even though "
                        "they have the same EV value. Ev now: " << currentEv <<
                        ", previous: " << prevEv << ". Brightness now: " <<
                        brightnesses[i] << ", previous: " << brightnesses[i-1] <<
                        std::endl;
            }
            // Only check timing if not saving to disk, since that slows things
            // down substantially
            if (!dumpFrames) {
                nsecs_t timeDelta = captureTimes[i] - captureTimes[i-1];
                nsecs_t expectedDelta = expList[i] > durationList[i] ?
                        expList[i] : durationList[i];
                WARN_LT(timeDelta, expectedDelta + DURATION_UPPER_BOUND) <<
                        "Capture took " << timeDelta << " ns to receive, but expected"
                        " frame duration was " << expectedDelta << " ns." <<
                        std::endl;
                WARN_GT(timeDelta, expectedDelta - DURATION_LOWER_BOUND) <<
                        "Capture took " << timeDelta << " ns to receive, but expected"
                        " frame duration was " << expectedDelta << " ns." <<
                        std::endl;
                dout << "Time delta from previous frame: " << timeDelta / 1e6 <<
                        " ms.  Expected " << expectedDelta / 1e6 << " ms" << std::endl;
            }
        }

        if (dumpFrames) {
            String8 dumpName =
                    String8::format("/data/local/tmp/camera2_test_variable_burst_frame_%03zu.yuv", i);
            dout << "  Writing YUV dump to " << dumpName << std::endl;
            DumpYuvToFile(dumpName, imgBuffer);
        }

        ASSERT_EQ(OK, mCpuConsumer->unlockBuffer(imgBuffer));
    }

}

}
}
}
