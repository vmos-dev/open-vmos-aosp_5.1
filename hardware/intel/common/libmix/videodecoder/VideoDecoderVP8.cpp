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

#include "VideoDecoderVP8.h"
#include "VideoDecoderTrace.h"
#include <string.h>

VideoDecoderVP8::VideoDecoderVP8(const char *mimeType)
    : VideoDecoderBase(mimeType, VBP_VP8) {
    invalidateReferenceFrames(0);
    invalidateReferenceFrames(1);
}

VideoDecoderVP8::~VideoDecoderVP8() {
    stop();
}

void VideoDecoderVP8::invalidateReferenceFrames(int toggle) {
    ReferenceFrameBuffer *p = mRFBs[toggle];
    for (int i = 0; i < VP8_REF_SIZE; i++) {
        p->index = (uint32_t) -1;
        p->surfaceBuffer = NULL;
        p++;
    }
}

void VideoDecoderVP8::clearAsReference(int toggle, int ref_type) {
    ReferenceFrameBuffer ref = mRFBs[toggle][ref_type];
    if (ref.surfaceBuffer) {
        ref.surfaceBuffer->asReferernce = false;
    }
}

void VideoDecoderVP8::updateFormatInfo(vbp_data_vp8 *data) {
    uint32_t width = data->codec_data->frame_width;
    uint32_t height = data->codec_data->frame_height;
    ITRACE("updateFormatInfo: current size: %d x %d, new size: %d x %d",
            mVideoFormatInfo.width, mVideoFormatInfo.height, width, height);

    if ((mVideoFormatInfo.width != width ||
            mVideoFormatInfo.height != height) &&
            width && height) {
        if ((VideoDecoderBase::alignMB(mVideoFormatInfo.width) != width) ||
            (VideoDecoderBase::alignMB(mVideoFormatInfo.height) != height)) {
            mSizeChanged = true;
            ITRACE("Video size is changed.");
        }
        mVideoFormatInfo.width = width;
        mVideoFormatInfo.height = height;
    }

    mVideoFormatInfo.cropLeft = data->codec_data->crop_left;
    mVideoFormatInfo.cropRight = data->codec_data->crop_right;
    mVideoFormatInfo.cropTop = data->codec_data->crop_top;
    mVideoFormatInfo.cropBottom = data->codec_data->crop_bottom;
    ITRACE("Cropping: left = %d, top = %d, right = %d, bottom = %d", data->codec_data->crop_left, data->codec_data->crop_top, data->codec_data->crop_right, data->codec_data->crop_bottom);

    mVideoFormatInfo.valid = true;

    setRenderRect();
}

Decode_Status VideoDecoderVP8::startVA(vbp_data_vp8 *data) {
    updateFormatInfo(data);

    VAProfile vaProfile = VAProfileVP8Version0_3;
    if (data->codec_data->version_num > 3) {
        return DECODE_PARSER_FAIL;
    }

    enableLowDelayMode(true);

    return VideoDecoderBase::setupVA(VP8_SURFACE_NUMBER + VP8_REF_SIZE, vaProfile);
}

Decode_Status VideoDecoderVP8::start(VideoConfigBuffer *buffer) {
    Decode_Status status;

    status = VideoDecoderBase::start(buffer);
    CHECK_STATUS("VideoDecoderBase::start");

    // We don't want base class to manage reference.
    VideoDecoderBase::ManageReference(false);

    if (buffer->data == NULL || buffer->size == 0) {
        WTRACE("No config data to start VA.");
        return DECODE_SUCCESS;
    }

    vbp_data_vp8 *data = NULL;
    status = VideoDecoderBase::parseBuffer(buffer->data, buffer->size, true, (void**)&data);
    CHECK_STATUS("VideoDecoderBase::parseBuffer");

    status = startVA(data);
    return status;
}

void VideoDecoderVP8::stop(void) {
    VideoDecoderBase::stop();

    invalidateReferenceFrames(0);
    invalidateReferenceFrames(1);
}

