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
* @file OMXCameraAdapter.cpp
*
* This file maps the Camera Hardware Interface to OMX.
*
*/

#include "CameraHal.h"
#include "OMXCameraAdapter.h"
#include "ErrorUtils.h"
#include "TICameraParameters.h"
#include <signal.h>
#include <math.h>

#include <cutils/properties.h>
#define UNLIKELY( exp ) (__builtin_expect( (exp) != 0, false ))
static int mDebugFps = 0;
static int mDebugFcs = 0;

#undef TRUE
#undef FALSE

#define HERE(Msg) {CAMHAL_LOGEB("--===line %d, %s===--\n", __LINE__, Msg);}

namespace android {

#undef LOG_TAG
///Maintain a separate tag for OMXCameraAdapter logs to isolate issues OMX specific
#define LOG_TAG "CameraHAL"

//frames skipped before recalculating the framerate
#define FPS_PERIOD 30

Mutex gAdapterLock;
/*--------------------Camera Adapter Class STARTS here-----------------------------*/

status_t OMXCameraAdapter::initialize(CameraProperties::Properties* caps)
{
    LOG_FUNCTION_NAME;

    char value[PROPERTY_VALUE_MAX];
    property_get("debug.camera.showfps", value, "0");
    mDebugFps = atoi(value);
    property_get("debug.camera.framecounts", value, "0");
    mDebugFcs = atoi(value);

    TIMM_OSAL_ERRORTYPE osalError = OMX_ErrorNone;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    status_t ret = NO_ERROR;


    mLocalVersionParam.s.nVersionMajor = 0x1;
    mLocalVersionParam.s.nVersionMinor = 0x1;
    mLocalVersionParam.s.nRevision = 0x0 ;
    mLocalVersionParam.s.nStep =  0x0;

    mPending3Asettings = 0;//E3AsettingsAll;
    mPendingCaptureSettings = 0;

    if ( 0 != mInitSem.Count() )
        {
        CAMHAL_LOGEB("Error mInitSem semaphore count %d", mInitSem.Count());
        LOG_FUNCTION_NAME_EXIT;
        return NO_INIT;
        }

    ///Update the preview and image capture port indexes
    mCameraAdapterParameters.mPrevPortIndex = OMX_CAMERA_PORT_VIDEO_OUT_PREVIEW;
    // temp changed in order to build OMX_CAMERA_PORT_VIDEO_OUT_IMAGE;
    mCameraAdapterParameters.mImagePortIndex = OMX_CAMERA_PORT_IMAGE_OUT_IMAGE;
    mCameraAdapterParameters.mMeasurementPortIndex = OMX_CAMERA_PORT_VIDEO_OUT_MEASUREMENT;
    //currently not supported use preview port instead
    mCameraAdapterParameters.mVideoPortIndex = OMX_CAMERA_PORT_VIDEO_OUT_PREVIEW;

    eError = OMX_Init();
    if (eError != OMX_ErrorNone) {
        CAMHAL_LOGEB("OMX_Init() failed, error: 0x%x", eError);
        return ErrorUtils::omxToAndroidError(eError);
    }
    mOmxInitialized = true;

    ///Get the handle to the OMX Component
    eError = OMXCameraAdapter::OMXCameraGetHandle(&mCameraAdapterParameters.mHandleComp, (OMX_PTR)this);
    if(eError != OMX_ErrorNone) {
        CAMHAL_LOGEB("OMX_GetHandle -0x%x", eError);
    }
    GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

    mComponentState = OMX_StateLoaded;

    CAMHAL_LOGVB("OMX_GetHandle -0x%x sensor_index = %lu", eError, mSensorIndex);
    eError = OMX_SendCommand(mCameraAdapterParameters.mHandleComp,
                                  OMX_CommandPortDisable,
                                  OMX_ALL,
                                  NULL);

    if(eError != OMX_ErrorNone) {
         CAMHAL_LOGEB("OMX_SendCommand(OMX_CommandPortDisable) -0x%x", eError);
    }
    GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

    // Register for port enable event
    ret = RegisterForEvent(mCameraAdapterParameters.mHandleComp,
                                 OMX_EventCmdComplete,
                                 OMX_CommandPortEnable,
                                 mCameraAdapterParameters.mPrevPortIndex,
                                 mInitSem);
    if(ret != NO_ERROR) {
         CAMHAL_LOGEB("Error in registering for event %d", ret);
         goto EXIT;
    }

    // Enable PREVIEW Port
    eError = OMX_SendCommand(mCameraAdapterParameters.mHandleComp,
                                 OMX_CommandPortEnable,
                                 mCameraAdapterParameters.mPrevPortIndex,
                                 NULL);
    if(eError != OMX_ErrorNone) {
        CAMHAL_LOGEB("OMX_SendCommand(OMX_CommandPortEnable) -0x%x", eError);
    }
    GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);

    // Wait for the port enable event to occur
    ret = mInitSem.WaitTimeout(OMX_CMD_TIMEOUT);
    if ( NO_ERROR == ret ) {
         CAMHAL_LOGDA("-Port enable event arrived");
    } else {
         ret |= RemoveEvent(mCameraAdapterParameters.mHandleComp,
                            OMX_EventCmdComplete,
                            OMX_CommandPortEnable,
                            mCameraAdapterParameters.mPrevPortIndex,
                            NULL);
         CAMHAL_LOGEA("Timeout for enabling preview port expired!");
         goto EXIT;
     }

    // Select the sensor
    OMX_CONFIG_SENSORSELECTTYPE sensorSelect;
    OMX_INIT_STRUCT_PTR (&sensorSelect, OMX_CONFIG_SENSORSELECTTYPE);
    sensorSelect.eSensor = (OMX_SENSORSELECT) mSensorIndex;
    eError = OMX_SetConfig(mCameraAdapterParameters.mHandleComp, ( OMX_INDEXTYPE ) OMX_TI_IndexConfigSensorSelect, &sensorSelect);
    if ( OMX_ErrorNone != eError ) {
        CAMHAL_LOGEB("Error while selecting the sensor index as %d - 0x%x", mSensorIndex, eError);
        return BAD_VALUE;
    } else {
        CAMHAL_LOGDB("Sensor %d selected successfully", mSensorIndex);
    }

    printComponentVersion(mCameraAdapterParameters.mHandleComp);

    mBracketingEnabled = false;
    mBracketingBuffersQueuedCount = 0;
    mBracketingRange = 1;
    mLastBracetingBufferIdx = 0;
    mOMXStateSwitch = false;

    mCaptureSignalled = false;
    mCaptureConfigured = false;
    mRecording = false;
    mWaitingForSnapshot = false;
    mSnapshotCount = 0;

    mCapMode = HIGH_QUALITY;
    mIPP = IPP_NULL;
    mVstabEnabled = false;
    mVnfEnabled = false;
    mBurstFrames = 1;
    mCapturedFrames = 0;
    mPictureQuality = 100;
    mCurrentZoomIdx = 0;
    mTargetZoomIdx = 0;
    mPreviousZoomIndx = 0;
    mReturnZoomStatus = false;
    mZoomInc = 1;
    mZoomParameterIdx = 0;
    mExposureBracketingValidEntries = 0;
    mSensorOverclock = false;
    mIternalRecordingHint = false;

    mDeviceOrientation = 0;
    mCapabilities = caps;
    mZoomUpdating = false;
    mZoomUpdate = false;

    mEXIFData.mGPSData.mAltitudeValid = false;
    mEXIFData.mGPSData.mDatestampValid = false;
    mEXIFData.mGPSData.mLatValid = false;
    mEXIFData.mGPSData.mLongValid = false;
    mEXIFData.mGPSData.mMapDatumValid = false;
    mEXIFData.mGPSData.mProcMethodValid = false;
    mEXIFData.mGPSData.mVersionIdValid = false;
    mEXIFData.mGPSData.mTimeStampValid = false;
    mEXIFData.mModelValid = false;
    mEXIFData.mMakeValid = false;

    // initialize command handling thread
    if(mCommandHandler.get() == NULL)
        mCommandHandler = new CommandHandler(this);

    if ( NULL == mCommandHandler.get() )
    {
        CAMHAL_LOGEA("Couldn't create command handler");
        return NO_MEMORY;
    }

    ret = mCommandHandler->run("CallbackThread", PRIORITY_URGENT_DISPLAY);
    if ( ret != NO_ERROR )
    {
        if( ret == INVALID_OPERATION){
            CAMHAL_LOGDA("command handler thread already runnning!!");
            ret = NO_ERROR;
        } else
        {
            CAMHAL_LOGEA("Couldn't run command handlerthread");
            return ret;
        }
    }

    // initialize omx callback handling thread
    if(mOMXCallbackHandler.get() == NULL)
        mOMXCallbackHandler = new OMXCallbackHandler(this);

    if ( NULL == mOMXCallbackHandler.get() )
    {
        CAMHAL_LOGEA("Couldn't create omx callback handler");
        return NO_MEMORY;
    }

    ret = mOMXCallbackHandler->run("OMXCallbackThread", PRIORITY_URGENT_DISPLAY);
    if ( ret != NO_ERROR )
    {
        if( ret == INVALID_OPERATION){
            CAMHAL_LOGDA("omx callback handler thread already runnning!!");
            ret = NO_ERROR;
        }else
        {
            CAMHAL_LOGEA("Couldn't run omx callback handler thread");
            return ret;
        }
    }

    //Remove any unhandled events
    if (!mEventSignalQ.isEmpty()) {
        for (unsigned int i = 0 ;i < mEventSignalQ.size(); i++ ) {
            TIUTILS::Message *msg = mEventSignalQ.itemAt(i);
            //remove from queue and free msg
            if ( NULL != msg ) {
                free(msg);
            }
        }
        mEventSignalQ.clear();
    }

    OMX_INIT_STRUCT_PTR (&mRegionPriority, OMX_TI_CONFIG_3A_REGION_PRIORITY);
    OMX_INIT_STRUCT_PTR (&mFacePriority, OMX_TI_CONFIG_3A_FACE_PRIORITY);
    mRegionPriority.nPortIndex = OMX_ALL;
    mFacePriority.nPortIndex = OMX_ALL;

    //Setting this flag will that the first setParameter call will apply all 3A settings
    //and will not conditionally apply based on current values.
    mFirstTimeInit = true;

    memset(mExposureBracketingValues, 0, EXP_BRACKET_RANGE*sizeof(int));
    mMeasurementEnabled = false;
    mFaceDetectionRunning = false;
    mFaceDetectionPaused = false;
    mFDSwitchAlgoPriority = false;

    memset(&mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mImagePortIndex], 0, sizeof(OMXCameraPortParameters));
    memset(&mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mPrevPortIndex], 0, sizeof(OMXCameraPortParameters));

    //Initialize 3A defaults
    ret = init3AParams(mParameters3A);
    if ( NO_ERROR != ret ) {
        CAMHAL_LOGEA("Couldn't init 3A params!");
        goto EXIT;
    }

    LOG_FUNCTION_NAME_EXIT;
    return ErrorUtils::omxToAndroidError(eError);

    EXIT:

    CAMHAL_LOGDB("Exiting function %s because of ret %d eError=%x", __FUNCTION__, ret, eError);
    performCleanupAfterError();
    LOG_FUNCTION_NAME_EXIT;
    return ErrorUtils::omxToAndroidError(eError);
}

void OMXCameraAdapter::performCleanupAfterError()
{
    if(mCameraAdapterParameters.mHandleComp)
        {
        ///Free the OMX component handle in case of error
        OMX_FreeHandle(mCameraAdapterParameters.mHandleComp);
        mCameraAdapterParameters.mHandleComp = NULL;
        }

    ///De-init the OMX
    OMX_Deinit();
    mComponentState = OMX_StateInvalid;
}

OMXCameraAdapter::OMXCameraPortParameters *OMXCameraAdapter::getPortParams(CameraFrame::FrameType frameType)
{
    OMXCameraAdapter::OMXCameraPortParameters *ret = NULL;

    switch ( frameType )
    {
    case CameraFrame::IMAGE_FRAME:
    case CameraFrame::RAW_FRAME:
        ret = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mImagePortIndex];
        break;
    case CameraFrame::PREVIEW_FRAME_SYNC:
    case CameraFrame::SNAPSHOT_FRAME:
    case CameraFrame::VIDEO_FRAME_SYNC:
        ret = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mPrevPortIndex];
        break;
    case CameraFrame::FRAME_DATA_SYNC:
        ret = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mMeasurementPortIndex];
        break;
    default:
        break;
    };

    return ret;
}

status_t OMXCameraAdapter::fillThisBuffer(void* frameBuf, CameraFrame::FrameType frameType)
{
    status_t ret = NO_ERROR;
    OMXCameraPortParameters *port = NULL;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    BaseCameraAdapter::AdapterState state;
    BaseCameraAdapter::getState(state);

    if ( ( PREVIEW_ACTIVE & state ) != PREVIEW_ACTIVE )
        {
        return NO_INIT;
        }

    if ( NULL == frameBuf )
        {
        return -EINVAL;
        }

    if ( (NO_ERROR == ret) &&
         ((CameraFrame::IMAGE_FRAME == frameType) || (CameraFrame::RAW_FRAME == frameType)) &&
         (1 > mCapturedFrames) &&
         (!mBracketingEnabled)) {
        // Signal end of image capture
        if ( NULL != mEndImageCaptureCallback) {
            mEndImageCaptureCallback(mEndCaptureData);
        }
        return NO_ERROR;
     }

    if ( NO_ERROR == ret )
        {
        port = getPortParams(frameType);
        if ( NULL == port )
            {
            CAMHAL_LOGEB("Invalid frameType 0x%x", frameType);
            ret = -EINVAL;
            }
        }

    if ( NO_ERROR == ret )
        {

        for ( int i = 0 ; i < port->mNumBufs ; i++)
            {
            if ( port->mBufferHeader[i]->pBuffer == frameBuf )
                {
                eError = OMX_FillThisBuffer(mCameraAdapterParameters.mHandleComp, port->mBufferHeader[i]);
                if ( eError != OMX_ErrorNone )
                    {
                    CAMHAL_LOGEB("OMX_FillThisBuffer 0x%x", eError);
                    goto EXIT;
                    }
                mFramesWithDucati++;
                break;
                }
            }

        }

    LOG_FUNCTION_NAME_EXIT;
    return ret;

EXIT:
    CAMHAL_LOGEB("Exiting function %s because of ret %d eError=%x", __FUNCTION__, ret, eError);
    performCleanupAfterError();
    //Since fillthisbuffer is called asynchronously, make sure to signal error to the app
    mErrorNotifier->errorNotify(CAMERA_ERROR_HARD);
    LOG_FUNCTION_NAME_EXIT;
    return (ret | ErrorUtils::omxToAndroidError(eError));
}

