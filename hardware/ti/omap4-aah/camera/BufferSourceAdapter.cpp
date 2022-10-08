/*
 * Copyright (C) Texas Instruments - http://www.ti.com/
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

#ifdef OMAP_ENHANCEMENT_CPCAM

#include "BufferSourceAdapter.h"
#include <ui/GraphicBuffer.h>
#include <ui/GraphicBufferMapper.h>
#include <hal_public.h>

namespace Ti {
namespace Camera {

static int getANWFormat(const char* parameters_format)
{
    int format = HAL_PIXEL_FORMAT_TI_NV12;

    if (parameters_format != NULL) {
        if (strcmp(parameters_format, android::CameraParameters::PIXEL_FORMAT_YUV422I) == 0) {
            CAMHAL_LOGDA("CbYCrY format selected");
            format = HAL_PIXEL_FORMAT_TI_UYVY;
        } else if (strcmp(parameters_format, android::CameraParameters::PIXEL_FORMAT_YUV420SP) == 0) {
            CAMHAL_LOGDA("YUV420SP format selected");
            format = HAL_PIXEL_FORMAT_TI_NV12;
        } else if (strcmp(parameters_format, android::CameraParameters::PIXEL_FORMAT_RGB565) == 0) {
            CAMHAL_LOGDA("RGB565 format selected");
            // TODO(XXX): not defined yet
            format = -1;
        } else if (strcmp(parameters_format, android::CameraParameters::PIXEL_FORMAT_BAYER_RGGB) == 0) {
            format = HAL_PIXEL_FORMAT_TI_Y16;
        } else {
            CAMHAL_LOGDA("Invalid format, NV12 format selected as default");
            format = HAL_PIXEL_FORMAT_TI_NV12;
        }
    }

    return format;
}

static int getUsageFromANW(int format)
{
    int usage = GRALLOC_USAGE_SW_READ_RARELY |
                GRALLOC_USAGE_SW_WRITE_NEVER;

    switch (format) {
        case HAL_PIXEL_FORMAT_TI_NV12:
        case HAL_PIXEL_FORMAT_TI_Y16:
            // This usage flag indicates to gralloc we want the
            // buffers to come from system heap
            usage |= GRALLOC_USAGE_PRIVATE_0;
            break;
        default:
            // No special flags needed
            break;
    }
    return usage;
}

static const char* getFormatFromANW(int format)
{
    switch (format) {
        case HAL_PIXEL_FORMAT_TI_NV12:
            // Assuming NV12 1D is RAW or Image frame
            return android::CameraParameters::PIXEL_FORMAT_YUV420SP;
        case HAL_PIXEL_FORMAT_TI_Y16:
            return android::CameraParameters::PIXEL_FORMAT_BAYER_RGGB;
        case HAL_PIXEL_FORMAT_TI_UYVY:
            return android::CameraParameters::PIXEL_FORMAT_YUV422I;
        default:
            break;
    }
    return android::CameraParameters::PIXEL_FORMAT_YUV420SP;
}

static CameraFrame::FrameType formatToOutputFrameType(const char* format) {
    switch (getANWFormat(format)) {
        case HAL_PIXEL_FORMAT_TI_NV12:
        case HAL_PIXEL_FORMAT_TI_Y16:
        case HAL_PIXEL_FORMAT_TI_UYVY:
            // Assuming NV12 1D is RAW or Image frame
            return CameraFrame::RAW_FRAME;
        default:
            break;
    }
    return CameraFrame::RAW_FRAME;
}

static int getHeightFromFormat(const char* format, int stride, int size) {
    CAMHAL_ASSERT((NULL != format) && (0 <= stride) && (0 <= size));
    switch (getANWFormat(format)) {
        case HAL_PIXEL_FORMAT_TI_NV12:
            return (size / (3 * stride)) * 2;
        case HAL_PIXEL_FORMAT_TI_Y16:
        case HAL_PIXEL_FORMAT_TI_UYVY:
            return (size / stride) / 2;
        default:
            break;
    }
    return 0;
}

/*--------------------BufferSourceAdapter Class STARTS here-----------------------------*/


