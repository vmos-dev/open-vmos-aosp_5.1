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



#ifndef OMX_VIDEO_ENCODER_BASE_H_
#define OMX_VIDEO_ENCODER_BASE_H_

#include "OMXComponentCodecBase.h"
#include <va/va_tpi.h>
#include <va/va_android.h>
#include <VideoEncoderHost.h>

#define LOGV(...) ALOGI_IF(mOmxLogLevel, __VA_ARGS__)
#define LOGI(...) ALOGI_IF(mOmxLogLevel, __VA_ARGS__)
#define LOGW(...) ALOGI_IF(mOmxLogLevel, __VA_ARGS__)
#define LOGD(...) ALOGI_IF(mOmxLogLevel, __VA_ARGS__)
#define LOGE ALOGE
#define LOGV_IF ALOGV_IF

using android::sp;

class OMXVideoEncoderBase : public OMXComponentCodecBase {
public:
    OMXVideoEncoderBase();
    virtual ~OMXVideoEncoderBase();

protected:
    virtual OMX_ERRORTYPE InitInputPort(void);
    virtual OMX_ERRORTYPE InitOutputPort(void);
    virtual OMX_ERRORTYPE InitInputPortFormatSpecific(OMX_PARAM_PORTDEFINITIONTYPE *paramPortDefinitionInput);
    virtual OMX_ERRORTYPE InitOutputPortFormatSpecific(OMX_PARAM_PORTDEFINITIONTYPE *paramPortDefinitionOutput) = 0;

    virtual OMX_ERRORTYPE ProcessorInit(void);
    virtual OMX_ERRORTYPE ProcessorDeinit(void);
    //virtual OMX_ERRORTYPE ProcessorStart(void);
    virtual OMX_ERRORTYPE ProcessorStop(void);
    //virtual OMX_ERRORTYPE ProcessorPause(void);
    //virtual OMX_ERRORTYPE ProcessorResume(void);
    virtual OMX_ERRORTYPE ProcessorFlush(OMX_U32 portIndex);
    virtual OMX_ERRORTYPE ProcessorProcess(
            OMX_BUFFERHEADERTYPE **buffers,
            buffer_retain_t *retains,
            OMX_U32 numberBuffers);

    virtual OMX_ERRORTYPE BuildHandlerList(void);

    DECLARE_HANDLER(OMXVideoEncoderBase, ParamVideoPortFormat);
    DECLARE_HANDLER(OMXVideoEncoderBase, ParamVideoBitrate);
    DECLARE_HANDLER(OMXVideoEncoderBase, IntelPrivateInfo);
    DECLARE_HANDLER(OMXVideoEncoderBase, ConfigIntelBitrate);
    DECLARE_HANDLER(OMXVideoEncoderBase, ConfigIntelAIR);
    DECLARE_HANDLER(OMXVideoEncoderBase, ParamVideoIntraRefresh);
    DECLARE_HANDLER(OMXVideoEncoderBase, ConfigVideoFramerate);
    DECLARE_HANDLER(OMXVideoEncoderBase, ConfigVideoIntraVOPRefresh);
    DECLARE_HANDLER(OMXVideoEncoderBase, ParamIntelAdaptiveSliceControl);
    //DECLARE_HANDLER(OMXVideoEncoderBase, ParamVideoProfileLevelQuerySupported);
    DECLARE_HANDLER(OMXVideoEncoderBase, StoreMetaDataInBuffers);
    DECLARE_HANDLER(OMXVideoEncoderBase, SyncEncoding);
    DECLARE_HANDLER(OMXVideoEncoderBase, PrependSPSPPS);
    DECLARE_HANDLER(OMXVideoEncoderBase, TemporalLayer);
    DECLARE_HANDLER(OMXVideoEncoderBase, ConfigVideoBitrate);
    DECLARE_HANDLER(OMXVideoEncoderBase, BlackFramePointer);

protected:
    virtual OMX_ERRORTYPE SetVideoEncoderParam();
protected:
    OMX_VIDEO_PARAM_BITRATETYPE mParamBitrate;
    OMX_VIDEO_CONFIG_PRI_INFOTYPE mConfigPriInfo;
    OMX_VIDEO_CONFIG_INTEL_BITRATETYPE mConfigIntelBitrate;
    OMX_VIDEO_CONFIG_INTEL_AIR mConfigIntelAir;
    OMX_VIDEO_PARAM_INTRAREFRESHTYPE mParamVideoRefresh;
    OMX_CONFIG_FRAMERATETYPE  mConfigFramerate;
    OMX_VIDEO_PARAM_INTEL_ADAPTIVE_SLICE_CONTROL mParamIntelAdaptiveSliceControl;
    OMX_VIDEO_PARAM_PROFILELEVELTYPE mParamProfileLevel;
    OMX_VIDEO_PARAM_INTEL_TEMPORAL_LAYER mTemporalLayer;
    OMX_VIDEO_CONFIG_BITRATETYPE mConfigBitrate;

    IVideoEncoder *mVideoEncoder;
    VideoParamsCommon *mEncoderParams;
    OMX_U32 mFrameInputCount;
    OMX_U32 mFrameOutputCount;
    OMX_BOOL mFirstFrame;
    OMX_BOOL mFrameRetrieved;
    OMX_BOOL mStoreMetaDataInBuffers;
    OMX_BOOL mSyncEncoding;
    int32_t mOmxLogLevel;
    OMX_PTR mBlackFramePointer;

private:

    enum {
        // OMX_PARAM_PORTDEFINITIONTYPE
        INPORT_MIN_BUFFER_COUNT = 1,
        // FIXME: increate input buffer count to 5
#ifdef USE_VIDEO_EFFECT
        INPORT_ACTUAL_BUFFER_COUNT = 5,
#else
        INPORT_ACTUAL_BUFFER_COUNT = 2,
#endif
        INPORT_BUFFER_SIZE = 1382400,

        // OMX_PARAM_PORTDEFINITIONTYPE
        OUTPORT_MIN_BUFFER_COUNT = 1,
        OUTPORT_ACTUAL_BUFFER_COUNT = 2,
        OUTPORT_BUFFER_SIZE = 1382400,
    };

};

#endif /* OMX_VIDEO_ENCODER_BASE_H_ */
