/*--------------------------------------------------------------------------
Copyright (c) 2010-2012, Code Aurora Forum. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of Code Aurora nor
      the names of its contributors may be used to endorse or promote
      products derived from this software without specific prior written
      permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
--------------------------------------------------------------------------*/
#ifndef H264_UTILS_H
#define H264_UTILS_H

/*========================================================================

                                 O p e n M M
         U t i l i t i e s   a n d   H e l p e r   R o u t i n e s

*//** @file H264_Utils.h
This module contains H264 video decoder utilities and helper routines.

*//*====================================================================== */

/* =======================================================================

                     INCLUDE FILES FOR MODULE

========================================================================== */
#include <stdio.h>
#include "Map.h"
#include "qtypes.h"
#include "OMX_Core.h"
#include "OMX_QCOMExtns.h"

#define STD_MIN(x,y) (((x) < (y)) ? (x) : (y))

#define OMX_CORE_720P_HEIGHT 720
#define OMX_CORE_720P_WIDTH 1280

#define PANSCAN_HDLR

/* =======================================================================

                        DATA DECLARATIONS

========================================================================== */

/* -----------------------------------------------------------------------
** Constant / Define Declarations
** ----------------------------------------------------------------------- */
// Common format block header definitions
#define MT_VIDEO_META_STREAM_HEADER             0x00
#define MT_VIDEO_MEDIA_STREAM_HEADER            0x01
#define MT_VIDEO_META_MEDIA_STREAM_HEADER       0x02

// H.264 format block header definitions
#define MT_VIDEO_H264_ACCESS_UNIT_FORMAT        0x00
#define MT_VIDEO_H264_NAL_FORMT                 0x01
#define MT_VIDEO_H264_BYTE_FORMAT               0x02
#define MT_VIDEO_H264_BYTE_STREAM_FORMAT        0x00
#define MT_VIDEO_H264_NAL_UNIT_STREAM_FORMAT    0x01
#define MT_VIDEO_H264_FORMAT_BLOCK_HEADER_SIZE  18

// MPEG-4 format block header definitions
#define MT_VIDEO_MPEG4_VOP_FORMAT               0x00
#define MT_VIDEO_MPEG4_SLICE_FORMAT             0x01
#define MT_VIDEO_MPEG4_BYTE_FORMAT              0x02
#define MT_VIDEO_MPEG4_FORMAT_BLOCK_HEADER_SIZE 15

// H.263 format block header definitions
#define MT_VIDEO_H263_PICTURE_FORMAT            0x00
#define MT_VIDEO_H263_GOB_FORMAT                0x01
#define MT_VIDEO_H263_SLICE_STRUCTURED_FORMAT   0x02
#define MT_VIDEO_H263_BYTE_FORMAT               0x03
#define MT_VIDEO_H263_FORMAT_BLOCK_HEADER_SIZE  16

/* =======================================================================
**                          Function Declarations
** ======================================================================= */

/* -----------------------------------------------------------------------
** Type Declarations
** ----------------------------------------------------------------------- */

// This type is used when parsing an H.264 bitstream to collect H.264 NAL
// units that need to go in the meta data.
struct H264ParamNalu {
    uint32 picSetID;
    uint32 seqSetID;
    uint32 picOrderCntType;
    bool frameMbsOnlyFlag;
    bool picOrderPresentFlag;
    uint32 picWidthInMbsMinus1;
    uint32 picHeightInMapUnitsMinus1;
    uint32 log2MaxFrameNumMinus4;
    uint32 log2MaxPicOrderCntLsbMinus4;
    bool deltaPicOrderAlwaysZeroFlag;
    //std::vector<uint8> nalu;
    uint32 nalu;
    uint32 crop_left;
    uint32 crop_right;
    uint32 crop_top;
    uint32 crop_bot;
};
//typedef map<uint32, H264ParamNalu> H264ParamNaluSet;
typedef Map<uint32, H264ParamNalu *> H264ParamNaluSet;

