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



#ifndef OMX_VIDEO_DECODER_VP9HWR_H_
#define OMX_VIDEO_DECODER_VP9HWR_H_

#include "OMXVideoDecoderBase.h"
#include "vpx/vpx_decoder.h"
#include "vpx/vpx_codec.h"
#include "vpx/vp8dx.h"
#include <va/va.h>
#include <va/va_android.h>
#include <va/va_tpi.h>

// VAAPI Allocator internal Mem ID
typedef struct vaapiMemId
{
    VASurfaceID*       m_surface;
    unsigned int       m_key; //Gralloc handle from which this srf was created
    unsigned char*     m_usrAddr;
    bool               m_render_done;
    bool               m_released;
}vaapiMemId;

typedef unsigned int Display;
#define ANDROID_DISPLAY_HANDLE 0x18c34078

#define DECODE_WITH_GRALLOC_BUFFER
#define VPX_DECODE_BORDER 0

#define MAX_NATIVE_BUFFER_COUNT 64

class OMXVideoDecoderVP9HWR : public OMXVideoDecoderBase {
public:
    OMXVideoDecoderVP9HWR();
    virtual ~OMXVideoDecoderVP9HWR();
    vaapiMemId* extMIDs[MAX_NATIVE_BUFFER_COUNT];
    int extUtilBufferCount;
    int extMappedNativeBufferCount;
    unsigned int extNativeBufferSize;
    // (or mapped from vaSurface) to a pre-set max size.
    int extActualBufferStride;
    int extActualBufferHeightStride;

protected:
    virtual OMX_ERRORTYPE InitInputPortFormatSpecific(OMX_PARAM_PORTDEFINITIONTYPE *paramPortDefinitionInput);
    virtual OMX_ERRORTYPE ProcessorInit(void);
    virtual OMX_ERRORTYPE ProcessorDeinit(void);
    virtual OMX_ERRORTYPE ProcessorStop(void);
    virtual OMX_ERRORTYPE ProcessorFlush(OMX_U32 portIndex);
    virtual OMX_ERRORTYPE ProcessorProcess(
            OMX_BUFFERHEADERTYPE ***pBuffers,
            buffer_retain_t *retains,
            OMX_U32 numberBuffers);
    virtual OMX_ERRORTYPE ProcessorReset(void);

    virtual OMX_ERRORTYPE ProcessorPreFillBuffer(OMX_BUFFERHEADERTYPE* buffer);
    virtual bool IsAllBufferAvailable(void);

    virtual OMX_ERRORTYPE PrepareConfigBuffer(VideoConfigBuffer *p);
    virtual OMX_ERRORTYPE PrepareDecodeBuffer(OMX_BUFFERHEADERTYPE *buffer, buffer_retain_t *retain, VideoDecodeBuffer *p);

    virtual OMX_ERRORTYPE BuildHandlerList(void);

    virtual OMX_ERRORTYPE FillRenderBuffer(OMX_BUFFERHEADERTYPE **pBuffer,
                                           buffer_retain_t *retain,
                                           OMX_U32 inportBufferFlags,
                                           OMX_BOOL *isResolutionChange);

    virtual OMX_ERRORTYPE HandleFormatChange(void);

    virtual OMX_COLOR_FORMATTYPE GetOutputColorFormat(int width);
    virtual OMX_ERRORTYPE GetDecoderOutputCropSpecific(OMX_PTR pStructure);
    virtual OMX_ERRORTYPE GetNativeBufferUsageSpecific(OMX_PTR pStructure);
    virtual OMX_ERRORTYPE SetNativeBufferModeSpecific(OMX_PTR pStructure);

    friend int reallocVP9FrameBuffer(void *user_priv, unsigned int new_size, vpx_codec_frame_buffer_t *fb);

    DECLARE_HANDLER(OMXVideoDecoderVP9HWR, ParamVideoVp9);
private:
    OMX_ERRORTYPE initDecoder();
    OMX_ERRORTYPE destroyDecoder();

    enum {
        // OMX_PARAM_PORTDEFINITIONTYPE
        INPORT_MIN_BUFFER_COUNT = 1,
        INPORT_ACTUAL_BUFFER_COUNT = 5,
        INPORT_BUFFER_SIZE = 1382400,
        OUTPORT_NATIVE_BUFFER_COUNT = 12, // 8 reference + 1 current + 3 for asynchronized mode
        OUTPORT_ACTUAL_BUFFER_COUNT = 12,  // for raw data mode
        INTERNAL_MAX_FRAME_WIDTH = 1920,
        INTERNAL_MAX_FRAME_HEIGHT = 1088,
    };

    void *mCtx;

    //OMX_VIDEO_PARAM_VP9TYPE mParamVp9;
    vpx_codec_frame_buffer_t* mFrameBuffers;
    int mNumFrameBuffer;

    // These members are for Adaptive playback
    uint32_t mDecodedImageWidth;
    uint32_t mDecodedImageHeight;
    uint32_t mDecodedImageNewWidth;
    uint32_t mDecodedImageNewHeight;

    Display* mDisplay;
    VADisplay mVADisplay;
};

#endif /* OMX_VIDEO_DECODER_VP9HWR_H_ */

