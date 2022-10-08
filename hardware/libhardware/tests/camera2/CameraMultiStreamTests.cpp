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

#include <inttypes.h>
#define LOG_TAG "CameraMultiStreamTest"
//#define LOG_NDEBUG 0
#include "CameraStreamFixture.h"
#include "TestExtensions.h"

#include <gtest/gtest.h>
#include <utils/Log.h>
#include <utils/StrongPointer.h>
#include <common/CameraDeviceBase.h>
#include <hardware/hardware.h>
#include <hardware/camera2.h>
#include <gui/SurfaceComposerClient.h>
#include <gui/Surface.h>

#define DEFAULT_FRAME_DURATION 33000000LL // 33ms
#define CAMERA_HEAP_COUNT       1
#define CAMERA_EXPOSURE_FORMAT CAMERA_STREAM_AUTO_CPU_FORMAT
#define CAMERA_DISPLAY_FORMAT HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED
#define CAMERA_MULTI_STREAM_DEBUGGING  0
#define CAMERA_FRAME_TIMEOUT    1000000000LL // nsecs (1 secs)
#define PREVIEW_RENDERING_TIME_INTERVAL 200000 // in unit of us, 200ms
// 1% tolerance margin for exposure sanity check against metadata
#define TOLERANCE_MARGIN_METADATA 0.01
// 5% tolerance margin for exposure sanity check against capture times
#define TOLERANCE_MARGIN_CAPTURE 0.05
/* constants for display */
#define DISPLAY_BUFFER_HEIGHT 1024
#define DISPLAY_BUFFER_WIDTH 1024
#define DISPLAY_BUFFER_FORMAT PIXEL_FORMAT_RGB_888

// This test intends to test large preview size but less than 1080p.
#define PREVIEW_WIDTH_CAP   1920
#define PREVIEW_HEIGHT_CAP  1080
// This test intends to test small metering burst size that is less than 640x480
#define METERING_WIDTH_CAP  640
#define METERING_HEIGHT_CAP 480

#define EXP_WAIT_MULTIPLIER 2

