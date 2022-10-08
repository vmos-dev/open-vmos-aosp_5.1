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
#include "OMXDCC.h"
#include "ErrorUtils.h"
#include "TICameraParameters.h"
#include <signal.h>
#include <math.h>

#include <cutils/properties.h>
#define UNLIKELY( exp ) (__builtin_expect( (exp) != 0, false ))
static int mDebugFps = 0;
static int mDebugFcs = 0;

#define HERE(Msg) {CAMHAL_LOGEB("--===line %d, %s===--\n", __LINE__, Msg);}

namespace Ti {
namespace Camera {

#ifdef CAMERAHAL_OMX_PROFILING

const char OMXCameraAdapter::DEFAULT_PROFILE_PATH[] = "/data/dbg/profile_data.bin";

#endif

//frames skipped before recalculating the framerate
#define FPS_PERIOD 30

android::Mutex gAdapterLock;
/*--------------------Camera Adapter Class STARTS here-----------------------------*/

status_t OMXCameraAdapter::initialize(CameraProperties::Properties* caps)
{
    LOG_FUNCTION_NAME;

    char value[PROPERTY_VALUE_MAX];
    const char *mountOrientationString = NULL;

    property_get("debug.camera.showfps", value, "0");
    mDebugFps = atoi(value);
    property_get("debug.camera.framecounts", value, "0");
    mDebugFcs = atoi(value);

#ifdef CAMERAHAL_OMX_PROFILING

    property_get("debug.camera.profile", value, "0");
    mDebugProfile = atoi(value);

#endif

    TIMM_OSAL_ERRORTYPE osalError = OMX_ErrorNone;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    status_t ret = NO_ERROR;

    mLocalVersionParam.s.nVersionMajor = 0x1;
    mLocalVersionParam.s.nVersionMinor = 0x1;
    mLocalVersionParam.s.nRevision = 0x0 ;
    mLocalVersionParam.s.nStep =  0x0;

    mPending3Asettings = 0;//E3AsettingsAll;
    mPendingCaptureSettings = 0;
    mPendingPreviewSettings = 0;
    mPendingReprocessSettings = 0;

    ret = mMemMgr.initialize();
    if ( ret != OK ) {
        CAMHAL_LOGE("MemoryManager initialization failed, error: %d", ret);
        return ret;
    }

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
    mCameraAdapterParameters.mVideoPortIndex = OMX_CAMERA_PORT_VIDEO_OUT_VIDEO;
    mCameraAdapterParameters.mVideoInPortIndex = OMX_CAMERA_PORT_VIDEO_IN_VIDEO;

    eError = OMX_Init();
    if (eError != OMX_ErrorNone) {
        CAMHAL_LOGEB("OMX_Init() failed, error: 0x%x", eError);
        return Utils::ErrorUtils::omxToAndroidError(eError);
    }
    mOmxInitialized = true;

    // Initialize the callback handles
    OMX_CALLBACKTYPE callbacks;
    callbacks.EventHandler    = Camera::OMXCameraAdapterEventHandler;
    callbacks.EmptyBufferDone = Camera::OMXCameraAdapterEmptyBufferDone;
    callbacks.FillBufferDone  = Camera::OMXCameraAdapterFillBufferDone;

    ///Get the handle to the OMX Component
    eError = OMXCameraAdapter::OMXCameraGetHandle(&mCameraAdapterParameters.mHandleComp, this, callbacks);
    if(eError != OMX_ErrorNone) {
        CAMHAL_LOGEB("OMX_GetHandle -0x%x", eError);
    }
    GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);

    mComponentState = OMX_StateLoaded;

    CAMHAL_LOGVB("OMX_GetHandle -0x%x sensor_index = %lu", eError, mSensorIndex);
    initDccFileDataSave(&mCameraAdapterParameters.mHandleComp, mCameraAdapterParameters.mPrevPortIndex);

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

#ifdef CAMERAHAL_DEBUG

    printComponentVersion(mCameraAdapterParameters.mHandleComp);

#endif

    mBracketingEnabled = false;
    mZoomBracketingEnabled = false;
    mBracketingBuffersQueuedCount = 0;
    mBracketingRange = 1;
    mLastBracetingBufferIdx = 0;
    mBracketingBuffersQueued = NULL;
    mOMXStateSwitch = false;
    mBracketingSet = false;
#ifdef CAMERAHAL_USE_RAW_IMAGE_SAVING
    mRawCapture = false;
    mYuvCapture = false;
#endif

    mCaptureSignalled = false;
    mCaptureConfigured = false;
    mReprocConfigured = false;
    mRecording = false;
    mWaitingForSnapshot = false;
    mPictureFormatFromClient = NULL;

    mCapabilitiesOpMode = MODE_MAX;
    mCapMode = INITIAL_MODE;
    mIPP = IPP_NULL;
    mVstabEnabled = false;
    mVnfEnabled = false;
    mBurstFrames = 1;
    mFlushShotConfigQueue = false;
    mPictureQuality = 100;
    mCurrentZoomIdx = 0;
    mTargetZoomIdx = 0;
    mPreviousZoomIndx = 0;
    mReturnZoomStatus = false;
    mZoomInc = 1;
    mZoomParameterIdx = 0;
    mExposureBracketingValidEntries = 0;
    mZoomBracketingValidEntries = 0;
    mSensorOverclock = false;
    mAutoConv = OMX_TI_AutoConvergenceModeMax;
    mManualConv = 0;

#ifdef CAMERAHAL_TUNA
    mIternalRecordingHint = false;
#endif

    mDeviceOrientation = 0;
    mFaceOrientation = 0;
    mCapabilities = caps;
    mZoomUpdating = false;
    mZoomUpdate = false;
    mGBCE = BRIGHTNESS_OFF;
    mGLBCE = BRIGHTNESS_OFF;
    mParameters3A.ExposureLock = OMX_FALSE;
    mParameters3A.WhiteBalanceLock = OMX_FALSE;

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

    mCapturedFrames = 0;
    mBurstFramesAccum = 0;
    mBurstFramesQueued = 0;

    //update the mDeviceOrientation with the sensor mount orientation.
    //So that the face detect will work before onOrientationEvent()
    //get triggered.
    CAMHAL_ASSERT(mCapabilities);
    mountOrientationString = mCapabilities->get(CameraProperties::ORIENTATION_INDEX);
    CAMHAL_ASSERT(mountOrientationString);
    mDeviceOrientation = atoi(mountOrientationString);
    mFaceOrientation = atoi(mountOrientationString);

    if (mSensorIndex != 2) {
        mCapabilities->setMode(MODE_HIGH_SPEED);
    }

    if (mCapabilities->get(CameraProperties::SUPPORTED_ZOOM_STAGES) != NULL) {
        mMaxZoomSupported = mCapabilities->getInt(CameraProperties::SUPPORTED_ZOOM_STAGES) + 1;
    } else {
        mMaxZoomSupported = 1;
    }

    // initialize command handling thread
    if(mCommandHandler.get() == NULL)
        mCommandHandler = new CommandHandler(this);

    if ( NULL == mCommandHandler.get() )
    {
        CAMHAL_LOGEA("Couldn't create command handler");
        return NO_MEMORY;
    }

    ret = mCommandHandler->run("CallbackThread", android::PRIORITY_URGENT_DISPLAY);
    if ( ret != NO_ERROR )
    {
        if( ret == INVALID_OPERATION){
            CAMHAL_LOGDA("command handler thread already runnning!!");
            ret = NO_ERROR;
        } else {
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

    ret = mOMXCallbackHandler->run("OMXCallbackThread", android::PRIORITY_URGENT_DISPLAY);
    if ( ret != NO_ERROR )
    {
        if( ret == INVALID_OPERATION){
            CAMHAL_LOGDA("omx callback handler thread already runnning!!");
            ret = NO_ERROR;
        } else {
            CAMHAL_LOGEA("Couldn't run omx callback handler thread");
            return ret;
        }
    }

    OMX_INIT_STRUCT_PTR (&mRegionPriority, OMX_TI_CONFIG_3A_REGION_PRIORITY);
    OMX_INIT_STRUCT_PTR (&mFacePriority, OMX_TI_CONFIG_3A_FACE_PRIORITY);
    mRegionPriority.nPortIndex = OMX_ALL;
    mFacePriority.nPortIndex = OMX_ALL;

    //Setting this flag will that the first setParameter call will apply all 3A settings
    //and will not conditionally apply based on current values.
    mFirstTimeInit = true;

    //Flag to avoid calling setVFramerate() before OMX_SetParameter(OMX_IndexParamPortDefinition)
    //Ducati will return an error otherwise.
    mSetFormatDone = false;

    memset(mExposureBracketingValues, 0, EXP_BRACKET_RANGE*sizeof(int));
    memset(mZoomBracketingValues, 0, ZOOM_BRACKET_RANGE*sizeof(int));
    mMeasurementEnabled = false;
    mFaceDetectionRunning = false;
    mFaceDetectionPaused = false;
    mFDSwitchAlgoPriority = false;

    metadataLastAnalogGain = -1;
    metadataLastExposureTime = -1;

    memset(&mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mImagePortIndex], 0, sizeof(OMXCameraPortParameters));
    memset(&mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mPrevPortIndex], 0, sizeof(OMXCameraPortParameters));
    memset(&mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mVideoPortIndex], 0, sizeof(OMXCameraPortParameters));
    memset(&mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mVideoInPortIndex], 0, sizeof(OMXCameraPortParameters));

    // initialize 3A defaults
    mParameters3A.Effect = getLUTvalue_HALtoOMX(OMXCameraAdapter::DEFAULT_EFFECT, EffLUT);
    mParameters3A.FlashMode = getLUTvalue_HALtoOMX(OMXCameraAdapter::DEFAULT_FLASH_MODE, FlashLUT);
    mParameters3A.SceneMode = getLUTvalue_HALtoOMX(OMXCameraAdapter::DEFAULT_SCENE_MODE, SceneLUT);
    mParameters3A.EVCompensation = atoi(OMXCameraAdapter::DEFAULT_EV_COMPENSATION);
    mParameters3A.Focus = getLUTvalue_HALtoOMX(OMXCameraAdapter::DEFAULT_FOCUS_MODE, FocusLUT);
    mParameters3A.ISO = getLUTvalue_HALtoOMX(OMXCameraAdapter::DEFAULT_ISO_MODE, IsoLUT);
    mParameters3A.Flicker = getLUTvalue_HALtoOMX(OMXCameraAdapter::DEFAULT_ANTIBANDING, FlickerLUT);
    mParameters3A.Brightness = atoi(OMXCameraAdapter::DEFAULT_BRIGHTNESS);
    mParameters3A.Saturation = atoi(OMXCameraAdapter::DEFAULT_SATURATION) - SATURATION_OFFSET;
    mParameters3A.Sharpness = atoi(OMXCameraAdapter::DEFAULT_SHARPNESS) - SHARPNESS_OFFSET;
    mParameters3A.Contrast = atoi(OMXCameraAdapter::DEFAULT_CONTRAST) - CONTRAST_OFFSET;
    mParameters3A.WhiteBallance = getLUTvalue_HALtoOMX(OMXCameraAdapter::DEFAULT_WB, WBalLUT);
    mParameters3A.Exposure = getLUTvalue_HALtoOMX(OMXCameraAdapter::DEFAULT_EXPOSURE_MODE, ExpLUT);
    mParameters3A.ExposureLock = OMX_FALSE;
    mParameters3A.FocusLock = OMX_FALSE;
    mParameters3A.WhiteBalanceLock = OMX_FALSE;

    mParameters3A.ManualExposure = 0;
    mParameters3A.ManualExposureRight = 0;
    mParameters3A.ManualGain = 0;
    mParameters3A.ManualGainRight = 0;

    mParameters3A.AlgoExternalGamma = OMX_FALSE;
    mParameters3A.AlgoNSF1 = OMX_TRUE;
    mParameters3A.AlgoNSF2 = OMX_TRUE;
    mParameters3A.AlgoSharpening = OMX_TRUE;
    mParameters3A.AlgoThreeLinColorMap = OMX_TRUE;
    mParameters3A.AlgoGIC = OMX_TRUE;
    memset(&mParameters3A.mGammaTable, 0, sizeof(mParameters3A.mGammaTable));

    LOG_FUNCTION_NAME_EXIT;
    return Utils::ErrorUtils::omxToAndroidError(eError);

    EXIT:

    CAMHAL_LOGDB("Exiting function %s because of ret %d eError=%x", __FUNCTION__, ret, eError);
    performCleanupAfterError();
    LOG_FUNCTION_NAME_EXIT;
    return Utils::ErrorUtils::omxToAndroidError(eError);
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
        ret = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mImagePortIndex];
        break;
    case CameraFrame::RAW_FRAME:
        if (mRawCapture) {
            ret = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mVideoPortIndex];
        } else {
            ret = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mImagePortIndex];
        }
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

