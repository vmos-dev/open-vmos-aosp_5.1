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
 *    Edward Lin <edward.lin@intel.com>
 *
 */

/*
 * Description  DMA code for mtx Platform     : Generic
 */

#ifndef _TNG_HOSTHEADER_H_
#define _TNG_HOSTHEADER_H_


#include "img_types.h"

#define MAX_MVC_VIEWS       2
#define MVC_BASE_VIEW_IDX   0
#define NON_MVC_VIEW        (~0x0)

#define MVC_SPS_ID          1
#define MVC_PPS_ID          1

/* Structure contains QP parameters, used with the DoHeader() routine */
typedef struct {
    IMG_UINT32 H264_QP;
    IMG_UINT32 H263_MPG4_FrameQ_scale;
    IMG_UINT32 H263_MPG4_SliceQ_scale;
} MTX_QP_INFO;

typedef struct {
    IMG_UINT8       frameType;
    IMG_BOOL8       weighted_pred_flag;     // Corresponds to field in the pps
    IMG_UINT8               weighted_bipred_idc;
    IMG_UINT32      luma_log2_weight_denom;
    IMG_UINT32      chroma_log2_weight_denom;
    IMG_BOOL8       weight_flag[3][2]; // Y,Cb, Cr Support for 2 ref pictures on P, or 1 pic in each direction on B.
    IMG_INT32               weight[3][2];
    IMG_INT32               offset[3][2];
} WEIGHTED_PREDICTION_VALUES;


/* #include "topaz_vlc_regs.h" */

/* Allocating 32 words (128 bytes aligned to 8 bytes) */
#define MAX_HEADERSIZEWORDS (32)

/*****************************************************************************
 * @details    Enum describing partially coded header element types
 * @brief          Header element type
 ****************************************************************************/
typedef enum
{
    ELEMENT_STARTCODE_RAWDATA=0,    //!< Raw data that includes a start code
    ELEMENT_STARTCODE_MIDHDR,       //!< Start code in middle of header
    ELEMENT_RAWDATA,                //!< Raw data
    ELEMENT_QP,                     //!< Insert the H264 Picture Header QP parameter (no rawdata)
    ELEMENT_SQP,	            //!< Insert the H264 Slice Header QP parameter (no rawdata)
    ELEMENT_FRAMEQSCALE,            //!< Insert the H263/MPEG4 Frame Q_scale parameter (vob_quant field) (no rawdata)
    ELEMENT_SLICEQSCALE,            //!< Insert the H263/MPEG4 Slice Q_scale parameter (quant_scale field) (no rawdata)
    ELEMENT_INSERTBYTEALIGN_H264,   //!< Insert the byte align field for H264 (no rawdata)
    ELEMENT_INSERTBYTEALIGN_MPG4,   //!< Insert the byte align field for MPEG4(no rawdata)
    ELEMENT_INSERTBYTEALIGN_MPG2,   //!< Insert the byte align field for MPEG2(no rawdata)
    ELEMENT_VBV_MPG2,
    ELEMENT_TEMPORAL_REF_MPG2,
    ELEMENT_CURRMBNR,               //!< Insert the current macrloblock number for a slice.

    ELEMENT_FRAME_NUM,              //!< Insert frame_num field (used as ID for ref. pictures in H264)
    ELEMENT_TEMPORAL_REFERENCE,     //!< Insert Temporal Reference field (used as ID for ref. pictures in H263)
    ELEMENT_EXTENDED_TR,            //!< Insert Extended Temporal Reference field
    ELEMENT_IDR_PIC_ID,             //!< Insert idr_pic_id field (used to distinguish consecutive IDR frames)
    ELEMENT_PIC_ORDER_CNT,                   //!< Insert pic_order_cnt_lsb field (used for display ordering in H264)
    ELEMENT_GOB_FRAME_ID,                     //!< Insert gob_frame_id field (used for display ordering in H263)
    ELEMENT_VOP_TIME_INCREMENT,         //!< Insert vop_time_increment field (used for display ordering in MPEG4)

    ELEMENT_MODULO_TIME_BASE,            //!< modulo_time_base used in MPEG4 (depends on vop_time_increment_resolution)

    ELEMENT_BOTTOM_FIELD,                     //!< Insert bottom_field flag
    ELEMENT_SLICE_NUM,                            //!< Insert slice num (used for GOB headers in H263)
    ELEMENT_MPEG2_SLICE_VERTICAL_POS,   //!< Insert slice vertical pos (MPEG2 slice header)
    ELEMENT_MPEG2_IS_INTRA_SLICE,             //!< Insert 1 bit flag indicating if slice is Intra or not (MPEG2 slice header)
    ELEMENT_MPEG2_PICTURE_STRUCTURE,  //!< Insert 2 bit field indicating if the current header is for a frame picture (11), top field (01) or bottom field (10) - (MPEG2 picture header)
    ELEMENT_REFERENCE,                               //!< NAL header element. Specifies if this frame is used as reference
    ELEMENT_ADAPTIVE,                                   //!< Adaptive reference marking mode: this element presented only in reference pictures
    ELEMENT_DIRECT_SPATIAL_MV_FLAG,       //!< Spatial direct mode flag
    ELEMENT_NUM_REF_IDX_ACTIVE,               //!< Override active number of references, if required
    ELEMENT_REORDER_L0,                             //!< Reference list 0 reordering
    ELEMENT_REORDER_L1,                             //!< Reference list 1 reordering
    ELEMENT_TEMPORAL_ID,                            //!< Temporal ID of the picture, used for MVC header
    ELEMENT_ANCHOR_PIC_FLAG,                   //!< True if this picture is an anchor picture

    BPH_SEI_NAL_INITIAL_CPB_REMOVAL_DELAY,
    BPH_SEI_NAL_INITIAL_CPB_REMOVAL_DELAY_OFFSET,
    PTH_SEI_NAL_CPB_REMOVAL_DELAY,
    PTH_SEI_NAL_DPB_OUTPUT_DELAY,

    ELEMENT_SLICEWEIGHTEDPREDICTIONSTRUCT,
    ELEMENT_CUSTOM_QUANT
} HEADER_ELEMENT_TYPE;

