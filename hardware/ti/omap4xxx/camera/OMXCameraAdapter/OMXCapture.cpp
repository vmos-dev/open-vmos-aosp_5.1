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

/**
* @file OMXCapture.cpp
*
* This file contains functionality for handling image capture.
*
*/

#undef LOG_TAG

#define LOG_TAG "CameraHAL"

#include "CameraHal.h"
#include "OMXCameraAdapter.h"
#include "ErrorUtils.h"


namespace android {

status_t OMXCameraAdapter::setParametersCapture(const CameraParameters &params,
                                                BaseCameraAdapter::AdapterState state)
{
    status_t ret = NO_ERROR;
    const char *str = NULL;
    int w, h;
    OMX_COLOR_FORMATTYPE pixFormat;
    const char *valstr = NULL;
    int varint = 0;

    LOG_FUNCTION_NAME;

    OMXCameraPortParameters *cap;
    cap = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mImagePortIndex];

    params.getPictureSize(&w, &h);

    if ( ( w != ( int ) cap->mWidth ) ||
          ( h != ( int ) cap->mHeight ) )
        {
        mPendingCaptureSettings |= SetFormat;
        }

    cap->mWidth = w;
    cap->mHeight = h;
    //TODO: Support more pixelformats
    //cap->mStride = 2;

    CAMHAL_LOGVB("Image: cap.mWidth = %d", (int)cap->mWidth);
    CAMHAL_LOGVB("Image: cap.mHeight = %d", (int)cap->mHeight);

    if ((valstr = params.getPictureFormat()) != NULL) {
        if (strcmp(valstr, (const char *) CameraParameters::PIXEL_FORMAT_YUV422I) == 0) {
            CAMHAL_LOGDA("CbYCrY format selected");
            pixFormat = OMX_COLOR_FormatCbYCrY;
            mPictureFormatFromClient = CameraParameters::PIXEL_FORMAT_YUV422I;
        } else if(strcmp(valstr, (const char *) CameraParameters::PIXEL_FORMAT_YUV420SP) == 0) {
            CAMHAL_LOGDA("YUV420SP format selected");
            pixFormat = OMX_COLOR_FormatYUV420SemiPlanar;
            mPictureFormatFromClient = CameraParameters::PIXEL_FORMAT_YUV420SP;
        } else if(strcmp(valstr, (const char *) CameraParameters::PIXEL_FORMAT_RGB565) == 0) {
            CAMHAL_LOGDA("RGB565 format selected");
            pixFormat = OMX_COLOR_Format16bitRGB565;
            mPictureFormatFromClient = CameraParameters::PIXEL_FORMAT_RGB565;
        } else if (strcmp(valstr, (const char *) CameraParameters::PIXEL_FORMAT_JPEG) == 0) {
            CAMHAL_LOGDA("JPEG format selected");
            pixFormat = OMX_COLOR_FormatUnused;
            mCodingMode = CodingNone;
            mPictureFormatFromClient = CameraParameters::PIXEL_FORMAT_JPEG;
        } else if (strcmp(valstr, (const char *) TICameraParameters::PIXEL_FORMAT_JPS) == 0) {
            CAMHAL_LOGDA("JPS format selected");
            pixFormat = OMX_COLOR_FormatUnused;
            mCodingMode = CodingJPS;
            mPictureFormatFromClient = TICameraParameters::PIXEL_FORMAT_JPS;
        } else if (strcmp(valstr, (const char *) TICameraParameters::PIXEL_FORMAT_MPO) == 0) {
            CAMHAL_LOGDA("MPO format selected");
            pixFormat = OMX_COLOR_FormatUnused;
            mCodingMode = CodingMPO;
            mPictureFormatFromClient = TICameraParameters::PIXEL_FORMAT_MPO;
        } else if (strcmp(valstr, (const char *) TICameraParameters::PIXEL_FORMAT_RAW) == 0) {
            CAMHAL_LOGDA("RAW Picture format selected");
            pixFormat = OMX_COLOR_FormatRawBayer10bit;
            mPictureFormatFromClient = TICameraParameters::PIXEL_FORMAT_RAW;
        } else {
            CAMHAL_LOGEA("Invalid format, JPEG format selected as default");
            pixFormat = OMX_COLOR_FormatUnused;
            mPictureFormatFromClient = NULL;
        }
    } else {
        CAMHAL_LOGEA("Picture format is NULL, defaulting to JPEG");
        pixFormat = OMX_COLOR_FormatUnused;
        mPictureFormatFromClient = NULL;
    }

    // JPEG capture is not supported in video mode by OMX Camera
    // Set capture format to yuv422i...jpeg encode will
    // be done on A9
    valstr = params.get(TICameraParameters::KEY_CAP_MODE);
    if ( (valstr && !strcmp(valstr, (const char *) TICameraParameters::VIDEO_MODE)) &&
         (pixFormat == OMX_COLOR_FormatUnused) ) {
        CAMHAL_LOGDA("Capturing in video mode...selecting yuv422i");
        pixFormat = OMX_COLOR_FormatCbYCrY;
    }

