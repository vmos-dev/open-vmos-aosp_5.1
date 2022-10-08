
/*
 * Copyright (C) Texas Instruments - http://www.ti.com/
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
/* =============================================================================
*             Texas Instruments OMAP(TM) Platform Software
*  (c) Copyright Texas Instruments, Incorporated.  All Rights Reserved.
*
*  Use of this software is controlled by the terms and conditions found
*  in the license agreement under which this software has been supplied.
* ============================================================================*/
/**
* @file OMX_VideoEncoder.c
*
* This file implements OMX Component for MPEG-4 encoder that
* is fully compliant with the OMX specification 1.5.
*
* @path  $(CSLPATH)\src
*
* @rev  0.1
*/
/* ---------------------------------------------------------------------------*/
/* =============================================================================
*!
*! Revision History
*! =============================================================================
*!
*! 24-Jul-2005 mf: Revisions appear in reverse chronological order;
*! that is, newest first.  The date format is dd-Mon-yyyy.
* ============================================================================*/

/* ------compilation control switches ----------------------------------------*/
/******************************************************************************
*  INCLUDE FILES
*******************************************************************************/
/* ----- system and platform files -------------------------------------------*/
#ifdef UNDER_CE
    #include <windows.h>
    #include <oaf_osal.h>
    #include <omx_core.h>
#else
    #include <wchar.h>
    #include <unistd.h>
    #include <sys/time.h>
    #include <sys/types.h>
    #include <sys/ioctl.h>
    #include <sys/select.h>
    #include <errno.h>
    #include <pthread.h>
    #include <dlfcn.h>
#endif

#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <dbapi.h>

/*------- Program Header Files -----------------------------------------------*/
#include "OMX_VideoEnc_Utils.h"
#include "OMX_VideoEnc_DSP.h"
#include "OMX_VideoEnc_Thread.h"

#ifdef RESOURCE_MANAGER_ENABLED
    #include <ResourceManagerProxyAPI.h>
#endif
#ifdef UNDER_CE
    extern HINSTANCE g_hLcmlDllHandle;
#endif
/* H.263 Supported Levels & profiles */
VIDEO_PROFILE_LEVEL_TYPE SupportedH263ProfileLevels[] = {
  {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level10},
  {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level20},
  {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level30},
  {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level40},
  {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level45},
  {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level50},
  {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level60},
  {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level70},
  {-1, -1}};

/* MPEG4 Supported Levels & profiles */
VIDEO_PROFILE_LEVEL_TYPE SupportedMPEG4ProfileLevels[] ={
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
  {OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level5},
  {-1,-1}};

/* AVC Supported Levels & profiles */
VIDEO_PROFILE_LEVEL_TYPE SupportedAVCProfileLevels[] ={
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
  {-1,-1}};
/******************************************************************************
*  EXTERNAL REFERENCES NOTE : only use if not found in header file
*******************************************************************************/
/*--------data declarations --------------------------------------------------*/
/*--------function prototypes ------------------------------------------------*/

/******************************************************************************
*  PUBLIC DECLARATIONS Defined here, used elsewhere
*******************************************************************************/
/*--------data declarations --------------------------------------------------*/
/*--------function prototypes ------------------------------------------------*/
#ifndef UNDER_CE
    OMX_ERRORTYPE OMX_ComponentInit(OMX_HANDLETYPE hComp);
#else
    #define OMX_EXPORT __declspec(dllexport)
    OMX_EXPORT OMX_ERRORTYPE OMX_ComponentInit (OMX_HANDLETYPE hComp);
#endif

/******************************************************************************
*  PRIVATE DECLARATIONS Defined here, used only here
*******************************************************************************/
/*--------data declarations --------------------------------------------------*/
#ifdef UNDER_CE
           static pthread_t ComponentThread;
#endif
/*--------macro definitions --------------------------------------------------*/
#define OMX_CONVERT_STATE(_s_, _p_)        \
    if (_p_ == 0) {                        \
        _s_ = "OMX_StateInvalid";          \
    }                                      \
    else if (_p_ == 1) {                   \
        _s_ = "OMX_StateLoaded";           \
    }                                      \
    else if (_p_ == 2) {                   \
        _s_ = "OMX_StateIdle";             \
    }                                      \
    else if (_p_ == 3) {                   \
        _s_ = "OMX_StateExecuting";        \
    }                                      \
    else if (_p_ == 4) {                   \
        _s_ = "OMX_StatePause";            \
    }                                      \
    else if (_p_ == 5) {                   \
        _s_ = "OMX_StateWaitForResources"; \
    }                                      \
    else {                                 \
        _s_ = "UnsupportedCommand";        \
    }

#define OMX_CONVERT_CMD(_s_, _p_)          \
    if (_p_ == 0) {                        \
        _s_ = "OMX_CommandStateSet";       \
    }                                      \
    else if (_p_ == 1) {                   \
        _s_ = "OMX_CommandFlush";          \
    }                                      \
    else if (_p_ == 2) {                   \
        _s_ = "OMX_CommandPortDisable";    \
    }                                      \
    else if (_p_ == 3) {                   \
        _s_ = "OMX_CommandPortEnable";     \
    }                                      \
    else if (_p_ == 4) {                   \
        _s_ = "OMX_CommandMarkBuffer";     \
    }                                      \
    else {                                 \
        _s_ = "UnsupportedCommand";        \
    }

/*--------function prototypes ------------------------------------------------*/
static OMX_ERRORTYPE SetCallbacks (OMX_IN OMX_HANDLETYPE hComponent,
                                   OMX_IN OMX_CALLBACKTYPE* pCallBacks,
                                   OMX_IN OMX_PTR pAppData);

static OMX_ERRORTYPE GetComponentVersion (OMX_HANDLETYPE hComponent,
                                          OMX_STRING  szComponentName,
                                          OMX_VERSIONTYPE* pComponentVersion,
                                          OMX_VERSIONTYPE* pSpecVersion,
                                          OMX_UUIDTYPE* pComponentUUID);

static OMX_ERRORTYPE SendCommand (OMX_IN OMX_HANDLETYPE hComponent,
                                  OMX_IN OMX_COMMANDTYPE Cmd,
                                  OMX_IN OMX_U32 nParam1,
                                  OMX_IN OMX_PTR pCmdData);

static OMX_ERRORTYPE GetParameter (OMX_IN OMX_HANDLETYPE hComponent,
                                   OMX_IN OMX_INDEXTYPE nParamIndex,
                                   OMX_INOUT OMX_PTR CompParamStruct);

static OMX_ERRORTYPE SetParameter (OMX_IN OMX_HANDLETYPE hComponent,
                                   OMX_IN OMX_INDEXTYPE nParamIndex,
                                   OMX_IN OMX_PTR CompParamStruct);

static OMX_ERRORTYPE GetConfig (OMX_HANDLETYPE hComponent,
                                OMX_INDEXTYPE nConfigIndex,
                                OMX_PTR ComponentConfigStructure);

static OMX_ERRORTYPE SetConfig (OMX_HANDLETYPE hComponent,
                                OMX_INDEXTYPE nConfigIndex,
                                OMX_PTR ComponentConfigStructure);

static OMX_ERRORTYPE EmptyThisBuffer (OMX_IN OMX_HANDLETYPE hComponent,
                                      OMX_IN OMX_BUFFERHEADERTYPE* pBuffer);

static OMX_ERRORTYPE FillThisBuffer (OMX_IN OMX_HANDLETYPE hComponent,
                                     OMX_IN OMX_BUFFERHEADERTYPE* pBuffer);

static OMX_ERRORTYPE GetState (OMX_IN OMX_HANDLETYPE hComponent,
                               OMX_OUT OMX_STATETYPE* pState);

static OMX_ERRORTYPE ComponentTunnelRequest (OMX_IN OMX_HANDLETYPE hComponent,
                                             OMX_IN OMX_U32 nPort,
                                             OMX_IN OMX_HANDLETYPE hTunneledComp,
                                             OMX_IN  OMX_U32 nTunneledPort,
                                             OMX_INOUT OMX_TUNNELSETUPTYPE* pTunnelSetup);

static OMX_ERRORTYPE UseBuffer (OMX_IN OMX_HANDLETYPE hComponent,
                                OMX_INOUT OMX_BUFFERHEADERTYPE** ppBufferHdr,
                                OMX_IN OMX_U32 nPortIndex,
                                OMX_IN OMX_PTR pAppPrivate,
                                OMX_IN OMX_U32 nSizeBytes,
                                OMX_IN OMX_U8* pBuffer);

static OMX_ERRORTYPE AllocateBuffer (OMX_IN OMX_HANDLETYPE hComponent,
                                     OMX_INOUT OMX_BUFFERHEADERTYPE** pBuffer,
                                     OMX_IN OMX_U32 nPortIndex,
                                     OMX_IN OMX_PTR pAppPrivate,
                                     OMX_IN OMX_U32 nSizeBytes);

static OMX_ERRORTYPE FreeBuffer (OMX_IN OMX_HANDLETYPE hComponent,
                                 OMX_IN OMX_U32 nPortIndex,
                                 OMX_IN OMX_BUFFERHEADERTYPE* pBuffer);

static OMX_ERRORTYPE ComponentDeInit (OMX_IN OMX_HANDLETYPE hComponent);

static OMX_ERRORTYPE VerifyTunnelConnection (VIDEOENC_PORT_TYPE* pPort,
                                             OMX_HANDLETYPE hTunneledComp,
                                             OMX_PARAM_PORTDEFINITIONTYPE* pPortDef,
                                             struct OMX_TI_Debug *dbg);

static OMX_ERRORTYPE ExtensionIndex (OMX_IN OMX_HANDLETYPE hComponent,
                                     OMX_IN OMX_STRING cParameterName,
                                     OMX_OUT OMX_INDEXTYPE* pIndexType);

#ifdef __KHRONOS_CONF_1_1__
static OMX_ERRORTYPE ComponentRoleEnum(OMX_IN OMX_HANDLETYPE hComponent,
                                       OMX_OUT OMX_U8 *cRole,
                                       OMX_IN OMX_U32 nIndex);
#endif

void CalculateBufferSize(OMX_PARAM_PORTDEFINITIONTYPE* pCompPort, VIDENC_COMPONENT_PRIVATE* pCompPrivate);
OMX_ERRORTYPE IsResolutionPlayable (OMX_U32 width, OMX_U32 height);

static const int iQ16_Const = 1 << 16;
static const float fQ16_Const = (float)(1 << 16);

static float Q16Tof(int nQ16)
{
    return nQ16 / fQ16_Const;
}

static int fToQ16(float f)
{
    return(int)(f*fQ16_Const);
}

extern OMX_U32 VIDENC_STRUCT_H264DEFBITRATE [VIDENC_MAXBITRATES][2];
extern OMX_U32 VIDENC_STRUCT_MPEG4DEFBITRATE [VIDENC_MAXBITRATES][2];
extern OMX_U32 VIDENC_STRUCT_H263DEFBITRATE [VIDENC_MAXBITRATES][2];

/*----------------------------------------------------------------------------*/
/**
  * OMX_ComponentInit() Set the all the function pointers of component
  *
  * This method will update the component function pointer to the handle
  *
  * @param hComp         handle for this instance of the component
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_ErrorInsufficientResources If the malloc fails
  **/
/*----------------------------------------------------------------------------*/

#ifdef UNDER_CE
OMX_EXPORT
#endif
OMX_ERRORTYPE OMX_ComponentInit (OMX_HANDLETYPE hComponent)
{
    OMX_COMPONENTTYPE* pHandle                  = NULL;
    OMX_ERRORTYPE eError                        = OMX_ErrorNone;
    VIDENC_COMPONENT_PRIVATE* pComponentPrivate = NULL;
    VIDEOENC_PORT_TYPE* pCompPortIn             = NULL;
    VIDEOENC_PORT_TYPE* pCompPortOut            = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDef      = NULL;
    OMX_VIDEO_PARAM_PORTFORMATTYPE* pPortFormat = NULL;
    OMX_PRIORITYMGMTTYPE* pPriorityMgmt         = NULL;
    VIDENC_NODE* pMemoryListHead                = NULL;
    struct OMX_TI_Debug dbg;

    OMX_S32 nError = 0;
    OMX_U32 i = 0;
    char* sDynamicFormat;
#ifdef UNDER_CE
    pthread_attr_t attr;
    memset(&attr, 0, sizeof(attr));
#endif


    /* get default settings for debug */
    OMX_DBG_INIT(dbg, "OMX_DBG_VIDENC");

    /*dlopen("libLCML.so", RTLD_LAZY);*/
    OMX_PRINT2(dbg, "Enter to ComponetInit\n");
    if (!hComponent)
    {
        eError = OMX_ErrorBadParameter;
        goto OMX_CONF_CMD_BAIL;
    }
    pHandle = (OMX_COMPONENTTYPE*)hComponent;
    eError = OMX_VIDENC_ListCreate(&dbg, &pMemoryListHead);
    OMX_CONF_BAIL_IF_ERROR(eError);

    /* Allocate memory for component's private data area */
    VIDENC_MALLOC(pHandle->pComponentPrivate,
                  sizeof(VIDENC_COMPONENT_PRIVATE),
                  VIDENC_COMPONENT_PRIVATE,
                  pMemoryListHead, dbg);

    pComponentPrivate = (VIDENC_COMPONENT_PRIVATE*)pHandle->pComponentPrivate;
    pComponentPrivate->pMemoryListHead = pMemoryListHead;

    pComponentPrivate->dbg = dbg;
    pComponentPrivate->compressionFormats[0]=OMX_VIDEO_CodingAVC;
    pComponentPrivate->compressionFormats[1]=OMX_VIDEO_CodingMPEG4;
    pComponentPrivate->compressionFormats[2]=OMX_VIDEO_CodingH263;
    pComponentPrivate->colorFormats[0]=OMX_COLOR_FormatYUV420Planar;
    pComponentPrivate->colorFormats[1]=OMX_COLOR_FormatYCbYCr;
    pComponentPrivate->colorFormats[2]=OMX_COLOR_FormatCbYCrY;

#ifdef __PERF_INSTRUMENTATION__
    pComponentPrivate->pPERF = PERF_Create(PERF_FOURCC('V','E',' ',' '),
                                           PERF_ModuleLLMM |
                                           PERF_ModuleVideoEncode);
#endif

    pComponentPrivate->bDeblockFilter       = OMX_TRUE;
    pComponentPrivate->nVBVSize             = 120;
    pComponentPrivate->bForceIFrame         = OMX_FALSE;
    pComponentPrivate->nIntraFrameInterval  = 30;
    pComponentPrivate->nQPI                 = 12;
    pComponentPrivate->nAIRRate             = 0;
    pComponentPrivate->ucUnrestrictedMV     = 0;
    pComponentPrivate->bHideEvents          = OMX_FALSE;
    pComponentPrivate->bHandlingFatalError  = OMX_FALSE;
    pComponentPrivate->bUnresponsiveDsp     = OMX_FALSE;
    pComponentPrivate->bCodecLoaded         = OMX_FALSE;
    pComponentPrivate->cComponentName       = "OMX.TI.Video.encoder";
    pComponentPrivate->sps = NULL;
    pComponentPrivate->spsLen = 0;

#ifdef __KHRONOS_CONF__
    pComponentPrivate->bPassingIdleToLoaded = OMX_FALSE;
    pComponentPrivate->bErrorLcmlHandle     = OMX_FALSE;
#endif

    /*Initialize Circular Buffer*/
    OMX_CONF_CIRCULAR_BUFFER_INIT(pComponentPrivate);

    /* ASO/FMO*/
    pComponentPrivate->numSliceASO = 0;
    for( i=0; i<MAXNUMSLCGPS;i++)
    {
        pComponentPrivate->asoSliceOrder[i] = 0;
    }
    pComponentPrivate->numSliceGroups                = 0;
    pComponentPrivate->sliceGroupMapType             = 0;
    pComponentPrivate->sliceGroupChangeDirectionFlag = 0;
    pComponentPrivate->sliceGroupChangeRate          = 0;
    pComponentPrivate->sliceGroupChangeCycle         = 0;
    for( i=0; i<MAXNUMSLCGPS;i++)
    {
        pComponentPrivate->sliceGroupParams[i] = 0;
    }

    /*Assigning address of Component Structure point to place holder inside
       component private structure
    */
    ((VIDENC_COMPONENT_PRIVATE*)pHandle->pComponentPrivate)->pHandle = pHandle;

    /* fill in function pointers */
    pHandle->SetCallbacks           = SetCallbacks;
    pHandle->GetComponentVersion    = GetComponentVersion;
    pHandle->SendCommand            = SendCommand;
    pHandle->GetParameter           = GetParameter;
    pHandle->SetParameter           = SetParameter;
    pHandle->GetConfig              = GetConfig;
    pHandle->SetConfig              = SetConfig;
    pHandle->GetExtensionIndex      = ExtensionIndex;
    pHandle->GetState               = GetState;
    pHandle->ComponentTunnelRequest = ComponentTunnelRequest;
    pHandle->UseBuffer              = UseBuffer;
    pHandle->AllocateBuffer         = AllocateBuffer;
    pHandle->FreeBuffer             = FreeBuffer;
    pHandle->EmptyThisBuffer        = EmptyThisBuffer;
    pHandle->FillThisBuffer         = FillThisBuffer;
    pHandle->ComponentDeInit        = ComponentDeInit;
#ifdef __KHRONOS_CONF_1_1__
    pHandle->ComponentRoleEnum      = ComponentRoleEnum;

sDynamicFormat = getenv("FORMAT");
/*printf("\n ** FORMAT = %s\n\n", sDynamicFormat);*/
#if 1
    if (sDynamicFormat != NULL)
    {
        if ( strcmp(sDynamicFormat,  "MPEG4") == 0 )
        {

    strcpy((char *)pComponentPrivate->componentRole.cRole, "video_encoder.mpeg4");
        }
        else if (strcmp(sDynamicFormat,  "H263") == 0 )
        {
    strcpy((char *)pComponentPrivate->componentRole.cRole, "video_encoder.h263");
        }
        else if (strcmp(sDynamicFormat,  "H264") == 0 )
        {
    strcpy((char *)pComponentPrivate->componentRole.cRole, "video_encoder.avc");
        }
        }
    else
    {
    strcpy((char *)pComponentPrivate->componentRole.cRole, "video_encoder.avc");
        }
#else
    strcpy((char *)pComponentPrivate->componentRole.cRole, "VideoEncode");
/*    strcpy((char *)pComponentPrivate->componentRole.cRole, "video_encoder.mpeg4");*/
#endif
#endif

    /* Allocate memory for component data structures */
    VIDENC_MALLOC(pComponentPrivate->pPortParamType,
                  sizeof(OMX_PORT_PARAM_TYPE),
                  OMX_PORT_PARAM_TYPE,
                  pMemoryListHead, dbg);
#ifdef __KHRONOS_CONF_1_1__
    VIDENC_MALLOC(pComponentPrivate->pPortAudioType,
                  sizeof(OMX_PORT_PARAM_TYPE),
                  OMX_PORT_PARAM_TYPE,
                  pMemoryListHead, dbg);
    VIDENC_MALLOC(pComponentPrivate->pPortImageType,
              sizeof(OMX_PORT_PARAM_TYPE),
              OMX_PORT_PARAM_TYPE,
                  pMemoryListHead, dbg);
    VIDENC_MALLOC(pComponentPrivate->pPortOtherType,
                  sizeof(OMX_PORT_PARAM_TYPE),
                  OMX_PORT_PARAM_TYPE,
                  pMemoryListHead, dbg);
#endif


    VIDENC_MALLOC(pComponentPrivate->pCompPort[VIDENC_INPUT_PORT],
                  sizeof(VIDEOENC_PORT_TYPE),
                  VIDEOENC_PORT_TYPE,
                  pMemoryListHead, dbg);
    VIDENC_MALLOC(pComponentPrivate->pCompPort[VIDENC_OUTPUT_PORT],
                  sizeof(VIDEOENC_PORT_TYPE),
                  VIDEOENC_PORT_TYPE,
                  pMemoryListHead, dbg);
    VIDENC_MALLOC(pComponentPrivate->pCompPort[VIDENC_INPUT_PORT]->pPortDef,
                  sizeof(OMX_PARAM_PORTDEFINITIONTYPE),
                  OMX_PARAM_PORTDEFINITIONTYPE,
                  pMemoryListHead, dbg);
    VIDENC_MALLOC(pComponentPrivate->pCompPort[VIDENC_OUTPUT_PORT]->pPortDef,
                  sizeof(OMX_PARAM_PORTDEFINITIONTYPE),
                  OMX_PARAM_PORTDEFINITIONTYPE,
                  pMemoryListHead, dbg);
    VIDENC_MALLOC(pComponentPrivate->pCompPort[VIDENC_INPUT_PORT]->pPortFormat,
                  sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE),
                  OMX_VIDEO_PARAM_PORTFORMATTYPE,
                  pMemoryListHead, dbg);
    VIDENC_MALLOC(pComponentPrivate->pCompPort[VIDENC_OUTPUT_PORT]->pPortFormat,
                  sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE),
                  OMX_VIDEO_PARAM_PORTFORMATTYPE,
                  pMemoryListHead, dbg);
#ifdef __KHRONOS_CONF_1_1__
    VIDENC_MALLOC(pComponentPrivate->pCompPort[VIDENC_INPUT_PORT]->pProfileType,
                  sizeof(OMX_VIDEO_PARAM_PROFILELEVELTYPE),
                  OMX_VIDEO_PARAM_PROFILELEVELTYPE,
                  pMemoryListHead, dbg);
    VIDENC_MALLOC(pComponentPrivate->pCompPort[VIDENC_OUTPUT_PORT]->pProfileType,
                  sizeof(OMX_VIDEO_PARAM_PROFILELEVELTYPE),
                  OMX_VIDEO_PARAM_PROFILELEVELTYPE,
                  pMemoryListHead, dbg);
    VIDENC_MALLOC(pComponentPrivate->pCompPort[VIDENC_INPUT_PORT]->pBitRateTypeConfig,
                  sizeof(OMX_VIDEO_CONFIG_BITRATETYPE),
                  OMX_VIDEO_CONFIG_BITRATETYPE,
                  pMemoryListHead, dbg);
    VIDENC_MALLOC(pComponentPrivate->pCompPort[VIDENC_OUTPUT_PORT]->pBitRateTypeConfig,
                  sizeof(OMX_VIDEO_CONFIG_BITRATETYPE),
                  OMX_VIDEO_CONFIG_BITRATETYPE,
                  pMemoryListHead, dbg);
    VIDENC_MALLOC(pComponentPrivate->pCompPort[VIDENC_INPUT_PORT]->pFrameRateConfig,
                  sizeof(OMX_CONFIG_FRAMERATETYPE),
                  OMX_CONFIG_FRAMERATETYPE,
                  pMemoryListHead, dbg);
    VIDENC_MALLOC(pComponentPrivate->pCompPort[VIDENC_OUTPUT_PORT]->pFrameRateConfig,
                  sizeof(OMX_CONFIG_FRAMERATETYPE),
                  OMX_CONFIG_FRAMERATETYPE,
                  pMemoryListHead, dbg);
    VIDENC_MALLOC(pComponentPrivate->pCompPort[VIDENC_OUTPUT_PORT]->pErrorCorrectionType,
                  sizeof(OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE),
                  OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE,
                  pMemoryListHead, dbg);
    VIDENC_MALLOC(pComponentPrivate->pCompPort[VIDENC_OUTPUT_PORT]->pIntraRefreshType,
                  sizeof(OMX_VIDEO_PARAM_INTRAREFRESHTYPE),
                  OMX_VIDEO_PARAM_INTRAREFRESHTYPE,
                  pMemoryListHead, dbg);
#endif
    VIDENC_MALLOC(pComponentPrivate->pCompPort[VIDENC_INPUT_PORT]->pBitRateType,
                  sizeof(OMX_VIDEO_PARAM_BITRATETYPE),
                  OMX_VIDEO_PARAM_BITRATETYPE,
                  pMemoryListHead, dbg);
    VIDENC_MALLOC(pComponentPrivate->pCompPort[VIDENC_OUTPUT_PORT]->pBitRateType,
                  sizeof(OMX_VIDEO_PARAM_BITRATETYPE),
                  OMX_VIDEO_PARAM_BITRATETYPE,
                  pMemoryListHead, dbg);
    VIDENC_MALLOC(pComponentPrivate->pPriorityMgmt,
                  sizeof(OMX_PRIORITYMGMTTYPE),
                  OMX_PRIORITYMGMTTYPE,
                  pMemoryListHead, dbg);
    VIDENC_MALLOC(pComponentPrivate->pH264,
                  sizeof(OMX_VIDEO_PARAM_AVCTYPE),
                  OMX_VIDEO_PARAM_AVCTYPE,
                  pMemoryListHead, dbg);
    VIDENC_MALLOC(pComponentPrivate->pMpeg4,
                  sizeof(OMX_VIDEO_PARAM_MPEG4TYPE),
                  OMX_VIDEO_PARAM_MPEG4TYPE,
                  pMemoryListHead, dbg);
    VIDENC_MALLOC(pComponentPrivate->pH263,
                  sizeof(OMX_VIDEO_PARAM_H263TYPE),
                  OMX_VIDEO_PARAM_H263TYPE,
                  pMemoryListHead, dbg);
    VIDENC_MALLOC(pComponentPrivate->pVidParamBitrate, 
                  sizeof(OMX_VIDEO_PARAM_BITRATETYPE), 
                  OMX_VIDEO_PARAM_BITRATETYPE, 
                  pMemoryListHead, dbg);
    VIDENC_MALLOC(pComponentPrivate->pQuantization,
                  sizeof(OMX_VIDEO_PARAM_QUANTIZATIONTYPE),
                  OMX_VIDEO_PARAM_QUANTIZATIONTYPE,
                  pMemoryListHead, dbg);
    VIDENC_MALLOC(pComponentPrivate->pH264IntraPeriod,
                  sizeof(OMX_VIDEO_CONFIG_AVCINTRAPERIOD),
                  OMX_VIDEO_CONFIG_AVCINTRAPERIOD,
                  pMemoryListHead, dbg);
    VIDENC_MALLOC(pComponentPrivate->pMotionVector,
                  sizeof(OMX_VIDEO_PARAM_MOTIONVECTORTYPE),
                  OMX_VIDEO_PARAM_MOTIONVECTORTYPE,
                  pMemoryListHead, dbg);
    VIDENC_MALLOC(pComponentPrivate->pCapabilityFlags,
                  sizeof(PV_OMXComponentCapabilityFlagsType),
                  PV_OMXComponentCapabilityFlagsType,
                  pMemoryListHead, dbg);

    /* Set pPortParamType defaults */
    OMX_CONF_INIT_STRUCT(pComponentPrivate->pPortParamType, OMX_PORT_PARAM_TYPE);
    pComponentPrivate->pPortParamType->nPorts = VIDENC_NUM_OF_PORTS;
    pComponentPrivate->pPortParamType->nStartPortNumber = VIDENC_INPUT_PORT;

#ifdef __KHRONOS_CONF_1_1__
    OMX_CONF_INIT_STRUCT(pComponentPrivate->pPortAudioType, OMX_PORT_PARAM_TYPE);
    pComponentPrivate->pPortAudioType->nPorts = 0;
    pComponentPrivate->pPortAudioType->nStartPortNumber = -1;

    OMX_CONF_INIT_STRUCT(pComponentPrivate->pPortImageType, OMX_PORT_PARAM_TYPE);
    pComponentPrivate->pPortImageType->nPorts = 0;
    pComponentPrivate->pPortImageType->nStartPortNumber = -1;

    OMX_CONF_INIT_STRUCT(pComponentPrivate->pPortOtherType, OMX_PORT_PARAM_TYPE);
    pComponentPrivate->pPortOtherType->nPorts = 0;
    pComponentPrivate->pPortOtherType->nStartPortNumber = -1;

#endif

    pCompPortIn = pComponentPrivate->pCompPort[VIDENC_INPUT_PORT];

    /* Set input port defaults */

    pPortDef = pCompPortIn->pPortDef;
    OMX_CONF_INIT_STRUCT(pPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
    pPortDef->nPortIndex                         = VIDENC_INPUT_PORT;
    pPortDef->eDir                               = OMX_DirInput;
    pPortDef->nBufferCountActual                 = VIDENC_NUM_OF_IN_BUFFERS;
    pPortDef->nBufferCountMin                    = 1;
    pPortDef->nBufferSize                        = 0;    /* It is calculated below */
    pPortDef->bEnabled                           = OMX_TRUE;
    pPortDef->bPopulated                         = OMX_FALSE;
    pPortDef->eDomain                            = OMX_PortDomainVideo;
    pPortDef->format.video.cMIMEType             = "yuv";
    pPortDef->format.video.pNativeRender         = NULL;
    pPortDef->format.video.nFrameWidth           = 176;
    pPortDef->format.video.nFrameHeight          = 144;
    pPortDef->format.video.nStride               = -1;
    pPortDef->format.video.nSliceHeight          = -1;
    pPortDef->format.video.xFramerate            = fToQ16(15.0);
    pPortDef->format.video.bFlagErrorConcealment = OMX_FALSE;
    pPortDef->format.video.eCompressionFormat    = OMX_VIDEO_CodingUnused;
    pPortDef->format.video.eColorFormat          = OMX_COLOR_FormatYUV420Planar;

    /* Set the default value of the run-time Target Frame Rate to the create-time Frame Rate */
    pComponentPrivate->nTargetFrameRate = Q16Tof(pPortDef->format.video.xFramerate);

    CalculateBufferSize(pPortDef, pComponentPrivate);
    pComponentPrivate->nInBufferSize = 0;

    for (i = 0; i < VIDENC_MAX_NUM_OF_IN_BUFFERS; i++)
    {
        VIDENC_MALLOC(pCompPortIn->pBufferPrivate[i],
                      sizeof(VIDENC_BUFFER_PRIVATE),
                      VIDENC_BUFFER_PRIVATE,
                      pMemoryListHead, dbg);
    }
    for (i = 0; i < VIDENC_MAX_NUM_OF_IN_BUFFERS; i++)
    {
        pCompPortIn->pBufferPrivate[i]->pBufferHdr = NULL;
    }
    pCompPortIn->nBufferCnt = 0;

    /* Set output port defaults */
    pCompPortOut = pComponentPrivate->pCompPort[VIDENC_OUTPUT_PORT];
    pPortDef = pComponentPrivate->pCompPort[VIDENC_OUTPUT_PORT]->pPortDef;
    OMX_CONF_INIT_STRUCT(pPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
    pPortDef->nPortIndex                         = VIDENC_OUTPUT_PORT;
    pPortDef->eDir                               = OMX_DirOutput;
    pPortDef->nBufferCountActual                 = VIDENC_NUM_OF_OUT_BUFFERS;
    pPortDef->nBufferCountMin                    = 1;
    pPortDef->nBufferSize                        = 0;  /* It is calculated below */
    pPortDef->bEnabled                           = OMX_TRUE;
    pPortDef->bPopulated                         = OMX_FALSE;
    pPortDef->eDomain                            = OMX_PortDomainVideo;
    pPortDef->format.video.cMIMEType             = "264";
    pPortDef->format.video.pNativeRender         = NULL;
    pPortDef->format.video.nFrameWidth           = 176;
    pPortDef->format.video.nFrameHeight          = 144;
    pPortDef->format.video.nStride               = -1;
    pPortDef->format.video.nSliceHeight          = -1;
    pPortDef->format.video.xFramerate            = fToQ16(15.0);
    pPortDef->format.video.bFlagErrorConcealment = OMX_FALSE;

    if (sDynamicFormat != NULL)
    {
        if ( strcmp(sDynamicFormat,  "MPEG4") == 0 )
        {
            pPortDef->format.video.eCompressionFormat = OMX_VIDEO_CodingMPEG4;
            pPortDef->format.video.cMIMEType             = "mp4";
            pPortDef->format.video.nBitrate              = VIDENC_STRUCT_MPEG4DEFBITRATE[0][1];
        }
        else if (strcmp(sDynamicFormat,  "H263") == 0 )
        {
            pPortDef->format.video.eCompressionFormat  = OMX_VIDEO_CodingH263;
            pPortDef->format.video.cMIMEType             = "mp4";
            pPortDef->format.video.nBitrate              = VIDENC_STRUCT_H263DEFBITRATE[0][1];
        }
        else if (strcmp(sDynamicFormat,  "H264") == 0 )
        {
            pPortDef->format.video.eCompressionFormat    = OMX_VIDEO_CodingAVC;
            pPortDef->format.video.cMIMEType             = "264";
            pPortDef->format.video.nBitrate              = VIDENC_STRUCT_H264DEFBITRATE[0][1];
        }
    }
    else
    {
        pPortDef->format.video.eCompressionFormat    = OMX_VIDEO_CodingAVC;
        pPortDef->format.video.cMIMEType             = "264";
        pPortDef->format.video.nBitrate              = VIDENC_STRUCT_H264DEFBITRATE[0][1];
    }

    pPortDef->format.video.eColorFormat          = OMX_COLOR_FormatUnused;

    CalculateBufferSize(pPortDef, pComponentPrivate);
    pComponentPrivate->nOutBufferSize = 0;

    for (i = 0; i < VIDENC_MAX_NUM_OF_OUT_BUFFERS; i++)
    {
        VIDENC_MALLOC(pCompPortOut->pBufferPrivate[i],
                      sizeof(VIDENC_BUFFER_PRIVATE),
                      VIDENC_BUFFER_PRIVATE,
                      pMemoryListHead, pComponentPrivate->dbg);
    }
    for (i = 0; i < VIDENC_MAX_NUM_OF_OUT_BUFFERS; i++)
    {
        pCompPortOut->pBufferPrivate[i]->pBufferHdr = NULL;
    }
    /*allocate MPEG4 metadata structure*/
    for (i = 0; i < VIDENC_MAX_NUM_OF_OUT_BUFFERS; i++)
    {
        VIDENC_MALLOC(pCompPortOut->pBufferPrivate[i]->pMetaData,
                      sizeof(VIDENC_MPEG4_SEGMENTMODE_METADATA),
                      VIDENC_MPEG4_SEGMENTMODE_METADATA,
                      pMemoryListHead, pComponentPrivate->dbg);
    }

    /*segment mode defaults*/
    pComponentPrivate->bMVDataEnable=OMX_FALSE;
    pComponentPrivate->bResyncDataEnable=OMX_FALSE;
    pCompPortOut->nBufferCnt = 0;

    /* Set input port format defaults */
    pPortFormat = pCompPortIn->pPortFormat;
    OMX_CONF_INIT_STRUCT(pPortFormat, OMX_VIDEO_PARAM_PORTFORMATTYPE);
    pPortFormat->nPortIndex         = VIDENC_INPUT_PORT;
    pPortFormat->nIndex             = 0x0;
    pPortFormat->eCompressionFormat = OMX_VIDEO_CodingUnused;
    pPortFormat->eColorFormat       = OMX_COLOR_FormatYUV420Planar;

    /* Set output port format defaults */
    pPortFormat = pCompPortOut->pPortFormat;
    OMX_CONF_INIT_STRUCT(pPortFormat, OMX_VIDEO_PARAM_PORTFORMATTYPE);
    pPortFormat->nPortIndex         = VIDENC_OUTPUT_PORT;
    pPortFormat->nIndex             = 0x0;

    if (sDynamicFormat != NULL)
    {
        if ( strcmp(sDynamicFormat,  "MPEG4") == 0 )
        {
            pPortFormat->eCompressionFormat = OMX_VIDEO_CodingMPEG4;
        }
        else if (strcmp(sDynamicFormat,  "H263") == 0 )
        {
            pPortFormat->eCompressionFormat = OMX_VIDEO_CodingH263;
        }
        else if (strcmp(sDynamicFormat,  "H264") == 0 )
        {
            pPortFormat->eCompressionFormat = OMX_VIDEO_CodingAVC;
        }
    }
    else
    {
        pPortFormat->eCompressionFormat = OMX_VIDEO_CodingAVC;
    }

    pPortFormat->eColorFormat       = OMX_COLOR_FormatUnused;

    /* Set pPriorityMgmt defaults */
    pPriorityMgmt = pComponentPrivate->pPriorityMgmt;
    OMX_CONF_INIT_STRUCT(pPriorityMgmt, OMX_PRIORITYMGMTTYPE);
    pPriorityMgmt->nGroupPriority   = -1;
    pPriorityMgmt->nGroupID         = -1;

    /* Buffer supplier setting */
    pCompPortIn->eSupplierSetting = OMX_BufferSupplyOutput;

    /* Set pH264 defaults */
    OMX_CONF_INIT_STRUCT(pComponentPrivate->pH264, OMX_VIDEO_PARAM_AVCTYPE);
    pComponentPrivate->pH264->nPortIndex                = VIDENC_OUTPUT_PORT;
    pComponentPrivate->pH264->nSliceHeaderSpacing       = 0;
    pComponentPrivate->pH264->nPFrames                  = -1;
    pComponentPrivate->pH264->nBFrames                  = -1;
    pComponentPrivate->pH264->bUseHadamard              = OMX_TRUE; /*OMX_FALSE*/
    pComponentPrivate->pH264->nRefFrames                = 1; /*-1;  */
    pComponentPrivate->pH264->nRefIdx10ActiveMinus1     = -1;
    pComponentPrivate->pH264->nRefIdx11ActiveMinus1     = -1;
    pComponentPrivate->pH264->bEnableUEP                = OMX_FALSE;
    pComponentPrivate->pH264->bEnableFMO                = OMX_FALSE;
    pComponentPrivate->pH264->bEnableASO                = OMX_FALSE;
    pComponentPrivate->pH264->bEnableRS                 = OMX_FALSE;
    pComponentPrivate->pH264->eProfile                  = OMX_VIDEO_AVCProfileBaseline; /*0x01;*/
    pComponentPrivate->pH264->eLevel                    = OMX_VIDEO_AVCLevel1; /*OMX_VIDEO_AVCLevel11;*/
    pComponentPrivate->pH264->nAllowedPictureTypes      = -1;
    pComponentPrivate->pH264->bFrameMBsOnly             = OMX_FALSE;
    pComponentPrivate->pH264->bMBAFF                    = OMX_FALSE;
    pComponentPrivate->pH264->bEntropyCodingCABAC       = OMX_FALSE;
    pComponentPrivate->pH264->bWeightedPPrediction      = OMX_FALSE;
    pComponentPrivate->pH264->nWeightedBipredicitonMode = -1;
    pComponentPrivate->pH264->bconstIpred               = OMX_FALSE;
    pComponentPrivate->pH264->bDirect8x8Inference       = OMX_FALSE;
    pComponentPrivate->pH264->bDirectSpatialTemporal    = OMX_FALSE;
    pComponentPrivate->pH264->nCabacInitIdc             = -1;
    pComponentPrivate->pH264->eLoopFilterMode           = 1;
    /*other h264 defaults*/
    pComponentPrivate->intra4x4EnableIdc                = INTRA4x4_IPSLICES;
    pComponentPrivate->maxMVperMB                       = 4;
    pComponentPrivate->nEncodingPreset                  = 3;/*0:DEFAULT/ 1:HIGH QUALITY/ 2:HIGH SPEED/ 3:USER DEFINED*/
    pComponentPrivate->AVCNALFormat                      = VIDENC_AVC_NAL_SLICE;/*VIDENC_AVC_NAL_UNIT;*/
    /* Set pMpeg4 defaults */
    OMX_CONF_INIT_STRUCT(pComponentPrivate->pMpeg4, OMX_VIDEO_PARAM_MPEG4TYPE);
    pComponentPrivate->pMpeg4->nPortIndex           = VIDENC_OUTPUT_PORT;
    pComponentPrivate->pMpeg4->nSliceHeaderSpacing  = 0;
    pComponentPrivate->pMpeg4->bSVH                 = OMX_FALSE;
    pComponentPrivate->pMpeg4->bGov                 = OMX_FALSE;
    pComponentPrivate->pMpeg4->nPFrames             = -1;
    pComponentPrivate->pMpeg4->nBFrames             = -1;
    pComponentPrivate->pMpeg4->nIDCVLCThreshold     = 0;  /*-1*/
    pComponentPrivate->pMpeg4->bACPred              = OMX_TRUE;
    pComponentPrivate->pMpeg4->nMaxPacketSize       = -1;
    pComponentPrivate->pMpeg4->nTimeIncRes          = -1;
    pComponentPrivate->pMpeg4->eProfile             = OMX_VIDEO_MPEG4ProfileSimple;
#ifdef __KHRONOS_CONF_1_1__
    pComponentPrivate->pMpeg4->eLevel               = OMX_VIDEO_MPEG4Level1;
#else
    pComponentPrivate->pMpeg4->eLevel               = 0x0;
#endif
    pComponentPrivate->pMpeg4->nAllowedPictureTypes = -1;
    pComponentPrivate->pMpeg4->nHeaderExtension     = 0;
    pComponentPrivate->pMpeg4->bReversibleVLC       = OMX_FALSE;

    /* Set pH263 defaults */
    OMX_CONF_INIT_STRUCT(pComponentPrivate->pH263, OMX_VIDEO_PARAM_H263TYPE);
    pComponentPrivate->pH263->nPortIndex               = VIDENC_OUTPUT_PORT;
    pComponentPrivate->pH263->nPFrames                 = 0;
    pComponentPrivate->pH263->nBFrames                 = 0;
    pComponentPrivate->pH263->eProfile                 = OMX_VIDEO_H263ProfileBaseline;
    pComponentPrivate->pH263->eLevel                   = OMX_VIDEO_H263Level10;
    pComponentPrivate->pH263->bPLUSPTYPEAllowed        = OMX_FALSE;
    pComponentPrivate->pH263->nAllowedPictureTypes     = 0;
    pComponentPrivate->pH263->bForceRoundingTypeToZero = OMX_TRUE;
    pComponentPrivate->pH263->nPictureHeaderRepetition = 0;
    pComponentPrivate->pH263->nGOBHeaderInterval       = 1;

    /* Set pVidParamBitrate and intraRefreshType defaults */
    OMX_CONF_INIT_STRUCT(pCompPortOut->pErrorCorrectionType, OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE);
    OMX_CONF_INIT_STRUCT(pCompPortOut->pIntraRefreshType, OMX_VIDEO_PARAM_INTRAREFRESHTYPE);
    pCompPortOut->pErrorCorrectionType->nPortIndex= VIDENC_OUTPUT_PORT;
    pCompPortOut->pIntraRefreshType->nPortIndex= VIDENC_OUTPUT_PORT;
    /*initDSP params*/
    /*Error resilience tools used by MPEG4/H263 encoder*/
    pCompPortOut->pErrorCorrectionType->bEnableHEC= OMX_TRUE;/*shouldn't be 0?*/
    pCompPortOut->pErrorCorrectionType->bEnableResync = OMX_TRUE;/*shouldn't be 0?*/
    pCompPortOut->pErrorCorrectionType->bEnableDataPartitioning= OMX_FALSE;
    pCompPortOut->pErrorCorrectionType->bEnableRVLC= OMX_FALSE;
    pCompPortOut->pErrorCorrectionType->nResynchMarkerSpacing = 1024;

    pCompPortOut->pIntraRefreshType->nAirRef = 10;
    /* Set pVidParamBitrate defaults */
    OMX_CONF_INIT_STRUCT(pComponentPrivate->pVidParamBitrate, OMX_VIDEO_PARAM_BITRATETYPE);
    pComponentPrivate->pVidParamBitrate->nPortIndex     = VIDENC_OUTPUT_PORT;
    pComponentPrivate->pVidParamBitrate->eControlRate   = OMX_Video_ControlRateConstant; 
    pComponentPrivate->pVidParamBitrate->nTargetBitrate = 64000;
    /**/
    pComponentPrivate->nMIRRate=0;
    /* Set pQuantization defaults */
    OMX_CONF_INIT_STRUCT(pComponentPrivate->pQuantization, OMX_VIDEO_PARAM_QUANTIZATIONTYPE);
    pComponentPrivate->pQuantization->nPortIndex = VIDENC_OUTPUT_PORT;
    pComponentPrivate->pQuantization->nQpI       = 12;
    pComponentPrivate->pQuantization->nQpP       = 0;
    pComponentPrivate->pQuantization->nQpB       = 0;

    /* Set pMotionVector defaults */
    OMX_CONF_INIT_STRUCT(pComponentPrivate->pMotionVector, OMX_VIDEO_PARAM_MOTIONVECTORTYPE);
    pComponentPrivate->pMotionVector->nPortIndex = VIDENC_OUTPUT_PORT;
    pComponentPrivate->pMotionVector->bFourMV    = 1;
    pComponentPrivate->pMotionVector->bUnrestrictedMVs = 0;   /* unused */
    pComponentPrivate->pMotionVector->eAccuracy  = OMX_Video_MotionVectorQuarterPel;
    pComponentPrivate->pMotionVector->sXSearchRange = pComponentPrivate->pMotionVector->sXSearchRange = 64;

    /* Set pIntraPeriod defaults */
    OMX_CONF_INIT_STRUCT(pComponentPrivate->pH264IntraPeriod, OMX_VIDEO_CONFIG_AVCINTRAPERIOD);
    pComponentPrivate->pH264IntraPeriod->nPortIndex = VIDENC_OUTPUT_PORT;
    pComponentPrivate->pH264IntraPeriod->nIDRPeriod = 0;
    pComponentPrivate->pH264IntraPeriod->nPFrames = 30;

#ifdef __KHRONOS_CONF_1_1__
    OMX_CONF_INIT_STRUCT(pCompPortIn->pProfileType, OMX_VIDEO_PARAM_PROFILELEVELTYPE);
    pCompPortIn->pProfileType->nPortIndex = VIDENC_INPUT_PORT;
    pCompPortIn->pProfileType->eLevel = OMX_VIDEO_AVCLevel1;
    pCompPortIn->pProfileType->eProfile = OMX_VIDEO_AVCProfileBaseline;
    pCompPortIn->pProfileType->nProfileIndex = 0;

    OMX_CONF_INIT_STRUCT(pCompPortOut->pProfileType, OMX_VIDEO_PARAM_PROFILELEVELTYPE);
    pCompPortOut->pProfileType->nPortIndex = VIDENC_OUTPUT_PORT;
    pCompPortOut->pProfileType->eLevel = OMX_VIDEO_AVCLevel1;
    pCompPortOut->pProfileType->eProfile = OMX_VIDEO_AVCProfileBaseline;
    pCompPortOut->pProfileType->nProfileIndex = 0;
    OMX_CONF_INIT_STRUCT(pCompPortIn->pFrameRateConfig, OMX_CONFIG_FRAMERATETYPE);
    pCompPortIn->pFrameRateConfig->nPortIndex = VIDENC_INPUT_PORT;
    pCompPortIn->pFrameRateConfig->xEncodeFramerate = 0;

    OMX_CONF_INIT_STRUCT(pCompPortOut->pFrameRateConfig, OMX_CONFIG_FRAMERATETYPE);
    pCompPortOut->pFrameRateConfig->nPortIndex = VIDENC_OUTPUT_PORT;
    pCompPortOut->pFrameRateConfig->xEncodeFramerate = (15<<16);

    OMX_CONF_INIT_STRUCT(pCompPortIn->pBitRateTypeConfig, OMX_VIDEO_CONFIG_BITRATETYPE);
    pCompPortIn->pBitRateTypeConfig->nPortIndex = VIDENC_INPUT_PORT;
    pCompPortIn->pBitRateTypeConfig->nEncodeBitrate = 0;

    OMX_CONF_INIT_STRUCT(pCompPortOut->pBitRateTypeConfig, OMX_VIDEO_CONFIG_BITRATETYPE);
    pCompPortOut->pBitRateTypeConfig->nPortIndex = VIDENC_OUTPUT_PORT;
    if(pPortDef->format.video.eCompressionFormat == OMX_VIDEO_CodingMPEG4)
    {
        pCompPortOut->pBitRateTypeConfig->nEncodeBitrate = VIDENC_STRUCT_MPEG4DEFBITRATE[0][1];
    }
    else if(pPortDef->format.video.eCompressionFormat == OMX_VIDEO_CodingH263) {
        pCompPortOut->pBitRateTypeConfig->nEncodeBitrate = VIDENC_STRUCT_H263DEFBITRATE[0][1];
    }
    else {
        pCompPortOut->pBitRateTypeConfig->nEncodeBitrate = VIDENC_STRUCT_H264DEFBITRATE[0][1];
    }

#endif
    OMX_CONF_INIT_STRUCT(pCompPortIn->pBitRateType, OMX_VIDEO_PARAM_BITRATETYPE);
    pCompPortIn->pBitRateType->nPortIndex = VIDENC_INPUT_PORT;
    pCompPortIn->pBitRateType->eControlRate = OMX_Video_ControlRateDisable;
    pCompPortIn->pBitRateType->nTargetBitrate = 0;

    OMX_CONF_INIT_STRUCT(pCompPortOut->pBitRateType, OMX_VIDEO_PARAM_BITRATETYPE);
    pCompPortOut->pBitRateType->nPortIndex = VIDENC_OUTPUT_PORT;
    pCompPortOut->pBitRateType->eControlRate = OMX_Video_ControlRateConstant;
    pCompPortOut->pBitRateType->nTargetBitrate = pCompPortOut->pBitRateTypeConfig->nEncodeBitrate;

    /*set the capability Flags needed by Opencore*/
    pComponentPrivate->pCapabilityFlags->iIsOMXComponentMultiThreaded=OMX_TRUE;
    pComponentPrivate->pCapabilityFlags->iOMXComponentCanHandleIncompleteFrames=OMX_FALSE;
    pComponentPrivate->pCapabilityFlags->iOMXComponentSupportsExternalInputBufferAlloc=OMX_FALSE;
    pComponentPrivate->pCapabilityFlags->iOMXComponentSupportsExternalOutputBufferAlloc=OMX_FALSE;
    pComponentPrivate->pCapabilityFlags->iOMXComponentSupportsMovableInputBuffers=OMX_TRUE;
    pComponentPrivate->pCapabilityFlags->iOMXComponentSupportsPartialFrames=OMX_FALSE;
    pComponentPrivate->pCapabilityFlags->iOMXComponentUsesFullAVCFrames=OMX_FALSE;
    pComponentPrivate->pCapabilityFlags->iOMXComponentUsesNALStartCode=OMX_FALSE;
    pComponentPrivate->nLastUpdateTime = 0;
    pComponentPrivate->nFrameRateUpdateInterval = 60;
    pComponentPrivate->nFrameCount = 0;
    pComponentPrivate->nVideoTime = 0;
    pComponentPrivate->EmptybufferdoneCount = 0;
    pComponentPrivate->EmptythisbufferCount = 0;
    pComponentPrivate->FillbufferdoneCount  = 0;
    pComponentPrivate->FillthisbufferCount  = 0;
#ifndef UNDER_CE
    /* Initialize Mutex for Buffer Tracking */
    pthread_mutex_init(&(pComponentPrivate->mVideoEncodeBufferMutex), NULL);
#else
    /* Add WinCE critical section API */
#endif

    /* create the pipe used to maintain free input buffers*/
    eError = pipe(pComponentPrivate->nFree_oPipe);
    if (eError)
    {
        OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorInsufficientResources);
    }

    /* create the pipe used to maintain input buffers*/
    eError = pipe(pComponentPrivate->nFilled_iPipe);
    if (eError)
    {
        OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorInsufficientResources);
    }

    /* create the pipe used to send commands to the thread */
    eError = pipe(pComponentPrivate->nCmdPipe);
    if (eError)
    {
        OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorInsufficientResources);
    }

    /* create the pipe used to send commands to the thread */
    eError = pipe(pComponentPrivate->nCmdDataPipe);
    if (eError)
    {
        OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorInsufficientResources);
    }

#ifdef RESOURCE_MANAGER_ENABLED
    /* Initialize Resource Manager */
    eError = RMProxy_NewInitalizeEx(OMX_COMPONENTTYPE_VIDEO);
    if (eError != OMX_ErrorNone)
    {
        OMX_PRMGR4(dbg, "Error returned from loading ResourceManagerProxy thread...\n");
        OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorInsufficientResources);
    }
#endif
    /* Create the Component Thread */
#ifdef UNDER_CE
    attr.__inheritsched = PTHREAD_EXPLICIT_SCHED;
    attr.__schedparam.__sched_priority = OMX_VIDEO_ENCODER_THREAD_PRIORITY;
    nError = pthread_create(&ComponentThread, &attr, OMX_VIDENC_Thread, pComponentPrivate);
#else
    nError = pthread_create(&pComponentPrivate->ComponentThread, NULL, OMX_VIDENC_Thread, pComponentPrivate);
#endif


#ifndef UNDER_CE
    if (nError == EAGAIN)
    {
        OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorInsufficientResources);
    }
#else
    if (nError || !(pComponentPrivate->ComponentThread))
    {
        OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorInsufficientResources);
    }
#endif

#ifdef __PERF_INSTRUMENTATION__
    PERF_ThreadCreated(pComponentPrivate->pPERF, pComponentPrivate->ComponentThread,
                       PERF_FOURCC('V','E',' ','T'));
#endif
#ifndef UNDER_CE
    /*pthread_mutex_init(&pComponentPrivate->videoe_mutex_app, NULL);*/
    pthread_mutex_init(&pComponentPrivate->videoe_mutex, NULL);
    pthread_cond_init (&pComponentPrivate->populate_cond, NULL);
    pthread_mutex_init(&pComponentPrivate->videoe_mutex_app, NULL);
    pthread_cond_init (&pComponentPrivate->unpopulate_cond, NULL);
    pthread_cond_init (&pComponentPrivate->flush_cond, NULL);
    pthread_cond_init (&pComponentPrivate->stop_cond, NULL);

    pthread_mutex_init(&bufferReturned_mutex, NULL);
    pthread_cond_init (&bufferReturned_condition, NULL);
#else
    OMX_CreateEvent(&(pComponentPrivate->InLoaded_event));
    OMX_CreateEvent(&(pComponentPrivate->InIdle_event));
#endif

    if(pthread_mutex_init(&pComponentPrivate->mutexStateChangeRequest, NULL)) {
       return OMX_ErrorUndefined;
    }

    if(pthread_cond_init (&pComponentPrivate->StateChangeCondition, NULL)) {
       return OMX_ErrorUndefined;
    }
OMX_CONF_CMD_BAIL:
    OMX_PRINT2(dbg, "Component Init Exit\n");
    return eError;
}

/*----------------------------------------------------------------------------*/
/**
  *  SetCallbacks() Sets application callbacks to the component
  *
  * This method will update application callbacks
  * the application.
  *
  * @param pComp         handle for this instance of the component
  * @param pCallBacks    application callbacks
  * @param ptr
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  **/
/*----------------------------------------------------------------------------*/

static OMX_ERRORTYPE SetCallbacks (OMX_IN  OMX_HANDLETYPE hComponent,
                                   OMX_IN  OMX_CALLBACKTYPE* pCallBacks,
                                   OMX_IN  OMX_PTR pAppData)
{
    OMX_ERRORTYPE eError                        = OMX_ErrorNone;
    OMX_COMPONENTTYPE* pHandle                  = NULL;
    VIDENC_COMPONENT_PRIVATE* pComponentPrivate = NULL;
    OMX_U32* pTmp                               = NULL;

    pComponentPrivate = (VIDENC_COMPONENT_PRIVATE*)(((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);
    OMX_CONF_CHECK_CMD(pComponentPrivate, 1, 1);
    OMX_DBG_CHECK_CMD(pComponentPrivate->dbg, pCallBacks, 1, 1);

    /*Copy the callbacks of the application to the component private */
    pTmp = memcpy (&(pComponentPrivate->sCbData), pCallBacks, sizeof(OMX_CALLBACKTYPE));
    if (pTmp == NULL)
    {
        OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorUndefined, pComponentPrivate->dbg,
                               OMX_TRACE4, "Failed to copy callbacks.\n");
    }

    /*copy the application private data to component memory*/
    pHandle = (OMX_COMPONENTTYPE*)hComponent;
    pHandle->pApplicationPrivate = pAppData;
    pComponentPrivate->eState = OMX_StateLoaded;

OMX_CONF_CMD_BAIL:
    return eError;
}

/*----------------------------------------------------------------------------*/
/**
  *  GetComponentVersion() Sets application callbacks to the component
  *
  * This method will update application callbacks
  * the application.
  *
  * @param pComp         handle for this instance of the component
  * @param pCallBacks    application callbacks
  * @param ptr
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  **/
/*----------------------------------------------------------------------------*/

static OMX_ERRORTYPE GetComponentVersion (OMX_HANDLETYPE hComp,
                                          OMX_STRING  szComponentName,
                                          OMX_VERSIONTYPE* pComponentVersion,
                                          OMX_VERSIONTYPE* pSpecVersion,
                                          OMX_UUIDTYPE* pComponentUUID)
{
    OMX_ERRORTYPE eError                        = OMX_ErrorNone;
    OMX_COMPONENTTYPE* pHandle                  = NULL;
    VIDENC_COMPONENT_PRIVATE* pComponentPrivate = NULL;

    OMX_CONF_CHECK_CMD(hComp, ((OMX_COMPONENTTYPE *) hComp)->pComponentPrivate, 1);
    pHandle = (OMX_COMPONENTTYPE*)hComp;
    pComponentPrivate = (VIDENC_COMPONENT_PRIVATE*)pHandle->pComponentPrivate;

    OMX_DBG_CHECK_CMD(pComponentPrivate->dbg, szComponentName, pComponentVersion, pSpecVersion);
    OMX_DBG_CHECK_CMD(pComponentPrivate->dbg, pComponentUUID, 1, 1);

    if (pComponentPrivate->eState == OMX_StateInvalid)
    {
        OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorIncorrectStateOperation,
                               pComponentPrivate->dbg, OMX_PRSTATE3,
                               "Component is in invalid state.\n");
    }

    strcpy(szComponentName, pComponentPrivate->cComponentName);
    memcpy(pComponentVersion,
           &(pComponentPrivate->ComponentVersion.s),
           sizeof(pComponentPrivate->ComponentVersion.s));
    memcpy(pSpecVersion,
           &(pComponentPrivate->SpecVersion.s),
           sizeof(pComponentPrivate->SpecVersion.s));

OMX_CONF_CMD_BAIL:
    return eError;
}

/*----------------------------------------------------------------------------*/
/**
  *  SendCommand() Sets application callbacks to the component
  *
  * This method will update application callbacks
  * the application.
  *
  * @param pComp         handle for this instance of the component
  * @param pCallBacks    application callbacks
  * @param ptr
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  **/
/*----------------------------------------------------------------------------*/

static OMX_ERRORTYPE SendCommand (OMX_IN OMX_HANDLETYPE hComponent,
                                  OMX_IN OMX_COMMANDTYPE Cmd,
                                  OMX_IN OMX_U32 nParam1,
                                  OMX_IN OMX_PTR pCmdData)
{
    OMX_ERRORTYPE eError                        = OMX_ErrorNone;
    int nRet                                    = 0;
    VIDENC_COMPONENT_PRIVATE* pComponentPrivate = NULL;
    char* szCommandType                         = NULL;
    char* szParam                               = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDefIn    = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDefOut   = NULL;
    VIDENC_NODE* pMemoryListHead                = NULL;

    OMX_CONF_CHECK_CMD(hComponent, ((OMX_COMPONENTTYPE *) hComponent)->pComponentPrivate, 1);

    pComponentPrivate = (VIDENC_COMPONENT_PRIVATE*)(((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);
    if (Cmd == OMX_CommandMarkBuffer)
    {
        OMX_DBG_CHECK_CMD(pComponentPrivate->dbg, pCmdData, 1, 1);
    }

    if (pComponentPrivate->eState == OMX_StateInvalid)
    {
        OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorIncorrectStateOperation,
                               pComponentPrivate->dbg, OMX_PRSTATE3,
                               "Component is in invalid state.\n");
    }

    pMemoryListHead = pComponentPrivate->pMemoryListHead;

#ifdef __PERF_INSTRUMENTATION__
    PERF_SendingCommand(pComponentPrivate->pPERF,
                        Cmd,
                        (Cmd == OMX_CommandMarkBuffer) ? ((OMX_U32) pCmdData) : nParam1,
                        PERF_ModuleComponent);
#endif

    switch (Cmd)
    {
        case OMX_CommandStateSet:
            /* Add a pending transition */
            if(AddStateTransition(pComponentPrivate) != OMX_ErrorNone) {
               return OMX_ErrorUndefined;
           }

#ifdef __KHRONOS_CONF__
            if(nParam1 == OMX_StateLoaded &&
               pComponentPrivate->eState == OMX_StateIdle)
            {
                pComponentPrivate->bPassingIdleToLoaded = OMX_TRUE;
            }
#endif
        OMX_PRCOMM2(pComponentPrivate->dbg, "Write to cmd pipe!\n");
            nRet = write(pComponentPrivate->nCmdPipe[1], &Cmd, sizeof(Cmd));
            if (nRet == -1) {
                 /* Decrement reference count without generating any signal */
                 if(RemoveStateTransition(pComponentPrivate, 0) != OMX_ErrorNone) {
                     return OMX_ErrorUndefined;
                 }
            }
            nRet = write(pComponentPrivate->nCmdDataPipe[1],
                         &nParam1,
                         sizeof(nParam1));
            if (nRet == -1) {
                 /* Decrement reference count without generating any signal */
                 if(RemoveStateTransition(pComponentPrivate, 0) != OMX_ErrorNone) {
                     return OMX_ErrorUndefined;
                 }
            }
            break;
        case OMX_CommandFlush:
            if (nParam1 > 1 && nParam1 != (OMX_U32)-1)
            {
            OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorBadPortIndex,
                                   pComponentPrivate->dbg, OMX_PRBUFFER4,
                                   "Invalid flush nParam1 (%lu).\n", nParam1);
            }
            nRet = write(pComponentPrivate->nCmdPipe[1], &Cmd, sizeof(Cmd));
            if (nRet == -1)
            {
            OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorUndefined,
                                   pComponentPrivate->dbg, OMX_PRCOMM4,
                                   "Failed to write to cmd pipe.\n");
            }
            nRet = write(pComponentPrivate->nCmdDataPipe[1],
                       &nParam1,
                       sizeof(nParam1));
            if (nRet == -1)
            {
            OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorUndefined,
                                   pComponentPrivate->dbg, OMX_PRCOMM4,
                                   "Failed to write to cmd pipe.\n");
            }
            break;
        case OMX_CommandPortDisable:

            pPortDefIn = pComponentPrivate->pCompPort[VIDENC_INPUT_PORT]->pPortDef;
            pPortDefOut = pComponentPrivate->pCompPort[VIDENC_OUTPUT_PORT]->pPortDef;

            if (nParam1 == VIDENC_INPUT_PORT ||
                nParam1 == VIDENC_OUTPUT_PORT ||
                nParam1 == (OMX_U32)-1)
            {
                if (nParam1 == VIDENC_INPUT_PORT || nParam1 == (OMX_U32)-1)
                {

                    pPortDefIn->bEnabled = OMX_FALSE;
                }
                if (nParam1 == VIDENC_OUTPUT_PORT || nParam1 == (OMX_U32)-1)
                {

                    pPortDefOut->bEnabled = OMX_FALSE;
                }
            }
            else
            {
            OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorBadParameter,
                                   pComponentPrivate->dbg, OMX_PRBUFFER4,
                                   "Invalid port disable nParam1 (%lu).\n", nParam1);
            }

            nRet = write(pComponentPrivate->nCmdPipe[1], &Cmd, sizeof(Cmd));
            if (nRet == -1)
            {
            OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorUndefined,
                                   pComponentPrivate->dbg, OMX_PRCOMM4,
                                   "Failed to write to cmd pipe.\n");
            }
            nRet = write(pComponentPrivate->nCmdDataPipe[1],
                         &nParam1,
                         sizeof(nParam1));
            if (nRet == -1)
            {
            OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorUndefined,
                                   pComponentPrivate->dbg, OMX_PRCOMM4,
                                   "Failed to write to cmd pipe.\n");
            }
            break;
        case OMX_CommandPortEnable:
            pPortDefIn = pComponentPrivate->pCompPort[VIDENC_INPUT_PORT]->pPortDef;
            pPortDefOut = pComponentPrivate->pCompPort[VIDENC_OUTPUT_PORT]->pPortDef;

            if (nParam1 == VIDENC_INPUT_PORT ||
                nParam1 == VIDENC_OUTPUT_PORT ||
                nParam1 == (OMX_U32)-1)
            {
                if (nParam1 == VIDENC_INPUT_PORT || nParam1 == (OMX_U32)-1)
                {
                    pPortDefIn->bEnabled = OMX_TRUE;
                }
                if (nParam1 == VIDENC_OUTPUT_PORT || nParam1 == (OMX_U32)-1)
                {
                    pPortDefOut->bEnabled = OMX_TRUE;
                }
            }
            else
            {
            OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorBadParameter,
                                   pComponentPrivate->dbg, OMX_PRBUFFER4,
                                   "Invalid port enable nParam1 (%lu).\n", nParam1);
            }

            nRet = write(pComponentPrivate->nCmdPipe[1], &Cmd, sizeof(Cmd));
            if (nRet == -1)
            {
            OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorUndefined,
                                   pComponentPrivate->dbg, OMX_PRCOMM4,
                                   "Failed to write to cmd pipe.\n");
            }
            nRet = write(pComponentPrivate->nCmdDataPipe[1],
                         &nParam1,
                         sizeof(nParam1));
            if (nRet == -1)
            {
            OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorUndefined,
                                   pComponentPrivate->dbg, OMX_PRCOMM4,
                                   "Failed to write to cmd pipe.\n");
            }
            break;
        case OMX_CommandMarkBuffer:
            if (nParam1 > 0)
            {
            OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorBadPortIndex,
                                   pComponentPrivate->dbg, OMX_PRBUFFER4,
                                   "Invalid port number (%lu).\n", nParam1);
            }
            nRet = write(pComponentPrivate->nCmdPipe[1], &Cmd, sizeof(Cmd));
            if (nRet == -1)
            {
            OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorUndefined,
                                   pComponentPrivate->dbg, OMX_PRCOMM4,
                                   "Failed to write to cmd pipe.\n");
            }
            nRet = write(pComponentPrivate->nCmdDataPipe[1],
                         &pCmdData,
                         sizeof(pCmdData));
            if (nRet == -1)
            {
            OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorUndefined,
                                   pComponentPrivate->dbg, OMX_PRCOMM4,
                                   "Failed to write to cmd pipe.\n");
            }
            break;
        case OMX_CommandMax:
            break;
        default:
        OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorUndefined,
                               pComponentPrivate->dbg, OMX_PRCOMM4,
                               "Invalid command (%x).\n", Cmd);
    }

    OMX_CONVERT_CMD(szCommandType, Cmd);
    if (Cmd == OMX_CommandStateSet)
    {
        OMX_CONVERT_STATE(szParam, nParam1);
        OMX_PRSTATE2(pComponentPrivate->dbg, "%s -> %s\n", szCommandType, szParam);
    }

OMX_CONF_CMD_BAIL:
    return eError;
}

/*----------------------------------------------------------------------------*/
/**
  *  GetParameter() Sets application callbacks to the component
  *
  * This method will update application callbacks
  * the application.
  *
  * @param pComp         handle for this instance of the component
  * @param pCallBacks    application callbacks
  * @param ptr
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  **/
/*----------------------------------------------------------------------------*/

static OMX_ERRORTYPE GetParameter (OMX_IN OMX_HANDLETYPE hComponent,
                                   OMX_IN OMX_INDEXTYPE nParamIndex,
                                   OMX_INOUT OMX_PTR ComponentParameterStructure)
{
    VIDENC_COMPONENT_PRIVATE* pComponentPrivate = NULL;
    OMX_ERRORTYPE eError                        = OMX_ErrorNone;
    OMX_U32* pTmp                               = NULL;
    VIDEOENC_PORT_TYPE* pCompPortIn             = NULL;
    VIDEOENC_PORT_TYPE* pCompPortOut            = NULL;

    OMX_CONF_CHECK_CMD(hComponent, ((OMX_COMPONENTTYPE *) hComponent)->pComponentPrivate, 1);

    pComponentPrivate = (VIDENC_COMPONENT_PRIVATE*)(((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);
    OMX_DBG_CHECK_CMD(pComponentPrivate->dbg, ComponentParameterStructure, 1, 1);

    if (pComponentPrivate->eState == OMX_StateInvalid)
    {
        OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorIncorrectStateOperation,
                               pComponentPrivate->dbg, OMX_PRSTATE3,
                               "Component is in invalid state.\n");
    }

    pCompPortIn     = pComponentPrivate->pCompPort[VIDENC_INPUT_PORT];
    pCompPortOut    = pComponentPrivate->pCompPort[VIDENC_OUTPUT_PORT];

    switch (nParamIndex)
    {
        case OMX_IndexParamVideoInit:
            pTmp = memcpy(ComponentParameterStructure,
                          pComponentPrivate->pPortParamType,
                          sizeof(OMX_PORT_PARAM_TYPE));
            if (pTmp == NULL)
            {
            OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorUndefined,
                                   pComponentPrivate->dbg, OMX_TRACE4,
                                   "Failed to copy parameter.\n");
            }
            break;
#ifdef __KHRONOS_CONF_1_1__

        case OMX_IndexParamImageInit:
            pTmp = memcpy(ComponentParameterStructure,
                          pComponentPrivate->pPortImageType,
                          sizeof(OMX_PORT_PARAM_TYPE));
            if (pTmp == NULL)
            {
            OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorUndefined,
                                   pComponentPrivate->dbg, OMX_TRACE4,
                                   "Failed to copy parameter.\n");
            }
            break;

        case OMX_IndexParamAudioInit:
            pTmp = memcpy(ComponentParameterStructure,
                          pComponentPrivate->pPortAudioType,
                          sizeof(OMX_PORT_PARAM_TYPE));
            if (pTmp == NULL)
            {
            OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorUndefined,
                                   pComponentPrivate->dbg, OMX_TRACE4,
                                   "Failed to copy parameter.\n");
            }
            break;

        case OMX_IndexParamOtherInit:
            pTmp = memcpy(ComponentParameterStructure,
                          pComponentPrivate->pPortOtherType,
                          sizeof(OMX_PORT_PARAM_TYPE));
            if (pTmp == NULL)
            {
            OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorUndefined,
                                   pComponentPrivate->dbg, OMX_TRACE4,
                                   "Failed to copy parameter.\n");
            }
            break;

#endif

        case OMX_IndexParamPortDefinition:
        {
            OMX_PARAM_PORTDEFINITIONTYPE *pPortDef = (OMX_PARAM_PORTDEFINITIONTYPE*)(ComponentParameterStructure);
            if (pPortDef->nPortIndex == pCompPortIn->pPortDef->nPortIndex)
            {
                pTmp = memcpy(ComponentParameterStructure,
                              pCompPortIn->pPortDef,
                              sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
                if (pTmp == NULL)
                {
                    OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorUndefined,
                                           pComponentPrivate->dbg, OMX_TRACE4,
                                           "Failed to copy parameter.\n");
                }
            }
            else if (pPortDef->nPortIndex == pCompPortOut->pPortDef->nPortIndex)
            {
                pTmp = memcpy(ComponentParameterStructure,
                              pCompPortOut->pPortDef,
                              sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
                if (pTmp == NULL)
                {
                    OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorUndefined,
                                           pComponentPrivate->dbg, OMX_TRACE4,
                                           "Failed to copy parameter.\n");
                }
            }
            else
            {
                eError = OMX_ErrorBadPortIndex;
            }
            break;
            }
        case OMX_IndexParamVideoPortFormat:
            if (((OMX_VIDEO_PARAM_PORTFORMATTYPE*)(ComponentParameterStructure))->nPortIndex ==
                pCompPortIn->pPortFormat->nPortIndex)
        {
                if (((OMX_VIDEO_PARAM_PORTFORMATTYPE*)(ComponentParameterStructure))->nIndex < 3)
                {
                    ((OMX_VIDEO_PARAM_PORTFORMATTYPE*)(ComponentParameterStructure))->eColorFormat =
                    pComponentPrivate->colorFormats[((OMX_VIDEO_PARAM_PORTFORMATTYPE*)(ComponentParameterStructure))->nIndex];
                    eError = OMX_ErrorNone;
                }
                else
                {
                    OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorNoMore,
                                           pComponentPrivate->dbg, OMX_TRACE4,
                                           "No such index.\n");
                }
            }
            else if (((OMX_VIDEO_PARAM_PORTFORMATTYPE*)(ComponentParameterStructure))->nPortIndex ==
                     pCompPortOut->pPortFormat->nPortIndex)
            {
                if (((OMX_VIDEO_PARAM_PORTFORMATTYPE*)(ComponentParameterStructure))->nIndex < 3)
                {
                    ((OMX_VIDEO_PARAM_PORTFORMATTYPE*)(ComponentParameterStructure))->eCompressionFormat =
                    pComponentPrivate->compressionFormats[((OMX_VIDEO_PARAM_PORTFORMATTYPE*)(ComponentParameterStructure))->nIndex];
                    eError = OMX_ErrorNone;
                }
                else
                {
                    /*OMX_TRACE("OMX_IndexParamVideoPortFormat OMX_ErrorNoMore, no such index\n");*/
                    OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorNoMore);
                }
            }
            else
        {
                eError = OMX_ErrorBadPortIndex;
            }
            break;
        case OMX_IndexParamPriorityMgmt:
            pTmp = memcpy(ComponentParameterStructure,
                          pComponentPrivate->pPriorityMgmt,
                          sizeof(OMX_PRIORITYMGMTTYPE));
            if (pTmp == NULL)
            {
            OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorUndefined,
                                   pComponentPrivate->dbg, OMX_TRACE4,
                                   "Failed to copy parameter.\n");
            }
            break;
        case OMX_IndexParamVideoAvc:
            if (((OMX_VIDEO_PARAM_AVCTYPE*)(ComponentParameterStructure))->nPortIndex ==
                pComponentPrivate->pH264->nPortIndex)
            {
                pTmp = memcpy(ComponentParameterStructure,
                              pComponentPrivate->pH264,
                              sizeof(OMX_VIDEO_PARAM_AVCTYPE));
                if (pTmp == NULL)
                {
                OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorUndefined,
                                       pComponentPrivate->dbg, OMX_TRACE4,
                                       "Failed to copy parameter.\n");
                }
            }
            else
            {
                eError = OMX_ErrorBadPortIndex;
            }
            break;
        case OMX_IndexParamVideoMpeg4:
            if (((OMX_VIDEO_PARAM_MPEG4TYPE*)(ComponentParameterStructure))->nPortIndex ==
                pComponentPrivate->pMpeg4->nPortIndex)
            {
                pTmp = memcpy(ComponentParameterStructure,
                              pComponentPrivate->pMpeg4,
                              sizeof(OMX_VIDEO_PARAM_MPEG4TYPE));
                if (pTmp == NULL)
                {
                OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorUndefined,
                                       pComponentPrivate->dbg, OMX_TRACE4,
                                       "Failed to copy parameter.\n");
                }
            }
            else
            {
                eError = OMX_ErrorBadPortIndex;
            }
            break;
        case OMX_IndexParamCompBufferSupplier:
        {
            OMX_PARAM_BUFFERSUPPLIERTYPE* pBuffSupplierParam = (OMX_PARAM_BUFFERSUPPLIERTYPE*)ComponentParameterStructure;
            if (pBuffSupplierParam->nPortIndex == VIDENC_INPUT_PORT)
            {
                pBuffSupplierParam->eBufferSupplier = pCompPortIn->eSupplierSetting;
            }
            else if (pBuffSupplierParam->nPortIndex == VIDENC_OUTPUT_PORT)
            {
                pBuffSupplierParam->eBufferSupplier = pCompPortOut->eSupplierSetting;
            }
            else
            {
                eError = OMX_ErrorBadPortIndex;
            }
            break;
        }

        case OMX_IndexParamVideoBitrate:
            if (((OMX_VIDEO_PARAM_BITRATETYPE*)(ComponentParameterStructure))->nPortIndex ==
            pComponentPrivate->pCompPort[VIDENC_OUTPUT_PORT]->pBitRateType->nPortIndex)
            {
                pTmp = memcpy(ComponentParameterStructure,
                              pComponentPrivate->pCompPort[VIDENC_OUTPUT_PORT]->pBitRateType,
                              sizeof(OMX_VIDEO_PARAM_BITRATETYPE));
                if (pTmp == NULL)
                {
                OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorUndefined,
                                       pComponentPrivate->dbg, OMX_TRACE4,
                                       "Failed to copy parameter.\n");
                }
            }
            else
            {
                eError = OMX_ErrorBadPortIndex;
            }
            break;

        case OMX_IndexParamVideoH263:
            if (((OMX_VIDEO_PARAM_H263TYPE*)(ComponentParameterStructure))->nPortIndex ==
                pComponentPrivate->pH263->nPortIndex)
            {
                pTmp = memcpy(ComponentParameterStructure,
                              pComponentPrivate->pH263,
                              sizeof(OMX_VIDEO_PARAM_H263TYPE));
                if (pTmp == NULL)
                {
                OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorUndefined,
                                       pComponentPrivate->dbg, OMX_TRACE4,
                                       "Failed to copy parameter.\n");
                }
            }
            else
            {
                eError = OMX_ErrorBadPortIndex;
            }
            break;
        case OMX_IndexParamVideoQuantization:
            if (((OMX_VIDEO_PARAM_QUANTIZATIONTYPE*)(ComponentParameterStructure))->nPortIndex ==
                pComponentPrivate->pQuantization->nPortIndex)
            {
                pTmp = memcpy(ComponentParameterStructure,
                              pComponentPrivate->pQuantization,
                              sizeof(OMX_VIDEO_PARAM_QUANTIZATIONTYPE));
                if (pTmp == NULL)
                {
                OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorUndefined,
                                       pComponentPrivate->dbg, OMX_TRACE4,
                                       "Failed to copy parameter.\n");
                }
            }
            else
            {
                eError = OMX_ErrorBadPortIndex;
            }
            break;


    case OMX_IndexParamVideoProfileLevelQuerySupported:
    {
       VIDEO_PROFILE_LEVEL_TYPE* pProfileLevel = NULL;
        OMX_U32 nNumberOfProfiles = 0;
       OMX_VIDEO_PARAM_PROFILELEVELTYPE *pParamProfileLevel = (OMX_VIDEO_PARAM_PROFILELEVELTYPE *)ComponentParameterStructure;
       pParamProfileLevel->nPortIndex = pCompPortOut->pPortDef->nPortIndex;
         /* Choose table based on compression format */
       switch(pCompPortOut->pPortDef->format.video.eCompressionFormat)
       {
          case OMX_VIDEO_CodingH263:
             pProfileLevel = SupportedH263ProfileLevels;
             nNumberOfProfiles = sizeof(SupportedH263ProfileLevels) / sizeof (VIDEO_PROFILE_LEVEL_TYPE);
             break;
          case OMX_VIDEO_CodingMPEG4:
             pProfileLevel = SupportedMPEG4ProfileLevels;
             nNumberOfProfiles = sizeof(SupportedMPEG4ProfileLevels) / sizeof (VIDEO_PROFILE_LEVEL_TYPE);
             break;
           case OMX_VIDEO_CodingAVC:
             pProfileLevel = SupportedAVCProfileLevels;
             nNumberOfProfiles = sizeof(SupportedAVCProfileLevels) / sizeof (VIDEO_PROFILE_LEVEL_TYPE);
             break;
           default:
              eError = OMX_ErrorBadParameter;
              return eError;
        }
        if(pParamProfileLevel->nProfileIndex >= nNumberOfProfiles) {
            return OMX_ErrorBadParameter;
        }
        /* Point to table entry based on index */
        pProfileLevel += pParamProfileLevel->nProfileIndex;

        /* -1 indicates end of table */
        if(pProfileLevel->nProfile != -1) {
           pParamProfileLevel->eProfile = pProfileLevel->nProfile;
           pParamProfileLevel->eLevel = pProfileLevel->nLevel;
           eError = OMX_ErrorNone;
        }
        else {
           eError = OMX_ErrorNoMore;
        }
    }
    break;
    case OMX_IndexParamVideoProfileLevelCurrent:
    {
       OMX_VIDEO_PARAM_PROFILELEVELTYPE *pParamProfileLevel = (OMX_VIDEO_PARAM_PROFILELEVELTYPE *)  ComponentParameterStructure;
       if (pCompPortOut->pPortDef->format.video.eCompressionFormat == OMX_VIDEO_CodingAVC) {
          pParamProfileLevel->eProfile = pComponentPrivate->pH264->eProfile;
           pParamProfileLevel->eLevel = pComponentPrivate->pH264->eLevel;
       }
       else if (pCompPortOut->pPortDef->format.video.eCompressionFormat == OMX_VIDEO_CodingMPEG4) {
          pParamProfileLevel->eProfile = pComponentPrivate->pMpeg4->eProfile;
          pParamProfileLevel->eLevel = pComponentPrivate->pMpeg4->eLevel;

       }
       else if (pCompPortOut->pPortDef->format.video.eCompressionFormat == OMX_VIDEO_CodingH263) {
          pParamProfileLevel->eProfile = pComponentPrivate->pH263->eProfile;
          pParamProfileLevel->eLevel = pComponentPrivate->pH263->eLevel;
       }
       else {
          eError = OMX_ErrorBadParameter;
       }

    }
    break;

        case OMX_IndexParamVideoErrorCorrection:
                    {
                    if (((OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE*)(ComponentParameterStructure))->nPortIndex ==
                        VIDENC_OUTPUT_PORT)
                    {
                        pTmp = memcpy(ComponentParameterStructure,
                                      pComponentPrivate->pCompPort[VIDENC_OUTPUT_PORT]->pErrorCorrectionType,
                                      sizeof(OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE));
                if (pTmp == NULL)
                {
                    OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorUndefined,
                                           pComponentPrivate->dbg, OMX_TRACE4,
                                           "Failed to copy OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE parameter.\n");
                }
            }
                    else
                    {
                        eError = OMX_ErrorBadPortIndex;
                    }
                    break;
                }

        case VideoEncodeCustomParamIndexVBVSize:
            (*((OMX_U32*)ComponentParameterStructure)) = (OMX_U32)pComponentPrivate->nVBVSize;
            break;
        case VideoEncodeCustomParamIndexDeblockFilter:
            (*((OMX_BOOL*)ComponentParameterStructure)) = (OMX_BOOL)pComponentPrivate->bDeblockFilter;
            break;
        case VideoEncodeCustomParamIndexEncodingPreset:
            (*((unsigned int*)ComponentParameterStructure)) = (unsigned int)pComponentPrivate->nEncodingPreset;
            break;
        case VideoEncodeCustomConfigIndexUnrestrictedMV:
            (*((OMX_U8*)ComponentParameterStructure)) = (OMX_U8)pComponentPrivate->ucUnrestrictedMV;
            break;
       case VideoEncodeCustomParamIndexNALFormat:
           (*((unsigned int*)ComponentParameterStructure)) = (unsigned int)pComponentPrivate->AVCNALFormat;
           break;
       case PV_OMX_COMPONENT_CAPABILITY_TYPE_INDEX:
            pTmp = memcpy(ComponentParameterStructure,
            pComponentPrivate->pCapabilityFlags,
            sizeof(PV_OMXComponentCapabilityFlagsType));
            if (pTmp == NULL)
            {
                OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorUndefined);
            }
            break;
       //not supported yet
       case OMX_IndexConfigCommonRotate:
           break;
        default:
            eError = OMX_ErrorUnsupportedIndex;
            break;
    }
OMX_CONF_CMD_BAIL:
    return eError;
}

/*----------------------------------------------------------------------------*/
/**
  *  SetParameter() Sets application callbacks to the component
  *
  * This method will update application callbacks
  * the application.
  *
  * @param pComp         handle for this instance of the component
  * @param pCallBacks    application callbacks
  * @param ptr
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  **/
/*----------------------------------------------------------------------------*/

static OMX_ERRORTYPE SetParameter (OMX_IN OMX_HANDLETYPE hComponent,
                                   OMX_IN OMX_INDEXTYPE nParamIndex,
                                   OMX_IN OMX_PTR pCompParam)
{
    VIDENC_COMPONENT_PRIVATE* pComponentPrivate = NULL;
    OMX_ERRORTYPE eError                        = OMX_ErrorNone;
    OMX_U32* pTmp                               = NULL;
    VIDENC_NODE* pMemoryListHead                = NULL;
    VIDEOENC_PORT_TYPE* pCompPortIn             = NULL;
    VIDEOENC_PORT_TYPE* pCompPortOut            = NULL;

#ifdef __KHRONOS_CONF_1_1__
    OMX_PARAM_COMPONENTROLETYPE  *pRole = NULL;
    OMX_VIDEO_PARAM_PROFILELEVELTYPE* sProfileLevel;
#endif
    OMX_CONF_CHECK_CMD(hComponent, ((OMX_COMPONENTTYPE *) hComponent)->pComponentPrivate, 1);

    pComponentPrivate = (VIDENC_COMPONENT_PRIVATE*)(((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);
    OMX_DBG_CHECK_CMD(pComponentPrivate->dbg, pCompParam, 1, 1);

    pMemoryListHead = pComponentPrivate->pMemoryListHead;
    pCompPortIn     = pComponentPrivate->pCompPort[VIDENC_INPUT_PORT];
    pCompPortOut    = pComponentPrivate->pCompPort[VIDENC_OUTPUT_PORT];

    if ((pComponentPrivate->eState != OMX_StateLoaded) &&
        (pComponentPrivate->eState != OMX_StateWaitForResources) &&
        (nParamIndex != OMX_IndexParamPortDefinition ||
            (pCompPortIn->pPortDef->bEnabled && pCompPortOut->pPortDef->bEnabled)))
    {
        OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorIncorrectStateOperation,
                               pComponentPrivate->dbg, OMX_PRSTATE3,
                               "Call in an invalid state.\n");
    }

    switch (nParamIndex)
    {
        case OMX_IndexParamVideoPortFormat:
        {
            OMX_VIDEO_PARAM_PORTFORMATTYPE* pComponentParam = (OMX_VIDEO_PARAM_PORTFORMATTYPE*)pCompParam;
            if (pComponentParam->nPortIndex == pCompPortIn->pPortFormat->nPortIndex)
            {
                pTmp = memcpy(pCompPortIn->pPortFormat,
                              pComponentParam,
                              sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
                if (pTmp == NULL)
                {
                    OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorUndefined,
                                           pComponentPrivate->dbg, OMX_TRACE4,
                                           "Failed to copy parameter.\n");
                }
            }
            else if (pComponentParam->nPortIndex == pCompPortOut->pPortFormat->nPortIndex)
            {
                pTmp = memcpy(pCompPortOut->pPortFormat,
                              pComponentParam,
                              sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
                if (pTmp == NULL)
                {
                    OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorUndefined,
                                           pComponentPrivate->dbg, OMX_TRACE4,
                                           "Failed to copy parameter.\n");
                }
            }
            else
            {
                eError = OMX_ErrorBadPortIndex;
            }
            break;
        }
        case OMX_IndexParamVideoInit:
            pTmp = memcpy(pComponentPrivate->pPortParamType,
                          (OMX_PORT_PARAM_TYPE*)pCompParam,
                          sizeof(OMX_PORT_PARAM_TYPE));
            if (pTmp == NULL)
            {
            OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorUndefined,
                                   pComponentPrivate->dbg, OMX_TRACE4,
                                   "Failed to copy parameter.\n");
            }
            break;
        case OMX_IndexParamPortDefinition:
        {
            OMX_PARAM_PORTDEFINITIONTYPE* pComponentParam = (OMX_PARAM_PORTDEFINITIONTYPE*)pCompParam;
            eError = IsResolutionPlayable(pCompPortIn->pPortDef->format.video.nFrameWidth, pCompPortIn->pPortDef->format.video.nFrameHeight);
            if (eError != OMX_ErrorNone)
            {
                /* OMX components co-existance, if resolution is 720p or higher let the 720p OMX
                 * component to take care of them */
                return OMX_ErrorBadParameter;
            }
            if (pComponentParam->nPortIndex == pCompPortIn->pPortDef->nPortIndex)
            {
                pTmp = memcpy(pCompPortIn->pPortDef,
                              pComponentParam,
                              sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
                if (pTmp == NULL)
                {
                    OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorUndefined,
                                           pComponentPrivate->dbg, OMX_TRACE4,
                                           "Failed to copy parameter.\n");
                }
                CalculateBufferSize(pCompPortIn->pPortDef, pComponentPrivate);
            }
            else if (pComponentParam->nPortIndex == pCompPortOut->pPortDef->nPortIndex)
            {
                pTmp = memcpy(pCompPortOut->pPortDef,
                              pComponentParam,
                              sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
                if (pTmp == NULL)
                {
                    OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorUndefined,
                                           pComponentPrivate->dbg, OMX_TRACE4,
                                           "Failed to copy parameter.\n");
                }
                CalculateBufferSize(pCompPortOut->pPortDef, pComponentPrivate);
                if(!pCompPortOut->pPortDef->format.video.nBitrate)
                {
                    pCompPortOut->pPortDef->format.video.nBitrate = OMX_VIDENC_GetDefaultBitRate(pComponentPrivate);
                }
                pCompPortOut->pBitRateTypeConfig->nEncodeBitrate =
                pCompPortOut->pBitRateType->nTargetBitrate =
                pCompPortOut->pPortDef->format.video.nBitrate;
            }
            else
            {
                eError = OMX_ErrorBadPortIndex;
            }
            break;
        }
        case OMX_IndexParamVideoAvc:
        {
            OMX_VIDEO_PARAM_AVCTYPE* pComponentParam = (OMX_VIDEO_PARAM_AVCTYPE*)pCompParam;
            if (pComponentParam->nPortIndex == pComponentPrivate->pH264->nPortIndex)
            {
                pTmp = memcpy(pComponentPrivate->pH264,
                              pCompParam,
                              sizeof(OMX_VIDEO_PARAM_AVCTYPE));
                if (pTmp == NULL)
                {
                    OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorUndefined,
                                           pComponentPrivate->dbg, OMX_TRACE4,
                                           "Failed to copy parameter.\n");
                }
                CalculateBufferSize(pCompPortOut->pPortDef, pComponentPrivate);
            }
            else
            {
                eError = OMX_ErrorBadPortIndex;
            }
            break;
        }
        case OMX_IndexParamVideoMpeg4:
        {
            OMX_VIDEO_PARAM_MPEG4TYPE* pComponentParam = (OMX_VIDEO_PARAM_MPEG4TYPE*)pCompParam;
            if (pComponentParam->nPortIndex == pComponentPrivate->pMpeg4->nPortIndex)
            {
                pTmp = memcpy(pComponentPrivate->pMpeg4,
                              pCompParam,
                              sizeof(OMX_VIDEO_PARAM_MPEG4TYPE));
                if (pTmp == NULL)
                {
                    OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorUndefined,
                                           pComponentPrivate->dbg, OMX_TRACE4,
                                           "Failed to copy parameter.\n");
                }
            }
            else
            {
                eError = OMX_ErrorBadPortIndex;
            }
            break;
        }
        case OMX_IndexParamPriorityMgmt:
            pTmp = memcpy(pComponentPrivate->pPriorityMgmt,
                          (OMX_PRIORITYMGMTTYPE*)pCompParam,
                          sizeof(OMX_PRIORITYMGMTTYPE));
            if (pTmp == NULL)
            {
            OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorUndefined,
                                   pComponentPrivate->dbg, OMX_TRACE4,
                                   "Failed to copy parameter.\n");
            }
            break;
        case OMX_IndexParamCompBufferSupplier:
        {
            OMX_PARAM_BUFFERSUPPLIERTYPE* pBuffSupplierParam = (OMX_PARAM_BUFFERSUPPLIERTYPE*)pCompParam;
            if (pBuffSupplierParam->nPortIndex == VIDENC_INPUT_PORT)
            {
                pCompPortIn->eSupplierSetting = pBuffSupplierParam->eBufferSupplier;
            }
            else if (pBuffSupplierParam->nPortIndex == VIDENC_OUTPUT_PORT)
            {
                pCompPortOut->eSupplierSetting = pBuffSupplierParam->eBufferSupplier;
            }
            else
            {
                eError = OMX_ErrorBadPortIndex;
            }
            break;
        }
        case OMX_IndexParamVideoBitrate:
        {
            OMX_VIDEO_PARAM_BITRATETYPE* pComponentParam = (OMX_VIDEO_PARAM_BITRATETYPE*)pCompParam;
            if (pComponentParam->nPortIndex == pComponentPrivate->pCompPort[VIDENC_OUTPUT_PORT]->pBitRateType->nPortIndex)
            {
                pTmp = memcpy(pComponentPrivate->pCompPort[VIDENC_OUTPUT_PORT]->pBitRateType,
                              pCompParam,
                              sizeof(OMX_VIDEO_PARAM_BITRATETYPE));
                if (pTmp == NULL)
                {
                    OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorUndefined,
                                           pComponentPrivate->dbg, OMX_TRACE4,
                                           "Failed to copy parameter.\n");
                }
                if(!pCompPortOut->pBitRateType->nTargetBitrate)
                {
                    pCompPortOut->pBitRateType->nTargetBitrate = OMX_VIDENC_GetDefaultBitRate(pComponentPrivate);
                }
                pCompPortOut->pPortDef->format.video.nBitrate =
                pCompPortOut->pBitRateTypeConfig->nEncodeBitrate =
                pCompPortOut->pBitRateType->nTargetBitrate;
            }
            else
            {
                eError = OMX_ErrorBadPortIndex;
            }
            break;
        }
        case OMX_IndexParamVideoErrorCorrection:
                {
                       pTmp = memcpy(pComponentPrivate->pCompPort[VIDENC_OUTPUT_PORT]->pErrorCorrectionType,
                                     pCompParam,
                                     sizeof(OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE));
                       if (pTmp == NULL)
                        {
                OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorUndefined,
                                       pComponentPrivate->dbg, OMX_TRACE4,
                                       "Failed to copy parameter.\n");
                        }
                }
                break;
        case OMX_IndexParamVideoH263:
        {
            OMX_VIDEO_PARAM_H263TYPE* pComponentParam = (OMX_VIDEO_PARAM_H263TYPE*)pCompParam;
            if (pComponentParam->nPortIndex == pComponentPrivate->pH263->nPortIndex)
            {
                pTmp = memcpy(pComponentPrivate->pH263,
                              pCompParam,
                              sizeof(OMX_VIDEO_PARAM_H263TYPE));
                if (pTmp == NULL)
                {
                    OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorUndefined,
                                           pComponentPrivate->dbg, OMX_TRACE4,
                                           "Failed to copy parameter.\n");
                }
            }
            else
            {
                eError = OMX_ErrorBadPortIndex;
            }
            break;
        }
        case OMX_IndexParamVideoQuantization:
        {
            OMX_VIDEO_PARAM_QUANTIZATIONTYPE* pComponentParam = (OMX_VIDEO_PARAM_QUANTIZATIONTYPE*)pCompParam;
            if (pComponentParam->nPortIndex == pComponentPrivate->pQuantization->nPortIndex)
            {
                pTmp = memcpy(pComponentPrivate->pQuantization,
                              pCompParam,
                              sizeof(OMX_VIDEO_PARAM_QUANTIZATIONTYPE));
                if (pTmp == NULL)
                {
                    OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorUndefined,
                                           pComponentPrivate->dbg, OMX_TRACE4,
                                           "Failed to copy parameter.\n");
                }
            }
            else
            {
                eError = OMX_ErrorBadPortIndex;
            }
            break;
        }
        case VideoEncodeCustomParamIndexVBVSize:
            pComponentPrivate->nVBVSize = (OMX_U32)(*((OMX_U32*)pCompParam));
            break;
        case VideoEncodeCustomParamIndexDeblockFilter:
            pComponentPrivate->bDeblockFilter = (OMX_BOOL)(*((OMX_BOOL*)pCompParam));
            break;

#ifdef __KHRONOS_CONF_1_1__
    case OMX_IndexParamStandardComponentRole:
        if (pCompParam)
        {
            pRole = (OMX_PARAM_COMPONENTROLETYPE *)pCompParam;
            memcpy(&(pComponentPrivate->componentRole), (void *)pRole, sizeof(OMX_PARAM_COMPONENTROLETYPE));
            if (strcmp((char *)pRole->cRole,"video_encoder.mpeg4")==0)
            {
                pCompPortOut->pPortDef->format.video.eCompressionFormat = OMX_VIDEO_CodingMPEG4;
                pCompPortOut->pPortFormat->eCompressionFormat = OMX_VIDEO_CodingMPEG4;
                pCompPortOut->pPortDef->format.video.nBitrate = VIDENC_STRUCT_MPEG4DEFBITRATE[0][1];
            }
            else if (strcmp((char *)pRole->cRole,"video_encoder.h263")==0)
            {
                pCompPortOut->pPortDef->format.video.eCompressionFormat = OMX_VIDEO_CodingH263;
                pCompPortOut->pPortFormat->eCompressionFormat = OMX_VIDEO_CodingH263;
                pCompPortOut->pPortDef->format.video.nBitrate = VIDENC_STRUCT_H263DEFBITRATE[0][1];
            }
            else if (strcmp((char *)pRole->cRole,"video_encoder.avc")==0)
            {
                pCompPortOut->pPortDef->format.video.eCompressionFormat = OMX_VIDEO_CodingAVC;
                pCompPortOut->pPortFormat->eCompressionFormat= OMX_VIDEO_CodingAVC;
                pCompPortOut->pPortDef->format.video.nBitrate = VIDENC_STRUCT_H264DEFBITRATE[0][1];
            }

            pCompPortOut->pPortFormat->eColorFormat = OMX_COLOR_FormatUnused;
            pCompPortOut->pPortDef->eDomain = OMX_PortDomainVideo;
            pCompPortOut->pPortDef->format.video.eColorFormat = OMX_COLOR_FormatUnused;
            pCompPortOut->pPortDef->format.video.nFrameWidth = 176;
            pCompPortOut->pPortDef->format.video.nFrameHeight = 144;
            pCompPortOut->pPortDef->format.video.xFramerate = fToQ16(15.0);
        }
        else
        {
            eError = OMX_ErrorBadParameter;
        }
        break;

    case OMX_IndexParamVideoProfileLevelCurrent:
        {
           VIDEO_PROFILE_LEVEL_TYPE* pProfileLevel = NULL;
           OMX_VIDEO_PARAM_PROFILELEVELTYPE *pParamProfileLevel = (OMX_VIDEO_PARAM_PROFILELEVELTYPE *)pCompParam;

           /* Choose table based on compression format */
           switch(pCompPortOut->pPortDef->format.video.eCompressionFormat)

            {
              case OMX_VIDEO_CodingH263:
                 pProfileLevel = &SupportedH263ProfileLevels;
                 break;
              case OMX_VIDEO_CodingMPEG4:
                 pProfileLevel = &SupportedMPEG4ProfileLevels;
                 break;
              case OMX_VIDEO_CodingAVC:
                 pProfileLevel = &SupportedAVCProfileLevels;
                 break;
              default:
                 eError = OMX_ErrorBadParameter;
                 return eError;
            }
            /* Check validity of profile & level parameters */
            while((pProfileLevel->nProfile != (OMX_S32)pParamProfileLevel->eProfile) ||
               (pProfileLevel->nLevel != (OMX_S32)pParamProfileLevel->eLevel)) {
               pProfileLevel++;
               if(pProfileLevel->nProfile == -1) break;
            }
            if(pProfileLevel->nProfile != -1) {
            /* Update profile & level values in the compression format specific structure */
               switch(pCompPortOut->pPortDef->format.video.eCompressionFormat) {
                  case OMX_VIDEO_CodingH263:
                     pComponentPrivate->pH263->eProfile = pParamProfileLevel->eProfile;
                     pComponentPrivate->pH263->eLevel = pParamProfileLevel->eLevel;
                     break;
                  case OMX_VIDEO_CodingMPEG4:
                     pComponentPrivate->pMpeg4->eProfile = pParamProfileLevel->eProfile;
                     pComponentPrivate->pMpeg4->eLevel = pParamProfileLevel->eLevel;
                     break;
                  case OMX_VIDEO_CodingAVC:
                     pComponentPrivate->pH264->eProfile = pParamProfileLevel->eProfile;
                     pComponentPrivate->pH264->eLevel = pParamProfileLevel->eLevel;
                  default:
                     eError = OMX_ErrorBadParameter;
                     return eError;
                }
                eError = OMX_ErrorNone;
            }
            else {
               eError = OMX_ErrorBadParameter;
            }
        }
        break;
#endif
        /*valid for H264 only*/
        case VideoEncodeCustomParamIndexEncodingPreset:
                pComponentPrivate->nEncodingPreset = (unsigned int)(*((unsigned int*)pCompParam));
            break;
        case VideoEncodeCustomConfigIndexUnrestrictedMV:
            pComponentPrivate->ucUnrestrictedMV = (OMX_U8)(*((OMX_U8*)pCompParam));
            break;
       case VideoEncodeCustomParamIndexNALFormat:
              pComponentPrivate->AVCNALFormat = (VIDENC_AVC_NAL_FORMAT)(*((unsigned int*)pCompParam));
       break;
       //not supported yet
       case OMX_IndexConfigCommonRotate:
       break;

        default:
            eError = OMX_ErrorUnsupportedIndex;
            break;
    }
OMX_CONF_CMD_BAIL:
    return eError;
}

/*----------------------------------------------------------------------------*/
/**
  *  GetConfig() Sets application callbacks to the component
  *
  * This method will update application callbacks
  * the application.
  *
  * @param pComp         handle for this instance of the component
  * @param pCallBacks    application callbacks
  * @param ptr
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  **/
/*----------------------------------------------------------------------------*/

static OMX_ERRORTYPE GetConfig (OMX_HANDLETYPE hComponent,
                                OMX_INDEXTYPE nConfigIndex,
                                OMX_PTR ComponentConfigStructure)
{
    VIDENC_COMPONENT_PRIVATE* pComponentPrivate = NULL;
    OMX_ERRORTYPE eError                        = OMX_ErrorNone;
    VIDENC_NODE* pMemoryListHead                = NULL;
    OMX_U32* pTmp                               = NULL;

    OMX_CONF_CHECK_CMD(hComponent, ((OMX_COMPONENTTYPE *) hComponent)->pComponentPrivate, 1);

    pComponentPrivate = (VIDENC_COMPONENT_PRIVATE*)(((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);
    OMX_DBG_CHECK_CMD(pComponentPrivate->dbg, ComponentConfigStructure, 1, 1);

    if (pComponentPrivate->eState == OMX_StateInvalid)
    {
        OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorIncorrectStateOperation,
                               pComponentPrivate->dbg, OMX_PRSTATE3,
                               "Component is in invalid state.\n");
    }

    pMemoryListHead = pComponentPrivate->pMemoryListHead;

    switch (nConfigIndex)
    {
        case VideoEncodeCustomConfigIndexForceIFrame:
            (*((OMX_BOOL*)ComponentConfigStructure)) = (OMX_BOOL)pComponentPrivate->bForceIFrame;
            break;
        case VideoEncodeCustomConfigIndexIntraFrameInterval:
            (*((OMX_U32*)ComponentConfigStructure)) = (OMX_U32)pComponentPrivate->nIntraFrameInterval;
            break;
        case VideoEncodeCustomConfigIndexTargetFrameRate:
            (*((OMX_U32*)ComponentConfigStructure)) = (OMX_U32)pComponentPrivate->nTargetFrameRate;
            break;
        case VideoEncodeCustomConfigIndexQPI:
            (*((OMX_U32*)ComponentConfigStructure)) = (OMX_U32)pComponentPrivate->nQPI;
            break;
        case VideoEncodeCustomConfigIndexAIRRate:
            (*((OMX_U32*)ComponentConfigStructure)) = (OMX_U32)pComponentPrivate->nAIRRate;
            break;
        /*ASO/FMO*/
        case VideoEncodeCustomConfigIndexNumSliceASO:
            (*((OMX_U32*)ComponentConfigStructure)) = (OMX_U32)pComponentPrivate->numSliceASO;
            break;
        case VideoEncodeCustomConfigIndexAsoSliceOrder:
            (*((OMX_U32*)ComponentConfigStructure)) = (OMX_U32)pComponentPrivate->asoSliceOrder;
            break;
        case VideoEncodeCustomConfigIndexNumSliceGroups:
            (*((OMX_U32*)ComponentConfigStructure)) = (OMX_U32)pComponentPrivate->numSliceGroups;
            break;
        case VideoEncodeCustomConfigIndexSliceGroupMapType:
            (*((OMX_U32*)ComponentConfigStructure)) = (OMX_U32)pComponentPrivate->sliceGroupMapType;
            break;
        case VideoEncodeCustomConfigIndexSliceGroupChangeDirectionFlag:
            (*((OMX_U32*)ComponentConfigStructure)) = (OMX_U32)pComponentPrivate->sliceGroupChangeDirectionFlag;
            break;
        case VideoEncodeCustomConfigIndexSliceGroupChangeRate:
            (*((OMX_U32*)ComponentConfigStructure)) = (OMX_U32)pComponentPrivate->sliceGroupChangeRate;
            break;
        case VideoEncodeCustomConfigIndexSliceGroupChangeCycle:
            (*((OMX_U32*)ComponentConfigStructure)) = (OMX_U32)pComponentPrivate->sliceGroupChangeCycle;
            break;
        case VideoEncodeCustomConfigIndexSliceGroupParams:
            (*((OMX_U32*)ComponentConfigStructure)) = (OMX_U32)pComponentPrivate->sliceGroupParams;
            break;

#ifdef __KHRONOS_CONF_1_1__
    case OMX_IndexConfigVideoFramerate:
            {

            pTmp = memcpy(ComponentConfigStructure,
                          pComponentPrivate->pCompPort[VIDENC_OUTPUT_PORT]->pFrameRateConfig,
                          sizeof(OMX_CONFIG_FRAMERATETYPE));
            if (pTmp == NULL)
            {
                OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorUndefined,
                                       pComponentPrivate->dbg, OMX_TRACE4,
                                       "Failed to copy config.\n");
            }
        }
    break;
    case OMX_IndexConfigVideoBitrate:
            {
            pTmp = memcpy(ComponentConfigStructure,
                          pComponentPrivate->pCompPort[VIDENC_OUTPUT_PORT]->pBitRateTypeConfig,
                          sizeof(OMX_VIDEO_CONFIG_BITRATETYPE));
            if (pTmp == NULL)
            {
                OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorUndefined,
                                       pComponentPrivate->dbg, OMX_TRACE4,
                                       "Failed to copy config.\n");
            }
        }
    break;
#endif
    case OMX_IndexParamVideoMotionVector:
        {
            /* also get parameters in this structure that are tracked outside of it */
            pComponentPrivate->pMotionVector->bFourMV = pComponentPrivate->maxMVperMB >= 4;
            pComponentPrivate->pMotionVector->eAccuracy = OMX_Video_MotionVectorQuarterPel;

            pTmp = memcpy(ComponentConfigStructure,
                          pComponentPrivate->pMotionVector,
                          sizeof(OMX_VIDEO_PARAM_MOTIONVECTORTYPE));
            if (pTmp == NULL)
            {
                OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorUndefined,
                                       pComponentPrivate->dbg, OMX_TRACE4,
                                       "Failed to copy config.\n");
            }
        }
    case OMX_IndexConfigVideoAVCIntraPeriod:
        {
            /* also get parameters in this structure that are tracked outside of it */
            pComponentPrivate->pH264IntraPeriod->nPFrames = pComponentPrivate->nIntraFrameInterval;
            pComponentPrivate->pH264IntraPeriod->nIDRPeriod = 0;

            pTmp = memcpy(ComponentConfigStructure,
                          pComponentPrivate->pH264IntraPeriod,
                          sizeof(OMX_VIDEO_CONFIG_AVCINTRAPERIOD));
            if (pTmp == NULL)
            {
                OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorUndefined,
                                       pComponentPrivate->dbg, OMX_TRACE4,
                                       "Failed to copy structure OMX_VIDEO_CONFIG_AVCINTRAPERIOD.\n");
            }
        }
    case OMX_IndexParamVideoIntraRefresh:
             {
                 pTmp = memcpy(ComponentConfigStructure,
                               pComponentPrivate->pCompPort[VIDENC_OUTPUT_PORT]->pIntraRefreshType,
                               sizeof(OMX_VIDEO_PARAM_INTRAREFRESHTYPE));
                 if (pTmp == NULL)
            {
                OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorUndefined,
                                       pComponentPrivate->dbg, OMX_TRACE4,
                                       "Failed to copy structure OMX_VIDEO_PARAM_INTRAREFRESHTYPE.\n");
            }
        }
    break;
    case OMX_IndexParamVideoErrorCorrection:
            {
            if (((OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE*)(ComponentConfigStructure))->nPortIndex ==
                VIDENC_OUTPUT_PORT)
            {
                pTmp = memcpy(ComponentConfigStructure,
                              pComponentPrivate->pCompPort[VIDENC_OUTPUT_PORT]->pErrorCorrectionType,
                              sizeof(OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE));
                if (pTmp == NULL)
                {
                    OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorUndefined,
                                           pComponentPrivate->dbg, OMX_TRACE4,
                                           "Failed to copy structure OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE.\n");
                }
            }
            else
            {
                eError = OMX_ErrorBadPortIndex;
            }
            break;
        }
    case VideoEncodeCustomConfigIndexDebug:
        OMX_DBG_GETCONFIG(pComponentPrivate->dbg, ComponentConfigStructure);
        break;
    case VideoEncodeCustomConfigIndexMIRRate:
            (*((OMX_U32*)ComponentConfigStructure)) = (OMX_U32)pComponentPrivate->nMIRRate;
            break;
    case VideoEncodeCustomConfigIndexMVDataEnable:
            (*((OMX_BOOL*)ComponentConfigStructure)) = (OMX_BOOL)pComponentPrivate->bMVDataEnable;
             break;
    case VideoEncodeCustomConfigIndexResyncDataEnable:
            (*((OMX_BOOL*)ComponentConfigStructure)) = (OMX_BOOL)pComponentPrivate->bResyncDataEnable;
             break;
    case VideoEncodeCustomConfigIndexMaxMVperMB:
            (*((OMX_U32*)ComponentConfigStructure))  =  (OMX_U32)pComponentPrivate->maxMVperMB;
            break;
    case VideoEncodeCustomConfigIndexIntra4x4EnableIdc:
            (*((IH264VENC_Intra4x4Params*)ComponentConfigStructure)) = (IH264VENC_Intra4x4Params)pComponentPrivate->intra4x4EnableIdc;
         break;
        default:
            eError = OMX_ErrorUnsupportedIndex;
            break;
    }

OMX_CONF_CMD_BAIL:
    return eError;
}

/*----------------------------------------------------------------------------*/
/**
  *  SetConfig() Sets application callbacks to the component
  *
  * This method will update application callbacks
  * the application.
  *
  * @param pComp         handle for this instance of the component
  * @param pCallBacks    application callbacks
  * @param ptr
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  **/
/*----------------------------------------------------------------------------*/

static OMX_ERRORTYPE SetConfig (OMX_HANDLETYPE hComponent,
                                OMX_INDEXTYPE nConfigIndex,
                                OMX_PTR ComponentConfigStructure)
{
    VIDENC_COMPONENT_PRIVATE* pComponentPrivate = NULL;
    OMX_ERRORTYPE eError                        = OMX_ErrorNone;
    VIDENC_NODE* pMemoryListHead                = NULL;
    OMX_U32 i;

    OMX_CONF_CHECK_CMD(hComponent, ((OMX_COMPONENTTYPE *) hComponent)->pComponentPrivate, 1);

    pComponentPrivate = (VIDENC_COMPONENT_PRIVATE*)(((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);
    OMX_DBG_CHECK_CMD(pComponentPrivate->dbg, ComponentConfigStructure, 1, 1);

    if (pComponentPrivate->eState == OMX_StateInvalid)
    {
        OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorIncorrectStateOperation,
                               pComponentPrivate->dbg, OMX_PRSTATE3,
                               "Component is in invalid state.\n");
    }

    pMemoryListHead = pComponentPrivate->pMemoryListHead;

    switch (nConfigIndex)
    {
        case VideoEncodeCustomConfigIndexForceIFrame:
            pComponentPrivate->bForceIFrame = (OMX_BOOL)(*((OMX_BOOL*)ComponentConfigStructure));
            break;
        case VideoEncodeCustomConfigIndexIntraFrameInterval:
            pComponentPrivate->nIntraFrameInterval = (OMX_U32)(*((OMX_U32*)ComponentConfigStructure));
            break;
        case VideoEncodeCustomConfigIndexTargetFrameRate:
            pComponentPrivate->nTargetFrameRate = (OMX_U32)(*((OMX_U32*)ComponentConfigStructure));
            break;
        case VideoEncodeCustomConfigIndexQPI:
            pComponentPrivate->nQPI = (OMX_U32)(*((OMX_U32*)ComponentConfigStructure));
            break;
        case VideoEncodeCustomConfigIndexAIRRate:
            pComponentPrivate->nAIRRate = (OMX_U32)(*((OMX_U32*)ComponentConfigStructure));
            break;
        /*ASO/FMO*/
        case VideoEncodeCustomConfigIndexNumSliceASO:
            pComponentPrivate->numSliceASO = (OMX_U32)(*((OMX_U32*)ComponentConfigStructure));
            break;
        case VideoEncodeCustomConfigIndexAsoSliceOrder:
            for(i=0; i<MAXNUMSLCGPS;i++)
            {
                pComponentPrivate->asoSliceOrder[i] = (OMX_U32)(*((*((OMX_U32**)ComponentConfigStructure))+i));
            }
            break;
        case VideoEncodeCustomConfigIndexNumSliceGroups:
            pComponentPrivate->numSliceGroups  = (OMX_U32)(*((OMX_U32*)ComponentConfigStructure));
            break;
        case VideoEncodeCustomConfigIndexSliceGroupMapType:
            pComponentPrivate->sliceGroupMapType = (OMX_U32)(*((OMX_U32*)ComponentConfigStructure));
            break;
        case VideoEncodeCustomConfigIndexSliceGroupChangeDirectionFlag:
            pComponentPrivate->sliceGroupChangeDirectionFlag = (OMX_U32)(*((OMX_U32*)ComponentConfigStructure));
            break;
        case VideoEncodeCustomConfigIndexSliceGroupChangeRate:
            pComponentPrivate->sliceGroupChangeRate = (OMX_U32)(*((OMX_U32*)ComponentConfigStructure));
            break;
        case VideoEncodeCustomConfigIndexSliceGroupChangeCycle:
            pComponentPrivate->sliceGroupChangeCycle = (OMX_U32)(*((OMX_U32*)ComponentConfigStructure));
            break;
        case VideoEncodeCustomConfigIndexSliceGroupParams:
            for(i=0; i<MAXNUMSLCGPS;i++)
            {
                pComponentPrivate->sliceGroupParams[i] = (OMX_U32)(*((*((OMX_U32**)ComponentConfigStructure))+i));
            }
            break;
    case OMX_IndexConfigVideoFramerate:
    {
        memcpy(pComponentPrivate->pCompPort[VIDENC_OUTPUT_PORT]->pFrameRateConfig,
               ComponentConfigStructure,
               sizeof(OMX_CONFIG_FRAMERATETYPE));
    }
    break;
    case OMX_IndexConfigVideoBitrate:
    {
        memcpy(pComponentPrivate->pCompPort[VIDENC_OUTPUT_PORT]->pBitRateTypeConfig,
               ComponentConfigStructure,
               sizeof(OMX_VIDEO_CONFIG_BITRATETYPE));

        if(!pComponentPrivate->pCompPort[VIDENC_OUTPUT_PORT]->pBitRateTypeConfig->nEncodeBitrate)
        {
            pComponentPrivate->pCompPort[VIDENC_OUTPUT_PORT]->pBitRateTypeConfig->nEncodeBitrate = OMX_VIDENC_GetDefaultBitRate(pComponentPrivate);
        }
        pComponentPrivate->pCompPort[VIDENC_OUTPUT_PORT]->pPortDef->format.video.nBitrate =
        pComponentPrivate->pCompPort[VIDENC_OUTPUT_PORT]->pBitRateType->nTargetBitrate =
        pComponentPrivate->pCompPort[VIDENC_OUTPUT_PORT]->pBitRateTypeConfig->nEncodeBitrate;
    }
    break;
    case OMX_IndexParamVideoErrorCorrection:
    {
        memcpy(pComponentPrivate->pCompPort[VIDENC_OUTPUT_PORT]->pErrorCorrectionType,
               ComponentConfigStructure,
               sizeof(OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE));
    }
    break;
    case OMX_IndexParamVideoIntraRefresh:
        {
        memcpy(pComponentPrivate->pCompPort[VIDENC_OUTPUT_PORT]->pIntraRefreshType,
                ComponentConfigStructure,
                sizeof(OMX_VIDEO_PARAM_INTRAREFRESHTYPE));
        }
        break;
    case OMX_IndexParamVideoMotionVector:
        {
            memcpy(pComponentPrivate->pMotionVector,
                    ComponentConfigStructure,
                    sizeof(OMX_VIDEO_PARAM_MOTIONVECTORTYPE));
            /* also set parameters set by this structure that are tracked outside of it */
            pComponentPrivate->maxMVperMB = pComponentPrivate->pMotionVector->bFourMV ? 4 : 1;

            /* quarter pixel accuracy must be always enabled */
            if (pComponentPrivate->pMotionVector->eAccuracy < OMX_Video_MotionVectorQuarterPel)
                eError = OMX_ErrorBadParameter;
        }
        break;
    case OMX_IndexConfigVideoAVCIntraPeriod:
        {
            memcpy(pComponentPrivate->pH264IntraPeriod,
                    ComponentConfigStructure,
                    sizeof(OMX_VIDEO_CONFIG_AVCINTRAPERIOD));
            /* also set parameters set by this structure that are tracked outside of it */
            pComponentPrivate->nIntraFrameInterval = pComponentPrivate->pH264IntraPeriod->nPFrames;
        }
        break;
    case VideoEncodeCustomConfigIndexDebug:
        OMX_DBG_SETCONFIG(pComponentPrivate->dbg, ComponentConfigStructure);
        break;
    case VideoEncodeCustomConfigIndexMIRRate:
                pComponentPrivate->nMIRRate = (OMX_U32)(*((OMX_U32*)ComponentConfigStructure));
        break;
    case VideoEncodeCustomConfigIndexMVDataEnable:
        pComponentPrivate->bMVDataEnable = (OMX_BOOL)(*((OMX_BOOL*)ComponentConfigStructure));
         break;
    case VideoEncodeCustomConfigIndexResyncDataEnable:
        pComponentPrivate->bResyncDataEnable = (OMX_BOOL)(*((OMX_BOOL*)ComponentConfigStructure));
         break;
    case VideoEncodeCustomConfigIndexMaxMVperMB:
        i = (OMX_U32)(*((OMX_U32*)ComponentConfigStructure));
        if (i==1 || i==4)
            pComponentPrivate->maxMVperMB=i;
        else
            eError = OMX_ErrorBadParameter;
         break;
    case VideoEncodeCustomConfigIndexIntra4x4EnableIdc:
        pComponentPrivate->intra4x4EnableIdc = (IH264VENC_Intra4x4Params)(*((IH264VENC_Intra4x4Params*)ComponentConfigStructure));
         break;
    default:
        eError = OMX_ErrorUnsupportedIndex;
         break;
        }

OMX_CONF_CMD_BAIL:
    return eError;
}

/*----------------------------------------------------------------------------*/
/**
  *  ExtensionIndex()
  *
  *
  *
  *
  * @param pComponent    handle for this instance of the component
  * @param pCallBacks    application callbacks
  * @param ptr
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  **/
/*----------------------------------------------------------------------------*/

static OMX_ERRORTYPE ExtensionIndex(OMX_IN OMX_HANDLETYPE hComponent,
                                       OMX_IN OMX_STRING cParameterName,
                                       OMX_OUT OMX_INDEXTYPE* pIndexType)
{
    VIDENC_CUSTOM_DEFINITION sVideoEncodeCustomIndex[] =
    {
        {"OMX.TI.VideoEncode.Param.VBVSize", VideoEncodeCustomParamIndexVBVSize},
        {"OMX.TI.VideoEncode.Param.DeblockFilter", VideoEncodeCustomParamIndexDeblockFilter},
        {"OMX.TI.VideoEncode.Config.ForceIFrame", VideoEncodeCustomConfigIndexForceIFrame},
        {"OMX.TI.VideoEncode.Config.IntraFrameInterval", VideoEncodeCustomConfigIndexIntraFrameInterval},
        {"OMX.TI.VideoEncode.Config.TargetFrameRate", VideoEncodeCustomConfigIndexTargetFrameRate},
        {"OMX.TI.VideoEncode.Config.QPI", VideoEncodeCustomConfigIndexQPI},
        {"OMX.TI.VideoEncode.Config.AIRRate", VideoEncodeCustomConfigIndexAIRRate},
        {"OMX.TI.VideoEncode.Config.UnrestrictedMV", VideoEncodeCustomConfigIndexUnrestrictedMV},

        /*Segment mode Metadata*/
        {"OMX.TI.VideoEncode.Config.MVDataEnable", VideoEncodeCustomConfigIndexMVDataEnable},
        {"OMX.TI.VideoEncode.Config.ResyncDataEnable", VideoEncodeCustomConfigIndexResyncDataEnable},

        /*ASO*/
        {"OMX.TI.VideoEncode.Config.NumSliceASO", VideoEncodeCustomConfigIndexNumSliceASO},
        {"OMX.TI.VideoEncode.Config.AsoSliceOrder", VideoEncodeCustomConfigIndexAsoSliceOrder},
        /*FMO*/
        {"OMX.TI.VideoEncode.Config.NumSliceGroups", VideoEncodeCustomConfigIndexNumSliceGroups},
        {"OMX.TI.VideoEncode.Config.SliceGroupMapType", VideoEncodeCustomConfigIndexSliceGroupMapType},
        {"OMX.TI.VideoEncode.Config.SliceGroupChangeDirectionFlag", VideoEncodeCustomConfigIndexSliceGroupChangeDirectionFlag},
        {"OMX.TI.VideoEncode.Config.SliceGroupChangeRate", VideoEncodeCustomConfigIndexSliceGroupChangeRate},
        {"OMX.TI.VideoEncode.Config.SliceGroupChangeCycle", VideoEncodeCustomConfigIndexSliceGroupChangeCycle},
        {"OMX.TI.VideoEncode.Config.SliceGroupParams", VideoEncodeCustomConfigIndexSliceGroupParams},
        /**/
        {"OMX.TI.VideoEncode.Config.MIRRate", VideoEncodeCustomConfigIndexMIRRate},
        {"OMX.TI.VideoEncode.Config.MaxMVperMB", VideoEncodeCustomConfigIndexMaxMVperMB},
        {"OMX.TI.VideoEncode.Config.Intra4x4EnableIdc", VideoEncodeCustomConfigIndexIntra4x4EnableIdc},
        {"OMX.TI.VideoEncode.Config.EncodingPreset", VideoEncodeCustomParamIndexEncodingPreset},
        {"OMX.TI.VideoEncode.Config.NALFormat", VideoEncodeCustomParamIndexNALFormat},
        {"OMX.TI.VideoEncode.Debug", VideoEncodeCustomConfigIndexDebug}
    };
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    int nIndex = 0;
    const int nNumberIndices = sizeof(sVideoEncodeCustomIndex)/sizeof(sVideoEncodeCustomIndex[0]);

    if (!hComponent || !pIndexType)
    {
        eError = OMX_ErrorBadParameter;
        goto OMX_CONF_CMD_BAIL;
    }

    for (nIndex = 0; nIndex < nNumberIndices; nIndex++)
    {
        if (!strcmp((const char*)cParameterName, (const char*)(&(sVideoEncodeCustomIndex[nIndex].cCustomName))))
        {
            *pIndexType = sVideoEncodeCustomIndex[nIndex].nCustomIndex;
            eError = OMX_ErrorNone;
            break;
        }
    }

OMX_CONF_CMD_BAIL:
    return eError;
}

/*----------------------------------------------------------------------------*/
/**
  *  GetState() Sets application callbacks to the component
  *
  * This method will update application callbacks
  * the application.
  *
  * @param pComp         handle for this instance of the component
  * @param pCallBacks    application callbacks
  * @param ptr
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  **/
/*----------------------------------------------------------------------------*/

static OMX_ERRORTYPE GetState (OMX_IN OMX_HANDLETYPE hComponent,
                               OMX_OUT OMX_STATETYPE* pState)
{
    OMX_ERRORTYPE eError                        = OMX_ErrorNone;
    OMX_COMPONENTTYPE* pHandle = NULL;
    VIDENC_COMPONENT_PRIVATE* pComponentPrivate = NULL;
    struct timespec abs_time = {0,0};
    int nPendingStateChangeRequests = 0;
    int ret = 0;
    /* Set to sufficiently high value */
    int mutex_timeout = 3;

    if(hComponent == NULL || pState == NULL) {
        return OMX_ErrorBadParameter;
    }

    pHandle = (OMX_COMPONENTTYPE*)hComponent;
    pComponentPrivate = (VIDENC_COMPONENT_PRIVATE*)pHandle->pComponentPrivate;

    /* Retrieve current state */
    if (pHandle && pHandle->pComponentPrivate) {
        /* Check for any pending state transition requests */
        if(pthread_mutex_lock(&pComponentPrivate->mutexStateChangeRequest)) {
            return OMX_ErrorUndefined;
        }
        nPendingStateChangeRequests = pComponentPrivate->nPendingStateChangeRequests;
        if(!nPendingStateChangeRequests) {
           if(pthread_mutex_unlock(&pComponentPrivate->mutexStateChangeRequest)) {
               return OMX_ErrorUndefined;
           }

           /* No pending state transitions */
           *pState = ((VIDENC_COMPONENT_PRIVATE*)pHandle->pComponentPrivate)->eState;
           eError = OMX_ErrorNone;
        }
        else {
                  /* Wait for component to complete state transition */
           clock_gettime(CLOCK_REALTIME, &abs_time);
           abs_time.tv_sec += mutex_timeout;
           abs_time.tv_nsec = 0;
          ret = pthread_cond_timedwait(&(pComponentPrivate->StateChangeCondition), &(pComponentPrivate->mutexStateChangeRequest), &abs_time);
           if (!ret) {
              /* Component has completed state transitions*/
              *pState = ((VIDENC_COMPONENT_PRIVATE*)pHandle->pComponentPrivate)->eState;
              if(pthread_mutex_unlock(&pComponentPrivate->mutexStateChangeRequest)) {
                 return OMX_ErrorUndefined;
              }
              eError = OMX_ErrorNone;
           }
           else if(ret == ETIMEDOUT) {
              /* Unlock mutex in case of timeout */
              OMX_ERROR4(pComponentPrivate->dbg, "GetState timed out\n");
              pthread_mutex_unlock(&pComponentPrivate->mutexStateChangeRequest);
              *pState = OMX_StateInvalid;
              return OMX_ErrorNone;
           }
        }
    }
     else {
        eError = OMX_ErrorInvalidComponent;
        *pState = OMX_StateInvalid;
    }

    return eError;
}

/*----------------------------------------------------------------------------*/
/**
  *  EmptyThisBuffer() Sets application callbacks to the component
  *
  * This method will update application callbacks
  * the application.
  *
  * @param pComp         handle for this instance of the component
  * @param pCallBacks    application callbacks
  * @param ptr
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  **/
/*----------------------------------------------------------------------------*/

static OMX_ERRORTYPE EmptyThisBuffer (OMX_IN OMX_HANDLETYPE hComponent,
                                      OMX_IN OMX_BUFFERHEADERTYPE* pBufHead)
{
    OMX_ERRORTYPE eError                        = OMX_ErrorNone;
    VIDENC_COMPONENT_PRIVATE* pComponentPrivate = NULL;
    int nRet                                    = 0;
    OMX_HANDLETYPE hTunnelComponent             = NULL;
    VIDENC_BUFFER_PRIVATE* pBufferPrivate       = NULL;
    VIDENC_NODE* pMemoryListHead                = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDefIn    = NULL;

    OMX_CONF_CHECK_CMD(hComponent, ((OMX_COMPONENTTYPE *) hComponent)->pComponentPrivate, 1);

    pComponentPrivate = (VIDENC_COMPONENT_PRIVATE*)(((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);
    OMX_DBG_CHECK_CMD(pComponentPrivate->dbg, pBufHead, 1, 1);

    hTunnelComponent = pComponentPrivate->pCompPort[VIDENC_INPUT_PORT]->hTunnelComponent;

    pComponentPrivate->pMarkData = pBufHead->pMarkData;
    pComponentPrivate->hMarkTargetComponent = pBufHead->hMarkTargetComponent;
    pPortDefIn = pComponentPrivate->pCompPort[VIDENC_INPUT_PORT]->pPortDef;
    if (!(pPortDefIn->bEnabled))
    {
        OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorIncorrectStateOperation,
                               pComponentPrivate->dbg, OMX_PRBUFFER4,
                               "Emptying buffer on disabled port.\n");
    }

    if(!hTunnelComponent)
    {
        if (pBufHead->nInputPortIndex != 0x0  ||
            pBufHead->nOutputPortIndex != OMX_NOPORT)
        {
            OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorBadPortIndex,
                                   pComponentPrivate->dbg, OMX_PRBUFFER4,
                                   "Emptying buffer on invalid port.\n");
        }

        if (pComponentPrivate->eState != OMX_StateExecuting &&
            pComponentPrivate->eState != OMX_StatePause)
        {
            OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorIncorrectStateOperation,
                                   pComponentPrivate->dbg, OMX_PRBUFFER4,
                                   "Emptying buffer in invalid state (%d).\n", pComponentPrivate->eState);
        }
    }

    OMX_CONF_CHK_VERSION(pBufHead, OMX_BUFFERHEADERTYPE, eError);
    pMemoryListHead = pComponentPrivate->pMemoryListHead;


#ifdef __PERF_INSTRUMENTATION__
    PERF_ReceivedFrame(pComponentPrivate->pPERF,
                       pBufHead->pBuffer,
                       pBufHead->nFilledLen,
                       PERF_ModuleHLMM);
#endif

#ifndef UNDER_CE
    if (pthread_mutex_lock(&(pComponentPrivate->mVideoEncodeBufferMutex)) != 0)
    {
        OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorHardware,
                               pComponentPrivate->dbg, OMX_TRACE4,
                               "pthread_mutex_lock() failed.\n");
    }
    pBufferPrivate = pBufHead->pInputPortPrivate;


    pBufferPrivate->eBufferOwner = VIDENC_BUFFER_WITH_COMPONENT;
    pBufferPrivate->bReadFromPipe = OMX_FALSE;
    /* Check if frame rate needs to be updated */
    pComponentPrivate->nFrameCount++;
    pComponentPrivate->nVideoTime = pBufHead->nTimeStamp - pComponentPrivate->nLastUpdateTime;

    if (pComponentPrivate->nFrameCount == pComponentPrivate->nFrameRateUpdateInterval) {

         if(pComponentPrivate->nVideoTime <= 0) {
            /* Incorrect time stamps */
            return OMX_ErrorBadParameter;
         }

        /* Timestamps are in micro seconds  */
        pComponentPrivate->nTargetFrameRate = pComponentPrivate->nFrameCount * 1000000 / pComponentPrivate->nVideoTime;

        if(pComponentPrivate->pCompPort[VIDENC_OUTPUT_PORT]->pPortDef->format.video.eCompressionFormat == OMX_VIDEO_CodingAVC) {
             /* H.264 Socket Node expects frame rate to be multiplied by 1000 */
             pComponentPrivate->nTargetFrameRate *= 1000;

             if((pComponentPrivate->nPrevTargetFrameRate) &&
                (pComponentPrivate->nTargetFrameRate ==  pComponentPrivate->nPrevTargetFrameRate)) {
                  pComponentPrivate->bForceIFrame = OMX_TRUE;
             }
             else {
                  pComponentPrivate->nPrevTargetFrameRate = pComponentPrivate->nTargetFrameRate;
                  pComponentPrivate->bForceIFrame = OMX_FALSE;
             }
        }
        pComponentPrivate->nLastUpdateTime = pBufHead->nTimeStamp;
        pComponentPrivate->nFrameCount = 0;
    }

    nRet = write(pComponentPrivate->nFilled_iPipe[1],
                 &(pBufHead),
                 sizeof(pBufHead));
    if (nRet == -1)
    {
        pthread_mutex_unlock(&(pComponentPrivate->mVideoEncodeBufferMutex));
        OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorHardware,
                               pComponentPrivate->dbg, OMX_PRBUFFER4,
                               "failed to write to nFilled_iPipe.\n");
    } else {
        OMX_VIDENC_IncrementBufferCountByOne(&pComponentPrivate->EmptythisbufferCount);
    }
    if (pthread_mutex_unlock(&(pComponentPrivate->mVideoEncodeBufferMutex)) != 0)
    {
        OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorHardware,
                               pComponentPrivate->dbg, OMX_TRACE4,
                               "pthread_mutex_unlock() failed.\n");
    }
#else
    pBufferPrivate->eBufferOwner = VIDENC_BUFFER_WITH_COMPONENT;
    pBufferPrivate->bReadFromPipe = OMX_FALSE;
    nRet = write(pComponentPrivate->nFilled_iPipe[1],
                 &(pBufHead),
                 sizeof(pBufHead));
    if (nRet == -1)
    {
        OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorHardware,
                               pComponentPrivate->dbg, OMX_PRBUFFER4,
                               "failed to write to nFilled_iPipe.\n");
    }
#endif

OMX_CONF_CMD_BAIL:
    return eError;
}

/*----------------------------------------------------------------------------*/
/**
  *  FillThisBuffer() Sets application callbacks to the component
  *
  * This method will update application callbacks
  * the application.
  *
  * @param pComp         handle for this instance of the component
  * @param pCallBacks    application callbacks
  * @param ptr
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  **/
/*----------------------------------------------------------------------------*/
static OMX_ERRORTYPE FillThisBuffer (OMX_IN OMX_HANDLETYPE hComponent,
                                     OMX_IN OMX_BUFFERHEADERTYPE* pBufHead)
{
    OMX_ERRORTYPE eError                        = OMX_ErrorNone;
    VIDENC_COMPONENT_PRIVATE* pComponentPrivate = NULL;
    int nRet                                    = 0;
    VIDENC_BUFFER_PRIVATE* pBufferPrivate       = NULL;
    OMX_HANDLETYPE hTunnelComponent             = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDefOut   = NULL;

    OMX_CONF_CHECK_CMD(hComponent, ((OMX_COMPONENTTYPE *) hComponent)->pComponentPrivate, 1);

    pComponentPrivate = (VIDENC_COMPONENT_PRIVATE*)(((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);
    OMX_DBG_CHECK_CMD(pComponentPrivate->dbg, pBufHead, 1, 1);


    hTunnelComponent = pComponentPrivate->pCompPort[VIDENC_OUTPUT_PORT]->hTunnelComponent;
    pPortDefOut = pComponentPrivate->pCompPort[VIDENC_OUTPUT_PORT]->pPortDef;
    pBufHead->nFilledLen = 0;
    pBufferPrivate = (VIDENC_BUFFER_PRIVATE*)pBufHead->pOutputPortPrivate;

    if (!(pPortDefOut->bEnabled))
    {
        OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorIncorrectStateOperation,
                               pComponentPrivate->dbg, OMX_PRBUFFER4,
                               "Filling buffer on disabled port.\n");
    }

    if(!hTunnelComponent)
    {
        if (pBufHead->nOutputPortIndex != 0x1  ||
            pBufHead->nInputPortIndex != OMX_NOPORT)
        {
            OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorBadPortIndex,
                                   pComponentPrivate->dbg, OMX_PRBUFFER4,
                                   "Filling buffer on invalid port.\n");
        }

        if (pComponentPrivate->eState != OMX_StateExecuting &&
            pComponentPrivate->eState != OMX_StatePause)
        {
            OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorIncorrectStateOperation,
                                   pComponentPrivate->dbg, OMX_PRBUFFER4,
                                   "Filling buffer in invalid state (%d).\n", pComponentPrivate->eState);
        }
    }
    OMX_CONF_CHK_VERSION(pBufHead, OMX_BUFFERHEADERTYPE, eError);
#ifdef __PERF_INSTRUMENTATION__
    PERF_ReceivedFrame(pComponentPrivate->pPERF,
                       pBufHead->pBuffer,
                       0,
                       PERF_ModuleHLMM);
#endif

    if (pComponentPrivate->pMarkBuf)
    {
        pBufHead->hMarkTargetComponent = pComponentPrivate->pMarkBuf->hMarkTargetComponent;
        pBufHead->pMarkData = pComponentPrivate->pMarkBuf->pMarkData;
        pComponentPrivate->pMarkBuf = NULL;
    }

    if (pComponentPrivate->pMarkData)
    {
        pBufHead->hMarkTargetComponent = pComponentPrivate->hMarkTargetComponent;
        pBufHead->pMarkData = pComponentPrivate->pMarkData;
        pComponentPrivate->pMarkData = NULL;
    }

#ifndef UNDER_CE
    if (pthread_mutex_lock(&(pComponentPrivate->mVideoEncodeBufferMutex)) != 0)
    {
        OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorHardware,
                               pComponentPrivate->dbg, OMX_TRACE4,
                               "pthread_mutex_lock() failed.\n");
    }


    pBufferPrivate->eBufferOwner = VIDENC_BUFFER_WITH_COMPONENT;
    pBufferPrivate->bReadFromPipe = OMX_FALSE;
    nRet = write(pComponentPrivate->nFree_oPipe[1],
                 &(pBufHead),
                 sizeof (pBufHead));
    if (nRet == -1)
    {
        pthread_mutex_unlock(&(pComponentPrivate->mVideoEncodeBufferMutex));
        OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorHardware,
                               pComponentPrivate->dbg, OMX_PRBUFFER4,
                               "failed to write to nFree_oPipe.\n");
    } else {
        OMX_VIDENC_IncrementBufferCountByOne(&pComponentPrivate->FillthisbufferCount);
    }
    if (pthread_mutex_unlock(&(pComponentPrivate->mVideoEncodeBufferMutex)) != 0)
    {
        OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorHardware,
                               pComponentPrivate->dbg, OMX_TRACE4,
                               "pthread_mutex_unlock() failed.\n");
    }
#else

    pBufferPrivate->eBufferOwner = VIDENC_BUFFER_WITH_COMPONENT;
    pBufferPrivate->bReadFromPipe = OMX_FALSE;
    nRet = write(pComponentPrivate->nFree_oPipe[1],
                 &(pBufHead),
                 sizeof (pBufHead));
    if (nRet == -1)
    {
        OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorHardware,
                               pComponentPrivate->dbg, OMX_PRBUFFER4,
                               "failed to write to nFree_oPipe.\n");
    }
#endif
OMX_CONF_CMD_BAIL:
    return eError;
}
/*----------------------------------------------------------------------------*/
/**
  * OMX_ComponentDeInit() Sets application callbacks to the component
  *
  * This method will update application callbacks
  * the application.
  *
  * @param pComp         handle for this instance of the component
  * @param pCallBacks    application callbacks
  * @param ptr
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  **/
/*----------------------------------------------------------------------------*/

static OMX_ERRORTYPE ComponentDeInit(OMX_IN OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE eError                        = OMX_ErrorNone;
    VIDENC_COMPONENT_PRIVATE* pComponentPrivate = NULL;
    LCML_DSP_INTERFACE* pLcmlHandle             = NULL;
    VIDENC_NODE* pMemoryListHead                = NULL;
    OMX_ERRORTYPE eErr  = OMX_ErrorNone;
    OMX_S32 nRet        = -1;
    OMX_S32 nStop       = -1;
    OMX_U32 nTimeout    = 0;
    struct OMX_TI_Debug dbg;

    OMX_CONF_CHECK_CMD(hComponent, ((OMX_COMPONENTTYPE *) hComponent)->pComponentPrivate, 1);
    pComponentPrivate = (VIDENC_COMPONENT_PRIVATE*)(((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);

    dbg = pComponentPrivate->dbg;

    pMemoryListHead=pComponentPrivate->pMemoryListHead;

#ifdef __PERF_INSTRUMENTATION__
    PERF_Boundary(pComponentPrivate->pPERF,
                  PERF_BoundaryStart | PERF_BoundaryCleanup);
#endif
    while(1)
    {
        if(!(pComponentPrivate->bHandlingFatalError))
        {
            if(!(pComponentPrivate->bErrorLcmlHandle) &&
               !(pComponentPrivate->bUnresponsiveDsp))
            { /* Add for ResourceExhaustionTest*/
                pLcmlHandle = pComponentPrivate->pLCML;
                if (pLcmlHandle != NULL)
                {
                    if (pComponentPrivate->bCodecStarted == OMX_TRUE ||
                    pComponentPrivate->bCodecLoaded == OMX_TRUE)
                    {
                        eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                                   EMMCodecControlDestroy,
                                                   NULL);
                        if (eError != OMX_ErrorNone)
                        {
                            OMX_PRDSP4(dbg, "error when requesting EMMCodecControlDestroy");
                            eError = OMX_ErrorUndefined;
                        }

#ifdef UNDER_CE
                        FreeLibrary(g_hLcmlDllHandle);
                        g_hLcmlDllHandle = NULL;
#endif
                    }
                }
            }
            break;
        }
        if(nTimeout++ > VIDENC_MAX_COMPONENT_TIMEOUT)
        {
            OMX_ERROR5(dbg, "TimeOut in HandlingFatalError!\n");
            break;
        }
        sched_yield();
    }


        /*Unload LCML */
    if(pComponentPrivate->pModLcml != NULL)
    {
#ifndef UNDER_CE
        dlclose(pComponentPrivate->pModLcml);
#else
        FreeLibrary(pComponentPrivate->pModLcml);
#endif
       pComponentPrivate->pModLcml = NULL;
       pComponentPrivate->pLCML = NULL;
    }


    pComponentPrivate->bCodecStarted = OMX_FALSE;

    nStop = -1;
#ifdef __PERF_INSTRUMENTATION__
    PERF_SendingCommand(pComponentPrivate->pPERF, nStop, 0, PERF_ModuleComponent);
#endif
    OMX_PRCOMM2(dbg, "eCmd: -1 Send\n");
    nRet = write(pComponentPrivate->nCmdPipe[1],
                 &nStop,
                 sizeof(OMX_COMMANDTYPE));

    /*Join the component thread*/
    /*pthread_cancel(ComponentThread);*/
#ifdef UNDER_CE
    eErr = pthread_join(ComponentThread, NULL);
#else
    eErr = pthread_join(pComponentPrivate->ComponentThread, NULL);
#endif
    if (eErr != 0)
    {
        eError = OMX_ErrorHardware;
        OMX_TRACE4(dbg, "Error pthread_join (%d).\n", eErr);
    }

    /*close the data pipe handles*/
    eErr = close(pComponentPrivate->nFree_oPipe[0]);
    if (0 != eErr && OMX_ErrorNone == eError)
    {
        eError = OMX_ErrorHardware;
        OMX_PRBUFFER4(dbg, "Error while closing data pipe (%d).\n", eErr);
    }

    eErr = close(pComponentPrivate->nFilled_iPipe[0]);
    if (0 != eErr && OMX_ErrorNone == eError)
    {
        eError = OMX_ErrorHardware;
        OMX_PRBUFFER4(dbg, "Error while closing data pipe (%d).\n", eErr);
    }

    eErr = close(pComponentPrivate->nFree_oPipe[1]);
    if (0 != eErr && OMX_ErrorNone == eError)
    {
        eError = OMX_ErrorHardware;
        OMX_PRBUFFER4(dbg, "Error while closing data pipe (%d).\n", eErr);
    }

    eErr = close(pComponentPrivate->nFilled_iPipe[1]);
    if (0 != eErr && OMX_ErrorNone == eError)
    {
        eError = OMX_ErrorHardware;
        OMX_PRBUFFER4(dbg, "Error while closing data pipe (%d).\n", eErr);
    }

    /*Close the command pipe handles*/
    eErr = close(pComponentPrivate->nCmdPipe[0]);
    if (0 != eErr && OMX_ErrorNone == eError)
    {
        eError = OMX_ErrorHardware;
        OMX_PRCOMM4(dbg, "Error while closing data pipe (%d).\n", eErr);
    }

    eErr = close(pComponentPrivate->nCmdPipe[1]);
    if (0 != eErr && OMX_ErrorNone == eError)
    {
        eError = OMX_ErrorHardware;
        OMX_PRCOMM4(dbg, "Error while closing data pipe (%d).\n", eErr);
    }
    /*Close the command data pipe handles*/
    eErr = close(pComponentPrivate->nCmdDataPipe[0]);
    if (0 != eErr && OMX_ErrorNone == eError)
    {
        eError = OMX_ErrorHardware;
        OMX_PRCOMM4(dbg, "Error while closing data pipe (%d).\n", eErr);
    }

    eErr = close(pComponentPrivate->nCmdDataPipe[1]);
    if (0 != eErr && OMX_ErrorNone == eError)
    {
        eError = OMX_ErrorHardware;
        OMX_PRCOMM4(dbg, "Error while closing data pipe (%d).\n", eErr);
    }

    OMX_PRINT2(dbg, "pipes closed...\n");

#ifndef UNDER_CE
    OMX_TRACE2(dbg, "destroy mVideoEncodeBufferMutex -> %p\n",
              &(pComponentPrivate->mVideoEncodeBufferMutex));
    /* Destroy Mutex for Buffer Tracking */
    nRet = pthread_mutex_destroy(&(pComponentPrivate->mVideoEncodeBufferMutex));
    while (nRet == EBUSY)
    {
        /* The mutex is busy. We need to unlock it, then destroy it. */
        OMX_TRACE2(dbg, "destroy status = EBUSY\n");
        pthread_mutex_unlock(&(pComponentPrivate->mVideoEncodeBufferMutex));
        nRet = pthread_mutex_destroy(&(pComponentPrivate->mVideoEncodeBufferMutex));
    }
#else
    /* Add WinCE critical section API here... */
#endif
#ifndef UNDER_CE
    pthread_mutex_destroy(&pComponentPrivate->videoe_mutex);
    pthread_cond_destroy(&pComponentPrivate->populate_cond);
    pthread_mutex_destroy(&pComponentPrivate->videoe_mutex_app);
    pthread_cond_destroy(&pComponentPrivate->unpopulate_cond);
    pthread_cond_destroy(&pComponentPrivate->flush_cond);
    pthread_cond_destroy(&pComponentPrivate->stop_cond);
    pthread_mutex_destroy(&bufferReturned_mutex);
    pthread_cond_destroy(&bufferReturned_condition);
#else
    OMX_DestroyEvent(&(pComponentPrivate->InLoaded_event));
    OMX_DestroyEvent(&(pComponentPrivate->InIdle_event));
#endif

#ifdef RESOURCE_MANAGER_ENABLED
    /* Deinitialize Resource Manager */
    eError = RMProxy_DeinitalizeEx(OMX_COMPONENTTYPE_VIDEO);
    if (eError != OMX_ErrorNone)
    {
        OMX_PRMGR4(dbg, "Error returned from destroy ResourceManagerProxy thread\n");
        eError = OMX_ErrorUndefined;
    }
#endif

#ifdef __PERF_INSTRUMENTATION__
    PERF_Boundary(pComponentPrivate->pPERF,
                  PERF_BoundaryComplete | PERF_BoundaryCleanup);
    PERF_Done(pComponentPrivate->pPERF);
#endif

    pthread_mutex_destroy(&pComponentPrivate->mutexStateChangeRequest);
    pthread_cond_destroy(&pComponentPrivate->StateChangeCondition);

    if (pComponentPrivate != NULL)
    {
        VIDENC_FREE(pComponentPrivate, pMemoryListHead, dbg);
    }

    /* Free Resources */
    OMX_VIDENC_ListDestroy(&dbg, pMemoryListHead);

    OMX_DBG_CLOSE(dbg);

OMX_CONF_CMD_BAIL:
    return eError;
}

/*----------------------------------------------------------------------------*/
/**
  *  UseBuffer()
  *
  *
  *
  *
  * @param
  * @param
  * @param
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  **/
/*----------------------------------------------------------------------------*/

OMX_ERRORTYPE UseBuffer(OMX_IN OMX_HANDLETYPE hComponent,
                        OMX_INOUT OMX_BUFFERHEADERTYPE** ppBufferHdr,
                        OMX_IN OMX_U32 nPortIndex,
                        OMX_IN OMX_PTR pAppPrivate,
                        OMX_IN OMX_U32 nSizeBytes,
                        OMX_IN OMX_U8* pBuffer)
{
    VIDENC_COMPONENT_PRIVATE* pComponentPrivate = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDef      = NULL;
    VIDEOENC_PORT_TYPE* pCompPort               = NULL;
    VIDENC_BUFFER_PRIVATE* pBufferPrivate       = NULL;
    OMX_U32 nBufferCnt      = -1;
    OMX_ERRORTYPE eError    = OMX_ErrorNone;
    OMX_HANDLETYPE hTunnelComponent = NULL;
    VIDENC_NODE* pMemoryListHead    = NULL;

    OMX_CONF_CHECK_CMD(hComponent, ((OMX_COMPONENTTYPE *) hComponent)->pComponentPrivate, 1);

    pComponentPrivate = (VIDENC_COMPONENT_PRIVATE*)(((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);
    OMX_DBG_CHECK_CMD(pComponentPrivate->dbg, ppBufferHdr, pBuffer, 1);

    if (nPortIndex == VIDENC_INPUT_PORT)
    {
       pPortDef = pComponentPrivate->pCompPort[VIDENC_INPUT_PORT]->pPortDef;
    }
    else if (nPortIndex == VIDENC_OUTPUT_PORT)
    {
        pPortDef = pComponentPrivate->pCompPort[VIDENC_OUTPUT_PORT]->pPortDef;
    }
    else
    {
        OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorBadParameter,
                               pComponentPrivate->dbg, OMX_PRBUFFER4,
                               "Using buffer on invalid port index.\n");
    }

    if (!pPortDef->bEnabled)
    {
        OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorIncorrectStateOperation,
                               pComponentPrivate->dbg, OMX_PRBUFFER4,
                               "Using buffer on disabled port.\n");
    }

    if (pPortDef->bPopulated)
    {
        OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorBadParameter,
                               pComponentPrivate->dbg, OMX_PRBUFFER4,
                               "Allocating duplicate buffer\n");
    }
    if (nSizeBytes < pPortDef->nBufferSize)
    {
        OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorBadParameter,
            pComponentPrivate->dbg, OMX_PRBUFFER4,
            "Allocating invalid size buffer: nBufferSize: %lu nSizeBytes: %lu\n", pPortDef->nBufferSize, nSizeBytes);
    }


    if (pComponentPrivate->eState == OMX_StateInvalid)
    {
        OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorInvalidState,
                               pComponentPrivate->dbg, OMX_PRBUFFER4,
                               "Using buffer in invalid state.\n");
    }

    pMemoryListHead=pComponentPrivate->pMemoryListHead;


#ifdef __PERF_INSTRUMENTATION__
    PERF_ReceivedBuffer(pComponentPrivate->pPERF,
                        pBuffer, nSizeBytes,
                        PERF_ModuleHLMM);
#endif

    nBufferCnt       = pComponentPrivate->pCompPort[nPortIndex]->nBufferCnt;
    hTunnelComponent = pComponentPrivate->pCompPort[VIDENC_INPUT_PORT]->hTunnelComponent;
    pCompPort        = pComponentPrivate->pCompPort[nPortIndex];
    pBufferPrivate   = pCompPort->pBufferPrivate[nBufferCnt];

    VIDENC_MALLOC(pBufferPrivate->pBufferHdr,
                  sizeof(OMX_BUFFERHEADERTYPE),
                  OMX_BUFFERHEADERTYPE, pMemoryListHead, pComponentPrivate->dbg);

    pBufferPrivate->pBufferHdr->nSize       = sizeof(OMX_BUFFERHEADERTYPE);
    pBufferPrivate->pBufferHdr->nVersion    = pPortDef->nVersion;
    pBufferPrivate->pBufferHdr->pBuffer     = pBuffer;
    pBufferPrivate->pBufferHdr->pAppPrivate = pAppPrivate;
    pBufferPrivate->pBufferHdr->nAllocLen   = nSizeBytes;

    if (hTunnelComponent != NULL)
    {
        /* set direction dependent fields */
        if (pPortDef->eDir == OMX_DirInput)
        {
            pBufferPrivate->pBufferHdr->nInputPortIndex  = nPortIndex;
            pBufferPrivate->pBufferHdr->nOutputPortIndex = pCompPort->nTunnelPort;
        }
        else
        {
            pBufferPrivate->pBufferHdr->nInputPortIndex  = pCompPort->nTunnelPort;
            pBufferPrivate->pBufferHdr->nOutputPortIndex = nPortIndex;
        }
    }
    else
    {
        if (nPortIndex == VIDENC_INPUT_PORT)
        {
            pBufferPrivate->pBufferHdr->nInputPortIndex  = VIDENC_INPUT_PORT;
            pBufferPrivate->pBufferHdr->nOutputPortIndex = OMX_NOPORT;
        }
        else
        {
            pBufferPrivate->pBufferHdr->nInputPortIndex  = OMX_NOPORT;
            pBufferPrivate->pBufferHdr->nOutputPortIndex = VIDENC_OUTPUT_PORT;
        }
    }
    *ppBufferHdr = pBufferPrivate->pBufferHdr;
    pBufferPrivate->pBufferHdr = pBufferPrivate->pBufferHdr;

    if (nPortIndex == VIDENC_INPUT_PORT)
    {
        pBufferPrivate->pBufferHdr->pInputPortPrivate = pBufferPrivate;
        if(!pComponentPrivate->nInBufferSize || (pComponentPrivate->nInBufferSize > nSizeBytes)) {
            pComponentPrivate->nInBufferSize = nSizeBytes;
        }
    }
    else
    {
       pBufferPrivate->pBufferHdr->pOutputPortPrivate = pBufferPrivate;
        if(!pComponentPrivate->nOutBufferSize || (pComponentPrivate->nOutBufferSize > nSizeBytes)) {
            pComponentPrivate->nOutBufferSize = nSizeBytes;
        }
    }
    pBufferPrivate->bAllocByComponent = OMX_FALSE;

    if (hTunnelComponent != NULL)
    {
        pBufferPrivate->eBufferOwner = VIDENC_BUFFER_WITH_TUNNELEDCOMP;
    }
    else
    {
        pBufferPrivate->eBufferOwner = VIDENC_BUFFER_WITH_CLIENT;
    }

    eError = OMX_VIDENC_Allocate_DSPResources(pComponentPrivate, nPortIndex);
    OMX_DBG_BAIL_IF_ERROR(eError, pComponentPrivate->dbg, OMX_PRDSP4,
                          "Failed to allocate DSP resources.\n");

    OMX_CONF_CIRCULAR_BUFFER_ADD_NODE(pComponentPrivate,
                                      pComponentPrivate->sCircularBuffer);
    pCompPort->nBufferCnt++;
    if(pCompPort->nBufferCnt == pPortDef->nBufferCountActual)
    {
#ifndef UNDER_CE
        pthread_mutex_lock(&pComponentPrivate->videoe_mutex_app);
        pPortDef->bPopulated = OMX_TRUE;
        pthread_cond_signal(&pComponentPrivate->populate_cond);
        pthread_mutex_unlock(&pComponentPrivate->videoe_mutex_app);
#else
        pPortDef->bPopulated = OMX_TRUE;
        OMX_SignalEvent(&(pComponentPrivate->InLoaded_event));
#endif
    }
OMX_CONF_CMD_BAIL:
    return eError;
}

/*----------------------------------------------------------------------------*/
/**
  *  FreeBuffer()
  *
  *
  *
  *
  * @param
  * @param
  * @param
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  **/
/*----------------------------------------------------------------------------*/