typedef struct _MTX_HEADER_ELEMENT_ {
    HEADER_ELEMENT_TYPE Element_Type;
    IMG_UINT8 ui8Size;
    IMG_UINT8 aui8Bits;
} MTX_HEADER_ELEMENT;


typedef struct _MTX_HEADER_PARAMS_ {
    IMG_UINT32 ui32Elements;
    MTX_HEADER_ELEMENT asElementStream[MAX_HEADERSIZEWORDS-1];
} MTX_HEADER_PARAMS;

#define ELEMENTS_EMPTY 9999

/* H264 Structures
 */

/* Define some constants for the variable elements in the header stream */
typedef enum _SHPROFILES {
    SH_PROFILE_BP = 0,          //!< H.264 Baseline Profile
    SH_PROFILE_MP = 1,          //!< H.264 Main Profile
    SH_PROFILE_HP = 2,          //!< H.264 High Profile
    SH_PROFILE_H444P = 3      //!< H.264 High 4:4:4 Profile
} SH_PROFILE_TYPE;

/* Level number definitions (integer level numbers, non-intermediary only.. except level 1b) */
typedef enum _SHLEVELS {
    SH_LEVEL_10 = 10,
    SH_LEVEL_1B = 9,
    SH_LEVEL_11 = 11,
    SH_LEVEL_12 = 12,
    SH_LEVEL_13 = 13,
    SH_LEVEL_20 = 20,
    SH_LEVEL_21 = 21,
    SH_LEVEL_22 = 22,
    SH_LEVEL_30 = 30,
    SH_LEVEL_31 = 31,
    SH_LEVEL_32 = 32,
    SH_LEVEL_40 = 40,
    SH_LEVEL_41 = 41,
    SH_LEVEL_42 = 42,
    SH_LEVEL_50 = 50,
    SH_LEVEL_51 = 51
} SH_LEVEL_TYPE;


typedef enum _SLHP_SLICEFRAME_TYPE_ {
    SLHP_P_SLICEFRAME_TYPE,
    SLHP_B_SLICEFRAME_TYPE,
    SLHP_I_SLICEFRAME_TYPE,
    SLHP_SP_SLICEFRAME_TYPE,
    SLHP_SI_SLICEFRAME_TYPE,
    SLHP_IDR_SLICEFRAME_TYPE

} SLHP_SLICEFRAME_TYPE;

