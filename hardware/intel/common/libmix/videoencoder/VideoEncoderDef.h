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

#ifndef __VIDEO_ENCODER_DEF_H__
#define __VIDEO_ENCODER_DEF_H__

#include <stdint.h>

#define STRING_TO_FOURCC(format) ((uint32_t)(((format)[0])|((format)[1]<<8)|((format)[2]<<16)|((format)[3]<<24)))
#define min(X,Y) (((X) < (Y)) ? (X) : (Y))
#define max(X,Y) (((X) > (Y)) ? (X) : (Y))

typedef int32_t Encode_Status;

// Video encode error code
enum {
    ENCODE_INVALID_SURFACE = -11,
    ENCODE_NO_REQUEST_DATA = -10,
    ENCODE_WRONG_STATE = -9,
    ENCODE_NOTIMPL = -8,
    ENCODE_NO_MEMORY = -7,
    ENCODE_NOT_INIT = -6,
    ENCODE_DRIVER_FAIL = -5,
    ENCODE_INVALID_PARAMS = -4,
    ENCODE_NOT_SUPPORTED = -3,
    ENCODE_NULL_PTR = -2,
    ENCODE_FAIL = -1,
    ENCODE_SUCCESS = 0,
    ENCODE_ALREADY_INIT = 1,
    ENCODE_SLICESIZE_OVERFLOW = 2,
    ENCODE_BUFFER_TOO_SMALL = 3, // The buffer passed to encode is too small to contain encoded data
    ENCODE_DEVICE_BUSY = 4,
    ENCODE_DATA_NOT_READY = 5,
};

typedef enum {
    OUTPUT_EVERYTHING = 0,  //Output whatever driver generates
    OUTPUT_CODEC_DATA = 1,
    OUTPUT_FRAME_DATA = 2, //Equal to OUTPUT_EVERYTHING when no header along with the frame data
    OUTPUT_ONE_NAL = 4,
    OUTPUT_ONE_NAL_WITHOUT_STARTCODE = 8,
    OUTPUT_LENGTH_PREFIXED = 16,
    OUTPUT_CODEDBUFFER = 32,
    OUTPUT_NALULENGTHS_PREFIXED = 64,
    OUTPUT_BUFFER_LAST
} VideoOutputFormat;

typedef enum {
    RAW_FORMAT_NONE = 0,
    RAW_FORMAT_YUV420 = 1,
    RAW_FORMAT_YUV422 = 2,
    RAW_FORMAT_YUV444 = 4,
    RAW_FORMAT_NV12 = 8,
    RAW_FORMAT_RGBA = 16,
    RAW_FORMAT_OPAQUE = 32,
    RAW_FORMAT_PROTECTED = 0x80000000,
    RAW_FORMAT_LAST
} VideoRawFormat;

typedef enum {
    RATE_CONTROL_NONE = 1,
    RATE_CONTROL_CBR = 2,
    RATE_CONTROL_VBR = 4,
    RATE_CONTROL_VCM = 8,
    RATE_CONTROL_LAST
} VideoRateControl;

typedef enum {
    PROFILE_MPEG2SIMPLE = 0,
    PROFILE_MPEG2MAIN,
    PROFILE_MPEG4SIMPLE,
    PROFILE_MPEG4ADVANCEDSIMPLE,
    PROFILE_MPEG4MAIN,
    PROFILE_H264BASELINE,
    PROFILE_H264MAIN,
    PROFILE_H264HIGH,
    PROFILE_VC1SIMPLE,
    PROFILE_VC1MAIN,
    PROFILE_VC1ADVANCED,
    PROFILE_H263BASELINE
} VideoProfile;

typedef enum {
    AVC_DELIMITER_LENGTHPREFIX = 0,
    AVC_DELIMITER_ANNEXB
} AVCDelimiterType;

typedef enum {
    VIDEO_ENC_NONIR,       // Non intra refresh
    VIDEO_ENC_CIR, 		// Cyclic intra refresh
    VIDEO_ENC_AIR, 		// Adaptive intra refresh
    VIDEO_ENC_BOTH,
    VIDEO_ENC_LAST
} VideoIntraRefreshType;