///Constant definitions
// TODO(XXX): Temporarily increase number of buffers we can allocate from ANW
// until faux-NPA mode is implemented
const int BufferSourceAdapter::NO_BUFFERS_IMAGE_CAPTURE_SYSTEM_HEAP = 15;

/**
 * Display Adapter class STARTS here..
 */
BufferSourceAdapter::BufferSourceAdapter() : mBufferCount(0)
{
    LOG_FUNCTION_NAME;

    mPixelFormat = NULL;
    mBuffers = NULL;
    mFrameProvider = NULL;
    mBufferSource = NULL;

    mFrameWidth = 0;
    mFrameHeight = 0;
    mPreviewWidth = 0;
    mPreviewHeight = 0;

    LOG_FUNCTION_NAME_EXIT;
}

BufferSourceAdapter::~BufferSourceAdapter()
{
    LOG_FUNCTION_NAME;

    freeBufferList(mBuffers);

    android::AutoMutex lock(mLock);

    destroy();

    if (mFrameProvider) {
        // Unregister with the frame provider
        mFrameProvider->disableFrameNotification(CameraFrame::ALL_FRAMES);
        delete mFrameProvider;
        mFrameProvider = NULL;
    }

    if (mQueueFrame.get()) {
        mQueueFrame->requestExit();
        mQueueFrame.clear();
    }

    if (mReturnFrame.get()) {
        mReturnFrame->requestExit();
        mReturnFrame.clear();
    }

    LOG_FUNCTION_NAME_EXIT;
}

status_t BufferSourceAdapter::initialize()
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    mReturnFrame.clear();
    mReturnFrame = new ReturnFrame(this);
    mReturnFrame->run();

    mQueueFrame.clear();
    mQueueFrame = new QueueFrame(this);
    mQueueFrame->run();

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

int BufferSourceAdapter::setPreviewWindow(preview_stream_ops_t *source)
{
    LOG_FUNCTION_NAME;

    if (!source) {
        CAMHAL_LOGEA("NULL window object passed to DisplayAdapter");
        LOG_FUNCTION_NAME_EXIT;
        return BAD_VALUE;
    }

    if (mBufferSource) {
        char id1[OP_STR_SIZE], id2[OP_STR_SIZE];
        status_t ret;

        ret = extendedOps()->get_id(mBufferSource, id1, sizeof(id1));
        if (ret != 0) {
            CAMHAL_LOGE("Surface::getId returned error %d", ret);
            return ret;
        }

        ret = extendedOps()->get_id(source, id2, sizeof(id2));
        if (ret != 0) {
            CAMHAL_LOGE("Surface::getId returned error %d", ret);
            return ret;
        }
        if ((0 >= strlen(id1)) || (0 >= strlen(id2))) {
            CAMHAL_LOGE("Cannot set ST without name: id1:\"%s\" id2:\"%s\"",
                        id1, id2);
            return NOT_ENOUGH_DATA;
        }
        if (0 == strcmp(id1, id2)) {
            return ALREADY_EXISTS;
        }

        // client has to unset mBufferSource before being able to set a new one
        return BAD_VALUE;
    }

    // Move to new source obj
    mBufferSource = source;

    LOG_FUNCTION_NAME_EXIT;

    return NO_ERROR;
}

bool BufferSourceAdapter::match(const char * str) {
    char id1[OP_STR_SIZE];
    status_t ret;

    ret = extendedOps()->get_id(mBufferSource, id1, sizeof(id1));

    if (ret != 0) {
        CAMHAL_LOGE("Surface::getId returned error %d", ret);
    }

    return strcmp(id1, str) == 0;
}

int BufferSourceAdapter::setFrameProvider(FrameNotifier *frameProvider)
{
    LOG_FUNCTION_NAME;

    if ( !frameProvider ) {
        CAMHAL_LOGEA("NULL passed for frame provider");
        LOG_FUNCTION_NAME_EXIT;
        return BAD_VALUE;
    }

    if ( NULL != mFrameProvider ) {
        delete mFrameProvider;
    }

    mFrameProvider = new FrameProvider(frameProvider, this, frameCallback);

    LOG_FUNCTION_NAME_EXIT;

    return NO_ERROR;
}

