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
 *    Lin Edward <edward.lin@intel.com>
 *
 */

#ifndef _TNG_CMDBUF_H_
#define _TNG_CMDBUF_H_

#include "psb_drv_video.h"
#include "psb_surface.h"
#include "psb_buffer.h"

#include <stdint.h>

//#ifndef MTX_CMDWORD_ID_MASK
#define MTX_CMDWORD_ID_MASK     (0x7f)
#define MTX_CMDWORD_ID_SHIFT    (0)
#define MTX_CMDWORD_CORE_MASK   (0xff)
#define MTX_CMDWORD_CORE_SHIFT  (8)
#define MTX_CMDWORD_COUNT_MASK  (0x7fff)
#define MTX_CMDWORD_COUNT_SHIFT (16)
#define MTX_CMDWORD_INT_SHIFT   (7)
#define MTX_CMDWORD_INT_MASK    (1)
//#endif

#define TNG_CMDBUF_SEQ_HEADER_IDX       (0)
#define TNG_CMDBUF_PIC_HEADER_IDX       (1)
#define TNG_CMDBUF_MVC_SEQ_HEADER_IDX (2)

/* @{ */
#define SHIFT_MTX_CMDWORD_ID    (0)
#define MASK_MTX_CMDWORD_ID     (0xff << SHIFT_MTX_CMDWORD_ID)
#define SHIFT_MTX_CMDWORD_CORE  (8)
#define MASK_MTX_CMDWORD_CORE   (0xff << SHIFT_MTX_CMDWORD_CORE)
#define SHIFT_MTX_CMDWORD_COUNT (16)
#define MASK_MTX_CMDWORD_COUNT  (0xffff << SHIFT_MTX_CMDWORD_COUNT)
/* @} */

#define SHIFT_MTX_WBWORD_ID    (0)
#define MASK_MTX_WBWORD_ID     (0xff << SHIFT_MTX_WBWORD_ID)
#define SHIFT_MTX_WBWORD_CORE  (8)
#define MASK_MTX_WBWORD_CORE   (0xff << SHIFT_MTX_WBWORD_CORE)

#define tng_align_KB(x)  (((x) + (IMG_UINT32)(0xfff)) & (~(IMG_UINT32)(0xfff)))

struct tng_cmdbuf_s {
    struct psb_buffer_s buf;
    unsigned int size;

    /* Relocation records */
    unsigned char *reloc_base;
    struct drm_psb_reloc *reloc_idx;

    /* CMD stream data */
    int cmd_count;
    unsigned char *cmd_base;
    unsigned char *cmd_start;
    IMG_UINT32 *cmd_idx;
    IMG_UINT32 *cmd_idx_saved[3]; /* idx saved for dual-core adjustion */

    unsigned int mem_size;

    /* JPEG quantization tables buffer*/
    struct psb_buffer_s jpeg_pic_params;
    void *jpeg_pic_params_p;

    /* JPEG MTX setup buffer */
    struct psb_buffer_s jpeg_header_mem;
    void *jpeg_header_mem_p;

    /* JPEG MTX setup interface buffer */
    struct psb_buffer_s jpeg_header_interface_mem;
    void *jpeg_header_interface_mem_p;


    /*buffer information g_apsCmdDataInfo */
    struct psb_buffer_s frame_mem;
    unsigned char *frame_mem_p;
    unsigned int frame_mem_index;

    /*picuture management g_apsCmdDataInfo */
    void *writeback_mem_p;

    /*buffer information g_apsCmdDataInfo
    struct psb_buffer_s coded_mem;
    void *coded_mem_p;
    */

    /* all frames share one topaz param buffer which contains InParamBase
     * AboveParam/BellowParam, and the buffer allocated when the context is created
     */
    struct psb_buffer_s *topaz_in_params_I;
    void *topaz_in_params_I_p;

    struct psb_buffer_s *topaz_in_params_P;
    void *topaz_in_params_P_p;

