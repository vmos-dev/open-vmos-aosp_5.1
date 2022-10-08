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
*/

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>

#include "psb_drv_debug.h"
#include "tng_hostdefs.h"
#include "tng_hostheader.h"
#include "tng_picmgmt.h"
#include "tng_jpegES.h"
#include "tng_trace.h"

#include "hwdefs/topazhp_core_regs.h"
#include "hwdefs/topazhp_multicore_regs_old.h"
#include "hwdefs/topaz_db_regs.h"
#include "hwdefs/topaz_vlc_regs.h"
#include "hwdefs/mvea_regs.h"
#include "hwdefs/topazhp_default_params.h"

unsigned int dump_address_content = 1;

#define PRINT_ARRAY_NEW( FEILD, NUM)            \
    for(i=0;i< NUM;i++) {                       \
        if(i%6==0)                              \
            drv_debug_msg(VIDEO_ENCODE_PDUMP,"\n\t\t");                   \
        drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t0x%x", data->FEILD[i]); } \
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\n\t\t}\n");

#define PRINT_ARRAY_INT( FEILD, NUM)            \
do {                                            \
    int tmp;                                    \
                                                \
    for(tmp=0;tmp< NUM;tmp++) {                 \
        if(tmp%6==0)                            \
            drv_debug_msg(VIDEO_ENCODE_PDUMP,"\n\t\t");                   \
        drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t0x%08x", FEILD[tmp]);         \
    }                                           \
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\n\t\t}\n");                        \
} while (0)


#define PRINT_ARRAY_BYTE( FEILD, NUM)           \
do {                                            \
    int tmp;                                    \
                                                \
    for(tmp=0;tmp< NUM;tmp++) {                 \
        if(tmp%8==0)                           \
            drv_debug_msg(VIDEO_ENCODE_PDUMP,"\n\t\t");                   \
        drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t0x%02x", FEILD[tmp]);         \
    }                                           \
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\n\t\t}\n");                        \
} while (0)


/*
#define PRINT_ARRAY( FEILD, NUM)                \
    for(i=0;i< NUM;i++)                         \
        drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t0x%x", data->FEILD[i]);       \
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\n\t\t}\n");
*/

#define PRINT_ARRAY( FEILD, NUM)  PRINT_ARRAY_NEW(FEILD, NUM)                

#define PRINT_ARRAY_ADDR(STR, FEILD, NUM)                       \
do {                                                            \
    int i = 0;                                                  \
    unsigned char *virt;                                        \
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\n");                     \
    for (i=0;i< NUM;i++)  {                                     \
        drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t%s[%02d]=x%08x\n", STR, i, data->FEILD[i]); \
    }                                                           \
} while (0)

static int MTX_HEADER_PARAMS_dump(MTX_HEADER_PARAMS *p);

static int DO_HEADER_dump(MTX_HEADER_PARAMS *data)
{
    MTX_HEADER_PARAMS *p = data;
    unsigned char *q=(unsigned char *)data;

    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t(===RawBits===)");
    PRINT_ARRAY_BYTE(q, 128);

    MTX_HEADER_PARAMS_dump(p);
        
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\n\n");
    
    return 0;
}

static void JPEG_MTX_DMA_dump(JPEG_MTX_DMA_SETUP *data)
{
    int i;
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	ComponentPlane{\n");
    for(i=0;i<MTX_MAX_COMPONENTS ;i++)
    {
        drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t\t   ui32PhysAddr=%d\n",data->ComponentPlane[i].ui32PhysAddr);
        drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t   ui32Stride=%d",data->ComponentPlane[i].ui32Stride);
        drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t   ui32Height=%d\n",data->ComponentPlane[i].ui32Height);
    }
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	}\n");
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	MCUComponent{\n");
    for(i=0;i<MTX_MAX_COMPONENTS ;i++)
    {
        drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t\t   ui32WidthBlocks=%d",data->MCUComponent[i].ui32WidthBlocks);
        drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t   ui32HeightBlocks=%d",data->MCUComponent[i].ui32HeightBlocks);
        drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t   ui32XLimit=%d\n",data->MCUComponent[i].ui32XLimit);
        drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t   ui32YLimit=%d\n",data->MCUComponent[i].ui32YLimit);
    }
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	}\n");
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	ui32ComponentsInScan =%d\n", data->ui32ComponentsInScan);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	ui32TableA =%d\n", data->ui32TableA);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	ui16DataInterleaveStatus =%d\n", data->ui16DataInterleaveStatus);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	ui16MaxPipes =%d\n", data->ui16MaxPipes);
    //drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	apWritebackRegions  {");
    //PRINT_ARRAY(	apWritebackRegions, WB_FIFO_SIZE);
}

static void ISSUE_BUFFER_dump(MTX_ISSUE_BUFFERS *data)
{
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	ui32MCUPositionOfScanAndPipeNo =%d\n", data->ui32MCUPositionOfScanAndPipeNo);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	ui32MCUCntAndResetFlag =%d\n", data->ui32MCUCntAndResetFlag);
}

static void JPEG_TABLE_dump(JPEG_MTX_QUANT_TABLE *data)
{
    int i;
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	aui8LumaQuantParams  {");
    PRINT_ARRAY(	aui8LumaQuantParams, QUANT_TABLE_SIZE_BYTES);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	aui8ChromaQuantParams  {");
    PRINT_ARRAY(	aui8ChromaQuantParams, QUANT_TABLE_SIZE_BYTES);
}

static char *IMG_FRAME_TEMPLATE_TYPE2Str(IMG_FRAME_TEMPLATE_TYPE tmp)
{
    switch (tmp){
    case IMG_FRAME_IDR:return "IMG_FRAME_IDR";
    case IMG_FRAME_INTRA:return "IMG_FRAME_INTRA";
    case IMG_FRAME_INTER_P:return "IMG_FRAME_INTER_P";
    case IMG_FRAME_INTER_B:return "IMG_FRAME_INTER_B";
    case IMG_FRAME_INTER_P_IDR:return "IMG_FRAME_INTER_P_IDR";
    case IMG_FRAME_UNDEFINED:return "IMG_FRAME_UNDEFINED";
    }

    return "Undefined";
}

static int apReconstructured_dump(context_ENC_p ctx)
{
    int i;
    context_ENC_mem* ps_mem = &(ctx->ctx_mem[0]);
    context_ENC_mem_size *ps_mem_size = &(ctx->ctx_mem_size);
    IMG_MTX_VIDEO_CONTEXT* data = NULL;

    psb_buffer_map(&(ps_mem->bufs_mtx_context), &(ps_mem->bufs_mtx_context.virtual_addr));
    if (ps_mem->bufs_mtx_context.virtual_addr == NULL) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s error: mapping mtx context\n", __FUNCTION__);
        return 0;
    }
    data = (IMG_MTX_VIDEO_CONTEXT*)(ps_mem->bufs_mtx_context.virtual_addr);
    if (data == NULL) {
        drv_debug_msg(VIDEO_DEBUG_ERROR,"%s data pointer is NULL\n", __FUNCTION__);
        return 0;
    }

    data = (IMG_MTX_VIDEO_CONTEXT*)(ps_mem->bufs_mtx_context.virtual_addr);

    psb_buffer_map(&(ps_mem->bufs_recon_pictures), &(ps_mem->bufs_recon_pictures.virtual_addr));
    if (ps_mem->bufs_recon_pictures.virtual_addr == NULL) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s error: mapping reconstructed buf\n", __FUNCTION__);
        return 0;
    }

    for (i = 0; i < 8; i++) {
	if (dump_address_content && data->apReconstructured[i]) {
	    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\tapReconstructured[%02d]=x%08x\n", i, data->apReconstructured[i]);
	    PRINT_ARRAY_BYTE(ps_mem->bufs_recon_pictures.virtual_addr, 64);
	} else {
	    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\tapReconstructured[%02d]=x%08x = {	}\n", i, data->apReconstructured[i]);
	}
    }

    psb_buffer_unmap(&(ps_mem->bufs_mtx_context));
    psb_buffer_unmap(&(ps_mem->bufs_recon_pictures));
    return 0;
}

static int apColocated_dump(context_ENC_p ctx)
{
    int i;
    context_ENC_mem* ps_mem = &(ctx->ctx_mem[0]);
    context_ENC_mem_size *ps_mem_size = &(ctx->ctx_mem_size);
    IMG_MTX_VIDEO_CONTEXT* data = NULL;

    psb_buffer_map(&(ps_mem->bufs_mtx_context), &(ps_mem->bufs_mtx_context.virtual_addr));
    if (ps_mem->bufs_mtx_context.virtual_addr == NULL) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s error: mapping mtx context\n", __FUNCTION__);
        return 0;
    }
    data = (IMG_MTX_VIDEO_CONTEXT*)(ps_mem->bufs_mtx_context.virtual_addr);
    if (data == NULL) {
        drv_debug_msg(VIDEO_DEBUG_ERROR,"%s data pointer is NULL\n", __FUNCTION__);
        return 0;
    }

    data = (IMG_MTX_VIDEO_CONTEXT*)(ps_mem->bufs_mtx_context.virtual_addr);

    psb_buffer_map(&(ps_mem->bufs_colocated), &(ps_mem->bufs_colocated.virtual_addr));
    if (ps_mem->bufs_colocated.virtual_addr == NULL) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s error: mapping colocated buf\n", __FUNCTION__);
        return 0;
    }

    for (i = 0; i < 8; i++) {
	if (dump_address_content && data->apReconstructured[i]) {
	    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\tapColocated[%02d]=x%08x\n", i, data->apColocated[i]);
	    PRINT_ARRAY_BYTE(ps_mem->bufs_colocated.virtual_addr, 64);
	} else {
	    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\tapColocated[%02d]=x%08x = {	}\n", i, data->apColocated[i]);
	}
    }

    psb_buffer_unmap(&(ps_mem->bufs_mtx_context));
    psb_buffer_unmap(&(ps_mem->bufs_colocated));
    return 0;
}

