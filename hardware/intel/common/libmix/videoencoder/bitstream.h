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

#ifndef __BITSTREAM_H__
#define __BITSTREAM_H__

#include <VideoEncoderBase.h>
#include <assert.h>

struct bitstream {
    unsigned int *buffer;
    int bit_offset;
    int max_size_in_dword;
};

#define BITSTREAM_ALLOCATE_STEPPING     4096

static unsigned int va_swap32(unsigned int val)
{
    unsigned char *pval = (unsigned char *)&val;

    return ((pval[0] << 24)     |
            (pval[1] << 16)     |
            (pval[2] << 8)      |
            (pval[3] << 0));
}

static void bitstream_start(bitstream *bs)
{
    bs->max_size_in_dword = BITSTREAM_ALLOCATE_STEPPING;
    bs->buffer = (unsigned int*)calloc(bs->max_size_in_dword * sizeof(int), 1);
    bs->bit_offset = 0;
}

static void bitstream_end(bitstream *bs)
{
    int pos = (bs->bit_offset >> 5);
    int bit_offset = (bs->bit_offset & 0x1f);
    int bit_left = 32 - bit_offset;

    if (bit_offset) {
        bs->buffer[pos] = va_swap32((bs->buffer[pos] << bit_left));
    }
}

static void bitstream_put_ui(bitstream *bs, unsigned int val, int size_in_bits)
{
    int pos = (bs->bit_offset >> 5);
    int bit_offset = (bs->bit_offset & 0x1f);
    int bit_left = 32 - bit_offset;

    if (!size_in_bits)
        return;

    bs->bit_offset += size_in_bits;

    if (bit_left > size_in_bits) {
        bs->buffer[pos] = (bs->buffer[pos] << size_in_bits | val);
    } else {
        size_in_bits -= bit_left;
        bs->buffer[pos] = (bs->buffer[pos] << bit_left) | (val >> size_in_bits);
        bs->buffer[pos] = va_swap32(bs->buffer[pos]);

        if (pos + 1 == bs->max_size_in_dword) {
            bs->max_size_in_dword += BITSTREAM_ALLOCATE_STEPPING;
            bs->buffer = (unsigned int*)realloc(bs->buffer, bs->max_size_in_dword * sizeof(unsigned int));
            if (bs->buffer == NULL)
                abort();
        }

        bs->buffer[pos + 1] = val;
    }
}

static void bitstream_put_ue(bitstream *bs, unsigned int val)
{
    int size_in_bits = 0;
    int tmp_val = ++val;

    while (tmp_val) {
        tmp_val >>= 1;
        size_in_bits++;
    }

    bitstream_put_ui(bs, 0, size_in_bits - 1); // leading zero
    bitstream_put_ui(bs, val, size_in_bits);
}

static void bitstream_put_se(bitstream *bs, int val)
{
    unsigned int new_val;

    if (val <= 0)
        new_val = -2 * val;
    else
        new_val = 2 * val - 1;

    bitstream_put_ue(bs, new_val);
}

static void bitstream_byte_aligning(bitstream *bs, int bit)
{
    int bit_offset = (bs->bit_offset & 0x7);
    int bit_left = 8 - bit_offset;
    int new_val;

    if (!bit_offset)
        return;

    assert(bit == 0 || bit == 1);

    if (bit)
        new_val = (1 << bit_left) - 1;
    else
        new_val = 0;

    bitstream_put_ui(bs, new_val, bit_left);
}

static void rbsp_trailing_bits(bitstream *bs)
{
    bitstream_put_ui(bs, 1, 1);
    bitstream_byte_aligning(bs, 0);
}

static void nal_start_code_prefix(bitstream *bs)
{
    bitstream_put_ui(bs, 0x00000001, 32);
}

static void nal_header(bitstream *bs, int nal_ref_idc, int nal_unit_type)
{
    bitstream_put_ui(bs, 0, 1);                /* forbidden_zero_bit: 0 */
    bitstream_put_ui(bs, nal_ref_idc, 2);
    bitstream_put_ui(bs, nal_unit_type, 5);
}

#define NAL_REF_IDC_NONE        0
#define NAL_REF_IDC_LOW         1
#define NAL_REF_IDC_MEDIUM      2
#define NAL_REF_IDC_HIGH        3

#define NAL_NON_IDR             1
#define NAL_IDR                 5
#define NAL_SPS                 7
#define NAL_PPS                 8
#define NAL_SEI			6

