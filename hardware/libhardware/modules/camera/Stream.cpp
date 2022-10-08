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

#include <stdio.h>
#include <hardware/camera3.h>
#include <hardware/gralloc.h>
#include <system/graphics.h>
#include <utils/Mutex.h>

//#define LOG_NDEBUG 0
#define LOG_TAG "Stream"
#include <cutils/log.h>

#define ATRACE_TAG (ATRACE_TAG_CAMERA | ATRACE_TAG_HAL)
#include <utils/Trace.h>

#include "Stream.h"

namespace default_camera_hal {

Stream::Stream(int id, camera3_stream_t *s)
  : mReuse(false),
    mId(id),
    mStream(s),
    mType(s->stream_type),
    mWidth(s->width),
    mHeight(s->height),
    mFormat(s->format),
    mUsage(0),
    mMaxBuffers(0),
    mRegistered(false),
    mBuffers(0),
    mNumBuffers(0)
{
}

Stream::~Stream()
{
    android::Mutex::Autolock al(mLock);
    unregisterBuffers_L();
}

void Stream::setUsage(uint32_t usage)
{
    android::Mutex::Autolock al(mLock);
    if (usage != mUsage) {
        mUsage = usage;
        mStream->usage = usage;
        unregisterBuffers_L();
    }
}

void Stream::setMaxBuffers(uint32_t max_buffers)
{
    android::Mutex::Autolock al(mLock);
    if (max_buffers != mMaxBuffers) {
        mMaxBuffers = max_buffers;
        mStream->max_buffers = max_buffers;
        unregisterBuffers_L();
    }
}

int Stream::getType()
{
    return mType;
}

bool Stream::isInputType()
{
    return mType == CAMERA3_STREAM_INPUT ||
        mType == CAMERA3_STREAM_BIDIRECTIONAL;
}

bool Stream::isOutputType()
{
    return mType == CAMERA3_STREAM_OUTPUT ||
        mType == CAMERA3_STREAM_BIDIRECTIONAL;
}

const char* Stream::typeToString(int type)
{
    switch (type) {
    case CAMERA3_STREAM_INPUT:
        return "CAMERA3_STREAM_INPUT";
    case CAMERA3_STREAM_OUTPUT:
        return "CAMERA3_STREAM_OUTPUT";
    case CAMERA3_STREAM_BIDIRECTIONAL:
        return "CAMERA3_STREAM_BIDIRECTIONAL";
    }
    return "Invalid stream type!";
}

const char* Stream::formatToString(int format)
{
    // See <system/graphics.h> for full list
    switch (format) {
    case HAL_PIXEL_FORMAT_BGRA_8888:
        return "BGRA 8888";
    case HAL_PIXEL_FORMAT_RGBA_8888:
        return "RGBA 8888";
    case HAL_PIXEL_FORMAT_RGBX_8888:
        return "RGBX 8888";
    case HAL_PIXEL_FORMAT_RGB_888:
        return "RGB 888";
    case HAL_PIXEL_FORMAT_RGB_565:
        return "RGB 565";
    case HAL_PIXEL_FORMAT_sRGB_A_8888:
        return "sRGB A 8888";
    case HAL_PIXEL_FORMAT_sRGB_X_8888:
        return "sRGB B 8888";
    case HAL_PIXEL_FORMAT_Y8:
        return "Y8";
    case HAL_PIXEL_FORMAT_Y16:
        return "Y16";
    case HAL_PIXEL_FORMAT_YV12:
        return "YV12";
    case HAL_PIXEL_FORMAT_YCbCr_422_SP:
        return "NV16";
    case HAL_PIXEL_FORMAT_YCrCb_420_SP:
        return "NV21";
    case HAL_PIXEL_FORMAT_YCbCr_422_I:
        return "YUY2";
    case HAL_PIXEL_FORMAT_RAW_SENSOR:
        return "RAW SENSOR";
    case HAL_PIXEL_FORMAT_BLOB:
        return "BLOB";
    case HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED:
        return "IMPLEMENTATION DEFINED";
    case HAL_PIXEL_FORMAT_YCbCr_420_888:
        return "FLEXIBLE YCbCr 420 888";
    }
    return "Invalid stream format!";
}

bool Stream::isRegistered()
{
    return mRegistered;
}

bool Stream::isValidReuseStream(int id, camera3_stream_t *s)
{
    if (id != mId) {
        ALOGE("%s:%d: Invalid camera id for reuse. Got %d expect %d",
                __func__, mId, id, mId);
        return false;
    }
    if (s != mStream) {
        ALOGE("%s:%d: Invalid stream handle for reuse. Got %p expect %p",
                __func__, mId, s, mStream);
        return false;
    }
    if (s->stream_type != mType) {
        ALOGE("%s:%d: Mismatched type in reused stream. Got %s(%d) "
                "expect %s(%d)", __func__, mId, typeToString(s->stream_type),
                s->stream_type, typeToString(mType), mType);
        return false;
    }
    if (s->format != mFormat) {
        ALOGE("%s:%d: Mismatched format in reused stream. Got %s(%d) "
                "expect %s(%d)", __func__, mId, formatToString(s->format),
                s->format, formatToString(mFormat), mFormat);
        return false;
    }
    if (s->width != mWidth) {
        ALOGE("%s:%d: Mismatched width in reused stream. Got %d expect %d",
                __func__, mId, s->width, mWidth);
        return false;
    }
    if (s->height != mHeight) {
        ALOGE("%s:%d: Mismatched height in reused stream. Got %d expect %d",
                __func__, mId, s->height, mHeight);
        return false;
    }
    return true;
}

int Stream::registerBuffers(const camera3_stream_buffer_set_t *buf_set)
{
    ATRACE_CALL();
    android::Mutex::Autolock al(mLock);

    if (buf_set->stream != mStream) {
        ALOGE("%s:%d: Buffer set for invalid stream. Got %p expect %p",
                __func__, mId, buf_set->stream, mStream);
        return -EINVAL;
    }

    mNumBuffers = buf_set->num_buffers;
    mBuffers = new buffer_handle_t*[mNumBuffers];

    for (unsigned int i = 0; i < mNumBuffers; i++) {
        ALOGV("%s:%d: Registering buffer %p", __func__, mId,
                buf_set->buffers[i]);
        mBuffers[i] = buf_set->buffers[i];
        // TODO: register buffers with hw, handle error cases
    }
    mRegistered = true;

    return 0;
}

// This must only be called with mLock held
void Stream::unregisterBuffers_L()
{
    mRegistered = false;
    mNumBuffers = 0;
    delete [] mBuffers;
    // TODO: unregister buffers from hw
}

void Stream::dump(int fd)
{
    android::Mutex::Autolock al(mLock);

    dprintf(fd, "Stream ID: %d (%p)\n", mId, mStream);
    dprintf(fd, "Stream Type: %s (%d)\n", typeToString(mType), mType);
    dprintf(fd, "Width: %"PRIu32" Height: %"PRIu32"\n", mWidth, mHeight);
    dprintf(fd, "Stream Format: %s (%d)", formatToString(mFormat), mFormat);
    // ToDo: prettyprint usage mask flags
    dprintf(fd, "Gralloc Usage Mask: %#"PRIx32"\n", mUsage);
    dprintf(fd, "Max Buffer Count: %"PRIu32"\n", mMaxBuffers);
    dprintf(fd, "Buffers Registered: %s\n", mRegistered ? "true" : "false");
    dprintf(fd, "Number of Buffers: %"PRIu32"\n", mNumBuffers);
    for (uint32_t i = 0; i < mNumBuffers; i++) {
        dprintf(fd, "Buffer %"PRIu32"/%"PRIu32": %p\n", i, mNumBuffers,
                mBuffers[i]);
    }
}

} // namespace default_camera_hal
