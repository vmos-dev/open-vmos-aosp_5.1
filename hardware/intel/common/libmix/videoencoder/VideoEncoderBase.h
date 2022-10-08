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

#ifndef __VIDEO_ENCODER_BASE_H__
#define __VIDEO_ENCODER_BASE_H__

#include <va/va.h>
#include <va/va_tpi.h>
#include "VideoEncoderDef.h"
#include "VideoEncoderInterface.h"
#include "IntelMetadataBuffer.h"
#include <utils/List.h>
#include <utils/threads.h>
#include "VideoEncoderUtils.h"

struct SurfaceMap {
    VASurfaceID surface;
    VASurfaceID surface_backup;
    IntelMetadataBufferType type;
    int32_t value;
    ValueInfo vinfo;
    bool added;
};

struct EncodeTask {
    VASurfaceID enc_surface;
    VASurfaceID ref_surface;
    VASurfaceID rec_surface;
    VABufferID coded_buffer;

    FrameType type;
    int flag;
    int64_t timestamp;  //corresponding input frame timestamp
    void *priv;  //input buffer data

    bool completed;   //if encode task is done complet by HW
};

class VideoEncoderBase : IVideoEncoder {

public:
    VideoEncoderBase();
    virtual ~VideoEncoderBase();

    virtual Encode_Status start(void);
    virtual void flush(void);
    virtual Encode_Status stop(void);
    virtual Encode_Status encode(VideoEncRawBuffer *inBuffer, uint32_t timeout);

    /*
    * getOutput can be called several time for a frame (such as first time  codec data, and second time others)
    * encoder will provide encoded data according to the format (whole frame, codec_data, sigle NAL etc)
    * If the buffer passed to encoded is not big enough, this API call will return ENCODE_BUFFER_TOO_SMALL
    * and caller should provide a big enough buffer and call again
    */
    virtual Encode_Status getOutput(VideoEncOutputBuffer *outBuffer, uint32_t timeout);

    virtual Encode_Status getParameters(VideoParamConfigSet *videoEncParams);
    virtual Encode_Status setParameters(VideoParamConfigSet *videoEncParams);
    virtual Encode_Status setConfig(VideoParamConfigSet *videoEncConfig);
    virtual Encode_Status getConfig(VideoParamConfigSet *videoEncConfig);
    virtual Encode_Status getMaxOutSize(uint32_t *maxSize);

protected:
    virtual Encode_Status sendEncodeCommand(EncodeTask* task) = 0;
    virtual Encode_Status derivedSetParams(VideoParamConfigSet *videoEncParams) = 0;
    virtual Encode_Status derivedGetParams(VideoParamConfigSet *videoEncParams) = 0;
    virtual Encode_Status derivedGetConfig(VideoParamConfigSet *videoEncConfig) = 0;
    virtual Encode_Status derivedSetConfig(VideoParamConfigSet *videoEncConfig) = 0;
    virtual Encode_Status getExtFormatOutput(VideoEncOutputBuffer *outBuffer) = 0;
    virtual Encode_Status updateFrameInfo(EncodeTask* task) ;

    Encode_Status renderDynamicFrameRate();
    Encode_Status renderDynamicBitrate(EncodeTask* task);
    Encode_Status renderHrd();
    Encode_Status queryProfileLevelConfig(VADisplay dpy, VAProfile profile);

private:
    void setDefaultParams(void);
    Encode_Status setUpstreamBuffer(VideoParamsUpstreamBuffer *upStreamBuffer);
    Encode_Status getNewUsrptrFromSurface(uint32_t width, uint32_t height, uint32_t format,
            uint32_t expectedSize, uint32_t *outsize, uint32_t *stride, uint8_t **usrptr);
    VASurfaceMap* findSurfaceMapByValue(intptr_t value);
    Encode_Status manageSrcSurface(VideoEncRawBuffer *inBuffer, VASurfaceID *sid);
    void PrepareFrameInfo(EncodeTask* task);

    Encode_Status prepareForOutput(VideoEncOutputBuffer *outBuffer, bool *useLocalBuffer);
    Encode_Status cleanupForOutput();
    Encode_Status outputAllData(VideoEncOutputBuffer *outBuffer);
    Encode_Status queryAutoReferenceConfig(VAProfile profile);
    Encode_Status querySupportedSurfaceMemTypes();
    Encode_Status copySurfaces(VASurfaceID srcId, VASurfaceID destId);
    VASurfaceID CreateSurfaceFromExternalBuf(int32_t value, ValueInfo& vinfo);

protected:

    bool mInitialized;
    bool mStarted;
    VADisplay mVADisplay;
    VAContextID mVAContext;
    VAConfigID mVAConfig;
    VAEntrypoint mVAEntrypoint;


    VideoParamsCommon mComParams;
    VideoParamsHRD mHrdParam;
    VideoParamsStoreMetaDataInBuffers mStoreMetaDataInBuffers;

    bool mNewHeader;

    bool mRenderMaxSliceSize; //Max Slice Size
    bool mRenderQP;
    bool mRenderAIR;
    bool mRenderCIR;
    bool mRenderFrameRate;
    bool mRenderBitRate;
    bool mRenderHrd;
    bool mRenderMaxFrameSize;
    bool mRenderMultiTemporal;
    bool mForceKFrame;

    VABufferID mSeqParamBuf;
    VABufferID mRcParamBuf;
    VABufferID mFrameRateParamBuf;
    VABufferID mPicParamBuf;
    VABufferID mSliceParamBuf;
    VASurfaceID* mAutoRefSurfaces;

    android::List <VASurfaceMap *> mSrcSurfaceMapList;  //all mapped surface info list from input buffer
    android::List <EncodeTask *> mEncodeTaskList;  //all encode tasks list
    android::List <VABufferID> mVACodedBufferList;  //all available codedbuffer list

    VASurfaceID mRefSurface;        //reference surface, only used in base
    VASurfaceID mRecSurface;        //reconstructed surface, only used in base
    uint32_t mFrameNum;
    uint32_t mCodedBufSize;
    bool mAutoReference;
    uint32_t mAutoReferenceSurfaceNum;
    uint32_t mEncPackedHeaders;
    uint32_t mEncMaxRefFrames;

    bool mSliceSizeOverflow;

    //Current Outputting task
    EncodeTask *mCurOutputTask;

    //Current outputting CodedBuffer status
    VABufferID mOutCodedBuffer;
    bool mCodedBufferMapped;
    uint8_t *mOutCodedBufferPtr;
    VACodedBufferSegment *mCurSegment;
    uint32_t mOffsetInSeg;
    uint32_t mTotalSize;
    uint32_t mTotalSizeCopied;
    android::Mutex               mCodedBuffer_Lock, mEncodeTask_Lock;
    android::Condition           mCodedBuffer_Cond, mEncodeTask_Cond;

    bool mFrameSkipped;

    //supported surface memory types
    int mSupportedSurfaceMemType;

    //VASurface mapping extra action
    int mVASurfaceMappingAction;

    // For Temporal Layer Bitrate FrameRate settings
    VideoConfigTemperalLayerBitrateFramerate mTemporalLayerBitrateFramerate[3];

#ifdef INTEL_VIDEO_XPROC_SHARING
    uint32_t mSessionFlag;
#endif
};
#endif /* __VIDEO_ENCODER_BASE_H__ */
