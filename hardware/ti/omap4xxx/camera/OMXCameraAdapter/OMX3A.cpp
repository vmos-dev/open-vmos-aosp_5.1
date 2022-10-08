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
* @file OMX3A.cpp
*
* This file contains functionality for handling 3A configurations.
*
*/

#undef LOG_TAG

#define LOG_TAG "CameraHAL"

#include "CameraHal.h"
#include "OMXCameraAdapter.h"
#include "ErrorUtils.h"

#include <cutils/properties.h>

#undef TRUE
#undef FALSE
#define TRUE "true"
#define FALSE "false"

#define METERING_AREAS_RANGE 0xFF

namespace android {
const SceneModesEntry* OMXCameraAdapter::getSceneModeEntry(const char* name,
                                                                  OMX_SCENEMODETYPE scene) {
    const SceneModesEntry* cameraLUT = NULL;
    const SceneModesEntry* entry = NULL;
    unsigned int numEntries = 0;

    // 1. Find camera's scene mode LUT
    for (unsigned int i = 0; i < ARRAY_SIZE(CameraToSensorModesLUT); i++) {
        if (strcmp(CameraToSensorModesLUT[i].name, name) == 0) {
            cameraLUT = CameraToSensorModesLUT[i].Table;
            numEntries = CameraToSensorModesLUT[i].size;
            break;
        }
    }

    // 2. Find scene mode entry in table
    if (!cameraLUT) {
        goto EXIT;
    }

    for (unsigned int i = 0; i < numEntries; i++) {
        if(cameraLUT[i].scene == scene) {
            entry = cameraLUT + i;
            break;
        }
    }
 EXIT:
    return entry;
}

status_t OMXCameraAdapter::setParameters3A(const CameraParameters &params,
                                           BaseCameraAdapter::AdapterState state)
{
    status_t ret = NO_ERROR;
    int mode = 0;
    const char *str = NULL;
    int varint = 0;
    BaseCameraAdapter::AdapterState nextState;
    BaseCameraAdapter::getNextState(nextState);

    LOG_FUNCTION_NAME;

    Mutex::Autolock lock(m3ASettingsUpdateLock);

    str = params.get(CameraParameters::KEY_SCENE_MODE);
    mode = getLUTvalue_HALtoOMX( str, SceneLUT);
    if ( mFirstTimeInit || ((str != NULL) && ( mParameters3A.SceneMode != mode )) ) {
        if ( 0 <= mode ) {
            mParameters3A.SceneMode = mode;
            if ((mode == OMX_Manual) && (mFirstTimeInit == false)){//Auto mode
                mFirstTimeInit = true;
            }
            if ((mode != OMX_Manual) &&
                (state & PREVIEW_ACTIVE) && !(nextState & CAPTURE_ACTIVE)) {
                // if setting preset scene mode, previewing, and not in the middle of capture
                // set preset scene mode immediately instead of in next FBD
                // for feedback params to work properly since they need to be read
                // by application in subsequent getParameters()
                ret |= setScene(mParameters3A);
                // re-apply EV compensation after setting scene mode since it probably reset it
                if(mParameters3A.EVCompensation) {
                   setEVCompensation(mParameters3A);
                }
                return ret;
            } else {
                mPending3Asettings |= SetSceneMode;
            }
        } else {
            mParameters3A.SceneMode = OMX_Manual;
        }
        CAMHAL_LOGVB("SceneMode %d", mParameters3A.SceneMode);
    }

#ifdef OMAP_ENHANCEMENT

    str = params.get(TICameraParameters::KEY_EXPOSURE_MODE);
    mode = getLUTvalue_HALtoOMX( str, ExpLUT);
    if ( ( str != NULL ) && ( mParameters3A.Exposure != mode ))
        {
        mParameters3A.Exposure = mode;
        CAMHAL_LOGDB("Exposure mode %d", mode);
        if ( 0 <= mParameters3A.Exposure )
            {
            mPending3Asettings |= SetExpMode;
            }
        }

#endif

    str = params.get(CameraParameters::KEY_WHITE_BALANCE);
    mode = getLUTvalue_HALtoOMX( str, WBalLUT);
    if (mFirstTimeInit || ((str != NULL) && (mode != mParameters3A.WhiteBallance)))
        {
        mParameters3A.WhiteBallance = mode;
        CAMHAL_LOGDB("Whitebalance mode %d", mode);
        if ( 0 <= mParameters3A.WhiteBallance )
            {
            mPending3Asettings |= SetWhiteBallance;
            }
        }

#ifdef OMAP_ENHANCEMENT

    varint = params.getInt(TICameraParameters::KEY_CONTRAST);
    if ( 0 <= varint )
        {
        if ( mFirstTimeInit ||
             ( (mParameters3A.Contrast  + CONTRAST_OFFSET) != varint ) )
            {
            mParameters3A.Contrast = varint - CONTRAST_OFFSET;
            CAMHAL_LOGDB("Contrast %d", mParameters3A.Contrast);
            mPending3Asettings |= SetContrast;
            }
        }

    varint = params.getInt(TICameraParameters::KEY_SHARPNESS);
    if ( 0 <= varint )
        {
        if ( mFirstTimeInit ||
             ((mParameters3A.Sharpness + SHARPNESS_OFFSET) != varint ))
            {
            mParameters3A.Sharpness = varint - SHARPNESS_OFFSET;
            CAMHAL_LOGDB("Sharpness %d", mParameters3A.Sharpness);
            mPending3Asettings |= SetSharpness;
            }
        }

    varint = params.getInt(TICameraParameters::KEY_SATURATION);
    if ( 0 <= varint )
        {
        if ( mFirstTimeInit ||
             ((mParameters3A.Saturation + SATURATION_OFFSET) != varint ) )
            {
            mParameters3A.Saturation = varint - SATURATION_OFFSET;
            CAMHAL_LOGDB("Saturation %d", mParameters3A.Saturation);
            mPending3Asettings |= SetSaturation;
            }
        }

    varint = params.getInt(TICameraParameters::KEY_BRIGHTNESS);
    if ( 0 <= varint )
        {
        if ( mFirstTimeInit ||
             (( mParameters3A.Brightness != varint )) )
            {
            mParameters3A.Brightness = (unsigned) varint;
            CAMHAL_LOGDB("Brightness %d", mParameters3A.Brightness);
            mPending3Asettings |= SetBrightness;
            }
        }

#endif

    str = params.get(CameraParameters::KEY_ANTIBANDING);
    mode = getLUTvalue_HALtoOMX(str,FlickerLUT);
    if ( mFirstTimeInit || ( ( str != NULL ) && ( mParameters3A.Flicker != mode ) ))
        {
        mParameters3A.Flicker = mode;
        CAMHAL_LOGDB("Flicker %d", mParameters3A.Flicker);
        if ( 0 <= mParameters3A.Flicker )
            {
            mPending3Asettings |= SetFlicker;
            }
        }

#ifdef OMAP_ENHANCEMENT

    str = params.get(TICameraParameters::KEY_ISO);
    mode = getLUTvalue_HALtoOMX(str, IsoLUT);
    CAMHAL_LOGVB("ISO mode arrived in HAL : %s", str);
    if ( mFirstTimeInit || (  ( str != NULL ) && ( mParameters3A.ISO != mode )) )
        {
        mParameters3A.ISO = mode;
        CAMHAL_LOGDB("ISO %d", mParameters3A.ISO);
        if ( 0 <= mParameters3A.ISO )
            {
            mPending3Asettings |= SetISO;
            }
        }

#endif

    str = params.get(CameraParameters::KEY_FOCUS_MODE);
    mode = getLUTvalue_HALtoOMX(str, FocusLUT);
    if ( (mFirstTimeInit || ((str != NULL) && (mParameters3A.Focus != mode))))
        {
        mPending3Asettings |= SetFocus;

        mParameters3A.Focus = mode;

        // if focus mode is set to infinity...update focus distance immediately
        if (mode == OMX_IMAGE_FocusControlAutoInfinity) {
            updateFocusDistances(mParameters);
        }

        CAMHAL_LOGDB("Focus %x", mParameters3A.Focus);
        }

    str = params.get(CameraParameters::KEY_EXPOSURE_COMPENSATION);
    varint = params.getInt(CameraParameters::KEY_EXPOSURE_COMPENSATION);
    if ( mFirstTimeInit ||
          (( str != NULL ) &&
                  (mParameters3A.EVCompensation != varint )))
        {
        CAMHAL_LOGDB("Setting EV Compensation to %d", varint);

        mParameters3A.EVCompensation = varint;
        mPending3Asettings |= SetEVCompensation;
        }

    str = params.get(CameraParameters::KEY_FLASH_MODE);
    mode = getLUTvalue_HALtoOMX( str, FlashLUT);
    if (  mFirstTimeInit || (( str != NULL ) && ( mParameters3A.FlashMode != mode )) )
        {
        if ( 0 <= mode )
            {
            mParameters3A.FlashMode = mode;
            mPending3Asettings |= SetFlash;
            }
        else
            {
            mParameters3A.FlashMode = OMX_Manual;
            }
        }

    CAMHAL_LOGVB("Flash Setting %s", str);
    CAMHAL_LOGVB("FlashMode %d", mParameters3A.FlashMode);

    str = params.get(CameraParameters::KEY_EFFECT);
    mode = getLUTvalue_HALtoOMX( str, EffLUT);
    if (  mFirstTimeInit || (( str != NULL ) && ( mParameters3A.Effect != mode )) )
        {
        mParameters3A.Effect = mode;
        CAMHAL_LOGDB("Effect %d", mParameters3A.Effect);
        if ( 0 <= mParameters3A.Effect )
            {
            mPending3Asettings |= SetEffect;
            }
        }

    str = params.get(CameraParameters::KEY_AUTO_EXPOSURE_LOCK_SUPPORTED);
    if ( (str != NULL) && (!strcmp(str, "true")) )
      {
        OMX_BOOL lock = OMX_FALSE;
        mUserSetExpLock = OMX_FALSE;
        str = params.get(CameraParameters::KEY_AUTO_EXPOSURE_LOCK);
        if (str && ((strcmp(str, "true")) == 0))
          {
            CAMHAL_LOGVA("Locking Exposure");
            lock = OMX_TRUE;
            mUserSetExpLock = OMX_TRUE;
          }
        else
          {
            CAMHAL_LOGVA("UnLocking Exposure");
          }

        if (mParameters3A.ExposureLock != lock)
          {
            mParameters3A.ExposureLock = lock;
            CAMHAL_LOGDB("ExposureLock %d", lock);
            mPending3Asettings |= SetExpLock;
          }
      }

    str = params.get(CameraParameters::KEY_AUTO_WHITEBALANCE_LOCK_SUPPORTED);
    if ( (str != NULL) && (!strcmp(str, "true")) )
      {
        OMX_BOOL lock = OMX_FALSE;
        mUserSetWbLock = OMX_FALSE;
        str = params.get(CameraParameters::KEY_AUTO_WHITEBALANCE_LOCK);
        if (str && ((strcmp(str, "true")) == 0))
          {
            CAMHAL_LOGVA("Locking WhiteBalance");
            lock = OMX_TRUE;
            mUserSetWbLock = OMX_TRUE;
          }
        else
          {
            CAMHAL_LOGVA("UnLocking WhiteBalance");
          }
        if (mParameters3A.WhiteBalanceLock != lock)
          {
            mParameters3A.WhiteBalanceLock = lock;
            CAMHAL_LOGDB("WhiteBalanceLock %d", lock);
            mPending3Asettings |= SetWBLock;
          }
      }

    str = params.get(TICameraParameters::KEY_AUTO_FOCUS_LOCK);
    if (str && (strcmp(str, TRUE) == 0) && (mParameters3A.FocusLock != OMX_TRUE)) {
        CAMHAL_LOGVA("Locking Focus");
        mParameters3A.FocusLock = OMX_TRUE;
        setFocusLock(mParameters3A);
    } else if (str && (strcmp(str, FALSE) == 0) && (mParameters3A.FocusLock != OMX_FALSE)) {
        CAMHAL_LOGVA("UnLocking Focus");
        mParameters3A.FocusLock = OMX_FALSE;
        setFocusLock(mParameters3A);
    }

    str = params.get(CameraParameters::KEY_METERING_AREAS);
    if ( (str != NULL) ) {
        size_t MAX_METERING_AREAS;
        Vector< sp<CameraArea> > tempAreas;

        MAX_METERING_AREAS = atoi(params.get(CameraParameters::KEY_MAX_NUM_METERING_AREAS));

        Mutex::Autolock lock(mMeteringAreasLock);

        ret = CameraArea::parseAreas(str, ( strlen(str) + 1 ), tempAreas);

        CAMHAL_LOGVB("areAreasDifferent? = %d",
                     CameraArea::areAreasDifferent(mMeteringAreas, tempAreas));

        if ( (NO_ERROR == ret) && CameraArea::areAreasDifferent(mMeteringAreas, tempAreas) ) {
            mMeteringAreas.clear();
            mMeteringAreas = tempAreas;

            if ( MAX_METERING_AREAS >= mMeteringAreas.size() ) {
                CAMHAL_LOGDB("Setting Metering Areas %s",
                        params.get(CameraParameters::KEY_METERING_AREAS));

                mPending3Asettings |= SetMeteringAreas;
            } else {
                CAMHAL_LOGEB("Metering areas supported %d, metering areas set %d",
                             MAX_METERING_AREAS, mMeteringAreas.size());
                ret = -EINVAL;
            }
        }
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

int OMXCameraAdapter::getLUTvalue_HALtoOMX(const char * HalValue, LUTtype LUT)
{
    int LUTsize = LUT.size;
    if( HalValue )
        for(int i = 0; i < LUTsize; i++)
            if( 0 == strcmp(LUT.Table[i].userDefinition, HalValue) )
                return LUT.Table[i].omxDefinition;

    return -ENOENT;
}

const char* OMXCameraAdapter::getLUTvalue_OMXtoHAL(int OMXValue, LUTtype LUT)
{
    int LUTsize = LUT.size;
    for(int i = 0; i < LUTsize; i++)
        if( LUT.Table[i].omxDefinition == OMXValue )
            return LUT.Table[i].userDefinition;

    return NULL;
}

status_t OMXCameraAdapter::init3AParams(Gen3A_settings &Gen3A)
{
    LOG_FUNCTION_NAME;

    Gen3A.Effect = getLUTvalue_HALtoOMX(OMXCameraAdapter::DEFAULT_EFFECT, EffLUT);
    Gen3A.FlashMode = getLUTvalue_HALtoOMX(OMXCameraAdapter::DEFAULT_FLASH_MODE, FlashLUT);
    Gen3A.SceneMode = getLUTvalue_HALtoOMX(OMXCameraAdapter::DEFAULT_SCENE_MODE, SceneLUT);
    Gen3A.EVCompensation = atoi(OMXCameraAdapter::DEFAULT_EV_COMPENSATION);
    Gen3A.Focus = getLUTvalue_HALtoOMX(OMXCameraAdapter::DEFAULT_FOCUS_MODE, FocusLUT);
    Gen3A.ISO = getLUTvalue_HALtoOMX(OMXCameraAdapter::DEFAULT_ISO_MODE, IsoLUT);
    Gen3A.Flicker = getLUTvalue_HALtoOMX(OMXCameraAdapter::DEFAULT_ANTIBANDING, FlickerLUT);
    Gen3A.Brightness = atoi(OMXCameraAdapter::DEFAULT_BRIGHTNESS);
    Gen3A.Saturation = atoi(OMXCameraAdapter::DEFAULT_SATURATION) - SATURATION_OFFSET;
    Gen3A.Sharpness = atoi(OMXCameraAdapter::DEFAULT_SHARPNESS) - SHARPNESS_OFFSET;
    Gen3A.Contrast = atoi(OMXCameraAdapter::DEFAULT_CONTRAST) - CONTRAST_OFFSET;
    Gen3A.WhiteBallance = getLUTvalue_HALtoOMX(OMXCameraAdapter::DEFAULT_WB, WBalLUT);
    Gen3A.Exposure = getLUTvalue_HALtoOMX(OMXCameraAdapter::DEFAULT_EXPOSURE_MODE, ExpLUT);
    Gen3A.ExposureLock = OMX_FALSE;
    Gen3A.FocusLock = OMX_FALSE;
    Gen3A.WhiteBalanceLock = OMX_FALSE;

    LOG_FUNCTION_NAME_EXIT;

    return NO_ERROR;
}

status_t OMXCameraAdapter::setExposureMode(Gen3A_settings& Gen3A)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONFIG_EXPOSURECONTROLTYPE exp;

    LOG_FUNCTION_NAME;

    if ( OMX_StateInvalid == mComponentState )
        {
        CAMHAL_LOGEA("OMX component is in invalid state");
        return NO_INIT;
        }

    OMX_INIT_STRUCT_PTR (&exp, OMX_CONFIG_EXPOSURECONTROLTYPE);
    exp.nPortIndex = OMX_ALL;
    exp.eExposureControl = (OMX_EXPOSURECONTROLTYPE)Gen3A.Exposure;

    eError =  OMX_SetConfig(mCameraAdapterParameters.mHandleComp,
                            OMX_IndexConfigCommonExposure,
                            &exp);
    if ( OMX_ErrorNone != eError )
        {
        CAMHAL_LOGEB("Error while configuring exposure mode 0x%x", eError);
        }
    else
        {
        CAMHAL_LOGDA("Camera exposure mode configured successfully");
        }

    LOG_FUNCTION_NAME_EXIT;

    return ErrorUtils::omxToAndroidError(eError);
}

static bool isFlashDisabled() {
#if (PROPERTY_VALUE_MAX < 5)
#error "PROPERTY_VALUE_MAX must be at least 5"
#endif

    // Ignore flash_off system property for user build.
    char buildType[PROPERTY_VALUE_MAX];
    if (property_get("ro.build.type", buildType, NULL) &&
        !strcasecmp(buildType, "user")) {
        return false;
    }

    char value[PROPERTY_VALUE_MAX];
    if (property_get("camera.flash_off", value, NULL) &&
        (!strcasecmp(value, "true") || !strcasecmp(value, "1"))) {
        ALOGW("flash is disabled for testing purpose");
        return true;
    }

    return false;
}

status_t OMXCameraAdapter::setFlashMode(Gen3A_settings& Gen3A)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_IMAGE_PARAM_FLASHCONTROLTYPE flash;
    OMX_CONFIG_FOCUSASSISTTYPE focusAssist;

    LOG_FUNCTION_NAME;

    if ( OMX_StateInvalid == mComponentState )
        {
        CAMHAL_LOGEA("OMX component is in invalid state");
        return NO_INIT;
        }

    OMX_INIT_STRUCT_PTR (&flash, OMX_IMAGE_PARAM_FLASHCONTROLTYPE);
    flash.nPortIndex = OMX_ALL;

    if (isFlashDisabled()) {
        flash.eFlashControl = ( OMX_IMAGE_FLASHCONTROLTYPE ) OMX_IMAGE_FlashControlOff;
    } else {
        flash.eFlashControl = ( OMX_IMAGE_FLASHCONTROLTYPE ) Gen3A.FlashMode;
    }

    CAMHAL_LOGDB("Configuring flash mode 0x%x", flash.eFlashControl);
    eError =  OMX_SetConfig(mCameraAdapterParameters.mHandleComp,
                            (OMX_INDEXTYPE) OMX_IndexConfigFlashControl,
                            &flash);
    if ( OMX_ErrorNone != eError )
        {
        CAMHAL_LOGEB("Error while configuring flash mode 0x%x", eError);
        }
    else
        {
        CAMHAL_LOGDA("Camera flash mode configured successfully");
        }

    if ( OMX_ErrorNone == eError )
        {
        OMX_INIT_STRUCT_PTR (&focusAssist, OMX_CONFIG_FOCUSASSISTTYPE);
        focusAssist.nPortIndex = OMX_ALL;
        if ( flash.eFlashControl == OMX_IMAGE_FlashControlOff )
            {
            focusAssist.bFocusAssist = OMX_FALSE;
            }
        else
            {
            focusAssist.bFocusAssist = OMX_TRUE;
            }

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
        }

    LOG_FUNCTION_NAME_EXIT;

    return ErrorUtils::omxToAndroidError(eError);
}

status_t OMXCameraAdapter::getFlashMode(Gen3A_settings& Gen3A)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_IMAGE_PARAM_FLASHCONTROLTYPE flash;

