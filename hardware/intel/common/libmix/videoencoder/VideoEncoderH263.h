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

#ifndef __VIDEO_ENCODER_H263_H__
#define __VIDEO_ENCODER_H263_H__

#include "VideoEncoderBase.h"

/**
  * H.263 Encoder class, derived from VideoEncoderBase
  */
class VideoEncoderH263: public VideoEncoderBase {
public:
    VideoEncoderH263();
    virtual ~VideoEncoderH263() {};

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
    virtual Encode_Status getExtFormatOutput(VideoEncOutputBuffer *) {
        return ENCODE_NOT_SUPPORTED;
    }
    //virtual Encode_Status updateFrameInfo(EncodeTask* task);

    // Local Methods
private:
    Encode_Status renderSequenceParams(EncodeTask *task);
    Encode_Status renderPictureParams(EncodeTask *task);
    Encode_Status renderSliceParams(EncodeTask *task);
};

#endif /* __VIDEO_ENCODER_H263_H__ */

