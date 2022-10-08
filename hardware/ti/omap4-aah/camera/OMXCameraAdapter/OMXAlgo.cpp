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
* @file OMXAlgo.cpp
*
* This file contains functionality for handling algorithm configurations.
*
*/

#include "CameraHal.h"
#include "OMXCameraAdapter.h"
#include "ErrorUtils.h"

#undef TRUE

namespace Ti {
namespace Camera {

status_t OMXCameraAdapter::setParametersAlgo(const android::CameraParameters &params,
                                             BaseCameraAdapter::AdapterState state)
{
    status_t ret = NO_ERROR;
    const char *valstr = NULL;
    const char *valManualStr = NULL;
    const char *oldstr = NULL;
    OMXCameraPortParameters *cap;
    BrightnessMode gbce = BRIGHTNESS_OFF;
    BrightnessMode glbce = BRIGHTNESS_OFF;

    LOG_FUNCTION_NAME;

    CaptureMode capMode;
    CAMHAL_LOGDB("Capture mode %s",  params.get(TICameraParameters::KEY_CAP_MODE));
    if ( (valstr = params.get(TICameraParameters::KEY_CAP_MODE)) != NULL ) {
        if (strcmp(valstr, (const char *) TICameraParameters::HIGH_PERFORMANCE_MODE) == 0) {
            capMode = OMXCameraAdapter::HIGH_SPEED;
            mCapabilitiesOpMode = MODE_HIGH_SPEED;
        } else if (strcmp(valstr, (const char *) TICameraParameters::EXPOSURE_BRACKETING) == 0) {
            capMode = OMXCameraAdapter::HIGH_SPEED;
            mCapabilitiesOpMode = MODE_HIGH_SPEED;
        } else if (strcmp(valstr, (const char *) TICameraParameters::ZOOM_BRACKETING) == 0) {
            capMode = OMXCameraAdapter::HIGH_SPEED;
            mCapabilitiesOpMode = MODE_HIGH_SPEED;
        } else if (strcmp(valstr, (const char *) TICameraParameters::HIGH_QUALITY_MODE) == 0) {
            capMode = OMXCameraAdapter::HIGH_QUALITY;
            mCapabilitiesOpMode = MODE_HIGH_QUALITY;
        } else if (strcmp(valstr, (const char *) TICameraParameters::HIGH_QUALITY_ZSL_MODE) == 0) {
            capMode = OMXCameraAdapter::HIGH_QUALITY_ZSL;
            mCapabilitiesOpMode = MODE_ZEROSHUTTERLAG;
        } else if (strcmp(valstr, (const char *) TICameraParameters::VIDEO_MODE) == 0) {
            capMode = OMXCameraAdapter::VIDEO_MODE;
            mCapabilitiesOpMode = MODE_VIDEO;
        } else if (strcmp(valstr, (const char *) TICameraParameters::VIDEO_MODE_HQ) == 0) {
              capMode = OMXCameraAdapter::VIDEO_MODE_HQ;
              mCapabilitiesOpMode = MODE_VIDEO_HIGH_QUALITY;
        } else if (strcmp(valstr, (const char *) TICameraParameters::CP_CAM_MODE) == 0) {
            capMode = OMXCameraAdapter::CP_CAM;
            mCapabilitiesOpMode = MODE_CPCAM;
        } else if (strcmp(valstr, (const char *) TICameraParameters::TEMP_BRACKETING) == 0) {
            capMode = OMXCameraAdapter::HIGH_SPEED;
            mCapabilitiesOpMode = MODE_HIGH_SPEED;
        } else {
            capMode = OMXCameraAdapter::HIGH_QUALITY;
            mCapabilitiesOpMode = MODE_HIGH_QUALITY;
        }

    } else {
        capMode = OMXCameraAdapter::HIGH_QUALITY;
        mCapabilitiesOpMode = MODE_HIGH_QUALITY;
    }

    if ( mSensorIndex == 2 ) {
       mCapabilitiesOpMode = MODE_STEREO;
    }

    if ( mCapMode != capMode ) {
        mCapMode = capMode;
        mOMXStateSwitch = true;
        mPendingPreviewSettings |= SetCapMode;
    }

    CAMHAL_LOGDB("Capture Mode set %d", mCapMode);

    /// Configure IPP, LDCNSF, GBCE and GLBCE only in HQ mode
    IPPMode ipp;
    if((mCapMode == OMXCameraAdapter::HIGH_QUALITY) || (mCapMode == OMXCameraAdapter::HIGH_QUALITY_ZSL)
            || (mCapMode == OMXCameraAdapter::VIDEO_MODE)
            || (mCapMode == OMXCameraAdapter::CP_CAM))
        {
          if ( (valstr = params.get(TICameraParameters::KEY_IPP)) != NULL )
            {
            if (strcmp(valstr, (const char *) TICameraParameters::IPP_LDCNSF) == 0)
                {
                ipp = OMXCameraAdapter::IPP_LDCNSF;
                }
            else if (strcmp(valstr, (const char *) TICameraParameters::IPP_LDC) == 0)
                {
                ipp = OMXCameraAdapter::IPP_LDC;
                }
            else if (strcmp(valstr, (const char *) TICameraParameters::IPP_NSF) == 0)
                {
                ipp = OMXCameraAdapter::IPP_NSF;
                }
            else if (strcmp(valstr, (const char *) TICameraParameters::IPP_NONE) == 0)
                {
                ipp = OMXCameraAdapter::IPP_NONE;
                }
            else
                {
                ipp = OMXCameraAdapter::IPP_NONE;
                }
            }
        else
            {
            ipp = OMXCameraAdapter::IPP_NONE;
            }

        CAMHAL_LOGVB("IPP Mode set %d", ipp);

        if (((valstr = params.get(TICameraParameters::KEY_GBCE)) != NULL) ) {
            if (strcmp(valstr, android::CameraParameters::TRUE ) == 0) {
                gbce = BRIGHTNESS_ON;
            } else {
                gbce = BRIGHTNESS_OFF;
            }

            if ( gbce != mGBCE ) {
                mGBCE = gbce;
                setGBCE(mGBCE);
            }

        } else if(mFirstTimeInit) {
            //Disable GBCE by default
            setGBCE(OMXCameraAdapter::BRIGHTNESS_OFF);
        }

        if ( ( valstr = params.get(TICameraParameters::KEY_GLBCE) ) != NULL ) {

            if (strcmp(valstr, android::CameraParameters::TRUE) == 0) {
                glbce = BRIGHTNESS_ON;
            } else {
                glbce = BRIGHTNESS_OFF;
            }

            if ( glbce != mGLBCE ) {
                mGLBCE = glbce;
                setGLBCE(mGLBCE);
            }

        } else if(mFirstTimeInit) {
            //Disable GLBCE by default
            setGLBCE(OMXCameraAdapter::BRIGHTNESS_OFF);
        }

    } else {
        ipp = OMXCameraAdapter::IPP_NONE;
    }

    if ( mIPP != ipp )
        {
        mIPP = ipp;
        mOMXStateSwitch = true;
        mPendingPreviewSettings |= SetLDC;
        mPendingPreviewSettings |= SetNSF;
        }

    ///Set VNF Configuration
    bool vnfEnabled = false;
    valstr = params.get(TICameraParameters::KEY_VNF);
    if (valstr && strcmp(valstr, android::CameraParameters::TRUE) == 0)
        {
        CAMHAL_LOGDA("VNF Enabled");
        vnfEnabled = true;
        }
    else
        {
        CAMHAL_LOGDA("VNF Disabled");
        vnfEnabled = false;
        }

    if ( mVnfEnabled != vnfEnabled )
        {
        mVnfEnabled = vnfEnabled;
        mOMXStateSwitch = true;
        mPendingPreviewSettings |= SetVNF;
        }

    ///Set VSTAB Configuration
    bool vstabEnabled = false;
    valstr = params.get(android::CameraParameters::KEY_VIDEO_STABILIZATION);
    if (valstr && strcmp(valstr, android::CameraParameters::TRUE) == 0) {
        CAMHAL_LOGDA("VSTAB Enabled");
        vstabEnabled = true;
        }
    else
        {
        CAMHAL_LOGDA("VSTAB Disabled");
        vstabEnabled = false;
        }

    if ( mVstabEnabled != vstabEnabled )
        {
        mVstabEnabled = vstabEnabled;
        mOMXStateSwitch = true;
        mPendingPreviewSettings |= SetVSTAB;
        }

    //A work-around for a failing call to OMX flush buffers
    if ( ( capMode = OMXCameraAdapter::VIDEO_MODE ) &&
         ( mVstabEnabled ) )
        {
        mOMXStateSwitch = true;
        }

#ifdef OMAP_ENHANCEMENT

    //Set Auto Convergence Mode
    valstr = params.get((const char *) TICameraParameters::KEY_AUTOCONVERGENCE_MODE);
    valManualStr = params.get(TICameraParameters::KEY_MANUAL_CONVERGENCE);

    cap = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mPrevPortIndex];

