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

#include "VideoDecoderWMV.h"
#include "VideoDecoderTrace.h"
#include <string.h>

VideoDecoderWMV::VideoDecoderWMV(const char *mimeType)
    : VideoDecoderBase(mimeType, VBP_VC1),
      mBufferIDs(NULL),
      mNumBufferIDs(0),
      mConfigDataParsed(false),
      mRangeMapped(false),
      mDeblockedCurrPicIndex(0),
      mDeblockedLastPicIndex(1),
      mDeblockedForwardPicIndex(2) {
}


VideoDecoderWMV::~VideoDecoderWMV() {
    stop();
}

Decode_Status VideoDecoderWMV::start(VideoConfigBuffer *buffer) {
    Decode_Status status;

    status = VideoDecoderBase::start(buffer);
    CHECK_STATUS("VideoDecoderBase::start");

    if (buffer->data == NULL || buffer->size == 0) {
        WTRACE("No config data to start VA.");
        return DECODE_SUCCESS;
    }

    vbp_data_vc1 *data = NULL;
    status = parseBuffer(buffer->data, buffer->size, &data);
    CHECK_STATUS("parseBuffer");

    status = startVA(data);
    return status;
}

void VideoDecoderWMV::stop(void) {
    if (mBufferIDs) {
        delete [] mBufferIDs;
        mBufferIDs = NULL;
    }
    mNumBufferIDs = 0;
    mConfigDataParsed = false;
    mRangeMapped = false;

    mDeblockedCurrPicIndex = 0;
    mDeblockedLastPicIndex = 1;
    mDeblockedForwardPicIndex = 2;

    VideoDecoderBase::stop();
}

void VideoDecoderWMV::flush(void) {
    VideoDecoderBase::flush();

    mRangeMapped = false;
    mDeblockedCurrPicIndex = 0;
    mDeblockedLastPicIndex = 1;
    mDeblockedForwardPicIndex = 2;
}

Decode_Status VideoDecoderWMV::decode(VideoDecodeBuffer *buffer) {
    Decode_Status status;
    vbp_data_vc1 *data = NULL;
    bool useGraphicbuffer = mConfigBuffer.flag & USE_NATIVE_GRAPHIC_BUFFER;
    if (buffer == NULL) {
        return DECODE_INVALID_DATA;
    }

    status = parseBuffer(buffer->data, buffer->size, &data);
    CHECK_STATUS("parseBuffer");

    if (!mVAStarted) {
        status = startVA(data);
        CHECK_STATUS("startVA");
    }

    if (mSizeChanged && !useGraphicbuffer) {
        mSizeChanged = false;
        return DECODE_FORMAT_CHANGE;
    }

    if ((mVideoFormatInfo.width != data->se_data->CODED_WIDTH ||
        mVideoFormatInfo.height != data->se_data->CODED_HEIGHT) &&
        data->se_data->CODED_WIDTH &&
        data->se_data->CODED_HEIGHT) {
        ITRACE("video size is changed from %dx%d to %dx%d", mVideoFormatInfo.width, mVideoFormatInfo.height,
                data->se_data->CODED_WIDTH, data->se_data->CODED_HEIGHT);
        mVideoFormatInfo.width = data->se_data->CODED_WIDTH;
        mVideoFormatInfo.height = data->se_data->CODED_HEIGHT;
        bool noNeedFlush = false;
        if (useGraphicbuffer) {
            noNeedFlush = (mVideoFormatInfo.width <= mVideoFormatInfo.surfaceWidth)
                    && (mVideoFormatInfo.height <= mVideoFormatInfo.surfaceHeight);
        }

        setRenderRect();

        if (noNeedFlush) {
            mSizeChanged = true;
        } else {
            flushSurfaceBuffers();
            mSizeChanged = false;
            return DECODE_FORMAT_CHANGE;
        }
    }

    status = decodeFrame(buffer, data);
    CHECK_STATUS("decodeFrame");
    return status;
}

