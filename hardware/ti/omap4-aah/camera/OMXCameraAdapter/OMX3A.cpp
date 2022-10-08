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

#include "CameraHal.h"
#include "OMXCameraAdapter.h"
#include "ErrorUtils.h"

#include <cutils/properties.h>

#define METERING_AREAS_RANGE 0xFF

static const char PARAM_SEP[] = ",";

namespace Ti {
namespace Camera {

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

status_t OMXCameraAdapter::setParameters3A(const android::CameraParameters &params,
                                           BaseCameraAdapter::AdapterState state)
{
    status_t ret = NO_ERROR;
    int mode = 0;
    const char *str = NULL;
    int varint = 0;
    BaseCameraAdapter::AdapterState nextState;
    BaseCameraAdapter::getNextState(nextState);

    LOG_FUNCTION_NAME;

    android::AutoMutex lock(m3ASettingsUpdateLock);

    str = params.get(android::CameraParameters::KEY_SCENE_MODE);
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
    if ( (str = params.get(TICameraParameters::KEY_EXPOSURE_MODE)) != NULL ) {
        mode = getLUTvalue_HALtoOMX(str, ExpLUT);
        if ( mParameters3A.Exposure != mode ) {
            // If either the new or the old exposure mode is manual set also
            // the SetManualExposure flag to call setManualExposureVal where
            // the auto gain and exposure flags are configured
            if ( mParameters3A.Exposure == OMX_ExposureControlOff ||
                 mode == OMX_ExposureControlOff ) {
                mPending3Asettings |= SetManualExposure;
            }
            mParameters3A.Exposure = mode;
            CAMHAL_LOGDB("Exposure mode %d", mode);
            if ( 0 <= mParameters3A.Exposure ) {
                mPending3Asettings |= SetExpMode;
            }
        }
        if ( mode == OMX_ExposureControlOff ) {
            mode = params.getInt(TICameraParameters::KEY_MANUAL_EXPOSURE);
            if ( mParameters3A.ManualExposure != mode ) {
                mParameters3A.ManualExposure = mode;
                CAMHAL_LOGDB("Manual Exposure = %d", mode);
                mPending3Asettings |= SetManualExposure;
            }
            mode = params.getInt(TICameraParameters::KEY_MANUAL_EXPOSURE_RIGHT);
            if ( mParameters3A.ManualExposureRight != mode ) {
                mParameters3A.ManualExposureRight = mode;
                CAMHAL_LOGDB("Manual Exposure right = %d", mode);
                mPending3Asettings |= SetManualExposure;
            }
            mode = params.getInt(TICameraParameters::KEY_MANUAL_GAIN_ISO);
            if ( mParameters3A.ManualGain != mode ) {
                mParameters3A.ManualGain = mode;
                CAMHAL_LOGDB("Manual Gain = %d", mode);
                mPending3Asettings |= SetManualExposure;
            }
            mode = params.getInt(TICameraParameters::KEY_MANUAL_GAIN_ISO_RIGHT);
            if ( mParameters3A.ManualGainRight != mode ) {
                mParameters3A.ManualGainRight = mode;
                CAMHAL_LOGDB("Manual Gain right = %d", mode);
                mPending3Asettings |= SetManualExposure;
            }
        }
    }
#endif

    str = params.get(android::CameraParameters::KEY_WHITE_BALANCE);
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

    str = params.get(android::CameraParameters::KEY_ANTIBANDING);
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

    str = params.get(android::CameraParameters::KEY_FOCUS_MODE);
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

    str = params.get(android::CameraParameters::KEY_EXPOSURE_COMPENSATION);
    varint = params.getInt(android::CameraParameters::KEY_EXPOSURE_COMPENSATION);
    if ( mFirstTimeInit || (str && (mParameters3A.EVCompensation != varint))) {
        CAMHAL_LOGDB("Setting EV Compensation to %d", varint);
        mParameters3A.EVCompensation = varint;
        mPending3Asettings |= SetEVCompensation;
        }

    str = params.get(android::CameraParameters::KEY_FLASH_MODE);
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
            mParameters3A.FlashMode = OMX_IMAGE_FlashControlAuto;
            }
        }

    CAMHAL_LOGVB("Flash Setting %s", str);
    CAMHAL_LOGVB("FlashMode %d", mParameters3A.FlashMode);

