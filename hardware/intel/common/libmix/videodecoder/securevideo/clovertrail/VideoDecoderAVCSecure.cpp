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

#include "VideoDecoderAVCSecure.h"
#include "VideoDecoderTrace.h"
#include <string.h>


#define STARTCODE_00                0x00
#define STARTCODE_01                0x01
#define STARTCODE_PREFIX_LEN        3
#define NALU_TYPE_MASK              0x1F


// mask for little endian, to mast the second and fourth bytes in the byte stream
#define STARTCODE_MASK0             0xFF000000 //0x00FF0000
#define STARTCODE_MASK1             0x0000FF00  //0x000000FF


typedef enum {
    NAL_UNIT_TYPE_unspecified0 = 0,
    NAL_UNIT_TYPE_SLICE,
    NAL_UNIT_TYPE_DPA,
    NAL_UNIT_TYPE_DPB,
    NAL_UNIT_TYPE_DPC,
    NAL_UNIT_TYPE_IDR,
    NAL_UNIT_TYPE_SEI,
    NAL_UNIT_TYPE_SPS,
    NAL_UNIT_TYPE_PPS,
    NAL_UNIT_TYPE_Acc_unit_delimiter,
    NAL_UNIT_TYPE_EOSeq,
    NAL_UNIT_TYPE_EOstream,
    NAL_UNIT_TYPE_filler_data,
    NAL_UNIT_TYPE_SPS_extension,
    NAL_UNIT_TYPE_Reserved14,
    NAL_UNIT_TYPE_Reserved15,
    NAL_UNIT_TYPE_Reserved16,
    NAL_UNIT_TYPE_Reserved17,
    NAL_UNIT_TYPE_Reserved18,
    NAL_UNIT_TYPE_ACP,
    NAL_UNIT_TYPE_Reserved20,
    NAL_UNIT_TYPE_Reserved21,
    NAL_UNIT_TYPE_Reserved22,
    NAL_UNIT_TYPE_Reserved23,
    NAL_UNIT_TYPE_unspecified24,
} NAL_UNIT_TYPE;

#ifndef min
#define min(X, Y)  ((X) <(Y) ? (X) : (Y))
#endif


static const uint8_t startcodePrefix[STARTCODE_PREFIX_LEN] = {0x00, 0x00, 0x01};


VideoDecoderAVCSecure::VideoDecoderAVCSecure(const char *mimeType)
    : VideoDecoderAVC(mimeType),
      mNaluHeaderBuffer(NULL),
      mInputBuffer(NULL) {

    memset(&mMetadata, 0, sizeof(NaluMetadata));
    memset(&mByteStream, 0, sizeof(NaluByteStream));
}

VideoDecoderAVCSecure::~VideoDecoderAVCSecure() {
}

Decode_Status VideoDecoderAVCSecure::start(VideoConfigBuffer *buffer) {
    Decode_Status status = VideoDecoderAVC::start(buffer);
    if (status != DECODE_SUCCESS) {
        return status;
    }

    mMetadata.naluInfo = new NaluInfo [MAX_NALU_NUMBER];
    mByteStream.byteStream = new uint8_t [MAX_NALU_HEADER_BUFFER];
    mNaluHeaderBuffer = new uint8_t [MAX_NALU_HEADER_BUFFER];

    if (mMetadata.naluInfo == NULL ||
        mByteStream.byteStream == NULL ||
        mNaluHeaderBuffer == NULL) {
        ETRACE("Failed to allocate memory.");
        // TODO: release all allocated memory
        return DECODE_MEMORY_FAIL;
    }
    return status;
}

void VideoDecoderAVCSecure::stop(void) {
    VideoDecoderAVC::stop();

    if (mMetadata.naluInfo) {
        delete [] mMetadata.naluInfo;
        mMetadata.naluInfo = NULL;
    }

    if (mByteStream.byteStream) {
        delete [] mByteStream.byteStream;
        mByteStream.byteStream = NULL;
    }

    if (mNaluHeaderBuffer) {
        delete [] mNaluHeaderBuffer;
        mNaluHeaderBuffer = NULL;
    }
}

