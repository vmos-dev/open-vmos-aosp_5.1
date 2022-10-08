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

#ifndef VIDEO_ENCODER_INTERFACE_H_
#define VIDEO_ENCODER_INTERFACE_H_

#include "VideoEncoderDef.h"

class IVideoEncoder {
public:
    virtual ~IVideoEncoder() {};
    virtual Encode_Status start(void) = 0;
    virtual Encode_Status stop(void) = 0;
    virtual void flush(void) = 0;
    virtual Encode_Status encode(VideoEncRawBuffer *inBuffer, uint32_t timeout = FUNC_BLOCK) = 0;
    virtual Encode_Status getOutput(VideoEncOutputBuffer *outBuffer, uint32_t timeout = FUNC_BLOCK) = 0;
    virtual Encode_Status getParameters(VideoParamConfigSet *videoEncParams) = 0;
    virtual Encode_Status setParameters(VideoParamConfigSet *videoEncParams) = 0;
    virtual Encode_Status getConfig(VideoParamConfigSet *videoEncConfig) = 0;
    virtual Encode_Status setConfig(VideoParamConfigSet *videoEncConfig) = 0;
    virtual Encode_Status getMaxOutSize(uint32_t *maxSize) = 0;
};

#endif /* VIDEO_ENCODER_INTERFACE_H_ */