enum VideoBufferSharingMode {
    BUFFER_SHARING_NONE = 1, //Means non shared buffer mode
    BUFFER_SHARING_CI = 2,
    BUFFER_SHARING_V4L2 = 4,
    BUFFER_SHARING_SURFACE = 8,
    BUFFER_SHARING_USRPTR = 16,
    BUFFER_SHARING_GFXHANDLE = 32,
    BUFFER_SHARING_KBUFHANDLE = 64,
    BUFFER_LAST
};

typedef enum {
    FTYPE_UNKNOWN = 0, // Unknown
    FTYPE_I = 1, // General I-frame type
    FTYPE_P = 2, // General P-frame type
    FTYPE_B = 3, // General B-frame type
    FTYPE_SI = 4, // H.263 SI-frame type
    FTYPE_SP = 5, // H.263 SP-frame type
    FTYPE_EI = 6, // H.264 EI-frame type
    FTYPE_EP = 7, // H.264 EP-frame type
    FTYPE_S = 8, // MPEG-4 S-frame type
    FTYPE_IDR = 9, // IDR-frame type
}FrameType;

//function call mode
#define FUNC_BLOCK        0xFFFFFFFF
#define FUNC_NONBLOCK        0

// Output buffer flag
#define ENCODE_BUFFERFLAG_ENDOFFRAME       0x00000001
#define ENCODE_BUFFERFLAG_PARTIALFRAME     0x00000002
#define ENCODE_BUFFERFLAG_SYNCFRAME        0x00000004
#define ENCODE_BUFFERFLAG_CODECCONFIG      0x00000008
#define ENCODE_BUFFERFLAG_DATACORRUPT      0x00000010
#define ENCODE_BUFFERFLAG_DATAINVALID      0x00000020
#define ENCODE_BUFFERFLAG_SLICEOVERFOLOW   0x00000040
#define ENCODE_BUFFERFLAG_ENDOFSTREAM     0x00000080
#define ENCODE_BUFFERFLAG_NSTOPFRAME        0x00000100

typedef struct {
    uint8_t *data;
    uint32_t bufferSize; //buffer size
    uint32_t dataSize; //actual size
    uint32_t offset; //buffer offset
    uint32_t remainingSize;
    int flag; //Key frame, Codec Data etc
    VideoOutputFormat format; //output format
    int64_t timeStamp; //reserved
    FrameType type;
    void *priv; //indicate corresponding input data
} VideoEncOutputBuffer;

typedef struct {
    uint8_t *data;
    uint32_t size;
    bool bufAvailable; //To indicate whether this buffer can be reused
    int64_t timeStamp; //reserved
    FrameType type; //frame type expected to be encoded
    int flag; // flag to indicate buffer property
    void *priv; //indicate corresponding input data
} VideoEncRawBuffer;

struct VideoEncSurfaceBuffer {
    VASurfaceID surface;
    uint8_t *usrptr;
    uint32_t index;
    bool bufAvailable;
    VideoEncSurfaceBuffer *next;
};

struct CirParams {
    uint32_t cir_num_mbs;

    CirParams &operator=(const CirParams &other) {
        if (this == &other) return *this;

        this->cir_num_mbs = other.cir_num_mbs;
        return *this;
    }
};

struct AirParams {
    uint32_t airMBs;
    uint32_t airThreshold;
    uint32_t airAuto;

    AirParams &operator=(const AirParams &other) {
        if (this == &other) return *this;

        this->airMBs= other.airMBs;
        this->airThreshold= other.airThreshold;
        this->airAuto = other.airAuto;
        return *this;
    }
};

struct VideoFrameRate {
    uint32_t frameRateNum;
    uint32_t frameRateDenom;

    VideoFrameRate &operator=(const VideoFrameRate &other) {
        if (this == &other) return *this;

        this->frameRateNum = other.frameRateNum;
        this->frameRateDenom = other.frameRateDenom;
        return *this;
    }
};

