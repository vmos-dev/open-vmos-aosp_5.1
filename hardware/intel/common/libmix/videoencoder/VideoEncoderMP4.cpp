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
#include "VideoEncoderMP4.h"
#include <va/va_tpi.h>

VideoEncoderMP4::VideoEncoderMP4()
    :mProfileLevelIndication(3)
    ,mFixedVOPTimeIncrement(0) {
    mComParams.profile = (VAProfile)PROFILE_MPEG4SIMPLE;
    mAutoReferenceSurfaceNum = 2;
}

Encode_Status VideoEncoderMP4::getHeaderPos(
        uint8_t *inBuffer, uint32_t bufSize, uint32_t *headerSize) {

    uint32_t bytesLeft = bufSize;

    *headerSize = 0;
    CHECK_NULL_RETURN_IFFAIL(inBuffer);

    if (bufSize < 4) {
        //bufSize shoule not < 4
        LOG_E("Buffer size too small\n");
        return ENCODE_FAIL;
    }

    while (bytesLeft > 4  &&
            (memcmp("\x00\x00\x01\xB6", &inBuffer[bufSize - bytesLeft], 4) &&
             memcmp("\x00\x00\x01\xB3", &inBuffer[bufSize - bytesLeft], 4))) {
        --bytesLeft;
    }

    if (bytesLeft <= 4) {
        LOG_E("NO header found\n");
        *headerSize = 0; //
    } else {
        *headerSize = bufSize - bytesLeft;
    }

    return ENCODE_SUCCESS;
}

Encode_Status VideoEncoderMP4::outputConfigData(
        VideoEncOutputBuffer *outBuffer) {

    Encode_Status ret = ENCODE_SUCCESS;
    uint32_t headerSize = 0;

    ret = getHeaderPos((uint8_t *)mCurSegment->buf + mOffsetInSeg,
            mCurSegment->size - mOffsetInSeg, &headerSize);
    CHECK_ENCODE_STATUS_RETURN("getHeaderPos");
    if (headerSize == 0) {
        outBuffer->dataSize = 0;
        mCurSegment = NULL;
        return ENCODE_NO_REQUEST_DATA;
    }

    if (headerSize <= outBuffer->bufferSize) {
        memcpy(outBuffer->data, (uint8_t *)mCurSegment->buf + mOffsetInSeg, headerSize);
        mTotalSizeCopied += headerSize;
        mOffsetInSeg += headerSize;
        outBuffer->dataSize = headerSize;
        outBuffer->remainingSize = 0;
        outBuffer->flag |= ENCODE_BUFFERFLAG_ENDOFFRAME;
        outBuffer->flag |= ENCODE_BUFFERFLAG_CODECCONFIG;
        outBuffer->flag |= ENCODE_BUFFERFLAG_SYNCFRAME;
    } else {
        // we need a big enough buffer, otherwise we won't output anything
        outBuffer->dataSize = 0;
        outBuffer->remainingSize = headerSize;
        outBuffer->flag |= ENCODE_BUFFERFLAG_DATAINVALID;
        LOG_E("Buffer size too small\n");
        return ENCODE_BUFFER_TOO_SMALL;
    }

    return ret;
}

Encode_Status VideoEncoderMP4::getExtFormatOutput(VideoEncOutputBuffer *outBuffer) {

    Encode_Status ret = ENCODE_SUCCESS;

    LOG_V("Begin\n");
    CHECK_NULL_RETURN_IFFAIL(outBuffer);

    switch (outBuffer->format) {
        case OUTPUT_CODEC_DATA: {
            // Output the codec config data
            ret = outputConfigData(outBuffer);
            CHECK_ENCODE_STATUS_CLEANUP("outputCodecData");
            break;
        }
        default:
            LOG_E("Invalid buffer mode for MPEG-4:2\n");
            ret = ENCODE_FAIL;
            break;
    }

    LOG_I("out size is = %d\n", outBuffer->dataSize);


CLEAN_UP:

    LOG_V("End\n");
    return ret;
}