Decode_Status VideoDecoderWMV::decodeFrame(VideoDecodeBuffer* buffer, vbp_data_vc1 *data) {
    Decode_Status status;
    mCurrentPTS = buffer->timeStamp;
    if (0 == data->num_pictures || NULL == data->pic_data) {
        WTRACE("Number of pictures is 0, buffer contains configuration data only?");
        return DECODE_SUCCESS;
    }

    if (data->pic_data[0].picture_is_skipped == VC1_PTYPE_SKIPPED) {

        // Do nothing for skip frame as the last frame will be rendered agian by natively
        // No needs to handle reference frame neither
        return DECODE_SUCCESS;
#if 0
        //use the last P or I frame surface for skipped frame and treat it as P frame
        if (mLastReference == NULL) {
            // TODO: handle this case
            WTRACE("The last reference is unavailable to construct skipped frame.");
            return DECODE_SUCCESS;
        }

        status = acquireSurfaceBuffer();
        CHECK_STATUS("acquireSurfaceBuffer");
        mAcquiredBuffer->renderBuffer.timeStamp = mCurrentPTS;
        mAcquiredBuffer->renderBuffer.flag = 0;
        mAcquiredBuffer->renderBuffer.scanFormat = mLastReference->renderBuffer.scanFormat;
        mAcquiredBuffer->renderBuffer.surface = mLastReference->renderBuffer.surface;
        // No need to update mappedData for HW decoding
        //mAcquiredBuffer->mappedData.data = mLastReference->mappedData.data;
        mAcquiredBuffer->referenceFrame = true;
        // let outputSurfaceBuffer handle "asReference" for VC1
        status = outputSurfaceBuffer();
        return status;
#endif
    }

    status = acquireSurfaceBuffer();
    CHECK_STATUS("acquireSurfaceBuffer");

    mAcquiredBuffer->renderBuffer.timeStamp = buffer->timeStamp;
    if (buffer->flag & HAS_DISCONTINUITY) {
        mAcquiredBuffer->renderBuffer.flag |= HAS_DISCONTINUITY;
    }
    if (buffer->flag & WANT_DECODE_ONLY) {
        mAcquiredBuffer->renderBuffer.flag |= WANT_DECODE_ONLY;
    }
    if (mSizeChanged) {
        mSizeChanged = false;
        mAcquiredBuffer->renderBuffer.flag |= IS_RESOLUTION_CHANGE;
    }

    if (data->num_pictures > 1) {
        if (data->pic_data[0].pic_parms->picture_fields.bits.is_first_field) {
            mAcquiredBuffer->renderBuffer.scanFormat = VA_TOP_FIELD;
        } else {
            mAcquiredBuffer->renderBuffer.scanFormat = VA_BOTTOM_FIELD;
        }
    } else {
        mAcquiredBuffer->renderBuffer.scanFormat = VA_FRAME_PICTURE;
    }

    mRangeMapped = (data->se_data->RANGE_MAPY_FLAG || data->se_data->RANGE_MAPUV_FLAG || data->se_data->RANGERED);

    int frameType = data->pic_data[0].pic_parms->picture_fields.bits.picture_type;
    mAcquiredBuffer->referenceFrame = (frameType == VC1_PTYPE_I || frameType == VC1_PTYPE_P);

    // TODO: handle multiple frames parsed from a sample buffer
    int numPictures = (data->num_pictures > 1) ? 2 : 1;

    for (int index = 0; index < numPictures; index++) {
        status = decodePicture(data, index);
        if (status != DECODE_SUCCESS) {
            endDecodingFrame(true);
            return status;
        }
    }

    if (mRangeMapped) {
        updateDeblockedPicIndexes(frameType);
    }

    // let outputSurfaceBuffer handle "asReference" for VC1
    status = outputSurfaceBuffer();
    return status;
}


