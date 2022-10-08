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

#ifndef STREAM_H_
#define STREAM_H_

#include <hardware/camera3.h>
#include <hardware/gralloc.h>
#include <system/graphics.h>
#include <utils/Mutex.h>

namespace default_camera_hal {
// Stream represents a single input or output stream for a camera device.
class Stream {
    public:
        Stream(int id, camera3_stream_t *s);
        ~Stream();

        // validate that astream's parameters match this stream's parameters
        bool isValidReuseStream(int id, camera3_stream_t *s);

        // Register buffers with hardware
        int registerBuffers(const camera3_stream_buffer_set_t *buf_set);

        void setUsage(uint32_t usage);
        void setMaxBuffers(uint32_t max_buffers);

        int getType();
        bool isInputType();
        bool isOutputType();
        bool isRegistered();
        const char* typeToString(int type);
        const char* formatToString(int format);
        void dump(int fd);

        // This stream is being reused. Used in stream configuration passes
        bool mReuse;

    private:
        // Clean up buffer state. must be called with mLock held.
        void unregisterBuffers_L();

        // The camera device id this stream belongs to
        const int mId;
        // Handle to framework's stream, used as a cookie for buffers
        camera3_stream_t *mStream;
        // Stream type: CAMERA3_STREAM_* (see <hardware/camera3.h>)
        const int mType;
        // Width in pixels of the buffers in this stream
        const uint32_t mWidth;
        // Height in pixels of the buffers in this stream
        const uint32_t mHeight;
        // Gralloc format: HAL_PIXEL_FORMAT_* (see <system/graphics.h>)
        const int mFormat;
        // Gralloc usage mask : GRALLOC_USAGE_* (see <hardware/gralloc.h>)
        uint32_t mUsage;
        // Max simultaneous in-flight buffers for this stream
        uint32_t mMaxBuffers;
        // Buffers have been registered for this stream and are ready
        bool mRegistered;
        // Array of handles to buffers currently in use by the stream
        buffer_handle_t **mBuffers;
        // Number of buffers in mBuffers
        unsigned int mNumBuffers;
        // Lock protecting the Stream object for modifications
        android::Mutex mLock;
};
} // namespace default_camera_hal

#endif // STREAM_H_
