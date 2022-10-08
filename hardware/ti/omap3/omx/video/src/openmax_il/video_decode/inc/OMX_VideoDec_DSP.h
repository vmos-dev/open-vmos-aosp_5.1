
/*
 * Copyright (C) Texas Instruments - http://www.ti.com/
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
/* =============================================================================
*             Texas Instruments OMAP(TM) Platform Software
*  (c) Copyright Texas Instruments, Incorporated.  All Rights Reserved.
*
*  Use of this software is controlled by the terms and conditions found
*  in the license agreement under which this software has been supplied.
* =========================================================================== */
/**
* @file OMX_VIDDEC_DSP.h
*
* This is a header file for a TI OMX video component that is fully 
* compliant with the OMX Video specification.
* This the file that the application that uses OMX would include 
* in its code.
*
* @path $(CSLPATH)\
* 
* @rev 0.1
*/
/* -------------------------------------------------------------------------- */

#ifndef OMX_VIDDEC_DSP__H
#define OMX_VIDDEC_DSP__H

#define OMX_H264DEC_NUM_DLLS 5
#define OMX_MP4DEC_NUM_DLLS  5
#define OMX_MP2DEC_NUM_DLLS  4 
#define OMX_WMVDEC_NUM_DLLS  5
 #ifdef VIDDEC_SPARK_CODE
    #define OMX_SPARKDEC_NUM_DLLS  5
 #endif

#ifndef VIDDEC_SN_R8_14
    #define VIDDEC_SN_R8_14
#endif

#ifndef UNDER_CE
    #define H264_DEC_NODE_DLL  "h264vdec_sn.dll64P"
    #define MP4_DEC_NODE_DLL   "mp4vdec_sn.dll64P"
    #define MP4720P_DEC_NODE_DLL   "mpeg4aridec_sn.dll64P"
    #define MP2_DEC_NODE_DLL   "mp2vdec_sn.dll64P"
    #define WMV_DEC_NODE_DLL   "wmv9dec_sn.dll64P"
    #define RINGIO_NODE_DLL    "ringio.dll64P"
    #define USN_DLL            "usn.dll64P"
    #define CONVERSIONS_DLL    "conversions.dll64P"
 #ifdef VIDDEC_SPARK_CODE
    #define SPARK_DEC_NODE_DLL  "sparkdec_sn.dll64P"
 #endif
#else
    #define H264_DEC_NODE_DLL "\\windows\\h264vdec_sn.dll64P"
    #define MP4_DEC_NODE_DLL  "\\windows\\mp4vdec_sn.dll64P"
    #define MP2_DEC_NODE_DLL  "\\windows\\mp2vdec_sn.dll64P"
    #define WMV_DEC_NODE_DLL  "\\windows\\wmv9dec_sn.dll64P"
    #define RINGIO_NODE_DLL   "\\windows\\ringio.dll64P"
    #define USN_DLL           "\\windows\\usn.dll64P"
    #define CONVERSIONS_DLL   "\\windows\\conversions.dll64P"
 #ifdef VIDDEC_SPARK_CODE
    #define SPARK_DEC_NODE_DLL "\\windows\\sparkdec_sn.dll64P" 
 #endif
#endif

#define CEILING_1000X(x) ((OMX_U32)(x) + (1000-(OMX_U32)(x)%1000))

