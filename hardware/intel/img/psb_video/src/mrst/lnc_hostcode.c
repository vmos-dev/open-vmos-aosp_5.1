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
 *    Zeng Li <zeng.li@intel.com>
 *    Shengquan Yuan  <shengquan.yuan@intel.com>
 *    Binglin Chen <binglin.chen@intel.com>
 *
 */



#include "psb_drv_video.h"

#include "lnc_hostcode.h"
#include "hwdefs/topaz_defs.h"
#include "psb_def.h"
#include "psb_cmdbuf.h"
#include <stdio.h>
#include "psb_output.h"
#include <wsbm/wsbm_manager.h>
#include "lnc_hostheader.h"
#include "psb_drv_debug.h"

#define ALIGN_TO(value, align) ((value + align - 1) & ~(align - 1))
#define PAGE_ALIGN(value) ALIGN_TO(value, 4096)

static VAStatus lnc__alloc_context_buffer(context_ENC_p ctx)
{
    int width, height;
    VAStatus vaStatus = VA_STATUS_SUCCESS;

    /* width and height should be source surface's w and h or ?? */
    width = ctx->obj_context->picture_width;
    height = ctx->obj_context->picture_height;

    ctx->pic_params_size  = 256;

    ctx->header_buffer_size = 4 * HEADER_SIZE + MAX_SLICES_PER_PICTURE * HEADER_SIZE;

    ctx->seq_header_ofs = 0;
    ctx->pic_header_ofs = HEADER_SIZE;
    ctx->eoseq_header_ofs = 2 * HEADER_SIZE;
    ctx->eostream_header_ofs = 3 * HEADER_SIZE;
    ctx->slice_header_ofs = 4 * HEADER_SIZE;

    ctx->sliceparam_buffer_size = ((sizeof(SLICE_PARAMS) + 15) & 0xfff0) * MAX_SLICES_PER_PICTURE;

    /* All frame share same MTX_CURRENT_IN_PARAMS and above/bellow param
     * create MTX_CURRENT_IN_PARAMS buffer seperately
     * every MB has one MTX_CURRENT_IN_PARAMS structure, and the (N+1) frame can
     * reuse (N) frame's structure
     */
    ctx->in_params_size = (10 + width * height / (16 * 16)) * sizeof(MTX_CURRENT_IN_PARAMS);
    ctx->bellow_params_size = BELOW_PARAMS_SIZE * width * height / (16 * 16) * 16;
    ctx->above_params_size = (width * height / 16) * 128 + 15;

    ctx->topaz_buffer_size = ctx->in_params_size + /* MTX_CURRENT_IN_PARAMS size */
                             ctx->bellow_params_size + /* above_params */
                             ctx->above_params_size; /* above_params */

    vaStatus = psb_buffer_create(ctx->obj_context->driver_data, ctx->in_params_size, psb_bt_cpu_vpu, &ctx->topaz_in_params_I);
    vaStatus |= psb_buffer_create(ctx->obj_context->driver_data, ctx->in_params_size, psb_bt_cpu_vpu, &ctx->topaz_in_params_P);
    vaStatus |= psb_buffer_create(ctx->obj_context->driver_data, ctx->above_params_size + ctx->bellow_params_size, psb_bt_cpu_vpu, &ctx->topaz_above_bellow_params);

    ctx->in_params_ofs = 0;
    ctx->bellow_params_ofs = 0;
    ctx->above_params_ofs = ctx->bellow_params_ofs + ctx->bellow_params_size;

    return vaStatus;
}

unsigned int lnc__get_ipe_control(enum drm_lnc_topaz_codec  eEncodingFormat)
{
    unsigned int RegVal = 0;

    RegVal = F_ENCODE(2, MVEA_CR_IPE_GRID_FINE_SEARCH) |
             F_ENCODE(0, MVEA_CR_IPE_GRID_SEARCH_SIZE) |
             F_ENCODE(1, MVEA_CR_IPE_Y_FINE_SEARCH);

    switch (eEncodingFormat) {
    case IMG_CODEC_H263_NO_RC:
    case IMG_CODEC_H263_VBR:
    case IMG_CODEC_H263_CBR:
        RegVal |= F_ENCODE(0, MVEA_CR_IPE_BLOCKSIZE) | F_ENCODE(0, MVEA_CR_IPE_ENCODING_FORMAT);
        break;
    case IMG_CODEC_MPEG4_NO_RC:
    case IMG_CODEC_MPEG4_VBR:
    case IMG_CODEC_MPEG4_CBR:
        RegVal |= F_ENCODE(1, MVEA_CR_IPE_BLOCKSIZE) | F_ENCODE(1, MVEA_CR_IPE_ENCODING_FORMAT);
    default:
        break;
    case IMG_CODEC_H264_NO_RC:
    case IMG_CODEC_H264_VBR:
    case IMG_CODEC_H264_CBR:
    case IMG_CODEC_H264_VCM:
        RegVal |= F_ENCODE(2, MVEA_CR_IPE_BLOCKSIZE) | F_ENCODE(2, MVEA_CR_IPE_ENCODING_FORMAT);
        break;
    }
    RegVal |= F_ENCODE(6, MVEA_CR_IPE_Y_CANDIDATE_NUM);
    return RegVal;
}


void lnc_DestroyContext(object_context_p obj_context)
{
    context_ENC_p ctx;
    ctx = (context_ENC_p)obj_context->format_data;
    if (NULL != ctx->slice_param_cache)
        free(ctx->slice_param_cache);
    if (NULL == ctx->save_seq_header_p)
        free(ctx->save_seq_header_p);
    free(obj_context->format_data);
    obj_context->format_data = NULL;
}

VAStatus lnc_CreateContext(
    object_context_p obj_context,
    object_config_p obj_config)
{
    int width, height;
    context_ENC_p ctx;
    VAStatus vaStatus;

    width = obj_context->picture_width;
    height = obj_context->picture_height;
    ctx = (context_ENC_p) calloc(1, sizeof(struct context_ENC_s));
    if (NULL == ctx) {
        vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
        DEBUG_FAILURE;
        return vaStatus;
    }

    obj_context->format_data = (void*) ctx;
    ctx->obj_context = obj_context;

    ctx->RawWidth = (unsigned short) width;
    ctx->RawHeight = (unsigned short) height;

    ctx->Width = (unsigned short)(~0xf & (width + 0xf));
    ctx->Height = (unsigned short)(~0xf & (height + 0xf));

    ctx->HeightMinus16MinusLRBTopOffset = ctx->Height - (MVEA_LRB_TOP_OFFSET + 16);
    ctx->HeightMinus32MinusLRBTopOffset = ctx->Height - (MVEA_LRB_TOP_OFFSET + 32);
    ctx->HeightMinusLRB_TopAndBottom_OffsetsPlus16 = ctx->Height - (MVEA_LRB_TOP_OFFSET + MVEA_LRB_TOP_OFFSET + 16);
    ctx->HeightMinusLRBSearchHeight = ctx->Height - MVEA_LRB_SEARCH_HEIGHT;

    ctx->FCode = 0;

    ctx->sRCParams.VCMBitrateMargin = 0;
    ctx->sRCParams.BufferSize = 0;
    ctx->sRCParams.InitialQp = 0;
    ctx->sRCParams.MinQP = 0;

    vaStatus = lnc__alloc_context_buffer(ctx);

    return vaStatus;
}