#define SLICE_TYPE_P            0
#define SLICE_TYPE_B            1
#define SLICE_TYPE_I            2

#define ENTROPY_MODE_CAVLC      0
#define ENTROPY_MODE_CABAC      1

#define PROFILE_IDC_BASELINE    66
#define PROFILE_IDC_MAIN        77
#define PROFILE_IDC_HIGH        100

static void sps_rbsp(bitstream *bs, VAProfile profile, int frame_bit_rate, VAEncSequenceParameterBufferH264 *seq_param)
{
    int profile_idc = 0;
    int constraint_set_flag = 0;

    if (profile == VAProfileH264High) {
        profile_idc = PROFILE_IDC_HIGH;
        constraint_set_flag |= (1 << 3); /* Annex A.2.4 */
    }
    else if (profile == VAProfileH264Main) {
        profile_idc = PROFILE_IDC_MAIN;
        constraint_set_flag |= (1 << 1); /* Annex A.2.2 */
    } else {
        profile_idc = PROFILE_IDC_BASELINE;
        constraint_set_flag |= (1 << 0); /* Annex A.2.1 */
    }

    bitstream_put_ui(bs, profile_idc, 8);               /* profile_idc */
    bitstream_put_ui(bs, !!(constraint_set_flag & 1), 1);                         /* constraint_set0_flag */
    bitstream_put_ui(bs, !!(constraint_set_flag & 2), 1);                         /* constraint_set1_flag */
    bitstream_put_ui(bs, !!(constraint_set_flag & 4), 1);                         /* constraint_set2_flag */
    bitstream_put_ui(bs, !!(constraint_set_flag & 8), 1);                         /* constraint_set3_flag */
    bitstream_put_ui(bs, 0, 4);                         /* reserved_zero_4bits */
    bitstream_put_ui(bs, seq_param->level_idc, 8);      /* level_idc */
    bitstream_put_ue(bs, seq_param->seq_parameter_set_id);      /* seq_parameter_set_id */

    if ( profile_idc == PROFILE_IDC_HIGH) {
        bitstream_put_ue(bs, 1);        /* chroma_format_idc = 1, 4:2:0 */
        bitstream_put_ue(bs, 0);        /* bit_depth_luma_minus8 */
        bitstream_put_ue(bs, 0);        /* bit_depth_chroma_minus8 */
        bitstream_put_ui(bs, 0, 1);     /* qpprime_y_zero_transform_bypass_flag */
        bitstream_put_ui(bs, 0, 1);     /* seq_scaling_matrix_present_flag */
    }

    bitstream_put_ue(bs, seq_param->seq_fields.bits.log2_max_frame_num_minus4); /* log2_max_frame_num_minus4 */
    bitstream_put_ue(bs, seq_param->seq_fields.bits.pic_order_cnt_type);        /* pic_order_cnt_type */

    if (seq_param->seq_fields.bits.pic_order_cnt_type == 0)
        bitstream_put_ue(bs, seq_param->seq_fields.bits.log2_max_pic_order_cnt_lsb_minus4);     /* log2_max_pic_order_cnt_lsb_minus4 */
    else {
        assert(0);
    }

    bitstream_put_ue(bs, seq_param->max_num_ref_frames);        /* num_ref_frames */
    bitstream_put_ui(bs, 0, 1);                                 /* gaps_in_frame_num_value_allowed_flag */

    bitstream_put_ue(bs, seq_param->picture_width_in_mbs - 1);  /* pic_width_in_mbs_minus1 */
    bitstream_put_ue(bs, seq_param->picture_height_in_mbs - 1); /* pic_height_in_map_units_minus1 */
    bitstream_put_ui(bs, seq_param->seq_fields.bits.frame_mbs_only_flag, 1);    /* frame_mbs_only_flag */

    if (!seq_param->seq_fields.bits.frame_mbs_only_flag) {
        assert(0);
    }

    bitstream_put_ui(bs, seq_param->seq_fields.bits.direct_8x8_inference_flag, 1);      /* direct_8x8_inference_flag */
    bitstream_put_ui(bs, seq_param->frame_cropping_flag, 1);            /* frame_cropping_flag */

    if (seq_param->frame_cropping_flag) {
        bitstream_put_ue(bs, seq_param->frame_crop_left_offset);        /* frame_crop_left_offset */
        bitstream_put_ue(bs, seq_param->frame_crop_right_offset);       /* frame_crop_right_offset */
        bitstream_put_ue(bs, seq_param->frame_crop_top_offset);         /* frame_crop_top_offset */
        bitstream_put_ue(bs, seq_param->frame_crop_bottom_offset);      /* frame_crop_bottom_offset */
    }

    if ( frame_bit_rate < 0 ) {
        bitstream_put_ui(bs, 0, 1); /* vui_parameters_present_flag */
    } else {
        bitstream_put_ui(bs, 1, 1); /* vui_parameters_present_flag */
        bitstream_put_ui(bs, 0, 1); /* aspect_ratio_info_present_flag */
        bitstream_put_ui(bs, 0, 1); /* overscan_info_present_flag */
        bitstream_put_ui(bs, 0, 1); /* video_signal_type_present_flag */
        bitstream_put_ui(bs, 0, 1); /* chroma_loc_info_present_flag */
        bitstream_put_ui(bs, 1, 1); /* timing_info_present_flag */
        {
            bitstream_put_ui(bs, 15, 32);
            bitstream_put_ui(bs, 900, 32);
            bitstream_put_ui(bs, 1, 1);
        }
        bitstream_put_ui(bs, 1, 1); /* nal_hrd_parameters_present_flag */
        {
            // hrd_parameters
            bitstream_put_ue(bs, 0);    /* cpb_cnt_minus1 */
            bitstream_put_ui(bs, 4, 4); /* bit_rate_scale */
            bitstream_put_ui(bs, 6, 4); /* cpb_size_scale */

            bitstream_put_ue(bs, frame_bit_rate - 1); /* bit_rate_value_minus1[0] */
            bitstream_put_ue(bs, frame_bit_rate*8 - 1); /* cpb_size_value_minus1[0] */
            bitstream_put_ui(bs, 1, 1);  /* cbr_flag[0] */

            bitstream_put_ui(bs, 23, 5);   /* initial_cpb_removal_delay_length_minus1 */
            bitstream_put_ui(bs, 23, 5);   /* cpb_removal_delay_length_minus1 */
            bitstream_put_ui(bs, 23, 5);   /* dpb_output_delay_length_minus1 */
            bitstream_put_ui(bs, 23, 5);   /* time_offset_length  */
        }
        bitstream_put_ui(bs, 0, 1);   /* vcl_hrd_parameters_present_flag */
        bitstream_put_ui(bs, 0, 1);   /* low_delay_hrd_flag */

        bitstream_put_ui(bs, 0, 1); /* pic_struct_present_flag */
        bitstream_put_ui(bs, 0, 1); /* bitstream_restriction_flag */
    }

    rbsp_trailing_bits(bs);     /* rbsp_trailing_bits */
}

