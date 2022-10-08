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

#include "ANativeWindowDisplayAdapter.h"
#include <OMX_IVCommon.h>
#include <ui/GraphicBuffer.h>
#include <ui/GraphicBufferMapper.h>
#include <hal_public.h>

namespace Ti {
namespace Camera {

///Constant declarations
///@todo Check the time units
const int ANativeWindowDisplayAdapter::DISPLAY_TIMEOUT = 1000;  // seconds

//Suspends buffers after given amount of failed dq's
const int ANativeWindowDisplayAdapter::FAILED_DQS_TO_SUSPEND = 3;


OMX_COLOR_FORMATTYPE toOMXPixFormat(const char* parameters_format)
{
    OMX_COLOR_FORMATTYPE pixFormat;

    if ( parameters_format != NULL )
    {
        if (strcmp(parameters_format, android::CameraParameters::PIXEL_FORMAT_YUV422I) == 0)
            {
            CAMHAL_LOGDA("CbYCrY format selected");
            pixFormat = OMX_COLOR_FormatCbYCrY;
            }
        else if(strcmp(parameters_format, android::CameraParameters::PIXEL_FORMAT_YUV420SP) == 0)
            {
            CAMHAL_LOGDA("YUV420SP format selected");
            pixFormat = OMX_COLOR_FormatYUV420SemiPlanar;
            }
        else if(strcmp(parameters_format, android::CameraParameters::PIXEL_FORMAT_RGB565) == 0)
            {
            CAMHAL_LOGDA("RGB565 format selected");
            pixFormat = OMX_COLOR_Format16bitRGB565;
            }
        else
            {
            CAMHAL_LOGDA("Invalid format, NV12 format selected as default");
            pixFormat = OMX_COLOR_FormatYUV420SemiPlanar;
        }
    }
    else {
        CAMHAL_LOGEA("Preview format is NULL, defaulting to NV12");
        pixFormat = OMX_COLOR_FormatYUV420SemiPlanar;
    }

    return pixFormat;
}

/*--------------------ANativeWindowDisplayAdapter Class STARTS here-----------------------------*/


/**
 * Display Adapter class STARTS here..
 */
ANativeWindowDisplayAdapter::ANativeWindowDisplayAdapter():mDisplayThread(NULL),
                                        mDisplayState(ANativeWindowDisplayAdapter::DISPLAY_INIT),
                                        mDisplayEnabled(false),
                                        mBufferCount(0)



{
    LOG_FUNCTION_NAME;

#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS

    mShotToShot = false;
    mStartCapture.tv_sec = 0;
    mStartCapture.tv_usec = 0;
    mStandbyToShot.tv_sec = 0;
    mStandbyToShot.tv_usec = 0;
    mMeasureStandby = false;
#endif

    mPixelFormat = NULL;
    mBuffers = NULL;
    mOffsetsMap = NULL;
    mFrameProvider = NULL;
    mANativeWindow = NULL;

    mFrameWidth = 0;
    mFrameHeight = 0;
    mPreviewWidth = 0;
    mPreviewHeight = 0;

    mSuspend = false;
    mFailedDQs = 0;

    mPaused = false;
    mXOff = -1;
    mYOff = -1;
    mFirstInit = false;

    mFD = -1;

    LOG_FUNCTION_NAME_EXIT;
}

ANativeWindowDisplayAdapter::~ANativeWindowDisplayAdapter()
{
    Utils::Semaphore sem;
    Utils::Message msg;

    LOG_FUNCTION_NAME;

    ///If Frame provider exists
    if (mFrameProvider) {
        // Unregister with the frame provider
        mFrameProvider->disableFrameNotification(CameraFrame::ALL_FRAMES);
        delete mFrameProvider;
        mFrameProvider = NULL;
    }

    ///The ANativeWindow object will get destroyed here
    destroy();

    ///If Display thread exists
    if(mDisplayThread.get())
        {
        ///Kill the display thread
        sem.Create();
        msg.command = DisplayThread::DISPLAY_EXIT;

        // Send the semaphore to signal once the command is completed
        msg.arg1 = &sem;

        ///Post the message to display thread
        mDisplayThread->msgQ().put(&msg);

        ///Wait for the ACK - implies that the thread is now started and waiting for frames
        sem.Wait();

        // Exit and cleanup the thread
        mDisplayThread->requestExitAndWait();

        // Delete the display thread
        mDisplayThread.clear();
    }

    LOG_FUNCTION_NAME_EXIT;

}

status_t ANativeWindowDisplayAdapter::initialize()
{
    LOG_FUNCTION_NAME;

    ///Create the display thread
    mDisplayThread = new DisplayThread(this);
    if ( !mDisplayThread.get() )
        {
        CAMHAL_LOGEA("Couldn't create display thread");
        LOG_FUNCTION_NAME_EXIT;
        return NO_MEMORY;
    }

    ///Start the display thread
    status_t ret = mDisplayThread->run("DisplayThread", android::PRIORITY_URGENT_DISPLAY);
    if ( ret != NO_ERROR )
        {
        CAMHAL_LOGEA("Couldn't run display thread");
        LOG_FUNCTION_NAME_EXIT;
        return ret;
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

int ANativeWindowDisplayAdapter::setPreviewWindow(preview_stream_ops_t* window)
{
    LOG_FUNCTION_NAME;
    ///Note that Display Adapter cannot work without a valid window object
    if ( !window)
        {
        CAMHAL_LOGEA("NULL window object passed to DisplayAdapter");
        LOG_FUNCTION_NAME_EXIT;
        return BAD_VALUE;
    }

    if ( window == mANativeWindow ) {
        return ALREADY_EXISTS;
    }

    ///Destroy the existing window object, if it exists
    destroy();

    ///Move to new window obj
    mANativeWindow = window;

    LOG_FUNCTION_NAME_EXIT;

    return NO_ERROR;
}

int ANativeWindowDisplayAdapter::setFrameProvider(FrameNotifier *frameProvider)
{
    LOG_FUNCTION_NAME;

    // Check for NULL pointer
    if ( !frameProvider ) {
        CAMHAL_LOGEA("NULL passed for frame provider");
        LOG_FUNCTION_NAME_EXIT;
        return BAD_VALUE;
    }

    //Release any previous frame providers
    if ( NULL != mFrameProvider ) {
        delete mFrameProvider;
    }

    /** Dont do anything here, Just save the pointer for use when display is
         actually enabled or disabled
    */
    mFrameProvider = new FrameProvider(frameProvider, this, frameCallbackRelay);

    LOG_FUNCTION_NAME_EXIT;

    return NO_ERROR;
}

int ANativeWindowDisplayAdapter::setErrorHandler(ErrorNotifier *errorNotifier)
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    if ( NULL == errorNotifier ) {
        CAMHAL_LOGEA("Invalid Error Notifier reference");
        ret = BAD_VALUE;
    }

    if ( NO_ERROR == ret )
        {
        mErrorNotifier = errorNotifier;
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS

status_t ANativeWindowDisplayAdapter::setSnapshotTimeRef(struct timeval *refTime)
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    if ( NULL != refTime )
        {
        android::AutoMutex lock(mLock);
        memcpy(&mStartCapture, refTime, sizeof(struct timeval));
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

#endif


int ANativeWindowDisplayAdapter::enableDisplay(int width, int height, struct timeval *refTime)
{
    Utils::Semaphore sem;
    Utils::Message msg;

    LOG_FUNCTION_NAME;

    if ( mDisplayEnabled )
        {
        CAMHAL_LOGDA("Display is already enabled");
        LOG_FUNCTION_NAME_EXIT;

        return NO_ERROR;
    }

#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS

    if ( NULL != refTime )
        {
        android::AutoMutex lock(mLock);
        memcpy(&mStandbyToShot, refTime, sizeof(struct timeval));
        mMeasureStandby = true;
    }

#endif

    //Send START_DISPLAY COMMAND to display thread. Display thread will start and then wait for a message
    sem.Create();
    msg.command = DisplayThread::DISPLAY_START;

    // Send the semaphore to signal once the command is completed
    msg.arg1 = &sem;

    ///Post the message to display thread
    mDisplayThread->msgQ().put(&msg);

    ///Wait for the ACK - implies that the thread is now started and waiting for frames
    sem.Wait();

    // Register with the frame provider for frames
    mFrameProvider->enableFrameNotification(CameraFrame::PREVIEW_FRAME_SYNC);
    mFrameProvider->enableFrameNotification(CameraFrame::SNAPSHOT_FRAME);

    mDisplayEnabled = true;
    mPreviewWidth = width;
    mPreviewHeight = height;

    CAMHAL_LOGVB("mPreviewWidth = %d mPreviewHeight = %d", mPreviewWidth, mPreviewHeight);

    LOG_FUNCTION_NAME_EXIT;

    return NO_ERROR;
}

int ANativeWindowDisplayAdapter::disableDisplay(bool cancel_buffer)
{
    status_t ret = NO_ERROR;
    android::GraphicBufferMapper &mapper = android::GraphicBufferMapper::get();

    LOG_FUNCTION_NAME;

    if(!mDisplayEnabled)
        {
        CAMHAL_LOGDA("Display is already disabled");
        LOG_FUNCTION_NAME_EXIT;
        return ALREADY_EXISTS;
    }

    // Unregister with the frame provider here
    mFrameProvider->disableFrameNotification(CameraFrame::PREVIEW_FRAME_SYNC);
    mFrameProvider->disableFrameNotification(CameraFrame::SNAPSHOT_FRAME);
    mFrameProvider->removeFramePointers();

    if ( NULL != mDisplayThread.get() )
        {
        //Send STOP_DISPLAY COMMAND to display thread. Display thread will stop and dequeue all messages
        // and then wait for message
        Utils::Semaphore sem;
        sem.Create();
        Utils::Message msg;
        msg.command = DisplayThread::DISPLAY_STOP;

        // Send the semaphore to signal once the command is completed
        msg.arg1 = &sem;

        ///Post the message to display thread
        mDisplayThread->msgQ().put(&msg);

        ///Wait for the ACK for display to be disabled

        sem.Wait();

    }

    android::AutoMutex lock(mLock);
    {
        ///Reset the display enabled flag
        mDisplayEnabled = false;

        // Reset pause flag since display is being disabled
        mPaused = false;

        ///Reset the offset values
        mXOff = -1;
        mYOff = -1;

        ///Reset the frame width and height values
        mFrameWidth =0;
        mFrameHeight = 0;
        mPreviewWidth = 0;
        mPreviewHeight = 0;

       if(cancel_buffer)
        {
        // Return the buffers to ANativeWindow here, the mFramesWithCameraAdapterMap is also cleared inside
        returnBuffersToWindow();
        }
       else
        {
        mANativeWindow = NULL;
        // Clear the frames with camera adapter map
        mFramesWithCameraAdapterMap.clear();
        }


    }
    LOG_FUNCTION_NAME_EXIT;

    return NO_ERROR;
}

status_t ANativeWindowDisplayAdapter::pauseDisplay(bool pause)
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    {
        android::AutoMutex lock(mLock);
        mPaused = pause;
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}


void ANativeWindowDisplayAdapter::destroy()
{
    LOG_FUNCTION_NAME;

    ///Check if the display is disabled, if not disable it
    if ( mDisplayEnabled )
    {
        CAMHAL_LOGDA("WARNING: Calling destroy of Display adapter when display enabled. Disabling display..");
        disableDisplay(false);
    }

    mBufferCount = 0;

    LOG_FUNCTION_NAME_EXIT;
}

// Implementation of inherited interfaces
CameraBuffer* ANativeWindowDisplayAdapter::allocateBufferList(int width, int height, const char* format, int &bytes, int numBufs)
{
    LOG_FUNCTION_NAME;
    status_t err;
    int i = -1;
    const int lnumBufs = numBufs;
    int undequeued = 0;
    android::GraphicBufferMapper &mapper = android::GraphicBufferMapper::get();
    android::Rect bounds;

    mBuffers = new CameraBuffer [lnumBufs];
    memset (mBuffers, 0, sizeof(CameraBuffer) * lnumBufs);

    mFramesType.clear();

    if ( NULL == mANativeWindow ) {
        return NULL;
    }

    // Set gralloc usage bits for window.
    err = mANativeWindow->set_usage(mANativeWindow, CAMHAL_GRALLOC_USAGE);
    if ( NO_ERROR != err ) {
        CAMHAL_LOGE("Surface::setUsage failed: %s (%d)", strerror(-err), -err);

        if ( NO_INIT == err ) {
            CAMHAL_LOGEA("Preview surface abandoned!");
            mANativeWindow = NULL;
        }

        return NULL;
    }

    CAMHAL_LOGDB("Number of buffers set to ANativeWindow %d", numBufs);
    ///Set the number of buffers needed for camera preview
    err = mANativeWindow->set_buffer_count(mANativeWindow, numBufs);
    if ( NO_ERROR != err ) {
        CAMHAL_LOGE("Surface::setBufferCount failed: %s (%d)", strerror(-err), -err);

        if ( NO_INIT == err ) {
            CAMHAL_LOGEA("Preview surface abandoned!");
            mANativeWindow = NULL;
        }

        return NULL;
    }
    CAMHAL_LOGDB("Configuring %d buffers for ANativeWindow", numBufs);
    mBufferCount = numBufs;


    // Set window geometry
    err = mANativeWindow->set_buffers_geometry(
            mANativeWindow,
            width,
            height,
            /*toOMXPixFormat(format)*/HAL_PIXEL_FORMAT_TI_NV12);  // Gralloc only supports NV12 alloc!

    if ( NO_ERROR != err ) {
        CAMHAL_LOGE("native_window_set_buffers_geometry failed: %s (%d)", strerror(-err), -err);

        if ( NO_INIT == err ) {
            CAMHAL_LOGEA("Preview surface abandoned!");
            mANativeWindow = NULL;
        }

        return NULL;
    }

    ///We just return the buffers from ANativeWindow, if the width and height are same, else (vstab, vnf case)
    ///re-allocate buffers using ANativeWindow and then get them
    ///@todo - Re-allocate buffers for vnf and vstab using the width, height, format, numBufs etc
    if ( mBuffers == NULL )
    {
        CAMHAL_LOGEA("Couldn't create array for ANativeWindow buffers");
        LOG_FUNCTION_NAME_EXIT;
        return NULL;
    }

    mANativeWindow->get_min_undequeued_buffer_count(mANativeWindow, &undequeued);
    mPixelFormat = CameraHal::getPixelFormatConstant(format);

    for ( i=0; i < mBufferCount; i++ )
    {
        buffer_handle_t *handle;
        int stride;  // dummy variable to get stride
        // TODO(XXX): Do we need to keep stride information in camera hal?

        err = mANativeWindow->dequeue_buffer(mANativeWindow, &handle, &stride);

        if ( NO_ERROR != err ) {
            CAMHAL_LOGE("Surface::dequeueBuffer failed: %s (%d)", strerror(-err), -err);

            if ( NO_INIT == err ) {
                CAMHAL_LOGEA("Preview surface abandoned!");
                mANativeWindow = NULL;
            }

            goto fail;
        }

        CAMHAL_LOGDB("got handle %p", handle);
        mBuffers[i].opaque = (void *)handle;
        mBuffers[i].type = CAMERA_BUFFER_ANW;
        mBuffers[i].format = mPixelFormat;
        mFramesWithCameraAdapterMap.add(handle, i);

        // Tag remaining preview buffers as preview frames
        if ( i >= ( mBufferCount - undequeued ) ) {
            mFramesType.add( (int) mBuffers[i].opaque,
                            CameraFrame::PREVIEW_FRAME_SYNC);
        }

        bytes = CameraHal::calculateBufferSize(format, width, height);

    }

    // lock the initial queueable buffers
    bounds.left = 0;
    bounds.top = 0;
    bounds.right = width;
    bounds.bottom = height;

    for( i = 0;  i < mBufferCount-undequeued; i++ )
    {
        void *y_uv[2];
        buffer_handle_t *handle = (buffer_handle_t *) mBuffers[i].opaque;

        mANativeWindow->lock_buffer(mANativeWindow, handle);

        mapper.lock(*handle, CAMHAL_GRALLOC_USAGE, bounds, y_uv);
        mBuffers[i].mapped = y_uv[0];
        mFrameProvider->addFramePointers(&mBuffers[i], y_uv);
    }

    // return the rest of the buffers back to ANativeWindow
    for(i = (mBufferCount-undequeued); i >= 0 && i < mBufferCount; i++)
    {
        buffer_handle_t *handle = (buffer_handle_t *) mBuffers[i].opaque;
        err = mANativeWindow->cancel_buffer(mANativeWindow, handle);
        if ( NO_ERROR != err ) {
            CAMHAL_LOGE("Surface::cancelBuffer failed: %s (%d)", strerror(-err), -err);

            if ( NO_INIT == err ) {
                CAMHAL_LOGEA("Preview surface abandoned!");
                mANativeWindow = NULL;
            }

            goto fail;
        }
        mFramesWithCameraAdapterMap.removeItem((buffer_handle_t *) mBuffers[i].opaque);
        //LOCK UNLOCK TO GET YUV POINTERS
        void *y_uv[2];
        mapper.lock(*(buffer_handle_t *) mBuffers[i].opaque, CAMHAL_GRALLOC_USAGE, bounds, y_uv);
        mBuffers[i].mapped = y_uv[0];
        mFrameProvider->addFramePointers(&mBuffers[i], y_uv);
        mapper.unlock(*(buffer_handle_t *) mBuffers[i].opaque);
    }

    mFirstInit = true;
    mFrameWidth = width;
    mFrameHeight = height;

    return mBuffers;

 fail:
    // need to cancel buffers if any were dequeued
    for (int start = 0; start < i && i > 0; start++) {
        status_t err = mANativeWindow->cancel_buffer(mANativeWindow,
                (buffer_handle_t *) mBuffers[start].opaque);
        if ( NO_ERROR != err ) {
          CAMHAL_LOGE("Surface::cancelBuffer failed w/ error 0x%08x", err);
          break;
        }
        mFramesWithCameraAdapterMap.removeItem((buffer_handle_t *) mBuffers[start].opaque);
    }

    freeBufferList(mBuffers);

    CAMHAL_LOGEA("Error occurred, performing cleanup");

    if ( NULL != mErrorNotifier.get() ) {
        mErrorNotifier->errorNotify(NO_MEMORY);
    }

    LOG_FUNCTION_NAME_EXIT;
    return NULL;

}

CameraBuffer* ANativeWindowDisplayAdapter::getBufferList(int *numBufs) {
    LOG_FUNCTION_NAME;
    if (numBufs) *numBufs = -1;

    return NULL;
}

uint32_t * ANativeWindowDisplayAdapter::getOffsets()
{
    const int lnumBufs = mBufferCount;

    LOG_FUNCTION_NAME;

    // TODO(XXX): Need to remove getOffsets from the API. No longer needed

    if ( NULL == mANativeWindow )
    {
        CAMHAL_LOGEA("mANativeWindow reference is missing");
        goto fail;
    }

    if( mBuffers == NULL)
    {
        CAMHAL_LOGEA("Buffers not allocated yet!!");
        goto fail;
    }

    if(mOffsetsMap == NULL)
    {
        mOffsetsMap = new uint32_t[lnumBufs];
        for(int i = 0; i < mBufferCount; i++)
        {
            mOffsetsMap[i] = 0;
        }
    }

    LOG_FUNCTION_NAME_EXIT;

    return mOffsetsMap;

 fail:

    if ( NULL != mOffsetsMap )
    {
        delete [] mOffsetsMap;
        mOffsetsMap = NULL;
    }

    if ( NULL != mErrorNotifier.get() ) {
        mErrorNotifier->errorNotify(INVALID_OPERATION);
    }

    LOG_FUNCTION_NAME_EXIT;

    return NULL;
}

status_t ANativeWindowDisplayAdapter::minUndequeueableBuffers(int& undequeueable) {
    LOG_FUNCTION_NAME;
    status_t ret = NO_ERROR;

    if(!mANativeWindow) {
        ret = INVALID_OPERATION;
        goto end;
    }

    ret = mANativeWindow->get_min_undequeued_buffer_count(mANativeWindow, &undequeueable);
    if ( NO_ERROR != ret ) {
        CAMHAL_LOGEB("get_min_undequeued_buffer_count failed: %s (%d)", strerror(-ret), -ret);

        if ( NO_INIT == ret ) {
            CAMHAL_LOGEA("Preview surface abandoned!");
            mANativeWindow = NULL;
        }

        return ret;
    }

 end:
    return ret;
    LOG_FUNCTION_NAME_EXIT;

}

status_t ANativeWindowDisplayAdapter::maxQueueableBuffers(unsigned int& queueable)
{
    LOG_FUNCTION_NAME;
    status_t ret = NO_ERROR;
    int undequeued = 0;

    if(mBufferCount == 0)
    {
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

int ANativeWindowDisplayAdapter::getFd()
{
    LOG_FUNCTION_NAME;

    if(mFD == -1)
    {
        buffer_handle_t *handle =  (buffer_handle_t *)mBuffers[0].opaque;
        IMG_native_handle_t *img = (IMG_native_handle_t *)handle;
        // TODO: should we dup the fd? not really necessary and another thing for ANativeWindow
        // to manage and close...

        mFD = dup(img->fd[0]);
    }

    LOG_FUNCTION_NAME_EXIT;

    return mFD;

}

status_t ANativeWindowDisplayAdapter::returnBuffersToWindow()
{
    status_t ret = NO_ERROR;

     android::GraphicBufferMapper &mapper = android::GraphicBufferMapper::get();
    //Give the buffers back to display here -  sort of free it
     if (mANativeWindow)
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

             ret = mANativeWindow->cancel_buffer(mANativeWindow, handle);
             if ( NO_INIT == ret ) {
                 CAMHAL_LOGEA("Preview surface abandoned!");
                 mANativeWindow = NULL;
                 return ret;
             } else if ( NO_ERROR != ret ) {
                 CAMHAL_LOGE("Surface::cancelBuffer() failed: %s (%d)",
                              strerror(-ret),
                              -ret);
                return ret;
             }
         }
     else
         CAMHAL_LOGE("mANativeWindow is NULL");

     ///Clear the frames with camera adapter map
     mFramesWithCameraAdapterMap.clear();

     return ret;

}

int ANativeWindowDisplayAdapter::freeBufferList(CameraBuffer * buflist)
{
    LOG_FUNCTION_NAME;

    status_t ret = NO_ERROR;

    android::AutoMutex lock(mLock);

    if(mBuffers != buflist)
    {
        CAMHAL_LOGEA("CameraHal passed wrong set of buffers to free!!!");
        if (mBuffers != NULL)
            delete []mBuffers;
        mBuffers = NULL;
    }

    /* FIXME this will probably want the list that was just deleted */
    returnBuffersToWindow();

    if ( NULL != buflist )
    {
        delete [] buflist;
        mBuffers = NULL;
    }

    if( mBuffers != NULL)
    {
        delete [] mBuffers;
        mBuffers = NULL;
    }

    if ( NULL != mOffsetsMap )
    {
        delete [] mOffsetsMap;
        mOffsetsMap = NULL;
    }

    if( mFD != -1)
    {
        close(mFD);  // close duped handle
        mFD = -1;
    }

    mFramesType.clear();

    return NO_ERROR;
}


bool ANativeWindowDisplayAdapter::supportsExternalBuffering()
{
    return false;
}

void ANativeWindowDisplayAdapter::displayThread()
{
    bool shouldLive = true;
    int timeout = 0;
    status_t ret;

    LOG_FUNCTION_NAME;

    while(shouldLive)
        {
        ret = Utils::MessageQueue::waitForMsg(&mDisplayThread->msgQ()
                                                                ,  &mDisplayQ
                                                                , NULL
                                                                , ANativeWindowDisplayAdapter::DISPLAY_TIMEOUT);

        if ( !mDisplayThread->msgQ().isEmpty() )
            {
            ///Received a message from CameraHal, process it
            shouldLive = processHalMsg();

            }
        else  if( !mDisplayQ.isEmpty())
            {
            if ( mDisplayState== ANativeWindowDisplayAdapter::DISPLAY_INIT )
                {

                ///If display adapter is not started, continue
                continue;

                }
            else
                {
                Utils::Message msg;
                ///Get the dummy msg from the displayQ
                if(mDisplayQ.get(&msg)!=NO_ERROR)
                    {
                    CAMHAL_LOGEA("Error in getting message from display Q");
                    continue;
                }

                // There is a frame from ANativeWindow for us to dequeue
                // We dequeue and return the frame back to Camera adapter
                if(mDisplayState == ANativeWindowDisplayAdapter::DISPLAY_STARTED)
                {
                    handleFrameReturn();
                }

                if (mDisplayState == ANativeWindowDisplayAdapter::DISPLAY_EXITED)
                    {
                    ///we exit the thread even though there are frames still to dequeue. They will be dequeued
                    ///in disableDisplay
                    shouldLive = false;
                }
            }
        }
    }

    LOG_FUNCTION_NAME_EXIT;
}


bool ANativeWindowDisplayAdapter::processHalMsg()
{
    Utils::Message msg;

    LOG_FUNCTION_NAME;


    mDisplayThread->msgQ().get(&msg);
    bool ret = true, invalidCommand = false;

    switch ( msg.command )
        {

        case DisplayThread::DISPLAY_START:

            CAMHAL_LOGDA("Display thread received DISPLAY_START command from Camera HAL");
            mDisplayState = ANativeWindowDisplayAdapter::DISPLAY_STARTED;

            break;

        case DisplayThread::DISPLAY_STOP:

            ///@bug There is no API to disable SF without destroying it
            ///@bug Buffers might still be w/ display and will get displayed
            ///@remarks Ideal seqyence should be something like this
            ///mOverlay->setParameter("enabled", false);
            CAMHAL_LOGDA("Display thread received DISPLAY_STOP command from Camera HAL");
            mDisplayState = ANativeWindowDisplayAdapter::DISPLAY_STOPPED;

            // flush frame message queue
            while ( !mDisplayQ.isEmpty() ) {
                Utils::Message message;
                mDisplayQ.get(&message);
            }

            break;

        case DisplayThread::DISPLAY_EXIT:

            CAMHAL_LOGDA("Display thread received DISPLAY_EXIT command from Camera HAL.");
            CAMHAL_LOGDA("Stopping display thread...");
            mDisplayState = ANativeWindowDisplayAdapter::DISPLAY_EXITED;
            ///Note that the SF can have pending buffers when we disable the display
            ///This is normal and the expectation is that they may not be displayed.
            ///This is to ensure that the user experience is not impacted
            ret = false;
            break;

        default:

            CAMHAL_LOGEB("Invalid Display Thread Command 0x%x.", msg.command);
            invalidCommand = true;

            break;
    }

    ///Signal the semaphore if it is sent as part of the message
    if ( ( msg.arg1 ) && ( !invalidCommand ) )
        {

        CAMHAL_LOGDA("+Signalling display semaphore");
        Utils::Semaphore &sem = *((Utils::Semaphore*)msg.arg1);

        sem.Signal();

        CAMHAL_LOGDA("-Signalling display semaphore");
    }


    LOG_FUNCTION_NAME_EXIT;
    return ret;
}


status_t ANativeWindowDisplayAdapter::PostFrame(ANativeWindowDisplayAdapter::DisplayFrame &dispFrame)
{
    status_t ret = NO_ERROR;
    uint32_t actualFramesWithDisplay = 0;
    android_native_buffer_t *buffer = NULL;
    android::GraphicBufferMapper &mapper = android::GraphicBufferMapper::get();
    int i;

    ///@todo Do cropping based on the stabilized frame coordinates
    ///@todo Insert logic to drop frames here based on refresh rate of
    ///display or rendering rate whichever is lower
    ///Queue the buffer to overlay

    if ( NULL == mANativeWindow ) {
        return NO_INIT;
    }

    if (!mBuffers || !dispFrame.mBuffer) {
        CAMHAL_LOGEA("NULL sent to PostFrame");
        return BAD_VALUE;
    }

    for ( i = 0; i < mBufferCount; i++ )
    {
        if ( dispFrame.mBuffer == &mBuffers[i] )
        {
            break;
        }
    }

#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS

    if ( mMeasureStandby ) {
        CameraHal::PPM("Standby to first shot: Sensor Change completed - ", &mStandbyToShot);
        mMeasureStandby = false;
    } else if (CameraFrame::CameraFrame::SNAPSHOT_FRAME == dispFrame.mType) {
        CameraHal::PPM("Shot to snapshot: ", &mStartCapture);
        mShotToShot = true;
    } else if ( mShotToShot ) {
        CameraHal::PPM("Shot to shot: ", &mStartCapture);
        mShotToShot = false;
    }

#endif

    android::AutoMutex lock(mLock);

    mFramesType.add( (int)mBuffers[i].opaque, dispFrame.mType);

    if ( mDisplayState == ANativeWindowDisplayAdapter::DISPLAY_STARTED &&
                (!mPaused ||  CameraFrame::CameraFrame::SNAPSHOT_FRAME == dispFrame.mType) &&
                !mSuspend)
    {
        uint32_t xOff, yOff;

        CameraHal::getXYFromOffset(&xOff, &yOff, dispFrame.mOffset, PAGE_SIZE, mPixelFormat);

        // Set crop only if current x and y offsets do not match with frame offsets
        if ((mXOff != xOff) || (mYOff != yOff)) {
            CAMHAL_LOGDB("offset = %u left = %d top = %d right = %d bottom = %d",
                          dispFrame.mOffset, xOff, yOff ,
                          xOff + mPreviewWidth, yOff + mPreviewHeight);

            // We'll ignore any errors here, if the surface is
            // already invalid, we'll know soon enough.
            mANativeWindow->set_crop(mANativeWindow, xOff, yOff,
                                     xOff + mPreviewWidth, yOff + mPreviewHeight);

            // Update the current x and y offsets
            mXOff = xOff;
            mYOff = yOff;
        }

        {
            buffer_handle_t *handle = (buffer_handle_t *) mBuffers[i].opaque;
            // unlock buffer before sending to display
            mapper.unlock(*handle);
            ret = mANativeWindow->enqueue_buffer(mANativeWindow, handle);
        }
        if ( NO_ERROR != ret ) {
            CAMHAL_LOGE("Surface::queueBuffer returned error %d", ret);
        }

        mFramesWithCameraAdapterMap.removeItem((buffer_handle_t *) dispFrame.mBuffer->opaque);


        // HWComposer has not minimum buffer requirement. We should be able to dequeue
        // the buffer immediately
        Utils::Message msg;
        mDisplayQ.put(&msg);

    }
    else
    {
        buffer_handle_t *handle = (buffer_handle_t *) mBuffers[i].opaque;

        // unlock buffer before giving it up
        mapper.unlock(*handle);

        // cancel buffer and dequeue another one
        ret = mANativeWindow->cancel_buffer(mANativeWindow, handle);
        if ( NO_ERROR != ret ) {
            CAMHAL_LOGE("Surface::cancelBuffer returned error %d", ret);
        }

        mFramesWithCameraAdapterMap.removeItem((buffer_handle_t *) dispFrame.mBuffer->opaque);

        Utils::Message msg;
        mDisplayQ.put(&msg);
        ret = NO_ERROR;
    }

    return ret;
}


bool ANativeWindowDisplayAdapter::handleFrameReturn()
{
    status_t err;
    buffer_handle_t *buf;
    int i = 0;
    unsigned int k;
    int stride;  // dummy variable to get stride
    android::GraphicBufferMapper &mapper = android::GraphicBufferMapper::get();
    android::Rect bounds;
    CameraFrame::FrameType frameType = CameraFrame::PREVIEW_FRAME_SYNC;

    void *y_uv[2];

    // TODO(XXX): Do we need to keep stride information in camera hal?

    if ( NULL == mANativeWindow ) {
        return false;
    }

    err = mANativeWindow->dequeue_buffer(mANativeWindow, &buf, &stride);
    if (err != 0) {
        CAMHAL_LOGE("Surface::dequeueBuffer failed: %s (%d)", strerror(-err), -err);

        if ( NO_INIT == err ) {
            CAMHAL_LOGEA("Preview surface abandoned!");
            mANativeWindow = NULL;
        }

        return false;
    }

    err = mANativeWindow->lock_buffer(mANativeWindow, buf);
    if ( NO_ERROR != err ) {
        CAMHAL_LOGE("Surface::lockBuffer failed: %s (%d)", strerror(-err), -err);

        if ( NO_INIT == err ) {
            CAMHAL_LOGEA("Preview surface abandoned!");
            mANativeWindow = NULL;
        }

        return false;
    }

    for(i = 0; i < mBufferCount; i++)
    {
        if (mBuffers[i].opaque == buf)
            break;
    }
    if (i == mBufferCount) {
        CAMHAL_LOGEB("Failed to find handle %p", buf);
    }

    // lock buffer before sending to FrameProvider for filling
    bounds.left = 0;
    bounds.top = 0;
    bounds.right = mFrameWidth;
    bounds.bottom = mFrameHeight;

    int lock_try_count = 0;
    while (mapper.lock(*(buffer_handle_t *) mBuffers[i].opaque, CAMHAL_GRALLOC_USAGE, bounds, y_uv) < 0){
      if (++lock_try_count > LOCK_BUFFER_TRIES){
        if ( NULL != mErrorNotifier.get() ){
          mErrorNotifier->errorNotify(CAMERA_ERROR_UNKNOWN);
        }
        return false;
      }
      CAMHAL_LOGEA("Gralloc Lock FrameReturn Error: Sleeping 15ms");
      usleep(15000);
    }

    {
        android::AutoMutex lock(mLock);
        mFramesWithCameraAdapterMap.add((buffer_handle_t *) mBuffers[i].opaque, i);

        for( k = 0; k < mFramesType.size() ; k++) {
            if(mFramesType.keyAt(k) == (int)mBuffers[i].opaque)
                break;
        }

        if ( k == mFramesType.size() ) {
            CAMHAL_LOGE("Frame type for preview buffer 0%x not found!!", mBuffers[i].opaque);
            return false;
        }

        frameType = (CameraFrame::FrameType) mFramesType.valueAt(k);
        mFramesType.removeItem((int) mBuffers[i].opaque);
    }

    CAMHAL_LOGVB("handleFrameReturn: found graphic buffer %d of %d", i, mBufferCount-1);
    mFrameProvider->returnFrame(&mBuffers[i], frameType);

    return true;
}

void ANativeWindowDisplayAdapter::frameCallbackRelay(CameraFrame* caFrame)
{

    if ( NULL != caFrame )
        {
        if ( NULL != caFrame->mCookie )
            {
            ANativeWindowDisplayAdapter *da = (ANativeWindowDisplayAdapter*) caFrame->mCookie;
            da->frameCallback(caFrame);
        }
        else
            {
            CAMHAL_LOGEB("Invalid Cookie in Camera Frame = %p, Cookie = %p", caFrame, caFrame->mCookie);
            }
        }
    else
        {
        CAMHAL_LOGEB("Invalid Camera Frame = %p", caFrame);
    }

}

void ANativeWindowDisplayAdapter::frameCallback(CameraFrame* caFrame)
{
    ///Call queueBuffer of overlay in the context of the callback thread

    DisplayFrame df;
    df.mBuffer = caFrame->mBuffer;
    df.mType = (CameraFrame::FrameType) caFrame->mFrameType;
    df.mOffset = caFrame->mOffset;
    df.mWidthStride = caFrame->mAlignment;
    df.mLength = caFrame->mLength;
    df.mWidth = caFrame->mWidth;
    df.mHeight = caFrame->mHeight;
    PostFrame(df);
}


/*--------------------ANativeWindowDisplayAdapter Class ENDS here-----------------------------*/

} // namespace Camera
} // namespace Ti