struct VideoResolution {
    uint32_t width;
    uint32_t height;

    VideoResolution &operator=(const VideoResolution &other) {
        if (this == &other) return *this;

        this->width = other.width;
        this->height = other.height;
        return *this;
    }
};

struct VideoRateControlParams {
    uint32_t bitRate;
    uint32_t initQP;
    uint32_t minQP;
    uint32_t maxQP;
    uint32_t I_minQP;
    uint32_t I_maxQP;
    uint32_t windowSize;
    uint32_t targetPercentage;
    uint32_t disableFrameSkip;
    uint32_t disableBitsStuffing;
    uint32_t enableIntraFrameQPControl;
    uint32_t temporalFrameRate;
    uint32_t temporalID;

    VideoRateControlParams &operator=(const VideoRateControlParams &other) {
        if (this == &other) return *this;

        this->bitRate = other.bitRate;
        this->initQP = other.initQP;
        this->minQP = other.minQP;
        this->maxQP = other.maxQP;
        this->I_minQP = other.I_minQP;
        this->I_maxQP = other.I_maxQP;
        this->windowSize = other.windowSize;
        this->targetPercentage = other.targetPercentage;
        this->disableFrameSkip = other.disableFrameSkip;
        this->disableBitsStuffing = other.disableBitsStuffing;
        this->enableIntraFrameQPControl = other.enableIntraFrameQPControl;
        this->temporalFrameRate = other.temporalFrameRate;
        this->temporalID = other.temporalID;

        return *this;
    }
};

struct SliceNum {
    uint32_t iSliceNum;
    uint32_t pSliceNum;

    SliceNum &operator=(const SliceNum &other) {
        if (this == &other) return *this;

        this->iSliceNum = other.iSliceNum;
        this->pSliceNum= other.pSliceNum;
        return *this;
    }
};

typedef struct {
    uint32_t realWidth;
    uint32_t realHeight;
    uint32_t lumaStride;
    uint32_t chromStride;
    uint32_t format;
} ExternalBufferAttrib;

struct Cropping {
    uint32_t LeftOffset;
    uint32_t RightOffset;
    uint32_t TopOffset;
    uint32_t BottomOffset;

    Cropping &operator=(const Cropping &other) {
        if (this == &other) return *this;

        this->LeftOffset = other.LeftOffset;
        this->RightOffset = other.RightOffset;
        this->TopOffset = other.TopOffset;
        this->BottomOffset = other.BottomOffset;
        return *this;
    }
};

struct SamplingAspectRatio {
    uint16_t SarWidth;
    uint16_t SarHeight;

    SamplingAspectRatio &operator=(const SamplingAspectRatio &other) {
        if (this == &other) return *this;

        this->SarWidth = other.SarWidth;
        this->SarHeight = other.SarHeight;
        return *this;
    }
};

enum VideoParamConfigType {
    VideoParamsTypeStartUnused = 0x01000000,
    VideoParamsTypeCommon,
    VideoParamsTypeAVC,
    VideoParamsTypeH263,
    VideoParamsTypeMP4,
    VideoParamsTypeVC1,
    VideoParamsTypeUpSteamBuffer,
    VideoParamsTypeUsrptrBuffer,
    VideoParamsTypeHRD,
    VideoParamsTypeStoreMetaDataInBuffers,
    VideoParamsTypeProfileLevel,
    VideoParamsTypeVP8,
    VideoParamsTypeTemporalLayer,

    VideoConfigTypeFrameRate,
    VideoConfigTypeBitRate,
    VideoConfigTypeResolution,
    VideoConfigTypeIntraRefreshType,
    VideoConfigTypeAIR,
    VideoConfigTypeCyclicFrameInterval,
    VideoConfigTypeAVCIntraPeriod,
    VideoConfigTypeNALSize,
    VideoConfigTypeIDRRequest,
    VideoConfigTypeSliceNum,
    VideoConfigTypeVP8,
    VideoConfigTypeVP8ReferenceFrame,
    VideoConfigTypeCIR,
    VideoConfigTypeVP8MaxFrameSizeRatio,
    VideoConfigTypeTemperalLayerBitrateFramerate,

