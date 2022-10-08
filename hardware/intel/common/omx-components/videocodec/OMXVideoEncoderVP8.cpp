
#define LOG_TAG "OMXVideoEncoderVP8"
#include "OMXVideoEncoderVP8.h"

static const char *VP8_MIME_TYPE = "video/x-vnd.on2.vp8";

OMXVideoEncoderVP8::OMXVideoEncoderVP8() {
    LOGV("OMXVideoEncoderVP8 is constructed.");
    mLastTimestamp = 0x7FFFFFFFFFFFFFFFLL;
    BuildHandlerList();
    mVideoEncoder = createVideoEncoder(VP8_MIME_TYPE);
    if(!mVideoEncoder) LOGE("OMX_ErrorInsufficientResources");
}

OMXVideoEncoderVP8::~OMXVideoEncoderVP8() {
    LOGV("OMXVideoEncoderVP8 is destructed.");
}

OMX_ERRORTYPE OMXVideoEncoderVP8::InitOutputPortFormatSpecific(OMX_PARAM_PORTDEFINITIONTYPE *paramPortDefinitionOutput) {

    memset(&mParamVp8, 0, sizeof(mParamVp8));
    SetTypeHeader(&mParamVp8, sizeof(mParamVp8));
    mParamVp8.nPortIndex = OUTPORT_INDEX;
    mParamVp8.eProfile = OMX_VIDEO_VP8ProfileMain;
    mParamVp8.eLevel = OMX_VIDEO_VP8Level_Version3;

    memset(&mConfigVideoVp8ReferenceFrame, 0, sizeof(mConfigVideoVp8ReferenceFrame));
    SetTypeHeader(&mConfigVideoVp8ReferenceFrame, sizeof(mConfigVideoVp8ReferenceFrame));
    mConfigVideoVp8ReferenceFrame.nPortIndex = OUTPORT_INDEX;
    mConfigVideoVp8ReferenceFrame.bUsePreviousFrame = OMX_TRUE;
    mConfigVideoVp8ReferenceFrame.bUseGoldenFrame = OMX_TRUE;
    mConfigVideoVp8ReferenceFrame.bUseAlternateFrame = OMX_TRUE;
    mConfigVideoVp8ReferenceFrame.bPreviousFrameRefresh = OMX_TRUE;
    mConfigVideoVp8ReferenceFrame.bGoldenFrameRefresh = OMX_TRUE;
    mConfigVideoVp8ReferenceFrame.bAlternateFrameRefresh = OMX_TRUE;

    paramPortDefinitionOutput->nBufferCountActual = OUTPORT_ACTUAL_BUFFER_COUNT;
    paramPortDefinitionOutput->nBufferCountMin = OUTPORT_MIN_BUFFER_COUNT;
    paramPortDefinitionOutput->nBufferSize = OUTPORT_BUFFER_SIZE;
    paramPortDefinitionOutput->format.video.cMIMEType = (OMX_STRING)VP8_MIME_TYPE;
    paramPortDefinitionOutput->format.video.eCompressionFormat = OMX_VIDEO_CodingVP8;

    // OMX_VIDEO_PARAM_INTEL_NUMBER_OF_TEMPORAL_LAYER
    memset(&mTemporalLayer, 0, sizeof(mTemporalLayer));
    SetTypeHeader(&mTemporalLayer, sizeof(mTemporalLayer));
    mTemporalLayer.nPortIndex = OUTPORT_INDEX;
    mTemporalLayer.nNumberOfTemporalLayer = 1;//default value is 1

    mParamProfileLevel.eProfile = OMX_VIDEO_VP8ProfileMain;
    mParamProfileLevel.eLevel = OMX_VIDEO_VP8Level_Version3;
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoEncoderVP8::SetVideoEncoderParam() {

    if (!mEncoderParams) {
        LOGE("NULL pointer: mEncoderParams");
        return OMX_ErrorBadParameter;
    }

    mVideoEncoder->getParameters(mEncoderParams);
    mEncoderParams->profile = VAProfileVP8Version0_3;
    return OMXVideoEncoderBase::SetVideoEncoderParam();
}

OMX_ERRORTYPE OMXVideoEncoderVP8::ProcessorInit(void) {
    return OMXVideoEncoderBase::ProcessorInit();
}

OMX_ERRORTYPE OMXVideoEncoderVP8::ProcessorDeinit(void) {
    return OMXVideoEncoderBase::ProcessorDeinit();
}

OMX_ERRORTYPE OMXVideoEncoderVP8::ProcessorProcess(OMX_BUFFERHEADERTYPE **buffers,
        buffer_retain_t *retains,
        OMX_U32) {

    VideoEncOutputBuffer outBuf;
    VideoEncRawBuffer inBuf;
    Encode_Status ret = ENCODE_SUCCESS;

    OMX_U32 outfilledlen = 0;
    OMX_S64 outtimestamp = 0;
    OMX_U32 outflags = 0;
    OMX_ERRORTYPE oret = OMX_ErrorNone;
    OMX_U32 frameDuration;
    OMX_U32 this_fps;
    if(buffers[INPORT_INDEX]->nFlags & OMX_BUFFERFLAG_EOS) {
        LOGV("%s(),%d: got OMX_BUFFERFLAG_EOS\n", __func__, __LINE__);
        outflags |= OMX_BUFFERFLAG_EOS;
    }

    if (!buffers[INPORT_INDEX]->nFilledLen) {
        LOGV("%s(),%d: input buffer's nFilledLen is zero\n",  __func__, __LINE__);
        goto out;
    }

    inBuf.data = buffers[INPORT_INDEX]->pBuffer + buffers[INPORT_INDEX]->nOffset;
    inBuf.size = buffers[INPORT_INDEX]->nFilledLen;
    inBuf.type = FTYPE_UNKNOWN;
    inBuf.timeStamp = buffers[INPORT_INDEX]->nTimeStamp;

    if (inBuf.timeStamp > mLastTimestamp) {
        frameDuration = (OMX_U32)(inBuf.timeStamp - mLastTimestamp);
    } else {
        frameDuration = (OMX_U32)(1000000 / mEncoderParams->frameRate.frameRateNum);
    }

    this_fps = (OMX_U32)((1000000.000 / frameDuration) * 1000 + 1)/1000;

    if(this_fps != mEncoderParams->frameRate.frameRateNum)
    {// a new FrameRate is coming
        mConfigFramerate.xEncodeFramerate = this_fps;
        mEncoderParams->frameRate.frameRateNum = this_fps;
        VideoConfigFrameRate framerate;
        mVideoEncoder->getConfig(&framerate);
        framerate.frameRate.frameRateDenom = 1;
        framerate.frameRate.frameRateNum = mConfigFramerate.xEncodeFramerate;
        ret = mVideoEncoder->setConfig(&framerate);
        if(ret != ENCODE_SUCCESS) {
               LOGW("Failed to set frame rate config");
        }
    }
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

    {
        outBuf.format = OUTPUT_EVERYTHING;
        ret = mVideoEncoder->getOutput(&outBuf);
        //CHECK_ENCODE_STATUS("getOutput");
        if(ret == ENCODE_NO_REQUEST_DATA) {
            mFrameRetrieved = OMX_TRUE;
            retains[OUTPORT_INDEX] = BUFFER_RETAIN_GETAGAIN;
            if (mSyncEncoding)
                retains[INPORT_INDEX] = BUFFER_RETAIN_NOT_RETAIN;
            else
                retains[INPORT_INDEX] = BUFFER_RETAIN_NOT_RETAIN;

            goto out;
        }

        LOGV("VP8 encode output data size = %d", outBuf.dataSize);


        outfilledlen = outBuf.dataSize;
        outtimestamp = buffers[INPORT_INDEX]->nTimeStamp;
        mLastTimestamp = inBuf.timeStamp;
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
                retains[INPORT_INDEX] = BUFFER_RETAIN_NOT_RETAIN;

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

OMX_ERRORTYPE OMXVideoEncoderVP8::BuildHandlerList(void) {
    OMXVideoEncoderBase::BuildHandlerList();
    AddHandler((OMX_INDEXTYPE)OMX_IndexParamVideoVp8, GetParamVideoVp8, SetParamVideoVp8);
    AddHandler((OMX_INDEXTYPE)OMX_IndexConfigVideoVp8ReferenceFrame, GetConfigVideoVp8ReferenceFrame, SetConfigVideoVp8ReferenceFrame);
    AddHandler((OMX_INDEXTYPE)OMX_IndexExtVP8MaxFrameSizeRatio, GetConfigVp8MaxFrameSizeRatio, SetConfigVp8MaxFrameSizeRatio);

    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoEncoderVP8::GetParamVideoVp8(OMX_PTR pStructure) {
    OMX_ERRORTYPE ret;
    OMX_VIDEO_PARAM_VP8TYPE *p = (OMX_VIDEO_PARAM_VP8TYPE*) pStructure;
    CHECK_TYPE_HEADER(p);
    CHECK_PORT_INDEX(p, OUTPORT_INDEX);

    memcpy(p, &mParamVp8, sizeof(*p));
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoEncoderVP8::SetParamVideoVp8(OMX_PTR pStructure) {
    OMX_ERRORTYPE ret;
    OMX_VIDEO_PARAM_VP8TYPE *p = (OMX_VIDEO_PARAM_VP8TYPE*) pStructure;
    CHECK_TYPE_HEADER(p);
    CHECK_PORT_INDEX(p, OUTPORT_INDEX);
    CHECK_SET_PARAM_STATE();

    memcpy(&mParamVp8, p, sizeof(mParamVp8));
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoEncoderVP8::GetConfigVideoVp8ReferenceFrame(OMX_PTR pStructure) {
    OMX_ERRORTYPE ret;
    OMX_VIDEO_VP8REFERENCEFRAMETYPE *p = (OMX_VIDEO_VP8REFERENCEFRAMETYPE*)pStructure;
    CHECK_TYPE_HEADER(p);
    CHECK_PORT_INDEX(p, OUTPORT_INDEX);

    memcpy(p, &mConfigVideoVp8ReferenceFrame, sizeof(*p));

    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoEncoderVP8::SetConfigVideoVp8ReferenceFrame(OMX_PTR pStructure) {
    OMX_ERRORTYPE ret;
    Encode_Status retStatus = ENCODE_SUCCESS;
    OMX_VIDEO_VP8REFERENCEFRAMETYPE *p = (OMX_VIDEO_VP8REFERENCEFRAMETYPE*) pStructure;
    CHECK_TYPE_HEADER(p);
    CHECK_PORT_INDEX(p, OUTPORT_INDEX);

    CHECK_SET_CONFIG_STATE();

    VideoConfigVP8ReferenceFrame configVP8ReferenceFrame;
    configVP8ReferenceFrame.no_ref_last = !p->bUsePreviousFrame;
    configVP8ReferenceFrame.no_ref_gf = !p->bUseGoldenFrame;
    configVP8ReferenceFrame.no_ref_arf = !p->bUseAlternateFrame;
    configVP8ReferenceFrame.refresh_alternate_frame = p->bAlternateFrameRefresh;
    configVP8ReferenceFrame.refresh_golden_frame = p->bGoldenFrameRefresh;
    configVP8ReferenceFrame.refresh_last = p->bPreviousFrameRefresh;

    retStatus = mVideoEncoder->setConfig(&configVP8ReferenceFrame);
    if(retStatus != ENCODE_SUCCESS) {
        LOGW("Failed to set reference frame");
    }
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoEncoderVP8::GetConfigVp8MaxFrameSizeRatio(OMX_PTR) {

    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoEncoderVP8::SetConfigVp8MaxFrameSizeRatio(OMX_PTR pStructure) {
    OMX_ERRORTYPE ret;
    Encode_Status retStatus = ENCODE_SUCCESS;
    OMX_VIDEO_CONFIG_INTEL_VP8_MAX_FRAME_SIZE_RATIO *p = (OMX_VIDEO_CONFIG_INTEL_VP8_MAX_FRAME_SIZE_RATIO*)pStructure;
    CHECK_TYPE_HEADER(p);
    CHECK_PORT_INDEX(p, OUTPORT_INDEX);

    CHECK_SET_CONFIG_STATE();

    VideoConfigVP8MaxFrameSizeRatio configVP8MaxFrameSizeRatio;
    configVP8MaxFrameSizeRatio.max_frame_size_ratio = p->nMaxFrameSizeRatio;

    retStatus = mVideoEncoder->setConfig(&configVP8MaxFrameSizeRatio);
    if(retStatus != ENCODE_SUCCESS) {
        LOGW("Failed to set vp8 max frame size ratio");
    }

    return OMX_ErrorNone;
}

DECLARE_OMX_COMPONENT("OMX.Intel.VideoEncoder.VP8", "video_encoder.vp8", OMXVideoEncoderVP8);