    struct psb_buffer_s *topaz_below_params;
    void *topaz_below_params_p;

    /* Every frame has its own PIC_PARAMS, SLICE_PARAMS and HEADER mem
     */

/* 
    PicParams:
    struct psb_buffer_s pic_params;
    void *pic_params_p;
*/
        
    /* Referenced buffers */
    psb_buffer_p *buffer_refs;
    int buffer_refs_count;
    int buffer_refs_allocated;
};

typedef struct tng_cmdbuf_s *tng_cmdbuf_p;

/*
 * Create command buffer
 */
VAStatus tng_cmdbuf_create(object_context_p obj_context,
                           psb_driver_data_p driver_data,
                           tng_cmdbuf_p cmdbuf
                          );

/*
 * Destroy buffer
 */
void tng_cmdbuf_destroy(tng_cmdbuf_p cmdbuf);

/*
 * Reset buffer & map
 *
 * Returns 0 on success
 */
int tng_cmdbuf_reset(tng_cmdbuf_p cmdbuf);

/*
 * Unmap buffer
 *
 * Returns 0 on success
 */
int tng_cmdbuf_unmap(tng_cmdbuf_p cmdbuf);

/*
 * Reference an addtional buffer "buf" in the command stream
 * Returns a reference index that can be used to refer to "buf" in
 * relocation records, on error -1 is returned.
 */
int tng_cmdbuf_buffer_ref(tng_cmdbuf_p cmdbuf, psb_buffer_p buf);

/* Creates a relocation record for a DWORD in the mapped "cmdbuf" at address
 * "addr_in_cmdbuf"
 * The relocation is based on the device virtual address of "ref_buffer"
 * "buf_offset" is be added to the device virtual address, and the sum is then
 * right shifted with "align_shift".
 * "mask" determines which bits of the target DWORD will be updated with the so
 * constructed address. The remaining bits will be filled with bits from "background".
 */
void tng_cmdbuf_add_relocation(tng_cmdbuf_p cmdbuf,
                               IMG_UINT32 *addr_in_dst_buffer,/*addr of dst_buffer for the DWORD*/
                               psb_buffer_p ref_buffer,
                               IMG_UINT32 buf_offset,
                               IMG_UINT32 mask,
                               IMG_UINT32 background,
                               IMG_UINT32 align_shift,
                               IMG_UINT32 dst_buffer, /*Index of the list refered by cmdbuf->buffer_refs */
                               IMG_UINT32 *start_of_dst_buffer);

#define TNG_RELOC_CMDBUF_START(dest, offset, buf)    tng_cmdbuf_add_relocation(cmdbuf, (IMG_UINT32*)(dest), buf, offset, 0XFFFFFFFF, 0, 0, 0, (IMG_UINT32 *)(cmdbuf->cmd_start))
/* do relocation in IMG_BUFFER_PARAMS: reference Y/UV base,CodedData */
#define TNG_RELOC_CMDBUF_FRAMES(dest, offset, buf)   tng_cmdbuf_add_relocation(cmdbuf, (IMG_UINT32*)(dest), buf, offset, 0XFFFFFFFF, 0, 0, 3,(IMG_UINT32 *)(cmdbuf->frame_mem_p))

/* do relocation in PIC_PARAMS: src/dst Y/UV base, InParamsBase, CodeBase, BellowParamsBase, AboveParamsBase
#define RELOC_PIC_PARAMS_PTG(dest, offset, buf) tng_cmdbuf_add_relocation(cmdbuf, (IMG_UINT32*)(dest), buf, offset, 0XFFFFFFFF, 0, 0, 1, (uint32_t *)cmdbuf->pic_params_p)
*/

/* do relocation in MTX_ENC_PARAMS */
#define RELOC_MTXCTX_PARAMS_PTG(dest, offset, buf)       tng_cmdbuf_add_relocation(cmdbuf, (IMG_UINT32*)(dest), buf, offset, 0XFFFFFFFF, 0, 0, 3,(uint32_t *)cmdbuf->mtx_ctx_mem_p)

