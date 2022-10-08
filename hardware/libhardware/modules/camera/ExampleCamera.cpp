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

#include <system/camera_metadata.h>
#include "Camera.h"

//#define LOG_NDEBUG 0
#define LOG_TAG "ExampleCamera"
#include <cutils/log.h>

#define ATRACE_TAG (ATRACE_TAG_CAMERA | ATRACE_TAG_HAL)
#include <utils/Trace.h>

#include "ExampleCamera.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

namespace default_camera_hal {

ExampleCamera::ExampleCamera(int id) : Camera(id)
{
}

ExampleCamera::~ExampleCamera()
{
}

camera_metadata_t *ExampleCamera::initStaticInfo()
{
    /*
     * Setup static camera info.  This will have to customized per camera
     * device.
     */
    Metadata m;

    /* android.control */
    int32_t android_control_ae_available_target_fps_ranges[] = {30, 30};
    m.addInt32(ANDROID_CONTROL_AE_AVAILABLE_TARGET_FPS_RANGES,
            ARRAY_SIZE(android_control_ae_available_target_fps_ranges),
            android_control_ae_available_target_fps_ranges);

    int32_t android_control_ae_compensation_range[] = {-4, 4};
    m.addInt32(ANDROID_CONTROL_AE_COMPENSATION_RANGE,
            ARRAY_SIZE(android_control_ae_compensation_range),
            android_control_ae_compensation_range);

    camera_metadata_rational_t android_control_ae_compensation_step[] = {{2,1}};
    m.addRational(ANDROID_CONTROL_AE_COMPENSATION_STEP,
            ARRAY_SIZE(android_control_ae_compensation_step),
            android_control_ae_compensation_step);

    int32_t android_control_max_regions[] = {/*AE*/ 1,/*AWB*/ 1,/*AF*/ 1};
    m.addInt32(ANDROID_CONTROL_MAX_REGIONS,
            ARRAY_SIZE(android_control_max_regions),
            android_control_max_regions);

    /* android.jpeg */
    int32_t android_jpeg_available_thumbnail_sizes[] = {0, 0, 128, 96};
    m.addInt32(ANDROID_JPEG_AVAILABLE_THUMBNAIL_SIZES,
            ARRAY_SIZE(android_jpeg_available_thumbnail_sizes),
            android_jpeg_available_thumbnail_sizes);

    int32_t android_jpeg_max_size[] = {13 * 1024 * 1024}; // 13MB
    m.addInt32(ANDROID_JPEG_MAX_SIZE,
            ARRAY_SIZE(android_jpeg_max_size),
            android_jpeg_max_size);

    /* android.lens */
    float android_lens_info_available_focal_lengths[] = {1.0};
    m.addFloat(ANDROID_LENS_INFO_AVAILABLE_FOCAL_LENGTHS,
            ARRAY_SIZE(android_lens_info_available_focal_lengths),
            android_lens_info_available_focal_lengths);

    /* android.request */
    int32_t android_request_max_num_output_streams[] = {0, 3, 1};
    m.addInt32(ANDROID_REQUEST_MAX_NUM_OUTPUT_STREAMS,
            ARRAY_SIZE(android_request_max_num_output_streams),
            android_request_max_num_output_streams);

    /* android.scaler */
    int32_t android_scaler_available_formats[] = {
            HAL_PIXEL_FORMAT_RAW_SENSOR,
            HAL_PIXEL_FORMAT_BLOB,
            HAL_PIXEL_FORMAT_RGBA_8888,
            HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED,
            // These are handled by YCbCr_420_888
            //        HAL_PIXEL_FORMAT_YV12,
            //        HAL_PIXEL_FORMAT_YCrCb_420_SP,
            HAL_PIXEL_FORMAT_YCbCr_420_888};
    m.addInt32(ANDROID_SCALER_AVAILABLE_FORMATS,
            ARRAY_SIZE(android_scaler_available_formats),
            android_scaler_available_formats);

    int64_t android_scaler_available_jpeg_min_durations[] = {1};
    m.addInt64(ANDROID_SCALER_AVAILABLE_JPEG_MIN_DURATIONS,
            ARRAY_SIZE(android_scaler_available_jpeg_min_durations),
            android_scaler_available_jpeg_min_durations);

    int32_t android_scaler_available_jpeg_sizes[] = {640, 480};
    m.addInt32(ANDROID_SCALER_AVAILABLE_JPEG_SIZES,
            ARRAY_SIZE(android_scaler_available_jpeg_sizes),
            android_scaler_available_jpeg_sizes);

    float android_scaler_available_max_digital_zoom[] = {1};
    m.addFloat(ANDROID_SCALER_AVAILABLE_MAX_DIGITAL_ZOOM,
            ARRAY_SIZE(android_scaler_available_max_digital_zoom),
            android_scaler_available_max_digital_zoom);

    int64_t android_scaler_available_processed_min_durations[] = {1};
    m.addInt64(ANDROID_SCALER_AVAILABLE_PROCESSED_MIN_DURATIONS,
            ARRAY_SIZE(android_scaler_available_processed_min_durations),
            android_scaler_available_processed_min_durations);

    int32_t android_scaler_available_processed_sizes[] = {640, 480};
    m.addInt32(ANDROID_SCALER_AVAILABLE_PROCESSED_SIZES,
            ARRAY_SIZE(android_scaler_available_processed_sizes),
            android_scaler_available_processed_sizes);

    int64_t android_scaler_available_raw_min_durations[] = {1};
    m.addInt64(ANDROID_SCALER_AVAILABLE_RAW_MIN_DURATIONS,
            ARRAY_SIZE(android_scaler_available_raw_min_durations),
            android_scaler_available_raw_min_durations);

    int32_t android_scaler_available_raw_sizes[] = {640, 480};
    m.addInt32(ANDROID_SCALER_AVAILABLE_RAW_SIZES,
            ARRAY_SIZE(android_scaler_available_raw_sizes),
            android_scaler_available_raw_sizes);

    /* android.sensor */

    int32_t android_sensor_info_active_array_size[] = {0, 0, 640, 480};
    m.addInt32(ANDROID_SENSOR_INFO_ACTIVE_ARRAY_SIZE,
            ARRAY_SIZE(android_sensor_info_active_array_size),
            android_sensor_info_active_array_size);

    int32_t android_sensor_info_sensitivity_range[] =
            {100, 1600};
    m.addInt32(ANDROID_SENSOR_INFO_SENSITIVITY_RANGE,
            ARRAY_SIZE(android_sensor_info_sensitivity_range),
            android_sensor_info_sensitivity_range);

    int64_t android_sensor_info_max_frame_duration[] = {30000000000};
    m.addInt64(ANDROID_SENSOR_INFO_MAX_FRAME_DURATION,
            ARRAY_SIZE(android_sensor_info_max_frame_duration),
            android_sensor_info_max_frame_duration);

    float android_sensor_info_physical_size[] = {3.2, 2.4};
    m.addFloat(ANDROID_SENSOR_INFO_PHYSICAL_SIZE,
            ARRAY_SIZE(android_sensor_info_physical_size),
            android_sensor_info_physical_size);

    int32_t android_sensor_info_pixel_array_size[] = {640, 480};
    m.addInt32(ANDROID_SENSOR_INFO_PIXEL_ARRAY_SIZE,
            ARRAY_SIZE(android_sensor_info_pixel_array_size),
            android_sensor_info_pixel_array_size);

    int32_t android_sensor_orientation[] = {0};
    m.addInt32(ANDROID_SENSOR_ORIENTATION,
            ARRAY_SIZE(android_sensor_orientation),
            android_sensor_orientation);

    /* End of static camera characteristics */

    return clone_camera_metadata(m.get());
}

int ExampleCamera::initDevice()
{
    int res;
    Metadata base;

    // Create standard settings templates from copies of base metadata
    // TODO: use vendor tags in base metadata
    res = base.add1UInt8(ANDROID_CONTROL_MODE, ANDROID_CONTROL_MODE_OFF);
    if (res)
        return res;

    // Use base settings to create all other templates and set them
    res = setPreviewTemplate(base);
    if (res)
        return res;
    res = setStillTemplate(base);
    if (res)
        return res;
    res = setRecordTemplate(base);
    if (res)
        return res;
    res = setSnapshotTemplate(base);
    if (res)
        return res;
    res = setZslTemplate(base);
    if (res)
        return res;

    return 0;
}

int ExampleCamera::setPreviewTemplate(Metadata m)
{
    // Setup default preview controls
    int res = m.add1UInt8(ANDROID_CONTROL_CAPTURE_INTENT,
                            ANDROID_CONTROL_CAPTURE_INTENT_PREVIEW);

    if (res)
        return res;
    // TODO: set fast auto-focus, auto-whitebalance, auto-exposure, auto flash
    return setTemplate(CAMERA3_TEMPLATE_PREVIEW, m.get());
}

int ExampleCamera::setStillTemplate(Metadata m)
{
    int res = m.add1UInt8(ANDROID_CONTROL_CAPTURE_INTENT,
                            ANDROID_CONTROL_CAPTURE_INTENT_STILL_CAPTURE);
    // Setup default still capture controls
    if (res)
        return res;
    // TODO: set fast auto-focus, auto-whitebalance, auto-exposure, auto flash
    return setTemplate(CAMERA3_TEMPLATE_STILL_CAPTURE, m.get());
}

int ExampleCamera::setRecordTemplate(Metadata m)
{
    int res = m.add1UInt8(ANDROID_CONTROL_CAPTURE_INTENT,
                            ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_RECORD);
    // Setup default video record controls
    if (res)
        return res;
    // TODO: set slow auto-focus, auto-whitebalance, auto-exposure, flash off
    return setTemplate(CAMERA3_TEMPLATE_VIDEO_RECORD, m.get());
}

int ExampleCamera::setSnapshotTemplate(Metadata m)
{
    int res = m.add1UInt8(ANDROID_CONTROL_CAPTURE_INTENT,
                            ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_SNAPSHOT);
    // Setup default video snapshot controls
    if (res)
        return res;
    // TODO: set slow auto-focus, auto-whitebalance, auto-exposure, flash off
    return setTemplate(CAMERA3_TEMPLATE_VIDEO_SNAPSHOT, m.get());
}

int ExampleCamera::setZslTemplate(Metadata m)
{
    int res = m.add1UInt8(ANDROID_CONTROL_CAPTURE_INTENT,
                            ANDROID_CONTROL_CAPTURE_INTENT_ZERO_SHUTTER_LAG);
    // Setup default zero shutter lag controls
    if (res)
        return res;
    // TODO: set reprocessing parameters for zsl input queue
    return setTemplate(CAMERA3_TEMPLATE_ZERO_SHUTTER_LAG, m.get());
}

bool ExampleCamera::isValidCaptureSettings(const camera_metadata_t* settings)
{
    // TODO: reject settings that cannot be captured
    return true;
}

} // namespace default_camera_hal
