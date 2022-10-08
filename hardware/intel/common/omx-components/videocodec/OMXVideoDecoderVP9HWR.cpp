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


#define LOG_NDEBUG 0
#define LOG_TAG "OMXVideoDecoderVP9HWR"
#include <wrs_omxil_core/log.h>
#include "OMXVideoDecoderVP9HWR.h"

#include <system/window.h>
#include <HardwareAPI.h>
#include <hardware/hardware.h>
#include <hardware/gralloc.h>
#include <system/graphics.h>

static const char* VP9_MIME_TYPE = "video/x-vnd.on2.vp9";

static int GetCPUCoreCount()
{
    int cpuCoreCount = 1;
#if defined(_SC_NPROCESSORS_ONLN)
    cpuCoreCount = sysconf(_SC_NPROCESSORS_ONLN);
#else
    // _SC_NPROC_ONLN must be defined...
    cpuCoreCount = sysconf(_SC_NPROC_ONLN);
#endif
    if (cpuCoreCount < 1) {
        LOGW("Get CPU Core Count error.");
        cpuCoreCount = 1;
    }
    LOGV("Number of CPU cores: %d", cpuCoreCount);
    return cpuCoreCount;
}


OMXVideoDecoderVP9HWR::OMXVideoDecoderVP9HWR()
{
    LOGV("OMXVideoDecoderVP9HWR is constructed.");

    mNumFrameBuffer = 0;
    mCtx = NULL;

    mNativeBufferCount = OUTPORT_NATIVE_BUFFER_COUNT;
    extUtilBufferCount = 0;
    extMappedNativeBufferCount = 0;
    BuildHandlerList();

    mDecodedImageWidth = 0;
    mDecodedImageHeight = 0;
    mDecodedImageNewWidth = 0;
    mDecodedImageNewHeight = 0;

#ifdef DECODE_WITH_GRALLOC_BUFFER
    // setup va
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    mDisplay = new Display;
    *mDisplay = ANDROID_DISPLAY_HANDLE;

    mVADisplay = vaGetDisplay(mDisplay);
    if (mVADisplay == NULL) {
        LOGE("vaGetDisplay failed.");
    }

    int majorVersion, minorVersion;
    vaStatus = vaInitialize(mVADisplay, &majorVersion, &minorVersion);
    if (vaStatus != VA_STATUS_SUCCESS) {
        LOGE("vaInitialize failed.");
    } else {
        LOGV("va majorVersion=%d, minorVersion=%d", majorVersion, minorVersion);
    }
#endif
}

OMXVideoDecoderVP9HWR::~OMXVideoDecoderVP9HWR()
{
    LOGV("OMXVideoDecoderVP9HWR is destructed.");

    unsigned int i = 0;

    if (mVADisplay) {
        vaTerminate(mVADisplay);
        mVADisplay = NULL;
    }
}


// Callback func for vpx decoder to get decode buffer
// Now we map from the vaSurface to deploy gralloc buffer
// as decode buffer
int getVP9FrameBuffer(void *user_priv,
                          unsigned int new_size,
                          vpx_codec_frame_buffer_t *fb)
{
    OMXVideoDecoderVP9HWR * p = (OMXVideoDecoderVP9HWR *)user_priv;
    if (fb == NULL) {
        return -1;
    }

    // TODO: Adaptive playback case needs to reconsider
    if (p->extNativeBufferSize < new_size) {
        LOGE("Provided frame buffer size < requesting min size.");
        return -1;
    }

    int i;
    for (i = 0; i < p->extMappedNativeBufferCount; i++ ) {
        if ((p->extMIDs[i]->m_render_done == true) &&
            (p->extMIDs[i]->m_released == true)) {
            fb->data = p->extMIDs[i]->m_usrAddr;
            fb->size = p->extNativeBufferSize;
            fb->fb_stride = p->extActualBufferStride;
            fb->fb_height_stride = p->extActualBufferHeightStride;
            fb->fb_index = i;
            p->extMIDs[i]->m_released = false;
            break;
        }
    }

    if (i == p->extMappedNativeBufferCount) {
        LOGE("No available frame buffer in pool.");
        return -1;
    }
    return 0;
}