#define STRING_UUID_LENGHT 37
/* DIVX_1.0_ARICENT_uuid */
#define STRING_MP4D720PSOCKET_TI_UUID "E7FDD4D8_4F0B_4325_A430_E5729975F54A"
static const struct DSP_UUID MP4D720PSOCKET_TI_UUID = {
    0xe7fdd4d8, 0x4f0b, 0x4325, 0xa4, 0x30, {
    0xe5, 0x72, 0x99, 0x75, 0xf5, 0x4a
    }
};
/* MP4VDSOCKET_TI_UUID = 7E4B8541_47A1_11D6_B156_00B0D017674B */
#define STRING_MP4DSOCKET_TI_UUID "7E4B8541_47A1_11D6_B156_00B0D017674B"
static const struct DSP_UUID MP4DSOCKET_TI_UUID = {
    0x7e4b8541, 0x47a1, 0x11d6, 0xb1, 0x56, {
    0x00, 0xb0, 0xd0, 0x17, 0x67, 0x4b
    }
};
/* MP2VDSOCKET_TI_UUID = 7E4B8541_47A1_11D6_B156_00B0D0176740 */
#define STRING_MP2DSOCKET_TI_UUID "7E4B8541_47A1_11D6_B156_00B0D0176740"
static const struct DSP_UUID MP2DSOCKET_TI_UUID = {
	0x7e4b8541, 0x47a1, 0x11d6, 0xb1, 0x56, {
	0x00, 0xb0, 0xd0, 0x17, 0x67, 0x40
    }
};
/* H264VDSOCKET_TI_UUID = CB1E9F0F_9D5A_4434_8449_1FED2F992DF7 */
#define STRING_H264VDSOCKET_TI_UUID "CB1E9F0F_9D5A_4434_8449_1FED2F992DF7"
static const struct DSP_UUID H264VDSOCKET_TI_UUID = {
    0xCB1E9F0F, 0x9D5A, 0x4434, 0x84, 0x49, {
    0x1F, 0xED, 0x2F, 0x99, 0x2D, 0xF7
    }
};
/* WMV9SOCKET_TI_UUID = 609DAB97_3DFC_471F_8AB9_4E56E834501B */
#define STRING_WMVDSOCKET_TI_UUID "609DAB97_3DFC_471F_8AB9_4E56E834501B"
static const struct DSP_UUID WMVDSOCKET_TI_UUID = {
    0x609DAB97, 0x3DFC, 0x471F, 0x8A, 0xB9, {
    0x4E, 0x56, 0xE8, 0x34, 0x50, 0x1B
    }
};

static const struct DSP_UUID USN_UUID = {
    0x79A3C8B3, 0x95F2, 0x403F, 0x9A, 0x4B, {
    0xCF, 0x80, 0x57, 0x73, 0x05, 0x41
    }
};

static const struct DSP_UUID RINGIO_TI_UUID = {
    0x47698bfb, 0xa7ee, 0x417e, 0xa6, 0x7a, {
    0x41, 0xc0, 0x27, 0x9e, 0xb8, 0x05
    }
};

static const struct DSP_UUID CONVERSIONS_UUID = {
            0x722DD0DA, 0xF532, 0x4238, 0xB8, 0x46, {
                        0xAB, 0xFF, 0x5D, 0xA4, 0xBA, 0x02
            }
};

#ifdef VIDDEC_SPARK_CODE 
    #define STRING_SPARKDSOCKET_TI_UUID "DD8AC7F0_33BF_446B_938E_FDF00B467ED6"
    static const struct DSP_UUID SPARKDSOCKET_TI_UUID = {
        0xdd8ac7f0, 0x33bf, 0x446b, 0x93, 0x8e, {
        0xfd, 0xf0, 0x0b, 0x46, 0x7e, 0xd6
        }
    };
#endif

typedef struct WMV9DEC_SNCreatePhArg {
    OMX_U16 unNumOfStreams;
    OMX_U16 unInputStreamID;
    OMX_U16 unInputBufferType;  
    OMX_U16 unInputNumBufsPerStream;

    OMX_U16 unOutputStreamID;
    OMX_U16 unOutputBufferType;
    OMX_U16 unOutputNumBufsPerStream;  
    OMX_U16 unReserved;

    OMX_U32 ulMaxWidth;
    OMX_U32 ulMaxHeight;
    OMX_U32 ulYUVFormat;
    OMX_U32 ulMaxFrameRate;
    OMX_U32 ulMaxBitRate;
    OMX_U32 ulDataEndianness;
    OMX_S32 ulProfile;
    OMX_S32 ulMaxLevel;
    OMX_U32 ulProcessMode;
    OMX_S32 lPreRollBufConfig;
    OMX_U32 usIsElementaryStream;
    OMX_U32 bCopiedCCDBuffer;
    OMX_U16 endArgs;
} WMV9DEC_SNCreatePhArg;

