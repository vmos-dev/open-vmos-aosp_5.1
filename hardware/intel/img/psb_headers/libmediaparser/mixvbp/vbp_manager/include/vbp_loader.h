/*
 * Copyright (C) 2014 Intel Corporation. All rights reserved.
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

#ifndef VBP_LOADER_H
#define VBP_LOADER_H

#include <va/va.h>

#ifdef USE_HW_VP8
#include <va/va_dec_vp8.h>
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif


#ifndef uint8
typedef unsigned char uint8;
#endif
#ifndef uint16
typedef unsigned short uint16;
#endif
#ifndef uint32
typedef unsigned int uint32;
#endif
#ifndef int16
typedef short int16;
#endif

typedef void *Handle;

/*
 * MPEG-4 Part 2 data structure
 */

typedef struct _vbp_codec_data_mp42
{
    uint8  profile_and_level_indication;
    uint32 video_object_layer_width;
    uint32 video_object_layer_height;

    // 0 for unspecified, PAL/NTSC/SECAM
    uint8  video_format;

    // 0 short range, 1 full range
    uint8  video_range;

    // default 2 (unspecified), 1 for BT709.
    uint8  matrix_coefficients;

    uint8  short_video_header;

    // always exist for mpeg-4,
    uint8   aspect_ratio_info;
    uint8   par_width;
    uint8   par_height;

    // bit rate
    int bit_rate;
} vbp_codec_data_mp42;

typedef struct _vbp_slice_data_mp42
{
    uint8* buffer_addr;
    uint32 slice_offset;
    uint32 slice_size;
    VASliceParameterBufferMPEG4 slice_param;
} vbp_slice_data_mp42;

typedef struct _vbp_picture_data_mp42 vbp_picture_data_mp42;

struct _vbp_picture_data_mp42
{
    uint8 vop_coded;
    uint16 vop_time_increment;
    /* indicates if current buffer contains parameter for the first slice of the picture */
    uint8 new_picture_flag;
    VAPictureParameterBufferMPEG4 picture_param;
    vbp_slice_data_mp42 slice_data;

    vbp_picture_data_mp42* next_picture_data;
};

typedef struct _vbp_data_mp42
{
    vbp_codec_data_mp42 codec_data;
    VAIQMatrixBufferMPEG4 iq_matrix_buffer;

    uint32 number_picture_data;
    uint32 number_pictures;

    vbp_picture_data_mp42 *picture_data;

} vbp_data_mp42;

/*
 * H.264 data structure
 */

typedef struct _vbp_codec_data_h264
{
    uint8 pic_parameter_set_id;
    uint8 seq_parameter_set_id;

    uint8 profile_idc;
    uint8 level_idc;
    /*constraint flag sets (h.264 Spec v2009)*/
    uint8 constraint_set0_flag;
    uint8 constraint_set1_flag;
    uint8 constraint_set2_flag;
    uint8 constraint_set3_flag;
    uint8 constraint_set4_flag;

    uint8 num_ref_frames;
    uint8 gaps_in_frame_num_value_allowed_flag;

    uint8 frame_mbs_only_flag;
    uint8 mb_adaptive_frame_field_flag;

    int frame_width;
    int frame_height;

    uint8 vui_parameters_present_flag;

    /* aspect ratio */
    uint8 aspect_ratio_idc;
    uint16 sar_width;
    uint16 sar_height;

    /* cropping information */
    int crop_top;
    int crop_bottom;
    int crop_left;
    int crop_right;

    /* video fromat */

    // default 5 unspecified
    uint8 video_format;
    uint8 video_full_range_flag;

    // default 2 unspecified
    uint8 matrix_coefficients;

    uint8 pic_order_cnt_type;
    int log2_max_pic_order_cnt_lsb_minus4;

    int bit_rate;

    int has_slice;
} vbp_codec_data_h264;

typedef struct _vbp_slice_data_h264
{
    uint8* buffer_addr;

    uint32 slice_offset; /* slice data offset */

    uint32 slice_size; /* slice data size */

    uint8 nal_unit_type;

    VASliceParameterBufferH264 slc_parms;

} vbp_slice_data_h264;


typedef struct _vbp_picture_data_h264
{
    VAPictureParameterBufferH264* pic_parms;

    uint32 num_slices;

    vbp_slice_data_h264* slc_data;

} vbp_picture_data_h264;


