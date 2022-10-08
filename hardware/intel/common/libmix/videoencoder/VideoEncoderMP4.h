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

#ifndef __VIDEO_ENCODER__MPEG4_H__
#define __VIDEO_ENCODER__MPEG4_H__

#include "VideoEncoderBase.h"

/**
  * MPEG-4:2 Encoder class, derived from VideoEncoderBase
  */
class VideoEncoderMP4: public VideoEncoderBase {
public:
    VideoEncoderMP4();
    virtual ~VideoEncoderMP4() {};

//    Encode_Status getOutput(VideoEncOutputBuffer *outBuffer);

protected:
    virtual Encode_Status sendEncodeCommand(EncodeTask *task);
    virtual Encode_Status derivedSetParams(VideoParamConfigSet *) {
        return ENCODE_SUCCESS;
    }
    virtual Encode_Status derivedGetParams(VideoParamConfigSet *) {
        return ENCODE_SUCCESS;
    }
    virtual Encode_Status derivedGetConfig(VideoParamConfigSet *) {
        return ENCODE_SUCCESS;
    }
    virtual Encode_Status derivedSetConfig(VideoParamConfigSet *) {
        return ENCODE_SUCCESS;
    }
    virtual Encode_Status getExtFormatOutput(VideoEncOutputBuffer *outBuffer);
    //virtual Encode_Status updateFrameInfo(EncodeTask* task);

    // Local Methods
private:
    Encode_Status getHeaderPos(uint8_t *inBuffer, uint32_t bufSize, uint32_t *headerSize);
    Encode_Status outputConfigData(VideoEncOutputBuffer *outBuffer);
    Encode_Status renderSequenceParams(EncodeTask *task);
    Encode_Status renderPictureParams(EncodeTask *task);
    Encode_Status renderSliceParams(EncodeTask *task);

    unsigned char mProfileLevelIndication;
    uint32_t mFixedVOPTimeIncrement;
};

#endif /* __VIDEO_ENCODER__MPEG4_H__ */
