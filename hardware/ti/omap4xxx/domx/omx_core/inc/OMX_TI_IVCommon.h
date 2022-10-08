/*
 * Copyright (c) 2010, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* -------------------------------------------------------------------------- *
 *
 * @file:OMX_TI_IVCommon.h
 * This header defines the structures specific to the config indices of msp_VPPM.
 *
 * @path ..\OMAPSW_SysDev\multimedia\omx\khronos1_1\omx_core\inc
 *
 * -------------------------------------------------------------------------- */

/* ======================================================================== *!
 *! Revision History
 *! ==================================================================== */

#ifndef OMX_TI_IVCommon_H
#define OMX_TI_IVCommon_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <OMX_IVCommon.h>
#include <OMX_Image.h>


#define MAX_URI_LENGTH      (OMX_MAX_STRINGNAME_SIZE)
#define MAX_ALGOAREAS       (35)

/*======================================================================= */
/* Enumerated values for operation mode for compressed image
 *
 * ENUMS:
 * Chunk         : Chunk based operation
 * NonChunk    : Non-chunk based operation
 */
 /* ======================================================================= */
typedef enum OMX_JPEG_COMPRESSEDMODETYPE {
    OMX_JPEG_ModeChunk = 0,
    OMX_JPEG_ModeNonChunk
}OMX_JPEG_COMPRESSEDMODETYPE ;


/*======================================================================= */
/* Enumerated values for operation mode for uncompressed image
 *
 * ENUMS:
 * Frame   :  Frame based operation
 * Slice   : Slice based operation
 * Stitch  : For stitching between image frames
 * Burst   :  For stitching between image frames
 */
 /* ======================================================================= */
typedef enum OMX_JPEG_UNCOMPRESSEDMODETYPE {
    OMX_JPEG_UncompressedModeFrame = 0,
    OMX_JPEG_UncompressedModeSlice,
    OMX_JPEG_UncompressedModeStitch,
    OMX_JPEG_UncompressedModeBurst
}OMX_JPEG_UNCOMPRESSEDMODETYPE;



/*======================================================================= */
/* Configuration structure for compressed image
 *
 * STRUCT MEMBERS:
 *  nSize                 : Size of the structure in bytes
 *  nVersion              : OMX specification version information
 *  nPortIndex            : Port that this structure applies to
 *  eCompressedImageMode  : Operating mode enumeration for compressed image
 */
 /*======================================================================= */
typedef struct OMX_JPEG_PARAM_COMPRESSEDMODETYPE{
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_JPEG_COMPRESSEDMODETYPE eCompressedImageMode;
}OMX_JPEG_PARAM_COMPRESSEDMODETYPE;



/*======================================================================= */
/* Uncompressed image Operating mode configuration structure
 *
 * STRUCT MEMBERS:
 * nSize                     : Size of the structure in bytes
 * nVersion                  : OMX specification version information
 * nPortIndex                : Port that this structure applies to
 * nBurstLength              : No of frames to be dumped in burst mode
 * eUncompressedImageMode    : uncompressed image mode information
 * eSourceType               : Image encode souce info
 * tRotationInfo             : Rotation related information
 */
 /*======================================================================= */
typedef struct OMX_JPEG_PARAM_UNCOMPRESSEDMODETYPE{
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nBurstLength;
    OMX_JPEG_UNCOMPRESSEDMODETYPE eUncompressedImageMode;
}OMX_JPEG_PARAM_UNCOMPRESSEDMODETYPE;


/*======================================================================= */
/* Subregion Decode Parameter configuration structure
 *
 * STRUCT MEMBERS:
 * nSize                     : Size of the structure in bytes
 * nVersion                  : OMX specification version information
 * nXOrg                     : Sectional decoding X origin
 * nYOrg                     : Sectional decoding Y origin
 * nXLength                  : Sectional decoding X length
 * nYLength                  : Sectional decoding Y length
 */
 /*======================================================================= */
typedef struct OMX_IMAGE_PARAM_DECODE_SUBREGION{
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nXOrg;
    OMX_U32 nYOrg;
    OMX_U32 nXLength;
    OMX_U32 nYLength;
}OMX_IMAGE_PARAM_DECODE_SUBREGION;


/**
 * sensor select  types
 */
typedef enum OMX_SENSORSELECT{
        OMX_PrimarySensor = 0,
        OMX_SecondarySensor,
        OMX_TI_StereoSensor,
        OMX_SensorTypeMax = 0x7fffffff
}OMX_SENSORSELECT;

/**
 *
 * Sensor Select
 */
typedef  struct OMX_CONFIG_SENSORSELECTTYPE {
OMX_U32  nSize; /**< Size of the structure in bytes */
OMX_VERSIONTYPE nVersion; /**< OMX specification version info */
OMX_U32 nPortIndex; /**< Port that this struct applies to */
OMX_SENSORSELECT eSensor; /**< sensor select */
} OMX_CONFIG_SENSORSELECTTYPE;

/**
 * Flicker cancellation types
 */
typedef enum OMX_COMMONFLICKERCANCELTYPE{
        OMX_FlickerCancelOff = 0,
        OMX_FlickerCancelAuto,
        OMX_FlickerCancel50,
        OMX_FlickerCancel60,
        OMX_FlickerCancel100,
        OMX_FlickerCancel120,
        OMX_FlickerCancelMax = 0x7fffffff
}OMX_COMMONFLICKERCANCELTYPE;

typedef struct OMX_CONFIG_FLICKERCANCELTYPE {
   OMX_U32 nSize;
   OMX_VERSIONTYPE nVersion;
   OMX_U32 nPortIndex;
   OMX_COMMONFLICKERCANCELTYPE eFlickerCancel;
} OMX_CONFIG_FLICKERCANCELTYPE;


/**
 * Sensor caleberation types
 */
typedef enum OMX_SENSORCALTYPE{
        OMX_SensorCalFull = 0,
        OMX_SensorCalQuick,
        OMX_SensorCalMax = 0x7fffffff
}OMX_SENSORCALTYPE;

typedef struct OMX_CONFIG_SENSORCALTYPE {
   OMX_U32 nSize;
   OMX_VERSIONTYPE nVersion;
   OMX_U32 nPortIndex;
   OMX_SENSORCALTYPE eSensorCal;
} OMX_CONFIG_SENSORCALTYPE;

/**
 * Scene mode types
 */
typedef enum OMX_SCENEMODETYPE{

        OMX_Manual = 0,
        OMX_Closeup,
        OMX_Portrait,
        OMX_Landscape,
        OMX_Underwater,
        OMX_Sport,
        OMX_SnowBeach,
        OMX_Mood,
        OMX_NightPortrait,
        OMX_NightIndoor,
        OMX_Fireworks,
        OMX_Document, /**< for still image */
        OMX_Barcode, /**< for still image */
        OMX_SuperNight, /**< for video */
        OMX_Cine, /**< for video */
        OMX_OldFilm, /**< for video */
        OMX_TI_Action,
        OMX_TI_Beach,
        OMX_TI_Candlelight,
        OMX_TI_Night,
        OMX_TI_Party,
        OMX_TI_Portrait,
        OMX_TI_Snow,
        OMX_TI_Steadyphoto,
        OMX_TI_Sunset,
        OMX_TI_Theatre,
        OMX_SceneModeMax = 0x7fffffff
}OMX_SCENEMODETYPE;

typedef struct OMX_CONFIG_SCENEMODETYPE {
   OMX_U32 nSize;
   OMX_VERSIONTYPE nVersion;
   OMX_U32 nPortIndex;
   OMX_SCENEMODETYPE eSceneMode;
} OMX_CONFIG_SCENEMODETYPE;

 /**
 * Port specific capture trigger
 * useful for the usecases with multiple capture ports.
 *
 * STRUCT MEMBERS:
 *  nSize      : Size of the structure in bytes
 *  nVersion   : OMX specification version information
 *  nPortIndex : Port that this structure applies to
 *  bExtCapturing : Start Captre at the specified port. Can be queried to know the status of a specific port.
 */
typedef struct OMX_CONFIG_EXTCAPTURING {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bExtCapturing;
} OMX_CONFIG_EXTCAPTURING;


 /**
 * Digital Zoom Speed
 *
 * STRUCT MEMBERS:
 *  nSize      : Size of the structure in bytes
 *  nVersion   : OMX specification version information
 *  nPortIndex : Port that this structure applies to
 *  nDigitalZoomSpeed      :  Optical zoom speed level. Special values:
 *      0 - stop current movement
 *      values from 1 to 254 are mapped proportionally to supported zoom speeds inside optical zoom driver.
 *      So 1 is slowest available optical zoom speed and 254 is fastest available optical zoom speed
 *      255 - default optical zoom speed value
 */
typedef struct OMX_CONFIG_DIGITALZOOMSPEEDTYPE {
   OMX_U32 nSize;
   OMX_VERSIONTYPE nVersion;
   OMX_U32 nPortIndex;
   OMX_U8 nDigitalZoomSpeed;
} OMX_CONFIG_DIGITALZOOMSPEEDTYPE;


 /**
 * Digital Zoom Target
 *
 * STRUCT MEMBERS:
 *  nSize      : Size of the structure in bytes
 *  nVersion   : OMX specification version information
 *  nPortIndex : Port that this structure applies to
 *  nDigitalZoomTarget      :  Default and minimum is 0. Maximum is determined by the current supported range
 */

typedef struct OMX_CONFIG_DIGITALZOOMTARGETTYPE {
   OMX_U32 nSize;
   OMX_VERSIONTYPE nVersion;
   OMX_U32 nPortIndex;
   OMX_U32 nDigitalZoomTarget;
} OMX_CONFIG_DIGITALZOOMTARGETTYPE;


/**
* Scale quality enums
*/
typedef enum OMX_SCALEQUALITY{
        OMX_DefaultScaling = 0, /** <default scaling if nothing is specified > */
        OMX_BetterScaling,   /** <better scaling> */
        OMX_BestScaling,  /** <best  scaling> */
        OMX_AutoScalingQuality,  /** <auto scaling quality> */
        OMX_FastScaling,   /** <fast scaling, prioritizes speed> */
        OMX_ScaleQualityMax = 0x7fffffff
}OMX_SCALEQUALITY;

/**
* Scaling Quality Mode
*/
typedef enum OMX_SCALEQUALITYMODE{
        OMX_SingleFrameScalingMode = 0, /** <default > */
        OMX_MultiFrameScalingMode,   /** <better scaling> */
        OMX_AutoScalingMode,  /** <best  scaling> */
        OMX_ScaleModeMax = 0x7fffffff
}OMX_SCALEQUALITYMODE;

 /**
 * Rescale quality control type
 *
 * STRUCT MEMBERS:
 *  nSize      : Size of the structure in bytes
 *  nVersion   : OMX specification version information
 *  eScaleQuality : controls the quality level.
 *  eScaleQualityMode      :  controls the scaling algo types
 */
typedef struct OMX_CONFIG_SCALEQUALITYTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_SCALEQUALITY eScaleQuality;
    OMX_SCALEQUALITYMODE eScaleQualityMode;
} OMX_CONFIG_SCALEQUALITYTYPE;

/**
* Smooth Zoom mode enum
* Starts or stops the Smooth Zoom.  Selecting INCREASE will cause an increasing digital zoom factor (increased cropping),
* with a shrinking viewable area and crop height percentage.  Selecting DECREASE will cause a decreasing digital zoom (decreased cropping),
* with a growing viewable area and crop height percentage.  The CaptureCropHeight will continue to update based on the SmoothZoomRate until
* the SmoothZoomMin or SmoothZoomMax zoom step is reached, the framework minimum zoom step is reached, the SmoothZoomRate becomes 0,
* or the SmoothZoomMode is set to OFF.
* NOTE: The message payload includes all parts of the message that is NOT part of the message header as listed for the CAM_SEND_DATA message.
*/
typedef enum OMX_SMOOTHZOOMMODE{
    OMX_Off=0, /**< default OFF */
    OMX_Increase,
    OMX_Decrease
}OMX_SMOOTHZOOMMODE;


 /**
 * Rescale quality control type
 *
 * STRUCT MEMBERS:
 *  nSize      : Size of the structure in bytes
 *  nVersion   : OMX specification version information
 *  eSmoothZoomMode : controls the smooth zoom feature.
 *  nSmoothZoomRate      :  Values from 0 to 65535 which represents percentage to increase per second, where 65535 = 100%, and 0 = 0%.
 *  nSmoothZoomQuantize:
 *  nSmoothZoomThresh
 *  nSmoothZoomMin
 *  nSmoothZoomMax
 */
typedef struct OMX_CONFIG_SMOOTHZOOMTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_SMOOTHZOOMMODE eSmoothZoomMode;
    OMX_U32 nSmoothZoomRate;
    OMX_U32 nSmoothZoomQuantize;
    OMX_U32 nSmoothZoomThresh;
    OMX_U32 nSmoothZoomMin;
    OMX_U32 nSmoothZoomMax;
} OMX_CONFIG_SMOOTHZOOMTYPE;

/**
 * Enumeration of possible Extended image filter types for OMX_CONFIG_IMAGEFILTERTYPE
 */
typedef enum OMX_EXTIMAGEFILTERTYPE {
    OMX_ImageFilterSepia = 0x7F000001,
    OMX_ImageFilterGrayScale,
    OMX_ImageFilterNatural,
    OMX_ImageFilterVivid,
    OMX_ImageFilterColourSwap,
    OMX_ImageFilterOutOfFocus,
    OMX_ImageFilterWaterColour,
    OMX_ImageFilterPastel,
    OMX_ImageFilterFilm,
    OMX_TI_ImageFilterBlackWhite,
    OMX_TI_ImageFilterWhiteBoard,
    OMX_TI_ImageFilterBlackBoard,
    OMX_TI_ImageFilterAqua,
    OMX_TI_ImageFilterPosterize
} OMX_EXTIMAGEFILTERTYPE;


/**
 * Image filter configuration extended
 *
 * STRUCT MEMBERS:
 *  nSize        : Size of the structure in bytes
 *  nVersion     : OMX specification version information
 *  nPortIndex   : Port that this structure applies to
 *  bBlemish : Enable/Disable Blemish correction
 */
typedef struct OMX_CONFIG_BLEMISHTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bBlemish;
} OMX_CONFIG_BLEMISHTYPE;