status_t OMXCameraAdapter::fillThisBuffer(CameraBuffer * frameBuf, CameraFrame::FrameType frameType)
{
    LOG_FUNCTION_NAME;

    status_t ret = NO_ERROR;
    OMXCameraPortParameters *port = NULL;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    BaseCameraAdapter::AdapterState state;
    BaseCameraAdapter::getState(state);
    bool isCaptureFrame = false;

    if ( ( PREVIEW_ACTIVE & state ) != PREVIEW_ACTIVE )
        {
        return NO_INIT;
        }

    if ( NULL == frameBuf )
        {
        return -EINVAL;
        }

    isCaptureFrame = (CameraFrame::IMAGE_FRAME == frameType) ||
                     (CameraFrame::RAW_FRAME == frameType);

    if ( NO_ERROR == ret )
        {
        port = getPortParams(frameType);
        if ( NULL == port )
            {
            CAMHAL_LOGEB("Invalid frameType 0x%x", frameType);
            ret = -EINVAL;
            }
        }

    if ( NO_ERROR == ret ) {
        for ( int i = 0 ; i < port->mNumBufs ; i++) {
            if ((CameraBuffer *) port->mBufferHeader[i]->pAppPrivate == frameBuf) {
                if ( isCaptureFrame && !mBracketingEnabled ) {
                    android::AutoMutex lock(mBurstLock);
                    if ((1 > mCapturedFrames) && !mBracketingEnabled && (mCapMode != CP_CAM)) {
                        // Signal end of image capture
                        if ( NULL != mEndImageCaptureCallback) {
                            mEndImageCaptureCallback(mEndCaptureData);
                        }
                        port->mStatus[i] = OMXCameraPortParameters::IDLE;
                        return NO_ERROR;
                    } else if (mBurstFramesQueued >= mBurstFramesAccum) {
                        port->mStatus[i] = OMXCameraPortParameters::IDLE;
                        return NO_ERROR;
                    }
                    mBurstFramesQueued++;
                }
                port->mStatus[i] = OMXCameraPortParameters::FILL;
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
    return (ret | Utils::ErrorUtils::omxToAndroidError(eError));
}

void OMXCameraAdapter::setParamS3D(OMX_U32 port, const char *valstr)
{
    OMXCameraPortParameters *cap;

    LOG_FUNCTION_NAME;

    cap = &mCameraAdapterParameters.mCameraPortParams[port];
    if (valstr != NULL)
        {
        if (strcmp(valstr, TICameraParameters::S3D_TB_FULL) == 0)
            {
            cap->mFrameLayoutType = OMX_TI_StereoFrameLayoutTopBottom;
            }
        else if (strcmp(valstr, TICameraParameters::S3D_SS_FULL) == 0)
            {
            cap->mFrameLayoutType = OMX_TI_StereoFrameLayoutLeftRight;
            }
        else if (strcmp(valstr, TICameraParameters::S3D_TB_SUBSAMPLED) == 0)
            {
            cap->mFrameLayoutType = OMX_TI_StereoFrameLayoutTopBottomSubsample;
            }
        else if (strcmp(valstr, TICameraParameters::S3D_SS_SUBSAMPLED) == 0)
            {
            cap->mFrameLayoutType = OMX_TI_StereoFrameLayoutLeftRightSubsample;
            }
        else
            {
            cap->mFrameLayoutType = OMX_TI_StereoFrameLayout2D;
            }
        }
    else
        {
        cap->mFrameLayoutType = OMX_TI_StereoFrameLayout2D;
        }

    LOG_FUNCTION_NAME_EXIT;
}

status_t OMXCameraAdapter::setParameters(const android::CameraParameters &params)
{
    LOG_FUNCTION_NAME;

    int mode = 0;
    status_t ret = NO_ERROR;
    bool updateImagePortParams = false;
    int minFramerate, maxFramerate, frameRate;
    const char *valstr = NULL;
    int w, h;
    OMX_COLOR_FORMATTYPE pixFormat;
    BaseCameraAdapter::AdapterState state;
    BaseCameraAdapter::getState(state);

    ///@todo Include more camera parameters
    if ( (valstr = params.getPreviewFormat()) != NULL ) {
        if(strcmp(valstr, android::CameraParameters::PIXEL_FORMAT_YUV420SP) == 0 ||
           strcmp(valstr, android::CameraParameters::PIXEL_FORMAT_YUV420P) == 0 ||
           strcmp(valstr, android::CameraParameters::PIXEL_FORMAT_YUV422I) == 0) {
            CAMHAL_LOGDA("YUV420SP format selected");
            pixFormat = OMX_COLOR_FormatYUV420PackedSemiPlanar;
        } else if(strcmp(valstr, android::CameraParameters::PIXEL_FORMAT_RGB565) == 0) {
            CAMHAL_LOGDA("RGB565 format selected");
            pixFormat = OMX_COLOR_Format16bitRGB565;
        } else {
            CAMHAL_LOGDA("Invalid format, CbYCrY format selected as default");
            pixFormat = OMX_COLOR_FormatCbYCrY;
        }
    } else {
        CAMHAL_LOGEA("Preview format is NULL, defaulting to CbYCrY");
        pixFormat = OMX_COLOR_FormatCbYCrY;
    }

    OMXCameraPortParameters *cap;
    cap = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mPrevPortIndex];

    params.getPreviewSize(&w, &h);
    frameRate = params.getPreviewFrameRate();
    params.getPreviewFpsRange(&minFramerate, &maxFramerate);
    minFramerate /= CameraHal::VFR_SCALE;
    maxFramerate /= CameraHal::VFR_SCALE;
    if ( ( 0 < minFramerate ) && ( 0 < maxFramerate ) ) {
        if ( minFramerate > maxFramerate ) {
            CAMHAL_LOGEA(" Min FPS set higher than MAX. So setting MIN and MAX to the higher value");
            maxFramerate = minFramerate;
        }

        if ( 0 >= frameRate ) {
            frameRate = maxFramerate;
        }

        if ( ( cap->mMinFrameRate != (OMX_U32) minFramerate ) ||
             ( cap->mMaxFrameRate != (OMX_U32) maxFramerate ) ) {
            cap->mMinFrameRate = minFramerate;
            cap->mMaxFrameRate = maxFramerate;
            setVFramerate(cap->mMinFrameRate, cap->mMaxFrameRate);
        }
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

#ifdef CAMERAHAL_TUNA
    valstr = params.get(TICameraParameters::KEY_RECORDING_HINT);
    if (!valstr || (valstr && (strcmp(valstr, android::CameraParameters::FALSE)))) {
        mIternalRecordingHint = false;
    } else {
        mIternalRecordingHint = true;
    }
#endif

#ifdef OMAP_ENHANCEMENT
    if ( (valstr = params.get(TICameraParameters::KEY_MEASUREMENT_ENABLE)) != NULL )
        {
        if (strcmp(valstr, android::CameraParameters::TRUE) == 0)
            {
            mMeasurementEnabled = true;
            }
        else if (strcmp(valstr, android::CameraParameters::FALSE) == 0)
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

#ifdef OMAP_ENHANCEMENT_S3D
    setParamS3D(mCameraAdapterParameters.mPrevPortIndex,
               params.get(TICameraParameters::KEY_S3D_PRV_FRAME_LAYOUT));
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

    if ( MODE_MAX != mCapabilitiesOpMode ) {
        mCapabilities->setMode(mCapabilitiesOpMode);
    }

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
        CAMHAL_LOGE("Unable to open file %s: %s", fn, strerror(fd));
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


#ifdef CAMERAHAL_USE_RAW_IMAGE_SAVING
static status_t saveBufferToFile(const void *buf, int size, const char *filename)
{
    if (size < 0) {
        CAMHAL_LOGE("Wrong buffer size: %d", size);
        return BAD_VALUE;
    }

    const int fd = open(filename, O_CREAT | O_WRONLY | O_SYNC | O_TRUNC, 0644);
    if (fd < 0) {
        CAMHAL_LOGE("ERROR: %s, Unable to save raw file", strerror(fd));
        return BAD_VALUE;
    }

    if (write(fd, buf, size) != (signed)size) {
        CAMHAL_LOGE("ERROR: Unable to write to raw file: %s ", strerror(errno));
        close(fd);
        return NO_MEMORY;
    }

    CAMHAL_LOGD("buffer=%p, size=%d stored at %s", buf, size, filename);

    close(fd);
    return OK;
}
#endif


void OMXCameraAdapter::getParameters(android::CameraParameters& params)
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

       if (mCapabilities) {
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
       valstr_supported = mParams.get(android::CameraParameters::KEY_SUPPORTED_WHITE_BALANCE);
       if (valstr && valstr_supported && strstr(valstr_supported, valstr))
           params.set(android::CameraParameters::KEY_WHITE_BALANCE , valstr);

       valstr = getLUTvalue_OMXtoHAL(mParameters3A.FlashMode, FlashLUT);
       valstr_supported = mParams.get(android::CameraParameters::KEY_SUPPORTED_FLASH_MODES);
       if (valstr && valstr_supported && strstr(valstr_supported, valstr))
           params.set(android::CameraParameters::KEY_FLASH_MODE, valstr);

       if ((mParameters3A.Focus == OMX_IMAGE_FocusControlAuto) &&
           ( (mCapMode != OMXCameraAdapter::VIDEO_MODE) &&
             (mCapMode != OMXCameraAdapter::VIDEO_MODE_HQ) ) ) {
           valstr = android::CameraParameters::FOCUS_MODE_CONTINUOUS_PICTURE;
       } else {
           valstr = getLUTvalue_OMXtoHAL(mParameters3A.Focus, FocusLUT);
       }
       valstr_supported = mParams.get(android::CameraParameters::KEY_SUPPORTED_FOCUS_MODES);
       if (valstr && valstr_supported && strstr(valstr_supported, valstr))
           params.set(android::CameraParameters::KEY_FOCUS_MODE, valstr);
    }

    //Query focus distances only when focus is running
    if ( ( AF_ACTIVE & state ) ||
         ( NULL == mParameters.get(android::CameraParameters::KEY_FOCUS_DISTANCES) ) )
        {
        updateFocusDistances(params);
        }
    else
        {
        params.set(android::CameraParameters::KEY_FOCUS_DISTANCES,
                   mParameters.get(android::CameraParameters::KEY_FOCUS_DISTANCES));
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
    android::AutoMutex lock(mZoomLock);
    //Immediate zoom should not be avaialable while smooth zoom is running
    if ( ZOOM_ACTIVE & state )
        {
        if ( mZoomParameterIdx != mCurrentZoomIdx )
            {
            mZoomParameterIdx += mZoomInc;
            }
        params.set(android::CameraParameters::KEY_ZOOM, mZoomParameterIdx);
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
        params.set(android::CameraParameters::KEY_ZOOM, mCurrentZoomIdx);
        }
    }

    //Populate current lock status
    if ( mUserSetExpLock || mParameters3A.ExposureLock ) {
        params.set(android::CameraParameters::KEY_AUTO_EXPOSURE_LOCK,
                android::CameraParameters::TRUE);
    } else {
        params.set(android::CameraParameters::KEY_AUTO_EXPOSURE_LOCK,
                android::CameraParameters::FALSE);
    }

    if ( mUserSetWbLock || mParameters3A.WhiteBalanceLock ) {
        params.set(android::CameraParameters::KEY_AUTO_WHITEBALANCE_LOCK,
                android::CameraParameters::TRUE);
    } else {
        params.set(android::CameraParameters::KEY_AUTO_WHITEBALANCE_LOCK,
                android::CameraParameters::FALSE);
    }

    // Update Picture size capabilities dynamically
    params.set(android::CameraParameters::KEY_SUPPORTED_PICTURE_SIZES,
                mCapabilities->get(CameraProperties::SUPPORTED_PICTURE_SIZES));

    // Update framerate capabilities dynamically
    params.set(android::CameraParameters::KEY_SUPPORTED_PREVIEW_FRAME_RATES,
               mCapabilities->get(CameraProperties::SUPPORTED_PREVIEW_FRAME_RATES));

    params.set(TICameraParameters::KEY_FRAMERATES_EXT_SUPPORTED,
               mCapabilities->get(CameraProperties::SUPPORTED_PREVIEW_FRAME_RATES_EXT));

    params.set(android::CameraParameters::KEY_SUPPORTED_PREVIEW_FPS_RANGE,
               mCapabilities->get(CameraProperties::FRAMERATE_RANGE_SUPPORTED));

    params.set(TICameraParameters::KEY_FRAMERATE_RANGES_EXT_SUPPORTED,
               mCapabilities->get(CameraProperties::FRAMERATE_RANGE_EXT_SUPPORTED));

    LOG_FUNCTION_NAME_EXIT;
}

status_t OMXCameraAdapter::setupTunnel(uint32_t SliceHeight, uint32_t EncoderHandle, uint32_t width, uint32_t height) {
    LOG_FUNCTION_NAME;

    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_HANDLETYPE *encoderHandle = (OMX_HANDLETYPE *)EncoderHandle;

    CAMHAL_LOGDB("\n %s: SliceHeight:%d, EncoderHandle:%d width:%d height:%d \n", __FUNCTION__, SliceHeight, EncoderHandle, width, height);

    if (SliceHeight == 0){
        CAMHAL_LOGEA("\n\n #### Encoder Slice Height Not received, Dont Setup Tunnel $$$$\n\n");
        return BAD_VALUE;
    }

    if (encoderHandle == NULL) {
        CAMHAL_LOGEA("Encoder Handle not set \n\n");
        return BAD_VALUE;
    }

    if ( 0 != mInitSem.Count() ) {
        CAMHAL_LOGEB("Error mInitSem semaphore count %d", mInitSem.Count());
        LOG_FUNCTION_NAME_EXIT;
        return NO_INIT;
    }

    // Register for port enable event
    ret = RegisterForEvent(mCameraAdapterParameters.mHandleComp,
            OMX_EventCmdComplete,
            OMX_CommandPortEnable,
            mCameraAdapterParameters.mVideoPortIndex,
            mInitSem);
    if(ret != NO_ERROR) {
        CAMHAL_LOGEB("Error in registering for event %d", ret);
        return UNKNOWN_ERROR;
    }

    // Enable VIDEO Port
    eError = OMX_SendCommand(mCameraAdapterParameters.mHandleComp,
            OMX_CommandPortEnable,
            mCameraAdapterParameters.mVideoPortIndex,
            NULL);
    if(eError != OMX_ErrorNone) {
        CAMHAL_LOGEB("OMX_SendCommand(OMX_CommandPortEnable) -0x%x", eError);
        return BAD_VALUE;
    }

    // Wait for the port enable event to occur
    ret = mInitSem.WaitTimeout(OMX_CMD_TIMEOUT);
    if ( NO_ERROR == ret ) {
        CAMHAL_LOGDA("-Port enable event arrived");
    } else {
        ret |= RemoveEvent(mCameraAdapterParameters.mHandleComp,
                OMX_EventCmdComplete,
                OMX_CommandPortEnable,
                mCameraAdapterParameters.mVideoPortIndex,
                NULL);
        CAMHAL_LOGEA("Timeout for enabling preview port expired!");
        return UNKNOWN_ERROR;
     }

    //Set the Video Port Params
    OMX_PARAM_PORTDEFINITIONTYPE portCheck;
    OMX_INIT_STRUCT_PTR (&portCheck, OMX_PARAM_PORTDEFINITIONTYPE);
    portCheck.nPortIndex = OMX_CAMERA_PORT_VIDEO_OUT_VIDEO;
    eError = OMX_GetParameter(mCameraAdapterParameters.mHandleComp,
                                OMX_IndexParamPortDefinition, &portCheck);
    if (eError!=OMX_ErrorNone) {
        CAMHAL_LOGEB("OMX_GetParameter OMX_IndexParamPortDefinition Error - %x", eError);
    }

    portCheck.format.video.nFrameWidth = width;
    portCheck.format.video.nFrameHeight = height;
    portCheck.format.video.eColorFormat = OMX_COLOR_FormatYUV420PackedSemiPlanar;
    eError = OMX_SetParameter(mCameraAdapterParameters.mHandleComp,
            OMX_IndexParamPortDefinition, &portCheck);
    if (eError!=OMX_ErrorNone) {
        CAMHAL_LOGEB("OMX_SetParameter OMX_IndexParamPortDefinition Error- %x", eError);
    }

    //Slice  Configuration
    OMX_TI_PARAM_VTCSLICE VTCSlice;
    OMX_INIT_STRUCT_PTR(&VTCSlice, OMX_TI_PARAM_VTCSLICE);
    eError = OMX_GetParameter(mCameraAdapterParameters.mHandleComp, (OMX_INDEXTYPE)OMX_TI_IndexParamVtcSlice, &VTCSlice);
    if (eError!=OMX_ErrorNone) {
        CAMHAL_LOGEB("OMX_GetParameter OMX_TI_IndexParamVtcSlice Error - %x", eError);
    }

    VTCSlice.nSliceHeight = SliceHeight;
    eError = OMX_SetParameter(mCameraAdapterParameters.mHandleComp, (OMX_INDEXTYPE)OMX_TI_IndexParamVtcSlice, &VTCSlice);
    if (OMX_ErrorNone != eError ) {
        CAMHAL_LOGEB("OMX_SetParameter on OMX_TI_IndexParamVtcSlice returned error: 0x%x", eError);
        return BAD_VALUE;
    }

    eError = OMX_SetupTunnel(mCameraAdapterParameters.mHandleComp,
            mCameraAdapterParameters.mVideoPortIndex, encoderHandle, 0);
    if (OMX_ErrorNone != eError ) {
        CAMHAL_LOGEB("OMX_SetupTunnel returned error: 0x%x", eError);
        return BAD_VALUE;
    }

    return NO_ERROR;
}

status_t OMXCameraAdapter::setSensorQuirks(int orientation,
                                           OMXCameraPortParameters &portParams,
                                           bool &portConfigured)
{
    status_t overclockStatus = NO_ERROR;
    int sensorID = -1;
    size_t overclockWidth;
    size_t overclockHeight;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_PARAM_PORTDEFINITIONTYPE portCheck;

    LOG_FUNCTION_NAME;

    portConfigured = false;
    OMX_INIT_STRUCT_PTR (&portCheck, OMX_PARAM_PORTDEFINITIONTYPE);

    portCheck.nPortIndex = mCameraAdapterParameters.mPrevPortIndex;

    eError = OMX_GetParameter (mCameraAdapterParameters.mHandleComp,
                               OMX_IndexParamPortDefinition,
                               &portCheck);

    if ( eError != OMX_ErrorNone ) {
        CAMHAL_LOGEB("OMX_GetParameter - %x", eError);
        return Utils::ErrorUtils::omxToAndroidError(eError);
    }

    if ( ( orientation == 90 ) || ( orientation == 270 ) ) {
        overclockWidth = 1080;
        overclockHeight = 1920;
    } else {
        overclockWidth = 1920;
        overclockHeight = 1080;
    }

    sensorID = mCapabilities->getInt(CameraProperties::CAMERA_SENSOR_ID);
    if( ( ( sensorID == SENSORID_IMX060 ) &&
          ( portParams.mWidth >= overclockWidth ) &&
          ( portParams.mHeight >= overclockHeight ) &&
          ( portParams.mFrameRate >= FRAME_RATE_FULL_HD ) ) ||
          (( sensorID == SENSORID_OV14825) &&
          ( portParams.mFrameRate >= FRAME_RATE_HIGH_HD ))||
        ( ( sensorID == SENSORID_OV5640 ) &&
          ( portParams.mWidth >= overclockWidth ) &&
          ( portParams.mHeight >= overclockHeight ) ) ) {
        overclockStatus = setSensorOverclock(true);
    } else {

        //WA: If the next port resolution doesn't require
        //    sensor overclocking, but the previous resolution
        //    needed it, then we have to first set new port
        //    resolution and then disable sensor overclocking.
        if( ( ( sensorID == SENSORID_IMX060 ) &&
              ( portCheck.format.video.nFrameWidth >= overclockWidth ) &&
              ( portCheck.format.video.nFrameHeight >= overclockHeight ) &&
              ( ( portCheck.format.video.xFramerate >> 16 ) >= FRAME_RATE_FULL_HD ) ) ||
              (( sensorID == SENSORID_OV14825) &&
              (( portCheck.format.video.xFramerate >> 16) >= FRAME_RATE_HIGH_HD ))||
             ( ( sensorID == SENSORID_OV5640 ) &&
              ( portCheck.format.video.nFrameWidth >= overclockWidth ) &&
              ( portCheck.format.video.nFrameHeight >= overclockHeight ) ) ) {
            status_t ret = setFormat(mCameraAdapterParameters.mPrevPortIndex,
                                     portParams);
            if ( NO_ERROR != ret ) {
                return ret;
            }

            // Another WA: Setting the port definition will reset the VFR
            //             configuration.
            setVFramerate(portParams.mMinFrameRate, portParams.mMaxFrameRate);

            portConfigured = true;
        }

        overclockStatus = setSensorOverclock(false);
    }

    LOG_FUNCTION_NAME_EXIT;

    return overclockStatus;
}
status_t OMXCameraAdapter::setFormat(OMX_U32 port, OMXCameraPortParameters &portParams)
{
    LOG_FUNCTION_NAME;

    status_t ret = NO_ERROR;
    size_t bufferCount;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_PARAM_PORTDEFINITIONTYPE portCheck;

    OMX_INIT_STRUCT_PTR (&portCheck, OMX_PARAM_PORTDEFINITIONTYPE);

    portCheck.nPortIndex = port;

    eError = OMX_GetParameter (mCameraAdapterParameters.mHandleComp,
                                OMX_IndexParamPortDefinition, &portCheck);
    if (eError!=OMX_ErrorNone) {
        CAMHAL_LOGEB("OMX_GetParameter - %x", eError);
    }
    GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);

    if (OMX_CAMERA_PORT_VIDEO_OUT_PREVIEW == port) {
        portCheck.format.video.nFrameWidth      = portParams.mWidth;
        portCheck.format.video.nFrameHeight     = portParams.mHeight;
        portCheck.format.video.eColorFormat     = portParams.mColorFormat;
        portCheck.format.video.nStride          = portParams.mStride;

        portCheck.format.video.xFramerate       = portParams.mFrameRate<<16;
        portCheck.nBufferSize                   = portParams.mStride * portParams.mHeight;
        portCheck.nBufferCountActual = portParams.mNumBufs;
        mFocusThreshold = FOCUS_THRESHOLD * portParams.mFrameRate;
        // Used for RAW capture
    } else if (OMX_CAMERA_PORT_VIDEO_OUT_VIDEO == port) {
        portCheck.format.video.nFrameWidth      = portParams.mWidth;
        portCheck.format.video.nFrameHeight     = portParams.mHeight;
        portCheck.format.video.eColorFormat     = OMX_COLOR_FormatRawBayer10bit; // portParams.mColorFormat;
        portCheck.nBufferCountActual            = 1; // portParams.mNumBufs;
    } else if (OMX_CAMERA_PORT_IMAGE_OUT_IMAGE == port) {
        portCheck.format.image.nFrameWidth      = portParams.mWidth;
        portCheck.format.image.nFrameHeight     = portParams.mHeight;
        if (OMX_COLOR_FormatUnused == portParams.mColorFormat) {
            portCheck.format.image.eColorFormat = OMX_COLOR_FormatCbYCrY;
            if (mCodingMode == CodingJPEG) {
                portCheck.format.image.eCompressionFormat = OMX_IMAGE_CodingJPEG;
            } else if (mCodingMode == CodingJPS) {
                portCheck.format.image.eCompressionFormat = (OMX_IMAGE_CODINGTYPE) OMX_TI_IMAGE_CodingJPS;
            } else if (mCodingMode == CodingMPO) {
                portCheck.format.image.eCompressionFormat = (OMX_IMAGE_CODINGTYPE) OMX_TI_IMAGE_CodingMPO;
            } else {
                portCheck.format.image.eCompressionFormat = OMX_IMAGE_CodingUnused;
            }
        } else {
            portCheck.format.image.eColorFormat       = portParams.mColorFormat;
            portCheck.format.image.eCompressionFormat = OMX_IMAGE_CodingUnused;
        }

#ifdef CAMERAHAL_USE_RAW_IMAGE_SAVING
        // RAW + YUV Capture
        if (mYuvCapture) {
            portCheck.format.image.eColorFormat       = OMX_COLOR_FormatCbYCrY;
            portCheck.format.image.eCompressionFormat = OMX_IMAGE_CodingUnused;
        }
#endif
        //Stride for 1D tiler buffer is zero
        portCheck.format.image.nStride          =  0;
        portCheck.nBufferCountActual = portParams.mNumBufs;
     } else if (OMX_CAMERA_PORT_VIDEO_IN_VIDEO == port) {
        portCheck.format.video.nFrameWidth      = portParams.mWidth;
        portCheck.format.video.nStride          = portParams.mStride;
        portCheck.format.video.nFrameHeight     = portParams.mHeight;
        portCheck.format.video.eColorFormat     = portParams.mColorFormat;
        portCheck.format.video.xFramerate       = 30 << 16;
        portCheck.nBufferCountActual            = portParams.mNumBufs;
    } else {
        CAMHAL_LOGEB("Unsupported port index (%lu)", port);
    }

    if (( mSensorIndex == OMX_TI_StereoSensor ) && (OMX_CAMERA_PORT_VIDEO_OUT_VIDEO != port)) {
        ret = setS3DFrameLayout(port);
        if ( NO_ERROR != ret )
            {
            CAMHAL_LOGEA("Error configuring stereo 3D frame layout");
            return ret;
            }
        }

    eError = OMX_SetParameter(mCameraAdapterParameters.mHandleComp,
            OMX_IndexParamPortDefinition, &portCheck);
    if (eError!=OMX_ErrorNone) {
        CAMHAL_LOGEB("OMX_SetParameter - %x", eError);
    }
    GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);

    /* check if parameters are set correctly by calling GetParameter() */
    eError = OMX_GetParameter(mCameraAdapterParameters.mHandleComp,
            OMX_IndexParamPortDefinition, &portCheck);
    if (eError!=OMX_ErrorNone) {
        CAMHAL_LOGEB("OMX_GetParameter - %x", eError);
    }
    GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);

    portParams.mBufSize = portCheck.nBufferSize;
    portParams.mStride = portCheck.format.image.nStride;

    if (OMX_CAMERA_PORT_IMAGE_OUT_IMAGE == port) {
        CAMHAL_LOGDB("\n *** IMG Width = %ld", portCheck.format.image.nFrameWidth);
        CAMHAL_LOGDB("\n *** IMG Height = %ld", portCheck.format.image.nFrameHeight);

        CAMHAL_LOGDB("\n *** IMG IMG FMT = %x", portCheck.format.image.eColorFormat);
        CAMHAL_LOGDB("\n *** IMG portCheck.nBufferSize = %ld\n",portCheck.nBufferSize);
        CAMHAL_LOGDB("\n *** IMG portCheck.nBufferCountMin = %ld\n",
                portCheck.nBufferCountMin);
        CAMHAL_LOGDB("\n *** IMG portCheck.nBufferCountActual = %ld\n",
                portCheck.nBufferCountActual);
        CAMHAL_LOGDB("\n *** IMG portCheck.format.image.nStride = %ld\n",
                portCheck.format.image.nStride);
    } else if (OMX_CAMERA_PORT_VIDEO_OUT_PREVIEW == port) {
        CAMHAL_LOGDB("\n *** PRV Width = %ld", portCheck.format.video.nFrameWidth);
        CAMHAL_LOGDB("\n *** PRV Height = %ld", portCheck.format.video.nFrameHeight);

        CAMHAL_LOGDB("\n *** PRV IMG FMT = %x", portCheck.format.video.eColorFormat);
        CAMHAL_LOGDB("\n *** PRV portCheck.nBufferSize = %ld\n",portCheck.nBufferSize);
        CAMHAL_LOGDB("\n *** PRV portCheck.nBufferCountMin = %ld\n",
                portCheck.nBufferCountMin);
        CAMHAL_LOGDB("\n *** PRV portCheck.nBufferCountActual = %ld\n",
                portCheck.nBufferCountActual);
        CAMHAL_LOGDB("\n ***PRV portCheck.format.video.nStride = %ld\n",
                portCheck.format.video.nStride);
    } else {
        CAMHAL_LOGDB("\n *** VID Width = %ld", portCheck.format.video.nFrameWidth);
        CAMHAL_LOGDB("\n *** VID Height = %ld", portCheck.format.video.nFrameHeight);

        CAMHAL_LOGDB("\n *** VID IMG FMT = %x", portCheck.format.video.eColorFormat);
        CAMHAL_LOGDB("\n *** VID portCheck.nBufferSize = %ld\n",portCheck.nBufferSize);
        CAMHAL_LOGDB("\n *** VID portCheck.nBufferCountMin = %ld\n",
                portCheck.nBufferCountMin);
        CAMHAL_LOGDB("\n *** VID portCheck.nBufferCountActual = %ld\n",
                portCheck.nBufferCountActual);
        CAMHAL_LOGDB("\n *** VID portCheck.format.video.nStride = %ld\n",
                portCheck.format.video.nStride);
    }

    mSetFormatDone = true;

    LOG_FUNCTION_NAME_EXIT;

    return Utils::ErrorUtils::omxToAndroidError(eError);

    EXIT:

    CAMHAL_LOGEB("Exiting function %s because of eError = 0x%x", __FUNCTION__, eError);

    LOG_FUNCTION_NAME_EXIT;

    return Utils::ErrorUtils::omxToAndroidError(eError);
}

status_t OMXCameraAdapter::flushBuffers(OMX_U32 nPort)
{
    LOG_FUNCTION_NAME;

    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    if ( 0 != mFlushSem.Count() )
        {
        CAMHAL_LOGEB("Error mFlushSem semaphore count %d", mFlushSem.Count());
        LOG_FUNCTION_NAME_EXIT;
        return NO_INIT;
        }

    OMXCameraPortParameters * mPreviewData = NULL;
    mPreviewData = &mCameraAdapterParameters.mCameraPortParams[nPort];

    ///Register for the FLUSH event
    ///This method just inserts a message in Event Q, which is checked in the callback
    ///The sempahore passed is signalled by the callback
    ret = RegisterForEvent(mCameraAdapterParameters.mHandleComp,
                                OMX_EventCmdComplete,
                                OMX_CommandFlush,
                                nPort,
                                mFlushSem);
    if(ret!=NO_ERROR)
        {
        CAMHAL_LOGEB("Error in registering for event %d", ret);
        goto EXIT;
        }

    ///Send FLUSH command to preview port
    eError = OMX_SendCommand (mCameraAdapterParameters.mHandleComp,
                              OMX_CommandFlush,
                              nPort,
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
                           nPort,
                           NULL);
        CAMHAL_LOGDA("Flush event timeout expired");
        goto EXIT;
        }

    mOMXCallbackHandler->flush();

    LOG_FUNCTION_NAME_EXIT;

    return (ret | Utils::ErrorUtils::omxToAndroidError(eError));

    EXIT:
    CAMHAL_LOGEB("Exiting function %s because of ret %d eError=%x", __FUNCTION__, ret, eError);
    performCleanupAfterError();
    LOG_FUNCTION_NAME_EXIT;
    return (ret | Utils::ErrorUtils::omxToAndroidError(eError));
}

