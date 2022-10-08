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


#include "psb_cmdbuf.h"

#include <unistd.h>
#include <stdio.h>

#include "hwdefs/mem_io.h"
#include "hwdefs/msvdx_offsets.h"
#include "hwdefs/dma_api.h"
#include "hwdefs/reg_io2.h"
#include "hwdefs/msvdx_vec_reg_io2.h"
#include "hwdefs/msvdx_vdmc_reg_io2.h"
#include "hwdefs/msvdx_mtx_reg_io2.h"
#include "hwdefs/msvdx_dmac_linked_list.h"
#include "hwdefs/msvdx_rendec_mtx_slice_cntrl_reg_io2.h"
#include "hwdefs/dxva_cmdseq_msg.h"
#include "hwdefs/dxva_fw_ctrl.h"
#include "hwdefs/fwrk_msg_mem_io.h"
#include "hwdefs/dxva_msg.h"
#include "hwdefs/msvdx_cmds_io2.h"
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>

#include "psb_def.h"
#include "psb_drv_debug.h"
#ifndef BAYTRAIL
#include "psb_ws_driver.h"
#endif
#include <wsbm/wsbm_pool.h>
#include <wsbm/wsbm_manager.h>
#include <wsbm/wsbm_util.h>
#include <wsbm/wsbm_fencemgr.h>

/*
 * Buffer layout:
 *         cmd_base <= cmd_idx < CMD_END() == lldma_base
 *         lldma_base <= lldma_idx < LLDMA_END() == (cmd_base + size)
 *
 * Reloc buffer layout:
 *         MTX_msg < reloc_base == MTX_msg + MTXMSG_SIZE
 *         reloc_base <= reloc_idx < RELOC_END() == (MTX_msg + reloc_size)
 */
#define MTXMSG_END(cmdbuf)    (cmdbuf->reloc_base)
#define RELOC_END(cmdbuf)     (cmdbuf->MTX_msg + cmdbuf->reloc_size)

#define CMD_END(cmdbuf)       (cmdbuf->lldma_base)
#define LLDMA_END(cmdbuf)     (cmdbuf->cmd_base + cmdbuf->size)

#define MTXMSG_SIZE           (0x1000)
#define RELOC_SIZE            (0x3000)

#define CMD_SIZE              (0x3000)
#define LLDMA_SIZE            (0x2000)

#define MTXMSG_MARGIN         (0x0040)
#define RELOC_MARGIN          (0x0800)

#define CMD_MARGIN            (0x0400)
#define LLDMA_MARGIN          (0x0400)
#define PSB_SLICE_EXTRACT_UPDATE (0x2)

/*
 * Create command buffer
 */
VAStatus psb_cmdbuf_create(object_context_p obj_context, psb_driver_data_p driver_data,
                           psb_cmdbuf_p cmdbuf
                          )
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    unsigned int size = CMD_SIZE + LLDMA_SIZE;
    unsigned int reloc_size = MTXMSG_SIZE + RELOC_SIZE;
    unsigned int regio_size = (obj_context->picture_width >> 4) * (obj_context->picture_height >> 4) * 172;

    cmdbuf->size = 0;
    cmdbuf->reloc_size = 0;
    cmdbuf->regio_size = 0;
    cmdbuf->MTX_msg = NULL;
    cmdbuf->cmd_base = NULL;
    cmdbuf->regio_base = NULL;
    cmdbuf->cmd_idx = NULL;
    cmdbuf->regio_idx = NULL;
    cmdbuf->cmd_bitstream_size = NULL;
    cmdbuf->lldma_base = NULL;
    cmdbuf->lldma_idx = NULL;
    cmdbuf->reloc_base = NULL;
    cmdbuf->reloc_idx = NULL;
    cmdbuf->reg_start = NULL;
    cmdbuf->rendec_block_start = NULL;
    cmdbuf->rendec_chunk_start = NULL;
    cmdbuf->skip_block_start = NULL;
    cmdbuf->last_next_segment_cmd = NULL;
    cmdbuf->buffer_refs_count = 0;
    cmdbuf->buffer_refs_allocated = 10;
    cmdbuf->buffer_refs = (psb_buffer_p *) calloc(1, sizeof(psb_buffer_p) * cmdbuf->buffer_refs_allocated);
    if (NULL == cmdbuf->buffer_refs) {
        cmdbuf->buffer_refs_allocated = 0;
        vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
    }
    if (VA_STATUS_SUCCESS == vaStatus) {
        vaStatus = psb_buffer_create(driver_data, size, psb_bt_cpu_vpu, &cmdbuf->buf);
        cmdbuf->size = size;
    }
    if (VA_STATUS_SUCCESS == vaStatus) {
        vaStatus = psb_buffer_create(driver_data, reloc_size, psb_bt_cpu_only, &cmdbuf->reloc_buf);
        cmdbuf->reloc_size = reloc_size;
    }
    if (VA_STATUS_SUCCESS == vaStatus) {
        vaStatus = psb_buffer_create(driver_data, regio_size, psb_bt_cpu_only, &cmdbuf->regio_buf);
        cmdbuf->regio_size = regio_size;
    }

    if (VA_STATUS_SUCCESS != vaStatus) {
        psb_cmdbuf_destroy(cmdbuf);
    }
    return vaStatus;
}

/*
 * Destroy buffer
 */
void psb_cmdbuf_destroy(psb_cmdbuf_p cmdbuf)
{
    if (cmdbuf->size) {
        psb_buffer_destroy(&cmdbuf->buf);
        cmdbuf->size = 0;
    }
    if (cmdbuf->reloc_size) {
        psb_buffer_destroy(&cmdbuf->reloc_buf);
        cmdbuf->reloc_size = 0;
    }
    if (cmdbuf->regio_size) {
        psb_buffer_destroy(&cmdbuf->regio_buf);
        cmdbuf->regio_size = 0;
    }
    if (cmdbuf->buffer_refs_allocated) {
        free(cmdbuf->buffer_refs);
        cmdbuf->buffer_refs = NULL;
        cmdbuf->buffer_refs_allocated = 0;
    }
}

/*
 * Reset buffer & map
 *
 * Returns 0 on success
 */
int psb_cmdbuf_reset(psb_cmdbuf_p cmdbuf)
{
    int ret;

    cmdbuf->MTX_msg = NULL;
    cmdbuf->cmd_base = NULL;
    cmdbuf->cmd_idx = NULL;
    cmdbuf->cmd_bitstream_size = NULL;
    cmdbuf->lldma_base = NULL;
    cmdbuf->lldma_idx = NULL;
    cmdbuf->reloc_base = NULL;
    cmdbuf->reloc_idx = NULL;
    cmdbuf->last_next_segment_cmd = NULL;

    cmdbuf->buffer_refs_count = 0;
    cmdbuf->cmd_count = 0;
    cmdbuf->deblock_count = 0;
    cmdbuf->oold_count = 0;
    cmdbuf->host_be_opp_count = 0;
    cmdbuf->frame_info_count = 0;
#ifdef SLICE_HEADER_PARSING
    cmdbuf->parse_count = 0;
#endif
    ret = psb_buffer_map(&cmdbuf->buf, &cmdbuf->cmd_base);
    if (ret) {
        return ret;
    }
    ret = psb_buffer_map(&cmdbuf->reloc_buf, &cmdbuf->MTX_msg);
    if (ret) {
        psb_buffer_unmap(&cmdbuf->buf);
        return ret;
    }

    cmdbuf->cmd_start = cmdbuf->cmd_base;
    cmdbuf->cmd_idx = (uint32_t *) cmdbuf->cmd_base;
    cmdbuf->cmd_bitstream_size = NULL;
    cmdbuf->lldma_base = cmdbuf->cmd_base + CMD_SIZE;
    cmdbuf->lldma_idx = cmdbuf->lldma_base;

    cmdbuf->reloc_base = cmdbuf->MTX_msg + MTXMSG_SIZE;
    cmdbuf->reloc_idx = (struct drm_psb_reloc *) cmdbuf->reloc_base;

    /* Add ourselves to the buffer list */
    psb_cmdbuf_buffer_ref(cmdbuf, &cmdbuf->reloc_buf); /* reloc buf == 0 */
    psb_cmdbuf_buffer_ref(cmdbuf, &cmdbuf->buf); /* cmd buf == 1 */
    return ret;
}

