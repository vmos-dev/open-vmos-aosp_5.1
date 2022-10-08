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
 *    Elaine Wang <elaine.wang@intel.com>
 *    Zeng Li <zeng.li@intel.com>
 *
 */

#include "img_types.h"
#include "psb_drv_video.h"
#include "psb_surface.h"
#include "pnw_cmdbuf.h"
#include "pnw_hostjpeg.h"
#include "pnw_hostheader.h"

#define TOPAZ_PIC_PARAMS_VERBOSE 0

#define MAX_SLICES_PER_PICTURE 72
#define MAX_TOPAZ_CORES        4
#define MAX_TOPAZ_CMD_COUNT    (0x1000)

#define TH_SKIP_IPE                     6
#define TH_INTER                        60
#define TH_INTER_QP                     10
#define TH_INTER_MAX_LEVEL      1500
#define TH_SKIP_SPE                     6
#define SPE_ZERO_THRESHOLD      6

#define MAX_NUM_CORES 2


/* defines used for the second 32 bit word of the coded data header */
/* the average Qp used in this frame */
#define RC_STATUS_FRAME_AVE_QP_MASK             0x0ff
/* At least one slice in this frame was large enough for the firmware to
 * try to reduce it by increasing Qp or skipping MBs */
#define RC_STATUS_FLAG_LARGE_SLICE              0x100
/* At least one slice in this frame was larger than the slice limit */
#define RC_STATUS_FLAG_SLICE_OVERFLOW   0x200
/* The peak bitrate was exceeded for this frame (VCM only) */
#define RC_STATUS_FLAG_BITRATE_OVERFLOW 0x400
#define SKIP_NEXT_FRAME                 0x800   /* The next frame should be skipped */

#define IS_H264_ENC(codec) \
    (IMG_CODEC_H264_CBR == (codec) || IMG_CODEC_H264_VCM == (codec))

/* commands for topaz,shared with user space driver */
enum drm_pnw_topaz_cmd {
    /* Common Commands */
    MTX_CMDID_NULL,
    MTX_CMDID_SHUTDOWN,

    /* Video Commands */
    MTX_CMDID_START_PIC,
    MTX_CMDID_DO_HEADER,
    MTX_CMDID_ENCODE_SLICE,
    MTX_CMDID_END_PIC,
    MTX_CMDID_FLUSH,

    /* JPEG Commands */
    MTX_CMDID_SETQUANT,  //!< (data: #JPEG_MTX_QUANT_TABLE)\n
    MTX_CMDID_RESET_ENCODE, //!< (no data)\n
    MTX_CMDID_ISSUEBUFF, //!< (data: #MTX_ISSUE_BUFFERS)\n
    MTX_CMDID_SETUP,    //!< (data: #JPEG_MTX_DMA_SETUP)\n\n

    /* SW Commands */
    MTX_CMDID_PAD = 0x7a, //!< Will be ignored by kernel
    MTX_CMDID_SW_WRITEREG = 0x7b,
    MTX_CMDID_SW_LEAVE_LOWPOWER = 0x7c,
    MTX_CMDID_SW_ENTER_LOWPOWER = 0x7e,
    MTX_CMDID_SW_NEW_CODEC = 0x7f
};


/* codecs topaz supports,shared with user space driver */
enum drm_pnw_topaz_codec {
    IMG_CODEC_JPEG = 0,
    IMG_CODEC_H264_NO_RC,
    IMG_CODEC_H264_VBR,
    IMG_CODEC_H264_CBR,
    IMG_CODEC_H263_NO_RC,
    IMG_CODEC_H263_VBR,
    IMG_CODEC_H263_CBR,
    IMG_CODEC_MPEG4_NO_RC,
    IMG_CODEC_MPEG4_VBR,
    IMG_CODEC_MPEG4_CBR,
    IMG_CODEC_H264_VCM,     //!< H264 low constant bitrate (video conferencing mode)
    IMG_CODEC_NUM
};

