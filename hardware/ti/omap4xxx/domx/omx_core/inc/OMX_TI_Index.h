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

/**
 *  @file  omx_ti_index.h
 *         This file contains the vendor(TI) specific indexes
 *
 *  @path \OMAPSW_SysDev\multimedia\omx\khronos1_1\omx_core\inc
 *
 *  @rev 1.0
 */

/*==============================================================
 *! Revision History
 *! ============================
 *! 20-Dec-2008 x0052661@ti.com, initial version
 *================================================================*/

#ifndef _OMX_TI_INDEX_H_
#define _OMX_TI_INDEX_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/******************************************************************
 *   INCLUDE FILES
 ******************************************************************/
#include <OMX_Types.h>

/*******************************************************************
 * EXTERNAL REFERENCE NOTE: only use if not found in header file
 *******************************************************************/
/*----------         function prototypes      ------------------- */
/*----------         data declarations        ------------------- */
/*******************************************************************
 * PUBLIC DECLARATIONS: defined here, used elsewhere
 *******************************************************************/
/*----------         function prototypes      ------------------- */
/*----------         data declarations        ------------------- */

typedef enum OMX_TI_INDEXTYPE {

    OMX_IndexConfigAutoPauseAfterCapture = OMX_IndexAutoPauseAfterCapture,

    /* Vendor specific area for storing indices */
    OMX_TI_IndexConfigChannelName = ((OMX_INDEXTYPE)OMX_IndexVendorStartUnused + 1), /**< reference: OMX_CONFIG_CHANNELNAME */

    OMX_TI_IndexParamJPEGUncompressedMode,      /**< reference: OMX_JPEG_PARAM_UNCOMPRESSEDMODETYPE */
    OMX_TI_IndexParamJPEGCompressedMode,        /**< reference: OMX_JPEG_PARAM_COMPRESSEDMODETYPE */
    OMX_TI_IndexParamDecodeSubregion,           /**< reference: OMX_IMAGE_PARAM_DECODE_SUBREGION */

    /* H264 Encoder Indices*/
	OMX_TI_IndexParamVideoDataSyncMode, //!< Refer to OMX_VIDEO_PARAM_DATASYNCMODETYPE structure
	OMX_TI_IndexParamVideoNALUsettings,	//!< use OMX_VIDEO_PARAM_AVCNALUCONTROLTYPE to configure the type os NALU to send along with the Different Frame Types
	OMX_TI_IndexParamVideoMEBlockSize,	//!< use OMX_VIDEO_PARAM_MEBLOCKSIZETYPE to specify the minimum block size used for motion estimation
	OMX_TI_IndexParamVideoIntraPredictionSettings,	//!< use OMX_VIDEO_PARAM_INTRAPREDTYPE to configure the intra prediction modes used for different block sizes
	OMX_TI_IndexParamVideoEncoderPreset,	//!< use OMX_VIDEO_PARAM_ENCODER_PRESETTYPE to select the encoding mode & rate control preset
	OMX_TI_IndexParamVideoFrameDataContentSettings,	//!< use OMX_TI_VIDEO_PARAM_FRAMEDATACONTENTTYPE to configure the data content tpye
	OMX_TI_IndexParamVideoTransformBlockSize,	//!< use OMX_VIDEO_PARAM_TRANSFORM_BLOCKSIZETYPE to specify the block size used for ttransformation
	OMX_TI_IndexParamVideoVUIsettings, //!use OMX_VIDEO_PARAM_VUIINFOTYPE
	OMX_TI_IndexParamVideoAdvancedFMO,
	OMX_TI_IndexConfigVideoPixelInfo,	//!<  Use OMX_VIDEO_CONFIG_PIXELINFOTYPE structure to know the pixel aspectratio & pixel range
	OMX_TI_IndexConfigVideoMESearchRange,	//!< use OMX_VIDEO_CONFIG_MESEARCHRANGETYPE to specify the ME Search settings
	OMX_TI_IndexConfigVideoQPSettings,	//!< use OMX_TI_VIDEO_CONFIG_QPSETTINGS to specify the ME Search settings
	OMX_TI_IndexConfigSliceSettings,		//!<use OMX_VIDEO_CONFIG_SLICECODINGTYPE to specify the ME Search settings
	OMX_TI_IndexParamAVCInterlaceSettings,            //!< use OMX_TI_VIDEO_PARAM_AVCINTERLACECODING to specify the ME Search settings
	OMX_TI_IndexParamStereoInfo2004Settings,          //!< use OMX_TI_VIDEO_AVCENC_STEREOINFO2004 to specify the 2004 SEI for AVC Encoder
	OMX_TI_IndexParamStereoFramePacking2010Settings,  //!< use OMX_TI_VIDEO_AVCENC_FRAMEPACKINGINFO2010 to specify 2010 SEI for AVC Encoder


    /* Camera Indices */
    OMX_TI_IndexConfigSensorSelect,             /**< reference: OMX_CONFIG_SENSORSELECTTYPE */
    OMX_IndexConfigFlickerCancel,               /**< reference: OMX_CONFIG_FLICKERCANCELTYPE */
    OMX_IndexConfigSensorCal,                   /**< reference: OMX_CONFIG_SENSORCALTYPE */
	OMX_IndexConfigISOSetting, /**< reference: OMX_CONFIG_ISOSETTINGTYPE */
    OMX_TI_IndexConfigSceneMode,                /**< reference: OMX_CONFIG_SCENEMODETYPE */

    OMX_IndexConfigDigitalZoomSpeed,            /**< reference: OMX_CONFIG_DIGITALZOOMSPEEDTYPE */
    OMX_IndexConfigDigitalZoomTarget,           /**< reference: OMX_CONFIG_DIGITALZOOMTARGETTYPE */

    OMX_IndexConfigCommonScaleQuality,          /**< reference: OMX_CONFIG_SCALEQUALITYTYPE */

    OMX_IndexConfigCommonDigitalZoomQuality,    /**< reference: OMX_CONFIG_SCALEQUALITYTYPE */

    OMX_IndexConfigOpticalZoomSpeed,            /**< reference: OMX_CONFIG_DIGITALZOOMSPEEDTYPE */
    OMX_IndexConfigOpticalZoomTarget,           /**< reference: OMX_CONFIG_DIGITALZOOMTARGETTYPE */

    OMX_IndexConfigSmoothZoom,                  /**< reference: OMX_CONFIG_SMOOTHZOOMTYPE */

    OMX_IndexConfigBlemish,                     /**< reference: OMX_CONFIG_BLEMISHTYPE */

    OMX_IndexConfigExtCaptureMode,              /**< reference: OMX_CONFIG_EXTCAPTUREMODETYPE */
	OMX_IndexConfigExtPrepareCapturing, /**< reference : OMX_CONFIG_BOOLEANTYPE */
	OMX_IndexConfigExtCapturing, /**< reference : OMX_CONFIG_EXTCAPTURING */

	OMX_IndexCameraOperatingMode, /**<  OMX_CONFIG_CAMOPERATINGMODETYPE */
    OMX_IndexConfigDigitalFlash,                /**< reference: OMX_CONFIG_BOOLEANTYPE */
    OMX_IndexConfigPrivacyIndicator,            /**< reference: OMX_CONFIG_BOOLEANTYPE */

    OMX_IndexConfigTorchMode,                   /**< reference: OMX_CONFIG_TORCHMODETYPE */

    OMX_IndexConfigSlowSync,                    /**< reference: OMX_CONFIG_BOOLEANTYPE */

	OMX_IndexConfigExtFocusRegion, /**< reference : OMX_CONFIG_EXTFOCUSREGIONTYPE */
    OMX_IndexConfigFocusAssist,                 /**< reference: OMX_CONFIG_BOOLEANTYPE */

    OMX_IndexConfigImageFocusLock,              /**< reference: OMX_IMAGE_CONFIG_LOCKTYPE */
    OMX_IndexConfigImageWhiteBalanceLock,       /**< reference: OMX_IMAGE_CONFIG_LOCKTYPE */
    OMX_IndexConfigImageExposureLock,           /**< reference: OMX_IMAGE_CONFIG_LOCKTYPE */
    OMX_IndexConfigImageAllLock,                /**< reference: OMX_IMAGE_CONFIG_LOCKTYPE */

    OMX_IndexConfigImageDeNoiseLevel,           /**< reference: OMX_IMAGE_CONFIG_PROCESSINGLEVELTYPE */
    OMX_IndexConfigSharpeningLevel,             /**< reference: OMX_IMAGE_CONFIG_PROCESSINGLEVELTYPE */
    OMX_IndexConfigDeBlurringLevel,             /**< reference: OMX_IMAGE_CONFIG_PROCESSINGLEVELTYPE */
    OMX_IndexConfigChromaCorrection,            /**< reference: OMX_IMAGE_CONFIG_PROCESSINGLEVELTYPE */
    OMX_IndexConfigDeMosaicingLevel,            /**< reference: OMX_IMAGE_CONFIG_PROCESSINGLEVELTYPE */

    OMX_IndexConfigCommonWhiteBalanceGain,      /**< reference: OMX_CONFIG_WHITEBALGAINTYPE */

    OMX_IndexConfigCommonRGB2RGB,               /**< reference: OMX_CONFIG_COLORCONVERSIONTYPE_II */
    OMX_IndexConfigCommonRGB2YUV,               /**< reference: OMX_CONFIG_COLORCONVERSIONTYPE_II */
	OMX_IndexConfigCommonYUV2RGB, /**< reference : OMX_CONFIG_EXT_COLORCONVERSIONTYPE */

    OMX_IndexConfigCommonGammaTable,            /**< reference: OMX_CONFIG_GAMMATABLETYPE */

    OMX_IndexConfigImageFaceDetection,          /**< reference: OMX_CONFIG_OBJDETECTIONTYPE */
    OMX_IndexConfigImageBarcodeDetection,       /**< reference: OMX_CONFIG_EXTRADATATYPE */
    OMX_IndexConfigImageSmileDetection,         /**< reference: OMX_CONFIG_OBJDETECTIONTYPE */
    OMX_IndexConfigImageBlinkDetection,         /**< reference: OMX_CONFIG_OBJDETECTIONTYPE */
    OMX_IndexConfigImageFrontObjectDetection,   /**< reference: OMX_CONFIG_EXTRADATATYPE */
    OMX_IndexConfigHistogramMeasurement,        /**< reference: OMX_CONFIG_HISTOGRAMTYPE */
    OMX_IndexConfigDistanceMeasurement,         /**< reference: OMX_CONFIG_EXTRADATATYPE */

    OMX_IndexConfigSnapshotToPreview,           /**< reference: OMX_CONFIG_BOOLEANTYPE */

	OMX_IndexConfigJpegHeaderType , /**< reference : OMX_CONFIG_JPEGHEEADERTYPE */
    OMX_IndexParamJpegMaxSize,                  /**< reference: OMX_IMAGE_JPEGMAXSIZE */

    OMX_IndexConfigRestartMarker,               /**< reference: OMX_CONFIG_BOOLEANTYPE */

    OMX_IndexParamImageStampOverlay,            /**< reference: OMX_PARAM_IMAGESTAMPOVERLAYTYPE */
    OMX_IndexParamThumbnail,                    /**< reference: OMX_PARAM_THUMBNAILTYPE */
    OMX_IndexConfigImageStabilization,          /**< reference: OMX_CONFIG_BOOLEANTYPE */
	OMX_IndexConfigMotionTriggeredImageStabilisation, /**< reference : OMX_CONFIG_BOOLEANTYPE */
    OMX_IndexConfigRedEyeRemoval,               /**< reference: OMX_CONFIG_REDEYEREMOVALTYPE */
    OMX_IndexParamHighISONoiseFiler,            /**< reference: OMX_CONFIG_BOOLEANTYPE */
    OMX_IndexParamLensDistortionCorrection,     /**< reference: OMX_CONFIG_BOOLEANTYPE */
    OMX_IndexParamLocalBrightnessAndContrast,   /**< reference: OMX_CONFIG_BOOLEANTYPE */
	OMX_IndexConfigChromaticAberrationCorrection, /**< reference: OMX_CONFIG_BOOLEANTYPE */
    OMX_IndexParamVideoCaptureYUVRange,         /**< reference: OMX_PARAM_VIDEOYUVRANGETYPE */

    OMX_IndexConfigFocusRegion,                 /**< reference: OMX_CONFIG_EXTFOCUSREGIONTYPE */
    OMX_IndexConfigImageMotionEstimation,       /**< reference: OMX_CONFIG_OBJDETECTIONTYPE */
    OMX_IndexParamProcessingOrder,              /**< reference: OMX_CONFIGPROCESSINGORDERTYPE */
    OMX_IndexParamFrameStabilisation,           /**< reference: OMX_CONFIG_BOOLEANTYPE */
    OMX_IndexParamVideoNoiseFilter,              /**< reference: OMX_PARAM_VIDEONOISEFILTERTYPE */

    OMX_IndexConfigOtherExtraDataControl,        /**< reference:  OMX_CONFIG_EXTRADATATYPE */
    OMX_TI_IndexParamBufferPreAnnouncement,             /**< reference: OMX_TI_PARAM_BUFFERPREANNOUNCE */
    OMX_TI_IndexConfigBufferRefCountNotification,       /**< reference: OMX_TI_CONFIG_BUFFERREFCOUNTNOTIFYTYPE */
    OMX_TI_IndexParam2DBufferAllocDimension,            /**< reference: OMX_CONFIG_RECTTYPE */
    OMX_TI_IndexConfigWhiteBalanceManualColorTemp,      /**< reference: OMX_TI_CONFIG_WHITEBALANCECOLORTEMPTPYPE */
    OMX_TI_IndexConfigFocusSpotWeighting,               /**< reference: OMX_TI_CONFIG_FOCUSSPOTWEIGHTINGTYPE */
    OMX_TI_IndexParamSensorOverClockMode,               /**< reference: OMX_CONFIG_BOOLEANTYPE */
    OMX_TI_IndexParamDccUriInfo,                        /**< reference: OMX_TI_PARAM_DCCURIINFO */
    OMX_TI_IndexParamDccUriBuffer,                      /**< reference: OMX_TI_PARAM_DCCURIBUFFER */

    /* MPEG4 and H264 encoder specific Indices */
    OMX_TI_IndexParamVideoIntraRefresh,         /**< reference: OMX_TI_VIDEO_PARAM_INTRAREFRESHTYPE */

    OMX_TI_IndexConfigShutterCallback,          /**< reference: OMX_CONFIG_BOOLEANTYPE */
    OMX_TI_IndexParamVarFrameRate,              /**< reference: OMX_PARAM_VARFARAMERATETYPE */
    OMX_TI_IndexConfigAutoConvergence,          /**< reference: OMX_TI_CONFIG_CONVERGENCETYPE */
    OMX_TI_IndexConfigRightExposureValue,       /**< reference: OMX_TI_CONFIG_EXPOSUREVALUERIGHTTYPE */
    OMX_TI_IndexConfigExifTags,                 /**< reference: OMX_TI_CONFIG_SHAREDBUFFER */
    OMX_TI_IndexParamVideoPayloadHeaderFlag,    /**< reference: OMX_TI_PARAM_PAYLOADHEADERFLAG */
    OMX_TI_IndexParamVideoIvfMode,              /**< reference: OMX_TI_PARAM_IVFFLAG */
    OMX_TI_IndexConfigCamCapabilities,          /**< reference: OMX_TI_CONFIG_SHAREDBUFFER */
    OMX_TI_IndexConfigFacePriority3a,           /**< reference: OMX_TI_CONFIG_3A_FACE_PRIORITY */
    OMX_TI_IndexConfigRegionPriority3a,         /**< reference: OMX_TI_CONFIG_3A_REGION_PRIORITY */
    OMX_TI_IndexParamAutoConvergence,           /**< reference: OMX_TI_PARAM_AUTOCONVERGENCETYPE */
    OMX_TI_IndexConfigAAAskipBuffer,            /**< reference: OMX_TI_CONFIG_AAASKIPBUFFERTYPE */
    OMX_TI_IndexParamStereoFrmLayout,           /**< reference: OMX_TI_FRAMELAYOUTTYPE */
    OMX_TI_IndexConfigLocalBrightnessContrastEnhance, /**< reference: OMX_TI_CONFIG_LOCAL_AND_GLOBAL_BRIGHTNESSCONTRASTTYPE */
    OMX_TI_IndexConfigGlobalBrightnessContrastEnhance, /**< reference: OMX_TI_CONFIG_LOCAL_AND_GLOBAL_BRIGHTNESSCONTRASTTYPE */
    OMX_TI_IndexConfigVarFrmRange,              /**< reference: OMX_TI_CONFIG_VARFRMRANGETYPE */
    OMX_TI_IndexParamAVCHRDBufferSizeSetting,   /**< reference: OMX_TI_VIDEO_PARAM_AVCHRDBUFFERSETTING */
    OMX_TI_IndexConfigAVCHRDBufferSizeSetting,   /**< reference: OMX_TI_VIDEO_CONFIG_AVCHRDBUFFERSETTING */
    OMX_TI_IndexConfigFocusDistance,              /**< reference: OMX_TI_CONFIG_FOCUSDISTANCETYPE */
    OMX_TI_IndexUseNativeBuffers,                /**< reference: OMX_TI_ParamUseNativeBuffer */
    OMX_TI_IndexParamUseEnhancedPortReconfig,     /**< reference: OMX_TI_IndexParamUseEnhancedPortReconfig */
    OMX_TI_IndexEncoderStoreMetadatInBuffers,
    OMX_TI_IndexParamZslHistoryLen,                     /**< reference: OMX_TI_PARAM_ZSLHISTORYLENTYPE */
    OMX_TI_IndexConfigZslDelay,                          /**< reference: OMX_TI_CONFIG_ZSLDELAYTYPE */
    OMX_TI_IndexParamMetaDataBufferInfo,                  /***< reference: OMX_TI_PARAM_METADATABUFFERINFO */
    OMX_TI_IndexConfigZslFrameSelectMethod,              /**< reference: OMX_TI_CONFIG_ZSLFRAMESELECTMETHODTYPE */
    OMX_TI_IndexAndroidNativeBufferUsage,          /**< reference: OMX_TI_IndexAndroidNativeBufferUsage */
    OMX_TI_IndexConfigAlgoAreas,                         /**< reference: OMX_PARAM_SHAREDBUFFER (pSharedBuff is OMX_ALGOAREASTYPE) */
    OMX_TI_IndexConfigAutofocusEnable             /**< reference: OMX_CONFIG_BOOLEANTYPE */
} OMX_TI_INDEXTYPE;



/*******************************************************************
 * PRIVATE DECLARATIONS: defined here, used only here
 *******************************************************************/
/*----------          data declarations        ------------------- */
/*----------          function prototypes      ------------------- */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _OMX_TI_INDEX_H_ */

