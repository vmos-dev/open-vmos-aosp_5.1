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

#include "BaseCameraAdapter.h"

const int EVENT_MASK = 0xffff;

namespace Ti {
namespace Camera {

const LUT cameraCommandsUserToHAL[] = {
    { "CAMERA_START_PREVIEW",                   CameraAdapter::CAMERA_START_PREVIEW },
    { "CAMERA_STOP_PREVIEW",                    CameraAdapter::CAMERA_STOP_PREVIEW },
    { "CAMERA_START_VIDEO",                     CameraAdapter::CAMERA_START_VIDEO },
    { "CAMERA_STOP_VIDEO",                      CameraAdapter::CAMERA_STOP_VIDEO },
    { "CAMERA_START_IMAGE_CAPTURE",             CameraAdapter::CAMERA_START_IMAGE_CAPTURE },
    { "CAMERA_STOP_IMAGE_CAPTURE",              CameraAdapter::CAMERA_STOP_IMAGE_CAPTURE },
    { "CAMERA_PERFORM_AUTOFOCUS",               CameraAdapter::CAMERA_PERFORM_AUTOFOCUS },
    { "CAMERA_CANCEL_AUTOFOCUS",                CameraAdapter::CAMERA_CANCEL_AUTOFOCUS },
    { "CAMERA_PREVIEW_FLUSH_BUFFERS",           CameraAdapter::CAMERA_PREVIEW_FLUSH_BUFFERS },
    { "CAMERA_START_SMOOTH_ZOOM",               CameraAdapter::CAMERA_START_SMOOTH_ZOOM },
    { "CAMERA_STOP_SMOOTH_ZOOM",                CameraAdapter::CAMERA_STOP_SMOOTH_ZOOM },
    { "CAMERA_USE_BUFFERS_PREVIEW",             CameraAdapter::CAMERA_USE_BUFFERS_PREVIEW },
    { "CAMERA_SET_TIMEOUT",                     CameraAdapter::CAMERA_SET_TIMEOUT },
    { "CAMERA_CANCEL_TIMEOUT",                  CameraAdapter::CAMERA_CANCEL_TIMEOUT },
    { "CAMERA_START_BRACKET_CAPTURE",           CameraAdapter::CAMERA_START_BRACKET_CAPTURE },
    { "CAMERA_STOP_BRACKET_CAPTURE",            CameraAdapter::CAMERA_STOP_BRACKET_CAPTURE },
    { "CAMERA_QUERY_RESOLUTION_PREVIEW",        CameraAdapter::CAMERA_QUERY_RESOLUTION_PREVIEW },
    { "CAMERA_QUERY_BUFFER_SIZE_IMAGE_CAPTURE", CameraAdapter::CAMERA_QUERY_BUFFER_SIZE_IMAGE_CAPTURE },
    { "CAMERA_QUERY_BUFFER_SIZE_PREVIEW_DATA",  CameraAdapter::CAMERA_QUERY_BUFFER_SIZE_PREVIEW_DATA },
    { "CAMERA_USE_BUFFERS_IMAGE_CAPTURE",       CameraAdapter::CAMERA_USE_BUFFERS_IMAGE_CAPTURE },
    { "CAMERA_USE_BUFFERS_PREVIEW_DATA",        CameraAdapter::CAMERA_USE_BUFFERS_PREVIEW_DATA },
    { "CAMERA_TIMEOUT_EXPIRED",                 CameraAdapter::CAMERA_TIMEOUT_EXPIRED },
    { "CAMERA_START_FD",                        CameraAdapter::CAMERA_START_FD },
    { "CAMERA_STOP_FD",                         CameraAdapter::CAMERA_STOP_FD },
    { "CAMERA_SWITCH_TO_EXECUTING",             CameraAdapter::CAMERA_SWITCH_TO_EXECUTING },
    { "CAMERA_USE_BUFFERS_VIDEO_CAPTURE",       CameraAdapter::CAMERA_USE_BUFFERS_VIDEO_CAPTURE },
#ifdef OMAP_ENHANCEMENT_CPCAM
    { "CAMERA_USE_BUFFERS_REPROCESS",           CameraAdapter::CAMERA_USE_BUFFERS_REPROCESS },
    { "CAMERA_START_REPROCESS",                 CameraAdapter::CAMERA_START_REPROCESS },
#endif
};

const LUTtypeHAL CamCommandsLUT = {
    sizeof(cameraCommandsUserToHAL)/sizeof(cameraCommandsUserToHAL[0]),
    cameraCommandsUserToHAL
};

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

    mSharedAllocator = NULL;

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

     android::AutoMutex lock(mSubscriberLock);

     mFrameSubscribers.clear();
     mImageSubscribers.clear();
     mRawSubscribers.clear();
     mVideoSubscribers.clear();
     mVideoInSubscribers.clear();
     mFocusSubscribers.clear();
     mShutterSubscribers.clear();
     mZoomSubscribers.clear();
     mSnapshotSubscribers.clear();
     mMetadataSubscribers.clear();

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
    android::AutoMutex lock(mSubscriberLock);

    LOG_FUNCTION_NAME;

    int32_t frameMsg = ((msgs >> MessageNotifier::FRAME_BIT_FIELD_POSITION) & EVENT_MASK);
    int32_t eventMsg = ((msgs >> MessageNotifier::EVENT_BIT_FIELD_POSITION) & EVENT_MASK);