typedef struct {
    OMX_S32 lBuffCount;
} WMV9DEC_UALGInputParam;

typedef struct {
    OMX_U32 ulDisplayID;
    OMX_U32 ulBytesConsumed;
    OMX_S32 iErrorCode;
    OMX_U32 ulDecodedFrameType;
} WMV9DEC_UALGOutputParam;

/* Mpeg4 Decoder Structures */
typedef enum
{
    /* 0 */           
    DECSTAT_Success,
    /* 0x80000001 */  DECSTAT_Failure = -0x7FFFFFFF,
    /* 0x80000002 */  DECSTAT_nonMpegStream,
    /* 0x80000003 */  DECSTAT_nonVideoMpegStream,
    /* 0x80000004 */  DECSTAT_unSupportedProfile,
    /* 0x80000005 */  DECSTAT_invalidBitstreamAddress,
    /* 0x80000006 */  DECSTAT_invalidVideoObjectLayerStartCode,
    /* 0x80000007 */  DECSTAT_nullBitstreamAddress,
    /* 0x80000008 */  DECSTAT_insufficientData,
    /* 0x80000009 */  DECSTAT_unsupportedVOL_verid,
    /* 0x8000000A */  DECSTAT_invalidAspectRatio,
    /* 0x8000000B */  DECSTAT_invalidChromaFormat,
    /* 0x8000000C */  DECSTAT_unsupportedVOLShape,
    /* 0x8000000D */  DECSTAT_invalidVOPTimeIncrementResolution,
    /* 0x8000000E */  DECSTAT_unsupportedFeatureInterlaced,
    /* 0x8000000F */  DECSTAT_unsupportedFeatureOBMC,
    /* 0x80000010 */  DECSTAT_unsupportedVideoDataPrecision, 
    /* 0x80000011 */  DECSTAT_unsupportedObjectType,
    /* 0x80000012 */  DECSTAT_unsupportedFirstQuantMethod,
    /* 0x80000013 */  DECSTAT_unsupportedFeatureScalability,
    /* 0x80000014 */  DECSTAT_invalidCallingOrder,
    /* 0x80000015 */  DECSTAT_invalidVideoObjectSC,
    /* 0x80000016 */  DECSTAT_invalidVOPSC,
    /* 0x80000017 */  DECSTAT_invalidQuant,
    /* 0x80000018 */  DECSTAT_invalidFcode,
    /* 0x80000019 */  DECSTAT_invalidMBnumInVPH,
    /* 0x8000001A */  DECSTAT_endOfSequence,
    /* 0x8000001B */  DECSTAT_invalidGOBnum,
    /* 0x8000001C */  DECSTAT_corruptedHeader,          
    /* 0x8000001D */  DECSTAT_corruptedBitStream,       
    /* 0x8000001E */  DECSTAT_unsupportedFeatureBFrames,
    /* 0x8000001F */  DECSTAT_unsupportedFeatureSprite,
    /* 0x80000020 */  DECSTAT_duringInitialization,
    /* 0x80000021 */  DECSTAT_initSuccess,
    /* 0x80000022 */  DECSTAT_noError,
    /* 0x80000023 */  DECSTAT_unsupportedFeatureQuarterPel,
    /* 0x80000024 */  DECSTAT_exceededResolution
} DEC_STATUS;