int BufferSourceAdapter::setErrorHandler(ErrorNotifier *errorNotifier)
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    if ( NULL == errorNotifier ) {
        CAMHAL_LOGEA("Invalid Error Notifier reference");
        return -EINVAL;
    }

    mErrorNotifier = errorNotifier;

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

int BufferSourceAdapter::enableDisplay(int width, int height,
                                       struct timeval *refTime)
{
    LOG_FUNCTION_NAME;
    CameraFrame::FrameType frameType;

    if (mFrameProvider == NULL) {
        // no-op frame provider not set yet
        return NO_ERROR;
    }

    if (mBufferSourceDirection == BUFFER_SOURCE_TAP_IN) {
        // only supporting one type of input frame
        frameType = CameraFrame::REPROCESS_INPUT_FRAME;
    } else {
        frameType = formatToOutputFrameType(mPixelFormat);
    }

    mFrameProvider->enableFrameNotification(frameType);
    LOG_FUNCTION_NAME_EXIT;

    return NO_ERROR;
}

int BufferSourceAdapter::disableDisplay(bool cancel_buffer)
{
    LOG_FUNCTION_NAME;

    if (mFrameProvider) mFrameProvider->disableFrameNotification(CameraFrame::ALL_FRAMES);

    LOG_FUNCTION_NAME_EXIT;

    return NO_ERROR;
}

status_t BufferSourceAdapter::pauseDisplay(bool pause)
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    // no-op for BufferSourceAdapter

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}


void BufferSourceAdapter::destroy()
{
    LOG_FUNCTION_NAME;

    mBufferCount = 0;

    LOG_FUNCTION_NAME_EXIT;
}