///API to give the buffers to Adapter
status_t OMXCameraAdapter::useBuffers(CameraMode mode, CameraBuffer * bufArr, int num, size_t length, unsigned int queueable)
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
            mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mImagePortIndex].mMaxQueueable = queueable;
            ret = UseBuffersCapture(bufArr, num);
            mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mImagePortIndex].mNumBufs = num;
            break;

        case CAMERA_VIDEO:
            mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mVideoPortIndex].mNumBufs =  num;
            mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mVideoPortIndex].mMaxQueueable = queueable;
            ret = UseBuffersRawCapture(bufArr, num);
            break;

        case CAMERA_MEASUREMENT:
            mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mMeasurementPortIndex].mNumBufs = num;
            mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mMeasurementPortIndex].mMaxQueueable = queueable;
            ret = UseBuffersPreviewData(bufArr, num);
            break;

        case CAMERA_REPROCESS:
            mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mVideoInPortIndex].mNumBufs = num;
            mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mVideoInPortIndex].mMaxQueueable = queueable;
            ret = UseBuffersReprocess(bufArr, num);
            break;
        }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::UseBuffersPreviewData(CameraBuffer * bufArr, int num)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMXCameraPortParameters * measurementData = NULL;
    android::AutoMutex lock(mPreviewDataBufferLock);

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
    return (ret | Utils::ErrorUtils::omxToAndroidError(eError));
}

