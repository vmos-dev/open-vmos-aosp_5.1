
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
*             Texas Instruments OMAP (TM) Platform Software
*  (c) Copyright Texas Instruments, Incorporated.  All Rights Reserved.
*
*  Use of this software is controlled by the terms and conditions found
*  in the license agreement under which this software has been supplied.
* =========================================================================== */
/**
* @file OMX_AmrEncoder.c
*
* This file implements OpenMAX (TM) 1.0 Specific APIs and its functionality
* that is fully compliant with the Khronos OpenMAX (TM) 1.0 Specification
*
* @path  $(CSLPATH)\OMAPSW_MPU\linux\audio\src\openmax_il\nbamr_enc\src
*
* @rev  1.0
*/
/* ----------------------------------------------------------------------------
*!
*! Revision History
*! ===================================
*! 21-sept-2006 bk: updated review findings for alpha release
*! 24-Aug-2006 bk: Khronos OpenMAX (TM) 1.0 Conformance tests some more
*! 18-July-2006 bk: Khronos OpenMAX (TM) 1.0 Conformance tests validated for few cases
*! 21-Jun-2006 bk: Khronos OpenMAX (TM) 1.0 migration done
*! 22-May-2006 bk: DASF recording quality improved
*! 19-Apr-2006 bk: DASF recording speed issue resloved
*! 23-Feb-2006 bk: DASF functionality added
*! 18-Jan-2006 bk: Repated recording issue fixed and LCML changes taken care
*! 14-Dec-2005 bk: Initial Version
*! 16-Nov-2005 bk: Initial Version
*! 23-Sept-2005 bk: Initial Version
*! 10-Sept-2005 bk: Initial Version
*! 10-Sept-2005 bk:
*! This is newest file
* =========================================================================== */
/* ------compilation control switches -------------------------*/
/****************************************************************
*  INCLUDE FILES
****************************************************************/
/* ----- system and platform files ----------------------------*/
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
#endif

#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <dbapi.h>

#ifdef DSP_RENDERING_ON
    #include <AudioManagerAPI.h>
#endif

#ifdef RESOURCE_MANAGER_ENABLED
    #include <ResourceManagerProxyAPI.h>
#endif

#ifdef __PERF_INSTRUMENTATION__
    #include "perf.h"
#endif

/*-------program files ----------------------------------------*/
#include <OMX_Component.h>

#include "OMX_AmrEnc_Utils.h"

/****************************************************************
*  EXTERNAL REFERENCES NOTE : only use if not found in header file
****************************************************************/
/*--------data declarations -----------------------------------*/

/*--------function prototypes ---------------------------------*/

/****************************************************************
*  PUBLIC DECLARATIONS Defined here, used elsewhere
****************************************************************/
/*--------data declarations -----------------------------------*/

/*--------function prototypes ---------------------------------*/

/****************************************************************
*  PRIVATE DECLARATIONS Defined here, used only here
****************************************************************/
/*--------data declarations -----------------------------------*/

/*--------function prototypes ---------------------------------*/
#define AMRNB_DEC_ROLE "audio_encoder.amrnb"

static OMX_ERRORTYPE SetCallbacks (OMX_HANDLETYPE hComp,
        OMX_CALLBACKTYPE* pCallBacks, OMX_PTR pAppData);
static OMX_ERRORTYPE GetComponentVersion (OMX_HANDLETYPE hComp,
                                          OMX_STRING pComponentName,
                                          OMX_VERSIONTYPE* pComponentVersion,
                                          OMX_VERSIONTYPE* pSpecVersion,
                                          OMX_UUIDTYPE* pComponentUUID);
static OMX_ERRORTYPE SendCommand (OMX_HANDLETYPE hComp, OMX_COMMANDTYPE nCommand,
       OMX_U32 nParam,OMX_PTR pCmdData);
static OMX_ERRORTYPE GetParameter(OMX_HANDLETYPE hComp, OMX_INDEXTYPE nParamIndex,
                                  OMX_PTR ComponentParamStruct);
static OMX_ERRORTYPE SetParameter (OMX_HANDLETYPE hComp,
                                   OMX_INDEXTYPE nParamIndex,
                                   OMX_PTR ComponentParamStruct);
static OMX_ERRORTYPE GetConfig (OMX_HANDLETYPE hComp,
                                OMX_INDEXTYPE nConfigIndex,
                                OMX_PTR pComponentConfigStructure);
static OMX_ERRORTYPE SetConfig (OMX_HANDLETYPE hComp,
                                OMX_INDEXTYPE nConfigIndex,
                                OMX_PTR pComponentConfigStructure);

static OMX_ERRORTYPE EmptyThisBuffer (OMX_HANDLETYPE hComp, OMX_BUFFERHEADERTYPE* pBuffer);

static OMX_ERRORTYPE FillThisBuffer (OMX_HANDLETYPE hComp, OMX_BUFFERHEADERTYPE* pBuffer);
static OMX_ERRORTYPE GetState (OMX_HANDLETYPE hComp, OMX_STATETYPE* pState);
static OMX_ERRORTYPE ComponentTunnelRequest (OMX_HANDLETYPE hComp,
                                             OMX_U32 nPort, OMX_HANDLETYPE hTunneledComp,
                                             OMX_U32 nTunneledPort,
                                             OMX_TUNNELSETUPTYPE* pTunnelSetup);
static OMX_ERRORTYPE ComponentDeInit(OMX_HANDLETYPE pHandle);
static OMX_ERRORTYPE AllocateBuffer (OMX_IN OMX_HANDLETYPE hComponent,
                   OMX_INOUT OMX_BUFFERHEADERTYPE** pBuffer,
                   OMX_IN OMX_U32 nPortIndex,
                   OMX_IN OMX_PTR pAppPrivate,
                   OMX_IN OMX_U32 nSizeBytes);

static OMX_ERRORTYPE FreeBuffer(
            OMX_IN  OMX_HANDLETYPE hComponent,
            OMX_IN  OMX_U32 nPortIndex,
            OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer);

static OMX_ERRORTYPE UseBuffer (
            OMX_IN OMX_HANDLETYPE hComponent,
            OMX_INOUT OMX_BUFFERHEADERTYPE** ppBufferHdr,
            OMX_IN OMX_U32 nPortIndex,
            OMX_IN OMX_PTR pAppPrivate,
            OMX_IN OMX_U32 nSizeBytes,
            OMX_IN OMX_U8* pBuffer);

static OMX_ERRORTYPE GetExtensionIndex(
            OMX_IN  OMX_HANDLETYPE hComponent,
            OMX_IN  OMX_STRING cParameterName,
            OMX_OUT OMX_INDEXTYPE* pIndexType);

static OMX_ERRORTYPE ComponentRoleEnum(
        OMX_IN OMX_HANDLETYPE hComponent,
        OMX_OUT OMX_U8 *cRole,
        OMX_IN OMX_U32 nIndex);                 

#ifdef DSP_RENDERING_ON
AM_COMMANDDATATYPE cmd_data;
#endif

/* interface with audio manager*/
#define FIFO1 "/dev/fifo.1"
#define FIFO2 "/dev/fifo.2"
#define PERMS 0666



/*-------------------------------------------------------------------*/
/**
  * OMX_ComponentInit() Set the all the function pointers of component
  *
  * This method will update the component function pointer to the handle
  *
  * @param hComp         handle for this instance of the component
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_ErrorInsufficientResources If the newmalloc fails
  **/