    if ( frameMsg != 0 )
        {
        CAMHAL_LOGVB("Frame message type id=0x%x subscription request", frameMsg);
        switch ( frameMsg )
            {
            case CameraFrame::PREVIEW_FRAME_SYNC:
                mFrameSubscribers.add((int) cookie, callback);
                break;
            case CameraFrame::FRAME_DATA_SYNC:
                mFrameDataSubscribers.add((int) cookie, callback);
                break;
            case CameraFrame::SNAPSHOT_FRAME:
                mSnapshotSubscribers.add((int) cookie, callback);
                break;
            case CameraFrame::IMAGE_FRAME:
                mImageSubscribers.add((int) cookie, callback);
                break;
            case CameraFrame::RAW_FRAME:
                mRawSubscribers.add((int) cookie, callback);
                break;
            case CameraFrame::VIDEO_FRAME_SYNC:
                mVideoSubscribers.add((int) cookie, callback);
                break;
            case CameraFrame::REPROCESS_INPUT_FRAME:
                mVideoInSubscribers.add((int) cookie, callback);
                break;
            default:
                CAMHAL_LOGEA("Frame message type id=0x%x subscription no supported yet!", frameMsg);
                break;
            }
        }

    if ( eventMsg != 0)
        {
        CAMHAL_LOGVB("Event message type id=0x%x subscription request", eventMsg);
        if ( CameraHalEvent::ALL_EVENTS == eventMsg )
            {
            mFocusSubscribers.add((int) cookie, eventCb);
            mShutterSubscribers.add((int) cookie, eventCb);
            mZoomSubscribers.add((int) cookie, eventCb);
            mMetadataSubscribers.add((int) cookie, eventCb);
            }
        else
            {
            CAMHAL_LOGEA("Event message type id=0x%x subscription no supported yet!", eventMsg);
            }
        }

    LOG_FUNCTION_NAME_EXIT;
}

void BaseCameraAdapter::disableMsgType(int32_t msgs, void* cookie)
{
    android::AutoMutex lock(mSubscriberLock);

    LOG_FUNCTION_NAME;

    int32_t frameMsg = ((msgs >> MessageNotifier::FRAME_BIT_FIELD_POSITION) & EVENT_MASK);
    int32_t eventMsg = ((msgs >> MessageNotifier::EVENT_BIT_FIELD_POSITION) & EVENT_MASK);

    if ( frameMsg != 0 )
        {
        CAMHAL_LOGVB("Frame message type id=0x%x remove subscription request", frameMsg);
        switch ( frameMsg )
            {
            case CameraFrame::PREVIEW_FRAME_SYNC:
                mFrameSubscribers.removeItem((int) cookie);
                break;
            case CameraFrame::FRAME_DATA_SYNC:
                mFrameDataSubscribers.removeItem((int) cookie);
                break;
            case CameraFrame::SNAPSHOT_FRAME:
                mSnapshotSubscribers.removeItem((int) cookie);
                break;
            case CameraFrame::IMAGE_FRAME:
                mImageSubscribers.removeItem((int) cookie);
                break;
            case CameraFrame::RAW_FRAME:
                mRawSubscribers.removeItem((int) cookie);
                break;
            case CameraFrame::VIDEO_FRAME_SYNC:
                mVideoSubscribers.removeItem((int) cookie);
                break;
            case CameraFrame::REPROCESS_INPUT_FRAME:
                mVideoInSubscribers.removeItem((int) cookie);
                break;
            case CameraFrame::ALL_FRAMES:
                mFrameSubscribers.removeItem((int) cookie);
                mFrameDataSubscribers.removeItem((int) cookie);
                mSnapshotSubscribers.removeItem((int) cookie);
                mImageSubscribers.removeItem((int) cookie);
                mRawSubscribers.removeItem((int) cookie);
                mVideoSubscribers.removeItem((int) cookie);
                mVideoInSubscribers.removeItem((int) cookie);
                break;
            default:
                CAMHAL_LOGEA("Frame message type id=0x%x subscription remove not supported yet!", frameMsg);
                break;
            }
        }

    if ( eventMsg != 0 )
        {
        CAMHAL_LOGVB("Event message type id=0x%x remove subscription request", eventMsg);
        if ( CameraHalEvent::ALL_EVENTS == eventMsg)
            {
            //TODO: Process case by case
            mFocusSubscribers.removeItem((int) cookie);
            mShutterSubscribers.removeItem((int) cookie);
            mZoomSubscribers.removeItem((int) cookie);
            mMetadataSubscribers.removeItem((int) cookie);
            }
        else
            {
            CAMHAL_LOGEA("Event message type id=0x%x subscription remove not supported yet!", eventMsg);
            }
        }

    LOG_FUNCTION_NAME_EXIT;
}

void BaseCameraAdapter::addFramePointers(CameraBuffer *frameBuf, void *buf)
{
  unsigned int *pBuf = (unsigned int *)buf;
  android::AutoMutex lock(mSubscriberLock);

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
  android::AutoMutex lock(mSubscriberLock);

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

void BaseCameraAdapter::returnFrame(CameraBuffer * frameBuf, CameraFrame::FrameType frameType)
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
        android::AutoMutex lock(mReturnFrameLock);

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
#ifdef CAMERAHAL_DEBUG
            if((mBuffersWithDucati.indexOfKey((int)camera_buffer_get_omx_ptr(frameBuf)) >= 0) &&
               ((CameraFrame::PREVIEW_FRAME_SYNC == frameType) ||
                 (CameraFrame::SNAPSHOT_FRAME == frameType)))
                {
                CAMHAL_LOGE("Buffer already with Ducati!! 0x%x", frameBuf);
                for(int i=0;i<mBuffersWithDucati.size();i++) CAMHAL_LOGE("0x%x", mBuffersWithDucati.keyAt(i));
                }
            mBuffersWithDucati.add((int)camera_buffer_get_omx_ptr(frameBuf),1);
#endif
            res = fillThisBuffer(frameBuf, frameType);
            }
        }

}

