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

// Utility classes for camera2 HAL testing

#define LOG_TAG "Camera2_test_utils"
#define LOG_NDEBUG 0

#include "utils/Log.h"
#include "camera2_utils.h"
#include <dlfcn.h>

namespace android {
namespace camera2 {
namespace tests {

/**
 * MetadataQueue
 */

MetadataQueue::MetadataQueue():
            mDevice(NULL),
            mFrameCount(0),
            mCount(0),
            mStreamSlotCount(0),
            mSignalConsumer(true)
{
    camera2_request_queue_src_ops::dequeue_request = consumer_dequeue;
    camera2_request_queue_src_ops::request_count = consumer_buffer_count;
    camera2_request_queue_src_ops::free_request = consumer_free;

    camera2_frame_queue_dst_ops::dequeue_frame = producer_dequeue;
    camera2_frame_queue_dst_ops::cancel_frame = producer_cancel;
    camera2_frame_queue_dst_ops::enqueue_frame = producer_enqueue;
}

MetadataQueue::~MetadataQueue() {
    freeBuffers(mEntries.begin(), mEntries.end());
    freeBuffers(mStreamSlot.begin(), mStreamSlot.end());
}

// Interface to camera2 HAL as consumer (input requests/reprocessing)
const camera2_request_queue_src_ops_t* MetadataQueue::getToConsumerInterface() {
    return static_cast<camera2_request_queue_src_ops_t*>(this);
}

void MetadataQueue::setFromConsumerInterface(camera2_device_t *d) {
    mDevice = d;
}

const camera2_frame_queue_dst_ops_t* MetadataQueue::getToProducerInterface() {
    return static_cast<camera2_frame_queue_dst_ops_t*>(this);
}

// Real interfaces
status_t MetadataQueue::enqueue(camera_metadata_t *buf) {
    Mutex::Autolock l(mMutex);

    mCount++;
    mEntries.push_back(buf);
    notEmpty.signal();

    if (mSignalConsumer && mDevice != NULL) {
        mSignalConsumer = false;

        mMutex.unlock();
        ALOGV("%s: Signaling consumer", __FUNCTION__);
        mDevice->ops->notify_request_queue_not_empty(mDevice);
        mMutex.lock();
    }
    return OK;
}

int MetadataQueue::getBufferCount() {
    Mutex::Autolock l(mMutex);
    if (mStreamSlotCount > 0) {
        return CAMERA2_REQUEST_QUEUE_IS_BOTTOMLESS;
    }
    return mCount;
}

status_t MetadataQueue::dequeue(camera_metadata_t **buf, bool incrementCount) {
    Mutex::Autolock l(mMutex);

    if (mCount == 0) {
        if (mStreamSlotCount == 0) {
            ALOGV("%s: Empty", __FUNCTION__);
            *buf = NULL;
            mSignalConsumer = true;
            return OK;
        }
        ALOGV("%s: Streaming %d frames to queue", __FUNCTION__,
              mStreamSlotCount);

        for (List<camera_metadata_t*>::iterator slotEntry = mStreamSlot.begin();
                slotEntry != mStreamSlot.end();
                slotEntry++ ) {
            size_t entries = get_camera_metadata_entry_count(*slotEntry);
            size_t dataBytes = get_camera_metadata_data_count(*slotEntry);

            camera_metadata_t *copy = allocate_camera_metadata(entries, dataBytes);
            append_camera_metadata(copy, *slotEntry);
            mEntries.push_back(copy);
        }
        mCount = mStreamSlotCount;
    }
    ALOGV("MetadataQueue: deque (%d buffers)", mCount);
    camera_metadata_t *b = *(mEntries.begin());
    mEntries.erase(mEntries.begin());

    if (incrementCount) {
        add_camera_metadata_entry(b,
                ANDROID_REQUEST_FRAME_COUNT,
                (void**)&mFrameCount, 1);
        mFrameCount++;
    }

    *buf = b;
    mCount--;

    return OK;
}

status_t MetadataQueue::waitForBuffer(nsecs_t timeout) {
    Mutex::Autolock l(mMutex);
    status_t res;
    while (mCount == 0) {
        res = notEmpty.waitRelative(mMutex,timeout);
        if (res != OK) return res;
    }
    return OK;
}

status_t MetadataQueue::setStreamSlot(camera_metadata_t *buf) {
    if (buf == NULL) {
        freeBuffers(mStreamSlot.begin(), mStreamSlot.end());
        mStreamSlotCount = 0;
        return OK;
    }
    if (mStreamSlotCount > 1) {
        List<camera_metadata_t*>::iterator deleter = ++mStreamSlot.begin();
        freeBuffers(++mStreamSlot.begin(), mStreamSlot.end());
        mStreamSlotCount = 1;
    }
    if (mStreamSlotCount == 1) {
        free_camera_metadata( *(mStreamSlot.begin()) );
        *(mStreamSlot.begin()) = buf;
    } else {
        mStreamSlot.push_front(buf);
        mStreamSlotCount = 1;
    }
    return OK;
}

status_t MetadataQueue::setStreamSlot(const List<camera_metadata_t*> &bufs) {
    if (mStreamSlotCount > 0) {
        freeBuffers(mStreamSlot.begin(), mStreamSlot.end());
    }
    mStreamSlot = bufs;
    mStreamSlotCount = mStreamSlot.size();

    return OK;
}

status_t MetadataQueue::freeBuffers(List<camera_metadata_t*>::iterator start,
                                    List<camera_metadata_t*>::iterator end) {
    while (start != end) {
        free_camera_metadata(*start);
        start = mStreamSlot.erase(start);
    }
    return OK;
}

MetadataQueue* MetadataQueue::getInstance(
        const camera2_request_queue_src_ops_t *q) {
    const MetadataQueue* cmq = static_cast<const MetadataQueue*>(q);
    return const_cast<MetadataQueue*>(cmq);
}

MetadataQueue* MetadataQueue::getInstance(
        const camera2_frame_queue_dst_ops_t *q) {
    const MetadataQueue* cmq = static_cast<const MetadataQueue*>(q);
    return const_cast<MetadataQueue*>(cmq);
}

int MetadataQueue::consumer_buffer_count(
        const camera2_request_queue_src_ops_t *q) {
    MetadataQueue *queue = getInstance(q);
    return queue->getBufferCount();
}

int MetadataQueue::consumer_dequeue(const camera2_request_queue_src_ops_t *q,
        camera_metadata_t **buffer) {
    MetadataQueue *queue = getInstance(q);
    return queue->dequeue(buffer, true);
}

int MetadataQueue::consumer_free(const camera2_request_queue_src_ops_t *q,
        camera_metadata_t *old_buffer) {
    free_camera_metadata(old_buffer);
    return OK;
}

int MetadataQueue::producer_dequeue(const camera2_frame_queue_dst_ops_t *q,
        size_t entries, size_t bytes,
        camera_metadata_t **buffer) {
    camera_metadata_t *new_buffer =
            allocate_camera_metadata(entries, bytes);
    if (new_buffer == NULL) return NO_MEMORY;
    *buffer = new_buffer;
        return OK;
}

int MetadataQueue::producer_cancel(const camera2_frame_queue_dst_ops_t *q,
        camera_metadata_t *old_buffer) {
    free_camera_metadata(old_buffer);
    return OK;
}

int MetadataQueue::producer_enqueue(const camera2_frame_queue_dst_ops_t *q,
        camera_metadata_t *filled_buffer) {
    MetadataQueue *queue = getInstance(q);
    return queue->enqueue(filled_buffer);
}

/**
 * NotifierListener
 */

NotifierListener::NotifierListener() {
}

status_t NotifierListener::getNotificationsFrom(camera2_device *dev) {
    if (!dev) return BAD_VALUE;
    status_t err;
    err = dev->ops->set_notify_callback(dev,
            notify_callback_dispatch,
            (void*)this);
    return err;
}

status_t NotifierListener::getNextNotification(int32_t *msg_type,
        int32_t *ext1,
        int32_t *ext2,
        int32_t *ext3) {
    Mutex::Autolock l(mMutex);
    if (mNotifications.size() == 0) return BAD_VALUE;
    return getNextNotificationLocked(msg_type, ext1, ext2, ext3);
}

status_t NotifierListener::waitForNotification(int32_t *msg_type,
        int32_t *ext1,
        int32_t *ext2,
        int32_t *ext3) {
    Mutex::Autolock l(mMutex);
    while (mNotifications.size() == 0) {
        mNewNotification.wait(mMutex);
    }
    return getNextNotificationLocked(msg_type, ext1, ext2, ext3);
}

int NotifierListener::numNotifications() {
    Mutex::Autolock l(mMutex);
    return mNotifications.size();
}

status_t NotifierListener::getNextNotificationLocked(int32_t *msg_type,
        int32_t *ext1,
        int32_t *ext2,
        int32_t *ext3) {
    *msg_type = mNotifications.begin()->msg_type;
    *ext1 = mNotifications.begin()->ext1;
    *ext2 = mNotifications.begin()->ext2;
    *ext3 = mNotifications.begin()->ext3;
    mNotifications.erase(mNotifications.begin());
    return OK;
}

void NotifierListener::onNotify(int32_t msg_type,
        int32_t ext1,
        int32_t ext2,
        int32_t ext3) {
    Mutex::Autolock l(mMutex);
    mNotifications.push_back(Notification(msg_type, ext1, ext2, ext3));
    mNewNotification.signal();
}

void NotifierListener::notify_callback_dispatch(int32_t msg_type,
        int32_t ext1,
        int32_t ext2,
        int32_t ext3,
        void *user) {
    NotifierListener *me = reinterpret_cast<NotifierListener*>(user);
    me->onNotify(msg_type, ext1, ext2, ext3);
}

/**
 * StreamAdapter
 */

#ifndef container_of
#define container_of(ptr, type, member) \
    (type *)((char*)(ptr) - offsetof(type, member))
#endif

StreamAdapter::StreamAdapter(sp<IGraphicBufferProducer> consumer):
        mState(UNINITIALIZED), mDevice(NULL),
        mId(-1),
        mWidth(0), mHeight(0), mFormat(0)
{
    mConsumerInterface = new Surface(consumer);
    camera2_stream_ops::dequeue_buffer = dequeue_buffer;
    camera2_stream_ops::enqueue_buffer = enqueue_buffer;
    camera2_stream_ops::cancel_buffer = cancel_buffer;
    camera2_stream_ops::set_crop = set_crop;
}

StreamAdapter::~StreamAdapter() {
    disconnect();
}

status_t StreamAdapter::connectToDevice(camera2_device_t *d,
        uint32_t width, uint32_t height, int format) {
    if (mState != UNINITIALIZED) return INVALID_OPERATION;
    if (d == NULL) {
        ALOGE("%s: Null device passed to stream adapter", __FUNCTION__);
        return BAD_VALUE;
    }

    status_t res;

    mWidth = width;
    mHeight = height;
    mFormat = format;

    // Allocate device-side stream interface

    uint32_t id;
    uint32_t formatActual; // ignored
    uint32_t usage;
    uint32_t maxBuffers = 2;
    res = d->ops->allocate_stream(d,
            mWidth, mHeight, mFormat, getStreamOps(),
            &id, &formatActual, &usage, &maxBuffers);
    if (res != OK) {
        ALOGE("%s: Device stream allocation failed: %s (%d)",
                __FUNCTION__, strerror(-res), res);
        mState = UNINITIALIZED;
        return res;
    }
    mDevice = d;

    mId = id;
    mUsage = usage;
    mMaxProducerBuffers = maxBuffers;

    // Configure consumer-side ANativeWindow interface

    res = native_window_api_connect(mConsumerInterface.get(),
            NATIVE_WINDOW_API_CAMERA);
    if (res != OK) {
        ALOGE("%s: Unable to connect to native window for stream %d",
                __FUNCTION__, mId);
        mState = ALLOCATED;
        return res;
    }

    res = native_window_set_usage(mConsumerInterface.get(), mUsage);
    if (res != OK) {
        ALOGE("%s: Unable to configure usage %08x for stream %d",
                __FUNCTION__, mUsage, mId);
        mState = CONNECTED;
        return res;
    }

    res = native_window_set_buffers_geometry(mConsumerInterface.get(),
            mWidth, mHeight, mFormat);
    if (res != OK) {
        ALOGE("%s: Unable to configure buffer geometry"
                " %d x %d, format 0x%x for stream %d",
                __FUNCTION__, mWidth, mHeight, mFormat, mId);
        mState = CONNECTED;
        return res;
    }

    int maxConsumerBuffers;
    res = mConsumerInterface->query(mConsumerInterface.get(),
            NATIVE_WINDOW_MIN_UNDEQUEUED_BUFFERS, &maxConsumerBuffers);
    if (res != OK) {
        ALOGE("%s: Unable to query consumer undequeued"
                " buffer count for stream %d", __FUNCTION__, mId);
        mState = CONNECTED;
        return res;
    }
    mMaxConsumerBuffers = maxConsumerBuffers;

    ALOGV("%s: Producer wants %d buffers, consumer wants %d", __FUNCTION__,
            mMaxProducerBuffers, mMaxConsumerBuffers);

    int totalBuffers = mMaxConsumerBuffers + mMaxProducerBuffers;

    res = native_window_set_buffer_count(mConsumerInterface.get(),
            totalBuffers);
    if (res != OK) {
        ALOGE("%s: Unable to set buffer count for stream %d",
                __FUNCTION__, mId);
        mState = CONNECTED;
        return res;
    }

    // Register allocated buffers with HAL device
    buffer_handle_t *buffers = new buffer_handle_t[totalBuffers];
    ANativeWindowBuffer **anwBuffers = new ANativeWindowBuffer*[totalBuffers];
    int bufferIdx = 0;
    for (; bufferIdx < totalBuffers; bufferIdx++) {
        res = native_window_dequeue_buffer_and_wait(mConsumerInterface.get(),
                &anwBuffers[bufferIdx]);
        if (res != OK) {
            ALOGE("%s: Unable to dequeue buffer %d for initial registration for"
                    "stream %d", __FUNCTION__, bufferIdx, mId);
            mState = CONNECTED;
            goto cleanUpBuffers;
        }
        buffers[bufferIdx] = anwBuffers[bufferIdx]->handle;
    }

    res = mDevice->ops->register_stream_buffers(mDevice,
            mId,
            totalBuffers,
            buffers);
    if (res != OK) {
        ALOGE("%s: Unable to register buffers with HAL device for stream %d",
                __FUNCTION__, mId);
        mState = CONNECTED;
    } else {
        mState = ACTIVE;
    }

cleanUpBuffers:
    for (int i = 0; i < bufferIdx; i++) {
        res = mConsumerInterface->cancelBuffer(mConsumerInterface.get(),
                anwBuffers[i], -1);
    }
    delete anwBuffers;
    delete buffers;

    return res;
}

status_t StreamAdapter::disconnect() {
    status_t res;
    if (mState >= ALLOCATED) {
        res = mDevice->ops->release_stream(mDevice, mId);
        if (res != OK) {
            ALOGE("%s: Unable to release stream %d",
                    __FUNCTION__, mId);
            return res;
        }
    }
    if (mState >= CONNECTED) {
        res = native_window_api_disconnect(mConsumerInterface.get(),
                NATIVE_WINDOW_API_CAMERA);
        if (res != OK) {
            ALOGE("%s: Unable to disconnect stream %d from native window",
                    __FUNCTION__, mId);
            return res;
        }
    }
    mId = -1;
    mState = DISCONNECTED;
    return OK;
}

int StreamAdapter::getId() {
    return mId;
}

const camera2_stream_ops *StreamAdapter::getStreamOps() {
    return static_cast<camera2_stream_ops *>(this);
}

ANativeWindow* StreamAdapter::toANW(const camera2_stream_ops_t *w) {
    return static_cast<const StreamAdapter*>(w)->mConsumerInterface.get();
}

int StreamAdapter::dequeue_buffer(const camera2_stream_ops_t *w,
        buffer_handle_t** buffer) {
    int res;
    int state = static_cast<const StreamAdapter*>(w)->mState;
    if (state != ACTIVE) {
        ALOGE("%s: Called when in bad state: %d", __FUNCTION__, state);
        return INVALID_OPERATION;
    }

    ANativeWindow *a = toANW(w);
    ANativeWindowBuffer* anb;
    res = native_window_dequeue_buffer_and_wait(a, &anb);
    if (res != OK) return res;

    *buffer = &(anb->handle);

    return res;
}

int StreamAdapter::enqueue_buffer(const camera2_stream_ops_t* w,
        int64_t timestamp,
        buffer_handle_t* buffer) {
    int state = static_cast<const StreamAdapter*>(w)->mState;
    if (state != ACTIVE) {
        ALOGE("%s: Called when in bad state: %d", __FUNCTION__, state);
        return INVALID_OPERATION;
    }
    ANativeWindow *a = toANW(w);
    status_t err;
    err = native_window_set_buffers_timestamp(a, timestamp);
    if (err != OK) return err;
    return a->queueBuffer(a,
            container_of(buffer, ANativeWindowBuffer, handle), -1);
}

int StreamAdapter::cancel_buffer(const camera2_stream_ops_t* w,
        buffer_handle_t* buffer) {
    int state = static_cast<const StreamAdapter*>(w)->mState;
    if (state != ACTIVE) {
        ALOGE("%s: Called when in bad state: %d", __FUNCTION__, state);
        return INVALID_OPERATION;
    }
    ANativeWindow *a = toANW(w);
    return a->cancelBuffer(a,
            container_of(buffer, ANativeWindowBuffer, handle), -1);
}

int StreamAdapter::set_crop(const camera2_stream_ops_t* w,
        int left, int top, int right, int bottom) {
    int state = static_cast<const StreamAdapter*>(w)->mState;
    if (state != ACTIVE) {
        ALOGE("%s: Called when in bad state: %d", __FUNCTION__, state);
        return INVALID_OPERATION;
    }
    ANativeWindow *a = toANW(w);
    android_native_rect_t crop = { left, top, right, bottom };
    return native_window_set_crop(a, &crop);
}

/**
 * FrameWaiter
 */

FrameWaiter::FrameWaiter():
        mPendingFrames(0) {
}

status_t FrameWaiter::waitForFrame(nsecs_t timeout) {
    status_t res;
    Mutex::Autolock lock(mMutex);
    while (mPendingFrames == 0) {
        res = mCondition.waitRelative(mMutex, timeout);
        if (res != OK) return res;
    }
    mPendingFrames--;
    return OK;
}

void FrameWaiter::onFrameAvailable(const BufferItem& /* item */) {
    Mutex::Autolock lock(mMutex);
    mPendingFrames++;
    mCondition.signal();
}

int HWModuleHelpers::closeModule(hw_module_t* module) {
    int status;

    if (!module) {
        return -EINVAL;
    }

    status = dlclose(module->dso);
    if (status != 0) {
        char const *err_str = dlerror();
        ALOGE("%s dlclose failed, error: %s", __func__, err_str ?: "unknown");
    }

    return status;
}

} // namespace tests
} // namespace camera2
} // namespace android