    LOG_FUNCTION_NAME;

    if ( OMX_StateInvalid == mComponentState ) {
        CAMHAL_LOGEA("OMX component is in invalid state");
        return NO_INIT;
    }

    OMX_INIT_STRUCT_PTR (&flash, OMX_IMAGE_PARAM_FLASHCONTROLTYPE);
    flash.nPortIndex = OMX_ALL;

    eError =  OMX_GetConfig(mCameraAdapterParameters.mHandleComp,
                            (OMX_INDEXTYPE) OMX_IndexConfigFlashControl,
                            &flash);

    if ( OMX_ErrorNone != eError ) {
        CAMHAL_LOGEB("Error while getting flash mode 0x%x", eError);
    } else {
        Gen3A.FlashMode = flash.eFlashControl;
        CAMHAL_LOGDB("Gen3A.FlashMode 0x%x", Gen3A.FlashMode);
    }

    LOG_FUNCTION_NAME_EXIT;

    return ErrorUtils::omxToAndroidError(eError);
}

status_t OMXCameraAdapter::setFocusMode(Gen3A_settings& Gen3A)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_IMAGE_CONFIG_FOCUSCONTROLTYPE focus;
    size_t top, left, width, height, weight;
    OMX_CONFIG_BOOLEANTYPE bOMX;

    LOG_FUNCTION_NAME;

    BaseCameraAdapter::AdapterState state;
    BaseCameraAdapter::getState(state);

    if ( OMX_StateInvalid == mComponentState )
        {
        CAMHAL_LOGEA("OMX component is in invalid state");
        return NO_INIT;
        }


    ///Face detection takes precedence over touch AF
    if ( mFaceDetectionRunning )
        {
        //Disable region priority first
        setAlgoPriority(REGION_PRIORITY, FOCUS_ALGO, false);

        //Enable face algorithm priority for focus
        setAlgoPriority(FACE_PRIORITY, FOCUS_ALGO , true);

        //Do normal focus afterwards
        ////FIXME: Check if the extended focus control is needed? this overrides caf
        //focusControl.eFocusControl = ( OMX_IMAGE_FOCUSCONTROLTYPE ) OMX_IMAGE_FocusControlExtended;
        }
    else if ( (!mFocusAreas.isEmpty()) && (!mFocusAreas.itemAt(0)->isZeroArea()) )
        {

        //Disable face priority first
        setAlgoPriority(FACE_PRIORITY, FOCUS_ALGO, false);

        //Enable region algorithm priority
        setAlgoPriority(REGION_PRIORITY, FOCUS_ALGO, true);


        //Do normal focus afterwards
        //FIXME: Check if the extended focus control is needed? this overrides caf
        //focus.eFocusControl = ( OMX_IMAGE_FOCUSCONTROLTYPE ) OMX_IMAGE_FocusControlExtended;

        }
    else
        {

        //Disable both region and face priority
        setAlgoPriority(REGION_PRIORITY, FOCUS_ALGO, false);

        setAlgoPriority(FACE_PRIORITY, FOCUS_ALGO, false);

        }

    if ( NO_ERROR == ret && ((state & AF_ACTIVE) == 0) )
        {
        OMX_INIT_STRUCT_PTR (&bOMX, OMX_CONFIG_BOOLEANTYPE);

        if ( Gen3A.Focus == OMX_IMAGE_FocusControlAutoInfinity)
            {
            // Don't lock at infinity, otherwise the AF cannot drive
            // the lens at infinity position
            if( set3ALock(mUserSetExpLock, mUserSetWbLock, OMX_FALSE) != NO_ERROR)
                {
                CAMHAL_LOGEA("Error Applying 3A locks");
                } else {
                CAMHAL_LOGDA("Focus locked. Applied focus locks successfully");
                }
            }
        if ( Gen3A.Focus == OMX_IMAGE_FocusControlAuto ||
             Gen3A.Focus == OMX_IMAGE_FocusControlAutoInfinity)
            {
            // Run focus scanning if switching to continuous infinity focus mode
            bOMX.bEnabled = OMX_TRUE;
            }
        else
            {
            bOMX.bEnabled = OMX_FALSE;
            }

        eError = OMX_SetConfig(mCameraAdapterParameters.mHandleComp,
                               (OMX_INDEXTYPE)OMX_TI_IndexConfigAutofocusEnable,
                               &bOMX);

        OMX_INIT_STRUCT_PTR (&focus, OMX_IMAGE_CONFIG_FOCUSCONTROLTYPE);
        focus.nPortIndex = mCameraAdapterParameters.mPrevPortIndex;
        focus.eFocusControl = (OMX_IMAGE_FOCUSCONTROLTYPE)Gen3A.Focus;

        CAMHAL_LOGDB("Configuring focus mode 0x%x", focus.eFocusControl);
        eError = OMX_SetConfig( mCameraAdapterParameters.mHandleComp, OMX_IndexConfigFocusControl, &focus);
        if ( OMX_ErrorNone != eError )
            {
            CAMHAL_LOGEB("Error while configuring focus mode 0x%x", eError);
            }
        else
            {
            CAMHAL_LOGDA("Camera focus mode configured successfully");
            }
        }

    LOG_FUNCTION_NAME_EXIT;

    return ErrorUtils::omxToAndroidError(eError);
}

