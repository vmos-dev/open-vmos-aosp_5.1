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
#include "OMXVideoDecoderH263.h"

// Be sure to have an equal string in VideoDecoderHost.cpp (libmix)
static const char* H263_MIME_TYPE = "video/h263";

OMXVideoDecoderH263::OMXVideoDecoderH263() {
    LOGV("OMXVideoDecoderH263 is constructed.");
    mVideoDecoder = createVideoDecoder(H263_MIME_TYPE);
    if (!mVideoDecoder) {
        LOGE("createVideoDecoder failed for \"%s\"", H263_MIME_TYPE);
    }
    mNativeBufferCount = OUTPORT_NATIVE_BUFFER_COUNT;
    BuildHandlerList();
}

OMXVideoDecoderH263::~OMXVideoDecoderH263() {
    LOGV("OMXVideoDecoderH263 is destructed.");
}

OMX_ERRORTYPE OMXVideoDecoderH263::InitInputPortFormatSpecific(OMX_PARAM_PORTDEFINITIONTYPE *paramPortDefinitionInput) {
    // OMX_PARAM_PORTDEFINITIONTYPE
    paramPortDefinitionInput->nBufferCountActual = INPORT_ACTUAL_BUFFER_COUNT;
    paramPortDefinitionInput->nBufferCountMin = INPORT_MIN_BUFFER_COUNT;
    paramPortDefinitionInput->nBufferSize = INPORT_BUFFER_SIZE;
    paramPortDefinitionInput->format.video.cMIMEType = (OMX_STRING)H263_MIME_TYPE;
    paramPortDefinitionInput->format.video.eCompressionFormat = OMX_VIDEO_CodingH263;

    // OMX_VIDEO_PARAM_H263TYPE
    memset(&mParamH263, 0, sizeof(mParamH263));
    SetTypeHeader(&mParamH263, sizeof(mParamH263));
    mParamH263.nPortIndex = INPORT_INDEX;
    // TODO: check eProfile/eLevel
    mParamH263.eProfile = OMX_VIDEO_H263ProfileBaseline;
    mParamH263.eLevel = OMX_VIDEO_H263Level70; //OMX_VIDEO_H263Level10;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoDecoderH263::ProcessorInit(void) {
    return OMXVideoDecoderBase::ProcessorInit();
}

OMX_ERRORTYPE OMXVideoDecoderH263::ProcessorDeinit(void) {
    return OMXVideoDecoderBase::ProcessorDeinit();
}

OMX_ERRORTYPE OMXVideoDecoderH263::ProcessorProcess(
        OMX_BUFFERHEADERTYPE ***pBuffers,
        buffer_retain_t *retains,
        OMX_U32 numberBuffers) {

    return OMXVideoDecoderBase::ProcessorProcess(pBuffers, retains, numberBuffers);
}

OMX_ERRORTYPE OMXVideoDecoderH263::PrepareConfigBuffer(VideoConfigBuffer *p) {
    return OMXVideoDecoderBase::PrepareConfigBuffer(p);
}

OMX_ERRORTYPE OMXVideoDecoderH263::PrepareDecodeBuffer(OMX_BUFFERHEADERTYPE *buffer, buffer_retain_t *retain, VideoDecodeBuffer *p) {
    return OMXVideoDecoderBase::PrepareDecodeBuffer(buffer, retain, p);
}

OMX_ERRORTYPE OMXVideoDecoderH263::BuildHandlerList(void) {
    OMXVideoDecoderBase::BuildHandlerList();
    AddHandler(OMX_IndexParamVideoH263, GetParamVideoH263, SetParamVideoH263);
    AddHandler(OMX_IndexParamVideoProfileLevelQuerySupported, GetParamVideoH263ProfileLevel, SetParamVideoH263ProfileLevel);
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoDecoderH263::GetParamVideoH263(OMX_PTR pStructure) {
    OMX_ERRORTYPE ret;
    OMX_VIDEO_PARAM_H263TYPE *p = (OMX_VIDEO_PARAM_H263TYPE *)pStructure;
    CHECK_TYPE_HEADER(p);
    CHECK_PORT_INDEX(p, INPORT_INDEX);

    memcpy(p, &mParamH263, sizeof(*p));
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoDecoderH263::SetParamVideoH263(OMX_PTR pStructure) {
    OMX_ERRORTYPE ret;
    OMX_VIDEO_PARAM_H263TYPE *p = (OMX_VIDEO_PARAM_H263TYPE *)pStructure;
    CHECK_TYPE_HEADER(p);
    CHECK_PORT_INDEX(p, INPORT_INDEX);
    CHECK_SET_PARAM_STATE();

    // TODO: do we need to check if port is enabled?
    // TODO: see SetPortH263Param implementation - Can we make simple copy????
    memcpy(&mParamH263, p, sizeof(mParamH263));
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoDecoderH263::GetParamVideoH263ProfileLevel(OMX_PTR pStructure) {
    OMX_ERRORTYPE ret;
    OMX_VIDEO_PARAM_PROFILELEVELTYPE *p = (OMX_VIDEO_PARAM_PROFILELEVELTYPE *)pStructure;
    CHECK_TYPE_HEADER(p);

    struct ProfileLevelTable {
        OMX_U32 profile;
        OMX_U32 level;
    } plTable[] = {
        {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level70}
    };

    OMX_U32 count = sizeof(plTable)/sizeof(ProfileLevelTable);
    CHECK_ENUMERATION_RANGE(p->nProfileIndex,count);

    p->eProfile = plTable[p->nProfileIndex].profile;
    p->eLevel = plTable[p->nProfileIndex].level;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoDecoderH263::SetParamVideoH263ProfileLevel(OMX_PTR) {
    LOGW("SetParamVideoH263ProfileLevel is not supported.");
    return OMX_ErrorUnsupportedSetting;
}

OMX_COLOR_FORMATTYPE OMXVideoDecoderH263::GetOutputColorFormat(int width)
{
#ifdef USE_GEN_HW
    return (OMX_COLOR_FORMATTYPE)HAL_PIXEL_FORMAT_NV12_X_TILED_INTEL;
#else
    return OMXVideoDecoderBase::GetOutputColorFormat(width);
#endif
}

OMX_ERRORTYPE OMXVideoDecoderH263::SetMaxOutputBufferCount(OMX_PARAM_PORTDEFINITIONTYPE *p) {
    OMX_ERRORTYPE ret;
    CHECK_TYPE_HEADER(p);
    CHECK_PORT_INDEX(p, OUTPORT_INDEX);

    p->nBufferCountActual = OUTPORT_NATIVE_BUFFER_COUNT;
    return OMXVideoDecoderBase::SetMaxOutputBufferCount(p);
}

DECLARE_OMX_COMPONENT("OMX.Intel.VideoDecoder.H263", "video_decoder.h263", OMXVideoDecoderH263);

