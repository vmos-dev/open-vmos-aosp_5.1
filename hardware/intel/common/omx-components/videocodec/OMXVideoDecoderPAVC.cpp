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


//#define LOG_NDEBUG 0
#define LOG_TAG "OMXVideoDecoder"
#include <wrs_omxil_core/log.h>
#include "OMXVideoDecoderPAVC.h"

// Be sure to have an equal string in VideoDecoderHost.cpp (libmix)
static const char* PAVC_MIME_TYPE = "video/PAVC";
#define INVALID_PTS (OMX_S64)-1


OMXVideoDecoderPAVC::OMXVideoDecoderPAVC() {
    LOGV("OMXVideoDecoderPAVC is constructed.");
    mVideoDecoder = createVideoDecoder(PAVC_MIME_TYPE);
    if (!mVideoDecoder) {
        LOGE("createVideoDecoder failed for \"%s\"", PAVC_MIME_TYPE);
    }

    BuildHandlerList();
}

OMXVideoDecoderPAVC::~OMXVideoDecoderPAVC() {
    LOGV("OMXVideoDecoderPAVC is destructed.");
}

OMX_ERRORTYPE OMXVideoDecoderPAVC::InitInputPortFormatSpecific(OMX_PARAM_PORTDEFINITIONTYPE *paramPortDefinitionInput) {
    // OMX_PARAM_PORTDEFINITIONTYPE
    paramPortDefinitionInput->nBufferCountActual = INPORT_ACTUAL_BUFFER_COUNT;
    paramPortDefinitionInput->nBufferCountMin = INPORT_MIN_BUFFER_COUNT;
    paramPortDefinitionInput->nBufferSize = INPORT_BUFFER_SIZE;
    paramPortDefinitionInput->format.video.cMIMEType = (OMX_STRING)PAVC_MIME_TYPE;
    paramPortDefinitionInput->format.video.eCompressionFormat = OMX_VIDEO_CodingAVC;

    // OMX_VIDEO_PARAM_AVCTYPE
    memset(&mParamAvc, 0, sizeof(mParamAvc));
    SetTypeHeader(&mParamAvc, sizeof(mParamAvc));
    mParamAvc.nPortIndex = INPORT_INDEX;
    // TODO: check eProfile/eLevel
    mParamAvc.eProfile = OMX_VIDEO_AVCProfileMain; //OMX_VIDEO_AVCProfileBaseline;
    mParamAvc.eLevel = OMX_VIDEO_AVCLevel41; //OMX_VIDEO_AVCLevel1;

    mCurrentProfile = mParamAvc.eProfile;
    mCurrentLevel = mParamAvc.eLevel;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoDecoderPAVC::ProcessorInit(void) {
    return OMXVideoDecoderBase::ProcessorInit();
}

OMX_ERRORTYPE OMXVideoDecoderPAVC::ProcessorDeinit(void) {
    return OMXVideoDecoderBase::ProcessorDeinit();
}

OMX_ERRORTYPE OMXVideoDecoderPAVC::ProcessorFlush(OMX_U32 portIndex) {
    return OMXVideoDecoderBase::ProcessorFlush(portIndex);
}

OMX_ERRORTYPE OMXVideoDecoderPAVC::ProcessorProcess(
        OMX_BUFFERHEADERTYPE ***pBuffers,
        buffer_retain_t *retains,
        OMX_U32 numberBuffers) {

    return OMXVideoDecoderBase::ProcessorProcess(pBuffers, retains, numberBuffers);
}

OMX_ERRORTYPE OMXVideoDecoderPAVC::PrepareConfigBuffer(VideoConfigBuffer *p) {
    OMX_ERRORTYPE ret;
    ret = OMXVideoDecoderBase::PrepareConfigBuffer(p);
    CHECK_RETURN_VALUE("OMXVideoDecoderBase::PrepareConfigBuffer");
    p->width = 1920;
    p->height = 1088;
    p->surfaceNumber = 16;
    p->profile = VAProfileH264High;
    p->flag =  WANT_SURFACE_PROTECTION | HAS_VA_PROFILE | HAS_SURFACE_NUMBER;
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoDecoderPAVC::PrepareDecodeBuffer(OMX_BUFFERHEADERTYPE *buffer, buffer_retain_t *retain, VideoDecodeBuffer *p) {
    OMX_ERRORTYPE ret;
    ret = OMXVideoDecoderBase::PrepareDecodeBuffer(buffer, retain, p);
    CHECK_RETURN_VALUE("OMXVideoDecoderBase::PrepareDecodeBuffer");

    // OMX_BUFFERFLAG_CODECCONFIG is an optional flag
    // if flag is set, buffer will only contain codec data.
    if (buffer->nFlags & OMX_BUFFERFLAG_CODECCONFIG) {
        LOGV("Received codec data for Protected AVC.");
        return ret;
    }

    if (buffer->nFlags & OMX_BUFFERFLAG_EXTRADATA) {
        p->flag |= HAS_EXTRADATA;
    } else {
        LOGW("No extra data found.");
    }
    return ret;
}

OMX_ERRORTYPE OMXVideoDecoderPAVC::BuildHandlerList(void) {
    OMXVideoDecoderBase::BuildHandlerList();
    AddHandler(OMX_IndexParamVideoAvc, GetParamVideoAvc, SetParamVideoAvc);
    AddHandler(OMX_IndexParamVideoProfileLevelQuerySupported, GetVideoProfileLevelQuerySupported, SetVideoProfileLevelQuerySupported);
    AddHandler(OMX_IndexParamVideoProfileLevelCurrent, GetVideoProfileLevelCurrent, SetVideoProfileLevelCurrent);
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoDecoderPAVC::GetParamVideoAvc(OMX_PTR pStructure) {
    OMX_ERRORTYPE ret;
    OMX_VIDEO_PARAM_AVCTYPE *p = (OMX_VIDEO_PARAM_AVCTYPE *)pStructure;
    CHECK_TYPE_HEADER(p);
    CHECK_PORT_INDEX(p, INPORT_INDEX);

    memcpy(p, &mParamAvc, sizeof(*p));
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoDecoderPAVC::SetParamVideoAvc(OMX_PTR pStructure) {
    OMX_ERRORTYPE ret;
    OMX_VIDEO_PARAM_AVCTYPE *p = (OMX_VIDEO_PARAM_AVCTYPE *)pStructure;
    CHECK_TYPE_HEADER(p);
    CHECK_PORT_INDEX(p, INPORT_INDEX);
    CHECK_SET_PARAM_STATE();

    // TODO: do we need to check if port is enabled?
    // TODO: see SetPortAvcParam implementation - Can we make simple copy????
    memcpy(&mParamAvc, p, sizeof(mParamAvc));
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoDecoderPAVC::GetVideoProfileLevelQuerySupported(OMX_PTR pStructure) {
    OMX_ERRORTYPE ret;
    OMX_VIDEO_PARAM_PROFILELEVELTYPE *p = (OMX_VIDEO_PARAM_PROFILELEVELTYPE *)pStructure;

    CHECK_TYPE_HEADER(p);
    CHECK_PORT_INDEX_RANGE(p);

    if (p->nProfileIndex != 0) {
        LOGE("No more profile index for GetVideoProfileLevelQuerySupported.");
        return OMX_ErrorNoMore;
    }
    p->eProfile = mParamAvc.eProfile;
    p->eLevel = mParamAvc.eLevel;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoDecoderPAVC::SetVideoProfileLevelQuerySupported(OMX_PTR) {
    LOGE("SetVideoProfileLevelQuerySupported is not supported.");
    return OMX_ErrorUnsupportedSetting;
}

OMX_ERRORTYPE OMXVideoDecoderPAVC::GetVideoProfileLevelCurrent(OMX_PTR pStructure) {
    OMX_ERRORTYPE ret;
    OMX_VIDEO_PARAM_PROFILELEVELTYPE *p = (OMX_VIDEO_PARAM_PROFILELEVELTYPE *)pStructure;

    CHECK_TYPE_HEADER(p);
    CHECK_PORT_INDEX_RANGE(p);

    if (p->nProfileIndex != 0) {
        LOGE("No more profile index for GetVideoProfileLevelCurrent.");
        return OMX_ErrorNoMore;
    }

    p->eProfile = mCurrentProfile;
    p->eLevel = mCurrentLevel;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoDecoderPAVC::SetVideoProfileLevelCurrent(OMX_PTR pStructure) {
    OMX_ERRORTYPE ret;
    OMX_VIDEO_PARAM_PROFILELEVELTYPE *p = (OMX_VIDEO_PARAM_PROFILELEVELTYPE *)pStructure;

    CHECK_TYPE_HEADER(p);
    CHECK_PORT_INDEX_RANGE(p);

    if (p->nProfileIndex != 0) {
        LOGE("Invalid profile index for SetVideoProfileLevelCurrent.");
        return OMX_ErrorBadParameter;
    }

    mCurrentProfile = (OMX_VIDEO_AVCPROFILETYPE) p->eProfile;
    mCurrentLevel = (OMX_VIDEO_AVCLEVELTYPE) p->eLevel;

    return OMX_ErrorNone;
}

DECLARE_OMX_COMPONENT("OMX.Intel.VideoDecoder.PAVC", "video_decoder.pavc", OMXVideoDecoderPAVC);

