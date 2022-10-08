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

#include "va_private.h"
#include "VideoDecoderAVCSecure.h"
#include "VideoDecoderTrace.h"
#include <string.h>

#define STARTCODE_PREFIX_LEN        3
#define NALU_TYPE_MASK              0x1F
#define MAX_NALU_HEADER_BUFFER      8192
static const uint8_t startcodePrefix[STARTCODE_PREFIX_LEN] = {0x00, 0x00, 0x01};

VideoDecoderAVCSecure::VideoDecoderAVCSecure(const char *mimeType)
    : VideoDecoderAVC(mimeType),
      mNaluHeaderBuffer(NULL),
      mSliceHeaderBuffer(NULL) {
    setParserType(VBP_H264SECURE);
}

VideoDecoderAVCSecure::~VideoDecoderAVCSecure() {
}

Decode_Status VideoDecoderAVCSecure::start(VideoConfigBuffer *buffer) {
    Decode_Status status = VideoDecoderAVC::start(buffer);
    if (status != DECODE_SUCCESS) {
        return status;
    }

    mNaluHeaderBuffer = new uint8_t [MAX_NALU_HEADER_BUFFER];

    if (mNaluHeaderBuffer == NULL) {
        ETRACE("Failed to allocate memory for mNaluHeaderBuffer");
        return DECODE_MEMORY_FAIL;
    }

    mSliceHeaderBuffer = new uint8_t [MAX_NALU_HEADER_BUFFER];
    if (mSliceHeaderBuffer == NULL) {
        ETRACE("Failed to allocate memory for mSliceHeaderBuffer");
        if (mNaluHeaderBuffer) {
            delete [] mNaluHeaderBuffer;
            mNaluHeaderBuffer = NULL;
        }
        return DECODE_MEMORY_FAIL;
    }

    return status;
}

void VideoDecoderAVCSecure::stop(void) {
    VideoDecoderAVC::stop();

    if (mNaluHeaderBuffer) {
        delete [] mNaluHeaderBuffer;
        mNaluHeaderBuffer = NULL;
    }

    if (mSliceHeaderBuffer) {
        delete [] mSliceHeaderBuffer;
        mSliceHeaderBuffer = NULL;
    }

}