/**
 * Enumeration of Bracket types
 * OMX_BracketExposureRelativeInEV:
 *      Exposure value is changed relative to the value set by automatic exposure.
 *      nBracketStartValue and nBracketStep are in Q16. Increment is additive.
 * OMX_BracketExposureAbsoluteMs:
 *      Exposure value is changed in absolute value in ms.
 *      nBracketStartValue and nBracketStep are in Q16. Increment is multiplicative.
 * OMX_BracketFocusRelative:
 *      Focus is adjusted relative to the focus set by auto focus.
 *      The value is S32 integer, and is the same as adjusting nFocusSteps of OMX_IMAGE_CONFIG_FOCUSCONTROLTYPE relatively.
 *      Increment is additive.
 * OMX_BracketFocusAbsolute:
 *      Focus position is adjusted absolutely. It is the same as setting nFocusSteps of
 *      OMX_IMAGE_CONFIG_FOCUSCONTROLTYPE relatively for each captures.
 *      The value should be interpreted as U32 value.  Increment is additive.
 * OMX_BracketFlashPower:
 *      Power of flash is adjusted relative to the automatic level. Increment is multiplicative.
 * OMX_BracketAperture:
 *      Aperture number relative to the automatic setting. Data in Q16 format. Increment is multiplicative.
 * OMX_BracketTemporal:
 *      To suppport temporal bracketing.
 */
typedef enum OMX_BRACKETMODETYPE {
    OMX_BracketExposureRelativeInEV = 0,
    OMX_BracketExposureAbsoluteMs,
    OMX_BracketFocusRelative,
    OMX_BracketFocusAbsolute,
    OMX_BracketFlashPower,
    OMX_BracketAperture,
    OMX_BracketTemporal,
    OMX_BrackerTypeKhronosExtensions = 0x6f000000,
    OMX_BrackerTypeVendorStartUnused = 0x7f000000,
    OMX_BracketTypeMax = 0x7FFFFFFF
} OMX_BRACKETMODETYPE;

typedef struct OMX_CONFIG_BRACKETINGTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BRACKETMODETYPE eBracketMode;
    OMX_U32 nNbrBracketingValues;
    OMX_S32 nBracketValues[10]; /**< 10 can be assumed */
} OMX_CONFIG_BRACKETINGTYPE;


/**
 * Capture mode types
 * Note: this list could get extended modified based on the type of interenal use-case pipelines implemented within the camera component.
 *
 *       OMX_CaptureImageHighSpeedBurst = 0,
 *       OMX_CaptureImageHighSpeedTemporalBracketing,
 *       OMX_CaptureImageProfileBase(Base):
 *       	Base one almost same as Highspeed one.
 *       OMX_CaptureImageProfileLowLight1(LL1):
 *       	Includes NSF2 in addition to Base processing
 *       OMX_CaptureImageProfileLowLight2(LL2):
 *       	Includes NSF2 and LBCE in addition to Base processing.
 *       OMX_CaptureImageProfileOpticalCorr1(OC1):
 *       	Includes LDC in addition to Base processing.
 *       OMX_CaptureImageProfileOpticalCorr2(OC2):
 *       	Includes LDC and CAC in addition to Base processing.
 *       OMX_CaptureImageProfileExtended1(Ext1):
 *       	Includes NSF2, LBCE, LDC, and CAC in addition to Base
 *       OMX_CaptureStereoImageCapture:
 *       	Stereo image capture use-case.
 *       OMX_CaptureImageMemoryInput:
 *       	need to take sensor input from INPUT port.
 *       OMX_CaptureVideo:
 *       OMX_CaptureHighSpeedVideo:
 *       OMX_CaptureVideoMemoryInput:
 *
 */
typedef enum OMX_CAMOPERATINGMODETYPE {
        OMX_CaptureImageHighSpeedBurst = 0,
        OMX_CaptureImageHighSpeedTemporalBracketing,
        OMX_CaptureImageProfileBase,
        OMX_CaptureImageProfileLowLight1,
        OMX_CaptureImageProfileLowLight2,
        OMX_CaptureImageProfileOpticalCorr1,
        OMX_CaptureImageProfileOpticalCorr2,
        OMX_CaptureImageProfileExtended1,
	OMX_CaptureStereoImageCapture,
        OMX_CaptureImageMemoryInput,
        OMX_CaptureVideo,
        OMX_CaptureHighSpeedVideo,
        OMX_CaptureVideoMemoryInput,
        OMX_TI_CaptureDummy,
        OMX_TI_CaptureGestureRecognition,
        OMX_TI_CaptureImageProfileZeroShutterLag,
        OMX_CamOperatingModeMax = 0x7fffffff
} OMX_CAMOPERATINGMODETYPE;
/**
 * Capture mode setting: applicable to multi shot capture also including bracketing.
 *
 * STRUCT MEMBERS:
 *  nSize        : Size of the structure in bytes
 *  nVersion     : OMX specification version information
 *  nPortIndex   : Port that this structure applies to
 *  eCamOperatingMode : specifies the camera operating mode.
 */
typedef struct OMX_CONFIG_CAMOPERATINGMODETYPE {
   OMX_U32 nSize;
   OMX_VERSIONTYPE nVersion;
   OMX_CAMOPERATINGMODETYPE eCamOperatingMode;
} OMX_CONFIG_CAMOPERATINGMODETYPE;


/**
 * Capture mode setting: applicable to multi shot capture also including bracketing.
 *
 * STRUCT MEMBERS:
 *  nSize        : Size of the structure in bytes
 *  nVersion     : OMX specification version information
 *  nPortIndex   : Port that this structure applies to
 *  nFrameRate   : when bContinuous is FALSE, need to define the frame rate of the muti-shot scenario. Since this would be applicable to IMAGE domain port, there is no port specific frame rate.
 *  nFrameBefore :
 * 	is specifying how many frames before the capture trigger shall be used.
 * 	It is implementation dependent how many is supported. This shall only be supported for images and not for video frames.
 * bPrepareCapture :
 *	should be set to true when nFrameBefore is greater than zero and before capturing of before-frames should start.
 *	The component is not allowed to deliver buffers until capturing starts. This shall only be supported for images and not for video frames.
 * bEnableBracketing :
 *	should be enabled when bracketing is used. In bracketing mode, one parameter can be changed per each capture.
 * tBracketConfigType :
 *	specifies bracket mode to use. Valid only when bEnableBracketing is set.
 */
typedef struct OMX_CONFIG_EXTCAPTUREMODETYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nFrameRate;
    OMX_U32 nFrameBefore;
    OMX_BOOL bPrepareCapture;
    OMX_BOOL bEnableBracketing;
    OMX_CONFIG_BRACKETINGTYPE tBracketConfigType;
} OMX_CONFIG_EXTCAPTUREMODETYPE;

/**
 * For Extended Focus region Type -
 */
typedef struct OMX_CONFIG_EXTFOCUSREGIONTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nRefPortIndex;
    OMX_S32 nLeft;
    OMX_S32 nTop;
    OMX_U32 nWidth;
    OMX_U32 nHeight;
} OMX_CONFIG_EXTFOCUSREGIONTYPE;

/**
 * Digital Flash Control
 * STRUCT MEMBERS:
 *  nSize        : Size of the structure in bytes
 *  nVersion     : OMX specification version information
 *  nPortIndex   : Port that this structure applies to
 *  bDigitalFlash : Digital flash type Enable/Disable -
 * Specifies whether the digital flash algorithm is enabled or disabled. This overrides the contrast and brightness settings.
 */
typedef struct OMX_CONFIG_DIGITALFLASHTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bDigitalFlash;
} OMX_CONFIG_DIGITALFLASHTYPE;



/**
 * Privacy Indicator Enable/Disable
 * STRUCT MEMBERS:
 *  nSize        : Size of the structure in bytes
 *  nVersion     : OMX specification version information
 *  nPortIndex   : Port that this structure applies to
 *  bPrivacyIndicator :
 *        Specifies whether the flash should be used to indicate image or video capture. When flash is not used for exposure,
 *        flash will be activated after exposure to indicate image capture.
 *        If video light is not used, the flash can be blinking or constant at low intensity to indicate capture but not affect exposure.
 *        Specifies whether the digital flash algorithm is enabled or disabled. This overrides the contrast and brightness settings.
 */
typedef struct OMX_CONFIG_PRIVACYINDICATOR {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bPrivacyIndicator;
} OMX_CONFIG_PRIVACYINDICATOR;


/**
 * Privacy Indicator Enable/Disable
 * STRUCT MEMBERS:
 *  nSize        : Size of the structure in bytes
 *  nVersion     : OMX specification version information
 *  nPortIndex   : Port that this structure applies to
 *  bTorchMode :
 *        Enable/Disable
 *      nIntensityLevel : relative intensity from 0 - 100
 *      nDuration : duration in msec
 */
typedef struct OMX_CONFIG_TORCHMODETYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bTorchMode;
    OMX_U32 nIntensityLevel;
    OMX_U32 nDuration;
} OMX_CONFIG_TORCHMODETYPE;



/**
 * Privacy Indicator Enable/Disable
 * DISABLE - Fire the xenon flash in the usual manner
 * ENABLE - Reduce the light intensity of the main flash (ex 1EV)
 *
 * STRUCT MEMBERS:
 *  nSize        : Size of the structure in bytes
 *  nVersion     : OMX specification version information
 *  nPortIndex   : Port that this structure applies to
 *  bSlowSync :
 *        Enable - OMX_TRUE/Disable - OMX_FALSE
 */
typedef struct OMX_CONFIG_SLOWSYNCTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bSlowSync;
} OMX_CONFIG_SLOWSYNCTYPE;


/**
 * Focus control extended enums. use this along with OMX_IMAGE_CONFIG_FOCUSCONTROLTYPE
 */
typedef enum OMX_IMAGE_EXTFOCUSCONTROLTYPE {
    OMX_IMAGE_FocusControlAutoMacro = 0x7F000001, /**< Reserved region for introducing Vendor Extensions */
    OMX_IMAGE_FocusControlAutoInfinity,
    OMX_IMAGE_FocusControlHyperfocal,
    OMX_IMAGE_FocusControlPortrait, /**< from Xena */
    OMX_IMAGE_FocusControlExtended, /**< from Xena */
    OMX_IMAGE_FocusControlContinousNormal, /**< from Xena */
    OMX_IMAGE_FocusControlContinousExtended /**< from Xena */
} OMX_IMAGE_EXTFOCUSCONTROLTYPE;



/**
 * Specifies whether the LED can be used to assist in autofocus, due to low lighting conditions.
 * ENABLE means use as determined by the auto exposure algorithm.
 *
 * STRUCT MEMBERS:
 *  nSize        : Size of the structure in bytes
 *  nVersion     : OMX specification version information
 *  nPortIndex   : Port that this structure applies to
 *  bFocusAssist :
 *        Enable - OMX_TRUE/Disable - OMX_FALSE
 */
typedef struct OMX_CONFIG_FOCUSASSISTTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bFocusAssist;
} OMX_CONFIG_FOCUSASSISTTYPE;



/**
 *for locking the focus
 *
 * STRUCT MEMBERS:
 *  nSize        : Size of the structure in bytes
 *  nVersion     : OMX specification version information
 *  nPortIndex   : Port that this structure applies to
 *  bFocusLock :
 *        Enable - OMX_TRUE/Disable - OMX_FALSE
 */
typedef struct OMX_CONFIG_FOCUSLOCKTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bFocusLock;
} OMX_CONFIG_FOCUSLOCKTYPE;


/**
 *for locking the White balance
 *
 * STRUCT MEMBERS:
 *  nSize        : Size of the structure in bytes
 *  nVersion     : OMX specification version information
 *  nPortIndex   : Port that this structure applies to
 *  bWhiteBalanceLock :
 *        Enable - OMX_TRUE/Disable - OMX_FALSE
 */
typedef struct OMX_CONFIG_WHITEBALANCELOCKTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bWhiteBalanceLock;
} OMX_CONFIG_WHITEBALANCELOCKTYPE;

/**
 *for locking the Exposure
 *
 * STRUCT MEMBERS:
 *  nSize        : Size of the structure in bytes
 *  nVersion     : OMX specification version information
 *  nPortIndex   : Port that this structure applies to
 *  bExposureLock :
 *        Enable - OMX_TRUE/Disable - OMX_FALSE
 */
typedef struct OMX_CONFIG_EXPOSURELOCKTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bExposureLock;
} OMX_CONFIG_EXPOSURELOCKTYPE;

/**
 *for locking the Exposure
 *  Simultaneously lock focus, white balance and exposure (and relevant other settings).
 *
 * STRUCT MEMBERS:
 *  nSize        : Size of the structure in bytes
 *  nVersion     : OMX specification version information
 *  nPortIndex   : Port that this structure applies to
 *  bAllLock :
 *        Enable - OMX_TRUE/Disable - OMX_FALSE
 */
typedef struct OMX_CONFIG_ALLLOCKTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bAllLock;
} OMX_CONFIG_ALLLOCKTYPE;

/**
 *for locking
 *  Simultaneously lock focus, white balance and exposure (and relevant other settings).
 *
 * STRUCT MEMBERS:
 *  nSize        : Size of the structure in bytes
 *  nVersion     : OMX specification version information
 *  nPortIndex   : Port that this structure applies to
 *  bLock :
 *        Enable - OMX_TRUE/Disable - OMX_FALSE
 *  bAtCapture:
 *
 */
typedef struct OMX_IMAGE_CONFIG_LOCKTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bLock;
    OMX_BOOL bAtCapture;
} OMX_IMAGE_CONFIG_LOCKTYPE;

/**
 * processig level types enum
 */
typedef enum OMX_PROCESSINGLEVEL{
        OMX_Min = 0,
        OMX_Low,
        OMX_Medium,
        OMX_High,
        OMX_Max,
        OMX_ProcessingLevelMax = 0x7fffffff
}OMX_PROCESSINGLEVEL;