status_t OMXCameraAdapter::getFocusMode(Gen3A_settings& Gen3A)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_IMAGE_CONFIG_FOCUSCONTROLTYPE focus;
    size_t top, left, width, height, weight;

    LOG_FUNCTION_NAME;

    if (OMX_StateInvalid == mComponentState) {
        CAMHAL_LOGEA("OMX component is in invalid state");
        return NO_INIT;
    }

    OMX_INIT_STRUCT_PTR (&focus, OMX_IMAGE_CONFIG_FOCUSCONTROLTYPE);
    focus.nPortIndex = mCameraAdapterParameters.mPrevPortIndex;
    eError = OMX_GetConfig(mCameraAdapterParameters.mHandleComp,
                           OMX_IndexConfigFocusControl, &focus);

    if (OMX_ErrorNone != eError) {
        CAMHAL_LOGEB("Error while configuring focus mode 0x%x", eError);
    } else {
        Gen3A.Focus = focus.eFocusControl;
        CAMHAL_LOGDB("Gen3A.Focus 0x%x", Gen3A.Focus);
    }

    LOG_FUNCTION_NAME_EXIT;

    return ErrorUtils::omxToAndroidError(eError);
}

status_t OMXCameraAdapter::setScene(Gen3A_settings& Gen3A)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONFIG_SCENEMODETYPE scene;

    LOG_FUNCTION_NAME;

    if ( OMX_StateInvalid == mComponentState )
        {
        CAMHAL_LOGEA("OMX component is in invalid state");
        return NO_INIT;
        }

    OMX_INIT_STRUCT_PTR (&scene, OMX_CONFIG_SCENEMODETYPE);
    scene.nPortIndex = OMX_ALL;
    scene.eSceneMode = ( OMX_SCENEMODETYPE ) Gen3A.SceneMode;

    CAMHAL_LOGDB("Configuring scene mode 0x%x", scene.eSceneMode);
    eError =  OMX_SetConfig(mCameraAdapterParameters.mHandleComp,
                            ( OMX_INDEXTYPE ) OMX_TI_IndexConfigSceneMode,
                            &scene);

    if (OMX_ErrorNone != eError) {
        CAMHAL_LOGEB("Error while configuring scene mode 0x%x", eError);
    } else {
        CAMHAL_LOGDA("Camera scene configured successfully");
        if (Gen3A.SceneMode != OMX_Manual) {
            // Get preset scene mode feedback
            getFocusMode(Gen3A);
            getFlashMode(Gen3A);
            getWBMode(Gen3A);

            // TODO(XXX): Re-enable these for mainline
            // getSharpness(Gen3A);
            // getSaturation(Gen3A);
            // getISO(Gen3A);
        }
    }

    LOG_FUNCTION_NAME_EXIT;

    return ErrorUtils::omxToAndroidError(eError);
}

