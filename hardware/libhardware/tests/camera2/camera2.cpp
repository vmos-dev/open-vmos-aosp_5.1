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

#define LOG_TAG "Camera2_test"
//#define LOG_NDEBUG 0

#include <utils/Log.h>
#include <gtest/gtest.h>
#include <iostream>
#include <fstream>

#include <utils/Vector.h>
#include <utils/KeyedVector.h>
#include <gui/CpuConsumer.h>
#include <ui/PixelFormat.h>
#include <system/camera_metadata.h>

#include "camera2_utils.h"
#include "TestExtensions.h"

namespace android {
namespace camera2 {
namespace tests {

class Camera2Test: public testing::Test {
  public:
    void SetUpModule() {
        int res;

        hw_module_t *module = NULL;
        res = hw_get_module(CAMERA_HARDWARE_MODULE_ID,
                (const hw_module_t **)&module);

        ASSERT_EQ(0, res)
                << "Failure opening camera hardware module: " << res;
        ASSERT_TRUE(NULL != module)
                << "No camera module was set by hw_get_module";

        IF_ALOGV() {
            std::cout << "  Camera module name: "
                    << module->name << std::endl;
            std::cout << "  Camera module author: "
                    << module->author << std::endl;
            std::cout << "  Camera module API version: 0x" << std::hex
                    << module->module_api_version << std::endl;
            std::cout << "  Camera module HAL API version: 0x" << std::hex
                    << module->hal_api_version << std::endl;
        }

        int16_t version2_0 = CAMERA_MODULE_API_VERSION_2_0;
        ASSERT_LE(version2_0, module->module_api_version)
                << "Camera module version is 0x"
                << std::hex << module->module_api_version
                << ", should be at least 2.0. (0x"
                << std::hex << CAMERA_MODULE_API_VERSION_2_0 << ")";

        sCameraModule = reinterpret_cast<camera_module_t*>(module);

        sNumCameras = sCameraModule->get_number_of_cameras();
        ASSERT_LT(0, sNumCameras) << "No camera devices available!";

        IF_ALOGV() {
            std::cout << "  Camera device count: " << sNumCameras << std::endl;
        }

        sCameraSupportsHal2 = new bool[sNumCameras];

        for (int i = 0; i < sNumCameras; i++) {
            camera_info info;
            res = sCameraModule->get_camera_info(i, &info);
            ASSERT_EQ(0, res)
                    << "Failure getting camera info for camera " << i;
            IF_ALOGV() {
                std::cout << "  Camera device: " << std::dec
                          << i << std::endl;;
                std::cout << "    Facing: " << std::dec
                          << info.facing  << std::endl;
                std::cout << "    Orientation: " << std::dec
                          << info.orientation  << std::endl;
                std::cout << "    Version: 0x" << std::hex <<
                        info.device_version  << std::endl;
            }
            if (info.device_version >= CAMERA_DEVICE_API_VERSION_2_0 &&
                    info.device_version < CAMERA_DEVICE_API_VERSION_3_0) {
                sCameraSupportsHal2[i] = true;
                ASSERT_TRUE(NULL != info.static_camera_characteristics);
                IF_ALOGV() {
                    std::cout << "    Static camera metadata:"  << std::endl;
                    dump_indented_camera_metadata(info.static_camera_characteristics,
                            0, 1, 6);
                }
            } else {
                sCameraSupportsHal2[i] = false;
            }
        }
    }

    void TearDownModule() {
        hw_module_t *module = reinterpret_cast<hw_module_t*>(sCameraModule);
        ASSERT_EQ(0, HWModuleHelpers::closeModule(module));
    }

    static const camera_module_t *getCameraModule() {
        return sCameraModule;
    }

    static int getNumCameras() {
        return sNumCameras;
    }

    static bool isHal2Supported(int id) {
        return sCameraSupportsHal2[id];
    }

