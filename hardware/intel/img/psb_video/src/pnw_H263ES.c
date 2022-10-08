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
 *
 */

#include "psb_def.h"
#include "psb_surface.h"
#include "psb_cmdbuf.h"
#include "pnw_H263ES.h"
#include "pnw_hostcode.h"
#include "pnw_hostheader.h"
#include "psb_drv_debug.h"

#include <stdlib.h>
#include <stdint.h>
#include <string.h>


#define TOPAZ_H263_MAX_BITRATE 16000000

#define INIT_CONTEXT_H263ES    context_ENC_p ctx = (context_ENC_p) (obj_context->format_data)
#define SURFACE(id)    ((object_surface_p) object_heap_lookup( &ctx->obj_context->driver_data->surface_heap, id ))
#define BUFFER(id)  ((object_buffer_p) object_heap_lookup( &ctx->obj_context->driver_data->buffer_heap, id ))


static void pnw_H263ES_QueryConfigAttributes(
    VAProfile __maybe_unused profile,
    VAEntrypoint __maybe_unused entrypoint,
    VAConfigAttrib *attrib_list,
    int num_attribs)
{
    int i;
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "pnw_H263ES_QueryConfigAttributes\n");

    /* RateControl attributes */
    for (i = 0; i < num_attribs; i++) {
        switch (attrib_list[i].type) {
        case VAConfigAttribRTFormat:
            break;

        case VAConfigAttribRateControl:
            attrib_list[i].value = VA_RC_NONE | VA_RC_CBR | VA_RC_VBR;
            break;

        default:
            attrib_list[i].value = VA_ATTRIB_NOT_SUPPORTED;
            break;
        }
    }

}


static VAStatus pnw_H263ES_ValidateConfig(
    object_config_p obj_config)
{
    int i;
    /* Check all attributes */
    for (i = 0; i < obj_config->attrib_count; i++) {
        switch (obj_config->attrib_list[i].type) {
        case VAConfigAttribRTFormat:
            /* Ignore */
            break;
        case VAConfigAttribRateControl:
            break;
        default:
            return VA_STATUS_ERROR_ATTR_NOT_SUPPORTED;
        }
    }

    return VA_STATUS_SUCCESS;
}


static VAStatus pnw_H263ES_CreateContext(
    object_context_p obj_context,
    object_config_p obj_config)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    unsigned int eRCmode;
    context_ENC_p ctx;
    int i;

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "pnw_H263ES_CreateContext\n");

    vaStatus = pnw_CreateContext(obj_context, obj_config, 0);/* alloc context_ENC_s and BO */

    if (VA_STATUS_SUCCESS != vaStatus)
        return VA_STATUS_ERROR_ALLOCATION_FAILED;

    ctx = (context_ENC_p) obj_context->format_data;

    for (i = 0; i < obj_config->attrib_count; i++) {
        if (obj_config->attrib_list[i].type == VAConfigAttribRateControl)
            break;
    }

    if (i >= obj_config->attrib_count)
        eRCmode = VA_RC_NONE;
    else
        eRCmode = obj_config->attrib_list[i].value;


    if (eRCmode == VA_RC_VBR) {
        ctx->eCodec = IMG_CODEC_H263_VBR;
        ctx->sRCParams.RCEnable = IMG_TRUE;
    } else if (eRCmode == VA_RC_CBR) {
        ctx->eCodec = IMG_CODEC_H263_CBR;
        ctx->sRCParams.RCEnable = IMG_TRUE;
    } else if (eRCmode == VA_RC_NONE) {
        ctx->eCodec = IMG_CODEC_H263_NO_RC;
        ctx->sRCParams.RCEnable = IMG_FALSE;
    } else
        return VA_STATUS_ERROR_UNSUPPORTED_RT_FORMAT;
    ctx->eFormat = IMG_CODEC_PL12;

    ctx->Slices = ctx->Height / 16;
    ctx->ParallelCores = 1; /* H263 encode use only one core from DDKv186 */

    ctx->IPEControl = pnw__get_ipe_control(ctx->eCodec);

    return vaStatus;


}


