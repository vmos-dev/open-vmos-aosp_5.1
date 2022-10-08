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
 *    Edward Lin <edward.lin@intel.com>
 *
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <wsbm/wsbm_manager.h>
#include "psb_buffer.h"
#include "tng_cmdbuf.h"
#include "psb_def.h"
#include "psb_drv_debug.h"
#include "tng_hostcode.h"
#include "psb_ws_driver.h"

#ifdef ANDROID
#include <linux/psb_drm.h>
#else
#include "psb_drm.h"
#endif

#include "tng_trace.h"

/*
 * Buffer layout:
 *         cmd_base <= cmd_idx < CMD_END() == reloc_base
 *         reloc_base <= reloc_idx < RELOC_END() == (reloc_size)
 */

#define RELOC_END(cmdbuf)     (cmdbuf->cmd_base + cmdbuf->size)
#define CMD_END(cmdbuf)       (cmdbuf->reloc_base)
#define RELOC_SIZE            (0x3000)
#define CMD_SIZE              (0x3000)
#define RELOC_MARGIN          (0x0800)
#define CMD_MARGIN            (0x0400)
#define MAX_CMD_COUNT         12
#define MTX_SEG_SIZE          (0x0800)

/*!
 *****************************************************************************
 *
 * @Name           Command word format
 *
 * @details    Mask and shift values for command word
 *
 ****************************************************************************/

/*
 * clear buffer
 */
void tng_cmdbuf_mem_unmap(tng_cmdbuf_p cmdbuf)
{
    psb_buffer_unmap(&cmdbuf->frame_mem);
    psb_buffer_unmap(&cmdbuf->jpeg_pic_params);
    psb_buffer_unmap(&cmdbuf->jpeg_header_mem);
    psb_buffer_unmap(&cmdbuf->jpeg_header_interface_mem);
    return ;
}

/*
 * clear buffer
 */
static void tng_cmdbuf_clear(tng_cmdbuf_p cmdbuf, int flag)
{
    switch (flag) {
        default:
        case 4:
            psb_buffer_destroy(&cmdbuf->jpeg_header_mem);
        case 3:
            psb_buffer_destroy(&cmdbuf->jpeg_pic_params);
        case 2:
        case 1:
            psb_buffer_destroy(&cmdbuf->frame_mem);
            break;
    }

    if (cmdbuf->size) {
        psb_buffer_destroy(&cmdbuf->buf);
        cmdbuf->size = 0;
    }
    if (cmdbuf->buffer_refs_allocated) {
        free(cmdbuf->buffer_refs);
        cmdbuf->buffer_refs = NULL;
        cmdbuf->buffer_refs_allocated = 0;
    }
}


/*
 * Create command buffer
 */

