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



#include "CameraHal.h"
#include <ui/GraphicBufferMapper.h>
#include <hal_public.h>

namespace Ti {
namespace Camera {

/**
 * Display handler class - This class basically handles the buffer posting to display
 */

class ANativeWindowDisplayAdapter : public DisplayAdapter
{
public:

    typedef struct
        {
        CameraBuffer *mBuffer;
        void *mUser;
        int mOffset;
        int mWidth;
        int mHeight;
        int mWidthStride;
        int mHeightStride;
        int mLength;
        CameraFrame::FrameType mType;
        } DisplayFrame;

    enum DisplayStates
        {
        DISPLAY_INIT = 0,
        DISPLAY_STARTED,
        DISPLAY_STOPPED,
        DISPLAY_EXITED
        };

public:

    ANativeWindowDisplayAdapter();
    virtual ~ANativeWindowDisplayAdapter();

    ///Initializes the display adapter creates any resources required
    virtual status_t initialize();

    virtual int setPreviewWindow(struct preview_stream_ops *window);
    virtual int setFrameProvider(FrameNotifier *frameProvider);
    virtual int setErrorHandler(ErrorNotifier *errorNotifier);
    virtual int enableDisplay(int width, int height, struct timeval *refTime = NULL);
    virtual int disableDisplay(bool cancel_buffer = true);
    virtual status_t pauseDisplay(bool pause);

#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS

    //Used for shot to snapshot measurement
    virtual status_t setSnapshotTimeRef(struct timeval *refTime = NULL);

#endif

    virtual bool supportsExternalBuffering();

    //Implementation of inherited interfaces
    virtual CameraBuffer * allocateBufferList(int width, int height, const char* format, int &bytes, int numBufs);
    virtual CameraBuffer *getBufferList(int *numBufs);
    virtual uint32_t * getOffsets() ;
    virtual int getFd() ;
    virtual int freeBufferList(CameraBuffer * buflist);

    virtual status_t maxQueueableBuffers(unsigned int& queueable);
    virtual status_t minUndequeueableBuffers(int& unqueueable);

    ///Class specific functions
    static void frameCallbackRelay(CameraFrame* caFrame);
    void frameCallback(CameraFrame* caFrame);

    void displayThread();

    private:
    void destroy();
    bool processHalMsg();
    status_t PostFrame(ANativeWindowDisplayAdapter::DisplayFrame &dispFrame);
    bool handleFrameReturn();
    status_t returnBuffersToWindow();

public:

    static const int DISPLAY_TIMEOUT;
    static const int FAILED_DQS_TO_SUSPEND;

    class DisplayThread : public android::Thread
        {
        ANativeWindowDisplayAdapter* mDisplayAdapter;
        Utils::MessageQueue mDisplayThreadQ;

        public:
            DisplayThread(ANativeWindowDisplayAdapter* da)
            : Thread(false), mDisplayAdapter(da) { }

        ///Returns a reference to the display message Q for display adapter to post messages
            Utils::MessageQueue& msgQ()
                {
                return mDisplayThreadQ;
                }

            virtual bool threadLoop()
                {
                mDisplayAdapter->displayThread();
                return false;
                }

            enum DisplayThreadCommands
                {
                DISPLAY_START,
                DISPLAY_STOP,
                DISPLAY_FRAME,
                DISPLAY_EXIT
                };
        };

    //friend declarations
friend class DisplayThread;

private:
    int postBuffer(void* displayBuf);

private:
    bool mFirstInit;
    bool mSuspend;
    int mFailedDQs;
    bool mPaused; //Pause state
    preview_stream_ops_t*  mANativeWindow;
    android::sp<DisplayThread> mDisplayThread;
    FrameProvider *mFrameProvider; ///Pointer to the frame provider interface
    Utils::MessageQueue mDisplayQ;
    unsigned int mDisplayState;
    ///@todo Have a common class for these members
    mutable android::Mutex mLock;
    bool mDisplayEnabled;
    int mBufferCount;
    CameraBuffer *mBuffers;
    //buffer_handle_t** mBufferHandleMap; // -> frames[i].BufferHandle
    //IMG_native_handle_t** mGrallocHandleMap; // -> frames[i].GrallocHandle
    uint32_t* mOffsetsMap; // -> frames[i].Offset
    int mFD;
    android::KeyedVector<buffer_handle_t *, int> mFramesWithCameraAdapterMap;
    android::KeyedVector<int, int> mFramesType;
    android::sp<ErrorNotifier> mErrorNotifier;

    uint32_t mFrameWidth;
    uint32_t mFrameHeight;
    uint32_t mPreviewWidth;
    uint32_t mPreviewHeight;

    uint32_t mXOff;
    uint32_t mYOff;

    const char *mPixelFormat;

#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS
    //Used for calculating standby to first shot
    struct timeval mStandbyToShot;
    bool mMeasureStandby;
    //Used for shot to snapshot/shot calculation
    struct timeval mStartCapture;
    bool mShotToShot;

#endif

};

} // namespace Camera
} // namespace Ti
