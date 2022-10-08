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
#include "OMXVideoDecoderAVC.h"


// Be sure to have an equal string in VideoDecoderHost.cpp (libmix)
static const char* AVC_MIME_TYPE = "video/h264";
#define INVALID_PTS (OMX_S64)-1


OMXVideoDecoderAVC::OMXVideoDecoderAVC()
    : mAccumulateBuffer(NULL),
      mBufferSize(0),
      mFilledLen(0),
      mTimeStamp(INVALID_PTS) {
    LOGV("OMXVideoDecoderAVC is constructed.");
    mVideoDecoder = createVideoDecoder(AVC_MIME_TYPE);
    if (!mVideoDecoder) {
        LOGE("createVideoDecoder failed for \"%s\"", AVC_MIME_TYPE);
    }
    // Override default native buffer count defined in the base class
    mNativeBufferCount = OUTPORT_NATIVE_BUFFER_COUNT;
    BuildHandlerList();
}

OMXVideoDecoderAVC::~OMXVideoDecoderAVC() {
    LOGV("OMXVideoDecoderAVC is destructed.");
}

OMX_ERRORTYPE OMXVideoDecoderAVC::InitInputPortFormatSpecific(OMX_PARAM_PORTDEFINITIONTYPE *paramPortDefinitionInput) {
    //OMX_VIDEO_PARAM_INTEL_AVC_DECODE_SETTINGS
    memset(&mDecodeSettings, 0, sizeof(mDecodeSettings));
    SetTypeHeader(&mDecodeSettings, sizeof(mDecodeSettings));
    mDecodeSettings.nMaxNumberOfReferenceFrame = NUM_REFERENCE_FRAME;

    // OMX_PARAM_PORTDEFINITIONTYPE
    paramPortDefinitionInput->nBufferCountActual = INPORT_ACTUAL_BUFFER_COUNT;
    paramPortDefinitionInput->nBufferCountMin = INPORT_MIN_BUFFER_COUNT;
    paramPortDefinitionInput->nBufferSize = INPORT_BUFFER_SIZE;
    paramPortDefinitionInput->format.video.cMIMEType = (OMX_STRING)AVC_MIME_TYPE;
    paramPortDefinitionInput->format.video.eCompressionFormat = OMX_VIDEO_CodingAVC;

    // OMX_VIDEO_PARAM_AVCTYPE
    memset(&mParamAvc, 0, sizeof(mParamAvc));
    SetTypeHeader(&mParamAvc, sizeof(mParamAvc));
    mParamAvc.nPortIndex = INPORT_INDEX;
    // TODO: check eProfile/eLevel
    mParamAvc.eProfile = OMX_VIDEO_AVCProfileHigh; //OMX_VIDEO_AVCProfileBaseline;
    mParamAvc.eLevel = OMX_VIDEO_AVCLevel41; //OMX_VIDEO_AVCLevel1;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoDecoderAVC::ProcessorInit(void) {
    return OMXVideoDecoderBase::ProcessorInit();
}

OMX_ERRORTYPE OMXVideoDecoderAVC::ProcessorDeinit(void) {
    if (mAccumulateBuffer) {
        delete mAccumulateBuffer;
    }
    mAccumulateBuffer = NULL;
    mBufferSize = 0;
    mFilledLen = 0;
    mTimeStamp = INVALID_PTS;

    return OMXVideoDecoderBase::ProcessorDeinit();
}

OMX_ERRORTYPE OMXVideoDecoderAVC::ProcessorFlush(OMX_U32 portIndex) {
    mFilledLen = 0;
    mTimeStamp = INVALID_PTS;
    return OMXVideoDecoderBase::ProcessorFlush(portIndex);
}

OMX_ERRORTYPE OMXVideoDecoderAVC::ProcessorProcess(
        OMX_BUFFERHEADERTYPE ***pBuffers,
        buffer_retain_t *retains,
        OMX_U32 numberBuffers) {

    return OMXVideoDecoderBase::ProcessorProcess(pBuffers, retains, numberBuffers);
}