/**
 *processing level type
 *  Simultaneously lock focus, white balance and exposure (and relevant other settings).
 *
 * STRUCT MEMBERS:
 *  nSize        : Size of the structure in bytes
 *  nVersion     : OMX specification version information
 *  nPortIndex   : Port that this structure applies to
 *  nLevel :
 *               nLevel hinting processing amount. Range of values is -100 to 100.
 *               0 causes no change to the image.  Increased values cause increased processing to occur, with 100 applying maximum processing.
 *               Negative values have the opposite effect of positive values.
 *  bAuto:
 *		sets if the processing should be applied according to input data.
 		It is allowed to combine the hint level with the auto setting,
 *		i.e. to give a bias to the automatic setting. When set to false, the processing should not take input data into account.
 */

typedef struct OMX_IMAGE_CONFIG_PROCESSINGLEVELTYPE {
OMX_U32 nSize;
OMX_VERSIONTYPE nVersion;
OMX_U32 nPortIndex;
OMX_S32 nLevel;
OMX_BOOL bAuto;
} OMX_IMAGE_CONFIG_PROCESSINGLEVELTYPE;


/**
 * White Balance control type extended enums - to be used along with the structure @OMX_CONFIG_WHITEBALCONTROLTYPE
 *
 *
 *
 */
typedef enum OMX_EXTWHITEBALCONTROLTYPE {
    OMX_WhiteBalControlFacePriorityMode = OMX_WhiteBalControlVendorStartUnused + 1, /**<  */
    OMX_TI_WhiteBalControlSunset,
    OMX_TI_WhiteBalControlShade,
    OMX_TI_WhiteBalControlTwilight,
    OMX_TI_WhiteBalControlWarmFluorescent
} OMX_EXTWHITEBALCONTROLTYPE;

/**
 *white balance gain type
 *  xWhiteBalanceGain and xWhiteBalanceOffset represents gain and offset for R, Gr, Gb, B channels respectively in Q16 format. \
 *  For example, new red pixel value = xWhiteBalanceGain[1]* the current pixel value + xWhiteBalanceOffset[1].
 *  All values assume that maximum value is 255. If internal implementation uses higher dynamic range, this value should be adjusted internally.
 *  nWhiteThreshhold  represents thresholds for "white" area measurments in Q16 format.
 *
 * STRUCT MEMBERS:
 *  nSize        : Size of the structure in bytes
 *  nVersion     : OMX specification version information
 *  nPortIndex   : Port that this structure applies to
 *
 */
typedef struct OMX_CONFIG_WHITEBALGAINTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_S32 xWhiteBalanceGain[4];
    OMX_S32 xWhiteBalanceOffset[4];
    OMX_S32 nWhiteThreshhold[4];
} OMX_CONFIG_WHITEBALGAINTYPE;

/**
 *  This structure represents linear color conversion from one space to another.  For example, to conversion from one RGB color into another RGB color space can be represented as
 *  R' =  xColorMatrix[1][1]*R + xColorMatrix[1][2]*G + xColorMatrix[1][3]*B + xColorOffset[1]
 *  G' = xColorMatrix[2][1]*R + xColorMatrix[2][2]*G + xColorMatrix[2][3]*B + xColorOffset[2]
 *  B' = xColorMatrix[3][1]*R + xColorMatrix[3][2]*G + xColorMatrix[3][3]*B + xColorOffset[3]
 *  Both xColorMatrix and xColorOffset are represented as Q16 value.
 *  bFullColorRange represents represents whether valid range of color is 0 to 255 (when set to TRUE) or 16 to 235 (for FALSE).
 *  Again all values assume that maximum value is 255. If internal implementation uses higher dynamic range, this value should be adjusted internally.
 *
 *
 * STRUCT MEMBERS:
 *  nSize        : Size of the structure in bytes
 *  nVersion     : OMX specification version information
 *  nPortIndex   : Port that this structure applies to
 *
 */
typedef struct OMX_CONFIG_EXT_COLORCONVERSIONTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_S32 xColorMatrix[3][3];
    OMX_S32 xColorOffset[3];
    OMX_BOOL bFullColorRange;
}OMX_CONFIG_EXT_COLORCONVERSIONTYPE;


/**
 * xGamma represents lool-up table for gamma correction in Q16 format.
 * All values assume that maximum value is 255. If internal implementation uses higher dynamic range, this value should be adjusted internally.
 *
 *
 *
 * STRUCT MEMBERS:
 *  nSize        : Size of the structure in bytes
 *  nVersion     : OMX specification version information
 *  nPortIndex   : Port that this structure applies to
 *
 */
typedef struct OMX_CONFIG_GAMMATABLETYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 xGamma[3][256];
}OMX_CONFIG_GAMMATABLETYPE;



/**
 * processig types
 */
typedef enum OMX_PROCESSINGTYPE{
        OMX_BloomingReduction = 0,
        OMX_Denoise,
        OMX_Sharpening,
        OMX_Deblurring,
        OMX_Demosaicing,
        OMX_ContrastEnhancement,
        OMX_ProcessingTypeMax = 0x7fffffff
}OMX_PROCESSINGTYPE;


typedef  struct OMX_CONFIGPROCESSINGORDERTYPE {
OMX_U32  nSize; /**< Size of the structure in bytes */
OMX_VERSIONTYPE nVersion; /**< OMX specification version info */
OMX_U32 nPortIndex; /**< Port that this struct applies to */
OMX_U32 nIndex;
OMX_PROCESSINGTYPE eProc;
} OMX_CONFIGPROCESSINGORDERTYPE;

/**
 * HIST TYPE
 */
typedef enum OMX_HISTTYPE{
        OMX_HistControlLuminance = 0, /**< Luminance histogram is calculated (Y)*/
        OMX_HistControlColorComponents, /**< A histogram per color component (R, G, B) is calculated*/
        OMX_HistControlChrominanceComponents /**< A histogram per chrominance component (Cb, Cr) is calculated.*/
}OMX_HISTTYPE;

/**
 * Histogram Setting
 *  nPortIndex is an output port. The port index decides on which port the extra data structur is sent on.
 *  bFrameLimited is a Boolean used to indicate if measurement shall be terminated after the specified number of
 *  frames if true frame limited measurement is enabled; otherwise the port does not terminate measurement until
 *  instructed to do so by the client.
 *  nFrameLimit is the limit on number of frames measured, this parameter is only valid if bFrameLimited is enabled.
 *  bMeasure is a Boolean that should be set to true when measurement shall begin, otherwise set to false. Query will give status information on if measurement is ongoing or not.
 *  nBins specifies the number of histogram bins. When queried with set to zero, the respons gives the maximum number of bins allowed.
 *  nLeft is the leftmost coordinate of the measurement area rectangle.
 *  nTop is the topmost coordinate of the measurement area rectangle.
 *  nWidth is the width of the measurement area rectangle in pixels.
 *  nHeight is the height of the measurement area rectangle in pixels.
 *  eHistType is an enumeration specifying the histogram type
 *
 *
 */

typedef struct OMX_CONFIG_HISTOGRAMTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bFrameLimited;
    OMX_U32 nFrameLimit;
    OMX_BOOL bMeasure;
    OMX_U32 nBins;
    OMX_S32 nLeft;
    OMX_S32 nTop;
    OMX_U32 nWidth;
    OMX_U32 nHeight;
    OMX_HISTTYPE eHistType;
} OMX_CONFIG_HISTOGRAMTYPE;

/**
 * Enums for HIST component type.
 */
typedef enum OMX_HISTCOMPONENTTYPE{
        OMX_HISTCOMP_Y = 0, /**<    Luminance histogram (Y) */
        OMX_HISTCOMP_YLOG,  /**< Logarithmic luminance histogram (Y)*/
        OMX_HISTCOMP_R, /**< Red histogram component (R)*/
        OMX_HISTCOMP_G, /**< Green histogram component (G)*/
        OMX_HISTCOMP_B, /**< Blue histogram component (B)*/
        OMX_HISTCOMP_Cb,    /**< Chroma blue histogram component (Cb)*/
        OMX_HISTCOMP_Cr /**< Chroma red histogram component (Cr) */
}OMX_HISTCOMPONENTTYPE;

 /**
 * The OMX_TI_CAMERAVIEWTYPE enumeration is used to identify the
 * particular camera view that the rest of the data in the structure is
 * associated with.
 */
typedef enum OMX_TI_CAMERAVIEWTYPE
{
    OMX_2D,     /**< Camera view in 2D sensor configuration */
    OMX_Left,   /**< Left camera view in stereo sensor configuration */
    OMX_Right,  /**< Right camera view in stereo sensor configuration */
    OMX_TI_CAMERAVIEWTYPE_32BIT_PATCH = 0x7FFFFFFF
} OMX_TI_CAMERAVIEWTYPE;
/**
 *  nSize is the size of the structure including the length of data field containing
 *  the histogram data.
 *  nBins is the number of bins in the histogram.
 *  eComponentType specifies the type of the histogram bins according to enum.
 *  It can be selected to generate multiple component types, then the extradata struct
 *  is repeated for each component type.
 *  data[1] first byte of the histogram data
 */
typedef struct OMX_HISTOGRAMTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_TI_CAMERAVIEWTYPE eCameraView;
    OMX_U32 nBins;
    OMX_HISTCOMPONENTTYPE eComponentType;
    OMX_U8  data[1];
} OMX_HISTOGRAMTYPE;

#define OMX_OTHER_EXTRADATATYPE_SIZE ( (OMX_U32)(((OMX_OTHER_EXTRADATATYPE*)0x0)->data) ) /**< Size of OMX_OTHER_EXTRADATATYPE**/
/**
 * The extra data having ancillary data is described with the following structure.
 * This data contains single flags and values
 * (not arrays) that have general usage for camera applications.
 */
typedef  struct OMX_TI_ANCILLARYDATATYPE {
    OMX_U32             nSize;
    OMX_VERSIONTYPE     nVersion;
    OMX_U32             nPortIndex;
    OMX_TI_CAMERAVIEWTYPE       eCameraView;
    OMX_U32             nAncillaryDataVersion;
    OMX_U32             nFrameNumber;
    OMX_U32             nShotNumber;
    OMX_U16             nInputImageHeight;
    OMX_U16             nInputImageWidth;
    OMX_U16             nOutputImageHeight;
    OMX_U16             nOutputImageWidth;
    OMX_U16             nDigitalZoomFactor;
    OMX_S16             nCropCenterColumn;
    OMX_S16             nCropCenterRow;
    OMX_U16             nOpticalZoomValue;
    OMX_U8              nFlashConfiguration;
    OMX_U8              nFlashUsage;
    OMX_U32             nFlashStatus;
    OMX_U8              nAFStatus;
    OMX_U8              nAWBStatus;
    OMX_U8              nAEStatus;
    OMX_U32             nExposureTime;
    OMX_U16             nEVCompensation;
    OMX_U8              nDigitalGainValue;
    OMX_U8              nAnalogGainValue;
    OMX_U16             nCurrentISO;
    OMX_U16             nReferenceISO;
    OMX_U8              nApertureValue;
    OMX_U8              nPixelRange;
    OMX_U16             nPixelAspectRatio;
    OMX_U8              nCameraShake;
    OMX_U16             nFocalDistance;
    OMX_U64             nParameterChangeFlags;
    OMX_U8              nNumFacesDetected;
    OMX_U8              nConvergenceMode;
    OMX_U8              nConvergenceStatus;
    OMX_U8              nDCCStatus;
} OMX_TI_ANCILLARYDATATYPE;

typedef struct OMX_TI_WHITEBALANCERESULTTYPE {
    OMX_U32             nSize;          /**< Size */
    OMX_VERSIONTYPE     nVersion;       /**< Version */
    OMX_U32             nPortIndex;     /**< Port Index */
    OMX_TI_CAMERAVIEWTYPE eCameraView;
    OMX_U16             nColorTemperature;      /**< White Balance Color Temperature in Kelvins */
    OMX_U16             nGainR;         /**< Bayer applied R color channel gain in (U13Q9) */
    OMX_U16             nGainGR;        /**< Bayer applied Gr color channel gain in (U13Q9) */
    OMX_U16             nGainGB;        /**< Bayer applied Gb color channel gain in (U13Q9) */
    OMX_U16             nGainB;         /**< Bayer applied B color channel gain in (U13Q9) */
    OMX_S16             nOffsetR;       /**< Bayer applied R color channel offset */
    OMX_S16             nOffsetGR;      /**< Bayer applied Gr color channel offset */
    OMX_S16             nOffsetGB;      /**< Bayer applied Gb color channel offset */
    OMX_S16             nOffsetB;       /**< Bayer applied B color channel offset */
} OMX_TI_WHITEBALANCERESULTTYPE;

/**
 * Unsaturated Regions data
 * The extra data having unsaturated regions data is
 * described with the following structure..
 */
typedef struct OMX_TI_UNSATURATEDREGIONSTYPE {
    OMX_U32             nSize;          /**< Size */
    OMX_VERSIONTYPE     nVersion;       /**< Version */
    OMX_U32             nPortIndex;     /**< Port Index */
    OMX_U16             nPaxelsX;       /**< The number of paxels in the horizontal direction */
    OMX_U16             nPaxelsY;       /**< The number of paxels in the vertical direction */
    OMX_U16             data[1];        /**< the first value of an array of values that represent */
} OMX_TI_UNSATURATEDREGIONSTYPE;

/**
 * OMX_BARCODETYPE
 */
typedef enum OMX_BARCODETYPE{
        OMX_BARCODE1D = 0,      /**< 1D barcode */
        OMX_BARCODE2D,          /**< 2D barcode */
}OMX_BARCODETYPE;
/**
 * Brcode detection data
 *	nLeft is the leftmost coordinate of the detected area rectangle.
 *	nTop is the topmost coordinate of the detected area rectangle.
 *	nWidth is the width of the detected area rectangle in pixels.
 *	nHeight is the height of the detected area rectangle in pixels.
 *	nOrientation is the orientation of the axis of the detected object. This refers to the angle between the vertical axis of barcode and the horizontal axis.
 *	eBarcodetype is an enumeration specifying the barcode type, as listed in the given table.
 */
typedef struct OMX_BARCODEDETECTIONTYPE {
	OMX_U32 nSize;
	OMX_VERSIONTYPE nVersion;
	OMX_U32 nPortIndex;
	OMX_TI_CAMERAVIEWTYPE eCameraView;
	OMX_S32 nLeft;
	OMX_S32 nTop;
	OMX_U32 nWidth;
	OMX_U32 nHeight;
	OMX_S32 nOrientation;
	OMX_BARCODETYPE eBarcodetype;
 } OMX_BARCODEDETECTIONTYPE;