    VideoParamsConfigExtension
};

struct VideoParamConfigSet {
    VideoParamConfigType type;
    uint32_t size;

    VideoParamConfigSet &operator=(const VideoParamConfigSet &other) {
        if (this == &other) return *this;
        this->type = other.type;
        this->size = other.size;
        return *this;
    }
};

struct VideoParamsCommon : VideoParamConfigSet {

    VAProfile profile;
    uint8_t level;
    VideoRawFormat rawFormat;
    VideoResolution resolution;
    VideoFrameRate frameRate;
    int32_t intraPeriod;
    VideoRateControl rcMode;
    VideoRateControlParams rcParams;
    VideoIntraRefreshType refreshType;
    int32_t cyclicFrameInterval;
    AirParams airParams;
    CirParams cirParams;
    uint32_t disableDeblocking;
    bool syncEncMode;
    //CodedBuffer properties
    uint32_t codedBufNum;
    uint32_t numberOfLayer;
    uint32_t nPeriodicity;
    uint32_t nLayerID[32];

    VideoParamsCommon() {
        type = VideoParamsTypeCommon;
        size = sizeof(VideoParamsCommon);
    }

    VideoParamsCommon &operator=(const VideoParamsCommon &other) {
        if (this == &other) return *this;

        VideoParamConfigSet::operator=(other);
        this->profile = other.profile;
        this->level = other.level;
        this->rawFormat = other.rawFormat;
        this->resolution = other.resolution;
        this->frameRate = other.frameRate;
        this->intraPeriod = other.intraPeriod;
        this->rcMode = other.rcMode;
        this->rcParams = other.rcParams;
        this->refreshType = other.refreshType;
        this->cyclicFrameInterval = other.cyclicFrameInterval;
        this->airParams = other.airParams;
        this->disableDeblocking = other.disableDeblocking;
        this->syncEncMode = other.syncEncMode;
        this->codedBufNum = other.codedBufNum;
        this->numberOfLayer = other.numberOfLayer;
        return *this;
    }
};

struct VideoParamsAVC : VideoParamConfigSet {
    uint32_t basicUnitSize;  //for rate control
    uint8_t VUIFlag;
    int32_t maxSliceSize;
    uint32_t idrInterval;
    uint32_t ipPeriod;
    uint32_t refFrames;
    SliceNum sliceNum;
    AVCDelimiterType delimiterType;
    Cropping crop;
    SamplingAspectRatio SAR;
    uint32_t refIdx10ActiveMinus1;
    uint32_t refIdx11ActiveMinus1;
    bool bFrameMBsOnly;
    bool bMBAFF;
    bool bEntropyCodingCABAC;
    bool bWeightedPPrediction;
    uint32_t weightedBipredicitonMode;
    bool bConstIpred ;
    bool bDirect8x8Inference;
    bool bDirectSpatialTemporal;
    uint32_t cabacInitIdc;

    VideoParamsAVC() {
        type = VideoParamsTypeAVC;
        size = sizeof(VideoParamsAVC);
    }

    VideoParamsAVC &operator=(const VideoParamsAVC &other) {
        if (this == &other) return *this;

        VideoParamConfigSet::operator=(other);
        this->basicUnitSize = other.basicUnitSize;
        this->VUIFlag = other.VUIFlag;
        this->maxSliceSize = other.maxSliceSize;
        this->idrInterval = other.idrInterval;
        this->ipPeriod = other.ipPeriod;
        this->refFrames = other.refFrames;
        this->sliceNum = other.sliceNum;
        this->delimiterType = other.delimiterType;
        this->crop.LeftOffset = other.crop.LeftOffset;
        this->crop.RightOffset = other.crop.RightOffset;
        this->crop.TopOffset = other.crop.TopOffset;
        this->crop.BottomOffset = other.crop.BottomOffset;
        this->SAR.SarWidth = other.SAR.SarWidth;
        this->SAR.SarHeight = other.SAR.SarHeight;

        this->refIdx10ActiveMinus1 = other.refIdx10ActiveMinus1;
        this->refIdx11ActiveMinus1 = other.refIdx11ActiveMinus1;
        this->bFrameMBsOnly = other.bFrameMBsOnly;
        this->bMBAFF = other.bMBAFF;
        this->bEntropyCodingCABAC = other.bEntropyCodingCABAC;
        this->bWeightedPPrediction = other.bWeightedPPrediction;
        this->weightedBipredicitonMode = other.weightedBipredicitonMode;
        this->bConstIpred = other.bConstIpred;
        this->bDirect8x8Inference = other.bDirect8x8Inference;
        this->bDirectSpatialTemporal = other.bDirectSpatialTemporal;
        this->cabacInitIdc = other.cabacInitIdc;
        return *this;
    }
};

