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
#include "VideoEncoderAVC.h"
#include <va/va_tpi.h>
#include <va/va_enc_h264.h>
#include <bitstream.h>

VideoEncoderAVC::VideoEncoderAVC()
    :VideoEncoderBase() {
    if(VideoEncoderBase::queryProfileLevelConfig(mVADisplay, VAProfileH264High) == ENCODE_SUCCESS){
        mComParams.profile = VAProfileH264High;
        mComParams.level = 42;
    }else if(VideoEncoderBase::queryProfileLevelConfig(mVADisplay, VAProfileH264Main) == ENCODE_SUCCESS){
        mComParams.profile = VAProfileH264Main;
        mComParams.level = 41;
    }
    mVideoParamsAVC.basicUnitSize = 0;
    mVideoParamsAVC.VUIFlag = 0;
    mVideoParamsAVC.sliceNum.iSliceNum = 2;
    mVideoParamsAVC.sliceNum.pSliceNum = 2;
    mVideoParamsAVC.idrInterval = 2;
    mVideoParamsAVC.ipPeriod = 1;
    mVideoParamsAVC.maxSliceSize = 0;
    mVideoParamsAVC.delimiterType = AVC_DELIMITER_ANNEXB;
    mSliceNum = 2;
    mVideoParamsAVC.crop.LeftOffset = 0;
    mVideoParamsAVC.crop.RightOffset = 0;
    mVideoParamsAVC.crop.TopOffset = 0;
    mVideoParamsAVC.crop.BottomOffset = 0;
    mVideoParamsAVC.SAR.SarWidth = 0;
    mVideoParamsAVC.SAR.SarHeight = 0;
    mVideoParamsAVC.bEntropyCodingCABAC = 0;
    mVideoParamsAVC.bWeightedPPrediction = 0;
    mVideoParamsAVC.bDirect8x8Inference = 0;
    mVideoParamsAVC.bConstIpred = 0;
    mAutoReferenceSurfaceNum = 4;

    packed_seq_header_param_buf_id = VA_INVALID_ID;
    packed_seq_buf_id = VA_INVALID_ID;
    packed_pic_header_param_buf_id = VA_INVALID_ID;
    packed_pic_buf_id = VA_INVALID_ID;
    packed_sei_header_param_buf_id = VA_INVALID_ID;   /* the SEI buffer */
    packed_sei_buf_id = VA_INVALID_ID;
}

Encode_Status VideoEncoderAVC::start() {

    Encode_Status ret = ENCODE_SUCCESS;
    LOG_V( "Begin\n");

    if (mComParams.rcMode == VA_RC_VCM) {
        // If we are in VCM, we will set slice num to max value
        // mVideoParamsAVC.sliceNum.iSliceNum = (mComParams.resolution.height + 15) / 16;
        // mVideoParamsAVC.sliceNum.pSliceNum = mVideoParamsAVC.sliceNum.iSliceNum;
    }

    ret = VideoEncoderBase::start ();
    CHECK_ENCODE_STATUS_RETURN("VideoEncoderBase::start");

    LOG_V( "end\n");
    return ret;
}

Encode_Status VideoEncoderAVC::derivedSetParams(VideoParamConfigSet *videoEncParams) {

    CHECK_NULL_RETURN_IFFAIL(videoEncParams);
    VideoParamsAVC *encParamsAVC = reinterpret_cast <VideoParamsAVC *> (videoEncParams);

    // AVC parames
    if (encParamsAVC->size != sizeof (VideoParamsAVC)) {
        return ENCODE_INVALID_PARAMS;
    }

    if(encParamsAVC->ipPeriod == 0 || encParamsAVC->ipPeriod >4)
        return ENCODE_INVALID_PARAMS;

    if((mComParams.intraPeriod >1)&&(mComParams.intraPeriod % encParamsAVC->ipPeriod !=0))
        return ENCODE_INVALID_PARAMS;

    mVideoParamsAVC = *encParamsAVC;
    if(mComParams.profile == VAProfileH264Baseline){
        mVideoParamsAVC.bEntropyCodingCABAC = 0;
        mVideoParamsAVC.bDirect8x8Inference = 0;
        mVideoParamsAVC.bWeightedPPrediction = 0;
    }
    return ENCODE_SUCCESS;
}

Encode_Status VideoEncoderAVC:: derivedGetParams(VideoParamConfigSet *videoEncParams) {

    CHECK_NULL_RETURN_IFFAIL(videoEncParams);
    VideoParamsAVC *encParamsAVC = reinterpret_cast <VideoParamsAVC *> (videoEncParams);

    // AVC parames
    if (encParamsAVC->size != sizeof (VideoParamsAVC)) {
        return ENCODE_INVALID_PARAMS;
    }

    *encParamsAVC = mVideoParamsAVC;
    return ENCODE_SUCCESS;

}

Encode_Status VideoEncoderAVC::derivedSetConfig(VideoParamConfigSet *videoEncConfig) {

    CHECK_NULL_RETURN_IFFAIL(videoEncConfig);
    LOG_V("Config type = %d\n", (int)videoEncConfig->type);

    switch (videoEncConfig->type) {
        case VideoConfigTypeAVCIntraPeriod: {

            VideoConfigAVCIntraPeriod *configAVCIntraPeriod =
                    reinterpret_cast <VideoConfigAVCIntraPeriod *> (videoEncConfig);
            // Config Intra Peroid
            if (configAVCIntraPeriod->size != sizeof (VideoConfigAVCIntraPeriod)) {
                return ENCODE_INVALID_PARAMS;
            }

            if(configAVCIntraPeriod->ipPeriod == 0 || configAVCIntraPeriod->ipPeriod >4)
                return ENCODE_INVALID_PARAMS;
            if((configAVCIntraPeriod->intraPeriod >1)&&(configAVCIntraPeriod->intraPeriod % configAVCIntraPeriod->ipPeriod !=0))
                return ENCODE_INVALID_PARAMS;

            mVideoParamsAVC.idrInterval = configAVCIntraPeriod->idrInterval;
            mVideoParamsAVC.ipPeriod = configAVCIntraPeriod->ipPeriod;
            mComParams.intraPeriod = configAVCIntraPeriod->intraPeriod;
            mNewHeader = true;
            break;
        }
        case VideoConfigTypeNALSize: {
            // Config MTU
            VideoConfigNALSize *configNALSize =
                    reinterpret_cast <VideoConfigNALSize *> (videoEncConfig);
            if (configNALSize->size != sizeof (VideoConfigNALSize)) {
                return ENCODE_INVALID_PARAMS;
            }

            mVideoParamsAVC.maxSliceSize = configNALSize->maxSliceSize;
            mRenderMaxSliceSize = true;
            break;
        }
        case VideoConfigTypeIDRRequest: {
            if(mVideoParamsAVC.ipPeriod >1)
                return ENCODE_FAIL;
            else
                mNewHeader = true;
            break;
        }
        case VideoConfigTypeSliceNum: {

            VideoConfigSliceNum *configSliceNum =
                    reinterpret_cast <VideoConfigSliceNum *> (videoEncConfig);
            // Config Slice size
            if (configSliceNum->size != sizeof (VideoConfigSliceNum)) {
                return ENCODE_INVALID_PARAMS;
            }

            mVideoParamsAVC.sliceNum = configSliceNum->sliceNum;
            break;
        }
        default: {
            LOG_E ("Invalid Config Type");
            break;
        }
    }

    return ENCODE_SUCCESS;
}