OMX_ERRORTYPE OMXVideoDecoderAVC::PrepareConfigBuffer(VideoConfigBuffer *p) {
    OMX_ERRORTYPE ret;

    ret = OMXVideoDecoderBase::PrepareConfigBuffer(p);
    CHECK_RETURN_VALUE("OMXVideoDecoderBase::PrepareConfigBuffer");

    if (mParamAvc.eProfile == OMX_VIDEO_AVCProfileBaseline) {
        p->flag |= WANT_LOW_DELAY;
    }

    if (mDecodeSettings.nMaxWidth == 0 ||
        mDecodeSettings.nMaxHeight == 0) {
        return OMX_ErrorNone;
    }

    LOGW("AVC Video decoder used in Video Conferencing Mode.");

    // For video conferencing application
    p->width = mDecodeSettings.nMaxWidth;
    p->height = mDecodeSettings.nMaxHeight;
    p->profile = VAProfileH264ConstrainedBaseline;
    if(!(p->flag & USE_NATIVE_GRAPHIC_BUFFER)) {
        p->surfaceNumber = mDecodeSettings.nMaxNumberOfReferenceFrame + EXTRA_REFERENCE_FRAME;
        p->flag = WANT_ERROR_CONCEALMENT | WANT_LOW_DELAY | HAS_SURFACE_NUMBER | HAS_VA_PROFILE;
    } else {
        p->flag |= WANT_ERROR_CONCEALMENT | WANT_LOW_DELAY | HAS_SURFACE_NUMBER | HAS_VA_PROFILE;
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoDecoderAVC::PrepareDecodeBuffer(OMX_BUFFERHEADERTYPE *buffer, buffer_retain_t *retain, VideoDecodeBuffer *p) {
    OMX_ERRORTYPE ret;
    ret = OMXVideoDecoderBase::PrepareDecodeBuffer(buffer, retain, p);
    CHECK_RETURN_VALUE("OMXVideoDecoderBase::PrepareDecodeBuffer");

    // OMX_BUFFERFLAG_CODECCONFIG is an optional flag
    // if flag is set, buffer will only contain codec data.
    if (buffer->nFlags & OMX_BUFFERFLAG_CODECCONFIG) {
        LOGV("Received AVC codec data.");
        return ret;
    }

    // OMX_BUFFERFLAG_ENDOFFRAME is an optional flag
    if (buffer->nFlags & (OMX_BUFFERFLAG_ENDOFFRAME | OMX_BUFFERFLAG_EOS)) {
        // TODO: if OMX_BUFFERFLAG_ENDOFFRAME indicates end of a NAL unit and in OMXVideoDecodeBase
        // we set buffer flag to HAS_COMPLETE_FRAME,  corruption will happen
        mTimeStamp = buffer->nTimeStamp;
        if (mFilledLen == 0) {
            // buffer is not accumulated and it contains a complete frame
            return ret;
        }
        // buffer contains  the last part of fragmented frame
        ret = AccumulateBuffer(buffer);
        CHECK_RETURN_VALUE("AccumulateBuffer");
        ret = FillDecodeBuffer(p);
        CHECK_RETURN_VALUE("FillDecodeBuffer");
        return ret;
    }

    LOGW("Received fragmented buffer.");
    // use time stamp to determine frame boundary
    if (mTimeStamp == INVALID_PTS) {
        // first ever buffer
        mTimeStamp = buffer->nTimeStamp;
    }

    if (mTimeStamp != buffer->nTimeStamp && mFilledLen != 0) {
        // buffer accumulated contains a complete frame
        ret = FillDecodeBuffer(p);
        CHECK_RETURN_VALUE("FillDecodeBuffer");
        // retain the current buffer
        *retain = BUFFER_RETAIN_GETAGAIN;
    } else {
        // buffer accumulation for beginning of fragmented buffer (mFilledLen == 0) or
        // middle/end of fragmented buffer (mFilledLen != 0)
        ret = AccumulateBuffer(buffer);
        CHECK_RETURN_VALUE("AccumulateBuffer");
        ret = OMX_ErrorNotReady;
    }

    if (buffer->nFilledLen != 0) {
        mTimeStamp = buffer->nTimeStamp;
    }
    return ret;
}

OMX_ERRORTYPE OMXVideoDecoderAVC::AccumulateBuffer(OMX_BUFFERHEADERTYPE *buffer) {
    // check if allocated buffer is big enough
    if (mFilledLen + buffer->nFilledLen > mBufferSize) {
        mBufferSize = mFilledLen + buffer->nFilledLen;
        if (mBufferSize < INPORT_BUFFER_SIZE) {
            mBufferSize = INPORT_BUFFER_SIZE;
        }
        if (mBufferSize == 0) {
            return OMX_ErrorBadParameter;
        }
        OMX_U8 *temp = new OMX_U8 [mBufferSize];
        if (temp == NULL) {
            mBufferSize = 0;
            return OMX_ErrorInsufficientResources;
        }
        if (mFilledLen != 0) {
            memcpy(temp, mAccumulateBuffer, mFilledLen);
        }
        if (mAccumulateBuffer) {
            delete [] mAccumulateBuffer;
        }
        mAccumulateBuffer = temp;
    }
    if (buffer->nFilledLen != 0) {
        memcpy(mAccumulateBuffer + mFilledLen, buffer->pBuffer + buffer->nOffset, buffer->nFilledLen);
    }
    mFilledLen += buffer->nFilledLen;
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoDecoderAVC::FillDecodeBuffer(VideoDecodeBuffer *p) {
    p->data = mAccumulateBuffer;
    p->size = mFilledLen;
    p->timeStamp = mTimeStamp;
    p->flag = HAS_COMPLETE_FRAME;

    mFilledLen = 0;
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoDecoderAVC::BuildHandlerList(void) {
    OMXVideoDecoderBase::BuildHandlerList();
    AddHandler(OMX_IndexParamVideoAvc, GetParamVideoAvc, SetParamVideoAvc);
    AddHandler((OMX_INDEXTYPE)OMX_IndexParamIntelAVCDecodeSettings, GetParamIntelAVCDecodeSettings, SetParamIntelAVCDecodeSettings);
    AddHandler(OMX_IndexParamVideoProfileLevelQuerySupported, GetParamVideoAVCProfileLevel, SetParamVideoAVCProfileLevel);
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoDecoderAVC::GetParamVideoAvc(OMX_PTR pStructure) {
    OMX_ERRORTYPE ret;
    OMX_VIDEO_PARAM_AVCTYPE *p = (OMX_VIDEO_PARAM_AVCTYPE *)pStructure;
    CHECK_TYPE_HEADER(p);
    CHECK_PORT_INDEX(p, INPORT_INDEX);

    memcpy(p, &mParamAvc, sizeof(*p));
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoDecoderAVC::SetParamVideoAvc(OMX_PTR pStructure) {
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

OMX_ERRORTYPE OMXVideoDecoderAVC::GetParamIntelAVCDecodeSettings(OMX_PTR) {
    return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE OMXVideoDecoderAVC::SetParamIntelAVCDecodeSettings(OMX_PTR pStructure) {
    LOGW("SetParamIntelAVCDecodeSettings");

    OMX_ERRORTYPE ret;
    OMX_VIDEO_PARAM_INTEL_AVC_DECODE_SETTINGS *p = (OMX_VIDEO_PARAM_INTEL_AVC_DECODE_SETTINGS *)pStructure;

    CHECK_TYPE_HEADER(p);
    CHECK_PORT_INDEX(p, INPORT_INDEX);
    CHECK_SET_PARAM_STATE();

    if(p->nMaxNumberOfReferenceFrame == 0) {
        // TODO: check if we just return in this case.
        p->nMaxNumberOfReferenceFrame = NUM_REFERENCE_FRAME;
    }
    LOGI("Maximum width = %u, height = %u, dpb = %u", p->nMaxWidth, p->nMaxHeight, p->nMaxNumberOfReferenceFrame);
    mDecodeSettings = *p;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoDecoderAVC::GetParamVideoAVCProfileLevel(OMX_PTR pStructure) {
    OMX_ERRORTYPE ret;
    OMX_VIDEO_PARAM_PROFILELEVELTYPE *p = (OMX_VIDEO_PARAM_PROFILELEVELTYPE *)pStructure;
    CHECK_TYPE_HEADER(p);
    CHECK_PORT_INDEX(p, INPORT_INDEX);

    struct ProfileLevelTable {
        OMX_U32 profile;
        OMX_U32 level;
    } plTable[] = {
        {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel42},
        {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel42},
        {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel42}
    };

    OMX_U32 count = sizeof(plTable)/sizeof(ProfileLevelTable);
    CHECK_ENUMERATION_RANGE(p->nProfileIndex,count);

    p->eProfile = plTable[p->nProfileIndex].profile;
    p->eLevel = plTable[p->nProfileIndex].level;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoDecoderAVC::SetParamVideoAVCProfileLevel(OMX_PTR) {
    LOGW("SetParamVideoAVCProfileLevel is not supported.");
    return OMX_ErrorUnsupportedSetting;
}

OMX_COLOR_FORMATTYPE OMXVideoDecoderAVC::GetOutputColorFormat(int width)
{
#ifdef USE_GEN_HW
    return OMX_INTEL_COLOR_FormatYUV420PackedSemiPlanar_Tiled;
#else
    return OMXVideoDecoderBase::GetOutputColorFormat(width);
#endif
}

OMX_ERRORTYPE OMXVideoDecoderAVC::SetMaxOutputBufferCount(OMX_PARAM_PORTDEFINITIONTYPE *p) {
    OMX_ERRORTYPE ret;
    CHECK_TYPE_HEADER(p);
    CHECK_PORT_INDEX(p, OUTPORT_INDEX);

    p->nBufferCountActual = MAX_OUTPORT_BUFFER_COUNT;
    return OMXVideoDecoderBase::SetMaxOutputBufferCount(p);
}

DECLARE_OMX_COMPONENT("OMX.Intel.VideoDecoder.AVC", "video_decoder.avc", OMXVideoDecoderAVC);
