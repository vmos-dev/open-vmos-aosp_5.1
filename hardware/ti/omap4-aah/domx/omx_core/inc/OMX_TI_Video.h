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

/* -------------------------------------------------------------------------- */
/*
 * @file:Omx_ti_video.h
 * This header defines the structures specific to the param or config indices of Openmax Video Component.
 *
 * @path:
 * \WTSD_DucatiMMSW\ omx\omx_il_1_x\omx_core\

 * -------------------------------------------------------------------------- */

/* =========================================================================
 *!
 *! Revision History
 *! =====================================================================
 *! 24-Dec-2008  Navneet         navneet@ti.com          Initial Version
 *! 14-Jul-2009  Radha Purnima   radhapurnima@ti.com
 *! 25-Aug-2009  Radha Purnima   radhapurnima@ti.com
 *! 16-May-2009  Shivaraj Shetty shettyshivaraj@ti.com
 * =========================================================================*/


#ifndef OMX_TI_VIDEO_H
#define OMX_TI_VIDEO_H
#define H264ENC_MAXNUMSLCGPS 2
#define OMXH264E_MAX_SLICE_SUPPORTED 64
#include <OMX_Core.h>

/**
 *	@brief	mode selection for the data that is given to the Codec
 */

typedef enum OMX_VIDEO_DATASYNCMODETYPE {
    OMX_Video_FixedLength,	//!<  Interms of multiples of 4K
    OMX_Video_SliceMode,		//!<  Slice mode
    OMX_Video_NumMBRows,	//!< Number of rows, each row is 16 lines of video
    OMX_Video_EntireFrame  	//!< Processing of entire frame data
} OMX_VIDEO_DATASYNCMODETYPE;


/* ========================================================================== */
/*!
 @brief OMX_TI_VIDEO_PARAM_DATAMODE  :to configure how the input and output data is fed to the Codec
 @param  nPortIndex  to specify the index of the port
 @param  eDataMode 	to specify the data mode
 						@sa  OMX_VIDEO_DATASYNCMODETYPE
 @param  nNumDataUnits	 to specify the number of data units (where units are of type defined by eDataMode)
 */
/* ==========================================================================*/
typedef struct OMX_VIDEO_PARAM_DATASYNCMODETYPE{
	OMX_U32 nSize;
	OMX_VERSIONTYPE nVersion;
	OMX_U32 nPortIndex;
	OMX_VIDEO_DATASYNCMODETYPE eDataMode;
	OMX_U32 nNumDataUnits;
} OMX_VIDEO_PARAM_DATASYNCMODETYPE;

/**
 *	@brief	Aspect Ratio type selection for the encoded bit stream
 */
typedef enum OMX_VIDEO_ASPECTRATIOTYPE{
	OMX_Video_AR_Unspecified,  //!< Unspecified aspect ratio
	OMX_Video_AR_Square ,  //!< 1:1 (square) aspect ratio
	OMX_Video_AR_12_11  ,  //!<  12:11  aspect ratio
	OMX_Video_AR_10_11  ,  //!<  10:11  aspect ratio
	OMX_Video_AR_16_11  ,  //!<  16:11  aspect ratio
	OMX_Video_AR_40_33  ,  //!<  40:33  aspect ratio
	OMX_Video_AR_24_11  ,  //!<  24:11  aspect ratio
	OMX_Video_AR_20_11  ,  //!<  20:11  aspect ratio
	OMX_Video_AR_32_11  ,  //!<  32:11  aspect ratio
	OMX_Video_AR_80_33  ,  //!<  80:33  aspect ratio
	OMX_Video_AR_18_11  ,  //!<  18:11  aspect ratio
	OMX_Video_AR_15_15  ,  //!<  15:15  aspect ratio
	OMX_Video_AR_64_33  ,  //!<  64:33  aspect ratio
	OMX_Video_AR_160_99 ,  //!<  160:99 aspect ratio
	OMX_Video_AR_4_3    ,  //!<  4:3    aspect ratio
	OMX_Video_AR_3_2    ,  //!<  3:2    aspect ratio
	OMX_Video_AR_2_1    ,  //!<  2:1    aspect ratio
	OMX_Video_AR_Extended = 255,       //!<  Extended aspect ratio
   	OMX_Video_AR_Extended_MAX =  0X7FFFFFFF
}OMX_VIDEO_ASPECTRATIOTYPE;


/* ========================================================================== */
/*!
 @brief OMX_TI_VIDEO_PARAM_VUI_SELECT  :to select the VUI Settings
 @param  bAspectRatioPresent flag to indicate the insertion of aspect ratio information in VUI part of bit-stream
 @param  ePixelAspectRatio to specify the Aspect Ratio
 @param  bFullRange to indicate whether pixel value range is specified as full range or not (0 to 255)
*/
/* ==========================================================================*/
typedef struct OMX_VIDEO_PARAM_VUIINFOTYPE {
	OMX_U32	 nSize;
	OMX_VERSIONTYPE nVersion;
	OMX_U32	 nPortIndex;
	OMX_BOOL bAspectRatioPresent;
	OMX_VIDEO_ASPECTRATIOTYPE ePixelAspectRatio;
	OMX_BOOL bFullRange;
}OMX_VIDEO_PARAM_VUIINFOTYPE;

/* ========================================================================== */
/*!
 @brief OMX_VIDEO_CONFIG_PIXELINFOTYPE  :to specify the information related to the input pixel data (aspect ratio & range) to the Codec
 										so that codec can incorporate this info in the coded bit stream
 @param  nWidth 	 to specify the Aspect ratio: width of the pixel
 @param  nHeight 	 to specify the Aspect ratio: height of the pixel
 */
/* ==========================================================================*/
typedef struct OMX_VIDEO_CONFIG_PIXELINFOTYPE  {
	OMX_U32 nSize;
	OMX_VERSIONTYPE nVersion;
	OMX_U32 nPortIndex;
	OMX_U32 nWidth;
	OMX_U32 nHeight;
} OMX_VIDEO_CONFIG_PIXELINFOTYPE;

/* ========================================================================== */
/*!
 @brief OMX_VIDEO_PARAM_AVCNALUCONTROLTYPE : to configure what NALU  need to send along the frames of different types (Intra,IDR...etc)
 @param  nStartofSequence 	to configure the different NALU (specified via enabling/disabling (1/0) the bit positions) that need to send along with the Start of sequence frame
 @param  nEndofSequence	 	to to configure the different NALU (specified via enabling/disabling (1/0) the bit positions) that need to send along with the End of sequence frame
 @param  nIDR 				to to configure the different NALU (specified via enabling/disabling (1/0) the bit positions) that need to send along with the IDR frame
 @param  nIntraPicture	  		to to configure the different NALU (specified via enabling/disabling (1/0) the bit positions) that need to send along with the Intra frame
 @param  nNonIntraPicture	  	to to configure the different NALU (specified via enabling/disabling (1/0) the bit positions) that need to send along with the Non Intra frame

Bit Position:   13|       12|      11|           10|      9|    8|    7|   6|      5|         4|              3|              2|              1|          0
NALU TYPE:  SPS_VUI|FILLER|EOSTREAM|EOSEQ|AUD|PPS|SPS|SEI|IDR_SLICE|SLICE_DP_C|SLICE_DP_B|SLICE_DP_A|SLICE|UNSPECIFIED \n
*/
/* ==========================================================================*/
typedef struct OMX_VIDEO_PARAM_AVCNALUCONTROLTYPE {
	OMX_U32	 nSize;
	OMX_VERSIONTYPE nVersion;
	OMX_U32	 	nPortIndex;
	OMX_U32 	nStartofSequence;
	OMX_U32 	nEndofSequence;
	OMX_U32 	nIDR;
	OMX_U32 	nIntraPicture;
	OMX_U32 	nNonIntraPicture;
}OMX_VIDEO_PARAM_AVCNALUCONTROLTYPE;


/* ========================================================================== */
/*!
 @brief OMX_VIDEO_CONFIG_MESEARCHRANGETYPE : to configure Motion Estimation Parameters
 @param  eMVAccuracy 		to specify the Motion Vector Accuracy
 							@sa OMX_VIDEO_MOTIONVECTORTYPE
 @param  sHorSearchRangeP	 	to Specify the Horizontal Search range for P Frame
 @param  sVerSearchRangeP		to Specify the Vertical Search range for P Frame
 @param  sHorSearchRangeB	  	to Specify the Horizontal Search range for B Frame
 @param  sVerSearchRangeB	  	to Specify the Vertical Search range for B Frame
*/
/* ==========================================================================*/
typedef struct OMX_VIDEO_CONFIG_MESEARCHRANGETYPE{
	OMX_U32	 nSize;
	OMX_VERSIONTYPE nVersion;
	OMX_U32	 nPortIndex;
	OMX_VIDEO_MOTIONVECTORTYPE eMVAccuracy;
	OMX_U32	 nHorSearchRangeP;
	OMX_U32	 nVerSearchRangeP;
	OMX_U32	 nHorSearchRangeB;
	OMX_U32	 nVerSearchRangeB;
}OMX_VIDEO_CONFIG_MESEARCHRANGETYPE;

/**
 *	@brief	Block size specification
 */
typedef enum OMX_VIDEO_BLOCKSIZETYPE {
	OMX_Video_Block_Size_16x16=0,
	OMX_Video_Block_Size_8x8,
	OMX_Video_Block_Size_8x4,
	OMX_Video_Block_Size_4x8,
	OMX_Video_Block_Size_4x4,
   	OMX_Video_Block_Size_MAX =  0X7FFFFFFF
}OMX_VIDEO_BLOCKSIZETYPE;

/* ========================================================================== */
/*!
 @brief OMX_VIDEO_PARAM_MEBLOCKSIZETYPE : to configure the Min Motion Estimation block size for P and B frames
 @param  eMinBlockSizeP 		to specify the Min Block size used for Motion Estimation incase of P Frames
 							@sa OMX_VIDEO_BLOCKSIZETYPE
 @param  eMinBlockSizeB	 	to specify the Min Block size used for Motion Estimation incase of B Frames
*/
/* ==========================================================================*/
typedef struct OMX_VIDEO_PARAM_MEBLOCKSIZETYPE{
	OMX_U32	 nSize;
	OMX_VERSIONTYPE nVersion;
	OMX_U32	 nPortIndex;
	OMX_VIDEO_BLOCKSIZETYPE eMinBlockSizeP;
	OMX_VIDEO_BLOCKSIZETYPE eMinBlockSizeB;
}OMX_VIDEO_PARAM_MEBLOCKSIZETYPE;

/**
 *	@brief	to select the chroma component used for Intra Prediction
 */
typedef enum OMX_VIDEO_CHROMACOMPONENTTYPE {
	OMX_Video_Chroma_Component_Cr_Only=0,	//!< consider only Cr chroma component for Intra prediction
	OMX_Video_Chroma_Component_Cb_Cr_Both,  //!< consider both (Cb & Cr) chroma components for Intra prediction
     OMX_Video_Chroma_Component_MAX =  0X7FFFFFFF
}OMX_VIDEO_CHROMACOMPONENTTYPE;

/* ========================================================================== */
/*!
 @brief OMX_VIDEO_PARAM_INTRAPREDTYPE : to configure the Modes for the different block sizes during Intra Prediction
 @param  nLumaIntra4x4Enable 	 	to configure the Modes for 4x4 block size during luma intra prediction: bit position specifies the modes that are enabled/disabled
								 HOR_UP|VERT_LEFT|HOR_DOWN|VERT_RIGHT|DIAG_DOWN_RIGHT|DIAG_DOWN_LEFT|DC|HOR|VER
 @param  nLumaIntra8x8Enable	 	to configure the Modes for 4x4 block size during luma intra prediction
 								HOR_UP|VERT_LEFT|HOR_DOWN|VERT_RIGHT|DIAG_DOWN_RIGHT|DIAG_DOWN_LEFT|DC|HOR|VER
 @param  nLumaIntra16x16Enable	 	to configure the Modes for 4x4 block size during luma intra prediction
								 PLANE|DC|HOR|VER
 @param  nChromaIntra8x8Enable	 	to configure the Modes for 4x4 block size during luma intra prediction
								 PLANE|DC|HOR|VER
 @param  eChromaComponentEnable	to select the chroma components used for the intra prediction
 								@sa OMX_VIDEO_CHROMACOMPONENTTYPE
*/
/* ==========================================================================*/
typedef struct OMX_VIDEO_PARAM_INTRAPREDTYPE{
	OMX_U32	 nSize;
	OMX_VERSIONTYPE nVersion;
	OMX_U32	 nPortIndex;
	OMX_U32	 nLumaIntra4x4Enable;
	OMX_U32 nLumaIntra8x8Enable;
	OMX_U32 nLumaIntra16x16Enable;
	OMX_U32 nChromaIntra8x8Enable;
	OMX_VIDEO_CHROMACOMPONENTTYPE eChromaComponentEnable;
}OMX_VIDEO_PARAM_INTRAPREDTYPE;


/**
 *	@brief	Encoding Mode Preset
 */
typedef enum OMX_VIDEO_ENCODING_MODE_PRESETTYPE {
	OMX_Video_Enc_Default=0, 	//!<  for all the params default values are taken
	OMX_Video_Enc_High_Quality, //!<  todo: mention the parameters that takes specific values depending on this selection
	OMX_Video_Enc_User_Defined,
	OMX_Video_Enc_High_Speed_Med_Quality,
	OMX_Video_Enc_Med_Speed_Med_Quality,
	OMX_Video_Enc_Med_Speed_High_Quality,
	OMX_Video_Enc_High_Speed,
   	OMX_Video_Enc_Preset_MAX =  0X7FFFFFFF
}OMX_VIDEO_ENCODING_MODE_PRESETTYPE;

/**
 *	@brief	Rate Control Preset
 */
typedef enum OMX_VIDEO_RATECONTROL_PRESETTYPE {
	OMX_Video_RC_Low_Delay,	//!<todo:  mention the parameters that takes specific values depending on this selection
	OMX_Video_RC_Storage,
	OMX_Video_RC_Twopass,
	OMX_Video_RC_None,
	OMX_Video_RC_User_Defined,
   	OMX_Video_RC_MAX =  0X7FFFFFFF
}OMX_VIDEO_RATECONTROL_PRESETTYPE;

/* ========================================================================== */
/*!
 @brief OMX_VIDEO_PARAM_ENCODER_PRESETTYPE : to select the preset for Encoding Mode & Rate Control
 @param  eEncodingModePreset		to specify Encoding Mode Preset
 							@sa OMX_VIDEO_ENCODING_MODE_PRESETTYPE
 @param  eRateControlPreset	to specify Rate Control Preset
 							@sa OMX_VIDEO_RATECONTROL_PRESETTYPE
*/
/* ==========================================================================*/
typedef struct OMX_VIDEO_PARAM_ENCODER_PRESETTYPE{
	OMX_U32	 nSize;
	OMX_VERSIONTYPE nVersion;
	OMX_U32	 nPortIndex;
	OMX_VIDEO_ENCODING_MODE_PRESETTYPE eEncodingModePreset;
	OMX_VIDEO_RATECONTROL_PRESETTYPE eRateControlPreset;
}OMX_VIDEO_PARAM_ENCODER_PRESETTYPE;


/**
 *	@brief	 input content type
 */