static int apPV_dump(context_ENC_p ctx)
{
    int i;
    context_ENC_mem* ps_mem = &(ctx->ctx_mem[0]);
    context_ENC_mem_size *ps_mem_size = &(ctx->ctx_mem_size);
    IMG_MTX_VIDEO_CONTEXT* data = NULL;

    psb_buffer_map(&(ps_mem->bufs_mtx_context), &(ps_mem->bufs_mtx_context.virtual_addr));
    if (ps_mem->bufs_mtx_context.virtual_addr == NULL) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s error: mapping mtx context\n", __FUNCTION__);
        return 0;
    }
    data = (IMG_MTX_VIDEO_CONTEXT*)(ps_mem->bufs_mtx_context.virtual_addr);
    if (data == NULL) {
        drv_debug_msg(VIDEO_DEBUG_ERROR,"%s data pointer is NULL\n", __FUNCTION__);
        return 0;
    }

    data = (IMG_MTX_VIDEO_CONTEXT*)(ps_mem->bufs_mtx_context.virtual_addr);

    psb_buffer_map(&(ps_mem->bufs_mv), &(ps_mem->bufs_mv.virtual_addr));
    if (ps_mem->bufs_mv.virtual_addr == NULL) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s error: mapping apMV buf\n", __FUNCTION__);
        return 0;
    }

    for (i = 0; i < 16; i++) {
	if (dump_address_content && data->apMV[i]) {
	    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\tapMV[%02d]=x%08x\n", i, data->apMV[i]);
	    PRINT_ARRAY_BYTE(ps_mem->bufs_mv.virtual_addr, 64);
	} else {
	    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\tapMV[%02d]=x%08x = {	}\n", i, data->apMV[i]);
	}
    }

    psb_buffer_unmap(&(ps_mem->bufs_mtx_context));
    psb_buffer_unmap(&(ps_mem->bufs_mv));
    return 0;
}

static int apWritebackRegions_dump(context_ENC_p ctx)
{
    int i;
    context_ENC_mem* ps_mem = &(ctx->ctx_mem[0]);
    context_ENC_mem_size *ps_mem_size = &(ctx->ctx_mem_size);
    IMG_MTX_VIDEO_CONTEXT* data = NULL;

    psb_buffer_map(&(ps_mem->bufs_mtx_context), &(ps_mem->bufs_mtx_context.virtual_addr));
    if (ps_mem->bufs_mtx_context.virtual_addr == NULL) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s error: mapping mtx context\n", __FUNCTION__);
        return 0;
    }
    data = (IMG_MTX_VIDEO_CONTEXT*)(ps_mem->bufs_mtx_context.virtual_addr);
    if (data == NULL) {
        drv_debug_msg(VIDEO_DEBUG_ERROR,"%s data pointer is NULL\n", __FUNCTION__);
        return 0;
    }

    data = (IMG_MTX_VIDEO_CONTEXT*)(ps_mem->bufs_mtx_context.virtual_addr);

    psb_buffer_map(&(ctx->bufs_writeback), &(ctx->bufs_writeback.virtual_addr));
    if (ctx->bufs_writeback.virtual_addr == NULL) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s error: mapping write back buf\n", __FUNCTION__);
        return 0;
    }

    for (i = 0; i < 32; i++) {
	if (dump_address_content && data->apWritebackRegions[i]) {
	    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\tapWritebackRegions[%02d]=x%08x\n", i, data->apWritebackRegions[i]);
	    PRINT_ARRAY_BYTE(ctx->bufs_writeback.virtual_addr, 64);
	} else {
	    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\tapWritebackRegions[%02d]=x%08x = {	}\n", i, data->apWritebackRegions[i]);
	}
    }

    psb_buffer_unmap(&(ps_mem->bufs_mtx_context));
    psb_buffer_unmap(&(ctx->bufs_writeback));
    return 0;
}

int apSliceParamsTemplates_dump(context_ENC_p ctx, IMG_UINT32 ui32StreamIndex, IMG_UINT32 ui32SliceBufIdx)
{
    //IMG_FRAME_TEMPLATE_TYPE eSliceType = (IMG_FRAME_TEMPLATE_TYPE)ui32SliceType;
    context_ENC_mem *ps_mem = &(ctx->ctx_mem[ui32StreamIndex]);
    SLICE_PARAMS *p = NULL;

    psb_buffer_map(&(ps_mem->bufs_slice_template), &(ps_mem->bufs_slice_template.virtual_addr));
    if (ps_mem->bufs_slice_template.virtual_addr == NULL) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s error: mapping slice template\n", __FUNCTION__);
        return 0;
    }

    p = (SLICE_PARAMS*)(ps_mem->bufs_slice_template.virtual_addr + (ctx->ctx_mem_size.slice_template * ui32SliceBufIdx));

    unsigned char *ptmp = (unsigned char*)&p->sSliceHdrTmpl;
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\tui32Flags=0x%08x\n", p->ui32Flags);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\tui32SliceConfig=0x%08x\n", p->ui32SliceConfig);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\tui32IPEControl=0x%08x\n", p->ui32IPEControl);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\tui32SeqConfig=0x%08x\n", p->ui32SeqConfig);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\teTemplateType=%s\n", IMG_FRAME_TEMPLATE_TYPE2Str(p->eTemplateType));

    //PRINT_ARRAY_BYTE(ptmp, 64);

    MTX_HEADER_PARAMS_dump(&p->sSliceHdrTmpl);

    psb_buffer_unmap(&(ps_mem->bufs_slice_template));
    
    return 0;
}

static int apPicHdrTemplates_dump(context_ENC_p ctx, IMG_UINT32 ui32StreamIndex, IMG_UINT32 count)
{
    uint32_t i;
    context_ENC_mem *ps_mem = &(ctx->ctx_mem[ui32StreamIndex]);
    MTX_HEADER_PARAMS *data;

    psb_buffer_map(&(ps_mem->bufs_pic_template), &(ps_mem->bufs_pic_template.virtual_addr));
    if (ps_mem->bufs_pic_template.virtual_addr == NULL) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s error: mapping pic template\n", __FUNCTION__);
        return 0;
    }

    for (i = 0; i < count; i++) {
        data = (MTX_HEADER_PARAMS *)(ps_mem->bufs_pic_template.virtual_addr + ctx->ctx_mem_size.pic_template * i);
        drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\tapPicHdrTemplates[%02d]=0x%08x  {\n", i, data);
        PRINT_ARRAY_BYTE(data, 64);                        \
        drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t}\n");
    }

    data = (MTX_HEADER_PARAMS *)ps_mem->bufs_pic_template.virtual_addr;
    MTX_HEADER_PARAMS_dump(data);

    psb_buffer_unmap(&(ps_mem->bufs_pic_template));
    return 0;
}

static int auui32SliceMap_dump(context_ENC_p ctx)
{
    int i;
    context_ENC_mem* ps_mem = &(ctx->ctx_mem[0]);
    context_ENC_mem_size *ps_mem_size = &(ctx->ctx_mem_size);
    IMG_MTX_VIDEO_CONTEXT* data = NULL;

    psb_buffer_map(&(ps_mem->bufs_mtx_context), &(ps_mem->bufs_mtx_context.virtual_addr));
    if (ps_mem->bufs_mtx_context.virtual_addr == NULL) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s error: mapping mtx context\n", __FUNCTION__);
        return 0;
    }
    data = (IMG_MTX_VIDEO_CONTEXT*)(ps_mem->bufs_mtx_context.virtual_addr);
    if (data == NULL) {
        drv_debug_msg(VIDEO_DEBUG_ERROR,"%s data pointer is NULL\n", __FUNCTION__);
        return 0;
    }

    data = (IMG_MTX_VIDEO_CONTEXT*)(ps_mem->bufs_mtx_context.virtual_addr);

    psb_buffer_map(&(ps_mem->bufs_slice_map), &(ps_mem->bufs_slice_map.virtual_addr));
    if (ps_mem->bufs_slice_map.virtual_addr == NULL) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s error: mapping slice map buf\n", __FUNCTION__);
        return 0;
    }

    for (i = 0; i < 9; i++) {
	if (dump_address_content && data->aui32SliceMap[i]) {
	    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\taui32SliceMap[%02d]=x%08x\n", i, data->aui32SliceMap[i]);
	    PRINT_ARRAY_BYTE(ps_mem->bufs_slice_map.virtual_addr, 64);
	} else {
	    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\taui32SliceMap[%02d]=x%08x = {	}\n", i, data->aui32SliceMap[i]);
	}
    }

    psb_buffer_unmap(&(ps_mem->bufs_mtx_context));
    psb_buffer_unmap(&(ps_mem->bufs_slice_map));
    return 0;
}

static int apSeqHeader_dump(context_ENC_p ctx, IMG_UINT32 ui32StreamIndex)
{
    MTX_HEADER_PARAMS *data;
    context_ENC_mem *ps_mem = &(ctx->ctx_mem[ui32StreamIndex]);
    tng_cmdbuf_p cmdbuf = ctx->obj_context->tng_cmdbuf;
    IMG_RC_PARAMS *psRCParams = &(ctx->sRCParams);
    H264_VUI_PARAMS *psVuiParams = &(ctx->sVuiParams);

    psb_buffer_map(&(ps_mem->bufs_seq_header), &(ps_mem->bufs_seq_header.virtual_addr));
    if (ps_mem->bufs_seq_header.virtual_addr == NULL) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s error: mapping seq header\n", __FUNCTION__);
        return 0;
    }

    data = (MTX_HEADER_PARAMS *)ps_mem->bufs_seq_header.virtual_addr;

    DO_HEADER_dump(data);

    psb_buffer_unmap(&(ps_mem->bufs_seq_header));
    return 0;
}