    if ( pixFormat != cap->mColorFormat )
        {
        mPendingCaptureSettings |= SetFormat;
        cap->mColorFormat = pixFormat;
        }

#ifdef OMAP_ENHANCEMENT

    str = params.get(TICameraParameters::KEY_EXP_BRACKETING_RANGE);
    if ( NULL != str ) {
        parseExpRange(str, mExposureBracketingValues, EXP_BRACKET_RANGE, mExposureBracketingValidEntries);
    } else {
        // if bracketing was previously set...we set again before capturing to clear
        if (mExposureBracketingValidEntries) mPendingCaptureSettings |= SetExpBracket;
        mExposureBracketingValidEntries = 0;
    }

#endif

    varint = params.getInt(CameraParameters::KEY_ROTATION);
    if ( varint != -1 )
        {
        if ( ( unsigned int )  varint != mPictureRotation) {
            mPendingCaptureSettings |= SetRotation;
        }
        mPictureRotation = varint;
        }
    else
        {
        if (mPictureRotation) mPendingCaptureSettings |= SetRotation;
        mPictureRotation = 0;
        }

    CAMHAL_LOGVB("Picture Rotation set %d", mPictureRotation);

#ifdef OMAP_ENHANCEMENT

    // Read Sensor Orientation and set it based on perating mode

     varint = params.getInt(TICameraParameters::KEY_SENSOR_ORIENTATION);
     if (( varint != -1 ) && (mCapMode == OMXCameraAdapter::VIDEO_MODE))
        {
         mSensorOrientation = varint;
         if (mSensorOrientation == 270 ||mSensorOrientation==90)
             {
             CAMHAL_LOGEA(" Orientation is 270/90. So setting counter rotation  to Ducati");
             mSensorOrientation +=180;
             mSensorOrientation%=360;
              }
         }
     else
        {
         mSensorOrientation = 0;
        }

     CAMHAL_LOGVB("Sensor Orientation  set : %d", mSensorOrientation);

    varint = params.getInt(TICameraParameters::KEY_BURST);
    if ( varint >= 1 )
        {
        if (varint != mBurstFrames) {
            mPendingCaptureSettings |= SetExpBracket;
        }
        mBurstFrames = varint;
        }
    else
        {
        if (mBurstFrames != 1) mPendingCaptureSettings |= SetExpBracket;
        mBurstFrames = 1;
        }

    CAMHAL_LOGVB("Burst Frames set %d", mBurstFrames);

#endif

    varint = params.getInt(CameraParameters::KEY_JPEG_QUALITY);
    if ( ( varint >= MIN_JPEG_QUALITY ) &&
         ( varint  <= MAX_JPEG_QUALITY ) )
        {
        if ( ( unsigned int ) varint != mPictureQuality) {
            mPendingCaptureSettings |= SetQuality;
        }
        mPictureQuality = varint;
        }
    else
        {
        if (mPictureQuality != MAX_JPEG_QUALITY) mPendingCaptureSettings |= SetQuality;
        mPictureQuality = MAX_JPEG_QUALITY;
        }

    CAMHAL_LOGVB("Picture Quality set %d", mPictureQuality);