typedef enum OMX_TI_VIDEO_FRAMECONTENTTYPE {
	OMX_TI_Video_Progressive = 0,			//!<Progressive frame
	OMX_TI_Video_Interlace_BothFieldsTogether = 1,	//!<Interlaced frame
	OMX_TI_Video_Interlace_OneField = 2,
	OMX_TI_Video_AVC_2004_StereoInfoType = 3,
	OMX_TI_Video_AVC_2010_StereoFramePackingType = 4,
	OMX_TI_Video_FrameContentType_MAX = 0x7FFFFFFF
}OMX_TI_VIDEO_FRAMECONTENTTYPE;

/**
 *	@brief	 Specifies the type of interlace content
 */
typedef enum OMX_TI_VIDEO_AVC_INTERLACE_CODINGTYPE {
	OMX_TI_Video_Interlace_PICAFF = 0,	//!< PicAFF type of interlace coding
	OMX_TI_Video_Interlace_MBAFF,		//!< MBAFF type of interlace coding
	OMX_TI_Video_Interlace_Fieldonly,	//!< Field only coding
	OMX_TI_Video_Interlace_Fieldonly_MRF=OMX_TI_Video_Interlace_Fieldonly,
	OMX_TI_Video_Interlace_Fieldonly_ARF,
	OMX_TI_Video_Interlace_Fieldonly_SPF,	//!< Field only coding where codec decides the partiy of the field to be used based upon content
	OMX_Video_Interlace_MAX = 0x7FFFFFFF
}OMX_TI_VIDEO_AVC_INTERLACE_CODINGTYPE;

/* ========================================================================== */
/*!
 @brief OMX_TI_VIDEO_PARAM_FRAMEDATACONTENTTYPE : to configure the data content
 @param  eContentType		to specify Content type
 							@sa OMX_VIDEO_FRAMECONTENTTYPE
*/
/* ==========================================================================*/
typedef struct OMX_TI_VIDEO_PARAM_FRAMEDATACONTENTTYPE{
	OMX_U32	 nSize;
	OMX_VERSIONTYPE nVersion;
	OMX_U32	 nPortIndex;
	OMX_TI_VIDEO_FRAMECONTENTTYPE eContentType;
}OMX_TI_VIDEO_PARAM_FRAMEDATACONTENTTYPE;

/* ========================================================================== */
/*!
 @brief OMX_TI_VIDEO_PARAM_AVCINTERLACECODING : to configure the interlace encoding related settings
 @param  eInterlaceCodingType	to specify the settings of interlace content
 							@sa OMX_VIDEO_INTERLACE_CODINGTYPE
 @param  bTopFieldFirst				to specify the first field sent is top or bottom
 @param  bBottomFieldIntra		to specify codec that encode bottomfield also as intra or not
*/
/* ==========================================================================*/
typedef struct OMX_TI_VIDEO_PARAM_AVCINTERLACECODING{
	OMX_U32	 nSize;
	OMX_VERSIONTYPE nVersion;
	OMX_U32	 nPortIndex;
	OMX_TI_VIDEO_AVC_INTERLACE_CODINGTYPE eInterlaceCodingType;
	OMX_BOOL bTopFieldFirst;
	OMX_BOOL bBottomFieldIntra;
}OMX_TI_VIDEO_PARAM_AVCINTERLACECODING;
/* ========================================================================== */
/*!
 @brief OMX_TI_VIDEO_PARAM_AVCENC_STEREOINFO2004  : to configure the 2004 related stereo information type
*/
/* ==========================================================================*/

typedef struct OMX_TI_VIDEO_PARAM_AVCENC_STEREOINFO2004
{
	OMX_U32          nSize;
	OMX_VERSIONTYPE  nVersion;
	OMX_U32          nPortIndex;
	OMX_BOOL         btopFieldIsLeftViewFlag;
	OMX_BOOL         bViewSelfContainedFlag;
} OMX_TI_VIDEO_AVCENC_STEREOINFO2004;

typedef enum OMX_TI_VIDEO_AVCSTEREO_FRAMEPACKTYPE{
	OMX_TI_Video_FRAMEPACK_CHECKERBOARD        = 0,
	OMX_TI_Video_FRAMEPACK_COLUMN_INTERLEAVING = 1,
	OMX_TI_Video_FRAMEPACK_ROW_INTERLEAVING    = 2,
	OMX_TI_Video_FRAMEPACK_SIDE_BY_SIDE        = 3,
	OMX_TI_Video_FRAMEPACK_TOP_BOTTOM          = 4,
	OMX_TI_Video_FRAMEPACK_TYPE_DEFAULT        = OMX_TI_Video_FRAMEPACK_SIDE_BY_SIDE,
	OMX_TI_Video_FRAMEPACK_TYPE_MAX = 0x7FFFFFFF
} OMX_TI_VIDEO_AVCSTEREO_FRAMEPACKTYPE;

/* ========================================================================== */
/*!
 @brief OMX_TI_VIDEO_PARAM_AVCENC_FRAMEPACKINGINFO2010 : to configure the 2010 related stereo information type
*/
/* ==========================================================================*/

typedef struct OMX_TI_VIDEO_PARAM_AVCENC_FRAMEPACKINGINFO2010
{
	OMX_U32          nSize;
	OMX_VERSIONTYPE nVersion;
	OMX_U32          nPortIndex;
	OMX_TI_VIDEO_AVCSTEREO_FRAMEPACKTYPE eFramePackingType;
	OMX_U8         nFrame0PositionX;
	OMX_U8         nFrame0PositionY;
	OMX_U8         nFrame1PositionX;
	OMX_U8         nFrame1PositionY;
}OMX_TI_VIDEO_PARAM_AVCENC_FRAMEPACKINGINFO2010;

/**
 *	@brief	 Specifies Transform Block Size
 */
typedef enum OMX_VIDEO_TRANSFORMBLOCKSIZETYPE {
	OMX_Video_Transform_Block_Size_4x4 =0,	//!< Transform blocks size is 8x8 : Valid for only High Profile
	OMX_Video_Transform_Block_Size_8x8,	//!< Transform blocks size is 4x4
	OMX_Video_Transform_Block_Size_Adaptive, //!< Adaptive transform block size : encoder decides as per content
    OMX_Video_Transform_Block_Size_MAX =  0X7FFFFFFF
}OMX_VIDEO_TRANSFORMBLOCKSIZETYPE;

/* ========================================================================== */
/*!
 @brief OMX_VIDEO_PARAM_TRANSFORM_BLOCKSIZETYPE : to select the Block Size used for transformation
 @param  eTransformBlocksize	to specify Block size used for transformation
 							@sa OMX_VIDEO_TRANSFORMBLOCKSIZETYPE
*/
/* ==========================================================================*/

typedef struct OMX_VIDEO_PARAM_TRANSFORM_BLOCKSIZETYPE{
	OMX_U32	 nSize;
	OMX_VERSIONTYPE nVersion;
	OMX_U32	 nPortIndex;
	OMX_VIDEO_TRANSFORMBLOCKSIZETYPE eTransformBlocksize;
}OMX_VIDEO_PARAM_TRANSFORM_BLOCKSIZETYPE;


/* ========================================================================== */
/*!
 @brief OMX_VIDEO_CONFIG_SLICECODINGTYPE : to configure the Slice Settings
 @param  eSliceMode	to specify the Slice mode
 							@sa OMX_VIDEO_AVCSLICEMODETYPE
 @param  nSlicesize to specify the sliceSize
*/
/* ==========================================================================*/

typedef struct OMX_VIDEO_CONFIG_SLICECODINGTYPE{
	OMX_U32	 nSize;
	OMX_VERSIONTYPE nVersion;
	OMX_U32	 nPortIndex;
	OMX_VIDEO_AVCSLICEMODETYPE eSliceMode;
	OMX_U32	 nSlicesize;
}OMX_VIDEO_CONFIG_SLICECODINGTYPE;

/**
 *	@brief	 Specifies Slice Group Change Direction Flag
 */
typedef enum OMX_VIDEO_SLIGRPCHANGEDIRTYPE{
  OMX_Video_Raster_Scan             = 0 , //!< 0 : Raster scan order
  OMX_Video_Clockwise              = 0 , //!< 0 : Clockwise (used for BOX OUT FMO Params)
  OMX_Video_Right                   = 0 , //!< 0 : RIGHT (Used for Wipe FMO type)
  OMX_Video_Reverse_Raster_Scan= 1 , //!< 1 : Reverse Raster Scan Order
  OMX_Video_Counter_Clockwise       = 1 , //!< 1 : Counter Clockwise (used for BOX OUT FMO Params)
  OMX_Video_Left                    = 1,  //!< 1 : LEFT (Used for Wipe FMO type)
  OMX_Video_Left_MAX =  0X7FFFFFFF
} OMX_VIDEO_SLICEGRPCHANGEDIRTYPE;

/* ========================================================================== */
/*!
 @brief OMX_TI_VIDEO_PARAM_FMO_ADVANCEDSETTINGS : to configure the FMO Settings
 @param
*/
/* ==========================================================================*/
typedef struct OMX_VIDEO_PARAM_AVCADVANCEDFMOTYPE{
	OMX_U32	 nSize;
	OMX_VERSIONTYPE nVersion;
	OMX_U32 nPortIndex;
	OMX_U8 nNumSliceGroups;
	OMX_U8 nSliceGroupMapType;
	OMX_VIDEO_SLICEGRPCHANGEDIRTYPE eSliceGrpChangeDir;
	OMX_U32 nSliceGroupChangeRate;
	OMX_U32 nSliceGroupChangeCycle;
	OMX_U32 nSliceGroupParams[H264ENC_MAXNUMSLCGPS] ;
}OMX_VIDEO_PARAM_AVCADVANCEDFMOTYPE;

/* ========================================================================== */
/*!
 @brief OMX_TI_VIDEO_CONFIG_QPSETTINGS : to configure the Qp Settings of I, P &B Frames
 @param  nQpI
*/
/* ==========================================================================*/

typedef struct OMX_VIDEO_CONFIG_QPSETTINGSTYPE{
	OMX_U32	 nSize;
	OMX_VERSIONTYPE nVersion;
	OMX_U32	 nPortIndex;
	OMX_U32	 nQpI;
	OMX_U32	 nQpMaxI;
	OMX_U32	 nQpMinI;
	OMX_U32	 nQpP;
	OMX_U32	 nQpMaxP;
	OMX_U32	 nQpMinP;
	OMX_U32	 nQpOffsetB;
	OMX_U32	 nQpMaxB;
	OMX_U32	 nQpMinB;
}OMX_VIDEO_CONFIG_QPSETTINGSTYPE;

/* ========================================================================== */
/*!
 @brief OMX_TI_VIDEO_PARAM_AVCHRDBUFFERSETTING : to configure the HRD
	(Hypothetical Reference Decoder) related params
 @param  nInitialBufferLevel	Initial buffer level for HRD compliance
 @param  nHRDBufferSize		Hypothetical Reference Decoder buffer size
 @param  nTargetBitrate		Target bitrate to encode with
*/
/* ==========================================================================*/

typedef struct OMX_TI_VIDEO_PARAM_AVCHRDBUFFERSETTING {
	OMX_U32     nSize;
	OMX_VERSIONTYPE     nVersion;
	OMX_U32    nPortIndex;
	OMX_U32    nInitialBufferLevel;
	OMX_U32    nHRDBufferSize;
	OMX_U32    nTargetBitrate;
} OMX_TI_VIDEO_PARAM_AVCHRDBUFFERSETTING;

/* ========================================================================== */
/*!
 @brief OMX_TI_VIDEO_CONFIG_AVCHRDBUFFERSETTING : to configure the HRD
	(Hypothetical Reference Decoder) related params
 @param  nHRDBufferSize		Hypothetical Reference Decoder Buffer Size
 @param  nEncodeBitrate		Target bitrate to encode with

*/
/* ==========================================================================*/

typedef struct OMX_TI_VIDEO_CONFIG_AVCHRDBUFFERSETTING {
	OMX_U32    nSize;
	OMX_VERSIONTYPE     nVersion;
	OMX_U32     nPortIndex;
	OMX_U32     nHRDBufferSize;
	OMX_U32     nEncodeBitrate;
} OMX_TI_VIDEO_CONFIG_AVCHRDBUFFERSETTING;

/* ========================================================================= */
/*!
 @brief OMX_TI_VIDEO_CODINGTYPE :
	Extension to video coding type enum for VP6 and VP7
 @param
*/
/* ==========================================================================*/

typedef enum OMX_TI_VIDEO_CODINGTYPE {
	OMX_VIDEO_CodingVP6 =
		(OMX_VIDEO_CODINGTYPE) OMX_VIDEO_CodingVendorStartUnused +1,  /* VP6 */
	OMX_VIDEO_CodingVP7, /* VP7 */
        OMX_TI_VIDEO_CodingSORENSONSPK, /* Sorenson spark*/
        OMX_VIDEO_CodingSVC,    /**< H.264/SVC */
	OMX_VIDEO_CodingVP8 /* VP8 */
}OMX_TI_VIDEO_CODINGTYPE;


/* ========================================================================= */
/*!
 @brief OMX_TI_VIDEO_MPEG4LEVELTYPE:
        Extension to MPEG-4 level to cater to level 6
 @param
*/
/* ==========================================================================*/
typedef enum OMX_TI_VIDEO_MPEG4LEVELTYPE {
        OMX_TI_VIDEO_MPEG4Level6  =
            (OMX_VIDEO_MPEG4LEVELTYPE) OMX_VIDEO_MPEG4LevelVendorStartUnused + 1
} OMX_TI_VIDEO_MPEG4LEVELTYPE;



/**
 *	@brief	 Specifies various intra refresh methods
 */
typedef enum OMX_TI_VIDEO_INTRAREFRESHTYPE {
    OMX_TI_VIDEO_IntraRefreshNone = 0,
    OMX_TI_VIDEO_IntraRefreshCyclicMbs,
    OMX_TI_VIDEO_IntraRefreshCyclicRows,
    OMX_TI_VIDEO_IntraRefreshMandatory,
    OMX_TI_VIDEO_IntraRefreshMax = 0x7FFFFFFF
} OMX_TI_VIDEO_INTRAREFRESHTYPE;


/* ========================================================================== */
/*!
 @brief OMX_TI_VIDEO_PARAM_INTRAREFRESHTYPE  : Configuration parameters for
                                               intra refresh settings
 @param  eRefreshMode		Various refresh modes supported
 @param  nIntraRefreshRate 	Intra refresh rate
*/
/* ==========================================================================*/

typedef struct OMX_TI_VIDEO_PARAM_INTRAREFRESHTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_TI_VIDEO_INTRAREFRESHTYPE eRefreshMode;
    OMX_U32 nIntraRefreshRate;
} OMX_TI_VIDEO_PARAM_INTRAREFRESHTYPE;


/* ============================================================================= */
/*!
 @brief OMX_TI_STEREODECINFO : Structure to access 2004 SEI message generated by
 H264 decoder as metatadata on its output port.
 */
/* ============================================================================= */

typedef struct OMX_TI_STEREODECINFO {
	OMX_U32 nFieldViewsFlag;
	OMX_U32 nTopFieldIsLeftViewFlag;
	OMX_U32 nCurrentFrameIsLeftViewFlag;
	OMX_U32 nNextFrameIsSecondViewFlag;
	OMX_U32 nLeftViewSelfContainedFlag;
	OMX_U32 nRightViewSelfContainedFlag;
} OMX_TI_STEREODECINFO;