static int pFirstPassOutParamAddr_dump(context_ENC_p ctx)
{
    int i;
    context_ENC_mem* ps_mem = &(ctx->ctx_mem[0]);
    context_ENC_mem_size *ps_mem_size = &(ctx->ctx_mem_size);
    IMG_MTX_VIDEO_CONTEXT* data= NULL;

    psb_buffer_map(&(ps_mem->bufs_mtx_context), &(ps_mem->bufs_mtx_context.virtual_addr));
    if (ps_mem->bufs_mtx_context.virtual_addr == NULL) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s error: mapping mtx ctx buf\n", __FUNCTION__);
        return 0;
    }

    data = (IMG_MTX_VIDEO_CONTEXT*)(ps_mem->bufs_mtx_context.virtual_addr);

    // if enabled, return the input-control buffer corresponding to this slot
    psb_buffer_map(&(ps_mem->bufs_first_pass_out_params), &(ps_mem->bufs_first_pass_out_params.virtual_addr));
    if (ps_mem->bufs_first_pass_out_params.virtual_addr == NULL) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s error: mapping first pass out param buf\n", __FUNCTION__);
        return 0;
    }

    for (i=0; i < 9; i++) {
        if (dump_address_content && data->pFirstPassOutParamAddr[i]) {
	    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\tpFirstPassOutParamAddr[%02d]=x%08x\n", i, data->pFirstPassOutParamAddr[i]);
            PRINT_ARRAY_BYTE(ps_mem->bufs_first_pass_out_params.virtual_addr, 64);
        } else {
	    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\tpFirstPassOutParamAddr[%02d]=x%08x = {	}\n", i, data->pFirstPassOutParamAddr[i]);
        }
    }

    psb_buffer_unmap(&(ps_mem->bufs_mtx_context));
    psb_buffer_unmap(&(ps_mem->bufs_first_pass_out_params));
    return 0;
}

static int pFirstPassOutBestMultipassParamAddr_dump(context_ENC_p ctx)
{
    int i;
    context_ENC_mem* ps_mem = &(ctx->ctx_mem[0]);
    context_ENC_mem_size *ps_mem_size = &(ctx->ctx_mem_size);
    IMG_MTX_VIDEO_CONTEXT* data= NULL;

    psb_buffer_map(&(ps_mem->bufs_mtx_context), &(ps_mem->bufs_mtx_context.virtual_addr));
    if (ps_mem->bufs_mtx_context.virtual_addr == NULL) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s error: mapping mtx ctx buf\n", __FUNCTION__);
        return 0;
    }

    data = (IMG_MTX_VIDEO_CONTEXT*)(ps_mem->bufs_mtx_context.virtual_addr);

    // if enabled, return the input-control buffer corresponding to this slot
    psb_buffer_map(&(ps_mem->bufs_first_pass_out_best_multipass_param), &(ps_mem->bufs_first_pass_out_best_multipass_param.virtual_addr));
    if (ps_mem->bufs_first_pass_out_best_multipass_param.virtual_addr == NULL) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s error: mapping first pass out param buf\n", __FUNCTION__);
        return 0;
    }

    for (i=0; i < 9; i++) {
        if (dump_address_content && data->pFirstPassOutBestMultipassParamAddr[i]) {
	    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\tpFirstPassOutBestMultipassParamAddr[%02d]=x%08x\n", i, data->pFirstPassOutBestMultipassParamAddr[i]);
            PRINT_ARRAY_BYTE(ps_mem->bufs_first_pass_out_best_multipass_param.virtual_addr, 64);
        } else {
	    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\tpFirstPassOutBestMultipassParamAddr[%02d]=x%08x = {	}\n", i, data->pFirstPassOutBestMultipassParamAddr[i]);
        }
    }

    psb_buffer_unmap(&(ps_mem->bufs_mtx_context));
    psb_buffer_unmap(&(ps_mem->bufs_first_pass_out_best_multipass_param));
    return 0;
}

static int pMBCtrlInParamsAddr_dump(context_ENC_p ctx, IMG_UINT32 __maybe_unused ui32StreamIndex)
{
    int i;
    context_ENC_mem* ps_mem = &(ctx->ctx_mem[0]);
    context_ENC_mem_size *ps_mem_size = &(ctx->ctx_mem_size);
    IMG_MTX_VIDEO_CONTEXT* data= NULL;

    psb_buffer_map(&(ps_mem->bufs_mtx_context), &(ps_mem->bufs_mtx_context.virtual_addr));
    if (ps_mem->bufs_mtx_context.virtual_addr == NULL) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s error: mapping mtx ctx buf\n", __FUNCTION__);
        return 0;
    }

    data = (IMG_MTX_VIDEO_CONTEXT*)(ps_mem->bufs_mtx_context.virtual_addr);

    // if enabled, return the input-control buffer corresponding to this slot
    psb_buffer_map(&(ps_mem->bufs_mb_ctrl_in_params), &(ps_mem->bufs_mb_ctrl_in_params.virtual_addr));
    if (ps_mem->bufs_mb_ctrl_in_params.virtual_addr == NULL) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s error: mapping mb ctrl buf\n", __FUNCTION__);
        return 0;
    }

    for (i=0; i < 9; i++) {
        if (dump_address_content && data->pMBCtrlInParamsAddr[i]) {
	    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\tpMBCtrlInParamsAddr[%02d]=x%08x\n", i, data->pMBCtrlInParamsAddr[i]);
            PRINT_ARRAY_BYTE(ps_mem->bufs_mb_ctrl_in_params.virtual_addr, 64);
        } else {
	    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\tpMBCtrlInParamsAddr[%02d]=x%08x = {	}\n", i, data->pMBCtrlInParamsAddr[i]);
        }
    }

    psb_buffer_unmap(&(ps_mem->bufs_mtx_context));
    psb_buffer_unmap(&(ps_mem->bufs_mb_ctrl_in_params));
    return 0;
}

static int apAboveParams_dump(context_ENC_p ctx)
{
    int i;
    context_ENC_mem* ps_mem = &(ctx->ctx_mem[0]);
    context_ENC_mem_size *ps_mem_size = &(ctx->ctx_mem_size);
    IMG_MTX_VIDEO_CONTEXT* data= NULL;

    psb_buffer_map(&(ps_mem->bufs_mtx_context), &(ps_mem->bufs_mtx_context.virtual_addr));
    if (ps_mem->bufs_mtx_context.virtual_addr == NULL) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s error: mapping mtx ctx buf\n", __FUNCTION__);
        return 0;
    }

    data = (IMG_MTX_VIDEO_CONTEXT*)(ps_mem->bufs_mtx_context.virtual_addr);

    // if enabled, return the input-control buffer corresponding to this slot
    psb_buffer_map(&(ps_mem->bufs_above_params), &(ps_mem->bufs_above_params.virtual_addr));
    if (ps_mem->bufs_above_params.virtual_addr == NULL) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s error: mapping above param ctrl buf\n", __FUNCTION__);
        return 0;
    }

    for (i=0; i < 2; i++) {
        if (dump_address_content && data->apAboveParams[i]) {
	    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\tapAboveParams[%02d]=x%08x\n", i, data->apAboveParams[i]);
            PRINT_ARRAY_BYTE(ps_mem->bufs_above_params.virtual_addr, 64);
        } else {
	    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\tapAboveParams[%02d]=x%08x = {	}\n", i, data->apAboveParams[i]);
        }
    }

    psb_buffer_unmap(&(ps_mem->bufs_mtx_context));
    psb_buffer_unmap(&(ps_mem->bufs_above_params));
    return 0;
}

static int aui32LTRefHeader_dump(context_ENC_p ctx)
{
    int i;
    context_ENC_mem* ps_mem = &(ctx->ctx_mem[0]);
    context_ENC_mem_size *ps_mem_size = &(ctx->ctx_mem_size);
    IMG_MTX_VIDEO_CONTEXT* data = NULL;

    psb_buffer_map(&(ps_mem->bufs_mtx_context), &(ps_mem->bufs_mtx_context.virtual_addr));
    if (ps_mem->bufs_mtx_context.virtual_addr == NULL) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s error: mapping mtx context\n", __FUNCTION__);
        return 0;
    }
    data = (IMG_MTX_VIDEO_CONTEXT*)(ps_mem->bufs_mtx_context.virtual_addr);
    if (data == NULL) {
        drv_debug_msg(VIDEO_DEBUG_ERROR,"%s data pointer is NULL\n", __FUNCTION__);
        return 0;
    }

    data = (IMG_MTX_VIDEO_CONTEXT*)(ps_mem->bufs_mtx_context.virtual_addr);

    psb_buffer_map(&(ps_mem->bufs_lt_ref_header), &(ps_mem->bufs_lt_ref_header.virtual_addr));
    if (ps_mem->bufs_lt_ref_header.virtual_addr == NULL) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s error: mapping lt ref buf\n", __FUNCTION__);
        return 0;
    }

    for (i = 0; i < 9; i++) {
	if (dump_address_content && data->aui32LTRefHeader[i]) {
	    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\taui32LTRefHeader[%02d]=x%08x\n", i, data->aui32LTRefHeader[i]);
	    PRINT_ARRAY_BYTE(ps_mem->bufs_lt_ref_header.virtual_addr, 64);
	} else {
	    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\taui32LTRefHeader[%02d]=x%08x = {	}\n", i, data->aui32LTRefHeader[i]);
	}
    }

    psb_buffer_unmap(&(ps_mem->bufs_mtx_context));
    psb_buffer_unmap(&(ps_mem->bufs_lt_ref_header));
    return 0;
}