typedef enum _frame_template_type_ {
    IMG_FRAME_IDR = 0,
    IMG_FRAME_INTRA,
    IMG_FRAME_INTER_P,
    IMG_FRAME_INTER_B,
    IMG_FRAME_INTER_P_IDR,
    IMG_FRAME_UNDEFINED
} IMG_FRAME_TEMPLATE_TYPE;

/* This holds the data that is needed at the start of a slice
 */
typedef struct _SLICE_PARAMS_ {

    IMG_UINT32      ui32Flags;      //!< Flags for slice encode

    // the config registers, these are passed straigth through from drivers to hardware.
    // change per slice
    IMG_UINT32      ui32SliceConfig;        //!< Value to use for Slice Config register
    IMG_UINT32      ui32IPEControl;         //!< Value to use for IPEControl register
    IMG_UINT32      ui32SeqConfig;          //!< Value to use for Sequencer Config register

    IMG_FRAME_TEMPLATE_TYPE eTemplateType;  //!< Slice header template type
    MTX_HEADER_PARAMS sSliceHdrTmpl;        //!< Template of corresponding slice header
} SLICE_PARAMS;

/* Input parameters for the header generation
 * Some of the following data structures may have fields that are actually static..
 * may want to prune them down a bit later.
 */
typedef struct _H264_VUI_PARAMS_STRUC {
    IMG_UINT32 vui_flag;
    IMG_UINT32 Time_Scale;
    IMG_UINT32 num_units_in_tick;
    IMG_UINT32 bit_rate_value_minus1; /* bitrate/64)-1 */
    IMG_UINT32 cbp_size_value_minus1; /* (bitrate*1.5)/16 */
    IMG_UINT8 CBR;
    IMG_UINT8 initial_cpb_removal_delay_length_minus1;
    IMG_UINT8 cpb_removal_delay_length_minus1;
    IMG_UINT8 dpb_output_delay_length_minus1;
    IMG_UINT8 time_offset_length;
} H264_VUI_PARAMS;

typedef struct _H264_CROP_PARAMS_STRUCT_ {
    IMG_BOOL bClip;
    IMG_UINT16 ui16LeftCropOffset;
    IMG_UINT16 ui16RightCropOffset;
    IMG_UINT16 ui16TopCropOffset;
    IMG_UINT16 ui16BottomCropOffset;
} H264_CROP_PARAMS;

typedef struct {
    IMG_UINT8       ui8ScalingLists4x4[6][16];
    IMG_UINT8       ui8ScalingLists8x8[2][64];
    IMG_UINT32      ui32ListMask;
} H264_SCALING_MATRIX_PARAMS;

typedef struct _H264_SEQUENCE_HEADER_PARAMS_STRUC {
    SH_PROFILE_TYPE ucProfile;
    SH_LEVEL_TYPE ucLevel;
    IMG_UINT8   ucWidth_in_mbs_minus1;
    IMG_UINT8   ucHeight_in_maps_units_minus1;
    IMG_UINT8   log2_max_pic_order_cnt;
    IMG_UINT8   max_num_ref_frames;
    IMG_UINT8   gaps_in_frame_num_value;
    IMG_UINT8   ucFrame_mbs_only_flag;
    IMG_UINT8   VUI_Params_Present;
    IMG_UINT8   seq_scaling_matrix_present_flag;
    IMG_BOOL    bUseDefaultScalingList;
    IMG_BOOL    bIsLossless;
    H264_VUI_PARAMS VUI_Params;
} H264_SEQUENCE_HEADER_PARAMS;

typedef struct {
    IMG_UINT8       pic_parameter_set_id;
    IMG_UINT8       seq_parameter_set_id;
    IMG_UINT8       entropy_coding_mode_flag;
    IMG_UINT8       weighted_pred_flag;
    IMG_UINT8       weighted_bipred_idc;
    IMG_INT8        chroma_qp_index_offset;
    IMG_UINT8       constrained_intra_pred_flag;
    IMG_UINT8       transform_8x8_mode_flag;
    IMG_BOOL        pic_scaling_matrix_present_flag;
    IMG_BOOL        bUseDefaultScalingList;
    IMG_INT8        second_chroma_qp_index_offset;
} H264_PICTURE_HEADER_PARAMS;