/*-------------------------------------------------------------------*/
OMX_ERRORTYPE OMX_ComponentInit (OMX_HANDLETYPE hComp)
{
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef_ip, *pPortDef_op;
    AMRENC_COMPONENT_PRIVATE *pComponentPrivate;
    OMX_AUDIO_PARAM_PCMMODETYPE *amr_ip;
    OMX_AUDIO_PARAM_AMRTYPE  *amr_op;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE*) hComp;
    NBAMRENC_PORT_TYPE *pCompPort = NULL;
    OMX_AUDIO_PARAM_PORTFORMATTYPE *pPortFormat = NULL;
    int i = 0;

    OMXDBG_PRINT(stderr, PRINT, 1, 0, "Entering OMX_ComponentInit\n");

    /*Set the all component function pointer to the handle */
    pHandle->SetCallbacks = SetCallbacks;
    pHandle->GetComponentVersion = GetComponentVersion;
    pHandle->SendCommand = SendCommand;
    pHandle->GetParameter = GetParameter;
    pHandle->SetParameter = SetParameter;
    pHandle->GetConfig = GetConfig;
    pHandle->SetConfig = SetConfig;
    pHandle->GetState = GetState;
    pHandle->EmptyThisBuffer = EmptyThisBuffer;
    pHandle->FillThisBuffer = FillThisBuffer;
    pHandle->ComponentTunnelRequest = ComponentTunnelRequest;
    pHandle->ComponentDeInit = ComponentDeInit;
    pHandle->AllocateBuffer = AllocateBuffer;
    pHandle->FreeBuffer = FreeBuffer;
    pHandle->UseBuffer = UseBuffer;
    pHandle->GetExtensionIndex = GetExtensionIndex;
    pHandle->ComponentRoleEnum = ComponentRoleEnum;


    /*Allocate the memory for Component private data area */
    OMX_MALLOC_GENERIC(pHandle->pComponentPrivate, AMRENC_COMPONENT_PRIVATE);


    ((AMRENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate)->pHandle = pHandle;
    pComponentPrivate = pHandle->pComponentPrivate;

    OMX_DBG_INIT(pComponentPrivate->dbg, "OMX_DBG_NBAMRENC");

#ifdef ANDROID /* leave this now, we may need them later. */
    pComponentPrivate->iPVCapabilityFlags.iIsOMXComponentMultiThreaded = OMX_TRUE;
    pComponentPrivate->iPVCapabilityFlags.iOMXComponentNeedsNALStartCode = OMX_FALSE;
    pComponentPrivate->iPVCapabilityFlags.iOMXComponentSupportsExternalOutputBufferAlloc = OMX_FALSE;
    pComponentPrivate->iPVCapabilityFlags.iOMXComponentSupportsExternalInputBufferAlloc = OMX_FALSE;
    pComponentPrivate->iPVCapabilityFlags.iOMXComponentSupportsMovableInputBuffers = OMX_FALSE;
    pComponentPrivate->iPVCapabilityFlags.iOMXComponentSupportsPartialFrames = OMX_FALSE;
    pComponentPrivate->iPVCapabilityFlags.iOMXComponentCanHandleIncompleteFrames = OMX_FALSE;
#endif

#ifdef __PERF_INSTRUMENTATION__
    pComponentPrivate->pPERF = PERF_Create(PERF_FOURCC('N','B','E','_'),
                                           PERF_ModuleLLMM | PERF_ModuleAudioDecode);
#endif

    OMX_MALLOC_GENERIC(pCompPort, NBAMRENC_PORT_TYPE);
    pComponentPrivate->pCompPort[NBAMRENC_INPUT_PORT] = pCompPort;

    OMX_MALLOC_GENERIC(pCompPort, NBAMRENC_PORT_TYPE);
    pComponentPrivate->pCompPort[NBAMRENC_OUTPUT_PORT] = pCompPort;

    OMX_MALLOC_GENERIC(pComponentPrivate->sPortParam, OMX_PORT_PARAM_TYPE);
    OMX_NBCONF_INIT_STRUCT(pComponentPrivate->sPortParam, OMX_PORT_PARAM_TYPE);

    /* Initialize sPortParam data structures to default values */
    pComponentPrivate->sPortParam->nPorts = 0x2;
    pComponentPrivate->sPortParam->nStartPortNumber = 0x0;

    /* Malloc and Set pPriorityMgmt defaults */
    OMX_MALLOC_GENERIC(pComponentPrivate->sPriorityMgmt, OMX_PRIORITYMGMTTYPE);
    OMX_NBCONF_INIT_STRUCT(pComponentPrivate->sPriorityMgmt, OMX_PRIORITYMGMTTYPE);

    /* Initialize sPriorityMgmt data structures to default values */
    pComponentPrivate->sPriorityMgmt->nGroupPriority = -1;
    pComponentPrivate->sPriorityMgmt->nGroupID = -1;

    OMX_MALLOC_GENERIC(amr_ip, OMX_AUDIO_PARAM_PCMMODETYPE);
    OMX_NBCONF_INIT_STRUCT(amr_ip, OMX_AUDIO_PARAM_PCMMODETYPE);
    pComponentPrivate->pcmParams = amr_ip;


    OMX_MALLOC_GENERIC(amr_op, OMX_AUDIO_PARAM_AMRTYPE);
    OMX_NBCONF_INIT_STRUCT(amr_op, OMX_AUDIO_PARAM_AMRTYPE);
    pComponentPrivate->amrParams = amr_op;

    /* newmalloc and initialize number of input buffers */
    OMX_MALLOC_GENERIC(pComponentPrivate->pInputBufferList, NBAMRENC_BUFFERLIST);
    pComponentPrivate->pInputBufferList->numBuffers = 0;

    /* newmalloc and initialize number of output buffers */
    OMX_MALLOC_GENERIC(pComponentPrivate->pOutputBufferList, NBAMRENC_BUFFERLIST);
    pComponentPrivate->pOutputBufferList->numBuffers = 0;

    for (i=0; i < NBAMRENC_MAX_NUM_OF_BUFS; i++) {
        pComponentPrivate->pOutputBufferList->pBufHdr[i] = NULL;
        pComponentPrivate->pInputBufferList->pBufHdr[i] = NULL;
    }

    /* Set input port defaults */
    OMX_MALLOC_GENERIC(pPortDef_ip, OMX_PARAM_PORTDEFINITIONTYPE);
    OMX_NBCONF_INIT_STRUCT(pPortDef_ip, OMX_PARAM_PORTDEFINITIONTYPE);
    pComponentPrivate->pPortDef[NBAMRENC_INPUT_PORT] = pPortDef_ip;

    
    /* Let's set defaults for IF2 conditions on Android and see what happens */
    pPortDef_ip->nPortIndex                         = NBAMRENC_INPUT_PORT;
    pPortDef_ip->eDir                               = OMX_DirInput;
    pPortDef_ip->nBufferCountActual                 = 5; //NBAMRENC_NUM_INPUT_BUFFERS;
    pPortDef_ip->nBufferCountMin                    = 5; //NBAMRENC_NUM_INPUT_BUFFERS;
    pPortDef_ip->nBufferSize                        = NBAMRENC_INPUT_FRAME_SIZE;
    pPortDef_ip->nBufferAlignment                   = DSP_CACHE_ALIGNMENT;
    pPortDef_ip->bEnabled                           = OMX_TRUE;
    pPortDef_ip->bPopulated                         = OMX_FALSE;
    pPortDef_ip->eDomain                            = OMX_PortDomainAudio;
    pPortDef_ip->format.audio.eEncoding             = OMX_AUDIO_CodingPCM;
    pPortDef_ip->format.audio.cMIMEType             = NULL;
    pPortDef_ip->format.audio.pNativeRender         = NULL;
    pPortDef_ip->format.audio.bFlagErrorConcealment = OMX_FALSE;

    /* Set output port defaults */
    OMX_MALLOC_GENERIC(pPortDef_op, OMX_PARAM_PORTDEFINITIONTYPE);
    OMX_NBCONF_INIT_STRUCT(pPortDef_op, OMX_PARAM_PORTDEFINITIONTYPE);
    pComponentPrivate->pPortDef[NBAMRENC_OUTPUT_PORT] = pPortDef_op;

    pPortDef_op->nPortIndex                         = NBAMRENC_OUTPUT_PORT;
    pPortDef_op->eDir                               = OMX_DirOutput;
    pPortDef_op->nBufferCountMin                    = 9; //NBAMRENC_NUM_OUTPUT_BUFFERS;
    pPortDef_op->nBufferCountActual                 = 9; //NBAMRENC_NUM_OUTPUT_BUFFERS;
    pPortDef_op->nBufferSize                        = 320; //NBAMRENC_OUTPUT_FRAME_SIZE;
    pPortDef_ip->nBufferAlignment                   = DSP_CACHE_ALIGNMENT;
    pPortDef_op->bEnabled                           = OMX_TRUE;
    pPortDef_op->bPopulated                         = OMX_FALSE;
    pPortDef_op->eDomain                            = OMX_PortDomainAudio;
    pPortDef_op->format.audio.eEncoding             = OMX_AUDIO_CodingAMR;
    pPortDef_op->format.audio.cMIMEType             = NULL;
    pPortDef_op->format.audio.pNativeRender         = NULL;
    pPortDef_op->format.audio.bFlagErrorConcealment = OMX_FALSE;

    OMX_MALLOC_GENERIC(pComponentPrivate->pCompPort[NBAMRENC_INPUT_PORT]->pPortFormat, OMX_AUDIO_PARAM_PORTFORMATTYPE);
    OMX_NBCONF_INIT_STRUCT(pComponentPrivate->pCompPort[NBAMRENC_INPUT_PORT]->pPortFormat, OMX_AUDIO_PARAM_PORTFORMATTYPE);
    /* Set input port format defaults */
    pPortFormat = pComponentPrivate->pCompPort[NBAMRENC_INPUT_PORT]->pPortFormat;
    OMX_NBCONF_INIT_STRUCT(pPortFormat, OMX_AUDIO_PARAM_PORTFORMATTYPE);
    pPortFormat->nPortIndex         = NBAMRENC_INPUT_PORT;
    pPortFormat->nIndex             = OMX_IndexParamAudioAmr;
    pPortFormat->eEncoding          = OMX_AUDIO_CodingPCM;  /*Data Expected on Input Port*/

    pComponentPrivate->bPreempted = OMX_FALSE;
  
    amr_ip->nPortIndex = NBAMRENC_INPUT_PORT; 
/*ck*/    amr_ip->nChannels = 1;  //was 2 
    amr_ip->eNumData= OMX_NumericalDataSigned; 
    amr_ip->nBitPerSample = 16;  
/*ck*/    amr_ip->nSamplingRate = 8000; //44100;           
    amr_ip->ePCMMode = OMX_AUDIO_PCMModeLinear; 
    amr_ip->bInterleaved = OMX_TRUE; /*For Encoders Only*/
    strcpy((char *)pComponentPrivate->componentRole.cRole, "audio_encoder.amrnb");  

    amr_op->nPortIndex = NBAMRENC_OUTPUT_PORT;
    amr_op->nChannels = 1; 
    amr_op->eAMRBandMode = OMX_AUDIO_AMRBandModeNB0;
    amr_op->eAMRDTXMode= OMX_AUDIO_AMRDTXModeOff;
    amr_op->eAMRFrameFormat =OMX_AUDIO_AMRFrameFormatConformance;


    OMX_MALLOC_GENERIC(pComponentPrivate->pCompPort[NBAMRENC_OUTPUT_PORT]->pPortFormat, OMX_AUDIO_PARAM_PORTFORMATTYPE);
    OMX_NBCONF_INIT_STRUCT(pComponentPrivate->pCompPort[NBAMRENC_OUTPUT_PORT]->pPortFormat, OMX_AUDIO_PARAM_PORTFORMATTYPE);
    /* Set output port format defaults */
    pPortFormat = pComponentPrivate->pCompPort[NBAMRENC_OUTPUT_PORT]->pPortFormat;
    OMX_NBCONF_INIT_STRUCT(pPortFormat, OMX_AUDIO_PARAM_PORTFORMATTYPE);
    pPortFormat->nPortIndex         = NBAMRENC_OUTPUT_PORT;
    pPortFormat->nIndex             = OMX_IndexParamAudioAmr;
    pPortFormat->eEncoding          = OMX_AUDIO_CodingAMR;

    OMX_PRDSP2(pComponentPrivate->dbg, "%d :: Setting dasf & mime & efr & acdn & amr and MultiFrame modes to 0\n",__LINE__);
    pComponentPrivate->dasfMode = 0;
    pComponentPrivate->teeMode = 0;
    pComponentPrivate->frameMode = 0;
    pComponentPrivate->acdnMode = 0;
    pComponentPrivate->efrMode  = 0;
    pComponentPrivate->amrMode  = 0;
    pComponentPrivate->nMultiFrameMode = 0;
    pComponentPrivate->bCompThreadStarted = 0;
    pComponentPrivate->pParams = NULL;
    pComponentPrivate->pAlgParam = NULL;
    pComponentPrivate->bInitParamsInitialized = 0;
    pComponentPrivate->amrMimeBytes[0]  = NBAMRENC_FRAME_SIZE_13;
    pComponentPrivate->amrMimeBytes[1]  = NBAMRENC_FRAME_SIZE_14;
    pComponentPrivate->amrMimeBytes[2]  = NBAMRENC_FRAME_SIZE_16;
    pComponentPrivate->amrMimeBytes[3]  = NBAMRENC_FRAME_SIZE_18;
    pComponentPrivate->amrMimeBytes[4]  = NBAMRENC_FRAME_SIZE_20;
    pComponentPrivate->amrMimeBytes[5]  = NBAMRENC_FRAME_SIZE_21;
    pComponentPrivate->amrMimeBytes[6]  = NBAMRENC_FRAME_SIZE_27;
    pComponentPrivate->amrMimeBytes[7]  = NBAMRENC_FRAME_SIZE_32;
    pComponentPrivate->amrMimeBytes[8]  = NBAMRENC_FRAME_SIZE_6;
    pComponentPrivate->amrMimeBytes[9]  = NBAMRENC_FRAME_SIZE_0;
    pComponentPrivate->amrMimeBytes[10] = NBAMRENC_FRAME_SIZE_0;
    pComponentPrivate->amrMimeBytes[11] = NBAMRENC_FRAME_SIZE_0;
    pComponentPrivate->amrMimeBytes[12] = NBAMRENC_FRAME_SIZE_0;
    pComponentPrivate->amrMimeBytes[13] = NBAMRENC_FRAME_SIZE_0;
    pComponentPrivate->amrMimeBytes[14] = NBAMRENC_FRAME_SIZE_0;
    pComponentPrivate->amrMimeBytes[15] = NBAMRENC_FRAME_SIZE_1;

    pComponentPrivate->amrIf2Bytes[0]  = NBAMRENC_FRAME_SIZE_13;
    pComponentPrivate->amrIf2Bytes[1]  = NBAMRENC_FRAME_SIZE_14;
    pComponentPrivate->amrIf2Bytes[2]  = NBAMRENC_FRAME_SIZE_16;
    pComponentPrivate->amrIf2Bytes[3]  = NBAMRENC_FRAME_SIZE_18;
    pComponentPrivate->amrIf2Bytes[4]  = NBAMRENC_FRAME_SIZE_19;
    pComponentPrivate->amrIf2Bytes[5]  = NBAMRENC_FRAME_SIZE_21;
    pComponentPrivate->amrIf2Bytes[6]  = NBAMRENC_FRAME_SIZE_26;
    pComponentPrivate->amrIf2Bytes[7]  = NBAMRENC_FRAME_SIZE_31;
    pComponentPrivate->amrIf2Bytes[8]  = NBAMRENC_FRAME_SIZE_6;
    pComponentPrivate->amrIf2Bytes[9]  = NBAMRENC_FRAME_SIZE_0;
    pComponentPrivate->amrIf2Bytes[10] = NBAMRENC_FRAME_SIZE_0;
    pComponentPrivate->amrIf2Bytes[11] = NBAMRENC_FRAME_SIZE_0;
    pComponentPrivate->amrIf2Bytes[12] = NBAMRENC_FRAME_SIZE_0;
    pComponentPrivate->amrIf2Bytes[13] = NBAMRENC_FRAME_SIZE_0;
    pComponentPrivate->amrIf2Bytes[14] = NBAMRENC_FRAME_SIZE_0;
    pComponentPrivate->amrIf2Bytes[15] = NBAMRENC_FRAME_SIZE_1;
    
    pComponentPrivate->pMarkBuf = NULL;
    pComponentPrivate->pMarkData = NULL;
    pComponentPrivate->nEmptyBufferDoneCount = 0;
    pComponentPrivate->nEmptyThisBufferCount = 0;
    pComponentPrivate->nFillBufferDoneCount = 0;
    pComponentPrivate->nFillThisBufferCount = 0;
    pComponentPrivate->strmAttr = NULL;
    pComponentPrivate->bDisableCommandParam = 0;
    pComponentPrivate->bEnableCommandParam = 0;
    pComponentPrivate->iHoldLen = 0;
    pComponentPrivate->iHoldBuffer = NULL;
    pComponentPrivate->pHoldBuffer = NULL;
    pComponentPrivate->nHoldLength = 0;
    pComponentPrivate->bFirstInputBufReceived = OMX_FALSE;
    pComponentPrivate->TimeStamp = 0;

    pComponentPrivate->nUnhandledFillThisBuffers=0;
    pComponentPrivate->nUnhandledEmptyThisBuffers = 0;
    
    pComponentPrivate->bFlushOutputPortCommandPending = OMX_FALSE;
    pComponentPrivate->bFlushInputPortCommandPending = OMX_FALSE;
    
    for (i=0; i < NBAMRENC_MAX_NUM_OF_BUFS; i++) {
        pComponentPrivate->pInputBufHdrPending[i] = NULL;
        pComponentPrivate->pOutputBufHdrPending[i] = NULL;
        pComponentPrivate->arrTickCount[i] = 0;
        pComponentPrivate->arrBufIndex[i] = 0;
    }
    pComponentPrivate->nNumInputBufPending = 0;
    pComponentPrivate->nNumOutputBufPending = 0;
    pComponentPrivate->bDisableCommandPending = 0;
    pComponentPrivate->bEnableCommandPending = 0;
    pComponentPrivate->bNoIdleOnStop= OMX_FALSE;

    pComponentPrivate->bDspStoppedWhileExecuting    = OMX_FALSE;

    pComponentPrivate->nOutStandingFillDones = 0;
    pComponentPrivate->nRuntimeInputBuffers=0;
    pComponentPrivate->nRuntimeOutputBuffers=0;
    pComponentPrivate->nNumOfFramesSent=0;
    pComponentPrivate->ptrLibLCML = NULL;
    pComponentPrivate->IpBufindex = 0;
    pComponentPrivate->OpBufindex = 0;
    pComponentPrivate->nOutStandingEmptyDones = 0;

    pComponentPrivate->bLoadedWaitingFreeBuffers    = OMX_FALSE;
    pComponentPrivate->ProcessingInputBuf = 0;
    pComponentPrivate->ProcessingOutputBuf = 0;

    OMX_MALLOC_SIZE(pComponentPrivate->sDeviceString, 100*sizeof(OMX_STRING),OMX_STRING);

    /* Initialize device string to the default value */
    strcpy((char*)pComponentPrivate->sDeviceString,":srcul/codec\0");
    
#ifndef UNDER_CE
    pthread_mutex_init(&pComponentPrivate->AlloBuf_mutex, NULL);
    pthread_cond_init (&pComponentPrivate->AlloBuf_threshold, NULL);
    pComponentPrivate->AlloBuf_waitingsignal = 0;

    pthread_mutex_init(&pComponentPrivate->codecStop_mutex, NULL);
    pthread_cond_init (&pComponentPrivate->codecStop_threshold, NULL);
    pComponentPrivate->codecStop_waitingsignal = 0;

    pthread_mutex_init(&pComponentPrivate->InLoaded_mutex, NULL);
    pthread_cond_init (&pComponentPrivate->InLoaded_threshold, NULL);
    pComponentPrivate->InLoaded_readytoidle = 0;

    pthread_mutex_init(&pComponentPrivate->InIdle_mutex, NULL);
    pthread_cond_init (&pComponentPrivate->InIdle_threshold, NULL);
    pComponentPrivate->InIdle_goingtoloaded = 0;

    pthread_mutex_init(&pComponentPrivate->ToLoaded_mutex, NULL);
#else
    OMX_CreateEvent(&(pComponentPrivate->AlloBuf_event));
    pComponentPrivate->AlloBuf_waitingsignal = 0;

    OMX_CreateEvent(&(pComponentPrivate->InLoaded_event));
    pComponentPrivate->InLoaded_readytoidle = 0;

    OMX_CreateEvent(&(pComponentPrivate->InIdle_event));
    pComponentPrivate->InIdle_goingtoloaded = 0;
#endif
    pComponentPrivate->bIsInvalidState = OMX_FALSE;

#ifdef RESOURCE_MANAGER_ENABLED
    eError = RMProxy_NewInitalize();

    OMX_PRINT1(pComponentPrivate->dbg, "%d :: OMX_ComponentInit\n", __LINE__);
    if (eError != OMX_ErrorNone) {
        OMX_ERROR4(pComponentPrivate->dbg, "%d :: Error returned from loading ResourceManagerProxy thread\n",__LINE__);
        goto EXIT;
    }
#endif

    eError = NBAMRENC_StartComponentThread(pHandle);
    OMX_PRINT1(pComponentPrivate->dbg, "%d :: OMX_ComponentInit\n", __LINE__);
    if (eError != OMX_ErrorNone) {
      OMX_ERROR4(pComponentPrivate->dbg, "%d :: Error returned from the Component\n",__LINE__);
      goto EXIT;
    }


#ifdef __PERF_INSTRUMENTATION__
    PERF_ThreadCreated(pComponentPrivate->pPERF, pComponentPrivate->ComponentThread,
                       PERF_FOURCC('N','B','E','T'));
#endif

#ifndef UNDER_CE
#ifdef DSP_RENDERING_ON
    /* start Audio Manager to get streamId */
    if((pComponentPrivate->fdwrite=open(FIFO1,O_WRONLY))<0) {
        OMX_ERROR4(pComponentPrivate->dbg, "%d :: [NBAMRE Encoder Component] - failure to open WRITE pipe\n",__LINE__);
    }

    if((pComponentPrivate->fdread=open(FIFO2,O_RDONLY))<0) {
        OMX_ERROR4(pComponentPrivate->dbg, "%d :: [NBAMRE Encoder Component] - failure to open READ pipe\n",__LINE__);
    }    
#endif
#endif
   if(pthread_mutex_init(&pComponentPrivate->mutexStateChangeRequest, NULL)) {
       return OMX_ErrorUndefined;
   }
	
   if(pthread_cond_init (&pComponentPrivate->StateChangeCondition, NULL)) {
       return OMX_ErrorUndefined;
   }

   pComponentPrivate->nPendingStateChangeRequests = 0;

EXIT:
    OMX_PRINT1(pComponentPrivate->dbg, "%d :: Exiting OMX_ComponentInit\n", __LINE__);
    OMX_PRINT1(pComponentPrivate->dbg, "%d :: Returning = 0x%x\n",__LINE__,eError);
    return eError;
}