void tng_trace_setvideo(context_ENC_p ctx, IMG_UINT32 ui32StreamID)
{
    unsigned int i;

    context_ENC_mem *ps_mem = &(ctx->ctx_mem[ui32StreamID]);
    context_ENC_mem_size *ps_mem_size = &(ctx->ctx_mem_size);
    IMG_MTX_VIDEO_CONTEXT* data= NULL;

    psb_buffer_map(&(ps_mem->bufs_mtx_context), &(ps_mem->bufs_mtx_context.virtual_addr));
    if (ps_mem->bufs_mtx_context.virtual_addr == NULL) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s error: mapping mtx context\n", __FUNCTION__);
        return ;
    }

    data = (IMG_MTX_VIDEO_CONTEXT*)(ps_mem->bufs_mtx_context.virtual_addr);

    if (data == NULL) {
        drv_debug_msg(VIDEO_DEBUG_ERROR,"%s data pointer is NULL\n", __FUNCTION__);
        return ;
    }

    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t==========IMG_MTX_VIDEO_CONTEXT=============\n");
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui64ClockDivBitrate=%lld\n", data->ui64ClockDivBitrate);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui32WidthInMbs=%d\n", data->ui32WidthInMbs);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui32PictureHeightInMbs=%d\n", data->ui32PictureHeightInMbs);
#ifdef FORCED_REFERENCE
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	apTmpReconstructured  {");
    PRINT_ARRAY_ADDR("apTmpReconstructured",	apTmpReconstructured, MAX_PIC_NODES);
#endif
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	apReconstructured  {\n");
    apReconstructured_dump(ctx);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	apColocated  {\n");
    apColocated_dump(ctx);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	apMV  {\n");
    apPV_dump(ctx);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	apInterViewMV  {");
//    PRINT_ARRAY(	apInterViewMV, 2 );
    PRINT_ARRAY_ADDR("apInterViewMV", apInterViewMV, 2);	

    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui32DebugCRCs=0x%x\n", data->ui32DebugCRCs);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	apWritebackRegions  {\n");
    apWritebackRegions_dump(ctx);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui32InitialCPBremovaldelayoffset=0x%x\n", data->ui32InitialCPBremovaldelayoffset);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui32MaxBufferMultClockDivBitrate=0x%x\n", data->ui32MaxBufferMultClockDivBitrate);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	pSEIBufferingPeriodTemplate=0x%x\n", data->pSEIBufferingPeriodTemplate);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	pSEIPictureTimingTemplate=0x%x\n", data->pSEIPictureTimingTemplate);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	b16EnableMvc=%d\n", data->b16EnableMvc);
    //drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	b16EnableInterViewReference=%d\n", data->b16EnableInterViewReference);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui16MvcViewIdx=0x%x\n", data->ui16MvcViewIdx);

    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	apSliceParamsTemplates  {\n");
    //PRINT_ARRAY_ADDR( apSliceParamsTemplates, 5);
    for (i=0; i<5; i++) {
        drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\tapSliceParamsTemplates[%d]=0x%08x  {\n", i, data->apSliceParamsTemplates[i]);
	apSliceParamsTemplates_dump(ctx, 0, i);
        drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t}\n");
    }

    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	apPicHdrTemplates  {\n");
    apPicHdrTemplates_dump(ctx, 0, 5);

    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	aui32SliceMap  {\n");
    auui32SliceMap_dump(ctx);

    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui32FlatGopStruct=0x%x\n", data->ui32FlatGopStruct);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	apSeqHeader        =0x%x\n", data->apSeqHeader);
    apSeqHeader_dump(ctx, 0);

    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	apSubSetSeqHeader  =0x%x\n", data->apSubSetSeqHeader);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	b16NoSequenceHeaders =0x%x\n", data->b16NoSequenceHeaders);
    
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	b8WeightedPredictionEnabled=%d\n", data->b8WeightedPredictionEnabled);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui8MTXWeightedImplicitBiPred=0x%x\n", data->ui8MTXWeightedImplicitBiPred);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	aui32WeightedPredictionVirtAddr  {");
    PRINT_ARRAY(aui32WeightedPredictionVirtAddr, MAX_SOURCE_SLOTS_SL);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui32HierarGopStruct=0x%x\n", data->ui32HierarGopStruct);

    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	pFirstPassOutParamAddr  {\n");
    pFirstPassOutParamAddr_dump(ctx);
#ifndef EXCLUDE_BEST_MP_DECISION_DATA
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	pFirstPassOutBestMultipassParamAddr  {");
    pFirstPassOutBestMultipassParamAddr_dump(ctx);
#endif
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	pMBCtrlInParamsAddr  {\n");
    pMBCtrlInParamsAddr_dump(ctx, 0);

    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui32InterIntraScale{");
    PRINT_ARRAY( ui32InterIntraScale, SCALE_TBL_SZ);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui32SkippedCodedScale  {");
    PRINT_ARRAY( ui32SkippedCodedScale, SCALE_TBL_SZ);


    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui32PicRowStride=0x%x\n", data->ui32PicRowStride);

    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	apAboveParams  {\n");
    apAboveParams_dump(ctx);

    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui32IdrPeriod =0x%x\n ", data->ui32IdrPeriod);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui32IntraLoopCnt =0x%x\n", data->ui32IntraLoopCnt);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui32BFrameCount =0x%x\n", data->ui32BFrameCount);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	b8Hierarchical=%d\n", data->b8Hierarchical);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui8MPEG2IntraDCPrecision =0x%x\n", data->ui8MPEG2IntraDCPrecision);

    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	aui8PicOnLevel  {");
    PRINT_ARRAY(aui8PicOnLevel, MAX_REF_LEVELS);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui32VopTimeResolution=0x%x\n", data->ui32VopTimeResolution);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui32InitialQp=0x%x\n", data->ui32InitialQp);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui32BUSize=0x%x\n", data->ui32BUSize);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	sMVSettingsIdr { \n\t\t\tui32MVCalc_Below=0x%x\n \t\t\tui32MVCalc_Colocated=0x%x\n \t\t\tui32MVCalc_Config=0x%x\n \t\t}\n", data->sMVSettingsIdr.ui32MVCalc_Below,data->sMVSettingsIdr.ui32MVCalc_Colocated, data->sMVSettingsIdr.ui32MVCalc_Config);

    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	sMVSettingsNonB { \n");
    for(i=0;i<MAX_BFRAMES +1;i++)
        drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t\tui32MVCalc_Below=0x%x   ui32MVCalc_Colocated=0x%x   ui32MVCalc_Config=0x%x }\n", data->sMVSettingsNonB[i].ui32MVCalc_Below,data->sMVSettingsNonB[i].ui32MVCalc_Colocated, data->sMVSettingsNonB[i].ui32MVCalc_Config);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t}\n");

    drv_debug_msg(VIDEO_ENCODE_PDUMP," \t	ui32MVSettingsBTable=0x%x\n", data->ui32MVSettingsBTable);
    drv_debug_msg(VIDEO_ENCODE_PDUMP," \t	ui32MVSettingsHierarchical=0x%x\n", data->ui32MVSettingsHierarchical);
    
#ifdef FIRMWARE_BIAS
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	aui32DirectBias_P  {");
    PRINT_ARRAY_NEW(aui32DirectBias_P,27 );
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	aui32InterBias_P  {");
    PRINT_ARRAY_NEW(aui32InterBias_P,27 );
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	aui32DirectBias_B  {");
    PRINT_ARRAY_NEW(aui32DirectBias_B,27 );
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	aui32InterBias_B  {");
    PRINT_ARRAY_NEW(aui32InterBias_B,27 );
#endif
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	eFormat=%d\n", data->eFormat);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	eStandard=%d\n", data->eStandard);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	eRCMode=%d\n", data->eRCMode);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	b8FirstPic=%d\n", data->b8FirstPic);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	b8IsInterlaced=%d\n", data->b8IsInterlaced);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	b8TopFieldFirst=%d\n", data->b8TopFieldFirst);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	b8ArbitrarySO=%d\n", data->b8ArbitrarySO);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	bOutputReconstructed=%d\n", data->bOutputReconstructed);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	b8DisableBitStuffing=%d\n", data->b8DisableBitStuffing);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	b8InsertHRDparams=%d\n", data->b8InsertHRDparams);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui8MaxSlicesPerPicture=%d\n", data->ui8MaxSlicesPerPicture);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui8NumPipes=%d\n", data->ui8NumPipes);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	bCARC=%d\n", data->bCARC);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	iCARCBaseline=%d\n", data->iCARCBaseline);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	uCARCThreshold=%d\n", data->uCARCThreshold);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	uCARCCutoff=%d\n", data->uCARCCutoff);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	uCARCNegRange=%d\n", data->uCARCNegRange);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	uCARCNegScale=%d\n", data->uCARCNegScale);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	uCARCPosRange=%d\n", data->uCARCPosRange);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	uCARCPosScale=%d\n", data->uCARCPosScale);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	uCARCShift=%d\n", data->uCARCShift);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui32MVClip_Config=%d\n", data->ui32MVClip_Config);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui32PredCombControl=%d\n", data->ui32PredCombControl);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui32LRITC_Tile_Use_Config=%d\n", data->ui32LRITC_Tile_Use_Config);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui32LRITC_Cache_Chunk_Config=%d\n", data->ui32LRITC_Cache_Chunk_Config);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui32IPEVectorClipping=%d\n", data->ui32IPEVectorClipping);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui32H264CompControl=%d\n", data->ui32H264CompControl);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui32H264CompIntraPredModes=%d\n", data->ui32H264CompIntraPredModes);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui32IPCM_0_Config=%d\n", data->ui32IPCM_0_Config);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui32IPCM_1_Config=%d\n", data->ui32IPCM_1_Config);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui32SPEMvdClipRange=%d\n", data->ui32SPEMvdClipRange);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui32JMCompControl=%d\n", data->ui32JMCompControl);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui32MBHostCtrl=%d\n", data->ui32MBHostCtrl);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui32DeblockCtrl=%d\n", data->ui32DeblockCtrl);
    
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui32SkipCodedInterIntra=%d\n", data->ui32SkipCodedInterIntra);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui32VLCControl=%d\n", data->ui32VLCControl);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui32VLCSliceControl=%d\n", data->ui32VLCSliceControl);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui32VLCSliceMBControl=%d\n", data->ui32VLCSliceMBControl);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui16CQPOffset=%d\n", data->ui16CQPOffset);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	b8CodedHeaderPerSlice=%d\n", data->b8CodedHeaderPerSlice);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui32FirstPicFlags=%d\n", data->ui32FirstPicFlags);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui32NonFirstPicFlags=%d\n", data->ui32NonFirstPicFlags);