VAStatus lnc_BeginPicture(context_ENC_p ctx)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    lnc_cmdbuf_p cmdbuf;
    int ret;

    ctx->src_surface = ctx->obj_context->current_render_target;

    /* clear frameskip flag to 0 */
    CLEAR_SURFACE_INFO_skipped_flag(ctx->src_surface->psb_surface);

    /* Initialise the command buffer */
    ret = lnc_context_get_next_cmdbuf(ctx->obj_context);
    if (ret) {
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "get next cmdbuf fail\n");
        vaStatus = VA_STATUS_ERROR_UNKNOWN;
        return vaStatus;
    }
    cmdbuf = ctx->obj_context->lnc_cmdbuf;

    /* map start_pic param */
    vaStatus = psb_buffer_map(&cmdbuf->pic_params, &cmdbuf->pic_params_p);
    if (vaStatus) {
        return vaStatus;
    }
    vaStatus = psb_buffer_map(&cmdbuf->header_mem, &cmdbuf->header_mem_p);
    if (vaStatus) {
        psb_buffer_unmap(&cmdbuf->pic_params);
        return vaStatus;
    }

    vaStatus = psb_buffer_map(&cmdbuf->slice_params, &cmdbuf->slice_params_p);
    if (vaStatus) {
        psb_buffer_unmap(&cmdbuf->pic_params);
        psb_buffer_unmap(&cmdbuf->header_mem);
        return vaStatus;
    }

    /* only map topaz param when necessary */
    cmdbuf->topaz_in_params_I_p = NULL;
    cmdbuf->topaz_in_params_P_p = NULL;
    cmdbuf->topaz_above_bellow_params_p = NULL;

    if (ctx->obj_context->frame_count == 0) { /* first picture */
        psb_driver_data_p driver_data = ctx->obj_context->driver_data;

        lnc_cmdbuf_insert_command(cmdbuf, MTX_CMDID_SW_NEW_CODEC, 3, driver_data->drm_context);
        lnc_cmdbuf_insert_command_param(cmdbuf, ctx->eCodec);
        lnc_cmdbuf_insert_command_param(cmdbuf, (ctx->Width << 16) | ctx->Height);
    }

    /* insert START_PIC command */
    lnc_cmdbuf_insert_command(cmdbuf, MTX_CMDID_START_PIC, 3, ctx->obj_context->frame_count);
    /* write the address of structure PIC_PARAMS following command MTX_CMDID_START_PIC
     * the content of PIC_PARAMS is filled when RenderPicture(...,VAEncPictureParameterBufferXXX)
     */
    RELOC_CMDBUF(cmdbuf->cmd_idx, 0, &cmdbuf->pic_params);
    cmdbuf->cmd_idx++;
    ctx->initial_qp_in_cmdbuf = cmdbuf->cmd_idx; /* remember the place */
    cmdbuf->cmd_idx++;

    ctx->obj_context->slice_count = 0;

    /* no RC paramter provided in vaBeginPicture
     * so delay RC param setup into vaRenderPicture(SequenceHeader...)
     */
    return vaStatus;
}


VAStatus lnc_RenderPictureParameter(context_ENC_p ctx)
{
    PIC_PARAMS *psPicParams;    /* PIC_PARAMS has been put in lnc_hostcode.h */
    object_surface_p src_surface;
    unsigned int srf_buf_offset;
    object_surface_p rec_surface;
    object_surface_p ref_surface;
    lnc_cmdbuf_p cmdbuf = ctx->obj_context->lnc_cmdbuf;
    VAStatus vaStatus = VA_STATUS_ERROR_UNKNOWN;

    psPicParams = (PIC_PARAMS *)cmdbuf->pic_params_p;

    /* second frame will reuse some rate control parameters (IN_PARAMS_MP4)
     * so only memset picture parames except IN_PARAMS
     * BUT now IN_RC_PARAMS was reload from the cache, so it now can
     * memset entirE PIC_PARAMS
     */
    memset(psPicParams, 0, (int)((unsigned char *)&psPicParams->sInParams - (unsigned char *)psPicParams));

    src_surface = ctx->src_surface;
    if (NULL == src_surface) {
        vaStatus = VA_STATUS_ERROR_INVALID_SURFACE;
        DEBUG_FAILURE;
        return vaStatus;
    }

    rec_surface = ctx->dest_surface;
    if (NULL == rec_surface) {
        vaStatus = VA_STATUS_ERROR_INVALID_SURFACE;
        DEBUG_FAILURE;
        return vaStatus;
    }

    /*The fisrt frame always is I frame and the content of reference frame wouldn't be used.
     * But the heights of ref and dest frame should be the same.
     * That allows Topaz to keep its motion vectors up to date, which helps maintain performance */
    if (ctx->obj_context->frame_count == 0)
        ctx->ref_surface = ctx->dest_surface;

    ref_surface = ctx->ref_surface;
    if (NULL == ref_surface) {
        vaStatus = VA_STATUS_ERROR_INVALID_SURFACE;
        DEBUG_FAILURE;
        return vaStatus;
    }

    /* clear frameskip flag */
    CLEAR_SURFACE_INFO_skipped_flag(rec_surface->psb_surface);
    CLEAR_SURFACE_INFO_skipped_flag(ref_surface->psb_surface);

    /* Write video data byte offset into Coded buffer
     * If it is here, it will be a SYNC point, which have performance impact
     * Move to psb__CreateBuffer
     vaStatus = psb_buffer_map(ctx->coded_buf->psb_buffer, &pBuffer);
     if(vaStatus) {
     DEBUG_FAILURE;
     return vaStatus;
     }
     *(IMG_UINT32 *)(pBuffer+8) = 16;
     psb_buffer_unmap(ctx->coded_buf->psb_buffer);
    */

    psPicParams->SrcYStride = src_surface->psb_surface->stride;
    switch (ctx->eFormat) {
    case IMG_CODEC_IYUV:        /* IYUV */
    case IMG_CODEC_PL8:
        psPicParams->SrcUVStride = src_surface->psb_surface->stride / 2;
        psPicParams->SrcUVRowStride = src_surface->psb_surface->stride * 16 / 2;
        break;
    case IMG_CODEC_IMC2:    /* IMC2 */
    case IMG_CODEC_PL12:
        psPicParams->SrcUVStride = src_surface->psb_surface->stride;
        psPicParams->SrcUVRowStride = src_surface->psb_surface->stride * 16;
        break;
    default:
        break;
    }
    psPicParams->SrcYRowStride    = src_surface->psb_surface->stride * 16;
    /* psPicParams->SrcUVRowStride = src_surface->psb_surface->stride * 16 / 2; */

    /* Dest(rec) stride
     * The are now literally Dst stride (not equivalent to 'offset to next row')
     */
#ifdef VA_EMULATOR
    /* only for simulator, va-emulator needs the actually stride for
     * reconstructed frame transfer (va-emulator->driver)
     */
    psPicParams->DstYStride = rec_surface->psb_surface->stride;
    psPicParams->DstUVStride = rec_surface->psb_surface->stride;
    psPicParams->DstYRowStride = rec_surface->psb_surface->stride * 16;
    psPicParams->DstUVRowStride = rec_surface->psb_surface->stride * 16 / 2;
#else
    psPicParams->DstYStride = rec_surface->height * 16;
    psPicParams->DstUVStride = rec_surface->height * 16 / 2;
    psPicParams->DstYRowStride = psPicParams->DstYStride;
    psPicParams->DstUVRowStride = psPicParams->DstUVStride;
#endif

    psPicParams->InParamsRowStride = (ctx->obj_context->picture_width / 16) * 256;
    psPicParams->BelowParamRowStride = (ctx->obj_context->picture_width / 16) * 32;

    psPicParams->Width  = ctx->Width;
    psPicParams->Height = ctx->Height;

    /* not sure why we are setting this up here... */
    psPicParams->Flags = 0;
    switch (ctx->eCodec) {
    case IMG_CODEC_H264_NO_RC:
    case IMG_CODEC_H264_VBR:
    case IMG_CODEC_H264_CBR:
    case IMG_CODEC_H264_VCM:
        psPicParams->Flags |= ISH264_FLAGS;
        break;
    case IMG_CODEC_H263_VBR:
    case IMG_CODEC_H263_CBR:
    case IMG_CODEC_H263_NO_RC:
        psPicParams->Flags |= ISH263_FLAGS;
        break;
    case IMG_CODEC_MPEG4_NO_RC:
    case IMG_CODEC_MPEG4_VBR:
    case IMG_CODEC_MPEG4_CBR:
        psPicParams->Flags |= ISMPEG4_FLAGS;
        break;
    default:
        return VA_STATUS_ERROR_UNKNOWN;
    }

    switch (ctx->eCodec) {
    case IMG_CODEC_H264_VBR:
    case IMG_CODEC_MPEG4_VBR:
    case IMG_CODEC_H263_VBR:
        psPicParams->Flags |= ISVBR_FLAGS;
        break;
    case IMG_CODEC_H264_VCM:
        psPicParams->Flags |= ISVCM_FLAGS;
        /* drop through to CBR case */
    case IMG_CODEC_H263_CBR:
    case IMG_CODEC_H264_CBR:
    case IMG_CODEC_MPEG4_CBR:
        psPicParams->Flags |= ISCBR_FLAGS;
        break;
    case IMG_CODEC_MPEG4_NO_RC:
    case IMG_CODEC_H263_NO_RC:
    case IMG_CODEC_H264_NO_RC:
        break;
    default:
        return VA_STATUS_ERROR_UNKNOWN;
    }


    if (ctx->sRCParams.RCEnable) {
        /* for the first frame, will setup RC params in EndPicture */
        if (ctx->obj_context->frame_count > 0) { /* reuse in_params parameter */
            /* reload IN_RC_PARAMS from cache */
            memcpy(&psPicParams->sInParams, &ctx->in_params_cache, sizeof(IN_RC_PARAMS));

            /* delay these into END_PICTURE timeframe */
            /*
            psPicParams->sInParams.BitsTransmitted = ctx->sRCParams.BitsTransmitted;
            */
        }
    } else
        psPicParams->sInParams.SeInitQP = ctx->sRCParams.InitialQp;

    /* some relocations have to been done here */
    srf_buf_offset = src_surface->psb_surface->buf.buffer_ofs;
    if (src_surface->psb_surface->buf.type == psb_bt_camera)
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "src surface GPU offset 0x%08x, luma offset 0x%08x\n",
                                 wsbmBOOffsetHint(src_surface->psb_surface->buf.drm_buf), srf_buf_offset);

    RELOC_PIC_PARAMS(&psPicParams->SrcYBase, srf_buf_offset, &src_surface->psb_surface->buf);
    switch (ctx->eFormat) {
    case IMG_CODEC_IYUV:
    case IMG_CODEC_PL8:
    case IMG_CODEC_PL12:
        RELOC_PIC_PARAMS(&psPicParams->SrcUBase,
                         srf_buf_offset + src_surface->psb_surface->chroma_offset,
                         &src_surface->psb_surface->buf);

        RELOC_PIC_PARAMS(&psPicParams->SrcVBase,
                         srf_buf_offset + src_surface->psb_surface->chroma_offset,
                         &src_surface->psb_surface->buf);

        break;
    case IMG_CODEC_IMC2:
    case IMG_CODEC_NV12:
        break;
    }

    /*
     * Do not forget this!
     * MTXWriteMem(MTXData.ui32CCBCtrlAddr + MTX_CCBCTRL_QP, sRCParams.ui32InitialQp);
     */
    /* following START_PIC, insert initial QP */
    *ctx->initial_qp_in_cmdbuf = ctx->sRCParams.InitialQp;


    RELOC_PIC_PARAMS(&psPicParams->DstYBase, 0, &rec_surface->psb_surface->buf);

    RELOC_PIC_PARAMS(&psPicParams->DstUVBase,
                     rec_surface->psb_surface->stride * rec_surface->height,
                     &rec_surface->psb_surface->buf);

    RELOC_PIC_PARAMS(&psPicParams->CodedBase, 0, ctx->coded_buf->psb_buffer);

    /* MTX_CURRENT_IN_PARAMS buffer is seperate buffer now */
    /*The type of frame will decide psPicParams->InParamsBase should
     * use cmdbuf->topaz_in_params_P or cmdbuf->topaz_in_params_I*/
    /*RELOC_PIC_PARAMS(&psPicParams->InParamsBase, ctx->in_params_ofs, cmdbuf->topaz_in_params_P);*/
    RELOC_PIC_PARAMS(&psPicParams->BelowParamsBase, ctx->bellow_params_ofs, cmdbuf->topaz_above_bellow_params);
    RELOC_PIC_PARAMS(&psPicParams->AboveParamsBase, ctx->above_params_ofs, cmdbuf->topaz_above_bellow_params);

    return VA_STATUS_SUCCESS;
}

static VAStatus lnc__PatchBitsConsumedInRCParam(context_ENC_p ctx)
{
    lnc_cmdbuf_p cmdbuf = ctx->obj_context->lnc_cmdbuf;
    /* PIC_PARAMS  *psPicParams = cmdbuf->pic_params_p; */
    VAStatus vaStatus;

    (void)cmdbuf;
    /* it will wait until last encode session is done */
    /* now it just wait the last session is done and the frame skip
     * is  */
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "will patch bits consumed for rc\n");
    if (ctx->pprevious_coded_buf) {
        vaStatus = psb_buffer_sync(ctx->pprevious_coded_buf->psb_buffer);
        if (vaStatus)
            return vaStatus;
    }

    return VA_STATUS_SUCCESS;
}

static VAStatus lnc_RedoRenderPictureSkippedFrame(context_ENC_p ctx)
{
    lnc_cmdbuf_p cmdbuf = ctx->obj_context->lnc_cmdbuf;
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    int i = 0;

    /* reset cmdbuf to skip existing picture/slice DO_HEAD commands */
    cmdbuf->cmd_idx = cmdbuf->cmd_idx_saved_frameskip;

    switch (ctx->eCodec) {
    case IMG_CODEC_H263_CBR: /* H263 don't need picture header/slice header, only reset command buf */
    case IMG_CODEC_H263_VBR:
        break;
    case IMG_CODEC_H264_VBR:
    case IMG_CODEC_H264_CBR: /* slice header needs redo */
    case IMG_CODEC_H264_VCM: {
        /* only need one slice header here */
        VAEncSliceParameterBuffer *pBuffer = &ctx->slice_param_cache[i];
        unsigned int MBSkipRun, FirstMBAddress;
        int deblock_on;

        if ((pBuffer->slice_flags.bits.disable_deblocking_filter_idc == 0)
            || (pBuffer->slice_flags.bits.disable_deblocking_filter_idc == 2))
            deblock_on = IMG_TRUE;
        else
            deblock_on = IMG_FALSE;

        if (1 /* ctx->sRCParams.RCEnable && ctx->sRCParams.FrameSkip */) /* we know it is true */
            MBSkipRun = (ctx->Width * ctx->Height) / 256;
        else
            MBSkipRun = 0;

        FirstMBAddress = (pBuffer->start_row_number * ctx->Width) / 16;
        /* Insert Do Header command, relocation is needed */

        lnc__H264_prepare_slice_header((IMG_UINT32 *)(cmdbuf->header_mem_p + ctx->slice_header_ofs + i * HEADER_SIZE),
                                       0, /*pBuffer->slice_flags.bits.is_intra*/
                                       pBuffer->slice_flags.bits.disable_deblocking_filter_idc,
                                       ctx->obj_context->frame_count,
                                       FirstMBAddress,
                                       MBSkipRun,
                                       (ctx->obj_context->frame_count == 0),
                                       pBuffer->slice_flags.bits.uses_long_term_ref,
                                       pBuffer->slice_flags.bits.is_long_term_ref,
                                       ctx->idr_pic_id);


        lnc_cmdbuf_insert_command(cmdbuf, MTX_CMDID_DO_HEADER, 2, (i << 2) | 2);
        RELOC_CMDBUF(cmdbuf->cmd_idx++,
                     ctx->slice_header_ofs + i * HEADER_SIZE,
                     &cmdbuf->header_mem);
    }

    break;
    case IMG_CODEC_MPEG4_VBR:
    case IMG_CODEC_MPEG4_CBR: /* only picture header need redo */
        lnc__MPEG4_prepare_vop_header((IMG_UINT32 *)(cmdbuf->header_mem_p + ctx->pic_header_ofs),
                                      IMG_FALSE /* bIsVOPCoded is false now */,
                                      ctx->MPEG4_vop_time_increment_frameskip, /* In testbench, this should be FrameNum */
                                      4,/* default value is 4,search range */
                                      ctx->MPEG4_picture_type_frameskip,
                                      ctx->MPEG4_vop_time_increment_resolution/* defaule value */);

        lnc_cmdbuf_insert_command(cmdbuf, MTX_CMDID_DO_HEADER, 2, 1);
        RELOC_CMDBUF(cmdbuf->cmd_idx++, ctx->pic_header_ofs, &cmdbuf->header_mem);
        vaStatus = lnc_RenderPictureParameter(ctx);
        break;
    default:
        drv_debug_msg(VIDEO_DEBUG_ERROR, "Non-RC mode should be here for FrameSkip handling\n");
        ASSERT(0);
    }

    return vaStatus;
}

static VAStatus lnc_SetupRCParam(context_ENC_p ctx)
{
    lnc_cmdbuf_p cmdbuf = ctx->obj_context->lnc_cmdbuf;
    PIC_PARAMS  *psPicParams = (PIC_PARAMS  *)cmdbuf->pic_params_p;
    int origin_qp;/* in DDK setup_rc will change qp strangly,
                   * just for keep same with DDK
                   */

    origin_qp = ctx->sRCParams.InitialQp;

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "will setup rc data\n");

    psPicParams->Flags |= ISRC_FLAGS;
    lnc__setup_rcdata(ctx, psPicParams, &ctx->sRCParams);

    /* restore it, just keep same with DDK */
    ctx->sRCParams.InitialQp = origin_qp;

    /* save IN_RC_PARAMS into the cache */
    memcpy(&ctx->in_params_cache, (unsigned char *)&psPicParams->sInParams, sizeof(IN_RC_PARAMS));

    return VA_STATUS_SUCCESS;
}

static VAStatus lnc_UpdateRCParam(context_ENC_p ctx)
{
    int origin_qp;
    lnc_cmdbuf_p cmdbuf = ctx->obj_context->lnc_cmdbuf;
    PIC_PARAMS  *psPicParams = (PIC_PARAMS  *)cmdbuf->pic_params_p;

    origin_qp = ctx->sRCParams.InitialQp;

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "will update rc data\n");
    lnc__update_rcdata(ctx, psPicParams, &ctx->sRCParams);

    /* set minQP if hosts set minQP */
    if (ctx->sRCParams.MinQP)
        psPicParams->sInParams.MinQPVal = ctx->sRCParams.MinQP;

    /* if seinitqp is set, restore the value hosts want */
    if (origin_qp) {
        psPicParams->sInParams.SeInitQP = origin_qp;
        psPicParams->sInParams.MyInitQP = origin_qp;
        ctx->sRCParams.InitialQp = origin_qp;
    }

    /* save IN_RC_PARAMS into the cache */
    memcpy(&ctx->in_params_cache, (unsigned char *)&psPicParams->sInParams, sizeof(IN_RC_PARAMS));

    return VA_STATUS_SUCCESS;
}

static VAStatus lnc_PatchRCMode(context_ENC_p ctx)
{
    int frame_skip = 0;

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "will patch rc data\n");
    /* it will ensure previous encode finished */
    lnc__PatchBitsConsumedInRCParam(ctx);

    /* get frameskip flag */
    lnc_surface_get_frameskip(ctx->obj_context->driver_data, ctx->src_surface->psb_surface, &frame_skip);
    /* current frame is skipped
     * redo RenderPicture with FrameSkip set
     */
    if (frame_skip == 1)
        lnc_RedoRenderPictureSkippedFrame(ctx);

    return VA_STATUS_SUCCESS;
}

VAStatus lnc_EndPicture(context_ENC_p ctx)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    lnc_cmdbuf_p cmdbuf = ctx->obj_context->lnc_cmdbuf;

    if (ctx->sRCParams.RCEnable == IMG_TRUE) {
        if (ctx->obj_context->frame_count == 0)
            lnc_SetupRCParam(ctx);
        else if (ctx->update_rc_control)
            lnc_UpdateRCParam(ctx);
        else
            lnc_PatchRCMode(ctx);
    }
    ctx->update_rc_control = 0;

    /* save current settings */
    ctx->previous_src_surface = ctx->src_surface;
    ctx->previous_ref_surface = ctx->ref_surface;
    ctx->previous_dest_surface = ctx->dest_surface; /* reconstructed surface */
    ctx->pprevious_coded_buf = ctx->previous_coded_buf;
    ctx->previous_coded_buf = ctx->coded_buf;

    lnc_cmdbuf_insert_command(cmdbuf, MTX_CMDID_END_PIC, 3, 0);
    lnc_cmdbuf_insert_command_param(cmdbuf, 0);/* two meaningless parameters */
    lnc_cmdbuf_insert_command_param(cmdbuf, 0);
    psb_buffer_unmap(&cmdbuf->pic_params);
    psb_buffer_unmap(&cmdbuf->header_mem);
    psb_buffer_unmap(&cmdbuf->slice_params);

    /* unmap MTX_CURRENT_IN_PARAMS buffer only when it is mapped */
    if (cmdbuf->topaz_in_params_I_p != NULL) {
        psb_buffer_unmap(cmdbuf->topaz_in_params_I);
        cmdbuf->topaz_in_params_I_p = NULL;
    }

    if (cmdbuf->topaz_in_params_P_p != NULL) {
        psb_buffer_unmap(cmdbuf->topaz_in_params_P);
        cmdbuf->topaz_in_params_P_p = NULL;
    }

    if (cmdbuf->topaz_above_bellow_params_p != NULL) {
        psb_buffer_unmap(cmdbuf->topaz_above_bellow_params);
        cmdbuf->topaz_above_bellow_params_p = NULL;
    }

    if (lnc_context_flush_cmdbuf(ctx->obj_context)) {
        vaStatus = VA_STATUS_ERROR_UNKNOWN;
    }

    return vaStatus;
}

static void lnc__setup_busize(context_ENC_p ctx)
{
    unsigned int old_busize = ctx->sRCParams.BUSize;

    /* it is called at EndPicture, we should now the Slice number */
    ctx->Slices = ctx->obj_context->slice_count;

    /* if no BU size is given then pick one ourselves */
    if (ctx->sRCParams.BUSize != 0)  { /* application provided BUSize */
        IMG_UINT32 MBs, MBsperSlice, MBsLastSlice;
        IMG_UINT32 BUs;
        IMG_INT32  SliceHeight;

        MBs     = ctx->Height * ctx->Width / (16 * 16);

        SliceHeight     = ctx->Height / ctx->Slices;
        /* SliceHeight += 15; */
        SliceHeight &= ~15;

        MBsperSlice     = (SliceHeight * ctx->Width) / (16 * 16);
        MBsLastSlice    = MBs - (MBsperSlice * (ctx->Slices - 1));

        /* they have given us a basic unit so validate it */
        if (ctx->sRCParams.BUSize < 6) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "ERROR: Basic unit size too small, must be greater than 6\n");
            ctx->sRCParams.BUSize = 0; /* need repatch */;
        }
        if (ctx->sRCParams.BUSize > MBsperSlice) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "ERROR: Basic unit size too large, must be less than the number of macroblocks in a slice\n");
            ctx->sRCParams.BUSize = 0; /* need repatch */;
        }
        if (ctx->sRCParams.BUSize > MBsLastSlice) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "ERROR: Basic unit size too large, must be less than number of macroblocks in the last slice\n");
            ctx->sRCParams.BUSize = 0; /* need repatch */;
        }
        BUs = MBsperSlice / ctx->sRCParams.BUSize;
        if ((BUs * ctx->sRCParams.BUSize) != MBsperSlice)   {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "ERROR: Basic unit size not an integer divisor of MB's in a slice");
            ctx->sRCParams.BUSize = 0; /* need repatch */;
        }
        if (BUs > 200) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "ERROR: Basic unit size too small. There must be less than 200 basic units per slice");
            ctx->sRCParams.BUSize = 0; /* need repatch */;
        }
        BUs = MBsLastSlice / ctx->sRCParams.BUSize;
        if (BUs > 200) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "ERROR: Basic unit size too small. There must be less than 200 basic units per slice");
            ctx->sRCParams.BUSize = 0; /* need repatch */;
        }
    }

    if (ctx->sRCParams.BUSize == 0)  {
        IMG_UINT32 MBs, MBsperSlice, MBsLastSlice;
        IMG_UINT32 BUs, BUsperSlice, BUsLastSlice;
        IMG_INT32  SliceHeight;

        MBs     = ctx->Height * ctx->Width / (16 * 16);

        SliceHeight     = ctx->Height / ctx->Slices;
        /* SliceHeight += 15; */
        SliceHeight &= ~15;

        MBsperSlice     = (SliceHeight * ctx->Width) / (16 * 16);
        MBsLastSlice = MBs - (MBsperSlice * (ctx->Slices - 1));

        /* we have to verify that MBs is divisiable by BU AND that BU is > pipeline length */
        if (ctx->sRCParams.BUSize < 6)  {
            ctx->sRCParams.BUSize = 6;
        }

        BUs = MBs / ctx->sRCParams.BUSize;
        while (BUs*ctx->sRCParams.BUSize != MBs) {
            ctx->sRCParams.BUSize++;
            BUs = MBs / ctx->sRCParams.BUSize;
        }

        /* Check number of BUs in the pipe is less than maximum number allowed 200  */
        BUsperSlice = MBsperSlice / ctx->sRCParams.BUSize;
        BUsLastSlice = MBsLastSlice / ctx->sRCParams.BUSize;
        while ((BUsperSlice  *(ctx->Slices - 1) + BUsLastSlice) > 200)  {
            ctx->sRCParams.BUSize++;
            BUsperSlice = MBsperSlice / ctx->sRCParams.BUSize;
            BUsLastSlice = MBsLastSlice / ctx->sRCParams.BUSize;
        }

        /* Check whether there are integer number of BUs in the slices  */
        BUsperSlice = MBsperSlice / ctx->sRCParams.BUSize;
        BUsLastSlice = MBsLastSlice / ctx->sRCParams.BUSize;
        while ((BUsperSlice*ctx->sRCParams.BUSize != MBsperSlice) ||
               (BUsLastSlice*ctx->sRCParams.BUSize != MBsLastSlice))   {
            ctx->sRCParams.BUSize++;
            BUsperSlice = MBsperSlice / ctx->sRCParams.BUSize;
            BUsLastSlice = MBsLastSlice / ctx->sRCParams.BUSize;
        }

        if (ctx->sRCParams.BUSize != old_busize)
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "Patched Basic unit to %d (original=%d)\n", ctx->sRCParams.BUSize, old_busize);
    }
}


/***********************************************************************************
 * Function Name      : SetupRCData
 * Inputs             :
 * Outputs            :
 * Returns            :
 * Description        : Sets up RC Data
 ************************************************************************************/
void lnc__setup_rcdata(
    context_ENC_p psContext,
    PIC_PARAMS *psPicParams,
    IMG_RC_PARAMS *psRCParams)
{
    IMG_UINT32   max_bitrate = psContext->Width * psContext->Height * 1.5 * 8 * 60;
    IMG_UINT8 InitialSeInitQP = 0;

    /* frameskip is always cleared, specially handled at vaEndPicture */
    psRCParams->FrameSkip = 0;

    if (!psRCParams->BitsPerSecond)
        psRCParams->BitsPerSecond = 64000;
    if (psRCParams->BitsPerSecond > max_bitrate)
        psRCParams->BitsPerSecond = max_bitrate;

    if (!psRCParams->FrameRate)
        psRCParams->FrameRate = 30;

    if (psRCParams->BufferSize == 0) {
        if (psRCParams->BitsPerSecond < 256000)
            psRCParams->BufferSize = (9 * psRCParams->BitsPerSecond) >> 1;
        else
            psRCParams->BufferSize = (5 * psRCParams->BitsPerSecond) >> 1;
    }
    psRCParams->InitialLevel = (3 * psRCParams->BufferSize) >> 4;
    psRCParams->InitialDelay = (13 * psRCParams->BufferSize) >> 4;

    lnc__setup_busize(psContext); /* calculate BasicUnitSize */

    psPicParams->sInParams.SeInitQP = psRCParams->InitialQp;

    psPicParams->sInParams.MBPerRow = (psContext->Width >> 4);
    psPicParams->sInParams.MBPerBU = psRCParams->BUSize;
    psPicParams->sInParams.MBPerFrm     = (psContext->Width >> 4) * (psContext->Height >> 4);
    psPicParams->sInParams.BUPerFrm     = (psPicParams->sInParams.MBPerFrm) / psRCParams->BUSize;

    InitialSeInitQP = psPicParams->sInParams.SeInitQP;

    lnc__update_rcdata(psContext, psPicParams, psRCParams);
    /* set minQP if hosts set minQP */
    if (psRCParams->MinQP)
        psPicParams->sInParams.MinQPVal = psRCParams->MinQP;

    /* if seinitqp is set, restore the value hosts want */
    if (InitialSeInitQP) {
        psPicParams->sInParams.SeInitQP = InitialSeInitQP;
        psPicParams->sInParams.MyInitQP = InitialSeInitQP;
        psRCParams->InitialQp = InitialSeInitQP;
    }
}

void lnc__update_rcdata(context_ENC_p psContext,
                        PIC_PARAMS *psPicParams,
                        IMG_RC_PARAMS *psRCParams)
{
    double              L1, L2, L3, L4, L5, flBpp;
    INT16               i16TempQP;
    IMG_INT32   i32BufferSizeInFrames;

    flBpp                                                                       = 1.0 * psRCParams->BitsPerSecond / (psRCParams->FrameRate * psContext->Width * psContext->Height);

    /* recalculate for small frames */
    if (psContext->Width <= 176)
        flBpp = flBpp / 2.0;

    psPicParams->sInParams.IntraPeriod  = psRCParams->IntraFreq;
    psPicParams->sInParams.BitRate              = psRCParams->BitsPerSecond;
    psPicParams->sInParams.IntraPeriod  = psRCParams->IntraFreq;

    psPicParams->sInParams.BitsPerFrm   = psRCParams->BitsPerSecond / psRCParams->FrameRate;
    psPicParams->sInParams.BitsPerGOP   = psPicParams->sInParams.BitsPerFrm * psRCParams->IntraFreq;
    psPicParams->sInParams.BitsPerBU            = psPicParams->sInParams.BitsPerFrm / (4 * psPicParams->sInParams.BUPerFrm);
    psPicParams->sInParams.BitsPerMB            = psPicParams->sInParams.BitsPerBU / psRCParams->BUSize;

    i32BufferSizeInFrames = psRCParams->BufferSize / psPicParams->sInParams.BitsPerFrm;

    // select thresholds and initial Qps etc that are codec dependent
    switch (psContext->eCodec) {
    case IMG_CODEC_H264_CBR:
    case IMG_CODEC_H264_VCM:
    case IMG_CODEC_H264_VBR:
        L1 = 0.1;
        L2 = 0.15;
        L3 = 0.2;
        psPicParams->sInParams.MaxQPVal = 51;

        // Set THSkip Values
        if (flBpp <= 0.07)
            psPicParams->THSkip = TH_SKIP_24;
        else if (flBpp <= 0.14)
            psPicParams->THSkip = TH_SKIP_12;
        else
            psPicParams->THSkip = TH_SKIP_0;

        if (flBpp <= 0.3)
            psPicParams->Flags |= ISRC_I16BIAS;

        // Setup MAX and MIN Quant Values
        if (flBpp >= 0.50)
            i16TempQP = 4;
        else if (flBpp > 0.133)
            i16TempQP = (unsigned int)(24 - (40 * flBpp));
        else
            i16TempQP = (unsigned int)(32 - (100 * flBpp));

        psPicParams->sInParams.MinQPVal = (max(min(psPicParams->sInParams.MaxQPVal, i16TempQP), 0));
        // Calculate Initial QP if it has not been specified

        L1 = 0.050568;
        L2 = 0.202272;
        L3 = 0.40454321;
        L4 = 0.80908642;
        L5 = 1.011358025;
        if (flBpp < L1)
            i16TempQP = (IMG_INT16)(47 - 78.10 * flBpp);

        else if (flBpp >= L1 && flBpp < L2)
            i16TempQP = (IMG_INT16)(46 - 72.51 * flBpp);

        else if (flBpp >= L2 && flBpp < L3)
            i16TempQP = (IMG_INT16)(36 - 24.72 * flBpp);

        else if (flBpp >= L3 && flBpp < L4)
            i16TempQP = (IMG_INT16)(34 - 19.78 * flBpp);

        else if (flBpp >= L4 && flBpp < L5)
            i16TempQP = (IMG_INT16)(27 - 9.89 * flBpp);

        else if (flBpp >= L5)
            i16TempQP = (IMG_INT16)(20 - 4.95 * flBpp);

        psPicParams->sInParams.SeInitQP = (IMG_UINT8)(max(min(psPicParams->sInParams.MaxQPVal, i16TempQP), 0));
        break;

    case IMG_CODEC_MPEG4_CBR:
    case IMG_CODEC_MPEG4_VBR:
        psPicParams->sInParams.MaxQPVal  = 31;

        if (psContext->Width == 176) {
            L1 = 0.1;
            L2 = 0.3;
            L3 = 0.6;
        } else if (psContext->Width == 352) {
            L1 = 0.2;
            L2 = 0.6;
            L3 = 1.2;
        } else {
            L1 = 0.25;
            L2 = 1.4;
            L3 = 2.4;
        }

        // Calculate Initial QP if it has not been specified
        if (flBpp <= L1)
            psPicParams->sInParams.SeInitQP = 31;
        else {
            if (flBpp <= L2)
                psPicParams->sInParams.SeInitQP = 25;
            else
                psPicParams->sInParams.SeInitQP = (flBpp <= L3) ? 20 : 10;
        }

        if (flBpp >= 0.25) {
            psPicParams->sInParams.MinQPVal = 1;
        } else {
            psPicParams->sInParams.MinQPVal = 2;
        }
        break;

    case IMG_CODEC_H263_CBR:
    case IMG_CODEC_H263_VBR:
        psPicParams->sInParams.MaxQPVal  = 31;

        if (psContext->Width == 176) {
            L1 = 0.1;
            L2 = 0.3;
            L3 = 0.6;
        } else if (psContext->Width == 352) {
            L1 = 0.2;
            L2 = 0.6;
            L3 = 1.2;
        } else {
            L1 = 0.25;
            L2 = 1.4;
            L3 = 2.4;
        }

        // Calculate Initial QP if it has not been specified
        if (flBpp <= L1)
            psPicParams->sInParams.SeInitQP = 31;
        else {
            if (flBpp <= L2)
                psPicParams->sInParams.SeInitQP = 25;
            else
                psPicParams->sInParams.SeInitQP = (flBpp <= L3) ? 20 : 10;
        }

        psPicParams->sInParams.MinQPVal = 3;

        break;

    default:
        /* the NO RC cases will fall here */
        break;
    }

    // Set up Input Parameters that are mode dependent
    switch (psContext->eCodec) {
    case IMG_CODEC_H264_NO_RC:
    case IMG_CODEC_H263_NO_RC:
    case IMG_CODEC_MPEG4_NO_RC:
        return;

    case IMG_CODEC_H264_VCM:
        psPicParams->Flags                              |= ISVCM_FLAGS | ISCBR_FLAGS;
        /* drop through to CBR case */
        /* for SD and above we can target 95% (122/128) of maximum bitrate */
        if (psRCParams->VCMBitrateMargin) {
            psPicParams->sInParams.VCMBitrateMargin = psRCParams->VCMBitrateMargin;
        } else {
            if (psContext->Height >= 480)
                psPicParams->sInParams.VCMBitrateMargin = 122;
            else
                psPicParams->sInParams.VCMBitrateMargin = 115; /* for less and SD we target 90% (115/128) of maximum bitrate */
            if (i32BufferSizeInFrames < 15)
                psPicParams->sInParams.VCMBitrateMargin -= 5;/* when we have a very small window size we reduce the target further to avoid too much skipping */
        }
        psPicParams->sInParams.ForceSkipMargin = 500;/* start skipping MBs when within 500 bits of slice or frame limit */

        // Set a scale factor to avoid overflows in maths
        if (psRCParams->BitsPerSecond < 1000000) {      // 1 Mbits/s
            psPicParams->sInParams.ScaleFactor = 0;
        } else if (psRCParams->BitsPerSecond < 2000000) { // 2 Mbits/s
            psPicParams->sInParams.ScaleFactor = 1;
        } else if (psRCParams->BitsPerSecond < 4000000) { // 4 Mbits/s
            psPicParams->sInParams.ScaleFactor = 2;
        } else if (psRCParams->BitsPerSecond < 8000000) { // 8 Mbits/s
            psPicParams->sInParams.ScaleFactor = 3;
        } else {
            psPicParams->sInParams.ScaleFactor = 4;
        }

        psPicParams->sInParams.BufferSize = i32BufferSizeInFrames;
        break;

    case IMG_CODEC_H264_CBR:
        psPicParams->Flags                              |= ISCBR_FLAGS;
        // ------------------- H264 CBR RC ------------------- //
        // Initialize the parameters of fluid flow traffic model.
        psPicParams->sInParams.BufferSize = psRCParams->BufferSize;

        // HRD consideration - These values are used by H.264 reference code.
        if (psRCParams->BitsPerSecond < 1000000) {      // 1 Mbits/s
            psPicParams->sInParams.ScaleFactor = 0;
        } else if (psRCParams->BitsPerSecond < 2000000) { // 2 Mbits/s
            psPicParams->sInParams.ScaleFactor = 1;
        } else if (psRCParams->BitsPerSecond < 4000000) { // 4 Mbits/s
            psPicParams->sInParams.ScaleFactor = 2;
        } else if (psRCParams->BitsPerSecond < 8000000) { // 8 Mbits/s
            psPicParams->sInParams.ScaleFactor = 3;
        } else {
            psPicParams->sInParams.ScaleFactor = 4;
        }
        break;

    case IMG_CODEC_MPEG4_CBR:
    case IMG_CODEC_H263_CBR:
        psPicParams->Flags                                      |= ISCBR_FLAGS;

        flBpp  = 256 * (psRCParams->BitsPerSecond / psContext->Width);
        flBpp /= (psContext->Height * psRCParams->FrameRate);

        if ((psPicParams->sInParams.MBPerFrm > 1024 && flBpp < 16) || (psPicParams->sInParams.MBPerFrm <= 1024 && flBpp < 24))
            psPicParams->sInParams.HalfFrameRate = 1;
        else
            psPicParams->sInParams.HalfFrameRate = 0;

        if (psPicParams->sInParams.HalfFrameRate >= 1) {
            psPicParams->sInParams.SeInitQP = 31;
            psPicParams->sInParams.AvQPVal = 31;
            psPicParams->sInParams.MyInitQP = 31;
        }

        if (psRCParams->BitsPerSecond <= 384000)
            psPicParams->sInParams.BufferSize = ((psRCParams->BitsPerSecond * 5) >> 1);
        else
            psPicParams->sInParams.BufferSize = psRCParams->BitsPerSecond * 4;
        break;

    case IMG_CODEC_MPEG4_VBR:
    case IMG_CODEC_H263_VBR:
    case IMG_CODEC_H264_VBR:
        psPicParams->Flags                              |= ISVBR_FLAGS;

        psPicParams->sInParams.MBPerBU  = psPicParams->sInParams.MBPerFrm;
        psPicParams->sInParams.BUPerFrm = 1;

        // Initialize the parameters of fluid flow traffic model.
        psPicParams->sInParams.BufferSize       = ((5 * psRCParams->BitsPerSecond) >> 1);

        // These scale factor are used only for rate control to avoid overflow
        // in fixed-point calculation these scale factors are decided by bit rate
        if (psRCParams->BitsPerSecond < 640000) {
            psPicParams->sInParams.ScaleFactor  = 2;                                            // related to complexity
        } else if (psRCParams->BitsPerSecond < 2000000) {
            psPicParams->sInParams.ScaleFactor  = 4;
        } else {
            psPicParams->sInParams.ScaleFactor  = 6;
        }
        break;
    default:
        break;
    }

    psPicParams->sInParams.MyInitQP             = psPicParams->sInParams.SeInitQP;
    psPicParams->sInParams.InitialDelay = psRCParams->InitialDelay;
    psPicParams->sInParams.InitialLevel = psRCParams->InitialLevel;
    psRCParams->InitialQp                               = psPicParams->sInParams.SeInitQP;
}



static void lnc__setup_qpvalue_h264(
    MTX_CURRENT_IN_PARAMS * psCurrent,
    IMG_BYTE bySliceQP)
{
    /* H.264 QP scaling tables */
    IMG_BYTE HOST_PVR_QP_SCALE_CR[52] = {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
        12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27,
        28, 29, 29, 30, 31, 32, 32, 33, 34, 34, 35, 35, 36, 36, 37, 37,
        37, 38, 38, 38, 39, 39, 39, 39
    };

    psCurrent->bySliceQP = bySliceQP;
    psCurrent->bySliceQPC = HOST_PVR_QP_SCALE_CR[psCurrent->bySliceQP];
}


static void lnc__setup_qpvalues_mpeg4(
    MTX_CURRENT_IN_PARAMS * psCurrent,
    IMG_BYTE bySliceQP)
{
    psCurrent->bySliceQP =      bySliceQP;
}


static void lnc__setup_slice_row_params(
    context_ENC_p ctx,
    IMG_BOOL IsIntra,
    IMG_UINT16 CurrentRowY,
    IMG_INT16 SliceStartRowY,
    IMG_INT16 SliceHeight,
    IMG_BOOL VectorsValid,
    int bySliceQP)
{
    /* Note: CurrentRowY and SliceStartRowY are now in pixels (not MacroBlocks)
     * - saves needless multiplications and divisions
     */
    IMG_INT16   Pos, YPos, srcY;
    MTX_CURRENT_IN_PARAMS *psCurrent;
    lnc_cmdbuf_p cmdbuf = ctx->obj_context->lnc_cmdbuf;
    IMG_UINT16  tmp;

    if (IsIntra && cmdbuf->topaz_in_params_I_p == NULL) {
        VAStatus vaStatus = psb_buffer_map(cmdbuf->topaz_in_params_I, &cmdbuf->topaz_in_params_I_p);
        if (vaStatus != VA_STATUS_SUCCESS) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "map topaz MTX_CURRENT_IN_PARAMS failed\n");
            return;
        }
    }

    if ((!IsIntra) && cmdbuf->topaz_in_params_P_p == NULL) {
        VAStatus vaStatus = psb_buffer_map(cmdbuf->topaz_in_params_P, &cmdbuf->topaz_in_params_P_p);
        if (vaStatus != VA_STATUS_SUCCESS) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "map topaz MTX_CURRENT_IN_PARAMS failed\n");
            return;
        }
    }
    if (IsIntra)
        psCurrent = (MTX_CURRENT_IN_PARAMS*)(cmdbuf->topaz_in_params_I_p + ctx->in_params_ofs);
    else
        psCurrent = (MTX_CURRENT_IN_PARAMS*)(cmdbuf->topaz_in_params_P_p + ctx->in_params_ofs);

    psCurrent += (CurrentRowY  * (ctx->Width) / 256);

    if ((YPos = srcY = CurrentRowY - MVEA_LRB_TOP_OFFSET) < 0)
        srcY = 0;
    else if (YPos > ctx->HeightMinusLRB_TopAndBottom_OffsetsPlus16)
        srcY = ctx->HeightMinusLRBSearchHeight;

    tmp = (CurrentRowY != SliceStartRowY);

    for (Pos = 0; Pos < (int)ctx->Width; Pos += 16, psCurrent++) {
        memset(psCurrent, 0, sizeof(MTX_CURRENT_IN_PARAMS));
        psCurrent->MVValid = 0;
        psCurrent->ParamsValid = 0;

        /* Setup the parameters and motion vectors */
        if (tmp) {
            psCurrent->MVValid = 66;
            psCurrent->ParamsValid |= PARAMS_ABOVE_VALID;

            if (Pos + 16 < (int)ctx->Width) {
                psCurrent->ParamsValid |= PARAMS_ABOVER_VALID;
                psCurrent->MVValid |= 4; /* (1<<2) */
            }

            if (Pos > 0 && (Pos < (int)ctx->Width)) {
                psCurrent->ParamsValid |= PARAMS_ABOVEL_VALID;
                psCurrent->MVValid |= 1; /* (1<<0) */
            }
        }
        if ((Pos == 0) && (CurrentRowY == SliceStartRowY)) {
            psCurrent->ParamsValid |= MB_START_OF_SLICE;/* OPTI? */
        }
        /* Have to fill in the right hand row of 4x4 vectors into the the left block */
        if (Pos) {
            psCurrent->MVValid |= 72; /* (1<<3)+(1<<6) */
            psCurrent->ParamsValid |= 8; /* (1<<3) */
        }
        if (Pos == (int)(ctx->Width - 16)) {
            /* indicate the last MB in a row */
            psCurrent->ParamsValid |= MB_END_OF_ROW;
            /* are we the last mb in the slice? */
            if (YPos == (SliceStartRowY + SliceHeight - (MVEA_LRB_TOP_OFFSET + 16))) {
                psCurrent->ParamsValid |= MB_END_OF_SLICE;
                if (YPos == ctx->HeightMinus16MinusLRBTopOffset) {
                    psCurrent->ParamsValid |= MB_END_OF_PICTURE;
                }
            }
        }
        /* And now the below block
         * should do some kind of check to see if we are the first inter block,
         * as otherwise the vectors will be invalid!
         */
        if (VectorsValid) {
            if (YPos < ctx->HeightMinus16MinusLRBTopOffset) {
                psCurrent->MVValid |= 16; /* (1<<4) */

                if (YPos < ctx->HeightMinus32MinusLRBTopOffset) {
                    psCurrent->MVValid |= 32; /* (1<<5) */
                }
            }
        }

        /* Set up IPEMin and Max for coordinate X in the search reference region */
        /* And set up flags in SPEMax when needed */
        if (Pos <= 48) {
            psCurrent->IPEMin[0] = 48 - Pos;
            psCurrent->RealEdge |= SPE_EDGE_LEFT;
        } else {
            psCurrent->IPEMin[0] = 3;
        }

        if ((Pos + 48 + 16) > (int)ctx->Width) {
            psCurrent->IPEMax[0] = (47 + ctx->Width) - Pos; /* (112 - 1) - ((Pos + 48+16) - ctx->Width); */
            psCurrent->RealEdge |= SPE_EDGE_RIGHT;
        } else {
            psCurrent->IPEMax[0] = 108; /* (112 - 1) - 3; */
        }

        /* Set up IPEMin and Max for Y coordinate in the search reference region */
        /* And set up flags in SPEMax when needed */
        if (YPos <= 0) {
            psCurrent->IPEMin[1] = 0;
            psCurrent->RealEdge |= SPE_EDGE_TOP;
        } else {
            psCurrent->IPEMin[1] = 3;
        }

        /* Max Y */
        if (YPos > ctx->HeightMinusLRB_TopAndBottom_OffsetsPlus16) {
            psCurrent->IPEMax[1] = MVEA_LRB_SEARCH_HEIGHT - 1;
            psCurrent->RealEdge |= SPE_EDGE_BOTTOM;
        } else {
            psCurrent->IPEMax[1] = MVEA_LRB_SEARCH_HEIGHT - 4;
        }

        psCurrent->CurBlockAddr = ((IMG_UINT8)(((YPos + MVEA_LRB_TOP_OFFSET) - srcY) / 16) << 4) | 0x3;

        /* Setup the control register values These will get setup and transferred to a different location within
         * the macroblock parameter structure.  They are then read out of the esb by the mtx and used to control
         * the hardware units
         */
        psCurrent->IPEControl = ctx->IPEControl;

        switch (ctx->eCodec) {
        case IMG_CODEC_H263_NO_RC:
        case IMG_CODEC_H263_VBR:
        case IMG_CODEC_H263_CBR:
            lnc__setup_qpvalues_mpeg4(psCurrent, bySliceQP);
            psCurrent->JMCompControl = F_ENCODE(2, MVEA_CR_JMCOMP_MODE);
            psCurrent->VLCControl = F_ENCODE(3, TOPAZ_VLC_CR_CODEC) |
                                    F_ENCODE(IsIntra ? 0 : 1, TOPAZ_VLC_CR_SLICE_CODING_TYPE);
            break;
        case IMG_CODEC_MPEG4_NO_RC:
        case IMG_CODEC_MPEG4_VBR:
        case IMG_CODEC_MPEG4_CBR:
            lnc__setup_qpvalues_mpeg4(psCurrent, bySliceQP);
            psCurrent->JMCompControl = F_ENCODE(1, MVEA_CR_JMCOMP_MODE) |
                                       F_ENCODE(1, MVEA_CR_JMCOMP_AC_ENABLE);
            psCurrent->VLCControl = F_ENCODE(2, TOPAZ_VLC_CR_CODEC) |
                                    F_ENCODE(IsIntra ? 0 : 1, TOPAZ_VLC_CR_SLICE_CODING_TYPE);
            break;
        default:
        case IMG_CODEC_H264_NO_RC:
        case IMG_CODEC_H264_VBR:
        case IMG_CODEC_H264_CBR:
        case IMG_CODEC_H264_VCM:
            lnc__setup_qpvalue_h264(psCurrent, bySliceQP);
            psCurrent->JMCompControl = F_ENCODE(0, MVEA_CR_JMCOMP_MODE);
            psCurrent->VLCControl = F_ENCODE(1, TOPAZ_VLC_CR_CODEC) |
                                    F_ENCODE(IsIntra ? 0 : 1, TOPAZ_VLC_CR_SLICE_CODING_TYPE);
            break;
        }
    }

    psCurrent->RealEdge = 0;

}

void lnc_setup_slice_params(
    context_ENC_p  ctx,
    IMG_UINT16 YSliceStartPos,
    IMG_UINT16 SliceHeight,
    IMG_BOOL IsIntra,
    IMG_BOOL  VectorsValid,
    int bySliceQP)
{
    IMG_UINT16 Rows, CurrentRowY;

    Rows = SliceHeight / 16;
    CurrentRowY = YSliceStartPos;

    while (Rows) {
        lnc__setup_slice_row_params(
            ctx,
            IsIntra,
            CurrentRowY,
            YSliceStartPos,
            SliceHeight,
            VectorsValid, bySliceQP);

        CurrentRowY += 16;
        Rows--;
    }

}



IMG_UINT32 lnc__send_encode_slice_params(
    context_ENC_p ctx,
    IMG_BOOL IsIntra,
    IMG_UINT16 CurrentRow,
    IMG_BOOL DeblockSlice,
    IMG_UINT32 FrameNum,
    IMG_UINT16 SliceHeight,
    IMG_UINT16 CurrentSlice,
    IMG_UINT32 MaxSliceSize)
{
    SLICE_PARAMS *psSliceParams;
    IMG_UINT16 RowOffset;

    psb_buffer_p psCoded;
    object_surface_p ref_surface;
    psb_buffer_p psRef;
    lnc_cmdbuf_p cmdbuf = ctx->obj_context->lnc_cmdbuf;


    ref_surface = ctx->ref_surface;
    psRef = &ctx->ref_surface->psb_surface->buf;
    psCoded = ctx->coded_buf->psb_buffer;

    psSliceParams = (SLICE_PARAMS *)(cmdbuf->slice_params_p +
                                     CurrentSlice * ((sizeof(SLICE_PARAMS) + 15) & 0xfff0));

    psSliceParams->SliceHeight = SliceHeight;
    psSliceParams->SliceStartRowNum = CurrentRow / 16;

    /* We want multiple ones of these so we can submit multiple slices without having to wait for the next */
    psSliceParams->CodedDataPos = 0;
    psSliceParams->TotalCoded = 0;
    psSliceParams->Flags = 0;

#ifdef VA_EMULATOR
    psSliceParams->RefYStride = ref_surface->psb_surface->stride;
    psSliceParams->RefUVStride = ref_surface->psb_surface->stride;
    psSliceParams->RefYRowStride =  ref_surface->psb_surface->stride * 16;
    psSliceParams->RefUVRowStride = ref_surface->psb_surface->stride * 16 / 2;
#else
    psSliceParams->RefYStride = ref_surface->height * 16;
    psSliceParams->RefUVStride = ref_surface->height * 8;
    psSliceParams->RefYRowStride =  psSliceParams->RefYStride;
    psSliceParams->RefUVRowStride = psSliceParams->RefUVStride;
#endif

    psSliceParams->FCode = ctx->FCode;/* Not clear yet, This field is not appare in firmware doc */
    RowOffset = CurrentRow - 32;
    if (RowOffset <= 0)
        RowOffset = 0;
    if (RowOffset > (ctx->Height - 80))
        RowOffset = (ctx->Height - 80);

    psSliceParams->MaxSliceSize = MaxSliceSize;
    psSliceParams->NumAirMBs = ctx->num_air_mbs;
    /* DDKv145: 3 lsb of threshold used as spacing between AIR MBs */
    psSliceParams->AirThreshold = ctx->air_threshold + (FrameNum & 3) + 2;

    if (ctx->autotune_air_flag)
        psSliceParams->Flags |= AUTOTUNE_AIR;

    if (!IsIntra) {
        psSliceParams->Flags |= ISINTER_FLAGS;
    }
    if (DeblockSlice) {
        psSliceParams->Flags |= DEBLOCK_FRAME;
    }
    switch (ctx->eCodec) {
    case IMG_CODEC_H263_NO_RC:
    case IMG_CODEC_H263_VBR:
    case IMG_CODEC_H263_CBR:
        psSliceParams->Flags |= ISH263_FLAGS;
        break;
    case IMG_CODEC_MPEG4_NO_RC:
    case IMG_CODEC_MPEG4_VBR:
    case IMG_CODEC_MPEG4_CBR:
        psSliceParams->Flags |= ISMPEG4_FLAGS;
        break;
    case IMG_CODEC_H264_NO_RC:
    case IMG_CODEC_H264_VBR:
    case IMG_CODEC_H264_CBR:
    case IMG_CODEC_H264_VCM:
        psSliceParams->Flags |= ISH264_FLAGS;
        break;
    default:
        psSliceParams->Flags |= ISH264_FLAGS;
        drv_debug_msg(VIDEO_DEBUG_ERROR, "No format specified defaulting to h.264\n");
        break;
    }
    /* we should also setup the interleaving requirements based on the source format */
    if (ctx->eFormat != IMG_CODEC_PL12)
        psSliceParams->Flags |= INTERLEAVE_TARGET;

    cmdbuf = ctx->obj_context->lnc_cmdbuf;

    RELOC_SLICE_PARAMS(&(psSliceParams->RefYBase), 16 * RowOffset, psRef);
    RELOC_SLICE_PARAMS(&(psSliceParams->RefUVBase),
                       ref_surface->psb_surface->stride * ref_surface->height + (RowOffset * 16 / 2),
                       psRef);
    RELOC_SLICE_PARAMS(&(psSliceParams->CodedData), 0, psCoded);

    lnc_cmdbuf_insert_command(cmdbuf, MTX_CMDID_ENCODE_SLICE, 2, (CurrentSlice << 2) | (IsIntra & 0x3));
    RELOC_CMDBUF(cmdbuf->cmd_idx++,
                 CurrentSlice *((sizeof(SLICE_PARAMS) + 15) & 0xfff0),
                 &cmdbuf->slice_params);

    return 0;
}



/*
 * Function Name      : Reset_EncoderParams
 * Description        : Reset Above & Below Params at the Start of Intra frame
 */
void lnc_reset_encoder_params(context_ENC_p ctx)
{
    unsigned char *Add_Below, *Add_Above;
    lnc_cmdbuf_p cmdbuf = ctx->obj_context->lnc_cmdbuf;

    /* all frames share the same Topaz param, in_param/aboveparam/bellow
     * map it only when necessary
     */
    if (cmdbuf->topaz_above_bellow_params_p == NULL) {
        VAStatus vaStatus = psb_buffer_map(cmdbuf->topaz_above_bellow_params, &cmdbuf->topaz_above_bellow_params_p);
        if (vaStatus != VA_STATUS_SUCCESS) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "map topaz MTX_CURRENT_IN_PARAMS failed\n");
            return;
        }
    }

    Add_Below = cmdbuf->topaz_above_bellow_params_p + ctx->bellow_params_ofs;
    memset(Add_Below, 0, ctx->bellow_params_size);

    Add_Above = cmdbuf->topaz_above_bellow_params_p + ctx->above_params_ofs;
    memset(Add_Above, 0, ctx->above_params_size);
}