VAStatus tng_cmdbuf_create(
    object_context_p obj_context,
    psb_driver_data_p driver_data,
    tng_cmdbuf_p cmdbuf)
{
    context_ENC_p ctx = (context_ENC_p) obj_context->format_data;

    VAStatus vaStatus = VA_STATUS_SUCCESS;
    unsigned int size = CMD_SIZE + RELOC_SIZE;

    cmdbuf->size = 0;
    cmdbuf->cmd_base = NULL;
    cmdbuf->cmd_idx = NULL;
    cmdbuf->reloc_base = NULL;
    cmdbuf->reloc_idx = NULL;
    cmdbuf->buffer_refs_count = 0;
    cmdbuf->buffer_refs_allocated = 10;
    cmdbuf->buffer_refs = (psb_buffer_p *) calloc(1, sizeof(psb_buffer_p) * cmdbuf->buffer_refs_allocated);
    if (NULL == cmdbuf->buffer_refs) {
        cmdbuf->buffer_refs_allocated = 0;
        vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
    }

    if (VA_STATUS_SUCCESS == vaStatus) {
        vaStatus = psb_buffer_create(driver_data, size, psb_bt_cpu_only, &cmdbuf->buf);
        cmdbuf->size = size;
    }

    if (VA_STATUS_SUCCESS != vaStatus) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "psb buffer create 0 \n", __FUNCTION__);
	tng_cmdbuf_clear(cmdbuf, 1);
        free(cmdbuf->buffer_refs);
        cmdbuf->buffer_refs = NULL;
        cmdbuf->buffer_refs_allocated = 0;
        return vaStatus;
    }

    cmdbuf->mem_size = tng_align_KB(TNG_HEADER_SIZE);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "mem size %d\n", __FUNCTION__, cmdbuf->mem_size);
    /* create buffer information buffer */
    //DEBUG-FIXME
    //tng__alloc_init_buffer(driver_data, COMM_CMD_FRAME_BUF_NUM * cmdbuf->mem_size, psb_bt_cpu_vpu, &cmdbuf->frame_mem);

    vaStatus = psb_buffer_create(driver_data, COMM_CMD_FRAME_BUF_NUM * cmdbuf->mem_size, psb_bt_cpu_vpu, &cmdbuf->frame_mem);
    if (VA_STATUS_SUCCESS != vaStatus) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "psb buffer create xx \n", __FUNCTION__);
        free(cmdbuf->buffer_refs);
        cmdbuf->buffer_refs = NULL;
        cmdbuf->buffer_refs_allocated = 0;
        return vaStatus;
    }
    /* all cmdbuf share one MTX_CURRENT_IN_PARAMS since every MB has a MTX_CURRENT_IN_PARAMS structure
     * and filling this structure for all MB is very time-consuming
     */

    /* create JPEG quantization buffer */
    vaStatus = psb_buffer_create(driver_data, ctx->jpeg_pic_params_size, psb_bt_cpu_vpu, &cmdbuf->jpeg_pic_params);
    if (VA_STATUS_SUCCESS != vaStatus) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "psb buffer create 1 \n", __FUNCTION__);
        tng_cmdbuf_clear(cmdbuf, 2);
        return vaStatus;
    }

    /* create JPEG MTX setup buffer */
    vaStatus = psb_buffer_create(driver_data, ctx->jpeg_header_mem_size, psb_bt_cpu_vpu, &cmdbuf->jpeg_header_mem);
    if (VA_STATUS_SUCCESS != vaStatus) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "psb buffer create 2 \n", __FUNCTION__);
        tng_cmdbuf_clear(cmdbuf, 3);
        return vaStatus;
    }

    /* create JPEG MTX setup interface buffer */
    vaStatus = psb_buffer_create(driver_data, ctx->jpeg_header_interface_mem_size, psb_bt_cpu_vpu, &cmdbuf->jpeg_header_interface_mem);
    if (VA_STATUS_SUCCESS != vaStatus) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "psb buffer create 3 \n", __FUNCTION__);
        tng_cmdbuf_clear(cmdbuf, 4);
        return vaStatus;
    }

    return vaStatus;
}

/*
 * Destroy buffer
 */
void tng_cmdbuf_destroy(tng_cmdbuf_p cmdbuf)
{
    psb_buffer_destroy(&cmdbuf->frame_mem);
    psb_buffer_destroy(&cmdbuf->jpeg_header_mem);
    psb_buffer_destroy(&cmdbuf->jpeg_pic_params);
    psb_buffer_destroy(&cmdbuf->jpeg_header_interface_mem);

    if (cmdbuf->size) {
        psb_buffer_destroy(&cmdbuf->buf);
        cmdbuf->size = 0;
    }
    if (cmdbuf->buffer_refs_allocated) {
        free(cmdbuf->buffer_refs);
        cmdbuf->buffer_refs = NULL;
        cmdbuf->buffer_refs_allocated = 0;
    }
    return ;
}

/*
 * Reset buffer & map
 *
 * Returns 0 on success
 */
int tng_cmdbuf_reset(tng_cmdbuf_p cmdbuf)
{
    int ret;
    cmdbuf->cmd_base = NULL;
    cmdbuf->cmd_idx = NULL;
    cmdbuf->reloc_base = NULL;
    cmdbuf->reloc_idx = NULL;

    cmdbuf->buffer_refs_count = 0;
    cmdbuf->frame_mem_index = 0;
    cmdbuf->cmd_count = 0;
    cmdbuf->mem_size = tng_align_KB(TNG_HEADER_SIZE);

    ret = psb_buffer_map(&cmdbuf->buf, &cmdbuf->cmd_base);
    if (ret) {
        return ret;
    }

    cmdbuf->cmd_start = cmdbuf->cmd_base;
    cmdbuf->cmd_idx = (IMG_UINT32 *) cmdbuf->cmd_base;

    cmdbuf->reloc_base = cmdbuf->cmd_base + CMD_SIZE;
    cmdbuf->reloc_idx = (struct drm_psb_reloc *) cmdbuf->reloc_base;

    /* Add ourselves to the buffer list */
    tng_cmdbuf_buffer_ref(cmdbuf, &cmdbuf->buf); /* cmd buf == 0 */
    return ret;
}