/**
 * Front object detection data
 *	nLeft is the leftmost coordinate of the detected area rectangle.
 *	nTop is the topmost coordinate of the detected area rectangle.
 *	nWidth is the width of the detected area rectangle in pixels.
 *	nHeight is the height of the detected area rectangle in pixels.
 */
typedef struct OMX_FRONTOBJDETECTIONTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_TI_CAMERAVIEWTYPE eCameraView;
    OMX_S32 nLeft;
    OMX_S32 nTop;
    OMX_U32 nWidth;
    OMX_U32 nHeight;
} OMX_FRONTOBJDETECTIONTYPE;

/**
 * Distance estimation data
 * nDistance is the estimated distance to the object in millimeters.
 * nLargestDiscrepancy is the estimated largest discrepancy of the distance to the object in millimeters. When equal to MAX_INT the discrepancy is unknown.
 */
typedef struct OMX_DISTANCEESTIMATIONTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_TI_CAMERAVIEWTYPE eCameraView;
    OMX_U32 nDistance;
    OMX_U32 nLargestDiscrepancy;
} OMX_DISTANCEESTIMATIONTYPE;

/**
 * Distance estimation data
 * nDistance is the estimated distance to the object in millimeters.
 * nLargestDiscrepancy is the estimated largest discrepancy of the distance to the object in millimeters. When equal to MAX_INT the discrepancy is unknown.
 */

typedef struct OMX_MOTIONESTIMATIONTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_TI_CAMERAVIEWTYPE eCameraView;
    OMX_S32 nPanX;
    OMX_S32 nPanY;
} OMX_MOTIONESTIMATIONTYPE;


/**
 * Focus region data
 *	nRefPortIndex is the port the image frame size is defined on. This image frame size is used as reference for the focus region rectangle.
 *	nLeft is the leftmost coordinate of the focus region rectangle.
 *	nTop is the topmost coordinate of the focus region rectangle.
 *	nWidth is the width of the focus region rectangle in pixels.
 *	nHeight is the height of the focus region rectangle in pixels.
 *
 */
typedef struct OMX_FOCUSREGIONTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_TI_CAMERAVIEWTYPE eCameraView;
    OMX_U32 nRefPortIndex;
    OMX_S32 nLeft;
    OMX_S32 nTop;
    OMX_U32 nWidth;
    OMX_U32 nHeight;
} OMX_FOCUSREGIONTYPE;

/**
 * OMX_ISOSETTINGTYPE: specifies its auto or manual setting
 *
 */
typedef enum OMX_ISOSETTINGTYPE{
        OMX_Auto = 0, /**<	*/
        OMX_IsoManual	/**< */
}OMX_ISOSETTINGTYPE;

/**
 *  nSize is the size of the structure including the length of data field containing
 *  the histogram data.
 *  eISOMode:
 *  	specifies the ISO seetting mode - auto/manual
 *  nISOSetting:
 *  	for manual mode client can specify the ISO setting.
 */

typedef struct OMX_CONFIG_ISOSETTINGTYPE{
	OMX_U32 nSize;
	OMX_VERSIONTYPE nVersion;
	OMX_U32 nPortIndex;
	OMX_ISOSETTINGTYPE eISOMode;
	OMX_U32 nISOSetting;
}OMX_CONFIG_ISOSETTINGTYPE;

/**
 * custom RAW format
 */
typedef struct OMX_CONFIG_RAWFORMATTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_VERSIONTYPE nFormatVersion;
    OMX_STRING cVendorName;
} OMX_CONFIG_RAWFORMATTYPE;

/**
 * Sensor type
 */
typedef struct OMX_CONFIG_SENSORTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_VERSIONTYPE nSensorVersion;
    OMX_STRING cModelName;
} OMX_CONFIG_SENSORTYPE;

/**
 * Sensor custom data type
 */
typedef struct OMX_CONFIG_SENSORCUSTOMDATATYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nDataLength;
    OMX_U8 xSensorData[1];
} OMX_CONFIG_SENSORCUSTOMDATATYPE;

/**
 * OMX_OBJDETECTQUALITY
 *
 */
typedef enum OMX_OBJDETECTQUALITY{
        OMX_FastDetection = 0, /**< A detection that prioritizes speed*/
        OMX_Default,    /**< The default detection, should be used when no control of the detection quality is given.*/
        OMX_BetterDetection,    /**< A detection that levels correct detection with speed*/
        OMX_BestDtection,   /**< A detection that prioritizes correct detection*/
        OMX_AUTODETECTION   /**< Automatically decide which object detection quality is best.*/
}OMX_OBJDETECTQUALITY;

/**
 * OBJECT DETECTION Type
 *      nPortIndex: is an output port. The port index decides on which port the extra data structur of detected object is sent on.
 *      bEnable : this controls ON/OFF for this object detection algirithm.
 *      bFrameLimited: is a Boolean used to indicate if detection shall be terminated after the specified number of frames if
 *          true frame limited detection is enabled; otherwise the port does not terminate detection until instructed to do so by the client.
 *      nFrameLimit: is the limit on number of frames detection is executed for, this parameter is only valid if bFrameLimited is enabled.
 *      nMaxNbrObjects: specifies the maximum number of objects that should be found in each frame. It is implementation dependent which objects are found.
 *      nLeft: is the leftmost coordinate of the detection area rectangle.
 *      nTop: is the topmost coordinate of the detection area rectangle.
 *      nWidth: is the width of the detection area rectangle in pixels.
 *      nHeight: is the height of the detection area rectangle in pixels.
 *      eObjDetectQuality: is an enumeration specifying the quality desired by the detection.
 *      nPriority: represents priority of each object when there are multiple objects detected.
 */

typedef struct OMX_CONFIG_OBJDETECTIONTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bEnable;
    OMX_BOOL bFrameLimited;
    OMX_U32 nFrameLimit;
    OMX_U32 nMaxNbrObjects;
    OMX_S32 nLeft;
    OMX_S32 nTop;
    OMX_U32 nWidth;
    OMX_U32 nHeight;
    OMX_OBJDETECTQUALITY eObjDetectQuality;
    OMX_U32 nPriority;
    OMX_U32 nDeviceOrientation;
 } OMX_CONFIG_OBJDETECTIONTYPE;


/**
 * OMX_OBJDETECTQUALITY
 *
 */
typedef enum OMX_DISTTYPE{
        OMX_DistanceControlFocus = 0, /**< focus objects distance type*/
        OMX_DISTANCECONTROL_RECT	/**< Evaluated distance to the object found in the rectangelar area indicated as input region.  */
}OMX_DISTTYPE;


/**
 * Distance mesurement
 *	bStarted is a Boolean. The IL client sets it to true to start the measurement .
 *		the IL client sets to false to stop the measurement. The IL client can query it to check if the measurement is ongoing.
 *	nLeft : is the leftmost coordinate of the rectangle.
 *	nTop : is the topmost coordinate of the rectangle.
 *	nWidth:  is the width of the rectangle in pixels.
 *	nHeight:  is the height of the rectangle in pixels.
 *	eDistType:  is an enumeration specifying the distance measurement type, as shown in
 */
typedef struct OMX_CONFIG_DISTANCETYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bStarted;
    OMX_S32 nLeft;
    OMX_S32 nTop;
    OMX_U32 nWidth;
    OMX_U32 nHeight;
    OMX_DISTTYPE eDistType;
} OMX_CONFIG_DISTANCETYPE;


/**
 * face detect data - face attribute
 *  nARGBEyeColor: is the indicates a 32-bit eye color of the person, where bits 0-7 are blue,
 *      bits 15-8 are green, bits 24-16 are red, and bits 31-24 are for alpha.
 *  nARGBSkinColor: is the indicates a 32-bit skin color of the person, where bits 0-7 are blue,
 *      bits 15-8 are green, bits 24-16 are red, and bits 31-24 are for alpha.
 *  nARGBHairColor: is the indicates a 32-bit hair color of the person, where bits 0-7 are blue,
 *      bits 15-8 are green, bits 24-16 are red, and bits 31-24 are for alpha.
 *  nSmileScore: a smile detection score between 0 and 100, where 0 means not detecting,
 *      1 means least certain and 100 means most certain a smile is detected.
 *  nBlinkScore: a eye-blink detection score between 0 and 100, where 0 means not detecting,
 *      1 means least certain and 100 means most certain an eye-blink is detected.
 *  xIdentity: represents the identity of the face. With identity equal to zero this is not supported.
 *      This can be used by a face recognition application. The component shall not reuse an identity value unless the same face.
 *      Can be used to track detected faces when it moves between frames. Specific usage of this field is implementation dependent.
 *      It can be some kind of ID.
 *
 */
typedef struct OMX_FACEATTRIBUTE {
        OMX_U32 nARGBEyeColor;
    OMX_U32 nARGBSkinColor;
    OMX_U32 nARGBHairColor;
    OMX_U32 nSmileScore;
    OMX_U32 nBlinkScore;
    OMX_U32 xIdentity[4];
} OMX_FACEATTRIBUTE;

/**
 * xGamma represents lool-up table for gamma correction in Q16 format.
 * All values assume that maximum value is 255. If internal implementation uses higher dynamic range, this value should be adjusted internally.
 *
 *
 *
 * STRUCT MEMBERS:
 *  nScore: is a detection score between 0 and 100, where 0 means unknown score, 1 means least certain and 100 means most certain the detection is correct.
 *  nLeft: is the leftmost coordinate of the detected area rectangle.
 *  nTop: is the topmost coordinate of the detected area rectangle.
 *  nWidth: is the width of the detected area rectangle in pixels.
 *  nHeight: is the height of the detected area rectangle in pixels.
 *  nOrientationRoll/Yaw/Pitch is the orientation of the axis of the detected object. Here roll angle is defined as the angle between the vertical axis of face and the horizontal axis. All angles can have the value of -180 to 180 degree in Q16 format. Some face detection algorithm may not be able to fill in the angles, this is denoted by the use of MAX_INT value.
 *  nPriority represents priority of each object when there are multiple objects detected.
 *  nFaceAttr describe the attributes of the detected face object with the following structure:
 *
 *
 */
typedef struct OMX_TI_FACERESULT {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_TI_CAMERAVIEWTYPE eCameraView;
    OMX_U32 nScore;
    OMX_S32 nLeft;
    OMX_S32 nTop;
OMX_U32 nWidth;
OMX_U32 nHeight;
OMX_S32 nOrientationRoll;
OMX_S32 nOrientationYaw;
OMX_S32 nOrientationPitch;
OMX_U32 nPriority;
OMX_FACEATTRIBUTE nFaceAttr;
} OMX_TI_FACERESULT;


/**
 * Face detection data
 * The extra data having face detection data is described with the following structure.
 * The parser should only assume that the first tFacePosition[ulFaceCount] of the 35 elements
 * of the array should contain valid data.
 */
typedef struct OMX_FACEDETECTIONTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_TI_CAMERAVIEWTYPE    eCameraView;
    OMX_U16 ulFaceCount;
    OMX_TI_FACERESULT tFacePosition[35];// 35 is max faces supported by FDIF
} OMX_FACEDETECTIONTYPE;

/**
 * MTIS Vendor Specific Motion estimation
 * The extra data having MTIS motion estimation data is
 * described with the following structure.
 */
typedef struct OMX_TI_MTISTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_TI_CAMERAVIEWTYPE eCameraView;
    OMX_S32 nMaxMVh;            /**< The maximum MV for horizontal direction */
    OMX_S32 nMaxMVv;            /**< The maximum MV for vertical direction */
    OMX_U16 nMVRelY[9];         /**< The mask for MV reliability */
    OMX_U16 nMVRelX[9];         /**< The mask for MV reliability */
    OMX_S32 nMVh[9];            /**< The MVs for horizontal direction */
    OMX_S32 nMVv[9];            /**< The MVs for vertical direction */
} OMX_TI_MTISTYPE;

/**
 * The OMX_EXTRADATATYPE enumeration is used to define the
 * possible extra data payload types.
 */
typedef enum OMX_EXT_EXTRADATATYPE
{
   OMX_ExifAttributes = 0x7F000001, /**< Reserved region for introducing Vendor Extensions */
   OMX_AncillaryData,                   /**< 0x7F000002 ancillary data */
   OMX_WhiteBalance,                    /**< 0x7F000003 white balance resultant data */
   OMX_UnsaturatedRegions,              /**< 0x7F000004 unsaturated regions data */
   OMX_FaceDetection, /**< face detect data */
   OMX_BarcodeDetection, /**< bar-code detct data */
   OMX_FrontObjectDetection, /**< Front object detection data */
   OMX_MotionEstimation, /**< motion Estimation data */
   OMX_TI_MTISType,                     /**< 0x7F000009 MTIS motion Estimation data */
   OMX_DistanceEstimation, /**< disctance estimation */
   OMX_Histogram, /**< histogram */
   OMX_FocusRegion, /**< focus region data */
   OMX_ExtraDataPanAndScan,             /**< 0x7F00000D pan and scan data */
   OMX_RawFormat, /**< custom RAW data format */
   OMX_SensorType, /**< vendor & model of the sensor being used */
   OMX_SensorCustomDataLength, /**< vendor specific custom data length */
   OMX_SensorCustomData, /**< vendor specific data */
   OMX_TI_FrameLayout,                  /**< 0x7F000012 vendor specific data */
   OMX_TI_SEIinfo2004Frame1,    /**< 0x7F000013 Used for 2004 SEI message to be provided by video decoders */
   OMX_TI_SEIinfo2004Frame2,    /**< 0x7F000014 Used for 2004 SEI message to be provided by video decoders */
   OMX_TI_SEIinfo2010Frame1,    /**< 0x7F000015 Used for 2010 SEI message to be provided by video decoders */
   OMX_TI_SEIinfo2010Frame2,    /**< 0x7F000016 Used for 2010 SEI message to be provided by video decoders */
   OMX_TI_RangeMappingInfo,     /**< 0x7F000017 Used for Range mapping info provided by Video Decoders */
   OMX_TI_RescalingInfo,        /**< 0x7F000018 Used for width/height rescaling info provided by Video Decoders */
   OMX_TI_WhiteBalanceOverWrite,        /**< 0x7F000019 Used for manual AWB settings */
   OMX_TI_ExtraData_Count,
   OMX_TI_ExtraData_Max = OMX_TI_ExtraData_Count - 1,
} OMX_EXT_EXTRADATATYPE;