typedef struct MP2VDEC_SNCreatePhArg
{
    OMX_U16     unNumOfStreams;
    OMX_U16     unInputStreamID;
    OMX_U16     unInputBufferType;
    OMX_U16     unInputNumBufsPerStream;
    OMX_U16     unOutputStreamID;
    OMX_U16     unOutputBufferType;
    OMX_U16     unOutputNumBufsPerStream;
    OMX_U16     unReserved;
    
    OMX_U32     ulMaxWidth; 
    OMX_U32     ulMaxHeight;
    OMX_U32     ulYUVFormat;
    OMX_U32     ulMaxFrameRate;
    OMX_U32     ulMaxBitRate;
    OMX_U32     ulDataEndianness;
    OMX_U32     ulProfile;
    OMX_S32     lMaxLevel;
    OMX_U32     ulProcessMode;
    OMX_S32     lPreRollBufConfig;   
    OMX_U32 	ulDisplayWidth; 
    OMX_U16     endArgs;
} MP2VDEC_SNCreatePhArg;

typedef struct
{
    OMX_S32            lBuffCount;
      
} MP2VDEC_UALGInputParam;

typedef struct
{
    OMX_U32           ulDisplayID;
    OMX_U32           ulBytesConsumed;
    OMX_S32           lErrorCode;
    OMX_U32           ulDecodedFrameType;    
    OMX_U32           ulQP[(640 * 480) / 256];
} MP2VDEC_UALGOutputParam;

typedef struct MP4VD_GPP_SN_Obj_CreatePhase {
    OMX_U16 unNumOfStreams;
    OMX_U16 unInputStreamID;
    OMX_U16 unInputBufferType;
    OMX_U16 unlInputNumBufsPerStream;

    OMX_U16 unOutputStreamID;
    OMX_U16 unOutputBufferType;
    OMX_U16 unOutputNumBufsPerStream;
    OMX_U16 unReserved;

    OMX_U32 ulMaxWidth; 
    OMX_U32 ulMaxHeight;
    OMX_U32 ulYUVFormat;
    OMX_U32 ulMaxFrameRate;
    OMX_U32 ulMaxBitRate;
    OMX_U32 ulDataEndianness;
    OMX_U32 ulProfile;
    OMX_S32 ulMaxLevel; 
    OMX_U32 ulProcessMode;
    OMX_S32 ulPreRollBufConfig;
    OMX_U32 ulDisplayWidth;
    OMX_U16 endArgs;
} MP4VD_GPP_SN_Obj_CreatePhase;

typedef struct
{
    OMX_S32 nBuffCount;
    OMX_U32 uRingIOBlocksize;
    OMX_S32 nPerformMode;
} MP4VD_GPP_SN_UALGInputParams;

typedef struct
{
    OMX_U32 ulDisplayID;
    OMX_U32 uBytesConsumed;
    OMX_S32 iErrorCode;
    OMX_U32 ulDecodedFrameType;
    OMX_U32 ulQP[(720 * 576) / 256];
    OMX_S32 lMbErrorBufFlag;
    OMX_U8  usMbErrorBuf[(720 * 576) / 256];

} MP4VD_GPP_SN_UALGOutputParams;

typedef struct H264VDEC_SNCreatePhArg {
   OMX_U16 unNumOfStreams;
   OMX_U16 unInputStreamID;
   OMX_U16 unInputBufferType;  
   OMX_U16 unInputNumBufsPerStream;

   OMX_U16 unOutputStreamID;
   OMX_U16 unOutputBufferType;
   OMX_U16 unOutputNumBufsPerStream;  
   OMX_U16 unReserved;

   OMX_U32 ulMaxWidth;
   OMX_U32 ulMaxHeight;
   OMX_U32 ulYUVFormat;
   OMX_U32 ulMaxFrameRate;
   OMX_U32 ulMaxBitRate;
   OMX_U32 ulDataEndianness;
   OMX_U32 ulProfile;
   OMX_S32 ulMaxLevel;
   OMX_U32 ulProcessMode;
   OMX_S32 lPreRollBufConfig;
   OMX_U32 ulBitStreamFormat;
   OMX_U32 ulDisplayWidth;
   OMX_U16 endArgs;
} H264VDEC_SNCreatePhArg;