Decode_Status VideoDecoderAVCSecure::decode(VideoDecodeBuffer *buffer) {
    Decode_Status status;
    int32_t sizeAccumulated = 0;
    int32_t sizeLeft = 0;
    uint8_t *pByteStream = NULL;
    NaluInfo *pNaluInfo = mMetadata.naluInfo;

    if (buffer->flag & IS_SECURE_DATA) {
        pByteStream = buffer->data;
        sizeLeft = buffer->size;
        mInputBuffer = NULL;
    } else {
        status = parseAnnexBStream(buffer->data, buffer->size, &mByteStream);
        CHECK_STATUS("parseAnnexBStream");
        pByteStream = mByteStream.byteStream;
        sizeLeft = mByteStream.streamPos;
        mInputBuffer = buffer->data;
    }
    if (sizeLeft < 4) {
        ETRACE("Not enough data to read number of NALU.");
        return DECODE_INVALID_DATA;
    }

    // read number of NALU
    memcpy(&(mMetadata.naluNumber), pByteStream, sizeof(int32_t));
    pByteStream += 4;
    sizeLeft -= 4;

    if (mMetadata.naluNumber == 0) {
        WTRACE("Number of NALU is ZERO!");
        return DECODE_SUCCESS;
    }

    for (int32_t i = 0; i < mMetadata.naluNumber; i++) {
        if (sizeLeft < 12) {
            ETRACE("Not enough data to parse NALU offset, size, header length for NALU %d, left = %d", i, sizeLeft);
            return DECODE_INVALID_DATA;
        }
        sizeLeft -= 12;
        // read NALU offset
        memcpy(&(pNaluInfo->naluOffset), pByteStream, sizeof(int32_t));
        pByteStream += 4;

        // read NALU size
        memcpy(&(pNaluInfo->naluLen), pByteStream, sizeof(int32_t));
        pByteStream += 4;

        // read NALU header length
        memcpy(&(pNaluInfo->naluHeaderLen), pByteStream, sizeof(int32_t));
        pByteStream += 4;

        if (sizeLeft < pNaluInfo->naluHeaderLen) {
            ETRACE("Not enough data to copy NALU header for %d, left = %d, header len = %d", i, sizeLeft, pNaluInfo->naluHeaderLen);
            return DECODE_INVALID_DATA;
        }

        sizeLeft -=  pNaluInfo->naluHeaderLen;

        if (pNaluInfo->naluHeaderLen) {
            // copy start code prefix to buffer
            memcpy(mNaluHeaderBuffer + sizeAccumulated,
                startcodePrefix,
                STARTCODE_PREFIX_LEN);
            sizeAccumulated += STARTCODE_PREFIX_LEN;

            // copy NALU header
            memcpy(mNaluHeaderBuffer + sizeAccumulated, pByteStream, pNaluInfo->naluHeaderLen);
            pByteStream += pNaluInfo->naluHeaderLen;

            sizeAccumulated += pNaluInfo->naluHeaderLen;
        } else {
            WTRACE("header len is zero for NALU %d", i);
        }

        // for next NALU
        pNaluInfo++;
    }

    buffer->data = mNaluHeaderBuffer;
    buffer->size = sizeAccumulated;

    return VideoDecoderAVC::decode(buffer);
}