/*
 * Unmap buffer
 *
 * Returns 0 on success
 */
int tng_cmdbuf_unmap(tng_cmdbuf_p cmdbuf)
{
    cmdbuf->cmd_base = NULL;
    cmdbuf->cmd_start = NULL;
    cmdbuf->cmd_idx = NULL;
    cmdbuf->reloc_base = NULL;
    cmdbuf->reloc_idx = NULL;
    cmdbuf->cmd_count = 0;
    psb_buffer_unmap(&cmdbuf->buf);
    return 0;
}


/*
 * Reference an addtional buffer "buf" in the command stream
 * Returns a reference index that can be used to refer to "buf" in
 * relocation records, -1 on error
 */
int tng_cmdbuf_buffer_ref(tng_cmdbuf_p cmdbuf, psb_buffer_p buf)
{
    int item_loc = 0;

    /*Reserve the same TTM BO twice will cause kernel lock up*/
    while ((item_loc < cmdbuf->buffer_refs_count)
           && (wsbmKBufHandle(wsbmKBuf(cmdbuf->buffer_refs[item_loc]->drm_buf))
               != wsbmKBufHandle(wsbmKBuf(buf->drm_buf))))
        //while( (item_loc < cmdbuf->buffer_refs_count) && (cmdbuf->buffer_refs[item_loc] != buf) )
    {
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
void tng_cmdbuf_add_relocation(tng_cmdbuf_p cmdbuf,
                               IMG_UINT32 *addr_in_dst_buffer,/*addr of dst_buffer for the DWORD*/
                               psb_buffer_p ref_buffer,
                               IMG_UINT32 buf_offset,
                               IMG_UINT32 mask,
                               IMG_UINT32 background,
                               IMG_UINT32 align_shift,
                               IMG_UINT32 dst_buffer,
                               IMG_UINT32 *start_of_dst_buffer) /*Index of the list refered by cmdbuf->buffer_refs */
{
    struct drm_psb_reloc *reloc = cmdbuf->reloc_idx;
    uint64_t presumed_offset = wsbmBOOffsetHint(ref_buffer->drm_buf);

    reloc->where = addr_in_dst_buffer - start_of_dst_buffer; /* Offset in DWORDs */

    reloc->buffer = tng_cmdbuf_buffer_ref(cmdbuf, ref_buffer);
    ASSERT(reloc->buffer != -1);

    reloc->reloc_op = PSB_RELOC_OP_OFFSET;
#ifndef VA_EMULATOR
    if (presumed_offset) {
        IMG_UINT32 new_val =  presumed_offset + buf_offset;

        new_val = ((new_val >> align_shift) << (align_shift << PSB_RELOC_ALSHIFT_SHIFT));
        new_val = (background & ~mask) | (new_val & mask);
        *addr_in_dst_buffer = new_val;
    } else {
        *addr_in_dst_buffer = PSB_RELOC_MAGIC;
    }
#else
    /* indicate subscript of relocation buffer */
    *addr_in_dst_buffer = reloc - (struct drm_psb_reloc *)cmdbuf->reloc_base;
#endif
    reloc->mask = mask;
    reloc->shift = align_shift << PSB_RELOC_ALSHIFT_SHIFT;
    reloc->pre_add = buf_offset;
    reloc->background = background;
    reloc->dst_buffer = dst_buffer;
    cmdbuf->reloc_idx++;

    ASSERT(((void *)(cmdbuf->reloc_idx)) < RELOC_END(cmdbuf));
}

/* Prepare one command package */
void tng_cmdbuf_insert_command(
    object_context_p obj_context,
    IMG_UINT32 stream_id,
    IMG_UINT32 cmd_id,
    IMG_UINT32 cmd_data,
    psb_buffer_p data_addr,
    IMG_UINT32 offset)
{
    IMG_UINT32 cmd_word;
    context_ENC_p ctx = (context_ENC_p) obj_context->format_data;
    context_ENC_cmdbuf *ps_cmd = &(ctx->ctx_cmdbuf[stream_id]);
    context_ENC_mem *ps_mem = &(ctx->ctx_mem[stream_id]);
    tng_cmdbuf_p cmdbuf = obj_context->tng_cmdbuf;
    psb_driver_data_p driver_data = ctx->obj_context->driver_data;
    int interrupt_flags;

    //CMD composed by user space does not generate Interrupt
    interrupt_flags = 0;

    assert(stream_id <= TOPAZHP_MAX_NUM_STREAMS);
   
    /* Write command to FIFO */
    {
        cmd_word = F_ENCODE(stream_id, MTX_MSG_CORE) |
            F_ENCODE(cmd_id, MTX_MSG_CMD_ID);

        if (cmd_id & MTX_CMDID_PRIORITY) {
            /* increment the command counter */
            ps_cmd->ui32HighCmdCount++;

            /* Prepare high priority command */
            cmd_word |= F_ENCODE(1, MTX_MSG_PRIORITY) |
                F_ENCODE(((ps_cmd->ui32LowCmdCount - 1) & 0xff) |(ps_cmd->ui32HighCmdCount << 8), MTX_MSG_COUNT);
        } else {
            /* Prepare low priority command */
            cmd_word |=
            F_ENCODE(ps_cmd->ui32LowCmdCount & 0xff, MTX_MSG_COUNT);
            ++(ps_cmd->ui32LowCmdCount);
        }
        drv_debug_msg(VIDEO_DEBUG_GENERAL,
            "%s: cmd_id = 0x%08x\n",
            __FUNCTION__, cmd_word);
        *cmdbuf->cmd_idx++ = cmd_word;
    }

    /* write command word into cmdbuf */
    *cmdbuf->cmd_idx++ = cmd_data;
/* Command data address */
    if (data_addr) {
        if (cmd_id == MTX_CMDID_RC_UPDATE) {
            *cmdbuf->cmd_idx++ = (IMG_UINT32)data_addr;
            drv_debug_msg(VIDEO_DEBUG_GENERAL,
                        "%s: data_addr = 0x%08x\n",
                        __FUNCTION__, *(cmdbuf->cmd_idx));
        } else {
            if ((cmd_id >= MTX_CMDID_SETQUANT) && (cmd_id <= MTX_CMDID_SETUP)) {
                if (cmd_id == MTX_CMDID_ISSUEBUFF)
                    TNG_RELOC_CMDBUF_START(cmdbuf->cmd_idx, offset, data_addr);
                else
                    tng_cmdbuf_set_phys(cmdbuf->cmd_idx, 0, data_addr, offset, 0);
            } else {
#ifdef _TNG_RELOC_
                TNG_RELOC_CMDBUF_START(cmdbuf->cmd_idx, offset, data_addr);
#else
                tng_cmdbuf_set_phys(cmdbuf->cmd_idx, 0, data_addr, offset, 0);
#endif
            }
            drv_debug_msg(VIDEO_DEBUG_GENERAL,
                "%s: data_addr = 0x%08x\n",
                __FUNCTION__, *(cmdbuf->cmd_idx));
            cmdbuf->cmd_idx++;
        }
    } else {
        *cmdbuf->cmd_idx++ = 0;
    }

    if (cmd_id == MTX_CMDID_SW_FILL_INPUT_CTRL) {
        *cmdbuf->cmd_idx++ = ctx->ui16IntraRefresh;
        *cmdbuf->cmd_idx++ = ctx->sRCParams.ui32InitialQp;
        *cmdbuf->cmd_idx++ = ctx->sRCParams.iMinQP;
        *cmdbuf->cmd_idx++ = ctx->ctx_mem_size.mb_ctrl_in_params;
        *cmdbuf->cmd_idx++ = ctx->ui32pseudo_rand_seed;
    }

    if (cmd_id == MTX_CMDID_SW_UPDATE_AIR_SEND) {
        *cmdbuf->cmd_idx++ = ctx->sAirInfo.i16AIRSkipCnt;
        *cmdbuf->cmd_idx++ = ctx->sAirInfo.i32NumAIRSPerFrame;
        *cmdbuf->cmd_idx++ = ctx->ctx_mem_size.mb_ctrl_in_params;
        *cmdbuf->cmd_idx++ = ctx->ui32FrameCount[0];
    }

    if (cmd_id == MTX_CMDID_SW_UPDATE_AIR_CALC) {
        *cmdbuf->cmd_idx++ = ctx->ctx_mem_size.first_pass_out_best_multipass_param;
        *cmdbuf->cmd_idx++ = ctx->sAirInfo.i32SAD_Threshold;
        *cmdbuf->cmd_idx++ = ctx->ui8EnableSelStatsFlags;
    }

    /* Command data address */
    if (cmd_id == MTX_CMDID_SETVIDEO) {
        *(cmdbuf->cmd_idx)++ = wsbmKBufHandle(wsbmKBuf(ctx->bufs_writeback.drm_buf));
        drv_debug_msg(VIDEO_DEBUG_GENERAL,
            "%s: cmd_param = 0x%08x\n",
            __FUNCTION__, *(cmdbuf->cmd_idx - 1));

    if (data_addr)
        *(cmdbuf->cmd_idx)++ = wsbmKBufHandle(wsbmKBuf(data_addr->drm_buf));

        *(cmdbuf->cmd_idx)++ = wsbmKBufHandle(wsbmKBuf(ps_mem->bufs_mb_ctrl_in_params.drm_buf));
        *(cmdbuf->cmd_idx)++ =  wsbmKBufHandle(wsbmKBuf(ps_mem->bufs_first_pass_out_params.drm_buf));
        *(cmdbuf->cmd_idx)++ =  wsbmKBufHandle(wsbmKBuf(ps_mem->bufs_first_pass_out_best_multipass_param.drm_buf));
    }

    if (cmd_id == MTX_CMDID_SETUP_INTERFACE) {
        *(cmdbuf->cmd_idx)++ = wsbmKBufHandle(wsbmKBuf(ctx->bufs_writeback.drm_buf));
        drv_debug_msg(VIDEO_DEBUG_GENERAL,
            "%s: cmd_param = 0x%08x\n",
            __FUNCTION__, *(cmdbuf->cmd_idx - 1));
    }

    if (cmd_id == MTX_CMDID_SHUTDOWN) {
        *(cmdbuf->cmd_idx)++ = ctx->eCodec;

        drv_debug_msg(VIDEO_DEBUG_GENERAL,
            "%s: cmd_param = 0x%08x\n",
            __FUNCTION__, *(cmdbuf->cmd_idx - 1));
    }

    return ;
}


/*
 * Advances "obj_context" to the next cmdbuf
 *
 * Returns 0 on success
 */
int tng_context_get_next_cmdbuf(object_context_p obj_context)
{
    tng_cmdbuf_p cmdbuf;
    int ret;

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s: start obj_context->tng_cmdbuf = %x\n", __FUNCTION__, obj_context->tng_cmdbuf);

    if (obj_context->tng_cmdbuf) {
        return 0;
    }

    obj_context->cmdbuf_current++;
    if (obj_context->cmdbuf_current >= TNG_MAX_CMDBUFS_ENCODE) {
        obj_context->cmdbuf_current = 0;
    }

    cmdbuf = obj_context->tng_cmdbuf_list[obj_context->cmdbuf_current];
    ret = tng_cmdbuf_reset(cmdbuf);
    if (!ret) {
        /* Success */
        obj_context->tng_cmdbuf = cmdbuf;
    }

//    tng_cmdbuf_buffer_ref(cmdbuf, &cmdbuf->pic_params);
//    tng_cmdbuf_buffer_ref(cmdbuf, &cmdbuf->slice_mem);
    return ret;
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
ptgDRMCmdBuf(int fd, int ioctl_offset, psb_buffer_p *buffer_list, int buffer_count, unsigned cmdBufHandle,
             unsigned cmdBufOffset, unsigned cmdBufSize,
             unsigned relocBufHandle, unsigned relocBufOffset,
             unsigned numRelocs, int __maybe_unused damage,
             unsigned engine, unsigned fence_flags, struct psb_ttm_fence_rep *fence_rep)
{
    drm_psb_cmdbuf_arg_t ca;
    struct psb_validate_arg *arg_list;
    int i, ret;
    unsigned int retry = 0;
    uint64_t mask = PSB_GPU_ACCESS_MASK;

    arg_list = (struct psb_validate_arg *) calloc(1, sizeof(struct psb_validate_arg) * buffer_count);
    if (arg_list == NULL) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "Allocate memory failed\n");
        return -ENOMEM;
    }

    for (i = 0; i < buffer_count; i++) {
        struct psb_validate_arg *arg = &(arg_list[i]);
        struct psb_validate_req *req = &arg->d.req;

        //memset(arg, 0, sizeof(*arg));
        req->next = (unsigned long) & (arg_list[i+1]);

        req->buffer_handle = wsbmKBufHandle(wsbmKBuf(buffer_list[i]->drm_buf));
        //req->group = 0;
        req->set_flags = (PSB_GPU_ACCESS_READ | PSB_GPU_ACCESS_WRITE) & mask;
        req->clear_flags = (~(PSB_GPU_ACCESS_READ | PSB_GPU_ACCESS_WRITE)) & mask;
#if 1
        req->presumed_gpu_offset = (uint64_t)wsbmBOOffsetHint(buffer_list[i]->drm_buf);
        req->presumed_flags = PSB_USE_PRESUMED;
#else
        req->presumed_flags = 0;
#endif
        req->pad64 = (IMG_UINT32)buffer_list[i]->pl_flags;
#ifndef BAYTRAIL
        req->unfence_flag = buffer_list[i]->unfence_flag;
#endif
    }
    arg_list[buffer_count-1].d.req.next = 0;

    memset(&ca, 0, sizeof(ca));

    ca.buffer_list = (uint64_t)((unsigned long)arg_list);
    ca.cmdbuf_handle = cmdBufHandle;
    ca.cmdbuf_offset = cmdBufOffset;
    ca.cmdbuf_size = cmdBufSize;
    ca.reloc_handle = relocBufHandle;
    ca.reloc_offset = relocBufOffset;
    ca.num_relocs = numRelocs;
    ca.engine = engine;
    ca.fence_flags = fence_flags;
    ca.fence_arg = (uint64_t)((unsigned long)fence_rep);
    //ca.damage = damage;


    do {
        ret = drmCommandWrite(fd, ioctl_offset, &ca, sizeof(ca));
        if (ret == -EAGAIN || ret == -EBUSY) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "drmCommandWrite returns with %s, retry\n",
                          ret==-EAGAIN?"EAGAIN":"EBUSY");
            retry++;
        }
    } while (ret == -EAGAIN || ret == -EBUSY);

    if (retry > 0)
        drv_debug_msg(VIDEO_DEBUG_ERROR,"drmCommandWrite tries %d time, finally %s with ret=%d\n",
                      retry, ret==0?"okay":"failed!", ret);

    if (ret)
        goto out;

    for (i = 0; i < buffer_count; i++) {
        struct psb_validate_arg *arg = &(arg_list[i]);
        struct psb_validate_rep *rep = &arg->d.rep;

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
        switch (buffer_list[i]->status) {
        case psb_bs_queued:
            buffer_list[i]->status = psb_bs_ready;
            break;

        case psb_bs_abandoned:
            psb_buffer_destroy(buffer_list[i]);
            free(buffer_list[i]);
            break;

        default:
            /* Not supposed to happen */
            ASSERT(0);
        }
    }

    return ret;
}