typedef struct _vbp_data_h264
{
    /* rolling counter of buffers sent by vbp_parse */
    uint32 buf_number;

    uint32 num_pictures;

    /* if SPS has been received */
    uint8  has_sps;

    /* if PPS has been received */
    uint8  has_pps;

    uint8  new_sps;

    uint8  new_pps;

    vbp_picture_data_h264* pic_data;

    /**
        * do we need to send matrix to VA for each picture? If not, we need
        * a flag indicating whether it is updated.
        */
    VAIQMatrixBufferH264* IQ_matrix_buf;

    vbp_codec_data_h264* codec_data;

#ifdef USE_SLICE_HEADER_PARSING
    VAParsePictureParameterBuffer* pic_parse_buffer;
#endif

} vbp_data_h264;

/*
 * vc1 data structure
 */
typedef struct _vbp_codec_data_vc1
{
    /* Sequence layer. */
    uint8  PROFILE;
    uint8  LEVEL;
    uint8  POSTPROCFLAG;
    uint8  PULLDOWN;
    uint8  INTERLACE;
    uint8  TFCNTRFLAG;
    uint8  FINTERPFLAG;
    uint8  PSF;

    // default 2: unspecified
    uint8  MATRIX_COEF;

    /* Entry point layer. */
    uint8  BROKEN_LINK;
    uint8  CLOSED_ENTRY;
    uint8  PANSCAN_FLAG;
    uint8  REFDIST_FLAG;
    uint8  LOOPFILTER;
    uint8  FASTUVMC;
    uint8  EXTENDED_MV;
    uint8  DQUANT;
    uint8  VSTRANSFORM;
    uint8  OVERLAP;
    uint8  QUANTIZER;
    uint16 CODED_WIDTH;
    uint16 CODED_HEIGHT;
    uint8  EXTENDED_DMV;
    uint8  RANGE_MAPY_FLAG;
    uint8  RANGE_MAPY;
    uint8  RANGE_MAPUV_FLAG;
    uint8  RANGE_MAPUV;

    /* Others. */
    uint8  RANGERED;
    uint8  MAXBFRAMES;
    uint8  MULTIRES;
    uint8  SYNCMARKER;
    uint8  RNDCTRL;
    uint8  REFDIST;
    uint16 widthMB;
    uint16 heightMB;

    uint8  INTCOMPFIELD;
    uint8  LUMSCALE2;
    uint8  LUMSHIFT2;

    // aspect ratio

    // default unspecified
    uint8 ASPECT_RATIO;

    uint8 ASPECT_HORIZ_SIZE;
    uint8 ASPECT_VERT_SIZE;
    // bit rate
    int bit_rate;
} vbp_codec_data_vc1;

typedef struct _vbp_slice_data_vc1
{
    uint8 *buffer_addr;
    uint32 slice_offset;
    uint32 slice_size;
    VASliceParameterBufferVC1 slc_parms;     /* pointer to slice parms */
} vbp_slice_data_vc1;


typedef struct _vbp_picture_data_vc1
{
    uint32 picture_is_skipped;                /* VC1_PTYPE_SKIPPED is PTYPE is skipped. */
    VAPictureParameterBufferVC1 *pic_parms;   /* current parsed picture header */
    uint32 size_bitplanes;                    /* based on number of MBs */
    uint8 *packed_bitplanes;                  /* contains up to three bitplanes packed for libVA */
    uint32 num_slices;                        /* number of slices.  always at least one */
    vbp_slice_data_vc1 *slc_data;             /* pointer to array of slice data */
} vbp_picture_data_vc1;

typedef struct _vbp_data_vc1
{
    uint32 buf_number;                        /* rolling counter of buffers sent by vbp_parse */
    vbp_codec_data_vc1 *se_data;              /* parsed SH/EPs */

    uint32 num_pictures;

    vbp_picture_data_vc1* pic_data;
} vbp_data_vc1;

#ifdef USE_HW_VP8
typedef struct _vbp_codec_data_vp8
{
    uint8 frame_type;
    uint8 version_num;
    int show_frame;

    uint32 frame_width;
    uint32 frame_height;

    int refresh_alt_frame;
    int refresh_golden_frame;
    int refresh_last_frame;

    /* cropping information */
    int crop_top;
    int crop_bottom;
    int crop_left;
    int crop_right;

    int golden_copied;
    int altref_copied;
} vbp_codec_data_vp8;