typedef struct _H264_SLICE_HEADER_PARAMS_STRUC {
    IMG_UINT32           First_MB_Address;
    IMG_INT32            luma_log2_weight_denom;
    IMG_INT32            chroma_log2_weight_denom;
    IMG_INT32            luma_weight_l0[2];
    IMG_INT32            luma_offset_l0[2];
    IMG_INT32            chromaB_weight_l0[2];
    IMG_INT32            chromaB_offset_l0[2];
    IMG_INT32            chromaR_weight_l0[2];
    IMG_INT32            chromaR_offset_l0[2];
    IMG_UINT8            uRefLongTermRefNum[2];
    IMG_INT8             diff_ref_pic_num[2]; //when non-zero reorders reference pic list
    IMG_UINT16           ui16MvcViewIdx;
    IMG_UINT8            ui8Start_Code_Prefix_Size_Bytes;
    IMG_UINT8            Frame_Num_DO;
    IMG_UINT8            Idr_Pic_Id;
    IMG_UINT8            log2_max_pic_order_cnt;
    IMG_UINT8            Picture_Num_DO;
    IMG_UINT8            Disable_Deblocking_Filter_Idc;
    IMG_UINT8            num_ref_idx_l0_active_minus1;
    IMG_UINT8            weighted_bipred_idc;
    IMG_UINT8            uLongTermRefNum;
    IMG_INT8             iDebAlphaOffsetDiv2;
    IMG_INT8             iDebBetaOffsetDiv2;
    SLHP_SLICEFRAME_TYPE SliceFrame_Type;
    IMG_BOOL             bPiCInterlace;
    IMG_BOOL             bFieldType;
    IMG_BOOL             bReferencePicture;
    IMG_BOOL             direct_spatial_mv_pred_flag;
    IMG_BOOL             weighted_pred_flag;    // Corresponds to field in the pps
    IMG_BOOL             chroma_weight_l0_flag[2];
    IMG_BOOL             luma_weight_l0_flag[2]; // Support for 2 ref pictures on P, or 1 pic in each direction on B.
    IMG_BOOL             bIsLongTermRef;
    IMG_BOOL             bRefIsLongTermRef[2]; //Long term reference info for reference frames
} H264_SLICE_HEADER_PARAMS;

/* MPEG4 Structures
 */
typedef enum _MPEG4_PROFILE {
    SP = 1,
    ASP = 3
} MPEG4_PROFILE_TYPE;

typedef enum _FIXED_VOP_TIME_ENUM {
    _30FPS = 1,
    _15FPS = 2,
    _10FPS = 3
} FIXED_VOP_TIME_TYPE;

typedef struct _VBVPARAMS_STRUC {
    IMG_UINT32  First_half_bit_rate;
    IMG_UINT32  Latter_half_bit_rate;
    IMG_UINT32  First_half_vbv_buffer_size;
    IMG_UINT32  Latter_half_vbv_buffer_size;
    IMG_UINT32  First_half_vbv_occupancy;
    IMG_UINT32  Latter_half_vbv_occupancy;
} VBVPARAMS;


/*
 * H263 Structures
 */

typedef enum _VOP_CODING_ENUM {
    I_FRAME = 0,
    P_FRAME = 1
} VOP_CODING_TYPE, H263_PICTURE_CODING_TYPE;

typedef enum _SEARCH_RANGE_ENUM {
    PLUSMINUS_32 = 2,
    PLUSMINUS_64 = 3,
    FCODE_EQ_4 = 4
}  SEARCH_RANGE_TYPE;

typedef enum _H263_SOURCE_FORMAT_ENUM {
    _128x96_SubQCIF = 1,
    _176x144_QCIF = 2,
    _352x288_CIF = 3,
    _704x576_4CIF = 4
} H263_SOURCE_FORMAT_TYPE;


#define SIZEINBITS(a) (sizeof(a)*8)

/* H264 header preparation */