typedef struct _RC_PARAMS_ {
    IMG_UINT32  BitsPerSecond;
    IMG_UINT32  InitialQp;
    IMG_UINT32  BUSize;
    IMG_UINT32  FrameRate;
    IMG_UINT32  BufferSize;
    IMG_UINT32  IntraFreq;
    IMG_UINT16  IDRFreq;
    IMG_UINT16   MinQP;
    IMG_BOOL    RCEnable;
    IMG_BOOL    FrameSkip;

    IMG_UINT8   Slices;
    IMG_UINT32   InitialLevel;
    IMG_INT32   InitialDelay;
    IMG_INT8    QCPOffset;

    IMG_BOOL    bDisableFrameSkipping;
    IMG_BOOL    bDisableBitStuffing;
    IMG_BOOL    bBitrateChanged;
} IMG_RC_PARAMS;

/*!
*****************************************************************************
*
* @Description    Struct describing rate control input parameters
*
* @Brief          Rate control input parameters
*
****************************************************************************/
typedef struct {
    IMG_UINT8   SeInitQP;               /* Initial QP for Sequence */
    IMG_UINT8   MinQPVal;               /* Minimum QP value to use */
    IMG_UINT8   MaxQPVal;               /* Maximum QP value to use */

    IMG_UINT8   MBPerRow;               /* Number of MBs Per Row */
    IMG_UINT16  MBPerFrm;               /* Number of MBs Per Frame */
    IMG_UINT16  MBPerBU;                /* Number of MBs Per BU */
    IMG_UINT16  BUPerFrm;               /* Number of BUs Per Frame */

    IMG_UINT16  IntraPeriod;    /* Intra frame frequency */

    IMG_INT32   BitsPerFrm;             /* Bits Per Frame */
    IMG_INT32   BitsPerBU;              /* Bits Per BU */
    IMG_INT32   BitsPerMB;              /* Bits Per MB */

    IMG_INT32   BitRate;                        /* Bit Rate (bps) */
    IMG_INT32   BufferSize;             /* Size of Buffer */
    IMG_INT32   InitialLevel;   /* Initial Level of Buffer */
    IMG_INT32   InitialDelay;   /* Initial Delay of Buffer */

    IMG_UINT8   ScaleFactor;            /* Scale Factor (H264 only) */

    /* Bitrate that should be targetted as a fraction of 128
     * relative to maximum bitrate i32BitRate (VCM mode only)*/
    IMG_UINT8   VCMBitrateMargin;

    IMG_UINT8   HalfFrameRate;  /* Half Frame Rate (MP4 only) */
    IMG_UINT8   FCode;                  /* F Code (MP4 only) */

    /* TO BE DELETED -- ONCE MP4 RC CODE IS OPTIMISED */
    IMG_INT32   BitsPerGOP;             /* Bits Per GOP (MP4 only) */
    IMG_UINT16  AvQPVal;                /* Average QP in Current Picture */
    IMG_UINT16  MyInitQP;               /* Initial Quantizer */

    /* The number of bits of margin to leave before forcing
     * skipped macroblocks (VCM mode only)*/
    IMG_INT32   ForeceSkipMargin;

    /* A constant used in rate control =
     * (GopSize/(BufferSize-InitialLevel))*256 */
    IMG_UINT32  RCScaleFactor;

    /* Only used in peak constrained VBR */
    IMG_INT32 TransferRate;
    IMG_INT32 MaxFrameSize;
} IN_RC_PARAMS;

typedef enum _TH_SKIP_SCALE_ {
    TH_SKIP_0 = 0,
    TH_SKIP_12 = 1,
    TH_SKIP_24 = 2
} TH_SKIP_SCALE;

struct context_ENC_s {
    object_context_p obj_context; /* back reference */

    IMG_UINT32       NumCores; /* Number of encode cores in Penwell */
    IMG_UINT32       ParallelCores; /* Number of cores to use */
    IMG_INT32       BelowParamsBufIdx;
    IMG_INT16       RawWidth;
    IMG_INT16       RawHeight;