/*
 * Unmap buffer
 *
 * Returns 0 on success
 */
int psb_cmdbuf_unmap(psb_cmdbuf_p cmdbuf)
{
    cmdbuf->MTX_msg = NULL;
    cmdbuf->cmd_base = NULL;
    cmdbuf->cmd_start = NULL;
    cmdbuf->cmd_idx = NULL;
    cmdbuf->cmd_bitstream_size = NULL;
    cmdbuf->lldma_base = NULL;
    cmdbuf->lldma_idx = NULL;
    cmdbuf->reloc_base = NULL;
    cmdbuf->reloc_idx = NULL;
    cmdbuf->cmd_count = 0;
    psb_buffer_unmap(&cmdbuf->buf);
    psb_buffer_unmap(&cmdbuf->reloc_buf);
    return 0;
}


/*
 * Reference an addtional buffer "buf" in the command stream
 * Returns a reference index that can be used to refer to "buf" in
 * relocation records, -1 on error
 */
int psb_cmdbuf_buffer_ref(psb_cmdbuf_p cmdbuf, psb_buffer_p buf)
{
    int item_loc = 0;

    // buf->next = NULL; /* buf->next only used for buffer list validation */
    while ((item_loc < cmdbuf->buffer_refs_count)
           && (wsbmKBufHandle(wsbmKBuf(cmdbuf->buffer_refs[item_loc]->drm_buf))
               != wsbmKBufHandle(wsbmKBuf(buf->drm_buf)))) {
        item_loc++;
    }
    if (item_loc == cmdbuf->buffer_refs_count) {
        /* Add new entry */
        if (item_loc >= cmdbuf->buffer_refs_allocated) {
            /* Allocate more entries */
            int new_size = cmdbuf->buffer_refs_allocated + 10;
            psb_buffer_p *new_array;
            new_array = (psb_buffer_p *) calloc(1, sizeof(psb_buffer_p) * new_size);
            if (NULL == new_array) {
                return -1; /* Allocation failure */
            }
            memcpy(new_array, cmdbuf->buffer_refs, sizeof(psb_buffer_p) * cmdbuf->buffer_refs_allocated);
            free(cmdbuf->buffer_refs);
            cmdbuf->buffer_refs_allocated = new_size;
            cmdbuf->buffer_refs = new_array;
        }
        cmdbuf->buffer_refs[item_loc] = buf;
        cmdbuf->buffer_refs_count++;
        buf->status = psb_bs_queued;

        buf->next = NULL;
        buf->unfence_flag = 0;
    }

    /* only for RAR buffers */
    if ((cmdbuf->buffer_refs[item_loc] != buf)
        && (buf->rar_handle != 0)) {
        psb_buffer_p tmp = cmdbuf->buffer_refs[item_loc];
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "RAR: found same drm buffer with different psb buffer, link them\n",
                                 tmp, buf);
        while ((tmp->next != NULL)) {
            tmp = tmp->next;
            if (tmp == buf) /* found same buffer */
                break;
        }

        if (tmp != buf) {
            tmp->next = buf; /* link it */
            buf->status = psb_bs_queued;
            buf->next = NULL;
        } else {
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "RAR: buffer aleady in the list, skip\n",
                                     tmp, buf);
        }
    }

    return item_loc;
}

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
                               uint32_t dst_buffer) /* 0 = reloc buf, 1 = cmdbuf, 2 = for host reloc */
{
    struct drm_psb_reloc *reloc = cmdbuf->reloc_idx;
    uint64_t presumed_offset = wsbmBOOffsetHint(ref_buffer->drm_buf);

    /* Check that address is within buffer range */
    if (dst_buffer) {
        ASSERT(((unsigned char *)(addr_in_cmdbuf)) >= cmdbuf->cmd_base);
        ASSERT(((unsigned char *)(addr_in_cmdbuf)) < LLDMA_END(cmdbuf));
        reloc->where = addr_in_cmdbuf - (uint32_t *) cmdbuf->cmd_base; /* Location in DWORDs */
    } else {
        ASSERT(((unsigned char *)(addr_in_cmdbuf)) >= cmdbuf->MTX_msg);
        ASSERT(((unsigned char *)(addr_in_cmdbuf)) < MTXMSG_END(cmdbuf));
        reloc->where = addr_in_cmdbuf - (uint32_t *) cmdbuf->MTX_msg; /* Location in DWORDs */
    }

    reloc->buffer = psb_cmdbuf_buffer_ref(cmdbuf, ref_buffer);
    ASSERT(reloc->buffer != -1);

    reloc->reloc_op = PSB_RELOC_OP_OFFSET;

    psb__trace_message("[RE] Reloc at offset %08x (%08x), offset = %08x background = %08x buffer = %d (%08x)\n",
        reloc->where, reloc->where << 2, buf_offset, background, reloc->buffer, presumed_offset);

    if (presumed_offset) {
        uint32_t new_val =  presumed_offset + buf_offset;
        new_val = ((new_val >> align_shift) << (align_shift << PSB_RELOC_ALSHIFT_SHIFT));
        new_val = (background & ~mask) | (new_val & mask);
        *addr_in_cmdbuf = new_val;
    } else {
        *addr_in_cmdbuf = PSB_RELOC_MAGIC;
    }

    reloc->mask = mask;
    reloc->shift = align_shift << PSB_RELOC_ALSHIFT_SHIFT;
    reloc->pre_add =  buf_offset;
    reloc->background = background;
    reloc->dst_buffer = dst_buffer;
    cmdbuf->reloc_idx++;

    ASSERT(((unsigned char *)(cmdbuf->reloc_idx)) < RELOC_END(cmdbuf));
}

/*
 * Advances "obj_context" to the next cmdbuf
 *
 * Returns 0 on success
 */
int psb_context_get_next_cmdbuf(object_context_p obj_context)
{
    psb_cmdbuf_p cmdbuf;
    int ret;

    if (obj_context->cmdbuf) {
        return 0;
    }

    obj_context->cmdbuf_current++;
    if (obj_context->cmdbuf_current >= PSB_MAX_CMDBUFS) {
        obj_context->cmdbuf_current = 0;
    }
    cmdbuf = obj_context->cmdbuf_list[obj_context->cmdbuf_current];
    ret = psb_cmdbuf_reset(cmdbuf);
    if (!ret) {
        /* Success */
        obj_context->cmdbuf = cmdbuf;
    }
    return ret;
}


static unsigned
psbTimeDiff(struct timeval *now, struct timeval *then)
{
    long long val;

    val = now->tv_sec - then->tv_sec;
    val *= 1000000LL;
    val += now->tv_usec;
    val -= then->tv_usec;
    if (val < 1LL)
        val = 1LL;

    return (unsigned) val;
}

/*
 * This is the user-space do-it-all interface to the drm cmdbuf ioctl.
 * It allows different buffers as command- and reloc buffer. A list of
 * cliprects to apply and whether to copy the clipRect content to all
 * scanout buffers (damage = 1).
 */
/*
 * Don't add debug statements in this function, it gets called with the
 * DRM lock held and output to an X terminal can cause X to deadlock
 */