typedef struct {
    OMX_S32 lBuffCount;
#if 1
    OMX_U32 ulNumOfNALU;
    OMX_U32 pNALUSizeArray[H264VDEC_SN_MAX_NALUNITS];
/*     OMX_U32 *pNALUSizeArray;*/
#endif
} H264VDEC_UALGInputParam;

#define H264VDEC_SN_MAX_MB_NUMBER 1620

typedef struct {
    OMX_U32 ulDisplayID;
    OMX_U32 ulBytesConsumed;
    OMX_S32 iErrorCode;
    OMX_U32 ulDecodedFrameType;
#if 1
    OMX_U32 ulNumOfNALUDecoded;
    OMX_S32 lMBErrStatFlag;
    OMX_U8  pMBErrStatOutBuf[H264VDEC_SN_MAX_MB_NUMBER];
#endif
} H264VDEC_UALGOutputParam;
#ifdef VIDDEC_SPARK_CODE 
typedef struct SPARKVD_GPP_SN_Obj_CreatePhase {
    OMX_U16 unNumOfStreams;
    OMX_U16 unInputStreamID;
    OMX_U16 unInputBufferType;
    OMX_U16 unlInputNumBufsPerStream;

    OMX_U16 unOutputStreamID;
    OMX_U16 unOutputBufferType;
    OMX_U16 unOutputNumBufsPerStream;

    OMX_U32 ulMaxWidth; 
    OMX_U32 ulMaxHeight;
    OMX_U32 ulYUVFormat;
    OMX_U32 ulMaxFrameRate;
    OMX_U32 ulMaxBitRate;
    OMX_U32 ulDataEndianness;
    OMX_U32 ulProfile;
    OMX_S32 ulMaxLevel; 
    OMX_U32 ulProcessMode;
    OMX_S32 ulPreRollBufConfig;
    OMX_U16 endArgs;
} SPARKVD_GPP_SN_Obj_CreatePhase;

typedef struct SPARKVD_GPP_SN_UALGInputParams
{
    long int lBuffCount;
    long int nIsSparkInput;
} SPARKVD_GPP_SN_UALGInputParams;

typedef struct SPARKVD_GPP_SN_UALGOutputParams
{
    OMX_U32 ulDisplayID;
    OMX_U32 uBytesConsumed;
    OMX_S32 iErrorCode;
    OMX_U32 ulDecodedFrameType;
    OMX_U32 ulQP[(720 * 576) / 256];
} SPARKVD_GPP_SN_UALGOutputParams;
#endif

typedef enum {
    USN_DSPACK_STOP          = 0x0200,
    USN_DSPACK_PAUSE         = 0x0300,
    USN_DSPACK_ALGCTRL       = 0x0400,
    USN_DSPACK_STRMCTRL      = 0x0500,
    USN_DSPMSG_BUFF_FREE     = 0x0600,
    USN_DSPACK_SET_STRM_NODE = 0x0700,
    USN_DSPACK_GET_NODE_PTR  = 0x0800,
    USN_DSPMSG_EVENT         = 0x0E00
}USN_NodeToHostCmd;

typedef enum {
    USN_ERR_NONE,
    USN_ERR_WARNING,
    USN_ERR_PROCESS,
    USN_ERR_PAUSE,
    USN_ERR_STOP,
    USN_ERR_ALGCTRL,
    USN_ERR_STRMCTRL,
    USN_ERR_UNKNOWN_MSG
} USN_ErrTypes;

typedef enum {
    IUALG_OK                  = 0x0000,
    IUALG_WARN_CONCEALED      = 0x0100,
    IUALG_WARN_UNDERFLOW      = 0x0200,
    IUALG_WARN_OVERFLOW       = 0x0300,
    IUALG_WARN_ENDOFDATA      = 0x0400,
    IUALG_WARN_PLAYCOMPLETED  = 0x0500,
    IUALG_ERR_BAD_HANDLE      = 0x0F00,
    IUALG_ERR_DATA_CORRUPT    = 0x0F01,
    IUALG_ERR_NOT_SUPPORTED   = 0x0F02,
    IUALG_ERR_ARGUMENT        = 0x0F03,
    IUALG_ERR_NOT_READY       = 0x0F04,
    IUALG_ERR_GENERAL         = 0x0FFF
}IUALG_Event;