/* do relocation in SLICE_PARAMS: reference Y/UV base,CodedData */
//#define RELOC_SLICE_PARAMS_PTG(dest, offset, buf)       tng_cmdbuf_add_relocation(cmdbuf, (IMG_UINT32*)(dest), buf, offset, 0XFFFFFFFF, 0, 0, 2,(uint32_t *)cmdbuf->slice_mem_p)


/* do relocation in IMG_BUFFER_PARAMS: reference Y/UV base,CodedData */
#define RELOC_PICMGMT_PARAMS_PTG(dest, offset, buf)       tng_cmdbuf_add_relocation(cmdbuf, (IMG_UINT32*)(dest), buf, offset, 0XFFFFFFFF, 0, 0, 3,(uint32_t *)cmdbuf->picmgmt_mem_p)

/* do relocation in PIC_PARAMS: src/dst Y/UV base, InParamsBase, CodeBase, BellowParamsBase, AboveParamsBase */
#define RELOC_JPEG_PIC_PARAMS_PTG(dest, offset, buf) tng_cmdbuf_add_relocation(cmdbuf, (IMG_UINT32*)(dest), buf, offset, 0XFFFFFFFF, 0, 0, 1, (IMG_UINT32 *)cmdbuf->jpeg_pic_params_p)

/* do relocation in IMG_BUFFER_PARAMS: reference Y/UV base,CodedData
#define RELOC_CODED_PARAMS_PTG(dest, offset, buf)       tng_cmdbuf_add_relocation(cmdbuf, (IMG_UINT32*)(dest), buf, offset, 0XFFFFFFFF, 0, 0, 3,(uint32_t *)cmdbuf->coded_mem_p)
*/

/* operation number is inserted by DRM */
/*
#define tng_cmdbuf_insert_command(cmdbuf,cmdhdr,size,hint)              \
    do { *cmdbuf->cmd_idx++ = ((cmdhdr) << 1) | ((size)<<8) | ((hint)<<16); } while(0)
*/

#define tng_cmdbuf_insert_command_param(param)   \
    do { *cmdbuf->cmd_idx++ = param; } while(0)


#define tng_cmdbuf_insert_reg_write(topaz_reg, base, offset, value)        \
    do { *cmdbuf->cmd_idx++ = topaz_reg; *cmdbuf->cmd_idx++ = base + offset; *cmdbuf->cmd_idx++ = value; count++; } while(0)

void tng_cmdbuf_insert_command(
    object_context_p obj_context, IMG_UINT32 core,
    IMG_UINT32 cmd_id, IMG_UINT32 cmd_data,
    psb_buffer_p data_addr, IMG_UINT32 offset);

/*
 * Advances "obj_context" to the next cmdbuf
 * Returns 0 on success
 */
int tng_context_get_next_cmdbuf(object_context_p obj_context);

/*
 * Submits the current cmdbuf
 * Returns 0 on success
 */
int tng_context_submit_cmdbuf(object_context_p obj_context);

/*
 * Get a encode surface FRAMESKIP flag, and store it into frame_skip argument
 * Returns 0 on success
 */
int tng_surface_get_frameskip(psb_driver_data_p driver_data, psb_surface_p psb_surface, int *frame_skip);

/*
 * Flushes the pending cmdbuf
 * Return 0 on success
 */
int tng_context_flush_cmdbuf(object_context_p obj_context);

void tng_cmdbuf_mem_unmap(tng_cmdbuf_p cmdbuf);

void tng_cmdbuf_set_phys(IMG_UINT32 *dest_buf, int dest_num, psb_buffer_p ref_buf, unsigned int ref_ofs, unsigned int ref_len);
int tng_get_pipe_number(object_context_p obj_context);
VAStatus tng_set_frame_skip_flag(object_context_p obj_context);

#endif /* _TNG_CMDBUF_H_ */