static int
psbDRMCmdBuf(int fd, int ioctl_offset, psb_buffer_p *buffer_list, int buffer_count, unsigned cmdBufHandle,
             unsigned cmdBufOffset, unsigned cmdBufSize,
             unsigned relocBufHandle, unsigned relocBufOffset,
             unsigned numRelocs, int __maybe_unused damage,
             unsigned engine, unsigned fence_flags, struct psb_ttm_fence_rep *fence_arg)
{
    drm_psb_cmdbuf_arg_t ca;
    struct psb_validate_arg *arg_list;
    int i;
    int ret;
    struct timeval then, now;
    Bool have_then = FALSE;
    uint64_t mask = PSB_GPU_ACCESS_MASK;

    arg_list = (struct psb_validate_arg *) calloc(1, sizeof(struct psb_validate_arg) * buffer_count);
    if (arg_list == NULL) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "Malloc failed \n");
        return -ENOMEM;
    }

    for (i = 0; i < buffer_count; i++) {
        struct psb_validate_arg *arg = &(arg_list[i]);
        struct psb_validate_req *req = &arg->d.req;

        req->next = (unsigned long) & (arg_list[i+1]);

        req->buffer_handle = wsbmKBufHandle(wsbmKBuf(buffer_list[i]->drm_buf));
        //req->group = 0;
        req->set_flags = (PSB_GPU_ACCESS_READ | PSB_GPU_ACCESS_WRITE) & mask;
        req->clear_flags = (~(PSB_GPU_ACCESS_READ | PSB_GPU_ACCESS_WRITE)) & mask;

        req->presumed_gpu_offset = (uint64_t)wsbmBOOffsetHint(buffer_list[i]->drm_buf);
        req->presumed_flags = PSB_USE_PRESUMED;
        req->pad64 = (uint32_t)buffer_list[i]->pl_flags;
#ifndef BAYTRAIL
        req->unfence_flag = buffer_list[i]->unfence_flag;
#endif
    }
    arg_list[buffer_count-1].d.req.next = 0;

    ca.buffer_list = (uint64_t)((unsigned long)arg_list);
    ca.fence_arg = (uint64_t)((unsigned long)fence_arg);

    ca.cmdbuf_handle = cmdBufHandle;
    ca.cmdbuf_offset = cmdBufOffset;
    ca.cmdbuf_size = cmdBufSize;

    ca.reloc_handle = relocBufHandle;
    ca.reloc_offset = relocBufOffset;
    ca.num_relocs = numRelocs;

    ca.fence_flags = fence_flags;
    ca.engine = engine;

#if 0
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "PSB submit: buffer_list   = %08x\n", ca.buffer_list);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "PSB submit: clip_rects    = %08x\n", ca.clip_rects);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "PSB submit: cmdbuf_handle = %08x\n", ca.cmdbuf_handle);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "PSB submit: cmdbuf_offset = %08x\n", ca.cmdbuf_offset);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "PSB submit: cmdbuf_size   = %08x\n", ca.cmdbuf_size);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "PSB submit: reloc_handle  = %08x\n", ca.reloc_handle);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "PSB submit: reloc_offset  = %08x\n", ca.reloc_offset);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "PSB submit: num_relocs    = %08x\n", ca.num_relocs);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "PSB submit: engine        = %08x\n", ca.engine);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "PSB submit: fence_flags   = %08x\n", ca.fence_flags);
#endif

    /*
     * X server Signals will clobber the kernel time out mechanism.
     * we need a user-space timeout as well.
     */
    do {
        ret = drmCommandWrite(fd, ioctl_offset, &ca, sizeof(ca));
        if (ret == EAGAIN) {
            if (!have_then) {
                if (gettimeofday(&then, NULL)) {
                    drv_debug_msg(VIDEO_DEBUG_ERROR, "Gettimeofday error.\n");
                    break;
                }

                have_then = TRUE;
            }
            if (gettimeofday(&now, NULL)) {
                drv_debug_msg(VIDEO_DEBUG_ERROR, "Gettimeofday error.\n");
                break;
            }

        }
    } while ((ret == EAGAIN) && (psbTimeDiff(&now, &then) < PSB_TIMEOUT_USEC));

    if (ret) {
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "command write return is %d\n", ret);
        goto out;
    }

    for (i = 0; i < buffer_count; i++) {
        struct psb_validate_arg *arg = &(arg_list[i]);
        struct psb_validate_rep *rep = &arg->d.rep;

#ifndef BAYTRAIL
        if (arg->d.req.unfence_flag)
            continue;
#endif

        if (!arg->handled) {
            ret = -EFAULT;
            goto out;
        }
        if (arg->ret != 0) {
            ret = arg->ret;
            goto out;
        }
        wsbmUpdateKBuf(wsbmKBuf(buffer_list[i]->drm_buf),
                       rep->gpu_offset, rep->placement, rep->fence_type_mask);
    }

out:
    free(arg_list);
    for (i = 0; i < buffer_count; i++) {
        /*
         * Buffer no longer queued in userspace
         */
        psb_buffer_p tmp = buffer_list[i];

        /*
         * RAR slice buffer/surface buffer are share one BO, and then only one in
         * buffer_list, but they are linked in psb_cmdbuf_buffer_ref

         */
        if (buffer_list[i]->rar_handle == 0)
            tmp->next = NULL; /* don't loop for non RAR buffer, "next" may be not initialized  */

        do {
            psb_buffer_p p = tmp;

            tmp = tmp->next;
            switch (p->status) {
            case psb_bs_queued:
                p->status = psb_bs_ready;
                break;

            case psb_bs_abandoned:
                psb_buffer_destroy(p);
                free(p);
                break;

            default:
                /* Not supposed to happen */
                ASSERT(0);
            }
        } while (tmp);
    }

    return ret;
}

#if 0
int psb_fence_destroy(struct _WsbmFenceObject *pFence)
{
    wsbmFenceUnreference(&pFence);

    return 0;
}

struct _WsbmFenceObject *
psb_fence_wait(psb_driver_data_p driver_data,
               struct psb_ttm_fence_rep *fence_rep, int *status)

{
    struct _WsbmFenceObject *fence = NULL;
    int ret = -1;

    /* copy fence information */
    if (fence_rep->error != 0) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "drm failed to create a fence"
                           " and has idled the HW\n");
        DEBUG_FAILURE_RET;
        return NULL;
    }

    fence = wsbmFenceCreate(driver_data->fence_mgr, fence_rep->fence_class,
                            fence_rep->fence_type,
                            (unsigned char *)fence_rep->handle,
                            0);
    if (fence)
        *status = wsbmFenceFinish(fence, fence_rep->fence_type, 0);

    return fence;
}
#endif

/*
 * Closes the last segment
 */
static void psb_cmdbuf_close_segment(psb_cmdbuf_p __maybe_unused cmdbuf)
{
#if 0
    uint32_t bytes_used = ((unsigned char *) cmdbuf->cmd_idx - cmdbuf->cmd_start) % MTX_SEG_SIZE;
    unsigned char *segment_start = (unsigned char *) cmdbuf->cmd_idx - bytes_used;
    uint32_t lldma_record_offset = psb_cmdbuf_lldma_create(cmdbuf,
                                   &(cmdbuf->buf), (segment_start - cmdbuf->cmd_base) /* offset */,
                                   bytes_used,
                                   0 /* destination offset */,
                                   LLDMA_TYPE_RENDER_BUFF_MC);
    uint32_t cmd = CMD_NEXT_SEG;
    RELOC_SHIFT4(*cmdbuf->last_next_segment_cmd, lldma_record_offset, cmd, &(cmdbuf->buf));
    *(cmdbuf->last_next_segment_cmd + 1) = bytes_used;
#endif
}