Decode_Status VideoDecoderAVCSecure::decode(VideoDecodeBuffer *buffer) {
    Decode_Status status;
    int32_t sizeAccumulated = 0;
    int32_t sliceHeaderSize = 0;
    int32_t sizeLeft = 0;
    int32_t sliceIdx = 0;
    uint8_t naluType;
    frame_info_t* pFrameInfo;

    mFrameSize = 0;
    if (buffer->flag & IS_SECURE_DATA) {
        VTRACE("Decoding protected video ...");
        mIsEncryptData = 1;
    } else {
        VTRACE("Decoding clear video ...");
        mIsEncryptData = 0;
        return VideoDecoderAVC::decode(buffer);
    }

    if (buffer->size != sizeof(frame_info_t)) {
        ETRACE("Not enough data to read frame_info_t!");
        return DECODE_INVALID_DATA;
    }
    pFrameInfo = (frame_info_t*) buffer->data;

    mFrameSize = pFrameInfo->length;
    VTRACE("mFrameSize = %d", mFrameSize);

    memcpy(&mEncParam, pFrameInfo->pavp, sizeof(pavp_info_t));
    for (int32_t i = 0; i < pFrameInfo->num_nalus; i++) {
        naluType = pFrameInfo->nalus[i].type & NALU_TYPE_MASK;
        if (naluType >= h264_NAL_UNIT_TYPE_SLICE && naluType <= h264_NAL_UNIT_TYPE_IDR) {
            memcpy(mSliceHeaderBuffer + sliceHeaderSize,
                &sliceIdx,
                sizeof(int32_t));
            sliceHeaderSize += 4;

            memcpy(mSliceHeaderBuffer + sliceHeaderSize,
                &pFrameInfo->data,
                sizeof(uint8_t*));
            sliceHeaderSize += sizeof(uint8_t*);

            memcpy(mSliceHeaderBuffer + sliceHeaderSize,
                &pFrameInfo->nalus[i].offset,
                sizeof(uint32_t));
            sliceHeaderSize += sizeof(uint32_t);

            memcpy(mSliceHeaderBuffer + sliceHeaderSize,
                &pFrameInfo->nalus[i].length,
                sizeof(uint32_t));
            sliceHeaderSize += sizeof(uint32_t);

            memcpy(mSliceHeaderBuffer + sliceHeaderSize,
                pFrameInfo->nalus[i].slice_header,
                sizeof(slice_header_t));
            sliceHeaderSize += sizeof(slice_header_t);
            if (pFrameInfo->nalus[i].type & 0x60) {
                memcpy(mSliceHeaderBuffer+sliceHeaderSize, pFrameInfo->dec_ref_pic_marking, sizeof(dec_ref_pic_marking_t));
            } else {
                memset(mSliceHeaderBuffer+sliceHeaderSize, 0, sizeof(dec_ref_pic_marking_t));
            }
            sliceHeaderSize += sizeof(dec_ref_pic_marking_t);
            sliceIdx++;
        } else if (naluType >= h264_NAL_UNIT_TYPE_SEI && naluType <= h264_NAL_UNIT_TYPE_PPS) {
            memcpy(mNaluHeaderBuffer + sizeAccumulated,
                startcodePrefix,
                STARTCODE_PREFIX_LEN);
            sizeAccumulated += STARTCODE_PREFIX_LEN;
            memcpy(mNaluHeaderBuffer + sizeAccumulated,
                pFrameInfo->nalus[i].data,
                pFrameInfo->nalus[i].length);
            sizeAccumulated += pFrameInfo->nalus[i].length;
        } else {
            WTRACE("Failure: DECODE_FRAME_DROPPED");
            return DECODE_FRAME_DROPPED;
        }
    }

    vbp_data_h264 *data = NULL;
    int new_sequence_to_handle = 0;

    if (sizeAccumulated > 0) {
        status =  VideoDecoderBase::parseBuffer(
                mNaluHeaderBuffer,
                sizeAccumulated,
                false,
                (void**)&data);
        CHECK_STATUS("VideoDecoderBase::parseBuffer");

        // [FIX DRC zoom issue] if one buffer contains more than one nalu
        // for example SPS+PPS+IDR, new_sps/new_pps flags set in parseBuffer
        // will be flushed in the following updateBuffer.
        // So that handleNewSequence will not be handled in decodeFrame()
        if (data->new_sps || data->new_pps) {
            new_sequence_to_handle = 1;
        }
    }

    if (sliceHeaderSize > 0) {
        memset(mSliceHeaderBuffer + sliceHeaderSize, 0xFF, 4);
        sliceHeaderSize += 4;
        status =  VideoDecoderBase::updateBuffer(
                mSliceHeaderBuffer,
                sliceHeaderSize,
                (void**)&data);
        CHECK_STATUS("VideoDecoderBase::updateBuffer");

        // in case the flags were flushed but indeed new sequence needed to be handled.
        if ((1 == new_sequence_to_handle) &&
            ((data->new_sps == 0) || (data->new_pps == 0))) {
            data->new_sps = 1;
            data->new_pps = 1;
        }
    }

    if (data == NULL) {
        ETRACE("Invalid data returned by parser!");
        return DECODE_MEMORY_FAIL;
    }

    if (!mVAStarted) {
         if (data->has_sps && data->has_pps) {
            status = startVA(data);
            CHECK_STATUS("startVA");
        } else {
            WTRACE("Can't start VA as either SPS or PPS is still not available.");
            return DECODE_SUCCESS;
        }
    }
    status = decodeFrame(buffer, data);
    return status;
}