status_t OMXCameraAdapter::switchToExecuting()
{
  status_t ret = NO_ERROR;
  Utils::Message msg;

  LOG_FUNCTION_NAME;

  mStateSwitchLock.lock();
  msg.command = CommandHandler::CAMERA_SWITCH_TO_EXECUTING;
  msg.arg1 = mErrorNotifier;
  ret = mCommandHandler->put(&msg);

  LOG_FUNCTION_NAME_EXIT;

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
  return (ret | Utils::ErrorUtils::omxToAndroidError(eError));
}

status_t OMXCameraAdapter::switchToIdle() {
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    LOG_FUNCTION_NAME;

    android::AutoMutex lock(mIdleStateSwitchLock);

    if ( mComponentState == OMX_StateIdle || mComponentState == OMX_StateLoaded  || mComponentState == OMX_StateInvalid) {
        CAMHAL_LOGDA("Already in OMX_StateIdle, OMX_Loaded state or OMX_StateInvalid state");
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

    mComponentState = OMX_StateIdle;

    return NO_ERROR;

EXIT:
    CAMHAL_LOGEB("Exiting function %s because of ret %d eError=%x", __FUNCTION__, ret, eError);
    performCleanupAfterError();
    LOG_FUNCTION_NAME_EXIT;
    return (ret | Utils::ErrorUtils::omxToAndroidError(eError));
}



status_t OMXCameraAdapter::prevPortEnable() {
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    LOG_FUNCTION_NAME;

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

    LOG_FUNCTION_NAME_EXIT;
    return (ret | Utils::ErrorUtils::omxToAndroidError(eError));

EXIT:
    CAMHAL_LOGEB("Exiting function %s because of ret %d eError=%x", __FUNCTION__, ret, eError);
    performCleanupAfterError();
    LOG_FUNCTION_NAME_EXIT;
    return (ret | Utils::ErrorUtils::omxToAndroidError(eError));
}

status_t OMXCameraAdapter::switchToLoaded(bool bPortEnableRequired) {
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    LOG_FUNCTION_NAME;

    android::AutoMutex lock(mStateSwitchLock);
    if ( mComponentState == OMX_StateLoaded  || mComponentState == OMX_StateInvalid) {
        CAMHAL_LOGDA("Already in OMX_Loaded state or OMX_StateInvalid state");
        return NO_ERROR;
    }

    if ( mComponentState != OMX_StateIdle) {
        ret = switchToIdle();
        if (ret != NO_ERROR) return ret;
    }

    if ( 0 != mSwitchToLoadedSem.Count() ) {
        CAMHAL_LOGEB("Error mSwitchToLoadedSem semaphore count %d", mSwitchToLoadedSem.Count());
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

    if ( !bPortEnableRequired ) {
        OMXCameraPortParameters *mCaptureData , *mPreviewData, *measurementData;
        mCaptureData = mPreviewData = measurementData = NULL;

        mPreviewData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mPrevPortIndex];
        mCaptureData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mImagePortIndex];
        measurementData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mMeasurementPortIndex];

        ///Free the OMX Buffers
        for ( int i = 0 ; i < mPreviewData->mNumBufs ; i++ ) {
            eError = OMX_FreeBuffer(mCameraAdapterParameters.mHandleComp,
                    mCameraAdapterParameters.mPrevPortIndex,
                    mPreviewData->mBufferHeader[i]);

            if(eError!=OMX_ErrorNone) {
                CAMHAL_LOGEB("OMX_FreeBuffer - %x", eError);
            }
            GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);
        }

        if ( mMeasurementEnabled ) {

            for ( int i = 0 ; i < measurementData->mNumBufs ; i++ ) {
                eError = OMX_FreeBuffer(mCameraAdapterParameters.mHandleComp,
                        mCameraAdapterParameters.mMeasurementPortIndex,
                        measurementData->mBufferHeader[i]);
                if(eError!=OMX_ErrorNone) {
                    CAMHAL_LOGEB("OMX_FreeBuffer - %x", eError);
                }
                GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);
            }

            {
                android::AutoMutex lock(mPreviewDataBufferLock);
                mPreviewDataBuffersAvailable.clear();
            }

        }
    }

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
    if (bPortEnableRequired == true) {
        prevPortEnable();
    }

    return (ret | Utils::ErrorUtils::omxToAndroidError(eError));