/*-------------------------------------------------------------------*/
/**
  *  SetCallbacks() Sets application callbacks to the component
  *
  * This method will update application callbacks
  * to the component. So that component can make use of those call back
  * while sending buffers to the application. And also it will copy the
  * application private data to component memory
  *
  * @param pComponent    handle for this instance of the component
  * @param pCallBacks    application callbacks
  * @param pAppData      Application private data
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  **/
/*-------------------------------------------------------------------*/

static OMX_ERRORTYPE SetCallbacks (OMX_HANDLETYPE pComponent,
                                   OMX_CALLBACKTYPE* pCallBacks,
                                   OMX_PTR pAppData)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE*)pComponent;

    AMRENC_COMPONENT_PRIVATE *pComponentPrivate =
                    (AMRENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    OMX_PRINT1(pComponentPrivate->dbg, "%d :: Entering SetCallbacks\n", __LINE__);
    if (pCallBacks == NULL) {
        eError = OMX_ErrorBadParameter;
        OMX_ERROR4(pComponentPrivate->dbg, "%d :: Received the empty callbacks from the application\n",__LINE__);
        goto EXIT;
    }

    /*Copy the callbacks of the application to the component private*/
    memcpy (&(pComponentPrivate->cbInfo), pCallBacks, sizeof(OMX_CALLBACKTYPE));

    /*copy the application private data to component memory */
    pHandle->pApplicationPrivate = pAppData;

    pComponentPrivate->curState = OMX_StateLoaded;
EXIT:
    OMX_PRINT1(pComponentPrivate->dbg, "%d :: Exiting SetCallbacks\n", __LINE__);
    OMX_PRINT1(pComponentPrivate->dbg, "%d :: Returning = 0x%x\n",__LINE__,eError);
    return eError;
}

/*-------------------------------------------------------------------*/
/**
  *  GetComponentVersion() This will return the component version
  *
  * This method will retrun the component version
  *
  * @param hComp               handle for this instance of the component
  * @param pCompnentName       Name of the component
  * @param pCompnentVersion    handle for this instance of the component
  * @param pSpecVersion        application callbacks
  * @param pCompnentUUID
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  **/
/*-------------------------------------------------------------------*/

static OMX_ERRORTYPE GetComponentVersion (OMX_HANDLETYPE hComp,
                                          OMX_STRING pComponentName,
                                          OMX_VERSIONTYPE* pComponentVersion,
                                          OMX_VERSIONTYPE* pSpecVersion,
                                          OMX_UUIDTYPE* pComponentUUID)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE*) hComp;
    AMRENC_COMPONENT_PRIVATE *pComponentPrivate = (AMRENC_COMPONENT_PRIVATE *) pHandle->pComponentPrivate;
    OMX_PRINT1(pComponentPrivate->dbg, "%d :: Entering GetComponentVersion\n", __LINE__);

#ifdef _ERROR_PROPAGATION__
    if (pComponentPrivate->curState == OMX_StateInvalid){
        eError = OMX_ErrorInvalidState;
        goto EXIT;
    }
#endif

    /* Copy component version structure */
    if(pComponentVersion != NULL && pComponentName != NULL) {
        strcpy(pComponentName, pComponentPrivate->cComponentName);
        memcpy(pComponentVersion, &(pComponentPrivate->ComponentVersion.s), sizeof(pComponentPrivate->ComponentVersion.s));
    }
    else {
        OMX_ERROR4(pComponentPrivate->dbg, "%d :: OMX_ErrorBadParameter from GetComponentVersion",__LINE__);
        eError = OMX_ErrorBadParameter;
    }

EXIT:
    OMX_PRINT1(pComponentPrivate->dbg, "%d :: Exiting GetComponentVersion\n", __LINE__);
    OMX_PRINT1(pComponentPrivate->dbg, "%d :: Returning = 0x%x\n",__LINE__,eError);
    return eError;
}
/*-------------------------------------------------------------------*/
/**
  *  SendCommand() used to send the commands to the component
  *
  * This method will be used by the application.
  *
  * @param phandle         handle for this instance of the component
  * @param Cmd             Command to be sent to the component
  * @param nParam          indicates commmad is sent using this method
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  **/
/*-------------------------------------------------------------------*/