    str = params.get(android::CameraParameters::KEY_EFFECT);
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

    str = params.get(android::CameraParameters::KEY_AUTO_EXPOSURE_LOCK_SUPPORTED);
    if ( (str != NULL) && (!strcmp(str, android::CameraParameters::TRUE)) )
      {
        OMX_BOOL lock = OMX_FALSE;
        mUserSetExpLock = OMX_FALSE;
        str = params.get(android::CameraParameters::KEY_AUTO_EXPOSURE_LOCK);
        if (str && ((strcmp(str, android::CameraParameters::TRUE)) == 0))
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

    str = params.get(android::CameraParameters::KEY_AUTO_WHITEBALANCE_LOCK_SUPPORTED);
    if ( (str != NULL) && (!strcmp(str, android::CameraParameters::TRUE)) )
      {
        OMX_BOOL lock = OMX_FALSE;
        mUserSetWbLock = OMX_FALSE;
        str = params.get(android::CameraParameters::KEY_AUTO_WHITEBALANCE_LOCK);
        if (str && ((strcmp(str, android::CameraParameters::TRUE)) == 0))
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
    if (str && (strcmp(str, android::CameraParameters::TRUE) == 0) && (mParameters3A.FocusLock != OMX_TRUE)) {
        CAMHAL_LOGVA("Locking Focus");
        mParameters3A.FocusLock = OMX_TRUE;
        setFocusLock(mParameters3A);
    } else if (str && (strcmp(str, android::CameraParameters::FALSE) == 0) && (mParameters3A.FocusLock != OMX_FALSE)) {
        CAMHAL_LOGVA("UnLocking Focus");
        mParameters3A.FocusLock = OMX_FALSE;
        setFocusLock(mParameters3A);
    }

    str = params.get(android::CameraParameters::KEY_METERING_AREAS);
    if ( (str != NULL) ) {
        size_t MAX_METERING_AREAS;
        android::Vector<android::sp<CameraArea> > tempAreas;

        MAX_METERING_AREAS = atoi(params.get(android::CameraParameters::KEY_MAX_NUM_METERING_AREAS));

        android::AutoMutex lock(mMeteringAreasLock);

        ret = CameraArea::parseAreas(str, ( strlen(str) + 1 ), tempAreas);

        CAMHAL_LOGVB("areAreasDifferent? = %d",
                     CameraArea::areAreasDifferent(mMeteringAreas, tempAreas));

        if ( (NO_ERROR == ret) && CameraArea::areAreasDifferent(mMeteringAreas, tempAreas) ) {
            mMeteringAreas.clear();
            mMeteringAreas = tempAreas;

            if ( MAX_METERING_AREAS >= mMeteringAreas.size() ) {
                CAMHAL_LOGDB("Setting Metering Areas %s",
                        params.get(android::CameraParameters::KEY_METERING_AREAS));

                mPending3Asettings |= SetMeteringAreas;
            } else {
                CAMHAL_LOGEB("Metering areas supported %d, metering areas set %d",
                             MAX_METERING_AREAS, mMeteringAreas.size());
                ret = -EINVAL;
            }
        }
    }

// TI extensions for enable/disable algos
    declareParameter3ABool(params, TICameraParameters::KEY_ALGO_EXTERNAL_GAMMA,
                       mParameters3A.AlgoExternalGamma, SetAlgoExternalGamma, "External Gamma");
    declareParameter3ABool(params, TICameraParameters::KEY_ALGO_NSF1,
                       mParameters3A.AlgoNSF1, SetAlgoNSF1, "NSF1");
    declareParameter3ABool(params, TICameraParameters::KEY_ALGO_NSF2,
                       mParameters3A.AlgoNSF2, SetAlgoNSF2, "NSF2");
    declareParameter3ABool(params, TICameraParameters::KEY_ALGO_SHARPENING,
                       mParameters3A.AlgoSharpening, SetAlgoSharpening, "Sharpening");
    declareParameter3ABool(params, TICameraParameters::KEY_ALGO_THREELINCOLORMAP,
                       mParameters3A.AlgoThreeLinColorMap, SetAlgoThreeLinColorMap, "ThreeLinColorMap");
    declareParameter3ABool(params, TICameraParameters::KEY_ALGO_GIC, mParameters3A.AlgoGIC, SetAlgoGIC, "GIC");

    // Gamma table
    str = params.get(TICameraParameters::KEY_GAMMA_TABLE);
    updateGammaTable(str);

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

void OMXCameraAdapter::updateGammaTable(const char* gamma)
{
    unsigned int plane = 0;
    unsigned int i = 0;
    bool gamma_changed = false;
    const char *a = gamma;
    OMX_TI_GAMMATABLE_ELEM_TYPE *elem[3] = { mParameters3A.mGammaTable.pR,
                                             mParameters3A.mGammaTable.pG,
                                             mParameters3A.mGammaTable.pB};

    if (!gamma) return;

    mPending3Asettings &= ~SetGammaTable;
    memset(&mParameters3A.mGammaTable, 0, sizeof(mParameters3A.mGammaTable));
    for (plane = 0; plane < 3; plane++) {
        a = strchr(a, '(');
        if (NULL != a) {
            a++;
            for (i = 0; i < OMX_TI_GAMMATABLE_SIZE; i++) {
                char *b;
                int newVal;
                newVal = strtod(a, &b);
                if (newVal != elem[plane][i].nOffset) {
                    elem[plane][i].nOffset = newVal;
                    gamma_changed = true;
                }
                a = strpbrk(b, ",:)");
                if ((NULL != a) && (':' == *a)) {
                    a++;
                } else if ((NULL != a) && (',' == *a)){
                    a++;
                    break;
                } else if ((NULL != a) && (')' == *a)){
                    a++;
                    break;
                } else {
                    CAMHAL_LOGE("Error while parsing values");
                    gamma_changed = false;
                    break;
                }
                newVal = strtod(a, &b);
                if (newVal != elem[plane][i].nSlope) {
                    elem[plane][i].nSlope = newVal;
                    gamma_changed = true;
                }
                a = strpbrk(b, ",:)");
                if ((NULL != a) && (',' == *a)) {
                    a++;
                } else if ((NULL != a) && (':' == *a)){
                    a++;
                    break;
                } else if ((NULL != a) && (')' == *a)){
                    a++;
                    break;
                } else {
                    CAMHAL_LOGE("Error while parsing values");
                    gamma_changed = false;
                    break;
                }
            }
            if ((OMX_TI_GAMMATABLE_SIZE - 1) != i) {
                CAMHAL_LOGE("Error while parsing values (incorrect count %u)", i);
                gamma_changed = false;
                break;
            }
        } else {
            CAMHAL_LOGE("Error while parsing planes (%u)", plane);
            gamma_changed = false;
            break;
        }
    }

    if (gamma_changed) {
        mPending3Asettings |= SetGammaTable;
    }
}

void OMXCameraAdapter::declareParameter3ABool(const android::CameraParameters &params, const char *key,
                                              OMX_BOOL &current_setting, E3ASettingsFlags pending,
                                              const char *msg)
{
    OMX_BOOL val = OMX_TRUE;
    const char *str = params.get(key);

    if (str && ((strcmp(str, android::CameraParameters::FALSE)) == 0))
        {
        CAMHAL_LOGVB("Disabling %s", msg);
        val = OMX_FALSE;
        }
    else
        {
        CAMHAL_LOGVB("Enabling %s", msg);
        }
    if (current_setting != val)
        {
        current_setting = val;
        CAMHAL_LOGDB("%s %s", msg, current_setting ? "enabled" : "disabled");
        mPending3Asettings |= pending;
        }
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

int OMXCameraAdapter::getMultipleLUTvalue_OMXtoHAL(int OMXValue, LUTtype LUT, char * supported)
{
    int num = 0;
    int remaining_size;
    int LUTsize = LUT.size;
    for(int i = 0; i < LUTsize; i++)
        if( LUT.Table[i].omxDefinition == OMXValue )
        {
            num++;
            if (supported[0] != '\0') {
                strncat(supported, PARAM_SEP, 1);
            }
            remaining_size = ((((int)MAX_PROP_VALUE_LENGTH - 1 - (int)strlen(supported)) < 0) ? 0 : (MAX_PROP_VALUE_LENGTH - 1 - strlen(supported)));
            strncat(supported, LUT.Table[i].userDefinition, remaining_size);
        }

    return num;
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

    return Utils::ErrorUtils::omxToAndroidError(eError);
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
        (!strcasecmp(value, android::CameraParameters::TRUE) || !strcasecmp(value, "1"))) {
        CAMHAL_LOGW("flash is disabled for testing purpose");
        return true;
    }

    return false;
}

status_t OMXCameraAdapter::setManualExposureVal(Gen3A_settings& Gen3A) {
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONFIG_EXPOSUREVALUETYPE expVal;
    OMX_TI_CONFIG_EXPOSUREVALUERIGHTTYPE expValRight;

    LOG_FUNCTION_NAME;

    if ( OMX_StateInvalid == mComponentState ) {
        CAMHAL_LOGEA("OMX component is in invalid state");
        return NO_INIT;
    }

    OMX_INIT_STRUCT_PTR (&expVal, OMX_CONFIG_EXPOSUREVALUETYPE);
    OMX_INIT_STRUCT_PTR (&expValRight, OMX_TI_CONFIG_EXPOSUREVALUERIGHTTYPE);
    expVal.nPortIndex = OMX_ALL;
    expValRight.nPortIndex = OMX_ALL;

    eError = OMX_GetConfig(mCameraAdapterParameters.mHandleComp,
                   OMX_IndexConfigCommonExposureValue,
                   &expVal);
    if ( OMX_ErrorNone == eError ) {
        eError = OMX_GetConfig(mCameraAdapterParameters.mHandleComp,
                       (OMX_INDEXTYPE) OMX_TI_IndexConfigRightExposureValue,
                       &expValRight);
    }
    if ( OMX_ErrorNone != eError ) {
        CAMHAL_LOGEB("OMX_GetConfig error 0x%x (manual exposure values)", eError);
        return Utils::ErrorUtils::omxToAndroidError(eError);
    }

    if ( Gen3A.Exposure != OMX_ExposureControlOff ) {
        expVal.bAutoShutterSpeed = OMX_TRUE;
        expVal.bAutoSensitivity = OMX_TRUE;
    } else {
        expVal.bAutoShutterSpeed = OMX_FALSE;
        expVal.nShutterSpeedMsec = Gen3A.ManualExposure;
        expValRight.nShutterSpeedMsec = Gen3A.ManualExposureRight;
        if ( Gen3A.ManualGain <= 0 || Gen3A.ManualGainRight <= 0 ) {
            expVal.bAutoSensitivity = OMX_TRUE;
        } else {
            expVal.bAutoSensitivity = OMX_FALSE;
            expVal.nSensitivity = Gen3A.ManualGain;
            expValRight.nSensitivity = Gen3A.ManualGainRight;
        }
    }

    eError = OMX_SetConfig(mCameraAdapterParameters.mHandleComp,
                            OMX_IndexConfigCommonExposureValue,
                            &expVal);
    if ( OMX_ErrorNone == eError ) {
        eError = OMX_SetConfig(mCameraAdapterParameters.mHandleComp,
                                (OMX_INDEXTYPE) OMX_TI_IndexConfigRightExposureValue,
                                &expValRight);
    }

    if ( OMX_ErrorNone != eError ) {
        CAMHAL_LOGEB("Error 0x%x while configuring manual exposure values", eError);
    } else {
        CAMHAL_LOGDA("Camera manual exposure values configured successfully");
    }

    LOG_FUNCTION_NAME_EXIT;

    return Utils::ErrorUtils::omxToAndroidError(eError);
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

    return Utils::ErrorUtils::omxToAndroidError(eError);
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

    return Utils::ErrorUtils::omxToAndroidError(eError);
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

    return Utils::ErrorUtils::omxToAndroidError(eError);
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

    return Utils::ErrorUtils::omxToAndroidError(eError);
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

    return Utils::ErrorUtils::omxToAndroidError(eError);
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

    return Utils::ErrorUtils::omxToAndroidError(eError);
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

    return Utils::ErrorUtils::omxToAndroidError(eError);
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

    // disable face and region priorities
    setAlgoPriority(FACE_PRIORITY, WHITE_BALANCE_ALGO, false);
    setAlgoPriority(REGION_PRIORITY, WHITE_BALANCE_ALGO, false);

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

    return Utils::ErrorUtils::omxToAndroidError(eError);
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

    return Utils::ErrorUtils::omxToAndroidError(eError);
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

    return Utils::ErrorUtils::omxToAndroidError(eError);
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

    return Utils::ErrorUtils::omxToAndroidError(eError);
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

    return Utils::ErrorUtils::omxToAndroidError(eError);
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

    return Utils::ErrorUtils::omxToAndroidError(eError);
}

status_t OMXCameraAdapter::setISO(Gen3A_settings& Gen3A)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONFIG_EXPOSUREVALUETYPE expValues;
    OMX_TI_CONFIG_EXPOSUREVALUERIGHTTYPE expValRight;

    LOG_FUNCTION_NAME;

    if ( OMX_StateInvalid == mComponentState )
        {
        CAMHAL_LOGEA("OMX component is in invalid state");
        return NO_INIT;
        }

    // In case of manual exposure Gain is applied from setManualExposureVal
    if ( Gen3A.Exposure == OMX_ExposureControlOff ) {
        return NO_ERROR;
    }

    OMX_INIT_STRUCT_PTR (&expValues, OMX_CONFIG_EXPOSUREVALUETYPE);
    OMX_INIT_STRUCT_PTR (&expValRight, OMX_TI_CONFIG_EXPOSUREVALUERIGHTTYPE);
    expValues.nPortIndex = mCameraAdapterParameters.mPrevPortIndex;
    expValRight.nPortIndex = mCameraAdapterParameters.mPrevPortIndex;

    eError = OMX_GetConfig( mCameraAdapterParameters.mHandleComp,
                    OMX_IndexConfigCommonExposureValue,
                    &expValues);

    if ( OMX_ErrorNone == eError ) {
        eError = OMX_GetConfig(mCameraAdapterParameters.mHandleComp,
                        (OMX_INDEXTYPE) OMX_TI_IndexConfigRightExposureValue,
                        &expValRight);
    }

    if ( OMX_ErrorNone != eError ) {
        CAMHAL_LOGEB("OMX_GetConfig error 0x%x (manual exposure values)", eError);
        return Utils::ErrorUtils::omxToAndroidError(eError);
    }

    if( 0 == Gen3A.ISO ) {
        expValues.bAutoSensitivity = OMX_TRUE;
    } else {
        expValues.bAutoSensitivity = OMX_FALSE;
        expValues.nSensitivity = Gen3A.ISO;
        expValRight.nSensitivity = expValues.nSensitivity;
    }

    eError = OMX_SetConfig( mCameraAdapterParameters.mHandleComp,
                            OMX_IndexConfigCommonExposureValue,
                            &expValues);

    if ( OMX_ErrorNone == eError ) {
        eError = OMX_SetConfig(mCameraAdapterParameters.mHandleComp,
                            (OMX_INDEXTYPE) OMX_TI_IndexConfigRightExposureValue,
                            &expValRight);
    }
    if ( OMX_ErrorNone != eError ) {
        CAMHAL_LOGEB("Error while configuring ISO 0x%x error = 0x%x",
                     ( unsigned int ) expValues.nSensitivity,
                     eError);
    } else {
        CAMHAL_LOGDB("ISO 0x%x configured successfully",
                     ( unsigned int ) expValues.nSensitivity);
    }

    LOG_FUNCTION_NAME_EXIT;

    return Utils::ErrorUtils::omxToAndroidError(eError);
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

    return Utils::ErrorUtils::omxToAndroidError(eError);
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

    return Utils::ErrorUtils::omxToAndroidError(eError);
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

  return Utils::ErrorUtils::omxToAndroidError(eError);
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

    return Utils::ErrorUtils::omxToAndroidError(eError);
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

    return Utils::ErrorUtils::omxToAndroidError(eError);
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
        CAMHAL_LOGDA("Exposure Lock GetConfig successfull");

        /* Apply locks only when not applied already */
        if ( lock.bLock  != toggleExp )
        {
            setExposureLock(mParameters3A);
        }

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
        CAMHAL_LOGDA("WhiteBalance Lock GetConfig successfull");

        /* Apply locks only when not applied already */
        if ( lock.bLock != toggleWb )
        {
            setWhiteBalanceLock(mParameters3A);
        }

    }
 EXIT:
    return Utils::ErrorUtils::omxToAndroidError(eError);
}

status_t OMXCameraAdapter::setMeteringAreas(Gen3A_settings& Gen3A)
{
  status_t ret = NO_ERROR;
  OMX_ERRORTYPE eError = OMX_ErrorNone;

  CameraBuffer *bufferlist;
  OMX_ALGOAREASTYPE *meteringAreas;
  OMX_TI_CONFIG_SHAREDBUFFER sharedBuffer;
  int areasSize = 0;

  LOG_FUNCTION_NAME

  android::AutoMutex lock(mMeteringAreasLock);

  if ( OMX_StateInvalid == mComponentState )
    {
      CAMHAL_LOGEA("OMX component is in invalid state");
      return NO_INIT;
    }

  areasSize = ((sizeof(OMX_ALGOAREASTYPE)+4095)/4096)*4096;
  bufferlist = mMemMgr.allocateBufferList(0, 0, NULL, areasSize, 1);
  meteringAreas = (OMX_ALGOAREASTYPE *)bufferlist[0].opaque;

  OMXCameraPortParameters * mPreviewData = NULL;
  mPreviewData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mPrevPortIndex];