    varint = params.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH);
    if ( varint >= 0 )
        {
        if ( ( unsigned int ) varint != mThumbWidth) {
            mPendingCaptureSettings |= SetThumb;
        }
        mThumbWidth = varint;
        }
    else
        {
        if (mThumbWidth != DEFAULT_THUMB_WIDTH) mPendingCaptureSettings |= SetThumb;
        mThumbWidth = DEFAULT_THUMB_WIDTH;
        }


    CAMHAL_LOGVB("Picture Thumb width set %d", mThumbWidth);

    varint = params.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT);
    if ( varint >= 0 )
        {
        if ( ( unsigned int ) varint != mThumbHeight) {
            mPendingCaptureSettings |= SetThumb;
        }
        mThumbHeight = varint;
        }
    else
        {
        if (mThumbHeight != DEFAULT_THUMB_HEIGHT) mPendingCaptureSettings |= SetThumb;
        mThumbHeight = DEFAULT_THUMB_HEIGHT;
        }


    CAMHAL_LOGVB("Picture Thumb height set %d", mThumbHeight);

    varint = params.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_QUALITY);
    if ( ( varint >= MIN_JPEG_QUALITY ) &&
         ( varint <= MAX_JPEG_QUALITY ) )
        {
        if ( ( unsigned int ) varint != mThumbQuality) {
            mPendingCaptureSettings |= SetThumb;
        }
        mThumbQuality = varint;
        }
    else
        {
        if (mThumbQuality != MAX_JPEG_QUALITY) mPendingCaptureSettings |= SetThumb;
        mThumbQuality = MAX_JPEG_QUALITY;
        }

    CAMHAL_LOGDB("Thumbnail Quality set %d", mThumbQuality);

    if (mFirstTimeInit) {
        mPendingCaptureSettings = ECapturesettingsAll;
    }

    if (mPendingCaptureSettings) {
        disableImagePort();
        if ( NULL != mReleaseImageBuffersCallback ) {
            mReleaseImageBuffersCallback(mReleaseData);
        }
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::getPictureBufferSize(size_t &length, size_t bufferCount)
{
    status_t ret = NO_ERROR;
    OMXCameraPortParameters *imgCaptureData = NULL;
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    LOG_FUNCTION_NAME;

    if ( NO_ERROR == ret )
        {
        imgCaptureData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mImagePortIndex];

        imgCaptureData->mNumBufs = bufferCount;

        // check if image port is already configured...
        // if it already configured then we don't have to query again
        if (!mCaptureConfigured) {
            ret = setFormat(OMX_CAMERA_PORT_IMAGE_OUT_IMAGE, *imgCaptureData);
        }

        if ( ret == NO_ERROR )
            {
            length = imgCaptureData->mBufSize;
            }
        else
            {
            CAMHAL_LOGEB("setFormat() failed 0x%x", ret);
            length = 0;
            }
        }

    CAMHAL_LOGDB("getPictureBufferSize %d", length);

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::parseExpRange(const char *rangeStr,
                                         int * expRange,
                                         size_t count,
                                         size_t &validEntries)
{
    status_t ret = NO_ERROR;
    char *ctx, *expVal;
    char *tmp = NULL;
    size_t i = 0;

    LOG_FUNCTION_NAME;

    if ( NULL == rangeStr )
        {
        return -EINVAL;
        }

    if ( NULL == expRange )
        {
        return -EINVAL;
        }

    if ( NO_ERROR == ret )
        {
        tmp = ( char * ) malloc( strlen(rangeStr) + 1 );

        if ( NULL == tmp )
            {
            CAMHAL_LOGEA("No resources for temporary buffer");
            return -1;
            }
        memset(tmp, '\0', strlen(rangeStr) + 1);

        }

    if ( NO_ERROR == ret )
        {
        strncpy(tmp, rangeStr, strlen(rangeStr) );
        expVal = strtok_r( (char *) tmp, CameraHal::PARAMS_DELIMITER, &ctx);

        i = 0;
        while ( ( NULL != expVal ) && ( i < count ) )
            {
            expRange[i] = atoi(expVal);
            expVal = strtok_r(NULL, CameraHal::PARAMS_DELIMITER, &ctx);
            i++;
            }
        validEntries = i;
        }

    if ( NULL != tmp )
        {
        free(tmp);
        }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::setExposureBracketing(int *evValues,
                                                 size_t evCount,
                                                 size_t frameCount)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONFIG_CAPTUREMODETYPE expCapMode;
    OMX_CONFIG_EXTCAPTUREMODETYPE extExpCapMode;

    LOG_FUNCTION_NAME;

    if ( OMX_StateInvalid == mComponentState )
        {
        CAMHAL_LOGEA("OMX component is in invalid state");
        ret = -EINVAL;
        }

    if ( NULL == evValues )
        {
        CAMHAL_LOGEA("Exposure compensation values pointer is invalid");
        ret = -EINVAL;
        }

    if ( NO_ERROR == ret )
        {
        OMX_INIT_STRUCT_PTR (&expCapMode, OMX_CONFIG_CAPTUREMODETYPE);
        expCapMode.nPortIndex = mCameraAdapterParameters.mImagePortIndex;

        /// If frameCount>0 but evCount<=0, then this is the case of HQ burst.
        //Otherwise, it is normal HQ capture
        ///If frameCount>0 and evCount>0 then this is the cause of HQ Exposure bracketing.
        if ( 0 == evCount && 0 == frameCount )
            {
            expCapMode.bFrameLimited = OMX_FALSE;
            }
        else
            {
            expCapMode.bFrameLimited = OMX_TRUE;
            expCapMode.nFrameLimit = frameCount;
            }

        eError =  OMX_SetConfig(mCameraAdapterParameters.mHandleComp,
                                OMX_IndexConfigCaptureMode,
                                &expCapMode);
        if ( OMX_ErrorNone != eError )
            {
            CAMHAL_LOGEB("Error while configuring capture mode 0x%x", eError);
            }
        else
            {
            CAMHAL_LOGDA("Camera capture mode configured successfully");
            }
        }

    if ( NO_ERROR == ret )
        {
        OMX_INIT_STRUCT_PTR (&extExpCapMode, OMX_CONFIG_EXTCAPTUREMODETYPE);
        extExpCapMode.nPortIndex = mCameraAdapterParameters.mImagePortIndex;

        if ( 0 == evCount )
            {
            extExpCapMode.bEnableBracketing = OMX_FALSE;
            }
        else
            {
            extExpCapMode.bEnableBracketing = OMX_TRUE;
            extExpCapMode.tBracketConfigType.eBracketMode = OMX_BracketExposureRelativeInEV;
            extExpCapMode.tBracketConfigType.nNbrBracketingValues = evCount - 1;
            }

        for ( unsigned int i = 0 ; i < evCount ; i++ )
            {
            extExpCapMode.tBracketConfigType.nBracketValues[i]  =  ( evValues[i] * ( 1 << Q16_OFFSET ) )  / 10;
            }

        eError =  OMX_SetConfig(mCameraAdapterParameters.mHandleComp,
                                ( OMX_INDEXTYPE ) OMX_IndexConfigExtCaptureMode,
                                &extExpCapMode);
        if ( OMX_ErrorNone != eError )
            {
            CAMHAL_LOGEB("Error while configuring extended capture mode 0x%x", eError);
            }
        else
            {
            CAMHAL_LOGDA("Extended camera capture mode configured successfully");
            }
        }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::setShutterCallback(bool enabled)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONFIG_CALLBACKREQUESTTYPE shutterRequstCallback;

    LOG_FUNCTION_NAME;

    if ( OMX_StateExecuting != mComponentState )
        {
        CAMHAL_LOGEA("OMX component not in executing state");
        ret = -1;
        }

    if ( NO_ERROR == ret )
        {

        OMX_INIT_STRUCT_PTR (&shutterRequstCallback, OMX_CONFIG_CALLBACKREQUESTTYPE);
        shutterRequstCallback.nPortIndex = OMX_ALL;

        if ( enabled )
            {
            shutterRequstCallback.bEnable = OMX_TRUE;
            shutterRequstCallback.nIndex = ( OMX_INDEXTYPE ) OMX_TI_IndexConfigShutterCallback;
            CAMHAL_LOGDA("Enabling shutter callback");
            }
        else
            {
            shutterRequstCallback.bEnable = OMX_FALSE;
            shutterRequstCallback.nIndex = ( OMX_INDEXTYPE ) OMX_TI_IndexConfigShutterCallback;
            CAMHAL_LOGDA("Disabling shutter callback");
            }

        eError =  OMX_SetConfig(mCameraAdapterParameters.mHandleComp,
                                ( OMX_INDEXTYPE ) OMX_IndexConfigCallbackRequest,
                                &shutterRequstCallback);
        if ( OMX_ErrorNone != eError )
            {
            CAMHAL_LOGEB("Error registering shutter callback 0x%x", eError);
            ret = -1;
            }
        else
            {
            CAMHAL_LOGDB("Shutter callback for index 0x%x registered successfully",
                         OMX_TI_IndexConfigShutterCallback);
            }
        }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::doBracketing(OMX_BUFFERHEADERTYPE *pBuffHeader,
                                        CameraFrame::FrameType typeOfFrame)
{
    status_t ret = NO_ERROR;
    int currentBufferIdx, nextBufferIdx;
    OMXCameraPortParameters * imgCaptureData = NULL;

    LOG_FUNCTION_NAME;

    imgCaptureData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mImagePortIndex];

    if ( OMX_StateExecuting != mComponentState )
        {
        CAMHAL_LOGEA("OMX component is not in executing state");
        ret = -EINVAL;
        }

    if ( NO_ERROR == ret )
        {
        currentBufferIdx = ( unsigned int ) pBuffHeader->pAppPrivate;

        if ( currentBufferIdx >= imgCaptureData->mNumBufs)
            {
            CAMHAL_LOGEB("Invalid bracketing buffer index 0x%x", currentBufferIdx);
            ret = -EINVAL;
            }
        }

    if ( NO_ERROR == ret )
        {
        mBracketingBuffersQueued[currentBufferIdx] = false;
        mBracketingBuffersQueuedCount--;

        if ( 0 >= mBracketingBuffersQueuedCount )
            {
            nextBufferIdx = ( currentBufferIdx + 1 ) % imgCaptureData->mNumBufs;
            mBracketingBuffersQueued[nextBufferIdx] = true;
            mBracketingBuffersQueuedCount++;
            mLastBracetingBufferIdx = nextBufferIdx;
            setFrameRefCount(imgCaptureData->mBufferHeader[nextBufferIdx]->pBuffer, typeOfFrame, 1);
            returnFrame(imgCaptureData->mBufferHeader[nextBufferIdx]->pBuffer, typeOfFrame);
            }
        }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::sendBracketFrames()
{
    status_t ret = NO_ERROR;
    int currentBufferIdx;
    OMXCameraPortParameters * imgCaptureData = NULL;

    LOG_FUNCTION_NAME;

    imgCaptureData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mImagePortIndex];

    if ( OMX_StateExecuting != mComponentState )
        {
        CAMHAL_LOGEA("OMX component is not in executing state");
        ret = -EINVAL;
        }

    if ( NO_ERROR == ret )
        {

        currentBufferIdx = mLastBracetingBufferIdx;
        do
            {
            currentBufferIdx++;
            currentBufferIdx %= imgCaptureData->mNumBufs;
            if (!mBracketingBuffersQueued[currentBufferIdx] )
                {
                CameraFrame cameraFrame;
                sendCallBacks(cameraFrame,
                              imgCaptureData->mBufferHeader[currentBufferIdx],
                              imgCaptureData->mImageType,
                              imgCaptureData);
                }
            } while ( currentBufferIdx != mLastBracetingBufferIdx );

        }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::startBracketing(int range)
{
    status_t ret = NO_ERROR;
    OMXCameraPortParameters * imgCaptureData = NULL;

    LOG_FUNCTION_NAME;

    imgCaptureData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mImagePortIndex];

    if ( OMX_StateExecuting != mComponentState )
        {
        CAMHAL_LOGEA("OMX component is not in executing state");
        ret = -EINVAL;
        }

        {
        Mutex::Autolock lock(mBracketingLock);

        if ( mBracketingEnabled )
            {
            return ret;
            }
        }

    if ( 0 == imgCaptureData->mNumBufs )
        {
        CAMHAL_LOGEB("Image capture buffers set to %d", imgCaptureData->mNumBufs);
        ret = -EINVAL;
        }

    if ( mPending3Asettings )
        apply3Asettings(mParameters3A);

    if ( NO_ERROR == ret )
        {
        Mutex::Autolock lock(mBracketingLock);

        mBracketingRange = range;
        mBracketingBuffersQueued = new bool[imgCaptureData->mNumBufs];
        if ( NULL == mBracketingBuffersQueued )
            {
            CAMHAL_LOGEA("Unable to allocate bracketing management structures");
            ret = -1;
            }

        if ( NO_ERROR == ret )
            {
            mBracketingBuffersQueuedCount = imgCaptureData->mNumBufs;
            mLastBracetingBufferIdx = mBracketingBuffersQueuedCount - 1;

            for ( int i = 0 ; i  < imgCaptureData->mNumBufs ; i++ )
                {
                mBracketingBuffersQueued[i] = true;
                }

            }
        }

    if ( NO_ERROR == ret )
        {

        ret = startImageCapture();
            {
            Mutex::Autolock lock(mBracketingLock);

            if ( NO_ERROR == ret )
                {
                mBracketingEnabled = true;
                }
            else
                {
                mBracketingEnabled = false;
                }
            }
        }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::stopBracketing()
{
  status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    Mutex::Autolock lock(mBracketingLock);

    if ( NULL != mBracketingBuffersQueued )
    {
        delete [] mBracketingBuffersQueued;
    }

    ret = stopImageCapture();

    mBracketingBuffersQueued = NULL;
    mBracketingEnabled = false;
    mBracketingBuffersQueuedCount = 0;
    mLastBracetingBufferIdx = 0;

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::startImageCapture()
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMXCameraPortParameters * capData = NULL;
    OMX_CONFIG_BOOLEANTYPE bOMX;

    LOG_FUNCTION_NAME;

    if(!mCaptureConfigured)
        {
        ///Image capture was cancelled before we could start
        return NO_ERROR;
        }

    if ( 0 != mStartCaptureSem.Count() )
        {
        CAMHAL_LOGEB("Error mStartCaptureSem semaphore count %d", mStartCaptureSem.Count());
        return NO_INIT;
        }

    if ((getNextState() & (CAPTURE_ACTIVE|BRACKETING_ACTIVE)) == 0) {
        CAMHAL_LOGDA("trying starting capture when already canceled");
        return NO_ERROR;
    }

    // Camera framework doesn't expect face callbacks once capture is triggered
    pauseFaceDetection(true);

    //During bracketing image capture is already active
    {
    Mutex::Autolock lock(mBracketingLock);
    if ( mBracketingEnabled )
        {
        //Stop bracketing, activate normal burst for the remaining images
        mBracketingEnabled = false;
        mCapturedFrames = mBracketingRange;
        ret = sendBracketFrames();
        if(ret != NO_ERROR)
            goto EXIT;
        else
            return ret;
        }
    }

    if ( NO_ERROR == ret ) {
        if (mPendingCaptureSettings & SetRotation) {
            mPendingCaptureSettings &= ~SetRotation;
            ret = setPictureRotation(mPictureRotation);
            if ( NO_ERROR != ret ) {
                CAMHAL_LOGEB("Error configuring image rotation %x", ret);
            }
        }
    }

    // need to enable wb data for video snapshot to fill in exif data
    if ((ret == NO_ERROR) && (mCapMode == VIDEO_MODE)) {
        // video snapshot uses wb data from snapshot frame
        ret = setExtraData(true, mCameraAdapterParameters.mPrevPortIndex, OMX_WhiteBalance);
    }

    //OMX shutter callback events are only available in hq mode
    if ( (HIGH_QUALITY == mCapMode) || (HIGH_QUALITY_ZSL== mCapMode))
        {

        if ( NO_ERROR == ret )
            {
            ret = RegisterForEvent(mCameraAdapterParameters.mHandleComp,
                                        (OMX_EVENTTYPE) OMX_EventIndexSettingChanged,
                                        OMX_ALL,
                                        OMX_TI_IndexConfigShutterCallback,
                                        mStartCaptureSem);
            }

        if ( NO_ERROR == ret )
            {
            ret = setShutterCallback(true);
            }

        }

    if ( NO_ERROR == ret ) {
        capData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mImagePortIndex];

        ///Queue all the buffers on capture port
        for ( int index = 0 ; index < capData->mNumBufs ; index++ ) {
            CAMHAL_LOGDB("Queuing buffer on Capture port - 0x%x",
                         ( unsigned int ) capData->mBufferHeader[index]->pBuffer);
            eError = OMX_FillThisBuffer(mCameraAdapterParameters.mHandleComp,
                        (OMX_BUFFERHEADERTYPE*)capData->mBufferHeader[index]);

            GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);
        }

        mWaitingForSnapshot = true;
        mCaptureSignalled = false;

        // Capturing command is not needed when capturing in video mode
        // Only need to queue buffers on image ports
        if (mCapMode != VIDEO_MODE) {
            OMX_INIT_STRUCT_PTR (&bOMX, OMX_CONFIG_BOOLEANTYPE);
            bOMX.bEnabled = OMX_TRUE;

            /// sending Capturing Command to the component
            eError = OMX_SetConfig(mCameraAdapterParameters.mHandleComp,
                                   OMX_IndexConfigCapturing,
                                   &bOMX);

            CAMHAL_LOGDB("Capture set - 0x%x", eError);

            GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);
        }
    }

    //OMX shutter callback events are only available in hq mode
    if ( (HIGH_QUALITY == mCapMode) || (HIGH_QUALITY_ZSL== mCapMode))
        {

        if ( NO_ERROR == ret )
            {
            ret = mStartCaptureSem.WaitTimeout(OMX_CAPTURE_TIMEOUT);
            }

        //If something bad happened while we wait
        if (mComponentState != OMX_StateExecuting)
          {
            CAMHAL_LOGEA("Invalid State after Image Capture Exitting!!!");
            goto EXIT;
          }

        if ( NO_ERROR == ret )
            {
            CAMHAL_LOGDA("Shutter callback received");
            notifyShutterSubscribers();
            }
        else
            {
            ret |= RemoveEvent(mCameraAdapterParameters.mHandleComp,
                               (OMX_EVENTTYPE) OMX_EventIndexSettingChanged,
                               OMX_ALL,
                               OMX_TI_IndexConfigShutterCallback,
                               NULL);
            CAMHAL_LOGEA("Timeout expired on shutter callback");
            goto EXIT;
            }

        }

    return (ret | ErrorUtils::omxToAndroidError(eError));