/* Issue deblock cmd, HW will do deblock instead of host */
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
                                  uint32_t is_oold)
{
    psb_cmdbuf_p cmdbuf = obj_context->cmdbuf;
    psb_driver_data_p driver_data = obj_context->driver_data;
    uint32_t msg_size = FW_DEVA_DEBLOCK_SIZE;
    unsigned int item_size = FW_DEVA_DECODE_SIZE; /* Size of a render/deocde msg */
    FW_VA_DEBLOCK_MSG *deblock_msg;

    uint32_t *msg = (uint32_t *)(cmdbuf->MTX_msg + item_size * cmdbuf->cmd_count);

    memset(msg, 0, sizeof(FW_VA_DEBLOCK_MSG));
    deblock_msg = (FW_VA_DEBLOCK_MSG *)msg;

    deblock_msg->header.bits.msg_size = FW_DEVA_DEBLOCK_SIZE;
    if (is_oold)
        deblock_msg->header.bits.msg_type = VA_MSGID_OOLD_MFLD;
    else
        deblock_msg->header.bits.msg_type = VA_MSGID_DEBLOCK_MFLD;
    deblock_msg->flags.bits.flags = FW_VA_RENDER_HOST_INT | FW_VA_RENDER_IS_LAST_SLICE | FW_DEVA_DEBLOCK_ENABLE;
    deblock_msg->flags.bits.slice_type = field_type;
    deblock_msg->operating_mode = obj_context->operating_mode;
    deblock_msg->mmu_context.bits.context = (uint8_t)(obj_context->msvdx_context);
    deblock_msg->pic_size.bits.frame_height_mb = (uint16_t)frame_height_mb;
    deblock_msg->pic_size.bits.pic_width_mb = (uint16_t)picture_widht_mb;
    deblock_msg->ext_stride_a = ext_stride_a;
    deblock_msg->rotation_flags = rotation_flags;

    RELOC_MSG(deblock_msg->address_a0, buf_a->buffer_ofs, buf_a);
    RELOC_MSG(deblock_msg->address_a1, buf_a->buffer_ofs + chroma_offset_a, buf_a);
    if (buf_b) {
        RELOC_MSG(deblock_msg->address_b0, buf_b->buffer_ofs, buf_b);
        RELOC_MSG(deblock_msg->address_b1, buf_b->buffer_ofs + chroma_offset_b, buf_b);
    }

    RELOC_MSG(deblock_msg->mb_param_address, colocate_buffer->buffer_ofs, colocate_buffer);
    cmdbuf->deblock_count++;
    return 0;
}

#ifdef PSBVIDEO_MSVDX_EC
int psb_context_submit_host_be_opp(object_context_p obj_context,
                                  psb_buffer_p buf_a,
                                  psb_buffer_p buf_b,
                                  psb_buffer_p __maybe_unused buf_c,
                                  uint32_t picture_widht_mb,
                                  uint32_t frame_height_mb,
                                  uint32_t rotation_flags,
                                  uint32_t field_type,
                                  uint32_t ext_stride_a,
                                  uint32_t chroma_offset_a,
                                  uint32_t chroma_offset_b)
{
    psb_cmdbuf_p cmdbuf = obj_context->cmdbuf;
    psb_driver_data_p driver_data = obj_context->driver_data;
    uint32_t msg_size = sizeof(FW_VA_DEBLOCK_MSG);
    unsigned int item_size = FW_DEVA_DECODE_SIZE; /* Size of a render/deocde msg */
    FW_VA_DEBLOCK_MSG *deblock_msg;

    uint32_t *msg = (uint32_t *)(cmdbuf->MTX_msg + item_size * cmdbuf->cmd_count + cmdbuf->deblock_count * msg_size);

    memset(msg, 0, sizeof(FW_VA_DEBLOCK_MSG));
    deblock_msg = (FW_VA_DEBLOCK_MSG *)msg;

    deblock_msg->header.bits.msg_size = FW_DEVA_DEBLOCK_SIZE;
    deblock_msg->header.bits.msg_type = VA_MSGID_HOST_BE_OPP_MFLD;
    deblock_msg->flags.bits.flags = FW_VA_RENDER_HOST_INT | FW_ERROR_DETECTION_AND_RECOVERY;
    deblock_msg->flags.bits.slice_type = field_type;
    deblock_msg->operating_mode = obj_context->operating_mode;
    deblock_msg->mmu_context.bits.context = (uint8_t)(obj_context->msvdx_context);
    deblock_msg->pic_size.bits.frame_height_mb = (uint16_t)frame_height_mb;
    deblock_msg->pic_size.bits.pic_width_mb = (uint16_t)picture_widht_mb;
    deblock_msg->ext_stride_a = ext_stride_a;
    deblock_msg->rotation_flags = rotation_flags;

    RELOC_MSG(deblock_msg->address_a0, buf_a->buffer_ofs, buf_a);
    RELOC_MSG(deblock_msg->address_a1, buf_a->buffer_ofs + chroma_offset_a, buf_a);
    RELOC_MSG(deblock_msg->address_b0, buf_b->buffer_ofs, buf_b);
    RELOC_MSG(deblock_msg->address_b1, buf_b->buffer_ofs + chroma_offset_b, buf_b);

    deblock_msg->mb_param_address = wsbmKBufHandle(wsbmKBuf(buf_a->drm_buf));
    cmdbuf->deblock_count++;
    return 0;
}
#endif
/*
 * Submits the current cmdbuf
 *
 * Returns 0 on success
 */
int psb_context_submit_cmdbuf(object_context_p obj_context)
{
    psb_cmdbuf_p cmdbuf = obj_context->cmdbuf;
    psb_driver_data_p driver_data = obj_context->driver_data;
    unsigned int item_size = FW_DEVA_DECODE_SIZE; /* Size of a render/deocde msg */
    int ret;


    uint32_t cmdbuffer_size = (unsigned char *) cmdbuf->cmd_idx - cmdbuf->cmd_start; // In bytes

    if (cmdbuf->last_next_segment_cmd) {
        cmdbuffer_size = cmdbuf->first_segment_size;
        psb_cmdbuf_close_segment(cmdbuf);
    }

    uint32_t msg_size = item_size;
    uint32_t *msg = (uint32_t *)(cmdbuf->MTX_msg + cmdbuf->cmd_count * msg_size + cmdbuf->frame_info_count * FW_VA_FRAME_INFO_SIZE);

    if (psb_video_trace_fp && (psb_video_trace_level & CMDMSG_TRACE)) {
        debug_cmd_start[cmdbuf->cmd_count] = cmdbuf->cmd_start - cmdbuf->cmd_base;
        debug_cmd_size[cmdbuf->cmd_count] = (unsigned char *) cmdbuf->cmd_idx - cmdbuf->cmd_start;
        debug_cmd_count = cmdbuf->cmd_count + 1;
    }

/*
    static int c = 0;
    static char pFileName[30];


    sprintf(pFileName , "cmdbuf%i.txt", c++);
    FILE* pF = fopen(pFileName, "w");

    fwrite(cmdbuf->cmd_start, 1, cmdbuffer_size, pF);
    fclose(pF);
*/
    ret = psb_cmdbuf_dump((unsigned int *)cmdbuf->cmd_start, cmdbuffer_size);
    if(ret)
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "psb_cmdbuf: dump cmdbuf fail\n");

    cmdbuf->cmd_count++;
    memset(msg, 0, msg_size);

    *cmdbuf->cmd_idx = 0; // Add a trailing 0 just in case.
    ASSERT(cmdbuffer_size < CMD_SIZE);
    ASSERT((unsigned char *) cmdbuf->cmd_idx < CMD_END(cmdbuf));

    MEMIO_WRITE_FIELD(msg, FWRK_GENMSG_SIZE,                  msg_size);
    MEMIO_WRITE_FIELD(msg, FWRK_GENMSG_ID,                    VA_MSGID_RENDER);

        MEMIO_WRITE_FIELD(msg, FW_DEVA_DECODE_CONTEXT, (obj_context->msvdx_context)); /* context is 8 bits */

    /* Point to CMDBUFFER */
    RELOC_MSG(*(msg + (FW_DEVA_DECODE_LLDMA_ADDRESS_OFFSET / sizeof(uint32_t))),
              (cmdbuf->cmd_start - cmdbuf->cmd_base), &(cmdbuf->buf));
    MEMIO_WRITE_FIELD(msg, FW_DEVA_DECODE_BUFFER_SIZE,          cmdbuffer_size / 4); // In dwords
    MEMIO_WRITE_FIELD(msg, FW_DEVA_DECODE_OPERATING_MODE,       obj_context->operating_mode);
    MEMIO_WRITE_FIELD(msg, FW_DEVA_DECODE_FLAGS,                obj_context->flags);

    if (psb_video_trace_fp && (psb_video_trace_level & LLDMA_TRACE)) {
        debug_lldma_count = (cmdbuf->lldma_idx - cmdbuf->lldma_base) / sizeof(DMA_sLinkedList);
        debug_lldma_start = cmdbuf->lldma_base - cmdbuf->cmd_base;
        /* Indicate last LLDMA record (for debugging) */
        ((uint32_t *)cmdbuf->lldma_idx)[1] = 0;
    }

    cmdbuf->cmd_start = (unsigned char *)cmdbuf->cmd_idx;

    if (psb_video_trace_fp) {
        return psb_context_flush_cmdbuf(obj_context);
    } else {
        if ((cmdbuf->cmd_count >= MAX_CMD_COUNT) ||
            (MTXMSG_END(cmdbuf) - (unsigned char *) msg < MTXMSG_MARGIN) ||
            (CMD_END(cmdbuf) - (unsigned char *) cmdbuf->cmd_idx < CMD_MARGIN) ||
            (LLDMA_END(cmdbuf) - cmdbuf->lldma_idx < LLDMA_MARGIN) ||
            (RELOC_END(cmdbuf) - (unsigned char *) cmdbuf->reloc_idx < RELOC_MARGIN)) {
            return psb_context_flush_cmdbuf(obj_context);
        }
    }
    return 0;
}

