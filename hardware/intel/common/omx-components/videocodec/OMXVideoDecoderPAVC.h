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

#ifndef OMX_VIDEO_DECODER_PAVC_H_
#define OMX_VIDEO_DECODER_PAVC_H_


#include "OMXVideoDecoderBase.h"

class OMXVideoDecoderPAVC : public OMXVideoDecoderBase {
public:
    OMXVideoDecoderPAVC();
    virtual ~OMXVideoDecoderPAVC();

protected:
    virtual OMX_ERRORTYPE InitInputPortFormatSpecific(OMX_PARAM_PORTDEFINITIONTYPE *paramPortDefinitionInput);
    virtual OMX_ERRORTYPE ProcessorInit(void);
    virtual OMX_ERRORTYPE ProcessorDeinit(void);
    virtual OMX_ERRORTYPE ProcessorFlush(OMX_U32 portIndex);
    virtual OMX_ERRORTYPE ProcessorProcess(
            OMX_BUFFERHEADERTYPE ***pBuffers,
            buffer_retain_t *retains,
            OMX_U32 numberBuffers);

   virtual OMX_ERRORTYPE PrepareConfigBuffer(VideoConfigBuffer *p);
   virtual OMX_ERRORTYPE PrepareDecodeBuffer(OMX_BUFFERHEADERTYPE *buffer, buffer_retain_t *retain, VideoDecodeBuffer *p);

   virtual OMX_ERRORTYPE BuildHandlerList(void);
   DECLARE_HANDLER(OMXVideoDecoderPAVC, ParamVideoAvc);
   DECLARE_HANDLER(OMXVideoDecoderPAVC, VideoProfileLevelQuerySupported);
   DECLARE_HANDLER(OMXVideoDecoderPAVC, VideoProfileLevelCurrent);


private:
    enum {
        // OMX_PARAM_PORTDEFINITIONTYPE
        INPORT_MIN_BUFFER_COUNT = 1,
        INPORT_ACTUAL_BUFFER_COUNT = 4,
        INPORT_BUFFER_SIZE = 1382400,

        // for OMX_VIDEO_PARAM_INTEL_AVC_DECODE_SETTINGS
        // default number of reference frame
        NUM_REFERENCE_FRAME = 4,

        // extra number of reference frame to allocate for video conferencing use case.
        // total number of reference frame allocated by default  in video conferencing use case is 10.
        EXTRA_REFERENCE_FRAME = 6,
    };

    OMX_VIDEO_PARAM_AVCTYPE mParamAvc;
    OMX_VIDEO_AVCPROFILETYPE mCurrentProfile;
    OMX_VIDEO_AVCLEVELTYPE mCurrentLevel;
    int32_t mIMROffset;

};

#endif /* OMX_VIDEO_DECODER_PAVC_H_ */