    if (cap->mFrameLayoutType != OMX_TI_StereoFrameLayout2D) {
        if ((valstr != NULL) || (valManualStr != NULL)) {
            setAutoConvergence(valstr, valManualStr, params);
            if (valstr != NULL) {
                CAMHAL_LOGDB("AutoConvergenceMode %s", valstr);
            }
            if (valManualStr != NULL) {
                CAMHAL_LOGDB("Manual Convergence  %s", valManualStr);
            }
        }

        //Set Mechanical Misalignment Correction
        valstr = params.get(TICameraParameters::KEY_MECHANICAL_MISALIGNMENT_CORRECTION);
        if ( valstr != NULL ) {
            setMechanicalMisalignmentCorrection(strcmp(valstr, android::CameraParameters::TRUE) == 0);
            CAMHAL_LOGDB("Mechanical Misalignment Correction %s", valstr);
        }
    }

#endif

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

// Set AutoConvergence
status_t OMXCameraAdapter::setAutoConvergence(const char *pValstr, const char *pValManualstr, const android::CameraParameters &params)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_TI_CONFIG_CONVERGENCETYPE ACParams;
    const char *str = NULL;
    android::Vector<android::sp<CameraArea> > tempAreas;
    int mode;
    int changed = 0;

    LOG_FUNCTION_NAME;

    if ( pValManualstr != NULL ) {
        OMX_S32 manualConvergence = (OMX_S32)strtol(pValManualstr ,0 ,0);

        if (mManualConv != manualConvergence) {
            mManualConv = manualConvergence;
            changed = 1;
        }
    }

