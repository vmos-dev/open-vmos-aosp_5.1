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

#ifndef __ANDROID_HAL_CAMERA2_TESTS_UTILS__
#define __ANDROID_HAL_CAMERA2_TESTS_UTILS__

// Utility classes for camera2 HAL testing

#include <system/camera_metadata.h>
#include <hardware/camera2.h>

#include <gui/Surface.h>
#include <gui/CpuConsumer.h>

#include <utils/List.h>
#include <utils/Mutex.h>
#include <utils/Condition.h>

namespace android {
namespace camera2 {
namespace tests {

/**
 * Queue class for both sending requests to a camera2 device, and for receiving
 * frames from a camera2 device.
 */
class MetadataQueue: public camera2_request_queue_src_ops_t,
                    public camera2_frame_queue_dst_ops_t {
  public:
    MetadataQueue();
    ~MetadataQueue();

    // Interface to camera2 HAL device, either for requests (device is consumer)
    // or for frames (device is producer)
    const camera2_request_queue_src_ops_t*   getToConsumerInterface();
    void setFromConsumerInterface(camera2_device_t *d);

    const camera2_frame_queue_dst_ops_t* getToProducerInterface();

    // Real interfaces. On enqueue, queue takes ownership of buffer pointer
    // On dequeue, user takes ownership of buffer pointer.
    status_t enqueue(camera_metadata_t *buf);
    status_t dequeue(camera_metadata_t **buf, bool incrementCount = true);
    int      getBufferCount();
    status_t waitForBuffer(nsecs_t timeout);

    // Set repeating buffer(s); if the queue is empty on a dequeue call, the
    // queue copies the contents of the stream slot into the queue, and then
    // dequeues the first new entry.
    status_t setStreamSlot(camera_metadata_t *buf);
    status_t setStreamSlot(const List<camera_metadata_t*> &bufs);

  private:
    status_t freeBuffers(List<camera_metadata_t*>::iterator start,
                         List<camera_metadata_t*>::iterator end);

    camera2_device_t *mDevice;

    Mutex mMutex;
    Condition notEmpty;

    int mFrameCount;

    int mCount;
    List<camera_metadata_t*> mEntries;
    int mStreamSlotCount;
    List<camera_metadata_t*> mStreamSlot;

    bool mSignalConsumer;

    static MetadataQueue* getInstance(const camera2_frame_queue_dst_ops_t *q);
    static MetadataQueue* getInstance(const camera2_request_queue_src_ops_t *q);

    static int consumer_buffer_count(const camera2_request_queue_src_ops_t *q);

    static int consumer_dequeue(const camera2_request_queue_src_ops_t *q,
            camera_metadata_t **buffer);

    static int consumer_free(const camera2_request_queue_src_ops_t *q,
            camera_metadata_t *old_buffer);

    static int producer_dequeue(const camera2_frame_queue_dst_ops_t *q,
            size_t entries, size_t bytes,
            camera_metadata_t **buffer);

    static int producer_cancel(const camera2_frame_queue_dst_ops_t *q,
            camera_metadata_t *old_buffer);

    static int producer_enqueue(const camera2_frame_queue_dst_ops_t *q,
            camera_metadata_t *filled_buffer);

};

/**
 * Basic class to receive and queue up notifications from the camera device
 */

class NotifierListener {
  public:

    NotifierListener();

    status_t getNotificationsFrom(camera2_device *dev);

    status_t getNextNotification(int32_t *msg_type, int32_t *ext1,
            int32_t *ext2, int32_t *ext3);

    status_t waitForNotification(int32_t *msg_type, int32_t *ext1,
            int32_t *ext2, int32_t *ext3);

    int numNotifications();

  private:

    status_t getNextNotificationLocked(int32_t *msg_type,
            int32_t *ext1, int32_t *ext2, int32_t *ext3);

    struct Notification {
        Notification(int32_t type, int32_t e1, int32_t e2, int32_t e3):
                msg_type(type),
                ext1(e1),
                ext2(e2),
                ext3(e3)
        {}

        int32_t msg_type;
        int32_t ext1;
        int32_t ext2;
        int32_t ext3;
    };

    List<Notification> mNotifications;

    Mutex mMutex;
    Condition mNewNotification;

    void onNotify(int32_t msg_type,
            int32_t ext1,
            int32_t ext2,
            int32_t ext3);

    static void notify_callback_dispatch(int32_t msg_type,
            int32_t ext1,
            int32_t ext2,
            int32_t ext3,
            void *user);

};

/**
 * Adapter from an IGraphicBufferProducer interface to camera2 device stream ops.
 * Also takes care of allocating/deallocating stream in device interface
 */
class StreamAdapter: public camera2_stream_ops {
  public:
    StreamAdapter(sp<IGraphicBufferProducer> consumer);

    ~StreamAdapter();

    status_t connectToDevice(camera2_device_t *d,
            uint32_t width, uint32_t height, int format);

    status_t disconnect();

    // Get stream ID. Only valid after a successful connectToDevice call.
    int      getId();

  private:
    enum {
        ERROR = -1,
        DISCONNECTED = 0,
        UNINITIALIZED,
        ALLOCATED,
        CONNECTED,
        ACTIVE
    } mState;

    sp<ANativeWindow> mConsumerInterface;
    camera2_device_t *mDevice;

    uint32_t mId;
    uint32_t mWidth;
    uint32_t mHeight;
    uint32_t mFormat;
    uint32_t mUsage;
    uint32_t mMaxProducerBuffers;
    uint32_t mMaxConsumerBuffers;

    const camera2_stream_ops *getStreamOps();

    static ANativeWindow* toANW(const camera2_stream_ops_t *w);

    static int dequeue_buffer(const camera2_stream_ops_t *w,
            buffer_handle_t** buffer);

    static int enqueue_buffer(const camera2_stream_ops_t* w,
            int64_t timestamp,
            buffer_handle_t* buffer);

    static int cancel_buffer(const camera2_stream_ops_t* w,
            buffer_handle_t* buffer);

    static int set_crop(const camera2_stream_ops_t* w,
            int left, int top, int right, int bottom);

};

/**
 * Simple class to wait on the CpuConsumer to have a frame available
 */
class FrameWaiter : public CpuConsumer::FrameAvailableListener {
  public:
    FrameWaiter();

    /**
     * Wait for max timeout nanoseconds for a new frame. Returns
     * OK if a frame is available, TIMED_OUT if the timeout was reached.
     */
    status_t waitForFrame(nsecs_t timeout);

    virtual void onFrameAvailable(const BufferItem& item);

    int mPendingFrames;
    Mutex mMutex;
    Condition mCondition;
};

struct HWModuleHelpers {
    /* attempt to unload the library with dlclose */
    static int closeModule(hw_module_t* module);
};

}
}
}

#endif
