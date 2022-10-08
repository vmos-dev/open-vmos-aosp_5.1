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


#ifndef OMX_VIDEO_ENCODER_AVC_H_
#define OMX_VIDEO_ENCODER_AVC_H_


#include "OMXVideoEncoderBase.h"
#include <utils/List.h>
#include <IntelMetadataBuffer.h>
#include <OMX_VideoExt.h>
enum {
    F_UNKNOWN = 0x00,    // Unknown
    F_I       = 0x01,    // General I-frame type
    F_P       = 0x02,    // General P-frame type
    F_B       = 0x03,    // General B-frame type
    F_SI      = 0x04,    // H.263 SI-frame type
    F_SP      = 0x05,    // H.263 SP-frame type
    F_EI      = 0x06,    // H.264 EI-frame type
    F_EP      = 0x07,    // H.264 EP-frame type
    F_S       = 0x08,    // MPEG-4 S-frame type
    F_IDR     = 0x09,    // IDR-frame type
};

enum {
    CACHE_NONE    = 0x00,    //nothing to be done
    CACHE_PUSH    = 0x01,    //push this frame into cache
    CACHE_POP     = 0x02,    //pop all from cache into queue head by STACK rule
    CACHE_RESET   = 0x03,    //reset cache, clear all cached frames
};

#define ENC_NSTOP    0x02000000

#define GET_FT(x)  ( (x & 0xF0000000 ) >> 28 )       //get frame type
#define GET_CO(x)  ( (x & 0x0C000000 ) >> 26 )       //get cache operation
#define GET_FC(x)  ( (x & 0x01FFFFFF ) )             //get frame count

#define SET_FT(x, y)  { x = ((x & ~0xF0000000) | (y << 28)); }
#define SET_CO(x, y)  { x = ((x & ~0x0C000000) | (y << 26 )); }
#define SET_FC(x, y)  { x = ((x & ~0x01FFFFFF) | (y & 0x01FFFFFF )); }

const char* FrameTypeStr[10] = {"UNKNOWN", "I", "P", "B", "SI", "SP", "EI", "EP", "S", "IDR"};
const char* CacheOperationStr[4]= {"NONE", "PUSH", "POP", "RESET"};

typedef struct {
    uint32_t FrameType;
    uint32_t CacheOperation;
    bool NotStopFrame;
    uint32_t FrameCount;
}Encode_Info;

#define MAX_H264_PROFILE 3

class OMXVideoEncoderAVC : public OMXVideoEncoderBase {
public:
    OMXVideoEncoderAVC();
    virtual ~OMXVideoEncoderAVC();

protected:
    virtual OMX_ERRORTYPE InitOutputPortFormatSpecific(OMX_PARAM_PORTDEFINITIONTYPE *paramPortDefinitionOutput);
    virtual OMX_ERRORTYPE ProcessorInit(void);
    virtual OMX_ERRORTYPE ProcessorDeinit(void);
    virtual OMX_ERRORTYPE ProcessorStop(void);
    virtual OMX_ERRORTYPE ProcessorProcess(
            OMX_BUFFERHEADERTYPE **buffers,
            buffer_retain_t *retains,
            OMX_U32 numberBuffers);
    virtual OMX_ERRORTYPE ProcessorPreEmptyBuffer(OMX_BUFFERHEADERTYPE* buffer);

    virtual OMX_ERRORTYPE BuildHandlerList(void);
    virtual OMX_ERRORTYPE SetVideoEncoderParam();
    DECLARE_HANDLER(OMXVideoEncoderAVC, ParamVideoAvc);
    DECLARE_HANDLER(OMXVideoEncoderAVC, ParamNalStreamFormat);
    DECLARE_HANDLER(OMXVideoEncoderAVC, ParamNalStreamFormatSupported);
    DECLARE_HANDLER(OMXVideoEncoderAVC, ParamNalStreamFormatSelect);
    DECLARE_HANDLER(OMXVideoEncoderAVC, ConfigVideoAVCIntraPeriod);
    DECLARE_HANDLER(OMXVideoEncoderAVC, ConfigVideoNalSize);
    DECLARE_HANDLER(OMXVideoEncoderAVC, ParamIntelAVCVUI);
    DECLARE_HANDLER(OMXVideoEncoderAVC, ParamVideoBytestream);
    DECLARE_HANDLER(OMXVideoEncoderAVC, ConfigIntelSliceNumbers);
    DECLARE_HANDLER(OMXVideoEncoderAVC, ParamVideoProfileLevelQuerySupported);


private:
    enum {
        // OMX_PARAM_PORTDEFINITIONTYPE
        OUTPORT_MIN_BUFFER_COUNT = 1,
        OUTPORT_ACTUAL_BUFFER_COUNT = 6,
        OUTPORT_BUFFER_SIZE = 1382400,
        NUM_REFERENCE_FRAME = 4,
    };

    OMX_VIDEO_PARAM_AVCTYPE mParamAvc;
    OMX_NALSTREAMFORMATTYPE mNalStreamFormat;
    OMX_VIDEO_CONFIG_AVCINTRAPERIOD mConfigAvcIntraPeriod;
    OMX_VIDEO_CONFIG_NALSIZE mConfigNalSize;
    OMX_VIDEO_PARAM_INTEL_AVCVUI mParamIntelAvcVui;
    OMX_VIDEO_CONFIG_INTEL_SLICE_NUMBERS mConfigIntelSliceNumbers;
    VideoParamsAVC *mAVCParams;

    OMX_U32 mInputPictureCount;
    OMX_U32 mFrameEncodedCount;

    List<OMX_BUFFERHEADERTYPE*> mBFrameList;

    OMX_BOOL ProcessCacheOperation(OMX_BUFFERHEADERTYPE **buffers);
    OMX_ERRORTYPE ProcessDataRetrieve(
            OMX_BUFFERHEADERTYPE **buffers,
            OMX_BOOL *outBufReturned);

    struct ProfileLevelTable {
        OMX_U32 profile;
        OMX_U32 level;
    };

    ProfileLevelTable mPLTable[MAX_H264_PROFILE];
    OMX_U32 mPLTableCount;

    OMX_BOOL mEmptyEOSBuf;
    OMX_BOOL mCSDOutputted;
};

#endif /* OMX_VIDEO_ENCODER_AVC_H_ */
