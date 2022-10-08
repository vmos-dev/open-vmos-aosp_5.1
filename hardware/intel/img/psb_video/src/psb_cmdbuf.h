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
 *    Waldo Bastian <waldo.bastian@intel.com>
 *
 */

#ifndef _PSB_CMDBUF_H_
#define _PSB_CMDBUF_H_

#include "psb_drv_video.h"
#include "psb_buffer.h"
//#include "xf86mm.h"
#include "hwdefs/dxva_fw_ctrl.h"
#include "hwdefs/lldma_defs.h"

#include <stdint.h>


#ifdef ANDROID
#define Bool int
#endif


typedef struct psb_cmdbuf_s *psb_cmdbuf_p;

struct psb_cmdbuf_s {
    struct psb_buffer_s buf;
    unsigned int size;
    struct psb_buffer_s reloc_buf;
    unsigned int reloc_size;

    struct psb_buffer_s regio_buf;
    unsigned int regio_size;
    unsigned char * regio_base;
    uint32_t *regio_idx;

    /* MTX msg */
    unsigned char *MTX_msg;
    /* Relocation records */
    unsigned char *reloc_base;
    struct drm_psb_reloc *reloc_idx;

    /* CMD stream data */
    int cmd_count;
    int deblock_count;
#ifdef SLICE_HEADER_PARSING
    int parse_count;
    int decode_flag;
#endif
    int oold_count;
    int host_be_opp_count;
    int frame_info_count;
    unsigned char *cmd_base;
    unsigned char *cmd_start;
    uint32_t *cmd_idx;
    uint32_t *cmd_bitstream_size; /* Pointer to bitstream size field in last SR_SETUP */
    /* LLDMA records */
    unsigned char *lldma_base;
    unsigned char *lldma_idx;
    unsigned char *lldma_last; /* Pointer to last LLDMA record */

    /* Referenced buffers */
    psb_buffer_p *buffer_refs;

    int buffer_refs_count;
    int buffer_refs_allocated;
    /* Pointer for Register commands */
    uint32_t *reg_start;
    uint32_t *reg_wt_p;
    uint32_t reg_next;
    uint32_t reg_flags;
    /* Pointer for Rendec Block commands */
    uint32_t *rendec_block_start;
    uint32_t *rendec_chunk_start;
    /* Pointer for Segment commands */
    uint32_t *last_next_segment_cmd;
    uint32_t first_segment_size;
    /* Pointer for Skip block commands */
    uint32_t *skip_block_start;
    uint32_t skip_condition;
};

/*
 * Create command buffer
 */
VAStatus psb_cmdbuf_create(
    object_context_p obj_context,
    psb_driver_data_p driver_data,
    psb_cmdbuf_p cmdbuf);

/*
 * Destroy buffer
 */
void psb_cmdbuf_destroy(psb_cmdbuf_p cmdbuf);

/*
 * Reset buffer & map
 *
 * Returns 0 on success
 */
int psb_cmdbuf_reset(psb_cmdbuf_p cmdbuf);

/*
 * Unmap buffer
 *
 * Returns 0 on success
 */
int psb_cmdbuf_unmap(psb_cmdbuf_p cmdbuf);

/*
 * Reference an addtional buffer "buf" in the command stream
 * Returns a reference index that can be used to refer to "buf" in
 * relocation records, on error -1 is returned.
 */
int psb_cmdbuf_buffer_ref(psb_cmdbuf_p cmdbuf, psb_buffer_p buf);

/* Creates a relocation record for a DWORD in the mapped "cmdbuf" at address
 * "addr_in_cmdbuf"
 * The relocation is based on the device virtual address of "ref_buffer"
 * "buf_offset" is be added to the device virtual address, and the sum is then
 * right shifted with "align_shift".
 * "mask" determines which bits of the target DWORD will be updated with the so
 * constructed address. The remaining bits will be filled with bits from "background".
 */