EXIT:
    CAMHAL_LOGEB("Exiting function %s because of ret %d eError=%x", __FUNCTION__, ret, eError);
    {
        android::AutoMutex lock(mPreviewBufferLock);
        ///Clear all the available preview buffers
        mPreviewBuffersAvailable.clear();
    }
    performCleanupAfterError();
    LOG_FUNCTION_NAME_EXIT;
    return (ret | Utils::ErrorUtils::omxToAndroidError(eError));
}

status_t OMXCameraAdapter::UseBuffersPreview(CameraBuffer * bufArr, int num)
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

    if ( mComponentState == OMX_StateLoaded ) {

        if (mPendingPreviewSettings & SetLDC) {
            mPendingPreviewSettings &= ~SetLDC;
            ret = setLDC(mIPP);
            if ( NO_ERROR != ret ) {
                CAMHAL_LOGEB("setLDC() failed %d", ret);
            }
        }

        if (mPendingPreviewSettings & SetNSF) {
            mPendingPreviewSettings &= ~SetNSF;
            ret = setNSF(mIPP);
            if ( NO_ERROR != ret ) {
                CAMHAL_LOGEB("setNSF() failed %d", ret);
            }
        }

        if (mPendingPreviewSettings & SetCapMode) {
            mPendingPreviewSettings &= ~SetCapMode;
            ret = setCaptureMode(mCapMode);
            if ( NO_ERROR != ret ) {
                CAMHAL_LOGEB("setCaptureMode() failed %d", ret);
            }
        }

        if( (mCapMode == OMXCameraAdapter::VIDEO_MODE) ||
            (mCapMode == OMXCameraAdapter::VIDEO_MODE_HQ) ) {

            if (mPendingPreviewSettings & SetVNF) {
                mPendingPreviewSettings &= ~SetVNF;
                ret = enableVideoNoiseFilter(mVnfEnabled);
                if ( NO_ERROR != ret){
                    CAMHAL_LOGEB("Error configuring VNF %x", ret);
                }
            }

            if (mPendingPreviewSettings & SetVSTAB) {
                mPendingPreviewSettings &= ~SetVSTAB;
                ret = enableVideoStabilization(mVstabEnabled);
                if ( NO_ERROR != ret) {
                    CAMHAL_LOGEB("Error configuring VSTAB %x", ret);
                }
            }

        }
    }

    ret = setSensorOrientation(mSensorOrientation);
    if ( NO_ERROR != ret )
        {
        CAMHAL_LOGEB("Error configuring Sensor Orientation %x", ret);
        mSensorOrientation = 0;
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
        OMX_U8 *ptr;

        ptr = (OMX_U8 *)camera_buffer_get_omx_ptr (&bufArr[index]);
        eError = OMX_UseBuffer( mCameraAdapterParameters.mHandleComp,
                                &pBufferHdr,
                                mCameraAdapterParameters.mPrevPortIndex,
                                0,
                                mPreviewData->mBufSize,
                                ptr);
        if(eError!=OMX_ErrorNone)
            {
            CAMHAL_LOGEB("OMX_UseBuffer-0x%x", eError);
            }
        GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);

        pBufferHdr->pAppPrivate = (OMX_PTR)&bufArr[index];
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
            OMX_U8 *ptr;

            ptr = (OMX_U8 *)camera_buffer_get_omx_ptr (&mPreviewDataBuffers[i]);
            eError = OMX_UseBuffer( mCameraAdapterParameters.mHandleComp,
                                    &pBufHdr,
                                    mCameraAdapterParameters.mMeasurementPortIndex,
                                    0,
                                    measurementData->mBufSize,
                                    ptr);

             if ( eError == OMX_ErrorNone )
                {
                pBufHdr->pAppPrivate = (OMX_PTR *)&mPreviewDataBuffers[i];
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

    return (ret | Utils::ErrorUtils::omxToAndroidError(eError));

    ///If there is any failure, we reach here.
    ///Here, we do any resource freeing and convert from OMX error code to Camera Hal error code
EXIT:
    mStateSwitchLock.unlock();

    CAMHAL_LOGEB("Exiting function %s because of ret %d eError=%x", __FUNCTION__, ret, eError);
    performCleanupAfterError();
    CAMHAL_LOGEB("Exiting function %s because of ret %d eError=%x", __FUNCTION__, ret, eError);

    LOG_FUNCTION_NAME_EXIT;

    return (ret | Utils::ErrorUtils::omxToAndroidError(eError));
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

    // Enable all preview mode extra data.
    if ( OMX_ErrorNone == eError) {
        ret |= setExtraData(true, mCameraAdapterParameters.mPrevPortIndex, OMX_AncillaryData);
        ret |= setExtraData(true, OMX_ALL, OMX_TI_VectShotInfo);
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

    //Queue all the buffers on preview port
    for(int index=0;index< mPreviewData->mMaxQueueable;index++)
        {
        CAMHAL_LOGDB("Queuing buffer on Preview port - 0x%x", (uint32_t)mPreviewData->mBufferHeader[index]->pBuffer);
        mPreviewData->mStatus[index] = OMXCameraPortParameters::FILL;
        eError = OMX_FillThisBuffer(mCameraAdapterParameters.mHandleComp,
                    (OMX_BUFFERHEADERTYPE*)mPreviewData->mBufferHeader[index]);
        if(eError!=OMX_ErrorNone)
            {
            CAMHAL_LOGEB("OMX_FillThisBuffer-0x%x", eError);
            }
        mFramesWithDucati++;
#ifdef CAMERAHAL_DEBUG
        mBuffersWithDucati.add((int)mPreviewData->mBufferHeader[index]->pBuffer,1);
#endif
        GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);
        }

    if ( mMeasurementEnabled )
        {

        for(int index=0;index< mPreviewData->mNumBufs;index++)
            {
            CAMHAL_LOGDB("Queuing buffer on Measurement port - 0x%x", (uint32_t) measurementData->mBufferHeader[index]->pBuffer);
            measurementData->mStatus[index] = OMXCameraPortParameters::FILL;
            eError = OMX_FillThisBuffer(mCameraAdapterParameters.mHandleComp,
                            (OMX_BUFFERHEADERTYPE*) measurementData->mBufferHeader[index]);
            if(eError!=OMX_ErrorNone)
                {
                CAMHAL_LOGEB("OMX_FillThisBuffer-0x%x", eError);
                }
            GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);
            }

        }

    setFocusCallback(true);

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
    mTunnelDestroyed = false;

    LOG_FUNCTION_NAME_EXIT;

    return (ret | Utils::ErrorUtils::omxToAndroidError(eError));

    EXIT:

    CAMHAL_LOGEB("Exiting function %s because of ret %d eError=%x", __FUNCTION__, ret, eError);
    performCleanupAfterError();
    mStateSwitchLock.unlock();
    LOG_FUNCTION_NAME_EXIT;

    return (ret | Utils::ErrorUtils::omxToAndroidError(eError));

}

status_t OMXCameraAdapter::destroyTunnel()
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
        return ALREADY_EXISTS;
    }

    if ( mComponentState != OMX_StateExecuting )
        {
        CAMHAL_LOGEA("Calling StopPreview() when not in EXECUTING state");
        LOG_FUNCTION_NAME_EXIT;
        return NO_INIT;
        }

    {
        android::AutoMutex lock(mFrameCountMutex);
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

    {
        android::AutoMutex lock(mDoAFMutex);
        mDoAFCond.broadcast();
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

    switchToIdle();

    mTunnelDestroyed = true;
    LOG_FUNCTION_NAME_EXIT;
    return (ret | Utils::ErrorUtils::omxToAndroidError(eError));

EXIT:
    CAMHAL_LOGEB("Exiting function %s because of ret %d eError=%x", __FUNCTION__, ret, eError);
    {
        android::AutoMutex lock(mPreviewBufferLock);
        ///Clear all the available preview buffers
        mPreviewBuffersAvailable.clear();
    }
    performCleanupAfterError();
    LOG_FUNCTION_NAME_EXIT;
    return (ret | Utils::ErrorUtils::omxToAndroidError(eError));

}

status_t OMXCameraAdapter::stopPreview() {
    LOG_FUNCTION_NAME;

    OMX_ERRORTYPE eError = OMX_ErrorNone;
    status_t ret = NO_ERROR;

    if (mTunnelDestroyed == false){
        ret = destroyTunnel();
        if (ret == ALREADY_EXISTS) {
            // Special case to handle invalid stopping preview in LOADED_PREVIEW_STATE
            return NO_ERROR;
        }
        if (ret != NO_ERROR) {
            CAMHAL_LOGEB(" destroyTunnel returned error ");
            return ret;
        }
    }

    mTunnelDestroyed = false;

    {
        android::AutoMutex lock(mPreviewBufferLock);
        ///Clear all the available preview buffers
        mPreviewBuffersAvailable.clear();
    }

    switchToLoaded();

    mFirstTimeInit = true;
    mPendingCaptureSettings = 0;
    mPendingReprocessSettings = 0;
    mFramesWithDucati = 0;
    mFramesWithDisplay = 0;
    mFramesWithEncoder = 0;

    LOG_FUNCTION_NAME_EXIT;

    return (ret | Utils::ErrorUtils::omxToAndroidError(eError));
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
            }
        else
            {
            mSensorOverclock = enable;
            }
        }

    LOG_FUNCTION_NAME_EXIT;

    return Utils::ErrorUtils::omxToAndroidError(eError);
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

status_t OMXCameraAdapter::setS3DFrameLayout(OMX_U32 port) const
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_TI_FRAMELAYOUTTYPE frameLayout;
    const OMXCameraPortParameters *cap =
        &mCameraAdapterParameters.mCameraPortParams[port];

    LOG_FUNCTION_NAME;

    OMX_INIT_STRUCT_PTR (&frameLayout, OMX_TI_FRAMELAYOUTTYPE);
    frameLayout.nPortIndex = port;
    eError = OMX_GetParameter(mCameraAdapterParameters.mHandleComp,
            (OMX_INDEXTYPE)OMX_TI_IndexParamStereoFrmLayout, &frameLayout);
    if (eError != OMX_ErrorNone)
        {
        CAMHAL_LOGEB("Error while getting S3D frame layout: 0x%x", eError);
        return -EINVAL;
        }

    if (cap->mFrameLayoutType == OMX_TI_StereoFrameLayoutTopBottomSubsample)
        {
        frameLayout.eFrameLayout = OMX_TI_StereoFrameLayoutTopBottom;
        frameLayout.nSubsampleRatio = 2;
        }
    else if (cap->mFrameLayoutType ==
                OMX_TI_StereoFrameLayoutLeftRightSubsample)
        {
        frameLayout.eFrameLayout = OMX_TI_StereoFrameLayoutLeftRight;
        frameLayout.nSubsampleRatio = 2;
        }
    else
        {
        frameLayout.eFrameLayout = cap->mFrameLayoutType;
        frameLayout.nSubsampleRatio = 1;
        }
    frameLayout.nSubsampleRatio = frameLayout.nSubsampleRatio << 7;

    eError = OMX_SetParameter(mCameraAdapterParameters.mHandleComp,
            (OMX_INDEXTYPE)OMX_TI_IndexParamStereoFrmLayout, &frameLayout);
    if (eError != OMX_ErrorNone)
        {
        CAMHAL_LOGEB("Error while setting S3D frame layout: 0x%x", eError);
        return -EINVAL;
        }
    else
        {
        CAMHAL_LOGDB("S3D frame layout %d applied successfully on port %lu",
                        frameLayout.eFrameLayout, port);
        }

    LOG_FUNCTION_NAME_EXIT;

    return NO_ERROR;
}