typedef struct OMX_TI_FRAMEPACKINGDECINFO {
	OMX_U32 nFramePackingArrangementId;
	OMX_U32 nFramePackingArrangementRepetitionPeriod;
	OMX_U8  nFramePackingArrangementCancelFlag;
	OMX_U8  nFramePackingArrangementType;
	OMX_U8  nQuincunxSamplingFlag;
	OMX_U8  nContentInterpretationType;
	OMX_U8  nSpatialFlippingFlag;
	OMX_U8  nFrame0FlippedFlag;
	OMX_U8  nFieldViewsFlag;
	OMX_U8  nCurrentFrameIsFrame0Flag;
	OMX_U8  nFrame0SelfContainedFlag;
	OMX_U8  nFrame1SelfContainedFlag;
	OMX_U8  nFrame0GridPositionX;
	OMX_U8  nFrame0GridPositionY;
	OMX_U8  nFrame1GridPositionX;
	OMX_U8  nFrame1GridPositionY;
	OMX_U8  nFramePackingArrangementReservedByte;
	OMX_U8  nFramePackingArrangementExtensionFlag;
} OMX_TI_FRAMEPACKINGDECINFO;

/* ============================================================================= */
/*!
 @brief OMX_TI_VIDEO_RANGEMAPPING : Structure to access luma and chroma range
                                    mapping generated by decoders as
                                    metatadata on its output port.
 @param nRangeMappingLuma     Luma scale factor for range mapping.
 @param nRangeMappingChroma   Chroma scale factor for range mapping.
*/
/* ============================================================================= */

typedef struct OMX_TI_VIDEO_RANGEMAPPING {
	OMX_U32 nRangeMappingLuma;
	OMX_U32 nRangeMappingChroma;
} OMX_TI_VIDEO_RANGEMAPPING;

/* ============================================================================= */
/*!
 @brief OMX_TI_VIDEO_RESCALINGMATRIX : Structure to access rescaled
                                       width/height generated by decoders
                                       as metatadata on its output port.
 @param nScaledHeight   Scaled image width for post processing for decoder.
 @param nScaledWidth    Scaled image height for post processing for decoder.
*/
/* ============================================================================= */

typedef struct OMX_TI_VIDEO_RESCALINGMATRIX {
	OMX_U32 nScaledHeight;
	OMX_U32 nScaledWidth;
} OMX_TI_VIDEO_RESCALINGMATRIX;


/*==========================================================================*/
/*!
 @brief OMX_TI_PARAM_PAYLOADHEADERFLAG : To specify the payload headerflag
                                         for VP6/VP7 decoder.
 @param bPayloadHeaderFlag      Flag - TRUE indicates that frame length and
                                timestamp(for IVF format) will be part of
                                frame input buffer.
                                Flag - FALSE indecates that frame length and
                                timestamp(for IVF format) will not be part of
                                the input buffer.
*/
/*==========================================================================*/

typedef struct OMX_TI_PARAM_PAYLOADHEADERFLAG {
	OMX_U32 nSize;
	OMX_VERSIONTYPE nVersion;
	OMX_BOOL bPayloadHeaderFlag;
} OMX_TI_PARAM_PAYLOADHEADERFLAG;


/*==========================================================================*/
/*!
@brief OMX_TI_PARAM_IVFFLAG : Suport added to handle IVF header Decoding Mode
@param bIvfFlag               TRUE enables IVF decoding mode.
                              FALSE indicates bitstream format is non-IVF.
*/
/*==========================================================================*/

typedef struct OMX_TI_PARAM_IVFFLAG {
	OMX_U32 nSize;
	OMX_VERSIONTYPE nVersion;
	OMX_BOOL bIvfFlag;
} OMX_TI_PARAM_IVFFLAG;

// A pointer to this struct is passed to OMX_SetParameter() when the extension
// index "OMX.google.android.index.storeMetaDataInBuffers"
// is given.
//
// When meta data is stored in the video buffers passed between OMX clients
// and OMX components, interpretation of the buffer data is up to the
// buffer receiver, and the data may or may not be the actual video data, but
// some information helpful for the receiver to locate the actual data.
// The buffer receiver thus needs to know how to interpret what is stored
// in these buffers, with mechanisms pre-determined externally. How to
// interpret the meta data is outside of the scope of this method.
//
// Currently, this is specifically used to pass meta data from video source
// (camera component, for instance) to video encoder to avoid memcpying of
// input video frame data. To do this, bStoreMetaDta is set to OMX_TRUE.
// If bStoreMetaData is set to false, real YUV frame data will be stored
// in the buffers. In addition, if no OMX_SetParameter() call is made
// with the corresponding extension index, real YUV data is stored
// in the buffers.
typedef struct OMX_VIDEO_STOREMETADATAINBUFFERSPARAMS {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bStoreMetaData;
} OMX_VIDEO_STOREMETADATAINBUFFERSPARAMS;


/**
 * Interlaced Video Content format
 *
 * STRUCT MEMBERS:
 *  nSize      : Size of the structure in bytes
 *  nVersion   : OMX specification version information
 *  nPortIndex : Port that this structure applies to
 *  nFormat    : bitmapped value indentifying the interlaced formats supported by component
 *  nTimeStamp : temporal timestamp information for the second field
 */
typedef struct OMX_TI_INTERLACEFORMATTYPE {
	OMX_U32 nSize;
	OMX_VERSIONTYPE nVersion;
	OMX_U32 nPortIndex;
	OMX_U32 nFormat;
	OMX_TICKS nTimeStamp;
} OMX_TI_INTERLACEFORMATTYPE;

/**
 * Interlace format types
 */
typedef enum OMX_TI_INTERLACETYPE {
	OMX_InterlaceFrameProgressive= 0x00,
	OMX_InterlaceInterleaveFrameTopFieldFirst= 0x01,
	OMX_InterlaceInterleaveFrameBottomFieldFirst= 0x02,
	OMX_InterlaceFrameTopFieldFirst= 0x04,
	OMX_InterlaceFrameBottomFieldFirst= 0x08,
	OMX_InterlaceInterleaveFieldTop= 0x10,
	OMX_InterlaceInterleaveFieldBottom= 0x20,
	OMX_InterlaceFmtMask= 0x7FFFFFFF
} OMX_TI_INTERLACETYPE;

/**
 * To query if the stream contains interlaced or progressive conten
 *
 * STRUCT MEMBERS:
 *  nSize             : Size of the structure in bytes
 *  nVersion          : OMX specification version information
 *  nPortIndex        : Port that this structure applies to
 *  bInterlaceFormat  : whether the stream contains interlace or progressive content
 *                        OMX_TRUE indicates interlace and OMX_FALSE indicates progressive
 *  nInterlaceFormats : bitmapped value identifying the interlace formats detected within the stream
 */
typedef struct OMX_TI_STREAMINTERLACEFORMATTYPE {
	OMX_U32 nSize;
	OMX_VERSIONTYPE nVersion;
	OMX_U32 nPortIndex;
	OMX_BOOL bInterlaceFormat;
	OMX_U32 nInterlaceFormats;
} OMX_TI_STREAMINTERLACEFORMAT;

/*
@brief OMX_TI_VIDEO_CONFIG_PICSIZECONTROLINFO : Structure to provide the configuration to compute min and max picture size
@param minPicSizeRatio : This ratio is used to compute minimum picture size in the following manner,
minPicSize = averagePicSize >> minPicSizeRatio. Allowed values are 1 to 4. Setting this to 0 will enable encoder chosen ratio.
@param maxPicSizeRatio : This ratio is used to compute maximum picture size in the following manner,
maxPicSize = averagePicSize * maxPicSizeRatio. Allowed values are 2 to 30. Setting this to 0 or 1 will enable encoder chosen ratio.
*/
/* ============================================================================= */
typedef struct OMX_TI_VIDEO_CONFIG_PICSIZECONTROLINFO {
    OMX_U32         nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32         nPortIndex;
    OMX_U16         minPicSizeRatio;
    OMX_U16         maxPicSizeRatio;
} OMX_TI_VIDEO_CONFIG_PICSIZECONTROLINFO;


/*!====================================================================!

    Currently we only support SVC baseline profile

 * !====================================================================!*/
    typedef enum OMX_TI_VIDEO_SVCPROFILETYPE {
        OMX_VIDEO_SVCProfileBaseline            = 0x01,     /**< Baseline profile */
        OMX_VIDEO_SVCProfileHigh                = 0x02,     /**< High profile */
        OMX_VIDEO_SVCProfileHighIntra           = 0x03,     /**< High Intra profile */
        OMX_VIDEO_SVCProfileMax                 = 0x7FFFFFFF
    } OMX_TI_VIDEO_SVCPROFILETYPE;


/*!====================================================================!

    Currently we support only SVC baseline profile upto level 4 for SVC encoder.

 * !====================================================================!*/
    typedef enum OMX_TI_VIDEO_SVCLEVELTYPE {
        OMX_VIDEO_SVCLevel1                     = 0x01,     /**< Level 1 */
        OMX_VIDEO_SVCLevel1b                    = 0x02,     /**< Level 1b */
        OMX_VIDEO_SVCLevel11                    = 0x04,     /**< Level 1.1 */
        OMX_VIDEO_SVCLevel12                    = 0x08,     /**< Level 1.2 */
        OMX_VIDEO_SVCLevel13                    = 0x10,     /**< Level 1.3 */
        OMX_VIDEO_SVCLevel2                     = 0x20,     /**< Level 2 */
        OMX_VIDEO_SVCLevel21                    = 0x40,     /**< Level 2.1 */
        OMX_VIDEO_SVCLevel22                    = 0x80,     /**< Level 2.2 */
        OMX_VIDEO_SVCLevel3                     = 0x100,    /**< Level 3 */
        OMX_VIDEO_SVCLevel31                    = 0x200,    /**< Level 3.1 */
        OMX_VIDEO_SVCLevel32                    = 0x400,    /**< Level 3.2 */
        OMX_VIDEO_SVCLevel4                     = 0x800,    /**< Level 4 */
        OMX_VIDEO_SVCLevel41                    = 0x1000,   /**< Level 4.1 */
        OMX_VIDEO_SVCLevel42                    = 0x2000,   /**< Level 4.2 */
        OMX_VIDEO_SVCLevel5                     = 0x4000,   /**< Level 5 */
        OMX_VIDEO_SVCLevel51                    = 0x8000,   /**< Level 5.1 */
        OMX_VIDEO_SVCLevelMax                   = 0x7FFFFFFF
    } OMX_TI_VIDEO_SVCLEVELTYPE;


    typedef struct OMX_VIDEO_SVC_STD_PARAMS {
        OMX_U32  nSliceHeaderSpacing;
        OMX_U32  nPFrames;
        OMX_U32  nBFrames;
        OMX_BOOL bUseHadamard;
        OMX_U32  nRefFrames;
        OMX_U32  nRefIdx10ActiveMinus1;
        OMX_U32  nRefIdx11ActiveMinus1;
        OMX_BOOL bEnableUEP;
        /* Not needed as per SVC encoder requirements
        OMX_BOOL                                bEnableFMO;
        OMX_BOOL                                bEnableASO;
        OMX_BOOL                                bEnableRS;
        */
        OMX_VIDEO_AVCLOOPFILTERTYPE eLoopFilterMode;
        OMX_U32                     nAllowedPictureTypes;
        OMX_BOOL                    bFrameMBsOnly;
        OMX_BOOL                    bMBAFF;
        OMX_BOOL                    bEntropyCodingCABAC;
        OMX_BOOL                    bWeightedPPrediction;
        OMX_U32                     nWeightedBipredicitonMode;
        OMX_BOOL                    bconstIpred;
        OMX_BOOL                    bDirect8x8Inference;
        OMX_BOOL                    bDirectSpatialTemporal;
        OMX_U32                     nCabacInitIdc;
    } OMX_VIDEO_SVC_STD_PARAMS;


    typedef struct OMX_VIDEO_SVC_RECTTYPE {
        OMX_S32 nLeft;
        OMX_S32 nTop;
        OMX_U32 nWidth;
        OMX_U32 nHeight;
    } OMX_VIDEO_SVC_RECTTYPE;


    typedef struct OMX_VIDEO_SVC_BITRATETYPE {
        OMX_VIDEO_CONTROLRATETYPE eControlRate;
        OMX_U32                   nTargetBitrate;
    } OMX_VIDEO_SVC_BITRATETYPE;


    typedef struct OMX_VIDEO_SVC_MOTIONVECTORTYPE {
        OMX_VIDEO_MOTIONVECTORTYPE eAccuracy;
        OMX_BOOL                   bUnrestrictedMVs;
        OMX_BOOL                   bFourMV;
        OMX_S32                    sXSearchRange;
        OMX_S32                    sYSearchRange;
    } OMX_VIDEO_SVC_MOTIONVECTORTYPE;


    typedef struct OMX_VIDEO_SVC_QUANTIZATIONTYPE {
        OMX_U32 nQpI;
        OMX_U32 nQpP;
        OMX_U32 nQpB;
    } OMX_VIDEO_SVC_QUANTIZATIONTYPE;


    typedef struct OMX_VIDEO_SVC_INTRAREFRESHTYPE {
        OMX_VIDEO_INTRAREFRESHTYPE eRefreshMode;
        OMX_U32                    nAirMBs;
        OMX_U32                    nAirRef;
        OMX_U32                    nCirMBs;
    } OMX_VIDEO_SVC_INTRAREFRESHTYPE;


    typedef struct OMX_VIDEO_SVC_VBSMCTYPE {
        OMX_BOOL b16x16;
        OMX_BOOL b16x8;
        OMX_BOOL b8x16;
        OMX_BOOL b8x8;
        OMX_BOOL b8x4;
        OMX_BOOL b4x8;
        OMX_BOOL b4x4;
    } OMX_VIDEO_SVC_VBSMCTYPE;


    typedef struct OMX_VIDEO_SVC_NALUCONTROLTYPE {
        OMX_U32 nStartofSequence;
        OMX_U32 nEndofSequence;
        OMX_U32 nIDR;
        OMX_U32 nIntraPicture;
        OMX_U32 nNonIntraPicture;
    }OMX_VIDEO_SVC_NALUCONTROLTYPE;


    typedef struct OMX_VIDEO_SVC_MEBLOCKSIZETYPE {
        OMX_VIDEO_BLOCKSIZETYPE eMinBlockSizeP;
        OMX_VIDEO_BLOCKSIZETYPE eMinBlockSizeB;
    }OMX_VIDEO_SVC_MEBLOCKSIZETYPE;


    typedef struct OMX_VIDEO_SVC_INTRAPREDTYPE {
        OMX_U32                       nLumaIntra4x4Enable;
        OMX_U32                       nLumaIntra8x8Enable;
        OMX_U32                       nLumaIntra16x16Enable;
        OMX_U32                       nChromaIntra8x8Enable;
        OMX_VIDEO_CHROMACOMPONENTTYPE eChromaComponentEnable;
    }OMX_VIDEO_SVC_INTRAPREDTYPE;


    typedef struct OMX_VIDEO_SVC_ENCODER_PRESETTYPE {
        OMX_VIDEO_ENCODING_MODE_PRESETTYPE eEncodingModePreset;
        OMX_VIDEO_RATECONTROL_PRESETTYPE   eRateControlPreset;
    }OMX_VIDEO_SVC_ENCODER_PRESETTYPE;


    typedef struct OMX_VIDEO_SVC_VUIINFOTYPE {
        OMX_BOOL                  bAspectRatioPresent;
        OMX_VIDEO_ASPECTRATIOTYPE ePixelAspectRatio;
        OMX_BOOL                  bFullRange;
    }OMX_VIDEO_SVC_VUIINFOTYPE;


    typedef struct OMX_VIDEO_SVC_HRDBUFFERSETTING {
        OMX_U32 nInitialBufferLevel;
        OMX_U32 nHRDBufferSize;
        OMX_U32 nTargetBitrate;
    }OMX_VIDEO_SVC_HRDBUFFERSETTING;


    typedef struct OMX_VIDEO_SVC_INTRAPERIOD {
        OMX_U32 nIDRPeriod;
        OMX_U32 nPFrames;
    } OMX_VIDEO_SVC_INTRAPERIOD;


    typedef struct OMX_VIDEO_SVC_PIXELINFOTYPE {
        OMX_U32 nWidth;
        OMX_U32 nHeight;
    } OMX_VIDEO_SVC_PIXELINFOTYPE;


    typedef struct OMX_VIDEO_SVC_MESEARCHRANGETYPE {
        OMX_VIDEO_MOTIONVECTORTYPE eMVAccuracy;
        OMX_U32                    nHorSearchRangeP;
        OMX_U32                    nVerSearchRangeP;
        OMX_U32                    nHorSearchRangeB;
        OMX_U32                    nVerSearchRangeB;
    }OMX_VIDEO_SVC_MESEARCHRANGETYPE;


    typedef struct OMX_VIDEO_SVC_QPSETTINGSTYPE {
        OMX_U32 nQpI;
        OMX_U32 nQpMaxI;
        OMX_U32 nQpMinI;
        OMX_U32 nQpP;
        OMX_U32 nQpMaxP;
        OMX_U32 nQpMinP;
        OMX_U32 nQpOffsetB;
        OMX_U32 nQpMaxB;
        OMX_U32 nQpMinB;
    }OMX_VIDEO_SVC_QPSETTINGSTYPE;


    typedef struct OMX_VIDEO_SVC_SLICECODINGTYPE {
        OMX_VIDEO_AVCSLICEMODETYPE eSliceMode;
        OMX_U32                    nSlicesize;
    }OMX_VIDEO_SVC_SLICECODINGTYPE;


    typedef struct OMX_VIDEO_EXEC_SVC_HRDBUFFERSETTING {
        OMX_U32 nHRDBufferSize;
        OMX_U32 nEncodeBitrate;
    }OMX_VIDEO_EXEC_SVC_HRDBUFFERSETTING;

