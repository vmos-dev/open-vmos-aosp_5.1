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

#include "CameraHal.h"
#include "OMXCameraAdapter.h"
#include "ErrorUtils.h"


namespace Ti {
namespace Camera {

status_t OMXCameraAdapter::setParametersCapture(const android::CameraParameters &params,
                                                BaseCameraAdapter::AdapterState state)
{
    status_t ret = NO_ERROR;
    const char *str = NULL;
    int w, h;
    OMX_COLOR_FORMATTYPE pixFormat;
    CodingMode codingMode = mCodingMode;
    const char *valstr = NULL;
    int varint = 0;
    OMX_TI_STEREOFRAMELAYOUTTYPE capFrmLayout;
    bool inCaptureState = false;

    LOG_FUNCTION_NAME;

    OMXCameraPortParameters *cap;
    cap = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mImagePortIndex];

    capFrmLayout = cap->mFrameLayoutType;
    setParamS3D(mCameraAdapterParameters.mImagePortIndex,
        params.get(TICameraParameters::KEY_S3D_CAP_FRAME_LAYOUT));
    if (capFrmLayout != cap->mFrameLayoutType) {
        mPendingCaptureSettings |= SetFormat;
    }

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
        if (strcmp(valstr, android::CameraParameters::PIXEL_FORMAT_YUV422I) == 0) {
            CAMHAL_LOGDA("CbYCrY format selected");
            pixFormat = OMX_COLOR_FormatCbYCrY;
            mPictureFormatFromClient = android::CameraParameters::PIXEL_FORMAT_YUV422I;
        } else if(strcmp(valstr, android::CameraParameters::PIXEL_FORMAT_YUV420SP) == 0) {
            CAMHAL_LOGDA("YUV420SP format selected");
            pixFormat = OMX_COLOR_FormatYUV420SemiPlanar;
            mPictureFormatFromClient = android::CameraParameters::PIXEL_FORMAT_YUV420SP;
        } else if(strcmp(valstr, android::CameraParameters::PIXEL_FORMAT_RGB565) == 0) {
            CAMHAL_LOGDA("RGB565 format selected");
            pixFormat = OMX_COLOR_Format16bitRGB565;
            mPictureFormatFromClient = android::CameraParameters::PIXEL_FORMAT_RGB565;
        } else if (strcmp(valstr, android::CameraParameters::PIXEL_FORMAT_JPEG) == 0) {
            CAMHAL_LOGDA("JPEG format selected");
            pixFormat = OMX_COLOR_FormatUnused;
            codingMode = CodingJPEG;
            mPictureFormatFromClient = android::CameraParameters::PIXEL_FORMAT_JPEG;
        } else if (strcmp(valstr, TICameraParameters::PIXEL_FORMAT_JPS) == 0) {
            CAMHAL_LOGDA("JPS format selected");
            pixFormat = OMX_COLOR_FormatUnused;
            codingMode = CodingJPS;
            mPictureFormatFromClient = TICameraParameters::PIXEL_FORMAT_JPS;
        } else if (strcmp(valstr, TICameraParameters::PIXEL_FORMAT_MPO) == 0) {
            CAMHAL_LOGDA("MPO format selected");
            pixFormat = OMX_COLOR_FormatUnused;
            codingMode = CodingMPO;
            mPictureFormatFromClient = TICameraParameters::PIXEL_FORMAT_MPO;
        } else if (strcmp(valstr, android::CameraParameters::PIXEL_FORMAT_BAYER_RGGB) == 0) {
            CAMHAL_LOGDA("RAW Picture format selected");
            pixFormat = OMX_COLOR_FormatRawBayer10bit;
            mPictureFormatFromClient = android::CameraParameters::PIXEL_FORMAT_BAYER_RGGB;
        } else {
            CAMHAL_LOGEA("Invalid format, JPEG format selected as default");
            pixFormat = OMX_COLOR_FormatUnused;
            codingMode = CodingJPEG;
            mPictureFormatFromClient = NULL;
        }
    } else {
        CAMHAL_LOGEA("Picture format is NULL, defaulting to JPEG");
        pixFormat = OMX_COLOR_FormatUnused;
        codingMode = CodingJPEG;
        mPictureFormatFromClient = NULL;
    }

#ifdef CAMERAHAL_USE_RAW_IMAGE_SAVING
    mRawCapture = false;
    mYuvCapture = false;

    valstr = params.get(TICameraParameters::KEY_CAP_MODE);
    if ( (!valstr || strcmp(valstr, TICameraParameters::HIGH_QUALITY_MODE) == 0) &&
            access(kRawImagesOutputDirPath, F_OK) != -1 ) {
        mRawCapture = true;
    }

    if (mRawCapture && (access(kYuvImagesOutputDirPath, F_OK) != -1)) {
        pixFormat = OMX_COLOR_FormatCbYCrY;
        mYuvCapture = true;
    }
#endif
    // JPEG capture is not supported in video mode by OMX Camera
    // Set capture format to yuv422i...jpeg encode will
    // be done on A9
    valstr = params.get(TICameraParameters::KEY_CAP_MODE);
    if ( (valstr && ( strcmp(valstr, (const char *) TICameraParameters::VIDEO_MODE) == 0 ||
                      strcmp(valstr, (const char *) TICameraParameters::VIDEO_MODE_HQ) == 0 ) ) &&
            (pixFormat == OMX_COLOR_FormatUnused) ) {
        CAMHAL_LOGDA("Capturing in video mode...selecting yuv422i");
        pixFormat = OMX_COLOR_FormatCbYCrY;
    }

    if (pixFormat != cap->mColorFormat || codingMode != mCodingMode) {
        mPendingCaptureSettings |= SetFormat;
        cap->mColorFormat = pixFormat;
        mCodingMode = codingMode;
    }

#ifdef OMAP_ENHANCEMENT
    str = params.get(TICameraParameters::KEY_TEMP_BRACKETING);
    if ( ( str != NULL ) &&
         ( strcmp(str, android::CameraParameters::TRUE) == 0 ) ) {

        if ( !mBracketingSet ) {
            mPendingCaptureSettings |= SetBurstExpBracket;
        }

        mBracketingSet = true;
    } else {

        if ( mBracketingSet ) {
            mPendingCaptureSettings |= SetBurstExpBracket;
        }

        mBracketingSet = false;
    }

    if ( (str = params.get(TICameraParameters::KEY_EXP_BRACKETING_RANGE)) != NULL ) {
        parseExpRange(str, mExposureBracketingValues, NULL,
                      mExposureGainBracketingModes,
                      EXP_BRACKET_RANGE, mExposureBracketingValidEntries);
        if (mCapMode == OMXCameraAdapter::CP_CAM) {
            mExposureBracketMode = OMX_BracketVectorShot;
        } else {
            mExposureBracketMode = OMX_BracketExposureRelativeInEV;
        }
        mPendingCaptureSettings |= SetBurstExpBracket;
    } else if ( (str = params.get(TICameraParameters::KEY_EXP_GAIN_BRACKETING_RANGE)) != NULL) {
        parseExpRange(str, mExposureBracketingValues, mExposureGainBracketingValues,
                      mExposureGainBracketingModes,
                      EXP_BRACKET_RANGE, mExposureBracketingValidEntries);
        if (mCapMode == OMXCameraAdapter::CP_CAM) {
            mExposureBracketMode = OMX_BracketVectorShot;
        } else {
            mExposureBracketMode = OMX_BracketExposureGainAbsolute;
        }
        mPendingCaptureSettings |= SetBurstExpBracket;
    } else {
        // always set queued shot config in CPCAM mode
        if (mCapMode == OMXCameraAdapter::CP_CAM) {
            mExposureBracketMode = OMX_BracketVectorShot;
            mPendingCaptureSettings |= SetBurstExpBracket;
        }
        // if bracketing was previously set...we set again before capturing to clear
        if (mExposureBracketingValidEntries) {
            mPendingCaptureSettings |= SetBurstExpBracket;
            mExposureBracketingValidEntries = 0;
        }
    }

    str = params.get(TICameraParameters::KEY_ZOOM_BRACKETING_RANGE);
    if ( NULL != str ) {
        parseExpRange(str, mZoomBracketingValues, NULL, NULL,
                      ZOOM_BRACKET_RANGE, mZoomBracketingValidEntries);
        mCurrentZoomBracketing = 0;
        mZoomBracketingEnabled = true;
    } else {
        if (mZoomBracketingValidEntries) {
            mZoomBracketingValidEntries = 0;
        }
        mZoomBracketingEnabled = false;
    }
#endif

