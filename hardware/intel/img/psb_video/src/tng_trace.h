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
#ifndef _TNG_TRACE_H_
#define _TNG_TRACE_H_

#include <va/va_enc_h264.h>
#include "tng_hostcode.h"
#include "psb_drv_video.h"

void tng_trace_setvideo(context_ENC_p ctx, IMG_UINT32 ui32StreamID);
void tng_H264ES_trace_seq_params(VAEncSequenceParameterBufferH264 *psTraceSeqParams);
void tng_H264ES_trace_pic_params(VAEncPictureParameterBufferH264 *psTracePicParams);
void tng_H264ES_trace_slice_params(VAEncSliceParameterBufferH264 *psTraceSliceParams);
void tng_H264ES_trace_misc_rc_params(VAEncMiscParameterRateControl *psTraceMiscRcParams);

void tng_trace_seq_header_params(H264_SEQUENCE_HEADER_PARAMS *psSHParams);
void tng_trace_pic_header_params(H264_PICTURE_HEADER_PARAMS *psSHParams);
void tng_trace_slice_header_params(H264_SLICE_HEADER_PARAMS *psSlHParams);
int apSliceParamsTemplates_dump(context_ENC_p ctx, IMG_UINT32 ui32StreamIndex, IMG_UINT32 ui32SliceBufIdx);
void tng__trace_seqconfig(
    IMG_BOOL bIsBPicture,
    IMG_BOOL bFieldMode,
    IMG_UINT8  ui8SwapChromas,
    IMG_BOOL32 ui32FrameStoreFormat,
    IMG_UINT8  uiDeblockIDC);
void tng__trace_seq_header(void* pointer);

#endif
