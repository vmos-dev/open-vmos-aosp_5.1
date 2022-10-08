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

#ifndef __VIDEO_ENCODER_VP8_H__
#define __VIDEO_ENCODER_VP8_H__

#include "VideoEncoderBase.h"

/**
  * VP8 Encoder class, derived from VideoEncoderBase
  */
class VideoEncoderVP8: public VideoEncoderBase {
public:
    VideoEncoderVP8();
    virtual ~VideoEncoderVP8();
    virtual Encode_Status start();



protected:
    virtual Encode_Status sendEncodeCommand(EncodeTask *task);
    virtual Encode_Status derivedSetParams(VideoParamConfigSet *videoEncParams);
    virtual Encode_Status derivedGetParams(VideoParamConfigSet *videoEncParams);
    virtual Encode_Status derivedGetConfig(VideoParamConfigSet *videoEncConfig);
    virtual Encode_Status derivedSetConfig(VideoParamConfigSet *videoEncConfig);
    virtual Encode_Status getExtFormatOutput(VideoEncOutputBuffer *) {
        return ENCODE_NOT_SUPPORTED;
    }

    // Local Methods
private:
        Encode_Status renderSequenceParams();
        Encode_Status renderPictureParams(EncodeTask *task);
        Encode_Status renderRCParams(uint32_t layer_id, bool total_bitrate);
        Encode_Status renderHRDParams(void);
        Encode_Status renderFrameRateParams(uint32_t layer_id, bool total_framerate);
        Encode_Status renderMaxFrameSizeParams(void);
        Encode_Status renderLayerStructureParam(void);

        VideoConfigVP8 mVideoConfigVP8;
        VideoParamsVP8 mVideoParamsVP8;
        VideoConfigVP8ReferenceFrame mVideoConfigVP8ReferenceFrame;
};

#endif /* __VIDEO_ENCODER_VP8_H__ */
