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
 * @file        Exynos_OMX_Mpeg4enc.c
 * @brief
 * @author      Yunji Kim (yunji.kim@samsung.com)
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
#include "Exynos_OMX_Mpeg4enc.h"
#include "ExynosVideoApi.h"
#include "Exynos_OSAL_SharedMemory.h"
#include "Exynos_OSAL_Event.h"

/* To use CSC_METHOD_HW in EXYNOS OMX, gralloc should allocate physical memory using FIMC */
/* It means GRALLOC_USAGE_HW_FIMC1 should be set on Native Window usage */
#include "csc.h"

#undef  EXYNOS_LOG_TAG
#define EXYNOS_LOG_TAG    "EXYNOS_MPEG4_ENC"
#define EXYNOS_LOG_OFF
//#define EXYNOS_TRACE_ON
#include "Exynos_OSAL_Log.h"

/* MPEG4 Encoder Supported Levels & profiles */
EXYNOS_OMX_VIDEO_PROFILELEVEL supportedMPEG4ProfileLevels[] ={
    {OMX_VIDEO_MPEG4ProfileSimple, OMX_VIDEO_MPEG4Level0},
    {OMX_VIDEO_MPEG4ProfileSimple, OMX_VIDEO_MPEG4Level0b},
    {OMX_VIDEO_MPEG4ProfileSimple, OMX_VIDEO_MPEG4Level1},
    {OMX_VIDEO_MPEG4ProfileSimple, OMX_VIDEO_MPEG4Level2},
    {OMX_VIDEO_MPEG4ProfileSimple, OMX_VIDEO_MPEG4Level3},
    {OMX_VIDEO_MPEG4ProfileSimple, OMX_VIDEO_MPEG4Level4},
    {OMX_VIDEO_MPEG4ProfileSimple, OMX_VIDEO_MPEG4Level4a},
    {OMX_VIDEO_MPEG4ProfileSimple, OMX_VIDEO_MPEG4Level5},
    {OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level0},
    {OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level0b},
    {OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level1},
    {OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level2},
    {OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level3},
    {OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level4},
    {OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level4a},
    {OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level5}};

/* H.263 Encoder Supported Levels & profiles */
EXYNOS_OMX_VIDEO_PROFILELEVEL supportedH263ProfileLevels[] = {
    {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level10},
    {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level20},
    {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level30},
    {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level40},
    {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level45},
    {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level50},
    {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level60},
    {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level70}};

static OMX_U32 OMXMpeg4ProfileToMFCProfile(OMX_VIDEO_MPEG4PROFILETYPE profile)
{
    OMX_U32 ret;

    switch (profile) {
    case OMX_VIDEO_MPEG4ProfileSimple:
        ret = 0;
        break;
    case OMX_VIDEO_MPEG4ProfileAdvancedSimple:
        ret = 1;
        break;
    default:
        ret = 0;
    };

    return ret;
}

static OMX_U32 OMXMpeg4LevelToMFCLevel(OMX_VIDEO_MPEG4LEVELTYPE level)
{
    OMX_U32 ret;

    switch (level) {
    case OMX_VIDEO_MPEG4Level0:
        ret = 0;
        break;
    case OMX_VIDEO_MPEG4Level0b:
        ret = 1;
        break;
    case OMX_VIDEO_MPEG4Level1:
        ret = 2;
        break;
    case OMX_VIDEO_MPEG4Level2:
        ret = 3;
        break;
    case OMX_VIDEO_MPEG4Level3:
        ret = 4;
        break;
    case OMX_VIDEO_MPEG4Level4:
    case OMX_VIDEO_MPEG4Level4a:
        ret = 6;
        break;
    case OMX_VIDEO_MPEG4Level5:
        ret = 7;
        break;
    default:
        ret = 0;
    };

    return ret;
}

static void Print_Mpeg4Enc_Param(ExynosVideoEncParam *pEncParam)
{
    ExynosVideoEncCommonParam *pCommonParam = &pEncParam->commonParam;
    ExynosVideoEncMpeg4Param  *pMpeg4Param  = &pEncParam->codecParam.mpeg4;

    /* common parameters */
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "SourceWidth             : %d", pCommonParam->SourceWidth);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "SourceHeight            : %d", pCommonParam->SourceHeight);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "IDRPeriod               : %d", pCommonParam->IDRPeriod);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "SliceMode               : %d", pCommonParam->SliceMode);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "RandomIntraMBRefresh    : %d", pCommonParam->RandomIntraMBRefresh);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "Bitrate                 : %d", pCommonParam->Bitrate);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "FrameQp                 : %d", pCommonParam->FrameQp);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "FrameQp_P               : %d", pCommonParam->FrameQp_P);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "QSCodeMax               : %d", pCommonParam->QSCodeMax);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "QSCodeMin               : %d", pCommonParam->QSCodeMin);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "PadControlOn            : %d", pCommonParam->PadControlOn);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "LumaPadVal              : %d", pCommonParam->LumaPadVal);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "CbPadVal                : %d", pCommonParam->CbPadVal);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "CrPadVal                : %d", pCommonParam->CrPadVal);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "FrameMap                : %d", pCommonParam->FrameMap);

    /* Mpeg4 specific parameters */
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "ProfileIDC              : %d", pMpeg4Param->ProfileIDC);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "LevelIDC                : %d", pMpeg4Param->LevelIDC);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "FrameQp_B               : %d", pMpeg4Param->FrameQp_B);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "TimeIncreamentRes       : %d", pMpeg4Param->TimeIncreamentRes);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "VopTimeIncreament       : %d", pMpeg4Param->VopTimeIncreament);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "SliceArgument           : %d", pMpeg4Param->SliceArgument);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "NumberBFrames           : %d", pMpeg4Param->NumberBFrames);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "DisableQpelME           : %d", pMpeg4Param->DisableQpelME);

    /* rate control related parameters */
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "EnableFRMRateControl    : %d", pCommonParam->EnableFRMRateControl);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "EnableMBRateControl     : %d", pCommonParam->EnableMBRateControl);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "CBRPeriodRf             : %d", pCommonParam->CBRPeriodRf);
}

static void Print_H263Enc_Param(ExynosVideoEncParam *pEncParam)
{
    ExynosVideoEncCommonParam *pCommonParam = &pEncParam->commonParam;
    ExynosVideoEncH263Param   *pH263Param   = &pEncParam->codecParam.h263;

    /* common parameters */
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "SourceWidth             : %d", pCommonParam->SourceWidth);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "SourceHeight            : %d", pCommonParam->SourceHeight);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "IDRPeriod               : %d", pCommonParam->IDRPeriod);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "SliceMode               : %d", pCommonParam->SliceMode);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "RandomIntraMBRefresh    : %d", pCommonParam->RandomIntraMBRefresh);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "Bitrate                 : %d", pCommonParam->Bitrate);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "FrameQp                 : %d", pCommonParam->FrameQp);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "FrameQp_P               : %d", pCommonParam->FrameQp_P);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "QSCodeMax               : %d", pCommonParam->QSCodeMax);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "QSCodeMin               : %d", pCommonParam->QSCodeMin);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "PadControlOn            : %d", pCommonParam->PadControlOn);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "LumaPadVal              : %d", pCommonParam->LumaPadVal);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "CbPadVal                : %d", pCommonParam->CbPadVal);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "CrPadVal                : %d", pCommonParam->CrPadVal);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "FrameMap                : %d", pCommonParam->FrameMap);

    /* H263 specific parameters */
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "FrameRate               : %d", pH263Param->FrameRate);

    /* rate control related parameters */
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "EnableFRMRateControl    : %d", pCommonParam->EnableFRMRateControl);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "EnableMBRateControl     : %d", pCommonParam->EnableMBRateControl);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "CBRPeriodRf             : %d", pCommonParam->CBRPeriodRf);
}