#ifndef EXCLUDE_ADAPTIVE_ROUNDING
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	bMCAdaptiveRoundingDisable=%d\n",data->bMCAdaptiveRoundingDisable);
    int j;
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui16MCAdaptiveRoundingOffsets[18][4]");
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\n");
    for(i=0;i<18;i++){
        for(j=0;j<4;j++)
            drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t\t0x%x", data-> ui16MCAdaptiveRoundingOffsets[i][j]);
        drv_debug_msg(VIDEO_ENCODE_PDUMP,"\n");
    }
#endif

#ifdef FORCED_REFERENCE
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui32PatchedReconAddress=0x%x\n", data->ui32PatchedReconAddress);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui32PatchedRef0Address=0x%x\n", data->ui32PatchedRef0Address);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui32PatchedRef1Address=0x%x\n", data->ui32PatchedRef1Address);
#endif
#ifdef LTREFHEADER
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	aui32LTRefHeader  {\n");
    aui32LTRefHeader_dump(ctx);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	i8SliceHeaderSlotNum=%d\n",data->i8SliceHeaderSlotNum);
#endif
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	b8ReconIsLongTerm=%d\n", data->b8ReconIsLongTerm);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	b8Ref0IsLongTerm=%d\n", data->b8Ref0IsLongTerm);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	b8Ref1IsLongTerm=%d\n", data->b8Ref1IsLongTerm);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui8RefSpacing=0x%x\n", data->ui8RefSpacing);

    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui8FirstPipe=0x%x\n", data->ui8FirstPipe);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui8LastPipe=0x%x\n", data->ui8LastPipe);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui8PipesToUseFlags=0x%x\n", data->ui8PipesToUseFlags);
    
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	sInParams {\n");
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	ui16MBPerFrm=%d\n",data->sInParams.ui16MBPerFrm);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	ui16MBPerBU=%d\n", data->sInParams.ui16MBPerBU);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	ui16BUPerFrm=%d\n",data->sInParams.ui16BUPerFrm);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	ui16IntraPerio=%d\n",data->sInParams.ui16IntraPeriod);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	ui16BFrames=%d\n", data->sInParams.ui16BFrames);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	bHierarchicalMode=%d\n",data->sInParams.mode.h264.bHierarchicalMode);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	i32BitsPerFrm=%d\n",   data->sInParams.i32BitsPerFrm);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	i32BitsPerBU=%d\n",    data->sInParams.i32BitsPerBU);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	i32BitsPerMB=%d\n",    data->sInParams.mode.other.i32BitsPerMB);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	i32BitRate=%d\n",data->sInParams.i32BitRate);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	i32BufferSiz=%d\n",data->sInParams.i32BufferSize );
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	i32InitialLevel=%d\n", data->sInParams.i32InitialLevel);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	i32InitialDelay=%d\n", data->sInParams.i32InitialDelay);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	i32BitsPerGOP=%d\n",   data->sInParams.mode.other.i32BitsPerGOP);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	ui16AvQPVal=%d\n", data->sInParams.mode.other.ui16AvQPVal);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	ui16MyInitQP=%d\n",data->sInParams.mode.other.ui16MyInitQP);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	ui32RCScaleFactor=%d\n",data->sInParams.mode.h264.ui32RCScaleFactor);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	bScDetectDis;=%d\n", data->sInParams.mode.h264.bScDetectDisable);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	bFrmSkipDisable=%d\n",data->sInParams.bFrmSkipDisable);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	bBUSkipDisable=%d\n",data->sInParams.mode.other.bBUSkipDisable);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	ui8SeInitQP=%d\n",    data->sInParams.ui8SeInitQP	);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	ui8MinQPVal=%d\n",    data->sInParams.ui8MinQPVal	);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	ui8MaxQPVal=%d\n",    data->sInParams.ui8MaxQPVal	);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	ui8MBPerRow=%d\n",    data->sInParams.ui8MBPerRow	);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	ui8ScaleFactor=%d\n",  data->sInParams.ui8ScaleFactor);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	ui8HalfFrame=%d\n",    data->sInParams.mode.other.ui8HalfFrameRate);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	ui8FCode=%d\n",        data->sInParams.mode.other.ui8FCode);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	i32TransferRate=%d\n",data->sInParams.mode.h264.i32TransferRate);
    
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t}\n");

    psb_buffer_unmap(&(ps_mem->bufs_mtx_context));

    return;
}

struct header_token {
    int token;
    char *str;
} header_tokens[] = {
    {ELEMENT_STARTCODE_RAWDATA,"ELEMENT_STARTCODE_RAWDATA=0"},
    {ELEMENT_STARTCODE_MIDHDR,"ELEMENT_STARTCODE_MIDHDR"},
    {ELEMENT_RAWDATA,"ELEMENT_RAWDATA"},
    {ELEMENT_QP,"ELEMENT_QP"},
    {ELEMENT_SQP,"ELEMENT_SQP"},
    {ELEMENT_FRAMEQSCALE,"ELEMENT_FRAMEQSCALE"},
    {ELEMENT_SLICEQSCALE,"ELEMENT_SLICEQSCALE"},
    {ELEMENT_INSERTBYTEALIGN_H264,"ELEMENT_INSERTBYTEALIGN_H264"},
    {ELEMENT_INSERTBYTEALIGN_MPG4,"ELEMENT_INSERTBYTEALIGN_MPG4"},
    {ELEMENT_INSERTBYTEALIGN_MPG2,"ELEMENT_INSERTBYTEALIGN_MPG2"},
    {ELEMENT_VBV_MPG2,"ELEMENT_VBV_MPG2"},
    {ELEMENT_TEMPORAL_REF_MPG2,"ELEMENT_TEMPORAL_REF_MPG2"},
    {ELEMENT_CURRMBNR,"ELEMENT_CURRMBNR"},
    {ELEMENT_FRAME_NUM,"ELEMENT_FRAME_NUM"},
    {ELEMENT_TEMPORAL_REFERENCE,"ELEMENT_TEMPORAL_REFERENCE"},
    {ELEMENT_EXTENDED_TR,"ELEMENT_EXTENDED_TR"},
    {ELEMENT_IDR_PIC_ID,"ELEMENT_IDR_PIC_ID"},
    {ELEMENT_PIC_ORDER_CNT,"ELEMENT_PIC_ORDER_CNT"},
    {ELEMENT_GOB_FRAME_ID,"ELEMENT_GOB_FRAME_ID"},
    {ELEMENT_VOP_TIME_INCREMENT,"ELEMENT_VOP_TIME_INCREMENT"},
    {ELEMENT_MODULO_TIME_BASE,"ELEMENT_MODULO_TIME_BASE"},
    {ELEMENT_BOTTOM_FIELD,"ELEMENT_BOTTOM_FIELD"},
    {ELEMENT_SLICE_NUM,"ELEMENT_SLICE_NUM"},
    {ELEMENT_MPEG2_SLICE_VERTICAL_POS,"ELEMENT_MPEG2_SLICE_VERTICAL_POS"},
    {ELEMENT_MPEG2_IS_INTRA_SLICE,"ELEMENT_MPEG2_IS_INTRA_SLICE"},
    {ELEMENT_MPEG2_PICTURE_STRUCTURE,"ELEMENT_MPEG2_PICTURE_STRUCTURE"},
    {ELEMENT_REFERENCE,"ELEMENT_REFERENCE"},
    {ELEMENT_ADAPTIVE,"ELEMENT_ADAPTIVE"},
    {ELEMENT_DIRECT_SPATIAL_MV_FLAG,"ELEMENT_DIRECT_SPATIAL_MV_FLAG"},
    {ELEMENT_NUM_REF_IDX_ACTIVE,"ELEMENT_NUM_REF_IDX_ACTIVE"},
    {ELEMENT_REORDER_L0,"ELEMENT_REORDER_L0"},
    {ELEMENT_REORDER_L1,"ELEMENT_REORDER_L1"},
    {ELEMENT_TEMPORAL_ID,"ELEMENT_TEMPORAL_ID"},
    {ELEMENT_ANCHOR_PIC_FLAG,"ELEMENT_ANCHOR_PIC_FLAG"},
    {BPH_SEI_NAL_INITIAL_CPB_REMOVAL_DELAY,"BPH_SEI_NAL_INITIAL_CPB_REMOVAL_DELAY"},
    {BPH_SEI_NAL_INITIAL_CPB_REMOVAL_DELAY_OFFSET,"BPH_SEI_NAL_INITIAL_CPB_REMOVAL_DELAY_OFFSET"},
    {PTH_SEI_NAL_CPB_REMOVAL_DELAY,"PTH_SEI_NAL_CPB_REMOVAL_DELAY"},
    {PTH_SEI_NAL_DPB_OUTPUT_DELAY,"PTH_SEI_NAL_DPB_OUTPUT_DELAY"},
    {ELEMENT_SLICEWEIGHTEDPREDICTIONSTRUCT,"ELEMENT_SLICEWEIGHTEDPREDICTIONSTRUCT"},
    {ELEMENT_CUSTOM_QUANT,"ELEMENT_CUSTOM_QUANT"}
};