status_t OMXCameraAdapter::setEVCompensation(Gen3A_settings& Gen3A)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONFIG_EXPOSUREVALUETYPE expValues;

    LOG_FUNCTION_NAME;

    if ( OMX_StateInvalid == mComponentState )
        {
        CAMHAL_LOGEA("OMX component is in invalid state");
        return NO_INIT;
        }

    OMX_INIT_STRUCT_PTR (&expValues, OMX_CONFIG_EXPOSUREVALUETYPE);
    expValues.nPortIndex = mCameraAdapterParameters.mPrevPortIndex;

    OMX_GetConfig( mCameraAdapterParameters.mHandleComp,
                   OMX_IndexConfigCommonExposureValue,
                   &expValues);
    CAMHAL_LOGDB("old EV Compensation for OMX = 0x%x", (int)expValues.xEVCompensation);
    CAMHAL_LOGDB("EV Compensation for HAL = %d", Gen3A.EVCompensation);

    expValues.xEVCompensation = ( Gen3A.EVCompensation * ( 1 << Q16_OFFSET ) )  / 10;
    eError = OMX_SetConfig( mCameraAdapterParameters.mHandleComp,
                            OMX_IndexConfigCommonExposureValue,
                            &expValues);
    CAMHAL_LOGDB("new EV Compensation for OMX = 0x%x", (int)expValues.xEVCompensation);
    if ( OMX_ErrorNone != eError )
        {
        CAMHAL_LOGEB("Error while configuring EV Compensation 0x%x error = 0x%x",
                     ( unsigned int ) expValues.xEVCompensation,
                     eError);
        }
    else
        {
        CAMHAL_LOGDB("EV Compensation 0x%x configured successfully",
                     ( unsigned int ) expValues.xEVCompensation);
        }

    LOG_FUNCTION_NAME_EXIT;

    return ErrorUtils::omxToAndroidError(eError);
}

status_t OMXCameraAdapter::getEVCompensation(Gen3A_settings& Gen3A)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONFIG_EXPOSUREVALUETYPE expValues;

    LOG_FUNCTION_NAME;

    if ( OMX_StateInvalid == mComponentState ) {
        CAMHAL_LOGEA("OMX component is in invalid state");
        return NO_INIT;
    }

    OMX_INIT_STRUCT_PTR (&expValues, OMX_CONFIG_EXPOSUREVALUETYPE);
    expValues.nPortIndex = mCameraAdapterParameters.mPrevPortIndex;

    OMX_GetConfig(mCameraAdapterParameters.mHandleComp,
                  OMX_IndexConfigCommonExposureValue,
                  &expValues);

    if ( OMX_ErrorNone != eError ) {
        CAMHAL_LOGEB("Error while getting EV Compensation error = 0x%x", eError);
    } else {
        Gen3A.EVCompensation = (10 * expValues.xEVCompensation) / (1 << Q16_OFFSET);
        CAMHAL_LOGDB("Gen3A.EVCompensation 0x%x", Gen3A.EVCompensation);
    }

    LOG_FUNCTION_NAME_EXIT;

    return ErrorUtils::omxToAndroidError(eError);
}