typedef enum {
    USN_STRMCMD_PLAY,
    USN_STRMCMD_PAUSE,
    USN_STRMCMD_STOP,
    USN_STRMCMD_SETCODECPARAMS,
    USN_STRMCMD_IDLE,
    USN_STRMCMD_FLUSH
}USN_StrmCmd;

/* 
 *  ======== VIDDEC_FrameType ========
 * XDM supported frame types for video 
 */
typedef enum {
    VIDDEC_I_FRAME =0, VIDDEC_P_FRAME, VIDDEC_B_FRAME, 
    VIDDEC_IDR_FRAME
} VIDDEC_FrameType;

#define OMX_VIDDEC_1_1
#ifdef OMX_VIDDEC_1_1
    #define OMX_BUFFERFLAG_SYNCFRAME 0x00000020
#endif
#define VIDDEC_BUFFERFLAG_FRAMETYPE_MASK                    0xF0000000
#define VIDDEC_BUFFERFLAG_FRAMETYPE_I_FRAME                 0x10000000
#define VIDDEC_BUFFERFLAG_FRAMETYPE_P_FRAME                 0x20000000
#define VIDDEC_BUFFERFLAG_FRAMETYPE_B_FRAME                 0x40000000
#define VIDDEC_BUFFERFLAG_FRAMETYPE_IDR_FRAME               0x80000000

#define VIDDEC_BUFFERFLAG_EXTENDERROR_MASK                  0x0FFFF000
#define VIDDEC_BUFFERFLAG_EXTENDERROR_DIRTY                 0x000FF000
#define VIDDEC_BUFFERFLAG_EXTENDERROR_APPLIEDCONCEALMENT    0x00200000
#define VIDDEC_BUFFERFLAG_EXTENDERROR_INSUFFICIENTDATA      0x00400000
#define VIDDEC_BUFFERFLAG_EXTENDERROR_CORRUPTEDDATA         0x00800000
#define VIDDEC_BUFFERFLAG_EXTENDERROR_CORRUPTEDHEADER       0x01000000
#define VIDDEC_BUFFERFLAG_EXTENDERROR_UNSUPPORTEDINPUT      0x02000000
#define VIDDEC_BUFFERFLAG_EXTENDERROR_UNSUPPORTEDPARAM      0x04000000
#define VIDDEC_BUFFERFLAG_EXTENDERROR_FATALERROR            0x08000000

/*
 *  Extended error enumeration for XDM compliant encoders and decoders:
 *
 *  Bit 16-32 : Reserved 
 *  Bit XDM_FATALERROR:       1 => Fatal error (Stop decoding) 
 *                            0 => Recoverable error 
 *  Bit XDM_UNSUPPORTEDPARAM: 1 => Unsupported input parameter or configuration 
 *                            0 => Ignore
 *  Bit XDM_UNSUPPORTEDINPUT: 1 => Unsupported feature/ parameter in input, 
 *                            0 => Ignore 
 *  Bit XDM_CORRUPTEDHEADER:  1 => Header problem/ corruption, 
 *                            0 => Ignore
 *  Bit XDM_CORRUPTEDDATA:    1 => Data problem /corruption, 
 *                            0 => Ignore
 *  Bit XDM_INSUFFICIENTDATA: 1 => Insufficient data, 
 *                            0 => Ignore
 *  Bit  XDM_APPLIEDCONCEALMENT:  1 => Applied concelement,  
 *                            0=> Ignore
 *  Bit  8 :    Reserved   
 *  Bit 7-0 :   Codec & Implementation specific
 *
 *  Notes :
 *  =====
 *  1) Algorithm will set multiple bits to 1 based on conditions. 
 *     e.g. it will set bits XDM_FATALERROR (fatal) and XDM_UNSUPPORTEDPARAM 
 *     (unsupported params) in case of unsupported run time parameters
 *  2) Some erors are applicable to decoders only.
 */