static char *header_to_str(int token)
{
    unsigned int i;
    struct header_token *p;
    
    for (i=0; i<sizeof(header_tokens)/sizeof(struct header_token); i++) {
        p = &header_tokens[i];
        if (p->token == token)
            return p->str;
    }

    return "Invalid header token";
}

static int MTX_HEADER_PARAMS_dump(MTX_HEADER_PARAMS *p)
{
    MTX_HEADER_ELEMENT *last_element=NULL;
    unsigned int i;
    
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\tui32Elements=%d\n", p->ui32Elements);
    for (i=0; i<p->ui32Elements; i++) {
        MTX_HEADER_ELEMENT *q = &(p->asElementStream[0]);

        if (last_element) {
            int ui8Offset = 0;
            IMG_UINT8 *ui8P;
            
            if (last_element->Element_Type==ELEMENT_STARTCODE_RAWDATA ||
                last_element->Element_Type==ELEMENT_RAWDATA ||
                last_element->Element_Type==ELEMENT_STARTCODE_MIDHDR)
            {
                //Add a new element aligned to word boundary
                //Find RAWBit size in bytes (rounded to word boundary))
                ui8Offset=last_element->ui8Size+8+31; // NumberofRawbits (excluding size of bit count field)+ size of the bitcount field
                ui8Offset/=32; //Now contains rawbits size in words
                ui8Offset+=1; //Now contains rawbits+element_type size in words
                ui8Offset*=4; //Convert to number of bytes (total size of structure in bytes, aligned to word boundary).
            }
            else
            {
                ui8Offset=4;
            }
            ui8P=(IMG_UINT8 *) last_element;
            ui8P+=ui8Offset;
            q=(MTX_HEADER_ELEMENT *) ui8P;
        }
        
        drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t----Head %d----\n",i);
        drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t\tElement_Type=%d(0x%x:%s)\n",
               q->Element_Type, q->Element_Type, header_to_str(q->Element_Type));

        if (q->Element_Type==ELEMENT_STARTCODE_RAWDATA ||
            q->Element_Type==ELEMENT_RAWDATA ||
            q->Element_Type==ELEMENT_STARTCODE_MIDHDR) {
            int i, ui8Offset = 0;
            IMG_UINT8 *ui8P;
            
            drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t\tui8Size=%d(0x%x)\n", q->ui8Size, q->ui8Size);
            drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t\t(====aui8Bits===)");
            
            //Find RAWBit size in bytes (rounded to word boundary))
            ui8Offset=q->ui8Size+8+31; // NumberofRawbits (excluding size of bit count field)+ size of the bitcount field
            ui8Offset/=32; //Now contains rawbits size in words
            //ui8Offset+=1; //Now contains rawbits+element_type size in words
            ui8Offset*=4; //Convert to number of bytes (total size of structure in bytes, aligned to word boundar
            
            ui8P = &q->aui8Bits;
            for (i=0; i<ui8Offset; i++) {
                if ((i%8) == 0)
                    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\n\t\t\t");
                drv_debug_msg(VIDEO_ENCODE_PDUMP,"0x%02x\t", *ui8P);
                ui8P++;
            }
            drv_debug_msg(VIDEO_ENCODE_PDUMP,"\n");
        } else {
            drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t\t(no ui8Size/aui8Bits for this type header)\n");
        }
        
        last_element = q;
    }

    return 0;
}

static char *eBufferType2str(IMG_REF_BUFFER_TYPE tmp)
{
    switch (tmp) {
    case IMG_BUFFER_REF0:
        return "IMG_BUFFER_REF0";
    case IMG_BUFFER_REF1:
        return "IMG_BUFFER_REF1";
    case IMG_BUFFER_RECON:
        return "IMG_BUFFER_RECON";
    default:
        return "Unknown Buffer Type";
    }
}

static void PROVIDEBUFFER_SOURCE_dump(void *data)
{
    IMG_SOURCE_BUFFER_PARAMS *source = (IMG_SOURCE_BUFFER_PARAMS*) data;

    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\tui32PhysAddrYPlane_Field0=0x%x\n",source->ui32PhysAddrYPlane_Field0);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\tui32PhysAddrUPlane_Field0=0x%x\n",source->ui32PhysAddrUPlane_Field0);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\tui32PhysAddrVPlane_Field0=0x%x\n",source->ui32PhysAddrVPlane_Field0);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\tui32PhysAddrYPlane_Field1=0x%x\n",source->ui32PhysAddrYPlane_Field1);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\tui32PhysAddrUPlane_Field1=0x%x\n",source->ui32PhysAddrUPlane_Field1);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\tui32PhysAddrVPlane_Field1=0x%x\n",source->ui32PhysAddrVPlane_Field1);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\tui32HostContext=%d\n",source->ui32HostContext);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\tui8DisplayOrderNum=%d\n",source->ui8DisplayOrderNum);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\tui8SlotNum=%d\n",source->ui8SlotNum);
    return ;
}

static int PROVIDEBUFFER_dump(unsigned int data)
{
    IMG_REF_BUFFER_TYPE eBufType = (data & MASK_MTX_MSG_PROVIDE_REF_BUFFER_USE) >> SHIFT_MTX_MSG_PROVIDE_REF_BUFFER_USE;
    //IMG_BUFFER_DATA *bufdata = p->sData;
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\teBufferType=(%s)\n",  eBufferType2str(eBufType));
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\n\n");
    
    return 0;
}

static char *eSubtype2str(IMG_PICMGMT_TYPE eSubtype)
{
    switch (eSubtype) {
        case IMG_PICMGMT_REF_TYPE:return "IMG_PICMGMT_REF_TYPE";
        case IMG_PICMGMT_GOP_STRUCT:return "IMG_PICMGMT_GOP_STRUCT";
        case IMG_PICMGMT_SKIP_FRAME:return "IMG_PICMGMT_SKIP_FRAME";
        case IMG_PICMGMT_EOS:return "IMG_PICMGMT_EOS";
        case IMG_PICMGMT_FLUSH:return "IMG_PICMGMT_FLUSH";
        case IMG_PICMGMT_QUANT:return "IMG_PICMGMT_QUANT";
        default: return "Unknow";
    }
}
int PICMGMT_dump(IMG_UINT32 data)
{
    IMG_PICMGMT_TYPE eSubType = (data & MASK_MTX_MSG_PICMGMT_SUBTYPE) >> SHIFT_MTX_MSG_PICMGMT_SUBTYPE;
    IMG_FRAME_TYPE eFrameType = 0;
    IMG_UINT32 ui32FrameCount = 0;

    drv_debug_msg(VIDEO_ENCODE_PDUMP,
        "\t\teSubtype=%d(%s)\n", eSubType, eSubtype2str(eSubType));
    drv_debug_msg(VIDEO_ENCODE_PDUMP,
        "\t\t(=====(additional data)=====\n");
    switch (eSubType) {
        case IMG_PICMGMT_REF_TYPE:
            eFrameType = (data & MASK_MTX_MSG_PICMGMT_DATA) >> SHIFT_MTX_MSG_PICMGMT_DATA;
            switch (eFrameType) {
                case IMG_INTRA_IDR:
                    drv_debug_msg(VIDEO_ENCODE_PDUMP,
                        "\t\teFrameType=IMG_INTRA_IDR\n");
                    break;
                case IMG_INTRA_FRAME:
                    drv_debug_msg(VIDEO_ENCODE_PDUMP,
                        "\t\teFrameType=IMG_INTRA_FRAME\n");
                    break;
                case IMG_INTER_P:
                    drv_debug_msg(VIDEO_ENCODE_PDUMP,
                        "\t\teFrameType=IMG_INTER_P\n");
                    break;
                case IMG_INTER_B:
                    drv_debug_msg(VIDEO_ENCODE_PDUMP,
                        "\t\teFrameType=IMG_INTER_B\n");
                    break;
            }
            break;
        case IMG_PICMGMT_EOS:
             ui32FrameCount = (data & MASK_MTX_MSG_PICMGMT_DATA) >> SHIFT_MTX_MSG_PICMGMT_DATA;
             drv_debug_msg(VIDEO_ENCODE_PDUMP,
                 "\t\tui32FrameCount=%d\n", ui32FrameCount);
             break;
        default:
             break;
    }
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\n\n");

    return 0;
}


static char * cmd2str(int cmdid)
{
    switch (cmdid) {
    case MTX_CMDID_NULL:        return "MTX_CMDID_NULL";
    case MTX_CMDID_SHUTDOWN:    return "MTX_CMDID_SHUTDOWN"; 
	// Video Commands
    case MTX_CMDID_DO_HEADER:   return "MTX_CMDID_DO_HEADER";
    case MTX_CMDID_ENCODE_FRAME:return "MTX_CMDID_ENCODE_FRAME";
    case MTX_CMDID_START_FRAME: return "MTX_CMDID_START_FRAME";
    case MTX_CMDID_ENCODE_SLICE:return "MTX_CMDID_ENCODE_SLICE";
    case MTX_CMDID_END_FRAME:   return "MTX_CMDID_END_FRAME";
    case MTX_CMDID_SETVIDEO:    return "MTX_CMDID_SETVIDEO";
    case MTX_CMDID_GETVIDEO:    return "MTX_CMDID_GETVIDEO";
    case MTX_CMDID_PICMGMT:     return "MTX_CMDID_PICMGMT";
    case MTX_CMDID_RC_UPDATE:   return "MTX_CMDID_RC_UPDATE";
    case MTX_CMDID_PROVIDE_SOURCE_BUFFER:return "MTX_CMDID_PROVIDE_SOURCE_BUFFER";
    case MTX_CMDID_PROVIDE_REF_BUFFER:   return "MTX_CMDID_PROVIDE_REF_BUFFER";
    case MTX_CMDID_PROVIDE_CODED_BUFFER: return "MTX_CMDID_PROVIDE_CODED_BUFFER";
    case MTX_CMDID_ABORT:       return "MTX_CMDID_ABORT";
	// JPEG commands
    case MTX_CMDID_SETQUANT:    return "MTX_CMDID_SETQUANT";         
    case MTX_CMDID_SETUP_INTERFACE:      return "MTX_CMDID_SETUP_INTERFACE"; 
    case MTX_CMDID_ISSUEBUFF:   return "MTX_CMDID_ISSUEBUFF";       
    case MTX_CMDID_SETUP:       return "MTX_CMDID_SETUP";           
    case MTX_CMDID_ENDMARKER:
    default:
        return "Invalid Command (%d)";
    }
}