    // Flush config queue
    // If TRUE: Flush queue and abort processing before enqueing
    valstr = params.get(TICameraParameters::KEY_FLUSH_SHOT_CONFIG_QUEUE);
    if ( NULL != valstr ) {
        if ( 0 == strcmp(valstr, android::CameraParameters::TRUE) ) {
            mFlushShotConfigQueue = true;
        } else if ( 0 == strcmp(valstr, android::CameraParameters::FALSE) ) {
            mFlushShotConfigQueue = false;
        } else {
            CAMHAL_LOGE("Missing flush shot config parameter. Will use current (%s)",
                        mFlushShotConfigQueue ? "true" : "false");
        }
    }

    if ( params.getInt(android::CameraParameters::KEY_ROTATION) != -1 )
        {
        if (params.getInt(android::CameraParameters::KEY_ROTATION) != (int) mPictureRotation) {
            mPendingCaptureSettings |= SetRotation;
        }
        mPictureRotation = params.getInt(android::CameraParameters::KEY_ROTATION);
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
    if ( varint != -1 )
        {
        mSensorOrientation = varint;
        if (mSensorOrientation == 270 ||mSensorOrientation==90)
            {
            CAMHAL_LOGEA(" Orientation is 270/90. So setting counter rotation to Ducati");
            mSensorOrientation +=180;
            mSensorOrientation%=360;
            }
        }
    else
        {
        mSensorOrientation = 0;
        }

    CAMHAL_LOGVB("Sensor Orientation  set : %d", mSensorOrientation);
#endif

#ifdef OMAP_ENHANCEMENT_BURST_CAPTURE
    varint = params.getInt(TICameraParameters::KEY_BURST);
    if ( varint >= 1 )
        {
        if (varint != (int) mBurstFrames) {
            mPendingCaptureSettings |= SetBurstExpBracket;
        }
        mBurstFrames = varint;
        }
    else
        {
        if (mBurstFrames != 1) mPendingCaptureSettings |= SetBurstExpBracket;
        mBurstFrames = 1;
        }

    CAMHAL_LOGVB("Burst Frames set %d", mBurstFrames);
#endif

    varint = params.getInt(android::CameraParameters::KEY_JPEG_QUALITY);
    if ( varint >= MIN_JPEG_QUALITY && varint <= MAX_JPEG_QUALITY ) {
        if (varint != mPictureQuality) {
            mPendingCaptureSettings |= SetQuality;
            mPictureQuality = varint;
        }
    } else {
        if (mPictureQuality != MAX_JPEG_QUALITY) {
            mPendingCaptureSettings |= SetQuality;
            mPictureQuality = MAX_JPEG_QUALITY;
        }
    }

    CAMHAL_LOGVB("Picture Quality set %d", mPictureQuality);

    varint = params.getInt(android::CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH);
    if ( varint >= 0 ) {
        if (varint != mThumbWidth) {
            mPendingCaptureSettings |= SetThumb;
            mThumbWidth = varint;
        }
    } else {
        if (mThumbWidth != DEFAULT_THUMB_WIDTH) {
            mPendingCaptureSettings |= SetThumb;
            mThumbWidth = DEFAULT_THUMB_WIDTH;
        }
    }

    CAMHAL_LOGVB("Picture Thumb width set %d", mThumbWidth);

    varint = params.getInt(android::CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT);
    if ( varint >= 0 ) {
        if (varint != mThumbHeight) {
            mPendingCaptureSettings |= SetThumb;
            mThumbHeight = varint;
        }
    } else {
        if (mThumbHeight != DEFAULT_THUMB_HEIGHT) {
            mPendingCaptureSettings |= SetThumb;
            mThumbHeight = DEFAULT_THUMB_HEIGHT;
        }
    }

    CAMHAL_LOGVB("Picture Thumb height set %d", mThumbHeight);

    varint = params.getInt(android::CameraParameters::KEY_JPEG_THUMBNAIL_QUALITY);
    if ( varint >= MIN_JPEG_QUALITY && varint <= MAX_JPEG_QUALITY ) {
        if (varint != mThumbQuality) {
            mPendingCaptureSettings |= SetThumb;
            mThumbQuality = varint;
        }
    } else {
        if (mThumbQuality != MAX_JPEG_QUALITY) {
            mPendingCaptureSettings |= SetThumb;
            mThumbQuality = MAX_JPEG_QUALITY;
        }
    }

    CAMHAL_LOGDB("Thumbnail Quality set %d", mThumbQuality);

    if (mFirstTimeInit) {
        mPendingCaptureSettings = ECapturesettingsAll;
    }

    cap = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mVideoPortIndex];
    cap->mWidth = params.getInt(TICameraParameters::RAW_WIDTH);
    cap->mHeight = params.getInt(TICameraParameters::RAW_HEIGHT);

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::getPictureBufferSize(CameraFrame &frame, size_t bufferCount)
{
    status_t ret = NO_ERROR;
    OMXCameraPortParameters *imgCaptureData = NULL;
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    LOG_FUNCTION_NAME;

    if ( NO_ERROR == ret )
        {
        imgCaptureData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mImagePortIndex];


        // If any settings have changed that need to be set with SetParam,
        // we will need to disable the port to set them
        if ((mPendingCaptureSettings & ECaptureParamSettings)) {
            disableImagePort();
            if ( NULL != mReleaseImageBuffersCallback ) {
                mReleaseImageBuffersCallback(mReleaseData);
            }
        }

        if (mPendingCaptureSettings & SetFormat) {
            ret = setFormat(OMX_CAMERA_PORT_IMAGE_OUT_IMAGE, *imgCaptureData);
        }

        if ( ret == NO_ERROR )
            {
            frame.mLength = imgCaptureData->mBufSize;
            frame.mWidth = imgCaptureData->mWidth;
            frame.mHeight = imgCaptureData->mHeight;
            frame.mAlignment = imgCaptureData->mStride;
            CAMHAL_LOGDB("getPictureBufferSize: width:%u height:%u alignment:%u length:%u",
                         frame.mWidth, frame.mHeight, frame.mAlignment, frame.mLength);
            }
        else
            {
            CAMHAL_LOGEB("setFormat() failed 0x%x", ret);
            }
        }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

int OMXCameraAdapter::getBracketingValueMode(const char *a, const char *b) const
{
    BracketingValueMode bvm = BracketingValueAbsolute;

    if ( (NULL != b) &&
         (NULL != a) &&
         (a < b) &&
         ( (NULL != memchr(a, '+', b - a)) ||
           (NULL != memchr(a, '-', b - a)) ) ) {
        bvm = BracketingValueRelative;
    }
    return bvm;
}

status_t OMXCameraAdapter::parseExpRange(const char *rangeStr,
                                         int *expRange,
                                         int *gainRange,
                                         int *expGainModes,
                                         size_t count,
                                         size_t &validEntries)
{
    status_t ret = NO_ERROR;
    char *end = NULL;
    const char *startPtr = NULL;
    size_t i = 0;

    LOG_FUNCTION_NAME;

    if ( NULL == rangeStr ){
        return -EINVAL;
    }

    if ( NULL == expRange ){
        return -EINVAL;
    }

    if ( NO_ERROR == ret ) {
        startPtr = rangeStr;
        do {
            // Relative Exposure example: "-30,-10, 0, 10, 30"
            // Absolute Gain ex. (exposure,gain) pairs: "(100,300),(200,300),(400,300),(800,300),(1600,300)"
            // Relative Gain ex. (exposure,gain) pairs: "(-30,+0),(-10, +0),(+0,+0),(+10,+0),(+30,+0)"
            // Forced relative Exposure example: "-30F,-10F, 0F, 10F, 30F"
            // Forced absolute Gain ex. (exposure,gain) pairs: "(100,300)F,(200,300)F,(400,300)F,(800,300)F,(1600,300)F"
            // Forced relative Gain ex. (exposure,gain) pairs: "(-30,+0)F,(-10, +0)F,(+0,+0)F,(+10,+0)F,(+30,+0)F"

            // skip '(' and ','
            while ((*startPtr == '(') ||  (*startPtr == ',')) startPtr++;

            expRange[i] = (int)strtol(startPtr, &end, 10);

            if (expGainModes) {
                // if gainRange is given rangeStr should be (exposure, gain) pair
                if (gainRange) {
                    int bvm_exp = getBracketingValueMode(startPtr, end);
                    startPtr = end + 1; // for the ','
                    gainRange[i] = (int)strtol(startPtr, &end, 10);

                    if (BracketingValueAbsolute == bvm_exp) {
                        expGainModes[i] = getBracketingValueMode(startPtr, end);
                    } else {
                        expGainModes[i] = bvm_exp;
                    }
                } else {
                    expGainModes[i] = BracketingValueCompensation;
                }
            }
            startPtr = end;

            // skip ')'
            while (*startPtr == ')') startPtr++;

            // Check for "forced" key
            if (expGainModes) {
                while ((*startPtr == 'F') || (*startPtr == 'f')) {
                    if ( BracketingValueAbsolute == expGainModes[i] ) {
                        expGainModes[i] = BracketingValueAbsoluteForced;
                    } else if ( BracketingValueRelative == expGainModes[i] ) {
                        expGainModes[i] = BracketingValueRelativeForced;
                    } else if ( BracketingValueCompensation == expGainModes[i] ) {
                        expGainModes[i] = BracketingValueCompensationForced;
                    } else {
                        CAMHAL_LOGE("Unexpected old mode 0x%x", expGainModes[i]);
                    }
                    startPtr++;
                }
            }

            i++;

        } while ((startPtr[0] != '\0') && (i < count));
        validEntries = i;
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::doExposureBracketing(int *evValues,
                                                 int *evValues2,
                                                 int *evModes2,
                                                 size_t evCount,
                                                 size_t frameCount,
                                                 bool flush,
                                                 OMX_BRACKETMODETYPE bracketMode)
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    if ( OMX_StateInvalid == mComponentState ) {
        CAMHAL_LOGEA("OMX component is in invalid state");
        ret = -EINVAL;
    }

    if ( NULL == evValues ) {
        CAMHAL_LOGEA("Exposure compensation values pointer is invalid");
        ret = -EINVAL;
    }

    if ( NO_ERROR == ret ) {
        if (bracketMode == OMX_BracketVectorShot) {
            ret = setVectorShot(evValues, evValues2, evModes2, evCount, frameCount, flush, bracketMode);
        } else {
            ret = setExposureBracketing(evValues, evValues2, evCount, frameCount, bracketMode);
        }
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::setVectorStop(bool toPreview)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_TI_CONFIG_VECTSHOTSTOPMETHODTYPE vecShotStop;


    LOG_FUNCTION_NAME;

    OMX_INIT_STRUCT_PTR(&vecShotStop, OMX_TI_CONFIG_VECTSHOTSTOPMETHODTYPE);

    vecShotStop.nPortIndex = mCameraAdapterParameters.mImagePortIndex;
    if (toPreview) {
        vecShotStop.eStopMethod =  OMX_TI_VECTSHOTSTOPMETHOD_GOTO_PREVIEW;
    } else {
        vecShotStop.eStopMethod =  OMX_TI_VECTSHOTSTOPMETHOD_WAIT_IN_CAPTURE;
    }

    eError =  OMX_SetConfig(mCameraAdapterParameters.mHandleComp,
                           (OMX_INDEXTYPE) OMX_TI_IndexConfigVectShotStopMethod,
                           &vecShotStop);
    if (OMX_ErrorNone != eError) {
        CAMHAL_LOGEB("Error while configuring bracket shot 0x%x", eError);
    } else {
        CAMHAL_LOGDA("Bracket shot configured successfully");
    }

    LOG_FUNCTION_NAME_EXIT;

    return (ret | Utils::ErrorUtils::omxToAndroidError(eError));
}

status_t OMXCameraAdapter::initVectorShot()
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONFIG_CAPTUREMODETYPE expCapMode;
    OMX_CONFIG_EXTCAPTUREMODETYPE extExpCapMode;

    LOG_FUNCTION_NAME;

    if (NO_ERROR == ret) {
        OMX_INIT_STRUCT_PTR (&expCapMode, OMX_CONFIG_CAPTUREMODETYPE);
        expCapMode.nPortIndex = mCameraAdapterParameters.mImagePortIndex;

        expCapMode.bFrameLimited = OMX_FALSE;

        eError =  OMX_SetConfig(mCameraAdapterParameters.mHandleComp,
                                OMX_IndexConfigCaptureMode,
                                &expCapMode);
        if (OMX_ErrorNone != eError) {
            CAMHAL_LOGEB("Error while configuring capture mode 0x%x", eError);
            goto exit;
        } else {
            CAMHAL_LOGDA("Camera capture mode configured successfully");
        }
    }

    if (NO_ERROR == ret) {
        OMX_INIT_STRUCT_PTR (&extExpCapMode, OMX_CONFIG_EXTCAPTUREMODETYPE);
        extExpCapMode.nPortIndex = mCameraAdapterParameters.mImagePortIndex;

        extExpCapMode.bEnableBracketing = OMX_TRUE;
        extExpCapMode.tBracketConfigType.eBracketMode = OMX_BracketVectorShot;

        eError =  OMX_SetConfig(mCameraAdapterParameters.mHandleComp,
                                ( OMX_INDEXTYPE ) OMX_IndexConfigExtCaptureMode,
                                &extExpCapMode);
        if ( OMX_ErrorNone != eError ) {
            CAMHAL_LOGEB("Error while configuring extended capture mode 0x%x", eError);
            goto exit;
        } else {
            CAMHAL_LOGDA("Extended camera capture mode configured successfully");
        }
    }


    if (NO_ERROR == ret) {
        // set vector stop method to stop in capture
        ret = setVectorStop(false);
    }

 exit:
    LOG_FUNCTION_NAME_EXIT;

    return (ret | Utils::ErrorUtils::omxToAndroidError(eError));
}

status_t OMXCameraAdapter::setVectorShot(int *evValues,
                                         int *evValues2,
                                         int *evModes2,
                                         size_t evCount,
                                         size_t frameCount,
                                         bool flush,
                                         OMX_BRACKETMODETYPE bracketMode)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_TI_CONFIG_ENQUEUESHOTCONFIGS enqueueShotConfigs;
    OMX_TI_CONFIG_QUERYAVAILABLESHOTS queryAvailableShots;
    bool doFlush = flush;

    LOG_FUNCTION_NAME;

    OMX_INIT_STRUCT_PTR(&enqueueShotConfigs, OMX_TI_CONFIG_ENQUEUESHOTCONFIGS);
    OMX_INIT_STRUCT_PTR(&queryAvailableShots, OMX_TI_CONFIG_QUERYAVAILABLESHOTS);

    queryAvailableShots.nPortIndex = mCameraAdapterParameters.mImagePortIndex;
    eError = OMX_GetConfig(mCameraAdapterParameters.mHandleComp,
                                (OMX_INDEXTYPE) OMX_TI_IndexConfigQueryAvailableShots,
                                &queryAvailableShots);
    if (OMX_ErrorNone != eError) {
        CAMHAL_LOGE("Error getting available shots 0x%x", eError);
        goto exit;
    } else {
        CAMHAL_LOGD("AVAILABLE SHOTS: %d", queryAvailableShots.nAvailableShots);
        if (queryAvailableShots.nAvailableShots < evCount) {
            // TODO(XXX): Need to implement some logic to handle this error
            CAMHAL_LOGE("Not enough available shots to fulfill this queue request");
            ret = -ENOSPC;
            goto exit;
        }
    }

    for ( unsigned int confID = 0; confID < evCount; ) {
        unsigned int i;
        for ( i = 0 ; (i < ARRAY_SIZE(enqueueShotConfigs.nShotConfig)) && (confID < evCount); i++, confID++ ) {
                CAMHAL_LOGD("%2u: (%7d,%4d) mode: %d", confID, evValues[confID], evValues2[confID], evModes2[confID]);
                enqueueShotConfigs.nShotConfig[i].nConfigId = confID;
                enqueueShotConfigs.nShotConfig[i].nFrames = 1;
                if ( (BracketingValueCompensation == evModes2[confID]) ||
                     (BracketingValueCompensationForced == evModes2[confID]) ) {
                    // EV compensation
                    enqueueShotConfigs.nShotConfig[i].nEC = evValues[confID];
                    enqueueShotConfigs.nShotConfig[i].nExp = 0;
                    enqueueShotConfigs.nShotConfig[i].nGain = 0;
                } else {
                    // exposure,gain pair
                    enqueueShotConfigs.nShotConfig[i].nEC = 0;
                    enqueueShotConfigs.nShotConfig[i].nExp = evValues[confID];
                    enqueueShotConfigs.nShotConfig[i].nGain = evValues2[confID];
                }
                enqueueShotConfigs.nShotConfig[i].eExpGainApplyMethod = OMX_TI_EXPGAINAPPLYMETHOD_ABSOLUTE;
                switch (evModes2[confID]) {
                    case BracketingValueAbsolute: // (exp,gain) pairs directly program sensor values
                    default :
                        enqueueShotConfigs.nShotConfig[i].eExpGainApplyMethod = OMX_TI_EXPGAINAPPLYMETHOD_ABSOLUTE;
                        break;
                    case BracketingValueRelative: // (exp,gain) pairs relative to AE settings and constraints
                    case BracketingValueCompensation: // EV compensation relative to AE settings and constraints
                        enqueueShotConfigs.nShotConfig[i].eExpGainApplyMethod = OMX_TI_EXPGAINAPPLYMETHOD_RELATIVE;
                        break;
                    case BracketingValueAbsoluteForced: // (exp,gain) pairs directly program sensor values
                        // are forced over constraints due to flicker, etc.
                        enqueueShotConfigs.nShotConfig[i].eExpGainApplyMethod = OMX_TI_EXPGAINAPPLYMETHOD_FORCE_ABSOLUTE;
                        break;
                    case BracketingValueRelativeForced: // (exp, gain) pairs relative to AE settings AND settings
                    case BracketingValueCompensationForced: // EV compensation relative to AE settings and constraints
                        // are forced over constraints due to flicker, etc.
                        enqueueShotConfigs.nShotConfig[i].eExpGainApplyMethod = OMX_TI_EXPGAINAPPLYMETHOD_FORCE_RELATIVE;
                        break;
                }
                enqueueShotConfigs.nShotConfig[i].bNoSnapshot = OMX_FALSE; // TODO: Make this configurable
        }

        // Repeat last exposure and again
        if ((confID == evCount) && (evCount > 0) && (frameCount > evCount) && (0 != i)) {
            enqueueShotConfigs.nShotConfig[i-1].nFrames = frameCount - evCount;
        }

        enqueueShotConfigs.nPortIndex = mCameraAdapterParameters.mImagePortIndex;
        enqueueShotConfigs.bFlushQueue = doFlush ? OMX_TRUE : OMX_FALSE;
        enqueueShotConfigs.nNumConfigs = i;
        eError =  OMX_SetConfig(mCameraAdapterParameters.mHandleComp,
                        ( OMX_INDEXTYPE ) OMX_TI_IndexConfigEnqueueShotConfigs,
                            &enqueueShotConfigs);
        if ( OMX_ErrorNone != eError ) {
            CAMHAL_LOGEB("Error while configuring enqueue shot 0x%x", eError);
            goto exit;
        } else {
            CAMHAL_LOGDA("Enqueue shot configured successfully");
        }
        // Flush only first time
        doFlush = false;
    }

    // Handle burst capture (no any bracketing) case
    if (0 == evCount) {
        CAMHAL_LOGE("Handle burst capture (no any bracketing) case");
        enqueueShotConfigs.nShotConfig[0].nConfigId = 0;
        enqueueShotConfigs.nShotConfig[0].nFrames = frameCount;
        enqueueShotConfigs.nShotConfig[0].nEC = 0;
        enqueueShotConfigs.nShotConfig[0].nExp = 0;
        enqueueShotConfigs.nShotConfig[0].nGain = 0;
        enqueueShotConfigs.nShotConfig[0].eExpGainApplyMethod = OMX_TI_EXPGAINAPPLYMETHOD_RELATIVE;
        enqueueShotConfigs.nShotConfig[0].bNoSnapshot = OMX_FALSE; // TODO: Make this configurable
        enqueueShotConfigs.nNumConfigs = 1;
        enqueueShotConfigs.nPortIndex = mCameraAdapterParameters.mImagePortIndex;
        enqueueShotConfigs.bFlushQueue = doFlush ? OMX_TRUE : OMX_FALSE;
        eError =  OMX_SetConfig(mCameraAdapterParameters.mHandleComp,
                        ( OMX_INDEXTYPE ) OMX_TI_IndexConfigEnqueueShotConfigs,
                            &enqueueShotConfigs);
        if ( OMX_ErrorNone != eError ) {
            CAMHAL_LOGEB("Error while configuring enqueue shot 0x%x", eError);
            goto exit;
        } else {
            CAMHAL_LOGDA("Enqueue shot configured successfully");
        }
    }

 exit:
    LOG_FUNCTION_NAME_EXIT;

    return (ret | Utils::ErrorUtils::omxToAndroidError(eError));
}

status_t OMXCameraAdapter::setExposureBracketing(int *evValues,
                                                 int *evValues2,
                                                 size_t evCount,
                                                 size_t frameCount,
                                                 OMX_BRACKETMODETYPE bracketMode)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONFIG_CAPTUREMODETYPE expCapMode;
    OMX_CONFIG_EXTCAPTUREMODETYPE extExpCapMode;