EXIT:
    CAMHAL_LOGEB("Exiting function %s because of ret %d eError=%x", __FUNCTION__, ret, eError);
    setExtraData(false, mCameraAdapterParameters.mPrevPortIndex, OMX_WhiteBalance);
    mWaitingForSnapshot = false;
    mCaptureSignalled = false;
    performCleanupAfterError();
    LOG_FUNCTION_NAME_EXIT;
    return (ret | ErrorUtils::omxToAndroidError(eError));
}

status_t OMXCameraAdapter::stopImageCapture()
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONFIG_BOOLEANTYPE bOMX;
    OMXCameraPortParameters *imgCaptureData = NULL;

    LOG_FUNCTION_NAME;

    if (!mCaptureConfigured) {
        //Capture is not ongoing, return from here
        return NO_ERROR;
    }

    if ( 0 != mStopCaptureSem.Count() ) {
        CAMHAL_LOGEB("Error mStopCaptureSem semaphore count %d", mStopCaptureSem.Count());
        goto EXIT;
    }

    //Disable the callback first
    mWaitingForSnapshot = false;
    mSnapshotCount = 0;

    // OMX shutter callback events are only available in hq mode
    if ((HIGH_QUALITY == mCapMode) || (HIGH_QUALITY_ZSL== mCapMode)) {
        //Disable the callback first
        ret = setShutterCallback(false);

        // if anybody is waiting on the shutter callback
        // signal them and then recreate the semaphore
        if ( 0 != mStartCaptureSem.Count() ) {

            for (int i = mStartCaptureSem.Count(); i < 0; i++) {
            ret |= SignalEvent(mCameraAdapterParameters.mHandleComp,
                               (OMX_EVENTTYPE) OMX_EventIndexSettingChanged,
                               OMX_ALL,
                               OMX_TI_IndexConfigShutterCallback,
                               NULL );
            }
            mStartCaptureSem.Create(0);
        }
    }

    // After capture, face detection should be disabled
    // and application needs to restart face detection
    stopFaceDetection();

    //Wait here for the capture to be done, in worst case timeout and proceed with cleanup
    mCaptureSem.WaitTimeout(OMX_CAPTURE_TIMEOUT);

    //If somethiing bad happened while we wait
    if (mComponentState == OMX_StateInvalid)
      {
        CAMHAL_LOGEA("Invalid State Image Capture Stop Exitting!!!");
        goto EXIT;
      }

    // Disable image capture
    // Capturing command is not needed when capturing in video mode
    if (mCapMode != VIDEO_MODE) {
        OMX_INIT_STRUCT_PTR (&bOMX, OMX_CONFIG_BOOLEANTYPE);
        bOMX.bEnabled = OMX_FALSE;
        imgCaptureData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mImagePortIndex];
        eError = OMX_SetConfig(mCameraAdapterParameters.mHandleComp,
                               OMX_IndexConfigCapturing,
                               &bOMX);
        if ( OMX_ErrorNone != eError ) {
            CAMHAL_LOGDB("Error during SetConfig- 0x%x", eError);
            ret = -1;
            goto EXIT;
        }
    }

    // had to enable wb data for video snapshot to fill in exif data
    // now that we are done...disable
    if ((ret == NO_ERROR) && (mCapMode == VIDEO_MODE)) {
        ret = setExtraData(false, mCameraAdapterParameters.mPrevPortIndex, OMX_WhiteBalance);
    }

    CAMHAL_LOGDB("Capture set - 0x%x", eError);

    mCaptureSignalled = true; //set this to true if we exited because of timeout

    {
        Mutex::Autolock lock(mFrameCountMutex);
        mFrameCount = 0;
        mFirstFrameCondition.broadcast();
    }

    return (ret | ErrorUtils::omxToAndroidError(eError));

EXIT:
    CAMHAL_LOGEB("Exiting function %s because of ret %d eError=%x", __FUNCTION__, ret, eError);
    //Release image buffers
    if ( NULL != mReleaseImageBuffersCallback ) {
        mReleaseImageBuffersCallback(mReleaseData);
    }

    {
        Mutex::Autolock lock(mFrameCountMutex);
        mFrameCount = 0;
        mFirstFrameCondition.broadcast();
    }

    performCleanupAfterError();
    LOG_FUNCTION_NAME_EXIT;
    return (ret | ErrorUtils::omxToAndroidError(eError));
}

status_t OMXCameraAdapter::disableImagePort(){
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMXCameraPortParameters *imgCaptureData = NULL;

    if (!mCaptureConfigured) {
        return NO_ERROR;
    }

    mCaptureConfigured = false;
    imgCaptureData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mImagePortIndex];

    ///Register for Image port Disable event
    ret = RegisterForEvent(mCameraAdapterParameters.mHandleComp,
                                OMX_EventCmdComplete,
                                OMX_CommandPortDisable,
                                mCameraAdapterParameters.mImagePortIndex,
                                mStopCaptureSem);
    ///Disable Capture Port
    eError = OMX_SendCommand(mCameraAdapterParameters.mHandleComp,
                                OMX_CommandPortDisable,
                                mCameraAdapterParameters.mImagePortIndex,
                                NULL);

    ///Free all the buffers on capture port
    if (imgCaptureData) {
        CAMHAL_LOGDB("Freeing buffer on Capture port - %d", imgCaptureData->mNumBufs);
        for ( int index = 0 ; index < imgCaptureData->mNumBufs ; index++) {
            CAMHAL_LOGDB("Freeing buffer on Capture port - 0x%x",
                         ( unsigned int ) imgCaptureData->mBufferHeader[index]->pBuffer);
            eError = OMX_FreeBuffer(mCameraAdapterParameters.mHandleComp,
                                    mCameraAdapterParameters.mImagePortIndex,
                                    (OMX_BUFFERHEADERTYPE*)imgCaptureData->mBufferHeader[index]);

            GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);
        }
    }
    CAMHAL_LOGDA("Waiting for port disable");
    //Wait for the image port enable event
    ret = mStopCaptureSem.WaitTimeout(OMX_CMD_TIMEOUT);

    //If somethiing bad happened while we wait
    if (mComponentState == OMX_StateInvalid)
      {
        CAMHAL_LOGEA("Invalid State after Disable Image Port Exitting!!!");
        goto EXIT;
      }

    if ( NO_ERROR == ret ) {
        CAMHAL_LOGDA("Port disabled");
    } else {
        ret |= RemoveEvent(mCameraAdapterParameters.mHandleComp,
                           OMX_EventCmdComplete,
                           OMX_CommandPortDisable,
                           mCameraAdapterParameters.mImagePortIndex,
                           NULL);
        CAMHAL_LOGDA("Timeout expired on port disable");
        goto EXIT;
    }

 EXIT:
    return (ret | ErrorUtils::omxToAndroidError(eError));
}