  if (!meteringAreas)
      {
      CAMHAL_LOGEB("Error allocating buffer for metering areas %d", eError);
      return -ENOMEM;
      }

  OMX_INIT_STRUCT_PTR (meteringAreas, OMX_ALGOAREASTYPE);

  meteringAreas->nPortIndex = OMX_ALL;
  meteringAreas->nNumAreas = mMeteringAreas.size();
  meteringAreas->nAlgoAreaPurpose = OMX_AlgoAreaExposure;

  for ( unsigned int n = 0; n < mMeteringAreas.size(); n++)
      {
        int widthDivisor = 1;
        int heightDivisor = 1;

        if (mPreviewData->mFrameLayoutType == OMX_TI_StereoFrameLayoutTopBottom) {
            heightDivisor = 2;
        }
        if (mPreviewData->mFrameLayoutType == OMX_TI_StereoFrameLayoutLeftRight) {
            widthDivisor = 2;
        }

      // transform the coordinates to 3A-type coordinates
      mMeteringAreas.itemAt(n)->transfrom((size_t)mPreviewData->mWidth/widthDivisor,
                                      (size_t)mPreviewData->mHeight/heightDivisor,
                                      (size_t&)meteringAreas->tAlgoAreas[n].nTop,
                                      (size_t&)meteringAreas->tAlgoAreas[n].nLeft,
                                      (size_t&)meteringAreas->tAlgoAreas[n].nWidth,
                                      (size_t&)meteringAreas->tAlgoAreas[n].nHeight);

      meteringAreas->tAlgoAreas[n].nLeft =
              ( meteringAreas->tAlgoAreas[n].nLeft * METERING_AREAS_RANGE ) / mPreviewData->mWidth;
      meteringAreas->tAlgoAreas[n].nTop =
              ( meteringAreas->tAlgoAreas[n].nTop* METERING_AREAS_RANGE ) / mPreviewData->mHeight;
      meteringAreas->tAlgoAreas[n].nWidth =
              ( meteringAreas->tAlgoAreas[n].nWidth * METERING_AREAS_RANGE ) / mPreviewData->mWidth;
      meteringAreas->tAlgoAreas[n].nHeight =
              ( meteringAreas->tAlgoAreas[n].nHeight * METERING_AREAS_RANGE ) / mPreviewData->mHeight;

      meteringAreas->tAlgoAreas[n].nPriority = mMeteringAreas.itemAt(n)->getWeight();

      CAMHAL_LOGDB("Metering area %d : top = %d left = %d width = %d height = %d prio = %d",
              n, (int)meteringAreas->tAlgoAreas[n].nTop, (int)meteringAreas->tAlgoAreas[n].nLeft,
              (int)meteringAreas->tAlgoAreas[n].nWidth, (int)meteringAreas->tAlgoAreas[n].nHeight,
              (int)meteringAreas->tAlgoAreas[n].nPriority);

      }

