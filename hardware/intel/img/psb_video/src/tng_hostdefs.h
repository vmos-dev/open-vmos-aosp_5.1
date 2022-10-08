/*
 * Copyright (c) 2011 Intel Corporation. All Rights Reserved.
 * Copyright (c) Imagination Technologies Limited, UK
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Edward Lin <edward.lin@intel.com>
 *
 */
#ifndef _TNG_HOSTDEFS_H_
#define _TNG_HOSTDEFS_H_

#include "img_types.h"
#include "hwdefs/coreflags.h"

#define FORCED_REFERENCE 1
#define LTREFHEADER 1

#define MIN_30_REV 0x00030000
#define MAX_30_REV 0x00030099
#define MIN_32_REV 0x00030200
#define MAX_32_REV 0x00030299
#define MIN_34_REV 0x00030400
#define MAX_34_REV 0x00030499
#define MIN_36_REV 0x00030600
#define MAX_36_REV 0x00030699

#define MVEA_MV_PARAM_REGION_SIZE 16
#define MVEA_ABOVE_PARAM_REGION_SIZE 96

#define ALIGN_64(X)  (((X)+63) &~63)
#define ALIGN_4(X)  (((X)+3) &~3)

#define MTX_CONTEXT_SIZE (10 * 1024)


#define SHIFT_GOP_FRAMETYPE		(0)
#define MASK_GOP_FRAMETYPE		(0x3 << SHIFT_GOP_FRAMETYPE)
#define SHIFT_GOP_REFERENCE		(2)
#define MASK_GOP_REFERENCE		(0x1 << SHIFT_GOP_REFERENCE)
#define SHIFT_GOP_POS			(3)
#define MASK_GOP_POS			(0x1f << SHIFT_GOP_POS)
#define SHIFT_GOP_LEVEL			(4)
#define MASK_GOP_LEVEL			(0xF << SHIFT_GOP_LEVEL)
#define SHIFT_GOP_REF0			(0 + 8)
#define MASK_GOP_REF0			(0xf << SHIFT_GOP_REF0)
#define SHIFT_GOP_REF1			(4 + 8)
#define MASK_GOP_REF1			(0xf << SHIFT_GOP_REF1)
/**********************************************************************************************************/

#define MTX_CMDID_PRIORITY 0x80
#define MV_ROW_STRIDE ((sizeof(IMG_MV_SETTINGS) * MAX_BFRAMES + 63) & ~63)
#define MV_OFFSET_IN_TABLE(BDistance, Position) ((BDistance) * MV_ROW_STRIDE + (Position) * sizeof(IMG_MV_SETTINGS))

//Edward FIXME
#define MAX_GOP_SIZE    (MAX_BFRAMES + 1)
#define MV_ROW_STRIDE ((sizeof(IMG_MV_SETTINGS) * MAX_BFRAMES + 63) & ~63)
#define MV_ROW2 ((MAX_BFRAMES) * (MAX_BFRAMES) + 1)

/* Specific to Standard Latency */

//details   Sizes for arrays that depend on reference usage pattern
//brief      Reference usage
#define MAX_REF_B_LEVELS       3
#define MAX_REF_SPACING        1
#define MAX_REF_I_OR_P_LEVELS  (MAX_REF_SPACING + 2)
#define MAX_REF_LEVELS         (MAX_REF_B_LEVELS + MAX_REF_I_OR_P_LEVELS)
#define MAX_PIC_NODES          (MAX_REF_LEVELS + 2)
#define MAX_MV                 (MAX_PIC_NODES * 2)

#define MAX_BFRAMES            7
#define MAX_GOP_SIZE           (MAX_BFRAMES + 1)
#define MAX_SOURCE_SLOTS_SL    (MAX_GOP_SIZE + 1)


//brief      WB FIFO
#define LOG2_WB_FIFO_SIZE      ( 5 )

#define WB_FIFO_SIZE           ( 1 << (LOG2_WB_FIFO_SIZE) )

#define SHIFT_WB_PRODUCER      ( 0 )
#define MASK_WB_PRODUCER       ( ((1 << LOG2_WB_FIFO_SIZE) - 1) << SHIFT_WB_PRODUCER )

#define SHIFT_WB_CONSUMER      ( 0 )
#define MASK_WB_CONSUMER       ( ((1 << LOG2_WB_FIFO_SIZE) - 1) << SHIFT_WB_CONSUMER )

/*****************************************************************************/
#define SCALE_TBL_SZ            (8)
#define TOPAZHP_NUM_PIPES       (2)
#define TNG_HEADER_SIZE         (128)
#define NUM_SLICE_TYPES         (5)
/*****************************************************************************/
#define SHIFT_MTX_MSG_CMD_ID          (0)
#define MASK_MTX_MSG_CMD_ID           (0x7f << SHIFT_MTX_MSG_CMD_ID)
#define SHIFT_MTX_MSG_PRIORITY        (7)
#define MASK_MTX_MSG_PRIORITY         (0x1 << SHIFT_MTX_MSG_PRIORITY)
#define SHIFT_MTX_MSG_CORE            (8)
#define MASK_MTX_MSG_CORE             (0xff << SHIFT_MTX_MSG_CORE)
#define SHIFT_MTX_MSG_COUNT           (16)
#define MASK_MTX_MSG_COUNT            (0xffff << SHIFT_MTX_MSG_COUNT)
#define SHIFT_MTX_MSG_MESSAGE_ID      (16)
#define MASK_MTX_MSG_MESSAGE_ID       (0xff << SHIFT_MTX_MSG_MESSAGE_ID)
/*****************************************************************************/
#define SHIFT_MTX_MSG_PICMGMT_SUBTYPE           (0)
#define MASK_MTX_MSG_PICMGMT_SUBTYPE            (0xff << SHIFT_MTX_MSG_PICMGMT_SUBTYPE)
#define SHIFT_MTX_MSG_PICMGMT_DATA              (8)
#define MASK_MTX_MSG_PICMGMT_DATA               (0xffffff << SHIFT_MTX_MSG_PICMGMT_DATA)

