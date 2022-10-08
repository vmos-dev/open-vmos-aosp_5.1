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


#include "pnw_cmdbuf.h"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <wsbm/wsbm_manager.h>

#include "psb_def.h"
#include "psb_drv_debug.h"
#include "pnw_hostcode.h"
#include "psb_ws_driver.h"

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


/*
 * Create command buffer
 */
VAStatus pnw_cmdbuf_create(
    object_context_p obj_context,
    psb_driver_data_p driver_data,
    pnw_cmdbuf_p cmdbuf)
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
        free(cmdbuf->buffer_refs);
        cmdbuf->buffer_refs = NULL;
        cmdbuf->buffer_refs_allocated = 0;
        return vaStatus;
    }

    /* create topaz parameter buffer */
    vaStatus = psb_buffer_create(driver_data, ctx->pic_params_size * MAX_TOPAZ_CORES, psb_bt_cpu_vpu, &cmdbuf->pic_params);
    if (VA_STATUS_SUCCESS != vaStatus)
        goto error_out5;

    /* create header buffer */
    vaStatus = psb_buffer_create(driver_data, ctx->header_buffer_size, psb_bt_cpu_vpu, &cmdbuf->header_mem);
    if (VA_STATUS_SUCCESS != vaStatus)
        goto error_out2;

    /* create slice parameter buffer */
    vaStatus = psb_buffer_create(driver_data, ctx->sliceparam_buffer_size, psb_bt_cpu_vpu, &cmdbuf->slice_params);
    if (VA_STATUS_SUCCESS != vaStatus)
        goto error_out1;

    /* all cmdbuf share one MTX_CURRENT_IN_PARAMS since every MB has a MTX_CURRENT_IN_PARAMS structure
     * and filling this structure for all MB is very time-consuming
     */

    cmdbuf->topaz_in_params_I = &ctx->topaz_in_params_I;
    cmdbuf->topaz_in_params_P = &ctx->topaz_in_params_P;

    cmdbuf->topaz_below_params = &ctx->topaz_below_params;
    cmdbuf->topaz_above_params = &ctx->topaz_above_params;

    return vaStatus;

error_out1:
    psb_buffer_destroy(&cmdbuf->header_mem);
error_out2:
    psb_buffer_destroy(&cmdbuf->pic_params);
error_out5:
    pnw_cmdbuf_destroy(cmdbuf);

    return vaStatus;
}

/*
 * Destroy buffer
 */
void pnw_cmdbuf_destroy(pnw_cmdbuf_p cmdbuf)
{
    if (cmdbuf->size) {
        psb_buffer_destroy(&cmdbuf->buf);
        cmdbuf->size = 0;
    }
    if (cmdbuf->buffer_refs_allocated) {
        free(cmdbuf->buffer_refs);
        cmdbuf->buffer_refs = NULL;
        cmdbuf->buffer_refs_allocated = 0;
    }

    psb_buffer_destroy(&cmdbuf->pic_params);
    psb_buffer_destroy(&cmdbuf->header_mem);
    psb_buffer_destroy(&cmdbuf->slice_params);

}

/*
 * Reset buffer & map
 *
 * Returns 0 on success
 */
int pnw_cmdbuf_reset(pnw_cmdbuf_p cmdbuf)
{
    int ret;

    cmdbuf->cmd_base = NULL;
    cmdbuf->cmd_idx = NULL;
    cmdbuf->reloc_base = NULL;
    cmdbuf->reloc_idx = NULL;

    cmdbuf->buffer_refs_count = 0;
    cmdbuf->cmd_count = 0;

    ret = psb_buffer_map(&cmdbuf->buf, &cmdbuf->cmd_base);
    if (ret) {
        return ret;
    }

    cmdbuf->cmd_start = cmdbuf->cmd_base;
    cmdbuf->cmd_idx = (uint32_t *) cmdbuf->cmd_base;

    cmdbuf->reloc_base = cmdbuf->cmd_base + CMD_SIZE;
    cmdbuf->reloc_idx = (struct drm_psb_reloc *) cmdbuf->reloc_base;

    /* Add ourselves to the buffer list */
    pnw_cmdbuf_buffer_ref(cmdbuf, &cmdbuf->buf); /* cmd buf == 0 */
    return ret;
}

/*
 * Unmap buffer
 *
 * Returns 0 on success
 */
