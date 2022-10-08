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


#define LOG_TAG "OMXVideoEncoderBase"
#include "OMXVideoEncoderBase.h"
#include "IntelMetadataBuffer.h"
#include <cutils/properties.h>
#include <wrs_omxil_core/log.h>

static const char *RAW_MIME_TYPE = "video/raw";

OMXVideoEncoderBase::OMXVideoEncoderBase()
    :mVideoEncoder(NULL)
    ,mEncoderParams(NULL)
    ,mFrameInputCount(0)
    ,mFrameOutputCount(0)
    ,mFirstFrame(OMX_TRUE)
    ,mFrameRetrieved(OMX_TRUE)
    ,mStoreMetaDataInBuffers(OMX_FALSE)
    ,mSyncEncoding(OMX_TRUE)
    ,mOmxLogLevel(0)
    ,mBlackFramePointer(NULL) {
    mEncoderParams = new VideoParamsCommon();
    if (!mEncoderParams) LOGE("OMX_ErrorInsufficientResources");

    char logLevelProp[128];
    if (property_get("omxenc.debug", logLevelProp, NULL)) {
        mOmxLogLevel = atoi(logLevelProp);
        LOGD("Debug level is %d", mOmxLogLevel);
    } 

    LOGV("OMXVideoEncoderBase::OMXVideoEncoderBase end");
}

OMXVideoEncoderBase::~OMXVideoEncoderBase() {

    // destroy ports
    if (this->ports) {
        if (this->ports[INPORT_INDEX]) {
            delete this->ports[INPORT_INDEX];
            this->ports[INPORT_INDEX] = NULL;
        }

        if (this->ports[OUTPORT_INDEX]) {
            delete this->ports[OUTPORT_INDEX];
            this->ports[OUTPORT_INDEX] = NULL;
        }
    }

    if (mBlackFramePointer) {
        free(mBlackFramePointer);
        mBlackFramePointer = NULL;
    }
    // Release video encoder object
    if(mVideoEncoder) {
        releaseVideoEncoder(mVideoEncoder);
        mVideoEncoder = NULL;
    }

    if(mEncoderParams) {
        delete mEncoderParams;
        mEncoderParams = NULL;
    }

}

OMX_ERRORTYPE OMXVideoEncoderBase::InitInputPort(void) {
    this->ports[INPORT_INDEX] = new PortVideo;
    if (this->ports[INPORT_INDEX] == NULL) {
        return OMX_ErrorInsufficientResources;
    }

    PortVideo *port = static_cast<PortVideo *>(this->ports[INPORT_INDEX]);

    // OMX_PARAM_PORTDEFINITIONTYPE
    OMX_PARAM_PORTDEFINITIONTYPE paramPortDefinitionInput;
    memset(&paramPortDefinitionInput, 0, sizeof(paramPortDefinitionInput));
    SetTypeHeader(&paramPortDefinitionInput, sizeof(paramPortDefinitionInput));
    paramPortDefinitionInput.nPortIndex = INPORT_INDEX;
    paramPortDefinitionInput.eDir = OMX_DirInput;
    paramPortDefinitionInput.nBufferCountActual = INPORT_ACTUAL_BUFFER_COUNT;
    paramPortDefinitionInput.nBufferCountMin = INPORT_MIN_BUFFER_COUNT;
    paramPortDefinitionInput.nBufferSize = INPORT_BUFFER_SIZE;
    paramPortDefinitionInput.bEnabled = OMX_TRUE;
    paramPortDefinitionInput.bPopulated = OMX_FALSE;
    paramPortDefinitionInput.eDomain = OMX_PortDomainVideo;
    paramPortDefinitionInput.format.video.cMIMEType = (OMX_STRING)RAW_MIME_TYPE;
    paramPortDefinitionInput.format.video.pNativeRender = NULL;
    paramPortDefinitionInput.format.video.nFrameWidth = 176;
    paramPortDefinitionInput.format.video.nFrameHeight = 144;
    paramPortDefinitionInput.format.video.nStride = 0;
    paramPortDefinitionInput.format.video.nSliceHeight = 0;
    paramPortDefinitionInput.format.video.nBitrate = 64000;
    paramPortDefinitionInput.format.video.xFramerate = 15 << 16;
    paramPortDefinitionInput.format.video.bFlagErrorConcealment = OMX_FALSE;
    paramPortDefinitionInput.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
    paramPortDefinitionInput.format.video.eColorFormat = OMX_COLOR_FormatYUV420SemiPlanar;
    paramPortDefinitionInput.format.video.pNativeWindow = NULL;
    paramPortDefinitionInput.bBuffersContiguous = OMX_FALSE;
    paramPortDefinitionInput.nBufferAlignment = 0;

    // Nothing specific to initialize input port.
    InitInputPortFormatSpecific(&paramPortDefinitionInput);

    port->SetPortDefinition(&paramPortDefinitionInput, true);

    // Set port buffer 4k aligned
    port->SetMemAlignment(4096);

    // OMX_VIDEO_PARAM_PORTFORMATTYPE
    OMX_VIDEO_PARAM_PORTFORMATTYPE paramPortFormat;
    memset(&paramPortFormat, 0, sizeof(paramPortFormat));
    SetTypeHeader(&paramPortFormat, sizeof(paramPortFormat));
    paramPortFormat.nPortIndex = INPORT_INDEX;
    paramPortFormat.nIndex = 0;
    paramPortFormat.eCompressionFormat = paramPortDefinitionInput.format.video.eCompressionFormat;
    paramPortFormat.eColorFormat = paramPortDefinitionInput.format.video.eColorFormat;
    paramPortFormat.xFramerate = paramPortDefinitionInput.format.video.xFramerate;

    port->SetPortVideoParam(&paramPortFormat, true);

    return OMX_ErrorNone;
}


