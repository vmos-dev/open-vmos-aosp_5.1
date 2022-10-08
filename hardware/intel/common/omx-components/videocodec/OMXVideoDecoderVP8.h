/*
* Copyright (c) 2012 Intel Corporation.  All rights reserved.
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



#ifndef OMX_VIDEO_DECODER_VP8_H_
#define OMX_VIDEO_DECODER_VP8_H_


#include "OMXVideoDecoderBase.h"
#include <OMX_VideoExt.h>

class OMXVideoDecoderVP8 : public OMXVideoDecoderBase {
public:
    OMXVideoDecoderVP8();
    virtual ~OMXVideoDecoderVP8();

protected:
    virtual OMX_ERRORTYPE InitInputPortFormatSpecific(OMX_PARAM_PORTDEFINITIONTYPE *paramPortDefinitionInput);
    virtual OMX_ERRORTYPE ProcessorInit(void);
    virtual OMX_ERRORTYPE ProcessorDeinit(void);
    virtual OMX_ERRORTYPE ProcessorProcess(
            OMX_BUFFERHEADERTYPE ***pBuffers,
            buffer_retain_t *retains,
            OMX_U32 numberBuffers);

   virtual OMX_ERRORTYPE PrepareConfigBuffer(VideoConfigBuffer *p);
   virtual OMX_ERRORTYPE PrepareDecodeBuffer(OMX_BUFFERHEADERTYPE *buffer, buffer_retain_t *retain, VideoDecodeBuffer *p);

   virtual OMX_ERRORTYPE BuildHandlerList(void);
   virtual OMX_ERRORTYPE SetMaxOutputBufferCount(OMX_PARAM_PORTDEFINITIONTYPE *p);
   virtual OMX_COLOR_FORMATTYPE GetOutputColorFormat(int width);
   DECLARE_HANDLER(OMXVideoDecoderVP8, ParamVideoVp8);

private:
    enum {
        // OMX_PARAM_PORTDEFINITIONTYPE
        INPORT_MIN_BUFFER_COUNT = 1,
        INPORT_ACTUAL_BUFFER_COUNT = 5,
        INPORT_BUFFER_SIZE = 1382400,

        OUTPORT_NATIVE_BUFFER_COUNT = 8,
    };

    OMX_VIDEO_PARAM_VP8TYPE mParamVp8;
};

#endif /* OMX_VIDEO_DECODER_VP8_H_ */