Encode_Status VideoEncoderAVC:: derivedGetConfig(
        VideoParamConfigSet *videoEncConfig) {

    CHECK_NULL_RETURN_IFFAIL(videoEncConfig);
    LOG_V("Config type = %d\n", (int)videoEncConfig->type);

    switch (videoEncConfig->type) {

        case VideoConfigTypeAVCIntraPeriod: {

            VideoConfigAVCIntraPeriod *configAVCIntraPeriod =
                    reinterpret_cast <VideoConfigAVCIntraPeriod *> (videoEncConfig);
            if (configAVCIntraPeriod->size != sizeof (VideoConfigAVCIntraPeriod)) {
                return ENCODE_INVALID_PARAMS;
            }

            configAVCIntraPeriod->idrInterval = mVideoParamsAVC.idrInterval;
            configAVCIntraPeriod->intraPeriod = mComParams.intraPeriod;
            configAVCIntraPeriod->ipPeriod = mVideoParamsAVC.ipPeriod;

            break;
        }
        case VideoConfigTypeNALSize: {

            VideoConfigNALSize *configNALSize =
                    reinterpret_cast <VideoConfigNALSize *> (videoEncConfig);
            if (configNALSize->size != sizeof (VideoConfigNALSize)) {
                return ENCODE_INVALID_PARAMS;
            }

            configNALSize->maxSliceSize = mVideoParamsAVC.maxSliceSize;
            break;
        }
        case VideoConfigTypeIDRRequest: {
            break;

        }
        case VideoConfigTypeSliceNum: {

            VideoConfigSliceNum *configSliceNum =
                    reinterpret_cast <VideoConfigSliceNum *> (videoEncConfig);
            if (configSliceNum->size != sizeof (VideoConfigSliceNum)) {
                return ENCODE_INVALID_PARAMS;
            }

            configSliceNum->sliceNum = mVideoParamsAVC.sliceNum;
            break;
        }
        default: {
            LOG_E ("Invalid Config Type");
            break;
        }
    }

    return ENCODE_SUCCESS;
}

Encode_Status VideoEncoderAVC::updateFrameInfo(EncodeTask* task) {
    uint32_t idrPeroid = mComParams.intraPeriod * mVideoParamsAVC.idrInterval;
    FrameType frametype;
    uint32_t frame_num = mFrameNum;
    uint32_t intraPeriod = mComParams.intraPeriod;

    if (idrPeroid != 0) {
        if(mVideoParamsAVC.ipPeriod > 1)
            frame_num = frame_num % (idrPeroid + 1);
        else
            frame_num = frame_num % idrPeroid ;
    }else{
        if (mComParams.intraPeriod == 0)
            intraPeriod = 0xFFFFFFFF;
    }


    if(frame_num ==0){
        frametype = FTYPE_IDR;
    }else if(intraPeriod ==1)
        // only I frame need intraPeriod=idrInterval=ipPeriod=0
        frametype = FTYPE_I;
    else if(mVideoParamsAVC.ipPeriod == 1){ // no B frame
        if((frame_num >  1) &&((frame_num -1)%intraPeriod == 0))
            frametype = FTYPE_I;
        else
            frametype = FTYPE_P;
    } else {
        if(((frame_num-1)%intraPeriod == 0)&&(frame_num >intraPeriod))
            frametype = FTYPE_I;
        else{
            frame_num = frame_num%intraPeriod;
            if(frame_num == 0)
                frametype = FTYPE_B;
            else if((frame_num-1)%mVideoParamsAVC.ipPeriod == 0)
                frametype = FTYPE_P;
            else
                frametype = FTYPE_B;
        }
    }

    if (frametype == FTYPE_IDR || frametype == FTYPE_I)
        task->flag |= ENCODE_BUFFERFLAG_SYNCFRAME;

    if (frametype != task->type) {
        const char* FrameTypeStr[10] = {"UNKNOWN", "I", "P", "B", "SI", "SP", "EI", "EP", "S", "IDR"};
        if ((uint32_t) task->type < 9)
            LOG_V("libMIX thinks it is %s Frame, the input is %s Frame", FrameTypeStr[frametype], FrameTypeStr[task->type]);
        else
            LOG_V("Wrong Frame type %d, type may not be initialized ?\n", task->type);
    }

//temparily comment out to avoid uninitialize error
//    if (task->type == FTYPE_UNKNOWN || (uint32_t) task->type > 9)
        task->type = frametype;

    return ENCODE_SUCCESS;
}

Encode_Status VideoEncoderAVC::getExtFormatOutput(VideoEncOutputBuffer *outBuffer) {

    Encode_Status ret = ENCODE_SUCCESS;

    LOG_V("Begin\n");

    switch (outBuffer->format) {
        case OUTPUT_CODEC_DATA: {
            // Output the codec data
            ret = outputCodecData(outBuffer);
            CHECK_ENCODE_STATUS_CLEANUP("outputCodecData");
            break;
        }

        case OUTPUT_ONE_NAL: {
            // Output only one NAL unit
            ret = outputOneNALU(outBuffer, true);
            CHECK_ENCODE_STATUS_CLEANUP("outputOneNALU");
            break;
        }

        case OUTPUT_ONE_NAL_WITHOUT_STARTCODE: {
            ret = outputOneNALU(outBuffer, false);
            CHECK_ENCODE_STATUS_CLEANUP("outputOneNALU");
            break;
        }

        case OUTPUT_LENGTH_PREFIXED: {
            // Output length prefixed
            ret = outputLengthPrefixed(outBuffer);
            CHECK_ENCODE_STATUS_CLEANUP("outputLengthPrefixed");
            break;
        }

        case OUTPUT_NALULENGTHS_PREFIXED: {
            // Output nalu lengths ahead of bitstream
            ret = outputNaluLengthsPrefixed(outBuffer);
            CHECK_ENCODE_STATUS_CLEANUP("outputNaluLengthsPrefixed");
            break;
        }

        default:
            LOG_E("Invalid buffer mode\n");
            ret = ENCODE_FAIL;
            break;
    }

    LOG_V("out size is = %d\n", outBuffer->dataSize);


CLEAN_UP:


    LOG_V("End\n");
    return ret;
}