    if ( pValstr != NULL ) {
        mode = getLUTvalue_HALtoOMX(pValstr, mAutoConvergenceLUT);

        if ( NAME_NOT_FOUND == mode ) {
            CAMHAL_LOGEB("Wrong convergence mode: %s", pValstr);
            LOG_FUNCTION_NAME_EXIT;
            return mode;
        }

        if ( mAutoConv != static_cast<OMX_TI_AUTOCONVERGENCEMODETYPE> (mode) ) {
            mAutoConv = static_cast<OMX_TI_AUTOCONVERGENCEMODETYPE> (mode);
            changed = 1;
        }
    }

    if ( OMX_TI_AutoConvergenceModeFocusFaceTouch == mAutoConv ) {
        android::AutoMutex lock(mTouchAreasLock);

        str = params.get(android::CameraParameters::KEY_METERING_AREAS);

        if ( NULL != str ) {
            ret = CameraArea::parseAreas(str, ( strlen(str) + 1 ), tempAreas);
        } else {
            CAMHAL_LOGEB("Touch areas not received in %s",
                         android::CameraParameters::KEY_METERING_AREAS);
            LOG_FUNCTION_NAME_EXIT;
            return BAD_VALUE;
        }

        if ( CameraArea::areAreasDifferent(mTouchAreas, tempAreas) ) {
            mTouchAreas.clear();
            mTouchAreas = tempAreas;
            changed = 1;
        }
    }

    if (!changed) {
        LOG_FUNCTION_NAME_EXIT;
        return NO_ERROR;
    }

    OMXCameraPortParameters * mPreviewData;
    mPreviewData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mPrevPortIndex];

    ACParams.nSize = (OMX_U32)sizeof(OMX_TI_CONFIG_CONVERGENCETYPE);
    ACParams.nVersion = mLocalVersionParam;
    ACParams.nPortIndex = mCameraAdapterParameters.mPrevPortIndex;

    OMX_GetConfig(mCameraAdapterParameters.mHandleComp,
                  (OMX_INDEXTYPE)OMX_TI_IndexConfigAutoConvergence,
                  &ACParams);

    ACParams.eACMode = mAutoConv;
    ACParams.nManualConverence = mManualConv;

    if (1 == mTouchAreas.size()) {
        int widthDivisor = 1;
        int heightDivisor = 1;

        if (mPreviewData->mFrameLayoutType == OMX_TI_StereoFrameLayoutTopBottom) {
            heightDivisor = 2;
        }
        if (mPreviewData->mFrameLayoutType == OMX_TI_StereoFrameLayoutLeftRight) {
            widthDivisor = 2;
        }

        // transform the coordinates to 3A-type coordinates
        mTouchAreas.itemAt(0)->transfrom((size_t)mPreviewData->mWidth/widthDivisor,
                                         (size_t)mPreviewData->mHeight/heightDivisor,
                                         (size_t&) ACParams.nACProcWinStartY,
                                         (size_t&) ACParams.nACProcWinStartX,
                                         (size_t&) ACParams.nACProcWinWidth,
                                         (size_t&) ACParams.nACProcWinHeight);
    }

    CAMHAL_LOGDB("nSize %d", (int)ACParams.nSize);
    CAMHAL_LOGDB("nPortIndex %d", (int)ACParams.nPortIndex);
    CAMHAL_LOGDB("nManualConverence %d", (int)ACParams.nManualConverence);
    CAMHAL_LOGDB("eACMode %d", (int)ACParams.eACMode);
    CAMHAL_LOGDB("nACProcWinStartX %d", (int)ACParams.nACProcWinStartX);
    CAMHAL_LOGDB("nACProcWinStartY %d", (int)ACParams.nACProcWinStartY);
    CAMHAL_LOGDB("nACProcWinWidth %d", (int)ACParams.nACProcWinWidth);
    CAMHAL_LOGDB("nACProcWinHeight %d", (int)ACParams.nACProcWinHeight);
    CAMHAL_LOGDB("bACStatus %d", (int)ACParams.bACStatus);

    eError =  OMX_SetConfig(mCameraAdapterParameters.mHandleComp,
                            (OMX_INDEXTYPE)OMX_TI_IndexConfigAutoConvergence,
                            &ACParams);