// call back function from libvpx to inform frame buffer
// not used anymore.
int releaseVP9FrameBuffer(void *user_priv, vpx_codec_frame_buffer_t *fb)
{
    int i;
    OMXVideoDecoderVP9HWR * p = (OMXVideoDecoderVP9HWR *)user_priv;
    if (fb == NULL) {
        return -1;
    }
    for (i = 0; i < p->extMappedNativeBufferCount; i++ ) {
        if (fb->data == p->extMIDs[i]->m_usrAddr) {
            p->extMIDs[i]->m_released = true;
            break;
        }
    }

    if (i == p->extMappedNativeBufferCount) {
        LOGE("Not found matching frame buffer in pool, libvpx's wrong?");
        return -1;
    }
    return 0;
}


OMX_ERRORTYPE OMXVideoDecoderVP9HWR::initDecoder()
{
    mCtx = new vpx_codec_ctx_t;
    vpx_codec_err_t vpx_err;
    vpx_codec_dec_cfg_t cfg;
    memset(&cfg, 0, sizeof(vpx_codec_dec_cfg_t));
    cfg.threads = GetCPUCoreCount();
    if ((vpx_err = vpx_codec_dec_init(
                (vpx_codec_ctx_t *)mCtx,
                 &vpx_codec_vp9_dx_algo,
                 &cfg, 0))) {
        LOGE("on2 decoder failed to initialize. (%d)", vpx_err);
        return OMX_ErrorNotReady;
    }

    mNumFrameBuffer = OUTPORT_NATIVE_BUFFER_COUNT;

    if (vpx_codec_set_frame_buffer_functions((vpx_codec_ctx_t *)mCtx,
                                    getVP9FrameBuffer,
                                    releaseVP9FrameBuffer,
                                    this)) {
      LOGE("Failed to configure external frame buffers");
      return OMX_ErrorNotReady;
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoDecoderVP9HWR::destroyDecoder()
{
    vpx_codec_destroy((vpx_codec_ctx_t *)mCtx);
    delete (vpx_codec_ctx_t *)mCtx;
    mCtx = NULL;
    return OMX_ErrorNone;
}


OMX_ERRORTYPE OMXVideoDecoderVP9HWR::InitInputPortFormatSpecific(
    OMX_PARAM_PORTDEFINITIONTYPE *paramPortDefinitionInput)
{
    paramPortDefinitionInput->nBufferCountActual = INPORT_ACTUAL_BUFFER_COUNT;
    paramPortDefinitionInput->nBufferCountMin = INPORT_MIN_BUFFER_COUNT;
    paramPortDefinitionInput->nBufferSize = INPORT_BUFFER_SIZE;
    paramPortDefinitionInput->format.video.cMIMEType = (OMX_STRING)VP9_MIME_TYPE;
    paramPortDefinitionInput->format.video.eCompressionFormat = OMX_VIDEO_CodingVP9;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoDecoderVP9HWR::ProcessorInit(void)
{
    unsigned int i = 0;

    for (i = 0; i < MAX_NATIVE_BUFFER_COUNT; i++) {
        extMIDs[i] = (vaapiMemId*)malloc(sizeof(vaapiMemId));
        extMIDs[i]->m_usrAddr = NULL;
        extMIDs[i]->m_surface = new VASurfaceID;
    }

    initDecoder();

    if (RAWDATA_MODE == mWorkingMode) {
        OMX_PARAM_PORTDEFINITIONTYPE paramPortDefinitionInput;

        memcpy(&paramPortDefinitionInput,
               this->ports[INPORT_INDEX]->GetPortDefinition(),
               sizeof(paramPortDefinitionInput));

        extNativeBufferSize = INTERNAL_MAX_FRAME_WIDTH *
                              INTERNAL_MAX_FRAME_HEIGHT * 1.5;
        extActualBufferStride = INTERNAL_MAX_FRAME_WIDTH;
        extActualBufferHeightStride = INTERNAL_MAX_FRAME_HEIGHT;

        for (i = 0; i < OUTPORT_ACTUAL_BUFFER_COUNT; i++ ) {
            extMIDs[i]->m_usrAddr = (unsigned char*)malloc(sizeof(unsigned char) *
                                    extNativeBufferSize);
            extMIDs[i]->m_render_done = true;
            extMIDs[i]->m_released = true;
        }
        extMappedNativeBufferCount = OUTPORT_ACTUAL_BUFFER_COUNT;
        return OMX_ErrorNone;
    }

#ifdef DECODE_WITH_GRALLOC_BUFFER
    if (mOMXBufferHeaderTypePtrNum > MAX_NATIVE_BUFFER_COUNT) {
        LOGE("Actual OMX outport buffer header type num (%d) > MAX_NATIVE_BUFFER_COUNT (%d)",
              mOMXBufferHeaderTypePtrNum, MAX_NATIVE_BUFFER_COUNT);
        return OMX_ErrorOverflow;
    }

    OMX_PARAM_PORTDEFINITIONTYPE paramPortDefinitionInput;

    memcpy(&paramPortDefinitionInput,
        this->ports[INPORT_INDEX]->GetPortDefinition(),
        sizeof(paramPortDefinitionInput));

    int surfaceWidth = mGraphicBufferParam.graphicBufferWidth;
    int surfaceHeight = mGraphicBufferParam.graphicBufferHeight;
    int surfaceStride = mGraphicBufferParam.graphicBufferStride;
    extNativeBufferSize = mGraphicBufferParam.graphicBufferStride *
                          mGraphicBufferParam.graphicBufferHeight * 1.5;
    extActualBufferStride = surfaceStride;
    extActualBufferHeightStride = surfaceHeight;

    for (i = 0; i < mOMXBufferHeaderTypePtrNum; i++) {
        OMX_BUFFERHEADERTYPE *buf_hdr = mOMXBufferHeaderTypePtrArray[i];

        extMIDs[i]->m_key = (unsigned int)(buf_hdr->pBuffer);
        extMIDs[i]->m_render_done = false;
        extMIDs[i]->m_released = true;

        VAStatus va_res;
        unsigned int buffer;
        VASurfaceAttrib attribs[2];
        VASurfaceAttribExternalBuffers* surfExtBuf = new VASurfaceAttribExternalBuffers;
        int32_t format = VA_RT_FORMAT_YUV420;

        surfExtBuf->buffers= (unsigned long *)&buffer;
        surfExtBuf->num_buffers = 1;
        surfExtBuf->pixel_format = VA_FOURCC_NV12;
        surfExtBuf->width = surfaceWidth;
        surfExtBuf->height = surfaceHeight;
        surfExtBuf->data_size = surfaceStride * surfaceHeight * 1.5;
        surfExtBuf->num_planes = 2;
        surfExtBuf->pitches[0] = surfaceStride;
        surfExtBuf->pitches[1] = surfaceStride;
        surfExtBuf->pitches[2] = 0;
        surfExtBuf->pitches[3] = 0;
        surfExtBuf->offsets[0] = 0;
        surfExtBuf->offsets[1] = surfaceStride * surfaceHeight;
        surfExtBuf->offsets[2] = 0;
        surfExtBuf->offsets[3] = 0;
        surfExtBuf->flags = VA_SURFACE_ATTRIB_MEM_TYPE_ANDROID_GRALLOC;

        surfExtBuf->buffers[0] = (unsigned int)buf_hdr->pBuffer;

        attribs[0].type = (VASurfaceAttribType)VASurfaceAttribMemoryType;
        attribs[0].flags = VA_SURFACE_ATTRIB_SETTABLE;
        attribs[0].value.type = VAGenericValueTypeInteger;
        attribs[0].value.value.i = VA_SURFACE_ATTRIB_MEM_TYPE_ANDROID_GRALLOC;

        attribs[1].type = (VASurfaceAttribType)VASurfaceAttribExternalBufferDescriptor;
        attribs[1].flags = VA_SURFACE_ATTRIB_SETTABLE;
        attribs[1].value.type = VAGenericValueTypePointer;
        attribs[1].value.value.p = (void *)surfExtBuf;

        va_res = vaCreateSurfaces(mVADisplay,
                                  format,
                                  surfaceWidth,
                                  surfaceHeight,
                                  extMIDs[i]->m_surface,
                                  1,
                                  attribs,
                                  2);

        if (va_res != VA_STATUS_SUCCESS) {
            LOGE("Failed to create vaSurface!");
            return OMX_ErrorUndefined;
        }

        delete surfExtBuf;

        VAImage image;
        unsigned char* usrptr;

        va_res = vaDeriveImage(mVADisplay, *(extMIDs[i]->m_surface), &image);
        if (VA_STATUS_SUCCESS == va_res) {
            va_res = vaMapBuffer(mVADisplay, image.buf, (void **) &usrptr);
            if (VA_STATUS_SUCCESS == va_res) {
                extMIDs[i]->m_usrAddr = usrptr;
                vaUnmapBuffer(mVADisplay, image.buf);
            }
            vaDestroyImage(mVADisplay, image.image_id);
        }
        extMappedNativeBufferCount++;
    }
    return OMX_ErrorNone;
#endif
}

OMX_ERRORTYPE OMXVideoDecoderVP9HWR::ProcessorDeinit(void)
{
    destroyDecoder();
    unsigned int i = 0;
    if (mWorkingMode == GRAPHICBUFFER_MODE) {
        for (i = 0; i < mOMXBufferHeaderTypePtrNum; i++) {
            if (extMIDs[i]->m_surface != NULL) {
                vaDestroySurfaces(mVADisplay, extMIDs[i]->m_surface, 1);
            }
        }

    } else if (mWorkingMode == RAWDATA_MODE) {
        for (i = 0; i < OUTPORT_ACTUAL_BUFFER_COUNT; i++ ) {
            if (extMIDs[i]->m_usrAddr != NULL) {
                free(extMIDs[i]->m_usrAddr);
                extMIDs[i]->m_usrAddr = NULL;
            }
        }
    }
    mOMXBufferHeaderTypePtrNum = 0;
    memset(&mGraphicBufferParam, 0, sizeof(mGraphicBufferParam));
    for (i = 0; i < MAX_NATIVE_BUFFER_COUNT; i++) {
        delete extMIDs[i]->m_surface;
        free(extMIDs[i]);
    }
    return OMXComponentCodecBase::ProcessorDeinit();
}

OMX_ERRORTYPE OMXVideoDecoderVP9HWR::ProcessorStop(void)
{
    return OMXComponentCodecBase::ProcessorStop();
}

OMX_ERRORTYPE OMXVideoDecoderVP9HWR::ProcessorFlush(OMX_U32)
{
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoDecoderVP9HWR::ProcessorPreFillBuffer(OMX_BUFFERHEADERTYPE* buffer)
{
    unsigned int handle = (unsigned int)buffer->pBuffer;
    unsigned int i = 0;

    if (buffer->nOutputPortIndex == OUTPORT_INDEX){
        for (i = 0; i < mOMXBufferHeaderTypePtrNum; i++) {
            if (handle == extMIDs[i]->m_key) {
                extMIDs[i]->m_render_done = true;
                break;
            }
        }
    }
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoDecoderVP9HWR::ProcessorProcess(
        OMX_BUFFERHEADERTYPE ***pBuffers,
        buffer_retain_t *retains,
        OMX_U32)
{
    OMX_ERRORTYPE ret;
    OMX_BUFFERHEADERTYPE *inBuffer = *pBuffers[INPORT_INDEX];
    OMX_BUFFERHEADERTYPE *outBuffer = *pBuffers[OUTPORT_INDEX];
    OMX_BOOL isResolutionChange = OMX_FALSE;

    if (inBuffer->pBuffer == NULL) {
        LOGE("Buffer to decode is empty.");
        return OMX_ErrorBadParameter;
    }

    if (inBuffer->nFlags & OMX_BUFFERFLAG_CODECCONFIG) {
        LOGI("Buffer has OMX_BUFFERFLAG_CODECCONFIG flag.");
    }

    if (inBuffer->nFlags & OMX_BUFFERFLAG_DECODEONLY) {
        LOGW("Buffer has OMX_BUFFERFLAG_DECODEONLY flag.");
    }

    if (inBuffer->nFlags & OMX_BUFFERFLAG_EOS) {
        if (inBuffer->nFilledLen == 0) {
            (*pBuffers[OUTPORT_INDEX])->nFilledLen = 0;
            (*pBuffers[OUTPORT_INDEX])->nFlags = OMX_BUFFERFLAG_EOS;
            return OMX_ErrorNone;
        }
    }

    if (vpx_codec_decode((vpx_codec_ctx_t *)mCtx,
                         inBuffer->pBuffer + inBuffer->nOffset,
                         inBuffer->nFilledLen,
                         NULL,
                         0)) {
        LOGE("on2 decoder failed to decode frame.");
        return OMX_ErrorBadParameter;
    }

    ret = FillRenderBuffer(pBuffers[OUTPORT_INDEX],
                           &retains[OUTPORT_INDEX],
                           ((*pBuffers[INPORT_INDEX]))->nFlags,
                           &isResolutionChange);

    if (ret == OMX_ErrorNone) {
        (*pBuffers[OUTPORT_INDEX])->nTimeStamp = inBuffer->nTimeStamp;
    }

    if (isResolutionChange) {
        HandleFormatChange();
    }

    bool inputEoS = ((*pBuffers[INPORT_INDEX])->nFlags & OMX_BUFFERFLAG_EOS);
    bool outputEoS = ((*pBuffers[OUTPORT_INDEX])->nFlags & OMX_BUFFERFLAG_EOS);
    // if output port is not eos, retain the input buffer
    // until all the output buffers are drained.
    if (inputEoS && !outputEoS) {
        retains[INPORT_INDEX] = BUFFER_RETAIN_GETAGAIN;
        // the input buffer is retained for draining purpose.
        // Set nFilledLen to 0 so buffer will not be decoded again.
        (*pBuffers[INPORT_INDEX])->nFilledLen = 0;
    }

    if (ret == OMX_ErrorNotReady) {
        retains[OUTPORT_INDEX] = BUFFER_RETAIN_GETAGAIN;
        ret = OMX_ErrorNone;
    }

    return ret;
}

OMX_ERRORTYPE OMXVideoDecoderVP9HWR::ProcessorReset(void)
{
    return OMX_ErrorNone;
}

static int ALIGN(int x, int y)
{
    // y must be a power of 2.
    return (x + y - 1) & ~(y - 1);
}

OMX_ERRORTYPE OMXVideoDecoderVP9HWR::HandleFormatChange(void)
{
    mDecodedImageWidth = mDecodedImageNewWidth;
    mDecodedImageHeight = mDecodedImageNewHeight;

    // Sync port definition as it may change.
    OMX_PARAM_PORTDEFINITIONTYPE paramPortDefinitionInput, paramPortDefinitionOutput;

    memcpy(&paramPortDefinitionInput,
        this->ports[INPORT_INDEX]->GetPortDefinition(),
        sizeof(paramPortDefinitionInput));

    memcpy(&paramPortDefinitionOutput,
        this->ports[OUTPORT_INDEX]->GetPortDefinition(),
        sizeof(paramPortDefinitionOutput));

    uint32_t width = mDecodedImageWidth;
    uint32_t height = mDecodedImageHeight;
    uint32_t stride = mDecodedImageWidth;
    uint32_t sliceHeight = mDecodedImageHeight;

    uint32_t widthCropped = mDecodedImageWidth;
    uint32_t heightCropped = mDecodedImageHeight;
    uint32_t strideCropped = widthCropped;
    uint32_t sliceHeightCropped = heightCropped;

    if (widthCropped == paramPortDefinitionOutput.format.video.nFrameWidth &&
        heightCropped == paramPortDefinitionOutput.format.video.nFrameHeight) {
        if (mWorkingMode == RAWDATA_MODE) {
            LOGW("Change of portsetting is not reported as size is not changed.");
            return OMX_ErrorNone;
        }
    }

    paramPortDefinitionInput.format.video.nFrameWidth = width;
    paramPortDefinitionInput.format.video.nFrameHeight = height;
    paramPortDefinitionInput.format.video.nStride = stride;
    paramPortDefinitionInput.format.video.nSliceHeight = sliceHeight;

    if (mWorkingMode == RAWDATA_MODE) {
        paramPortDefinitionOutput.format.video.nFrameWidth = widthCropped;
        paramPortDefinitionOutput.format.video.nFrameHeight = heightCropped;
        paramPortDefinitionOutput.format.video.nStride = strideCropped;
        paramPortDefinitionOutput.format.video.nSliceHeight = sliceHeightCropped;
    } else if (mWorkingMode == GRAPHICBUFFER_MODE) {
        // when the width and height ES parse are not larger than allocated graphic buffer in outport,
        // there is no need to reallocate graphic buffer,just report the crop info to omx client
        if (width <= mGraphicBufferParam.graphicBufferWidth &&
            height <= mGraphicBufferParam.graphicBufferHeight) {
            this->ports[INPORT_INDEX]->SetPortDefinition(&paramPortDefinitionInput, true);
            this->ports[OUTPORT_INDEX]->ReportOutputCrop();
            return OMX_ErrorNone;
        }

        if (width > mGraphicBufferParam.graphicBufferWidth ||
            height > mGraphicBufferParam.graphicBufferHeight) {
            // update the real decoded resolution to outport instead of display resolution
            // for graphic buffer reallocation
            // when the width and height parsed from ES are larger than allocated graphic buffer in outport,
            paramPortDefinitionOutput.format.video.nFrameWidth = width;
            paramPortDefinitionOutput.format.video.nFrameHeight = (height + 0x1f) & ~0x1f;
            paramPortDefinitionOutput.format.video.eColorFormat = GetOutputColorFormat(
                    paramPortDefinitionOutput.format.video.nFrameWidth);
            paramPortDefinitionOutput.format.video.nStride = stride;
            paramPortDefinitionOutput.format.video.nSliceHeight = sliceHeight;
       }
    }

    paramPortDefinitionOutput.bEnabled = (OMX_BOOL)false;
    mOMXBufferHeaderTypePtrNum = 0;
    memset(&mGraphicBufferParam, 0, sizeof(mGraphicBufferParam));

    this->ports[INPORT_INDEX]->SetPortDefinition(&paramPortDefinitionInput, true);
    this->ports[OUTPORT_INDEX]->SetPortDefinition(&paramPortDefinitionOutput, true);

    this->ports[OUTPORT_INDEX]->ReportPortSettingsChanged();
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoDecoderVP9HWR::FillRenderBuffer(OMX_BUFFERHEADERTYPE **pBuffer,
                                                      buffer_retain_t *retain,
                                                      OMX_U32 inportBufferFlags,
                                                      OMX_BOOL *isResolutionChange)
{
    OMX_BUFFERHEADERTYPE *buffer = *pBuffer;
    OMX_BUFFERHEADERTYPE *buffer_orign = buffer;

    OMX_ERRORTYPE ret = OMX_ErrorNone;

    vpx_codec_iter_t iter = NULL;
    vpx_image_t *img = NULL;
    img = vpx_codec_get_frame((vpx_codec_ctx_t *)mCtx, &iter);

    if (img != NULL) {
        if ((mDecodedImageWidth == 0) && (mDecodedImageHeight == 0)) { // init value
            mDecodedImageWidth = img->d_w;
            mDecodedImageHeight = img->d_h;
        }
        if ((mDecodedImageWidth != img->d_w) && (mDecodedImageHeight != img->d_h)) {
            mDecodedImageNewWidth = img->d_w;
            mDecodedImageNewHeight = img->d_h;
            *isResolutionChange = OMX_TRUE;
        }
    }

    if (mWorkingMode == RAWDATA_MODE) {
        if (img == NULL) {
            LOGE("vpx_codec_get_frame return NULL.");
            return OMX_ErrorNotReady;
        }

        // in Raw data mode, this flag should be always true
        extMIDs[img->fb_index]->m_render_done = true;

        void *dst = buffer->pBuffer;
        uint8_t *dst_y = (uint8_t *)dst;
        const OMX_PARAM_PORTDEFINITIONTYPE *paramPortDefinitionInput
                                      = this->ports[INPORT_INDEX]->GetPortDefinition();

        size_t inBufferWidth = paramPortDefinitionInput->format.video.nFrameWidth;
        size_t inBufferHeight = paramPortDefinitionInput->format.video.nFrameHeight;

        const OMX_PARAM_PORTDEFINITIONTYPE *paramPortDefinitionOutput
                                      = this->ports[OUTPORT_INDEX]->GetPortDefinition();

        size_t dst_y_size = paramPortDefinitionOutput->format.video.nStride *
                            paramPortDefinitionOutput->format.video.nFrameHeight;
        size_t dst_c_stride = ALIGN(paramPortDefinitionOutput->format.video.nStride / 2, 16);
        size_t dst_c_size = dst_c_stride * paramPortDefinitionOutput->format.video.nFrameHeight / 2;
        uint8_t *dst_v = dst_y + dst_y_size;
        uint8_t *dst_u = dst_v + dst_c_size;

        //test border
        dst_y += VPX_DECODE_BORDER * paramPortDefinitionOutput->format.video.nStride + VPX_DECODE_BORDER;
        dst_v += (VPX_DECODE_BORDER/2) * dst_c_stride + (VPX_DECODE_BORDER/2);
        dst_u += (VPX_DECODE_BORDER/2) * dst_c_stride + (VPX_DECODE_BORDER/2);

        const uint8_t *srcLine = (const uint8_t *)img->planes[PLANE_Y];

        for (size_t i = 0; i < img->d_h; ++i) {
            memcpy(dst_y, srcLine, img->d_w);

            srcLine += img->stride[PLANE_Y];
            dst_y += paramPortDefinitionOutput->format.video.nStride;
        }

        srcLine = (const uint8_t *)img->planes[PLANE_U];
        for (size_t i = 0; i < img->d_h / 2; ++i) {
            memcpy(dst_u, srcLine, img->d_w / 2);

            srcLine += img->stride[PLANE_U];
            dst_u += dst_c_stride;
        }

        srcLine = (const uint8_t *)img->planes[PLANE_V];
        for (size_t i = 0; i < img->d_h / 2; ++i) {
            memcpy(dst_v, srcLine, img->d_w / 2);

            srcLine += img->stride[PLANE_V];
            dst_v += dst_c_stride;
        }

        buffer->nOffset = 0;
        buffer->nFilledLen = dst_y_size + dst_c_size * 2;
        if (inportBufferFlags & OMX_BUFFERFLAG_EOS) {
            buffer->nFlags = OMX_BUFFERFLAG_EOS;
        }

        if (buffer_orign != buffer) {
            *retain = BUFFER_RETAIN_OVERRIDDEN;
        }
        ret = OMX_ErrorNone;

        return ret;

    }

#ifdef DECODE_WITH_GRALLOC_BUFFER
    if (NULL != img) {
        buffer = *pBuffer = mOMXBufferHeaderTypePtrArray[img->fb_index];

        if ((unsigned int)(buffer->pBuffer) != extMIDs[img->fb_index]->m_key) {
            LOGE("There is gralloc handle mismatching between pool\
                  and mOMXBufferHeaderTypePtrArray.");
            return OMX_ErrorNotReady;
        }

        extMIDs[img->fb_index]->m_render_done = false;

        buffer->nOffset = 0;

        size_t dst_y_size = img->d_w * img->d_h;

        buffer->nFilledLen = dst_y_size * 1.5; // suport only 4:2:0 for now

        if (inportBufferFlags & OMX_BUFFERFLAG_EOS) {
            buffer->nFlags = OMX_BUFFERFLAG_EOS;
        }

        if (buffer_orign != buffer) {
            *retain = BUFFER_RETAIN_OVERRIDDEN;
        }

        return OMX_ErrorNone;
    } else {
        LOGE("vpx_codec_get_frame return NULL.");
        return OMX_ErrorNotReady;
    }
#endif
}

OMX_ERRORTYPE OMXVideoDecoderVP9HWR::PrepareConfigBuffer(VideoConfigBuffer *)
{
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoDecoderVP9HWR::PrepareDecodeBuffer(OMX_BUFFERHEADERTYPE *,
                                                         buffer_retain_t *,
                                                         VideoDecodeBuffer *)
{
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoDecoderVP9HWR::BuildHandlerList(void)
{
    OMXVideoDecoderBase::BuildHandlerList();
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoDecoderVP9HWR::GetParamVideoVp9(OMX_PTR)
{
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoDecoderVP9HWR::SetParamVideoVp9(OMX_PTR)
{
    return OMX_ErrorNone;
}

OMX_COLOR_FORMATTYPE OMXVideoDecoderVP9HWR::GetOutputColorFormat(int)
{
    LOGV("Output color format is OMX_INTEL_COLOR_FormatHalYV12.");
    return (OMX_COLOR_FORMATTYPE)HAL_PIXEL_FORMAT_YV12;
}

OMX_ERRORTYPE OMXVideoDecoderVP9HWR::GetDecoderOutputCropSpecific(OMX_PTR pStructure)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_CONFIG_RECTTYPE *rectParams = (OMX_CONFIG_RECTTYPE *)pStructure;

    CHECK_TYPE_HEADER(rectParams);

    if (rectParams->nPortIndex != OUTPORT_INDEX) {
        return OMX_ErrorUndefined;
    }

    const OMX_PARAM_PORTDEFINITIONTYPE *paramPortDefinitionInput
                                      = this->ports[INPORT_INDEX]->GetPortDefinition();

    rectParams->nLeft = VPX_DECODE_BORDER;
    rectParams->nTop = VPX_DECODE_BORDER;
    rectParams->nWidth = paramPortDefinitionInput->format.video.nFrameWidth;
    rectParams->nHeight = paramPortDefinitionInput->format.video.nFrameHeight;

    return ret;
}

OMX_ERRORTYPE OMXVideoDecoderVP9HWR::GetNativeBufferUsageSpecific(OMX_PTR pStructure)
{
    OMX_ERRORTYPE ret;
    android::GetAndroidNativeBufferUsageParams *param =
        (android::GetAndroidNativeBufferUsageParams*)pStructure;
    CHECK_TYPE_HEADER(param);

    param->nUsage |= (GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_SW_READ_NEVER \
                     | GRALLOC_USAGE_SW_WRITE_OFTEN | GRALLOC_USAGE_EXTERNAL_DISP);
    return OMX_ErrorNone;

}

OMX_ERRORTYPE OMXVideoDecoderVP9HWR::SetNativeBufferModeSpecific(OMX_PTR pStructure)
{
    OMX_ERRORTYPE ret;
    EnableAndroidNativeBuffersParams *param =
        (EnableAndroidNativeBuffersParams*)pStructure;

    CHECK_TYPE_HEADER(param);
    CHECK_PORT_INDEX_RANGE(param);
    CHECK_SET_PARAM_STATE();

    if (!param->enable) {
        mWorkingMode = RAWDATA_MODE;
        return OMX_ErrorNone;
    }
    mWorkingMode = GRAPHICBUFFER_MODE;
    PortVideo *port = NULL;
    port = static_cast<PortVideo *>(this->ports[OUTPORT_INDEX]);


    OMX_PARAM_PORTDEFINITIONTYPE port_def;
    memcpy(&port_def,port->GetPortDefinition(),sizeof(port_def));
    port_def.nBufferCountMin = mNativeBufferCount;
    port_def.nBufferCountActual = mNativeBufferCount;
    port_def.format.video.cMIMEType = (OMX_STRING)VA_VED_RAW_MIME_TYPE;
    port_def.format.video.eColorFormat = OMX_INTEL_COLOR_FormatYUV420PackedSemiPlanar;

    // add borders for libvpx decode need.
    port_def.format.video.nFrameHeight += VPX_DECODE_BORDER * 2;
    port_def.format.video.nFrameWidth += VPX_DECODE_BORDER * 2;
    // make heigth 32bit align
    port_def.format.video.nFrameHeight = (port_def.format.video.nFrameHeight + 0x1f) & ~0x1f;
    port_def.format.video.eColorFormat = GetOutputColorFormat(
                        port_def.format.video.nFrameWidth);
    port->SetPortDefinition(&port_def,true);

     return OMX_ErrorNone;
}


bool OMXVideoDecoderVP9HWR::IsAllBufferAvailable(void)
{
    bool b = ComponentBase::IsAllBufferAvailable();
    if (b == false) {
        return false;
    }

    PortVideo *port = NULL;
    port = static_cast<PortVideo *>(this->ports[OUTPORT_INDEX]);
    const OMX_PARAM_PORTDEFINITIONTYPE* port_def = port->GetPortDefinition();
     // if output port is disabled, retain the input buffer
    if (!port_def->bEnabled) {
        return false;
    }

    unsigned int i = 0;
    int found = 0;

    if (RAWDATA_MODE == mWorkingMode) {
        for (i = 0; i < OUTPORT_ACTUAL_BUFFER_COUNT; i++) {
            if (extMIDs[i]->m_released == true) {
               found ++;
               if (found > 1) { //libvpx sometimes needs 2 buffer when calling decode once.
                   return true;
               }
            }
        }
    } else { // graphic buffer mode
        for (i = 0; i < mOMXBufferHeaderTypePtrNum; i++) {
            if ((extMIDs[i]->m_render_done == true) && (extMIDs[i]->m_released == true)) {
               found ++;
               if (found > 1) { //libvpx sometimes needs 2 buffer when calling decode once.
                   return true;
               }
            }
        }
    }

    b = false;

    return b;
}

DECLARE_OMX_COMPONENT("OMX.Intel.VideoDecoder.VP9.hwr", "video_decoder.vp9", OMXVideoDecoderVP9HWR);

