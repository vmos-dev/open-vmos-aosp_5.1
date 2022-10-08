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



#ifndef OMX_VIDEO_ENCODER_MPEG4_H_
#define OMX_VIDEO_ENCODER_MPEG4_H_


#include "OMXVideoEncoderBase.h"

class OMXVideoEncoderMPEG4 : public OMXVideoEncoderBase {
public:
    OMXVideoEncoderMPEG4();
    virtual ~OMXVideoEncoderMPEG4();

protected:
    virtual OMX_ERRORTYPE InitOutputPortFormatSpecific(OMX_PARAM_PORTDEFINITIONTYPE *paramPortDefinitionOutput);
    virtual OMX_ERRORTYPE ProcessorInit(void);
    virtual OMX_ERRORTYPE ProcessorDeinit(void);
    virtual OMX_ERRORTYPE ProcessorProcess(
            OMX_BUFFERHEADERTYPE **buffers,
            buffer_retain_t *retains,
            OMX_U32 numberBuffers);

    virtual OMX_ERRORTYPE BuildHandlerList(void);
    virtual OMX_ERRORTYPE SetVideoEncoderParam();
    DECLARE_HANDLER(OMXVideoEncoderMPEG4, ParamVideoMpeg4);
    DECLARE_HANDLER(OMXVideoEncoderMPEG4, ParamVideoProfileLevelQuerySupported);

private:
    enum {
        // OMX_PARAM_PORTDEFINITIONTYPE
        OUTPORT_MIN_BUFFER_COUNT = 1,
        OUTPORT_ACTUAL_BUFFER_COUNT = 2,
        OUTPORT_BUFFER_SIZE = 1382400,
    };

    OMX_VIDEO_PARAM_MPEG4TYPE mParamMpeg4;
};

#endif /* OMX_VIDEO_ENCODER_MPEG4_H_ */