/**
 * SVC params
 *
 * STRUCT MEMBERS:
 *  nSize                     : Size of the structure in bytes
 *  nVersion                  : OMX specification version information
 *  nPortIndex                : Port that this structure applies to
 *  nSliceHeaderSpacing       : Number of macroblocks between slice header, put
 *                              zero if not used
 *  nPFrames                  : Number of P frames between each I frame
 *  nBFrames                  : Number of B frames between each I frame
 *  bUseHadamard              : Enable/disable Hadamard transform
 *  nRefFrames                : Max number of reference frames to use for inter
 *                              motion search (1-16)
 *  nRefIdxTrailing           : Pic param set ref frame index (index into ref
 *                              frame buffer of trailing frames list), B frame
 *                              support
 *  nRefIdxForward            : Pic param set ref frame index (index into ref
 *                              frame buffer of forward frames list), B frame
 *                              support
 *  bEnableUEP                : Enable/disable unequal error protection. This
 *                              is only valid of data partitioning is enabled.
 *  bEnableFMO                : Enable/disable flexible macroblock ordering
 *  bEnableASO                : Enable/disable arbitrary slice ordering
 *  bEnableRS                 : Enable/disable sending of redundant slices
 *  eProfile                  : AVC profile(s) to use
 *  eLevel                    : AVC level(s) to use
 *  nAllowedPictureTypes      : Specifies the picture types allowed in the
 *                              bitstream
 *  bFrameMBsOnly             : specifies that every coded picture of the
 *                              coded video sequence is a coded frame
 *                              containing only frame macroblocks
 *  bMBAFF                    : Enable/disable switching between frame and
 *                              field macroblocks within a picture
 *  bEntropyCodingCABAC       : Entropy decoding method to be applied for the
 *                              syntax elements for which two descriptors appear
 *                              in the syntax tables
 *  bWeightedPPrediction      : Enable/disable weighted prediction shall not
 *                              be applied to P and SP slices
 *  nWeightedBipredicitonMode : Default weighted prediction is applied to B
 *                              slices
 *  bconstIpred               : Enable/disable intra prediction
 *  bDirect8x8Inference       : Specifies the method used in the derivation
 *                              process for luma motion vectors for B_Skip,
 *                              B_Direct_16x16 and B_Direct_8x8 as specified
 *                              in subclause 8.4.1.2 of the AVC spec
 *  bDirectSpatialTemporal    : Flag indicating spatial or temporal direct
 *                              mode used in B slice coding (related to
 *                              bDirect8x8Inference) . Spatial direct mode is
 *                              more common and should be the default.
 *  nCabacInitIdx             : Index used to init CABAC contexts
 *  eLoopFilterMode           : Enable/disable loop filter
 */
    typedef struct OMX_TI_VIDEO_PARAM_SVCTYPE {
        OMX_U32         nSize;
        OMX_VERSIONTYPE nVersion;
        OMX_U32         nPortIndex;

        OMX_U32                nActualFrameWidth;
        OMX_U32                nActualFrameHeight;
        OMX_S32                nStride;
        OMX_U32                xFramerate;
        OMX_COLOR_FORMATTYPE   eColorFormat;
        OMX_VIDEO_SVC_RECTTYPE sRecType;

        OMX_VIDEO_SVC_STD_PARAMS sBasicParams;

        OMX_U32                     nRefFrames;
        OMX_TI_VIDEO_SVCPROFILETYPE eProfile;
        OMX_TI_VIDEO_SVCLEVELTYPE   eLevel;

        OMX_U32                   xEncodeFramerate;
        OMX_VIDEO_SVC_BITRATETYPE sBitRateParams;

        OMX_VIDEO_SVC_MOTIONVECTORTYPE sMotionVectorParams;
        OMX_VIDEO_SVC_QUANTIZATIONTYPE sQuantizationParams;
        OMX_VIDEO_SVC_INTRAREFRESHTYPE sIntraRefreshParams;
        OMX_VIDEO_SVC_VBSMCTYPE        sVBSMCParams;

        //OMX_NALUFORMATSTYPE               eNaluFormat;
        OMX_VIDEO_SVC_NALUCONTROLTYPE sNalUnitParams;

        OMX_VIDEO_SVC_MEBLOCKSIZETYPE    sMEBlockSizeParams;
        OMX_VIDEO_SVC_INTRAPREDTYPE      sIntraPredParams;
        OMX_VIDEO_SVC_ENCODER_PRESETTYPE sEncPresetParams;
        OMX_VIDEO_TRANSFORMBLOCKSIZETYPE eTransformBlocksize;
        OMX_VIDEO_SVC_VUIINFOTYPE        sVUIInfoParams;
        OMX_VIDEO_SVC_HRDBUFFERSETTING   sHRDBufferParams;

        OMX_U32 nNumTemporalLayers;
        OMX_S32 nDependencyID;
        OMX_S32 nQualityID;
        //OMX_VIDEO_SVC_ENCODE_MODE         eModeOfEncode;

        OMX_U32 nErrorConcealmentMode;
        OMX_U32 nDeblockFilterMode;
    } OMX_TI_VIDEO_PARAM_SVCTYPE;

 typedef struct OMX_TI_VIDEO_CONFIG_SVCLAYERDETAILS {
    OMX_U32         nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32         nPortIndex;

    OMX_U32 nNumLayers;
    OMX_U32 nLayerId;
    OMX_U8  nPriorityId;
    OMX_U8  nDependencyId;
    OMX_U8  nQualityId;
    OMX_U8  nTemporalId;
    OMX_U8  nBitrateInfoPresentFlag;
    OMX_U8  nFramerateInfoPresentFlag;
    OMX_U8  nFramesizeInfoPresentFlag;
    OMX_U16 nAvgBitrate;
    OMX_U16 nMaxBitrate;
    OMX_U16 nAvgFramerate;
    OMX_U32 nFrameWidth;
    OMX_U32 nFrameHeight;

    OMX_U32 nLayerIndex;     /* Used to query for individual layer details */

} OMX_TI_VIDEO_CONFIG_SVCLAYERDETAILS;

typedef struct OMX_TI_VIDEO_CONFIG_SVCTARGETLAYER {
    OMX_U32         nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32         nPortIndex;

    OMX_U32 nSvcTargetLayerDID;
    OMX_U32 nSvcTargetLayerTID;
    OMX_U32 nSvcTargetLayerQID;

} OMX_TI_VIDEO_CONFIG_SVCTARGETLAYER;
/* ========================================================================== */
/*!
@brief OMX_TI_VIDEO_SLICEDATAINFO : to configure the Slice Settings
@param  nNumofSlices   number of validfields to be read
@param  nSliceSizeConfigured   variable that indicates the MaxSlice configured
                               & (n*nSliceSizeConfigured) gives the buff offset
                               for nth slice in the o/p buffer
@param  nSliceSize   gives the SliceSize
*/
/* ==========================================================================*/
typedef struct OMX_TI_VIDEO_SLICEDATAINFO {
    OMX_U32 nNumofSlices;
    OMX_U32 nSliceSizeConfigured;
    OMX_U32 nSliceSize[OMXH264E_MAX_SLICE_SUPPORTED];
} OMX_TI_VIDEO_SLICEDATAINFO;
/**
*	@brief	mode selection for the data that is given to the Codec
 */

typedef enum _OMX_VIDEO_AVCLTRMODE {
  OMX_H264ENC_LTRP_NONE = 0,
        /**< No longterm refernce frame in the sequnce
           */
  OMX_H264ENC_LTRP_REFERTOIDR = 1,
          /**< Mark all the I frames as long term-reference frames and
           * based on the frame control IH264ENC_Control, refere to
           * a long-term reference frame (I frame).
           */
  OMX_H264ENC_LTRP_REFERTOP_PROACTIVE =2,
          /**< Two long term frames are supported in this schme and
           * long-term index marking and refernce frame update is done based
           * the IH264ENC_Control values
           */
  OMX_H264ENC_LTRP_REFERTOP_REACTIVE = 3
          /**< This is not supported in the current version of encoder
           */
} OMX_VIDEO_AVCLTRMODE;


/* ============================================================================= */
/*
@brief OMX_TI_VIDEO_PARAM_AVC_LTRP : Structure to enable the configuration of Long Term reference Picture feature in H264 Encoder for the session
Enabling this parameter will instruct encoder to keep its recent I/IDR frame in its reference buffer list.
So it increases the DDR foot print by one frame buffer
@param eLTRMode	: enables the LongTerm Reference Picture, possible modes: 0, 1, 2
@param nLTRInterval : interval of the write indicating to codec interms of the frame number
*/
/* ============================================================================= */
typedef struct OMX_TI_VIDEO_PARAM_AVC_LTRP{
	OMX_U32 nSize;
	OMX_VERSIONTYPE nVersion;
	OMX_U32 nPortIndex;
	OMX_VIDEO_AVCLTRMODE eLTRMode;
	OMX_U32 nLTRInterval;
} OMX_TI_VIDEO_PARAM_AVC_LTRP;

/*
@brief OMX_TI_VIDEO_CONFIG_AVC_LTRP : Structure to provide the configuration to acknowledge successful decode of previous LTR
@param eLTRFrameDecoded	: tells the decoder that the LTR has been decoded successfully when set to TRUE
*/
/* ============================================================================= */
typedef struct OMX_TI_VIDEO_CONFIG_AVC_LTRP{
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bEnableNextLTR;
} OMX_TI_VIDEO_CONFIG_AVC_LTRP;

/* ============================================================================= */
/*
@brief OMX_TI_VIDEO_CONFIG_AVC_LTRP_INTERVAL : Structure to enable the update of the LTRP
Interval during runtime
@param nLTRInterval : interval of the write indicating to codec interms of the frame number
*/
/* ============================================================================= */
typedef struct OMX_TI_VIDEO_CONFIG_AVC_LTRP_INTERVAL{
	OMX_U32 nSize;
	OMX_VERSIONTYPE nVersion;
	OMX_U32 nPortIndex;
	OMX_U32 nLTRInterval;
} OMX_TI_VIDEO_CONFIG_AVC_LTRP_INTERVAL;

/* ============================================================================= */
/*
@brief OMX_TI_VIDEO_CONFIG_AVC_LTRP_INTERVAL : Structure to enable timestamps in decode order
            at i/p of decoders.
*/
/* ============================================================================= */
typedef struct OMX_TI_PARAM_TIMESTAMP_IN_DECODE_ORDER{
	OMX_U32 nSize;
	OMX_VERSIONTYPE nVersion;
        OMX_BOOL bEnabled;
} OMX_TI_PARAM_TIMESTAMP_IN_DECODE_ORDER;

/* ============================================================================= */
/*
@brief OMX_TI_VIDEO_PARAM_AUTO_FRAMERATE_UPDATE : Structure to enable dynamic update of frame rate
*/
/* ============================================================================= */
typedef struct OMX_TI_VIDEO_PARAM_AUTO_FRAMERATE_UPDATE {
    OMX_U32         nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32         nPortIndex;
    OMX_BOOL        bEnableAutoVFRUpdate;
    OMX_U32         nDiffThresholdtoUpdate;
    OMX_U32         nMaxSessionFrameRate;
} OMX_TI_VIDEO_PARAM_AUTO_FRAMERATE_UPDATE;

/* ============================================================================= */
/*
@brief OMX_TI_PARAM_SKIP_GREY_OUTPUT_FRAMES : Structure to enable feature to skip grey output
           frames which doesn't have proper reference.
*/
/* ============================================================================= */
typedef struct OMX_TI_PARAM_SKIP_GREY_OUTPUT_FRAMES {
    OMX_U32         nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_BOOL        bEnabled;
} OMX_TI_PARAM_SKIP_GREY_OUTPUT_FRAMES;

/* ============================================================================= */
/*
@brief OMX_TI_PARAM_DECMETADATA : Structure to enable different codec metadata
           for video decoders.
*/
/* ============================================================================= */
typedef struct OMX_TI_PARAM_DECMETADATA {
    OMX_U32         nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32         nPortIndex;
    OMX_BOOL        bEnableMBInfo;
    OMX_BOOL        bEnableTranscodeMode;
    OMX_BOOL        bEnableSEIInfo;
    OMX_BOOL        bEnableVUIInfo;
} OMX_TI_PARAM_DECMETADATA;

/**
 ******************************************************************************
 *  @enum       OMX_TI_VIDEO_MBERRSTATUS
 *  @brief      This enum indicates if a MB was in error or not
 *
 ******************************************************************************
*/
typedef enum {
    OMX_TI_VIDEO_MB_NOERROR = 0,
    /**
    *  MB was non-erroneous
    */
    OMX_TI_VIDEO_MB_ERROR = 1
                            /**
                            * MB was erroneous
                            */
} OMX_TI_VIDEO_MBERRSTATUS;


