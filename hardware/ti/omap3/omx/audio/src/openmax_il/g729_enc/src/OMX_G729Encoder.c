
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
 * @file OMX_G729Encoder.c
 *
 * This file implements OpenMAX (TM) 1.0 Specific APIs and its functionality
 * that is fully compliant with the Khronos OpenMAX (TM) 1.0 Specification
 *
 * @path  $(CSLPATH)\OMAPSW_MPU\linux\audio\src\openmax_il\g729_enc\src
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
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <errno.h>
#include <pthread.h>
#include <stdarg.h> 
#endif

#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <dbapi.h>

#ifdef __PERF_INSTRUMENTATION__
#include "perf.h"
#endif
/*-------program files ----------------------------------------*/
#include <OMX_Component.h>

#include "OMX_G729Enc_Utils.h"

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

static OMX_ERRORTYPE SetCallbacks (OMX_HANDLETYPE hComp,
                                   OMX_CALLBACKTYPE* pCallBacks,
                                   OMX_PTR pAppData);
static OMX_ERRORTYPE GetComponentVersion (OMX_HANDLETYPE hComp,
                                          OMX_STRING pComponentName,
                                          OMX_VERSIONTYPE* pComponentVersion,
                                          OMX_VERSIONTYPE* pSpecVersion,
                                          OMX_UUIDTYPE* pComponentUUID);
static OMX_ERRORTYPE SendCommand (OMX_HANDLETYPE hComp,
                                  OMX_COMMANDTYPE nCommand,
                                  OMX_U32 nParam,OMX_PTR pCmdData);