void VideoDecoderVP8::flush(void) {
    VideoDecoderBase::flush();

    invalidateReferenceFrames(0);
    invalidateReferenceFrames(1);
}

Decode_Status VideoDecoderVP8::decode(VideoDecodeBuffer *buffer) {
    Decode_Status status;
    vbp_data_vp8 *data = NULL;
    if (buffer == NULL) {
        ETRACE("VideoDecodeBuffer is NULL.");
        return DECODE_INVALID_DATA;
    }

    status = VideoDecoderBase::parseBuffer(
                 buffer->data,
                 buffer->size,
                 false,
                 (void**)&data);
    CHECK_STATUS("VideoDecoderBase::parseBuffer");

    mShowFrame = data->codec_data->show_frame;

    if (!mVAStarted) {
        status = startVA(data);
        CHECK_STATUS("startVA");
    }

    VideoDecoderBase::setRotationDegrees(buffer->rotationDegrees);

    status = decodeFrame(buffer, data);

    return status;
}

Decode_Status VideoDecoderVP8::decodeFrame(VideoDecodeBuffer* buffer, vbp_data_vp8 *data) {
    Decode_Status status;
    bool useGraphicbuffer = mConfigBuffer.flag & USE_NATIVE_GRAPHIC_BUFFER;
    mCurrentPTS = buffer->timeStamp;
    if (0 == data->num_pictures || NULL == data->pic_data) {
        WTRACE("Number of pictures is 0.");
        return DECODE_SUCCESS;
    }

    if (VP8_KEY_FRAME == data->codec_data->frame_type) {
        if (mSizeChanged && !useGraphicbuffer){
            mSizeChanged = false;
            return DECODE_FORMAT_CHANGE;
        } else {
            updateFormatInfo(data);
            bool noNeedFlush = false;
            if (useGraphicbuffer) {
                noNeedFlush = (mVideoFormatInfo.width <= mVideoFormatInfo.surfaceWidth)
                        && (mVideoFormatInfo.height <= mVideoFormatInfo.surfaceHeight);
            }
            if (mSizeChanged == true && !noNeedFlush) {
                flushSurfaceBuffers();
                mSizeChanged = false;
                return DECODE_FORMAT_CHANGE;
            }
        }
    }

    if (data->codec_data->frame_type == VP8_SKIPPED_FRAME) {
        // Do nothing for skip frame as the last frame will be rendered agian by natively
        return DECODE_SUCCESS;
    }

    status = acquireSurfaceBuffer();
    CHECK_STATUS("acquireSurfaceBuffer");

    // set referenceFrame to true if frame decoded is I/P frame, false otherwise.
    int frameType = data->codec_data->frame_type;
    mAcquiredBuffer->referenceFrame = (frameType == VP8_KEY_FRAME || frameType == VP8_INTER_FRAME);
    // assume it is frame picture.
    mAcquiredBuffer->renderBuffer.scanFormat = VA_FRAME_PICTURE;
    mAcquiredBuffer->renderBuffer.timeStamp = buffer->timeStamp;
    mAcquiredBuffer->renderBuffer.flag = 0;
    if (buffer->flag & WANT_DECODE_ONLY) {
        mAcquiredBuffer->renderBuffer.flag |= WANT_DECODE_ONLY;
    }
    if (mSizeChanged) {
        mSizeChanged = false;
        mAcquiredBuffer->renderBuffer.flag |= IS_RESOLUTION_CHANGE;
    }

    // Here data->num_pictures is always equal to 1
    for (uint32_t index = 0; index < data->num_pictures; index++) {
        status = decodePicture(data, index);
        if (status != DECODE_SUCCESS) {
            endDecodingFrame(true);
            return status;
        }
    }

    if (frameType != VP8_SKIPPED_FRAME) {
        updateReferenceFrames(data);
    }

    // if sample is successfully decoded, call outputSurfaceBuffer(); otherwise
    // call releaseSurfacebuffer();
    status = outputSurfaceBuffer();
    return status;
}