    if ( eError != OMX_ErrorNone ) {
        CAMHAL_LOGEB("Error while setting AutoConvergence 0x%x", eError);
        ret = BAD_VALUE;
    } else {
        CAMHAL_LOGDA("AutoConvergence applied successfully");
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::enableVideoNoiseFilter(bool enable)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_PARAM_VIDEONOISEFILTERTYPE vnfCfg;


    LOG_FUNCTION_NAME;

    if ( NO_ERROR == ret )
        {
        OMX_INIT_STRUCT_PTR (&vnfCfg, OMX_PARAM_VIDEONOISEFILTERTYPE);

        if ( enable )
            {
            CAMHAL_LOGDA("VNF is enabled");
            vnfCfg.eMode = OMX_VideoNoiseFilterModeOn;
            }
        else
            {
            CAMHAL_LOGDA("VNF is disabled");
            vnfCfg.eMode = OMX_VideoNoiseFilterModeOff;
            }

        eError =  OMX_SetParameter(mCameraAdapterParameters.mHandleComp,
                                   ( OMX_INDEXTYPE ) OMX_IndexParamVideoNoiseFilter,
                                   &vnfCfg);
        if ( OMX_ErrorNone != eError )
            {
            CAMHAL_LOGEB("Error while configuring video noise filter 0x%x", eError);
            ret = -1;
            }
        else
            {
            CAMHAL_LOGDA("Video noise filter is configured successfully");
            }
        }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::enableVideoStabilization(bool enable)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONFIG_FRAMESTABTYPE frameStabCfg;


    LOG_FUNCTION_NAME;

    if ( NO_ERROR == ret )
        {
        OMX_CONFIG_BOOLEANTYPE vstabp;
        OMX_INIT_STRUCT_PTR (&vstabp, OMX_CONFIG_BOOLEANTYPE);
        if(enable)
            {
            vstabp.bEnabled = OMX_TRUE;
            }
        else
            {
            vstabp.bEnabled = OMX_FALSE;
            }

        eError = OMX_SetParameter(mCameraAdapterParameters.mHandleComp,
                                  (OMX_INDEXTYPE)OMX_IndexParamFrameStabilisation,
                                  &vstabp);
        if ( OMX_ErrorNone != eError )
            {
            CAMHAL_LOGEB("Error while configuring video stabilization param 0x%x", eError);
            ret = -1;
            }
        else
            {
            CAMHAL_LOGDA("Video stabilization param configured successfully");
            }

        }

    if ( NO_ERROR == ret )
        {

        OMX_INIT_STRUCT_PTR (&frameStabCfg, OMX_CONFIG_FRAMESTABTYPE);


        eError =  OMX_GetConfig(mCameraAdapterParameters.mHandleComp,
                                ( OMX_INDEXTYPE ) OMX_IndexConfigCommonFrameStabilisation,
                                &frameStabCfg);
        if ( OMX_ErrorNone != eError )
            {
            CAMHAL_LOGEB("Error while getting video stabilization mode 0x%x",
                         (unsigned int)eError);
            ret = -1;
            }

        CAMHAL_LOGDB("VSTAB Port Index = %d", (int)frameStabCfg.nPortIndex);

        frameStabCfg.nPortIndex = mCameraAdapterParameters.mPrevPortIndex;
        if ( enable )
            {
            CAMHAL_LOGDA("VSTAB is enabled");
            frameStabCfg.bStab = OMX_TRUE;
            }
        else
            {
            CAMHAL_LOGDA("VSTAB is disabled");
            frameStabCfg.bStab = OMX_FALSE;

            }

        eError =  OMX_SetConfig(mCameraAdapterParameters.mHandleComp,
                                ( OMX_INDEXTYPE ) OMX_IndexConfigCommonFrameStabilisation,
                                &frameStabCfg);
        if ( OMX_ErrorNone != eError )
            {
            CAMHAL_LOGEB("Error while configuring video stabilization mode 0x%x", eError);
            ret = -1;
            }
        else
            {
            CAMHAL_LOGDA("Video stabilization mode configured successfully");
            }
        }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::setGBCE(OMXCameraAdapter::BrightnessMode mode)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_TI_CONFIG_LOCAL_AND_GLOBAL_BRIGHTNESSCONTRASTTYPE bControl;

    LOG_FUNCTION_NAME;

    if ( OMX_StateInvalid == mComponentState )
        {
        CAMHAL_LOGEA("OMX component is in invalid state");
        ret = -EINVAL;
        }

    if ( NO_ERROR == ret )
        {
        OMX_INIT_STRUCT_PTR (&bControl, OMX_TI_CONFIG_LOCAL_AND_GLOBAL_BRIGHTNESSCONTRASTTYPE);

        bControl.nPortIndex = OMX_ALL;

        switch ( mode )
            {
            case OMXCameraAdapter::BRIGHTNESS_ON:
                {
                bControl.eControl = OMX_TI_BceModeOn;
                break;
                }
            case OMXCameraAdapter::BRIGHTNESS_AUTO:
                {
                bControl.eControl = OMX_TI_BceModeAuto;
                break;
                }
            case OMXCameraAdapter::BRIGHTNESS_OFF:
            default:
                {
                bControl.eControl = OMX_TI_BceModeOff;
                break;
                }
            }

        eError = OMX_SetConfig(mCameraAdapterParameters.mHandleComp,
                               ( OMX_INDEXTYPE ) OMX_TI_IndexConfigGlobalBrightnessContrastEnhance,
                               &bControl);
        if ( OMX_ErrorNone != eError )
            {
            CAMHAL_LOGEB("Error while setting GBCE 0x%x", eError);
            }
        else
            {
            CAMHAL_LOGDB("GBCE configured successfully 0x%x", mode);
            }
        }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::setGLBCE(OMXCameraAdapter::BrightnessMode mode)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_TI_CONFIG_LOCAL_AND_GLOBAL_BRIGHTNESSCONTRASTTYPE bControl;

    LOG_FUNCTION_NAME;

    if ( OMX_StateInvalid == mComponentState )
        {
        CAMHAL_LOGEA("OMX component is in invalid state");
        ret = -EINVAL;
        }

    if ( NO_ERROR == ret )
        {
        OMX_INIT_STRUCT_PTR (&bControl, OMX_TI_CONFIG_LOCAL_AND_GLOBAL_BRIGHTNESSCONTRASTTYPE);
        bControl.nPortIndex = OMX_ALL;

        switch ( mode )
            {
            case OMXCameraAdapter::BRIGHTNESS_ON:
                {
                bControl.eControl = OMX_TI_BceModeOn;
                break;
                }
            case OMXCameraAdapter::BRIGHTNESS_AUTO:
                {
                bControl.eControl = OMX_TI_BceModeAuto;
                break;
                }
            case OMXCameraAdapter::BRIGHTNESS_OFF:
            default:
                {
                bControl.eControl = OMX_TI_BceModeOff;
                break;
                }
            }

        eError = OMX_SetConfig(mCameraAdapterParameters.mHandleComp,
                               ( OMX_INDEXTYPE ) OMX_TI_IndexConfigLocalBrightnessContrastEnhance,
                               &bControl);
        if ( OMX_ErrorNone != eError )
            {
            CAMHAL_LOGEB("Error while configure GLBCE 0x%x", eError);
            }
        else
            {
            CAMHAL_LOGDA("GLBCE configured successfully");
            }
        }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::setCaptureMode(OMXCameraAdapter::CaptureMode mode)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONFIG_CAMOPERATINGMODETYPE camMode;
    OMX_CONFIG_BOOLEANTYPE bCAC;
    OMX_TI_CONFIG_SINGLEPREVIEWMODETYPE singlePrevMode;

    LOG_FUNCTION_NAME;

    //CAC is disabled by default
    OMX_INIT_STRUCT_PTR (&bCAC, OMX_CONFIG_BOOLEANTYPE);
    OMX_INIT_STRUCT_PTR (&singlePrevMode, OMX_TI_CONFIG_SINGLEPREVIEWMODETYPE);
    bCAC.bEnabled = OMX_FALSE;

    if ( NO_ERROR == ret )
        {

        OMX_INIT_STRUCT_PTR (&camMode, OMX_CONFIG_CAMOPERATINGMODETYPE);
        if ( mSensorIndex == OMX_TI_StereoSensor ) {
            if ( OMXCameraAdapter::VIDEO_MODE == mode ) {
                CAMHAL_LOGDA("Camera mode: STEREO VIDEO");
                camMode.eCamOperatingMode = OMX_TI_StereoVideo;
            } else {
                CAMHAL_LOGDA("Camera mode: STEREO");
                camMode.eCamOperatingMode = OMX_CaptureStereoImageCapture;
            }
        } else if ( OMXCameraAdapter::HIGH_SPEED == mode ) {
            CAMHAL_LOGDA("Camera mode: HIGH SPEED");
            camMode.eCamOperatingMode = OMX_CaptureImageHighSpeedTemporalBracketing;
        } else if ( OMXCameraAdapter::CP_CAM == mode ) {
            CAMHAL_LOGDA("Camera mode: CP CAM");
            camMode.eCamOperatingMode = OMX_TI_CPCam;
            // TODO(XXX): Hardcode for now until we implement re-proc pipe
            singlePrevMode.eMode = OMX_TI_SinglePreviewMode_ImageCaptureHighSpeed;
        } else if( OMXCameraAdapter::HIGH_QUALITY == mode ) {
            CAMHAL_LOGDA("Camera mode: HIGH QUALITY");
            camMode.eCamOperatingMode = OMX_CaptureImageProfileBase;
        } else if( OMXCameraAdapter::HIGH_QUALITY_ZSL== mode ) {
            const char* valstr = NULL;
            CAMHAL_LOGDA("Camera mode: HIGH QUALITY_ZSL");
            camMode.eCamOperatingMode = OMX_TI_CaptureImageProfileZeroShutterLag;

#ifdef CAMERAHAL_TUNA
            if ( !mIternalRecordingHint ) {
                zslHistoryLen.nHistoryLen = 5;
            }
#endif

        } else if( OMXCameraAdapter::VIDEO_MODE == mode ) {
            CAMHAL_LOGDA("Camera mode: VIDEO MODE");
            camMode.eCamOperatingMode = OMX_CaptureVideo;
        } else if( OMXCameraAdapter::VIDEO_MODE_HQ == mode ) {
            CAMHAL_LOGDA("Camera mode: VIDEO MODE HQ");
            camMode.eCamOperatingMode = OMX_CaptureHighQualityVideo;
        } else {
            CAMHAL_LOGEA("Camera mode: INVALID mode passed!");
            return BAD_VALUE;
        }

        if( NO_ERROR == ret )
            {
            eError =  OMX_SetParameter(mCameraAdapterParameters.mHandleComp,
                                       ( OMX_INDEXTYPE ) OMX_IndexCameraOperatingMode,
                                       &camMode);
            if ( OMX_ErrorNone != eError )
                {
                CAMHAL_LOGEB("Error while configuring camera mode 0x%x", eError);
                ret = Utils::ErrorUtils::omxToAndroidError(eError);
                }
            else
                {
                CAMHAL_LOGDA("Camera mode configured successfully");
                }
            }

        if((NO_ERROR == ret) && (OMXCameraAdapter::CP_CAM == mode)) {
            //Configure Single Preview Mode
            eError =  OMX_SetConfig(mCameraAdapterParameters.mHandleComp,
                                    ( OMX_INDEXTYPE ) OMX_TI_IndexConfigSinglePreviewMode,
                                    &singlePrevMode);
            if ( OMX_ErrorNone != eError ) {
                CAMHAL_LOGEB("Error while configuring single preview mode 0x%x", eError);
                ret = Utils::ErrorUtils::omxToAndroidError(eError);
            } else {
                CAMHAL_LOGDA("single preview mode configured successfully");
            }
        }


        if( NO_ERROR == ret )
            {
            //Configure CAC
            eError =  OMX_SetConfig(mCameraAdapterParameters.mHandleComp,
                                    ( OMX_INDEXTYPE ) OMX_IndexConfigChromaticAberrationCorrection,
                                    &bCAC);
            if ( OMX_ErrorNone != eError )
                {
                CAMHAL_LOGEB("Error while configuring CAC 0x%x", eError);
                ret = Utils::ErrorUtils::omxToAndroidError(eError);
                }
            else
                {
                CAMHAL_LOGDA("CAC configured successfully");
                }
            }
        }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::setLDC(OMXCameraAdapter::IPPMode mode)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONFIG_BOOLEANTYPE bOMX;

    LOG_FUNCTION_NAME;

    if ( OMX_StateLoaded != mComponentState )
        {
        CAMHAL_LOGEA("OMX component is not in loaded state");
        ret = -EINVAL;
        }

    if ( NO_ERROR == ret )
        {
        OMX_INIT_STRUCT_PTR (&bOMX, OMX_CONFIG_BOOLEANTYPE);

        switch ( mode )
            {
            case OMXCameraAdapter::IPP_LDCNSF:
            case OMXCameraAdapter::IPP_LDC:
                {
                bOMX.bEnabled = OMX_TRUE;
                break;
                }
            case OMXCameraAdapter::IPP_NONE:
            case OMXCameraAdapter::IPP_NSF:
            default:
                {
                bOMX.bEnabled = OMX_FALSE;
                break;
                }
            }

        CAMHAL_LOGVB("Configuring LDC mode 0x%x", bOMX.bEnabled);
        eError = OMX_SetParameter(mCameraAdapterParameters.mHandleComp,
                                  ( OMX_INDEXTYPE ) OMX_IndexParamLensDistortionCorrection,
                                  &bOMX);
        if ( OMX_ErrorNone != eError )
            {
            CAMHAL_LOGEA("Error while setting LDC");
            ret = -1;
            }
        }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::setNSF(OMXCameraAdapter::IPPMode mode)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_PARAM_ISONOISEFILTERTYPE nsf;

    LOG_FUNCTION_NAME;

    if ( OMX_StateLoaded != mComponentState )
        {
        CAMHAL_LOGEA("OMX component is not in loaded state");
        ret = -EINVAL;
        }

    if ( NO_ERROR == ret )
        {
        OMX_INIT_STRUCT_PTR (&nsf, OMX_PARAM_ISONOISEFILTERTYPE);
        nsf.nPortIndex = OMX_ALL;

        switch ( mode )
            {
            case OMXCameraAdapter::IPP_LDCNSF:
            case OMXCameraAdapter::IPP_NSF:
                {
                nsf.eMode = OMX_ISONoiseFilterModeOn;
                break;
                }
            case OMXCameraAdapter::IPP_LDC:
            case OMXCameraAdapter::IPP_NONE:
            default:
                {
                nsf.eMode = OMX_ISONoiseFilterModeOff;
                break;
                }
            }

        CAMHAL_LOGVB("Configuring NSF mode 0x%x", nsf.eMode);
        eError = OMX_SetParameter(mCameraAdapterParameters.mHandleComp,
                                  (OMX_INDEXTYPE)OMX_IndexParamHighISONoiseFiler,
                                  &nsf);
        if ( OMX_ErrorNone != eError )
            {
            CAMHAL_LOGEA("Error while setting NSF");
            ret = -1;
            }
        }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::setImageQuality(unsigned int quality)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_IMAGE_PARAM_QFACTORTYPE jpegQualityConf;

    LOG_FUNCTION_NAME;

    if ( OMX_StateInvalid == mComponentState )
        {
        CAMHAL_LOGEA("OMX component is in invalid state");
        ret = -EINVAL;
        }

    if ( NO_ERROR == ret )
        {
        OMX_INIT_STRUCT(jpegQualityConf, OMX_IMAGE_PARAM_QFACTORTYPE);
        jpegQualityConf.nQFactor = quality;
        jpegQualityConf.nPortIndex = mCameraAdapterParameters.mImagePortIndex;

        eError = OMX_SetParameter(mCameraAdapterParameters.mHandleComp,
                                  OMX_IndexParamQFactor,
                                  &jpegQualityConf);
        if ( OMX_ErrorNone != eError )
            {
            CAMHAL_LOGEB("Error while configuring jpeg Quality 0x%x", eError);
            ret = -1;
            }
        }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::setThumbnailParams(unsigned int width,
                                              unsigned int height,
                                              unsigned int quality)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_PARAM_THUMBNAILTYPE thumbConf;

    LOG_FUNCTION_NAME;

    if ( OMX_StateInvalid == mComponentState )
        {
        CAMHAL_LOGEA("OMX component is in invalid state");
        ret = -EINVAL;
        }

    if ( NO_ERROR == ret )
        {
        OMX_INIT_STRUCT(thumbConf, OMX_PARAM_THUMBNAILTYPE);
        thumbConf.nPortIndex = mCameraAdapterParameters.mImagePortIndex;

        eError = OMX_GetParameter(mCameraAdapterParameters.mHandleComp,
                                  ( OMX_INDEXTYPE ) OMX_IndexParamThumbnail,
                                  &thumbConf);
        if ( OMX_ErrorNone != eError )
            {
            CAMHAL_LOGEB("Error while retrieving thumbnail size 0x%x", eError);
            ret = -1;
            }

        //CTS Requirement: width or height equal to zero should
        //result in absent EXIF thumbnail
        if ( ( 0 == width ) || ( 0 == height ) )
            {
            thumbConf.nWidth = mThumbRes[0].width;
            thumbConf.nHeight = mThumbRes[0].height;
            thumbConf.eCompressionFormat = OMX_IMAGE_CodingUnused;
            }
        else
            {
            thumbConf.nWidth = width;
            thumbConf.nHeight = height;
            thumbConf.nQuality = quality;
            thumbConf.eCompressionFormat = OMX_IMAGE_CodingJPEG;
            }

        CAMHAL_LOGDB("Thumbnail width = %d, Thumbnail Height = %d", width, height);

        eError = OMX_SetParameter(mCameraAdapterParameters.mHandleComp,
                                  ( OMX_INDEXTYPE ) OMX_IndexParamThumbnail,
                                  &thumbConf);
        if ( OMX_ErrorNone != eError )
            {
            CAMHAL_LOGEB("Error while configuring thumbnail size 0x%x", eError);
            ret = -1;
            }
        }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::setAlgoPriority(AlgoPriority priority,
                                           Algorithm3A algo,
                                           bool enable)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    LOG_FUNCTION_NAME;

    if ( OMX_StateInvalid == mComponentState ) {
        CAMHAL_LOGEA("OMX component is in invalid state");
        return NO_INIT;
    }

    if ( FACE_PRIORITY == priority ) {

        if ( algo & WHITE_BALANCE_ALGO ) {
            if ( enable ) {
                mFacePriority.bAwbFaceEnable =  OMX_TRUE;
            } else {
                mFacePriority.bAwbFaceEnable =  OMX_FALSE;
            }
        }

        if ( algo & EXPOSURE_ALGO ) {
            if ( enable ) {
                mFacePriority.bAeFaceEnable =  OMX_TRUE;
            } else {
                mFacePriority.bAeFaceEnable =  OMX_FALSE;
            }
        }

        if ( algo & FOCUS_ALGO ) {
            if ( enable ) {
                mFacePriority.bAfFaceEnable =  OMX_TRUE;
            } else {
                mFacePriority.bAfFaceEnable =  OMX_FALSE;
            }
        }

        eError =  OMX_SetConfig(mCameraAdapterParameters.mHandleComp,
                                ( OMX_INDEXTYPE ) OMX_TI_IndexConfigFacePriority3a,
                                &mFacePriority);
        if ( OMX_ErrorNone != eError ) {
            CAMHAL_LOGEB("Error while configuring face priority 0x%x", eError);
        } else {
            CAMHAL_LOGDB("Face priority for algorithms set successfully 0x%x, 0x%x, 0x%x",
                         mFacePriority.bAfFaceEnable,
                         mFacePriority.bAeFaceEnable,
                         mFacePriority.bAwbFaceEnable);
        }

    } else if ( REGION_PRIORITY == priority ) {

        if ( algo & WHITE_BALANCE_ALGO ) {
            if ( enable ) {
                mRegionPriority.bAwbRegionEnable= OMX_TRUE;
            } else {
                mRegionPriority.bAwbRegionEnable = OMX_FALSE;
            }
        }

        if ( algo & EXPOSURE_ALGO ) {
            if ( enable ) {
                mRegionPriority.bAeRegionEnable = OMX_TRUE;
            } else {
                mRegionPriority.bAeRegionEnable = OMX_FALSE;
            }
        }

        if ( algo & FOCUS_ALGO ) {
            if ( enable ) {
                mRegionPriority.bAfRegionEnable = OMX_TRUE;
            } else {
                mRegionPriority.bAfRegionEnable = OMX_FALSE;
            }
        }

        eError =  OMX_SetConfig(mCameraAdapterParameters.mHandleComp,
                                ( OMX_INDEXTYPE ) OMX_TI_IndexConfigRegionPriority3a,
                                &mRegionPriority);
        if ( OMX_ErrorNone != eError ) {
            CAMHAL_LOGEB("Error while configuring region priority 0x%x", eError);
        } else {
            CAMHAL_LOGDB("Region priority for algorithms set successfully 0x%x, 0x%x, 0x%x",
                         mRegionPriority.bAfRegionEnable,
                         mRegionPriority.bAeRegionEnable,
                         mRegionPriority.bAwbRegionEnable);
        }

    }

    LOG_FUNCTION_NAME_EXIT;

    return Utils::ErrorUtils::omxToAndroidError(eError);
}

status_t OMXCameraAdapter::setPictureRotation(unsigned int degree)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONFIG_ROTATIONTYPE rotation;

