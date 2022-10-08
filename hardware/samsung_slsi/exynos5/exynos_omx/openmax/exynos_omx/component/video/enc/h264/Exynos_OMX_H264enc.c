/*
 *
 * Copyright 2012 Samsung Electronics S.LSI Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * @file        Exynos_OMX_H264enc.c
 * @brief
 * @author      SeungBeom Kim (sbcrux.kim@samsung.com)
 * @version     2.0.0
 * @history
 *   2012.02.20 : Create
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Exynos_OMX_Macros.h"
#include "Exynos_OMX_Basecomponent.h"
#include "Exynos_OMX_Baseport.h"
#include "Exynos_OMX_Venc.h"
#include "Exynos_OSAL_ETC.h"
#include "Exynos_OSAL_Semaphore.h"
#include "Exynos_OSAL_Thread.h"
#include "Exynos_OSAL_Android.h"
#include "library_register.h"
#include "Exynos_OMX_H264enc.h"
#include "ExynosVideoApi.h"
#include "Exynos_OSAL_SharedMemory.h"
#include "Exynos_OSAL_Event.h"

/* To use CSC_METHOD_HW in EXYNOS OMX, gralloc should allocate physical memory using FIMC */
/* It means GRALLOC_USAGE_HW_FIMC1 should be set on Native Window usage */
#include "csc.h"

#undef  EXYNOS_LOG_TAG
#define EXYNOS_LOG_TAG    "EXYNOS_H264_ENC"
#define EXYNOS_LOG_OFF
//#define EXYNOS_TRACE_ON
#include "Exynos_OSAL_Log.h"

/* H.264 Encoder Supported Levels & profiles */
EXYNOS_OMX_VIDEO_PROFILELEVEL supportedAVCProfileLevels[] ={
    {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel1},
    {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel1b},
    {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel11},
    {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel12},
    {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel13},
    {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel2},
    {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel21},
    {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel22},
    {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel3},
    {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel31},
    {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel32},
    {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel4},
    {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel41},
    {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel42},

    {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel1},
    {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel1b},
    {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel11},
    {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel12},
    {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel13},
    {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel2},
    {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel21},
    {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel22},
    {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel3},
    {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel31},
    {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel32},
    {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel4},
    {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel41},
    {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel42},

    {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel1},
    {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel1b},
    {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel11},
    {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel12},
    {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel13},
    {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel2},
    {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel21},
    {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel22},
    {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel3},
    {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel31},
    {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel32},
    {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel4},
    {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel41},
    {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel42}};

static OMX_U32 OMXAVCProfileToProfileIDC(OMX_VIDEO_AVCPROFILETYPE profile)
{
    OMX_U32 ret = 0;

    if (profile == OMX_VIDEO_AVCProfileBaseline)
        ret = 0;
    else if (profile == OMX_VIDEO_AVCProfileMain)
        ret = 2;
    else if (profile == OMX_VIDEO_AVCProfileHigh)
        ret = 4;

    return ret;
}

static OMX_U32 OMXAVCLevelToLevelIDC(OMX_VIDEO_AVCLEVELTYPE level)
{
    OMX_U32 ret = 11; //default OMX_VIDEO_AVCLevel4

    if (level == OMX_VIDEO_AVCLevel1)
        ret = 0;
    else if (level == OMX_VIDEO_AVCLevel1b)
        ret = 1;
    else if (level == OMX_VIDEO_AVCLevel11)
        ret = 2;
    else if (level == OMX_VIDEO_AVCLevel12)
        ret = 3;
    else if (level == OMX_VIDEO_AVCLevel13)
        ret = 4;
    else if (level == OMX_VIDEO_AVCLevel2)
        ret = 5;
    else if (level == OMX_VIDEO_AVCLevel21)
        ret = 6;
    else if (level == OMX_VIDEO_AVCLevel22)
        ret = 7;
    else if (level == OMX_VIDEO_AVCLevel3)
        ret = 8;
    else if (level == OMX_VIDEO_AVCLevel31)
        ret = 9;
    else if (level == OMX_VIDEO_AVCLevel32)
        ret = 10;
    else if (level == OMX_VIDEO_AVCLevel4)
        ret = 11;
    else if (level == OMX_VIDEO_AVCLevel41)
        ret = 12;
    else if (level == OMX_VIDEO_AVCLevel42)
        ret = 13;

    return ret;
}

static OMX_U8 *FindDelimiter(OMX_U8 *pBuffer, OMX_U32 size)
{
    OMX_U32 i;

    for (i = 0; i < size - 3; i++) {
        if ((pBuffer[i] == 0x00)   &&
            (pBuffer[i + 1] == 0x00) &&
            (pBuffer[i + 2] == 0x00) &&
            (pBuffer[i + 3] == 0x01))
            return (pBuffer + i);
    }

    return NULL;
}

static void Print_H264Enc_Param(ExynosVideoEncParam *pEncParam)
{
    ExynosVideoEncCommonParam *pCommonParam = &pEncParam->commonParam;
    ExynosVideoEncH264Param   *pH264Param   = &pEncParam->codecParam.h264;

    /* common parameters */
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "SourceWidth             : %d", pCommonParam->SourceWidth);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "SourceHeight            : %d", pCommonParam->SourceHeight);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "IDRPeriod               : %d", pCommonParam->IDRPeriod);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "SliceMode               : %d", pCommonParam->SliceMode);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "RandomIntraMBRefresh    : %d", pCommonParam->RandomIntraMBRefresh);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "Bitrate                 : %d", pCommonParam->Bitrate);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "FrameQp                 : %d", pCommonParam->FrameQp);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "FrameQp_P               : %d", pCommonParam->FrameQp_P);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "QSCodeMax              : %d", pCommonParam->QSCodeMax);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "QSCodeMin               : %d", pCommonParam->QSCodeMin);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "PadControlOn            : %d", pCommonParam->PadControlOn);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "LumaPadVal              : %d", pCommonParam->LumaPadVal);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "CbPadVal                : %d", pCommonParam->CbPadVal);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "CrPadVal                : %d", pCommonParam->CrPadVal);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "FrameMap                : %d", pCommonParam->FrameMap);

    /* H.264 specific parameters */
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "ProfileIDC              : %d", pH264Param->ProfileIDC);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "LevelIDC                : %d", pH264Param->LevelIDC);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "FrameQp_B               : %d", pH264Param->FrameQp_B);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "FrameRate               : %d", pH264Param->FrameRate);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "SliceArgument           : %d", pH264Param->SliceArgument);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "NumberBFrames           : %d", pH264Param->NumberBFrames);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "NumberReferenceFrames   : %d", pH264Param->NumberReferenceFrames);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "NumberRefForPframes     : %d", pH264Param->NumberRefForPframes);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "LoopFilterDisable       : %d", pH264Param->LoopFilterDisable);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "LoopFilterAlphaC0Offset : %d", pH264Param->LoopFilterAlphaC0Offset);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "LoopFilterBetaOffset    : %d", pH264Param->LoopFilterBetaOffset);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "SymbolMode              : %d", pH264Param->SymbolMode);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "PictureInterlace        : %d", pH264Param->PictureInterlace);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "Transform8x8Mode        : %d", pH264Param->Transform8x8Mode);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "DarkDisable             : %d", pH264Param->DarkDisable);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "SmoothDisable           : %d", pH264Param->SmoothDisable);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "StaticDisable           : %d", pH264Param->StaticDisable);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "ActivityDisable         : %d", pH264Param->ActivityDisable);

    /* rate control related parameters */
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "EnableFRMRateControl    : %d", pCommonParam->EnableFRMRateControl);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "EnableMBRateControl     : %d", pCommonParam->EnableMBRateControl);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "CBRPeriodRf             : %d", pCommonParam->CBRPeriodRf);
}

static void Set_H264Enc_Param(EXYNOS_OMX_BASECOMPONENT *pExynosComponent)
{
    EXYNOS_OMX_BASEPORT           *pExynosInputPort  = NULL;
    EXYNOS_OMX_BASEPORT           *pExynosOutputPort = NULL;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc         = NULL;
    EXYNOS_H264ENC_HANDLE         *pH264Enc          = NULL;
    EXYNOS_MFC_H264ENC_HANDLE     *pMFCH264Handle    = NULL;

    ExynosVideoEncParam       *pEncParam    = NULL;
    ExynosVideoEncCommonParam *pCommonParam = NULL;
    ExynosVideoEncH264Param   *pH264Param   = NULL;

    pVideoEnc = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    pH264Enc = (EXYNOS_H264ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
    pMFCH264Handle = &pH264Enc->hMFCH264Handle;
    pExynosInputPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    pExynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];

    pEncParam = &pMFCH264Handle->encParam;
    pCommonParam = &pEncParam->commonParam;
    pH264Param = &pEncParam->codecParam.h264;
    pEncParam->eCompressionFormat = VIDEO_CODING_AVC;
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "eCompressionFormat: %d", pEncParam->eCompressionFormat);

    /* common parameters */
    pCommonParam->SourceWidth  = pExynosOutputPort->portDefinition.format.video.nFrameWidth;
    pCommonParam->SourceHeight = pExynosOutputPort->portDefinition.format.video.nFrameHeight;
    pCommonParam->IDRPeriod    = pH264Enc->AVCComponent[OUTPUT_PORT_INDEX].nPFrames + 1;
    pCommonParam->SliceMode    = 0;
    pCommonParam->Bitrate      = pExynosOutputPort->portDefinition.format.video.nBitrate;
    pCommonParam->FrameQp      = pVideoEnc->quantization.nQpI;
    pCommonParam->FrameQp_P    = pVideoEnc->quantization.nQpP;
    pCommonParam->QSCodeMax    = 51;
    pCommonParam->QSCodeMin    = 10;
    pCommonParam->PadControlOn = 0;    /* 0: disable, 1: enable */
    pCommonParam->LumaPadVal   = 0;
    pCommonParam->CbPadVal     = 0;
    pCommonParam->CrPadVal     = 0;

    if (pVideoEnc->intraRefresh.eRefreshMode == OMX_VIDEO_IntraRefreshCyclic) {
        /* Cyclic Mode */
        pCommonParam->RandomIntraMBRefresh = pVideoEnc->intraRefresh.nCirMBs;
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "RandomIntraMBRefresh: %d", pCommonParam->RandomIntraMBRefresh);
    } else {
        /* Don't support "Adaptive" and "Cyclic + Adaptive" */
        pCommonParam->RandomIntraMBRefresh = 0;
    }

    if (pExynosInputPort->bufferProcessType == BUFFER_SHARE) {
        if (pVideoEnc->ANBColorFormat == OMX_COLOR_FormatYUV420SemiPlanar)
            pCommonParam->FrameMap = VIDEO_COLORFORMAT_NV12;
        if (pVideoEnc->ANBColorFormat == OMX_SEC_COLOR_FormatNV12Tiled)
            pCommonParam->FrameMap = VIDEO_COLORFORMAT_NV12_TILED;
    } else {
        switch ((EXYNOS_OMX_COLOR_FORMATTYPE)pExynosInputPort->portDefinition.format.video.eColorFormat) {
        case OMX_COLOR_FormatYUV420SemiPlanar:
        case OMX_COLOR_FormatYUV420Planar: /* Converted to NV12 in Exynos_CSC_InputData */
#ifdef USE_METADATABUFFERTYPE
        case OMX_COLOR_FormatAndroidOpaque:
#endif
            pCommonParam->FrameMap = VIDEO_COLORFORMAT_NV12;
            break;
        case OMX_SEC_COLOR_FormatNV21Linear:
            pCommonParam->FrameMap = VIDEO_COLORFORMAT_NV21;
            break;
        case OMX_SEC_COLOR_FormatNV12Tiled:
        default:
            pCommonParam->FrameMap = VIDEO_COLORFORMAT_NV12_TILED;
            break;
        }
    }

    /* H.264 specific parameters */
    pH264Param->ProfileIDC   = OMXAVCProfileToProfileIDC(pH264Enc->AVCComponent[OUTPUT_PORT_INDEX].eProfile);    /*0: OMX_VIDEO_AVCProfileMain */
    pH264Param->LevelIDC     = OMXAVCLevelToLevelIDC(pH264Enc->AVCComponent[OUTPUT_PORT_INDEX].eLevel);    /*40: OMX_VIDEO_AVCLevel4 */
    pH264Param->FrameQp_B    = pVideoEnc->quantization.nQpB;
    pH264Param->FrameRate    = (pExynosInputPort->portDefinition.format.video.xFramerate) >> 16;
    pH264Param->SliceArgument = 0;    /* Slice mb/byte size number */
    pH264Param->NumberBFrames = 0;    /* 0 ~ 2 */
    pH264Param->NumberReferenceFrames = 1;
    pH264Param->NumberRefForPframes   = 1;
    pH264Param->LoopFilterDisable     = 1;    /* 1: Loop Filter Disable, 0: Filter Enable */
    pH264Param->LoopFilterAlphaC0Offset = 0;
    pH264Param->LoopFilterBetaOffset    = 0;
    pH264Param->SymbolMode       = 0;    /* 0: CAVLC, 1: CABAC */
    pH264Param->PictureInterlace = 0;
    pH264Param->Transform8x8Mode = 0;    /* 0: 4x4, 1: allow 8x8 */
    pH264Param->DarkDisable     = 1;
    pH264Param->SmoothDisable   = 1;
    pH264Param->StaticDisable   = 1;
    pH264Param->ActivityDisable = 1;

    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "pVideoEnc->eControlRate[OUTPUT_PORT_INDEX]: 0x%x", pVideoEnc->eControlRate[OUTPUT_PORT_INDEX]);
    /* rate control related parameters */
    switch (pVideoEnc->eControlRate[OUTPUT_PORT_INDEX]) {
    case OMX_Video_ControlRateDisable:
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "Video Encode DBR");
        pCommonParam->EnableFRMRateControl = 0;    /* 0: Disable, 1: Frame level RC */
        pCommonParam->EnableMBRateControl  = 0;    /* 0: Disable, 1:MB level RC */
        pCommonParam->CBRPeriodRf          = 100;
        break;
    case OMX_Video_ControlRateConstant:
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "Video Encode CBR");
        pCommonParam->EnableFRMRateControl = 1;    /* 0: Disable, 1: Frame level RC */
        pCommonParam->EnableMBRateControl  = 1;    /* 0: Disable, 1:MB level RC */
        pCommonParam->CBRPeriodRf          = 9;
        break;
    case OMX_Video_ControlRateVariable:
    default: /*Android default */
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "Video Encode VBR");
        pCommonParam->EnableFRMRateControl = 1;    /* 0: Disable, 1: Frame level RC */
        pCommonParam->EnableMBRateControl  = 1;    /* 0: Disable, 1:MB level RC */
        pCommonParam->CBRPeriodRf          = 100;
        break;
    }

    Print_H264Enc_Param(pEncParam);
}