/**
 *  Macro definitions required for SEI support: HRD sequence parameter set
 */
#define OMX_TI_VIDEO_H264VDEC_MAXCPBCNT        32

/**
 *  Macro definitions required for SEI support: HRD sequence parameter set
 */
#define OMX_TI_VIDEO_H264VDEC_MAXUSERDATA_PAYLOAD 300

/**
 ******************************************************************************
 *  @struct OMX_TI_VIDEO_H264VDEC_HrdParams
 *
 *  @brief  This structure contains the HRD parameter elements.
 *
 *  @param  cpb_cnt_minus1 : Number of alternative CPB specifications in the
 *                           bit-stream
 *  @param  bit_rate_scale : Together with bit_rate_value[i], it specifies the
 *                           maximum input bit-rate for the ith CPB.
 *  @param  cpb_size_scale : Together with cpb_size_value[i], specifies the
 *                           maximum CPB size for the ith CPB.
 *  @param  bit_rate_value_minus1[OMX_TI_VIDEO_H264VDEC_MAXCPBCNT] :Maximum input bitrate
 *                                                     for the ith CPB
 *  @param  cpb_size_value_minus1[OMX_TI_VIDEO_H264VDEC_MAXCPBCNT] :Maximum CPB size for the
 *                                                     ith CPB
 *  @param  vbr_cbr_flag[OMX_TI_VIDEO_H264VDEC_MAXCPBCNT] :Specifies the ith CPB is operated
 *                          in Constant Bit-rate mode or variable bit-rate mode
 *  @param  initial_cpb_removal_delay_length_minus1 :Length in bits of
 *                                   initial_cpb_removal_length syntax element
 *  @param  cpb_removal_delay_length_minus1 :Length in bits of
 *                                      cpb_removal_delay_length syntax element
 *  @param  dpb_output_delay_length_minus1 :Length in bits of
 *                                       dpb_output_delay_length syntax element
 *  @param  time_offset_length : Length in bits of time_offset syntax element
 ******************************************************************************
*/
typedef struct OMX_TI_VIDEO_H264VDEC_HrdParams {
    OMX_U32 cpb_cnt_minus1;
    OMX_U8  bit_rate_scale;
    OMX_U8  cpb_size_scale;
    OMX_U32 bit_rate_value_minus1[OMX_TI_VIDEO_H264VDEC_MAXCPBCNT];
    OMX_U32 cpb_size_value_minus1[OMX_TI_VIDEO_H264VDEC_MAXCPBCNT];
    OMX_U8  vbr_cbr_flag[OMX_TI_VIDEO_H264VDEC_MAXCPBCNT];
    OMX_U8  initial_cpb_removal_delay_length_minus1;
    OMX_U8  cpb_removal_delay_length_minus1;
    OMX_U8  dpb_output_delay_length_minus1;
    OMX_U8  time_offset_length;
} OMX_TI_VIDEO_H264VDEC_HrdParams;

/**
 ******************************************************************************
 *  @struct OMX_TI_VIDEO_H264VDEC_SVCVuiParams
 *
 *  @brief   This structure contains VUI  message syntax elements for scalable
 *           video stream
 *
 *  @param  parsed_flag :1 - Indicates that in the current process call, c
 *                            contents of the structure is updated
 *                       0 - Indicates contents of the structure is not updated
 *
 *  @param  svc_vui_ext_num_entries_minus1:(svc_vui_ext_num_entries_minus1 + 1)
 *                      specifies the number of information
 *                        entries that are present in the SVC
 *                      VUI parameters extension syntax
 *                        structure
 *  @param  svc_vui_ext_dependency_id:indicate the max value of DId for the
 *                    i-th subset of coded video sequences
 *  @param  svc_vui_ext_quality_id:indicate the max value of QId for the
 *                    i-th subset of coded video sequences
 *  @param  svc_vui_ext_temporal_id: indicate the max value of TId for the
 *                    i-th subset of coded video sequences
 *  @param  svc_vui_ext_timing_info_present_flag: Flag to tells that
 *                          svc_vui_ext_num_units_in_tick,
 *                          svc_vui_ext_time_scale,
 *                          svc_vui_ext_fixed_frame_rate_flag
 *                          are present for current coded
 *                          sequence or not.
 *  @param  svc_vui_ext_num_units_in_tick: specifies the value of num_units_in_tick
 *  @param  svc_vui_ext_time_scale: specifies the value of time_scale
 *  @param  svc_vui_ext_fixed_frame_rate_flag: specifies the value of
 *                         fixed_frame_rate_flag
 *  @param  svc_vui_ext_nal_hrd_parameters_present_flag:specifies the
 *                value of nal_hrd_parameters_present_flag
 *  @param  svc_vui_ext_vcl_hrd_parameters_present_flag: ] specifies the
 *                value of vcl_hrd_parameters_present_flag
 *  @param  svc_vui_ext_low_delay_hrd_flag: specifies the value
 *          of low_delay_hrd_flag
 *  @param  svc_vui_ext_pic_struct_present_flag: specifies the value
 *           of pic_struct_present_flag
 *
 ******************************************************************************
*/

typedef struct sOMX_TI_VIDEO_H264VDEC_SVCVuiParams {
    OMX_U32 parsed_flag;
    OMX_U16 svc_vui_ext_num_entries_minus1;
    OMX_U16 svc_vui_ext_dependency_id;
    OMX_U16 svc_vui_ext_quality_id;
    OMX_U16 svc_vui_ext_temporal_id;
    OMX_U16 svc_vui_ext_timing_info_present_flag;
    OMX_U32 svc_vui_ext_num_units_in_tick;
    OMX_U32 svc_vui_ext_time_scale;
    OMX_U16 svc_vui_ext_fixed_frame_rate_flag;
    OMX_U16 svc_vui_ext_nal_hrd_parameters_present_flag;
    OMX_U16 svc_vui_ext_vcl_hrd_parameters_present_flag;
    OMX_U16 svc_vui_ext_low_delay_hrd_flag;
    OMX_U16 svc_vui_ext_pic_struct_present_flag;
} OMX_TI_VIDEO_H264VDEC_SVCVuiParams;

/**
 ******************************************************************************
 *  @struct OMX_TI_VIDEO_H264VDEC_VuiParams
 *
 *  @brief  This structure contains the VUI Sequence Parameter elements.
 *
 *  @param  parsed_flag :1 - Indicates that in the current process call, c
 *                            contents of the structure is updated
 *                       0 - Indicates contents of the structure is not updated
 *  @param  aspect_ratio_info_present_flag :Indicates whether aspect ratio idc
 *                                          is present or not.
 *  @param  aspect_ratio_idc : Aspect ratio of Luma samples
 *  @param  sar_width : Horizontal size of sample aspect ratio
 *  @param  sar_height : Vertical size of sample aspect ratio
 *  @param  overscan_info_present_flag : Cropped decoded pictures are suitable
 *                                       for display or not.
 *  @param  overscan_appropriate_flag : Overscan_appropriate_flag
 *  @param  video_signal_type_present_flag : Flag indicates whether
 *          video_format, video_full_range_flag and colour_description_present_
 *          flag are present or not
 *  @param  video_format :Video format indexed by a table. For example,PAL/NTSC
 *  @param  video_full_range_flag : Black level, luma and chroma ranges. It
 *                                  should be used for BT.601 compliance
 *  @param  colour_description_present_flag:Indicates whether colour_primaries,
 *                transfer_characteristics and matrix_coefficients are present.
 *  @param  colour_primaries :Chromaticity co-ordinates of source primaries
 *  @param  transfer_characteristics :Opto-electronic transfer characteristics
 *          of the source picture
 *  @param  matrix_coefficients :Matrix coefficients for deriving Luma and
 *          chroma data from RGB components.
 *  @param  chroma_location_info_present_flag : Flag indicates whether
 *          chroma_sample_loc_type_top field and chroma_sample_loctype
 *          bottom_field are present.
 *  @param  chroma_sample_loc_type_top_field : Location of chroma_sample top
 *          field
 *  @param  chroma_sample_loc_type_bottom_field :Location of chroma_sample
 *          bottom field
 *  @param  timing_info_present_flag :Indicates whether num_units_in_tick,
 *          time_scale, and fixed_frame_rate_flag are present.
 *  @param  num_units_in_tick :Number of units of a clock that corresponds to 1
 *          increment of a clock tick counter
 *  @param  time_scale :Indicates actual increase in time for 1 increment of a
 *          clock tick counter
 *  @param  fixed_frame_rate_flag :Indicates how the temporal distance between
 *          HRD output times of any two output pictures is constrained
 *  @param  nal_hrd_parameters_present_flag :Indicates whether
 *          nal_hrd_parameters are present
 *  @param  nal_hrd_pars : NAL HRD Parameters
 *  @param  vcl_hrd_parameters_present_flag :Indicates whether
 *          vcl_hrd_parameters are present
 *  @param  vcl_hrd_pars : VCL HRD Parameters
 *  @param  low_delay_hrd_flag :HRD operational mode as in Annex C of the
 *          standard
 *  @param  pic_struct_present_flag :Indicates whether picture timing SEI
 *          messages are present
 *  @param  bitstream_restriction_flag :Indicates if the bit-stream restriction
 *          parameters are present
 *  @param  motion_vectors_over_pic_boundaries_flag :Specifies whether motion
 *          vectors can point to regions outside the picture boundaries
 *  @param  max_bytes_per_pic_denom :Maximum number of bytes not exceeded by
 *          the sum of sizes of all VCL NAL units of a single coded picture
 *  @param  max_bits_per_mb_denom :Maximum number of bits taken by any coded MB
 *  @param  log2_max_mv_length_vertical :Maximum value of any motion vector\u2019s
 *          vertical component
 *  @param  log2_max_mv_length_horizontal :Maximum value of any motion vector\u2019s
 *           horizontal component
 *  @param  max_dec_frame_reordering :
 *  @param  num_reorder_frames :Maximum number of frames that need to be
 *          re-ordered
 *  @param  max_dec_frame_buffering :Size of HRD decoded buffer (DPB) in terms
 *          of frame buffers
 *  @param  svcVuiParams :  struct instance of vui parameters for svc
 *
 ******************************************************************************
*/
typedef struct OMX_TI_VIDEO_H264VDEC_VuiParams {
    OMX_U32                            parsed_flag;
    OMX_U8                             aspect_ratio_info_present_flag;
    OMX_U32                            aspect_ratio_idc;
    OMX_U32                            sar_width;
    OMX_U32                            sar_height;
    OMX_U8                             overscan_info_present_flag;
    OMX_U8                             overscan_appropriate_flag;
    OMX_U8                             video_signal_type_present_flag;
    OMX_U8                             video_format;
    OMX_U8                             video_full_range_flag;
    OMX_U8                             colour_description_present_flag;
    OMX_U8                             colour_primaries;
    OMX_U8                             transfer_characteristics;
    OMX_U8                             matrix_coefficients;
    OMX_U8                             chroma_location_info_present_flag;
    OMX_U32                            chroma_sample_loc_type_top_field;
    OMX_U32                            chroma_sample_loc_type_bottom_field;
    OMX_U8                             timing_info_present_flag;
    OMX_U32                            num_units_in_tick;
    OMX_U32                            time_scale;
    OMX_U8                             fixed_frame_rate_flag;
    OMX_U8                             nal_hrd_parameters_present_flag;
    OMX_TI_VIDEO_H264VDEC_HrdParams    nal_hrd_pars;
    OMX_U8                             vcl_hrd_parameters_present_flag;
    OMX_TI_VIDEO_H264VDEC_HrdParams    vcl_hrd_pars;
    OMX_U8                             low_delay_hrd_flag;
    OMX_U8                             pic_struct_present_flag;
    OMX_U8                             bitstream_restriction_flag;
    OMX_U8                             motion_vectors_over_pic_boundaries_flag;
    OMX_U32                            max_bytes_per_pic_denom;
    OMX_U32                            max_bits_per_mb_denom;
    OMX_U32                            log2_max_mv_length_vertical;
    OMX_U32                            log2_max_mv_length_horizontal;
    OMX_U32                            max_dec_frame_reordering;
    OMX_U32                            num_reorder_frames;
    OMX_U32                            max_dec_frame_buffering;
    OMX_TI_VIDEO_H264VDEC_SVCVuiParams svcVuiParams;
} OMX_TI_VIDEO_H264VDEC_VuiParams;

/**
 ******************************************************************************
 *  @struct OMX_TI_VIDEO_H264VDEC_SeiUserDataRegITUT
 *
 *  @brief  This structure contains the user data SEI msg elements
 *
 *  @param  parsed_flag :1 - Indicates that in the current process call,
 *                           contents of the structure is updated
 *                       0 - Indicates contents of the structure is not updated
 *  @param  num_payload_bytes :Specifies the size of the payload
 *  @param  itu_t_t35_country_code : A byte having a value specified as a
 *                          country code by ITU-T Recommendation T.35 Annex A
 *  @param  itu_t_t35_country_code_extension_byte :A byte having a value
 *          specified as a country code by ITU-T Recommendation T.35 Annex B
 *  @param  itu_t_t35_payload_byte[] : A byte containing data registered as
 *          specified by ITU-T Recommendation T.35.
 *  @param  dataOverflowFlag: This indicates if pay load data is more than the
 *                            array size i.e., OMX_TI_VIDEO_H264VDEC_MAXUSERDATA_PAYLOAD.
 ******************************************************************************
*/
typedef struct OMX_TI_VIDEO_H264VDEC_SeiUserDataRegITUT {
    OMX_U32 parsed_flag;
    OMX_U32 num_payload_bytes;
    OMX_U8  itu_t_t35_country_code;
    OMX_U8  itu_t_t35_country_code_extension_byte;
    OMX_U8  itu_t_t35_payload_byte[OMX_TI_VIDEO_H264VDEC_MAXUSERDATA_PAYLOAD];
    OMX_U8  dataOverflowFlag;
} OMX_TI_VIDEO_H264VDEC_SeiUserDataRegITUT;

/**
 ******************************************************************************
 *  @struct OMX_TI_VIDEO_H264VDEC_SeiUserDataUnReg
 *
 *  @brief  This structure contains the user data SEI msg elements
 *
 *  @param  parsed_flag :1 - Indicates that in the current process call,
 *                           contents of the structure is updated
 *                       0 - Indicates contents of the structure is not updated
 *  @param  num_payload_bytes :Specifies the size of the payload
 *  @param  uuid_iso_iec_11578 :Value specified as a UUID according to the
 *                              procedures of ISO/IEC 11578:1996 Annex A.
 *  @param  user_data_payload_byte :Byte containing data having syntax and
 *                                semantics as specified by the UUID generator.
 *  @param  dataOverflowFlag: This indicates if pay load data is more than the
 *                            array size i.e., OMX_TI_VIDEO_H264VDEC_MAXUSERDATA_PAYLOAD.
 ******************************************************************************
*/
typedef struct OMX_TI_VIDEO_H264VDEC_SeiUserDataUnReg {
    OMX_U32 parsed_flag;
    OMX_U32 num_payload_bytes;
    OMX_U8  uuid_iso_iec_11578[16];
    OMX_U8  user_data_payload_byte[OMX_TI_VIDEO_H264VDEC_MAXUSERDATA_PAYLOAD];
    OMX_U8  dataOverflowFlag;
} OMX_TI_VIDEO_H264VDEC_SeiUserDataUnReg;