    static camera2_device_t *openCameraDevice(int id) {
        ALOGV("Opening camera %d", id);
        if (NULL == sCameraSupportsHal2) return NULL;
        if (id >= sNumCameras) return NULL;
        if (!sCameraSupportsHal2[id]) return NULL;

        hw_device_t *device = NULL;
        const camera_module_t *cam_module = getCameraModule();
        if (cam_module == NULL) {
            return NULL;
        }

        char camId[10];
        int res;

        snprintf(camId, 10, "%d", id);
        res = cam_module->common.methods->open(
            (const hw_module_t*)cam_module,
            camId,
            &device);
        if (res != NO_ERROR || device == NULL) {
            return NULL;
        }
        camera2_device_t *cam_device =
                reinterpret_cast<camera2_device_t*>(device);
        return cam_device;
    }

    static status_t configureCameraDevice(camera2_device_t *dev,
            MetadataQueue &requestQueue,
            MetadataQueue  &frameQueue,
            NotifierListener &listener) {

        status_t err;

        err = dev->ops->set_request_queue_src_ops(dev,
                requestQueue.getToConsumerInterface());
        if (err != OK) return err;

        requestQueue.setFromConsumerInterface(dev);

        err = dev->ops->set_frame_queue_dst_ops(dev,
                frameQueue.getToProducerInterface());
        if (err != OK) return err;

        err = listener.getNotificationsFrom(dev);
        if (err != OK) return err;

        return OK;
    }

    static status_t closeCameraDevice(camera2_device_t **cam_dev) {
        int res;
        if (*cam_dev == NULL ) return OK;

        ALOGV("Closing camera %p", cam_dev);

        hw_device_t *dev = reinterpret_cast<hw_device_t *>(*cam_dev);
        res = dev->close(dev);
        *cam_dev = NULL;
        return res;
    }

    void setUpCamera(int id) {
        ASSERT_GT(sNumCameras, id);
        status_t res;

        if (mDevice != NULL) {
            closeCameraDevice(&mDevice);
        }
        mId = id;
        mDevice = openCameraDevice(mId);
        ASSERT_TRUE(NULL != mDevice) << "Failed to open camera device";

        camera_info info;
        res = sCameraModule->get_camera_info(id, &info);
        ASSERT_EQ(OK, res);

        mDeviceVersion = info.device_version;
        mStaticInfo = info.static_camera_characteristics;
        buildOutputResolutions();

        res = configureCameraDevice(mDevice,
                mRequests,
                mFrames,
                mNotifications);
        ASSERT_EQ(OK, res) << "Failure to configure camera device";

    }

    void setUpStream(sp<IGraphicBufferProducer> consumer,
            int width, int height, int format, int *id) {
        status_t res;

        StreamAdapter* stream = new StreamAdapter(consumer);

        ALOGV("Creating stream, format 0x%x, %d x %d", format, width, height);
        res = stream->connectToDevice(mDevice, width, height, format);
        ASSERT_EQ(NO_ERROR, res) << "Failed to connect to stream: "
                                 << strerror(-res);
        mStreams.push_back(stream);

        *id = stream->getId();
    }

    void disconnectStream(int id) {
        status_t res;
        unsigned int i=0;
        for (; i < mStreams.size(); i++) {
            if (mStreams[i]->getId() == id) {
                res = mStreams[i]->disconnect();
                ASSERT_EQ(NO_ERROR, res) <<
                        "Failed to disconnect stream " << id;
                break;
            }
        }
        ASSERT_GT(mStreams.size(), i) << "Stream id not found:" << id;
    }