typedef enum {
    VIDDEC_XDM_APPLIEDCONCEALMENT=9, VIDDEC_XDM_INSUFFICIENTDATA=10, VIDDEC_XDM_CORRUPTEDDATA=11,
    VIDDEC_XDM_CORRUPTEDHEADER=12, VIDDEC_XDM_UNSUPPORTEDINPUT=13, VIDDEC_XDM_UNSUPPORTEDPARAM=14,
    VIDDEC_XDM_FATALERROR=15
} VIDDEC_XDM_ErrorBit;

#define VIDDEC_ISFLAGSET(x,y)         (((x)>>(y)) & 0x1) 
/*#define VIDDEC_ISFLAGSET(x,y)         ((((OMX_S32*)(x))>>((OMX_S32*)(y))) & 0x1) */

/** @enum M4H3DEC_TI_ERROR */
typedef enum {
  /* 00 */  M4H3DEC_TI_ERROR_Success = 0,
  /* 01 */  M4H3DEC_TI_ERROR_Failure ,
  /* 02 */  M4H3DEC_TI_ERROR_nonMpegStream,
  /* 03 */  M4H3DEC_TI_ERROR_nonVideoMpegStream,
  /* 04 */  M4H3DEC_TI_ERROR_unSupportedProfile,
  /* 05 */  M4H3DEC_TI_ERROR_invalidBitstreamAddress,
  /* 06 */  M4H3DEC_TI_ERROR_invalidVideoObjectLayerStartCode,
  /* 07 */  M4H3DEC_TI_ERROR_nullBitstreamAddress,
  /* 08 */  M4H3DEC_TI_ERROR_insufficientData,
  /* 09 */  M4H3DEC_TI_ERROR_unsupportedVOL_verid,
  /* 10 */  M4H3DEC_TI_ERROR_invalidAspectRatio,
  /* 11 */  M4H3DEC_TI_ERROR_invalidChromaFormat,
  /* 12 */  M4H3DEC_TI_ERROR_unsupportedVOLShape,
  /* 13 */  M4H3DEC_TI_ERROR_invalidVOPTimeIncrementResolution,
  /* 14 */  M4H3DEC_TI_ERROR_unsupportedFeatureInterlaced,
  /* 15 */  M4H3DEC_TI_ERROR_unsupportedFeatureOBMC,
  /* 16 */  M4H3DEC_TI_ERROR_unsupportedVideoDataPrecision, 
  /* 17 */  M4H3DEC_TI_ERROR_unsupportedObjectType,
  /* 18 */  M4H3DEC_TI_ERROR_unsupportedFirstQuantMethod,
  /* 19 */  M4H3DEC_TI_ERROR_unsupportedFeatureScalability,
  /* 20 */  M4H3DEC_TI_ERROR_invalidCallingOrder,
  /* 21 */  M4H3DEC_TI_ERROR_invalidVideoObjectSC,
  /* 22 */  M4H3DEC_TI_ERROR_invalidVOPSC,
  /* 23 */  M4H3DEC_TI_ERROR_invalidQuant,
  /* 24 */  M4H3DEC_TI_ERROR_invalidFcode,
  /* 25 */  M4H3DEC_TI_ERROR_invalidMBnumInVPH,
  /* 26 */  M4H3DEC_TI_ERROR_endOfSequence,
  /* 27 */  M4H3DEC_TI_ERROR_invalidGOBnum,
  /* 28 */  M4H3DEC_TI_ERROR_corruptedHeader,          
  /* 29 */  M4H3DEC_TI_ERROR_corruptedBitStream,       
  /* 30 */  M4H3DEC_TI_ERROR_unsupportedFeatureBFrames,
  /* 31 */  M4H3DEC_TI_ERROR_unsupportedFeatureSprite,
  /* 32 */  M4H3DEC_TI_ERROR_unsupportedFeatureQuarterPel,
  /* 33 */  M4H3DEC_TI_ERROR_exceededResolution,
  /* 34 */    M4H3DEC_TI_ERROR_unsupportedFeatureIntraDcVlcThreshold,
  /* 35 */  M4H3DEC_TI_ERROR_invalidValue,
  /* 36 */  M4H3DEC_TI_ERROR_stuffingInMB,
  /* 37 */  M4H3DEC_TI_ERROR_numMbRowsInVpExceeded,
  /* 38 */  M4H3DEC_TI_ERROR_cannotdecodempeg4,
  /* 39 */  M4H3DEC_TI_ERROR_incorrectWidthHeight,
  /* 40 */  M4H3DEC_TI_ERROR_insufficientMemory,
} VIDDEC_M4H3DEC_TI_ERROR;