static void pnw_H263ES_DestroyContext(
    object_context_p obj_context)
{

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "pnw_H263ES_DestroyContext\n");

    pnw_DestroyContext(obj_context);
}

static VAStatus pnw_H263ES_BeginPicture(
    object_context_p obj_context)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;

    INIT_CONTEXT_H263ES;

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "pnw_H263ES_BeginPicture\n");

    vaStatus = pnw_BeginPicture(ctx);

    return vaStatus;
}


static VAStatus pnw__H263ES_process_sequence_param(context_ENC_p ctx, object_buffer_p obj_buffer)
{
    /*
     * process rate control parameters
     */
    VAEncSequenceParameterBufferH263 *pSequenceParams;
    int i;
    unsigned int frame_size;

    ASSERT(obj_buffer->type == VAEncSequenceParameterBufferType);
    ASSERT(obj_buffer->num_elements == 1);
    ASSERT(obj_buffer->size == sizeof(VAEncSequenceParameterBufferH263));
    //initialize the frame_rate and qp
    ctx->sRCParams.InitialQp = 15;
    ctx->sRCParams.MinQP = 1;
    ctx->sRCParams.FrameRate = 30;

    if ((obj_buffer->num_elements != 1) ||
        (obj_buffer->size != sizeof(VAEncSequenceParameterBufferH263))) {
        return VA_STATUS_ERROR_UNKNOWN;
    }

    pSequenceParams = (VAEncSequenceParameterBufferH263 *) obj_buffer->buffer_data;
    obj_buffer->buffer_data = NULL;
    obj_buffer->size = 0;

    if (pSequenceParams->bits_per_second > TOPAZ_H263_MAX_BITRATE) {
        ctx->sRCParams.BitsPerSecond = TOPAZ_H263_MAX_BITRATE;
        drv_debug_msg(VIDEO_DEBUG_GENERAL, " bits_per_second(%d) exceeds \
		the maximum bitrate, set it with %d\n",
                                 pSequenceParams->bits_per_second,
                                 TOPAZ_H263_MAX_BITRATE);
    } else
        ctx->sRCParams.BitsPerSecond = pSequenceParams->bits_per_second;

    ctx->sRCParams.FrameRate = (pSequenceParams->frame_rate < 1) ? 1 :
        ((65535 < pSequenceParams->frame_rate) ? 65535 : pSequenceParams->frame_rate);
    ctx->sRCParams.InitialQp = pSequenceParams->initial_qp;
    ctx->sRCParams.MinQP = pSequenceParams->min_qp;
    ctx->sRCParams.BUSize = 0; /* default 0, and will be set in pnw__setup_busize */

    ctx->sRCParams.BufferSize = ctx->sRCParams.BitsPerSecond;
    frame_size = ctx->sRCParams.BitsPerSecond / ctx->sRCParams.FrameRate;
    /*
        IMG_UINT16 MBRows = 0;
        if(ctx->Height <= 400)
            MBRows = 1;
        else if (ctx->Height < 800)
            MBRows = 2;
    */
    ctx->sRCParams.Slices = ctx->Slices;
    ctx->sRCParams.QCPOffset = 0;/* FIXME */
    if (ctx->sRCParams.IntraFreq != pSequenceParams->intra_period
            && ctx->raw_frame_count != 0
            && ctx->sRCParams.IntraFreq != 0
            && ((ctx->obj_context->frame_count + 1) % ctx->sRCParams.IntraFreq) != 0
            && (!ctx->sRCParams.bDisableFrameSkipping)) {
        drv_debug_msg(VIDEO_DEBUG_ERROR,
                "Changing intra period value in the middle of a GOP is\n"
                "not allowed if frame skip isn't disabled.\n"
                "it can cause I frame been skipped\n");
        return VA_STATUS_ERROR_INVALID_PARAMETER;
    }
    else
        ctx->sRCParams.IntraFreq = pSequenceParams->intra_period;

    ctx->sRCParams.InitialLevel = (3 * ctx->sRCParams.BufferSize) >> 4;
    /* Aligned with target frame size */
    ctx->sRCParams.InitialLevel += (frame_size / 2);
    ctx->sRCParams.InitialLevel /= frame_size;
    ctx->sRCParams.InitialLevel *= frame_size;
    ctx->sRCParams.InitialDelay = ctx->sRCParams.BufferSize - ctx->sRCParams.InitialLevel;
    ctx->buffer_size = ctx->sRCParams.BufferSize;

    /*if (ctx->sRCParams.BitsPerSecond < 256000)
        ctx->sRCParams.BufferSize = (9 * ctx->sRCParams.BitsPerSecond) >> 1;
    else
        ctx->sRCParams.BufferSize = (5 * ctx->sRCParams.BitsPerSecond) >> 1;*/

    if (ctx->raw_frame_count == 0) { /* Add Register IO behind begin Picture */
        for (i = (ctx->ParallelCores - 1); i >= 0; i--)
            pnw_set_bias(ctx, i);
    }

    free(pSequenceParams);
    return VA_STATUS_SUCCESS;
}


/* Work done by the followinig funcs in testbench should be done here.
 * IMG_C_StartPicture()
 * IMG_C_InsertSync()
 * APP_H263_SendPictureHeader()
 * Frame skip must be taken into consideration
 * */
static VAStatus pnw__H263ES_process_picture_param(context_ENC_p ctx, object_buffer_p obj_buffer)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    VAEncPictureParameterBufferH263 *pBuffer;
    pnw_cmdbuf_p cmdbuf = ctx->obj_context->pnw_cmdbuf;
    H263_SOURCE_FORMAT_TYPE SourceFormatType;
    int i;

    ASSERT(obj_buffer->type == VAEncPictureParameterBufferType);

    if ((obj_buffer->num_elements != 1) ||
        (obj_buffer->size != sizeof(VAEncPictureParameterBufferH263))) {
        return VA_STATUS_ERROR_UNKNOWN;
    }

    /* Transfer ownership of VAEncPictureParameterBufferH263 data */
    pBuffer = (VAEncPictureParameterBufferH263 *) obj_buffer->buffer_data;
    obj_buffer->buffer_data = NULL;
    obj_buffer->size = 0;

    ctx->ref_surface = SURFACE(pBuffer->reference_picture);
    ctx->dest_surface = SURFACE(pBuffer->reconstructed_picture);
    ctx->coded_buf = BUFFER(pBuffer->coded_buf);

    ctx->RawWidth = pBuffer->picture_width;
    ctx->RawHeight = pBuffer->picture_height;

    /* Insert do_header command here */

    if ((ctx->Width == 128) && (ctx->Height == 96))
        SourceFormatType = _128x96_SubQCIF;
    else if ((ctx->Width == 176) && (ctx->Height == 144))
        SourceFormatType = _176x144_QCIF;
    else if ((ctx->Width == 352) && (ctx->Height == 288))
        SourceFormatType = _352x288_CIF;
    else if ((ctx->Width == 704) && (ctx->Height == 576))
        SourceFormatType = _704x576_4CIF;
    else if ((ctx->Width <= 720) && (ctx->Height <= 576))
        SourceFormatType = 7;
    else {
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "Unsupported resolution!\n");
        return VA_STATUS_ERROR_RESOLUTION_NOT_SUPPORTED;
    }

    memset(cmdbuf->header_mem_p + ctx->pic_header_ofs,
           0,
           HEADER_SIZE);
    pnw__H263_prepare_picture_header(cmdbuf->header_mem_p + ctx->pic_header_ofs,
                                     ctx->obj_context->frame_count,
                                     pBuffer->picture_type,
                                     SourceFormatType,
                                     ctx->sRCParams.FrameRate,
                                     ctx->RawWidth,
                                     ctx->RawHeight);

    pnw_cmdbuf_insert_command_package(ctx->obj_context,
                                      ctx->ParallelCores - 1, /* Send to the last core as this will complete first */
                                      MTX_CMDID_DO_HEADER,
                                      &cmdbuf->header_mem,
                                      ctx->pic_header_ofs);


    /* Prepare START_PICTURE params */
    /* Actually H263 only use 1 core from DDKv186 */
    for (i = (ctx->ParallelCores - 1); i >= 0; i--)
        vaStatus = pnw_RenderPictureParameter(ctx, i);

    free(pBuffer);
    return vaStatus;
}


static VAStatus pnw__H263ES_process_slice_param(context_ENC_p ctx, object_buffer_p obj_buffer)
{
    /* Prepare InParams for macros of current slice, insert slice header, insert do slice command */
    VAEncSliceParameterBuffer *pBuffer;
    pnw_cmdbuf_p cmdbuf = ctx->obj_context->pnw_cmdbuf;
    PIC_PARAMS *psPicParams = (PIC_PARAMS *)(cmdbuf->pic_params_p);
    unsigned int i;
    int slice_param_idx;

    ASSERT(obj_buffer->type == VAEncSliceParameterBufferType);

    /* Transfer ownership of VAEncPictureParameterBufferH263 data */
    pBuffer = (VAEncSliceParameterBuffer *) obj_buffer->buffer_data;
    obj_buffer->size = 0;

    /*In case the slice number changes*/
    if ((ctx->slice_param_cache != NULL) && (obj_buffer->num_elements != ctx->slice_param_num)) {
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "Slice number changes. Previous value is %d. Now it's %d\n",
                                 ctx->slice_param_num, obj_buffer->num_elements);
        free(ctx->slice_param_cache);
        ctx->slice_param_cache = NULL;
        ctx->slice_param_num = 0;
    }

    if (NULL == ctx->slice_param_cache) {
        ctx->slice_param_num = obj_buffer->num_elements;
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "Allocate %d VAEncSliceParameterBuffer cache buffers\n", 2 * ctx->slice_param_num);
        ctx->slice_param_cache = calloc(2 * ctx->slice_param_num, sizeof(VAEncSliceParameterBuffer));
        if (NULL == ctx->slice_param_cache) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "Run out of memory!\n");
            free(obj_buffer->buffer_data);
            return VA_STATUS_ERROR_ALLOCATION_FAILED;
        }
    }

    for (i = 0; i < obj_buffer->num_elements; i++) {
        /*Todo list:
         *1.Insert Do header command
         *2.setup InRowParams
         *3.setup Slice params
         *4.Insert Do slice command
         * */
        unsigned char deblock_idc;

        deblock_idc = pBuffer->slice_flags.bits.disable_deblocking_filter_idc;

        /* Insert Do Header command, relocation is needed */
        if (ctx->obj_context->slice_count) {  /*First slice of a frame need not slice header*/
            memset(cmdbuf->header_mem_p + ctx->slice_header_ofs
                   + ctx->obj_context->slice_count * HEADER_SIZE,
                   0,
                   HEADER_SIZE);

            pnw__H263_prepare_GOBslice_header(
                cmdbuf->header_mem_p + ctx->slice_header_ofs + ctx->obj_context->slice_count * HEADER_SIZE,
                ctx->obj_context->slice_count,
                ctx->obj_context->frame_count);

            ctx->LastSliceNum[ctx->SliceToCore] = ctx->obj_context->slice_count;

            pnw_cmdbuf_insert_command_package(ctx->obj_context,
                                              ctx->SliceToCore,
                                              MTX_CMDID_DO_HEADER,
                                              &cmdbuf->header_mem,
                                              ctx->slice_header_ofs + ctx->obj_context->slice_count * HEADER_SIZE);

            if (0 == ctx->SliceToCore) {
                ctx->SliceToCore = ctx->ParallelCores;
            }
            ctx->SliceToCore--;

        }

        if ((pBuffer->start_row_number == 0) && pBuffer->slice_flags.bits.is_intra) {
            pnw_reset_encoder_params(ctx); /* originally do in BeginPicture, but beginpicture has no intra info */
            ctx->BelowParamsBufIdx = (ctx->BelowParamsBufIdx + 1) & 0x1;
        }

        slice_param_idx = (pBuffer->slice_flags.bits.is_intra ? 0 : 1) * ctx->slice_param_num + i;
        if (VAEncSliceParameter_Equal(&ctx->slice_param_cache[slice_param_idx], pBuffer) == 0) {
            /* cache current param parameters */
            memcpy(&ctx->slice_param_cache[slice_param_idx],
                   pBuffer, sizeof(VAEncSliceParameterBuffer));

            /* Setup InParams value*/
            pnw_setup_slice_params(ctx,
                                   pBuffer->start_row_number * 16,
                                   pBuffer->slice_height * 16,
                                   pBuffer->slice_flags.bits.is_intra,
                                   ctx->obj_context->frame_count > 0,
                                   psPicParams->sInParams.SeInitQP);
        }

        /* Insert do slice command and setup related buffer value */
        pnw__send_encode_slice_params(ctx,
                                      pBuffer->slice_flags.bits.is_intra,
                                      pBuffer->start_row_number * 16,
                                      deblock_idc,
                                      ctx->obj_context->frame_count,
                                      pBuffer->slice_height * 16,
                                      ctx->obj_context->slice_count);

        drv_debug_msg(VIDEO_DEBUG_GENERAL, "Now frame_count/slice_count is %d/%d\n",
                                 ctx->obj_context->frame_count, ctx->obj_context->slice_count);

        ctx->obj_context->slice_count++;
        pBuffer++;        /*Move to the next buffer*/

        ASSERT(ctx->obj_context->slice_count < MAX_SLICES_PER_PICTURE);
    }

    free(obj_buffer->buffer_data);
    obj_buffer->buffer_data = NULL;
    return VA_STATUS_SUCCESS;
}