void tng__H264ES_prepare_sequence_header(
    void *pHeaderMemory,
    H264_VUI_PARAMS *psVUI_Params,
    H264_CROP_PARAMS *psCropParams,
    IMG_UINT16 ui16PictureWidth,
    IMG_UINT16 ui16PictureHeight,
    IMG_UINT32 ui32CustomQuantMask,
    IMG_UINT8 ui8ProfileIdc,
    IMG_UINT8 ui8LevelIdc,
    IMG_UINT8 ui8FieldCount,
    IMG_UINT8 ui8MaxNumRefFrames,
    IMG_BOOL  bPpsScaling,
    IMG_BOOL  bUseDefaultScalingList,
    IMG_BOOL  bEnableLossless,
    IMG_BOOL  bASO
);

void tng__H264ES_prepare_picture_header(
    void *pHeaderMemory,
    IMG_BOOL    bCabacEnabled,
    IMG_BOOL    b_8x8transform,
    IMG_BOOL    bIntraConstrained,
    IMG_INT8    i8CQPOffset,
    IMG_BOOL    bWeightedPrediction,
    IMG_UINT8   ui8WeightedBiPred,
    IMG_BOOL    bMvcPPS,
    IMG_BOOL    bScalingMatrix,
    IMG_BOOL    bScalingLists
);

void tng__H264_prepare_slice_header(
    IMG_UINT32 *pHeaderMemory,
    IMG_BOOL        bIntraSlice,
    IMG_BOOL        bInterBSlice,
    IMG_BOOL        bMultiRef,
    IMG_UINT8       ui8DisableDeblockingFilterIDC,
    IMG_UINT32      ui32DisplayFrameNumber,
    IMG_UINT32      ui32FrameNumId,
    IMG_UINT32      uiFirst_MB_Address,
    IMG_UINT32      uiMBSkipRun,
    IMG_BOOL        bCabacEnabled,
    IMG_BOOL        bIsInterlaced,
    IMG_UINT8       ui8FieldNum,
    WEIGHTED_PREDICTION_VALUES *pWeightedSetup,
    IMG_BOOL        bIsLongTermRef);

/* MPEG4 header preparation */
void tng__MPEG4_prepare_sequence_header(
    void *pHeaderMemory,
    IMG_BOOL bBFrame,
    MPEG4_PROFILE_TYPE sProfile,
    IMG_UINT8 Profile_and_level_indication,
    FIXED_VOP_TIME_TYPE sFixed_vop_time_increment,
    IMG_UINT32 Picture_Width_Pixels,
    IMG_UINT32 Picture_Height_Pixels,
    VBVPARAMS * psVBVParams,
    IMG_UINT32 VopTimeResolution);

void tng__MPEG4_prepare_vop_header(
    IMG_UINT32 *pHeaderMem,
    IMG_BOOL bIsVOP_coded,
    IMG_UINT32 VOP_time_increment,
    IMG_UINT8 sSearch_range,
    IMG_UINT8 eVop_Coding_Type,
    IMG_UINT32 VopTimeResolution);


/* H263 header preparation */
void tng__H263_prepare_sequence_header(
    IMG_UINT32 *pHeaderMem,
    IMG_UINT8 Profile_and_level_indication);

void tng__H263_prepare_picture_header(
    IMG_UINT32 *pHeaderMem,
    IMG_UINT8 Temporal_Ref,
    H263_PICTURE_CODING_TYPE PictureCodingType,
    H263_SOURCE_FORMAT_TYPE SourceFormatType,
    IMG_UINT8 FrameRate,
    IMG_UINT16 PictureWidth,
    IMG_UINT16 PictureHeigth);

void tng__H263_notforsims_prepare_video_pictureheader(
    MTX_HEADER_PARAMS* pMTX_Header,
    H263_PICTURE_CODING_TYPE ePictureCodingType,
    H263_SOURCE_FORMAT_TYPE eSourceFormatType,
    IMG_UINT8 ui8FrameRate,
    IMG_UINT32 ui32PictureWidth,
    IMG_UINT32 ui32PictureHeigth);

void tng__H263_prepare_GOBslice_header(
    IMG_UINT32 *pHeaderMem,
    IMG_UINT8 GOBNumber,
    IMG_UINT8 GOBFrameId);

