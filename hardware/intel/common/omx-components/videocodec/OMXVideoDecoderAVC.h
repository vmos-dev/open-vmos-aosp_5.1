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

#ifndef OMX_VIDEO_DECODER_AVC_H_
#define OMX_VIDEO_DECODER_AVC_H_


#include "OMXVideoDecoderBase.h"

class OMXVideoDecoderAVC : public OMXVideoDecoderBase {
public:
    OMXVideoDecoderAVC();
    virtual ~OMXVideoDecoderAVC();

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
   virtual OMX_ERRORTYPE SetMaxOutputBufferCount(OMX_PARAM_PORTDEFINITIONTYPE *p);
   virtual OMX_COLOR_FORMATTYPE GetOutputColorFormat(int width);
   DECLARE_HANDLER(OMXVideoDecoderAVC, ParamVideoAvc);
   DECLARE_HANDLER(OMXVideoDecoderAVC, ParamIntelAVCDecodeSettings);
   DECLARE_HANDLER(OMXVideoDecoderAVC, ParamVideoAVCProfileLevel);

private:
    inline OMX_ERRORTYPE AccumulateBuffer(OMX_BUFFERHEADERTYPE *buffer);
    inline OMX_ERRORTYPE FillDecodeBuffer(VideoDecodeBuffer *p);

private:
    enum {
        // OMX_PARAM_PORTDEFINITIONTYPE
        INPORT_MIN_BUFFER_COUNT = 1,
        INPORT_ACTUAL_BUFFER_COUNT = 5,
        INPORT_BUFFER_SIZE = 1382400,

        // for OMX_VIDEO_PARAM_INTEL_AVC_DECODE_SETTINGS
        // default number of reference frame
        NUM_REFERENCE_FRAME = 4,

        // extra number of reference frame to allocate for video conferencing use case.
        // total number of reference frame allocated by default  in video conferencing use case is 10.
        EXTRA_REFERENCE_FRAME = 6,

        // a typical value for 1080p clips
        OUTPORT_NATIVE_BUFFER_COUNT = 11,

        MAX_OUTPORT_BUFFER_COUNT = 23,
    };

    OMX_VIDEO_PARAM_AVCTYPE mParamAvc;

    // This parameter is used for video conferencing use case. Application or OMX client can preset
    // maximum video size and maximum reference frame (default value is NUM_REFERENCE_FRAME). Using these
    // information OMX AVC decoder can start up video decoder library without paring configuration data, or start up
    // video decoder at earlier stage.
    // If actual video size is less than the maximum video size, frame cropping will be used in the encoder side.
    OMX_VIDEO_PARAM_INTEL_AVC_DECODE_SETTINGS mDecodeSettings;

private:
    OMX_U8 *mAccumulateBuffer;
    OMX_U32 mBufferSize;
    OMX_U32 mFilledLen;
    OMX_TICKS mTimeStamp;
};

#endif /* OMX_VIDEO_DECODER_AVC_H_ */