typedef enum {
  NALU_TYPE_UNSPECIFIED = 0,
  NALU_TYPE_NON_IDR,
  NALU_TYPE_PARTITION_A,
  NALU_TYPE_PARTITION_B,
  NALU_TYPE_PARTITION_C,
  NALU_TYPE_IDR,
  NALU_TYPE_SEI,
  NALU_TYPE_SPS,
  NALU_TYPE_PPS,
  NALU_TYPE_ACCESS_DELIM,
  NALU_TYPE_EOSEQ,
  NALU_TYPE_EOSTREAM,
  NALU_TYPE_FILLER_DATA,
  NALU_TYPE_RESERVED,
} NALU_TYPE;

// NAL header information
typedef struct {
  uint32 nal_ref_idc;
  uint32 nalu_type;
  uint32 forbidden_zero_bit;
} NALU;

// This structure contains persistent information about an H.264 stream as it
// is parsed.
//struct H264StreamInfo {
//    H264ParamNaluSet pic;
//    H264ParamNaluSet seq;
//};

class extra_data_parser;

class RbspParser
/******************************************************************************
 ** This class is used to convert an H.264 NALU (network abstraction layer
 ** unit) into RBSP (raw byte sequence payload) and extract bits from it.
 *****************************************************************************/
{
public:
    RbspParser (const uint8 *begin, const uint8 *end);

    virtual ~RbspParser ();

    uint32 next ();
    void advance ();
    uint32 u (uint32 n);
    uint32 ue ();
    int32 se ();

private:
    const     uint8 *begin, *end;
    int32     pos;
    uint32    bit;
    uint32    cursor;
    bool      advanceNeeded;
};

class H264_Utils
{
public:
    H264_Utils();
    ~H264_Utils();
    void initialize_frame_checking_environment();
    void allocate_rbsp_buffer(uint32 inputBufferSize);
    bool isNewFrame(OMX_BUFFERHEADERTYPE *p_buf_hdr,
                    OMX_IN OMX_U32 size_of_nal_length_field,
                    OMX_OUT OMX_BOOL &isNewFrame);
    uint32 nalu_type;

private:
    boolean extract_rbsp(OMX_IN   OMX_U8  *buffer,
                         OMX_IN   OMX_U32 buffer_length,
                         OMX_IN   OMX_U32 size_of_nal_length_field,
                         OMX_OUT  OMX_U8  *rbsp_bistream,
                         OMX_OUT  OMX_U32 *rbsp_length,
                         OMX_OUT  NALU    *nal_unit);

    unsigned          m_height;
    unsigned          m_width;
    H264ParamNaluSet  pic;
    H264ParamNaluSet  seq;
    uint8             *m_rbspBytes;
    NALU              m_prv_nalu;
    bool              m_forceToStichNextNAL;
    bool              m_au_data;
};

class perf_metrics
{
  public:
    perf_metrics() :
      start_time(0),
      proc_time(0),
      active(false)
    {
    };
    ~perf_metrics() {};
    void start();
    void stop();
    void end(OMX_U32 units_cntr = 0);
    void reset();
    OMX_U64 processing_time_us();
  private:
    inline OMX_U64 get_act_time();
    OMX_U64 start_time;
    OMX_U64 proc_time;
    bool active;
};

#define EMULATION_PREVENTION_THREE_BYTE 0x03
#define MAX_CPB_COUNT     32
#define NO_PAN_SCAN_BIT   0x00000100
#define MAX_PAN_SCAN_RECT 3
#define VALID_TS(ts)      ((ts < LLONG_MAX)? true : false)
#define NALU_TYPE_VUI (NALU_TYPE_RESERVED + 1)

enum SEI_PAYLOAD_TYPE
{
  BUFFERING_PERIOD = 0,
  PIC_TIMING,
  PAN_SCAN_RECT,
  FILLER_PAYLOAD,
  USER_DATA_REGISTERED_ITU_T_T35,
  USER_DATA_UNREGISTERED,
  RECOVERY_POINT,
  DEC_REF_PIC_MARKING_REPETITION,
  SPARE_PIC,
  SCENE_INFO,
  SUB_SEQ_INFO,
  SUB_SEQ_LAYER_CHARACTERISTICS,
  SUB_SEQ_CHARACTERISTICS,
  FULL_FRAME_FREEZE,
  FULL_FRAME_FREEZE_RELEASE,
  FULL_FRAME_SNAPSHOT,
  PROGRESSIVE_REFINEMENT_SEGMENT_START,
  PROGRESSIVE_REFINEMENT_SEGMENT_END,
  SEI_PAYLOAD_FRAME_PACKING_ARRANGEMENT = 0x2D
};