#define SHIFT_MTX_MSG_RC_UPDATE_QP              (0)
#define MASK_MTX_MSG_RC_UPDATE_QP               (0x3f << SHIFT_MTX_MSG_RC_UPDATE_QP)
#define SHIFT_MTX_MSG_RC_UPDATE_BITRATE         (6)
#define MASK_MTX_MSG_RC_UPDATE_BITRATE          (0x03ffffff << SHIFT_MTX_MSG_RC_UPDATE_BITRATE)

#define SHIFT_MTX_MSG_PROVIDE_REF_BUFFER_USE    (0)
#define MASK_MTX_MSG_PROVIDE_REF_BUFFER_USE     (0xff << SHIFT_MTX_MSG_PROVIDE_REF_BUFFER_USE)
#define SHIFT_MTX_MSG_PROVIDE_REF_BUFFER_SLOT   (8)
#define MASK_MTX_MSG_PROVIDE_REF_BUFFER_SLOT    (0xff << SHIFT_MTX_MSG_PROVIDE_REF_BUFFER_SLOT)
#define SHIFT_MTX_MSG_PROVIDE_REF_BUFFER_LT     (16)
#define MASK_MTX_MSG_PROVIDE_REF_BUFFER_LT      (0xff << SHIFT_MTX_MSG_PROVIDE_REF_BUFFER_LT)

#define SHIFT_MTX_MSG_PROVIDE_CODED_BUFFER_SLOT (0)
#define MASK_MTX_MSG_PROVIDE_CODED_BUFFER_SLOT  (0x0f << SHIFT_MTX_MSG_PROVIDE_CODED_BUFFER_SLOT)
#define SHIFT_MTX_MSG_PROVIDE_CODED_BUFFER_SIZE (4)
#define MASK_MTX_MSG_PROVIDE_CODED_BUFFER_SIZE  (0x0fffffff << SHIFT_MTX_MSG_PROVIDE_CODED_BUFFER_SLOT)

#define SHIFT_MTX_MSG_RC_UPDATE_MIN_QP                  (0)
#define MASK_MTX_MSG_RC_UPDATE_MIN_QP                   (0x3f << SHIFT_MTX_MSG_RC_UPDATE_MIN_QP)
#define SHIFT_MTX_MSG_RC_UPDATE_MAX_QP                  (6)
#define MASK_MTX_MSG_RC_UPDATE_MAX_QP                   (0x3f << SHIFT_MTX_MSG_RC_UPDATE_MAX_QP)
#define SHIFT_MTX_MSG_RC_UPDATE_INTRA                   (12)
#define MASK_MTX_MSG_RC_UPDATE_INTRA                    (0xffff << SHIFT_MTX_MSG_RC_UPDATE_INTRA)

#define RC_MASK_frame_width        (1<<0)
#define RC_MASK_frame_height       (1<<1)
#define RC_MASK_bits_per_second    (1<<2)
#define RC_MASK_target_percentage  (1<<3)
#define RC_MASK_window_size        (1<<4)
#define RC_MASK_initial_qp         (1<<5)
#define RC_MASK_min_qp             (1<<6)
#define RC_MASK_force_kf           (1<<7)
#define RC_MASK_no_ref_last        (1<<8)
#define RC_MASK_no_ref_gf          (1<<9)
#define RC_MASK_no_ref_arf         (1<<10)
#define RC_MASK_frame_rate         (1<<11)
#define RC_MASK_intra_period       (1<<12)
#define RC_MASK_intra_idr_period   (1<<13)
#define RC_MASK_ip_period          (1<<14)
#define RC_MASK_quality            (1<<15)
#define RC_MASK_refresh_entropy_probs    (1<<16)
#define RC_MASK_copy_buffer_to_golden    (1<<17)
#define RC_MASK_copy_buffer_to_alternate (1<<18)
#define RC_MASK_refresh_last             (1<<19)
#define RC_MASK_refresh_golden_frame     (1<<20)
#define RC_MASK_refresh_alternate_frame  (1<<21)
#define RC_MASK_max_qp             (1<<22)

/*!
 *****************************************************************************
 *
 * @details   
 *
 * Enum describing Command IDs.  Some commands require data to be DMA'd in
 * from the Host, with the base address of the data specified in the Command  
 * Data Address word of the command.  The data required is specified with each
 * command type.
 * 
 * @brief          Command IDs
 *
 ****************************************************************************/
typedef enum {
	// Common Commands
    MTX_CMDID_NULL,                   //!< (no data)\n Null command does nothing\n
    MTX_CMDID_SHUTDOWN,               //!< (no data)\n shutdown the MTX\n

    // Video Commands
    MTX_CMDID_DO_HEADER,              //!< (extra data: #MTX_HEADER_PARAMS)\n Command for Sequence, Picture and Slice headers\n
    MTX_CMDID_ENCODE_FRAME,           //!< (no data)\n Encode frame data\n
    MTX_CMDID_START_FRAME,            //!< (no data)\n Prepare to encode frame\n
    MTX_CMDID_ENCODE_SLICE,           //!< (no data)\n Encode slice data\n
    MTX_CMDID_END_FRAME,              //!< (no data)\n Complete frame encoding\n
    MTX_CMDID_SETVIDEO,               //!< (data: pipe number, extra data: #IMG_MTX_VIDEO_CONTEXT)\n Set MTX Video Context\n
    MTX_CMDID_GETVIDEO,               //!< (data: pipe number, extra data: #IMG_MTX_VIDEO_CONTEXT)\n Get MTX Video Context\n
    MTX_CMDID_DO_CHANGE_PIPEWORK,     //!< (data: new pipe allocations for the context)\n Change pipe allocation for a Video Context\n
    MTX_CMDID_PICMGMT,                //!< (data: subtype and parameters, extra data: #IMG_PICMGMT_CUSTOM_QUANT_DATA (optional))\n Change encoding parameters\n
    MTX_CMDID_RC_UPDATE,              //!< (data: QP and bitrate)\n Change encoding parameters\n
    MTX_CMDID_PROVIDE_SOURCE_BUFFER,  //!< (extra data: #IMG_SOURCE_BUFFER_PARAMS)\n Transfer source buffer from host\n
    MTX_CMDID_PROVIDE_REF_BUFFER,     //!< (data: buffer parameters, extra data: reference buffer)\n Transfer reference buffer from host\n
    MTX_CMDID_PROVIDE_CODED_BUFFER,   //!< (data: slot and size, extra data: coded buffer)\n Transfer output buffer from host\n
    MTX_CMDID_ABORT,                  //!< (no data)\n Stop encoding and release all buffers\n

    // JPEG commands
    MTX_CMDID_SETQUANT,	             //!< (extra data: #JPEG_MTX_QUANT_TABLE)\n
    MTX_CMDID_SETUP_INTERFACE,       //!< (extra data: #JPEG WRITEBACK POINTERS)\n
    MTX_CMDID_ISSUEBUFF,             //!< (extra data: #MTX_ISSUE_BUFFERS)\n
    MTX_CMDID_SETUP,                 //!< (extra data: #JPEG_MTX_DMA_SETUP)\n\n

    MTX_CMDID_ENDMARKER,             //!< end marker for enum

    /* SW Commands */
    MTX_CMDID_PAD = 0x7a,           //!< Will be ignored by kernel
    MTX_CMDID_SW_WRITEREG = 0x7b,
    MTX_CMDID_SW_LEAVE_LOWPOWER = 0x7c,
    MTX_CMDID_SW_ENTER_LOWPOWER = 0x7e,
    MTX_CMDID_SW_NEW_CODEC = 0x7f,
    MTX_CMDID_SW_FILL_INPUT_CTRL = 0x81,
    MTX_CMDID_SW_UPDATE_AIR_SEND = 0x82,
    MTX_CMDID_SW_AIR_BUF_CLEAR = 0x83,
    MTX_CMDID_SW_UPDATE_AIR_CALC = 0x84
} tng_MTX_CMD_ID;


/*!
 *****************************************************************************
 * @details    Enum describing MTX firmware version (codec and rate control)
 * @brief          Firmware version
 ****************************************************************************/
typedef enum
{
    IMG_CODEC_JPEG = 0,    /* !< JPEG */
    IMG_CODEC_H264_NO_RC,  /* !< H264 with no rate control */
    IMG_CODEC_H264_VBR,    /* !< H264 variable bitrate */
    IMG_CODEC_H264_CBR,    /* !< H264 constant bitrate */
    IMG_CODEC_H264_VCM,    /* !< H264 video conferance mode */
    IMG_CODEC_H264_LLRC,   /* !< H264 low-latency rate control */
    IMG_CODEC_H264_ALL_RC, /* !< H264 with multiple rate control modes */
    IMG_CODEC_H263_NO_RC,  /* !< H263 with no rate control */
    IMG_CODEC_H263_VBR,    /* !< H263 variable bitrate */
    IMG_CODEC_H263_CBR,    /* !< H263 constant bitrate */
    IMG_CODEC_MPEG4_NO_RC, /* !< MPEG4 with no rate control */
    IMG_CODEC_MPEG4_VBR,   /* !< MPEG4 variable bitrate */
    IMG_CODEC_MPEG4_CBR,   /* !< MPEG4 constant bitrate */
    IMG_CODEC_MPEG2_NO_RC, /* !< MPEG2 with no rate control */
    IMG_CODEC_MPEG2_VBR,   /* !< MPEG2 variable bitrate */
    IMG_CODEC_MPEG2_CBR,   /* !< MPEG2 constant bitrate */
    IMG_CODEC_H264MVC_NO_RC, /* !< MVC H264 with no rate control */
    IMG_CODEC_H264MVC_CBR, /* !< MVC H264 constant bitrate */
    IMG_CODEC_H264MVC_VBR, /* !< MVC H264 variable bitrate */
    IMG_CODEC_NUM
} IMG_CODEC;

/*!
 *****************************************************************************
 * @details    Enum describing encoding standard (codec)
 * @brief          Encoding standard
 ****************************************************************************/
typedef enum
{
    IMG_STANDARD_NONE = 0,   //!< There is no FW in MTX memory
    IMG_STANDARD_JPEG,       //!< JPEG
    IMG_STANDARD_H264,       //!< H264 with no rate control
    IMG_STANDARD_H263,       //!< H263 with no rate control
    IMG_STANDARD_MPEG4,      //!< MPEG4 with no rate control
    IMG_STANDARD_MPEG2       //!< MPEG2 with no rate control
} IMG_STANDARD;

/*!
 *****************************************************************************
 * @details    Enum describing image surface format types
 * @brief          Image surface format
 ****************************************************************************/
typedef enum 
{
    IMG_CODEC_YUV,                  //!< Planar Y U V
    IMG_CODEC_YV12,                 //!< YV12 format Data
    IMG_CODEC_IMC2,                 //!< IMC2 format Data
    IMG_CODEC_PL8,                   //!< PL8 format YUV data
    IMG_CODEC_PL12,                 //!< PL12 format YUV data
    IMG_CODEC_422_YUV,           //!< YUV format 422 data
    IMG_CODEC_422_YV12,          //!< YV12 format 422 data
    IMG_CODEC_422_PL8,            //!< PL8 format 422 data
    IMG_CODEC_422_IMC2,          //!< PL8 format 422 data
    IMG_CODEC_422_PL12,          //!< PL12 format 422 data
    IMG_CODEC_Y0UY1V_8888,   //!< 422 YUYV data
    IMG_CODEC_Y0VY1U_8888,   //!< 422 YVYU data
    IMG_CODEC_UY0VY1_8888,   //!< 422 UYVY data
    IMG_CODEC_VY0UY1_8888,   //!< 422 VYUY data
    PVR_SURF_UNSPECIFIED      //!< End of the enum
} IMG_FORMAT;

/*****************************************************************************
 * @details    Structure describing coded header data returned by the firmware. 
 *             The size of the structure should not be more than 48-bytes 
 *              (i.e. CODED_BUFFER_HEADER_SIZE)
 * @brief      Coded header structure
 ****************************************************************************/
typedef struct
{
    IMG_UINT32  ui32BytesWritten;   //!< Bytes in this coded buffer excluding this header
    IMG_UINT32  ui32Feedback;       //!< Feedback word for this coded buffers
    IMG_UINT32  ui32ExtraFeedback;  //!< Extra feedback word for this coded buffers
    IMG_UINT32  ui32HostCtx;            //!< Host context value

    IMG_UINT16  ui16_I_MbCnt;       //!< Number of MBs coded as I-macroblocks in this slice
    IMG_UINT16  ui16_P_MbCnt;       //!< Number of MBs coded as P-macroblocks in this slice

    IMG_UINT16  ui16_B_MbCnt;       //!< Number of MBs coded as B-macroblocks in this slice
    IMG_UINT16  ui16_Skip_MbCnt;    //!< Number of MBs coded as skipped in this slice

    IMG_UINT16  ui16_IPCM_MbCnt;    //!< Number of macroblocks coded as IPCM in this slice
    IMG_UINT8  ui8_InterSumSatdHi;    //!< High 8 bits for the inter sum satd
    IMG_UINT8  ui8_IntraSumSatdHi;    //!< High 8 bits for the intra sum satd

    IMG_UINT32  ui32_DC_Bits;       //!< Number of bits use for coding DC coefficients in this slice
    IMG_UINT32  ui32_MV_Bits;       //!< Number of bits used for coding all Motion vector data in this slice
    IMG_UINT32  ui32_Symbols_Bits;  //!< Number of bits used for coding all MB level symbols in this slice
    IMG_UINT32  ui32_Residual_Bits; //!< Number of bits used for coding residual data in all MBs in this slice

    IMG_UINT32  ui32_QpyInter;      //!< Sum of QPy/Qscale for all Inter-MBs in the slice
    IMG_UINT32  ui32_QpyIntra;      //!< Sum of QPy/Qscale for all Intra-MBs in the slice
    IMG_UINT32  ui32_InterSumSatd;  //!< Sum of SATD for all Inter-MBs in the slice
    IMG_UINT32  ui32_IntraSumSatd;  //!< Sum of SATD for all Intra-MBs in the slice
    IMG_UINT32  ui32_MVOutputIndex; //!< Index into the motion vector buffers for this frame
} CODED_DATA_HDR, *P_CODED_DATA_HDR;

/*!
 ************************************************************
 *
 * @details     Mask defines for the -ui8EnableSelStatsFlags variable
 *
 * @brief       Selectable first pass and multipass statistics flag values
 *
 *************************************************************/
#define ESF_FIRST_STAGE_STATS 1
#define ESF_MP_BEST_MB_DECISION_STATS 2
#define ESF_MP_BEST_MOTION_VECTOR_STATS 4

/*!
******************************************************************************
 @details    Struct describing Bias parameters
 @brief          Bias parameters
******************************************************************************/
typedef struct tag_IMG_BIAS_PARAMS {
    IMG_UINT32      uLambdaSAD;
    IMG_UINT32      uLambdaSATD;
    IMG_UINT32      uLambdaSATDTable;


    IMG_UINT32      uIPESkipVecBias;
    IMG_UINT32      uDirectVecBias;
    IMG_UINT32      uSPESkipVecBias;

    IMG_UINT32  uisz1;
    IMG_UINT32  uisz2;

    IMG_INT32 uzb4;
    IMG_INT32 uzb8;

    IMG_UINT32  uTHInter;
    IMG_UINT32  uTHInterQP;
    IMG_UINT32  uTHInterMaxLevel;
    IMG_UINT32  uTHSkipIPE;
    IMG_UINT32  uTHSkipSPE;

    IMG_UINT32       iIntra16Bias;
    IMG_UINT32       iInterMBBias;
    IMG_UINT32       iInterMBBiasB;

    IMG_BOOL16      bRCEnable;
    IMG_BOOL16      bRCBiases;
    IMG_BOOL16      bZeroDetectionDisable;

} IMG_BIAS_PARAMS;

#define TOPAZ_PIC_PARAMS_VERBOSE 0

#define MAX_SLICES_PER_PICTURE 72
#define MAX_TOPAZ_CORES        4
#define MAX_TOPAZ_CMD_COUNT    (0x1000)

#define MAX_NUM_CORES 2

/* defines used for the second 32 bit word of the coded data header */
/* The peak bitrate was exceeded for this frame */
#define RC_STATUS_FLAG_BITRATE_OVERFLOW 0x00000080
/* At least one slice in this frame was larger than the slice limit */
#define RC_STATUS_FLAG_SLICE_OVERFLOW   0x00000040
/* At least one slice in this frame was large enough for the firmware to try to reduce it by increasing Qp or skipping MBs */
#define RC_STATUS_FLAG_LARGE_SLICE           0x00000020
#define SKIP_NEXT_FRAME                 0x800   /* The next frame should be skipped */

typedef enum _WEIGHTED_BIPRED_IDC_ {
    WBI_NONE = 0x0,
    WBI_EXPLICIT,
    WBI_IMPLICIT
} WEIGHTED_BIPRED_IDC;

typedef enum _IMG_RCMODE_ {
    IMG_RCMODE_NONE = 0,
    IMG_RCMODE_CBR,
    IMG_RCMODE_VBR,
    IMG_RCMODE_ERC, //Example Rate Control
    IMG_RCMODE_VCM
} IMG_RCMODE;

/*!
*****************************************************************************
@details    Struct describing rate control params
@brief          Rate control parameters
****************************************************************************/
typedef struct _RC_PARAMS_ {
    IMG_UINT32  ui32BitsPerSecond;  //!< Bit rate
    IMG_UINT32  ui32TransferBitsPerSecond; //!< Transfer rate of encoded data from encoder to the output
    IMG_UINT32  ui32InitialQp;      //!< Initial QP (only field used by JPEG)
    IMG_UINT32  ui32BUSize;         //!< Basic unit size
    IMG_UINT32  ui32FrameRate;
    IMG_UINT32  ui32BufferSize;
    IMG_UINT32  ui32IntraFreq;
    IMG_UINT32  ui32SliceByteLimit;
    IMG_UINT32  ui32SliceMBLimit;
    IMG_INT32   i32InitialLevel;
    IMG_INT32   i32InitialDelay;
    IMG_UINT16   iMinQP;
    IMG_UINT16  ui16BFrames;
    IMG_BOOL16  b16Hierarchical;
    IMG_BOOL16  bIsH264Codec;
    IMG_BOOL    bRCEnable;
    IMG_BOOL    bScDetectDisable;
    IMG_BOOL    bDisableFrameSkipping;
    IMG_BOOL    bDisableBitStuffing;
    IMG_RCMODE  eRCMode;
    IMG_UINT8   u8Slices;
    IMG_INT8    i8QCPOffset;

    IMG_BOOL    bBitrateChanged;
} IMG_RC_PARAMS;

/*!
 *****************************************************************************
 * @details    Struct describing rate control input parameters
 * @brief          Rate control input parameters
 ****************************************************************************/
typedef struct
{
    IMG_UINT16 ui16MBPerFrm;        //!< Number of MBs Per Frame
    IMG_UINT16 ui16MBPerBU;         //!< Number of MBs Per BU
    IMG_UINT16 ui16BUPerFrm;        //!< Number of BUs Per Frame

    IMG_UINT16 ui16IntraPeriod;       //!< Intra frame frequency
    IMG_UINT16 ui16BFrames;         //!< B frame frequency   

    IMG_INT32  i32BitsPerFrm;        //!< Bits Per Frame
    IMG_INT32  i32BitsPerBU;	  //!< Bits Per BU

    IMG_INT32  i32BitRate;             //!< Bit Rate (bps)
    IMG_INT32  i32BufferSize;        //!< Size of Buffer (VCM mode: in frames; all other modes: in bits)
    IMG_INT32  i32InitialLevel;        //!< Initial Level of Buffer
    IMG_INT32  i32InitialDelay;       //!< Initial Delay of Buffer

    IMG_BOOL16 bFrmSkipDisable; //!< Disable Frame skipping	

    IMG_UINT8  ui8SeInitQP;          //!< Initial QP for Sequence
    IMG_UINT8  ui8MinQPVal;        //!< Minimum QP value to use
    IMG_UINT8  ui8MaxQPVal;       //!< Maximum QP value to use

    IMG_UINT8  ui8ScaleFactor;     //!< Scale Factor used to limit the range of arithmetic with high resolutions and bitrates	
    IMG_UINT8  ui8MBPerRow;      //!< Number of MBs Per Row

    union {
        struct {
            IMG_INT32   i32TransferRate;    //!< Rate at which bits are sent from encoder to the output after each frame finished encoding
            IMG_BOOL16  bScDetectDisable;   //!< Disable Scene Change detection
            IMG_UINT32  ui32RCScaleFactor;  //!< Constant used in rate control = (GopSize/(BufferSize-InitialLevel))*256
            IMG_BOOL16  bHierarchicalMode;  //!< Flag indicating Hierarchical B Pic or Flat mode rate control
        } h264;
        struct {
            IMG_UINT8   ui8HalfFrameRate;   //!< Half Frame Rate (MP4 only)
            IMG_UINT8   ui8FCode;           //!< F Code (MP4 only)
            IMG_INT32   i32BitsPerGOP;	    //!< Bits Per GOP (MP4 only)
            IMG_BOOL16  bBUSkipDisable;     //!< Disable BU skipping
            IMG_INT32   i32BitsPerMB;       //!< Bits Per MB
            IMG_UINT16  ui16AvQPVal;        //!< Average QP in Current Picture
            IMG_UINT16  ui16MyInitQP;       //!< Initial Quantizer
        } other;
    } mode;
} IN_RC_PARAMS;

typedef enum _frame_type_ {
    IMG_INTRA_IDR = 0,
    IMG_INTRA_FRAME,
    IMG_INTER_P,
    IMG_INTER_B,
} IMG_FRAME_TYPE;

typedef struct {
    IMG_UINT32 ui32Flags;//!< Picture parameter flags
    IN_RC_PARAMS sInParams;//!< Rate control parameters
} PIC_PARAMS;

typedef struct tag_IMG_MV_SETTINGS {
    IMG_UINT32      ui32MVCalc_Below;
    IMG_UINT32      ui32MVCalc_Colocated;
    IMG_UINT32      ui32MVCalc_Config;
} IMG_MV_SETTINGS;

typedef enum {
    BLK_SZ_16x16 = 0,           //!< Use 16x16 block size for motion search. This is the smallest for h.263
    BLK_SZ_8x8 = 1,                     //!< Use 'upto' 8x8 block size for motion search. This is the smallest for MPEG-4
    BLK_SZ_4x4 = 2,                     //!< Use 'upto' 4x4 block size for motion search. This is the smallest for H.264
    BLK_SZ_DEFAULT = 3  //!< Driver picks the best possible block size for this encode session
} IMG_IPE_MINBLOCKSIZE;

typedef struct {
    IMG_BOOL16      bDisableIntra4x4;           //!< Disable Intra 4x4.
    IMG_BOOL16      bDisableIntra8x8;           //!< Disable Intra 8x8.
    IMG_BOOL16      bDisableIntra16x16; //!< Disable Intra 16x16.
    IMG_BOOL16      bDisableInter8x8;           //!< Disable Inter 8x8.
    IMG_BOOL16      bRestrictInter4x4;          //!< Only one 8x8 block may be divided into 4x4 block per MB
    IMG_BOOL16      bDisableBPicRef_1;  //!< Don't allow b-picture to refer reference-picture-1
    IMG_BOOL16      bDisableBPicRef_0;  //!< Don't allow b-picture to refer reference-picture-0
    IMG_BOOL16      bEnable8x16MVDetect;//!< Enable 8x16 motion vector block size detection
    IMG_BOOL16      bEnable16x8MVDetect;//!< Enable 16x8 motion vector block size detection
    IMG_BOOL16      bDisableBFrames;    //!< Disable B-frames in encoded output
    IMG_IPE_MINBLOCKSIZE    eMinBlkSz;  //!< Smallest block size used for motion search
} IMG_ENCODE_FEATURES;

typedef enum {
    ENC_PROFILE_DEFAULT = 0,    //!< Sets features for default video encode
    ENC_PROFILE_LOWCOMPLEXITY,  //!< Sets features for low complexity video encode
    ENC_PROFILE_HIGHCOMPLEXITY  //!< Sets features for low delay video encode
} IMG_VIDEO_ENC_PROFILE;

typedef enum {
    H264ES_PROFILE_BASELINE = 5,
    H264ES_PROFILE_MAIN,
    H264ES_PROFILE_HIGH
} IMG_VIDEO_H264ES_PROFILE;

#define MAX_SLICESPERPIC                (68)

/*!
 ***********************************************************************************
 *
 * Description        : Video encode context
 *
 ************************************************************************************/
typedef struct tag_IMG_MTX_VIDEO_CONTEXT
{
    IMG_UINT64      ui64ClockDivBitrate; // keep this at the top as it has alignment issues 

    IMG_UINT32      ui32WidthInMbs;                         //!< target output width
    IMG_UINT32      ui32PictureHeightInMbs;                 //!< target output height

#ifdef FORCED_REFERENCE
    IMG_UINT32      apTmpReconstructured[MAX_PIC_NODES];
#endif
    IMG_UINT32      apReconstructured[MAX_PIC_NODES];
    IMG_UINT32      apColocated[MAX_PIC_NODES];
    IMG_UINT32      apMV[MAX_MV];
    IMG_UINT32      apInterViewMV[2];

    IMG_UINT32      ui32DebugCRCs;                          //!< Send debug information from Register CRCs to Host with the coded buffer

    IMG_UINT32      apWritebackRegions[WB_FIFO_SIZE];       //!< Data section

    IMG_UINT32      ui32InitialCPBremovaldelayoffset;
    IMG_UINT32      ui32MaxBufferMultClockDivBitrate;
    IMG_UINT32      pSEIBufferingPeriodTemplate;
    IMG_UINT32      pSEIPictureTimingTemplate;

    IMG_BOOL16      b16EnableMvc;
    IMG_UINT16      ui16MvcViewIdx;
    IMG_UINT32      apSliceParamsTemplates[5];
    IMG_UINT32      apPicHdrTemplates[4];

    IMG_UINT32      apSeqHeader;
    IMG_UINT32      apSubSetSeqHeader;
    IMG_BOOL16      b16NoSequenceHeaders;

    IMG_UINT32      aui32SliceMap[MAX_SOURCE_SLOTS_SL];     //!< Slice map of the source picture

    IMG_UINT32      ui32FlatGopStruct;                      //!< Address of Flat MiniGop structure

    IMG_BOOL8       b8WeightedPredictionEnabled;
    IMG_UINT8       ui8MTXWeightedImplicitBiPred;
    IMG_UINT32      aui32WeightedPredictionVirtAddr[MAX_SOURCE_SLOTS_SL];

    IMG_UINT32      ui32HierarGopStruct;                    //!< Address of hierarchical MiniGop structure

    IMG_UINT32      pFirstPassOutParamAddr[MAX_SOURCE_SLOTS_SL];                //!< Output Parameters of the First Pass
#ifndef EXCLUDE_BEST_MP_DECISION_DATA
    IMG_UINT32      pFirstPassOutBestMultipassParamAddr[MAX_SOURCE_SLOTS_SL];   //!< Selectable Output Best MV Parameters data of the First Pass
#endif
    IMG_UINT32      pMBCtrlInParamsAddr[MAX_SOURCE_SLOTS_SL];                   //!< Input Parameters to the second pass

    IMG_UINT32      ui32InterIntraScale[SCALE_TBL_SZ];
    IMG_UINT32      ui32SkippedCodedScale[SCALE_TBL_SZ];

    IMG_UINT32      ui32PicRowStride;                       //!< Strides of source Y data and chroma data

    IMG_UINT32      apAboveParams[TOPAZHP_NUM_PIPES];       //!< Picture level parameters (supplied by driver)

    IMG_UINT32      ui32IdrPeriod;
    IMG_UINT32      ui32IntraLoopCnt;
    IMG_UINT32      ui32BFrameCount;
    IMG_BOOL8       b8Hierarchical;
    IMG_UINT8       ui8MPEG2IntraDCPrecision;               //!< Only used in MPEG2, 2 bit field (0 = 8 bit, 1 = 9 bit, 2 = 10 bit and 3=11 bit precision). Set to zero for other encode standards.
    IMG_UINT8       aui8PicOnLevel[MAX_REF_LEVELS];

    IMG_UINT32      ui32VopTimeResolution;

    IMG_UINT32      ui32InitialQp;                          //!< Initial QP (only field used by JPEG)
    IMG_UINT32      ui32BUSize;                             //!< Basic unit size

    IMG_MV_SETTINGS sMVSettingsIdr;

    IMG_MV_SETTINGS sMVSettingsNonB[MAX_BFRAMES + 1];

    IMG_UINT32      ui32MVSettingsBTable;
    IMG_UINT32      ui32MVSettingsHierarchical;
#ifdef FIRMWARE_BIAS
    IMG_UINT32      aui32DirectBias_P[27];
    IMG_UINT32      aui32InterBias_P[27];

    IMG_UINT32      aui32DirectBias_B[27];
    IMG_UINT32      aui32InterBias_B[27];
#endif

    IMG_FORMAT      eFormat;                                //!< Pixel format of the source surface
    IMG_STANDARD    eStandard;                              //!< Encoder standard (H264 / H263 / MPEG4 / JPEG)
    IMG_RCMODE      eRCMode;                                //!< RC flavour

    IMG_BOOL8       b8FirstPic;
    IMG_BOOL8       b8IsInterlaced;
    IMG_BOOL8       b8TopFieldFirst;
    IMG_BOOL8       b8ArbitrarySO;
    IMG_BOOL8       bOutputReconstructed;

    IMG_BOOL8       b8DisableBitStuffing;

    IMG_BOOL8       b8InsertHRDparams;

    IMG_UINT8       ui8MaxSlicesPerPicture;

    /* Contents Adaptive Rate Control parameters*/
    IMG_BOOL16      bCARC;
    IMG_INT32       iCARCBaseline;
    IMG_UINT32      uCARCThreshold;
    IMG_UINT32      uCARCCutoff;
    IMG_UINT32      uCARCNegRange;
    IMG_UINT32      uCARCNegScale;
    IMG_UINT32      uCARCPosRange;
    IMG_UINT32      uCARCPosScale;
    IMG_UINT32      uCARCShift;

    IMG_UINT32      ui32MVClip_Config;                      //!< Value to use for MVClip_Config  register
    IMG_UINT32      ui32PredCombControl;                    //!< Value to use for Predictor combiner register
    IMG_UINT32      ui32LRITC_Tile_Use_Config;              //!< Value to use for LRITC_Tile_Use_Config register
    IMG_UINT32      ui32LRITC_Cache_Chunk_Config;           //!< Value to use for LRITC_Tile_Free_Config register
    IMG_UINT32      ui32IPEVectorClipping;                  //!< Value to use for IPEVectorClipping register
    IMG_UINT32      ui32H264CompControl;                    //!< Value to use for H264CompControl register
    IMG_UINT32      ui32H264CompIntraPredModes;             //!< Value to use for H264CompIntraPredMode register
    IMG_UINT32      ui32IPCM_0_Config;                      //!< Value to use for IPCM_0 Config register
    IMG_UINT32      ui32IPCM_1_Config;                      //!< Value to use for IPCM_1 Config register
    IMG_UINT32      ui32SPEMvdClipRange;                    //!< Value to use for SPEMvdClipRange register
    IMG_UINT32      ui32JMCompControl;                      //!< Value to use for JMCompControl register
    IMG_UINT32      ui32MBHostCtrl;                         //!< Value to use for MB_HOST_CONTROL register
    IMG_UINT32      ui32DeblockCtrl;                        //!< Value for the CR_DB_DISABLE_DEBLOCK_IDC register
    IMG_UINT32      ui32SkipCodedInterIntra;                //!< Value for the CR_DB_DISABLE_DEBLOCK_IDC register

    IMG_UINT32      ui32VLCControl;
    IMG_UINT32      ui32VLCSliceControl;                    //!< Slice control register value. Configures the size of a slice 
    IMG_UINT32      ui32VLCSliceMBControl;                  //!< Slice control register value. Configures the size of a slice 
    IMG_UINT16      ui16CQPOffset;                          //!< Chroma QP offset to use (when PPS id = 0) 

    IMG_BOOL8       b8CodedHeaderPerSlice;

    IMG_UINT32      ui32FirstPicFlags;
    IMG_UINT32      ui32NonFirstPicFlags;

#ifndef EXCLUDE_ADAPTIVE_ROUNDING
    IMG_BOOL16      bMCAdaptiveRoundingDisable;
    IMG_UINT16      ui16MCAdaptiveRoundingOffsets[18][4];
    IMG_INT16       i16MCAdaptiveRoundingOffsetsDelta[7][4];
#endif

#ifdef FORCED_REFERENCE
    IMG_UINT32      ui32PatchedReconAddress;                //!< Reconstructed address to allow host picture management
    IMG_UINT32      ui32PatchedRef0Address;                 //!< Reference 0 address to allow host picture management
    IMG_UINT32      ui32PatchedRef1Address;                 //!< Reference 1 address to allow host picture management
#endif
#ifdef LTREFHEADER
    IMG_UINT32      aui32LTRefHeader[MAX_SOURCE_SLOTS_SL];
    IMG_INT8        i8SliceHeaderSlotNum;
#endif
    IMG_BOOL8       b8ReconIsLongTerm;
    IMG_BOOL8       b8Ref0IsLongTerm;
    IMG_BOOL8       b8Ref1IsLongTerm;
    IMG_UINT8       ui8RefSpacing;

#if INPUT_SCALER_SUPPORTED
    IMG_UINT32      ui32ScalerInputSizeReg;
    IMG_UINT32      ui32ScalerCropReg;
    IMG_UINT32      ui32ScalerPitchReg;
    IMG_UINT32      asHorScalerCoeffRegs[4];
    IMG_UINT32      asVerScalerCoeffRegs[4];
#endif

    IMG_UINT8       ui8NumPipes;
    IMG_UINT8       ui8FirstPipe;
    IMG_UINT8       ui8LastPipe;
    IMG_UINT8       ui8PipesToUseFlags;

    /*
    The following IN_RC_PARAMS should never be used by RC.
    This is because MVC RC module is unable to alter them, thus
    they may (and will, in case of MVC) contain incorrect values.
    */
    IN_RC_PARAMS    sInParams;
}IMG_MTX_VIDEO_CONTEXT;

typedef struct _OMX_CARC_PARAMS {
    IMG_BOOL        bCARC;
    IMG_INT32       i32CARCBaseline;
    IMG_UINT32      ui32CARCThreshold;
    IMG_UINT32      ui32CARCCutoff;
    IMG_UINT32      ui32CARCNegRange;
    IMG_UINT32      ui32CARCNegScale;
    IMG_UINT32      ui32CARCPosRange;
    IMG_UINT32      ui32CARCPosScale;
    IMG_UINT32      ui32CARCShift;
} OMX_CARC_PARAMS;

typedef struct tag_IMG_BIAS_TABLES {
    IMG_UINT32 aui32LambdaBias[53];
    IMG_UINT32 aui32IntraBias[53];
    IMG_UINT32 aui32IntraScale[53];
    IMG_UINT32 aui32QpBias[53];

    IMG_UINT32 aui32DirectBias_P[53];
    IMG_UINT32 aui32InterBias_P[53];

    IMG_UINT32 aui32DirectBias_B[53];
    IMG_UINT32 aui32InterBias_B[53];

    IMG_UINT32  ui32sz1;
    IMG_UINT32      ui32RejectThresholdH264;

    IMG_UINT32      ui32FCode;                          //!< value only used in MPEG4

    IMG_UINT32      ui32LritcCacheChunkConfig;
    IMG_UINT32      ui32SeqConfigInit;

} IMG_BIAS_TABLES;

/*!
*****************************************************************************
@details                Struct describing surface component info
@brief                  Surface component info
****************************************************************************/
typedef struct {
    IMG_UINT32 ui32Step;
    IMG_UINT32 ui32Width;
    IMG_UINT32 ui32Height;
    IMG_UINT32 ui32PhysWidth;
    IMG_UINT32 ui32PhysHeight;
} IMG_SURF_COMPONENT_INFO;