    LOG_FUNCTION_NAME;

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
            extExpCapMode.tBracketConfigType.eBracketMode = bracketMode;
            extExpCapMode.tBracketConfigType.nNbrBracketingValues = evCount - 1;
            }

        for ( unsigned int i = 0 ; i < evCount ; i++ )
            {
            if (bracketMode == OMX_BracketExposureGainAbsolute) {
                extExpCapMode.tBracketConfigType.nBracketValues[i]  =  evValues[i];
                extExpCapMode.tBracketConfigType.nBracketValues2[i]  =  evValues2[i];
            } else {
                // assuming OMX_BracketExposureRelativeInEV
                extExpCapMode.tBracketConfigType.nBracketValues[i]  =  ( evValues[i] * ( 1 << Q16_OFFSET ) )  / 10;
            }
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
        CameraBuffer *buffer = (CameraBuffer *)pBuffHeader->pAppPrivate;
        currentBufferIdx = buffer->index;

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
            setFrameRefCount((CameraBuffer *)imgCaptureData->mBufferHeader[nextBufferIdx]->pAppPrivate, typeOfFrame, 1);
            returnFrame((CameraBuffer *)imgCaptureData->mBufferHeader[nextBufferIdx]->pAppPrivate, typeOfFrame);
            }
        }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::sendBracketFrames(size_t &framesSent)
{
    status_t ret = NO_ERROR;
    int currentBufferIdx;
    OMXCameraPortParameters * imgCaptureData = NULL;

    LOG_FUNCTION_NAME;

    imgCaptureData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mImagePortIndex];
    framesSent = 0;

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
                framesSent++;
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
        android::AutoMutex lock(mBracketingLock);

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
        android::AutoMutex lock(mBracketingLock);

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
            mBurstFramesAccum = imgCaptureData->mNumBufs;
            mLastBracetingBufferIdx = mBracketingBuffersQueuedCount - 1;

            for ( int i = 0 ; i  < imgCaptureData->mNumBufs ; i++ )
                {
                mBracketingBuffersQueued[i] = true;
                }

            }
        }

    if ( NO_ERROR == ret )
        {
        CachedCaptureParameters* cap_params = cacheCaptureParameters();
        ret = startImageCapture(true, cap_params);
        delete cap_params;
            {
            android::AutoMutex lock(mBracketingLock);

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

    ret = stopImageCapture();

    android::AutoMutex lock(mBracketingLock);

    if ( NULL != mBracketingBuffersQueued )
    {
        delete [] mBracketingBuffersQueued;
    }

    mBracketingBuffersQueued = NULL;
    mBracketingEnabled = false;
    mBracketingBuffersQueuedCount = 0;
    mLastBracetingBufferIdx = 0;

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::startImageCapture(bool bracketing, CachedCaptureParameters* capParams)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMXCameraPortParameters * capData = NULL;
    OMX_CONFIG_BOOLEANTYPE bOMX;
    size_t bracketingSent = 0;

    LOG_FUNCTION_NAME;

    android::AutoMutex lock(mImageCaptureLock);

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

    if ( !bracketing ) {
        if ((getNextState() & (CAPTURE_ACTIVE|BRACKETING_ACTIVE)) == 0) {
            CAMHAL_LOGDA("trying starting capture when already canceled");
            return NO_ERROR;
        }
    }

    if (!capParams) {
        CAMHAL_LOGE("Invalid cached parameters sent!");
        return BAD_VALUE;
    }

    // Camera framework doesn't expect face callbacks once capture is triggered
    pauseFaceDetection(true);

    //During bracketing image capture is already active
    {
    android::AutoMutex lock(mBracketingLock);
    if ( mBracketingEnabled )
        {
        //Stop bracketing, activate normal burst for the remaining images
        mBracketingEnabled = false;
        ret = sendBracketFrames(bracketingSent);

        // Check if we accumulated enough buffers
        if ( bracketingSent < ( mBracketingRange - 1 ) )
            {
            mCapturedFrames = mBracketingRange + ( ( mBracketingRange - 1 ) - bracketingSent );
            }
        else
            {
            mCapturedFrames = mBracketingRange;
            }
        mBurstFramesQueued = 0;
        mBurstFramesAccum = mCapturedFrames;

        if(ret != NO_ERROR)
            goto EXIT;
        else
            return ret;
        }
    }

    if ( NO_ERROR == ret ) {
        if (capParams->mPendingCaptureSettings & SetRotation) {
            mPendingCaptureSettings &= ~SetRotation;
            ret = setPictureRotation(mPictureRotation);
            if ( NO_ERROR != ret ) {
                CAMHAL_LOGEB("Error configuring image rotation %x", ret);
            }
        }

        if (capParams->mPendingCaptureSettings & SetBurstExpBracket) {
            mPendingCaptureSettings &= ~SetBurstExpBracket;
            if ( mBracketingSet ) {
                ret = doExposureBracketing(capParams->mExposureBracketingValues,
                                            capParams->mExposureGainBracketingValues,
                                            capParams->mExposureGainBracketingModes,
                                            0,
                                            0,
                                            capParams->mFlushShotConfigQueue,
                                            capParams->mExposureBracketMode);
            } else {
                ret = doExposureBracketing(capParams->mExposureBracketingValues,
                                    capParams->mExposureGainBracketingValues,
                                    capParams->mExposureGainBracketingModes,
                                    capParams->mExposureBracketingValidEntries,
                                    capParams->mBurstFrames,
                                    capParams->mFlushShotConfigQueue,
                                    capParams->mExposureBracketMode);
            }

            if ( ret != NO_ERROR ) {
                CAMHAL_LOGEB("setExposureBracketing() failed %d", ret);
                goto EXIT;
            }
        }
    }

#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS
            CameraHal::PPM("startImageCapture bracketing configs done: ", &mStartCapture);
#endif

    capData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mImagePortIndex];

    //OMX shutter callback events are only available in hq mode
    if ( (HIGH_QUALITY == mCapMode) || (HIGH_QUALITY_ZSL== mCapMode)) {
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

    if (mPending3Asettings) {
        apply3Asettings(mParameters3A);
    }

    if (ret == NO_ERROR) {
        int index = 0;
        int queued = 0;
        android::AutoMutex lock(mBurstLock);

        if (capParams->mFlushShotConfigQueue) {
            // reset shot queue
            mCapturedFrames = mBurstFrames;
            mBurstFramesAccum = mBurstFrames;
            mBurstFramesQueued = 0;
            for ( int index = 0 ; index < capData->mNumBufs ; index++ ) {
                if (OMXCameraPortParameters::FILL == capData->mStatus[index]) {
                    mBurstFramesQueued++;
                }
            }
        } else {
            mCapturedFrames += mBurstFrames;
            mBurstFramesAccum += mBurstFrames;
        }
        CAMHAL_LOGD("mBurstFramesQueued = %d mBurstFramesAccum = %d index = %d "
                    "capData->mNumBufs = %d queued = %d capData->mMaxQueueable = %d",
                    mBurstFramesQueued,mBurstFramesAccum,index,
                    capData->mNumBufs,queued,capData->mMaxQueueable);
        CAMHAL_LOGD("%d", (mBurstFramesQueued < mBurstFramesAccum)
                          && (index < capData->mNumBufs)
                          && (queued < capData->mMaxQueueable));
        while ((mBurstFramesQueued < mBurstFramesAccum) &&
               (index < capData->mNumBufs) &&
               (queued < capData->mMaxQueueable)) {
            if (capData->mStatus[index] == OMXCameraPortParameters::IDLE) {
                CAMHAL_LOGDB("Queuing buffer on Capture port - %p",
                             capData->mBufferHeader[index]->pBuffer);
                capData->mStatus[index] = OMXCameraPortParameters::FILL;
                eError = OMX_FillThisBuffer(mCameraAdapterParameters.mHandleComp,
                            (OMX_BUFFERHEADERTYPE*)capData->mBufferHeader[index]);
                GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);
                mBurstFramesQueued++;
                queued++;
            } else if (OMXCameraPortParameters::FILL == capData->mStatus[index]) {
               CAMHAL_LOGE("Not queueing index = %d", index);
                queued++;
            }
            index++;
        }

#ifdef CAMERAHAL_USE_RAW_IMAGE_SAVING
        if (mRawCapture) {
            capData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mVideoPortIndex];

            ///Queue all the buffers on capture port
            for ( int index = 0 ; index < capData->mNumBufs ; index++ ) {
                CAMHAL_LOGDB("Queuing buffer on Video port (for RAW capture) - 0x%x", ( unsigned int ) capData->mBufferHeader[index]->pBuffer);
                capData->mStatus[index] = OMXCameraPortParameters::FILL;
                eError = OMX_FillThisBuffer(mCameraAdapterParameters.mHandleComp,
                        (OMX_BUFFERHEADERTYPE*)capData->mBufferHeader[index]);

                GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);
            }
        }