/*
 * Flushes all cmdbufs
 */
int psb_context_flush_cmdbuf(object_context_p obj_context)
{
    psb_cmdbuf_p cmdbuf = obj_context->cmdbuf;
    psb_driver_data_p driver_data = obj_context->driver_data;
    unsigned int fence_flags;
    /* unsigned int fence_handle = 0; */
    struct psb_ttm_fence_rep fence_rep;
    unsigned int reloc_offset;
    unsigned int num_relocs;
    int ret;
    unsigned int item_size = FW_DEVA_DECODE_SIZE; /* Size of a render/deocde msg */

#ifdef SLICE_HEADER_PARSING
    if ((NULL == cmdbuf) ||
        (0 == (cmdbuf->cmd_count + cmdbuf->deblock_count + cmdbuf->host_be_opp_count +
            cmdbuf->frame_info_count + cmdbuf->parse_count))) {
        return 0; // Nothing to do
    }
#else
    if ((NULL == cmdbuf) ||
        (0 == (cmdbuf->cmd_count + cmdbuf->deblock_count + cmdbuf->host_be_opp_count + cmdbuf->frame_info_count))) {
        return 0; // Nothing to do
    }
#endif

    uint32_t msg_size = 0;
    uint32_t *msg = (uint32_t *)cmdbuf->MTX_msg;
    int32_t i;
    uint32_t index;

    /* LOCK */
    ret = LOCK_HARDWARE(driver_data);
    if (ret) {
        UNLOCK_HARDWARE(driver_data);
        DEBUG_FAILURE_RET;
        return ret;
    }

    for (i = 1; i <= cmdbuf->frame_info_count; i++) {
        msg_size += FW_VA_FRAME_INFO_SIZE;
        msg += FW_VA_FRAME_INFO_SIZE / sizeof(uint32_t);
    }

    for (i = 1; i <= cmdbuf->cmd_count; i++) {
        uint32_t flags;

        flags = MEMIO_READ_FIELD(msg, FW_DEVA_DECODE_FLAGS);

        /* Update flags */
        int bBatchEnd = (i == (cmdbuf->cmd_count + cmdbuf->deblock_count + cmdbuf->oold_count
                               + cmdbuf->host_be_opp_count));
        flags |=
            (bBatchEnd ? FW_VA_RENDER_HOST_INT : FW_VA_RENDER_NO_RESPONCE_MSG);

#ifdef PSBVIDEO_MSVDX_EC
        if (driver_data->ec_enabled)
            flags |= FW_ERROR_DETECTION_AND_RECOVERY;
#endif

        MEMIO_WRITE_FIELD(msg, FW_DEVA_DECODE_FLAGS, flags);

        psb__trace_message("MSG BUFFER_SIZE       = %08x\n", MEMIO_READ_FIELD(msg, FW_DEVA_DECODE_BUFFER_SIZE));
        psb__trace_message("MSG OPERATING_MODE    = %08x\n", MEMIO_READ_FIELD(msg, FW_DEVA_DECODE_OPERATING_MODE));
        psb__trace_message("MSG FLAGS             = %08x\n", MEMIO_READ_FIELD(msg, FW_DEVA_DECODE_FLAGS));

        drv_debug_msg(VIDEO_DEBUG_GENERAL, "MSG BUFFER_SIZE       = %08x\n", MEMIO_READ_FIELD(msg, FW_DEVA_DECODE_BUFFER_SIZE));
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "MSG OPERATING_MODE    = %08x\n", MEMIO_READ_FIELD(msg, FW_DEVA_DECODE_OPERATING_MODE));
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "MSG FLAGS             = %08x\n", MEMIO_READ_FIELD(msg, FW_DEVA_DECODE_FLAGS));

#if 0  /* todo */
        /* Update SAREA */
        driver_data->psb_sarea->msvdx_context = obj_context->msvdx_context;
#endif
        msg += item_size / sizeof(uint32_t);
        msg_size += item_size;
    }

    /* Assume deblock message is following render messages and no more render message behand deblock message */
    for (i = 1; i <= cmdbuf->deblock_count; i++) {
            msg_size += sizeof(FW_VA_DEBLOCK_MSG);
    }

    for (i = 1; i <= cmdbuf->oold_count; i++) {
        msg_size += sizeof(FW_VA_DEBLOCK_MSG);
    }

    for (i = 1; i <= cmdbuf->host_be_opp_count; i++) {
        msg_size += FW_VA_HOST_BE_OPP_SIZE;
    }
#ifdef SLICE_HEADER_PARSING
    for (i = 1; i <= cmdbuf->parse_count; i++) {
        msg_size += sizeof(struct fw_slice_header_extract_msg);
    }
#endif
    /* Now calculate the total number of relocations */
    reloc_offset = cmdbuf->reloc_base - cmdbuf->MTX_msg;
    num_relocs = (((unsigned char *) cmdbuf->reloc_idx) - cmdbuf->reloc_base) / sizeof(struct drm_psb_reloc);

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "Cmdbuf MTXMSG size = %08x [%08x]\n", msg_size, MTXMSG_SIZE);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "Cmdbuf CMD size = %08x - %d[%08x]\n", (unsigned char *) cmdbuf->cmd_idx - cmdbuf->cmd_base, cmdbuf->cmd_count, CMD_SIZE);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "Cmdbuf LLDMA size = %08x [%08x]\n", cmdbuf->lldma_idx - cmdbuf->lldma_base, LLDMA_SIZE);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "Cmdbuf RELOC size = %08x [%08x]\n", num_relocs * sizeof(struct drm_psb_reloc), RELOC_SIZE);

    psb_cmdbuf_unmap(cmdbuf);

    psb__trace_message(NULL); /* Flush trace */

    ASSERT(NULL == cmdbuf->MTX_msg);
    ASSERT(NULL == cmdbuf->reloc_base);

    if (psb_video_trace_fp)
        fence_flags = 0;
    else
        fence_flags = DRM_PSB_FENCE_NO_USER;

#ifdef SLICE_HEADER_PARSING
    if (obj_context->msvdx_frame_end)
        fence_flags |= PSB_SLICE_EXTRACT_UPDATE;
