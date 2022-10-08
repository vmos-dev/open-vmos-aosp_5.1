
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
* @file OMX_VideoEnc_DSP.h
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

#ifndef OMX_VIDEOENC_DSP__H
#define OMX_VIDEOENC_DSP__H

#define OMX_H264ENC_NUM_DLLS 3
#define OMX_MP4ENC_NUM_DLLS  3
#define MAXNUMSLCGPS      8  /*< max. number of slice groups*/

#ifndef UNDER_CE
    #define H264_ENC_NODE_DLL "h264venc_sn.dll64P"
    #define MP4_ENC_NODE_DLL  "m4venc_sn.dll64P"
    #define USN_DLL           "usn.dll64P"
#else
    #define H264_ENC_NODE_DLL "/windows/h264venc_sn.dll64P"
    #define MP4_ENC_NODE_DLL  "/windows/m4venc_sn.dll64P"
    #define USN_DLL           "/windows/usn.dll64P"
#endif

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
    USN_STRMCMD_PLAY,
    USN_STRMCMD_PAUSE,
    USN_STRMCMD_STOP,
    USN_STRMCMD_SETCODECPARAMS,
    USN_STRMCMD_IDLE,
    USN_STRMCMD_FLUSH
}USN_StrmCmd;

typedef enum
{
  IH264_BYTE_STREAM = 0,
  IH264_NALU_STREAM

}IH264VENC_StreamFormat;

/*H264 Encoder Specific Error Code bits*/
typedef enum
{
  IH264VENC_SEQPARAMERR=0,          /* Indicates error during sequence parameter set generation*/
  IH264VENC_PICPARAMERR,            /* Indicates error during picture parameter set generation*/
  IH264VENC_COMPRESSEDSIZEOVERFLOW, /* Compressed data exceeds the maximum compressed size limit*/
  IH264VENC_INVALIDQPPARAMETER,     /* Out of Range initial Quantization parameter*/
  IH264VENC_INVALIDPROFILELEVEL,    /* Invalid profile or Level*/
  IH264VENC_INVALIDRCALGO,          /* Invalid RateControl Algorithm*/
  IH264VENC_SLICEEXCEEDSMAXBYTES,   /* Slice exceeds the maximum allowed bytes*/
  IH264VENC_DEVICENOTREADY          /* Indicates the device is not ready*/


} IH264VENC_ErrorBit;

/* H.264 Encoder Slice and Picture level Loop Filter Control*/
typedef enum
{
  FILTER_ALL_EDGES = 0,             /* Enable filtering of all the edges*/
  DISABLE_FILTER_ALL_EDGES,         /* Disable filtering of all the edges*/
  DISABLE_FILTER_SLICE_EDGES        /* Disable filtering of slice edges */

} IH264VENC_LoopFilterParams ;


/* H.264 Encoder Slice level Control for Intra4x4 Modes */
typedef enum
{
  INTRA4x4_NONE = 0 ,   /* Disable Intra4x4 modes */
  INTRA4x4_ISLICES  ,   /* Enable Intra4x4 modes only in I Slices*/
  INTRA4x4_IPSLICES     /* Enable Intra4x4 modes only in I and P Slices*/

} IH264VENC_Intra4x4Params ;

/* Level Identifier for H.264 Encoder*/
typedef enum
{
  IH264_LEVEL_10 = 10,  /* Level 1.0*/
  IH264_LEVEL_1b =  9,  /* Level 1.b*/
  IH264_LEVEL_11 = 11,  /* Level 1.1*/
  IH264_LEVEL_12 = 12,  /* Level 1.2*/
  IH264_LEVEL_13 = 13,  /* Level 1.3*/
  IH264_LEVEL_20 = 20,  /* Level 2.0*/
  IH264_LEVEL_21 = 21,  /* Level 2.1*/
  IH264_LEVEL_22 = 22,  /* Level 2.2*/
  IH264_LEVEL_30 = 30   /* Level 3.0*/

} IH264VENC_Level ;


