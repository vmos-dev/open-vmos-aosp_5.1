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
 */


/******************************************************************************

 @File         dxva_cmdseq_msg.h

 @Title        Debug driver

 @Platform     </b>\n

 @Description  </b>\n This file contains the VA_CMDSEQ_MSG_H Definitions.

******************************************************************************/
#if !defined (__VA_CMDSEQ_MSG_H__)
#define __VA_CMDSEQ_MSG_H__

#ifdef __cplusplus
extern "C" {
#endif

    /* Deblock parameters */
    typedef struct {
        uint32_t handle;        /* struct ttm_buffer_object * of REGIO */
        uint32_t buffer_size;
        uint32_t ctxid;

        uint32_t *pPicparams;
        void     *regio_kmap;   /* virtual of regio */
        uint32_t pad[3];
    } DEBLOCKPARAMS;

    /* Host BE OPP parameters */
    typedef struct {
        uint32_t handle;        /* struct ttm_buffer_object * of REGIO */
        uint32_t buffer_stride;
        uint32_t buffer_size;
        uint32_t picture_width_mb;
        uint32_t size_mb;
    } FRAME_INFO_PARAMS;

    typedef struct {
        union {
            struct {
uint32_t msg_size       :
                8;
uint32_t msg_type       :
                8;
uint32_t msg_fence      :
                16;
            } bits;
            uint32_t value;
        } header;
        union {
            struct {
uint32_t flags          :
                16;
uint32_t slice_type     :
                8;
uint32_t padding        :
                8;
            } bits;
            uint32_t value;
        } flags;
        uint32_t operating_mode;
        union {
            struct {
uint32_t context        :
                8;
uint32_t mmu_ptd        :
                24;
            } bits;
            uint32_t value;
        } mmu_context;
        union {
            struct {
uint32_t frame_height_mb        :
                16;
uint32_t pic_width_mb   :
                16;
            } bits;
            uint32_t value;
        } pic_size;
        uint32_t address_a0;
        uint32_t address_a1;
        uint32_t mb_param_address;
        uint32_t ext_stride_a;
        uint32_t address_b0;
        uint32_t address_b1;
        uint32_t rotation_flags;
        /* additional msg outside of IMG msg */
        uint32_t address_c0;
        uint32_t address_c1;
    } FW_VA_DEBLOCK_MSG;

    /* OOLD message */
    typedef struct {
        uint32_t pad[5];
        uint32_t SOURCE_LUMA_BUFFER_ADDRESS;
        uint32_t SOURCE_CHROMA_BUFFER_ADDRESS;
        uint32_t SOURCE_MB_PARAM_ADDRESS;
        uint32_t TARGET_LUMA_BUFFER_ADDRESS;
        uint32_t TARGET_CHROMA_BUFFER_ADDRESS;
    } FW_VA_OOLD_MSG;

struct fw_slice_header_extract_msg {
       union {
               struct {
                       uint32_t msg_size:8;
                       uint32_t msg_type:8;
                       uint32_t msg_fence:16;
               } bits;
               uint32_t value;
       } header;

       union {
               struct {
                       uint32_t flags:16;
                       uint32_t res:16;
               } bits;
               uint32_t value;
       } flags;

       uint32_t src;

       union {
               struct {
                       uint32_t context:8;
                       uint32_t mmu_ptd:24;
               } bits;
               uint32_t value;
       } mmu_context;

       uint32_t dst;
       uint32_t src_size;
       uint32_t dst_size;

       union {
               struct {
                       uint32_t expected_pps_id:8;
                       uint32_t nalu_header_unit_type:5;
                       uint32_t nalu_header_ref_idc:2;
                       uint32_t nalu_header_reserved:1;
                       uint32_t continue_parse_flag:1;
                       uint32_t frame_mbs_only_flag:1;
                       uint32_t pic_order_present_flag:1;
                       uint32_t delta_pic_order_always_zero_flag:1;
                       uint32_t redundant_pic_cnt_present_flag:1;
                       uint32_t weighted_pred_flag:1;
                       uint32_t entropy_coding_mode_flag:1;
                       uint32_t deblocking_filter_control_present_flag:1;
                       uint32_t weighted_bipred_idc:2;
                       uint32_t residual_colour_transform_flag:1;
                       uint32_t chroma_format_idc:2;
                       uint32_t idr_flag:1;
                       uint32_t pic_order_cnt_type:2;
               } bits;
               uint32_t value;
       } flag_bitfield;

       union {
               struct {
                       uint8_t num_slice_groups_minus1:3;
                       uint8_t num_ref_idc_l1_active_minus1:5;
                       uint8_t slice_group_map_type:3;
                       uint8_t num_ref_idc_l0_active_minus1:5;
                       uint8_t log2_slice_group_change_cycle:4;
                       uint8_t slice_header_bit_offset:4;
                       uint8_t log2_max_frame_num_minus4:4;
                       uint8_t log2_max_pic_order_cnt_lsb_minus4:4;
               } bits;
               uint32_t value;
       } pic_param0;
};

#define FW_VA_RENDER_SIZE               (32)

// FW_VA_RENDER     MSG_SIZE
#define FW_VA_RENDER_MSG_SIZE_ALIGNMENT         (1)
#define FW_VA_RENDER_MSG_SIZE_TYPE              IMG_UINT8
#define FW_VA_RENDER_MSG_SIZE_MASK              (0xFF)
#define FW_VA_RENDER_MSG_SIZE_LSBMASK           (0xFF)
#define FW_VA_RENDER_MSG_SIZE_OFFSET            (0x0000)
#define FW_VA_RENDER_MSG_SIZE_SHIFT             (0)

// FW_VA_RENDER     ID
#define FW_VA_RENDER_ID_ALIGNMENT               (1)
#define FW_VA_RENDER_ID_TYPE            IMG_UINT8
#define FW_VA_RENDER_ID_MASK            (0xFF)
#define FW_VA_RENDER_ID_LSBMASK         (0xFF)
#define FW_VA_RENDER_ID_OFFSET          (0x0001)
#define FW_VA_RENDER_ID_SHIFT           (0)

// FW_VA_RENDER     BUFFER_SIZE
#define FW_VA_RENDER_BUFFER_SIZE_ALIGNMENT              (2)
#define FW_VA_RENDER_BUFFER_SIZE_TYPE           IMG_UINT16
#define FW_VA_RENDER_BUFFER_SIZE_MASK           (0x0FFF)
#define FW_VA_RENDER_BUFFER_SIZE_LSBMASK                (0x0FFF)
#define FW_VA_RENDER_BUFFER_SIZE_OFFSET         (0x0002)
#define FW_VA_RENDER_BUFFER_SIZE_SHIFT          (0)

// FW_VA_RENDER     MMUPTD
#define FW_VA_RENDER_MMUPTD_ALIGNMENT           (4)
#define FW_VA_RENDER_MMUPTD_TYPE                IMG_UINT32
#define FW_VA_RENDER_MMUPTD_MASK                (0xFFFFFFFF)
#define FW_VA_RENDER_MMUPTD_LSBMASK             (0xFFFFFFFF)
#define FW_VA_RENDER_MMUPTD_OFFSET              (0x0004)
#define FW_VA_RENDER_MMUPTD_SHIFT               (0)

// FW_VA_RENDER     LLDMA_ADDRESS
#define FW_VA_RENDER_LLDMA_ADDRESS_ALIGNMENT            (4)
#define FW_VA_RENDER_LLDMA_ADDRESS_TYPE         IMG_UINT32
#define FW_VA_RENDER_LLDMA_ADDRESS_MASK         (0xFFFFFFFF)
#define FW_VA_RENDER_LLDMA_ADDRESS_LSBMASK              (0xFFFFFFFF)
#define FW_VA_RENDER_LLDMA_ADDRESS_OFFSET               (0x0008)
#define FW_VA_RENDER_LLDMA_ADDRESS_SHIFT                (0)

// FW_VA_RENDER     CONTEXT
#define FW_VA_RENDER_CONTEXT_ALIGNMENT          (4)
#define FW_VA_RENDER_CONTEXT_TYPE               IMG_UINT32
#define FW_VA_RENDER_CONTEXT_MASK               (0xFFFFFFFF)
#define FW_VA_RENDER_CONTEXT_LSBMASK            (0xFFFFFFFF)
#define FW_VA_RENDER_CONTEXT_OFFSET             (0x000C)
#define FW_VA_RENDER_CONTEXT_SHIFT              (0)

// FW_VA_RENDER     FENCE_VALUE
#define FW_VA_RENDER_FENCE_VALUE_ALIGNMENT              (4)
#define FW_VA_RENDER_FENCE_VALUE_TYPE           IMG_UINT32
#define FW_VA_RENDER_FENCE_VALUE_MASK           (0xFFFFFFFF)
#define FW_VA_RENDER_FENCE_VALUE_LSBMASK                (0xFFFFFFFF)
#define FW_VA_RENDER_FENCE_VALUE_OFFSET         (0x0010)
#define FW_VA_RENDER_FENCE_VALUE_SHIFT          (0)

// FW_VA_RENDER     OPERATING_MODE
#define FW_VA_RENDER_OPERATING_MODE_ALIGNMENT           (4)
#define FW_VA_RENDER_OPERATING_MODE_TYPE                IMG_UINT32
#define FW_VA_RENDER_OPERATING_MODE_MASK                (0xFFFFFFFF)
#define FW_VA_RENDER_OPERATING_MODE_LSBMASK             (0xFFFFFFFF)
#define FW_VA_RENDER_OPERATING_MODE_OFFSET              (0x0014)
#define FW_VA_RENDER_OPERATING_MODE_SHIFT               (0)

// FW_VA_RENDER     FIRST_MB_IN_SLICE
#define FW_VA_RENDER_FIRST_MB_IN_SLICE_ALIGNMENT                (2)
#define FW_VA_RENDER_FIRST_MB_IN_SLICE_TYPE             IMG_UINT16
#define FW_VA_RENDER_FIRST_MB_IN_SLICE_MASK             (0xFFFF)
#define FW_VA_RENDER_FIRST_MB_IN_SLICE_LSBMASK          (0xFFFF)
#define FW_VA_RENDER_FIRST_MB_IN_SLICE_OFFSET           (0x0018)
#define FW_VA_RENDER_FIRST_MB_IN_SLICE_SHIFT            (0)

// FW_VA_RENDER     LAST_MB_IN_FRAME
#define FW_VA_RENDER_LAST_MB_IN_FRAME_ALIGNMENT         (2)
#define FW_VA_RENDER_LAST_MB_IN_FRAME_TYPE              IMG_UINT16
#define FW_VA_RENDER_LAST_MB_IN_FRAME_MASK              (0xFFFF)
#define FW_VA_RENDER_LAST_MB_IN_FRAME_LSBMASK           (0xFFFF)
#define FW_VA_RENDER_LAST_MB_IN_FRAME_OFFSET            (0x001A)
#define FW_VA_RENDER_LAST_MB_IN_FRAME_SHIFT             (0)

// FW_VA_RENDER     FLAGS
#define FW_VA_RENDER_FLAGS_ALIGNMENT            (4)
#define FW_VA_RENDER_FLAGS_TYPE         IMG_UINT32
#define FW_VA_RENDER_FLAGS_MASK         (0xFFFFFFFF)
#define FW_VA_RENDER_FLAGS_LSBMASK              (0xFFFFFFFF)
#define FW_VA_RENDER_FLAGS_OFFSET               (0x001C)
#define FW_VA_RENDER_FLAGS_SHIFT                (0)

#define FW_DEVA_DECODE_SIZE             (20)

// FW_DEVA_DECODE     MSG_ID
#define FW_DEVA_DECODE_MSG_ID_ALIGNMENT         (2)
#define FW_DEVA_DECODE_MSG_ID_TYPE              IMG_UINT16
#define FW_DEVA_DECODE_MSG_ID_MASK              (0xFFFF)
#define FW_DEVA_DECODE_MSG_ID_LSBMASK           (0xFFFF)
#define FW_DEVA_DECODE_MSG_ID_OFFSET            (0x0002)
#define FW_DEVA_DECODE_MSG_ID_SHIFT             (0)

// FW_DEVA_DECODE     ID
#define FW_DEVA_DECODE_ID_ALIGNMENT             (1)
#define FW_DEVA_DECODE_ID_TYPE          IMG_UINT8
#define FW_DEVA_DECODE_ID_MASK          (0xFF)
#define FW_DEVA_DECODE_ID_LSBMASK               (0xFF)
#define FW_DEVA_DECODE_ID_OFFSET                (0x0001)
#define FW_DEVA_DECODE_ID_SHIFT         (0)

// FW_DEVA_DECODE     MSG_SIZE
#define FW_DEVA_DECODE_MSG_SIZE_ALIGNMENT               (1)
#define FW_DEVA_DECODE_MSG_SIZE_TYPE            IMG_UINT8
#define FW_DEVA_DECODE_MSG_SIZE_MASK            (0xFF)
#define FW_DEVA_DECODE_MSG_SIZE_LSBMASK         (0xFF)
#define FW_DEVA_DECODE_MSG_SIZE_OFFSET          (0x0000)
#define FW_DEVA_DECODE_MSG_SIZE_SHIFT           (0)

// FW_DEVA_DECODE     FLAGS
#define FW_DEVA_DECODE_FLAGS_ALIGNMENT          (2)
#define FW_DEVA_DECODE_FLAGS_TYPE               IMG_UINT16
#define FW_DEVA_DECODE_FLAGS_MASK               (0xFFFF)
#define FW_DEVA_DECODE_FLAGS_LSBMASK            (0xFFFF)
#define FW_DEVA_DECODE_FLAGS_OFFSET             (0x0004)
#define FW_DEVA_DECODE_FLAGS_SHIFT              (0)

// FW_DEVA_DECODE     BUFFER_SIZE
#define FW_DEVA_DECODE_BUFFER_SIZE_ALIGNMENT            (2)
#define FW_DEVA_DECODE_BUFFER_SIZE_TYPE         IMG_UINT16
#define FW_DEVA_DECODE_BUFFER_SIZE_MASK         (0xFFFF)
#define FW_DEVA_DECODE_BUFFER_SIZE_LSBMASK              (0xFFFF)
#define FW_DEVA_DECODE_BUFFER_SIZE_OFFSET               (0x0006)
#define FW_DEVA_DECODE_BUFFER_SIZE_SHIFT                (0)

// FW_DEVA_DECODE     LLDMA_ADDRESS
#define FW_DEVA_DECODE_LLDMA_ADDRESS_ALIGNMENT          (4)
#define FW_DEVA_DECODE_LLDMA_ADDRESS_TYPE               IMG_UINT32
#define FW_DEVA_DECODE_LLDMA_ADDRESS_MASK               (0xFFFFFFFF)
#define FW_DEVA_DECODE_LLDMA_ADDRESS_LSBMASK            (0xFFFFFFFF)
#define FW_DEVA_DECODE_LLDMA_ADDRESS_OFFSET             (0x0008)
#define FW_DEVA_DECODE_LLDMA_ADDRESS_SHIFT              (0)

// FW_DEVA_DECODE     MMUPTD
#define FW_DEVA_DECODE_MMUPTD_ALIGNMENT         (4)
#define FW_DEVA_DECODE_MMUPTD_TYPE              IMG_UINT32
#define FW_DEVA_DECODE_MMUPTD_MASK              (0xFFFFFF00)
#define FW_DEVA_DECODE_MMUPTD_LSBMASK           (0x00FFFFFF)
#define FW_DEVA_DECODE_MMUPTD_OFFSET            (0x000C)
#define FW_DEVA_DECODE_MMUPTD_SHIFT             (8)

// FW_DEVA_DECODE     CONTEXT
#define FW_DEVA_DECODE_CONTEXT_ALIGNMENT                (1)
#define FW_DEVA_DECODE_CONTEXT_TYPE             IMG_UINT8
#define FW_DEVA_DECODE_CONTEXT_MASK             (0xFF)
#define FW_DEVA_DECODE_CONTEXT_LSBMASK          (0xFF)
#define FW_DEVA_DECODE_CONTEXT_OFFSET           (0x000C)

#define FW_VA_DEBLOCK_SIZE              (16 + 32) /* 32 bytes for DEBLOCKPARAMS */
#define FW_DEVA_DEBLOCK_SIZE            (48)

#define FW_DEVA_DECODE_CONTEXT_SHIFT            (0)

// FW_DEVA_DECODE     OPERATING_MODE
#define FW_DEVA_DECODE_OPERATING_MODE_ALIGNMENT         (4)
#define FW_DEVA_DECODE_OPERATING_MODE_TYPE              IMG_UINT32
#define FW_DEVA_DECODE_OPERATING_MODE_MASK              (0xFFFFFFFF)
#define FW_DEVA_DECODE_OPERATING_MODE_LSBMASK           (0xFFFFFFFF)
#define FW_DEVA_DECODE_OPERATING_MODE_OFFSET            (0x0010)
#define FW_DEVA_DECODE_OPERATING_MODE_SHIFT             (0)

// FW_VA_DEBLOCK     MSG_SIZE
#define FW_VA_DEBLOCK_MSG_SIZE_ALIGNMENT                (1)
#define FW_VA_DEBLOCK_MSG_SIZE_TYPE             IMG_UINT8
#define FW_VA_DEBLOCK_MSG_SIZE_MASK             (0xFF)
#define FW_VA_DEBLOCK_MSG_SIZE_LSBMASK          (0xFF)
#define FW_VA_DEBLOCK_MSG_SIZE_OFFSET           (0x0000)
#define FW_VA_DEBLOCK_MSG_SIZE_SHIFT            (0)

// FW_VA_DEBLOCK     ID
#define FW_VA_DEBLOCK_ID_ALIGNMENT              (1)
#define FW_VA_DEBLOCK_ID_TYPE           IMG_UINT8
#define FW_VA_DEBLOCK_ID_MASK           (0xFF)
#define FW_VA_DEBLOCK_ID_LSBMASK                (0xFF)
#define FW_VA_DEBLOCK_ID_OFFSET         (0x0001)
#define FW_VA_DEBLOCK_ID_SHIFT          (0)

// FW_VA_DEBLOCK     FLAGS
#define FW_VA_DEBLOCK_FLAGS_ALIGNMENT           (2)
#define FW_VA_DEBLOCK_FLAGS_TYPE                IMG_UINT16
#define FW_VA_DEBLOCK_FLAGS_MASK                (0xFFFF)
#define FW_VA_DEBLOCK_FLAGS_LSBMASK             (0xFFFF)
#define FW_VA_DEBLOCK_FLAGS_OFFSET              (0x0002)
#define FW_VA_DEBLOCK_FLAGS_SHIFT               (0)

// FW_VA_DEBLOCK     CONTEXT
#define FW_VA_DEBLOCK_CONTEXT_ALIGNMENT         (4)
#define FW_VA_DEBLOCK_CONTEXT_TYPE              IMG_UINT32
#define FW_VA_DEBLOCK_CONTEXT_MASK              (0xFFFFFFFF)
#define FW_VA_DEBLOCK_CONTEXT_LSBMASK           (0xFFFFFFFF)
#define FW_VA_DEBLOCK_CONTEXT_OFFSET            (0x0004)
#define FW_VA_DEBLOCK_CONTEXT_SHIFT             (0)

// FW_VA_DEBLOCK     FENCE_VALUE
#define FW_VA_DEBLOCK_FENCE_VALUE_ALIGNMENT             (4)
#define FW_VA_DEBLOCK_FENCE_VALUE_TYPE          IMG_UINT32
#define FW_VA_DEBLOCK_FENCE_VALUE_MASK          (0xFFFFFFFF)
#define FW_VA_DEBLOCK_FENCE_VALUE_LSBMASK               (0xFFFFFFFF)
#define FW_VA_DEBLOCK_FENCE_VALUE_OFFSET                (0x0008)
#define FW_VA_DEBLOCK_FENCE_VALUE_SHIFT         (0)

// FW_VA_DEBLOCK     MMUPTD
#define FW_VA_DEBLOCK_MMUPTD_ALIGNMENT          (4)
#define FW_VA_DEBLOCK_MMUPTD_TYPE               IMG_UINT32
#define FW_VA_DEBLOCK_MMUPTD_MASK               (0xFFFFFFFF)
#define FW_VA_DEBLOCK_MMUPTD_LSBMASK            (0xFFFFFFFF)
#define FW_VA_DEBLOCK_MMUPTD_OFFSET             (0x000C)
#define FW_VA_DEBLOCK_MMUPTD_SHIFT              (0)

#define FW_VA_OOLD_SIZE         (40)

// FW_VA_OOLD     MSG_SIZE
#define FW_VA_OOLD_MSG_SIZE_ALIGNMENT           (1)
#define FW_VA_OOLD_MSG_SIZE_TYPE                IMG_UINT8
#define FW_VA_OOLD_MSG_SIZE_MASK                (0xFF)
#define FW_VA_OOLD_MSG_SIZE_LSBMASK             (0xFF)
#define FW_VA_OOLD_MSG_SIZE_OFFSET              (0x0000)
#define FW_VA_OOLD_MSG_SIZE_SHIFT               (0)

// FW_VA_OOLD     ID
#define FW_VA_OOLD_ID_ALIGNMENT         (1)
#define FW_VA_OOLD_ID_TYPE              IMG_UINT8
#define FW_VA_OOLD_ID_MASK              (0xFF)
#define FW_VA_OOLD_ID_LSBMASK           (0xFF)
#define FW_VA_OOLD_ID_OFFSET            (0x0001)
#define FW_VA_OOLD_ID_SHIFT             (0)

// FW_VA_OOLD     SLICE_FIELD_TYPE
#define FW_VA_OOLD_SLICE_FIELD_TYPE_ALIGNMENT           (1)
#define FW_VA_OOLD_SLICE_FIELD_TYPE_TYPE                IMG_UINT8
#define FW_VA_OOLD_SLICE_FIELD_TYPE_MASK                (0x03)
#define FW_VA_OOLD_SLICE_FIELD_TYPE_LSBMASK             (0x03)
#define FW_VA_OOLD_SLICE_FIELD_TYPE_OFFSET              (0x0002)
#define FW_VA_OOLD_SLICE_FIELD_TYPE_SHIFT               (0)

// FW_VA_OOLD     MMUPTD
#define FW_VA_OOLD_MMUPTD_ALIGNMENT             (4)
#define FW_VA_OOLD_MMUPTD_TYPE          IMG_UINT32
#define FW_VA_OOLD_MMUPTD_MASK          (0xFFFFFFFF)
#define FW_VA_OOLD_MMUPTD_LSBMASK               (0xFFFFFFFF)
#define FW_VA_OOLD_MMUPTD_OFFSET                (0x0004)
#define FW_VA_OOLD_MMUPTD_SHIFT         (0)

// FW_VA_OOLD     FENCE_VALUE
#define FW_VA_OOLD_FENCE_VALUE_ALIGNMENT                (4)
#define FW_VA_OOLD_FENCE_VALUE_TYPE             IMG_UINT32
#define FW_VA_OOLD_FENCE_VALUE_MASK             (0xFFFFFFFF)
#define FW_VA_OOLD_FENCE_VALUE_LSBMASK          (0xFFFFFFFF)
#define FW_VA_OOLD_FENCE_VALUE_OFFSET           (0x0008)
#define FW_VA_OOLD_FENCE_VALUE_SHIFT            (0)

// FW_VA_OOLD     OPERATING_MODE
#define FW_VA_OOLD_OPERATING_MODE_ALIGNMENT             (4)
#define FW_VA_OOLD_OPERATING_MODE_TYPE          IMG_UINT32
#define FW_VA_OOLD_OPERATING_MODE_MASK          (0xFFFFFFFF)
#define FW_VA_OOLD_OPERATING_MODE_LSBMASK               (0xFFFFFFFF)
#define FW_VA_OOLD_OPERATING_MODE_OFFSET                (0x000C)
#define FW_VA_OOLD_OPERATING_MODE_SHIFT         (0)

// FW_VA_OOLD     FRAME_HEIGHT_MBS
#define FW_VA_OOLD_FRAME_HEIGHT_MBS_ALIGNMENT           (2)
#define FW_VA_OOLD_FRAME_HEIGHT_MBS_TYPE                IMG_UINT16
#define FW_VA_OOLD_FRAME_HEIGHT_MBS_MASK                (0xFFFF)
#define FW_VA_OOLD_FRAME_HEIGHT_MBS_LSBMASK             (0xFFFF)
#define FW_VA_OOLD_FRAME_HEIGHT_MBS_OFFSET              (0x0010)
#define FW_VA_OOLD_FRAME_HEIGHT_MBS_SHIFT               (0)

// FW_VA_OOLD     PIC_WIDTH_MBS
#define FW_VA_OOLD_PIC_WIDTH_MBS_ALIGNMENT              (2)
#define FW_VA_OOLD_PIC_WIDTH_MBS_TYPE           IMG_UINT16
#define FW_VA_OOLD_PIC_WIDTH_MBS_MASK           (0xFFFF)
#define FW_VA_OOLD_PIC_WIDTH_MBS_LSBMASK                (0xFFFF)
#define FW_VA_OOLD_PIC_WIDTH_MBS_OFFSET         (0x0012)
#define FW_VA_OOLD_PIC_WIDTH_MBS_SHIFT          (0)

// FW_VA_OOLD     SOURCE_LUMA_BUFFER_ADDRESS
#define FW_VA_OOLD_SOURCE_LUMA_BUFFER_ADDRESS_ALIGNMENT         (4)
#define FW_VA_OOLD_SOURCE_LUMA_BUFFER_ADDRESS_TYPE              IMG_UINT32
#define FW_VA_OOLD_SOURCE_LUMA_BUFFER_ADDRESS_MASK              (0xFFFFFFFF)
#define FW_VA_OOLD_SOURCE_LUMA_BUFFER_ADDRESS_LSBMASK           (0xFFFFFFFF)
#define FW_VA_OOLD_SOURCE_LUMA_BUFFER_ADDRESS_OFFSET            (0x0014)
#define FW_VA_OOLD_SOURCE_LUMA_BUFFER_ADDRESS_SHIFT             (0)

// FW_VA_OOLD     SOURCE_CHROMA_BUFFER_ADDRESS
#define FW_VA_OOLD_SOURCE_CHROMA_BUFFER_ADDRESS_ALIGNMENT               (4)
#define FW_VA_OOLD_SOURCE_CHROMA_BUFFER_ADDRESS_TYPE            IMG_UINT32
#define FW_VA_OOLD_SOURCE_CHROMA_BUFFER_ADDRESS_MASK            (0xFFFFFFFF)
#define FW_VA_OOLD_SOURCE_CHROMA_BUFFER_ADDRESS_LSBMASK         (0xFFFFFFFF)
#define FW_VA_OOLD_SOURCE_CHROMA_BUFFER_ADDRESS_OFFSET          (0x0018)
#define FW_VA_OOLD_SOURCE_CHROMA_BUFFER_ADDRESS_SHIFT           (0)

// FW_VA_OOLD     SOURCE_MB_PARAM_ADDRESS
#define FW_VA_OOLD_SOURCE_MB_PARAM_ADDRESS_ALIGNMENT            (4)
#define FW_VA_OOLD_SOURCE_MB_PARAM_ADDRESS_TYPE         IMG_UINT32
#define FW_VA_OOLD_SOURCE_MB_PARAM_ADDRESS_MASK         (0xFFFFFFFF)
#define FW_VA_OOLD_SOURCE_MB_PARAM_ADDRESS_LSBMASK              (0xFFFFFFFF)
#define FW_VA_OOLD_SOURCE_MB_PARAM_ADDRESS_OFFSET               (0x001C)
#define FW_VA_OOLD_SOURCE_MB_PARAM_ADDRESS_SHIFT                (0)

// FW_VA_OOLD     TARGET_LUMA_BUFFER_ADDRESS
#define FW_VA_OOLD_TARGET_LUMA_BUFFER_ADDRESS_ALIGNMENT         (4)
#define FW_VA_OOLD_TARGET_LUMA_BUFFER_ADDRESS_TYPE              IMG_UINT32
#define FW_VA_OOLD_TARGET_LUMA_BUFFER_ADDRESS_MASK              (0xFFFFFFFF)
#define FW_VA_OOLD_TARGET_LUMA_BUFFER_ADDRESS_LSBMASK           (0xFFFFFFFF)
#define FW_VA_OOLD_TARGET_LUMA_BUFFER_ADDRESS_OFFSET            (0x0020)
#define FW_VA_OOLD_TARGET_LUMA_BUFFER_ADDRESS_SHIFT             (0)

// FW_VA_OOLD     TARGET_CHROMA_BUFFER_ADDRESS
#define FW_VA_OOLD_TARGET_CHROMA_BUFFER_ADDRESS_ALIGNMENT               (4)
#define FW_VA_OOLD_TARGET_CHROMA_BUFFER_ADDRESS_TYPE            IMG_UINT32
#define FW_VA_OOLD_TARGET_CHROMA_BUFFER_ADDRESS_MASK            (0xFFFFFFFF)
#define FW_VA_OOLD_TARGET_CHROMA_BUFFER_ADDRESS_LSBMASK         (0xFFFFFFFF)
#define FW_VA_OOLD_TARGET_CHROMA_BUFFER_ADDRESS_OFFSET          (0x0024)
#define FW_VA_OOLD_TARGET_CHROMA_BUFFER_ADDRESS_SHIFT           (0)

#define FW_VA_CMD_COMPLETED_SIZE                (12)

// FW_VA_CMD_COMPLETED     MSG_SIZE
#define FW_VA_CMD_COMPLETED_MSG_SIZE_ALIGNMENT          (1)
#define FW_VA_CMD_COMPLETED_MSG_SIZE_TYPE               IMG_UINT8
#define FW_VA_CMD_COMPLETED_MSG_SIZE_MASK               (0xFF)
#define FW_VA_CMD_COMPLETED_MSG_SIZE_LSBMASK            (0xFF)
#define FW_VA_CMD_COMPLETED_MSG_SIZE_OFFSET             (0x0000)
#define FW_VA_CMD_COMPLETED_MSG_SIZE_SHIFT              (0)

// FW_VA_CMD_COMPLETED     ID
#define FW_VA_CMD_COMPLETED_ID_ALIGNMENT                (1)
#define FW_VA_CMD_COMPLETED_ID_TYPE             IMG_UINT8
#define FW_VA_CMD_COMPLETED_ID_MASK             (0xFF)
#define FW_VA_CMD_COMPLETED_ID_LSBMASK          (0xFF)
#define FW_VA_CMD_COMPLETED_ID_OFFSET           (0x0001)
#define FW_VA_CMD_COMPLETED_ID_SHIFT            (0)

// FW_VA_CMD_COMPLETED     FENCE_VALUE
#define FW_VA_CMD_COMPLETED_FENCE_VALUE_ALIGNMENT               (4)
#define FW_VA_CMD_COMPLETED_FENCE_VALUE_TYPE            IMG_UINT32
#define FW_VA_CMD_COMPLETED_FENCE_VALUE_MASK            (0xFFFFFFFF)
#define FW_VA_CMD_COMPLETED_FENCE_VALUE_LSBMASK         (0xFFFFFFFF)
#define FW_VA_CMD_COMPLETED_FENCE_VALUE_OFFSET          (0x0004)
#define FW_VA_CMD_COMPLETED_FENCE_VALUE_SHIFT           (0)

// FW_VA_CMD_COMPLETED     FLAGS
#define FW_VA_CMD_COMPLETED_FLAGS_ALIGNMENT             (4)
#define FW_VA_CMD_COMPLETED_FLAGS_TYPE          IMG_UINT32
#define FW_VA_CMD_COMPLETED_FLAGS_MASK          (0xFFFFFFFF)
#define FW_VA_CMD_COMPLETED_FLAGS_LSBMASK               (0xFFFFFFFF)
#define FW_VA_CMD_COMPLETED_FLAGS_OFFSET                (0x0008)
#define FW_VA_CMD_COMPLETED_FLAGS_SHIFT         (0)

#define FW_VA_CMD_FAILED_SIZE           (12)

// FW_VA_CMD_FAILED     MSG_SIZE
#define FW_VA_CMD_FAILED_MSG_SIZE_ALIGNMENT             (1)
#define FW_VA_CMD_FAILED_MSG_SIZE_TYPE          IMG_UINT8
#define FW_VA_CMD_FAILED_MSG_SIZE_MASK          (0xFF)
#define FW_VA_CMD_FAILED_MSG_SIZE_LSBMASK               (0xFF)
#define FW_VA_CMD_FAILED_MSG_SIZE_OFFSET                (0x0000)
#define FW_VA_CMD_FAILED_MSG_SIZE_SHIFT         (0)

// FW_VA_CMD_FAILED     ID
#define FW_VA_CMD_FAILED_ID_ALIGNMENT           (1)
#define FW_VA_CMD_FAILED_ID_TYPE                IMG_UINT8
#define FW_VA_CMD_FAILED_ID_MASK                (0xFF)
#define FW_VA_CMD_FAILED_ID_LSBMASK             (0xFF)
#define FW_VA_CMD_FAILED_ID_OFFSET              (0x0001)
#define FW_VA_CMD_FAILED_ID_SHIFT               (0)

// FW_VA_CMD_FAILED     FLAGS
#define FW_VA_CMD_FAILED_FLAGS_ALIGNMENT                (2)
#define FW_VA_CMD_FAILED_FLAGS_TYPE             IMG_UINT16
#define FW_VA_CMD_FAILED_FLAGS_MASK             (0xFFFF)
#define FW_VA_CMD_FAILED_FLAGS_LSBMASK          (0xFFFF)
#define FW_VA_CMD_FAILED_FLAGS_OFFSET           (0x0002)
#define FW_VA_CMD_FAILED_FLAGS_SHIFT            (0)

// FW_VA_CMD_FAILED     FENCE_VALUE
#define FW_VA_CMD_FAILED_FENCE_VALUE_ALIGNMENT          (4)
#define FW_VA_CMD_FAILED_FENCE_VALUE_TYPE               IMG_UINT32
#define FW_VA_CMD_FAILED_FENCE_VALUE_MASK               (0xFFFFFFFF)
#define FW_VA_CMD_FAILED_FENCE_VALUE_LSBMASK            (0xFFFFFFFF)
#define FW_VA_CMD_FAILED_FENCE_VALUE_OFFSET             (0x0004)
#define FW_VA_CMD_FAILED_FENCE_VALUE_SHIFT              (0)

// FW_VA_CMD_FAILED     IRQSTATUS
#define FW_VA_CMD_FAILED_IRQSTATUS_ALIGNMENT            (4)
#define FW_VA_CMD_FAILED_IRQSTATUS_TYPE         IMG_UINT32
#define FW_VA_CMD_FAILED_IRQSTATUS_MASK         (0xFFFFFFFF)
#define FW_VA_CMD_FAILED_IRQSTATUS_LSBMASK              (0xFFFFFFFF)
#define FW_VA_CMD_FAILED_IRQSTATUS_OFFSET               (0x0008)
#define FW_VA_CMD_FAILED_IRQSTATUS_SHIFT                (0)

#define FW_VA_DEBLOCK_REQUIRED_SIZE             (8)

// FW_VA_DEBLOCK_REQUIRED     MSG_SIZE
#define FW_VA_DEBLOCK_REQUIRED_MSG_SIZE_ALIGNMENT               (1)
#define FW_VA_DEBLOCK_REQUIRED_MSG_SIZE_TYPE            IMG_UINT8
#define FW_VA_DEBLOCK_REQUIRED_MSG_SIZE_MASK            (0xFF)
#define FW_VA_DEBLOCK_REQUIRED_MSG_SIZE_LSBMASK         (0xFF)
#define FW_VA_DEBLOCK_REQUIRED_MSG_SIZE_OFFSET          (0x0000)
#define FW_VA_DEBLOCK_REQUIRED_MSG_SIZE_SHIFT           (0)

// FW_VA_DEBLOCK_REQUIRED     ID
#define FW_VA_DEBLOCK_REQUIRED_ID_ALIGNMENT             (1)
#define FW_VA_DEBLOCK_REQUIRED_ID_TYPE          IMG_UINT8
#define FW_VA_DEBLOCK_REQUIRED_ID_MASK          (0xFF)
#define FW_VA_DEBLOCK_REQUIRED_ID_LSBMASK               (0xFF)
#define FW_VA_DEBLOCK_REQUIRED_ID_OFFSET                (0x0001)
#define FW_VA_DEBLOCK_REQUIRED_ID_SHIFT         (0)

// FW_VA_DEBLOCK_REQUIRED     CONTEXT
#define FW_VA_DEBLOCK_REQUIRED_CONTEXT_ALIGNMENT                (4)
#define FW_VA_DEBLOCK_REQUIRED_CONTEXT_TYPE             IMG_UINT32
#define FW_VA_DEBLOCK_REQUIRED_CONTEXT_MASK             (0xFFFFFFFF)
#define FW_VA_DEBLOCK_REQUIRED_CONTEXT_LSBMASK          (0xFFFFFFFF)
#define FW_VA_DEBLOCK_REQUIRED_CONTEXT_OFFSET           (0x0004)
#define FW_VA_DEBLOCK_REQUIRED_CONTEXT_SHIFT            (0)

#define FW_VA_HW_PANIC_SIZE             (12)

// FW_VA_HW_PANIC     FLAGS
#define FW_VA_HW_PANIC_FLAGS_ALIGNMENT          (2)
#define FW_VA_HW_PANIC_FLAGS_TYPE               IMG_UINT16
#define FW_VA_HW_PANIC_FLAGS_MASK               (0xFFFF)
#define FW_VA_HW_PANIC_FLAGS_LSBMASK            (0xFFFF)
#define FW_VA_HW_PANIC_FLAGS_OFFSET             (0x0002)
#define FW_VA_HW_PANIC_FLAGS_SHIFT              (0)

// FW_VA_HW_PANIC     MSG_SIZE
#define FW_VA_HW_PANIC_MSG_SIZE_ALIGNMENT               (1)
#define FW_VA_HW_PANIC_MSG_SIZE_TYPE            IMG_UINT8
#define FW_VA_HW_PANIC_MSG_SIZE_MASK            (0xFF)
#define FW_VA_HW_PANIC_MSG_SIZE_LSBMASK         (0xFF)
#define FW_VA_HW_PANIC_MSG_SIZE_OFFSET          (0x0000)
#define FW_VA_HW_PANIC_MSG_SIZE_SHIFT           (0)

// FW_VA_HW_PANIC     ID
#define FW_VA_HW_PANIC_ID_ALIGNMENT             (1)
#define FW_VA_HW_PANIC_ID_TYPE          IMG_UINT8
#define FW_VA_HW_PANIC_ID_MASK          (0xFF)
#define FW_VA_HW_PANIC_ID_LSBMASK               (0xFF)
#define FW_VA_HW_PANIC_ID_OFFSET                (0x0001)
#define FW_VA_HW_PANIC_ID_SHIFT         (0)

// FW_VA_HW_PANIC     FENCE_VALUE
#define FW_VA_HW_PANIC_FENCE_VALUE_ALIGNMENT            (4)
#define FW_VA_HW_PANIC_FENCE_VALUE_TYPE         IMG_UINT32
#define FW_VA_HW_PANIC_FENCE_VALUE_MASK         (0xFFFFFFFF)
#define FW_VA_HW_PANIC_FENCE_VALUE_LSBMASK              (0xFFFFFFFF)
#define FW_VA_HW_PANIC_FENCE_VALUE_OFFSET               (0x0004)
#define FW_VA_HW_PANIC_FENCE_VALUE_SHIFT                (0)

// FW_VA_HW_PANIC     IRQSTATUS
#define FW_VA_HW_PANIC_IRQSTATUS_ALIGNMENT              (4)
#define FW_VA_HW_PANIC_IRQSTATUS_TYPE           IMG_UINT32
#define FW_VA_HW_PANIC_IRQSTATUS_MASK           (0xFFFFFFFF)
#define FW_VA_HW_PANIC_IRQSTATUS_LSBMASK                (0xFFFFFFFF)
#define FW_VA_HW_PANIC_IRQSTATUS_OFFSET         (0x0008)
#define FW_VA_HW_PANIC_IRQSTATUS_SHIFT          (0)

#define FW_VA_HOST_BE_OPP_SIZE 48
// FW_VA_HOST_BE_OPP     CONTEXT
#define FW_VA_HOST_BE_OPP_CONTEXT_ALIGNMENT             (1)
#define FW_VA_HOST_BE_OPP_CONTEXT_TYPE          IMG_UINT8
#define FW_VA_HOST_BE_OPP_CONTEXT_MASK          (0xF)
#define FW_VA_HOST_BE_OPP_CONTEXT_LSBMASK       (0xF)
#define FW_VA_HOST_BE_OPP_CONTEXT_OFFSET        (0x000C)
#define FW_VA_HOST_BE_OPP_CONTEXT_SHIFT         (0)

// FW_VA_HOST_BE_OPP    FLAGS
#define FW_VA_HOST_BE_OPP_FLAGS_ALIGNMENT               (2)
#define FW_VA_HOST_BE_OPP_FLAGS_TYPE            IMG_UINT16
#define FW_VA_HOST_BE_OPP_FLAGS_MASK            (0xFFFF)
#define FW_VA_HOST_BE_OPP_FLAGS_LSBMASK         (0xFFFF)
#define FW_VA_HOST_BE_OPP_FLAGS_OFFSET          (0x0004)
#define FW_VA_HOST_BE_OPP_FLAGS_SHIFT           (0)

#define FW_VA_FRAME_INFO_SIZE 24 /* 20 bytes for FRAME_INFO_PARAMS */

#ifdef __cplusplus
}
#endif

#endif /* __VA_CMDSEQ_MSG_H__ */
