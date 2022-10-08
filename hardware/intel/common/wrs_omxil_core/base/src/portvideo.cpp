/*
 * portvideo.cpp, port class for video
 *
 * Copyright (c) 2009-2010 Wind River Systems, Inc.
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

#include <stdlib.h>
#include <string.h>

#include <OMX_Core.h>
#include <OMX_Component.h>

#include <componentbase.h>
#include <portvideo.h>

#define LOG_TAG "portvideo"
#include <log.h>

PortVideo::PortVideo()
{
    memset(&videoparam, 0, sizeof(videoparam));
    ComponentBase::SetTypeHeader(&videoparam, sizeof(videoparam));

    videoparam.nIndex = 0;
    videoparam.eCompressionFormat = OMX_VIDEO_CodingUnused;
    videoparam.eColorFormat = OMX_COLOR_FormatYUV420Planar;
    videoparam.xFramerate = 15 << 16;

    memset(&bitrateparam, 0, sizeof(bitrateparam));
    ComponentBase::SetTypeHeader(&bitrateparam, sizeof(bitrateparam));

    bitrateparam.eControlRate = OMX_Video_ControlRateConstant;
    bitrateparam.nTargetBitrate = 64000;

    memset(&privateinfoparam, 0, sizeof(privateinfoparam));
    ComponentBase::SetTypeHeader(&privateinfoparam, sizeof(privateinfoparam));

    privateinfoparam.nCapacity = 0;
    privateinfoparam.nHolder = NULL;

    mbufsharing = OMX_FALSE;
}

//PortVideo::~PortVideo()
//{
//    if(privateinfoparam.nHolder != NULL) {
//        free(privateinfoparam.nHolder);
//        privateinfoparam.nHolder = NULL;
//    }
//}

OMX_ERRORTYPE PortVideo::SetPortVideoParam(
    const OMX_VIDEO_PARAM_PORTFORMATTYPE *p, bool internal)
{
    if (!internal) {
        OMX_ERRORTYPE ret;

        ret = ComponentBase::CheckTypeHeader((void *)p, sizeof(*p));
        if (ret != OMX_ErrorNone)
            return ret;
        if (videoparam.nPortIndex != p->nPortIndex)
            return OMX_ErrorBadPortIndex;
    }
    else
        videoparam.nPortIndex = p->nPortIndex;

    videoparam.nIndex = p->nIndex;
    videoparam.eCompressionFormat = p->eCompressionFormat;
    videoparam.eColorFormat = p->eColorFormat;
    videoparam.xFramerate = p->xFramerate;

    return OMX_ErrorNone;
}

const OMX_VIDEO_PARAM_PORTFORMATTYPE *PortVideo::GetPortVideoParam(void)
{
    return &videoparam;
}

OMX_ERRORTYPE PortVideo::SetPortBitrateParam(
    const OMX_VIDEO_PARAM_BITRATETYPE *p, bool internal)
{
    if (!internal) {
        OMX_ERRORTYPE ret;

        ret = ComponentBase::CheckTypeHeader((void *)p, sizeof(*p));
        if (ret != OMX_ErrorNone)
            return ret;
        if (bitrateparam.nPortIndex != p->nPortIndex)
            return OMX_ErrorBadPortIndex;
    }
    else
        bitrateparam.nPortIndex = p->nPortIndex;

    bitrateparam.eControlRate = p->eControlRate;
    bitrateparam.nTargetBitrate = p->nTargetBitrate;

    return OMX_ErrorNone;
}

const OMX_VIDEO_PARAM_BITRATETYPE *PortVideo::GetPortBitrateParam(void)
{
    return &bitrateparam;
}

OMX_ERRORTYPE PortVideo::SetPortBufferSharingInfo(OMX_BOOL isbufsharing)
{
    mbufsharing = isbufsharing;

    return OMX_ErrorNone;
}

const OMX_BOOL *PortVideo::GetPortBufferSharingInfo(void)
{
    return &mbufsharing;
}


OMX_ERRORTYPE PortVideo::SetPortPrivateInfoParam(
    const OMX_VIDEO_CONFIG_PRI_INFOTYPE *p, bool internal)
{
    if (!internal) {
        OMX_ERRORTYPE ret;

        ret = ComponentBase::CheckTypeHeader((void *)p, sizeof(*p));
        if (ret != OMX_ErrorNone)
            return ret;
        if (privateinfoparam.nPortIndex != p->nPortIndex)
            return OMX_ErrorBadPortIndex;
    }
    else
        privateinfoparam.nPortIndex = p->nPortIndex;

    const OMX_BOOL *isbufsharing = GetPortBufferSharingInfo();
    if(*isbufsharing) {
        //if(privateinfoparam.nHolder != NULL) {
        //    free(privateinfoparam.nHolder);
        //    privateinfoparam.nHolder = NULL;
        //}
        if(p->nHolder != NULL) {
            privateinfoparam.nCapacity = p->nCapacity;
            privateinfoparam.nHolder = (OMX_PTR)malloc(sizeof(OMX_U32) * (p->nCapacity));
            memcpy(privateinfoparam.nHolder, p->nHolder, sizeof(OMX_U32) * (p->nCapacity));
        } else {
            privateinfoparam.nCapacity = 0;
            privateinfoparam.nHolder = NULL;
        }
    }
    
    return OMX_ErrorNone;
}

const OMX_VIDEO_CONFIG_PRI_INFOTYPE *PortVideo::GetPortPrivateInfoParam(void)
{
    return &privateinfoparam;
}

/* end of PortVideo */
#if 0
PortAvc::PortAvc()
{
    OMX_VIDEO_PARAM_PORTFORMATTYPE videoparam;
    memcpy(&videoparam, GetPortVideoParam(), sizeof(videoparam));
    videoparam.eCompressionFormat = OMX_VIDEO_CodingAVC;
    videoparam.eColorFormat = OMX_COLOR_FormatUnused;
    videoparam.xFramerate = 15 << 16;
    SetPortVideoParam(&videoparam, false);

    memset(&avcparam, 0, sizeof(avcparam));
    ComponentBase::SetTypeHeader(&avcparam, sizeof(avcparam));

    avcparam.eProfile = OMX_VIDEO_AVCProfileVendorStartUnused;  //defaul profle for buffer sharing in opencore
    avcparam.eLevel = OMX_VIDEO_AVCLevelVendorStartUnused;     //default level for buffer sharing in opencore

//set buffer sharing mode
#ifdef COMPONENT_SUPPORT_BUFFER_SHARING
    SetPortBufferSharingInfo(OMX_TRUE);
#endif

}
OMX_ERRORTYPE PortAvc::SetPortAvcParam(
    const OMX_VIDEO_PARAM_AVCTYPE *p, bool overwrite_readonly)
{
    if (!overwrite_readonly) {
        OMX_ERRORTYPE ret;

        ret = ComponentBase::CheckTypeHeader((void *)p, sizeof(*p));
        if (ret != OMX_ErrorNone)
            return ret;
        if (avcparam.nPortIndex != p->nPortIndex)
            return OMX_ErrorBadPortIndex;
    }
    else {
        OMX_VIDEO_PARAM_PORTFORMATTYPE videoparam;

        memcpy(&videoparam, GetPortVideoParam(), sizeof(videoparam));
        videoparam.nPortIndex = p->nPortIndex;
        SetPortVideoParam(&videoparam, true);

        avcparam.nPortIndex = p->nPortIndex;
    }
    avcparam.nSliceHeaderSpacing = p->nSliceHeaderSpacing;
    avcparam.nPFrames = p->nPFrames;
    avcparam.nBFrames = p->nBFrames;
    avcparam.bUseHadamard = p->bUseHadamard;
    avcparam.nRefFrames = p->nRefFrames;
    avcparam.nRefIdx10ActiveMinus1 = p->nRefIdx10ActiveMinus1;
    avcparam.nRefIdx11ActiveMinus1 = p->nRefIdx11ActiveMinus1;
    avcparam.bEnableUEP = p->bEnableUEP;
    avcparam.bEnableFMO = p->bEnableFMO;
    avcparam.bEnableASO = p->bEnableASO;
    avcparam.bEnableRS = p->bEnableRS;
    avcparam.nAllowedPictureTypes = p->nAllowedPictureTypes;
    avcparam.bFrameMBsOnly = p->bFrameMBsOnly;
    avcparam.bMBAFF = p->bMBAFF;
    avcparam.bEntropyCodingCABAC = p->bEntropyCodingCABAC;
    avcparam.bWeightedPPrediction = p->bWeightedPPrediction;
    avcparam.nWeightedBipredicitonMode = p->nWeightedBipredicitonMode;
    avcparam.bconstIpred = p->bconstIpred;
    avcparam.bDirect8x8Inference = p->bDirect8x8Inference;
    avcparam.bDirectSpatialTemporal = p->bDirectSpatialTemporal;
    avcparam.nCabacInitIdc = p->nCabacInitIdc;
    avcparam.eLoopFilterMode = p->eLoopFilterMode;

#ifdef COMPONENT_SUPPORT_OPENCORE
#ifdef COMPONENT_SUPPORT_BUFFER_SHARING
// sepcial case ,not change default profile and level for opencore buffer sharing.
#else
    avcparam.eProfile = p->eProfile;
    avcparam.eLevel = p->eLevel;
#endif
#else
    avcparam.eProfile = p->eProfile;
    avcparam.eLevel = p->eLevel;
#endif

    return OMX_ErrorNone;
}

const OMX_VIDEO_PARAM_AVCTYPE *PortAvc::GetPortAvcParam(void)
{
    return &avcparam;
}

/* end of PortAvc */
#endif

