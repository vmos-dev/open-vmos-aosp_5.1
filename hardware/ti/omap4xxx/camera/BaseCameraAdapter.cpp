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



#define LOG_TAG "CameraHAL"

#include "BaseCameraAdapter.h"

namespace android {

/*--------------------Camera Adapter Class STARTS here-----------------------------*/

BaseCameraAdapter::BaseCameraAdapter()
{
    mReleaseImageBuffersCallback = NULL;
    mEndImageCaptureCallback = NULL;
    mErrorNotifier = NULL;
    mEndCaptureData = NULL;
    mReleaseData = NULL;
    mRecording = false;

    mPreviewBuffers = NULL;
    mPreviewBufferCount = 0;
    mPreviewBuffersLength = 0;

    mVideoBuffers = NULL;
    mVideoBuffersCount = 0;
    mVideoBuffersLength = 0;

    mCaptureBuffers = NULL;
    mCaptureBuffersCount = 0;
    mCaptureBuffersLength = 0;

    mPreviewDataBuffers = NULL;
    mPreviewDataBuffersCount = 0;
    mPreviewDataBuffersLength = 0;

    mAdapterState = INTIALIZED_STATE;

#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS
    mStartFocus.tv_sec = 0;
    mStartFocus.tv_usec = 0;
    mStartCapture.tv_sec = 0;
    mStartCapture.tv_usec = 0;
#endif

}

BaseCameraAdapter::~BaseCameraAdapter()
{
     LOG_FUNCTION_NAME;

     Mutex::Autolock lock(mSubscriberLock);

     mFrameSubscribers.clear();
     mImageSubscribers.clear();
     mRawSubscribers.clear();
     mVideoSubscribers.clear();
     mFocusSubscribers.clear();
     mShutterSubscribers.clear();
     mZoomSubscribers.clear();
     mFaceSubscribers.clear();

     LOG_FUNCTION_NAME_EXIT;
}

status_t BaseCameraAdapter::registerImageReleaseCallback(release_image_buffers_callback callback, void *user_data)
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    mReleaseImageBuffersCallback = callback;
    mReleaseData = user_data;

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t BaseCameraAdapter::registerEndCaptureCallback(end_image_capture_callback callback, void *user_data)
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    mEndImageCaptureCallback= callback;
    mEndCaptureData = user_data;

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t BaseCameraAdapter::setErrorHandler(ErrorNotifier *errorNotifier)
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    if ( NULL == errorNotifier )
        {
        CAMHAL_LOGEA("Invalid Error Notifier reference");
        ret = -EINVAL;
        }