Decode_Status VideoDecoderAVCSecure::decodeSlice(vbp_data_h264 *data, uint32_t picIndex, uint32_t sliceIndex) {

    Decode_Status status;
    VAStatus vaStatus;
    uint32_t bufferIDCount = 0;
    // maximum 4 buffers to render a slice: picture parameter, IQMatrix, slice parameter, slice data
    VABufferID bufferIDs[4];

    vbp_picture_data_h264 *picData = &(data->pic_data[picIndex]);
    vbp_slice_data_h264 *sliceData = &(picData->slc_data[sliceIndex]);
    VAPictureParameterBufferH264 *picParam = picData->pic_parms;
    VASliceParameterBufferH264 *sliceParam = &(sliceData->slc_parms);

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

        // Check there is no reference frame loss before decoding a frame

        // Update  the reference frames and surface IDs for DPB and current frame
        status = updateDPB(picParam);
        CHECK_STATUS("updateDPB");

        //We have to provide a hacked DPB rather than complete DPB for libva as workaround
        status = updateReferenceFrames(picData);
        CHECK_STATUS("updateReferenceFrames");

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
    }

    status = setReference(sliceParam);
    CHECK_STATUS("setReference");

    // find which naluinfo is correlated to current slice
    int naluIndex = 0;
    uint32_t accumulatedHeaderLen = 0;
    uint32_t headerLen = 0;
    for (; naluIndex < mMetadata.naluNumber; naluIndex++)  {
        headerLen = mMetadata.naluInfo[naluIndex].naluHeaderLen;
        if (headerLen == 0) {
            WTRACE("lenght of current NAL unit is 0.");
            continue;
        }
        accumulatedHeaderLen += STARTCODE_PREFIX_LEN;
        if (accumulatedHeaderLen + headerLen > sliceData->slice_offset) {
            break;
        }
        accumulatedHeaderLen += headerLen;
    }

    if (sliceData->slice_offset != accumulatedHeaderLen) {
        WTRACE("unexpected slice offset %d, accumulatedHeaderLen = %d", sliceData->slice_offset, accumulatedHeaderLen);
    }

    sliceParam->slice_data_size = mMetadata.naluInfo[naluIndex].naluLen;
    sliceData->slice_size = sliceParam->slice_data_size;

    // no need to update:
    // sliceParam->slice_data_offset - 0 always
    // sliceParam->slice_data_bit_offset - relative to  sliceData->slice_offset

    vaStatus = vaCreateBuffer(
        mVADisplay,
        mVAContext,
        VASliceParameterBufferType,
        sizeof(VASliceParameterBufferH264),
        1,
        sliceParam,
        &bufferIDs[bufferIDCount]);
    CHECK_VA_STATUS("vaCreateSliceParameterBuffer");
    bufferIDCount++;

    // sliceData->slice_offset - accumulatedHeaderLen is the absolute offset to start codes of current NAL unit
    // offset points to first byte of NAL unit
    uint32_t sliceOffset = mMetadata.naluInfo[naluIndex].naluOffset;
    if (mInputBuffer != NULL) {
        vaStatus = vaCreateBuffer(
            mVADisplay,
            mVAContext,
            VASliceDataBufferType,
            sliceData->slice_size, //size
            1,        //num_elements
            mInputBuffer  + sliceOffset,
            &bufferIDs[bufferIDCount]);
    } else {
        vaStatus = vaCreateBuffer(
            mVADisplay,
            mVAContext,
            VAProtectedSliceDataBufferType,
            sliceData->slice_size, //size
            1,        //num_elements
            (uint8_t*)sliceOffset, // IMR offset
            &bufferIDs[bufferIDCount]);
    }
    CHECK_VA_STATUS("vaCreateSliceDataBuffer");
    bufferIDCount++;

    vaStatus = vaRenderPicture(
        mVADisplay,
        mVAContext,
        bufferIDs,
        bufferIDCount);
    CHECK_VA_STATUS("vaRenderPicture");

    return DECODE_SUCCESS;
}


// Parse byte string pattern "0x000001" (3 bytes)  in the current buffer.
// Returns offset of position following  the pattern in the buffer if pattern is found or -1 if not found.
int32_t VideoDecoderAVCSecure::findNalUnitOffset(uint8_t *stream, int32_t offset, int32_t length) {
    uint8_t *ptr;
    uint32_t left = 0, data = 0, phase = 0;
    uint8_t mask1 = 0, mask2 = 0;

    /* Meaning of phase:
        0: initial status, "0x000001" bytes are not found so far;
        1: one "0x00" byte is found;
        2: two or more consecutive "0x00" bytes" are found;
        3: "0x000001" patten is found ;
        4: if there is one more byte after "0x000001";
       */

    left = length;
    ptr = (uint8_t *) (stream + offset);
    phase = 0;

    // parse until there is more data and start code not found
    while ((left > 0) && (phase < 3)) {
        // Check if the address is 32-bit aligned & phase=0, if thats the case we can check 4 bytes instead of one byte at a time.
        if (((((uint32_t)ptr) & 0x3) == 0) && (phase == 0)) {
            while (left > 3) {
                data = *((uint32_t *)ptr);
                mask1 = (STARTCODE_00 != (data & STARTCODE_MASK0));
                mask2 = (STARTCODE_00 != (data & STARTCODE_MASK1));
                // If second byte and fourth byte are not zero's then we cannot have a start code here,
                //  as we need two consecutive zero bytes for a start code pattern.
                if (mask1 && mask2) {
                    // skip 4 bytes and start over
                    ptr += 4;
                    left -=4;
                    continue;
                } else {
                    break;
                }
            }
        }

        // At this point either data is not on a 32-bit boundary or phase > 0 so we look at one byte at a time
        if (left > 0) {
            if (*ptr == STARTCODE_00) {
                phase++;
                if (phase > 2) {
                    // more than 2 consecutive '0x00' bytes is found
                    phase = 2;
                }
            } else if ((*ptr == STARTCODE_01) && (phase == 2)) {
                // start code is found
                phase = 3;
            } else {
                // reset lookup
                phase = 0;
            }
            ptr++;
            left--;
        }
    }

    if ((left > 0) && (phase == 3)) {
        phase = 4;
        // return offset of position following the pattern in the buffer which matches "0x000001" byte string
        return (int32_t)(ptr - stream);
    }
    return -1;
}


