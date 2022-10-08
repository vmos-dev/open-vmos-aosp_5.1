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
* @file CameraHal.cpp
*
* This file maps the Camera Hardware Interface to V4L2.
*
*/

#include "CameraHal.h"
#include "ANativeWindowDisplayAdapter.h"
#include "BufferSourceAdapter.h"
#include "TICameraParameters.h"
#include "CameraProperties.h"
#include <cutils/properties.h>

#include <poll.h>
#include <math.h>

namespace Ti {
namespace Camera {

extern "C" CameraAdapter* OMXCameraAdapter_Factory(size_t);
extern "C" CameraAdapter* V4LCameraAdapter_Factory(size_t);

/*****************************************************************************/

////Constant definitions and declarations
////@todo Have a CameraProperties class to store these parameters as constants for every camera
////       Currently, they are hard-coded

const int CameraHal::NO_BUFFERS_PREVIEW = MAX_CAMERA_BUFFERS;
const int CameraHal::NO_BUFFERS_IMAGE_CAPTURE = 5;
const int CameraHal::SW_SCALING_FPS_LIMIT = 15;

const uint32_t MessageNotifier::EVENT_BIT_FIELD_POSITION = 16;

const uint32_t MessageNotifier::FRAME_BIT_FIELD_POSITION = 0;

// TODO(XXX): Temporarily increase number of buffers we can allocate from ANW
// until faux-NPA mode is implemented
const int CameraHal::NO_BUFFERS_IMAGE_CAPTURE_SYSTEM_HEAP = 15;

#ifdef CAMERAHAL_USE_RAW_IMAGE_SAVING
// HACK: Default path to directory where RAW images coming from video port will be saved to.
//       If directory not exists the saving is skipped and video port frame is ignored.
//       The directory name is choosed in so weird way to enable RAW images saving only when
//       directory has been created explicitly by user.
extern const char * const kRawImagesOutputDirPath = "/data/misc/camera/RaW_PiCtUrEs";
extern const char * const kYuvImagesOutputDirPath = "/data/misc/camera/YuV_PiCtUrEs";
#endif

/******************************************************************************/


#ifdef OMAP_ENHANCEMENT_CPCAM
static int dummy_update_and_get_buffer(preview_stream_ops_t*, buffer_handle_t**, int*) {
    return INVALID_OPERATION;
}

static int dummy_get_buffer_dimension(preview_stream_ops_t*, int*, int*) {
    return INVALID_OPERATION;
}

static int dummy_get_buffer_format(preview_stream_ops_t*, int*) {
    return INVALID_OPERATION;
}

static int dummy_set_metadata(preview_stream_ops_t*, const camera_memory_t*) {
    return INVALID_OPERATION;
}

static int dummy_get_id(preview_stream_ops_t*, char *data, unsigned int dataSize) {
    return INVALID_OPERATION;
}

static int dummy_get_buffer_count(preview_stream_ops_t*, int *count) {
    return INVALID_OPERATION;
}

static int dummy_get_crop(preview_stream_ops_t*,
                          int *, int *, int *, int *) {
    return INVALID_OPERATION;
}

static int dummy_get_current_size(preview_stream_ops_t*,
                                  int *, int *) {
    return INVALID_OPERATION;
}
#endif

#ifdef OMAP_ENHANCEMENT
static preview_stream_extended_ops_t dummyPreviewStreamExtendedOps = {
#ifdef OMAP_ENHANCEMENT_CPCAM
    dummy_update_and_get_buffer,
    dummy_get_buffer_dimension,
    dummy_get_buffer_format,
    dummy_set_metadata,
    dummy_get_id,
    dummy_get_buffer_count,
    dummy_get_crop,
    dummy_get_current_size,
#endif
};
#endif


DisplayAdapter::DisplayAdapter()
{
#ifdef OMAP_ENHANCEMENT
    mExtendedOps = &dummyPreviewStreamExtendedOps;
#endif
}

#ifdef OMAP_ENHANCEMENT
void DisplayAdapter::setExtendedOps(preview_stream_extended_ops_t * extendedOps) {
    mExtendedOps = extendedOps ? extendedOps : &dummyPreviewStreamExtendedOps;
}
#endif



#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS

struct timeval CameraHal::mStartPreview;
struct timeval CameraHal::mStartFocus;
struct timeval CameraHal::mStartCapture;

#endif

static void orientation_cb(uint32_t orientation, uint32_t tilt, void* cookie) {
    CameraHal *camera = NULL;

    if (cookie) {
        camera = (CameraHal*) cookie;
        camera->onOrientationEvent(orientation, tilt);
    }

}

/*-------------Camera Hal Interface Method definitions STARTS here--------------------*/

/**
  Callback function to receive orientation events from SensorListener
 */
void CameraHal::onOrientationEvent(uint32_t orientation, uint32_t tilt) {
    LOG_FUNCTION_NAME;

    if ( NULL != mCameraAdapter ) {
        mCameraAdapter->onOrientationEvent(orientation, tilt);
    }

    LOG_FUNCTION_NAME_EXIT;
}

/**
   @brief Set the notification and data callbacks

   @param[in] notify_cb Notify callback for notifying the app about events and errors
   @param[in] data_cb   Buffer callback for sending the preview/raw frames to the app
   @param[in] data_cb_timestamp Buffer callback for sending the video frames w/ timestamp
   @param[in] user  Callback cookie
   @return none

 */
void CameraHal::setCallbacks(camera_notify_callback notify_cb,
                            camera_data_callback data_cb,
                            camera_data_timestamp_callback data_cb_timestamp,
                            camera_request_memory get_memory,
                            void *user)
{
    LOG_FUNCTION_NAME;

    if ( NULL != mAppCallbackNotifier.get() )
    {
            mAppCallbackNotifier->setCallbacks(this,
                                                notify_cb,
                                                data_cb,
                                                data_cb_timestamp,
                                                get_memory,
                                                user);
    }

    if ( NULL != mCameraAdapter ) {
        mCameraAdapter->setSharedAllocator(get_memory);
    }

    LOG_FUNCTION_NAME_EXIT;
}

/**
   @brief Enable a message, or set of messages.

   @param[in] msgtype Bitmask of the messages to enable (defined in include/ui/Camera.h)
   @return none

 */
void CameraHal::enableMsgType(int32_t msgType)
{
    LOG_FUNCTION_NAME;

    if ( ( msgType & CAMERA_MSG_SHUTTER ) && ( !mShutterEnabled ) )
        {
        msgType &= ~CAMERA_MSG_SHUTTER;
        }

    // ignoring enable focus message from camera service
    // we will enable internally in autoFocus call
    msgType &= ~CAMERA_MSG_FOCUS;
#ifdef ANDROID_API_JB_OR_LATER
    msgType &= ~CAMERA_MSG_FOCUS_MOVE;
#endif

    {
    android::AutoMutex lock(mLock);
    mMsgEnabled |= msgType;
    }

    if(mMsgEnabled & CAMERA_MSG_PREVIEW_FRAME)
    {
        if(mDisplayPaused)
        {
            CAMHAL_LOGDA("Preview currently paused...will enable preview callback when restarted");
            msgType &= ~CAMERA_MSG_PREVIEW_FRAME;
        }else
        {
            CAMHAL_LOGDA("Enabling Preview Callback");
        }
    }
    else
    {
        CAMHAL_LOGDB("Preview callback not enabled %x", msgType);
    }


    ///Configure app callback notifier with the message callback required
    mAppCallbackNotifier->enableMsgType (msgType);

    LOG_FUNCTION_NAME_EXIT;
}

/**
   @brief Disable a message, or set of messages.

   @param[in] msgtype Bitmask of the messages to disable (defined in include/ui/Camera.h)
   @return none

 */
void CameraHal::disableMsgType(int32_t msgType)
{
    LOG_FUNCTION_NAME;

        {
        android::AutoMutex lock(mLock);
        mMsgEnabled &= ~msgType;
        }

    if( msgType & CAMERA_MSG_PREVIEW_FRAME)
        {
        CAMHAL_LOGDA("Disabling Preview Callback");
        }

    ///Configure app callback notifier
    mAppCallbackNotifier->disableMsgType (msgType);

    LOG_FUNCTION_NAME_EXIT;
}

/**
   @brief Query whether a message, or a set of messages, is enabled.

   Note that this is operates as an AND, if any of the messages queried are off, this will
   return false.

   @param[in] msgtype Bitmask of the messages to query (defined in include/ui/Camera.h)
   @return true If all message types are enabled
          false If any message type

 */
int CameraHal::msgTypeEnabled(int32_t msgType)
{
    int32_t msgEnabled = 0;

    LOG_FUNCTION_NAME;
    android::AutoMutex lock(mLock);

    msgEnabled = mMsgEnabled;
    if (!previewEnabled() && !mPreviewInitializationDone) {
        msgEnabled &= ~(CAMERA_MSG_PREVIEW_FRAME | CAMERA_MSG_PREVIEW_METADATA);
    }

    LOG_FUNCTION_NAME_EXIT;
    return (msgEnabled & msgType);
}

/**
   @brief Set the camera parameters.

   @param[in] params Camera parameters to configure the camera
   @return NO_ERROR
   @todo Define error codes

 */
int CameraHal::setParameters(const char* parameters)
{

    LOG_FUNCTION_NAME;

    android::CameraParameters params;

    android::String8 str_params(parameters);
    params.unflatten(str_params);

    LOG_FUNCTION_NAME_EXIT;

    return setParameters(params);
}

/**
   @brief Set the camera parameters.

   @param[in] params Camera parameters to configure the camera
   @return NO_ERROR
   @todo Define error codes

 */
int CameraHal::setParameters(const android::CameraParameters& params)
{

    LOG_FUNCTION_NAME;

    int w, h;
    int framerate;
    int maxFPS, minFPS;
    const char *valstr = NULL;
    int varint = 0;
    status_t ret = NO_ERROR;
    // Needed for KEY_RECORDING_HINT
    bool restartPreviewRequired = false;
    bool updateRequired = false;
    android::CameraParameters oldParams = mParameters;

#ifdef V4L_CAMERA_ADAPTER
    if (strcmp (V4L_CAMERA_NAME_USB, mCameraProperties->get(CameraProperties::CAMERA_NAME)) == 0 ) {
        updateRequired = true;
    }
#endif

    {
        android::AutoMutex lock(mLock);

        ///Ensure that preview is not enabled when the below parameters are changed.
        if(!previewEnabled())
            {
            if ((valstr = params.getPreviewFormat()) != NULL) {
                if ( isParameterValid(valstr, mCameraProperties->get(CameraProperties::SUPPORTED_PREVIEW_FORMATS))) {
                    mParameters.setPreviewFormat(valstr);
                    CAMHAL_LOGDB("PreviewFormat set %s", valstr);
                } else {
                    CAMHAL_LOGEB("Invalid preview format: %s. Supported: %s", valstr,
                        mCameraProperties->get(CameraProperties::SUPPORTED_PREVIEW_FORMATS));
                    return BAD_VALUE;
                }
            }

            if ((valstr = params.get(TICameraParameters::KEY_VNF)) != NULL) {
                if (strcmp(mCameraProperties->get(CameraProperties::VNF_SUPPORTED),
                           android::CameraParameters::TRUE) == 0) {
                    CAMHAL_LOGDB("VNF %s", valstr);
                    mParameters.set(TICameraParameters::KEY_VNF, valstr);
                } else if (strcmp(valstr, android::CameraParameters::TRUE) == 0) {
                    CAMHAL_LOGEB("ERROR: Invalid VNF: %s", valstr);
                    return BAD_VALUE;
                } else {
                    mParameters.set(TICameraParameters::KEY_VNF,
                                    android::CameraParameters::FALSE);
                }
            }

            if ((valstr = params.get(android::CameraParameters::KEY_VIDEO_STABILIZATION)) != NULL) {
                // make sure we support vstab...if we don't and application is trying to set
                // vstab then return an error
                if (strcmp(mCameraProperties->get(CameraProperties::VSTAB_SUPPORTED),
                           android::CameraParameters::TRUE) == 0) {
                    CAMHAL_LOGDB("VSTAB %s", valstr);
                    mParameters.set(android::CameraParameters::KEY_VIDEO_STABILIZATION, valstr);
                } else if (strcmp(valstr, android::CameraParameters::TRUE) == 0) {
                    CAMHAL_LOGEB("ERROR: Invalid VSTAB: %s", valstr);
                    return BAD_VALUE;
                } else {
                    mParameters.set(android::CameraParameters::KEY_VIDEO_STABILIZATION,
                                    android::CameraParameters::FALSE);
                }
            }

            if( (valstr = params.get(TICameraParameters::KEY_CAP_MODE)) != NULL) {

                    if (strcmp(TICameraParameters::VIDEO_MODE, valstr)) {
                        mCapModeBackup = valstr;
                    }

                    CAMHAL_LOGDB("Capture mode set %s", valstr);

                    const char *currentMode = mParameters.get(TICameraParameters::KEY_CAP_MODE);
                    if ( NULL != currentMode ) {
                        if ( strcmp(currentMode, valstr) != 0 ) {
                            updateRequired = true;
                        }
                    } else {
                        updateRequired = true;
                    }

                    mParameters.set(TICameraParameters::KEY_CAP_MODE, valstr);
            } else if (!mCapModeBackup.isEmpty()) {
                // Restore previous capture mode after stopPreview()
                mParameters.set(TICameraParameters::KEY_CAP_MODE,
                                mCapModeBackup.string());
                updateRequired = true;
            }

#ifdef OMAP_ENHANCEMENT_VTC
            if ((valstr = params.get(TICameraParameters::KEY_VTC_HINT)) != NULL ) {
                mParameters.set(TICameraParameters::KEY_VTC_HINT, valstr);
                if (strcmp(valstr, android::CameraParameters::TRUE) == 0) {
                    mVTCUseCase = true;
                } else {
                    mVTCUseCase = false;
                }
                CAMHAL_LOGDB("VTC Hint = %d", mVTCUseCase);
            }

            if (mVTCUseCase) {
                if ((valstr = params.get(TICameraParameters::KEY_VIDEO_ENCODER_HANDLE)) != NULL ) {
                    mParameters.set(TICameraParameters::KEY_VIDEO_ENCODER_HANDLE, valstr);
                }

                if ((valstr = params.get(TICameraParameters::KEY_VIDEO_ENCODER_SLICE_HEIGHT)) != NULL ) {
                    mParameters.set(TICameraParameters::KEY_VIDEO_ENCODER_SLICE_HEIGHT, valstr);
                }
            }
#endif
        }

        if ((valstr = params.get(TICameraParameters::KEY_IPP)) != NULL) {
            if (isParameterValid(valstr,mCameraProperties->get(CameraProperties::SUPPORTED_IPP_MODES))) {
                if ((mParameters.get(TICameraParameters::KEY_IPP) == NULL) ||
                        (strcmp(valstr, mParameters.get(TICameraParameters::KEY_IPP)))) {
                    CAMHAL_LOGDB("IPP mode set %s", params.get(TICameraParameters::KEY_IPP));
                    mParameters.set(TICameraParameters::KEY_IPP, valstr);
                    restartPreviewRequired = true;
                }
            } else {
                CAMHAL_LOGEB("ERROR: Invalid IPP mode: %s", valstr);
                return BAD_VALUE;
            }
        }

        if ( (valstr = params.get(TICameraParameters::KEY_S3D_PRV_FRAME_LAYOUT)) != NULL )
            {
            if (strcmp(valstr, mParameters.get(TICameraParameters::KEY_S3D_PRV_FRAME_LAYOUT)))
                {
                CAMHAL_LOGDB("Stereo 3D preview image layout is %s", valstr);
                mParameters.set(TICameraParameters::KEY_S3D_PRV_FRAME_LAYOUT, valstr);
                restartPreviewRequired = true;
                }
            }

#ifdef OMAP_ENHANCEMENT
        int orientation =0;
        if((valstr = params.get(TICameraParameters::KEY_SENSOR_ORIENTATION)) != NULL)
            {
            doesSetParameterNeedUpdate(valstr,
                                       mParameters.get(TICameraParameters::KEY_SENSOR_ORIENTATION),
                                       updateRequired);

            orientation = params.getInt(TICameraParameters::KEY_SENSOR_ORIENTATION);
            if ( orientation < 0 || orientation >= 360 || (orientation%90) != 0 ) {
                CAMHAL_LOGE("Invalid sensor orientation: %s. Value must be one of: [0, 90, 180, 270]", valstr);
                return BAD_VALUE;
            }

            CAMHAL_LOGD("Sensor Orientation is set to %d", orientation);
            mParameters.set(TICameraParameters::KEY_SENSOR_ORIENTATION, valstr);
            }
#endif

        params.getPreviewSize(&w, &h);
        if (w == -1 && h == -1) {
            CAMHAL_LOGEA("Unable to get preview size");
            return BAD_VALUE;
        }

        mVideoWidth = w;
        mVideoHeight = h;

        // Handle RECORDING_HINT to Set/Reset Video Mode Parameters
        valstr = params.get(android::CameraParameters::KEY_RECORDING_HINT);
        if(valstr != NULL)
            {
            CAMHAL_LOGDB("Recording Hint is set to %s", valstr);
            if(strcmp(valstr, android::CameraParameters::TRUE) == 0)
                {
                CAMHAL_LOGVB("Video Resolution: %d x %d", mVideoWidth, mVideoHeight);
#ifdef OMAP_ENHANCEMENT_VTC
                if (!mVTCUseCase)
#endif
                {
                    int maxFPS, minFPS;

                    params.getPreviewFpsRange(&minFPS, &maxFPS);
                    maxFPS /= CameraHal::VFR_SCALE;
                    if ( ( maxFPS <= SW_SCALING_FPS_LIMIT ) ) {
                        getPreferredPreviewRes(&w, &h);
                    }
                }
                mParameters.set(android::CameraParameters::KEY_RECORDING_HINT, valstr);
                restartPreviewRequired |= setVideoModeParameters(params);
                }
            else if(strcmp(valstr, android::CameraParameters::FALSE) == 0)
                {
                mParameters.set(android::CameraParameters::KEY_RECORDING_HINT, valstr);
                restartPreviewRequired |= resetVideoModeParameters();
                }
            else
                {
                CAMHAL_LOGEA("Invalid RECORDING_HINT");
                return BAD_VALUE;
                }
            }
        else
            {
            // This check is required in following case.
            // If VideoRecording activity sets KEY_RECORDING_HINT to TRUE and
            // ImageCapture activity doesnot set KEY_RECORDING_HINT to FALSE (i.e. simply NULL),
            // then Video Mode parameters may remain present in ImageCapture activity as well.
            CAMHAL_LOGDA("Recording Hint is set to NULL");
            mParameters.set(android::CameraParameters::KEY_RECORDING_HINT, "");
            restartPreviewRequired |= resetVideoModeParameters();
            }

        if ( (!isResolutionValid(w, h, mCameraProperties->get(CameraProperties::SUPPORTED_PREVIEW_SIZES)))
                && (!isResolutionValid(w, h, mCameraProperties->get(CameraProperties::SUPPORTED_PREVIEW_SUBSAMPLED_SIZES)))
                && (!isResolutionValid(w, h, mCameraProperties->get(CameraProperties::SUPPORTED_PREVIEW_SIDEBYSIDE_SIZES)))
                && (!isResolutionValid(w, h, mCameraProperties->get(CameraProperties::SUPPORTED_PREVIEW_TOPBOTTOM_SIZES))) ) {
            CAMHAL_LOGEB("Invalid preview resolution %d x %d", w, h);
            return BAD_VALUE;
        }

        int oldWidth, oldHeight;
        mParameters.getPreviewSize(&oldWidth, &oldHeight);
        if ( ( oldWidth != w ) || ( oldHeight != h ) )
            {
            mParameters.setPreviewSize(w, h);
            restartPreviewRequired = true;
            }

        CAMHAL_LOGDB("Preview Resolution: %d x %d", w, h);

        if ((valstr = params.get(android::CameraParameters::KEY_FOCUS_MODE)) != NULL) {
            if (isParameterValid(valstr, mCameraProperties->get(CameraProperties::SUPPORTED_FOCUS_MODES))) {
                CAMHAL_LOGDB("Focus mode set %s", valstr);

                // we need to take a decision on the capture mode based on whether CAF picture or
                // video is chosen so the behavior of each is consistent to the application
                if(strcmp(valstr, android::CameraParameters::FOCUS_MODE_CONTINUOUS_PICTURE) == 0){
                    restartPreviewRequired |= resetVideoModeParameters();
                } else if (strcmp(valstr, android::CameraParameters::FOCUS_MODE_CONTINUOUS_VIDEO) == 0){
                    restartPreviewRequired |= setVideoModeParameters(params);
                }

                mParameters.set(android::CameraParameters::KEY_FOCUS_MODE, valstr);
             } else {
                CAMHAL_LOGEB("ERROR: Invalid FOCUS mode = %s", valstr);
                return BAD_VALUE;
             }
        }

        mRawCapture = false;

#ifdef CAMERAHAL_USE_RAW_IMAGE_SAVING
        valstr = params.get(TICameraParameters::KEY_CAP_MODE);
        if ( (!valstr || strcmp(valstr, TICameraParameters::HIGH_QUALITY_MODE) == 0) &&
                access(kRawImagesOutputDirPath, F_OK) != -1 ) {
            mRawCapture = true;
        }
#endif

        if ( (valstr = params.get(TICameraParameters::KEY_S3D_CAP_FRAME_LAYOUT)) != NULL )
            {
            CAMHAL_LOGDB("Stereo 3D capture image layout is %s", valstr);
            mParameters.set(TICameraParameters::KEY_S3D_CAP_FRAME_LAYOUT, valstr);
            }

        params.getPictureSize(&w, &h);
        if ( (isResolutionValid(w, h, mCameraProperties->get(CameraProperties::SUPPORTED_PICTURE_SIZES)))
                || (isResolutionValid(w, h, mCameraProperties->get(CameraProperties::SUPPORTED_PICTURE_SUBSAMPLED_SIZES)))
                || (isResolutionValid(w, h, mCameraProperties->get(CameraProperties::SUPPORTED_PICTURE_TOPBOTTOM_SIZES)))
                || (isResolutionValid(w, h, mCameraProperties->get(CameraProperties::SUPPORTED_PICTURE_SIDEBYSIDE_SIZES))) ) {
            mParameters.setPictureSize(w, h);
        } else {
            CAMHAL_LOGEB("ERROR: Invalid picture resolution %d x %d", w, h);
            return BAD_VALUE;
        }

        CAMHAL_LOGDB("Picture Size by App %d x %d", w, h);

        if ( (valstr = params.getPictureFormat()) != NULL ) {
            if (isParameterValid(valstr,mCameraProperties->get(CameraProperties::SUPPORTED_PICTURE_FORMATS))) {
                if ((strcmp(valstr, android::CameraParameters::PIXEL_FORMAT_BAYER_RGGB) == 0) &&
                    mCameraProperties->get(CameraProperties::MAX_PICTURE_WIDTH) &&
                    mCameraProperties->get(CameraProperties::MAX_PICTURE_HEIGHT)) {
                    unsigned int width = 0, height = 0;
                    // Set picture size to full frame for raw bayer capture
                    width = atoi(mCameraProperties->get(CameraProperties::MAX_PICTURE_WIDTH));
                    height = atoi(mCameraProperties->get(CameraProperties::MAX_PICTURE_HEIGHT));
                    mParameters.setPictureSize(width,height);
                }
                mParameters.setPictureFormat(valstr);
            } else {
                CAMHAL_LOGEB("ERROR: Invalid picture format: %s",valstr);
                ret = BAD_VALUE;
            }
        }

#ifdef OMAP_ENHANCEMENT_BURST_CAPTURE
        if ((valstr = params.get(TICameraParameters::KEY_BURST)) != NULL) {
            if (params.getInt(TICameraParameters::KEY_BURST) >=0) {
                CAMHAL_LOGDB("Burst set %s", valstr);
                mParameters.set(TICameraParameters::KEY_BURST, valstr);
            } else {
                CAMHAL_LOGEB("ERROR: Invalid Burst value: %s",valstr);
                return BAD_VALUE;
            }
        }
#endif

        // Variable framerate ranges have higher priority over
        // deprecated constant FPS. "KEY_PREVIEW_FPS_RANGE" should
        // be cleared by the client in order for constant FPS to get
        // applied.
        // If Port FPS needs to be used for configuring, then FPS RANGE should not be set by the APP.
        valstr = params.get(android::CameraParameters::KEY_PREVIEW_FPS_RANGE);
        if (valstr != NULL && strlen(valstr)) {
            int curMaxFPS = 0;
            int curMinFPS = 0;

            // APP wants to set FPS range
            // Set framerate = MAXFPS
            CAMHAL_LOGDA("APP IS CHANGING FRAME RATE RANGE");

            mParameters.getPreviewFpsRange(&curMinFPS, &curMaxFPS);
            CAMHAL_LOGDB("## current minFPS = %d; maxFPS=%d",curMinFPS, curMaxFPS);

            params.getPreviewFpsRange(&minFPS, &maxFPS);
            CAMHAL_LOGDB("## requested minFPS = %d; maxFPS=%d",minFPS, maxFPS);
            // Validate VFR
            if (!isFpsRangeValid(minFPS, maxFPS, params.get(android::CameraParameters::KEY_SUPPORTED_PREVIEW_FPS_RANGE)) &&
                !isFpsRangeValid(minFPS, maxFPS, params.get(TICameraParameters::KEY_FRAMERATE_RANGES_EXT_SUPPORTED))) {
                CAMHAL_LOGEA("Invalid FPS Range");
                return BAD_VALUE;
            } else {
                framerate = maxFPS / CameraHal::VFR_SCALE;
                mParameters.setPreviewFrameRate(framerate);
                CAMHAL_LOGDB("SET FRAMERATE %d", framerate);
                mParameters.set(android::CameraParameters::KEY_PREVIEW_FPS_RANGE, valstr);
                CAMHAL_LOGDB("FPS Range = %s", valstr);
                if ( curMaxFPS == (FRAME_RATE_HIGH_HD * CameraHal::VFR_SCALE) &&
                     maxFPS < (FRAME_RATE_HIGH_HD * CameraHal::VFR_SCALE) ) {
                    restartPreviewRequired = true;
                }
            }
        } else {
            framerate = params.getPreviewFrameRate();
            if (!isParameterValid(framerate, params.get(android::CameraParameters::KEY_SUPPORTED_PREVIEW_FRAME_RATES)) &&
                !isParameterValid(framerate, params.get(TICameraParameters::KEY_FRAMERATES_EXT_SUPPORTED))) {
                CAMHAL_LOGEA("Invalid frame rate");
                return BAD_VALUE;
            }
            char tmpBuffer[MAX_PROP_VALUE_LENGTH];

            sprintf(tmpBuffer, "%d,%d", framerate * CameraHal::VFR_SCALE, framerate * CameraHal::VFR_SCALE);
            mParameters.setPreviewFrameRate(framerate);
            CAMHAL_LOGDB("SET FRAMERATE %d", framerate);
            mParameters.set(android::CameraParameters::KEY_PREVIEW_FPS_RANGE, tmpBuffer);
            CAMHAL_LOGDB("FPS Range = %s", tmpBuffer);
        }

        if ((valstr = params.get(TICameraParameters::KEY_GBCE)) != NULL) {
            if (strcmp(mCameraProperties->get(CameraProperties::SUPPORTED_GBCE),
                    android::CameraParameters::TRUE) == 0) {
                CAMHAL_LOGDB("GBCE %s", valstr);
                mParameters.set(TICameraParameters::KEY_GBCE, valstr);
            } else if (strcmp(valstr, android::CameraParameters::TRUE) == 0) {
                CAMHAL_LOGEB("ERROR: Invalid GBCE: %s", valstr);
                return BAD_VALUE;
            } else {
                mParameters.set(TICameraParameters::KEY_GBCE, android::CameraParameters::FALSE);
            }
        } else {
            mParameters.set(TICameraParameters::KEY_GBCE, android::CameraParameters::FALSE);
        }

        if ((valstr = params.get(TICameraParameters::KEY_GLBCE)) != NULL) {
            if (strcmp(mCameraProperties->get(CameraProperties::SUPPORTED_GLBCE),
                    android::CameraParameters::TRUE) == 0) {
                CAMHAL_LOGDB("GLBCE %s", valstr);
                mParameters.set(TICameraParameters::KEY_GLBCE, valstr);
            } else if (strcmp(valstr, android::CameraParameters::TRUE) == 0) {
                CAMHAL_LOGEB("ERROR: Invalid GLBCE: %s", valstr);
                return BAD_VALUE;
            } else {
                mParameters.set(TICameraParameters::KEY_GLBCE, android::CameraParameters::FALSE);
            }
        } else {
            mParameters.set(TICameraParameters::KEY_GLBCE, android::CameraParameters::FALSE);
        }

#ifdef OMAP_ENHANCEMENT_S3D
        ///Update the current parameter set
        if ( (valstr = params.get(TICameraParameters::KEY_AUTOCONVERGENCE_MODE)) != NULL ) {
            CAMHAL_LOGDB("AutoConvergence mode set = %s", valstr);
            mParameters.set(TICameraParameters::KEY_AUTOCONVERGENCE_MODE, valstr);
        }

        if ( (valstr = params.get(TICameraParameters::KEY_MANUAL_CONVERGENCE)) != NULL ) {
            int manualConvergence = (int)strtol(valstr, 0, 0);

            if ( ( manualConvergence < strtol(mCameraProperties->get(CameraProperties::SUPPORTED_MANUAL_CONVERGENCE_MIN), 0, 0) ) ||
                    ( manualConvergence > strtol(mCameraProperties->get(CameraProperties::SUPPORTED_MANUAL_CONVERGENCE_MAX), 0, 0) ) ) {
                CAMHAL_LOGEB("ERROR: Invalid Manual Convergence = %d", manualConvergence);
                return BAD_VALUE;
            } else {
                CAMHAL_LOGDB("ManualConvergence Value = %d", manualConvergence);
                mParameters.set(TICameraParameters::KEY_MANUAL_CONVERGENCE, valstr);
            }
        }

        if((valstr = params.get(TICameraParameters::KEY_MECHANICAL_MISALIGNMENT_CORRECTION)) != NULL) {
            if ( strcmp(mCameraProperties->get(CameraProperties::MECHANICAL_MISALIGNMENT_CORRECTION_SUPPORTED),
                    android::CameraParameters::TRUE) == 0 ) {
                CAMHAL_LOGDB("Mechanical Mialignment Correction is %s", valstr);
                mParameters.set(TICameraParameters::KEY_MECHANICAL_MISALIGNMENT_CORRECTION, valstr);
            } else {
                mParameters.remove(TICameraParameters::KEY_MECHANICAL_MISALIGNMENT_CORRECTION);
            }
        }

        if ((valstr = params.get(TICameraParameters::KEY_EXPOSURE_MODE)) != NULL) {
            if (isParameterValid(valstr, mCameraProperties->get(CameraProperties::SUPPORTED_EXPOSURE_MODES))) {
                CAMHAL_LOGDB("Exposure mode set = %s", valstr);
                mParameters.set(TICameraParameters::KEY_EXPOSURE_MODE, valstr);
                if (!strcmp(valstr, TICameraParameters::EXPOSURE_MODE_MANUAL)) {
                    int manualVal;
                    if ((valstr = params.get(TICameraParameters::KEY_MANUAL_EXPOSURE)) != NULL) {
                        manualVal = params.getInt(TICameraParameters::KEY_MANUAL_EXPOSURE);
                        if (manualVal < mParameters.getInt(TICameraParameters::KEY_SUPPORTED_MANUAL_EXPOSURE_MIN) ||
                                manualVal > mParameters.getInt(TICameraParameters::KEY_SUPPORTED_MANUAL_EXPOSURE_MAX)) {
                            CAMHAL_LOGEB("ERROR: Manual Exposure = %s is out of range - "
                                            "setting minimum supported value", valstr);
                            valstr = mParameters.get(TICameraParameters::KEY_SUPPORTED_MANUAL_EXPOSURE_MIN);
                        }
                        CAMHAL_LOGDB("Manual Exposure = %s", valstr);
                        mParameters.set(TICameraParameters::KEY_MANUAL_EXPOSURE, valstr);
                    }
                    if ((valstr = params.get(TICameraParameters::KEY_MANUAL_EXPOSURE_RIGHT)) != NULL) {
                        manualVal = params.getInt(TICameraParameters::KEY_MANUAL_EXPOSURE_RIGHT);
                        if (manualVal < mParameters.getInt(TICameraParameters::KEY_SUPPORTED_MANUAL_EXPOSURE_MIN) ||
                                manualVal > mParameters.getInt(TICameraParameters::KEY_SUPPORTED_MANUAL_EXPOSURE_MAX)) {
                            CAMHAL_LOGEB("ERROR: Manual Exposure right = %s is out of range - "
                                            "setting minimum supported value", valstr);
                            valstr = mParameters.get(TICameraParameters::KEY_SUPPORTED_MANUAL_EXPOSURE_MIN);
                        }
                        CAMHAL_LOGDB("Manual Exposure right = %s", valstr);
                        mParameters.set(TICameraParameters::KEY_MANUAL_EXPOSURE_RIGHT, valstr);
                    }
                    if ((valstr = params.get(TICameraParameters::KEY_MANUAL_GAIN_ISO)) != NULL) {
                        manualVal = params.getInt(TICameraParameters::KEY_MANUAL_GAIN_ISO);
                        if (manualVal < mParameters.getInt(TICameraParameters::KEY_SUPPORTED_MANUAL_GAIN_ISO_MIN) ||
                                manualVal > mParameters.getInt(TICameraParameters::KEY_SUPPORTED_MANUAL_GAIN_ISO_MAX)) {
                            CAMHAL_LOGEB("ERROR: Manual Gain = %s is out of range - "
                                            "setting minimum supported value", valstr);
                            valstr = mParameters.get(TICameraParameters::KEY_SUPPORTED_MANUAL_GAIN_ISO_MIN);
                        }
                        CAMHAL_LOGDB("Manual Gain = %s", valstr);
                        mParameters.set(TICameraParameters::KEY_MANUAL_GAIN_ISO, valstr);
                    }
                    if ((valstr = params.get(TICameraParameters::KEY_MANUAL_GAIN_ISO_RIGHT)) != NULL) {
                        manualVal = params.getInt(TICameraParameters::KEY_MANUAL_GAIN_ISO_RIGHT);
                        if (manualVal < mParameters.getInt(TICameraParameters::KEY_SUPPORTED_MANUAL_GAIN_ISO_MIN) ||
                                manualVal > mParameters.getInt(TICameraParameters::KEY_SUPPORTED_MANUAL_GAIN_ISO_MAX)) {
                            CAMHAL_LOGEB("ERROR: Manual Gain right = %s is out of range - "
                                            "setting minimum supported value", valstr);
                            valstr = mParameters.get(TICameraParameters::KEY_SUPPORTED_MANUAL_GAIN_ISO_MIN);
                        }
                        CAMHAL_LOGDB("Manual Gain right = %s", valstr);
                        mParameters.set(TICameraParameters::KEY_MANUAL_GAIN_ISO_RIGHT, valstr);
                    }
                }
            } else {
                CAMHAL_LOGEB("ERROR: Invalid Exposure mode = %s", valstr);
                return BAD_VALUE;
            }
        }
#endif

        if ((valstr = params.get(android::CameraParameters::KEY_WHITE_BALANCE)) != NULL) {
           if ( isParameterValid(valstr, mCameraProperties->get(CameraProperties::SUPPORTED_WHITE_BALANCE))) {
               CAMHAL_LOGDB("White balance set %s", valstr);
               mParameters.set(android::CameraParameters::KEY_WHITE_BALANCE, valstr);
            } else {
               CAMHAL_LOGEB("ERROR: Invalid white balance  = %s", valstr);
               return BAD_VALUE;
            }
        }

#ifdef OMAP_ENHANCEMENT
        if ((valstr = params.get(TICameraParameters::KEY_CONTRAST)) != NULL) {
            if (params.getInt(TICameraParameters::KEY_CONTRAST) >= 0 ) {
                CAMHAL_LOGDB("Contrast set %s", valstr);
                mParameters.set(TICameraParameters::KEY_CONTRAST, valstr);
            } else {
                CAMHAL_LOGEB("ERROR: Invalid Contrast  = %s", valstr);
                return BAD_VALUE;
            }
        }

        if ((valstr =params.get(TICameraParameters::KEY_SHARPNESS)) != NULL) {
            if (params.getInt(TICameraParameters::KEY_SHARPNESS) >= 0 ) {
                CAMHAL_LOGDB("Sharpness set %s", valstr);
                mParameters.set(TICameraParameters::KEY_SHARPNESS, valstr);
            } else {
                CAMHAL_LOGEB("ERROR: Invalid Sharpness = %s", valstr);
                return BAD_VALUE;
            }
        }

        if ((valstr = params.get(TICameraParameters::KEY_SATURATION)) != NULL) {
            if (params.getInt(TICameraParameters::KEY_SATURATION) >= 0 ) {
                CAMHAL_LOGDB("Saturation set %s", valstr);
                mParameters.set(TICameraParameters::KEY_SATURATION, valstr);
             } else {
                CAMHAL_LOGEB("ERROR: Invalid Saturation = %s", valstr);
                return BAD_VALUE;
            }
        }

        if ((valstr = params.get(TICameraParameters::KEY_BRIGHTNESS)) != NULL) {
            if (params.getInt(TICameraParameters::KEY_BRIGHTNESS) >= 0 ) {
                CAMHAL_LOGDB("Brightness set %s", valstr);
                mParameters.set(TICameraParameters::KEY_BRIGHTNESS, valstr);
            } else {
                CAMHAL_LOGEB("ERROR: Invalid Brightness = %s", valstr);
                return BAD_VALUE;
            }
         }
#endif

        if ((valstr = params.get(android::CameraParameters::KEY_ANTIBANDING)) != NULL) {
            if (isParameterValid(valstr, mCameraProperties->get(CameraProperties::SUPPORTED_ANTIBANDING))) {
                CAMHAL_LOGDB("Antibanding set %s", valstr);
                mParameters.set(android::CameraParameters::KEY_ANTIBANDING, valstr);
             } else {
                CAMHAL_LOGEB("ERROR: Invalid Antibanding = %s", valstr);
                return BAD_VALUE;
             }
         }

#ifdef OMAP_ENHANCEMENT
        if ((valstr = params.get(TICameraParameters::KEY_ISO)) != NULL) {
            if (isParameterValid(valstr, mCameraProperties->get(CameraProperties::SUPPORTED_ISO_VALUES))) {
                CAMHAL_LOGDB("ISO set %s", valstr);
                mParameters.set(TICameraParameters::KEY_ISO, valstr);
            } else {
                CAMHAL_LOGEB("ERROR: Invalid ISO = %s", valstr);
                return BAD_VALUE;
            }
        }
#endif

        if( (valstr = params.get(android::CameraParameters::KEY_FOCUS_AREAS)) != NULL )
            {
            CAMHAL_LOGDB("Focus areas position set %s", params.get(android::CameraParameters::KEY_FOCUS_AREAS));
            mParameters.set(android::CameraParameters::KEY_FOCUS_AREAS, valstr);
            }

#ifdef OMAP_ENHANCEMENT
        if( (valstr = params.get(TICameraParameters::KEY_MEASUREMENT_ENABLE)) != NULL )
            {
            CAMHAL_LOGDB("Measurements set to %s", valstr);
            mParameters.set(TICameraParameters::KEY_MEASUREMENT_ENABLE, valstr);

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
#endif

        if( (valstr = params.get(android::CameraParameters::KEY_EXPOSURE_COMPENSATION)) != NULL)
            {
            CAMHAL_LOGDB("Exposure compensation set %s", params.get(android::CameraParameters::KEY_EXPOSURE_COMPENSATION));
            mParameters.set(android::CameraParameters::KEY_EXPOSURE_COMPENSATION, valstr);
            }

        if ((valstr = params.get(android::CameraParameters::KEY_SCENE_MODE)) != NULL) {
            if (isParameterValid(valstr, mCameraProperties->get(CameraProperties::SUPPORTED_SCENE_MODES))) {
                CAMHAL_LOGDB("Scene mode set %s", valstr);
                doesSetParameterNeedUpdate(valstr,
                                           mParameters.get(android::CameraParameters::KEY_SCENE_MODE),
                                           updateRequired);
                mParameters.set(android::CameraParameters::KEY_SCENE_MODE, valstr);
            } else {
                CAMHAL_LOGEB("ERROR: Invalid Scene mode = %s", valstr);
                return BAD_VALUE;
            }
        }

        if ((valstr = params.get(android::CameraParameters::KEY_FLASH_MODE)) != NULL) {
            if (isParameterValid(valstr, mCameraProperties->get(CameraProperties::SUPPORTED_FLASH_MODES))) {
                CAMHAL_LOGDB("Flash mode set %s", valstr);
                mParameters.set(android::CameraParameters::KEY_FLASH_MODE, valstr);
            } else {
                CAMHAL_LOGEB("ERROR: Invalid Flash mode = %s", valstr);
                return BAD_VALUE;
            }
        }

        if ((valstr = params.get(android::CameraParameters::KEY_EFFECT)) != NULL) {
            if (isParameterValid(valstr, mCameraProperties->get(CameraProperties::SUPPORTED_EFFECTS))) {
                CAMHAL_LOGDB("Effect set %s", valstr);
                mParameters.set(android::CameraParameters::KEY_EFFECT, valstr);
             } else {
                CAMHAL_LOGEB("ERROR: Invalid Effect = %s", valstr);
                return BAD_VALUE;
             }
        }

        varint = params.getInt(android::CameraParameters::KEY_ROTATION);
        if ( varint >= 0 ) {
            CAMHAL_LOGDB("Rotation set %d", varint);
            mParameters.set(android::CameraParameters::KEY_ROTATION, varint);
        }

        varint = params.getInt(android::CameraParameters::KEY_JPEG_QUALITY);
        if ( varint >= 0 ) {
            CAMHAL_LOGDB("Jpeg quality set %d", varint);
            mParameters.set(android::CameraParameters::KEY_JPEG_QUALITY, varint);
        }

        varint = params.getInt(android::CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH);
        if ( varint >= 0 ) {
            CAMHAL_LOGDB("Thumbnail width set %d", varint);
            mParameters.set(android::CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH, varint);
        }

        varint = params.getInt(android::CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT);
        if ( varint >= 0 ) {
            CAMHAL_LOGDB("Thumbnail width set %d", varint);
            mParameters.set(android::CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT, varint);
        }

        varint = params.getInt(android::CameraParameters::KEY_JPEG_THUMBNAIL_QUALITY);
        if ( varint >= 0 ) {
            CAMHAL_LOGDB("Thumbnail quality set %d", varint);
            mParameters.set(android::CameraParameters::KEY_JPEG_THUMBNAIL_QUALITY, varint);
        }

        if( (valstr = params.get(android::CameraParameters::KEY_GPS_LATITUDE)) != NULL )
            {
            CAMHAL_LOGDB("GPS latitude set %s", params.get(android::CameraParameters::KEY_GPS_LATITUDE));
            mParameters.set(android::CameraParameters::KEY_GPS_LATITUDE, valstr);
            }else{
                mParameters.remove(android::CameraParameters::KEY_GPS_LATITUDE);
            }

        if( (valstr = params.get(android::CameraParameters::KEY_GPS_LONGITUDE)) != NULL )
            {
            CAMHAL_LOGDB("GPS longitude set %s", params.get(android::CameraParameters::KEY_GPS_LONGITUDE));
            mParameters.set(android::CameraParameters::KEY_GPS_LONGITUDE, valstr);
            }else{
                mParameters.remove(android::CameraParameters::KEY_GPS_LONGITUDE);
            }

        if( (valstr = params.get(android::CameraParameters::KEY_GPS_ALTITUDE)) != NULL )
            {
            CAMHAL_LOGDB("GPS altitude set %s", params.get(android::CameraParameters::KEY_GPS_ALTITUDE));
            mParameters.set(android::CameraParameters::KEY_GPS_ALTITUDE, valstr);
            }else{
                mParameters.remove(android::CameraParameters::KEY_GPS_ALTITUDE);
            }

        if( (valstr = params.get(android::CameraParameters::KEY_GPS_TIMESTAMP)) != NULL )
            {
            CAMHAL_LOGDB("GPS timestamp set %s", params.get(android::CameraParameters::KEY_GPS_TIMESTAMP));
            mParameters.set(android::CameraParameters::KEY_GPS_TIMESTAMP, valstr);
            }else{
                mParameters.remove(android::CameraParameters::KEY_GPS_TIMESTAMP);
            }

        if( (valstr = params.get(TICameraParameters::KEY_GPS_DATESTAMP)) != NULL )
            {
            CAMHAL_LOGDB("GPS datestamp set %s", valstr);
            mParameters.set(TICameraParameters::KEY_GPS_DATESTAMP, valstr);
            }else{
                mParameters.remove(TICameraParameters::KEY_GPS_DATESTAMP);
            }

        if( (valstr = params.get(android::CameraParameters::KEY_GPS_PROCESSING_METHOD)) != NULL )
            {
            CAMHAL_LOGDB("GPS processing method set %s", params.get(android::CameraParameters::KEY_GPS_PROCESSING_METHOD));
            mParameters.set(android::CameraParameters::KEY_GPS_PROCESSING_METHOD, valstr);
            }else{
                mParameters.remove(android::CameraParameters::KEY_GPS_PROCESSING_METHOD);
            }

        if( (valstr = params.get(TICameraParameters::KEY_GPS_MAPDATUM )) != NULL )
            {
            CAMHAL_LOGDB("GPS MAPDATUM set %s", valstr);
            mParameters.set(TICameraParameters::KEY_GPS_MAPDATUM, valstr);
            }else{
                mParameters.remove(TICameraParameters::KEY_GPS_MAPDATUM);
            }

        if( (valstr = params.get(TICameraParameters::KEY_GPS_VERSION)) != NULL )
            {
            CAMHAL_LOGDB("GPS MAPDATUM set %s", valstr);
            mParameters.set(TICameraParameters::KEY_GPS_VERSION, valstr);
            }else{
                mParameters.remove(TICameraParameters::KEY_GPS_VERSION);
            }

        if( (valstr = params.get(TICameraParameters::KEY_EXIF_MODEL)) != NULL )
            {
            CAMHAL_LOGDB("EXIF Model set %s", valstr);
            mParameters.set(TICameraParameters::KEY_EXIF_MODEL, valstr);
            }

        if( (valstr = params.get(TICameraParameters::KEY_EXIF_MAKE)) != NULL )
            {
            CAMHAL_LOGDB("EXIF Make set %s", valstr);
            mParameters.set(TICameraParameters::KEY_EXIF_MAKE, valstr);
            }

#ifdef OMAP_ENHANCEMENT
        if( (valstr = params.get(TICameraParameters::KEY_EXP_BRACKETING_RANGE)) != NULL )
            {
            CAMHAL_LOGDB("Exposure Bracketing set %s", params.get(TICameraParameters::KEY_EXP_BRACKETING_RANGE));
            mParameters.set(TICameraParameters::KEY_EXP_BRACKETING_RANGE, valstr);
            mParameters.remove(TICameraParameters::KEY_EXP_GAIN_BRACKETING_RANGE);
            }
        else if ((valstr = params.get(TICameraParameters::KEY_EXP_GAIN_BRACKETING_RANGE)) != NULL) {
            CAMHAL_LOGDB("ABS Exposure+Gain Bracketing set %s", params.get(TICameraParameters::KEY_EXP_GAIN_BRACKETING_RANGE));
            mParameters.set(TICameraParameters::KEY_EXP_GAIN_BRACKETING_RANGE, valstr);
            mParameters.remove(TICameraParameters::KEY_EXP_BRACKETING_RANGE);
        } else
            {
            mParameters.remove(TICameraParameters::KEY_EXP_BRACKETING_RANGE);
            }

        if( (valstr = params.get(TICameraParameters::KEY_ZOOM_BRACKETING_RANGE)) != NULL ) {
            CAMHAL_LOGDB("Zoom Bracketing range %s", valstr);
            mParameters.set(TICameraParameters::KEY_ZOOM_BRACKETING_RANGE, valstr);
        } else {
            mParameters.remove(TICameraParameters::KEY_ZOOM_BRACKETING_RANGE);
        }
#endif

        if ((valstr = params.get(android::CameraParameters::KEY_ZOOM)) != NULL ) {
            varint = atoi(valstr);
            if ( varint >= 0 && varint <= mMaxZoomSupported ) {
                CAMHAL_LOGDB("Zoom set %d", varint);
                doesSetParameterNeedUpdate(valstr,
                                           mParameters.get(android::CameraParameters::KEY_ZOOM),
                                           updateRequired);
                mParameters.set(android::CameraParameters::KEY_ZOOM, valstr);
             } else {
                CAMHAL_LOGEB("ERROR: Invalid Zoom: %s", valstr);
                return BAD_VALUE;
            }
        }

        if( (valstr = params.get(android::CameraParameters::KEY_AUTO_EXPOSURE_LOCK)) != NULL )
          {
            CAMHAL_LOGDB("Auto Exposure Lock set %s", valstr);
            doesSetParameterNeedUpdate(valstr,
                                       mParameters.get(android::CameraParameters::KEY_AUTO_EXPOSURE_LOCK),
                                       updateRequired);
            mParameters.set(android::CameraParameters::KEY_AUTO_EXPOSURE_LOCK, valstr);
          }

        if( (valstr = params.get(android::CameraParameters::KEY_AUTO_WHITEBALANCE_LOCK)) != NULL )
          {
            CAMHAL_LOGDB("Auto WhiteBalance Lock set %s", valstr);
            doesSetParameterNeedUpdate(valstr,
                                       mParameters.get(android::CameraParameters::KEY_AUTO_WHITEBALANCE_LOCK),
                                       updateRequired);
            mParameters.set(android::CameraParameters::KEY_AUTO_WHITEBALANCE_LOCK, valstr);
          }
        if( (valstr = params.get(android::CameraParameters::KEY_METERING_AREAS)) != NULL )
            {
            CAMHAL_LOGDB("Metering areas position set %s", params.get(android::CameraParameters::KEY_METERING_AREAS));
            mParameters.set(android::CameraParameters::KEY_METERING_AREAS, valstr);
            }

        if( (valstr = params.get(TICameraParameters::RAW_WIDTH)) != NULL ) {
            CAMHAL_LOGDB("Raw image width set %s", params.get(TICameraParameters::RAW_WIDTH));
            mParameters.set(TICameraParameters::RAW_WIDTH, valstr);
        }

        if( (valstr = params.get(TICameraParameters::RAW_HEIGHT)) != NULL ) {
            CAMHAL_LOGDB("Raw image height set %s", params.get(TICameraParameters::RAW_HEIGHT));
            mParameters.set(TICameraParameters::RAW_HEIGHT, valstr);
        }

        //TI extensions for enable/disable algos
        if( (valstr = params.get(TICameraParameters::KEY_ALGO_EXTERNAL_GAMMA)) != NULL )
            {
            CAMHAL_LOGDB("External Gamma set %s", valstr);
            mParameters.set(TICameraParameters::KEY_ALGO_EXTERNAL_GAMMA, valstr);
            }

        if( (valstr = params.get(TICameraParameters::KEY_ALGO_NSF1)) != NULL )
            {
            CAMHAL_LOGDB("NSF1 set %s", valstr);
            mParameters.set(TICameraParameters::KEY_ALGO_NSF1, valstr);
            }

        if( (valstr = params.get(TICameraParameters::KEY_ALGO_NSF2)) != NULL )
            {
            CAMHAL_LOGDB("NSF2 set %s", valstr);
            mParameters.set(TICameraParameters::KEY_ALGO_NSF2, valstr);
            }

        if( (valstr = params.get(TICameraParameters::KEY_ALGO_SHARPENING)) != NULL )
            {
            CAMHAL_LOGDB("Sharpening set %s", valstr);
            mParameters.set(TICameraParameters::KEY_ALGO_SHARPENING, valstr);
            }

        if( (valstr = params.get(TICameraParameters::KEY_ALGO_THREELINCOLORMAP)) != NULL )
            {
            CAMHAL_LOGDB("Color Conversion set %s", valstr);
            mParameters.set(TICameraParameters::KEY_ALGO_THREELINCOLORMAP, valstr);
            }

        if( (valstr = params.get(TICameraParameters::KEY_ALGO_GIC)) != NULL )
            {
            CAMHAL_LOGDB("Green Inballance Correction set %s", valstr);
            mParameters.set(TICameraParameters::KEY_ALGO_GIC, valstr);
            }

        if( (valstr = params.get(TICameraParameters::KEY_GAMMA_TABLE)) != NULL )
            {
            CAMHAL_LOGDB("Manual gamma table set %s", valstr);
            mParameters.set(TICameraParameters::KEY_GAMMA_TABLE, valstr);
            }

        android::CameraParameters adapterParams = mParameters;

#ifdef OMAP_ENHANCEMENT
        if( NULL != params.get(TICameraParameters::KEY_TEMP_BRACKETING_RANGE_POS) )
            {
            int posBracketRange = params.getInt(TICameraParameters::KEY_TEMP_BRACKETING_RANGE_POS);
            if ( 0 < posBracketRange )
                {
                mBracketRangePositive = posBracketRange;
                }
            }
        CAMHAL_LOGDB("Positive bracketing range %d", mBracketRangePositive);


        if( NULL != params.get(TICameraParameters::KEY_TEMP_BRACKETING_RANGE_NEG) )
            {
            int negBracketRange = params.getInt(TICameraParameters::KEY_TEMP_BRACKETING_RANGE_NEG);
            if ( 0 < negBracketRange )
                {
                mBracketRangeNegative = negBracketRange;
                }
            }
        CAMHAL_LOGDB("Negative bracketing range %d", mBracketRangeNegative);

        if( ( (valstr = params.get(TICameraParameters::KEY_TEMP_BRACKETING)) != NULL) &&
            ( strcmp(valstr, android::CameraParameters::TRUE) == 0 )) {
            if ( !mBracketingEnabled ) {
                CAMHAL_LOGDA("Enabling bracketing");
                mBracketingEnabled = true;
            } else {
                CAMHAL_LOGDA("Bracketing already enabled");
            }
            adapterParams.set(TICameraParameters::KEY_TEMP_BRACKETING, valstr);
            mParameters.set(TICameraParameters::KEY_TEMP_BRACKETING, valstr);
        } else if ( ( (valstr = params.get(TICameraParameters::KEY_TEMP_BRACKETING)) != NULL ) &&
            ( strcmp(valstr, android::CameraParameters::FALSE) == 0 )) {
            CAMHAL_LOGDA("Disabling bracketing");

            adapterParams.set(TICameraParameters::KEY_TEMP_BRACKETING, valstr);
            mParameters.set(TICameraParameters::KEY_TEMP_BRACKETING, valstr);
            mBracketingEnabled = false;
            if ( mBracketingRunning ) {
                stopImageBracketing();
            }

        } else {
            adapterParams.remove(TICameraParameters::KEY_TEMP_BRACKETING);
            mParameters.remove(TICameraParameters::KEY_TEMP_BRACKETING);
        }
#endif

#ifdef OMAP_ENHANCEMENT_VTC
        if (mVTCUseCase && !mTunnelSetup && (mCameraAdapter != NULL) &&
                ((mParameters.get(TICameraParameters::KEY_VIDEO_ENCODER_HANDLE)) != NULL )&&
                ((mParameters.get(TICameraParameters::KEY_VIDEO_ENCODER_SLICE_HEIGHT)) != NULL )) {

            uint32_t sliceHeight = mParameters.getInt(TICameraParameters::KEY_VIDEO_ENCODER_SLICE_HEIGHT);
            uint32_t encoderHandle = mParameters.getInt(TICameraParameters::KEY_VIDEO_ENCODER_HANDLE);
            int w, h;
            mParameters.getPreviewSize(&w, &h);
            status_t done = mCameraAdapter->sendCommand(CameraAdapter::CAMERA_SETUP_TUNNEL, sliceHeight, encoderHandle, w, h);
            if (done == NO_ERROR) mTunnelSetup = true;
            ret |= done;
        }
#endif

        // Only send parameters to adapter if preview is already
        // enabled or doesSetParameterNeedUpdate says so. Initial setParameters to camera adapter,
        // will be called in startPreview()
        // TODO(XXX): Need to identify other parameters that need update from camera adapter
        if ( (NULL != mCameraAdapter) &&
             (mPreviewEnabled || updateRequired) &&
             (!(mPreviewEnabled && restartPreviewRequired)) ) {
            ret |= mCameraAdapter->setParameters(adapterParams);
        }

#ifdef OMAP_ENHANCEMENT
        if( ( (valstr = params.get(TICameraParameters::KEY_SHUTTER_ENABLE)) != NULL ) &&
            ( strcmp(valstr, android::CameraParameters::TRUE) == 0 ))
            {
            CAMHAL_LOGDA("Enabling shutter sound");

            mShutterEnabled = true;
            mMsgEnabled |= CAMERA_MSG_SHUTTER;
            mParameters.set(TICameraParameters::KEY_SHUTTER_ENABLE, valstr);
            }
        else if ( ( (valstr = params.get(TICameraParameters::KEY_SHUTTER_ENABLE)) != NULL ) &&
            ( strcmp(valstr, android::CameraParameters::FALSE) == 0 ))
            {
            CAMHAL_LOGDA("Disabling shutter sound");

            mShutterEnabled = false;
            mMsgEnabled &= ~CAMERA_MSG_SHUTTER;
            mParameters.set(TICameraParameters::KEY_SHUTTER_ENABLE, valstr);
            }
#endif
    }

    //On fail restore old parameters
    if ( NO_ERROR != ret ) {
        mParameters = oldParams;
    }

    // Restart Preview if needed by KEY_RECODING_HINT only if preview is already running.
    // If preview is not started yet, Video Mode parameters will take effect on next startPreview()
    if (restartPreviewRequired && previewEnabled() && !mRecordingEnabled) {
        CAMHAL_LOGDA("Restarting Preview");
        ret = restartPreview();
    } else if (restartPreviewRequired && !previewEnabled() &&
                mDisplayPaused && !mRecordingEnabled) {
        CAMHAL_LOGDA("Restarting preview in paused mode");
        ret = restartPreview();

        // TODO(XXX): If there is some delay between the restartPreview call and the code
        // below, then the user could see some preview frames and callbacks. Let's find
        // a better place to put this later...
        if (ret == NO_ERROR) {
            mDisplayPaused = true;
            mPreviewEnabled = false;
            ret = mDisplayAdapter->pauseDisplay(mDisplayPaused);
        }
    }

    if ( !mBracketingRunning && mBracketingEnabled ) {
        startImageBracketing();
    }

    if (ret != NO_ERROR)
        {
        CAMHAL_LOGEA("Failed to restart Preview");
        return ret;
        }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t CameraHal::allocPreviewBufs(int width, int height, const char* previewFormat,
                                        unsigned int buffercount, unsigned int &max_queueable)
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    if(mDisplayAdapter.get() == NULL)
    {
        // Memory allocation of preview buffers is now placed in gralloc
        // CameraHal should not allocate preview buffers without DisplayAdapter
        return NO_MEMORY;
    }

    if(!mPreviewBuffers)
    {
        mPreviewLength = 0;
        mPreviewBuffers = mDisplayAdapter->allocateBufferList(width, height,
                                                                    previewFormat,
                                                                    mPreviewLength,
                                                                    buffercount);
        if (NULL == mPreviewBuffers ) {
            CAMHAL_LOGEA("Couldn't allocate preview buffers");
            return NO_MEMORY;
        }

        mPreviewOffsets = (uint32_t *) mDisplayAdapter->getOffsets();
        if ( NULL == mPreviewOffsets ) {
            CAMHAL_LOGEA("Buffer mapping failed");
            return BAD_VALUE;
        }

        mBufProvider = (BufferProvider*) mDisplayAdapter.get();

        ret = mDisplayAdapter->maxQueueableBuffers(max_queueable);
        if (ret != NO_ERROR) {
            return ret;
        }
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t CameraHal::freePreviewBufs()
{
    status_t ret = NO_ERROR;
    LOG_FUNCTION_NAME;

    CAMHAL_LOGDB("mPreviewBuffers = %p", mPreviewBuffers);
    if(mPreviewBuffers)
        {
        ret = mBufProvider->freeBufferList(mPreviewBuffers);
        mPreviewBuffers = NULL;
        LOG_FUNCTION_NAME_EXIT;
        return ret;
        }
    LOG_FUNCTION_NAME_EXIT;
    return ret;
}


status_t CameraHal::allocPreviewDataBufs(size_t size, size_t bufferCount)
{
    status_t ret = NO_ERROR;
    int bytes;

    LOG_FUNCTION_NAME;

    bytes = size;

    if ( NO_ERROR == ret )
        {
        if( NULL != mPreviewDataBuffers )
            {
            ret = freePreviewDataBufs();
            }
        }

    if ( NO_ERROR == ret )
        {
        bytes = ((bytes+4095)/4096)*4096;
        mPreviewDataBuffers = mMemoryManager->allocateBufferList(0, 0, NULL, bytes, bufferCount);

        CAMHAL_LOGDB("Size of Preview data buffer = %d", bytes);
        if( NULL == mPreviewDataBuffers )
            {
            CAMHAL_LOGEA("Couldn't allocate image buffers using memory manager");
            ret = -NO_MEMORY;
            }
        else
            {
            bytes = size;
            }
        }

    if ( NO_ERROR == ret )
        {
        mPreviewDataFd = mMemoryManager->getFd();
        mPreviewDataLength = bytes;
        mPreviewDataOffsets = mMemoryManager->getOffsets();
        }
    else
        {
        mPreviewDataFd = -1;
        mPreviewDataLength = 0;
        mPreviewDataOffsets = NULL;
        }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t CameraHal::freePreviewDataBufs()
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    if ( NO_ERROR == ret )
        {

        if( NULL != mPreviewDataBuffers )
            {

            ret = mMemoryManager->freeBufferList(mPreviewDataBuffers);
            mPreviewDataBuffers = NULL;

            }
        }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t CameraHal::allocImageBufs(unsigned int width, unsigned int height, size_t size,
                                   const char* previewFormat, unsigned int bufferCount)
{
    status_t ret = NO_ERROR;
    int bytes = size;

    LOG_FUNCTION_NAME;

    // allocate image buffers only if not already allocated
    if(NULL != mImageBuffers) {
        return NO_ERROR;
    }

    if ( NO_ERROR == ret ) {
        bytes = ((bytes+4095)/4096)*4096;
        mImageBuffers = mMemoryManager->allocateBufferList(0, 0, previewFormat, bytes, bufferCount);
        CAMHAL_LOGDB("Size of Image cap buffer = %d", bytes);
        if( NULL == mImageBuffers ) {
            CAMHAL_LOGEA("Couldn't allocate image buffers using memory manager");
            ret = -NO_MEMORY;
        } else {
            bytes = size;
        }
    }

    if ( NO_ERROR == ret ) {
        mImageFd = mMemoryManager->getFd();
        mImageLength = bytes;
        mImageOffsets = mMemoryManager->getOffsets();
        mImageCount = bufferCount;
    } else {
        mImageFd = -1;
        mImageLength = 0;
        mImageOffsets = NULL;
        mImageCount = 0;
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t CameraHal::allocVideoBufs(uint32_t width, uint32_t height, uint32_t bufferCount)
{
  status_t ret = NO_ERROR;
  LOG_FUNCTION_NAME;

  if( NULL != mVideoBuffers ){
    ret = freeVideoBufs(mVideoBuffers);
    mVideoBuffers = NULL;
  }

  if ( NO_ERROR == ret ){
    int32_t stride;
    CameraBuffer *buffers = new CameraBuffer [bufferCount];

    memset (buffers, 0, sizeof(CameraBuffer) * bufferCount);

    if (buffers != NULL){
      for (unsigned int i = 0; i< bufferCount; i++){
        android::GraphicBufferAllocator &GrallocAlloc = android::GraphicBufferAllocator::get();
        buffer_handle_t handle;
        ret = GrallocAlloc.alloc(width, height, HAL_PIXEL_FORMAT_NV12, CAMHAL_GRALLOC_USAGE, &handle, &stride);
        if (ret != NO_ERROR){
          CAMHAL_LOGEA("Couldn't allocate video buffers using Gralloc");
          ret = -NO_MEMORY;
          for (unsigned int j=0; j< i; j++){
            CAMHAL_LOGEB("Freeing Gralloc Buffer %p", buffers[i].opaque);
            GrallocAlloc.free((buffer_handle_t)buffers[i].opaque);
          }
          delete [] buffers;
          goto exit;
        }
        buffers[i].type = CAMERA_BUFFER_GRALLOC;
        buffers[i].opaque = (void *)handle;
        CAMHAL_LOGVB("*** Gralloc Handle =0x%x ***", handle);
      }

      mVideoBuffers = buffers;
    }
    else{
      CAMHAL_LOGEA("Couldn't allocate video buffers ");
      ret = -NO_MEMORY;
    }
  }

 exit:
  LOG_FUNCTION_NAME_EXIT;

  return ret;
}

status_t CameraHal::allocRawBufs(int width, int height, const char* previewFormat, int bufferCount)
{
   status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME


    ///@todo Enhance this method allocImageBufs() to take in a flag for burst capture
    ///Always allocate the buffers for image capture using MemoryManager
    if (NO_ERROR == ret) {
        if(( NULL != mVideoBuffers )) {
            // Re-use the buffer for raw capture.
            return ret;
        }
    }

    if ( NO_ERROR == ret ) {
        mVideoLength = 0;
        mVideoLength = (((width * height * 2) + 4095)/4096)*4096;
        mVideoBuffers = mMemoryManager->allocateBufferList(width, height, previewFormat,
                                                           mVideoLength, bufferCount);

        CAMHAL_LOGDB("Size of Video cap buffer (used for RAW capture) %d", mVideoLength);
        if( NULL == mVideoBuffers ) {
            CAMHAL_LOGEA("Couldn't allocate Video buffers using memory manager");
            ret = -NO_MEMORY;
        }
    }

    if ( NO_ERROR == ret ) {
        mVideoFd = mMemoryManager->getFd();
        mVideoOffsets = mMemoryManager->getOffsets();
    } else {
        mVideoFd = -1;
        mVideoOffsets = NULL;
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

void endImageCapture( void *userData)
{
    LOG_FUNCTION_NAME;

    if ( NULL != userData )
        {
        CameraHal *c = reinterpret_cast<CameraHal *>(userData);
        c->signalEndImageCapture();
        }

    LOG_FUNCTION_NAME_EXIT;
}

void releaseImageBuffers(void *userData)
{
    LOG_FUNCTION_NAME;

    if (NULL != userData) {
        CameraHal *c = reinterpret_cast<CameraHal *>(userData);
        c->freeImageBufs();
    }

    LOG_FUNCTION_NAME_EXIT;
}

status_t CameraHal::signalEndImageCapture()
{
    status_t ret = NO_ERROR;
    int w,h;
    android::AutoMutex lock(mLock);

    LOG_FUNCTION_NAME;

    if (mBufferSourceAdapter_Out.get()) {
        mBufferSourceAdapter_Out->disableDisplay();
    }

    if (mBufferSourceAdapter_In.get()) {
        mBufferSourceAdapter_In->disableDisplay();
    }

    if ( mBracketingRunning ) {
        stopImageBracketing();
    } else {
        mCameraAdapter->sendCommand(CameraAdapter::CAMERA_STOP_IMAGE_CAPTURE);
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t CameraHal::freeImageBufs()
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    if (NULL == mImageBuffers) {
        return -EINVAL;
    }

    if (mBufferSourceAdapter_Out.get()) {
        mBufferSourceAdapter_Out = 0;
    } else {
        ret = mMemoryManager->freeBufferList(mImageBuffers);
    }

    mImageBuffers = NULL;

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t CameraHal::freeVideoBufs(CameraBuffer *bufs)
{
  status_t ret = NO_ERROR;

  LOG_FUNCTION_NAME;

  int count = atoi(mCameraProperties->get(CameraProperties::REQUIRED_PREVIEW_BUFS));
  if(bufs == NULL)
    {
      CAMHAL_LOGEA("NULL pointer passed to freeVideoBuffer");
      LOG_FUNCTION_NAME_EXIT;
      return BAD_VALUE;
    }

  android::GraphicBufferAllocator &GrallocAlloc = android::GraphicBufferAllocator::get();

  for(int i = 0; i < count; i++){
    CAMHAL_LOGVB("Free Video Gralloc Handle 0x%x", bufs[i].opaque);
    GrallocAlloc.free((buffer_handle_t)bufs[i].opaque);
  }

  LOG_FUNCTION_NAME_EXIT;

  return ret;
}

status_t CameraHal::freeRawBufs()
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME

    if ( NO_ERROR == ret ) {
        if( NULL != mVideoBuffers ) {
            ///@todo Pluralise the name of this method to freeBuffers
            ret = mMemoryManager->freeBufferList(mVideoBuffers);
            mVideoBuffers = NULL;
        } else {
            ret = -EINVAL;
        }
    }

    LOG_FUNCTION_NAME_EXIT

    return ret;
}

/**
   @brief Start preview mode.

   @param none
   @return NO_ERROR Camera switched to VF mode
   @todo Update function header with the different errors that are possible

 */
status_t CameraHal::startPreview() {
    LOG_FUNCTION_NAME;

    // When tunneling is enabled during VTC, startPreview happens in 2 steps:
    // When the application sends the command CAMERA_CMD_PREVIEW_INITIALIZATION,
    // cameraPreviewInitialization() is called, which in turn causes the CameraAdapter
    // to move from loaded to idle state. And when the application calls startPreview,
    // the CameraAdapter moves from idle to executing state.
    //
    // If the application calls startPreview() without sending the command
    // CAMERA_CMD_PREVIEW_INITIALIZATION, then the function cameraPreviewInitialization()
    // AND startPreview() are executed. In other words, if the application calls
    // startPreview() without sending the command CAMERA_CMD_PREVIEW_INITIALIZATION,
    // then the CameraAdapter moves from loaded to idle to executing state in one shot.
    status_t ret = cameraPreviewInitialization();

    // The flag mPreviewInitializationDone is set to true at the end of the function
    // cameraPreviewInitialization(). Therefore, if everything goes alright, then the
    // flag will be set. Sometimes, the function cameraPreviewInitialization() may
    // return prematurely if all the resources are not available for starting preview.
    // For example, if the preview window is not set, then it would return NO_ERROR.
    // Under such circumstances, one should return from startPreview as well and should
    // not continue execution. That is why, we check the flag and not the return value.
    if (!mPreviewInitializationDone) return ret;

    // Once startPreview is called, there is no need to continue to remember whether
    // the function cameraPreviewInitialization() was called earlier or not. And so
    // the flag mPreviewInitializationDone is reset here. Plus, this preserves the
    // current behavior of startPreview under the circumstances where the application
    // calls startPreview twice or more.
    mPreviewInitializationDone = false;

    ///Enable the display adapter if present, actual overlay enable happens when we post the buffer
    if(mDisplayAdapter.get() != NULL) {
        CAMHAL_LOGDA("Enabling display");
        int width, height;
        mParameters.getPreviewSize(&width, &height);

#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS
        ret = mDisplayAdapter->enableDisplay(width, height, &mStartPreview);
#else
        ret = mDisplayAdapter->enableDisplay(width, height, NULL);
#endif

        if ( ret != NO_ERROR ) {
            CAMHAL_LOGEA("Couldn't enable display");

            // FIXME: At this stage mStateSwitchLock is locked and unlock is supposed to be called
            //        only from mCameraAdapter->sendCommand(CameraAdapter::CAMERA_START_PREVIEW)
            //        below. But this will never happen because of goto error. Thus at next
            //        startPreview() call CameraHAL will be deadlocked.
            //        Need to revisit mStateSwitch lock, for now just abort the process.
            CAMHAL_ASSERT_X(false,
                "At this stage mCameraAdapter->mStateSwitchLock is still locked, "
                "deadlock is guaranteed");

            goto error;
        }

    }

    ///Send START_PREVIEW command to adapter
    CAMHAL_LOGDA("Starting CameraAdapter preview mode");

    ret = mCameraAdapter->sendCommand(CameraAdapter::CAMERA_START_PREVIEW);

    if(ret!=NO_ERROR) {
        CAMHAL_LOGEA("Couldn't start preview w/ CameraAdapter");
        goto error;
    }
    CAMHAL_LOGDA("Started preview");

    mPreviewEnabled = true;
    mPreviewStartInProgress = false;
    return ret;

    error:

        CAMHAL_LOGEA("Performing cleanup after error");

        //Do all the cleanup
        freePreviewBufs();
        mCameraAdapter->sendCommand(CameraAdapter::CAMERA_STOP_PREVIEW);
        if(mDisplayAdapter.get() != NULL) {
            mDisplayAdapter->disableDisplay(false);
        }
        mAppCallbackNotifier->stop();
        mPreviewStartInProgress = false;
        mPreviewEnabled = false;
        LOG_FUNCTION_NAME_EXIT;

        return ret;
}

////////////
/**
   @brief Set preview mode related initialization
          -> Camera Adapter set params
          -> Allocate buffers
          -> Set use buffers for preview
   @param none
   @return NO_ERROR
   @todo Update function header with the different errors that are possible

 */
status_t CameraHal::cameraPreviewInitialization()
{

    status_t ret = NO_ERROR;
    CameraAdapter::BuffersDescriptor desc;
    CameraFrame frame;
    unsigned int required_buffer_count;
    unsigned int max_queueble_buffers;

#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS
        gettimeofday(&mStartPreview, NULL);
#endif

    LOG_FUNCTION_NAME;

    if (mPreviewInitializationDone) {
        return NO_ERROR;
    }

    if ( mPreviewEnabled ){
      CAMHAL_LOGDA("Preview already running");
      LOG_FUNCTION_NAME_EXIT;
      return ALREADY_EXISTS;
    }

    if ( NULL != mCameraAdapter ) {
      ret = mCameraAdapter->setParameters(mParameters);
    }

    if ((mPreviewStartInProgress == false) && (mDisplayPaused == false)){
      ret = mCameraAdapter->sendCommand(CameraAdapter::CAMERA_QUERY_RESOLUTION_PREVIEW,( int ) &frame);
      if ( NO_ERROR != ret ){
        CAMHAL_LOGEB("Error: CAMERA_QUERY_RESOLUTION_PREVIEW %d", ret);
        return ret;
      }

      ///Update the current preview width and height
      mPreviewWidth = frame.mWidth;
      mPreviewHeight = frame.mHeight;
    }

    ///If we don't have the preview callback enabled and display adapter,
    if(!mSetPreviewWindowCalled || (mDisplayAdapter.get() == NULL)){
      CAMHAL_LOGD("Preview not started. Preview in progress flag set");
      mPreviewStartInProgress = true;
      ret = mCameraAdapter->sendCommand(CameraAdapter::CAMERA_SWITCH_TO_EXECUTING);
      if ( NO_ERROR != ret ){
        CAMHAL_LOGEB("Error: CAMERA_SWITCH_TO_EXECUTING %d", ret);
        return ret;
      }
      return NO_ERROR;
    }

    if( (mDisplayAdapter.get() != NULL) && ( !mPreviewEnabled ) && ( mDisplayPaused ) )
        {
        CAMHAL_LOGDA("Preview is in paused state");

        mDisplayPaused = false;
        mPreviewEnabled = true;
        if ( NO_ERROR == ret )
            {
            ret = mDisplayAdapter->pauseDisplay(mDisplayPaused);

            if ( NO_ERROR != ret )
                {
                CAMHAL_LOGEB("Display adapter resume failed %x", ret);
                }
            }
        //restart preview callbacks
        if(mMsgEnabled & CAMERA_MSG_PREVIEW_FRAME)
        {
            mAppCallbackNotifier->enableMsgType (CAMERA_MSG_PREVIEW_FRAME);
        }

        signalEndImageCapture();
        return ret;
        }

    required_buffer_count = atoi(mCameraProperties->get(CameraProperties::REQUIRED_PREVIEW_BUFS));

    ///Allocate the preview buffers
    ret = allocPreviewBufs(mPreviewWidth, mPreviewHeight, mParameters.getPreviewFormat(), required_buffer_count, max_queueble_buffers);

    if ( NO_ERROR != ret )
        {
        CAMHAL_LOGEA("Couldn't allocate buffers for Preview");
        goto error;
        }

    if ( mMeasurementEnabled )
        {

        ret = mCameraAdapter->sendCommand(CameraAdapter::CAMERA_QUERY_BUFFER_SIZE_PREVIEW_DATA,
                                          ( int ) &frame,
                                          required_buffer_count);
        if ( NO_ERROR != ret )
            {
            return ret;
            }

         ///Allocate the preview data buffers
        ret = allocPreviewDataBufs(frame.mLength, required_buffer_count);
        if ( NO_ERROR != ret ) {
            CAMHAL_LOGEA("Couldn't allocate preview data buffers");
            goto error;
           }

        if ( NO_ERROR == ret )
            {
            desc.mBuffers = mPreviewDataBuffers;
            desc.mOffsets = mPreviewDataOffsets;
            desc.mFd = mPreviewDataFd;
            desc.mLength = mPreviewDataLength;
            desc.mCount = ( size_t ) required_buffer_count;
            desc.mMaxQueueable = (size_t) required_buffer_count;

            mCameraAdapter->sendCommand(CameraAdapter::CAMERA_USE_BUFFERS_PREVIEW_DATA,
                                        ( int ) &desc);
            }

        }

    ///Pass the buffers to Camera Adapter
    desc.mBuffers = mPreviewBuffers;
    desc.mOffsets = mPreviewOffsets;
    desc.mFd = mPreviewFd;
    desc.mLength = mPreviewLength;
    desc.mCount = ( size_t ) required_buffer_count;
    desc.mMaxQueueable = (size_t) max_queueble_buffers;

    ret = mCameraAdapter->sendCommand(CameraAdapter::CAMERA_USE_BUFFERS_PREVIEW,
                                      ( int ) &desc);

    if ( NO_ERROR != ret )
        {
        CAMHAL_LOGEB("Failed to register preview buffers: 0x%x", ret);
        freePreviewBufs();
        return ret;
        }

    ///Start the callback notifier
    ret = mAppCallbackNotifier->start();

    if( ALREADY_EXISTS == ret )
        {
        //Already running, do nothing
        CAMHAL_LOGDA("AppCallbackNotifier already running");
        ret = NO_ERROR;
        }
    else if ( NO_ERROR == ret ) {
        CAMHAL_LOGDA("Started AppCallbackNotifier..");
        mAppCallbackNotifier->setMeasurements(mMeasurementEnabled);
        }
    else
        {
        CAMHAL_LOGDA("Couldn't start AppCallbackNotifier");
        goto error;
        }

    if (ret == NO_ERROR) mPreviewInitializationDone = true;

    mAppCallbackNotifier->startPreviewCallbacks(mParameters, mPreviewBuffers, mPreviewOffsets, mPreviewFd, mPreviewLength, required_buffer_count);

    return ret;

    error:

        CAMHAL_LOGEA("Performing cleanup after error");

        //Do all the cleanup
        freePreviewBufs();
        mCameraAdapter->sendCommand(CameraAdapter::CAMERA_STOP_PREVIEW);
        if(mDisplayAdapter.get() != NULL)
            {
            mDisplayAdapter->disableDisplay(false);
            }
        mAppCallbackNotifier->stop();
        mPreviewStartInProgress = false;
        mPreviewEnabled = false;
        LOG_FUNCTION_NAME_EXIT;

        return ret;
}

/**
   @brief Sets ANativeWindow object.

   Preview buffers provided to CameraHal via this object. DisplayAdapter will be interfacing with it
   to render buffers to display.

   @param[in] window The ANativeWindow object created by Surface flinger
   @return NO_ERROR If the ANativeWindow object passes validation criteria
   @todo Define validation criteria for ANativeWindow object. Define error codes for scenarios

 */
status_t CameraHal::setPreviewWindow(struct preview_stream_ops *window)
{
    status_t ret = NO_ERROR;
    CameraAdapter::BuffersDescriptor desc;

    LOG_FUNCTION_NAME;
    mSetPreviewWindowCalled = true;

   ///If the Camera service passes a null window, we destroy existing window and free the DisplayAdapter
    if(!window)
    {
        if(mDisplayAdapter.get() != NULL)
        {
            ///NULL window passed, destroy the display adapter if present
            CAMHAL_LOGD("NULL window passed, destroying display adapter");
            mDisplayAdapter.clear();
            ///@remarks If there was a window previously existing, we usually expect another valid window to be passed by the client
            ///@remarks so, we will wait until it passes a valid window to begin the preview again
            mSetPreviewWindowCalled = false;
        }
        CAMHAL_LOGD("NULL ANativeWindow passed to setPreviewWindow");
        return NO_ERROR;
    }else if(mDisplayAdapter.get() == NULL)
    {
        // Need to create the display adapter since it has not been created
        // Create display adapter
        mDisplayAdapter = new ANativeWindowDisplayAdapter();
#ifdef OMAP_ENHANCEMENT
        mDisplayAdapter->setExtendedOps(mExtendedPreviewStreamOps);
#endif
        ret = NO_ERROR;
        if(!mDisplayAdapter.get() || ((ret=mDisplayAdapter->initialize())!=NO_ERROR))
        {
            if(ret!=NO_ERROR)
            {
                mDisplayAdapter.clear();
                CAMHAL_LOGEA("DisplayAdapter initialize failed");
                LOG_FUNCTION_NAME_EXIT;
                return ret;
            }
            else
            {
                CAMHAL_LOGEA("Couldn't create DisplayAdapter");
                LOG_FUNCTION_NAME_EXIT;
                return NO_MEMORY;
            }
        }

        // DisplayAdapter needs to know where to get the CameraFrames from inorder to display
        // Since CameraAdapter is the one that provides the frames, set it as the frame provider for DisplayAdapter
        mDisplayAdapter->setFrameProvider(mCameraAdapter);

        // Any dynamic errors that happen during the camera use case has to be propagated back to the application
        // via CAMERA_MSG_ERROR. AppCallbackNotifier is the class that  notifies such errors to the application
        // Set it as the error handler for the DisplayAdapter
        mDisplayAdapter->setErrorHandler(mAppCallbackNotifier.get());

        // Update the display adapter with the new window that is passed from CameraService
        ret  = mDisplayAdapter->setPreviewWindow(window);
        if(ret!=NO_ERROR)
            {
            CAMHAL_LOGEB("DisplayAdapter setPreviewWindow returned error %d", ret);
            }

        if(mPreviewStartInProgress)
        {
            CAMHAL_LOGDA("setPreviewWindow called when preview running");
            // Start the preview since the window is now available
            ret = startPreview();
        }
    } else {
        // Update the display adapter with the new window that is passed from CameraService
        ret = mDisplayAdapter->setPreviewWindow(window);
        if ( (NO_ERROR == ret) && previewEnabled() ) {
            restartPreview();
        } else if (ret == ALREADY_EXISTS) {
            // ALREADY_EXISTS should be treated as a noop in this case
            ret = NO_ERROR;
        }
    }
    LOG_FUNCTION_NAME_EXIT;

    return ret;

}


#ifdef OMAP_ENHANCEMENT_CPCAM
void CameraHal::setExtendedPreviewStreamOps(preview_stream_extended_ops_t *ops)
{
    mExtendedPreviewStreamOps = ops;
}

/**
   @brief Sets Tapout Surfaces.

   Buffers provided to CameraHal via this object for tap-out
   functionality.

   @param[in] window The ANativeWindow object created by Surface flinger
   @return NO_ERROR If the ANativeWindow object passes validation criteria
   @todo Define validation criteria for ANativeWindow object. Define error codes for scenarios

 */
status_t CameraHal::setTapoutLocked(struct preview_stream_ops *tapout)
{
    status_t ret = NO_ERROR;
    int index = -1;

    LOG_FUNCTION_NAME;

    if (!tapout) {
        CAMHAL_LOGD("Missing argument");
        LOG_FUNCTION_NAME_EXIT;
        return NO_ERROR;
    }

    // Set tapout point
    // 1. Check name of tap-out
    // 2. If not already set, then create a new one
    // 3. Allocate buffers. If user is re-setting the surface, free buffers first and re-allocate
    //    in case dimensions have changed

    for (unsigned int i = 0; i < mOutAdapters.size(); i++) {
        android::sp<DisplayAdapter> out;
        out = mOutAdapters.itemAt(i);
        ret = out->setPreviewWindow(tapout);
        if (ret == ALREADY_EXISTS) {
            CAMHAL_LOGD("Tap Out already set at index = %d", i);
            index = i;
            ret = NO_ERROR;
        }
    }

    if (index < 0) {
        android::sp<DisplayAdapter> out  = new BufferSourceAdapter();

        ret = out->initialize();
        if (ret != NO_ERROR) {
            out.clear();
            CAMHAL_LOGEA("DisplayAdapter initialize failed");
            goto exit;
        }

        // BufferSourceAdapter will be handler of the extended OPS
        out->setExtendedOps(mExtendedPreviewStreamOps);

        // CameraAdapter will be the frame provider for BufferSourceAdapter
        out->setFrameProvider(mCameraAdapter);

        // BufferSourceAdapter will use ErrorHandler to send errors back to
        // the application
        out->setErrorHandler(mAppCallbackNotifier.get());

        // Update the display adapter with the new window that is passed from CameraService
        ret  = out->setPreviewWindow(tapout);
        if(ret != NO_ERROR) {
            CAMHAL_LOGEB("DisplayAdapter setPreviewWindow returned error %d", ret);
            goto exit;
        }

        if (NULL != mCameraAdapter) {
            unsigned int bufferCount, max_queueable;
            CameraFrame frame;

            bufferCount = out->getBufferCount();
            if (bufferCount < 1) bufferCount = NO_BUFFERS_IMAGE_CAPTURE_SYSTEM_HEAP;

            ret = mCameraAdapter->sendCommand(CameraAdapter::CAMERA_QUERY_BUFFER_SIZE_IMAGE_CAPTURE,
                                                  ( int ) &frame,
                                                  bufferCount);
            if (NO_ERROR != ret) {
                CAMHAL_LOGEB("CAMERA_QUERY_BUFFER_SIZE_IMAGE_CAPTURE returned error 0x%x", ret);
            }
            if (NO_ERROR == ret) {
                CameraBuffer *bufs = NULL;
                unsigned int stride;
                unsigned int height = frame.mHeight;
                int size = frame.mLength;

                stride = frame.mAlignment / getBPP(mParameters.getPictureFormat());
                bufs = out->allocateBufferList(stride,
                                               height,
                                               mParameters.getPictureFormat(),
                                               size,
                                               bufferCount);
                if (bufs == NULL){
                    CAMHAL_LOGEB("error allocating buffer list");
                    goto exit;
                }
            }
        }
        mOutAdapters.add(out);
    }

exit:

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

/**
   @brief Releases Tapout Surfaces.

   @param[in] window The ANativeWindow object created by Surface flinger
   @return NO_ERROR If the ANativeWindow object passes validation criteria
   @todo Define validation criteria for ANativeWindow object. Define error codes for scenarios

 */
status_t CameraHal::releaseTapoutLocked(struct preview_stream_ops *tapout)
{
    status_t ret = NO_ERROR;
    char id[OP_STR_SIZE];

    LOG_FUNCTION_NAME;

    if (!tapout) {
        CAMHAL_LOGD("Missing argument");
        LOG_FUNCTION_NAME_EXIT;
        return NO_ERROR;
    }

    // Get the name of tapout
    ret = mExtendedPreviewStreamOps->get_id(tapout, id, sizeof(id));
    if (NO_ERROR != ret) {
        CAMHAL_LOGEB("get_id OPS returned error %d", ret);
        return ret;
    }

    // 1. Check name of tap-out
    // 2. If exist, then free buffers and then remove it
    if (mBufferSourceAdapter_Out.get() && mBufferSourceAdapter_Out->match(id)) {
        CAMHAL_LOGD("REMOVE tap out %p previously set as current", tapout);
        mBufferSourceAdapter_Out.clear();
    }
    for (unsigned int i = 0; i < mOutAdapters.size(); i++) {
        android::sp<DisplayAdapter> out;
        out = mOutAdapters.itemAt(i);
        if (out->match(id)) {
            CAMHAL_LOGD("REMOVE tap out %p \"%s\" at position %d", tapout, id, i);
            mOutAdapters.removeAt(i);
            break;
        }
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

/**
   @brief Sets Tapin Surfaces.

   Buffers provided to CameraHal via this object for tap-in
   functionality.

   @param[in] window The ANativeWindow object created by Surface flinger
   @return NO_ERROR If the ANativeWindow object passes validation criteria
   @todo Define validation criteria for ANativeWindow object. Define error codes for scenarios

 */
status_t CameraHal::setTapinLocked(struct preview_stream_ops *tapin)
{
    status_t ret = NO_ERROR;
    int index = -1;

    LOG_FUNCTION_NAME;

    if (!tapin) {
        CAMHAL_LOGD("Missing argument");
        LOG_FUNCTION_NAME_EXIT;
        return NO_ERROR;
    }

    // 1. Set tapin point
    // 1. Check name of tap-in
    // 2. If not already set, then create a new one
    // 3. Allocate buffers. If user is re-setting the surface, free buffers first and re-allocate
    //    in case dimensions have changed
    for (unsigned int i = 0; i < mInAdapters.size(); i++) {
        android::sp<DisplayAdapter> in;
        in = mInAdapters.itemAt(i);
        ret = in->setPreviewWindow(tapin);
        if (ret == ALREADY_EXISTS) {
            CAMHAL_LOGD("Tap In already set at index = %d", i);
            index = i;
            ret = NO_ERROR;
        }
    }

    if (index < 0) {
        android::sp<DisplayAdapter> in  = new BufferSourceAdapter();

        ret = in->initialize();
        if (ret != NO_ERROR) {
            in.clear();
            CAMHAL_LOGEA("DisplayAdapter initialize failed");
            goto exit;
        }

        // BufferSourceAdapter will be handler of the extended OPS
        in->setExtendedOps(mExtendedPreviewStreamOps);

        // CameraAdapter will be the frame provider for BufferSourceAdapter
        in->setFrameProvider(mCameraAdapter);

        // BufferSourceAdapter will use ErrorHandler to send errors back to
        // the application
        in->setErrorHandler(mAppCallbackNotifier.get());

        // Update the display adapter with the new window that is passed from CameraService
        ret  = in->setPreviewWindow(tapin);
        if(ret != NO_ERROR) {
            CAMHAL_LOGEB("DisplayAdapter setPreviewWindow returned error %d", ret);
            goto exit;
        }

        mInAdapters.add(in);
    }

exit:

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}


/**
   @brief Releases Tapin Surfaces.

   @param[in] window The ANativeWindow object created by Surface flinger
   @return NO_ERROR If the ANativeWindow object passes validation criteria
   @todo Define validation criteria for ANativeWindow object. Define error codes for scenarios

 */
status_t CameraHal::releaseTapinLocked(struct preview_stream_ops *tapin)
{
    status_t ret = NO_ERROR;
    char id[OP_STR_SIZE];

    LOG_FUNCTION_NAME;

    if (!tapin) {
        CAMHAL_LOGD("Missing argument");
        LOG_FUNCTION_NAME_EXIT;
        return NO_ERROR;
    }

    // Get the name of tapin
    ret = mExtendedPreviewStreamOps->get_id(tapin, id, sizeof(id));
    if (NO_ERROR != ret) {
        CAMHAL_LOGEB("get_id OPS returned error %d", ret);
        return ret;
    }

    // 1. Check name of tap-in
    // 2. If exist, then free buffers and then remove it
    if (mBufferSourceAdapter_In.get() && mBufferSourceAdapter_In->match(id)) {
        CAMHAL_LOGD("REMOVE tap in %p previously set as current", tapin);
        mBufferSourceAdapter_In.clear();
    }
    for (unsigned int i = 0; i < mInAdapters.size(); i++) {
        android::sp<DisplayAdapter> in;
        in = mInAdapters.itemAt(i);
        if (in->match(id)) {
            CAMHAL_LOGD("REMOVE tap in %p \"%s\" at position %d", tapin, id, i);
            mInAdapters.removeAt(i);
            break;
        }
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}


/**
   @brief Sets ANativeWindow object.

   Buffers provided to CameraHal via this object for tap-in/tap-out
   functionality.

   TODO(XXX): this is just going to use preview_stream_ops for now, but we
   most likely need to extend it when we want more functionality

   @param[in] window The ANativeWindow object created by Surface flinger
   @return NO_ERROR If the ANativeWindow object passes validation criteria
   @todo Define validation criteria for ANativeWindow object. Define error codes for scenarios

 */
status_t CameraHal::setBufferSource(struct preview_stream_ops *tapin, struct preview_stream_ops *tapout)
{
    status_t ret = NO_ERROR;
    int index = -1;

    LOG_FUNCTION_NAME;

    android::AutoMutex lock(mLock);

    CAMHAL_LOGD ("setBufferSource(%p, %p)", tapin, tapout);

    ret = setTapoutLocked(tapout);
    if (ret != NO_ERROR) {
        CAMHAL_LOGE("setTapoutLocked returned error 0x%x", ret);
        goto exit;
    }

    ret = setTapinLocked(tapin);
    if (ret != NO_ERROR) {
        CAMHAL_LOGE("setTapinLocked returned error 0x%x", ret);
        goto exit;
    }

exit:
    LOG_FUNCTION_NAME_EXIT;

    return ret;
}


/**
   @brief Releases ANativeWindow object.

   Release Buffers previously released with setBufferSource()

   TODO(XXX): this is just going to use preview_stream_ops for now, but we
   most likely need to extend it when we want more functionality

   @param[in] window The ANativeWindow object created by Surface flinger
   @return NO_ERROR If the ANativeWindow object passes validation criteria
   @todo Define validation criteria for ANativeWindow object. Define error codes for scenarios

 */
status_t CameraHal::releaseBufferSource(struct preview_stream_ops *tapin, struct preview_stream_ops *tapout)
{
    status_t ret = NO_ERROR;
    int index = -1;

    LOG_FUNCTION_NAME;

    android::AutoMutex lock(mLock);
    CAMHAL_LOGD ("releaseBufferSource(%p, %p)", tapin, tapout);
    if (tapout) {
        ret |= releaseTapoutLocked(tapout);
        if (ret != NO_ERROR) {
            CAMHAL_LOGE("Error %d to release tap out", ret);
        }
    }

    if (tapin) {
        ret |= releaseTapinLocked(tapin);
        if (ret != NO_ERROR) {
            CAMHAL_LOGE("Error %d to release tap in", ret);
        }
    }

exit:

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}
#endif


/**
   @brief Stop a previously started preview.

   @param none
   @return none

 */
void CameraHal::stopPreview()
{
    LOG_FUNCTION_NAME;

    if( (!previewEnabled() && !mDisplayPaused) || mRecordingEnabled)
        {
        LOG_FUNCTION_NAME_EXIT;
        return;
        }

    bool imageCaptureRunning = (mCameraAdapter->getState() & CameraAdapter::CAPTURE_STATE) &&
                                    (mCameraAdapter->getNextState() != CameraAdapter::PREVIEW_STATE);
    if(mDisplayPaused && !imageCaptureRunning)
        {
        // Display is paused, which essentially means there is no preview active.
        // Note: this is done so that when stopPreview is called by client after
        // an image capture, we do not de-initialize the camera adapter and
        // restart over again.

        return;
        }

    forceStopPreview();

    // Reset Capture-Mode to default, so that when we switch from VideoRecording
    // to ImageCapture, CAPTURE_MODE is not left to VIDEO_MODE.
    CAMHAL_LOGDA("Resetting Capture-Mode to default");
    mParameters.set(TICameraParameters::KEY_CAP_MODE, "");

    LOG_FUNCTION_NAME_EXIT;
}

/**
   @brief Returns true if preview is enabled

   @param none
   @return true If preview is running currently
         false If preview has been stopped

 */
bool CameraHal::previewEnabled()
{
    LOG_FUNCTION_NAME;

    return (mPreviewEnabled || mPreviewStartInProgress);
}

/**
   @brief Start record mode.

  When a record image is available a CAMERA_MSG_VIDEO_FRAME message is sent with
  the corresponding frame. Every record frame must be released by calling
  releaseRecordingFrame().

   @param none
   @return NO_ERROR If recording could be started without any issues
   @todo Update the header with possible error values in failure scenarios

 */
status_t CameraHal::startRecording( )
{
    int w, h;
    const char *valstr = NULL;
    bool restartPreviewRequired = false;
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;


#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS

            gettimeofday(&mStartPreview, NULL);

#endif

    if(!previewEnabled())
        {
        return NO_INIT;
        }

    // set internal recording hint in case camera adapter needs to make some
    // decisions....(will only be sent to camera adapter if camera restart is required)
    mParameters.set(TICameraParameters::KEY_RECORDING_HINT, android::CameraParameters::TRUE);

    // if application starts recording in continuous focus picture mode...
    // then we need to force default capture mode (as opposed to video mode)
    if ( ((valstr = mParameters.get(android::CameraParameters::KEY_FOCUS_MODE)) != NULL) &&
         (strcmp(valstr, android::CameraParameters::FOCUS_MODE_CONTINUOUS_PICTURE) == 0) ){
        restartPreviewRequired = resetVideoModeParameters();
    }

    // only need to check recording hint if preview restart is not already needed
    valstr = mParameters.get(android::CameraParameters::KEY_RECORDING_HINT);
    if ( !restartPreviewRequired &&
         (!valstr || (valstr && (strcmp(valstr, android::CameraParameters::TRUE) != 0))) ) {
        restartPreviewRequired = setVideoModeParameters(mParameters);
    }

    if (restartPreviewRequired) {
        {
            android::AutoMutex lock(mLock);
            mCapModeBackup = mParameters.get(TICameraParameters::KEY_CAP_MODE);
        }
        ret = restartPreview();
    }

    if ( NO_ERROR == ret )
      {
        int count = atoi(mCameraProperties->get(CameraProperties::REQUIRED_PREVIEW_BUFS));
        mParameters.getPreviewSize(&w, &h);
        CAMHAL_LOGDB("%s Video Width=%d Height=%d", __FUNCTION__, mVideoWidth, mVideoHeight);

        if ((w != mVideoWidth) && (h != mVideoHeight))
          {
            ret = allocVideoBufs(mVideoWidth, mVideoHeight, count);
            if ( NO_ERROR != ret )
              {
                CAMHAL_LOGEB("allocImageBufs returned error 0x%x", ret);
                mParameters.remove(TICameraParameters::KEY_RECORDING_HINT);
                return ret;
              }

            mAppCallbackNotifier->useVideoBuffers(true);
            mAppCallbackNotifier->setVideoRes(mVideoWidth, mVideoHeight);
            ret = mAppCallbackNotifier->initSharedVideoBuffers(mPreviewBuffers, mPreviewOffsets, mPreviewFd, mPreviewLength, count, mVideoBuffers);
          }
        else
          {
            mAppCallbackNotifier->useVideoBuffers(false);
            mAppCallbackNotifier->setVideoRes(mPreviewWidth, mPreviewHeight);
            ret = mAppCallbackNotifier->initSharedVideoBuffers(mPreviewBuffers, mPreviewOffsets, mPreviewFd, mPreviewLength, count, NULL);
          }
      }

    if ( NO_ERROR == ret )
        {
         ret = mAppCallbackNotifier->startRecording();
        }

    if ( NO_ERROR == ret )
        {
        ///Buffers for video capture (if different from preview) are expected to be allocated within CameraAdapter
         ret =  mCameraAdapter->sendCommand(CameraAdapter::CAMERA_START_VIDEO);
        }

    if ( NO_ERROR == ret )
        {
        mRecordingEnabled = true;
        }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

/**
   @brief Set the camera parameters specific to Video Recording.

   This function checks for the camera parameters which have to be set for recording.
   Video Recording needs CAPTURE_MODE to be VIDEO_MODE. This function sets it.
   This function also enables Video Recording specific functions like VSTAB & VNF.

   @param none
   @return true if preview needs to be restarted for VIDEO_MODE parameters to take effect.
   @todo Modify the policies for enabling VSTAB & VNF usecase based later.

 */
bool CameraHal::setVideoModeParameters(const android::CameraParameters& params)
{
    const char *valstr = NULL;
    const char *valstrRemote = NULL;
    bool restartPreviewRequired = false;

    LOG_FUNCTION_NAME;

    // Set CAPTURE_MODE to VIDEO_MODE, if not set already and Restart Preview
    valstr = mParameters.get(TICameraParameters::KEY_CAP_MODE);
    if ( (valstr == NULL) ||
        ( (valstr != NULL) && ( (strcmp(valstr, (const char *) TICameraParameters::VIDEO_MODE) != 0) &&
                                (strcmp(valstr, (const char *) TICameraParameters::VIDEO_MODE_HQ ) != 0) ) ) ) {
        CAMHAL_LOGDA("Set CAPTURE_MODE to VIDEO_MODE");
        mParameters.set(TICameraParameters::KEY_CAP_MODE, (const char *) TICameraParameters::VIDEO_MODE);
        restartPreviewRequired = true;
    }

    // set VSTAB. restart is required if vstab value has changed
    if ( (valstrRemote = params.get(android::CameraParameters::KEY_VIDEO_STABILIZATION)) != NULL ) {
        // make sure we support vstab
        if (strcmp(mCameraProperties->get(CameraProperties::VSTAB_SUPPORTED),
                   android::CameraParameters::TRUE) == 0) {
            valstr = mParameters.get(android::CameraParameters::KEY_VIDEO_STABILIZATION);
            // vstab value has changed
            if ((valstr != NULL) &&
                 strcmp(valstr, valstrRemote) != 0) {
                restartPreviewRequired = true;
            }
            mParameters.set(android::CameraParameters::KEY_VIDEO_STABILIZATION,
                            valstrRemote);
        }
    } else if (mParameters.get(android::CameraParameters::KEY_VIDEO_STABILIZATION)) {
        // vstab was configured but now unset
        restartPreviewRequired = true;
        mParameters.remove(android::CameraParameters::KEY_VIDEO_STABILIZATION);
    }

    // Set VNF
    if ((valstrRemote = params.get(TICameraParameters::KEY_VNF)) == NULL) {
        CAMHAL_LOGDA("Enable VNF");
        mParameters.set(TICameraParameters::KEY_VNF, android::CameraParameters::TRUE);
        restartPreviewRequired = true;
    } else {
        valstr = mParameters.get(TICameraParameters::KEY_VNF);
        if (valstr && strcmp(valstr, valstrRemote) != 0) {
            restartPreviewRequired = true;
        }
        mParameters.set(TICameraParameters::KEY_VNF, valstrRemote);
    }

#if !defined(OMAP_ENHANCEMENT) && !defined(ENHANCED_DOMX)
        // For VSTAB alone for 1080p resolution, padded width goes > 2048, which cannot be rendered by GPU.
        // In such case, there is support in Ducati for combination of VSTAB & VNF requiring padded width < 2048.
        // So we are forcefully enabling VNF, if VSTAB is enabled for 1080p resolution.
        int w, h;
        params.getPreviewSize(&w, &h);
        valstr = mParameters.get(android::CameraParameters::KEY_VIDEO_STABILIZATION);
        if (valstr && (strcmp(valstr, android::CameraParameters::TRUE) == 0) && (w == 1920)) {
            CAMHAL_LOGDA("Force Enable VNF for 1080p");
            const char *valKeyVnf = mParameters.get(TICameraParameters::KEY_VNF);
            if(!valKeyVnf || (strcmp(valKeyVnf, android::CameraParameters::TRUE) != 0)) {
                mParameters.set(TICameraParameters::KEY_VNF, android::CameraParameters::TRUE);
                restartPreviewRequired = true;
            }
        }
#endif

    LOG_FUNCTION_NAME_EXIT;

    return restartPreviewRequired;
}

/**
   @brief Reset the camera parameters specific to Video Recording.

   This function resets CAPTURE_MODE and disables Recording specific functions like VSTAB & VNF.

   @param none
   @return true if preview needs to be restarted for VIDEO_MODE parameters to take effect.

 */
bool CameraHal::resetVideoModeParameters()
{
    const char *valstr = NULL;
    bool restartPreviewRequired = false;
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    // ignore this if we are already recording
    if (mRecordingEnabled) {
        return false;
    }

    // Set CAPTURE_MODE to VIDEO_MODE, if not set already and Restart Preview
    valstr = mParameters.get(TICameraParameters::KEY_CAP_MODE);
    if ((valstr != NULL) && (strcmp(valstr, TICameraParameters::VIDEO_MODE) == 0)) {
        CAMHAL_LOGDA("Reset Capture-Mode to default");
        mParameters.set(TICameraParameters::KEY_CAP_MODE, "");
        restartPreviewRequired = true;
    }

    LOG_FUNCTION_NAME_EXIT;

    return restartPreviewRequired;
}

/**
   @brief Restart the preview with setParameter.

   This function restarts preview, for some VIDEO_MODE parameters to take effect.

   @param none
   @return NO_ERROR If recording parameters could be set without any issues

 */
status_t CameraHal::restartPreview()
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    // Retain CAPTURE_MODE before calling stopPreview(), since it is reset in stopPreview().

    forceStopPreview();

    {
        android::AutoMutex lock(mLock);
        if (!mCapModeBackup.isEmpty()) {
            mParameters.set(TICameraParameters::KEY_CAP_MODE, mCapModeBackup.string());
            mCapModeBackup = "";
        } else {
            mParameters.set(TICameraParameters::KEY_CAP_MODE, "");
        }
        mCameraAdapter->setParameters(mParameters);
    }

    ret = startPreview();

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

/**
   @brief Stop a previously started recording.

   @param none
   @return none

 */
void CameraHal::stopRecording()
{
    CameraAdapter::AdapterState currentState;

    LOG_FUNCTION_NAME;

    android::AutoMutex lock(mLock);

    if (!mRecordingEnabled )
        {
        return;
        }

    currentState = mCameraAdapter->getState();
    if (currentState == CameraAdapter::VIDEO_CAPTURE_STATE) {
        mCameraAdapter->sendCommand(CameraAdapter::CAMERA_STOP_IMAGE_CAPTURE);
    }

    mAppCallbackNotifier->stopRecording();

    mCameraAdapter->sendCommand(CameraAdapter::CAMERA_STOP_VIDEO);

    mRecordingEnabled = false;

    if ( mAppCallbackNotifier->getUesVideoBuffers() ){
      freeVideoBufs(mVideoBuffers);
      if (mVideoBuffers){
        CAMHAL_LOGVB(" FREEING mVideoBuffers %p", mVideoBuffers);
        delete [] mVideoBuffers;
      }
      mVideoBuffers = NULL;
    }

    // reset internal recording hint in case camera adapter needs to make some
    // decisions....(will only be sent to camera adapter if camera restart is required)
    mParameters.remove(TICameraParameters::KEY_RECORDING_HINT);

    LOG_FUNCTION_NAME_EXIT;
}

/**
   @brief Returns true if recording is enabled.

   @param none
   @return true If recording is currently running
         false If recording has been stopped

 */
int CameraHal::recordingEnabled()
{
    LOG_FUNCTION_NAME;

    LOG_FUNCTION_NAME_EXIT;

    return mRecordingEnabled;
}

/**
   @brief Release a record frame previously returned by CAMERA_MSG_VIDEO_FRAME.

   @param[in] mem MemoryBase pointer to the frame being released. Must be one of the buffers
               previously given by CameraHal
   @return none

 */
void CameraHal::releaseRecordingFrame(const void* mem)
{
    LOG_FUNCTION_NAME;

    //CAMHAL_LOGDB(" 0x%x", mem->pointer());

    if ( ( mRecordingEnabled ) && mem != NULL)
    {
        mAppCallbackNotifier->releaseRecordingFrame(mem);
    }

    LOG_FUNCTION_NAME_EXIT;

    return;
}

/**
   @brief Start auto focus

   This call asynchronous.
   The notification callback routine is called with CAMERA_MSG_FOCUS once when
   focusing is complete. autoFocus() will be called again if another auto focus is
   needed.

   @param none
   @return NO_ERROR
   @todo Define the error codes if the focus is not locked

 */
status_t CameraHal::autoFocus()
{
    status_t ret = NO_ERROR;

#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS

    gettimeofday(&mStartFocus, NULL);

#endif

    LOG_FUNCTION_NAME;

    android::AutoMutex lock(mLock);

    mMsgEnabled |= CAMERA_MSG_FOCUS;

    if ( NULL == mCameraAdapter )
        {
            ret = -1;
            goto EXIT;
        }

    CameraAdapter::AdapterState state;
    ret = mCameraAdapter->getState(state);
    if (ret != NO_ERROR)
        {
            goto EXIT;
        }

    if (state == CameraAdapter::AF_STATE)
        {
            CAMHAL_LOGI("Ignoring start-AF (already in progress)");
            goto EXIT;
        }

#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS

    //pass the autoFocus timestamp along with the command to camera adapter
    ret = mCameraAdapter->sendCommand(CameraAdapter::CAMERA_PERFORM_AUTOFOCUS, ( int ) &mStartFocus);

#else

    ret = mCameraAdapter->sendCommand(CameraAdapter::CAMERA_PERFORM_AUTOFOCUS);

#endif

EXIT:
    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

/**
   @brief Cancels auto-focus function.

   If the auto-focus is still in progress, this function will cancel it.
   Whether the auto-focus is in progress or not, this function will return the
   focus position to the default. If the camera does not support auto-focus, this is a no-op.


   @param none
   @return NO_ERROR If the cancel succeeded
   @todo Define error codes if cancel didnt succeed

 */
status_t CameraHal::cancelAutoFocus()
{
    LOG_FUNCTION_NAME;

    android::AutoMutex lock(mLock);
    android::CameraParameters adapterParams = mParameters;
    mMsgEnabled &= ~CAMERA_MSG_FOCUS;

    if( NULL != mCameraAdapter )
    {
        adapterParams.set(TICameraParameters::KEY_AUTO_FOCUS_LOCK, android::CameraParameters::FALSE);
        mCameraAdapter->setParameters(adapterParams);
        mCameraAdapter->sendCommand(CameraAdapter::CAMERA_CANCEL_AUTOFOCUS);
        mAppCallbackNotifier->flushEventQueue();
    }

    LOG_FUNCTION_NAME_EXIT;
    return NO_ERROR;
}

void CameraHal::setEventProvider(int32_t eventMask, MessageNotifier * eventNotifier)
{

    LOG_FUNCTION_NAME;

    if ( NULL != mEventProvider )
        {
        mEventProvider->disableEventNotification(CameraHalEvent::ALL_EVENTS);
        delete mEventProvider;
        mEventProvider = NULL;
        }

    mEventProvider = new EventProvider(eventNotifier, this, eventCallbackRelay);
    if ( NULL == mEventProvider )
        {
        CAMHAL_LOGEA("Error in creating EventProvider");
        }
    else
        {
        mEventProvider->enableEventNotification(eventMask);
        }

    LOG_FUNCTION_NAME_EXIT;
}

void CameraHal::eventCallbackRelay(CameraHalEvent* event)
{
    LOG_FUNCTION_NAME;

    CameraHal *appcbn = ( CameraHal * ) (event->mCookie);
    appcbn->eventCallback(event );

    LOG_FUNCTION_NAME_EXIT;
}

void CameraHal::eventCallback(CameraHalEvent* event)
{
    LOG_FUNCTION_NAME;

    LOG_FUNCTION_NAME_EXIT;
}

status_t CameraHal::startImageBracketing()
{
    status_t ret = NO_ERROR;
    CameraFrame frame;
    CameraAdapter::BuffersDescriptor desc;
    unsigned int max_queueable = 0;



#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS

        gettimeofday(&mStartCapture, NULL);

#endif

        LOG_FUNCTION_NAME;

        if(!previewEnabled() && !mDisplayPaused)
            {
            LOG_FUNCTION_NAME_EXIT;
            return NO_INIT;
            }

        if ( !mBracketingEnabled )
            {
            return ret;
            }

        if ( NO_ERROR == ret )
            {
            mBracketingRunning = true;
            }

        if (  (NO_ERROR == ret) && ( NULL != mCameraAdapter ) )
            {
            ret = mCameraAdapter->sendCommand(CameraAdapter::CAMERA_QUERY_BUFFER_SIZE_IMAGE_CAPTURE,
                                              ( int ) &frame,
                                              ( mBracketRangeNegative + 1 ));

            if ( NO_ERROR != ret )
                {
                CAMHAL_LOGEB("CAMERA_QUERY_BUFFER_SIZE_IMAGE_CAPTURE returned error 0x%x", ret);
                }
            }

        if ( NO_ERROR == ret )
            {
            if ( NULL != mAppCallbackNotifier.get() )
                 {
                 mAppCallbackNotifier->setBurst(true);
                 }
            }

        if ( NO_ERROR == ret )
            {
            mParameters.getPictureSize(( int * ) &frame.mWidth,
                                       ( int * ) &frame.mHeight);

            ret = allocImageBufs(frame.mWidth,
                                 frame.mHeight,
                                 frame.mLength,
                                 mParameters.getPictureFormat(),
                                 ( mBracketRangeNegative + 1 ));
            if ( NO_ERROR != ret )
              {
                CAMHAL_LOGEB("allocImageBufs returned error 0x%x", ret);
              }
            }

        if (  (NO_ERROR == ret) && ( NULL != mCameraAdapter ) )
            {

            desc.mBuffers = mImageBuffers;
            desc.mOffsets = mImageOffsets;
            desc.mFd = mImageFd;
            desc.mLength = mImageLength;
            desc.mCount = ( size_t ) ( mBracketRangeNegative + 1 );
            desc.mMaxQueueable = ( size_t ) ( mBracketRangeNegative + 1 );

            ret = mCameraAdapter->sendCommand(CameraAdapter::CAMERA_USE_BUFFERS_IMAGE_CAPTURE,
                                              ( int ) &desc);

            if ( NO_ERROR == ret )
                {

#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS

                 //pass capture timestamp along with the camera adapter command
                ret = mCameraAdapter->sendCommand(CameraAdapter::CAMERA_START_BRACKET_CAPTURE,  ( mBracketRangePositive + 1 ),  (int) &mStartCapture);

#else

                ret = mCameraAdapter->sendCommand(CameraAdapter::CAMERA_START_BRACKET_CAPTURE, ( mBracketRangePositive + 1 ));

#endif

                }
            }

        return ret;
}

status_t CameraHal::stopImageBracketing()
{
        status_t ret = NO_ERROR;

        LOG_FUNCTION_NAME;

        if( !previewEnabled() )
            {
            return NO_INIT;
            }

        mBracketingRunning = false;

        ret = mCameraAdapter->sendCommand(CameraAdapter::CAMERA_STOP_BRACKET_CAPTURE);

        LOG_FUNCTION_NAME_EXIT;

        return ret;
}

/**
   @brief Take a picture.

   @param none
   @return NO_ERROR If able to switch to image capture
   @todo Define error codes if unable to switch to image capture

 */
status_t CameraHal::takePicture(const char *params)
{
    android::AutoMutex lock(mLock);
    return __takePicture(params);
}

/**
   @brief Internal function for getting a captured image.
          shared by takePicture and reprocess.
   @param none
   @return NO_ERROR If able to switch to image capture
   @todo Define error codes if unable to switch to image capture

 */
status_t CameraHal::__takePicture(const char *params, struct timeval *captureStart)
{
    // cancel AF state if needed (before any operation and mutex lock)
    if (mCameraAdapter->getState() == CameraAdapter::AF_STATE) {
        cancelAutoFocus();
    }

    status_t ret = NO_ERROR;
    CameraFrame frame;
    CameraAdapter::BuffersDescriptor desc;
    int burst = -1;
    const char *valstr = NULL;
    unsigned int bufferCount = 1;
    unsigned int max_queueable = 0;
    unsigned int rawBufferCount = 1;
    bool isCPCamMode = false;
    android::sp<DisplayAdapter> outAdapter = 0;
    bool reuseTapout = false;

#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS

    if ( NULL == captureStart ) {
        gettimeofday(&mStartCapture, NULL);
    } else {
        memcpy(&mStartCapture, captureStart, sizeof(struct timeval));
    }

#endif

    LOG_FUNCTION_NAME;

    if(!previewEnabled() && !mDisplayPaused)
        {
        LOG_FUNCTION_NAME_EXIT;
        CAMHAL_LOGEA("Preview not started...");
        return NO_INIT;
        }

    valstr = mParameters.get(TICameraParameters::KEY_CAP_MODE);

    isCPCamMode = valstr && !strcmp(valstr, TICameraParameters::CP_CAM_MODE);

    // return error if we are already capturing
    // however, we can queue a capture when in cpcam mode
    if ( ((mCameraAdapter->getState() == CameraAdapter::CAPTURE_STATE &&
          mCameraAdapter->getNextState() != CameraAdapter::PREVIEW_STATE) ||
         (mCameraAdapter->getState() == CameraAdapter::VIDEO_CAPTURE_STATE &&
          mCameraAdapter->getNextState() != CameraAdapter::VIDEO_STATE)) &&
         !isCPCamMode) {
        CAMHAL_LOGEA("Already capturing an image...");
        return NO_INIT;
    }

    // we only support video snapshot if we are in video mode (recording hint is set)
    if ( (mCameraAdapter->getState() == CameraAdapter::VIDEO_STATE) &&
         (valstr && ( strcmp(valstr, TICameraParameters::VIDEO_MODE) &&
                      strcmp(valstr, TICameraParameters::VIDEO_MODE_HQ ) ) ) ) {
        CAMHAL_LOGEA("Trying to capture while recording without recording hint set...");
        return INVALID_OPERATION;
    }

#ifdef OMAP_ENHANCEMENT_CPCAM
    // check if camera application is using shots parameters
    // api. parameters set here override anything set using setParameters
    // TODO(XXX): Just going to use legacy TI parameters for now. Need
    // add new APIs in CameraHal to utilize android::ShotParameters later, so
    // we don't have to parse through the whole set of parameters
    // in camera adapter
    if (strlen(params) > 0) {
        android::ShotParameters shotParams;
        const char *valStr;
        const char *valExpComp, *valExpGain;
        int valNum;

        android::String8 shotParams8(params);

        shotParams.unflatten(shotParams8);
        mParameters.remove(TICameraParameters::KEY_EXP_GAIN_BRACKETING_RANGE);
        mParameters.remove(TICameraParameters::KEY_EXP_BRACKETING_RANGE);

        valExpGain = shotParams.get(android::ShotParameters::KEY_EXP_GAIN_PAIRS);
        valExpComp = shotParams.get(android::ShotParameters::KEY_EXP_COMPENSATION);
        if (NULL != valExpComp) {
            mParameters.set(TICameraParameters::KEY_EXP_BRACKETING_RANGE, valExpComp);
        } else if (NULL != valExpGain) {
            mParameters.set(TICameraParameters::KEY_EXP_GAIN_BRACKETING_RANGE, valExpGain);
        }

        valNum = shotParams.getInt(android::ShotParameters::KEY_BURST);
        if (valNum >= 0) {
            mParameters.set(TICameraParameters::KEY_BURST, valNum);
            burst = valNum;
        }

        valStr = shotParams.get(android::ShotParameters::KEY_FLUSH_CONFIG);
        if (valStr!= NULL) {
            if ( 0 == strcmp(valStr, android::ShotParameters::TRUE) ) {
                mParameters.set(TICameraParameters::KEY_FLUSH_SHOT_CONFIG_QUEUE,
                                android::CameraParameters::TRUE);
            } else if ( 0 == strcmp(valStr, android::ShotParameters::FALSE) ) {
                mParameters.set(TICameraParameters::KEY_FLUSH_SHOT_CONFIG_QUEUE,
                                android::CameraParameters::FALSE);
            }
        }

        valStr = shotParams.get(android::ShotParameters::KEY_CURRENT_TAP_OUT);
        if (valStr != NULL) {
            int index = -1;
            for (unsigned int i = 0; i < mOutAdapters.size(); i++) {
                if(mOutAdapters.itemAt(i)->match(valStr)) {
                    index = i;
                    break;
                }
            }
            if (index < 0) {
                CAMHAL_LOGE("Invalid tap out surface passed to camerahal");
                return BAD_VALUE;
            }
            CAMHAL_LOGD("Found matching out adapter at %d", index);
            outAdapter = mOutAdapters.itemAt(index);
            if ( outAdapter == mBufferSourceAdapter_Out ) {
                reuseTapout = true;
            }
        }

        mCameraAdapter->setParameters(mParameters);
    } else
#endif
    {
        // TODO(XXX): Should probably reset burst and bracketing params
        // when we remove legacy TI parameters implementation
    }

#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS

    CameraHal::PPM("Takepicture parameters set: ", &mStartCapture);

#endif

    // if we are already in the middle of a capture and using the same
    // tapout ST...then we just need setParameters and start image
    // capture to queue more shots
    if (((mCameraAdapter->getState() & CameraAdapter::CAPTURE_STATE) ==
              CameraAdapter::CAPTURE_STATE) &&
         (mCameraAdapter->getNextState() != CameraAdapter::PREVIEW_STATE) &&
         (reuseTapout) ) {
#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS
        //pass capture timestamp along with the camera adapter command
        ret = mCameraAdapter->sendCommand(CameraAdapter::CAMERA_START_IMAGE_CAPTURE,
                                          (int) &mStartCapture);
#else
        ret = mCameraAdapter->sendCommand(CameraAdapter::CAMERA_START_IMAGE_CAPTURE);
#endif
        return ret;
    }

    if ( !mBracketingRunning )
    {
         // if application didn't set burst through android::ShotParameters
         // then query from TICameraParameters
         if ((burst == -1) && (NO_ERROR == ret)) {
            burst = mParameters.getInt(TICameraParameters::KEY_BURST);
         }

         //Allocate all buffers only in burst capture case
         if ( burst > 0 ) {
             // For CPCam mode...allocate for worst case burst
             bufferCount = isCPCamMode || (burst > CameraHal::NO_BUFFERS_IMAGE_CAPTURE) ?
                               CameraHal::NO_BUFFERS_IMAGE_CAPTURE : burst;

             if (outAdapter.get()) {
                if ( reuseTapout ) {
                    bufferCount = mImageCount;
                } else {
                    bufferCount = outAdapter->getBufferCount();
                    if (bufferCount < 1) {
                        bufferCount = NO_BUFFERS_IMAGE_CAPTURE_SYSTEM_HEAP;
                    }
                }
             }

             if ( NULL != mAppCallbackNotifier.get() ) {
                 mAppCallbackNotifier->setBurst(true);
             }
         } else if ( mBracketingEnabled ) {
             bufferCount = mBracketRangeNegative + 1;
             if ( NULL != mAppCallbackNotifier.get() ) {
                 mAppCallbackNotifier->setBurst(false);
             }
         } else {
             if ( NULL != mAppCallbackNotifier.get() ) {
                 mAppCallbackNotifier->setBurst(false);
             }
         }

        // pause preview during normal image capture
        // do not pause preview if recording (video state)
        if ( (NO_ERROR == ret) && (NULL != mDisplayAdapter.get()) ) {
            if (mCameraAdapter->getState() != CameraAdapter::VIDEO_STATE) {
                mDisplayPaused = true;
                mPreviewEnabled = false;
                ret = mDisplayAdapter->pauseDisplay(mDisplayPaused);
                // since preview is paused we should stop sending preview frames too
                if(mMsgEnabled & CAMERA_MSG_PREVIEW_FRAME) {
                    mAppCallbackNotifier->disableMsgType (CAMERA_MSG_PREVIEW_FRAME);
                }
            }

#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS
            mDisplayAdapter->setSnapshotTimeRef(&mStartCapture);
#endif
        }

        // if we taking video snapshot...
        if ((NO_ERROR == ret) && (mCameraAdapter->getState() == CameraAdapter::VIDEO_STATE)) {
            // enable post view frames if not already enabled so we can internally
            // save snapshot frames for generating thumbnail
            if((mMsgEnabled & CAMERA_MSG_POSTVIEW_FRAME) == 0) {
                mAppCallbackNotifier->enableMsgType(CAMERA_MSG_POSTVIEW_FRAME);
            }
        }

        if ( (NO_ERROR == ret) && (NULL != mCameraAdapter) )
            {
            if ( NO_ERROR == ret )
                ret = mCameraAdapter->sendCommand(CameraAdapter::CAMERA_QUERY_BUFFER_SIZE_IMAGE_CAPTURE,
                                                  ( int ) &frame,
                                                  bufferCount);

            if ( NO_ERROR != ret )
                {
                CAMHAL_LOGEB("CAMERA_QUERY_BUFFER_SIZE_IMAGE_CAPTURE returned error 0x%x", ret);
                }
            }

#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS

    CameraHal::PPM("Takepicture buffer size queried: ", &mStartCapture);

#endif

        if (outAdapter.get()) {
            // Avoid locking the tapout again when reusing it
            if (!reuseTapout) {
                // Need to reset buffers if we are switching adapters since we don't know
                // the state of the new buffer list
                ret = outAdapter->maxQueueableBuffers(max_queueable);
                if (NO_ERROR != ret) {
                    CAMHAL_LOGE("Couldn't get max queuable");
                    return ret;
                }
                mImageBuffers = outAdapter->getBuffers(true);
                mImageOffsets = outAdapter->getOffsets();
                mImageFd = outAdapter->getFd();
                mImageLength = outAdapter->getSize();
                mImageCount = bufferCount;
                mBufferSourceAdapter_Out = outAdapter;
            }
        } else {
            mBufferSourceAdapter_Out.clear();
            // allocImageBufs will only allocate new buffers if mImageBuffers is NULL
            if ( NO_ERROR == ret ) {
                max_queueable = bufferCount;
                ret = allocImageBufs(frame.mAlignment / getBPP(mParameters.getPictureFormat()),
                                     frame.mHeight,
                                     frame.mLength,
                                     mParameters.getPictureFormat(),
                                     bufferCount);
                if ( NO_ERROR != ret ) {
                    CAMHAL_LOGEB("allocImageBufs returned error 0x%x", ret);
                }
            }
        }

#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS

    CameraHal::PPM("Takepicture buffers allocated: ", &mStartCapture);
    memcpy(&mImageBuffers->ppmStamp, &mStartCapture, sizeof(struct timeval));

#endif

    if (  (NO_ERROR == ret) && ( NULL != mCameraAdapter ) )
            {
            desc.mBuffers = mImageBuffers;
            desc.mOffsets = mImageOffsets;
            desc.mFd = mImageFd;
            desc.mLength = mImageLength;
            desc.mCount = ( size_t ) bufferCount;
            desc.mMaxQueueable = ( size_t ) max_queueable;

            ret = mCameraAdapter->sendCommand(CameraAdapter::CAMERA_USE_BUFFERS_IMAGE_CAPTURE,
                                              ( int ) &desc);
            }
        if (mRawCapture) {
            if ( NO_ERROR == ret ) {
                CAMHAL_LOGDB("Raw capture buffers setup - %s", mParameters.getPictureFormat());
                ret = allocRawBufs(mParameters.getInt(TICameraParameters::RAW_WIDTH),
                                   mParameters.getInt(TICameraParameters::RAW_HEIGHT),
                                   android::CameraParameters::PIXEL_FORMAT_BAYER_RGGB,
                                   rawBufferCount);

                if ( NO_ERROR != ret ) {
                    CAMHAL_LOGEB("allocRawBufs (for RAW capture) returned error 0x%x", ret);
                }
            }

            if ((NO_ERROR == ret) && ( NULL != mCameraAdapter )) {
                desc.mBuffers = mVideoBuffers;
                desc.mOffsets = mVideoOffsets;
                desc.mFd = mVideoFd;
                desc.mLength = mVideoLength;
                desc.mCount = ( size_t ) rawBufferCount;
                desc.mMaxQueueable = ( size_t ) rawBufferCount;

                ret = mCameraAdapter->sendCommand(CameraAdapter::CAMERA_USE_BUFFERS_VIDEO_CAPTURE,
                        ( int ) &desc);
            }
        }
    }

#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS

        CameraHal::PPM("Takepicture buffers registered: ", &mStartCapture);

#endif

    if ((ret == NO_ERROR) && mBufferSourceAdapter_Out.get()) {
        mBufferSourceAdapter_Out->enableDisplay(0, 0, NULL);
    }

    if ((NO_ERROR == ret) && (NULL != mCameraAdapter)) {

#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS

         //pass capture timestamp along with the camera adapter command
        ret = mCameraAdapter->sendCommand(CameraAdapter::CAMERA_START_IMAGE_CAPTURE,  (int) &mStartCapture);

        CameraHal::PPM("Takepicture capture started: ", &mStartCapture);

#else

        ret = mCameraAdapter->sendCommand(CameraAdapter::CAMERA_START_IMAGE_CAPTURE);

#endif

    }

    return ret;
}

/**
   @brief Cancel a picture that was started with takePicture.

   Calling this method when no picture is being taken is a no-op.

   @param none
   @return NO_ERROR If cancel succeeded. Cancel can succeed if image callback is not sent
   @todo Define error codes

 */
status_t CameraHal::cancelPicture( )
{
    LOG_FUNCTION_NAME;
    status_t ret = NO_ERROR;

    ret = signalEndImageCapture();
    return NO_ERROR;
}

/**
   @brief Return the camera parameters.

   @param none
   @return Currently configured camera parameters

 */
char* CameraHal::getParameters()
{
    android::String8 params_str8;
    char* params_string;
    const char * valstr = NULL;

    LOG_FUNCTION_NAME;

    if( NULL != mCameraAdapter )
    {
        mCameraAdapter->getParameters(mParameters);
    }

    if ( (valstr = mParameters.get(TICameraParameters::KEY_S3D_CAP_FRAME_LAYOUT)) != NULL ) {
        if (!strcmp(TICameraParameters::S3D_TB_FULL, valstr)) {
            mParameters.set(android::CameraParameters::KEY_SUPPORTED_PICTURE_SIZES, mParameters.get(TICameraParameters::KEY_SUPPORTED_PICTURE_TOPBOTTOM_SIZES));
        } else if (!strcmp(TICameraParameters::S3D_SS_FULL, valstr)) {
            mParameters.set(android::CameraParameters::KEY_SUPPORTED_PICTURE_SIZES, mParameters.get(TICameraParameters::KEY_SUPPORTED_PICTURE_SIDEBYSIDE_SIZES));
        } else if ((!strcmp(TICameraParameters::S3D_TB_SUBSAMPLED, valstr))
            || (!strcmp(TICameraParameters::S3D_SS_SUBSAMPLED, valstr))) {
            mParameters.set(android::CameraParameters::KEY_SUPPORTED_PICTURE_SIZES, mParameters.get(TICameraParameters::KEY_SUPPORTED_PICTURE_SUBSAMPLED_SIZES));
        }
    }

    if ( (valstr = mParameters.get(TICameraParameters::KEY_S3D_PRV_FRAME_LAYOUT)) != NULL ) {
        if (!strcmp(TICameraParameters::S3D_TB_FULL, valstr)) {
            mParameters.set(android::CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES, mParameters.get(TICameraParameters::KEY_SUPPORTED_PREVIEW_TOPBOTTOM_SIZES));
        } else if (!strcmp(TICameraParameters::S3D_SS_FULL, valstr)) {
            mParameters.set(android::CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES, mParameters.get(TICameraParameters::KEY_SUPPORTED_PREVIEW_SIDEBYSIDE_SIZES));
        } else if ((!strcmp(TICameraParameters::S3D_TB_SUBSAMPLED, valstr))
                || (!strcmp(TICameraParameters::S3D_SS_SUBSAMPLED, valstr))) {
            mParameters.set(android::CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES, mParameters.get(TICameraParameters::KEY_SUPPORTED_PREVIEW_SUBSAMPLED_SIZES));
        }
    }

    android::CameraParameters mParams = mParameters;

    // Handle RECORDING_HINT to Set/Reset Video Mode Parameters
    valstr = mParameters.get(android::CameraParameters::KEY_RECORDING_HINT);
    if(valstr != NULL)
      {
        if(strcmp(valstr, android::CameraParameters::TRUE) == 0)
          {
            //HACK FOR MMS MODE
            resetPreviewRes(&mParams);
          }
      }

    // do not send internal parameters to upper layers
    mParams.remove(TICameraParameters::KEY_RECORDING_HINT);
    mParams.remove(TICameraParameters::KEY_AUTO_FOCUS_LOCK);

    params_str8 = mParams.flatten();

    // camera service frees this string...
    params_string = (char*) malloc(sizeof(char) * (params_str8.length()+1));
    strcpy(params_string, params_str8.string());

    LOG_FUNCTION_NAME_EXIT;

    ///Return the current set of parameters

    return params_string;
}


#ifdef OMAP_ENHANCEMENT_CPCAM
/**
   @brief Starts reprocessing operation.
 */
status_t CameraHal::reprocess(const char *params)
{
    status_t ret = NO_ERROR;
    int bufferCount = 0;
    CameraAdapter::BuffersDescriptor desc;
    CameraBuffer *reprocBuffers = NULL;
    android::ShotParameters shotParams;
    const char *valStr = NULL;
    struct timeval startReprocess;

    android::AutoMutex lock(mLock);

    LOG_FUNCTION_NAME;

#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS

    gettimeofday(&startReprocess, NULL);

#endif

    // 0. Get tap in surface
    if (strlen(params) > 0) {
        android::String8 shotParams8(params);
        shotParams.unflatten(shotParams8);
    }

    valStr = shotParams.get(android::ShotParameters::KEY_CURRENT_TAP_IN);
    if (valStr != NULL) {
        int index = -1;
        for (unsigned int i = 0; i < mInAdapters.size(); i++) {
            if(mInAdapters.itemAt(i)->match(valStr)) {
                index = i;
                break;
            }
        }
        if (index < 0) {
            CAMHAL_LOGE("Invalid tap in surface passed to camerahal");
            return BAD_VALUE;
        }
        CAMHAL_LOGD("Found matching in adapter at %d", index);
        mBufferSourceAdapter_In = mInAdapters.itemAt(index);
    } else {
        CAMHAL_LOGE("No tap in surface sent with shot config!");
        return BAD_VALUE;
    }

    // 1. Get buffers
    if (mBufferSourceAdapter_In.get()) {
        reprocBuffers = mBufferSourceAdapter_In->getBufferList(&bufferCount);
    }

    if (!reprocBuffers) {
        CAMHAL_LOGE("Error: couldn't get input buffers for reprocess()");
        goto exit;
    }


#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS

    CameraHal::PPM("Got reprocess buffers: ", &startReprocess);

#endif

    // 2. Get buffer information and parse parameters
    {
        shotParams.setBurst(bufferCount);
    }

#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS

    memcpy(&reprocBuffers->ppmStamp, &startReprocess, sizeof(struct timeval));

#endif

    // 3. Give buffer to camera adapter
    desc.mBuffers = reprocBuffers;
    desc.mOffsets = 0;
    desc.mFd = 0;
    desc.mLength = 0;
    desc.mCount = (size_t) bufferCount;
    desc.mMaxQueueable = (size_t) bufferCount;

    ret = mCameraAdapter->sendCommand(CameraAdapter::CAMERA_USE_BUFFERS_REPROCESS, (int) &desc);
    if (ret != NO_ERROR) {
        CAMHAL_LOGE("Error calling camera use buffers");
        goto exit;
    }

#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS

    CameraHal::PPM("Reprocess buffers registered: ", &startReprocess);

#endif

    // 4. Start reprocessing
    ret = mBufferSourceAdapter_In->enableDisplay(0, 0, NULL);
    if (ret != NO_ERROR) {
        CAMHAL_LOGE("Error enabling tap in point");
        goto exit;
    }

    // 5. Start capturing
    ret = __takePicture(shotParams.flatten().string(), &startReprocess);

exit:
    return ret;
}

/**
   @brief Cancels current reprocessing operation

 */
status_t CameraHal::cancel_reprocess( )
{
    LOG_FUNCTION_NAME;
    status_t ret = NO_ERROR;

    ret = signalEndImageCapture();
    return NO_ERROR;
}
#endif


void CameraHal::putParameters(char *parms)
{
    free(parms);
}

/**
   @brief Send command to camera driver.

   @param none
   @return NO_ERROR If the command succeeds
   @todo Define the error codes that this function can return

 */
status_t CameraHal::sendCommand(int32_t cmd, int32_t arg1, int32_t arg2)
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    if ( ( NO_ERROR == ret ) && ( NULL == mCameraAdapter ) )
        {
        CAMHAL_LOGEA("No CameraAdapter instance");
        return -EINVAL;
        }

    ///////////////////////////////////////////////////////
    // Following commands do NOT need preview to be started
    ///////////////////////////////////////////////////////

    switch ( cmd ) {
#ifdef ANDROID_API_JB_OR_LATER
    case CAMERA_CMD_ENABLE_FOCUS_MOVE_MSG:
    {
        const bool enable = static_cast<bool>(arg1);
        android::AutoMutex lock(mLock);
        if ( enable ) {
            mMsgEnabled |= CAMERA_MSG_FOCUS_MOVE;
        } else {
            mMsgEnabled &= ~CAMERA_MSG_FOCUS_MOVE;
        }
    }
        return OK;
#endif
    }

    if ( ret == OK && !previewEnabled()
#ifdef OMAP_ENHANCEMENT_VTC
            && (cmd != CAMERA_CMD_PREVIEW_INITIALIZATION)
#endif
         ) {
        CAMHAL_LOGEA("Preview is not running");
        ret = -EINVAL;
    }

    ///////////////////////////////////////////////////////
    // Following commands NEED preview to be started
    ///////////////////////////////////////////////////////

    if ( NO_ERROR == ret )
        {
        switch(cmd)
            {
            case CAMERA_CMD_START_SMOOTH_ZOOM:

                ret = mCameraAdapter->sendCommand(CameraAdapter::CAMERA_START_SMOOTH_ZOOM, arg1);

                break;
            case CAMERA_CMD_STOP_SMOOTH_ZOOM:

                ret = mCameraAdapter->sendCommand(CameraAdapter::CAMERA_STOP_SMOOTH_ZOOM);
                break;

            case CAMERA_CMD_START_FACE_DETECTION:

                ret = mCameraAdapter->sendCommand(CameraAdapter::CAMERA_START_FD);

                break;

            case CAMERA_CMD_STOP_FACE_DETECTION:

                ret = mCameraAdapter->sendCommand(CameraAdapter::CAMERA_STOP_FD);

                break;

#ifdef OMAP_ENHANCEMENT_VTC
            case CAMERA_CMD_PREVIEW_DEINITIALIZATION:
                if(mDisplayAdapter.get() != NULL) {
                    ///Stop the buffer display first
                    mDisplayAdapter->disableDisplay();
                }

                if(mAppCallbackNotifier.get() != NULL) {
                    //Stop the callback sending
                    mAppCallbackNotifier->stop();
                    mAppCallbackNotifier->flushAndReturnFrames();
                    mAppCallbackNotifier->stopPreviewCallbacks();
                }

                ret = mCameraAdapter->sendCommand(CameraAdapter::CAMERA_DESTROY_TUNNEL);
                mTunnelSetup = false;

                break;

            case CAMERA_CMD_PREVIEW_INITIALIZATION:
                ret = cameraPreviewInitialization();

                break;
#endif

            default:
                break;
            };
        }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

/**
   @brief Release the hardware resources owned by this object.

   Note that this is *not* done in the destructor.

   @param none
   @return none

 */
void CameraHal::release()
{
    LOG_FUNCTION_NAME;
    ///@todo Investigate on how release is used by CameraService. Vaguely remember that this is called
    ///just before CameraHal object destruction
    deinitialize();
    LOG_FUNCTION_NAME_EXIT;
}


/**
   @brief Dump state of the camera hardware

   @param[in] fd    File descriptor
   @param[in] args  Arguments
   @return NO_ERROR Dump succeeded
   @todo  Error codes for dump fail

 */
status_t  CameraHal::dump(int fd) const
{
    LOG_FUNCTION_NAME;
    ///Implement this method when the h/w dump function is supported on Ducati side
    return NO_ERROR;
}

/*-------------Camera Hal Interface Method definitions ENDS here--------------------*/




/*-------------Camera Hal Internal Method definitions STARTS here--------------------*/

/**
   @brief Constructor of CameraHal

   Member variables are initialized here.  No allocations should be done here as we
   don't use c++ exceptions in the code.

 */
CameraHal::CameraHal(int cameraId)
{
    LOG_FUNCTION_NAME;

    ///Initialize all the member variables to their defaults
    mPreviewEnabled = false;
    mPreviewBuffers = NULL;
    mImageBuffers = NULL;
    mBufProvider = NULL;
    mPreviewStartInProgress = false;
    mVideoBuffers = NULL;
    mVideoBufProvider = NULL;
    mRecordingEnabled = false;
    mDisplayPaused = false;
    mSetPreviewWindowCalled = false;
    mMsgEnabled = 0;
    mAppCallbackNotifier = NULL;
    mMemoryManager = NULL;
    mCameraAdapter = NULL;
    mBracketingEnabled = false;
    mBracketingRunning = false;
    mEventProvider = NULL;
    mBracketRangePositive = 1;
    mBracketRangeNegative = 1;
    mMaxZoomSupported = 0;
    mShutterEnabled = true;
    mMeasurementEnabled = false;
    mPreviewDataBuffers = NULL;
    mCameraProperties = NULL;
    mCurrentTime = 0;
    mFalsePreview = 0;
    mImageOffsets = NULL;
    mImageLength = 0;
    mImageFd = 0;
    mImageCount = 0;
    mVideoOffsets = NULL;
    mVideoFd = 0;
    mVideoLength = 0;
    mPreviewDataOffsets = NULL;
    mPreviewDataFd = 0;
    mPreviewDataLength = 0;
    mPreviewFd = 0;
    mPreviewWidth = 0;
    mPreviewHeight = 0;
    mPreviewLength = 0;
    mPreviewOffsets = NULL;
    mPreviewRunning = 0;
    mPreviewStateOld = 0;
    mRecordingEnabled = 0;
    mRecordEnabled = 0;
    mSensorListener = NULL;
    mVideoWidth = 0;
    mVideoHeight = 0;
#ifdef OMAP_ENHANCEMENT_VTC
    mVTCUseCase = false;
    mTunnelSetup = false;
#endif
    mPreviewInitializationDone = false;

#ifdef OMAP_ENHANCEMENT_CPCAM
    mExtendedPreviewStreamOps = 0;
#endif

    //These values depends on the sensor characteristics

    mRawCapture = false;

#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS

    //Initialize the CameraHAL constructor timestamp, which is used in the
    // PPM() method as time reference if the user does not supply one.
    gettimeofday(&ppm_start, NULL);

#endif

    mCameraIndex = cameraId;

    LOG_FUNCTION_NAME_EXIT;
}

/**
   @brief Destructor of CameraHal

   This function simply calls deinitialize() to free up memory allocate during construct
   phase
 */
CameraHal::~CameraHal()
{
    LOG_FUNCTION_NAME;

    ///Call de-initialize here once more - it is the last chance for us to relinquish all the h/w and s/w resources
    deinitialize();

    if ( NULL != mEventProvider )
        {
        mEventProvider->disableEventNotification(CameraHalEvent::ALL_EVENTS);
        delete mEventProvider;
        mEventProvider = NULL;
        }

    /// Free the callback notifier
    mAppCallbackNotifier.clear();

    /// Free the display adapter
    mDisplayAdapter.clear();

    if ( NULL != mCameraAdapter ) {
        int strongCount = mCameraAdapter->getStrongCount();

        mCameraAdapter->decStrong(mCameraAdapter);

        mCameraAdapter = NULL;
    }

    freeImageBufs();
    freeRawBufs();

    /// Free the memory manager
    mMemoryManager.clear();

    LOG_FUNCTION_NAME_EXIT;
}

/**
   @brief Initialize the Camera HAL

   Creates CameraAdapter, AppCallbackNotifier, DisplayAdapter and MemoryManager

   @param None
   @return NO_ERROR - On success
         NO_MEMORY - On failure to allocate memory for any of the objects
   @remarks Camera Hal internal function

 */

status_t CameraHal::initialize(CameraProperties::Properties* properties)
{
    LOG_FUNCTION_NAME;

    int sensor_index = 0;
    const char* sensor_name = NULL;

    ///Initialize the event mask used for registering an event provider for AppCallbackNotifier
    ///Currently, registering all events as to be coming from CameraAdapter
    int32_t eventMask = CameraHalEvent::ALL_EVENTS;

    // Get my camera properties
    mCameraProperties = properties;

    if(!mCameraProperties)
    {
        goto fail_loop;
    }

    // Dump the properties of this Camera
    // will only print if DEBUG macro is defined
    mCameraProperties->dump();

    if (strcmp(CameraProperties::DEFAULT_VALUE, mCameraProperties->get(CameraProperties::CAMERA_SENSOR_INDEX)) != 0 )
        {
        sensor_index = atoi(mCameraProperties->get(CameraProperties::CAMERA_SENSOR_INDEX));
        }

    if (strcmp(CameraProperties::DEFAULT_VALUE, mCameraProperties->get(CameraProperties::CAMERA_NAME)) != 0 ) {
        sensor_name = mCameraProperties->get(CameraProperties::CAMERA_NAME);
    }
    CAMHAL_LOGDB("Sensor index= %d; Sensor name= %s", sensor_index, sensor_name);

    if (strcmp(sensor_name, V4L_CAMERA_NAME_USB) == 0) {
#ifdef V4L_CAMERA_ADAPTER
        mCameraAdapter = V4LCameraAdapter_Factory(sensor_index);
#endif
    }
    else {
#ifdef OMX_CAMERA_ADAPTER
        mCameraAdapter = OMXCameraAdapter_Factory(sensor_index);
#endif
    }

    if ( ( NULL == mCameraAdapter ) || (mCameraAdapter->initialize(properties)!=NO_ERROR))
        {
        CAMHAL_LOGEA("Unable to create or initialize CameraAdapter");
        mCameraAdapter = NULL;
        goto fail_loop;
        }

    mCameraAdapter->incStrong(mCameraAdapter);
    mCameraAdapter->registerImageReleaseCallback(releaseImageBuffers, (void *) this);
    mCameraAdapter->registerEndCaptureCallback(endImageCapture, (void *)this);

    if(!mAppCallbackNotifier.get())
        {
        /// Create the callback notifier
        mAppCallbackNotifier = new AppCallbackNotifier();
        if( ( NULL == mAppCallbackNotifier.get() ) || ( mAppCallbackNotifier->initialize() != NO_ERROR))
            {
            CAMHAL_LOGEA("Unable to create or initialize AppCallbackNotifier");
            goto fail_loop;
            }
        }

    if(!mMemoryManager.get())
        {
        /// Create Memory Manager
        mMemoryManager = new MemoryManager();
        if( ( NULL == mMemoryManager.get() ) || ( mMemoryManager->initialize() != NO_ERROR))
            {
            CAMHAL_LOGEA("Unable to create or initialize MemoryManager");
            goto fail_loop;
            }
        }

    ///Setup the class dependencies...

    ///AppCallbackNotifier has to know where to get the Camera frames and the events like auto focus lock etc from.
    ///CameraAdapter is the one which provides those events
    ///Set it as the frame and event providers for AppCallbackNotifier
    ///@remarks  setEventProvider API takes in a bit mask of events for registering a provider for the different events
    ///         That way, if events can come from DisplayAdapter in future, we will be able to add it as provider
    ///         for any event
    mAppCallbackNotifier->setEventProvider(eventMask, mCameraAdapter);
    mAppCallbackNotifier->setFrameProvider(mCameraAdapter);

    ///Any dynamic errors that happen during the camera use case has to be propagated back to the application
    ///via CAMERA_MSG_ERROR. AppCallbackNotifier is the class that  notifies such errors to the application
    ///Set it as the error handler for CameraAdapter
    mCameraAdapter->setErrorHandler(mAppCallbackNotifier.get());

    ///Start the callback notifier
    if(mAppCallbackNotifier->start() != NO_ERROR)
      {
        CAMHAL_LOGEA("Couldn't start AppCallbackNotifier");
        goto fail_loop;
      }

    CAMHAL_LOGDA("Started AppCallbackNotifier..");
    mAppCallbackNotifier->setMeasurements(mMeasurementEnabled);

    ///Initialize default parameters
    initDefaultParameters();


    if ( setParameters(mParameters) != NO_ERROR )
        {
        CAMHAL_LOGEA("Failed to set default parameters?!");
        }

    // register for sensor events
    mSensorListener = new SensorListener();
    if (mSensorListener.get()) {
        if (mSensorListener->initialize() == NO_ERROR) {
            mSensorListener->setCallbacks(orientation_cb, this);
            mSensorListener->enableSensor(SensorListener::SENSOR_ORIENTATION);
        } else {
            CAMHAL_LOGEA("Error initializing SensorListener. not fatal, continuing");
            mSensorListener.clear();
            mSensorListener = NULL;
        }
    }

    LOG_FUNCTION_NAME_EXIT;

    return NO_ERROR;

    fail_loop:

        ///Free up the resources because we failed somewhere up
        deinitialize();
        LOG_FUNCTION_NAME_EXIT;

        return NO_MEMORY;

}

bool CameraHal::isResolutionValid(unsigned int width, unsigned int height, const char *supportedResolutions)
{
    bool ret = false;
    status_t status = NO_ERROR;
    char tmpBuffer[MAX_PROP_VALUE_LENGTH];
    char *pos = NULL;

    LOG_FUNCTION_NAME;

    if (NULL == supportedResolutions) {
        CAMHAL_LOGEA("Invalid supported resolutions string");
        goto exit;
    }

    status = snprintf(tmpBuffer, MAX_PROP_VALUE_LENGTH - 1, "%dx%d", width, height);
    if (0 > status) {
        CAMHAL_LOGEA("Error encountered while generating validation string");
        goto exit;
    }

    ret = isParameterValid(tmpBuffer, supportedResolutions);

exit:
    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

bool CameraHal::isFpsRangeValid(int fpsMin, int fpsMax, const char *supportedFpsRanges)
{
    bool ret = false;
    char supported[MAX_PROP_VALUE_LENGTH];
    char *pos;
    int suppFpsRangeArray[2];
    int i = 0;

    LOG_FUNCTION_NAME;

    if ( NULL == supportedFpsRanges ) {
        CAMHAL_LOGEA("Invalid supported FPS ranges string");
        return false;
    }

    if (fpsMin <= 0 || fpsMax <= 0 || fpsMin > fpsMax) {
        return false;
    }

    strncpy(supported, supportedFpsRanges, MAX_PROP_VALUE_LENGTH);
    pos = strtok(supported, " (,)");
    while (pos != NULL) {
        suppFpsRangeArray[i] = atoi(pos);
        if (i++) {
            if (fpsMin >= suppFpsRangeArray[0] && fpsMax <= suppFpsRangeArray[1]) {
                ret = true;
                break;
            }
            i = 0;
        }
        pos = strtok(NULL, " (,)");
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

bool CameraHal::isParameterValid(const char *param, const char *supportedParams)
{
    bool ret = false;
    char *pos;
    char supported[MAX_PROP_VALUE_LENGTH];

    LOG_FUNCTION_NAME;

    if (NULL == supportedParams) {
        CAMHAL_LOGEA("Invalid supported parameters string");
        goto exit;
    }

    if (NULL == param) {
        CAMHAL_LOGEA("Invalid parameter string");
        goto exit;
    }

    strncpy(supported, supportedParams, MAX_PROP_VALUE_LENGTH - 1);

    pos = strtok(supported, ",");
    while (pos != NULL) {
        if (!strcmp(pos, param)) {
            ret = true;
            break;
        }
        pos = strtok(NULL, ",");
    }

exit:
    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

bool CameraHal::isParameterValid(int param, const char *supportedParams)
{
    bool ret = false;
    status_t status;
    char tmpBuffer[MAX_PROP_VALUE_LENGTH];

    LOG_FUNCTION_NAME;

    if (NULL == supportedParams) {
        CAMHAL_LOGEA("Invalid supported parameters string");
        goto exit;
    }

    status = snprintf(tmpBuffer, MAX_PROP_VALUE_LENGTH - 1, "%d", param);
    if (0 > status) {
        CAMHAL_LOGEA("Error encountered while generating validation string");
        goto exit;
    }

    ret = isParameterValid(tmpBuffer, supportedParams);

exit:
    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t CameraHal::doesSetParameterNeedUpdate(const char* new_param, const char* old_param, bool& update) {
    if (!new_param || !old_param) {
        return -EINVAL;
    }

    // if params mismatch we should update parameters for camera adapter
    if ((strcmp(new_param, old_param) != 0)) {
       update = true;
    }

   return NO_ERROR;
}

status_t CameraHal::parseResolution(const char *resStr, int &width, int &height)
{
    status_t ret = NO_ERROR;
    char *ctx, *pWidth, *pHeight;
    const char *sep = "x";

    LOG_FUNCTION_NAME;

    if ( NULL == resStr )
        {
        return -EINVAL;
        }

    //This fixes "Invalid input resolution"
    char *resStr_copy = (char *)malloc(strlen(resStr) + 1);
    if ( NULL != resStr_copy )
        {
        strcpy(resStr_copy, resStr);
        pWidth = strtok_r(resStr_copy, sep, &ctx);

        if ( NULL != pWidth )
            {
            width = atoi(pWidth);
            }
        else
            {
            CAMHAL_LOGEB("Invalid input resolution %s", resStr);
            ret = -EINVAL;
            }
        }

    if ( NO_ERROR == ret )
        {
        pHeight = strtok_r(NULL, sep, &ctx);

        if ( NULL != pHeight )
            {
            height = atoi(pHeight);
            }
        else
            {
            CAMHAL_LOGEB("Invalid input resolution %s", resStr);
            ret = -EINVAL;
            }
        }

    free(resStr_copy);
    resStr_copy = NULL;

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

void CameraHal::insertSupportedParams()
{
    LOG_FUNCTION_NAME;

    android::CameraParameters &p = mParameters;

    ///Set the name of the camera
    p.set(TICameraParameters::KEY_CAMERA_NAME, mCameraProperties->get(CameraProperties::CAMERA_NAME));

    mMaxZoomSupported = atoi(mCameraProperties->get(CameraProperties::SUPPORTED_ZOOM_STAGES));

    p.set(android::CameraParameters::KEY_SUPPORTED_PICTURE_SIZES, mCameraProperties->get(CameraProperties::SUPPORTED_PICTURE_SIZES));
    p.set(android::CameraParameters::KEY_SUPPORTED_PICTURE_FORMATS, mCameraProperties->get(CameraProperties::SUPPORTED_PICTURE_FORMATS));
    p.set(android::CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES, mCameraProperties->get(CameraProperties::SUPPORTED_PREVIEW_SIZES));
    p.set(android::CameraParameters::KEY_SUPPORTED_PREVIEW_FORMATS, mCameraProperties->get(CameraProperties::SUPPORTED_PREVIEW_FORMATS));
    p.set(TICameraParameters::KEY_SUPPORTED_PICTURE_SUBSAMPLED_SIZES, mCameraProperties->get(CameraProperties::SUPPORTED_PICTURE_SUBSAMPLED_SIZES));
    p.set(TICameraParameters::KEY_SUPPORTED_PICTURE_SIDEBYSIDE_SIZES, mCameraProperties->get(CameraProperties::SUPPORTED_PICTURE_SIDEBYSIDE_SIZES));
    p.set(TICameraParameters::KEY_SUPPORTED_PICTURE_TOPBOTTOM_SIZES, mCameraProperties->get(CameraProperties::SUPPORTED_PICTURE_TOPBOTTOM_SIZES));
    p.set(TICameraParameters::KEY_SUPPORTED_PREVIEW_SUBSAMPLED_SIZES, mCameraProperties->get(CameraProperties::SUPPORTED_PREVIEW_SUBSAMPLED_SIZES));
    p.set(TICameraParameters::KEY_SUPPORTED_PREVIEW_SIDEBYSIDE_SIZES, mCameraProperties->get(CameraProperties::SUPPORTED_PREVIEW_SIDEBYSIDE_SIZES));
    p.set(TICameraParameters::KEY_SUPPORTED_PREVIEW_TOPBOTTOM_SIZES, mCameraProperties->get(CameraProperties::SUPPORTED_PREVIEW_TOPBOTTOM_SIZES));
    p.set(android::CameraParameters::KEY_SUPPORTED_PREVIEW_FRAME_RATES, mCameraProperties->get(CameraProperties::SUPPORTED_PREVIEW_FRAME_RATES));
    p.set(TICameraParameters::KEY_FRAMERATES_EXT_SUPPORTED, mCameraProperties->get(CameraProperties::SUPPORTED_PREVIEW_FRAME_RATES_EXT));
    p.set(android::CameraParameters::KEY_SUPPORTED_PREVIEW_FPS_RANGE, mCameraProperties->get(CameraProperties::FRAMERATE_RANGE_SUPPORTED));
    p.set(TICameraParameters::KEY_FRAMERATE_RANGES_EXT_SUPPORTED, mCameraProperties->get(CameraProperties::FRAMERATE_RANGE_EXT_SUPPORTED));
    p.set(android::CameraParameters::KEY_SUPPORTED_JPEG_THUMBNAIL_SIZES, mCameraProperties->get(CameraProperties::SUPPORTED_THUMBNAIL_SIZES));
    p.set(android::CameraParameters::KEY_SUPPORTED_WHITE_BALANCE, mCameraProperties->get(CameraProperties::SUPPORTED_WHITE_BALANCE));
    p.set(android::CameraParameters::KEY_SUPPORTED_EFFECTS, mCameraProperties->get(CameraProperties::SUPPORTED_EFFECTS));
    p.set(android::CameraParameters::KEY_SUPPORTED_SCENE_MODES, mCameraProperties->get(CameraProperties::SUPPORTED_SCENE_MODES));
    p.set(android::CameraParameters::KEY_SUPPORTED_FLASH_MODES, mCameraProperties->get(CameraProperties::SUPPORTED_FLASH_MODES));
    p.set(android::CameraParameters::KEY_SUPPORTED_FOCUS_MODES, mCameraProperties->get(CameraProperties::SUPPORTED_FOCUS_MODES));
    p.set(android::CameraParameters::KEY_SUPPORTED_ANTIBANDING, mCameraProperties->get(CameraProperties::SUPPORTED_ANTIBANDING));
    p.set(android::CameraParameters::KEY_MAX_EXPOSURE_COMPENSATION, mCameraProperties->get(CameraProperties::SUPPORTED_EV_MAX));
    p.set(android::CameraParameters::KEY_MIN_EXPOSURE_COMPENSATION, mCameraProperties->get(CameraProperties::SUPPORTED_EV_MIN));
    p.set(android::CameraParameters::KEY_EXPOSURE_COMPENSATION_STEP, mCameraProperties->get(CameraProperties::SUPPORTED_EV_STEP));
    p.set(TICameraParameters::KEY_SUPPORTED_EXPOSURE, mCameraProperties->get(CameraProperties::SUPPORTED_EXPOSURE_MODES));
    p.set(TICameraParameters::KEY_SUPPORTED_MANUAL_EXPOSURE_MIN, mCameraProperties->get(CameraProperties::SUPPORTED_MANUAL_EXPOSURE_MIN));
    p.set(TICameraParameters::KEY_SUPPORTED_MANUAL_EXPOSURE_MAX, mCameraProperties->get(CameraProperties::SUPPORTED_MANUAL_EXPOSURE_MAX));
    p.set(TICameraParameters::KEY_SUPPORTED_MANUAL_EXPOSURE_STEP, mCameraProperties->get(CameraProperties::SUPPORTED_MANUAL_EXPOSURE_STEP));
    p.set(TICameraParameters::KEY_SUPPORTED_MANUAL_GAIN_ISO_MIN, mCameraProperties->get(CameraProperties::SUPPORTED_MANUAL_GAIN_ISO_MIN));
    p.set(TICameraParameters::KEY_SUPPORTED_MANUAL_GAIN_ISO_MAX, mCameraProperties->get(CameraProperties::SUPPORTED_MANUAL_GAIN_ISO_MAX));
    p.set(TICameraParameters::KEY_SUPPORTED_MANUAL_GAIN_ISO_STEP, mCameraProperties->get(CameraProperties::SUPPORTED_MANUAL_GAIN_ISO_STEP));
    p.set(TICameraParameters::KEY_SUPPORTED_ISO_VALUES, mCameraProperties->get(CameraProperties::SUPPORTED_ISO_VALUES));
    p.set(android::CameraParameters::KEY_ZOOM_RATIOS, mCameraProperties->get(CameraProperties::SUPPORTED_ZOOM_RATIOS));
    p.set(android::CameraParameters::KEY_MAX_ZOOM, mCameraProperties->get(CameraProperties::SUPPORTED_ZOOM_STAGES));
    p.set(android::CameraParameters::KEY_ZOOM_SUPPORTED, mCameraProperties->get(CameraProperties::ZOOM_SUPPORTED));
    p.set(android::CameraParameters::KEY_SMOOTH_ZOOM_SUPPORTED, mCameraProperties->get(CameraProperties::SMOOTH_ZOOM_SUPPORTED));
    p.set(TICameraParameters::KEY_SUPPORTED_IPP, mCameraProperties->get(CameraProperties::SUPPORTED_IPP_MODES));
    p.set(TICameraParameters::KEY_S3D_PRV_FRAME_LAYOUT_VALUES, mCameraProperties->get(CameraProperties::S3D_PRV_FRAME_LAYOUT_VALUES));
    p.set(TICameraParameters::KEY_S3D_CAP_FRAME_LAYOUT_VALUES, mCameraProperties->get(CameraProperties::S3D_CAP_FRAME_LAYOUT_VALUES));
    p.set(TICameraParameters::KEY_AUTOCONVERGENCE_MODE_VALUES, mCameraProperties->get(CameraProperties::AUTOCONVERGENCE_MODE_VALUES));
    p.set(TICameraParameters::KEY_SUPPORTED_MANUAL_CONVERGENCE_MIN, mCameraProperties->get(CameraProperties::SUPPORTED_MANUAL_CONVERGENCE_MIN));
    p.set(TICameraParameters::KEY_SUPPORTED_MANUAL_CONVERGENCE_MAX, mCameraProperties->get(CameraProperties::SUPPORTED_MANUAL_CONVERGENCE_MAX));
    p.set(TICameraParameters::KEY_SUPPORTED_MANUAL_CONVERGENCE_STEP, mCameraProperties->get(CameraProperties::SUPPORTED_MANUAL_CONVERGENCE_STEP));
    p.set(android::CameraParameters::KEY_VIDEO_STABILIZATION_SUPPORTED, mCameraProperties->get(CameraProperties::VSTAB_SUPPORTED));
    p.set(TICameraParameters::KEY_VNF_SUPPORTED, mCameraProperties->get(CameraProperties::VNF_SUPPORTED));
    p.set(android::CameraParameters::KEY_AUTO_EXPOSURE_LOCK_SUPPORTED, mCameraProperties->get(CameraProperties::AUTO_EXPOSURE_LOCK_SUPPORTED));
    p.set(android::CameraParameters::KEY_AUTO_WHITEBALANCE_LOCK_SUPPORTED, mCameraProperties->get(CameraProperties::AUTO_WHITEBALANCE_LOCK_SUPPORTED));
    p.set(android::CameraParameters::KEY_VIDEO_SNAPSHOT_SUPPORTED, mCameraProperties->get(CameraProperties::VIDEO_SNAPSHOT_SUPPORTED));
    p.set(TICameraParameters::KEY_MECHANICAL_MISALIGNMENT_CORRECTION_SUPPORTED, mCameraProperties->get(CameraProperties::MECHANICAL_MISALIGNMENT_CORRECTION_SUPPORTED));
    p.set(TICameraParameters::KEY_CAP_MODE_VALUES, mCameraProperties->get(CameraProperties::CAP_MODE_VALUES));

    LOG_FUNCTION_NAME_EXIT;

}

void CameraHal::initDefaultParameters()
{
    //Purpose of this function is to initialize the default current and supported parameters for the currently
    //selected camera.

    android::CameraParameters &p = mParameters;
    int currentRevision, adapterRevision;
    status_t ret = NO_ERROR;
    int width, height;
    const char *valstr;

    LOG_FUNCTION_NAME;

    insertSupportedParams();

    ret = parseResolution(mCameraProperties->get(CameraProperties::PREVIEW_SIZE), width, height);

    if ( NO_ERROR == ret )
        {
        p.setPreviewSize(width, height);
        }
    else
        {
        p.setPreviewSize(MIN_WIDTH, MIN_HEIGHT);
        }

    ret = parseResolution(mCameraProperties->get(CameraProperties::PICTURE_SIZE), width, height);

    if ( NO_ERROR == ret )
        {
        p.setPictureSize(width, height);
        }
    else
        {
        p.setPictureSize(PICTURE_WIDTH, PICTURE_HEIGHT);
        }

    ret = parseResolution(mCameraProperties->get(CameraProperties::JPEG_THUMBNAIL_SIZE), width, height);

    if ( NO_ERROR == ret )
        {
        p.set(android::CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH, width);
        p.set(android::CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT, height);
        }
    else
        {
        p.set(android::CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH, MIN_WIDTH);
        p.set(android::CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT, MIN_HEIGHT);
        }

    //Insert default values
    p.setPreviewFrameRate(atoi(mCameraProperties->get(CameraProperties::PREVIEW_FRAME_RATE)));
    p.set(android::CameraParameters::KEY_PREVIEW_FPS_RANGE, mCameraProperties->get(CameraProperties::FRAMERATE_RANGE));
    p.setPreviewFormat(mCameraProperties->get(CameraProperties::PREVIEW_FORMAT));
    p.setPictureFormat(mCameraProperties->get(CameraProperties::PICTURE_FORMAT));
    p.set(android::CameraParameters::KEY_JPEG_QUALITY, mCameraProperties->get(CameraProperties::JPEG_QUALITY));
    p.set(android::CameraParameters::KEY_WHITE_BALANCE, mCameraProperties->get(CameraProperties::WHITEBALANCE));
    p.set(android::CameraParameters::KEY_EFFECT,  mCameraProperties->get(CameraProperties::EFFECT));
    p.set(android::CameraParameters::KEY_ANTIBANDING, mCameraProperties->get(CameraProperties::ANTIBANDING));
    p.set(android::CameraParameters::KEY_FLASH_MODE, mCameraProperties->get(CameraProperties::FLASH_MODE));
    p.set(android::CameraParameters::KEY_FOCUS_MODE, mCameraProperties->get(CameraProperties::FOCUS_MODE));
    p.set(android::CameraParameters::KEY_EXPOSURE_COMPENSATION, mCameraProperties->get(CameraProperties::EV_COMPENSATION));
    p.set(android::CameraParameters::KEY_SCENE_MODE, mCameraProperties->get(CameraProperties::SCENE_MODE));
    p.set(android::CameraParameters::KEY_ZOOM, mCameraProperties->get(CameraProperties::ZOOM));
    p.set(TICameraParameters::KEY_CONTRAST, mCameraProperties->get(CameraProperties::CONTRAST));
    p.set(TICameraParameters::KEY_SATURATION, mCameraProperties->get(CameraProperties::SATURATION));
    p.set(TICameraParameters::KEY_BRIGHTNESS, mCameraProperties->get(CameraProperties::BRIGHTNESS));
    p.set(TICameraParameters::KEY_SHARPNESS, mCameraProperties->get(CameraProperties::SHARPNESS));
    p.set(TICameraParameters::KEY_EXPOSURE_MODE, mCameraProperties->get(CameraProperties::EXPOSURE_MODE));
    p.set(TICameraParameters::KEY_MANUAL_EXPOSURE, mCameraProperties->get(CameraProperties::SUPPORTED_MANUAL_EXPOSURE_MIN));
    p.set(TICameraParameters::KEY_MANUAL_EXPOSURE_RIGHT, mCameraProperties->get(CameraProperties::SUPPORTED_MANUAL_EXPOSURE_MIN));
    p.set(TICameraParameters::KEY_MANUAL_GAIN_ISO, mCameraProperties->get(CameraProperties::SUPPORTED_MANUAL_GAIN_ISO_MIN));
    p.set(TICameraParameters::KEY_MANUAL_GAIN_ISO_RIGHT, mCameraProperties->get(CameraProperties::SUPPORTED_MANUAL_GAIN_ISO_MIN));
    p.set(TICameraParameters::KEY_ISO, mCameraProperties->get(CameraProperties::ISO_MODE));
    p.set(TICameraParameters::KEY_IPP, mCameraProperties->get(CameraProperties::IPP));
    p.set(TICameraParameters::KEY_GBCE, mCameraProperties->get(CameraProperties::GBCE));
    p.set(TICameraParameters::KEY_GBCE_SUPPORTED, mCameraProperties->get(CameraProperties::SUPPORTED_GBCE));
    p.set(TICameraParameters::KEY_GLBCE, mCameraProperties->get(CameraProperties::GLBCE));
    p.set(TICameraParameters::KEY_GLBCE_SUPPORTED, mCameraProperties->get(CameraProperties::SUPPORTED_GLBCE));
    p.set(TICameraParameters::KEY_S3D_PRV_FRAME_LAYOUT, mCameraProperties->get(CameraProperties::S3D_PRV_FRAME_LAYOUT));
    p.set(TICameraParameters::KEY_S3D_CAP_FRAME_LAYOUT, mCameraProperties->get(CameraProperties::S3D_CAP_FRAME_LAYOUT));
    p.set(TICameraParameters::KEY_AUTOCONVERGENCE_MODE, mCameraProperties->get(CameraProperties::AUTOCONVERGENCE_MODE));
    p.set(TICameraParameters::KEY_MANUAL_CONVERGENCE, mCameraProperties->get(CameraProperties::MANUAL_CONVERGENCE));
    p.set(android::CameraParameters::KEY_VIDEO_STABILIZATION, mCameraProperties->get(CameraProperties::VSTAB));
    p.set(TICameraParameters::KEY_VNF, mCameraProperties->get(CameraProperties::VNF));
    p.set(android::CameraParameters::KEY_FOCAL_LENGTH, mCameraProperties->get(CameraProperties::FOCAL_LENGTH));
    p.set(android::CameraParameters::KEY_HORIZONTAL_VIEW_ANGLE, mCameraProperties->get(CameraProperties::HOR_ANGLE));
    p.set(android::CameraParameters::KEY_VERTICAL_VIEW_ANGLE, mCameraProperties->get(CameraProperties::VER_ANGLE));
    p.set(TICameraParameters::KEY_SENSOR_ORIENTATION, mCameraProperties->get(CameraProperties::SENSOR_ORIENTATION));
    p.set(TICameraParameters::KEY_EXIF_MAKE, mCameraProperties->get(CameraProperties::EXIF_MAKE));
    p.set(TICameraParameters::KEY_EXIF_MODEL, mCameraProperties->get(CameraProperties::EXIF_MODEL));
    p.set(android::CameraParameters::KEY_JPEG_THUMBNAIL_QUALITY, mCameraProperties->get(CameraProperties::JPEG_THUMBNAIL_QUALITY));
    p.set(android::CameraParameters::KEY_VIDEO_FRAME_FORMAT, "OMX_TI_COLOR_FormatYUV420PackedSemiPlanar");
    p.set(android::CameraParameters::KEY_MAX_NUM_DETECTED_FACES_HW, mCameraProperties->get(CameraProperties::MAX_FD_HW_FACES));
    p.set(android::CameraParameters::KEY_MAX_NUM_DETECTED_FACES_SW, mCameraProperties->get(CameraProperties::MAX_FD_SW_FACES));
    p.set(TICameraParameters::KEY_MECHANICAL_MISALIGNMENT_CORRECTION, mCameraProperties->get(CameraProperties::MECHANICAL_MISALIGNMENT_CORRECTION));
    // Only one area a.k.a Touch AF for now.
    // TODO: Add support for multiple focus areas.
    p.set(android::CameraParameters::KEY_MAX_NUM_FOCUS_AREAS, mCameraProperties->get(CameraProperties::MAX_FOCUS_AREAS));
    p.set(android::CameraParameters::KEY_AUTO_EXPOSURE_LOCK, mCameraProperties->get(CameraProperties::AUTO_EXPOSURE_LOCK));
    p.set(android::CameraParameters::KEY_AUTO_WHITEBALANCE_LOCK, mCameraProperties->get(CameraProperties::AUTO_WHITEBALANCE_LOCK));
    p.set(android::CameraParameters::KEY_MAX_NUM_METERING_AREAS, mCameraProperties->get(CameraProperties::MAX_NUM_METERING_AREAS));
    p.set(TICameraParameters::RAW_WIDTH, mCameraProperties->get(CameraProperties::RAW_WIDTH));
    p.set(TICameraParameters::RAW_HEIGHT,mCameraProperties->get(CameraProperties::RAW_HEIGHT));

    // TI extensions for enable/disable algos
    // Hadcoded for now
    p.set(TICameraParameters::KEY_ALGO_EXTERNAL_GAMMA, android::CameraParameters::FALSE);
    p.set(TICameraParameters::KEY_ALGO_NSF1, android::CameraParameters::TRUE);
    p.set(TICameraParameters::KEY_ALGO_NSF2, android::CameraParameters::TRUE);
    p.set(TICameraParameters::KEY_ALGO_SHARPENING, android::CameraParameters::TRUE);
    p.set(TICameraParameters::KEY_ALGO_THREELINCOLORMAP, android::CameraParameters::TRUE);
    p.set(TICameraParameters::KEY_ALGO_GIC, android::CameraParameters::TRUE);

    LOG_FUNCTION_NAME_EXIT;
}

/**
   @brief Stop a previously started preview.
   @param none
   @return none

 */
void CameraHal::forceStopPreview()
{
    LOG_FUNCTION_NAME;

    // stop bracketing if it is running
    if ( mBracketingRunning ) {
        stopImageBracketing();
    }

    if(mDisplayAdapter.get() != NULL) {
        ///Stop the buffer display first
        mDisplayAdapter->disableDisplay();
    }

    if(mAppCallbackNotifier.get() != NULL) {
        //Stop the callback sending
        mAppCallbackNotifier->stop();
        mAppCallbackNotifier->flushAndReturnFrames();
        mAppCallbackNotifier->stopPreviewCallbacks();
    }

    if ( NULL != mCameraAdapter ) {
        // only need to send these control commands to state machine if we are
        // passed the LOADED_PREVIEW_STATE
        if (mCameraAdapter->getState() > CameraAdapter::LOADED_PREVIEW_STATE) {
           // according to javadoc...FD should be stopped in stopPreview
           // and application needs to call startFaceDection again
           // to restart FD
           mCameraAdapter->sendCommand(CameraAdapter::CAMERA_STOP_FD);
        }

        mCameraAdapter->rollbackToInitializedState();

    }

    freePreviewBufs();
    freePreviewDataBufs();

    mPreviewEnabled = false;
    mDisplayPaused = false;
    mPreviewStartInProgress = false;

    LOG_FUNCTION_NAME_EXIT;
}

/**
   @brief Deallocates memory for all the resources held by Camera HAL.

   Frees the following objects- CameraAdapter, AppCallbackNotifier, DisplayAdapter,
   and Memory Manager

   @param none
   @return none

 */
void CameraHal::deinitialize()
{
    LOG_FUNCTION_NAME;

    if ( mPreviewEnabled || mDisplayPaused ) {
        forceStopPreview();
    }

    mSetPreviewWindowCalled = false;

    if (mSensorListener.get()) {
        mSensorListener->disableSensor(SensorListener::SENSOR_ORIENTATION);
        mSensorListener.clear();
        mSensorListener = NULL;
    }

    mBufferSourceAdapter_Out.clear();
    mBufferSourceAdapter_In.clear();
    mOutAdapters.clear();
    mInAdapters.clear();

    LOG_FUNCTION_NAME_EXIT;

}

status_t CameraHal::storeMetaDataInBuffers(bool enable)
{
    LOG_FUNCTION_NAME;

    return mAppCallbackNotifier->useMetaDataBufferMode(enable);

    LOG_FUNCTION_NAME_EXIT;
}

void CameraHal::getPreferredPreviewRes(int *width, int *height)
{
    LOG_FUNCTION_NAME;

    // We request Ducati for a higher resolution so preview looks better and then downscale the frame before the callback.
    // TODO: This should be moved to configuration constants and boolean flag whether to provide such optimization
    // Also consider providing this configurability of the desired display resolution from the application
    if ( ( *width == 320 ) && ( *height == 240 ) ) {
        *width = 640;
        *height = 480;
    } else if ( ( *width == 176 ) && ( *height == 144 ) ) {
        *width = 704;
        *height = 576;
    }

    LOG_FUNCTION_NAME_EXIT;
}

void CameraHal::resetPreviewRes(android::CameraParameters *params)
{
  LOG_FUNCTION_NAME;

  if ( (mVideoWidth <= 320) && (mVideoHeight <= 240)){
    params->setPreviewSize(mVideoWidth, mVideoHeight);
  }

  LOG_FUNCTION_NAME_EXIT;
}

void *
camera_buffer_get_omx_ptr (CameraBuffer *buffer)
{
    CAMHAL_LOGV("buffer_type %d opaque %p", buffer->type, buffer->opaque);

    if (buffer->type == CAMERA_BUFFER_ANW) {
        buffer_handle_t *handle = (buffer_handle_t *)buffer->opaque;
        CAMHAL_LOGV("anw %08x", *handle);
        return (void *)*handle;
    } else if (buffer->type == CAMERA_BUFFER_ION) {
        return (void *)buffer->fd;
    } else {
        CAMHAL_LOGV("other %08x", buffer->opaque);
        return (void *)buffer->opaque;
    }
}

} // namespace Camera
} // namespace Ti