CameraBuffer* BufferSourceAdapter::allocateBufferList(int width, int dummyHeight, const char* format,
                                                      int &bytes, int numBufs)
{
    LOG_FUNCTION_NAME;
    status_t err;
    int i = -1;
    const int lnumBufs = numBufs;
    int undequeued = 0;
    android::GraphicBufferMapper &mapper = android::GraphicBufferMapper::get();

    mBuffers = new CameraBuffer [lnumBufs];
    memset (mBuffers, 0, sizeof(CameraBuffer) * lnumBufs);

    if ( NULL == mBufferSource ) {
        return NULL;
    }

    int pixFormat = getANWFormat(format);
    int usage = getUsageFromANW(pixFormat);
    mPixelFormat = CameraHal::getPixelFormatConstant(format);

    // Set gralloc usage bits for window.
    err = mBufferSource->set_usage(mBufferSource, usage);
    if (err != 0) {
        CAMHAL_LOGE("native_window_set_usage failed: %s (%d)", strerror(-err), -err);

        if ( ENODEV == err ) {
            CAMHAL_LOGEA("Preview surface abandoned!");
            mBufferSource = NULL;
        }

        return NULL;
    }

    CAMHAL_LOGDB("Number of buffers set to BufferSourceAdapter %d", numBufs);
    // Set the number of buffers needed for this buffer source
    err = mBufferSource->set_buffer_count(mBufferSource, numBufs);
    if (err != 0) {
        CAMHAL_LOGE("native_window_set_buffer_count failed: %s (%d)", strerror(-err), -err);

        if ( ENODEV == err ) {
            CAMHAL_LOGEA("Preview surface abandoned!");
            mBufferSource = NULL;
        }

        return NULL;
    }

    CAMHAL_LOGDB("Configuring %d buffers for ANativeWindow", numBufs);
    mBufferCount = numBufs;

    // re-calculate height depending on stride and size
    int height = getHeightFromFormat(format, width, bytes);

    // Set window geometry
    err = mBufferSource->set_buffers_geometry(mBufferSource,
                                              width, height,
                                              pixFormat);

    if (err != 0) {
        CAMHAL_LOGE("native_window_set_buffers_geometry failed: %s (%d)", strerror(-err), -err);
        if ( ENODEV == err ) {
            CAMHAL_LOGEA("Preview surface abandoned!");
            mBufferSource = NULL;
        }
        return NULL;
    }

    if ( mBuffers == NULL ) {
        CAMHAL_LOGEA("Couldn't create array for ANativeWindow buffers");
        LOG_FUNCTION_NAME_EXIT;
        return NULL;
    }

    mBufferSource->get_min_undequeued_buffer_count(mBufferSource, &undequeued);

    for (i = 0; i < mBufferCount; i++ ) {
        buffer_handle_t *handle;
        int stride;  // dummy variable to get stride
        // TODO(XXX): Do we need to keep stride information in camera hal?

        err = mBufferSource->dequeue_buffer(mBufferSource, &handle, &stride);

        if (err != 0) {
            CAMHAL_LOGEB("dequeueBuffer failed: %s (%d)", strerror(-err), -err);
            if ( ENODEV == err ) {
                CAMHAL_LOGEA("Preview surface abandoned!");
                mBufferSource = NULL;
            }
            goto fail;
        }

        CAMHAL_LOGDB("got handle %p", handle);
        mBuffers[i].opaque = (void *)handle;
        mBuffers[i].type = CAMERA_BUFFER_ANW;
        mBuffers[i].format = mPixelFormat;
        mFramesWithCameraAdapterMap.add(handle, i);

        bytes = CameraHal::calculateBufferSize(format, width, height);
    }

    for( i = 0;  i < mBufferCount-undequeued; i++ ) {
        void *y_uv[2];
        android::Rect bounds(width, height);

        buffer_handle_t *handle = (buffer_handle_t *) mBuffers[i].opaque;
        mBufferSource->lock_buffer(mBufferSource, handle);
        mapper.lock(*handle, CAMHAL_GRALLOC_USAGE, bounds, y_uv);
        mBuffers[i].mapped = y_uv[0];
    }

    // return the rest of the buffers back to ANativeWindow
    for(i = (mBufferCount-undequeued); i >= 0 && i < mBufferCount; i++) {
        buffer_handle_t *handle = (buffer_handle_t *) mBuffers[i].opaque;
        void *y_uv[2];
        android::Rect bounds(width, height);

        mapper.lock(*handle, CAMHAL_GRALLOC_USAGE, bounds, y_uv);
        mBuffers[i].mapped = y_uv[0];
        mapper.unlock(*handle);

        err = mBufferSource->cancel_buffer(mBufferSource, handle);
        if (err != 0) {
            CAMHAL_LOGEB("cancel_buffer failed: %s (%d)", strerror(-err), -err);
            if ( ENODEV == err ) {
                CAMHAL_LOGEA("Preview surface abandoned!");
                mBufferSource = NULL;
            }
            goto fail;
        }
        mFramesWithCameraAdapterMap.removeItem((buffer_handle_t *) mBuffers[i].opaque);
    }

    mFrameWidth = width;
    mFrameHeight = height;
    mBufferSourceDirection = BUFFER_SOURCE_TAP_OUT;

    return mBuffers;

 fail:
    // need to cancel buffers if any were dequeued
    for (int start = 0; start < i && i > 0; start++) {
        int err = mBufferSource->cancel_buffer(mBufferSource,
                (buffer_handle_t *) mBuffers[start].opaque);
        if (err != 0) {
          CAMHAL_LOGEB("cancelBuffer failed w/ error 0x%08x", err);
          break;
        }
        mFramesWithCameraAdapterMap.removeItem((buffer_handle_t *) mBuffers[start].opaque);
    }

    freeBufferList(mBuffers);

    CAMHAL_LOGEA("Error occurred, performing cleanup");

    if (NULL != mErrorNotifier.get()) {
        mErrorNotifier->errorNotify(-ENOMEM);
    }

    LOG_FUNCTION_NAME_EXIT;
    return NULL;

}

