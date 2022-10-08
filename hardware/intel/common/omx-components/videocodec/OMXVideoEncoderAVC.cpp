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

#define LOG_TAG "OMXVideoEncoderAVC"
#include "OMXVideoEncoderAVC.h"
#include "IntelMetadataBuffer.h"

static const char *AVC_MIME_TYPE = "video/h264";

struct ProfileMap {
    OMX_VIDEO_AVCPROFILETYPE key;
    VAProfile value;
    const char *name;
};

struct LevelMap {
    OMX_VIDEO_AVCLEVELTYPE key;
    uint32_t value;
    const char *name;
};

static ProfileMap ProfileTable[] = {
    { OMX_VIDEO_AVCProfileBaseline, VAProfileH264Baseline, "AVC Baseline" },
    { OMX_VIDEO_AVCProfileMain, VAProfileH264Main, "AVC Main" },
    { OMX_VIDEO_AVCProfileHigh, VAProfileH264High, "AVC High" },
    { (OMX_VIDEO_AVCPROFILETYPE) 0, (VAProfile) 0, "Not Supported" },
};

static LevelMap LevelTable[] = {
    { OMX_VIDEO_AVCLevel4, 40, "AVC Level4" },
    { OMX_VIDEO_AVCLevel41, 41, "AVC Level41" },
    { OMX_VIDEO_AVCLevel42, 42, "AVC Level42" },
    { OMX_VIDEO_AVCLevel5, 50, "AVC Level5" },
    { OMX_VIDEO_AVCLevel51, 51, "AVC Level51" },
    { (OMX_VIDEO_AVCLEVELTYPE) 0, 0, "Not Supported" },
};

#define FIND_BYKEY(table, x, y)  {\
        for(int ii = 0; ; ii++) { \
            if (table[ii].key == x || table[ii].key == 0) { \
                y = ii; \
                break; \
            } \
        } \
    }\

#define FIND_BYVALUE(table, x, y)  {\
        for(int ii = 0; ; ii++) { \
            if (table[ii].value == x || table[ii].value == 0) { \
                y = ii; \
                break; \
            } \
        } \
    } \

OMXVideoEncoderAVC::OMXVideoEncoderAVC() {
    BuildHandlerList();
    mVideoEncoder = createVideoEncoder(AVC_MIME_TYPE);
    if (!mVideoEncoder) {
        LOGE("OMX_ErrorInsufficientResources");
        return;
    }

    mAVCParams = new VideoParamsAVC();
    if (!mAVCParams) {
        LOGE("OMX_ErrorInsufficientResources");
        return;
    }

    //Query supported Profile/Level
    mPLTableCount = 0;

    VAProfile profiles[MAX_H264_PROFILE] = {VAProfileH264High, VAProfileH264Main, VAProfileH264Baseline};

    VideoParamsProfileLevel pl;
    for (int i=0; i < MAX_H264_PROFILE; i++) {
        pl.profile = profiles[i];
        pl.level = 0;
        pl.isSupported = false;

        mVideoEncoder->getParameters(&pl);
        if (pl.isSupported) {
            uint32_t profile_index;
            uint32_t level_index;

            FIND_BYVALUE(ProfileTable, pl.profile,  profile_index);
            if (ProfileTable[profile_index].key == (OMX_VIDEO_AVCPROFILETYPE) 0)
                continue;

            FIND_BYVALUE(LevelTable, pl.level,  level_index);
            if (LevelTable[level_index].key == (OMX_VIDEO_AVCLEVELTYPE) 0)
                continue;

            mPLTable[mPLTableCount].profile = ProfileTable[profile_index].key;
            mPLTable[mPLTableCount].level = LevelTable[level_index].key;
            mPLTableCount ++;
            LOGV("Support Profile:%s, Level:%s\n", ProfileTable[profile_index].name, LevelTable[level_index].name);
        }
    }

    mEmptyEOSBuf = OMX_FALSE;
}

OMXVideoEncoderAVC::~OMXVideoEncoderAVC() {
    if(mAVCParams) {
        delete mAVCParams;
        mAVCParams = NULL;
    }
}

