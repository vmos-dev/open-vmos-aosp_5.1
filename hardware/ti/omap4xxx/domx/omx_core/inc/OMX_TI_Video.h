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
 *! 24-Dec-2008  Navneet 	navneet@ti.com  	Initial Version
 *! 14-Jul-2009	Radha Purnima  radhapurnima@ti.com
 *! 25-Aug-2009	Radha Purnima  radhapurnima@ti.com
 * =========================================================================*/


#ifndef OMX_TI_VIDEO_H
#define OMX_TI_VIDEO_H
#define H264ENC_MAXNUMSLCGPS 2

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
	OMX_TI_VIDEO_CodingSORENSONSPK   /* Sorenson Spark */
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

#endif /* OMX_TI_VIDEO_H */