namespace android {
namespace camera2 {
namespace tests {

static const CameraStreamParams DEFAULT_STREAM_PARAMETERS = {
    /*mFormat*/     CAMERA_EXPOSURE_FORMAT,
    /*mHeapCount*/  CAMERA_HEAP_COUNT
};

static const CameraStreamParams DISPLAY_STREAM_PARAMETERS = {
    /*mFormat*/     CAMERA_DISPLAY_FORMAT,
    /*mHeapCount*/  CAMERA_HEAP_COUNT
};

class CameraMultiStreamTest
    : public ::testing::Test,
      public CameraStreamFixture {

public:
    CameraMultiStreamTest() : CameraStreamFixture(DEFAULT_STREAM_PARAMETERS) {
        TEST_EXTENSION_FORKING_CONSTRUCTOR;

        if (HasFatalFailure()) {
            return;
        }
        /**
         * Don't create default stream, each test is in charge of creating
         * its own streams.
         */
    }

    ~CameraMultiStreamTest() {
        TEST_EXTENSION_FORKING_DESTRUCTOR;
    }

    sp<SurfaceComposerClient> mComposerClient;
    sp<SurfaceControl> mSurfaceControl;

    void CreateOnScreenSurface(sp<ANativeWindow>& surface) {
        mComposerClient = new SurfaceComposerClient;
        ASSERT_EQ(NO_ERROR, mComposerClient->initCheck());

        mSurfaceControl = mComposerClient->createSurface(
                String8("CameraMultiStreamTest StreamingImage Surface"),
                DISPLAY_BUFFER_HEIGHT, DISPLAY_BUFFER_WIDTH,
                DISPLAY_BUFFER_FORMAT, 0);

        ASSERT_NE((void*)NULL, mSurfaceControl.get());
        ASSERT_TRUE(mSurfaceControl->isValid());

        SurfaceComposerClient::openGlobalTransaction();
        ASSERT_EQ(NO_ERROR, mSurfaceControl->setLayer(0x7FFFFFFF));
        ASSERT_EQ(NO_ERROR, mSurfaceControl->show());
        SurfaceComposerClient::closeGlobalTransaction();

        surface = mSurfaceControl->getSurface();

        ASSERT_NE((void*)NULL, surface.get());
    }

    struct Size {
        int32_t width;
        int32_t height;
    };

    // Select minimal size by number of pixels.
    void GetMinSize(const int32_t* data, size_t count,
            Size* min, int32_t* idx) {
        ASSERT_NE((int32_t*)NULL, data);
        int32_t minIdx = 0;
        int32_t minSize = INT_MAX, tempSize;
        for (size_t i = 0; i < count; i+=2) {
            tempSize = data[i] * data[i+1];
            if (minSize > tempSize) {
                minSize = tempSize;
                minIdx = i;
            }
        }
        min->width = data[minIdx];
        min->height = data[minIdx + 1];
        *idx = minIdx;
    }

    // Select maximal size by number of pixels.
    void GetMaxSize(const int32_t* data, size_t count,
            Size* max, int32_t* idx) {
        ASSERT_NE((int32_t*)NULL, data);
        int32_t maxIdx = 0;
        int32_t maxSize = INT_MIN, tempSize;
        for (size_t i = 0; i < count; i+=2) {
            tempSize = data[i] * data[i+1];
            if (maxSize < tempSize) {
                maxSize = tempSize;
                maxIdx = i;
            }
        }
        max->width = data[maxIdx];
        max->height = data[maxIdx + 1];
        *idx = maxIdx;
    }

    // Cap size by number of pixels.
    Size CapSize(Size cap, Size input) {
        if (input.width * input.height > cap.width * cap.height) {
            return cap;
        }
        return input;
    }

    struct CameraStream : public RefBase {

    public:
        /**
         * Only initialize the variables here, do the ASSERT check in
         * SetUp function. To make this stream useful, the SetUp must
         * be called before using it.
         */
        CameraStream(
                int width,
                int height,
                const sp<CameraDeviceBase>& device,
                CameraStreamParams param, sp<ANativeWindow> surface,
                bool useCpuConsumer)
            : mDevice(device),
              mWidth(width),
              mHeight(height) {
            mFormat = param.mFormat;
            if (useCpuConsumer) {
                sp<IGraphicBufferProducer> producer;
                sp<IGraphicBufferConsumer> consumer;
                BufferQueue::createBufferQueue(&producer, &consumer);
                mCpuConsumer = new CpuConsumer(consumer, param.mHeapCount);
                mCpuConsumer->setName(String8(
                        "CameraMultiStreamTest::mCpuConsumer"));
                mNativeWindow = new Surface(producer);
            } else {
                // Render the stream to screen.
                mCpuConsumer = NULL;
                mNativeWindow = surface;
            }

            mFrameListener = new FrameListener();
            if (mCpuConsumer != 0) {
                mCpuConsumer->setFrameAvailableListener(mFrameListener);
            }
        }

        /**
         * Finally create camera stream, and do the ASSERT check, since we
         * can not do it in ctor.
         */
        void SetUp() {
            ASSERT_EQ(OK,
                mDevice->createStream(mNativeWindow,
                    mWidth, mHeight, mFormat,
                    &mStreamId));

            ASSERT_NE(-1, mStreamId);
        }

        int GetStreamId() { return mStreamId; }
        sp<CpuConsumer> GetConsumer() { return mCpuConsumer; }
        sp<FrameListener> GetFrameListener() { return mFrameListener; }

    protected:
        ~CameraStream() {
            if (mDevice.get()) {
                mDevice->waitUntilDrained();
                mDevice->deleteStream(mStreamId);
            }
            // Clear producer before consumer.
            mNativeWindow.clear();
            mCpuConsumer.clear();
        }

    private:
        sp<FrameListener>       mFrameListener;
        sp<CpuConsumer>         mCpuConsumer;
        sp<ANativeWindow>       mNativeWindow;
        sp<CameraDeviceBase>    mDevice;
        int                     mStreamId;
        int                     mWidth;
        int                     mHeight;
        int                     mFormat;
    };

    int64_t GetExposureValue(const CameraMetadata& metaData) {
        camera_metadata_ro_entry_t entry =
                metaData.find(ANDROID_SENSOR_EXPOSURE_TIME);
        EXPECT_EQ(1u, entry.count);
        if (entry.count == 1) {
            return entry.data.i64[0];
        }
        return -1;
    }

    int32_t GetSensitivity(const CameraMetadata& metaData) {
        camera_metadata_ro_entry_t entry =
                metaData.find(ANDROID_SENSOR_SENSITIVITY);
        EXPECT_EQ(1u, entry.count);
        if (entry.count == 1) {
            return entry.data.i32[0];
        }
        return -1;
    }

    int64_t GetFrameDuration(const CameraMetadata& metaData) {
        camera_metadata_ro_entry_t entry =
                metaData.find(ANDROID_SENSOR_FRAME_DURATION);
        EXPECT_EQ(1u, entry.count);
        if (entry.count == 1) {
            return entry.data.i64[0];
        }
        return -1;
    }

    void CreateRequests(CameraMetadata& previewRequest,
            CameraMetadata& meteringRequest,
            CameraMetadata& captureRequest,
            int previewStreamId,
            int meteringStreamId,
            int captureStreamId) {
        int32_t requestId = 0;
        Vector<int32_t> previewStreamIds;
        previewStreamIds.push(previewStreamId);
        ASSERT_EQ(OK, mDevice->createDefaultRequest(CAMERA2_TEMPLATE_PREVIEW,
                &previewRequest));
        ASSERT_EQ(OK, previewRequest.update(ANDROID_REQUEST_OUTPUT_STREAMS,
                previewStreamIds));
        ASSERT_EQ(OK, previewRequest.update(ANDROID_REQUEST_ID,
                &requestId, 1));

        // Create metering request, manual settings
        // Manual control: Disable 3A, noise reduction, edge sharping
        uint8_t cmOff = static_cast<uint8_t>(ANDROID_CONTROL_MODE_OFF);
        uint8_t nrOff = static_cast<uint8_t>(ANDROID_NOISE_REDUCTION_MODE_OFF);
        uint8_t sharpOff = static_cast<uint8_t>(ANDROID_EDGE_MODE_OFF);
        Vector<int32_t> meteringStreamIds;
        meteringStreamIds.push(meteringStreamId);
        ASSERT_EQ(OK, mDevice->createDefaultRequest(
                CAMERA2_TEMPLATE_PREVIEW,
                &meteringRequest));
        ASSERT_EQ(OK, meteringRequest.update(
                ANDROID_REQUEST_OUTPUT_STREAMS,
                meteringStreamIds));
        ASSERT_EQ(OK, meteringRequest.update(
                ANDROID_CONTROL_MODE,
                &cmOff, 1));
        ASSERT_EQ(OK, meteringRequest.update(
                ANDROID_NOISE_REDUCTION_MODE,
                &nrOff, 1));
        ASSERT_EQ(OK, meteringRequest.update(
                ANDROID_EDGE_MODE,
                &sharpOff, 1));

        // Create capture request, manual settings
        Vector<int32_t> captureStreamIds;
        captureStreamIds.push(captureStreamId);
        ASSERT_EQ(OK, mDevice->createDefaultRequest(
                CAMERA2_TEMPLATE_PREVIEW,
                &captureRequest));
        ASSERT_EQ(OK, captureRequest.update(
                ANDROID_REQUEST_OUTPUT_STREAMS,
                captureStreamIds));
        ASSERT_EQ(OK, captureRequest.update(
                ANDROID_CONTROL_MODE,
                &cmOff, 1));
        ASSERT_EQ(OK, captureRequest.update(
                ANDROID_NOISE_REDUCTION_MODE,
                &nrOff, 1));
        ASSERT_EQ(OK, captureRequest.update(
                ANDROID_EDGE_MODE,
                &sharpOff, 1));
    }

    sp<CameraStream> CreateStream(
            int width,
            int height,
            const sp<CameraDeviceBase>& device,
            CameraStreamParams param = DEFAULT_STREAM_PARAMETERS,
            sp<ANativeWindow> surface = NULL,
            bool useCpuConsumer = true) {
        param.mFormat = MapAutoFormat(param.mFormat);
        return new CameraStream(width, height, device,
                param, surface, useCpuConsumer);
    }

    void CaptureBurst(CameraMetadata& request, size_t requestCount,
            const Vector<int64_t>& exposures,
            const Vector<int32_t>& sensitivities,
            const sp<CameraStream>& stream,
            int64_t minFrameDuration,
            int32_t* requestIdStart) {
        ASSERT_EQ(OK, request.update(ANDROID_SENSOR_FRAME_DURATION,
                &minFrameDuration, 1));
        // Submit a series of requests with the specified exposure/gain values.
        int32_t targetRequestId = *requestIdStart;
        for (size_t i = 0; i < requestCount; i++) {
            ASSERT_EQ(OK, request.update(ANDROID_REQUEST_ID, requestIdStart, 1));
            ASSERT_EQ(OK, request.update(ANDROID_SENSOR_EXPOSURE_TIME, &exposures[i], 1));
            ASSERT_EQ(OK, request.update(ANDROID_SENSOR_SENSITIVITY, &sensitivities[i], 1));
            ASSERT_EQ(OK, mDevice->capture(request));
            ALOGV("Submitting request with: id %d with exposure %"PRId64", sensitivity %d",
                    *requestIdStart, exposures[i], sensitivities[i]);
            if (CAMERA_MULTI_STREAM_DEBUGGING) {
                request.dump(STDOUT_FILENO);
            }
            (*requestIdStart)++;
        }
        // Get capture burst results.
        Vector<nsecs_t> captureBurstTimes;
        sp<CpuConsumer> consumer = stream->GetConsumer();
        sp<FrameListener> listener = stream->GetFrameListener();

        // Set wait limit based on expected frame duration.
        int64_t waitLimit = CAMERA_FRAME_TIMEOUT;
        for (size_t i = 0; i < requestCount; i++) {
            ALOGV("Reading request result %zu", i);

            /**
             * Raise the timeout to be at least twice as long as the exposure
             * time. to avoid a false positive when the timeout is too short.
             */
            if ((exposures[i] * EXP_WAIT_MULTIPLIER) > waitLimit) {
                waitLimit = exposures[i] * EXP_WAIT_MULTIPLIER;
            }

            CaptureResult result;
            CameraMetadata frameMetadata;
            int32_t resultRequestId;
            do {
                ASSERT_EQ(OK, mDevice->waitForNextFrame(waitLimit));
                ASSERT_EQ(OK, mDevice->getNextResult(&result));
                frameMetadata = result.mMetadata;

                camera_metadata_entry_t resultEntry = frameMetadata.find(ANDROID_REQUEST_ID);
                ASSERT_EQ(1u, resultEntry.count);
                resultRequestId = resultEntry.data.i32[0];
                if (CAMERA_MULTI_STREAM_DEBUGGING) {
                    std::cout << "capture result req id: " << resultRequestId << std::endl;
                }
            } while (resultRequestId != targetRequestId);
            targetRequestId++;
            ALOGV("Got capture burst result for request %zu", i);

            // Validate capture result
            if (CAMERA_MULTI_STREAM_DEBUGGING) {
                frameMetadata.dump(STDOUT_FILENO);
            }

            // TODO: Need revisit it to figure out an accurate margin.
            int64_t resultExposure = GetExposureValue(frameMetadata);
            int32_t resultSensitivity = GetSensitivity(frameMetadata);
            EXPECT_LE(sensitivities[i] * (1.0 - TOLERANCE_MARGIN_METADATA), resultSensitivity);
            EXPECT_GE(sensitivities[i] * (1.0 + TOLERANCE_MARGIN_METADATA), resultSensitivity);
            EXPECT_LE(exposures[i] * (1.0 - TOLERANCE_MARGIN_METADATA), resultExposure);
            EXPECT_GE(exposures[i] * (1.0 + TOLERANCE_MARGIN_METADATA), resultExposure);

            ASSERT_EQ(OK, listener->waitForFrame(waitLimit));
            captureBurstTimes.push_back(systemTime());
            CpuConsumer::LockedBuffer imgBuffer;
            ASSERT_EQ(OK, consumer->lockNextBuffer(&imgBuffer));
            ALOGV("Got capture buffer for request %zu", i);

            /**
             * TODO: Validate capture buffer. Current brightness calculation
             * is too slow, it also doesn't account for saturation effects,
             * which is quite common since we are going over a significant
             * range of EVs. we need figure out some reliable way to validate
             * buffer data.
             */

            ASSERT_EQ(OK, consumer->unlockBuffer(imgBuffer));
            if (i > 0) {
                nsecs_t timeDelta =
                        captureBurstTimes[i] - captureBurstTimes[i-1];
                EXPECT_GE(timeDelta * ( 1 + TOLERANCE_MARGIN_CAPTURE), exposures[i]);
            }
        }
    }

    /**
     * Intentionally shadow default CreateStream function from base class,
     * because we don't want any test in this class to use the default
     * stream creation function.
     */
    void CreateStream() {
    }
};

/**
 * This test adds multiple stream use case test, basically, test 3
 * streams:
 *
 * 1. Preview stream, with large size that is no bigger than 1080p
 * we render this stream to display and vary the exposure time for
 * for certain amount of time for visualization purpose.
 *
 * 2. Metering stream, with small size that is no bigger than VGA size.
 * a burst is issued for different exposure times and analog gains
 * (or analog gain implemented sensitivities) then check if the capture
 * result metadata matches the request.
 *
 * 3. Capture stream, this is basically similar as meterting stream, but
 * has large size, which is the largest supported JPEG capture size.
 *
 * This multiple stream test is to test if HAL supports:
 *
 * 1. Multiple streams like above, HAL should support at least 3 streams
 * concurrently: one preview stream, 2 other YUV stream.
 *
 * 2. Manual control(gain/exposure) of mutiple burst capture.
 */
// Disable this test for now, as we need cleanup the usage of the deprecated tag quite a bit.
TEST_F(CameraMultiStreamTest, DISABLED_MultiBurst) {

    TEST_EXTENSION_FORKING_INIT;

    const int32_t* implDefData;
    size_t implDefCount;
    const int32_t* jpegData;
    size_t jpegCount;
    if (getDeviceVersion() < CAMERA_DEVICE_API_VERSION_3_2) {
        camera_metadata_ro_entry availableProcessedSizes =
            GetStaticEntry(ANDROID_SCALER_AVAILABLE_PROCESSED_SIZES);
        ASSERT_EQ(0u, availableProcessedSizes.count % 2);
        ASSERT_GE(availableProcessedSizes.count, 2u);
        camera_metadata_ro_entry availableProcessedMinFrameDurations =
            GetStaticEntry(ANDROID_SCALER_AVAILABLE_PROCESSED_MIN_DURATIONS);
        EXPECT_EQ(availableProcessedSizes.count,
            availableProcessedMinFrameDurations.count * 2);

        camera_metadata_ro_entry availableJpegSizes =
            GetStaticEntry(ANDROID_SCALER_AVAILABLE_JPEG_SIZES);
        ASSERT_EQ(0u, availableJpegSizes.count % 2);
        ASSERT_GE(availableJpegSizes.count, 2u);
        implDefData = availableProcessedSizes.data.i32;
        implDefCount = availableProcessedSizes.count;
        jpegData = availableJpegSizes.data.i32;
        jpegCount = availableJpegSizes.count;
    } else {
        const int32_t *implDefResolutions;
        size_t   implDefResolutionsCount;

        getResolutionList(HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED, &implDefData, &implDefCount);
        ASSERT_NE(0u, implDefCount)
            << "Missing implementation defined sizes";
        ASSERT_EQ(0u, implDefCount % 2);
        ASSERT_GE(implDefCount, 2u);

        getResolutionList(HAL_PIXEL_FORMAT_BLOB, &jpegData, &jpegCount);
        ASSERT_EQ(0u, jpegCount % 2);
        ASSERT_GE(jpegCount, 2u);
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

    // Find the right sizes for preview, metering, and capture streams
    int64_t minFrameDuration = DEFAULT_FRAME_DURATION;
    Size processedMinSize, processedMaxSize, jpegMaxSize;

    int32_t minIdx, maxIdx;
    GetMinSize(implDefData, implDefCount, &processedMinSize, &minIdx);
    GetMaxSize(implDefData, implDefCount, &processedMaxSize, &maxIdx);
    ALOGV("Found processed max size: %dx%d, min size = %dx%d",
            processedMaxSize.width, processedMaxSize.height,
            processedMinSize.width, processedMinSize.height);

    if (getDeviceVersion() < CAMERA_DEVICE_API_VERSION_3_2) {
        camera_metadata_ro_entry availableProcessedMinFrameDurations =
            GetStaticEntry(ANDROID_SCALER_AVAILABLE_PROCESSED_MIN_DURATIONS);
        minFrameDuration =
            availableProcessedMinFrameDurations.data.i64[maxIdx / 2];
    } else {
        minFrameDuration = getMinFrameDurationFor(
                HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED,
                processedMaxSize.width, processedMaxSize.height);
    }

    EXPECT_GT(minFrameDuration, 0);

    if (minFrameDuration <= 0) {
        minFrameDuration = DEFAULT_FRAME_DURATION;
    }

    ALOGV("targeted minimal frame duration is: %"PRId64"ns", minFrameDuration);

    GetMaxSize(jpegData, jpegCount, &jpegMaxSize, &maxIdx);
    ALOGV("Found Jpeg size max idx = %d", maxIdx);

    // Max Jpeg size should be available in processed sizes. Use it for
    // YUV capture anyway.
    EXPECT_EQ(processedMaxSize.width, jpegMaxSize.width);
    EXPECT_EQ(processedMaxSize.height, jpegMaxSize.height);

    // Cap preview size.
    Size previewLimit = { PREVIEW_WIDTH_CAP, PREVIEW_HEIGHT_CAP };
    // FIXME: need make sure the previewLimit is supported by HAL.
    Size previewSize = CapSize(previewLimit, processedMaxSize);
    // Cap Metering size.
    Size meteringLimit = { METERING_WIDTH_CAP, METERING_HEIGHT_CAP };
    // Cap metering size to VGA (VGA is mandatory by CDD)
    Size meteringSize = CapSize(meteringLimit, processedMinSize);
    // Capture stream should be the max size of jpeg sizes.
    ALOGV("preview size: %dx%d, metering size: %dx%d, capture size: %dx%d",
            previewSize.width, previewSize.height,
            meteringSize.width, meteringSize.height,
            jpegMaxSize.width, jpegMaxSize.height);

    // Create streams
    // Preview stream: small resolution, render on the screen.
    sp<CameraStream> previewStream;
    {
        sp<ANativeWindow> surface;
        ASSERT_NO_FATAL_FAILURE(CreateOnScreenSurface(/*out*/surface));
        previewStream = CreateStream(
                previewSize.width,
                previewSize.height,
                mDevice,
                DISPLAY_STREAM_PARAMETERS,
                surface,
                false);
        ASSERT_NE((void*)NULL, previewStream.get());
        ASSERT_NO_FATAL_FAILURE(previewStream->SetUp());
    }
    // Metering burst stream: small resolution yuv stream
    sp<CameraStream> meteringStream =
            CreateStream(
                    meteringSize.width,
                    meteringSize.height,
                    mDevice);
    ASSERT_NE((void*)NULL, meteringStream.get());
    ASSERT_NO_FATAL_FAILURE(meteringStream->SetUp());
    // Capture burst stream: full resolution yuv stream
    sp<CameraStream> captureStream =
            CreateStream(
                    jpegMaxSize.width,
                    jpegMaxSize.height,
                    mDevice);
    ASSERT_NE((void*)NULL, captureStream.get());
    ASSERT_NO_FATAL_FAILURE(captureStream->SetUp());

    // Create Preview request.
    CameraMetadata previewRequest, meteringRequest, captureRequest;
    ASSERT_NO_FATAL_FAILURE(CreateRequests(previewRequest, meteringRequest,
            captureRequest, previewStream->GetStreamId(),
            meteringStream->GetStreamId(), captureStream->GetStreamId()));

    // Start preview
    if (CAMERA_MULTI_STREAM_DEBUGGING) {
        previewRequest.dump(STDOUT_FILENO);
    }

    // Generate exposure and sensitivity lists
    camera_metadata_ro_entry exposureTimeRange =
        GetStaticEntry(ANDROID_SENSOR_INFO_EXPOSURE_TIME_RANGE);
    ASSERT_EQ(exposureTimeRange.count, 2u);
    int64_t minExp = exposureTimeRange.data.i64[0];
    int64_t maxExp = exposureTimeRange.data.i64[1];
    ASSERT_GT(maxExp, minExp);

    camera_metadata_ro_entry sensivityRange =
        GetStaticEntry(ANDROID_SENSOR_INFO_SENSITIVITY_RANGE);
    ASSERT_EQ(2u, sensivityRange.count);
    int32_t minSensitivity = sensivityRange.data.i32[0];
    int32_t maxSensitivity = sensivityRange.data.i32[1];
    camera_metadata_ro_entry maxAnalogSenEntry =
            GetStaticEntry(ANDROID_SENSOR_MAX_ANALOG_SENSITIVITY);
    EXPECT_EQ(1u, maxAnalogSenEntry.count);
    int32_t maxAnalogSensitivity = maxAnalogSenEntry.data.i32[0];
    EXPECT_LE(maxAnalogSensitivity, maxSensitivity);
    // Only test the sensitivity implemented by analog gain.
    if (maxAnalogSensitivity > maxSensitivity) {
        // Fallback to maxSensitity
        maxAnalogSensitivity = maxSensitivity;
    }

    // sensitivity list, only include the sensitivities that are implemented
    // purely by analog gain if possible.
    Vector<int32_t> sensitivities;
    Vector<int64_t> exposures;
    size_t count = (maxAnalogSensitivity - minSensitivity + 99) / 100;
    sensitivities.push_back(minSensitivity);
    for (size_t i = 1; i < count; i++) {
        sensitivities.push_back(minSensitivity + i * 100);
    }
    sensitivities.push_back(maxAnalogSensitivity);
    ALOGV("Sensitivity Range: min=%d, max=%d", minSensitivity,
            maxAnalogSensitivity);
    int64_t exp = minExp;
    while (exp < maxExp) {
        exposures.push_back(exp);
        exp *= 2;
    }
    // Sweep the exposure value for preview, just for visual inspection purpose.
    uint8_t cmOff = static_cast<uint8_t>(ANDROID_CONTROL_MODE_OFF);
    for (size_t i = 0; i < exposures.size(); i++) {
        ASSERT_EQ(OK, previewRequest.update(
                ANDROID_CONTROL_MODE,
                &cmOff, 1));
        ASSERT_EQ(OK, previewRequest.update(
                ANDROID_SENSOR_EXPOSURE_TIME,
                &exposures[i], 1));
        ALOGV("Submitting preview request %zu with exposure %"PRId64,
                i, exposures[i]);

        ASSERT_EQ(OK, mDevice->setStreamingRequest(previewRequest));

        // Let preview run 200ms on screen for each exposure time.
        usleep(PREVIEW_RENDERING_TIME_INTERVAL);
    }

    size_t requestCount = sensitivities.size();
    if (requestCount > exposures.size()) {
        requestCount = exposures.size();
    }

    // To maintain the request id uniqueness (preview request id is 0), make burst capture start
    // request id 1 here.
    int32_t requestIdStart = 1;
    /**
     * Submit metering request, set default frame duration to minimal possible
     * value, we want the capture to run as fast as possible. HAL should adjust
     * the frame duration to minimal necessary value to support the requested
     * exposure value if exposure is larger than frame duration.
     */
    CaptureBurst(meteringRequest, requestCount, exposures, sensitivities,
            meteringStream, minFrameDuration, &requestIdStart);

    /**
     * Submit capture request, set default frame duration to minimal possible
     * value, we want the capture to run as fast as possible. HAL should adjust
     * the frame duration to minimal necessary value to support the requested
     * exposure value if exposure is larger than frame duration.
     */
    CaptureBurst(captureRequest, requestCount, exposures, sensitivities,
            captureStream, minFrameDuration, &requestIdStart);

    ASSERT_EQ(OK, mDevice->clearStreamingRequest());
}

}
}
}