#if 0
static struct _WsbmFenceObject *
lnc_fence_wait(psb_driver_data_p driver_data,
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
                            (void *)fence_rep->handle,
                            0);
    if (fence)
        *status = wsbmFenceFinish(fence, fence_rep->fence_type, 0);

    return fence;
}
#endif

/*
 * Submits the current cmdbuf
 *
 * Returns 0 on success
 */
int tng_context_submit_cmdbuf(object_context_p __maybe_unused obj_context)
{
    return 0;
}



/*
 * FrameSkip is only meaningful for RC enabled mode
 * Topaz raises this flag after surface N encoding is finished (vaSyncSurface gets back)
 * then for the next encode surface N+1 (ctx->src_surface) frameskip flag is cleared in vaBeginPicuture
 * and is always set in vaEndPicture:lnc_PatchRCMode
 * vaQuerySurfaceStatus is supposed only to be called after vaEndPicture/vaSyncSurface,
 * The caller should ensure the surface pertains to an encode context
 */
int tng_surface_get_frameskip(psb_driver_data_p __maybe_unused driver_data,
                              psb_surface_p surface,
                              int *frame_skip)
{
    /* bit31 indicate if frameskip is already settled, it is used to record the frame skip flag for old surfaces
     * bit31 is cleared when the surface is used as encode render target or reference/reconstrucure target
     */
    if (GET_SURFACE_INFO_skipped_flag(surface) & SURFACE_INFO_SKIP_FLAG_SETTLED) {
        *frame_skip = GET_SURFACE_INFO_skipped_flag(surface) & 1;
    } else
        *frame_skip = 0;

    return 0;
}