typedef struct
{
  OMX_U32  cpb_cnt;
  OMX_U8   bit_rate_scale;
  OMX_U8   cpb_size_scale;
  OMX_U32  bit_rate_value[MAX_CPB_COUNT];
  OMX_U32  cpb_size_value[MAX_CPB_COUNT];
  OMX_U8   cbr_flag[MAX_CPB_COUNT];
  OMX_U8   initial_cpb_removal_delay_length;
  OMX_U8   cpb_removal_delay_length;
  OMX_U8   dpb_output_delay_length;
  OMX_U8   time_offset_length;
} h264_hrd_param;

typedef struct
{
  OMX_U32  aspect_ratio_idc;
  OMX_U32  aspect_ratio_x;
  OMX_U32  aspect_ratio_y;
} h264_aspect_ratio_info;

typedef struct
{
  OMX_U8   aspect_ratio_info_present_flag;
  h264_aspect_ratio_info aspect_ratio_info;
  OMX_U8   timing_info_present_flag;
  OMX_U32  num_units_in_tick;
  OMX_U32  time_scale;
  OMX_U8   fixed_frame_rate_flag;
  OMX_U8   nal_hrd_parameters_present_flag;
  h264_hrd_param   nal_hrd_parameters;
  OMX_U8   vcl_hrd_parameters_present_flag;
  h264_hrd_param   vcl_hrd_parameters;
  OMX_U8   low_delay_hrd_flag;
  OMX_U8   pic_struct_present_flag;
  OMX_S64  fixed_fps_prev_ts;
} h264_vui_param;

typedef struct
{
  OMX_U32 cpb_removal_delay;
  OMX_U32 dpb_output_delay;
  OMX_U8  pic_struct;
  OMX_U32 num_clock_ts;
  bool    clock_ts_flag;
  OMX_U8  ct_type;
  OMX_U32 nuit_field_based_flag;
  OMX_U8  counting_type;
  OMX_U8  full_timestamp_flag;
  OMX_U8  discontinuity_flag;
  OMX_U8  cnt_dropped_flag;
  OMX_U32 n_frames;
  OMX_U32 seconds_value;
  OMX_U32 minutes_value;
  OMX_U32 hours_value;
  OMX_S32 time_offset;
  bool    is_valid;
} h264_sei_pic_timing;

typedef struct
{
  OMX_U32  initial_cpb_removal_delay[MAX_CPB_COUNT];
  OMX_U32  initial_cpb_removal_delay_offset[MAX_CPB_COUNT];
  OMX_U32  au_cntr;
  OMX_S64  reference_ts;
  bool     is_valid;
} h264_sei_buf_period;

typedef struct
{
  OMX_U32  rect_id;
  OMX_U8   rect_cancel_flag;
  OMX_U32  cnt;
  OMX_S32  rect_left_offset[MAX_PAN_SCAN_RECT];
  OMX_S32  rect_right_offset[MAX_PAN_SCAN_RECT];
  OMX_S32  rect_top_offset[MAX_PAN_SCAN_RECT];
  OMX_S32  rect_bottom_offset[MAX_PAN_SCAN_RECT];
  OMX_U32  rect_repetition_period;
} h264_pan_scan;

#ifdef PANSCAN_HDLR
template <class NODE_STRUCT>
class omx_dl_list
{
public:
  omx_dl_list() { head = tail = NULL; } ;
  ~omx_dl_list() {};
  void add_multiple(NODE_STRUCT *data_arr, int data_num);
  NODE_STRUCT *remove_first();
  NODE_STRUCT *remove_last();
  void add_last(NODE_STRUCT *data_ptr);
  NODE_STRUCT *watch_first();
  NODE_STRUCT *watch_last();
private:
  NODE_STRUCT *head, *tail;
};

class panscan_handler
{
public:
  panscan_handler();
  ~panscan_handler();
  bool initialize(int num_data);
  h264_pan_scan *get_free();
  h264_pan_scan *get_populated(OMX_S64 frame_ts);
  void update_last(OMX_S64 frame_ts);
private:
  typedef struct PANSCAN_NODE
  {
    h264_pan_scan pan_scan_param;
    OMX_S64  start_ts, end_ts;
    bool active;
    PANSCAN_NODE *next, *prev;
  } PANSCAN_NODE;
  omx_dl_list<PANSCAN_NODE> panscan_used;
  omx_dl_list<PANSCAN_NODE> panscan_free;
  PANSCAN_NODE *panscan_data;
};