status_t OMXCameraAdapter::autoFocus()
{
    status_t ret = NO_ERROR;
    Utils::Message msg;

    LOG_FUNCTION_NAME;

    {
        android::AutoMutex lock(mFrameCountMutex);
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

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::takePicture()
{
    status_t ret = NO_ERROR;
    Utils::Message msg;

    LOG_FUNCTION_NAME;

    if (mNextState != REPROCESS_STATE) {
        android::AutoMutex lock(mFrameCountMutex);
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

    // TODO(XXX): re-using take picture to kick off reprocessing pipe
    // Need to rethink this approach during reimplementation
    if (mNextState == REPROCESS_STATE) {
        msg.command = CommandHandler::CAMERA_START_REPROCESS;
    } else {
        msg.command = CommandHandler::CAMERA_START_IMAGE_CAPTURE;
    }

    msg.arg1 = mErrorNotifier;
    msg.arg2 = cacheCaptureParameters();
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
        ret = switchToLoaded(true);
        if ( NO_ERROR != ret )
            {
            CAMHAL_LOGEB("switchToLoaded() failed 0x%x", ret);
            goto exit;
            }

        mOMXStateSwitch = false;
        }

    if ( OMX_StateLoaded == mComponentState )
        {

        if (mPendingPreviewSettings & SetLDC) {
            mPendingPreviewSettings &= ~SetLDC;
            ret = setLDC(mIPP);
            if ( NO_ERROR != ret ) {
                CAMHAL_LOGEB("setLDC() failed %d", ret);
                LOG_FUNCTION_NAME_EXIT;
                goto exit;
            }
        }

        if (mPendingPreviewSettings & SetNSF) {
            mPendingPreviewSettings &= ~SetNSF;
            ret = setNSF(mIPP);
            if ( NO_ERROR != ret ) {
                CAMHAL_LOGEB("setNSF() failed %d", ret);
                LOG_FUNCTION_NAME_EXIT;
                goto exit;
            }
        }

        if (mPendingPreviewSettings & SetCapMode) {
            mPendingPreviewSettings &= ~SetCapMode;
            ret = setCaptureMode(mCapMode);
            if ( NO_ERROR != ret ) {
                CAMHAL_LOGEB("setCaptureMode() failed %d", ret);
            }
        }

        if((mCapMode == OMXCameraAdapter::VIDEO_MODE) ||
           (mCapMode == OMXCameraAdapter::VIDEO_MODE_HQ) ) {

            if (mPendingPreviewSettings & SetVNF) {
                mPendingPreviewSettings &= ~SetVNF;
                ret = enableVideoNoiseFilter(mVnfEnabled);
                if ( NO_ERROR != ret){
                    CAMHAL_LOGEB("Error configuring VNF %x", ret);
                }
            }

            if (mPendingPreviewSettings & SetVSTAB) {
                mPendingPreviewSettings &= ~SetVSTAB;
                ret = enableVideoStabilization(mVstabEnabled);
                if ( NO_ERROR != ret) {
                    CAMHAL_LOGEB("Error configuring VSTAB %x", ret);
                }
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

    // if tilt angle is greater than DEGREES_TILT_IGNORE
    // we are going to ignore the orientation returned from
    // sensor. the orientation returned from sensor is not
    // reliable. Value of DEGREES_TILT_IGNORE may need adjusting
    if (tilt > DEGREES_TILT_IGNORE) {
        return;
    }

    int mountOrientation = 0;
    bool isFront = false;
    if (mCapabilities) {
        const char * const mountOrientationString =
                mCapabilities->get(CameraProperties::ORIENTATION_INDEX);
        if (mountOrientationString) {
            mountOrientation = atoi(mountOrientationString);
        }

        const char * const facingString = mCapabilities->get(CameraProperties::FACING_INDEX);
        if (facingString) {
            isFront = strcmp(facingString, TICameraParameters::FACING_FRONT) == 0;
        }
    }

    // direction is a constant sign for facing, meaning the rotation direction relative to device
    // +1 (clockwise) for back sensor and -1 (counter-clockwise) for front sensor
    const int direction = isFront ? -1 : 1;

    int rotation = mountOrientation + direction*orientation;

    // crop the calculated value to [0..360) range
    while ( rotation < 0 ) rotation += 360;
    rotation %= 360;

    if (rotation != mDeviceOrientation) {
        mDeviceOrientation = rotation;

        // restart face detection with new rotation
        setFaceDetectionOrientation(mDeviceOrientation);
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
                        Utils::Message *msg = mEventSignalQ.itemAt(i);
                        if ( NULL != msg )
                          {
                            Utils::Semaphore *sem  = (Utils::Semaphore*) msg->arg3;
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
    android::AutoMutex lock(mEventLock);
    Utils::Message *msg;
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
                    Utils::Semaphore *sem  = (Utils::Semaphore*) msg->arg3;
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
                Utils::Message msg;
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
  android::AutoMutex lock(mEventLock);
  Utils::Message *msg;
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
                  Utils::Semaphore *sem  = (Utils::Semaphore*) msg->arg3;
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
                                          OMX_IN Utils::Semaphore &semaphore)
{
    status_t ret = NO_ERROR;
    ssize_t res;
    android::AutoMutex lock(mEventLock);

    LOG_FUNCTION_NAME;
    Utils::Message * msg = ( struct Utils::Message * ) malloc(sizeof(struct Utils::Message));
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
    status_t  stat = NO_ERROR;
    status_t  res1, res2;
    OMXCameraPortParameters  *pPortParam;
    CameraFrame::FrameType typeOfFrame = CameraFrame::ALL_FRAMES;
    unsigned int refCount = 0;
    unsigned int mask = 0xFFFF;
    CameraFrame cameraFrame;
    OMX_TI_PLATFORMPRIVATE *platformPrivate;

    res1 = res2 = NO_ERROR;

    if (!pBuffHeader || !pBuffHeader->pBuffer) {
        CAMHAL_LOGE("NULL Buffer from OMX");
        return OMX_ErrorNone;
    }

    pPortParam = &(mCameraAdapterParameters.mCameraPortParams[pBuffHeader->nInputPortIndex]);
    platformPrivate = (OMX_TI_PLATFORMPRIVATE*) pBuffHeader->pPlatformPrivate;

    if (pBuffHeader->nInputPortIndex == OMX_CAMERA_PORT_VIDEO_IN_VIDEO) {
        typeOfFrame = CameraFrame::REPROCESS_INPUT_FRAME;
        mask = (unsigned int)CameraFrame::REPROCESS_INPUT_FRAME;

        stat = sendCallBacks(cameraFrame, pBuffHeader, mask, pPortParam);
   }

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
        CAMHAL_LOGI("Camera %d Frames, %f FPS", mFrameCount, mFps);
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
    Utils::Message msg;
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

#ifdef CAMERAHAL_OMX_PROFILING

status_t OMXCameraAdapter::storeProfilingData(OMX_BUFFERHEADERTYPE* pBuffHeader) {
    OMX_TI_PLATFORMPRIVATE *platformPrivate = NULL;
    OMX_OTHER_EXTRADATATYPE *extraData = NULL;
    FILE *fd = NULL;

    LOG_FUNCTION_NAME

    if ( UNLIKELY( mDebugProfile ) ) {

        platformPrivate =  static_cast<OMX_TI_PLATFORMPRIVATE *> (pBuffHeader->pPlatformPrivate);
        extraData = getExtradata(platformPrivate->pMetaDataBuffer,
                static_cast<OMX_EXTRADATATYPE> (OMX_TI_ProfilerData));

        if ( NULL != extraData ) {
            if( extraData->eType == static_cast<OMX_EXTRADATATYPE> (OMX_TI_ProfilerData) ) {

                fd = fopen(DEFAULT_PROFILE_PATH, "ab");
                if ( NULL != fd ) {
                    fwrite(extraData->data, 1, extraData->nDataSize, fd);
                    fclose(fd);
                } else {
                    return -errno;
                }

            } else {
                return NOT_ENOUGH_DATA;
            }
        } else {
            return NOT_ENOUGH_DATA;
        }
    }

    LOG_FUNCTION_NAME_EXIT

    return NO_ERROR;
}

#endif

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
    android::sp<CameraMetadataResult> metadataResult = NULL;
    unsigned int mask = 0xFFFF;
    CameraFrame cameraFrame;
    OMX_OTHER_EXTRADATATYPE *extraData;
    OMX_TI_ANCILLARYDATATYPE *ancillaryData = NULL;
    bool snapshotFrame = false;

    if ( NULL == pBuffHeader ) {
        return OMX_ErrorBadParameter;
    }

#ifdef CAMERAHAL_OMX_PROFILING

    storeProfilingData(pBuffHeader);

#endif

    res1 = res2 = NO_ERROR;

    if ( !pBuffHeader || !pBuffHeader->pBuffer ) {
        CAMHAL_LOGEA("NULL Buffer from OMX");
        return OMX_ErrorNone;
    }

    pPortParam = &(mCameraAdapterParameters.mCameraPortParams[pBuffHeader->nOutputPortIndex]);

    // Find buffer and mark it as filled
    for (int i = 0; i < pPortParam->mNumBufs; i++) {
        if (pPortParam->mBufferHeader[i] == pBuffHeader) {
            pPortParam->mStatus[i] = OMXCameraPortParameters::DONE;
        }
    }

    if (pBuffHeader->nOutputPortIndex == OMX_CAMERA_PORT_VIDEO_OUT_PREVIEW)
        {

        if ( ( PREVIEW_ACTIVE & state ) != PREVIEW_ACTIVE )
            {
            return OMX_ErrorNone;
            }

        if ( mWaitingForSnapshot ) {
            extraData = getExtradata(pBuffHeader->pPlatformPrivate,
                                     (OMX_EXTRADATATYPE) OMX_AncillaryData);

            if ( NULL != extraData ) {
                ancillaryData = (OMX_TI_ANCILLARYDATATYPE*) extraData->data;
                if ((OMX_2D_Snap == ancillaryData->eCameraView)
                    || (OMX_3D_Left_Snap == ancillaryData->eCameraView)
                    || (OMX_3D_Right_Snap == ancillaryData->eCameraView)) {
                    snapshotFrame = OMX_TRUE;
                } else {
                    snapshotFrame = OMX_FALSE;
                }
                mPending3Asettings |= SetFocus;
            }
        }

        ///Prepare the frames to be sent - initialize CameraFrame object and reference count
        // TODO(XXX): ancillary data for snapshot frame is not being sent for video snapshot
        //            if we are waiting for a snapshot and in video mode...go ahead and send
        //            this frame as a snapshot
        if( mWaitingForSnapshot &&  (mCapturedFrames > 0) &&
            (snapshotFrame || (mCapMode == VIDEO_MODE) || (mCapMode == VIDEO_MODE_HQ ) ))
            {
            typeOfFrame = CameraFrame::SNAPSHOT_FRAME;
            mask = (unsigned int)CameraFrame::SNAPSHOT_FRAME;

            // video snapshot gets ancillary data and wb info from last snapshot frame
            mCaptureAncillaryData = ancillaryData;
            mWhiteBalanceData = NULL;
            extraData = getExtradata(pBuffHeader->pPlatformPrivate,
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

        //CAMHAL_LOGV("FBD pBuffer = 0x%x", pBuffHeader->pBuffer);

        if( mWaitingForSnapshot )
            {
            if ( !mBracketingEnabled &&
                 ((HIGH_SPEED == mCapMode) ||
                  (VIDEO_MODE == mCapMode) ||
                  (VIDEO_MODE_HQ == mCapMode)) )
                {
                    notifyShutterSubscribers();
                }
            }

        stat = sendCallBacks(cameraFrame, pBuffHeader, mask, pPortParam);
        mFramesWithDisplay++;

        mFramesWithDucati--;

#ifdef CAMERAHAL_DEBUG
        if(mBuffersWithDucati.indexOfKey((uint32_t)pBuffHeader->pBuffer)<0)
            {
            CAMHAL_LOGE("Buffer was never with Ducati!! %p", pBuffHeader->pBuffer);
            for(unsigned int i=0;i<mBuffersWithDucati.size();i++) CAMHAL_LOGE("0x%x", mBuffersWithDucati.keyAt(i));
            }
        mBuffersWithDucati.removeItem((int)pBuffHeader->pBuffer);
#endif

        if(mDebugFcs)
            CAMHAL_LOGEB("C[%d] D[%d] E[%d]", mFramesWithDucati, mFramesWithDisplay, mFramesWithEncoder);

        recalculateFPS();

        createPreviewMetadata(pBuffHeader, metadataResult, pPortParam->mWidth, pPortParam->mHeight);
        if ( NULL != metadataResult.get() ) {
            notifyMetadataSubscribers(metadataResult);
            metadataResult.clear();
        }

        {
            android::AutoMutex lock(mFaceDetectionLock);
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

        sniffDccFileDataSave(pBuffHeader);

        stat |= advanceZoom();

        // On the fly update to 3A settings not working
        // Do not update 3A here if we are in the middle of a capture
        // or in the middle of transitioning to it
        if( mPending3Asettings &&
                ( (nextState & CAPTURE_ACTIVE) == 0 ) &&
                ( (state & CAPTURE_ACTIVE) == 0 ) ) {
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

        pixFormat = pPortParam->mColorFormat;

        if ( OMX_COLOR_FormatUnused == pixFormat )
            {
            typeOfFrame = CameraFrame::IMAGE_FRAME;
            mask = (unsigned int) CameraFrame::IMAGE_FRAME;
        } else if ( pixFormat == OMX_COLOR_FormatCbYCrY &&
                  ((mPictureFormatFromClient &&
                          !strcmp(mPictureFormatFromClient,
                                  android::CameraParameters::PIXEL_FORMAT_JPEG)) ||
                   !mPictureFormatFromClient) ) {
            // signals to callbacks that this needs to be coverted to jpeg
            // before returning to framework
            typeOfFrame = CameraFrame::IMAGE_FRAME;
            mask = (unsigned int) CameraFrame::IMAGE_FRAME;
            cameraFrame.mQuirks |= CameraFrame::ENCODE_RAW_YUV422I_TO_JPEG;
            cameraFrame.mQuirks |= CameraFrame::FORMAT_YUV422I_UYVY;

            // populate exif data and pass to subscribers via quirk
            // subscriber is in charge of freeing exif data
            ExifElementsTable* exif = new ExifElementsTable();
            setupEXIF_libjpeg(exif, mCaptureAncillaryData, mWhiteBalanceData);
            cameraFrame.mQuirks |= CameraFrame::HAS_EXIF_DATA;
            cameraFrame.mCookie2 = (void*) exif;
        } else {
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
            android::AutoMutex lock(mBracketingLock);
            if ( mBracketingEnabled )
                {
                doBracketing(pBuffHeader, typeOfFrame);
                return eError;
                }
            }

            if (mZoomBracketingEnabled) {
                doZoom(mZoomBracketingValues[mCurrentZoomBracketing]);
                CAMHAL_LOGDB("Current Zoom Bracketing: %d", mZoomBracketingValues[mCurrentZoomBracketing]);
                mCurrentZoomBracketing++;
                if (mCurrentZoomBracketing == ARRAY_SIZE(mZoomBracketingValues)) {
                    mZoomBracketingEnabled = false;
                }
            }

        if ( 1 > mCapturedFrames )
            {
            goto EXIT;
            }

#ifdef OMAP_ENHANCEMENT_CPCAM
        if ( NULL != mSharedAllocator ) {
            cameraFrame.mMetaData = new CameraMetadataResult(getMetaData(pBuffHeader->pPlatformPrivate, mSharedAllocator));
        }
#endif

        CAMHAL_LOGDB("Captured Frames: %d", mCapturedFrames);

        mCapturedFrames--;

#ifdef CAMERAHAL_USE_RAW_IMAGE_SAVING
        if (mYuvCapture) {
            struct timeval timeStampUsec;
            gettimeofday(&timeStampUsec, NULL);

            time_t saveTime;
            time(&saveTime);
            const struct tm * const timeStamp = gmtime(&saveTime);

            char filename[256];
            snprintf(filename,256, "%s/yuv_%d_%d_%d_%lu.yuv",
                    kYuvImagesOutputDirPath,
                    timeStamp->tm_hour,
                    timeStamp->tm_min,
                    timeStamp->tm_sec,
                    timeStampUsec.tv_usec);

            const status_t saveBufferStatus = saveBufferToFile(((CameraBuffer*)pBuffHeader->pAppPrivate)->mapped,
                                               pBuffHeader->nFilledLen, filename);

            if (saveBufferStatus != OK) {
                CAMHAL_LOGE("ERROR: %d, while saving yuv!", saveBufferStatus);
            } else {
                CAMHAL_LOGD("yuv_%d_%d_%d_%lu.yuv successfully saved in %s",
                        timeStamp->tm_hour,
                        timeStamp->tm_min,
                        timeStamp->tm_sec,
                        timeStampUsec.tv_usec,
                        kYuvImagesOutputDirPath);
            }
        }
#endif

        stat = sendCallBacks(cameraFrame, pBuffHeader, mask, pPortParam);
#ifdef OMAP_ENHANCEMENT_CPCAM
        if ( NULL != cameraFrame.mMetaData.get() ) {
            cameraFrame.mMetaData.clear();
        }
#endif

        }
        else if (pBuffHeader->nOutputPortIndex == OMX_CAMERA_PORT_VIDEO_OUT_VIDEO) {
            typeOfFrame = CameraFrame::RAW_FRAME;
            pPortParam->mImageType = typeOfFrame;
            {
                android::AutoMutex lock(mLock);
                if( ( CAPTURE_ACTIVE & state ) != CAPTURE_ACTIVE ) {
                    goto EXIT;
                }
            }

            CAMHAL_LOGD("RAW buffer done on video port, length = %d", pBuffHeader->nFilledLen);

            mask = (unsigned int) CameraFrame::RAW_FRAME;

#ifdef CAMERAHAL_USE_RAW_IMAGE_SAVING
            if ( mRawCapture ) {
                struct timeval timeStampUsec;
                gettimeofday(&timeStampUsec, NULL);

                time_t saveTime;
                time(&saveTime);
                const struct tm * const timeStamp = gmtime(&saveTime);

                char filename[256];
                snprintf(filename,256, "%s/raw_%d_%d_%d_%lu.raw",
                         kRawImagesOutputDirPath,
                         timeStamp->tm_hour,
                         timeStamp->tm_min,
                         timeStamp->tm_sec,
                         timeStampUsec.tv_usec);

                const status_t saveBufferStatus = saveBufferToFile( ((CameraBuffer*)pBuffHeader->pAppPrivate)->mapped,
                                                   pBuffHeader->nFilledLen, filename);

                if (saveBufferStatus != OK) {
                    CAMHAL_LOGE("ERROR: %d , while saving raw!", saveBufferStatus);
                } else {
                    CAMHAL_LOGD("raw_%d_%d_%d_%lu.raw successfully saved in %s",
                                timeStamp->tm_hour,
                                timeStamp->tm_min,
                                timeStamp->tm_sec,
                                timeStampUsec.tv_usec,
                                kRawImagesOutputDirPath);
                    stat = sendCallBacks(cameraFrame, pBuffHeader, mask, pPortParam);
                }
            }
#endif
        } else {
            CAMHAL_LOGEA("Frame received for non-(preview/capture/measure) port. This is yet to be supported");
            goto EXIT;
        }

    if ( NO_ERROR != stat )
        {
        CameraBuffer *camera_buffer;

        camera_buffer = (CameraBuffer *)pBuffHeader->pAppPrivate;

        CAMHAL_LOGDB("sendFrameToSubscribers error: %d", stat);
        returnFrame(camera_buffer, typeOfFrame);
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
        android::AutoMutex lock(mFrameCountMutex);
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

  android::AutoMutex lock(mSubscriberLock);

  //frame.mFrameType = typeOfFrame;
  frame.mFrameMask = mask;
  frame.mBuffer = (CameraBuffer *)pBuffHeader->pAppPrivate;
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

bool OMXCameraAdapter::CommandHandler::Handler()
{
    Utils::Message msg;
    volatile int forever = 1;
    status_t stat;
    ErrorNotifier *errorNotify = NULL;

    LOG_FUNCTION_NAME;

    while ( forever )
        {
        stat = NO_ERROR;
        CAMHAL_LOGDA("Handler: waiting for messsage...");
        Utils::MessageQueue::waitForMsg(&mCommandMsgQ, NULL, NULL, -1);
        {
        android::AutoMutex lock(mLock);
        mCommandMsgQ.get(&msg);
        }
        CAMHAL_LOGDB("msg.command = %d", msg.command);
        switch ( msg.command ) {
            case CommandHandler::CAMERA_START_IMAGE_CAPTURE:
            {
                OMXCameraAdapter::CachedCaptureParameters* cap_params =
                        static_cast<OMXCameraAdapter::CachedCaptureParameters*>(msg.arg2);
                stat = mCameraAdapter->startImageCapture(false, cap_params);
                delete cap_params;
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
            case CommandHandler::CAMERA_START_REPROCESS:
            {
                OMXCameraAdapter::CachedCaptureParameters* cap_params =
                        static_cast<OMXCameraAdapter::CachedCaptureParameters*>(msg.arg2);
                stat = mCameraAdapter->startReprocess();
                stat = mCameraAdapter->startImageCapture(false, cap_params);
                delete cap_params;
                break;
            }
        }

        }

    LOG_FUNCTION_NAME_EXIT;

    return false;
}

bool OMXCameraAdapter::OMXCallbackHandler::Handler()
{
    Utils::Message msg;
    volatile int forever = 1;
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    while(forever){
        Utils::MessageQueue::waitForMsg(&mCommandMsgQ, NULL, NULL, -1);
        {
        android::AutoMutex lock(mLock);
        mCommandMsgQ.get(&msg);
        mIsProcessed = false;
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

        {
            android::AutoMutex locker(mLock);
            CAMHAL_UNUSED(locker);

            mIsProcessed = mCommandMsgQ.isEmpty();
            if ( mIsProcessed )
                mCondition.signal();
        }
    }

    // force the condition to wake
    {
        android::AutoMutex locker(mLock);
        CAMHAL_UNUSED(locker);

        mIsProcessed = true;
        mCondition.signal();
    }

    LOG_FUNCTION_NAME_EXIT;
    return false;
}

void OMXCameraAdapter::OMXCallbackHandler::flush()
{
    LOG_FUNCTION_NAME;

    android::AutoMutex locker(mLock);
    CAMHAL_UNUSED(locker);

    if ( mIsProcessed )
        return;

    mCondition.wait(mLock);
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
#ifdef CAMERAHAL_TUNA
    extraDataControl.eCameraView = OMX_2D;
#endif

    if (enable) {
        extraDataControl.bEnable = OMX_TRUE;
    } else {
        extraDataControl.bEnable = OMX_FALSE;
    }

    eError =  OMX_SetConfig(mCameraAdapterParameters.mHandleComp,
                           (OMX_INDEXTYPE) OMX_IndexConfigOtherExtraDataControl,
                            &extraDataControl);

    LOG_FUNCTION_NAME_EXIT;

    return (ret | Utils::ErrorUtils::omxToAndroidError(eError));
}

OMX_OTHER_EXTRADATATYPE *OMXCameraAdapter::getExtradata(const OMX_PTR ptrPrivate, OMX_EXTRADATATYPE type) const
{
    if ( NULL != ptrPrivate ) {
        const OMX_TI_PLATFORMPRIVATE *platformPrivate = (const OMX_TI_PLATFORMPRIVATE *) ptrPrivate;

        CAMHAL_LOGVB("Size = %d, sizeof = %d, pAuxBuf = 0x%x, pAuxBufSize= %d, pMetaDataBufer = 0x%x, nMetaDataSize = %d",
                      platformPrivate->nSize,
                      sizeof(OMX_TI_PLATFORMPRIVATE),
                      platformPrivate->pAuxBuf1,
                      platformPrivate->pAuxBufSize1,
                      platformPrivate->pMetaDataBuffer,
                      platformPrivate->nMetaDataSize);
        if ( sizeof(OMX_TI_PLATFORMPRIVATE) == platformPrivate->nSize ) {
            if ( 0 < platformPrivate->nMetaDataSize ) {
                OMX_U32 remainingSize = platformPrivate->nMetaDataSize;
                OMX_OTHER_EXTRADATATYPE *extraData = (OMX_OTHER_EXTRADATATYPE *) platformPrivate->pMetaDataBuffer;
                if ( NULL != extraData ) {
                    while ( extraData->eType && extraData->nDataSize && extraData->data &&
                        (remainingSize >= extraData->nSize)) {
                        if ( type == extraData->eType ) {
                            return extraData;
                        }
                        remainingSize -= extraData->nSize;
                        extraData = (OMX_OTHER_EXTRADATATYPE*) ((char*)extraData + extraData->nSize);
                    }
                } else {
                    CAMHAL_LOGEB("OMX_TI_PLATFORMPRIVATE pMetaDataBuffer is NULL");
                }
            } else {
                CAMHAL_LOGEB("OMX_TI_PLATFORMPRIVATE nMetaDataSize is size is %d",
                             ( unsigned int ) platformPrivate->nMetaDataSize);
            }
        } else {
            CAMHAL_LOGEB("OMX_TI_PLATFORMPRIVATE size mismatch: expected = %d, received = %d",
                         ( unsigned int ) sizeof(OMX_TI_PLATFORMPRIVATE),
                         ( unsigned int ) platformPrivate->nSize);
        }
    }  else {
        CAMHAL_LOGEA("Invalid OMX_TI_PLATFORMPRIVATE");
    }

    // Required extradata type wasn't found
    return NULL;
}

OMXCameraAdapter::CachedCaptureParameters* OMXCameraAdapter::cacheCaptureParameters() {
    CachedCaptureParameters* params = new CachedCaptureParameters();

    params->mPendingCaptureSettings = mPendingCaptureSettings;
    params->mPictureRotation = mPictureRotation;
    memcpy(params->mExposureBracketingValues,
           mExposureBracketingValues,
           sizeof(mExposureBracketingValues));
    memcpy(params->mExposureGainBracketingValues,
           mExposureGainBracketingValues,
           sizeof(mExposureGainBracketingValues));
    memcpy(params->mExposureGainBracketingModes,
           mExposureGainBracketingModes,
           sizeof(mExposureGainBracketingModes));
    params->mExposureBracketingValidEntries = mExposureBracketingValidEntries;
    params->mExposureBracketMode = mExposureBracketMode;
    params->mBurstFrames = mBurstFrames;
    params->mFlushShotConfigQueue = mFlushShotConfigQueue;

   return params;
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
    mDccData.pData = NULL;

    mInitSem.Create(0);
    mFlushSem.Create(0);
    mUsePreviewDataSem.Create(0);
    mUsePreviewSem.Create(0);
    mUseCaptureSem.Create(0);
    mUseReprocessSem.Create(0);
    mStartPreviewSem.Create(0);
    mStopPreviewSem.Create(0);
    mStartCaptureSem.Create(0);
    mStopCaptureSem.Create(0);
    mStopReprocSem.Create(0);
    mSwitchToLoadedSem.Create(0);
    mCaptureSem.Create(0);

    mSwitchToExecSem.Create(0);

    mCameraAdapterParameters.mHandleComp = 0;

    mUserSetExpLock = OMX_FALSE;
    mUserSetWbLock = OMX_FALSE;

    mFramesWithDucati = 0;
    mFramesWithDisplay = 0;
    mFramesWithEncoder = 0;

#ifdef CAMERAHAL_OMX_PROFILING

    mDebugProfile = 0;

#endif

    mPreviewPortInitialized = false;

    LOG_FUNCTION_NAME_EXIT;
}

OMXCameraAdapter::~OMXCameraAdapter()
{
    LOG_FUNCTION_NAME;

    android::AutoMutex lock(gAdapterLock);

    if ( mOmxInitialized ) {
        // return to OMX Loaded state
        switchToLoaded();

        saveDccFileDataSave();

        closeDccFileDataSave();
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
            Utils::Message *msg = mEventSignalQ.itemAt(i);
            //remove from queue and free msg
            if ( NULL != msg )
              {
                Utils::Semaphore *sem  = (Utils::Semaphore*) msg->arg3;
                sem->Signal();
                free(msg);

              }
          }
       mEventSignalQ.clear();
      }

    //Exit and free ref to command handling thread
    if ( NULL != mCommandHandler.get() )
    {
        Utils::Message msg;
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
        Utils::Message msg;
        msg.command = OMXCallbackHandler::COMMAND_EXIT;
        //Clear all messages pending first
        mOMXCallbackHandler->clearCommandQ();
        mOMXCallbackHandler->put(&msg);
        mOMXCallbackHandler->requestExitAndWait();
        mOMXCallbackHandler.clear();
    }

    LOG_FUNCTION_NAME_EXIT;
}

extern "C" CameraAdapter* OMXCameraAdapter_Factory(size_t sensor_index)
{
    CameraAdapter *adapter = NULL;
    android::AutoMutex lock(gAdapterLock);

    LOG_FUNCTION_NAME;

    adapter = new OMXCameraAdapter(sensor_index);
    if ( adapter ) {
        CAMHAL_LOGDB("New OMX Camera adapter instance created for sensor %d",sensor_index);
    } else {
        CAMHAL_LOGEA("OMX Camera adapter create failed for sensor index = %d!",sensor_index);
    }

    LOG_FUNCTION_NAME_EXIT;

    return adapter;
}

OMX_ERRORTYPE OMXCameraAdapter::OMXCameraGetHandle(OMX_HANDLETYPE *handle, OMX_PTR pAppData,
        const OMX_CALLBACKTYPE & callbacks)
{
    OMX_ERRORTYPE eError = OMX_ErrorUndefined;

    for ( int i = 0; i < 5; ++i ) {
        if ( i > 0 ) {
            // sleep for 100 ms before next attempt
            usleep(100000);
        }

        // setup key parameters to send to Ducati during init
        OMX_CALLBACKTYPE oCallbacks = callbacks;

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


class CapabilitiesHandler
{
public:
    CapabilitiesHandler()
    {
        mComponent = 0;
    }

    const OMX_HANDLETYPE & component() const
    {
        return mComponent;
    }

    OMX_HANDLETYPE & componentRef()
    {
        return mComponent;
    }

    status_t fetchCapabiltiesForMode(OMX_CAMOPERATINGMODETYPE mode,
                                     int sensorId,
                                     CameraProperties::Properties * properties)
    {
        OMX_CONFIG_CAMOPERATINGMODETYPE camMode;

        OMX_INIT_STRUCT_PTR (&camMode, OMX_CONFIG_CAMOPERATINGMODETYPE);
        camMode.eCamOperatingMode = mode;

        OMX_ERRORTYPE eError =  OMX_SetParameter(component(),
                           ( OMX_INDEXTYPE ) OMX_IndexCameraOperatingMode,
                           &camMode);

        if ( OMX_ErrorNone != eError ) {
            CAMHAL_LOGE("Error while configuring camera mode in CameraAdapter_Capabilities 0x%x", eError);
            return BAD_VALUE;
        }

        // get and fill capabilities
        OMXCameraAdapter::getCaps(sensorId, properties, component());

        return NO_ERROR;
    }

    status_t fetchCapabilitiesForSensor(int sensorId,
                                        CameraProperties::Properties * properties)
    {
        // sensor select
        OMX_CONFIG_SENSORSELECTTYPE sensorSelect;
        OMX_INIT_STRUCT_PTR (&sensorSelect, OMX_CONFIG_SENSORSELECTTYPE);
        sensorSelect.eSensor = (OMX_SENSORSELECT)sensorId;

        CAMHAL_LOGD("Selecting sensor %d...", sensorId);
        const OMX_ERRORTYPE sensorSelectError = OMX_SetConfig(component(),
                (OMX_INDEXTYPE)OMX_TI_IndexConfigSensorSelect, &sensorSelect);
        CAMHAL_LOGD("Selecting sensor %d... DONE", sensorId);

        if ( sensorSelectError != OMX_ErrorNone ) {
            CAMHAL_LOGD("Max supported sensor number reached: %d", sensorId);
            return BAD_VALUE;
        }

        status_t err = NO_ERROR;
        if ( sensorId == 2 ) {
            CAMHAL_LOGD("Camera mode: STEREO");
            properties->setMode(MODE_STEREO);
            err = fetchCapabiltiesForMode(OMX_CaptureStereoImageCapture,
                                          sensorId,
                                          properties);
        } else {
            CAMHAL_LOGD("Camera MONO");

            CAMHAL_LOGD("Camera mode: HQ ");
            properties->setMode(MODE_HIGH_QUALITY);
            err = fetchCapabiltiesForMode(OMX_CaptureImageProfileBase,
                                          sensorId,
                                          properties);
            if ( NO_ERROR != err ) {
                return err;
            }

            CAMHAL_LOGD("Camera mode: VIDEO ");
            properties->setMode(MODE_VIDEO);
            err = fetchCapabiltiesForMode(OMX_CaptureVideo,
                                          sensorId,
                                          properties);
            if ( NO_ERROR != err ) {
                return err;
            }

            CAMHAL_LOGD("Camera mode: ZSL ");
            properties->setMode(MODE_ZEROSHUTTERLAG);
            err = fetchCapabiltiesForMode(OMX_TI_CaptureImageProfileZeroShutterLag,
                                          sensorId,
                                          properties);
            if ( NO_ERROR != err ) {
                return err;
            }

            CAMHAL_LOGD("Camera mode: HS ");
            properties->setMode(MODE_HIGH_SPEED);
            err = fetchCapabiltiesForMode(OMX_CaptureImageHighSpeedTemporalBracketing,
                                          sensorId,
                                          properties);
            if ( NO_ERROR != err ) {
                return err;
            }

            CAMHAL_LOGD("Camera mode: CPCAM ");
            properties->setMode(MODE_CPCAM);
            err = fetchCapabiltiesForMode(OMX_TI_CPCam,
                                          sensorId,
                                          properties);
            if ( NO_ERROR != err ) {
                return err;
            }

#ifdef CAMERAHAL_OMAP5_CAPTURE_MODES

            CAMHAL_LOGD("Camera mode: VIDEO HQ ");
            properties->setMode(MODE_VIDEO_HIGH_QUALITY);
            err = fetchCapabiltiesForMode(OMX_CaptureHighQualityVideo,
                                          sensorId,
                                          properties);
            if ( NO_ERROR != err ) {
                return err;
            }

#endif

        }

        return err;
    }

private:
    OMX_HANDLETYPE mComponent;
    OMX_STATETYPE mState;
};

extern "C" status_t OMXCameraAdapter_Capabilities(
        CameraProperties::Properties * const properties_array,
        const int starting_camera, const int max_camera, int & supportedCameras)
{
    LOG_FUNCTION_NAME;

    supportedCameras = 0;

    int num_cameras_supported = 0;
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    android::AutoMutex lock(gAdapterLock);

    if (!properties_array) {
        CAMHAL_LOGEB("invalid param: properties = 0x%p", properties_array);
        LOG_FUNCTION_NAME_EXIT;
        return BAD_VALUE;
    }

    eError = OMX_Init();
    if (eError != OMX_ErrorNone) {
      CAMHAL_LOGEB("Error OMX_Init -0x%x", eError);
      return Utils::ErrorUtils::omxToAndroidError(eError);
    }

    CapabilitiesHandler handler;
    OMX_CALLBACKTYPE callbacks;
    callbacks.EventHandler = 0;
    callbacks.EmptyBufferDone = 0;
    callbacks.FillBufferDone = 0;

    eError = OMXCameraAdapter::OMXCameraGetHandle(&handler.componentRef(), &handler, callbacks);
    if (eError != OMX_ErrorNone) {
        CAMHAL_LOGEB("OMX_GetHandle -0x%x", eError);
        goto EXIT;
    }

    DCCHandler dcc_handler;
    dcc_handler.loadDCC(handler.componentRef());

    // Continue selecting sensor and then querying OMX Camera for it's capabilities
    // When sensor select returns an error, we know to break and stop
    while (eError == OMX_ErrorNone &&
           (starting_camera + num_cameras_supported) < max_camera) {

        const int sensorId = num_cameras_supported;
        CameraProperties::Properties * properties = properties_array + starting_camera + sensorId;
        const status_t err = handler.fetchCapabilitiesForSensor(sensorId, properties);

        if ( err != NO_ERROR )
            break;

        num_cameras_supported++;
        CAMHAL_LOGEB("Number of OMX Cameras detected = %d \n",num_cameras_supported);
    }

     // clean up
     if(handler.component()) {
         CAMHAL_LOGD("Freeing the component...");
         OMX_FreeHandle(handler.component());
         CAMHAL_LOGD("Freeing the component... DONE");
         handler.componentRef() = NULL;
     }

 EXIT:
    CAMHAL_LOGD("Deinit...");
    OMX_Deinit();
    CAMHAL_LOGD("Deinit... DONE");

    if ( eError != OMX_ErrorNone )
    {
        CAMHAL_LOGE("Error: 0x%x", eError);
        LOG_FUNCTION_NAME_EXIT;
        return Utils::ErrorUtils::omxToAndroidError(eError);
    }

    supportedCameras = num_cameras_supported;

    LOG_FUNCTION_NAME_EXIT;

    return NO_ERROR;
}

} // namespace Camera
} // namespace Ti


/*--------------------Camera Adapter Class ENDS here-----------------------------*/