void psb_cmdbuf_add_relocation(psb_cmdbuf_p cmdbuf,
                               uint32_t *addr_in_cmdbuf,
                               psb_buffer_p ref_buffer,
                               uint32_t buf_offset,
                               uint32_t mask,
                               uint32_t background,
                               uint32_t align_shift,
                               uint32_t dst_buffer);

#define RELOC(dest, offset, buf)        psb_cmdbuf_add_relocation(cmdbuf, (uint32_t*) &dest, buf, offset, 0XFFFFFFFF, 0, 0, 1)
#define RELOC_MSG(dest, offset, buf)    psb_cmdbuf_add_relocation(cmdbuf, (uint32_t*) &dest, buf, offset, 0XFFFFFFFF, 0, 0, 0)
#define RELOC_SHIFT4(dest, offset, background, buf)     psb_cmdbuf_add_relocation(cmdbuf, (uint32_t*) &dest, buf, offset, 0X0FFFFFFF, background, 4, 1)
#define RELOC_REGIO(dest, offset, buf, dst)     psb_cmdbuf_add_relocation(cmdbuf, (uint32_t*) &dest, buf, offset, 0XFFFFFFFF, 0, 0, dst)

/*
 * Advances "obj_context" to the next cmdbuf
 *
 * Returns 0 on success
 */
int psb_context_get_next_cmdbuf(object_context_p obj_context);

int psb_context_submit_deblock(object_context_p obj_context);

int psb_context_submit_oold(object_context_p obj_context,
                            psb_buffer_p src_buf,
                            psb_buffer_p dst_buf,
                            psb_buffer_p colocate_buffer,
                            uint32_t picture_width_in_mb,
                            uint32_t frame_height_in_mb,
                            uint32_t field_type,
                            uint32_t chroma_offset);

int psb_context_submit_host_be_opp(object_context_p obj_context,
                                  psb_buffer_p buf_a,
                                  psb_buffer_p buf_b,
                                  psb_buffer_p buf_c,
                                  uint32_t picture_widht_mb,
                                  uint32_t frame_height_mb,
                                  uint32_t rotation_flags,
                                  uint32_t field_type,
                                  uint32_t ext_stride_a,
                                  uint32_t chroma_offset_a,
                                  uint32_t chroma_offset_b);

int psb_context_submit_hw_deblock(object_context_p obj_context,
                                  psb_buffer_p buf_a,
                                  psb_buffer_p buf_b,
                                  psb_buffer_p colocate_buffer,
                                  uint32_t picture_widht_mb,
                                  uint32_t frame_height_mb,
                                  uint32_t rotation_flags,
                                  uint32_t field_type,
                                  uint32_t ext_stride_a,
                                  uint32_t chroma_offset_a,
                                  uint32_t chroma_offset_b,
                                  uint32_t is_oold);

int psb_context_submit_hw_deblock(object_context_p obj_context,
                                  psb_buffer_p buf_a,
                                  psb_buffer_p buf_b,
                                  psb_buffer_p colocate_buffer,
                                  uint32_t picture_widht_mb,
                                  uint32_t frame_height_mb,
                                  uint32_t rotation_flags,
                                  uint32_t field_type,
                                  uint32_t ext_stride_a,
                                  uint32_t chroma_offset_a,
                                  uint32_t chroma_offset_b,
                                  uint32_t is_oold);

/*
 * Submits the current cmdbuf
 *
 * Returns 0 on success
 */
int psb_context_submit_cmdbuf(object_context_p obj_context);

/*
 * Flushes the pending cmdbuf
 *
 * Return 0 on success
 */
int psb_context_flush_cmdbuf(object_context_p obj_context);
void psb_cmdbuf_skip_start_block(psb_cmdbuf_p cmdbuf, uint32_t skip_condition);

