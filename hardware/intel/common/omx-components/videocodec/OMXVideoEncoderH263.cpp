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


#define LOG_TAG "OMXVideoEncoderH263"
#include "OMXVideoEncoderH263.h"

static const char *H263_MIME_TYPE = "video/h263";

OMXVideoEncoderH263::OMXVideoEncoderH263() {
    LOGV("Constructer for OMXVideoEncoderH263.");
    BuildHandlerList();
    mVideoEncoder = createVideoEncoder(H263_MIME_TYPE);
    if (!mVideoEncoder) LOGE("OMX_ErrorInsufficientResources");
#ifdef SYNC_MODE
    mSyncEncoding = OMX_TRUE;
#endif
}

OMXVideoEncoderH263::~OMXVideoEncoderH263() {
    LOGV("Destructer for OMXVideoEncoderH263.");
}

OMX_ERRORTYPE OMXVideoEncoderH263::InitOutputPortFormatSpecific(OMX_PARAM_PORTDEFINITIONTYPE *paramPortDefinitionOutput) {
    // OMX_VIDEO_PARAM_H263TYPE
    memset(&mParamH263, 0, sizeof(mParamH263));
    SetTypeHeader(&mParamH263, sizeof(mParamH263));
    mParamH263.nPortIndex = OUTPORT_INDEX;
    mParamH263.eProfile = OMX_VIDEO_H263ProfileBaseline;
    // TODO: check eLevel, 10
    mParamH263.eLevel = OMX_VIDEO_H263Level45; //OMX_VIDEO_H263Level10;

    // override OMX_PARAM_PORTDEFINITIONTYPE
    paramPortDefinitionOutput->nBufferCountActual = OUTPORT_ACTUAL_BUFFER_COUNT;
    paramPortDefinitionOutput->nBufferCountMin = OUTPORT_MIN_BUFFER_COUNT;
    paramPortDefinitionOutput->nBufferSize = OUTPORT_BUFFER_SIZE;
    paramPortDefinitionOutput->format.video.cMIMEType = (OMX_STRING)H263_MIME_TYPE;
    paramPortDefinitionOutput->format.video.eCompressionFormat = OMX_VIDEO_CodingH263;

    // override OMX_VIDEO_PARAM_PROFILELEVELTYPE
    // TODO: check if profile/level supported is correct
    mParamProfileLevel.eProfile = mParamH263.eProfile;
    mParamProfileLevel.eLevel = mParamH263.eLevel; //OMX_VIDEO_H263Level70

    // override OMX_VIDEO_PARAM_BITRATETYPE
    mParamBitrate.nTargetBitrate = 64000;

    // override OMX_VIDEO_CONFIG_INTEL_BITRATETYPE
    mConfigIntelBitrate.nInitialQP = 15;  // Initial QP for I frames
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoEncoderH263::SetVideoEncoderParam(void) {

    if (!mEncoderParams) {
        LOGE("NULL pointer: mEncoderParams");
        return OMX_ErrorBadParameter;
    }

    mVideoEncoder->getParameters(mEncoderParams);
    mEncoderParams->profile = (VAProfile)PROFILE_H263BASELINE;
    return OMXVideoEncoderBase::SetVideoEncoderParam();
}

OMX_ERRORTYPE OMXVideoEncoderH263::ProcessorInit(void) {
    LOGV("OMXVideoEncoderH263::ProcessorInit\n");
    return OMXVideoEncoderBase::ProcessorInit();
}

OMX_ERRORTYPE OMXVideoEncoderH263::ProcessorDeinit(void) {
    return OMXVideoEncoderBase::ProcessorDeinit();
}

OMX_ERRORTYPE OMXVideoEncoderH263::ProcessorProcess(
    OMX_BUFFERHEADERTYPE **buffers,
    buffer_retain_t *retains,
    OMX_U32) {
    LOGV("OMXVideoEncoderH263::ProcessorProcess \n");

    VideoEncOutputBuffer outBuf;
    VideoEncRawBuffer inBuf;
    OMX_U32 outfilledlen = 0;
    OMX_S64 outtimestamp = 0;
    OMX_U32 outflags = 0;

    OMX_ERRORTYPE oret = OMX_ErrorNone;
    Encode_Status ret = ENCODE_SUCCESS;

    LOGV("%s(): enter encode\n", __func__);

    LOGV_IF(buffers[INPORT_INDEX]->nFlags & OMX_BUFFERFLAG_EOS,
            "%s(),%d: got OMX_BUFFERFLAG_EOS\n", __func__, __LINE__);

    if (!buffers[INPORT_INDEX]->nFilledLen) {
        LOGE("%s(),%d: input buffer's nFilledLen is zero\n", __func__, __LINE__);
        goto out;
    }

    inBuf.data = buffers[INPORT_INDEX]->pBuffer + buffers[INPORT_INDEX]->nOffset;
    inBuf.size = buffers[INPORT_INDEX]->nFilledLen;
    inBuf.type = FTYPE_UNKNOWN;
    inBuf.flag = 0;
    inBuf.timeStamp = buffers[INPORT_INDEX]->nTimeStamp;

    LOGV("buffer_in.data=%x, data_size=%d",
         (unsigned)inBuf.data, inBuf.size);

    outBuf.data =	buffers[OUTPORT_INDEX]->pBuffer + buffers[OUTPORT_INDEX]->nOffset;
    outBuf.bufferSize = buffers[OUTPORT_INDEX]->nAllocLen - buffers[OUTPORT_INDEX]->nOffset;
    outBuf.dataSize = 0;

    if(mFrameRetrieved) {
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

    if (mSyncEncoding == OMX_FALSE && mFrameInputCount == 0) {
        retains[INPORT_INDEX] = BUFFER_RETAIN_ACCUMULATE;
        retains[OUTPORT_INDEX] = BUFFER_RETAIN_GETAGAIN;
        mFrameRetrieved = OMX_TRUE;
        goto out;
    }

    outBuf.format = OUTPUT_EVERYTHING;
    ret = mVideoEncoder->getOutput(&outBuf);
    // CHECK_ENCODE_STATUS("encode");
    if(ret == ENCODE_NO_REQUEST_DATA) {
        mFrameRetrieved = OMX_TRUE;
        retains[OUTPORT_INDEX] = BUFFER_RETAIN_GETAGAIN;
        if (mSyncEncoding)
            retains[INPORT_INDEX] = BUFFER_RETAIN_NOT_RETAIN;
        else
            retains[INPORT_INDEX] = BUFFER_RETAIN_ACCUMULATE;

        goto out;
    }

    LOGV("output data size = %d", outBuf.dataSize);
    outfilledlen = outBuf.dataSize;
    outtimestamp = outBuf.timeStamp;


    if (outBuf.flag & ENCODE_BUFFERFLAG_SYNCFRAME) {
        outflags |= OMX_BUFFERFLAG_SYNCFRAME;
    }

    if(outBuf.flag & ENCODE_BUFFERFLAG_ENDOFFRAME) {
        outflags |= OMX_BUFFERFLAG_ENDOFFRAME;
        mFrameRetrieved = OMX_TRUE;
        if (mSyncEncoding)
            retains[INPORT_INDEX] = BUFFER_RETAIN_NOT_RETAIN;
        else
            retains[INPORT_INDEX] = BUFFER_RETAIN_ACCUMULATE;

    } else {
        retains[INPORT_INDEX] = BUFFER_RETAIN_GETAGAIN;  //get again

    }

    if (outfilledlen > 0) {
        retains[OUTPORT_INDEX] = BUFFER_RETAIN_NOT_RETAIN;
    } else {
        retains[OUTPORT_INDEX] = BUFFER_RETAIN_GETAGAIN;
    }


    if(ret == ENCODE_SLICESIZE_OVERFLOW) {
        LOGV("%s(), mix_video_encode returns MIX_RESULT_VIDEO_ENC_SLICESIZE_OVERFLOW"
             , __func__);
        oret = (OMX_ERRORTYPE)OMX_ErrorIntelExtSliceSizeOverflow;
    }
#if SHOW_FPS
    {
        struct timeval t;
        OMX_TICKS current_ts, interval_ts;
        float current_fps, average_fps;

        t.tv_sec = t.tv_usec = 0;
        gettimeofday(&t, NULL);

        current_ts =(nsecs_t)t.tv_sec * 1000000000 + (nsecs_t)t.tv_usec * 1000;
        interval_ts = current_ts - lastTs;
        lastTs = current_ts;

        current_fps = (float)1000000000 / (float)interval_ts;
        average_fps = (current_fps + lastFps) / 2;
        lastFps = current_fps;

        LOGD("FPS = %2.1f\n", average_fps);
    }
#endif

out:

    if(retains[OUTPORT_INDEX] != BUFFER_RETAIN_GETAGAIN) {
        buffers[OUTPORT_INDEX]->nFilledLen = outfilledlen;
        buffers[OUTPORT_INDEX]->nTimeStamp = outtimestamp;
        buffers[OUTPORT_INDEX]->nFlags = outflags;

        LOGV("********** output buffer: len=%d, ts=%lld, flags=%x",
             outfilledlen,
             outtimestamp,
             outflags);
    }

    if (retains[INPORT_INDEX] == BUFFER_RETAIN_NOT_RETAIN ||
            retains[INPORT_INDEX] == BUFFER_RETAIN_ACCUMULATE ) {
        mFrameInputCount ++;
    }

    if (retains[OUTPORT_INDEX] == BUFFER_RETAIN_NOT_RETAIN)
        mFrameOutputCount ++;

    LOGV_IF(oret == OMX_ErrorNone, "%s(),%d: exit, encode is done\n", __func__, __LINE__);

    return oret;

}

OMX_ERRORTYPE OMXVideoEncoderH263::BuildHandlerList(void) {
    OMXVideoEncoderBase::BuildHandlerList();
    AddHandler(OMX_IndexParamVideoH263, GetParamVideoH263, SetParamVideoH263);
    AddHandler(OMX_IndexParamVideoProfileLevelQuerySupported, GetParamVideoProfileLevelQuerySupported, SetParamVideoProfileLevelQuerySupported);
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoEncoderH263::GetParamVideoProfileLevelQuerySupported(OMX_PTR pStructure) {
    OMX_ERRORTYPE ret;
    OMX_VIDEO_PARAM_PROFILELEVELTYPE *p = (OMX_VIDEO_PARAM_PROFILELEVELTYPE *)pStructure;
    CHECK_TYPE_HEADER(p);
    CHECK_PORT_INDEX(p, OUTPORT_INDEX);

    struct ProfileLevelTable {
        OMX_U32 profile;
        OMX_U32 level;
    } plTable[] = {
        {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level45}
    };

    OMX_U32 count = sizeof(plTable)/sizeof(ProfileLevelTable);
    CHECK_ENUMERATION_RANGE(p->nProfileIndex,count);

    p->eProfile = plTable[p->nProfileIndex].profile;
    p->eLevel = plTable[p->nProfileIndex].level;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoEncoderH263::SetParamVideoProfileLevelQuerySupported(OMX_PTR) {
    LOGW("SetParamVideoH263ProfileLevel is not supported.");
    return OMX_ErrorUnsupportedSetting;
}

OMX_ERRORTYPE OMXVideoEncoderH263::GetParamVideoH263(OMX_PTR pStructure) {
    OMX_ERRORTYPE ret;
    OMX_VIDEO_PARAM_H263TYPE *p = (OMX_VIDEO_PARAM_H263TYPE *)pStructure;
    CHECK_TYPE_HEADER(p);
    CHECK_PORT_INDEX(p, OUTPORT_INDEX);

    memcpy(p, &mParamH263, sizeof(*p));
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoEncoderH263::SetParamVideoH263(OMX_PTR pStructure) {
    OMX_ERRORTYPE ret;
    OMX_VIDEO_PARAM_H263TYPE *p = (OMX_VIDEO_PARAM_H263TYPE *)pStructure;
    CHECK_TYPE_HEADER(p);
    CHECK_PORT_INDEX(p, OUTPORT_INDEX);
    CHECK_SET_PARAM_STATE();

    // TODO: do we need to check if port is enabled?
    // TODO: see SetPortH263Param implementation - Can we make simple copy????
    memcpy(&mParamH263, p, sizeof(mParamH263));
    return OMX_ErrorNone;
}


DECLARE_OMX_COMPONENT("OMX.Intel.VideoEncoder.H263", "video_encoder.h263", OMXVideoEncoderH263);