OMX_ERRORTYPE FreeBuffer(OMX_IN  OMX_HANDLETYPE hComponent,
                         OMX_IN  OMX_U32 nPortIndex,
                         OMX_IN  OMX_BUFFERHEADERTYPE* pBufHead)
{
    OMX_ERRORTYPE eError                        = OMX_ErrorNone;
    OMX_COMPONENTTYPE* pHandle                  = NULL;
    VIDENC_COMPONENT_PRIVATE* pComponentPrivate = NULL;
    char* pTemp                                 = NULL;
    VIDEOENC_PORT_TYPE* pCompPort               = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDef      = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDefOut   = NULL;
    OMX_VIDEO_CODINGTYPE eCompressionFormat     = -1;
    OMX_U32 nBufferCnt                          = -1;
    OMX_U8 nCount                               = 0;
    VIDENC_BUFFER_PRIVATE* pBufferPrivate       = NULL;
    VIDENC_NODE* pMemoryListHead                = NULL;

    OMX_CONF_CHECK_CMD(hComponent, ((OMX_COMPONENTTYPE *) hComponent)->pComponentPrivate, 1);

    pComponentPrivate = (VIDENC_COMPONENT_PRIVATE*)(((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);
    OMX_DBG_CHECK_CMD(pComponentPrivate->dbg, pBufHead, 1, 1);
    /*OMX_CONF_CHK_VERSION(pBufHead, OMX_BUFFERHEADERTYPE, eError); Makes CONF_FlushTest Fail*/

    pHandle = (OMX_COMPONENTTYPE*)hComponent;

    pMemoryListHead = pComponentPrivate->pMemoryListHead;
    pCompPort = pComponentPrivate->pCompPort[nPortIndex];
    pPortDefOut = pComponentPrivate->pCompPort[VIDENC_OUTPUT_PORT]->pPortDef;
    pPortDef = pComponentPrivate->pCompPort[nPortIndex]->pPortDef;
    nBufferCnt = pComponentPrivate->pCompPort[nPortIndex]->nBufferCnt;

    eCompressionFormat = pPortDefOut->format.video.eCompressionFormat;

    if (nPortIndex == VIDENC_INPUT_PORT)
    {
        pBufferPrivate = pBufHead->pInputPortPrivate;
        if (pBufferPrivate != NULL)
        {
            if (pBufferPrivate->pUalgParam != NULL)
            {
                pTemp = (char*)pBufferPrivate->pUalgParam;
                pTemp -= 128;
                if (eCompressionFormat == OMX_VIDEO_CodingAVC)
                {
                    pBufferPrivate->pUalgParam = (H264VE_GPP_SN_UALGInputParams*)pTemp;
                }
                else if (eCompressionFormat == OMX_VIDEO_CodingMPEG4 ||
                         eCompressionFormat ==OMX_VIDEO_CodingH263)
                {
                    pBufferPrivate->pUalgParam = (MP4VE_GPP_SN_UALGInputParams*)pTemp;
                }
            }
            VIDENC_FREE(pBufferPrivate->pUalgParam, pMemoryListHead, pComponentPrivate->dbg);
        }
    }
    else if (nPortIndex == VIDENC_OUTPUT_PORT)
    {
        pBufferPrivate = pBufHead->pOutputPortPrivate;
        if (pBufferPrivate != NULL)
        {
            if (pBufferPrivate->pUalgParam != NULL)
            {
                pTemp = (char*)pBufferPrivate->pUalgParam;
                pTemp -= 128;
                if (eCompressionFormat == OMX_VIDEO_CodingAVC)
                {
                    pBufferPrivate->pUalgParam = (H264VE_GPP_SN_UALGOutputParams*)pTemp;
                }
                else if (eCompressionFormat == OMX_VIDEO_CodingMPEG4 ||
                         eCompressionFormat ==OMX_VIDEO_CodingH263)
                {
                    pBufferPrivate->pUalgParam = (MP4VE_GPP_SN_UALGOutputParams*)pTemp;
                }
            }
            VIDENC_FREE(pBufferPrivate->pUalgParam, pMemoryListHead, pComponentPrivate->dbg);
        }
    }
    else
    {
        OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorBadPortIndex,
                               pComponentPrivate->dbg, OMX_PRBUFFER4,
                               "Freeing buffer on invalid port index.\n");
    }

    if (pPortDef->bEnabled && pComponentPrivate->eState != OMX_StateIdle && pComponentPrivate->eState != OMX_StateInvalid)
    {
        OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorIncorrectStateOperation,
                               pComponentPrivate->dbg, OMX_PRBUFFER4,
                               "Freeing buffer in invalid state or on a disabled port.\n");
    }
    OMX_CONF_CHECK_CMD(pBufferPrivate, 1, 1);

#ifdef __PERF_INSTRUMENTATION__
    PERF_SendingBuffer(pComponentPrivate->pPERF,
                       pBufHead->pBuffer, pBufHead->nAllocLen,
                       (pBufferPrivate->bAllocByComponent == OMX_TRUE) ?
                       PERF_ModuleMemory :
                       PERF_ModuleHLMM);
#endif

    if (pBufferPrivate->bAllocByComponent == OMX_TRUE)
    {
        if (pBufHead->pBuffer != NULL)
        {
            pBufHead->pBuffer -= 128;
            pBufHead->pBuffer = (unsigned char*)pBufHead->pBuffer;
            VIDENC_FREE(pBufHead->pBuffer, pMemoryListHead, pComponentPrivate->dbg);
        }
    }

    while (1)
    {
        if (pCompPort->pBufferPrivate[nCount]->pBufferHdr == pBufHead)
        {
            break;
        }
        nCount++;
    }

    if (pBufHead != NULL)
    {
        VIDENC_FREE(pBufHead, pMemoryListHead, pComponentPrivate->dbg);
    }

    OMX_CONF_CIRCULAR_BUFFER_DELETE_NODE(pComponentPrivate,
                                         pComponentPrivate->sCircularBuffer);
    pCompPort->nBufferCnt--;
    if (pCompPort->nBufferCnt == 0)
    {

#ifndef UNDER_CE
       pthread_mutex_lock(&pComponentPrivate->videoe_mutex_app);
       pPortDef->bPopulated = OMX_FALSE;
       pthread_cond_signal(&pComponentPrivate->unpopulate_cond);
       pthread_mutex_unlock(&pComponentPrivate->videoe_mutex_app);
#else
       pPortDef->bPopulated = OMX_FALSE;
       OMX_SignalEvent(&(pComponentPrivate->InIdle_event));
#endif
    }

    if (pPortDef->bEnabled &&
        (pComponentPrivate->eState == OMX_StateIdle ||
         pComponentPrivate->eState == OMX_StateExecuting  ||
         pComponentPrivate->eState == OMX_StatePause))
    {
#ifdef  __KHRONOS_CONF__
         if(!pComponentPrivate->bPassingIdleToLoaded)
#endif
             OMX_VIDENC_EVENT_HANDLER(pComponentPrivate,
                                      OMX_EventError,
                                      OMX_ErrorPortUnpopulated,
                                      nPortIndex,
                                      NULL);
    }
OMX_CONF_CMD_BAIL:
    return eError;
}

/*----------------------------------------------------------------------------*/
/**
  *  AllocateBuffer()
  *
  *
  *
  *
  * @param
  * @param
  * @param
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  **/
/*----------------------------------------------------------------------------*/

OMX_ERRORTYPE AllocateBuffer(OMX_IN OMX_HANDLETYPE hComponent,
                             OMX_INOUT OMX_BUFFERHEADERTYPE** pBufHead,
                             OMX_IN OMX_U32 nPortIndex,
                             OMX_IN OMX_PTR pAppPrivate,
                             OMX_IN OMX_U32 nSizeBytes)
{
    OMX_COMPONENTTYPE* pHandle                  = NULL;
    VIDENC_COMPONENT_PRIVATE* pComponentPrivate = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDef      = NULL;
    VIDEOENC_PORT_TYPE* pCompPort               = NULL;
    OMX_HANDLETYPE hTunnelComponent             = NULL;
    VIDENC_BUFFER_PRIVATE* pBufferPrivate       = NULL;
    OMX_U32 nBufferCnt                          = -1;
    OMX_ERRORTYPE eError                        = OMX_ErrorNone;
    VIDENC_NODE* pMemoryListHead                = NULL;

    OMX_CONF_CHECK_CMD(hComponent, ((OMX_COMPONENTTYPE *) hComponent)->pComponentPrivate, 1);

    pComponentPrivate = (VIDENC_COMPONENT_PRIVATE*)(((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);
    OMX_DBG_CHECK_CMD(pComponentPrivate->dbg, pBufHead, 1, 1);

    pHandle = (OMX_COMPONENTTYPE*)hComponent;

    if (nPortIndex == VIDENC_INPUT_PORT)
    {
       pPortDef = pComponentPrivate->pCompPort[VIDENC_INPUT_PORT]->pPortDef;
    }
    else if (nPortIndex == VIDENC_OUTPUT_PORT)
    {
        pPortDef = pComponentPrivate->pCompPort[VIDENC_OUTPUT_PORT]->pPortDef;
    }
    else
    {
        OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorBadParameter,
                               pComponentPrivate->dbg, OMX_PRBUFFER4,
                               "Allocating buffer on invalid port index.\n");
    }
    if (!pPortDef->bEnabled)
    {
        OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorIncorrectStateOperation,
                               pComponentPrivate->dbg, OMX_PRBUFFER4,
                               "Allocating buffer on disabled port.\n");
    }

    if (pPortDef->bPopulated)
    {
        OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorBadParameter,
                               pComponentPrivate->dbg, OMX_PRBUFFER4,
                               "Allocating duplicate buffer\n");
    }
    if (nSizeBytes < pPortDef->nBufferSize)
    {
        OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorBadParameter,
            pComponentPrivate->dbg, OMX_PRBUFFER4,
            "Allocating invalid size buffer: nBufferSize: %lu nSizeBytes: %lu\n", pPortDef->nBufferSize, nSizeBytes);
    }

    pMemoryListHead = pComponentPrivate->pMemoryListHead;
    pCompPort = pComponentPrivate->pCompPort[nPortIndex];
    nBufferCnt = pComponentPrivate->pCompPort[nPortIndex]->nBufferCnt;
    hTunnelComponent = pComponentPrivate->pCompPort[VIDENC_INPUT_PORT]->hTunnelComponent;
    pBufferPrivate = pComponentPrivate->pCompPort[nPortIndex]->pBufferPrivate[nBufferCnt];

    VIDENC_MALLOC(*pBufHead,
                  sizeof(OMX_BUFFERHEADERTYPE),
                  OMX_BUFFERHEADERTYPE,
                  pMemoryListHead, pComponentPrivate->dbg);

    if (nPortIndex == VIDENC_INPUT_PORT)
    {
        (*pBufHead)->nInputPortIndex  = VIDENC_INPUT_PORT;
        (*pBufHead)->nOutputPortIndex = OMX_NOPORT;
    }
    else
    {
        (*pBufHead)->nInputPortIndex  = OMX_NOPORT;
        (*pBufHead)->nOutputPortIndex = VIDENC_OUTPUT_PORT;
    }

    VIDENC_MALLOC((*pBufHead)->pBuffer,
                  nSizeBytes + 256,
                  OMX_U8,
                  pMemoryListHead, pComponentPrivate->dbg);
    ((*pBufHead)->pBuffer) += 128;
    ((*pBufHead)->pBuffer) = (unsigned char*)((*pBufHead)->pBuffer);
    (*pBufHead)->nSize       = sizeof(OMX_BUFFERHEADERTYPE);
    (*pBufHead)->nVersion    = pPortDef->nVersion;
    (*pBufHead)->pAppPrivate = pAppPrivate;
    (*pBufHead)->nAllocLen   = nSizeBytes;
    pBufferPrivate->pBufferHdr = *pBufHead;

#ifdef __PERF_INSTRUMENTATION__
    PERF_ReceivedBuffer(pComponentPrivate->pPERF,
                        (*pBufHead)->pBuffer, nSizeBytes,
                        PERF_ModuleMemory);
#endif

    if (nPortIndex == VIDENC_INPUT_PORT)
    {
        pBufferPrivate->pBufferHdr->pInputPortPrivate = pBufferPrivate;
        if(!pComponentPrivate->nInBufferSize || (pComponentPrivate->nInBufferSize > nSizeBytes)) {
            pComponentPrivate->nInBufferSize = nSizeBytes;
        }
    }
    else
    {
        pBufferPrivate->pBufferHdr->pOutputPortPrivate = pBufferPrivate;
        if(!pComponentPrivate->nOutBufferSize || (pComponentPrivate->nOutBufferSize > nSizeBytes)) {
            pComponentPrivate->nOutBufferSize = nSizeBytes;
        }
    }
    pBufferPrivate->bAllocByComponent = OMX_TRUE;

    if (hTunnelComponent != NULL)
    {
        pBufferPrivate->eBufferOwner = VIDENC_BUFFER_WITH_TUNNELEDCOMP;
    }
    else
    {
        pBufferPrivate->eBufferOwner = VIDENC_BUFFER_WITH_CLIENT;
    }

    eError = OMX_VIDENC_Allocate_DSPResources(pComponentPrivate, nPortIndex);
    OMX_DBG_BAIL_IF_ERROR(eError, pComponentPrivate->dbg, OMX_PRDSP4,
                          "Failed to allocate DSP resources.\n");

    OMX_CONF_CIRCULAR_BUFFER_ADD_NODE(pComponentPrivate,
                                      pComponentPrivate->sCircularBuffer);

    pCompPort->nBufferCnt++;
    if(pCompPort->nBufferCnt == pPortDef->nBufferCountActual)
    {
#ifndef UNDER_CE
        pthread_mutex_lock(&pComponentPrivate->videoe_mutex_app);
        pPortDef->bPopulated = OMX_TRUE;
        pthread_cond_signal(&pComponentPrivate->populate_cond);
        pthread_mutex_unlock(&pComponentPrivate->videoe_mutex_app);
#else
        pPortDef->bPopulated = OMX_TRUE;
        OMX_SignalEvent(&(pComponentPrivate->InLoaded_event));
#endif
    }

OMX_CONF_CMD_BAIL:
    return eError;
}


/*----------------------------------------------------------------------------*/
/**
  *  VerifyTunnelConnection()
  *
  *
  *
  *
  * @param
  * @param
  * @param
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  **/
/*----------------------------------------------------------------------------*/

OMX_ERRORTYPE VerifyTunnelConnection(VIDEOENC_PORT_TYPE* pPort,
                                     OMX_HANDLETYPE hTunneledComp,
                                     OMX_PARAM_PORTDEFINITIONTYPE* pPortDef,
                                     struct OMX_TI_Debug *dbg)
{
   OMX_PARAM_PORTDEFINITIONTYPE sPortDef;
   OMX_ERRORTYPE eError = OMX_ErrorNone;

    OMX_DBG_CHECK_CMD(*dbg, pPort, hTunneledComp, pPortDef);

   sPortDef.nSize = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
   sPortDef.nVersion.s.nVersionMajor = 0x1;
   sPortDef.nVersion.s.nVersionMinor = 0x0;
   sPortDef.nPortIndex = pPort->nTunnelPort;

   eError = OMX_GetParameter(hTunneledComp,
                             OMX_IndexParamPortDefinition,
                             &sPortDef);
   if (eError != OMX_ErrorNone)
   {
       return eError;
   }

   switch (pPortDef->eDomain)
   {
       case OMX_PortDomainOther:
           if (sPortDef.format.other.eFormat!= pPortDef->format.other.eFormat)
           {
               pPort->hTunnelComponent = 0;
               pPort->nTunnelPort      = 0;
               return OMX_ErrorPortsNotCompatible;
           }
           break;
       case OMX_PortDomainAudio:
           if (sPortDef.format.audio.eEncoding != pPortDef->format.audio.eEncoding)
           {
               pPort->hTunnelComponent = 0;
               pPort->nTunnelPort      = 0;
               return OMX_ErrorPortsNotCompatible;
           }
           break;
       case OMX_PortDomainVideo:
           if (sPortDef.format.video.eCompressionFormat != pPortDef->format.video.eCompressionFormat)
           {
               pPort->hTunnelComponent = 0;
               pPort->nTunnelPort      = 0;
               return OMX_ErrorPortsNotCompatible;
           }
           break;
       case OMX_PortDomainImage:
           if (sPortDef.format.image.eCompressionFormat != pPortDef->format.image.eCompressionFormat)
           {
               pPort->hTunnelComponent = 0;
               pPort->nTunnelPort      = 0;
               return OMX_ErrorPortsNotCompatible;
           }
           break;
       default:
           pPort->hTunnelComponent = 0;
           pPort->nTunnelPort      = 0;
           return OMX_ErrorPortsNotCompatible;
   }

OMX_CONF_CMD_BAIL:
    return eError;
}

/*-------------------------------------------------------------------*/
/**
  * IsTIOMXComponent()
  * Check if the component is TI component.
  * @param hTunneledComp Component Tunnel Pipe
  * @retval OMX_TRUE   Input is a TI component.
  *             OMX_FALSE  Input is a not a TI component.
  *
  **/
/*-------------------------------------------------------------------*/

static OMX_BOOL IsTIOMXComponent(OMX_HANDLETYPE hComp, struct OMX_TI_Debug *dbg)
{

    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_STRING pTunnelcComponentName = NULL;
    OMX_VERSIONTYPE* pTunnelComponentVersion = NULL;
    OMX_VERSIONTYPE* pSpecVersion = NULL;
    OMX_UUIDTYPE* pComponentUUID = NULL;
    char *pSubstring = NULL;
    OMX_BOOL bResult = OMX_TRUE;

    pTunnelcComponentName = malloc(128);

    if (pTunnelcComponentName == NULL)
    {
        eError = OMX_ErrorInsufficientResources;
        OMX_TRACE4(*dbg, "Error in video encoder OMX_ErrorInsufficientResources %d\n",__LINE__);
        goto EXIT;
    }

    pTunnelComponentVersion = malloc(sizeof(OMX_VERSIONTYPE));
    if (pTunnelComponentVersion == NULL)
    {
        OMX_TRACE4(*dbg, "Error in video encoder OMX_ErrorInsufficientResources %d\n",__LINE__);
        eError = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    pSpecVersion = malloc(sizeof(OMX_VERSIONTYPE));
    if (pSpecVersion == NULL)
    {
        OMX_TRACE4(*dbg, "Error in video encoder OMX_ErrorInsufficientResources %d\n",__LINE__);
        eError = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    pComponentUUID = malloc(sizeof(OMX_UUIDTYPE));
    if (pComponentUUID == NULL)
    {
        OMX_TRACE4(*dbg, "Error in video encoder OMX_ErrorInsufficientResources %d\n",__LINE__);
        eError = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    eError = OMX_GetComponentVersion (hComp, pTunnelcComponentName, pTunnelComponentVersion, pSpecVersion, pComponentUUID);

    /* Check if tunneled component is a TI component */
    pSubstring = strstr(pTunnelcComponentName, "OMX.TI.");
    if (pSubstring == NULL)
    {
        bResult = OMX_FALSE;
    }

EXIT:
    free(pTunnelcComponentName);
    free(pTunnelComponentVersion);
    free(pSpecVersion);
    free(pComponentUUID);

    return bResult;
} /* End of IsTIOMXComponent */




/*----------------------------------------------------------------------------*/
/**
  *  ComponentTunnelRequest() Sets application callbacks to the component
  *
  * This method will update application callbacks
  * the application.
  *
  * @param pComp         handle for this instance of the component
  * @param pCallBacks    application callbacks
  * @param ptr
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  **/
/*----------------------------------------------------------------------------*/

OMX_ERRORTYPE ComponentTunnelRequest(OMX_IN  OMX_HANDLETYPE hComponent,
                                     OMX_IN  OMX_U32 nPort,
                                     OMX_IN  OMX_HANDLETYPE hTunneledComp,
                                     OMX_IN  OMX_U32 nTunneledPort,
                                     OMX_INOUT OMX_TUNNELSETUPTYPE* pTunnelSetup)
{
    OMX_ERRORTYPE eError       = OMX_ErrorNone;
    OMX_COMPONENTTYPE* pHandle = (OMX_COMPONENTTYPE*)hComponent;
    VIDENC_COMPONENT_PRIVATE* pComponentPrivate = (VIDENC_COMPONENT_PRIVATE*)pHandle->pComponentPrivate;
    OMX_PARAM_BUFFERSUPPLIERTYPE sBufferSupplier;
    VIDEOENC_PORT_TYPE* pPort = pComponentPrivate->pCompPort[nPort];
    if (pTunnelSetup == NULL || hTunneledComp == 0)
    {
        /* cancel previous tunnel */
        pPort->hTunnelComponent = 0;
        pPort->nTunnelPort = 0;
        pPort->eSupplierSetting = OMX_BufferSupplyUnspecified;
    }
    else
    {
        pHandle = (OMX_COMPONENTTYPE*)hComponent;
        if (!pHandle->pComponentPrivate)
        {
            OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorBadParameter);
        }
        pComponentPrivate = (VIDENC_COMPONENT_PRIVATE*)pHandle->pComponentPrivate;
        pPort = pComponentPrivate->pCompPort[nPort];

        if (pComponentPrivate->pCompPort[VIDENC_INPUT_PORT]->pPortDef->eDir != OMX_DirInput &&
            pComponentPrivate->pCompPort[VIDENC_INPUT_PORT]->pPortDef->eDir != OMX_DirOutput)
        {
            return OMX_ErrorBadParameter;
        }

        /* Check if the other component is developed by TI */
        if (IsTIOMXComponent(hTunneledComp, &pComponentPrivate->dbg) != OMX_TRUE)
        {
            eError = OMX_ErrorTunnelingUnsupported;
            goto OMX_CONF_CMD_BAIL;
        }
        pPort->hTunnelComponent = hTunneledComp;
        pPort->nTunnelPort = nTunneledPort;

        if (pPort->pPortDef->eDir == OMX_DirOutput)
        {
            /* Component is the output (source of data) */
            pTunnelSetup->eSupplier = pPort->eSupplierSetting;
        }
        else
        {
            /* Component is the input (sink of data) */
            eError = VerifyTunnelConnection(pPort,
                                            hTunneledComp,
                                            pPort->pPortDef,
                                            &pComponentPrivate->dbg);
            if(OMX_ErrorNone != eError)
            {
                OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorPortsNotCompatible,
                                       pComponentPrivate->dbg, OMX_PRCOMM3,
                                       "VerifyTunnelConnection failed.\n");
            }

            /* If specified obey output port's preferences. Otherwise choose output */
            pPort->eSupplierSetting = pTunnelSetup->eSupplier;
            if (OMX_BufferSupplyUnspecified == pPort->eSupplierSetting)
            {
                pPort->eSupplierSetting = pTunnelSetup->eSupplier = OMX_BufferSupplyOutput;
            }

            /* Tell the output port who the supplier is */
            sBufferSupplier.nSize = sizeof(sBufferSupplier);
            sBufferSupplier.nVersion.s.nVersionMajor = 0x1;
            sBufferSupplier.nVersion.s.nVersionMinor = 0x0;
            sBufferSupplier.nPortIndex = nTunneledPort;
            sBufferSupplier.eBufferSupplier = pPort->eSupplierSetting;

            eError = OMX_SetParameter(hTunneledComp,
                                      OMX_IndexParamCompBufferSupplier,
                                      &sBufferSupplier);
            eError = OMX_GetParameter(hTunneledComp,
                                      OMX_IndexParamCompBufferSupplier,
                                      &sBufferSupplier);

            if (sBufferSupplier.eBufferSupplier != pPort->eSupplierSetting)
            {
                OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorPortsNotCompatible,
                                       pComponentPrivate->dbg, OMX_PRCOMM3,
                                       "SetParameter: OMX_IndexParamCompBufferSupplier failed to change setting.\n");
            }
        }
    }
OMX_CONF_CMD_BAIL:
    return eError;
}

#ifdef __KHRONOS_CONF_1_1__

/*----------------------------------------------------------------------------*/
/**
  *  ComponentRoleEnum()
  *
  *
  * @param pComp         handle for this instance of the component
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  **/
/*----------------------------------------------------------------------------*/
static OMX_ERRORTYPE ComponentRoleEnum(OMX_IN OMX_HANDLETYPE hComponent,
                                       OMX_OUT OMX_U8 *cRole,
                                       OMX_IN OMX_U32 nIndex)
{
    VIDENC_COMPONENT_PRIVATE *pComponentPrivate;
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    if (hComponent==NULL)
    {
        goto OMX_CONF_CMD_BAIL;
        eError= OMX_ErrorBadParameter;
        }

    pComponentPrivate = (VIDENC_COMPONENT_PRIVATE*)(((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);

    if(nIndex == 0)
    {
      strncpy((char*)cRole, (char *)pComponentPrivate->componentRole.cRole, sizeof(OMX_U8) * OMX_MAX_STRINGNAME_SIZE - 1);
    }
    else
    {
      eError = OMX_ErrorNoMore;
    }

OMX_CONF_CMD_BAIL:
    return eError;
};
#endif