#endif

        mWaitingForSnapshot = true;
        mCaptureSignalled = false;

        // Capturing command is not needed when capturing in video mode
        // Only need to queue buffers on image ports
        if ( ( mCapMode != VIDEO_MODE ) && ( mCapMode != VIDEO_MODE_HQ ) ) {
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

#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS
            CameraHal::PPM("startImageCapture image buffers queued and capture enabled: ", &mStartCapture);
#endif

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

#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS
            CameraHal::PPM("startImageCapture shutter event received: ", &mStartCapture);
#endif

    return (ret | Utils::ErrorUtils::omxToAndroidError(eError));

EXIT:
    CAMHAL_LOGEB("Exiting function %s because of ret %d eError=%x", __FUNCTION__, ret, eError);
    mWaitingForSnapshot = false;
    mCaptureSignalled = false;
    performCleanupAfterError();
    LOG_FUNCTION_NAME_EXIT;
    return (ret | Utils::ErrorUtils::omxToAndroidError(eError));
}

status_t OMXCameraAdapter::stopImageCapture()
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONFIG_BOOLEANTYPE bOMX;
    OMXCameraPortParameters *imgCaptureData = NULL;

    LOG_FUNCTION_NAME;

    android::AutoMutex lock(mImageCaptureLock);

    if (!mCaptureConfigured) {
        //Capture is not ongoing, return from here
        return NO_ERROR;
    }

    if ( 0 != mStopCaptureSem.Count() ) {
        CAMHAL_LOGEB("Error mStopCaptureSem semaphore count %d", mStopCaptureSem.Count());
        goto EXIT;
    }

    // TODO(XXX): Reprocessing is currently piggy-backing capture commands
    if (mAdapterState == REPROCESS_STATE) {
        ret = stopReprocess();
    }

    //Disable the callback first
    mWaitingForSnapshot = false;

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
    } else if (CP_CAM == mCapMode) {
        // Reset shot config queue
        OMX_TI_CONFIG_ENQUEUESHOTCONFIGS resetShotConfigs;
        OMX_INIT_STRUCT_PTR(&resetShotConfigs, OMX_TI_CONFIG_ENQUEUESHOTCONFIGS);

        resetShotConfigs.nPortIndex = mCameraAdapterParameters.mImagePortIndex;
        resetShotConfigs.bFlushQueue = OMX_TRUE;
        resetShotConfigs.nNumConfigs = 0;
        eError =  OMX_SetConfig(mCameraAdapterParameters.mHandleComp,
                        ( OMX_INDEXTYPE ) OMX_TI_IndexConfigEnqueueShotConfigs,
                            &resetShotConfigs);
        if ( OMX_ErrorNone != eError ) {
            CAMHAL_LOGEB("Error while reset shot config 0x%x", eError);
            goto EXIT;
        } else {
            CAMHAL_LOGDA("Shot config reset successfully");
        }
    }

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
    if ( ( mCapMode != VIDEO_MODE ) && ( mCapMode != VIDEO_MODE_HQ ) ) {
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

    CAMHAL_LOGDB("Capture set - 0x%x", eError);

    mCaptureSignalled = true; //set this to true if we exited because of timeout

    {
        android::AutoMutex lock(mFrameCountMutex);
        mFrameCount = 0;
        mFirstFrameCondition.broadcast();
    }

    // Stop is always signalled externally in CPCAM mode
    // We need to make sure we really stop
    if ((mCapMode == CP_CAM)) {
        disableReprocess();
        disableImagePort();
        if ( NULL != mReleaseImageBuffersCallback ) {
            mReleaseImageBuffersCallback(mReleaseData);
        }
    }

    // Moving code for below commit here as an optimization for continuous capture,
    // so focus settings don't have to reapplied after each capture
    // c78fa2a CameraHAL: Always reset focus mode after capture
    // Workaround when doing many consecutive shots, CAF wasn't getting restarted.
    mPending3Asettings |= SetFocus;

    mCapturedFrames = 0;
    mBurstFramesAccum = 0;
    mBurstFramesQueued = 0;

    return (ret | Utils::ErrorUtils::omxToAndroidError(eError));

EXIT:
    CAMHAL_LOGEB("Exiting function %s because of ret %d eError=%x", __FUNCTION__, ret, eError);
    //Release image buffers
    if ( NULL != mReleaseImageBuffersCallback ) {
        mReleaseImageBuffersCallback(mReleaseData);
    }

    {
        android::AutoMutex lock(mFrameCountMutex);
        mFrameCount = 0;
        mFirstFrameCondition.broadcast();
    }

    performCleanupAfterError();
    LOG_FUNCTION_NAME_EXIT;
    return (ret | Utils::ErrorUtils::omxToAndroidError(eError));
}