status_t OMXCameraAdapter::setParameters(const CameraParameters &params)
{
    LOG_FUNCTION_NAME;

    const char * str = NULL;
    int mode = 0;
    status_t ret = NO_ERROR;
    bool updateImagePortParams = false;
    int minFramerate, maxFramerate, frameRate;
    const char *valstr = NULL;
    const char *oldstr = NULL;
    int w, h;
    OMX_COLOR_FORMATTYPE pixFormat;
    BaseCameraAdapter::AdapterState state;
    BaseCameraAdapter::getState(state);

    ///@todo Include more camera parameters
    if ( (valstr = params.getPreviewFormat()) != NULL )
        {
        if(strcmp(valstr, (const char *) CameraParameters::PIXEL_FORMAT_YUV420SP) == 0 ||
           strcmp(valstr, (const char *) CameraParameters::PIXEL_FORMAT_YUV420P) == 0 ||
           strcmp(valstr, (const char *) CameraParameters::PIXEL_FORMAT_YUV422I) == 0)
            {
            CAMHAL_LOGDA("YUV420SP format selected");
            pixFormat = OMX_COLOR_FormatYUV420SemiPlanar;
            }
        else if(strcmp(valstr, (const char *) CameraParameters::PIXEL_FORMAT_RGB565) == 0)
            {
            CAMHAL_LOGDA("RGB565 format selected");
            pixFormat = OMX_COLOR_Format16bitRGB565;
            }
        else
            {
            CAMHAL_LOGDA("Invalid format, CbYCrY format selected as default");
            pixFormat = OMX_COLOR_FormatCbYCrY;
            }
        }
    else
        {
        CAMHAL_LOGEA("Preview format is NULL, defaulting to CbYCrY");
        pixFormat = OMX_COLOR_FormatCbYCrY;
        }

    OMXCameraPortParameters *cap;
    cap = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mPrevPortIndex];

    params.getPreviewSize(&w, &h);
    frameRate = params.getPreviewFrameRate();
    minFramerate = params.getInt(TICameraParameters::KEY_MINFRAMERATE);
    maxFramerate = params.getInt(TICameraParameters::KEY_MAXFRAMERATE);
    if ( ( 0 < minFramerate ) &&
         ( 0 < maxFramerate ) )
        {
        if ( minFramerate > maxFramerate )
            {
             CAMHAL_LOGEA(" Min FPS set higher than MAX. So setting MIN and MAX to the higher value");
             maxFramerate = minFramerate;
            }

        if ( 0 >= frameRate )
            {
            frameRate = maxFramerate;
            }

        if( ( cap->mMinFrameRate != minFramerate ) ||
            ( cap->mMaxFrameRate != maxFramerate ) )
            {
            cap->mMinFrameRate = minFramerate;
            cap->mMaxFrameRate = maxFramerate;
            setVFramerate(cap->mMinFrameRate, cap->mMaxFrameRate);
            }
        }

    // TODO(XXX): Limiting 1080p to (24,24) or (15,15) for now. Need to remove later.
    if ((w >= 1920) && (h >= 1080)) {
        cap->mMaxFrameRate = cap->mMinFrameRate;
        setVFramerate(cap->mMinFrameRate, cap->mMaxFrameRate);
    }

    if ( 0 < frameRate )
        {
        cap->mColorFormat = pixFormat;
        cap->mWidth = w;
        cap->mHeight = h;
        cap->mFrameRate = frameRate;

        CAMHAL_LOGVB("Prev: cap.mColorFormat = %d", (int)cap->mColorFormat);
        CAMHAL_LOGVB("Prev: cap.mWidth = %d", (int)cap->mWidth);
        CAMHAL_LOGVB("Prev: cap.mHeight = %d", (int)cap->mHeight);
        CAMHAL_LOGVB("Prev: cap.mFrameRate = %d", (int)cap->mFrameRate);

        //TODO: Add an additional parameter for video resolution
       //use preview resolution for now
        cap = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mPrevPortIndex];
        cap->mColorFormat = pixFormat;
        cap->mWidth = w;
        cap->mHeight = h;
        cap->mFrameRate = frameRate;

        CAMHAL_LOGVB("Video: cap.mColorFormat = %d", (int)cap->mColorFormat);
        CAMHAL_LOGVB("Video: cap.mWidth = %d", (int)cap->mWidth);
        CAMHAL_LOGVB("Video: cap.mHeight = %d", (int)cap->mHeight);
        CAMHAL_LOGVB("Video: cap.mFrameRate = %d", (int)cap->mFrameRate);

        ///mStride is set from setBufs() while passing the APIs
        cap->mStride = 4096;
        cap->mBufSize = cap->mStride * cap->mHeight;
        }

    if ( ( cap->mWidth >= 1920 ) &&
         ( cap->mHeight >= 1080 ) &&
         ( cap->mFrameRate >= FRAME_RATE_FULL_HD ) &&
         ( !mSensorOverclock ) )
        {
        mOMXStateSwitch = true;
        }
    else if ( ( ( cap->mWidth < 1920 ) ||
               ( cap->mHeight < 1080 ) ||
               ( cap->mFrameRate < FRAME_RATE_FULL_HD ) ) &&
               ( mSensorOverclock ) )
        {
        mOMXStateSwitch = true;
        }

    valstr = params.get(TICameraParameters::KEY_RECORDING_HINT);
    if (!valstr || (valstr && (strcmp(valstr, CameraParameters::FALSE)))) {
        mIternalRecordingHint = false;
    } else {
        mIternalRecordingHint = true;
    }

#ifdef OMAP_ENHANCEMENT

    if ( (valstr = params.get(TICameraParameters::KEY_MEASUREMENT_ENABLE)) != NULL )
        {
        if (strcmp(valstr, (const char *) TICameraParameters::MEASUREMENT_ENABLE) == 0)
            {
            mMeasurementEnabled = true;
            }
        else if (strcmp(valstr, (const char *) TICameraParameters::MEASUREMENT_DISABLE) == 0)
            {
            mMeasurementEnabled = false;
            }
        else
            {
            mMeasurementEnabled = false;
            }
        }
    else
        {
        //Disable measurement data by default
        mMeasurementEnabled = false;
        }

#endif

    ret |= setParametersCapture(params, state);

    ret |= setParameters3A(params, state);

    ret |= setParametersAlgo(params, state);

    ret |= setParametersFocus(params, state);

    ret |= setParametersFD(params, state);

    ret |= setParametersZoom(params, state);

    ret |= setParametersEXIF(params, state);

    mParams = params;
    mFirstTimeInit = false;

    LOG_FUNCTION_NAME_EXIT;
    return ret;
}

void saveFile(unsigned char   *buff, int width, int height, int format) {
    static int      counter = 1;
    int             fd = -1;
    char            fn[256];

    LOG_FUNCTION_NAME;

    fn[0] = 0;
    sprintf(fn, "/preview%03d.yuv", counter);
    fd = open(fn, O_CREAT | O_WRONLY | O_SYNC | O_TRUNC, 0777);
    if(fd < 0) {
        ALOGE("Unable to open file %s: %s", fn, strerror(fd));
        return;
    }

    CAMHAL_LOGVB("Copying from 0x%x, size=%d x %d", buff, width, height);

    //method currently supports only nv12 dumping
    int stride = width;
    uint8_t *bf = (uint8_t*) buff;
    for(int i=0;i<height;i++)
        {
        write(fd, bf, width);
        bf += 4096;
        }

    for(int i=0;i<height/2;i++)
        {
        write(fd, bf, stride);
        bf += 4096;
        }

    close(fd);


    counter++;

    LOG_FUNCTION_NAME_EXIT;
}

void OMXCameraAdapter::getParameters(CameraParameters& params)
{
    status_t ret = NO_ERROR;
    OMX_CONFIG_EXPOSUREVALUETYPE exp;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    BaseCameraAdapter::AdapterState state;
    BaseCameraAdapter::getState(state);
    const char *valstr = NULL;
    LOG_FUNCTION_NAME;

    if( mParameters3A.SceneMode != OMX_Manual ) {
       const char *valstr_supported = NULL;

       // if preview is not started...we still need to feedback the proper params
       // look up the settings in the LUT
       if (((state & PREVIEW_ACTIVE) == 0) && mCapabilities) {
           const SceneModesEntry* entry = NULL;
           entry = getSceneModeEntry(mCapabilities->get(CameraProperties::CAMERA_NAME),
                                    (OMX_SCENEMODETYPE) mParameters3A.SceneMode);
           if(entry) {
               mParameters3A.Focus = entry->focus;
               mParameters3A.FlashMode = entry->flash;
               mParameters3A.WhiteBallance = entry->wb;
           }
       }

       valstr = getLUTvalue_OMXtoHAL(mParameters3A.WhiteBallance, WBalLUT);
       valstr_supported = mParams.get(CameraParameters::KEY_SUPPORTED_WHITE_BALANCE);
       if (valstr && valstr_supported && strstr(valstr_supported, valstr))
           params.set(CameraParameters::KEY_WHITE_BALANCE , valstr);

       valstr = getLUTvalue_OMXtoHAL(mParameters3A.FlashMode, FlashLUT);
       valstr_supported = mParams.get(CameraParameters::KEY_SUPPORTED_FLASH_MODES);
       if (valstr && valstr_supported && strstr(valstr_supported, valstr))
           params.set(CameraParameters::KEY_FLASH_MODE, valstr);

       if ((mParameters3A.Focus == OMX_IMAGE_FocusControlAuto) &&
           (mCapMode != OMXCameraAdapter::VIDEO_MODE)) {
           valstr = CameraParameters::FOCUS_MODE_CONTINUOUS_PICTURE;
       } else {
           valstr = getLUTvalue_OMXtoHAL(mParameters3A.Focus, FocusLUT);
       }
       valstr_supported = mParams.get(CameraParameters::KEY_SUPPORTED_FOCUS_MODES);
       if (valstr && valstr_supported && strstr(valstr_supported, valstr))
           params.set(CameraParameters::KEY_FOCUS_MODE, valstr);
    }

    //Query focus distances only when focus is running
    if ( ( AF_ACTIVE & state ) ||
         ( NULL == mParameters.get(CameraParameters::KEY_FOCUS_DISTANCES) ) )
        {
        updateFocusDistances(params);
        }
    else
        {
        params.set(CameraParameters::KEY_FOCUS_DISTANCES,
                   mParameters.get(CameraParameters::KEY_FOCUS_DISTANCES));
        }

#ifdef OMAP_ENHANCEMENT

    OMX_INIT_STRUCT_PTR (&exp, OMX_CONFIG_EXPOSUREVALUETYPE);
    exp.nPortIndex = OMX_ALL;

    eError = OMX_GetConfig(mCameraAdapterParameters.mHandleComp,
                           OMX_IndexConfigCommonExposureValue,
                           &exp);
    if ( OMX_ErrorNone == eError )
        {
        params.set(TICameraParameters::KEY_CURRENT_ISO, exp.nSensitivity);
        }
    else
        {
        CAMHAL_LOGEB("OMX error 0x%x, while retrieving current ISO value", eError);
        }

#endif

    {
    Mutex::Autolock lock(mZoomLock);
    //Immediate zoom should not be avaialable while smooth zoom is running
    if ( ZOOM_ACTIVE & state )
        {
        if ( mZoomParameterIdx != mCurrentZoomIdx )
            {
            mZoomParameterIdx += mZoomInc;
            }
        params.set( CameraParameters::KEY_ZOOM, mZoomParameterIdx);
        if ( ( mCurrentZoomIdx == mTargetZoomIdx ) &&
             ( mZoomParameterIdx == mCurrentZoomIdx ) )
            {

            if ( NO_ERROR == ret )
                {

                ret =  BaseCameraAdapter::setState(CAMERA_STOP_SMOOTH_ZOOM);

                if ( NO_ERROR == ret )
                    {
                    ret = BaseCameraAdapter::commitState();
                    }
                else
                    {
                    ret |= BaseCameraAdapter::rollbackState();
                    }

                }

            }

        CAMHAL_LOGDB("CameraParameters Zoom = %d", mCurrentZoomIdx);
        }
    else
        {
        params.set( CameraParameters::KEY_ZOOM, mCurrentZoomIdx);
        }
    }

    //Populate current lock status
    if ( mParameters3A.ExposureLock ) {
        params.set(CameraParameters::KEY_AUTO_EXPOSURE_LOCK,
                   CameraParameters::TRUE);
    } else {
        params.set(CameraParameters::KEY_AUTO_EXPOSURE_LOCK,
                CameraParameters::FALSE);
    }

    if ( mParameters3A.WhiteBalanceLock ) {
        params.set(CameraParameters::KEY_AUTO_WHITEBALANCE_LOCK,
                CameraParameters::TRUE);
    } else {
        params.set(CameraParameters::KEY_AUTO_WHITEBALANCE_LOCK,
                CameraParameters::FALSE);
    }

    LOG_FUNCTION_NAME_EXIT;
}

status_t OMXCameraAdapter::setFormat(OMX_U32 port, OMXCameraPortParameters &portParams)
{
    size_t bufferCount;

    LOG_FUNCTION_NAME;

    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_PARAM_PORTDEFINITIONTYPE portCheck;

    OMX_INIT_STRUCT_PTR (&portCheck, OMX_PARAM_PORTDEFINITIONTYPE);

    portCheck.nPortIndex = port;

    eError = OMX_GetParameter (mCameraAdapterParameters.mHandleComp,
                                OMX_IndexParamPortDefinition, &portCheck);
    if(eError!=OMX_ErrorNone)
        {
        CAMHAL_LOGEB("OMX_GetParameter - %x", eError);
        }
    GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);

    if ( OMX_CAMERA_PORT_VIDEO_OUT_PREVIEW == port )
        {
        portCheck.format.video.nFrameWidth      = portParams.mWidth;
        portCheck.format.video.nFrameHeight     = portParams.mHeight;
        portCheck.format.video.eColorFormat     = portParams.mColorFormat;
        portCheck.format.video.nStride          = portParams.mStride;
        if( ( portCheck.format.video.nFrameWidth >= 1920 ) &&
            ( portCheck.format.video.nFrameHeight >= 1080 ) &&
            ( portParams.mFrameRate >= FRAME_RATE_FULL_HD ) )
            {
            setSensorOverclock(true);
            }
        else
            {
            setSensorOverclock(false);
            }

        portCheck.format.video.xFramerate       = portParams.mFrameRate<<16;
        portCheck.nBufferSize                   = portParams.mStride * portParams.mHeight;
        portCheck.nBufferCountActual = portParams.mNumBufs;
        mFocusThreshold = FOCUS_THRESHOLD * portParams.mFrameRate;
        }
    else if ( OMX_CAMERA_PORT_IMAGE_OUT_IMAGE == port )
        {
        portCheck.format.image.nFrameWidth      = portParams.mWidth;
        portCheck.format.image.nFrameHeight     = portParams.mHeight;
        if ( OMX_COLOR_FormatUnused == portParams.mColorFormat && mCodingMode == CodingNone )
            {
            portCheck.format.image.eColorFormat     = OMX_COLOR_FormatCbYCrY;
            portCheck.format.image.eCompressionFormat = OMX_IMAGE_CodingJPEG;
            }
        else if ( OMX_COLOR_FormatUnused == portParams.mColorFormat && mCodingMode == CodingJPS )
            {
            portCheck.format.image.eColorFormat       = OMX_COLOR_FormatCbYCrY;
            portCheck.format.image.eCompressionFormat = (OMX_IMAGE_CODINGTYPE) OMX_TI_IMAGE_CodingJPS;
            }
        else if ( OMX_COLOR_FormatUnused == portParams.mColorFormat && mCodingMode == CodingMPO )
            {
            portCheck.format.image.eColorFormat       = OMX_COLOR_FormatCbYCrY;
            portCheck.format.image.eCompressionFormat = (OMX_IMAGE_CODINGTYPE) OMX_TI_IMAGE_CodingMPO;
            }
        else if ( OMX_COLOR_FormatUnused == portParams.mColorFormat && mCodingMode == CodingRAWJPEG )
            {
            //TODO: OMX_IMAGE_CodingJPEG should be changed to OMX_IMAGE_CodingRAWJPEG when
            // RAW format is supported
            portCheck.format.image.eColorFormat       = OMX_COLOR_FormatCbYCrY;
            portCheck.format.image.eCompressionFormat = OMX_IMAGE_CodingJPEG;
            }
        else if ( OMX_COLOR_FormatUnused == portParams.mColorFormat && mCodingMode == CodingRAWMPO )
            {
            //TODO: OMX_IMAGE_CodingJPEG should be changed to OMX_IMAGE_CodingRAWMPO when
            // RAW format is supported
            portCheck.format.image.eColorFormat       = OMX_COLOR_FormatCbYCrY;
            portCheck.format.image.eCompressionFormat = OMX_IMAGE_CodingJPEG;
            }
        else
            {
            portCheck.format.image.eColorFormat     = portParams.mColorFormat;
            portCheck.format.image.eCompressionFormat = OMX_IMAGE_CodingUnused;
            }

        //Stride for 1D tiler buffer is zero
        portCheck.format.image.nStride          =  0;
        portCheck.nBufferSize                   =  portParams.mStride * portParams.mWidth * portParams.mHeight;
        portCheck.nBufferCountActual = portParams.mNumBufs;
        }
    else
        {
        CAMHAL_LOGEB("Unsupported port index 0x%x", (unsigned int)port);
        }

    eError = OMX_SetParameter(mCameraAdapterParameters.mHandleComp,
                            OMX_IndexParamPortDefinition, &portCheck);
    if(eError!=OMX_ErrorNone)
        {
        CAMHAL_LOGEB("OMX_SetParameter - %x", eError);
        }
    GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);

    /* check if parameters are set correctly by calling GetParameter() */
    eError = OMX_GetParameter(mCameraAdapterParameters.mHandleComp,
                                        OMX_IndexParamPortDefinition, &portCheck);
    if(eError!=OMX_ErrorNone)
        {
        CAMHAL_LOGEB("OMX_GetParameter - %x", eError);
        }
    GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);

    portParams.mBufSize = portCheck.nBufferSize;
    portParams.mStride = portCheck.format.image.nStride;

    if ( OMX_CAMERA_PORT_IMAGE_OUT_IMAGE == port )
        {
        CAMHAL_LOGDB("\n *** IMG Width = %ld", portCheck.format.image.nFrameWidth);
        CAMHAL_LOGDB("\n ***IMG Height = %ld", portCheck.format.image.nFrameHeight);

        CAMHAL_LOGDB("\n ***IMG IMG FMT = %x", portCheck.format.image.eColorFormat);
        CAMHAL_LOGDB("\n ***IMG portCheck.nBufferSize = %ld\n",portCheck.nBufferSize);
        CAMHAL_LOGDB("\n ***IMG portCheck.nBufferCountMin = %ld\n",
                                                portCheck.nBufferCountMin);
        CAMHAL_LOGDB("\n ***IMG portCheck.nBufferCountActual = %ld\n",
                                                portCheck.nBufferCountActual);
        CAMHAL_LOGDB("\n ***IMG portCheck.format.image.nStride = %ld\n",
                                                portCheck.format.image.nStride);
        }
    else
        {
        CAMHAL_LOGDB("\n *** PRV Width = %ld", portCheck.format.video.nFrameWidth);
        CAMHAL_LOGDB("\n ***PRV Height = %ld", portCheck.format.video.nFrameHeight);

        CAMHAL_LOGDB("\n ***PRV IMG FMT = %x", portCheck.format.video.eColorFormat);
        CAMHAL_LOGDB("\n ***PRV portCheck.nBufferSize = %ld\n",portCheck.nBufferSize);
        CAMHAL_LOGDB("\n ***PRV portCheck.nBufferCountMin = %ld\n",
                                                portCheck.nBufferCountMin);
        CAMHAL_LOGDB("\n ***PRV portCheck.nBufferCountActual = %ld\n",
                                                portCheck.nBufferCountActual);
        CAMHAL_LOGDB("\n ***PRV portCheck.format.video.nStride = %ld\n",
                                                portCheck.format.video.nStride);
        }

    LOG_FUNCTION_NAME_EXIT;

    return ErrorUtils::omxToAndroidError(eError);

    EXIT:

    CAMHAL_LOGEB("Exiting function %s because of eError=%x", __FUNCTION__, eError);

    LOG_FUNCTION_NAME_EXIT;

    return ErrorUtils::omxToAndroidError(eError);
}