CameraBuffer *BufferSourceAdapter::getBuffers(bool reset) {
    int undequeued = 0;
    status_t err;
    android::Mutex::Autolock lock(mLock);

    if (!mBufferSource || !mBuffers) {
        CAMHAL_LOGE("Adapter is not set up properly: "
                    "mBufferSource:%p mBuffers:%p",
                     mBufferSource, mBuffers);
        goto fail;
    }

    // CameraHal is indicating to us that the state of the mBuffer
    // might have changed. We might need to check the state of the
    // buffer list and pass a new one depending on the state of our
    // surface
    if (reset) {
        const int lnumBufs = mBufferCount;
        android::GraphicBufferMapper &mapper = android::GraphicBufferMapper::get();
        android::Rect bounds(mFrameWidth, mFrameHeight);
        void *y_uv[2];
        CameraBuffer * newBuffers = NULL;
        unsigned int index = 0;
        android::KeyedVector<void*, int> missingIndices;

        newBuffers = new CameraBuffer [lnumBufs];
        memset (newBuffers, 0, sizeof(CameraBuffer) * lnumBufs);

        // Use this vector to figure out missing indices
        for (int i = 0; i < mBufferCount; i++) {
            missingIndices.add(mBuffers[i].opaque, i);
        }

        // assign buffers that we have already dequeued
        for (index = 0; index < mFramesWithCameraAdapterMap.size(); index++) {
            int value = mFramesWithCameraAdapterMap.valueAt(index);
            newBuffers[index].opaque = mBuffers[value].opaque;
            newBuffers[index].type = mBuffers[value].type;
            newBuffers[index].format = mBuffers[value].format;
            newBuffers[index].mapped = mBuffers[value].mapped;
            mFramesWithCameraAdapterMap.replaceValueAt(index, index);
            missingIndices.removeItem(newBuffers[index].opaque);
        }

        mBufferSource->get_min_undequeued_buffer_count(mBufferSource, &undequeued);

        // dequeue the rest of the buffers
        for (index; index < (unsigned int)(mBufferCount-undequeued); index++) {
            buffer_handle_t *handle;
            int stride;  // dummy variable to get stride

            err = mBufferSource->dequeue_buffer(mBufferSource, &handle, &stride);
            if (err != 0) {
                CAMHAL_LOGEB("dequeueBuffer failed: %s (%d)", strerror(-err), -err);
                if ( ENODEV == err ) {
                    CAMHAL_LOGEA("Preview surface abandoned!");
                    mBufferSource = NULL;
                }
                goto fail;
            }
            newBuffers[index].opaque = (void *)handle;
            newBuffers[index].type = CAMERA_BUFFER_ANW;
            newBuffers[index].format = mPixelFormat;
            mFramesWithCameraAdapterMap.add(handle, index);

            mBufferSource->lock_buffer(mBufferSource, handle);
            mapper.lock(*handle, CAMHAL_GRALLOC_USAGE, bounds, y_uv);
            newBuffers[index].mapped = y_uv[0];
            CAMHAL_LOGDB("got handle %p", handle);

            missingIndices.removeItem(newBuffers[index].opaque);
        }

        // now we need to figure out which buffers aren't dequeued
        // which are in mBuffers but not newBuffers yet
        if ((mBufferCount - index) != missingIndices.size()) {
            CAMHAL_LOGD("Hrmm somethings gone awry. We are missing a different number"
                        " of buffers than we can fill");
        }
        for (unsigned int i = 0; i < missingIndices.size(); i++) {
            int j = missingIndices.valueAt(i);

            CAMHAL_LOGD("Filling at %d", j);
            newBuffers[index].opaque = mBuffers[j].opaque;
            newBuffers[index].type = mBuffers[j].type;
            newBuffers[index].format = mBuffers[j].format;
            newBuffers[index].mapped = mBuffers[j].mapped;
        }

        delete [] mBuffers;
        mBuffers = newBuffers;
    }

    return mBuffers;

 fail:
    return NULL;
}

unsigned int BufferSourceAdapter::getSize() {
    android::Mutex::Autolock lock(mLock);
    return CameraHal::calculateBufferSize(mPixelFormat, mFrameWidth, mFrameHeight);
}

int BufferSourceAdapter::getBufferCount() {
    int count = -1;

    android::Mutex::Autolock lock(mLock);
    if (mBufferSource) extendedOps()->get_buffer_count(mBufferSource, &count);
    return count;
}