/**
 ******************************************************************************
 *  @struct OMX_TI_VIDEO_H264VDEC_SeiBufferingPeriod
 *
 *  @brief   This structure contains the buffering period SEI msg elements
 *
 *  @param  parsed_flag :1 - Indicates that in the current process call,
 *                           contents of the structure is updated
 *                       0 - Indicates contents of the structure is not updated
 *  @param  seq_parameter_set_id :Specifies the sequence parameter set that
 *                                contains the sequence HRD attributes
 *  @param  nal_cpb_removal_delay :Specifies the delay for the indexed NAL CPB
 *          between the time of arrival in the CPB of the first bit of the
 *          coded data associated with the access unit associated with the
 *          buffering period SEI message and the time of removal from the CPB
 *          of the coded data associated with the same access unit, for the
 *          first buffering period after HRD initialization.
 *  @param  nal_cpb_removal_delay_offset :Used for the indexed NAL CPB in
 *          combination with the cpb_removal_delay to specify the initial
 *          delivery time of coded access units to the CPB
 *  @param  vcl_cpb_removal_delay :Specifies the delay for the indexed VCL CPB
 *          between the time of arrival in the CPB of the first bit of the
 *          coded data associated with the access unit associated with the
 *          buffering period SEI message and the time of removal from the CPB
 *          of the coded data associated with the same access unit, for the
 *          first buffering period after HRD initialization.
 *  @param  vcl_cpb_removal_delay_offset :Used for the indexed VCL CPB in
 *          combination with the cpb_removal_delay to specify the initial
 *          delivery time of coded access units to the CPB
 ******************************************************************************
*/
typedef struct OMX_TI_VIDEO_H264VDEC_SeiBufferingPeriod {
    OMX_U32 parsed_flag;
    OMX_U32 seq_parameter_set_id;
    OMX_U32 nal_cpb_removal_delay[OMX_TI_VIDEO_H264VDEC_MAXCPBCNT];
    OMX_U32 nal_cpb_removal_delay_offset[OMX_TI_VIDEO_H264VDEC_MAXCPBCNT];
    OMX_U32 vcl_cpb_removal_delay[OMX_TI_VIDEO_H264VDEC_MAXCPBCNT];
    OMX_U32 vcl_cpb_removal_delay_offset[OMX_TI_VIDEO_H264VDEC_MAXCPBCNT];
}OMX_TI_VIDEO_H264VDEC_SeiBufferingPeriod;
/**
 ******************************************************************************
 *  @struct OMX_TI_VIDEO_H264VDEC_SeiPanScanRect
 *
 *  @brief   This structure contains the pan scan rectangle SEI msg elements
 *
 *  @param  parsed_flag :1 - Indicates that in the current process call,
 *                           contents of the structure is updated
 *                       0 - Indicates contents of the structure is not updated
 *  @param  pan_scan_rect_id :Specifies an identifying number that may be used
 *                            to identify the purpose of the pan-scan rectangle
 *  @param  pan_scan_rect_cancel_flag :Equal to 1 indicates that the SEI
 *                    message cancels the persistence of any previous pan-scan
 *                    rectangle SEI message in output order.
 *                    pan_scan_rect_cancel_flag equal to 0 indicates that
 *                    pan-scan rectangle information follows.
 *  @param  pan_scan_cnt_minus1 :Specifies the number of pan-scan rectangles
 *          that are present in the SEI message
 *  @param  pan_scan_rect_left_offset :Specifies as signed integer quantities
 *          in units of one-sixteenth sample spacing relative to the luma
 *          sampling grid, the location of the pan-scan rectangle
 *  @param  pan_scan_rect_right_offset :Specifies as signed integer quantities
 *          in units of one-sixteenth sample spacing relative to the luma
 *          sampling grid, the location of the pan-scan rectangle
 *  @param  pan_scan_rect_top_offset : Top offset
 *  @param  pan_scan_rect_bottom_offset : Bottom offset
 *  @param  pan_scan_rect_repetition_period :Specifies the persistence of the
 *          pan-scan rectangle SEI message and may specify a picture order
 *          count interval within which another pan-scan rectangle SEI message
 *          with the same value of pan_scan_rect_id or the end of the coded
 *          video sequence shall be present in the bit-stream
 ******************************************************************************
*/
typedef struct OMX_TI_VIDEO_H264VDEC_SeiPanScanRect {
    OMX_U32 parsed_flag;
    OMX_U32 pan_scan_rect_id;
    OMX_U32 pan_scan_rect_cancel_flag;
    OMX_U32 pan_scan_cnt_minus1;
    OMX_S32 pan_scan_rect_left_offset[3];
    OMX_S32 pan_scan_rect_right_offset[3];
    OMX_S32 pan_scan_rect_top_offset[3];
    OMX_S32 pan_scan_rect_bottom_offset[3];
    OMX_U32 pan_scan_rect_repetition_period;
} OMX_TI_VIDEO_H264VDEC_SeiPanScanRect;

/**
 ******************************************************************************
 *  @struct OMX_TI_VIDEO_H264VDEC_SeiProgRefineStart
 *
 *  @brief  This structure contains the progressive refinement start SEI msg
 *          elements
 *
 *  @param  parsed_flag :1 - Indicates that in the current process call,
 *                           contents of the structure is updated
 *                       0 - Indicates contents of the structure is not updated
 *  @param  progressive_refinement_id :Specifies an identification number for
 *          the progressive refinement operation.
 *  @param  num_refinement_steps_minus1 :Specifies the number of reference
 *          frames in the tagged set of consecutive coded pictures
 ******************************************************************************
*/
typedef struct OMX_TI_VIDEO_H264VDEC_SeiProgRefineStart {
    OMX_U32 parsed_flag;
    OMX_U32 progressive_refinement_id;
    OMX_U32 num_refinement_steps_minus1;
} OMX_TI_VIDEO_H264VDEC_SeiProgRefineStart;
/**
 ******************************************************************************
 *  @struct OMX_TI_VIDEO_H264VDEC_SeiProgRefineEnd
 *
 *  @brief  TThis structure contains the progressive refinement end SEI msg
 *          elements
 *
 *  @param  parsed_flag :1 - Indicates that in the current process call,
 *                           contents of the structure is updated
 *                       0 - Indicates contents of the structure is not updated
 *  @param  progressive_refinement_id :Specifies an identification number for
 *                    the progressive refinement operation.
 ******************************************************************************
*/
typedef struct OMX_TI_VIDEO_H264VDEC_SeiProgRefineEnd {
    OMX_U32 parsed_flag;
    OMX_U32 progressive_refinement_id;
} OMX_TI_VIDEO_H264VDEC_SeiProgRefineEnd;
/**
 ******************************************************************************
 *  @struct OMX_TI_VIDEO_H264VDEC_SeiRecoveryPointInfo
 *
 *  @brief  This structure contains the sRecovery Point Info SEI msg elements
 *
 *  @param  parsed_flag :1 - Indicates that in the current process call,
 *                           contents of the structure is updated
 *                       0 - Indicates contents of the structure is not updated
 *  @param  exact_match_flag :Indicates whether decoded pictures at and
 *          subsequent to the specified recovery point in output order derived
 *          by starting the decoding process at the access unit associated with
 *          the recovery point SEI message, will be an exact match to the
 *          pictures that would be produced by starting the decoding process
 *        at the location of a previous IDR access unit in the NAL unit stream.
 *  @param  recovery_frame_cnt :Specifies the recovery point of output pictures
 *          in output order
 *  @param  broken_link_flag :Indicates the presence or absence of a broken
 *                            link in the NAL unit stream
 *  @param  changing_slice_group_idc :Indicates whether decoded pictures are
 *          correct or approximately correct in content at and subsequent to
 *          the recovery point in output order when all macro-blocks of the
 *          primary coded pictures are decoded within the changing slice group
 *          period.
 ******************************************************************************
*/
typedef struct OMX_TI_VIDEO_H264VDEC_SeiRecoveryPointInfo {
    OMX_U32 parsed_flag;
    OMX_U32 recovery_frame_cnt;
    OMX_U32 exact_match_flag;
    OMX_U32 broken_link_flag;
    OMX_U32 changing_slice_group_idc;
} OMX_TI_VIDEO_H264VDEC_SeiRecoveryPointInfo;

/**
 ******************************************************************************
 *  @struct OMX_TI_VIDEO_H264VDEC_SeiPictureTiming
 *
 *  @brief  This structure contains the picture timing SEI msg elements
 *
 *  @param  parsed_flag :1 - Indicates that in the current process call,
 *                           contents of the structure is updated
 *                       0 - Indicates contents of the structure is not updated
 *  @param     NumClockTs :
 *  @param     cpb_removal_delay :Specifies how many clock ticks to wait after
 *               removal from the CPB of the access unit associated with the
 *               most recent buffering period SEI message before removing from
 *               the buffer the access unit data associated with the picture
 *               timing SEI message.
 *  @param     dpb_output_delay : Used to compute the DPB output time of the
 *               picture.
 *  @param     pic_struct : Indicates whether a picture  should be displayed as
 *               a frame or field
 *  @param     clock_time_stamp_flag[4]:1 - Indicates number of clock timestamp
 *                            syntax elements present and follow immediately
 *                            0 \u2013 Indicates associated clock timestamp syntax
 *                                elements not present
 *  @param     ct_type[4] : Indicates the scan type(interlaced or progressive)
 *                          of the source material
 *  @param     nuit_field_based_flag[4] : Used to calculate the clockTimestamp
 *  @param     counting_type[4] : Specifies the method of dropping values of
 *                                n_frames
 *  @param     full_timestamp_flag[4] : 1 - Specifies that the n_frames syntax
 *                                      element is followed by seconds_value,
 *                                      minutes_value, and hours_value.
 *                                      0 - Specifies that the n_frames syntax
 *                                      element is followed by seconds_flag
 *  @param     discontinuity_flag[4] : Indicates whether the difference between
 *               the current value of clockTimestamp and the value of
 *               clockTimestamp computed from the previous clockTimestamp in
 *               output order can be interpreted as the time difference between
 *               the times of origin or capture of the associated frames or
 *               fields.
 *  @param     cnt_dropped_flag[4] : Specifies the skipping of one or more
 *               values of n_frames using the counting method
 *  @param     n_frames[4] : Specifies the value of nFrames used to compute
 *                            clockTimestamp.
 *  @param     seconds_flag[4] : equal to 1 specifies that seconds_value and
 *                               minutes_flag are present when
 *                               full_timestamp_flag is equal to 0.
 *  @param     minutes_flag[4] : equal to 1 specifies that minutes_value and
 *                              hours_flag are present when full_timestamp_flag
 *                              is equal to 0 and seconds_flag is equal to 1.
 *  @param     hours_flag[4] :  equal to 1 specifies that hours_value is
 *                              present when full_timestamp_flag is equal to 0
 *                              and seconds_flag is equal to 1 and minutes_flag
 *                              is equal to 1.
 *  @param     seconds_value[4] : Specifies the value of sS used to compute
 *                                clockTimestamp.
 *  @param     minutes_value[4] : Specifies the value of mM used to compute
 *                                clockTimestamp.
 *  @param     hours_value[4] : Specifies the value of tOffset used to compute
 *                              clockTimestamp
 *  @param     time_offset[4] : Specifies the value of tOffset used to compute
 *                              clockTimestamp
 ******************************************************************************
*/
typedef struct OMX_TI_VIDEO_H264VDEC_SeiPictureTiming {
    OMX_U32 parsed_flag;
    OMX_U32 NumClockTs;
    OMX_U32 cpb_removal_delay;
    OMX_U32 dpb_output_delay;
    OMX_U32 pic_struct;
    OMX_U32 clock_time_stamp_flag[4];
    OMX_U32 ct_type[4];
    OMX_U32 nuit_field_based_flag[4];
    OMX_U32 counting_type[4];
    OMX_U32 full_timestamp_flag[4];
    OMX_U32 discontinuity_flag[4];
    OMX_U32 cnt_dropped_flag[4];
    OMX_U32 n_frames[4];
    OMX_U32 seconds_flag[4];
    OMX_U32 minutes_flag[4];
    OMX_U32 hours_flag[4];
    OMX_U32 seconds_value[4];
    OMX_U32 minutes_value[4];
    OMX_U32 hours_value[4];
    OMX_S32 time_offset[4];
}OMX_TI_VIDEO_H264VDEC_SeiPictureTiming;
/**
 ******************************************************************************
 *  @struct OMX_TI_VIDEO_H264VDEC_SeiFullFrameFreezeRep
 *
 *  @brief  This structure contains the full frmae freeze repetition SEI msg
 *          elements
 *
 *  @param  parsed_flag :1 - Indicates that in the current process call,
 *                           contents of the structure is updated
 *                       0 - Indicates contents of the structure is not updated
 *  @param  full_frame_freeze_repetition_period :Specifies the persistence of
 *            the full-frame freeze SEI message
 ******************************************************************************
*/
typedef struct OMX_TI_VIDEO_H264VDEC_SeiFullFrameFreezeRep {
    OMX_U32 parsed_flag;
    OMX_U32 full_frame_freeze_repetition_period;
} OMX_TI_VIDEO_H264VDEC_SeiFullFrameFreezeRep;

/**
 ******************************************************************************
 *  @struct OMX_TI_VIDEO_H264VDEC_SeiFullFrameFreezeRel
 *
 *  @brief   This structure contains frame freeze release SEI msg elements
 *
 *  @param  parsed_flag :1 - Indicates that in the current process call,
 *                           contents of the structure is updated
 *                       0 - Indicates contents of the structure is not updated
 *  @param  payloadSize : Size of the frame_freeze_release payload
 ******************************************************************************
*/
typedef struct OMX_TI_VIDEO_H264VDEC_SeiFullFrameFreezeRel {
    OMX_U32 parsed_flag;
    OMX_U32 payloadSize;
} OMX_TI_VIDEO_H264VDEC_SeiFullFrameFreezeRel;

/**
 ******************************************************************************
 *  @struct OMX_TI_VIDEO_H264VDEC_SeiStereoVideoInfo
 *
 *  @brief   This structure contains stereo video information SEI msg elements
 *
 *  @param  parsed_flag :1 - Indicates that in the current process call,
 *                           contents of the structure is updated
 *                       0 - Indicates contents of the structure is not updated
 *  @param  field_views_flag :   1 - indicates that all pictures in the current
 *                  coded video sequence are fields
 *                0 - indicates that all pictures in the current
 *                  coded video sequence are frames.
 *  @param  top_field_is_left_view_flag :
 *                1 - top field is a left  view.
 *                0 - topfield is right view.
 *  @param  current_frame_is_left_view_flag :
 *                1 - current frame is left view.
 *                0 - current frame is right view.
 *  @param  next_frame_is_second_view_flag :
 *                1 - current picture and a next picture in
 *                  output order form a stereo video pair.
 *                0 - current picture and a previous picture in
 *                  output order form a stereo video pair.
 *  @param  left_view_self_contained_flag :
 *                1 - it will not use right view as a reference
 *                  picture for inter prediction
 *                0 - it may use right view as a reference
 *                  picture for inter prediction.
 *  @param  right_view_self_contained_flag :
 *                1 - it will not use left view as a reference
 *                  picture for inter prediction
 *                0 - it may use left view as a reference
 *                  picture for inter prediction.
 ******************************************************************************
*/
typedef struct OMX_TI_VIDEO_H264VDEC_SeiStereoVideoInfo {
    OMX_U32 parsed_flag;
    OMX_U32 field_views_flag;
    OMX_U32 top_field_is_left_view_flag;
    OMX_U32 current_frame_is_left_view_flag;
    OMX_U32 next_frame_is_second_view_flag;
    OMX_U32 left_view_self_contained_flag;
    OMX_U32 right_view_self_contained_flag;
} OMX_TI_VIDEO_H264VDEC_SeiStereoVideoInfo;