struct VideoParamsUpstreamBuffer : VideoParamConfigSet {

    VideoParamsUpstreamBuffer() {
        type = VideoParamsTypeUpSteamBuffer;
        size = sizeof(VideoParamsUpstreamBuffer);
    }

    VideoBufferSharingMode bufferMode;
    intptr_t *bufList;
    uint32_t bufCnt;
    ExternalBufferAttrib *bufAttrib;
    void *display;
};

struct VideoParamsUsrptrBuffer : VideoParamConfigSet {

    VideoParamsUsrptrBuffer() {
        type = VideoParamsTypeUsrptrBuffer;
        size = sizeof(VideoParamsUsrptrBuffer);
    }

    //input
    uint32_t width;
    uint32_t height;
    uint32_t format;
    uint32_t expectedSize;

    //output
    uint32_t actualSize;
    uint32_t stride;
    uint8_t *usrPtr;
};

struct VideoParamsHRD : VideoParamConfigSet {

    VideoParamsHRD() {
        type = VideoParamsTypeHRD;
        size = sizeof(VideoParamsHRD);
    }

    uint32_t bufferSize;
    uint32_t initBufferFullness;
};

struct VideoParamsStoreMetaDataInBuffers : VideoParamConfigSet {

    VideoParamsStoreMetaDataInBuffers() {
        type = VideoParamsTypeStoreMetaDataInBuffers;
        size = sizeof(VideoParamsStoreMetaDataInBuffers);
    }

    bool isEnabled;
};

struct VideoParamsProfileLevel : VideoParamConfigSet {

    VideoParamsProfileLevel() {
        type = VideoParamsTypeProfileLevel;
        size = sizeof(VideoParamsProfileLevel);
    }

    VAProfile profile;
    uint32_t level;
    bool isSupported;
};

struct VideoParamsTemporalLayer : VideoParamConfigSet {

    VideoParamsTemporalLayer() {
        type = VideoParamsTypeTemporalLayer;
        size = sizeof(VideoParamsTemporalLayer);
    }

    uint32_t numberOfLayer;
    uint32_t nPeriodicity;
    uint32_t nLayerID[32];
};


struct VideoConfigFrameRate : VideoParamConfigSet {

    VideoConfigFrameRate() {
        type = VideoConfigTypeFrameRate;
        size = sizeof(VideoConfigFrameRate);
    }

    VideoFrameRate frameRate;
};

struct VideoConfigBitRate : VideoParamConfigSet {

    VideoConfigBitRate() {
        type = VideoConfigTypeBitRate;
        size = sizeof(VideoConfigBitRate);
    }

    VideoRateControlParams rcParams;
};

struct VideoConfigAVCIntraPeriod : VideoParamConfigSet {

    VideoConfigAVCIntraPeriod() {
        type = VideoConfigTypeAVCIntraPeriod;
        size = sizeof(VideoConfigAVCIntraPeriod);
    }

    uint32_t idrInterval;  //How many Intra frame will have a IDR frame
    uint32_t intraPeriod;
    uint32_t ipPeriod;
};

struct VideoConfigNALSize : VideoParamConfigSet {

    VideoConfigNALSize() {
        type = VideoConfigTypeNALSize;
        size = sizeof(VideoConfigNALSize);
    }