    LOG_FUNCTION_NAME;

    if ( OMX_StateInvalid == mComponentState )
        {
        CAMHAL_LOGEA("OMX component is in invalid state");
        ret = -1;
        }

    if ( NO_ERROR == ret )
        {
        OMX_INIT_STRUCT(rotation, OMX_CONFIG_ROTATIONTYPE);
        rotation.nRotation = degree;
        rotation.nPortIndex = mCameraAdapterParameters.mImagePortIndex;

        eError = OMX_SetConfig(mCameraAdapterParameters.mHandleComp,
                               OMX_IndexConfigCommonRotate,
                               &rotation);
        if ( OMX_ErrorNone != eError )
            {
            CAMHAL_LOGEB("Error while configuring rotation 0x%x", eError);
            }
        }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::setSensorOrientation(unsigned int degree)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONFIG_ROTATIONTYPE sensorOrientation;
    int tmpHeight, tmpWidth;
    OMXCameraPortParameters *mPreviewData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mPrevPortIndex];

    LOG_FUNCTION_NAME;
    if ( OMX_StateInvalid == mComponentState ) {
        CAMHAL_LOGEA("OMX component is in invalid state");
        ret = -1;
    }

    /* Set Temproary Port resolution.
    * For resolution with height >= 720,
    * resolution cannot be set without configuring orientation.
    * So we first set a temp resolution. We have used VGA
    */
    if ( mPreviewData->mHeight >= 720 ) {
        tmpHeight = mPreviewData->mHeight;
        tmpWidth = mPreviewData->mWidth;
        mPreviewData->mWidth = 640;
        mPreviewData->mHeight = 480;

        ret = setFormat(OMX_CAMERA_PORT_VIDEO_OUT_PREVIEW, *mPreviewData);
        if ( NO_ERROR != ret ) {
            CAMHAL_LOGEB("Error while configuring format 0x%x", ret);
            return ret;
        }

        mPreviewData->mWidth = tmpWidth;
        mPreviewData->mHeight = tmpHeight;
        mPreviewPortInitialized = true;
    }
    else if (!mPreviewPortInitialized) {
        ret = setFormat(OMX_CAMERA_PORT_VIDEO_OUT_PREVIEW, *mPreviewData);
        if ( NO_ERROR != ret ) {
            CAMHAL_LOGEB("Error while configuring format 0x%x", ret);
            return ret;
        }
        mPreviewPortInitialized = true;
    }

    /* Now set Required Orientation*/
    if ( NO_ERROR == ret ) {
        OMX_INIT_STRUCT(sensorOrientation, OMX_CONFIG_ROTATIONTYPE);
        sensorOrientation.nPortIndex = mCameraAdapterParameters.mPrevPortIndex;
        sensorOrientation.nRotation = degree;
        eError = OMX_SetConfig(mCameraAdapterParameters.mHandleComp,
                               OMX_IndexConfigCommonRotate,
                               &sensorOrientation);
        if ( OMX_ErrorNone != eError ) {
            CAMHAL_LOGEB("Error while configuring rotation 0x%x", eError);
        }
        CAMHAL_LOGVB(" Currently Sensor Orientation is set to : %d",
                     ( unsigned int ) sensorOrientation.nRotation);
        CAMHAL_LOGVB(" Sensor Configured for Port : %d",
                     ( unsigned int ) sensorOrientation.nPortIndex);
    }

    /* Now set the required resolution as requested */
    if ( NO_ERROR == ret ) {
        bool portConfigured = false;
        ret = setSensorQuirks(degree,
                mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mPrevPortIndex],
                portConfigured);
        if ( NO_ERROR != ret ) {
            CAMHAL_LOGEB("Error while configuring setSensorQuirks 0x%x", ret);
            return ret;
        }

        if ( !portConfigured ) {
            ret = setFormat (mCameraAdapterParameters.mPrevPortIndex,
                             mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mPrevPortIndex]);
            if ( NO_ERROR != ret ) {
                CAMHAL_LOGEB("Error while configuring format 0x%x", ret);
                return ret;
            }

            // Another WA: Setting the port definition will reset the VFR
            //             configuration.
            setVFramerate(mPreviewData->mMinFrameRate,
                          mPreviewData->mMaxFrameRate);
        }
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::setVFramerate(OMX_U32 minFrameRate, OMX_U32 maxFrameRate)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_TI_CONFIG_VARFRMRANGETYPE vfr;
    OMXCameraPortParameters * mPreviewData =
        &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mPrevPortIndex];

    LOG_FUNCTION_NAME;

    if ( OMX_StateInvalid == mComponentState ) {
        CAMHAL_LOGEA("OMX component is in invalid state");
        ret = -EINVAL;
    }

    if ( !mSetFormatDone ) {
        return NO_INIT;
    }

    if ( NO_ERROR == ret ) {
        OMX_INIT_STRUCT_PTR (&vfr, OMX_TI_CONFIG_VARFRMRANGETYPE);

        vfr.xMin = minFrameRate<<16;
        vfr.xMax = maxFrameRate<<16;

        eError = OMX_SetConfig(mCameraAdapterParameters.mHandleComp,
                               (OMX_INDEXTYPE)OMX_TI_IndexConfigVarFrmRange,
                               &vfr);
        if(OMX_ErrorNone != eError) {
            CAMHAL_LOGEB("Error while setting VFR min = %d, max = %d, error = 0x%x",
                         ( unsigned int ) minFrameRate,
                         ( unsigned int ) maxFrameRate,
                         eError);
            ret = -1;
        } else {
            CAMHAL_LOGDB("VFR Configured Successfully [%d:%d]",
                        ( unsigned int ) minFrameRate,
                        ( unsigned int ) maxFrameRate);
        }
    }

    return ret;
 }

status_t OMXCameraAdapter::setMechanicalMisalignmentCorrection(const bool enable)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_TI_CONFIG_MM mm;

    LOG_FUNCTION_NAME;

    mm.nVersion = mLocalVersionParam;
    mm.nSize = sizeof(OMX_TI_CONFIG_MM);
    mm.nPortIndex = mCameraAdapterParameters.mPrevPortIndex;
    mm.bMM = enable ? OMX_TRUE : OMX_FALSE;

    eError = OMX_SetConfig(mCameraAdapterParameters.mHandleComp,
                           (OMX_INDEXTYPE)OMX_TI_IndexConfigMechanicalMisalignment,
                           &mm);

    if(OMX_ErrorNone != eError) {
        CAMHAL_LOGEB("Error while enabling mechanical misalignment correction. error = 0x%x", eError);
        ret = -1;
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

} // namespace Camera
} // namespace Ti
