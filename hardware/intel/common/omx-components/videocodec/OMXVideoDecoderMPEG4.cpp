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


// #define LOG_NDEBUG 0
#define LOG_TAG "OMXVideoDecoder"
#include <wrs_omxil_core/log.h>
#include "OMXVideoDecoderMPEG4.h"

// Be sure to have an equal string in VideoDecoderHost.cpp (libmix)
static const char* MPEG4_MIME_TYPE = "video/mpeg4";

OMXVideoDecoderMPEG4::OMXVideoDecoderMPEG4() {
    LOGV("OMXVideoDecoderMPEG4 is constructed.");
    mVideoDecoder = createVideoDecoder(MPEG4_MIME_TYPE);
    if (!mVideoDecoder) {
        LOGE("createVideoDecoder failed for \"%s\"", MPEG4_MIME_TYPE);
    }
    mNativeBufferCount = OUTPORT_NATIVE_BUFFER_COUNT;
    BuildHandlerList();
}

OMXVideoDecoderMPEG4::~OMXVideoDecoderMPEG4() {
    LOGV("OMXVideoDecoderMPEG4 is destructed.");
}

OMX_ERRORTYPE OMXVideoDecoderMPEG4::InitInputPortFormatSpecific(OMX_PARAM_PORTDEFINITIONTYPE *paramPortDefinitionInput) {
    // OMX_PARAM_PORTDEFINITIONTYPE
    paramPortDefinitionInput->nBufferCountActual = INPORT_ACTUAL_BUFFER_COUNT;
    paramPortDefinitionInput->nBufferCountMin = INPORT_MIN_BUFFER_COUNT;
    paramPortDefinitionInput->nBufferSize = INPORT_BUFFER_SIZE;
    paramPortDefinitionInput->format.video.cMIMEType = (OMX_STRING)MPEG4_MIME_TYPE;
    paramPortDefinitionInput->format.video.eCompressionFormat = OMX_VIDEO_CodingMPEG4;

    // OMX_VIDEO_PARAM_MPEG4TYPE
    memset(&mParamMpeg4, 0, sizeof(mParamMpeg4));
    SetTypeHeader(&mParamMpeg4, sizeof(mParamMpeg4));
    mParamMpeg4.nPortIndex = INPORT_INDEX;
    // TODO: check eProfile/eLevel
    mParamMpeg4.eProfile = OMX_VIDEO_MPEG4ProfileAdvancedSimple; //OMX_VIDEO_MPEG4ProfileSimple;
    mParamMpeg4.eLevel = OMX_VIDEO_MPEG4Level3;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoDecoderMPEG4::ProcessorInit(void) {
    return OMXVideoDecoderBase::ProcessorInit();
}

OMX_ERRORTYPE OMXVideoDecoderMPEG4::ProcessorDeinit(void) {
    return OMXVideoDecoderBase::ProcessorDeinit();
}

OMX_ERRORTYPE OMXVideoDecoderMPEG4::ProcessorProcess(
        OMX_BUFFERHEADERTYPE ***pBuffers,
        buffer_retain_t *retains,
        OMX_U32 numberBuffers) {

    return OMXVideoDecoderBase::ProcessorProcess(pBuffers, retains, numberBuffers);
}

OMX_ERRORTYPE OMXVideoDecoderMPEG4::PrepareConfigBuffer(VideoConfigBuffer *p) {
    return OMXVideoDecoderBase::PrepareConfigBuffer(p);
}

OMX_ERRORTYPE OMXVideoDecoderMPEG4::PrepareDecodeBuffer(OMX_BUFFERHEADERTYPE *buffer, buffer_retain_t *retain, VideoDecodeBuffer *p) {
    return OMXVideoDecoderBase::PrepareDecodeBuffer(buffer, retain, p);
}

OMX_ERRORTYPE OMXVideoDecoderMPEG4::BuildHandlerList(void) {
    OMXVideoDecoderBase::BuildHandlerList();
    AddHandler(OMX_IndexParamVideoMpeg4, GetParamVideoMpeg4, SetParamVideoMpeg4);
    AddHandler(OMX_IndexParamVideoProfileLevelQuerySupported, GetParamVideoMpeg4ProfileLevel, SetParamVideoMpeg4ProfileLevel);
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoDecoderMPEG4::GetParamVideoMpeg4(OMX_PTR pStructure) {
    OMX_ERRORTYPE ret;
    OMX_VIDEO_PARAM_MPEG4TYPE *p = (OMX_VIDEO_PARAM_MPEG4TYPE *)pStructure;
    CHECK_TYPE_HEADER(p);
    CHECK_PORT_INDEX(p, INPORT_INDEX);

    memcpy(p, &mParamMpeg4, sizeof(*p));
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoDecoderMPEG4::SetParamVideoMpeg4(OMX_PTR pStructure) {
    OMX_ERRORTYPE ret;
    OMX_VIDEO_PARAM_MPEG4TYPE *p = (OMX_VIDEO_PARAM_MPEG4TYPE *)pStructure;
    CHECK_TYPE_HEADER(p);
    CHECK_PORT_INDEX(p, INPORT_INDEX);
    CHECK_SET_PARAM_STATE();

    // TODO: do we need to check if port is enabled?
    // TODO: see SetPortMpeg4Param implementation - Can we make simple copy????
    memcpy(&mParamMpeg4, p, sizeof(mParamMpeg4));
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoDecoderMPEG4::GetParamVideoMpeg4ProfileLevel(OMX_PTR pStructure) {
    OMX_ERRORTYPE ret;
    OMX_VIDEO_PARAM_PROFILELEVELTYPE *p = (OMX_VIDEO_PARAM_PROFILELEVELTYPE *)pStructure;
    CHECK_TYPE_HEADER(p);

    struct ProfileLevelTable {
        OMX_U32 profile;
        OMX_U32 level;
    } plTable[] = {
        {OMX_VIDEO_MPEG4ProfileSimple, OMX_VIDEO_MPEG4Level3},
        {OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level5}
    };

    OMX_U32 count = sizeof(plTable)/sizeof(ProfileLevelTable);
    CHECK_ENUMERATION_RANGE(p->nProfileIndex,count);

    p->eProfile = plTable[p->nProfileIndex].profile;
    p->eLevel = plTable[p->nProfileIndex].level;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoDecoderMPEG4::SetParamVideoMpeg4ProfileLevel(OMX_PTR) {
    LOGW("SetParamVideoMpeg4ProfileLevel is not supported.");
    return OMX_ErrorUnsupportedSetting;
}

OMX_COLOR_FORMATTYPE OMXVideoDecoderMPEG4::GetOutputColorFormat(int width)
{
#ifdef USE_GEN_HW
    return (OMX_COLOR_FORMATTYPE)HAL_PIXEL_FORMAT_NV12_X_TILED_INTEL;
#else
    return OMXVideoDecoderBase::GetOutputColorFormat(width);
#endif
}

OMX_ERRORTYPE OMXVideoDecoderMPEG4::SetMaxOutputBufferCount(OMX_PARAM_PORTDEFINITIONTYPE *p) {
    OMX_ERRORTYPE ret;
    CHECK_TYPE_HEADER(p);
    CHECK_PORT_INDEX(p, OUTPORT_INDEX);

    p->nBufferCountActual = OUTPORT_NATIVE_BUFFER_COUNT;
    return OMXVideoDecoderBase::SetMaxOutputBufferCount(p);
}

DECLARE_OMX_COMPONENT("OMX.Intel.VideoDecoder.MPEG4", "video_decoder.mpeg4", OMXVideoDecoderMPEG4);