  OMX_INIT_STRUCT_PTR (&sharedBuffer, OMX_TI_CONFIG_SHAREDBUFFER);

  sharedBuffer.nPortIndex = OMX_ALL;
  sharedBuffer.nSharedBuffSize = areasSize;
  sharedBuffer.pSharedBuff = (OMX_U8 *)camera_buffer_get_omx_ptr (&bufferlist[0]);

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
  if (NULL != bufferlist)
      {
      mMemMgr.freeBufferList(bufferlist);
      }

  return ret;
}

//TI extensions for enable/disable algos
status_t OMXCameraAdapter::setParameter3ABoolInvert(const OMX_INDEXTYPE omx_idx,
                                                    const OMX_BOOL data, const char *msg)
{
    OMX_BOOL inv_data;

    if (OMX_TRUE == data)
        {
        inv_data = OMX_FALSE;
        }
    else if (OMX_FALSE == data)
        {
        inv_data = OMX_TRUE;
        }
    else
        {
        return BAD_VALUE;
        }
    return setParameter3ABool(omx_idx, inv_data, msg);
}

status_t OMXCameraAdapter::setParameter3ABool(const OMX_INDEXTYPE omx_idx,
                                              const OMX_BOOL data, const char *msg)
{
  OMX_ERRORTYPE eError = OMX_ErrorNone;
  OMX_CONFIG_BOOLEANTYPE cfgdata;

  LOG_FUNCTION_NAME

  if ( OMX_StateInvalid == mComponentState )
    {
      CAMHAL_LOGEA("OMX component is in invalid state");
      return NO_INIT;
    }

  OMX_INIT_STRUCT_PTR (&cfgdata, OMX_CONFIG_BOOLEANTYPE);
  cfgdata.bEnabled = data;
  eError = OMX_SetConfig( mCameraAdapterParameters.mHandleComp,
                          omx_idx,
                          &cfgdata);
  if ( OMX_ErrorNone != eError )
    {
      CAMHAL_LOGEB("Error while configuring %s error = 0x%x", msg, eError);
    }
  else
    {
      CAMHAL_LOGDB("%s configured successfully %d ", msg, cfgdata.bEnabled);
    }

  LOG_FUNCTION_NAME_EXIT

  return Utils::ErrorUtils::omxToAndroidError(eError);
}