status_t BaseCameraAdapter::sendCommand(CameraCommands operation, int value1, int value2, int value3, int value4) {
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
                    android::AutoMutex lock(mPreviewBufferLock);
                    mPreviewBuffers = desc->mBuffers;
                    mPreviewBuffersLength = desc->mLength;
                    mPreviewBuffersAvailable.clear();
                    mSnapshotBuffersAvailable.clear();
                    for ( uint32_t i = 0 ; i < desc->mMaxQueueable ; i++ )
                        {
                        mPreviewBuffersAvailable.add(&mPreviewBuffers[i], 0);
                        }
                    // initial ref count for undeqeueued buffers is 1 since buffer provider
                    // is still holding on to it
                    for ( uint32_t i = desc->mMaxQueueable ; i < desc->mCount ; i++ )
                        {
                        mPreviewBuffersAvailable.add(&mPreviewBuffers[i], 1);
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
                        android::AutoMutex lock(mPreviewDataBufferLock);
                        mPreviewDataBuffers = desc->mBuffers;
                        mPreviewDataBuffersLength = desc->mLength;
                        mPreviewDataBuffersAvailable.clear();
                        for ( uint32_t i = 0 ; i < desc->mMaxQueueable ; i++ )
                            {
                            mPreviewDataBuffersAvailable.add(&mPreviewDataBuffers[i], 0);
                            }
                        // initial ref count for undeqeueued buffers is 1 since buffer provider
                        // is still holding on to it
                        for ( uint32_t i = desc->mMaxQueueable ; i < desc->mCount ; i++ )
                            {
                            mPreviewDataBuffersAvailable.add(&mPreviewDataBuffers[i], 1);
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
                    android::AutoMutex lock(mCaptureBufferLock);
                    mCaptureBuffers = desc->mBuffers;
                    mCaptureBuffersLength = desc->mLength;
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

#ifdef OMAP_ENHANCEMENT_CPCAM
        case CameraAdapter::CAMERA_USE_BUFFERS_REPROCESS:
            CAMHAL_LOGDA("Use buffers for reprocessing");
            desc = (BuffersDescriptor *) value1;

            if (NULL == desc) {
                CAMHAL_LOGEA("Invalid capture buffers!");
                return -EINVAL;
            }

            if (ret == NO_ERROR) {
                ret = setState(operation);
            }

            if (ret == NO_ERROR) {
                android::AutoMutex lock(mVideoInBufferLock);
                mVideoInBuffers = desc->mBuffers;
                mVideoInBuffersAvailable.clear();
                for (uint32_t i = 0 ; i < desc->mMaxQueueable ; i++) {
                    mVideoInBuffersAvailable.add(&mVideoInBuffers[i], 0);
                }
                // initial ref count for undeqeueued buffers is 1 since buffer provider
                // is still holding on to it
                for ( uint32_t i = desc->mMaxQueueable ; i < desc->mCount ; i++ ) {
                    mVideoInBuffersAvailable.add(&mVideoInBuffers[i], 1);
                }
                ret = useBuffers(CameraAdapter::CAMERA_REPROCESS,
                                 desc->mBuffers,
                                 desc->mCount,
                                 desc->mLength,
                                 desc->mMaxQueueable);
            }

            if ( ret == NO_ERROR ) {
                ret = commitState();
            } else {
                ret |= rollbackState();
            }

            break;
#endif

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
                     ret = getPictureBufferSize(*frame, value2);
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

         case CameraAdapter::CAMERA_USE_BUFFERS_VIDEO_CAPTURE:

             CAMHAL_LOGDA("Use buffers for video (RAW + JPEG) capture");
             desc = ( BuffersDescriptor * ) value1;

             if ( NULL == desc ) {
                 CAMHAL_LOGEA("Invalid capture buffers!");
                 return -EINVAL;
             }

             if ( ret == NO_ERROR ) {
                 ret = setState(operation);
             }

             if ( ret == NO_ERROR ) {
                 android::AutoMutex lock(mVideoBufferLock);
                 mVideoBuffers = desc->mBuffers;
                 mVideoBuffersLength = desc->mLength;
                 mVideoBuffersAvailable.clear();
                 for ( uint32_t i = 0 ; i < desc->mMaxQueueable ; i++ ) {
                     mVideoBuffersAvailable.add(&mVideoBuffers[i], 1);
                 }
                 // initial ref count for undeqeueued buffers is 1 since buffer provider
                 // is still holding on to it
                 for ( uint32_t i = desc->mMaxQueueable ; i < desc->mCount ; i++ ) {
                     mVideoBuffersAvailable.add(&mVideoBuffers[i], 1);
                 }
             }

             if ( NULL != desc ) {
                 ret = useBuffers(CameraAdapter::CAMERA_VIDEO,
                         desc->mBuffers,
                         desc->mCount,
                         desc->mLength,
                         desc->mMaxQueueable);
             }

             if ( ret == NO_ERROR ) {
                 ret = commitState();
             } else {
                 ret |= rollbackState();
             }

             break;

        case CameraAdapter::CAMERA_SWITCH_TO_EXECUTING:
            ret = switchToExecuting();
            break;

#ifdef OMAP_ENHANCEMENT_VTC
        case CameraAdapter::CAMERA_SETUP_TUNNEL:
            ret = setupTunnel(value1, value2, value3, value4);
            break;

        case CameraAdapter::CAMERA_DESTROY_TUNNEL:
            ret = destroyTunnel();
            break;
#endif

        case CameraAdapter::CAMERA_PREVIEW_INITIALIZATION:
            ret = cameraPreviewInitialization();
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

        CAMHAL_LOGD("Sending shutter callback");

        eventCb ( &shutterEvent );
    }

    shutterEvent.mEventData.clear();

    LOG_FUNCTION_NAME_EXIT;

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

status_t BaseCameraAdapter::notifyMetadataSubscribers(android::sp<CameraMetadataResult> &meta)
{
    event_callback eventCb;
    CameraHalEvent metaEvent;
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    if ( mMetadataSubscribers.size() == 0 ) {
        CAMHAL_LOGDA("No preview metadata subscribers!");
        return NO_INIT;
    }

    metaEvent.mEventData = new CameraHalEvent::CameraHalEventData();
    if ( NULL == metaEvent.mEventData.get() ) {
        return -ENOMEM;
    }

    metaEvent.mEventType = CameraHalEvent::EVENT_METADATA;
    metaEvent.mEventData->metadataEvent = meta;

    for (unsigned int i = 0 ; i < mMetadataSubscribers.size(); i++ ) {
        metaEvent.mCookie = (void *) mMetadataSubscribers.keyAt(i);
        eventCb = (event_callback) mMetadataSubscribers.valueAt(i);

        eventCb ( &metaEvent );
    }

    metaEvent.mEventData.clear();

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
            ret = __sendFrameToSubscribers(frame, &mSnapshotSubscribers, CameraFrame::SNAPSHOT_FRAME);
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
        case CameraFrame::REPROCESS_INPUT_FRAME:
          {
            ret = __sendFrameToSubscribers(frame, &mVideoInSubscribers, CameraFrame::REPROCESS_INPUT_FRAME);
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
                                                     android::KeyedVector<int, frame_callback> *subscribers,
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
          frame->mYuv[1] = frame->mYuv[0] + (frame->mLength + frame->mOffset)*2/3;
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

int BaseCameraAdapter::setInitFrameRefCount(CameraBuffer * buf, unsigned int mask)
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
          setFrameRefCount(buf, CameraFrame::SNAPSHOT_FRAME, mSnapshotSubscribers.size());
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
      case CameraFrame::REPROCESS_INPUT_FRAME:
        {
          setFrameRefCount(buf,CameraFrame::REPROCESS_INPUT_FRAME, mVideoInSubscribers.size());
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

int BaseCameraAdapter::getFrameRefCount(CameraBuffer * frameBuf, CameraFrame::FrameType frameType)
{
    int res = -1;

    LOG_FUNCTION_NAME;

    switch ( frameType )
        {
        case CameraFrame::IMAGE_FRAME:
        case CameraFrame::RAW_FRAME:
                {
                android::AutoMutex lock(mCaptureBufferLock);
                res = mCaptureBuffersAvailable.valueFor(frameBuf );
                }
            break;
        case CameraFrame::SNAPSHOT_FRAME:
                {
                android::AutoMutex lock(mSnapshotBufferLock);
                res = mSnapshotBuffersAvailable.valueFor( ( unsigned int ) frameBuf );
                }
            break;
        case CameraFrame::PREVIEW_FRAME_SYNC:
                {
                android::AutoMutex lock(mPreviewBufferLock);
                res = mPreviewBuffersAvailable.valueFor(frameBuf );
                }
            break;
        case CameraFrame::FRAME_DATA_SYNC:
                {
                android::AutoMutex lock(mPreviewDataBufferLock);
                res = mPreviewDataBuffersAvailable.valueFor(frameBuf );
                }
            break;
        case CameraFrame::VIDEO_FRAME_SYNC:
                {
                android::AutoMutex lock(mVideoBufferLock);
                res = mVideoBuffersAvailable.valueFor(frameBuf );
                }
            break;
        case CameraFrame::REPROCESS_INPUT_FRAME: {
            android::AutoMutex lock(mVideoInBufferLock);
            res = mVideoInBuffersAvailable.valueFor(frameBuf );
        }
            break;
        default:
            break;
        };

    LOG_FUNCTION_NAME_EXIT;

    return res;
}

void BaseCameraAdapter::setFrameRefCount(CameraBuffer * frameBuf, CameraFrame::FrameType frameType, int refCount)
{

    LOG_FUNCTION_NAME;

    switch ( frameType )
        {
        case CameraFrame::IMAGE_FRAME:
        case CameraFrame::RAW_FRAME:
                {
                android::AutoMutex lock(mCaptureBufferLock);
                mCaptureBuffersAvailable.replaceValueFor(frameBuf, refCount);
                }
            break;
        case CameraFrame::SNAPSHOT_FRAME:
                {
                android::AutoMutex lock(mSnapshotBufferLock);
                mSnapshotBuffersAvailable.replaceValueFor(  ( unsigned int ) frameBuf, refCount);
                }
            break;
        case CameraFrame::PREVIEW_FRAME_SYNC:
                {
                android::AutoMutex lock(mPreviewBufferLock);
                mPreviewBuffersAvailable.replaceValueFor(frameBuf, refCount);
                }
            break;
        case CameraFrame::FRAME_DATA_SYNC:
                {
                android::AutoMutex lock(mPreviewDataBufferLock);
                mPreviewDataBuffersAvailable.replaceValueFor(frameBuf, refCount);
                }
            break;
        case CameraFrame::VIDEO_FRAME_SYNC:
                {
                android::AutoMutex lock(mVideoBufferLock);
                mVideoBuffersAvailable.replaceValueFor(frameBuf, refCount);
                }
            break;
        case CameraFrame::REPROCESS_INPUT_FRAME: {
            android::AutoMutex lock(mVideoInBufferLock);
            mVideoInBuffersAvailable.replaceValueFor(frameBuf, refCount);
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

    android::AutoMutex lock(mVideoBufferLock);

    //If the capture is already ongoing, return from here.
    if ( mRecording )
        {
        ret = NO_INIT;
        }


    if ( NO_ERROR == ret )
        {

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
            CameraBuffer *frameBuf = mVideoBuffersAvailable.keyAt(i);
            if( getFrameRefCount(frameBuf,  CameraFrame::VIDEO_FRAME_SYNC) > 0)
                {
                returnFrame(frameBuf, CameraFrame::VIDEO_FRAME_SYNC);
                }
            }

        mVideoBuffersAvailable.clear();

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

status_t BaseCameraAdapter::useBuffers(CameraMode mode, CameraBuffer* bufArr, int num, size_t length, unsigned int queueable)
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t BaseCameraAdapter::fillThisBuffer(CameraBuffer * frameBuf, CameraFrame::FrameType frameType)
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

status_t BaseCameraAdapter::getPictureBufferSize(CameraFrame &frame, size_t bufferCount)
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

const char* BaseCameraAdapter::getLUTvalue_translateHAL(int Value, LUTtypeHAL LUT) {
    int LUTsize = LUT.size;
    for(int i = 0; i < LUTsize; i++)
        if( LUT.Table[i].halDefinition == Value )
            return LUT.Table[i].userDefinition;

    return NULL;
}

status_t BaseCameraAdapter::setupTunnel(uint32_t SliceHeight, uint32_t EncoderHandle, uint32_t width, uint32_t height) {
  status_t ret = NO_ERROR;
  LOG_FUNCTION_NAME;
  LOG_FUNCTION_NAME_EXIT;
  return ret;
}

status_t BaseCameraAdapter::destroyTunnel() {
  status_t ret = NO_ERROR;
  LOG_FUNCTION_NAME;
  LOG_FUNCTION_NAME_EXIT;
  return ret;
}

status_t BaseCameraAdapter::cameraPreviewInitialization() {
  status_t ret = NO_ERROR;
  LOG_FUNCTION_NAME;
  LOG_FUNCTION_NAME_EXIT;
  return ret;
}

status_t BaseCameraAdapter::setState(CameraCommands operation)
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    const char *printState = getLUTvalue_translateHAL(operation, CamCommandsLUT);

    mLock.lock();

    switch ( mAdapterState )
        {

        case INTIALIZED_STATE:

            switch ( operation )
                {

                case CAMERA_USE_BUFFERS_PREVIEW:
                    CAMHAL_LOGDB("Adapter state switch INTIALIZED_STATE->LOADED_PREVIEW_STATE event = %s",
                            printState);
                    mNextState = LOADED_PREVIEW_STATE;
                    break;

                //These events don't change the current state
                case CAMERA_QUERY_RESOLUTION_PREVIEW:
                case CAMERA_QUERY_BUFFER_SIZE_PREVIEW_DATA:
                    CAMHAL_LOGDB("Adapter state switch INTIALIZED_STATE->INTIALIZED_STATE event  = %s",
                            printState);
                    mNextState = INTIALIZED_STATE;
                    break;
                case CAMERA_STOP_BRACKET_CAPTURE:
                case CAMERA_STOP_IMAGE_CAPTURE:
                    ret = INVALID_OPERATION;
                    break;
                case CAMERA_CANCEL_AUTOFOCUS:
                    ret = INVALID_OPERATION;
                    break;

                default:
                    CAMHAL_LOGEB("Adapter state switch INTIALIZED_STATE Invalid Op! event  = %s",
                            printState);
                    ret = INVALID_OPERATION;
                    break;

                }

            break;

        case LOADED_PREVIEW_STATE:

            switch ( operation )
                {

                case CAMERA_START_PREVIEW:
                    CAMHAL_LOGDB("Adapter state switch LOADED_PREVIEW_STATE->PREVIEW_STATE event = %s",
                            printState);
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
                    CAMHAL_LOGDB("Adapter state switch LOADED_PREVIEW_STATE->LOADED_PREVIEW_STATE event = %s",
                            printState);
                    mNextState = LOADED_PREVIEW_STATE;
                    break;

                default:
                    CAMHAL_LOGDB("Adapter state switch LOADED_PREVIEW Invalid Op! event = %s",
                            printState);
                    ret = INVALID_OPERATION;
                    break;

                }

            break;

        case PREVIEW_STATE:

            switch ( operation )
                {

                case CAMERA_STOP_PREVIEW:
                    CAMHAL_LOGDB("Adapter state switch PREVIEW_STATE->INTIALIZED_STATE event = %s",
                            printState);
                    mNextState = INTIALIZED_STATE;
                    break;

                case CAMERA_PERFORM_AUTOFOCUS:
                    CAMHAL_LOGDB("Adapter state switch PREVIEW_STATE->AF_STATE event = %s",
                            printState);
                    mNextState = AF_STATE;
                    break;

                case CAMERA_START_SMOOTH_ZOOM:
                    CAMHAL_LOGDB("Adapter state switch PREVIEW_STATE->ZOOM_STATE event = %s",
                            printState);
                    mNextState = ZOOM_STATE;
                    break;

                case CAMERA_USE_BUFFERS_IMAGE_CAPTURE:
                    CAMHAL_LOGDB("Adapter state switch PREVIEW_STATE->LOADED_CAPTURE_STATE event = %s",
                            printState);
                    mNextState = LOADED_CAPTURE_STATE;
                    break;

#ifdef OMAP_ENHANCEMENT_CPCAM
                case CAMERA_USE_BUFFERS_REPROCESS:
                    CAMHAL_LOGDB("Adapter state switch PREVIEW_STATE->LOADED_REPROCESS_STATE event = %s",
                                 printState);
                    mNextState = LOADED_REPROCESS_STATE;
                    break;
#endif

                case CAMERA_START_VIDEO:
                    CAMHAL_LOGDB("Adapter state switch PREVIEW_STATE->VIDEO_STATE event = %s",
                            printState);
                    mNextState = VIDEO_STATE;
                    break;

                case CAMERA_CANCEL_AUTOFOCUS:
                case CAMERA_QUERY_BUFFER_SIZE_IMAGE_CAPTURE:
                case CAMERA_STOP_SMOOTH_ZOOM:
                    CAMHAL_LOGDB("Adapter state switch PREVIEW_ACTIVE->PREVIEW_ACTIVE event = %s",
                            printState);
                    mNextState = PREVIEW_STATE;
                    break;

                case CAMERA_STOP_IMAGE_CAPTURE:
                case CAMERA_STOP_BRACKET_CAPTURE:
                    CAMHAL_LOGDB("Adapter state switch PREVIEW_ACTIVE->PREVIEW_ACTIVE event = %s",
                                 printState);
                    ret = INVALID_OPERATION;
                    break;

                default:
                    CAMHAL_LOGEB("Adapter state switch PREVIEW_ACTIVE Invalid Op! event = %s",
                            printState);
                    ret = INVALID_OPERATION;
                    break;

                }

            break;

#ifdef OMAP_ENHANCEMENT_CPCAM
        case LOADED_REPROCESS_STATE:
            switch (operation) {
                case CAMERA_USE_BUFFERS_IMAGE_CAPTURE:
                    CAMHAL_LOGDB("Adapter state switch LOADED_REPROCESS_STATE->LOADED_REPROCESS_CAPTURE_STATE event = %s",
                                 printState);
                    mNextState = LOADED_REPROCESS_CAPTURE_STATE;
                    break;
                case CAMERA_QUERY_BUFFER_SIZE_IMAGE_CAPTURE:
                    CAMHAL_LOGDB("Adapter state switch LOADED_REPROCESS_STATE->LOADED_REPROCESS_STATE event = %s",
                                 printState);
                    mNextState = LOADED_REPROCESS_STATE;
                    break;
                default:
                    CAMHAL_LOGEB("Adapter state switch LOADED_REPROCESS_STATE Invalid Op! event = %s",
                                 printState);
                    ret = INVALID_OPERATION;
                    break;
                }

            break;

        case LOADED_REPROCESS_CAPTURE_STATE:
            switch (operation) {
                case CAMERA_START_IMAGE_CAPTURE:
                    CAMHAL_LOGDB("Adapter state switch LOADED_REPROCESS_CAPTURE_STATE->REPROCESS_STATE event = %s",
                                 printState);
                    mNextState = REPROCESS_STATE;
                    break;
                default:
                    CAMHAL_LOGEB("Adapter state switch LOADED_REPROCESS_CAPTURE_STATE Invalid Op! event = %s",
                                 printState);
                    ret = INVALID_OPERATION;
                    break;
            }
            break;
#endif

        case LOADED_CAPTURE_STATE:

            switch ( operation )
                {

                case CAMERA_START_IMAGE_CAPTURE:
                    CAMHAL_LOGDB("Adapter state switch LOADED_CAPTURE_STATE->CAPTURE_STATE event = %s",
                            printState);
                    mNextState = CAPTURE_STATE;
                    break;

                case CAMERA_START_BRACKET_CAPTURE:
                    CAMHAL_LOGDB("Adapter state switch LOADED_CAPTURE_STATE->BRACKETING_STATE event = %s",
                            printState);
                    mNextState = BRACKETING_STATE;
                    break;

                case CAMERA_USE_BUFFERS_VIDEO_CAPTURE:
                    //Hadnle this state for raw capture path.
                    //Just need to keep the same state.
                    //The next CAMERA_START_IMAGE_CAPTURE command will assign the mNextState.
                    CAMHAL_LOGDB("Adapter state switch LOADED_CAPTURE_STATE->LOADED_CAPTURE_STATE event = %s",
                            printState);
                    break;

                default:
                    CAMHAL_LOGEB("Adapter state switch LOADED_CAPTURE_STATE Invalid Op! event = %s",
                            printState);
                    ret = INVALID_OPERATION;
                    break;

                }

            break;

        case CAPTURE_STATE:

            switch ( operation )
                {
                case CAMERA_STOP_IMAGE_CAPTURE:
                case CAMERA_STOP_BRACKET_CAPTURE:
                    CAMHAL_LOGDB("Adapter state switch CAPTURE_STATE->PREVIEW_STATE event = %s",
                            printState);
                    mNextState = PREVIEW_STATE;
                    break;

                case CAMERA_QUERY_BUFFER_SIZE_IMAGE_CAPTURE:
                case CAMERA_START_IMAGE_CAPTURE:
                     CAMHAL_LOGDB("Adapter state switch CAPTURE_STATE->CAPTURE_STATE event = %s",
                                 printState);
                    mNextState = CAPTURE_STATE;
                    break;

#ifdef OMAP_ENHANCEMENT_CPCAM
                case CAMERA_USE_BUFFERS_REPROCESS:
                    CAMHAL_LOGDB("Adapter state switch CAPTURE_STATE->->LOADED_REPROCESS_STATE event = %s",
                                 printState);
                    mNextState = LOADED_REPROCESS_STATE;
                    break;
#endif

                default:
                    CAMHAL_LOGEB("Adapter state switch CAPTURE_STATE Invalid Op! event = %s",
                            printState);
                    ret = INVALID_OPERATION;
                    break;

                }

            break;

        case BRACKETING_STATE:

            switch ( operation )
                {

                case CAMERA_STOP_IMAGE_CAPTURE:
                case CAMERA_STOP_BRACKET_CAPTURE:
                    CAMHAL_LOGDB("Adapter state switch BRACKETING_STATE->PREVIEW_STATE event = %s",
                            printState);
                    mNextState = PREVIEW_STATE;
                    break;

                case CAMERA_START_IMAGE_CAPTURE:
                    CAMHAL_LOGDB("Adapter state switch BRACKETING_STATE->CAPTURE_STATE event = %s",
                            printState);
                    mNextState = CAPTURE_STATE;
                    break;

                default:
                    CAMHAL_LOGEB("Adapter state switch BRACKETING_STATE Invalid Op! event = %s",
                            printState);
                    ret = INVALID_OPERATION;
                    break;

                }

            break;

        case AF_STATE:

            switch ( operation )
                {

                case CAMERA_CANCEL_AUTOFOCUS:
                    CAMHAL_LOGDB("Adapter state switch AF_STATE->PREVIEW_STATE event = %s",
                            printState);
                    mNextState = PREVIEW_STATE;
                    break;

                case CAMERA_START_SMOOTH_ZOOM:
                    CAMHAL_LOGDB("Adapter state switch AF_STATE->AF_ZOOM_STATE event = %s",
                            printState);
                    mNextState = AF_ZOOM_STATE;
                    break;

                default:
                    CAMHAL_LOGEB("Adapter state switch AF_STATE Invalid Op! event = %s",
                            printState);
                    ret = INVALID_OPERATION;
                    break;

                }

            break;

        case ZOOM_STATE:

            switch ( operation )
                {

                case CAMERA_CANCEL_AUTOFOCUS:
                    CAMHAL_LOGDB("Adapter state switch AF_STATE->PREVIEW_STATE event = %s",
                            printState);
                    mNextState = ZOOM_STATE;
                    break;

                case CAMERA_STOP_SMOOTH_ZOOM:
                    CAMHAL_LOGDB("Adapter state switch ZOOM_STATE->PREVIEW_STATE event = %s",
                            printState);
                    mNextState = PREVIEW_STATE;
                    break;

                case CAMERA_PERFORM_AUTOFOCUS:
                    CAMHAL_LOGDB("Adapter state switch ZOOM_STATE->AF_ZOOM_STATE event = %s",
                            printState);
                    mNextState = AF_ZOOM_STATE;
                    break;

                case CAMERA_START_VIDEO:
                    CAMHAL_LOGDB("Adapter state switch ZOOM_STATE->VIDEO_ZOOM_STATE event = %s",
                            printState);
                    mNextState = VIDEO_ZOOM_STATE;
                    break;

                default:
                    CAMHAL_LOGEB("Adapter state switch ZOOM_STATE Invalid Op! event = %s",
                            printState);
                    ret = INVALID_OPERATION;
                    break;

                }

            break;

        case VIDEO_STATE:

            switch ( operation )
                {

                case CAMERA_STOP_VIDEO:
                    CAMHAL_LOGDB("Adapter state switch VIDEO_STATE->PREVIEW_STATE event = %s",
                            printState);
                    mNextState = PREVIEW_STATE;
                    break;

                case CAMERA_PERFORM_AUTOFOCUS:
                    CAMHAL_LOGDB("Adapter state switch VIDEO_STATE->VIDEO_AF_STATE event = %s",
                            printState);
                    mNextState = VIDEO_AF_STATE;
                    break;

                case CAMERA_START_SMOOTH_ZOOM:
                    CAMHAL_LOGDB("Adapter state switch VIDEO_STATE->VIDEO_ZOOM_STATE event = %s",
                            printState);
                    mNextState = VIDEO_ZOOM_STATE;
                    break;

                case CAMERA_USE_BUFFERS_IMAGE_CAPTURE:
                    CAMHAL_LOGDB("Adapter state switch VIDEO_STATE->VIDEO_LOADED_CAPTURE_STATE event = %s",
                            printState);
                    mNextState = VIDEO_LOADED_CAPTURE_STATE;
                    break;

                case CAMERA_QUERY_BUFFER_SIZE_IMAGE_CAPTURE:
                    CAMHAL_LOGDB("Adapter state switch VIDEO_STATE->VIDEO_STATE event = %s",
                            printState);
                    mNextState = VIDEO_STATE;
                    break;

                default:
                    CAMHAL_LOGEB("Adapter state switch VIDEO_STATE Invalid Op! event = %s",
                            printState);
                    ret = INVALID_OPERATION;
                    break;

                }

            break;

        case VIDEO_AF_STATE:

            switch ( operation )
                {

                case CAMERA_CANCEL_AUTOFOCUS:
                    CAMHAL_LOGDB("Adapter state switch VIDEO_AF_STATE->VIDEO_STATE event = %s",
                            printState);
                    mNextState = VIDEO_STATE;
                    break;

                default:
                    CAMHAL_LOGEB("Adapter state switch VIDEO_AF_STATE Invalid Op! event = %s",
                            printState);
                    ret = INVALID_OPERATION;
                    break;

                }

            break;

        case VIDEO_LOADED_CAPTURE_STATE:

            switch ( operation )
                {

                case CAMERA_START_IMAGE_CAPTURE:
                    CAMHAL_LOGDB("Adapter state switch LOADED_CAPTURE_STATE->CAPTURE_STATE event = %s",
                            printState);
                    mNextState = VIDEO_CAPTURE_STATE;
                    break;

                default:
                    CAMHAL_LOGEB("Adapter state switch LOADED_CAPTURE_STATE Invalid Op! event = %s",
                            printState);
                    ret = INVALID_OPERATION;
                    break;

                }

            break;

        case VIDEO_CAPTURE_STATE:

            switch ( operation )
                {
                case CAMERA_STOP_IMAGE_CAPTURE:
                    CAMHAL_LOGDB("Adapter state switch CAPTURE_STATE->PREVIEW_STATE event = %s",
                            printState);
                    mNextState = VIDEO_STATE;
                    break;

                default:
                    CAMHAL_LOGEB("Adapter state switch CAPTURE_STATE Invalid Op! event = %s",
                            printState);
                    ret = INVALID_OPERATION;
                    break;

                }

            break;

        case AF_ZOOM_STATE:

            switch ( operation )
                {

                case CAMERA_STOP_SMOOTH_ZOOM:
                    CAMHAL_LOGDB("Adapter state switch AF_ZOOM_STATE->AF_STATE event = %s",
                            printState);
                    mNextState = AF_STATE;
                    break;

                case CAMERA_CANCEL_AUTOFOCUS:
                    CAMHAL_LOGDB("Adapter state switch AF_ZOOM_STATE->ZOOM_STATE event = %s",
                            printState);
                    mNextState = ZOOM_STATE;
                    break;

                default:
                    CAMHAL_LOGEB("Adapter state switch AF_ZOOM_STATE Invalid Op! event = %s",
                            printState);
                    ret = INVALID_OPERATION;
                    break;

                }

            break;

        case VIDEO_ZOOM_STATE:

            switch ( operation )
                {

                case CAMERA_STOP_SMOOTH_ZOOM:
                    CAMHAL_LOGDB("Adapter state switch VIDEO_ZOOM_STATE->VIDEO_STATE event = %s",
                            printState);
                    mNextState = VIDEO_STATE;
                    break;

                case CAMERA_STOP_VIDEO:
                    CAMHAL_LOGDB("Adapter state switch VIDEO_ZOOM_STATE->ZOOM_STATE event = %s",
                            printState);
                    mNextState = ZOOM_STATE;
                    break;

                default:
                    CAMHAL_LOGEB("Adapter state switch VIDEO_ZOOM_STATE Invalid Op! event = %s",
                            printState);
                    ret = INVALID_OPERATION;
                    break;

                }

            break;

        case BRACKETING_ZOOM_STATE:

            switch ( operation )
                {

                case CAMERA_STOP_SMOOTH_ZOOM:
                    CAMHAL_LOGDB("Adapter state switch BRACKETING_ZOOM_STATE->BRACKETING_STATE event = %s",
                            printState);
                    mNextState = BRACKETING_STATE;
                    break;

                default:
                    CAMHAL_LOGEB("Adapter state switch BRACKETING_ZOOM_STATE Invalid Op! event = %s",
                            printState);
                    ret = INVALID_OPERATION;
                    break;

                }

            break;

#ifdef OMAP_ENHANCEMENT_CPCAM
        case REPROCESS_STATE:
            switch (operation) {
                case CAMERA_STOP_IMAGE_CAPTURE:
                    CAMHAL_LOGDB("Adapter state switch REPROCESS_STATE->PREVIEW_STATE event = %s",
                                 printState);
                    mNextState = PREVIEW_STATE;
                    break;
                case CAMERA_QUERY_BUFFER_SIZE_IMAGE_CAPTURE:
                case CAMERA_START_IMAGE_CAPTURE:
                     CAMHAL_LOGDB("Adapter state switch REPROCESS_STATE->REPROCESS_STATE event = %s",
                                 printState);
                    mNextState = REPROCESS_STATE;
                    break;
                case CAMERA_USE_BUFFERS_REPROCESS:
                     CAMHAL_LOGDB("Adapter state switch REPROCESS_STATE->REPROCESS_STATE event = %s",
                                 printState);
                    mNextState = LOADED_REPROCESS_STATE;
                    break;

                case CAMERA_USE_BUFFERS_IMAGE_CAPTURE:
                    CAMHAL_LOGDB("Adapter state switch REPROCESS_STATE->LOADED_CAPTURE_STATE event = %s",
                            printState);
                    mNextState = LOADED_CAPTURE_STATE;
                    break;
                default:
                    CAMHAL_LOGEB("Adapter state switch REPROCESS_STATE Invalid Op! event = %s",
                                 printState);
                    ret = INVALID_OPERATION;
                    break;

                }

            break;
#endif


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
#ifdef OMAP_ENHANCEMENT_CPCAM
        case REPROCESS_STATE:
#endif
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

    android::AutoMutex lock(mLock);

    LOG_FUNCTION_NAME_EXIT;

    return mAdapterState;
}

CameraAdapter::AdapterState BaseCameraAdapter::getNextState()
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    android::AutoMutex lock(mLock);

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

extern "C" status_t OMXCameraAdapter_Capabilities(
        CameraProperties::Properties * const properties_array,
        const int starting_camera, const int max_camera, int & supportedCameras);
extern "C" status_t V4LCameraAdapter_Capabilities(
        CameraProperties::Properties * const properties_array,
        const int starting_camera, const int max_camera, int & supportedCameras);

extern "C" status_t CameraAdapter_Capabilities(
        CameraProperties::Properties * const properties_array,
        const int starting_camera, const int max_camera, int & supportedCameras)
{

    status_t ret = NO_ERROR;
    status_t err = NO_ERROR;
    int num_cameras_supported = 0;

    LOG_FUNCTION_NAME;

    supportedCameras = 0;
#ifdef OMX_CAMERA_ADAPTER
    //Query OMX cameras
    err = OMXCameraAdapter_Capabilities( properties_array, starting_camera,
                                         max_camera, supportedCameras);
    if(err != NO_ERROR) {
        CAMHAL_LOGEA("error while getting OMXCameraAdapter capabilities");
        ret = UNKNOWN_ERROR;
    }
#endif
#ifdef V4L_CAMERA_ADAPTER
    //Query V4L cameras
    err = V4LCameraAdapter_Capabilities( properties_array, (const int) supportedCameras,
                                         max_camera, num_cameras_supported);
    if(err != NO_ERROR) {
        CAMHAL_LOGEA("error while getting V4LCameraAdapter capabilities");
        ret = UNKNOWN_ERROR;
    }
#endif

    supportedCameras += num_cameras_supported;
    CAMHAL_LOGEB("supportedCameras= %d\n", supportedCameras);
    LOG_FUNCTION_NAME_EXIT;
    return ret;
}

//-----------------------------------------------------------------------------

} // namespace Camera
} // namespace Ti

/*--------------------Camera Adapter Class ENDS here-----------------------------*/

