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

#ifndef __VIDEO_ENCODER_AVC_H__
#define __VIDEO_ENCODER_AVC_H__

#include "VideoEncoderBase.h"

class VideoEncoderAVC : public VideoEncoderBase {

public:
    VideoEncoderAVC();
    ~VideoEncoderAVC() {};

    virtual Encode_Status start();

    virtual Encode_Status derivedSetParams(VideoParamConfigSet *videoEncParams);
    virtual Encode_Status derivedGetParams(VideoParamConfigSet *videoEncParams);
    virtual Encode_Status derivedGetConfig(VideoParamConfigSet *videoEncConfig);
    virtual Encode_Status derivedSetConfig(VideoParamConfigSet *videoEncConfig);

protected:

    virtual Encode_Status sendEncodeCommand(EncodeTask *task);
    virtual Encode_Status getExtFormatOutput(VideoEncOutputBuffer *outBuffer);
    virtual Encode_Status updateFrameInfo(EncodeTask* task);
private:
    // Local Methods

    Encode_Status getOneNALUnit(uint8_t *inBuffer, uint32_t bufSize, uint32_t *nalSize, uint32_t *nalType, uint32_t *nalOffset, uint32_t status);
    Encode_Status getHeader(uint8_t *inBuffer, uint32_t bufSize, uint32_t *headerSize, uint32_t status);
    Encode_Status outputCodecData(VideoEncOutputBuffer *outBuffer);
    Encode_Status outputOneNALU(VideoEncOutputBuffer *outBuffer, bool startCode);
    Encode_Status outputLengthPrefixed(VideoEncOutputBuffer *outBuffer);
    Encode_Status outputNaluLengthsPrefixed(VideoEncOutputBuffer *outBuffer);

    Encode_Status renderMaxSliceSize();
    Encode_Status renderAIR();
    Encode_Status renderCIR();
    Encode_Status renderSequenceParams(EncodeTask *task);
    Encode_Status renderPictureParams(EncodeTask *task);
    Encode_Status renderSliceParams(EncodeTask *task);
    int calcLevel(int numMbs);
    Encode_Status renderPackedSequenceParams(EncodeTask *task);
    Encode_Status renderPackedPictureParams(EncodeTask *task);

public:

    VideoParamsAVC mVideoParamsAVC;
    uint32_t mSliceNum;
    VABufferID packed_seq_header_param_buf_id;
    VABufferID packed_seq_buf_id;
    VABufferID packed_pic_header_param_buf_id;
    VABufferID packed_pic_buf_id;
    VABufferID packed_sei_header_param_buf_id;   /* the SEI buffer */
    VABufferID packed_sei_buf_id;

};

#endif /* __VIDEO_ENCODER_AVC_H__ */