/**
 ******************************************************************************
 *  @struct OMX_TI_VIDEO_H264VDEC_SeiFramePacking
 *
 *  @brief  This structure contains frame packing arrangement SEI msg elements
 *
 *  @param  frame_packing_arrangement_id :
 *                contains an identifying number that may be used to identify
 *                the usage of the frame packing arrangement SEI message.
 *  @param  frame_packing_arrangement_cancel_flag :
 *                1 - equal to 1 indicates that the frame packing arrangement
 *                    SEI message cancels the persistence of any previous frame
 *                    packing arrangement SEI message in output order.
 *                0 - indicates that frame packing arrangement info follows
 *  @param  frame_packing_arrangement_type :
 *                indicates the type of packing arrangement of the frames
 *  @param  quincunx_sampling_flag :
 *                1 - indicates that each color component plane of each
 *                    constituent frame is quincunx sampled
 *                0 - indicates that each color component plane of each
 *                    constituent frame is not quincunx sampled
 *  @param  content_interpretation_type :
 *                1 - frame 0 being associated with the left view and frame 1
 *                    being associated with the right view
 *                2 - frame 0 being associated with the right view and frame 1
 *                    being associated with the left view
 *  @param  spatial_flipping_flag :
 *                1 - spatial flipping is enabled for any one of the frame
 *                    constituent, if frame_packing_arrangement_type is 3 or 4.
 *                0 - spatial flipping is disabled for any one of the frame
 *                    constituent, if frame_packing_arrangement_type is 3 or 4.
 *  @param  frame0_flipped_flag :
 *                1 - frame 0 is spatially flipped
 *                0 - frame 1 is spatially flipped
 *  @param  field_views_flag :
 *                1 - indicates that all pictures in the current coded video
 *                   sequence are coded as complementary field pairs.
 *                0 - indicates that all pictures in the current coded video
 *                   sequence are coded as frame.
 *  @param  current_frame_is_frame0_flag :
 *                1 - indicates that the current decoded frame is constituent
 *                    frame 0 and the next decoded frame in output order
 *                    is constituent frame 1.
 *                0 - indicates that the current decoded frame is constituent
 *                    frame 1 and the next decoded frame in output order
 *                    is constituent frame 0.
 *  @param  frame0_self_contained_flag :
 *                1 - indicates that the constituent frame 0 is dependent on
 *                    constituent frame 1 in decoding process
 *                0 - indicates that the constituent frame 0 may dependent on
 *                    constituent frame 1 in decoding process
 *  @param  frame1_self_contained_flag :
 *                1 - indicates that the constituent frame 1 is dependent on
 *                    constituent frame 0 in decoding process
 *                0 - indicates that the constituent frame 1 may dependent on
 *                    constituent frame 0 in decoding process
 *  @param  frame0_grid_position_x :
 *                specifies the horizontal location of the upper left
 *                sample of constituent frame 0 in the units of one
 *                sixteenth of the luma samples
 *  @param  frame0_grid_position_y :
 *                specifies the vertical location of the upper left
 *                sample of constituent frame 0 in the units of one
 *                sixteenth of the luma samples
 *  @param  frame1_grid_position_x :
 *                specifies the horizontal location of the upper left
 *                sample of constituent frame 1 in the units of one
 *                sixteenth of the luma samples
 *  @param  frame1_grid_position_y :
 *                specifies the vertical location of the upper left
 *                sample of constituent frame 1 in the units of one
 *                sixteenth of the luma samples
 *  @param  frame_packing_arrangement_reserved_byte :
 *                reserved for the future use.
 *  @param  frame_packing_arrangement_repetition_period :
 *                specifies the persistence of the frame packing arrangement
 *                SEI message and may specify a frame order count interval
 *                within which another frame packing arrangement SEI message
 *                with the same value of frame_packing_arrangement_id or the
 *                end of the coded video sequence shall be present in the
 *                bitstream.
 *  @param  frame_packing_arrangement_extension_flag :
 *                0 - indicates that no additional data follows within the
 *                    frame packing arrangement SEI message.
 *                1 - Reserved for the future use.
 ******************************************************************************
*/
typedef struct OMX_TI_VIDEO_H264VDEC_SeiFramePacking {
    OMX_U32 parsed_flag;
    OMX_U32 frame_packing_arrangement_id;
    OMX_U32 frame_packing_arrangement_repetition_period;
    OMX_U8  frame_packing_arrangement_cancel_flag;
    OMX_U8  frame_packing_arrangement_type;
    OMX_U8  quincunx_sampling_flag;
    OMX_U8  content_interpretation_type;
    OMX_U8  spatial_flipping_flag;
    OMX_U8  frame0_flipped_flag;
    OMX_U8  field_views_flag;
    OMX_U8  current_frame_is_frame0_flag;
    OMX_U8  frame0_self_contained_flag;
    OMX_U8  frame1_self_contained_flag;
    OMX_U8  frame0_grid_position_x;
    OMX_U8  frame0_grid_position_y;
    OMX_U8  frame1_grid_position_x;
    OMX_U8  frame1_grid_position_y;
    OMX_U8  frame_packing_arrangement_reserved_byte;
    OMX_U8  frame_packing_arrangement_extension_flag;
} OMX_TI_VIDEO_H264VDEC_SeiFramePacking;


/**
 ******************************************************************************
 *  @struct OMX_TI_VIDEO_H264VDEC_SeiMessages
 *
 *  @brief   This structure contains all the supported SEI msg objects
 *
 *  @param  parsed_flag :1 - Indicates that in the current process call,
 *                           contents of the structure is updated
 *                       0 - Indicates contents of the structure is not updated
 *  @param  full_frame_freeze : Full-frame freeze SEI message
 *  @param  full_frame_freeze_release :Cancels the effect of any full-frame
 *             freeze SEI message sent with pictures that precede the current
 *             picture in the output order.
 *  @param  prog_refine_start :Specifies the beginning of a set of consecutive
 *             coded pictures that is labeled as the current picture followed
 *             by a sequence of one or more pictures of refinement of the
 *             quality of the current picture, rather than as a representation
 *             of a continually moving scene.
 *  @param  prog_refine_end : Specifies end of progressive refinement.
 *  @param  user_data_registered :Message contains user data registered as
 *            specified by ITU-T Recommendation T.35
 *  @param  user_data_unregistered :Message contains unregistered user data
 *            identified by a UUID
 *  @param  buffering_period_info :Message specifies the buffering period
 *  @param  pan_scan_rect :Message specifies the coordinates of a rectangle
 *            relative to the cropping rectangle of the sequence parameter set
 *  @param  recovery_pt_info :The recovery point SEI message assists a decoder
 *            in determining when the decoding process will produce acceptable
 *            pictures for display after the decoder initiates random access or
 *            after the encoder indicates a broken link in the sequence.
 *  @param  pic_timing :Specifies timing information regarding cpb delays, dpb
*              output delay, and so on.
 *  @param  stereo_video_info :stereo video information SEI message consist of
 *      pair of picture forming stereo view content.
 ******************************************************************************
*/
typedef struct OMX_TI_VIDEO_H264VDEC_SeiMessages {
    OMX_U32                                     parsed_flag;
    OMX_TI_VIDEO_H264VDEC_SeiFullFrameFreezeRep full_frame_freeze;
    OMX_TI_VIDEO_H264VDEC_SeiFullFrameFreezeRel full_frame_freeze_release;
    OMX_TI_VIDEO_H264VDEC_SeiProgRefineStart    prog_refine_start;
    OMX_TI_VIDEO_H264VDEC_SeiProgRefineEnd      prog_refine_end;
    OMX_TI_VIDEO_H264VDEC_SeiUserDataRegITUT    user_data_registered;
    OMX_TI_VIDEO_H264VDEC_SeiUserDataUnReg      user_data_unregistered;
    OMX_TI_VIDEO_H264VDEC_SeiBufferingPeriod    buffering_period_info;
    OMX_TI_VIDEO_H264VDEC_SeiPanScanRect        pan_scan_rect;
    OMX_TI_VIDEO_H264VDEC_SeiRecoveryPointInfo  recovery_pt_info;
    OMX_TI_VIDEO_H264VDEC_SeiPictureTiming      pic_timing;
    OMX_TI_VIDEO_H264VDEC_SeiStereoVideoInfo    stereo_video_info;
    OMX_TI_VIDEO_H264VDEC_SeiFramePacking       frame_packing;
} OMX_TI_VIDEO_H264VDEC_SeiMessages;


/**
 ******************************************************************************
 *  @struct _sErrConcealStr
 *  @brief  This str holds up the required Info for implementing the SCV EC,
 *          this will get updated by H.264 decoder while decoding the  SVC
 *          Base/Target Layers
 *
 *  @param  CurrMbInfoBufPointer  :Base Address of the current decoded frame
 *                                   MB Info buffer
 *
 *  @param  CurrMbStatusBufPointer: Base Address of the current decoded frame
 *                                   MB staus buffer pointer
 *
 *  @param  currFrameY            : Base Address of the current decoded Luma
 *                                  frame buffer pointer (physical pointer)
 *
 *  @param  currFrameUV           : Base Address of the current decoded Chroma
 *                                  frame buffer pointer (physical pointer)
 *
 *  @param  refConclY             : Base Address of the ref decoded Luma
 *                                  frame buffer pointer (virtual pointer)
 *
 *  @param  refConclUV            : Base Address of the ref decoded Chroma
 *                                  frame buffer pointer (virtual pointer)
 *
 *  @param  TilerBaseAddress      : TBA vaule for the VDMA
 *
 *  @param  pSliceInfoFlags       : Flag to enable slice info
 *
 *  @param  ref_width             : Resultant Horizontal LUMA picture size
 *                                  after Pad size addition on both Left
 *                                  & Right sides. This gets used as
 *                                  stride during vDMA programming.
 *                                  In case of TILER,the stride is fixed,
 *                                  independant of Picture width, and
 *                                  only changes with TILER mode.
 *
 *  @param  ref_width_c           : Resultant Horizontal CHROMA picture size
 *                                  after Pad size addition on both Left &
 *                                  Right sides.
 *
 *
 *  @param  ref_frame_height      : In case of Interlaced streams,the picure
 *                                  store is different i.e., store each field
 *                                  by applying PAD on top & bottom lines.
 *                                  Hence the picture height will be Height
 *                                  plus four times the Pad size. This
 *                                  variable holds this resultant value.
 *
 *  @param  mb_width              : Picture width in terms of Macroblocks
 *
 *  @param  mb_height             : Picture height in terms of Macroblocks.
 *
 *  @param  image_width           : Image width of the decoded frame
 *
 *  @param  image_width           : Image height of the decoded frame
 *
 *  @param  frameType             : Frame type of the current frame.
 *
 *  @param  picaff_frame          : Flag to indicate whether current picture
 *                                  is of Frame type & referring to Field
 *                                  picture as reference.
 *
 *  @param  mb_aff_frame_flag     : Flag to indicate whether the current
 *                                  decoding picture is MBAFF type.
 *
 *  @param  field_pic_flag        : Flag to indicate whether the current
 *                                  decoding picture is field type.
 *
 *  @param  bottom_field_flag     : This parameter equal to 1 specifies that
 *                                  the slice is part of a coded bottom field.
 *                                  bottom_field_flag equalto 0 specifies
 *                                  that the picture is a coded top field.
 *
 *  @param  nonPairedFieldPic     : Flag to indicate Non paired field picture.
 *
 *  @param  prev_pic_bottom_field : this variable Indicates if the previous
 *                                  picture was a bottom field or not (a Flag)
 ******************************************************************************
*/

typedef struct OMX_TI_VIDEO_H264VDEC_ErrConcealStr {
    OMX_S32 ErrConcealmentEnable;
    OMX_S32 CurrMbInfoBufPointer;
    OMX_S32 CurrMbStatusBufPointer;
    OMX_S32 CurrMbInfoIresBufPointer;
    OMX_S32 currFrameY;
    OMX_S32 currFrameUV;
    OMX_S32 refConclY;
    OMX_S32 refConclUV;
    OMX_U32 TilerBaseAddress;
    OMX_U16 ref_width;
    OMX_U16 ref_width_c;
    OMX_U16 ref_frame_height;
    OMX_U16 mb_width;
    OMX_U16 mb_height;
    OMX_U16 image_width;
    OMX_U16 image_height;
    OMX_U8  frameType;
    OMX_U8  picaff_frame;
    OMX_U8  mb_aff_frame_flag;
    OMX_U8  field_pic_flag;
    OMX_U8  bottom_field_flag;
    OMX_U8  nonPairedFieldPic;
    OMX_U8  prev_pic_bottom_field;
}OMX_TI_VIDEO_H264VDEC_ErrConcealStr;

/**
 *  Size of sliceinfo flags - We have two slice info flag arrays in SL2, one
 *  for ECD3 and the other for MC3. ECD3 flag is one bit per MB. Since Maximum
 *  supported number of MBs in a frame is 128 x 128 = 16384, we need 16384/8 =
 *  2048 bytes for the slice info flag array for ECD3. But for the MC3 array,
 *  we always make the next bit also as 1 to enable loading into ping and pong
 *  memories of MCBUF. So we need an extra bit for the MC3 array, to avoid
 *  buffer overflow when the last MB is a new slice. To keep the next SL2 buffer
 *  in 16-byte aligned position (some buffers need it) we round the size to next
 *  multiple of 16, i.e., 2064.
*/
#define OMX_TI_VIDEO_SLICEINFO_FLAGSIZE  2064

/**
 ******************************************************************************
 *  @struct _sErrConcealLayerStr
 *  @brief  This str holds up the required Info for implementing the SCV EC,
 *          this will get updated by H.264 decoder while decoding the  SVC
 *          Base/Target Layers
 *
 *  @param  svcEcStr              : structure instance of sSVCErrConcealStr
 *
 *  @param  pSliceInfoFlags       : Array to store the sliceInfo flag
 *
 *
 ******************************************************************************
*/
typedef struct OMX_TI_VIDEO_H264VDEC_ErrConcealLayerStr {
    OMX_TI_VIDEO_H264VDEC_ErrConcealStr sECStr;
    OMX_U8                              pSliceInfoFlags[OMX_TI_VIDEO_SLICEINFO_FLAGSIZE];
}OMX_TI_VIDEO_H264VDEC_ErrConcealLayerStr;