status_t OMXCameraAdapter::flushBuffers()
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    TIMM_OSAL_ERRORTYPE err;
    TIMM_OSAL_U32 uRequestedEvents = OMXCameraAdapter::CAMERA_PORT_FLUSH;
    TIMM_OSAL_U32 pRetrievedEvents;

    if ( 0 != mFlushSem.Count() )
        {
        CAMHAL_LOGEB("Error mFlushSem semaphore count %d", mFlushSem.Count());
        LOG_FUNCTION_NAME_EXIT;
        return NO_INIT;
        }

    LOG_FUNCTION_NAME;

    OMXCameraPortParameters * mPreviewData = NULL;
    mPreviewData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mPrevPortIndex];

    ///Register for the FLUSH event
    ///This method just inserts a message in Event Q, which is checked in the callback
    ///The sempahore passed is signalled by the callback
    ret = RegisterForEvent(mCameraAdapterParameters.mHandleComp,
                                OMX_EventCmdComplete,
                                OMX_CommandFlush,
                                OMX_CAMERA_PORT_VIDEO_OUT_PREVIEW,
                                mFlushSem);
    if(ret!=NO_ERROR)
        {
        CAMHAL_LOGEB("Error in registering for event %d", ret);
        goto EXIT;
        }

    ///Send FLUSH command to preview port
    eError = OMX_SendCommand (mCameraAdapterParameters.mHandleComp,
                              OMX_CommandFlush,
                              mCameraAdapterParameters.mPrevPortIndex,
                              NULL);

    if(eError!=OMX_ErrorNone)
        {
        CAMHAL_LOGEB("OMX_SendCommand(OMX_CommandFlush)-0x%x", eError);
        }
    GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);

    CAMHAL_LOGDA("Waiting for flush event");

    ///Wait for the FLUSH event to occur
    ret = mFlushSem.WaitTimeout(OMX_CMD_TIMEOUT);

    //If somethiing bad happened while we wait
    if (mComponentState == OMX_StateInvalid)
      {
        CAMHAL_LOGEA("Invalid State after Flush Exitting!!!");
        goto EXIT;
      }

    if ( NO_ERROR == ret )
        {
        CAMHAL_LOGDA("Flush event received");
        }
    else
        {
        ret |= RemoveEvent(mCameraAdapterParameters.mHandleComp,
                           OMX_EventCmdComplete,
                           OMX_CommandFlush,
                           OMX_CAMERA_PORT_VIDEO_OUT_PREVIEW,
                           NULL);
        CAMHAL_LOGDA("Flush event timeout expired");
        goto EXIT;
        }

    LOG_FUNCTION_NAME_EXIT;

    return (ret | ErrorUtils::omxToAndroidError(eError));

    EXIT:
    CAMHAL_LOGEB("Exiting function %s because of ret %d eError=%x", __FUNCTION__, ret, eError);
    performCleanupAfterError();
    LOG_FUNCTION_NAME_EXIT;
    return (ret | ErrorUtils::omxToAndroidError(eError));
}

///API to give the buffers to Adapter
status_t OMXCameraAdapter::useBuffers(CameraMode mode, void* bufArr, int num, size_t length, unsigned int queueable)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    switch(mode)
        {
        case CAMERA_PREVIEW:
            mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mPrevPortIndex].mNumBufs =  num;
            mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mPrevPortIndex].mMaxQueueable = queueable;
            ret = UseBuffersPreview(bufArr, num);
            break;

        case CAMERA_IMAGE_CAPTURE:
            mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mImagePortIndex].mNumBufs = num;
            mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mImagePortIndex].mMaxQueueable = queueable;
            ret = UseBuffersCapture(bufArr, num);
            break;

        case CAMERA_VIDEO:
            mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mPrevPortIndex].mNumBufs =  num;
            mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mPrevPortIndex].mMaxQueueable = queueable;
            ret = UseBuffersPreview(bufArr, num);
            break;

        case CAMERA_MEASUREMENT:
            mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mMeasurementPortIndex].mNumBufs = num;
            mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mMeasurementPortIndex].mMaxQueueable = queueable;
            ret = UseBuffersPreviewData(bufArr, num);
            break;

        }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::UseBuffersPreviewData(void* bufArr, int num)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMXCameraPortParameters * measurementData = NULL;
    uint32_t *buffers;
    Mutex::Autolock lock( mPreviewDataBufferLock);

    LOG_FUNCTION_NAME;

    if ( mComponentState != OMX_StateLoaded )
        {
        CAMHAL_LOGEA("Calling UseBuffersPreviewData() when not in LOADED state");
        return BAD_VALUE;
        }

    if ( NULL == bufArr )
        {
        CAMHAL_LOGEA("NULL pointer passed for buffArr");
        return BAD_VALUE;
        }

    if ( 0 != mUsePreviewDataSem.Count() )
        {
        CAMHAL_LOGEB("Error mUsePreviewDataSem semaphore count %d", mUsePreviewDataSem.Count());
        LOG_FUNCTION_NAME_EXIT;
        return NO_INIT;
        }

    if ( NO_ERROR == ret )
        {
        measurementData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mMeasurementPortIndex];
        measurementData->mNumBufs = num ;
        buffers= (uint32_t*) bufArr;
        }

    if ( NO_ERROR == ret )
        {
         ///Register for port enable event on measurement port
        ret = RegisterForEvent(mCameraAdapterParameters.mHandleComp,
                                      OMX_EventCmdComplete,
                                      OMX_CommandPortEnable,
                                      mCameraAdapterParameters.mMeasurementPortIndex,
                                      mUsePreviewDataSem);

        if ( ret == NO_ERROR )
            {
            CAMHAL_LOGDB("Registering for event %d", ret);
            }
        else
            {
            CAMHAL_LOGEB("Error in registering for event %d", ret);
            goto EXIT;
            }
        }

    if ( NO_ERROR == ret )
        {
         ///Enable MEASUREMENT Port
         eError = OMX_SendCommand(mCameraAdapterParameters.mHandleComp,
                                      OMX_CommandPortEnable,
                                      mCameraAdapterParameters.mMeasurementPortIndex,
                                      NULL);

            if ( eError == OMX_ErrorNone )
                {
                CAMHAL_LOGDB("OMX_SendCommand(OMX_CommandPortEnable) -0x%x", eError);
                }
            else
                {
                CAMHAL_LOGEB("OMX_SendCommand(OMX_CommandPortEnable) -0x%x", eError);
                goto EXIT;
                }
        }

    if ( NO_ERROR == ret )
        {
        ret = mUsePreviewDataSem.WaitTimeout(OMX_CMD_TIMEOUT);

        //If somethiing bad happened while we wait
        if (mComponentState == OMX_StateInvalid)
          {
            CAMHAL_LOGEA("Invalid State after measurement port enable Exitting!!!");
            goto EXIT;
          }

        if ( NO_ERROR == ret )
            {
            CAMHAL_LOGDA("Port enable event arrived on measurement port");
            }
        else
            {
            ret |= RemoveEvent(mCameraAdapterParameters.mHandleComp,
                               OMX_EventCmdComplete,
                               OMX_CommandPortEnable,
                               mCameraAdapterParameters.mMeasurementPortIndex,
                               NULL);
            CAMHAL_LOGEA("Timeout expoired during port enable on measurement port");
            goto EXIT;
            }

        CAMHAL_LOGDA("Port enable event arrived on measurement port");
        }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
EXIT:
    CAMHAL_LOGEB("Exiting function %s because of ret %d eError=%x", __FUNCTION__, ret, eError);
    performCleanupAfterError();
    LOG_FUNCTION_NAME_EXIT;
    return (ret | ErrorUtils::omxToAndroidError(eError));
}

status_t OMXCameraAdapter::switchToExecuting()
{
  status_t ret = NO_ERROR;
  TIUTILS::Message msg;

  LOG_FUNCTION_NAME;

  mStateSwitchLock.lock();
  msg.command = CommandHandler::CAMERA_SWITCH_TO_EXECUTING;
  msg.arg1 = mErrorNotifier;
  ret = mCommandHandler->put(&msg);

  LOG_FUNCTION_NAME;

  return ret;
}

status_t OMXCameraAdapter::doSwitchToExecuting()
{
  status_t ret = NO_ERROR;
  OMX_ERRORTYPE eError = OMX_ErrorNone;
  LOG_FUNCTION_NAME;

  if ( (mComponentState == OMX_StateExecuting) || (mComponentState == OMX_StateInvalid) ){
    CAMHAL_LOGDA("Already in OMX_Executing state or OMX_StateInvalid state");
    mStateSwitchLock.unlock();
    return NO_ERROR;
  }

  if ( 0 != mSwitchToExecSem.Count() ){
    CAMHAL_LOGEB("Error mSwitchToExecSem semaphore count %d", mSwitchToExecSem.Count());
    goto EXIT;
  }

  ///Register for Preview port DISABLE  event
  ret = RegisterForEvent(mCameraAdapterParameters.mHandleComp,
                         OMX_EventCmdComplete,
                         OMX_CommandPortDisable,
                         mCameraAdapterParameters.mPrevPortIndex,
                         mSwitchToExecSem);
  if ( NO_ERROR != ret ){
    CAMHAL_LOGEB("Error in registering Port Disable for event %d", ret);
    goto EXIT;
  }
  ///Disable Preview Port
  eError = OMX_SendCommand(mCameraAdapterParameters.mHandleComp,
                           OMX_CommandPortDisable,
                           mCameraAdapterParameters.mPrevPortIndex,
                           NULL);
  ret = mSwitchToExecSem.WaitTimeout(OMX_CMD_TIMEOUT);
  if (ret != NO_ERROR){
    CAMHAL_LOGEB("Timeout PREVIEW PORT DISABLE %d", ret);
  }

  CAMHAL_LOGVB("PREV PORT DISABLED %d", ret);

  ///Register for IDLE state switch event
  ret = RegisterForEvent(mCameraAdapterParameters.mHandleComp,
                         OMX_EventCmdComplete,
                         OMX_CommandStateSet,
                         OMX_StateIdle,
                         mSwitchToExecSem);
  if(ret!=NO_ERROR)
    {
      CAMHAL_LOGEB("Error in IDLE STATE SWITCH %d", ret);
      goto EXIT;
    }
  eError = OMX_SendCommand (mCameraAdapterParameters.mHandleComp ,
                            OMX_CommandStateSet,
                            OMX_StateIdle,
                            NULL);
  GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);
  ret = mSwitchToExecSem.WaitTimeout(OMX_CMD_TIMEOUT);
  if (ret != NO_ERROR){
    CAMHAL_LOGEB("Timeout IDLE STATE SWITCH %d", ret);
    goto EXIT;
  }
  mComponentState = OMX_StateIdle;
  CAMHAL_LOGVB("OMX_SendCommand(OMX_StateIdle) 0x%x", eError);

  ///Register for EXECUTING state switch event
  ret = RegisterForEvent(mCameraAdapterParameters.mHandleComp,
                         OMX_EventCmdComplete,
                         OMX_CommandStateSet,
                         OMX_StateExecuting,
                         mSwitchToExecSem);
  if(ret!=NO_ERROR)
    {
      CAMHAL_LOGEB("Error in EXECUTING STATE SWITCH %d", ret);
      goto EXIT;
    }
  eError = OMX_SendCommand (mCameraAdapterParameters.mHandleComp ,
                            OMX_CommandStateSet,
                            OMX_StateExecuting,
                            NULL);
  GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);
  ret = mSwitchToExecSem.WaitTimeout(OMX_CMD_TIMEOUT);
  if (ret != NO_ERROR){
    CAMHAL_LOGEB("Timeout EXEC STATE SWITCH %d", ret);
    goto EXIT;
  }
  mComponentState = OMX_StateExecuting;
  CAMHAL_LOGVB("OMX_SendCommand(OMX_StateExecuting) 0x%x", eError);

  mStateSwitchLock.unlock();

  LOG_FUNCTION_NAME_EXIT;
  return ret;

 EXIT:
  CAMHAL_LOGEB("Exiting function %s because of ret %d eError=%x", __FUNCTION__, ret, eError);
  performCleanupAfterError();
  mStateSwitchLock.unlock();
  LOG_FUNCTION_NAME_EXIT;
  return (ret | ErrorUtils::omxToAndroidError(eError));
}