status_t OMXCameraAdapter::setWBMode(Gen3A_settings& Gen3A)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONFIG_WHITEBALCONTROLTYPE wb;

    LOG_FUNCTION_NAME;

    if ( OMX_StateInvalid == mComponentState )
        {
        CAMHAL_LOGEA("OMX component is in invalid state");
        return NO_INIT;
        }

    OMX_INIT_STRUCT_PTR (&wb, OMX_CONFIG_WHITEBALCONTROLTYPE);
    wb.nPortIndex = mCameraAdapterParameters.mPrevPortIndex;
    wb.eWhiteBalControl = ( OMX_WHITEBALCONTROLTYPE ) Gen3A.WhiteBallance;

    if ( WB_FACE_PRIORITY == Gen3A.WhiteBallance )
        {
        //Disable Region priority and enable Face priority
        setAlgoPriority(REGION_PRIORITY, WHITE_BALANCE_ALGO, false);
        setAlgoPriority(FACE_PRIORITY, WHITE_BALANCE_ALGO, true);

        //Then set the mode to auto
        wb.eWhiteBalControl = OMX_WhiteBalControlAuto;
        }
    else
        {
        //Disable Face and Region priority
        setAlgoPriority(FACE_PRIORITY, WHITE_BALANCE_ALGO, false);
        setAlgoPriority(REGION_PRIORITY, WHITE_BALANCE_ALGO, false);
        }

    eError = OMX_SetConfig( mCameraAdapterParameters.mHandleComp,
                         OMX_IndexConfigCommonWhiteBalance,
                         &wb);
    if ( OMX_ErrorNone != eError )
        {
        CAMHAL_LOGEB("Error while configuring Whitebalance mode 0x%x error = 0x%x",
                     ( unsigned int ) wb.eWhiteBalControl,
                     eError);
        }
    else
        {
        CAMHAL_LOGDB("Whitebalance mode 0x%x configured successfully",
                     ( unsigned int ) wb.eWhiteBalControl);
        }

    LOG_FUNCTION_NAME_EXIT;

    return eError;
}

status_t OMXCameraAdapter::getWBMode(Gen3A_settings& Gen3A)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONFIG_WHITEBALCONTROLTYPE wb;

    LOG_FUNCTION_NAME;

    if ( OMX_StateInvalid == mComponentState ) {
        CAMHAL_LOGEA("OMX component is in invalid state");
        return NO_INIT;
    }

    OMX_INIT_STRUCT_PTR (&wb, OMX_CONFIG_WHITEBALCONTROLTYPE);
    wb.nPortIndex = mCameraAdapterParameters.mPrevPortIndex;

    eError = OMX_GetConfig(mCameraAdapterParameters.mHandleComp,
                           OMX_IndexConfigCommonWhiteBalance,
                           &wb);

    if (OMX_ErrorNone != eError) {
        CAMHAL_LOGEB("Error while getting Whitebalance mode error = 0x%x", eError);
    } else {
        Gen3A.WhiteBallance = wb.eWhiteBalControl;
        CAMHAL_LOGDB("Gen3A.WhiteBallance 0x%x", Gen3A.WhiteBallance);
    }

    LOG_FUNCTION_NAME_EXIT;

    return eError;
}

status_t OMXCameraAdapter::setFlicker(Gen3A_settings& Gen3A)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONFIG_FLICKERCANCELTYPE flicker;

    LOG_FUNCTION_NAME;

    if ( OMX_StateInvalid == mComponentState )
        {
        CAMHAL_LOGEA("OMX component is in invalid state");
        return NO_INIT;
        }

    OMX_INIT_STRUCT_PTR (&flicker, OMX_CONFIG_FLICKERCANCELTYPE);
    flicker.nPortIndex = mCameraAdapterParameters.mPrevPortIndex;
    flicker.eFlickerCancel = (OMX_COMMONFLICKERCANCELTYPE)Gen3A.Flicker;

    eError = OMX_SetConfig( mCameraAdapterParameters.mHandleComp,
                            (OMX_INDEXTYPE)OMX_IndexConfigFlickerCancel,
                            &flicker );
    if ( OMX_ErrorNone != eError )
        {
        CAMHAL_LOGEB("Error while configuring Flicker mode 0x%x error = 0x%x",
                     ( unsigned int ) flicker.eFlickerCancel,
                     eError);
        }
    else
        {
        CAMHAL_LOGDB("Flicker mode 0x%x configured successfully",
                     ( unsigned int ) flicker.eFlickerCancel);
        }

    LOG_FUNCTION_NAME_EXIT;

    return ErrorUtils::omxToAndroidError(eError);
}

status_t OMXCameraAdapter::setBrightness(Gen3A_settings& Gen3A)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONFIG_BRIGHTNESSTYPE brightness;

    LOG_FUNCTION_NAME;

    if ( OMX_StateInvalid == mComponentState )
        {
        CAMHAL_LOGEA("OMX component is in invalid state");
        return NO_INIT;
        }

    OMX_INIT_STRUCT_PTR (&brightness, OMX_CONFIG_BRIGHTNESSTYPE);
    brightness.nPortIndex = mCameraAdapterParameters.mPrevPortIndex;
    brightness.nBrightness = Gen3A.Brightness;

    eError = OMX_SetConfig( mCameraAdapterParameters.mHandleComp,
                         OMX_IndexConfigCommonBrightness,
                         &brightness);
    if ( OMX_ErrorNone != eError )
        {
        CAMHAL_LOGEB("Error while configuring Brightness 0x%x error = 0x%x",
                     ( unsigned int ) brightness.nBrightness,
                     eError);
        }
    else
        {
        CAMHAL_LOGDB("Brightness 0x%x configured successfully",
                     ( unsigned int ) brightness.nBrightness);
        }

    LOG_FUNCTION_NAME_EXIT;

    return ErrorUtils::omxToAndroidError(eError);
}

status_t OMXCameraAdapter::setContrast(Gen3A_settings& Gen3A)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONFIG_CONTRASTTYPE contrast;

    LOG_FUNCTION_NAME;

    if ( OMX_StateInvalid == mComponentState )
        {
        CAMHAL_LOGEA("OMX component is in invalid state");
        return NO_INIT;
        }

    OMX_INIT_STRUCT_PTR (&contrast, OMX_CONFIG_CONTRASTTYPE);
    contrast.nPortIndex = mCameraAdapterParameters.mPrevPortIndex;
    contrast.nContrast = Gen3A.Contrast;

    eError = OMX_SetConfig( mCameraAdapterParameters.mHandleComp,
                         OMX_IndexConfigCommonContrast,
                         &contrast);
    if ( OMX_ErrorNone != eError )
        {
        CAMHAL_LOGEB("Error while configuring Contrast 0x%x error = 0x%x",
                     ( unsigned int ) contrast.nContrast,
                     eError);
        }
    else
        {
        CAMHAL_LOGDB("Contrast 0x%x configured successfully",
                     ( unsigned int ) contrast.nContrast);
        }

    LOG_FUNCTION_NAME_EXIT;

    return eError;
}