static void Change_H264Enc_Param(EXYNOS_OMX_BASECOMPONENT *pExynosComponent)
{
    EXYNOS_OMX_BASEPORT           *pExynosInputPort  = NULL;
    EXYNOS_OMX_BASEPORT           *pExynosOutputPort = NULL;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc         = NULL;
    EXYNOS_H264ENC_HANDLE         *pH264Enc          = NULL;
    EXYNOS_MFC_H264ENC_HANDLE     *pMFCH264Handle    = NULL;

    ExynosVideoEncOps         *pEncOps      = NULL;
    ExynosVideoEncParam       *pEncParam    = NULL;
    ExynosVideoEncCommonParam *pCommonParam = NULL;
    ExynosVideoEncH264Param   *pH264Param   = NULL;

    int setParam = 0;

    pVideoEnc = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    pH264Enc = (EXYNOS_H264ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
    pMFCH264Handle = &pH264Enc->hMFCH264Handle;
    pExynosInputPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    pExynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    pEncOps = pMFCH264Handle->pEncOps;

    pEncParam = &pMFCH264Handle->encParam;
    pCommonParam = &pEncParam->commonParam;
    pH264Param = &pEncParam->codecParam.h264;

    if (pVideoEnc->IntraRefreshVOP == OMX_TRUE) {
        setParam = VIDEO_FRAME_I;
        pEncOps->Set_FrameType(pH264Enc->hMFCH264Handle.hMFCHandle, setParam);
        pVideoEnc->IntraRefreshVOP = OMX_FALSE;
    }
    if (pCommonParam->IDRPeriod != (int)pH264Enc->AVCComponent[OUTPUT_PORT_INDEX].nPFrames + 1) {
        setParam = pH264Enc->AVCComponent[OUTPUT_PORT_INDEX].nPFrames + 1;
        pEncOps->Set_IDRPeriod(pH264Enc->hMFCH264Handle.hMFCHandle, setParam);
    }
    if (pCommonParam->Bitrate != (int)pExynosOutputPort->portDefinition.format.video.nBitrate) {
        setParam = pExynosOutputPort->portDefinition.format.video.nBitrate;
        pEncOps->Set_BitRate(pH264Enc->hMFCH264Handle.hMFCHandle, setParam);
    }
    if (pH264Param->FrameRate != (int)((pExynosInputPort->portDefinition.format.video.xFramerate) >> 16)) {
        setParam = (pExynosInputPort->portDefinition.format.video.xFramerate) >> 16;
        pEncOps->Set_FrameRate(pH264Enc->hMFCH264Handle.hMFCHandle, setParam);
    }

    Set_H264Enc_Param(pExynosComponent);
}

OMX_ERRORTYPE GetCodecInputPrivateData(OMX_PTR codecBuffer, OMX_PTR addr[], OMX_U32 size[])
{
    OMX_ERRORTYPE       ret = OMX_ErrorNone;

EXIT:
    return ret;
}

OMX_ERRORTYPE GetCodecOutputPrivateData(OMX_PTR codecBuffer, OMX_PTR *pVirtAddr, OMX_U32 *dataSize)
{
    OMX_ERRORTYPE       ret = OMX_ErrorNone;
    ExynosVideoBuffer  *pCodecBuffer;

    if (codecBuffer == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pCodecBuffer = (ExynosVideoBuffer *)codecBuffer;

    if (pVirtAddr != NULL)
        *pVirtAddr = pCodecBuffer->planes[0].addr;

    if (dataSize != NULL)
        *dataSize = pCodecBuffer->planes[0].allocSize;

    pCodecBuffer = (ExynosVideoBuffer *)codecBuffer;

EXIT:
    return ret;
}

OMX_ERRORTYPE H264CodecOpen(EXYNOS_H264ENC_HANDLE *pH264Enc)
{
    OMX_ERRORTYPE           ret = OMX_ErrorNone;

    ExynosVideoEncOps       *pEncOps    = NULL;
    ExynosVideoEncBufferOps *pInbufOps  = NULL;
    ExynosVideoEncBufferOps *pOutbufOps = NULL;

    FunctionIn();

    if (pH264Enc == NULL) {
        ret = OMX_ErrorBadParameter;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorBadParameter, Line:%d", __LINE__);
        goto EXIT;
    }

    /* alloc ops structure */
    pEncOps = (ExynosVideoEncOps *)Exynos_OSAL_Malloc(sizeof(ExynosVideoEncOps));
    pInbufOps = (ExynosVideoEncBufferOps *)Exynos_OSAL_Malloc(sizeof(ExynosVideoEncBufferOps));
    pOutbufOps = (ExynosVideoEncBufferOps *)Exynos_OSAL_Malloc(sizeof(ExynosVideoEncBufferOps));

    if ((pEncOps == NULL) || (pInbufOps == NULL) || (pOutbufOps == NULL)) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to allocate encoder ops buffer");
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    pH264Enc->hMFCH264Handle.pEncOps = pEncOps;
    pH264Enc->hMFCH264Handle.pInbufOps = pInbufOps;
    pH264Enc->hMFCH264Handle.pOutbufOps = pOutbufOps;

    /* function pointer mapping */
    pEncOps->nSize = sizeof(ExynosVideoEncOps);
    pInbufOps->nSize = sizeof(ExynosVideoEncBufferOps);
    pOutbufOps->nSize = sizeof(ExynosVideoEncBufferOps);

    Exynos_Video_Register_Encoder(pEncOps, pInbufOps, pOutbufOps);

    /* check mandatory functions for encoder ops */
    if ((pEncOps->Init == NULL) || (pEncOps->Finalize == NULL) ||
        (pEncOps->Set_FrameTag == NULL) || (pEncOps->Get_FrameTag == NULL)) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Mandatory functions must be supplied");
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    /* check mandatory functions for buffer ops */
    if ((pInbufOps->Setup == NULL) || (pOutbufOps->Setup == NULL) ||
        (pInbufOps->Run == NULL) || (pOutbufOps->Run == NULL) ||
        (pInbufOps->Stop == NULL) || (pOutbufOps->Stop == NULL) ||
        (pInbufOps->Enqueue == NULL) || (pOutbufOps->Enqueue == NULL) ||
        (pInbufOps->Dequeue == NULL) || (pOutbufOps->Dequeue == NULL)) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Mandatory functions must be supplied");
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    /* alloc context, open, querycap */
    pH264Enc->hMFCH264Handle.hMFCHandle = pH264Enc->hMFCH264Handle.pEncOps->Init(V4L2_MEMORY_DMABUF);
    if (pH264Enc->hMFCH264Handle.hMFCHandle == NULL) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to allocate context buffer");
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    ret = OMX_ErrorNone;

EXIT:
    if (ret != OMX_ErrorNone) {
        if (pEncOps != NULL) {
            Exynos_OSAL_Free(pEncOps);
            pH264Enc->hMFCH264Handle.pEncOps = NULL;
        }
        if (pInbufOps != NULL) {
            Exynos_OSAL_Free(pInbufOps);
            pH264Enc->hMFCH264Handle.pInbufOps = NULL;
        }
        if (pOutbufOps != NULL) {
            Exynos_OSAL_Free(pOutbufOps);
            pH264Enc->hMFCH264Handle.pOutbufOps = NULL;
        }
    }

    FunctionOut();

    return ret;
}

OMX_ERRORTYPE H264CodecClose(EXYNOS_H264ENC_HANDLE *pH264Enc)
{
    OMX_ERRORTYPE            ret = OMX_ErrorNone;
    void                    *hMFCHandle = NULL;
    ExynosVideoEncOps       *pEncOps    = NULL;
    ExynosVideoEncBufferOps *pInbufOps  = NULL;
    ExynosVideoEncBufferOps *pOutbufOps = NULL;

    FunctionIn();

    if (pH264Enc == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    hMFCHandle = pH264Enc->hMFCH264Handle.hMFCHandle;
    pEncOps    = pH264Enc->hMFCH264Handle.pEncOps;
    pInbufOps  = pH264Enc->hMFCH264Handle.pInbufOps;
    pOutbufOps = pH264Enc->hMFCH264Handle.pOutbufOps;

    if (hMFCHandle != NULL) {
        pEncOps->Finalize(hMFCHandle);
        hMFCHandle = pH264Enc->hMFCH264Handle.hMFCHandle = NULL;
    }
    if (pOutbufOps != NULL) {
        Exynos_OSAL_Free(pOutbufOps);
        pOutbufOps = pH264Enc->hMFCH264Handle.pOutbufOps = NULL;
    }
    if (pInbufOps != NULL) {
        Exynos_OSAL_Free(pInbufOps);
        pInbufOps = pH264Enc->hMFCH264Handle.pInbufOps = NULL;
    }
    if (pEncOps != NULL) {
        Exynos_OSAL_Free(pEncOps);
        pEncOps = pH264Enc->hMFCH264Handle.pEncOps = NULL;
    }

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE H264CodecStart(OMX_COMPONENTTYPE *pOMXComponent, OMX_U32 nPortIndex)
{
    OMX_ERRORTYPE            ret = OMX_ErrorNone;
    void                    *hMFCHandle = NULL;
    ExynosVideoEncOps       *pEncOps    = NULL;
    ExynosVideoEncBufferOps *pInbufOps  = NULL;
    ExynosVideoEncBufferOps *pOutbufOps = NULL;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc = NULL;
    EXYNOS_H264ENC_HANDLE   *pH264Enc = NULL;

    FunctionIn();

    if (pOMXComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pVideoEnc = (EXYNOS_OMX_VIDEOENC_COMPONENT *)((EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate)->hComponentHandle;
    if (pVideoEnc == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pH264Enc = (EXYNOS_H264ENC_HANDLE *)pVideoEnc->hCodecHandle;
    if (pH264Enc == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    hMFCHandle = pH264Enc->hMFCH264Handle.hMFCHandle;
    pEncOps    = pH264Enc->hMFCH264Handle.pEncOps;
    pInbufOps  = pH264Enc->hMFCH264Handle.pInbufOps;
    pOutbufOps = pH264Enc->hMFCH264Handle.pOutbufOps;

    if (nPortIndex == INPUT_PORT_INDEX)
        pInbufOps->Run(hMFCHandle);
    else if (nPortIndex == OUTPUT_PORT_INDEX)
        pOutbufOps->Run(hMFCHandle);

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE H264CodecStop(OMX_COMPONENTTYPE *pOMXComponent, OMX_U32 nPortIndex)
{
    OMX_ERRORTYPE            ret = OMX_ErrorNone;
    void                    *hMFCHandle = NULL;
    ExynosVideoEncOps       *pEncOps    = NULL;
    ExynosVideoEncBufferOps *pInbufOps  = NULL;
    ExynosVideoEncBufferOps *pOutbufOps = NULL;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc = NULL;
    EXYNOS_H264ENC_HANDLE   *pH264Enc = NULL;

    FunctionIn();

    if (pOMXComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pVideoEnc = (EXYNOS_OMX_VIDEOENC_COMPONENT *)((EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate)->hComponentHandle;
    if (pVideoEnc == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pH264Enc = (EXYNOS_H264ENC_HANDLE *)pVideoEnc->hCodecHandle;
    if (pH264Enc == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    hMFCHandle = pH264Enc->hMFCH264Handle.hMFCHandle;
    pEncOps    = pH264Enc->hMFCH264Handle.pEncOps;
    pInbufOps  = pH264Enc->hMFCH264Handle.pInbufOps;
    pOutbufOps = pH264Enc->hMFCH264Handle.pOutbufOps;

    if ((nPortIndex == INPUT_PORT_INDEX) && (pInbufOps != NULL))
        pInbufOps->Stop(hMFCHandle);
    else if ((nPortIndex == OUTPUT_PORT_INDEX) && (pOutbufOps != NULL))
        pOutbufOps->Stop(hMFCHandle);

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE H264CodecOutputBufferProcessRun(OMX_COMPONENTTYPE *pOMXComponent, OMX_U32 nPortIndex)
{
    OMX_ERRORTYPE            ret = OMX_ErrorNone;
    void                    *hMFCHandle = NULL;
    ExynosVideoEncOps       *pEncOps    = NULL;
    ExynosVideoEncBufferOps *pInbufOps  = NULL;
    ExynosVideoEncBufferOps *pOutbufOps = NULL;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc = NULL;
    EXYNOS_H264ENC_HANDLE   *pH264Enc = NULL;

    FunctionIn();

    if (pOMXComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pVideoEnc = (EXYNOS_OMX_VIDEOENC_COMPONENT *)((EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate)->hComponentHandle;
    if (pVideoEnc == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pH264Enc = (EXYNOS_H264ENC_HANDLE *)pVideoEnc->hCodecHandle;
    if (pH264Enc == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    hMFCHandle = pH264Enc->hMFCH264Handle.hMFCHandle;
    pEncOps    = pH264Enc->hMFCH264Handle.pEncOps;
    pInbufOps  = pH264Enc->hMFCH264Handle.pInbufOps;
    pOutbufOps = pH264Enc->hMFCH264Handle.pOutbufOps;

    if (nPortIndex == INPUT_PORT_INDEX) {
        if (pH264Enc->bSourceStart == OMX_FALSE) {
            Exynos_OSAL_SignalSet(pH264Enc->hSourceStartEvent);
            Exynos_OSAL_SleepMillisec(0);
        }
    }

    if (nPortIndex == OUTPUT_PORT_INDEX) {
        if (pH264Enc->bDestinationStart == OMX_FALSE) {
            Exynos_OSAL_SignalSet(pH264Enc->hDestinationStartEvent);
            Exynos_OSAL_SleepMillisec(0);
        }
    }

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE H264CodecRegistCodecBuffers(
    OMX_COMPONENTTYPE   *pOMXComponent,
    OMX_U32              nPortIndex,
    OMX_U32              nBufferCnt)
{
    OMX_ERRORTYPE                    ret                = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT        *pExynosComponent   = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEOENC_COMPONENT   *pVideoEnc          = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_H264ENC_HANDLE           *pH264Enc           = (EXYNOS_H264ENC_HANDLE *)pVideoEnc->hCodecHandle;
    void                            *hMFCHandle         = pH264Enc->hMFCH264Handle.hMFCHandle;
    CODEC_ENC_BUFFER               **ppCodecBuffer      = NULL;
    ExynosVideoEncBufferOps         *pBufOps            = NULL;
    ExynosVideoPlane                *pPlanes            = NULL;

    OMX_U32 nPlaneCnt = 0;
    int i, j;

    FunctionIn();

    if (nPortIndex == INPUT_PORT_INDEX) {
        ppCodecBuffer   = &(pVideoEnc->pMFCEncInputBuffer[0]);
        nPlaneCnt       = MFC_INPUT_BUFFER_PLANE;
        pBufOps         = pH264Enc->hMFCH264Handle.pInbufOps;
    } else {
        ppCodecBuffer   = &(pVideoEnc->pMFCEncOutputBuffer[0]);
        nPlaneCnt       = MFC_OUTPUT_BUFFER_PLANE;
        pBufOps         = pH264Enc->hMFCH264Handle.pOutbufOps;
    }

    pPlanes = (ExynosVideoPlane *)Exynos_OSAL_Malloc(sizeof(ExynosVideoPlane) * nPlaneCnt);
    if (pPlanes == NULL) {
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    /* Register buffer */
    for (i = 0; i < nBufferCnt; i++) {
        for (j = 0; j < nPlaneCnt; j++) {
            pPlanes[j].addr         = ppCodecBuffer[i]->pVirAddr[j];
            pPlanes[j].fd           = ppCodecBuffer[i]->fd[j];
            pPlanes[j].allocSize    = ppCodecBuffer[i]->bufferSize[j];
        }

        if (pBufOps->Register(hMFCHandle, pPlanes, nPlaneCnt) != VIDEO_ERROR_NONE) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "PORT[%d]: Failed to Register buffer", nPortIndex);
            ret = OMX_ErrorInsufficientResources;
            Exynos_OSAL_Free(pPlanes);
            goto EXIT;
        }
    }

    Exynos_OSAL_Free(pPlanes);

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE H264CodecEnQueueAllBuffer(OMX_COMPONENTTYPE *pOMXComponent, OMX_U32 nPortIndex)
{
    OMX_ERRORTYPE                  ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_H264ENC_HANDLE         *pH264Enc = (EXYNOS_H264ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
    void                          *hMFCHandle = pH264Enc->hMFCH264Handle.hMFCHandle;
    EXYNOS_OMX_BASEPORT           *pExynosInputPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT           *pExynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    int i, nOutbufs;

    ExynosVideoEncOps       *pEncOps    = pH264Enc->hMFCH264Handle.pEncOps;
    ExynosVideoEncBufferOps *pInbufOps  = pH264Enc->hMFCH264Handle.pInbufOps;
    ExynosVideoEncBufferOps *pOutbufOps = pH264Enc->hMFCH264Handle.pOutbufOps;

    FunctionIn();

    if ((nPortIndex != INPUT_PORT_INDEX) && (nPortIndex != OUTPUT_PORT_INDEX)) {
        ret = OMX_ErrorBadPortIndex;
        goto EXIT;
    }

    if ((nPortIndex == INPUT_PORT_INDEX) &&
        (pH264Enc->bSourceStart == OMX_TRUE)) {
        Exynos_CodecBufferReset(pExynosComponent, INPUT_PORT_INDEX);

        for (i = 0; i < MFC_INPUT_BUFFER_NUM_MAX; i++)  {
            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "pVideoEnc->pMFCEncInputBuffer[%d]: 0x%x", i, pVideoEnc->pMFCEncInputBuffer[i]);
            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "pVideoEnc->pMFCEncInputBuffer[%d]->pVirAddr[0]: 0x%x", i, pVideoEnc->pMFCEncInputBuffer[i]->pVirAddr[0]);
            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "pVideoEnc->pMFCEncInputBuffer[%d]->pVirAddr[1]: 0x%x", i, pVideoEnc->pMFCEncInputBuffer[i]->pVirAddr[1]);

            Exynos_CodecBufferEnQueue(pExynosComponent, INPUT_PORT_INDEX, pVideoEnc->pMFCEncInputBuffer[i]);
        }

        pInbufOps->Clear_Queue(hMFCHandle);
    } else if ((nPortIndex == OUTPUT_PORT_INDEX) &&
               (pH264Enc->bDestinationStart == OMX_TRUE)) {
        OMX_U32 dataLen[2] = {0, 0};
        ExynosVideoBuffer *pBuffer = NULL;

        Exynos_CodecBufferReset(pExynosComponent, OUTPUT_PORT_INDEX);

        for (i = 0; i < MFC_OUTPUT_BUFFER_NUM_MAX; i++) {
            pOutbufOps->Get_Buffer(hMFCHandle, i, &pBuffer);
            Exynos_CodecBufferEnQueue(pExynosComponent, OUTPUT_PORT_INDEX, (OMX_PTR)pBuffer);
            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "pVideoEnc->pMFCEncOutputBuffer[%d]: 0x%x", i, pVideoEnc->pMFCEncOutputBuffer[i]);
            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "pVideoEnc->pMFCEncInputBuffer[%d]->pVirAddr[0]: 0x%x", i, pVideoEnc->pMFCEncInputBuffer[i]->pVirAddr[0]);
            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "pVideoEnc->pMFCEncInputBuffer[%d]->pVirAddr[1]: 0x%x", i, pVideoEnc->pMFCEncInputBuffer[i]->pVirAddr[1]);
        }
        pOutbufOps->Clear_Queue(hMFCHandle);
    } else {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE H264CodecSrcSetup(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *pSrcInputData)
{
    OMX_ERRORTYPE                  ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_H264ENC_HANDLE         *pH264Enc = (EXYNOS_H264ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
    EXYNOS_MFC_H264ENC_HANDLE     *pMFCH264Handle    = &pH264Enc->hMFCH264Handle;
    void                          *hMFCHandle = pMFCH264Handle->hMFCHandle;
    EXYNOS_OMX_BASEPORT           *pExynosInputPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT           *pExynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    OMX_U32                        oneFrameSize = pSrcInputData->dataLen;

    ExynosVideoEncOps       *pEncOps    = pH264Enc->hMFCH264Handle.pEncOps;
    ExynosVideoEncBufferOps *pInbufOps  = pH264Enc->hMFCH264Handle.pInbufOps;
    ExynosVideoEncBufferOps *pOutbufOps = pH264Enc->hMFCH264Handle.pOutbufOps;
    ExynosVideoEncParam     *pEncParam    = NULL;

    ExynosVideoGeometry      bufferConf;
    OMX_U32                  inputBufferNumber = 0;
    int i, nOutbufs;

    FunctionIn();

    if ((oneFrameSize <= 0) && (pSrcInputData->nFlags & OMX_BUFFERFLAG_EOS)) {
        OMX_BUFFERHEADERTYPE *OMXBuffer = NULL;
        OMXBuffer = Exynos_OutputBufferGetQueue_Direct(pExynosComponent);
        if (OMXBuffer == NULL) {
            ret = OMX_ErrorUndefined;
            goto EXIT;
        }

        OMXBuffer->nTimeStamp = pSrcInputData->timeStamp;
        OMXBuffer->nFlags = pSrcInputData->nFlags;
        Exynos_OMX_OutputBufferReturn(pOMXComponent, OMXBuffer);

        ret = OMX_ErrorNone;
        goto EXIT;
    }

    Set_H264Enc_Param(pExynosComponent);
    pEncParam = &pMFCH264Handle->encParam;
    if (pEncOps->Set_EncParam) {
        if(pEncOps->Set_EncParam(pH264Enc->hMFCH264Handle.hMFCHandle, pEncParam) != VIDEO_ERROR_NONE) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to set geometry for input buffer");
            ret = OMX_ErrorInsufficientResources;
            goto EXIT;
        }
    }

    if (pMFCH264Handle->bPrependSpsPpsToIdr == OMX_TRUE) {
        if (pEncOps->Enable_PrependSpsPpsToIdr)
            pEncOps->Enable_PrependSpsPpsToIdr(pH264Enc->hMFCH264Handle.hMFCHandle);
        else
            Exynos_OSAL_Log(EXYNOS_LOG_WARNING, "%s: Not supported control: Enable_PrependSpsPpsToIdr", __func__);
    }

    /* input buffer info: only 3 config values needed */
    Exynos_OSAL_Memset(&bufferConf, 0, sizeof(bufferConf));
    bufferConf.eColorFormat = pEncParam->commonParam.FrameMap;//VIDEO_COLORFORMAT_NV12;
    bufferConf.nFrameWidth = pExynosInputPort->portDefinition.format.video.nFrameWidth;
    bufferConf.nFrameHeight = pExynosInputPort->portDefinition.format.video.nFrameHeight;
    pInbufOps->Set_Shareable(hMFCHandle);
    if (pExynosInputPort->bufferProcessType == BUFFER_SHARE) {
        inputBufferNumber = MAX_INPUTBUFFER_NUM_DYNAMIC;
    } else if ((pExynosInputPort->bufferProcessType & BUFFER_COPY) == BUFFER_COPY) {
        inputBufferNumber = MFC_INPUT_BUFFER_NUM_MAX;
    }

    if ((pExynosInputPort->bufferProcessType & BUFFER_COPY) == BUFFER_COPY) {
        /* should be done before prepare input buffer */
        if (pInbufOps->Enable_Cacheable(hMFCHandle) != VIDEO_ERROR_NONE) {
            ret = OMX_ErrorInsufficientResources;
            goto EXIT;
        }
    }

    /* set input buffer geometry */
    if (pInbufOps->Set_Geometry) {
        if (pInbufOps->Set_Geometry(hMFCHandle, &bufferConf) != VIDEO_ERROR_NONE) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to set geometry for input buffer");
            ret = OMX_ErrorInsufficientResources;
            goto EXIT;
        }
    }

    /* setup input buffer */
    if (pInbufOps->Setup(hMFCHandle, inputBufferNumber) != VIDEO_ERROR_NONE) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to setup input buffer");
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    ExynosVideoPlane planes[MFC_INPUT_BUFFER_PLANE];
    int plane;

    if ((pExynosInputPort->bufferProcessType & BUFFER_COPY) == BUFFER_COPY) {
        ret = H264CodecRegistCodecBuffers(pOMXComponent, INPUT_PORT_INDEX, MFC_INPUT_BUFFER_NUM_MAX);
        if (ret != OMX_ErrorNone)
            goto EXIT;
    } else if (pExynosInputPort->bufferProcessType == BUFFER_SHARE) {
        if (pExynosInputPort->bStoreMetaData == OMX_TRUE) {
            /*************/
            /*    TBD    */
            /*************/
            /* Does not require any actions. */
        } else {
            ret = OMX_ErrorNotImplemented;
            goto EXIT;
        }
    }

    pH264Enc->hMFCH264Handle.bConfiguredMFCSrc = OMX_TRUE;
    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE H264CodecDstSetup(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE                  ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_H264ENC_HANDLE         *pH264Enc = (EXYNOS_H264ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
    EXYNOS_MFC_H264ENC_HANDLE     *pMFCH264Handle    = &pH264Enc->hMFCH264Handle;
    void                          *hMFCHandle = pMFCH264Handle->hMFCHandle;
    EXYNOS_OMX_BASEPORT           *pExynosInputPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT           *pExynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];

    ExynosVideoEncOps       *pEncOps    = pH264Enc->hMFCH264Handle.pEncOps;
    ExynosVideoEncBufferOps *pInbufOps  = pH264Enc->hMFCH264Handle.pInbufOps;
    ExynosVideoEncBufferOps *pOutbufOps = pH264Enc->hMFCH264Handle.pOutbufOps;
    ExynosVideoGeometry      bufferConf;
    int i, nOutbufs;

    FunctionIn();

    int OutBufferSize = pExynosOutputPort->portDefinition.format.video.nFrameWidth * pExynosOutputPort->portDefinition.format.video.nFrameHeight * 3 / 2;
    /* set geometry for output (dst) */
    if (pOutbufOps->Set_Geometry) {
        /* output buffer info: only 2 config values needed */
        bufferConf.eCompressionFormat = VIDEO_CODING_AVC;
        bufferConf.nSizeImage = OutBufferSize;

        if (pOutbufOps->Set_Geometry(pH264Enc->hMFCH264Handle.hMFCHandle, &bufferConf) != VIDEO_ERROR_NONE) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to set geometry for output buffer");
            ret = OMX_ErrorInsufficientResources;
            goto EXIT;
        }
    }

    /* should be done before prepare output buffer */
    if (pOutbufOps->Enable_Cacheable(hMFCHandle) != VIDEO_ERROR_NONE) {
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    pOutbufOps->Set_Shareable(hMFCHandle);
    int SetupBufferNumber = 0;
    if ((pExynosOutputPort->bufferProcessType & BUFFER_COPY) == BUFFER_COPY)
        SetupBufferNumber = MFC_OUTPUT_BUFFER_NUM_MAX;
    else
        SetupBufferNumber = pExynosOutputPort->portDefinition.nBufferCountActual;
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "SetupBufferNumber:%d", SetupBufferNumber);

    if (pOutbufOps->Setup(pH264Enc->hMFCH264Handle.hMFCHandle, SetupBufferNumber) != VIDEO_ERROR_NONE) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to setup output buffer");
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    OMX_U32 dataLen[MFC_OUTPUT_BUFFER_PLANE] = {0};
    if ((pExynosOutputPort->bufferProcessType & BUFFER_COPY) == BUFFER_COPY) {
        OMX_U32 nPlaneSize[MFC_OUTPUT_BUFFER_PLANE] = {0};
        nPlaneSize[0] = OutBufferSize;
        ret = Exynos_Allocate_CodecBuffers(pOMXComponent, OUTPUT_PORT_INDEX, MFC_OUTPUT_BUFFER_NUM_MAX, nPlaneSize);
        if (ret != OMX_ErrorNone)
            goto EXIT;

        ret = H264CodecRegistCodecBuffers(pOMXComponent, OUTPUT_PORT_INDEX, MFC_OUTPUT_BUFFER_NUM_MAX);
        if (ret != OMX_ErrorNone)
            goto EXIT;

        /* Enqueue output buffer */
        for (i = 0; i < MFC_OUTPUT_BUFFER_NUM_MAX; i++) {
            pOutbufOps->Enqueue(hMFCHandle, (unsigned char **)pVideoEnc->pMFCEncOutputBuffer[i]->pVirAddr,
                                (unsigned int *)dataLen, MFC_OUTPUT_BUFFER_PLANE, NULL);
        }
    } else if ((pExynosOutputPort->bufferProcessType & BUFFER_SHARE) == BUFFER_SHARE) {
        /* Register output buffer */
        /*************/
        /*    TBD    */
        /*************/
        ExynosVideoErrorType codecReturn = VIDEO_ERROR_NONE;
        ExynosVideoPlane plane;
        for (i = 0; i < pExynosOutputPort->portDefinition.nBufferCountActual; i++) {
            plane.addr = pExynosOutputPort->extendBufferHeader[i].OMXBufferHeader->pBuffer;
            plane.fd = pExynosOutputPort->extendBufferHeader[i].buf_fd[0];
            plane.allocSize = OutBufferSize;
            if (pOutbufOps->Register(hMFCHandle, &plane, MFC_OUTPUT_BUFFER_PLANE) != VIDEO_ERROR_NONE) {
                Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to Register output buffer");
                ret = OMX_ErrorInsufficientResources;
                goto EXIT;
            }
            codecReturn = pOutbufOps->Enqueue(hMFCHandle, (unsigned char **)&pExynosOutputPort->extendBufferHeader[i].OMXBufferHeader->pBuffer,
                                   (unsigned int *)dataLen, MFC_OUTPUT_BUFFER_PLANE, NULL);
            if (codecReturn != VIDEO_ERROR_NONE) {
                Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Output buffer that has been entered is invalid.");
                ret = OMX_ErrorUndefined;
                goto EXIT;
            }
        }
    }

    /* start header encoding */
    if (pOutbufOps->Run(hMFCHandle) != VIDEO_ERROR_NONE) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to run output buffer");
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    if (pExynosOutputPort->bufferProcessType == BUFFER_SHARE) {
        OMX_BUFFERHEADERTYPE *OMXBuffer = NULL;
        ExynosVideoBuffer *pVideoBuffer = NULL;

        OMXBuffer = Exynos_OutputBufferGetQueue_Direct(pExynosComponent);
        if (OMXBuffer == OMX_ErrorNone) {
            ret = OMX_ErrorUndefined;
            goto EXIT;
        }

        if ((pVideoBuffer = pOutbufOps->Dequeue(hMFCHandle)) == NULL) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: %d: Failed - pOutbufOps->Dequeue", __FUNCTION__, __LINE__);
            ret = OMX_ErrorUndefined;
            goto EXIT;
        }

        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "dst:0x%x, src:0x%x, dataSize:%d",
                            OMXBuffer->pBuffer,
                            pVideoBuffer->planes[0].addr,
                            pVideoBuffer->planes[0].dataSize);
        Exynos_OSAL_Memcpy(OMXBuffer->pBuffer, pVideoBuffer->planes[0].addr, pVideoBuffer->planes[0].dataSize);
        OMXBuffer->nFilledLen = pVideoBuffer->planes[0].dataSize;
        OMXBuffer->nOffset = 0;
        OMXBuffer->nTimeStamp = 0;
        OMXBuffer->nFlags |= OMX_BUFFERFLAG_CODECCONFIG;
        OMXBuffer->nFlags |= OMX_BUFFERFLAG_ENDOFFRAME;
        Exynos_OMX_OutputBufferReturn(pOMXComponent, OMXBuffer);

        pVideoEnc->bFirstOutput = OMX_TRUE;
        ret = OMX_ErrorNone;

        H264CodecStop(pOMXComponent, OUTPUT_PORT_INDEX);
    }
    pH264Enc->hMFCH264Handle.bConfiguredMFCDst = OMX_TRUE;

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_H264Enc_GetParameter(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nParamIndex,
    OMX_INOUT OMX_PTR     pComponentParameterStructure)
{
    OMX_ERRORTYPE             ret              = OMX_ErrorNone;
    OMX_COMPONENTTYPE        *pOMXComponent    = NULL;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;

    FunctionIn();

    if (hComponent == NULL || pComponentParameterStructure == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = Exynos_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }
    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    if (pExynosComponent->currentState == OMX_StateInvalid ) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    switch (nParamIndex) {
    case OMX_IndexParamVideoAvc:
    {
        OMX_VIDEO_PARAM_AVCTYPE *pDstAVCComponent = (OMX_VIDEO_PARAM_AVCTYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_AVCTYPE *pSrcAVCComponent = NULL;
        EXYNOS_H264ENC_HANDLE   *pH264Enc = NULL;

        ret = Exynos_OMX_Check_SizeVersion(pDstAVCComponent, sizeof(OMX_VIDEO_PARAM_AVCTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pDstAVCComponent->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pH264Enc = (EXYNOS_H264ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
        pSrcAVCComponent = &pH264Enc->AVCComponent[pDstAVCComponent->nPortIndex];

        Exynos_OSAL_Memcpy(pDstAVCComponent, pSrcAVCComponent, sizeof(OMX_VIDEO_PARAM_AVCTYPE));
    }
        break;
    case OMX_IndexParamStandardComponentRole:
    {
        OMX_PARAM_COMPONENTROLETYPE *pComponentRole = (OMX_PARAM_COMPONENTROLETYPE *)pComponentParameterStructure;
        ret = Exynos_OMX_Check_SizeVersion(pComponentRole, sizeof(OMX_PARAM_COMPONENTROLETYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        Exynos_OSAL_Strcpy((char *)pComponentRole->cRole, EXYNOS_OMX_COMPONENT_H264_ENC_ROLE);
    }
        break;
    case OMX_IndexParamVideoProfileLevelQuerySupported:
    {
        OMX_VIDEO_PARAM_PROFILELEVELTYPE *pDstProfileLevel = (OMX_VIDEO_PARAM_PROFILELEVELTYPE*)pComponentParameterStructure;
        EXYNOS_OMX_VIDEO_PROFILELEVEL *pProfileLevel = NULL;
        OMX_U32 maxProfileLevelNum = 0;

        ret = Exynos_OMX_Check_SizeVersion(pDstProfileLevel, sizeof(OMX_VIDEO_PARAM_PROFILELEVELTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pDstProfileLevel->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pProfileLevel = supportedAVCProfileLevels;
        maxProfileLevelNum = sizeof(supportedAVCProfileLevels) / sizeof(EXYNOS_OMX_VIDEO_PROFILELEVEL);

        if (pDstProfileLevel->nProfileIndex >= maxProfileLevelNum) {
            ret = OMX_ErrorNoMore;
            goto EXIT;
        }

        pProfileLevel += pDstProfileLevel->nProfileIndex;
        pDstProfileLevel->eProfile = pProfileLevel->profile;
        pDstProfileLevel->eLevel = pProfileLevel->level;
    }
        break;
    case OMX_IndexParamVideoProfileLevelCurrent:
    {
        OMX_VIDEO_PARAM_PROFILELEVELTYPE *pDstProfileLevel = (OMX_VIDEO_PARAM_PROFILELEVELTYPE*)pComponentParameterStructure;
        OMX_VIDEO_PARAM_AVCTYPE *pSrcAVCComponent = NULL;
        EXYNOS_H264ENC_HANDLE *pH264Enc = NULL;

        ret = Exynos_OMX_Check_SizeVersion(pDstProfileLevel, sizeof(OMX_VIDEO_PARAM_PROFILELEVELTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pDstProfileLevel->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pH264Enc = (EXYNOS_H264ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
        pSrcAVCComponent = &pH264Enc->AVCComponent[pDstProfileLevel->nPortIndex];

        pDstProfileLevel->eProfile = pSrcAVCComponent->eProfile;
        pDstProfileLevel->eLevel = pSrcAVCComponent->eLevel;
    }
        break;
    case OMX_IndexParamVideoErrorCorrection:
    {
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pDstErrorCorrectionType = (OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pSrcErrorCorrectionType = NULL;
        EXYNOS_H264ENC_HANDLE *pH264Enc = NULL;

        ret = Exynos_OMX_Check_SizeVersion(pDstErrorCorrectionType, sizeof(OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pDstErrorCorrectionType->nPortIndex != OUTPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pH264Enc = (EXYNOS_H264ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
        pSrcErrorCorrectionType = &pH264Enc->errorCorrectionType[OUTPUT_PORT_INDEX];

        pDstErrorCorrectionType->bEnableHEC = pSrcErrorCorrectionType->bEnableHEC;
        pDstErrorCorrectionType->bEnableResync = pSrcErrorCorrectionType->bEnableResync;
        pDstErrorCorrectionType->nResynchMarkerSpacing = pSrcErrorCorrectionType->nResynchMarkerSpacing;
        pDstErrorCorrectionType->bEnableDataPartitioning = pSrcErrorCorrectionType->bEnableDataPartitioning;
        pDstErrorCorrectionType->bEnableRVLC = pSrcErrorCorrectionType->bEnableRVLC;
    }
        break;
    default:
        ret = Exynos_OMX_VideoEncodeGetParameter(hComponent, nParamIndex, pComponentParameterStructure);
        break;
    }
EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_H264Enc_SetParameter(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nIndex,
    OMX_IN OMX_PTR        pComponentParameterStructure)
{
    OMX_ERRORTYPE             ret              = OMX_ErrorNone;
    OMX_COMPONENTTYPE        *pOMXComponent    = NULL;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;

    FunctionIn();

    if (hComponent == NULL || pComponentParameterStructure == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = Exynos_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }
    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    if (pExynosComponent->currentState == OMX_StateInvalid ) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    switch (nIndex) {
    case OMX_IndexParamVideoAvc:
    {
        OMX_VIDEO_PARAM_AVCTYPE *pDstAVCComponent = NULL;
        OMX_VIDEO_PARAM_AVCTYPE *pSrcAVCComponent = (OMX_VIDEO_PARAM_AVCTYPE *)pComponentParameterStructure;
        EXYNOS_H264ENC_HANDLE   *pH264Enc = NULL;

        ret = Exynos_OMX_Check_SizeVersion(pSrcAVCComponent, sizeof(OMX_VIDEO_PARAM_AVCTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pSrcAVCComponent->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pH264Enc = (EXYNOS_H264ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
        pDstAVCComponent = &pH264Enc->AVCComponent[pSrcAVCComponent->nPortIndex];

        Exynos_OSAL_Memcpy(pDstAVCComponent, pSrcAVCComponent, sizeof(OMX_VIDEO_PARAM_AVCTYPE));
    }
        break;
    case OMX_IndexParamStandardComponentRole:
    {
        OMX_PARAM_COMPONENTROLETYPE *pComponentRole = (OMX_PARAM_COMPONENTROLETYPE*)pComponentParameterStructure;

        ret = Exynos_OMX_Check_SizeVersion(pComponentRole, sizeof(OMX_PARAM_COMPONENTROLETYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if ((pExynosComponent->currentState != OMX_StateLoaded) && (pExynosComponent->currentState != OMX_StateWaitForResources)) {
            ret = OMX_ErrorIncorrectStateOperation;
            goto EXIT;
        }

        if (!Exynos_OSAL_Strcmp((char*)pComponentRole->cRole, EXYNOS_OMX_COMPONENT_H264_ENC_ROLE)) {
            pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX].portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingAVC;
        } else {
            ret = OMX_ErrorBadParameter;
            goto EXIT;
        }
    }
        break;
    case OMX_IndexParamVideoProfileLevelCurrent:
    {
        OMX_VIDEO_PARAM_PROFILELEVELTYPE *pSrcProfileLevel = (OMX_VIDEO_PARAM_PROFILELEVELTYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_AVCTYPE *pDstAVCComponent = NULL;
        EXYNOS_H264ENC_HANDLE *pH264Enc = NULL;

        ret = Exynos_OMX_Check_SizeVersion(pSrcProfileLevel, sizeof(OMX_VIDEO_PARAM_PROFILELEVELTYPE));
        if (ret != OMX_ErrorNone)
            goto EXIT;

        if (pSrcProfileLevel->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pH264Enc = (EXYNOS_H264ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;

        pDstAVCComponent = &pH264Enc->AVCComponent[pSrcProfileLevel->nPortIndex];
        pDstAVCComponent->eProfile = pSrcProfileLevel->eProfile;
        pDstAVCComponent->eLevel = pSrcProfileLevel->eLevel;
    }
        break;
    case OMX_IndexParamVideoErrorCorrection:
    {
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pSrcErrorCorrectionType = (OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pDstErrorCorrectionType = NULL;
        EXYNOS_H264ENC_HANDLE *pH264Enc = NULL;

        ret = Exynos_OMX_Check_SizeVersion(pSrcErrorCorrectionType, sizeof(OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pSrcErrorCorrectionType->nPortIndex != OUTPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pH264Enc = (EXYNOS_H264ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
        pDstErrorCorrectionType = &pH264Enc->errorCorrectionType[OUTPUT_PORT_INDEX];

        pDstErrorCorrectionType->bEnableHEC = pSrcErrorCorrectionType->bEnableHEC;
        pDstErrorCorrectionType->bEnableResync = pSrcErrorCorrectionType->bEnableResync;
        pDstErrorCorrectionType->nResynchMarkerSpacing = pSrcErrorCorrectionType->nResynchMarkerSpacing;
        pDstErrorCorrectionType->bEnableDataPartitioning = pSrcErrorCorrectionType->bEnableDataPartitioning;
        pDstErrorCorrectionType->bEnableRVLC = pSrcErrorCorrectionType->bEnableRVLC;
    }
        break;
    case OMX_IndexParamPrependSPSPPSToIDR:
    {
        EXYNOS_H264ENC_HANDLE *pH264Enc = (EXYNOS_H264ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;

        ret = Exynos_OSAL_SetPrependSPSPPSToIDR(pComponentParameterStructure, &(pH264Enc->hMFCH264Handle.bPrependSpsPpsToIdr));
    }
        break;
    default:
        ret = Exynos_OMX_VideoEncodeSetParameter(hComponent, nIndex, pComponentParameterStructure);
        break;
    }
EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_H264Enc_GetConfig(
    OMX_HANDLETYPE hComponent,
    OMX_INDEXTYPE nIndex,
    OMX_PTR pComponentConfigStructure)
{
    OMX_ERRORTYPE             ret              = OMX_ErrorNone;
    OMX_COMPONENTTYPE        *pOMXComponent    = NULL;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;
    EXYNOS_H264ENC_HANDLE    *pH264Enc         = NULL;

    FunctionIn();

    if (hComponent == NULL || pComponentConfigStructure == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = Exynos_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }
    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    if (pExynosComponent->currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }
    pH264Enc = (EXYNOS_H264ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;

    switch (nIndex) {
    case OMX_IndexConfigVideoAVCIntraPeriod:
    {
        OMX_VIDEO_CONFIG_AVCINTRAPERIOD *pAVCIntraPeriod = (OMX_VIDEO_CONFIG_AVCINTRAPERIOD *)pComponentConfigStructure;
        OMX_U32           portIndex = pAVCIntraPeriod->nPortIndex;

        if ((portIndex != OUTPUT_PORT_INDEX)) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        } else {
            pAVCIntraPeriod->nIDRPeriod = pH264Enc->AVCComponent[OUTPUT_PORT_INDEX].nPFrames + 1;
            pAVCIntraPeriod->nPFrames = pH264Enc->AVCComponent[OUTPUT_PORT_INDEX].nPFrames;
        }
    }
        break;
    default:
        ret = Exynos_OMX_VideoEncodeGetConfig(hComponent, nIndex, pComponentConfigStructure);
        break;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_H264Enc_SetConfig(
    OMX_HANDLETYPE hComponent,
    OMX_INDEXTYPE nIndex,
    OMX_PTR pComponentConfigStructure)
{
    OMX_ERRORTYPE                  ret              = OMX_ErrorNone;
    OMX_COMPONENTTYPE             *pOMXComponent    = NULL;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent = NULL;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc        = NULL;
    EXYNOS_H264ENC_HANDLE         *pH264Enc         = NULL;

    FunctionIn();

    if (hComponent == NULL || pComponentConfigStructure == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = Exynos_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }
    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    if (pExynosComponent->currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    pVideoEnc = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    pH264Enc = (EXYNOS_H264ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;

    switch (nIndex) {
    case OMX_IndexConfigVideoIntraPeriod:
    {
        EXYNOS_OMX_VIDEOENC_COMPONENT *pVEncBase = ((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle);
        OMX_U32 nPFrames = (*((OMX_U32 *)pComponentConfigStructure)) - 1;

        pH264Enc->AVCComponent[OUTPUT_PORT_INDEX].nPFrames = nPFrames;

        ret = OMX_ErrorNone;
    }
        break;
    case OMX_IndexConfigVideoAVCIntraPeriod:
    {
        OMX_VIDEO_CONFIG_AVCINTRAPERIOD *pAVCIntraPeriod = (OMX_VIDEO_CONFIG_AVCINTRAPERIOD *)pComponentConfigStructure;
        OMX_U32           portIndex = pAVCIntraPeriod->nPortIndex;

        if ((portIndex != OUTPUT_PORT_INDEX)) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        } else {
            if (pAVCIntraPeriod->nIDRPeriod == (pAVCIntraPeriod->nPFrames + 1))
                pH264Enc->AVCComponent[OUTPUT_PORT_INDEX].nPFrames = pAVCIntraPeriod->nPFrames;
            else {
                ret = OMX_ErrorBadParameter;
                goto EXIT;
            }
        }
    }
        break;
    default:
        ret = Exynos_OMX_VideoEncodeSetConfig(hComponent, nIndex, pComponentConfigStructure);
        break;
    }

EXIT:
    if (ret == OMX_ErrorNone)
        pVideoEnc->configChange = OMX_TRUE;

    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_H264Enc_GetExtensionIndex(
    OMX_IN OMX_HANDLETYPE  hComponent,
    OMX_IN OMX_STRING      cParameterName,
    OMX_OUT OMX_INDEXTYPE *pIndexType)
{
    OMX_ERRORTYPE             ret              = OMX_ErrorNone;
    OMX_COMPONENTTYPE        *pOMXComponent    = NULL;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = Exynos_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }
    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    if ((cParameterName == NULL) || (pIndexType == NULL)) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if (pExynosComponent->currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }
    if (Exynos_OSAL_Strcmp(cParameterName, EXYNOS_INDEX_CONFIG_VIDEO_INTRAPERIOD) == 0) {
        *pIndexType = OMX_IndexConfigVideoIntraPeriod;
        ret = OMX_ErrorNone;
    } else if (Exynos_OSAL_Strcmp(cParameterName, EXYNOS_INDEX_PARAM_PREPEND_SPSPPS_TO_IDR) == 0) {
        *pIndexType = OMX_IndexParamPrependSPSPPSToIDR;
        ret = OMX_ErrorNone;
    } else {
        ret = Exynos_OMX_VideoEncodeGetExtensionIndex(hComponent, cParameterName, pIndexType);
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_H264Enc_ComponentRoleEnum(OMX_HANDLETYPE hComponent, OMX_U8 *cRole, OMX_U32 nIndex)
{
    OMX_ERRORTYPE               ret              = OMX_ErrorNone;
    OMX_COMPONENTTYPE          *pOMXComponent    = NULL;
    EXYNOS_OMX_BASECOMPONENT   *pExynosComponent = NULL;

    FunctionIn();

    if ((hComponent == NULL) || (cRole == NULL)) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if (nIndex == (MAX_COMPONENT_ROLE_NUM-1)) {
        Exynos_OSAL_Strcpy((char *)cRole, EXYNOS_OMX_COMPONENT_H264_ENC_ROLE);
        ret = OMX_ErrorNone;
    } else {
        ret = OMX_ErrorNoMore;
    }

EXIT:
    FunctionOut();

    return ret;
}

/* MFC Init */
OMX_ERRORTYPE Exynos_H264Enc_Init(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE             ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_OMX_BASEPORT      *pExynosInputPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT      *pExynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    EXYNOS_H264ENC_HANDLE    *pH264Enc = (EXYNOS_H264ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;;
    EXYNOS_MFC_H264ENC_HANDLE     *pMFCH264Handle = NULL;
    OMX_PTR                   hMFCHandle = pH264Enc->hMFCH264Handle.hMFCHandle;
    OMX_COLOR_FORMATTYPE      eColorFormat;

    ExynosVideoEncOps       *pEncOps    = NULL;
    ExynosVideoEncBufferOps *pInbufOps  = NULL;
    ExynosVideoEncBufferOps *pOutbufOps = NULL;

    int i = 0;

    FunctionIn();

    pH264Enc->hMFCH264Handle.bConfiguredMFCSrc = OMX_FALSE;
    pH264Enc->hMFCH264Handle.bConfiguredMFCDst = OMX_FALSE;
    pExynosComponent->bUseFlagEOF = OMX_TRUE;
    pExynosComponent->bSaveFlagEOS = OMX_FALSE;
    pExynosComponent->bBehaviorEOS = OMX_FALSE;

    eColorFormat = pExynosInputPort->portDefinition.format.video.eColorFormat;
    if (pExynosInputPort->bStoreMetaData == OMX_TRUE) {
        if (eColorFormat == OMX_COLOR_FormatAndroidOpaque) {
            pExynosInputPort->bufferProcessType = BUFFER_COPY;
        } else {
            pExynosInputPort->bufferProcessType = BUFFER_SHARE;
        }
    } else {
        pExynosInputPort->bufferProcessType = BUFFER_COPY;
    }

    /* H.264 Codec Open */
    ret = H264CodecOpen(pH264Enc);
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }
    pMFCH264Handle = &pH264Enc->hMFCH264Handle;

    pEncOps    = pH264Enc->hMFCH264Handle.pEncOps;
    pInbufOps  = pH264Enc->hMFCH264Handle.pInbufOps;
    pOutbufOps = pH264Enc->hMFCH264Handle.pOutbufOps;

    if ((pExynosOutputPort->bufferProcessType & BUFFER_COPY) == BUFFER_COPY) {
        Exynos_OSAL_SemaphoreCreate(&pExynosOutputPort->codecSemID);
        Exynos_OSAL_QueueCreate(&pExynosOutputPort->codecBufferQ, MAX_QUEUE_ELEMENTS);
    } else if (pExynosOutputPort->bufferProcessType == BUFFER_SHARE) {
        /*************/
        /*    TBD    */
        /*************/
        /* Does not require any actions. */
    }

    pH264Enc->bSourceStart = OMX_FALSE;
    Exynos_OSAL_SignalCreate(&pH264Enc->hSourceStartEvent);
    pH264Enc->bDestinationStart = OMX_FALSE;
    Exynos_OSAL_SignalCreate(&pH264Enc->hDestinationStartEvent);

    Exynos_OSAL_Memset(pExynosComponent->timeStamp, -19771003, sizeof(OMX_TICKS) * MAX_TIMESTAMP);
    Exynos_OSAL_Memset(pExynosComponent->nFlags, 0, sizeof(OMX_U32) * MAX_FLAGS);
    pH264Enc->hMFCH264Handle.indexTimestamp = 0;
    pH264Enc->hMFCH264Handle.outputIndexTimestamp = 0;

    pExynosComponent->getAllDelayBuffer = OMX_FALSE;

EXIT:
    FunctionOut();

    return ret;
}

/* MFC Terminate */
OMX_ERRORTYPE Exynos_H264Enc_Terminate(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE                  ret              = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc        = ((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle);
    EXYNOS_OMX_BASEPORT      *pExynosInputPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT      *pExynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    EXYNOS_H264ENC_HANDLE    *pH264Enc = (EXYNOS_H264ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
    OMX_PTR                hMFCHandle = pH264Enc->hMFCH264Handle.hMFCHandle;

    ExynosVideoEncOps       *pEncOps    = pH264Enc->hMFCH264Handle.pEncOps;
    ExynosVideoEncBufferOps *pInbufOps  = pH264Enc->hMFCH264Handle.pInbufOps;
    ExynosVideoEncBufferOps *pOutbufOps = pH264Enc->hMFCH264Handle.pOutbufOps;

    int i = 0, plane = 0;

    FunctionIn();

    Exynos_OSAL_SignalTerminate(pH264Enc->hDestinationStartEvent);
    pH264Enc->hDestinationStartEvent = NULL;
    pH264Enc->bDestinationStart = OMX_FALSE;
    Exynos_OSAL_SignalTerminate(pH264Enc->hSourceStartEvent);
    pH264Enc->hSourceStartEvent = NULL;
    pH264Enc->bSourceStart = OMX_FALSE;

    if ((pExynosOutputPort->bufferProcessType & BUFFER_COPY) == BUFFER_COPY) {
        Exynos_Free_CodecBuffers(pOMXComponent, OUTPUT_PORT_INDEX);
        Exynos_OSAL_QueueTerminate(&pExynosOutputPort->codecBufferQ);
        Exynos_OSAL_SemaphoreTerminate(pExynosOutputPort->codecSemID);
    } else if (pExynosOutputPort->bufferProcessType == BUFFER_SHARE) {
        /*************/
        /*    TBD    */
        /*************/
        /* Does not require any actions. */
    }

    if ((pExynosInputPort->bufferProcessType & BUFFER_COPY) == BUFFER_COPY) {
        Exynos_Free_CodecBuffers(pOMXComponent, INPUT_PORT_INDEX);
        Exynos_OSAL_QueueTerminate(&pExynosInputPort->codecBufferQ);
        Exynos_OSAL_SemaphoreTerminate(pExynosInputPort->codecSemID);
        pVideoEnc->bFirstInput  = OMX_TRUE;
    } else if (pExynosInputPort->bufferProcessType == BUFFER_SHARE) {
        /*************/
        /*    TBD    */
        /*************/
        /* Does not require any actions. */
    }
    H264CodecClose(pH264Enc);

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_H264Enc_SrcIn(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *pSrcInputData)
{
    OMX_ERRORTYPE               ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_H264ENC_HANDLE         *pH264Enc = (EXYNOS_H264ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
    void                          *hMFCHandle = pH264Enc->hMFCH264Handle.hMFCHandle;
    EXYNOS_OMX_BASEPORT *pExynosInputPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT *pExynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    OMX_U32  oneFrameSize = pSrcInputData->dataLen;
    ExynosVideoEncOps       *pEncOps    = pH264Enc->hMFCH264Handle.pEncOps;
    ExynosVideoEncBufferOps *pInbufOps  = pH264Enc->hMFCH264Handle.pInbufOps;
    ExynosVideoEncBufferOps *pOutbufOps = pH264Enc->hMFCH264Handle.pOutbufOps;
    ExynosVideoErrorType codecReturn = VIDEO_ERROR_NONE;
    int i;

    FunctionIn();

    if (pH264Enc->hMFCH264Handle.bConfiguredMFCSrc == OMX_FALSE) {
        ret = H264CodecSrcSetup(pOMXComponent, pSrcInputData);
        if ((pSrcInputData->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS)
            goto EXIT;
    }
    if (pH264Enc->hMFCH264Handle.bConfiguredMFCDst == OMX_FALSE) {
        ret = H264CodecDstSetup(pOMXComponent);
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }
    }

    if (pVideoEnc->configChange == OMX_TRUE) {
        Change_H264Enc_Param(pExynosComponent);
        pVideoEnc->configChange = OMX_FALSE;
    }
    if ((pSrcInputData->dataLen >= 0) ||
        ((pSrcInputData->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS)) {
        OMX_U32 nAllocLen[MFC_INPUT_BUFFER_PLANE] = {0, 0};
        OMX_U32 pMFCYUVDataSize[MFC_INPUT_BUFFER_PLANE]  = {NULL, NULL};
        ExynosVideoPlane planes[MFC_INPUT_BUFFER_PLANE];
        int plane;

        pExynosComponent->timeStamp[pH264Enc->hMFCH264Handle.indexTimestamp] = pSrcInputData->timeStamp;
        pExynosComponent->nFlags[pH264Enc->hMFCH264Handle.indexTimestamp] = pSrcInputData->nFlags;
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "input timestamp %lld us (%.2f secs), Tag: %d, nFlags: 0x%x", pSrcInputData->timeStamp, pSrcInputData->timeStamp / 1E6, pH264Enc->hMFCH264Handle.indexTimestamp, pSrcInputData->nFlags);
        pEncOps->Set_FrameTag(hMFCHandle, pH264Enc->hMFCH264Handle.indexTimestamp);
        pH264Enc->hMFCH264Handle.indexTimestamp++;
        pH264Enc->hMFCH264Handle.indexTimestamp %= MAX_TIMESTAMP;

        /* queue work for input buffer */
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "Exynos_H264Enc_SrcIn(): oneFrameSize: %d, bufferHeader: 0x%x", oneFrameSize, pSrcInputData->bufferHeader);
        pMFCYUVDataSize[0] = pExynosInputPort->portDefinition.format.video.nFrameWidth * pExynosInputPort->portDefinition.format.video.nFrameHeight;
        pMFCYUVDataSize[1] = pMFCYUVDataSize[0] / 2;

#ifdef USE_METADATABUFFERTYPE
        nAllocLen[0] = ALIGN_TO_16B(pExynosInputPort->portDefinition.format.video.nFrameWidth) *
                            ALIGN_TO_16B(pExynosInputPort->portDefinition.format.video.nFrameHeight);
        nAllocLen[1] = ALIGN(nAllocLen[0]/2,256);

        if ((pExynosInputPort->bStoreMetaData == OMX_TRUE) &&
            (pExynosInputPort->bufferProcessType == BUFFER_SHARE)) {
            codecReturn = pInbufOps->ExtensionEnqueue(hMFCHandle,
                                        (unsigned char **)pSrcInputData->buffer.multiPlaneBuffer.dataBuffer,
                                        (unsigned char **)pSrcInputData->buffer.multiPlaneBuffer.fd,
                                        (unsigned int *)nAllocLen, (unsigned int *)pMFCYUVDataSize,
                                        MFC_INPUT_BUFFER_PLANE, pSrcInputData->bufferHeader);
        } else {
            codecReturn = pInbufOps->Enqueue(hMFCHandle, (unsigned char **)pSrcInputData->buffer.multiPlaneBuffer.dataBuffer,
                                        (unsigned int *)pMFCYUVDataSize, MFC_INPUT_BUFFER_PLANE, pSrcInputData->bufferHeader);
        }
#else
        codecReturn = pInbufOps->Enqueue(hMFCHandle, (unsigned char **)pSrcInputData->buffer.multiPlaneBuffer.dataBuffer,
                                    (unsigned int *)pMFCYUVDataSize, MFC_INPUT_BUFFER_PLANE, pSrcInputData->bufferHeader);
#endif
        if (codecReturn != VIDEO_ERROR_NONE) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: %d: Failed - pInbufOps->Enqueue", __FUNCTION__, __LINE__);
            ret = (OMX_ERRORTYPE)OMX_ErrorCodecEncode;
            goto EXIT;
        }
        H264CodecStart(pOMXComponent, INPUT_PORT_INDEX);
        if (pH264Enc->bSourceStart == OMX_FALSE) {
            pH264Enc->bSourceStart = OMX_TRUE;
            Exynos_OSAL_SignalSet(pH264Enc->hSourceStartEvent);
            Exynos_OSAL_SleepMillisec(0);
        }
        if (pH264Enc->bDestinationStart == OMX_FALSE) {
            pH264Enc->bDestinationStart = OMX_TRUE;
            Exynos_OSAL_SignalSet(pH264Enc->hDestinationStartEvent);
            Exynos_OSAL_SleepMillisec(0);
        }
    }

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_H264Enc_SrcOut(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *pSrcOutputData)
{
    OMX_ERRORTYPE                  ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_H264ENC_HANDLE         *pH264Enc = (EXYNOS_H264ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
    void                          *hMFCHandle = pH264Enc->hMFCH264Handle.hMFCHandle;
    EXYNOS_OMX_BASEPORT     *pExynosInputPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    ExynosVideoEncOps       *pEncOps    = pH264Enc->hMFCH264Handle.pEncOps;
    ExynosVideoEncBufferOps *pInbufOps  = pH264Enc->hMFCH264Handle.pInbufOps;
    ExynosVideoBuffer       *pVideoBuffer;
    ExynosVideoBuffer        videoBuffer;

    FunctionIn();

    if ((pExynosInputPort->bStoreMetaData == OMX_TRUE) &&
        (pExynosInputPort->bufferProcessType == BUFFER_SHARE)) {
        if (pInbufOps->ExtensionDequeue(hMFCHandle, &videoBuffer) == VIDEO_ERROR_NONE)
            pVideoBuffer = &videoBuffer;
        else
            pVideoBuffer = NULL;
    } else {
        pVideoBuffer = pInbufOps->Dequeue(hMFCHandle);
    }

    pSrcOutputData->dataLen       = 0;
    pSrcOutputData->usedDataLen   = 0;
    pSrcOutputData->remainDataLen = 0;
    pSrcOutputData->nFlags    = 0;
    pSrcOutputData->timeStamp = 0;

    if (pVideoBuffer == NULL) {
        pSrcOutputData->buffer.singlePlaneBuffer.dataBuffer = NULL;
        pSrcOutputData->allocSize  = 0;
        pSrcOutputData->pPrivate = NULL;
        pSrcOutputData->bufferHeader = NULL;
    } else {
        int plane = 0;
        for (plane = 0; plane < MFC_INPUT_BUFFER_PLANE; plane++) {
            pSrcOutputData->buffer.multiPlaneBuffer.dataBuffer[plane] = pVideoBuffer->planes[plane].addr;
            pSrcOutputData->buffer.multiPlaneBuffer.fd[plane] = pVideoBuffer->planes[plane].fd;
        }
        pSrcOutputData->allocSize = pVideoBuffer->planes[0].allocSize +
                                        pVideoBuffer->planes[1].allocSize +
                                        pVideoBuffer->planes[2].allocSize;

        if ((pExynosInputPort->bufferProcessType & BUFFER_COPY) == BUFFER_COPY) {
            int i = 0;
            while (pSrcOutputData->buffer.multiPlaneBuffer.dataBuffer[0] != pVideoEnc->pMFCEncInputBuffer[i]->pVirAddr[0]) {
                if (i >= MFC_INPUT_BUFFER_NUM_MAX) {
                    Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: %d: Failed - Lost buffer", __FUNCTION__, __LINE__);
                    ret = (OMX_ERRORTYPE)OMX_ErrorCodecEncode;
                    goto EXIT;
                }
                i++;
            }
            pVideoEnc->pMFCEncInputBuffer[i]->dataSize = 0;
            pSrcOutputData->pPrivate = pVideoEnc->pMFCEncInputBuffer[i];
        }

        /* For Share Buffer */
        pSrcOutputData->bufferHeader = (OMX_BUFFERHEADERTYPE*)pVideoBuffer->pPrivate;
    }

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_H264Enc_DstIn(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *pDstInputData)
{
    OMX_ERRORTYPE                  ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_H264ENC_HANDLE         *pH264Enc = (EXYNOS_H264ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
    void                          *hMFCHandle = pH264Enc->hMFCH264Handle.hMFCHandle;
    ExynosVideoEncOps       *pEncOps    = pH264Enc->hMFCH264Handle.pEncOps;
    ExynosVideoEncBufferOps *pOutbufOps = pH264Enc->hMFCH264Handle.pOutbufOps;
    OMX_U32 dataLen = 0;
    ExynosVideoErrorType codecReturn = VIDEO_ERROR_NONE;

    FunctionIn();

    if (pDstInputData->buffer.singlePlaneBuffer.dataBuffer == NULL) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to find input buffer");
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    codecReturn = pOutbufOps->Enqueue(hMFCHandle, (unsigned char **)&pDstInputData->buffer.singlePlaneBuffer.dataBuffer,
                     (unsigned int *)&dataLen, MFC_OUTPUT_BUFFER_PLANE, pDstInputData->bufferHeader);

    if (codecReturn != VIDEO_ERROR_NONE) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: %d: Failed - pOutbufOps->Enqueue", __FUNCTION__, __LINE__);
        ret = (OMX_ERRORTYPE)OMX_ErrorCodecEncode;
        goto EXIT;
    }
    H264CodecStart(pOMXComponent, OUTPUT_PORT_INDEX);

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_H264Enc_DstOut(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *pDstOutputData)
{
    OMX_ERRORTYPE                  ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_H264ENC_HANDLE         *pH264Enc = (EXYNOS_H264ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
    void                          *hMFCHandle = pH264Enc->hMFCH264Handle.hMFCHandle;
    ExynosVideoEncOps       *pEncOps    = pH264Enc->hMFCH264Handle.pEncOps;
    ExynosVideoEncBufferOps *pOutbufOps = pH264Enc->hMFCH264Handle.pOutbufOps;
    ExynosVideoBuffer       *pVideoBuffer;
    ExynosVideoFrameStatusType displayStatus = VIDEO_FRAME_STATUS_UNKNOWN;
    ExynosVideoGeometry bufferGeometry;
    OMX_S32 indexTimestamp = 0;

    FunctionIn();

    if (pH264Enc->bDestinationStart == OMX_FALSE) {
        ret = OMX_ErrorNone;
        goto EXIT;
    }

    if ((pVideoBuffer = pOutbufOps->Dequeue(hMFCHandle)) == NULL) {
        ret = OMX_ErrorNone;
        goto EXIT;
    }

    pH264Enc->hMFCH264Handle.outputIndexTimestamp++;
    pH264Enc->hMFCH264Handle.outputIndexTimestamp %= MAX_TIMESTAMP;

    pDstOutputData->buffer.singlePlaneBuffer.dataBuffer = pVideoBuffer->planes[0].addr;
    pDstOutputData->buffer.singlePlaneBuffer.fd = pVideoBuffer->planes[0].fd;
    pDstOutputData->allocSize   = pVideoBuffer->planes[0].allocSize;
    pDstOutputData->dataLen     = pVideoBuffer->planes[0].dataSize;
    pDstOutputData->remainDataLen = pVideoBuffer->planes[0].dataSize;
    pDstOutputData->usedDataLen = 0;
    pDstOutputData->pPrivate = pVideoBuffer;
    /* For Share Buffer */
    pDstOutputData->bufferHeader = (OMX_BUFFERHEADERTYPE *)pVideoBuffer->pPrivate;

    if (pVideoEnc->bFirstOutput == OMX_FALSE) {
        OMX_U8 *p = NULL;
        int iSpsSize = 0;
        int iPpsSize = 0;

        /* Calculate sps/pps size if needed */
        p = FindDelimiter((OMX_U8 *)(pDstOutputData->buffer.singlePlaneBuffer.dataBuffer + 4),
                            pDstOutputData->dataLen - 4);

        iSpsSize = (unsigned int)p - (unsigned int)pDstOutputData->buffer.singlePlaneBuffer.dataBuffer;
        pH264Enc->hMFCH264Handle.headerData.pHeaderSPS =
            (OMX_PTR)pDstOutputData->buffer.singlePlaneBuffer.dataBuffer;
        pH264Enc->hMFCH264Handle.headerData.SPSLen = iSpsSize;

        iPpsSize = pDstOutputData->dataLen - iSpsSize;
        pH264Enc->hMFCH264Handle.headerData.pHeaderPPS =
            (OMX_U8 *)pDstOutputData->buffer.singlePlaneBuffer.dataBuffer + iSpsSize;
        pH264Enc->hMFCH264Handle.headerData.PPSLen = iPpsSize;

        pDstOutputData->timeStamp = 0;
        pDstOutputData->nFlags |= OMX_BUFFERFLAG_CODECCONFIG;
        pDstOutputData->nFlags |= OMX_BUFFERFLAG_ENDOFFRAME;
        pVideoEnc->bFirstOutput = OMX_TRUE;
    } else {
        indexTimestamp = pEncOps->Get_FrameTag(pH264Enc->hMFCH264Handle.hMFCHandle);
        if ((indexTimestamp < 0) || (indexTimestamp >= MAX_TIMESTAMP)) {
            pDstOutputData->timeStamp = pExynosComponent->timeStamp[pH264Enc->hMFCH264Handle.outputIndexTimestamp];
            pDstOutputData->nFlags = pExynosComponent->nFlags[pH264Enc->hMFCH264Handle.outputIndexTimestamp];
        } else {
            pDstOutputData->timeStamp = pExynosComponent->timeStamp[indexTimestamp];
            pDstOutputData->nFlags = pExynosComponent->nFlags[indexTimestamp];
        }

        pDstOutputData->nFlags |= OMX_BUFFERFLAG_ENDOFFRAME;
        if (pVideoBuffer->frameType == VIDEO_FRAME_I)
            pDstOutputData->nFlags |= OMX_BUFFERFLAG_SYNCFRAME;
    }

    if ((displayStatus == VIDEO_FRAME_STATUS_CHANGE_RESOL) ||
        (((pDstOutputData->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS) &&
         (pExynosComponent->bBehaviorEOS == OMX_FALSE))) {
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "%x displayStatus:%d, nFlags0x%x", pExynosComponent, displayStatus, pDstOutputData->nFlags);
        pDstOutputData->remainDataLen = 0;
    }
    if (((pDstOutputData->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS) &&
        (pExynosComponent->bBehaviorEOS == OMX_TRUE))
        pExynosComponent->bBehaviorEOS = OMX_FALSE;

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_H264Enc_srcInputBufferProcess(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *pSrcInputData)
{
    OMX_ERRORTYPE             ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_H264ENC_HANDLE    *pH264Enc = (EXYNOS_H264ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
    EXYNOS_OMX_BASEPORT      *pExynosInputPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];

    FunctionIn();

    if ((!CHECK_PORT_ENABLED(pExynosInputPort)) || (!CHECK_PORT_POPULATED(pExynosInputPort))) {
        ret = OMX_ErrorNone;
        goto EXIT;
    }
    if (OMX_FALSE == Exynos_Check_BufferProcess_State(pExynosComponent, INPUT_PORT_INDEX)) {
        ret = OMX_ErrorNone;
        goto EXIT;
    }

    ret = Exynos_H264Enc_SrcIn(pOMXComponent, pSrcInputData);
    if (ret != OMX_ErrorNone) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: %d: Failed - SrcIn -> event is thrown to client", __FUNCTION__, __LINE__);
        pExynosComponent->pCallbacks->EventHandler((OMX_HANDLETYPE)pOMXComponent,
                                                pExynosComponent->callbackData,
                                                OMX_EventError, ret, 0, NULL);
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_H264Enc_srcOutputBufferProcess(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *pSrcOutputData)
{
    OMX_ERRORTYPE             ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_H264ENC_HANDLE    *pH264Enc = (EXYNOS_H264ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
    EXYNOS_OMX_BASEPORT      *pExynosInputPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];

    FunctionIn();

    if ((!CHECK_PORT_ENABLED(pExynosInputPort)) || (!CHECK_PORT_POPULATED(pExynosInputPort))) {
        ret = OMX_ErrorNone;
        goto EXIT;
    }

    if ((pExynosInputPort->bufferProcessType & BUFFER_COPY) == BUFFER_COPY) {
        if (OMX_FALSE == Exynos_Check_BufferProcess_State(pExynosComponent, INPUT_PORT_INDEX)) {
            ret = OMX_ErrorNone;
            goto EXIT;
        }
    }
    if ((pH264Enc->bSourceStart == OMX_FALSE) &&
       (!CHECK_PORT_BEING_FLUSHED(pExynosInputPort))) {
        Exynos_OSAL_SignalWait(pH264Enc->hSourceStartEvent, DEF_MAX_WAIT_TIME);
        Exynos_OSAL_SignalReset(pH264Enc->hSourceStartEvent);
    }

    ret = Exynos_H264Enc_SrcOut(pOMXComponent, pSrcOutputData);
    if ((ret != OMX_ErrorNone) && (pExynosComponent->currentState == OMX_StateExecuting)) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: %d: Failed - SrcOut -> event is thrown to client", __FUNCTION__, __LINE__);
        pExynosComponent->pCallbacks->EventHandler((OMX_HANDLETYPE)pOMXComponent,
                                                pExynosComponent->callbackData,
                                                OMX_EventError, ret, 0, NULL);
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_H264Enc_dstInputBufferProcess(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *pDstInputData)
{
    OMX_ERRORTYPE             ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_H264ENC_HANDLE    *pH264Enc = (EXYNOS_H264ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
    EXYNOS_OMX_BASEPORT      *pExynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];

    FunctionIn();

    if ((!CHECK_PORT_ENABLED(pExynosOutputPort)) || (!CHECK_PORT_POPULATED(pExynosOutputPort))) {
        ret = OMX_ErrorNone;
        goto EXIT;
    }
    if (OMX_FALSE == Exynos_Check_BufferProcess_State(pExynosComponent, OUTPUT_PORT_INDEX)) {
        ret = OMX_ErrorNone;
        goto EXIT;
    }
    if (pExynosOutputPort->bufferProcessType == BUFFER_SHARE) {
        if ((pH264Enc->bDestinationStart == OMX_FALSE) &&
           (!CHECK_PORT_BEING_FLUSHED(pExynosOutputPort))) {
            Exynos_OSAL_SignalWait(pH264Enc->hDestinationStartEvent, DEF_MAX_WAIT_TIME);
            Exynos_OSAL_SignalReset(pH264Enc->hDestinationStartEvent);
        }
    }
    if (pH264Enc->hMFCH264Handle.bConfiguredMFCDst == OMX_TRUE) {
        ret = Exynos_H264Enc_DstIn(pOMXComponent, pDstInputData);
        if (ret != OMX_ErrorNone) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: %d: Failed - DstIn -> event is thrown to client", __FUNCTION__, __LINE__);
            pExynosComponent->pCallbacks->EventHandler((OMX_HANDLETYPE)pOMXComponent,
                                                pExynosComponent->callbackData,
                                                OMX_EventError, ret, 0, NULL);
        }
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_H264Enc_dstOutputBufferProcess(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *pDstOutputData)
{
    OMX_ERRORTYPE             ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_H264ENC_HANDLE    *pH264Enc = (EXYNOS_H264ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
    EXYNOS_OMX_BASEPORT      *pExynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];

    FunctionIn();

    if ((!CHECK_PORT_ENABLED(pExynosOutputPort)) || (!CHECK_PORT_POPULATED(pExynosOutputPort))) {
        ret = OMX_ErrorNone;
        goto EXIT;
    }
    if (OMX_FALSE == Exynos_Check_BufferProcess_State(pExynosComponent, OUTPUT_PORT_INDEX)) {
        ret = OMX_ErrorNone;
        goto EXIT;
    }

    if ((pExynosOutputPort->bufferProcessType & BUFFER_COPY) == BUFFER_COPY) {
        if ((pH264Enc->bDestinationStart == OMX_FALSE) &&
           (!CHECK_PORT_BEING_FLUSHED(pExynosOutputPort))) {
            Exynos_OSAL_SignalWait(pH264Enc->hDestinationStartEvent, DEF_MAX_WAIT_TIME);
            Exynos_OSAL_SignalReset(pH264Enc->hDestinationStartEvent);
        }
    }
    ret = Exynos_H264Enc_DstOut(pOMXComponent, pDstOutputData);
    if ((ret != OMX_ErrorNone) && (pExynosComponent->currentState == OMX_StateExecuting)) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: %d: Failed - DstOut -> event is thrown to client", __FUNCTION__, __LINE__);
        pExynosComponent->pCallbacks->EventHandler((OMX_HANDLETYPE)pOMXComponent,
                                                pExynosComponent->callbackData,
                                                OMX_EventError, ret, 0, NULL);
    }

EXIT:
    FunctionOut();

    return ret;
}

OSCL_EXPORT_REF OMX_ERRORTYPE Exynos_OMX_ComponentInit(OMX_HANDLETYPE hComponent, OMX_STRING componentName)
{
    OMX_ERRORTYPE                  ret              = OMX_ErrorNone;
    OMX_COMPONENTTYPE             *pOMXComponent    = NULL;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent = NULL;
    EXYNOS_OMX_BASEPORT           *pExynosPort      = NULL;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc        = NULL;
    EXYNOS_H264ENC_HANDLE         *pH264Enc         = NULL;
    int i = 0;

    FunctionIn();

    if ((hComponent == NULL) || (componentName == NULL)) {
        ret = OMX_ErrorBadParameter;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorBadParameter, Line:%d", __LINE__);
        goto EXIT;
    }
    if (Exynos_OSAL_Strcmp(EXYNOS_OMX_COMPONENT_H264_ENC, componentName) != 0) {
        ret = OMX_ErrorBadParameter;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorBadParameter, componentName:%s, Line:%d", componentName, __LINE__);
        goto EXIT;
    }

    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = Exynos_OMX_VideoEncodeComponentInit(pOMXComponent);
    if (ret != OMX_ErrorNone) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_Error, Line:%d", __LINE__);
        goto EXIT;
    }
    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    pExynosComponent->codecType = HW_VIDEO_ENC_CODEC;

    pExynosComponent->componentName = (OMX_STRING)Exynos_OSAL_Malloc(MAX_OMX_COMPONENT_NAME_SIZE);
    if (pExynosComponent->componentName == NULL) {
        Exynos_OMX_VideoEncodeComponentDeinit(pOMXComponent);
        ret = OMX_ErrorInsufficientResources;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
        goto EXIT;
    }
    Exynos_OSAL_Memset(pExynosComponent->componentName, 0, MAX_OMX_COMPONENT_NAME_SIZE);

    pH264Enc = Exynos_OSAL_Malloc(sizeof(EXYNOS_H264ENC_HANDLE));
    if (pH264Enc == NULL) {
        Exynos_OMX_VideoEncodeComponentDeinit(pOMXComponent);
        ret = OMX_ErrorInsufficientResources;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
        goto EXIT;
    }
    Exynos_OSAL_Memset(pH264Enc, 0, sizeof(EXYNOS_H264ENC_HANDLE));
    pVideoEnc = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    pVideoEnc->hCodecHandle = (OMX_HANDLETYPE)pH264Enc;
    pVideoEnc->quantization.nQpI = 20;
    pVideoEnc->quantization.nQpP = 20;
    pVideoEnc->quantization.nQpB = 20;

    Exynos_OSAL_Strcpy(pExynosComponent->componentName, EXYNOS_OMX_COMPONENT_H264_ENC);
    /* Set componentVersion */
    pExynosComponent->componentVersion.s.nVersionMajor = VERSIONMAJOR_NUMBER;
    pExynosComponent->componentVersion.s.nVersionMinor = VERSIONMINOR_NUMBER;
    pExynosComponent->componentVersion.s.nRevision     = REVISION_NUMBER;
    pExynosComponent->componentVersion.s.nStep         = STEP_NUMBER;
    /* Set specVersion */
    pExynosComponent->specVersion.s.nVersionMajor = VERSIONMAJOR_NUMBER;
    pExynosComponent->specVersion.s.nVersionMinor = VERSIONMINOR_NUMBER;
    pExynosComponent->specVersion.s.nRevision     = REVISION_NUMBER;
    pExynosComponent->specVersion.s.nStep         = STEP_NUMBER;

    /* Input port */
    pExynosPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    pExynosPort->portDefinition.format.video.nFrameWidth = DEFAULT_FRAME_WIDTH;
    pExynosPort->portDefinition.format.video.nFrameHeight= DEFAULT_FRAME_HEIGHT;
    pExynosPort->portDefinition.format.video.nStride = 0; /*DEFAULT_FRAME_WIDTH;*/
    pExynosPort->portDefinition.nBufferSize = DEFAULT_VIDEO_INPUT_BUFFER_SIZE;
    pExynosPort->portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
    Exynos_OSAL_Memset(pExynosPort->portDefinition.format.video.cMIMEType, 0, MAX_OMX_MIMETYPE_SIZE);
    Exynos_OSAL_Strcpy(pExynosPort->portDefinition.format.video.cMIMEType, "raw/video");
    pExynosPort->portDefinition.format.video.eColorFormat = OMX_COLOR_FormatYUV420SemiPlanar;
    pExynosPort->portDefinition.bEnabled = OMX_TRUE;
    pExynosPort->bufferProcessType = BUFFER_COPY;
    pExynosPort->portWayType = WAY2_PORT;

    /* Output port */
    pExynosPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    pExynosPort->portDefinition.format.video.nFrameWidth = DEFAULT_FRAME_WIDTH;
    pExynosPort->portDefinition.format.video.nFrameHeight= DEFAULT_FRAME_HEIGHT;
    pExynosPort->portDefinition.format.video.nStride = 0; /*DEFAULT_FRAME_WIDTH;*/
    pExynosPort->portDefinition.nBufferSize = DEFAULT_VIDEO_OUTPUT_BUFFER_SIZE;
    pExynosPort->portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingAVC;
    Exynos_OSAL_Memset(pExynosPort->portDefinition.format.video.cMIMEType, 0, MAX_OMX_MIMETYPE_SIZE);
    Exynos_OSAL_Strcpy(pExynosPort->portDefinition.format.video.cMIMEType, "video/avc");
    pExynosPort->portDefinition.format.video.eColorFormat = OMX_COLOR_FormatUnused;
    pExynosPort->portDefinition.bEnabled = OMX_TRUE;
    pExynosPort->bufferProcessType = BUFFER_SHARE;
    pExynosPort->portWayType = WAY2_PORT;

    for(i = 0; i < ALL_PORT_NUM; i++) {
        INIT_SET_SIZE_VERSION(&pH264Enc->AVCComponent[i], OMX_VIDEO_PARAM_AVCTYPE);
        pH264Enc->AVCComponent[i].nPortIndex = i;
        pH264Enc->AVCComponent[i].eProfile   = OMX_VIDEO_AVCProfileBaseline;
        pH264Enc->AVCComponent[i].eLevel     = OMX_VIDEO_AVCLevel31;

        pH264Enc->AVCComponent[i].nPFrames = 20;
    }

    pOMXComponent->GetParameter      = &Exynos_H264Enc_GetParameter;
    pOMXComponent->SetParameter      = &Exynos_H264Enc_SetParameter;
    pOMXComponent->GetConfig         = &Exynos_H264Enc_GetConfig;
    pOMXComponent->SetConfig         = &Exynos_H264Enc_SetConfig;
    pOMXComponent->GetExtensionIndex = &Exynos_H264Enc_GetExtensionIndex;
    pOMXComponent->ComponentRoleEnum = &Exynos_H264Enc_ComponentRoleEnum;
    pOMXComponent->ComponentDeInit   = &Exynos_OMX_ComponentDeinit;

    pExynosComponent->exynos_codec_componentInit      = &Exynos_H264Enc_Init;
    pExynosComponent->exynos_codec_componentTerminate = &Exynos_H264Enc_Terminate;

    pVideoEnc->exynos_codec_srcInputProcess  = &Exynos_H264Enc_srcInputBufferProcess;
    pVideoEnc->exynos_codec_srcOutputProcess = &Exynos_H264Enc_srcOutputBufferProcess;
    pVideoEnc->exynos_codec_dstInputProcess  = &Exynos_H264Enc_dstInputBufferProcess;
    pVideoEnc->exynos_codec_dstOutputProcess = &Exynos_H264Enc_dstOutputBufferProcess;

    pVideoEnc->exynos_codec_start         = &H264CodecStart;
    pVideoEnc->exynos_codec_stop          = &H264CodecStop;
    pVideoEnc->exynos_codec_bufferProcessRun = &H264CodecOutputBufferProcessRun;
    pVideoEnc->exynos_codec_enqueueAllBuffer = &H264CodecEnQueueAllBuffer;

    pVideoEnc->exynos_checkInputFrame        = NULL;
    pVideoEnc->exynos_codec_getCodecInputPrivateData  = &GetCodecInputPrivateData;
    pVideoEnc->exynos_codec_getCodecOutputPrivateData = &GetCodecOutputPrivateData;

    pVideoEnc->hSharedMemory = Exynos_OSAL_SharedMemory_Open();
    if (pVideoEnc->hSharedMemory == NULL) {
        Exynos_OSAL_Free(pH264Enc);
        pH264Enc = ((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle = NULL;
        Exynos_OMX_VideoEncodeComponentDeinit(pOMXComponent);
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    pExynosComponent->currentState = OMX_StateLoaded;

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_ComponentDeinit(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE            ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE          *pOMXComponent = NULL;
    EXYNOS_OMX_BASECOMPONENT   *pExynosComponent = NULL;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc = NULL;
    EXYNOS_H264ENC_HANDLE      *pH264Enc         = NULL;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    pVideoEnc = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;

    Exynos_OSAL_SharedMemory_Close(pVideoEnc->hSharedMemory);

    Exynos_OSAL_Free(pExynosComponent->componentName);
    pExynosComponent->componentName = NULL;

    pH264Enc = (EXYNOS_H264ENC_HANDLE *)pVideoEnc->hCodecHandle;
    if (pH264Enc != NULL) {
        Exynos_OSAL_Free(pH264Enc);
        pH264Enc = pVideoEnc->hCodecHandle = NULL;
    }

    ret = Exynos_OMX_VideoEncodeComponentDeinit(pOMXComponent);
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}