/* Picture Order Count Type Identifier for H.264 Encoder*/
typedef enum
{
  IH264_POC_TYPE_0 = 0,  /* POC type 0*/
  IH264_POC_TYPE_2 = 2   /* POC type 2*/

} IH264VENC_PicOrderCountType ;

/* Picture Order Count Type Identifier for H.264 Encoder*/
typedef enum
{
  IH264_INTRAREFRESH_NONE       = 0 ,  /* Doesn't insert forcefully intra macro blocks*/
  IH264_INTRAREFRESH_CYCLIC_MBS     ,  /* Insters intra macro blocks in a cyclic fashion :*/
                                       /* cyclic interval is equal to airMbPeriod*/
  IH264_INTRAREFRESH_CYCLIC_SLICES  ,  /* Insters Intra Slices in a cyclic fashion: */
                                       /* no of intra slices is equal to sliceRefreshRowNumber*/
  IH264_INTRAREFRESH_RDOPT_MBS         /* position of intra macro blocks is intelligently */
                                       /* chosen by encoder, but the number of forcely coded*/
                                       /* intra macro blocks in a frame is gaurnteed to be */
                                       /* equal to totalMbsInFrame/airMbPeriod : Not valid for DM6446*/

} IH264VENC_IntraRefreshMethods ;

typedef enum
{
  IH264_INTERLEAVED_SLICE_GRP             = 0 , /* 0 : Interleaved Slices*/
  IH264_FOREGRND_WITH_LEFTOVER_SLICE_GRP  = 2 , /* 2 : ForeGround with Left Over*/
  IH264_RASTER_SCAN_SLICE_GRP             = 4   /* 4 : Raster Scan*/

} IH264VENC_SliceGroupMapType ;

typedef enum
{
  IH264_RASTER_SCAN             = 0 , /* 0 : Raster scan order*/
  IH264_REVERSE_RASTER_SCAN           /* 1 : Reverse Raster Scan Order*/

} IH264VENC_SliceGroupChangeDirection ;

/* H264 Encoder DSP s/n create phase arguments */
typedef struct H264VE_GPP_SN_Obj_CreatePhase {
    unsigned short usNumStreams;
    unsigned short usStreamId;
    unsigned short usBuffTypeInStream;
    unsigned short usMaxBuffsInStream;
    unsigned short usStreamId2;
    unsigned short usBuffTypeInStream2;
    unsigned short usMaxBuffsInStream2;

    unsigned short usReserved1;

    unsigned int   ulWidth;
    unsigned int   ulHeight;
    unsigned int   ulTargetBitRate;
    unsigned int   ulBitstreamBuffSize;
    unsigned int   ulIntraFramePeriod;
    unsigned int   ulFrameRate;

    unsigned char  ucYUVFormat;
    unsigned char  ucUnrestrictedMV;
    unsigned char  ucNumRefFrames;
    unsigned char  ucRateControlAlgorithm;
    unsigned char  ucIDREnable;
    unsigned char  ucDeblockingEnable;
    unsigned char  ucMVRange;
    unsigned char  ucQPIFrame;
    unsigned char  ucProfile;
    unsigned char  ucLevel;

    unsigned short usNalCallback;

    unsigned int   ulEncodingPreset;
    unsigned int   ulRcAlgo;
    unsigned short endArgs;
} H264VE_GPP_SN_Obj_CreatePhase;

/* H264 Encoder DSP s/n run-time dynamic parameters*/
typedef struct IVIDENC_DynamicParams {
     OMX_U32    size;              /*  size of this structure             */
     OMX_U32    inputHeight;       /*  Input frame height                 */
     OMX_U32    inputWidth;        /*  Input frame width                  */
     OMX_U32    refFrameRate;      /*  Reference or input frame rate*1000 */
     OMX_U32    targetFrameRate;   /*  Target frame rate * 1000           */
     OMX_U32    targetBitRate;     /*  Target bit rate in bits per second */
     OMX_U32    intraFrameInterval;/*  I frame interval e.g. 30           */
     OMX_U32    generateHeader;    /*  XDM_ENCODE_AU, XDM_GENERATE_HEADER */
     OMX_U32    captureWidth;      /*  DEFAULT(0): use imagewidth as pitch
                                       *  else use given capture width for
                                       *  pitch provided it is greater than
                                       *  image width.
                                       */
     OMX_U32    forceIFrame;       /*  Force given frame as I or IDR (in H.264)
                                             frame       */
} IVIDENC_DynamicParams;