/*!
*****************************************************************************
@details                Struct describing a frame
@brief                  Frame information
****************************************************************************/
typedef struct {
    //IMG_BUFFER *psYPlaneBuffer;                    //!< pointer to the image buffer
    //IMG_BUFFER *psUPlaneBuffer;                    //!< pointer to the image buffer
    //IMG_BUFFER *psVPlaneBuffer;                    //!< pointer to the image buffer
    IMG_UINT32 ui32Width;                          //!< stride of pBuffer
    IMG_UINT32 ui32Height;                         //!< height of picture in pBuffer

    IMG_UINT32 ui32ComponentCount;                 //!< number of colour components used
    IMG_FORMAT eFormat;

    IMG_UINT32      aui32ComponentOffset[3];
    IMG_UINT32      aui32BottomComponentOffset[3];
    IMG_SURF_COMPONENT_INFO aui32ComponentInfo[3];

    IMG_INT32 i32YComponentOffset;
    IMG_INT32 i32UComponentOffset;
    IMG_INT32 i32VComponentOffset;

    IMG_INT32 i32Field0YOffset, i32Field1YOffset;
    IMG_INT32 i32Field0UOffset, i32Field1UOffset;
    IMG_INT32 i32Field0VOffset, i32Field1VOffset;

    IMG_UINT16 ui16SrcYStride, ui16SrcUVStride;

} IMG_FRAME, JPEG_SOURCE_SURFACE;


/*!
*****************************************************************************
 @details    Struct describing the capabilities of the encoder
 @brief          Encoder Caps
****************************************************************************/
typedef struct _IMG_ENC_CAPS {
    IMG_UINT16      ui16MinSlices;                  //!< Minimum slices to use
    IMG_UINT16      ui16MaxSlices;                  //!< Maximum slices to use
    IMG_UINT16      ui16RecommendedSlices;  //!< Recommended number of slices
    IMG_UINT32      ui32NumCores;                   //!< Number of cores that will be used
    IMG_UINT32      ui32CoreFeatures;               //!< Core features flags
} IMG_ENC_CAPS;

typedef struct _IMG_FIRST_STAGE_MB_PARAMS {
    IMG_UINT16      ui16Ipe0Sad;        //!< Final SAD value for best candidate calculated by IPE 0
    IMG_UINT16      ui16Ipe1Sad;        //!< Final SAD value for best candidate calculated by IPE 1
    IMG_UINT8       ui8Ipe0Blks;        //!< Block dimentions for IPE 0 for this Macro-Block
    IMG_UINT8       ui8Ipe1Blks;        //!< Block dimentions for IPE 1 for this Macro-Block
    IMG_UINT8       ui8CARCCmplxVal;    //!< CARC complexity value for this macroblock
    IMG_UINT8       ui8dummy;           //!< Reserved (added for alignment).

} IMG_FIRST_STAGE_MB_PARAMS, *P_IMG_FIRST_STAGE_MB_PARAMS;

/*!
 *****************************************************************************
 *
 * @Name           Picture Parameter Flags
 *
 * @details    Picture parameter flags used in the PIC_PARAM structure
 *
 ****************************************************************************/
/* @{ */
#define ISINTERP_FLAGS                          (0x00000001)
#define ISMPEG2_FLAGS                          (0x00000002)
#define ISMPEG4_FLAGS                          (0x00000004)
#define ISH263_FLAGS                              (0x00000008)
#define ISRC_FLAGS                                 (0x00000010)
#define ISRC_I16BIAS                                (0x00000020)
#define LOW_LATENCY_INTRA_ON_FLY   (0x00000040)
#define ISINTERB_FLAGS                          (0x00000080)
#define ISSCENE_DISABLED                     (0x00000100)
#define ISMULTIREF_FLAGS                     (0x00000200)
#define SPATIALDIRECT_FLAGS               (0x00000400)
/* @} */

/*!
 *****************************************************************************
 *
 * @details    SEI (Buffering Period and Picture Timing) Constants shared between host and firmware
 *
 * @brief      SEI Constants
 *
 ****************************************************************************/
#define BPH_SEI_NAL_INITIAL_CPB_REMOVAL_DELAY_SIZE 23
#define BPH_SEI_NAL_INITIAL_CPB_REMOVAL_DELAY_OFFSET_SIZE BPH_SEI_NAL_INITIAL_CPB_REMOVAL_DELAY_SIZE
#define PTH_SEI_NAL_CPB_REMOVAL_DELAY_SIZE 23
#define PTH_SEI_NAL_DPB_OUTPUT_DELAY_SIZE 7

/*!
 *****************************************************************************
 * @details    Enum describing threshold values for skipped MB biasing
 * @brief          Skip thresholds
 ****************************************************************************/
typedef enum
{
    TH_SKIP_0    = 0,    //!< Bias threshold for QP 0 to 12
    TH_SKIP_12  = 1,    //!< Bias threshold for QP 12 to 24
    TH_SKIP_24  = 2     //!< Bias threshold for QP 24 and above
} TH_SKIP_SCALE;


typedef enum 
{
    MTX_SCRATCHREG_FULLNESS = 0,   //!< Coded buffer fullness
    MTX_SCRATCHREG_TOHOST,         //!< Reg for MTX->Host data
    MTX_SCRATCHREG_TOMTX,          //!< Reg for Host->MTX data

    MTX_SCRATCHREG_SIZE            //!< End marker for enum
} MTX_eScratchRegData;


#define MASK_INTEL_CH_PY 0x000000FF
#define SHIFT_INTEL_CH_PY 0
#define MASK_INTEL_CH_MX 0x0000FF00
#define SHIFT_INTEL_CH_MX 8
#define MASK_INTEL_CH_PM 0x00FF0000
#define SHIFT_INTEL_CH_PM 16

#define MASK_INTEL_H264_ConfigReg1 0x0000001F
#define SHIFT_INTEL_H264_ConfigReg1 0
#define MASK_INTEL_H264_ConfigReg2 0x00003F00
#define SHIFT_INTEL_H264_ConfigReg2 8
#define MASK_INTEL_H264_LL 0x00010000
#define SHIFT_INTEL_H264_LL 16
#define MASK_INTEL_H264_LL8X8P 0x00020000
#define SHIFT_INTEL_H264_LL8X8P 17
#define INTEL_SZ  0x0344
#define INTEL_CHCF 0x0050
#define INTEL_H264_RT 0x0184




#endif //_TNG_HOSTDEFS_H_