static void pps_rbsp(bitstream *bs, VAEncPictureParameterBufferH264 *pic_param)
{

    bitstream_put_ue(bs, pic_param->pic_parameter_set_id);      /* pic_parameter_set_id */
    bitstream_put_ue(bs, pic_param->seq_parameter_set_id);      /* seq_parameter_set_id */

    bitstream_put_ui(bs, pic_param->pic_fields.bits.entropy_coding_mode_flag, 1);  /* entropy_coding_mode_flag */

    bitstream_put_ui(bs, 0, 1);                         /* pic_order_present_flag: 0 */

    bitstream_put_ue(bs, 0);                            /* num_slice_groups_minus1 */

    bitstream_put_ue(bs, pic_param->num_ref_idx_l0_active_minus1);      /* num_ref_idx_l0_active_minus1 */
    bitstream_put_ue(bs, pic_param->num_ref_idx_l1_active_minus1);      /* num_ref_idx_l1_active_minus1 1 */

    bitstream_put_ui(bs, pic_param->pic_fields.bits.weighted_pred_flag, 1);     /* weighted_pred_flag: 0 */
    bitstream_put_ui(bs, pic_param->pic_fields.bits.weighted_bipred_idc, 2);	/* weighted_bipred_idc: 0 */

    bitstream_put_se(bs, pic_param->pic_init_qp - 26);  /* pic_init_qp_minus26 */
    bitstream_put_se(bs, 0);                            /* pic_init_qs_minus26 */
    bitstream_put_se(bs, 0);                            /* chroma_qp_index_offset */

    bitstream_put_ui(bs, pic_param->pic_fields.bits.deblocking_filter_control_present_flag, 1); /* deblocking_filter_control_present_flag */
    bitstream_put_ui(bs, 0, 1);                         /* constrained_intra_pred_flag */
    bitstream_put_ui(bs, 0, 1);                         /* redundant_pic_cnt_present_flag */

    /* more_rbsp_data */
    bitstream_put_ui(bs, pic_param->pic_fields.bits.transform_8x8_mode_flag, 1);    /*transform_8x8_mode_flag */
    bitstream_put_ui(bs, 0, 1);                         /* pic_scaling_matrix_present_flag */
    bitstream_put_se(bs, pic_param->second_chroma_qp_index_offset );    /*second_chroma_qp_index_offset */

    rbsp_trailing_bits(bs);
}