Decode_Status VideoDecoderAVCSecure::copyNaluHeader(uint8_t *stream, NaluByteStream *naluStream) {
    uint8_t naluType;
    int32_t naluHeaderLen;

    naluType = *(uint8_t *)(stream + naluStream->naluOffset);
    naluType &= NALU_TYPE_MASK;
    // first update nalu header length based on nalu type
    if (naluType >= NAL_UNIT_TYPE_SLICE && naluType <= NAL_UNIT_TYPE_IDR) {
        // coded slice, return only up to MAX_SLICE_HEADER_SIZE bytes
        naluHeaderLen = min(naluStream->naluLen, MAX_SLICE_HEADER_SIZE);
    } else if (naluType >= NAL_UNIT_TYPE_SEI && naluType <= NAL_UNIT_TYPE_PPS) {
        //sps, pps, sei, etc, return the entire NAL unit in clear
        naluHeaderLen = naluStream->naluLen;
    } else {
        return DECODE_FRAME_DROPPED;
    }

    memcpy(naluStream->byteStream + naluStream->streamPos, &(naluStream->naluOffset), sizeof(int32_t));
    naluStream->streamPos += 4;

    memcpy(naluStream->byteStream + naluStream->streamPos, &(naluStream->naluLen), sizeof(int32_t));
    naluStream->streamPos += 4;

    memcpy(naluStream->byteStream + naluStream->streamPos, &naluHeaderLen, sizeof(int32_t));
    naluStream->streamPos += 4;

    if (naluHeaderLen) {
        memcpy(naluStream->byteStream + naluStream->streamPos, (uint8_t*)(stream + naluStream->naluOffset), naluHeaderLen);
        naluStream->streamPos += naluHeaderLen;
    }
    return DECODE_SUCCESS;
}


// parse start-code prefixed stream, also knowns as Annex B byte stream, commonly used in AVI, ES, MPEG2 TS container
Decode_Status VideoDecoderAVCSecure::parseAnnexBStream(uint8_t *stream, int32_t length, NaluByteStream *naluStream) {
    int32_t naluOffset, offset, left;
    NaluInfo *info;
    uint32_t ret = DECODE_SUCCESS;

    naluOffset = 0;
    offset = 0;
    left = length;

    // leave 4 bytes to copy nalu count
    naluStream->streamPos = 4;
    naluStream->naluCount = 0;
    memset(naluStream->byteStream, 0, MAX_NALU_HEADER_BUFFER);

    for (; ;) {
        naluOffset = findNalUnitOffset(stream, offset, left);
        if (naluOffset == -1) {
            break;
        }

        if (naluStream->naluCount == 0) {
            naluStream->naluOffset = naluOffset;
        } else {
            naluStream->naluLen = naluOffset - naluStream->naluOffset - STARTCODE_PREFIX_LEN;
            ret = copyNaluHeader(stream, naluStream);
            if (ret != DECODE_SUCCESS && ret != DECODE_FRAME_DROPPED) {
                LOGW("copyNaluHeader returned %d", ret);
                return ret;
            }
            // starting position for next NALU
            naluStream->naluOffset = naluOffset;
        }

        if (ret == DECODE_SUCCESS) {
            naluStream->naluCount++;
        }

        // update next lookup position and length
        offset = naluOffset + 1; // skip one byte of NAL unit type
        left = length - offset;
    }

    if (naluStream->naluCount > 0) {
        naluStream->naluLen = length - naluStream->naluOffset;
        memcpy(naluStream->byteStream, &(naluStream->naluCount), sizeof(int32_t));
        // ignore return value, either DECODE_SUCCESS or DECODE_FRAME_DROPPED
        copyNaluHeader(stream, naluStream);
        return DECODE_SUCCESS;
    }

    LOGW("number of valid NALU is 0!");
    return DECODE_SUCCESS;
}