#endif
    /* cmdbuf will be validated as part of the buffer list */
    /* Submit */
    wsbmWriteLockKernelBO();
    ret = psbDRMCmdBuf(driver_data->drm_fd, driver_data->execIoctlOffset, cmdbuf->buffer_refs,
                       cmdbuf->buffer_refs_count,
                       wsbmKBufHandle(wsbmKBuf(cmdbuf->reloc_buf.drm_buf)),
                       0, msg_size,
                       wsbmKBufHandle(wsbmKBuf(cmdbuf->reloc_buf.drm_buf)),
                       reloc_offset, num_relocs,
                       0, PSB_ENGINE_DECODE, fence_flags, &fence_rep);
    wsbmWriteUnlockKernelBO();
    UNLOCK_HARDWARE(driver_data);

    if (ret) {
        obj_context->cmdbuf = NULL;
        obj_context->slice_count++;

        DEBUG_FAILURE_RET;
        return ret;
    }

    if (psb_video_trace_fp) {
#if 0
        static int error_count = 0;
        int status = 0;
        struct _WsbmFenceObject *fence = NULL;
        fence = psb_fence_wait(driver_data, &fence_rep, &status);
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "psb_fence_wait returns: %d (fence=0x%08x)\n", status, fence);
#endif

        psb_buffer_map(&cmdbuf->buf, &cmdbuf->cmd_base);
        int ret;
        ret = psb_buffer_map(&cmdbuf->reloc_buf, &cmdbuf->MTX_msg);
        if(ret) {
            psb_buffer_unmap(&cmdbuf->buf);
            return ret;
        }

        if (psb_video_trace_level & LLDMA_TRACE) {
            psb__trace_message("lldma_count = %d, vitual=0x%08x\n",
                               debug_lldma_count,  wsbmBOOffsetHint(cmdbuf->buf.drm_buf) + CMD_SIZE);
            for (index = 0; index < debug_lldma_count; index++) {
                DMA_sLinkedList* pasDmaList = (DMA_sLinkedList*)(cmdbuf->cmd_base + debug_lldma_start);
                pasDmaList += index;

                psb__trace_message("\nLLDMA record at offset %08x\n", ((unsigned char*)pasDmaList) - cmdbuf->cmd_base);
                DW(0, BSWAP,    31, 31)
                DW(0, DIR,    30, 30)
                DW(0, PW,    29, 28)
                DW(1, List_FIN, 31, 31)
                DW(1, List_INT, 30, 30)
                DW(1, PI,    18, 17)
                DW(1, INCR,    16, 16)
                DW(1, LEN,    15, 0)
                DWH(2, ADDR,    22, 0)
                DW(3, ACC_DEL,    31, 29)
                DW(3, BURST,    28, 26)
                DWH(3, EXT_SA,    3, 0)
                DW(4, 2D_MODE,    16, 16)
                DW(4, REP_COUNT, 10, 0)
                DWH(5, LINE_ADD_OFF, 25, 16)
                DW(5, ROW_LENGTH, 9, 0)
                DWH(6, SA, 31, 0)
                DWH(7, LISTPTR, 27, 0)
            }
        }

        if (psb_video_trace_level & AUXBUF_TRACE) {
            psb__trace_message("debug_dump_count = %d\n", debug_dump_count);
            for (index = 0; index < debug_dump_count; index++) {
                unsigned char *buf_addr;
                psb__trace_message("Buffer %d = '%s' offset = %08x size = %08x\n", index, debug_dump_name[index], debug_dump_offset[index], debug_dump_size[index]);
                if (debug_dump_buf[index]->rar_handle
                    || (psb_buffer_map(debug_dump_buf[index], &buf_addr) != 0)) {
                    psb__trace_message("Unmappable buffer,e.g. RAR buffer\n");
                    continue;
                }

                g_hexdump_offset = 0;
                psb__hexdump(buf_addr + debug_dump_offset[index], debug_dump_size[index]);
                psb_buffer_unmap(debug_dump_buf[index]);
            }
            debug_dump_count = 0;
        }

        if (psb_video_trace_level & CMDMSG_TRACE) {
            psb__trace_message("cmd_count = %d, virtual=0x%08x\n",
                               debug_cmd_count, wsbmBOOffsetHint(cmdbuf->buf.drm_buf));
            for (index = 0; index < debug_cmd_count; index++) {
                uint32_t *msg = (uint32_t *)(cmdbuf->MTX_msg + index * item_size);
                uint32_t j;
                drv_debug_msg(VIDEO_DEBUG_GENERAL, "start = %08x size = %08x\n", debug_cmd_start[index], debug_cmd_size[index]);
                debug_dump_cmdbuf((uint32_t *)(cmdbuf->cmd_base + debug_cmd_start[index]), debug_cmd_size[index]);

                for (j = 0; j < item_size / 4; j++) {
                    psb__trace_message("MTX msg[%d] = 0x%08x", j, *(msg + j));
                    switch (j) {
                    case 0:
                        psb__trace_message("[BufferSize|ID|MSG_SIZE]\n");
                        break;
                    case 1:
                        psb__trace_message("[MMUPTD]\n");
                        break;
                    case 2:
                        psb__trace_message("[LLDMA_address]\n");
                        break;
                    case 3:
                        psb__trace_message("[Context]\n");
                        break;
                    case 4:
                        psb__trace_message("[Fence_Value]\n");
                        break;
                    case 5:
                        psb__trace_message("[Operating_Mode]\n");
                        break;
                    case 6:
                        psb__trace_message("[LastMB|FirstMB]\n");
                        break;
                    case 7:
                        psb__trace_message("[Flags]\n");
                        break;
                    default:
                        psb__trace_message("[overflow]\n");
                        break;
                    }
                }
            }
            debug_cmd_count = 0;
        }
        psb_buffer_unmap(&cmdbuf->buf);
        psb_buffer_unmap(&cmdbuf->reloc_buf);

        cmdbuf->cmd_base = NULL;
#if 0
        if (status) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "RENDERING ERROR FRAME=%03d SLICE=%02d status=%d\n", obj_context->frame_count, obj_context->slice_count, status);
            error_count++;
            ASSERT(status != 2);
            ASSERT(error_count < 40); /* Exit on 40 errors */
        }
        if (fence)
            psb_fence_destroy(fence);
#endif
    }

    obj_context->cmdbuf = NULL;
    obj_context->slice_count++;

    return 0;
}


typedef enum {
    MMU_GROUP0 = 0,
    MMU_GROUP1 = 1,
} MMU_GROUP;

typedef enum    {
    HOST_TO_MSVDX = 0,
    MSXDX_TO_HOST = 1,
} DMA_DIRECTION;

typedef struct {
    IMG_UINT32 ui32DevDestAddr ;        /* destination address */
    DMA_ePW     ePeripheralWidth;
    DMA_ePeriphIncrSize ePeriphIncrSize;
    DMA_ePeriphIncr     ePeriphIncr;
    IMG_BOOL            bSynchronous;
    MMU_GROUP           eMMUGroup;
    DMA_DIRECTION       eDMADir;
    DMA_eBurst          eDMA_eBurst;
} DMA_DETAIL_LOOKUP;