    IMG_UINT16       Width;  /* ~0xf & (RawWidth + 0xf)*/
    IMG_UINT16       Height;     /* ~0xf & (RawHeight + 0xf */

    IMG_UINT16       Slices;
    enum drm_pnw_topaz_codec eCodec;
    IMG_FORMAT      eFormat;
    unsigned int    FCode;
    IMG_RC_PARAMS   sRCParams;
    IMG_INT32       AccessUnitNum;
    IMG_UINT32      CmdCount;
    IMG_UINT32      LastSync[2][MAX_TOPAZ_CORES];
    IMG_INT32       FrmIdx;
    IMG_BOOL        SliceHeaderReady[MAX_SLICES_PER_PICTURE];

    IMG_UINT32      InBuffer; /* total coded data in Byte */

    IMG_INT16       HeightMinus16MinusLRBTopOffset;
    IMG_INT16       HeightMinus32MinusLRBTopOffset;
    IMG_INT16       HeightMinusLRB_TopAndBottom_OffsetsPlus16;
    IMG_INT16       HeightMinusLRBSearchHeight;
    IMG_UINT32      IPEControl;
    IMG_BOOL        SyncSequencer;

    IMG_UINT32       SliceToCore;  /* Core number to send current slice to */
    IMG_INT32       LastSliceNum[MAX_TOPAZ_CORES]; /* Slice number of last slice sent to core */

    object_surface_p    src_surface;
    object_surface_p    ref_surface;
    object_surface_p    dest_surface;/* reconstructed surface */
    object_buffer_p     coded_buf;

    /* save previous settings */
    object_surface_p    previous_src_surface;
    object_surface_p    previous_ref_surface;
    object_surface_p    previous_dest_surface; /* reconstructed surface */

    /* point to the place in cmdbuf following START_PIC, the initial_qp will fill into it later */
    uint32_t *initial_qp_in_cmdbuf;


    /* global topaz_params buffer shared by every cmdbuffer
     * it is because filling InParams for every MB is very time-consuming
     * and in most cases, we can reuse previous frames buffer
     */
    /* 0 and 1 are for in_parms, 2 is for bellow and above params*/

    struct psb_buffer_s topaz_in_params_I;
    struct psb_buffer_s topaz_in_params_P;

    struct psb_buffer_s topaz_below_params; /* MB MVs read & written by HW */
    struct psb_buffer_s topaz_above_params; /* MB MVs read & written by HW */

    uint32_t topaz_buffer_size;
    uint32_t in_params_size;
    uint32_t below_params_size;
    uint32_t above_params_size;

    /* offset in topaz_param buffer */
    uint32_t in_params_ofs;

    uint32_t below_params_ofs;
    uint32_t above_params_ofs;

    uint32_t pic_params_size;

    uint32_t header_buffer_size;

    uint32_t seq_header_ofs;
    uint32_t pic_header_ofs;
    uint32_t eoseq_header_ofs;
    uint32_t eostream_header_ofs;
    uint32_t slice_header_ofs;
    /*HRD SEI header*/
    uint32_t aud_header_ofs;
    uint32_t sei_buf_prd_ofs;
    uint32_t sei_pic_tm_ofs;
    uint32_t sei_pic_fpa_ofs;
    uint32_t sei_pic_data_size;
    
    uint32_t sliceparam_buffer_size;

    IN_RC_PARAMS in_params_cache; /* following frames reuse the first frame's IN_RC_PARAMS, cache it */
    TH_SKIP_SCALE THSkip;
    uint32_t pic_params_flags;

    VAEncSliceParameterBuffer *slice_param_cache;
    uint16_t slice_param_num;

    IMG_UINT16 MPEG4_vop_time_increment_resolution;

    /* saved information for FrameSkip redo */
    uint32_t MPEG4_vop_time_increment_frameskip;
    uint32_t MPEG4_picture_type_frameskip;
    uint8_t profile_idc;

    uint8_t force_idr_h264;

    /*If only one slice, it's zero. Otherwise it indicates size of parted coded_buf per slice*/
    uint32_t coded_buf_per_slice;