status_t OMXCameraAdapter::setSharpness(Gen3A_settings& Gen3A)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_IMAGE_CONFIG_PROCESSINGLEVELTYPE procSharpness;

    LOG_FUNCTION_NAME;

    if ( OMX_StateInvalid == mComponentState )
        {
        CAMHAL_LOGEA("OMX component is in invalid state");
        return NO_INIT;
        }

    OMX_INIT_STRUCT_PTR (&procSharpness, OMX_IMAGE_CONFIG_PROCESSINGLEVELTYPE);
    procSharpness.nPortIndex = mCameraAdapterParameters.mPrevPortIndex;
    procSharpness.nLevel = Gen3A.Sharpness;

    if( procSharpness.nLevel == 0 )
        {
        procSharpness.bAuto = OMX_TRUE;
        }
    else
        {
        procSharpness.bAuto = OMX_FALSE;
        }

    eError = OMX_SetConfig( mCameraAdapterParameters.mHandleComp,
                         (OMX_INDEXTYPE)OMX_IndexConfigSharpeningLevel,
                         &procSharpness);
    if ( OMX_ErrorNone != eError )
        {
        CAMHAL_LOGEB("Error while configuring Sharpness 0x%x error = 0x%x",
                     ( unsigned int ) procSharpness.nLevel,
                     eError);
        }
    else
        {
        CAMHAL_LOGDB("Sharpness 0x%x configured successfully",
                     ( unsigned int ) procSharpness.nLevel);
        }

    LOG_FUNCTION_NAME_EXIT;

    return ErrorUtils::omxToAndroidError(eError);
}

status_t OMXCameraAdapter::getSharpness(Gen3A_settings& Gen3A)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_IMAGE_CONFIG_PROCESSINGLEVELTYPE procSharpness;

    LOG_FUNCTION_NAME;

    if (OMX_StateInvalid == mComponentState) {
        CAMHAL_LOGEA("OMX component is in invalid state");
        return NO_INIT;
    }

    OMX_INIT_STRUCT_PTR (&procSharpness, OMX_IMAGE_CONFIG_PROCESSINGLEVELTYPE);
    procSharpness.nPortIndex = mCameraAdapterParameters.mPrevPortIndex;

    eError = OMX_GetConfig(mCameraAdapterParameters.mHandleComp,
                           (OMX_INDEXTYPE)OMX_IndexConfigSharpeningLevel,
                           &procSharpness);

    if (OMX_ErrorNone != eError) {
        CAMHAL_LOGEB("Error while configuring Sharpness error = 0x%x", eError);
    } else {
        Gen3A.Sharpness = procSharpness.nLevel;
        CAMHAL_LOGDB("Gen3A.Sharpness 0x%x", Gen3A.Sharpness);
    }

    LOG_FUNCTION_NAME_EXIT;

    return ErrorUtils::omxToAndroidError(eError);
}

status_t OMXCameraAdapter::setSaturation(Gen3A_settings& Gen3A)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONFIG_SATURATIONTYPE saturation;

    LOG_FUNCTION_NAME;

    if ( OMX_StateInvalid == mComponentState )
        {
        CAMHAL_LOGEA("OMX component is in invalid state");
        return NO_INIT;
        }

    OMX_INIT_STRUCT_PTR (&saturation, OMX_CONFIG_SATURATIONTYPE);
    saturation.nPortIndex = mCameraAdapterParameters.mPrevPortIndex;
    saturation.nSaturation = Gen3A.Saturation;

    eError = OMX_SetConfig( mCameraAdapterParameters.mHandleComp,
                         OMX_IndexConfigCommonSaturation,
                         &saturation);
    if ( OMX_ErrorNone != eError )
        {
        CAMHAL_LOGEB("Error while configuring Saturation 0x%x error = 0x%x",
                     ( unsigned int ) saturation.nSaturation,
                     eError);
        }
    else
        {
        CAMHAL_LOGDB("Saturation 0x%x configured successfully",
                     ( unsigned int ) saturation.nSaturation);
        }

    LOG_FUNCTION_NAME_EXIT;

    return ErrorUtils::omxToAndroidError(eError);
}

status_t OMXCameraAdapter::getSaturation(Gen3A_settings& Gen3A)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONFIG_SATURATIONTYPE saturation;

    LOG_FUNCTION_NAME;

    if (OMX_StateInvalid == mComponentState) {
        CAMHAL_LOGEA("OMX component is in invalid state");
        return NO_INIT;
    }

    OMX_INIT_STRUCT_PTR (&saturation, OMX_CONFIG_SATURATIONTYPE);
    saturation.nPortIndex = mCameraAdapterParameters.mPrevPortIndex;

    eError = OMX_GetConfig( mCameraAdapterParameters.mHandleComp,
                         OMX_IndexConfigCommonSaturation,
                         &saturation);

    if (OMX_ErrorNone != eError) {
        CAMHAL_LOGEB("Error while getting Saturation error = 0x%x", eError);
    } else {
        Gen3A.Saturation = saturation.nSaturation;
        CAMHAL_LOGDB("Gen3A.Saturation 0x%x", Gen3A.Saturation);
    }

    LOG_FUNCTION_NAME_EXIT;

    return ErrorUtils::omxToAndroidError(eError);
}

status_t OMXCameraAdapter::setISO(Gen3A_settings& Gen3A)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONFIG_EXPOSUREVALUETYPE expValues;

    LOG_FUNCTION_NAME;

    if ( OMX_StateInvalid == mComponentState )
        {
        CAMHAL_LOGEA("OMX component is in invalid state");
        return NO_INIT;
        }

    OMX_INIT_STRUCT_PTR (&expValues, OMX_CONFIG_EXPOSUREVALUETYPE);
    expValues.nPortIndex = mCameraAdapterParameters.mPrevPortIndex;

    OMX_GetConfig( mCameraAdapterParameters.mHandleComp,
                   OMX_IndexConfigCommonExposureValue,
                   &expValues);

    if( 0 == Gen3A.ISO )
        {
        expValues.bAutoSensitivity = OMX_TRUE;
        }
    else
        {
        expValues.bAutoSensitivity = OMX_FALSE;
        expValues.nSensitivity = Gen3A.ISO;
        }

    eError = OMX_SetConfig( mCameraAdapterParameters.mHandleComp,
                         OMX_IndexConfigCommonExposureValue,
                         &expValues);
    if ( OMX_ErrorNone != eError )
        {
        CAMHAL_LOGEB("Error while configuring ISO 0x%x error = 0x%x",
                     ( unsigned int ) expValues.nSensitivity,
                     eError);
        }
    else
        {
        CAMHAL_LOGDB("ISO 0x%x configured successfully",
                     ( unsigned int ) expValues.nSensitivity);
        }

    LOG_FUNCTION_NAME_EXIT;

    return ErrorUtils::omxToAndroidError(eError);
}

status_t OMXCameraAdapter::getISO(Gen3A_settings& Gen3A)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONFIG_EXPOSUREVALUETYPE expValues;

    LOG_FUNCTION_NAME;

    if (OMX_StateInvalid == mComponentState) {
        CAMHAL_LOGEA("OMX component is in invalid state");
        return NO_INIT;
    }

    OMX_INIT_STRUCT_PTR (&expValues, OMX_CONFIG_EXPOSUREVALUETYPE);
    expValues.nPortIndex = mCameraAdapterParameters.mPrevPortIndex;

    OMX_GetConfig( mCameraAdapterParameters.mHandleComp,
                   OMX_IndexConfigCommonExposureValue,
                   &expValues);

    if (OMX_ErrorNone != eError) {
        CAMHAL_LOGEB("Error while getting ISO error = 0x%x", eError);
    } else {
        Gen3A.ISO = expValues.nSensitivity;
        CAMHAL_LOGDB("Gen3A.ISO %d", Gen3A.ISO);
    }

    LOG_FUNCTION_NAME_EXIT;

    return ErrorUtils::omxToAndroidError(eError);
}

