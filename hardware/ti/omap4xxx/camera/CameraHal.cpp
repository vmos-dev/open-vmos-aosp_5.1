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

#define LOG_TAG "CameraHAL"

#include "CameraHal.h"
#include "ANativeWindowDisplayAdapter.h"
#include "TICameraParameters.h"
#include "CameraProperties.h"
#include <cutils/properties.h>

#include <poll.h>
#include <math.h>

namespace android {

extern "C" CameraAdapter* CameraAdapter_Factory(size_t);

/*****************************************************************************/

////Constant definitions and declarations
////@todo Have a CameraProperties class to store these parameters as constants for every camera
////       Currently, they are hard-coded

const int CameraHal::NO_BUFFERS_PREVIEW = MAX_CAMERA_BUFFERS;
const int CameraHal::NO_BUFFERS_IMAGE_CAPTURE = 2;

const uint32_t MessageNotifier::EVENT_BIT_FIELD_POSITION = 0;
const uint32_t MessageNotifier::FRAME_BIT_FIELD_POSITION = 0;

/******************************************************************************/

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
    msgType &= ~(CAMERA_MSG_FOCUS | CAMERA_MSG_FOCUS_MOVE);

    {
    Mutex::Autolock lock(mLock);
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
        Mutex::Autolock lock(mLock);
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
    LOG_FUNCTION_NAME;
    Mutex::Autolock lock(mLock);
    LOG_FUNCTION_NAME_EXIT;
    return (mMsgEnabled & msgType);
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

    CameraParameters params;

    String8 str_params(parameters);
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
int CameraHal::setParameters(const CameraParameters& params)
{

   LOG_FUNCTION_NAME;

    int w, h;
    int w_orig, h_orig;
    int framerate,minframerate;
    int maxFPS, minFPS;
    const char *valstr = NULL;
    int varint = 0;
    status_t ret = NO_ERROR;
    CameraParameters oldParams = mParameters;
    // Needed for KEY_RECORDING_HINT
    bool restartPreviewRequired = false;
    bool updateRequired = false;
    bool videoMode = false;

    {
        Mutex::Autolock lock(mLock);

        ///Ensure that preview is not enabled when the below parameters are changed.
        if(!previewEnabled())
            {

            CAMHAL_LOGDB("PreviewFormat %s", params.getPreviewFormat());

            if ((valstr = params.getPreviewFormat()) != NULL) {
                if ( isParameterValid(valstr, mCameraProperties->get(CameraProperties::SUPPORTED_PREVIEW_FORMATS))) {
                    mParameters.setPreviewFormat(valstr);
                } else {
                    CAMHAL_LOGEB("Invalid preview format.Supported: %s",  mCameraProperties->get(CameraProperties::SUPPORTED_PREVIEW_FORMATS));
                    return BAD_VALUE;
                }
            }

            varint = params.getInt(TICameraParameters::KEY_VNF);
            valstr = params.get(TICameraParameters::KEY_VNF);
            if ( valstr != NULL ) {
                if ( ( varint == 0 ) || ( varint == 1 ) ) {
                    CAMHAL_LOGDB("VNF set %s", valstr);
                    mParameters.set(TICameraParameters::KEY_VNF, varint);
                } else {
                    CAMHAL_LOGEB("ERROR: Invalid VNF: %s", valstr);
                    return BAD_VALUE;
                }
            }

            if ((valstr = params.get(CameraParameters::KEY_VIDEO_STABILIZATION)) != NULL) {
                // make sure we support vstab...if we don't and application is trying to set
                // vstab then return an error
                if (strcmp(mCameraProperties->get(CameraProperties::VSTAB_SUPPORTED),
                           CameraParameters::TRUE) == 0) {
                    CAMHAL_LOGDB("VSTAB %s",valstr);
                    mParameters.set(CameraParameters::KEY_VIDEO_STABILIZATION, valstr);
                } else if (strcmp(valstr, CameraParameters::TRUE) == 0) {
                    CAMHAL_LOGEB("ERROR: Invalid VSTAB: %s", valstr);
                    return BAD_VALUE;
                } else {
                    mParameters.set(CameraParameters::KEY_VIDEO_STABILIZATION,
                                    CameraParameters::FALSE);
                }
            }



            if( (valstr = params.get(TICameraParameters::KEY_CAP_MODE)) != NULL)
                {
                CAMHAL_LOGDB("Capture mode set %s", valstr);
                mParameters.set(TICameraParameters::KEY_CAP_MODE, valstr);
                }

            if ((valstr = params.get(TICameraParameters::KEY_IPP)) != NULL) {
                if (isParameterValid(valstr,mCameraProperties->get(CameraProperties::SUPPORTED_IPP_MODES))) {
                    CAMHAL_LOGDB("IPP mode set %s", valstr);
                    mParameters.set(TICameraParameters::KEY_IPP, valstr);
                } else {
                    CAMHAL_LOGEB("ERROR: Invalid IPP mode: %s", valstr);
                    return BAD_VALUE;
                }
            }

#ifdef OMAP_ENHANCEMENT

            if((valstr = params.get(TICameraParameters::KEY_S3D2D_PREVIEW)) != NULL)
                {
                CAMHAL_LOGDB("Stereo 3D->2D Preview mode is %s", params.get(TICameraParameters::KEY_S3D2D_PREVIEW));
                mParameters.set(TICameraParameters::KEY_S3D2D_PREVIEW, valstr);
                }

            if((valstr = params.get(TICameraParameters::KEY_AUTOCONVERGENCE)) != NULL)
                {
                CAMHAL_LOGDB("AutoConvergence mode is %s", params.get(TICameraParameters::KEY_AUTOCONVERGENCE));
                mParameters.set(TICameraParameters::KEY_AUTOCONVERGENCE, valstr);
                }
#endif

            }

            params.getPreviewSize(&w, &h);
            if (w == -1 && h == -1) {
                CAMHAL_LOGEA("Unable to get preview size");
                return BAD_VALUE;
              }

            int oldWidth, oldHeight;
            mParameters.getPreviewSize(&oldWidth, &oldHeight);

#ifdef OMAP_ENHANCEMENT

            int orientation =0;
            if((valstr = params.get(TICameraParameters::KEY_SENSOR_ORIENTATION)) != NULL)
                {
                CAMHAL_LOGDB("Sensor Orientation is set to %s", params.get(TICameraParameters::KEY_SENSOR_ORIENTATION));
                mParameters.set(TICameraParameters::KEY_SENSOR_ORIENTATION, valstr);
                orientation = params.getInt(TICameraParameters::KEY_SENSOR_ORIENTATION);
                }

            if(orientation ==90 || orientation ==270)
           {
              if ( !isResolutionValid(h,w, mCameraProperties->get(CameraProperties::SUPPORTED_PREVIEW_SIZES)))
               {
                CAMHAL_LOGEB("Invalid preview resolution %d x %d", w, h);
                return BAD_VALUE;
               }
              else
              {
                mParameters.setPreviewSize(w, h);
                mVideoWidth = w;
                mVideoHeight = h;
               }
           }
           else
           {
            if ( !isResolutionValid(w, h, mCameraProperties->get(CameraProperties::SUPPORTED_PREVIEW_SIZES)))
                {
                CAMHAL_LOGEB("Invalid preview resolution %d x %d", w, h);
                return BAD_VALUE;
                }
            else
                {
                mParameters.setPreviewSize(w, h);
                }
           }


#else

        if ( !isResolutionValid(w, h, mCameraProperties->get(CameraProperties::SUPPORTED_PREVIEW_SIZES))) {
            CAMHAL_LOGEB("Invalid preview resolution %d x %d", w, h);
            return BAD_VALUE;
        } else {
            mParameters.setPreviewSize(w, h);
        }

#endif

        if ( ( oldWidth != w ) || ( oldHeight != h ) ) {
            restartPreviewRequired |= true;
        }

        CAMHAL_LOGDB("PreviewResolution by App %d x %d", w, h);

        // Handle RECORDING_HINT to Set/Reset Video Mode Parameters
        valstr = params.get(CameraParameters::KEY_RECORDING_HINT);
        if(valstr != NULL)
            {
            if(strcmp(valstr, CameraParameters::TRUE) == 0)
                {
                CAMHAL_LOGDB("Recording Hint is set to %s", valstr);
                mParameters.set(CameraParameters::KEY_RECORDING_HINT, valstr);
                videoMode = true;
                int w, h;

                params.getPreviewSize(&w, &h);
                CAMHAL_LOGVB("%s Preview Width=%d Height=%d\n", __FUNCTION__, w, h);
                //HACK FOR MMS
                mVideoWidth = w;
                mVideoHeight = h;
                CAMHAL_LOGVB("%s Video Width=%d Height=%d\n", __FUNCTION__, mVideoWidth, mVideoHeight);

                setPreferredPreviewRes(w, h);
                mParameters.getPreviewSize(&w, &h);
                CAMHAL_LOGVB("%s Preview Width=%d Height=%d\n", __FUNCTION__, w, h);
                //Avoid restarting preview for MMS HACK
                if ((w != mVideoWidth) && (h != mVideoHeight))
                    {
                    restartPreviewRequired = false;
                    }

                restartPreviewRequired |= setVideoModeParameters(params);
                }
            else if(strcmp(valstr, CameraParameters::FALSE) == 0)
                {
                CAMHAL_LOGDB("Recording Hint is set to %s", valstr);
                mParameters.set(CameraParameters::KEY_RECORDING_HINT, valstr);
                restartPreviewRequired |= resetVideoModeParameters();
                params.getPreviewSize(&mVideoWidth, &mVideoHeight);
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
            mParameters.set(CameraParameters::KEY_RECORDING_HINT, "");
            restartPreviewRequired |= resetVideoModeParameters();
            params.getPreviewSize(&mVideoWidth, &mVideoHeight);
            }

        if ((valstr = params.get(CameraParameters::KEY_FOCUS_MODE)) != NULL) {
            if (isParameterValid(valstr, mCameraProperties->get(CameraProperties::SUPPORTED_FOCUS_MODES))) {
                CAMHAL_LOGDB("Focus mode set %s", valstr);

                // we need to take a decision on the capture mode based on whether CAF picture or
                // video is chosen so the behavior of each is consistent to the application
                if(strcmp(valstr, CameraParameters::FOCUS_MODE_CONTINUOUS_PICTURE) == 0){
                    restartPreviewRequired |= resetVideoModeParameters();
                } else if (strcmp(valstr, CameraParameters::FOCUS_MODE_CONTINUOUS_VIDEO) == 0){
                    restartPreviewRequired |= setVideoModeParameters(params);
                }

                mParameters.set(CameraParameters::KEY_FOCUS_MODE, valstr);
             } else {
                CAMHAL_LOGEB("ERROR: Invalid FOCUS mode = %s", valstr);
                return BAD_VALUE;
             }
        }

        ///Below parameters can be changed when the preview is running
        if ( (valstr = params.getPictureFormat()) != NULL ) {
            if (isParameterValid(valstr, mCameraProperties->get(CameraProperties::SUPPORTED_PICTURE_FORMATS))) {
                mParameters.setPictureFormat(valstr);
            } else {
                CAMHAL_LOGEB("ERROR: Invalid picture format: %s",valstr);
                return BAD_VALUE;
            }
        }

        params.getPictureSize(&w, &h);
        if ( isResolutionValid(w, h, mCameraProperties->get(CameraProperties::SUPPORTED_PICTURE_SIZES))) {
            mParameters.setPictureSize(w, h);
        } else {
            CAMHAL_LOGEB("ERROR: Invalid picture resolution %dx%d", w, h);
            return BAD_VALUE;
        }

        CAMHAL_LOGDB("Picture Size by App %d x %d", w, h);

#ifdef OMAP_ENHANCEMENT

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

        framerate = params.getPreviewFrameRate();
        valstr = params.get(CameraParameters::KEY_PREVIEW_FPS_RANGE);
        CAMHAL_LOGDB("FRAMERATE %d", framerate);

        CAMHAL_LOGVB("Passed FRR: %s, Supported FRR %s", valstr
                        , mCameraProperties->get(CameraProperties::FRAMERATE_RANGE_SUPPORTED));
        CAMHAL_LOGVB("Passed FR: %d, Supported FR %s", framerate
                        , mCameraProperties->get(CameraProperties::SUPPORTED_PREVIEW_FRAME_RATES));


        //Perform parameter validation
        if(!isParameterValid(valstr
                        , mCameraProperties->get(CameraProperties::FRAMERATE_RANGE_SUPPORTED))
                        || !isParameterValid(framerate,
                                      mCameraProperties->get(CameraProperties::SUPPORTED_PREVIEW_FRAME_RATES)))
        {
            CAMHAL_LOGEA("Invalid frame rate range or frame rate");
            return BAD_VALUE;
        }

        // Variable framerate ranges have higher priority over
        // deprecated constant FPS. "KEY_PREVIEW_FPS_RANGE" should
        // be cleared by the client in order for constant FPS to get
        // applied.
        if ( strcmp(valstr, mCameraProperties->get(CameraProperties::FRAMERATE_RANGE))  != 0)
          {
            // APP wants to set FPS range
            //Set framerate = MAXFPS
            CAMHAL_LOGDA("APP IS CHANGING FRAME RATE RANGE");
            params.getPreviewFpsRange(&minFPS, &maxFPS);

            if ( ( 0 > minFPS ) || ( 0 > maxFPS ) )
              {
                CAMHAL_LOGEA("ERROR: FPS Range is negative!");
                return BAD_VALUE;
              }

            framerate = maxFPS /CameraHal::VFR_SCALE;

          }
        else
          {
              if ( framerate != atoi(mCameraProperties->get(CameraProperties::PREVIEW_FRAME_RATE)) )
              {

                selectFPSRange(framerate, &minFPS, &maxFPS);
                CAMHAL_LOGDB("Select FPS Range %d %d", minFPS, maxFPS);
              }
              else
                {
                    if (videoMode) {
                        valstr = mCameraProperties->get(CameraProperties::FRAMERATE_RANGE_VIDEO);
                        CameraParameters temp;
                        temp.set(CameraParameters::KEY_PREVIEW_FPS_RANGE, valstr);
                        temp.getPreviewFpsRange(&minFPS, &maxFPS);
                    }
                    else {
                        valstr = mCameraProperties->get(CameraProperties::FRAMERATE_RANGE_IMAGE);
                        CameraParameters temp;
                        temp.set(CameraParameters::KEY_PREVIEW_FPS_RANGE, valstr);
                        temp.getPreviewFpsRange(&minFPS, &maxFPS);
                    }

                    framerate = maxFPS / CameraHal::VFR_SCALE;
                }

          }

        CAMHAL_LOGDB("FPS Range = %s", valstr);
        CAMHAL_LOGDB("DEFAULT FPS Range = %s", mCameraProperties->get(CameraProperties::FRAMERATE_RANGE));

        minFPS /= CameraHal::VFR_SCALE;
        maxFPS /= CameraHal::VFR_SCALE;

        if ( ( 0 == minFPS ) || ( 0 == maxFPS ) )
          {
            CAMHAL_LOGEA("ERROR: FPS Range is invalid!");
            return BAD_VALUE;
          }

        if ( maxFPS < minFPS )
          {
            CAMHAL_LOGEA("ERROR: Max FPS is smaller than Min FPS!");
            return BAD_VALUE;
          }
        CAMHAL_LOGDB("SET FRAMERATE %d", framerate);
        mParameters.setPreviewFrameRate(framerate);
        mParameters.set(CameraParameters::KEY_PREVIEW_FPS_RANGE, params.get(CameraParameters::KEY_PREVIEW_FPS_RANGE));

        CAMHAL_LOGDB("FPS Range [%d, %d]", minFPS, maxFPS);
        mParameters.set(TICameraParameters::KEY_MINFRAMERATE, minFPS);
        mParameters.set(TICameraParameters::KEY_MAXFRAMERATE, maxFPS);

        if( ( valstr = params.get(TICameraParameters::KEY_GBCE) ) != NULL )
            {
            CAMHAL_LOGDB("GBCE Value = %s", valstr);
            mParameters.set(TICameraParameters::KEY_GBCE, valstr);
            }

        if( ( valstr = params.get(TICameraParameters::KEY_GLBCE) ) != NULL )
            {
            CAMHAL_LOGDB("GLBCE Value = %s", valstr);
            mParameters.set(TICameraParameters::KEY_GLBCE, valstr);
            }

#ifdef OMAP_ENHANCEMENT

        ///Update the current parameter set
        if( (valstr = params.get(TICameraParameters::KEY_AUTOCONVERGENCE)) != NULL)
            {
            CAMHAL_LOGDB("AutoConvergence Mode is set = %s", params.get(TICameraParameters::KEY_AUTOCONVERGENCE));
            mParameters.set(TICameraParameters::KEY_AUTOCONVERGENCE, valstr);
            }

        if( (valstr = params.get(TICameraParameters::KEY_MANUALCONVERGENCE_VALUES)) !=NULL )
            {
            CAMHAL_LOGDB("ManualConvergence Value = %s", params.get(TICameraParameters::KEY_MANUALCONVERGENCE_VALUES));
            mParameters.set(TICameraParameters::KEY_MANUALCONVERGENCE_VALUES, valstr);
            }

        if ((valstr = params.get(TICameraParameters::KEY_EXPOSURE_MODE)) != NULL) {
            if (isParameterValid(valstr, mCameraProperties->get(CameraProperties::SUPPORTED_EXPOSURE_MODES))) {
                CAMHAL_LOGDB("Exposure set = %s", valstr);
                mParameters.set(TICameraParameters::KEY_EXPOSURE_MODE, valstr);
            } else {
                CAMHAL_LOGEB("ERROR: Invalid Exposure  = %s", valstr);
                return BAD_VALUE;
            }
        }

#endif

        if ((valstr = params.get(CameraParameters::KEY_WHITE_BALANCE)) != NULL) {
           if ( isParameterValid(valstr, mCameraProperties->get(CameraProperties::SUPPORTED_WHITE_BALANCE))) {
               CAMHAL_LOGDB("White balance set %s", valstr);
               mParameters.set(CameraParameters::KEY_WHITE_BALANCE, valstr);
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

        if ((valstr = params.get(CameraParameters::KEY_ANTIBANDING)) != NULL) {
            if (isParameterValid(valstr, mCameraProperties->get(CameraProperties::SUPPORTED_ANTIBANDING))) {
                CAMHAL_LOGDB("Antibanding set %s", valstr);
                mParameters.set(CameraParameters::KEY_ANTIBANDING, valstr);
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

        if( (valstr = params.get(CameraParameters::KEY_FOCUS_AREAS)) != NULL )
            {
            CAMHAL_LOGDB("Focus areas position set %s",valstr);
            mParameters.set(CameraParameters::KEY_FOCUS_AREAS, valstr);
            }

#ifdef OMAP_ENHANCEMENT

        if( (valstr = params.get(TICameraParameters::KEY_MEASUREMENT_ENABLE)) != NULL )
            {
            CAMHAL_LOGDB("Measurements set to %s", params.get(TICameraParameters::KEY_MEASUREMENT_ENABLE));
            mParameters.set(TICameraParameters::KEY_MEASUREMENT_ENABLE, valstr);

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

#endif

        if( (valstr = params.get(CameraParameters::KEY_EXPOSURE_COMPENSATION)) != NULL)
            {
            CAMHAL_LOGDB("Exposure compensation set %s", valstr);
            mParameters.set(CameraParameters::KEY_EXPOSURE_COMPENSATION, valstr);
            }

        if ((valstr = params.get(CameraParameters::KEY_SCENE_MODE)) != NULL) {
            if (isParameterValid(valstr, mCameraProperties->get(CameraProperties::SUPPORTED_SCENE_MODES))) {
                CAMHAL_LOGDB("Scene mode set %s", valstr);
                doesSetParameterNeedUpdate(valstr,
                                           mParameters.get(CameraParameters::KEY_SCENE_MODE),
                                           updateRequired);
                mParameters.set(CameraParameters::KEY_SCENE_MODE, valstr);
            } else {
                CAMHAL_LOGEB("ERROR: Invalid Scene mode = %s", valstr);
                return BAD_VALUE;
            }
        }

        if ((valstr = params.get(CameraParameters::KEY_FLASH_MODE)) != NULL) {
            if (isParameterValid(valstr, mCameraProperties->get(CameraProperties::SUPPORTED_FLASH_MODES))) {
                CAMHAL_LOGDB("Flash mode set %s", valstr);
                mParameters.set(CameraParameters::KEY_FLASH_MODE, valstr);
            } else {
                CAMHAL_LOGEB("ERROR: Invalid Flash mode = %s", valstr);
                return BAD_VALUE;
            }
        }

        if ((valstr = params.get(CameraParameters::KEY_EFFECT)) != NULL) {
            if (isParameterValid(valstr, mCameraProperties->get(CameraProperties::SUPPORTED_EFFECTS))) {
                CAMHAL_LOGDB("Effect set %s", valstr);
                mParameters.set(CameraParameters::KEY_EFFECT, valstr);
             } else {
                CAMHAL_LOGEB("ERROR: Invalid Effect = %s", valstr);
                return BAD_VALUE;
             }
        }

        varint = params.getInt(CameraParameters::KEY_ROTATION);
        if( varint >=0 )
            {
            CAMHAL_LOGDB("Rotation set %d", varint);
            mParameters.set(CameraParameters::KEY_ROTATION, varint);
            }

        varint = params.getInt(CameraParameters::KEY_JPEG_QUALITY);
        if( varint >= 0 )
            {
            CAMHAL_LOGDB("Jpeg quality set %d", varint);
            mParameters.set(CameraParameters::KEY_JPEG_QUALITY, varint);
            }

        varint = params.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH);
        if( varint >=0 )
            {
            CAMHAL_LOGDB("Thumbnail width set %d", varint);
            mParameters.set(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH, varint);
            }

        varint = params.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT);
        if( varint >=0 )
            {
            CAMHAL_LOGDB("Thumbnail width set %d", varint);
            mParameters.set(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT, varint);
            }

        varint = params.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_QUALITY);
        if( varint >=0 )
            {
            CAMHAL_LOGDB("Thumbnail quality set %d", varint);
            mParameters.set(CameraParameters::KEY_JPEG_THUMBNAIL_QUALITY, varint);
            }

        if( (valstr = params.get(CameraParameters::KEY_GPS_LATITUDE)) != NULL )
            {
            CAMHAL_LOGDB("GPS latitude set %s", valstr);
            mParameters.set(CameraParameters::KEY_GPS_LATITUDE, valstr);
            }else{
                mParameters.remove(CameraParameters::KEY_GPS_LATITUDE);
            }

        if( (valstr = params.get(CameraParameters::KEY_GPS_LONGITUDE)) != NULL )
            {
            CAMHAL_LOGDB("GPS longitude set %s", valstr);
            mParameters.set(CameraParameters::KEY_GPS_LONGITUDE, valstr);
            }else{
                mParameters.remove(CameraParameters::KEY_GPS_LONGITUDE);
            }

        if( (valstr = params.get(CameraParameters::KEY_GPS_ALTITUDE)) != NULL )
            {
            CAMHAL_LOGDB("GPS altitude set %s", valstr);
            mParameters.set(CameraParameters::KEY_GPS_ALTITUDE, valstr);
            }else{
                mParameters.remove(CameraParameters::KEY_GPS_ALTITUDE);
            }

        if( (valstr = params.get(CameraParameters::KEY_GPS_TIMESTAMP)) != NULL )
            {
            CAMHAL_LOGDB("GPS timestamp set %s", valstr);
            mParameters.set(CameraParameters::KEY_GPS_TIMESTAMP, valstr);
            }else{
                mParameters.remove(CameraParameters::KEY_GPS_TIMESTAMP);
            }

        if( (valstr = params.get(TICameraParameters::KEY_GPS_DATESTAMP)) != NULL )
            {
            CAMHAL_LOGDB("GPS datestamp set %s", valstr);
            mParameters.set(TICameraParameters::KEY_GPS_DATESTAMP, valstr);
            }else{
                mParameters.remove(TICameraParameters::KEY_GPS_DATESTAMP);
            }

        if( (valstr = params.get(CameraParameters::KEY_GPS_PROCESSING_METHOD)) != NULL )
            {
            CAMHAL_LOGDB("GPS processing method set %s", valstr);
            mParameters.set(CameraParameters::KEY_GPS_PROCESSING_METHOD, valstr);
            }else{
                mParameters.remove(CameraParameters::KEY_GPS_PROCESSING_METHOD);
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
            }
        else
            {
            mParameters.remove(TICameraParameters::KEY_EXP_BRACKETING_RANGE);
            }

#endif

        valstr = params.get(CameraParameters::KEY_ZOOM);
        varint = params.getInt(CameraParameters::KEY_ZOOM);
        if ( valstr != NULL ) {
            if ( ( varint >= 0 ) && ( varint <= mMaxZoomSupported ) ) {
                CAMHAL_LOGDB("Zoom set %s", valstr);
                doesSetParameterNeedUpdate(valstr,
                                           mParameters.get(CameraParameters::KEY_ZOOM),
                                           updateRequired);
                mParameters.set(CameraParameters::KEY_ZOOM, valstr);
             } else {
                CAMHAL_LOGEB("ERROR: Invalid Zoom: %s", valstr);
                return BAD_VALUE;
            }
        }

        if( (valstr = params.get(CameraParameters::KEY_AUTO_EXPOSURE_LOCK)) != NULL )
          {
            CAMHAL_LOGDB("Auto Exposure Lock set %s", valstr);
            doesSetParameterNeedUpdate(valstr,
                                       mParameters.get(CameraParameters::KEY_AUTO_EXPOSURE_LOCK),
                                       updateRequired);
            mParameters.set(CameraParameters::KEY_AUTO_EXPOSURE_LOCK, valstr);
          }

        if( (valstr = params.get(CameraParameters::KEY_AUTO_WHITEBALANCE_LOCK)) != NULL )
          {
            CAMHAL_LOGDB("Auto WhiteBalance Lock set %s", valstr);
            doesSetParameterNeedUpdate(valstr,
                                       mParameters.get(CameraParameters::KEY_AUTO_WHITEBALANCE_LOCK),
                                       updateRequired);
            mParameters.set(CameraParameters::KEY_AUTO_WHITEBALANCE_LOCK, valstr);
          }
        if( (valstr = params.get(CameraParameters::KEY_METERING_AREAS)) != NULL )
            {
            CAMHAL_LOGDB("Metering areas position set %s", valstr);
            mParameters.set(CameraParameters::KEY_METERING_AREAS, valstr);
            }

        // Only send parameters to adapter if preview is already
        // enabled or doesSetParameterNeedUpdate says so. Initial setParameters to camera adapter,
        // will be called in startPreview()
        // TODO(XXX): Need to identify other parameters that need update from camera adapter
        if ( (NULL != mCameraAdapter) && (mPreviewEnabled || updateRequired) ) {
            ret |= mCameraAdapter->setParameters(mParameters);
        }

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
            ( strcmp(valstr, TICameraParameters::BRACKET_ENABLE) == 0 ))
            {
            if ( !mBracketingEnabled )
                {
                CAMHAL_LOGDA("Enabling bracketing");
                mBracketingEnabled = true;

                //Wait for AF events to enable bracketing
                if ( NULL != mCameraAdapter )
                    {
                    setEventProvider( CameraHalEvent::ALL_EVENTS, mCameraAdapter );
                    }
                }
            else
                {
                CAMHAL_LOGDA("Bracketing already enabled");
                }
            }
        else if ( ( (valstr = params.get(TICameraParameters::KEY_TEMP_BRACKETING)) != NULL ) &&
            ( strcmp(valstr, TICameraParameters::BRACKET_DISABLE) == 0 ))
            {
            CAMHAL_LOGDA("Disabling bracketing");

            mBracketingEnabled = false;
            stopImageBracketing();

            //Remove AF events subscription
            if ( NULL != mEventProvider )
                {
                mEventProvider->disableEventNotification( CameraHalEvent::ALL_EVENTS );
                delete mEventProvider;
                mEventProvider = NULL;
                }

            }

        if( ( (valstr = params.get(TICameraParameters::KEY_SHUTTER_ENABLE)) != NULL ) &&
            ( strcmp(valstr, TICameraParameters::SHUTTER_ENABLE) == 0 ))
            {
            CAMHAL_LOGDA("Enabling shutter sound");

            mShutterEnabled = true;
            mMsgEnabled |= CAMERA_MSG_SHUTTER;
            mParameters.set(TICameraParameters::KEY_SHUTTER_ENABLE, valstr);
            }
        else if ( ( (valstr = params.get(TICameraParameters::KEY_SHUTTER_ENABLE)) != NULL ) &&
            ( strcmp(valstr, TICameraParameters::SHUTTER_DISABLE) == 0 ))
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
        CAMHAL_LOGDA("Stopping Preview");
        forceStopPreview();
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

    if(!mPreviewBufs)
    {
        ///@todo Pluralise the name of this method to allocateBuffers
        mPreviewLength = 0;
        mPreviewBufs = (int32_t *) mDisplayAdapter->allocateBuffer(width, height,
                                                                    previewFormat,
                                                                    mPreviewLength,
                                                                    buffercount);

	if (NULL == mPreviewBufs ) {
            CAMHAL_LOGEA("Couldn't allocate preview buffers");
            return NO_MEMORY;
         }

        mPreviewOffsets = (uint32_t *) mDisplayAdapter->getOffsets();
        if ( NULL == mPreviewOffsets ) {
            CAMHAL_LOGEA("Buffer mapping failed");
            return BAD_VALUE;
         }

        mPreviewFd = mDisplayAdapter->getFd();
        if ( -1 == mPreviewFd ) {
            CAMHAL_LOGEA("Invalid handle");
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

    CAMHAL_LOGDB("mPreviewBufs = 0x%x", (unsigned int)mPreviewBufs);
    if(mPreviewBufs)
        {
        ///@todo Pluralise the name of this method to freeBuffers
        ret = mBufProvider->freeBuffer(mPreviewBufs);
        mPreviewBufs = NULL;
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
        if( NULL != mPreviewDataBufs )
            {
            ret = freePreviewDataBufs();
            }
        }

    if ( NO_ERROR == ret )
        {
        bytes = ((bytes+4095)/4096)*4096;
        mPreviewDataBufs = (int32_t *)mMemoryManager->allocateBuffer(0, 0, NULL, bytes, bufferCount);

        CAMHAL_LOGDB("Size of Preview data buffer = %d", bytes);
        if( NULL == mPreviewDataBufs )
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

    LOG_FUNCTION_NAME;

    return ret;
}

status_t CameraHal::freePreviewDataBufs()
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    if ( NO_ERROR == ret )
        {

        if( NULL != mPreviewDataBufs )
            {

            ///@todo Pluralise the name of this method to freeBuffers
            ret = mMemoryManager->freeBuffer(mPreviewDataBufs);
            mPreviewDataBufs = NULL;

            }
        }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t CameraHal::allocImageBufs(unsigned int width, unsigned int height, size_t size, const char* previewFormat, unsigned int bufferCount)
{
    status_t ret = NO_ERROR;
    int bytes;

    LOG_FUNCTION_NAME;

    bytes = size;

    // allocate image buffers only if not already allocated
    if(NULL != mImageBufs) {
        return NO_ERROR;
    }

    if ( NO_ERROR == ret )
        {
        bytes = ((bytes+4095)/4096)*4096;
        mImageBufs = (int32_t *)mMemoryManager->allocateBuffer(0, 0, previewFormat, bytes, bufferCount);

        CAMHAL_LOGDB("Size of Image cap buffer = %d", bytes);
        if( NULL == mImageBufs )
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
        mImageFd = mMemoryManager->getFd();
        mImageLength = bytes;
        mImageOffsets = mMemoryManager->getOffsets();
        }
    else
        {
        mImageFd = -1;
        mImageLength = 0;
        mImageOffsets = NULL;
        }

    LOG_FUNCTION_NAME;

    return ret;
}

status_t CameraHal::allocVideoBufs(uint32_t width, uint32_t height, uint32_t bufferCount)
{
  status_t ret = NO_ERROR;
  LOG_FUNCTION_NAME;

  if( NULL != mVideoBufs ){
    ret = freeVideoBufs(mVideoBufs);
    mVideoBufs = NULL;
  }

  if ( NO_ERROR == ret ){
    int32_t stride;
    buffer_handle_t *bufsArr = new buffer_handle_t [bufferCount];

    if (bufsArr != NULL){
      for (int i = 0; i< bufferCount; i++){
        GraphicBufferAllocator &GrallocAlloc = GraphicBufferAllocator::get();
        buffer_handle_t buf;
        ret = GrallocAlloc.alloc(width, height, HAL_PIXEL_FORMAT_NV12, CAMHAL_GRALLOC_USAGE, &buf, &stride);
        if (ret != NO_ERROR){
          CAMHAL_LOGEA("Couldn't allocate video buffers using Gralloc");
          ret = -NO_MEMORY;
          for (int j=0; j< i; j++){
            buf = (buffer_handle_t)bufsArr[j];
            CAMHAL_LOGEB("Freeing Gralloc Buffer 0x%x", buf);
            GrallocAlloc.free(buf);
          }
          delete [] bufsArr;
          goto exit;
        }
        bufsArr[i] = buf;
        CAMHAL_LOGVB("*** Gralloc Handle =0x%x ***", buf);
      }

      mVideoBufs = (int32_t *)bufsArr;
    }
    else{
      CAMHAL_LOGEA("Couldn't allocate video buffers ");
      ret = -NO_MEMORY;
    }
  }

 exit:
  LOG_FUNCTION_NAME;

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
    CameraParameters adapterParams = mParameters;
    Mutex::Autolock lock(mLock);

    LOG_FUNCTION_NAME;

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

    if ( NO_ERROR == ret )
        {

        if( NULL != mImageBufs )
            {

            ///@todo Pluralise the name of this method to freeBuffers
            ret = mMemoryManager->freeBuffer(mImageBufs);
            mImageBufs = NULL;

            }
        else
            {
            ret = -EINVAL;
            }

        }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t CameraHal::freeVideoBufs(void *bufs)
{
  status_t ret = NO_ERROR;

  LOG_FUNCTION_NAME;

  buffer_handle_t *pBuf = (buffer_handle_t*)bufs;
  int count = atoi(mCameraProperties->get(CameraProperties::REQUIRED_PREVIEW_BUFS));
  if(pBuf == NULL)
    {
      CAMHAL_LOGEA("NULL pointer passed to freeVideoBuffer");
      LOG_FUNCTION_NAME_EXIT;
      return BAD_VALUE;
    }

  GraphicBufferAllocator &GrallocAlloc = GraphicBufferAllocator::get();

  for(int i = 0; i < count; i++){
    buffer_handle_t ptr = *pBuf++;
    CAMHAL_LOGVB("Free Video Gralloc Handle 0x%x", ptr);
    GrallocAlloc.free(ptr);
  }

  LOG_FUNCTION_NAME_EXIT;

  return ret;
}

/**
   @brief Start preview mode.

   @param none
   @return NO_ERROR Camera switched to VF mode
   @todo Update function header with the different errors that are possible

 */
status_t CameraHal::startPreview()
{

    status_t ret = NO_ERROR;
    CameraAdapter::BuffersDescriptor desc;
    CameraFrame frame;
    const char *valstr = NULL;
    unsigned int required_buffer_count;
    unsigned int max_queueble_buffers;

#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS
        gettimeofday(&mStartPreview, NULL);
#endif

    LOG_FUNCTION_NAME;

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
      //Update the padded width and height - required for VNF and VSTAB
      mParameters.set(TICameraParameters::KEY_PADDED_WIDTH, mPreviewWidth);
      mParameters.set(TICameraParameters::KEY_PADDED_HEIGHT, mPreviewHeight);

    }

    ///If we don't have the preview callback enabled and display adapter,
    if(!mSetPreviewWindowCalled || (mDisplayAdapter.get() == NULL)){
      CAMHAL_LOGDA("Preview not started. Preview in progress flag set");
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
            desc.mBuffers = mPreviewDataBufs;
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
    desc.mBuffers = mPreviewBufs;
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

    mAppCallbackNotifier->startPreviewCallbacks(mParameters, mPreviewBufs, mPreviewOffsets, mPreviewFd, mPreviewLength, required_buffer_count);

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

    ///Enable the display adapter if present, actual overlay enable happens when we post the buffer
    if(mDisplayAdapter.get() != NULL)
        {
        CAMHAL_LOGDA("Enabling display");
        bool isS3d = false;
        DisplayAdapter::S3DParameters s3dParams;
        int width, height;
        mParameters.getPreviewSize(&width, &height);
#if 0 //TODO: s3d is not part of bringup...will reenable
        if ( (valstr = mParameters.get(TICameraParameters::KEY_S3D_SUPPORTED)) != NULL) {
            isS3d = (strcmp(valstr, "true") == 0);
        }
        if ( (valstr = mParameters.get(TICameraParameters::KEY_S3D2D_PREVIEW)) != NULL) {
            if (strcmp(valstr, "off") == 0)
                {
                CAMHAL_LOGEA("STEREO 3D->2D PREVIEW MODE IS OFF");
                //TODO: obtain the frame packing configuration from camera or user settings
                //once side by side configuration is supported
                s3dParams.mode = OVERLAY_S3D_MODE_ON;
                s3dParams.framePacking = OVERLAY_S3D_FORMAT_OVERUNDER;
                s3dParams.order = OVERLAY_S3D_ORDER_LF;
                s3dParams.subSampling = OVERLAY_S3D_SS_NONE;
                }
            else
                {
                CAMHAL_LOGEA("STEREO 3D->2D PREVIEW MODE IS ON");
                s3dParams.mode = OVERLAY_S3D_MODE_OFF;
                s3dParams.framePacking = OVERLAY_S3D_FORMAT_OVERUNDER;
                s3dParams.order = OVERLAY_S3D_ORDER_LF;
                s3dParams.subSampling = OVERLAY_S3D_SS_NONE;
                }
        }
#endif //if 0

#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS

        ret = mDisplayAdapter->enableDisplay(width, height, &mStartPreview, isS3d ? &s3dParams : NULL);

#else

        ret = mDisplayAdapter->enableDisplay(width, height, NULL, isS3d ? &s3dParams : NULL);

#endif

        if ( ret != NO_ERROR )
            {
            CAMHAL_LOGEA("Couldn't enable display");
            goto error;
            }

        }

    ///Send START_PREVIEW command to adapter
    CAMHAL_LOGDA("Starting CameraAdapter preview mode");

    ret = mCameraAdapter->sendCommand(CameraAdapter::CAMERA_START_PREVIEW);

    if(ret!=NO_ERROR)
        {
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
            CAMHAL_LOGDA("NULL window passed, destroying display adapter");
            mDisplayAdapter.clear();
            ///@remarks If there was a window previously existing, we usually expect another valid window to be passed by the client
            ///@remarks so, we will wait until it passes a valid window to begin the preview again
            mSetPreviewWindowCalled = false;
        }
        CAMHAL_LOGDA("NULL ANativeWindow passed to setPreviewWindow");
        return NO_ERROR;
    }else if(mDisplayAdapter.get() == NULL)
    {
        // Need to create the display adapter since it has not been created
        // Create display adapter
        mDisplayAdapter = new ANativeWindowDisplayAdapter();
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

    bool imageCaptureRunning = (mCameraAdapter->getState() == CameraAdapter::CAPTURE_STATE) &&
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
    mParameters.set(TICameraParameters::KEY_RECORDING_HINT, CameraParameters::TRUE);

    // if application starts recording in continuous focus picture mode...
    // then we need to force default capture mode (as opposed to video mode)
    if ( ((valstr = mParameters.get(CameraParameters::KEY_FOCUS_MODE)) != NULL) &&
         (strcmp(valstr, CameraParameters::FOCUS_MODE_CONTINUOUS_PICTURE) == 0) ){
        restartPreviewRequired = resetVideoModeParameters();
    }

    // only need to check recording hint if preview restart is not already needed
    valstr = mParameters.get(CameraParameters::KEY_RECORDING_HINT);
    if ( !restartPreviewRequired &&
         (!valstr || (valstr && (strcmp(valstr, CameraParameters::TRUE) != 0))) ) {
        restartPreviewRequired = setVideoModeParameters(mParameters);
    }

    if (restartPreviewRequired) {
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
            ret = mAppCallbackNotifier->initSharedVideoBuffers(mPreviewBufs, mPreviewOffsets, mPreviewFd, mPreviewLength, count, mVideoBufs);
          }
        else
          {
            mAppCallbackNotifier->useVideoBuffers(false);
            mAppCallbackNotifier->setVideoRes(mPreviewWidth, mPreviewHeight);
            ret = mAppCallbackNotifier->initSharedVideoBuffers(mPreviewBufs, mPreviewOffsets, mPreviewFd, mPreviewLength, count, NULL);
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
bool CameraHal::setVideoModeParameters(const CameraParameters& params)
{
    const char *valstr = NULL;
    const char *valstrRemote = NULL;
    bool restartPreviewRequired = false;
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    // Set CAPTURE_MODE to VIDEO_MODE, if not set already and Restart Preview
    valstr = mParameters.get(TICameraParameters::KEY_CAP_MODE);
    if ( (valstr == NULL) ||
        ( (valstr != NULL) && (strcmp(valstr, (const char *) TICameraParameters::VIDEO_MODE) != 0) ) )
        {
        CAMHAL_LOGDA("Set CAPTURE_MODE to VIDEO_MODE");
        mParameters.set(TICameraParameters::KEY_CAP_MODE, (const char *) TICameraParameters::VIDEO_MODE);
        restartPreviewRequired = true;
        }

    // Check if CAPTURE_MODE is VIDEO_MODE, since VSTAB & VNF work only in VIDEO_MODE.
    valstr = mParameters.get(TICameraParameters::KEY_CAP_MODE);
    if (strcmp(valstr, (const char *) TICameraParameters::VIDEO_MODE) == 0) {
       valstrRemote = params.get(CameraParameters::KEY_VIDEO_STABILIZATION);
       // set VSTAB. restart is required if vstab value has changed
       if ( valstrRemote != NULL) {
            // make sure we support vstab
            if (strcmp(mCameraProperties->get(CameraProperties::VSTAB_SUPPORTED),
                       CameraParameters::TRUE) == 0) {
                valstr = mParameters.get(CameraParameters::KEY_VIDEO_STABILIZATION);
                // vstab value has changed
                if ((valstr != NULL) &&
                     strcmp(valstr, valstrRemote) != 0) {
                    restartPreviewRequired = true;
                }
                mParameters.set(CameraParameters::KEY_VIDEO_STABILIZATION, valstrRemote);
            }
        } else if (mParameters.get(CameraParameters::KEY_VIDEO_STABILIZATION)) {
            // vstab was configured but now unset
            restartPreviewRequired = true;
            mParameters.remove(CameraParameters::KEY_VIDEO_STABILIZATION);
        }

        // Set VNF
        valstrRemote = params.get(TICameraParameters::KEY_VNF);
        if ( valstrRemote == NULL) {
            CAMHAL_LOGDA("Enable VNF");
            mParameters.set(TICameraParameters::KEY_VNF, "1");
            restartPreviewRequired = true;
        } else {
            valstr = mParameters.get(TICameraParameters::KEY_VNF);
            if (valstr && strcmp(valstr, valstrRemote) != 0) {
                restartPreviewRequired = true;
            }
            mParameters.set(TICameraParameters::KEY_VNF, valstrRemote);
        }

        // For VSTAB alone for 1080p resolution, padded width goes > 2048, which cannot be rendered by GPU.
        // In such case, there is support in Ducati for combination of VSTAB & VNF requiring padded width < 2048.
        // So we are forcefully enabling VNF, if VSTAB is enabled for 1080p resolution.
        valstr = mParameters.get(CameraParameters::KEY_VIDEO_STABILIZATION);
        if (valstr && (strcmp(valstr, CameraParameters::TRUE) == 0) && (mPreviewWidth == 1920)) {
            CAMHAL_LOGDA("Force Enable VNF for 1080p");
            mParameters.set(TICameraParameters::KEY_VNF, "1");
            restartPreviewRequired = true;
        }
    }
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
    const char *valstr = NULL;
    char tmpvalstr[30];
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    // Retain CAPTURE_MODE before calling stopPreview(), since it is reset in stopPreview().
    tmpvalstr[0] = 0;
    valstr = mParameters.get(TICameraParameters::KEY_CAP_MODE);
    if(valstr != NULL)
        {
        if(sizeof(tmpvalstr) < (strlen(valstr)+1))
            {
            return -EINVAL;
            }

        strncpy(tmpvalstr, valstr, sizeof(tmpvalstr));
        tmpvalstr[sizeof(tmpvalstr)-1] = 0;
        }

    forceStopPreview();

    {
        Mutex::Autolock lock(mLock);
        mParameters.set(TICameraParameters::KEY_CAP_MODE, tmpvalstr);
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

    Mutex::Autolock lock(mLock);

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
      freeVideoBufs(mVideoBufs);
      if (mVideoBufs){
        CAMHAL_LOGVB(" FREEING mVideoBufs 0x%x", mVideoBufs);
        delete [] mVideoBufs;
      }
      mVideoBufs = NULL;
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

    Mutex::Autolock lock(mLock);

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

    Mutex::Autolock lock(mLock);
    CameraParameters adapterParams = mParameters;
    mMsgEnabled &= ~CAMERA_MSG_FOCUS;

    if( NULL != mCameraAdapter )
    {
        adapterParams.set(TICameraParameters::KEY_AUTO_FOCUS_LOCK, CameraParameters::FALSE);
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

    if ( NULL != event )
        {
        switch( event->mEventType )
            {
            case CameraHalEvent::EVENT_FOCUS_LOCKED:
            case CameraHalEvent::EVENT_FOCUS_ERROR:
                {
                if ( mBracketingEnabled )
                    {
                    startImageBracketing();
                    }
                break;
                }
            default:
                {
                break;
                }
            };
        }

    LOG_FUNCTION_NAME_EXIT;
}

status_t CameraHal::startImageBracketing()
{
        status_t ret = NO_ERROR;
        CameraFrame frame;
        CameraAdapter::BuffersDescriptor desc;

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

            desc.mBuffers = mImageBufs;
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
status_t CameraHal::takePicture( )
{
    status_t ret = NO_ERROR;
    CameraFrame frame;
    CameraAdapter::BuffersDescriptor desc;
    int burst;
    const char *valstr = NULL;
    unsigned int bufferCount = 1;

    Mutex::Autolock lock(mLock);

#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS

    gettimeofday(&mStartCapture, NULL);

#endif

    LOG_FUNCTION_NAME;

    if(!previewEnabled() && !mDisplayPaused)
        {
        LOG_FUNCTION_NAME_EXIT;
        CAMHAL_LOGEA("Preview not started...");
        return NO_INIT;
        }

    // return error if we are already capturing
    if ( (mCameraAdapter->getState() == CameraAdapter::CAPTURE_STATE &&
          mCameraAdapter->getNextState() != CameraAdapter::PREVIEW_STATE) ||
         (mCameraAdapter->getState() == CameraAdapter::VIDEO_CAPTURE_STATE &&
          mCameraAdapter->getNextState() != CameraAdapter::VIDEO_STATE) ) {
        CAMHAL_LOGEA("Already capturing an image...");
        return NO_INIT;
    }

    // we only support video snapshot if we are in video mode (recording hint is set)
    valstr = mParameters.get(TICameraParameters::KEY_CAP_MODE);
    if ( (mCameraAdapter->getState() == CameraAdapter::VIDEO_STATE) &&
         (valstr && strcmp(valstr, TICameraParameters::VIDEO_MODE)) ) {
        CAMHAL_LOGEA("Trying to capture while recording without recording hint set...");
        return INVALID_OPERATION;
    }

    if ( !mBracketingRunning )
        {

         if ( NO_ERROR == ret )
            {
            burst = mParameters.getInt(TICameraParameters::KEY_BURST);
            }

         //Allocate all buffers only in burst capture case
         if ( burst > 1 )
             {
             bufferCount = CameraHal::NO_BUFFERS_IMAGE_CAPTURE;
             if ( NULL != mAppCallbackNotifier.get() )
                 {
                 mAppCallbackNotifier->setBurst(true);
                 }
             }
         else
             {
             if ( NULL != mAppCallbackNotifier.get() )
                 {
                 mAppCallbackNotifier->setBurst(false);
                 }
             }

        // pause preview during normal image capture
        // do not pause preview if recording (video state)
        if (NO_ERROR == ret &&
                NULL != mDisplayAdapter.get() &&
                burst < 1) {
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

        if ( NO_ERROR == ret )
            {
            mParameters.getPictureSize(( int * ) &frame.mWidth,
                                       ( int * ) &frame.mHeight);

            ret = allocImageBufs(frame.mWidth,
                                 frame.mHeight,
                                 frame.mLength,
                                 mParameters.getPictureFormat(),
                                 bufferCount);
            if ( NO_ERROR != ret )
                {
                CAMHAL_LOGEB("allocImageBufs returned error 0x%x", ret);
                }
            }

        if (  (NO_ERROR == ret) && ( NULL != mCameraAdapter ) )
            {
            desc.mBuffers = mImageBufs;
            desc.mOffsets = mImageOffsets;
            desc.mFd = mImageFd;
            desc.mLength = mImageLength;
            desc.mCount = ( size_t ) bufferCount;
            desc.mMaxQueueable = ( size_t ) bufferCount;

            ret = mCameraAdapter->sendCommand(CameraAdapter::CAMERA_USE_BUFFERS_IMAGE_CAPTURE,
                                              ( int ) &desc);
            }
        }

    if ( ( NO_ERROR == ret ) && ( NULL != mCameraAdapter ) )
        {

#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS

         //pass capture timestamp along with the camera adapter command
        ret = mCameraAdapter->sendCommand(CameraAdapter::CAMERA_START_IMAGE_CAPTURE,  (int) &mStartCapture);

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

    Mutex::Autolock lock(mLock);

    mCameraAdapter->sendCommand(CameraAdapter::CAMERA_STOP_IMAGE_CAPTURE);

    return NO_ERROR;
}

/**
   @brief Return the camera parameters.

   @param none
   @return Currently configured camera parameters

 */
char* CameraHal::getParameters()
{
    String8 params_str8;
    char* params_string;
    const char * valstr = NULL;

    LOG_FUNCTION_NAME;

    if( NULL != mCameraAdapter )
    {
        mCameraAdapter->getParameters(mParameters);
    }

    CameraParameters mParams = mParameters;

    // Handle RECORDING_HINT to Set/Reset Video Mode Parameters
    valstr = mParameters.get(CameraParameters::KEY_RECORDING_HINT);
    if(valstr != NULL)
      {
        if(strcmp(valstr, CameraParameters::TRUE) == 0)
          {
            //HACK FOR MMS MODE
            resetPreviewRes(&mParams, mVideoWidth, mVideoHeight);
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
    switch(cmd) {
        case CAMERA_CMD_ENABLE_FOCUS_MOVE_MSG:
            bool enable = static_cast<bool>(arg1);
            Mutex::Autolock lock(mLock);
            if (enable) {
                mMsgEnabled |= CAMERA_MSG_FOCUS_MOVE;
            } else {
                mMsgEnabled &= ~CAMERA_MSG_FOCUS_MOVE;
            }
            return NO_ERROR;
        break;
    }

    if ( ( NO_ERROR == ret ) && ( !previewEnabled() ))
        {
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

            case CAMERA_CMD_START_FACE_DETECTION:

                ret = mCameraAdapter->sendCommand(CameraAdapter::CAMERA_START_FD);

                break;

            case CAMERA_CMD_STOP_FACE_DETECTION:

                ret = mCameraAdapter->sendCommand(CameraAdapter::CAMERA_STOP_FD);

                break;

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
    mPreviewBufs = NULL;
    mImageBufs = NULL;
    mBufProvider = NULL;
    mPreviewStartInProgress = false;
    mVideoBufs = NULL;
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
    mPreviewDataBufs = NULL;
    mCameraProperties = NULL;
    mCurrentTime = 0;
    mFalsePreview = 0;
    mImageOffsets = NULL;
    mImageLength = 0;
    mImageFd = 0;
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

    CAMHAL_LOGDB("Sensor index %d", sensor_index);

    mCameraAdapter = CameraAdapter_Factory(sensor_index);
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
    bool ret = true;
    status_t status = NO_ERROR;
    char tmpBuffer[PARAM_BUFFER + 1];
    char *pos = NULL;

    LOG_FUNCTION_NAME;

    if ( NULL == supportedResolutions )
        {
        CAMHAL_LOGEA("Invalid supported resolutions string");
        ret = false;
        goto exit;
        }

    status = snprintf(tmpBuffer, PARAM_BUFFER, "%dx%d", width, height);
    if ( 0 > status )
        {
        CAMHAL_LOGEA("Error encountered while generating validation string");
        ret = false;
        goto exit;
        }

    pos = strstr(supportedResolutions, tmpBuffer);
    if ( NULL == pos )
        {
        ret = false;
        }
    else
        {
        ret = true;
        }

exit:

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

bool CameraHal::isParameterValid(const char *param, const char *supportedParams)
{
    bool ret = true;
    char *pos = NULL;

    LOG_FUNCTION_NAME;

    if ( NULL == supportedParams )
        {
        CAMHAL_LOGEA("Invalid supported parameters string");
        ret = false;
        goto exit;
        }

    if ( NULL == param )
        {
        CAMHAL_LOGEA("Invalid parameter string");
        ret = false;
        goto exit;
        }

    pos = strstr(supportedParams, param);
    if ( NULL == pos )
        {
        ret = false;
        }
    else
        {
        ret = true;
        }

exit:

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

bool CameraHal::isParameterValid(int param, const char *supportedParams)
{
    bool ret = true;
    char *pos = NULL;
    status_t status;
    char tmpBuffer[PARAM_BUFFER + 1];

    LOG_FUNCTION_NAME;

    if ( NULL == supportedParams )
        {
        CAMHAL_LOGEA("Invalid supported parameters string");
        ret = false;
        goto exit;
        }

    status = snprintf(tmpBuffer, PARAM_BUFFER, "%d", param);
    if ( 0 > status )
        {
        CAMHAL_LOGEA("Error encountered while generating validation string");
        ret = false;
        goto exit;
        }

    pos = strstr(supportedParams, tmpBuffer);
    if ( NULL == pos )
        {
        ret = false;
        }
    else
        {
        ret = true;
        }

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
    char *tmp = NULL;

    LOG_FUNCTION_NAME;

    if ( NULL == resStr )
        {
        return -EINVAL;
        }

    //This fixes "Invalid input resolution"
    char *resStr_copy = (char *)malloc(strlen(resStr) + 1);
    if ( NULL!=resStr_copy ) {
    if ( NO_ERROR == ret )
        {
        strcpy(resStr_copy, resStr);
        pWidth = strtok_r( (char *) resStr_copy, sep, &ctx);

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
     }
    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

void CameraHal::insertSupportedParams()
{
    char tmpBuffer[PARAM_BUFFER + 1];

    LOG_FUNCTION_NAME;

    CameraParameters &p = mParameters;

    ///Set the name of the camera
    p.set(TICameraParameters::KEY_CAMERA_NAME, mCameraProperties->get(CameraProperties::CAMERA_NAME));

    mMaxZoomSupported = atoi(mCameraProperties->get(CameraProperties::SUPPORTED_ZOOM_STAGES));

    p.set(CameraParameters::KEY_SUPPORTED_PICTURE_SIZES, mCameraProperties->get(CameraProperties::SUPPORTED_PICTURE_SIZES));
    p.set(CameraParameters::KEY_SUPPORTED_PICTURE_FORMATS, mCameraProperties->get(CameraProperties::SUPPORTED_PICTURE_FORMATS));
    p.set(CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES, mCameraProperties->get(CameraProperties::SUPPORTED_PREVIEW_SIZES));
    p.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FORMATS, mCameraProperties->get(CameraProperties::SUPPORTED_PREVIEW_FORMATS));
    p.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FRAME_RATES, mCameraProperties->get(CameraProperties::SUPPORTED_PREVIEW_FRAME_RATES));
    p.set(CameraParameters::KEY_SUPPORTED_JPEG_THUMBNAIL_SIZES, mCameraProperties->get(CameraProperties::SUPPORTED_THUMBNAIL_SIZES));
    p.set(CameraParameters::KEY_SUPPORTED_WHITE_BALANCE, mCameraProperties->get(CameraProperties::SUPPORTED_WHITE_BALANCE));
    p.set(CameraParameters::KEY_SUPPORTED_EFFECTS, mCameraProperties->get(CameraProperties::SUPPORTED_EFFECTS));
    p.set(CameraParameters::KEY_SUPPORTED_SCENE_MODES, mCameraProperties->get(CameraProperties::SUPPORTED_SCENE_MODES));
    p.set(CameraParameters::KEY_SUPPORTED_FLASH_MODES, mCameraProperties->get(CameraProperties::SUPPORTED_FLASH_MODES));
    p.set(CameraParameters::KEY_SUPPORTED_FOCUS_MODES, mCameraProperties->get(CameraProperties::SUPPORTED_FOCUS_MODES));
    p.set(CameraParameters::KEY_SUPPORTED_ANTIBANDING, mCameraProperties->get(CameraProperties::SUPPORTED_ANTIBANDING));
    p.set(CameraParameters::KEY_MAX_EXPOSURE_COMPENSATION, mCameraProperties->get(CameraProperties::SUPPORTED_EV_MAX));
    p.set(CameraParameters::KEY_MIN_EXPOSURE_COMPENSATION, mCameraProperties->get(CameraProperties::SUPPORTED_EV_MIN));
    p.set(CameraParameters::KEY_EXPOSURE_COMPENSATION_STEP, mCameraProperties->get(CameraProperties::SUPPORTED_EV_STEP));
    p.set(CameraParameters::KEY_SUPPORTED_SCENE_MODES, mCameraProperties->get(CameraProperties::SUPPORTED_SCENE_MODES));
    p.set(TICameraParameters::KEY_SUPPORTED_EXPOSURE, mCameraProperties->get(CameraProperties::SUPPORTED_EXPOSURE_MODES));
    p.set(TICameraParameters::KEY_SUPPORTED_ISO_VALUES, mCameraProperties->get(CameraProperties::SUPPORTED_ISO_VALUES));
    p.set(CameraParameters::KEY_ZOOM_RATIOS, mCameraProperties->get(CameraProperties::SUPPORTED_ZOOM_RATIOS));
    p.set(CameraParameters::KEY_MAX_ZOOM, mCameraProperties->get(CameraProperties::SUPPORTED_ZOOM_STAGES));
    p.set(CameraParameters::KEY_ZOOM_SUPPORTED, mCameraProperties->get(CameraProperties::ZOOM_SUPPORTED));
    p.set(CameraParameters::KEY_SMOOTH_ZOOM_SUPPORTED, mCameraProperties->get(CameraProperties::SMOOTH_ZOOM_SUPPORTED));
    p.set(TICameraParameters::KEY_SUPPORTED_IPP, mCameraProperties->get(CameraProperties::SUPPORTED_IPP_MODES));
    p.set(TICameraParameters::KEY_S3D_SUPPORTED,mCameraProperties->get(CameraProperties::S3D_SUPPORTED));
    p.set(TICameraParameters::KEY_S3D2D_PREVIEW_MODE,mCameraProperties->get(CameraProperties::S3D2D_PREVIEW_MODES));
    p.set(TICameraParameters::KEY_AUTOCONVERGENCE_MODE, mCameraProperties->get(CameraProperties::AUTOCONVERGENCE_MODE));
    p.set(TICameraParameters::KEY_MANUALCONVERGENCE_VALUES, mCameraProperties->get(CameraProperties::MANUALCONVERGENCE_VALUES));
    p.set(CameraParameters::KEY_VIDEO_STABILIZATION_SUPPORTED, mCameraProperties->get(CameraProperties::VSTAB_SUPPORTED));
    p.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FPS_RANGE, mCameraProperties->get(CameraProperties::FRAMERATE_RANGE_SUPPORTED));
    p.set(TICameraParameters::KEY_SENSOR_ORIENTATION, mCameraProperties->get(CameraProperties::SENSOR_ORIENTATION));
    p.set(TICameraParameters::KEY_SENSOR_ORIENTATION_VALUES, mCameraProperties->get(CameraProperties::SENSOR_ORIENTATION_VALUES));
    p.set(CameraParameters::KEY_AUTO_EXPOSURE_LOCK_SUPPORTED, mCameraProperties->get(CameraProperties::AUTO_EXPOSURE_LOCK_SUPPORTED));
    p.set(CameraParameters::KEY_AUTO_WHITEBALANCE_LOCK_SUPPORTED, mCameraProperties->get(CameraProperties::AUTO_WHITEBALANCE_LOCK_SUPPORTED));
    p.set(CameraParameters::KEY_VIDEO_SNAPSHOT_SUPPORTED, mCameraProperties->get(CameraProperties::VIDEO_SNAPSHOT_SUPPORTED));

    LOG_FUNCTION_NAME_EXIT;

}

void CameraHal::initDefaultParameters()
{
    //Purpose of this function is to initialize the default current and supported parameters for the currently
    //selected camera.

    CameraParameters &p = mParameters;
    int currentRevision, adapterRevision;
    status_t ret = NO_ERROR;
    int width, height;

    LOG_FUNCTION_NAME;

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
        p.set(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH, width);
        p.set(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT, height);
        }
    else
        {
        p.set(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH, MIN_WIDTH);
        p.set(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT, MIN_HEIGHT);
        }

    insertSupportedParams();

    //Insert default values
    p.setPreviewFrameRate(atoi(mCameraProperties->get(CameraProperties::PREVIEW_FRAME_RATE)));
    p.setPreviewFormat(mCameraProperties->get(CameraProperties::PREVIEW_FORMAT));
    p.setPictureFormat(mCameraProperties->get(CameraProperties::PICTURE_FORMAT));
    p.set(CameraParameters::KEY_JPEG_QUALITY, mCameraProperties->get(CameraProperties::JPEG_QUALITY));
    p.set(CameraParameters::KEY_WHITE_BALANCE, mCameraProperties->get(CameraProperties::WHITEBALANCE));
    p.set(CameraParameters::KEY_EFFECT,  mCameraProperties->get(CameraProperties::EFFECT));
    p.set(CameraParameters::KEY_ANTIBANDING, mCameraProperties->get(CameraProperties::ANTIBANDING));
    p.set(CameraParameters::KEY_FLASH_MODE, mCameraProperties->get(CameraProperties::FLASH_MODE));
    p.set(CameraParameters::KEY_FOCUS_MODE, mCameraProperties->get(CameraProperties::FOCUS_MODE));
    p.set(CameraParameters::KEY_EXPOSURE_COMPENSATION, mCameraProperties->get(CameraProperties::EV_COMPENSATION));
    p.set(CameraParameters::KEY_SCENE_MODE, mCameraProperties->get(CameraProperties::SCENE_MODE));
    p.set(CameraParameters::KEY_FLASH_MODE, mCameraProperties->get(CameraProperties::FLASH_MODE));
    p.set(CameraParameters::KEY_ZOOM, mCameraProperties->get(CameraProperties::ZOOM));
    p.set(TICameraParameters::KEY_CONTRAST, mCameraProperties->get(CameraProperties::CONTRAST));
    p.set(TICameraParameters::KEY_SATURATION, mCameraProperties->get(CameraProperties::SATURATION));
    p.set(TICameraParameters::KEY_BRIGHTNESS, mCameraProperties->get(CameraProperties::BRIGHTNESS));
    p.set(TICameraParameters::KEY_SHARPNESS, mCameraProperties->get(CameraProperties::SHARPNESS));
    p.set(TICameraParameters::KEY_EXPOSURE_MODE, mCameraProperties->get(CameraProperties::EXPOSURE_MODE));
    p.set(TICameraParameters::KEY_ISO, mCameraProperties->get(CameraProperties::ISO_MODE));
    p.set(TICameraParameters::KEY_IPP, mCameraProperties->get(CameraProperties::IPP));
    p.set(TICameraParameters::KEY_GBCE, mCameraProperties->get(CameraProperties::GBCE));
    p.set(TICameraParameters::KEY_S3D2D_PREVIEW, mCameraProperties->get(CameraProperties::S3D2D_PREVIEW));
    p.set(TICameraParameters::KEY_AUTOCONVERGENCE, mCameraProperties->get(CameraProperties::AUTOCONVERGENCE));
    p.set(TICameraParameters::KEY_MANUALCONVERGENCE_VALUES, mCameraProperties->get(CameraProperties::MANUALCONVERGENCE_VALUES));
    p.set(CameraParameters::KEY_VIDEO_STABILIZATION, mCameraProperties->get(CameraProperties::VSTAB));
    p.set(CameraParameters::KEY_FOCAL_LENGTH, mCameraProperties->get(CameraProperties::FOCAL_LENGTH));
    p.set(CameraParameters::KEY_HORIZONTAL_VIEW_ANGLE, mCameraProperties->get(CameraProperties::HOR_ANGLE));
    p.set(CameraParameters::KEY_VERTICAL_VIEW_ANGLE, mCameraProperties->get(CameraProperties::VER_ANGLE));
    p.set(CameraParameters::KEY_PREVIEW_FPS_RANGE,mCameraProperties->get(CameraProperties::FRAMERATE_RANGE));
    p.set(TICameraParameters::KEY_SENSOR_ORIENTATION, mCameraProperties->get(CameraProperties::SENSOR_ORIENTATION));
    p.set(TICameraParameters::KEY_SENSOR_ORIENTATION_VALUES, mCameraProperties->get(CameraProperties::SENSOR_ORIENTATION_VALUES));
    p.set(TICameraParameters::KEY_EXIF_MAKE, mCameraProperties->get(CameraProperties::EXIF_MAKE));
    p.set(TICameraParameters::KEY_EXIF_MODEL, mCameraProperties->get(CameraProperties::EXIF_MODEL));
    p.set(CameraParameters::KEY_JPEG_THUMBNAIL_QUALITY, mCameraProperties->get(CameraProperties::JPEG_THUMBNAIL_QUALITY));
    p.set(CameraParameters::KEY_VIDEO_FRAME_FORMAT, "OMX_TI_COLOR_FormatYUV420PackedSemiPlanar");
    p.set(CameraParameters::KEY_MAX_NUM_DETECTED_FACES_HW, mCameraProperties->get(CameraProperties::MAX_FD_HW_FACES));
    p.set(CameraParameters::KEY_MAX_NUM_DETECTED_FACES_SW, mCameraProperties->get(CameraProperties::MAX_FD_SW_FACES));

    // Only one area a.k.a Touch AF for now.
    // TODO: Add support for multiple focus areas.
    p.set(CameraParameters::KEY_MAX_NUM_FOCUS_AREAS, mCameraProperties->get(CameraProperties::MAX_FOCUS_AREAS));
    p.set(CameraParameters::KEY_AUTO_EXPOSURE_LOCK, mCameraProperties->get(CameraProperties::AUTO_EXPOSURE_LOCK));
    p.set(CameraParameters::KEY_AUTO_WHITEBALANCE_LOCK, mCameraProperties->get(CameraProperties::AUTO_WHITEBALANCE_LOCK));
    p.set(CameraParameters::KEY_MAX_NUM_METERING_AREAS, mCameraProperties->get(CameraProperties::MAX_NUM_METERING_AREAS));

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
    stopImageBracketing();

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

    LOG_FUNCTION_NAME_EXIT;

}

status_t CameraHal::storeMetaDataInBuffers(bool enable)
{
    LOG_FUNCTION_NAME;

    return mAppCallbackNotifier->useMetaDataBufferMode(enable);

    LOG_FUNCTION_NAME_EXIT;
}

void CameraHal::selectFPSRange(int framerate, int *min_fps, int *max_fps)
{
  char * ptr;
  char supported[MAX_PROP_VALUE_LENGTH];
  int fpsrangeArray[2];
  int i = 0;

  LOG_FUNCTION_NAME;
  size_t size = strlen(mCameraProperties->get(CameraProperties::FRAMERATE_RANGE_SUPPORTED))+1;
  strncpy(supported, mCameraProperties->get(CameraProperties::FRAMERATE_RANGE_SUPPORTED), size);

  ptr = strtok (supported," (,)");

  while (ptr != NULL)
    {
      fpsrangeArray[i]= atoi(ptr)/CameraHal::VFR_SCALE;
      if (i == 1)
        {
          if (framerate == fpsrangeArray[i])
            {
              CAMHAL_LOGDB("SETTING FPS RANGE min = %d max = %d \n", fpsrangeArray[0], fpsrangeArray[1]);
              *min_fps = fpsrangeArray[0]*CameraHal::VFR_SCALE;
              *max_fps = fpsrangeArray[1]*CameraHal::VFR_SCALE;
              break;
            }
        }
      ptr = strtok (NULL, " (,)");
      i++;
      i%=2;
    }

  LOG_FUNCTION_NAME_EXIT;

}

void CameraHal::setPreferredPreviewRes(int width, int height)
{
  LOG_FUNCTION_NAME;

  if ( (width == 320) && (height == 240)){
    mParameters.setPreviewSize(640,480);
  }
  if ( (width == 176) && (height == 144)){
    mParameters.setPreviewSize(704,576);
  }

  LOG_FUNCTION_NAME_EXIT;
}

void CameraHal::resetPreviewRes(CameraParameters *mParams, int width, int height)
{
  LOG_FUNCTION_NAME;

  if ( (width <= 320) && (height <= 240)){
    mParams->setPreviewSize(mVideoWidth, mVideoHeight);
  }

  LOG_FUNCTION_NAME_EXIT;
}

};