Decode_Status VideoDecoderVP8::decodePicture(vbp_data_vp8 *data, int32_t picIndex) {
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    Decode_Status status;
    uint32_t bufferIDCount = 0;
    VABufferID bufferIDs[5];

    vbp_picture_data_vp8 *picData = &(data->pic_data[picIndex]);
    VAPictureParameterBufferVP8 *picParams = picData->pic_parms;

    status = setReference(picParams);
    CHECK_STATUS("setReference");

    vaStatus = vaBeginPicture(mVADisplay, mVAContext, mAcquiredBuffer->renderBuffer.surface);
    CHECK_VA_STATUS("vaBeginPicture");
    // setting mDecodingFrame to true so vaEndPicture will be invoked to end the picture decoding.
    mDecodingFrame = true;

    vaStatus = vaCreateBuffer(
                   mVADisplay,
                   mVAContext,
                   VAPictureParameterBufferType,
                   sizeof(VAPictureParameterBufferVP8),
                   1,
                   picParams,
                   &bufferIDs[bufferIDCount]);
    CHECK_VA_STATUS("vaCreatePictureParameterBuffer");
    bufferIDCount++;

    vaStatus = vaCreateBuffer(
                   mVADisplay,
                   mVAContext,
                   VAProbabilityBufferType,
                   sizeof(VAProbabilityDataBufferVP8),
                   1,
                   data->prob_data,
                   &bufferIDs[bufferIDCount]);
    CHECK_VA_STATUS("vaCreateProbabilityBuffer");
    bufferIDCount++;

    vaStatus = vaCreateBuffer(
                   mVADisplay,
                   mVAContext,
                   VAIQMatrixBufferType,
                   sizeof(VAIQMatrixBufferVP8),
                   1,
                   data->IQ_matrix_buf,
                   &bufferIDs[bufferIDCount]);
    CHECK_VA_STATUS("vaCreateIQMatrixBuffer");
    bufferIDCount++;

    /* Here picData->num_slices is always equal to 1 */
    for (uint32_t i = 0; i < picData->num_slices; i++) {
        vaStatus = vaCreateBuffer(
                       mVADisplay,
                       mVAContext,
                       VASliceParameterBufferType,
                       sizeof(VASliceParameterBufferVP8),
                       1,
                       &(picData->slc_data[i].slc_parms),
                       &bufferIDs[bufferIDCount]);
        CHECK_VA_STATUS("vaCreateSliceParameterBuffer");
        bufferIDCount++;

        vaStatus = vaCreateBuffer(
                       mVADisplay,
                       mVAContext,
                       VASliceDataBufferType,
                       picData->slc_data[i].slice_size, //size
                       1,        //num_elements
                       picData->slc_data[i].buffer_addr + picData->slc_data[i].slice_offset,
                       &bufferIDs[bufferIDCount]);
        CHECK_VA_STATUS("vaCreateSliceDataBuffer");
        bufferIDCount++;
    }

    vaStatus = vaRenderPicture(
                   mVADisplay,
                   mVAContext,
                   bufferIDs,
                   bufferIDCount);
    CHECK_VA_STATUS("vaRenderPicture");

    vaStatus = vaEndPicture(mVADisplay, mVAContext);
    mDecodingFrame = false;
    CHECK_VA_STATUS("vaEndPicture");

    return DECODE_SUCCESS;
}