CameraBuffer* BufferSourceAdapter::getBufferList(int *num) {
    LOG_FUNCTION_NAME;
    status_t err;
    const int lnumBufs = 1;
    int formatSource;
    android::GraphicBufferMapper &mapper = android::GraphicBufferMapper::get();
    buffer_handle_t *handle;

    // TODO(XXX): Only supporting one input buffer at a time right now
    *num = 1;
    mBuffers = new CameraBuffer [lnumBufs];
    memset (mBuffers, 0, sizeof(CameraBuffer) * lnumBufs);

    if ( NULL == mBufferSource ) {
        return NULL;
    }

    err = extendedOps()->update_and_get_buffer(mBufferSource, &handle, &mBuffers[0].stride);
    if (err != 0) {
        CAMHAL_LOGEB("update and get buffer failed: %s (%d)", strerror(-err), -err);
        if ( ENODEV == err ) {
            CAMHAL_LOGEA("Preview surface abandoned!");
            mBufferSource = NULL;
        }
        goto fail;
    }

    CAMHAL_LOGD("got handle %p", handle);
    mBuffers[0].opaque = (void *)handle;
    mBuffers[0].type = CAMERA_BUFFER_ANW;
    mFramesWithCameraAdapterMap.add(handle, 0);

    err = extendedOps()->get_buffer_dimension(mBufferSource, &mBuffers[0].width, &mBuffers[0].height);
    err = extendedOps()->get_buffer_format(mBufferSource, &formatSource);

    int t, l, r, b, w, h;
    err = extendedOps()->get_crop(mBufferSource, &l, &t, &r, &b);
    err = extendedOps()->get_current_size(mBufferSource, &w, &h);

    // lock buffer
    {
        void *y_uv[2];
        android::Rect bounds(mBuffers[0].width, mBuffers[0].height);
        mapper.lock(*handle, CAMHAL_GRALLOC_USAGE, bounds, y_uv);
        mBuffers[0].mapped = y_uv[0];
    }

    mFrameWidth = mBuffers[0].width;
    mFrameHeight = mBuffers[0].height;
    mPixelFormat = getFormatFromANW(formatSource);

    mBuffers[0].format = mPixelFormat;
    mBuffers[0].actual_size = CameraHal::calculateBufferSize(mPixelFormat, w, h);
    mBuffers[0].offset = t * w + l * CameraHal::getBPP(mPixelFormat);
    mBufferSourceDirection = BUFFER_SOURCE_TAP_IN;

    return mBuffers;

 fail:
    // need to cancel buffers if any were dequeued
    freeBufferList(mBuffers);

    if (NULL != mErrorNotifier.get()) {
        mErrorNotifier->errorNotify(-ENOMEM);
    }

    LOG_FUNCTION_NAME_EXIT;
    return NULL;
}

uint32_t * BufferSourceAdapter::getOffsets()
{
    LOG_FUNCTION_NAME;

    LOG_FUNCTION_NAME_EXIT;

    return NULL;
}

int BufferSourceAdapter::minUndequeueableBuffers(int& undequeueable) {
    LOG_FUNCTION_NAME;
    int ret = NO_ERROR;

    if(!mBufferSource)
    {
        ret = INVALID_OPERATION;
        goto end;
    }

    ret = mBufferSource->get_min_undequeued_buffer_count(mBufferSource, &undequeueable);
    if ( NO_ERROR != ret ) {
        CAMHAL_LOGEB("get_min_undequeued_buffer_count failed: %s (%d)", strerror(-ret), -ret);
        if ( ENODEV == ret ) {
            CAMHAL_LOGEA("Preview surface abandoned!");
            mBufferSource = NULL;
        }
        return -ret;
    }

 end:
    return ret;
    LOG_FUNCTION_NAME_EXIT;

}

int BufferSourceAdapter::maxQueueableBuffers(unsigned int& queueable)
{
    LOG_FUNCTION_NAME;
    int ret = NO_ERROR;
    int undequeued = 0;

    if(mBufferCount == 0) {
        ret = INVALID_OPERATION;
        goto end;
    }

    ret = minUndequeueableBuffers(undequeued);
    if (ret != NO_ERROR) {
        goto end;
    }

    queueable = mBufferCount - undequeued;

 end:
    return ret;
    LOG_FUNCTION_NAME_EXIT;
}