    void buildOutputResolutions() {
        status_t res;
        if (mDeviceVersion < CAMERA_DEVICE_API_VERSION_3_2) {
            return;
        }
        if (mOutputResolutions.isEmpty()) {
            camera_metadata_ro_entry_t availableStrmConfigs;
            res = find_camera_metadata_ro_entry(mStaticInfo,
                    ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS,
                    &availableStrmConfigs);
            ASSERT_EQ(OK, res);
            ASSERT_EQ(0u, availableStrmConfigs.count % 4);
            for (uint32_t i = 0; i < availableStrmConfigs.count; i += 4) {
                int32_t format = availableStrmConfigs.data.i32[i];
                int32_t width = availableStrmConfigs.data.i32[i + 1];
                int32_t height = availableStrmConfigs.data.i32[i + 2];
                int32_t inOrOut = availableStrmConfigs.data.i32[i + 3];
                if (inOrOut == ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT) {
                    int index = mOutputResolutions.indexOfKey(format);
                    if (index < 0) {
                        index = mOutputResolutions.add(format, new Vector<int32_t>());
                        ASSERT_TRUE(index >= 0);
                    }
                    Vector<int32_t> *resolutions = mOutputResolutions.editValueAt(index);
                    resolutions->add(width);
                    resolutions->add(height);
                }
            }
        }
    }

    void deleteOutputResolutions() {
        for (uint32_t i = 0; i < mOutputResolutions.size(); i++) {
            Vector<int32_t>* resolutions = mOutputResolutions.editValueAt(i);
            delete resolutions;
        }
        mOutputResolutions.clear();
    }

    void getResolutionList(int32_t format,
            const int32_t **list,
            size_t *count) {
        status_t res;
        ALOGV("Getting resolutions for format %x", format);
        if (mDeviceVersion < CAMERA_DEVICE_API_VERSION_3_2) {
            if (format != HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED) {
                camera_metadata_ro_entry_t availableFormats;
                res = find_camera_metadata_ro_entry(mStaticInfo,
                        ANDROID_SCALER_AVAILABLE_FORMATS,
                        &availableFormats);
                ASSERT_EQ(OK, res);

                uint32_t formatIdx;
                for (formatIdx=0; formatIdx < availableFormats.count; formatIdx++) {
                    if (availableFormats.data.i32[formatIdx] == format) break;
                }
                ASSERT_NE(availableFormats.count, formatIdx)
                    << "No support found for format 0x" << std::hex << format;
            }

            camera_metadata_ro_entry_t availableSizes;
            if (format == HAL_PIXEL_FORMAT_RAW_SENSOR) {
                res = find_camera_metadata_ro_entry(mStaticInfo,
                        ANDROID_SCALER_AVAILABLE_RAW_SIZES,
                        &availableSizes);
            } else if (format == HAL_PIXEL_FORMAT_BLOB) {
                res = find_camera_metadata_ro_entry(mStaticInfo,
                        ANDROID_SCALER_AVAILABLE_JPEG_SIZES,
                        &availableSizes);
            } else {
                res = find_camera_metadata_ro_entry(mStaticInfo,
                        ANDROID_SCALER_AVAILABLE_PROCESSED_SIZES,
                        &availableSizes);
            }
            ASSERT_EQ(OK, res);

            *list = availableSizes.data.i32;
            *count = availableSizes.count;
        } else {
            int index = mOutputResolutions.indexOfKey(format);
            ASSERT_TRUE(index >= 0);
            Vector<int32_t>* resolutions = mOutputResolutions.valueAt(index);
            *list = resolutions->array();
            *count = resolutions->size();
        }
    }

    status_t waitUntilDrained() {
        static const uint32_t kSleepTime = 50000; // 50 ms
        static const uint32_t kMaxSleepTime = 10000000; // 10 s
        ALOGV("%s: Camera %d: Starting wait", __FUNCTION__, mId);

        // TODO: Set up notifications from HAL, instead of sleeping here
        uint32_t totalTime = 0;
        while (mDevice->ops->get_in_progress_count(mDevice) > 0) {
            usleep(kSleepTime);
            totalTime += kSleepTime;
            if (totalTime > kMaxSleepTime) {
                ALOGE("%s: Waited %d us, %d requests still in flight", __FUNCTION__,
                        mDevice->ops->get_in_progress_count(mDevice), totalTime);
                return TIMED_OUT;
            }
        }
        ALOGV("%s: Camera %d: HAL is idle", __FUNCTION__, mId);
        return OK;
    }