/*
 * Write a SR_SETUP_CMD referencing a bitstream buffer to the command buffer
 *
 * The slice data runs from buffer_offset_in_bytes to buffer_offset_in_bytes + size_in_bytes
 * The first bit to be processed is buffer_offset_in_bytes + offset_in_bits
 *
 * TODO: Return something
 */

void psb_cmdbuf_dma_write_bitstream(psb_cmdbuf_p cmdbuf,
                                      psb_buffer_p bitstream_buf,
                                      uint32_t buffer_offset,
                                      uint32_t size_in_bytes,
                                      uint32_t offset_in_bits,
                                      uint32_t flags);

void psb_cmdbuf_dma_write_cmdbuf(psb_cmdbuf_p cmdbuf,
    psb_buffer_p bitstream_buf,
    uint32_t buffer_offset,
    uint32_t size,
    uint32_t dest_offset,
    DMA_TYPE type);

#ifdef SLICE_HEADER_PARSING
void psb_cmdbuf_dma_write_key(psb_cmdbuf_p cmdbuf,
                                      uint32_t flags,
                                      uint32_t key);
#endif
/*
 * Create a command to set registers
 */
void psb_cmdbuf_reg_start_block(psb_cmdbuf_p cmdbuf, uint32_t flags);

void psb_cmdbuf_reg_set(psb_cmdbuf_p cmdbuf, uint32_t reg, uint32_t val);

#define psb_cmdbuf_reg_set_RELOC( cmdbuf, reg, buffer,buffer_offset)             \
    do { *cmdbuf->cmd_idx++ = reg; RELOC(*cmdbuf->cmd_idx++, buffer_offset, buffer); } while (0)

void psb_cmdbuf_reg_set_address(psb_cmdbuf_p cmdbuf,
                                uint32_t reg,
                                psb_buffer_p buffer,
                                uint32_t buffer_offset);

/*
 * Finish a command to set registers
 */
void psb_cmdbuf_reg_end_block(psb_cmdbuf_p cmdbuf);

/*
 * Create a RENDEC command block
 */
void psb_cmdbuf_rendec_start(psb_cmdbuf_p cmdbuf, uint32_t dest_address);

#define psb_cmdbuf_rendec_write( cmdbuf, val ) \
    do { *cmdbuf->cmd_idx++ = val; } while(0)

void psb_cmdbuf_rendec_write_block(psb_cmdbuf_p cmdbuf,
                                   unsigned char *block,
                                   uint32_t size);

void psb_cmdbuf_rendec_write_address(psb_cmdbuf_p cmdbuf,
                                     psb_buffer_p buffer,
                                     uint32_t buffer_offset);

typedef enum {
    SKIP_ON_CONTEXT_SWITCH = 1,
} E_SKIP_CONDITION;

/*
 * Create a conditional SKIP block
 */
void psb_cmdbuf_skip_start_block(psb_cmdbuf_p cmdbuf, uint32_t skip_condition);

/*
 * Terminate a conditional SKIP block
 */
void psb_cmdbuf_skip_end_block(psb_cmdbuf_p cmdbuf);

/*
 * Terminate a conditional SKIP block
 */
void psb_cmdbuf_rendec_end(psb_cmdbuf_p cmdbuf);
/*
 * Write RegIO record into buffer
 */
int psb_cmdbuf_second_pass(object_context_p obj_context,
                           uint32_t OperatingModeCmd,
                           unsigned char * pvParamBase,
                           uint32_t PicWidthInMbs,
                           uint32_t FrameHeightInMbs,
                           psb_buffer_p target_buffer,
                           uint32_t chroma_offset
                          );

void *psb_cmdbuf_alloc_space(psb_cmdbuf_p cmdbuf, uint32_t byte_size);

void psb_cmdbuf_dma_write_bitstream_chained(psb_cmdbuf_p cmdbuf,
        psb_buffer_p bitstream_buf,
        uint32_t size_in_bytes);

#endif /* _PSB_CMDBUF_H_ */