typedef struct IH264VENC_DynamicParams {
    IVIDENC_DynamicParams videncDynamicParams ; /* must be followed for all video encoders*/
    OMX_U32      qpIntra                   ; /* initial QP of I frames Range[-1,51]. -1 is for auto initialization.*/
    OMX_U32      qpInter                   ; /* initial QP of P frames Range[-1,51]. -1 is for auto initialization.*/
    OMX_U32      qpMax                     ; /* Maximum QP to be used  Range[0,51]*/
    OMX_U32      qpMin                     ; /*< Minimum QP to be used  Range[0,51]*/
    OMX_U32      lfDisableIdc              ; /* Controls enable/disable loop filter, See IH264VENC_LoopFilterParams for more details*/
    OMX_U32      quartPelDisable           ; /*< enable/disable Quarter Pel Interpolation*/
    OMX_U32      airMbPeriod               ; /* Adaptive Intra Refesh MB Period: Period at which intra macro blocks should be insterted in a frame*/
    OMX_U32      maxMBsPerSlice            ; /* Maximum number of macro block in a slice <minimum value is 8>*/
    OMX_U32      maxBytesPerSlice          ; /* Maximum number of bytes in a slice */
    OMX_U32      sliceRefreshRowStartNumber; /* Row number from which slice needs to be intra coded*/
    OMX_U32      sliceRefreshRowNumber     ; /* Number of rows to be coded as intra slice*/
    OMX_U32      filterOffsetA             ; /* alpha offset for loop filter [-12, 12] even number*/
    OMX_U32      filterOffsetB             ; /* beta offset for loop filter [-12, 12] even number*/
    OMX_U32      log2MaxFNumMinus4         ; /*Limits the maximum frame number in the bit-stream to (1<< (log2MaxFNumMinus4 + 4)) Range[0,12]*/
    OMX_U32      chromaQPIndexOffset       ; /*Specifies offset to be added to luma QP for addressing QPC values table for chroma components. Valid value is between -12 and 12, (inclusive)*/
    OMX_U32      constrainedIntraPredEnable; /* Controls the intra macroblock coding in P slices [0,1]*/
    OMX_U32      picOrderCountType         ; /* Picture Order count type Valid values 0, 2*/
    OMX_U32      maxMVperMB                ; /* enable/Disable Multiple Motion vector per MB, valid values are [1, 4] [For DM6446, allowed value is only 1]*/
    OMX_U32      intra4x4EnableIdc         ; /* See IH264VENC_Intra4x4Params for more details*/
    OMX_U32      mvDataEnable              ; /*enable/Disable Motion vector access*/
    OMX_U32      hierCodingEnable          ; /*Enable/Disable Hierarchical P Frame (non-reference P frame) Coding. [Not useful for DM6446]*/
    OMX_U32      streamFormat              ; /* Signals the type of stream generated with Call-back*/
    OMX_U32      intraRefreshMethod        ; /* Mechanism to do intra Refresh, see IH264VENC_IntraRefreshMethods for valid values*/
    OMX_U32      perceptualQuant           ; /* Enable Perceptual Quantization a.k.a. Perceptual Rate Control*/
    OMX_U32      sceneChangeDet            ; /* Enable Scene Change Detection*/

    void   (*pfNalUnitCallBack)(OMX_U32 *pNalu, OMX_U32 *pPacketSizeInBytes, void *pContext) ; /* Function pointer of the call-back function to be used by Encoder*/
    void *pContext                          ; /*pointer to context structure used during callback*/
    /*Following Parameter are related to Arbitrary Slice Ordering (ASO)*/
    OMX_U32 numSliceASO                    ; /* Number of valid enteries in asoSliceOrder array valid range is [0,8],
                                               where 0 and 1 doesn't have any effect*/
    OMX_U32 asoSliceOrder[MAXNUMSLCGPS]    ; /* Array containing the order of slices in which they should
                                                be present in bit-stream. vaild enteries are [0, any entry lesser than numSlicesASO]*/
    /* Following Parameter are related to Flexible macro block ordering (FMO)*/
    OMX_U32 numSliceGroups                 ; /* Total Number of slice groups, valid enteries are [0,8]*/
    OMX_U32 sliceGroupMapType              ; /* Slice GroupMapType : For Valid enteries see IH264VENC_SliceGroupMapType*/
    OMX_U32 sliceGroupChangeDirectionFlag  ; /* Slice Group Change Direction Flag: Only valid when sliceGroupMapType
                                                 is equal to IH264_RASTER_SCAN_SLICE_GRP.
                                                 For valid values refer IH264VENC_SliceGroupChangeDirection*/
    OMX_U32 sliceGroupChangeRate           ; /* Slice Group Change Rate: Only valid when sliceGroupMapType
                                              is equal to IH264_RASTER_SCAN_SLICE_GRP.
                                               valid values are : [0, factor of number of Mbs in a row]*/
    OMX_U32 sliceGroupChangeCycle          ; /* Slice Group Change Cycle: Only valid when sliceGroupMapType
                                               is equal to IH264_RASTER_SCAN_SLICE_GRP.
                                               Valid values can be 0 to numMbsRowsInPicture, also constrained
                                               by sliceGroupChangeRate*sliceGroupChangeCycle < totalMbsInFrame*/
    OMX_U32 sliceGroupParams[MAXNUMSLCGPS] ; /* This field is useful in case of sliceGroupMapType equal to either
                                              IH264_INTERLEAVED_SLICE_GRP or IH264_FOREGRND_WITH_LEFTOVER_SLICE_GRP
                                              In both cases it has different meaning:
                                              In case of IH264_INTERLEAVED_SLICE_GRP:
                                              The i-th entery in this array is used to specify the number of consecutive
                                               slice group macroblocks to be assigned to the i-th slice group in
                                               raster scan order of slice group macroblock units.
                                               Valid values are 0 to totalMbsInFrame again constrained by sum of all the elements
                                               shouldn't exceed totalMbsInFrame
                                               In case of IH264_FOREGRND_WITH_LEFTOVER_SLICE_GRP:
                                               First entry in the array specify the start position of foreground region in terms
                                               of macroblock number, valid values are [0, totalMbsInFrame-1]
                                               Second entry in the array specify the end position of foreground region in terms
                                               of macroblock number, valid values are [0, totalMbsInFrame-1] with following constrains:
                                               endPos > startPos && endPos%mbsInOneRow > startPos%mbsInOneRow*/
} IH264VENC_DynamicParams;