typedef struct _vbp_slice_data_vp8
{
    uint8 *buffer_addr;
    uint32 slice_offset;
    uint32 slice_size;
    VASliceParameterBufferVP8 slc_parms;     /* pointer to slice parms */
} vbp_slice_data_vp8;

typedef struct _vbp_picture_data_vp8
{
    VAPictureParameterBufferVP8* pic_parms;   /* current parsed picture header */

    uint32 num_slices;                        /* number of slices.  always one for VP8 */
    vbp_slice_data_vp8 *slc_data;             /* pointer to array of slice data */
} vbp_picture_data_vp8;

typedef struct _vbp_data_vp8
{
    uint32 buf_number;                        /* rolling counter of buffers sent by vbp_parse */
    vbp_codec_data_vp8 *codec_data;

    uint32 num_pictures;

    vbp_picture_data_vp8* pic_data;

    VAProbabilityDataBufferVP8* prob_data;
    VAIQMatrixBufferVP8* IQ_matrix_buf;
} vbp_data_vp8;
#endif

enum _picture_type
{
    VC1_PTYPE_I,
    VC1_PTYPE_P,
    VC1_PTYPE_B,
    VC1_PTYPE_BI,
    VC1_PTYPE_SKIPPED
};

enum _vbp_parser_error
{
    VBP_OK,
    VBP_TYPE,
    VBP_LOAD,
    VBP_INIT,
    VBP_DATA,
    VBP_DONE,
    VBP_MEM,
    VBP_PARM,
    VBP_PARTIAL,
    VBP_MULTI,
    VBP_ERROR
};

enum _vbp_parser_type
{
    VBP_VC1,
    VBP_MPEG2,
    VBP_MPEG4,
    VBP_H264,
#ifdef USE_HW_VP8
    VBP_VP8,
#endif
#if (defined USE_AVC_SHORT_FORMAT || defined USE_SLICE_HEADER_PARSING)
    VBP_H264SECURE,
#endif
};


/*
 * open video bitstream parser to parse a specific media type.
 * @param  parser_type: one of the types defined in #vbp_parser_type
 * @param  hcontext: pointer to hold returned VBP context handle.
 * @return VBP_OK on success, anything else on failure.
 *
 */
uint32 vbp_open(uint32 parser_type, Handle *hcontext);

/*
 * close video bitstream parser.
 * @param hcontext: VBP context handle.
 * @returns VBP_OK on success, anything else on failure.
 *
 */
uint32 vbp_close(Handle hcontext);

/*
 * parse bitstream.
 * @param hcontext: handle to VBP context.
 * @param data: pointer to bitstream buffer.
 * @param size: size of bitstream buffer.
 * @param init_flag: 1 if buffer contains bitstream configuration data, 0 otherwise.
 * @return VBP_OK on success, anything else on failure.
 *
 */
uint32 vbp_parse(Handle hcontext, uint8 *data, uint32 size, uint8 init_data_flag);

/*
 * query parsing result.
 * @param hcontext: handle to VBP context.
 * @param data: pointer to hold a data blob that contains parsing result.
 * Structure of data blob is determined by the media type.
 * @return VBP_OK on success, anything else on failure.
 *
 */
uint32 vbp_query(Handle hcontext, void **data);


/*
 * flush any un-parsed bitstream.
 * @param hcontext: handle to VBP context.
 * @returns VBP_OK on success, anything else on failure.
 *
 */
uint32 vbp_flush(Handle hcontent);

#if (defined USE_AVC_SHORT_FORMAT || defined USE_SLICE_HEADER_PARSING)
/*
 * update the the vbp context using the new data
 * @param hcontext: handle to VBP context.
 * @param data: pointer to the new data buffer.
 * @param size: size of new data buffer.
 * @param data: pointer to hold a data blob that contains parsing result.
 * @returns VBP_OK on success, anything else on failure.
 *
*/
uint32 vbp_update(Handle hcontext, void *newdata, uint32 size, void **data);
#endif

#endif /* VBP_LOADER_H */