OMX_ERRORTYPE OMXVideoEncoderBase::InitOutputPort(void) {
    this->ports[OUTPORT_INDEX] = new PortVideo;
    if (this->ports[OUTPORT_INDEX] == NULL) {
        return OMX_ErrorInsufficientResources;
    }

    PortVideo *port = static_cast<PortVideo *>(this->ports[OUTPORT_INDEX]);

    // OMX_VIDEO_PARAM_BITRATETYPE
    memset(&mParamBitrate, 0, sizeof(mParamBitrate));
    SetTypeHeader(&mParamBitrate, sizeof(mParamBitrate));
    mParamBitrate.nPortIndex = OUTPORT_INDEX;
    mParamBitrate.eControlRate = OMX_Video_ControlRateVariable;
    mParamBitrate.nTargetBitrate = 192000; // to be overridden

    // OMX_VIDEO_CONFIG_PRI_INFOTYPE
    memset(&mConfigPriInfo, 0, sizeof(mConfigPriInfo));
    SetTypeHeader(&mConfigPriInfo, sizeof(mConfigPriInfo));
    mConfigPriInfo.nPortIndex = OUTPORT_INDEX;
    mConfigPriInfo.nCapacity = 0;
    mConfigPriInfo.nHolder = NULL;

    // OMX_VIDEO_CONFIG_INTEL_BITRATETYPE
    memset(&mConfigIntelBitrate, 0, sizeof(mConfigIntelBitrate));
    SetTypeHeader(&mConfigIntelBitrate, sizeof(mConfigIntelBitrate));
    mConfigIntelBitrate.nPortIndex = OUTPORT_INDEX;
    mConfigIntelBitrate.nMaxEncodeBitrate = 0; // Maximum bitrate
    mConfigIntelBitrate.nTargetPercentage = 95; // Target bitrate as percentage of maximum bitrate; e.g. 95 is 95%
    mConfigIntelBitrate.nWindowSize = 0; // Window size in milliseconds allowed for bitrate to reach target
    mConfigIntelBitrate.nInitialQP = 0;  // Initial QP for I frames
    mConfigIntelBitrate.nMinQP = 0;
    mConfigIntelBitrate.nMaxQP = 0;
    mConfigIntelBitrate.nFrameRate = 0;
    mConfigIntelBitrate.nTemporalID = 0;

    // OMX_VIDEO_CONFIG_BITRATETYPE
    memset(&mConfigBitrate, 0, sizeof(mConfigBitrate));
    SetTypeHeader(&mConfigBitrate, sizeof(mConfigBitrate));
    mConfigBitrate.nPortIndex = OUTPORT_INDEX;
    mConfigBitrate.nEncodeBitrate = 0; // Maximum bitrate

    // OMX_VIDEO_CONFIG_INTEL_AIR
    memset(&mConfigIntelAir, 0, sizeof(mConfigIntelAir));
    SetTypeHeader(&mConfigIntelAir, sizeof(mConfigIntelAir));
    mConfigIntelAir.nPortIndex = OUTPORT_INDEX;
    mConfigIntelAir.bAirEnable = OMX_FALSE;
    mConfigIntelAir.bAirAuto = OMX_FALSE;
    mConfigIntelAir.nAirMBs = 0;
    mConfigIntelAir.nAirThreshold = 0;

    // OMX_VIDEO_CONFIG_INTEL_AIR
    memset(&mParamVideoRefresh, 0, sizeof(mParamVideoRefresh));
    SetTypeHeader(&mParamVideoRefresh, sizeof(mParamVideoRefresh));
    mParamVideoRefresh.nPortIndex = OUTPORT_INDEX;
    mParamVideoRefresh.eRefreshMode = OMX_VIDEO_IntraRefreshAdaptive;
    mParamVideoRefresh.nAirMBs = 0;
    mParamVideoRefresh.nAirRef = 0;
    mParamVideoRefresh.nCirMBs = 0;
    
    // OMX_CONFIG_FRAMERATETYPE
    memset(&mConfigFramerate, 0, sizeof(mConfigFramerate));
    SetTypeHeader(&mConfigFramerate, sizeof(mConfigFramerate));
    mConfigFramerate.nPortIndex = OUTPORT_INDEX;
    mConfigFramerate.xEncodeFramerate =  0; // Q16 format

    // OMX_VIDEO_PARAM_INTEL_ADAPTIVE_SLICE_CONTROL
    memset(&mParamIntelAdaptiveSliceControl, 0, sizeof(mParamIntelAdaptiveSliceControl));
    SetTypeHeader(&mParamIntelAdaptiveSliceControl, sizeof(mParamIntelAdaptiveSliceControl));
    mParamIntelAdaptiveSliceControl.nPortIndex = OUTPORT_INDEX;
    mParamIntelAdaptiveSliceControl.bEnable = OMX_FALSE;
    mParamIntelAdaptiveSliceControl.nMinPSliceNumber = 5;
    mParamIntelAdaptiveSliceControl.nNumPFramesToSkip = 8;
    mParamIntelAdaptiveSliceControl.nSliceSizeThreshold = 1200;

    // OMX_VIDEO_PARAM_PROFILELEVELTYPE
    memset(&mParamProfileLevel, 0, sizeof(mParamProfileLevel));
    SetTypeHeader(&mParamProfileLevel, sizeof(mParamProfileLevel));
    mParamProfileLevel.nPortIndex = OUTPORT_INDEX;
    mParamProfileLevel.eProfile = 0; // undefined profile, to be overridden
    mParamProfileLevel.eLevel = 0; // undefined level, to be overridden

    // OMX_PARAM_PORTDEFINITIONTYPE
    OMX_PARAM_PORTDEFINITIONTYPE paramPortDefinitionOutput;
    memset(&paramPortDefinitionOutput, 0, sizeof(paramPortDefinitionOutput));
    SetTypeHeader(&paramPortDefinitionOutput, sizeof(paramPortDefinitionOutput));
    paramPortDefinitionOutput.nPortIndex = OUTPORT_INDEX;
    paramPortDefinitionOutput.eDir = OMX_DirOutput;
    paramPortDefinitionOutput.nBufferCountActual = OUTPORT_ACTUAL_BUFFER_COUNT; // to be overridden
    paramPortDefinitionOutput.nBufferCountMin = OUTPORT_MIN_BUFFER_COUNT;
    paramPortDefinitionOutput.nBufferSize = OUTPORT_BUFFER_SIZE; // to be overridden
    paramPortDefinitionOutput.bEnabled = OMX_TRUE;
    paramPortDefinitionOutput.bPopulated = OMX_FALSE;
    paramPortDefinitionOutput.eDomain = OMX_PortDomainVideo;
    paramPortDefinitionOutput.format.video.cMIMEType = NULL; // to be overridden
    paramPortDefinitionOutput.format.video.pNativeRender = NULL;
    paramPortDefinitionOutput.format.video.nFrameWidth = 176;
    paramPortDefinitionOutput.format.video.nFrameHeight = 144;
    paramPortDefinitionOutput.format.video.nStride = 176;
    paramPortDefinitionOutput.format.video.nSliceHeight = 144;
    paramPortDefinitionOutput.format.video.nBitrate = 64000;
    paramPortDefinitionOutput.format.video.xFramerate = 15 << 16;
    paramPortDefinitionOutput.format.video.bFlagErrorConcealment = OMX_FALSE;
    paramPortDefinitionOutput.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused; // to be overridden
    paramPortDefinitionOutput.format.video.eColorFormat = OMX_COLOR_FormatUnused;
    paramPortDefinitionOutput.format.video.pNativeWindow = NULL;
    paramPortDefinitionOutput.bBuffersContiguous = OMX_FALSE;
    paramPortDefinitionOutput.nBufferAlignment = 0;

    InitOutputPortFormatSpecific(&paramPortDefinitionOutput);

    port->SetPortDefinition(&paramPortDefinitionOutput, true);
    port->SetPortBitrateParam(&mParamBitrate, true);

    // OMX_VIDEO_PARAM_PORTFORMATTYPE
    OMX_VIDEO_PARAM_PORTFORMATTYPE paramPortFormat;
    memset(&paramPortFormat, 0, sizeof(paramPortFormat));
    SetTypeHeader(&paramPortFormat, sizeof(paramPortFormat));
    paramPortFormat.nPortIndex = OUTPORT_INDEX;
    paramPortFormat.nIndex = 0;
    paramPortFormat.eCompressionFormat = paramPortDefinitionOutput.format.video.eCompressionFormat;
    paramPortFormat.eColorFormat = paramPortDefinitionOutput.format.video.eColorFormat;
    paramPortFormat.xFramerate = paramPortDefinitionOutput.format.video.xFramerate;

    port->SetPortVideoParam(&paramPortFormat, true);
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoEncoderBase::InitInputPortFormatSpecific(OMX_PARAM_PORTDEFINITIONTYPE *) {
    // no format specific to initialize input
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoEncoderBase::SetVideoEncoderParam() {

    Encode_Status ret = ENCODE_SUCCESS;
    PortVideo *port_in = NULL;
    PortVideo *port_out = NULL;
    OMX_VIDEO_CONTROLRATETYPE controlrate;
    const OMX_PARAM_PORTDEFINITIONTYPE *paramPortDefinitionInput = NULL;
    LOGV("OMXVideoEncoderBase::SetVideoEncoderParam called\n");

    port_in = static_cast<PortVideo *>(ports[INPORT_INDEX]);
    port_out = static_cast<PortVideo *>(ports[OUTPORT_INDEX]);
    paramPortDefinitionInput = port_in->GetPortDefinition();
    mEncoderParams->resolution.height = paramPortDefinitionInput->format.video.nFrameHeight;
    mEncoderParams->resolution.width = paramPortDefinitionInput->format.video.nFrameWidth;
    const OMX_VIDEO_PARAM_BITRATETYPE *bitrate = port_out->GetPortBitrateParam();

    mEncoderParams->frameRate.frameRateDenom = 1;
    if(mConfigFramerate.xEncodeFramerate != 0) {
        mEncoderParams->frameRate.frameRateNum = mConfigFramerate.xEncodeFramerate;
    } else {
        mEncoderParams->frameRate.frameRateNum = paramPortDefinitionInput->format.video.xFramerate >> 16;
        mConfigFramerate.xEncodeFramerate = paramPortDefinitionInput->format.video.xFramerate >> 16;
    }

    if(mEncoderParams->intraPeriod == 0) {
        OMX_U32 intraPeriod = mEncoderParams->frameRate.frameRateNum / 2;
        mEncoderParams->intraPeriod = (intraPeriod < 15) ? 15 : intraPeriod;   // Limit intra frame period to ensure video quality for low bitrate application.
    }

    if (paramPortDefinitionInput->format.video.eColorFormat == OMX_COLOR_FormatAndroidOpaque)
        mEncoderParams->rawFormat = RAW_FORMAT_OPAQUE;
    else
        mEncoderParams->rawFormat = RAW_FORMAT_NV12;

    LOGV("frameRate.frameRateDenom = %d\n", mEncoderParams->frameRate.frameRateDenom);
    LOGV("frameRate.frameRateNum = %d\n", mEncoderParams->frameRate.frameRateNum);
    LOGV("intraPeriod = %d\n ", mEncoderParams->intraPeriod);
    mEncoderParams->rcParams.initQP = mConfigIntelBitrate.nInitialQP;
    mEncoderParams->rcParams.minQP = mConfigIntelBitrate.nMinQP;
    mEncoderParams->rcParams.maxQP = 0;
    mEncoderParams->rcParams.I_minQP = 0;
    mEncoderParams->rcParams.I_maxQP = 0;
    mEncoderParams->rcParams.windowSize = mConfigIntelBitrate.nWindowSize;
    mEncoderParams->rcParams.targetPercentage = mConfigIntelBitrate.nTargetPercentage;
    mEncoderParams->rcParams.enableIntraFrameQPControl = 0;

    mEncoderParams->rcParams.bitRate = mParamBitrate.nTargetBitrate;
    if ((mParamBitrate.eControlRate == OMX_Video_ControlRateConstant )|| 
            (mParamBitrate.eControlRate == OMX_Video_ControlRateConstantSkipFrames)) {
        LOGV("%s(), eControlRate == OMX_Video_Intel_ControlRateConstant", __func__);
        mEncoderParams->rcMode = RATE_CONTROL_CBR;
    } else if ((mParamBitrate.eControlRate == OMX_Video_ControlRateVariable) ||
            (mParamBitrate.eControlRate == OMX_Video_ControlRateVariableSkipFrames)) {
        LOGV("%s(), eControlRate == OMX_Video_Intel_ControlRateVariable", __func__);
        mEncoderParams->rcMode = RATE_CONTROL_VBR;
    } else if (mParamBitrate.eControlRate == (OMX_VIDEO_CONTROLRATETYPE)OMX_Video_Intel_ControlRateVideoConferencingMode) {
        LOGV("%s(), eControlRate == OMX_Video_Intel_ControlRateVideoConferencingMode ", __func__);
        mEncoderParams->rcMode = RATE_CONTROL_VCM;
        if(mConfigIntelBitrate.nMaxEncodeBitrate >0)
            mEncoderParams->rcParams.bitRate = mConfigIntelBitrate.nMaxEncodeBitrate;
        if(mConfigIntelAir.bAirEnable == OMX_TRUE) {
            mEncoderParams->airParams.airAuto = mConfigIntelAir.bAirAuto;
            mEncoderParams->airParams.airMBs = mConfigIntelAir.nAirMBs;
            mEncoderParams->airParams.airThreshold = mConfigIntelAir.nAirThreshold;
            mEncoderParams->refreshType = VIDEO_ENC_AIR;
        } else {
            mEncoderParams->refreshType = VIDEO_ENC_NONIR;
        }
        LOGV("refreshType = %d\n", mEncoderParams->refreshType);
    } else {
        mEncoderParams->rcMode = RATE_CONTROL_NONE;
    }

    ret = mVideoEncoder->setParameters(mEncoderParams);
    CHECK_ENCODE_STATUS("setParameters");
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoEncoderBase::ProcessorInit(void) {
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    ret = SetVideoEncoderParam();
    CHECK_STATUS("SetVideoEncoderParam");

    Encode_Status status = mVideoEncoder->start();
    if (status != ENCODE_SUCCESS) {
        LOGE("Start failed, status = 0x%08x\n", status);
        return OMX_ErrorUndefined;
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoEncoderBase::ProcessorDeinit(void) {
    OMX_ERRORTYPE ret;

    if(mVideoEncoder) {
        mVideoEncoder->stop();
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoEncoderBase::ProcessorStop(void) {

    this->ports[INPORT_INDEX]->ReturnAllRetainedBuffers();
    return OMX_ErrorNone;
}
OMX_ERRORTYPE OMXVideoEncoderBase:: ProcessorProcess(
    OMX_BUFFERHEADERTYPE **,
    buffer_retain_t *,
    OMX_U32) {

    LOGV("OMXVideoEncoderBase:: ProcessorProcess \n");
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoEncoderBase::ProcessorFlush(OMX_U32 portIndex) {
    LOGV("OMXVideoEncoderBase::ProcessorFlush\n");
    if (portIndex == INPORT_INDEX || portIndex == OMX_ALL) {
        this->ports[INPORT_INDEX]->ReturnAllRetainedBuffers();
        mVideoEncoder->flush();
    }
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoEncoderBase::BuildHandlerList(void) {
    OMXComponentCodecBase::BuildHandlerList();
    AddHandler(OMX_IndexParamVideoPortFormat, GetParamVideoPortFormat, SetParamVideoPortFormat);
    AddHandler(OMX_IndexParamVideoBitrate, GetParamVideoBitrate, SetParamVideoBitrate);
    AddHandler((OMX_INDEXTYPE)OMX_IndexIntelPrivateInfo, GetIntelPrivateInfo, SetIntelPrivateInfo);
    AddHandler((OMX_INDEXTYPE)OMX_IndexConfigIntelBitrate, GetConfigIntelBitrate, SetConfigIntelBitrate);
    AddHandler((OMX_INDEXTYPE)OMX_IndexConfigIntelAIR, GetConfigIntelAIR, SetConfigIntelAIR);
    AddHandler((OMX_INDEXTYPE)OMX_IndexParamVideoIntraRefresh, GetParamVideoIntraRefresh, SetParamVideoIntraRefresh);
    AddHandler(OMX_IndexConfigVideoFramerate, GetConfigVideoFramerate, SetConfigVideoFramerate);
    AddHandler(OMX_IndexConfigVideoIntraVOPRefresh, GetConfigVideoIntraVOPRefresh, SetConfigVideoIntraVOPRefresh);
    //AddHandler(OMX_IndexParamIntelAdaptiveSliceControl, GetParamIntelAdaptiveSliceControl, SetParamIntelAdaptiveSliceControl);
    //AddHandler(OMX_IndexParamVideoProfileLevelQuerySupported, GetParamVideoProfileLevelQuerySupported, SetParamVideoProfileLevelQuerySupported);
    AddHandler((OMX_INDEXTYPE)OMX_IndexStoreMetaDataInBuffers, GetStoreMetaDataInBuffers, SetStoreMetaDataInBuffers);
    AddHandler((OMX_INDEXTYPE)OMX_IndexExtSyncEncoding, GetSyncEncoding, SetSyncEncoding);
    AddHandler((OMX_INDEXTYPE)OMX_IndexExtPrependSPSPPS, GetPrependSPSPPS, SetPrependSPSPPS);
    AddHandler((OMX_INDEXTYPE)OMX_IndexExtTemporalLayer, GetTemporalLayer,SetTemporalLayer);
    AddHandler((OMX_INDEXTYPE)OMX_IndexConfigVideoBitrate, GetConfigVideoBitrate, SetConfigVideoBitrate);
    AddHandler((OMX_INDEXTYPE)OMX_IndexExtRequestBlackFramePointer, GetBlackFramePointer, GetBlackFramePointer);
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoEncoderBase::GetParamVideoPortFormat(OMX_PTR pStructure) {
    OMX_ERRORTYPE ret;
    OMX_U32 index;
    OMX_VIDEO_PARAM_PORTFORMATTYPE *p = (OMX_VIDEO_PARAM_PORTFORMATTYPE *)pStructure;

    CHECK_TYPE_HEADER(p);
    CHECK_PORT_INDEX_RANGE(p);
    CHECK_ENUMERATION_RANGE(p->nIndex, 2);

    PortVideo *port = NULL;
    port = static_cast<PortVideo *>(this->ports[p->nPortIndex]);
    index = p->nIndex;
    memcpy(p, port->GetPortVideoParam(), sizeof(*p));
    // port supports OMX_COLOR_FormatYUV420SemiPlanar & OMX_COLOR_FormatAndroidOpaque
    if (index == 1) {
        p->nIndex = 1;
        p->eColorFormat = OMX_COLOR_FormatAndroidOpaque;
    }
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoEncoderBase::SetParamVideoPortFormat(OMX_PTR pStructure) {
    OMX_ERRORTYPE ret;
    OMX_VIDEO_PARAM_PORTFORMATTYPE *p = (OMX_VIDEO_PARAM_PORTFORMATTYPE *)pStructure;

    CHECK_TYPE_HEADER(p);
    CHECK_PORT_INDEX_RANGE(p);
    CHECK_SET_PARAM_STATE();

    // TODO: do we need to check if port is enabled?
    PortVideo *port = NULL;
    port = static_cast<PortVideo *>(this->ports[p->nPortIndex]);
#if 0
    if (p->eColorFormat ==  OMX_COLOR_FormatAndroidOpaque) {
        p->nIndex = 0;
        p->eColorFormat = OMX_COLOR_FormatYUV420SemiPlanar;
    }
#endif
    port->SetPortVideoParam(p, false);
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoEncoderBase::GetParamVideoBitrate(OMX_PTR pStructure) {
    OMX_ERRORTYPE ret;
    OMX_VIDEO_PARAM_BITRATETYPE *p = (OMX_VIDEO_PARAM_BITRATETYPE *)pStructure;

    CHECK_TYPE_HEADER(p);
    CHECK_PORT_INDEX(p, OUTPORT_INDEX);
    memcpy(p, &mParamBitrate, sizeof(*p));
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoEncoderBase::SetParamVideoBitrate(OMX_PTR pStructure) {
    OMX_ERRORTYPE ret;
    OMX_VIDEO_PARAM_BITRATETYPE *p = (OMX_VIDEO_PARAM_BITRATETYPE *)pStructure;
    CHECK_TYPE_HEADER(p);
    CHECK_PORT_INDEX(p, OUTPORT_INDEX);
    CHECK_SET_PARAM_STATE();
    OMX_U32 index = p->nPortIndex;
    PortVideo *port = NULL;
    // This disables other type of bitrate control mechanism
    // TODO: check if it is desired

    // TODO: can we override  mParamBitrate.nPortIndex (See SetPortBitrateParam)
    mParamBitrate.eControlRate = p->eControlRate;
    mParamBitrate.nTargetBitrate = p->nTargetBitrate;

    port = static_cast<PortVideo *>(ports[index]);
    ret = port->SetPortBitrateParam(p, false);
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoEncoderBase::GetIntelPrivateInfo(OMX_PTR pStructure) {
    OMX_ERRORTYPE ret;
    OMX_VIDEO_CONFIG_PRI_INFOTYPE *p = (OMX_VIDEO_CONFIG_PRI_INFOTYPE *)pStructure;

    CHECK_TYPE_HEADER(p);
    CHECK_PORT_INDEX(p, OUTPORT_INDEX);
    memcpy(p, &mConfigPriInfo, sizeof(*p));
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoEncoderBase::SetIntelPrivateInfo(OMX_PTR pStructure) {
    OMX_ERRORTYPE ret;
    OMX_VIDEO_CONFIG_PRI_INFOTYPE *p = (OMX_VIDEO_CONFIG_PRI_INFOTYPE *)pStructure;
    CHECK_TYPE_HEADER(p);
    CHECK_PORT_INDEX(p, OUTPORT_INDEX);

    // OMX_VIDEO_CONFIG_PRI_INFOTYPE is static parameter?
    CHECK_SET_PARAM_STATE();

    // TODO: can we override  mConfigPriInfo.nPortIndex (See SetPortPrivateInfoParam)

    if(p->nHolder != NULL) {
        // TODO: do we need to free nHolder?
        if (mConfigPriInfo.nHolder) {
            free(mConfigPriInfo.nHolder);
        }
        mConfigPriInfo.nCapacity = p->nCapacity;
        // TODO: nCapacity is in 8-bit unit or 32-bit unit?
        // TODO: check memory allocation
        mConfigPriInfo.nHolder = (OMX_PTR)malloc(sizeof(OMX_U32) * p->nCapacity);
        memcpy(mConfigPriInfo.nHolder, p->nHolder, sizeof(OMX_U32) * p->nCapacity);
    } else {
        mConfigPriInfo.nCapacity = 0;
        mConfigPriInfo.nHolder = NULL;
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoEncoderBase::GetConfigIntelBitrate(OMX_PTR pStructure) {
    OMX_ERRORTYPE ret;
    OMX_VIDEO_CONFIG_INTEL_BITRATETYPE *p = (OMX_VIDEO_CONFIG_INTEL_BITRATETYPE *)pStructure;

    CHECK_TYPE_HEADER(p);
    CHECK_PORT_INDEX(p, OUTPORT_INDEX);
    memcpy(p, &mConfigIntelBitrate, sizeof(*p));
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoEncoderBase::SetConfigIntelBitrate(OMX_PTR pStructure) {
    OMX_ERRORTYPE ret;
    Encode_Status retStatus = ENCODE_SUCCESS;
    if (mParamBitrate.eControlRate == OMX_Video_ControlRateMax){
        LOGE("SetConfigIntelBitrate failed. Feature is disabled.");
        return OMX_ErrorUnsupportedIndex;
    }
    OMX_VIDEO_CONFIG_INTEL_BITRATETYPE *p = (OMX_VIDEO_CONFIG_INTEL_BITRATETYPE *)pStructure;
    CHECK_TYPE_HEADER(p);
    CHECK_PORT_INDEX(p, OUTPORT_INDEX);

    // set in either Loaded state (ComponentSetParam) or Executing state (ComponentSetConfig)
    mConfigIntelBitrate = *p;

    // return OMX_ErrorNone if not in Executing state
    // TODO: return OMX_ErrorIncorrectStateOperation?
    CHECK_SET_CONFIG_STATE();

    VideoConfigBitRate configBitRate;
    configBitRate.rcParams.bitRate = mConfigIntelBitrate.nMaxEncodeBitrate;
    configBitRate.rcParams.initQP = mConfigIntelBitrate.nInitialQP;
    configBitRate.rcParams.minQP = mConfigIntelBitrate.nMinQP;
    configBitRate.rcParams.maxQP = mConfigIntelBitrate.nMaxQP;
    configBitRate.rcParams.I_minQP = 0;
    configBitRate.rcParams.I_maxQP = 0;
    configBitRate.rcParams.windowSize = mConfigIntelBitrate.nWindowSize;
    configBitRate.rcParams.targetPercentage = mConfigIntelBitrate.nTargetPercentage;
    configBitRate.rcParams.enableIntraFrameQPControl = 0;
    configBitRate.rcParams.temporalFrameRate = mConfigIntelBitrate.nFrameRate;
    configBitRate.rcParams.temporalID = mConfigIntelBitrate.nTemporalID;
    retStatus = mVideoEncoder->setConfig(&configBitRate);
    if(retStatus != ENCODE_SUCCESS) {
        LOGW("failed to set IntelBitrate");
    }
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoEncoderBase::GetConfigIntelAIR(OMX_PTR pStructure) {
    OMX_ERRORTYPE ret;
    OMX_VIDEO_CONFIG_INTEL_AIR *p = (OMX_VIDEO_CONFIG_INTEL_AIR *)pStructure;

    CHECK_TYPE_HEADER(p);
    CHECK_PORT_INDEX(p, OUTPORT_INDEX);
    memcpy(p, &mConfigIntelAir, sizeof(*p));
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoEncoderBase::SetConfigIntelAIR(OMX_PTR pStructure) {
    OMX_ERRORTYPE ret;
    Encode_Status retStatus = ENCODE_SUCCESS;

    OMX_VIDEO_CONFIG_INTEL_AIR *p = (OMX_VIDEO_CONFIG_INTEL_AIR *)pStructure;
    CHECK_TYPE_HEADER(p);
    CHECK_PORT_INDEX(p, OUTPORT_INDEX);

    // set in either Loaded  state (ComponentSetParam) or Executing state (ComponentSetConfig)
    mConfigIntelAir = *p;

    // return OMX_ErrorNone if not in Executing state
    // TODO: return OMX_ErrorIncorrectStateOperation?
    CHECK_SET_CONFIG_STATE();

    VideoConfigAIR configAIR;
    VideoConfigIntraRefreshType configIntraRefreshType;
    if(mConfigIntelAir.bAirEnable == OMX_TRUE) {
        configAIR.airParams.airAuto = mConfigIntelAir.bAirAuto;
        configAIR.airParams.airMBs = mConfigIntelAir.nAirMBs;
        configAIR.airParams.airThreshold = mConfigIntelAir.nAirThreshold;
        configIntraRefreshType.refreshType = VIDEO_ENC_AIR;
    } else {
        configIntraRefreshType.refreshType = VIDEO_ENC_NONIR;
    }

    retStatus = mVideoEncoder->setConfig(&configAIR);
    if(retStatus != ENCODE_SUCCESS) {
        LOGW("Failed to set AIR config");
    }

    retStatus = mVideoEncoder->setConfig(&configIntraRefreshType);
    if(retStatus != ENCODE_SUCCESS) {
        LOGW("Failed to set refresh config");
    }
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoEncoderBase::GetParamVideoIntraRefresh(OMX_PTR pStructure) {
    OMX_ERRORTYPE ret;
    OMX_VIDEO_PARAM_INTRAREFRESHTYPE *p = (OMX_VIDEO_PARAM_INTRAREFRESHTYPE *)pStructure;

    CHECK_TYPE_HEADER(p);
    CHECK_PORT_INDEX(p, OUTPORT_INDEX);
    memcpy(p, &mParamVideoRefresh, sizeof(*p));
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoEncoderBase::SetParamVideoIntraRefresh(OMX_PTR pStructure) {
    OMX_ERRORTYPE ret;
    Encode_Status retStatus = ENCODE_SUCCESS;

    OMX_VIDEO_PARAM_INTRAREFRESHTYPE *p = (OMX_VIDEO_PARAM_INTRAREFRESHTYPE *)pStructure;
    CHECK_TYPE_HEADER(p);
    CHECK_PORT_INDEX(p, OUTPORT_INDEX);

    // set in either Loaded  state (ComponentSetParam) or Executing state (ComponentSetConfig)
    mParamVideoRefresh = *p;

    // return OMX_ErrorNone if not in Executing state
    // TODO: return OMX_ErrorIncorrectStateOperation?
    CHECK_SET_PARAM_STATE();

    VideoConfigIntraRefreshType configIntraRefreshType;
    configIntraRefreshType.refreshType = (VideoIntraRefreshType)(mParamVideoRefresh.eRefreshMode + 1);
    if(configIntraRefreshType.refreshType == VIDEO_ENC_CIR){
     VideoConfigCIR configCIR;
        VideoConfigIntraRefreshType configIntraRefreshType;
        configCIR.cirParams.cir_num_mbs = mParamVideoRefresh.nCirMBs;
        configIntraRefreshType.refreshType = VIDEO_ENC_CIR;

        retStatus = mVideoEncoder->setConfig(&configCIR);
        if(retStatus != ENCODE_SUCCESS) {
            LOGW("Failed to set CIR config");
        }
    }else{
        VideoConfigAIR configAIR;

        configAIR.airParams.airMBs = mParamVideoRefresh.nAirMBs;
        configAIR.airParams.airThreshold = mParamVideoRefresh.nAirRef; 

        retStatus = mVideoEncoder->setConfig(&configAIR);
        if(retStatus != ENCODE_SUCCESS) {
            LOGW("Failed to set AIR config");
        }

    }

    retStatus = mVideoEncoder->setConfig(&configIntraRefreshType);
    if(retStatus != ENCODE_SUCCESS) {
        LOGW("Failed to set refresh config");
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoEncoderBase::GetConfigVideoFramerate(OMX_PTR pStructure) {
    OMX_ERRORTYPE ret;
    OMX_CONFIG_FRAMERATETYPE *p = (OMX_CONFIG_FRAMERATETYPE *)pStructure;

    CHECK_TYPE_HEADER(p);
    CHECK_PORT_INDEX(p, OUTPORT_INDEX);
    memcpy(p, &mConfigFramerate, sizeof(*p));
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoEncoderBase::SetConfigVideoFramerate(OMX_PTR pStructure) {
    OMX_ERRORTYPE ret;
    Encode_Status retStatus = ENCODE_SUCCESS;
    if (mParamBitrate.eControlRate == OMX_Video_ControlRateMax){
        LOGE("SetConfigVideoFramerate failed. Feature is disabled.");
        return OMX_ErrorUnsupportedIndex;
    }
    OMX_CONFIG_FRAMERATETYPE *p = (OMX_CONFIG_FRAMERATETYPE *)pStructure;
    CHECK_TYPE_HEADER(p);
    CHECK_PORT_INDEX(p, OUTPORT_INDEX);

    // set in either Loaded state  (ComponentSetParam) or Executing state (ComponentSetConfig)
    mConfigFramerate = *p;

    // return OMX_ErrorNone if not in Executing state
    // TODO, return OMX_ErrorIncorrectStateOperation?
    CHECK_SET_CONFIG_STATE();

    VideoConfigFrameRate framerate;
    framerate.frameRate.frameRateDenom = 1;
    framerate.frameRate.frameRateNum = mConfigFramerate.xEncodeFramerate >> 16;
    retStatus = mVideoEncoder->setConfig(&framerate);
    if(retStatus != ENCODE_SUCCESS) {
        LOGW("Failed to set frame rate config");
    }
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoEncoderBase::GetConfigVideoIntraVOPRefresh(OMX_PTR) {
    LOGW("GetConfigVideoIntraVOPRefresh is not supported.");
    return OMX_ErrorUnsupportedSetting;
}

OMX_ERRORTYPE OMXVideoEncoderBase::SetConfigVideoIntraVOPRefresh(OMX_PTR pStructure) {
    OMX_ERRORTYPE ret;
    Encode_Status retStatus = ENCODE_SUCCESS;
    OMX_CONFIG_INTRAREFRESHVOPTYPE *p = (OMX_CONFIG_INTRAREFRESHVOPTYPE *)pStructure;
    CHECK_TYPE_HEADER(p);
    CHECK_PORT_INDEX(p, OUTPORT_INDEX);

    // return OMX_ErrorNone if not in Executing state
    // TODO: return OMX_ErrorIncorrectStateOperation?
    CHECK_SET_CONFIG_STATE();

    if(p->IntraRefreshVOP == OMX_TRUE) {
        VideoParamConfigSet configIDRRequest;
        configIDRRequest.type = VideoConfigTypeIDRRequest;
        retStatus = mVideoEncoder->setConfig(&configIDRRequest);
        if(retStatus != ENCODE_SUCCESS) {
            LOGW("Failed to set refresh config");
        }
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoEncoderBase::GetParamIntelAdaptiveSliceControl(OMX_PTR pStructure) {

    OMX_ERRORTYPE ret;
    OMX_VIDEO_PARAM_INTEL_ADAPTIVE_SLICE_CONTROL *p = (OMX_VIDEO_PARAM_INTEL_ADAPTIVE_SLICE_CONTROL *)pStructure;

    CHECK_TYPE_HEADER(p);
    CHECK_PORT_INDEX(p, OUTPORT_INDEX);
    memcpy(p, &mParamIntelAdaptiveSliceControl, sizeof(*p));

    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoEncoderBase::SetParamIntelAdaptiveSliceControl(OMX_PTR pStructure) {

    OMX_ERRORTYPE ret;
    if (mParamBitrate.eControlRate == OMX_Video_ControlRateMax) {
        LOGE("SetParamIntelAdaptiveSliceControl failed. Feature is disabled.");
        return OMX_ErrorUnsupportedIndex;
    }
    OMX_VIDEO_PARAM_INTEL_ADAPTIVE_SLICE_CONTROL *p = (OMX_VIDEO_PARAM_INTEL_ADAPTIVE_SLICE_CONTROL *)pStructure;
    CHECK_TYPE_HEADER(p);
    CHECK_PORT_INDEX(p, OUTPORT_INDEX);

    // set only in Loaded state (ComponentSetParam)
    CHECK_SET_PARAM_STATE();

    mParamIntelAdaptiveSliceControl = *p;

    return OMX_ErrorNone;
}

/*
OMX_ERRORTYPE OMXVideoEncoderBase::GetParamVideoProfileLevelQuerySupported(OMX_PTR pStructure) {
    OMX_ERRORTYPE ret;
    OMX_VIDEO_PARAM_PROFILELEVELTYPE *p = (OMX_VIDEO_PARAM_PROFILELEVELTYPE *)pStructure;

    CHECK_TYPE_HEADER(p);
    CHECK_PORT_INDEX(p, OUTPORT_INDEX);

    // assign values instead of memory coping to avoid nProfileIndex being overridden
    p->eProfile = mParamProfileLevel.eProfile;
    p->eLevel = mParamProfileLevel.eLevel;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoEncoderBase::SetParamVideoProfileLevelQuerySupported(OMX_PTR pStructure) {
    LOGW("SetParamVideoProfileLevelQuerySupported is not supported.");
    return OMX_ErrorUnsupportedSetting;
}
*/

OMX_ERRORTYPE OMXVideoEncoderBase::GetStoreMetaDataInBuffers(OMX_PTR pStructure) {
    OMX_ERRORTYPE ret;
    StoreMetaDataInBuffersParams *p = (StoreMetaDataInBuffersParams *)pStructure;

    CHECK_TYPE_HEADER(p);
    CHECK_PORT_INDEX(p, INPORT_INDEX);

    p->bStoreMetaData = mStoreMetaDataInBuffers;

    return OMX_ErrorNone;
};
OMX_ERRORTYPE OMXVideoEncoderBase::SetStoreMetaDataInBuffers(OMX_PTR pStructure) {
    OMX_ERRORTYPE ret;
    StoreMetaDataInBuffersParams *p = (StoreMetaDataInBuffersParams *)pStructure;
    VideoParamsStoreMetaDataInBuffers StoreMetaDataInBuffers;
    PortVideo *port = static_cast<PortVideo *>(this->ports[INPORT_INDEX]);
    PortVideo *output_port = static_cast<PortVideo *>(this->ports[OUTPORT_INDEX]);
    uint32_t maxSize = 0;

    CHECK_TYPE_HEADER(p);
    CHECK_PORT_INDEX(p, INPORT_INDEX);

    LOGD("SetStoreMetaDataInBuffers (enabled = %x)", p->bStoreMetaData);
    if(mStoreMetaDataInBuffers == p->bStoreMetaData)
        return OMX_ErrorNone;

    StoreMetaDataInBuffers.isEnabled = p->bStoreMetaData;
    if (mVideoEncoder->setParameters(&StoreMetaDataInBuffers) != ENCODE_SUCCESS)
        return OMX_ErrorNotReady;

    mStoreMetaDataInBuffers = p->bStoreMetaData;

    if(mStoreMetaDataInBuffers){
        // for input port buffer
        OMX_PARAM_PORTDEFINITIONTYPE *paramPortDefinitionInput;
        const OMX_PARAM_PORTDEFINITIONTYPE *paramPortDefinitionInput_get;

        paramPortDefinitionInput_get = port->GetPortDefinition();
        paramPortDefinitionInput = (OMX_PARAM_PORTDEFINITIONTYPE *)paramPortDefinitionInput_get;
        paramPortDefinitionInput->nBufferSize = IntelMetadataBuffer::GetMaxBufferSize();
    }
    else
    {
        const OMX_PARAM_PORTDEFINITIONTYPE *paramPortDefinitionInput_get;

        paramPortDefinitionInput_get = port->GetPortDefinition();
        port->SetPortDefinition(paramPortDefinitionInput_get, true);
    }

    LOGD("SetStoreMetaDataInBuffers success");
    return OMX_ErrorNone;
};

OMX_ERRORTYPE OMXVideoEncoderBase::GetSyncEncoding(OMX_PTR pStructure) {
    OMX_BOOL* syncEncoding = static_cast<OMX_BOOL*>(pStructure);

    *syncEncoding = mSyncEncoding;

    return OMX_ErrorNone;
};

OMX_ERRORTYPE OMXVideoEncoderBase::SetSyncEncoding(OMX_PTR pStructure) {
    CHECK_SET_PARAM_STATE();

    mSyncEncoding = *(static_cast<OMX_BOOL*>(pStructure));

    LOGD("SetSyncEncoding %d", mSyncEncoding);

    return OMX_ErrorNone;
};

OMX_ERRORTYPE OMXVideoEncoderBase::GetPrependSPSPPS(OMX_PTR) {
    return OMX_ErrorNone;
};

OMX_ERRORTYPE OMXVideoEncoderBase::SetPrependSPSPPS(OMX_PTR) {
    LOGD("SetPrependSPSPPS success");
    return OMX_ErrorNone;
};

OMX_ERRORTYPE OMXVideoEncoderBase::GetTemporalLayer(OMX_PTR pStructure) {
    OMX_ERRORTYPE ret;
    OMX_VIDEO_PARAM_INTEL_TEMPORAL_LAYER* p = static_cast<OMX_VIDEO_PARAM_INTEL_TEMPORAL_LAYER*>(pStructure);

    CHECK_TYPE_HEADER(p);
    CHECK_PORT_INDEX(p, OUTPORT_INDEX);
    memcpy(p, &mTemporalLayer, sizeof(*p));
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoEncoderBase::SetTemporalLayer(OMX_PTR pStructure) {
    OMX_ERRORTYPE ret;
    OMX_VIDEO_PARAM_INTEL_TEMPORAL_LAYER *p = (OMX_VIDEO_PARAM_INTEL_TEMPORAL_LAYER *)pStructure;
    VideoParamsTemporalLayer TemporalLayer;
    OMX_U32 i;

    CHECK_TYPE_HEADER(p);
    CHECK_PORT_INDEX(p, OUTPORT_INDEX);

    LOGE("SetTemporalLayer (enabled = %d)", p->nNumberOfTemporalLayer);

    TemporalLayer.numberOfLayer = p->nNumberOfTemporalLayer;
    TemporalLayer.nPeriodicity = p->nPeriodicity;
    for(i=0;i<p->nPeriodicity;i++)
        TemporalLayer.nLayerID[i] = p->nLayerID[i];

    if (mVideoEncoder->setParameters(&TemporalLayer) != ENCODE_SUCCESS)
        return OMX_ErrorNotReady;

    LOGE("SetTemporalLayer success");
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoEncoderBase::GetBlackFramePointer(OMX_PTR pStructure) {
    OMX_ERRORTYPE ret;
    OMX_VIDEO_INTEL_REQUEST_BALCK_FRAME_POINTER *p = (OMX_VIDEO_INTEL_REQUEST_BALCK_FRAME_POINTER *)pStructure;

    CHECK_TYPE_HEADER(p);
    CHECK_PORT_INDEX(p, INPORT_INDEX);

    PortVideo *port_in = static_cast<PortVideo *>(ports[INPORT_INDEX]);
    const OMX_PARAM_PORTDEFINITIONTYPE *paramPortDefinitionInput = port_in->GetPortDefinition();
    OMX_U32 width = paramPortDefinitionInput->format.video.nFrameWidth;
    OMX_U32 height = paramPortDefinitionInput->format.video.nFrameHeight;
    OMX_U32 lumaSize = width * height;
    OMX_U32 bufferSize = width * height * 3 / 2;

    if(mBlackFramePointer) {
        free(mBlackFramePointer);
        mBlackFramePointer = NULL;
    } else {
        mBlackFramePointer = (OMX_PTR)memalign(4096, bufferSize); // align to page size
        if(!mBlackFramePointer) {
            return OMX_ErrorInsufficientResources;
        }
        memset(mBlackFramePointer, 0x0, lumaSize);
        memset((OMX_PTR)((uint64_t)mBlackFramePointer + lumaSize), 0x80, lumaSize / 2);
        p->nFramePointer = (OMX_U32)mBlackFramePointer;
    }
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoEncoderBase::SetBlackFramePointer(OMX_PTR) {
    return OMX_ErrorUnsupportedSetting;
}

OMX_ERRORTYPE OMXVideoEncoderBase::GetConfigVideoBitrate(OMX_PTR pStructure) {

    OMX_ERRORTYPE ret;
    OMX_VIDEO_CONFIG_BITRATETYPE *p = (OMX_VIDEO_CONFIG_BITRATETYPE *)pStructure;

    CHECK_TYPE_HEADER(p);
    CHECK_PORT_INDEX(p, OUTPORT_INDEX);
    memcpy(p, &mConfigBitrate, sizeof(*p));
    return OMX_ErrorNone;
}
OMX_ERRORTYPE OMXVideoEncoderBase::SetConfigVideoBitrate(OMX_PTR pStructure){
    OMX_ERRORTYPE ret;
    Encode_Status retStatus = ENCODE_SUCCESS;
    if (mParamBitrate.eControlRate == OMX_Video_ControlRateMax){
        LOGE("SetConfigIntelBitrate failed. Feature is disabled.");
        return OMX_ErrorUnsupportedIndex;
    }
    OMX_VIDEO_CONFIG_BITRATETYPE *p = (OMX_VIDEO_CONFIG_BITRATETYPE *)pStructure;
    CHECK_TYPE_HEADER(p);
    CHECK_PORT_INDEX(p, OUTPORT_INDEX);

    // set in either Loaded state (ComponentSetParam) or Executing state (ComponentSetConfig)
    mConfigBitrate = *p;

    // return OMX_ErrorNone if not in Executing state
    // TODO: return OMX_ErrorIncorrectStateOperation?
    // CHECK_SET_CONFIG_STATE();

    VideoConfigBitRate configBitRate;
    configBitRate.rcParams.bitRate = mConfigBitrate.nEncodeBitrate;
    configBitRate.rcParams.temporalID = 0;
    retStatus = mVideoEncoder->setConfig(&configBitRate);
    if(retStatus != ENCODE_SUCCESS) {
        LOGW("failed to set IntelBitrate");
    }
    return OMX_ErrorNone;
}