    /*JPEG context*/
    TOPAZSC_JPEG_ENCODER_CONTEXT *jpeg_ctx;

    /*H264 SEI_INSERTION*/
    IMG_BOOL bInserHRDParams;

    uint32_t max_slice_size;
    unsigned char *save_seq_header_p;

    IMG_INT16 num_air_mbs;
    IMG_INT16 air_threshold;

    uint32_t buffer_size;
    uint32_t initial_buffer_fullness;

    H264_VUI_PARAMS VUI_Params;
    IMG_BOOL bInsertVUI;

    /*H264 idr_pic_id field in slice header*/
    uint16_t idr_pic_id;
    unsigned char none_vcl_nal;

    /*Keep growing and won't be reset on IDR frame*/
    uint32_t raw_frame_count;
};

typedef struct context_ENC_s *context_ENC_p;

/*#define BELOW_PARAMS_SIZE 8*/

#define HEADER_SIZE             128*2

#define BELOW_PARAMS_SIZE       16
#define REGION_TYPE_2D          1
#define REGION_TYPE_LINEAR      0
#define REGION_TYPE_2DREF       3


#define MAX_RESIDUAL_PER_MB_H264        1260
#define ISINTER_FLAGS           0x1
#define ISH264_FLAGS            0x2
#define ISMPEG4_FLAGS           0x4
#define ISH263_FLAGS            0x8
#define DEBLOCK_FRAME           0x10
#define ISRC_FLAGS              0x20
#define ISCBR_FLAGS             0x40
#define ISVCM_FLAGS             0x80
#define ISVBR_FLAGS             0x100
#define ISCFS_FLAGS             0x200
#define INTERLEAVE_TARGET       0x400
#define FIRST_FRAME             0x800
#define SYNC_SEQUENCER          0x1000
#define DEBLOCK_SLICE           0x2000
#define DISABLE_FRAME_SKIPPING  0x4000 //!< Flag indicatest that frame skipping should be disabled.
#define DISABLE_BIT_STUFFING    0x8000 //!< Flag indicates that bit stuffing should be disabled.

#define SPE_EDGE_LEFT   1       /* ->bMinXRealEdge*/
#define SPE_EDGE_RIGHT  2       /* ->bMaxXRealEdge*/
#define SPE_EDGE_TOP    4   /* ->bMinYRealEdge*/
#define SPE_EDGE_BOTTOM 8       /* ->bMaxYRealEdge*/

#define BPH_SEI_NAL_INITIAL_CPB_REMOVAL_DELAY_SIZE 23
#define PTH_SEI_NAL_CPB_REMOVAL_DELAY_SIZE 23
#define PTH_SEI_NAL_DPB_OUTPUT_DELAY_SIZE 7

#define FW_TOKEN_USED 0
#define NOT_USED_BY_TOPAZ 0

typedef struct {
    /* Transferred into the input params area of the macroblock parameter structure*/
    IMG_BYTE    CurBlockAddr;
    IMG_BYTE    IPEMin[2];
    IMG_BYTE    IPEMax[2];
    IMG_BYTE    RealEdge;  /*bMinXRealEdge, bMaxXRealEdge, bMinXRealEdge and bMinYRealEdge*/
    /* Surrounding block availability */
    IMG_BYTE    MVValid;
    IMG_BYTE    ParamsValid;
    IMG_BYTE     bySliceQP;
    IMG_BYTE     bySliceQPC;

    IMG_BYTE     Reserved[6]; /* This is padding to make the transfers 16 bytes aligned*/
    /* Transferred into the SW communication section of the macroblock
     * parameter structure We shall EDMA the whole lot of this into eiob
     * in one go, and then use two TDMA's to put it into seperate locations
     * within the macroblock structure
     */
    IMG_UINT32   IPEControl;
    IMG_UINT32   SPEControl;
    IMG_UINT32   JMCompControl;
    IMG_UINT32   VLCControl;
} MTX_CURRENT_IN_PARAMS;

