/*
* Copyright (c) 2009-2011 Intel Corporation.  All rights reserved.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include <string.h>
#include <stdlib.h>
#include "VideoEncoderLog.h"
#include "VideoEncoderH263.h"
#include <va/va_tpi.h>

VideoEncoderH263::VideoEncoderH263() {
    mComParams.profile = (VAProfile)PROFILE_H263BASELINE;
    mAutoReferenceSurfaceNum = 2;
}

Encode_Status VideoEncoderH263::sendEncodeCommand(EncodeTask *task) {

    Encode_Status ret = ENCODE_SUCCESS;
    LOG_V( "Begin\n");

    if (mFrameNum == 0) {
        ret = renderSequenceParams(task);
        CHECK_ENCODE_STATUS_RETURN("renderSequenceParams");
    }

    ret = renderPictureParams(task);
    CHECK_ENCODE_STATUS_RETURN("renderPictureParams");

    ret = renderSliceParams(task);
    CHECK_ENCODE_STATUS_RETURN("renderSliceParams");

    LOG_V( "End\n");
    return ENCODE_SUCCESS;
}


Encode_Status VideoEncoderH263::renderSequenceParams(EncodeTask *) {

    VAStatus vaStatus = VA_STATUS_SUCCESS;
    VAEncSequenceParameterBufferH263 h263SequenceParam = VAEncSequenceParameterBufferH263();
    uint32_t frameRateNum = mComParams.frameRate.frameRateNum;
    uint32_t frameRateDenom = mComParams.frameRate.frameRateDenom;

    LOG_V( "Begin\n\n");
    //set up the sequence params for HW
    h263SequenceParam.bits_per_second= mComParams.rcParams.bitRate;
    h263SequenceParam.frame_rate = 
            (unsigned int) (frameRateNum + frameRateDenom /2) / frameRateDenom;   //hard-coded, driver need;
    h263SequenceParam.initial_qp = mComParams.rcParams.initQP;
    h263SequenceParam.min_qp = mComParams.rcParams.minQP;
    h263SequenceParam.intra_period = mComParams.intraPeriod;

    //h263_seq_param.fixed_vop_rate = 30;

    LOG_V("===h263 sequence params===\n");
    LOG_I( "bitrate = %d\n", h263SequenceParam.bits_per_second);
    LOG_I( "frame_rate = %d\n", h263SequenceParam.frame_rate);
    LOG_I( "initial_qp = %d\n", h263SequenceParam.initial_qp);
    LOG_I( "min_qp = %d\n", h263SequenceParam.min_qp);
    LOG_I( "intra_period = %d\n\n", h263SequenceParam.intra_period);

    vaStatus = vaCreateBuffer(
            mVADisplay, mVAContext,
            VAEncSequenceParameterBufferType,
            sizeof(h263SequenceParam),
            1, &h263SequenceParam,
            &mSeqParamBuf);
    CHECK_VA_STATUS_RETURN("vaCreateBuffer");

    vaStatus = vaRenderPicture(mVADisplay, mVAContext, &mSeqParamBuf, 1);
    CHECK_VA_STATUS_RETURN("vaRenderPicture");

    LOG_V( "end\n");
    return ENCODE_SUCCESS;
}

Encode_Status VideoEncoderH263::renderPictureParams(EncodeTask *task) {

    VAStatus vaStatus = VA_STATUS_SUCCESS;
    VAEncPictureParameterBufferH263 h263PictureParams = VAEncPictureParameterBufferH263();

    LOG_V( "Begin\n\n");

    // set picture params for HW
    if(mAutoReference == false){
        h263PictureParams.reference_picture = task->ref_surface;
        h263PictureParams.reconstructed_picture = task->rec_surface;
    }else {
        h263PictureParams.reference_picture = mAutoRefSurfaces[0];
        h263PictureParams.reconstructed_picture = mAutoRefSurfaces[1];
    }

    h263PictureParams.coded_buf = task->coded_buffer;
    h263PictureParams.picture_width = mComParams.resolution.width;
    h263PictureParams.picture_height = mComParams.resolution.height;
    h263PictureParams.picture_type = (task->type == FTYPE_I) ? VAEncPictureTypeIntra : VAEncPictureTypePredictive;

    LOG_V("======h263 picture params======\n");
    LOG_V( "reference_picture = 0x%08x\n", h263PictureParams.reference_picture);
    LOG_V( "reconstructed_picture = 0x%08x\n", h263PictureParams.reconstructed_picture);
    LOG_V( "coded_buf = 0x%08x\n", h263PictureParams.coded_buf);
//    LOG_I( "coded_buf_index = %d\n", mCodedBufIndex);
    LOG_V( "picture_width = %d\n", h263PictureParams.picture_width);
    LOG_V( "picture_height = %d\n",h263PictureParams.picture_height);
    LOG_V( "picture_type = %d\n\n",h263PictureParams.picture_type);

    vaStatus = vaCreateBuffer(
            mVADisplay, mVAContext,
            VAEncPictureParameterBufferType,
            sizeof(h263PictureParams),
            1,&h263PictureParams,
            &mPicParamBuf);
    CHECK_VA_STATUS_RETURN("vaCreateBuffer");

    vaStatus = vaRenderPicture(mVADisplay, mVAContext, &mPicParamBuf , 1);
    CHECK_VA_STATUS_RETURN("vaRenderPicture");

    LOG_V( "end\n");
    return ENCODE_SUCCESS;
}

Encode_Status VideoEncoderH263::renderSliceParams(EncodeTask *task) {

    VAStatus vaStatus = VA_STATUS_SUCCESS;
    uint32_t sliceHeight;
    uint32_t sliceHeightInMB;

    LOG_V("Begin\n\n");

    sliceHeight = mComParams.resolution.height;
    sliceHeight += 15;
    sliceHeight &= (~15);
    sliceHeightInMB = sliceHeight / 16;

    vaStatus = vaCreateBuffer(
            mVADisplay, mVAContext,
            VAEncSliceParameterBufferType,
            sizeof(VAEncSliceParameterBuffer),
            1, NULL, &mSliceParamBuf);
    CHECK_VA_STATUS_RETURN("vaCreateBuffer");

    VAEncSliceParameterBuffer *sliceParams;
    vaStatus = vaMapBuffer(mVADisplay, mSliceParamBuf, (void **)&sliceParams);
    CHECK_VA_STATUS_RETURN("vaMapBuffer");

    // starting MB row number for this slice
    sliceParams->start_row_number = 0;
    // slice height measured in MB
    sliceParams->slice_height = sliceHeightInMB;
    sliceParams->slice_flags.bits.is_intra = (task->type == FTYPE_I)?1:0;
    sliceParams->slice_flags.bits.disable_deblocking_filter_idc = 0;

    LOG_V("======h263 slice params======\n");
    LOG_V("start_row_number = %d\n", (int) sliceParams->start_row_number);
    LOG_V("slice_height_in_mb = %d\n", (int) sliceParams->slice_height);
    LOG_V("slice.is_intra = %d\n", (int) sliceParams->slice_flags.bits.is_intra);

    vaStatus = vaUnmapBuffer(mVADisplay, mSliceParamBuf);
    CHECK_VA_STATUS_RETURN("vaUnmapBuffer");

    vaStatus = vaRenderPicture(mVADisplay, mVAContext, &mSliceParamBuf, 1);
    CHECK_VA_STATUS_RETURN("vaRenderPicture");

    LOG_V("end\n");
    return ENCODE_SUCCESS;
}
