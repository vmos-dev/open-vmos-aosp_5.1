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


#ifndef VIDEO_DECODER_INTERFACE_H_
#define VIDEO_DECODER_INTERFACE_H_

#include "VideoDecoderDefs.h"

class IVideoDecoder {
public:
    virtual ~IVideoDecoder() {}
    virtual Decode_Status start(VideoConfigBuffer *buffer) = 0;
    virtual Decode_Status reset(VideoConfigBuffer *buffer) = 0;
    virtual void stop(void) = 0;
    virtual void flush() = 0;
    virtual Decode_Status decode(VideoDecodeBuffer *buffer) = 0;
    virtual void freeSurfaceBuffers(void) = 0;
    virtual const VideoRenderBuffer* getOutput(bool draining = false, VideoErrorBuffer *output_buf = NULL) = 0;
    virtual const VideoFormatInfo* getFormatInfo(void) = 0;
    virtual Decode_Status signalRenderDone(void * graphichandler) = 0;
    virtual bool checkBufferAvail() = 0;
    virtual Decode_Status getRawDataFromSurface(VideoRenderBuffer *renderBuffer = NULL, uint8_t *pRawData = NULL, uint32_t *pSize = NULL, bool internal = true) = 0;
    virtual void enableErrorReport(bool enabled) = 0;
};

#endif /* VIDEO_DECODER_INTERFACE_H_ */