static VAStatus pnw_H263ES_RenderPicture(
    object_context_p obj_context,
    object_buffer_p *buffers,
    int num_buffers)
{
    int i;
    INIT_CONTEXT_H263ES;
    VAStatus vaStatus = VA_STATUS_SUCCESS;

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "pnw_H263ES_RenderPicture\n");

    for (i = 0; i < num_buffers; i++) {
        object_buffer_p obj_buffer = buffers[i];

        switch (obj_buffer->type) {
        case VAEncSequenceParameterBufferType:
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "pnw_H263_RenderPicture got VAEncSequenceParameterBufferType\n");
            vaStatus = pnw__H263ES_process_sequence_param(ctx, obj_buffer);
            DEBUG_FAILURE;
            break;

        case VAEncPictureParameterBufferType:
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "pnw_H263_RenderPicture got VAEncPictureParameterBuffer\n");
            vaStatus = pnw__H263ES_process_picture_param(ctx, obj_buffer);
            DEBUG_FAILURE;
            break;

        case VAEncSliceParameterBufferType:
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "pnw_H263_RenderPicture got VAEncSliceParameterBufferType\n");
            vaStatus = pnw__H263ES_process_slice_param(ctx, obj_buffer);
            DEBUG_FAILURE;
            break;

        default:
            vaStatus = VA_STATUS_ERROR_UNKNOWN;
            DEBUG_FAILURE;
        }
        if (vaStatus != VA_STATUS_SUCCESS) {
            break;
        }
    }


    return vaStatus;
}

static VAStatus pnw_H263ES_EndPicture(
    object_context_p obj_context)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    INIT_CONTEXT_H263ES;

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "pnw_H263ES_EndPicture\n");

    vaStatus = pnw_EndPicture(ctx);

    return vaStatus;
}


struct format_vtable_s pnw_H263ES_vtable = {
queryConfigAttributes:
    pnw_H263ES_QueryConfigAttributes,
validateConfig:
    pnw_H263ES_ValidateConfig,
createContext:
    pnw_H263ES_CreateContext,
destroyContext:
    pnw_H263ES_DestroyContext,
beginPicture:
    pnw_H263ES_BeginPicture,
renderPicture:
    pnw_H263ES_RenderPicture,
endPicture:
    pnw_H263ES_EndPicture
};
