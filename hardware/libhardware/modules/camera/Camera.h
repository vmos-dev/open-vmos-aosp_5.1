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

#ifndef CAMERA_H_
#define CAMERA_H_

#include <hardware/hardware.h>
#include <hardware/camera3.h>
#include <utils/Mutex.h>
#include "Metadata.h"
#include "Stream.h"

namespace default_camera_hal {
// Camera represents a physical camera on a device.
// This is constructed when the HAL module is loaded, one per physical camera.
// It is opened by the framework, and must be closed before it can be opened
// again.
// This is an abstract class, containing all logic and data shared between all
// camera devices (front, back, etc) and common to the ISP.
class Camera {
    public:
        // id is used to distinguish cameras. 0 <= id < NUM_CAMERAS.
        // module is a handle to the HAL module, used when the device is opened.
        Camera(int id);
        virtual ~Camera();

        // Common Camera Device Operations (see <hardware/camera_common.h>)
        int open(const hw_module_t *module, hw_device_t **device);
        int getInfo(struct camera_info *info);
        int close();

        // Camera v3 Device Operations (see <hardware/camera3.h>)
        int initialize(const camera3_callback_ops_t *callback_ops);
        int configureStreams(camera3_stream_configuration_t *stream_list);
        int registerStreamBuffers(const camera3_stream_buffer_set_t *buf_set);
        const camera_metadata_t *constructDefaultRequestSettings(int type);
        int processCaptureRequest(camera3_capture_request_t *request);
        void dump(int fd);


    protected:
        // Initialize static camera characteristics for individual device
        virtual camera_metadata_t *initStaticInfo() = 0;
        // Verify settings are valid for a capture
        virtual bool isValidCaptureSettings(const camera_metadata_t *) = 0;
        // Separate initialization method for individual devices when opened
        virtual int initDevice() = 0;
        // Accessor used by initDevice() to set the templates' metadata
        int setTemplate(int type, camera_metadata_t *static_info);
        // Prettyprint template names
        const char* templateToString(int type);

    private:
        // Camera device handle returned to framework for use
        camera3_device_t mDevice;
        // Reuse a stream already created by this device
        Stream *reuseStream(camera3_stream_t *astream);
        // Destroy all streams in a stream array, and the array itself
        void destroyStreams(Stream **array, int count);
        // Verify a set of streams is valid in aggregate
        bool isValidStreamSet(Stream **array, int count);
        // Calculate usage and max_bufs of each stream
        void setupStreams(Stream **array, int count);
        // Copy new settings for re-use and clean up old settings.
        void setSettings(const camera_metadata_t *new_settings);
        // Verify settings are valid for reprocessing an input buffer
        bool isValidReprocessSettings(const camera_metadata_t *settings);
        // Process an output buffer
        int processCaptureBuffer(const camera3_stream_buffer_t *in,
                camera3_stream_buffer_t *out);
        // Send a shutter notify message with start of exposure time
        void notifyShutter(uint32_t frame_number, uint64_t timestamp);
        // Is type a valid template type (and valid index into mTemplates)
        bool isValidTemplateType(int type);

        // Identifier used by framework to distinguish cameras
        const int mId;
        // Metadata containing persistent camera characteristics
        Metadata mMetadata;
        // camera_metadata structure containing static characteristics
        camera_metadata_t *mStaticInfo;
        // Busy flag indicates camera is in use
        bool mBusy;
        // Camera device operations handle shared by all devices
        const static camera3_device_ops_t sOps;
        // Methods used to call back into the framework
        const camera3_callback_ops_t *mCallbackOps;
        // Lock protecting the Camera object for modifications
        android::Mutex mDeviceLock;
        // Lock protecting only static camera characteristics, which may
        // be accessed without the camera device open
        android::Mutex mStaticInfoLock;
        // Array of handles to streams currently in use by the device
        Stream **mStreams;
        // Number of streams in mStreams
        int mNumStreams;
        // Static array of standard camera settings templates
        camera_metadata_t *mTemplates[CAMERA3_TEMPLATE_COUNT];
        // Most recent request settings seen, memoized to be reused
        camera_metadata_t *mSettings;
};
} // namespace default_camera_hal

#endif // CAMERA_H_