/* ======================================================================= */
/* WMV9DEC_UALGDynamicParams - This structure defines the run-time algorithm
 * specific and UALG parameters which can be configured.
 *
 * @param ulDecodeHeader    :Parse header.
 *
 * @param ulDisplayWidth    :Pitch value. Used as pitch only if this value 
 * is greater than Video width.
 *
 * @param ulFrameSkipMode   :Frame skip mode.
 *          
 * @param ulPPType          :Post-processing type required.
 *
 */
/* ==================================================================== */
typedef struct WMV9DEC_UALGDynamicParams
{
#ifdef VIDDEC_SN_R8_14
    OMX_S32 size;
#endif
    OMX_U32 ulDecodeHeader;
    OMX_U32 ulDisplayWidth;
    OMX_U32 ulFrameSkipMode;
    OMX_U32 ulPPType;
    OMX_U16 usIsElementaryStream;
    
}WMV9DEC_UALGDynamicParams;

typedef struct H264_Iualg_Cmd_SetStatus
{
#ifdef VIDDEC_SN_R8_14
    OMX_S32 size;
#endif
    OMX_U32 ulDecodeHeader; 
    OMX_U32 ulDisplayWidth;  
    OMX_U32 ulFrameSkipMode; 
    OMX_U32 ulPPType;        
    
/*#if defined(WMV9PGIN)    */
 /*   OMX_U16 usIsElementaryStream; */
/*#endif */   
} H264_Iualg_Cmd_SetStatus;

typedef struct MP4VDEC_UALGDynamicParams
{
#ifdef VIDDEC_SN_R8_14
  OMX_S32           size;
#endif
  OMX_U32         ulDecodeHeader;    
  OMX_U32         ulDisplayWidth;  
  OMX_U32         ulFrameSkipMode; 
  OMX_U32         ulPPType;
  OMX_BOOL        useHighPrecIdctQp1;
}MP4VDEC_UALGDynamicParams;

#ifdef VIDDEC_SPARK_CODE 
typedef struct SPARKVDEC_UALGDynamicParams
{
#ifdef VIDDEC_SN_R8_14
  OMX_S32           size;
#endif
  OMX_U32         ulDecodeHeader;    
  OMX_U32         ulDisplayWidth;  
  OMX_U32         ulFrameSkipMode; 
  OMX_U32        ulPPType;
}SPARKVDEC_UALGDynamicParams;
#endif

typedef struct MP2VDEC_UALGDynamicParams
{
#ifdef VIDDEC_SN_R8_14
    OMX_S32            size;
#endif
    OMX_U32            ulDecodeHeader;    
    OMX_U32            ulDisplayWidth;  
    OMX_U32            ulFrameSkipMode; 
    OMX_U32            ulPPType;
    OMX_U32            ulPpNone;                  /* This will generate 4:2:0 planar output */
    OMX_U32            ulDyna_chroma_format;      /* If ON, chroma format can be modified at the format level */
  
} MP2VDEC_UALGDynamicParams;

#endif

