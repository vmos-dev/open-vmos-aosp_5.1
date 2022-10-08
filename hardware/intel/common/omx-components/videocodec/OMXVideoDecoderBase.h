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


#ifndef OMX_VIDEO_DECODER_BASE_H_
#define OMX_VIDEO_DECODER_BASE_H_


#include "OMXComponentCodecBase.h"
#include "VideoDecoderInterface.h"
#include "VideoDecoderHost.h"

#ifdef USE_GEN_HW
#include "graphics.h"
#endif

static const char* VA_VED_RAW_MIME_TYPE = "video/x-raw-vaved";
static const uint32_t VA_VED_COLOR_FORMAT = 0x20;


class OMXVideoDecoderBase : public OMXComponentCodecBase {
public:
    OMXVideoDecoderBase();
    virtual ~OMXVideoDecoderBase();

protected:
    virtual OMX_ERRORTYPE InitInputPort(void);
    virtual OMX_ERRORTYPE InitOutputPort(void);
    virtual OMX_ERRORTYPE InitInputPortFormatSpecific(OMX_PARAM_PORTDEFINITIONTYPE *input) = 0;
    virtual OMX_ERRORTYPE InitOutputPortFormatSpecific(OMX_PARAM_PORTDEFINITIONTYPE *output);

    virtual OMX_ERRORTYPE ProcessorInit(void);
    virtual OMX_ERRORTYPE ProcessorDeinit(void);
    virtual OMX_ERRORTYPE ProcessorStart(void);
    virtual OMX_ERRORTYPE ProcessorStop(void);
    virtual OMX_ERRORTYPE ProcessorPause(void);
    virtual OMX_ERRORTYPE ProcessorResume(void);
    virtual OMX_ERRORTYPE ProcessorFlush(OMX_U32 portIndex);
    virtual OMX_ERRORTYPE ProcessorProcess(
            OMX_BUFFERHEADERTYPE ***pBuffers,
            buffer_retain_t *retains,
            OMX_U32 numberBuffers);
    virtual OMX_ERRORTYPE ProcessorReset(void);
    virtual bool IsAllBufferAvailable(void);
    virtual OMX_ERRORTYPE SetMaxOutputBufferCount(OMX_PARAM_PORTDEFINITIONTYPE *p);
    virtual OMX_ERRORTYPE ProcessorPreFillBuffer(OMX_BUFFERHEADERTYPE* buffer);
    virtual OMX_ERRORTYPE ProcessorPreFreeBuffer(OMX_U32 nPortIndex,OMX_BUFFERHEADERTYPE * pBuffer);
    virtual OMX_ERRORTYPE PrepareConfigBuffer(VideoConfigBuffer *p);
    virtual OMX_ERRORTYPE PrepareDecodeBuffer(OMX_BUFFERHEADERTYPE *buffer, buffer_retain_t *retain, VideoDecodeBuffer *p);
    virtual OMX_ERRORTYPE FillRenderBuffer(OMX_BUFFERHEADERTYPE **pBuffer,  buffer_retain_t *retain,
                              OMX_U32 inportBufferFlags,  OMX_BOOL *isResolutionChange);
    virtual OMX_ERRORTYPE HandleFormatChange(void);
    virtual OMX_ERRORTYPE TranslateDecodeStatus(Decode_Status status);
    virtual OMX_COLOR_FORMATTYPE GetOutputColorFormat(int width);
    virtual OMX_ERRORTYPE BuildHandlerList(void);
    virtual OMX_ERRORTYPE SetDecoderOutputCropSpecific(OMX_PTR pStructure);
    virtual OMX_ERRORTYPE GetDecoderOutputCropSpecific(OMX_PTR pStructure);
    virtual OMX_ERRORTYPE GetNativeBufferUsageSpecific(OMX_PTR pStructure);
    virtual OMX_ERRORTYPE SetNativeBufferUsageSpecific(OMX_PTR pStructure);
    virtual OMX_ERRORTYPE SetNativeBufferModeSpecific(OMX_PTR pStructure);
    virtual OMX_ERRORTYPE GetNativeBufferModeSpecific(OMX_PTR pStructure);

    DECLARE_HANDLER(OMXVideoDecoderBase, ParamVideoPortFormat);
    DECLARE_HANDLER(OMXVideoDecoderBase, CapabilityFlags);
    DECLARE_HANDLER(OMXVideoDecoderBase, NativeBufferUsage);
    DECLARE_HANDLER(OMXVideoDecoderBase, NativeBuffer);
    DECLARE_HANDLER(OMXVideoDecoderBase, NativeBufferMode);
    DECLARE_HANDLER(OMXVideoDecoderBase, DecoderRotation);
    DECLARE_HANDLER(OMXVideoDecoderBase, DecoderOutputCrop);
#ifdef TARGET_HAS_ISV
    DECLARE_HANDLER(OMXVideoDecoderBase, DecoderVppBufferNum);
#endif
    DECLARE_HANDLER(OMXVideoDecoderBase, ErrorReportMode);

private:
    enum {
        // OMX_PARAM_PORTDEFINITIONTYPE
        INPORT_MIN_BUFFER_COUNT = 1,
        INPORT_ACTUAL_BUFFER_COUNT = 256,
        INPORT_BUFFER_SIZE = 1382400,

        // OMX_PARAM_PORTDEFINITIONTYPE
        OUTPORT_MIN_BUFFER_COUNT = 1,
        OUTPORT_ACTUAL_BUFFER_COUNT = 4,
        OUTPORT_BUFFER_SIZE = 1382400,

        OUTPORT_NATIVE_BUFFER_COUNT = 10,
    };

    struct GraphicBufferParam {
        uint32_t graphicBufferStride;
        uint32_t graphicBufferWidth;
        uint32_t graphicBufferHeight;
        uint32_t graphicBufferColorFormat;
    };
    uint32_t mRotationDegrees;
#ifdef TARGET_HAS_ISV
    uint32_t mVppBufferNum;
#endif

protected:
    IVideoDecoder *mVideoDecoder;
    int  mNativeBufferCount;
    enum WorkingMode {
        GRAPHICBUFFER_MODE,
        RAWDATA_MODE,
    };
    WorkingMode mWorkingMode;
    bool mErrorReportEnabled;
    GraphicBufferParam mGraphicBufferParam;
    uint32_t mOMXBufferHeaderTypePtrNum;
    OMX_BUFFERHEADERTYPE *mOMXBufferHeaderTypePtrArray[MAX_GRAPHIC_BUFFER_NUM];
};

#endif /* OMX_VIDEO_DECODER_BASE_H_ */