static void Set_Mpeg4Enc_Param(EXYNOS_OMX_BASECOMPONENT *pExynosComponent)
{
    EXYNOS_OMX_BASEPORT           *pExynosInputPort  = NULL;
    EXYNOS_OMX_BASEPORT           *pExynosOutputPort = NULL;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc         = NULL;
    EXYNOS_MPEG4ENC_HANDLE        *pMpeg4Enc         = NULL;
    EXYNOS_MFC_MPEG4ENC_HANDLE     *pMFCMpeg4Handle;

    ExynosVideoEncParam       *pEncParam    = NULL;
    ExynosVideoEncCommonParam *pCommonParam = NULL;
    ExynosVideoEncMpeg4Param  *pMpeg4Param  = NULL;

    pVideoEnc = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    pMpeg4Enc = pVideoEnc->hCodecHandle;
    pMFCMpeg4Handle    = &pMpeg4Enc->hMFCMpeg4Handle;
    pExynosInputPort  = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    pExynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];

    pEncParam    = &pMFCMpeg4Handle->encParam;
    pCommonParam = &pEncParam->commonParam;
    pMpeg4Param  = &pEncParam->codecParam.mpeg4;
    pEncParam->eCompressionFormat = VIDEO_CODING_MPEG4;
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "eCompressionFormat: %d", pEncParam->eCompressionFormat);

    /* common parameters */
    pCommonParam->SourceWidth  = pExynosOutputPort->portDefinition.format.video.nFrameWidth;
    pCommonParam->SourceHeight = pExynosOutputPort->portDefinition.format.video.nFrameHeight;
    pCommonParam->IDRPeriod    = pMpeg4Enc->mpeg4Component[OUTPUT_PORT_INDEX].nPFrames + 1;
    pCommonParam->SliceMode    = 0;
    pCommonParam->Bitrate      = pExynosOutputPort->portDefinition.format.video.nBitrate;
    pCommonParam->FrameQp      = pVideoEnc->quantization.nQpI;
    pCommonParam->FrameQp_P    = pVideoEnc->quantization.nQpP;
    pCommonParam->QSCodeMax    = 30;
    pCommonParam->QSCodeMin    = 10;
    pCommonParam->PadControlOn = 0; /* 0: Use boundary pixel, 1: Use the below setting value */
    pCommonParam->LumaPadVal   = 0;
    pCommonParam->CbPadVal     = 0;
    pCommonParam->CrPadVal     = 0;

    if (pVideoEnc->intraRefresh.eRefreshMode == OMX_VIDEO_IntraRefreshCyclic) {
        /* Cyclic Mode */
        pCommonParam->RandomIntraMBRefresh = pVideoEnc->intraRefresh.nCirMBs;
    } else {
        /* Don't support "Adaptive" and "Cyclic + Adaptive" */
        pCommonParam->RandomIntraMBRefresh = 0;
    }

    if (pExynosInputPort->bufferProcessType == BUFFER_SHARE) {
        if (pVideoEnc->ANBColorFormat == OMX_SEC_COLOR_FormatNV21Linear)
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

    /* Mpeg4 specific parameters */
    pMpeg4Param->ProfileIDC = OMXMpeg4ProfileToMFCProfile(pMpeg4Enc->mpeg4Component[OUTPUT_PORT_INDEX].eProfile);
    pMpeg4Param->LevelIDC   = OMXMpeg4LevelToMFCLevel(pMpeg4Enc->mpeg4Component[OUTPUT_PORT_INDEX].eLevel);
    pMpeg4Param->FrameQp_B  = pVideoEnc->quantization.nQpB;
    pMpeg4Param->TimeIncreamentRes = (pExynosInputPort->portDefinition.format.video.xFramerate) >> 16;
    pMpeg4Param->VopTimeIncreament = 1;
    pMpeg4Param->SliceArgument = 0; /* MB number or byte number */
    pMpeg4Param->NumberBFrames = 0; /* 0(not used) ~ 2 */
    pMpeg4Param->DisableQpelME = 1;

    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "pVideoEnc->eControlRate[OUTPUT_PORT_INDEX]: 0x%x", pVideoEnc->eControlRate[OUTPUT_PORT_INDEX]);
    /* rate control related parameters */
    switch (pVideoEnc->eControlRate[OUTPUT_PORT_INDEX]) {
    case OMX_Video_ControlRateDisable:
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "Video Encode DBR");
        pCommonParam->EnableFRMRateControl = 0; /* 0: Disable, 1: Frame level RC */
        pCommonParam->EnableMBRateControl  = 0; /* 0: Disable, 1:MB level RC */
        pCommonParam->CBRPeriodRf          = 100;
        break;
    case OMX_Video_ControlRateConstant:
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "Video Encode CBR");
        pCommonParam->EnableFRMRateControl = 1; /* 0: Disable, 1: Frame level RC */
        pCommonParam->EnableMBRateControl  = 1; /* 0: Disable, 1:MB level RC */
        pCommonParam->CBRPeriodRf          = 9;
        break;
    case OMX_Video_ControlRateVariable:
    default: /*Android default */
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "Video Encode VBR");
        pCommonParam->EnableFRMRateControl = 1; /* 0: Disable, 1: Frame level RC */
        pCommonParam->EnableMBRateControl  = 1; /* 0: Disable, 1:MB level RC */
        pCommonParam->CBRPeriodRf          = 100;
        break;
    }

    Print_Mpeg4Enc_Param(pEncParam);
}

static void Set_H263Enc_Param(EXYNOS_OMX_BASECOMPONENT *pExynosComponent)
{
    EXYNOS_OMX_BASEPORT           *pExynosInputPort  = NULL;
    EXYNOS_OMX_BASEPORT           *pExynosOutputPort = NULL;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc         = NULL;
    EXYNOS_MPEG4ENC_HANDLE        *pMpeg4Enc         = NULL;
    EXYNOS_MFC_MPEG4ENC_HANDLE     *pMFCMpeg4Handle;

    ExynosVideoEncParam       *pEncParam    = NULL;
    ExynosVideoEncCommonParam *pCommonParam = NULL;
    ExynosVideoEncH263Param   *pH263Param   = NULL;

    pVideoEnc = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    pMpeg4Enc = pVideoEnc->hCodecHandle;
    pMFCMpeg4Handle    = &pMpeg4Enc->hMFCMpeg4Handle;
    pExynosInputPort  = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    pExynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];

    pEncParam    = &pMFCMpeg4Handle->encParam;
    pCommonParam = &pEncParam->commonParam;
    pH263Param   = &pEncParam->codecParam.h263;
    pEncParam->eCompressionFormat = VIDEO_CODING_H263;
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "eCompressionFormat: %d", pEncParam->eCompressionFormat);

    /* common parameters */
    pCommonParam->SourceWidth  = pExynosOutputPort->portDefinition.format.video.nFrameWidth;
    pCommonParam->SourceHeight = pExynosOutputPort->portDefinition.format.video.nFrameHeight;
    pCommonParam->IDRPeriod    = pMpeg4Enc->h263Component[OUTPUT_PORT_INDEX].nPFrames + 1;
    pCommonParam->SliceMode    = 0;
    pCommonParam->Bitrate      = pExynosOutputPort->portDefinition.format.video.nBitrate;
    pCommonParam->FrameQp      = pVideoEnc->quantization.nQpI;
    pCommonParam->FrameQp_P    = pVideoEnc->quantization.nQpP;
    pCommonParam->QSCodeMax    = 30;
    pCommonParam->QSCodeMin    = 10;
    pCommonParam->PadControlOn = 0; /* 0: Use boundary pixel, 1: Use the below setting value */
    pCommonParam->LumaPadVal   = 0;
    pCommonParam->CbPadVal     = 0;
    pCommonParam->CrPadVal     = 0;

    if (pVideoEnc->intraRefresh.eRefreshMode == OMX_VIDEO_IntraRefreshCyclic) {
        /* Cyclic Mode */
        pCommonParam->RandomIntraMBRefresh = pVideoEnc->intraRefresh.nCirMBs;
    } else {
        /* Don't support "Adaptive" and "Cyclic + Adaptive" */
        pCommonParam->RandomIntraMBRefresh = 0;
    }

    if (pExynosInputPort->bufferProcessType == BUFFER_SHARE) {
        if (pVideoEnc->ANBColorFormat == OMX_SEC_COLOR_FormatNV21Linear)
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

    /* H263 specific parameters */
    pH263Param->FrameRate            = (pExynosInputPort->portDefinition.format.video.xFramerate) >> 16;

    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "pVideoEnc->eControlRate[OUTPUT_PORT_INDEX]: 0x%x", pVideoEnc->eControlRate[OUTPUT_PORT_INDEX]);
    /* rate control related parameters */
    switch (pVideoEnc->eControlRate[OUTPUT_PORT_INDEX]) {
    case OMX_Video_ControlRateDisable:
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "Video Encode DBR");
        pCommonParam->EnableFRMRateControl = 0; /* 0: Disable, 1: Frame level RC */
        pCommonParam->EnableMBRateControl  = 0; /* 0: Disable, 1:MB level RC */
        pCommonParam->CBRPeriodRf          = 100;
        break;
    case OMX_Video_ControlRateConstant:
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "Video Encode CBR");
        pCommonParam->EnableFRMRateControl = 1; /* 0: Disable, 1: Frame level RC */
        pCommonParam->EnableMBRateControl  = 1; /* 0: Disable, 1:MB level RC */
        pCommonParam->CBRPeriodRf          = 9;
        break;
    case OMX_Video_ControlRateVariable:
    default: /*Android default */
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "Video Encode VBR");
        pCommonParam->EnableFRMRateControl = 1; /* 0: Disable, 1: Frame level RC */
        pCommonParam->EnableMBRateControl  = 1; /* 0: Disable, 1:MB level RC */
        pCommonParam->CBRPeriodRf          = 100;
        break;
    }

    Print_H263Enc_Param(pEncParam);
}

static void Change_Mpeg4Enc_Param(EXYNOS_OMX_BASECOMPONENT *pExynosComponent)
{
    EXYNOS_OMX_BASEPORT           *pExynosInputPort  = NULL;
    EXYNOS_OMX_BASEPORT           *pExynosOutputPort = NULL;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc         = NULL;
    EXYNOS_MPEG4ENC_HANDLE        *pMpeg4Enc         = NULL;
    EXYNOS_MFC_MPEG4ENC_HANDLE     *pMFCMpeg4Handle;

    ExynosVideoEncOps         *pEncOps      = NULL;
    ExynosVideoEncParam       *pEncParam    = NULL;
    ExynosVideoEncCommonParam *pCommonParam = NULL;
    ExynosVideoEncMpeg4Param  *pMpeg4Param  = NULL;

    int setParam = 0;

    pVideoEnc = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    pMpeg4Enc = pVideoEnc->hCodecHandle;
    pMFCMpeg4Handle    = &pMpeg4Enc->hMFCMpeg4Handle;
    pExynosInputPort  = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    pExynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    pEncOps = pMFCMpeg4Handle->pEncOps;

    pEncParam    = &pMFCMpeg4Handle->encParam;
    pCommonParam = &pEncParam->commonParam;
    pMpeg4Param  = &pEncParam->codecParam.mpeg4;

    if (pVideoEnc->IntraRefreshVOP == OMX_TRUE) {
        setParam = VIDEO_FRAME_I;
        pEncOps->Set_FrameType(pMpeg4Enc->hMFCMpeg4Handle.hMFCHandle, setParam);
        pVideoEnc->IntraRefreshVOP = OMX_FALSE;
    }
    if (pCommonParam->IDRPeriod != (int)pMpeg4Enc->mpeg4Component[OUTPUT_PORT_INDEX].nPFrames + 1) {
        setParam = pMpeg4Enc->mpeg4Component[OUTPUT_PORT_INDEX].nPFrames + 1;
        pEncOps->Set_IDRPeriod(pMpeg4Enc->hMFCMpeg4Handle.hMFCHandle, setParam);
    }
    if (pCommonParam->Bitrate != (int)pExynosOutputPort->portDefinition.format.video.nBitrate) {
        setParam = pExynosOutputPort->portDefinition.format.video.nBitrate;
        pEncOps->Set_BitRate(pMpeg4Enc->hMFCMpeg4Handle.hMFCHandle, setParam);
    }
    if (pMpeg4Param->TimeIncreamentRes != (int)((pExynosInputPort->portDefinition.format.video.xFramerate) >> 16)) {
        setParam = (pExynosInputPort->portDefinition.format.video.xFramerate) >> 16;
        pEncOps->Set_FrameRate(pMpeg4Enc->hMFCMpeg4Handle.hMFCHandle, setParam);
    }

    Set_Mpeg4Enc_Param(pExynosComponent);
}