/* H264 Encoder DSP s/n run-time input parameters */
typedef struct H264VE_GPP_SN_UALGInputParams
{

    IH264VENC_DynamicParams H264VENC_TI_DYNAMICPARAMS;
    OMX_U32   ulFrameIndex;

} H264VE_GPP_SN_UALGInputParams;

/* H264 Encoder DSP s/n run-time output parameters */
typedef struct H264VE_GPP_SN_UALGOutputParams {
    OMX_U32   ulBitstreamSize;
    OMX_S32   lFrameType;
    OMX_U32   ulNALUnitsPerFrame;   /*Number of total NAL units per frame*/
    OMX_U32   ulNALUnitsSizes[240];
    OMX_U32   ulFrameIndex;         /*Gives the number of the input frame wich NAL unit belongs*/
    OMX_U32   ulNALUnitIndex;       /*Number of current NAL unit inside the frame*/
} H264VE_GPP_SN_UALGOutputParams;

/* MPEG4/H263 Encoder DSP s/n create phase arguments */
typedef struct MP4VE_GPP_SN_Obj_CreatePhase {
    unsigned short usNumStreams;
    unsigned short usStreamId;
    unsigned short usBuffTypeInStream;
    unsigned short usMaxBuffsInStream;
    unsigned short usStreamId2;
    unsigned short usBuffTypeInStream2;
    unsigned short usMaxBuffsInStream2;
    unsigned short usReserved1;

    unsigned int   ulWidth;
    unsigned int   ulHeight;
    unsigned int   ulTargetBitRate;
    unsigned int   ulVBVSize;
    unsigned int   ulGOBHeadersInterval;

    unsigned char  ucIsMPEG4;
    unsigned char  ucYUVFormat;
    unsigned char  ucHEC;
    unsigned char  ucResyncMarker;
    unsigned char  ucDataPartitioning;
    unsigned char  ucReversibleVLC;
    unsigned char  ucUnrestrictedMV;
    unsigned char  ucFrameRate;
    unsigned char  ucRateControlAlgorithm;
    unsigned char  ucQPFirstIFrame;
    unsigned char  ucProfile;
    unsigned char  ucLevel;
    unsigned int   ulMaxDelay;

#ifndef MODE_3410
    unsigned int   ulVbvParamEnable;
    unsigned int   ulH263SliceMode;
#endif

    unsigned int   ulUseGOV;
    unsigned int   ulUseVOS;
    unsigned int   enableH263AnnexI;
    unsigned int   enableH263AnnexJ;
    unsigned int   enableH263AnnexT;

    unsigned short endArgs;
} MP4VE_GPP_SN_Obj_CreatePhase;