int BufferSourceAdapter::getFd()
{
    LOG_FUNCTION_NAME;

    LOG_FUNCTION_NAME_EXIT;

    return -1;

}

status_t BufferSourceAdapter::returnBuffersToWindow()
{
    status_t ret = NO_ERROR;
    android::GraphicBufferMapper &mapper = android::GraphicBufferMapper::get();

    //Give the buffers back to display here -  sort of free it
    if (mBufferSource) {
        for(unsigned int i = 0; i < mFramesWithCameraAdapterMap.size(); i++) {
            int value = mFramesWithCameraAdapterMap.valueAt(i);
            buffer_handle_t *handle = (buffer_handle_t *) mBuffers[value].opaque;

            // if buffer index is out of bounds skip
            if ((value < 0) || (value >= mBufferCount)) {
                CAMHAL_LOGEA("Potential out bounds access to handle...skipping");
                continue;
            }

            // unlock buffer before giving it up
            mapper.unlock(*handle);

            ret = mBufferSource->cancel_buffer(mBufferSource, handle);
            if ( ENODEV == ret ) {
                CAMHAL_LOGEA("Preview surface abandoned!");
                mBufferSource = NULL;
                return -ret;
            } else if ( NO_ERROR != ret ) {
                CAMHAL_LOGEB("cancel_buffer() failed: %s (%d)",
                              strerror(-ret),
                              -ret);
                return -ret;
            }
        }
    } else {
         CAMHAL_LOGE("mBufferSource is NULL");
    }

     ///Clear the frames with camera adapter map
     mFramesWithCameraAdapterMap.clear();

     return ret;

}

int BufferSourceAdapter::freeBufferList(CameraBuffer * buflist)
{
    LOG_FUNCTION_NAME;

    status_t ret = NO_ERROR;

    if ( mBuffers != buflist ) {
        return BAD_VALUE;
    }

    android::AutoMutex lock(mLock);

    if (mBufferSourceDirection == BUFFER_SOURCE_TAP_OUT) returnBuffersToWindow();

    if( mBuffers != NULL)
    {
        delete [] mBuffers;
        mBuffers = NULL;
    }

    return NO_ERROR;
}


bool BufferSourceAdapter::supportsExternalBuffering()
{
    return false;
}

void BufferSourceAdapter::addFrame(CameraFrame* frame)
{
    if (mQueueFrame.get()) {
        mQueueFrame->addFrame(frame);
    }
}

void BufferSourceAdapter::handleFrameCallback(CameraFrame* frame)
{
    status_t ret = NO_ERROR;
    buffer_handle_t *handle = NULL;
    int i;
    uint32_t x, y;
    android::GraphicBufferMapper &mapper = android::GraphicBufferMapper::get();

    android::AutoMutex lock(mLock);

    if (!mBuffers || !frame->mBuffer) {
        CAMHAL_LOGEA("Adapter sent BufferSourceAdapter a NULL frame?");
        return;
    }

    for ( i = 0; i < mBufferCount; i++ ) {
        if (frame->mBuffer == &mBuffers[i]) {
            break;
        }
    }

    if (i >= mBufferCount) {
        CAMHAL_LOGD("Can't find frame in buffer list");
        if (frame->mFrameType != CameraFrame::REPROCESS_INPUT_FRAME) {
            mFrameProvider->returnFrame(frame->mBuffer,
                    static_cast<CameraFrame::FrameType>(frame->mFrameType));
        }
        return;
    }

    handle = (buffer_handle_t *) mBuffers[i].opaque;

    // Handle input buffers
    // TODO(XXX): Move handling of input buffers out of here if
    // it becomes more complex
    if (frame->mFrameType == CameraFrame::REPROCESS_INPUT_FRAME) {
        CAMHAL_LOGD("Unlock %p (buffer #%d)", handle, i);
        mapper.unlock(*handle);
        return;
    }

    CameraHal::getXYFromOffset(&x, &y, frame->mOffset, frame->mAlignment, mPixelFormat);
    CAMHAL_LOGVB("offset = %u left = %d top = %d right = %d bottom = %d",
                  frame->mOffset, x, y, x + frame->mWidth, y + frame->mHeight);
    ret = mBufferSource->set_crop(mBufferSource, x, y, x + frame->mWidth, y + frame->mHeight);
    if (NO_ERROR != ret) {
        CAMHAL_LOGE("mBufferSource->set_crop returned error %d", ret);
        goto fail;
    }

    if ( NULL != frame->mMetaData.get() ) {
        camera_memory_t *extMeta = frame->mMetaData->getExtendedMetadata();
        if ( NULL != extMeta ) {
            camera_metadata_t *metaData = static_cast<camera_metadata_t *> (extMeta->data);
            metaData->timestamp = frame->mTimestamp;
            ret = extendedOps()->set_metadata(mBufferSource, extMeta);
            if (ret != 0) {
                CAMHAL_LOGE("Surface::set_metadata returned error %d", ret);
                goto fail;
            }
        }
    }

    // unlock buffer before enqueueing
    mapper.unlock(*handle);

    ret = mBufferSource->enqueue_buffer(mBufferSource, handle);
    if (ret != 0) {
        CAMHAL_LOGE("Surface::queueBuffer returned error %d", ret);
        goto fail;
    }

    mFramesWithCameraAdapterMap.removeItem((buffer_handle_t *) frame->mBuffer->opaque);

    return;

fail:
    mFramesWithCameraAdapterMap.clear();
    mBufferSource = NULL;
    mReturnFrame->requestExit();
    mQueueFrame->requestExit();
}