status_t OMXCameraAdapter::setAlgoExternalGamma(Gen3A_settings& Gen3A)
{
    return setParameter3ABool((OMX_INDEXTYPE) OMX_TI_IndexConfigExternalGamma, Gen3A.AlgoExternalGamma, "External Gamma");
}

status_t OMXCameraAdapter::setAlgoNSF1(Gen3A_settings& Gen3A)
{
    return setParameter3ABoolInvert((OMX_INDEXTYPE) OMX_TI_IndexConfigDisableNSF1, Gen3A.AlgoNSF1, "NSF1");
}

status_t OMXCameraAdapter::setAlgoNSF2(Gen3A_settings& Gen3A)
{
    return setParameter3ABoolInvert((OMX_INDEXTYPE) OMX_TI_IndexConfigDisableNSF2, Gen3A.AlgoNSF2, "NSF2");
}

status_t OMXCameraAdapter::setAlgoSharpening(Gen3A_settings& Gen3A)
{
    return setParameter3ABoolInvert((OMX_INDEXTYPE) OMX_TI_IndexConfigDisableSharpening, Gen3A.AlgoSharpening, "Sharpening");
}

status_t OMXCameraAdapter::setAlgoThreeLinColorMap(Gen3A_settings& Gen3A)
{
    return setParameter3ABoolInvert((OMX_INDEXTYPE) OMX_TI_IndexConfigDisableThreeLinColorMap, Gen3A.AlgoThreeLinColorMap, "Color Conversion");
}