/* MPEG4/H263 Encoder DSP s/n run-time input parameters */
typedef struct MP4VE_GPP_SN_UALGInputParams {
    unsigned int   ulFrameIndex;
    unsigned int   ulTargetFrameRate;
    unsigned int   ulTargetBitRate;
    unsigned int   ulIntraFrameInterval;
    unsigned int   ulGenerateHeader;
    unsigned int   ulForceIFrame;

    unsigned int   ulResyncInterval;
    unsigned int   ulHecInterval;
    unsigned int   ulAIRRate;
    unsigned int   ulMIRRate;
    unsigned int   ulQPIntra;
    unsigned int   ulfCode;
    unsigned int   ulHalfPel;
    unsigned int   ulACPred;
    unsigned int   ul4MV;
    unsigned int   uluseUMV;
    unsigned int   ulMVDataEnable;
    unsigned int   ulResyncDataEnable;

    unsigned int   ulQPInter;               /* default QP for P frame, range 1 to 31  */
    unsigned int   ulLastFrame;
    unsigned int   ulcapturewidth;
    unsigned int   ulQpMax;
    unsigned int   ulQpMin;
} MP4VE_GPP_SN_UALGInputParams;

/* MPEG4/H263 Encoder DSP s/n run-time output parameters */
typedef struct MP4VE_GPP_SN_UALGOutputParams {
    unsigned int   ulBitstreamSize;
    unsigned int  cFrameType;/*changed from unsigned char  as SN did*/
    unsigned int   mvDataSize;
    unsigned int   numPackets;
    #ifdef MODE_3410
    unsigned char   MVData[9600];
    unsigned char   ResyncData[4800];
    #else
    unsigned char   MVData[12960];
    unsigned int    ResyncData[1620];
    #endif
} MP4VE_GPP_SN_UALGOutputParams;

/*
 *  ======== IVIDEO_RateControlPreset ========
 *  IVIDEO_DEFAULT => Default rate control of encoder
 *  IVIDEO_LOW_DELAY => CBR rate control for video conferencing
 *  IVIDEO_STORAGE => VBR rate control for local storage (DVD) recording
 *  IVIDEO_TWOPASS => two pass rate control for non real time applications
 *  IVIDEO_USER_DEFINED => User defined configuration using advanced parameters
 */
typedef enum {
    IVIDEO_LOW_DELAY =1, IVIDEO_STORAGE, IVIDEO_TWOPASS, IVIDEO_NONE, IVIDEO_USER_DEFINED
} IVIDEO_RateControlPreset;

#endif