typedef struct { /* corresponding bytes inside the MB_IN structure: */
    IMG_BYTE    BlockSizes;              /****************/
    IMG_BYTE    IntraMode;               /*              */
    IMG_BYTE    Intra4x4ModesBottom[2];  /*              */
    IMG_BYTE    CodeType;                /*  [64 : 71]   */
    IMG_BYTE    Reserved2;               /*              */
    /*IMG_BYTE   SAD;*/
    IMG_BYTE    QPy;                     /*              */
    IMG_BYTE    QPc;                     /****************/

    IMG_BYTE    Reserved3[8];    /* This is padding to make the transfers 16 byte aligned*/

    IMG_UINT16  LumaSubBlockCoded;      /****************/
    IMG_BYTE    ChromaSubBlockCoded;    /*              */
    IMG_BYTE    LumaChromaDCCoded;      /*              */
    /*  [129 : 143] */
    IMG_BYTE    Lambda;                 /*              */
    IMG_BYTE    Reserved[3];            /*              */
    /*              */
    IMG_BYTE    Intra4x4ModeDeltas[8];  /****************/

    /* Motion vectors */
    IMG_UINT16  IntegerMV[16][2];      /* [207 : 144]  */
    /* input region from host */
} MTX_CURRENT_OUT_PARAMS;


typedef struct _PIC_PARAMS_ {
    IMG_UINT32          SrcYBase;
    IMG_UINT32          SrcUBase;
    IMG_UINT32          SrcVBase;
    IMG_UINT32          DstYBase;
    IMG_UINT32          DstUVBase;

    IMG_UINT16          SrcYStride;
    IMG_UINT16          SrcUVStride;
    IMG_UINT16          SrcYRowStride;
    IMG_UINT16          SrcUVRowStride;

    IMG_UINT16          DstYStride;
    IMG_UINT16          DstUVStride;

    IMG_UINT32          CodedBase;

    IMG_UINT32          BelowParamsInBase;
    IMG_UINT32          BelowParamsOutBase;

    IMG_UINT32          AboveParamsBase;

    IMG_UINT16          Width;
    IMG_UINT16          Height;
    IMG_UINT32          Flags;

    IN_RC_PARAMS        sInParams;

    IMG_UINT16          SearchWidth;
    IMG_UINT16          SearchHeight;

    IMG_UINT16          NumSlices;                      //!< Number of slices in the picture

    IMG_BOOL16 		IsPerSliceOutput;
    // SEI_INSERTION
    IMG_UINT64          ClockDivBitrate;
    IMG_UINT32          MaxBufferMultClockDivBitrate;
} PIC_PARAMS;

typedef enum {
    INTRA_MB_OFF = 0,
    INTRA_MB_AIR = 1,
    INTRA_MB_SCANNING = 2
} INTRA_MB_OPERATION_MODE;

/* ScanningIntraParams */
#define SCANNING_INTRA_MODE_MASK        (0x0000000f)
#define SCANNING_INTRA_MODE_SHIFT       (0)
#define SCANNING_INTRA_WIDTH_MASK       (0x000000f0)
#define SCANNING_INTRA_WIDTH_SHIFT      (4)
#define SCANNING_INTRA_WIDTH_MAX        (15)
#define SCANNING_INTRA_STEP_MASK        (0x00000f00)
#define SCANNING_INTRA_STEP_SHIFT       (8)
#define SCANNING_INTRA_STEP_MAX         (15)

#define MTX_FLAG_WB_SKIPFRAME   (0x00000002)
#define MTX_FLAG_RC_MASK                (0xFFFF0000)
#define MTX_FLAG_RC_PICPARAM    (0x00010000)
#define MTX_FLAG_RC_SLICEPARAM  (0x00020000)
#define MTX_FLAG_RC_BUPARAM             (0x00040000)
#define MTX_FLAG_RC_GETBUPARAM  (0x00080000)
#define MTX_FLAG_RC_FRM_LEN         (0x00100000)


/* This holds the data that is needed at the start of a slice
 */