VAStatus tng_set_frame_skip_flag(object_context_p obj_context)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    context_ENC_p ctx = (context_ENC_p) obj_context->format_data;
    context_ENC_frame_buf *ps_buf = &(ctx->ctx_frame_buf);

    if (ctx && ps_buf->previous_src_surface) {
        SET_SURFACE_INFO_skipped_flag(ps_buf->previous_src_surface->psb_surface, 1);
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "Detected a skipped frame for surface 0x%08x.\n",
            ps_buf->previous_src_surface->psb_surface);
    }

    return vaStatus;
}


/*
 * Flushes all cmdbufs
 */
int tng_context_flush_cmdbuf(object_context_p obj_context)
{
    tng_cmdbuf_p cmdbuf = obj_context->tng_cmdbuf;
    psb_driver_data_p driver_data = obj_context->driver_data;
    unsigned int fence_flags;
    struct psb_ttm_fence_rep fence_rep;
    unsigned int reloc_offset;
    unsigned int num_relocs;
    int ret;
    unsigned int cmdbuffer_size = (unsigned int) (((unsigned char *)(cmdbuf->cmd_idx)) - cmdbuf->cmd_start); /* In bytes */

    ASSERT(cmdbuffer_size < CMD_SIZE);
    ASSERT((void *) cmdbuf->cmd_idx < CMD_END(cmdbuf));
    /* LOCK */
    ret = LOCK_HARDWARE(driver_data);
    if (ret) {
        UNLOCK_HARDWARE(driver_data);
        DEBUG_FAILURE_RET;
        return ret;
    }

    /* Now calculate the total number of relocations */
    reloc_offset = cmdbuf->reloc_base - cmdbuf->cmd_base;
    num_relocs = (((unsigned char *) (cmdbuf->reloc_idx)) - cmdbuf->reloc_base) / sizeof(struct drm_psb_reloc);

    tng_cmdbuf_unmap(cmdbuf);

    ASSERT(NULL == cmdbuf->reloc_base);

#ifdef DEBUG_TRACE
    fence_flags = 0;
#else
    fence_flags = DRM_PSB_FENCE_NO_USER;
#endif


    wsbmWriteLockKernelBO();
#if 1 //_PO_DEBUG_
    ret = ptgDRMCmdBuf(driver_data->drm_fd, driver_data->execIoctlOffset, /* FIXME Still use ioctl cmd? */
                       cmdbuf->buffer_refs, cmdbuf->buffer_refs_count, wsbmKBufHandle(wsbmKBuf(cmdbuf->buf.drm_buf)),
                       0, cmdbuffer_size,/*unsigned cmdBufSize*/
                       wsbmKBufHandle(wsbmKBuf(cmdbuf->buf.drm_buf)), reloc_offset, num_relocs,
                       0, LNC_ENGINE_ENCODE, fence_flags, &fence_rep); /* FIXME use LNC_ENGINE_ENCODE */
#endif
    wsbmWriteUnlockKernelBO();

    UNLOCK_HARDWARE(driver_data);

    if (ret) {
        obj_context->tng_cmdbuf = NULL;

        DEBUG_FAILURE_RET;
        return ret;
    }

#if 0 /*DEBUG_TRACE*/
    int status = -1;
    struct _WsbmFenceObject *fence = NULL;

    fence = lnc_fence_wait(driver_data, &fence_rep, &status);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "psb_fence_wait returns: %d (fence=0x%08x)\n", status, fence);

    if (fence)
        wsbmFenceUnreference(fence);
#endif

    obj_context->tng_cmdbuf = NULL;

    return 0;
}


void tng_cmdbuf_set_phys(IMG_UINT32 *dest_buf, int dest_num,
    psb_buffer_p ref_buf, unsigned int ref_ofs, unsigned int ref_len)
{
    int i = 0;
    IMG_UINT32 addr_phys = (IMG_UINT32)wsbmBOOffsetHint(ref_buf->drm_buf) + ref_ofs;

//    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s: drm_buf 0x%08x, addr_phys 0x%08x, virt addr 0x%08x\n", __FUNCTION__, ref_buf->drm_buf, addr_phys, ref_buf->virtual_addr );

    do {
        dest_buf[i] =  addr_phys;
        ++i;
        addr_phys += ref_len;
    } while(i < dest_num);
    return ;
}


int tng_get_pipe_number(object_context_p obj_context)
{

    context_ENC_p ctx = (context_ENC_p)(obj_context->format_data);
    return ctx->ui8PipesToUse;

}