/**
 ******************************************************************************
 *  @struct OMX_TI_VIDEO_H264VDEC_CommonInfo
 *
 *  @brief
 *
 ******************************************************************************
*/
typedef struct OMX_TI_VIDEO_H264VDEC_CommonInfo {
    OMX_U32 codec_type : 8;
    OMX_U32 fmt_type : 8;
    OMX_U32 mb_ll_avail : 1;
    OMX_U32 mb_ul_avail : 1;
    OMX_U32 mb_uu_avail : 1;
    OMX_U32 mb_ur_avail : 1;
    OMX_U32 pic_bound_l : 1;
    OMX_U32 pic_bound_u : 1;
    OMX_U32 pic_bound_r : 1;
    OMX_U32 pic_bound_b : 1;
    OMX_U32 first_mb_flag : 1;
    OMX_U32 error_flag : 1;
    OMX_U32 zero : 6;
    OMX_U32 zeroes : 16;
    OMX_U32 mb_addr : 16;

} OMX_TI_VIDEO_H264VDEC_CommonInfo;

/**
 ******************************************************************************
 *  @struct OMX_TI_VIDEO_H264VDEC_MotionVector
 *
 *  @brief
 *
 ******************************************************************************
*/
typedef struct OMX_TI_VIDEO_H264VDEC_MotionVector {
    OMX_S16 x;
    OMX_S16 y;
} OMX_TI_VIDEO_H264VDEC_MotionVector;

/**
 ******************************************************************************
 *  @struct OMX_TI_VIDEO_H264VDEC_CabacContext
 *
 *  @brief
 *
 ******************************************************************************
*/
typedef struct OMX_TI_VIDEO_H264VDEC_CabacContext {
    OMX_TI_VIDEO_H264VDEC_MotionVector mvd_l0[4];
    OMX_TI_VIDEO_H264VDEC_MotionVector mvd_l1[4];

} OMX_TI_VIDEO_H264VDEC_CabacContext;

/**
 ******************************************************************************
 *  @struct OMX_TI_VIDEO_H264VDEC_TotalCoefLuma
 *
 *  @brief
 *
 ******************************************************************************
*/
typedef struct OMX_TI_VIDEO_H264VDEC_TotalCoefLuma {
    OMX_U8 right[3];
    OMX_U8 bottom_right;
    OMX_U8 bottom[3];
    OMX_U8 zero;
} OMX_TI_VIDEO_H264VDEC_TotalCoefLuma;

/**
 ******************************************************************************
 *  @struct OMX_TI_VIDEO_H264VDEC_TotalCoefChroma
 *
 *  @brief
 *
 ******************************************************************************
*/
typedef struct OMX_TI_VIDEO_H264VDEC_TotalCoefChroma {
    OMX_U8 right_cb;
    OMX_U8 bottom_right_cb;
    OMX_U8 bottom_cb;
    OMX_U8 zero;
    OMX_U8 right_cr;
    OMX_U8 bottom_right_cr;
    OMX_U8 bottom_cr;
    OMX_U8 zero1;
} OMX_TI_VIDEO_H264VDEC_TotalCoefChroma;

/**
 ******************************************************************************
 *  @struct OMX_TI_VIDEO_H264VDEC_CavlcContext
 *
 *  @brief
 *
 ******************************************************************************
*/
typedef struct OMX_TI_VIDEO_H264VDEC_CavlcContext {
    unsigned long long                    zeroes[2];
    OMX_TI_VIDEO_H264VDEC_TotalCoefLuma   total_coef_luma;
    OMX_TI_VIDEO_H264VDEC_TotalCoefChroma total_coef_chroma;

} OMX_TI_VIDEO_H264VDEC_CavlcContext;

/**
 ******************************************************************************
 *  @struct OMX_TI_VIDEO_H264VDEC_IntraPredMode
 *
 *  @brief
 *
 ******************************************************************************
*/
typedef struct OMX_TI_VIDEO_H264VDEC_IntraPredMode {
    OMX_U32 ipred_mode0 : 4;
    OMX_U32 ipred_mode1 : 4;
    OMX_U32 ipred_mode2 : 4;
    OMX_U32 ipred_mode3 : 4;
    OMX_U32 ipred_mode4 : 4;
    OMX_U32 ipred_mode5 : 4;
    OMX_U32 ipred_mode6 : 4;
    OMX_U32 ipred_mode7 : 4;
    OMX_U32 ipred_mode8 : 4;
    OMX_U32 ipred_mode9 : 4;
    OMX_U32 ipred_mode10 : 4;
    OMX_U32 ipred_mode11 : 4;
    OMX_U32 ipred_mode12 : 4;
    OMX_U32 ipred_mode13 : 4;
    OMX_U32 ipred_mode14 : 4;
    OMX_U32 ipred_mode15 : 4;

} OMX_TI_VIDEO_H264VDEC_IntraPredMode;


/**
 ******************************************************************************
 *  @struct OMX_TI_VIDEO_H264VDEC_MbPredType
 *
 *  @brief
 *
 ******************************************************************************
*/
typedef struct OMX_TI_VIDEO_H264VDEC_MbPredType {
    OMX_U32 mbskip : 1;
    OMX_U32 tr8x8 : 1;
    OMX_U32 mb_field : 1;
    OMX_U32 cond_mbskip : 1;
    OMX_U32 c_ipred_mode : 2;
    OMX_U32 zero : 1;
    OMX_U32 end_of_slice : 1;
    OMX_U32 mb_y_mod2 : 1;
    OMX_U32 zero1 : 7;
    OMX_U32 refidx_equal_flag_l0 : 1;
    OMX_U32 refidx_equal_flag_l1 : 1;
    OMX_U32 mv_equal_flag_l0 : 1;
    OMX_U32 mv_equal_flag_l1 : 1;
    OMX_U32 zeroes : 4;
    OMX_U32 mb_type : 8;
    OMX_U8  sub_mb_type[4];

} OMX_TI_VIDEO_H264VDEC_MbPredType;

/**
 ******************************************************************************
 *  @struct OMX_TI_VIDEO_H264VDEC_QpCbp
 *
 *  @brief
 *
 ******************************************************************************
*/
typedef struct OMX_TI_VIDEO_H264VDEC_QpCbp {
    OMX_U32 cbp;
    OMX_U8  qp_y;
    OMX_U8  qp_cb;
    OMX_U8  qp_cr;
    OMX_U8  zero;
} OMX_TI_VIDEO_H264VDEC_QpCbp;

/**
 ******************************************************************************
 *  @struct OMX_TI_VIDEO_H264VDEC_RefPicControl
 *
 *  @brief
 *
 ******************************************************************************
*/
typedef struct OMX_TI_VIDEO_H264VDEC_RefPicControl {
    OMX_U8 refidx[4];
    OMX_U8 refpicid[4];

} OMX_TI_VIDEO_H264VDEC_RefPicControl;

/**
 ******************************************************************************
 *  @struct OMX_TI_VIDEO_H264VDEC_MvBidirectional16
 *
 *  @brief
 *
 ******************************************************************************
*/
typedef struct OMX_TI_VIDEO_H264VDEC_MvBidirectional16 {
    OMX_TI_VIDEO_H264VDEC_MotionVector mv_forward[16];
    OMX_TI_VIDEO_H264VDEC_MotionVector mv_backward[16];
} OMX_TI_VIDEO_H264VDEC_MvBidirectional16;


/**
 ******************************************************************************
 *  @struct OMX_TI_VIDEO_H264VDEC_MvBidirectional4
 *
 *  @brief
 *
 ******************************************************************************
*/
typedef struct OMX_TI_VIDEO_H264VDEC_MvBidirectional4 {
    OMX_TI_VIDEO_H264VDEC_MotionVector mv_forward[4];
    OMX_TI_VIDEO_H264VDEC_MotionVector mv_backward[4];

} OMX_TI_VIDEO_H264VDEC_MvBidirectional4;

/**
 ******************************************************************************
 *  @struct OMX_TI_VIDEO_H264VDEC_MbInfo
 *
 *  @brief  This structure details the data format for MB information shared to
 *          application. This helps application to understand all fields
 *          the way codec uses MB info internally. This structure is of size
 *          208 Bytes.
 *
 *  @param  info : This elements gives details about the MB placement in the
 *                 frame.
 *
 *  @param  cabac: This field holds the context data for a CABAC coded MB
 *
 *  @param  cavlc: This field holds the context data for a CAVLC coded MB
 *
 *  @param  ipred_mode: This field holds information of intra prediction modes
 *                      at 4x4 level, for intra coded MB.
 *
 *  @param  mb_pred_type: This indicates prediction specific details for inter
 *                        coded MB
 *
 *  @param  qp_cbp: This gives coded & QP informations for both LUMA & CHROMA
 *                   components of a Macro Block.
 *
 *  @param  l0_ref_pic_control: Informs all details about reference indices
 *                              at 8x8 block level in L0 direction
 *
 *  @param  l1_ref_pic_control: Informs all details about reference indices
 *                              at 8x8 block level in L1 direction
 *
 *  @param  mv_forward: Lists all Motion vectors at 4x4 level in L0 direction
 *
 *  @param  bidirectional16: Lists all Motion vectors at 4x4 level in both
 *                           directions
 *
 *  @param  bidirectional4: Lists all Motion vectors at 8x8 level in both
 *                          directions
 *
 ******************************************************************************
*/
typedef struct OMX_TI_VIDEO_H264VDEC_MbInfo {
    OMX_TI_VIDEO_H264VDEC_CommonInfo info;

    union {
        OMX_TI_VIDEO_H264VDEC_CabacContext cabac;
        OMX_TI_VIDEO_H264VDEC_CavlcContext cavlc;
    } OMX_TI_VIDEO_H264VDEC_context;

    OMX_TI_VIDEO_H264VDEC_IntraPredMode ipred_mode;
    OMX_TI_VIDEO_H264VDEC_MbPredType    mb_pred_type;
    OMX_TI_VIDEO_H264VDEC_QpCbp         qp_cbp;
    OMX_TI_VIDEO_H264VDEC_RefPicControl l0_ref_pic_control;
    OMX_TI_VIDEO_H264VDEC_RefPicControl l1_ref_pic_control;

    union {
        OMX_TI_VIDEO_H264VDEC_MotionVector      mv_forward[16];
        OMX_TI_VIDEO_H264VDEC_MvBidirectional16 bidirectional16;
        OMX_TI_VIDEO_H264VDEC_MvBidirectional4  bidirectional4;
    } OMX_TI_VIDEO_H264VDEC_motion_vecs;

} OMX_TI_VIDEO_H264VDEC_MbInfo;



/**
********************************************************************************
*  @struct  OMX_TI_VIDEO_VC1VDEC_MbInfo
*
*  @brief   MB information structure that is written out by the IVA-HD hardware.
*
*  @note    None:
*
********************************************************************************
*/
typedef struct OMX_TI_VIDEO_VC1VDEC_MbInfo {
    /* MB address                                                               */
    OMX_U8 mb_addr;
    /* Error flag                                                               */
    OMX_U8 error_flag;
    /* First MB flag                                                            */
    OMX_U8 first_mb_flag;
    /* Picture bound                                                            */
    OMX_U8 pic_bound_b;
    /* Upper picture bound                                                      */
    OMX_U8 pic_bound_u;
    /* Right picture bound                                                      */
    OMX_U8 pic_bound_r;
    /* Left picture bound                                                       */
    OMX_U8 pic_bound_l;
    /* Availability of upper right MB                                           */
    OMX_U8 mb_ur_avail;
    /* Availability of upper MB                                                 */
    OMX_U8 mb_uu_avail;
    /* Availability of upper left MB                                            */
    OMX_U8 mb_ul_avail;
    /* Availability of left MB                                                  */
    OMX_U8 mb_ll_avail;
    /* Macroblock header format type                                            */
    OMX_U8 fmt_type;
    /* Codec type                                                               */
    OMX_U8 codec_type;
    /* Indicates DC values of each Y block in current MB                        */
    OMX_U8 dc_coef_q_y[4];
    /* Indicates DC values of Cr block in current MB                            */
    OMX_U8 dc_coef_q_cr;
    /* Indicates DC values of Cb block in current MB                            */
    OMX_U8 dc_coef_q_cb;
    /* Block type of cr block                                                   */
    OMX_U8 block_type_cr;
    /* Block type of cb block                                                   */
    OMX_U8 block_type_cb;
    /* Block types of luma                                                      */
    OMX_U8 block_type_y[4];
    /* In decoding, if the current macroblock is the last macroblock in a slice,*/
    /* ECD sets 1 to this field during executing the macroblock. Otherwise, ECD */
    /* sets 0 to this field                                                     */
    OMX_U8 end_of_slice;
    /* 1 : allow skipping current MB if CBP = 0                                 */
    OMX_U8 cond_skip_flag;
    /*  Skipped / non skipped MB                                                */
    OMX_U8 skip;
    /* 1 indicates that overlap filtering is in use for the macroblock.         */
    OMX_U8 overlap;
    /* 1 indicates that AC prediction is in use for the macroblock              */
    OMX_U8 acpred;
    /* Denotes inter-prediction direction for the macroblock in B-picture       */
    OMX_U8 b_picture_direction;
    /* Denotes the number of motion vectors.                                    */
    OMX_U8 mv_mode;
    /* 1 indicates that the field transform is in use for the macroblock.       */
    OMX_U8 fieldtx;
    /* 1 indicates that field inter-prediction is in use                        */
    OMX_U8 mv_type;
    /* Equals the reference frame distance                                      */
    OMX_U8 refdist;
    /* 1 indicates that macroblock quantizer-scale (MQUANT) overflows           */
    OMX_U8 mquant_overflow;
    /* Equals the quantizer-scale for the macroblock                            */
    OMX_U8 quant;
    /* 1 indicates that 0.5 shall be added to PQUANT in calculation of          */
    /* quantizer-scale. This field is valid for decoding only.                  */
    OMX_U8 halfqp;
    /* Equals the DC coefficient step size which is derived from MQUANT in the  */
    /*   bit-stream                                                             */
    OMX_U8 dc_step_size;
    /* Denotes the coded sub-block pattern for cr block                         */
    OMX_U8 cbp_cr;
    /* Denotes the coded sub-block pattern for cb block                         */
    OMX_U8 cbp_cb;
    /* Denotes the coded sub-block pattern for luma blocks                      */
    OMX_U8 cbp_y[3];
    /* Denotes the backward reference field picture                             */
    OMX_U8 mv_bw_ref_y[4];
    /* Denotes the forward reference field picture                              */
    OMX_U8 mv_fw_ref_y[3];
    /* Unclipped forward motion vector for luma                                 */
    OMX_U8 mv_fw_y[4][4];
    /* Unclipped backward motion vector for luma                                */
    OMX_U8 mv_bw_y[1][1];
    /* Unclipped backward motion vector for chroma                              */
    OMX_U8 mv_bw_c[2];
    /* Unclipped forward motion vector for chroma                               */
    OMX_U8 mv_fw_c[2];
    /* Clipped forward motion vector for luma                                   */
    OMX_U8 cmv_fw_y[4][4];
    /* Clipped backward motion vector for luma                                  */
    OMX_U8 cmv_bw_y[4][4];
    /* Clipped forward motion vector for chroma                                 */
    OMX_U8 cmv_fw_c[4][4];
    /* Clipped backward motion vector for chroma                                */
    OMX_U8 cmv_bw_c[4][4];

}OMX_TI_VIDEO_VC1VDEC_MbInfo;

#endif /* OMX_TI_VIDEO_H */

