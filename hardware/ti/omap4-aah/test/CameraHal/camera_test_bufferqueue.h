#ifndef CAMERA_TEST_BUFFER_QUEUE_H
#define CAMERA_TEST_BUFFER_QUEUE_H

#ifdef ANDROID_API_JB_OR_LATER

#include <gui/Surface.h>
#include <gui/GLConsumer.h>
#include <gui/SurfaceComposerClient.h>

#include "camera_test.h"

#define CAMHAL_LOGV            ALOGV
#define CAMHAL_LOGE            ALOGE
#define PRINTOVER(arg...)      ALOGD(#arg)
#define LOG_FUNCTION_NAME      ALOGD("%d: %s() ENTER", __LINE__, __FUNCTION__);
#define LOG_FUNCTION_NAME_EXIT ALOGD("%d: %s() EXIT", __LINE__, __FUNCTION__);

using namespace android;

class FrameConsumer : public BufferQueue::ProxyConsumerListener {
public:
    FrameConsumer():
            BufferQueue::ProxyConsumerListener(NULL), mPendingFrames(0) {
    }

    virtual ~FrameConsumer() {
        onFrameAvailable();
    }

    void waitForFrame() {
        Mutex::Autolock lock(mMutex);
        while (mPendingFrames == 0) {
            mCondition.wait(mMutex);
        }
        mPendingFrames--;
    }

    virtual void onFrameAvailable() {
        Mutex::Autolock lock(mMutex);
        mPendingFrames++;
        mCondition.signal();
    }
    
    virtual void onBuffersReleased() {}

    int mPendingFrames;
    Mutex mMutex;
    Condition mCondition;
};

class BQ_BufferSourceThread : public BufferSourceThread {
public:
    BQ_BufferSourceThread(int tex_id, sp<Camera> camera) : BufferSourceThread(camera) {
        mBufferQueue = new BufferQueue(true, 1);
        mFW = new FrameConsumer();
        mBufferQueue->setSynchronousMode(true);
        mBufferQueue->consumerConnect(mFW);
        mCamera->setBufferSource(NULL, mBufferQueue);
    }
    virtual ~BQ_BufferSourceThread() {
        mCamera->releaseBufferSource(NULL, mBufferQueue);
    }

    virtual bool threadLoop() {
        sp<GraphicBuffer> graphic_buffer;
        BufferQueue::BufferItem item;

        mFW->waitForFrame();
        if (!mDestroying) {
            status_t status;
            status = mBufferQueue->acquireBuffer(&item, 0);
            if (status == BufferQueue::NO_BUFFER_AVAILABLE) {
                // no buffer to handle, return and we'll try again
                return true;
            }
            printf("=== Metadata for buffer %d ===\n", mCounter);
            if (item.mGraphicBuffer != NULL) {
                unsigned int slot = item.mBuf;
                // For whatever reason, BufferQueue only gives us the graphic buffer
                // the first time we acquire it. We are expected to hold a reference to
                // it there after...
                mBufferSlots[slot].mGraphicBuffer = item.mGraphicBuffer;
                mBufferSlots[slot].mCrop = item.mCrop;
            }
            showMetadata(item.mMetadata);
            printf("\n");
            graphic_buffer = mBufferSlots[item.mBuf].mGraphicBuffer;
            mDeferThread->add(graphic_buffer, item.mCrop, mCounter++, item.mBuf);
            restartCapture();
            return true;
        }
        return false;
    }

    virtual void requestExit() {
        Thread::requestExit();

        mDestroying = true;
        mFW->onFrameAvailable();
    }

    virtual void setBuffer(android::ShotParameters &params) {
        {
            String8 id = mBufferQueue->getId();

            if (!id.isEmpty()) {
                params.set(KEY_TAP_OUT_SURFACES, id);
            } else {
                params.remove(KEY_TAP_OUT_SURFACES);
            }
        }
    }

    virtual void onHandled(sp<GraphicBuffer> &gbuf, unsigned int slot) {
        mBufferQueue->releaseBuffer(slot, EGL_NO_DISPLAY, EGL_NO_SYNC_KHR);
    }

private:
    sp<BufferQueue> mBufferQueue;
    sp<FrameConsumer> mFW;
    BufferQueue::BufferItem mBufferSlots[BufferQueue::NUM_BUFFER_SLOTS];
};

class BQ_BufferSourceInput : public BufferSourceInput {
public:
    BQ_BufferSourceInput(int tex_id, sp<Camera> camera) :
                  BufferSourceInput(camera), mTexId(tex_id) {
        mBufferQueue = new BufferQueue(true, 1);
        sp<IGraphicBufferProducer> bufferProducer = mBufferQueue;
        mWindowTapIn = new Surface(bufferProducer);
        mCamera->setBufferSource(mBufferQueue, NULL);
    }
    virtual ~BQ_BufferSourceInput() {
        mCamera->releaseBufferSource(mBufferQueue, NULL);
    }

    virtual void setInput(buffer_info_t bufinfo, const char *format, android::ShotParameters &params) {
        mBufferQueue->setDefaultBufferSize(bufinfo.width, bufinfo.height);
        BufferSourceInput::setInput(bufinfo, format, params);
        {
            String8 id = mBufferQueue->getId();

            if (!id.isEmpty()) {
                params.set(KEY_TAP_IN_SURFACE, id);
            } else {
                params.remove(KEY_TAP_IN_SURFACE);
            }
        }
    }

private:
    sp<BufferQueue> mBufferQueue;
    int mTexId;
};
#endif // ANDROID_API_JB_OR_LATER
#endif // CAMERA_TEST_BUFFER_QUEUE_H