    virtual void SetUp() {
        TEST_EXTENSION_FORKING_SET_UP;

        SetUpModule();

        const ::testing::TestInfo* const testInfo =
                ::testing::UnitTest::GetInstance()->current_test_info();
        (void)testInfo;

        ALOGV("*** Starting test %s in test case %s", testInfo->name(),
              testInfo->test_case_name());
        mDevice = NULL;
    }

    virtual void TearDown() {
        TEST_EXTENSION_FORKING_TEAR_DOWN;

        for (unsigned int i = 0; i < mStreams.size(); i++) {
            delete mStreams[i];
        }
        if (mDevice != NULL) {
            closeCameraDevice(&mDevice);
        }

        deleteOutputResolutions();
        TearDownModule();
    }

    int mId;
    camera2_device    *mDevice;
    uint32_t mDeviceVersion;
    const camera_metadata_t *mStaticInfo;

    MetadataQueue    mRequests;
    MetadataQueue    mFrames;
    NotifierListener mNotifications;

    Vector<StreamAdapter*> mStreams;
    KeyedVector<int32_t, Vector<int32_t>* > mOutputResolutions;

  private:
    static camera_module_t *sCameraModule;
    static int              sNumCameras;
    static bool            *sCameraSupportsHal2;

};

camera_module_t *Camera2Test::sCameraModule = NULL;
bool *Camera2Test::sCameraSupportsHal2      = NULL;
int Camera2Test::sNumCameras                = 0;

static const nsecs_t USEC = 1000;
static const nsecs_t MSEC = 1000*USEC;
static const nsecs_t SEC = 1000*MSEC;


TEST_F(Camera2Test, OpenClose) {

    TEST_EXTENSION_FORKING_INIT;

    status_t res;

    for (int id = 0; id < getNumCameras(); id++) {
        if (!isHal2Supported(id)) continue;

        camera2_device_t *d = openCameraDevice(id);
        ASSERT_TRUE(NULL != d) << "Failed to open camera device";

        res = closeCameraDevice(&d);
        ASSERT_EQ(NO_ERROR, res) << "Failed to close camera device";
    }
}

TEST_F(Camera2Test, Capture1Raw) {

    TEST_EXTENSION_FORKING_INIT;

    status_t res;

    for (int id = 0; id < getNumCameras(); id++) {
        if (!isHal2Supported(id)) continue;

        ASSERT_NO_FATAL_FAILURE(setUpCamera(id));

        sp<IGraphicBufferProducer> bqProducer;
        sp<IGraphicBufferConsumer> bqConsumer;
        BufferQueue::createBufferQueue(&bqProducer, &bqConsumer);
        sp<CpuConsumer> rawConsumer = new CpuConsumer(bqConsumer, 1);
        sp<FrameWaiter> rawWaiter = new FrameWaiter();
        rawConsumer->setFrameAvailableListener(rawWaiter);

        const int32_t *rawResolutions;
        size_t   rawResolutionsCount;

        int format = HAL_PIXEL_FORMAT_RAW_SENSOR;

        getResolutionList(format,
                &rawResolutions, &rawResolutionsCount);

        if (rawResolutionsCount <= 0) {
            const ::testing::TestInfo* const test_info =
                ::testing::UnitTest::GetInstance()->current_test_info();
            std::cerr << "Skipping test "
                      << test_info->test_case_name() << "."
                      << test_info->name()
                      << " because the optional format was not available: "
                      << "RAW_SENSOR" << std::endl;
            return;
        }

        ASSERT_LT((size_t)0, rawResolutionsCount);

        // Pick first available raw resolution
        int width = rawResolutions[0];
        int height = rawResolutions[1];

        int streamId;
        ASSERT_NO_FATAL_FAILURE(
            setUpStream(bqProducer, width, height, format, &streamId) );

        camera_metadata_t *request;
        request = allocate_camera_metadata(20, 2000);

        uint8_t metadataMode = ANDROID_REQUEST_METADATA_MODE_FULL;
        add_camera_metadata_entry(request,
                ANDROID_REQUEST_METADATA_MODE,
                (void**)&metadataMode, 1);
        uint32_t outputStreams = streamId;
        add_camera_metadata_entry(request,
                ANDROID_REQUEST_OUTPUT_STREAMS,
                (void**)&outputStreams, 1);

        uint64_t exposureTime = 10*MSEC;
        add_camera_metadata_entry(request,
                ANDROID_SENSOR_EXPOSURE_TIME,
                (void**)&exposureTime, 1);
        uint64_t frameDuration = 30*MSEC;
        add_camera_metadata_entry(request,
                ANDROID_SENSOR_FRAME_DURATION,
                (void**)&frameDuration, 1);
        uint32_t sensitivity = 100;
        add_camera_metadata_entry(request,
                ANDROID_SENSOR_SENSITIVITY,
                (void**)&sensitivity, 1);
        uint8_t requestType = ANDROID_REQUEST_TYPE_CAPTURE;
        add_camera_metadata_entry(request,
                ANDROID_REQUEST_TYPE,
                (void**)&requestType, 1);

        uint32_t hourOfDay = 12;
        add_camera_metadata_entry(request,
                0x80000000, // EMULATOR_HOUROFDAY
                &hourOfDay, 1);

        IF_ALOGV() {
            std::cout << "Input request: " << std::endl;
            dump_indented_camera_metadata(request, 0, 1, 2);
        }

        res = mRequests.enqueue(request);
        ASSERT_EQ(NO_ERROR, res) << "Can't enqueue request: " << strerror(-res);

        res = mFrames.waitForBuffer(exposureTime + SEC);
        ASSERT_EQ(NO_ERROR, res) << "No frame to get: " << strerror(-res);

        camera_metadata_t *frame;
        res = mFrames.dequeue(&frame);
        ASSERT_EQ(NO_ERROR, res);
        ASSERT_TRUE(frame != NULL);

        IF_ALOGV() {
            std::cout << "Output frame:" << std::endl;
            dump_indented_camera_metadata(frame, 0, 1, 2);
        }

        res = rawWaiter->waitForFrame(exposureTime + SEC);
        ASSERT_EQ(NO_ERROR, res);

        CpuConsumer::LockedBuffer buffer;
        res = rawConsumer->lockNextBuffer(&buffer);
        ASSERT_EQ(NO_ERROR, res);

        IF_ALOGV() {
            const char *dumpname =
                    "/data/local/tmp/camera2_test-capture1raw-dump.raw";
            ALOGV("Dumping raw buffer to %s", dumpname);
            // Write to file
            std::ofstream rawFile(dumpname);
            size_t bpp = 2;
            for (unsigned int y = 0; y < buffer.height; y++) {
                rawFile.write(
                        (const char *)(buffer.data + y * buffer.stride * bpp),
                        buffer.width * bpp);
            }
            rawFile.close();
        }

        res = rawConsumer->unlockBuffer(buffer);
        ASSERT_EQ(NO_ERROR, res);

        ASSERT_EQ(OK, waitUntilDrained());
        ASSERT_NO_FATAL_FAILURE(disconnectStream(streamId));

        res = closeCameraDevice(&mDevice);
        ASSERT_EQ(NO_ERROR, res) << "Failed to close camera device";

    }
}

TEST_F(Camera2Test, CaptureBurstRaw) {

    TEST_EXTENSION_FORKING_INIT;

    status_t res;

    for (int id = 0; id < getNumCameras(); id++) {
        if (!isHal2Supported(id)) continue;

        ASSERT_NO_FATAL_FAILURE(setUpCamera(id));

        sp<IGraphicBufferProducer> bqProducer;
        sp<IGraphicBufferConsumer> bqConsumer;
        BufferQueue::createBufferQueue(&bqProducer, &bqConsumer);
        sp<CpuConsumer> rawConsumer = new CpuConsumer(bqConsumer, 1);
        sp<FrameWaiter> rawWaiter = new FrameWaiter();
        rawConsumer->setFrameAvailableListener(rawWaiter);

        const int32_t *rawResolutions;
        size_t    rawResolutionsCount;

        int format = HAL_PIXEL_FORMAT_RAW_SENSOR;

        getResolutionList(format,
                &rawResolutions, &rawResolutionsCount);

        if (rawResolutionsCount <= 0) {
            const ::testing::TestInfo* const test_info =
                ::testing::UnitTest::GetInstance()->current_test_info();
            std::cerr << "Skipping test "
                      << test_info->test_case_name() << "."
                      << test_info->name()
                      << " because the optional format was not available: "
                      << "RAW_SENSOR" << std::endl;
            return;
        }

        ASSERT_LT((uint32_t)0, rawResolutionsCount);

        // Pick first available raw resolution
        int width = rawResolutions[0];
        int height = rawResolutions[1];

        int streamId;
        ASSERT_NO_FATAL_FAILURE(
            setUpStream(bqProducer, width, height, format, &streamId) );

        camera_metadata_t *request;
        request = allocate_camera_metadata(20, 2000);

        uint8_t metadataMode = ANDROID_REQUEST_METADATA_MODE_FULL;
        add_camera_metadata_entry(request,
                ANDROID_REQUEST_METADATA_MODE,
                (void**)&metadataMode, 1);
        uint32_t outputStreams = streamId;
        add_camera_metadata_entry(request,
                ANDROID_REQUEST_OUTPUT_STREAMS,
                (void**)&outputStreams, 1);

        uint64_t frameDuration = 30*MSEC;
        add_camera_metadata_entry(request,
                ANDROID_SENSOR_FRAME_DURATION,
                (void**)&frameDuration, 1);
        uint32_t sensitivity = 100;
        add_camera_metadata_entry(request,
                ANDROID_SENSOR_SENSITIVITY,
                (void**)&sensitivity, 1);
        uint8_t requestType = ANDROID_REQUEST_TYPE_CAPTURE;
        add_camera_metadata_entry(request,
                ANDROID_REQUEST_TYPE,
                (void**)&requestType, 1);

        uint32_t hourOfDay = 12;
        add_camera_metadata_entry(request,
                0x80000000, // EMULATOR_HOUROFDAY
                &hourOfDay, 1);

        IF_ALOGV() {
            std::cout << "Input request template: " << std::endl;
            dump_indented_camera_metadata(request, 0, 1, 2);
        }

        int numCaptures = 10;

        // Enqueue numCaptures requests with increasing exposure time

        uint64_t exposureTime = 100 * USEC;
        for (int reqCount = 0; reqCount < numCaptures; reqCount++ ) {
            camera_metadata_t *req;
            req = allocate_camera_metadata(20, 2000);
            append_camera_metadata(req, request);

            add_camera_metadata_entry(req,
                    ANDROID_SENSOR_EXPOSURE_TIME,
                    (void**)&exposureTime, 1);
            exposureTime *= 2;

            res = mRequests.enqueue(req);
            ASSERT_EQ(NO_ERROR, res) << "Can't enqueue request: "
                    << strerror(-res);
        }

        // Get frames and image buffers one by one
        uint64_t expectedExposureTime = 100 * USEC;
        for (int frameCount = 0; frameCount < 10; frameCount++) {
            res = mFrames.waitForBuffer(SEC + expectedExposureTime);
            ASSERT_EQ(NO_ERROR, res) << "No frame to get: " << strerror(-res);

            camera_metadata_t *frame;
            res = mFrames.dequeue(&frame);
            ASSERT_EQ(NO_ERROR, res);
            ASSERT_TRUE(frame != NULL);

            camera_metadata_entry_t frameNumber;
            res = find_camera_metadata_entry(frame,
                    ANDROID_REQUEST_FRAME_COUNT,
                    &frameNumber);
            ASSERT_EQ(NO_ERROR, res);
            ASSERT_EQ(frameCount, *frameNumber.data.i32);

            res = rawWaiter->waitForFrame(SEC + expectedExposureTime);
            ASSERT_EQ(NO_ERROR, res) <<
                    "Never got raw data for capture " << frameCount;

            CpuConsumer::LockedBuffer buffer;
            res = rawConsumer->lockNextBuffer(&buffer);
            ASSERT_EQ(NO_ERROR, res);

            IF_ALOGV() {
                char dumpname[60];
                snprintf(dumpname, 60,
                        "/data/local/tmp/camera2_test-"
                        "captureBurstRaw-dump_%d.raw",
                        frameCount);
                ALOGV("Dumping raw buffer to %s", dumpname);
                // Write to file
                std::ofstream rawFile(dumpname);
                for (unsigned int y = 0; y < buffer.height; y++) {
                    rawFile.write(
                            (const char *)(buffer.data + y * buffer.stride * 2),
                            buffer.width * 2);
                }
                rawFile.close();
            }

            res = rawConsumer->unlockBuffer(buffer);
            ASSERT_EQ(NO_ERROR, res);

            expectedExposureTime *= 2;
        }
    }
}

TEST_F(Camera2Test, ConstructDefaultRequests) {

    TEST_EXTENSION_FORKING_INIT;

    status_t res;

    for (int id = 0; id < getNumCameras(); id++) {
        if (!isHal2Supported(id)) continue;

        ASSERT_NO_FATAL_FAILURE(setUpCamera(id));

        for (int i = CAMERA2_TEMPLATE_PREVIEW; i < CAMERA2_TEMPLATE_COUNT;
             i++) {
            camera_metadata_t *request = NULL;
            res = mDevice->ops->construct_default_request(mDevice,
                    i,
                    &request);
            EXPECT_EQ(NO_ERROR, res) <<
                    "Unable to construct request from template type " << i;
            EXPECT_TRUE(request != NULL);
            EXPECT_LT((size_t)0, get_camera_metadata_entry_count(request));
            EXPECT_LT((size_t)0, get_camera_metadata_data_count(request));

            IF_ALOGV() {
                std::cout << "  ** Template type " << i << ":"<<std::endl;
                dump_indented_camera_metadata(request, 0, 2, 4);
            }

            free_camera_metadata(request);
        }
    }
}

TEST_F(Camera2Test, Capture1Jpeg) {
    status_t res;

    for (int id = 0; id < getNumCameras(); id++) {
        if (!isHal2Supported(id)) continue;

        ASSERT_NO_FATAL_FAILURE(setUpCamera(id));

        sp<IGraphicBufferProducer> bqProducer;
        sp<IGraphicBufferConsumer> bqConsumer;
        BufferQueue::createBufferQueue(&bqProducer, &bqConsumer);
        sp<CpuConsumer> jpegConsumer = new CpuConsumer(bqConsumer, 1);
        sp<FrameWaiter> jpegWaiter = new FrameWaiter();
        jpegConsumer->setFrameAvailableListener(jpegWaiter);

        const int32_t *jpegResolutions;
        size_t   jpegResolutionsCount;

        int format = HAL_PIXEL_FORMAT_BLOB;

        getResolutionList(format,
                &jpegResolutions, &jpegResolutionsCount);
        ASSERT_LT((size_t)0, jpegResolutionsCount);

        // Pick first available JPEG resolution
        int width = jpegResolutions[0];
        int height = jpegResolutions[1];

        int streamId;
        ASSERT_NO_FATAL_FAILURE(
            setUpStream(bqProducer, width, height, format, &streamId) );

        camera_metadata_t *request;
        request = allocate_camera_metadata(20, 2000);

        uint8_t metadataMode = ANDROID_REQUEST_METADATA_MODE_FULL;
        add_camera_metadata_entry(request,
                ANDROID_REQUEST_METADATA_MODE,
                (void**)&metadataMode, 1);
        uint32_t outputStreams = streamId;
        add_camera_metadata_entry(request,
                ANDROID_REQUEST_OUTPUT_STREAMS,
                (void**)&outputStreams, 1);

        uint64_t exposureTime = 10*MSEC;
        add_camera_metadata_entry(request,
                ANDROID_SENSOR_EXPOSURE_TIME,
                (void**)&exposureTime, 1);
        uint64_t frameDuration = 30*MSEC;
        add_camera_metadata_entry(request,
                ANDROID_SENSOR_FRAME_DURATION,
                (void**)&frameDuration, 1);
        uint32_t sensitivity = 100;
        add_camera_metadata_entry(request,
                ANDROID_SENSOR_SENSITIVITY,
                (void**)&sensitivity, 1);
        uint8_t requestType = ANDROID_REQUEST_TYPE_CAPTURE;
        add_camera_metadata_entry(request,
                ANDROID_REQUEST_TYPE,
                (void**)&requestType, 1);

        uint32_t hourOfDay = 12;
        add_camera_metadata_entry(request,
                0x80000000, // EMULATOR_HOUROFDAY
                &hourOfDay, 1);

        IF_ALOGV() {
            std::cout << "Input request: " << std::endl;
            dump_indented_camera_metadata(request, 0, 1, 4);
        }

        res = mRequests.enqueue(request);
        ASSERT_EQ(NO_ERROR, res) << "Can't enqueue request: " << strerror(-res);

        res = mFrames.waitForBuffer(exposureTime + SEC);
        ASSERT_EQ(NO_ERROR, res) << "No frame to get: " << strerror(-res);

        camera_metadata_t *frame;
        res = mFrames.dequeue(&frame);
        ASSERT_EQ(NO_ERROR, res);
        ASSERT_TRUE(frame != NULL);

        IF_ALOGV() {
            std::cout << "Output frame:" << std::endl;
            dump_indented_camera_metadata(frame, 0, 1, 4);
        }

        res = jpegWaiter->waitForFrame(exposureTime + SEC);
        ASSERT_EQ(NO_ERROR, res);

        CpuConsumer::LockedBuffer buffer;
        res = jpegConsumer->lockNextBuffer(&buffer);
        ASSERT_EQ(NO_ERROR, res);

        IF_ALOGV() {
            const char *dumpname =
                    "/data/local/tmp/camera2_test-capture1jpeg-dump.jpeg";
            ALOGV("Dumping raw buffer to %s", dumpname);
            // Write to file
            std::ofstream jpegFile(dumpname);
            size_t bpp = 1;
            for (unsigned int y = 0; y < buffer.height; y++) {
                jpegFile.write(
                        (const char *)(buffer.data + y * buffer.stride * bpp),
                        buffer.width * bpp);
            }
            jpegFile.close();
        }

        res = jpegConsumer->unlockBuffer(buffer);
        ASSERT_EQ(NO_ERROR, res);

        ASSERT_EQ(OK, waitUntilDrained());
        ASSERT_NO_FATAL_FAILURE(disconnectStream(streamId));

        res = closeCameraDevice(&mDevice);
        ASSERT_EQ(NO_ERROR, res) << "Failed to close camera device";

    }
}

} // namespace tests
} // namespace camera2
} // namespace android