/**
 * Enable Extra-data on a specific port.
 *
 *
 *
 * STRUCT MEMBERS:
 *  nSize        : Size of the structure in bytes
 *  nVersion     : OMX specification version information
 *  nPortIndex   : Port on which this extra data to be assosiated
 *  eExtraDataType :  Extra data type
 *  bEnable      : Eneble/Disable this extra-data through port.
 *
 */
typedef struct OMX_CONFIG_EXTRADATATYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_EXT_EXTRADATATYPE eExtraDataType;
    OMX_TI_CAMERAVIEWTYPE eCameraView;
    OMX_BOOL bEnable;
} OMX_CONFIG_EXTRADATATYPE;

/**
 * JPEG header type
 * */

typedef enum OMX_JPEGHEADERTYPE{
	OMX_NoHeader = 0,
	OMX_JFIF,
	OMX_EXIF
}OMX_JPEGHEADERTYPE;
/**
 * Re-start marker configuration
 *
 *
 *
 * STRUCT MEMBERS:
 *  nSize        : Size of the structure in bytes
 *  nVersion     : OMX specification version information
 *  nPortIndex   : Port on which this extra data to be assosiated
 *  eJpegHeaderType : JPEG header type EXIF, JFIF, or No heeader.
 */

typedef struct OMX_CONFIG_JPEGHEEADERTYPE{
	OMX_U32 nSize;
	OMX_VERSIONTYPE nVersion;
	OMX_U32 nPortIndex;
	OMX_JPEGHEADERTYPE eJpegHeaderType;
}OMX_CONFIG_JPEGHEEADERTYPE;

/**
 * Re-start marker configuration
 *
 *
 *
 * STRUCT MEMBERS:
 *  nSize        : Size of the structure in bytes
 *  nVersion     : OMX specification version information
 *  nPortIndex   : Port on which this extra data to be assosiated
 *  nRstInterval :  interval at which RST markers are to be inserted.
 *  bEnable      : Eneble/Disable this RST marker insertion feature.
 *
 */

typedef struct OMX_CONFIG_RSTMARKER{
	OMX_U32 nSize;
	OMX_VERSIONTYPE nVersion;
	OMX_U32 nPortIndex;
	OMX_U32 nRstInterval;
	OMX_BOOL nEnable;
}OMX_CONFIG_RSTMARKER;

/**
 * Enable Extra-data on a specific port.
 *
 *
 *
 * STRUCT MEMBERS:
 *  nSize        : Size of the structure in bytes
 *  nVersion     : OMX specification version information
 *  nPortIndex   : Port that this structure applies to
 * nMaxSize : max size
 *
 *
 */
typedef struct OMX_IMAGE_JPEGMAXSIZE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nMaxSize;
} OMX_IMAGE_JPEGMAXSIZE;


typedef enum OMX_IMAGESTAMPOPERATION{
    OMX_NewImageStamp = 0,
    OMX_Continuation
}OMX_IMAGESTAMPOPERATION;


/**
 * Enable Extra-data on a specific port.
 *
 *
 *
 * STRUCT MEMBERS:
 *  nSize        : Size of the structure in bytes
 *  nVersion     : OMX specification version information
 *  nPortIndex   : Port that this structure applies to
 * nMaxSize : max size
 *
 *
 */
typedef struct OMX_PARAM_IMAGESTAMPOVERLAYTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_IMAGESTAMPOPERATION nOp;
    OMX_U32 nLeft;
    OMX_U32 nTop;
    OMX_U32 nHeight;
    OMX_U32 nWidth;
    OMX_COLOR_FORMATTYPE eFormat;
    OMX_U8 * pBitMap;
} OMX_PARAM_IMAGESTAMPOVERLAYTYPE;


/**
 * Enable Extra-data on a specific port.
 *
 *
 *
 * STRUCT MEMBERS:
 *  nSize        : Size of the structure in bytes
 *  nVersion     : OMX specification version information
 *  nPortIndex   : Port that this structure applies to
 * nMaxSize : max size
 *
 *
 */
typedef struct OMX_PARAM_THUMBNAILTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nHeight;
    OMX_U32 nWidth;
    OMX_IMAGE_CODINGTYPE eCompressionFormat;
    OMX_COLOR_FORMATTYPE eColorFormat;
    OMX_U32 nQuality;
    OMX_U32 nMaxSize;
} OMX_PARAM_THUMBNAILTYPE;

/**
 * Red-Eye Removal Enum
 */
typedef enum OMX_REDEYEREMOVALTYPE{
    OMX_RedEyeRemovalOff    = 0, /** No red eye removal*/
    OMX_RedEyeRemovalOn, /**    Red eye removal on*/
    OMX_RedEyeRemovalAuto,  /** Red eye removal will be done automatically when detected*/
    OMX_RedEyeRemovalKhronosExtensions = 0x6F000000,    /** Reserved region for introducing Khronos Standard Extensions*/
    OMX_RedEyeRemovalVendorStartUnused = 0x7F000000,    /** Reserved region for introducing Vendor Extensions*/
    OMX_RedEyeRemovalMax = 0x7FFFFFFF
}OMX_REDEYEREMOVALTYPE;

/**
 * Enable Extra-data on a specific port.
 *
 *
 *
 * STRUCT MEMBERS:
 *  nSize        : Size of the structure in bytes
 *  nVersion     : OMX specification version information
 *  nPortIndex   : Port that this structure applies to
 *  nLeft: is the leftmost coordinate of the detection area rectangle (such as face region).
 *  nTop: is the topmost coordinate of the detection area rectangle (such as face region).
 *  nWidth: is the width of the detection area rectangle  in pixels.
 *  nHeight: is the height of the detection area rectangle in pixels.
 *  nARGBEyeColor indicates a 32-bit eye color to replace the red-eye, where bits 0-7 are blue, bits 15-8 are green, bits 24-16 are red, and bits 31-24 are for alpha. When all zero indicates automatic choice.

 *
 */
typedef struct OMX_CONFIG_REDEYEREMOVALTYPE {
   OMX_U32 nSize;
   OMX_VERSIONTYPE nVersion;
   OMX_U32 nPortIndex;
   OMX_S32 nLeft;
   OMX_S32 nTop;
   OMX_U32 nWidth;
   OMX_U32 nHeight;
   OMX_U32 nARGBEyeColor;
   OMX_REDEYEREMOVALTYPE eMode;
} OMX_CONFIG_REDEYEREMOVALTYPE;






/**
 * Video capture YUV Range Enum
 */
typedef enum OMX_VIDEOYUVRANGETYPE{
    OMX_ITURBT601 = 0,
    OMX_Full8Bit,
    OMX_VideoYUVRangeKhronosExtensions = 0x6F000000,    /** Reserved region for introducing Khronos Standard Extensions*/
    OMX_VideoYUVRangeVendorStartUnused = 0x7F000000,    /** Reserved region for introducing Vendor Extensions*/
    OMX_VideoYUVRangeMax = 0x7FFFFFFF
}OMX_VIDEOYUVRANGETYPE;

/**
 * Enable Extra-data on a specific port.
 *
 *
 *
 * STRUCT MEMBERS:
 *  nSize        : Size of the structure in bytes
 *  nVersion     : OMX specification version information
 *  nPortIndex   : Port that this structure applies to
 *
 */
typedef struct OMX_PARAM_VIDEOYUVRANGETYPE {
   OMX_U32 nSize;
   OMX_VERSIONTYPE nVersion;
   OMX_U32 nPortIndex;
   OMX_VIDEOYUVRANGETYPE eYUVRange;
} OMX_PARAM_VIDEOYUVRANGETYPE;

/**
 * Video noise filter mode range enum
 */
typedef enum OMX_VIDEONOISEFILTERMODETYPE{
    OMX_VideoNoiseFilterModeOff = 0,
    OMX_VideoNoiseFilterModeOn,
    OMX_VideoNoiseFilterModeAuto,
    OMX_VideoNoiseFilterModeExtensions = 0x6F000000,    /** Reserved region for introducing Khronos Standard Extensions */
    OMX_VideoNoiseFilterModeStartUnused = 0x7F000000,   /** Reserved region for introducing Vendor Extensions */
    OMX_VideoNoiseFilterModeMax = 0x7FFFFFFF
} OMX_VIDEONOISEFILTERMODETYPE;

/**
 * Enable video noise filter.
 *
 * STRUCT MEMBERS:
 *  nSize       : Size of the structure in bytes
 *  nVersion    : OMX specification version information
 *  nPortIndex  : Port that this structure applies to
 *  eMode       : Video noise filter mode (on/off/auto)
 */
typedef struct OMX_PARAM_VIDEONOISEFILTERTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_VIDEONOISEFILTERMODETYPE eMode;
} OMX_PARAM_VIDEONOISEFILTERTYPE;


/**
 * High ISO Noise filter mode range enum
 */
typedef enum OMX_ISONOISEFILTERMODETYPE{
    OMX_ISONoiseFilterModeOff = 0,
    OMX_ISONoiseFilterModeOn,
    OMX_ISONoiseFilterModeAuto,
    OMX_ISONoiseFilterModeExtensions = 0x6F000000,    /** Reserved region for introducing Khronos Standard Extensions */
    OMX_ISONoiseFilterModeStartUnused = 0x7F000000,   /** Reserved region for introducing Vendor Extensions */
    OMX_ISONoiseFilterModeMax = 0x7FFFFFFF
} OMX_ISONOISEFILTERMODETYPE;

/**
 * Enable ISO noise filter.
 *
 * STRUCT MEMBERS:
 *  nSize       : Size of the structure in bytes
 *  nVersion    : OMX specification version information
 *  nPortIndex  : Port that this structure applies to
 *  eMode       : ISO noise filter (NSF2 is used) mode (on/off/auto)
 */
typedef struct OMX_PARAM_ISONOISEFILTERTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_ISONOISEFILTERMODETYPE eMode;
} OMX_PARAM_ISONOISEFILTERTYPE;

/**
 * Structure used to to call OMX_GetParams() for each
 * increment of "Index" starting with "0"
 *
 * STRUCT MEMBERS:
 * nSize            : Size of the structure in bytes
 * nVersion         : OMX specification version information
 * nIndex           : Index of the sDCCURI 0..MAX_URI_LENGTH
 * sDCCURI          : Look-up table containing strings. Ends with '\0'
 */
typedef struct OMX_TI_PARAM_DCCURIINFO {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nIndex;
    OMX_S8 sDCCURI[MAX_URI_LENGTH];
} OMX_TI_PARAM_DCCURIINFO;

/**
 * Structure used to configure DCC buffer
 *
 * STRUCT MEMBERS:
 * nSize            : Size of the structure in bytes
 * nVersion         : OMX specification version information
 * nDCCURIBuffSize  : Size of the pDCCURIBuff in bytes
 * pDCCURIBuff      : Pointer to a buffer
 */
typedef struct OMX_TI_PARAM_DCCURIBUFFER {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nDCCURIBuffSize;
    OMX_U8 *pDCCURIBuff;
} OMX_TI_PARAM_DCCURIBUFFER;

/**
 * Manual White Balance color temperature
 * STRUCT MEMBERS:
 *  nSize        : Size of the structure in bytes
 *  nVersion     : OMX specification version information
 *  nPortIndex   : Port that this structure applies to
 *  nColorTemperature : Color Temperature in K
 */
typedef struct OMX_TI_CONFIG_WHITEBALANCECOLORTEMPTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nColorTemperature;
} OMX_TI_CONFIG_WHITEBALANCECOLORTEMPTYPE;

/**
 * Focus spot weighting range enum
 */
typedef enum OMX_TI_CONFIG_FOCUSSPOTMODETYPE {
    OMX_FocusSpotDefault = 0,                           /** Makes CommonFocusRegion to be used. */
    OMX_FocusSpotSinglecenter,
    OMX_FocusSpotMultiNormal,
    OMX_FocusSpotMultiAverage,
    OMX_FocusSpotMultiCenter,
    OMX_FocusSpotExtensions = 0x6F000000,               /** Reserved region for introducing Khronos Standard Extensions */
    OMX_FocusSpotModeStartUnused = 0x7F000000,          /** Reserved region for introducing Vendor Extensions */
    OMX_FocusSpotModeMax = 0x7FFFFFFF
} OMX_TI_CONFIG_FOCUSSPOTMODETYPE;

/**
 * Focus Spot Weighting configuration.
 *
 * STRUCT MEMBERS:
 *  nSize       : Size of the structure in bytes
 *  nVersion    : OMX specification version information
 *  nPortIndex  : Port that this structure applies to
 *  eMode       : Spot Weighting mode
 */
typedef struct OMX_TI_CONFIG_FOCUSSPOTWEIGHTINGTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_TI_CONFIG_FOCUSSPOTMODETYPE eMode;
} OMX_TI_CONFIG_FOCUSSPOTWEIGHTINGTYPE;

/**
 * Enumeration of possible Exposure control types for OMX_EXPOSURECONTROLTYPE
 */
typedef enum OMX_TI_EXTEXPOSURECONTROLTYPE {
    OMX_TI_ExposureControlVeryLong = OMX_ExposureControlVendorStartUnused + 1
} OMX_TI_EXTEXPOSURECONTROLTYPE;

/**
 * Variable frame rate configuration.
 *
 * STRUCT MEMBERS:
 *  nSize         : Size of the structure in bytes
 *  nVersion      : OMX specification version information
 *  nPortIndex    : Port that this structure applies to
 *  xMinFramerate : Minimum variable frame rate value
 */
typedef struct OMX_TI_PARAM_VARFRAMERATETYPE {
	OMX_U32 nSize;
	OMX_VERSIONTYPE nVersion;
	OMX_U32 nPortIndex;
	OMX_U32 xMinFramerate;
} OMX_TI_PARAM_VARFRAMERATETYPE;

/**
 * Exposure config for right frame
 */