static OMX_ERRORTYPE SendCommand (OMX_HANDLETYPE phandle,
                                  OMX_COMMANDTYPE Cmd,
                                  OMX_U32 nParam,
                                  OMX_PTR pCmdData)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)phandle;
    AMRENC_COMPONENT_PRIVATE *pCompPrivate =
             (AMRENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    int nRet = 0;
   
#ifdef _ERROR_PROPAGATION__
    if (pCompPrivate->curState == OMX_StateInvalid){
        eError = OMX_ErrorInvalidState;
        goto EXIT;
    }
#else
    OMX_PRINT1(pCompPrivate->dbg, "%d :: Entering SendCommand()\n", __LINE__);
    if(pCompPrivate->curState == OMX_StateInvalid) {
        eError = OMX_ErrorInvalidState;
        OMX_ERROR4(pCompPrivate->dbg, "%d :: Error OMX_ErrorInvalidState Sent to App\n",__LINE__);
        pCompPrivate->cbInfo.EventHandler (pHandle, 
                                            pHandle->pApplicationPrivate,
                                            OMX_EventError, 
                                            OMX_ErrorInvalidState,
                                            OMX_TI_ErrorMinor,
                                            "Invalid State");
        goto EXIT;
    }
#endif
#ifdef __PERF_INSTRUMENTATION__
    PERF_SendingCommand(pCompPrivate->pPERF,
                        Cmd,
                        (Cmd == OMX_CommandMarkBuffer) ? ((OMX_U32) pCmdData) : nParam,
                        PERF_ModuleComponent);
#endif
    switch(Cmd) {
        case OMX_CommandStateSet:
            OMX_PRINT1(pCompPrivate->dbg, "%d :: OMX_CommandStateSet SendCommand\n",__LINE__);
            if (nParam == OMX_StateLoaded) {
                pCompPrivate->bLoadedCommandPending = OMX_TRUE;
            }            
            if(pCompPrivate->curState == OMX_StateLoaded) {
                if((nParam == OMX_StateExecuting) || (nParam == OMX_StatePause)) {
                    pCompPrivate->cbInfo.EventHandler ( pHandle,
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventError,
                                                        OMX_ErrorIncorrectStateTransition,
                                                        OMX_TI_ErrorMinor,
                                                        NULL);
                    goto EXIT;
                }

                if(nParam == OMX_StateInvalid) {
                    OMX_PRINT1(pCompPrivate->dbg, "%d :: OMX_CommandStateSet SendCommand\n",__LINE__);
                    pCompPrivate->curState = OMX_StateInvalid;
                    pCompPrivate->cbInfo.EventHandler ( pHandle,
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventError,
                                                        OMX_ErrorInvalidState,
                                                        OMX_TI_ErrorMinor,
                                                        NULL);
                    goto EXIT;
                }
            }

            /* Add a pending transition */
            if(AddStateTransition(pCompPrivate) != OMX_ErrorNone) {
               return OMX_ErrorUndefined;
            }
            break;

        case OMX_CommandFlush:
            OMX_PRINT1(pCompPrivate->dbg, "%d :: OMX_CommandFlush SendCommand\n",__LINE__);
            if(nParam > 1 && nParam != -1) {
                eError = OMX_ErrorBadPortIndex;
                OMX_ERROR4(pCompPrivate->dbg, "%d :: OMX_ErrorBadPortIndex from SendCommand",__LINE__);
                goto EXIT;
            }
            break;
        case OMX_CommandPortDisable:
            OMX_PRINT1(pCompPrivate->dbg, "%d :: OMX_CommandPortDisable SendCommand\n",__LINE__);
            break;
        case OMX_CommandPortEnable:
            OMX_PRINT1(pCompPrivate->dbg, "%d :: OMX_CommandPortEnable SendCommand\n",__LINE__);
            break;
        case OMX_CommandMarkBuffer:
            OMX_PRINT1(pCompPrivate->dbg, "%d :: OMX_CommandMarkBuffer SendCommand\n",__LINE__);
            if (nParam > 0) {
                eError = OMX_ErrorBadPortIndex;
                OMX_ERROR4(pCompPrivate->dbg, "%d :: OMX_ErrorBadPortIndex from SendCommand",__LINE__);
                goto EXIT;
            }
            break;
        default:
            OMX_ERROR4(pCompPrivate->dbg, "%d :: Command Received Default eError\n",__LINE__);
            pCompPrivate->cbInfo.EventHandler ( pHandle,
                                                pHandle->pApplicationPrivate,
                                                OMX_EventError,
                                                OMX_ErrorUndefined,
                                                OMX_TI_ErrorMinor,
                                                "Invalid Command");
            break;

    }

    nRet = write (pCompPrivate->cmdPipe[1], &Cmd, sizeof(Cmd));
    if (nRet == -1) {
       eError = OMX_ErrorInsufficientResources;
       OMX_ERROR4(pCompPrivate->dbg, "%d :: OMX_ErrorInsufficientResources from SendCommand",__LINE__);
       goto EXIT;
    }

    if (Cmd == OMX_CommandMarkBuffer) {
        nRet = write(pCompPrivate->cmdDataPipe[1], &pCmdData,
                            sizeof(OMX_PTR));
    } else {
        nRet = write(pCompPrivate->cmdDataPipe[1], &nParam,
                            sizeof(OMX_U32));
    }
    if (nRet == -1) {
        OMX_ERROR4(pCompPrivate->dbg, "%d :: OMX_ErrorInsufficientResources from SendCommand",__LINE__);
        if(Cmd == OMX_CommandStateSet) {
           if(RemoveStateTransition(pCompPrivate, OMX_FALSE) != OMX_ErrorNone) {
              return OMX_ErrorUndefined;
           }
        }
        eError = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

#ifdef DSP_RENDERING_ON


        /* add for Tee device control*/
    if(Cmd == OMX_CommandStateSet && nParam == OMX_StateExecuting) {
        if(pCompPrivate->teeMode != TEEMODE_NONE) {
            /* enable acoustic supporting*/
            OMX_PRINT2(pCompPrivate->dbg, "[NBAMR encoder] --- supporting TeeDN\n");
            OMX_PRINT2(pCompPrivate->dbg, "[NBAMR encoder] --- Send Tee commnad\n");

            /* enable Tee device command*/
            cmd_data.hComponent = pHandle;
            /*
            AM_CommandTDNPlayMode           --- play mode only
            AM_CommandTDNLoopBackMode       --- loopback mode pnly
            AM_CommandTDNPlayLoopBackMode   --- loopback + playback mode
            */
            if (pCompPrivate->teeMode == TEEMODE_PLAYBACK) {
                cmd_data.AM_Cmd = AM_CommandTDNPlayMode ;
            }
            else if (pCompPrivate->teeMode == TEEMODE_LOOPBACK) {
                
                cmd_data.AM_Cmd = AM_CommandTDNLoopBackMode ;
            }
            else if (pCompPrivate->teeMode == TEEMODE_PLAYLOOPBACK) {
                cmd_data.AM_Cmd = AM_CommandTDNPlayLoopBackMode ;
            }
            cmd_data.param1 = 0;
            cmd_data.param2 = 0;
            cmd_data.streamID = 0;
            if((write(pCompPrivate->fdwrite, &cmd_data, sizeof(cmd_data)))<0) {
                OMX_TRACE4(pCompPrivate->dbg, "[NBAMR encoder] - fail to send Tee command to audio manager\n");
                eError = OMX_ErrorHardware;
                goto EXIT;
            }
        }
        else {
            OMX_PRDSP2(pCompPrivate->dbg, "[NBAMR encoder] --- Normal DASF Mode\n");
        }
    }
#endif


EXIT:
    OMX_PRINT1(pCompPrivate->dbg, "%d :: Exiting SendCommand()\n", __LINE__);
    OMX_PRINT1(pCompPrivate->dbg, "%d :: Returning = 0x%x\n",__LINE__,eError);
    return eError;
}
/*-------------------------------------------------------------------*/
/**
  *  GetParameter() Gets the current configurations of the component
  *
  * @param hComp         handle for this instance of the component
  * @param nParamIndex
  * @param ComponentParameterStructure
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  **/
/*-------------------------------------------------------------------*/

static OMX_ERRORTYPE GetParameter (OMX_HANDLETYPE hComp,
                                   OMX_INDEXTYPE nParamIndex,
                                   OMX_PTR ComponentParameterStructure)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    AMRENC_COMPONENT_PRIVATE  *pComponentPrivate;
    OMX_PARAM_PORTDEFINITIONTYPE *pParameterStructure;

    pComponentPrivate = (AMRENC_COMPONENT_PRIVATE *)(((OMX_COMPONENTTYPE*)hComp)->pComponentPrivate);
    pParameterStructure = (OMX_PARAM_PORTDEFINITIONTYPE*)ComponentParameterStructure;
    OMX_PRINT1(pComponentPrivate->dbg, "%d :: Entering the GetParameter\n",__LINE__);
    if (pParameterStructure == NULL) {
        eError = OMX_ErrorBadParameter;
        OMX_ERROR4(pComponentPrivate->dbg, "%d :: OMX_ErrorBadPortIndex from GetParameter",__LINE__);
        goto EXIT;
    }

#ifdef _ERROR_PROPAGATION__
    if (pComponentPrivate->curState == OMX_StateInvalid){
        eError = OMX_ErrorInvalidState;
        goto EXIT;
    }
#else
    if(pComponentPrivate->curState == OMX_StateInvalid) {
        eError = OMX_ErrorIncorrectStateOperation;
        OMX_ERROR4(pComponentPrivate->dbg, "%d :: OMX_ErrorIncorrectStateOperation from GetParameter",__LINE__);
        goto EXIT;
    }
#endif
    switch(nParamIndex){
        case OMX_IndexParamAudioInit:
            if (pComponentPrivate->sPortParam == NULL) {
                eError = OMX_ErrorBadParameter;
		break;
	    }
            OMX_PRDSP2(pComponentPrivate->dbg, "%d :: GetParameter OMX_IndexParamAudioInit \n",__LINE__);
            memcpy(ComponentParameterStructure, pComponentPrivate->sPortParam, sizeof(OMX_PORT_PARAM_TYPE));
            break;

        case OMX_IndexParamPortDefinition:

            OMX_PRDSP2(pComponentPrivate->dbg, "%d :: GetParameter OMX_IndexParamPortDefinition \n",__LINE__);
            if(((OMX_PARAM_PORTDEFINITIONTYPE *)(ComponentParameterStructure))->nPortIndex ==
                pComponentPrivate->pPortDef[NBAMRENC_INPUT_PORT]->nPortIndex) {
                    memcpy(ComponentParameterStructure, pComponentPrivate->pPortDef[NBAMRENC_INPUT_PORT], sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
            }
            else if(((OMX_PARAM_PORTDEFINITIONTYPE *)(ComponentParameterStructure))->nPortIndex ==
                                      pComponentPrivate->pPortDef[NBAMRENC_OUTPUT_PORT]->nPortIndex) {
                memcpy(ComponentParameterStructure, pComponentPrivate->pPortDef[NBAMRENC_OUTPUT_PORT], sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
            }
            else {
                OMX_ERROR4(pComponentPrivate->dbg, "%d :: OMX_ErrorBadPortIndex from GetParameter \n",__LINE__);
                eError = OMX_ErrorBadPortIndex;
            }
            break;

        case OMX_IndexParamAudioPortFormat:

            OMX_PRDSP2(pComponentPrivate->dbg, "%d :: GetParameter OMX_IndexParamAudioPortFormat \n",__LINE__);
            if(((OMX_AUDIO_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nPortIndex ==
                pComponentPrivate->pPortDef[NBAMRENC_INPUT_PORT]->nPortIndex) {
                    if(((OMX_AUDIO_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nIndex >
                      pComponentPrivate->pCompPort[NBAMRENC_INPUT_PORT]->pPortFormat->nPortIndex) {
                        eError = OMX_ErrorNoMore;
                    }
                    else {
                        memcpy(ComponentParameterStructure, pComponentPrivate->pCompPort[NBAMRENC_INPUT_PORT]->pPortFormat, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
                    }
                }
                else if(((OMX_AUDIO_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nPortIndex ==
                            pComponentPrivate->pPortDef[NBAMRENC_OUTPUT_PORT]->nPortIndex){
                    if(((OMX_AUDIO_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nIndex >
                                                 pComponentPrivate->pCompPort[NBAMRENC_OUTPUT_PORT]->pPortFormat->nPortIndex) {
                        eError = OMX_ErrorNoMore;
                    }
                    else {
                        memcpy(ComponentParameterStructure, pComponentPrivate->pCompPort[NBAMRENC_OUTPUT_PORT]->pPortFormat, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
                    }
                }
                else {
                    OMX_ERROR4(pComponentPrivate->dbg, "%d :: OMX_ErrorBadPortIndex from GetParameter \n",__LINE__);
                    eError = OMX_ErrorBadPortIndex;
                }
            break;

        case OMX_IndexParamAudioAmr:
            if (pComponentPrivate->amrParams == NULL) {
                eError = OMX_ErrorBadParameter;
	        break;
	    }
            memcpy(ComponentParameterStructure, pComponentPrivate->amrParams, sizeof(OMX_AUDIO_PARAM_AMRTYPE));
            switch (((OMX_AUDIO_PARAM_AMRTYPE *)(ComponentParameterStructure))->eAMRBandMode)
            {
                 case SN_AUDIO_BR122:
                      ((OMX_AUDIO_PARAM_AMRTYPE *)(ComponentParameterStructure))->eAMRBandMode =   OMX_AUDIO_AMRBandModeNB7;
                      break;
                 case SN_AUDIO_BR102:
                      ((OMX_AUDIO_PARAM_AMRTYPE *)(ComponentParameterStructure))->eAMRBandMode =   OMX_AUDIO_AMRBandModeNB6;
                      break;
                 case SN_AUDIO_BR795:
                      ((OMX_AUDIO_PARAM_AMRTYPE *)(ComponentParameterStructure))->eAMRBandMode =   OMX_AUDIO_AMRBandModeNB5;
                      break;
                 case SN_AUDIO_BR74:
                      ((OMX_AUDIO_PARAM_AMRTYPE *)(ComponentParameterStructure))->eAMRBandMode =   OMX_AUDIO_AMRBandModeNB4;
                      break;
                 case SN_AUDIO_BR67:
                      ((OMX_AUDIO_PARAM_AMRTYPE *)(ComponentParameterStructure))->eAMRBandMode =   OMX_AUDIO_AMRBandModeNB3;
                      break;
                 case SN_AUDIO_BR59:
                      ((OMX_AUDIO_PARAM_AMRTYPE *)(ComponentParameterStructure))->eAMRBandMode =   OMX_AUDIO_AMRBandModeNB2;
                      break;
                 case SN_AUDIO_BR515:
                      ((OMX_AUDIO_PARAM_AMRTYPE *)(ComponentParameterStructure))->eAMRBandMode =   OMX_AUDIO_AMRBandModeNB1;
                      break;
                 case SN_AUDIO_475:
                      ((OMX_AUDIO_PARAM_AMRTYPE *)(ComponentParameterStructure))->eAMRBandMode =   OMX_AUDIO_AMRBandModeNB0;
                      break;
                 default:
                      ((OMX_AUDIO_PARAM_AMRTYPE *)(ComponentParameterStructure))->eAMRBandMode = OMX_AUDIO_AMRBandModeNB0;
                      break;   
            }          
            break;

      case OMX_IndexParamPriorityMgmt:
            if (pComponentPrivate->sPriorityMgmt == NULL) {
                eError = OMX_ErrorBadParameter;
	        break;
	    }
            OMX_PRDSP2(pComponentPrivate->dbg, "%d :: GetParameter OMX_IndexParamPriorityMgmt \n",__LINE__);
            memcpy(ComponentParameterStructure, pComponentPrivate->sPriorityMgmt, sizeof(OMX_PRIORITYMGMTTYPE));
            break;

      case OMX_IndexParamAudioPcm:
          if (pComponentPrivate->pcmParams == NULL) {
              eError = OMX_ErrorBadParameter;
	      break;
	  }
          memcpy(ComponentParameterStructure, pComponentPrivate->pcmParams, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
          break;

       case OMX_IndexParamCompBufferSupplier:
     if(((OMX_PARAM_BUFFERSUPPLIERTYPE *)(ComponentParameterStructure))->nPortIndex == OMX_DirInput) {
                    OMX_PRDSP2(pComponentPrivate->dbg, ":: GetParameter OMX_IndexParamCompBufferSupplier \n");
                    /*  memcpy(ComponentParameterStructure, pBufferSupplier, sizeof(OMX_PARAM_BUFFERSUPPLIERTYPE)); */                  
                }
                else if(((OMX_PARAM_BUFFERSUPPLIERTYPE *)(ComponentParameterStructure))->nPortIndex == OMX_DirOutput) {
                    OMX_PRBUFFER1(pComponentPrivate->dbg, ":: GetParameter OMX_IndexParamCompBufferSupplier \n");
                    /*memcpy(ComponentParameterStructure, pBufferSupplier, sizeof(OMX_PARAM_BUFFERSUPPLIERTYPE)); */
                } 
                else {
                    OMX_ERROR1(pComponentPrivate->dbg, ":: OMX_ErrorBadPortIndex from GetParameter");
                    eError = OMX_ErrorBadPortIndex;
                }
                break;      

#ifdef ANDROID
    case (OMX_INDEXTYPE) PV_OMX_COMPONENT_CAPABILITY_TYPE_INDEX:
    {
    OMX_PRINT1(pComponentPrivate->dbg, "Entering PV_OMX_COMPONENT_CAPABILITY_TYPE_INDEX::%d\n", __LINE__);
        PV_OMXComponentCapabilityFlagsType* pCap_flags = (PV_OMXComponentCapabilityFlagsType *) ComponentParameterStructure;
        if (NULL == pCap_flags)
        {
            OMX_ERROR4(pComponentPrivate->dbg, "%d :: ERROR PV_OMX_COMPONENT_CAPABILITY_TYPE_INDEX\n", __LINE__);
            eError =  OMX_ErrorBadParameter;
            goto EXIT;
        }
        OMX_PRINT2(pComponentPrivate->dbg, "%d :: Copying PV_OMX_COMPONENT_CAPABILITY_TYPE_INDEX\n", __LINE__);
        memcpy(pCap_flags, &(pComponentPrivate->iPVCapabilityFlags), sizeof(PV_OMXComponentCapabilityFlagsType));
    eError = OMX_ErrorNone;
    }
    break;
#endif          

        case OMX_IndexParamVideoInit:
                break;

         case OMX_IndexParamImageInit:
                break;

         case OMX_IndexParamOtherInit:
                break;

        default:
            OMX_ERROR4(pComponentPrivate->dbg, "%d :: OMX_ErrorUnsupportedIndex GetParameter \n",__LINE__);
            eError = OMX_ErrorUnsupportedIndex;
            break;
    }
EXIT:
    OMX_PRINT1(pComponentPrivate->dbg, "%d :: Exiting GetParameter\n",__LINE__);
    OMX_PRINT1(pComponentPrivate->dbg, "%d :: Returning = 0x%x\n",__LINE__,eError);
    return eError;
}
/*-------------------------------------------------------------------*/
/**
  *  SetParameter() Sets configuration paramets to the component
  *
  * @param hComp         handle for this instance of the component
  * @param nParamIndex
  * @param pCompParam
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  **/
/*-------------------------------------------------------------------*/

static OMX_ERRORTYPE SetParameter (OMX_HANDLETYPE hComp,
                                   OMX_INDEXTYPE nParamIndex,
                                   OMX_PTR pCompParam)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE* pHandle= (OMX_COMPONENTTYPE*)hComp;
    AMRENC_COMPONENT_PRIVATE  *pComponentPrivate;
    OMX_AUDIO_PARAM_PORTFORMATTYPE* pComponentParam = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE *pComponentParamPort = NULL;
    OMX_AUDIO_PARAM_AMRTYPE *pCompAmrParam = NULL;
    OMX_PARAM_COMPONENTROLETYPE  *pRole;
    OMX_AUDIO_PARAM_PCMMODETYPE *amr_ip;
    OMX_PARAM_BUFFERSUPPLIERTYPE sBufferSupplier;

    pComponentPrivate = (AMRENC_COMPONENT_PRIVATE *)(((OMX_COMPONENTTYPE*)hComp)->pComponentPrivate);


    OMX_PRINT1(pComponentPrivate->dbg, "%d :: Entering the SetParameter\n",__LINE__);
    if (pCompParam == NULL) {
        eError = OMX_ErrorBadParameter;
        OMX_ERROR4(pComponentPrivate->dbg, "%d :: OMX_ErrorBadParameter from SetParameter",__LINE__);
        goto EXIT;
    }
    if (pComponentPrivate->curState != OMX_StateLoaded) {
        eError = OMX_ErrorIncorrectStateOperation;
        OMX_ERROR4(pComponentPrivate->dbg, "%d :: OMX_ErrorIncorrectStateOperation from SetParameter",__LINE__);
        goto EXIT;
    }
#ifdef _ERROR_PROPAGATION__
    if (pComponentPrivate->curState == OMX_StateInvalid){
        eError = OMX_ErrorInvalidState;
        goto EXIT;
    }
#endif
    switch(nParamIndex) {
        case OMX_IndexParamAudioPortFormat:
                OMX_PRDSP2(pComponentPrivate->dbg, "%d :: SetParameter OMX_IndexParamAudioPortFormat \n",__LINE__);
                pComponentParam = (OMX_AUDIO_PARAM_PORTFORMATTYPE *)pCompParam;
                if ( pComponentParam->nPortIndex == pComponentPrivate->pCompPort[NBAMRENC_INPUT_PORT]->pPortFormat->nPortIndex ) {
                    memcpy(pComponentPrivate->pCompPort[NBAMRENC_INPUT_PORT]->pPortFormat, pComponentParam, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
                } else if ( pComponentParam->nPortIndex == pComponentPrivate->pCompPort[NBAMRENC_OUTPUT_PORT]->pPortFormat->nPortIndex ) {
                    memcpy(pComponentPrivate->pCompPort[NBAMRENC_OUTPUT_PORT]->pPortFormat, pComponentParam, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
                } else {
                    OMX_ERROR4(pComponentPrivate->dbg, "%d :: OMX_ErrorBadPortIndex from SetParameter",__LINE__);
                    eError = OMX_ErrorBadPortIndex;
                }
            break;
            
        case OMX_IndexParamAudioAmr:

                OMX_PRDSP2(pComponentPrivate->dbg, "%d :: SetParameter OMX_IndexParamAudioAmr \n",__LINE__);
                pCompAmrParam = (OMX_AUDIO_PARAM_AMRTYPE *)pCompParam;
                if(pCompAmrParam->nPortIndex == 0) {         /* 0 means Input port */
                    memcpy(((AMRENC_COMPONENT_PRIVATE*)
                            pHandle->pComponentPrivate)->pcmParams, pCompAmrParam, sizeof(OMX_AUDIO_PARAM_AMRTYPE));
                } else if (pCompAmrParam->nPortIndex == 1) { /* 1 means Output port */
                    if (((AMRENC_COMPONENT_PRIVATE *) pHandle->pComponentPrivate)->amrParams == NULL) {
                        eError = OMX_ErrorBadParameter;
                        goto EXIT;
                    }

                    memcpy(((AMRENC_COMPONENT_PRIVATE *)
                            pHandle->pComponentPrivate)->amrParams, pCompAmrParam, sizeof(OMX_AUDIO_PARAM_AMRTYPE));

                    switch (pCompAmrParam->eAMRBandMode)
                    {
                             case OMX_AUDIO_AMRBandModeNB7:
                                  pComponentPrivate->amrParams->eAMRBandMode = SN_AUDIO_BR122;
                                  OMX_PRDSP2(pComponentPrivate->dbg, "%d :: SetParameter OMX_IndexParamAudioAmr:: pCompAmrParam->eAMRBandMode = SN_AUDIO_BR122 \n",__LINE__);
                                  break;
                             case OMX_AUDIO_AMRBandModeNB6:
                                  pComponentPrivate->amrParams->eAMRBandMode = SN_AUDIO_BR102;
                                  OMX_PRDSP2(pComponentPrivate->dbg, "%d :: SetParameter OMX_IndexParamAudioAmr:: pCompAmrParam->eAMRBandMode = SN_AUDIO_BR102 \n",__LINE__);
                                  break;
                             case OMX_AUDIO_AMRBandModeNB5:
                                  pComponentPrivate->amrParams->eAMRBandMode = SN_AUDIO_BR795;
                                  OMX_PRDSP2(pComponentPrivate->dbg, "%d :: SetParameter OMX_IndexParamAudioAmr:: pCompAmrParam->eAMRBandMode = SN_AUDIO_BR795 \n",__LINE__);
                                  break;
                             case OMX_AUDIO_AMRBandModeNB4:
                                  pComponentPrivate->amrParams->eAMRBandMode = SN_AUDIO_BR74;
                                  OMX_PRDSP2(pComponentPrivate->dbg, "%d :: SetParameter OMX_IndexParamAudioAmr:: pCompAmrParam->eAMRBandMode = SN_AUDIO_BR74 \n",__LINE__);
                                  break;
                             case OMX_AUDIO_AMRBandModeNB3:
                                  pComponentPrivate->amrParams->eAMRBandMode = SN_AUDIO_BR67;
                                  OMX_PRDSP2(pComponentPrivate->dbg, "%d :: SetParameter OMX_IndexParamAudioAmr:: pCompAmrParam->eAMRBandMode = SN_AUDIO_BR67 \n",__LINE__);
                                  break;
                             case OMX_AUDIO_AMRBandModeNB2:
                                  pComponentPrivate->amrParams->eAMRBandMode = SN_AUDIO_BR59;
                                  OMX_PRDSP2(pComponentPrivate->dbg, "%d :: SetParameter OMX_IndexParamAudioAmr:: pCompAmrParam->eAMRBandMode = SN_AUDIO_BR59 \n",__LINE__);
                                  break;
                             case OMX_AUDIO_AMRBandModeNB1:
                                  pComponentPrivate->amrParams->eAMRBandMode = SN_AUDIO_BR515;
                                  OMX_PRDSP2(pComponentPrivate->dbg, "%d :: SetParameter OMX_IndexParamAudioAmr:: pCompAmrParam->eAMRBandMode = SN_AUDIO_BR515 \n",__LINE__);
                                  break;
                             case OMX_AUDIO_AMRBandModeNB0:
                                  pComponentPrivate->amrParams->eAMRBandMode = SN_AUDIO_475;
                                  OMX_PRDSP2(pComponentPrivate->dbg, "%d :: SetParameter OMX_IndexParamAudioAmr:: pCompAmrParam->eAMRBandMode = SN_AUDIO_BR475 \n",__LINE__);
                                  break;
                             default:
                                  pComponentPrivate->amrParams->eAMRBandMode = SN_AUDIO_BR122;
                                  OMX_PRDSP2(pComponentPrivate->dbg, "%d :: SetParameter OMX_IndexParamAudioAmr:: pCompAmrParam->eAMRBandMode =DEFAULT!! SN_AUDIO_BR122 \n",__LINE__);
                                  break;
                    }
                    if (pCompAmrParam->eAMRFrameFormat == OMX_AUDIO_AMRFrameFormatConformance) {
                        pComponentPrivate->frameMode = NBAMRENC_FORMATCONFORMANCE;
                    }
                    else if((pCompAmrParam->eAMRFrameFormat == OMX_AUDIO_AMRFrameFormatFSF) ||
                            (pCompAmrParam->eAMRFrameFormat == OMX_AUDIO_AMRFrameFormatRTPPayload)){
                            pComponentPrivate->frameMode = NBAMRENC_MIMEMODE;
                            OMX_PRDSP1(pComponentPrivate->dbg, "%d :: SetParameter OMX_IndexParamAudioAmr:: pCompAmrParam->eAMRFrameFormate = FSF \n",__LINE__);
                    }
                    else {
                        pComponentPrivate->frameMode = NBAMRENC_IF2;
                        OMX_PRDSP1(pComponentPrivate->dbg, "%d :: SetParameter OMX_IndexParamAudioAmr:: pCompAmrParam->eAMRFrameFormate = IF2 \n",__LINE__);
                    }
                    if(pCompAmrParam->eAMRDTXMode == OMX_AUDIO_AMRDTXasEFR) {
                        OMX_PRDSP1(pComponentPrivate->dbg, "%d :: SetParameter OMX_IndexParamAudioAmr:: pCompAmrParam->eAMRDTXMode = XasEFR\n",__LINE__);
                        /*     pComponentPrivate->efrMode = 1; */
                    }  
                }
                else {
                    OMX_ERROR4(pComponentPrivate->dbg, "%d :: OMX_ErrorBadPortIndex from SetParameter",__LINE__);
                    eError = OMX_ErrorBadPortIndex;
                }
            break;
            
        case OMX_IndexParamPortDefinition:
                pComponentParamPort = (OMX_PARAM_PORTDEFINITIONTYPE *)pCompParam;
                OMX_PRDSP2(pComponentPrivate->dbg, "%d :: SetParameter OMX_IndexParamPortDefinition \n",__LINE__);
                if(((OMX_PARAM_PORTDEFINITIONTYPE *)(pCompParam))->nPortIndex ==
                                    pComponentPrivate->pPortDef[NBAMRENC_INPUT_PORT]->nPortIndex) {
                    OMX_PRINT1(pComponentPrivate->dbg, "%d :: SetParameter OMX_IndexParamPortDefinition \n",__LINE__);
                    memcpy(pComponentPrivate->pPortDef[NBAMRENC_INPUT_PORT], pCompParam, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
                }
                else if(((OMX_PARAM_PORTDEFINITIONTYPE *)(pCompParam))->nPortIndex ==
                                  pComponentPrivate->pPortDef[NBAMRENC_OUTPUT_PORT]->nPortIndex) {
                    OMX_PRINT1(pComponentPrivate->dbg, "%d :: SetParameter OMX_IndexParamPortDefinition \n",__LINE__);
                    memcpy(pComponentPrivate->pPortDef[NBAMRENC_OUTPUT_PORT], pCompParam, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
                }
                else {
                    OMX_ERROR4(pComponentPrivate->dbg, "%d :: OMX_ErrorBadPortIndex from SetParameter",__LINE__);
                    eError = OMX_ErrorBadPortIndex;
                }
            break;
            
        case OMX_IndexParamPriorityMgmt:
	        if (pComponentPrivate->sPriorityMgmt == NULL) {
                    eError = OMX_ErrorBadParameter;
		    break;
		}
                OMX_PRDSP2(pComponentPrivate->dbg, "%d :: SetParameter OMX_IndexParamPriorityMgmt \n",__LINE__);
                memcpy(pComponentPrivate->sPriorityMgmt, (OMX_PRIORITYMGMTTYPE*)pCompParam, sizeof(OMX_PRIORITYMGMTTYPE));
            break;

        case OMX_IndexParamAudioInit:
	        if (pComponentPrivate->sPortParam == NULL) {
                    eError = OMX_ErrorBadParameter;
		    break;
		}
                OMX_PRDSP2(pComponentPrivate->dbg, "%d :: SetParameter OMX_IndexParamAudioInit \n",__LINE__);
                memcpy(pComponentPrivate->sPortParam, (OMX_PORT_PARAM_TYPE*)pCompParam, sizeof(OMX_PORT_PARAM_TYPE));
            break;
            
        case OMX_IndexParamStandardComponentRole: /*-----------------------------*/
           if (pCompParam) {
               pRole = (OMX_PARAM_COMPONENTROLETYPE *)pCompParam;
               memcpy(&(pComponentPrivate->componentRole), (void *)pRole, sizeof(OMX_PARAM_COMPONENTROLETYPE));
           } else {
               eError = OMX_ErrorBadParameter;
           }
        break;

        case OMX_IndexParamAudioPcm:
            if(pCompParam && (pComponentPrivate->pcmParams)){
                 amr_ip = (OMX_AUDIO_PARAM_PCMMODETYPE *)pCompParam;
                 memcpy(pComponentPrivate->pcmParams, amr_ip, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
            }
            else{
                  eError = OMX_ErrorBadParameter;
            }
        break;

        case OMX_IndexParamCompBufferSupplier:             
            if(((OMX_PARAM_PORTDEFINITIONTYPE *)(pCompParam))->nPortIndex ==
                                    pComponentPrivate->pPortDef[NBAMRENC_INPUT_PORT]->nPortIndex) {
                    OMX_PRBUFFER2(pComponentPrivate->dbg, ":: SetParameter OMX_IndexParamCompBufferSupplier \n");
                                   sBufferSupplier.eBufferSupplier = OMX_BufferSupplyInput;
                                   memcpy(&sBufferSupplier, pCompParam, sizeof(OMX_PARAM_BUFFERSUPPLIERTYPE));                                 
                    
                }
                else if(((OMX_PARAM_PORTDEFINITIONTYPE *)(pCompParam))->nPortIndex ==
                                  pComponentPrivate->pPortDef[NBAMRENC_OUTPUT_PORT]->nPortIndex) {
                    OMX_PRBUFFER1(pComponentPrivate->dbg, ":: SetParameter OMX_IndexParamCompBufferSupplier \n");
                    sBufferSupplier.eBufferSupplier = OMX_BufferSupplyOutput;
                    memcpy(&sBufferSupplier, pCompParam, sizeof(OMX_PARAM_BUFFERSUPPLIERTYPE));
                } 
                else {
                    OMX_ERROR1(pComponentPrivate->dbg, ":: OMX_ErrorBadPortIndex from SetParameter");
                    eError = OMX_ErrorBadPortIndex;
                }
            break;
            
        default:
                OMX_ERROR4(pComponentPrivate->dbg, "%d :: SetParameter OMX_ErrorUnsupportedIndex \n",__LINE__);
                eError = OMX_ErrorUnsupportedIndex;
            break;
    }
EXIT:
    OMX_PRINT1(pComponentPrivate->dbg, "%d :: Exiting SetParameter\n",__LINE__);
    OMX_PRINT1(pComponentPrivate->dbg, "%d :: Returning = 0x%x\n",__LINE__,eError);
    return eError;
}

/*-------------------------------------------------------------------*/
/**
  *  GetConfig() Gets the current configuration of to the component
  *
  * @param hComp         handle for this instance of the component
  * @param nConfigIndex
  * @param ComponentConfigStructure
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  **/
/*-------------------------------------------------------------------*/

static OMX_ERRORTYPE GetConfig (OMX_HANDLETYPE hComp,
                                OMX_INDEXTYPE nConfigIndex,
                                OMX_PTR ComponentConfigStructure)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE* pHandle = (OMX_COMPONENTTYPE*)hComp;
    AMRENC_COMPONENT_PRIVATE *pComponentPrivate =
                         (AMRENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;

    TI_OMX_STREAM_INFO *streamInfo;
    OMX_MALLOC_GENERIC(streamInfo, TI_OMX_STREAM_INFO);
#ifdef _ERROR_PROPAGATION__
    if (pComponentPrivate->curState == OMX_StateInvalid){
        eError = OMX_ErrorInvalidState;
        goto EXIT;
    }
#endif
    if(nConfigIndex == OMX_IndexCustomNBAMRENCStreamIDConfig){
        streamInfo->streamId = pComponentPrivate->streamID;
        memcpy(ComponentConfigStructure,streamInfo,sizeof(TI_OMX_STREAM_INFO));
    } 
    else if(nConfigIndex == OMX_IndexCustomDebug){
    OMX_DBG_GETCONFIG(pComponentPrivate->dbg, ComponentConfigStructure);
    }

EXIT:
    OMX_MEMFREE_STRUCT(streamInfo);
    OMX_PRINT1(pComponentPrivate->dbg, "%d :: Exiting GetConfig. Returning = 0x%x\n",__LINE__,eError);
    return eError;
}
/*-------------------------------------------------------------------*/
/**
  *  SetConfig() Sets the configraiton to the component
  *
  * @param hComp         handle for this instance of the component
  * @param nConfigIndex
  * @param ComponentConfigStructure
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  **/
/*-------------------------------------------------------------------*/

static OMX_ERRORTYPE SetConfig (OMX_HANDLETYPE hComp,
                                OMX_INDEXTYPE nConfigIndex,
                                OMX_PTR ComponentConfigStructure)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    AMRENC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    OMX_COMPONENTTYPE* pHandle = (OMX_COMPONENTTYPE*)hComp;
    TI_OMX_DSP_DEFINITION *pTiDspDefinition;
    OMX_S16 *customFlag = NULL;      
    
    TI_OMX_DATAPATH dataPath;                   
    OMX_AUDIO_CONFIG_VOLUMETYPE *pGainStructure = NULL;
#ifdef DSP_RENDERING_ON
    AM_COMMANDDATATYPE cmd_data;
#endif    
                         
    AMRENC_DPRINT("%d :: Entering SetConfig\n", __LINE__);
    if (pHandle == NULL) {
	AMRENC_DPRINT("%d :: Invalid HANDLE OMX_ErrorBadParameter \n", __LINE__);
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pComponentPrivate = (AMRENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
#ifdef _ERROR_PROPAGATION__
    if (pComponentPrivate->curState == OMX_StateInvalid){
        eError = OMX_ErrorInvalidState;
        goto EXIT;
    }
#endif
    switch (nConfigIndex) {

        case OMX_IndexCustomNBAMRENCModeConfig:
            pTiDspDefinition = (TI_OMX_DSP_DEFINITION*)ComponentConfigStructure;
            if (pTiDspDefinition == NULL) {
                eError = OMX_ErrorBadParameter;
                OMX_ERROR4(pComponentPrivate->dbg, "%d :: OMX_ErrorBadParameter from SetConfig\n",__LINE__);
                goto EXIT;
             }
            pComponentPrivate->acdnMode = pTiDspDefinition->acousticMode;       
            pComponentPrivate->dasfMode = pTiDspDefinition->dasfMode;
            pComponentPrivate->teeMode = pTiDspDefinition->teeMode;
             if( 2 == pComponentPrivate->dasfMode ){
                   pComponentPrivate->dasfMode--;
             }
             pComponentPrivate->streamID = pTiDspDefinition->streamId;

            break;
/**-------------------**/
        case  OMX_IndexCustomNBAMRENCDataPath:
            customFlag = (OMX_S16*)ComponentConfigStructure;
/*            dataPath = *customFlag;             */
            switch(dataPath) {
                case DATAPATH_APPLICATION:
                    OMX_MMMIXER_DATAPATH(pComponentPrivate->sDeviceString, RENDERTYPE_ENCODER, pComponentPrivate->streamID);
                    break;

                case DATAPATH_APPLICATION_TEE:
                    strcpy((char*)pComponentPrivate->sDeviceString, ":i0:o1\0");
                    break;

                case DATAPATH_APPLICATION_RTMIXER:
                    strcpy((char*)pComponentPrivate->sDeviceString,(char*)RTM_STRING_ENCODER);
                    break;
                    
                case DATAPATH_ACDN:
                     strcpy((char*)pComponentPrivate->sDeviceString,(char*)ACDN_STRING_ENCODER);
                     break;
                default:
                break;
                    
            }
        break;
/**-------------------**/
        case OMX_IndexConfigAudioVolume:
#ifdef DSP_RENDERING_ON
            pGainStructure = (OMX_AUDIO_CONFIG_VOLUMETYPE *)ComponentConfigStructure;
            cmd_data.hComponent = hComp;
            cmd_data.AM_Cmd = AM_CommandSWGain;
            cmd_data.param1 = pGainStructure->sVolume.nValue;
            cmd_data.param2 = 0;
            cmd_data.streamID = pComponentPrivate->streamID;

            if((write(pComponentPrivate->fdwrite, &cmd_data, sizeof(cmd_data)))<0)
            {   
                OMX_PRMGR4(pComponentPrivate->dbg, "[NBAMRENC encoder] - fail to send command to audio manager\n");
            }
            else
            {
                OMX_PRMGR2(pComponentPrivate->dbg, "[NBAMRENC encoder] - ok to send command to audio manager\n");
            }
#endif
            break;

    case OMX_IndexCustomDebug: 
        OMX_DBG_SETCONFIG(pComponentPrivate->dbg, ComponentConfigStructure);
        break;
            
    default:
        eError = OMX_ErrorUnsupportedIndex;
        break;
    }
EXIT:
    if (pComponentPrivate != NULL) {
	OMX_PRINT1(pComponentPrivate->dbg, "%d :: Exiting SetConfig\n", __LINE__);
	OMX_PRINT1(pComponentPrivate->dbg, "%d :: Returning = 0x%x\n", __LINE__, eError);
    }
    return eError;
}

/*-------------------------------------------------------------------*/
/**
  *  GetState() Gets the current state of the component
  *
  * @param pCompomponent handle for this instance of the component
  * @param pState
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  **/
/*-------------------------------------------------------------------*/

static OMX_ERRORTYPE GetState (OMX_HANDLETYPE pComponent, OMX_STATETYPE* pState)
{
    OMX_ERRORTYPE eError                        = OMX_ErrorNone;
    OMX_COMPONENTTYPE* pHandle = NULL;
    AMRENC_COMPONENT_PRIVATE* pComponentPrivate = NULL;
    struct timespec abs_time = {0,0};
    int nPendingStateChangeRequests = 0;
    int ret = 0;
    /* Set to sufficiently high value */
    int mutex_timeout = 3;

    if(pComponent == NULL || pState == NULL) {
        return OMX_ErrorBadParameter;
    }

    pHandle = (OMX_COMPONENTTYPE*)pComponent;
    pComponentPrivate = (AMRENC_COMPONENT_PRIVATE*)pHandle->pComponentPrivate;

    /* Retrieve current state */
    if (pHandle && pHandle->pComponentPrivate) {
        /* Check for any pending state transition requests */
        pthread_mutex_lock(&pComponentPrivate->mutexStateChangeRequest);
        while (pComponentPrivate->nPendingStateChangeRequests != 0) {
           /* Wait for component to complete state transition */
           clock_gettime(CLOCK_REALTIME, &abs_time);
           abs_time.tv_sec += mutex_timeout;
           abs_time.tv_nsec = 0;
           ret = pthread_cond_timedwait(&(pComponentPrivate->StateChangeCondition),
                    &(pComponentPrivate->mutexStateChangeRequest), &abs_time);
           if (ret == ETIMEDOUT) {
              OMX_ERROR4(pComponentPrivate->dbg, "GetState() timeout at state %d",
                    pComponentPrivate->curState);
              *pState = OMX_StateInvalid;
              break;
           }
        }
        if (!ret) {
            *pState = pComponentPrivate->curState;
        }
        pthread_mutex_unlock(&pComponentPrivate->mutexStateChangeRequest);
     }
     else {
        eError = OMX_ErrorInvalidComponent;
        *pState = OMX_StateInvalid;
    }

    return eError;

EXIT:
    OMX_PRINT1 (pComponentPrivate->dbg, "%d :: AACENC: Exiting GetState\n", __LINE__);
    return eError;

}

/*-------------------------------------------------------------------*/
/**
  *  EmptyThisBuffer() This callback is used to send the input buffer to
  *  component
  *
  * @param pComponent       handle for this instance of the component
  * @param nPortIndex       input port index
  * @param pBuffer          buffer to be sent to codec
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  **/
/*-------------------------------------------------------------------*/

static OMX_ERRORTYPE EmptyThisBuffer (OMX_HANDLETYPE pComponent,
                                      OMX_BUFFERHEADERTYPE* pBuffer)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    int ret = 0;

    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef;

    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    AMRENC_COMPONENT_PRIVATE *pComponentPrivate =
                         (AMRENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    pPortDef = ((AMRENC_COMPONENT_PRIVATE*)pComponentPrivate)->pPortDef[NBAMRENC_INPUT_PORT];

#ifdef _ERROR_PROPAGATION__
    if (pComponentPrivate->curState == OMX_StateInvalid){
        eError = OMX_ErrorInvalidState;
        goto EXIT;
    }
#endif
#ifdef __PERF_INSTRUMENTATION__
    PERF_ReceivedFrame(pComponentPrivate->pPERF,
                       pBuffer->pBuffer,
                       pBuffer->nFilledLen,
                       PERF_ModuleHLMM);
#endif

    OMX_PRINT1(pComponentPrivate->dbg, "%d :: Entering EmptyThisBuffer\n", __LINE__);
    if (pBuffer == NULL) {
        eError = OMX_ErrorBadParameter;
        OMX_ERROR4(pComponentPrivate->dbg, "%d :: About to return OMX_ErrorBadParameter\n",__LINE__);
        goto EXIT;
    }

    if (pBuffer->nSize != sizeof(OMX_BUFFERHEADERTYPE)) {
        eError = OMX_ErrorBadParameter;
        OMX_ERROR4(pComponentPrivate->dbg, "%d :: About to return OMX_ErrorBadParameter\n",__LINE__);
        goto EXIT;
    }

    if (!pPortDef->bEnabled) {
        eError  = OMX_ErrorIncorrectStateOperation;
        OMX_ERROR4(pComponentPrivate->dbg, "%d :: About to return OMX_ErrorIncorrectStateOperation\n",__LINE__);
        goto EXIT;
    }

    if (pBuffer->nVersion.nVersion != pComponentPrivate->nVersion) {
        eError = OMX_ErrorVersionMismatch;
        OMX_ERROR4(pComponentPrivate->dbg, "%d :: About to return OMX_ErrorVersionMismatch\n",__LINE__);
        goto EXIT;
    }

    if (pBuffer->nInputPortIndex != NBAMRENC_INPUT_PORT) {
        eError  = OMX_ErrorBadPortIndex;
        OMX_ERROR4(pComponentPrivate->dbg, "%d :: About to return OMX_ErrorBadPortIndex\n",__LINE__);
        goto EXIT;
    }

    if (pComponentPrivate->curState != OMX_StateExecuting && pComponentPrivate->curState != OMX_StatePause) {
        eError= OMX_ErrorIncorrectStateOperation;
        OMX_ERROR4(pComponentPrivate->dbg, "%d :: About to return OMX_ErrorIncorrectStateOperation\n",__LINE__);
        goto EXIT;
    }

 
    OMX_PRBUFFER2(pComponentPrivate->dbg, "----------------------------------------------------------------\n");
    OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: Comp Sending Filled ip buff = %p to CompThread\n",__LINE__,pBuffer);
    OMX_PRBUFFER2(pComponentPrivate->dbg, "----------------------------------------------------------------\n");

    pComponentPrivate->app_nBuf--;  
    pComponentPrivate->ProcessingInputBuf++;

    pComponentPrivate->pMarkData = pBuffer->pMarkData;
    pComponentPrivate->hMarkTargetComponent = pBuffer->hMarkTargetComponent;

    pComponentPrivate->nUnhandledEmptyThisBuffers++;
    ret = write (pComponentPrivate->dataPipe[1], &pBuffer, sizeof(OMX_BUFFERHEADERTYPE*));
    if (ret == -1) {
        OMX_ERROR4(pComponentPrivate->dbg, "%d :: Error in Writing to the Data pipe\n", __LINE__);
        eError = OMX_ErrorHardware;
        goto EXIT;
    }
EXIT:
    OMX_PRINT1(pComponentPrivate->dbg, "%d :: Exiting EmptyThisBuffer\n", __LINE__);
    OMX_PRINT1(pComponentPrivate->dbg, "%d :: Returning = 0x%x\n",__LINE__,eError);
    return eError;
}
/*-------------------------------------------------------------------*/
/**
  *  FillThisBuffer() This callback is used to send the output buffer to
  *  the component
  *
  * @param pComponent    handle for this instance of the component
  * @param nPortIndex    output port number
  * @param pBuffer       buffer to be sent to codec
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  **/
/*-------------------------------------------------------------------*/

static OMX_ERRORTYPE FillThisBuffer (OMX_HANDLETYPE pComponent,
                                     OMX_BUFFERHEADERTYPE* pBuffer)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    int ret = 0;
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    AMRENC_COMPONENT_PRIVATE *pComponentPrivate =
                         (AMRENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    pPortDef = ((AMRENC_COMPONENT_PRIVATE*)pComponentPrivate)->pPortDef[NBAMRENC_OUTPUT_PORT];

#ifdef _ERROR_PROPAGATION__
    if (pComponentPrivate->curState == OMX_StateInvalid){
        eError = OMX_ErrorInvalidState;
        goto EXIT;
    }
#endif
#ifdef __PERF_INSTRUMENTATION__
    PERF_ReceivedFrame(pComponentPrivate->pPERF,
                       pBuffer->pBuffer,
                       0,
                       PERF_ModuleHLMM);
#endif
    OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: Entering FillThisBuffer\n", __LINE__);
    OMX_PRBUFFER2(pComponentPrivate->dbg, "------------------------------------------------------------------\n");
    OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: Comp Sending Emptied op buff = %p to CompThread\n",__LINE__,pBuffer);
    OMX_PRBUFFER2(pComponentPrivate->dbg, "------------------------------------------------------------------\n");
    if (pBuffer == NULL) {
        eError = OMX_ErrorBadParameter;
        OMX_ERROR4(pComponentPrivate->dbg, " %d :: About to return OMX_ErrorBadParameter\n",__LINE__);
        goto EXIT;
    }

    if (pBuffer->nSize != sizeof(OMX_BUFFERHEADERTYPE)) {
        eError = OMX_ErrorBadParameter;
        OMX_ERROR4(pComponentPrivate->dbg, " %d :: About to return OMX_ErrorBadParameter\n",__LINE__);
        goto EXIT;
    }

    if (!pPortDef->bEnabled) {
        eError  = OMX_ErrorIncorrectStateOperation;
        OMX_ERROR4(pComponentPrivate->dbg, "%d :: About to return OMX_ErrorIncorrectStateOperation\n",__LINE__);
        goto EXIT;
    }

    if (pBuffer->nVersion.nVersion != pComponentPrivate->nVersion) {
        eError = OMX_ErrorVersionMismatch;
        OMX_ERROR4(pComponentPrivate->dbg, " %d :: About to return OMX_ErrorVersionMismatch\n",__LINE__);
        goto EXIT;
    }

    if (pBuffer->nOutputPortIndex != NBAMRENC_OUTPUT_PORT) {
        eError  = OMX_ErrorBadPortIndex;
        OMX_ERROR4(pComponentPrivate->dbg, " %d :: About to return OMX_ErrorBadPortIndex\n",__LINE__);
        goto EXIT;
    }

    if(pComponentPrivate->curState != OMX_StateExecuting && pComponentPrivate->curState != OMX_StatePause) {
        eError = OMX_ErrorIncorrectStateOperation;
        OMX_ERROR4(pComponentPrivate->dbg, "%d ::OMX_ErrorIncorrectStateOperation because state it is no more OMX_StatePause nor OMX_StateExecuting\n",__LINE__);
        goto EXIT;
    }

    pComponentPrivate->app_nBuf--;
    if(pComponentPrivate->pMarkBuf != NULL){
        pBuffer->hMarkTargetComponent = pComponentPrivate->pMarkBuf->hMarkTargetComponent;
        pBuffer->pMarkData = pComponentPrivate->pMarkBuf->pMarkData;
        pComponentPrivate->pMarkBuf = NULL;
    }

    if (pComponentPrivate->pMarkData != NULL) {
        pBuffer->hMarkTargetComponent = pComponentPrivate->hMarkTargetComponent;
        pBuffer->pMarkData = pComponentPrivate->pMarkData;
        pComponentPrivate->pMarkData = NULL;
    }

    
    pComponentPrivate->ProcessingOutputBuf++;
    pComponentPrivate->nUnhandledFillThisBuffers++;
    ret = write (pComponentPrivate->dataPipe[1], &pBuffer, sizeof (OMX_BUFFERHEADERTYPE*));
    if (ret == -1) {
        OMX_ERROR4(pComponentPrivate->dbg, "%d :: Error in Writing to the Data pipe\n", __LINE__);
        eError = OMX_ErrorHardware;
        goto EXIT;
    }
    pComponentPrivate->nFillThisBufferCount++;
EXIT:
    OMX_PRINT1(pComponentPrivate->dbg, "%d :: Exiting FillThisBuffer\n", __LINE__);
    OMX_PRINT1(pComponentPrivate->dbg, "%d :: Returning = 0x%x\n",__LINE__,eError);
    return eError;
}
/*-------------------------------------------------------------------*/
/**
  * OMX_ComponentDeinit() this methold will de init the component
  *
  * @param pComp         handle for this instance of the component
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  **/
/*-------------------------------------------------------------------*/

static OMX_ERRORTYPE ComponentDeInit(OMX_HANDLETYPE pHandle)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    OMX_COMPONENTTYPE *pComponent = (OMX_COMPONENTTYPE *)pHandle;
    AMRENC_COMPONENT_PRIVATE *pComponentPrivate =
                         (AMRENC_COMPONENT_PRIVATE *)pComponent->pComponentPrivate;
    struct OMX_TI_Debug dbg;
    dbg = pComponentPrivate->dbg;

    OMX_PRINT1(dbg, "%d :: Entering ComponentDeInit\n", __LINE__);

#ifdef __PERF_INSTRUMENTATION__
    PERF_Boundary(pComponentPrivate->pPERF,
                  PERF_BoundaryStart | PERF_BoundaryCleanup);
#endif

#ifdef DSP_RENDERING_ON
    close(pComponentPrivate->fdwrite);
    close(pComponentPrivate->fdread);
#endif
#ifdef RESOURCE_MANAGER_ENABLED
    /* eError = RMProxy_SendCommand(pHandle, RMProxy_FreeResource, OMX_NBAMR_Encoder_COMPONENT, 0, NULL); */
    eError = RMProxy_NewSendCommand(pHandle, RMProxy_FreeResource, OMX_NBAMR_Encoder_COMPONENT, 0, 3456, NULL);
    if (eError != OMX_ErrorNone) {
         OMX_ERROR4(dbg, "%d ::Error returned from destroy ResourceManagerProxy thread\n",
                                                        __LINE__);
    }
    eError = RMProxy_Deinitalize();
    if (eError != OMX_ErrorNone) {
         OMX_ERROR4(dbg, "%d :: Error from RMProxy_Deinitalize\n",__LINE__);
         goto EXIT;
    }
#endif

    pComponentPrivate->bIsThreadstop = 1;
    eError = NBAMRENC_StopComponentThread(pHandle);
    if (eError != OMX_ErrorNone) {
         OMX_ERROR4(dbg, "%d :: Error from NBAMRENC_StopComponentThread\n",__LINE__);
         goto EXIT;
    }
    /* Wait for thread to exit so we can get the status into "eError" */
    /* close the pipe handles */
    eError = NBAMRENC_FreeCompResources(pHandle);
    if (eError != OMX_ErrorNone) {
         OMX_ERROR4(dbg, "%d :: Error from NBAMRENC_FreeCompResources\n",__LINE__);
         goto EXIT;
    }

#ifndef UNDER_CE
    pthread_mutex_destroy(&pComponentPrivate->InLoaded_mutex);
    pthread_cond_destroy(&pComponentPrivate->InLoaded_threshold);

    pthread_mutex_destroy(&pComponentPrivate->InIdle_mutex);
    pthread_cond_destroy(&pComponentPrivate->InIdle_threshold);

    pthread_mutex_destroy(&pComponentPrivate->AlloBuf_mutex);
    pthread_cond_destroy(&pComponentPrivate->AlloBuf_threshold);
#else
    OMX_DestroyEvent(&(pComponentPrivate->InLoaded_event));
    OMX_DestroyEvent(&(pComponentPrivate->InIdle_event));
    OMX_DestroyEvent(&(pComponentPrivate->AlloBuf_event));
#endif

    pthread_mutex_destroy(&pComponentPrivate->mutexStateChangeRequest);
    pthread_cond_destroy(&pComponentPrivate->StateChangeCondition);
#ifdef __PERF_INSTRUMENTATION__
    PERF_Boundary(pComponentPrivate->pPERF,
                  PERF_BoundaryComplete | PERF_BoundaryCleanup);
    PERF_Done(pComponentPrivate->pPERF);
#endif

    OMX_MEMFREE_STRUCT(pComponentPrivate->sDeviceString);
    OMX_MEMFREE_STRUCT(pComponentPrivate);

EXIT:
    OMXDBG_PRINT(stderr, PRINT, 1, 0, "%d :: Exiting ComponentDeInit Returning 0x%x\n", __LINE__, eError);
    return eError;
}

/*-------------------------------------------------------------------*/
/**
  *  ComponentTunnelRequest() this method is not implemented in 1.5
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
/*-------------------------------------------------------------------*/

static OMX_ERRORTYPE ComponentTunnelRequest (OMX_HANDLETYPE hComp,
                                             OMX_U32 nPort, OMX_HANDLETYPE hTunneledComp,
                                             OMX_U32 nTunneledPort,
                                             OMX_TUNNELSETUPTYPE* pTunnelSetup)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    eError = OMX_ErrorNotImplemented;
    OMXDBG_PRINT(stderr, PRINT, 1, 0, "%d :: Entering ComponentTunnelRequest\n", __LINE__);
    OMXDBG_PRINT(stderr, PRINT, 1, 0, "%d :: Exiting ComponentTunnelRequest\n", __LINE__);
    OMXDBG_PRINT(stderr, PRINT, 1, 0, "%d :: Returning = 0x%x\n",__LINE__,eError);
    return eError;
}

/*-------------------------------------------------------------------*/
/**
  *  AllocateBuffer()

  * @param pComp         handle for this instance of the component
  * @param pCallBacks    application callbacks
  * @param ptr
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  **/
/*-------------------------------------------------------------------*/

static OMX_ERRORTYPE AllocateBuffer (OMX_IN OMX_HANDLETYPE hComponent,
                   OMX_INOUT OMX_BUFFERHEADERTYPE** pBuffer,
                   OMX_IN OMX_U32 nPortIndex,
                   OMX_IN OMX_PTR pAppPrivate,
                   OMX_IN OMX_U32 nSizeBytes)

{
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef;
    AMRENC_COMPONENT_PRIVATE *pComponentPrivate;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_BUFFERHEADERTYPE *pBufferHeader;

    pComponentPrivate = (AMRENC_COMPONENT_PRIVATE *)
            (((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);

    pPortDef = ((AMRENC_COMPONENT_PRIVATE*)pComponentPrivate)->pPortDef[nPortIndex];
#ifdef _ERROR_PROPAGATION__
    if (pComponentPrivate->curState == OMX_StateInvalid){
        eError = OMX_ErrorInvalidState;
        goto EXIT;
    }
#endif
    OMX_PRINT1(pComponentPrivate->dbg, "%d :: Entering AllocateBuffer\n", __LINE__);
    OMX_PRCOMM1(pComponentPrivate->dbg, "%d :: pPortDef = %p\n", __LINE__,pPortDef);
    OMX_PRCOMM1(pComponentPrivate->dbg, "%d :: pPortDef->bEnabled = %d\n", __LINE__,pPortDef->bEnabled);
    while (1) {
        if(pPortDef->bEnabled) {
            break;
        }
        pComponentPrivate->AlloBuf_waitingsignal = 1;
#ifndef UNDER_CE
        pthread_mutex_lock(&pComponentPrivate->AlloBuf_mutex);
        pthread_cond_wait(&pComponentPrivate->AlloBuf_threshold, &pComponentPrivate->AlloBuf_mutex);
        pthread_mutex_unlock(&pComponentPrivate->AlloBuf_mutex);
#else
        OMX_WaitForEvent(&(pComponentPrivate->AlloBuf_event));
#endif
        break;
    }

    OMX_MALLOC_GENERIC(pBufferHeader, OMX_BUFFERHEADERTYPE);

    OMX_MALLOC_SIZE_DSPALIGN(pBufferHeader->pBuffer, nSizeBytes, OMX_U8);
    
    if (nPortIndex == NBAMRENC_INPUT_PORT) {        
        pBufferHeader->nInputPortIndex = nPortIndex;
        pBufferHeader->nOutputPortIndex = -1;
        pComponentPrivate->pInputBufferList->pBufHdr[pComponentPrivate->pInputBufferList->numBuffers] = pBufferHeader;
        pComponentPrivate->pInputBufferList->bBufferPending[pComponentPrivate->pInputBufferList->numBuffers] = 0;
        pComponentPrivate->pInputBufferList->bufferOwner[pComponentPrivate->pInputBufferList->numBuffers++] = 1;
        if (pComponentPrivate->pInputBufferList->numBuffers == pPortDef->nBufferCountActual) {
            pPortDef->bPopulated = OMX_TRUE;
            OMX_PRCOMM1(pComponentPrivate->dbg, "%d :: pPortDef->bPopulated = %d\n", __LINE__, pPortDef->bPopulated);
        }
    }
    else if (nPortIndex == NBAMRENC_OUTPUT_PORT) {
        pBufferHeader->nInputPortIndex = -1;
        pBufferHeader->nOutputPortIndex = nPortIndex;
        pComponentPrivate->pOutputBufferList->pBufHdr[pComponentPrivate->pOutputBufferList->numBuffers] = pBufferHeader;
        pComponentPrivate->pOutputBufferList->bBufferPending[pComponentPrivate->pOutputBufferList->numBuffers] = 0;
        pComponentPrivate->pOutputBufferList->bufferOwner[pComponentPrivate->pOutputBufferList->numBuffers++] = 1;
        OMX_MALLOC_GENERIC(pBufferHeader->pOutputPortPrivate, NBAMRENC_BUFDATA);

        if (pComponentPrivate->pOutputBufferList->numBuffers == pPortDef->nBufferCountActual) {
            pPortDef->bPopulated = OMX_TRUE;
            OMX_PRCOMM1(pComponentPrivate->dbg, "%d :: pPortDef->bPopulated = %d\n", __LINE__, pPortDef->bPopulated);
        }
    }
    else {
        eError = OMX_ErrorBadPortIndex;
        OMX_ERROR4(pComponentPrivate->dbg, " %d :: About to return OMX_ErrorBadPortIndex\n",__LINE__);
        goto EXIT;
    }
    if (((!pComponentPrivate->dasfMode) &&
       ((pComponentPrivate->pPortDef[NBAMRENC_OUTPUT_PORT]->bPopulated == pComponentPrivate->pPortDef[NBAMRENC_OUTPUT_PORT]->bEnabled)&&
       (pComponentPrivate->pPortDef[NBAMRENC_INPUT_PORT]->bPopulated == pComponentPrivate->pPortDef[NBAMRENC_INPUT_PORT]->bEnabled) &&
       (pComponentPrivate->InLoaded_readytoidle)))/*File Mode*/  
       || 
       ((pComponentPrivate->dasfMode) &&
       ((pComponentPrivate->pPortDef[NBAMRENC_OUTPUT_PORT]->bPopulated == pComponentPrivate->pPortDef[NBAMRENC_OUTPUT_PORT]->bEnabled)&&
       (pComponentPrivate->InLoaded_readytoidle))))/*Dasf Mode*/                                               
    {
        pComponentPrivate->InLoaded_readytoidle = 0;
#ifndef UNDER_CE
        pthread_mutex_lock(&pComponentPrivate->InLoaded_mutex);
        pthread_cond_signal(&pComponentPrivate->InLoaded_threshold);
        pthread_mutex_unlock(&pComponentPrivate->InLoaded_mutex);
#else
        OMX_SignalEvent(&(pComponentPrivate->InLoaded_event));
#endif
    }
    pBufferHeader->pAppPrivate = pAppPrivate;
    pBufferHeader->pPlatformPrivate = pComponentPrivate;
    pBufferHeader->nAllocLen = nSizeBytes;
    pBufferHeader->nVersion.s.nVersionMajor = NBAMRENC_MAJOR_VER;
    pBufferHeader->nVersion.s.nVersionMinor = NBAMRENC_MINOR_VER;
    pComponentPrivate->nVersion = pBufferHeader->nVersion.nVersion;
    pBufferHeader->nSize = sizeof(OMX_BUFFERHEADERTYPE);
    *pBuffer = pBufferHeader;

    if (pComponentPrivate->bEnableCommandPending && pPortDef->bPopulated) {
        SendCommand (pComponentPrivate->pHandle,
                     OMX_CommandPortEnable,
                     pComponentPrivate->bEnableCommandParam,NULL);
    }



EXIT:

#ifdef __PERF_INSTRUMENTATION__
    PERF_ReceivedBuffer(pComponentPrivate->pPERF,
                        (*pBuffer)->pBuffer, nSizeBytes,
                        PERF_ModuleMemory);
#endif

    OMX_PRINT1(pComponentPrivate->dbg, "%d :: Exiting AllocateBuffer\n",__LINE__);
    OMX_PRINT1(pComponentPrivate->dbg, "%d :: Returning = 0x%x\n",__LINE__,eError);
    return eError;
}

/*-------------------------------------------------------------------*/
/**
  *  FreeBuffer()

  * @param hComponent   handle for this instance of the component
  * @param pCallBacks   application callbacks
  * @param ptr
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  **/
/*-------------------------------------------------------------------*/

static OMX_ERRORTYPE FreeBuffer(
            OMX_IN  OMX_HANDLETYPE hComponent,
            OMX_IN  OMX_U32 nPortIndex,
            OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    AMRENC_COMPONENT_PRIVATE * pComponentPrivate = NULL;
    OMX_BUFFERHEADERTYPE* buff;
    int i = 0;
    int inputIndex = -1;
    int outputIndex = -1;
    OMX_COMPONENTTYPE *pHandle;

    pComponentPrivate = (AMRENC_COMPONENT_PRIVATE *)
                                (((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);

    pHandle = (OMX_COMPONENTTYPE *) pComponentPrivate->pHandle;

    OMX_PRINT1(pComponentPrivate->dbg, "%d :: Entering FreeBuffer\n", __LINE__);
    for (i=0; i < NBAMRENC_MAX_NUM_OF_BUFS; i++) {
        buff = pComponentPrivate->pInputBufferList->pBufHdr[i];
        if (buff == pBuffer) {
            OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: Found matching input buffer\n",__LINE__);
            OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: buff = %p\n",__LINE__,buff);
            OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: pBuffer = %p\n",__LINE__,pBuffer);
            inputIndex = i;
            break;
        }
        else {
            OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: This is not a match\n",__LINE__);
            OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: buff = %p\n",__LINE__,buff);
            OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: pBuffer = %p\n",__LINE__,pBuffer);
        }
    }

    for (i=0; i < NBAMRENC_MAX_NUM_OF_BUFS; i++) {
        buff = pComponentPrivate->pOutputBufferList->pBufHdr[i];
        if (buff == pBuffer) {
            OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: Found matching output buffer\n",__LINE__);
            OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: buff = %p\n",__LINE__,buff);
            OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: pBuffer = %p\n",__LINE__,pBuffer);
            outputIndex = i;
            break;
        }
        else {
            OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: This is not a match\n",__LINE__);
            OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: buff = %p\n",__LINE__,buff);
            OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: pBuffer = %p\n",__LINE__,pBuffer);
        }
    }


    if (inputIndex != -1) {
        if (pComponentPrivate->pInputBufferList->bufferOwner[inputIndex] == 1) {
            OMX_MEMFREE_STRUCT_DSPALIGN(pComponentPrivate->pInputBufferList->pBufHdr[inputIndex]->pBuffer, OMX_U8);
        }

#ifdef __PERF_INSTRUMENTATION__
    PERF_SendingBuffer(pComponentPrivate->pPERF,
                       pComponentPrivate->pInputBufferList->pBufHdr[inputIndex]->pBuffer,
                       pComponentPrivate->pInputBufferList->pBufHdr[inputIndex]->nAllocLen,
                       PERF_ModuleMemory );
#endif
        
        OMX_MEMFREE_STRUCT(pComponentPrivate->pInputBufferList->pBufHdr[inputIndex]);

            pComponentPrivate->pInputBufferList->numBuffers--;
        if (pComponentPrivate->pInputBufferList->numBuffers <
            pComponentPrivate->pPortDef[NBAMRENC_INPUT_PORT]->nBufferCountMin) {

            pComponentPrivate->pPortDef[NBAMRENC_INPUT_PORT]->bPopulated = OMX_FALSE;
            }
        if(pComponentPrivate->pPortDef[NBAMRENC_INPUT_PORT]->bEnabled &&
               pComponentPrivate->bLoadedCommandPending == OMX_FALSE &&
                (pComponentPrivate->curState == OMX_StateIdle ||
                pComponentPrivate->curState == OMX_StateExecuting ||
                pComponentPrivate->curState == OMX_StatePause)) {
            pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                    pHandle->pApplicationPrivate,
                                                    OMX_EventError,
                                                    OMX_ErrorPortUnpopulated,
                                                    OMX_TI_ErrorMinor,
                                                    "Input Port Unpopulated");
        }
    }
    else if (outputIndex != -1) {
         if (pComponentPrivate->pOutputBufferList->bufferOwner[outputIndex] == 1) {
            OMX_MEMFREE_STRUCT_DSPALIGN(pComponentPrivate->pOutputBufferList->pBufHdr[outputIndex]->pBuffer, OMX_U8);
        }

#ifdef __PERF_INSTRUMENTATION__
    PERF_SendingBuffer(pComponentPrivate->pPERF,
                       pComponentPrivate->pOutputBufferList->pBufHdr[outputIndex]->pBuffer,
                       pComponentPrivate->pOutputBufferList->pBufHdr[outputIndex]->nAllocLen,
                       PERF_ModuleMemory );
#endif

        OMX_MEMFREE_STRUCT(pComponentPrivate->pOutputBufferList->pBufHdr[outputIndex]->pOutputPortPrivate);
        OMX_MEMFREE_STRUCT(pComponentPrivate->pOutputBufferList->pBufHdr[outputIndex]);

        pComponentPrivate->pOutputBufferList->numBuffers--;
        if (pComponentPrivate->pOutputBufferList->numBuffers <
            pComponentPrivate->pPortDef[NBAMRENC_OUTPUT_PORT]->nBufferCountMin) {
            pComponentPrivate->pPortDef[NBAMRENC_OUTPUT_PORT]->bPopulated = OMX_FALSE;
        }
        if( pComponentPrivate->pPortDef[NBAMRENC_OUTPUT_PORT]->bEnabled &&
            pComponentPrivate->bLoadedCommandPending == OMX_FALSE &&
            (pComponentPrivate->curState == OMX_StateIdle ||
            pComponentPrivate->curState == OMX_StateExecuting ||
            pComponentPrivate->curState == OMX_StatePause)) {
            pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                    pHandle->pApplicationPrivate,
                                                    OMX_EventError,
                                                    OMX_ErrorPortUnpopulated,
                                                    OMX_TI_ErrorMinor,
                                                    "Output Port Unpopulated");
        }
    }
    else {
        OMX_ERROR4(pComponentPrivate->dbg, "%d :: Returning OMX_ErrorBadParameter\n",__LINE__);
        eError = OMX_ErrorBadParameter;
    }
    
    pthread_mutex_lock(&pComponentPrivate->ToLoaded_mutex);
    OMX_PRBUFFER1(pComponentPrivate->dbg, "num input buffers es %d, \n num output buffers es %d,\n InIdle_goingtoloaded es %d \n",
            pComponentPrivate->pInputBufferList->numBuffers, pComponentPrivate->pOutputBufferList->numBuffers, pComponentPrivate->InIdle_goingtoloaded);

       if ((!pComponentPrivate->pInputBufferList->numBuffers &&
            !pComponentPrivate->pOutputBufferList->numBuffers) &&
            pComponentPrivate->InIdle_goingtoloaded)
       {
#ifndef UNDER_CE
           pthread_mutex_lock(&pComponentPrivate->InIdle_mutex);
           pComponentPrivate->InIdle_goingtoloaded = 0;
           pthread_cond_signal(&pComponentPrivate->InIdle_threshold);
           pthread_mutex_unlock(&pComponentPrivate->InIdle_mutex);
#else
           OMX_SignalEvent(&(pComponentPrivate->InIdle_event));
#endif
       }

    pthread_mutex_unlock(&pComponentPrivate->ToLoaded_mutex);
    if (pComponentPrivate->bDisableCommandPending &&
        (pComponentPrivate->pInputBufferList->numBuffers + pComponentPrivate->pOutputBufferList->numBuffers == 0)) {
            SendCommand (pComponentPrivate->pHandle,OMX_CommandPortDisable,pComponentPrivate->bDisableCommandParam,NULL);
    }
    OMX_PRINT1(pComponentPrivate->dbg, "%d :: Exiting FreeBuffer\n", __LINE__);
    OMX_PRINT1(pComponentPrivate->dbg, "%d :: Returning = 0x%x\n",__LINE__,eError);
    return eError;
}

/*-------------------------------------------------------------------*/
/**
  *  UseBuffer()

  * @param pComp         handle for this instance of the component
  * @param pCallBacks    application callbacks
  * @param ptr
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  **/
/*-------------------------------------------------------------------*/
static OMX_ERRORTYPE UseBuffer (
            OMX_IN OMX_HANDLETYPE hComponent,
            OMX_INOUT OMX_BUFFERHEADERTYPE** ppBufferHdr,
            OMX_IN OMX_U32 nPortIndex,
            OMX_IN OMX_PTR pAppPrivate,
            OMX_IN OMX_U32 nSizeBytes,
            OMX_IN OMX_U8* pBuffer)
{
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef;
    AMRENC_COMPONENT_PRIVATE *pComponentPrivate;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_BUFFERHEADERTYPE *pBufferHeader;

    pComponentPrivate = (AMRENC_COMPONENT_PRIVATE *)
            (((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);

#ifdef _ERROR_PROPAGATION__
    if (pComponentPrivate->curState == OMX_StateInvalid){
        eError = OMX_ErrorInvalidState;
        goto EXIT;
    }

#endif
#ifdef __PERF_INSTRUMENTATION__
    PERF_ReceivedBuffer(pComponentPrivate->pPERF,
                        pBuffer, nSizeBytes,
                        PERF_ModuleHLMM);
#endif

    pPortDef = ((AMRENC_COMPONENT_PRIVATE*)pComponentPrivate)->pPortDef[nPortIndex];
    OMX_PRBUFFER1(pComponentPrivate->dbg, "%d :: Entering UseBuffer\n", __LINE__);
    OMX_PRCOMM1(pComponentPrivate->dbg, "%d :: pPortDef->bPopulated = %d \n",__LINE__,pPortDef->bPopulated);

    if(!pPortDef->bEnabled) {
        OMX_ERROR4(pComponentPrivate->dbg, "%d :: About to return OMX_ErrorIncorrectStateOperation\n",__LINE__);
        eError = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }

    OMX_MALLOC_GENERIC(pBufferHeader, OMX_BUFFERHEADERTYPE);
    
    if (nPortIndex == NBAMRENC_OUTPUT_PORT) {
        pBufferHeader->nInputPortIndex = -1;
        pBufferHeader->nOutputPortIndex = nPortIndex;
        pComponentPrivate->pOutputBufferList->pBufHdr[pComponentPrivate->pOutputBufferList->numBuffers] = pBufferHeader;
        pComponentPrivate->pOutputBufferList->bBufferPending[pComponentPrivate->pOutputBufferList->numBuffers] = 0;
        pComponentPrivate->pOutputBufferList->bufferOwner[pComponentPrivate->pOutputBufferList->numBuffers++] = 0;
        OMX_MALLOC_GENERIC(pBufferHeader->pOutputPortPrivate, NBAMRENC_BUFDATA);

        if (pComponentPrivate->pOutputBufferList->numBuffers == pPortDef->nBufferCountActual) {
            pPortDef->bPopulated = OMX_TRUE;
        }
    }
    else {
        pBufferHeader->nInputPortIndex = nPortIndex;
        pBufferHeader->nOutputPortIndex = -1;
        pComponentPrivate->pInputBufferList->pBufHdr[pComponentPrivate->pInputBufferList->numBuffers] = pBufferHeader;
        pComponentPrivate->pInputBufferList->bBufferPending[pComponentPrivate->pInputBufferList->numBuffers] = 0;
        pComponentPrivate->pInputBufferList->bufferOwner[pComponentPrivate->pInputBufferList->numBuffers++] = 0;
        if (pComponentPrivate->pInputBufferList->numBuffers == pPortDef->nBufferCountActual) {
            pPortDef->bPopulated = OMX_TRUE;
        }
    }

    if (((!pComponentPrivate->dasfMode) &&
       ((pComponentPrivate->pPortDef[NBAMRENC_OUTPUT_PORT]->bPopulated == pComponentPrivate->pPortDef[NBAMRENC_OUTPUT_PORT]->bEnabled)&&
       (pComponentPrivate->pPortDef[NBAMRENC_INPUT_PORT]->bPopulated == pComponentPrivate->pPortDef[NBAMRENC_INPUT_PORT]->bEnabled) &&
       (pComponentPrivate->InLoaded_readytoidle)))/*File Mode*/  
       || 
       ((pComponentPrivate->dasfMode) &&
       ((pComponentPrivate->pPortDef[NBAMRENC_OUTPUT_PORT]->bPopulated == pComponentPrivate->pPortDef[NBAMRENC_OUTPUT_PORT]->bEnabled)&&
       (pComponentPrivate->InLoaded_readytoidle))))/*Dasf Mode*/ 
    {
        pComponentPrivate->InLoaded_readytoidle = 0;
#ifndef UNDER_CE
        pthread_mutex_lock(&pComponentPrivate->InLoaded_mutex);
        pthread_cond_signal(&pComponentPrivate->InLoaded_threshold);
        pthread_mutex_unlock(&pComponentPrivate->InLoaded_mutex);
#else
        OMX_SignalEvent(&(pComponentPrivate->InLoaded_event));
#endif
    }
    pBufferHeader->pAppPrivate = pAppPrivate;
    pBufferHeader->pPlatformPrivate = pComponentPrivate;
    pBufferHeader->nAllocLen = nSizeBytes;
    pBufferHeader->nVersion.s.nVersionMajor = NBAMRENC_MAJOR_VER;
    pBufferHeader->nVersion.s.nVersionMinor = NBAMRENC_MINOR_VER;
    pComponentPrivate->nVersion = pBufferHeader->nVersion.nVersion;
    pBufferHeader->pBuffer = pBuffer;
    pBufferHeader->nSize = sizeof(OMX_BUFFERHEADERTYPE);
    *ppBufferHdr = pBufferHeader;

    if (pComponentPrivate->bEnableCommandPending){
        SendCommand (pComponentPrivate->pHandle,
                     OMX_CommandPortEnable,
                     pComponentPrivate->bEnableCommandParam,NULL);
    }
        
EXIT:
    OMX_PRINT1(pComponentPrivate->dbg, "%d :: Exiting UseBuffer\n", __LINE__);
    OMX_PRINT1(pComponentPrivate->dbg, "%d :: Returning = 0x%x\n",__LINE__,eError);
    return eError;
}

/* ================================================================================= */
/**
* @fn GetExtensionIndex() description for GetExtensionIndex
GetExtensionIndex().
Returns index for vendor specific settings.
*
*  @see         OMX_Core.h
*/
/* ================================================================================ */
static OMX_ERRORTYPE GetExtensionIndex(
            OMX_IN  OMX_HANDLETYPE hComponent,
            OMX_IN  OMX_STRING cParameterName,
            OMX_OUT OMX_INDEXTYPE* pIndexType)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    OMXDBG_PRINT(stderr, PRINT, 1, 0, "GetExtensionIndex\n");
    
    if (!(strcmp(cParameterName,"OMX.TI.index.config.tispecific"))) {
        *pIndexType = OMX_IndexCustomNBAMRENCModeConfig;
        OMXDBG_PRINT(stderr, PRINT, 2, 0, "OMX_IndexCustomNBAMRENCModeConfig\n");
    }
    else if(!(strcmp(cParameterName,"OMX.TI.index.config.nbamrstreamIDinfo"))) {
        *pIndexType = OMX_IndexCustomNBAMRENCStreamIDConfig;
    }
    else if(!(strcmp(cParameterName,"OMX.TI.index.config.nbamr.datapath"))) 
    {
        *pIndexType = OMX_IndexCustomNBAMRENCDataPath;
        }    
    else if(!(strcmp(cParameterName,"OMX.TI.AMR.Encode.Debug"))) 
    {
        *pIndexType = OMX_IndexCustomDebug;
        }    
    else {
        eError = OMX_ErrorBadParameter;
    }

    OMXDBG_PRINT(stderr, PRINT, 1, 0, "Exiting GetExtensionIndex\n");
    return eError;
}

#ifdef UNDER_CE
/* ================================================================================= */
/**
* @fns Sleep replace for WIN CE
*/
/* ================================================================================ */
int OMX_CreateEvent(OMX_Event *event){
    int ret = OMX_ErrorNone;
    HANDLE createdEvent = NULL;
    if(event == NULL){
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    event->event  = CreateEvent(NULL, TRUE, FALSE, NULL);
    if(event->event == NULL)
        ret = (int)GetLastError();
EXIT:
    return ret;
}

int OMX_SignalEvent(OMX_Event *event){
     int ret = OMX_ErrorNone;
     if(event == NULL){
        ret = OMX_ErrorBadParameter;
        goto EXIT;
     }
     SetEvent(event->event);
     ret = (int)GetLastError();
EXIT:
    return ret;
}

int OMX_WaitForEvent(OMX_Event *event) {
     int ret = OMX_ErrorNone;
     if(event == NULL){
        ret = OMX_ErrorBadParameter;
        goto EXIT;
     }
     WaitForSingleObject(event->event, INFINITE);
     ret = (int)GetLastError();
EXIT:
     return ret;
}

int OMX_DestroyEvent(OMX_Event *event) {
     int ret = OMX_ErrorNone;
     if(event == NULL){
        ret = OMX_ErrorBadParameter;
        goto EXIT;
     }
     CloseHandle(event->event);
EXIT:
     return ret;
}
#endif

/* ================================================================================= */
/**
* @fn ComponentRoleEnum() description for ComponentRoleEnum()  

Returns the role at the given index
*
*  @see         OMX_Core.h
*/
/* ================================================================================ */
static OMX_ERRORTYPE ComponentRoleEnum(
         OMX_IN OMX_HANDLETYPE hComponent,
      OMX_OUT OMX_U8 *cRole,
      OMX_IN OMX_U32 nIndex)
{
    AMRENC_COMPONENT_PRIVATE *pComponentPrivate;
    
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    pComponentPrivate = (AMRENC_COMPONENT_PRIVATE *)(((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);

    if(nIndex == 0){
      if (cRole == NULL) {
          eError = OMX_ErrorBadParameter;
      }
      else {
          memcpy(cRole, &pComponentPrivate->componentRole.cRole, sizeof(OMX_U8) * OMX_MAX_STRINGNAME_SIZE); 
          OMX_PRINT1(pComponentPrivate->dbg, "::::In ComponenetRoleEnum: cRole is set to %s\n",cRole);
      }
    }
    else {
      eError = OMX_ErrorNoMore;
        }
    return eError;
}