static OMX_ERRORTYPE GetParameter(OMX_HANDLETYPE hComp,
                                  OMX_INDEXTYPE nParamIndex,
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
static OMX_ERRORTYPE EmptyThisBuffer (OMX_HANDLETYPE hComp,
                                      OMX_BUFFERHEADERTYPE* pBuffer);
static OMX_ERRORTYPE FillThisBuffer (OMX_HANDLETYPE hComp,
                                     OMX_BUFFERHEADERTYPE* pBuffer);
static OMX_ERRORTYPE GetState (OMX_HANDLETYPE hComp, OMX_STATETYPE* pState);
static OMX_ERRORTYPE ComponentTunnelRequest (OMX_HANDLETYPE hComp,
                                             OMX_U32 nPort,
                                             OMX_HANDLETYPE hTunneledComp,
                                             OMX_U32 nTunneledPort,
                                             OMX_TUNNELSETUPTYPE* pTunnelSetup);
static OMX_ERRORTYPE ComponentDeInit(OMX_HANDLETYPE pHandle);
static OMX_ERRORTYPE AllocateBuffer (OMX_IN OMX_HANDLETYPE hComponent,
                                     OMX_INOUT OMX_BUFFERHEADERTYPE** pBuffer,
                                     OMX_IN OMX_U32 nPortIndex,
                                     OMX_IN OMX_PTR pAppPrivate,
                                     OMX_IN OMX_U32 nSizeBytes);
static OMX_ERRORTYPE FreeBuffer(OMX_IN  OMX_HANDLETYPE hComponent,
                                OMX_IN  OMX_U32 nPortIndex,
                                OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer);
static OMX_ERRORTYPE UseBuffer (OMX_IN OMX_HANDLETYPE hComponent,
                                OMX_INOUT OMX_BUFFERHEADERTYPE** ppBufferHdr,
                                OMX_IN OMX_U32 nPortIndex,
                                OMX_IN OMX_PTR pAppPrivate,
                                OMX_IN OMX_U32 nSizeBytes,
                                OMX_IN OMX_U8* pBuffer);
static OMX_ERRORTYPE GetExtensionIndex(OMX_IN  OMX_HANDLETYPE hComponent,
                                       OMX_IN  OMX_STRING cParameterName,
                                       OMX_OUT OMX_INDEXTYPE* pIndexType);

static OMX_ERRORTYPE ComponentRoleEnum(
                                       OMX_IN OMX_HANDLETYPE hComponent,
                                       OMX_OUT OMX_U8 *cRole,
                                       OMX_IN OMX_U32 nIndex);
                
/* interface with audio manager*/
#define FIFO1 "/dev/fifo.1"
#define FIFO2 "/dev/fifo.2"
#define PERMS 0666

#ifdef DSP_RENDERING_ON
AM_COMMANDDATATYPE cmd_data;
#endif



/*-------------------------------------------------------------------*/
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
/*-------------------------------------------------------------------*/
OMX_ERRORTYPE OMX_ComponentInit (OMX_HANDLETYPE hComp)
{
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef_ip = NULL, *pPortDef_op = NULL;
    G729ENC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    OMX_AUDIO_PARAM_G729TYPE *g729_op = NULL;
    OMX_AUDIO_PARAM_PCMMODETYPE *g729_ip = NULL;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE*) hComp;
    G729ENC_PORT_TYPE *pCompPort = NULL;
    OMX_AUDIO_PARAM_PORTFORMATTYPE *pInPortFormat = NULL;
    OMX_AUDIO_PARAM_PORTFORMATTYPE *pOutPortFormat = NULL;
    int i = 0;

    G729ENC_DPRINT("OMX_ComponentInit\n");
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
    OMX_G729MALLOC_STRUCT(pHandle->pComponentPrivate, G729ENC_COMPONENT_PRIVATE);
    ((G729ENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate)->pHandle = pHandle;
    pComponentPrivate = pHandle->pComponentPrivate;

#ifdef __PERF_INSTRUMENTATION__
    pComponentPrivate->pPERF = PERF_Create(PERF_FOURCC('7','2','9',' '),
                                           PERF_ModuleLLMM |
                                           PERF_ModuleAudioDecode);
#endif
    OMX_G729MALLOC_STRUCT(pCompPort, G729ENC_PORT_TYPE);
    pComponentPrivate->pCompPort[G729ENC_INPUT_PORT] = pCompPort;

    OMX_G729MALLOC_STRUCT(pCompPort, G729ENC_PORT_TYPE);
    pComponentPrivate->pCompPort[G729ENC_OUTPUT_PORT] = pCompPort;

    OMX_G729MALLOC_STRUCT(pComponentPrivate->sPortParam, OMX_PORT_PARAM_TYPE);
    OMX_G729CONF_INIT_STRUCT(pComponentPrivate->sPortParam, OMX_PORT_PARAM_TYPE);

    /* Initialize sPortParam data structures to default values */
    pComponentPrivate->sPortParam->nPorts = 0x2;
    pComponentPrivate->sPortParam->nStartPortNumber = 0x0;

    /* Malloc and Set pPriorityMgmt defaults */
    OMX_G729MALLOC_STRUCT(pComponentPrivate->sPriorityMgmt, OMX_PRIORITYMGMTTYPE);
    OMX_G729CONF_INIT_STRUCT(pComponentPrivate->sPriorityMgmt, OMX_PRIORITYMGMTTYPE);

    /* Initialize sPriorityMgmt data structures to default values */
    pComponentPrivate->sPriorityMgmt->nGroupPriority = -1;
    pComponentPrivate->sPriorityMgmt->nGroupID = -1;

    OMX_G729MALLOC_STRUCT(g729_ip, OMX_AUDIO_PARAM_PCMMODETYPE);
    OMX_G729CONF_INIT_STRUCT(g729_ip, OMX_AUDIO_PARAM_PCMMODETYPE);
    pComponentPrivate->pcmParams = g729_ip;

    /* PCM format defaults */
    g729_ip->nPortIndex = G729ENC_INPUT_PORT;
    g729_ip->nChannels = 2; 
    g729_ip->eNumData= OMX_NumericalDataSigned; 
    g729_ip->nBitPerSample = 16;  
    g729_ip->nSamplingRate = 44100;          
    g729_ip->ePCMMode = OMX_AUDIO_PCMModeLinear; 

    OMX_G729MALLOC_STRUCT(g729_op, OMX_AUDIO_PARAM_G729TYPE);
    OMX_G729CONF_INIT_STRUCT(g729_op, OMX_AUDIO_PARAM_G729TYPE);
    pComponentPrivate->g729Params = g729_op;
    g729_op->nPortIndex = G729ENC_OUTPUT_PORT;

    /* malloc and initialize number of input buffers */
    OMX_G729MALLOC_STRUCT(pComponentPrivate->pInputBufferList, G729ENC_BUFFERLIST);
    pComponentPrivate->pInputBufferList->numBuffers = 0;

    /* malloc and initialize number of output buffers */
    OMX_G729MALLOC_STRUCT(pComponentPrivate->pOutputBufferList, G729ENC_BUFFERLIST);
    pComponentPrivate->pOutputBufferList->numBuffers = 0;

    for (i=0; i < G729ENC_MAX_NUM_OF_BUFS; i++)
    {
        pComponentPrivate->pOutputBufferList->pBufHdr[i] = NULL;
        pComponentPrivate->pInputBufferList->pBufHdr[i] = NULL;
        pComponentPrivate->arrTickCount[i] = 0;
        pComponentPrivate->arrTimestamp[i] = 0;
    }

    /* Set input port defaults */
    OMX_G729MALLOC_STRUCT(pPortDef_ip, OMX_PARAM_PORTDEFINITIONTYPE);
    OMX_G729CONF_INIT_STRUCT(pPortDef_ip, OMX_PARAM_PORTDEFINITIONTYPE);
    pComponentPrivate->pPortDef[G729ENC_INPUT_PORT] = pPortDef_ip;

    pPortDef_ip->nPortIndex                         = G729ENC_INPUT_PORT;
    pPortDef_ip->eDir                               = OMX_DirInput;
    pPortDef_ip->nBufferCountActual                 = G729ENC_NUM_INPUT_BUFFERS;
    pPortDef_ip->nBufferCountMin                    = G729ENC_NUM_INPUT_BUFFERS;
    pPortDef_ip->nBufferSize                        = G729ENC_INPUT_FRAME_SIZE;
    pPortDef_ip->bEnabled                           = OMX_TRUE;
    pPortDef_ip->bPopulated                         = OMX_FALSE;
    pPortDef_ip->eDomain                            = OMX_PortDomainAudio;
    pPortDef_ip->format.audio.eEncoding             = OMX_AUDIO_CodingPCM;
    pPortDef_ip->format.audio.cMIMEType             = NULL;
    pPortDef_ip->format.audio.pNativeRender         = NULL;
    pPortDef_ip->format.audio.bFlagErrorConcealment = OMX_FALSE;

    /* Set output port defaults */
    OMX_G729MALLOC_STRUCT(pPortDef_op, OMX_PARAM_PORTDEFINITIONTYPE);
    OMX_G729CONF_INIT_STRUCT(pPortDef_op, OMX_PARAM_PORTDEFINITIONTYPE);
    pComponentPrivate->pPortDef[G729ENC_OUTPUT_PORT] = pPortDef_op;

    pPortDef_op->nPortIndex                         = G729ENC_OUTPUT_PORT;
    pPortDef_op->eDir                               = OMX_DirOutput;
    pPortDef_op->nBufferCountMin                    = G729ENC_NUM_OUTPUT_BUFFERS;
    pPortDef_op->nBufferCountActual                 = G729ENC_NUM_OUTPUT_BUFFERS;
    pPortDef_op->nBufferSize                        = G729ENC_OUTPUT_FRAME_SIZE;
    pPortDef_op->bEnabled                           = OMX_TRUE;
    pPortDef_op->bPopulated                         = OMX_FALSE;
    pPortDef_op->eDomain                            = OMX_PortDomainAudio;
    pPortDef_op->format.audio.eEncoding             = OMX_AUDIO_CodingG729;
    pPortDef_op->format.audio.cMIMEType             = NULL;
    pPortDef_op->format.audio.pNativeRender         = NULL;
    pPortDef_op->format.audio.bFlagErrorConcealment = OMX_FALSE;

    OMX_G729MALLOC_STRUCT(pComponentPrivate->pCompPort[G729ENC_INPUT_PORT]->pPortFormat, OMX_AUDIO_PARAM_PORTFORMATTYPE);
    OMX_G729CONF_INIT_STRUCT(pComponentPrivate->pCompPort[G729ENC_INPUT_PORT]->pPortFormat, OMX_AUDIO_PARAM_PORTFORMATTYPE);
    /* Set input port format defaults */
    pInPortFormat = pComponentPrivate->pCompPort[G729ENC_INPUT_PORT]->pPortFormat;
    OMX_G729CONF_INIT_STRUCT(pInPortFormat, OMX_AUDIO_PARAM_PORTFORMATTYPE);
    pInPortFormat->nPortIndex         = G729ENC_INPUT_PORT;
    pInPortFormat->nIndex             = OMX_IndexParamAudioPcm;
    pInPortFormat->eEncoding          = OMX_AUDIO_CodingPCM;

    OMX_G729MALLOC_STRUCT(pComponentPrivate->pCompPort[G729ENC_OUTPUT_PORT]->pPortFormat, OMX_AUDIO_PARAM_PORTFORMATTYPE);
    OMX_G729CONF_INIT_STRUCT(pComponentPrivate->pCompPort[G729ENC_OUTPUT_PORT]->pPortFormat, OMX_AUDIO_PARAM_PORTFORMATTYPE);
    /* Set output port format defaults */
    pOutPortFormat = pComponentPrivate->pCompPort[G729ENC_OUTPUT_PORT]->pPortFormat;
    OMX_G729CONF_INIT_STRUCT(pOutPortFormat, OMX_AUDIO_PARAM_PORTFORMATTYPE);
    pOutPortFormat->nPortIndex         = G729ENC_OUTPUT_PORT;
    pOutPortFormat->nIndex             = OMX_IndexParamAudioG729;
    pOutPortFormat->eEncoding          = OMX_AUDIO_CodingG729;

    G729ENC_DPRINT("Setting dasf & mime & efr & acdn & g729 and MultiFrame modes to 0\n");
    pComponentPrivate->dasfMode = 0;
    pComponentPrivate->rtmx = 0;
    pComponentPrivate->mimeMode = 0;
    pComponentPrivate->acdnMode = 0;
    pComponentPrivate->efrMode  = 0;
    pComponentPrivate->g729Mode  = 0;
    pComponentPrivate->nMultiFrameMode = 0;
    pComponentPrivate->bPortDefsAllocated = OMX_TRUE;
    pComponentPrivate->bCompThreadStarted = 0;
    pComponentPrivate->bPlayCompleteFlag = 0;
    pComponentPrivate->pParams = NULL;
    pComponentPrivate->pAlgParam = NULL;
    pComponentPrivate->bInitParamsInitialized = 0;
    pComponentPrivate->pMarkBuf = NULL;
    pComponentPrivate->pMarkData = NULL;
    pComponentPrivate->nEmptyBufferDoneCount = 0;
    pComponentPrivate->nEmptyThisBufferCount = 0;
    pComponentPrivate->nFillBufferDoneCount = 0;
    pComponentPrivate->nFillThisBufferCount = 0;
    pComponentPrivate->strmAttr = NULL;
    pComponentPrivate->nDataWritesWhilePaused = 0;
    pComponentPrivate->bIdleCommandPending = 0;
    pComponentPrivate->bDisableCommandParam = 0;
    pComponentPrivate->bIsEOFSent = 0;
    pComponentPrivate->bBypassDSP = 0;
    pComponentPrivate->iHoldLen = 0;
    pComponentPrivate->iHoldBuffer = NULL;
    pComponentPrivate->pHoldBuffer = NULL;
    pComponentPrivate->nHoldLength = 0;
    pComponentPrivate->IpBufindex = 0;
    pComponentPrivate->OpBufindex = 0;
    pComponentPrivate->bDisableCommandParam = 0;
    pComponentPrivate->bEnableCommandParam = 0;
    pComponentPrivate->bDisableCommandPending = 0;
    pComponentPrivate->bEnableCommandPending = 0;
    pComponentPrivate->nUnhandledFillThisBuffers=0;
    pComponentPrivate->nUnhandledEmptyThisBuffers = 0;
    pComponentPrivate->bFlushOutputPortCommandPending = OMX_FALSE;
    pComponentPrivate->bFlushInputPortCommandPending = OMX_FALSE;
    pComponentPrivate->bPreempted = OMX_FALSE;

    pComponentPrivate->sDeviceString = malloc(100*sizeof(OMX_STRING));
    if (pComponentPrivate->sDeviceString == NULL) {
	G729ENC_EPRINT("OMX_ErrorInsufficientResources.\n");
	eError = OMX_ErrorInsufficientResources;
	goto EXIT;
    }

    strcpy((char*)pComponentPrivate->sDeviceString,"/eteedn:i0:o0/codec\0");
        
    
    for (i=0; i < G729ENC_MAX_NUM_OF_BUFS; i++)
    {
        pComponentPrivate->pInputBufHdrPending[i] = NULL;
        pComponentPrivate->pOutputBufHdrPending[i] = NULL;
    }
    pComponentPrivate->bJustReenabled = 0;
    pComponentPrivate->nInvalidFrameCount = 0;
    pComponentPrivate->nNumInputBufPending = 0;
    pComponentPrivate->nNumOutputBufPending = 0;
    pComponentPrivate->bDisableCommandPending = 0;
    pComponentPrivate->bNoIdleOnStop= OMX_FALSE;
    pComponentPrivate->bIdleCommandPending = OMX_FALSE;
    pComponentPrivate->nOutStandingFillDones = 0;
    pComponentPrivate->bDspStoppedWhileExecuting = OMX_FALSE;
    pComponentPrivate->nRuntimeInputBuffers=0;
    pComponentPrivate->nRuntimeOutputBuffers=0;
#ifndef UNDER_CE
    pthread_mutex_init(&pComponentPrivate->AlloBuf_mutex, NULL);
    pthread_cond_init (&pComponentPrivate->AlloBuf_threshold, NULL);
    pComponentPrivate->AlloBuf_waitingsignal = 0;
    pthread_mutex_init(&pComponentPrivate->InLoaded_mutex, NULL);
    pthread_cond_init (&pComponentPrivate->InLoaded_threshold, NULL);
    pComponentPrivate->InLoaded_readytoidle = 0;
    pthread_mutex_init(&pComponentPrivate->InIdle_mutex, NULL);
    pthread_cond_init (&pComponentPrivate->InIdle_threshold, NULL);
    pComponentPrivate->InIdle_goingtoloaded = 0;
#else
    OMX_CreateEvent(&(pComponentPrivate->AlloBuf_event));
    pComponentPrivate->AlloBuf_waitingsignal = 0;
    OMX_CreateEvent(&(pComponentPrivate->InLoaded_event));
    pComponentPrivate->InLoaded_readytoidle = 0;
    OMX_CreateEvent(&(pComponentPrivate->InIdle_event));
    pComponentPrivate->InIdle_goingtoloaded = 0;
#endif
    

#ifdef RESOURCE_MANAGER_ENABLED
    eError = RMProxy_NewInitalize();
    if (eError != OMX_ErrorNone)
    {
        G729ENC_EPRINT("From loading ResourceManagerProxy thread.\n");
        goto EXIT;
    }
#endif

    eError = G729ENC_StartComponentThread(pHandle);
    if (eError != OMX_ErrorNone)
    {
        G729ENC_EPRINT("From the Component.\n");
        goto EXIT;
    }

#ifdef DSP_RENDERING_ON
    if((pComponentPrivate->fdwrite=open(FIFO1,O_WRONLY))<0)
    {
        G729ENC_EPRINT("Failure to open WRITE pipe\n");
    }
    if((pComponentPrivate->fdread=open(FIFO2,O_RDONLY))<0)
    {
        G729ENC_EPRINT("Failure to open READ pipe\n");
    }
#endif

#ifdef __PERF_INSTRUMENTATION__
    PERF_ThreadCreated(pComponentPrivate->pPERF, pComponentPrivate->ComponentThread,
                       PERF_FOURCC('7','2','9','T'));
#endif
 EXIT:
    G729ENC_DPRINT("Exiting. Returning = 0x%x\n", eError);
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
    G729ENC_COMPONENT_PRIVATE *pComponentPrivate =
        (G729ENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;

    G729ENC_DPRINT("Entering\n");
    if (pCallBacks == NULL)
    {
        eError = OMX_ErrorBadParameter;
        G729ENC_EPRINT("Received empty callbacks from the application.\n");
        goto EXIT;
    }
    /*Copy the callbacks of the application to the component private*/
    memcpy (&(pComponentPrivate->cbInfo), pCallBacks, sizeof(OMX_CALLBACKTYPE));
    /*copy the application private data to component memory */
    pHandle->pApplicationPrivate = pAppData;
    pComponentPrivate->curState = OMX_StateLoaded;
#ifdef __PERF_INSTRUMENTATION__
    PERF_Boundary(pComponentPrivate->pPERFcomp,PERF_BoundaryComplete | PERF_BoundaryCleanup);
#endif
 EXIT:
    G729ENC_DPRINT("Exiting. Returning = 0x%x\n", eError);
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
    G729ENC_COMPONENT_PRIVATE *pComponentPrivate = (G729ENC_COMPONENT_PRIVATE *) pHandle->pComponentPrivate;

    G729ENC_DPRINT("Entering\n");
#ifdef _ERROR_PROPAGATION__
    if (pComponentPrivate->curState == OMX_StateInvalid)
    {
        eError = OMX_ErrorInvalidState;
        goto EXIT;
    }
#endif
    /* Copy component version structure */
    if(pComponentVersion != NULL && pComponentName != NULL)
    {
        strcpy(pComponentName, pComponentPrivate->cComponentName);
        memcpy(pComponentVersion, &(pComponentPrivate->ComponentVersion.s),
               sizeof(pComponentPrivate->ComponentVersion.s));
    }
    else
    {
        G729ENC_DPRINT("OMX_ErrorBadParameter from GetComponentVersion");
        eError = OMX_ErrorBadParameter;
    }
 EXIT:
    G729ENC_DPRINT("Exiting. Returning = 0x%x\n", eError);
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
    G729ENC_COMPONENT_PRIVATE *pCompPrivate =
        (G729ENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    int nRet = 0;

#ifdef _ERROR_PROPAGATION__
    if (pCompPrivate->curState == OMX_StateInvalid)
    {
        eError = OMX_ErrorInvalidState;
        goto EXIT;
    }
#else
    if(pCompPrivate->curState == OMX_StateInvalid)
    {
        eError = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }
#endif
#ifdef __PERF_INSTRUMENTATION__
    PERF_SendingCommand(pCompPrivate->pPERF,
                        Cmd,
                        (Cmd == OMX_CommandMarkBuffer) ? ((OMX_U32) pCmdData) : nParam,
                        PERF_ModuleComponent);
#endif
    G729ENC_DPRINT("Entering\n");
    switch(Cmd)
    {
    case OMX_CommandStateSet:
        G729ENC_DPRINT("OMX_CommandStateSet SendCommand\n");
        if (nParam == OMX_StateLoaded) {
            pCompPrivate->bLoadedCommandPending = OMX_TRUE;
        }
        if(pCompPrivate->curState == OMX_StateLoaded)
        {
            if((nParam == OMX_StateExecuting) || (nParam == OMX_StatePause))
            {
                pCompPrivate->cbInfo.EventHandler ( pHandle,
                                                    pHandle->pApplicationPrivate,
                                                    OMX_EventError,
                                                    OMX_ErrorIncorrectStateTransition,
                                                    0,
                                                    NULL);
                goto EXIT;
            }
            if(nParam == OMX_StateInvalid)
            {
                G729ENC_DPRINT("OMX_CommandStateSet SendCommand\n");
                pCompPrivate->curState = OMX_StateInvalid;
                pCompPrivate->cbInfo.EventHandler ( pHandle,
                                                    pHandle->pApplicationPrivate,
                                                    OMX_EventError,
                                                    OMX_ErrorInvalidState,
                                                    0,
                                                    NULL);
                goto EXIT;
            }
        }
        break;
    case OMX_CommandFlush:
        G729ENC_DPRINT("OMX_CommandFlush SendCommand\n");
        if(nParam > 1 && nParam != -1)
        {
            eError = OMX_ErrorBadPortIndex;
            G729ENC_EPRINT("OMX_ErrorBadPortIndex.\n");
            goto EXIT;
        }
        break;
    case OMX_CommandPortDisable:
        G729ENC_DPRINT("OMX_CommandPortDisable SendCommand\n");
        break;
    case OMX_CommandPortEnable:
        G729ENC_DPRINT("OMX_CommandPortEnable SendCommand\n");
        break;
    case OMX_CommandMarkBuffer:
        G729ENC_DPRINT("OMX_CommandMarkBuffer SendCommand\n");
        if (nParam > 0)
        {
            eError = OMX_ErrorBadPortIndex;
            G729ENC_EPRINT("OMX_ErrorBadPortIndex.\n");
            goto EXIT;
        }
        break;
    default:
        G729ENC_DPRINT("Command Received Default eError\n");
        pCompPrivate->cbInfo.EventHandler ( pHandle,
                                            pHandle->pApplicationPrivate,
                                            OMX_EventError,
                                            OMX_ErrorUndefined,
                                            0,
                                            "Invalid Command");
        break;
    }
    nRet = write (pCompPrivate->cmdPipe[1], &Cmd, sizeof(Cmd));
    if (nRet == -1)
    {
        eError = OMX_ErrorInsufficientResources;
        G729ENC_EPRINT("OMX_ErrorInsufficientResources.\n");
        goto EXIT;
    }
    if (Cmd == OMX_CommandMarkBuffer)
    {
        nRet = write(pCompPrivate->cmdDataPipe[1], &pCmdData, sizeof(OMX_PTR));
    }
    else
    {
        nRet = write(pCompPrivate->cmdDataPipe[1], &nParam, sizeof(OMX_U32));
    }
    if (nRet == -1)
    {
        eError = OMX_ErrorInsufficientResources;
        G729ENC_EPRINT("OMX_ErrorInsufficientResources.\n");
        goto EXIT;
    }
 EXIT:
    G729ENC_DPRINT("Exiting. Returning = 0x%x\n", eError);
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
    G729ENC_COMPONENT_PRIVATE  *pComponentPrivate = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE *pParameterStructure = NULL;
    pComponentPrivate = (G729ENC_COMPONENT_PRIVATE *)(((OMX_COMPONENTTYPE*)hComp)->pComponentPrivate);
    pParameterStructure = (OMX_PARAM_PORTDEFINITIONTYPE*)ComponentParameterStructure;

    G729ENC_DPRINT("Entering\n");
    if (pParameterStructure == NULL)
    {
        eError = OMX_ErrorBadParameter;
        G729ENC_EPRINT("OMX_ErrorBadParameter.\n");
        goto EXIT;
    }
#ifdef _ERROR_PROPAGATION__
    if (pComponentPrivate->curState == OMX_StateInvalid)
    {
        eError = OMX_ErrorInvalidState;
        goto EXIT;
    }
#else
    if(pComponentPrivate->curState == OMX_StateInvalid)
    {
        eError = OMX_ErrorIncorrectStateOperation;
        G729ENC_EPRINT("OMX_ErrorIncorrectStateOperation.\n");
        goto EXIT;
    }
#endif
    switch(nParamIndex)
    {
    case OMX_IndexParamAudioInit:
        G729ENC_DPRINT("case OMX_IndexParamAudioInit\n");
	if (pComponentPrivate->sPortParam == NULL) {
            eError = OMX_ErrorBadParameter;
	    break;
	}
        memcpy(ComponentParameterStructure, pComponentPrivate->sPortParam, 
               sizeof(OMX_PORT_PARAM_TYPE));
        break;
    case OMX_IndexParamPortDefinition:
        G729ENC_DPRINT("case OMX_IndexParamPortDefinition \n");
        if(((OMX_PARAM_PORTDEFINITIONTYPE *)(ComponentParameterStructure))->nPortIndex ==
           pComponentPrivate->pPortDef[G729ENC_INPUT_PORT]->nPortIndex)
        {
            memcpy(ComponentParameterStructure,
                   pComponentPrivate->pPortDef[G729ENC_INPUT_PORT],
                   sizeof(OMX_PARAM_PORTDEFINITIONTYPE)); 
        } 
        else if(((OMX_PARAM_PORTDEFINITIONTYPE *)(ComponentParameterStructure))->nPortIndex ==
                pComponentPrivate->pPortDef[G729ENC_OUTPUT_PORT]->nPortIndex)
        {
            memcpy(ComponentParameterStructure,
                   pComponentPrivate->pPortDef[G729ENC_OUTPUT_PORT],
                   sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
        } 
        else
        {
            eError = OMX_ErrorBadPortIndex;
            G729ENC_EPRINT("OMX_ErrorBadPortIndex.\n");
        }
        break;
    case OMX_IndexParamAudioPortFormat:
        G729ENC_DPRINT("case OMX_IndexParamAudioPortFormat\n");
        if(((OMX_AUDIO_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nPortIndex ==
           pComponentPrivate->pPortDef[G729ENC_INPUT_PORT]->nPortIndex)
        {
            if(((OMX_AUDIO_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nIndex >
               pComponentPrivate->pCompPort[G729ENC_INPUT_PORT]->pPortFormat->nPortIndex)
            {
                eError = OMX_ErrorNoMore;
            }
            else
            {
                memcpy(ComponentParameterStructure,
                       pComponentPrivate->pCompPort[G729ENC_INPUT_PORT]->pPortFormat,
                       sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
            }
        }
        else if(((OMX_AUDIO_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nPortIndex ==
                pComponentPrivate->pPortDef[G729ENC_OUTPUT_PORT]->nPortIndex)
        {
            if(((OMX_AUDIO_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nIndex >
               pComponentPrivate->pCompPort[G729ENC_OUTPUT_PORT]->pPortFormat->nPortIndex)
            {
                eError = OMX_ErrorNoMore;
            }
            else
            {
                memcpy(ComponentParameterStructure,
                       pComponentPrivate->pCompPort[G729ENC_OUTPUT_PORT]->pPortFormat,
                       sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
            }
        }
        else
        {
            eError = OMX_ErrorBadPortIndex;
            G729ENC_EPRINT("OMX_ErrorBadPortIndex.\n");
        }
        break;
    case OMX_IndexParamAudioG729:
        G729ENC_DPRINT("Case OMX_IndexParamAudioG729\n");
        if(((OMX_AUDIO_PARAM_G729TYPE *)(ComponentParameterStructure))->nPortIndex ==
           pComponentPrivate->g729Params->nPortIndex)
        {
            memcpy(ComponentParameterStructure,
                   pComponentPrivate->g729Params, sizeof(OMX_AUDIO_PARAM_G729TYPE));
        } 
        else
        {
            eError = OMX_ErrorBadPortIndex;
            G729ENC_EPRINT("OMX_ErrorBadPortIndex.\n");
        }
        break;

    case OMX_IndexParamAudioPcm:
        if(((OMX_AUDIO_PARAM_PCMMODETYPE *)(ComponentParameterStructure))->nPortIndex ==
           pComponentPrivate->pcmParams->nPortIndex) {

            memcpy(ComponentParameterStructure,
                   pComponentPrivate->pcmParams,
                   sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
        } 
        else {
            G729ENC_DPRINT("%d :: OMX_ErrorBadPortIndex from GetParameter \n",__LINE__);
            eError = OMX_ErrorBadPortIndex;
        }
        break;            
    case OMX_IndexParamPriorityMgmt:
	if (pComponentPrivate->sPriorityMgmt == NULL) {
            eError = OMX_ErrorBadParameter;
	    break;
	}
        G729ENC_DPRINT("Case OMX_IndexParamPriorityMgmt\n");
        memcpy(ComponentParameterStructure,
               pComponentPrivate->sPriorityMgmt,
               sizeof(OMX_PRIORITYMGMTTYPE));
        break;

    case OMX_IndexParamCompBufferSupplier:
        if(((OMX_PARAM_BUFFERSUPPLIERTYPE *)(ComponentParameterStructure))->nPortIndex == OMX_DirInput) {
            G729ENC_DPRINT(":: GetParameter OMX_IndexParamCompBufferSupplier \n");
            /*  memcpy(ComponentParameterStructure, pBufferSupplier, sizeof(OMX_PARAM_BUFFERSUPPLIERTYPE)); */
        }
        else
            if(((OMX_PARAM_BUFFERSUPPLIERTYPE *)(ComponentParameterStructure))->nPortIndex == OMX_DirOutput) {
                G729ENC_DPRINT(":: GetParameter OMX_IndexParamCompBufferSupplier \n");
                /*memcpy(ComponentParameterStructure, pBufferSupplier, sizeof(OMX_PARAM_BUFFERSUPPLIERTYPE)); */
            } 
            else {
                G729ENC_DPRINT(":: OMX_ErrorBadPortIndex from GetParameter");
                eError = OMX_ErrorBadPortIndex;
            }
        break;

    case OMX_IndexParamVideoInit:
        break;

    case OMX_IndexParamImageInit:
        break;

    case OMX_IndexParamOtherInit:
        break;  
        
    default:
        eError = OMX_ErrorUnsupportedIndex;
        G729ENC_EPRINT("OMX_ErrorUnsupportedIndex.\n");
        break;
    }
 EXIT:
    G729ENC_DPRINT("Exiting. Returning = 0x%x\n", eError);
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
    G729ENC_COMPONENT_PRIVATE  *pComponentPrivate = NULL;
    OMX_AUDIO_PARAM_PORTFORMATTYPE* pComponentParam = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE *pComponentParamPort = NULL;
    OMX_AUDIO_PARAM_G729TYPE *pCompG729Param = NULL;
    /*    OMX_PARAM_COMPONENTROLETYPE  *pRole = NULL; */
    OMX_AUDIO_PARAM_PCMMODETYPE* pPcmPort = NULL;
    OMX_PARAM_BUFFERSUPPLIERTYPE sBufferSupplier;       
    pComponentPrivate = (G729ENC_COMPONENT_PRIVATE *)(((OMX_COMPONENTTYPE*)hComp)->pComponentPrivate);

    G729ENC_DPRINT("Entering\n");
    if (pCompParam == NULL)
    {
        eError = OMX_ErrorBadParameter;
        G729ENC_EPRINT("OMX_ErrorBadParameter.\n");
        goto EXIT;
    }
    if (pComponentPrivate->curState != OMX_StateLoaded)
    {
        eError = OMX_ErrorIncorrectStateOperation;
        G729ENC_EPRINT("OMX_ErrorIncorrectStateOperation.\n");
        goto EXIT;
    }
#ifdef _ERROR_PROPAGATION__
    if (pComponentPrivate->curState == OMX_StateInvalid)
    {
        eError = OMX_ErrorInvalidState;
        goto EXIT;
    }
#endif
    switch(nParamIndex)
    {
    case OMX_IndexParamAudioPortFormat:
        G729ENC_DPRINT("SetParameter OMX_IndexParamAudioPortFormat\n");
        pComponentParam = (OMX_AUDIO_PARAM_PORTFORMATTYPE *)pCompParam;
        if ( pComponentParam->nPortIndex == pComponentPrivate->pCompPort[G729ENC_INPUT_PORT]->pPortFormat->nPortIndex )
        {
            memcpy(pComponentPrivate->pCompPort[G729ENC_INPUT_PORT]->pPortFormat,
                   pComponentParam, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
        }
        else if ( pComponentParam->nPortIndex == pComponentPrivate->pCompPort[G729ENC_OUTPUT_PORT]->pPortFormat->nPortIndex )
        {
            memcpy(pComponentPrivate->pCompPort[G729ENC_OUTPUT_PORT]->pPortFormat,
                   pComponentParam, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
        }
        else
        {
            eError = OMX_ErrorBadPortIndex;
            G729ENC_EPRINT("OMX_ErrorBadPortIndex.\n");
        }
        break;
    case OMX_IndexParamAudioG729:
        pCompG729Param = (OMX_AUDIO_PARAM_G729TYPE *)pCompParam;
        if (pCompG729Param->nPortIndex == G729ENC_OUTPUT_PORT)
        { 
            memcpy(((G729ENC_COMPONENT_PRIVATE *)
                    pHandle->pComponentPrivate)->g729Params,
                   pCompG729Param, sizeof(OMX_AUDIO_PARAM_G729TYPE));
        }
        else
        {
            eError = OMX_ErrorBadPortIndex;
            G729ENC_EPRINT("OMX_ErrorBadPortIndex.\n");
        }
        break;
    case OMX_IndexParamPortDefinition:
        pComponentParamPort = (OMX_PARAM_PORTDEFINITIONTYPE *)pCompParam;
        G729ENC_DPRINT("Case OMX_IndexParamPortDefinition\n");
        if(((OMX_PARAM_PORTDEFINITIONTYPE *)(pCompParam))->nPortIndex ==
           pComponentPrivate->pPortDef[G729ENC_INPUT_PORT]->nPortIndex)
        {
            G729ENC_DPRINT("Case OMX_IndexParamPortDefinition\n");
            memcpy(pComponentPrivate->pPortDef[G729ENC_INPUT_PORT], 
                   pCompParam, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
        }
        else if(((OMX_PARAM_PORTDEFINITIONTYPE *)(pCompParam))->nPortIndex ==
                pComponentPrivate->pPortDef[G729ENC_OUTPUT_PORT]->nPortIndex)
        {
            G729ENC_DPRINT("Case OMX_IndexParamPortDefinition\n");
            memcpy(pComponentPrivate->pPortDef[G729ENC_OUTPUT_PORT], 
                   pCompParam, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
        }
        else
        {
            eError = OMX_ErrorBadPortIndex;
            G729ENC_EPRINT("OMX_ErrorBadPortIndex.\n");
        }
        break;
    case OMX_IndexParamPriorityMgmt:
        if (pComponentPrivate->sPriorityMgmt == NULL) {
            eError = OMX_ErrorBadParameter;
	    break;
	}
        G729ENC_DPRINT("Case OMX_IndexParamPriorityMgmt\n");
        memcpy(pComponentPrivate->sPriorityMgmt,
               (OMX_PRIORITYMGMTTYPE*)pCompParam,
               sizeof(OMX_PRIORITYMGMTTYPE));
        break;
    case OMX_IndexParamAudioInit:
        if (pComponentPrivate->sPortParam == NULL) {
            eError = OMX_ErrorBadParameter;
	    break;
	}
        G729ENC_DPRINT("SetParameter OMX_IndexParamAudioInit\n");
        memcpy(pComponentPrivate->sPortParam,
               (OMX_PORT_PARAM_TYPE*)pCompParam,
               sizeof(OMX_PORT_PARAM_TYPE));
        break;

    case OMX_IndexParamStandardComponentRole:
        /*if (pCompParam) {
          pRole = (OMX_PARAM_COMPONENTROLETYPE *)pCompParam;
          memcpy(&(pComponentPrivate->componentRole), (void *)pRole, sizeof(OMX_PARAM_COMPONENTROLETYPE));
          } else {
          eError = OMX_ErrorBadParameter;
          }*/
        eError = OMX_ErrorBadParameter;
        break;

    case OMX_IndexParamAudioPcm:
        if(pCompParam){
        if (pComponentPrivate->pcmParams == NULL) {
            eError = OMX_ErrorBadParameter;
	    break;
	}
            pPcmPort= (OMX_AUDIO_PARAM_PCMMODETYPE *)pCompParam;
            memcpy(pComponentPrivate->pcmParams, pPcmPort, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
        }
        else{
            eError = OMX_ErrorBadParameter;
        }
        break;

    case OMX_IndexParamCompBufferSupplier:
        /*      eError = OMX_ErrorBadPortIndex; *//*remove for StdAudioDecoderTest, leave for other tests*/
        if(((OMX_PARAM_PORTDEFINITIONTYPE *)(pCompParam))->nPortIndex ==
           pComponentPrivate->pPortDef[OMX_DirInput]->nPortIndex) {
            G729ENC_DPRINT(":: SetParameter OMX_IndexParamCompBufferSupplier \n");
            sBufferSupplier.eBufferSupplier = OMX_BufferSupplyInput;
            memcpy(&sBufferSupplier, pCompParam, sizeof(OMX_PARAM_BUFFERSUPPLIERTYPE));                                                             
                                        
        }
        else if(((OMX_PARAM_PORTDEFINITIONTYPE *)(pCompParam))->nPortIndex ==
                pComponentPrivate->pPortDef[OMX_DirOutput]->nPortIndex) {
            G729ENC_DPRINT(":: SetParameter OMX_IndexParamCompBufferSupplier \n");
            sBufferSupplier.eBufferSupplier = OMX_BufferSupplyOutput;
            memcpy(&sBufferSupplier, pCompParam, sizeof(OMX_PARAM_BUFFERSUPPLIERTYPE));
        } 
        else {
            G729ENC_DPRINT(":: OMX_ErrorBadPortIndex from SetParameter");
            eError = OMX_ErrorBadPortIndex;
        }
        break;
    default:
        eError = OMX_ErrorUnsupportedIndex;
        G729ENC_EPRINT("OMX_ErrorUnsupportedIndex.\n");
        break;
    }
 EXIT:
    G729ENC_DPRINT("Exiting. Returning = 0x%x\n", eError);
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
    G729ENC_COMPONENT_PRIVATE *pComponentPrivate =
        (G729ENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    
    G729ENC_DPRINT("Entering\n");
#ifdef _ERROR_PROPAGATION__
    if (pComponentPrivate->curState == OMX_StateInvalid)
    {
        eError = OMX_ErrorInvalidState;
        goto EXIT;
    }
#endif
 EXIT:    
    G729ENC_DPRINT("Exiting. Returning = 0x%x\n", eError);
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
    G729ENC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    OMX_COMPONENTTYPE* pHandle = (OMX_COMPONENTTYPE*)hComp;
    TI_OMX_DSP_DEFINITION *pTiDspDefinition = NULL;
    TI_OMX_DSP_DEFINITION *configData = NULL;
    OMX_AUDIO_CONFIG_VOLUMETYPE *pGainStructure = NULL;
    TI_OMX_DATAPATH dataPath;
    OMX_S16 *customFlag = NULL;
    G729ENC_DPRINT("Entering\n");
    if (pHandle == NULL)
    {
        eError = OMX_ErrorBadParameter;
        G729ENC_EPRINT("Invalid HANDLE OMX_ErrorBadParameter.\n");
        goto EXIT;
    }

    pComponentPrivate = (G729ENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
#ifdef _ERROR_PROPAGATION__
    if (pComponentPrivate->curState == OMX_StateInvalid)
    {
        eError = OMX_ErrorInvalidState;
        goto EXIT;
    }
#endif

    switch (nConfigIndex)
    {
    case OMX_IndexCustomG729ENCModeConfig: 
        pTiDspDefinition = (TI_OMX_DSP_DEFINITION*)ComponentConfigStructure;
        memcpy(&(pComponentPrivate->tiOmxDspDefinition), pTiDspDefinition,
               sizeof(TI_OMX_DSP_DEFINITION));
        pComponentPrivate->dasfMode = pComponentPrivate->tiOmxDspDefinition.dasfMode;
        pComponentPrivate->acdnMode = pComponentPrivate->tiOmxDspDefinition.acousticMode;
        pComponentPrivate->streamID = pComponentPrivate->tiOmxDspDefinition.streamId;
        break;
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
            G729ENC_EPRINT("Fail to send command to audio manager.\n");
        }
        else
        {
            G729ENC_DPRINT("Ok to send command to audio manager\n");
        }
#endif
        break;
    case OMX_IndexCustomG729ENCHeaderInfoConfig:
        G729ENC_DPRINT("Case OMX_IndexCustomWbAmrEncHeaderInfoConfig\n");
        configData = (TI_OMX_DSP_DEFINITION*)ComponentConfigStructure;
        if (configData == NULL)
        {
            eError = OMX_ErrorBadParameter;
            G729ENC_EPRINT("OMX_ErrorBadParameter.\n");
            goto EXIT;
        }

        /*pComponentPrivate->amrMode = *customFlag;*/
        pComponentPrivate->acdnMode = configData->acousticMode;
        pComponentPrivate->dasfMode = configData->dasfMode;
        if (2 == pComponentPrivate->dasfMode)
        {
            pComponentPrivate->dasfMode--;
            pComponentPrivate->rtmx = 1; 
        }
        pComponentPrivate->streamID = configData->streamId;
            
        break;
            
    case  OMX_IndexCustomG729ENCDataPath:
        customFlag = (OMX_S16*)ComponentConfigStructure;
        if (customFlag == NULL) {
            eError = OMX_ErrorBadParameter;
            goto EXIT;
        }

        dataPath = *customFlag;

        switch(dataPath) {
        case DATAPATH_APPLICATION:
            OMX_MMMIXER_DATAPATH(pComponentPrivate->sDeviceString, RENDERTYPE_ENCODER, pComponentPrivate->streamID);
            /*                strcpy((char*)pComponentPrivate->sDeviceString,(char*)ETEEDN_STRING_ENCODER); */
            break;

        case DATAPATH_APPLICATION_RTMIXER:
            strcpy((char*)pComponentPrivate->sDeviceString,(char*)RTM_STRING_ENCODER);
            break;

        default:
            break;
                    
        }
        break;
            
    default:
        eError = OMX_ErrorUnsupportedIndex;
        G729ENC_EPRINT("OMX_ErrorUnsupportedIndex.\n");
        break;
    }
 EXIT:
    G729ENC_DPRINT("Exiting. Returning = 0x%x\n", eError);
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
    OMX_ERRORTYPE eError = OMX_ErrorUndefined;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    
    G729ENC_DPRINT("Entering GetState\n");
    if (!pState)
    {
        eError = OMX_ErrorBadParameter;
        G729ENC_EPRINT("OMX_ErrorBadParameter.\n");
        goto EXIT;
    }
    if (pHandle && pHandle->pComponentPrivate)
    {
        *pState =  ((G729ENC_COMPONENT_PRIVATE*)
                    pHandle->pComponentPrivate)->curState;
    }
    else
    {
        *pState = OMX_StateLoaded;
    }
    eError = OMX_ErrorNone;
 EXIT:
    G729ENC_DPRINT("Exiting. Returning = 0x%x\n", eError);
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
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef = NULL;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    G729ENC_COMPONENT_PRIVATE *pComponentPrivate =
        (G729ENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    pPortDef = ((G729ENC_COMPONENT_PRIVATE*)pComponentPrivate)->pPortDef[G729ENC_INPUT_PORT];
#ifdef _ERROR_PROPAGATION__
    if (pComponentPrivate->curState == OMX_StateInvalid)
    {
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

    G729ENC_DPRINT("Entering EmptyThisBuffer\n");
    if (pBuffer == NULL)
    {
        eError = OMX_ErrorBadParameter;
        G729ENC_EPRINT("OMX_ErrorBadParameter.\n");
        goto EXIT;
    }
    if (pBuffer->nSize != sizeof(OMX_BUFFERHEADERTYPE))
    {
        eError = OMX_ErrorBadParameter;
        G729ENC_EPRINT("OMX_ErrorBadParameter.\n");
        goto EXIT;
    }
    if (!pPortDef->bEnabled)
    {
        eError  = OMX_ErrorIncorrectStateOperation;
        G729ENC_EPRINT("OMX_ErrorIncorrectStateOperation.\n");
        goto EXIT;
    }
    if (pBuffer->nVersion.nVersion != pComponentPrivate->nVersion)
    {
        eError = OMX_ErrorVersionMismatch;
        G729ENC_EPRINT("OMX_ErrorVersionMismatch.\n");
        goto EXIT;
    }
    if (pBuffer->nInputPortIndex != G729ENC_INPUT_PORT)
    {
        eError  = OMX_ErrorBadPortIndex;
        G729ENC_EPRINT("OMX_ErrorBadPortIndex.\n");
        goto EXIT;
    }
    if (pComponentPrivate->curState != OMX_StateExecuting &&
        pComponentPrivate->curState != OMX_StatePause)
    {
        eError= OMX_ErrorIncorrectStateOperation;
        G729ENC_EPRINT("OMX_ErrorIncorrectStateOperation.\n");
        goto EXIT;
    }
    G729ENC_DPRINT("Comp Sending Filled ip buff = %p to CompThread\n", pBuffer);

    pComponentPrivate->nUnhandledEmptyThisBuffers++;
    pComponentPrivate->app_nBuf--;
    pComponentPrivate->pMarkData = pBuffer->pMarkData;
    pComponentPrivate->hMarkTargetComponent = pBuffer->hMarkTargetComponent;
    ret = write (pComponentPrivate->dataPipe[1], &pBuffer, sizeof(OMX_BUFFERHEADERTYPE*));
    if (ret == -1)
    {
        eError = OMX_ErrorHardware;
        G729ENC_EPRINT("in Writing to the Data pipe.\n");
        goto EXIT;
    }
    pComponentPrivate->nEmptyThisBufferCount++;
 EXIT:
    G729ENC_DPRINT("Exiting. Returning = 0x%x\n", eError);
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
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef = NULL;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    G729ENC_COMPONENT_PRIVATE *pComponentPrivate =
        (G729ENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    pPortDef = ((G729ENC_COMPONENT_PRIVATE*)pComponentPrivate)->pPortDef[G729ENC_OUTPUT_PORT];

#ifdef _ERROR_PROPAGATION__
    if (pComponentPrivate->curState == OMX_StateInvalid)
    {
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
    G729ENC_DPRINT("Entering\n");
    G729ENC_DPRINT("Comp Sending Emptied op buff = %p to CompThread\n", pBuffer);
    if (pBuffer == NULL)
    {
        eError = OMX_ErrorBadParameter;
        G729ENC_EPRINT("OMX_ErrorBadParameter.\n");
        goto EXIT;
    }
    if (pBuffer->nSize != sizeof(OMX_BUFFERHEADERTYPE))
    {
        eError = OMX_ErrorBadParameter;
        G729ENC_EPRINT("OMX_ErrorBadParameter.\n");
        goto EXIT;
    }
    if (!pPortDef->bEnabled)
    {
        eError  = OMX_ErrorIncorrectStateOperation;
        G729ENC_EPRINT("OMX_ErrorIncorrectStateOperation.\n");
        goto EXIT;
    }
    if (pBuffer->nVersion.nVersion != pComponentPrivate->nVersion)
    {
        eError = OMX_ErrorVersionMismatch;
        G729ENC_EPRINT("OMX_ErrorVersionMismatch.\n");
        goto EXIT;
    }
    if (pBuffer->nOutputPortIndex != G729ENC_OUTPUT_PORT)
    {
        eError  = OMX_ErrorBadPortIndex;
        G729ENC_EPRINT("OMX_ErrorBadPortIndex.\n");
        goto EXIT;
    }
    if(pComponentPrivate->curState != OMX_StateExecuting &&
       pComponentPrivate->curState != OMX_StatePause)
    {
        eError = OMX_ErrorIncorrectStateOperation;
        G729ENC_EPRINT("OMX_ErrorIncorrectStateOperation.\n");
        goto EXIT;
    }
    pBuffer->nFilledLen = 0;
    /*Filling the Output buffer with zero */
    memset(pBuffer->pBuffer, 0, pBuffer->nAllocLen);
    pComponentPrivate->app_nBuf--;
    if(pComponentPrivate->pMarkBuf != NULL)
    {
        pBuffer->hMarkTargetComponent = pComponentPrivate->pMarkBuf->hMarkTargetComponent;
        pBuffer->pMarkData = pComponentPrivate->pMarkBuf->pMarkData;
        pComponentPrivate->pMarkBuf = NULL;
    }
    if (pComponentPrivate->pMarkData != NULL)
    {
        pBuffer->hMarkTargetComponent = pComponentPrivate->hMarkTargetComponent;
        pBuffer->pMarkData = pComponentPrivate->pMarkData;
        pComponentPrivate->pMarkData = NULL;
    }
    if (pBuffer->nFlags != OMX_BUFFERFLAG_EOS){
        pComponentPrivate->nUnhandledFillThisBuffers++;
    }
    ret = write (pComponentPrivate->dataPipe[1], &pBuffer,
                 sizeof (OMX_BUFFERHEADERTYPE*));

    if (ret == -1) 
    {
        eError = OMX_ErrorHardware;
        G729ENC_EPRINT("in Writing to the Data pipe.\n");
        goto EXIT;
    }

    pComponentPrivate->nFillThisBufferCount++;
 EXIT:
    G729ENC_DPRINT("Exiting. Returning = 0x%x\n", eError);
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
    G729ENC_COMPONENT_PRIVATE *pComponentPrivate =
        (G729ENC_COMPONENT_PRIVATE *)pComponent->pComponentPrivate;

#ifdef __PERF_INSTRUMENTATION__
    PERF_Boundary(pComponentPrivate->pPERF,
                  PERF_BoundaryStart | PERF_BoundaryCleanup);
#endif

#ifdef DSP_RENDERING_ON
    close(pComponentPrivate->fdwrite);
    close(pComponentPrivate->fdread);
#endif
    
#ifdef RESOURCE_MANAGER_ENABLED 
    eError = RMProxy_NewSendCommand(pHandle, RMProxy_FreeResource, OMX_G729_Encoder_COMPONENT, 0, 3456,NULL);
    if (eError != OMX_ErrorNone) {
        G729ENC_EPRINT ("%d ::OMX_G729Encoder.c :: Error returned from destroy ResourceManagerProxy thread\n",
                        __LINE__);                      
    }
    eError = RMProxy_Deinitalize();
    if (eError != OMX_ErrorNone)
    {
        G729ENC_EPRINT("from destroy ResourceManagerProxy thread.\n");
    }
#endif    
    pComponentPrivate->bIsThreadstop = 1;
    eError = G729ENC_StopComponentThread(pHandle);
    if (eError != OMX_ErrorNone)
    {
        G729ENC_EPRINT("from G729ENC_StopComponentThread.\n");
        goto EXIT;
    }
    /* Wait for thread to exit so we can get the status into "eError" */
    /* close the pipe handles */
    eError = G729ENC_FreeCompResources(pHandle);
    if (eError != OMX_ErrorNone)
    {
        G729ENC_EPRINT("from G729ENC_FreeCompResources.\n");
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

#ifdef __PERF_INSTRUMENTATION__
    PERF_Boundary(pComponentPrivate->pPERF,
                  PERF_BoundaryComplete | PERF_BoundaryCleanup);
    PERF_Done(pComponentPrivate->pPERF);
#endif    

    if (pComponentPrivate->sDeviceString != NULL) {
        free(pComponentPrivate->sDeviceString);
    }

    OMX_G729MEMFREE_STRUCT(pComponentPrivate);
 EXIT:
    G729ENC_DPRINT("Exiting ComponentDeInit. Returning = 0x%x\n", eError);
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

    G729ENC_DPRINT("Entering\n");
    eError = OMX_ErrorNotImplemented;
    G729ENC_EPRINT("OMX_ErrorNotImplemented.\n");
    G729ENC_DPRINT("Exiting. Returning = 0x%x\n", eError);
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
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef = NULL;
    G729ENC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_BUFFERHEADERTYPE *pBufferHeader = NULL;

    pComponentPrivate = (G729ENC_COMPONENT_PRIVATE *)
        (((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);

    pPortDef = ((G729ENC_COMPONENT_PRIVATE*)pComponentPrivate)->pPortDef[nPortIndex];
#ifdef _ERROR_PROPAGATION__
    if (pComponentPrivate->curState == OMX_StateInvalid)
    {
        eError = OMX_ErrorInvalidState;
        goto EXIT;
    }
#endif    
    G729ENC_DPRINT("Entering AllocateBuffer\n");
    G729ENC_DPRINT("pPortDef = %p\n", pPortDef);
    G729ENC_DPRINT("pPortDef->bEnabled = %d\n", pPortDef->bEnabled);
    if(!(pPortDef->bEnabled))
    {
        pComponentPrivate->AlloBuf_waitingsignal = 1;  
#ifndef UNDER_CE
        pthread_mutex_lock(&pComponentPrivate->AlloBuf_mutex); 
        pthread_cond_wait(&pComponentPrivate->AlloBuf_threshold,
                          &pComponentPrivate->AlloBuf_mutex);
        pthread_mutex_unlock(&pComponentPrivate->AlloBuf_mutex);
#else
        OMX_WaitForEvent(&(pComponentPrivate->AlloBuf_event));
#endif
    }
    OMX_G729MALLOC_STRUCT(pBufferHeader, OMX_BUFFERHEADERTYPE);
    pBufferHeader->pBuffer = (OMX_U8 *)malloc(nSizeBytes + 256);
    G729ENC_MEMPRINT("%d :: [ALLOC]  %p\n",__LINE__,pBufferHeader->pBuffer);
    if (pBufferHeader->pBuffer == NULL)
    {
        /* Free previously allocated memory before bailing */
        if (pBufferHeader)
        {
            free(pBufferHeader);
            pBufferHeader = NULL;
        }
        eError = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    pBufferHeader->pBuffer += 128;
    if (nPortIndex == G729ENC_INPUT_PORT)
    {
        pBufferHeader->nInputPortIndex = nPortIndex;
        pBufferHeader->nOutputPortIndex = -1;
        pComponentPrivate->pInputBufferList->pBufHdr[pComponentPrivate->pInputBufferList->numBuffers] = pBufferHeader;
        pComponentPrivate->pInputBufferList->bBufferPending[pComponentPrivate->pInputBufferList->numBuffers] = 0;
        pComponentPrivate->pInputBufferList->bufferOwner[pComponentPrivate->pInputBufferList->numBuffers++] = 1;
        if (pComponentPrivate->pInputBufferList->numBuffers == pPortDef->nBufferCountActual)
        {
            pPortDef->bPopulated = OMX_TRUE;
            G729ENC_DPRINT("pPortDef->bPopulated = %d\n", pPortDef->bPopulated);
        }
    }
    else if (nPortIndex == G729ENC_OUTPUT_PORT)
    {
        pBufferHeader->nInputPortIndex = -1;
        pBufferHeader->nOutputPortIndex = nPortIndex;
        pComponentPrivate->pOutputBufferList->pBufHdr[pComponentPrivate->pOutputBufferList->numBuffers] = pBufferHeader;
        pComponentPrivate->pOutputBufferList->bBufferPending[pComponentPrivate->pOutputBufferList->numBuffers] = 0;
        pComponentPrivate->pOutputBufferList->bufferOwner[pComponentPrivate->pOutputBufferList->numBuffers++] = 1;
        if (pComponentPrivate->pOutputBufferList->numBuffers == pPortDef->nBufferCountActual)
        {
            pPortDef->bPopulated = OMX_TRUE;
            G729ENC_DPRINT("pPortDef->bPopulated = %d\n", pPortDef->bPopulated);
        }
    }
    else
    {
        eError = OMX_ErrorBadPortIndex;
        G729ENC_EPRINT("About to return OMX_ErrorBadPortIndex\n");
        goto EXIT;
    }
    /* Removing sleep() calls. Input buffer enabled and populated as well as output buffer. */
    if((pComponentPrivate->pPortDef[G729ENC_OUTPUT_PORT]->bPopulated == pComponentPrivate->pPortDef[G729ENC_OUTPUT_PORT]->bEnabled)&&
       (pComponentPrivate->pPortDef[G729ENC_INPUT_PORT]->bPopulated == pComponentPrivate->pPortDef[G729ENC_INPUT_PORT]->bEnabled) &&
       (pComponentPrivate->InLoaded_readytoidle))
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
    pBufferHeader->nVersion.s.nVersionMajor = G729ENC_MAJOR_VER;
    pBufferHeader->nVersion.s.nVersionMinor = G729ENC_MINOR_VER;
    pBufferHeader->nVersion.s.nRevision = 0;
    pBufferHeader->nVersion.s.nStep = 0;
    pComponentPrivate->nVersion = pBufferHeader->nVersion.nVersion;
    pBufferHeader->nSize = sizeof(OMX_BUFFERHEADERTYPE);
    *pBuffer = pBufferHeader;

    if (pComponentPrivate->bEnableCommandPending && pPortDef->bPopulated) {
        SendCommand (pComponentPrivate->pHandle,
                     OMX_CommandPortEnable,
                     pComponentPrivate->bEnableCommandParam,NULL);
    }

#ifdef __PERF_INSTRUMENTATION__
    PERF_ReceivedBuffer(pComponentPrivate->pPERF,
                        (*pBuffer)->pBuffer, nSizeBytes,
                        PERF_ModuleMemory);
#endif     
 EXIT:
    G729ENC_DPRINT("Exiting AllocateBuffer. Returning = 0x%x\n", eError);
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
    G729ENC_COMPONENT_PRIVATE * pComponentPrivate = NULL;
    OMX_BUFFERHEADERTYPE* buff = NULL;
    OMX_U8* tempBuff = NULL;
    int i = 0;
    int inputIndex = -1;
    int outputIndex = -1;
    OMX_COMPONENTTYPE *pHandle = NULL;
    
    pComponentPrivate = (G729ENC_COMPONENT_PRIVATE *)
        (((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);
    pHandle = (OMX_COMPONENTTYPE *) pComponentPrivate->pHandle;

    G729ENC_DPRINT("Entering FreeBuffer\n");
    if (nPortIndex == G729ENC_INPUT_PORT){
        for (i=0; i < G729ENC_MAX_NUM_OF_BUFS; i++)
        {
            buff = pComponentPrivate->pInputBufferList->pBufHdr[i];
            if (buff == pBuffer)
            {
                G729ENC_DPRINT("Found matching input buffer\n");
                G729ENC_DPRINT("buff = %p\n", buff);
                G729ENC_DPRINT("pBuffer = %p\n", pBuffer);
                inputIndex = i;
                break;
            }
            else
            {
                G729ENC_DPRINT("This is not a match\n");
                G729ENC_DPRINT("buff = %p\n", buff);
                G729ENC_DPRINT("pBuffer = %p\n", pBuffer);
            }
        }
    }
    else if (nPortIndex == G729ENC_OUTPUT_PORT){
        for (i=0; i < G729ENC_MAX_NUM_OF_BUFS; i++)
        {
            buff = pComponentPrivate->pOutputBufferList->pBufHdr[i];
            if (buff == pBuffer)
            {
                G729ENC_DPRINT("Found matching output buffer\n");
                G729ENC_DPRINT("buff = %p\n", buff);
                G729ENC_DPRINT("pBuffer = %p\n", pBuffer);
                outputIndex = i;
                break;
            }
            else
            {
                G729ENC_DPRINT("This is not a match\n");
                G729ENC_DPRINT("buff = %p\n", buff);
                G729ENC_DPRINT("pBuffer = %p\n", pBuffer);
            }
        }
    }
    if (inputIndex != -1)
    {
        if (pComponentPrivate->pInputBufferList->bufferOwner[inputIndex] == 1)
        {
            tempBuff = pComponentPrivate->pInputBufferList->pBufHdr[inputIndex]->pBuffer;
            if (tempBuff != 0)
            {
                tempBuff -= 128;
            }
            OMX_G729MEMFREE_STRUCT(tempBuff);
        }
#ifdef __PERF_INSTRUMENTATION__
        PERF_SendingBuffer(pComponentPrivate->pPERF,
                           pComponentPrivate->pInputBufferList->pBufHdr[inputIndex]->pBuffer, 
                           pComponentPrivate->pInputBufferList->pBufHdr[inputIndex]->nAllocLen,
                           PERF_ModuleMemory );
#endif
        OMX_G729MEMFREE_STRUCT(pComponentPrivate->pInputBufferList->pBufHdr[inputIndex]);
        pComponentPrivate->pInputBufferList->numBuffers--;
        if (pComponentPrivate->pInputBufferList->numBuffers <
            pComponentPrivate->pPortDef[G729ENC_INPUT_PORT]->nBufferCountActual)
        {
            pComponentPrivate->pPortDef[G729ENC_INPUT_PORT]->bPopulated = OMX_FALSE;
        }
        if(pComponentPrivate->pPortDef[G729ENC_INPUT_PORT]->bEnabled &&
           pComponentPrivate->bLoadedCommandPending == OMX_FALSE &&
           (pComponentPrivate->curState == OMX_StateIdle ||
            pComponentPrivate->curState == OMX_StateExecuting ||
            pComponentPrivate->curState == OMX_StatePause))
        {
            pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                    pHandle->pApplicationPrivate,
                                                    OMX_EventError,
                                                    OMX_ErrorPortUnpopulated,
                                                    nPortIndex,
                                                    NULL);
        }
    }
    else if (outputIndex != -1)
    {
        if (pComponentPrivate->pOutputBufferList->bufferOwner[outputIndex] == 1) {
            tempBuff = pComponentPrivate->pOutputBufferList->pBufHdr[outputIndex]->pBuffer;
            if (tempBuff != 0)
            {
                tempBuff -= 128;
            }
            OMX_G729MEMFREE_STRUCT(tempBuff);
        }
#ifdef __PERF_INSTRUMENTATION__
        PERF_SendingBuffer(pComponentPrivate->pPERF,
                           pComponentPrivate->pOutputBufferList->pBufHdr[outputIndex]->pBuffer, 
                           pComponentPrivate->pOutputBufferList->pBufHdr[outputIndex]->nAllocLen,
                           PERF_ModuleMemory );
#endif
        OMX_G729MEMFREE_STRUCT(pComponentPrivate->pOutputBufferList->pBufHdr[outputIndex]);

        pComponentPrivate->pOutputBufferList->numBuffers--;
        if (pComponentPrivate->pOutputBufferList->numBuffers <
            pComponentPrivate->pPortDef[G729ENC_OUTPUT_PORT]->nBufferCountActual)
        {
            pComponentPrivate->pPortDef[G729ENC_OUTPUT_PORT]->bPopulated = OMX_FALSE;
        }
        if(pComponentPrivate->pPortDef[G729ENC_OUTPUT_PORT]->bEnabled &&
           pComponentPrivate->bLoadedCommandPending == OMX_FALSE &&
           (pComponentPrivate->curState == OMX_StateIdle ||
            pComponentPrivate->curState == OMX_StateExecuting ||
            pComponentPrivate->curState == OMX_StatePause))
        {
            pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                    pHandle->pApplicationPrivate,
                                                    OMX_EventError,
                                                    OMX_ErrorPortUnpopulated,
                                                    nPortIndex,
                                                    NULL);
        }
    }
    else
    {
        eError = OMX_ErrorBadParameter;
        G729ENC_EPRINT("OMX_ErrorBadParameter.\n");
    }
    if ((!pComponentPrivate->pInputBufferList->numBuffers &&
         !pComponentPrivate->pOutputBufferList->numBuffers) &&
        pComponentPrivate->InIdle_goingtoloaded)
    {
        pComponentPrivate->InIdle_goingtoloaded = 0;
#ifndef UNDER_CE
        pthread_mutex_lock(&pComponentPrivate->InIdle_mutex);
        pthread_cond_signal(&pComponentPrivate->InIdle_threshold);
        pthread_mutex_unlock(&pComponentPrivate->InIdle_mutex);
#else
        OMX_SignalEvent(&(pComponentPrivate->InIdle_event));
#endif
    }
    if (pComponentPrivate->bDisableCommandPending && (pComponentPrivate->pInputBufferList->numBuffers + 
                                                      pComponentPrivate->pOutputBufferList->numBuffers == 0)) {
        SendCommand (pComponentPrivate->pHandle, OMX_CommandPortDisable,
                     pComponentPrivate->bDisableCommandParam,NULL);
    }
    G729ENC_DPRINT("Exiting FreeBuffer Returning = 0x%x\n", eError);
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
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef = NULL;
    G729ENC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_BUFFERHEADERTYPE *pBufferHeader = NULL;
    
    pComponentPrivate = (G729ENC_COMPONENT_PRIVATE *)
        (((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);

#ifdef _ERROR_PROPAGATION__
    if (pComponentPrivate->curState == OMX_StateInvalid)
    {
        eError = OMX_ErrorInvalidState;
        goto EXIT;
    }
#endif
    pPortDef = ((G729ENC_COMPONENT_PRIVATE*)pComponentPrivate)->pPortDef[nPortIndex];
    G729ENC_DPRINT("Entering\n");
    G729ENC_DPRINT("pPortDef->bPopulated = %d \n", pPortDef->bPopulated);
    if(!pPortDef->bEnabled)
    {
        eError = OMX_ErrorIncorrectStateOperation;
        G729ENC_EPRINT("OMX_ErrorIncorrectStateOperation.\n");
        goto EXIT;
    }
    if(nSizeBytes != pPortDef->nBufferSize || pPortDef->bPopulated)
    {
        eError = OMX_ErrorBadParameter;
        G729ENC_EPRINT("OMX_ErrorBadParameter.\n");
        goto EXIT;
    }
    
    OMX_G729MALLOC_STRUCT(pBufferHeader, OMX_BUFFERHEADERTYPE);
    if (nPortIndex == G729ENC_OUTPUT_PORT)
    {
        pBufferHeader->nInputPortIndex = -1;
        pBufferHeader->nOutputPortIndex = nPortIndex;
        pComponentPrivate->pOutputBufferList->pBufHdr[pComponentPrivate->pOutputBufferList->numBuffers] = pBufferHeader;
        pComponentPrivate->pOutputBufferList->bBufferPending[pComponentPrivate->pOutputBufferList->numBuffers] = 0;
        pComponentPrivate->pOutputBufferList->bufferOwner[pComponentPrivate->pOutputBufferList->numBuffers++] = 0;
        if (pComponentPrivate->pOutputBufferList->numBuffers == pPortDef->nBufferCountActual)
        {
            pPortDef->bPopulated = OMX_TRUE;
        }
    }
    else
    {
        pBufferHeader->nInputPortIndex = nPortIndex;
        pBufferHeader->nOutputPortIndex = -1;
        pComponentPrivate->pInputBufferList->pBufHdr[pComponentPrivate->pInputBufferList->numBuffers] = pBufferHeader;
        pComponentPrivate->pInputBufferList->bBufferPending[pComponentPrivate->pInputBufferList->numBuffers] = 0;
        pComponentPrivate->pInputBufferList->bufferOwner[pComponentPrivate->pInputBufferList->numBuffers++] = 0;
        if (pComponentPrivate->pInputBufferList->numBuffers == pPortDef->nBufferCountActual)
        {
            pPortDef->bPopulated = OMX_TRUE;
        }
    }
    if((pComponentPrivate->pPortDef[G729ENC_OUTPUT_PORT]->bPopulated == pComponentPrivate->pPortDef[G729ENC_OUTPUT_PORT]->bEnabled)&&
       (pComponentPrivate->pPortDef[G729ENC_INPUT_PORT]->bPopulated == pComponentPrivate->pPortDef[G729ENC_INPUT_PORT]->bEnabled) &&
       (pComponentPrivate->InLoaded_readytoidle))
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
    pBufferHeader->nVersion.s.nVersionMajor = G729ENC_MAJOR_VER;
    pBufferHeader->nVersion.s.nVersionMinor = G729ENC_MINOR_VER;
    pBufferHeader->nVersion.s.nRevision = 0;
    pBufferHeader->nVersion.s.nStep = 0;
    pComponentPrivate->nVersion = pBufferHeader->nVersion.nVersion;
    pBufferHeader->pBuffer = pBuffer;
    pBufferHeader->nSize = sizeof(OMX_BUFFERHEADERTYPE);
    *ppBufferHdr = pBufferHeader;
    
    if (pComponentPrivate->bEnableCommandPending){
        SendCommand (pComponentPrivate->pHandle,
                     OMX_CommandPortEnable,
                     pComponentPrivate->bEnableCommandParam,NULL);
    }
#ifdef __PERF_INSTRUMENTATION__
    PERF_ReceivedBuffer(pComponentPrivate->pPERF,
                        pBuffer, nSizeBytes,
                        PERF_ModuleHLMM);
#endif
 EXIT:
    G729ENC_DPRINT("Exiting. Returning = 0x%x\n", eError);
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

    G729ENC_DPRINT("Entering\n");
    if (!(strcmp(cParameterName,"OMX.TI.index.config.tispecific")))
    {
        *pIndexType = OMX_IndexCustomG729ENCModeConfig;
        G729ENC_DPRINT("OMX_IndexCustomG729ENCModeConfig\n");
    }
    else if(!(strcmp(cParameterName,"OMX.TI.index.config.g729headerinfo")))
    {
        *pIndexType = OMX_IndexCustomG729ENCHeaderInfoConfig;
    }
    else if(!(strcmp(cParameterName,"OMX.TI.index.config.G729streamIDinfo")))
    {
        *pIndexType = OMX_IndexCustomG729ENCStreamIDConfig;
    }
    else if(!(strcmp(cParameterName,"OMX.TI.index.config.g729.datapath"))) 
    {
        *pIndexType = OMX_IndexCustomG729ENCDataPath;
    }
    
    else
    {
        eError = OMX_ErrorBadParameter;
    }
    G729ENC_DPRINT("Exiting\n");
    return eError;
}


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

    OMX_ERRORTYPE eError = OMX_ErrorNone;

    /* This is a non standard component and does not support roles */
    eError = OMX_ErrorNotImplemented;

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

    if(event == NULL)
    {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    event->event  = CreateEvent(NULL, TRUE, FALSE, NULL);
    if(event->event == NULL)
    {
        ret = (int)GetLastError();
    }
 EXIT:
    return ret;
}

int OMX_SignalEvent(OMX_Event *event){
    int ret = OMX_ErrorNone;

    if(event == NULL)
    {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }     
    SetEvent(event->event);
    ret = (int)GetLastError();
 EXIT:
    return ret;
}

int OMX_WaitForEvent(OMX_Event *event)
{
    int ret = OMX_ErrorNone;

    if(event == NULL)
    {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }     
    WaitForSingleObject(event->event, INFINITE);
    ret = (int)GetLastError();
 EXIT:
    return ret;
}

int OMX_DestroyEvent(OMX_Event *event)
{
    int ret = OMX_ErrorNone;

    if(event == NULL)
    {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }  
    CloseHandle(event->event);
 EXIT:    
    return ret;
}
#endif

void G729ENC_eprint(int iLineNum, const char *szFunctionName, const char *strFormat, ...)
{
#ifndef UNDER_CE 
    va_list list;

    fprintf(stdout, "ERROR::%s():%d: ", szFunctionName, iLineNum);
    fprintf(stdout, "%s", DBG_TEXT_WHITE);
    va_start(list, strFormat);
    vfprintf(stdout, strFormat, list);
    va_end(list);
#endif
}

void G729ENC_Log(const char *szFileName, int iLineNum, const char *szFunctionName, const char *strFormat, ...)
{
#ifndef UNDER_CE 
    va_list list;
        
    if (szFileName ==(const char *) "OMX_G729Encoder.c")
    {
        fprintf(stdout, "%s", DBG_TEXT_GREEN);
    }
    else if (szFileName == (const char *)"OMX_G729Enc_ComponentThread.c")
    {
        fprintf(stdout, "%s", DBG_TEXT_YELLOW);
    }
    else if (szFileName == (const char *)"OMX_G729Enc_Utils.c")
    {
        fprintf(stdout, "%s", DBG_TEXT_CYAN);
    }
    else
    {
        fprintf(stdout, "%s", DBG_TEXT_WHITE);
    }
    fprintf(stdout, "%s::", szFileName);
    fprintf(stdout, "%s", DBG_TEXT_WHITE);
    fprintf(stdout, "%s():%d: ", szFunctionName, iLineNum);
    fprintf(stdout, "%s", DBG_TEXT_WHITE);
    va_start(list, strFormat);
    vfprintf(stdout, strFormat, list);
    va_end(list);
#endif
}