Encode_Status VideoEncoderMP4::renderSequenceParams(EncodeTask *) {

    VAStatus vaStatus = VA_STATUS_SUCCESS;
    VAEncSequenceParameterBufferMPEG4 mp4SequenceParams = VAEncSequenceParameterBufferMPEG4();

    uint32_t frameRateNum = mComParams.frameRate.frameRateNum;
    uint32_t frameRateDenom = mComParams.frameRate.frameRateDenom;

    LOG_V( "Begin\n\n");
    // set up the sequence params for HW
    mp4SequenceParams.profile_and_level_indication = mProfileLevelIndication;
    mp4SequenceParams.video_object_layer_width= mComParams.resolution.width;
    mp4SequenceParams.video_object_layer_height= mComParams.resolution.height;
    mp4SequenceParams.vop_time_increment_resolution =
            (unsigned int) (frameRateNum + frameRateDenom /2) / frameRateDenom;
    mp4SequenceParams.fixed_vop_time_increment= mFixedVOPTimeIncrement;
    mp4SequenceParams.bits_per_second= mComParams.rcParams.bitRate;
    mp4SequenceParams.frame_rate =
            (unsigned int) (frameRateNum + frameRateDenom /2) / frameRateDenom;
    mp4SequenceParams.initial_qp = mComParams.rcParams.initQP;
    mp4SequenceParams.min_qp = mComParams.rcParams.minQP;
    mp4SequenceParams.intra_period = mComParams.intraPeriod;
    //mpeg4_seq_param.fixed_vop_rate = 30;

    LOG_V("===mpeg4 sequence params===\n");
    LOG_I("profile_and_level_indication = %d\n", (uint32_t)mp4SequenceParams.profile_and_level_indication);
    LOG_I("intra_period = %d\n", mp4SequenceParams.intra_period);
    LOG_I("video_object_layer_width = %d\n", mp4SequenceParams.video_object_layer_width);
    LOG_I("video_object_layer_height = %d\n", mp4SequenceParams.video_object_layer_height);
    LOG_I("vop_time_increment_resolution = %d\n", mp4SequenceParams.vop_time_increment_resolution);
    LOG_I("fixed_vop_rate = %d\n", mp4SequenceParams.fixed_vop_rate);
    LOG_I("fixed_vop_time_increment = %d\n", mp4SequenceParams.fixed_vop_time_increment);
    LOG_I("bitrate = %d\n", mp4SequenceParams.bits_per_second);
    LOG_I("frame_rate = %d\n", mp4SequenceParams.frame_rate);
    LOG_I("initial_qp = %d\n", mp4SequenceParams.initial_qp);
    LOG_I("min_qp = %d\n", mp4SequenceParams.min_qp);
    LOG_I("intra_period = %d\n\n", mp4SequenceParams.intra_period);

    vaStatus = vaCreateBuffer(
            mVADisplay, mVAContext,
            VAEncSequenceParameterBufferType,
            sizeof(mp4SequenceParams),
            1, &mp4SequenceParams,
            &mSeqParamBuf);
    CHECK_VA_STATUS_RETURN("vaCreateBuffer");

    vaStatus = vaRenderPicture(mVADisplay, mVAContext, &mSeqParamBuf, 1);
    CHECK_VA_STATUS_RETURN("vaRenderPicture");

    LOG_V( "end\n");
    return ENCODE_SUCCESS;
}