void tng_H264ES_trace_seq_params(VAEncSequenceParameterBufferH264 *psTraceSeqParams)
{
    drv_debug_msg(VIDEO_DEBUG_GENERAL,"%s: level_idc = 0x%08x\n", __FUNCTION__, psTraceSeqParams->level_idc);
    drv_debug_msg(VIDEO_DEBUG_GENERAL,"%s: max_num_ref_frames = 0x%08x\n", __FUNCTION__, psTraceSeqParams->max_num_ref_frames);
    drv_debug_msg(VIDEO_DEBUG_GENERAL,"%s: intra_idr_period = 0x%08x\n", __FUNCTION__, psTraceSeqParams->intra_idr_period);
    drv_debug_msg(VIDEO_DEBUG_GENERAL,"%s: intra_period = 0x%08x\n", __FUNCTION__, psTraceSeqParams->intra_period);
    drv_debug_msg(VIDEO_DEBUG_GENERAL,"%s: ip_period = 0x%08x\n", __FUNCTION__, psTraceSeqParams->ip_period);
    drv_debug_msg(VIDEO_DEBUG_GENERAL,"%s: bits_per_second = 0x%08x\n", __FUNCTION__, psTraceSeqParams->bits_per_second);

    drv_debug_msg(VIDEO_DEBUG_GENERAL,"%s: frame_cropping_flag = 0x%08x\n", __FUNCTION__, psTraceSeqParams->frame_cropping_flag);
    drv_debug_msg(VIDEO_DEBUG_GENERAL,"%s: frame_crop_left_offset = 0x%08x\n", __FUNCTION__, psTraceSeqParams->frame_crop_left_offset);
    drv_debug_msg(VIDEO_DEBUG_GENERAL,"%s: frame_crop_right_offset = 0x%08x\n", __FUNCTION__, psTraceSeqParams->frame_crop_right_offset);
    drv_debug_msg(VIDEO_DEBUG_GENERAL,"%s: frame_crop_top_offset = 0x%08x\n", __FUNCTION__, psTraceSeqParams->frame_crop_top_offset);
    drv_debug_msg(VIDEO_DEBUG_GENERAL,"%s: frame_crop_bottom_offset = 0x%08x\n", __FUNCTION__, psTraceSeqParams->frame_crop_bottom_offset);
    return;
}


void tng_H264ES_trace_pic_params(VAEncPictureParameterBufferH264 *psTracePicParams)
{
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s: CurrPic.picture_id = 0x%08x\n",__FUNCTION__, psTracePicParams->CurrPic.picture_id);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s: coded_buf = 0x%08x\n",__FUNCTION__, psTracePicParams->coded_buf);

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s: ReferenceFrames[0] = 0x%08x\n",__FUNCTION__, psTracePicParams->ReferenceFrames[0].picture_id);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s: ReferenceFrames[1] = 0x%08x\n",__FUNCTION__, psTracePicParams->ReferenceFrames[1].picture_id);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s: ReferenceFrames[2] = 0x%08x\n",__FUNCTION__, psTracePicParams->ReferenceFrames[2].picture_id);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s: ReferenceFrames[3] = 0x%08x\n",__FUNCTION__, psTracePicParams->ReferenceFrames[3].picture_id);
                
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s: pic_fields = 0x%08x\n",__FUNCTION__, psTracePicParams->pic_fields.value);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s: pic_init_qp = 0x%08x\n",__FUNCTION__, psTracePicParams->pic_init_qp);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s: last_picture = 0x%08x\n",__FUNCTION__, psTracePicParams->last_picture);
    return;
}


void tng_H264ES_trace_slice_params(VAEncSliceParameterBufferH264 *psTraceSliceParams)
{
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s: disable_deblocking_filter_idc = 0x%08x\n",__FUNCTION__, psTraceSliceParams->disable_deblocking_filter_idc);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s: macroblock_address = 0x%08x\n",__FUNCTION__, psTraceSliceParams->macroblock_address);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s: num_macroblocks = 0x%08x\n",__FUNCTION__, psTraceSliceParams->num_macroblocks);
    return;
}

void tng_H264ES_trace_misc_rc_params(VAEncMiscParameterRateControl *psTraceMiscRcParams)
{
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s bits_per_second = 0x%08x\n", __FUNCTION__, psTraceMiscRcParams->bits_per_second);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s window_size = 0x%08x\n", __FUNCTION__, psTraceMiscRcParams->window_size);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s initial_qp = 0x%08x\n", __FUNCTION__, psTraceMiscRcParams->initial_qp);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s min_qp = 0x%08x\n", __FUNCTION__, psTraceMiscRcParams->min_qp);
    return;
}

/*********************************************************************/
void tng_trace_seq_header_params(H264_SEQUENCE_HEADER_PARAMS *psSHParams)
{
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s ucProfile                         = %x\n", __FUNCTION__, psSHParams->ucProfile);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s ucLevel                           = %x\n", __FUNCTION__, psSHParams->ucLevel);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s ucWidth_in_mbs_minus1             = %x\n", __FUNCTION__, psSHParams->ucWidth_in_mbs_minus1);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s ucHeight_in_maps_units_minus1     = %x\n", __FUNCTION__, psSHParams->ucHeight_in_maps_units_minus1);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s log2_max_pic_order_cnt            = %x\n", __FUNCTION__, psSHParams->log2_max_pic_order_cnt);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s max_num_ref_frames                = %x\n", __FUNCTION__, psSHParams->max_num_ref_frames);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s gaps_in_frame_num_value           = %x\n", __FUNCTION__, psSHParams->gaps_in_frame_num_value);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s ucFrame_mbs_only_flag             = %x\n", __FUNCTION__, psSHParams->ucFrame_mbs_only_flag);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s VUI_Params_Present                = %x\n", __FUNCTION__, psSHParams->VUI_Params_Present);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s seq_scaling_matrix_present_flag   = %x\n", __FUNCTION__, psSHParams->seq_scaling_matrix_present_flag);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s bUseDefaultScalingList            = %x\n", __FUNCTION__, psSHParams->bUseDefaultScalingList);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s bIsLossless                       = %x\n", __FUNCTION__, psSHParams->bIsLossless);
  if (psSHParams->VUI_Params_Present) {
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s Time_Scale            = %x\n", __FUNCTION__, psSHParams->VUI_Params.Time_Scale);            //!< Time scale as defined in the H.264 specification
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s bit_rate_value_minus1 = %x\n", __FUNCTION__, psSHParams->VUI_Params.bit_rate_value_minus1); //!< An inter framebitrate/64)-1
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s cbp_size_value_minus1 = %x\n", __FUNCTION__, psSHParams->VUI_Params.cbp_size_value_minus1); //!< An inter frame(bitrate*1.5)/16
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s CBR                   = %x\n", __FUNCTION__, psSHParams->VUI_Params.CBR);                   //!< CBR as defined in the H.264 specification
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s initial_cpb_removal   = %x\n", __FUNCTION__, psSHParams->VUI_Params.initial_cpb_removal_delay_length_minus1); //!< as defined in the H.264 specification
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s cpb_removal_delay_length_minus1 = %x\n", __FUNCTION__, psSHParams->VUI_Params.cpb_removal_delay_length_minus1); //!< as defined in the H.264 specification
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s dpb_output_delay_length_minus1  = %x\n", __FUNCTION__, psSHParams->VUI_Params.dpb_output_delay_length_minus1);  //!< as defined in the H.264 specification
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s time_offset_length              = %x\n", __FUNCTION__, psSHParams->VUI_Params.time_offset_length);              //!< as defined in the H.264 specification
    }
}

void tng_trace_pic_header_params(H264_PICTURE_HEADER_PARAMS *psSHParams)
{
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s pic_parameter_set_id;           = %x\n", __FUNCTION__, psSHParams->pic_parameter_set_id);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s seq_parameter_set_id;           = %x\n", __FUNCTION__, psSHParams->seq_parameter_set_id);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s entropy_coding_mode_flag;       = %x\n", __FUNCTION__, psSHParams->entropy_coding_mode_flag);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s weighted_pred_flag;             = %x\n", __FUNCTION__, psSHParams->weighted_pred_flag);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s weighted_bipred_idc;            = %x\n", __FUNCTION__, psSHParams->weighted_bipred_idc);           
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s chroma_qp_index_offset;         = %x\n", __FUNCTION__, psSHParams->chroma_qp_index_offset);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s constrained_intra_pred_flag;    = %x\n", __FUNCTION__, psSHParams->constrained_intra_pred_flag);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s transform_8x8_mode_flag;        = %x\n", __FUNCTION__, psSHParams->transform_8x8_mode_flag);  
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s pic_scaling_matrix_present_flag = %x\n", __FUNCTION__, psSHParams->pic_scaling_matrix_present_flag);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s bUseDefaultScalingList;         = %x\n", __FUNCTION__, psSHParams->bUseDefaultScalingList);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s second_chroma_qp_index_offset;  = %x\n", __FUNCTION__, psSHParams->second_chroma_qp_index_offset);

}