#if 1 // Debug panscan data

#define PRINT_PANSCAN_PARAM(H264_PARAM)
#define PRINT_PANSCAN_DATA(NODE)

#else

#define PRINT_PANSCAN_PARAM(H264_PARAM) \
do {\
  ALOGE("%s(): left_off(%ld) right_off(%ld) top_off(%ld) bottom_off(%ld)",\
    __FUNCTION__,\
    (H264_PARAM).rect_left_offset[0],\
    (H264_PARAM).rect_right_offset[0],\
    (H264_PARAM).rect_top_offset[0],\
    (H264_PARAM).rect_bottom_offset[0]);\
}while(0)

#define PRINT_PANSCAN_DATA(NODE) \
do {\
  if (NODE) {\
    ALOGE("%s(): PANSCAN DATA start_ts(%lld) end_ts(%lld)", __FUNCTION__,\
	  (NODE)->start_ts, (NODE)->end_ts);\
	PRINT_PANSCAN_PARAM(NODE->pan_scan_param);\
  }\
}while(0)

#endif // End debug panscan data

#endif

class h264_stream_parser
{
  public:
    h264_stream_parser();
    ~h264_stream_parser();
    void reset();
    void fill_pan_scan_data(OMX_QCOM_PANSCAN *dest_pan_scan, OMX_S64 timestamp);
    void fill_aspect_ratio_info(OMX_QCOM_ASPECT_RATIO *dest_aspect_ratio);
    void parse_nal(OMX_U8* data_ptr, OMX_U32 data_len,
                   OMX_U32 nal_type = NALU_TYPE_UNSPECIFIED,
                   bool enable_emu_sc = true);
    OMX_S64 process_ts_with_sei_vui(OMX_S64 timestamp);
    void get_frame_pack_data(OMX_QCOM_FRAME_PACK_ARRANGEMENT *frame_pack);
    bool is_mbaff();
    void get_frame_rate(OMX_U32 *frame_rate);
#ifdef PANSCAN_HDLR
    void update_panscan_data(OMX_S64 timestamp);
#endif
  private:
    void init_bitstream(OMX_U8* data, OMX_U32 size);
    OMX_U32 extract_bits(OMX_U32 n);
    inline bool more_bits();
    void read_word();
    OMX_U32 uev();
    OMX_S32 sev();
    OMX_S32 iv(OMX_U32 n_bits);
    void parse_sps();
    void parse_vui(bool vui_in_extradata = true);
    void aspect_ratio_info();
    void hrd_parameters(h264_hrd_param *hrd_param);
    void parse_sei();
    void sei_buffering_period();
    void sei_picture_timing();
    void sei_pan_scan();
    void scaling_list(OMX_U32 size_of_scaling_list);

    void print_pan_data(h264_pan_scan *pan_scan_param);
    void print_frame_pack();

    OMX_U32 get_nal_unit_type(OMX_U32 *nal_unit_type);
    OMX_S64 calculate_buf_period_ts(OMX_S64 timestamp);
    OMX_S64 calculate_fixed_fps_ts(OMX_S64 timestamp, OMX_U32 DeltaTfiDivisor);
    void parse_frame_pack();

    OMX_U32 curr_32_bit;
    OMX_U32 bits_read;
    OMX_U32 zero_cntr;
    OMX_U32 emulation_code_skip_cntr;
    OMX_U8* bitstream;
    OMX_U32 bitstream_bytes;
    OMX_U32 frame_rate;
    bool    emulation_sc_enabled;

    h264_vui_param vui_param;
    h264_sei_buf_period sei_buf_period;
    h264_sei_pic_timing sei_pic_timing;
#ifdef PANSCAN_HDLR
    panscan_handler *panscan_hdl;
#else
    h264_pan_scan panscan_param;
#endif
    OMX_QCOM_FRAME_PACK_ARRANGEMENT frame_packing_arrangement;
	bool 	mbaff_flag;
};

#endif /* H264_UTILS_H */