status_t OMXCameraAdapter::switchToLoaded()
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    LOG_FUNCTION_NAME;

    Mutex::Autolock lock(mStateSwitchLock);

    if ( mComponentState == OMX_StateLoaded  || mComponentState == OMX_StateInvalid)
        {
        CAMHAL_LOGDA("Already in OMX_Loaded state or OMX_StateInvalid state");
        return NO_ERROR;
        }

    if ( 0 != mSwitchToLoadedSem.Count() )
        {
        CAMHAL_LOGEB("Error mSwitchToLoadedSem semaphore count %d", mSwitchToLoadedSem.Count());
        goto EXIT;
        }

    ///Register for EXECUTING state transition.
    ///This method just inserts a message in Event Q, which is checked in the callback
    ///The sempahore passed is signalled by the callback
    ret = RegisterForEvent(mCameraAdapterParameters.mHandleComp,
                           OMX_EventCmdComplete,
                           OMX_CommandStateSet,
                           OMX_StateIdle,
                           mSwitchToLoadedSem);

    if(ret!=NO_ERROR)
        {
        CAMHAL_LOGEB("Error in registering for event %d", ret);
        goto EXIT;
        }

    eError = OMX_SendCommand (mCameraAdapterParameters.mHandleComp,
                              OMX_CommandStateSet,
                              OMX_StateIdle,
                              NULL);

    if(eError!=OMX_ErrorNone)
        {
        CAMHAL_LOGEB("OMX_SendCommand(OMX_StateIdle) - %x", eError);
        }

    GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);

    ///Wait for the EXECUTING ->IDLE transition to arrive

    CAMHAL_LOGDA("EXECUTING->IDLE state changed");
    ret = mSwitchToLoadedSem.WaitTimeout(OMX_CMD_TIMEOUT);

    //If somethiing bad happened while we wait
    if (mComponentState == OMX_StateInvalid)
      {
        CAMHAL_LOGEA("Invalid State after EXECUTING->IDLE Exitting!!!");
        goto EXIT;
      }

    if ( NO_ERROR == ret )
        {
        CAMHAL_LOGDA("EXECUTING->IDLE state changed");
        }
    else
        {
        ret |= RemoveEvent(mCameraAdapterParameters.mHandleComp,
                           OMX_EventCmdComplete,
                           OMX_CommandStateSet,
                           OMX_StateIdle,
                           NULL);
        CAMHAL_LOGEA("Timeout expired on EXECUTING->IDLE state change");
        goto EXIT;
        }

    ///Register for LOADED state transition.
    ///This method just inserts a message in Event Q, which is checked in the callback
    ///The sempahore passed is signalled by the callback
    ret = RegisterForEvent(mCameraAdapterParameters.mHandleComp,
                           OMX_EventCmdComplete,
                           OMX_CommandStateSet,
                           OMX_StateLoaded,
                           mSwitchToLoadedSem);

    if(ret!=NO_ERROR)
        {
        CAMHAL_LOGEB("Error in registering for event %d", ret);
        goto EXIT;
        }

    eError = OMX_SendCommand (mCameraAdapterParameters.mHandleComp,
                              OMX_CommandStateSet,
                              OMX_StateLoaded,
                              NULL);

    if(eError!=OMX_ErrorNone)
        {
        CAMHAL_LOGEB("OMX_SendCommand(OMX_StateLoaded) - %x", eError);
        }
    GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);

    CAMHAL_LOGDA("Switching IDLE->LOADED state");
    ret = mSwitchToLoadedSem.WaitTimeout(OMX_CMD_TIMEOUT);

    //If somethiing bad happened while we wait
    if (mComponentState == OMX_StateInvalid)
      {
        CAMHAL_LOGEA("Invalid State after IDLE->LOADED Exitting!!!");
        goto EXIT;
      }

    if ( NO_ERROR == ret )
        {
        CAMHAL_LOGDA("IDLE->LOADED state changed");
        }
    else
        {
        ret |= RemoveEvent(mCameraAdapterParameters.mHandleComp,
                           OMX_EventCmdComplete,
                           OMX_CommandStateSet,
                           OMX_StateLoaded,
                           NULL);
        CAMHAL_LOGEA("Timeout expired on IDLE->LOADED state change");
        goto EXIT;
        }

    mComponentState = OMX_StateLoaded;

    ///Register for Preview port ENABLE event
    ret = RegisterForEvent(mCameraAdapterParameters.mHandleComp,
                           OMX_EventCmdComplete,
                           OMX_CommandPortEnable,
                           mCameraAdapterParameters.mPrevPortIndex,
                           mSwitchToLoadedSem);

    if ( NO_ERROR != ret )
        {
        CAMHAL_LOGEB("Error in registering for event %d", ret);
        goto EXIT;
        }

    ///Enable Preview Port
    eError = OMX_SendCommand(mCameraAdapterParameters.mHandleComp,
                             OMX_CommandPortEnable,
                             mCameraAdapterParameters.mPrevPortIndex,
                             NULL);


    CAMHAL_LOGDB("OMX_SendCommand(OMX_CommandStateSet) 0x%x", eError);
    GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);

    CAMHAL_LOGDA("Enabling Preview port");
    ///Wait for state to switch to idle
    ret = mSwitchToLoadedSem.WaitTimeout(OMX_CMD_TIMEOUT);

    //If somethiing bad happened while we wait
    if (mComponentState == OMX_StateInvalid)
      {
        CAMHAL_LOGEA("Invalid State after Enabling Preview port Exitting!!!");
        goto EXIT;
      }

    if ( NO_ERROR == ret )
        {
        CAMHAL_LOGDA("Preview port enabled!");
        }
    else
        {
        ret |= RemoveEvent(mCameraAdapterParameters.mHandleComp,
                           OMX_EventCmdComplete,
                           OMX_CommandPortEnable,
                           mCameraAdapterParameters.mPrevPortIndex,
                           NULL);
        CAMHAL_LOGEA("Preview enable timedout");

        goto EXIT;
        }

    return (ret | ErrorUtils::omxToAndroidError(eError));

EXIT:
    CAMHAL_LOGEB("Exiting function %s because of ret %d eError=%x", __FUNCTION__, ret, eError);
    performCleanupAfterError();
    LOG_FUNCTION_NAME_EXIT;
    return (ret | ErrorUtils::omxToAndroidError(eError));
}

status_t OMXCameraAdapter::UseBuffersPreview(void* bufArr, int num)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    int tmpHeight, tmpWidth;

    LOG_FUNCTION_NAME;

    if(!bufArr)
        {
        CAMHAL_LOGEA("NULL pointer passed for buffArr");
        LOG_FUNCTION_NAME_EXIT;
        return BAD_VALUE;
        }

    OMXCameraPortParameters * mPreviewData = NULL;
    OMXCameraPortParameters *measurementData = NULL;
    mPreviewData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mPrevPortIndex];
    measurementData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mMeasurementPortIndex];
    mPreviewData->mNumBufs = num ;
    uint32_t *buffers = (uint32_t*)bufArr;

    if ( 0 != mUsePreviewSem.Count() )
        {
        CAMHAL_LOGEB("Error mUsePreviewSem semaphore count %d", mUsePreviewSem.Count());
        LOG_FUNCTION_NAME_EXIT;
        return NO_INIT;
        }

    if(mPreviewData->mNumBufs != num)
        {
        CAMHAL_LOGEA("Current number of buffers doesnt equal new num of buffers passed!");
        LOG_FUNCTION_NAME_EXIT;
        return BAD_VALUE;
        }

    mStateSwitchLock.lock();

    if ( mComponentState == OMX_StateLoaded )
        {

        ret = setLDC(mIPP);
        if ( NO_ERROR != ret )
            {
            CAMHAL_LOGEB("setLDC() failed %d", ret);
            LOG_FUNCTION_NAME_EXIT;
            return ret;
            }

        ret = setNSF(mIPP);
        if ( NO_ERROR != ret )
            {
            CAMHAL_LOGEB("setNSF() failed %d", ret);
            LOG_FUNCTION_NAME_EXIT;
            return ret;
            }

        ret = setCaptureMode(mCapMode);
        if ( NO_ERROR != ret )
            {
            CAMHAL_LOGEB("setCaptureMode() failed %d", ret);
            LOG_FUNCTION_NAME_EXIT;
            return ret;
            }

        CAMHAL_LOGDB("Camera Mode = %d", mCapMode);

        if( mCapMode == OMXCameraAdapter::VIDEO_MODE )
            {
            ///Enable/Disable Video Noise Filter
            ret = enableVideoNoiseFilter(mVnfEnabled);
            if ( NO_ERROR != ret)
                {
                CAMHAL_LOGEB("Error configuring VNF %x", ret);
                return ret;
                }

            ///Enable/Disable Video Stabilization
            ret = enableVideoStabilization(mVstabEnabled);
            if ( NO_ERROR != ret)
                {
                CAMHAL_LOGEB("Error configuring VSTAB %x", ret);
                return ret;
                }
            }
        else
            {
            ret = enableVideoNoiseFilter(false);
            if ( NO_ERROR != ret)
                {
                CAMHAL_LOGEB("Error configuring VNF %x", ret);
                return ret;
                }
            ///Enable/Disable Video Stabilization
            ret = enableVideoStabilization(false);
            if ( NO_ERROR != ret)
                {
                CAMHAL_LOGEB("Error configuring VSTAB %x", ret);
                return ret;
                }
            }
        }

    ret = setSensorOrientation(mSensorOrientation);
    if ( NO_ERROR != ret )
        {
        CAMHAL_LOGEB("Error configuring Sensor Orientation %x", ret);
        mSensorOrientation = 0;
        }

    ret = setVFramerate(mPreviewData->mMinFrameRate, mPreviewData->mMaxFrameRate);
    if ( ret != NO_ERROR )
        {
        CAMHAL_LOGEB("VFR configuration failed 0x%x", ret);
        LOG_FUNCTION_NAME_EXIT;
        return ret;
        }

    if ( mComponentState == OMX_StateLoaded )
        {
        ///Register for IDLE state switch event
        ret = RegisterForEvent(mCameraAdapterParameters.mHandleComp,
                               OMX_EventCmdComplete,
                               OMX_CommandStateSet,
                               OMX_StateIdle,
                               mUsePreviewSem);

        if(ret!=NO_ERROR)
            {
            CAMHAL_LOGEB("Error in registering for event %d", ret);
            goto EXIT;
            }

        ///Once we get the buffers, move component state to idle state and pass the buffers to OMX comp using UseBuffer
        eError = OMX_SendCommand (mCameraAdapterParameters.mHandleComp ,
                                  OMX_CommandStateSet,
                                  OMX_StateIdle,
                                  NULL);

        CAMHAL_LOGDB("OMX_SendCommand(OMX_CommandStateSet) 0x%x", eError);

        GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);

        mComponentState = OMX_StateIdle;
        }
    else
        {
            ///Register for Preview port ENABLE event
            ret = RegisterForEvent(mCameraAdapterParameters.mHandleComp,
                                   OMX_EventCmdComplete,
                                   OMX_CommandPortEnable,
                                   mCameraAdapterParameters.mPrevPortIndex,
                                   mUsePreviewSem);

            if ( NO_ERROR != ret )
                {
                CAMHAL_LOGEB("Error in registering for event %d", ret);
                goto EXIT;
                }

            ///Enable Preview Port
            eError = OMX_SendCommand(mCameraAdapterParameters.mHandleComp,
                                     OMX_CommandPortEnable,
                                     mCameraAdapterParameters.mPrevPortIndex,
                                     NULL);
        }


    ///Configure DOMX to use either gralloc handles or vptrs
    OMX_TI_PARAMUSENATIVEBUFFER domxUseGrallocHandles;
    OMX_INIT_STRUCT_PTR (&domxUseGrallocHandles, OMX_TI_PARAMUSENATIVEBUFFER);

    domxUseGrallocHandles.nPortIndex = mCameraAdapterParameters.mPrevPortIndex;
    domxUseGrallocHandles.bEnable = OMX_TRUE;

    eError = OMX_SetParameter(mCameraAdapterParameters.mHandleComp,
                            (OMX_INDEXTYPE)OMX_TI_IndexUseNativeBuffers, &domxUseGrallocHandles);
    if(eError!=OMX_ErrorNone)
        {
        CAMHAL_LOGEB("OMX_SetParameter - %x", eError);
        }
    GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);

    OMX_BUFFERHEADERTYPE *pBufferHdr;
    for(int index=0;index<num;index++) {

        CAMHAL_LOGDB("OMX_UseBuffer(0x%x)", buffers[index]);
        eError = OMX_UseBuffer( mCameraAdapterParameters.mHandleComp,
                                &pBufferHdr,
                                mCameraAdapterParameters.mPrevPortIndex,
                                0,
                                mPreviewData->mBufSize,
                                (OMX_U8*)buffers[index]);
        if(eError!=OMX_ErrorNone)
            {
            CAMHAL_LOGEB("OMX_UseBuffer-0x%x", eError);
            }
        GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);

        //pBufferHdr->pAppPrivate =  (OMX_PTR)pBufferHdr;
        pBufferHdr->nSize = sizeof(OMX_BUFFERHEADERTYPE);
        pBufferHdr->nVersion.s.nVersionMajor = 1 ;
        pBufferHdr->nVersion.s.nVersionMinor = 1 ;
        pBufferHdr->nVersion.s.nRevision = 0 ;
        pBufferHdr->nVersion.s.nStep =  0;
        mPreviewData->mBufferHeader[index] = pBufferHdr;
    }

    if ( mMeasurementEnabled )
        {

        for( int i = 0; i < num; i++ )
            {
            OMX_BUFFERHEADERTYPE *pBufHdr;
            eError = OMX_UseBuffer( mCameraAdapterParameters.mHandleComp,
                                    &pBufHdr,
                                    mCameraAdapterParameters.mMeasurementPortIndex,
                                    0,
                                    measurementData->mBufSize,
                                    (OMX_U8*)(mPreviewDataBuffers[i]));

             if ( eError == OMX_ErrorNone )
                {
                pBufHdr->nSize = sizeof(OMX_BUFFERHEADERTYPE);
                pBufHdr->nVersion.s.nVersionMajor = 1 ;
                pBufHdr->nVersion.s.nVersionMinor = 1 ;
                pBufHdr->nVersion.s.nRevision = 0 ;
                pBufHdr->nVersion.s.nStep =  0;
                measurementData->mBufferHeader[i] = pBufHdr;
                }
            else
                {
                CAMHAL_LOGEB("OMX_UseBuffer -0x%x", eError);
                ret = BAD_VALUE;
                break;
                }
            }

        }

    CAMHAL_LOGDA("Registering preview buffers");

    ret = mUsePreviewSem.WaitTimeout(OMX_CMD_TIMEOUT);

    //If somethiing bad happened while we wait
    if (mComponentState == OMX_StateInvalid)
      {
        CAMHAL_LOGEA("Invalid State after Registering preview buffers Exitting!!!");
        goto EXIT;
      }

    if ( NO_ERROR == ret )
        {
        CAMHAL_LOGDA("Preview buffer registration successfull");
        }
    else
        {
        if ( mComponentState == OMX_StateLoaded )
            {
            ret |= RemoveEvent(mCameraAdapterParameters.mHandleComp,
                               OMX_EventCmdComplete,
                               OMX_CommandStateSet,
                               OMX_StateIdle,
                               NULL);
            }
        else
            {
            ret |= SignalEvent(mCameraAdapterParameters.mHandleComp,
                               OMX_EventCmdComplete,
                               OMX_CommandPortEnable,
                               mCameraAdapterParameters.mPrevPortIndex,
                               NULL);
            }
        CAMHAL_LOGEA("Timeout expired on preview buffer registration");
        goto EXIT;
        }

    LOG_FUNCTION_NAME_EXIT;

    return (ret | ErrorUtils::omxToAndroidError(eError));

    ///If there is any failure, we reach here.
    ///Here, we do any resource freeing and convert from OMX error code to Camera Hal error code
EXIT:
    mStateSwitchLock.unlock();

    CAMHAL_LOGEB("Exiting function %s because of ret %d eError=%x", __FUNCTION__, ret, eError);
    performCleanupAfterError();
    CAMHAL_LOGEB("Exiting function %s because of ret %d eError=%x", __FUNCTION__, ret, eError);

    LOG_FUNCTION_NAME_EXIT;

    return (ret | ErrorUtils::omxToAndroidError(eError));
}

status_t OMXCameraAdapter::startPreview()
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMXCameraPortParameters *mPreviewData = NULL;
    OMXCameraPortParameters *measurementData = NULL;

    LOG_FUNCTION_NAME;

    if( 0 != mStartPreviewSem.Count() )
        {
        CAMHAL_LOGEB("Error mStartPreviewSem semaphore count %d", mStartPreviewSem.Count());
        ret = NO_INIT;
        goto EXIT;
        }

    mPreviewData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mPrevPortIndex];
    measurementData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mMeasurementPortIndex];

    if( OMX_StateIdle == mComponentState )
        {
        ///Register for EXECUTING state transition.
        ///This method just inserts a message in Event Q, which is checked in the callback
        ///The sempahore passed is signalled by the callback
        ret = RegisterForEvent(mCameraAdapterParameters.mHandleComp,
                               OMX_EventCmdComplete,
                               OMX_CommandStateSet,
                               OMX_StateExecuting,
                               mStartPreviewSem);

        if(ret!=NO_ERROR)
            {
            CAMHAL_LOGEB("Error in registering for event %d", ret);
            goto EXIT;
            }

        ///Switch to EXECUTING state
        eError = OMX_SendCommand(mCameraAdapterParameters.mHandleComp,
                                 OMX_CommandStateSet,
                                 OMX_StateExecuting,
                                 NULL);

        if(eError!=OMX_ErrorNone)
            {
            CAMHAL_LOGEB("OMX_SendCommand(OMX_StateExecuting)-0x%x", eError);
            }

        CAMHAL_LOGDA("+Waiting for component to go into EXECUTING state");
        ret = mStartPreviewSem.WaitTimeout(OMX_CMD_TIMEOUT);

        //If somethiing bad happened while we wait
        if (mComponentState == OMX_StateInvalid)
          {
            CAMHAL_LOGEA("Invalid State after IDLE_EXECUTING Exitting!!!");
            goto EXIT;
          }

        if ( NO_ERROR == ret )
            {
            CAMHAL_LOGDA("+Great. Component went into executing state!!");
            }
        else
            {
            ret |= RemoveEvent(mCameraAdapterParameters.mHandleComp,
                               OMX_EventCmdComplete,
                               OMX_CommandStateSet,
                               OMX_StateExecuting,
                               NULL);
            CAMHAL_LOGDA("Timeout expired on executing state switch!");
            goto EXIT;
            }

        mComponentState = OMX_StateExecuting;

        }

    mStateSwitchLock.unlock();

    apply3Asettings(mParameters3A);
    //Queue all the buffers on preview port
    for(int index=0;index< mPreviewData->mMaxQueueable;index++)
        {
        CAMHAL_LOGDB("Queuing buffer on Preview port - 0x%x", (uint32_t)mPreviewData->mBufferHeader[index]->pBuffer);
        eError = OMX_FillThisBuffer(mCameraAdapterParameters.mHandleComp,
                    (OMX_BUFFERHEADERTYPE*)mPreviewData->mBufferHeader[index]);
        if(eError!=OMX_ErrorNone)
            {
            CAMHAL_LOGEB("OMX_FillThisBuffer-0x%x", eError);
            }
        mFramesWithDucati++;
#ifdef DEGUG_LOG
        mBuffersWithDucati.add((uint32_t)mPreviewData->mBufferHeader[index]->pBuffer,1);
#endif
        GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);
        }

    if ( mMeasurementEnabled )
        {

        for(int index=0;index< mPreviewData->mNumBufs;index++)
            {
            CAMHAL_LOGDB("Queuing buffer on Measurement port - 0x%x", (uint32_t) measurementData->mBufferHeader[index]->pBuffer);
            eError = OMX_FillThisBuffer(mCameraAdapterParameters.mHandleComp,
                            (OMX_BUFFERHEADERTYPE*) measurementData->mBufferHeader[index]);
            if(eError!=OMX_ErrorNone)
                {
                CAMHAL_LOGEB("OMX_FillThisBuffer-0x%x", eError);
                }
            GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);
            }

        }

    // Enable Ancillary data. The nDCCStatus field is used to signify
    // whether the preview frame is a snapshot
    if ( OMX_ErrorNone == eError)
        {
        ret =  setExtraData(true, OMX_ALL, OMX_AncillaryData);
        }


    if ( mPending3Asettings )
        apply3Asettings(mParameters3A);

    // enable focus callbacks just once here
    // fixes an issue with slow callback registration in Ducati
    if ( NO_ERROR == ret ) {
        ret = setFocusCallback(true);
    }

    //reset frame rate estimates
    mFPS = 0.0f;
    mLastFPS = 0.0f;
    // start frame count from 0. i.e first frame after
    // startPreview will be the 0th reference frame
    // this way we will wait for second frame until
    // takePicture/autoFocus is allowed to run. we
    // are seeing SetConfig/GetConfig fail after
    // calling after the first frame and not failing
    // after the second frame
    mFrameCount = -1;
    mLastFrameCount = 0;
    mIter = 1;
    mLastFPSTime = systemTime();

    LOG_FUNCTION_NAME_EXIT;

    return (ret | ErrorUtils::omxToAndroidError(eError));

    EXIT:

    CAMHAL_LOGEB("Exiting function %s because of ret %d eError=%x", __FUNCTION__, ret, eError);
    performCleanupAfterError();
    mStateSwitchLock.unlock();
    LOG_FUNCTION_NAME_EXIT;

    return (ret | ErrorUtils::omxToAndroidError(eError));

}

status_t OMXCameraAdapter::stopPreview()
{
    LOG_FUNCTION_NAME;

    OMX_ERRORTYPE eError = OMX_ErrorNone;
    status_t ret = NO_ERROR;

    OMXCameraPortParameters *mCaptureData , *mPreviewData, *measurementData;
    mCaptureData = mPreviewData = measurementData = NULL;

    mPreviewData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mPrevPortIndex];
    mCaptureData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mImagePortIndex];
    measurementData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mMeasurementPortIndex];

   if (mAdapterState == LOADED_PREVIEW_STATE) {
       // Something happened in CameraHal between UseBuffers and startPreview
       // this means that state switch is still locked..so we need to unlock else
       // deadlock will occur on the next start preview
       mStateSwitchLock.unlock();
       return NO_ERROR;
   }

    if ( mComponentState != OMX_StateExecuting )
        {
        CAMHAL_LOGEA("Calling StopPreview() when not in EXECUTING state");
        LOG_FUNCTION_NAME_EXIT;
        return NO_INIT;
        }

    {
        Mutex::Autolock lock(mFrameCountMutex);
        // we should wait for the first frame to come before trying to stopPreview...if not
        // we might put OMXCamera in a bad state (IDLE->LOADED timeout). Seeing this a lot
        // after a capture
        if (mFrameCount < 1) {
            // I want to wait for at least two frames....
            mFrameCount = -1;

            // first frame may time some time to come...so wait for an adequate amount of time
            // which 2 * OMX_CAPTURE_TIMEOUT * 1000 will cover.
            ret = mFirstFrameCondition.waitRelative(mFrameCountMutex,
                                                    (nsecs_t) 2 * OMX_CAPTURE_TIMEOUT * 1000);
        }
        // even if we timeout waiting for the first frame...go ahead with trying to stop preview
        // signal anybody that might be waiting
        mFrameCount = 0;
        mFirstFrameCondition.broadcast();
    }

    ret = cancelAutoFocus();
    if(ret!=NO_ERROR)
    {
        CAMHAL_LOGEB("Error canceling autofocus %d", ret);
        // Error, but we probably still want to continue to stop preview
    }

    OMX_CONFIG_FOCUSASSISTTYPE focusAssist;
    OMX_INIT_STRUCT_PTR (&focusAssist, OMX_CONFIG_FOCUSASSISTTYPE);
    focusAssist.nPortIndex = OMX_ALL;
    focusAssist.bFocusAssist = OMX_FALSE;
    CAMHAL_LOGDB("Configuring AF Assist mode 0x%x", focusAssist.bFocusAssist);
    eError =  OMX_SetConfig(mCameraAdapterParameters.mHandleComp,
                            (OMX_INDEXTYPE) OMX_IndexConfigFocusAssist,
                            &focusAssist);
    if ( OMX_ErrorNone != eError )
        {
        CAMHAL_LOGEB("Error while configuring AF Assist mode 0x%x", eError);
        }
    else
        {
        CAMHAL_LOGDA("Camera AF Assist  mode configured successfully");
        }

    if ( 0 != mStopPreviewSem.Count() )
        {
        CAMHAL_LOGEB("Error mStopPreviewSem semaphore count %d", mStopPreviewSem.Count());
        LOG_FUNCTION_NAME_EXIT;
        return NO_INIT;
        }

    ret = disableImagePort();
    if ( NO_ERROR != ret ) {
        CAMHAL_LOGEB("disable image port failed 0x%x", ret);
        goto EXIT;
    }

    CAMHAL_LOGDB("Average framerate: %f", mFPS);

    //Avoid state switching of the OMX Component
    ret = flushBuffers();
    if ( NO_ERROR != ret )
        {
        CAMHAL_LOGEB("Flush Buffers failed 0x%x", ret);
        goto EXIT;
        }

    ///Register for Preview port Disable event
    ret = RegisterForEvent(mCameraAdapterParameters.mHandleComp,
                           OMX_EventCmdComplete,
                           OMX_CommandPortDisable,
                           mCameraAdapterParameters.mPrevPortIndex,
                           mStopPreviewSem);

    ///Disable Preview Port
    eError = OMX_SendCommand(mCameraAdapterParameters.mHandleComp,
                             OMX_CommandPortDisable,
                             mCameraAdapterParameters.mPrevPortIndex,
                             NULL);

    ///Free the OMX Buffers
    for ( int i = 0 ; i < mPreviewData->mNumBufs ; i++ )
        {
        eError = OMX_FreeBuffer(mCameraAdapterParameters.mHandleComp,
                                mCameraAdapterParameters.mPrevPortIndex,
                                mPreviewData->mBufferHeader[i]);

        if(eError!=OMX_ErrorNone)
            {
            CAMHAL_LOGEB("OMX_FreeBuffer - %x", eError);
            }
        GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);
        }

    if ( mMeasurementEnabled )
        {

            for ( int i = 0 ; i < measurementData->mNumBufs ; i++ )
                {
                eError = OMX_FreeBuffer(mCameraAdapterParameters.mHandleComp,
                                        mCameraAdapterParameters.mMeasurementPortIndex,
                                        measurementData->mBufferHeader[i]);
                if(eError!=OMX_ErrorNone)
                    {
                    CAMHAL_LOGEB("OMX_FreeBuffer - %x", eError);
                    }
                GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);
                }

            {
            Mutex::Autolock lock(mPreviewDataBufferLock);
            mPreviewDataBuffersAvailable.clear();
            }

        }

    CAMHAL_LOGDA("Disabling preview port");
    ret = mStopPreviewSem.WaitTimeout(OMX_CMD_TIMEOUT);

    //If somethiing bad happened while we wait
    if (mComponentState == OMX_StateInvalid)
      {
        CAMHAL_LOGEA("Invalid State after Disabling preview port Exitting!!!");
        goto EXIT;
      }

    if ( NO_ERROR == ret )
        {
        CAMHAL_LOGDA("Preview port disabled");
        }
    else
        {
        ret |= RemoveEvent(mCameraAdapterParameters.mHandleComp,
                           OMX_EventCmdComplete,
                           OMX_CommandPortDisable,
                           mCameraAdapterParameters.mPrevPortIndex,
                           NULL);
        CAMHAL_LOGEA("Timeout expired on preview port disable");
        goto EXIT;
        }

        {
        Mutex::Autolock lock(mPreviewBufferLock);
        ///Clear all the available preview buffers
        mPreviewBuffersAvailable.clear();
        }

    switchToLoaded();


    mFirstTimeInit = true;
    mPendingCaptureSettings = 0;
    mFramesWithDucati = 0;
    mFramesWithDisplay = 0;
    mFramesWithEncoder = 0;

    LOG_FUNCTION_NAME_EXIT;

    return (ret | ErrorUtils::omxToAndroidError(eError));

EXIT:
    CAMHAL_LOGEB("Exiting function %s because of ret %d eError=%x", __FUNCTION__, ret, eError);
    {
    Mutex::Autolock lock(mPreviewBufferLock);
    ///Clear all the available preview buffers
    mPreviewBuffersAvailable.clear();
    }
    performCleanupAfterError();
    LOG_FUNCTION_NAME_EXIT;
    return (ret | ErrorUtils::omxToAndroidError(eError));

}

status_t OMXCameraAdapter::setSensorOverclock(bool enable)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONFIG_BOOLEANTYPE bOMX;

    LOG_FUNCTION_NAME;

    if ( OMX_StateLoaded != mComponentState )
        {
        CAMHAL_LOGDA("OMX component is not in loaded state");
        return ret;
        }

    if ( NO_ERROR == ret )
        {
        OMX_INIT_STRUCT_PTR (&bOMX, OMX_CONFIG_BOOLEANTYPE);

        if ( enable )
            {
            bOMX.bEnabled = OMX_TRUE;
            }
        else
            {
            bOMX.bEnabled = OMX_FALSE;
            }

        CAMHAL_LOGDB("Configuring Sensor overclock mode 0x%x", bOMX.bEnabled);
        eError = OMX_SetParameter(mCameraAdapterParameters.mHandleComp, ( OMX_INDEXTYPE ) OMX_TI_IndexParamSensorOverClockMode, &bOMX);
        if ( OMX_ErrorNone != eError )
            {
            CAMHAL_LOGEB("Error while setting Sensor overclock 0x%x", eError);
            ret = BAD_VALUE;
            }
        else
            {
            mSensorOverclock = enable;
            }
        }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::printComponentVersion(OMX_HANDLETYPE handle)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_VERSIONTYPE compVersion;
    char compName[OMX_MAX_STRINGNAME_SIZE];
    char *currentUUID = NULL;
    size_t offset = 0;

    LOG_FUNCTION_NAME;

    if ( NULL == handle )
        {
        CAMHAL_LOGEB("Invalid OMX Handle =0x%x",  ( unsigned int ) handle);
        ret = -EINVAL;
        }

    mCompUUID[0] = 0;

    if ( NO_ERROR == ret )
        {
        eError = OMX_GetComponentVersion(handle,
                                      compName,
                                      &compVersion,
                                      &mCompRevision,
                                      &mCompUUID
                                    );
        if ( OMX_ErrorNone != eError )
            {
            CAMHAL_LOGEB("OMX_GetComponentVersion returned 0x%x", eError);
            ret = BAD_VALUE;
            }
        }

    if ( NO_ERROR == ret )
        {
        CAMHAL_LOGVB("OMX Component name: [%s]", compName);
        CAMHAL_LOGVB("OMX Component version: [%u]", ( unsigned int ) compVersion.nVersion);
        CAMHAL_LOGVB("Spec version: [%u]", ( unsigned int ) mCompRevision.nVersion);
        CAMHAL_LOGVB("Git Commit ID: [%s]", mCompUUID);
        currentUUID = ( char * ) mCompUUID;
        }

    if ( NULL != currentUUID )
        {
        offset = strlen( ( const char * ) mCompUUID) + 1;
        if ( (int)currentUUID + (int)offset - (int)mCompUUID < OMX_MAX_STRINGNAME_SIZE )
            {
            currentUUID += offset;
            CAMHAL_LOGVB("Git Branch: [%s]", currentUUID);
            }
        else
            {
            ret = BAD_VALUE;
            }
    }

    if ( NO_ERROR == ret )
        {
        offset = strlen( ( const char * ) currentUUID) + 1;

        if ( (int)currentUUID + (int)offset - (int)mCompUUID < OMX_MAX_STRINGNAME_SIZE )
            {
            currentUUID += offset;
            CAMHAL_LOGVB("Build date and time: [%s]", currentUUID);
            }
        else
            {
            ret = BAD_VALUE;
            }
        }

    if ( NO_ERROR == ret )
        {
        offset = strlen( ( const char * ) currentUUID) + 1;

        if ( (int)currentUUID + (int)offset - (int)mCompUUID < OMX_MAX_STRINGNAME_SIZE )
            {
            currentUUID += offset;
            CAMHAL_LOGVB("Build description: [%s]", currentUUID);
            }
        else
            {
            ret = BAD_VALUE;
            }
        }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::autoFocus()
{
    status_t ret = NO_ERROR;
    TIUTILS::Message msg;

    LOG_FUNCTION_NAME;

    {
        Mutex::Autolock lock(mFrameCountMutex);
        if (mFrameCount < 1) {
            // first frame may time some time to come...so wait for an adequate amount of time
            // which 2 * OMX_CAPTURE_TIMEOUT * 1000 will cover.
            ret = mFirstFrameCondition.waitRelative(mFrameCountMutex,
                                                    (nsecs_t) 2 * OMX_CAPTURE_TIMEOUT * 1000);
            if ((NO_ERROR != ret) || (mFrameCount == 0)) {
                goto EXIT;
            }
        }
    }

    msg.command = CommandHandler::CAMERA_PERFORM_AUTOFOCUS;
    msg.arg1 = mErrorNotifier;
    ret = mCommandHandler->put(&msg);

 EXIT:

    LOG_FUNCTION_NAME;

    return ret;
}

status_t OMXCameraAdapter::takePicture()
{
    status_t ret = NO_ERROR;
    TIUTILS::Message msg;

    LOG_FUNCTION_NAME;

    {
        Mutex::Autolock lock(mFrameCountMutex);
        if (mFrameCount < 1) {
            // first frame may time some time to come...so wait for an adequate amount of time
            // which 2 * OMX_CAPTURE_TIMEOUT * 1000 will cover.
            ret = mFirstFrameCondition.waitRelative(mFrameCountMutex,
                                                   (nsecs_t) 2 * OMX_CAPTURE_TIMEOUT * 1000);
            if ((NO_ERROR != ret) || (mFrameCount == 0)) {
                goto EXIT;
            }
        }
    }

    msg.command = CommandHandler::CAMERA_START_IMAGE_CAPTURE;
    msg.arg1 = mErrorNotifier;
    ret = mCommandHandler->put(&msg);

 EXIT:
    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::startVideoCapture()
{
    return BaseCameraAdapter::startVideoCapture();
}

status_t OMXCameraAdapter::stopVideoCapture()
{
    return BaseCameraAdapter::stopVideoCapture();
}

//API to get the frame size required to be allocated. This size is used to override the size passed
//by camera service when VSTAB/VNF is turned ON for example
status_t OMXCameraAdapter::getFrameSize(size_t &width, size_t &height)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONFIG_RECTTYPE tFrameDim;

    LOG_FUNCTION_NAME;

    OMX_INIT_STRUCT_PTR (&tFrameDim, OMX_CONFIG_RECTTYPE);
    tFrameDim.nPortIndex = mCameraAdapterParameters.mPrevPortIndex;

    if ( mOMXStateSwitch )
        {
        ret = switchToLoaded();
        if ( NO_ERROR != ret )
            {
            CAMHAL_LOGEB("switchToLoaded() failed 0x%x", ret);
            goto exit;
            }

        mOMXStateSwitch = false;
        }

    if ( OMX_StateLoaded == mComponentState )
        {

        ret = setLDC(mIPP);
        if ( NO_ERROR != ret )
            {
            CAMHAL_LOGEB("setLDC() failed %d", ret);
            LOG_FUNCTION_NAME_EXIT;
            goto exit;
            }

        ret = setNSF(mIPP);
        if ( NO_ERROR != ret )
            {
            CAMHAL_LOGEB("setNSF() failed %d", ret);
            LOG_FUNCTION_NAME_EXIT;
            goto exit;
            }

        ret = setCaptureMode(mCapMode);
        if ( NO_ERROR != ret )
            {
            CAMHAL_LOGEB("setCaptureMode() failed %d", ret);
            }

        if(mCapMode == OMXCameraAdapter::VIDEO_MODE)
            {
            if ( NO_ERROR == ret )
                {
                ///Enable/Disable Video Noise Filter
                ret = enableVideoNoiseFilter(mVnfEnabled);
                }

            if ( NO_ERROR != ret)
                {
                CAMHAL_LOGEB("Error configuring VNF %x", ret);
                }

            if ( NO_ERROR == ret )
                {
                ///Enable/Disable Video Stabilization
                ret = enableVideoStabilization(mVstabEnabled);
                }

            if ( NO_ERROR != ret)
                {
                CAMHAL_LOGEB("Error configuring VSTAB %x", ret);
                }
             }
        else
            {
            if ( NO_ERROR == ret )
                {
                ///Enable/Disable Video Noise Filter
                ret = enableVideoNoiseFilter(false);
                }

            if ( NO_ERROR != ret)
                {
                CAMHAL_LOGEB("Error configuring VNF %x", ret);
                }

            if ( NO_ERROR == ret )
                {
                ///Enable/Disable Video Stabilization
                ret = enableVideoStabilization(false);
                }

            if ( NO_ERROR != ret)
                {
                CAMHAL_LOGEB("Error configuring VSTAB %x", ret);
                }
            }

        }

    ret = setSensorOrientation(mSensorOrientation);
    if ( NO_ERROR != ret )
        {
        CAMHAL_LOGEB("Error configuring Sensor Orientation %x", ret);
        mSensorOrientation = 0;
        }

    if ( NO_ERROR == ret )
        {
        eError = OMX_GetParameter(mCameraAdapterParameters.mHandleComp, ( OMX_INDEXTYPE ) OMX_TI_IndexParam2DBufferAllocDimension, &tFrameDim);
        if ( OMX_ErrorNone == eError)
            {
            width = tFrameDim.nWidth;
            height = tFrameDim.nHeight;
            }
        }

exit:

    CAMHAL_LOGDB("Required frame size %dx%d", width, height);
    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::getFrameDataSize(size_t &dataFrameSize, size_t bufferCount)
{
    status_t ret = NO_ERROR;
    OMX_PARAM_PORTDEFINITIONTYPE portCheck;
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    LOG_FUNCTION_NAME;

    if ( OMX_StateLoaded != mComponentState )
        {
        CAMHAL_LOGEA("Calling getFrameDataSize() when not in LOADED state");
        dataFrameSize = 0;
        ret = BAD_VALUE;
        }

    if ( NO_ERROR == ret  )
        {
        OMX_INIT_STRUCT_PTR(&portCheck, OMX_PARAM_PORTDEFINITIONTYPE);
        portCheck.nPortIndex = mCameraAdapterParameters.mMeasurementPortIndex;

        eError = OMX_GetParameter(mCameraAdapterParameters.mHandleComp, OMX_IndexParamPortDefinition, &portCheck);
        if ( OMX_ErrorNone != eError )
            {
            CAMHAL_LOGEB("OMX_GetParameter on OMX_IndexParamPortDefinition returned: 0x%x", eError);
            dataFrameSize = 0;
            ret = BAD_VALUE;
            }
        }

    if ( NO_ERROR == ret )
        {
        portCheck.nBufferCountActual = bufferCount;
        eError = OMX_SetParameter(mCameraAdapterParameters.mHandleComp, OMX_IndexParamPortDefinition, &portCheck);
        if ( OMX_ErrorNone != eError )
            {
            CAMHAL_LOGEB("OMX_SetParameter on OMX_IndexParamPortDefinition returned: 0x%x", eError);
            dataFrameSize = 0;
            ret = BAD_VALUE;
            }
        }

    if ( NO_ERROR == ret  )
        {
        eError = OMX_GetParameter(mCameraAdapterParameters.mHandleComp, OMX_IndexParamPortDefinition, &portCheck);
        if ( OMX_ErrorNone != eError )
            {
            CAMHAL_LOGEB("OMX_GetParameter on OMX_IndexParamPortDefinition returned: 0x%x", eError);
            ret = BAD_VALUE;
            }
        else
            {
            mCameraAdapterParameters.mCameraPortParams[portCheck.nPortIndex].mBufSize = portCheck.nBufferSize;
            dataFrameSize = portCheck.nBufferSize;
            }
        }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

void OMXCameraAdapter::onOrientationEvent(uint32_t orientation, uint32_t tilt)
{
    LOG_FUNCTION_NAME;

    static const unsigned int DEGREES_TILT_IGNORE = 45;
    int device_orientation = 0;
    int mount_orientation = 0;
    const char *facing_direction = NULL;

    // if tilt angle is greater than DEGREES_TILT_IGNORE
    // we are going to ignore the orientation returned from
    // sensor. the orientation returned from sensor is not
    // reliable. Value of DEGREES_TILT_IGNORE may need adjusting
    if (tilt > DEGREES_TILT_IGNORE) {
        return;
    }

    if (mCapabilities) {
        if (mCapabilities->get(CameraProperties::ORIENTATION_INDEX)) {
            mount_orientation = atoi(mCapabilities->get(CameraProperties::ORIENTATION_INDEX));
        }
        facing_direction = mCapabilities->get(CameraProperties::FACING_INDEX);
    }

    // calculate device orientation relative to the sensor orientation
    // front camera display is mirrored...needs to be accounted for when orientation
    // is 90 or 270...since this will result in a flip on orientation otherwise
    if (facing_direction && !strcmp(facing_direction, TICameraParameters::FACING_FRONT) &&
        (orientation == 90 || orientation == 270)) {
        device_orientation = (orientation - mount_orientation + 360) % 360;
    } else {  // back-facing camera
        device_orientation = (orientation + mount_orientation) % 360;
    }

    if (device_orientation != mDeviceOrientation) {
        mDeviceOrientation = device_orientation;

        mFaceDetectionLock.lock();
        if (mFaceDetectionRunning) {
            // restart face detection with new rotation
            setFaceDetection(true, mDeviceOrientation);
        }
        mFaceDetectionLock.unlock();
    }
    CAMHAL_LOGVB("orientation = %d tilt = %d device_orientation = %d", orientation, tilt, mDeviceOrientation);

    LOG_FUNCTION_NAME_EXIT;
}

/* Application callback Functions */
/*========================================================*/
/* @ fn SampleTest_EventHandler :: Application callback   */
/*========================================================*/
OMX_ERRORTYPE OMXCameraAdapterEventHandler(OMX_IN OMX_HANDLETYPE hComponent,
                                          OMX_IN OMX_PTR pAppData,
                                          OMX_IN OMX_EVENTTYPE eEvent,
                                          OMX_IN OMX_U32 nData1,
                                          OMX_IN OMX_U32 nData2,
                                          OMX_IN OMX_PTR pEventData)
{
    LOG_FUNCTION_NAME;

    CAMHAL_LOGDB("Event %d", eEvent);

    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMXCameraAdapter *oca = (OMXCameraAdapter*)pAppData;
    ret = oca->OMXCameraAdapterEventHandler(hComponent, eEvent, nData1, nData2, pEventData);

    LOG_FUNCTION_NAME_EXIT;
    return ret;
}

/* Application callback Functions */
/*========================================================*/
/* @ fn SampleTest_EventHandler :: Application callback   */
/*========================================================*/
OMX_ERRORTYPE OMXCameraAdapter::OMXCameraAdapterEventHandler(OMX_IN OMX_HANDLETYPE hComponent,
                                          OMX_IN OMX_EVENTTYPE eEvent,
                                          OMX_IN OMX_U32 nData1,
                                          OMX_IN OMX_U32 nData2,
                                          OMX_IN OMX_PTR pEventData)
{

    LOG_FUNCTION_NAME;

    OMX_ERRORTYPE eError = OMX_ErrorNone;
    CAMHAL_LOGDB("+OMX_Event %x, %d %d", eEvent, (int)nData1, (int)nData2);

    switch (eEvent) {
        case OMX_EventCmdComplete:
            CAMHAL_LOGDB("+OMX_EventCmdComplete %d %d", (int)nData1, (int)nData2);

            if (OMX_CommandStateSet == nData1) {
                mCameraAdapterParameters.mState = (OMX_STATETYPE) nData2;

            } else if (OMX_CommandFlush == nData1) {
                CAMHAL_LOGDB("OMX_CommandFlush received for port %d", (int)nData2);

            } else if (OMX_CommandPortDisable == nData1) {
                CAMHAL_LOGDB("OMX_CommandPortDisable received for port %d", (int)nData2);

            } else if (OMX_CommandPortEnable == nData1) {
                CAMHAL_LOGDB("OMX_CommandPortEnable received for port %d", (int)nData2);

            } else if (OMX_CommandMarkBuffer == nData1) {
                ///This is not used currently
            }

            CAMHAL_LOGDA("-OMX_EventCmdComplete");
        break;

        case OMX_EventIndexSettingChanged:
            CAMHAL_LOGDB("OMX_EventIndexSettingChanged event received data1 0x%x, data2 0x%x",
                            ( unsigned int ) nData1, ( unsigned int ) nData2);
            break;

        case OMX_EventError:
            CAMHAL_LOGDB("OMX interface failed to execute OMX command %d", (int)nData1);
            CAMHAL_LOGDA("See OMX_INDEXTYPE for reference");
            if ( NULL != mErrorNotifier && ( ( OMX_U32 ) OMX_ErrorHardware == nData1 ) && mComponentState != OMX_StateInvalid)
              {
                CAMHAL_LOGEA("***Got Fatal Error Notification***\n");
                mComponentState = OMX_StateInvalid;
                /*
                Remove any unhandled events and
                unblock any waiting semaphores
                */
                if ( !mEventSignalQ.isEmpty() )
                  {
                    for (unsigned int i = 0 ; i < mEventSignalQ.size(); i++ )
                      {
                        CAMHAL_LOGEB("***Removing %d EVENTS***** \n", mEventSignalQ.size());
                        //remove from queue and free msg
                        TIUTILS::Message *msg = mEventSignalQ.itemAt(i);
                        if ( NULL != msg )
                          {
                            Semaphore *sem  = (Semaphore*) msg->arg3;
                            if ( sem )
                              {
                                sem->Signal();
                              }
                            free(msg);
                          }
                      }
                    mEventSignalQ.clear();
                  }
                ///Report Error to App
                mErrorNotifier->errorNotify(CAMERA_ERROR_FATAL);
              }
            break;

        case OMX_EventMark:
        break;

        case OMX_EventPortSettingsChanged:
        break;

        case OMX_EventBufferFlag:
        break;

        case OMX_EventResourcesAcquired:
        break;

        case OMX_EventComponentResumed:
        break;

        case OMX_EventDynamicResourcesAvailable:
        break;

        case OMX_EventPortFormatDetected:
        break;

        default:
        break;
    }

    ///Signal to the thread(s) waiting that the event has occured
    SignalEvent(hComponent, eEvent, nData1, nData2, pEventData);

   LOG_FUNCTION_NAME_EXIT;
   return eError;

    EXIT:

    CAMHAL_LOGEB("Exiting function %s because of eError=%x", __FUNCTION__, eError);
    LOG_FUNCTION_NAME_EXIT;
    return eError;
}

OMX_ERRORTYPE OMXCameraAdapter::SignalEvent(OMX_IN OMX_HANDLETYPE hComponent,
                                          OMX_IN OMX_EVENTTYPE eEvent,
                                          OMX_IN OMX_U32 nData1,
                                          OMX_IN OMX_U32 nData2,
                                          OMX_IN OMX_PTR pEventData)
{
    Mutex::Autolock lock(mEventLock);
    TIUTILS::Message *msg;
    bool eventSignalled = false;

    LOG_FUNCTION_NAME;

    if ( !mEventSignalQ.isEmpty() )
        {
        CAMHAL_LOGDA("Event queue not empty");

        for ( unsigned int i = 0 ; i < mEventSignalQ.size() ; i++ )
            {
            msg = mEventSignalQ.itemAt(i);
            if ( NULL != msg )
                {
                if( ( msg->command != 0 || msg->command == ( unsigned int ) ( eEvent ) )
                    && ( !msg->arg1 || ( OMX_U32 ) msg->arg1 == nData1 )
                    && ( !msg->arg2 || ( OMX_U32 ) msg->arg2 == nData2 )
                    && msg->arg3)
                    {
                    Semaphore *sem  = (Semaphore*) msg->arg3;
                    CAMHAL_LOGDA("Event matched, signalling sem");
                    mEventSignalQ.removeAt(i);
                    //Signal the semaphore provided
                    sem->Signal();
                    free(msg);
                    eventSignalled = true;
                    break;
                    }
                }
            }
        }
    else
        {
        CAMHAL_LOGDA("Event queue empty!!!");
        }

    // Special handling for any unregistered events
    if (!eventSignalled) {
        // Handling for focus callback
        if ((nData2 == OMX_IndexConfigCommonFocusStatus) &&
            (eEvent == (OMX_EVENTTYPE) OMX_EventIndexSettingChanged)) {
                TIUTILS::Message msg;
                msg.command = OMXCallbackHandler::CAMERA_FOCUS_STATUS;
                msg.arg1 = NULL;
                msg.arg2 = NULL;
                mOMXCallbackHandler->put(&msg);
        }
    }

    LOG_FUNCTION_NAME_EXIT;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXCameraAdapter::RemoveEvent(OMX_IN OMX_HANDLETYPE hComponent,
                                            OMX_IN OMX_EVENTTYPE eEvent,
                                            OMX_IN OMX_U32 nData1,
                                            OMX_IN OMX_U32 nData2,
                                            OMX_IN OMX_PTR pEventData)
{
  Mutex::Autolock lock(mEventLock);
  TIUTILS::Message *msg;
  LOG_FUNCTION_NAME;

  if ( !mEventSignalQ.isEmpty() )
    {
      CAMHAL_LOGDA("Event queue not empty");

      for ( unsigned int i = 0 ; i < mEventSignalQ.size() ; i++ )
        {
          msg = mEventSignalQ.itemAt(i);
          if ( NULL != msg )
            {
              if( ( msg->command != 0 || msg->command == ( unsigned int ) ( eEvent ) )
                  && ( !msg->arg1 || ( OMX_U32 ) msg->arg1 == nData1 )
                  && ( !msg->arg2 || ( OMX_U32 ) msg->arg2 == nData2 )
                  && msg->arg3)
                {
                  Semaphore *sem  = (Semaphore*) msg->arg3;
                  CAMHAL_LOGDA("Event matched, signalling sem");
                  mEventSignalQ.removeAt(i);
                  free(msg);
                  break;
                }
            }
        }
    }
  else
    {
      CAMHAL_LOGEA("Event queue empty!!!");
    }
  LOG_FUNCTION_NAME_EXIT;

  return OMX_ErrorNone;
}


status_t OMXCameraAdapter::RegisterForEvent(OMX_IN OMX_HANDLETYPE hComponent,
                                          OMX_IN OMX_EVENTTYPE eEvent,
                                          OMX_IN OMX_U32 nData1,
                                          OMX_IN OMX_U32 nData2,
                                          OMX_IN Semaphore &semaphore)
{
    status_t ret = NO_ERROR;
    ssize_t res;
    Mutex::Autolock lock(mEventLock);

    LOG_FUNCTION_NAME;
    TIUTILS::Message * msg = ( struct TIUTILS::Message * ) malloc(sizeof(struct TIUTILS::Message));
    if ( NULL != msg )
        {
        msg->command = ( unsigned int ) eEvent;
        msg->arg1 = ( void * ) nData1;
        msg->arg2 = ( void * ) nData2;
        msg->arg3 = ( void * ) &semaphore;
        msg->arg4 =  ( void * ) hComponent;
        res = mEventSignalQ.add(msg);
        if ( NO_MEMORY == res )
            {
            CAMHAL_LOGEA("No ressources for inserting OMX events");
            free(msg);
            ret = -ENOMEM;
            }
        }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

/*========================================================*/
/* @ fn SampleTest_EmptyBufferDone :: Application callback*/
/*========================================================*/
OMX_ERRORTYPE OMXCameraAdapterEmptyBufferDone(OMX_IN OMX_HANDLETYPE hComponent,
                                   OMX_IN OMX_PTR pAppData,
                                   OMX_IN OMX_BUFFERHEADERTYPE* pBuffHeader)
{
    LOG_FUNCTION_NAME;

    OMX_ERRORTYPE eError = OMX_ErrorNone;

    OMXCameraAdapter *oca = (OMXCameraAdapter*)pAppData;
    eError = oca->OMXCameraAdapterEmptyBufferDone(hComponent, pBuffHeader);

    LOG_FUNCTION_NAME_EXIT;
    return eError;
}


/*========================================================*/
/* @ fn SampleTest_EmptyBufferDone :: Application callback*/
/*========================================================*/
OMX_ERRORTYPE OMXCameraAdapter::OMXCameraAdapterEmptyBufferDone(OMX_IN OMX_HANDLETYPE hComponent,
                                   OMX_IN OMX_BUFFERHEADERTYPE* pBuffHeader)
{

   LOG_FUNCTION_NAME;

   LOG_FUNCTION_NAME_EXIT;

   return OMX_ErrorNone;
}

static void debugShowFPS()
{
    static int mFrameCount = 0;
    static int mLastFrameCount = 0;
    static nsecs_t mLastFpsTime = 0;
    static float mFps = 0;
    mFrameCount++;
    if (!(mFrameCount & 0x1F)) {
        nsecs_t now = systemTime();
        nsecs_t diff = now - mLastFpsTime;
        mFps = ((mFrameCount - mLastFrameCount) * float(s2ns(1))) / diff;
        mLastFpsTime = now;
        mLastFrameCount = mFrameCount;
        ALOGD("Camera %d Frames, %f FPS", mFrameCount, mFps);
    }
    // XXX: mFPS has the value we want
}

/*========================================================*/
/* @ fn SampleTest_FillBufferDone ::  Application callback*/
/*========================================================*/
OMX_ERRORTYPE OMXCameraAdapterFillBufferDone(OMX_IN OMX_HANDLETYPE hComponent,
                                   OMX_IN OMX_PTR pAppData,
                                   OMX_IN OMX_BUFFERHEADERTYPE* pBuffHeader)
{
    TIUTILS::Message msg;
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    if (UNLIKELY(mDebugFps)) {
        debugShowFPS();
    }

    OMXCameraAdapter *adapter =  ( OMXCameraAdapter * ) pAppData;
    if ( NULL != adapter )
        {
        msg.command = OMXCameraAdapter::OMXCallbackHandler::CAMERA_FILL_BUFFER_DONE;
        msg.arg1 = ( void * ) hComponent;
        msg.arg2 = ( void * ) pBuffHeader;
        adapter->mOMXCallbackHandler->put(&msg);
        }

    return eError;
}

/*========================================================*/
/* @ fn SampleTest_FillBufferDone ::  Application callback*/
/*========================================================*/
OMX_ERRORTYPE OMXCameraAdapter::OMXCameraAdapterFillBufferDone(OMX_IN OMX_HANDLETYPE hComponent,
                                   OMX_IN OMX_BUFFERHEADERTYPE* pBuffHeader)
{

    status_t  stat = NO_ERROR;
    status_t  res1, res2;
    OMXCameraPortParameters  *pPortParam;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    CameraFrame::FrameType typeOfFrame = CameraFrame::ALL_FRAMES;
    unsigned int refCount = 0;
    BaseCameraAdapter::AdapterState state, nextState;
    BaseCameraAdapter::getState(state);
    BaseCameraAdapter::getNextState(nextState);
    sp<CameraFDResult> fdResult = NULL;
    unsigned int mask = 0xFFFF;
    CameraFrame cameraFrame;
    OMX_TI_PLATFORMPRIVATE *platformPrivate;
    OMX_OTHER_EXTRADATATYPE *extraData;
    OMX_TI_ANCILLARYDATATYPE *ancillaryData = NULL;
    bool snapshotFrame = false;

    res1 = res2 = NO_ERROR;
    pPortParam = &(mCameraAdapterParameters.mCameraPortParams[pBuffHeader->nOutputPortIndex]);

    if ( !pBuffHeader || !pBuffHeader->pBuffer ) {
        CAMHAL_LOGEA("NULL Buffer from OMX");
        return OMX_ErrorNone;
    }

    if (pBuffHeader->nOutputPortIndex == OMX_CAMERA_PORT_VIDEO_OUT_PREVIEW)
        {

        if ( ( PREVIEW_ACTIVE & state ) != PREVIEW_ACTIVE )
            {
            return OMX_ErrorNone;
            }

        if ( mWaitingForSnapshot )
            {
            platformPrivate = (OMX_TI_PLATFORMPRIVATE*) pBuffHeader->pPlatformPrivate;
            extraData = getExtradata((OMX_OTHER_EXTRADATATYPE*) platformPrivate->pMetaDataBuffer,
                    (OMX_EXTRADATATYPE) OMX_AncillaryData);

            if ( NULL != extraData )
                {
                ancillaryData = (OMX_TI_ANCILLARYDATATYPE*) extraData->data;
                snapshotFrame = ancillaryData->nDCCStatus;
                mPending3Asettings |= SetFocus;
                }
            }

        recalculateFPS();
            {
            Mutex::Autolock lock(mFaceDetectionLock);
            if ( mFaceDetectionRunning && !mFaceDetectionPaused ) {
                detectFaces(pBuffHeader, fdResult, pPortParam->mWidth, pPortParam->mHeight);
                if ( NULL != fdResult.get() ) {
                    notifyFaceSubscribers(fdResult);
                    fdResult.clear();
                }
                if ( mFDSwitchAlgoPriority ) {

                    //Disable region priority and enable face priority for AF
                    setAlgoPriority(REGION_PRIORITY, FOCUS_ALGO, false);
                    setAlgoPriority(FACE_PRIORITY, FOCUS_ALGO , true);

                    //Disable Region priority and enable Face priority
                    setAlgoPriority(REGION_PRIORITY, EXPOSURE_ALGO, false);
                    setAlgoPriority(FACE_PRIORITY, EXPOSURE_ALGO, true);
                    mFDSwitchAlgoPriority = false;
                }
            }
            }

        ///Prepare the frames to be sent - initialize CameraFrame object and reference count
        // TODO(XXX): ancillary data for snapshot frame is not being sent for video snapshot
        //            if we are waiting for a snapshot and in video mode...go ahead and send
        //            this frame as a snapshot
        if( mWaitingForSnapshot &&  (mCapturedFrames > 0) &&
            (snapshotFrame || (mCapMode == VIDEO_MODE)))
            {
            typeOfFrame = CameraFrame::SNAPSHOT_FRAME;
            mask = (unsigned int)CameraFrame::SNAPSHOT_FRAME;

            // video snapshot gets ancillary data and wb info from last snapshot frame
            mCaptureAncillaryData = ancillaryData;
            mWhiteBalanceData = NULL;
            extraData = getExtradata((OMX_OTHER_EXTRADATATYPE*) platformPrivate->pMetaDataBuffer,
                                     (OMX_EXTRADATATYPE) OMX_WhiteBalance);
            if ( NULL != extraData )
                {
                mWhiteBalanceData = (OMX_TI_WHITEBALANCERESULTTYPE*) extraData->data;
                }
            }
        else
            {
            typeOfFrame = CameraFrame::PREVIEW_FRAME_SYNC;
            mask = (unsigned int)CameraFrame::PREVIEW_FRAME_SYNC;
            }

        if (mRecording)
            {
            mask |= (unsigned int)CameraFrame::VIDEO_FRAME_SYNC;
            mFramesWithEncoder++;
            }

        //ALOGV("FBD pBuffer = 0x%x", pBuffHeader->pBuffer);

        if( mWaitingForSnapshot )
          {
            mSnapshotCount++;

            if ( (mSnapshotCount == 1) &&
                 ((HIGH_SPEED == mCapMode) || (VIDEO_MODE == mCapMode)) )
              {
                notifyShutterSubscribers();
              }
          }

        stat = sendCallBacks(cameraFrame, pBuffHeader, mask, pPortParam);
        mFramesWithDisplay++;

        mFramesWithDucati--;

#ifdef DEBUG_LOG
        if(mBuffersWithDucati.indexOfKey((int)pBuffHeader->pBuffer)<0)
            {
            ALOGE("Buffer was never with Ducati!! 0x%x", pBuffHeader->pBuffer);
            for(int i=0;i<mBuffersWithDucati.size();i++) ALOGE("0x%x", mBuffersWithDucati.keyAt(i));
            }
        mBuffersWithDucati.removeItem((int)pBuffHeader->pBuffer);
#endif

        if(mDebugFcs)
            CAMHAL_LOGEB("C[%d] D[%d] E[%d]", mFramesWithDucati, mFramesWithDisplay, mFramesWithEncoder);

        stat |= advanceZoom();

        // On the fly update to 3A settings not working
        // Do not update 3A here if we are in the middle of a capture
        // or in the middle of transitioning to it
        if( mPending3Asettings &&
                ( (nextState & CAPTURE_ACTIVE) == 0 ) &&
                ( (state & CAPTURE_ACTIVE) == 0 ) )
            {
            apply3Asettings(mParameters3A);
            }

        }
    else if( pBuffHeader->nOutputPortIndex == OMX_CAMERA_PORT_VIDEO_OUT_MEASUREMENT )
        {
        typeOfFrame = CameraFrame::FRAME_DATA_SYNC;
        mask = (unsigned int)CameraFrame::FRAME_DATA_SYNC;

        stat = sendCallBacks(cameraFrame, pBuffHeader, mask, pPortParam);
       }
    else if( pBuffHeader->nOutputPortIndex == OMX_CAMERA_PORT_IMAGE_OUT_IMAGE )
        {
        OMX_COLOR_FORMATTYPE pixFormat;
        const char *valstr = NULL;

        pixFormat = mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mImagePortIndex].mColorFormat;

        if ( OMX_COLOR_FormatUnused == pixFormat )
            {
            typeOfFrame = CameraFrame::IMAGE_FRAME;
            mask = (unsigned int) CameraFrame::IMAGE_FRAME;
        } else if ( pixFormat == OMX_COLOR_FormatCbYCrY &&
                  ((mPictureFormatFromClient &&
                    !strcmp(mPictureFormatFromClient, CameraParameters::PIXEL_FORMAT_JPEG)) ||
                    !mPictureFormatFromClient) ) {
            // signals to callbacks that this needs to be coverted to jpeg
            // before returning to framework
            typeOfFrame = CameraFrame::IMAGE_FRAME;
            mask = (unsigned int) CameraFrame::IMAGE_FRAME;
            cameraFrame.mQuirks |= CameraFrame::ENCODE_RAW_YUV422I_TO_JPEG;

            // populate exif data and pass to subscribers via quirk
            // subscriber is in charge of freeing exif data
            ExifElementsTable* exif = new ExifElementsTable();
            setupEXIF_libjpeg(exif, mCaptureAncillaryData, mWhiteBalanceData);
            cameraFrame.mQuirks |= CameraFrame::HAS_EXIF_DATA;
            cameraFrame.mCookie2 = (void*) exif;
            }
        else
          {
            typeOfFrame = CameraFrame::RAW_FRAME;
            mask = (unsigned int) CameraFrame::RAW_FRAME;
          }

            pPortParam->mImageType = typeOfFrame;

            if((mCapturedFrames>0) && !mCaptureSignalled)
                {
                mCaptureSignalled = true;
                mCaptureSem.Signal();
                }

            if( ( CAPTURE_ACTIVE & state ) != CAPTURE_ACTIVE )
                {
                goto EXIT;
                }

            {
            Mutex::Autolock lock(mBracketingLock);
            if ( mBracketingEnabled )
                {
                doBracketing(pBuffHeader, typeOfFrame);
                return eError;
                }
            }

        if ( 1 > mCapturedFrames )
            {
            goto EXIT;
            }

        CAMHAL_LOGDB("Captured Frames: %d", mCapturedFrames);

        mCapturedFrames--;

        stat = sendCallBacks(cameraFrame, pBuffHeader, mask, pPortParam);

        }
    else
        {
        CAMHAL_LOGEA("Frame received for non-(preview/capture/measure) port. This is yet to be supported");
        goto EXIT;
        }

    if ( NO_ERROR != stat )
        {
        CAMHAL_LOGDB("sendFrameToSubscribers error: %d", stat);
        returnFrame(pBuffHeader->pBuffer, typeOfFrame);
        }

    return eError;

    EXIT:

    CAMHAL_LOGEB("Exiting function %s because of ret %d eError=%x", __FUNCTION__, stat, eError);

    if ( NO_ERROR != stat )
        {
        if ( NULL != mErrorNotifier )
            {
            mErrorNotifier->errorNotify(CAMERA_ERROR_UNKNOWN);
            }
        }

    return eError;
}

status_t OMXCameraAdapter::recalculateFPS()
{
    float currentFPS;

    {
        Mutex::Autolock lock(mFrameCountMutex);
        mFrameCount++;
        if (mFrameCount == 1) {
            mFirstFrameCondition.broadcast();
        }
    }

    if ( ( mFrameCount % FPS_PERIOD ) == 0 )
        {
        nsecs_t now = systemTime();
        nsecs_t diff = now - mLastFPSTime;
        currentFPS =  ((mFrameCount - mLastFrameCount) * float(s2ns(1))) / diff;
        mLastFPSTime = now;
        mLastFrameCount = mFrameCount;

        if ( 1 == mIter )
            {
            mFPS = currentFPS;
            }
        else
            {
            //cumulative moving average
            mFPS = mLastFPS + (currentFPS - mLastFPS)/mIter;
            }

        mLastFPS = mFPS;
        mIter++;
        }

    return NO_ERROR;
}

status_t OMXCameraAdapter::sendFrame(CameraFrame &frame)
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;


    if ( NO_ERROR == ret )
        {
        ret = sendFrameToSubscribers(&frame);
        }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::sendCallBacks(CameraFrame frame, OMX_IN OMX_BUFFERHEADERTYPE *pBuffHeader, unsigned int mask, OMXCameraPortParameters *port)
{
  status_t ret = NO_ERROR;

  LOG_FUNCTION_NAME;

  if ( NULL == port)
    {
      CAMHAL_LOGEA("Invalid portParam");
      return -EINVAL;
    }

  if ( NULL == pBuffHeader )
    {
      CAMHAL_LOGEA("Invalid Buffer header");
      return -EINVAL;
    }

  Mutex::Autolock lock(mSubscriberLock);

  //frame.mFrameType = typeOfFrame;
  frame.mFrameMask = mask;
  frame.mBuffer = pBuffHeader->pBuffer;
  frame.mLength = pBuffHeader->nFilledLen;
  frame.mAlignment = port->mStride;
  frame.mOffset = pBuffHeader->nOffset;
  frame.mWidth = port->mWidth;
  frame.mHeight = port->mHeight;
  frame.mYuv[0] = NULL;
  frame.mYuv[1] = NULL;

  if ( onlyOnce && mRecording )
    {
      mTimeSourceDelta = (pBuffHeader->nTimeStamp * 1000) - systemTime(SYSTEM_TIME_MONOTONIC);
      onlyOnce = false;
    }

  frame.mTimestamp = (pBuffHeader->nTimeStamp * 1000) - mTimeSourceDelta;

  ret = setInitFrameRefCount(frame.mBuffer, mask);

  if (ret != NO_ERROR) {
     CAMHAL_LOGDB("Error in setInitFrameRefCount %d", ret);
  } else {
      ret = sendFrameToSubscribers(&frame);
  }

  CAMHAL_LOGVB("B 0x%x T %llu", frame.mBuffer, pBuffHeader->nTimeStamp);

  LOG_FUNCTION_NAME_EXIT;

  return ret;
}

status_t OMXCameraAdapter::initCameraFrame( CameraFrame &frame,
                                            OMX_IN OMX_BUFFERHEADERTYPE *pBuffHeader,
                                            int typeOfFrame,
                                            OMXCameraPortParameters *port)
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    if ( NULL == port)
        {
        CAMHAL_LOGEA("Invalid portParam");
        return -EINVAL;
        }

    if ( NULL == pBuffHeader )
        {
        CAMHAL_LOGEA("Invalid Buffer header");
        return -EINVAL;
        }

    frame.mFrameType = typeOfFrame;
    frame.mBuffer = pBuffHeader->pBuffer;
    frame.mLength = pBuffHeader->nFilledLen;
    frame.mAlignment = port->mStride;
    frame.mOffset = pBuffHeader->nOffset;
    frame.mWidth = port->mWidth;
    frame.mHeight = port->mHeight;

    // Timestamp in pBuffHeader->nTimeStamp is derived on DUCATI side, which is
    // is not  same time value as derived using systemTime. It would be ideal to use
    // exactly same time source across Android and Ducati, which is limited by
    // system now. So, workaround for now is to find the time offset between the two
    // time sources and compensate the difference, along with the latency involved
    // in camera buffer reaching CameraHal. Also, Do timeset offset calculation only
    // when recording is in progress, when nTimestamp will be populated by Camera
    if ( onlyOnce && mRecording )
        {
        mTimeSourceDelta = (pBuffHeader->nTimeStamp * 1000) - systemTime(SYSTEM_TIME_MONOTONIC);
        mTimeSourceDelta += kCameraBufferLatencyNs;
        onlyOnce = false;
        }

    // Calculating the new video timestamp based on offset from ducati source.
    frame.mTimestamp = (pBuffHeader->nTimeStamp * 1000) - mTimeSourceDelta;

        LOG_FUNCTION_NAME_EXIT;

    return ret;
}

bool OMXCameraAdapter::CommandHandler::Handler()
{
    TIUTILS::Message msg;
    volatile int forever = 1;
    status_t stat;
    ErrorNotifier *errorNotify = NULL;

    LOG_FUNCTION_NAME;

    while ( forever )
        {
        stat = NO_ERROR;
        CAMHAL_LOGDA("Handler: waiting for messsage...");
        TIUTILS::MessageQueue::waitForMsg(&mCommandMsgQ, NULL, NULL, -1);
        {
        Mutex::Autolock lock(mLock);
        mCommandMsgQ.get(&msg);
        }
        CAMHAL_LOGDB("msg.command = %d", msg.command);
        switch ( msg.command ) {
            case CommandHandler::CAMERA_START_IMAGE_CAPTURE:
            {
                stat = mCameraAdapter->startImageCapture();
                break;
            }
            case CommandHandler::CAMERA_PERFORM_AUTOFOCUS:
            {
                stat = mCameraAdapter->doAutoFocus();
                break;
            }
            case CommandHandler::COMMAND_EXIT:
            {
                CAMHAL_LOGDA("Exiting command handler");
                forever = 0;
                break;
            }
            case CommandHandler::CAMERA_SWITCH_TO_EXECUTING:
            {
              stat = mCameraAdapter->doSwitchToExecuting();
              break;
            }
        }

        }

    LOG_FUNCTION_NAME_EXIT;

    return false;
}

bool OMXCameraAdapter::OMXCallbackHandler::Handler()
{
    TIUTILS::Message msg;
    volatile int forever = 1;
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    while(forever){
        TIUTILS::MessageQueue::waitForMsg(&mCommandMsgQ, NULL, NULL, -1);
        {
        Mutex::Autolock lock(mLock);
        mCommandMsgQ.get(&msg);
        }

        switch ( msg.command ) {
            case OMXCallbackHandler::CAMERA_FILL_BUFFER_DONE:
            {
                ret = mCameraAdapter->OMXCameraAdapterFillBufferDone(( OMX_HANDLETYPE ) msg.arg1,
                                                                     ( OMX_BUFFERHEADERTYPE *) msg.arg2);
                break;
            }
            case OMXCallbackHandler::CAMERA_FOCUS_STATUS:
            {
                mCameraAdapter->handleFocusCallback();
                break;
            }
            case CommandHandler::COMMAND_EXIT:
            {
                CAMHAL_LOGDA("Exiting OMX callback handler");
                forever = 0;
                break;
            }
        }
    }

    LOG_FUNCTION_NAME_EXIT;
    return false;
}

status_t OMXCameraAdapter::setExtraData(bool enable, OMX_U32 nPortIndex, OMX_EXT_EXTRADATATYPE eType) {
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONFIG_EXTRADATATYPE extraDataControl;

    LOG_FUNCTION_NAME;

    if ( ( OMX_StateInvalid == mComponentState ) ||
         ( NULL == mCameraAdapterParameters.mHandleComp ) ) {
        CAMHAL_LOGEA("OMX component is in invalid state");
        return -EINVAL;
    }

    OMX_INIT_STRUCT_PTR (&extraDataControl, OMX_CONFIG_EXTRADATATYPE);

    extraDataControl.nPortIndex = nPortIndex;
    extraDataControl.eExtraDataType = eType;
    extraDataControl.eCameraView = OMX_2D;

    if (enable) {
        extraDataControl.bEnable = OMX_TRUE;
    } else {
        extraDataControl.bEnable = OMX_FALSE;
    }

    eError =  OMX_SetConfig(mCameraAdapterParameters.mHandleComp,
                           (OMX_INDEXTYPE) OMX_IndexConfigOtherExtraDataControl,
                            &extraDataControl);

    LOG_FUNCTION_NAME_EXIT;

    return (ret | ErrorUtils::omxToAndroidError(eError));
}


OMX_OTHER_EXTRADATATYPE *OMXCameraAdapter::getExtradata(OMX_OTHER_EXTRADATATYPE *extraData, OMX_EXTRADATATYPE type)
{
  if ( NULL != extraData )
      {
      while ( extraData->nDataSize != 0 )
          {
          if ( type == extraData->eType )
              {
              return extraData;
              }
          extraData = (OMX_OTHER_EXTRADATATYPE*) ((char*)extraData + extraData->nSize);
          }
      }
  // Required extradata type wasn't found
  return NULL;
}

OMXCameraAdapter::OMXCameraAdapter(size_t sensor_index)
{
    LOG_FUNCTION_NAME;

    mOmxInitialized = false;
    mComponentState = OMX_StateInvalid;
    mSensorIndex = sensor_index;
    mPictureRotation = 0;
    // Initial values
    mTimeSourceDelta = 0;
    onlyOnce = true;

    mInitSem.Create(0);
    mFlushSem.Create(0);
    mUsePreviewDataSem.Create(0);
    mUsePreviewSem.Create(0);
    mUseCaptureSem.Create(0);
    mStartPreviewSem.Create(0);
    mStopPreviewSem.Create(0);
    mStartCaptureSem.Create(0);
    mStopCaptureSem.Create(0);
    mSwitchToLoadedSem.Create(0);
    mCaptureSem.Create(0);

    mSwitchToExecSem.Create(0);

    mCameraAdapterParameters.mHandleComp = 0;

    mUserSetExpLock = OMX_FALSE;
    mUserSetWbLock = OMX_FALSE;

    mFramesWithDucati = 0;
    mFramesWithDisplay = 0;
    mFramesWithEncoder = 0;

    LOG_FUNCTION_NAME_EXIT;
}

OMXCameraAdapter::~OMXCameraAdapter()
{
    LOG_FUNCTION_NAME;

    Mutex::Autolock lock(gAdapterLock);

    if ( mOmxInitialized ) {
        // return to OMX Loaded state
        switchToLoaded();

        // deinit the OMX
        if ( mComponentState == OMX_StateLoaded || mComponentState == OMX_StateInvalid ) {
            // free the handle for the Camera component
            if ( mCameraAdapterParameters.mHandleComp ) {
                OMX_FreeHandle(mCameraAdapterParameters.mHandleComp);
                mCameraAdapterParameters.mHandleComp = NULL;
            }
        }

        OMX_Deinit();
        mOmxInitialized = false;
    }

    //Remove any unhandled events
    if ( !mEventSignalQ.isEmpty() )
      {
        for (unsigned int i = 0 ; i < mEventSignalQ.size() ; i++ )
          {
            TIUTILS::Message *msg = mEventSignalQ.itemAt(i);
            //remove from queue and free msg
            if ( NULL != msg )
              {
                Semaphore *sem  = (Semaphore*) msg->arg3;
                sem->Signal();
                free(msg);

              }
          }
       mEventSignalQ.clear();
      }

    //Exit and free ref to command handling thread
    if ( NULL != mCommandHandler.get() )
    {
        TIUTILS::Message msg;
        msg.command = CommandHandler::COMMAND_EXIT;
        msg.arg1 = mErrorNotifier;
        mCommandHandler->clearCommandQ();
        mCommandHandler->put(&msg);
        mCommandHandler->requestExitAndWait();
        mCommandHandler.clear();
    }

    //Exit and free ref to callback handling thread
    if ( NULL != mOMXCallbackHandler.get() )
    {
        TIUTILS::Message msg;
        msg.command = OMXCallbackHandler::COMMAND_EXIT;
        //Clear all messages pending first
        mOMXCallbackHandler->clearCommandQ();
        mOMXCallbackHandler->put(&msg);
        mOMXCallbackHandler->requestExitAndWait();
        mOMXCallbackHandler.clear();
    }

    LOG_FUNCTION_NAME_EXIT;
}

extern "C" CameraAdapter* CameraAdapter_Factory(size_t sensor_index)
{
    CameraAdapter *adapter = NULL;
    Mutex::Autolock lock(gAdapterLock);

    LOG_FUNCTION_NAME;

    adapter = new OMXCameraAdapter(sensor_index);
    if ( adapter ) {
        CAMHAL_LOGDB("New OMX Camera adapter instance created for sensor %d",sensor_index);
    } else {
        CAMHAL_LOGEA("Camera adapter create failed!");
    }

    LOG_FUNCTION_NAME_EXIT;

    return adapter;
}

OMX_ERRORTYPE OMXCameraAdapter::OMXCameraGetHandle(OMX_HANDLETYPE *handle, OMX_PTR pAppData )
{
    OMX_ERRORTYPE eError = OMX_ErrorUndefined;

    for ( int i = 0; i < 5; ++i ) {
        if ( i > 0 ) {
            // sleep for 100 ms before next attempt
            usleep(100000);
        }

        // setup key parameters to send to Ducati during init
        OMX_CALLBACKTYPE oCallbacks;

        // initialize the callback handles
        oCallbacks.EventHandler    = android::OMXCameraAdapterEventHandler;
        oCallbacks.EmptyBufferDone = android::OMXCameraAdapterEmptyBufferDone;
        oCallbacks.FillBufferDone  = android::OMXCameraAdapterFillBufferDone;

        // get handle
        eError = OMX_GetHandle(handle, (OMX_STRING)"OMX.TI.DUCATI1.VIDEO.CAMERA", pAppData, &oCallbacks);
        if ( eError == OMX_ErrorNone ) {
            return OMX_ErrorNone;
        }

        CAMHAL_LOGEB("OMX_GetHandle() failed, error: 0x%x", eError);
    }

    *handle = 0;
    return eError;
}

extern "C" int CameraAdapter_Capabilities(CameraProperties::Properties* properties_array,
                                          const unsigned int starting_camera,
                                          const unsigned int max_camera) {
    int num_cameras_supported = 0;
    CameraProperties::Properties* properties = NULL;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_HANDLETYPE handle = NULL;
    OMX_TI_CAPTYPE caps;

    LOG_FUNCTION_NAME;

    Mutex::Autolock lock(gAdapterLock);

    if (!properties_array) {
        CAMHAL_LOGEB("invalid param: properties = 0x%p", properties_array);
        LOG_FUNCTION_NAME_EXIT;
        return -EINVAL;
    }

    eError = OMX_Init();
    if (eError != OMX_ErrorNone) {
      CAMHAL_LOGEB("Error OMX_Init -0x%x", eError);
      return eError;
    }

    eError = OMXCameraAdapter::OMXCameraGetHandle(&handle);
    if (eError != OMX_ErrorNone) {
        CAMHAL_LOGEB("OMX_GetHandle -0x%x", eError);
        goto EXIT;
    }

    // Continue selecting sensor and then querying OMX Camera for it's capabilities
    // When sensor select returns an error, we know to break and stop
    while (eError == OMX_ErrorNone &&
           (starting_camera + num_cameras_supported) < max_camera) {
        // sensor select
        OMX_CONFIG_SENSORSELECTTYPE sensorSelect;
        OMX_INIT_STRUCT_PTR (&sensorSelect, OMX_CONFIG_SENSORSELECTTYPE);
        sensorSelect.eSensor = (OMX_SENSORSELECT) num_cameras_supported;
        eError = OMX_SetConfig(handle, ( OMX_INDEXTYPE ) OMX_TI_IndexConfigSensorSelect, &sensorSelect);

        if ( OMX_ErrorNone != eError ) {
            break;
        }

        // get and fill capabilities
        properties = properties_array + starting_camera + num_cameras_supported;
        OMXCameraAdapter::getCaps(properties, handle);

        // need to fill facing information
        // assume that only sensor 0 is back facing
        if (num_cameras_supported == 0) {
            properties->set(CameraProperties::FACING_INDEX, TICameraParameters::FACING_BACK);
        } else {
            properties->set(CameraProperties::FACING_INDEX, TICameraParameters::FACING_FRONT);
        }

        num_cameras_supported++;
    }

 EXIT:
    // clean up
    if(handle) {
        OMX_FreeHandle(handle);
        handle=NULL;
    }
    OMX_Deinit();

    LOG_FUNCTION_NAME_EXIT;

    return num_cameras_supported;
}

};


/*--------------------Camera Adapter Class ENDS here-----------------------------*/