Decode_Status VideoDecoderVP8::setReference(VAPictureParameterBufferVP8 *picParam) {
    int frameType = picParam->pic_fields.bits.key_frame;
    switch (frameType) {
    case VP8_KEY_FRAME:
        picParam->last_ref_frame = VA_INVALID_SURFACE;
        picParam->alt_ref_frame = VA_INVALID_SURFACE;
        picParam->golden_ref_frame = VA_INVALID_SURFACE;
        break;
    case VP8_INTER_FRAME:
        if (mRFBs[0][VP8_LAST_REF_PIC].surfaceBuffer   == NULL ||
                mRFBs[0][VP8_ALT_REF_PIC].surfaceBuffer    == NULL ||
                mRFBs[0][VP8_GOLDEN_REF_PIC].surfaceBuffer == NULL) {
            mAcquiredBuffer->renderBuffer.errBuf.errorNumber = 1;
            mAcquiredBuffer->renderBuffer.errBuf.errorArray[0].type = DecodeRefMissing;
            return DECODE_NO_REFERENCE;
        }
        //mRFBs[0][VP8_LAST_REF_PIC].surfaceBuffer = mLastReference;
        picParam->last_ref_frame = mRFBs[0][VP8_LAST_REF_PIC].surfaceBuffer->renderBuffer.surface;
        picParam->alt_ref_frame = mRFBs[0][VP8_ALT_REF_PIC].surfaceBuffer->renderBuffer.surface;
        picParam->golden_ref_frame = mRFBs[0][VP8_GOLDEN_REF_PIC].surfaceBuffer->renderBuffer.surface;
        break;
    case VP8_SKIPPED_FRAME:
        // will never happen here
        break;
    default:
        return DECODE_PARSER_FAIL;
    }

    return DECODE_SUCCESS;
}

void VideoDecoderVP8::updateReferenceFrames(vbp_data_vp8 *data) {
    /* Refresh last frame reference buffer using the currently reconstructed frame */
    refreshLastReference(data);

    /* Refresh golden frame reference buffer using the currently reconstructed frame */
    refreshGoldenReference(data);

    /* Refresh alternative frame reference buffer using the currently reconstructed frame */
    refreshAltReference(data);

    /* Update reference frames */
    for (int i = 0; i < VP8_REF_SIZE; i++) {
        VideoSurfaceBuffer *p = mRFBs[1][i].surfaceBuffer;
        int j;
        for (j = 0; j < VP8_REF_SIZE; j++) {
            if (p == mRFBs[0][j].surfaceBuffer) {
                break;
            }
        }
        if (j == VP8_REF_SIZE) {
            clearAsReference(1, i);
        }
    }
}

void VideoDecoderVP8::refreshLastReference(vbp_data_vp8 *data) {
    /* Save previous last reference */
    mRFBs[1][VP8_LAST_REF_PIC].surfaceBuffer = mRFBs[0][VP8_LAST_REF_PIC].surfaceBuffer;
    mRFBs[1][VP8_LAST_REF_PIC].index = mRFBs[0][VP8_LAST_REF_PIC].index;

    /* For key frame, this is always true */
    if (data->codec_data->refresh_last_frame) {
        mRFBs[0][VP8_LAST_REF_PIC].surfaceBuffer = mAcquiredBuffer;
        mRFBs[0][VP8_LAST_REF_PIC].index = mAcquiredBuffer->renderBuffer.surface;
        mRFBs[0][VP8_LAST_REF_PIC].surfaceBuffer->asReferernce = true;
    }
}

