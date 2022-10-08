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

#ifndef BUFFER_SOURCE_ADAPTER_H
#define BUFFER_SOURCE_ADAPTER_H

#ifdef OMAP_ENHANCEMENT_CPCAM

#include "CameraHal.h"
#include <ui/GraphicBufferMapper.h>
#include <hal_public.h>

namespace Ti {
namespace Camera {

/**
 * Handles enqueueing/dequeing buffers to tap-in/tap-out points
 * TODO(XXX): this class implements DisplayAdapter for now
 * but this will most likely change once tap-in/tap-out points
 * are better defined
 */

class BufferSourceAdapter : public DisplayAdapter
{
// private types
private:
    ///Constant declarations
    static const int NO_BUFFERS_IMAGE_CAPTURE_SYSTEM_HEAP;


    // helper class to return frame in different thread context
    class ReturnFrame : public android::Thread {
    public:
        ReturnFrame(BufferSourceAdapter* __this) : mBufferSourceAdapter(__this) {
            android::AutoMutex lock(mReturnFrameMutex);
            mDestroying = false;
            mFrameCount = 0;
        }

        ~ReturnFrame() {
            android::AutoMutex lock(mReturnFrameMutex);
         }

        void signal() {
            android::AutoMutex lock(mReturnFrameMutex);
            mFrameCount++;
            mReturnFrameCondition.signal();
        }

        virtual void requestExit() {
            Thread::requestExit();

            android::AutoMutex lock(mReturnFrameMutex);
            mDestroying = true;
            mReturnFrameCondition.signal();
        }

        virtual bool threadLoop() {
            android::AutoMutex lock(mReturnFrameMutex);
            if ( 0 >= mFrameCount ) {
                mReturnFrameCondition.wait(mReturnFrameMutex);
            }
            if (!mDestroying) {
                mBufferSourceAdapter->handleFrameReturn();
                mFrameCount--;
            }
            return true;
        }

    private:
        BufferSourceAdapter* mBufferSourceAdapter;
        android::Condition mReturnFrameCondition;
        android::Mutex mReturnFrameMutex;
        int mFrameCount;
        bool mDestroying;
    };

    // helper class to queue frame in different thread context
    class QueueFrame : public android::Thread {
    public:
        QueueFrame(BufferSourceAdapter* __this) : mBufferSourceAdapter(__this) {
            mDestroying = false;
        }

        ~QueueFrame() {
         }

        void addFrame(CameraFrame *frame) {
            android::AutoMutex lock(mFramesMutex);
            mFrames.add(new CameraFrame(*frame));
            mFramesCondition.signal();
        }

        virtual void requestExit() {
            Thread::requestExit();

            mDestroying = true;

            android::AutoMutex lock(mFramesMutex);
            while (!mFrames.empty()) {
                CameraFrame *frame = mFrames.itemAt(0);
                mFrames.removeAt(0);
                frame->mMetaData.clear();
                delete frame;
            }
            mFramesCondition.signal();
        }

        virtual bool threadLoop() {
            CameraFrame *frame = NULL;
            {
                android::AutoMutex lock(mFramesMutex);
                while (mFrames.empty() && !mDestroying) mFramesCondition.wait(mFramesMutex);
                if (!mDestroying) {
                    frame = mFrames.itemAt(0);
                    mFrames.removeAt(0);
                }
            }

            if (frame) {
                mBufferSourceAdapter->handleFrameCallback(frame);
                frame->mMetaData.clear();

                // signal return frame thread that it can dequeue a buffer now
                mBufferSourceAdapter->mReturnFrame->signal();

                delete frame;
            }

            return true;
        }

    private:
        BufferSourceAdapter* mBufferSourceAdapter;
        android::Vector<CameraFrame *> mFrames;
        android::Condition mFramesCondition;
        android::Mutex mFramesMutex;
        bool mDestroying;
    };

    enum {
        BUFFER_SOURCE_TAP_IN,
        BUFFER_SOURCE_TAP_OUT
    };

// public member functions
public:
    BufferSourceAdapter();
    virtual ~BufferSourceAdapter();

    virtual status_t initialize();
    virtual int setPreviewWindow(struct preview_stream_ops *source);
    virtual int setFrameProvider(FrameNotifier *frameProvider);
    virtual int setErrorHandler(ErrorNotifier *errorNotifier);
    virtual int enableDisplay(int width, int height, struct timeval *refTime = NULL);
    virtual int disableDisplay(bool cancel_buffer = true);
    virtual status_t pauseDisplay(bool pause);
#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS
    // Not implemented in this class
    virtual status_t setSnapshotTimeRef(struct timeval *refTime = NULL) { return NO_ERROR; }
#endif
    virtual bool supportsExternalBuffering();
    virtual CameraBuffer * allocateBufferList(int width, int dummyHeight, const char* format, int &bytes, int numBufs);
    virtual CameraBuffer *getBufferList(int *numBufs);
    virtual uint32_t * getOffsets() ;
    virtual int getFd() ;
    virtual int freeBufferList(CameraBuffer * buflist);
    virtual int maxQueueableBuffers(unsigned int& queueable);
    virtual int minUndequeueableBuffers(int& unqueueable);
    virtual bool match(const char * str);

    virtual CameraBuffer * getBuffers(bool reset = false);
    virtual unsigned int getSize();
    virtual int getBufferCount();

    static void frameCallback(CameraFrame* caFrame);
    void addFrame(CameraFrame* caFrame);
    void handleFrameCallback(CameraFrame* caFrame);
    bool handleFrameReturn();

private:
    void destroy();
    status_t returnBuffersToWindow();

private:
    preview_stream_ops_t*  mBufferSource;
    FrameProvider *mFrameProvider; // Pointer to the frame provider interface

    mutable android::Mutex mLock;
    int mBufferCount;
    CameraBuffer *mBuffers;

    android::KeyedVector<buffer_handle_t *, int> mFramesWithCameraAdapterMap;
    android::sp<ErrorNotifier> mErrorNotifier;
    android::sp<ReturnFrame> mReturnFrame;
    android::sp<QueueFrame> mQueueFrame;

    uint32_t mFrameWidth;
    uint32_t mFrameHeight;
    uint32_t mPreviewWidth;
    uint32_t mPreviewHeight;

    int mBufferSourceDirection;

    const char *mPixelFormat;
};

} // namespace Camera
} // namespace Ti

#endif

#endif