Encode_Status VideoEncoderAVC::getOneNALUnit(
        uint8_t *inBuffer, uint32_t bufSize, uint32_t *nalSize,
        uint32_t *nalType, uint32_t *nalOffset, uint32_t status) {
    uint32_t pos = 0;
    uint32_t zeroByteCount = 0;
    uint32_t singleByteTable[3][2] = {{1,0},{2,0},{2,3}};
    uint32_t dataRemaining = 0;
    uint8_t *dataPtr;

    // Don't need to check parameters here as we just checked by caller
    while ((inBuffer[pos++] == 0x00)) {
        zeroByteCount ++;
        if (pos >= bufSize)  //to make sure the buffer to be accessed is valid
            break;
    }

    if (inBuffer[pos - 1] != 0x01 || zeroByteCount < 2) {
        LOG_E("The stream is not AnnexB format \n");
        LOG_E("segment status is %x \n", status);
        return ENCODE_FAIL; //not AnnexB, we won't process it
    }

    *nalType = (*(inBuffer + pos)) & 0x1F;
    LOG_V ("NAL type = 0x%x\n", *nalType);

    zeroByteCount = 0;
    *nalOffset = pos;

    if (status & VA_CODED_BUF_STATUS_SINGLE_NALU) {
        *nalSize = bufSize - pos;
        return ENCODE_SUCCESS;
    }

    dataPtr  = inBuffer + pos;
    dataRemaining = bufSize - pos + 1;

    while ((dataRemaining > 0) && (zeroByteCount < 3)) {
        if (((((intptr_t)dataPtr) & 0xF ) == 0) && (0 == zeroByteCount)
               && (dataRemaining > 0xF)) {

            __asm__  (
                //Data input
                "movl %1, %%ecx\n\t"//data_ptr=>ecx
                "movl %0, %%eax\n\t"//data_remaing=>eax
                //Main compare loop
                //
                "0:\n\t"   //MATCH_8_ZERO:
                "pxor %%xmm0,%%xmm0\n\t"//set 0=>xmm0
                "pcmpeqb (%%ecx),%%xmm0\n\t"//data_ptr=xmm0,(byte==0)?0xFF:0x00
                "pmovmskb %%xmm0, %%edx\n\t"//edx[0]=xmm0[7],edx[1]=xmm0[15],...,edx[15]=xmm0[127]
                "test $0xAAAA, %%edx\n\t"//edx& 1010 1010 1010 1010b
                "jnz 2f\n\t"//Not equal to zero means that at least one byte 0x00

                "1:\n\t"  //PREPARE_NEXT_MATCH:
                "sub $0x10, %%eax\n\t"//16 + ecx --> ecx
                "add $0x10, %%ecx\n\t"//eax-16 --> eax
                "cmp $0x10, %%eax\n\t"
                "jge 0b\n\t"//search next 16 bytes

                "2:\n\t"   //DATA_RET:
                "movl %%ecx, %1\n\t"//output ecx->data_ptr
                "movl %%eax, %0\n\t"//output eax->data_remaining
                : "+m"(dataRemaining), "+m"(dataPtr)
                :
                :"eax", "ecx", "edx", "xmm0"
                );
            if (0 >= dataRemaining) {
                break;
            }

        }
        //check the value of each byte
        if ((*dataPtr) >= 2) {

            zeroByteCount = 0;

        }
        else {
            zeroByteCount = singleByteTable[zeroByteCount][*dataPtr];
         }

        dataPtr ++;
        dataRemaining --;
    }

    if ((3 == zeroByteCount) && (dataRemaining > 0)) {

        *nalSize =  bufSize - dataRemaining - *nalOffset - 3;

    } else if (0 == dataRemaining) {

        *nalSize = bufSize - *nalOffset;
    }
    return ENCODE_SUCCESS;
}

Encode_Status VideoEncoderAVC::getHeader(
        uint8_t *inBuffer, uint32_t bufSize, uint32_t *headerSize, uint32_t status) {

    uint32_t nalType = 0;
    uint32_t nalSize = 0;
    uint32_t nalOffset = 0;
    uint32_t size = 0;
    uint8_t *buf = inBuffer;
    Encode_Status ret = ENCODE_SUCCESS;

    *headerSize = 0;
    CHECK_NULL_RETURN_IFFAIL(inBuffer);

    if (bufSize == 0) {
        //bufSize shoule not be 0, error happens
        LOG_E("Buffer size is 0\n");
        return ENCODE_FAIL;
    }

    while (1) {
        nalType = nalSize = nalOffset = 0;
        ret = getOneNALUnit(buf, bufSize, &nalSize, &nalType, &nalOffset, status);
        CHECK_ENCODE_STATUS_RETURN("getOneNALUnit");

        LOG_V("NAL type = %d, NAL size = %d, offset = %d\n", nalType, nalSize, nalOffset);
        size = nalSize + nalOffset;

        // Codec_data should be SPS or PPS
        if (nalType == 7 || nalType == 8) {
            *headerSize += size;
            buf += size;
            bufSize -= size;
        } else {
            LOG_V("No header found or no header anymore\n");
            break;
        }
    }

    return ENCODE_SUCCESS;
}