OMX_ERRORTYPE OMXVideoEncoderAVC::InitOutputPortFormatSpecific(OMX_PARAM_PORTDEFINITIONTYPE *paramPortDefinitionOutput) {
    // OMX_VIDEO_PARAM_AVCTYPE
    memset(&mParamAvc, 0, sizeof(mParamAvc));
    SetTypeHeader(&mParamAvc, sizeof(mParamAvc));
    mParamAvc.nPortIndex = OUTPORT_INDEX;

    if (mPLTableCount > 0) {
        mParamAvc.eProfile = (OMX_VIDEO_AVCPROFILETYPE) mPLTable[0].profile;
        mParamAvc.eLevel = (OMX_VIDEO_AVCLEVELTYPE)mPLTable[0].level;
    } else {
        LOGE("No supported profile/level\n");
        return OMX_ErrorUndefined;
    }
    mParamAvc.nAllowedPictureTypes = OMX_VIDEO_PictureTypeI | OMX_VIDEO_PictureTypeP;
    mParamAvc.nPFrames = 29;
    mParamAvc.nBFrames = 0;

    // OMX_NALSTREAMFORMATTYPE
    memset(&mNalStreamFormat, 0, sizeof(mNalStreamFormat));
    SetTypeHeader(&mNalStreamFormat, sizeof(mNalStreamFormat));
    mNalStreamFormat.nPortIndex = OUTPORT_INDEX;
    // TODO: check if this is desired Nalu Format
    //mNalStreamFormat.eNaluFormat = OMX_NaluFormatLengthPrefixedSeparateFirstHeader;
    // OMX_VIDEO_CONFIG_AVCINTRAPERIOD
    memset(&mConfigAvcIntraPeriod, 0, sizeof(mConfigAvcIntraPeriod));
    SetTypeHeader(&mConfigAvcIntraPeriod, sizeof(mConfigAvcIntraPeriod));
    mConfigAvcIntraPeriod.nPortIndex = OUTPORT_INDEX;
    // TODO: need to be populated from Video Encoder
    mConfigAvcIntraPeriod.nIDRPeriod = 1;
    mConfigAvcIntraPeriod.nPFrames = 29;

    // OMX_VIDEO_CONFIG_NALSIZE
    memset(&mConfigNalSize, 0, sizeof(mConfigNalSize));
    SetTypeHeader(&mConfigNalSize, sizeof(mConfigNalSize));
    mConfigNalSize.nPortIndex = OUTPORT_INDEX;
    mConfigNalSize.nNaluBytes = 0;

    // OMX_VIDEO_PARAM_INTEL_AVCVUI
    memset(&mParamIntelAvcVui, 0, sizeof(mParamIntelAvcVui));
    SetTypeHeader(&mParamIntelAvcVui, sizeof(mParamIntelAvcVui));
    mParamIntelAvcVui.nPortIndex = OUTPORT_INDEX;
    mParamIntelAvcVui.bVuiGeneration = OMX_FALSE;

    // OMX_VIDEO_CONFIG_INTEL_SLICE_NUMBERS
    memset(&mConfigIntelSliceNumbers, 0, sizeof(mConfigIntelSliceNumbers));
    SetTypeHeader(&mConfigIntelSliceNumbers, sizeof(mConfigIntelSliceNumbers));
    mConfigIntelSliceNumbers.nPortIndex = OUTPORT_INDEX;
    mConfigIntelSliceNumbers.nISliceNumber = 1;
    mConfigIntelSliceNumbers.nPSliceNumber = 1;

    // Override OMX_PARAM_PORTDEFINITIONTYPE
    paramPortDefinitionOutput->nBufferCountActual = OUTPORT_ACTUAL_BUFFER_COUNT;
    paramPortDefinitionOutput->nBufferCountMin = OUTPORT_MIN_BUFFER_COUNT;
    paramPortDefinitionOutput->nBufferSize = OUTPORT_BUFFER_SIZE;
    paramPortDefinitionOutput->format.video.cMIMEType = (OMX_STRING)AVC_MIME_TYPE;
    paramPortDefinitionOutput->format.video.eCompressionFormat = OMX_VIDEO_CodingAVC;

    // Override OMX_VIDEO_PARAM_PROFILELEVELTYPE
    // TODO: check if profile/level supported is correct
    mParamProfileLevel.eProfile = mParamAvc.eProfile;
    mParamProfileLevel.eLevel = mParamAvc.eLevel;

    // Override OMX_VIDEO_PARAM_BITRATETYPE
    mParamBitrate.nTargetBitrate = 192000;

    // Override OMX_VIDEO_CONFIG_INTEL_BITRATETYPE
    mConfigIntelBitrate.nInitialQP = 0;  // Initial QP for I frames

    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoEncoderAVC::SetVideoEncoderParam(void) {

    Encode_Status ret = ENCODE_SUCCESS;
    LOGV("OMXVideoEncoderAVC::SetVideoEncoderParam");

    if (!mEncoderParams) {
        LOGE("NULL pointer: mEncoderParams");
        return OMX_ErrorBadParameter;
    }

    mVideoEncoder->getParameters(mEncoderParams);
    uint32_t index;
    FIND_BYKEY(ProfileTable, mParamAvc.eProfile, index);
    if (ProfileTable[index].value != 0)
        mEncoderParams->profile = ProfileTable[index].value;

    if (mParamAvc.nAllowedPictureTypes & OMX_VIDEO_PictureTypeB)
        mEncoderParams->intraPeriod = mParamAvc.nPFrames + mParamAvc.nBFrames;
    else
        mEncoderParams->intraPeriod = mParamAvc.nPFrames + 1;

    // 0 - all luma and chroma block edges of the slice are filtered
    // 1 - deblocking is disabled for all block edges of the slice
    // 2 - all luma and chroma block edges of the slice are filtered
    // with exception of the block edges that coincide with slice boundaries
    mEncoderParams->disableDeblocking = 0;

    OMXVideoEncoderBase::SetVideoEncoderParam();

    mVideoEncoder->getParameters(mAVCParams);
    if(mParamIntelAvcVui.bVuiGeneration == OMX_TRUE) {
        mAVCParams->VUIFlag = 1;
    }
    // For resolution below VGA, single core can hit the performance target and provide VQ gain
    if (mEncoderParams->resolution.width <= 640 && mEncoderParams->resolution.height <= 480) {
        mConfigIntelSliceNumbers.nISliceNumber = 1;
        mConfigIntelSliceNumbers.nPSliceNumber = 1;
    }
    mAVCParams->sliceNum.iSliceNum = mConfigIntelSliceNumbers.nISliceNumber;
    mAVCParams->sliceNum.pSliceNum = mConfigIntelSliceNumbers.nPSliceNumber;
    mAVCParams->maxSliceSize = mConfigNalSize.nNaluBytes * 8;

    if (mEncoderParams->intraPeriod == 0) {
        mAVCParams->idrInterval = 0;
        mAVCParams->ipPeriod = 1;
    } else {
        mAVCParams->idrInterval = mConfigAvcIntraPeriod.nIDRPeriod; //idrinterval
        if (mParamAvc.nAllowedPictureTypes & OMX_VIDEO_PictureTypeB)
            mAVCParams->ipPeriod = mEncoderParams->intraPeriod / mParamAvc.nPFrames;
        else
            mAVCParams->ipPeriod = 1;
    }

    ret = mVideoEncoder ->setParameters(mAVCParams);
    CHECK_ENCODE_STATUS("setParameters");

    LOGV("VUIFlag = %d\n", mAVCParams->VUIFlag);
    LOGV("sliceNum.iSliceNum = %d\n", mAVCParams->sliceNum.iSliceNum);
    LOGV("sliceNum.pSliceNum = %d\n", mAVCParams->sliceNum.pSliceNum);
    LOGV("maxSliceSize = %d\n ", mAVCParams->maxSliceSize);
    LOGV("intraPeriod = %d\n ", mEncoderParams->intraPeriod);
    LOGV("idrInterval = %d\n ", mAVCParams->idrInterval);
    LOGV("ipPeriod = %d\n ", mAVCParams->ipPeriod);
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoEncoderAVC::ProcessorInit(void) {
    mCSDOutputted = OMX_FALSE;
    mInputPictureCount = 0;
    mFrameEncodedCount = 0;
    return  OMXVideoEncoderBase::ProcessorInit();
}

OMX_ERRORTYPE OMXVideoEncoderAVC::ProcessorDeinit(void) {
    return OMXVideoEncoderBase::ProcessorDeinit();
}

OMX_ERRORTYPE OMXVideoEncoderAVC::ProcessorStop(void) {
    OMX_BUFFERHEADERTYPE *omxbuf = NULL;

    while(!mBFrameList.empty()) {
        omxbuf = * mBFrameList.begin();
        this->ports[INPORT_INDEX]->ReturnThisBuffer(omxbuf);
        mBFrameList.erase(mBFrameList.begin());
    }

    mEmptyEOSBuf = OMX_FALSE;
    return OMXVideoEncoderBase::ProcessorStop();
}

OMX_ERRORTYPE OMXVideoEncoderAVC::ProcessorPreEmptyBuffer(OMX_BUFFERHEADERTYPE* buffer) {
    OMX_U32 EncodeInfo = 0;
    OMX_U32 EncodeFrameType = 0;

    uint32_t poc = 0;
    uint32_t idrPeriod = mAVCParams->idrInterval;
    uint32_t IntraPeriod = mEncoderParams->intraPeriod;
    uint32_t IpPeriod = mAVCParams->ipPeriod;
    bool BFrameEnabled = IpPeriod > 1;
    uint32_t GOP = 0;

    if (idrPeriod == 0 || IntraPeriod == 0) {
        GOP = 0xFFFFFFFF;
        if (IntraPeriod == 0)
            IntraPeriod = 0xFFFFFFFF;
    } else if (BFrameEnabled)
        GOP = IntraPeriod*idrPeriod + 1;
    else
        GOP = IntraPeriod*idrPeriod;

    LOGV("ProcessorPreEmptyBuffer idrPeriod=%d, IntraPeriod=%d, IpPeriod=%d, BFrameEnabled=%d\n", idrPeriod, IntraPeriod, IpPeriod, BFrameEnabled);

    //decide frame type, refer Merrifield Video Encoder Driver HLD Chapter 3.17
    poc = mInputPictureCount % GOP;

    if (poc == 0 /*IDR*/) {
            EncodeFrameType = F_IDR;
    } else if (IntraPeriod == 1) {
            EncodeFrameType = F_I;
    }else if ((poc > IpPeriod) && ((poc - IpPeriod) % IntraPeriod == 0))/*I*/{
            EncodeFrameType = F_I;
            if (BFrameEnabled)
                SET_CO(EncodeInfo, CACHE_POP);
    } else if ((poc % IpPeriod == 0) /*P*/ || (buffer->nFlags & OMX_BUFFERFLAG_EOS)/*EOS,always P*/) {
            EncodeFrameType = F_P;
            if (BFrameEnabled)
                SET_CO(EncodeInfo, CACHE_POP);
    } else { /*B*/
            EncodeFrameType = F_B;
            SET_CO(EncodeInfo, CACHE_PUSH);
    }

    SET_FT(EncodeInfo, EncodeFrameType);
    SET_FC(EncodeInfo, mInputPictureCount);

    buffer->pPlatformPrivate = (OMX_PTR) EncodeInfo;

    LOGV("ProcessorPreEmptyBuffer Frame %d, Type %s, EncodeInfo %x\n", mInputPictureCount, FrameTypeStr[EncodeFrameType], EncodeInfo);

    mInputPictureCount ++;
    return OMX_ErrorNone;
}

OMX_BOOL OMXVideoEncoderAVC::ProcessCacheOperation(OMX_BUFFERHEADERTYPE **buffers) {

    OMX_BOOL Cached = OMX_FALSE;

    //get frame encode info
    Encode_Info eInfo;
    uint32_t encodeInfo 	= (uint32_t) buffers[INPORT_INDEX]->pPlatformPrivate;
    eInfo.FrameType 		   = GET_FT(encodeInfo);

    eInfo.CacheOperation	= GET_CO(encodeInfo);
    eInfo.NotStopFrame		= encodeInfo & ENC_NSTOP;
    eInfo.FrameCount		 = GET_FC(encodeInfo);

    LOGV("ProcessCacheOperation Frame %d, type:%s, CacheOps:%s, NoSTOP=%d, EOS=%d\n",
            eInfo.FrameCount, FrameTypeStr[eInfo.FrameType], CacheOperationStr[eInfo.CacheOperation],
            eInfo.NotStopFrame, buffers[INPORT_INDEX]->nFlags & OMX_BUFFERFLAG_EOS);

    OMX_BOOL emptyEOSBuf = OMX_FALSE;
    if (buffers[INPORT_INDEX]->nFilledLen == 0 && buffers[INPORT_INDEX]->nFlags & OMX_BUFFERFLAG_EOS) {
        //meet an empty EOS buffer
        emptyEOSBuf = OMX_TRUE;
        LOGV("ProcessCacheOperation: This frame is Empty EOS buffer\n");
    }

    if (eInfo.CacheOperation == CACHE_NONE) {
        //nothing to do
    } else if (eInfo.CacheOperation == CACHE_PUSH) {
        mBFrameList.push_front(buffers[INPORT_INDEX]);
        Cached = OMX_TRUE;
        LOGV("ProcessCacheOperation: This B frame is cached\n");

    } else if (eInfo.CacheOperation == CACHE_POP) {
        eInfo.NotStopFrame = true;  //it is also a nstop frame

        OMX_BUFFERHEADERTYPE *omxbuf = NULL;
        uint32_t i = 0;
        uint32_t bframecount = mBFrameList.size();

        LOGV("BFrameList size = %d\n", bframecount);

        while(!mBFrameList.empty()) {
            /*TODO: need to handle null data buffer with EOS
                     !NULL EOS case:   B1 B2 P(EOS)     ->    P B1 B2(EOS)
                     NULL EOS case: B1 B2 NULL(EOS)    ->    B2 B1 NULL(EOS)
            */

            if (emptyEOSBuf) {
                omxbuf = *mBFrameList.begin();
                ports[INPORT_INDEX]->PushThisBuffer(omxbuf);
                mBFrameList.erase(mBFrameList.begin()); //clear it from internal queue

            } else {
                omxbuf = *mBFrameList.begin();

                if (buffers[INPORT_INDEX]->nFlags & OMX_BUFFERFLAG_EOS && i == 0 )  {
                    //this is final encode frame, mark it is new EOS and remove original EOS
                    omxbuf->nFlags |= OMX_BUFFERFLAG_EOS;
				    buffers[INPORT_INDEX]->nFlags &= ~OMX_BUFFERFLAG_EOS;
                } else {
                    //all these frames except final B frame in miniGOP can't be stopped at any time
                    //to avoid not breaking miniGOP integrity
                    if (i > 0) {
                        uint32_t tmp = (uint32_t) omxbuf->pPlatformPrivate;
                        tmp |= ENC_NSTOP;
                        omxbuf->pPlatformPrivate = (OMX_PTR) tmp;
                    }
                }
                ports[INPORT_INDEX]->RetainThisBuffer(omxbuf, false); //push bufferq head

                mBFrameList.erase(mBFrameList.begin()); //clear it from internal queue
            }

            i++;
        }

        if (emptyEOSBuf)
            ports[INPORT_INDEX]->PushThisBuffer(buffers[INPORT_INDEX]); //put it at the tail

    } else if (eInfo.CacheOperation == CACHE_RESET) {
//        mBFrameList.clear();
    }

    eInfo.CacheOperation = CACHE_NONE;

    /* restore all states into input OMX buffer
    */
    if (eInfo.NotStopFrame)
        encodeInfo |= ENC_NSTOP;
    else
        encodeInfo &= ~ENC_NSTOP;

    SET_CO(encodeInfo, eInfo.CacheOperation);
    buffers[INPORT_INDEX]->pPlatformPrivate = (OMX_PTR) encodeInfo;

    LOGV("ProcessCacheOperation Completed return %d\n", Cached);
    return Cached;
}

OMX_ERRORTYPE OMXVideoEncoderAVC::ProcessDataRetrieve(
    OMX_BUFFERHEADERTYPE **buffers, OMX_BOOL *outBufReturned) {

    OMX_NALUFORMATSTYPE NaluFormat = mNalStreamFormat.eNaluFormat;

    // NaluFormat not set, setting default
    if (NaluFormat == 0) {
        NaluFormat = (OMX_NALUFORMATSTYPE)OMX_NaluFormatStartCodesSeparateFirstHeader;
        mNalStreamFormat.eNaluFormat = NaluFormat;
    }

    VideoEncOutputBuffer outBuf;
    outBuf.data = buffers[OUTPORT_INDEX]->pBuffer;
    outBuf.bufferSize = buffers[OUTPORT_INDEX]->nAllocLen;
    outBuf.dataSize = 0;
    outBuf.remainingSize = 0;
    outBuf.flag = 0;
    outBuf.timeStamp = 0;
    outBuf.offset = 0;

    switch (NaluFormat) {
        case OMX_NaluFormatStartCodes:
            outBuf.format = OUTPUT_EVERYTHING;
            break;

        case OMX_NaluFormatOneNaluPerBuffer:
            outBuf.format = OUTPUT_ONE_NAL;
            break;

        default:
            if (NaluFormat == (OMX_NALUFORMATSTYPE)OMX_NaluFormatStartCodesSeparateFirstHeader||
                NaluFormat == (OMX_NALUFORMATSTYPE)OMX_NaluFormatLengthPrefixedSeparateFirstHeader){
                if(!mCSDOutputted) {
                    LOGV("Output codec data for first frame\n");
                    outBuf.format = OUTPUT_CODEC_DATA;
                } else {
                    if (NaluFormat == (OMX_NALUFORMATSTYPE)OMX_NaluFormatStartCodesSeparateFirstHeader)
                        outBuf.format = OUTPUT_EVERYTHING;
                    else
                        outBuf.format = OUTPUT_NALULENGTHS_PREFIXED;
                }
                break;
            } else {
                return OMX_ErrorUndefined;
            }
    }

    //start getOutput
    Encode_Status ret = mVideoEncoder->getOutput(&outBuf, FUNC_NONBLOCK);

    if (ret < ENCODE_SUCCESS) {
        LOGE("libMIX getOutput Failed. ret = 0x%08x\n", ret);
        outBuf.dataSize = 0;
        outBuf.flag |= ENCODE_BUFFERFLAG_ENDOFFRAME;
        if (ret == ENCODE_NO_REQUEST_DATA) {
            if (mEmptyEOSBuf) {
                //make sure no data encoding in HW, then emit one empty out buffer with EOS
                outBuf.flag |= ENCODE_BUFFERFLAG_ENDOFSTREAM;
                LOGV("no more data encoding, will signal empty EOS output buf\n");
            } else {
                //if not meet Empty EOS buffer, shouldn't get this error
                LOGE("sever error, should not happend here\n");
                //return OMX_ErrorUndefined; //not return error here to avoid omxcodec crash
            }
        }

    } else if (ret == ENCODE_BUFFER_TOO_SMALL) {
        LOGE("output buffer too small\n");
        // Return code could not be ENCODE_BUFFER_TOO_SMALL, or we will have dead lock issue
        return OMX_ErrorUndefined;
    } else if (ret == ENCODE_DATA_NOT_READY) {
        LOGV("Call libMIX getOutput againe due to 'data not ready'\n");
        ret = mVideoEncoder->getOutput(&outBuf);
    }

    LOGV("libMIX getOutput data size= %d, flag=0x%08x", outBuf.dataSize, outBuf.flag);
    OMX_U32 outfilledlen = outBuf.dataSize;
    OMX_U32 outoffset = outBuf.offset;
    OMX_S64 outtimestamp = outBuf.timeStamp;
    OMX_U32 outflags = 0;

    //if codecconfig
    if (outBuf.flag & ENCODE_BUFFERFLAG_CODECCONFIG)
        outflags |= OMX_BUFFERFLAG_CODECCONFIG;

    //if syncframe
    if (outBuf.flag & ENCODE_BUFFERFLAG_SYNCFRAME)
        outflags |= OMX_BUFFERFLAG_SYNCFRAME;

    //if eos
    if (outBuf.flag & ENCODE_BUFFERFLAG_ENDOFSTREAM)
        outflags |= OMX_BUFFERFLAG_EOS;

    //if full encoded data retrieved
    if(outBuf.flag & ENCODE_BUFFERFLAG_ENDOFFRAME) {
        LOGV("got a complete libmix Frame\n");
        outflags |= OMX_BUFFERFLAG_ENDOFFRAME;

        if ((NaluFormat == (OMX_NALUFORMATSTYPE)OMX_NaluFormatStartCodesSeparateFirstHeader
             || NaluFormat == (OMX_NALUFORMATSTYPE)OMX_NaluFormatLengthPrefixedSeparateFirstHeader )
             && !mCSDOutputted && outfilledlen > 0) {
            mCSDOutputted = OMX_TRUE;

        } else {
            ports[INPORT_INDEX]->ReturnOneRetainedBuffer();  //return one retained frame from head
            mFrameOutputCount  ++;
        }
    }

    if (outfilledlen == 0) {
        if (mEmptyEOSBuf) {
            //emit empty EOS out buf since meet empty EOS input buf
            buffers[OUTPORT_INDEX]->nFilledLen = 0;
            buffers[OUTPORT_INDEX]->nTimeStamp = 0;
            buffers[OUTPORT_INDEX]->nFlags = outflags;
            *outBufReturned = OMX_TRUE;
            LOGV("emit one empty EOS OMX output buf = %p:%d, flag = 0x%08x, ts=%lld", buffers[OUTPORT_INDEX]->pBuffer, outfilledlen, outflags, outtimestamp);
        } else
            //not emit out buf since something wrong
            *outBufReturned = OMX_FALSE;

    } else {
        buffers[OUTPORT_INDEX]->nOffset = outoffset;
        buffers[OUTPORT_INDEX]->nFilledLen = outfilledlen;
        buffers[OUTPORT_INDEX]->nTimeStamp = outtimestamp;
        buffers[OUTPORT_INDEX]->nFlags = outflags;
        if (outBuf.flag & ENCODE_BUFFERFLAG_NSTOPFRAME)
            buffers[OUTPORT_INDEX]->pPlatformPrivate = (OMX_PTR) 0x00000001;  //indicate it is nstop frame
        *outBufReturned = OMX_TRUE;
        LOGV("emit one OMX output buf = %p:%d, flag = 0x%08x, ts=%lld", buffers[OUTPORT_INDEX]->pBuffer, outfilledlen, outflags, outtimestamp);

    }

    LOGV("ProcessDataRetrieve OK, mFrameEncodedCount=%d , mFrameOutputCount=%d\n", mFrameEncodedCount, mFrameOutputCount);
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoEncoderAVC::ProcessorProcess(
    OMX_BUFFERHEADERTYPE **buffers,
    buffer_retain_t *retains,
    OMX_U32) {

    OMX_ERRORTYPE oret = OMX_ErrorNone;
    Encode_Status ret = ENCODE_SUCCESS;

    bool FrameEncoded = false;

    if (buffers[INPORT_INDEX]) {
        LOGV("input buffer has new frame\n");

        //get frame encode info
        Encode_Info eInfo;
        uint32_t encodeInfo 	= (uint32_t) buffers[INPORT_INDEX]->pPlatformPrivate;
        eInfo.FrameType 		   = GET_FT(encodeInfo);
        eInfo.CacheOperation	= GET_CO(encodeInfo);
        eInfo.NotStopFrame		= encodeInfo & ENC_NSTOP;
        eInfo.FrameCount		 = GET_FC(encodeInfo);

        //handle frame cache operation
        if (ProcessCacheOperation(buffers)) {
            //frame is cached, nothing should be done in this case, just store status and return
            retains[INPORT_INDEX] = BUFFER_RETAIN_CACHE;
            retains[OUTPORT_INDEX] = BUFFER_RETAIN_GETAGAIN;
            return OMX_ErrorNone;
        }

        //try encode if frame is not cached
        VideoEncRawBuffer inBuf;

        inBuf.data = buffers[INPORT_INDEX]->pBuffer + buffers[INPORT_INDEX]->nOffset;
        inBuf.size = buffers[INPORT_INDEX]->nFilledLen;
        inBuf.flag = 0;
        inBuf.timeStamp = buffers[INPORT_INDEX]->nTimeStamp;

        if (inBuf.size == 0 && buffers[INPORT_INDEX]->nFlags & OMX_BUFFERFLAG_EOS) {
            //meet an empty EOS buffer, retain it directly and return from here
            retains[INPORT_INDEX] = BUFFER_RETAIN_ACCUMULATE;
            retains[OUTPORT_INDEX] = BUFFER_RETAIN_GETAGAIN;
            mEmptyEOSBuf = OMX_TRUE;
            return OMX_ErrorNone;
        }

        if (buffers[INPORT_INDEX]->nFlags & OMX_BUFFERFLAG_EOS)
            inBuf.flag |= ENCODE_BUFFERFLAG_ENDOFSTREAM;
        if (eInfo.NotStopFrame)
            inBuf.flag |= ENCODE_BUFFERFLAG_NSTOPFRAME;
        inBuf.type = (FrameType) eInfo.FrameType;

        LOGV("start libmix encoding\n");
        // encode and setConfig need to be thread safe
        pthread_mutex_lock(&mSerializationLock);
        ret = mVideoEncoder->encode(&inBuf, FUNC_NONBLOCK);
        pthread_mutex_unlock(&mSerializationLock);
        LOGV("end libmix encoding\n");

		retains[INPORT_INDEX] = BUFFER_RETAIN_NOT_RETAIN;
        if (ret == ENCODE_DEVICE_BUSY) {
			//encoder is busy, put buf back and come again
            LOGV("encoder is busy, push buffer back to get again\n");
            retains[INPORT_INDEX] = BUFFER_RETAIN_GETAGAIN;
        } else {
            //if error, this buf will be returned
            CHECK_ENCODE_STATUS("encode");

            LOGV("put buffer to encoder and retain this buffer\n");
            mFrameEncodedCount ++;
            FrameEncoded = true;
            retains[INPORT_INDEX] = BUFFER_RETAIN_ACCUMULATE;
        }

    } else {
        //no new coming frames, but maybe still have frames not outputted
        LOGV("input buffer is null\n");
    }

    retains[OUTPORT_INDEX] = BUFFER_RETAIN_GETAGAIN; //set to default value
    //just call getoutput if no frame encoded in this cycle to avoid retained buffer queue wrong state
    if (!FrameEncoded) {
        OMX_BOOL OutBufReturned = OMX_FALSE;
        oret = ProcessDataRetrieve(buffers, &OutBufReturned);
        if (OutBufReturned)
            retains[OUTPORT_INDEX] = BUFFER_RETAIN_NOT_RETAIN;
    }

    LOGV("ProcessorProcess ret=%x", oret);
    return oret;

}

OMX_ERRORTYPE OMXVideoEncoderAVC::BuildHandlerList(void) {
    OMXVideoEncoderBase::BuildHandlerList();
    AddHandler(OMX_IndexParamVideoAvc, GetParamVideoAvc, SetParamVideoAvc);
    AddHandler((OMX_INDEXTYPE)OMX_IndexParamNalStreamFormat, GetParamNalStreamFormat, SetParamNalStreamFormat);
    AddHandler((OMX_INDEXTYPE)OMX_IndexParamNalStreamFormatSupported, GetParamNalStreamFormatSupported, SetParamNalStreamFormatSupported);
    AddHandler((OMX_INDEXTYPE)OMX_IndexParamNalStreamFormatSelect, GetParamNalStreamFormatSelect, SetParamNalStreamFormatSelect);
    AddHandler(OMX_IndexConfigVideoAVCIntraPeriod, GetConfigVideoAVCIntraPeriod, SetConfigVideoAVCIntraPeriod);
    AddHandler(OMX_IndexConfigVideoNalSize, GetConfigVideoNalSize, SetConfigVideoNalSize);
    AddHandler((OMX_INDEXTYPE)OMX_IndexConfigIntelSliceNumbers, GetConfigIntelSliceNumbers, SetConfigIntelSliceNumbers);
    AddHandler((OMX_INDEXTYPE)OMX_IndexParamIntelAVCVUI, GetParamIntelAVCVUI, SetParamIntelAVCVUI);
    AddHandler((OMX_INDEXTYPE)OMX_IndexParamVideoBytestream, GetParamVideoBytestream, SetParamVideoBytestream);
    AddHandler((OMX_INDEXTYPE)OMX_IndexParamVideoProfileLevelQuerySupported, GetParamVideoProfileLevelQuerySupported, SetParamVideoProfileLevelQuerySupported);
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoEncoderAVC::GetParamVideoProfileLevelQuerySupported(OMX_PTR pStructure) {
    OMX_ERRORTYPE ret;
    OMX_VIDEO_PARAM_PROFILELEVELTYPE *p = (OMX_VIDEO_PARAM_PROFILELEVELTYPE *)pStructure;
    CHECK_TYPE_HEADER(p);
    CHECK_PORT_INDEX(p, OUTPORT_INDEX);

    CHECK_ENUMERATION_RANGE(p->nProfileIndex,mPLTableCount);

    p->eProfile = mPLTable[p->nProfileIndex].profile;
    p->eLevel = mPLTable[p->nProfileIndex].level;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoEncoderAVC::SetParamVideoProfileLevelQuerySupported(OMX_PTR) {
    LOGW("SetParamVideoAVCProfileLevel is not supported.");
    return OMX_ErrorUnsupportedSetting;
}

OMX_ERRORTYPE OMXVideoEncoderAVC::GetParamVideoAvc(OMX_PTR pStructure) {
    OMX_ERRORTYPE ret;
    OMX_VIDEO_PARAM_AVCTYPE *p = (OMX_VIDEO_PARAM_AVCTYPE *)pStructure;
    CHECK_TYPE_HEADER(p);
    CHECK_PORT_INDEX(p, OUTPORT_INDEX);

    mVideoEncoder->getParameters(mAVCParams);
    if(mParamAvc.eProfile == OMX_VIDEO_AVCProfileHigh)
    {
        mAVCParams->bEntropyCodingCABAC = 1;
        mAVCParams->bDirect8x8Inference = 1;
    }
    mParamAvc.bEntropyCodingCABAC = (OMX_BOOL)mAVCParams->bEntropyCodingCABAC;
    mParamAvc.bWeightedPPrediction = (OMX_BOOL)mAVCParams->bWeightedPPrediction;
    mParamAvc.nRefIdx10ActiveMinus1 = mAVCParams->refIdx10ActiveMinus1;
    mParamAvc.nRefIdx11ActiveMinus1 = mAVCParams->refIdx11ActiveMinus1;
    mParamAvc.nWeightedBipredicitonMode = mAVCParams->weightedBipredicitonMode;
    mParamAvc.bDirect8x8Inference = (OMX_BOOL)mAVCParams->bDirect8x8Inference;
    mParamAvc.bDirectSpatialTemporal = (OMX_BOOL)mAVCParams->bDirectSpatialTemporal;
    mParamAvc.nCabacInitIdc = mAVCParams->cabacInitIdc;
    mParamAvc.bFrameMBsOnly = (OMX_BOOL)mAVCParams->bFrameMBsOnly;
    mParamAvc.bconstIpred = (OMX_BOOL)mAVCParams->bConstIpred;
    memcpy(p, &mParamAvc, sizeof(*p));
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoEncoderAVC::SetParamVideoAvc(OMX_PTR pStructure) {
    OMX_ERRORTYPE ret;
    OMX_VIDEO_PARAM_AVCTYPE *p = (OMX_VIDEO_PARAM_AVCTYPE *)pStructure;
    CHECK_TYPE_HEADER(p);
    CHECK_PORT_INDEX(p, OUTPORT_INDEX);
    CHECK_SET_PARAM_STATE();

    //Check if parameters are valid

    if(p->bEnableASO == OMX_TRUE)
        return OMX_ErrorUnsupportedSetting;

    if(p->bEnableFMO == OMX_TRUE)
        return OMX_ErrorUnsupportedSetting;

    if(p->bEnableUEP == OMX_TRUE)
        return OMX_ErrorUnsupportedSetting;

    if(p->bEnableRS == OMX_TRUE)
        return OMX_ErrorUnsupportedSetting;

    if (p->eProfile == OMX_VIDEO_AVCProfileBaseline &&
            (p->nAllowedPictureTypes & OMX_VIDEO_PictureTypeB) )
        return OMX_ErrorBadParameter;

    if (p->nAllowedPictureTypes & OMX_VIDEO_PictureTypeP && (p->nPFrames == 0))
        return OMX_ErrorBadParameter;

    if (p->nAllowedPictureTypes & OMX_VIDEO_PictureTypeB ) {
        if (p->nBFrames == 0)
            return OMX_ErrorBadParameter;

        //IpPeriod must be integer
        uint32_t IntraPeriod = mParamAvc.nPFrames + mParamAvc.nBFrames ;
        if (IntraPeriod % mParamAvc.nPFrames != 0)
            return OMX_ErrorBadParameter;

        //IntraPeriod must be multipe of IpPeriod.
        uint32_t IpPeriod = IntraPeriod /mParamAvc.nPFrames;
        if (IntraPeriod % IpPeriod != 0)
            return OMX_ErrorBadParameter;
    }

    // TODO: do we need to check if port is enabled?
    // TODO: see SetPortAvcParam implementation - Can we make simple copy????
    memcpy(&mParamAvc, p, sizeof(mParamAvc));
    mVideoEncoder->getParameters(mAVCParams);
    mAVCParams->bEntropyCodingCABAC = mParamAvc.bEntropyCodingCABAC;
    mAVCParams->bDirect8x8Inference = mParamAvc.bDirect8x8Inference;
    if(mParamAvc.eProfile == OMX_VIDEO_AVCProfileBaseline){
        mAVCParams->bEntropyCodingCABAC = 0;
        mAVCParams->bDirect8x8Inference = 0;
    }
    mVideoEncoder->setParameters(mAVCParams);


    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoEncoderAVC::GetParamNalStreamFormat(OMX_PTR pStructure) {
    OMX_ERRORTYPE ret;
    OMX_NALSTREAMFORMATTYPE *p = (OMX_NALSTREAMFORMATTYPE *)pStructure;

    CHECK_TYPE_HEADER(p);
    CHECK_PORT_INDEX(p, OUTPORT_INDEX);
    // TODO: check if this is desired format
    p->eNaluFormat = mNalStreamFormat.eNaluFormat; //OMX_NaluFormatStartCodes;
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoEncoderAVC::SetParamNalStreamFormat(OMX_PTR pStructure) {
    OMX_ERRORTYPE ret;
    OMX_NALSTREAMFORMATTYPE *p = (OMX_NALSTREAMFORMATTYPE *)pStructure;

    CHECK_TYPE_HEADER(p);
    CHECK_PORT_INDEX(p, OUTPORT_INDEX);
    LOGV("p->eNaluFormat =%d\n",p->eNaluFormat);
    if(p->eNaluFormat != OMX_NaluFormatStartCodes &&
            p->eNaluFormat != (OMX_NALUFORMATSTYPE)OMX_NaluFormatStartCodesSeparateFirstHeader &&
            p->eNaluFormat != OMX_NaluFormatOneNaluPerBuffer &&
            p->eNaluFormat != (OMX_NALUFORMATSTYPE)OMX_NaluFormatLengthPrefixedSeparateFirstHeader) {
        LOGE("Format not support\n");
        return OMX_ErrorUnsupportedSetting;
    }
    mNalStreamFormat.eNaluFormat = p->eNaluFormat;
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoEncoderAVC::GetParamNalStreamFormatSupported(OMX_PTR pStructure) {
    OMX_ERRORTYPE ret;
    OMX_NALSTREAMFORMATTYPE *p = (OMX_NALSTREAMFORMATTYPE *)pStructure;

    CHECK_TYPE_HEADER(p);
    CHECK_PORT_INDEX(p, OUTPORT_INDEX);
    p->eNaluFormat = (OMX_NALUFORMATSTYPE)
                     (OMX_NaluFormatStartCodes |
                      OMX_NaluFormatStartCodesSeparateFirstHeader |
                      OMX_NaluFormatOneNaluPerBuffer|
                      OMX_NaluFormatLengthPrefixedSeparateFirstHeader);

    // TODO: check if this is desired format
    // OMX_NaluFormatFourByteInterleaveLength |
    // OMX_NaluFormatZeroByteInterleaveLength);
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoEncoderAVC::SetParamNalStreamFormatSupported(OMX_PTR) {
    LOGW("SetParamNalStreamFormatSupported is not supported.");
    return OMX_ErrorUnsupportedSetting;
}

OMX_ERRORTYPE OMXVideoEncoderAVC::GetParamNalStreamFormatSelect(OMX_PTR) {
    LOGW("GetParamNalStreamFormatSelect is not supported.");
    return OMX_ErrorUnsupportedSetting;
}

OMX_ERRORTYPE OMXVideoEncoderAVC::SetParamNalStreamFormatSelect(OMX_PTR pStructure) {
    OMX_ERRORTYPE ret;
    OMX_NALSTREAMFORMATTYPE *p = (OMX_NALSTREAMFORMATTYPE *)pStructure;
    CHECK_TYPE_HEADER(p);
    CHECK_PORT_INDEX(p, OUTPORT_INDEX);

    // return OMX_ErrorIncorrectStateOperation if not in Loaded state
    CHECK_SET_PARAM_STATE();

    if (p->eNaluFormat != OMX_NaluFormatStartCodes &&
            p->eNaluFormat != (OMX_NALUFORMATSTYPE)OMX_NaluFormatStartCodesSeparateFirstHeader &&
            p->eNaluFormat != OMX_NaluFormatOneNaluPerBuffer&&
            p->eNaluFormat != (OMX_NALUFORMATSTYPE)OMX_NaluFormatLengthPrefixedSeparateFirstHeader) {
        //p->eNaluFormat != OMX_NaluFormatFourByteInterleaveLength &&
        //p->eNaluFormat != OMX_NaluFormatZeroByteInterleaveLength) {
        // TODO: check if this is desried
        return OMX_ErrorBadParameter;
    }

    mNalStreamFormat = *p;
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoEncoderAVC::GetConfigVideoAVCIntraPeriod(OMX_PTR pStructure) {
    OMX_ERRORTYPE ret;
    OMX_VIDEO_CONFIG_AVCINTRAPERIOD *p = (OMX_VIDEO_CONFIG_AVCINTRAPERIOD *)pStructure;

    CHECK_TYPE_HEADER(p);
    CHECK_PORT_INDEX(p, OUTPORT_INDEX);
    // TODO: populate mConfigAvcIntraPeriod from VideoEncoder
    // return OMX_ErrorNotReady if VideoEncoder is not created.
    memcpy(p, &mConfigAvcIntraPeriod, sizeof(*p));
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoEncoderAVC::SetConfigVideoAVCIntraPeriod(OMX_PTR pStructure) {
    OMX_ERRORTYPE ret;
    Encode_Status retStatus = ENCODE_SUCCESS;
    OMX_VIDEO_CONFIG_AVCINTRAPERIOD *p = (OMX_VIDEO_CONFIG_AVCINTRAPERIOD *)pStructure;
    CHECK_TYPE_HEADER(p);
    CHECK_PORT_INDEX(p, OUTPORT_INDEX);

    // return OMX_ErrorNone if not in Executing state
    // TODO:  return OMX_ErrorIncorrectStateOperation?
    CHECK_SET_CONFIG_STATE();

    //check if parameters are valid
    if ( ( (mParamAvc.nAllowedPictureTypes & OMX_VIDEO_PictureTypeP) || 
           (mParamAvc.nAllowedPictureTypes & OMX_VIDEO_PictureTypeB) ) && 
         p->nPFrames == 0 )
        return OMX_ErrorBadParameter;

    // TODO: apply AVC Intra Period configuration in Executing state
    VideoConfigAVCIntraPeriod avcIntraPreriod;

    if (mParamAvc.nAllowedPictureTypes & OMX_VIDEO_PictureTypeB) {
        avcIntraPreriod.intraPeriod = p->nPFrames;
        if (p->nPFrames % mParamAvc.nBFrames != 0)
            return OMX_ErrorBadParameter;
        avcIntraPreriod.ipPeriod = p->nPFrames / mParamAvc.nBFrames;

        if (avcIntraPreriod.intraPeriod % avcIntraPreriod.ipPeriod != 0)
            return OMX_ErrorBadParameter;

        avcIntraPreriod.idrInterval = p->nIDRPeriod;
    } else {
        avcIntraPreriod.intraPeriod = p->nPFrames + 1;
        avcIntraPreriod.ipPeriod = 1;
        if (avcIntraPreriod.intraPeriod == 0)
            avcIntraPreriod.idrInterval = 0;
        else
            avcIntraPreriod.idrInterval = p->nIDRPeriod;
    }

    retStatus = mVideoEncoder->setConfig(&avcIntraPreriod);
    if(retStatus !=  ENCODE_SUCCESS) {
        LOGW("set avc intra period config failed");
    }

    mEncoderParams->intraPeriod = avcIntraPreriod.intraPeriod;
    mAVCParams->idrInterval = avcIntraPreriod.idrInterval;
    mAVCParams->ipPeriod = avcIntraPreriod.ipPeriod;

    mConfigAvcIntraPeriod = *p;
    mConfigAvcIntraPeriod.nIDRPeriod = avcIntraPreriod.idrInterval;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoEncoderAVC::GetConfigVideoNalSize(OMX_PTR pStructure) {
    OMX_ERRORTYPE ret;
    OMX_VIDEO_CONFIG_NALSIZE *p = (OMX_VIDEO_CONFIG_NALSIZE *)pStructure;

    CHECK_TYPE_HEADER(p);
    CHECK_PORT_INDEX(p, OUTPORT_INDEX);
    memcpy(p, &mConfigNalSize, sizeof(*p));
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoEncoderAVC::SetConfigVideoNalSize(OMX_PTR pStructure) {
    OMX_ERRORTYPE ret;
    Encode_Status retStatus = ENCODE_SUCCESS;
    if (mParamBitrate.eControlRate == OMX_Video_ControlRateMax) {
        LOGE("SetConfigVideoNalSize failed. Feature is disabled.");
        return OMX_ErrorUnsupportedIndex;
    }
    OMX_VIDEO_CONFIG_NALSIZE *p = (OMX_VIDEO_CONFIG_NALSIZE *)pStructure;
    CHECK_TYPE_HEADER(p);
    CHECK_PORT_INDEX(p, OUTPORT_INDEX);

    // set in either Loaded  state (ComponentSetParam) or Executing state (ComponentSetConfig)
    mConfigNalSize = *p;

    // return OMX_ErrorNone if not in Executing state
    // TODO: return OMX_ErrorIncorrectStateOperation?
    CHECK_SET_CONFIG_STATE();

    if (mParamBitrate.eControlRate != (OMX_VIDEO_CONTROLRATETYPE)OMX_Video_Intel_ControlRateVideoConferencingMode) {
        LOGE("SetConfigVideoNalSize failed. Feature is supported only in VCM.");
        return OMX_ErrorUnsupportedSetting;
    }
    VideoConfigNALSize configNalSize;
    configNalSize.maxSliceSize = mConfigNalSize.nNaluBytes * 8;
    retStatus = mVideoEncoder->setConfig(&configNalSize);
    if(retStatus != ENCODE_SUCCESS) {
        LOGW("set NAL size config failed");
    }
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoEncoderAVC::GetConfigIntelSliceNumbers(OMX_PTR pStructure) {
    OMX_ERRORTYPE ret;
    OMX_VIDEO_CONFIG_INTEL_SLICE_NUMBERS *p = (OMX_VIDEO_CONFIG_INTEL_SLICE_NUMBERS *)pStructure;

    CHECK_TYPE_HEADER(p);
    CHECK_PORT_INDEX(p, OUTPORT_INDEX);
    memcpy(p, &mConfigIntelSliceNumbers, sizeof(*p));
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoEncoderAVC::SetConfigIntelSliceNumbers(OMX_PTR pStructure) {
    OMX_ERRORTYPE ret;
    Encode_Status retStatus = ENCODE_SUCCESS;
    if (mParamBitrate.eControlRate == OMX_Video_ControlRateMax) {
        LOGE("SetConfigIntelSliceNumbers failed. Feature is disabled.");
        return OMX_ErrorUnsupportedIndex;
    }
    OMX_VIDEO_CONFIG_INTEL_SLICE_NUMBERS *p = (OMX_VIDEO_CONFIG_INTEL_SLICE_NUMBERS *)pStructure;
    CHECK_TYPE_HEADER(p);
    CHECK_PORT_INDEX(p, OUTPORT_INDEX);

    // set in either Loaded  state (ComponentSetParam) or Executing state (ComponentSetConfig)
    mConfigIntelSliceNumbers = *p;

    // return OMX_ErrorNone if not in Executing state
    // TODO: return OMX_ErrorIncorrectStateOperation?
    CHECK_SET_CONFIG_STATE();

    if (mParamBitrate.eControlRate != (OMX_VIDEO_CONTROLRATETYPE)OMX_Video_Intel_ControlRateVideoConferencingMode) {
        LOGE("SetConfigIntelSliceNumbers failed. Feature is supported only in VCM.");
        return OMX_ErrorUnsupportedSetting;
    }
    VideoConfigSliceNum sliceNum;
    sliceNum.sliceNum.iSliceNum = mConfigIntelSliceNumbers.nISliceNumber;
    sliceNum.sliceNum.pSliceNum = mConfigIntelSliceNumbers.nPSliceNumber;
    retStatus = mVideoEncoder->setConfig(&sliceNum);
    if(retStatus != ENCODE_SUCCESS) {
        LOGW("set silce num config failed!\n");
    }
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoEncoderAVC::GetParamIntelAVCVUI(OMX_PTR pStructure) {

    OMX_ERRORTYPE ret;
    OMX_VIDEO_PARAM_INTEL_AVCVUI *p = (OMX_VIDEO_PARAM_INTEL_AVCVUI *)pStructure;

    CHECK_TYPE_HEADER(p);
    CHECK_PORT_INDEX(p, OUTPORT_INDEX);
    memcpy(p, &mParamIntelAvcVui, sizeof(*p));

    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoEncoderAVC::SetParamIntelAVCVUI(OMX_PTR pStructure) {

    OMX_ERRORTYPE ret;
    OMX_VIDEO_PARAM_INTEL_AVCVUI *p = (OMX_VIDEO_PARAM_INTEL_AVCVUI *)pStructure;
    CHECK_TYPE_HEADER(p);
    CHECK_PORT_INDEX(p, OUTPORT_INDEX);

    // set only in Loaded state (ComponentSetParam)
    CHECK_SET_PARAM_STATE();

    mParamIntelAvcVui = *p;
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoEncoderAVC::GetParamVideoBytestream(OMX_PTR) {
    return OMX_ErrorUnsupportedSetting;
}

OMX_ERRORTYPE OMXVideoEncoderAVC::SetParamVideoBytestream(OMX_PTR pStructure) {
    OMX_ERRORTYPE ret;
    OMX_VIDEO_PARAM_BYTESTREAMTYPE *p = (OMX_VIDEO_PARAM_BYTESTREAMTYPE *)pStructure;
    CHECK_TYPE_HEADER(p);
    CHECK_PORT_INDEX(p, OUTPORT_INDEX);

    // set only in Loaded state (ComponentSetParam)
    CHECK_SET_PARAM_STATE();

    if (p->bBytestream == OMX_TRUE) {
        mNalStreamFormat.eNaluFormat = OMX_NaluFormatStartCodes;
    } else {
        // TODO: do we need to override the Nalu format?
        mNalStreamFormat.eNaluFormat = (OMX_NALUFORMATSTYPE)OMX_NaluFormatZeroByteInterleaveLength;
    }

    return OMX_ErrorNone;
}


DECLARE_OMX_COMPONENT("OMX.Intel.VideoEncoder.AVC", "video_encoder.avc", OMXVideoEncoderAVC);