Decode_Status VideoDecoderWMV::decodePicture(vbp_data_vc1 *data, int32_t picIndex) {
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    Decode_Status status;
    int32_t bufferIDCount = 0;
    vbp_picture_data_vc1 *picData = &(data->pic_data[picIndex]);
    VAPictureParameterBufferVC1 *picParams = picData->pic_parms;

    if (picParams == NULL) {
        return DECODE_PARSER_FAIL;
    }

    status = allocateVABufferIDs(picData->num_slices * 2 + 2);
    CHECK_STATUS("allocateVABufferIDs");

    status = setReference(picParams, picIndex, mAcquiredBuffer->renderBuffer.surface);
    CHECK_STATUS("setReference");

    if (mRangeMapped) {
        // keep the destination surface for the picture after decoding and in-loop filtering
        picParams->inloop_decoded_picture = mExtraSurfaces[mDeblockedCurrPicIndex];
    } else {
        picParams->inloop_decoded_picture = VA_INVALID_SURFACE;
    }

    vaStatus = vaBeginPicture(mVADisplay, mVAContext, mAcquiredBuffer->renderBuffer.surface);
    CHECK_VA_STATUS("vaBeginPicture");
    // setting mDecodingFrame to true so vaEndPicture will be invoked to end the picture decoding.
    mDecodingFrame = true;

    vaStatus = vaCreateBuffer(
            mVADisplay,
            mVAContext,
            VAPictureParameterBufferType,
            sizeof(VAPictureParameterBufferVC1),
            1,
            picParams,
            &mBufferIDs[bufferIDCount]);
    CHECK_VA_STATUS("vaCreatePictureParameterBuffer");
    bufferIDCount++;

    if (picParams->bitplane_present.value) {
        vaStatus = vaCreateBuffer(
                mVADisplay,
                mVAContext,
                VABitPlaneBufferType,
                picData->size_bitplanes,
                1,
                picData->packed_bitplanes,
                &mBufferIDs[bufferIDCount]);
        CHECK_VA_STATUS("vaCreateBitPlaneBuffer");
        bufferIDCount++;
    }

    for (uint32_t i = 0; i < picData->num_slices; i++) {
        vaStatus = vaCreateBuffer(
                mVADisplay,
                mVAContext,
                VASliceParameterBufferType,
                sizeof(VASliceParameterBufferVC1),
                1,
                &(picData->slc_data[i].slc_parms),
                &mBufferIDs[bufferIDCount]);
        CHECK_VA_STATUS("vaCreateSliceParameterBuffer");
        bufferIDCount++;

        vaStatus = vaCreateBuffer(
                mVADisplay,
                mVAContext,
                VASliceDataBufferType,
                //size
                picData->slc_data[i].slice_size,
                //num_elements
                1,
                //slice data buffer pointer
                //Note that this is the original data buffer ptr;
                // offset to the actual slice data is provided in
                // slice_data_offset in VASliceParameterBufferVC1
                picData->slc_data[i].buffer_addr + picData->slc_data[i].slice_offset,
                &mBufferIDs[bufferIDCount]);
        CHECK_VA_STATUS("vaCreateSliceDataBuffer");
        bufferIDCount++;
    }

    vaStatus = vaRenderPicture(
            mVADisplay,
            mVAContext,
            mBufferIDs,
            bufferIDCount);
    CHECK_VA_STATUS("vaRenderPicture");

    vaStatus = vaEndPicture(mVADisplay, mVAContext);
    mDecodingFrame = false;
    CHECK_VA_STATUS("vaRenderPicture");

    return DECODE_SUCCESS;
}


Decode_Status VideoDecoderWMV::setReference(
        VAPictureParameterBufferVC1 *params,
        int32_t picIndex,
        VASurfaceID current) {
    int frameType = params->picture_fields.bits.picture_type;
    switch (frameType) {
        case VC1_PTYPE_I:
            params->forward_reference_picture = current;
            params->backward_reference_picture = current;
            break;
        case VC1_PTYPE_P:
            // check REFDIST in the picture parameter buffer
            if (0 != params->reference_fields.bits.reference_distance_flag &&
                0 != params->reference_fields.bits.reference_distance) {
                /* The previous decoded frame (distance is up to 16 but not 0) is used
                            for reference. Not supported here.
                            */
                return DECODE_NO_REFERENCE;
            }
            if (1 == picIndex) {
                // handle interlace field coding case
                if (1 == params->reference_fields.bits.num_reference_pictures ||
                    1 == params->reference_fields.bits.reference_field_pic_indicator) {
                    /*
                                    two reference fields or the second closest I/P field is used for
                                    prediction. Set forward reference picture to INVALID so it will be
                                    updated to a valid previous reconstructed reference frame later.
                                    */
                    params->forward_reference_picture = VA_INVALID_SURFACE;
                } else {
                   /* the closest I/P is used for reference so it must be the
                                  complementary field in the same surface.
                                 */
                    params->forward_reference_picture = current;
                }
            }
            if (VA_INVALID_SURFACE == params->forward_reference_picture) {
                if (mLastReference == NULL) {
                    return DECODE_NO_REFERENCE;
                }
                params->forward_reference_picture = mLastReference->renderBuffer.surface;
            }
            params->backward_reference_picture = VA_INVALID_SURFACE;
            break;
        case VC1_PTYPE_B:
            if (mForwardReference == NULL || mLastReference == NULL) {
                return DECODE_NO_REFERENCE;
            }
            params->forward_reference_picture = mForwardReference->renderBuffer.surface;
            params->backward_reference_picture = mLastReference->renderBuffer.surface;
            break;
        case VC1_PTYPE_BI:
            params->forward_reference_picture = VA_INVALID_SURFACE;
            params->backward_reference_picture = VA_INVALID_SURFACE;
            break;
        case VC1_PTYPE_SKIPPED:
            //Will never happen here
            break;
        default:
            break;
    }
    return DECODE_SUCCESS;
}

