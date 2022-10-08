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

#ifndef __PV_SOFT_MPEG4_ENCODER__
#define __PV_SOFT_MPEG4_ENCODER__

#include <va/va.h>
#include <va/va_tpi.h>
#include "VideoEncoderDef.h"
#include "VideoEncoderInterface.h"
#include "IntelMetadataBuffer.h"

#include <media/stagefright/MediaBuffer.h>
#include <media/stagefright/foundation/ABase.h>
#include "SimpleSoftOMXComponent.h"
#include "mp4enc_api.h"

class PVSoftMPEG4Encoder : IVideoEncoder {

public:
    PVSoftMPEG4Encoder(const char *name);
    virtual ~PVSoftMPEG4Encoder();

    virtual Encode_Status start(void) {return initEncoder();}
    virtual void flush(void) { }
    virtual Encode_Status stop(void) {return releaseEncoder();}
    virtual Encode_Status encode(VideoEncRawBuffer *inBuffer, uint32_t timeout);

    virtual Encode_Status getOutput(VideoEncOutputBuffer *outBuffer, uint32_t timeout);

    virtual Encode_Status getParameters(VideoParamConfigSet *videoEncParams);
    virtual Encode_Status setParameters(VideoParamConfigSet *videoEncParams);
    virtual Encode_Status setConfig(VideoParamConfigSet *videoEncConfig) {return ENCODE_SUCCESS;}
    virtual Encode_Status getConfig(VideoParamConfigSet *videoEncConfig) {return ENCODE_SUCCESS;}
    virtual Encode_Status getMaxOutSize(uint32_t *maxSize) {return ENCODE_SUCCESS;}

private:
    void setDefaultParams(void);
    VideoParamsCommon mComParams;

    MP4EncodingMode mEncodeMode;
    int32_t  mVideoWidth;
    int32_t  mVideoHeight;
    int32_t  mVideoFrameRate;
    int32_t  mVideoBitRate;
    int32_t  mVideoColorFormat;
    bool     mStoreMetaDataInBuffers;
    int32_t  mIDRFrameRefreshIntervalInSec;

    int64_t  mNumInputFrames;
    bool     mStarted;
    bool     mSawInputEOS;
    bool     mSignalledError;
    int64_t mCurTimestampUs;
    int64_t mLastTimestampUs;

    tagvideoEncControls   *mHandle;
    tagvideoEncOptions    *mEncParams;
    uint8_t               *mInputFrameData;
    uint8_t               *mTrimedInputData;
    uint8_t mVolHeader[256];
    int32_t mVolHeaderLength;

    Encode_Status initEncParams();
    Encode_Status initEncoder();
    Encode_Status releaseEncoder();

    DISALLOW_EVIL_CONSTRUCTORS(PVSoftMPEG4Encoder);
};

#endif
