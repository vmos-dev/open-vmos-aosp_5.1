/*
* Copyright (c) 2009-2011 Intel Corporation.  All rights reserved.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#ifndef SEC_VIDEO_PARSER_H_
#define SEC_VIDEO_PARSER_H_

#include <stdint.h>

/* H264 start code values */
typedef enum _h264_nal_unit_type
{
    h264_NAL_UNIT_TYPE_unspecified = 0,
    h264_NAL_UNIT_TYPE_SLICE,
    h264_NAL_UNIT_TYPE_DPA,
    h264_NAL_UNIT_TYPE_DPB,
    h264_NAL_UNIT_TYPE_DPC,
    h264_NAL_UNIT_TYPE_IDR,
    h264_NAL_UNIT_TYPE_SEI,
    h264_NAL_UNIT_TYPE_SPS,
    h264_NAL_UNIT_TYPE_PPS,
    h264_NAL_UNIT_TYPE_Acc_unit_delimiter,
    h264_NAL_UNIT_TYPE_EOSeq,
    h264_NAL_UNIT_TYPE_EOstream,
    h264_NAL_UNIT_TYPE_filler_data,
    h264_NAL_UNIT_TYPE_SPS_extension,
    h264_NAL_UNIT_TYPE_ACP = 19,
    h264_NAL_UNIT_TYPE_Slice_extension = 20
} h264_nal_unit_type_t;

#define MAX_OP  16

enum dec_ref_pic_marking_flags {
    IDR_PIC_FLAG = 0,
    NO_OUTPUT_OF_PRIOR_PICS_FLAG,
    LONG_TERM_REFERENCE_FLAG,
    ADAPTIVE_REF_PIC_MARKING_MODE_FLAG
};

typedef struct _dec_ref_pic_marking_t {
    union {
        uint8_t flags;
        struct {
            uint8_t idr_pic_flag:1;
            uint8_t no_output_of_prior_pics_flag:1;
            uint8_t long_term_reference_flag:1;
            uint8_t adaptive_ref_pic_marking_mode_flag:1;
        };
    };
    struct {
        uint8_t memory_management_control_operation;
        union {
            struct {
                uint8_t difference_of_pic_nums_minus1;
            } op1;
            struct {
                uint8_t long_term_pic_num;
            } op2;
            struct {
                uint8_t difference_of_pic_nums_minus1;
                uint8_t long_term_frame_idx;
            } op3;
            struct {
                uint8_t max_long_term_frame_idx_plus1;
            } op4;
            struct {
                uint8_t long_term_frame_idx;
            } op6;
        };
    } op[MAX_OP];
} dec_ref_pic_marking_t;

enum slice_header_flags {
    FIELD_PIC_FLAG = 0,
    BOTTOM_FIELD_FLAG
};

typedef struct _slice_header_t {
    uint8_t nal_unit_type;
    uint8_t pps_id;
    uint8_t padding;    // TODO: padding needed because flags in secfw impl. is a big-endian uint16_t
    union {
        uint8_t flags;
        struct {
            uint8_t field_pic_flag:1;
            uint8_t bottom_field_flag:1;
        };
    };
    uint32_t first_mb_in_slice;
    uint32_t frame_num;
    uint16_t idr_pic_id;
    uint16_t pic_order_cnt_lsb;
    int32_t delta_pic_order_cnt[2];
    int32_t delta_pic_order_cnt_bottom;
} slice_header_t;

typedef struct {
    uint8_t type;
    uint32_t offset;
    uint8_t* data;
    uint32_t length;
    slice_header_t* slice_header;
} nalu_info_t;

typedef struct {
    uint32_t iv[4];
    uint32_t mode;
    uint32_t app_id;
} pavp_info_t;

#define MAX_NUM_NALUS   20

typedef struct {
    uint8_t* data;
    uint32_t length;
    pavp_info_t* pavp;
    dec_ref_pic_marking_t* dec_ref_pic_marking;
    uint32_t num_nalus;
    nalu_info_t nalus[MAX_NUM_NALUS];
} frame_info_t;

int parser_init(void);
int parse_frame(uint8_t* frame, uint32_t frame_size, uint8_t* nalu_data, uint32_t* nalu_data_size);

// DEBUG PRINTING
void print_slice_header(slice_header_t* slice_header);
void print_dec_ref_pic_marking(dec_ref_pic_marking_t* dec_ref_pic_marking);
void print_data_bytes(uint8_t* data, uint32_t count);
void print_nalu_data(uint8_t* nalu_data, uint32_t nalu_data_size);

// BYTESWAPPING
uint16_t byteswap_16(uint16_t word);
uint32_t byteswap_32(uint32_t dword);
void byteswap_slice_header(slice_header_t* slice_header);
void byteswap_dec_ref_pic_marking(dec_ref_pic_marking_t* dec_ref_pic_marking);
void byteswap_nalu_data(uint8_t* nalu_data, uint32_t nalu_data_size);

#endif /* SEC_VIDEO_PARSER_H_ */