void tng__H264ES_prepare_AUD_header(unsigned char *virtual_addr);

void tng__H264ES_prepare_SEI_buffering_period_header(
    unsigned char *virtual_addr,
    IMG_UINT8 ui8NalHrdBpPresentFlag,
    IMG_UINT8 ui8nal_cpb_cnt_minus1,
    IMG_UINT8 ui8nal_initial_cpb_removal_delay_length,
    IMG_UINT32 ui32nal_initial_cpb_removal_delay,
    IMG_UINT32 ui32nal_initial_cpb_removal_delay_offset,
    IMG_UINT8 ui8VclHrdBpPresentFlag,
    IMG_UINT32 ui32vcl_initial_cpb_removal_delay,
    IMG_UINT32 ui32vcl_initial_cpb_removal_delay_offset);

void tng__H264ES_prepare_SEI_picture_timing_header(
    unsigned char *virtual_addr,
    IMG_UINT8 ui8CpbDpbDelaysPresentFlag,
    IMG_UINT32 ui32cpb_removal_delay_length_minus1,
    IMG_UINT32 ui32dpb_output_delay_length_minus1,
    IMG_UINT32 ui32cpb_removal_delay,
    IMG_UINT32 ui32dpb_output_delay,
    IMG_UINT8 ui8pic_struct_present_flag,
    IMG_UINT8 ui8pic_struct,
    IMG_UINT8 ui8NumClockTS,
    IMG_UINT8 *aui8clock_timestamp_flag,
    IMG_UINT8 ui8full_timestamp_flag,
    IMG_UINT8 ui8seconds_flag,
    IMG_UINT8 ui8minutes_flag,
    IMG_UINT8 ui8hours_flag,
    IMG_UINT8 ui8seconds_value,
    IMG_UINT8 ui8minutes_value,
    IMG_UINT8 ui8hours_value,
    IMG_UINT8 ui8ct_type,
    IMG_UINT8 ui8nuit_field_based_flag,
    IMG_UINT8 ui8counting_type,
    IMG_UINT8 ui8discontinuity_flag,
    IMG_UINT8 ui8cnt_dropped_flag,
    IMG_UINT8 ui8n_frames,
    IMG_UINT8 ui8time_offset_length,
    IMG_INT32 i32time_offset);

void tng__H264ES_notforsims_prepare_sliceheader(
    IMG_UINT8       *slice_mem_p,
    IMG_UINT32      ui32SliceType,
    IMG_UINT8       ui8DisableDeblockingFilterIDC,
    IMG_UINT32      uiFirst_MB_Address,
    IMG_UINT32      uiMBSkipRun,
    IMG_BOOL        bCabacEnabled,
    IMG_BOOL        bIsInterlaced,
    IMG_UINT16      ui16MvcViewIdx,
    IMG_BOOL        bIsLongTermRef);

void tng__H264ES_prepare_mvc_sequence_header(
    void *pHeaderMemory,
    H264_CROP_PARAMS *psCropParams,
    IMG_UINT16 ui16PictureWidth,
    IMG_UINT16 ui16PictureHeight,
    IMG_UINT32 ui32CustomQuantMask,
    IMG_UINT8 ui8ProfileIdc,
    IMG_UINT8 ui8LevelIdc,
    IMG_UINT8 ui8FieldCount,
    IMG_UINT8 ui8MaxNumRefFrames,
    IMG_BOOL  bPpsScaling,
    IMG_BOOL  bUseDefaultScalingList,
    IMG_BOOL  bEnableLossless,
    IMG_BOOL  bASO);

void tng__H263ES_notforsims_prepare_gobsliceheader(IMG_UINT8 *slice_mem_p);
void tng__MPEG2_prepare_sliceheader(IMG_UINT8 *slice_mem_p);
void tng__MPEG4_notforsims_prepare_vop_header(
    MTX_HEADER_PARAMS* pMTX_Header,
    IMG_BOOL bIsVOP_coded,
    SEARCH_RANGE_TYPE eSearch_range,
    VOP_CODING_TYPE eVop_Coding_Type);

#endif /* _TNG_HOSTHEADER_H_ */