status_t OMXCameraAdapter::disableImagePort(){
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMXCameraPortParameters *imgCaptureData = NULL;
    OMXCameraPortParameters *imgRawCaptureData = NULL;

    if (!mCaptureConfigured) {
        return NO_ERROR;
    }

    mCaptureConfigured = false;
    imgCaptureData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mImagePortIndex];
    imgRawCaptureData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mVideoPortIndex]; // for RAW capture

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

    deinitInternalBuffers(mCameraAdapterParameters.mImagePortIndex);

    // since port settings are not persistent after port is disabled...
    mPendingCaptureSettings |= SetFormat;

#ifdef CAMERAHAL_USE_RAW_IMAGE_SAVING

    if (mRawCapture) {
        ///Register for Video port Disable event
        ret = RegisterForEvent(mCameraAdapterParameters.mHandleComp,
                OMX_EventCmdComplete,
                OMX_CommandPortDisable,
                mCameraAdapterParameters.mVideoPortIndex,
                mStopCaptureSem);
        ///Disable RawCapture Port
        eError = OMX_SendCommand(mCameraAdapterParameters.mHandleComp,
                OMX_CommandPortDisable,
                mCameraAdapterParameters.mVideoPortIndex,
                NULL);

        GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);

        ///Free all the buffers on RawCapture port
        if (imgRawCaptureData) {
            CAMHAL_LOGDB("Freeing buffer on Capture port - %d", imgRawCaptureData->mNumBufs);
            for ( int index = 0 ; index < imgRawCaptureData->mNumBufs ; index++) {
                CAMHAL_LOGDB("Freeing buffer on Capture port - 0x%x", ( unsigned int ) imgRawCaptureData->mBufferHeader[index]->pBuffer);
                eError = OMX_FreeBuffer(mCameraAdapterParameters.mHandleComp,
                        mCameraAdapterParameters.mVideoPortIndex,
                        (OMX_BUFFERHEADERTYPE*)imgRawCaptureData->mBufferHeader[index]);

                GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);
            }
        }
        CAMHAL_LOGDA("Waiting for Video port disable");
        //Wait for the image port enable event
        mStopCaptureSem.WaitTimeout(OMX_CMD_TIMEOUT);
        CAMHAL_LOGDA("Video Port disabled");
    }