static const DMA_DETAIL_LOOKUP DmaDetailLookUp[] = {
    /* LLDMA_TYPE_VLC_TABLE */ {
        REG_MSVDX_VEC_VLC_OFFSET  ,
        DMA_PWIDTH_16_BIT,      /* 16 bit wide data*/
        DMA_PERIPH_INCR_4,      /* Incrament the dest by 32 bits */
        DMA_PERIPH_INCR_ON,
        IMG_TRUE,
        MMU_GROUP0,
        HOST_TO_MSVDX,
        DMA_BURST_2
    },
    /* LLDMA_TYPE_BITSTREAM */ {
        (REG_MSVDX_VEC_OFFSET + MSVDX_VEC_CR_VEC_SHIFTREG_STREAMIN_OFFSET),
        DMA_PWIDTH_8_BIT,
        DMA_PERIPH_INCR_1,
        DMA_PERIPH_INCR_OFF,
        IMG_FALSE,
        MMU_GROUP0,
        HOST_TO_MSVDX,
        DMA_BURST_4
    },
    /*LLDMA_TYPE_RESIDUAL*/             {
        (REG_MSVDX_VDMC_OFFSET + MSVDX_VDMC_CR_VDMC_RESIDUAL_DIRECT_INSERT_DATA_OFFSET),
        DMA_PWIDTH_32_BIT,
        DMA_PERIPH_INCR_1,
        DMA_PERIPH_INCR_OFF,
        IMG_FALSE,
        MMU_GROUP1,
        HOST_TO_MSVDX,
        DMA_BURST_4
    },

    /*LLDMA_TYPE_RENDER_BUFF_MC*/{
        (REG_MSVDX_MTX_OFFSET + MTX_CORE_CR_MTX_SYSC_CDMAT_OFFSET),
        DMA_PWIDTH_32_BIT,
        DMA_PERIPH_INCR_1,
        DMA_PERIPH_INCR_OFF,
        IMG_TRUE,
        MMU_GROUP1,
        HOST_TO_MSVDX,
        DMA_BURST_1             /* Into MTX */
    },
    /*LLDMA_TYPE_RENDER_BUFF_VLD*/{
        (REG_MSVDX_MTX_OFFSET + MTX_CORE_CR_MTX_SYSC_CDMAT_OFFSET),
        DMA_PWIDTH_32_BIT,
        DMA_PERIPH_INCR_1,
        DMA_PERIPH_INCR_OFF,
        IMG_TRUE,
        MMU_GROUP0,
        HOST_TO_MSVDX,
        DMA_BURST_1             /* Into MTX */
    },
    /*LLDMA_TYPE_MPEG4_FESTATE_SAVE*/{
        (REG_MSVDX_VEC_RAM_OFFSET + 0xB90),
        DMA_PWIDTH_32_BIT,
        DMA_PERIPH_INCR_4,
        DMA_PERIPH_INCR_ON,
        IMG_TRUE,
        MMU_GROUP0,
        MSXDX_TO_HOST,
        DMA_BURST_2              /* From VLR */
    },
    /*LLDMA_TYPE_MPEG4_FESTATE_RESTORE*/{
        (REG_MSVDX_VEC_RAM_OFFSET + 0xB90),
        DMA_PWIDTH_32_BIT,
        DMA_PERIPH_INCR_4,
        DMA_PERIPH_INCR_ON,
        IMG_TRUE,
        MMU_GROUP0,
        HOST_TO_MSVDX,
        DMA_BURST_2             /* Into VLR */
    },
    /*LLDMA_TYPE_H264_PRELOAD_SAVE*/{
        (REG_MSVDX_MTX_OFFSET + MTX_CORE_CR_MTX_SYSC_CDMAT_OFFSET),
        DMA_PWIDTH_32_BIT,
        DMA_PERIPH_INCR_1,
        DMA_PERIPH_INCR_OFF,
        IMG_TRUE,       /* na */
        MMU_GROUP1,
        MSXDX_TO_HOST,
        DMA_BURST_1             /* From MTX */
    },
    /*LLDMA_TYPE_H264_PRELOAD_RESTORE*/{
        (REG_MSVDX_MTX_OFFSET + MTX_CORE_CR_MTX_SYSC_CDMAT_OFFSET),
        DMA_PWIDTH_32_BIT,
        DMA_PERIPH_INCR_1,
        DMA_PERIPH_INCR_OFF,
        IMG_TRUE,       /* na */
        MMU_GROUP1,
        HOST_TO_MSVDX,
        DMA_BURST_1             /* Into MTX */
    },
    /*LLDMA_TYPE_VC1_PRELOAD_SAVE*/{
        (REG_MSVDX_MTX_OFFSET + MTX_CORE_CR_MTX_SYSC_CDMAT_OFFSET),
        DMA_PWIDTH_32_BIT,
        DMA_PERIPH_INCR_1,
        DMA_PERIPH_INCR_OFF,
        IMG_TRUE,       /* na */
        MMU_GROUP0,
        MSXDX_TO_HOST,
        DMA_BURST_1             //2     /* From MTX */
    },
    /*LLDMA_TYPE_VC1_PRELOAD_RESTORE*/{
        (REG_MSVDX_MTX_OFFSET + MTX_CORE_CR_MTX_SYSC_CDMAT_OFFSET),
        DMA_PWIDTH_32_BIT,
        DMA_PERIPH_INCR_1,
        DMA_PERIPH_INCR_OFF,
        IMG_TRUE,       /* na */
        MMU_GROUP0,
        HOST_TO_MSVDX,
        DMA_BURST_1             /* Into MTX */
    },
    /*LLDMA_TYPE_MEM_SET */{
        (REG_MSVDX_VEC_RAM_OFFSET + 0xCC0),
        DMA_PWIDTH_32_BIT,
        DMA_PERIPH_INCR_4,
        DMA_PERIPH_INCR_OFF,
        IMG_TRUE,       /* na */
        MMU_GROUP0,
        MSXDX_TO_HOST,
        DMA_BURST_4                     /* From VLR */
    },

};

#define MAX_DMA_LEN     ( 0xffff )

void *psb_cmdbuf_alloc_space(psb_cmdbuf_p cmdbuf, uint32_t byte_size)
{
    void *pos = (void *)cmdbuf->cmd_idx;
    ASSERT(!(byte_size % 4));

    cmdbuf->cmd_idx += (byte_size / 4);

    return pos;
}

void psb_cmdbuf_dma_write_cmdbuf(psb_cmdbuf_p cmdbuf,
                                   psb_buffer_p bitstream_buf,
                                   uint32_t buffer_offset,
                                   uint32_t size,
                                   uint32_t dest_offset,
                                   DMA_TYPE type)
{
    ASSERT(size < 0xFFFF);
    ASSERT(buffer_offset < 0xFFFF);

    DMA_CMD_WITH_OFFSET* dma_cmd;

    if(dest_offset==0)
    {
            dma_cmd = (DMA_CMD_WITH_OFFSET*)psb_cmdbuf_alloc_space(cmdbuf, sizeof(DMA_CMD));
            dma_cmd->ui32Cmd = 0;
    }
    else
    {
            dma_cmd = (DMA_CMD_WITH_OFFSET*)psb_cmdbuf_alloc_space(cmdbuf, sizeof(DMA_CMD_WITH_OFFSET));
            dma_cmd->ui32Cmd = CMD_DMA_OFFSET_FLAG; // Set flag indicating that offset is deffined
            dma_cmd->ui32ByteOffset = dest_offset;
    }

    dma_cmd->ui32Cmd |= CMD_DMA;
    dma_cmd->ui32Cmd |= (IMG_UINT32)type;
    dma_cmd->ui32Cmd |= size;
    /* dma_cmd->ui32DevVirtAdd  = ui32DevVirtAddress; */
    RELOC(dma_cmd->ui32DevVirtAdd, buffer_offset, bitstream_buf);
}

/*
 * Write a CMD_SR_SETUP referencing a bitstream buffer to the command buffer
 */