status_t OMXCameraAdapter::setEffect(Gen3A_settings& Gen3A)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONFIG_IMAGEFILTERTYPE effect;

    LOG_FUNCTION_NAME;

    if ( OMX_StateInvalid == mComponentState )
        {
        CAMHAL_LOGEA("OMX component is in invalid state");
        return NO_INIT;
        }

    OMX_INIT_STRUCT_PTR (&effect, OMX_CONFIG_IMAGEFILTERTYPE);
    effect.nPortIndex = mCameraAdapterParameters.mPrevPortIndex;
    effect.eImageFilter = (OMX_IMAGEFILTERTYPE ) Gen3A.Effect;

    eError = OMX_SetConfig( mCameraAdapterParameters.mHandleComp,
                         OMX_IndexConfigCommonImageFilter,
                         &effect);
    if ( OMX_ErrorNone != eError )
        {
        CAMHAL_LOGEB("Error while configuring Effect 0x%x error = 0x%x",
                     ( unsigned int )  effect.eImageFilter,
                     eError);
        }
    else
        {
        CAMHAL_LOGDB("Effect 0x%x configured successfully",
                     ( unsigned int )  effect.eImageFilter);
        }

    LOG_FUNCTION_NAME_EXIT;

    return ErrorUtils::omxToAndroidError(eError);
}

status_t OMXCameraAdapter::setWhiteBalanceLock(Gen3A_settings& Gen3A)
{
  OMX_ERRORTYPE eError = OMX_ErrorNone;
  OMX_IMAGE_CONFIG_LOCKTYPE lock;

  LOG_FUNCTION_NAME

  if ( OMX_StateInvalid == mComponentState )
    {
      CAMHAL_LOGEA("OMX component is in invalid state");
      return NO_INIT;
    }

  OMX_INIT_STRUCT_PTR (&lock, OMX_IMAGE_CONFIG_LOCKTYPE);
  lock.nPortIndex = mCameraAdapterParameters.mPrevPortIndex;
  lock.bLock = Gen3A.WhiteBalanceLock;
  eError = OMX_SetConfig( mCameraAdapterParameters.mHandleComp,
                          (OMX_INDEXTYPE)OMX_IndexConfigImageWhiteBalanceLock,
                          &lock);
  if ( OMX_ErrorNone != eError )
    {
      CAMHAL_LOGEB("Error while configuring WhiteBalance Lock error = 0x%x", eError);
    }
  else
    {
      CAMHAL_LOGDB("WhiteBalance Lock configured successfully %d ", lock.bLock);
    }
  LOG_FUNCTION_NAME_EXIT

  return ErrorUtils::omxToAndroidError(eError);
}

status_t OMXCameraAdapter::setExposureLock(Gen3A_settings& Gen3A)
{
  OMX_ERRORTYPE eError = OMX_ErrorNone;
  OMX_IMAGE_CONFIG_LOCKTYPE lock;

  LOG_FUNCTION_NAME

  if ( OMX_StateInvalid == mComponentState )
    {
      CAMHAL_LOGEA("OMX component is in invalid state");
      return NO_INIT;
    }

  OMX_INIT_STRUCT_PTR (&lock, OMX_IMAGE_CONFIG_LOCKTYPE);
  lock.nPortIndex = mCameraAdapterParameters.mPrevPortIndex;
  lock.bLock = Gen3A.ExposureLock;
  eError = OMX_SetConfig( mCameraAdapterParameters.mHandleComp,
                          (OMX_INDEXTYPE)OMX_IndexConfigImageExposureLock,
                          &lock);
  if ( OMX_ErrorNone != eError )
    {
      CAMHAL_LOGEB("Error while configuring Exposure Lock error = 0x%x", eError);
    }
  else
    {
      CAMHAL_LOGDB("Exposure Lock configured successfully %d ", lock.bLock);
    }
  LOG_FUNCTION_NAME_EXIT

    return ErrorUtils::omxToAndroidError(eError);
}

status_t OMXCameraAdapter::setFocusLock(Gen3A_settings& Gen3A)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_IMAGE_CONFIG_LOCKTYPE lock;

    LOG_FUNCTION_NAME

    if ( OMX_StateInvalid == mComponentState ) {
        CAMHAL_LOGEA("OMX component is in invalid state");
        return NO_INIT;
    }

    OMX_INIT_STRUCT_PTR (&lock, OMX_IMAGE_CONFIG_LOCKTYPE);
    lock.nPortIndex = mCameraAdapterParameters.mPrevPortIndex;

    lock.bLock = Gen3A.FocusLock;
    eError = OMX_SetConfig(mCameraAdapterParameters.mHandleComp,
                           (OMX_INDEXTYPE)OMX_IndexConfigImageFocusLock,
                           &lock);

    if ( OMX_ErrorNone != eError ) {
        CAMHAL_LOGEB("Error while configuring Focus Lock error = 0x%x", eError);
    } else {
        CAMHAL_LOGDB("Focus Lock configured successfully %d ", lock.bLock);
    }

    LOG_FUNCTION_NAME_EXIT

    return ErrorUtils::omxToAndroidError(eError);
}

status_t OMXCameraAdapter::set3ALock(OMX_BOOL toggleExp, OMX_BOOL toggleWb, OMX_BOOL toggleFocus)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_IMAGE_CONFIG_LOCKTYPE lock;

    LOG_FUNCTION_NAME

    if ( OMX_StateInvalid == mComponentState )
    {
      CAMHAL_LOGEA("OMX component is in invalid state");
      return NO_INIT;
    }

    OMX_INIT_STRUCT_PTR (&lock, OMX_IMAGE_CONFIG_LOCKTYPE);
    lock.nPortIndex = mCameraAdapterParameters.mPrevPortIndex;

    mParameters3A.ExposureLock = toggleExp;
    mParameters3A.FocusLock = toggleFocus;
    mParameters3A.WhiteBalanceLock = toggleWb;

    eError = OMX_GetConfig( mCameraAdapterParameters.mHandleComp,
                            (OMX_INDEXTYPE)OMX_IndexConfigImageExposureLock,
                            &lock);

    if ( OMX_ErrorNone != eError )
    {
        CAMHAL_LOGEB("Error GetConfig Exposure Lock error = 0x%x", eError);
        goto EXIT;
    }
    else
    {
        const char *lock_state_exp = toggleExp ? TRUE : FALSE;
        CAMHAL_LOGDA("Exposure Lock GetConfig successfull");

        /* Apply locks only when not applied already */
        if ( lock.bLock  != toggleExp )
        {
            setExposureLock(mParameters3A);
        }

        mParams.set(CameraParameters::KEY_AUTO_EXPOSURE_LOCK, lock_state_exp);
    }

    OMX_INIT_STRUCT_PTR (&lock, OMX_IMAGE_CONFIG_LOCKTYPE);
    lock.nPortIndex = mCameraAdapterParameters.mPrevPortIndex;
    eError = OMX_GetConfig( mCameraAdapterParameters.mHandleComp,
                            (OMX_INDEXTYPE)OMX_IndexConfigImageFocusLock,
                            &lock);

    if ( OMX_ErrorNone != eError )
    {
        CAMHAL_LOGEB("Error GetConfig Focus Lock error = 0x%x", eError);
        goto EXIT;
    }
    else
    {
        CAMHAL_LOGDB("Focus Lock GetConfig successfull bLock(%d)", lock.bLock);

        /* Apply locks only when not applied already */
        if ( lock.bLock  != toggleFocus )
        {
            setFocusLock(mParameters3A);
        }
    }

    OMX_INIT_STRUCT_PTR (&lock, OMX_IMAGE_CONFIG_LOCKTYPE);
    lock.nPortIndex = mCameraAdapterParameters.mPrevPortIndex;
    eError = OMX_GetConfig( mCameraAdapterParameters.mHandleComp,
                            (OMX_INDEXTYPE)OMX_IndexConfigImageWhiteBalanceLock,
                            &lock);

    if ( OMX_ErrorNone != eError )
    {
        CAMHAL_LOGEB("Error GetConfig WhiteBalance Lock error = 0x%x", eError);
        goto EXIT;
    }
    else
    {
        const char *lock_state_wb = toggleWb ? TRUE : FALSE;
        CAMHAL_LOGDA("WhiteBalance Lock GetConfig successfull");

        /* Apply locks only when not applied already */
        if ( lock.bLock != toggleWb )
        {
            setWhiteBalanceLock(mParameters3A);
        }

        mParams.set(CameraParameters::KEY_AUTO_WHITEBALANCE_LOCK, lock_state_wb);
    }
 EXIT:
    return ErrorUtils::omxToAndroidError(eError);
}