Decode_Status VideoDecoderAVCSecure::decodeSlice(vbp_data_h264 *data, uint32_t picIndex, uint32_t sliceIndex) {
    Decode_Status status;
    VAStatus vaStatus;
    uint32_t bufferIDCount = 0;
    // maximum 4 buffers to render a slice: picture parameter, IQMatrix, slice parameter, slice data
    VABufferID bufferIDs[5];

    vbp_picture_data_h264 *picData = &(data->pic_data[picIndex]);
    vbp_slice_data_h264 *sliceData = &(picData->slc_data[sliceIndex]);
    VAPictureParameterBufferH264 *picParam = picData->pic_parms;
    VASliceParameterBufferH264 *sliceParam = &(sliceData->slc_parms);
    VAEncryptionParameterBuffer encryptParam;

    if (sliceParam->first_mb_in_slice == 0 || mDecodingFrame == false) {
        // either condition indicates start of a new frame
        if (sliceParam->first_mb_in_slice != 0) {
            WTRACE("The first slice is lost.");
            // TODO: handle the first slice lost
        }
        if (mDecodingFrame) {
            // interlace content, complete decoding the first field
            vaStatus = vaEndPicture(mVADisplay, mVAContext);
            CHECK_VA_STATUS("vaEndPicture");

            // for interlace content, top field may be valid only after the second field is parsed
            mAcquiredBuffer->pictureOrder= picParam->CurrPic.TopFieldOrderCnt;
        }

        // Update  the reference frames and surface IDs for DPB and current frame
        status = updateDPB(picParam);
        CHECK_STATUS("updateDPB");

        vaStatus = vaBeginPicture(mVADisplay, mVAContext, mAcquiredBuffer->renderBuffer.surface);
        CHECK_VA_STATUS("vaBeginPicture");

        // start decoding a frame
        mDecodingFrame = true;

        vaStatus = vaCreateBuffer(
            mVADisplay,
            mVAContext,
            VAPictureParameterBufferType,
            sizeof(VAPictureParameterBufferH264),
            1,
            picParam,
            &bufferIDs[bufferIDCount]);
        CHECK_VA_STATUS("vaCreatePictureParameterBuffer");
        bufferIDCount++;

        vaStatus = vaCreateBuffer(
            mVADisplay,
            mVAContext,
            VAIQMatrixBufferType,
            sizeof(VAIQMatrixBufferH264),
            1,
            data->IQ_matrix_buf,
            &bufferIDs[bufferIDCount]);
        CHECK_VA_STATUS("vaCreateIQMatrixBuffer");
        bufferIDCount++;

        if (mIsEncryptData) {
            memset(&encryptParam, 0, sizeof(VAEncryptionParameterBuffer));
            encryptParam.pavpCounterMode = 4;
            encryptParam.pavpEncryptionType = 2;
            encryptParam.hostEncryptMode = 2;
            encryptParam.pavpHasBeenEnabled = 1;
            encryptParam.app_id = 0;
            memcpy(encryptParam.pavpAesCounter, mEncParam.iv, 16);

            vaStatus = vaCreateBuffer(
                mVADisplay,
                mVAContext,
                (VABufferType)VAEncryptionParameterBufferType,
                sizeof(VAEncryptionParameterBuffer),
                1,
                &encryptParam,
                &bufferIDs[bufferIDCount]);
            CHECK_VA_STATUS("vaCreateEncryptionParameterBuffer");
            bufferIDCount++;
        }

        vaStatus = vaCreateBuffer(
            mVADisplay,
            mVAContext,
            VASliceDataBufferType,
            mFrameSize, //size
            1,        //num_elements
            sliceData->buffer_addr + sliceData->slice_offset,
            &bufferIDs[bufferIDCount]);
        CHECK_VA_STATUS("vaCreateSliceDataBuffer");
        bufferIDCount++;

    }

    vaStatus = vaCreateBuffer(
        mVADisplay,
        mVAContext,
        VASliceParameterBufferType,
        sizeof(VASliceParameterBufferH264Base),
        1,
        sliceParam,
        &bufferIDs[bufferIDCount]);

    CHECK_VA_STATUS("vaCreateSliceParameterBuffer");
    bufferIDCount++;

    vaStatus = vaRenderPicture(
        mVADisplay,
        mVAContext,
        bufferIDs,
        bufferIDCount);
    CHECK_VA_STATUS("vaRenderPicture");

    return DECODE_SUCCESS;
}

Decode_Status VideoDecoderAVCSecure::getCodecSpecificConfigs(
    VAProfile profile, VAConfigID *config)
{
    VAStatus vaStatus;
    VAConfigAttrib attrib[2];

    if (config == NULL) {
        ETRACE("Invalid parameter!");
        return DECODE_FAIL;
    }

    attrib[0].type = VAConfigAttribRTFormat;
    attrib[0].value = VA_RT_FORMAT_YUV420;
    attrib[1].type = VAConfigAttribDecSliceMode;
    attrib[1].value = VA_DEC_SLICE_MODE_NORMAL;

    vaStatus = vaGetConfigAttributes(mVADisplay,profile,VAEntrypointVLD, &attrib[1], 1);

    if (attrib[1].value & VA_DEC_SLICE_MODE_BASE)
    {
        ITRACE("AVC short format used");
        attrib[1].value = VA_DEC_SLICE_MODE_BASE;
    } else if (attrib[1].value & VA_DEC_SLICE_MODE_NORMAL) {
        ITRACE("AVC long format ssed");
        attrib[1].value = VA_DEC_SLICE_MODE_NORMAL;
    } else {
        ETRACE("Unsupported Decode Slice Mode!");
        return DECODE_FAIL;
    }

    vaStatus = vaCreateConfig(
            mVADisplay,
            profile,
            VAEntrypointVLD,
            &attrib[0],
            2,
            config);
    CHECK_VA_STATUS("vaCreateConfig");

    return DECODE_SUCCESS;
}
