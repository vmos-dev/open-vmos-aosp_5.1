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
* @file OMXFocus.cpp
*
* This file contains functionality for handling focus configurations.
*
*/

#include "CameraHal.h"
#include "OMXCameraAdapter.h"
#include "ErrorUtils.h"

#define TOUCH_FOCUS_RANGE 0xFF
#define AF_IMAGE_CALLBACK_TIMEOUT 5000000 //5 seconds timeout
#define AF_VIDEO_CALLBACK_TIMEOUT 2800000 //2.8 seconds timeout

namespace Ti {
namespace Camera {

const nsecs_t OMXCameraAdapter::CANCEL_AF_TIMEOUT =  seconds_to_nanoseconds(1);

status_t OMXCameraAdapter::setParametersFocus(const android::CameraParameters &params,
                                              BaseCameraAdapter::AdapterState state)
{
    status_t ret = NO_ERROR;
    const char *str = NULL;
    android::Vector<android::sp<CameraArea> > tempAreas;
    size_t MAX_FOCUS_AREAS;

    LOG_FUNCTION_NAME;

    android::AutoMutex lock(mFocusAreasLock);

    str = params.get(android::CameraParameters::KEY_FOCUS_AREAS);

    MAX_FOCUS_AREAS = atoi(params.get(android::CameraParameters::KEY_MAX_NUM_FOCUS_AREAS));

    if ( NULL != str ) {
        ret = CameraArea::parseAreas(str, ( strlen(str) + 1 ), tempAreas);
    }

    if ( (NO_ERROR == ret) && CameraArea::areAreasDifferent(mFocusAreas, tempAreas) ) {
        mFocusAreas.clear();
        mFocusAreas = tempAreas;
        if ( MAX_FOCUS_AREAS < mFocusAreas.size() ) {
            CAMHAL_LOGEB("Focus areas supported %d, focus areas set %d",
                         MAX_FOCUS_AREAS,
                         mFocusAreas.size());
            ret = -EINVAL;
        }
        else {
            if ( !mFocusAreas.isEmpty() ) {
                setTouchFocus();
            }
        }
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::doAutoFocus()
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_IMAGE_CONFIG_FOCUSCONTROLTYPE focusControl;
    OMX_PARAM_FOCUSSTATUSTYPE focusStatus;
    OMX_CONFIG_BOOLEANTYPE bOMX;
    CameraAdapter::AdapterState state;
    nsecs_t timeout = 0;

    LOG_FUNCTION_NAME;

    if ( OMX_StateInvalid == mComponentState )
      {
        CAMHAL_LOGEA("OMX component in Invalid state");
        returnFocusStatus(false);
        return -EINVAL;
      }

    if ( OMX_StateExecuting != mComponentState )
        {
        CAMHAL_LOGEA("OMX component not in executing state");
        returnFocusStatus(false);
        return NO_ERROR;
        }

    if( ((AF_ACTIVE & getState()) != AF_ACTIVE) && ((AF_ACTIVE & getNextState()) != AF_ACTIVE) ) {
       CAMHAL_LOGDA("Auto focus got canceled before doAutoFocus could be called");
       return NO_ERROR;
    }

    // AF when fixed focus modes are set should be a no-op.
    if ( ( mParameters3A.Focus == OMX_IMAGE_FocusControlOff ) ||
         ( mParameters3A.Focus == OMX_IMAGE_FocusControlAutoInfinity ) ||
         ( mParameters3A.Focus == OMX_IMAGE_FocusControlHyperfocal ) ) {
        returnFocusStatus(true);
        return NO_ERROR;
    }

    OMX_INIT_STRUCT_PTR (&focusStatus, OMX_PARAM_FOCUSSTATUSTYPE);

    // If the app calls autoFocus, the camera will stop sending face callbacks.
    pauseFaceDetection(true);

    // This is needed for applying FOCUS_REGION correctly
    if ( (!mFocusAreas.isEmpty()) && (!mFocusAreas.itemAt(0)->isZeroArea()))
    {
    //Disable face priority
    setAlgoPriority(FACE_PRIORITY, FOCUS_ALGO, false);

    //Enable region algorithm priority
    setAlgoPriority(REGION_PRIORITY, FOCUS_ALGO, true);
    }

    OMX_INIT_STRUCT_PTR (&focusControl, OMX_IMAGE_CONFIG_FOCUSCONTROLTYPE);
    focusControl.eFocusControl = ( OMX_IMAGE_FOCUSCONTROLTYPE ) mParameters3A.Focus;

    if (mParameters3A.FocusLock) {
        // this basically means user never called cancelAutoFocus after a scan...
        // if this is the case we need to unlock AF to ensure we will do a scan
        if (set3ALock(mUserSetExpLock, mUserSetWbLock, OMX_FALSE) != NO_ERROR) {
            CAMHAL_LOGEA("Error Unlocking 3A locks");
        } else {
            CAMHAL_LOGDA("AE/AWB unlocked successfully");
        }

    } else if ( mParameters3A.Focus == OMX_IMAGE_FocusControlAuto ) {
        // In case we have CAF running we should first check the AF status.
        // If it has managed to lock, then do as usual and return status
        // immediately.
        ret = checkFocus(&focusStatus);
        if ( NO_ERROR != ret ) {
            CAMHAL_LOGEB("Focus status check failed 0x%x!", ret);
            return ret;
        } else {
            CAMHAL_LOGDB("Focus status check 0x%x!", focusStatus.eFocusStatus);
        }
    }

    if ( (focusControl.eFocusControl == OMX_IMAGE_FocusControlAuto &&
         ( focusStatus.eFocusStatus == OMX_FocusStatusRequest ||
           focusStatus.eFocusStatus == OMX_FocusStatusUnableToReach ||
           focusStatus.eFocusStatus == OMX_FocusStatusLost ) ) ||
            (mParameters3A.Focus !=  (OMX_IMAGE_FOCUSCONTROLTYPE)OMX_IMAGE_FocusControlAuto) ) {
        OMX_INIT_STRUCT_PTR (&bOMX, OMX_CONFIG_BOOLEANTYPE);
        bOMX.bEnabled = OMX_TRUE;

        //Enable focus scanning
        eError = OMX_SetConfig(mCameraAdapterParameters.mHandleComp,
                               (OMX_INDEXTYPE)OMX_TI_IndexConfigAutofocusEnable,
                               &bOMX);
        if ( OMX_ErrorNone != eError ) {
            return Utils::ErrorUtils::omxToAndroidError(eError);
        }

        {
            android::AutoMutex lock(mDoAFMutex);

        // force AF, Ducati will take care of whether CAF
        // or AF will be performed, depending on light conditions
        if ( focusControl.eFocusControl == OMX_IMAGE_FocusControlAuto &&
             ( focusStatus.eFocusStatus == OMX_FocusStatusUnableToReach ||
               focusStatus.eFocusStatus == OMX_FocusStatusLost ) ) {
            focusControl.eFocusControl = OMX_IMAGE_FocusControlAutoLock;
        }

            eError =  OMX_SetConfig(mCameraAdapterParameters.mHandleComp,
                                    OMX_IndexConfigFocusControl,
                                    &focusControl);

            if ( OMX_ErrorNone != eError ) {
                CAMHAL_LOGEB("Error while starting focus 0x%x", eError);
                return INVALID_OPERATION;
            } else {
                CAMHAL_LOGDA("Autofocus started successfully");
            }

            // No need to wait if preview is about to stop
            getNextState(state);
            if ( ( PREVIEW_ACTIVE & state ) != PREVIEW_ACTIVE ) {
                return NO_ERROR;
            }

            // configure focus timeout based on capture mode
            timeout = (mCapMode == VIDEO_MODE) || (mCapMode == VIDEO_MODE_HQ) ?
                            ( ( nsecs_t ) AF_VIDEO_CALLBACK_TIMEOUT * 1000 ) :
                            ( ( nsecs_t ) AF_IMAGE_CALLBACK_TIMEOUT * 1000 );


            ret = mDoAFCond.waitRelative(mDoAFMutex, timeout);
        }

        //If somethiing bad happened while we wait
        if (mComponentState == OMX_StateInvalid) {
          CAMHAL_LOGEA("Invalid State after Auto Focus Exitting!!!");
          return -EINVAL;
        }

        if(ret != NO_ERROR) {
            CAMHAL_LOGEA("Autofocus callback timeout expired");
            ret = returnFocusStatus(true);
        } else {
            CAMHAL_LOGDA("Autofocus callback received");
            ret = returnFocusStatus(false);
        }
    } else { // Focus mode in continuous
        if ( NO_ERROR == ret ) {
            ret = returnFocusStatus(true);
            mPending3Asettings |= SetFocus;
        }
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::stopAutoFocus()
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_IMAGE_CONFIG_FOCUSCONTROLTYPE focusControl;

    LOG_FUNCTION_NAME;

    if ( OMX_StateInvalid == mComponentState )  {
        CAMHAL_LOGEA("OMX component in Invalid state");
        returnFocusStatus(false);
        return -EINVAL;
    }

    if ( OMX_StateExecuting != mComponentState ) {
          CAMHAL_LOGEA("OMX component not in executing state");
        return NO_ERROR;
    }

    if ( mParameters3A.Focus == OMX_IMAGE_FocusControlAutoInfinity ) {
        // No need to stop focus if we are in infinity mode. Nothing to stop.
        return NO_ERROR;
    }

    OMX_INIT_STRUCT_PTR (&focusControl, OMX_IMAGE_CONFIG_FOCUSCONTROLTYPE);
    focusControl.eFocusControl = OMX_IMAGE_FocusControlOff;

    eError =  OMX_SetConfig(mCameraAdapterParameters.mHandleComp,
                            OMX_IndexConfigFocusControl,
                            &focusControl);
    if ( OMX_ErrorNone != eError ) {
        CAMHAL_LOGEB("Error while stopping focus 0x%x", eError);
        return Utils::ErrorUtils::omxToAndroidError(eError);
    }
#ifdef CAMERAHAL_TUNA
    else {
        // This is a WA. Usually the OMX Camera component should
        // generate AF status change OMX event fairly quickly
        // ( after one preview frame ) and this notification should
        // actually come from 'handleFocusCallback()'.
        android::AutoMutex lock(mDoAFMutex);
        mDoAFCond.broadcast();
    }
#endif

    LOG_FUNCTION_NAME_EXIT;

    return NO_ERROR;
}

status_t OMXCameraAdapter::getFocusMode(OMX_IMAGE_CONFIG_FOCUSCONTROLTYPE &focusMode)
{;
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    LOG_FUNCTION_NAME;

    if ( OMX_StateInvalid == mComponentState ) {
        CAMHAL_LOGEA("OMX component is in invalid state");
        return NO_INIT;
    }

    OMX_INIT_STRUCT_PTR (&focusMode, OMX_IMAGE_CONFIG_FOCUSCONTROLTYPE);
    focusMode.nPortIndex = mCameraAdapterParameters.mPrevPortIndex;

    eError =  OMX_GetConfig(mCameraAdapterParameters.mHandleComp,
                            OMX_IndexConfigFocusControl,
                            &focusMode);

    if ( OMX_ErrorNone != eError ) {
        CAMHAL_LOGEB("Error while retrieving focus mode 0x%x", eError);
    }

    LOG_FUNCTION_NAME_EXIT;

    return Utils::ErrorUtils::omxToAndroidError(eError);
}

status_t OMXCameraAdapter::cancelAutoFocus()
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_IMAGE_CONFIG_FOCUSCONTROLTYPE focusMode;

    LOG_FUNCTION_NAME;

    ret = getFocusMode(focusMode);
    if ( NO_ERROR != ret ) {
        return ret;
    }

    //Stop the AF only for modes other than CAF, Inifinity or Off
    if ( ( focusMode.eFocusControl != OMX_IMAGE_FocusControlAuto ) &&
         ( focusMode.eFocusControl != ( OMX_IMAGE_FOCUSCONTROLTYPE )
                 OMX_IMAGE_FocusControlAutoInfinity ) &&
         ( focusMode.eFocusControl != OMX_IMAGE_FocusControlOff ) ) {
        android::AutoMutex lock(mCancelAFMutex);
        stopAutoFocus();
        ret = mCancelAFCond.waitRelative(mCancelAFMutex, CANCEL_AF_TIMEOUT);
        if ( NO_ERROR != ret ) {
            CAMHAL_LOGE("Cancel AF timeout!");
        }
    } else if (focusMode.eFocusControl == OMX_IMAGE_FocusControlAuto) {
       // This re-enabling of CAF doesn't seem to
       // be needed any more.
       // re-apply CAF after unlocking and canceling
       // mPending3Asettings |= SetFocus;
    }

    {
        // Signal to 'doAutoFocus()'
        android::AutoMutex lock(mDoAFMutex);
        mDoAFCond.broadcast();
    }

    // If the apps call #cancelAutoFocus()}, the face callbacks will also resume.
    pauseFaceDetection(false);

    LOG_FUNCTION_NAME_EXIT;

    return ret;

}

status_t OMXCameraAdapter::setFocusCallback(bool enabled)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONFIG_CALLBACKREQUESTTYPE focusRequstCallback;

    LOG_FUNCTION_NAME;

    if ( OMX_StateInvalid == mComponentState )
      {
        CAMHAL_LOGEA("OMX component in Invalid state");
        ret = -EINVAL;
      }

    if ( OMX_StateExecuting != mComponentState )
        {
          CAMHAL_LOGEA("OMX component not in executing state");
        return NO_ERROR;
        }

    if ( NO_ERROR == ret )
        {

        OMX_INIT_STRUCT_PTR (&focusRequstCallback, OMX_CONFIG_CALLBACKREQUESTTYPE);
        focusRequstCallback.nPortIndex = OMX_ALL;
        focusRequstCallback.nIndex = OMX_IndexConfigCommonFocusStatus;

        if ( enabled )
            {
            focusRequstCallback.bEnable = OMX_TRUE;
            }
        else
            {
            focusRequstCallback.bEnable = OMX_FALSE;
            }

        eError =  OMX_SetConfig(mCameraAdapterParameters.mHandleComp,
                                (OMX_INDEXTYPE) OMX_IndexConfigCallbackRequest,
                                &focusRequstCallback);
        if ( OMX_ErrorNone != eError )
            {
            CAMHAL_LOGEB("Error registering focus callback 0x%x", eError);
            ret = -1;
            }
        else
            {
            CAMHAL_LOGDB("Autofocus callback for index 0x%x registered successfully",
                         OMX_IndexConfigCommonFocusStatus);
            }
        }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::returnFocusStatus(bool timeoutReached)
{
    status_t ret = NO_ERROR;
    OMX_PARAM_FOCUSSTATUSTYPE eFocusStatus;
    CameraHalEvent::FocusStatus focusStatus = CameraHalEvent::FOCUS_STATUS_FAIL;
    BaseCameraAdapter::AdapterState state, nextState;
    BaseCameraAdapter::getState(state);
    BaseCameraAdapter::getNextState(nextState);

    LOG_FUNCTION_NAME;

    OMX_INIT_STRUCT(eFocusStatus, OMX_PARAM_FOCUSSTATUSTYPE);

    if( ((AF_ACTIVE & state ) != AF_ACTIVE) && ((AF_ACTIVE & nextState ) != AF_ACTIVE) )
       {
        /// We don't send focus callback if focus was not started
       CAMHAL_LOGDA("Not sending focus callback because focus was not started");
       return NO_ERROR;
       }

    if ( NO_ERROR == ret )
        {

        if ( !timeoutReached )
            {
            ret = checkFocus(&eFocusStatus);

            if ( NO_ERROR != ret )
                {
                CAMHAL_LOGEA("Focus status check failed!");
                }
            }
        }

    if ( NO_ERROR == ret )
        {

        if ( timeoutReached )
            {
            focusStatus = CameraHalEvent::FOCUS_STATUS_FAIL;
            }
        else
            {
            switch (eFocusStatus.eFocusStatus)
                {
                    case OMX_FocusStatusReached:
                        {
                        focusStatus = CameraHalEvent::FOCUS_STATUS_SUCCESS;
                        break;
                        }
                    case OMX_FocusStatusOff: // AF got canceled
                        return NO_ERROR;
                    case OMX_FocusStatusUnableToReach:
                    case OMX_FocusStatusRequest:
                    default:
                        {
                        focusStatus = CameraHalEvent::FOCUS_STATUS_FAIL;
                        break;
                        }
                }
            // Lock CAF after AF call
            if( set3ALock(mUserSetExpLock, mUserSetWbLock, OMX_TRUE) != NO_ERROR) {
                CAMHAL_LOGEA("Error Applying 3A locks");
            } else {
                CAMHAL_LOGDA("Focus locked. Applied focus locks successfully");
            }

            stopAutoFocus();
            }
        //Query current focus distance after AF is complete
        updateFocusDistances(mParameters);
       }

    ret =  BaseCameraAdapter::setState(CAMERA_CANCEL_AUTOFOCUS);
    if ( NO_ERROR == ret )
        {
        ret = BaseCameraAdapter::commitState();
        }
    else
        {
        ret |= BaseCameraAdapter::rollbackState();
        }

    if ( NO_ERROR == ret )
        {
        notifyFocusSubscribers(focusStatus);
        }

    // After focus, face detection will resume sending face callbacks
    pauseFaceDetection(false);

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::checkFocus(OMX_PARAM_FOCUSSTATUSTYPE *eFocusStatus)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    LOG_FUNCTION_NAME;

    if ( NULL == eFocusStatus )
        {
        CAMHAL_LOGEA("Invalid focus status");
        ret = -EINVAL;
        }

    if ( OMX_StateInvalid == mComponentState )
      {
        CAMHAL_LOGEA("OMX component in Invalid state");
        ret = -EINVAL;
      }

    if ( OMX_StateExecuting != mComponentState )
        {
        CAMHAL_LOGEA("OMX component not in executing state");
        ret = NO_ERROR;
        }

    if ( NO_ERROR == ret )
        {
        OMX_INIT_STRUCT_PTR (eFocusStatus, OMX_PARAM_FOCUSSTATUSTYPE);

        eError = OMX_GetConfig(mCameraAdapterParameters.mHandleComp,
                               OMX_IndexConfigCommonFocusStatus,
                               eFocusStatus);
        if ( OMX_ErrorNone != eError )
            {
            CAMHAL_LOGEB("Error while retrieving focus status: 0x%x", eError);
            ret = -1;
            }
        }

    if ( NO_ERROR == ret )
        {
        CAMHAL_LOGDB("Focus Status: %d", eFocusStatus->eFocusStatus);
        }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::updateFocusDistances(android::CameraParameters &params)
{
    OMX_U32 focusNear, focusOptimal, focusFar;
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    ret = getFocusDistances(focusNear, focusOptimal, focusFar);
    if ( NO_ERROR == ret)
        {
        ret = addFocusDistances(focusNear, focusOptimal, focusFar, params);
            if ( NO_ERROR != ret )
                {
                CAMHAL_LOGEB("Error in call to addFocusDistances() 0x%x", ret);
                }
        }
    else
        {
        CAMHAL_LOGEB("Error in call to getFocusDistances() 0x%x", ret);
        }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::getFocusDistances(OMX_U32 &near,OMX_U32 &optimal, OMX_U32 &far)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError;

    OMX_TI_CONFIG_FOCUSDISTANCETYPE focusDist;

    LOG_FUNCTION_NAME;

    if ( OMX_StateInvalid == mComponentState )
        {
        CAMHAL_LOGEA("OMX component is in invalid state");
        ret = UNKNOWN_ERROR;
        }

    if ( NO_ERROR == ret )
        {
        OMX_INIT_STRUCT_PTR(&focusDist, OMX_TI_CONFIG_FOCUSDISTANCETYPE);
        focusDist.nPortIndex = mCameraAdapterParameters.mPrevPortIndex;

        eError = OMX_GetConfig(mCameraAdapterParameters.mHandleComp,
                               ( OMX_INDEXTYPE ) OMX_TI_IndexConfigFocusDistance,
                               &focusDist);
        if ( OMX_ErrorNone != eError )
            {
            CAMHAL_LOGEB("Error while querying focus distances 0x%x", eError);
            ret = UNKNOWN_ERROR;
            }

        }

    if ( NO_ERROR == ret )
        {
        near = focusDist.nFocusDistanceNear;
        optimal = focusDist.nFocusDistanceOptimal;
        far = focusDist.nFocusDistanceFar;
        }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::encodeFocusDistance(OMX_U32 dist, char *buffer, size_t length)
{
    status_t ret = NO_ERROR;
    uint32_t focusScale = 1000;
    float distFinal;

    LOG_FUNCTION_NAME;

    if(mParameters3A.Focus == OMX_IMAGE_FocusControlAutoInfinity)
        {
        dist=0;
        }

    if ( NO_ERROR == ret )
        {
        if ( 0 == dist )
            {
            strncpy(buffer, android::CameraParameters::FOCUS_DISTANCE_INFINITY, ( length - 1 ));
            }
        else
            {
            distFinal = dist;
            distFinal /= focusScale;
            snprintf(buffer, ( length - 1 ) , "%5.3f", distFinal);
            }
        }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::addFocusDistances(OMX_U32 &near,
                                             OMX_U32 &optimal,
                                             OMX_U32 &far,
                                             android::CameraParameters& params)
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    if ( NO_ERROR == ret )
        {
        ret = encodeFocusDistance(near, mFocusDistNear, FOCUS_DIST_SIZE);
        if ( NO_ERROR != ret )
            {
            CAMHAL_LOGEB("Error encoding near focus distance 0x%x", ret);
            }
        }

    if ( NO_ERROR == ret )
        {
        ret = encodeFocusDistance(optimal, mFocusDistOptimal, FOCUS_DIST_SIZE);
        if ( NO_ERROR != ret )
            {
            CAMHAL_LOGEB("Error encoding near focus distance 0x%x", ret);
            }
        }

    if ( NO_ERROR == ret )
        {
        ret = encodeFocusDistance(far, mFocusDistFar, FOCUS_DIST_SIZE);
        if ( NO_ERROR != ret )
            {
            CAMHAL_LOGEB("Error encoding near focus distance 0x%x", ret);
            }
        }

    if ( NO_ERROR == ret )
        {
        snprintf(mFocusDistBuffer, ( FOCUS_DIST_BUFFER_SIZE - 1) ,"%s,%s,%s", mFocusDistNear,
                                                                              mFocusDistOptimal,
                                                                              mFocusDistFar);

        params.set(android::CameraParameters::KEY_FOCUS_DISTANCES, mFocusDistBuffer);
        }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::setTouchFocus()
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    OMX_ALGOAREASTYPE *focusAreas;
    OMX_TI_CONFIG_SHAREDBUFFER sharedBuffer;
    CameraBuffer *bufferlist;
    int areasSize = 0;

    LOG_FUNCTION_NAME;

    if ( OMX_StateInvalid == mComponentState )
        {
        CAMHAL_LOGEA("OMX component is in invalid state");
        ret = -1;
        }

    if ( NO_ERROR == ret )
        {

        areasSize = ((sizeof(OMX_ALGOAREASTYPE)+4095)/4096)*4096;
        bufferlist = mMemMgr.allocateBufferList(0, 0, NULL, areasSize, 1);
        focusAreas = (OMX_ALGOAREASTYPE*) bufferlist[0].opaque;

        OMXCameraPortParameters * mPreviewData = NULL;
        mPreviewData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mPrevPortIndex];

        if (!focusAreas)
            {
            CAMHAL_LOGEB("Error allocating buffer for focus areas %d", eError);
            return -ENOMEM;
            }

        OMX_INIT_STRUCT_PTR (focusAreas, OMX_ALGOAREASTYPE);

        focusAreas->nPortIndex = OMX_ALL;
        focusAreas->nNumAreas = mFocusAreas.size();
        focusAreas->nAlgoAreaPurpose = OMX_AlgoAreaFocus;

        // If the area is the special case of (0, 0, 0, 0, 0), then
        // the algorithm needs nNumAreas to be set to 0,
        // in order to automatically choose the best fitting areas.
        if ( mFocusAreas.itemAt(0)->isZeroArea() )
            {
            focusAreas->nNumAreas = 0;
            }

        for ( unsigned int n = 0; n < mFocusAreas.size(); n++) {
            int widthDivisor = 1;
            int heightDivisor = 1;

            if (mPreviewData->mFrameLayoutType == OMX_TI_StereoFrameLayoutTopBottom) {
                heightDivisor = 2;
            }
            if (mPreviewData->mFrameLayoutType == OMX_TI_StereoFrameLayoutLeftRight) {
                widthDivisor = 2;
            }

            // transform the coordinates to 3A-type coordinates
            mFocusAreas.itemAt(n)->transfrom((size_t)mPreviewData->mWidth/widthDivisor,
                                            (size_t)mPreviewData->mHeight/heightDivisor,
                                            (size_t&)focusAreas->tAlgoAreas[n].nTop,
                                            (size_t&)focusAreas->tAlgoAreas[n].nLeft,
                                            (size_t&)focusAreas->tAlgoAreas[n].nWidth,
                                            (size_t&)focusAreas->tAlgoAreas[n].nHeight);

            focusAreas->tAlgoAreas[n].nLeft =
                    ( focusAreas->tAlgoAreas[n].nLeft * TOUCH_FOCUS_RANGE ) / mPreviewData->mWidth;
            focusAreas->tAlgoAreas[n].nTop =
                    ( focusAreas->tAlgoAreas[n].nTop* TOUCH_FOCUS_RANGE ) / mPreviewData->mHeight;
            focusAreas->tAlgoAreas[n].nWidth =
                    ( focusAreas->tAlgoAreas[n].nWidth * TOUCH_FOCUS_RANGE ) / mPreviewData->mWidth;
            focusAreas->tAlgoAreas[n].nHeight =
                    ( focusAreas->tAlgoAreas[n].nHeight * TOUCH_FOCUS_RANGE ) / mPreviewData->mHeight;
            focusAreas->tAlgoAreas[n].nPriority = mFocusAreas.itemAt(n)->getWeight();

             CAMHAL_LOGDB("Focus area %d : top = %d left = %d width = %d height = %d prio = %d",
                    n, (int)focusAreas->tAlgoAreas[n].nTop, (int)focusAreas->tAlgoAreas[n].nLeft,
                    (int)focusAreas->tAlgoAreas[n].nWidth, (int)focusAreas->tAlgoAreas[n].nHeight,
                    (int)focusAreas->tAlgoAreas[n].nPriority);
        }

        OMX_INIT_STRUCT_PTR (&sharedBuffer, OMX_TI_CONFIG_SHAREDBUFFER);

        sharedBuffer.nPortIndex = OMX_ALL;
        sharedBuffer.nSharedBuffSize = areasSize;
        sharedBuffer.pSharedBuff = (OMX_U8 *) camera_buffer_get_omx_ptr (&bufferlist[0]);

        if ( NULL == sharedBuffer.pSharedBuff )
            {
            CAMHAL_LOGEA("No resources to allocate OMX shared buffer");
            ret = -ENOMEM;
            goto EXIT;
            }

            eError =  OMX_SetConfig(mCameraAdapterParameters.mHandleComp,
                                      (OMX_INDEXTYPE) OMX_TI_IndexConfigAlgoAreas, &sharedBuffer);

        if ( OMX_ErrorNone != eError )
            {
            CAMHAL_LOGEB("Error while setting Focus Areas configuration 0x%x", eError);
            ret = -EINVAL;
            }

    EXIT:
        if (NULL != bufferlist)
            {
            mMemMgr.freeBufferList (bufferlist);
            }
        }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

void OMXCameraAdapter::handleFocusCallback() {
    OMX_PARAM_FOCUSSTATUSTYPE eFocusStatus;
    CameraHalEvent::FocusStatus focusStatus = CameraHalEvent::FOCUS_STATUS_FAIL;
    status_t ret = NO_ERROR;
    BaseCameraAdapter::AdapterState nextState;
    BaseCameraAdapter::getNextState(nextState);

    OMX_INIT_STRUCT(eFocusStatus, OMX_PARAM_FOCUSSTATUSTYPE);

    ret = checkFocus(&eFocusStatus);

    if (NO_ERROR != ret) {
        CAMHAL_LOGEA("Focus status check failed!");
        // signal and unblock doAutoFocus
        if (AF_ACTIVE & nextState) {
            android::AutoMutex lock(mDoAFMutex);
            mDoAFCond.broadcast();
        }
        return;
    }

    if ( eFocusStatus.eFocusStatus == OMX_FocusStatusOff ) {
        android::AutoMutex lock(mCancelAFMutex);
        mCancelAFCond.signal();
        return;
    }

    if (eFocusStatus.eFocusStatus != OMX_FocusStatusRequest) {
        // signal doAutoFocus when a end of scan message comes
        // ignore start of scan
        android::AutoMutex lock(mDoAFMutex);
        mDoAFCond.broadcast();
    }

    if (mParameters3A.Focus != (OMX_IMAGE_FOCUSCONTROLTYPE) OMX_IMAGE_FocusControlAuto) {
       CAMHAL_LOGDA("unregistered focus callback when not in CAF or doAutoFocus... not handling");
       return;
    }

    // Handling for CAF Callbacks
    switch (eFocusStatus.eFocusStatus) {
        case OMX_FocusStatusRequest:
            focusStatus = CameraHalEvent::FOCUS_STATUS_PENDING;
            break;
        case OMX_FocusStatusReached:
        case OMX_FocusStatusOff:
        case OMX_FocusStatusUnableToReach:
        default:
            focusStatus = CameraHalEvent::FOCUS_STATUS_DONE;
            break;
    }

    notifyFocusSubscribers(focusStatus);
}

} // namespace Camera
} // namespace Ti