Encode_Status VideoEncoderAVC::outputCodecData(
        VideoEncOutputBuffer *outBuffer) {

    Encode_Status ret = ENCODE_SUCCESS;
    uint32_t headerSize = 0;

    ret = getHeader((uint8_t *)mCurSegment->buf + mOffsetInSeg,
            mCurSegment->size - mOffsetInSeg, &headerSize, mCurSegment->status);
    CHECK_ENCODE_STATUS_RETURN("getHeader");
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

Encode_Status VideoEncoderAVC::outputOneNALU(
        VideoEncOutputBuffer *outBuffer, bool startCode) {

    uint32_t nalType = 0;
    uint32_t nalSize = 0;
    uint32_t nalOffset = 0;
    uint32_t sizeToBeCopied = 0;

    Encode_Status ret = ENCODE_SUCCESS;
    CHECK_NULL_RETURN_IFFAIL(mCurSegment->buf);

    ret = getOneNALUnit((uint8_t *)mCurSegment->buf + mOffsetInSeg,
            mCurSegment->size - mOffsetInSeg, &nalSize, &nalType, &nalOffset, mCurSegment->status);
    CHECK_ENCODE_STATUS_RETURN("getOneNALUnit");

    // check if we need startcode along with the payload
    if (startCode) {
        sizeToBeCopied = nalSize + nalOffset;
    } else {
        sizeToBeCopied = nalSize;
    }

    if (sizeToBeCopied <= outBuffer->bufferSize) {
        if (startCode) {
            memcpy(outBuffer->data, (uint8_t *)mCurSegment->buf + mOffsetInSeg, sizeToBeCopied);
        } else {
            memcpy(outBuffer->data, (uint8_t *)mCurSegment->buf + mOffsetInSeg + nalOffset,
                   sizeToBeCopied);
        }
        mTotalSizeCopied += sizeToBeCopied;
        mOffsetInSeg += (nalSize + nalOffset);
        outBuffer->dataSize = sizeToBeCopied;
        outBuffer->flag |= ENCODE_BUFFERFLAG_PARTIALFRAME;
        outBuffer->remainingSize = 0;
    } else {
        // if nothing to be copied out, set flag to invalid
        outBuffer->dataSize = 0;
        outBuffer->flag |= ENCODE_BUFFERFLAG_DATAINVALID;
        outBuffer->remainingSize = sizeToBeCopied;
        LOG_W("Buffer size too small\n");
        return ENCODE_BUFFER_TOO_SMALL;
    }

    // check if all data in current segment has been copied out
    if (mCurSegment->size == mOffsetInSeg) {
        if (mCurSegment->next != NULL) {
            mCurSegment = (VACodedBufferSegment *)mCurSegment->next;
            mOffsetInSeg = 0;
        } else {
            LOG_V("End of stream\n");
            outBuffer->flag |= ENCODE_BUFFERFLAG_ENDOFFRAME;
            mCurSegment = NULL;
        }
    }

    return ENCODE_SUCCESS;
}

Encode_Status VideoEncoderAVC::outputLengthPrefixed(VideoEncOutputBuffer *outBuffer) {

    Encode_Status ret = ENCODE_SUCCESS;
    uint32_t nalType = 0;
    uint32_t nalSize = 0;
    uint32_t nalOffset = 0;
    uint32_t sizeCopiedHere = 0;

    CHECK_NULL_RETURN_IFFAIL(mCurSegment->buf);

    while (1) {

        if (mCurSegment->size < mOffsetInSeg || outBuffer->bufferSize < sizeCopiedHere) {
            LOG_E("mCurSegment->size < mOffsetInSeg  || outBuffer->bufferSize < sizeCopiedHere\n");
            return ENCODE_FAIL;
        }

        // we need to handle the whole bitstream NAL by NAL
        ret = getOneNALUnit(
                (uint8_t *)mCurSegment->buf + mOffsetInSeg,
                mCurSegment->size - mOffsetInSeg, &nalSize, &nalType, &nalOffset, mCurSegment->status);
        CHECK_ENCODE_STATUS_RETURN("getOneNALUnit");

        if (nalSize + 4 <= outBuffer->bufferSize - sizeCopiedHere) {
            // write the NAL length to bit stream
            outBuffer->data[sizeCopiedHere] = (nalSize >> 24) & 0xff;
            outBuffer->data[sizeCopiedHere + 1] = (nalSize >> 16) & 0xff;
            outBuffer->data[sizeCopiedHere + 2] = (nalSize >> 8)  & 0xff;
            outBuffer->data[sizeCopiedHere + 3] = nalSize   & 0xff;

            sizeCopiedHere += 4;
            mTotalSizeCopied += 4;

            memcpy(outBuffer->data + sizeCopiedHere,
                   (uint8_t *)mCurSegment->buf + mOffsetInSeg + nalOffset, nalSize);

            sizeCopiedHere += nalSize;
            mTotalSizeCopied += nalSize;
            mOffsetInSeg += (nalSize + nalOffset);

        } else {
            outBuffer->dataSize = sizeCopiedHere;
            // In case the start code is 3-byte length but we use 4-byte for length prefixed
            // so the remainingSize size may larger than the remaining data size
            outBuffer->remainingSize = mTotalSize - mTotalSizeCopied + 100;
            outBuffer->flag |= ENCODE_BUFFERFLAG_PARTIALFRAME;
            LOG_E("Buffer size too small\n");
            return ENCODE_BUFFER_TOO_SMALL;
        }

        // check if all data in current segment has been copied out
        if (mCurSegment->size == mOffsetInSeg) {
            if (mCurSegment->next != NULL) {
                mCurSegment = (VACodedBufferSegment *)mCurSegment->next;
                mOffsetInSeg = 0;
            } else {
                LOG_V("End of stream\n");
                outBuffer->dataSize = sizeCopiedHere;
                outBuffer->remainingSize = 0;
                outBuffer->flag |= ENCODE_BUFFERFLAG_ENDOFFRAME;
                mCurSegment = NULL;
                break;
            }
        }
    }

    return ENCODE_SUCCESS;
}

Encode_Status VideoEncoderAVC::outputNaluLengthsPrefixed(VideoEncOutputBuffer *outBuffer) {

    Encode_Status ret = ENCODE_SUCCESS;
    uint32_t nalType = 0;
    uint32_t nalSize = 0;
    uint32_t nalOffset = 0;
    uint32_t sizeCopiedHere = 0;
    const uint32_t NALUINFO_OFFSET = 256;
    uint32_t nalNum = 0;

    CHECK_NULL_RETURN_IFFAIL(mCurSegment->buf);

    while (1) {

        if (mCurSegment->size < mOffsetInSeg || outBuffer->bufferSize < sizeCopiedHere) {
            LOG_E("mCurSegment->size < mOffsetInSeg  || outBuffer->bufferSize < sizeCopiedHere\n");
            return ENCODE_FAIL;
        }

        // we need to handle the whole bitstream NAL by NAL
        ret = getOneNALUnit(
                (uint8_t *)mCurSegment->buf + mOffsetInSeg,
                mCurSegment->size - mOffsetInSeg, &nalSize, &nalType, &nalOffset, mCurSegment->status);
        CHECK_ENCODE_STATUS_RETURN("getOneNALUnit");

        if (nalSize + 4 <= outBuffer->bufferSize - NALUINFO_OFFSET - sizeCopiedHere) {

            memcpy(outBuffer->data + NALUINFO_OFFSET + sizeCopiedHere,
                   (uint8_t *)mCurSegment->buf + mOffsetInSeg, nalSize + nalOffset);

            sizeCopiedHere += nalSize + nalOffset;
            mTotalSizeCopied += nalSize + nalOffset;
            mOffsetInSeg += (nalSize + nalOffset);

        } else {
            outBuffer->dataSize = sizeCopiedHere;
            // In case the start code is 3-byte length but we use 4-byte for length prefixed
            // so the remainingSize size may larger than the remaining data size
            outBuffer->remainingSize = mTotalSize - mTotalSizeCopied + 100;
            outBuffer->flag |= ENCODE_BUFFERFLAG_PARTIALFRAME;
            LOG_E("Buffer size too small\n");
            return ENCODE_BUFFER_TOO_SMALL;
        }

        nalNum ++;
        uint32_t *nalLength = (uint32_t *) (outBuffer->data + (nalNum+1) * 4);

        *nalLength = nalSize + nalOffset;

        // check if all data in current segment has been copied out
        if (mCurSegment->size == mOffsetInSeg) {
            if (mCurSegment->next != NULL) {
                mCurSegment = (VACodedBufferSegment *)mCurSegment->next;
                mOffsetInSeg = 0;
            } else {
                LOG_V("End of stream\n");
                outBuffer->dataSize = sizeCopiedHere;
                outBuffer->remainingSize = 0;
                outBuffer->flag |= ENCODE_BUFFERFLAG_ENDOFFRAME;
                mCurSegment = NULL;
                break;
            }
        }
    }

    outBuffer->offset = NALUINFO_OFFSET;
    uint32_t *nalHead = (uint32_t *) outBuffer->data;
    *nalHead = 0x4E414C4C; //'nall'
    *(++nalHead) = nalNum;

    return ENCODE_SUCCESS;
}

Encode_Status VideoEncoderAVC::sendEncodeCommand(EncodeTask *task) {
    Encode_Status ret = ENCODE_SUCCESS;

    LOG_V( "Begin\n");

    if (mFrameNum == 0 || mNewHeader) {
        if (mRenderHrd) {
            ret = renderHrd();
            mRenderHrd = false;
            CHECK_ENCODE_STATUS_RETURN("renderHrd");
        }

        mFrameNum = 0;
        ret = renderSequenceParams(task);
        CHECK_ENCODE_STATUS_RETURN("renderSequenceParams");
        if (mNewHeader) {
            mNewHeader = false; //Set to require new header filed to false
            mFrameNum = 0; //reset mFrameNum to 0
            updateFrameInfo(task); //recalculate frame info if mNewHeader is set true after PrepareFrameInfo in encode()
        }
    }

    if (mRenderMaxSliceSize && mVideoParamsAVC.maxSliceSize != 0) {
        ret = renderMaxSliceSize();
        CHECK_ENCODE_STATUS_RETURN("renderMaxSliceSize");
        mRenderMaxSliceSize = false;
    }

    if (mComParams.rcParams.enableIntraFrameQPControl && (task->type == FTYPE_IDR || task->type == FTYPE_I))
        mRenderBitRate = true;

    if (mRenderBitRate) {
        ret = VideoEncoderBase::renderDynamicBitrate(task);
        CHECK_ENCODE_STATUS_RETURN("renderDynamicBitrate");
    }

    if (mRenderAIR &&
        (mComParams.refreshType == VIDEO_ENC_AIR ||
        mComParams.refreshType == VIDEO_ENC_BOTH)) {

        ret = renderAIR();
        CHECK_ENCODE_STATUS_RETURN("renderAIR");

        mRenderAIR = false;
    }
    
    if (mRenderCIR) {

        ret = renderCIR();
        CHECK_ENCODE_STATUS_RETURN("renderCIR");

        mRenderCIR = false;
    }

    if (mRenderFrameRate) {

        ret = VideoEncoderBase::renderDynamicFrameRate();
        CHECK_ENCODE_STATUS_RETURN("renderDynamicFrameRate");

        mRenderFrameRate = false;
    }

    ret = renderPictureParams(task);
    CHECK_ENCODE_STATUS_RETURN("renderPictureParams");

    if (mFrameNum == 0 && (mEncPackedHeaders != VA_ATTRIB_NOT_SUPPORTED)) {
        ret = renderPackedSequenceParams(task);
        CHECK_ENCODE_STATUS_RETURN("renderPackedSequenceParams");

        ret = renderPackedPictureParams(task);
        CHECK_ENCODE_STATUS_RETURN("renderPackedPictureParams");
    }

    ret = renderSliceParams(task);
    CHECK_ENCODE_STATUS_RETURN("renderSliceParams");

    LOG_V( "End\n");
    return ENCODE_SUCCESS;
}


Encode_Status VideoEncoderAVC::renderMaxSliceSize() {

    VAStatus vaStatus = VA_STATUS_SUCCESS;
    LOG_V( "Begin\n\n");

    if (mComParams.rcMode != RATE_CONTROL_VCM) {
        LOG_W ("Not in VCM mode, but call send_max_slice_size\n");
        return ENCODE_SUCCESS;
    }

    VAEncMiscParameterBuffer *miscEncParamBuf;
    VAEncMiscParameterMaxSliceSize *maxSliceSizeParam;
    VABufferID miscParamBufferID;

    vaStatus = vaCreateBuffer(
            mVADisplay, mVAContext,
            VAEncMiscParameterBufferType,
            sizeof(VAEncMiscParameterBuffer) + sizeof(VAEncMiscParameterMaxSliceSize),
            1, NULL, &miscParamBufferID);
    CHECK_VA_STATUS_RETURN("vaCreateBuffer");

    vaStatus = vaMapBuffer(mVADisplay, miscParamBufferID, (void **)&miscEncParamBuf);
    CHECK_VA_STATUS_RETURN("vaMapBuffer");

    miscEncParamBuf->type = VAEncMiscParameterTypeMaxSliceSize;
    maxSliceSizeParam = (VAEncMiscParameterMaxSliceSize *)miscEncParamBuf->data;

    maxSliceSizeParam->max_slice_size = mVideoParamsAVC.maxSliceSize;

    vaStatus = vaUnmapBuffer(mVADisplay, miscParamBufferID);
    CHECK_VA_STATUS_RETURN("vaUnmapBuffer");

    LOG_I( "max slice size = %d\n", maxSliceSizeParam->max_slice_size);

    vaStatus = vaRenderPicture(mVADisplay, mVAContext, &miscParamBufferID, 1);
    CHECK_VA_STATUS_RETURN("vaRenderPicture");

    return ENCODE_SUCCESS;
}

Encode_Status VideoEncoderAVC::renderCIR(){
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    LOG_V( "%s Begin\n", __FUNCTION__);

    VABufferID miscParamBufferCIRid;
    VAEncMiscParameterBuffer *misc_param;
    VAEncMiscParameterCIR *misc_cir_param;

    vaStatus = vaCreateBuffer(mVADisplay, mVAContext,
            VAEncMiscParameterBufferType,
            sizeof(VAEncMiscParameterBuffer) + sizeof(VAEncMiscParameterCIR),
            1,
            NULL,
            &miscParamBufferCIRid);
    CHECK_VA_STATUS_RETURN("vaCreateBuffer");

    vaStatus = vaMapBuffer(mVADisplay, miscParamBufferCIRid,  (void **)&misc_param);
    CHECK_VA_STATUS_RETURN("vaMapBuffer");

    misc_param->type = VAEncMiscParameterTypeCIR;
    misc_cir_param = (VAEncMiscParameterCIR *)misc_param->data;
    misc_cir_param->cir_num_mbs = mComParams.cirParams.cir_num_mbs;
    LOG_I( "cir_num_mbs %d \n", misc_cir_param->cir_num_mbs);

    vaUnmapBuffer(mVADisplay, miscParamBufferCIRid);
    CHECK_VA_STATUS_RETURN("vaUnmapBuffer");

    vaStatus = vaRenderPicture(mVADisplay, mVAContext, &miscParamBufferCIRid, 1);
    CHECK_VA_STATUS_RETURN("vaRenderPicture");

    return ENCODE_SUCCESS;
}

Encode_Status VideoEncoderAVC::renderAIR() {
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    LOG_V( "Begin\n\n");

    VAEncMiscParameterBuffer   *miscEncParamBuf;
    VAEncMiscParameterAIR *airParams;
    VABufferID miscParamBufferID;

    vaStatus = vaCreateBuffer(
            mVADisplay, mVAContext,
            VAEncMiscParameterBufferType,
            sizeof(miscEncParamBuf) + sizeof(VAEncMiscParameterAIR),
            1, NULL, &miscParamBufferID);
    CHECK_VA_STATUS_RETURN("vaCreateBuffer");

    vaStatus = vaMapBuffer(mVADisplay, miscParamBufferID, (void **)&miscEncParamBuf);
    CHECK_VA_STATUS_RETURN("vaMapBuffer");

    miscEncParamBuf->type = VAEncMiscParameterTypeAIR;
    airParams = (VAEncMiscParameterAIR *)miscEncParamBuf->data;

    airParams->air_num_mbs = mComParams.airParams.airMBs;
    airParams->air_threshold= mComParams.airParams.airThreshold;
    airParams->air_auto = mComParams.airParams.airAuto;

    vaStatus = vaUnmapBuffer(mVADisplay, miscParamBufferID);
    CHECK_VA_STATUS_RETURN("vaUnmapBuffer");

    vaStatus = vaRenderPicture(mVADisplay, mVAContext, &miscParamBufferID, 1);
    CHECK_VA_STATUS_RETURN("vaRenderPicture");

    LOG_I( "airThreshold = %d\n", airParams->air_threshold);
    return ENCODE_SUCCESS;
}

int VideoEncoderAVC::calcLevel(int numMbs) {
    int level = 30;

    if (numMbs < 1620) {
        level = 30;
    } else if (numMbs < 3600) {
        level = 31;
    } else if (numMbs < 5120) {
        level = 32;
    } else if (numMbs < 8192) {
        level = 41;
    } else if (numMbs < 8704) {
        level = 42;
    } else if (numMbs < 22080) {
        level = 50;
    } else if (numMbs < 36864) {
        level = 51;
    } else {
        LOG_W("No such level can support that resolution");
        level = 51;
    }
    return level;
}

Encode_Status VideoEncoderAVC::renderSequenceParams(EncodeTask *) {

    VAStatus vaStatus = VA_STATUS_SUCCESS;
    VAEncSequenceParameterBufferH264 avcSeqParams = VAEncSequenceParameterBufferH264();
    VAEncMiscParameterBuffer   *miscEncRCParamBuf;
    VAEncMiscParameterBuffer   *miscEncFrameRateParamBuf;
    VAEncMiscParameterRateControl *rcMiscParam;
    VAEncMiscParameterFrameRate *framerateParam;
    int level;
    uint32_t frameRateNum = mComParams.frameRate.frameRateNum;
    uint32_t frameRateDenom = mComParams.frameRate.frameRateDenom;

    LOG_V( "Begin\n\n");
    vaStatus = vaCreateBuffer(mVADisplay, mVAContext,
            VAEncMiscParameterBufferType,
            sizeof (VAEncMiscParameterBuffer) + sizeof (VAEncMiscParameterRateControl),
            1, NULL,
            &mRcParamBuf);
    CHECK_VA_STATUS_RETURN("vaCreateBuffer");
    vaStatus = vaMapBuffer(mVADisplay, mRcParamBuf, (void **)&miscEncRCParamBuf);
    CHECK_VA_STATUS_RETURN("vaMapBuffer");

    vaStatus = vaCreateBuffer(mVADisplay, mVAContext,
            VAEncMiscParameterBufferType,
            sizeof (VAEncMiscParameterBuffer) + sizeof (VAEncMiscParameterFrameRate),
            1, NULL,
            &mFrameRateParamBuf);
    CHECK_VA_STATUS_RETURN("vaCreateBuffer");
    vaStatus = vaMapBuffer(mVADisplay, mFrameRateParamBuf, (void **)&miscEncFrameRateParamBuf);
    CHECK_VA_STATUS_RETURN("vaMapBuffer");

    miscEncRCParamBuf->type = VAEncMiscParameterTypeRateControl;
    rcMiscParam = (VAEncMiscParameterRateControl  *)miscEncRCParamBuf->data;
    miscEncFrameRateParamBuf->type = VAEncMiscParameterTypeFrameRate;
    framerateParam = (VAEncMiscParameterFrameRate *)miscEncFrameRateParamBuf->data;
    // set up the sequence params for HW
    // avcSeqParams.level_idc = mLevel;
    avcSeqParams.intra_period = mComParams.intraPeriod;
    avcSeqParams.intra_idr_period = mVideoParamsAVC.idrInterval;
    avcSeqParams.ip_period = mVideoParamsAVC.ipPeriod;
    avcSeqParams.picture_width_in_mbs = (mComParams.resolution.width + 15) / 16;
    avcSeqParams.picture_height_in_mbs = (mComParams.resolution.height + 15) / 16;

    level = calcLevel (avcSeqParams.picture_width_in_mbs * avcSeqParams.picture_height_in_mbs);
    avcSeqParams.level_idc = level;
    avcSeqParams.bits_per_second = mComParams.rcParams.bitRate;
    framerateParam->framerate =
            (unsigned int) (frameRateNum + frameRateDenom /2 ) / frameRateDenom;
    rcMiscParam->initial_qp = mComParams.rcParams.initQP;
    rcMiscParam->min_qp = mComParams.rcParams.minQP;
    rcMiscParam->max_qp = mComParams.rcParams.maxQP;
    if (mComParams.rcParams.enableIntraFrameQPControl) {
        rcMiscParam->min_qp = mComParams.rcParams.I_minQP;
        rcMiscParam->max_qp = mComParams.rcParams.I_maxQP;
    }
    rcMiscParam->window_size = mComParams.rcParams.windowSize;
    //target bitrate is sent to libva through Sequence Parameter Buffer
    rcMiscParam->bits_per_second = 0;
    rcMiscParam->basic_unit_size = mVideoParamsAVC.basicUnitSize; //for rate control usage
    avcSeqParams.intra_period = mComParams.intraPeriod;
    //avcSeqParams.vui_flag = 248;
    avcSeqParams.vui_parameters_present_flag = mVideoParamsAVC.VUIFlag;
    avcSeqParams.num_units_in_tick = frameRateDenom;
    avcSeqParams.time_scale = 2 * frameRateNum;
    avcSeqParams.seq_parameter_set_id = 0;
    if (mVideoParamsAVC.crop.LeftOffset ||
            mVideoParamsAVC.crop.RightOffset ||
            mVideoParamsAVC.crop.TopOffset ||
            mVideoParamsAVC.crop.BottomOffset) {
        avcSeqParams.frame_cropping_flag = true;
        avcSeqParams.frame_crop_left_offset = mVideoParamsAVC.crop.LeftOffset;
        avcSeqParams.frame_crop_right_offset = mVideoParamsAVC.crop.RightOffset;
        avcSeqParams.frame_crop_top_offset = mVideoParamsAVC.crop.TopOffset;
        avcSeqParams.frame_crop_bottom_offset = mVideoParamsAVC.crop.BottomOffset;
    } else {
        avcSeqParams.frame_cropping_flag = false;

        if (mComParams.resolution.width & 0xf) {
            avcSeqParams.frame_cropping_flag = true;
            uint32_t AWidth = (mComParams.resolution.width + 0xf) & (~0xf);
            avcSeqParams.frame_crop_right_offset = ( AWidth - mComParams.resolution.width ) / 2;
        }

        if (mComParams.resolution.height & 0xf) {
            avcSeqParams.frame_cropping_flag = true;
            uint32_t AHeight = (mComParams.resolution.height + 0xf) & (~0xf);
            avcSeqParams.frame_crop_bottom_offset = ( AHeight - mComParams.resolution.height ) / 2;
        }
    }

    if(avcSeqParams.vui_parameters_present_flag && (mVideoParamsAVC.SAR.SarWidth || mVideoParamsAVC.SAR.SarHeight)) {
        avcSeqParams.vui_fields.bits.aspect_ratio_info_present_flag = true;
        avcSeqParams.aspect_ratio_idc = 0xff /* Extended_SAR */;
        avcSeqParams.sar_width = mVideoParamsAVC.SAR.SarWidth;
        avcSeqParams.sar_height = mVideoParamsAVC.SAR.SarHeight;
    }

    avcSeqParams.max_num_ref_frames = 1;

    if(avcSeqParams.ip_period > 1)
        avcSeqParams.max_num_ref_frames = 2;

    LOG_V("===h264 sequence params===\n");
    LOG_V( "seq_parameter_set_id = %d\n", (uint32_t)avcSeqParams.seq_parameter_set_id);
    LOG_V( "level_idc = %d\n", (uint32_t)avcSeqParams.level_idc);
    LOG_V( "intra_period = %d\n", avcSeqParams.intra_period);
    LOG_V( "idr_interval = %d\n", avcSeqParams.intra_idr_period);
    LOG_V( "picture_width_in_mbs = %d\n", avcSeqParams.picture_width_in_mbs);
    LOG_V( "picture_height_in_mbs = %d\n", avcSeqParams.picture_height_in_mbs);
    LOG_V( "bitrate = %d\n", rcMiscParam->bits_per_second);
    LOG_V( "frame_rate = %d\n", framerateParam->framerate);
    LOG_V( "initial_qp = %d\n", rcMiscParam->initial_qp);
    LOG_V( "min_qp = %d\n", rcMiscParam->min_qp);
    LOG_V( "basic_unit_size = %d\n", rcMiscParam->basic_unit_size);
    LOG_V( "bDirect8x8Inference = %d\n",mVideoParamsAVC.bDirect8x8Inference);

    // Not sure whether these settings work for all drivers
    avcSeqParams.seq_fields.bits.frame_mbs_only_flag = 1;
    avcSeqParams.seq_fields.bits.pic_order_cnt_type = 0;
    avcSeqParams.seq_fields.bits.direct_8x8_inference_flag = mVideoParamsAVC.bDirect8x8Inference;

    avcSeqParams.seq_fields.bits.log2_max_frame_num_minus4 = 0;
    avcSeqParams.seq_fields.bits.log2_max_pic_order_cnt_lsb_minus4 = 2;
//    avcSeqParams.time_scale = 900;
//    avcSeqParams.num_units_in_tick = 15;			/* Tc = num_units_in_tick / time_sacle */
    // Not sure whether these settings work for all drivers

    vaStatus = vaUnmapBuffer(mVADisplay, mRcParamBuf);
    CHECK_VA_STATUS_RETURN("vaUnmapBuffer");
    vaStatus = vaUnmapBuffer(mVADisplay, mFrameRateParamBuf);
    CHECK_VA_STATUS_RETURN("vaUnmapBuffer");
    vaStatus = vaCreateBuffer(
            mVADisplay, mVAContext,
            VAEncSequenceParameterBufferType,
            sizeof(avcSeqParams), 1, &avcSeqParams,
            &mSeqParamBuf);
    CHECK_VA_STATUS_RETURN("vaCreateBuffer");
    vaStatus = vaRenderPicture(mVADisplay, mVAContext, &mFrameRateParamBuf, 1);
    CHECK_VA_STATUS_RETURN("vaRenderPicture");
    vaStatus = vaRenderPicture(mVADisplay, mVAContext, &mSeqParamBuf, 1);
    CHECK_VA_STATUS_RETURN("vaRenderPicture");
    vaStatus = vaRenderPicture(mVADisplay, mVAContext, &mRcParamBuf, 1);
    CHECK_VA_STATUS_RETURN("vaRenderPicture");

    return ENCODE_SUCCESS;
}

Encode_Status VideoEncoderAVC::renderPackedSequenceParams(EncodeTask *) {

    VAStatus vaStatus = VA_STATUS_SUCCESS;
    VAEncSequenceParameterBufferH264 *avcSeqParams;
    VAEncPackedHeaderParameterBuffer packed_header_param_buffer;
    unsigned char *packed_seq_buffer = NULL;
    unsigned int length_in_bits, offset_in_bytes;

    LOG_V("Begin\n");

    vaStatus = vaMapBuffer(mVADisplay, mSeqParamBuf, (void **)&avcSeqParams);
    CHECK_VA_STATUS_RETURN("vaMapBuffer");

    length_in_bits = build_packed_seq_buffer(&packed_seq_buffer, mComParams.profile, avcSeqParams);
    packed_header_param_buffer.type = VAEncPackedHeaderSequence;
    packed_header_param_buffer.bit_length = length_in_bits;
    packed_header_param_buffer.has_emulation_bytes = 0;
    vaStatus = vaCreateBuffer(mVADisplay, mVAContext,
            VAEncPackedHeaderParameterBufferType,
            sizeof(packed_header_param_buffer), 1, &packed_header_param_buffer,
            &packed_seq_header_param_buf_id);
    CHECK_VA_STATUS_RETURN("vaCreateBuffer");

    vaStatus = vaCreateBuffer(mVADisplay, mVAContext,
            VAEncPackedHeaderDataBufferType,
            (length_in_bits + 7) / 8, 1, packed_seq_buffer,
            &packed_seq_buf_id);
    CHECK_VA_STATUS_RETURN("vaCreateBuffer");

    vaStatus = vaRenderPicture(mVADisplay, mVAContext, &packed_seq_header_param_buf_id, 1);
    CHECK_VA_STATUS_RETURN("vaRenderPicture");

    vaStatus = vaRenderPicture(mVADisplay, mVAContext, &packed_seq_buf_id, 1);
    CHECK_VA_STATUS_RETURN("vaRenderPicture");

    vaStatus = vaUnmapBuffer(mVADisplay, mSeqParamBuf);
    CHECK_VA_STATUS_RETURN("vaUnmapBuffer");

    free(packed_seq_buffer);

    LOG_V("End\n");

    return vaStatus;
}

Encode_Status VideoEncoderAVC::renderPictureParams(EncodeTask *task) {

    VAStatus vaStatus = VA_STATUS_SUCCESS;
    VAEncPictureParameterBufferH264 avcPicParams = VAEncPictureParameterBufferH264();
    uint32_t RefFrmIdx;

    LOG_V( "Begin\n\n");
    // set picture params for HW
    if (mAutoReference == false) {
        for (RefFrmIdx = 0; RefFrmIdx < 16; RefFrmIdx++) {
            avcPicParams.ReferenceFrames[RefFrmIdx].picture_id = VA_INVALID_ID;
            avcPicParams.ReferenceFrames[RefFrmIdx].flags = VA_PICTURE_H264_INVALID;
        }
        avcPicParams.ReferenceFrames[0].picture_id= task->ref_surface;
        avcPicParams.ReferenceFrames[0].flags = VA_PICTURE_H264_SHORT_TERM_REFERENCE;
        avcPicParams.CurrPic.picture_id= task->rec_surface;
        // Not sure whether these settings work for all drivers
        avcPicParams.CurrPic.TopFieldOrderCnt = mFrameNum * 2;

        avcPicParams.pic_fields.bits.transform_8x8_mode_flag = 0;
        avcPicParams.seq_parameter_set_id = 0;
        avcPicParams.pic_parameter_set_id = 0;

        avcPicParams.last_picture = 0;
        avcPicParams.frame_num = 0;

        avcPicParams.pic_init_qp = 26;
        avcPicParams.num_ref_idx_l0_active_minus1 = 0;
        avcPicParams.num_ref_idx_l1_active_minus1 = 0;

        avcPicParams.pic_fields.bits.idr_pic_flag = 0;
        avcPicParams.pic_fields.bits.reference_pic_flag = 0;
        avcPicParams.pic_fields.bits.entropy_coding_mode_flag = 0;
        avcPicParams.pic_fields.bits.weighted_pred_flag = 0;
        avcPicParams.pic_fields.bits.weighted_bipred_idc = 0;
        avcPicParams.pic_fields.bits.transform_8x8_mode_flag = 0;
        avcPicParams.pic_fields.bits.deblocking_filter_control_present_flag = 1;

        avcPicParams.frame_num = mFrameNum;
        avcPicParams.pic_fields.bits.reference_pic_flag = 1;
        // Not sure whether these settings work for all drivers
    }else {
        avcPicParams.CurrPic.picture_id= VA_INVALID_SURFACE;
        for(uint32_t i =0; i< mAutoReferenceSurfaceNum; i++)
            avcPicParams.ReferenceFrames[i].picture_id = mAutoRefSurfaces[i];
    }

    avcPicParams.pic_fields.bits.idr_pic_flag = (mFrameNum == 0);
    avcPicParams.pic_fields.bits.entropy_coding_mode_flag = mVideoParamsAVC.bEntropyCodingCABAC;
    avcPicParams.coded_buf = task->coded_buffer;
    avcPicParams.last_picture = 0;

    LOG_V("======h264 picture params======\n");
    LOG_V( "reference_picture = 0x%08x\n", avcPicParams.ReferenceFrames[0].picture_id);
    LOG_V( "reconstructed_picture = 0x%08x\n", avcPicParams.CurrPic.picture_id);
    LOG_V( "coded_buf = 0x%08x\n", avcPicParams.coded_buf);
    //LOG_I( "picture_width = %d\n", avcPicParams.picture_width);
    //LOG_I( "picture_height = %d\n\n", avcPicParams.picture_height);

    vaStatus = vaCreateBuffer(
            mVADisplay, mVAContext,
            VAEncPictureParameterBufferType,
            sizeof(avcPicParams),
            1,&avcPicParams,
            &mPicParamBuf);
    CHECK_VA_STATUS_RETURN("vaCreateBuffer");

    vaStatus = vaRenderPicture(mVADisplay, mVAContext, &mPicParamBuf, 1);
    CHECK_VA_STATUS_RETURN("vaRenderPicture");

    LOG_V( "end\n");
    return ENCODE_SUCCESS;
}

Encode_Status VideoEncoderAVC::renderPackedPictureParams(EncodeTask *) {

    VAStatus vaStatus = VA_STATUS_SUCCESS;
    VAEncPictureParameterBufferH264 *avcPicParams;
    VAEncPackedHeaderParameterBuffer packed_header_param_buffer;
    unsigned char *packed_pic_buffer = NULL;
    unsigned int length_in_bits, offset_in_bytes;

    LOG_V("Begin\n");

    vaStatus = vaMapBuffer(mVADisplay, mPicParamBuf, (void **)&avcPicParams);
    CHECK_VA_STATUS_RETURN("vaMapBuffer");

    length_in_bits = build_packed_pic_buffer(&packed_pic_buffer, avcPicParams);
    packed_header_param_buffer.type = VAEncPackedHeaderPicture;
    packed_header_param_buffer.bit_length = length_in_bits;
    packed_header_param_buffer.has_emulation_bytes = 0;
    vaStatus = vaCreateBuffer(mVADisplay, mVAContext,
            VAEncPackedHeaderParameterBufferType,
            sizeof(packed_header_param_buffer), 1, &packed_header_param_buffer,
            &packed_pic_header_param_buf_id);
    CHECK_VA_STATUS_RETURN("vaCreateBuffer");

    vaStatus = vaCreateBuffer(mVADisplay, mVAContext,
            VAEncPackedHeaderDataBufferType,
            (length_in_bits + 7) / 8, 1, packed_pic_buffer,
            &packed_pic_buf_id);
    CHECK_VA_STATUS_RETURN("vaCreateBuffer");

    vaStatus = vaRenderPicture(mVADisplay, mVAContext, &packed_pic_header_param_buf_id, 1);
    CHECK_VA_STATUS_RETURN("vaRenderPicture");

    vaStatus = vaRenderPicture(mVADisplay, mVAContext, &packed_pic_buf_id, 1);
    CHECK_VA_STATUS_RETURN("vaRenderPicture");

    vaStatus = vaUnmapBuffer(mVADisplay, mSeqParamBuf);
    CHECK_VA_STATUS_RETURN("vaUnmapBuffer");

    free(packed_pic_buffer);

    LOG_V("End\n");

    return vaStatus;
}

Encode_Status VideoEncoderAVC::renderSliceParams(EncodeTask *task) {

    VAStatus vaStatus = VA_STATUS_SUCCESS;

    uint32_t sliceNum = 0;
    uint32_t sliceIndex = 0;
    uint32_t sliceHeightInMB = 0;
    uint32_t maxSliceNum = 0;
    uint32_t minSliceNum = 0;
    uint32_t actualSliceHeightInMB = 0;
    uint32_t startRowInMB = 0;
    uint32_t modulus = 0;
    uint32_t RefFrmIdx;

    LOG_V( "Begin\n\n");

    maxSliceNum = (mComParams.resolution.height + 15) / 16;
    minSliceNum = 1;

    if (task->type == FTYPE_I || task->type == FTYPE_IDR) {
        sliceNum = mVideoParamsAVC.sliceNum.iSliceNum;
    } else {
        sliceNum = mVideoParamsAVC.sliceNum.pSliceNum;
    }

    if (sliceNum < minSliceNum) {
        LOG_W("Slice Number is too small");
        sliceNum = minSliceNum;
    }

    if (sliceNum > maxSliceNum) {
        LOG_W("Slice Number is too big");
        sliceNum = maxSliceNum;
    }

    mSliceNum= sliceNum;
    modulus = maxSliceNum % sliceNum;
    sliceHeightInMB = (maxSliceNum - modulus) / sliceNum ;

    vaStatus = vaCreateBuffer(
            mVADisplay, mVAContext,
            VAEncSliceParameterBufferType,
            sizeof(VAEncSliceParameterBufferH264),
            sliceNum, NULL,
            &mSliceParamBuf);
    CHECK_VA_STATUS_RETURN("vaCreateBuffer");

    VAEncSliceParameterBufferH264 *sliceParams, *currentSlice;

    vaStatus = vaMapBuffer(mVADisplay, mSliceParamBuf, (void **)&sliceParams);
    CHECK_VA_STATUS_RETURN("vaMapBuffer");
    if(!sliceParams)
        return ENCODE_NULL_PTR;
    memset(sliceParams, 0 , sizeof(VAEncSliceParameterBufferH264));
    if(!sliceParams)
        return ENCODE_NULL_PTR;

    currentSlice = sliceParams;
    startRowInMB = 0;
    for (sliceIndex = 0; sliceIndex < sliceNum; sliceIndex++) {
        currentSlice = sliceParams + sliceIndex;
        actualSliceHeightInMB = sliceHeightInMB;
        if (sliceIndex < modulus) {
            actualSliceHeightInMB ++;
        }

        // starting MB row number for this slice, suppose macroblock 16x16
        currentSlice->macroblock_address = startRowInMB * ((mComParams.resolution.width + 0xf) & ~0xf) / 16;
        // slice height measured in MB
        currentSlice->num_macroblocks = actualSliceHeightInMB * ((mComParams.resolution.width + 0xf) & ~0xf) / 16;
        if(task->type == FTYPE_I||task->type == FTYPE_IDR)
            currentSlice->slice_type = 2;
        else if(task->type == FTYPE_P)
            currentSlice->slice_type = 0;
        else if(task->type == FTYPE_B)
            currentSlice->slice_type = 1;
        currentSlice->disable_deblocking_filter_idc = mComParams.disableDeblocking;

        // This is a temporary fix suggested by Binglin for bad encoding quality issue
        // TODO: We need a long term design for this field
        //currentSlice->slice_flags.bits.uses_long_term_ref = 0;
        //currentSlice->slice_flags.bits.is_long_term_ref = 0;

        LOG_V("======AVC slice params======\n");
        LOG_V( "slice_index = %d\n", (int) sliceIndex);
        LOG_V( "macroblock_address = %d\n", (int) currentSlice->macroblock_address);
        LOG_V( "slice_height_in_mb = %d\n", (int) currentSlice->num_macroblocks);
        LOG_V( "slice.type = %d\n", (int) currentSlice->slice_type);
        LOG_V("disable_deblocking_filter_idc = %d\n\n", (int) currentSlice->disable_deblocking_filter_idc);

        // Not sure whether these settings work for all drivers
        currentSlice->pic_parameter_set_id = 0;
        currentSlice->pic_order_cnt_lsb = mFrameNum * 2;
        currentSlice->direct_spatial_mv_pred_flag = 0;
        currentSlice->num_ref_idx_l0_active_minus1 = 0;      /* FIXME: ??? */
        currentSlice->num_ref_idx_l1_active_minus1 = 0;
        currentSlice->cabac_init_idc = 0;
        currentSlice->slice_qp_delta = 0;
        currentSlice->disable_deblocking_filter_idc = 0;
        currentSlice->slice_alpha_c0_offset_div2 = 2;
        currentSlice->slice_beta_offset_div2 = 2;
        currentSlice->idr_pic_id = 0;
        for (RefFrmIdx = 0; RefFrmIdx < 32; RefFrmIdx++) {
            currentSlice->RefPicList0[RefFrmIdx].picture_id = VA_INVALID_ID;
            currentSlice->RefPicList0[RefFrmIdx].flags = VA_PICTURE_H264_INVALID;
        }
        currentSlice->RefPicList0[0].picture_id = task->ref_surface;
        currentSlice->RefPicList0[0].flags = VA_PICTURE_H264_SHORT_TERM_REFERENCE;
        // Not sure whether these settings work for all drivers

        startRowInMB += actualSliceHeightInMB;
    }

    vaStatus = vaUnmapBuffer(mVADisplay, mSliceParamBuf);
    CHECK_VA_STATUS_RETURN("vaUnmapBuffer");

    vaStatus = vaRenderPicture(mVADisplay, mVAContext, &mSliceParamBuf, 1);
    CHECK_VA_STATUS_RETURN("vaRenderPicture");
    LOG_V( "end\n");
    return ENCODE_SUCCESS;
}