typedef struct _SLICE_PARAMS_ {

    IMG_UINT16  SliceStartRowNum;
    IMG_UINT16  SliceHeight; /*Height of slice in pixels*/

    IMG_UINT32  RefYBase;
    IMG_UINT32  RefUVBase;
    IMG_UINT16  RefYStride;
    IMG_UINT16  RefUVStride;
    IMG_UINT16  RefYRowStride;
    IMG_UINT16  RefUVRowStride;

    IMG_UINT32  HostCtx;  /*Host context to insert into coded data header*/
    IMG_UINT32  Flags;
    IMG_UINT32  MaxSliceSize;
    IMG_UINT32  FCode; /*FCode value MP4 only*/

    /*Pointer to array of MTX_CURRENT_IN_PARAMS for each MB*/
    IMG_UINT32  InParamsBase;

    /*Maximum number of Adaptive intra refresh macroblocks for this slice*/
    IMG_INT16   NumAirMBs;

    /*Theshold value used in Adaptive intra refresh calculation.*/
    IMG_INT16   AirThreshold;
    IMG_UINT32  ScanningIntraParams;
} SLICE_PARAMS;

enum {
    CBR = 0,
    VBR
} eRCMode;

enum {
    EH263 = 0,
    EMpeg4 = 1,
    EH264 = 2,
    EHJpeg = 3
} eEncodingFormat;
#define VAEncSliceParameter_Equal(src, dst)                             \
    (((src)->start_row_number == (dst)->start_row_number)               \
     && ((src)->slice_height == (dst)->slice_height)                    \
     && ((src)->slice_flags.bits.is_intra == (dst)->slice_flags.bits.is_intra) \
     && ((src)->slice_flags.bits.disable_deblocking_filter_idc == (dst)->slice_flags.bits.disable_deblocking_filter_idc))

#define VAEncSliceParameter_LightEqual(src, dst)                             \
    (((src)->start_row_number == (dst)->start_row_number)               \
     && ((src)->slice_height == (dst)->slice_height)                    \
     && ((src)->slice_flags.bits.disable_deblocking_filter_idc == (dst)->slice_flags.bits.disable_deblocking_filter_idc))



#define SURFACE_INFO_SKIP_FLAG_SETTLED 0X80000000
#define GET_SURFACE_INFO_skipped_flag(psb_surface) ((int) (psb_surface->extra_info[5]))
#define SET_SURFACE_INFO_skipped_flag(psb_surface, value) psb_surface->extra_info[5] = (SURFACE_INFO_SKIP_FLAG_SETTLED | value)
#define CLEAR_SURFACE_INFO_skipped_flag(psb_surface) psb_surface->extra_info[5] = 0

VAStatus pnw_CreateContext(object_context_p obj_context,
                           object_config_p obj_config,
                           unsigned char is_JPEG);


void pnw__setup_rcdata(context_ENC_p ctx, PIC_PARAMS *psPicParams, IMG_RC_PARAMS *rc_params);

void pnw_DestroyContext(
    object_context_p obj_context
);

VAStatus pnw_BeginPicture(context_ENC_p ctx);
VAStatus pnw_EndPicture(context_ENC_p ctx);

void pnw_setup_slice_params(
    context_ENC_p  ctx, IMG_UINT16 YSliceStartPos,
    IMG_UINT16 SliceHeight, IMG_BOOL IsIntra,
    IMG_BOOL  VectorsValid, int bySliceQP);

IMG_UINT32 pnw__send_encode_slice_params(
    context_ENC_p ctx,
    IMG_BOOL IsIntra,
    IMG_UINT16 CurrentRow,
    IMG_UINT8  DeblockIDC,
    IMG_UINT32 FrameNum,
    IMG_UINT16 SliceHeight,
    IMG_UINT16 CurrentSlice);

VAStatus pnw_RenderPictureParameter(context_ENC_p ctx, int core);


void pnw_reset_encoder_params(context_ENC_p ctx);
unsigned int pnw__get_ipe_control(enum drm_pnw_topaz_codec  eEncodingFormat);


VAStatus pnw_set_bias(context_ENC_p ctx, int core);

