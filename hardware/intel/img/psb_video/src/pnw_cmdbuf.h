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
 *    Zeng Li <zeng.li@intel.com>
 *
 */

#ifndef _PNW_CMDBUF_H_
#define _PNW_CMDBUF_H_

#include "psb_drv_video.h"
#include "psb_surface.h"
#include "psb_buffer.h"
#ifdef ANDROID
#include <drm/ttm/ttm_placement.h>
#include <linux/psb_drm.h>
#else
#include <psb_drm.h>
#endif

#include <stdint.h>

#define MTX_CMDWORD_ID_MASK     (0x7f)
#define MTX_CMDWORD_ID_SHIFT    (0)
#define MTX_CMDWORD_CORE_MASK   (0xff)
#define MTX_CMDWORD_CORE_SHIFT  (8)
#define MTX_CMDWORD_COUNT_MASK  (0x7fff)
#define MTX_CMDWORD_COUNT_SHIFT (16)
#define MTX_CMDWORD_INT_SHIFT   (7)
#define MTX_CMDWORD_INT_MASK    (1)

#define PNW_CMDBUF_START_PIC_IDX (0)
#define PNW_CMDBUF_SEQ_HEADER_IDX (1)
#define PNW_CMDBUF_PIC_HEADER_IDX (2)
#define PNW_CMDBUF_SEI_BUF_PERIOD_IDX (3)
#define PNW_CMDBUF_SEI_PIC_TIMING_IDX (4)
#define PNW_CMDBUF_SAVING_MAX (5)

struct pnw_cmdbuf_s {
    struct psb_buffer_s buf;
    unsigned int size;

    /* Relocation records */
    unsigned char *reloc_base;
    struct drm_psb_reloc *reloc_idx;

    /* CMD stream data */
    int cmd_count;
    unsigned char *cmd_base;
    unsigned char *cmd_start;
    uint32_t *cmd_idx;
    uint32_t *cmd_idx_saved[PNW_CMDBUF_SAVING_MAX]; /* idx saved for dual-core adjustion */

    /* all frames share one topaz param buffer which contains InParamBase
     * AboveParam/BellowParam, and the buffer allocated when the context is created
     */
    struct psb_buffer_s *topaz_in_params_I;
    unsigned char *topaz_in_params_I_p;

    struct psb_buffer_s *topaz_in_params_P;
    unsigned char *topaz_in_params_P_p;

    struct psb_buffer_s *topaz_below_params;
    unsigned char *topaz_below_params_p;

    /* Every frame has its own PIC_PARAMS, SLICE_PARAMS and HEADER mem
     */

    /* PicParams: */
    struct psb_buffer_s pic_params;
    unsigned char *pic_params_p;

    /* SeqHeaderMem PicHeaderMem EOSeqHeaderMem  EOStreamHeaderMem SliceHeaderMem[MAX_SLICES_PER_PICTURE]*/
    struct psb_buffer_s header_mem;
    unsigned char *header_mem_p;

    /*SliceParams[MAX_SLICES_PER_PICTURE] */
    struct psb_buffer_s slice_params;
    unsigned char *slice_params_p;

    /* AboveParams[MAX_TOPAZ_CORES]; */
    struct psb_buffer_s *topaz_above_params;
    unsigned char *topaz_above_params_p;

    /* Referenced buffers */
    psb_buffer_p *buffer_refs;
    int buffer_refs_count;
    int buffer_refs_allocated;

};

typedef struct pnw_cmdbuf_s *pnw_cmdbuf_p;

/*
 * Create command buffer
 */
VAStatus pnw_cmdbuf_create(object_context_p obj_context,
                           psb_driver_data_p driver_data,
                           pnw_cmdbuf_p cmdbuf
                          );

/*
 * Destroy buffer
 */
void pnw_cmdbuf_destroy(pnw_cmdbuf_p cmdbuf);

/*
 * Reset buffer & map
 *
 * Returns 0 on success
 */
int pnw_cmdbuf_reset(pnw_cmdbuf_p cmdbuf);

/*
 * Unmap buffer
 *
 * Returns 0 on success
 */
int pnw_cmdbuf_unmap(pnw_cmdbuf_p cmdbuf);