status_t OMXCameraAdapter::setAlgoGIC(Gen3A_settings& Gen3A)
{
    return setParameter3ABoolInvert((OMX_INDEXTYPE) OMX_TI_IndexConfigDisableGIC, Gen3A.AlgoGIC, "Green Inballance Correction");
}

status_t OMXCameraAdapter::setGammaTable(Gen3A_settings& Gen3A)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    CameraBuffer *bufferlist = NULL;
    OMX_TI_CONFIG_GAMMATABLE_TYPE *gammaTable = NULL;
    OMX_TI_CONFIG_SHAREDBUFFER sharedBuffer;
    int tblSize = 0;

    LOG_FUNCTION_NAME;

    if ( OMX_StateInvalid == mComponentState ) {
        CAMHAL_LOGEA("OMX component is in invalid state");
        ret = NO_INIT;
        goto EXIT;
    }

    tblSize = ((sizeof(OMX_TI_CONFIG_GAMMATABLE_TYPE)+4095)/4096)*4096;
    bufferlist = mMemMgr.allocateBufferList(0, 0, NULL, tblSize, 1);
    if (NULL == bufferlist) {
        CAMHAL_LOGEB("Error allocating buffer for gamma table");
        ret =  NO_MEMORY;
        goto EXIT;
    }
    gammaTable = (OMX_TI_CONFIG_GAMMATABLE_TYPE *)bufferlist[0].mapped;
    if (NULL == gammaTable) {
        CAMHAL_LOGEB("Error allocating buffer for gamma table (wrong data pointer)");
        ret =  NO_MEMORY;
        goto EXIT;
    }

    memcpy(gammaTable, &mParameters3A.mGammaTable, sizeof(OMX_TI_CONFIG_GAMMATABLE_TYPE));

#ifdef CAMERAHAL_DEBUG
    {
        android::String8 DmpR;
        android::String8 DmpG;
        android::String8 DmpB;
        for (unsigned int i=0; i<OMX_TI_GAMMATABLE_SIZE;i++) {
            DmpR.appendFormat(" %d:%d;", (int)gammaTable->pR[i].nOffset, (int)(int)gammaTable->pR[i].nSlope);
            DmpG.appendFormat(" %d:%d;", (int)gammaTable->pG[i].nOffset, (int)(int)gammaTable->pG[i].nSlope);
            DmpB.appendFormat(" %d:%d;", (int)gammaTable->pB[i].nOffset, (int)(int)gammaTable->pB[i].nSlope);
        }
        CAMHAL_LOGE("Gamma table R:%s", DmpR.string());
        CAMHAL_LOGE("Gamma table G:%s", DmpG.string());
        CAMHAL_LOGE("Gamma table B:%s", DmpB.string());
    }
#endif

    OMX_INIT_STRUCT_PTR (&sharedBuffer, OMX_TI_CONFIG_SHAREDBUFFER);
    sharedBuffer.nPortIndex = OMX_ALL;
    sharedBuffer.nSharedBuffSize = sizeof(OMX_TI_CONFIG_GAMMATABLE_TYPE);
    sharedBuffer.pSharedBuff = (OMX_U8 *)camera_buffer_get_omx_ptr (&bufferlist[0]);
    if ( NULL == sharedBuffer.pSharedBuff ) {
        CAMHAL_LOGEA("No resources to allocate OMX shared buffer");
        ret = NO_MEMORY;
        goto EXIT;
    }

    eError =  OMX_SetConfig(mCameraAdapterParameters.mHandleComp,
                            (OMX_INDEXTYPE) OMX_TI_IndexConfigGammaTable, &sharedBuffer);
    if ( OMX_ErrorNone != eError ) {
        CAMHAL_LOGEB("Error while setting Gamma Table configuration 0x%x", eError);
        ret = BAD_VALUE;
        goto EXIT;
    } else {
        CAMHAL_LOGDA("Gamma Table SetConfig successfull.");
    }

EXIT:

    if (NULL != bufferlist) {
        mMemMgr.freeBufferList(bufferlist);
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::apply3Asettings( Gen3A_settings& Gen3A )
{
    status_t ret = NO_ERROR;
    unsigned int currSett; // 32 bit
    int portIndex;

    LOG_FUNCTION_NAME;

    android::AutoMutex lock(m3ASettingsUpdateLock);

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

                case SetManualExposure: {
                    ret |= setManualExposureVal(Gen3A);
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

                //TI extensions for enable/disable algos
                case SetAlgoExternalGamma:
                  {
                    ret |= setAlgoExternalGamma(Gen3A);
                  }
                  break;

                case SetAlgoNSF1:
                  {
                    ret |= setAlgoNSF1(Gen3A);
                  }
                  break;

                case SetAlgoNSF2:
                  {
                    ret |= setAlgoNSF2(Gen3A);
                  }
                  break;

                case SetAlgoSharpening:
                  {
                    ret |= setAlgoSharpening(Gen3A);
                  }
                  break;

                case SetAlgoThreeLinColorMap:
                  {
                    ret |= setAlgoThreeLinColorMap(Gen3A);
                  }
                  break;

                case SetAlgoGIC:
                  {
                    ret |= setAlgoGIC(Gen3A);
                  }
                  break;

                case SetGammaTable:
                  {
                    ret |= setGammaTable(Gen3A);
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

} // namespace Camera
} // namespace Ti