typedef struct OMX_TI_CONFIG_EXPOSUREVALUERIGHTTYPE {
	OMX_U32 nSize;
	OMX_VERSIONTYPE nVersion;
	OMX_U32 nPortIndex;
	OMX_U32 nApertureFNumber;  /**< e.g. nApertureFNumber = 2 implies "f/2" - Q16 format */
	OMX_U32 nShutterSpeedMsec; /**< Shutterspeed in milliseconds */
	OMX_U32 nSensitivity;      /**< e.g. nSensitivity = 100 implies "ISO 100" */
} OMX_TI_CONFIG_EXPOSUREVALUERIGHTTYPE;

/**
 * Auto Convergence mode enum
 */
typedef enum OMX_TI_AUTOCONVERGENCEMODETYPE {
	OMX_TI_AutoConvergenceModeDisable,
	OMX_TI_AutoConvergenceModeFrame,
	OMX_TI_AutoConvergenceModeCenter,
	OMX_TI_AutoConvergenceModeFocusFaceTouch,
	OMX_TI_AutoConvergenceModeManual,
	OMX_TI_AutoConvergenceExtensions = 0x6F000000,    /** Reserved region for introducing Khronos Standard Extensions */
	OMX_TI_AutoConvergenceStartUnused = 0x7F000000,   /** Reserved region for introducing Vendor Extensions */
	OMX_TI_AutoConvergenceModeMax = 0x7FFFFFFF
} OMX_TI_AUTOCONVERGENCEMODETYPE;

/**
 * Variable farame rate configuration.
 *
 * STRUCT MEMBERS:
 *  nSize             : Size of the structure in bytes
 *  nVersion          : OMX specification version information
 *  nPortIndex        : Port that this structure applies to
 *  eACMode           : Auto convergence mode
 *  nManualConverence : Manual Converence value
 *  nACProcWinStartX  : Start X AC Window
 *  nACProcWinStartY  : Start Y AC Window
 *  nACProcWinWidth   : Width of AC Window
 *  nACProcWinHeight  : Height of AC Window
 *  bACStatus         : output status from AL alg
 */
typedef struct OMX_TI_CONFIG_CONVERGENCETYPE {
	OMX_U32 nSize;
	OMX_VERSIONTYPE nVersion;
	OMX_U32 nPortIndex;
	OMX_TI_AUTOCONVERGENCEMODETYPE eACMode;
	OMX_S32 nManualConverence;
	OMX_U32 nACProcWinStartX;
	OMX_U32 nACProcWinStartY;
	OMX_U32 nACProcWinWidth;
	OMX_U32 nACProcWinHeight;
	OMX_BOOL bACStatus;
} OMX_TI_CONFIG_CONVERGENCETYPE;

/**
 * Camera specific version.
 *
 * STRUCT MEMBERS:
 *  nBranch        : Branch
 *  nCommitID      : Commit ID
 *  nBuildDateTime : Build date and time
 *  nExtraInfo     : rederved for future use
 */
typedef struct OMX_TI_CAMERASPECVERSIONTYPE {
	OMX_U8 nBranch[64];
	OMX_U8 nCommitID[64];
	OMX_U8 nBuildDateTime[64];
	OMX_U8 nExtraInfo[64];
} OMX_TI_CAMERASPECVERSIONTYPE;

/**
 * Stereo frame layout enum
 */
typedef enum OMX_TI_STEREOFRAMELAYOUTTYPE {
	OMX_TI_StereoFrameLayout2D,
	OMX_TI_StereoFrameLayoutTopBottom,
	OMX_TI_StereoFrameLayoutLeftRight,
	OMX_TI_StereoFrameLayoutMax = 0x7FFFFFFF
} OMX_TI_STEREOFRAMELAYOUTTYPE;

/**
 * Camera frame layout type.
 *
 * STRUCT MEMBERS:
 *  eFrameLayout    : frame layout
 *  nSubsampleRatio : subsample ratio
 */
typedef struct OMX_TI_FRAMELAYOUTTYPE {
	OMX_U32 nSize;
	OMX_VERSIONTYPE nVersion;
	OMX_U32 nPortIndex;
	OMX_TI_STEREOFRAMELAYOUTTYPE eFrameLayout;
	OMX_U32 nSubsampleRatio; /**  Subsampling ratio, Q15.7 */
} OMX_TI_FRAMELAYOUTTYPE;

/**
 * The OMX_TI_COLOR_FORMATTYPE enumeration is used to define the
 * extended color format types.
 */
typedef enum OMX_TI_COLOR_FORMATTYPE {
	OMX_TI_COLOR_FormatYUV420PackedSemiPlanarInterlaced =
	    (OMX_COLOR_FORMATTYPE) OMX_COLOR_FormatVendorStartUnused + 1,
	OMX_TI_COLOR_FormatRawBayer10bitStereo =
	    OMX_COLOR_FormatVendorStartUnused + 2, /**< 10 bit raw for stereo */
	OMX_TI_COLOR_FormatYUV420PackedSemiPlanar =
            (OMX_COLOR_FORMATTYPE) OMX_COLOR_FormatVendorStartUnused  + 0x100, /* 0x100 is used since it is the corresponding HAL pixel fromat */
        OMX_COLOR_FormatAndroidOpaque =
	    (OMX_COLOR_FORMATTYPE) OMX_COLOR_FormatVendorStartUnused  + 0x789 /**< Platform specified opaque format set to unique value 0x789*/
} OMX_TI_COLOR_FORMATTYPE;

/**
 * The OMX_TI_EXIFTAGSTATUS enumeration is used to define the
 * tag status types.
 */
typedef enum OMX_TI_EXIFTAGSTATUS {
	OMX_TI_TagReadOnly,     /**< implies this tag is generated within omx-camera >*/
	OMX_TI_TagReadWrite,    /**< implies this tag can be overwritten by client >*/
	OMX_TI_TagUpdated,      /**< client has to use this to indicate the specific tag is overwritten >*/
	OMX_TI_ExifStatus_Max = 0x7fffffff
} OMX_TI_EXIFTAGSTATUS;