void VideoDecoderWMV::updateDeblockedPicIndexes(int frameType) {
    int32_t curPicIndex = mDeblockedCurrPicIndex;

    /* Out Loop (range map) buffers */
    if (frameType != VC1_PTYPE_SKIPPED) {
        if ((frameType == VC1_PTYPE_I) || (frameType == VC1_PTYPE_P)) {
            mDeblockedCurrPicIndex = mDeblockedLastPicIndex;
            mDeblockedLastPicIndex = curPicIndex;
        } else {
            mDeblockedCurrPicIndex = mDeblockedForwardPicIndex;
            mDeblockedForwardPicIndex = curPicIndex;
        }
    }
}

Decode_Status VideoDecoderWMV::updateConfigData(
        uint8_t *configData,
        int32_t configDataLen,
        uint8_t **newConfigData,
        int32_t* newConfigDataLen) {
    int32_t i = 0;
    uint8_t *p = configData;

    /* Check for start codes.  If one exist, then this is VC-1 and not WMV. */
    while (i < configDataLen - 2) {
        if ((p[i] == 0) &&
            (p[i + 1] == 0) &&
            (p[i + 2] == 1)) {
            *newConfigData = NULL;
            *newConfigDataLen = 0;
            return DECODE_SUCCESS;
        }
        i++;
    }

    *newConfigDataLen = configDataLen + 9;
    p = *newConfigData = new uint8_t [*newConfigDataLen];
    if (!p) {
       return DECODE_MEMORY_FAIL;
    }

    /* If we get here we have 4+ bytes of codec data that must be formatted */
    /* to pass through as an RCV sequence header. */
    p[0] = 0;
    p[1] = 0;
    p[2] = 1;
    p[3] = 0x0f;  /* Start code. */
    p[4] = (mVideoFormatInfo.width >> 8) & 0x0ff;
    p[5] = mVideoFormatInfo.width & 0x0ff;
    p[6] = (mVideoFormatInfo.height >> 8) & 0x0ff;
    p[7] = mVideoFormatInfo.height & 0x0ff;

    memcpy(p + 8, configData, configDataLen);
    *(p + configDataLen + 8) = 0x80;

    return DECODE_SUCCESS;
}

Decode_Status VideoDecoderWMV::startVA(vbp_data_vc1 *data) {
    updateFormatInfo(data);

    VAProfile vaProfile;
    switch (data->se_data->PROFILE) {
        case 0:
        vaProfile = VAProfileVC1Simple;
        break;
        case 1:
        vaProfile = VAProfileVC1Main;
        break;
        default:
        vaProfile = VAProfileVC1Advanced;
        break;
    }

    return VideoDecoderBase::setupVA(VC1_SURFACE_NUMBER, vaProfile, VC1_EXTRA_SURFACE_NUMBER);
}