void tng_trace_slice_header_params(H264_SLICE_HEADER_PARAMS *psSlHParams)
{
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s: start addr = 0x%08x\n", __FUNCTION__, psSlHParams);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "SlHParams.ui8Start_Code_Prefix_Size_Bytes 0x%08x\n", psSlHParams->ui8Start_Code_Prefix_Size_Bytes);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "SlHParams.SliceFrame_Type                 0x%08x\n", psSlHParams->SliceFrame_Type);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "SlHParams.First_MB_Address                0x%08x\n", psSlHParams->First_MB_Address);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "SlHParams.Frame_Num_DO                    0x%08x\n", psSlHParams->Frame_Num_DO);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "SlHParams.Idr_Pic_Id                      0x%08x\n", psSlHParams->Idr_Pic_Id);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "SlHParams.log2_max_pic_order_cnt          0x%08x\n", psSlHParams->log2_max_pic_order_cnt);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "SlHParams.Picture_Num_DO                  0x%08x\n", psSlHParams->Picture_Num_DO);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "SlHParams.Disable_Deblocking_Filter_Idc   0x%08x\n", psSlHParams->Disable_Deblocking_Filter_Idc);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "SlHParams.bPiCInterlace                   0x%08x\n", psSlHParams->bPiCInterlace);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "SlHParams.bFieldType                      0x%08x\n", psSlHParams->bFieldType);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "SlHParams.bReferencePicture               0x%08x\n", psSlHParams->bReferencePicture);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "SlHParams.iDebAlphaOffsetDiv2             0x%08x\n", psSlHParams->iDebAlphaOffsetDiv2);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "SlHParams.iDebBetaOffsetDiv2              0x%08x\n", psSlHParams->iDebBetaOffsetDiv2);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "SlHParams.direct_spatial_mv_pred_flag     0x%08x\n", psSlHParams->direct_spatial_mv_pred_flag);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "SlHParams.num_ref_idx_l0_active_minus1    0x%08x\n", psSlHParams->num_ref_idx_l0_active_minus1);

  drv_debug_msg(VIDEO_DEBUG_GENERAL, "SlHParams.diff_ref_pic_num[0] 0x%08x\n", psSlHParams->diff_ref_pic_num[0]);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "SlHParams.diff_ref_pic_num[1] 0x%08x\n", psSlHParams->diff_ref_pic_num[1]);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "SlHParams.weighted_pred_flag  0x%08x\n", psSlHParams->weighted_pred_flag);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "SlHParams.weighted_bipred_idc 0x%08x\n", psSlHParams->weighted_bipred_idc);

  drv_debug_msg(VIDEO_DEBUG_GENERAL, "SlHParams.luma_log2_weight_denom   0x%08x\n", psSlHParams->luma_log2_weight_denom);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "SlHParams.chroma_log2_weight_denom 0x%08x\n", psSlHParams->chroma_log2_weight_denom);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "SlHParams.luma_weight_l0_flag[0]  0x%08x\n",  psSlHParams->luma_weight_l0_flag[0]);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "SlHParams.luma_weight_l0_flag[1]  0x%08x\n",  psSlHParams->luma_weight_l0_flag[1]);

  drv_debug_msg(VIDEO_DEBUG_GENERAL, "SlHParams.SlHParams.luma_weight_l0[0] 0x%08x\n", psSlHParams->luma_weight_l0[0]);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "SlHParams.SlHParams.luma_weight_l0[1] 0x%08x\n", psSlHParams->luma_weight_l0[1]);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "SlHParams.luma_offset_l0[0]           0x%08x\n", psSlHParams->luma_offset_l0[0]);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "SlHParams.luma_offset_l0[1]           0x%08x\n", psSlHParams->luma_offset_l0[1]);

  drv_debug_msg(VIDEO_DEBUG_GENERAL, "SlHParams.chroma_weight_l0_flag[0]    0x%08x\n", psSlHParams->chroma_weight_l0_flag[0]);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "SlHParams.chroma_weight_l0_flag[1]    0x%08x\n", psSlHParams->chroma_weight_l0_flag[1]);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "SlHParams.chromaB_weight_l0[0]        0x%08x\n", psSlHParams->chromaB_weight_l0[0]);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "SlHParams.chromaB_offset_l0[0]        0x%08x\n", psSlHParams->chromaB_offset_l0[0]);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "SlHParams.chromaR_weight_l0[0]        0x%08x\n", psSlHParams->chromaR_weight_l0[0]);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "SlHParams.chromaR_offset_l0[0]        0x%08x\n", psSlHParams->chromaR_offset_l0[0]);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "SlHParams.chromaB_weight_l0[1]        0x%08x\n", psSlHParams->chromaB_weight_l0[1]);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "SlHParams.chromaB_offset_l0[1]        0x%08x\n", psSlHParams->chromaB_offset_l0[1]);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "SlHParams.chromaR_weight_l0[1]        0x%08x\n", psSlHParams->chromaR_weight_l0[1]);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "SlHParams.chromaR_offset_l0[1]        0x%08x\n", psSlHParams->chromaR_offset_l0[1]);
 
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "SlHParams.ui16MvcViewIdx    0x%08x\n", psSlHParams->ui16MvcViewIdx);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "SlHParams.bIsLongTermRef    0x%08x\n", psSlHParams->bIsLongTermRef);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "SlHParams.uLongTermRefNum   0x%08x\n", psSlHParams->uLongTermRefNum);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "SlHParams.bRefIsLongTermRef[0]  0x%08x\n", psSlHParams->bRefIsLongTermRef[0]);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "SlHParams.bRefIsLongTermRef[1]  0x%08x\n", psSlHParams->bRefIsLongTermRef[1]);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "SlHParams.uRefLongTermRefNum[0] 0x%08x\n", psSlHParams->uRefLongTermRefNum[0]);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "SlHParams.uRefLongTermRefNum[1] 0x%08x\n", psSlHParams->uRefLongTermRefNum[1]);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s: end \n", __FUNCTION__);
}

void tng__trace_seqconfig(
    IMG_BOOL bIsBPicture,
    IMG_BOOL bFieldMode,
    IMG_UINT8  ui8SwapChromas,
    IMG_BOOL32 ui32FrameStoreFormat,
    IMG_UINT8  uiDeblockIDC)
{
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s ui32SeqConfig 0x%08x\n", __FUNCTION__, F_ENCODE(0, TOPAZHP_CR_TEMPORAL_PIC0_BELOW_IN_VALID)                      );
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s ui32SeqConfig 0x%08x\n", __FUNCTION__, F_ENCODE(0, TOPAZHP_CR_TEMPORAL_PIC1_BELOW_IN_VALID)                    );
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s ui32SeqConfig 0x%08x\n", __FUNCTION__, F_ENCODE(0, TOPAZHP_CR_ABOVE_OUT_OF_SLICE_VALID)                        );
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s ui32SeqConfig 0x%08x\n", __FUNCTION__, F_ENCODE(1, TOPAZHP_CR_WRITE_TEMPORAL_PIC0_BELOW_VALID)                 );
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s ui32SeqConfig 0x%08x\n", __FUNCTION__, F_ENCODE(0, TOPAZHP_CR_REF_PIC0_VALID)                                  );
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s ui32SeqConfig 0x%08x\n", __FUNCTION__, F_ENCODE(0, TOPAZHP_CR_REF_PIC1_VALID)                                  );
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s ui32SeqConfig 0x%08x\n", __FUNCTION__, F_ENCODE(!bIsBPicture, TOPAZHP_CR_REF_PIC1_EQUAL_PIC0)                  );
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s ui32SeqConfig 0x%08x\n", __FUNCTION__, F_ENCODE(bFieldMode ? 1 : 0 , TOPAZHP_CR_FIELD_MODE)                    );
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s ui32SeqConfig 0x%08x\n", __FUNCTION__, F_ENCODE(ui8SwapChromas, TOPAZHP_CR_FRAME_STORE_CHROMA_SWAP)            );
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s ui32SeqConfig 0x%08x\n", __FUNCTION__, F_ENCODE(ui32FrameStoreFormat, TOPAZHP_CR_FRAME_STORE_FORMAT)           );
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s ui32SeqConfig 0x%08x\n", __FUNCTION__, F_ENCODE(TOPAZHP_CR_ENCODER_STANDARD_H264, TOPAZHP_CR_ENCODER_STANDARD) );
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s ui32SeqConfig 0x%08x\n", __FUNCTION__, F_ENCODE(uiDeblockIDC == 1 ? 0 : 1, TOPAZHP_CR_DEBLOCK_ENABLE));
}

void tng__trace_seq_header(void* pointer)
{
    context_ENC_p ctx = NULL;
    if (pointer == NULL)
        return ;

    ctx = (context_ENC_p)pointer;

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s: ucProfile = %d\n",              __FUNCTION__, ctx->ui8ProfileIdc);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s: ucLevel = %d\n",                __FUNCTION__, ctx->ui8LevelIdc);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s: ui16Width = %d\n",              __FUNCTION__, ctx->ui16Width);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s: ui16PictureHeight = %d\n",      __FUNCTION__, ctx->ui16PictureHeight);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s: ui32CustomQuantMask = %d\n",    __FUNCTION__, ctx->ui32CustomQuantMask);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s: ui8FieldCount = %d\n",          __FUNCTION__, ctx->ui8FieldCount);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s: ui8MaxNumRefFrames = %d\n",     __FUNCTION__, ctx->ui8MaxNumRefFrames);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s: bPpsScaling = %d\n",            __FUNCTION__, ctx->bPpsScaling);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s: bUseDefaultScalingList = %d\n", __FUNCTION__, ctx->bUseDefaultScalingList);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s: bEnableLossless = %d\n",        __FUNCTION__, ctx->bEnableLossless);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s: bArbitrarySO = %d\n",           __FUNCTION__, ctx->bArbitrarySO);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s: vui_flag = %d\n",               __FUNCTION__, ctx->sVuiParams.vui_flag);
}

