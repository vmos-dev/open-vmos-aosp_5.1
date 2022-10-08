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


#define LOG_TAG "OMXVideoEncoderMPEG4"
#include "OMXVideoEncoderMPEG4.h"

static const char *MPEG4_MIME_TYPE = "video/mpeg4";

OMXVideoEncoderMPEG4::OMXVideoEncoderMPEG4() {
    LOGV("OMXVideoEncoderMPEG4 is constructed.");
    BuildHandlerList();
    mVideoEncoder = createVideoEncoder(MPEG4_MIME_TYPE);
    if (!mVideoEncoder) LOGE("OMX_ErrorInsufficientResources");
}

OMXVideoEncoderMPEG4::~OMXVideoEncoderMPEG4() {
    LOGV("OMXVideoEncoderMPEG4 is destructed.");
}

OMX_ERRORTYPE OMXVideoEncoderMPEG4::InitOutputPortFormatSpecific(OMX_PARAM_PORTDEFINITIONTYPE *paramPortDefinitionOutput) {
    // OMX_VIDEO_PARAM_MPEG4TYPE
    memset(&mParamMpeg4, 0, sizeof(mParamMpeg4));
    SetTypeHeader(&mParamMpeg4, sizeof(mParamMpeg4));
    mParamMpeg4.nPortIndex = OUTPORT_INDEX;
    mParamMpeg4.eProfile = OMX_VIDEO_MPEG4ProfileSimple;
    // TODO: Check eLevel (Level3)
    mParamMpeg4.eLevel = OMX_VIDEO_MPEG4Level5; //OMX_VIDEO_MPEG4Level3;

    // override OMX_PARAM_PORTDEFINITIONTYPE
    paramPortDefinitionOutput->nBufferCountActual = OUTPORT_ACTUAL_BUFFER_COUNT;
    paramPortDefinitionOutput->nBufferCountMin = OUTPORT_MIN_BUFFER_COUNT;
    paramPortDefinitionOutput->nBufferSize = OUTPORT_BUFFER_SIZE;
    paramPortDefinitionOutput->format.video.cMIMEType = (OMX_STRING)MPEG4_MIME_TYPE;
    paramPortDefinitionOutput->format.video.eCompressionFormat = OMX_VIDEO_CodingMPEG4;

    // override OMX_VIDEO_PARAM_PROFILELEVELTYPE
    // TODO: check if profile/level supported is correct
    mParamProfileLevel.eProfile = mParamMpeg4.eProfile;
    mParamProfileLevel.eLevel = mParamMpeg4.eLevel; //OMX_VIDEO_MPEG4Level5;

    // override OMX_VIDEO_CONFIG_INTEL_BITRATETYPE
    mConfigIntelBitrate.nInitialQP = 15;  // Initial QP for I frames
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoEncoderMPEG4::SetVideoEncoderParam(void) {

    if (!mEncoderParams) {
        LOGE("NULL pointer: mEncoderParams");
        return OMX_ErrorBadParameter;
    }

    mVideoEncoder->getParameters(mEncoderParams);
    mEncoderParams->profile = (VAProfile)PROFILE_MPEG4SIMPLE;
    return OMXVideoEncoderBase::SetVideoEncoderParam();
}

OMX_ERRORTYPE OMXVideoEncoderMPEG4::ProcessorInit(void) {
    return OMXVideoEncoderBase::ProcessorInit();
}

OMX_ERRORTYPE OMXVideoEncoderMPEG4::ProcessorDeinit(void) {
    return OMXVideoEncoderBase::ProcessorDeinit();
}

OMX_ERRORTYPE OMXVideoEncoderMPEG4::ProcessorProcess(
    OMX_BUFFERHEADERTYPE **buffers,
    buffer_retain_t *retains,
    OMX_U32) {

    VideoEncOutputBuffer outBuf;
    VideoEncRawBuffer inBuf;
    Encode_Status ret = ENCODE_SUCCESS;

    OMX_U32 outfilledlen = 0;
    OMX_S64 outtimestamp = 0;
    OMX_U32 outflags = 0;
    OMX_ERRORTYPE oret = OMX_ErrorNone;


    LOGV_IF(buffers[INPORT_INDEX]->nFlags & OMX_BUFFERFLAG_EOS,
            "%s(),%d: got OMX_BUFFERFLAG_EOS\n", __func__, __LINE__);

    if (!buffers[INPORT_INDEX]->nFilledLen) {
        LOGV("%s(),%d: input buffer's nFilledLen is zero\n",  __func__, __LINE__);
        goto out;
    }

    inBuf.data = buffers[INPORT_INDEX]->pBuffer + buffers[INPORT_INDEX]->nOffset;
    inBuf.size = buffers[INPORT_INDEX]->nFilledLen;
    inBuf.type = FTYPE_UNKNOWN;
    inBuf.flag = 0;
    inBuf.timeStamp = buffers[INPORT_INDEX]->nTimeStamp;

    LOGV("inBuf.data=%x, size=%d", (unsigned)inBuf.data, inBuf.size);

    outBuf.data =
        buffers[OUTPORT_INDEX]->pBuffer + buffers[OUTPORT_INDEX]->nOffset;
    outBuf.dataSize = 0;
    outBuf.bufferSize = buffers[OUTPORT_INDEX]->nAllocLen - buffers[OUTPORT_INDEX]->nOffset;


    if (mFrameRetrieved) {
        // encode and setConfig need to be thread safe
        pthread_mutex_unlock(&mSerializationLock);
        ret = mVideoEncoder->encode(&inBuf);
        pthread_mutex_unlock(&mSerializationLock);

        CHECK_ENCODE_STATUS("encode");
        mFrameRetrieved = OMX_FALSE;

        // This is for buffer contention, we won't release current buffer
        // but the last input buffer
        ports[INPORT_INDEX]->ReturnAllRetainedBuffers();
    }

    if (mFirstFrame) {
        LOGV("mFirstFrame\n");
        outBuf.format = OUTPUT_CODEC_DATA;
        ret = mVideoEncoder->getOutput(&outBuf);
        CHECK_ENCODE_STATUS("getOutput");
        // Return code could not be ENCODE_BUFFER_TOO_SMALL
        // If we don't return error, we will have dead lock issue
        if (ret == ENCODE_BUFFER_TOO_SMALL) {
            return OMX_ErrorUndefined;
        }

        LOGV("output codec data size = %d", outBuf.dataSize);

        outflags |= OMX_BUFFERFLAG_CODECCONFIG;
        outflags |= OMX_BUFFERFLAG_ENDOFFRAME;
        outflags |= OMX_BUFFERFLAG_SYNCFRAME;

        retains[INPORT_INDEX] = BUFFER_RETAIN_GETAGAIN;
        outfilledlen = outBuf.dataSize;
        mFirstFrame = OMX_FALSE;
    } else {
        if (mSyncEncoding == OMX_FALSE && mFrameInputCount == 1) {
            retains[INPORT_INDEX] = BUFFER_RETAIN_ACCUMULATE;
            retains[OUTPORT_INDEX] = BUFFER_RETAIN_GETAGAIN;
            mFrameRetrieved = OMX_TRUE;
            goto out;
        }

        outBuf.format = OUTPUT_EVERYTHING;
        mVideoEncoder->getOutput(&outBuf);
        CHECK_ENCODE_STATUS("getOutput");

        LOGV("output data size = %d", outBuf.dataSize);


        outfilledlen = outBuf.dataSize;
        outtimestamp = outBuf.timeStamp;

        if (outBuf.flag & ENCODE_BUFFERFLAG_SYNCFRAME) {
            outflags |= OMX_BUFFERFLAG_SYNCFRAME;
        }

        if (outBuf.flag & ENCODE_BUFFERFLAG_ENDOFFRAME) {
            LOGV("Get buffer done\n");
            outflags |= OMX_BUFFERFLAG_ENDOFFRAME;
            mFrameRetrieved = OMX_TRUE;
            if (mSyncEncoding)
                retains[INPORT_INDEX] = BUFFER_RETAIN_NOT_RETAIN;
            else
                retains[INPORT_INDEX] = BUFFER_RETAIN_ACCUMULATE;

        } else {
            retains[INPORT_INDEX] = BUFFER_RETAIN_GETAGAIN;  //get again

        }

    }

    if (outfilledlen > 0) {
        retains[OUTPORT_INDEX] = BUFFER_RETAIN_NOT_RETAIN;
    } else {
        retains[OUTPORT_INDEX] = BUFFER_RETAIN_GETAGAIN;
    }



#if SHOW_FPS
    {
        struct timeval t;
        OMX_TICKS current_ts, interval_ts;
        float current_fps, average_fps;

        t.tv_sec = t.tv_usec = 0;
        gettimeofday(&t, NULL);

        current_ts =
            (nsecs_t)t.tv_sec * 1000000000 + (nsecs_t)t.tv_usec * 1000;
        interval_ts = current_ts - lastTs;
        lastTs = current_ts;

        current_fps = (float)1000000000 / (float)interval_ts;
        average_fps = (current_fps + lastFps) / 2;
        lastFps = current_fps;

        LOGV("FPS = %2.1f\n", average_fps);
    }
#endif

out:

    if (retains[OUTPORT_INDEX] != BUFFER_RETAIN_GETAGAIN) {
        buffers[OUTPORT_INDEX]->nFilledLen = outfilledlen;
        buffers[OUTPORT_INDEX]->nTimeStamp = outtimestamp;
        buffers[OUTPORT_INDEX]->nFlags = outflags;
    }

    if (retains[INPORT_INDEX] == BUFFER_RETAIN_NOT_RETAIN ||
            retains[INPORT_INDEX] == BUFFER_RETAIN_ACCUMULATE ) {
        mFrameInputCount ++;
    }

    if (retains[OUTPORT_INDEX] == BUFFER_RETAIN_NOT_RETAIN)
        mFrameOutputCount ++;

    return oret;

}

OMX_ERRORTYPE OMXVideoEncoderMPEG4::BuildHandlerList(void) {
    OMXVideoEncoderBase::BuildHandlerList();
    AddHandler(OMX_IndexParamVideoMpeg4, GetParamVideoMpeg4, SetParamVideoMpeg4);
    AddHandler(OMX_IndexParamVideoProfileLevelQuerySupported, GetParamVideoProfileLevelQuerySupported, SetParamVideoProfileLevelQuerySupported);
    return OMX_ErrorNone;
}


OMX_ERRORTYPE OMXVideoEncoderMPEG4::GetParamVideoProfileLevelQuerySupported(OMX_PTR pStructure) {
    OMX_ERRORTYPE ret;
    OMX_VIDEO_PARAM_PROFILELEVELTYPE *p = (OMX_VIDEO_PARAM_PROFILELEVELTYPE *)pStructure;
    CHECK_TYPE_HEADER(p);
    CHECK_PORT_INDEX(p, OUTPORT_INDEX);

    struct ProfileLevelTable {
        OMX_U32 profile;
        OMX_U32 level;
    } plTable[] = {
        {OMX_VIDEO_MPEG4ProfileSimple, OMX_VIDEO_MPEG4Level5},
    };

    OMX_U32 count = sizeof(plTable)/sizeof(ProfileLevelTable);
    CHECK_ENUMERATION_RANGE(p->nProfileIndex,count);

    p->eProfile = plTable[p->nProfileIndex].profile;
    p->eLevel = plTable[p->nProfileIndex].level;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoEncoderMPEG4::SetParamVideoProfileLevelQuerySupported(OMX_PTR) {
    LOGW("SetParamVideoMpeg4ProfileLevel is not supported.");
    return OMX_ErrorUnsupportedSetting;
}

OMX_ERRORTYPE OMXVideoEncoderMPEG4::GetParamVideoMpeg4(OMX_PTR pStructure) {
    OMX_ERRORTYPE ret;
    OMX_VIDEO_PARAM_MPEG4TYPE *p = (OMX_VIDEO_PARAM_MPEG4TYPE *)pStructure;
    CHECK_TYPE_HEADER(p);
    CHECK_PORT_INDEX(p, OUTPORT_INDEX);

    memcpy(p, &mParamMpeg4, sizeof(*p));
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoEncoderMPEG4::SetParamVideoMpeg4(OMX_PTR pStructure) {
    OMX_ERRORTYPE ret;
    OMX_VIDEO_PARAM_MPEG4TYPE *p = (OMX_VIDEO_PARAM_MPEG4TYPE *)pStructure;
    CHECK_TYPE_HEADER(p);
    CHECK_PORT_INDEX(p, OUTPORT_INDEX);
    CHECK_SET_PARAM_STATE();

    // TODO: do we need to check if port is enabled?
    // TODO: see SetPortMpeg4Param implementation - Can we make simple copy????
    memcpy(&mParamMpeg4, p, sizeof(mParamMpeg4));
    return OMX_ErrorNone;
}


DECLARE_OMX_COMPONENT("OMX.Intel.VideoEncoder.MPEG4", "video_encoder.mpeg4", OMXVideoEncoderMPEG4);