Encode_Status VideoEncoderMP4::renderPictureParams(EncodeTask *task) {

    VAStatus vaStatus = VA_STATUS_SUCCESS;
    VAEncPictureParameterBufferMPEG4 mpeg4_pic_param = VAEncPictureParameterBufferMPEG4();
    LOG_V( "Begin\n\n");
    // set picture params for HW
    if(mAutoReference == false){
        mpeg4_pic_param.reference_picture = task->ref_surface;
        mpeg4_pic_param.reconstructed_picture = task->rec_surface;
    }else {
        mpeg4_pic_param.reference_picture = mAutoRefSurfaces[0];
        mpeg4_pic_param.reconstructed_picture = mAutoRefSurfaces[1];
    }

    mpeg4_pic_param.coded_buf = task->coded_buffer;
    mpeg4_pic_param.picture_width = mComParams.resolution.width;
    mpeg4_pic_param.picture_height = mComParams.resolution.height;
    mpeg4_pic_param.vop_time_increment= mFrameNum;
    mpeg4_pic_param.picture_type = (task->type == FTYPE_I) ? VAEncPictureTypeIntra : VAEncPictureTypePredictive;

    LOG_V("======mpeg4 picture params======\n");
    LOG_V("reference_picture = 0x%08x\n", mpeg4_pic_param.reference_picture);
    LOG_V("reconstructed_picture = 0x%08x\n", mpeg4_pic_param.reconstructed_picture);
    LOG_V("coded_buf = 0x%08x\n", mpeg4_pic_param.coded_buf);
//    LOG_I("coded_buf_index = %d\n", mCodedBufIndex);
    LOG_V("picture_width = %d\n", mpeg4_pic_param.picture_width);
    LOG_V("picture_height = %d\n", mpeg4_pic_param.picture_height);
    LOG_V("vop_time_increment = %d\n", mpeg4_pic_param.vop_time_increment);
    LOG_V("picture_type = %d\n\n", mpeg4_pic_param.picture_type);

    vaStatus = vaCreateBuffer(
            mVADisplay, mVAContext,
            VAEncPictureParameterBufferType,
            sizeof(mpeg4_pic_param),
            1,&mpeg4_pic_param,
            &mPicParamBuf);
    CHECK_VA_STATUS_RETURN("vaCreateBuffer");

    vaStatus = vaRenderPicture(mVADisplay, mVAContext, &mPicParamBuf, 1);
    CHECK_VA_STATUS_RETURN("vaRenderPicture");

    return ENCODE_SUCCESS;
}


Encode_Status VideoEncoderMP4::renderSliceParams(EncodeTask *task) {

    VAStatus vaStatus = VA_STATUS_SUCCESS;
    uint32_t sliceHeight;
    uint32_t sliceHeightInMB;

    VAEncSliceParameterBuffer sliceParams;

    LOG_V( "Begin\n\n");

    sliceHeight = mComParams.resolution.height;
    sliceHeight += 15;
    sliceHeight &= (~15);
    sliceHeightInMB = sliceHeight / 16;

    sliceParams.start_row_number = 0;
    sliceParams.slice_height = sliceHeightInMB;
    sliceParams.slice_flags.bits.is_intra = (task->type == FTYPE_I)?1:0;
    sliceParams.slice_flags.bits.disable_deblocking_filter_idc = 0;

    LOG_V("======mpeg4 slice params======\n");
    LOG_I( "start_row_number = %d\n", (int) sliceParams.start_row_number);
    LOG_I( "sliceHeightInMB = %d\n", (int) sliceParams.slice_height);
    LOG_I( "is_intra = %d\n", (int) sliceParams.slice_flags.bits.is_intra);

    vaStatus = vaCreateBuffer(
            mVADisplay, mVAContext,
            VAEncSliceParameterBufferType,
            sizeof(VAEncSliceParameterBuffer),
            1, &sliceParams,
            &mSliceParamBuf);
    CHECK_VA_STATUS_RETURN("vaCreateBuffer");

    vaStatus = vaRenderPicture(mVADisplay, mVAContext, &mSliceParamBuf, 1);
    CHECK_VA_STATUS_RETURN("vaRenderPicture");

    LOG_V( "end\n");
    return ENCODE_SUCCESS;
}

Encode_Status VideoEncoderMP4::sendEncodeCommand(EncodeTask *task) {
    Encode_Status ret = ENCODE_SUCCESS;
    LOG_V( "Begin\n");

    if (mFrameNum == 0) {
        ret = renderSequenceParams(task);
        CHECK_ENCODE_STATUS_RETURN("renderSequenceParams");
    }

    ret = renderPictureParams(task);
    CHECK_ENCODE_STATUS_RETURN("renderPictureParams");

    ret = renderSliceParams(task);
    CHECK_ENCODE_STATUS_RETURN("renderPictureParams");

    LOG_V( "End\n");
    return ENCODE_SUCCESS;
}