void VideoDecoderWMV::updateFormatInfo(vbp_data_vc1 *data) {
    ITRACE("updateFormatInfo: current size: %d x %d, new size: %d x %d",
        mVideoFormatInfo.width, mVideoFormatInfo.height,
        data->se_data->CODED_WIDTH, data->se_data->CODED_HEIGHT);

    mVideoFormatInfo.cropBottom = data->se_data->CODED_HEIGHT > mVideoFormatInfo.height ?
                                                                           data->se_data->CODED_HEIGHT - mVideoFormatInfo.height : 0;
    mVideoFormatInfo.cropRight = data->se_data->CODED_WIDTH > mVideoFormatInfo.width ?
                                                                      data->se_data->CODED_WIDTH - mVideoFormatInfo.width : 0;

     if ((mVideoFormatInfo.width != data->se_data->CODED_WIDTH ||
        mVideoFormatInfo.height != data->se_data->CODED_HEIGHT) &&
        data->se_data->CODED_WIDTH &&
        data->se_data->CODED_HEIGHT) {
        // encoded image size
        mVideoFormatInfo.width = data->se_data->CODED_WIDTH;
        mVideoFormatInfo.height = data->se_data->CODED_HEIGHT;
        mSizeChanged = true;
        ITRACE("Video size is changed.");
    }

    // scaling has been performed on the decoded image.
    mVideoFormatInfo.videoRange = 1;

    switch (data->se_data->MATRIX_COEF) {
        case 1:
            mVideoFormatInfo.colorMatrix = VA_SRC_BT709;
            break;
        // ITU-R BT.1700, ITU-R BT.601-5, and SMPTE 293M-1996.
        case 6:
            mVideoFormatInfo.colorMatrix = VA_SRC_BT601;
            break;
        default:
            // unknown color matrix, set to 0 so color space flag will not be set.
            mVideoFormatInfo.colorMatrix = 0;
            break;
    }

    mVideoFormatInfo.aspectX = data->se_data->ASPECT_HORIZ_SIZE;
    mVideoFormatInfo.aspectY = data->se_data->ASPECT_VERT_SIZE;
    mVideoFormatInfo.bitrate = 0; //data->se_data->bitrate;
    mVideoFormatInfo.valid = true;

    setRenderRect();
}

Decode_Status VideoDecoderWMV::allocateVABufferIDs(int32_t number) {
    if (mNumBufferIDs > number) {
        return DECODE_SUCCESS;
    }
    if (mBufferIDs) {
        delete [] mBufferIDs;
    }
    mBufferIDs = NULL;
    mNumBufferIDs = 0;
    mBufferIDs = new VABufferID [number];
    if (mBufferIDs == NULL) {
        return DECODE_MEMORY_FAIL;
    }
    mNumBufferIDs = number;
    return DECODE_SUCCESS;
}

Decode_Status VideoDecoderWMV::parseBuffer(uint8_t *data, int32_t size, vbp_data_vc1 **vbpData) {
    Decode_Status status;

    if (data == NULL || size == 0) {
        return DECODE_INVALID_DATA;
    }

    if (mConfigDataParsed) {
        status = VideoDecoderBase::parseBuffer(data, size, false, (void**)vbpData);
        CHECK_STATUS("VideoDecoderBase::parseBuffer");
    } else {
        uint8_t *newData = NULL;
        int32_t newSize = 0;
        status = updateConfigData(data, size, &newData, &newSize);
        CHECK_STATUS("updateConfigData");

        if (newSize) {
            status = VideoDecoderBase::parseBuffer(newData, newSize, true, (void**)vbpData);
            delete [] newData;
        } else {
            status = VideoDecoderBase::parseBuffer(data, size, true, (void**)vbpData);
        }
        CHECK_STATUS("VideoDecoderBase::parseBuffer");
        mConfigDataParsed = true;
    }
    return DECODE_SUCCESS;
}


Decode_Status VideoDecoderWMV::checkHardwareCapability() {
#ifndef USE_GEN_HW
    VAStatus vaStatus;
    VAConfigAttrib cfgAttribs[2];
    cfgAttribs[0].type = VAConfigAttribMaxPictureWidth;
    cfgAttribs[1].type = VAConfigAttribMaxPictureHeight;
    vaStatus = vaGetConfigAttributes(mVADisplay, VAProfileVC1Advanced,
            VAEntrypointVLD, cfgAttribs, 2);
    CHECK_VA_STATUS("vaGetConfigAttributes");
    if (cfgAttribs[0].value * cfgAttribs[1].value < (uint32_t)mVideoFormatInfo.width * (uint32_t)mVideoFormatInfo.height) {
        ETRACE("hardware supports resolution %d * %d smaller than the clip resolution %d * %d",
                cfgAttribs[0].value, cfgAttribs[1].value, mVideoFormatInfo.width, mVideoFormatInfo.height);
        return DECODE_DRIVER_FAIL;
    }
#endif
    return DECODE_SUCCESS;
}