void psb_cmdbuf_dma_write_bitstream(psb_cmdbuf_p cmdbuf,
                                      psb_buffer_p bitstream_buf,
                                      uint32_t buffer_offset,
                                      uint32_t size_in_bytes,
                                      uint32_t offset_in_bits,
                                      uint32_t flags)
{
    /*
     * We use byte alignment instead of 32bit alignment.
     * The third frame of sa10164.vc1 results in the following bitstream
     * patttern:
     * [0000] 00 00 03 01 76 dc 04 8d
     * with offset_in_bits = 0x1e
     * This causes an ENTDEC failure because 00 00 03 is a start code
     * By byte aligning the datastream the start code will be eliminated.
     */
//don't need to change the offset_in_bits, size_in_bytes and buffer_offset
#if 0
#define ALIGNMENT        sizeof(uint8_t)
    uint32_t bs_offset_in_dwords    = ((offset_in_bits / 8) / ALIGNMENT);
    size_in_bytes                   -= bs_offset_in_dwords * ALIGNMENT;
    offset_in_bits                  -= bs_offset_in_dwords * 8 * ALIGNMENT;
    buffer_offset                   += bs_offset_in_dwords * ALIGNMENT;
#endif

    *cmdbuf->cmd_idx++ = CMD_SR_SETUP | flags;
    *cmdbuf->cmd_idx++ = offset_in_bits;
    cmdbuf->cmd_bitstream_size = cmdbuf->cmd_idx;
    *cmdbuf->cmd_idx++ = size_in_bytes;
    *cmdbuf->cmd_idx++ = (CMD_BITSTREAM_DMA | size_in_bytes);
    RELOC(*cmdbuf->cmd_idx++, buffer_offset, bitstream_buf);
}

#ifdef SLICE_HEADER_PARSING
/*
 * Write a CMD_SR_SETUP referencing a bitstream buffer to the command buffer
 */
void psb_cmdbuf_dma_write_key(psb_cmdbuf_p cmdbuf,
                                      uint32_t flags,
                                      uint32_t key)
{
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "pass key, flags is 0x%x, key is 0x%x.\n", flags, key);
    *cmdbuf->cmd_idx++ = CMD_SR_SETUP | flags;
    *cmdbuf->cmd_idx++ = key;
}
#endif

/*
 * Chain a LLDMA bitstream command to the previous one
 */
void psb_cmdbuf_dma_write_bitstream_chained(psb_cmdbuf_p cmdbuf,
        psb_buffer_p bitstream_buf,
        uint32_t size_in_bytes)
{
    *cmdbuf->cmd_idx++ = (CMD_BITSTREAM_DMA | size_in_bytes);
    RELOC(*cmdbuf->cmd_idx++, bitstream_buf->buffer_ofs, bitstream_buf);

    *(cmdbuf->cmd_bitstream_size) += size_in_bytes;
}

void psb_cmdbuf_reg_start_block(psb_cmdbuf_p cmdbuf, uint32_t flags)
{
    ASSERT(NULL == cmdbuf->reg_start); /* Can't have both */

    cmdbuf->reg_wt_p = cmdbuf->cmd_idx;
    cmdbuf->reg_next = 0;
    cmdbuf->reg_flags = (flags << 4); /* flags are diff between DE2 & DE3 */
    cmdbuf->reg_start = NULL;
}

void psb_cmdbuf_reg_set(psb_cmdbuf_p cmdbuf, uint32_t reg, uint32_t val)
{
    if(cmdbuf->reg_start && (reg == cmdbuf->reg_next))
    {
        /* Incrament header size */
        *cmdbuf->reg_start += (0x1 << 16);
    }
    else
    {
        cmdbuf->reg_start = cmdbuf->reg_wt_p++;
        *cmdbuf->reg_start = CMD_REGVALPAIR_WRITE | cmdbuf->reg_flags | 0x10000 | (reg & 0xfffff); /* We want host reg addr */
    }
    *cmdbuf->reg_wt_p++ = val;
    cmdbuf->reg_next = reg + 4;
}

void psb_cmdbuf_reg_set_address(psb_cmdbuf_p cmdbuf,
                                         uint32_t reg,
                                         psb_buffer_p buffer,
                                         uint32_t buffer_offset)
{
    if(cmdbuf->reg_start && (reg == cmdbuf->reg_next))
    {
        /* Incrament header size */
        *cmdbuf->reg_start += (0x1 << 16);
    }
    else
    {
        cmdbuf->reg_start = cmdbuf->reg_wt_p++;
        *cmdbuf->reg_start = CMD_REGVALPAIR_WRITE | cmdbuf->reg_flags | 0x10000 | (reg & 0xfffff); /* We want host reg addr */
    }

    RELOC(*cmdbuf->reg_wt_p++, buffer_offset, buffer);
    cmdbuf->reg_next = reg + 4;
}

void psb_cmdbuf_reg_end_block(psb_cmdbuf_p cmdbuf)
{
    cmdbuf->cmd_idx = cmdbuf->reg_wt_p;
    cmdbuf->reg_start = NULL;
}

typedef enum {
    MTX_CTRL_HEADER = 0,
    RENDEC_SL_HDR,
    RENDEC_SL_NULL,
    RENDEC_CK_HDR,
} RENDEC_CHUNK_OFFSETS;

/*
 * Start a new rendec block of another format
 */
void psb_cmdbuf_rendec_start(psb_cmdbuf_p cmdbuf, uint32_t dest_address)
{
    ASSERT(((dest_address >> 2)& ~0xfff) == 0);
    cmdbuf->rendec_chunk_start = cmdbuf->cmd_idx++;
    *cmdbuf->rendec_chunk_start = CMD_RENDEC_BLOCK | dest_address;
}

void psb_cmdbuf_rendec_write_block(psb_cmdbuf_p cmdbuf,
                                   unsigned char *block,
                                   uint32_t size)
{
    ASSERT((size & 0x3) == 0);
    unsigned int i;
    for (i = 0; i < size; i += 4) {
        uint32_t val = block[i] | (block[i+1] << 8) | (block[i+2] << 16) | (block[i+3] << 24);
        psb_cmdbuf_rendec_write(cmdbuf, val);
    }
}

void psb_cmdbuf_rendec_write_address(psb_cmdbuf_p cmdbuf,
                                     psb_buffer_p buffer,
                                     uint32_t buffer_offset)
{
    RELOC(*cmdbuf->cmd_idx++, buffer_offset, buffer);
}

/*
 * Finish a RENDEC block
 */
void psb_cmdbuf_rendec_end(psb_cmdbuf_p cmdbuf)
{
    ASSERT(NULL != cmdbuf->rendec_chunk_start); /* Must have an open RENDEC chunk */
    uint32_t dword_count = cmdbuf->cmd_idx - cmdbuf->rendec_chunk_start;

    ASSERT((dword_count - 1) <= 0xff);

    *cmdbuf->rendec_chunk_start += ((dword_count - 1) << 16);
    cmdbuf->rendec_chunk_start = NULL;
}

/*
 * Create a conditional SKIP block
 */
void psb_cmdbuf_skip_start_block(psb_cmdbuf_p cmdbuf, uint32_t skip_condition)
{
    ASSERT(NULL == cmdbuf->rendec_block_start); /* Can't be inside a rendec block */
    ASSERT(NULL == cmdbuf->reg_start); /* Can't be inside a reg block */
    ASSERT(NULL == cmdbuf->skip_block_start); /* Can't be inside another skip block (limitation of current sw design)*/

    cmdbuf->skip_condition = skip_condition;
    cmdbuf->skip_block_start = cmdbuf->cmd_idx++;
}

/*
 * Terminate a conditional SKIP block
 */
void psb_cmdbuf_skip_end_block(psb_cmdbuf_p cmdbuf)
{
    ASSERT(NULL == cmdbuf->rendec_block_start); /* Rendec block must be closed */
    ASSERT(NULL == cmdbuf->reg_start); /* Reg block must be closed */
    ASSERT(NULL != cmdbuf->skip_block_start); /* Skip block must still be open */

    uint32_t block_size = cmdbuf->cmd_idx - (cmdbuf->skip_block_start + 1);

    *cmdbuf->skip_block_start = CMD_CONDITIONAL_SKIP | (cmdbuf->skip_condition << 20) | block_size;
    cmdbuf->skip_block_start = NULL;
}