void VideoDecoderVP8::refreshGoldenReference(vbp_data_vp8 *data) {
    /* Save previous golden reference */
    mRFBs[1][VP8_GOLDEN_REF_PIC].surfaceBuffer = mRFBs[0][VP8_GOLDEN_REF_PIC].surfaceBuffer;
    mRFBs[1][VP8_GOLDEN_REF_PIC].index = mRFBs[0][VP8_GOLDEN_REF_PIC].index;

    if (data->codec_data->golden_copied != BufferCopied_NoneToGolden) {
        if (data->codec_data->golden_copied == BufferCopied_LastToGolden) {
            /* LastFrame is copied to GoldenFrame */
            mRFBs[0][VP8_GOLDEN_REF_PIC].surfaceBuffer = mRFBs[1][VP8_LAST_REF_PIC].surfaceBuffer;
            mRFBs[0][VP8_GOLDEN_REF_PIC].index = mRFBs[1][VP8_LAST_REF_PIC].index;
        } else if (data->codec_data->golden_copied == BufferCopied_AltRefToGolden) {
            /* AltRefFrame is copied to GoldenFrame */
            mRFBs[0][VP8_GOLDEN_REF_PIC].surfaceBuffer = mRFBs[0][VP8_ALT_REF_PIC].surfaceBuffer;
            mRFBs[0][VP8_GOLDEN_REF_PIC].index = mRFBs[0][VP8_ALT_REF_PIC].index;
        }
    }

    /* For key frame, this is always true */
    if (data->codec_data->refresh_golden_frame) {
        mRFBs[0][VP8_GOLDEN_REF_PIC].surfaceBuffer = mAcquiredBuffer;
        mRFBs[0][VP8_GOLDEN_REF_PIC].index = mAcquiredBuffer->renderBuffer.surface;
        mRFBs[0][VP8_GOLDEN_REF_PIC].surfaceBuffer->asReferernce = true;
    }
}

void VideoDecoderVP8::refreshAltReference(vbp_data_vp8 *data) {
    /* Save previous alternative reference */
    mRFBs[1][VP8_ALT_REF_PIC].surfaceBuffer = mRFBs[0][VP8_ALT_REF_PIC].surfaceBuffer;
    mRFBs[1][VP8_ALT_REF_PIC].index = mRFBs[0][VP8_ALT_REF_PIC].index;

    if (data->codec_data->altref_copied != BufferCopied_NoneToAltRef) {
        if (data->codec_data->altref_copied == BufferCopied_LastToAltRef) {
            /* LastFrame is copied to AltRefFrame */
            mRFBs[0][VP8_ALT_REF_PIC].surfaceBuffer = mRFBs[1][VP8_LAST_REF_PIC].surfaceBuffer;
            mRFBs[0][VP8_ALT_REF_PIC].index = mRFBs[1][VP8_LAST_REF_PIC].index;
        } else if (data->codec_data->altref_copied == BufferCopied_GoldenToAltRef) {
            /* GoldenFrame is copied to AltRefFrame */
            mRFBs[0][VP8_ALT_REF_PIC].surfaceBuffer = mRFBs[1][VP8_GOLDEN_REF_PIC].surfaceBuffer;
            mRFBs[0][VP8_ALT_REF_PIC].index = mRFBs[1][VP8_GOLDEN_REF_PIC].index;
        }
    }

    /* For key frame, this is always true */
    if (data->codec_data->refresh_alt_frame) {
        mRFBs[0][VP8_ALT_REF_PIC].surfaceBuffer = mAcquiredBuffer;
        mRFBs[0][VP8_ALT_REF_PIC].index = mAcquiredBuffer->renderBuffer.surface;
        mRFBs[0][VP8_ALT_REF_PIC].surfaceBuffer->asReferernce = true;
    }
}


Decode_Status VideoDecoderVP8::checkHardwareCapability() {
    VAStatus vaStatus;
    VAConfigAttrib cfgAttribs[2];
    cfgAttribs[0].type = VAConfigAttribMaxPictureWidth;
    cfgAttribs[1].type = VAConfigAttribMaxPictureHeight;
    vaStatus = vaGetConfigAttributes(mVADisplay, VAProfileVP8Version0_3,
            VAEntrypointVLD, cfgAttribs, 2);
    CHECK_VA_STATUS("vaGetConfigAttributes");
    if (cfgAttribs[0].value * cfgAttribs[1].value < (uint32_t)mVideoFormatInfo.width * (uint32_t)mVideoFormatInfo.height) {
        ETRACE("hardware supports resolution %d * %d smaller than the clip resolution %d * %d",
                cfgAttribs[0].value, cfgAttribs[1].value, mVideoFormatInfo.width, mVideoFormatInfo.height);
        return DECODE_DRIVER_FAIL;
    }

    return DECODE_SUCCESS;
}