status_t OMXCameraAdapter::UseBuffersCapture(void* bufArr, int num)
{
    LOG_FUNCTION_NAME;

    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMXCameraPortParameters * imgCaptureData = NULL;
    uint32_t *buffers = (uint32_t*)bufArr;
    OMXCameraPortParameters cap;

    imgCaptureData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mImagePortIndex];

    if ( 0 != mUseCaptureSem.Count() )
        {
        CAMHAL_LOGEB("Error mUseCaptureSem semaphore count %d", mUseCaptureSem.Count());
        return BAD_VALUE;
        }

    // capture is already configured...we can skip this step
    if (mCaptureConfigured) {

        if ( NO_ERROR == ret )
            {
            ret = setupEXIF();
            if ( NO_ERROR != ret )
                {
                CAMHAL_LOGEB("Error configuring EXIF Buffer %x", ret);
                }
            }

        mCapturedFrames = mBurstFrames;
        return NO_ERROR;
    }

    imgCaptureData->mNumBufs = num;

    //TODO: Support more pixelformats

    CAMHAL_LOGDB("Params Width = %d", (int)imgCaptureData->mWidth);
    CAMHAL_LOGDB("Params Height = %d", (int)imgCaptureData->mWidth);

    if (mPendingCaptureSettings & SetFormat) {
        mPendingCaptureSettings &= ~SetFormat;
        ret = setFormat(OMX_CAMERA_PORT_IMAGE_OUT_IMAGE, *imgCaptureData);
        if ( ret != NO_ERROR ) {
            CAMHAL_LOGEB("setFormat() failed %d", ret);
            LOG_FUNCTION_NAME_EXIT;
            return ret;
        }
    }

    if (mPendingCaptureSettings & SetThumb) {
        mPendingCaptureSettings &= ~SetThumb;
        ret = setThumbnailParams(mThumbWidth, mThumbHeight, mThumbQuality);
        if ( NO_ERROR != ret) {
            CAMHAL_LOGEB("Error configuring thumbnail size %x", ret);
            return ret;
        }
    }

    if (mPendingCaptureSettings & SetExpBracket) {
        mPendingCaptureSettings &= ~SetExpBracket;
        ret = setExposureBracketing( mExposureBracketingValues,
                                     mExposureBracketingValidEntries, mBurstFrames);
        if ( ret != NO_ERROR ) {
            CAMHAL_LOGEB("setExposureBracketing() failed %d", ret);
            goto EXIT;
        }
    }

    if (mPendingCaptureSettings & SetQuality) {
        mPendingCaptureSettings &= ~SetQuality;
        ret = setImageQuality(mPictureQuality);
        if ( NO_ERROR != ret) {
            CAMHAL_LOGEB("Error configuring image quality %x", ret);
            goto EXIT;
        }
    }

    ///Register for Image port ENABLE event
    ret = RegisterForEvent(mCameraAdapterParameters.mHandleComp,
                           OMX_EventCmdComplete,
                           OMX_CommandPortEnable,
                           mCameraAdapterParameters.mImagePortIndex,
                           mUseCaptureSem);

    ///Enable Capture Port
    eError = OMX_SendCommand(mCameraAdapterParameters.mHandleComp,
                             OMX_CommandPortEnable,
                             mCameraAdapterParameters.mImagePortIndex,
                             NULL);

    CAMHAL_LOGDB("OMX_UseBuffer = 0x%x", eError);
    GOTO_EXIT_IF(( eError != OMX_ErrorNone ), eError);

    for ( int index = 0 ; index < imgCaptureData->mNumBufs ; index++ )
    {
        OMX_BUFFERHEADERTYPE *pBufferHdr;
        CAMHAL_LOGDB("OMX_UseBuffer Capture address: 0x%x, size = %d",
                     (unsigned int)buffers[index],
                     (int)imgCaptureData->mBufSize);

        eError = OMX_UseBuffer(mCameraAdapterParameters.mHandleComp,
                               &pBufferHdr,
                               mCameraAdapterParameters.mImagePortIndex,
                               0,
                               mCaptureBuffersLength,
                               (OMX_U8*)buffers[index]);

        CAMHAL_LOGDB("OMX_UseBuffer = 0x%x", eError);
        GOTO_EXIT_IF(( eError != OMX_ErrorNone ), eError);

        pBufferHdr->pAppPrivate = (OMX_PTR) index;
        pBufferHdr->nSize = sizeof(OMX_BUFFERHEADERTYPE);
        pBufferHdr->nVersion.s.nVersionMajor = 1 ;
        pBufferHdr->nVersion.s.nVersionMinor = 1 ;
        pBufferHdr->nVersion.s.nRevision = 0;
        pBufferHdr->nVersion.s.nStep =  0;
        imgCaptureData->mBufferHeader[index] = pBufferHdr;
    }

    //Wait for the image port enable event
    CAMHAL_LOGDA("Waiting for port enable");
    ret = mUseCaptureSem.WaitTimeout(OMX_CMD_TIMEOUT);

    //If somethiing bad happened while we wait
    if (mComponentState == OMX_StateInvalid)
      {
        CAMHAL_LOGEA("Invalid State after Enable Image Port Exitting!!!");
        goto EXIT;
      }

    if ( ret == NO_ERROR )
        {
        CAMHAL_LOGDA("Port enabled");
        }
    else
        {
        ret |= RemoveEvent(mCameraAdapterParameters.mHandleComp,
                           OMX_EventCmdComplete,
                           OMX_CommandPortEnable,
                           mCameraAdapterParameters.mImagePortIndex,
                           NULL);
        CAMHAL_LOGDA("Timeout expired on port enable");
        goto EXIT;
        }

    if ( NO_ERROR == ret )
        {
        ret = setupEXIF();
        if ( NO_ERROR != ret )
            {
            CAMHAL_LOGEB("Error configuring EXIF Buffer %x", ret);
            }
        }

    mCapturedFrames = mBurstFrames;
    mCaptureConfigured = true;

    return (ret | ErrorUtils::omxToAndroidError(eError));

EXIT:
    CAMHAL_LOGEB("Exiting function %s because of ret %d eError=%x", __FUNCTION__, ret, eError);
    //Release image buffers
    if ( NULL != mReleaseImageBuffersCallback ) {
        mReleaseImageBuffersCallback(mReleaseData);
    }
    performCleanupAfterError();
    LOG_FUNCTION_NAME_EXIT;
    return (ret | ErrorUtils::omxToAndroidError(eError));

}

};
