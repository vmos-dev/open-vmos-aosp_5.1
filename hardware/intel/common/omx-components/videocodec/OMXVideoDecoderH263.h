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


#ifndef OMX_VIDEO_DECODER_H263_H_
#define OMX_VIDEO_DECODER_H263_H_


#include "OMXVideoDecoderBase.h"

class OMXVideoDecoderH263 : public OMXVideoDecoderBase {
public:
    OMXVideoDecoderH263();
    virtual ~OMXVideoDecoderH263();

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
   DECLARE_HANDLER(OMXVideoDecoderH263, ParamVideoH263);
   DECLARE_HANDLER(OMXVideoDecoderH263, ParamVideoH263ProfileLevel);

private:
    enum {
        // OMX_PARAM_PORTDEFINITIONTYPE
        INPORT_MIN_BUFFER_COUNT = 1,
        INPORT_ACTUAL_BUFFER_COUNT = 5,
        INPORT_BUFFER_SIZE = 1382400,

        OUTPORT_NATIVE_BUFFER_COUNT = 9,
    };

    OMX_VIDEO_PARAM_H263TYPE mParamH263;
};


#endif /* OMX_VIDEO_DECODER_H263_H_ */