static void Change_H263Enc_Param(EXYNOS_OMX_BASECOMPONENT *pExynosComponent)
{
    EXYNOS_OMX_BASEPORT           *pExynosInputPort  = NULL;
    EXYNOS_OMX_BASEPORT           *pExynosOutputPort = NULL;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc         = NULL;
    EXYNOS_MPEG4ENC_HANDLE        *pMpeg4Enc         = NULL;
    EXYNOS_MFC_MPEG4ENC_HANDLE     *pMFCMpeg4Handle;

    ExynosVideoEncOps         *pEncOps      = NULL;
    ExynosVideoEncParam       *pEncParam    = NULL;
    ExynosVideoEncCommonParam *pCommonParam = NULL;
    ExynosVideoEncH263Param   *pH263Param   = NULL;

    int setParam = 0;

    pVideoEnc = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    pMpeg4Enc = pVideoEnc->hCodecHandle;
    pMFCMpeg4Handle    = &pMpeg4Enc->hMFCMpeg4Handle;
    pExynosInputPort  = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    pExynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    pEncOps = pMFCMpeg4Handle->pEncOps;

    pEncParam    = &pMFCMpeg4Handle->encParam;
    pCommonParam = &pEncParam->commonParam;
    pH263Param   = &pEncParam->codecParam.h263;

    if (pVideoEnc->IntraRefreshVOP == OMX_TRUE) {
        setParam = VIDEO_FRAME_I;
        pEncOps->Set_FrameType(pMpeg4Enc->hMFCMpeg4Handle.hMFCHandle, setParam);
        pVideoEnc->IntraRefreshVOP = OMX_FALSE;
    }
    if (pCommonParam->IDRPeriod != (int)pMpeg4Enc->h263Component[OUTPUT_PORT_INDEX].nPFrames + 1) {
        setParam = pMpeg4Enc->h263Component[OUTPUT_PORT_INDEX].nPFrames + 1;
        pEncOps->Set_IDRPeriod(pMpeg4Enc->hMFCMpeg4Handle.hMFCHandle, setParam);
    }
    if (pCommonParam->Bitrate != (int)pExynosOutputPort->portDefinition.format.video.nBitrate) {
        setParam = pExynosOutputPort->portDefinition.format.video.nBitrate;
        pEncOps->Set_BitRate(pMpeg4Enc->hMFCMpeg4Handle.hMFCHandle, setParam);
    }
    if (pH263Param->FrameRate != (int)((pExynosInputPort->portDefinition.format.video.xFramerate) >> 16)) {
        setParam = (pExynosInputPort->portDefinition.format.video.xFramerate) >> 16;
        pEncOps->Set_FrameRate(pMpeg4Enc->hMFCMpeg4Handle.hMFCHandle, setParam);
    }

    Set_H263Enc_Param(pExynosComponent);
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

OMX_ERRORTYPE Mpeg4CodecOpen(EXYNOS_MPEG4ENC_HANDLE *pMpeg4Enc)
{
    OMX_ERRORTYPE           ret = OMX_ErrorNone;

    ExynosVideoEncOps       *pEncOps    = NULL;
    ExynosVideoEncBufferOps *pInbufOps  = NULL;
    ExynosVideoEncBufferOps *pOutbufOps = NULL;

    FunctionIn();

    if (pMpeg4Enc == NULL) {
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

    pMpeg4Enc->hMFCMpeg4Handle.pEncOps = pEncOps;
    pMpeg4Enc->hMFCMpeg4Handle.pInbufOps = pInbufOps;
    pMpeg4Enc->hMFCMpeg4Handle.pOutbufOps = pOutbufOps;

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
    pMpeg4Enc->hMFCMpeg4Handle.hMFCHandle = pMpeg4Enc->hMFCMpeg4Handle.pEncOps->Init(V4L2_MEMORY_DMABUF);
    if (pMpeg4Enc->hMFCMpeg4Handle.hMFCHandle == NULL) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to allocate context buffer");
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    ret = OMX_ErrorNone;

EXIT:
    if (ret != OMX_ErrorNone) {
        if (pEncOps != NULL) {
            Exynos_OSAL_Free(pEncOps);
            pMpeg4Enc->hMFCMpeg4Handle.pEncOps = NULL;
        }
        if (pInbufOps != NULL) {
            Exynos_OSAL_Free(pInbufOps);
            pMpeg4Enc->hMFCMpeg4Handle.pInbufOps = NULL;
        }
        if (pOutbufOps != NULL) {
            Exynos_OSAL_Free(pOutbufOps);
            pMpeg4Enc->hMFCMpeg4Handle.pOutbufOps = NULL;
        }
    }

    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Mpeg4CodecClose(EXYNOS_MPEG4ENC_HANDLE *pMpeg4Enc)
{
    OMX_ERRORTYPE            ret = OMX_ErrorNone;
    void                    *hMFCHandle = NULL;
    ExynosVideoEncOps       *pEncOps    = NULL;
    ExynosVideoEncBufferOps *pInbufOps  = NULL;
    ExynosVideoEncBufferOps *pOutbufOps = NULL;

    FunctionIn();

    if (pMpeg4Enc == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    hMFCHandle = pMpeg4Enc->hMFCMpeg4Handle.hMFCHandle;
    pEncOps    = pMpeg4Enc->hMFCMpeg4Handle.pEncOps;
    pInbufOps  = pMpeg4Enc->hMFCMpeg4Handle.pInbufOps;
    pOutbufOps = pMpeg4Enc->hMFCMpeg4Handle.pOutbufOps;

    if (hMFCHandle != NULL) {
        pEncOps->Finalize(hMFCHandle);
        hMFCHandle = pMpeg4Enc->hMFCMpeg4Handle.hMFCHandle = NULL;
    }
    if (pOutbufOps != NULL) {
        Exynos_OSAL_Free(pOutbufOps);
        pOutbufOps = pMpeg4Enc->hMFCMpeg4Handle.pOutbufOps = NULL;
    }
    if (pInbufOps != NULL) {
        Exynos_OSAL_Free(pInbufOps);
        pInbufOps = pMpeg4Enc->hMFCMpeg4Handle.pInbufOps = NULL;
    }
    if (pEncOps != NULL) {
        Exynos_OSAL_Free(pEncOps);
        pEncOps = pMpeg4Enc->hMFCMpeg4Handle.pEncOps = NULL;
    }

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Mpeg4CodecStart(OMX_COMPONENTTYPE *pOMXComponent, OMX_U32 nPortIndex)
{
    OMX_ERRORTYPE            ret = OMX_ErrorNone;
    void                    *hMFCHandle = NULL;
    ExynosVideoEncOps       *pEncOps    = NULL;
    ExynosVideoEncBufferOps *pInbufOps  = NULL;
    ExynosVideoEncBufferOps *pOutbufOps = NULL;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc = NULL;
    EXYNOS_MPEG4ENC_HANDLE   *pMpeg4Enc = NULL;

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

    pMpeg4Enc = (EXYNOS_MPEG4ENC_HANDLE *)pVideoEnc->hCodecHandle;
    if (pMpeg4Enc == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    hMFCHandle = pMpeg4Enc->hMFCMpeg4Handle.hMFCHandle;
    pEncOps    = pMpeg4Enc->hMFCMpeg4Handle.pEncOps;
    pInbufOps  = pMpeg4Enc->hMFCMpeg4Handle.pInbufOps;
    pOutbufOps = pMpeg4Enc->hMFCMpeg4Handle.pOutbufOps;

    if (nPortIndex == INPUT_PORT_INDEX)
        pInbufOps->Run(hMFCHandle);
    else if (nPortIndex == OUTPUT_PORT_INDEX)
        pOutbufOps->Run(hMFCHandle);

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Mpeg4CodecStop(OMX_COMPONENTTYPE *pOMXComponent, OMX_U32 nPortIndex)
{
    OMX_ERRORTYPE            ret = OMX_ErrorNone;
    void                    *hMFCHandle = NULL;
    ExynosVideoEncOps       *pEncOps    = NULL;
    ExynosVideoEncBufferOps *pInbufOps  = NULL;
    ExynosVideoEncBufferOps *pOutbufOps = NULL;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc = NULL;
    EXYNOS_MPEG4ENC_HANDLE   *pMpeg4Enc = NULL;

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
    pMpeg4Enc = (EXYNOS_MPEG4ENC_HANDLE *)pVideoEnc->hCodecHandle;
    if (pMpeg4Enc == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    hMFCHandle = pMpeg4Enc->hMFCMpeg4Handle.hMFCHandle;
    pEncOps    = pMpeg4Enc->hMFCMpeg4Handle.pEncOps;
    pInbufOps  = pMpeg4Enc->hMFCMpeg4Handle.pInbufOps;
    pOutbufOps = pMpeg4Enc->hMFCMpeg4Handle.pOutbufOps;

    if ((nPortIndex == INPUT_PORT_INDEX) && (pInbufOps != NULL))
        pInbufOps->Stop(hMFCHandle);
    else if ((nPortIndex == OUTPUT_PORT_INDEX) && (pOutbufOps != NULL))
        pOutbufOps->Stop(hMFCHandle);

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Mpeg4CodecOutputBufferProcessRun(OMX_COMPONENTTYPE *pOMXComponent, OMX_U32 nPortIndex)
{
    OMX_ERRORTYPE            ret = OMX_ErrorNone;
    void                    *hMFCHandle = NULL;
    ExynosVideoEncOps       *pEncOps    = NULL;
    ExynosVideoEncBufferOps *pInbufOps  = NULL;
    ExynosVideoEncBufferOps *pOutbufOps = NULL;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc = NULL;
    EXYNOS_MPEG4ENC_HANDLE   *pMpeg4Enc = NULL;

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
    pMpeg4Enc = (EXYNOS_MPEG4ENC_HANDLE *)pVideoEnc->hCodecHandle;
    if (pMpeg4Enc == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    hMFCHandle = pMpeg4Enc->hMFCMpeg4Handle.hMFCHandle;
    pEncOps    = pMpeg4Enc->hMFCMpeg4Handle.pEncOps;
    pInbufOps  = pMpeg4Enc->hMFCMpeg4Handle.pInbufOps;
    pOutbufOps = pMpeg4Enc->hMFCMpeg4Handle.pOutbufOps;

    if (nPortIndex == INPUT_PORT_INDEX) {
        if (pMpeg4Enc->bSourceStart == OMX_FALSE) {
            Exynos_OSAL_SignalSet(pMpeg4Enc->hSourceStartEvent);
            Exynos_OSAL_SleepMillisec(0);
        }
    }

    if (nPortIndex == OUTPUT_PORT_INDEX) {
        if (pMpeg4Enc->bDestinationStart == OMX_FALSE) {
            Exynos_OSAL_SignalSet(pMpeg4Enc->hDestinationStartEvent);
            Exynos_OSAL_SleepMillisec(0);
        }
    }

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Mpeg4CodecRegistCodecBuffers(
    OMX_COMPONENTTYPE   *pOMXComponent,
    OMX_U32              nPortIndex,
    OMX_U32              nBufferCnt)
{
    OMX_ERRORTYPE                    ret                = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT        *pExynosComponent   = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEOENC_COMPONENT   *pVideoEnc          = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_MPEG4ENC_HANDLE          *pMpeg4Enc          = (EXYNOS_MPEG4ENC_HANDLE *)pVideoEnc->hCodecHandle;
    void                            *hMFCHandle         = pMpeg4Enc->hMFCMpeg4Handle.hMFCHandle;
    CODEC_ENC_BUFFER               **ppCodecBuffer      = NULL;
    ExynosVideoEncBufferOps         *pBufOps            = NULL;
    ExynosVideoPlane                *pPlanes            = NULL;

    OMX_U32 nPlaneCnt = 0;
    int i, j;

    FunctionIn();

    if (nPortIndex == INPUT_PORT_INDEX) {
        ppCodecBuffer   = &(pVideoEnc->pMFCEncInputBuffer[0]);
        nPlaneCnt       = MFC_INPUT_BUFFER_PLANE;
        pBufOps         = pMpeg4Enc->hMFCMpeg4Handle.pInbufOps;
    } else {
        ppCodecBuffer   = &(pVideoEnc->pMFCEncOutputBuffer[0]);
        nPlaneCnt       = MFC_OUTPUT_BUFFER_PLANE;
        pBufOps         = pMpeg4Enc->hMFCMpeg4Handle.pOutbufOps;
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

OMX_ERRORTYPE Mpeg4CodecEnQueueAllBuffer(OMX_COMPONENTTYPE *pOMXComponent, OMX_U32 nPortIndex)
{
    OMX_ERRORTYPE                  ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_MPEG4ENC_HANDLE         *pMpeg4Enc = (EXYNOS_MPEG4ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
    void                          *hMFCHandle = pMpeg4Enc->hMFCMpeg4Handle.hMFCHandle;
    EXYNOS_OMX_BASEPORT           *pExynosInputPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT           *pExynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    int i, nOutbufs;

    ExynosVideoEncOps       *pEncOps    = pMpeg4Enc->hMFCMpeg4Handle.pEncOps;
    ExynosVideoEncBufferOps *pInbufOps  = pMpeg4Enc->hMFCMpeg4Handle.pInbufOps;
    ExynosVideoEncBufferOps *pOutbufOps = pMpeg4Enc->hMFCMpeg4Handle.pOutbufOps;

    FunctionIn();

    if ((nPortIndex != INPUT_PORT_INDEX) && (nPortIndex != OUTPUT_PORT_INDEX)) {
        ret = OMX_ErrorBadPortIndex;
        goto EXIT;
    }

    if ((nPortIndex == INPUT_PORT_INDEX) &&
        (pMpeg4Enc->bSourceStart == OMX_TRUE)) {
        Exynos_CodecBufferReset(pExynosComponent, INPUT_PORT_INDEX);

        for (i = 0; i < MFC_INPUT_BUFFER_NUM_MAX; i++)  {
            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "pVideoEnc->pMFCEncInputBuffer[%d]: 0x%x", i, pVideoEnc->pMFCEncInputBuffer[i]);
            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "pVideoEnc->pMFCEncInputBuffer[%d]->pVirAddr[0]: 0x%x", i, pVideoEnc->pMFCEncInputBuffer[i]->pVirAddr[0]);
            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "pVideoEnc->pMFCEncInputBuffer[%d]->pVirAddr[1]: 0x%x", i, pVideoEnc->pMFCEncInputBuffer[i]->pVirAddr[1]);

            Exynos_CodecBufferEnQueue(pExynosComponent, INPUT_PORT_INDEX, pVideoEnc->pMFCEncInputBuffer[i]);
        }

        pInbufOps->Clear_Queue(hMFCHandle);
    } else if ((nPortIndex == OUTPUT_PORT_INDEX) &&
               (pMpeg4Enc->bDestinationStart == OMX_TRUE)) {
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

OMX_ERRORTYPE Mpeg4CodecSrcSetup(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *pSrcInputData)
{
    OMX_ERRORTYPE                  ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_MPEG4ENC_HANDLE         *pMpeg4Enc = (EXYNOS_MPEG4ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
    EXYNOS_MFC_MPEG4ENC_HANDLE     *pMFCMpeg4Handle    = &pMpeg4Enc->hMFCMpeg4Handle;
    void                          *hMFCHandle = pMFCMpeg4Handle->hMFCHandle;
    EXYNOS_OMX_BASEPORT      *pExynosInputPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT           *pExynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    OMX_U32                     oneFrameSize = pSrcInputData->dataLen;

    ExynosVideoEncOps       *pEncOps    = pMpeg4Enc->hMFCMpeg4Handle.pEncOps;
    ExynosVideoEncBufferOps *pInbufOps  = pMpeg4Enc->hMFCMpeg4Handle.pInbufOps;
    ExynosVideoEncBufferOps *pOutbufOps = pMpeg4Enc->hMFCMpeg4Handle.pOutbufOps;
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

    if (pMpeg4Enc->hMFCMpeg4Handle.codecType == CODEC_TYPE_MPEG4)
        Set_Mpeg4Enc_Param(pExynosComponent);
    else
        Set_H263Enc_Param(pExynosComponent);

    pEncParam = &pMFCMpeg4Handle->encParam;
    if (pEncOps->Set_EncParam) {
        if(pEncOps->Set_EncParam(pMpeg4Enc->hMFCMpeg4Handle.hMFCHandle, pEncParam) != VIDEO_ERROR_NONE) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to set geometry for input buffer");
            ret = OMX_ErrorInsufficientResources;
            goto EXIT;
        }
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
        if (pInbufOps->Set_Geometry(pMpeg4Enc->hMFCMpeg4Handle.hMFCHandle, &bufferConf) != VIDEO_ERROR_NONE) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to set geometry for input buffer");
            ret = OMX_ErrorInsufficientResources;
            goto EXIT;
        }
    }

    /* setup input buffer */
    if (pInbufOps->Setup(pMpeg4Enc->hMFCMpeg4Handle.hMFCHandle, inputBufferNumber) != VIDEO_ERROR_NONE) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to setup input buffer");
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    ExynosVideoPlane planes[MFC_INPUT_BUFFER_PLANE];
    int plane;

    if ((pExynosInputPort->bufferProcessType & BUFFER_COPY) == BUFFER_COPY) {
        ret = Mpeg4CodecRegistCodecBuffers(pOMXComponent, INPUT_PORT_INDEX, MFC_INPUT_BUFFER_NUM_MAX);
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

    pMpeg4Enc->hMFCMpeg4Handle.bConfiguredMFCSrc = OMX_TRUE;
    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Mpeg4CodecDstSetup(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE                  ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_MPEG4ENC_HANDLE         *pMpeg4Enc = (EXYNOS_MPEG4ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
    EXYNOS_MFC_MPEG4ENC_HANDLE     *pMFCMpeg4Handle    = &pMpeg4Enc->hMFCMpeg4Handle;
    void                          *hMFCHandle = pMFCMpeg4Handle->hMFCHandle;
    EXYNOS_OMX_BASEPORT           *pExynosInputPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT           *pExynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];

    ExynosVideoEncOps       *pEncOps    = pMpeg4Enc->hMFCMpeg4Handle.pEncOps;
    ExynosVideoEncBufferOps *pInbufOps  = pMpeg4Enc->hMFCMpeg4Handle.pInbufOps;
    ExynosVideoEncBufferOps *pOutbufOps = pMpeg4Enc->hMFCMpeg4Handle.pOutbufOps;
    ExynosVideoGeometry      bufferConf;
    int i, nOutbufs;

    FunctionIn();

    int OutBufferSize = pExynosOutputPort->portDefinition.format.video.nFrameWidth * pExynosOutputPort->portDefinition.format.video.nFrameHeight * 3 / 2;
    /* set geometry for output (dst) */
    if (pOutbufOps->Set_Geometry) {
        /* only 2 config values needed */
        if (pMpeg4Enc->hMFCMpeg4Handle.codecType == CODEC_TYPE_MPEG4)
            bufferConf.eCompressionFormat = VIDEO_CODING_MPEG4;
        else
            bufferConf.eCompressionFormat = VIDEO_CODING_H263;
        bufferConf.nSizeImage = OutBufferSize;

        if (pOutbufOps->Set_Geometry(pMpeg4Enc->hMFCMpeg4Handle.hMFCHandle, &bufferConf) != VIDEO_ERROR_NONE) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to set geometry for output buffer");
            ret = OMX_ErrorInsufficientResources;
            goto EXIT;
        }
    }

    /* should be done before prepare output buffer */
    if (pOutbufOps->Enable_Cacheable) {
        if (pOutbufOps->Enable_Cacheable(hMFCHandle) != VIDEO_ERROR_NONE) {
            ret = OMX_ErrorInsufficientResources;
            goto EXIT;
        }
    }

    if (pOutbufOps->Set_Shareable) {
        pOutbufOps->Set_Shareable(hMFCHandle);
    }
    int SetupBufferNumber = 0;
    if ((pExynosOutputPort->bufferProcessType & BUFFER_COPY) == BUFFER_COPY)
        SetupBufferNumber = MFC_OUTPUT_BUFFER_NUM_MAX;
    else
        SetupBufferNumber = pExynosOutputPort->portDefinition.nBufferCountActual;
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "SetupBufferNumber:%d", SetupBufferNumber);

    if (pOutbufOps->Setup(pMpeg4Enc->hMFCMpeg4Handle.hMFCHandle, SetupBufferNumber) != VIDEO_ERROR_NONE) {
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

        ret = Mpeg4CodecRegistCodecBuffers(pOMXComponent, OUTPUT_PORT_INDEX, MFC_OUTPUT_BUFFER_NUM_MAX);
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
    if (pOutbufOps->Run) {
        if (pOutbufOps->Run(hMFCHandle) != VIDEO_ERROR_NONE) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to run output buffer");
            ret = OMX_ErrorInsufficientResources;
            goto EXIT;
        }
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

        Mpeg4CodecStop(pOMXComponent, OUTPUT_PORT_INDEX);
    }
    pMpeg4Enc->hMFCMpeg4Handle.bConfiguredMFCDst = OMX_TRUE;

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_Mpeg4Enc_GetParameter(
    OMX_IN    OMX_HANDLETYPE hComponent,
    OMX_IN    OMX_INDEXTYPE  nParamIndex,
    OMX_INOUT OMX_PTR        pComponentParameterStructure)
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
    case OMX_IndexParamVideoMpeg4:
    {
        OMX_VIDEO_PARAM_MPEG4TYPE *pDstMpeg4Component = (OMX_VIDEO_PARAM_MPEG4TYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_MPEG4TYPE *pSrcMpeg4Component = NULL;
        EXYNOS_MPEG4ENC_HANDLE    *pMpeg4Enc = NULL;

        ret = Exynos_OMX_Check_SizeVersion(pDstMpeg4Component, sizeof(OMX_VIDEO_PARAM_MPEG4TYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pDstMpeg4Component->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pMpeg4Enc = (EXYNOS_MPEG4ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
        pSrcMpeg4Component = &pMpeg4Enc->mpeg4Component[pDstMpeg4Component->nPortIndex];

        Exynos_OSAL_Memcpy(pDstMpeg4Component, pSrcMpeg4Component, sizeof(OMX_VIDEO_PARAM_MPEG4TYPE));
    }
        break;
    case OMX_IndexParamVideoH263:
    {
        OMX_VIDEO_PARAM_H263TYPE  *pDstH263Component = (OMX_VIDEO_PARAM_H263TYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_H263TYPE  *pSrcH263Component = NULL;
        EXYNOS_MPEG4ENC_HANDLE    *pMpeg4Enc = NULL;

        ret = Exynos_OMX_Check_SizeVersion(pDstH263Component, sizeof(OMX_VIDEO_PARAM_H263TYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pDstH263Component->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pMpeg4Enc = (EXYNOS_MPEG4ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
        pSrcH263Component = &pMpeg4Enc->h263Component[pDstH263Component->nPortIndex];

        Exynos_OSAL_Memcpy(pDstH263Component, pSrcH263Component, sizeof(OMX_VIDEO_PARAM_H263TYPE));
    }
        break;
    case OMX_IndexParamStandardComponentRole:
    {
        OMX_S32 codecType;
        OMX_PARAM_COMPONENTROLETYPE *pComponentRole = (OMX_PARAM_COMPONENTROLETYPE *)pComponentParameterStructure;
        ret = Exynos_OMX_Check_SizeVersion(pComponentRole, sizeof(OMX_PARAM_COMPONENTROLETYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        codecType = ((EXYNOS_MPEG4ENC_HANDLE *)(((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle))->hMFCMpeg4Handle.codecType;
        if (codecType == CODEC_TYPE_MPEG4)
            Exynos_OSAL_Strcpy((char *)pComponentRole->cRole, EXYNOS_OMX_COMPONENT_MPEG4_ENC_ROLE);
        else
            Exynos_OSAL_Strcpy((char *)pComponentRole->cRole, EXYNOS_OMX_COMPONENT_H263_ENC_ROLE);
    }
        break;
    case OMX_IndexParamVideoProfileLevelQuerySupported:
    {
        OMX_VIDEO_PARAM_PROFILELEVELTYPE *pDstProfileLevel = (OMX_VIDEO_PARAM_PROFILELEVELTYPE *)pComponentParameterStructure;
        EXYNOS_OMX_VIDEO_PROFILELEVEL    *pProfileLevel = NULL;
        OMX_U32                           maxProfileLevelNum = 0;
        OMX_S32                           codecType;

        ret = Exynos_OMX_Check_SizeVersion(pDstProfileLevel, sizeof(OMX_VIDEO_PARAM_PROFILELEVELTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pDstProfileLevel->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        codecType = ((EXYNOS_MPEG4ENC_HANDLE *)(((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle))->hMFCMpeg4Handle.codecType;
        if (codecType == CODEC_TYPE_MPEG4) {
            pProfileLevel = supportedMPEG4ProfileLevels;
            maxProfileLevelNum = sizeof(supportedMPEG4ProfileLevels) / sizeof(EXYNOS_OMX_VIDEO_PROFILELEVEL);
        } else {
            pProfileLevel = supportedH263ProfileLevels;
            maxProfileLevelNum = sizeof(supportedH263ProfileLevels) / sizeof(EXYNOS_OMX_VIDEO_PROFILELEVEL);
        }

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
        OMX_VIDEO_PARAM_PROFILELEVELTYPE *pDstProfileLevel = (OMX_VIDEO_PARAM_PROFILELEVELTYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_MPEG4TYPE        *pSrcMpeg4Component = NULL;
        OMX_VIDEO_PARAM_H263TYPE         *pSrcH263Component = NULL;
        EXYNOS_MPEG4ENC_HANDLE           *pMpeg4Enc = NULL;
        OMX_S32                           codecType;

        ret = Exynos_OMX_Check_SizeVersion(pDstProfileLevel, sizeof(OMX_VIDEO_PARAM_PROFILELEVELTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pDstProfileLevel->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pMpeg4Enc = (EXYNOS_MPEG4ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
        codecType = pMpeg4Enc->hMFCMpeg4Handle.codecType;
        if (codecType == CODEC_TYPE_MPEG4) {
            pSrcMpeg4Component = &pMpeg4Enc->mpeg4Component[pDstProfileLevel->nPortIndex];
            pDstProfileLevel->eProfile = pSrcMpeg4Component->eProfile;
            pDstProfileLevel->eLevel = pSrcMpeg4Component->eLevel;
        } else {
            pSrcH263Component = &pMpeg4Enc->h263Component[pDstProfileLevel->nPortIndex];
            pDstProfileLevel->eProfile = pSrcH263Component->eProfile;
            pDstProfileLevel->eLevel = pSrcH263Component->eLevel;
        }
    }
        break;
    case OMX_IndexParamVideoErrorCorrection:
    {
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pDstErrorCorrectionType = (OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pSrcErrorCorrectionType = NULL;
        EXYNOS_MPEG4ENC_HANDLE              *pMpeg4Enc = NULL;

        ret = Exynos_OMX_Check_SizeVersion(pDstErrorCorrectionType, sizeof(OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pDstErrorCorrectionType->nPortIndex != OUTPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pMpeg4Enc = (EXYNOS_MPEG4ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
        pSrcErrorCorrectionType = &pMpeg4Enc->errorCorrectionType[OUTPUT_PORT_INDEX];

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

OMX_ERRORTYPE Exynos_Mpeg4Enc_SetParameter(
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
    case OMX_IndexParamVideoMpeg4:
    {
        OMX_VIDEO_PARAM_MPEG4TYPE *pDstMpeg4Component = NULL;
        OMX_VIDEO_PARAM_MPEG4TYPE *pSrcMpeg4Component = (OMX_VIDEO_PARAM_MPEG4TYPE *)pComponentParameterStructure;
        EXYNOS_MPEG4ENC_HANDLE    *pMpeg4Enc = NULL;

        ret = Exynos_OMX_Check_SizeVersion(pSrcMpeg4Component, sizeof(OMX_VIDEO_PARAM_MPEG4TYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pSrcMpeg4Component->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pMpeg4Enc = (EXYNOS_MPEG4ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
        pDstMpeg4Component = &pMpeg4Enc->mpeg4Component[pSrcMpeg4Component->nPortIndex];

        Exynos_OSAL_Memcpy(pDstMpeg4Component, pSrcMpeg4Component, sizeof(OMX_VIDEO_PARAM_MPEG4TYPE));
    }
        break;
    case OMX_IndexParamVideoH263:
    {
        OMX_VIDEO_PARAM_H263TYPE *pDstH263Component = NULL;
        OMX_VIDEO_PARAM_H263TYPE *pSrcH263Component = (OMX_VIDEO_PARAM_H263TYPE *)pComponentParameterStructure;
        EXYNOS_MPEG4ENC_HANDLE   *pMpeg4Enc = NULL;

        ret = Exynos_OMX_Check_SizeVersion(pSrcH263Component, sizeof(OMX_VIDEO_PARAM_H263TYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pSrcH263Component->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pMpeg4Enc = (EXYNOS_MPEG4ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
        pDstH263Component = &pMpeg4Enc->h263Component[pSrcH263Component->nPortIndex];

        Exynos_OSAL_Memcpy(pDstH263Component, pSrcH263Component, sizeof(OMX_VIDEO_PARAM_H263TYPE));
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

        if (!Exynos_OSAL_Strcmp((char*)pComponentRole->cRole, EXYNOS_OMX_COMPONENT_MPEG4_ENC_ROLE)) {
            pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX].portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingMPEG4;
            //((EXYNOS_MPEG4ENC_HANDLE *)(((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle))->hMFCMpeg4Handle.codecType = CODEC_TYPE_MPEG4;
        } else if (!Exynos_OSAL_Strcmp((char*)pComponentRole->cRole, EXYNOS_OMX_COMPONENT_H263_ENC_ROLE)) {
            pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX].portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingH263;
            //((EXYNOS_MPEG4ENC_HANDLE *)(((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle))->hMFCMpeg4Handle.codecType = CODEC_TYPE_H263;
        } else {
            ret = OMX_ErrorBadParameter;
            goto EXIT;
        }
    }
        break;
    case OMX_IndexParamVideoProfileLevelCurrent:
    {
        OMX_VIDEO_PARAM_PROFILELEVELTYPE *pSrcProfileLevel = (OMX_VIDEO_PARAM_PROFILELEVELTYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_MPEG4TYPE        *pDstMpeg4Component = NULL;
        OMX_VIDEO_PARAM_H263TYPE         *pDstH263Component = NULL;
        EXYNOS_MPEG4ENC_HANDLE           *pMpeg4Enc = NULL;
        OMX_S32                           codecType;

        ret = Exynos_OMX_Check_SizeVersion(pSrcProfileLevel, sizeof(OMX_VIDEO_PARAM_PROFILELEVELTYPE));
        if (ret != OMX_ErrorNone)
            goto EXIT;

        if (pSrcProfileLevel->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pMpeg4Enc = (EXYNOS_MPEG4ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
        codecType = pMpeg4Enc->hMFCMpeg4Handle.codecType;
        if (codecType == CODEC_TYPE_MPEG4) {
            /*
             * To do: Check validity of profile & level parameters
             */

            pDstMpeg4Component = &pMpeg4Enc->mpeg4Component[pSrcProfileLevel->nPortIndex];
            pDstMpeg4Component->eProfile = pSrcProfileLevel->eProfile;
            pDstMpeg4Component->eLevel = pSrcProfileLevel->eLevel;
        } else {
            /*
             * To do: Check validity of profile & level parameters
             */

            pDstH263Component = &pMpeg4Enc->h263Component[pSrcProfileLevel->nPortIndex];
            pDstH263Component->eProfile = pSrcProfileLevel->eProfile;
            pDstH263Component->eLevel = pSrcProfileLevel->eLevel;
        }
    }
        break;
    case OMX_IndexParamVideoErrorCorrection:
    {
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pSrcErrorCorrectionType = (OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pDstErrorCorrectionType = NULL;
        EXYNOS_MPEG4ENC_HANDLE              *pMpeg4Enc = NULL;

        ret = Exynos_OMX_Check_SizeVersion(pSrcErrorCorrectionType, sizeof(OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pSrcErrorCorrectionType->nPortIndex != OUTPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pMpeg4Enc = (EXYNOS_MPEG4ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
        pDstErrorCorrectionType = &pMpeg4Enc->errorCorrectionType[OUTPUT_PORT_INDEX];

        pDstErrorCorrectionType->bEnableHEC = pSrcErrorCorrectionType->bEnableHEC;
        pDstErrorCorrectionType->bEnableResync = pSrcErrorCorrectionType->bEnableResync;
        pDstErrorCorrectionType->nResynchMarkerSpacing = pSrcErrorCorrectionType->nResynchMarkerSpacing;
        pDstErrorCorrectionType->bEnableDataPartitioning = pSrcErrorCorrectionType->bEnableDataPartitioning;
        pDstErrorCorrectionType->bEnableRVLC = pSrcErrorCorrectionType->bEnableRVLC;
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

OMX_ERRORTYPE Exynos_Mpeg4Enc_GetConfig(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nIndex,
    OMX_IN OMX_PTR        pComponentConfigStructure)
{
    OMX_ERRORTYPE             ret              = OMX_ErrorNone;
    OMX_COMPONENTTYPE        *pOMXComponent    = NULL;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;

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

    switch (nIndex) {
    default:
        ret = Exynos_OMX_VideoEncodeGetConfig(hComponent, nIndex, pComponentConfigStructure);
        break;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_Mpeg4Enc_SetConfig(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nIndex,
    OMX_IN OMX_PTR        pComponentConfigStructure)
{
    OMX_ERRORTYPE                  ret              = OMX_ErrorNone;
    OMX_COMPONENTTYPE             *pOMXComponent    = NULL;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent = NULL;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc        = NULL;
    EXYNOS_MPEG4ENC_HANDLE        *pMpeg4Enc        = NULL;

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
    pMpeg4Enc = (EXYNOS_MPEG4ENC_HANDLE *)pVideoEnc->hCodecHandle;

    switch (nIndex) {
    case OMX_IndexConfigVideoIntraPeriod:
    {
        OMX_U32 nPFrames = (*((OMX_U32 *)pComponentConfigStructure)) - 1;

        if (pMpeg4Enc->hMFCMpeg4Handle.codecType == CODEC_TYPE_MPEG4)
            pMpeg4Enc->mpeg4Component[OUTPUT_PORT_INDEX].nPFrames = nPFrames;
        else
            pMpeg4Enc->h263Component[OUTPUT_PORT_INDEX].nPFrames = nPFrames;

        ret = OMX_ErrorNone;
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

OMX_ERRORTYPE Exynos_Mpeg4Enc_GetExtensionIndex(
    OMX_IN  OMX_HANDLETYPE  hComponent,
    OMX_IN  OMX_STRING      cParameterName,
    OMX_OUT OMX_INDEXTYPE  *pIndexType)
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
    } else {
        ret = Exynos_OMX_VideoEncodeGetExtensionIndex(hComponent, cParameterName, pIndexType);
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_Mpeg4Enc_ComponentRoleEnum(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_OUT OMX_U8        *cRole,
    OMX_IN  OMX_U32        nIndex)
{
    OMX_ERRORTYPE               ret              = OMX_ErrorNone;
    OMX_COMPONENTTYPE          *pOMXComponent    = NULL;
    EXYNOS_OMX_BASECOMPONENT   *pExynosComponent = NULL;
    OMX_S32                     codecType;

    FunctionIn();

    if ((hComponent == NULL) || (cRole == NULL)) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if (nIndex != (MAX_COMPONENT_ROLE_NUM - 1)) { /* supports only one role */
        ret = OMX_ErrorNoMore;
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

    codecType = ((EXYNOS_MPEG4ENC_HANDLE *)(((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle))->hMFCMpeg4Handle.codecType;
    if (codecType == CODEC_TYPE_MPEG4)
        Exynos_OSAL_Strcpy((char *)cRole, EXYNOS_OMX_COMPONENT_MPEG4_ENC_ROLE);
    else
        Exynos_OSAL_Strcpy((char *)cRole, EXYNOS_OMX_COMPONENT_H263_ENC_ROLE);

EXIT:
    FunctionOut();

    return ret;
}

/* MFC Init */
OMX_ERRORTYPE Exynos_Mpeg4Enc_Init(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE             ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_OMX_BASEPORT      *pExynosInputPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT      *pExynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    EXYNOS_MPEG4ENC_HANDLE    *pMpeg4Enc = (EXYNOS_MPEG4ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;;
    EXYNOS_MFC_MPEG4ENC_HANDLE     *pMFCMpeg4Handle    = &pMpeg4Enc->hMFCMpeg4Handle;
    OMX_PTR                   hMFCHandle = pMpeg4Enc->hMFCMpeg4Handle.hMFCHandle;
    OMX_COLOR_FORMATTYPE      eColorFormat;

    ExynosVideoEncOps       *pEncOps    = NULL;
    ExynosVideoEncBufferOps *pInbufOps  = NULL;
    ExynosVideoEncBufferOps *pOutbufOps = NULL;

    int i = 0;

    FunctionIn();

    pMpeg4Enc->hMFCMpeg4Handle.bConfiguredMFCSrc = OMX_FALSE;
    pMpeg4Enc->hMFCMpeg4Handle.bConfiguredMFCDst = OMX_FALSE;
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

    /* Mpeg4/H.263 Codec Open */
    ret = Mpeg4CodecOpen(pMpeg4Enc);
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    pEncOps    = pMpeg4Enc->hMFCMpeg4Handle.pEncOps;
    pInbufOps  = pMpeg4Enc->hMFCMpeg4Handle.pInbufOps;
    pOutbufOps = pMpeg4Enc->hMFCMpeg4Handle.pOutbufOps;

    if ((pExynosOutputPort->bufferProcessType & BUFFER_COPY) == BUFFER_COPY) {
        Exynos_OSAL_SemaphoreCreate(&pExynosOutputPort->codecSemID);
        Exynos_OSAL_QueueCreate(&pExynosOutputPort->codecBufferQ, MAX_QUEUE_ELEMENTS);
    } else if (pExynosOutputPort->bufferProcessType == BUFFER_SHARE) {
        /*************/
        /*    TBD    */
        /*************/
        /* Does not require any actions. */
    }

    pMpeg4Enc->bSourceStart = OMX_FALSE;
    Exynos_OSAL_SignalCreate(&pMpeg4Enc->hSourceStartEvent);
    pMpeg4Enc->bDestinationStart = OMX_FALSE;
    Exynos_OSAL_SignalCreate(&pMpeg4Enc->hDestinationStartEvent);

    Exynos_OSAL_Memset(pExynosComponent->timeStamp, -19771003, sizeof(OMX_TICKS) * MAX_TIMESTAMP);
    Exynos_OSAL_Memset(pExynosComponent->nFlags, 0, sizeof(OMX_U32) * MAX_FLAGS);
    pMpeg4Enc->hMFCMpeg4Handle.indexTimestamp = 0;
    pMpeg4Enc->hMFCMpeg4Handle.outputIndexTimestamp = 0;

    pExynosComponent->getAllDelayBuffer = OMX_FALSE;

EXIT:
    FunctionOut();

    return ret;
}

/* MFC Terminate */
OMX_ERRORTYPE Exynos_Mpeg4Enc_Terminate(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE                  ret              = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc        = ((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle);
    EXYNOS_OMX_BASEPORT      *pExynosInputPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT      *pExynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    EXYNOS_MPEG4ENC_HANDLE    *pMpeg4Enc = (EXYNOS_MPEG4ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
    OMX_PTR                hMFCHandle = pMpeg4Enc->hMFCMpeg4Handle.hMFCHandle;

    ExynosVideoEncOps       *pEncOps    = pMpeg4Enc->hMFCMpeg4Handle.pEncOps;
    ExynosVideoEncBufferOps *pInbufOps  = pMpeg4Enc->hMFCMpeg4Handle.pInbufOps;
    ExynosVideoEncBufferOps *pOutbufOps = pMpeg4Enc->hMFCMpeg4Handle.pOutbufOps;

    int i = 0, plane = 0;

    FunctionIn();

    Exynos_OSAL_SignalTerminate(pMpeg4Enc->hDestinationStartEvent);
    pMpeg4Enc->hDestinationStartEvent = NULL;
    pMpeg4Enc->bDestinationStart = OMX_FALSE;
    Exynos_OSAL_SignalTerminate(pMpeg4Enc->hSourceStartEvent);
    pMpeg4Enc->hSourceStartEvent = NULL;
    pMpeg4Enc->bSourceStart = OMX_FALSE;

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
    Mpeg4CodecClose(pMpeg4Enc);

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_Mpeg4Enc_SrcIn(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *pSrcInputData)
{
    OMX_ERRORTYPE               ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_MPEG4ENC_HANDLE         *pMpeg4Enc = (EXYNOS_MPEG4ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
    void                          *hMFCHandle = pMpeg4Enc->hMFCMpeg4Handle.hMFCHandle;
    EXYNOS_OMX_BASEPORT *pExynosInputPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT *pExynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    OMX_U32  oneFrameSize = pSrcInputData->dataLen;
    ExynosVideoEncOps       *pEncOps    = pMpeg4Enc->hMFCMpeg4Handle.pEncOps;
    ExynosVideoEncBufferOps *pInbufOps  = pMpeg4Enc->hMFCMpeg4Handle.pInbufOps;
    ExynosVideoEncBufferOps *pOutbufOps = pMpeg4Enc->hMFCMpeg4Handle.pOutbufOps;
    ExynosVideoErrorType codecReturn = VIDEO_ERROR_NONE;
    int i;

    FunctionIn();

    if (pMpeg4Enc->hMFCMpeg4Handle.bConfiguredMFCSrc == OMX_FALSE) {
        ret = Mpeg4CodecSrcSetup(pOMXComponent, pSrcInputData);
        if ((pSrcInputData->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS)
            goto EXIT;
    }
    if (pMpeg4Enc->hMFCMpeg4Handle.bConfiguredMFCDst == OMX_FALSE) {
        ret = Mpeg4CodecDstSetup(pOMXComponent);
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }
    }

    if (pVideoEnc->configChange == OMX_TRUE) {
        if (pMpeg4Enc->hMFCMpeg4Handle.codecType == CODEC_TYPE_MPEG4)
            Change_Mpeg4Enc_Param(pExynosComponent);
        else
            Change_H263Enc_Param(pExynosComponent);
        pVideoEnc->configChange = OMX_FALSE;
    }
    if ((pSrcInputData->dataLen >= 0) ||
        ((pSrcInputData->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS)) {
        OMX_U32 nAllocLen[MFC_INPUT_BUFFER_PLANE] = {0, 0};
        OMX_U32 pMFCYUVDataSize[MFC_INPUT_BUFFER_PLANE]  = {NULL, NULL};
        ExynosVideoPlane planes[MFC_INPUT_BUFFER_PLANE];
        int plane;

        pExynosComponent->timeStamp[pMpeg4Enc->hMFCMpeg4Handle.indexTimestamp] = pSrcInputData->timeStamp;
        pExynosComponent->nFlags[pMpeg4Enc->hMFCMpeg4Handle.indexTimestamp] = pSrcInputData->nFlags;
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "input timestamp %lld us (%.2f secs), Tag: %d, nFlags: 0x%x", pSrcInputData->timeStamp, pSrcInputData->timeStamp / 1E6, pMpeg4Enc->hMFCMpeg4Handle.indexTimestamp, pSrcInputData->nFlags);
        pEncOps->Set_FrameTag(hMFCHandle, pMpeg4Enc->hMFCMpeg4Handle.indexTimestamp);
        pMpeg4Enc->hMFCMpeg4Handle.indexTimestamp++;
        pMpeg4Enc->hMFCMpeg4Handle.indexTimestamp %= MAX_TIMESTAMP;

        /* queue work for input buffer */
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "Exynos_Mpeg4Enc_SrcIn(): oneFrameSize: %d, bufferHeader: 0x%x", oneFrameSize, pSrcInputData->bufferHeader);
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
        Mpeg4CodecStart(pOMXComponent, INPUT_PORT_INDEX);
        if (pMpeg4Enc->bSourceStart == OMX_FALSE) {
            pMpeg4Enc->bSourceStart = OMX_TRUE;
            Exynos_OSAL_SignalSet(pMpeg4Enc->hSourceStartEvent);
            Exynos_OSAL_SleepMillisec(0);
        }
        if (pMpeg4Enc->bDestinationStart == OMX_FALSE) {
            pMpeg4Enc->bDestinationStart = OMX_TRUE;
            Exynos_OSAL_SignalSet(pMpeg4Enc->hDestinationStartEvent);
            Exynos_OSAL_SleepMillisec(0);
        }
    }

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_Mpeg4Enc_SrcOut(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *pSrcOutputData)
{
    OMX_ERRORTYPE                  ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_MPEG4ENC_HANDLE         *pMpeg4Enc = (EXYNOS_MPEG4ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
    void                          *hMFCHandle = pMpeg4Enc->hMFCMpeg4Handle.hMFCHandle;
    EXYNOS_OMX_BASEPORT     *pExynosInputPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    ExynosVideoEncOps       *pEncOps    = pMpeg4Enc->hMFCMpeg4Handle.pEncOps;
    ExynosVideoEncBufferOps *pInbufOps  = pMpeg4Enc->hMFCMpeg4Handle.pInbufOps;
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

OMX_ERRORTYPE Exynos_Mpeg4Enc_DstIn(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *pDstInputData)
{
    OMX_ERRORTYPE                  ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_MPEG4ENC_HANDLE         *pMpeg4Enc = (EXYNOS_MPEG4ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
    void                          *hMFCHandle = pMpeg4Enc->hMFCMpeg4Handle.hMFCHandle;
    ExynosVideoEncOps       *pEncOps    = pMpeg4Enc->hMFCMpeg4Handle.pEncOps;
    ExynosVideoEncBufferOps *pOutbufOps = pMpeg4Enc->hMFCMpeg4Handle.pOutbufOps;
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
    Mpeg4CodecStart(pOMXComponent, OUTPUT_PORT_INDEX);

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_Mpeg4Enc_DstOut(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *pDstOutputData)
{
    OMX_ERRORTYPE                  ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_MPEG4ENC_HANDLE         *pMpeg4Enc = (EXYNOS_MPEG4ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
    void                          *hMFCHandle = pMpeg4Enc->hMFCMpeg4Handle.hMFCHandle;
    ExynosVideoEncOps       *pEncOps    = pMpeg4Enc->hMFCMpeg4Handle.pEncOps;
    ExynosVideoEncBufferOps *pOutbufOps = pMpeg4Enc->hMFCMpeg4Handle.pOutbufOps;
    ExynosVideoBuffer       *pVideoBuffer;
    ExynosVideoFrameStatusType displayStatus = VIDEO_FRAME_STATUS_UNKNOWN;
    ExynosVideoGeometry bufferGeometry;
    OMX_S32 indexTimestamp = 0;

    FunctionIn();

    if (pMpeg4Enc->bDestinationStart == OMX_FALSE) {
        ret = OMX_ErrorNone;
        goto EXIT;
    }

    if ((pVideoBuffer = pOutbufOps->Dequeue(hMFCHandle)) == NULL) {
        ret = OMX_ErrorNone;
        goto EXIT;
    }

    pMpeg4Enc->hMFCMpeg4Handle.outputIndexTimestamp++;
    pMpeg4Enc->hMFCMpeg4Handle.outputIndexTimestamp %= MAX_TIMESTAMP;

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

        pDstOutputData->timeStamp = 0;
        pDstOutputData->nFlags |= OMX_BUFFERFLAG_CODECCONFIG;
        pDstOutputData->nFlags |= OMX_BUFFERFLAG_ENDOFFRAME;
        pVideoEnc->bFirstOutput = OMX_TRUE;
    } else {
        indexTimestamp = pEncOps->Get_FrameTag(pMpeg4Enc->hMFCMpeg4Handle.hMFCHandle);
        if ((indexTimestamp < 0) || (indexTimestamp >= MAX_TIMESTAMP)) {
            pDstOutputData->timeStamp = pExynosComponent->timeStamp[pMpeg4Enc->hMFCMpeg4Handle.outputIndexTimestamp];
            pDstOutputData->nFlags = pExynosComponent->nFlags[pMpeg4Enc->hMFCMpeg4Handle.outputIndexTimestamp];
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

OMX_ERRORTYPE Exynos_Mpeg4Enc_srcInputBufferProcess(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *pSrcInputData)
{
    OMX_ERRORTYPE             ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_MPEG4ENC_HANDLE    *pMpeg4Enc = (EXYNOS_MPEG4ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
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

    ret = Exynos_Mpeg4Enc_SrcIn(pOMXComponent, pSrcInputData);
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

OMX_ERRORTYPE Exynos_Mpeg4Enc_srcOutputBufferProcess(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *pSrcOutputData)
{
    OMX_ERRORTYPE             ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_MPEG4ENC_HANDLE    *pMpeg4Enc = (EXYNOS_MPEG4ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
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
    if ((pMpeg4Enc->bSourceStart == OMX_FALSE) &&
       (!CHECK_PORT_BEING_FLUSHED(pExynosInputPort))) {
        Exynos_OSAL_SignalWait(pMpeg4Enc->hSourceStartEvent, DEF_MAX_WAIT_TIME);
        Exynos_OSAL_SignalReset(pMpeg4Enc->hSourceStartEvent);
    }

    ret = Exynos_Mpeg4Enc_SrcOut(pOMXComponent, pSrcOutputData);
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

OMX_ERRORTYPE Exynos_Mpeg4Enc_dstInputBufferProcess(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *pDstInputData)
{
    OMX_ERRORTYPE             ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_MPEG4ENC_HANDLE    *pMpeg4Enc = (EXYNOS_MPEG4ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
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
        if ((pMpeg4Enc->bDestinationStart == OMX_FALSE) &&
           (!CHECK_PORT_BEING_FLUSHED(pExynosOutputPort))) {
            Exynos_OSAL_SignalWait(pMpeg4Enc->hDestinationStartEvent, DEF_MAX_WAIT_TIME);
            Exynos_OSAL_SignalReset(pMpeg4Enc->hDestinationStartEvent);
        }
    }
    if (pMpeg4Enc->hMFCMpeg4Handle.bConfiguredMFCDst == OMX_TRUE) {
        ret = Exynos_Mpeg4Enc_DstIn(pOMXComponent, pDstInputData);
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

OMX_ERRORTYPE Exynos_Mpeg4Enc_dstOutputBufferProcess(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *pDstOutputData)
{
    OMX_ERRORTYPE             ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_MPEG4ENC_HANDLE    *pMpeg4Enc = (EXYNOS_MPEG4ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
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
        if ((pMpeg4Enc->bDestinationStart == OMX_FALSE) &&
           (!CHECK_PORT_BEING_FLUSHED(pExynosOutputPort))) {
            Exynos_OSAL_SignalWait(pMpeg4Enc->hDestinationStartEvent, DEF_MAX_WAIT_TIME);
            Exynos_OSAL_SignalReset(pMpeg4Enc->hDestinationStartEvent);
        }
    }
    ret = Exynos_Mpeg4Enc_DstOut(pOMXComponent, pDstOutputData);
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
    EXYNOS_MPEG4ENC_HANDLE        *pMpeg4Enc        = NULL;
    OMX_S32                        codecType        = -1;
    int i = 0;

    FunctionIn();

    if ((hComponent == NULL) || (componentName == NULL)) {
        ret = OMX_ErrorBadParameter;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: parameters are null, ret: %X", __FUNCTION__, ret);
        goto EXIT;
    }
    if (Exynos_OSAL_Strcmp(EXYNOS_OMX_COMPONENT_MPEG4_ENC, componentName) == 0) {
        codecType = CODEC_TYPE_MPEG4;
    } else if (Exynos_OSAL_Strcmp(EXYNOS_OMX_COMPONENT_H263_ENC, componentName) == 0) {
        codecType = CODEC_TYPE_H263;
    } else {
        ret = OMX_ErrorBadParameter;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: componentName(%s) error, ret: %X", __FUNCTION__, componentName, ret);
        goto EXIT;
    }

    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = Exynos_OMX_VideoEncodeComponentInit(pOMXComponent);
    if (ret != OMX_ErrorNone) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: Exynos_OMX_VideoDecodeComponentInit error, ret: %X", __FUNCTION__, ret);
        goto EXIT;
    }
    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    pExynosComponent->codecType = HW_VIDEO_ENC_CODEC;

    pExynosComponent->componentName = (OMX_STRING)Exynos_OSAL_Malloc(MAX_OMX_COMPONENT_NAME_SIZE);
    if (pExynosComponent->componentName == NULL) {
        Exynos_OMX_VideoEncodeComponentDeinit(pOMXComponent);
        ret = OMX_ErrorInsufficientResources;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: componentName alloc error, ret: %X", __FUNCTION__, ret);
        goto EXIT;
    }
    Exynos_OSAL_Memset(pExynosComponent->componentName, 0, MAX_OMX_COMPONENT_NAME_SIZE);

    pMpeg4Enc = Exynos_OSAL_Malloc(sizeof(EXYNOS_MPEG4ENC_HANDLE));
    if (pMpeg4Enc == NULL) {
        Exynos_OMX_VideoEncodeComponentDeinit(pOMXComponent);
        ret = OMX_ErrorInsufficientResources;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: EXYNOS_MPEG4ENC_HANDLE alloc error, ret: %X", __FUNCTION__, ret);
        goto EXIT;
    }
    Exynos_OSAL_Memset(pMpeg4Enc, 0, sizeof(EXYNOS_MPEG4ENC_HANDLE));
    pVideoEnc = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    pVideoEnc->hCodecHandle = (OMX_HANDLETYPE)pMpeg4Enc;
    pMpeg4Enc->hMFCMpeg4Handle.codecType = codecType;

    if (codecType == CODEC_TYPE_MPEG4)
        Exynos_OSAL_Strcpy(pExynosComponent->componentName, EXYNOS_OMX_COMPONENT_MPEG4_ENC);
    else
        Exynos_OSAL_Strcpy(pExynosComponent->componentName, EXYNOS_OMX_COMPONENT_H263_ENC);

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
    pExynosPort->portDefinition.format.video.nBitrate = 64000;
    pExynosPort->portDefinition.format.video.xFramerate= (15 << 16);
    pExynosPort->portDefinition.nBufferSize = DEFAULT_VIDEO_INPUT_BUFFER_SIZE;
    pExynosPort->portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
    pExynosPort->portDefinition.format.video.pNativeRender = 0;
    pExynosPort->portDefinition.format.video.bFlagErrorConcealment = OMX_FALSE;
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
    pExynosPort->portDefinition.format.video.nBitrate = 64000;
    pExynosPort->portDefinition.format.video.xFramerate= (15 << 16);
    pExynosPort->portDefinition.nBufferSize = DEFAULT_VIDEO_OUTPUT_BUFFER_SIZE;
    if (codecType == CODEC_TYPE_MPEG4) {
        pExynosPort->portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingMPEG4;
        Exynos_OSAL_Memset(pExynosPort->portDefinition.format.video.cMIMEType, 0, MAX_OMX_MIMETYPE_SIZE);
        Exynos_OSAL_Strcpy(pExynosPort->portDefinition.format.video.cMIMEType, "video/mpeg4");
    } else {
        pExynosPort->portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingH263;
        Exynos_OSAL_Memset(pExynosPort->portDefinition.format.video.cMIMEType, 0, MAX_OMX_MIMETYPE_SIZE);
        Exynos_OSAL_Strcpy(pExynosPort->portDefinition.format.video.cMIMEType, "video/h263");
    }
    pExynosPort->portDefinition.format.video.pNativeRender = 0;
    pExynosPort->portDefinition.format.video.bFlagErrorConcealment = OMX_FALSE;
    pExynosPort->portDefinition.format.video.eColorFormat = OMX_COLOR_FormatUnused;
    pExynosPort->portDefinition.bEnabled = OMX_TRUE;
    pExynosPort->bufferProcessType = BUFFER_SHARE;
    pExynosPort->portWayType = WAY2_PORT;

    if (codecType == CODEC_TYPE_MPEG4) {
        for(i = 0; i < ALL_PORT_NUM; i++) {
            INIT_SET_SIZE_VERSION(&pMpeg4Enc->mpeg4Component[i], OMX_VIDEO_PARAM_MPEG4TYPE);
            pMpeg4Enc->mpeg4Component[i].nPortIndex = i;
            pMpeg4Enc->mpeg4Component[i].eProfile   = OMX_VIDEO_MPEG4ProfileSimple;
            pMpeg4Enc->mpeg4Component[i].eLevel     = OMX_VIDEO_MPEG4Level4;

            pMpeg4Enc->mpeg4Component[i].nPFrames = 10;
            pMpeg4Enc->mpeg4Component[i].nBFrames = 0;          /* No support for B frames */
            pMpeg4Enc->mpeg4Component[i].nMaxPacketSize = 256;  /* Default value */
            pMpeg4Enc->mpeg4Component[i].nAllowedPictureTypes =  OMX_VIDEO_PictureTypeI | OMX_VIDEO_PictureTypeP;
            pMpeg4Enc->mpeg4Component[i].bGov = OMX_FALSE;

        }
    } else {
        for(i = 0; i < ALL_PORT_NUM; i++) {
            INIT_SET_SIZE_VERSION(&pMpeg4Enc->h263Component[i], OMX_VIDEO_PARAM_H263TYPE);
            pMpeg4Enc->h263Component[i].nPortIndex = i;
            pMpeg4Enc->h263Component[i].eProfile   = OMX_VIDEO_H263ProfileBaseline;
            pMpeg4Enc->h263Component[i].eLevel     = OMX_VIDEO_H263Level45;

            pMpeg4Enc->h263Component[i].nPFrames = 20;
            pMpeg4Enc->h263Component[i].nBFrames = 0;          /* No support for B frames */
            pMpeg4Enc->h263Component[i].bPLUSPTYPEAllowed = OMX_FALSE;
            pMpeg4Enc->h263Component[i].nAllowedPictureTypes = OMX_VIDEO_PictureTypeI | OMX_VIDEO_PictureTypeP;
            pMpeg4Enc->h263Component[i].bForceRoundingTypeToZero = OMX_TRUE;
            pMpeg4Enc->h263Component[i].nPictureHeaderRepetition = 0;
            pMpeg4Enc->h263Component[i].nGOBHeaderInterval = 0;
        }
    }

    pOMXComponent->GetParameter      = &Exynos_Mpeg4Enc_GetParameter;
    pOMXComponent->SetParameter      = &Exynos_Mpeg4Enc_SetParameter;
    pOMXComponent->GetConfig         = &Exynos_Mpeg4Enc_GetConfig;
    pOMXComponent->SetConfig         = &Exynos_Mpeg4Enc_SetConfig;
    pOMXComponent->GetExtensionIndex = &Exynos_Mpeg4Enc_GetExtensionIndex;
    pOMXComponent->ComponentRoleEnum = &Exynos_Mpeg4Enc_ComponentRoleEnum;
    pOMXComponent->ComponentDeInit   = &Exynos_OMX_ComponentDeinit;

    pExynosComponent->exynos_codec_componentInit      = &Exynos_Mpeg4Enc_Init;
    pExynosComponent->exynos_codec_componentTerminate = &Exynos_Mpeg4Enc_Terminate;

    pVideoEnc->exynos_codec_srcInputProcess  = &Exynos_Mpeg4Enc_srcInputBufferProcess;
    pVideoEnc->exynos_codec_srcOutputProcess = &Exynos_Mpeg4Enc_srcOutputBufferProcess;
    pVideoEnc->exynos_codec_dstInputProcess  = &Exynos_Mpeg4Enc_dstInputBufferProcess;
    pVideoEnc->exynos_codec_dstOutputProcess = &Exynos_Mpeg4Enc_dstOutputBufferProcess;

    pVideoEnc->exynos_codec_start         = &Mpeg4CodecStart;
    pVideoEnc->exynos_codec_stop          = &Mpeg4CodecStop;
    pVideoEnc->exynos_codec_bufferProcessRun = &Mpeg4CodecOutputBufferProcessRun;
    pVideoEnc->exynos_codec_enqueueAllBuffer = &Mpeg4CodecEnQueueAllBuffer;

    pVideoEnc->exynos_checkInputFrame        = NULL;
    pVideoEnc->exynos_codec_getCodecInputPrivateData  = &GetCodecInputPrivateData;
    pVideoEnc->exynos_codec_getCodecOutputPrivateData = &GetCodecOutputPrivateData;

    pVideoEnc->hSharedMemory = Exynos_OSAL_SharedMemory_Open();
    if (pVideoEnc->hSharedMemory == NULL) {
        Exynos_OSAL_Free(pMpeg4Enc);
        pMpeg4Enc = ((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle = NULL;
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
    OMX_ERRORTYPE               ret              = OMX_ErrorNone;
    OMX_COMPONENTTYPE          *pOMXComponent    = NULL;
    EXYNOS_OMX_BASECOMPONENT   *pExynosComponent = NULL;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc = NULL;
    EXYNOS_MPEG4ENC_HANDLE     *pMpeg4Enc        = NULL;

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

    pMpeg4Enc = (EXYNOS_MPEG4ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
    if (pMpeg4Enc != NULL) {
        Exynos_OSAL_Free(pMpeg4Enc);
        pMpeg4Enc = ((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle = NULL;
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