typedef struct OMX_TI_CONFIG_EXIF_TAGS {
	OMX_U32                 nSize;
	OMX_VERSIONTYPE         nVersion;
	OMX_U32                 nPortIndex;
	OMX_TI_EXIFTAGSTATUS    eStatusImageWidth;
	OMX_U32                 ulImageWidth;
	OMX_TI_EXIFTAGSTATUS    eStatusImageHeight;
	OMX_U32                 ulImageHeight;
	OMX_TI_EXIFTAGSTATUS    eStatusBitsPerSample;
	OMX_U16                 usBitsPerSample[3];
	OMX_TI_EXIFTAGSTATUS    eStatusCompression;
	OMX_U16                 usCompression;
	OMX_TI_EXIFTAGSTATUS    eStatusPhotometricInterpretation;
	OMX_U16                 usPhotometricInterpretation;
	OMX_TI_EXIFTAGSTATUS    eStatusOrientation;
	OMX_U16                 usOrientation;
	OMX_TI_EXIFTAGSTATUS    eStatusSamplesPerPixel;
	OMX_U16                 usSamplesPerPixel;
	OMX_TI_EXIFTAGSTATUS    eStatusPlanarConfiguration;
	OMX_U16                 usPlanarConfiguration;
	OMX_TI_EXIFTAGSTATUS    eStatusYCbCrSubSampling;
	OMX_U16                 usYCbCrSubSampling[2];
	OMX_TI_EXIFTAGSTATUS    eStatusYCbCrPositioning;
	OMX_U16                 usYCbCrPositioning;
	OMX_TI_EXIFTAGSTATUS    eStatusXResolution;
	OMX_U32                 ulXResolution[2];
	OMX_TI_EXIFTAGSTATUS    eStatusYResolution;
	OMX_U32                 ulYResolution[2];
	OMX_TI_EXIFTAGSTATUS    eStatusResolutionUnit;
	OMX_U16                 usResolutionUnit;

	OMX_TI_EXIFTAGSTATUS    eStatusRowsPerStrip;
	OMX_U32                 ulRowsPerStrip;
	OMX_TI_EXIFTAGSTATUS    eStatusDataSize;
	OMX_U32                 ulDataSize;

	OMX_TI_EXIFTAGSTATUS    eStatusTransferFunction;
	OMX_U16                 usTransferFunction[3*256];
	OMX_TI_EXIFTAGSTATUS    eStatusWhitePoint;
	OMX_U32                 ulWhitePoint[4]; //2x2
	OMX_TI_EXIFTAGSTATUS    eStatusPrimaryChromaticities;
	OMX_U32                 ulPrimaryChromaticities[12]; //2x6
	OMX_TI_EXIFTAGSTATUS    eStatusYCbCrCoefficients;
	OMX_U32                 ulYCbCrCoefficients[6]; //2x3
	OMX_TI_EXIFTAGSTATUS    eStatusReferenceBlackWhite;
	OMX_U32                 ulReferenceBlackWhite[12]; //2x6
	OMX_TI_EXIFTAGSTATUS    eStatusDateTime;
	OMX_S8*                 pDateTimeBuff;
	OMX_U32                 ulDateTimeBuffSizeBytes;
	OMX_TI_EXIFTAGSTATUS    eStatusImageDescription;
	OMX_S8*                 pImageDescriptionBuff;
	OMX_U32                 ulImageDescriptionBuffSizeBytes;
	OMX_TI_EXIFTAGSTATUS    eStatusMake;
	OMX_S8*                 pMakeBuff;
	OMX_U32                 ulMakeBuffSizeBytes;
	OMX_TI_EXIFTAGSTATUS    eStatusModel;
	OMX_S8*                 pModelBuff;
	OMX_U32                 ulModelBuffSizeBytes;
	OMX_TI_EXIFTAGSTATUS    eStatusSoftware;
	OMX_S8*                 pSoftwareBuff;
	OMX_U32                 ulSoftwareBuffSizeBytes;
	OMX_TI_EXIFTAGSTATUS    eStatusArtist;
	OMX_S8*                 pArtistBuff;
	OMX_U32                 ulArtistBuffSizeBytes;
	OMX_TI_EXIFTAGSTATUS    eStatusCopyright;
	OMX_S8*                 pCopyrightBuff;
	OMX_U32                 ulCopyrightBuffSizeBytes;

	OMX_TI_EXIFTAGSTATUS    eStatusExifVersion;
	OMX_S8                  cExifVersion[4];
	OMX_TI_EXIFTAGSTATUS    eStatusFlashpixVersion;
	OMX_S8                  cFlashpixVersion[4];
	OMX_TI_EXIFTAGSTATUS    eStatusColorSpace;
	OMX_U16                 usColorSpace;
	OMX_TI_EXIFTAGSTATUS    eStatusComponentsConfiguration;
	OMX_S8                  cComponentsConfiguration[4];
	OMX_TI_EXIFTAGSTATUS    eStatusCompressedBitsPerPixel;
	OMX_U32                 ulCompressedBitsPerPixel[2];
	OMX_TI_EXIFTAGSTATUS    eStatusPixelXDimension;
	OMX_U32                 ulPixelXDimension;
	OMX_TI_EXIFTAGSTATUS    eStatusPixelYDimension;
	OMX_U32                 ulPixelYDimension;
	OMX_TI_EXIFTAGSTATUS    eStatusMakerNote;
	OMX_S8*                 pMakerNoteBuff;
	OMX_U32                 ulMakerNoteBuffSizeBytes;
	OMX_TI_EXIFTAGSTATUS    eStatusUserComment;
	OMX_S8*                 pUserCommentBuff;
	OMX_U32                 ulUserCommentBuffSizeBytes;
	OMX_TI_EXIFTAGSTATUS    eStatusRelatedSoundFile;
	OMX_S8                  cRelatedSoundFile[13];
	OMX_TI_EXIFTAGSTATUS    eStatusDateTimeOriginal;
	OMX_S8*                 pDateTimeOriginalBuff;
	OMX_U32                 ulDateTimeOriginalBuffSizeBytes;
	OMX_TI_EXIFTAGSTATUS    eStatusDateTimeDigitized;
	OMX_S8*                 pDateTimeDigitizedBuff;
	OMX_U32                 ulDateTimeDigitizedBuffSizeBytes;
	OMX_TI_EXIFTAGSTATUS    eStatusSubSecTime;
	OMX_S8*                 pSubSecTimeBuff;
	OMX_U32                 ulSubSecTimeBuffSizeBytes;
	OMX_TI_EXIFTAGSTATUS    eStatusSubSecTimeOriginal;
	OMX_S8*                 pSubSecTimeOriginalBuff;
	OMX_U32                 ulSubSecTimeOriginalBuffSizeBytes;
	OMX_TI_EXIFTAGSTATUS    eStatusSubSecTimeDigitized;
	OMX_S8*                 pSubSecTimeDigitizedBuff;
	OMX_U32                 ulSubSecTimeDigitizedBuffSizeBytes;

	OMX_TI_EXIFTAGSTATUS    eStatusExposureTime;
	OMX_U32                 ulExposureTime[2];
	OMX_TI_EXIFTAGSTATUS    eStatusFNumber;
	OMX_U32                 ulFNumber[2];
	OMX_TI_EXIFTAGSTATUS    eStatusExposureProgram;
	OMX_U16                 usExposureProgram;
	OMX_TI_EXIFTAGSTATUS    eStatusSpectralSensitivity;
	OMX_S8*                 pSpectralSensitivityBuff;
	OMX_U32                 ulSpectralSensitivityBuffSizeBytes;
	OMX_TI_EXIFTAGSTATUS    eStatusISOCount;
	OMX_U16                 usISOCount;
	OMX_TI_EXIFTAGSTATUS    eStatusISOSpeedRatings;
	OMX_U16*                pISOSpeedRatings;
	OMX_TI_EXIFTAGSTATUS    eStatusOECF;
	OMX_S8*                 pOECFBuff;
	OMX_U32                 ulOECFBuffSizeBytes;
	OMX_TI_EXIFTAGSTATUS    eStatusShutterSpeedValue;
	OMX_S32                 slShutterSpeedValue[2];
	OMX_TI_EXIFTAGSTATUS    eStatusApertureValue;
	OMX_U32                 ulApertureValue[2];
	OMX_TI_EXIFTAGSTATUS    eStatusBrightnessValue;
	OMX_S32                 slBrightnessValue[2];
	OMX_TI_EXIFTAGSTATUS    eStatusExposureBiasValue;
	OMX_S32                 slExposureBiasValue[2];
	OMX_TI_EXIFTAGSTATUS    eStatusMaxApertureValue;
	OMX_U32                 ulMaxApertureValue[2];
	OMX_TI_EXIFTAGSTATUS    eStatusSubjectDistance;
	OMX_U32                 ulSubjectDistance[2];
	OMX_TI_EXIFTAGSTATUS    eStatusMeteringMode;
	OMX_U16                 usMeteringMode;
	OMX_TI_EXIFTAGSTATUS    eStatusLightSource;
	OMX_U16                 usLightSource;
	OMX_TI_EXIFTAGSTATUS    eStatusFlash;
	OMX_U16                 usFlash;
	OMX_TI_EXIFTAGSTATUS    eStatusFocalLength;
	OMX_U32                 ulFocalLength[2];
	OMX_TI_EXIFTAGSTATUS    eStatusSubjectArea;
	OMX_U16                 usSubjectArea[4];
	OMX_TI_EXIFTAGSTATUS    eStatusFlashEnergy;
	OMX_U32                 ulFlashEnergy[2];
	OMX_TI_EXIFTAGSTATUS    eStatusSpatialFrequencyResponse;
	OMX_S8*                 pSpatialFrequencyResponseBuff;
	OMX_U32                 ulSpatialFrequencyResponseBuffSizeBytes;
	OMX_TI_EXIFTAGSTATUS    eStatusFocalPlaneXResolution;
	OMX_U32                 ulFocalPlaneXResolution[2];
	OMX_TI_EXIFTAGSTATUS    eStatusFocalPlaneYResolution;
	OMX_U32                 ulFocalPlaneYResolution[2];
	OMX_TI_EXIFTAGSTATUS    eStatusFocalPlaneResolutionUnit;
	OMX_U16                 usFocalPlaneResolutionUnit;
	OMX_TI_EXIFTAGSTATUS    eStatusSubjectLocation;
	OMX_U16                 usSubjectLocation[2];
	OMX_TI_EXIFTAGSTATUS    eStatusExposureIndex;
	OMX_U32                 ulExposureIndex[2];
	OMX_TI_EXIFTAGSTATUS    eStatusSensingMethod;
	OMX_U16                 usSensingMethod;
	OMX_TI_EXIFTAGSTATUS    eStatusFileSource;
	OMX_S8                  cFileSource;
	OMX_TI_EXIFTAGSTATUS    eStatusSceneType;
	OMX_S8                  cSceneType;
	OMX_TI_EXIFTAGSTATUS    eStatusCFAPattern;
	OMX_S8*                 pCFAPatternBuff;
	OMX_U32                 ulCFAPatternBuffSizeBytes;
	OMX_TI_EXIFTAGSTATUS    eStatusCustomRendered;
	OMX_U16                 usCustomRendered;
	OMX_TI_EXIFTAGSTATUS    eStatusExposureMode;
	OMX_U16                 usExposureMode;
	OMX_TI_EXIFTAGSTATUS    eStatusWhiteBalance;
	OMX_U16                 usWhiteBalance;
	OMX_TI_EXIFTAGSTATUS    eStatusDigitalZoomRatio;
	OMX_U32                 ulDigitalZoomRatio[2];
	OMX_TI_EXIFTAGSTATUS    eStatusFocalLengthIn35mmFilm;
	OMX_U16                 usFocalLengthIn35mmFilm;
	OMX_TI_EXIFTAGSTATUS    eStatusSceneCaptureType;
	OMX_U16                 usSceneCaptureType;
	OMX_TI_EXIFTAGSTATUS    eStatusGainControl;
	OMX_U16                 usGainControl;
	OMX_TI_EXIFTAGSTATUS    eStatusContrast;
	OMX_U16                 usContrast;
	OMX_TI_EXIFTAGSTATUS    eStatusSaturation;
	OMX_U16                 usSaturation;
	OMX_TI_EXIFTAGSTATUS    eStatusSharpness;
	OMX_U16                 usSharpness;
	OMX_TI_EXIFTAGSTATUS    eStatusDeviceSettingDescription;
	OMX_S8*                 pDeviceSettingDescriptionBuff;
	OMX_U32                 ulDeviceSettingDescriptionBuffSizeBytes;
	OMX_TI_EXIFTAGSTATUS    eStatusSubjectDistanceRange;
	OMX_U16                 usSubjectDistanceRange;

	OMX_TI_EXIFTAGSTATUS    eStatusImageUniqueID;
	OMX_S8                  cImageUniqueID[33];
	OMX_U8*                 pPrivateNextIFDPointer;    //Should not be used by the application
	OMX_U8*                 pPrivateThumbnailSize;     //Should not be used by the application
	OMX_U8*                 pPrivateTiffHeaderPointer; //Should not be used by the application

	OMX_TI_EXIFTAGSTATUS    eStatusGpsVersionId;
	OMX_U8                  ucGpsVersionId[4];
	OMX_TI_EXIFTAGSTATUS    eStatusGpslatitudeRef;
	OMX_S8                  cGpslatitudeRef[2];
	OMX_TI_EXIFTAGSTATUS    eStatusGpsLatitude;
	OMX_U32                 ulGpsLatitude[6];
	OMX_TI_EXIFTAGSTATUS    eStatusGpsLongitudeRef;
	OMX_S8                  cGpsLongitudeRef[2];
	OMX_TI_EXIFTAGSTATUS    eStatusGpsLongitude;
	OMX_U32                 ulGpsLongitude[6];
	OMX_TI_EXIFTAGSTATUS    eStatusGpsAltitudeRef;
	OMX_U8                  ucGpsAltitudeRef;
	OMX_TI_EXIFTAGSTATUS    eStatusGpsAltitude;
	OMX_U32                 ulGpsAltitude[2];
	OMX_TI_EXIFTAGSTATUS    eStatusGpsTimeStamp;
	OMX_U32                 ulGpsTimeStamp[6];
	OMX_TI_EXIFTAGSTATUS    eStatusGpsSatellites;
	OMX_S8*                 pGpsSatellitesBuff;
	OMX_U32                 ulGpsSatellitesBuffSizeBytes;
	OMX_TI_EXIFTAGSTATUS    eStatusGpsStatus;
	OMX_S8                  cGpsStatus[2];
	OMX_TI_EXIFTAGSTATUS    eStatusGpsMeasureMode;
	OMX_S8                  cGpsMeasureMode[2];
	OMX_TI_EXIFTAGSTATUS    eStatusGpsDop;
	OMX_U32                 ulGpsDop[2];
	OMX_TI_EXIFTAGSTATUS    eStatusGpsSpeedRef;
	OMX_S8                  cGpsSpeedRef[2];
	OMX_TI_EXIFTAGSTATUS    eStatusGpsSpeed;
	OMX_U32                 ulGpsSpeed[2];
	OMX_TI_EXIFTAGSTATUS    eStatusGpsTrackRef;
	OMX_S8                  cGpsTrackRef[2];
	OMX_TI_EXIFTAGSTATUS    eStatusGpsTrack;
	OMX_U32                 ulGpsTrack[2];
	OMX_TI_EXIFTAGSTATUS    eStatusGpsImgDirectionRef;
	OMX_S8                  cGpsImgDirectionRef[2];
	OMX_TI_EXIFTAGSTATUS    eStatusGpsImgDirection;
	OMX_U32                 ulGpsImgDirection[2];
	OMX_TI_EXIFTAGSTATUS    eStatusGpsMapDatum;
	OMX_S8*                 pGpsMapDatumBuff;
	OMX_U32                 ulGpsMapDatumBuffSizeBytes;
	OMX_TI_EXIFTAGSTATUS    eStatusGpsDestLatitudeRef;
	OMX_S8                  cGpsDestLatitudeRef[2];
	OMX_TI_EXIFTAGSTATUS    eStatusGpsDestLatitude;
	OMX_U32                 ulGpsDestLatitude[6];
	OMX_TI_EXIFTAGSTATUS    eStatusGpsDestLongitudeRef;
	OMX_S8                  cGpsDestLongitudeRef[2];
	OMX_TI_EXIFTAGSTATUS    eStatusGpsDestLongitude;
	OMX_U32                 ulGpsDestLongitude[6];
	OMX_TI_EXIFTAGSTATUS    eStatusGpsDestBearingRef;
	OMX_S8                  cGpsDestBearingRef[2];
	OMX_TI_EXIFTAGSTATUS    eStatusGpsDestBearing;
	OMX_U32                 ulGpsDestBearing[2];
	OMX_TI_EXIFTAGSTATUS    eStatusGpsDestDistanceRef;
	OMX_S8                  cGpsDestDistanceRef[2];
	OMX_TI_EXIFTAGSTATUS    eStatusGpsDestDistance;
	OMX_U32                 ulGpsDestDistance[2];
	OMX_TI_EXIFTAGSTATUS    eStatusGpsProcessingMethod;
	OMX_S8*                 pGpsProcessingMethodBuff;
	OMX_U32                 ulGpsProcessingMethodBuffSizeBytes;
	OMX_TI_EXIFTAGSTATUS    eStatusGpsAreaInformation;
	OMX_S8*                 pGpsAreaInformationBuff;
	OMX_U32                 ulGpsAreaInformationBuffSizeBytes;
	OMX_TI_EXIFTAGSTATUS    eStatusGpsDateStamp;
	OMX_S8                  cGpsDateStamp[11];
	OMX_TI_EXIFTAGSTATUS    eStatusGpsDifferential;
	OMX_U16                 usGpsDifferential;
} OMX_TI_CONFIG_EXIF_TAGS;

/**
 * Structure used to configure current OMX_TI_SENMOUNT_TYPE
 *
 * @param nSenId
 * @param nRotation
 */
typedef struct OMX_TI_SENMOUNT_TYPE {
    OMX_U32             nSenId;
    OMX_U32             nRotation;
}OMX_TI_SENMOUNT_TYPE;

/**
 * Structure used to configure current OMX_TI_VARFPSTYPE
 *
 * @param nVarFPSMin    Number of the smallest FPS supported.
 * @param nVarFPSMax    Number of the biggest FPS supported.
 */
typedef struct OMX_TI_VARFPSTYPE {
    OMX_U32                 nVarFPSMin;
    OMX_U32                 nVarFPSMax;
} OMX_TI_VARFPSTYPE;

/**
 * Structure used to configure current OMX_TI_CONFIG_SHAREDBUFFER
 *
 * STRUCT MEMBERS:
 * nSize            : Size of the structure in bytes
 * nVersion         : OMX specification version information
 * nPortIndex       : Port that this structure applies to
 * nSharedBuffSize  : Size of the pSharedBuff in bytes
 * pSharedBuff      : Pointer to a buffer
 */
typedef struct OMX_TI_CONFIG_SHAREDBUFFER {
	OMX_U32 nSize;
	OMX_VERSIONTYPE nVersion;
	OMX_U32 nPortIndex;
	OMX_U32 nSharedBuffSize;
	OMX_U8* pSharedBuff;
} OMX_TI_CONFIG_SHAREDBUFFER;

/**
 * Structure used to configure current OMX_TI_CAPRESTYPE
 *
 * STRUCT MEMBERS:
 * nSize            : Size of the structure in bytes
 * nVersion         : OMX specification version information
 * nPortIndex       : Port that this structure applies to
 * nWidthMin        : Number of the smallest width supported
 * nHeightMin       : Number of the smallest height supported
 * nWidthMax        : Number of the biggest width supported
 * nHeightMax       : Number of the biggest height supported
 */
typedef struct OMX_TI_CAPRESTYPE {
	OMX_U32         nSize;
	OMX_VERSIONTYPE nVersion;
	OMX_U32         nPortIndex;
	OMX_U32         nWidthMin;  // smallest width supported
	OMX_U32         nHeightMin; // smallest height supported
	OMX_U32         nWidthMax;  // biggest width supported
	OMX_U32         nHeightMax; // biggest height supported
} OMX_TI_CAPRESTYPE;

/**
 * Structure used to configure current OMX_TI_CAPTYPE
 *
 * STRUCT MEMBERS:
 * nSize                                : Size of the structure in bytes
 * nVersion                             : OMX specification version information
 * nPortIndex                           : Port that this structure applies to
 * ulPreviewFormatCount                 : Number of the supported preview pixelformat count
 * ePreviewFormats                      : Array containing the supported preview pixelformat count
 * ulImageFormatCount                   : Number of the supported image pixelformat count
 * eImageFormats                        : Array containing the supported image pixelformat count
 * tPreviewResRange                     : Supported preview resolution range
 * tImageResRange                       : Supported image resolution range
 * tThumbResRange                       : Supported thumbnail resolution range
 * ulWhiteBalanceCount                  : Supported whitebalance mode count
 * eWhiteBalanceModes                   : Array containing the whitebalance modes
 * ulColorEffectCount                   : Supported effects count
 * eColorEffects                        : Array containing the supported effects
 * xMaxWidthZoom                        : Fixed point value stored as Q16 representing the maximum value for the Zoom allowed on Width
 * xMaxHeightZoom                       : Fixed point value stored as Q16 representing the maximum value for the Zoom allowed on Height
 * ulFlickerCount                       : Number of the supported anti-flicker modes
 * eFlicker                             : Array containing the supported anti-flicker modes
 * ulExposureModeCount                  : Number of the supported exposure modes
 * eExposureModes                       : Array containing the supported exposure modes
 * bLensDistortionCorrectionSupported   : Flag for Lens Distortion Correction Algorithm support
 * bISONoiseFilterSupported             : Flag for Noise Filter Algorithm support
 * xEVCompensationMin                   : Fixed point value stored as Q16 representing the EVCompensation minumum allowed value
 * xEVCompensationMax                   : Fixed point value stored as Q16 representing the EVCompensation maximum allowed value
 * nSensitivityMax                      : nSensitivityMax = 100 implies maximum supported equal to "ISO 100"
 * ulFocusModeCount                     : Number of the supported focus modes
 * eFocusModes                          : Array containing the supported focus modes
 * ulSceneCount                         : Number of the supported scenes
 * eSceneModes                          : Array containing the supported scenes
 * ulFlashCount                         : Number of the supported flash modes
 * eFlashModes                          : Array containing the supported flash modes
 * xFramerateMin                        : Fixed point value stored as Q16 representing the minimum framerate allowed
 * xFramerateMax                        : Fixed point value stored as Q16 representing the maximum framerate allowed
 * bContrastSupported                   : Flag showing if the contrast is supported
 * bSaturationSupported                 : Flag showing if the saturation is supported
 * bBrightnessSupported                 : Flag showing if the brightness is supported
 * bProcessingLevelSupported            : Flag showing if the processing level is supported
 * bQFactorSupported                    : Flag showing if the QFactor is supported
 * ulPrvVarFPSModesCount                : Number of preview FPS modes
 * tPrvVarFPSModes                      : Preview FPS modes
 * ulCapVarFPSModesCount                : Number of capture FPS modes
 * tCapVarFPSModes                      : Capture FPS modes
 * tSenMounting                         : Sensor mount information
 */