#endif

EXIT:
    return (ret | Utils::ErrorUtils::omxToAndroidError(eError));
}

status_t OMXCameraAdapter::initInternalBuffers(OMX_U32 portIndex)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    int index = 0;
    OMX_TI_PARAM_USEBUFFERDESCRIPTOR bufferdesc;

    /* Indicate to Ducati that we're planning to use dynamically-mapped buffers */
    OMX_INIT_STRUCT_PTR (&bufferdesc, OMX_TI_PARAM_USEBUFFERDESCRIPTOR);
    bufferdesc.nPortIndex = portIndex;
    bufferdesc.bEnabled = OMX_FALSE;
    bufferdesc.eBufferType = OMX_TI_BufferTypePhysicalPageList;

    eError = OMX_SetParameter(mCameraAdapterParameters.mHandleComp,
            (OMX_INDEXTYPE) OMX_TI_IndexUseBufferDescriptor,
            &bufferdesc);
    if (eError!=OMX_ErrorNone) {
        CAMHAL_LOGEB("OMX_SetParameter - %x", eError);
        return -EINVAL;
    }

    CAMHAL_LOGDA("Initializing internal buffers");
    do {
        OMX_TI_PARAM_COMPONENTBUFALLOCTYPE bufferalloc;
        OMX_TI_PARAM_COMPONENTBUFALLOCTYPE bufferallocset;
        OMX_INIT_STRUCT_PTR (&bufferalloc, OMX_TI_PARAM_COMPONENTBUFALLOCTYPE);
        bufferalloc.nPortIndex = portIndex;
        bufferalloc.nIndex = index;

        eError = OMX_GetParameter (mCameraAdapterParameters.mHandleComp,
                (OMX_INDEXTYPE)OMX_TI_IndexParamComponentBufferAllocation,
                &bufferalloc);
        if (eError == OMX_ErrorNoMore) {
            return NO_ERROR;
        }
        if (eError != OMX_ErrorNone) {
            CAMHAL_LOGE("GetParameter failed error = 0x%x", eError);
            break;
        }

        CAMHAL_LOGDB("Requesting buftype %d of size %dx%d",
            (int)bufferalloc.eBufType, (int)bufferalloc.nAllocWidth,
            (int)bufferalloc.nAllocLines);

        bufferalloc.eBufType = OMX_TI_BufferTypeHardwareReserved1D;

        OMX_INIT_STRUCT_PTR (&bufferallocset, OMX_TI_PARAM_COMPONENTBUFALLOCTYPE);
        bufferallocset.nPortIndex = portIndex;
        bufferallocset.nIndex = index;
        bufferallocset.eBufType = OMX_TI_BufferTypeHardwareReserved1D;
        bufferallocset.nAllocWidth = bufferalloc.nAllocWidth;
        bufferallocset.nAllocLines = bufferalloc.nAllocLines;

        eError = OMX_SetParameter (mCameraAdapterParameters.mHandleComp,
                (OMX_INDEXTYPE)OMX_TI_IndexParamComponentBufferAllocation,
                &bufferallocset);
        if (eError != OMX_ErrorNone) {
            CAMHAL_LOGE("SetParameter failed, error=%08x", eError);
            if (eError == OMX_ErrorNoMore) return NO_ERROR;
            break;
        }

        index++;

        /* 1 is an arbitrary limit */
    } while (index < 1);

    CAMHAL_LOGV("Ducati requested too many (>1) internal buffers");

    return -EINVAL;
}