    uint32_t maxSliceSize;
};

struct VideoConfigResolution : VideoParamConfigSet {

    VideoConfigResolution() {
        type = VideoConfigTypeResolution;
        size = sizeof(VideoConfigResolution);
    }

    VideoResolution resolution;
};

struct VideoConfigIntraRefreshType : VideoParamConfigSet {

    VideoConfigIntraRefreshType() {
        type = VideoConfigTypeIntraRefreshType;
        size = sizeof(VideoConfigIntraRefreshType);
    }

    VideoIntraRefreshType refreshType;
};

struct VideoConfigCyclicFrameInterval : VideoParamConfigSet {

    VideoConfigCyclicFrameInterval() {
        type = VideoConfigTypeCyclicFrameInterval;
        size = sizeof(VideoConfigCyclicFrameInterval);
    }

    int32_t cyclicFrameInterval;
};

struct VideoConfigCIR : VideoParamConfigSet {

    VideoConfigCIR() {
        type = VideoConfigTypeCIR;
        size = sizeof(VideoConfigCIR);
    }

    CirParams cirParams;
};

struct VideoConfigAIR : VideoParamConfigSet {

    VideoConfigAIR() {
        type = VideoConfigTypeAIR;
        size = sizeof(VideoConfigAIR);
    }

    AirParams airParams;
};

struct VideoConfigSliceNum : VideoParamConfigSet {

    VideoConfigSliceNum() {
        type = VideoConfigTypeSliceNum;
        size = sizeof(VideoConfigSliceNum);
    }

    SliceNum sliceNum;
};

struct VideoParamsVP8 : VideoParamConfigSet {

        uint32_t profile;
        uint32_t error_resilient;
        uint32_t num_token_partitions;
        uint32_t kf_auto;
        uint32_t kf_min_dist;
        uint32_t kf_max_dist;
        uint32_t min_qp;
        uint32_t max_qp;
        uint32_t init_qp;
        uint32_t rc_undershoot;
        uint32_t rc_overshoot;
        uint32_t hrd_buf_size;
        uint32_t hrd_buf_initial_fullness;
        uint32_t hrd_buf_optimal_fullness;
        uint32_t max_frame_size_ratio;

        VideoParamsVP8() {
                type = VideoParamsTypeVP8;
                size = sizeof(VideoParamsVP8);
        }
};

struct VideoConfigVP8 : VideoParamConfigSet {

        uint32_t force_kf;
        uint32_t refresh_entropy_probs;
        uint32_t value;
        unsigned char sharpness_level;

        VideoConfigVP8 () {
                type = VideoConfigTypeVP8;
                size = sizeof(VideoConfigVP8);
        }
};

struct VideoConfigVP8ReferenceFrame : VideoParamConfigSet {

        uint32_t no_ref_last;
        uint32_t no_ref_gf;
        uint32_t no_ref_arf;
        uint32_t refresh_last;
        uint32_t refresh_golden_frame;
        uint32_t refresh_alternate_frame;

        VideoConfigVP8ReferenceFrame () {
                type = VideoConfigTypeVP8ReferenceFrame;
                size = sizeof(VideoConfigVP8ReferenceFrame);
        }
};

struct VideoConfigVP8MaxFrameSizeRatio : VideoParamConfigSet {

    VideoConfigVP8MaxFrameSizeRatio() {
        type = VideoConfigTypeVP8MaxFrameSizeRatio;
        size = sizeof(VideoConfigVP8MaxFrameSizeRatio);
    }

    uint32_t max_frame_size_ratio;
};

struct VideoConfigTemperalLayerBitrateFramerate : VideoParamConfigSet {

       VideoConfigTemperalLayerBitrateFramerate() {
                type = VideoConfigTypeTemperalLayerBitrateFramerate;
                size = sizeof(VideoConfigTemperalLayerBitrateFramerate);
        }

        uint32_t nLayerID;
        uint32_t bitRate;
        uint32_t frameRate;
};

#endif /*  __VIDEO_ENCODER_DEF_H__ */