int build_packed_seq_buffer(unsigned char **header_buffer, VAProfile profile, VAEncSequenceParameterBufferH264 *seq_param)
{
    bitstream bs;

    bitstream_start(&bs);
    nal_start_code_prefix(&bs);
    nal_header(&bs, NAL_REF_IDC_HIGH, NAL_SPS);
    sps_rbsp(&bs, profile, seq_param->bits_per_second, seq_param);
    bitstream_end(&bs);

    *header_buffer = (unsigned char *)bs.buffer;
    return bs.bit_offset;
}

int build_packed_pic_buffer(unsigned char **header_buffer, VAEncPictureParameterBufferH264 *pic_param)
{
    bitstream bs;

    bitstream_start(&bs);
    nal_start_code_prefix(&bs);
    nal_header(&bs, NAL_REF_IDC_HIGH, NAL_PPS);
    pps_rbsp(&bs, pic_param);
    bitstream_end(&bs);

    *header_buffer = (unsigned char *)bs.buffer;
    return bs.bit_offset;
}

int build_packed_sei_buffer_timing(unsigned int init_cpb_removal_delay,
				unsigned int init_cpb_removal_delay_offset,
				unsigned int cpb_removal_length,
				unsigned int cpb_removal_delay,
				unsigned int dpb_output_length,
				unsigned int dpb_output_delay,
				unsigned char **sei_buffer)
{
    unsigned char *byte_buf;
    int bp_byte_size, i, pic_byte_size;

    bitstream nal_bs;
    bitstream sei_bp_bs, sei_pic_bs;

    bitstream_start(&sei_bp_bs);
    bitstream_put_ue(&sei_bp_bs, 0);       /*seq_parameter_set_id*/
    bitstream_put_ui(&sei_bp_bs, init_cpb_removal_delay, cpb_removal_length);
    bitstream_put_ui(&sei_bp_bs, init_cpb_removal_delay_offset, cpb_removal_length);
    if ( sei_bp_bs.bit_offset & 0x7) {
        bitstream_put_ui(&sei_bp_bs, 1, 1);
    }
    bitstream_end(&sei_bp_bs);
    bp_byte_size = (sei_bp_bs.bit_offset + 7) / 8;

    bitstream_start(&sei_pic_bs);
    bitstream_put_ui(&sei_pic_bs, cpb_removal_delay, cpb_removal_length);
    bitstream_put_ui(&sei_pic_bs, dpb_output_delay, dpb_output_length);
    if ( sei_pic_bs.bit_offset & 0x7) {
        bitstream_put_ui(&sei_pic_bs, 1, 1);
    }
    bitstream_end(&sei_pic_bs);
    pic_byte_size = (sei_pic_bs.bit_offset + 7) / 8;

    bitstream_start(&nal_bs);
    nal_start_code_prefix(&nal_bs);
    nal_header(&nal_bs, NAL_REF_IDC_NONE, NAL_SEI);

    /* Write the SEI buffer period data */
    bitstream_put_ui(&nal_bs, 0, 8);
    bitstream_put_ui(&nal_bs, bp_byte_size, 8);

    byte_buf = (unsigned char *)sei_bp_bs.buffer;
    for(i = 0; i < bp_byte_size; i++) {
        bitstream_put_ui(&nal_bs, byte_buf[i], 8);
    }
    free(byte_buf);
	/* write the SEI timing data */
    bitstream_put_ui(&nal_bs, 0x01, 8);
    bitstream_put_ui(&nal_bs, pic_byte_size, 8);

    byte_buf = (unsigned char *)sei_pic_bs.buffer;
    for(i = 0; i < pic_byte_size; i++) {
        bitstream_put_ui(&nal_bs, byte_buf[i], 8);
    }
    free(byte_buf);

    rbsp_trailing_bits(&nal_bs);
    bitstream_end(&nal_bs);

    *sei_buffer = (unsigned char *)nal_bs.buffer;

    return nal_bs.bit_offset;
}

#endif