int pnw_cmdbuf_unmap(pnw_cmdbuf_p cmdbuf)
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
int pnw_cmdbuf_buffer_ref(pnw_cmdbuf_p cmdbuf, psb_buffer_p buf)
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
void pnw_cmdbuf_add_relocation(pnw_cmdbuf_p cmdbuf,
                               uint32_t *addr_in_dst_buffer,/*addr of dst_buffer for the DWORD*/
                               psb_buffer_p ref_buffer,
                               uint32_t buf_offset,
                               uint32_t mask,
                               uint32_t background,
                               uint32_t align_shift,
                               uint32_t dst_buffer,
                               uint32_t *start_of_dst_buffer) /*Index of the list refered by cmdbuf->buffer_refs */
{
    struct drm_psb_reloc *reloc = cmdbuf->reloc_idx;
    uint64_t presumed_offset = wsbmBOOffsetHint(ref_buffer->drm_buf);

    reloc->where = addr_in_dst_buffer - start_of_dst_buffer; /* Offset in DWORDs */

    reloc->buffer = pnw_cmdbuf_buffer_ref(cmdbuf, ref_buffer);
    ASSERT(reloc->buffer != -1);

    reloc->reloc_op = PSB_RELOC_OP_OFFSET;
#ifndef VA_EMULATOR
    if (presumed_offset) {
        uint32_t new_val =  presumed_offset + buf_offset;

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

    ASSERT(((unsigned char *)(cmdbuf->reloc_idx)) < RELOC_END(cmdbuf));
}

/* Prepare one command package */
void pnw_cmdbuf_insert_command_package(object_context_p obj_context,
                                       int32_t core,
                                       uint32_t cmd_id,
                                       psb_buffer_p command_data,
                                       uint32_t offset)
{
    uint32_t cmd_word;
    context_ENC_p ctx = (context_ENC_p) obj_context->format_data;
    pnw_cmdbuf_p cmdbuf = obj_context->pnw_cmdbuf;
    int interrupt_flags;

    /* only MTX_CMDID_END_PIC by master core generate interrupt */
    interrupt_flags = (cmd_id == MTX_CMDID_END_PIC) | (core == 0);
    interrupt_flags = 0; /*CMD composed by user space does not generate Interrupt*/

    /* Calculate command word */
    cmd_id &= MTX_CMDWORD_ID_MASK;
    core &= MTX_CMDWORD_CORE_MASK;

    core %= MAX_TOPAZ_CORES;

    cmd_word = ((ctx->CmdCount & MTX_CMDWORD_COUNT_MASK) << MTX_CMDWORD_COUNT_SHIFT)
               | ((0 & MTX_CMDWORD_INT_MASK) << MTX_CMDWORD_INT_SHIFT) /* Do not generate interrupt */
               | (core << MTX_CMDWORD_CORE_SHIFT)
               | (cmd_id << MTX_CMDWORD_ID_SHIFT);

    /* write command word into cmdbuf */
    *cmdbuf->cmd_idx++ = cmd_word;

    /* Command data address */
    if (command_data) {
        RELOC_CMDBUF_PNW(cmdbuf->cmd_idx, offset, command_data);
        cmdbuf->cmd_idx++;
    } else {
        *cmdbuf->cmd_idx++ = 0;
    }


    *cmdbuf->cmd_idx++ = 0;             /* Write back buffer address */

    *cmdbuf->cmd_idx++ = 0; /*ctx->CmdCount; */ /* Write back value */

    ctx->LastSync[(ctx->FrmIdx) & 0x1][core] = ctx->CmdCount;

    /* increment the command counter */
    ctx->CmdCount = (ctx->CmdCount + 1) % MAX_TOPAZ_CMD_COUNT;
}

/*
 * Advances "obj_context" to the next cmdbuf
 *
 * Returns 0 on success
 */
int pnw_context_get_next_cmdbuf(object_context_p obj_context)
{
    pnw_cmdbuf_p cmdbuf;
    int ret;

    if (obj_context->pnw_cmdbuf) {
        return 0;
    }

    obj_context->cmdbuf_current++;
    if (obj_context->cmdbuf_current >= PNW_MAX_CMDBUFS_ENCODE) {
        obj_context->cmdbuf_current = 0;
    }

    cmdbuf = obj_context->pnw_cmdbuf_list[obj_context->cmdbuf_current];
    ret = pnw_cmdbuf_reset(cmdbuf);
    if (!ret) {
        /* Success */
        obj_context->pnw_cmdbuf = cmdbuf;
    }

    /* added pic_params/slice_params into ref, so the index is 1/2 */
    pnw_cmdbuf_buffer_ref(cmdbuf, &cmdbuf->pic_params);
    pnw_cmdbuf_buffer_ref(cmdbuf, &cmdbuf->slice_params);

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
pnwDRMCmdBuf(int fd, int ioctl_offset, psb_buffer_p *buffer_list, int buffer_count, unsigned cmdBufHandle,
             unsigned cmdBufOffset, unsigned cmdBufSize,
             unsigned relocBufHandle, unsigned relocBufOffset,
             unsigned numRelocs, int __maybe_unused damage,
             unsigned engine, unsigned fence_flags, struct psb_ttm_fence_rep *fence_rep)
{
    drm_psb_cmdbuf_arg_t ca;
    struct psb_validate_arg *arg_list;
    int i;
    int ret;
    uint64_t mask = PSB_GPU_ACCESS_MASK;

    arg_list = (struct psb_validate_arg *) calloc(1, sizeof(struct psb_validate_arg) * buffer_count);
    if (arg_list == NULL) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "Allocate memory failed\n");
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
#if 1
        req->presumed_gpu_offset = (uint64_t)wsbmBOOffsetHint(buffer_list[i]->drm_buf);
        req->presumed_flags = PSB_USE_PRESUMED;
#else
        req->presumed_flags = 0;
#endif
        req->pad64 = (uint32_t)buffer_list[i]->pl_flags;
    }
    arg_list[buffer_count-1].d.req.next = 0;

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

    do {
        ret = drmCommandWrite(fd, ioctl_offset, &ca, sizeof(ca));
    } while (ret == EAGAIN);

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
                            (unsigned char *)fence_rep->handle,
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
int pnw_context_submit_cmdbuf(object_context_p __maybe_unused obj_context)
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
int pnw_surface_get_frameskip(psb_driver_data_p __maybe_unused driver_data,
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


/*
 * Flushes all cmdbufs
 */
int pnw_context_flush_cmdbuf(object_context_p obj_context)
{
    pnw_cmdbuf_p cmdbuf = obj_context->pnw_cmdbuf;
    psb_driver_data_p driver_data = obj_context->driver_data;
    unsigned int fence_flags;
    struct psb_ttm_fence_rep fence_rep;
    unsigned int reloc_offset;
    unsigned int num_relocs;
    int ret;
    unsigned int cmdbuffer_size = (unsigned char *) cmdbuf->cmd_idx - cmdbuf->cmd_start; /* In bytes */

    ASSERT(cmdbuffer_size < CMD_SIZE);
    ASSERT((unsigned char *) cmdbuf->cmd_idx < CMD_END(cmdbuf));
    /* LOCK */
    ret = LOCK_HARDWARE(driver_data);
    if (ret) {
        UNLOCK_HARDWARE(driver_data);
        DEBUG_FAILURE_RET;
        return ret;
    }

    /* Now calculate the total number of relocations */
    reloc_offset = cmdbuf->reloc_base - cmdbuf->cmd_base;
    num_relocs = (((unsigned char *) cmdbuf->reloc_idx) - cmdbuf->reloc_base) / sizeof(struct drm_psb_reloc);

    pnw_cmdbuf_unmap(cmdbuf);

    ASSERT(NULL == cmdbuf->reloc_base);

    if (psb_video_trace_fp)
        fence_flags = 0;
    else
        fence_flags = DRM_PSB_FENCE_NO_USER;

#ifndef LNC_ENGINE_ENCODE
#define LNC_ENGINE_ENCODE  5
#endif

    wsbmWriteLockKernelBO();
    ret = pnwDRMCmdBuf(driver_data->drm_fd, driver_data->execIoctlOffset, /* FIXME Still use ioctl cmd? */
                       cmdbuf->buffer_refs, cmdbuf->buffer_refs_count, wsbmKBufHandle(wsbmKBuf(cmdbuf->buf.drm_buf)),
                       0, cmdbuffer_size,/*unsigned cmdBufSize*/
                       wsbmKBufHandle(wsbmKBuf(cmdbuf->buf.drm_buf)), reloc_offset, num_relocs,
                       0, LNC_ENGINE_ENCODE, fence_flags, &fence_rep); /* FIXME use LNC_ENGINE_ENCODE */

    wsbmWriteUnlockKernelBO();
    UNLOCK_HARDWARE(driver_data);

    if (ret) {
        obj_context->pnw_cmdbuf = NULL;

        DEBUG_FAILURE_RET;
        return ret;
    }

#if 0
    int status = -1;
    struct _WsbmFenceObject *fence = NULL;

    fence = lnc_fence_wait(driver_data, &fence_rep, &status);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "psb_fence_wait returns: %d (fence=0x%08x)\n", status, fence);

    if (fence)
        wsbmFenceUnreference(fence);
#endif

    obj_context->pnw_cmdbuf = NULL;

    return 0;
}


int pnw_get_parallel_core_number(object_context_p obj_context)
{

    context_ENC_p ctx = (context_ENC_p)(obj_context->format_data);
    return ctx->ParallelCores;

}