typedef struct OMX_TI_CAPTYPE {
	OMX_U32                 nSize;
	OMX_VERSIONTYPE         nVersion;
	OMX_U32                 nPortIndex;
	OMX_U16                 ulPreviewFormatCount;   // supported preview pixelformat count
	OMX_COLOR_FORMATTYPE    ePreviewFormats[100];
	OMX_U16                 ulImageFormatCount;     // supported image pixelformat count
	OMX_COLOR_FORMATTYPE    eImageFormats[100];
	OMX_TI_CAPRESTYPE       tPreviewResRange;       // supported preview resolution range
	OMX_TI_CAPRESTYPE       tImageResRange;         // supported image resolution range
	OMX_TI_CAPRESTYPE       tThumbResRange;         // supported thumbnail resolution range
	OMX_U16                 ulWhiteBalanceCount;    // supported whitebalance mode count
	OMX_WHITEBALCONTROLTYPE eWhiteBalanceModes[100];
	OMX_U16                 ulColorEffectCount;     // supported effects count
	OMX_IMAGEFILTERTYPE     eColorEffects[100];
	OMX_S32                 xMaxWidthZoom;          // Fixed point value stored as Q16
	OMX_S32                 xMaxHeightZoom;         // Fixed point value stored as Q16
	OMX_U16                 ulFlickerCount;         // supported anti-flicker mode count
	OMX_COMMONFLICKERCANCELTYPE     eFlicker[100];
	OMX_U16                 ulExposureModeCount;    // supported exposure mode count
	OMX_EXPOSURECONTROLTYPE eExposureModes[100];
	OMX_BOOL                bLensDistortionCorrectionSupported;
	OMX_BOOL                bISONoiseFilterSupported;
	OMX_S32                 xEVCompensationMin;     // Fixed point value stored as Q16
	OMX_S32                 xEVCompensationMax;     // Fixed point value stored as Q16
	OMX_U32                 nSensitivityMax;        // nSensitivityMax = 100 implies maximum supported equal to "ISO 100"
	OMX_U16                 ulFocusModeCount;       // supported focus mode count
	OMX_IMAGE_FOCUSCONTROLTYPE      eFocusModes[100];
	OMX_U16                 ulSceneCount;           // supported scene count
	OMX_SCENEMODETYPE       eSceneModes[100];
	OMX_U16                 ulFlashCount;           // supported flash modes count
	OMX_IMAGE_FLASHCONTROLTYPE      eFlashModes[100];
	OMX_U32                 xFramerateMin;          // Fixed point value stored as Q16
	OMX_U32                 xFramerateMax;          // Fixed point value stored as Q16
	OMX_BOOL                bContrastSupported;
	OMX_BOOL                bSaturationSupported;
	OMX_BOOL                bBrightnessSupported;
	OMX_BOOL                bProcessingLevelSupported;
	OMX_BOOL                bQFactorSupported;
	OMX_U16                 ulPrvVarFPSModesCount;  // supported variable FPS preview modes count
	OMX_TI_VARFPSTYPE       tPrvVarFPSModes[10];
	OMX_U16                 ulCapVarFPSModesCount;  // supported variable FPS capture modes count
	OMX_TI_VARFPSTYPE       tCapVarFPSModes[10];
	OMX_TI_SENMOUNT_TYPE    tSenMounting;
        OMX_U16                 ulAlgoAreasFocusCount;    // supported number of AlgoAreas for focus areas
       OMX_U16                  ulAlgoAreasExposureCount; // supported number of AlgoAreas for exposure areas
} OMX_TI_CAPTYPE;



/**
 * Defines 3A Face priority mode.
 *
 * STRUCT MEMBERS:
 *  nSize               : Size of the structure in bytes
 *  nVersion            : OMX specification version information
 *  nPortIndex          : Port that this structure applies to
 *  bAwbFaceEnable      : Enable Face priority for Auto White Balance
 *  bAeFaceEnable       : Enable Face priority for Auto Exposure
 *  bAfFaceEnable       : Enable Face priority for Auto Focus
 */
typedef struct OMX_TI_CONFIG_3A_FACE_PRIORITY {
	OMX_U32 nSize;
	OMX_VERSIONTYPE nVersion;
	OMX_U32 nPortIndex;
	OMX_BOOL bAwbFaceEnable;
	OMX_BOOL bAeFaceEnable;
	OMX_BOOL bAfFaceEnable;
} OMX_TI_CONFIG_3A_FACE_PRIORITY;

/**
 * Defines 3A Region priority mode.
 *
 * STRUCT MEMBERS:
 *  nSize               : Size of the structure in bytes
 *  nVersion            : OMX specification version information
 *  nPortIndex          : Port that this structure applies to
 *  bAwbFaceEnable      : Enable Region priority for Auto White Balance
 *  bAeFaceEnable       : Enable Region priority for Auto Exposure
 *  bAfFaceEnable       : Enable Region priority for Auto Focus
 */
typedef struct OMX_TI_CONFIG_3A_REGION_PRIORITY {
	OMX_U32 nSize;
	OMX_VERSIONTYPE nVersion;
	OMX_U32 nPortIndex;
	OMX_BOOL bAwbRegionEnable;
	OMX_BOOL bAeRegionEnable;
	OMX_BOOL bAfRegionEnable;
} OMX_TI_CONFIG_3A_REGION_PRIORITY;

/*
* STRUCT MEMBERS:
* nSize         : Size of the structure in bytes
* nVersion      : OMX specification version information
* nPortIndex    : Port that this structure applies to
* bAutoConvergence : Enable/Disable Auto Convergence
*/
typedef struct OMX_TI_PARAM_AUTOCONVERGENCETYPE {
	OMX_U32 nSize;
	OMX_VERSIONTYPE nVersion;
	OMX_U32 nPortIndex;
	OMX_BOOL bAutoConvergence;
} OMX_TI_PARAM_AUTOCONVERGENCETYPE;

/**
 * Focus distance configuration
 *
 *  STRUCT MEMBERS:
 *  nSize: Size of the structure in bytes
 *  nVersion: OMX specification version information
 *  nPortIndex: Port that this structure applies to
 *  nFocusDistanceNear : Specifies the near focus distance in mm ( 0 equals infinity )
 *  nFocusDistanceOptimal : Specifies the optimal focus distance in mm ( 0 equals infinity )
 *  nFocusDistanceFar : Specifies the far focus distance in mm ( 0 equals infinity )
 *  nLensPosition : Specifies the current lens position in driver units
 */
typedef struct OMX_TI_CONFIG_FOCUSDISTANCETYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nFocusDistanceNear;
    OMX_U32 nFocusDistanceOptimal;
    OMX_U32 nFocusDistanceFar;
    OMX_S32 nLensPosition;
} OMX_TI_CONFIG_FOCUSDISTANCETYPE;

/*
* STRUCT MEMBERS:
* nSize             : Size of the structure in bytes
* nVersion          : OMX specification version information
* nPortIndex        : Port that this structure applies to
* pAAAskipBuff      : Pointer to a buffer
* AAAskipBuffId     : Id of the send buffer
* AAAskipBuffSize   : Size of the sent buffer
*/
typedef struct OMX_TI_CONFIG_AAASKIPBUFFERTYPE {
	OMX_U32 nSize;
	OMX_VERSIONTYPE nVersion;
	OMX_U32 nPortIndex;
	OMX_PTR pAAAskipBuff;
	OMX_U32 AAAskipBuffId;
	OMX_U32 AAAskipBuffSize;
} OMX_TI_CONFIG_AAASKIPBUFFERTYPE;


/**
 * The OMX_TI_BRIGHTNESSCONTRASTCRTLTYPE enumeration is used to define the
 * brightness and contrast mode types.
 */
typedef enum OMX_TI_BRIGHTNESSCONTRASTCRTLTYPE {
	OMX_TI_BceModeOff = 0,
	OMX_TI_BceModeOn,
	OMX_TI_BceModeAuto,
	OMX_TI_BceModeMax = 0x7FFFFFFF
} OMX_TI_BRIGHTNESSCONTRASTCRTLTYPE;

/**
 * Local and global brightness contrast type.
 *
 * STRUCT MEMBERS:
 *  nSize             : Size of the structure in bytes
 *  nVersion          : OMX specification version information
 *  nPortIndex        : Port that this structure applies to
 *  eControl          : Control field for GLBCE
 */
typedef struct OMX_TI_CONFIG_LOCAL_AND_GLOBAL_BRIGHTNESSCONTRASTTYPE {
	OMX_U32 nSize;
	OMX_VERSIONTYPE nVersion;
	OMX_U32 nPortIndex;
	OMX_TI_BRIGHTNESSCONTRASTCRTLTYPE eControl;
} OMX_TI_CONFIG_LOCAL_AND_GLOBAL_BRIGHTNESSCONTRASTTYPE;

/**
 * Uncompressed image operating mode configuration structure.
 *
 * @param nSize         Size of the structure in bytes.
 * @param nVersion      OMX specification version information.
 * @param xMin          The minimum frame rate allowed.
 *                      Units are Q16 frames per second.
 * @param xMax          The maximum frame rate allowed.
 *                      Units are Q16 frames per second.
 */

typedef struct OMX_TI_CONFIG_VARFRMRANGETYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 xMin;
    OMX_U32 xMax;
} OMX_TI_CONFIG_VARFRMRANGETYPE;

/**
* A pointer to this struct is passed to the OMX_SetParameter when the extension
* index for the 'OMX.google.android.index.enableAndroidNativeBuffers' extension
* is given.
* The corresponding extension Index is OMX_TI_IndexUseNativeBuffers.
* This will be used to inform OMX about the presence of gralloc pointers instead
* of virtual pointers
*/
typedef struct OMX_TI_PARAMUSENATIVEBUFFER {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bEnable;
} OMX_TI_PARAMUSENATIVEBUFFER;

/**
* A pointer to this struct is passed to OMX_GetParameter when the extension
* index for the 'OMX.google.android.index.getAndroidNativeBufferUsage'
* extension is given.
* The corresponding extension Index is OMX_TI_IndexAndroidNativeBufferUsage.
* The usage bits returned from this query will be used to allocate the Gralloc
* buffers that get passed to the useAndroidNativeBuffer command.
*/
typedef struct OMX_TI_PARAMNATIVEBUFFERUSAGE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nUsage;
} OMX_TI_PARAMNATIVEBUFFERUSAGE;

/*==========================================================================*/
/*!
@brief OMX_TI_PARAM_ENHANCEDPORTRECONFIG : Suport added to new port reconfig usage
@param bUsePortReconfigForCrop       Enables port reconfig for crop.
@param bUsePortReconfigForPadding    Enables port reconfig for padding
*/
/*==========================================================================*/

typedef struct OMX_TI_PARAM_ENHANCEDPORTRECONFIG {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bUsePortReconfigForCrop;
    OMX_BOOL bUsePortReconfigForPadding;
} OMX_TI_PARAM_ENHANCEDPORTRECONFIG;

/**
* Define the frames queue len for ZSL
*
* STRUCT MEMBERS:
* nSize: Size of the structure in bytes
* nVersion: OMX specification version information
* nHistoryLen: History len in number of frames
*/
typedef struct OMX_TI_PARAM_ZSLHISTORYLENTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nHistoryLen;
} OMX_TI_PARAM_ZSLHISTORYLENTYPE;

/**
* Define the frame delay in ms for ZSL
*
* STRUCT MEMBERS:
* nSize: Size of the structure in bytes
* nVersion: OMX specification version information
* nDelay: Capture frame delay in ms
*/
typedef struct OMX_TI_CONFIG_ZSLDELAYTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_S32 nDelay;
} OMX_TI_CONFIG_ZSLDELAYTYPE;

/**
 * AlogAreas purpose
 * This type specifies the purpose of areas specified in OMX_ALGOAREASTYPE.
 * */
typedef enum OMX_ALGOAREAPURPOSE{
    OMX_AlgoAreaFocus = 0, // Multi region focus
    OMX_AlgoAreaExposure,
}OMX_ALGOAREAPURPOSE;

typedef  struct OMX_ALGOAREA {
    OMX_S32 nLeft;                      /**< The leftmost coordinate of the area rectangle */
    OMX_S32 nTop;                       /**< The topmost coordinate of the area rectangle */
    OMX_U32 nWidth;                     /**< The width of the area rectangle in pixels */
    OMX_U32 nHeight;                    /**< The height of the area rectangle in pixels */
    OMX_U32 nPriority;                  /**< Priority - ranges from 1 to 1000 */
}OMX_ALGOAREA;

/**
 * Algorythm areas type
 * This type defines areas for Multi Region Focus,
 * or another algorithm region parameters,
 * such as Multi Region Auto Exposure.
 *
 * STRUCT MEMBERS:
 *  nSize            : Size of the structure in bytes
 *  nVersion         : OMX specification version information
 *  nPortIndex       : Port index
 *  tAreaPosition    : Area definition - coordinates and purpose - Multi Region Focus, Auto Exposure, etc.
 *  nNumAreas        : Number of areas defined in the array
 *  nAlgoAreaPurpose : Algo area purpose - eg. Multi Region Focus is OMX_AlgoAreaFocus
 */
typedef  struct OMX_ALGOAREASTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;

    OMX_U32 nNumAreas;
    OMX_ALGOAREA tAlgoAreas[MAX_ALGOAREAS];
    OMX_ALGOAREAPURPOSE nAlgoAreaPurpose;
} OMX_ALGOAREASTYPE;



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif


