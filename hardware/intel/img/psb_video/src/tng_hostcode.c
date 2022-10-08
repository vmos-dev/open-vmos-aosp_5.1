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
 *    Elaine Wang <elaine.wang@intel.com>
 *    Zeng Li <zeng.li@intel.com>
 *    Edward Lin <edward.lin@intel.com>
 *
 */

#include "psb_drv_video.h"
//#include "tng_H263ES.h"
#include "tng_hostheader.h"
#include "tng_hostcode.h"
#include "psb_def.h"
#include "psb_drv_debug.h"
#include "psb_cmdbuf.h"
#include "psb_buffer.h"
#include <stdio.h>
#include "psb_output.h"
#include "tng_picmgmt.h"
#include "tng_hostbias.h"
#include "tng_hostair.h"
#ifdef _TOPAZHP_PDUMP_
#include "tng_trace.h"
#endif
#include <wsbm/wsbm_manager.h>

#include "hwdefs/topazhp_core_regs.h"
#include "hwdefs/topazhp_multicore_regs_old.h"
#include "hwdefs/topaz_db_regs.h"
#include "hwdefs/topaz_vlc_regs.h"
#include "hwdefs/mvea_regs.h"
#include "hwdefs/topazhp_default_params.h"

#define ALIGN_TO(value, align) ((value + align - 1) & ~(align - 1))
#define PAGE_ALIGN(value) ALIGN_TO(value, 4096)
#define DEFAULT_MVCALC_CONFIG   ((0x00040303)|(MASK_TOPAZHP_CR_MVCALC_JITTER_POINTER_RST))
#define DEFAULT_MVCALC_COLOCATED        (0x00100100)
#define MVEA_MV_PARAM_REGION_SIZE 16
#define MVEA_ABOVE_PARAM_REGION_SIZE 96
#define QUANT_LISTS_SIZE                (224)
#define _1080P_30FPS (((1920*1088)/256)*30)
#define tng_align_64(X)  (((X)+63) &~63)
#define tng_align_4(X)  (((X)+3) &~3)

/* #define MTX_CONTEXT_ITEM_OFFSET(type, member) (size_t)&(((type*)0)->member) */

#define DEFAULT_CABAC_DB_MARGIN    (0x190)
#define NOT_USED_BY_TOPAZ 0
/*
#define _TOPAZHP_CMDBUF_
*/
#ifdef _TOPAZHP_CMDBUF_
static void tng__trace_cmdbuf_words(tng_cmdbuf_p cmdbuf)
{
    int i = 0;
    IMG_UINT32 *ptmp = (IMG_UINT32 *)(cmdbuf->cmd_start);
    IMG_UINT32 *pend = (IMG_UINT32 *)(cmdbuf->cmd_idx);
    do {
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s: command words [%d] = 0x%08x\n", __FUNCTION__, i++, (unsigned int)(*ptmp++));
    } while(ptmp < pend);
    return ;
}
#endif

#if 0
static IMG_UINT32 tng__get_codedbuffer_size(
    IMG_STANDARD eStandard,
    IMG_UINT16 ui16MBWidth,
    IMG_UINT16 ui16MBHeight,
    IMG_RC_PARAMS * psRCParams
)
{
    if (eStandard == IMG_STANDARD_H264) {
        // allocate based on worst case qp size
        return ((IMG_UINT32)ui16MBWidth * (IMG_UINT32)ui16MBHeight * 400);
    }

    if (psRCParams->ui32InitialQp <= 5)
        return ((IMG_UINT32)ui16MBWidth * (IMG_UINT32)ui16MBHeight * 1600);

    return ((IMG_UINT32)ui16MBWidth * (IMG_UINT32)ui16MBHeight * 900);
}


static IMG_UINT32 tng__get_codedbuf_size_according_bitrate(
    IMG_RC_PARAMS * psRCParams
)
{
    return ((psRCParams->ui32BitsPerSecond + psRCParams->ui32FrameRate / 2) / psRCParams->ui32FrameRate) * 2;
}

static IMG_UINT32 tng__get_buffer_size(IMG_UINT32 src_size)
{
    return (src_size + 0x1000) & (~0xfff);
}
#endif

//static inline
VAStatus tng__alloc_init_buffer(
    psb_driver_data_p driver_data,
    unsigned int size,
    psb_buffer_type_t type,
    psb_buffer_p buf)
{
    unsigned char *pch_virt_addr;
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    vaStatus = psb_buffer_create(driver_data, size, type, buf);
    if (VA_STATUS_SUCCESS != vaStatus) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "alloc mem params buffer");
        return vaStatus;
    }

    vaStatus = psb_buffer_map(buf, &pch_virt_addr);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s: phy addr 0x%08x, vir addr 0x%08x\n", __FUNCTION__, buf->drm_buf, pch_virt_addr);
    if ((vaStatus) || (pch_virt_addr == NULL)) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s: map buf 0x%08x\n", __FUNCTION__, (IMG_UINT32)pch_virt_addr);
        psb_buffer_destroy(buf);
    } else {
        memset(pch_virt_addr, 0, size);
        psb_buffer_unmap(buf);
    }

    return vaStatus;
}

static VAStatus tng__alloc_context_buffer(context_ENC_p ctx, IMG_UINT8 ui8IsJpeg, IMG_UINT32 ui32StreamID)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    IMG_UINT32 ui32_pic_width, ui32_pic_height;
    IMG_UINT32 ui32_mb_per_row, ui32_mb_per_column;
    IMG_UINT32 ui32_adj_mb_per_row = 0; 
    IMG_RC_PARAMS *psRCParams = &(ctx->sRCParams);
    psb_driver_data_p ps_driver_data = ctx->obj_context->driver_data;
    context_ENC_mem *ps_mem = &(ctx->ctx_mem[ui32StreamID]);
    context_ENC_mem_size *ps_mem_size = &(ctx->ctx_mem_size);
    
    if (ctx->eStandard == IMG_STANDARD_H264) {
        ctx->ui8PipesToUse = tng__min(ctx->ui8PipesToUse, ctx->ui8SlicesPerPicture);
    } else {
        ctx->ui8PipesToUse = 1;
    }

    ctx->i32PicNodes  = (psRCParams->b16Hierarchical ? MAX_REF_B_LEVELS : 0) + 4;
    ctx->i32MVStores = (ctx->i32PicNodes * 2);
    ctx->i32CodedBuffers = (IMG_INT32)(ctx->ui8PipesToUse) * (ctx->bIsInterlaced ? 3 : 2);
    ctx->ui8SlotsInUse = psRCParams->ui16BFrames + 2;

    if (0 != ui8IsJpeg) {
        ctx->jpeg_pic_params_size = (sizeof(JPEG_MTX_QUANT_TABLE) + 0x3f) & (~0x3f);
        ctx->jpeg_header_mem_size = (sizeof(JPEG_MTX_DMA_SETUP) + 0x3f) & (~0x3f);
        ctx->jpeg_header_interface_mem_size = (sizeof(JPEG_MTX_WRITEBACK_MEMORY) + 0x3f) & (~0x3f);

        //write back region
        ps_mem_size->writeback = tng_align_KB(COMM_WB_DATA_BUF_SIZE);
        tng__alloc_init_buffer(ps_driver_data, WB_FIFO_SIZE * ps_mem_size->writeback, psb_bt_cpu_vpu, &(ctx->bufs_writeback));

        drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s: end\n", __FUNCTION__);
        return vaStatus;
    }

    /* width and height should be source surface's w and h or ?? */
    ui32_pic_width = ctx->obj_context->picture_width;
    ui32_mb_per_row = (ctx->obj_context->picture_width + 15) >> 4;
    ui32_pic_height = ctx->obj_context->picture_height;
    ui32_mb_per_column = (ctx->obj_context->picture_height + 15) >> 4;
    ui32_adj_mb_per_row = ((ui32_mb_per_row + 7)>>3)<<3;  // Ensure multiple of 8 MBs per row

    //command buffer use
    ps_mem_size->pic_template = ps_mem_size->slice_template = 
    ps_mem_size->seq_header = tng_align_KB(TNG_HEADER_SIZE);
    tng__alloc_init_buffer(ps_driver_data, ps_mem_size->seq_header,
        psb_bt_cpu_vpu, &(ps_mem->bufs_seq_header));

    if (ctx->bEnableMVC)
        tng__alloc_init_buffer(ps_driver_data, ps_mem_size->seq_header,
            psb_bt_cpu_vpu, &(ps_mem->bufs_sub_seq_header));

    tng__alloc_init_buffer(ps_driver_data, 4 * ps_mem_size->pic_template,
        psb_bt_cpu_vpu, &(ps_mem->bufs_pic_template));

    tng__alloc_init_buffer(ps_driver_data, NUM_SLICE_TYPES * ps_mem_size->slice_template,
        psb_bt_cpu_vpu, &(ps_mem->bufs_slice_template));

    ps_mem_size->mtx_context = tng_align_KB(MTX_CONTEXT_SIZE);
    tng__alloc_init_buffer(ps_driver_data, ps_mem_size->mtx_context,
        psb_bt_cpu_vpu, &(ps_mem->bufs_mtx_context));

    //sei header(AUDHeader+SEIBufferPeriodMem+SEIPictureTimingHeaderMem)
    ps_mem_size->sei_header = tng_align_KB(64);
    tng__alloc_init_buffer(ps_driver_data, 3 * ps_mem_size->sei_header,
        psb_bt_cpu_vpu, &(ps_mem->bufs_sei_header));

    //gop header
    ps_mem_size->flat_gop = ps_mem_size->hierar_gop = tng_align_KB(64);
    tng__alloc_init_buffer(ps_driver_data, ps_mem_size->flat_gop,
        psb_bt_cpu_vpu, &(ps_mem->bufs_flat_gop));
    tng__alloc_init_buffer(ps_driver_data, ps_mem_size->hierar_gop,
        psb_bt_cpu_vpu, &(ps_mem->bufs_hierar_gop));

    //above params
    ps_mem_size->above_params = tng_align_KB(MVEA_ABOVE_PARAM_REGION_SIZE * tng_align_64(ui32_mb_per_row));
    tng__alloc_init_buffer(ps_driver_data, (IMG_UINT32)(ctx->ui8PipesToUse) * ps_mem_size->above_params,
        psb_bt_cpu_vpu, &(ps_mem->bufs_above_params));

    //ctx->mv_setting_btable_size = tng_align_KB(MAX_BFRAMES * (tng_align_64(sizeof(IMG_MV_SETTINGS) * MAX_BFRAMES)));
    ps_mem_size->mv_setting_btable = tng_align_KB(MAX_BFRAMES * MV_ROW_STRIDE);
    tng__alloc_init_buffer(ps_driver_data, ps_mem_size->mv_setting_btable,
        psb_bt_cpu_vpu, &(ps_mem->bufs_mv_setting_btable));
    
    ps_mem_size->mv_setting_hierar = tng_align_KB(MAX_BFRAMES * sizeof(IMG_MV_SETTINGS));
    tng__alloc_init_buffer(ps_driver_data, ps_mem_size->mv_setting_hierar,
        psb_bt_cpu_vpu, &(ps_mem->bufs_mv_setting_hierar));

    //colocated params
    ps_mem_size->colocated = tng_align_KB(MVEA_MV_PARAM_REGION_SIZE * tng_align_4(ui32_mb_per_row * ui32_mb_per_column));
    tng__alloc_init_buffer(ps_driver_data, ctx->i32PicNodes * ps_mem_size->colocated,
        psb_bt_cpu_vpu, &(ps_mem->bufs_colocated));

    ps_mem_size->interview_mv = ps_mem_size->mv = ps_mem_size->colocated;
    tng__alloc_init_buffer(ps_driver_data, ctx->i32MVStores * ps_mem_size->mv,
        psb_bt_cpu_vpu, &(ps_mem->bufs_mv));

    if (ctx->bEnableMVC) {
        tng__alloc_init_buffer(ps_driver_data, 2 * ps_mem_size->interview_mv,
            psb_bt_cpu_vpu, &(ps_mem->bufs_interview_mv));
    }

    //write back region
    ps_mem_size->writeback = tng_align_KB(COMM_WB_DATA_BUF_SIZE);
    tng__alloc_init_buffer(ps_driver_data, WB_FIFO_SIZE * ps_mem_size->writeback,
        psb_bt_cpu_vpu, &(ctx->bufs_writeback));

    ps_mem_size->slice_map = tng_align_KB(0x1500); //(1 + MAX_SLICESPERPIC * 2 + 15) & ~15);
    tng__alloc_init_buffer(ps_driver_data, ctx->ui8SlotsInUse * ps_mem_size->slice_map,
        psb_bt_cpu_vpu, &(ps_mem->bufs_slice_map));

    ps_mem_size->weighted_prediction = tng_align_KB(sizeof(WEIGHTED_PREDICTION_VALUES));
    tng__alloc_init_buffer(ps_driver_data, ctx->ui8SlotsInUse * ps_mem_size->weighted_prediction,
        psb_bt_cpu_vpu, &(ps_mem->bufs_weighted_prediction));

#ifdef LTREFHEADER
    ps_mem_size->lt_ref_header = tng_align_KB((sizeof(MTX_HEADER_PARAMS)+63)&~63);
    tng__alloc_init_buffer(ps_driver_data, ctx->ui8SlotsInUse * ps_mem_size->lt_ref_header,
        psb_bt_cpu_vpu, &(ps_mem->bufs_lt_ref_header));
#endif

    ps_mem_size->recon_pictures = tng_align_KB((tng_align_64(ui32_pic_width)*tng_align_64(ui32_pic_height))*3/2);
    tng__alloc_init_buffer(ps_driver_data, ctx->i32PicNodes * ps_mem_size->recon_pictures,
        psb_bt_cpu_vpu, &(ps_mem->bufs_recon_pictures));
    
    ctx->ctx_mem_size.first_pass_out_params = tng_align_KB(sizeof(IMG_FIRST_STAGE_MB_PARAMS) * ui32_mb_per_row *  ui32_mb_per_column); 
    tng__alloc_init_buffer(ps_driver_data, ctx->ui8SlotsInUse * ctx->ctx_mem_size.first_pass_out_params,
        psb_bt_cpu_vpu, &(ps_mem->bufs_first_pass_out_params));

#ifndef EXCLUDE_BEST_MP_DECISION_DATA
    ctx->ctx_mem_size.first_pass_out_best_multipass_param = tng_align_KB(ui32_mb_per_column * (((5*ui32_mb_per_row)+3)>>2) * 64); 
    tng__alloc_init_buffer(ps_driver_data, ctx->ui8SlotsInUse * ctx->ctx_mem_size.first_pass_out_best_multipass_param,
        psb_bt_cpu_vpu, &(ps_mem->bufs_first_pass_out_best_multipass_param));
#endif

    ctx->ctx_mem_size.mb_ctrl_in_params = tng_align_KB(sizeof(IMG_FIRST_STAGE_MB_PARAMS) * ui32_adj_mb_per_row *  ui32_mb_per_column); 
    tng__alloc_init_buffer(ps_driver_data, ctx->ui8SlotsInUse * ctx->ctx_mem_size.mb_ctrl_in_params,
        psb_bt_cpu_vpu, &(ps_mem->bufs_mb_ctrl_in_params));

    ctx->ctx_mem_size.lowpower_params = tng_align_KB(TNG_HEADER_SIZE);
    tng__alloc_init_buffer(ps_driver_data, ps_mem_size->lowpower_params,
        psb_bt_cpu_vpu, &(ps_mem->bufs_lowpower_params));

    ctx->ctx_mem_size.lowpower_data = tng_align_KB(0x10000);

    return vaStatus;
}

static void tng__free_context_buffer(context_ENC_p ctx, unsigned char is_JPEG, unsigned int stream_id)
{
    context_ENC_mem *ps_mem = &(ctx->ctx_mem[stream_id]);

    if (0 != is_JPEG) {
        psb_buffer_destroy(&(ctx->bufs_writeback));
        return;
    }
    psb_buffer_destroy(&(ps_mem->bufs_seq_header));
    if (ctx->bEnableMVC)
        psb_buffer_destroy(&(ps_mem->bufs_sub_seq_header));
    psb_buffer_destroy(&(ps_mem->bufs_pic_template));
    psb_buffer_destroy(&(ps_mem->bufs_slice_template));
    psb_buffer_destroy(&(ps_mem->bufs_mtx_context));
    psb_buffer_destroy(&(ps_mem->bufs_sei_header));

    psb_buffer_destroy(&(ps_mem->bufs_flat_gop));
    psb_buffer_destroy(&(ps_mem->bufs_hierar_gop));
    psb_buffer_destroy(&(ps_mem->bufs_above_params));
    psb_buffer_destroy(&(ps_mem->bufs_mv_setting_btable));
    psb_buffer_destroy(&(ps_mem->bufs_mv_setting_hierar));
    psb_buffer_destroy(&(ps_mem->bufs_colocated));
    psb_buffer_destroy(&(ps_mem->bufs_mv));
    if (ctx->bEnableMVC)
        psb_buffer_destroy(&(ps_mem->bufs_interview_mv));

    psb_buffer_destroy(&(ctx->bufs_writeback));
    psb_buffer_destroy(&(ps_mem->bufs_slice_map));
    psb_buffer_destroy(&(ps_mem->bufs_weighted_prediction));
#ifdef LTREFHEADER
    psb_buffer_destroy(&(ps_mem->bufs_lt_ref_header));
#endif
    psb_buffer_destroy(&(ps_mem->bufs_recon_pictures));
    psb_buffer_destroy(&(ps_mem->bufs_first_pass_out_params));
#ifndef EXCLUDE_BEST_MP_DECISION_DATA
    psb_buffer_destroy(&(ps_mem->bufs_first_pass_out_best_multipass_param));
#endif
    psb_buffer_destroy(&(ps_mem->bufs_mb_ctrl_in_params));
    psb_buffer_destroy(&(ps_mem->bufs_lowpower_params));

    return ;
}

unsigned int tng__get_ipe_control(IMG_CODEC  eEncodingFormat)
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

void tng__setup_enc_profile_features(context_ENC_p ctx, IMG_UINT32 ui32EncProfile)
{
    IMG_ENCODE_FEATURES * pEncFeatures = &ctx->sEncFeatures;
    /* Set the default values first */
    pEncFeatures->bDisableBPicRef_0 = IMG_FALSE;
    pEncFeatures->bDisableBPicRef_1 = IMG_FALSE;

    pEncFeatures->bDisableInter8x8 = IMG_FALSE;
    pEncFeatures->bRestrictInter4x4 = IMG_FALSE;

    pEncFeatures->bDisableIntra4x4  = IMG_FALSE;
    pEncFeatures->bDisableIntra8x8  = IMG_FALSE;
    pEncFeatures->bDisableIntra16x16 = IMG_FALSE;

    pEncFeatures->bEnable8x16MVDetect   = IMG_TRUE;
    pEncFeatures->bEnable16x8MVDetect   = IMG_TRUE;
    pEncFeatures->bDisableBFrames       = IMG_FALSE;

    pEncFeatures->eMinBlkSz = BLK_SZ_DEFAULT;

    switch (ui32EncProfile) {
        case ENC_PROFILE_LOWCOMPLEXITY:
            pEncFeatures->bDisableInter8x8  = IMG_TRUE;
            pEncFeatures->bRestrictInter4x4 = IMG_TRUE;
            pEncFeatures->bDisableIntra4x4  = IMG_TRUE;
            pEncFeatures->bDisableIntra8x8  = IMG_TRUE;
            pEncFeatures->bRestrictInter4x4 = IMG_TRUE;
            pEncFeatures->eMinBlkSz         = BLK_SZ_16x16;
            pEncFeatures->bDisableBFrames   = IMG_TRUE;
            break;

        case ENC_PROFILE_HIGHCOMPLEXITY:
            pEncFeatures->bDisableBPicRef_0 = IMG_FALSE;
            pEncFeatures->bDisableBPicRef_1 = IMG_FALSE;

            pEncFeatures->bDisableInter8x8 = IMG_FALSE;
            pEncFeatures->bRestrictInter4x4 = IMG_FALSE;

            pEncFeatures->bDisableIntra4x4  = IMG_FALSE;
            pEncFeatures->bDisableIntra8x8  = IMG_FALSE;
            pEncFeatures->bDisableIntra16x16 = IMG_FALSE;

            pEncFeatures->bEnable8x16MVDetect   = IMG_TRUE;
            pEncFeatures->bEnable16x8MVDetect   = IMG_TRUE;
            break;
    }

    if (ctx->eStandard != IMG_STANDARD_H264) {
	pEncFeatures->bEnable8x16MVDetect = IMG_FALSE;
	pEncFeatures->bEnable16x8MVDetect = IMG_FALSE;
    }

    return;
}

VAStatus tng__patch_hw_profile(context_ENC_p ctx)
{
    IMG_UINT32 ui32IPEControl = 0;
    IMG_UINT32 ui32PredCombControl = 0;
    IMG_ENCODE_FEATURES * psEncFeatures = &(ctx->sEncFeatures);

    // bDisableIntra4x4
    if (psEncFeatures->bDisableIntra4x4)
        ui32PredCombControl |= F_ENCODE(1, TOPAZHP_CR_INTRA4X4_DISABLE);

    //bDisableIntra8x8
    if (psEncFeatures->bDisableIntra8x8)
        ui32PredCombControl |= F_ENCODE(1, TOPAZHP_CR_INTRA8X8_DISABLE);

    //bDisableIntra16x16, check if atleast one of the other Intra mode is enabled
    if ((psEncFeatures->bDisableIntra16x16) &&
        (!(psEncFeatures->bDisableIntra8x8) || !(psEncFeatures->bDisableIntra4x4))) {
        ui32PredCombControl |= F_ENCODE(1, TOPAZHP_CR_INTRA16X16_DISABLE);
    }

    if (psEncFeatures->bRestrictInter4x4) {
//        ui32PredCombControl |= F_ENCODE(1, TOPAZHP_CR_INTER4X4_RESTRICT);
        ui32IPEControl |= F_ENCODE(1, TOPAZHP_CR_IPE_MV_NUMBER_RESTRICTION);
    }

    if (psEncFeatures->bDisableInter8x8)
        ui32PredCombControl |= F_ENCODE(1, TOPAZHP_CR_INTER8X8_DISABLE);

    if (psEncFeatures->bDisableBPicRef_1)
        ui32PredCombControl |= F_ENCODE(1, TOPAZHP_CR_B_PIC1_DISABLE);
    else if (psEncFeatures->bDisableBPicRef_0)
        ui32PredCombControl |= F_ENCODE(1, TOPAZHP_CR_B_PIC0_DISABLE);

    // save predictor combiner control in video encode parameter set
    ctx->ui32PredCombControl = ui32PredCombControl;

    // set blocksize
    ui32IPEControl |= F_ENCODE(psEncFeatures->eMinBlkSz, TOPAZHP_CR_IPE_BLOCKSIZE);

    if (psEncFeatures->bEnable8x16MVDetect)
        ui32IPEControl |= F_ENCODE(1, TOPAZHP_CR_IPE_8X16_ENABLE);

    if (psEncFeatures->bEnable16x8MVDetect)
        ui32IPEControl |= F_ENCODE(1, TOPAZHP_CR_IPE_16X8_ENABLE);

    if (psEncFeatures->bDisableBFrames)
        ctx->sRCParams.ui16BFrames = 0;

    //save IPE-control register
    ctx->ui32IPEControl = ui32IPEControl;

    return VA_STATUS_SUCCESS;
}

#ifdef _TOPAZHP_CMDBUF_
static void tng__trace_cmdbuf(tng_cmdbuf_p cmdbuf, int idx)
{
    IMG_UINT32 ui32CmdTmp[4];
    IMG_UINT32 *ptmp = (IMG_UINT32 *)(cmdbuf->cmd_start);
    IMG_UINT32 *pend = (IMG_UINT32 *)(cmdbuf->cmd_idx);
    IMG_UINT32 ui32Len;

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s: start, stream (%d), ptmp (0x%08x), pend (0x%08x}\n", __FUNCTION__, idx, (unsigned int)ptmp, (unsigned int)pend);

    if (idx)
        return ;

    while (ptmp < pend) {
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s: ptmp (0x%08x}\n", __FUNCTION__, *ptmp);
        if ((*ptmp & 0x7f) == MTX_CMDID_SW_NEW_CODEC) {
            ptmp += 4;
        } else if ((*ptmp & 0x7f) == MTX_CMDID_SW_LEAVE_LOWPOWER) {
            ptmp += 2;
        } else if ((*ptmp & 0x7f) == MTX_CMDID_SW_WRITEREG) {
            ui32Len = *(++ptmp);
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s: len = %d\n", __FUNCTION__, ui32Len);
            ptmp += (ui32Len * 3) + 1;
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s: reg ptmp (0x%08x}\n", __FUNCTION__, *ptmp);
        } else if ((*ptmp & 0x7f) == MTX_CMDID_DO_HEADER) {
            ui32CmdTmp[0] = *ptmp++;
            ui32CmdTmp[1] = *ptmp++;
            ui32CmdTmp[2] = *ptmp++;
            ui32CmdTmp[3] = 0;
            //topazhp_dump_command((unsigned int*)ui32CmdTmp);
            ptmp += 2;
        } else if (
            ((*ptmp & 0x7f) == MTX_CMDID_SETVIDEO)||
            ((*ptmp & 0x7f) == MTX_CMDID_SHUTDOWN)) {
            ui32CmdTmp[0] = *ptmp++;
            ui32CmdTmp[1] = *ptmp++;
            ui32CmdTmp[2] = *ptmp++;
            ui32CmdTmp[3] = *ptmp++;
            //topazhp_dump_command((unsigned int*)ui32CmdTmp);
        } else if (
            ((*ptmp & 0x7f) == MTX_CMDID_PROVIDE_SOURCE_BUFFER) ||
            ((*ptmp & 0x7f) == MTX_CMDID_PROVIDE_REF_BUFFER) ||
            ((*ptmp & 0x7f) == MTX_CMDID_PROVIDE_CODED_BUFFER) ||
            ((*ptmp & 0x7f) == MTX_CMDID_PICMGMT) ||
            ((*ptmp & 0x7f) == MTX_CMDID_ENCODE_FRAME)) {
            ui32CmdTmp[0] = *ptmp++;
            ui32CmdTmp[1] = *ptmp++;
            ui32CmdTmp[2] = *ptmp++;
            ui32CmdTmp[3] = 0;
            //topazhp_dump_command((unsigned int*)ui32CmdTmp);
        } else {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "%s: error leave lowpower = 0x%08x\n", __FUNCTION__, *ptmp++);            
        }
    }

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s: end\n", __FUNCTION__);

    return ;
}
#endif

void tng_DestroyContext(object_context_p obj_context, unsigned char is_JPEG)
{
    context_ENC_p ctx;
//    tng_cmdbuf_p cmdbuf = ctx->obj_context->tng_cmdbuf;
    ctx = (context_ENC_p)obj_context->format_data;
    FRAME_ORDER_INFO *psFrameInfo = &(ctx->sFrameOrderInfo);

    if (psFrameInfo->slot_consume_dpy_order != NULL)
        free(psFrameInfo->slot_consume_dpy_order);
    if (psFrameInfo->slot_consume_enc_order != NULL)
        free(psFrameInfo->slot_consume_enc_order);

    tng_air_buf_free(ctx);

    tng__free_context_buffer(ctx, is_JPEG, 0);

    if (ctx->bEnableMVC)
        tng__free_context_buffer(ctx, is_JPEG, 1);

    free(obj_context->format_data);
    obj_context->format_data = NULL;
}

static VAStatus tng__init_rc_params(context_ENC_p ctx, object_config_p obj_config)
{
    IMG_RC_PARAMS *psRCParams = &(ctx->sRCParams);
    unsigned int eRCmode = 0;
    memset(psRCParams, 0, sizeof(IMG_RC_PARAMS));
    IMG_INT32 i;

    //set RC mode
    for (i = 0; i < obj_config->attrib_count; i++) {
        if (obj_config->attrib_list[i].type == VAConfigAttribRateControl)
            break;
    }

    if (i >= obj_config->attrib_count) {
        eRCmode = VA_RC_NONE;
    } else {
        eRCmode = obj_config->attrib_list[i].value;
    }

    ctx->sRCParams.bRCEnable = IMG_TRUE;
    ctx->sRCParams.bDisableBitStuffing = IMG_FALSE;

    if (eRCmode == VA_RC_NONE) {
        ctx->sRCParams.bRCEnable = IMG_FALSE;
        ctx->sRCParams.eRCMode = IMG_RCMODE_NONE;
    } else if (eRCmode == VA_RC_CBR) {
        ctx->sRCParams.eRCMode = IMG_RCMODE_CBR;
    } else if (eRCmode == VA_RC_VBR) {
        ctx->sRCParams.eRCMode = IMG_RCMODE_VBR;
    } else if (eRCmode == VA_RC_VCM) {
        ctx->sRCParams.eRCMode = IMG_RCMODE_VCM;
    } else {
        ctx->sRCParams.bRCEnable = IMG_FALSE;
        drv_debug_msg(VIDEO_DEBUG_ERROR, "not support this RT Format\n");
        return VA_STATUS_ERROR_UNSUPPORTED_RT_FORMAT;
    }

    psRCParams->bScDetectDisable = IMG_FALSE;
    psRCParams->ui32SliceByteLimit = 0;
    psRCParams->ui32SliceMBLimit = 0;
    psRCParams->bIsH264Codec = (ctx->eStandard == IMG_STANDARD_H264) ? IMG_TRUE : IMG_FALSE;
    return VA_STATUS_SUCCESS;
}

/**************************************************************************************************
* Function:             IMG_C_GetEncoderCaps
* Description:  Get the capabilities of the encoder for the given codec
*
***************************************************************************************************/
//FIXME
static const IMG_UINT32 g_ui32PipesAvailable = TOPAZHP_PIPE_NUM;
static const IMG_UINT32 g_ui32CoreDes1 = TOPAZHP_PIPE_NUM;
static const IMG_UINT32 g_ui32CoreRev = 0x00030401;

static IMG_UINT32 tng__get_num_pipes()
{
    return g_ui32PipesAvailable;
}

static IMG_UINT32 tng__get_core_des1()
{
    return g_ui32CoreDes1;
}

static IMG_UINT32 tng__get_core_rev()
{
    return g_ui32CoreRev;
}

static VAStatus tng__get_encoder_caps(context_ENC_p ctx)
{
    IMG_ENC_CAPS *psCaps = &(ctx->sCapsParams);
    IMG_UINT16 ui16Height = ctx->ui16FrameHeight;
    IMG_UINT32 ui32NumCores = 0;
    IMG_UINT16 ui16MBRows = 0; //MB Rows in a GOB(slice);

    ctx->ui32CoreRev = tng__get_core_rev();
    psCaps->ui32CoreFeatures = tng__get_core_des1();

    /* get the actual number of cores */
    ui32NumCores = tng__get_num_pipes();

    switch (ctx->eStandard) {
        case IMG_STANDARD_JPEG:
            psCaps->ui16MaxSlices = ui16Height / 8;
            psCaps->ui16MinSlices = 1;
            psCaps->ui16RecommendedSlices = ui32NumCores;
            break;
        case IMG_STANDARD_H264:
            psCaps->ui16MaxSlices = ui16Height / 16;
            psCaps->ui16MinSlices = 1;
            psCaps->ui16RecommendedSlices = ui32NumCores;
            break;
        case IMG_STANDARD_MPEG2:
            psCaps->ui16MaxSlices = 174; // Slice labelling dictates a maximum of 174 slices
            psCaps->ui16MinSlices = 1;
            psCaps->ui16RecommendedSlices = (ui16Height + 15) / 16;
            break;
        case IMG_STANDARD_H263:
            // if the original height of the pic is less than 400 , k is 1. refer standard.
            if (ui16Height <= 400) {
                ui16MBRows = 1;
            } else if (ui16Height < 800) { // if between 400 and 800 it's 2.            
                ui16MBRows = 2;
            } else {
                ui16MBRows = 4;
            }
            // before rounding is done for the height.
            // get the GOB's based on this MB Rows and not vice-versa.
            psCaps->ui16RecommendedSlices = (ui16Height + 15) >> (4 + (ui16MBRows >> 1));
            psCaps->ui16MaxSlices = psCaps->ui16MinSlices = psCaps->ui16RecommendedSlices;
            break;
        case IMG_STANDARD_MPEG4:
            psCaps->ui16MaxSlices = 1;
            psCaps->ui16MinSlices = 1;
            psCaps->ui16RecommendedSlices = 1;
            break;
        default:
            break;
    }
    return VA_STATUS_SUCCESS;
}

static VAStatus tng__init_context(context_ENC_p ctx)
{
    VAStatus vaStatus = 0;

    /* Mostly sure about the following video parameters */
    //ctx->ui32HWProfile = pParams->ui32HWProfile;
    ctx->ui32FrameCount[0] = ctx->ui32FrameCount[1] = 0;
    /* Using Extended parameters */
    ctx->ui8PipesToUse = (IMG_UINT8)(tng__get_num_pipes() & (IMG_UINT32)0xff);
    //carc params
    ctx->sCARCParams.bCARC             = 0;
    ctx->sCARCParams.i32CARCBaseline   = 0;
    ctx->sCARCParams.ui32CARCThreshold = TOPAZHP_DEFAULT_uCARCThreshold;
    ctx->sCARCParams.ui32CARCCutoff    = TOPAZHP_DEFAULT_uCARCCutoff;
    ctx->sCARCParams.ui32CARCNegRange  = TOPAZHP_DEFAULT_uCARCNegRange;
    ctx->sCARCParams.ui32CARCNegScale  = TOPAZHP_DEFAULT_uCARCNegScale;
    ctx->sCARCParams.ui32CARCPosRange  = TOPAZHP_DEFAULT_uCARCPosRange;
    ctx->sCARCParams.ui32CARCPosScale  = TOPAZHP_DEFAULT_uCARCPosScale;
    ctx->sCARCParams.ui32CARCShift     = TOPAZHP_DEFAULT_uCARCShift;

    ctx->bUseDefaultScalingList = IMG_FALSE;
    ctx->ui32CabacBinLimit = TOPAZHP_DEFAULT_uCABACBinLimit; //This parameter need not be exposed
    if (ctx->ui32CabacBinLimit == 0)
        ctx->ui32CabacBinFlex = 0;//This parameter need not be exposed
    else
        ctx->ui32CabacBinFlex = TOPAZHP_DEFAULT_uCABACBinFlex;//This parameter need not be exposed

    ctx->ui32FCode = 4;                     //This parameter need not be exposed
    ctx->iFineYSearchSize = 2;//This parameter need not be exposed
    ctx->ui32VopTimeResolution = 15;//This parameter need not be exposed
//    ctx->bEnabledDynamicBPic = IMG_FALSE;//Related to Rate Control,which is no longer needed.
    ctx->bH264IntraConstrained = IMG_FALSE;//This parameter need not be exposed
    ctx->bEnableInpCtrl     = IMG_FALSE;//This parameter need not be exposed
    ctx->bEnableAIR = 0;
    ctx->bEnableCIR = 0;
    ctx->bEnableHostBias = (ctx->bEnableAIR != 0);//This parameter need not be exposed
    ctx->bEnableHostQP = IMG_FALSE; //This parameter need not be exposed
    ctx->ui8CodedSkippedIndex = 3;//This parameter need not be exposed
    ctx->ui8InterIntraIndex         = 3;//This parameter need not be exposed
    ctx->uMaxChunks = 0xA0;//This parameter need not be exposed
    ctx->uChunksPerMb = 0x40;//This parameter need not be exposed
    ctx->uPriorityChunks = (0xA0 - 0x60);//This parameter need not be exposed
    ctx->bWeightedPrediction = IMG_FALSE;//Weighted Prediction is not supported in TopazHP Version 3.0
    ctx->ui8VPWeightedImplicitBiPred = 0;//Weighted Prediction is not supported in TopazHP Version 3.0
    ctx->bSkipDuplicateVectors = IMG_FALSE;//By default false Newly Added
    ctx->bEnableCumulativeBiases = IMG_FALSE;//By default false Newly Added
    ctx->ui16UseCustomScalingLists = 0;//By default false Newly Added
    ctx->bPpsScaling = IMG_FALSE;//By default false Newly Added
    ctx->ui8MPEG2IntraDCPrecision = 0;//By default 0 Newly Added
    ctx->uMBspS = 0;//By default 0 Newly Added
    ctx->bMultiReferenceP = IMG_FALSE;//By default false Newly Added
    ctx->ui8RefSpacing=0;//By default 0 Newly Added
    ctx->bSpatialDirect = 0;//By default 0 Newly Added
    ctx->ui32DebugCRCs = 0;//By default false Newly Added
    ctx->bEnableMVC = IMG_FALSE;//By default false Newly Added
    ctx->ui16MVCViewIdx = (IMG_UINT16)(NON_MVC_VIEW);//Newly Added
    ctx->bSrcAllocInternally = IMG_FALSE;//By default false Newly Added
    ctx->bCodedAllocInternally = IMG_FALSE;//By default true Newly Added
    ctx->bHighLatency = IMG_TRUE;//Newly Added
    ctx->i32NumAIRMBs = -1;
    ctx->i32AIRThreshold = -1;
    ctx->i16AIRSkipCnt = -1;
    ctx->i32LastCIRIndex = -1;
    //Need to check on the following parameters
    ctx->ui8EnableSelStatsFlags  = IMG_FALSE;//Default Value ?? Extended Parameter ??
    ctx->bH2648x8Transform = IMG_FALSE;//Default Value ?? Extended Parameter or OMX_VIDEO_PARAM_AVCTYPE -> bDirect8x8Inference??
    //FIXME: Zhaohan, eStandard is always 0 here.
    ctx->bNoOffscreenMv = (ctx->eStandard == IMG_STANDARD_H263) ? IMG_TRUE : IMG_FALSE; //Default Value ?? Extended Parameter and bUseOffScreenMVUserSetting
    ctx->bNoSequenceHeaders = IMG_FALSE;
    ctx->bTopFieldFirst = IMG_TRUE;
    ctx->sBiasTables.ui32FCode = ctx->ui32FCode;
    ctx->ui32pseudo_rand_seed = UNINIT_PARAM;
    ctx->bVPAdaptiveRoundingDisable = IMG_TRUE;

    //Default fcode is 4
    if (!ctx->sBiasTables.ui32FCode)
	ctx->sBiasTables.ui32FCode = 4;

    ctx->uiCbrBufferTenths = TOPAZHP_DEFAULT_uiCbrBufferTenths;

    tng__setup_enc_profile_features(ctx, ENC_PROFILE_DEFAULT);

    vaStatus = tng__patch_hw_profile(ctx);
    if (vaStatus != VA_STATUS_SUCCESS) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s: tng__patch_hw_profile\n", __FUNCTION__);
    }

    return VA_STATUS_SUCCESS;
}

VAStatus tng_CreateContext(
    object_context_p obj_context,
    object_config_p obj_config,
    unsigned char is_JPEG)
{
    VAStatus vaStatus = 0;
    unsigned short ui16Width, ui16Height;
    context_ENC_p ctx;

    ui16Width = obj_context->picture_width;
    ui16Height = obj_context->picture_height;
    ctx = (context_ENC_p) calloc(1, sizeof(struct context_ENC_s));
    if (NULL == ctx) {
        vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
        DEBUG_FAILURE;
        return vaStatus;
    }

    memset((void*)ctx, 0, sizeof(struct context_ENC_s));

    obj_context->format_data = (void*) ctx;
    ctx->obj_context = obj_context;

    if (is_JPEG == 0) {
        ctx->ui16Width = (unsigned short)(~0xf & (ui16Width + 0xf));
        ctx->ui16FrameHeight = (unsigned short)(~0xf & (ui16Height + 0xf));

        vaStatus = tng__init_context(ctx);
        if (vaStatus != VA_STATUS_SUCCESS) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "init Context params");
        } 

        vaStatus = tng__init_rc_params(ctx, obj_config);
        if (vaStatus != VA_STATUS_SUCCESS) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "init rc params");
        } 
    } else {
        /*JPEG only require them are even*/
        ctx->ui16Width = (unsigned short)(~0x1 & (ui16Width + 0x1));
        ctx->ui16FrameHeight = (unsigned short)(~0x1 & (ui16Height + 0x1));
    }

    ctx->eFormat = IMG_CODEC_PL12;     // use default

    tng__setup_enc_profile_features(ctx, ENC_PROFILE_DEFAULT);

    if (is_JPEG) {
        vaStatus = tng__alloc_context_buffer(ctx, is_JPEG, 0);
        if (vaStatus != VA_STATUS_SUCCESS) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "setup enc profile");
        }
    }

    return vaStatus;
}

VAStatus tng_BeginPicture(context_ENC_p ctx)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    context_ENC_frame_buf *ps_buf = &(ctx->ctx_frame_buf);
    tng_cmdbuf_p cmdbuf;
    int ret;

    ctx->ui32StreamID = 0;

    if (ctx->ui32RawFrameCount != 0)
        ps_buf->previous_src_surface = ps_buf->src_surface;
    ps_buf->src_surface = ctx->obj_context->current_render_target;

    vaStatus = tng__get_encoder_caps(ctx);
    if (vaStatus != VA_STATUS_SUCCESS) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s: tng__get_encoder_caps\n", __FUNCTION__);
    }

    /* clear frameskip flag to 0 */
    CLEAR_SURFACE_INFO_skipped_flag(ps_buf->src_surface->psb_surface);

    /* Initialise the command buffer */
    ret = tng_context_get_next_cmdbuf(ctx->obj_context);
    if (ret) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "get next cmdbuf fail\n");
        vaStatus = VA_STATUS_ERROR_UNKNOWN;
        return vaStatus;
    }
    cmdbuf = ctx->obj_context->tng_cmdbuf;

    //FIXME
    /* only map topaz param when necessary */
    cmdbuf->topaz_in_params_I_p = NULL;
    cmdbuf->topaz_in_params_P_p = NULL;
    ctx->obj_context->slice_count = 0;

    tng_cmdbuf_buffer_ref(cmdbuf, &(ctx->obj_context->current_render_target->psb_surface->buf));
    
    return vaStatus;
}

static VAStatus tng__provide_buffer_BFrames(context_ENC_p ctx, IMG_UINT32 ui32StreamIndex)
{
    IMG_RC_PARAMS * psRCParams = &(ctx->sRCParams);
    FRAME_ORDER_INFO *psFrameInfo = &(ctx->sFrameOrderInfo);
    int slot_index = 0;
    unsigned long long display_order = 0;
    IMG_INT32  i32SlotBuf  = (IMG_INT32)(psRCParams->ui16BFrames + 2);
    IMG_UINT32 ui32SlotBuf = (IMG_UINT32)(psRCParams->ui16BFrames + 2);
    IMG_UINT32 ui32FrameIdx = ctx->ui32FrameCount[ui32StreamIndex];

    if (ui32StreamIndex == 0)
        getFrameDpyOrder(ui32FrameIdx, psRCParams->ui16BFrames, ctx->ui32IntraCnt,
             ctx->ui32IdrPeriod, psFrameInfo, &display_order);

    slot_index = psFrameInfo->last_slot;

    drv_debug_msg(VIDEO_DEBUG_GENERAL,
        "%s: (int)ui32FrameIdx = %d, psRCParams->ui16BFrames = %d, psRCParams->ui32IntraFreq = %d, ctx->ui32IdrPeriod = %d\n",
        __FUNCTION__, (int)ui32FrameIdx, (int)psRCParams->ui16BFrames, (int)psRCParams->ui32IntraFreq, ctx->ui32IdrPeriod);

    drv_debug_msg(VIDEO_DEBUG_GENERAL,
        "%s: last_slot = %d, last_frame_type = %d, display_order = %d\n",
        __FUNCTION__, psFrameInfo->last_slot, psFrameInfo->last_frame_type, display_order);

    if (ui32FrameIdx < ui32SlotBuf) {
        if (ui32FrameIdx == 0) {
            tng_send_source_frame(ctx, 0, 0);
        } else if (ui32FrameIdx == 1) {
            slot_index = 1;
            do {
                tng_send_source_frame(ctx, slot_index, slot_index);
                ++slot_index;
            } while(slot_index < i32SlotBuf);
        } else {
            slot_index = ui32FrameIdx - 1;
            tng_send_source_frame(ctx, slot_index, slot_index);
        }
    } else {
        tng_send_source_frame(ctx, slot_index , display_order);
    }

    return VA_STATUS_SUCCESS;
}

VAStatus tng__provide_buffer_PFrames(context_ENC_p ctx, IMG_UINT32 ui32StreamIndex)
{
    IMG_UINT32 ui32FrameIdx = ctx->ui32FrameCount[ui32StreamIndex];

    drv_debug_msg(VIDEO_DEBUG_GENERAL,
        "%s: frame count = %d, SlotsInUse = %d, ui32FrameIdx = %d\n",
        __FUNCTION__, (int)ui32FrameIdx, ctx->ui8SlotsInUse, ui32FrameIdx);

    tng_send_source_frame(ctx, ctx->ui8SlotsCoded, ui32FrameIdx);
/*
    if (ctx->ui32IntraCnt == 0)
        ctx->eFrameType = IMG_INTRA_FRAME;
    else
        if (ui32FrameIdx % ctx->ui32IntraCnt == 0)
            ctx->eFrameType = IMG_INTRA_FRAME;
        else
            ctx->eFrameType = IMG_INTER_P;

    if (ctx->ui32IdrPeriod == 0) {
        if (ui32FrameIdx == 0)
            ctx->eFrameType = IMG_INTRA_IDR;
    } else {
        if (ctx->ui32IntraCnt == 0) {
            if (ui32FrameIdx % ctx->ui32IdrPeriod == 0)
                ctx->eFrameType = IMG_INTRA_IDR;
        } else {
            if (ui32FrameIdx % (ctx->ui32IntraCnt * ctx->ui32IdrPeriod) == 0)
                ctx->eFrameType = IMG_INTRA_IDR;
        }
    }
*/
    ctx->eFrameType = IMG_INTER_P;

    if (ui32FrameIdx % ctx->ui32IntraCnt == 0)
        ctx->eFrameType = IMG_INTRA_FRAME;

    if (ui32FrameIdx % (ctx->ui32IdrPeriod * ctx->ui32IntraCnt) == 0)
        ctx->eFrameType = IMG_INTRA_IDR;

    drv_debug_msg(VIDEO_DEBUG_GENERAL,"%s: ctx->eFrameType = %d\n", __FUNCTION__, ctx->eFrameType);

    return VA_STATUS_SUCCESS;
}

static IMG_UINT16 H264_ROUNDING_OFFSETS[18][4] = {
    {  683, 683, 683, 683 },   /* 0  I-Slice - INTRA4  LUMA */
    {  683, 683, 683, 683 },   /* 1  P-Slice - INTRA4  LUMA */
    {  683, 683, 683, 683 },   /* 2  B-Slice - INTRA4  LUMA */

    {  683, 683, 683, 683 },   /* 3  I-Slice - INTRA8  LUMA */
    {  683, 683, 683, 683 },   /* 4  P-Slice - INTRA8  LUMA */
    {  683, 683, 683, 683 },   /* 5  B-Slice - INTRA8  LUMA */
    {  341, 341, 341, 341 },   /* 6  P-Slice - INTER8  LUMA */
    {  341, 341, 341, 341 },   /* 7  B-Slice - INTER8  LUMA */

    {  683, 683, 683, 000 },   /* 8  I-Slice - INTRA16 LUMA */
    {  683, 683, 683, 000 },   /* 9  P-Slice - INTRA16 LUMA */
    {  683, 683, 683, 000 },   /* 10 B-Slice - INTRA16 LUMA */
    {  341, 341, 341, 341 },   /* 11 P-Slice - INTER16 LUMA */
    {  341, 341, 341, 341 },   /* 12 B-Slice - INTER16 LUMA */

    {  683, 683, 683, 000 },   /* 13 I-Slice - INTRA16 CHROMA */
    {  683, 683, 683, 000 },   /* 14 P-Slice - INTRA16 CHROMA */
    {  683, 683, 683, 000 },   /* 15 B-Slice - INTRA16 CHROMA */
    {  341, 341, 341, 000 },   /* 16 P-Slice - INTER16 CHROMA */
    {  341, 341, 341, 000 } /* 17 B-Slice - INTER16 CHROMA */
};

static IMG_UINT16 tng__create_gop_frame(
    IMG_UINT8 * pui8Level, IMG_BOOL bReference,
    IMG_UINT8 ui8Pos, IMG_UINT8 ui8Ref0Level,
    IMG_UINT8 ui8Ref1Level, IMG_FRAME_TYPE eFrameType)
{
    *pui8Level = ((ui8Ref0Level > ui8Ref1Level) ? ui8Ref0Level : ui8Ref1Level)  + 1;

    return F_ENCODE(bReference, GOP_REFERENCE) |
           F_ENCODE(ui8Pos, GOP_POS) |
           F_ENCODE(ui8Ref0Level, GOP_REF0) |
           F_ENCODE(ui8Ref1Level, GOP_REF1) |
           F_ENCODE(eFrameType, GOP_FRAMETYPE);
}

static void tng__minigop_generate_flat(void* buffer_p, IMG_UINT32 ui32BFrameCount, IMG_UINT32 ui32RefSpacing, IMG_UINT8 aui8PicOnLevel[])
{
    /* B B B B P */
    IMG_UINT8 ui8EncodeOrderPos;
    IMG_UINT8 ui8Level;
    IMG_UINT16 * psGopStructure = (IMG_UINT16 *)buffer_p;

    psGopStructure[0] = tng__create_gop_frame(&ui8Level, IMG_TRUE, MAX_BFRAMES, ui32RefSpacing, 0, IMG_INTER_P);
    aui8PicOnLevel[ui8Level]++;

    for (ui8EncodeOrderPos = 1; ui8EncodeOrderPos < MAX_GOP_SIZE; ui8EncodeOrderPos++) {
        psGopStructure[ui8EncodeOrderPos] = tng__create_gop_frame(&ui8Level, IMG_FALSE,
                                            ui8EncodeOrderPos - 1, ui32RefSpacing, ui32RefSpacing + 1, IMG_INTER_B);
        aui8PicOnLevel[ui8Level] = ui32BFrameCount;
    }

    for( ui8EncodeOrderPos = 0; ui8EncodeOrderPos < MAX_GOP_SIZE; ui8EncodeOrderPos++) {
        drv_debug_msg(VIDEO_DEBUG_GENERAL,
            "%s: psGopStructure = 0x%06x\n", __FUNCTION__, psGopStructure[ui8EncodeOrderPos]);
    }

    return ;
}

static void tng__gop_split(IMG_UINT16 ** pasGopStructure, IMG_INT8 i8Ref0, IMG_INT8 i8Ref1,
                           IMG_UINT8 ui8Ref0Level, IMG_UINT8 ui8Ref1Level, IMG_UINT8 aui8PicOnLevel[])
{
    IMG_UINT8 ui8Distance = i8Ref1 - i8Ref0;
    IMG_UINT8 ui8Position = i8Ref0 + (ui8Distance >> 1);
    IMG_UINT8 ui8Level;

    if (ui8Distance == 1)
        return;

    /* mark middle as this level */

    (*pasGopStructure)++;
    **pasGopStructure = tng__create_gop_frame(&ui8Level, ui8Distance >= 3, ui8Position, ui8Ref0Level, ui8Ref1Level, IMG_INTER_B);
    aui8PicOnLevel[ui8Level]++;

    if (ui8Distance >= 4)
        tng__gop_split(pasGopStructure, i8Ref0, ui8Position, ui8Ref0Level, ui8Level, aui8PicOnLevel);

    if (ui8Distance >= 3)
        tng__gop_split(pasGopStructure, ui8Position, i8Ref1, ui8Level, ui8Ref1Level, aui8PicOnLevel);
}

static void tng_minigop_generate_hierarchical(void* buffer_p, IMG_UINT32 ui32BFrameCount,
        IMG_UINT32 ui32RefSpacing, IMG_UINT8 aui8PicOnLevel[])
{
    IMG_UINT8 ui8Level;
    IMG_UINT16 * psGopStructure = (IMG_UINT16 *)buffer_p;

    psGopStructure[0] = tng__create_gop_frame(&ui8Level, IMG_TRUE, ui32BFrameCount, ui32RefSpacing, 0, IMG_INTER_P);
    aui8PicOnLevel[ui8Level]++;

    tng__gop_split(&psGopStructure, -1, ui32BFrameCount, ui32RefSpacing, ui32RefSpacing + 1, aui8PicOnLevel);
}

static void tng__generate_scale_tables(IMG_MTX_VIDEO_CONTEXT* psMtxEncCtx)
{
    psMtxEncCtx->ui32InterIntraScale[0] = 0x0004;  // Force intra by scaling its cost by 0
    psMtxEncCtx->ui32InterIntraScale[1] = 0x0103;  // favour intra by a factor 3
    psMtxEncCtx->ui32InterIntraScale[2] = 0x0102;  // favour intra by a factor 2
    psMtxEncCtx->ui32InterIntraScale[3] = 0x0101;  // no bias
    psMtxEncCtx->ui32InterIntraScale[4] = 0x0101;  // no bias
    psMtxEncCtx->ui32InterIntraScale[5] = 0x0201;  // favour inter by a factor 2
    psMtxEncCtx->ui32InterIntraScale[6] = 0x0301;  // favour inter by a factor 3
    psMtxEncCtx->ui32InterIntraScale[7] = 0x0400;  // Force inter by scaling it's cost by 0

    psMtxEncCtx->ui32SkippedCodedScale[0] = 0x0004;  // Force coded by scaling its cost by 0
    psMtxEncCtx->ui32SkippedCodedScale[1] = 0x0103;  // favour coded by a factor 3
    psMtxEncCtx->ui32SkippedCodedScale[2] = 0x0102;  // favour coded by a factor 2
    psMtxEncCtx->ui32SkippedCodedScale[3] = 0x0101;  // no bias
    psMtxEncCtx->ui32SkippedCodedScale[4] = 0x0101;  // no bias
    psMtxEncCtx->ui32SkippedCodedScale[5] = 0x0201;  // favour skipped by a factor 2
    psMtxEncCtx->ui32SkippedCodedScale[6] = 0x0301;  // favour skipped by a factor 3
    psMtxEncCtx->ui32SkippedCodedScale[7] = 0x0400;  // Force skipped by scaling it's cost by 0
    return ;
}

/*!
******************************************************************************
 @Function      tng_update_driver_mv_scaling
 @details
        Setup the registers for scaling candidate motion vectors to take into account
        how far away (temporally) the reference pictures are
******************************************************************************/

static IMG_INT tng__abs(IMG_INT a)
{
    if (a < 0)
        return -a;
    else
        return a;
}

static IMG_INT tng__abs32(IMG_INT32 a)
{
    if (a < 0)
        return -a;
    else
        return a;
}

void tng_update_driver_mv_scaling(
    IMG_UINT32 uFrameNum,
    IMG_UINT32 uRef0Num,
    IMG_UINT32 uRef1Num,
    IMG_UINT32 ui32PicFlags,
    IMG_BOOL   bSkipDuplicateVectors,
    IMG_UINT32 * pui32MVCalc_Below,
    IMG_UINT32 * pui32MVCalc_Colocated,
    IMG_UINT32 * pui32MVCalc_Config)
{
    IMG_UINT32 uMvCalcConfig = 0;
    IMG_UINT32 uMvCalcColocated = F_ENCODE(0x10, TOPAZHP_CR_TEMPORAL_BLEND);
    IMG_UINT32 uMvCalcBelow = 0;

    //If b picture calculate scaling factor for colocated motion vectors
    if (ui32PicFlags & ISINTERB_FLAGS) {
        IMG_INT tb, td, tx;
        IMG_INT iDistScale;

        //calculation taken from H264 spec
        tb = (uFrameNum * 2) - (uRef1Num * 2);
        td = (uRef0Num  * 2) - (uRef1Num * 2);
        tx = (16384 + tng__abs(td / 2)) / td;
        iDistScale = (tb * tx + 32) >> 6;
        if (iDistScale > 1023) iDistScale = 1023;
        if (iDistScale < -1024) iDistScale = -1024;
        uMvCalcColocated |= F_ENCODE(iDistScale, TOPAZHP_CR_COL_DIST_SCALE_FACT);

        //We assume the below temporal mvs are from the latest reference frame
        //rather then the most recently encoded B frame (as Bs aren't reference)

        //Fwd temporal is same as colocated mv scale
        uMvCalcBelow     |= F_ENCODE(iDistScale, TOPAZHP_CR_PIC0_DIST_SCALE_FACTOR);

        //Bkwd temporal needs to be scaled by the recipricol amount in the other direction
        tb = (uFrameNum * 2) - (uRef0Num * 2);
        td = (uRef0Num  * 2) - (uRef1Num * 2);
        tx = (16384 + tng__abs(td / 2)) / td;
        iDistScale = (tb * tx + 32) >> 6;
        if (iDistScale > 1023) iDistScale = 1023;
        if (iDistScale < -1024) iDistScale = -1024;
        uMvCalcBelow |= F_ENCODE(iDistScale, TOPAZHP_CR_PIC1_DIST_SCALE_FACTOR);
    } else {
        //Don't scale the temporal below mvs
        uMvCalcBelow |= F_ENCODE(1 << 8, TOPAZHP_CR_PIC0_DIST_SCALE_FACTOR);

        if (uRef0Num != uRef1Num) {
            IMG_INT iRef0Dist, iRef1Dist;
            IMG_INT iScale;

            //Distance to second reference picture may be different when
            //using multiple reference frames on P. Scale based on difference
            //in temporal distance to ref pic 1 compared to distance to ref pic 0
            iRef0Dist = (uFrameNum - uRef0Num);
            iRef1Dist = (uFrameNum - uRef1Num);
            iScale    = (iRef1Dist << 8) / iRef0Dist;

            if (iScale > 1023) iScale = 1023;
            if (iScale < -1024) iScale = -1024;

            uMvCalcBelow |= F_ENCODE(iScale, TOPAZHP_CR_PIC1_DIST_SCALE_FACTOR);
        } else
            uMvCalcBelow |= F_ENCODE(1 << 8, TOPAZHP_CR_PIC1_DIST_SCALE_FACTOR);
    }

    if (uFrameNum > 0) {
        IMG_INT ref0_distance, ref1_distance;
        IMG_INT jitter0, jitter1;

        ref0_distance = tng__abs32((IMG_INT32)uFrameNum - (IMG_INT32)uRef0Num);
        ref1_distance = tng__abs32((IMG_INT32)uFrameNum - (IMG_INT32)uRef1Num);

        if (!(ui32PicFlags & ISINTERB_FLAGS)) {
            jitter0 = ref0_distance * 1;
            jitter1 = jitter0 > 1 ? 1 : 2;
        } else {
            jitter0 = ref1_distance * 1;
            jitter1 = ref0_distance * 1;
        }

        //Hardware can only cope with 1 - 4 jitter factors
        jitter0 = (jitter0 > 4) ? 4 : (jitter0 < 1) ? 1 : jitter0;
        jitter1 = (jitter1 > 4) ? 4 : (jitter1 < 1) ? 1 : jitter1;

        //Hardware can only cope with 1 - 4 jitter factors
        assert(jitter0 > 0 && jitter0 <= 4 && jitter1 > 0 && jitter1 <= 4);

        uMvCalcConfig |= F_ENCODE(jitter0 - 1, TOPAZHP_CR_MVCALC_IPE0_JITTER_FACTOR) |
                         F_ENCODE(jitter1 - 1, TOPAZHP_CR_MVCALC_IPE1_JITTER_FACTOR);
    }

    uMvCalcConfig |= F_ENCODE(1, TOPAZHP_CR_MVCALC_DUP_VEC_MARGIN);
    uMvCalcConfig |= F_ENCODE(7, TOPAZHP_CR_MVCALC_GRID_MB_X_STEP);
    uMvCalcConfig |= F_ENCODE(13, TOPAZHP_CR_MVCALC_GRID_MB_Y_STEP);
    uMvCalcConfig |= F_ENCODE(3, TOPAZHP_CR_MVCALC_GRID_SUB_STEP);
    uMvCalcConfig |= F_ENCODE(1, TOPAZHP_CR_MVCALC_GRID_DISABLE);

    if (bSkipDuplicateVectors)
        uMvCalcConfig |= F_ENCODE(1, TOPAZHP_CR_MVCALC_NO_PSEUDO_DUPLICATES);

    * pui32MVCalc_Below =   uMvCalcBelow;
    * pui32MVCalc_Colocated = uMvCalcColocated;
    * pui32MVCalc_Config = uMvCalcConfig;
}

static void tng__prepare_mv_estimates(context_ENC_p ctx, IMG_UINT32 ui32StreamIndex)
{
    context_ENC_mem* ps_mem = &(ctx->ctx_mem[ui32StreamIndex]);
    IMG_MTX_VIDEO_CONTEXT* psMtxEncCtx = NULL;
    IMG_UINT32 ui32Distance;

    psb_buffer_map(&(ps_mem->bufs_mtx_context), &(ps_mem->bufs_mtx_context.virtual_addr));
    if (ps_mem->bufs_mtx_context.virtual_addr == NULL) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s error: mapping mtx context\n", __FUNCTION__);
        return ;
    }

    psMtxEncCtx = (IMG_MTX_VIDEO_CONTEXT*)(ps_mem->bufs_mtx_context.virtual_addr);

    /* IDR */
    psMtxEncCtx->sMVSettingsIdr.ui32MVCalc_Config = DEFAULT_MVCALC_CONFIG;  // default based on TRM
    psMtxEncCtx->sMVSettingsIdr.ui32MVCalc_Colocated = 0x00100100;// default based on TRM
    psMtxEncCtx->sMVSettingsIdr.ui32MVCalc_Below = 0x01000100;      // default based on TRM

    tng_update_driver_mv_scaling(
        0, 0, 0, 0, IMG_FALSE, //psMtxEncCtx->bSkipDuplicateVectors, //By default false Newly Added
        &psMtxEncCtx->sMVSettingsIdr.ui32MVCalc_Below,
        &psMtxEncCtx->sMVSettingsIdr.ui32MVCalc_Colocated,
        &psMtxEncCtx->sMVSettingsIdr.ui32MVCalc_Config);

    /* NonB (I or P) */
    for (ui32Distance = 1; ui32Distance <= MAX_BFRAMES + 1; ui32Distance++) {
        psMtxEncCtx->sMVSettingsNonB[ui32Distance - 1].ui32MVCalc_Config = DEFAULT_MVCALC_CONFIG;       // default based on TRM
        psMtxEncCtx->sMVSettingsNonB[ui32Distance - 1].ui32MVCalc_Colocated = 0x00100100;// default based on TRM
        psMtxEncCtx->sMVSettingsNonB[ui32Distance - 1].ui32MVCalc_Below = 0x01000100;   // default based on TRM


        tng_update_driver_mv_scaling(ui32Distance, 0, 0, 0, IMG_FALSE, //psMtxEncCtx->bSkipDuplicateVectors,
                                     &psMtxEncCtx->sMVSettingsNonB[ui32Distance - 1].ui32MVCalc_Below,
                                     &psMtxEncCtx->sMVSettingsNonB[ui32Distance - 1].ui32MVCalc_Colocated,
                                     &psMtxEncCtx->sMVSettingsNonB[ui32Distance - 1].ui32MVCalc_Config);
    }

    {
        IMG_UINT32 ui32DistanceB;
        IMG_UINT32 ui32Position;
        context_ENC_mem *ps_mem = &(ctx->ctx_mem[ui32StreamIndex]);
        IMG_MV_SETTINGS *pHostMVSettingsHierarchical = NULL;
        IMG_MV_SETTINGS *pMvElement = NULL;
        IMG_MV_SETTINGS *pHostMVSettingsBTable = NULL;

        psb_buffer_map(&(ps_mem->bufs_mv_setting_btable), &(ps_mem->bufs_mv_setting_btable.virtual_addr));
        if (ps_mem->bufs_mv_setting_btable.virtual_addr == NULL) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "%s error: mapping mv setting btable\n", __FUNCTION__);
            return ;
        }

        pHostMVSettingsBTable = (IMG_MV_SETTINGS *)(ps_mem->bufs_mv_setting_btable.virtual_addr);

        for (ui32DistanceB = 0; ui32DistanceB < MAX_BFRAMES; ui32DistanceB++) {
            for (ui32Position = 1; ui32Position <= ui32DistanceB + 1; ui32Position++) {
                pMvElement = (IMG_MV_SETTINGS * ) ((IMG_UINT8 *) pHostMVSettingsBTable + MV_OFFSET_IN_TABLE(ui32DistanceB, ui32Position - 1));
                pMvElement->ui32MVCalc_Config= (DEFAULT_MVCALC_CONFIG|MASK_TOPAZHP_CR_MVCALC_GRID_DISABLE);    // default based on TRM
                pMvElement->ui32MVCalc_Colocated=0x00100100;// default based on TRM
                pMvElement->ui32MVCalc_Below=0x01000100;	// default based on TRM

                tng_update_driver_mv_scaling(
                    ui32Position, ui32DistanceB + 2, 0, ISINTERB_FLAGS, IMG_FALSE,
                    &pMvElement->ui32MVCalc_Below,
                    &pMvElement->ui32MVCalc_Colocated,
                    &pMvElement->ui32MVCalc_Config);
            }
        }

        if (ctx->b_is_mv_setting_hierar){
            pHostMVSettingsHierarchical = (IMG_MV_SETTINGS *)(ps_mem->bufs_mv_setting_hierar.virtual_addr);

            for (ui32DistanceB = 0; ui32DistanceB < MAX_BFRAMES; ui32DistanceB++) {
                pMvElement = (IMG_MV_SETTINGS * ) ((IMG_UINT8 *)pHostMVSettingsBTable + MV_OFFSET_IN_TABLE(ui32DistanceB, ui32DistanceB >> 1));
                //memcpy(pHostMVSettingsHierarchical + ui32DistanceB, , sizeof(IMG_MV_SETTINGS));
                pHostMVSettingsHierarchical[ui32DistanceB].ui32MVCalc_Config    = pMvElement->ui32MVCalc_Config;
                pHostMVSettingsHierarchical[ui32DistanceB].ui32MVCalc_Colocated = pMvElement->ui32MVCalc_Colocated;
                pHostMVSettingsHierarchical[ui32DistanceB].ui32MVCalc_Below     = pMvElement->ui32MVCalc_Below;
            }
        }
        psb_buffer_unmap(&(ps_mem->bufs_mv_setting_btable));
    }

    psb_buffer_unmap(&(ps_mem->bufs_mtx_context));

    return ;
}

static void tng__adjust_picflags(
    context_ENC_p ctx,
    IMG_RC_PARAMS * psRCParams,
    IMG_BOOL bFirstPic,
    IMG_UINT32 * pui32Flags)
{
    IMG_UINT32 ui32Flags;
    PIC_PARAMS * psPicParams = &ctx->sPicParams;
    ui32Flags = psPicParams->ui32Flags;

    if (!psRCParams->bRCEnable || (!bFirstPic))
        ui32Flags = 0;

    switch (ctx->eStandard) {
    case IMG_STANDARD_NONE:
        break;
    case IMG_STANDARD_H264:
        break;
    case IMG_STANDARD_H263:
        ui32Flags |= ISH263_FLAGS;
        break;
    case IMG_STANDARD_MPEG4:
        ui32Flags |= ISMPEG4_FLAGS;
        break;
    case IMG_STANDARD_MPEG2:
        ui32Flags |= ISMPEG2_FLAGS;
        break;
    default:
        break;
    }
    * pui32Flags = ui32Flags;
}

#define gbLowLatency 0

static void tng__setup_rcdata(context_ENC_p ctx)
{
    IMG_RC_PARAMS *psRCParams = &(ctx->sRCParams);
    PIC_PARAMS    *psPicParams = &(ctx->sPicParams);
    
    IMG_INT32 i32FrameRate, i32TmpQp;
    double        L1, L2, L3,L4, L5, L6, flBpp;
    IMG_INT32 i32BufferSizeInFrames;

    if (ctx->bInsertHRDParams &&
       (ctx->eStandard == IMG_STANDARD_H264)) {
       psRCParams->ui32BufferSize = ctx->buffer_size;
       psRCParams->i32InitialLevel = ctx->buffer_size - ctx->initial_buffer_fullness;
       psRCParams->i32InitialDelay = ctx->initial_buffer_fullness;
    }

    // If Bit Rate and Basic Units are not specified then set to default values.
    if (psRCParams->ui32BitsPerSecond == 0 && !ctx->bEnableMVC) {
        psRCParams->ui32BitsPerSecond = 640000;     // kbps
    }
    
    if (!psRCParams->ui32BUSize) {
        psRCParams->ui32BUSize = (ctx->ui16PictureHeight>>4) * (ctx->ui16Width>>4);		// BU = 1 Frame
    }

    if (!psRCParams->ui32FrameRate) {
        psRCParams->ui32FrameRate = 30;		// fps
    }

    // Calculate Bits Per Pixel
    if (ctx->ui16Width <= 176 ) {
        i32FrameRate    = 30;
    } else {
        i32FrameRate	= psRCParams->ui32FrameRate;
    }

    flBpp = 1.0 * psRCParams->ui32BitsPerSecond / (i32FrameRate * ctx->ui16Width * ctx->ui16FrameHeight);

    psPicParams->sInParams.ui8SeInitQP          = psRCParams->ui32InitialQp;
    psPicParams->sInParams.ui8MBPerRow      = (ctx->ui16Width>>4);
    psPicParams->sInParams.ui16MBPerBU       = psRCParams->ui32BUSize;
    psPicParams->sInParams.ui16MBPerFrm     = (ctx->ui16Width>>4) * (ctx->ui16PictureHeight>>4);
    psPicParams->sInParams.ui16BUPerFrm      = (psPicParams->sInParams.ui16MBPerFrm) / psRCParams->ui32BUSize;

    psPicParams->sInParams.ui16IntraPeriod      = psRCParams->ui32IntraFreq;
    psPicParams->sInParams.ui16BFrames         = psRCParams->ui16BFrames;
    psPicParams->sInParams.i32BitRate             = psRCParams->ui32BitsPerSecond;

    psPicParams->sInParams.bFrmSkipDisable   = psRCParams->bDisableFrameSkipping;
    psPicParams->sInParams.i32BitsPerFrm       = (psRCParams->ui32BitsPerSecond + psRCParams->ui32FrameRate/2) / psRCParams->ui32FrameRate;
    psPicParams->sInParams.i32BitsPerBU         = psPicParams->sInParams.i32BitsPerFrm / (4 * psPicParams->sInParams.ui16BUPerFrm);

    // Codec-dependant fields
    if (ctx->eStandard == IMG_STANDARD_H264) {
        psPicParams->sInParams.mode.h264.i32TransferRate = (psRCParams->ui32TransferBitsPerSecond + psRCParams->ui32FrameRate/2) / psRCParams->ui32FrameRate;
        psPicParams->sInParams.mode.h264.bHierarchicalMode =   psRCParams->b16Hierarchical;
    } else {
        psPicParams->sInParams.mode.other.i32BitsPerGOP      = (psRCParams->ui32BitsPerSecond / psRCParams->ui32FrameRate) * psRCParams->ui32IntraFreq;
        psPicParams->sInParams.mode.other.ui16AvQPVal         = psRCParams->ui32InitialQp;
        psPicParams->sInParams.mode.other.ui16MyInitQP         = psRCParams->ui32InitialQp;
    }

    
    if (psPicParams->sInParams.i32BitsPerFrm) {
        i32BufferSizeInFrames = (psRCParams->ui32BufferSize + (psPicParams->sInParams.i32BitsPerFrm/2))/psPicParams->sInParams.i32BitsPerFrm;
    } else {
        IMG_ASSERT(ctx->bEnableMvc && "Can happen only in MVC mode");
        /* Asigning more or less `normal` value. To be overriden by MVC RC module */
        i32BufferSizeInFrames = 30;
    }

    // select thresholds and initial Qps etc that are codec dependent 
    switch (ctx->eStandard) {
        case IMG_STANDARD_H264:
            L1 = 0.1;	L2 = 0.15;	L3 = 0.2;
            psPicParams->sInParams.ui8MaxQPVal = 51;
            ctx->ui32KickSize = psPicParams->sInParams.ui16MBPerBU;

            // Setup MAX and MIN Quant Values
            if (psRCParams->iMinQP == 0) {
                if (flBpp >= 0.50)
                    i32TmpQp = 4;
                else if (flBpp > 0.133)
                    i32TmpQp = (IMG_INT32)(22 - (40*flBpp));
                else
                    i32TmpQp = (IMG_INT32)(30 - (100 * flBpp));

                /* Adjust minQp up for small buffer size and down for large buffer size */
                if (i32BufferSizeInFrames < 5) {
                    i32TmpQp += 2;
                }

                if (i32BufferSizeInFrames > 40) {
                    if(i32TmpQp>=1)
                        i32TmpQp -= 1;
                }
                /* for HD content allow a lower minQp as bitrate is more easily controlled in this case */
                if (psPicParams->sInParams.ui16MBPerFrm > 2000) {
                        i32TmpQp -= 6;
                }
            } else
                i32TmpQp = psRCParams->iMinQP;				
			
            if (i32TmpQp < 2) {
                psPicParams->sInParams.ui8MinQPVal = 2;
            } else {
                psPicParams->sInParams.ui8MinQPVal = i32TmpQp;
            }

            // Calculate Initial QP if it has not been specified
            i32TmpQp = psPicParams->sInParams.ui8SeInitQP;
            if (psPicParams->sInParams.ui8SeInitQP==0) {
                L1 = 0.050568;
                L2 = 0.202272;
                L3 = 0.40454321;
                L4 = 0.80908642;
                L5 = 1.011358025;

                if (flBpp < L1)
                    i32TmpQp = (IMG_INT32)(45 - 78.10*flBpp);
                else if (flBpp>=L1 && flBpp<L2)
                    i32TmpQp = (IMG_INT32)(44 - 72.51*flBpp);
                else if (flBpp>=L2 && flBpp<L3)
                    i32TmpQp = (IMG_INT32)(34 - 24.72*flBpp);
                else if (flBpp>=L3 && flBpp<L4)
                    i32TmpQp = (IMG_INT32)(32 - 19.78*flBpp);
                else if (flBpp>=L4 && flBpp<L5)
                    i32TmpQp = (IMG_INT32)(25 - 9.89*flBpp);
                else if (flBpp>=L5)
                    i32TmpQp = (IMG_INT32)(18 - 4.95*flBpp);

                /* Adjust ui8SeInitQP up for small buffer size or small fps */
                /* Adjust ui8SeInitQP up for small gop size */
                if ((i32BufferSizeInFrames < 20) || (psRCParams->ui32IntraFreq < 20)) {
                    i32TmpQp += 2;
                }

		/* for very small buffers increase initial Qp even more */
		if(i32BufferSizeInFrames < 5)
		{
			i32TmpQp += 8;
		}

                /* start on a lower initial Qp for HD content as the coding is more efficient */
                if (psPicParams->sInParams.ui16MBPerFrm > 2000) {
                    i32TmpQp -= 2;
                }

		if(psPicParams->sInParams.ui16IntraPeriod ==1)
		{
		    /* for very small GOPS start with a much higher initial Qp */
		    i32TmpQp += 12;
		} else if (psPicParams->sInParams.ui16IntraPeriod<5) {
		    /* for very small GOPS start with a much higher initial Qp */
		    i32TmpQp += 6;
		}
            }
            if (i32TmpQp>49) {
                i32TmpQp = 49;
            }
            if (i32TmpQp < psPicParams->sInParams.ui8MinQPVal) {
                i32TmpQp = psPicParams->sInParams.ui8MinQPVal;
            }
            psPicParams->sInParams.ui8SeInitQP = i32TmpQp;

            if(flBpp <= 0.3)
                psPicParams->ui32Flags |= ISRC_I16BIAS;

            break;

        case IMG_STANDARD_MPEG4:
        case IMG_STANDARD_MPEG2:
        case IMG_STANDARD_H263:
            psPicParams->sInParams.ui8MaxQPVal	 = 31;
            if (ctx->ui16Width == 176) {
                L1 = 0.042;    L2 = 0.084;    L3 = 0.126;    L4 = 0.168;    L5 = 0.336;    L6=0.505;
            } else if (ctx->ui16Width == 352) {	
                L1 = 0.064;    L2 = 0.084;    L3 = 0.106;    L4 = 0.126;    L5 = 0.168;    L6=0.210;
            } else {
                L1 = 0.050;    L2 = 0.0760;    L3 = 0.096;   L4 = 0.145;    L5 = 0.193;    L6=0.289;
            }

            if (psPicParams->sInParams.ui8SeInitQP==0) {
                if (flBpp < L1)
                    psPicParams->sInParams.ui8SeInitQP = 31;
                else if (flBpp>=L1 && flBpp<L2)
                    psPicParams->sInParams.ui8SeInitQP = 26;
                else if (flBpp>=L2 && flBpp<L3)
                    psPicParams->sInParams.ui8SeInitQP = 22;
                else if (flBpp>=L3 && flBpp<L4)
                    psPicParams->sInParams.ui8SeInitQP = 18;
                else if (flBpp>=L4 && flBpp<L5)
                    psPicParams->sInParams.ui8SeInitQP = 14;
                else if (flBpp>=L5 && flBpp<L6)
                    psPicParams->sInParams.ui8SeInitQP = 10;
                else
                    psPicParams->sInParams.ui8SeInitQP = 8;

                /* Adjust ui8SeInitQP up for small buffer size or small fps */
                /* Adjust ui8SeInitQP up for small gop size */
                if ((i32BufferSizeInFrames < 20) || (psRCParams->ui32IntraFreq < 20)) {
                    psPicParams->sInParams.ui8SeInitQP += 2;
                }

                if (psPicParams->sInParams.ui8SeInitQP > psPicParams->sInParams.ui8MaxQPVal) {
                    psPicParams->sInParams.ui8SeInitQP = psPicParams->sInParams.ui8MaxQPVal;
                }
                psPicParams->sInParams.mode.other.ui16AvQPVal =  psPicParams->sInParams.ui8SeInitQP;
            }
            psPicParams->sInParams.ui8MinQPVal = 2;

            /* Adjust minQp up for small buffer size and down for large buffer size */
            if (i32BufferSizeInFrames < 20) {
                psPicParams->sInParams.ui8MinQPVal += 1;
            }
            break;

        default:
            /* the NO RC cases will fall here */
            break;
    }

    if (ctx->sRCParams.eRCMode == IMG_RCMODE_VBR) {
        psPicParams->sInParams.ui16MBPerBU  = psPicParams->sInParams.ui16MBPerFrm;
        psPicParams->sInParams.ui16BUPerFrm = 1;

        // Initialize the parameters of fluid flow traffic model. 
        psPicParams->sInParams.i32BufferSize   = psRCParams->ui32BufferSize;


        // These scale factor are used only for rate control to avoid overflow
        // in fixed-point calculation these scale factors are decided by bit rate
        if (psRCParams->ui32BitsPerSecond < 640000) {
            psPicParams->sInParams.ui8ScaleFactor  = 2;						// related to complexity
        }
        else if (psRCParams->ui32BitsPerSecond < 2000000) {
            // 2 Mbits
            psPicParams->sInParams.ui8ScaleFactor  = 4;
        }
        else if(psRCParams->ui32BitsPerSecond < 8000000) {
            // 8 Mbits
            psPicParams->sInParams.ui8ScaleFactor  = 6;
        } else
            psPicParams->sInParams.ui8ScaleFactor  = 8;
    } else {
        // Set up Input Parameters that are mode dependent
        switch (ctx->eStandard) {
            case IMG_STANDARD_H264:
                // ------------------- H264 CBR RC ------------------- //	
                // Initialize the parameters of fluid flow traffic model.
                psPicParams->sInParams.i32BufferSize = psRCParams->ui32BufferSize;

                // HRD consideration - These values are used by H.264 reference code.
                if (psRCParams->ui32BitsPerSecond < 1000000) {
                // 1 Mbits/s 
                    psPicParams->sInParams.ui8ScaleFactor = 0;
                } else if (psRCParams->ui32BitsPerSecond < 2000000) {
                // 2 Mbits/s
                    psPicParams->sInParams.ui8ScaleFactor = 1;
                } else if (psRCParams->ui32BitsPerSecond < 4000000) {
                // 4 Mbits/s 
                    psPicParams->sInParams.ui8ScaleFactor = 2;
                } else if (psRCParams->ui32BitsPerSecond < 8000000) {
                // 8 Mbits/s
                    psPicParams->sInParams.ui8ScaleFactor = 3;
                } else  {
                    psPicParams->sInParams.ui8ScaleFactor = 4; 
                }

                if (ctx->sRCParams.eRCMode == IMG_RCMODE_VCM) {
                    psPicParams->sInParams.i32BufferSize = i32BufferSizeInFrames;
                }
                break;
            case IMG_STANDARD_MPEG4:
            case IMG_STANDARD_MPEG2:
            case IMG_STANDARD_H263:
                flBpp  = 256 * (psRCParams->ui32BitsPerSecond/ctx->ui16Width);
                flBpp /= (ctx->ui16FrameHeight * psRCParams->ui32FrameRate);

                if ((psPicParams->sInParams.ui16MBPerFrm > 1024 && flBpp < 16) || (psPicParams->sInParams.ui16MBPerFrm <= 1024 && flBpp < 24))
                    psPicParams->sInParams.mode.other.ui8HalfFrameRate = 1;
                else
                    psPicParams->sInParams.mode.other.ui8HalfFrameRate = 0;

                if (psPicParams->sInParams.mode.other.ui8HalfFrameRate >= 1) {
                    psPicParams->sInParams.ui8SeInitQP = 31;
                    psPicParams->sInParams.mode.other.ui16AvQPVal = 31;
                    psPicParams->sInParams.mode.other.ui16MyInitQP = 31;
                }

                psPicParams->sInParams.i32BufferSize = psRCParams->ui32BufferSize;
                break;
            default:
                break;
        }
    }

    if (psRCParams->bScDetectDisable)
        psPicParams->ui32Flags  |= ISSCENE_DISABLED;

    psPicParams->sInParams.i32InitialDelay	= psRCParams->i32InitialDelay;
    psPicParams->sInParams.i32InitialLevel	= psRCParams->i32InitialLevel;
    psRCParams->ui32InitialQp = psPicParams->sInParams.ui8SeInitQP;

    /* The rate control uses this value to adjust the reaction rate to larger than expected frames */
    if (ctx->eStandard == IMG_STANDARD_H264) {
        if (psPicParams->sInParams.i32BitsPerFrm) {
            const IMG_INT32 bitsPerGop = (psRCParams->ui32BitsPerSecond / psRCParams->ui32FrameRate) * psRCParams->ui32IntraFreq;
            psPicParams->sInParams.mode.h264.ui32RCScaleFactor = (bitsPerGop * 256) /
                (psPicParams->sInParams.i32BufferSize - psPicParams->sInParams.i32InitialLevel);
        } else {
            psPicParams->sInParams.mode.h264.ui32RCScaleFactor = 0;
        }
    } else {
        psPicParams->sInParams.mode.other.ui16MyInitQP		= psPicParams->sInParams.ui8SeInitQP;
    }

    return ;
}

static void tng__save_slice_params_template(
    context_ENC_p ctx,
    IMG_UINT32  ui32SliceBufIdx,
    IMG_UINT32  ui32SliceType,
    IMG_UINT32  ui32IPEControl,
    IMG_UINT32  ui32Flags,
    IMG_UINT32  ui32SliceConfig,
    IMG_UINT32  ui32SeqConfig,
    IMG_UINT32  ui32StreamIndex
)
{
    IMG_FRAME_TEMPLATE_TYPE eSliceType = (IMG_FRAME_TEMPLATE_TYPE)ui32SliceType;
    context_ENC_mem *ps_mem = &(ctx->ctx_mem[ui32StreamIndex]);
    SLICE_PARAMS *slice_temp_p = NULL;

    psb_buffer_map(&(ps_mem->bufs_slice_template), &(ps_mem->bufs_slice_template.virtual_addr));
    if (ps_mem->bufs_slice_template.virtual_addr == NULL) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s error: mapping slice template\n", __FUNCTION__);
        return ;
    }

    slice_temp_p = (SLICE_PARAMS*)(ps_mem->bufs_slice_template.virtual_addr + (ctx->ctx_mem_size.slice_template * ui32SliceBufIdx));

    slice_temp_p->eTemplateType = eSliceType;
    slice_temp_p->ui32Flags = ui32Flags;
    slice_temp_p->ui32IPEControl = ui32IPEControl;
    slice_temp_p->ui32SliceConfig = ui32SliceConfig;
    slice_temp_p->ui32SeqConfig = ui32SeqConfig;

    psb_buffer_unmap(&(ps_mem->bufs_slice_template));

    return ;
}


/*****************************************************************************
 * Function Name        :       PrepareEncodeSliceParams
 *
 ****************************************************************************/
static IMG_UINT32 tng__prepare_encode_sliceparams(
    context_ENC_p ctx,
    IMG_UINT32  ui32SliceBufIdx,
    IMG_UINT32  ui32SliceType,
    IMG_UINT16  __maybe_unused ui16CurrentRow,
    IMG_UINT16  ui16SliceHeight,
    IMG_UINT8   uiDeblockIDC,
    IMG_BOOL    bFieldMode,
    IMG_INT     iFineYSearchSize,
    IMG_UINT32  ui32StreamIndex
)
{
    IMG_UINT32      ui32FrameStoreFormat;
    IMG_UINT8       ui8SwapChromas;
    IMG_UINT32      ui32MBsPerKick, ui32KicksPerSlice;
    IMG_UINT32      ui32IPEControl;
    IMG_UINT32      ui32Flags = 0;
    IMG_UINT32      ui32SliceConfig = 0;
    IMG_UINT32      ui32SeqConfig = 0;
    IMG_BOOL bIsIntra = IMG_FALSE;
    IMG_BOOL bIsBPicture = IMG_FALSE;
    IMG_BOOL bIsIDR = IMG_FALSE;
    IMG_IPE_MINBLOCKSIZE blkSz;
    IMG_FRAME_TEMPLATE_TYPE eSliceType = (IMG_FRAME_TEMPLATE_TYPE)ui32SliceType;

    if (!ctx) {
        return VA_STATUS_ERROR_INVALID_CONTEXT;
    }

    /* We want multiple ones of these so we can submit multiple slices without having to wait for the next*/
    ui32IPEControl = ctx->ui32IPEControl;
    bIsIntra = ((eSliceType == IMG_FRAME_IDR) || (eSliceType == IMG_FRAME_INTRA));
    bIsBPicture = (eSliceType == IMG_FRAME_INTER_B);
    bIsIDR = ((eSliceType == IMG_FRAME_IDR) || (eSliceType == IMG_FRAME_INTER_P_IDR));

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s PTG bIsIntra  = %x\n", __FUNCTION__, bIsIntra);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s PTG bIsBFrame = %x\n", __FUNCTION__, bIsBPicture);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s PTG bIsIDR    = %x\n", __FUNCTION__, bIsIDR);
    /* extract block size */
    blkSz = F_EXTRACT(ui32IPEControl, TOPAZHP_CR_IPE_BLOCKSIZE);
    /* mask-out the block size bits from ui32IPEControl */
    ui32IPEControl &= ~(F_MASK(TOPAZHP_CR_IPE_BLOCKSIZE));
    switch (ctx->eStandard) {
    case IMG_STANDARD_NONE:
    case IMG_STANDARD_JPEG:
        break;

    case IMG_STANDARD_H264:
        if (blkSz > 2) blkSz = 2;
        if (bIsBPicture) {
            if (blkSz > 1) blkSz = 1;
        }
#ifdef BRN_30322
        else if (bIsIntra) {
            if (blkSz == 0) blkSz = 1; // Workaround for BRN 30322
        }
#endif

#ifdef BRN_30550
        if (ctx->bCabacEnabled)
            if (blkSz == 0) blkSz = 1;
#endif
        if (ctx->uMBspS >= _1080P_30FPS) {
            ui32IPEControl |= F_ENCODE(iFineYSearchSize, TOPAZHP_CR_IPE_LRITC_BOUNDARY) |
                              F_ENCODE(iFineYSearchSize, TOPAZHP_CR_IPE_Y_FINE_SEARCH);
        } else {
            ui32IPEControl |= F_ENCODE(iFineYSearchSize + 1, TOPAZHP_CR_IPE_LRITC_BOUNDARY) |
                              F_ENCODE(iFineYSearchSize, TOPAZHP_CR_IPE_Y_FINE_SEARCH);

        }
        if (ctx->bLimitNumVectors)
            ui32IPEControl |= F_ENCODE(1, TOPAZHP_CR_IPE_MV_NUMBER_RESTRICTION);
        break;

    case IMG_STANDARD_H263:
        blkSz = 0;
        ui32IPEControl = F_ENCODE(iFineYSearchSize + 1, TOPAZHP_CR_IPE_LRITC_BOUNDARY) |
                         F_ENCODE(iFineYSearchSize, TOPAZHP_CR_IPE_Y_FINE_SEARCH) |
                         F_ENCODE(0, TOPAZHP_CR_IPE_4X4_SEARCH);
        //We only support a maxium vector of 15.5 pixels in H263
        break;

    case IMG_STANDARD_MPEG4:
        if (blkSz > BLK_SZ_8x8) blkSz = BLK_SZ_8x8;
        ui32IPEControl |= F_ENCODE(iFineYSearchSize + 1, TOPAZHP_CR_IPE_LRITC_BOUNDARY) |
                          F_ENCODE(iFineYSearchSize, TOPAZHP_CR_IPE_Y_FINE_SEARCH) |
                          F_ENCODE(0, TOPAZHP_CR_IPE_4X4_SEARCH);
        // FIXME Should be 1, set to zero for hardware testing.
        break;
    case IMG_STANDARD_MPEG2:
        if (blkSz != BLK_SZ_16x16) blkSz = BLK_SZ_16x16;
        ui32IPEControl |= F_ENCODE(iFineYSearchSize + 1, TOPAZHP_CR_IPE_LRITC_BOUNDARY) |
                          F_ENCODE(iFineYSearchSize, TOPAZHP_CR_IPE_Y_FINE_SEARCH) |
                          F_ENCODE(0, TOPAZHP_CR_IPE_4X4_SEARCH);
        // FIXME Should be 1, set to zero for hardware testing.
        break;
    }

    {
        IMG_BOOL bRestrict4x4SearchSize;
        IMG_UINT32 uLritcBoundary;

        if (ctx->uMBspS >= _1080P_30FPS)
            bRestrict4x4SearchSize = 1;
        else
            bRestrict4x4SearchSize = 0;

        ui32IPEControl |= F_ENCODE(blkSz, TOPAZHP_CR_IPE_BLOCKSIZE);
        uLritcBoundary = (blkSz != BLK_SZ_16x16) ? (iFineYSearchSize + (bRestrict4x4SearchSize ? 0 : 1)) : 1;
        if (uLritcBoundary > 3) {
            return VA_STATUS_ERROR_UNKNOWN;
        }

        /* Minium sub block size to calculate motion vectors for. 0=16x16, 1=8x8, 2=4x4 */
        ui32IPEControl = F_INSERT(ui32IPEControl, blkSz, TOPAZHP_CR_IPE_BLOCKSIZE);
        ui32IPEControl = F_INSERT(ui32IPEControl, iFineYSearchSize, TOPAZHP_CR_IPE_Y_FINE_SEARCH);
        ui32IPEControl = F_INSERT(ui32IPEControl, ctx->bLimitNumVectors, TOPAZHP_CR_IPE_MV_NUMBER_RESTRICTION);

        ui32IPEControl = F_INSERT(ui32IPEControl, uLritcBoundary, TOPAZHP_CR_IPE_LRITC_BOUNDARY);  // 8x8 search
        ui32IPEControl = F_INSERT(ui32IPEControl, bRestrict4x4SearchSize ? 0 : 1, TOPAZHP_CR_IPE_4X4_SEARCH);

    }
    ui32IPEControl = F_INSERT(ui32IPEControl, ctx->bHighLatency, TOPAZHP_CR_IPE_HIGH_LATENCY);
//              psSliceParams->ui32IPEControl = ui32IPEControl;

    if (!bIsIntra) {
        if (bIsBPicture)
            ui32Flags |= ISINTERB_FLAGS;
        else
            ui32Flags |= ISINTERP_FLAGS;
    }
    switch (ctx->eStandard)  {
    case IMG_STANDARD_NONE:
        break;
    case IMG_STANDARD_H263:
        ui32Flags |= ISH263_FLAGS;
        break;
    case IMG_STANDARD_MPEG4:
        ui32Flags |= ISMPEG4_FLAGS;
        break;
    case IMG_STANDARD_MPEG2:
        ui32Flags |= ISMPEG2_FLAGS;
        break;
    default:
        break;
    }

    if (ctx->bMultiReferenceP && !(bIsIntra || bIsBPicture))
        ui32Flags |= ISMULTIREF_FLAGS;
    if (ctx->bSpatialDirect && bIsBPicture)
        ui32Flags |= SPATIALDIRECT_FLAGS;

    if (bIsIntra) {
        ui32SliceConfig = F_ENCODE(TOPAZHP_CR_SLICE_TYPE_I_SLICE, TOPAZHP_CR_SLICE_TYPE);
    } else {
        if (bIsBPicture) {
            ui32SliceConfig = F_ENCODE(TOPAZHP_CR_SLICE_TYPE_B_SLICE, TOPAZHP_CR_SLICE_TYPE);
        } else {
            // p frame
            ui32SliceConfig = F_ENCODE(TOPAZHP_CR_SLICE_TYPE_P_SLICE, TOPAZHP_CR_SLICE_TYPE);
        }
    }

    ui32MBsPerKick = ctx->ui32KickSize;
    // we need to figure out the number of kicks and mb's per kick to use.
    // on H.264 we will use a MB's per kick of basic unit
    // on other rc varients we will use mb's per kick of width
    ui32KicksPerSlice = ((ui16SliceHeight / 16) * (ctx->ui16Width / 16)) / ui32MBsPerKick;
    assert((ui32KicksPerSlice * ui32MBsPerKick) == ((ui16SliceHeight / 16)*(ctx->ui16Width / 16)));

    // need some sensible ones don't look to be implemented yet...
    // change per stream

    if ((ctx->eFormat == IMG_CODEC_UY0VY1_8888) || (ctx->eFormat == IMG_CODEC_VY0UY1_8888))
        ui32FrameStoreFormat = 3;
    else if ((ctx->eFormat == IMG_CODEC_Y0UY1V_8888) || (ctx->eFormat == IMG_CODEC_Y0VY1U_8888))
        ui32FrameStoreFormat = 2;
    else if ((ctx->eFormat == IMG_CODEC_PL12) || (ctx->eFormat == IMG_CODEC_422_PL12))
        ui32FrameStoreFormat = 1;
    else
        ui32FrameStoreFormat = 0;

    if ((ctx->eFormat == IMG_CODEC_VY0UY1_8888) || (ctx->eFormat == IMG_CODEC_Y0VY1U_8888))
        ui8SwapChromas = 1;
    else
        ui8SwapChromas = 0;

    switch (ctx->eStandard) {
    case IMG_STANDARD_NONE:
    case IMG_STANDARD_JPEG:
        break;
    case IMG_STANDARD_H264:
        /* H264 */

        ui32SeqConfig = F_ENCODE(0, TOPAZHP_CR_TEMPORAL_PIC0_BELOW_IN_VALID)
                        | F_ENCODE(0, TOPAZHP_CR_TEMPORAL_PIC1_BELOW_IN_VALID)
                        | F_ENCODE(0, TOPAZHP_CR_ABOVE_OUT_OF_SLICE_VALID)
                        | F_ENCODE(1, TOPAZHP_CR_WRITE_TEMPORAL_PIC0_BELOW_VALID)
                        | F_ENCODE(0, TOPAZHP_CR_REF_PIC0_VALID)
                        | F_ENCODE(0, TOPAZHP_CR_REF_PIC1_VALID)
                        | F_ENCODE(!bIsBPicture, TOPAZHP_CR_REF_PIC1_EQUAL_PIC0)
                        | F_ENCODE(bFieldMode ? 1 : 0 , TOPAZHP_CR_FIELD_MODE)
                        | F_ENCODE(ui8SwapChromas, TOPAZHP_CR_FRAME_STORE_CHROMA_SWAP)
                        | F_ENCODE(ui32FrameStoreFormat, TOPAZHP_CR_FRAME_STORE_FORMAT)
                        | F_ENCODE(TOPAZHP_CR_ENCODER_STANDARD_H264, TOPAZHP_CR_ENCODER_STANDARD)
                        | F_ENCODE(uiDeblockIDC == 1 ? 0 : 1, TOPAZHP_CR_DEBLOCK_ENABLE);

        if (ctx->sRCParams.ui16BFrames) {
            ui32SeqConfig |= F_ENCODE(1, TOPAZHP_CR_WRITE_TEMPORAL_COL_VALID);
            if ((ui32Flags & ISINTERB_FLAGS) == ISINTERB_FLAGS)
                ui32SeqConfig |= F_ENCODE(1, TOPAZHP_CR_TEMPORAL_COL_IN_VALID);
        }

        if (!bIsBPicture) {
            ui32SeqConfig |= F_ENCODE(1, TOPAZHP_CR_WRITE_TEMPORAL_COL_VALID);
        }

        break;
    case IMG_STANDARD_MPEG4:
        /* MPEG4 */
        ui32SeqConfig = F_ENCODE(1, TOPAZHP_CR_WRITE_RECON_PIC)
                        | F_ENCODE(0, TOPAZHP_CR_DEBLOCK_ENABLE)
                        | F_ENCODE(0, TOPAZHP_CR_TEMPORAL_PIC0_BELOW_IN_VALID)
                        | F_ENCODE(0, TOPAZHP_CR_TEMPORAL_PIC1_BELOW_IN_VALID)
                        | F_ENCODE(0, TOPAZHP_CR_ABOVE_OUT_OF_SLICE_VALID)
                        | F_ENCODE(((ui32Flags & ISINTERP_FLAGS) == ISINTERP_FLAGS), TOPAZHP_CR_WRITE_TEMPORAL_PIC0_BELOW_VALID)
                        | F_ENCODE(0, TOPAZHP_CR_REF_PIC0_VALID)
                        | F_ENCODE(0, TOPAZHP_CR_REF_PIC1_VALID)
                        | F_ENCODE(1, TOPAZHP_CR_REF_PIC1_EQUAL_PIC0)
                        | F_ENCODE(0, TOPAZHP_CR_FIELD_MODE)
                        | F_ENCODE(ui8SwapChromas, TOPAZHP_CR_FRAME_STORE_CHROMA_SWAP)
                        | F_ENCODE(ui32FrameStoreFormat, TOPAZHP_CR_FRAME_STORE_FORMAT)
                        | F_ENCODE(TOPAZHP_CR_ENCODER_STANDARD_MPEG4, TOPAZHP_CR_ENCODER_STANDARD);
        break;
    case IMG_STANDARD_MPEG2:
        /* MPEG2 */
        ui32SeqConfig = F_ENCODE(1, TOPAZHP_CR_WRITE_RECON_PIC)
                        | F_ENCODE(0, TOPAZHP_CR_DEBLOCK_ENABLE)
                        | F_ENCODE(0, TOPAZHP_CR_TEMPORAL_PIC0_BELOW_IN_VALID)
                        | F_ENCODE(0, TOPAZHP_CR_TEMPORAL_PIC1_BELOW_IN_VALID)
                        | F_ENCODE(0, TOPAZHP_CR_ABOVE_OUT_OF_SLICE_VALID)
                        | F_ENCODE(((ui32Flags & ISINTERP_FLAGS) == ISINTERP_FLAGS), TOPAZHP_CR_WRITE_TEMPORAL_PIC0_BELOW_VALID)
                        | F_ENCODE(1, TOPAZHP_CR_REF_PIC0_VALID)
                        | F_ENCODE(0, TOPAZHP_CR_REF_PIC1_VALID)
                        | F_ENCODE(1, TOPAZHP_CR_REF_PIC1_EQUAL_PIC0)
                        | F_ENCODE(bFieldMode ? 1 : 0 , TOPAZHP_CR_FIELD_MODE)
                        | F_ENCODE(ui8SwapChromas, TOPAZHP_CR_FRAME_STORE_CHROMA_SWAP)
                        | F_ENCODE(ui32FrameStoreFormat, TOPAZHP_CR_FRAME_STORE_FORMAT)
                        | F_ENCODE(TOPAZHP_CR_ENCODER_STANDARD_MPEG2, TOPAZHP_CR_ENCODER_STANDARD);
        break;
    case IMG_STANDARD_H263:
        /* H263 */
        ui32SeqConfig = F_ENCODE(1, TOPAZHP_CR_WRITE_RECON_PIC)
                        | F_ENCODE(0, TOPAZHP_CR_DEBLOCK_ENABLE)
                        | F_ENCODE(0, TOPAZHP_CR_TEMPORAL_PIC0_BELOW_IN_VALID)
                        | F_ENCODE(0, TOPAZHP_CR_TEMPORAL_PIC1_BELOW_IN_VALID)
                        | F_ENCODE(0, TOPAZHP_CR_ABOVE_OUT_OF_SLICE_VALID)
                        | F_ENCODE(((ui32Flags & ISINTERP_FLAGS) == ISINTERP_FLAGS), TOPAZHP_CR_WRITE_TEMPORAL_PIC0_BELOW_VALID)
                        | F_ENCODE(0, TOPAZHP_CR_REF_PIC0_VALID)
                        | F_ENCODE(0, TOPAZHP_CR_REF_PIC1_VALID)
                        | F_ENCODE(1, TOPAZHP_CR_REF_PIC1_EQUAL_PIC0)
                        | F_ENCODE(0, TOPAZHP_CR_FIELD_MODE)
                        | F_ENCODE(ui8SwapChromas, TOPAZHP_CR_FRAME_STORE_CHROMA_SWAP)
                        | F_ENCODE(ui32FrameStoreFormat, TOPAZHP_CR_FRAME_STORE_FORMAT)
                        | F_ENCODE(TOPAZHP_CR_ENCODER_STANDARD_H263, TOPAZHP_CR_ENCODER_STANDARD);
        break;
    }

    if (bIsBPicture)        {
        ui32SeqConfig |= F_ENCODE(0, TOPAZHP_CR_TEMPORAL_PIC1_BELOW_IN_VALID)
                         | F_ENCODE(0, TOPAZHP_CR_WRITE_TEMPORAL_PIC1_BELOW_VALID)
                         | F_ENCODE(1, TOPAZHP_CR_REF_PIC1_VALID)
                         | F_ENCODE(1, TOPAZHP_CR_TEMPORAL_COL_IN_VALID);
    }

    if (ctx->ui8EnableSelStatsFlags & ESF_FIRST_STAGE_STATS)        {
        ui32SeqConfig |= F_ENCODE(1, TOPAZHP_CR_WRITE_MB_FIRST_STAGE_VALID);
    }

    if (ctx->ui8EnableSelStatsFlags & ESF_MP_BEST_MB_DECISION_STATS ||
        ctx->ui8EnableSelStatsFlags & ESF_MP_BEST_MOTION_VECTOR_STATS)  {
        ui32SeqConfig |= F_ENCODE(1, TOPAZHP_CR_BEST_MULTIPASS_OUT_VALID);

        if (!(ctx->ui8EnableSelStatsFlags & ESF_MP_BEST_MOTION_VECTOR_STATS)) {
            ui32SeqConfig |= F_ENCODE(1, TOPAZHP_CR_BEST_MVS_OUT_DISABLE);// 64 Byte Best Multipass Motion Vector output disabled by default
        }
    }

    if (ctx->bEnableInpCtrl) {
        ui32SeqConfig |= F_ENCODE(1, TOPAZHP_CR_MB_CONTROL_IN_VALID);
    }

    if (eSliceType == IMG_FRAME_IDR) {
        ctx->sBiasTables.ui32SeqConfigInit = ui32SeqConfig;
    }

    tng__save_slice_params_template(ctx, ui32SliceBufIdx, eSliceType,
        ui32IPEControl, ui32Flags, ui32SliceConfig, ui32SeqConfig, ui32StreamIndex);

    return 0;
}

void tng__mpeg4_generate_pic_hdr_template(
    context_ENC_p ctx,
    IMG_FRAME_TEMPLATE_TYPE ui8SliceType,
    IMG_UINT8 ui8Search_range)
{
    context_ENC_mem *ps_mem = &(ctx->ctx_mem[ctx->ui32StreamID]);
    MTX_HEADER_PARAMS * pPicHeaderMem;
    VOP_CODING_TYPE eVop_Coding_Type;
    IMG_BOOL8 b8IsVopCoded;
    IMG_UINT8 ui8OriginalSliceType = ui8SliceType;

    /* MPEG4: We do not support B-frames at the moment, so we use a spare slot, to store a template for the skipped frame */
    if (ui8SliceType == IMG_FRAME_INTER_B)
    {
	ui8SliceType = IMG_FRAME_INTER_P;
	b8IsVopCoded = IMG_FALSE;
    } else {
	b8IsVopCoded = IMG_TRUE;
    }

    eVop_Coding_Type = (ui8SliceType == IMG_FRAME_INTER_P) ? P_FRAME : I_FRAME;

    psb_buffer_map(&(ps_mem->bufs_pic_template), &(ps_mem->bufs_pic_template.virtual_addr));
    if (ps_mem->bufs_pic_template.virtual_addr == NULL) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s error: mapping pic template\n", __FUNCTION__);
        return ;
    }

    pPicHeaderMem = (MTX_HEADER_PARAMS *)((IMG_UINT8*)(ps_mem->bufs_pic_template.virtual_addr + (ctx->ctx_mem_size.pic_template * ui8OriginalSliceType)));
    //todo fix time resolution
    tng__MPEG4_notforsims_prepare_vop_header(pPicHeaderMem, b8IsVopCoded, ui8Search_range, eVop_Coding_Type);
    psb_buffer_unmap(&(ps_mem->bufs_pic_template));

}

void tng__h263_generate_pic_hdr_template(
    context_ENC_p ctx,
    IMG_FRAME_TEMPLATE_TYPE eFrameType,
    IMG_UINT16 ui16Width,
    IMG_UINT16 ui16Heigh)

{
    context_ENC_mem *ps_mem = &(ctx->ctx_mem[ctx->ui32StreamID]);
    MTX_HEADER_PARAMS * pPicHeaderMem = NULL;
    H263_PICTURE_CODING_TYPE ePictureCodingType = ((eFrameType == IMG_FRAME_INTRA)|| (eFrameType == IMG_FRAME_IDR)) ? I_FRAME : P_FRAME;
       
    psb_buffer_map(&(ps_mem->bufs_pic_template), &(ps_mem->bufs_pic_template.virtual_addr));
    if (ps_mem->bufs_pic_template.virtual_addr == NULL) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s error: mapping pic template\n", __FUNCTION__);
        return ;
    }

    pPicHeaderMem = (MTX_HEADER_PARAMS *)((IMG_UINT8*)(ps_mem->bufs_pic_template.virtual_addr + (ctx->ctx_mem_size.pic_template * eFrameType)));

    IMG_UINT8 ui8FrameRate = (IMG_UINT8)ctx->sRCParams.ui32FrameRate;

    // Get a pointer to the memory the header will be written to
    tng__H263_notforsims_prepare_video_pictureheader(
        pPicHeaderMem,
        ePictureCodingType,
        ctx->ui8H263SourceFormat,
        ui8FrameRate,
        ui16Width,
        ui16Heigh);

    psb_buffer_unmap(&(ps_mem->bufs_pic_template));

}


static void tng__MPEG4ES_send_seq_header(context_ENC_p ctx, IMG_UINT32 ui32StreamIndex)
{
    context_ENC_mem *ps_mem = &(ctx->ctx_mem[ui32StreamIndex]);
    tng_cmdbuf_p cmdbuf = ctx->obj_context->tng_cmdbuf;

    psb_buffer_map(&(ps_mem->bufs_seq_header), &(ps_mem->bufs_seq_header.virtual_addr));
    if (ps_mem->bufs_seq_header.virtual_addr == NULL) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s error: mapping seq template\n", __FUNCTION__);
        return ;
    }

    tng__MPEG4_prepare_sequence_header(ps_mem->bufs_seq_header.virtual_addr,
                                       IMG_FALSE,//FIXME: Zhaohan bFrame
                                       ctx->ui8ProfileIdc,//profile
                                       ctx->ui8LevelIdc,//ui8Profile_lvl_indication
                                       3,//ui8Fixed_vop_time_increment
                                       ctx->obj_context->picture_width,//ui8Fixed_vop_time_increment
                                       ctx->obj_context->picture_height,//ui32Picture_Height_Pixels
                                       NULL,//VBVPARAMS
                                       ctx->ui32VopTimeResolution);
    psb_buffer_unmap(&(ps_mem->bufs_seq_header));

    cmdbuf->cmd_idx_saved[TNG_CMDBUF_SEQ_HEADER_IDX] = cmdbuf->cmd_idx;
}

static void tng__H264ES_send_seq_header(context_ENC_p ctx, IMG_UINT32 ui32StreamIndex)
{
    context_ENC_mem *ps_mem = &(ctx->ctx_mem[ui32StreamIndex]);
    tng_cmdbuf_p cmdbuf = ctx->obj_context->tng_cmdbuf;
    IMG_RC_PARAMS *psRCParams = &(ctx->sRCParams);
    H264_VUI_PARAMS *psVuiParams = &(ctx->sVuiParams);

//    memset(psVuiParams, 0, sizeof(H264_VUI_PARAMS));

    if (psRCParams->eRCMode != IMG_RCMODE_NONE) {
        psVuiParams->vui_flag = 1;
        if (psVuiParams->num_units_in_tick == 0 || psVuiParams->Time_Scale == 0) {
            psVuiParams->num_units_in_tick = 1;
            psVuiParams->Time_Scale = psRCParams->ui32FrameRate * 2;
        }
        psVuiParams->bit_rate_value_minus1 = psRCParams->ui32BitsPerSecond / 64 - 1;
        psVuiParams->cbp_size_value_minus1 = psRCParams->ui32BufferSize / 64 - 1;
        psVuiParams->CBR = ((psRCParams->eRCMode == IMG_RCMODE_CBR) && (!psRCParams->bDisableBitStuffing)) ? 1 : 0;
        psVuiParams->initial_cpb_removal_delay_length_minus1 = BPH_SEI_NAL_INITIAL_CPB_REMOVAL_DELAY_SIZE - 1;
        psVuiParams->cpb_removal_delay_length_minus1 = PTH_SEI_NAL_CPB_REMOVAL_DELAY_SIZE - 1;
        psVuiParams->dpb_output_delay_length_minus1 = PTH_SEI_NAL_DPB_OUTPUT_DELAY_SIZE - 1;
        psVuiParams->time_offset_length = 24;
    }
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s psVuiParams->vui_flag = %d\n", __FUNCTION__, psVuiParams->vui_flag);

    psb_buffer_map(&(ps_mem->bufs_seq_header), &(ps_mem->bufs_seq_header.virtual_addr));
    if (ps_mem->bufs_seq_header.virtual_addr == NULL) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s error: mapping seq header\n", __FUNCTION__);
        return ;
    }

    tng__H264ES_prepare_sequence_header(
        ps_mem->bufs_seq_header.virtual_addr,
        &(ctx->sVuiParams),
        &(ctx->sCropParams),
        ctx->ui16Width,         //ui8_picture_width_in_mbs
        ctx->ui16PictureHeight, //ui8_picture_height_in_mbs
        ctx->ui32CustomQuantMask,    //0,  ui8_custom_quant_mask
        ctx->ui8ProfileIdc,          //ui8_profile
        ctx->ui8LevelIdc,            //ui8_level
        ctx->ui8FieldCount,          //1,  ui8_field_count
        ctx->ui8MaxNumRefFrames,     //1,  ui8_max_num_ref_frames
        ctx->bPpsScaling,            //0   ui8_pps_scaling_cnt
        ctx->bUseDefaultScalingList, //0,  b_use_default_scaling_list
        ctx->bEnableLossless,        //0,  blossless
        ctx->bArbitrarySO
    );
    psb_buffer_unmap(&(ps_mem->bufs_seq_header));

    if (ctx->bEnableMVC) {
        psb_buffer_map(&(ps_mem->bufs_sub_seq_header), &(ps_mem->bufs_sub_seq_header.virtual_addr));
        if (ps_mem->bufs_sub_seq_header.virtual_addr == NULL) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "%s error: mapping sub seq header\n", __FUNCTION__);
            return ;
        }
        tng__H264ES_prepare_mvc_sequence_header(
            ps_mem->bufs_sub_seq_header.virtual_addr,
            &(ctx->sCropParams),
            ctx->ui16Width,         //ui8_picture_width_in_mbs
            ctx->ui16PictureHeight, //ui8_picture_height_in_mbs
            ctx->ui32CustomQuantMask,    //0,  ui8_custom_quant_mask
            ctx->ui8ProfileIdc,          //ui8_profile
            ctx->ui8LevelIdc,            //ui8_level
            ctx->ui8FieldCount,          //1,  ui8_field_count
            ctx->ui8MaxNumRefFrames,     //1,  ui8_max_num_ref_frames
            ctx->bPpsScaling,            //0   ui8_pps_scaling_cnt
            ctx->bUseDefaultScalingList, //0,  b_use_default_scaling_list
            ctx->bEnableLossless,        //0,  blossless
            ctx->bArbitrarySO
        );
        psb_buffer_unmap(&(ps_mem->bufs_sub_seq_header));
    }

    cmdbuf->cmd_idx_saved[TNG_CMDBUF_SEQ_HEADER_IDX] = cmdbuf->cmd_idx;

    return ;
}

static void tng__H264ES_send_pic_header(context_ENC_p ctx, IMG_UINT32 ui32StreamIndex)
{
    context_ENC_mem *ps_mem = &(ctx->ctx_mem[ui32StreamIndex]);
    IMG_BOOL bDepViewPPS = IMG_FALSE;

    if ((ctx->bEnableMVC) && (ctx->ui16MVCViewIdx != 0) &&
        (ctx->ui16MVCViewIdx != (IMG_UINT16)(NON_MVC_VIEW))) {
        bDepViewPPS = IMG_TRUE;
    }

    psb_buffer_map(&(ps_mem->bufs_pic_template), &(ps_mem->bufs_pic_template.virtual_addr));
    if (ps_mem->bufs_pic_template.virtual_addr == NULL) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s error: mapping pic template\n", __FUNCTION__);
        return ;
    }

    tng__H264ES_prepare_picture_header(
        ps_mem->bufs_pic_template.virtual_addr,
        ctx->bCabacEnabled,
        ctx->bH2648x8Transform,     //IMG_BOOL    b_8x8transform,
        ctx->bH264IntraConstrained, //IMG_BOOL    bIntraConstrained,
        0, //IMG_INT8    i8CQPOffset,
        0, //IMG_BOOL    bWeightedPrediction,
        0, //IMG_UINT8   ui8WeightedBiPred,
        bDepViewPPS, //IMG_BOOL    bMvcPPS,
        0, //IMG_BOOL    bScalingMatrix,
        0  //IMG_BOOL    bScalingLists
    );

    psb_buffer_unmap(&(ps_mem->bufs_pic_template));
    return ;
}

static void tng__H264ES_send_hrd_header(context_ENC_p ctx, IMG_UINT32 ui32StreamIndex)
{
    unsigned int ui32nal_initial_cpb_removal_delay;
    unsigned int ui32nal_initial_cpb_removal_delay_offset;
    uint32_t ui32cpb_removal_delay;
    IMG_RC_PARAMS *psRCParams = &(ctx->sRCParams);
    context_ENC_mem *ps_mem = &(ctx->ctx_mem[ui32StreamIndex]);
    H264_VUI_PARAMS *psVuiParams = &(ctx->sVuiParams);
    IMG_UINT8 aui8clocktimestampflag[1];
    aui8clocktimestampflag[0] = IMG_FALSE;

    ui32nal_initial_cpb_removal_delay =
        90000 * (1.0 * psRCParams->i32InitialDelay / psRCParams->ui32BitsPerSecond);
    ui32nal_initial_cpb_removal_delay_offset =
        90000 * (1.0 * ctx->buffer_size / psRCParams->ui32BitsPerSecond)
        - ui32nal_initial_cpb_removal_delay;

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "Insert SEI buffer period message with "
            "ui32nal_initial_cpb_removal_delay(%d) and "
            "ui32nal_initial_cpb_removal_delay_offset(%d)\n",
            ui32nal_initial_cpb_removal_delay,
            ui32nal_initial_cpb_removal_delay_offset);

    psb_buffer_map(&(ps_mem->bufs_sei_header), &(ps_mem->bufs_sei_header.virtual_addr));
    if (ps_mem->bufs_sei_header.virtual_addr == NULL) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s error: mapping sei header\n", __FUNCTION__);
        return ;
    }

    if ((!ctx->bEnableMVC) || (ctx->ui16MVCViewIdx == 0)) {
        tng__H264ES_prepare_AUD_header(ps_mem->bufs_sei_header.virtual_addr);
    }
    
    tng__H264ES_prepare_SEI_buffering_period_header(
        ps_mem->bufs_sei_header.virtual_addr + (ctx->ctx_mem_size.sei_header),
        0,// ui8cpb_cnt_minus1,
        psVuiParams->initial_cpb_removal_delay_length_minus1+1, //ui8initial_cpb_removal_delay_length,
        1, //ui8NalHrdBpPresentFlag,
        ui32nal_initial_cpb_removal_delay, // ui32nal_initial_cpb_removal_delay,
        ui32nal_initial_cpb_removal_delay_offset, //ui32nal_initial_cpb_removal_delay_offset,
        0, //ui8VclHrdBpPresentFlag - CURRENTLY HARD CODED TO ZERO IN TOPAZ
        NOT_USED_BY_TOPAZ, // ui32vcl_initial_cpb_removal_delay, (not used when ui8VclHrdBpPresentFlag = 0)
        NOT_USED_BY_TOPAZ); // ui32vcl_initial_cpb_removal_delay_offset (not used when ui8VclHrdBpPresentFlag = 0)

    /* ui32cpb_removal_delay is zero for 1st frame and will be reset
     * after a IDR frame */
    if (ctx->ui32FrameCount[ui32StreamIndex] == 0) {
        if (ctx->ui32RawFrameCount == 0)
            ui32cpb_removal_delay = 0;
        else
            ui32cpb_removal_delay =
                ctx->ui32IdrPeriod * ctx->ui32IntraCnt * 2;
    } else
        ui32cpb_removal_delay = 2 * ctx->ui32FrameCount[ui32StreamIndex];

    tng__H264ES_prepare_SEI_picture_timing_header(
        ps_mem->bufs_sei_header.virtual_addr + (ctx->ctx_mem_size.sei_header * 2),
        1, //ui8CpbDpbDelaysPresentFlag,
        psVuiParams->cpb_removal_delay_length_minus1, //cpb_removal_delay_length_minus1,
        psVuiParams->dpb_output_delay_length_minus1, //dpb_output_delay_length_minus1,
        ui32cpb_removal_delay, //ui32cpb_removal_delay,
        2, //ui32dpb_output_delay,
        0, //ui8pic_struct_present_flag (contained in the sequence header, Topaz hard-coded default to 0)
        NOT_USED_BY_TOPAZ, //ui8pic_struct, (not used when ui8pic_struct_present_flag = 0)
        NOT_USED_BY_TOPAZ, //NumClockTS, (not used when ui8pic_struct_present_flag = 0)
        aui8clocktimestampflag, //abclock_timestamp_flag, (not used when ui8pic_struct_present_flag = 0)
        NOT_USED_BY_TOPAZ, //ui8full_timestamp_flag, (not used when ui8pic_struct_present_flag = 0)
        NOT_USED_BY_TOPAZ, //ui8seconds_flag, (not used when ui8pic_struct_present_flag = 0)
        NOT_USED_BY_TOPAZ, //ui8minutes_flag, (not used when ui8pic_struct_present_flag = 0)
        NOT_USED_BY_TOPAZ, //ui8hours_flag, (not used when ui8pic_struct_present_flag = 0)
        NOT_USED_BY_TOPAZ, //seconds_value, (not used when ui8pic_struct_present_flag = 0)
        NOT_USED_BY_TOPAZ, //minutes_value, (not used when ui8pic_struct_present_flag = 0)
        NOT_USED_BY_TOPAZ, //hours_value, (not used when ui8pic_struct_present_flag = 0)
        NOT_USED_BY_TOPAZ, //ct_type (2=Unknown) See TRM Table D 2 ?Mapping of ct_type to source picture scan  (not used when ui8pic_struct_present_flag = 0)
        NOT_USED_BY_TOPAZ, //nuit_field_based_flag, (not used when ui8pic_struct_present_flag = 0)
        NOT_USED_BY_TOPAZ, //counting_type (See TRM Table D 3 ?Definition of counting_type values)  (not used when ui8pic_struct_present_flag = 0)
        NOT_USED_BY_TOPAZ, //ui8discontinuity_flag, (not used when ui8pic_struct_present_flag = 0)
        NOT_USED_BY_TOPAZ, //ui8cnt_dropped_flag, (not used when ui8pic_struct_present_flag = 0)
        NOT_USED_BY_TOPAZ, //n_frames, (not used when ui8pic_struct_present_flag = 0)
        NOT_USED_BY_TOPAZ, //time_offset_length, (not used when ui8pic_struct_present_flag = 0)
        NOT_USED_BY_TOPAZ); //time_offset (not used when ui8pic_struct_present_flag = 0)
    psb_buffer_unmap(&(ps_mem->bufs_sei_header));

    return ;
}

static void tng__generate_slice_params_template(
    context_ENC_p ctx,
    IMG_UINT32 slice_buf_idx,
    IMG_UINT32 slice_type,
    IMG_UINT32 ui32StreamIndex
)
{
    context_ENC_mem *ps_mem = &(ctx->ctx_mem[ui32StreamIndex]);
    IMG_UINT8  *slice_mem_temp_p = NULL;
    IMG_UINT32 ui32SliceHeight = 0;
    IMG_FRAME_TEMPLATE_TYPE slice_temp_type = (IMG_FRAME_TEMPLATE_TYPE)slice_type;
    IMG_FRAME_TEMPLATE_TYPE buf_idx = (IMG_FRAME_TEMPLATE_TYPE)slice_buf_idx;

    if (ctx->ui8SlicesPerPicture != 0)
        ui32SliceHeight = ctx->ui16PictureHeight / ctx->ui8SlicesPerPicture;
    else
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s slice height\n", __FUNCTION__);
    
    ui32SliceHeight &= ~15;

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s PTG ui8DeblockIDC    = %x\n", __FUNCTION__, ctx->ui8DeblockIDC   );
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s PTG ui32SliceHeight  = %x\n", __FUNCTION__, ui32SliceHeight );
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s PTG bIsInterlaced    = %x\n", __FUNCTION__, ctx->bIsInterlaced   );
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s PTG iFineYSearchSize = %x\n", __FUNCTION__, ctx->iFineYSearchSize);

    tng__prepare_encode_sliceparams(
        ctx,
        slice_buf_idx,
        slice_temp_type,
        0,                        // ui16CurrentRow,
        ui32SliceHeight,
        ctx->ui8DeblockIDC,       // uiDeblockIDC
        ctx->bIsInterlaced,       // bFieldMode
        ctx->iFineYSearchSize,
        ui32StreamIndex
    );

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s PTG bCabacEnabled  = %x\n", __FUNCTION__, ctx->bCabacEnabled );
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s PTG ui16MVCViewIdx = %x\n", __FUNCTION__, ctx->ui16MVCViewIdx);

    if(ctx->bEnableMVC)
        ctx->ui16MVCViewIdx = (IMG_UINT16)ui32StreamIndex;

    if (ps_mem->bufs_slice_template.virtual_addr == NULL) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s error: mapping slice template\n", __FUNCTION__);
        return ;
    }

    slice_mem_temp_p = (IMG_UINT8*)(ps_mem->bufs_slice_template.virtual_addr + (ctx->ctx_mem_size.slice_template * buf_idx));
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s: addr 0x%08x, virtual 0x%08x, size = 0x%08x, buf_idx = %x\n",
        __FUNCTION__, slice_mem_temp_p, ps_mem->bufs_slice_template.virtual_addr, ctx->ctx_mem_size.slice_template, buf_idx);

    /* Prepare Slice Header Template */
    switch (ctx->eStandard) {
    case IMG_STANDARD_NONE:
    case IMG_STANDARD_JPEG:
    case IMG_STANDARD_MPEG4:
        break;
    case IMG_STANDARD_H264:
        //H264_NOTFORSIMS_PrepareSliceHeader
        tng__H264ES_notforsims_prepare_sliceheader(
            slice_mem_temp_p,
            slice_temp_type,
            ctx->ui8DeblockIDC,
            0,                   // ui32FirstMBAddress
            0,                   // uiMBSkipRun
            ctx->bCabacEnabled,
            ctx->bIsInterlaced,
            ctx->ui16MVCViewIdx, //(IMG_UINT16)(NON_MVC_VIEW);
            IMG_FALSE            // bIsLongTermRef
        );
        break;

    case IMG_STANDARD_H263:
        tng__H263ES_notforsims_prepare_gobsliceheader(slice_mem_temp_p);
        break;

    case IMG_STANDARD_MPEG2:
        tng__MPEG2_prepare_sliceheader(slice_mem_temp_p);
        break;
    }

    psb_buffer_unmap(&(ps_mem->bufs_slice_template));

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s: end \n", __FUNCTION__);

    return ;
}

//H264_PrepareTemplates
static VAStatus tng__prepare_templates(context_ENC_p ctx, IMG_UINT32 ui32StreamIndex)
{
    IMG_RC_PARAMS *psRCParams = &(ctx->sRCParams);
    PIC_PARAMS *psPicParams = &(ctx->sPicParams);
    IN_RC_PARAMS* psInParams = &(psPicParams->sInParams);
    psPicParams->ui32Flags = 0;

    tng__prepare_mv_estimates(ctx, ui32StreamIndex);

    switch (ctx->eStandard) {
        case IMG_STANDARD_H263:
            psPicParams->ui32Flags |= ISH263_FLAGS;
            break;
        case IMG_STANDARD_MPEG4:
            psPicParams->ui32Flags |= ISMPEG4_FLAGS;
            break;
        case IMG_STANDARD_MPEG2:
            psPicParams->ui32Flags |= ISMPEG2_FLAGS;
            break;
        default:
            break;
    }

    if (psRCParams->eRCMode) {
        psPicParams->ui32Flags |= ISRC_FLAGS;
        tng__setup_rcdata(ctx);
    } else {
        psInParams->ui8SeInitQP  = psRCParams->ui32InitialQp;
        psInParams->ui8MBPerRow  = (ctx->ui16Width >> 4);
        psInParams->ui16MBPerFrm = (ctx->ui16Width >> 4) * (ctx->ui16PictureHeight >> 4);
        psInParams->ui16MBPerBU  = psRCParams->ui32BUSize;
        psInParams->ui16BUPerFrm = (psInParams->ui16MBPerFrm) / psRCParams->ui32BUSize;
        ctx->ui32KickSize = psInParams->ui16MBPerBU;
    }

    // Prepare Slice header templates
    tng__generate_slice_params_template(ctx, (IMG_UINT32)IMG_FRAME_IDR, (IMG_UINT32)IMG_FRAME_IDR, ui32StreamIndex);
    tng__generate_slice_params_template(ctx, (IMG_UINT32)IMG_FRAME_INTRA, (IMG_UINT32)IMG_FRAME_INTRA, ui32StreamIndex);
    tng__generate_slice_params_template(ctx, (IMG_UINT32)IMG_FRAME_INTER_P, (IMG_UINT32)IMG_FRAME_INTER_P, ui32StreamIndex);
    switch(ctx->eStandard) {
	case IMG_STANDARD_H264:
	    tng__generate_slice_params_template(ctx, (IMG_UINT32)IMG_FRAME_INTER_B, (IMG_UINT32)IMG_FRAME_INTER_B, ui32StreamIndex);
	    if (ctx->bEnableMVC)
		tng__generate_slice_params_template(ctx, (IMG_UINT32)IMG_FRAME_INTER_P_IDR, (IMG_UINT32)IMG_FRAME_INTER_P_IDR, ui32StreamIndex);
            tng__H264ES_send_seq_header(ctx, 0);
            tng__H264ES_send_pic_header(ctx, 0);
            if (ctx->bInsertHRDParams)
                tng__H264ES_send_hrd_header(ctx, 0);
	    break;
	case IMG_STANDARD_H263:
	    tng__generate_slice_params_template(ctx, (IMG_UINT32)IMG_FRAME_INTER_B, (IMG_UINT32)IMG_FRAME_INTER_B, ui32StreamIndex);
	    /* Here H263 uses the actual width and height */
	    tng__h263_generate_pic_hdr_template(ctx, IMG_FRAME_IDR, ctx->h263_actual_width, ctx->h263_actual_height);
            tng__h263_generate_pic_hdr_template(ctx, IMG_FRAME_INTRA, ctx->h263_actual_width, ctx->h263_actual_height);
            tng__h263_generate_pic_hdr_template(ctx, IMG_FRAME_INTER_P, ctx->h263_actual_width, ctx->h263_actual_height);
	    break;
	case IMG_STANDARD_MPEG4:
	    tng__generate_slice_params_template(ctx, (IMG_UINT32)IMG_FRAME_INTER_B, (IMG_UINT32)IMG_FRAME_INTER_P, ui32StreamIndex);
	    tng__mpeg4_generate_pic_hdr_template(ctx, IMG_FRAME_IDR, 4);
            tng__mpeg4_generate_pic_hdr_template(ctx, IMG_FRAME_INTRA, 4);
            tng__mpeg4_generate_pic_hdr_template(ctx, IMG_FRAME_INTER_P, 4);
	    tng__mpeg4_generate_pic_hdr_template(ctx, IMG_FRAME_INTER_B, 4);
	    break;
	default:
	    break;
    }

    //FIXME: Zhaohan tng__mpeg2/mpeg4_generate_pic_hdr_template(IMG_FRAME_IDR\IMG_FRAME_INTRA\IMG_FRAME_INTER_P\IMG_FRAME_INTER_B);
 
/*
    else {
        slice_mem_temp_p = (IMG_UINT8*)cmdbuf->slice_mem_p + (((IMG_UINT32)IMG_FRAME_INTER_P_IDR) * cmdbuf->mem_size);
        memset(slice_mem_temp_p, 0, 128);
    }
*/
    // Prepare Pic Params Templates
    tng__adjust_picflags(ctx, psRCParams, IMG_TRUE, &(ctx->ui32FirstPicFlags));
    tng__adjust_picflags(ctx, psRCParams, IMG_FALSE, &(ctx->ui32NonFirstPicFlags));

    return VA_STATUS_SUCCESS;
}

#if INPUT_SCALER_SUPPORTED
static IMG_FLOAT VIDEO_CalculateBessel0 (IMG_FLOAT fX)
{
    IMG_FLOAT fAX, fY;

    fAX = (IMG_FLOAT)IMG_FABS(fX);

    if (fAX < 3.75) 	{
        fY = (IMG_FLOAT)(fX / 3.75);
        fY *= fY;

        return (IMG_FLOAT)(1.0 + fY *
            (3.5156229 + fY *
            (3.0899424 + fY *
            (1.2067492 + fY *
            (0.2659732 + fY *
            (0.360768e-1 + fY * 0.45813e-2))))));
    }

    fY = (IMG_FLOAT)(3.75 / fAX);

    return (IMG_FLOAT)((IMG_EXP(fAX) / IMG_SQRT(fAX)) *
        (0.39894228 + fY *
        (0.1328592e-1 + fY *
        (0.225319e-2 + fY *
        (-0.157565e-2 + fY *
        (0.916281e-2 + fY *
        (-0.2057706e-1 + fY *
        (0.2635537e-1 + fY *
        (-0.1647633e-1 + fY * 0.392377e-2)))))))));
}

static IMG_FLOAT VIDEO_SincFunc (IMG_FLOAT fInput, IMG_FLOAT fScale)
{
    IMG_FLOAT fX;
    IMG_FLOAT fKaiser;

    /* Kaiser window */
    fX = fInput / (4.0f / 2.0f) - 1.0f;
    fX = (IMG_FLOAT)IMG_SQRT(1.0f - fX * fX);
    fKaiser = VIDEO_CalculateBessel0(2.0f * fX) / VIDEO_CalculateBessel0(2.0f);

    /* Sinc function */
    fX = 4.0f / 2.0f - fInput;
    if (fX == 0) {
        return fKaiser;
    }

    fX *= 0.9f * fScale * 3.1415926535897f;

    return fKaiser * (IMG_FLOAT)(IMG_SIN(fX) / fX);
}

static void VIDEO_CalcCoefs_FromPitch (IMG_FLOAT	fPitch, IMG_UINT8 aui8Table[4][16])
{
    /* Based on sim code */
    /* The function is symmetrical, so we only need to calculate the first half of the taps, and the middle value. */

    IMG_FLOAT	fScale;
    IMG_UINT32	ui32I, ui32Tap;
    IMG_FLOAT	afTable[4][16];
    IMG_INT32	i32Total;
    IMG_FLOAT	fTotal;
    IMG_INT32	i32MiddleTap, i32MiddleI;		/* Mirrored / middle Values for I and T */

    if (fPitch < 1.0f) {
        fScale = 1.0f;
    } else {
        fScale = 1.0f / fPitch;
    }

    for (ui32I = 0; ui32I < 16; ui32I++) {
        for (ui32Tap = 0; ui32Tap < 4; ui32Tap++) {
            afTable[ui32Tap][ui32I] = VIDEO_SincFunc(((IMG_FLOAT)ui32Tap) + ((IMG_FLOAT)ui32I) / 16.0f, fScale);
        } 
    }

    for (ui32Tap = 0; ui32Tap < 2; ui32Tap++) {
        for (ui32I = 0; ui32I < 16; ui32I++) {
            /* Copy the table around the centre point */
            i32MiddleTap = (3 - ui32Tap) + (16 - ui32I) / 16;
            i32MiddleI = (16 - ui32I) & 15;
            if ((IMG_UINT32)i32MiddleTap < 4) {
                afTable[i32MiddleTap][i32MiddleI] = afTable[ui32Tap][ui32I];
            }
        }
    }

    /* The middle value */
    afTable[2][0] = VIDEO_SincFunc(2.0f, fScale);
	
    /* Normalize this interpolation point, and convert to 2.6 format, truncating the result	*/
    for (ui32I = 0; ui32I < 16; ui32I++) {
        fTotal = 0.0f;
        i32Total = 0;

        for (ui32Tap = 0; ui32Tap < 4; ui32Tap++) {
            fTotal += afTable[ui32Tap][ui32I];
        }

        for (ui32Tap = 0; ui32Tap < 4; ui32Tap++) {
            aui8Table[ui32Tap][ui32I] = (IMG_UINT8)((afTable[ui32Tap][ui32I] * 64.0f) / fTotal);
            i32Total += aui8Table[ui32Tap][ui32I];
        }

        if (ui32I <= 8) { /* normalize any floating point errors */
            i32Total -= 64;
            if (ui32I == 8) {
                i32Total /= 2;
            }
            /* Subtract the error from the I Point in the first tap ( this will not get
            mirrored, as it would go off the end). */
            aui8Table[0][ui32I] = (IMG_UINT8)(aui8Table[0][ui32I] - (IMG_UINT8)i32Total); 
        }
    }

    /* Copy the normalised table around the centre point */
    for (ui32Tap = 0; ui32Tap < 2; ui32Tap++) {
        for (ui32I = 0; ui32I < 16; ui32I++) {
            i32MiddleTap = (3 - ui32Tap) + (16 - ui32I) / 16;
            i32MiddleI = (16 - ui32I) & 15;
            if ((IMG_UINT32)i32MiddleTap < 4) {
                aui8Table[i32MiddleTap][i32MiddleI] = aui8Table[ui32Tap][ui32I];
            }
        }
    }
    return ;
}
#endif

static void tng__setvideo_params(context_ENC_p ctx, IMG_UINT32 ui32StreamIndex)
{
    context_ENC_mem *ps_mem = &(ctx->ctx_mem[ui32StreamIndex]);
    context_ENC_frame_buf *ps_buf = &(ctx->ctx_frame_buf);
    IMG_MTX_VIDEO_CONTEXT* psMtxEncContext = NULL;
    IMG_RC_PARAMS * psRCParams = &(ctx->sRCParams);
    //IMG_UINT16 ui16WidthInMbs = (ctx->ui16Width + 15) >> 4;
    //IMG_UINT16 ui16FrameHeightInMbs = (ctx->ui16FrameHeight + 15) >> 4;
    IMG_INT nIndex;
    IMG_UINT8 ui8Flag;
#ifndef EXCLUDE_ADAPTIVE_ROUNDING
    IMG_INT32 i, j;
#endif

    psb_buffer_map(&(ps_mem->bufs_mtx_context), &(ps_mem->bufs_mtx_context.virtual_addr));
    if (ps_mem->bufs_mtx_context.virtual_addr == NULL) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s error: mapping slice template\n", __FUNCTION__);
        return ;
    }

    psMtxEncContext = (IMG_MTX_VIDEO_CONTEXT*)(ps_mem->bufs_mtx_context.virtual_addr);

    ctx->i32PicNodes = (psRCParams->b16Hierarchical ? MAX_REF_B_LEVELS : 0) + ctx->ui8RefSpacing + 4;
    ctx->i32MVStores = (ctx->i32PicNodes * 2);
    ctx->ui8SlotsInUse = psRCParams->ui16BFrames + 2;

    psMtxEncContext->ui32InitialQp = ctx->sRCParams.ui32InitialQp;
    psMtxEncContext->ui32BUSize = ctx->sRCParams.ui32BUSize;
    psMtxEncContext->ui16CQPOffset = (ctx->sRCParams.i8QCPOffset & 0x1f) | ((ctx->sRCParams.i8QCPOffset & 0x1f) << 8);
    psMtxEncContext->eStandard = ctx->eStandard;
    psMtxEncContext->ui32WidthInMbs = ctx->ui16Width >> 4;
    psMtxEncContext->ui32PictureHeightInMbs = ctx->ui16PictureHeight >> 4;
    psMtxEncContext->bOutputReconstructed = (ps_buf->rec_surface != NULL) ? IMG_TRUE : IMG_FALSE;
    psMtxEncContext->ui32VopTimeResolution = ctx->ui32VopTimeResolution;
    psMtxEncContext->ui8MaxSlicesPerPicture = ctx->ui8SlicesPerPicture;
    psMtxEncContext->ui8NumPipes = ctx->ui8PipesToUse;
    psMtxEncContext->eFormat = ctx->eFormat;

    psMtxEncContext->b8IsInterlaced = ctx->bIsInterlaced;
    psMtxEncContext->b8TopFieldFirst = ctx->bTopFieldFirst;
    psMtxEncContext->b8ArbitrarySO = ctx->bArbitrarySO;

    psMtxEncContext->ui32IdrPeriod = ctx->ui32IdrPeriod * ctx->ui32IntraCnt;
    psMtxEncContext->ui32BFrameCount = ctx->sRCParams.ui16BFrames;
    psMtxEncContext->b8Hierarchical = (IMG_BOOL8) ctx->sRCParams.b16Hierarchical;
    psMtxEncContext->ui32IntraLoopCnt = ctx->ui32IntraCnt;
    psMtxEncContext->ui8RefSpacing = ctx->ui8RefSpacing;
    psMtxEncContext->ui32DebugCRCs = ctx->ui32DebugCRCs;

    psMtxEncContext->ui8FirstPipe = ctx->ui8BasePipe;
    psMtxEncContext->ui8LastPipe = ctx->ui8BasePipe + ctx->ui8PipesToUse - 1;
    psMtxEncContext->ui8PipesToUseFlags = 0;
    ui8Flag = 0x1 << psMtxEncContext->ui8FirstPipe;
    for (nIndex = 0; nIndex < psMtxEncContext->ui8NumPipes; nIndex++, ui8Flag<<=1)
        psMtxEncContext->ui8PipesToUseFlags |= ui8Flag; //Pipes used MUST be contiguous from the BasePipe offset

    // Only used in MPEG2, 2 bit field (0 = 8 bit, 1 = 9 bit, 2 = 10 bit and 3=11 bit precision)
    if (ctx->eStandard == IMG_STANDARD_MPEG2)
        psMtxEncContext->ui8MPEG2IntraDCPrecision = ctx->ui8MPEG2IntraDCPrecision;

    psMtxEncContext->b16EnableMvc = ctx->bEnableMVC;
    psMtxEncContext->ui16MvcViewIdx = ctx->ui16MVCViewIdx;
    if (ctx->eStandard == IMG_STANDARD_H264)
        psMtxEncContext->b16NoSequenceHeaders = ctx->bNoSequenceHeaders;

    {
        IMG_UINT16 ui16SrcYStride = 0, ui16SrcUVStride = 0;
        // 3 Components: Y, U, V
        ctx->ui16BufferStride = ps_buf->src_surface->psb_surface->stride;
        ui16SrcUVStride = ui16SrcYStride = ctx->ui16BufferStride;
        ctx->ui16BufferHeight = ctx->ui16FrameHeight;
        switch (ctx->eFormat) {
        case IMG_CODEC_YUV:
        case IMG_CODEC_PL8:
        case IMG_CODEC_YV12:
            ui16SrcUVStride = ui16SrcYStride / 2;
            break;

        case IMG_CODEC_PL12:            // Interleaved chroma pixels
        case IMG_CODEC_IMC2:            // Interleaved chroma rows
        case IMG_CODEC_422_YUV:         // Odd-numbered chroma rows unused
        case IMG_CODEC_422_YV12:        // Odd-numbered chroma rows unused
        case IMG_CODEC_422_PL8:         // Odd-numbered chroma rows unused
        case IMG_CODEC_Y0UY1V_8888: // Interleaved luma and chroma pixels
        case IMG_CODEC_Y0VY1U_8888: // Interleaved luma and chroma pixels
        case IMG_CODEC_UY0VY1_8888: // Interleaved luma and chroma pixels
        case IMG_CODEC_VY0UY1_8888: // Interleaved luma and chroma pixels
            ui16SrcUVStride = ui16SrcYStride;
            break;

        case IMG_CODEC_422_IMC2:        // Interleaved chroma pixels and unused odd-numbered chroma rows
        case IMG_CODEC_422_PL12:        // Interleaved chroma rows and unused odd-numbered chroma rows
            ui16SrcUVStride = ui16SrcYStride * 2;
            break;

        default:
            break;
        }

        if ((ctx->bIsInterlaced) && (ctx->bIsInterleaved)) {
            ui16SrcYStride *= 2;                    // ui16SrcYStride
            ui16SrcUVStride *= 2;           // ui16SrcUStride
        }

        psMtxEncContext->ui32PicRowStride = F_ENCODE(ui16SrcYStride >> 6, TOPAZHP_CR_CUR_PIC_LUMA_STRIDE) |
                                            F_ENCODE(ui16SrcUVStride >> 6, TOPAZHP_CR_CUR_PIC_CHROMA_STRIDE);
    }

    psMtxEncContext->eRCMode = ctx->sRCParams.eRCMode;
    psMtxEncContext->b8DisableBitStuffing = ctx->sRCParams.bDisableBitStuffing;
    psMtxEncContext->b8FirstPic = IMG_TRUE;

    /*Contents Adaptive Rate Control Parameters*/
    psMtxEncContext->bCARC          =  ctx->sCARCParams.bCARC;
    psMtxEncContext->iCARCBaseline  =  ctx->sCARCParams.i32CARCBaseline;
    psMtxEncContext->uCARCThreshold =  ctx->sCARCParams.ui32CARCThreshold;
    psMtxEncContext->uCARCCutoff    =  ctx->sCARCParams.ui32CARCCutoff;
    psMtxEncContext->uCARCNegRange  =  ctx->sCARCParams.ui32CARCNegRange;
    psMtxEncContext->uCARCNegScale  =  ctx->sCARCParams.ui32CARCNegScale;
    psMtxEncContext->uCARCPosRange  =  ctx->sCARCParams.ui32CARCPosRange;
    psMtxEncContext->uCARCPosScale  =  ctx->sCARCParams.ui32CARCPosScale;
    psMtxEncContext->uCARCShift     =  ctx->sCARCParams.ui32CARCShift;
    psMtxEncContext->ui32MVClip_Config =  F_ENCODE(ctx->bNoOffscreenMv, TOPAZHP_CR_MVCALC_RESTRICT_PICTURE);
    psMtxEncContext->ui32LRITC_Tile_Use_Config = 0;
    psMtxEncContext->ui32LRITC_Cache_Chunk_Config = 0;

    psMtxEncContext->ui32IPCM_0_Config = F_ENCODE(ctx->ui32CabacBinFlex, TOPAZ_VLC_CR_CABAC_BIN_FLEX) |
        F_ENCODE(DEFAULT_CABAC_DB_MARGIN, TOPAZ_VLC_CR_CABAC_DB_MARGIN);

    psMtxEncContext->ui32IPCM_1_Config = F_ENCODE(3200, TOPAZ_VLC_CR_IPCM_THRESHOLD) |
        F_ENCODE(ctx->ui32CabacBinLimit, TOPAZ_VLC_CR_CABAC_BIN_LIMIT);

    // leave alone until high profile and constrained modes are defined.
    psMtxEncContext->ui32H264CompControl  = F_ENCODE((ctx->bCabacEnabled ? 0 : 1), TOPAZHP_CR_H264COMP_8X8_CAVLC);  

    psMtxEncContext->ui32H264CompControl |= F_ENCODE(ctx->bUseDefaultScalingList ? 1 : 0, TOPAZHP_CR_H264COMP_DEFAULT_SCALING_LIST);

    psMtxEncContext->ui32H264CompControl |= F_ENCODE(ctx->bH2648x8Transform ? 1 : 0, TOPAZHP_CR_H264COMP_8X8_TRANSFORM);

    psMtxEncContext->ui32H264CompControl |= F_ENCODE(ctx->bH264IntraConstrained ? 1 : 0, TOPAZHP_CR_H264COMP_CONSTRAINED_INTRA);


#ifndef EXCLUDE_ADAPTIVE_ROUNDING
    psMtxEncContext->bMCAdaptiveRoundingDisable = ctx->bVPAdaptiveRoundingDisable;
    psMtxEncContext->ui32H264CompControl |= F_ENCODE(psMtxEncContext->bMCAdaptiveRoundingDisable ? 0 : 1, TOPAZHP_CR_H264COMP_ADAPT_ROUND_ENABLE);

    if (!psMtxEncContext->bMCAdaptiveRoundingDisable)
        for (i = 0; i < 4; i++)
            for (j = 0; j < 18; j++)
                psMtxEncContext->ui16MCAdaptiveRoundingOffsets[j][i] = H264_ROUNDING_OFFSETS[j][i];
#endif

    if ((ctx->eStandard == IMG_STANDARD_H264) && (ctx->ui32CoreRev >= MIN_34_REV)) {
        psMtxEncContext->ui32H264CompControl |= F_ENCODE(USE_VCM_HW_SUPPORT, TOPAZHP_CR_H264COMP_VIDEO_CONF_ENABLE);
    }

    psMtxEncContext->ui32H264CompControl |=
           F_ENCODE(ctx->ui16UseCustomScalingLists & 0x01 ? 1 : 0, TOPAZHP_CR_H264COMP_CUSTOM_QUANT_4X4_INTRA_LUMA_ENABLE)
        | F_ENCODE(ctx->ui16UseCustomScalingLists & 0x02 ? 1 : 0, TOPAZHP_CR_H264COMP_CUSTOM_QUANT_4X4_INTRA_CB_ENABLE)
        | F_ENCODE(ctx->ui16UseCustomScalingLists & 0x04 ? 1 : 0, TOPAZHP_CR_H264COMP_CUSTOM_QUANT_4X4_INTRA_CR_ENABLE)
        | F_ENCODE(ctx->ui16UseCustomScalingLists & 0x08 ? 1 : 0, TOPAZHP_CR_H264COMP_CUSTOM_QUANT_4X4_INTER_LUMA_ENABLE)
        | F_ENCODE(ctx->ui16UseCustomScalingLists & 0x10 ? 1 : 0, TOPAZHP_CR_H264COMP_CUSTOM_QUANT_4X4_INTER_CB_ENABLE)
        | F_ENCODE(ctx->ui16UseCustomScalingLists & 0x20 ? 1 : 0, TOPAZHP_CR_H264COMP_CUSTOM_QUANT_4X4_INTER_CR_ENABLE)
        | F_ENCODE(ctx->ui16UseCustomScalingLists & 0x40 ? 1 : 0, TOPAZHP_CR_H264COMP_CUSTOM_QUANT_8X8_INTRA_LUMA_ENABLE)
        | F_ENCODE(ctx->ui16UseCustomScalingLists & 0x80 ? 1 : 0, TOPAZHP_CR_H264COMP_CUSTOM_QUANT_8X8_INTER_LUMA_ENABLE);

    psMtxEncContext->ui32H264CompControl |=
           F_ENCODE(ctx->bEnableLossless ? 1 : 0, INTEL_H264_LL)
        | F_ENCODE(ctx->bLossless8x8Prefilter ? 1 : 0, INTEL_H264_LL8X8P);

    psMtxEncContext->ui32H264CompIntraPredModes = 0x3ffff;// leave at default for now.
    psMtxEncContext->ui32PredCombControl = ctx->ui32PredCombControl;
    psMtxEncContext->ui32SkipCodedInterIntra = F_ENCODE(ctx->ui8InterIntraIndex, TOPAZHP_CR_INTER_INTRA_SCALE_IDX)
        |F_ENCODE(ctx->ui8CodedSkippedIndex, TOPAZHP_CR_SKIPPED_CODED_SCALE_IDX);

    if (ctx->bEnableInpCtrl) {
        psMtxEncContext->ui32MBHostCtrl = F_ENCODE(ctx->bEnableHostQP, TOPAZHP_CR_MB_HOST_QP)
            |F_ENCODE(ctx->bEnableHostBias, TOPAZHP_CR_MB_HOST_SKIPPED_CODED_SCALE)
            |F_ENCODE(ctx->bEnableHostBias, TOPAZHP_CR_MB_HOST_INTER_INTRA_SCALE);
        psMtxEncContext->ui32PredCombControl |= F_ENCODE(1, TOPAZHP_CR_INTER_INTRA_SCALE_ENABLE)
            |F_ENCODE(1, TOPAZHP_CR_SKIPPED_CODED_SCALE_ENABLE);
    }

    if (ctx->bEnableCumulativeBiases)
        psMtxEncContext->ui32PredCombControl |= F_ENCODE(1, TOPAZHP_CR_CUMULATIVE_BIASES_ENABLE);

    psMtxEncContext->ui32PredCombControl |=
        F_ENCODE((((ctx->ui8InterIntraIndex == 3) && (ctx->ui8CodedSkippedIndex == 3)) ? 0 : 1), TOPAZHP_CR_INTER_INTRA_SCALE_ENABLE) |
        F_ENCODE((ctx->ui8CodedSkippedIndex == 3 ? 0 : 1), TOPAZHP_CR_SKIPPED_CODED_SCALE_ENABLE);
    // workaround for BRN 33252, if the Skip coded scale is set then we also have to set the inter/inter enable. We set it enabled and rely on the default value of 3 (unbiased) to keep things behaving.
    //      psMtxEncContext->ui32PredCombControl |= F_ENCODE((ctx->ui8InterIntraIndex==3?0:1), TOPAZHP_CR_INTER_INTRA_SCALE_ENABLE) | F_ENCODE((ctx->ui8CodedSkippedIndex==3?0:1), TOPAZHP_CR_SKIPPED_CODED_SCALE_ENABLE);
    //psMtxEncContext->ui32PredCombControl |= F_ENCODE(1, TOPAZHP_CR_INTER_INTRA_SCALE_ENABLE) | F_ENCODE(1, TOPAZHP_CR_SKIPPED_CODED_SCALE_ENABLE);
    psMtxEncContext->ui32DeblockCtrl = F_ENCODE(ctx->ui8DeblockIDC, TOPAZ_DB_CR_DISABLE_DEBLOCK_IDC);
    psMtxEncContext->ui32VLCControl = 0;

    switch (ctx->eStandard) {
    case IMG_STANDARD_H264:
        psMtxEncContext->ui32VLCControl |= F_ENCODE(1, TOPAZ_VLC_CR_CODEC); // 1 for H.264 note this is inconsistant with the sequencer value
        psMtxEncContext->ui32VLCControl |= F_ENCODE(0, TOPAZ_VLC_CR_CODEC_EXTEND);

        break;
    case IMG_STANDARD_H263:
        psMtxEncContext->ui32VLCControl |= F_ENCODE(3, TOPAZ_VLC_CR_CODEC); // 3 for H.263 note this is inconsistant with the sequencer value
        psMtxEncContext->ui32VLCControl |= F_ENCODE(0, TOPAZ_VLC_CR_CODEC_EXTEND);

        break;
    case IMG_STANDARD_MPEG4:
        psMtxEncContext->ui32VLCControl |= F_ENCODE(2, TOPAZ_VLC_CR_CODEC); // 2 for Mpeg4 note this is inconsistant with the sequencer value
        psMtxEncContext->ui32VLCControl |= F_ENCODE(0, TOPAZ_VLC_CR_CODEC_EXTEND);
        break;
    case IMG_STANDARD_MPEG2:
        psMtxEncContext->ui32VLCControl |= F_ENCODE(0, TOPAZ_VLC_CR_CODEC);
        psMtxEncContext->ui32VLCControl |= F_ENCODE(1, TOPAZ_VLC_CR_CODEC_EXTEND);
        break;
    default:
        break;
    }

    if (ctx->bCabacEnabled) {
        psMtxEncContext->ui32VLCControl |= F_ENCODE(1, TOPAZ_VLC_CR_CABAC_ENABLE); // 2 for Mpeg4 note this is inconsistant with the sequencer value
    }

    psMtxEncContext->ui32VLCControl |= F_ENCODE(ctx->bIsInterlaced ? 1 : 0, TOPAZ_VLC_CR_VLC_FIELD_CODED);
    psMtxEncContext->ui32VLCControl |= F_ENCODE(ctx->bH2648x8Transform ? 1 : 0, TOPAZ_VLC_CR_VLC_8X8_TRANSFORM);
    psMtxEncContext->ui32VLCControl |= F_ENCODE(ctx->bH264IntraConstrained ? 1 : 0, TOPAZ_VLC_CR_VLC_CONSTRAINED_INTRA);

    psMtxEncContext->ui32VLCSliceControl = F_ENCODE(ctx->sRCParams.ui32SliceByteLimit, TOPAZ_VLC_CR_SLICE_SIZE_LIMIT);
    psMtxEncContext->ui32VLCSliceMBControl = F_ENCODE(ctx->sRCParams.ui32SliceMBLimit, TOPAZ_VLC_CR_SLICE_MBS_LIMIT);

        switch (ctx->eStandard) {
            case IMG_STANDARD_H264: {
                IMG_UINT32 ui32VertMVLimit = 255; // default to no clipping
                if (ctx->ui32VertMVLimit)
                    ui32VertMVLimit = ctx->ui32VertMVLimit;
                // as topaz can only cope with at most 255 (in the register field)
                ui32VertMVLimit = tng__min(255,ui32VertMVLimit);
                // workaround for BRN 29973 and 30032
                {
#if defined(BRN_29973) || defined(BRN_30032)
                    if (ctx->ui32CoreRev <= 0x30006) {
                        if ((ctx->ui16Width / 16) & 1) // BRN 30032, if odd number of macroblocks we need to limit vector to +-112
                            psMtxEncContext->ui32IPEVectorClipping = F_ENCODE(1, TOPAZHP_CR_IPE_VECTOR_CLIPPING_ENABLED)
                                |F_ENCODE(112, TOPAZHP_CR_IPE_VECTOR_CLIPPING_X)
                                |F_ENCODE(ui32VertMVLimit, TOPAZHP_CR_IPE_VECTOR_CLIPPING_Y);
                        else // for 29973 we need to limit vector to +-120
                            if (ctx->ui16Width >= 1920)
                                psMtxEncContext->ui32IPEVectorClipping = F_ENCODE(1, TOPAZHP_CR_IPE_VECTOR_CLIPPING_ENABLED)
                                |F_ENCODE(120, TOPAZHP_CR_IPE_VECTOR_CLIPPING_X)
                                |F_ENCODE(ui32VertMVLimit, TOPAZHP_CR_IPE_VECTOR_CLIPPING_Y);
                            else
                                psMtxEncContext->ui32IPEVectorClipping = F_ENCODE(1, TOPAZHP_CR_IPE_VECTOR_CLIPPING_ENABLED)
                                |F_ENCODE(255, TOPAZHP_CR_IPE_VECTOR_CLIPPING_X)
                                |F_ENCODE(ui32VertMVLimit, TOPAZHP_CR_IPE_VECTOR_CLIPPING_Y);
                    } else
#endif
                        {
                            psMtxEncContext->ui32IPEVectorClipping =
                            F_ENCODE(1, TOPAZHP_CR_IPE_VECTOR_CLIPPING_ENABLED) |
                            F_ENCODE(255, TOPAZHP_CR_IPE_VECTOR_CLIPPING_X) |
                            F_ENCODE(ui32VertMVLimit, TOPAZHP_CR_IPE_VECTOR_CLIPPING_Y);
                        }
                    }
                    psMtxEncContext->ui32SPEMvdClipRange = F_ENCODE(0, TOPAZHP_CR_SPE_MVD_CLIP_ENABLE);
            }
            break;
            case IMG_STANDARD_H263:
                {
                    psMtxEncContext->ui32IPEVectorClipping = F_ENCODE(1, TOPAZHP_CR_IPE_VECTOR_CLIPPING_ENABLED)
                        | F_ENCODE(16 - 1, TOPAZHP_CR_IPE_VECTOR_CLIPPING_X)
                        | F_ENCODE(16 - 1, TOPAZHP_CR_IPE_VECTOR_CLIPPING_Y);
            
                    psMtxEncContext->ui32SPEMvdClipRange = F_ENCODE(1, TOPAZHP_CR_SPE_MVD_CLIP_ENABLE)
                        | F_ENCODE( 62, TOPAZHP_CR_SPE_MVD_POS_CLIP)
                        | F_ENCODE(-64, TOPAZHP_CR_SPE_MVD_NEG_CLIP);
                }
                break;
            case IMG_STANDARD_MPEG4:
            case IMG_STANDARD_MPEG2:
                {
                    IMG_UINT8 uX, uY, uResidualSize;
                    //The maximum MPEG4 and MPEG2 motion vector differential in 1/2 pixels is
                    //calculated as 1 << (fcode - 1) * 32 or *16 in MPEG2
            
                    uResidualSize=(ctx->eStandard == IMG_STANDARD_MPEG4 ? 32 : 16);
            
                    uX = tng__min(128 - 1, (((1<<(ctx->sBiasTables.ui32FCode - 1)) * uResidualSize)/4) - 1);
                    uY = tng__min(104 - 1, (((1<<(ctx->sBiasTables.ui32FCode - 1)) * uResidualSize)/4) - 1);
            
                    //Write to register
                    psMtxEncContext->ui32IPEVectorClipping = F_ENCODE(1, TOPAZHP_CR_IPE_VECTOR_CLIPPING_ENABLED)
                        | F_ENCODE(uX, TOPAZHP_CR_IPE_VECTOR_CLIPPING_X)
                        | F_ENCODE(uY, TOPAZHP_CR_IPE_VECTOR_CLIPPING_Y);
            
                    psMtxEncContext->ui32SPEMvdClipRange = F_ENCODE(0, TOPAZHP_CR_SPE_MVD_CLIP_ENABLE);
                }
                break;
            case IMG_STANDARD_JPEG:
			case IMG_STANDARD_NONE:
			default:
			    break;
        }
#ifdef FIRMWARE_BIAS
    //copy the bias tables
    {
        int n;
        for (n = 52; n >= 0; n -= 2)    {
            psMtxEncContext->aui32DirectBias_P[n >> 1] = ctx->sBiasTables.aui32DirectBias_P[n];
            psMtxEncContext->aui32InterBias_P[n >> 1] = ctx->sBiasTables.aui32InterBias_P[n];
            psMtxEncContext->aui32DirectBias_B[n >> 1] = ctx->sBiasTables.aui32DirectBias_B[n];
            psMtxEncContext->aui32InterBias_B[n >> 1] = ctx->sBiasTables.aui32InterBias_B[n];
        }
    }
#endif

    //copy the scale-tables
    tng__generate_scale_tables(psMtxEncContext);
//      memcpy(psMtxEncContext->ui32InterIntraScale, ctx->ui32InterIntraScale, sizeof(IMG_UINT32)*SCALE_TBL_SZ);
//      memcpy(psMtxEncContext->ui32SkippedCodedScale, ctx->ui32SkippedCodedScale, sizeof(IMG_UINT32)*SCALE_TBL_SZ);

    // WEIGHTED PREDICTION
    psMtxEncContext->b8WeightedPredictionEnabled = ctx->bWeightedPrediction;
    psMtxEncContext->ui8MTXWeightedImplicitBiPred = ctx->ui8VPWeightedImplicitBiPred;

    // SEI_INSERTION
    psMtxEncContext->b8InsertHRDparams = ctx->bInsertHRDParams;
    if (psMtxEncContext->b8InsertHRDparams & !ctx->sRCParams.ui32BitsPerSecond) {   //ctx->uBitRate
        /* HRD parameters are meaningless without a bitrate */
        psMtxEncContext->b8InsertHRDparams = IMG_FALSE;
    }
    if (psMtxEncContext->b8InsertHRDparams) {
        psMtxEncContext->ui64ClockDivBitrate = (90000 * 0x100000000LL);
        psMtxEncContext->ui64ClockDivBitrate /= ctx->sRCParams.ui32BitsPerSecond;                       //ctx->uBitRate;
        psMtxEncContext->ui32MaxBufferMultClockDivBitrate = (IMG_UINT32)(((IMG_UINT64)(ctx->sRCParams.ui32BufferSize) *
                (IMG_UINT64) 90000) / (IMG_UINT64) ctx->sRCParams.ui32BitsPerSecond);
    }

    memcpy(&psMtxEncContext->sInParams, &ctx->sPicParams.sInParams, sizeof(IN_RC_PARAMS));
    // Update MV Scaling settings
    // IDR
    //      memcpy(&psMtxEncContext->sMVSettingsIdr, &ctx->sMVSettingsIdr, sizeof(IMG_MV_SETTINGS));
    // NonB (I or P)
    //      for (i = 0; i <= MAX_BFRAMES; i++)
    //              memcpy(&psMtxEncContext->sMVSettingsNonB[i], &ctx->sMVSettingsNonB[i], sizeof(IMG_MV_SETTINGS));

    psMtxEncContext->ui32LRITC_Cache_Chunk_Config =
        F_ENCODE(ctx->uChunksPerMb, INTEL_CH_PM) |
        F_ENCODE(ctx->uMaxChunks, INTEL_CH_MX) |
        F_ENCODE(ctx->uMaxChunks - ctx->uPriorityChunks, INTEL_CH_PY);


    //they would be set in function tng__prepare_templates()
    psMtxEncContext->ui32FirstPicFlags = ctx->ui32FirstPicFlags;
    psMtxEncContext->ui32NonFirstPicFlags = ctx->ui32NonFirstPicFlags;

#ifdef LTREFHEADER
    psMtxEncContext->i8SliceHeaderSlotNum = -1;
#endif

    {
        memset(psMtxEncContext->aui8PicOnLevel, 0, sizeof(psMtxEncContext->aui8PicOnLevel));
        psb_buffer_map(&(ps_mem->bufs_flat_gop), &(ps_mem->bufs_flat_gop.virtual_addr));
        if (ps_mem->bufs_flat_gop.virtual_addr == NULL) {
           drv_debug_msg(VIDEO_DEBUG_ERROR, "%s error: mapping flat gop\n", __FUNCTION__);
           return ;
        }
        tng__minigop_generate_flat(ps_mem->bufs_flat_gop.virtual_addr, psMtxEncContext->ui32BFrameCount,
            psMtxEncContext->ui8RefSpacing, psMtxEncContext->aui8PicOnLevel);
        psb_buffer_unmap(&(ps_mem->bufs_flat_gop));
        
        if (ctx->sRCParams.b16Hierarchical) {
            memset(psMtxEncContext->aui8PicOnLevel, 0, sizeof(psMtxEncContext->aui8PicOnLevel));
            psb_buffer_map(&(ps_mem->bufs_hierar_gop), &(ps_mem->bufs_hierar_gop.virtual_addr));
            if (ps_mem->bufs_hierar_gop.virtual_addr == NULL) {
                drv_debug_msg(VIDEO_DEBUG_ERROR, "%s error: mapping sei header\n", __FUNCTION__);
                return ;
            }
            tng_minigop_generate_hierarchical(ps_mem->bufs_hierar_gop.virtual_addr, psMtxEncContext->ui32BFrameCount,
                psMtxEncContext->ui8RefSpacing, psMtxEncContext->aui8PicOnLevel);
            psb_buffer_unmap(&(ps_mem->bufs_hierar_gop));
        }
    }

#if INPUT_SCALER_SUPPORTED
    if (ctx->bEnableScaler) {
        IMG_UINT8 sccCoeffs[4][16];
        IMG_UINT32 ui32PitchX, ui32PitchY;
        IMG_INT32 i32Phase, i32Tap;

        ui32PitchX = (((IMG_UINT32)(psVideoParams->ui16SourceWidth - psVideoParams->ui8CropLeft - psVideoParams->ui8CropRight)) << 12) / psVideoParams->ui16Width;
        ui32PitchY = (((IMG_UINT32)(psVideoParams->ui16SourceFrameHeight - psVideoParams->ui8CropTop - psVideoParams->ui8CropBottom)) << 12) / psVideoParams->ui16FrameHeight;

        // Input size
        psMtxEncContext->ui32ScalerInputSizeReg = F_ENCODE(psVideoParams->ui16SourceWidth - 1, TOPAZHP_EXT_CR_SCALER_INPUT_WIDTH_MIN1) |
            F_ENCODE((psVideoParams->ui16SourceFrameHeight >> (psVideo->bIsInterlaced ? 1 : 0)) - 1, TOPAZHP_EXT_CR_SCALER_INPUT_HEIGHT_MIN1);

        psMtxEncContext->ui32ScalerCropReg = F_ENCODE(psVideoParams->ui8CropLeft, TOPAZHP_EXT_CR_SCALER_INPUT_CROP_HOR) |
            F_ENCODE(psVideoParams->ui8CropTop, TOPAZHP_EXT_CR_SCALER_INPUT_CROP_VER);

        // Scale factors
        psMtxEncContext->ui32ScalerPitchReg = 0;

        if (ui32PitchX > 0x3FFF) {
            psMtxEncContext->ui32ScalerPitchReg |= F_ENCODE(1, TOPAZHP_EXT_CR_SCALER_HOR_BILINEAR_FILTER);
            ui32PitchX >>= 1;
        }

        if (ui32PitchX > 0x3FFF) {
            ui32PitchX = 0x3FFF;
        }

        if (ui32PitchY > 0x3FFF) {
            psMtxEncContext->ui32ScalerPitchReg |= F_ENCODE(1, TOPAZHP_EXT_CR_SCALER_VER_BILINEAR_FILTER);
            ui32PitchY >>= 1;
        }

        if (ui32PitchY > 0x3FFF) {
            ui32PitchY = 0x3FFF;
        }

        psMtxEncContext->ui32ScalerPitchReg |= F_ENCODE(ui32PitchX, TOPAZHP_EXT_CR_SCALER_INPUT_HOR_PITCH) |
            F_ENCODE(ui32PitchY, TOPAZHP_EXT_CR_SCALER_INPUT_VER_PITCH);


        // Coefficients
        VIDEO_CalcCoefs_FromPitch(((IMG_FLOAT)ui32PitchX) / 4096.0f, sccCoeffs);

        for (i32Phase = 0; i32Phase < 4; i32Phase++) {
            psMtxEncContext->asHorScalerCoeffRegs[i32Phase] = 0;
            for (i32Tap = 0; i32Tap < 4; i32Tap++) 	{
                psMtxEncContext->asHorScalerCoeffRegs[i32Phase] |= F_ENCODE(sccCoeffs[3 - i32Tap][(i32Phase * 2) + 1], TOPAZHP_EXT_CR_SCALER_HOR_LUMA_COEFF(i32Tap));
            }
        }

        VIDEO_CalcCoefs_FromPitch(((IMG_FLOAT)ui32PitchY) / 4096.0f, sccCoeffs);

        for (i32Phase = 0; i32Phase < 4; i32Phase++) {
            psMtxEncContext->asVerScalerCoeffRegs[i32Phase] = 0;
            for (i32Tap = 0; i32Tap < 4; i32Tap++) {
                psMtxEncContext->asVerScalerCoeffRegs[i32Phase] |= F_ENCODE(sccCoeffs[3 - i32Tap][(i32Phase * 2) + 1], TOPAZHP_EXT_CR_SCALER_VER_LUMA_COEFF(i32Tap));
            }
        }
    } else {
        // Disable scaling
        psMtxEncContext->ui32ScalerInputSizeReg = 0;
    }
#endif // INPUT_SCALER_SUPPORTED

    psb_buffer_unmap(&(ps_mem->bufs_mtx_context));

    return ;
}

static void tng__setvideo_cmdbuf(context_ENC_p ctx, IMG_UINT32 ui32StreamIndex)
{
    context_ENC_mem *ps_mem = &(ctx->ctx_mem[ui32StreamIndex]);
    context_ENC_mem_size *ps_mem_size = &(ctx->ctx_mem_size);
#ifndef _TNG_FRAMES_
    context_ENC_frame_buf *ps_buf = &(ctx->ctx_frame_buf);
#endif
    IMG_MTX_VIDEO_CONTEXT* psMtxEncContext = NULL;
    IMG_INT32 i;

    psb_buffer_map(&(ps_mem->bufs_mtx_context), &(ps_mem->bufs_mtx_context.virtual_addr));
    if (ps_mem->bufs_mtx_context.virtual_addr == NULL) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s error: mapping sei header\n", __FUNCTION__);
        return ;
    }
    psMtxEncContext = (IMG_MTX_VIDEO_CONTEXT*)(ps_mem->bufs_mtx_context.virtual_addr);

    tng_cmdbuf_set_phys(&(psMtxEncContext->ui32MVSettingsBTable), 0, 
        &(ps_mem->bufs_mv_setting_btable), 0, 0);
    if (ctx->sRCParams.b16Hierarchical)
        tng_cmdbuf_set_phys(&psMtxEncContext->ui32MVSettingsHierarchical, 0, 
            &(ps_mem->bufs_mv_setting_hierar), 0, 0);
#ifdef _TNG_FRAMES_
    tng_cmdbuf_set_phys(psMtxEncContext->apReconstructured, ctx->i32PicNodes,
        &(ps_mem->bufs_recon_pictures), 0, ps_mem_size->recon_pictures);
#else
    for (i = 0; i < ctx->i32PicNodes; i++) {
        tng_cmdbuf_set_phys(&(psMtxEncContext->apReconstructured[i]), 0,
            &(ps_buf->ref_surface[i]->psb_surface->buf), 0, 0);
    }
#endif

    tng_cmdbuf_set_phys(psMtxEncContext->apColocated, ctx->i32PicNodes,
        &(ps_mem->bufs_colocated), 0, ps_mem_size->colocated);
    
    tng_cmdbuf_set_phys(psMtxEncContext->apMV, ctx->i32MVStores,
        &(ps_mem->bufs_mv), 0, ps_mem_size->mv);

    if (ctx->bEnableMVC) {
        tng_cmdbuf_set_phys(psMtxEncContext->apInterViewMV, 2, 
            &(ps_mem->bufs_interview_mv), 0, ps_mem_size->interview_mv);
    }

    tng_cmdbuf_set_phys(psMtxEncContext->apWritebackRegions, WB_FIFO_SIZE,
        &(ctx->bufs_writeback), 0, ps_mem_size->writeback);

    tng_cmdbuf_set_phys(psMtxEncContext->apAboveParams, (IMG_UINT32)(ctx->ui8PipesToUse),
        &(ps_mem->bufs_above_params), 0, ps_mem_size->above_params);

    // SEI_INSERTION
    if (ctx->bInsertHRDParams) {
        tng_cmdbuf_set_phys(&psMtxEncContext->pSEIBufferingPeriodTemplate, 0,
            &(ps_mem->bufs_sei_header), ps_mem_size->sei_header, ps_mem_size->sei_header);
        tng_cmdbuf_set_phys(&psMtxEncContext->pSEIPictureTimingTemplate, 0,
            &(ps_mem->bufs_sei_header), ps_mem_size->sei_header * 2, ps_mem_size->sei_header);
    }

    tng_cmdbuf_set_phys(psMtxEncContext->apSliceParamsTemplates, NUM_SLICE_TYPES,
        &(ps_mem->bufs_slice_template), 0, ps_mem_size->slice_template);
    
    tng_cmdbuf_set_phys(psMtxEncContext->aui32SliceMap, ctx->ui8SlotsInUse,
        &(ps_mem->bufs_slice_map), 0, ps_mem_size->slice_map);

    // WEIGHTED PREDICTION
    if (ctx->bWeightedPrediction || (ctx->ui8VPWeightedImplicitBiPred == WBI_EXPLICIT)) {
        tng_cmdbuf_set_phys(psMtxEncContext->aui32WeightedPredictionVirtAddr, ctx->ui8SlotsInUse,
            &(ps_mem->bufs_weighted_prediction), 0, ps_mem_size->weighted_prediction);
    }

    tng_cmdbuf_set_phys(&psMtxEncContext->ui32FlatGopStruct, 0, &(ps_mem->bufs_flat_gop), 0, 0);
    if (psMtxEncContext->b8Hierarchical)
        tng_cmdbuf_set_phys(&psMtxEncContext->ui32HierarGopStruct, 0, &(ps_mem->bufs_hierar_gop), 0, 0);

#ifdef LTREFHEADER
    tng_cmdbuf_set_phys(psMtxEncContext->aui32LTRefHeader, ctx->ui8SlotsInUse,
        &(ps_mem->bufs_lt_ref_header), 0, ps_mem_size->lt_ref_header);
#endif

    tng_cmdbuf_set_phys(psMtxEncContext->apPicHdrTemplates, 4,
        &(ps_mem->bufs_pic_template), 0, ps_mem_size->pic_template);

    if (ctx->eStandard == IMG_STANDARD_H264) {
        tng_cmdbuf_set_phys(&(psMtxEncContext->apSeqHeader), 0, 
            &(ps_mem->bufs_seq_header), 0, ps_mem_size->seq_header);
        if (ctx->bEnableMVC)
            tng_cmdbuf_set_phys(&(psMtxEncContext->apSubSetSeqHeader), 0,
                &(ps_mem->bufs_sub_seq_header), 0, ps_mem_size->seq_header);
    }

    if (ctx->ui8EnableSelStatsFlags & ESF_FIRST_STAGE_STATS) {
        tng_cmdbuf_set_phys(psMtxEncContext->pFirstPassOutParamAddr, ctx->ui8SlotsInUse,
            &(ps_mem->bufs_first_pass_out_params), 0, ps_mem_size->first_pass_out_params);
    }

#ifndef EXCLUDE_BEST_MP_DECISION_DATA
    // Store the feedback memory address for all "5" slots in the context
    if (ctx->ui8EnableSelStatsFlags & ESF_MP_BEST_MB_DECISION_STATS
        || ctx->ui8EnableSelStatsFlags & ESF_MP_BEST_MOTION_VECTOR_STATS) {
        tng_cmdbuf_set_phys(psMtxEncContext->pFirstPassOutBestMultipassParamAddr, ctx->ui8SlotsInUse,
            &(ps_mem->bufs_first_pass_out_best_multipass_param), 0, ps_mem_size->first_pass_out_best_multipass_param);
    }
#endif

    //Store the MB-Input control parameter memory for all the 5-slots in the context
    if (ctx->bEnableInpCtrl) {
        tng_cmdbuf_set_phys(psMtxEncContext->pMBCtrlInParamsAddr, ctx->ui8SlotsInUse,
            &(ps_mem->bufs_mb_ctrl_in_params), 0, ps_mem_size->mb_ctrl_in_params);
    }

    psb_buffer_unmap(&(ps_mem->bufs_mtx_context));

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s end\n", __FUNCTION__);

    return ;
}

static VAStatus tng__validate_params(context_ENC_p ctx)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    IMG_UINT16 ui16WidthInMbs = (ctx->ui16Width + 15) >> 4;
    IMG_UINT16 ui16PictureHeight = ((ctx->ui16FrameHeight >> (ctx->bIsInterlaced ? 1 : 0)) + 15) & ~15;
    IMG_UINT16 ui16FrameHeightInMbs = (ctx->ui16FrameHeight + 15) >> 4;

    if ((ctx->ui16Width & 0xf) != 0) {
        return VA_STATUS_ERROR_INVALID_PARAMETER;
    }

    if ((ctx->ui16FrameHeight & 0xf) != 0) {
        return VA_STATUS_ERROR_INVALID_PARAMETER;
    }

    ctx->uMBspS = ui16WidthInMbs * ui16FrameHeightInMbs * ctx->sRCParams.ui32FrameRate;

    if (ctx->ui32CoreRev >= MIN_36_REV) {
        if ((ctx->ui16Width > 4096) || (ctx->ui16PictureHeight > 4096)) {
            return VA_STATUS_ERROR_INVALID_PARAMETER;
        }
        if ((ui16WidthInMbs << 4) * ui16PictureHeight > 2048 * 2048) {
            return VA_STATUS_ERROR_INVALID_PARAMETER;
        }
    } else {
        if ((ctx->ui16Width > 2048) || (ui16PictureHeight > 2048)) {
            return VA_STATUS_ERROR_INVALID_PARAMETER;
        }
    }

    if (ctx->eStandard == IMG_STANDARD_H264) {
        if ((ctx->ui8DeblockIDC == 0) && (ctx->bArbitrarySO))
            ctx->ui8DeblockIDC = 2;

        if ((ctx->ui8DeblockIDC == 0) && ((IMG_UINT32)(ctx->ui8PipesToUse) > 1) && (ctx->ui8SlicesPerPicture > 1)) {
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "WARNING: Full deblocking with multiple pipes will cause a mismatch between reconstructed and encoded video\n");
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "Consider using -deblockIDC 2 or -deblockIDC 1 instead if matching reconstructed video is required.\n");
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "WARNING: Forcing -deblockIDC = 2 for HW verification.\n");
            ctx->ui8DeblockIDC = 2;
        }
    } else if (ctx->eStandard == IMG_STANDARD_H263) {
		ctx->bArbitrarySO = IMG_FALSE;
		ctx->ui8DeblockIDC = 1;
    } else {
        ctx->ui8DeblockIDC = 1;
    }

    //ctx->sRCParams.ui32SliceByteLimit = 0;
    ctx->sRCParams.ui32SliceMBLimit = 0;
    //slice params
    if (ctx->ui8SlicesPerPicture == 0)
        ctx->ui8SlicesPerPicture = ctx->sCapsParams.ui16RecommendedSlices;
    else {
        if (ctx->ui8SlicesPerPicture > ctx->sCapsParams.ui16MaxSlices)
            ctx->ui8SlicesPerPicture = ctx->sCapsParams.ui16MaxSlices;
        else if (ctx->ui8SlicesPerPicture < ctx->sCapsParams.ui16MinSlices)
            ctx->ui8SlicesPerPicture = ctx->sCapsParams.ui16MinSlices;
    }

    if (ctx->ui32pseudo_rand_seed == UNINIT_PARAM) {
        // When -randseed is uninitialised, initialise seed using other commandline values
        ctx->ui32pseudo_rand_seed = (IMG_UINT32) ((ctx->sRCParams.ui32InitialQp + 
            ctx->ui16PictureHeight + ctx->ui16Width + ctx->sRCParams.ui32BitsPerSecond) & 0xffffffff);
        // iQP_Luma + pParams->uHeight + pParams->uWidth + pParams->uBitRate) & 0xffffffff);
    }

    if (ctx->eStandard == IMG_STANDARD_H264) {
        ctx->ui8PipesToUse = tng__min(ctx->ui8PipesToUse, ctx->ui8SlicesPerPicture);
    } else {
        ctx->ui8PipesToUse = 1;
    }

    return vaStatus;
}

static VAStatus tng__validate_busize(context_ENC_p ctx)
{
    //IMG_RC_PARAMS *psRCParams = &(ctx->sRCParams);
    // if no BU size is given then pick one ourselves, if doing arbitrary slice order then make BU = width in bu's
    // forces slice boundaries to no be mid-row
    if (ctx->bArbitrarySO || (ctx->ui32BasicUnit == 0)) {
        ctx->ui32BasicUnit = (ctx->ui16Width / 16);
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "Patched Basic unit to %d\n", ctx->ui32BasicUnit);
    } else {
        IMG_UINT32 ui32MBs, ui32MBsperSlice, ui32MBsLastSlice;
        IMG_UINT32 ui32BUs;
        IMG_INT32  i32SliceHeight;
        IMG_UINT32 ui32MaxSlicesPerPipe, ui32MaxMBsPerPipe, ui32MaxBUsPerPipe;

        ui32MBs  = ctx->ui16PictureHeight * ctx->ui16Width / (16 * 16);

        i32SliceHeight = ctx->ui16PictureHeight / ctx->ui8SlicesPerPicture;
        i32SliceHeight &= ~15;

        ui32MBsperSlice = (i32SliceHeight * ctx->ui16Width) / (16 * 16);
        ui32MBsLastSlice = ui32MBs - (ui32MBsperSlice * (ctx->ui8SlicesPerPicture - 1));

        // they have given us a basic unit so validate it
        if (ctx->ui32BasicUnit < 6) {
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "\nERROR: Basic unit size too small, must be greater than 6\n");
            return VA_STATUS_ERROR_UNKNOWN;
        }

        if (ctx->ui32BasicUnit > ui32MBsperSlice) {
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "\nERROR: Basic unit size(%d) too large", ctx->ui32BasicUnit);
            drv_debug_msg(VIDEO_DEBUG_GENERAL, " must not be greater than the number of macroblocks in a slice (%d)\n", ui32MBsperSlice);
            return VA_STATUS_ERROR_UNKNOWN;
        }
        if (ctx->ui32BasicUnit > ui32MBsLastSlice) {
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "\nERROR: Basic unit size(%d) too large", ctx->ui32BasicUnit);
            drv_debug_msg(VIDEO_DEBUG_GENERAL, " must not be greater than the number of macroblocks in a slice (%d)\n", ui32MBsLastSlice);
            return VA_STATUS_ERROR_UNKNOWN;
        }

        ui32BUs = ui32MBsperSlice / ctx->ui32BasicUnit;
        if ((ui32BUs * ctx->ui32BasicUnit) != ui32MBsperSlice) {
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "\nERROR: Basic unit size(%d) not an integer divisor of MB's in a slice(%d)",
                                     ctx->ui32BasicUnit, ui32MBsperSlice);
            return VA_STATUS_ERROR_UNKNOWN;
        }

        ui32BUs = ui32MBsLastSlice / ctx->ui32BasicUnit;
        if ((ui32BUs * ctx->ui32BasicUnit) != ui32MBsLastSlice) {
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "\nERROR: Basic unit size(%d) not an integer divisor of MB's in the last slice(%d)",
                                     ctx->ui32BasicUnit, ui32MBsLastSlice);
            return VA_STATUS_ERROR_UNKNOWN;
        }

        // check if the number of BUs per pipe is greater than 200
        ui32MaxSlicesPerPipe = (IMG_UINT32)(ctx->ui8SlicesPerPicture + ctx->ui8PipesToUse - 1) / (IMG_UINT32)(ctx->ui8PipesToUse);
        ui32MaxMBsPerPipe = (ui32MBsperSlice * (ui32MaxSlicesPerPipe - 1)) + ui32MBsLastSlice;
        ui32MaxBUsPerPipe = (ui32MaxMBsPerPipe + ctx->ui32BasicUnit - 1) / ctx->ui32BasicUnit;
        if (ui32MaxBUsPerPipe > 200) {
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "\nERROR: Basic unit size too small. There must be less than 201 basic units per slice");
            return VA_STATUS_ERROR_UNKNOWN;
        }
    }

    ctx->sRCParams.ui32BUSize = ctx->ui32BasicUnit;
    return VA_STATUS_SUCCESS;
}

static VAStatus tng__cmdbuf_new_codec(context_ENC_p ctx)
{
     VAStatus vaStatus = VA_STATUS_SUCCESS;
     tng_cmdbuf_p cmdbuf = ctx->obj_context->tng_cmdbuf;
     psb_driver_data_p driver_data = ctx->obj_context->driver_data;
     context_ENC_mem *ps_mem = &(ctx->ctx_mem[0]);

     *cmdbuf->cmd_idx++ =
        ((MTX_CMDID_SW_NEW_CODEC & MTX_CMDWORD_ID_MASK) << MTX_CMDWORD_ID_SHIFT) |
        ((ctx->eCodec        & MTX_CMDWORD_CORE_MASK) << MTX_CMDWORD_CORE_SHIFT) |
        (((driver_data->context_id & MTX_CMDWORD_COUNT_MASK) << MTX_CMDWORD_COUNT_SHIFT));
 //       (((driver_data->drm_context & MTX_CMDWORD_COUNT_MASK) << MTX_CMDWORD_COUNT_SHIFT));

     tng_cmdbuf_insert_command_param((ctx->ui16Width << 16) | ctx->ui16PictureHeight);

    return vaStatus;
}

static VAStatus tng__cmdbuf_doheader(context_ENC_p ctx)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    context_ENC_mem *ps_mem = &(ctx->ctx_mem[ctx->ui32StreamID]);
     tng_cmdbuf_p cmdbuf = ctx->obj_context->tng_cmdbuf;

    cmdbuf->cmd_idx_saved[TNG_CMDBUF_PIC_HEADER_IDX] = cmdbuf->cmd_idx;
    tng_cmdbuf_insert_command(ctx->obj_context, 0,
                                      MTX_CMDID_DO_HEADER,
                                      0,
                                      &(ps_mem->bufs_seq_header),
                                      0);
    return vaStatus;
}

static VAStatus tng__cmdbuf_lowpower(context_ENC_p ctx)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    tng_cmdbuf_p cmdbuf = ctx->obj_context->tng_cmdbuf;
    psb_driver_data_p driver_data = ctx->obj_context->driver_data;

    *cmdbuf->cmd_idx++ =
        ((MTX_CMDID_SW_LEAVE_LOWPOWER & MTX_CMDWORD_ID_MASK) << MTX_CMDWORD_ID_SHIFT) |
        (((ctx->ui32RawFrameCount == 0 ? 1 : 0)  & MTX_CMDWORD_CORE_MASK) << MTX_CMDWORD_CORE_SHIFT) |
        (((driver_data->context_id & MTX_CMDWORD_COUNT_MASK) << MTX_CMDWORD_COUNT_SHIFT));

    tng_cmdbuf_insert_command_param(ctx->eCodec);

    return vaStatus;
}

static VAStatus tng__cmdbuf_load_bias(context_ENC_p ctx)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    
    //init bias parameters
     tng_init_bias_params(ctx);
    
     vaStatus = tng__generate_bias(ctx);
     if (vaStatus != VA_STATUS_SUCCESS) {
         drv_debug_msg(VIDEO_DEBUG_ERROR, "%s: generate bias params\n", __FUNCTION__, vaStatus);
     }
    
     vaStatus = tng_load_bias(ctx, IMG_INTER_P);
     if (vaStatus != VA_STATUS_SUCCESS) {
         drv_debug_msg(VIDEO_DEBUG_ERROR, "%s: load bias params\n", __FUNCTION__, vaStatus);
     }
    return vaStatus;
}

static VAStatus tng__cmdbuf_setvideo(context_ENC_p ctx, IMG_UINT32 ui32StreamIndex)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    context_ENC_mem *ps_mem = &(ctx->ctx_mem[ui32StreamIndex]);

    tng__setvideo_params(ctx, ui32StreamIndex);
    tng__setvideo_cmdbuf(ctx, ui32StreamIndex);

    tng_cmdbuf_insert_command(ctx->obj_context, ctx->ui32StreamID,
        MTX_CMDID_SETVIDEO, 0, &(ps_mem->bufs_mtx_context), 0);

    return vaStatus;
}

static void tng__rc_update(
    context_ENC_p ctx,
    IMG_UINT32 ui32NewBitrate,
    IMG_UINT8 ui8NewFrameQP,
    IMG_UINT8 ui8NewFrameMinQP,
    IMG_UINT8 ui8NewFrameMaxQP,
    IMG_UINT16 ui16NewIntraPeriod)
{
    psb_buffer_p buf = (psb_buffer_p)(F_ENCODE(ui8NewFrameMinQP, MTX_MSG_RC_UPDATE_MIN_QP) |
                        F_ENCODE(ui8NewFrameMaxQP, MTX_MSG_RC_UPDATE_MAX_QP) |
                        F_ENCODE(ui16NewIntraPeriod, MTX_MSG_RC_UPDATE_INTRA));
    tng_cmdbuf_insert_command(
                        ctx->obj_context,
                        ctx->ui32StreamID,
                        MTX_CMDID_RC_UPDATE,
                        F_ENCODE(ui8NewFrameQP, MTX_MSG_RC_UPDATE_QP) |
                            F_ENCODE(ui32NewBitrate, MTX_MSG_RC_UPDATE_BITRATE),
                        buf,
                        0);
}

static VAStatus tng__update_ratecontrol(context_ENC_p ctx, IMG_UINT32 ui32StreamIndex)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    IMG_RC_PARAMS *psRCParams = &(ctx->sRCParams);
    IMG_UINT32 ui32CmdData = 0;
    IMG_UINT32 ui32NewBitsPerFrame = 0;
    IMG_UINT8 ui8NewVCMIFrameQP = 0;

    if (!(ctx->rc_update_flag))
        return vaStatus;

    tng__setup_rcdata(ctx);

    drv_debug_msg(VIDEO_DEBUG_GENERAL,
        "%s: frame[%d] bits_per_second = %d, min_qp = %d, max_qp = %d, initial_qp = %d\n",
        __FUNCTION__, ctx->ui32FrameCount[ui32StreamIndex], psRCParams->ui32BitsPerSecond,
        psRCParams->iMinQP, ctx->max_qp, psRCParams->ui32InitialQp);

    if (ctx->rc_update_flag & RC_MASK_frame_rate) {
	tng__rc_update(ctx, psRCParams->ui32BitsPerSecond, -1, -1, -1, -1);
	ctx->rc_update_flag &= ~RC_MASK_frame_rate;
    }

    if (ctx->rc_update_flag & RC_MASK_bits_per_second) {
	tng__rc_update(ctx, psRCParams->ui32BitsPerSecond, -1, -1, -1, -1);
	ctx->rc_update_flag &= ~RC_MASK_bits_per_second;
    }

    if (ctx->rc_update_flag & RC_MASK_min_qp) {
	tng__rc_update(ctx, -1, -1, psRCParams->iMinQP, -1, -1);
	ctx->rc_update_flag &= ~RC_MASK_min_qp;
    }

    if (ctx->rc_update_flag & RC_MASK_max_qp) {
	tng__rc_update(ctx, -1, -1, -1, ctx->max_qp, -1);
	ctx->rc_update_flag &= ~RC_MASK_max_qp;
    }

    if (ctx->rc_update_flag & RC_MASK_initial_qp) {
	tng__rc_update(ctx, -1, psRCParams->ui32InitialQp, -1, -1, -1);
	ctx->rc_update_flag &= ~RC_MASK_initial_qp;
    }

    if (ctx->rc_update_flag & RC_MASK_intra_period) {
	tng__rc_update(ctx, -1, -1, -1, -1, ctx->ui32IntraCnt);
	ctx->rc_update_flag &= ~RC_MASK_intra_period;
    }

    return vaStatus;
}

static VAStatus tng__update_frametype(context_ENC_p ctx, IMG_FRAME_TYPE eFrameType)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    IMG_UINT32 ui32CmdData = 0;

    ui32CmdData = F_ENCODE(IMG_PICMGMT_REF_TYPE, MTX_MSG_PICMGMT_SUBTYPE) |
                F_ENCODE(eFrameType, MTX_MSG_PICMGMT_DATA);

    tng_cmdbuf_insert_command(ctx->obj_context, ctx->ui32StreamID,
        MTX_CMDID_PICMGMT ,
        ui32CmdData, 0, 0);

    return vaStatus;
}

static VAStatus tng__cmdbuf_send_picmgmt(context_ENC_p ctx, IMG_UINT32 ui32StreamIndex)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    IMG_RC_PARAMS *psRCParams = &(ctx->sRCParams);

    if (!(ctx->rc_update_flag))
        return vaStatus;

    //tng__setup_rcdata(ctx);
    drv_debug_msg(VIDEO_DEBUG_GENERAL,
        "%s: ui32BitsPerSecond = %d, ui32FrameRate = %d, ui32InitialQp = %d\n",
        __FUNCTION__, psRCParams->ui32BitsPerSecond,
        psRCParams->ui32FrameRate, psRCParams->ui32InitialQp);
    drv_debug_msg(VIDEO_DEBUG_GENERAL,
        "%s: frame_count[%d] = %d\n", __FUNCTION__,
        ui32StreamIndex, ctx->ui32FrameCount[ui32StreamIndex]);

    return vaStatus;
}


static VAStatus tng__cmdbuf_provide_buffer(context_ENC_p ctx, IMG_UINT32 ui32StreamIndex)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;

    if (ctx->ui8PipesToUse == 1) {
        tng_send_codedbuf(ctx, ctx->ui8SlotsCoded);
    } else {
        /*Make sure DMA start is 128bits alignment*/
        tng_send_codedbuf(ctx, ctx->ui8SlotsCoded * 2);
        tng_send_codedbuf(ctx, ctx->ui8SlotsCoded * 2 + 1);
    }

    if (ctx->sRCParams.ui16BFrames > 0)
        tng__provide_buffer_BFrames(ctx, ui32StreamIndex);
    else
        tng__provide_buffer_PFrames(ctx, ui32StreamIndex);
/*
    if (ctx->ui32LastPicture != 0) {
        drv_debug_msg(VIDEO_DEBUG_GENERAL,
            "%s: frame_count[%d] = %d\n", __FUNCTION__,
            ui32StreamIndex, ctx->ui32FrameCount[ui32StreamIndex]);
        tng_picmgmt_update(ctx,IMG_PICMGMT_EOS, ctx->ui32LastPicture);
    }
*/
#ifdef _TOPAZHP_REC_
    tng_send_rec_frames(ctx, -1, 0);
    tng_send_ref_frames(ctx, 0, 0);
    tng_send_ref_frames(ctx, 1, 0);
#endif

    ctx->ui8SlotsCoded = (ctx->ui8SlotsCoded + 1) & 1;

    return vaStatus;
}

VAStatus tng__set_ctx_buf(context_ENC_p ctx, IMG_UINT32 __maybe_unused ui32StreamID)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    IMG_UINT8 ui8IsJpeg;

    vaStatus = tng__validate_params(ctx);
    if (vaStatus != VA_STATUS_SUCCESS) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "validate params");
    }

    vaStatus = tng__validate_busize(ctx);
    if (vaStatus != VA_STATUS_SUCCESS) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "validate busize");
    }
    ctx->ctx_cmdbuf[0].ui32LowCmdCount = 0xa5a5a5a5 %  MAX_TOPAZ_CMD_COUNT;
    ctx->ctx_cmdbuf[0].ui32HighCmdCount = 0;
    ctx->ctx_cmdbuf[0].ui32HighWBReceived = 0;

    ui8IsJpeg = (ctx->eStandard == IMG_STANDARD_JPEG) ? 1 : 0;
    vaStatus = tng__alloc_context_buffer(ctx, ui8IsJpeg, 0);
    if (vaStatus != VA_STATUS_SUCCESS) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "setup enc profile");
    }
    return vaStatus;
}

static VAStatus tng__set_headers (context_ENC_p ctx, IMG_UINT32 __maybe_unused ui32StreamID)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    IMG_UINT8 ui8SlotIdx = 0;

    vaStatus = tng__prepare_templates(ctx, 0);
    if (vaStatus != VA_STATUS_SUCCESS) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "prepare_templates\n");
    }
    
    for (ui8SlotIdx = 0; ui8SlotIdx < ctx->ui8SlotsInUse; ui8SlotIdx++)
        tng_fill_slice_map(ctx, (IMG_UINT32)ui8SlotIdx, 0);

    return vaStatus;
}

static VAStatus tng__set_cmd_buf(context_ENC_p ctx, IMG_UINT32 __maybe_unused ui32StreamID)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;

    vaStatus = tng__cmdbuf_new_codec(ctx);
    if (vaStatus != VA_STATUS_SUCCESS) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "cmdbuf new codec\n");
    }
    
    vaStatus = tng__cmdbuf_lowpower(ctx);
    if (vaStatus != VA_STATUS_SUCCESS) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "cmdbuf lowpower\n");
    }
    
    vaStatus = tng__cmdbuf_load_bias(ctx);
    if (vaStatus != VA_STATUS_SUCCESS) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "cmdbuf load bias\n");
    }
    
    vaStatus = tng__cmdbuf_setvideo(ctx, 0);
    if (vaStatus != VA_STATUS_SUCCESS) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "cmdbuf setvideo\n");
    }
    return vaStatus;
}

VAStatus tng__end_one_frame(context_ENC_p ctx, IMG_UINT32 ui32StreamID)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    context_ENC_frame_buf *ps_buf = &(ctx->ctx_frame_buf);

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "ui32StreamID is %d.\n", ui32StreamID);

    /* save current settings */
    ps_buf->previous_src_surface = ps_buf->src_surface;
#ifdef _TNG_FRAMES_
    ps_buf->previous_ref_surface = ps_buf->ref_surface;
#else
    ps_buf->previous_ref_surface = ps_buf->ref_surface[0];
#endif

    /*Frame Skip flag in Coded Buffer of frame N determines if frame N+2
    * should be skipped, which means sending encoding commands of frame N+1 doesn't
    * have to wait until frame N is completed encoded. It reduces the precision of
    * rate control but improves HD encoding performance a lot.*/
    ps_buf->pprevious_coded_buf = ps_buf->previous_coded_buf;
    ps_buf->previous_coded_buf = ps_buf->coded_buf;

    ctx->ePreFrameType = ctx->eFrameType;

    return vaStatus;
}

VAStatus tng_EndPicture(context_ENC_p ctx)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    tng_cmdbuf_p cmdbuf = ctx->obj_context->tng_cmdbuf;
    context_ENC_mem *ps_mem = &(ctx->ctx_mem[0]);
    unsigned int offset;
    int value;

    drv_debug_msg(VIDEO_DEBUG_GENERAL,"%s: ctx->ui8SlicesPerPicture = %d, ctx->ui32FrameCount[0] = %d\n",
         __FUNCTION__, ctx->ui8SlicesPerPicture, ctx->ui32FrameCount[0]);

    if (ctx->ui32FrameCount[0] == 0) {
        vaStatus = tng__set_ctx_buf(ctx, 0);
        if (vaStatus != VA_STATUS_SUCCESS) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "set ctx buf \n");
        }
        vaStatus = tng__set_headers(ctx, 0);
        if (vaStatus != VA_STATUS_SUCCESS) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "set headers \n");
        }

        vaStatus = tng__set_cmd_buf(ctx, 0);
        if (vaStatus != VA_STATUS_SUCCESS) {
           drv_debug_msg(VIDEO_DEBUG_ERROR, "set cmd buf \n");
        }

#ifdef _TOPAZHP_PDUMP_
	tng_trace_setvideo(ctx, 0);
#endif
    } else {
        vaStatus = tng__cmdbuf_lowpower(ctx);
        if (vaStatus != VA_STATUS_SUCCESS) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "cmdbuf lowpower\n");
        }
    }

    if (ctx->sRCParams.eRCMode != IMG_RCMODE_NONE ||
	ctx->rc_update_flag) {
        vaStatus = tng__update_ratecontrol(ctx, ctx->ui32StreamID);
        if (vaStatus != VA_STATUS_SUCCESS) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "send picmgmt");
        }
    }

    if ((ctx->idr_force_flag == 1) && (ctx->sRCParams.ui16BFrames == 0)){
        vaStatus = tng__update_frametype(ctx, IMG_FRAME_IDR);
        if (vaStatus != VA_STATUS_SUCCESS) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "send picmgmt IDR");
        }
        ctx->idr_force_flag =0;
    }

    vaStatus = tng__cmdbuf_provide_buffer(ctx, ctx->ui32StreamID);
    if (vaStatus != VA_STATUS_SUCCESS) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "provide buffer");
    }

    if (ctx->bEnableAIR == IMG_TRUE ||
	ctx->bEnableCIR == IMG_TRUE) {
	tng_air_set_input_control(ctx, 0);

	if (ctx->bEnableAIR == IMG_TRUE)
	    tng_air_set_output_control(ctx, 0);
    }

    if (ctx->eStandard == IMG_STANDARD_MPEG4) {
        if (ctx->ui32FrameCount[0] == 0) {
            vaStatus = tng__cmdbuf_doheader(ctx);
            if (vaStatus != VA_STATUS_SUCCESS) {
                drv_debug_msg(VIDEO_DEBUG_ERROR, "cmdbuf doheader\n");
            }
        }
        tng__MPEG4ES_send_seq_header(ctx, ctx->ui32StreamID);
    }

    if (ctx->bInsertHRDParams)
	tng_cmdbuf_insert_command(ctx->obj_context, ctx->ui32StreamID,
	    MTX_CMDID_DO_HEADER, 0, &(ps_mem->bufs_sei_header), 0);

    tng_cmdbuf_insert_command(ctx->obj_context, ctx->ui32StreamID,
        MTX_CMDID_ENCODE_FRAME, 0, 0, 0);

#ifdef _TOPAZHP_CMDBUF_
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s addr = 0x%08x \n", __FUNCTION__, cmdbuf);
    tng__trace_cmdbuf_words(cmdbuf);
    tng__trace_cmdbuf(cmdbuf, ctx->ui32StreamID);
#endif

    //    tng_buffer_unmap(ctx, ctx->ui32StreamID);
    tng_cmdbuf_mem_unmap(cmdbuf);

    vaStatus = tng__end_one_frame(ctx, 0);
    if (vaStatus != VA_STATUS_SUCCESS) {
       drv_debug_msg(VIDEO_DEBUG_ERROR, "setting when one frame ends\n");
    }

    if (tng_context_flush_cmdbuf(ctx->obj_context)) {
        vaStatus = VA_STATUS_ERROR_UNKNOWN;
    }


    ++(ctx->ui32FrameCount[ctx->ui32StreamID]);
    ++(ctx->ui32RawFrameCount);
    return vaStatus;
}