    if ( NO_ERROR == ret )
        {
        mErrorNotifier = errorNotifier;
        }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

void BaseCameraAdapter::enableMsgType(int32_t msgs, frame_callback callback, event_callback eventCb, void* cookie)
{
    Mutex::Autolock lock(mSubscriberLock);

    LOG_FUNCTION_NAME;

    if ( CameraFrame::PREVIEW_FRAME_SYNC == msgs )
        {
        mFrameSubscribers.add((int) cookie, callback);
        }
    else if ( CameraFrame::FRAME_DATA_SYNC == msgs )
        {
        mFrameDataSubscribers.add((int) cookie, callback);
        }
    else if ( CameraFrame::IMAGE_FRAME == msgs)
        {
        mImageSubscribers.add((int) cookie, callback);
        }
    else if ( CameraFrame::RAW_FRAME == msgs)
        {
        mRawSubscribers.add((int) cookie, callback);
        }
    else if ( CameraFrame::VIDEO_FRAME_SYNC == msgs)
        {
        mVideoSubscribers.add((int) cookie, callback);
        }
    else if ( CameraHalEvent::ALL_EVENTS == msgs)
        {
        mFocusSubscribers.add((int) cookie, eventCb);
        mShutterSubscribers.add((int) cookie, eventCb);
        mZoomSubscribers.add((int) cookie, eventCb);
        mFaceSubscribers.add((int) cookie, eventCb);
        }
    else
        {
        CAMHAL_LOGEA("Message type subscription no supported yet!");
        }

    LOG_FUNCTION_NAME_EXIT;
}

void BaseCameraAdapter::disableMsgType(int32_t msgs, void* cookie)
{
    Mutex::Autolock lock(mSubscriberLock);

    LOG_FUNCTION_NAME;

    if ( CameraFrame::PREVIEW_FRAME_SYNC == msgs )
        {
        mFrameSubscribers.removeItem((int) cookie);
        }
    else if ( CameraFrame::FRAME_DATA_SYNC == msgs )
        {
        mFrameDataSubscribers.removeItem((int) cookie);
        }
    else if ( CameraFrame::IMAGE_FRAME == msgs)
        {
        mImageSubscribers.removeItem((int) cookie);
        }
    else if ( CameraFrame::RAW_FRAME == msgs)
        {
        mRawSubscribers.removeItem((int) cookie);
        }
    else if ( CameraFrame::VIDEO_FRAME_SYNC == msgs)
        {
        mVideoSubscribers.removeItem((int) cookie);
        }
    else if ( CameraFrame::ALL_FRAMES  == msgs )
        {
        mFrameSubscribers.removeItem((int) cookie);
        mFrameDataSubscribers.removeItem((int) cookie);
        mImageSubscribers.removeItem((int) cookie);
        mRawSubscribers.removeItem((int) cookie);
        mVideoSubscribers.removeItem((int) cookie);
        }
    else if ( CameraHalEvent::ALL_EVENTS == msgs)
        {
         //Subscribe only for focus
         //TODO: Process case by case
        mFocusSubscribers.removeItem((int) cookie);
        mShutterSubscribers.removeItem((int) cookie);
        mZoomSubscribers.removeItem((int) cookie);
        mFaceSubscribers.removeItem((int) cookie);
        }
    else
        {
        CAMHAL_LOGEB("Message type 0x%x subscription no supported yet!", msgs);
        }

    LOG_FUNCTION_NAME_EXIT;
}

void BaseCameraAdapter::addFramePointers(void *frameBuf, void *buf)
{
  unsigned int *pBuf = (unsigned int *)buf;
  Mutex::Autolock lock(mSubscriberLock);

  if ((frameBuf != NULL) && ( pBuf != NULL) )
    {
      CameraFrame *frame = new CameraFrame;
      frame->mBuffer = frameBuf;
      frame->mYuv[0] = pBuf[0];
      frame->mYuv[1] = pBuf[1];
      mFrameQueue.add(frameBuf, frame);

      CAMHAL_LOGVB("Adding Frame=0x%x Y=0x%x UV=0x%x", frame->mBuffer, frame->mYuv[0], frame->mYuv[1]);
    }
}

void BaseCameraAdapter::removeFramePointers()
{
  Mutex::Autolock lock(mSubscriberLock);

  int size = mFrameQueue.size();
  CAMHAL_LOGVB("Removing %d Frames = ", size);
  for (int i = 0; i < size; i++)
    {
      CameraFrame *frame = (CameraFrame *)mFrameQueue.valueAt(i);
      CAMHAL_LOGVB("Free Frame=0x%x Y=0x%x UV=0x%x", frame->mBuffer, frame->mYuv[0], frame->mYuv[1]);
      delete frame;
    }
  mFrameQueue.clear();
}

void BaseCameraAdapter::returnFrame(void* frameBuf, CameraFrame::FrameType frameType)
{
    status_t res = NO_ERROR;
    size_t subscriberCount = 0;
    int refCount = -1;

    if ( NULL == frameBuf )
        {
        CAMHAL_LOGEA("Invalid frameBuf");
        return;
        }

    if ( NO_ERROR == res)
        {
        Mutex::Autolock lock(mReturnFrameLock);

        refCount = getFrameRefCount(frameBuf,  frameType);

        if(frameType == CameraFrame::PREVIEW_FRAME_SYNC)
            {
            mFramesWithDisplay--;
            }
        else if(frameType == CameraFrame::VIDEO_FRAME_SYNC)
            {
            mFramesWithEncoder--;
            }

        if ( 0 < refCount )
            {

            refCount--;
            setFrameRefCount(frameBuf, frameType, refCount);


            if ( mRecording && (CameraFrame::VIDEO_FRAME_SYNC == frameType) ) {
                refCount += getFrameRefCount(frameBuf, CameraFrame::PREVIEW_FRAME_SYNC);
            } else if ( mRecording && (CameraFrame::PREVIEW_FRAME_SYNC == frameType) ) {
                refCount += getFrameRefCount(frameBuf, CameraFrame::VIDEO_FRAME_SYNC);
            } else if ( mRecording && (CameraFrame::SNAPSHOT_FRAME == frameType) ) {
                refCount += getFrameRefCount(frameBuf, CameraFrame::VIDEO_FRAME_SYNC);
            }


            }
        else
            {
            CAMHAL_LOGDA("Frame returned when ref count is already zero!!");
            return;
            }
        }

    CAMHAL_LOGVB("REFCOUNT 0x%x %d", frameBuf, refCount);

    if ( NO_ERROR == res )
        {
        //check if someone is holding this buffer
        if ( 0 == refCount )
            {
#ifdef DEBUG_LOG
            if(mBuffersWithDucati.indexOfKey((int)frameBuf)>=0)
                {
                ALOGE("Buffer already with Ducati!! 0x%x", frameBuf);
                for(int i=0;i<mBuffersWithDucati.size();i++) ALOGE("0x%x", mBuffersWithDucati.keyAt(i));
                }
            mBuffersWithDucati.add((int)frameBuf,1);
#endif
            res = fillThisBuffer(frameBuf, frameType);
            }
        }

}

status_t BaseCameraAdapter::sendCommand(CameraCommands operation, int value1, int value2, int value3)
{
    status_t ret = NO_ERROR;
    struct timeval *refTimestamp;
    BuffersDescriptor *desc = NULL;
    CameraFrame *frame = NULL;

    LOG_FUNCTION_NAME;

    switch ( operation ) {
        case CameraAdapter::CAMERA_USE_BUFFERS_PREVIEW:
                CAMHAL_LOGDA("Use buffers for preview");
                desc = ( BuffersDescriptor * ) value1;

                if ( NULL == desc )
                    {
                    CAMHAL_LOGEA("Invalid preview buffers!");
                    return -EINVAL;
                    }

                if ( ret == NO_ERROR )
                    {
                    ret = setState(operation);
                    }

                if ( ret == NO_ERROR )
                    {
                    Mutex::Autolock lock(mPreviewBufferLock);
                    mPreviewBuffers = (int *) desc->mBuffers;
                    mPreviewBuffersLength = desc->mLength;
                    mPreviewBuffersAvailable.clear();
                    for ( uint32_t i = 0 ; i < desc->mMaxQueueable ; i++ )
                        {
                        mPreviewBuffersAvailable.add(mPreviewBuffers[i], 0);
                        }
                    // initial ref count for undeqeueued buffers is 1 since buffer provider
                    // is still holding on to it
                    for ( uint32_t i = desc->mMaxQueueable ; i < desc->mCount ; i++ )
                        {
                        mPreviewBuffersAvailable.add(mPreviewBuffers[i], 1);
                        }
                    }

                if ( NULL != desc )
                    {
                    ret = useBuffers(CameraAdapter::CAMERA_PREVIEW,
                                     desc->mBuffers,
                                     desc->mCount,
                                     desc->mLength,
                                     desc->mMaxQueueable);
                    }

                if ( ret == NO_ERROR )
                    {
                    ret = commitState();
                    }
                else
                    {
                    ret |= rollbackState();
                    }

                break;

        case CameraAdapter::CAMERA_USE_BUFFERS_PREVIEW_DATA:
                    CAMHAL_LOGDA("Use buffers for preview data");
                    desc = ( BuffersDescriptor * ) value1;

                    if ( NULL == desc )
                        {
                        CAMHAL_LOGEA("Invalid preview data buffers!");
                        return -EINVAL;
                        }

                    if ( ret == NO_ERROR )
                        {
                        ret = setState(operation);
                        }

                    if ( ret == NO_ERROR )
                        {
                        Mutex::Autolock lock(mPreviewDataBufferLock);
                        mPreviewDataBuffers = (int *) desc->mBuffers;
                        mPreviewDataBuffersLength = desc->mLength;
                        mPreviewDataBuffersAvailable.clear();
                        for ( uint32_t i = 0 ; i < desc->mMaxQueueable ; i++ )
                            {
                            mPreviewDataBuffersAvailable.add(mPreviewDataBuffers[i], 0);
                            }
                        // initial ref count for undeqeueued buffers is 1 since buffer provider
                        // is still holding on to it
                        for ( uint32_t i = desc->mMaxQueueable ; i < desc->mCount ; i++ )
                            {
                            mPreviewDataBuffersAvailable.add(mPreviewDataBuffers[i], 1);
                            }
                        }

                    if ( NULL != desc )
                        {
                        ret = useBuffers(CameraAdapter::CAMERA_MEASUREMENT,
                                         desc->mBuffers,
                                         desc->mCount,
                                         desc->mLength,
                                         desc->mMaxQueueable);
                        }

                    if ( ret == NO_ERROR )
                        {
                        ret = commitState();
                        }
                    else
                        {
                        ret |= rollbackState();
                        }

                    break;

        case CameraAdapter::CAMERA_USE_BUFFERS_IMAGE_CAPTURE:
                CAMHAL_LOGDA("Use buffers for image capture");
                desc = ( BuffersDescriptor * ) value1;

                if ( NULL == desc )
                    {
                    CAMHAL_LOGEA("Invalid capture buffers!");
                    return -EINVAL;
                    }

                if ( ret == NO_ERROR )
                    {
                    ret = setState(operation);
                    }

                if ( ret == NO_ERROR )
                    {
                    Mutex::Autolock lock(mCaptureBufferLock);
                    mCaptureBuffers = (int *) desc->mBuffers;
                    mCaptureBuffersLength = desc->mLength;
                    mCaptureBuffersAvailable.clear();
                    for ( uint32_t i = 0 ; i < desc->mMaxQueueable ; i++ )
                        {
                        mCaptureBuffersAvailable.add(mCaptureBuffers[i], 0);
                        }
                    // initial ref count for undeqeueued buffers is 1 since buffer provider
                    // is still holding on to it
                    for ( uint32_t i = desc->mMaxQueueable ; i < desc->mCount ; i++ )
                        {
                        mCaptureBuffersAvailable.add(mCaptureBuffers[i], 1);
                        }
                    }

                if ( NULL != desc )
                    {
                    ret = useBuffers(CameraAdapter::CAMERA_IMAGE_CAPTURE,
                                     desc->mBuffers,
                                     desc->mCount,
                                     desc->mLength,
                                     desc->mMaxQueueable);
                    }

                if ( ret == NO_ERROR )
                    {
                    ret = commitState();
                    }
                else
                    {
                    ret |= rollbackState();
                    }

                break;

        case CameraAdapter::CAMERA_START_SMOOTH_ZOOM:
            {

            if ( ret == NO_ERROR )
                {
                ret = setState(operation);
                }

            if ( ret == NO_ERROR )
                {
                ret = startSmoothZoom(value1);
                }

            if ( ret == NO_ERROR )
                {
                ret = commitState();
                }
            else
                {
                ret |= rollbackState();
                }

            break;

            }

        case CameraAdapter::CAMERA_STOP_SMOOTH_ZOOM:
            {

            if ( ret == NO_ERROR )
                {
                ret = setState(operation);
                }

            if ( ret == NO_ERROR )
                {
                ret = stopSmoothZoom();
                }

            if ( ret == NO_ERROR )
                {
                ret = commitState();
                }
            else
                {
                ret |= rollbackState();
                }

            break;

            }

        case CameraAdapter::CAMERA_START_PREVIEW:
            {

                CAMHAL_LOGDA("Start Preview");

            if ( ret == NO_ERROR )
                {
                ret = setState(operation);
                }

            if ( ret == NO_ERROR )
                {
                ret = startPreview();
                }

            if ( ret == NO_ERROR )
                {
                ret = commitState();
                }
            else
                {
                ret |= rollbackState();
                }

            break;

            }

        case CameraAdapter::CAMERA_STOP_PREVIEW:
            {

            CAMHAL_LOGDA("Stop Preview");

            if ( ret == NO_ERROR )
                {
                ret = setState(operation);
                }

            if ( ret == NO_ERROR )
                {
                ret = stopPreview();
                }

            if ( ret == NO_ERROR )
                {
                ret = commitState();
                }
            else
                {
                ret |= rollbackState();
                }

            break;

            }

        case CameraAdapter::CAMERA_START_VIDEO:
            {

            CAMHAL_LOGDA("Start video recording");

            if ( ret == NO_ERROR )
                {
                ret = setState(operation);
                }

            if ( ret == NO_ERROR )
                {
                ret = startVideoCapture();
                }

            if ( ret == NO_ERROR )
                {
                ret = commitState();
                }
            else
                {
                ret |= rollbackState();
                }

            break;

            }

        case CameraAdapter::CAMERA_STOP_VIDEO:
            {

            CAMHAL_LOGDA("Stop video recording");

            if ( ret == NO_ERROR )
                {
                ret = setState(operation);
                }

            if ( ret == NO_ERROR )
                {
                ret = stopVideoCapture();
                }

            if ( ret == NO_ERROR )
                {
                ret = commitState();
                }
            else
                {
                ret |= rollbackState();
                }

            break;

            }

        case CameraAdapter::CAMERA_PREVIEW_FLUSH_BUFFERS:
            {

            if ( ret == NO_ERROR )
                {
                ret = setState(operation);
                }

            if ( ret == NO_ERROR )
                {
                ret = flushBuffers();
                }

            if ( ret == NO_ERROR )
                {
                ret = commitState();
                }
            else
                {
                ret |= rollbackState();
                }

            break;

            }

        case CameraAdapter::CAMERA_START_IMAGE_CAPTURE:
            {

#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS

            refTimestamp = ( struct timeval * ) value1;
            if ( NULL != refTimestamp )
                {
                memcpy( &mStartCapture, refTimestamp, sizeof( struct timeval ));
                }

#endif

            if ( ret == NO_ERROR )
                {
                ret = setState(operation);
                }

            if ( ret == NO_ERROR )
                {
                ret = takePicture();
                }

            if ( ret == NO_ERROR )
                {
                ret = commitState();
                }
            else
                {
                ret |= rollbackState();
                }

            break;

            }

        case CameraAdapter::CAMERA_STOP_IMAGE_CAPTURE:
            {

            if ( ret == NO_ERROR )
                {
                ret = setState(operation);
                }

            if ( ret == NO_ERROR )
                {
                ret = stopImageCapture();
                }

            if ( ret == NO_ERROR )
                {
                ret = commitState();
                }
            else
                {
                ret |= rollbackState();
                }

            break;

            }

        case CameraAdapter::CAMERA_START_BRACKET_CAPTURE:
            {

#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS

            refTimestamp = ( struct timeval * ) value2;
            if ( NULL != refTimestamp )
                {
                memcpy( &mStartCapture, refTimestamp, sizeof( struct timeval ));
                }

#endif

            if ( ret == NO_ERROR )
                {
                ret = setState(operation);
                }

            if ( ret == NO_ERROR )
                {
                ret = startBracketing(value1);
                }

            if ( ret == NO_ERROR )
                {
                ret = commitState();
                }
            else
                {
                ret |= rollbackState();
                }

            break;

            }

        case CameraAdapter::CAMERA_STOP_BRACKET_CAPTURE:
            {

            if ( ret == NO_ERROR )
                {
                ret = setState(operation);
                }

            if ( ret == NO_ERROR )
                {
                ret = stopBracketing();
                }

            if ( ret == NO_ERROR )
                {
                ret = commitState();
                }
            else
                {
                ret |= rollbackState();
                }

            break;

            }

        case CameraAdapter::CAMERA_PERFORM_AUTOFOCUS:

#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS

            refTimestamp = ( struct timeval * ) value1;
            if ( NULL != refTimestamp )
                {
                memcpy( &mStartFocus, refTimestamp, sizeof( struct timeval ));
                }

#endif

            if ( ret == NO_ERROR )
                {
                ret = setState(operation);
                }

            if ( ret == NO_ERROR )
                {
                ret = autoFocus();
                }

            if ( ret == NO_ERROR )
                {
                ret = commitState();
                }
            else
                {
                ret |= rollbackState();
                }

            break;

        case CameraAdapter::CAMERA_CANCEL_AUTOFOCUS:

            if ( ret == NO_ERROR )
                {
                ret = setState(operation);
                }

            if ( ret == NO_ERROR )
                {
                ret = cancelAutoFocus();
                }

            if ( ret == NO_ERROR )
                {
                ret = commitState();
                }
            else
                {
                ret |= rollbackState();
                }

            break;

        case CameraAdapter::CAMERA_QUERY_RESOLUTION_PREVIEW:

             if ( ret == NO_ERROR )
                 {
                 ret = setState(operation);
                 }

             if ( ret == NO_ERROR )
                 {
                 frame = ( CameraFrame * ) value1;

                 if ( NULL != frame )
                     {
                     ret = getFrameSize(frame->mWidth, frame->mHeight);
                     }
                 else
                     {
                     ret = -EINVAL;
                     }
                 }

             if ( ret == NO_ERROR )
                 {
                 ret = commitState();
                 }
             else
                 {
                 ret |= rollbackState();
                 }

             break;

         case CameraAdapter::CAMERA_QUERY_BUFFER_SIZE_IMAGE_CAPTURE:

             if ( ret == NO_ERROR )
                 {
                 ret = setState(operation);
                 }

             if ( ret == NO_ERROR )
                 {
                 frame = ( CameraFrame * ) value1;

                 if ( NULL != frame )
                     {
                     ret = getPictureBufferSize(frame->mLength, value2);
                     }
                 else
                     {
                     ret = -EINVAL;
                     }
                 }

             if ( ret == NO_ERROR )
                 {
                 ret = commitState();
                 }
             else
                 {
                 ret |= rollbackState();
                 }

             break;

         case CameraAdapter::CAMERA_QUERY_BUFFER_SIZE_PREVIEW_DATA:

             if ( ret == NO_ERROR )
                 {
                 ret = setState(operation);
                 }

             if ( ret == NO_ERROR )
                 {
                 frame = ( CameraFrame * ) value1;

                 if ( NULL != frame )
                     {
                     ret = getFrameDataSize(frame->mLength, value2);
                     }
                 else
                     {
                     ret = -EINVAL;
                     }
                 }

             if ( ret == NO_ERROR )
                 {
                 ret = commitState();
                 }
             else
                 {
                 ret |= rollbackState();
                 }

             break;

         case CameraAdapter::CAMERA_START_FD:

             ret = startFaceDetection();

             break;

         case CameraAdapter::CAMERA_STOP_FD:

             ret = stopFaceDetection();

             break;

         case CameraAdapter::CAMERA_SWITCH_TO_EXECUTING:
           ret = switchToExecuting();
           break;

        default:
            CAMHAL_LOGEB("Command 0x%x unsupported!", operation);
            break;
    };

    LOG_FUNCTION_NAME_EXIT;
    return ret;
}

status_t BaseCameraAdapter::notifyFocusSubscribers(CameraHalEvent::FocusStatus status)
{
    event_callback eventCb;
    CameraHalEvent focusEvent;
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    if ( mFocusSubscribers.size() == 0 ) {
        CAMHAL_LOGDA("No Focus Subscribers!");
        return NO_INIT;
    }

#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS
     if (status == CameraHalEvent::FOCUS_STATUS_PENDING) {
        gettimeofday(&mStartFocus, NULL);
     } else {
        //dump the AF latency
        CameraHal::PPM("Focus finished in: ", &mStartFocus);
    }
#endif

    focusEvent.mEventData = new CameraHalEvent::CameraHalEventData();
    if ( NULL == focusEvent.mEventData.get() ) {
        return -ENOMEM;
    }

    focusEvent.mEventType = CameraHalEvent::EVENT_FOCUS_LOCKED;
    focusEvent.mEventData->focusEvent.focusStatus = status;

    for (unsigned int i = 0 ; i < mFocusSubscribers.size(); i++ )
        {
        focusEvent.mCookie = (void *) mFocusSubscribers.keyAt(i);
        eventCb = (event_callback) mFocusSubscribers.valueAt(i);
        eventCb ( &focusEvent );
        }

    focusEvent.mEventData.clear();

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t BaseCameraAdapter::notifyShutterSubscribers()
{
    CameraHalEvent shutterEvent;
    event_callback eventCb;
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    if ( mShutterSubscribers.size() == 0 )
        {
        CAMHAL_LOGEA("No shutter Subscribers!");
        return NO_INIT;
        }

    shutterEvent.mEventData = new CameraHalEvent::CameraHalEventData();
    if ( NULL == shutterEvent.mEventData.get() ) {
        return -ENOMEM;
    }

    shutterEvent.mEventType = CameraHalEvent::EVENT_SHUTTER;
    shutterEvent.mEventData->shutterEvent.shutterClosed = true;

    for (unsigned int i = 0 ; i < mShutterSubscribers.size() ; i++ ) {
        shutterEvent.mCookie = ( void * ) mShutterSubscribers.keyAt(i);
        eventCb = ( event_callback ) mShutterSubscribers.valueAt(i);

        CAMHAL_LOGDA("Sending shutter callback");

        eventCb ( &shutterEvent );
    }

    shutterEvent.mEventData.clear();

    LOG_FUNCTION_NAME;

    return ret;
}

status_t BaseCameraAdapter::notifyZoomSubscribers(int zoomIdx, bool targetReached)
{
    event_callback eventCb;
    CameraHalEvent zoomEvent;
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    if ( mZoomSubscribers.size() == 0 ) {
        CAMHAL_LOGDA("No zoom Subscribers!");
        return NO_INIT;
    }

    zoomEvent.mEventData = new CameraHalEvent::CameraHalEventData();
    if ( NULL == zoomEvent.mEventData.get() ) {
        return -ENOMEM;
    }

    zoomEvent.mEventType = CameraHalEvent::EVENT_ZOOM_INDEX_REACHED;
    zoomEvent.mEventData->zoomEvent.currentZoomIndex = zoomIdx;
    zoomEvent.mEventData->zoomEvent.targetZoomIndexReached = targetReached;

    for (unsigned int i = 0 ; i < mZoomSubscribers.size(); i++ ) {
        zoomEvent.mCookie = (void *) mZoomSubscribers.keyAt(i);
        eventCb = (event_callback) mZoomSubscribers.valueAt(i);

        eventCb ( &zoomEvent );
    }

    zoomEvent.mEventData.clear();

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t BaseCameraAdapter::notifyFaceSubscribers(sp<CameraFDResult> &faces)
{
    event_callback eventCb;
    CameraHalEvent faceEvent;
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    if ( mFaceSubscribers.size() == 0 ) {
        CAMHAL_LOGDA("No face detection subscribers!");
        return NO_INIT;
    }

    faceEvent.mEventData = new CameraHalEvent::CameraHalEventData();
    if ( NULL == faceEvent.mEventData.get() ) {
        return -ENOMEM;
    }

    faceEvent.mEventType = CameraHalEvent::EVENT_FACE;
    faceEvent.mEventData->faceEvent = faces;

    for (unsigned int i = 0 ; i < mFaceSubscribers.size(); i++ ) {
        faceEvent.mCookie = (void *) mFaceSubscribers.keyAt(i);
        eventCb = (event_callback) mFaceSubscribers.valueAt(i);

        eventCb ( &faceEvent );
    }

    faceEvent.mEventData.clear();

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t BaseCameraAdapter::sendFrameToSubscribers(CameraFrame *frame)
{
    status_t ret = NO_ERROR;
    unsigned int mask;

    if ( NULL == frame )
        {
        CAMHAL_LOGEA("Invalid CameraFrame");
        return -EINVAL;
        }

    for( mask = 1; mask < CameraFrame::ALL_FRAMES; mask <<= 1){
      if( mask & frame->mFrameMask ){
        switch( mask ){

        case CameraFrame::IMAGE_FRAME:
          {
#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS
            CameraHal::PPM("Shot to Jpeg: ", &mStartCapture);
#endif
            ret = __sendFrameToSubscribers(frame, &mImageSubscribers, CameraFrame::IMAGE_FRAME);
          }
          break;
        case CameraFrame::RAW_FRAME:
          {
            ret = __sendFrameToSubscribers(frame, &mRawSubscribers, CameraFrame::RAW_FRAME);
          }
          break;
        case CameraFrame::PREVIEW_FRAME_SYNC:
          {
            ret = __sendFrameToSubscribers(frame, &mFrameSubscribers, CameraFrame::PREVIEW_FRAME_SYNC);
          }
          break;
        case CameraFrame::SNAPSHOT_FRAME:
          {
            ret = __sendFrameToSubscribers(frame, &mFrameSubscribers, CameraFrame::SNAPSHOT_FRAME);
          }
          break;
        case CameraFrame::VIDEO_FRAME_SYNC:
          {
            ret = __sendFrameToSubscribers(frame, &mVideoSubscribers, CameraFrame::VIDEO_FRAME_SYNC);
          }
          break;
        case CameraFrame::FRAME_DATA_SYNC:
          {
            ret = __sendFrameToSubscribers(frame, &mFrameDataSubscribers, CameraFrame::FRAME_DATA_SYNC);
          }
          break;
        default:
          CAMHAL_LOGEB("FRAMETYPE NOT SUPPORTED 0x%x", mask);
        break;
        }//SWITCH
        frame->mFrameMask &= ~mask;

        if (ret != NO_ERROR) {
            goto EXIT;
        }
      }//IF
    }//FOR

 EXIT:
    return ret;
}

status_t BaseCameraAdapter::__sendFrameToSubscribers(CameraFrame* frame,
                                                     KeyedVector<int, frame_callback> *subscribers,
                                                     CameraFrame::FrameType frameType)
{
    size_t refCount = 0;
    status_t ret = NO_ERROR;
    frame_callback callback = NULL;

    frame->mFrameType = frameType;

    if ( (frameType == CameraFrame::PREVIEW_FRAME_SYNC) ||
         (frameType == CameraFrame::VIDEO_FRAME_SYNC) ||
         (frameType == CameraFrame::SNAPSHOT_FRAME) ){
        if (mFrameQueue.size() > 0){
          CameraFrame *lframe = (CameraFrame *)mFrameQueue.valueFor(frame->mBuffer);
          frame->mYuv[0] = lframe->mYuv[0];
          frame->mYuv[1] = lframe->mYuv[1];
        }
        else{
          CAMHAL_LOGDA("Empty Frame Queue");
          return -EINVAL;
        }
      }

    if (NULL != subscribers) {
        refCount = getFrameRefCount(frame->mBuffer, frameType);

        if (refCount == 0) {
            CAMHAL_LOGDA("Invalid ref count of 0");
            return -EINVAL;
        }

        if (refCount > subscribers->size()) {
            CAMHAL_LOGEB("Invalid ref count for frame type: 0x%x", frameType);
            return -EINVAL;
        }

        CAMHAL_LOGVB("Type of Frame: 0x%x address: 0x%x refCount start %d",
                     frame->mFrameType,
                     ( uint32_t ) frame->mBuffer,
                     refCount);

        for ( unsigned int i = 0 ; i < refCount; i++ ) {
            frame->mCookie = ( void * ) subscribers->keyAt(i);
            callback = (frame_callback) subscribers->valueAt(i);

            if (!callback) {
                CAMHAL_LOGEB("callback not set for frame type: 0x%x", frameType);
                return -EINVAL;
            }

            callback(frame);
        }
    } else {
        CAMHAL_LOGEA("Subscribers is null??");
        return -EINVAL;
    }

    return ret;
}

int BaseCameraAdapter::setInitFrameRefCount(void* buf, unsigned int mask)
{
  int ret = NO_ERROR;
  unsigned int lmask;

  LOG_FUNCTION_NAME;

  if (buf == NULL)
    {
      return -EINVAL;
    }

  for( lmask = 1; lmask < CameraFrame::ALL_FRAMES; lmask <<= 1){
    if( lmask & mask ){
      switch( lmask ){

      case CameraFrame::IMAGE_FRAME:
        {
          setFrameRefCount(buf, CameraFrame::IMAGE_FRAME, (int) mImageSubscribers.size());
        }
        break;
      case CameraFrame::RAW_FRAME:
        {
          setFrameRefCount(buf, CameraFrame::RAW_FRAME, mRawSubscribers.size());
        }
        break;
      case CameraFrame::PREVIEW_FRAME_SYNC:
        {
          setFrameRefCount(buf, CameraFrame::PREVIEW_FRAME_SYNC, mFrameSubscribers.size());
        }
        break;
      case CameraFrame::SNAPSHOT_FRAME:
        {
          setFrameRefCount(buf, CameraFrame::SNAPSHOT_FRAME, mFrameSubscribers.size());
        }
        break;
      case CameraFrame::VIDEO_FRAME_SYNC:
        {
          setFrameRefCount(buf,CameraFrame::VIDEO_FRAME_SYNC, mVideoSubscribers.size());
        }
        break;
      case CameraFrame::FRAME_DATA_SYNC:
        {
          setFrameRefCount(buf, CameraFrame::FRAME_DATA_SYNC, mFrameDataSubscribers.size());
        }
        break;
      default:
        CAMHAL_LOGEB("FRAMETYPE NOT SUPPORTED 0x%x", lmask);
        break;
      }//SWITCH
      mask &= ~lmask;
    }//IF
  }//FOR
  LOG_FUNCTION_NAME_EXIT;
  return ret;
}

int BaseCameraAdapter::getFrameRefCount(void* frameBuf, CameraFrame::FrameType frameType)
{
    int res = -1;

    LOG_FUNCTION_NAME;

    switch ( frameType )
        {
        case CameraFrame::IMAGE_FRAME:
        case CameraFrame::RAW_FRAME:
                {
                Mutex::Autolock lock(mCaptureBufferLock);
                res = mCaptureBuffersAvailable.valueFor( ( unsigned int ) frameBuf );
                }
            break;
        case CameraFrame::PREVIEW_FRAME_SYNC:
        case CameraFrame::SNAPSHOT_FRAME:
                {
                Mutex::Autolock lock(mPreviewBufferLock);
                res = mPreviewBuffersAvailable.valueFor( ( unsigned int ) frameBuf );
                }
            break;
        case CameraFrame::FRAME_DATA_SYNC:
                {
                Mutex::Autolock lock(mPreviewDataBufferLock);
                res = mPreviewDataBuffersAvailable.valueFor( ( unsigned int ) frameBuf );
                }
            break;
        case CameraFrame::VIDEO_FRAME_SYNC:
                {
                Mutex::Autolock lock(mVideoBufferLock);
                res = mVideoBuffersAvailable.valueFor( ( unsigned int ) frameBuf );
                }
            break;
        default:
            break;
        };

    LOG_FUNCTION_NAME_EXIT;

    return res;
}

void BaseCameraAdapter::setFrameRefCount(void* frameBuf, CameraFrame::FrameType frameType, int refCount)
{

    LOG_FUNCTION_NAME;

    switch ( frameType )
        {
        case CameraFrame::IMAGE_FRAME:
        case CameraFrame::RAW_FRAME:
                {
                Mutex::Autolock lock(mCaptureBufferLock);
                mCaptureBuffersAvailable.replaceValueFor(  ( unsigned int ) frameBuf, refCount);
                }
            break;
        case CameraFrame::PREVIEW_FRAME_SYNC:
        case CameraFrame::SNAPSHOT_FRAME:
                {
                Mutex::Autolock lock(mPreviewBufferLock);
                mPreviewBuffersAvailable.replaceValueFor(  ( unsigned int ) frameBuf, refCount);
                }
            break;
        case CameraFrame::FRAME_DATA_SYNC:
                {
                Mutex::Autolock lock(mPreviewDataBufferLock);
                mPreviewDataBuffersAvailable.replaceValueFor(  ( unsigned int ) frameBuf, refCount);
                }
            break;
        case CameraFrame::VIDEO_FRAME_SYNC:
                {
                Mutex::Autolock lock(mVideoBufferLock);
                mVideoBuffersAvailable.replaceValueFor(  ( unsigned int ) frameBuf, refCount);
                }
            break;
        default:
            break;
        };

    LOG_FUNCTION_NAME_EXIT;

}

status_t BaseCameraAdapter::startVideoCapture()
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    Mutex::Autolock lock(mVideoBufferLock);

    //If the capture is already ongoing, return from here.
    if ( mRecording )
        {
        ret = NO_INIT;
        }


    if ( NO_ERROR == ret )
        {

        mVideoBuffersAvailable.clear();

        for ( unsigned int i = 0 ; i < mPreviewBuffersAvailable.size() ; i++ )
            {
            mVideoBuffersAvailable.add(mPreviewBuffersAvailable.keyAt(i), 0);
            }

        mRecording = true;
        }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t BaseCameraAdapter::stopVideoCapture()
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    if ( !mRecording )
        {
        ret = NO_INIT;
        }

    if ( NO_ERROR == ret )
        {
        for ( unsigned int i = 0 ; i < mVideoBuffersAvailable.size() ; i++ )
            {
            void *frameBuf = ( void * ) mVideoBuffersAvailable.keyAt(i);
            if( getFrameRefCount(frameBuf,  CameraFrame::VIDEO_FRAME_SYNC) > 0)
                {
                returnFrame(frameBuf, CameraFrame::VIDEO_FRAME_SYNC);
                }
            }

        mRecording = false;
        }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

//-----------------Stub implementation of the interface ------------------------------

status_t BaseCameraAdapter::takePicture()
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t BaseCameraAdapter::stopImageCapture()
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t BaseCameraAdapter::startBracketing(int range)
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t BaseCameraAdapter::stopBracketing()
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t BaseCameraAdapter::autoFocus()
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    notifyFocusSubscribers(CameraHalEvent::FOCUS_STATUS_FAIL);

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t BaseCameraAdapter::cancelAutoFocus()
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t BaseCameraAdapter::startSmoothZoom(int targetIdx)
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t BaseCameraAdapter::stopSmoothZoom()
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t BaseCameraAdapter::startPreview()
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t BaseCameraAdapter::stopPreview()
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t BaseCameraAdapter::useBuffers(CameraMode mode, void* bufArr, int num, size_t length, unsigned int queueable)
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t BaseCameraAdapter::fillThisBuffer(void* frameBuf, CameraFrame::FrameType frameType)
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t BaseCameraAdapter::getFrameSize(size_t &width, size_t &height)
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t BaseCameraAdapter::getFrameDataSize(size_t &dataFrameSize, size_t bufferCount)
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t BaseCameraAdapter::getPictureBufferSize(size_t &length, size_t bufferCount)
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t BaseCameraAdapter::startFaceDetection()
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t BaseCameraAdapter::stopFaceDetection()
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t BaseCameraAdapter::switchToExecuting()
{
  status_t ret = NO_ERROR;
  LOG_FUNCTION_NAME;
  LOG_FUNCTION_NAME_EXIT;
  return ret;
}

status_t BaseCameraAdapter::setState(CameraCommands operation)
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    mLock.lock();

    switch ( mAdapterState )
        {

        case INTIALIZED_STATE:

            switch ( operation )
                {

                case CAMERA_USE_BUFFERS_PREVIEW:
                    CAMHAL_LOGDB("Adapter state switch INTIALIZED_STATE->LOADED_PREVIEW_STATE event = 0x%x",
                                 operation);
                    mNextState = LOADED_PREVIEW_STATE;
                    break;

                //These events don't change the current state
                case CAMERA_QUERY_RESOLUTION_PREVIEW:
                case CAMERA_QUERY_BUFFER_SIZE_IMAGE_CAPTURE:
                case CAMERA_QUERY_BUFFER_SIZE_PREVIEW_DATA:
                    CAMHAL_LOGDB("Adapter state switch INTIALIZED_STATE->INTIALIZED_STATE event = 0x%x",
                                 operation);
                    mNextState = INTIALIZED_STATE;
                    break;

                case CAMERA_CANCEL_AUTOFOCUS:
                case CAMERA_STOP_BRACKET_CAPTURE:
                case CAMERA_STOP_IMAGE_CAPTURE:
                    ret = INVALID_OPERATION;
                    break;

                default:
                    CAMHAL_LOGEB("Adapter state switch INTIALIZED_STATE Invalid Op! event = 0x%x",
                                 operation);
                    ret = INVALID_OPERATION;
                    break;

                }

            break;

        case LOADED_PREVIEW_STATE:

            switch ( operation )
                {

                case CAMERA_START_PREVIEW:
                    CAMHAL_LOGDB("Adapter state switch LOADED_PREVIEW_STATE->PREVIEW_STATE event = 0x%x",
                                 operation);
                    mNextState = PREVIEW_STATE;
                    break;

                case CAMERA_STOP_PREVIEW:
                    CAMHAL_LOGDB("Adapter state switch LOADED_PREVIEW_STATE->INTIALIZED_STATE event = 0x%x",
                                 operation);
                    mNextState = INTIALIZED_STATE;
                    break;

                //These events don't change the current state
                case CAMERA_QUERY_BUFFER_SIZE_IMAGE_CAPTURE:
                case CAMERA_QUERY_BUFFER_SIZE_PREVIEW_DATA:
                case CAMERA_USE_BUFFERS_PREVIEW_DATA:
                    CAMHAL_LOGDB("Adapter state switch LOADED_PREVIEW_STATE->LOADED_PREVIEW_STATE event = 0x%x",
                                 operation);
                    mNextState = LOADED_PREVIEW_STATE;
                    break;

                default:
                    CAMHAL_LOGDB("Adapter state switch LOADED_PREVIEW Invalid Op! event = 0x%x",
                                 operation);
                    ret = INVALID_OPERATION;
                    break;

                }

            break;

        case PREVIEW_STATE:

            switch ( operation )
                {

                case CAMERA_STOP_PREVIEW:
                    CAMHAL_LOGDB("Adapter state switch PREVIEW_STATE->INTIALIZED_STATE event = 0x%x",
                                 operation);
                    mNextState = INTIALIZED_STATE;
                    break;

                case CAMERA_PERFORM_AUTOFOCUS:
                    CAMHAL_LOGDB("Adapter state switch PREVIEW_STATE->AF_STATE event = 0x%x",
                                 operation);
                    mNextState = AF_STATE;
                    break;

                case CAMERA_START_SMOOTH_ZOOM:
                    CAMHAL_LOGDB("Adapter state switch PREVIEW_STATE->ZOOM_STATE event = 0x%x",
                                 operation);
                    mNextState = ZOOM_STATE;
                    break;

                case CAMERA_USE_BUFFERS_IMAGE_CAPTURE:
                    CAMHAL_LOGDB("Adapter state switch PREVIEW_STATE->LOADED_CAPTURE_STATE event = 0x%x",
                                 operation);
                    mNextState = LOADED_CAPTURE_STATE;
                    break;

                case CAMERA_START_VIDEO:
                    CAMHAL_LOGDB("Adapter state switch PREVIEW_STATE->VIDEO_STATE event = 0x%x",
                                 operation);
                    mNextState = VIDEO_STATE;
                    break;

                case CAMERA_CANCEL_AUTOFOCUS:
                case CAMERA_QUERY_BUFFER_SIZE_IMAGE_CAPTURE:
                case CAMERA_STOP_SMOOTH_ZOOM:
                    CAMHAL_LOGDB("Adapter state switch PREVIEW_ACTIVE->PREVIEW_ACTIVE event = 0x%x",
                                 operation);
                    mNextState = PREVIEW_STATE;
                    break;

                default:
                    CAMHAL_LOGEB("Adapter state switch PREVIEW_ACTIVE Invalid Op! event = 0x%x",
                                 operation);
                    ret = INVALID_OPERATION;
                    break;

                }

            break;

        case LOADED_CAPTURE_STATE:

            switch ( operation )
                {

                case CAMERA_START_IMAGE_CAPTURE:
                    CAMHAL_LOGDB("Adapter state switch LOADED_CAPTURE_STATE->CAPTURE_STATE event = 0x%x",
                                 operation);
                    mNextState = CAPTURE_STATE;
                    break;

                case CAMERA_START_BRACKET_CAPTURE:
                    CAMHAL_LOGDB("Adapter state switch LOADED_CAPTURE_STATE->BRACKETING_STATE event = 0x%x",
                                 operation);
                    mNextState = BRACKETING_STATE;
                    break;

                default:
                    CAMHAL_LOGEB("Adapter state switch LOADED_CAPTURE_STATE Invalid Op! event = 0x%x",
                                 operation);
                    ret = INVALID_OPERATION;
                    break;

                }

            break;

        case CAPTURE_STATE:

            switch ( operation )
                {
                case CAMERA_STOP_IMAGE_CAPTURE:
                case CAMERA_STOP_BRACKET_CAPTURE:
                    CAMHAL_LOGDB("Adapter state switch CAPTURE_STATE->PREVIEW_STATE event = 0x%x",
                                 operation);
                    mNextState = PREVIEW_STATE;
                    break;

                default:
                    CAMHAL_LOGEB("Adapter state switch CAPTURE_STATE Invalid Op! event = 0x%x",
                                 operation);
                    ret = INVALID_OPERATION;
                    break;

                }

            break;

        case BRACKETING_STATE:

            switch ( operation )
                {

                case CAMERA_STOP_IMAGE_CAPTURE:
                case CAMERA_STOP_BRACKET_CAPTURE:
                    CAMHAL_LOGDB("Adapter state switch BRACKETING_STATE->PREVIEW_STATE event = 0x%x",
                                 operation);
                    mNextState = PREVIEW_STATE;
                    break;

                case CAMERA_START_IMAGE_CAPTURE:
                    CAMHAL_LOGDB("Adapter state switch BRACKETING_STATE->CAPTURE_STATE event = 0x%x",
                                 operation);
                    mNextState = CAPTURE_STATE;
                    break;

                default:
                    CAMHAL_LOGEB("Adapter state switch BRACKETING_STATE Invalid Op! event = 0x%x",
                                 operation);
                    ret = INVALID_OPERATION;
                    break;

                }

            break;

        case AF_STATE:

            switch ( operation )
                {

                case CAMERA_CANCEL_AUTOFOCUS:
                    CAMHAL_LOGDB("Adapter state switch AF_STATE->PREVIEW_STATE event = 0x%x",
                                 operation);
                    mNextState = PREVIEW_STATE;
                    break;

                case CAMERA_START_IMAGE_CAPTURE:
                    CAMHAL_LOGDB("Adapter state switch AF_STATE->CAPTURE_STATE event = 0x%x",
                                 operation);
                    mNextState = CAPTURE_STATE;
                    break;

                case CAMERA_START_SMOOTH_ZOOM:
                    CAMHAL_LOGDB("Adapter state switch AF_STATE->AF_ZOOM_STATE event = 0x%x",
                                 operation);
                    mNextState = AF_ZOOM_STATE;
                    break;

                default:
                    CAMHAL_LOGEB("Adapter state switch AF_STATE Invalid Op! event = 0x%x",
                                 operation);
                    ret = INVALID_OPERATION;
                    break;

                }

            break;

        case ZOOM_STATE:

            switch ( operation )
                {

                case CAMERA_CANCEL_AUTOFOCUS:
                    CAMHAL_LOGDB("Adapter state switch AF_STATE->PREVIEW_STATE event = 0x%x",
                                 operation);
                    mNextState = ZOOM_STATE;
                    break;

                case CAMERA_STOP_SMOOTH_ZOOM:
                    CAMHAL_LOGDB("Adapter state switch ZOOM_STATE->PREVIEW_STATE event = 0x%x",
                                 operation);
                    mNextState = PREVIEW_STATE;
                    break;

                case CAMERA_PERFORM_AUTOFOCUS:
                    CAMHAL_LOGDB("Adapter state switch ZOOM_STATE->AF_ZOOM_STATE event = 0x%x",
                                 operation);
                    mNextState = AF_ZOOM_STATE;
                    break;

                case CAMERA_START_VIDEO:
                    CAMHAL_LOGDB("Adapter state switch ZOOM_STATE->VIDEO_ZOOM_STATE event = 0x%x",
                                 operation);
                    mNextState = VIDEO_ZOOM_STATE;
                    break;

                default:
                    CAMHAL_LOGEB("Adapter state switch ZOOM_STATE Invalid Op! event = 0x%x",
                                 operation);
                    ret = INVALID_OPERATION;
                    break;

                }

            break;

        case VIDEO_STATE:

            switch ( operation )
                {

                case CAMERA_STOP_VIDEO:
                    CAMHAL_LOGDB("Adapter state switch VIDEO_STATE->PREVIEW_STATE event = 0x%x",
                                 operation);
                    mNextState = PREVIEW_STATE;
                    break;

                case CAMERA_PERFORM_AUTOFOCUS:
                    CAMHAL_LOGDB("Adapter state switch VIDEO_STATE->VIDEO_AF_STATE event = 0x%x",
                                 operation);
                    mNextState = VIDEO_AF_STATE;
                    break;

                case CAMERA_START_SMOOTH_ZOOM:
                    CAMHAL_LOGDB("Adapter state switch VIDEO_STATE->VIDEO_ZOOM_STATE event = 0x%x",
                                 operation);
                    mNextState = VIDEO_ZOOM_STATE;
                    break;

                case CAMERA_USE_BUFFERS_IMAGE_CAPTURE:
                    CAMHAL_LOGDB("Adapter state switch VIDEO_STATE->VIDEO_LOADED_CAPTURE_STATE event = 0x%x",
                                 operation);
                    mNextState = VIDEO_LOADED_CAPTURE_STATE;
                    break;

                case CAMERA_QUERY_BUFFER_SIZE_IMAGE_CAPTURE:
                    CAMHAL_LOGDB("Adapter state switch VIDEO_STATE->VIDEO_STATE event = 0x%x",
                                 operation);
                    mNextState = VIDEO_STATE;
                    break;

                default:
                    CAMHAL_LOGEB("Adapter state switch VIDEO_STATE Invalid Op! event = 0x%x",
                                 operation);
                    ret = INVALID_OPERATION;
                    break;

                }

            break;

        case VIDEO_AF_STATE:

            switch ( operation )
                {

                case CAMERA_CANCEL_AUTOFOCUS:
                    CAMHAL_LOGDB("Adapter state switch VIDEO_AF_STATE->VIDEO_STATE event = 0x%x",
                                 operation);
                    mNextState = VIDEO_STATE;
                    break;

                default:
                    CAMHAL_LOGEB("Adapter state switch VIDEO_AF_STATE Invalid Op! event = 0x%x",
                                 operation);
                    ret = INVALID_OPERATION;
                    break;

                }

            break;

        case VIDEO_LOADED_CAPTURE_STATE:

            switch ( operation )
                {

                case CAMERA_START_IMAGE_CAPTURE:
                    CAMHAL_LOGDB("Adapter state switch LOADED_CAPTURE_STATE->CAPTURE_STATE event = 0x%x",
                                 operation);
                    mNextState = VIDEO_CAPTURE_STATE;
                    break;

                default:
                    CAMHAL_LOGEB("Adapter state switch LOADED_CAPTURE_STATE Invalid Op! event = 0x%x",
                                 operation);
                    ret = INVALID_OPERATION;
                    break;

                }

            break;

        case VIDEO_CAPTURE_STATE:

            switch ( operation )
                {
                case CAMERA_STOP_IMAGE_CAPTURE:
                    CAMHAL_LOGDB("Adapter state switch CAPTURE_STATE->PREVIEW_STATE event = 0x%x",
                                 operation);
                    mNextState = VIDEO_STATE;
                    break;

                default:
                    CAMHAL_LOGEB("Adapter state switch CAPTURE_STATE Invalid Op! event = 0x%x",
                                 operation);
                    ret = INVALID_OPERATION;
                    break;

                }

            break;

        case AF_ZOOM_STATE:

            switch ( operation )
                {

                case CAMERA_STOP_SMOOTH_ZOOM:
                    CAMHAL_LOGDB("Adapter state switch AF_ZOOM_STATE->AF_STATE event = 0x%x",
                                 operation);
                    mNextState = AF_STATE;
                    break;

                case CAMERA_CANCEL_AUTOFOCUS:
                    CAMHAL_LOGDB("Adapter state switch AF_ZOOM_STATE->ZOOM_STATE event = 0x%x",
                                 operation);
                    mNextState = ZOOM_STATE;
                    break;

                default:
                    CAMHAL_LOGEB("Adapter state switch AF_ZOOM_STATE Invalid Op! event = 0x%x",
                                 operation);
                    ret = INVALID_OPERATION;
                    break;

                }

            break;

        case VIDEO_ZOOM_STATE:

            switch ( operation )
                {

                case CAMERA_STOP_SMOOTH_ZOOM:
                    CAMHAL_LOGDB("Adapter state switch VIDEO_ZOOM_STATE->VIDEO_STATE event = 0x%x",
                                 operation);
                    mNextState = VIDEO_STATE;
                    break;

                case CAMERA_STOP_VIDEO:
                    CAMHAL_LOGDB("Adapter state switch VIDEO_ZOOM_STATE->ZOOM_STATE event = 0x%x",
                                 operation);
                    mNextState = ZOOM_STATE;
                    break;

                default:
                    CAMHAL_LOGEB("Adapter state switch VIDEO_ZOOM_STATE Invalid Op! event = 0x%x",
                                 operation);
                    ret = INVALID_OPERATION;
                    break;

                }

            break;

        case BRACKETING_ZOOM_STATE:

            switch ( operation )
                {

                case CAMERA_STOP_SMOOTH_ZOOM:
                    CAMHAL_LOGDB("Adapter state switch BRACKETING_ZOOM_STATE->BRACKETING_STATE event = 0x%x",
                                 operation);
                    mNextState = BRACKETING_STATE;
                    break;

                default:
                    CAMHAL_LOGEB("Adapter state switch BRACKETING_ZOOM_STATE Invalid Op! event = 0x%x",
                                 operation);
                    ret = INVALID_OPERATION;
                    break;

                }

            break;

        default:
            CAMHAL_LOGEA("Invalid Adapter state!");
            ret = INVALID_OPERATION;
        }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t BaseCameraAdapter::rollbackToInitializedState()
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    while ((getState() != INTIALIZED_STATE) && (ret == NO_ERROR)) {
        ret = rollbackToPreviousState();
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t BaseCameraAdapter::rollbackToPreviousState()
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    CameraAdapter::AdapterState currentState = getState();

    switch (currentState) {
        case INTIALIZED_STATE:
            return NO_ERROR;

        case PREVIEW_STATE:
            ret = sendCommand(CAMERA_STOP_PREVIEW);
            break;

        case CAPTURE_STATE:
            ret = sendCommand(CAMERA_STOP_IMAGE_CAPTURE);
            break;

        case BRACKETING_STATE:
            ret = sendCommand(CAMERA_STOP_BRACKET_CAPTURE);
            break;

        case AF_STATE:
            ret = sendCommand(CAMERA_CANCEL_AUTOFOCUS);
            break;

        case ZOOM_STATE:
            ret = sendCommand(CAMERA_STOP_SMOOTH_ZOOM);
            break;

        case VIDEO_STATE:
            ret = sendCommand(CAMERA_STOP_VIDEO);
            break;

        case VIDEO_AF_STATE:
            ret = sendCommand(CAMERA_CANCEL_AUTOFOCUS);
            break;

        case VIDEO_CAPTURE_STATE:
            ret = sendCommand(CAMERA_STOP_IMAGE_CAPTURE);
            break;

        case AF_ZOOM_STATE:
            ret = sendCommand(CAMERA_STOP_SMOOTH_ZOOM);
            break;

        case VIDEO_ZOOM_STATE:
            ret = sendCommand(CAMERA_STOP_SMOOTH_ZOOM);
            break;

        case BRACKETING_ZOOM_STATE:
            ret = sendCommand(CAMERA_STOP_SMOOTH_ZOOM);
            break;

        default:
            CAMHAL_LOGEA("Invalid Adapter state!");
            ret = INVALID_OPERATION;
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

//State transition finished successfully.
//Commit the state and unlock the adapter state.
status_t BaseCameraAdapter::commitState()
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    mAdapterState = mNextState;

    mLock.unlock();

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t BaseCameraAdapter::rollbackState()
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    mNextState = mAdapterState;

    mLock.unlock();

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

// getNextState() and getState()
// publicly exposed functions to retrieve the adapter states
// please notice that these functions are locked
CameraAdapter::AdapterState BaseCameraAdapter::getState()
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    Mutex::Autolock lock(mLock);

    LOG_FUNCTION_NAME_EXIT;

    return mAdapterState;
}

CameraAdapter::AdapterState BaseCameraAdapter::getNextState()
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    Mutex::Autolock lock(mLock);

    LOG_FUNCTION_NAME_EXIT;

    return mNextState;
}

// getNextState() and getState()
// internal protected functions to retrieve the adapter states
// please notice that these functions are NOT locked to help
// internal functions query state in the middle of state
// transition
status_t BaseCameraAdapter::getState(AdapterState &state)
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    state = mAdapterState;

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t BaseCameraAdapter::getNextState(AdapterState &state)
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    state = mNextState;

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

void BaseCameraAdapter::onOrientationEvent(uint32_t orientation, uint32_t tilt)
{
    LOG_FUNCTION_NAME;
    LOG_FUNCTION_NAME_EXIT;
}
//-----------------------------------------------------------------------------



};

/*--------------------Camera Adapter Class ENDS here-----------------------------*/