bool BufferSourceAdapter::handleFrameReturn()
{
    status_t err;
    buffer_handle_t *buf;
    int i = 0;
    int stride;  // dummy variable to get stride
    CameraFrame::FrameType type;
    android::GraphicBufferMapper &mapper = android::GraphicBufferMapper::get();
    void *y_uv[2];
    android::Rect bounds(mFrameWidth, mFrameHeight);

    android::AutoMutex lock(mLock);

    if ( (NULL == mBufferSource) || (NULL == mBuffers) ) {
        return false;
    }

    err = mBufferSource->dequeue_buffer(mBufferSource, &buf, &stride);
    if (err != 0) {
        CAMHAL_LOGEB("dequeueBuffer failed: %s (%d)", strerror(-err), -err);

        if ( ENODEV == err ) {
            CAMHAL_LOGEA("Preview surface abandoned!");
            mBufferSource = NULL;
        }

        return false;
    }

    err = mBufferSource->lock_buffer(mBufferSource, buf);
    if (err != 0) {
        CAMHAL_LOGEB("lockbuffer failed: %s (%d)", strerror(-err), -err);

        if ( ENODEV == err ) {
            CAMHAL_LOGEA("Preview surface abandoned!");
            mBufferSource = NULL;
        }

        return false;
    }

    mapper.lock(*buf, CAMHAL_GRALLOC_USAGE, bounds, y_uv);

    for(i = 0; i < mBufferCount; i++) {
        if (mBuffers[i].opaque == buf)
            break;
    }

    if (i >= mBufferCount) {
        CAMHAL_LOGEB("Failed to find handle %p", buf);
    }

    mFramesWithCameraAdapterMap.add((buffer_handle_t *) mBuffers[i].opaque, i);

    CAMHAL_LOGVB("handleFrameReturn: found graphic buffer %d of %d", i, mBufferCount - 1);

    mFrameProvider->returnFrame(&mBuffers[i], formatToOutputFrameType(mPixelFormat));
    return true;
}

void BufferSourceAdapter::frameCallback(CameraFrame* caFrame)
{
    if ((NULL != caFrame) && (NULL != caFrame->mCookie)) {
        BufferSourceAdapter *da = (BufferSourceAdapter*) caFrame->mCookie;
        da->addFrame(caFrame);
    } else {
        CAMHAL_LOGEB("Invalid Cookie in Camera Frame = %p, Cookie = %p",
                    caFrame, caFrame ? caFrame->mCookie : NULL);
    }
}

/*--------------------BufferSourceAdapter Class ENDS here-----------------------------*/

} // namespace Camera
} // namespace Ti

#endif