status_t OMXCameraAdapter::deinitInternalBuffers(OMX_U32 portIndex)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_TI_PARAM_USEBUFFERDESCRIPTOR bufferdesc;

    OMX_INIT_STRUCT_PTR (&bufferdesc, OMX_TI_PARAM_USEBUFFERDESCRIPTOR);
    bufferdesc.nPortIndex = portIndex;
    bufferdesc.bEnabled = OMX_FALSE;
    bufferdesc.eBufferType = OMX_TI_BufferTypeDefault;

    eError = OMX_SetParameter(mCameraAdapterParameters.mHandleComp,
            (OMX_INDEXTYPE) OMX_TI_IndexUseBufferDescriptor,
            &bufferdesc);
    if (eError!=OMX_ErrorNone) {
        CAMHAL_LOGEB("OMX_SetParameter - %x", eError);
        return -EINVAL;
    }

    OMX_TI_PARAM_COMPONENTBUFALLOCTYPE bufferalloc;
    OMX_INIT_STRUCT_PTR (&bufferalloc, OMX_TI_PARAM_COMPONENTBUFALLOCTYPE);
    bufferalloc.nPortIndex = portIndex;
    bufferalloc.eBufType = OMX_TI_BufferTypeDefault;
    bufferalloc.nAllocWidth = 1;
    bufferalloc.nAllocLines = 1;
    eError = OMX_SetParameter(mCameraAdapterParameters.mHandleComp,
            (OMX_INDEXTYPE) OMX_TI_IndexParamComponentBufferAllocation,
            &bufferalloc);
    if (eError!=OMX_ErrorNone) {
        CAMHAL_LOGEB("OMX_SetParameter - %x", eError);
        return -EINVAL;
    }

    return Utils::ErrorUtils::omxToAndroidError(eError);
}

status_t OMXCameraAdapter::UseBuffersCapture(CameraBuffer * bufArr, int num)
{
    LOG_FUNCTION_NAME;

    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMXCameraPortParameters * imgCaptureData = NULL;
    OMXCameraPortParameters cap;

    imgCaptureData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mImagePortIndex];

    if ( 0 != mUseCaptureSem.Count() )
        {
        CAMHAL_LOGEB("Error mUseCaptureSem semaphore count %d", mUseCaptureSem.Count());
        return BAD_VALUE;
        }

    CAMHAL_ASSERT(num > 0);

    // if some setting that requires a SetParameter (including
    // changing buffer types) then we need to disable the port
    // before being allowed to apply the settings
    if ((mPendingCaptureSettings & ECaptureParamSettings) ||
            bufArr[0].type != imgCaptureData->mBufferType ||
            imgCaptureData->mNumBufs != num) {
        if (mCaptureConfigured) {
            disableImagePort();
            if ( NULL != mReleaseImageBuffersCallback ) {
                mReleaseImageBuffersCallback(mReleaseData);
            }
        }

        imgCaptureData->mBufferType = bufArr[0].type;
        imgCaptureData->mNumBufs = num;

        CAMHAL_LOGDB("Params Width = %d", (int)imgCaptureData->mWidth);
        CAMHAL_LOGDB("Params Height = %d", (int)imgCaptureData->mHeight);

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

        if (mPendingCaptureSettings & SetQuality) {
            mPendingCaptureSettings &= ~SetQuality;
            ret = setImageQuality(mPictureQuality);
            if ( NO_ERROR != ret) {
                CAMHAL_LOGEB("Error configuring image quality %x", ret);
                goto EXIT;
            }
        }

        // Configure DOMX to use either gralloc handles or vptrs
        {
            OMX_TI_PARAMUSENATIVEBUFFER domxUseGrallocHandles;
            OMX_INIT_STRUCT_PTR (&domxUseGrallocHandles, OMX_TI_PARAMUSENATIVEBUFFER);

            domxUseGrallocHandles.nPortIndex = mCameraAdapterParameters.mImagePortIndex;
            if (bufArr[0].type == CAMERA_BUFFER_ANW) {
                CAMHAL_LOGD ("Using ANW Buffers");
                initInternalBuffers(mCameraAdapterParameters.mImagePortIndex);
                domxUseGrallocHandles.bEnable = OMX_TRUE;
            } else {
                CAMHAL_LOGD ("Using ION Buffers");
                domxUseGrallocHandles.bEnable = OMX_FALSE;
            }

            eError = OMX_SetParameter(mCameraAdapterParameters.mHandleComp,
                                    (OMX_INDEXTYPE)OMX_TI_IndexUseNativeBuffers, &domxUseGrallocHandles);
            if (eError!=OMX_ErrorNone) {
                CAMHAL_LOGEB("OMX_SetParameter - %x", eError);
            }
            GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);
        }

#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS

    CameraHal::PPM("Takepicture image port configuration: ", &bufArr->ppmStamp);

#endif

        // Register for Image port ENABLE event
        ret = RegisterForEvent(mCameraAdapterParameters.mHandleComp,
                               OMX_EventCmdComplete,
                               OMX_CommandPortEnable,
                               mCameraAdapterParameters.mImagePortIndex,
                               mUseCaptureSem);

        // Enable Capture Port
        eError = OMX_SendCommand(mCameraAdapterParameters.mHandleComp,
                                 OMX_CommandPortEnable,
                                 mCameraAdapterParameters.mImagePortIndex,
                                 NULL);

        CAMHAL_LOGDB("OMX_UseBuffer = 0x%x", eError);
        GOTO_EXIT_IF(( eError != OMX_ErrorNone ), eError);

        for (int index = 0 ; index < imgCaptureData->mNumBufs ; index++) {
            OMX_BUFFERHEADERTYPE *pBufferHdr;
            CAMHAL_LOGDB("OMX_UseBuffer Capture address: 0x%x, size = %d",
                         (unsigned int)bufArr[index].opaque,
                         (int)imgCaptureData->mBufSize);

            eError = OMX_UseBuffer(mCameraAdapterParameters.mHandleComp,
                                   &pBufferHdr,
                                   mCameraAdapterParameters.mImagePortIndex,
                                   0,
                                   imgCaptureData->mBufSize,
                                   (OMX_U8*)camera_buffer_get_omx_ptr(&bufArr[index]));

            CAMHAL_LOGDB("OMX_UseBuffer = 0x%x", eError);
            GOTO_EXIT_IF(( eError != OMX_ErrorNone ), eError);

            pBufferHdr->pAppPrivate = (OMX_PTR) &bufArr[index];
            bufArr[index].index = index;
            pBufferHdr->nSize = sizeof(OMX_BUFFERHEADERTYPE);
            pBufferHdr->nVersion.s.nVersionMajor = 1 ;
            pBufferHdr->nVersion.s.nVersionMinor = 1 ;
            pBufferHdr->nVersion.s.nRevision = 0;
            pBufferHdr->nVersion.s.nStep =  0;
            imgCaptureData->mBufferHeader[index] = pBufferHdr;
            imgCaptureData->mStatus[index] = OMXCameraPortParameters::IDLE;
        }

        // Wait for the image port enable event
        CAMHAL_LOGDA("Waiting for port enable");
        ret = mUseCaptureSem.WaitTimeout(OMX_CMD_TIMEOUT);

        // If somethiing bad happened while we wait
        if (mComponentState == OMX_StateInvalid) {
            CAMHAL_LOGEA("Invalid State after Enable Image Port Exitting!!!");
            goto EXIT;
          }

        if (ret != NO_ERROR) {
            ret |= RemoveEvent(mCameraAdapterParameters.mHandleComp,
                               OMX_EventCmdComplete,
                               OMX_CommandPortEnable,
                               mCameraAdapterParameters.mImagePortIndex,
                               NULL);
            CAMHAL_LOGDA("Timeout expired on port enable");
            goto EXIT;
        }
        CAMHAL_LOGDA("Port enabled");

#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS

    CameraHal::PPM("Takepicture image port enabled and buffers registered: ", &bufArr->ppmStamp);

#endif

        if (mNextState != LOADED_REPROCESS_CAPTURE_STATE) {
            // Enable WB and vector shot extra data for metadata
            setExtraData(true, mCameraAdapterParameters.mImagePortIndex, OMX_WhiteBalance);
            setExtraData(true, mCameraAdapterParameters.mImagePortIndex, OMX_TI_LSCTable);
        }

        // CPCam mode only supports vector shot
        // Regular capture is not supported
        if ( (mCapMode == CP_CAM) && (mNextState != LOADED_REPROCESS_CAPTURE_STATE) ) {
            initVectorShot();
        }

        mCaptureBuffersAvailable.clear();
        for (unsigned int i = 0; i < imgCaptureData->mMaxQueueable; i++ ) {
            mCaptureBuffersAvailable.add(&mCaptureBuffers[i], 0);
        }

        // initial ref count for undeqeueued buffers is 1 since buffer provider
        // is still holding on to it
        for (unsigned int i = imgCaptureData->mMaxQueueable; i < imgCaptureData->mNumBufs; i++ ) {
            mCaptureBuffersAvailable.add(&mCaptureBuffers[i], 1);
        }
    }

    if ( NO_ERROR == ret )
        {
        ret = setupEXIF();
        if ( NO_ERROR != ret )
            {
            CAMHAL_LOGEB("Error configuring EXIF Buffer %x", ret);
            }
        }

    // Choose proper single preview mode for cp capture (reproc or hs)
    if (( NO_ERROR == ret) && (OMXCameraAdapter::CP_CAM == mCapMode)) {
        OMX_TI_CONFIG_SINGLEPREVIEWMODETYPE singlePrevMode;
        OMX_INIT_STRUCT_PTR (&singlePrevMode, OMX_TI_CONFIG_SINGLEPREVIEWMODETYPE);
        if (mNextState == LOADED_CAPTURE_STATE) {
            singlePrevMode.eMode = OMX_TI_SinglePreviewMode_ImageCaptureHighSpeed;
        } else if (mNextState == LOADED_REPROCESS_CAPTURE_STATE) {
            singlePrevMode.eMode = OMX_TI_SinglePreviewMode_Reprocess;
        } else {
            CAMHAL_LOGE("Wrong state trying to start a capture in CPCAM mode?");
            singlePrevMode.eMode = OMX_TI_SinglePreviewMode_ImageCaptureHighSpeed;
        }
        eError =  OMX_SetConfig(mCameraAdapterParameters.mHandleComp,
                                (OMX_INDEXTYPE) OMX_TI_IndexConfigSinglePreviewMode,
                                &singlePrevMode);
        if ( OMX_ErrorNone != eError ) {
            CAMHAL_LOGEB("Error while configuring single preview mode 0x%x", eError);
            ret = Utils::ErrorUtils::omxToAndroidError(eError);
        } else {
            CAMHAL_LOGDA("single preview mode configured successfully");
        }
    }

#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS

    CameraHal::PPM("Takepicture extra configs on image port done: ", &bufArr->ppmStamp);

#endif

    mCaptureConfigured = true;

#ifdef CAMERAHAL_USE_RAW_IMAGE_SAVING
    if (mRawCapture) {
        mCaptureConfigured = false;
    }
#endif

    return (ret | Utils::ErrorUtils::omxToAndroidError(eError));

EXIT:
    CAMHAL_LOGEB("Exiting function %s because of ret %d eError=%x", __FUNCTION__, ret, eError);
    setExtraData(false, mCameraAdapterParameters.mImagePortIndex, OMX_WhiteBalance);
    // TODO: WA: if domx client disables VectShotInfo metadata on the image port, this causes
    // VectShotInfo to be disabled internally on preview port also. Remove setting in OMXCapture
    // setExtraData(false, mCameraAdapterParameters.mImagePortIndex, OMX_TI_VectShotInfo);
    setExtraData(false, mCameraAdapterParameters.mImagePortIndex, OMX_TI_LSCTable);
    //Release image buffers
    if ( NULL != mReleaseImageBuffersCallback ) {
        mReleaseImageBuffersCallback(mReleaseData);
    }
    performCleanupAfterError();
    LOG_FUNCTION_NAME_EXIT;
    return (ret | Utils::ErrorUtils::omxToAndroidError(eError));

}
status_t OMXCameraAdapter::UseBuffersRawCapture(CameraBuffer *bufArr, int num)
{
    LOG_FUNCTION_NAME
    status_t ret;
    OMX_ERRORTYPE eError;
    OMXCameraPortParameters * imgRawCaptureData = NULL;
    Utils::Semaphore camSem;
    OMXCameraPortParameters cap;

    imgRawCaptureData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mVideoPortIndex];

    if (mCaptureConfigured) {
        return NO_ERROR;
    }

    camSem.Create();

    // mWaitingForSnapshot is true only when we're in the process of capturing
    if (mWaitingForSnapshot) {
        ///Register for Video port Disable event
        ret = RegisterForEvent(mCameraAdapterParameters.mHandleComp,
                (OMX_EVENTTYPE) OMX_EventCmdComplete,
                OMX_CommandPortDisable,
                mCameraAdapterParameters.mVideoPortIndex,
                camSem);

        ///Disable Capture Port
        eError = OMX_SendCommand(mCameraAdapterParameters.mHandleComp,
                OMX_CommandPortDisable,
                mCameraAdapterParameters.mVideoPortIndex,
                NULL);

        CAMHAL_LOGDA("Waiting for port disable");
        //Wait for the image port enable event
        camSem.Wait();
        CAMHAL_LOGDA("Port disabled");
    }

    imgRawCaptureData->mNumBufs = num;

    CAMHAL_LOGDB("RAW Max sensor width = %d", (int)imgRawCaptureData->mWidth);
    CAMHAL_LOGDB("RAW Max sensor height = %d", (int)imgRawCaptureData->mHeight);

    ret = setFormat(OMX_CAMERA_PORT_VIDEO_OUT_VIDEO, *imgRawCaptureData);

    if (ret != NO_ERROR) {
        CAMHAL_LOGEB("setFormat() failed %d", ret);
        LOG_FUNCTION_NAME_EXIT
        return ret;
    }

    ///Register for Video port ENABLE event
    ret = RegisterForEvent(mCameraAdapterParameters.mHandleComp,
                                (OMX_EVENTTYPE) OMX_EventCmdComplete,
                                OMX_CommandPortEnable,
                                mCameraAdapterParameters.mVideoPortIndex,
                                camSem);

    ///Enable Video Capture Port
    eError = OMX_SendCommand(mCameraAdapterParameters.mHandleComp,
                                OMX_CommandPortEnable,
                                mCameraAdapterParameters.mVideoPortIndex,
                                NULL);

    mCaptureBuffersLength = (int)imgRawCaptureData->mBufSize;
    for ( int index = 0 ; index < imgRawCaptureData->mNumBufs ; index++ ) {
        OMX_BUFFERHEADERTYPE *pBufferHdr;
        CAMHAL_LOGDB("OMX_UseBuffer rawCapture address: 0x%x, size = %d ",
                     (unsigned int)bufArr[index].opaque,
                     (int)imgRawCaptureData->mBufSize );

        eError = OMX_UseBuffer( mCameraAdapterParameters.mHandleComp,
                                &pBufferHdr,
                                mCameraAdapterParameters.mVideoPortIndex,
                                0,
                                mCaptureBuffersLength,
                                (OMX_U8*)camera_buffer_get_omx_ptr(&bufArr[index]));
        if (eError != OMX_ErrorNone) {
            CAMHAL_LOGEB("OMX_UseBuffer = 0x%x", eError);
        }

        GOTO_EXIT_IF(( eError != OMX_ErrorNone ), eError);

        pBufferHdr->pAppPrivate = (OMX_PTR) &bufArr[index];
        bufArr[index].index = index;
        pBufferHdr->nSize = sizeof(OMX_BUFFERHEADERTYPE);
        pBufferHdr->nVersion.s.nVersionMajor = 1 ;
        pBufferHdr->nVersion.s.nVersionMinor = 1 ;
        pBufferHdr->nVersion.s.nRevision = 0;
        pBufferHdr->nVersion.s.nStep =  0;
        imgRawCaptureData->mBufferHeader[index] = pBufferHdr;

    }

    //Wait for the image port enable event
    CAMHAL_LOGDA("Waiting for port enable");
    camSem.Wait();
    CAMHAL_LOGDA("Port enabled");

    if (NO_ERROR == ret) {
        ret = setupEXIF();
        if ( NO_ERROR != ret ) {
            CAMHAL_LOGEB("Error configuring EXIF Buffer %x", ret);
        }
    }

    mCapturedFrames = mBurstFrames;
    mBurstFramesQueued = 0;
    mCaptureConfigured = true;

    EXIT:

    if (eError != OMX_ErrorNone) {
        if ( NULL != mErrorNotifier )
        {
            mErrorNotifier->errorNotify(eError);
        }
    }

    LOG_FUNCTION_NAME_EXIT

    return ret;
}

} // namespace Camera
} // namespace Ti