/*
 * Reference an addtional buffer "buf" in the command stream
 * Returns a reference index that can be used to refer to "buf" in
 * relocation records, on error -1 is returned.
 */
int pnw_cmdbuf_buffer_ref(pnw_cmdbuf_p cmdbuf, psb_buffer_p buf);

/* Creates a relocation record for a DWORD in the mapped "cmdbuf" at address
 * "addr_in_cmdbuf"
 * The relocation is based on the device virtual address of "ref_buffer"
 * "buf_offset" is be added to the device virtual address, and the sum is then
 * right shifted with "align_shift".
 * "mask" determines which bits of the target DWORD will be updated with the so
 * constructed address. The remaining bits will be filled with bits from "background".
 */
void pnw_cmdbuf_add_relocation(pnw_cmdbuf_p cmdbuf,
                               uint32_t *addr_in_dst_buffer,/*addr of dst_buffer for the DWORD*/
                               psb_buffer_p ref_buffer,
                               uint32_t buf_offset,
                               uint32_t mask,
                               uint32_t background,
                               uint32_t align_shift,
                               uint32_t dst_buffer, /*Index of the list refered by cmdbuf->buffer_refs */
                               uint32_t *start_of_dst_buffer);

#define RELOC_CMDBUF_PNW(dest, offset, buf)     pnw_cmdbuf_add_relocation(cmdbuf, (uint32_t*)(dest), buf, offset, 0XFFFFFFFF, 0, 0, 0, (uint32_t *)cmdbuf->cmd_start)

/* do relocation in PIC_PARAMS: src/dst Y/UV base, InParamsBase, CodeBase, BellowParamsBase, AboveParamsBase */
#define RELOC_PIC_PARAMS_PNW(dest, offset, buf) pnw_cmdbuf_add_relocation(cmdbuf, (uint32_t*)(dest), buf, offset, 0XFFFFFFFF, 0, 0, 1, (uint32_t *)cmdbuf->pic_params_p)

/* do relocation in SLICE_PARAMS: reference Y/UV base,CodedData */
#define RELOC_SLICE_PARAMS_PNW(dest, offset, buf)       pnw_cmdbuf_add_relocation(cmdbuf, (uint32_t*)(dest), buf, offset, 0XFFFFFFFF, 0, 0, 2,(uint32_t *)cmdbuf->slice_params_p)

/* operation number is inserted by DRM */
/*
#define pnw_cmdbuf_insert_command(cmdbuf,cmdhdr,size,hint)              \
    do { *cmdbuf->cmd_idx++ = ((cmdhdr) << 1) | ((size)<<8) | ((hint)<<16); } while(0)
*/

#define pnw_cmdbuf_insert_command_param(param)   \
    do { *cmdbuf->cmd_idx++ = param; } while(0)


#define pnw_cmdbuf_insert_reg_write(base, offset, value)        \
    do { *cmdbuf->cmd_idx++ = base + offset; *cmdbuf->cmd_idx++ = value; count++; } while(0)

void pnw_cmdbuf_insert_command_package(object_context_p obj_context,
                                       int32_t core,
                                       uint32_t cmd_id,
                                       psb_buffer_p command_data,
                                       uint32_t offset);

/*
 * Advances "obj_context" to the next cmdbuf
 *
 * Returns 0 on success
 */
int pnw_context_get_next_cmdbuf(object_context_p obj_context);

/*
 * Submits the current cmdbuf
 *
 * Returns 0 on success
 */
int pnw_context_submit_cmdbuf(object_context_p obj_context);

/*
 * Get a encode surface FRAMESKIP flag, and store it into frame_skip argument
 *
 * Returns 0 on success
 */
int pnw_surface_get_frameskip(psb_driver_data_p driver_data, psb_surface_p psb_surface, int *frame_skip);

/*
 * Flushes the pending cmdbuf
 *
 * Return 0 on success
 */
int pnw_context_flush_cmdbuf(object_context_p obj_context);

/*
 * Get the number of paralled cores used.
 *
 * Return the number of paralled cores used.
 */
int pnw_get_parallel_core_number(object_context_p obj_context);
#endif /* _PNW_CMDBUF_H_ */