status_t OMXCameraAdapter::setMeteringAreas(Gen3A_settings& Gen3A)
{
  status_t ret = NO_ERROR;
  OMX_ERRORTYPE eError = OMX_ErrorNone;

  OMX_ALGOAREASTYPE **meteringAreas;
  OMX_TI_CONFIG_SHAREDBUFFER sharedBuffer;
  MemoryManager memMgr;
  int areasSize = 0;

  LOG_FUNCTION_NAME

  Mutex::Autolock lock(mMeteringAreasLock);

  if ( OMX_StateInvalid == mComponentState )
    {
      CAMHAL_LOGEA("OMX component is in invalid state");
      return NO_INIT;
    }

  areasSize = ((sizeof(OMX_ALGOAREASTYPE)+4095)/4096)*4096;
  meteringAreas = (OMX_ALGOAREASTYPE**) memMgr.allocateBuffer(0, 0, NULL, areasSize, 1);

  OMXCameraPortParameters * mPreviewData = NULL;
  mPreviewData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mPrevPortIndex];

  if (!meteringAreas)
      {
      CAMHAL_LOGEB("Error allocating buffer for metering areas %d", eError);
      return -ENOMEM;
      }

  OMX_INIT_STRUCT_PTR (meteringAreas[0], OMX_ALGOAREASTYPE);

  meteringAreas[0]->nPortIndex = OMX_ALL;
  meteringAreas[0]->nNumAreas = mMeteringAreas.size();
  meteringAreas[0]->nAlgoAreaPurpose = OMX_AlgoAreaExposure;

  for ( unsigned int n = 0; n < mMeteringAreas.size(); n++)
      {
      // transform the coordinates to 3A-type coordinates
      mMeteringAreas.itemAt(n)->transfrom((size_t)mPreviewData->mWidth,
                                      (size_t)mPreviewData->mHeight,
                                      (size_t&)meteringAreas[0]->tAlgoAreas[n].nTop,
                                      (size_t&)meteringAreas[0]->tAlgoAreas[n].nLeft,
                                      (size_t&)meteringAreas[0]->tAlgoAreas[n].nWidth,
                                      (size_t&)meteringAreas[0]->tAlgoAreas[n].nHeight);

      meteringAreas[0]->tAlgoAreas[n].nLeft =
              ( meteringAreas[0]->tAlgoAreas[n].nLeft * METERING_AREAS_RANGE ) / mPreviewData->mWidth;
      meteringAreas[0]->tAlgoAreas[n].nTop =
              ( meteringAreas[0]->tAlgoAreas[n].nTop* METERING_AREAS_RANGE ) / mPreviewData->mHeight;
      meteringAreas[0]->tAlgoAreas[n].nWidth =
              ( meteringAreas[0]->tAlgoAreas[n].nWidth * METERING_AREAS_RANGE ) / mPreviewData->mWidth;
      meteringAreas[0]->tAlgoAreas[n].nHeight =
              ( meteringAreas[0]->tAlgoAreas[n].nHeight * METERING_AREAS_RANGE ) / mPreviewData->mHeight;

      meteringAreas[0]->tAlgoAreas[n].nPriority = mMeteringAreas.itemAt(n)->getWeight();

      CAMHAL_LOGDB("Metering area %d : top = %d left = %d width = %d height = %d prio = %d",
              n, (int)meteringAreas[0]->tAlgoAreas[n].nTop, (int)meteringAreas[0]->tAlgoAreas[n].nLeft,
              (int)meteringAreas[0]->tAlgoAreas[n].nWidth, (int)meteringAreas[0]->tAlgoAreas[n].nHeight,
              (int)meteringAreas[0]->tAlgoAreas[n].nPriority);

      }

  OMX_INIT_STRUCT_PTR (&sharedBuffer, OMX_TI_CONFIG_SHAREDBUFFER);

  sharedBuffer.nPortIndex = OMX_ALL;
  sharedBuffer.nSharedBuffSize = areasSize;
  sharedBuffer.pSharedBuff = (OMX_U8 *) meteringAreas[0];

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
  else
      {
      CAMHAL_LOGDA("Metering Areas SetConfig successfull.");
      }

 EXIT:
  if (NULL != meteringAreas)
      {
      memMgr.freeBuffer((void*) meteringAreas);
      meteringAreas = NULL;
      }

  return ret;
}

status_t OMXCameraAdapter::apply3Asettings( Gen3A_settings& Gen3A )
{
    status_t ret = NO_ERROR;
    unsigned int currSett; // 32 bit
    int portIndex;

    LOG_FUNCTION_NAME;

    Mutex::Autolock lock(m3ASettingsUpdateLock);

    /*
     * Scenes have a priority during the process
     * of applying 3A related parameters.
     * They can override pretty much all other 3A
     * settings and similarly get overridden when
     * for instance the focus mode gets switched.
     * There is only one exception to this rule,
     * the manual a.k.a. auto scene.
     */
    if (SetSceneMode & mPending3Asettings) {
        mPending3Asettings &= ~SetSceneMode;
        ret |= setScene(Gen3A);
        // re-apply EV compensation after setting scene mode since it probably reset it
        if(Gen3A.EVCompensation) {
            setEVCompensation(Gen3A);
        }
        return ret;
    } else if (OMX_Manual != Gen3A.SceneMode) {
        // only certain settings are allowed when scene mode is set
        mPending3Asettings &= (SetEVCompensation | SetFocus | SetWBLock |
                               SetExpLock | SetWhiteBallance | SetFlash);
        if ( mPending3Asettings == 0 ) return NO_ERROR;
    }

    for( currSett = 1; currSett < E3aSettingMax; currSett <<= 1)
        {
        if( currSett & mPending3Asettings )
            {
            switch( currSett )
                {
                case SetEVCompensation:
                    {
                    ret |= setEVCompensation(Gen3A);
                    break;
                    }

                case SetWhiteBallance:
                    {
                    ret |= setWBMode(Gen3A);
                    break;
                    }

                case SetFlicker:
                    {
                    ret |= setFlicker(Gen3A);
                    break;
                    }

                case SetBrightness:
                    {
                    ret |= setBrightness(Gen3A);
                    break;
                    }

                case SetContrast:
                    {
                    ret |= setContrast(Gen3A);
                    break;
                    }

                case SetSharpness:
                    {
                    ret |= setSharpness(Gen3A);
                    break;
                    }

                case SetSaturation:
                    {
                    ret |= setSaturation(Gen3A);
                    break;
                    }

                case SetISO:
                    {
                    ret |= setISO(Gen3A);
                    break;
                    }

                case SetEffect:
                    {
                    ret |= setEffect(Gen3A);
                    break;
                    }

                case SetFocus:
                    {
                    ret |= setFocusMode(Gen3A);
                    break;
                    }

                case SetExpMode:
                    {
                    ret |= setExposureMode(Gen3A);
                    break;
                    }

                case SetFlash:
                    {
                    ret |= setFlashMode(Gen3A);
                    break;
                    }

                case SetExpLock:
                  {
                    ret |= setExposureLock(Gen3A);
                    break;
                  }

                case SetWBLock:
                  {
                    ret |= setWhiteBalanceLock(Gen3A);
                    break;
                  }
                case SetMeteringAreas:
                  {
                    ret |= setMeteringAreas(Gen3A);
                  }
                  break;
                default:
                    CAMHAL_LOGEB("this setting (0x%x) is still not supported in CameraAdapter ",
                                 currSett);
                    break;
                }
                mPending3Asettings &= ~currSett;
            }
        }

        LOG_FUNCTION_NAME_EXIT;

        return ret;
}

};
