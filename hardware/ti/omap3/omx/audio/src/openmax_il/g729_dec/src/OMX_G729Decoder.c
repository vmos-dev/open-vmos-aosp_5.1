
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
 * @file OMX_G729Decoder.c
 *
 * This file implements OMX Component for G729 decoder that
 * is fully compliant with the OMX Audio specification .
 *
 * @path  $(OMAPSW_MPU)\linux\audio\src\openmax_il\g729_dec\src
 *
 * @rev  0.4
 */
/* ----------------------------------------------------------------------------- 
 *! 
 *! Revision History 
 *! ===================================
 *! Date         Author(s)            Version  Description
 *! ---------    -------------------  -------  ---------------------------------
 *! 03-Jan-2007  A.Donjon                         0.1      Code update for G729 DECODER
 *! 01-Mar-2007  A.Donjon                         0.2      RM, DVFS changes
 *! 08-Jun-2007  A.Donjon                         0.3      Variable input buffer size
 *! 04-Jul-2007  A.Donjon                         0.4      Init params   
 *! 
 *!
 * ================================================================================= */

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

#endif
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <dbapi.h>

/*-------program files ----------------------------------------*/

#ifdef __PERF_INSTRUMENTATION__
#include "perf.h"
#endif

#include <OMX_Component.h>
#include <TIDspOmx.h>

#include "OMX_G729Decoder.h"
#include "OMX_G729Dec_Utils.h"

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
                                   OMX_CALLBACKTYPE* pCallBacks, OMX_PTR pAppData);
static OMX_ERRORTYPE GetComponentVersion (OMX_HANDLETYPE hComp,
                                          OMX_STRING pComponentName,
                                          OMX_VERSIONTYPE* pComponentVersion,
                                          OMX_VERSIONTYPE* pSpecVersion,
                                          OMX_UUIDTYPE* pComponentUUID);

static OMX_ERRORTYPE SendCommand (OMX_HANDLETYPE hComp, OMX_COMMANDTYPE nCommand,
                                  OMX_U32 nParam, OMX_PTR pCmdData);

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
                                       OMX_OUT OMX_INDEXTYPE *pIndexType);

static OMX_ERRORTYPE ComponentRoleEnum(
                                       OMX_IN OMX_HANDLETYPE hComponent,
                                       OMX_OUT OMX_U8 *cRole,
                                       OMX_IN OMX_U32 nIndex);

/* ======================================================================= */
/**
 * @def    FIFO1, FIFO2              Define Fifo Path
 */
/* ======================================================================= */            
#define FIFO1 "/dev/fifo.1"
#define FIFO2 "/dev/fifo.2"

/* ======================================================================= */
/**
 * @def    PERMS                      Define Read and Write Permisions.
 */
/* ======================================================================= */            
#define PERMS 0666

#ifdef DSP_RENDERING_ON
AM_COMMANDDATATYPE cmd_data;
#endif



/*--------macros ----------------------------------------------*/
            

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
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef_ip = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDef_op = NULL;
    OMX_AUDIO_PARAM_PORTFORMATTYPE *pInPortFormat = NULL;
    OMX_AUDIO_PARAM_PORTFORMATTYPE *pOutPortFormat = NULL;
    G729DEC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    OMX_AUDIO_PARAM_G729TYPE *g729_ip = NULL;
    OMX_AUDIO_PARAM_PCMMODETYPE *g729_op = NULL;
    OMX_ERRORTYPE error = OMX_ErrorNone;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE*) hComp;
    OMX_S16 i=0;

    G729DEC_DPRINT ("%d ::OMX_ComponentInit\n", __LINE__);
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
    OMX_G729MALLOC_STRUCT(pHandle->pComponentPrivate,G729DEC_COMPONENT_PRIVATE);
    if(pHandle->pComponentPrivate == NULL) {
        error = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    ((G729DEC_COMPONENT_PRIVATE *)
     pHandle->pComponentPrivate)->pHandle = pHandle;
    pComponentPrivate = pHandle->pComponentPrivate;      

#ifdef __PERF_INSTRUMENTATION__
    pComponentPrivate->pPERF = PERF_Create(PERF_FOURCC('7','2','9',' '),
                                           PERF_ModuleLLMM |
                                           PERF_ModuleAudioDecode);
#endif

    /* Initialize component data structures to default values */
    pComponentPrivate->sPortParam.nPorts = 0x2;
    pComponentPrivate->sPortParam.nStartPortNumber = 0x0;
    error = OMX_ErrorNone;

    OMX_G729MALLOC_STRUCT(g729_ip, OMX_AUDIO_PARAM_G729TYPE);
    OMX_G729MALLOC_STRUCT(g729_op, OMX_AUDIO_PARAM_PCMMODETYPE);

    ((G729DEC_COMPONENT_PRIVATE *)
     pHandle->pComponentPrivate)->g729Params = g729_ip;
    ((G729DEC_COMPONENT_PRIVATE *)
     pHandle->pComponentPrivate)->pcmParams = g729_op;

    G729DEC_DPRINT("%d Malloced g729Params = 0x%p\n",__LINE__,((G729DEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate)->g729Params);
    G729DEC_DPRINT("%d Malloced pcmParams = 0x%p\n",__LINE__,((G729DEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate)->pcmParams);

 
    OMX_G729MALLOC_STRUCT(pComponentPrivate->pInputBufferList, G729DEC_BUFFERLIST);

    pComponentPrivate->pInputBufferList->numBuffers = 0; /* initialize number of buffers */
    OMX_G729MALLOC_STRUCT(pComponentPrivate->pOutputBufferList, G729DEC_BUFFERLIST);

    pComponentPrivate->pOutputBufferList->numBuffers = 0; /* initialize number of buffers */
    pComponentPrivate->bPlayCompleteFlag = 0;
    for (i=0; i < MAX_NUM_OF_BUFS; i++) {
        pComponentPrivate->pOutputBufferList->pBufHdr[i] = NULL;
        pComponentPrivate->pInputBufferList->pBufHdr[i] = NULL;
        pComponentPrivate->arrTickCount[i] = 0;
        pComponentPrivate->arrTimestamp[i] = 0;
    }
        
    G729DEC_DPRINT("Setting dasfmode to 0\n");
    pComponentPrivate->rtmx = 0;
    pComponentPrivate->dasfmode = 0;
    pComponentPrivate->bPortDefsAllocated = 1;
    pComponentPrivate->bCompThreadStarted = 0;
    pComponentPrivate->nHoldLength = 0;
    pComponentPrivate->pHoldBuffer = NULL;
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
    for (i=0; i < MAX_NUM_OF_BUFS; i++) {
        pComponentPrivate->pInputBufHdrPending[i] = NULL;
        pComponentPrivate->pOutputBufHdrPending[i] = NULL;
    }
    pComponentPrivate->bJustReenabled = 0;
    pComponentPrivate->nInvalidFrameCount = 0;
    pComponentPrivate->nNumInputBufPending = 0;
    pComponentPrivate->nNumOutputBufPending = 0;
    pComponentPrivate->bDisableCommandPending = 0;
    pComponentPrivate->nOutStandingFillDones = 0;
    pComponentPrivate->bStopSent=0;
    pComponentPrivate->bBypassDSP = OMX_FALSE;
    pComponentPrivate->bNoIdleOnStop = OMX_FALSE;
    pComponentPrivate->pParams = NULL;
    pComponentPrivate->sDeviceString = malloc(100*sizeof(OMX_STRING));
    if (pComponentPrivate->sDeviceString == NULL) {
	 G729DEC_DPRINT("%d ::malloc failed\n", __LINE__);
	 goto EXIT;
    }
    strcpy((char*)pComponentPrivate->sDeviceString,"/eteedn:i0:o0/codec\0");
    pComponentPrivate->IpBufindex = 0;
    pComponentPrivate->OpBufindex = 0;
    pComponentPrivate->nUnhandledFillThisBuffers=0;
    pComponentPrivate->nUnhandledEmptyThisBuffers = 0;
    pComponentPrivate->bFlushOutputPortCommandPending = OMX_FALSE;
    pComponentPrivate->bFlushInputPortCommandPending = OMX_FALSE;    
    pComponentPrivate->bFlushEventPending = 0;
    pComponentPrivate->bPreempted = OMX_FALSE;

    /* G729 format defaults */
    g729_ip->nPortIndex = 0;
    
    /* PCM format defaults */
    g729_op->nPortIndex = 1;
    g729_op->nChannels = 2; 
    g729_op->eNumData= OMX_NumericalDataSigned; 
    g729_op->nBitPerSample = 16;  
    g729_op->nSamplingRate = 44100;          
    g729_op->ePCMMode = OMX_AUDIO_PCMModeLinear; 

    /* Malloc and Set pPriorityMgmt defaults */
    OMX_G729MALLOC_STRUCT(pComponentPrivate->sPriorityMgmt, OMX_PRIORITYMGMTTYPE);
    OMX_G729CONF_INIT_STRUCT(pComponentPrivate->sPriorityMgmt, OMX_PRIORITYMGMTTYPE);

    /* Initialize sPriorityMgmt data structures to default values */
    pComponentPrivate->sPriorityMgmt->nGroupPriority = -1;
    pComponentPrivate->sPriorityMgmt->nGroupID = -1;
    
    /* Set input port defaults */
    OMX_G729MALLOC_STRUCT(pPortDef_ip, OMX_PARAM_PORTDEFINITIONTYPE);
    OMX_G729CONF_INIT_STRUCT(pPortDef_ip, OMX_PARAM_PORTDEFINITIONTYPE);
    pComponentPrivate->pPortDef[G729DEC_INPUT_PORT] = pPortDef_ip;

    G729DEC_MEMPRINT("%d:[ALLOC] %p\n",__LINE__,pPortDef_ip);

#ifdef UNDER_CE
    memset(pPortDef_ip,0,sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
#endif

    /* Set input port defaults */
    OMX_G729MALLOC_STRUCT(pPortDef_op, OMX_PARAM_PORTDEFINITIONTYPE);
    OMX_G729CONF_INIT_STRUCT(pPortDef_op, OMX_PARAM_PORTDEFINITIONTYPE);
    pComponentPrivate->pPortDef[G729DEC_OUTPUT_PORT] = pPortDef_op;


    G729DEC_MEMPRINT("%d:[ALLOC] %p\n",__LINE__,pPortDef_op);
    
#ifdef UNDER_CE
    memset(pPortDef_op,0,sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
#endif

    G729DEC_DPRINT ("%d ::pPortDef_ip = 0x%x\n", __LINE__,pPortDef_ip);
    G729DEC_DPRINT ("%d ::pPortDef_op = 0x%x\n", __LINE__,pPortDef_op);

    pPortDef_ip->nPortIndex = 0x0;
    pPortDef_ip->nBufferCountActual = NUM_G729DEC_INPUT_BUFFERS;
    pPortDef_ip->nBufferCountMin = NUM_G729DEC_INPUT_BUFFERS;
    pPortDef_ip->eDir = OMX_DirInput;
    pPortDef_ip->bEnabled = OMX_TRUE;
    pPortDef_ip->nBufferSize = INPUT_G729DEC_BUFFER_SIZE_MIN;
    pPortDef_ip->bPopulated = 0;
    pPortDef_ip->eDomain = OMX_PortDomainAudio;
    pPortDef_ip->format.audio.eEncoding = OMX_AUDIO_CodingG729; 

    pPortDef_op->nPortIndex = 0x1;
    pPortDef_op->nBufferCountActual = NUM_G729DEC_OUTPUT_BUFFERS;
    pPortDef_op->nBufferCountMin = NUM_G729DEC_OUTPUT_BUFFERS;
    pPortDef_op->eDir = OMX_DirOutput;
    pPortDef_op->bEnabled = OMX_TRUE;
    pPortDef_op->nBufferSize = OUTPUT_G729DEC_BUFFER_SIZE_MIN;
    pPortDef_op->bPopulated = 0;
    pPortDef_op->eDomain  = OMX_PortDomainAudio;
    pPortDef_op->format.audio.eEncoding = OMX_AUDIO_CodingPCM;

    OMX_G729MALLOC_STRUCT(pComponentPrivate->pInPortFormat, OMX_AUDIO_PARAM_PORTFORMATTYPE);
    OMX_G729CONF_INIT_STRUCT(pComponentPrivate->pInPortFormat, OMX_AUDIO_PARAM_PORTFORMATTYPE);
    /* Set input port format defaults */
    pInPortFormat = pComponentPrivate->pInPortFormat;
    OMX_G729CONF_INIT_STRUCT(pInPortFormat, OMX_AUDIO_PARAM_PORTFORMATTYPE);
    pInPortFormat->nPortIndex         = G729DEC_INPUT_PORT;
    pInPortFormat->nIndex             = OMX_IndexParamAudioG729;
    pInPortFormat->eEncoding          = OMX_AUDIO_CodingG729;

    /* Set output port format defaults */
    OMX_G729MALLOC_STRUCT(pComponentPrivate->pOutPortFormat, OMX_AUDIO_PARAM_PORTFORMATTYPE);
    OMX_G729CONF_INIT_STRUCT(pComponentPrivate->pOutPortFormat, OMX_AUDIO_PARAM_PORTFORMATTYPE);
    pOutPortFormat = (OMX_AUDIO_PARAM_PORTFORMATTYPE*) pComponentPrivate->pOutPortFormat;
    OMX_G729CONF_INIT_STRUCT(pOutPortFormat, OMX_AUDIO_PARAM_PORTFORMATTYPE);
    pOutPortFormat->nPortIndex         = G729DEC_OUTPUT_PORT;
    pOutPortFormat->nIndex             = OMX_IndexParamAudioPcm;
    pOutPortFormat->eEncoding          = OMX_AUDIO_CodingPCM;

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
    error = RMProxy_NewInitalize();
    if (error != OMX_ErrorNone) {
        G729DEC_DPRINT ("%d ::Error returned from loading ResourceManagerProxy thread\n",
                        __LINE__);
        goto EXIT;
    }
#endif

    error = G729DEC_StartComponentThread(pHandle);
    if (error != OMX_ErrorNone) {
        G729DEC_DPRINT ("%d ::Error returned from the Component\n",
                        __LINE__);
        goto EXIT;
    }
    
#ifdef DSP_RENDERING_ON
    if((pComponentPrivate->fdwrite=open(FIFO1,O_WRONLY))<0) {
        G729DEC_DPRINT("[G729 Dec Component] - failure to open WRITE pipe\n");
    }

    G729DEC_DPRINT ("%d ::OMX_ComponentInit\n", __LINE__);
    if((pComponentPrivate->fdread=open(FIFO2,O_RDONLY))<0) {
        G729DEC_DPRINT("[G729 Dec Component] - failure to open READ pipe\n");
        goto EXIT;
    }
#endif    
 EXIT:
    G729DEC_DPRINT ("%d ::OMX_ComponentInit - returning %d\n", __LINE__,error);
    return error;
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

    G729DEC_COMPONENT_PRIVATE *pComponentPrivate =
        (G729DEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;

    if (pCallBacks == NULL) {
        G729DEC_DPRINT("About to return OMX_ErrorBadParameter on line %d\n",__LINE__);
        eError = OMX_ErrorBadParameter;
        G729DEC_DPRINT ("%d :: Received the empty callbacks from the \
                application\n",__LINE__);
        goto EXIT;
    }

    /*Copy the callbacks of the application to the component private */
    memcpy (&(pComponentPrivate->cbInfo), pCallBacks, sizeof(OMX_CALLBACKTYPE));

    /*copy the application private data to component memory */
    pHandle->pApplicationPrivate = pAppData;

    pComponentPrivate->curState = OMX_StateLoaded;

 EXIT:
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
    OMX_ERRORTYPE eError = OMX_ErrorNotImplemented;

    
    G729DEC_DPRINT("Exiting. Returning = 0x%x\n", eError);
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
                                  OMX_U32 nParam,OMX_PTR pCmdData)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    ssize_t nRet = 0;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)phandle;
    G729DEC_COMPONENT_PRIVATE *pCompPrivate =
        (G729DEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
 
    G729DEC_DPRINT ("%d:::Inside SendCommand\n",__LINE__);
#ifdef __PERF_INSTRUMENTATION__
    PERF_SendingCommand(pCompPrivate->pPERF,
                        Cmd,
                        (Cmd == OMX_CommandMarkBuffer) ? ((OMX_U32) pCmdData) : nParam,
                        PERF_ModuleComponent);
#endif

    G729DEC_DPRINT("phandle = %p\n",phandle);
    G729DEC_DPRINT("pCompPrivate = %p\n",pCompPrivate);

    pCompPrivate->pHandle = phandle;

 
    if(pCompPrivate->curState == OMX_StateInvalid){
        G729DEC_DPRINT ("%d:::Inside SendCommand\n",__LINE__);
        eError = OMX_ErrorInvalidState;
        G729DEC_DPRINT("%d :: G729DEC: Error Notofication \
                                         Sent to App\n",__LINE__);
        pCompPrivate->cbInfo.EventHandler (
                                           pHandle, pHandle->pApplicationPrivate,
                                           OMX_EventError, OMX_ErrorInvalidState,0,
                                           "Invalid State");

        goto EXIT;
    }


    switch(Cmd) {
    case OMX_CommandStateSet:
        G729DEC_DPRINT ("%d:::Inside SendCommand\n",__LINE__);
        G729DEC_DPRINT ("%d:::pCompPrivate->curState = %d\n",__LINE__,pCompPrivate->curState);
        if (nParam == OMX_StateLoaded) {
            pCompPrivate->bLoadedCommandPending = OMX_TRUE;
        }
        if(pCompPrivate->curState == OMX_StateLoaded) {
            if((nParam == OMX_StateExecuting) || (nParam == OMX_StatePause)) {
                pCompPrivate->cbInfo.EventHandler (
                                                   pHandle,
                                                   pHandle->pApplicationPrivate,
                                                   OMX_EventError,
                                                   OMX_ErrorIncorrectStateTransition,
                                                   0,
                                                   NULL);
                goto EXIT;
            }

            if(nParam == OMX_StateInvalid) {
                G729DEC_DPRINT ("%d:::Inside SendCommand\n",__LINE__);
                pCompPrivate->curState = OMX_StateInvalid;
                pCompPrivate->cbInfo.EventHandler (
                                                   pHandle,
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
        G729DEC_DPRINT ("%d:::Inside SendCommand\n",__LINE__);
        if(nParam > 1 && nParam != -1) {
            eError = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        break;
    case OMX_CommandPortDisable:
        G729DEC_DPRINT ("%d:::Inside SendCommand\n",__LINE__);
        break;
    case OMX_CommandPortEnable:
        G729DEC_DPRINT ("%d:::Inside SendCommand\n",__LINE__);
        break;
    case OMX_CommandMarkBuffer:
        G729DEC_DPRINT ("%d:::Inside SendCommand\n",__LINE__);
        if (nParam > 0) {
            eError = OMX_ErrorBadPortIndex;
            goto EXIT;
        }
        break;
    default:
        G729DEC_DPRINT("%d :: G729DEC: Command Received Default \
                                                      error\n",__LINE__);
        pCompPrivate->cbInfo.EventHandler (
                                           pHandle, pHandle->pApplicationPrivate,
                                           OMX_EventError,
                                           OMX_ErrorUndefined,0,
                                           "Invalid Command");
        break;

    }

    G729DEC_DPRINT ("%d:::Inside SendCommand\n",__LINE__);
    nRet = write (pCompPrivate->cmdPipe[1], &Cmd, sizeof(Cmd));
    if (nRet == -1) {
        G729DEC_DPRINT ("%d:::Inside SendCommand\n",__LINE__);
        eError = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    if (Cmd == OMX_CommandMarkBuffer) {
        nRet = write(pCompPrivate->cmdDataPipe[1], &pCmdData, 
                     sizeof(OMX_PTR));
    }
    else {
        nRet = write(pCompPrivate->cmdDataPipe[1], &nParam,
                     sizeof(OMX_U32));
    }

    G729DEC_DPRINT ("%d:::Inside SendCommand\n",__LINE__);
    G729DEC_DPRINT ("%d:::nRet = %d\n",__LINE__,nRet);
    if (nRet == -1) {
        G729DEC_DPRINT ("%d:::Inside SendCommand\n",__LINE__);
        eError = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

#ifdef DSP_RENDERING_ON
    if(Cmd == OMX_CommandStateSet && nParam == OMX_StateExecuting) {
        /* enable Tee device command*/
        cmd_data.hComponent = pHandle;
        cmd_data.AM_Cmd = AM_CommandTDNDownlinkMode;
        cmd_data.param1 = 0;
        cmd_data.param2 = 0;
        cmd_data.streamID = 0;                  
        if((write(pCompPrivate->fdwrite, &cmd_data, sizeof(cmd_data)))<0) {
            eError = OMX_ErrorHardware;
            goto EXIT;
        }
    }
#endif     


 EXIT:
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
    G729DEC_COMPONENT_PRIVATE  *pComponentPrivate = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE *pParameterStructure = NULL;
    /*    OMX_PARAM_BUFFERSUPPLIERTYPE *pBufferSupplier = NULL;      */

    G729DEC_DPRINT ("Inside the GetParameter Line %d\n",__LINE__);

    G729DEC_DPRINT("%d :: Inside the GetParameter:: %x\n",__LINE__,nParamIndex);

    pComponentPrivate = (G729DEC_COMPONENT_PRIVATE *)(((OMX_COMPONENTTYPE*)hComp)->pComponentPrivate);
    pParameterStructure = (OMX_PARAM_PORTDEFINITIONTYPE*)ComponentParameterStructure;

    if (pParameterStructure == NULL) {
        eError = OMX_ErrorBadParameter;
        goto EXIT;

    }
    G729DEC_DPRINT("pParameterStructure = %p\n",pParameterStructure);


    G729DEC_DPRINT ("Inside the GetParameter Line %d\n",__LINE__);
    if(pComponentPrivate->curState == OMX_StateInvalid) {
        pComponentPrivate->cbInfo.EventHandler(
                                               hComp,
                                               ((OMX_COMPONENTTYPE *)hComp)->pApplicationPrivate,
                                               OMX_EventError,
                                               OMX_ErrorIncorrectStateOperation, 
                                               0,
                                               NULL);
        G729DEC_DPRINT ("Inside the GetParameter Line %d\n",__LINE__);
    }
    G729DEC_DPRINT ("Inside the GetParameter Line %d\n",__LINE__);
    switch(nParamIndex){

    case OMX_IndexParamAudioInit:
        G729DEC_DPRINT ("Inside the GetParameter Line %d\n",__LINE__);
        G729DEC_DPRINT ("OMX_IndexParamAudioInit\n");
        memcpy(ComponentParameterStructure, &pComponentPrivate->sPortParam, sizeof(OMX_PORT_PARAM_TYPE));
        break;

    case OMX_IndexParamPortDefinition:
        G729DEC_DPRINT ("Inside the GetParameter Line %d\n",__LINE__);
        G729DEC_DPRINT ("pParameterStructure->nPortIndex = %d\n",pParameterStructure->nPortIndex);
        G729DEC_DPRINT ("pComponentPrivate->pPortDef[G729DEC_INPUT_PORT]->nPortIndex = %d\n",pComponentPrivate->pPortDef[G729DEC_INPUT_PORT]->nPortIndex);
        if(((OMX_PARAM_PORTDEFINITIONTYPE *)(ComponentParameterStructure))->nPortIndex == 
           pComponentPrivate->pPortDef[G729DEC_INPUT_PORT]->nPortIndex) {
            G729DEC_DPRINT ("Inside the GetParameter Line %d\n",__LINE__);

            memcpy(ComponentParameterStructure,
                   pComponentPrivate->pPortDef[G729DEC_INPUT_PORT], 
                   sizeof(OMX_PARAM_PORTDEFINITIONTYPE)
                   );
            G729DEC_DPRINT ("Inside the GetParameter Line %d\n",__LINE__);

        } else if(((OMX_PARAM_PORTDEFINITIONTYPE *)(ComponentParameterStructure))->nPortIndex == 
                  pComponentPrivate->pPortDef[G729DEC_OUTPUT_PORT]->nPortIndex) {
            G729DEC_DPRINT ("Inside the GetParameter Line %d\n",__LINE__);

            memcpy(ComponentParameterStructure, 
                   pComponentPrivate->pPortDef[G729DEC_OUTPUT_PORT], 
                   sizeof(OMX_PARAM_PORTDEFINITIONTYPE)
                   );
            G729DEC_DPRINT ("Inside the GetParameter Line %d\n",__LINE__);

        } else {
            G729DEC_DPRINT ("Inside the GetParameter Line %d\n",__LINE__);

            eError = OMX_ErrorBadPortIndex;
        }
        break;

    case OMX_IndexParamAudioPortFormat:
        G729DEC_DPRINT ("((OMX_AUDIO_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nPortIndex = %d\n",((OMX_AUDIO_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nPortIndex);
        G729DEC_DPRINT ("pComponentPrivate->pInPortFormat.nPortIndex= %d\n",pComponentPrivate->pInPortFormat->nPortIndex);
        G729DEC_DPRINT ("pComponentPrivate->pOutPortFormat.nPortIndex= %d\n",pComponentPrivate->pOutPortFormat->nPortIndex);
        if(((OMX_AUDIO_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nPortIndex == 
           pComponentPrivate->pInPortFormat->nPortIndex) {
            if(((OMX_AUDIO_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nPortIndex >
               pComponentPrivate->pInPortFormat->nPortIndex){
                eError = OMX_ErrorNoMore;
            }
            else{
                memcpy(ComponentParameterStructure, pComponentPrivate->pInPortFormat, 
                       sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
            }
        }
        else if(((OMX_AUDIO_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nPortIndex == 
                pComponentPrivate->pOutPortFormat->nPortIndex){
            if(((OMX_AUDIO_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nPortIndex >
               pComponentPrivate->pInPortFormat->nPortIndex){
                eError = OMX_ErrorNoMore;
            }
            else{
                memcpy(ComponentParameterStructure, pComponentPrivate->pOutPortFormat, 
                       sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
            }
        }
        else {
            G729DEC_DPRINT ("Inside the GetParameter Line %d\n",__LINE__);
            eError = OMX_ErrorBadPortIndex;
        }
        break;

    case OMX_IndexParamAudioG729:
        G729DEC_DPRINT("%d :: GetParameter OMX_IndexParamAudioG729 \n",__LINE__);
        if(((OMX_AUDIO_PARAM_G729TYPE *)(ComponentParameterStructure))->nPortIndex ==
           pComponentPrivate->g729Params->nPortIndex) {
            memcpy(ComponentParameterStructure,
                   pComponentPrivate->g729Params,
                   sizeof(OMX_AUDIO_PARAM_G729TYPE));
        } 
        else {
            G729DEC_DPRINT("%d :: OMX_ErrorBadPortIndex from GetParameter \n",__LINE__);
            eError = OMX_ErrorBadPortIndex;
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
            G729DEC_DPRINT("%d :: OMX_ErrorBadPortIndex from GetParameter \n",__LINE__);
            eError = OMX_ErrorBadPortIndex;
        }
        break;

    case OMX_IndexParamPriorityMgmt:
        if (pComponentPrivate->sPriorityMgmt == NULL) {
            eError = OMX_ErrorBadParameter;
            goto EXIT;
        }
        memcpy(ComponentParameterStructure, pComponentPrivate->sPriorityMgmt, sizeof(OMX_PRIORITYMGMTTYPE));
        break;

    case OMX_IndexParamCompBufferSupplier:
        if(((OMX_PARAM_BUFFERSUPPLIERTYPE *)(ComponentParameterStructure))->nPortIndex == OMX_DirInput) {
            G729DEC_DPRINT(":: GetParameter OMX_IndexParamCompBufferSupplier \n");
            /*  memcpy(ComponentParameterStructure, pBufferSupplier, sizeof(OMX_PARAM_BUFFERSUPPLIERTYPE)); */
        }
        else
            if(((OMX_PARAM_BUFFERSUPPLIERTYPE *)(ComponentParameterStructure))->nPortIndex == OMX_DirOutput) {
                G729DEC_DPRINT(":: GetParameter OMX_IndexParamCompBufferSupplier \n");
                /*memcpy(ComponentParameterStructure, pBufferSupplier, sizeof(OMX_PARAM_BUFFERSUPPLIERTYPE)); */
            } 
            else {
                G729DEC_DPRINT(":: OMX_ErrorBadPortIndex from GetParameter");
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
        break;
    }
 EXIT:
    G729DEC_DPRINT("%d :: Exiting GetParameter:: %x\n",__LINE__,nParamIndex);
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
    G729DEC_COMPONENT_PRIVATE  *pComponentPrivate = NULL;
    OMX_AUDIO_PARAM_PCMMODETYPE* pPcmPort = NULL;       
    OMX_PARAM_BUFFERSUPPLIERTYPE *pBufferSupplier = NULL;       

    pComponentPrivate = (G729DEC_COMPONENT_PRIVATE *)(((OMX_COMPONENTTYPE*)hComp)->pComponentPrivate);

    if (pCompParam == NULL) {
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }

    switch(nParamIndex) {
    case OMX_IndexParamAudioPortFormat:
        {
            /* 0 means Input port */
            if(pComponentPrivate->pInPortFormat->nPortIndex == ((OMX_AUDIO_PARAM_PORTFORMATTYPE*)pCompParam)->nPortIndex) {
                memcpy(pComponentPrivate->pInPortFormat, pCompParam, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
            } 
            else if (pComponentPrivate->pOutPortFormat->nPortIndex == ((OMX_AUDIO_PARAM_PORTFORMATTYPE*)pCompParam)->nPortIndex) {
                /* 1 means Output port */
                memcpy(pComponentPrivate->pOutPortFormat, pCompParam, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
            }
            else {
                G729DEC_DPRINT ("%d :: Wrong Port Index Parameter\n", __LINE__);
                eError = OMX_ErrorBadParameter;
                goto EXIT;
            }
        }
        break;
            
    case OMX_IndexParamAudioG729:
        {
            OMX_AUDIO_PARAM_G729TYPE *pCompG729Param =
                (OMX_AUDIO_PARAM_G729TYPE *)pCompParam;
	    if (((G729DEC_COMPONENT_PRIVATE*)
	        pHandle->pComponentPrivate)->g729Params == NULL) {
                eError = OMX_ErrorBadParameter;
                goto EXIT;
            }
            /* 0 means Input port */
	    if(pCompG729Param->nPortIndex == 0) {
		memcpy(((G729DEC_COMPONENT_PRIVATE*)
	            pHandle->pComponentPrivate)->g729Params,
		    pCompG729Param, sizeof(OMX_AUDIO_PARAM_G729TYPE));
	    }
            else {
                eError = OMX_ErrorBadPortIndex;
            }
        }
        break;
    case OMX_IndexParamPortDefinition:
        if(((OMX_PARAM_PORTDEFINITIONTYPE *)(pCompParam))->nPortIndex == 
           pComponentPrivate->pPortDef[G729DEC_INPUT_PORT]->nPortIndex) {

            memcpy(pComponentPrivate->pPortDef[G729DEC_INPUT_PORT], 
                   pCompParam,
                   sizeof(OMX_PARAM_PORTDEFINITIONTYPE)
                   );

        } 
        else if(((OMX_PARAM_PORTDEFINITIONTYPE *)(pCompParam))->nPortIndex == 
                pComponentPrivate->pPortDef[G729DEC_OUTPUT_PORT]->nPortIndex) {

            memcpy(pComponentPrivate->pPortDef[G729DEC_OUTPUT_PORT], 
                   pCompParam, 
                   sizeof(OMX_PARAM_PORTDEFINITIONTYPE)
                   );

        } 
        else {
            eError = OMX_ErrorBadPortIndex;
        }
        break;
    case OMX_IndexParamPriorityMgmt:
        if (pComponentPrivate->curState != OMX_StateLoaded) {
            eError = OMX_ErrorIncorrectStateOperation;
            goto EXIT;
        }
        else{
	    if (pComponentPrivate->sPriorityMgmt == NULL) {
                eError = OMX_ErrorBadParameter;
                goto EXIT;
            }

            memcpy(pComponentPrivate->sPriorityMgmt, (OMX_PRIORITYMGMTTYPE*)pCompParam, sizeof(OMX_PRIORITYMGMTTYPE)); 
                        
        }
        break;

    case OMX_IndexParamStandardComponentRole:
        /*      if (pCompParam) {
                pRole = (OMX_PARAM_COMPONENTROLETYPE *)pCompParam;
                memcpy(&(pComponentPrivate->componentRole), (void *)pRole, sizeof(OMX_PARAM_COMPONENTROLETYPE));
                } else {
                eError = OMX_ErrorBadParameter;
                }*/
        eError = OMX_ErrorBadParameter;
        break;

    case OMX_IndexParamAudioPcm:
        if (pComponentPrivate->pcmParams == NULL) {
            eError = OMX_ErrorBadParameter;
            goto EXIT;
        }
        if(pCompParam){
            pPcmPort= (OMX_AUDIO_PARAM_PCMMODETYPE *)pCompParam;
            memcpy(pComponentPrivate->pcmParams, pPcmPort, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
        }
        else{
            eError = OMX_ErrorBadParameter;
        }
        break;

    case OMX_IndexParamCompBufferSupplier:
        OMX_G729MALLOC_STRUCT(pBufferSupplier, OMX_PARAM_BUFFERSUPPLIERTYPE);
        if(((OMX_PARAM_PORTDEFINITIONTYPE *)(pCompParam))->nPortIndex ==
           pComponentPrivate->pPortDef[OMX_DirInput]->nPortIndex) {
            G729DEC_DPRINT(":: SetParameter OMX_IndexParamCompBufferSupplier \n");
            pBufferSupplier->eBufferSupplier = OMX_BufferSupplyInput;
            memcpy(pBufferSupplier, pCompParam, sizeof(OMX_PARAM_BUFFERSUPPLIERTYPE));                                                              
                                        
        }
        else if(((OMX_PARAM_PORTDEFINITIONTYPE *)(pCompParam))->nPortIndex ==
                pComponentPrivate->pPortDef[OMX_DirOutput]->nPortIndex) {
            G729DEC_DPRINT(":: SetParameter OMX_IndexParamCompBufferSupplier \n");
            pBufferSupplier->eBufferSupplier = OMX_BufferSupplyOutput;
            memcpy(pBufferSupplier, pCompParam, sizeof(OMX_PARAM_BUFFERSUPPLIERTYPE));
        } 
        else {
            G729DEC_DPRINT(":: OMX_ErrorBadPortIndex from SetParameter");
            eError = OMX_ErrorBadPortIndex;
        }
        break;

    default:
        break;

    }
 EXIT:
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

    G729DEC_COMPONENT_PRIVATE *pComponentPrivate = NULL;

    pComponentPrivate = (G729DEC_COMPONENT_PRIVATE *)
        (((OMX_COMPONENTTYPE*)hComp)->pComponentPrivate);
  
    if (pComponentPrivate == NULL) 
        eError = OMX_ErrorBadParameter;
    else
        memcpy(ComponentConfigStructure,pComponentPrivate,sizeof(G729DEC_COMPONENT_PRIVATE));

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
    OMX_COMPONENTTYPE* pHandle = (OMX_COMPONENTTYPE*)hComp;
    G729DEC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    OMX_S16 *customFlag = NULL;
    TI_OMX_DSP_DEFINITION *configData = NULL;
    
    OMX_AUDIO_CONFIG_MUTETYPE *pMuteStructure = NULL; 
    OMX_AUDIO_CONFIG_VOLUMETYPE *pVolumeStructure = NULL; 

    TI_OMX_DATAPATH dataPath;
    
    
    G729DEC_DPRINT("%d :: Entering SetConfig\n", __LINE__);
    if (pHandle == NULL) {
        G729DEC_DPRINT ("%d :: Invalid HANDLE OMX_ErrorBadParameter \n",__LINE__);
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pComponentPrivate =
	 (G729DEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    switch (nConfigIndex) {
    case  OMX_IndexCustomG729DecHeaderInfoConfig:
        {
            G729DEC_DPRINT("%d :: SetConfig OMX_IndexCustomNbG729DecHeaderInfoConfig \n",__LINE__);
            configData = (TI_OMX_DSP_DEFINITION*)ComponentConfigStructure;
            if (configData == NULL) {
                eError = OMX_ErrorBadParameter;
                G729DEC_DPRINT("%d :: OMX_ErrorBadParameter from SetConfig\n",__LINE__);
                goto EXIT;
            }
            pComponentPrivate->dasfmode = configData->dasfMode;
            if(configData->dasfMode == 2){
                pComponentPrivate->dasfmode = 1;
                pComponentPrivate->rtmx = 1;
            }
            pComponentPrivate->acdnmode = configData->acousticMode;

            if (pComponentPrivate->dasfmode ){
                pComponentPrivate->pPortDef[G729DEC_OUTPUT_PORT]->bEnabled = 0;                                               
            }

            G729DEC_DPRINT("pComponentPrivate->acdnmode = %d\n",pComponentPrivate->acdnmode);
            G729DEC_DPRINT("pComponentPrivate->dasfmode = %d\n",pComponentPrivate->dasfmode);
                  
            pComponentPrivate->streamID = configData->streamId;
            break;
        }
    case  OMX_IndexCustomG729DecDataPath:
        customFlag = (OMX_S16*)ComponentConfigStructure;
        if (customFlag == NULL) {
            eError = OMX_ErrorBadParameter;
            goto EXIT;
        }

        dataPath = *customFlag;

        switch(dataPath) {
        case DATAPATH_APPLICATION:
            OMX_MMMIXER_DATAPATH(pComponentPrivate->sDeviceString,
                                 RENDERTYPE_DECODER, pComponentPrivate->streamID);
            /*          strcpy((char*)pComponentPrivate->sDeviceString,(char*)ETEEDN_STRING); */
            break;

        case DATAPATH_APPLICATION_RTMIXER:
            strcpy((char*)pComponentPrivate->sDeviceString,(char*)RTM_STRING);
            break;

        case DATAPATH_ACDN:
            strcpy((char*)pComponentPrivate->sDeviceString,(char*)ACDN_STRING);
            break;

        default:
            break;
                
        }
        break;
        
    case OMX_IndexCustomG729DecModeDasfConfig:
        {
            G729DEC_DPRINT("%d :: SetConfig OMX_IndexCustomG729DecModeDasfConfig \n",__LINE__);
            customFlag = (OMX_S16*)ComponentConfigStructure;
            if (customFlag == NULL) {
                eError = OMX_ErrorBadParameter;
                G729DEC_DPRINT("%d :: OMX_ErrorBadParameter from SetConfig\n",__LINE__);
                goto EXIT;
            }
            pComponentPrivate->dasfmode = *customFlag;
            G729DEC_DPRINT("pComponentPrivate->dasfmode = %d\n",pComponentPrivate->dasfmode);
            if (pComponentPrivate->dasfmode ){
                pComponentPrivate->pPortDef[G729DEC_OUTPUT_PORT]->bEnabled = 0;                                               
            }
            break;
        }
    case OMX_IndexConfigAudioMute:
        {
#ifdef DSP_RENDERING_ON
            pMuteStructure = (OMX_AUDIO_CONFIG_MUTETYPE *)ComponentConfigStructure; 
            G729DEC_DPRINT("Set Mute/Unmute for playback stream\n"); 
            cmd_data.hComponent = hComp; 
            if(pMuteStructure->bMute == OMX_TRUE) 
            { 
                G729DEC_DPRINT("Mute the playback stream\n"); 
                cmd_data.AM_Cmd = AM_CommandStreamMute; 
            } 
            else 
            { 
                G729DEC_DPRINT("unMute the playback stream\n"); 
                cmd_data.AM_Cmd = AM_CommandStreamUnMute; 
            } 
            cmd_data.param1 = 0; 
            cmd_data.param2 = 0; 
            cmd_data.streamID = pComponentPrivate->streamID; 
            if((write(pComponentPrivate->fdwrite, &cmd_data, sizeof(cmd_data)))<0) 
            {    
                G729DEC_DPRINT("[G729 decoder] - fail to send Mute command to audio manager\n"); 
            } 
     
            break;     
#endif
        }
    case OMX_IndexConfigAudioVolume:
        {
#ifdef DSP_RENDERING_ON
            pVolumeStructure = (OMX_AUDIO_CONFIG_VOLUMETYPE *)ComponentConfigStructure; 
            G729DEC_DPRINT("Set volume for playback stream\n"); 
            cmd_data.hComponent = hComp; 
            cmd_data.AM_Cmd = AM_CommandSWGain; 
            cmd_data.param1 = pVolumeStructure->sVolume.nValue; 
            cmd_data.param2 = 0; 
            cmd_data.streamID = pComponentPrivate->streamID; 
       
            if((write(pComponentPrivate->fdwrite, &cmd_data, sizeof(cmd_data)))<0)
            {    
                G729DEC_DPRINT("[G729 decoder] - fail to send Volume command to audio manager\n"); 
            } 

            break;
#endif
        }
    default:
        eError = OMX_ErrorUnsupportedIndex;
        break;
    }
 EXIT:
    G729DEC_DPRINT("%d :: Exiting SetConfig\n", __LINE__);
    G729DEC_DPRINT("%d :: Returning = 0x%x\n",__LINE__,eError);
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
    OMX_ERRORTYPE error = OMX_ErrorUndefined;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;

    if (!pState) {
        error = OMX_ErrorBadParameter;
        G729DEC_DPRINT("About to return OMX_ErrorBadParameter on line %d\n",__LINE__);
        goto EXIT;
    }

    if (pHandle && pHandle->pComponentPrivate) {
        *pState =  ((G729DEC_COMPONENT_PRIVATE*)
                    pHandle->pComponentPrivate)->curState;
    } else {
        *pState = OMX_StateLoaded;
    }

    error = OMX_ErrorNone;

 EXIT:
    return error;
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
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    G729DEC_COMPONENT_PRIVATE *pComponentPrivate =
        (G729DEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef = NULL;
    ssize_t ret = 0;

#ifdef __PERF_INSTRUMENTATION__
    PERF_ReceivedFrame(pComponentPrivate->pPERF,
                       pBuffer->pBuffer,
                       pBuffer->nFilledLen,
                       PERF_ModuleHLMM);
#endif

    pPortDef = ((G729DEC_COMPONENT_PRIVATE*) 
                pComponentPrivate)->pPortDef[G729DEC_INPUT_PORT];
    if(!pPortDef->bEnabled) {
        G729DEC_DPRINT("About to return OMX_ErrorIncorrectStateOperation Line %d\n",__LINE__);
        eError = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }
    if (pBuffer == NULL) {
        eError = OMX_ErrorBadParameter;
        G729DEC_DPRINT("About to return OMX_ErrorBadParameter on line %d\n",__LINE__);
        goto EXIT;
    }
    if (pBuffer->nSize != sizeof(OMX_BUFFERHEADERTYPE)) {
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if (pBuffer->nVersion.nVersion != pComponentPrivate->nVersion) {
        eError = OMX_ErrorVersionMismatch;
        goto EXIT;
    }
    if (pBuffer->nInputPortIndex != G729DEC_INPUT_PORT) {
        eError  = OMX_ErrorBadPortIndex;
        goto EXIT;
    }
    if(pComponentPrivate->curState != OMX_StateExecuting &&
       pComponentPrivate->curState != OMX_StatePause)
    {
        G729DEC_DPRINT("About to return OMX_ErrorIncorrectStateOperation Line %d\n",__LINE__);
        eError = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }
    G729DEC_DPRINT("\n------------------------------------------\n\n");
    G729DEC_DPRINT ("%d :: Component Sending Filled ip buff %p \
                             to Component Thread\n",__LINE__,pBuffer);
    G729DEC_DPRINT("\n------------------------------------------\n\n");

    pComponentPrivate->app_nBuf--;
    pComponentPrivate->pMarkData = pBuffer->pMarkData;
    pComponentPrivate->hMarkTargetComponent = pBuffer->hMarkTargetComponent;
    pComponentPrivate->nUnhandledEmptyThisBuffers++;    
    ret = write (pComponentPrivate->dataPipe[1], &pBuffer,
                 sizeof(OMX_BUFFERHEADERTYPE*));       
    if (ret == -1) {
        G729DEC_DPRINT ("%d :: Error in Writing to the Data pipe\n", __LINE__);
        eError = OMX_ErrorHardware;
        goto EXIT;
    }
    pComponentPrivate->nEmptyThisBufferCount++;
 EXIT:
    G729DEC_DPRINT("Exiting EmptyThisBuffer()\n");
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
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    G729DEC_COMPONENT_PRIVATE *pComponentPrivate =
        (G729DEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef = NULL;
    
#ifdef __PERF_INSTRUMENTATION__
    PERF_ReceivedFrame(pComponentPrivate->pPERF,
                       pBuffer->pBuffer,
                       0,
                       PERF_ModuleHLMM);
#endif

    G729DEC_DPRINT("\n------------------------------------------\n\n");
    G729DEC_DPRINT ("%d :: Component Sending Emptied op buff %p \
                                  to Component Thread\n",__LINE__,pBuffer);
    G729DEC_DPRINT("\n------------------------------------------\n\n");
    pPortDef = ((G729DEC_COMPONENT_PRIVATE*) 
                pComponentPrivate)->pPortDef[G729DEC_OUTPUT_PORT];
    if(!pPortDef->bEnabled) {
        G729DEC_DPRINT("About to return OMX_ErrorIncorrectStateOperation Line %d\n",__LINE__);
        eError = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }
    if (pBuffer == NULL) {
        eError = OMX_ErrorBadParameter;
        G729DEC_DPRINT("About to return OMX_ErrorBadParameter on line %d\n",__LINE__);
        goto EXIT;
    }
    if (pBuffer->nSize != sizeof(OMX_BUFFERHEADERTYPE)) {
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if (pBuffer->nVersion.nVersion != pComponentPrivate->nVersion) {
        eError = OMX_ErrorVersionMismatch;
        goto EXIT;
    }
    if (pBuffer->nOutputPortIndex != G729DEC_OUTPUT_PORT) {
        eError  = OMX_ErrorBadPortIndex;
        goto EXIT;
    }
    if (pComponentPrivate->curState == OMX_StatePause) {
        G729DEC_DPRINT("FillThisBuffer called while paused\n");
    }
    if(pComponentPrivate->curState != OMX_StateExecuting && pComponentPrivate->curState != OMX_StatePause) {
        G729DEC_DPRINT("About to return OMX_ErrorIncorrectStateOperation Line %d\n",__LINE__);
        eError = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }
    pBuffer->nFilledLen = 0;
    /*Filling the Output buffer with zero */
    memset (pBuffer->pBuffer,0,pComponentPrivate->pPortDef[G729DEC_OUTPUT_PORT]->nBufferSize);
    pComponentPrivate->app_nBuf--;
    G729DEC_DPRINT("%d:Decrementing app_nBuf = %d\n",__LINE__,pComponentPrivate->app_nBuf);
    G729DEC_DPRINT("pComponentPrivate->pMarkBuf = 0x%x\n",pComponentPrivate->pMarkBuf);
    G729DEC_DPRINT("pComponentPrivate->pMarkData = 0x%x\n",pComponentPrivate->pMarkData);
    
    if(pComponentPrivate->pMarkBuf){
        pBuffer->hMarkTargetComponent = pComponentPrivate->pMarkBuf->hMarkTargetComponent;
        pBuffer->pMarkData = pComponentPrivate->pMarkBuf->pMarkData;
        pComponentPrivate->pMarkBuf = NULL;
    }
    if (pComponentPrivate->pMarkData) {
        pBuffer->hMarkTargetComponent = pComponentPrivate->hMarkTargetComponent;
        pBuffer->pMarkData = pComponentPrivate->pMarkData;
        pComponentPrivate->pMarkData = NULL;
    }
    pComponentPrivate->nUnhandledFillThisBuffers++;    
    write (pComponentPrivate->dataPipe[1], &pBuffer,
           sizeof (OMX_BUFFERHEADERTYPE*));
    pComponentPrivate->nFillThisBufferCount++;

 EXIT:
    G729DEC_DPRINT("Exiting FillThisBuffer()\n");
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

    /* inform audio manager to remove the streamID*/
    /* compose the data */
    OMX_COMPONENTTYPE *pComponent = (OMX_COMPONENTTYPE *)pHandle;
    G729DEC_COMPONENT_PRIVATE *pComponentPrivate =
        (G729DEC_COMPONENT_PRIVATE *)pComponent->pComponentPrivate;
    
#ifdef __PERF_INSTRUMENTATION__
    PERF_Boundary(pComponentPrivate->pPERF,
                  PERF_BoundaryStart | PERF_BoundaryCleanup);
#endif

    G729DEC_DPRINT ("%d ::ComponentDeInit\n",__LINE__);

#ifdef DSP_RENDERING_ON
    close(pComponentPrivate->fdwrite);
    close(pComponentPrivate->fdread);
#endif

#ifdef RESOURCE_MANAGER_ENABLED
    eError = RMProxy_NewSendCommand(pHandle, RMProxy_FreeResource, OMX_G729_Decoder_COMPONENT, 0, 3456,NULL);
    if (eError != OMX_ErrorNone) {
        G729DEC_DPRINT ("%d ::OMX_G729Decoder.c :: Error returned from destroy ResourceManagerProxy thread\n",
                        __LINE__);
    }
    eError = RMProxy_Deinitalize();
    if (eError != OMX_ErrorNone) {
        G729DEC_DPRINT ("%d ::Error returned from destroy ResourceManagerProxy thread\n",
                        __LINE__);
    }
#endif

    pComponentPrivate->bIsStopping = 1;
    eError = G729DEC_StopComponentThread(pHandle);
    /* Wait for thread to exit so we can get the status into "error" */

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

    /* close the pipe handles */
    G729DEC_FreeCompResources(pHandle);
    G729DEC_DPRINT ("%d ::After G729DEC_FreeCompResources\n",__LINE__);

#ifdef __PERF_INSTRUMENTATION__
    PERF_Boundary(pComponentPrivate->pPERF,
                  PERF_BoundaryComplete | PERF_BoundaryCleanup);
    PERF_Done(pComponentPrivate->pPERF);
#endif 

    OMX_G729MEMFREE_STRUCT(pComponentPrivate->sDeviceString);
    OMX_G729MEMFREE_STRUCT(pComponentPrivate->pInputBufferList);
    OMX_G729MEMFREE_STRUCT(pComponentPrivate->pOutputBufferList);
    OMX_G729MEMFREE_STRUCT(pComponentPrivate->g729Params);
    OMX_G729MEMFREE_STRUCT(pComponentPrivate->pcmParams);
    OMX_G729MEMFREE_STRUCT(pComponentPrivate->sPriorityMgmt);
    OMX_G729MEMFREE_STRUCT(pComponentPrivate->pPortDef[G729DEC_INPUT_PORT]);
    OMX_G729MEMFREE_STRUCT(pComponentPrivate->pPortDef[G729DEC_OUTPUT_PORT]);
    OMX_G729MEMFREE_STRUCT(pComponentPrivate->pInPortFormat);
    OMX_G729MEMFREE_STRUCT(pComponentPrivate->pOutPortFormat);
    
    OMX_G729MEMFREE_STRUCT(pComponentPrivate);
    G729DEC_DPRINT ("%d ::After free(pComponentPrivate)\n",__LINE__);

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
    OMX_ERRORTYPE eError = OMX_ErrorNotImplemented;
    
    G729DEC_DPRINT (stderr, "Inside the ComponentTunnelRequest\n");

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
    G729DEC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_BUFFERHEADERTYPE *pBufferHeader = NULL;
    
    pComponentPrivate = (G729DEC_COMPONENT_PRIVATE *)
        (((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);

    pPortDef = ((G729DEC_COMPONENT_PRIVATE*) 
                pComponentPrivate)->pPortDef[nPortIndex];
    G729DEC_DPRINT ("%d :: pPortDef = 0x%x\n", __LINE__,pPortDef);
    G729DEC_DPRINT ("%d :: pPortDef->bEnabled = %d\n", __LINE__,pPortDef->bEnabled);

    G729DEC_DPRINT ("pPortDef->bEnabled = %d\n", pPortDef->bEnabled);
    if(!(pPortDef->bEnabled))
    {
        pComponentPrivate->AlloBuf_waitingsignal = 1;  
           
#ifndef UNDER_CE
        pthread_mutex_lock(&pComponentPrivate->AlloBuf_mutex); 
        pthread_cond_wait(&pComponentPrivate->AlloBuf_threshold, &pComponentPrivate->AlloBuf_mutex);
        pthread_mutex_unlock(&pComponentPrivate->AlloBuf_mutex);
#else
        OMX_WaitForEvent(&(pComponentPrivate->AlloBuf_event));
#endif

    }
    OMX_G729MALLOC_STRUCT(pBufferHeader, OMX_BUFFERHEADERTYPE);
    pBufferHeader->pBuffer = (OMX_U8 *)malloc(nSizeBytes + EXTRA_BUFFBYTES);
    memset(pBufferHeader->pBuffer, 0x0, nSizeBytes + EXTRA_BUFFBYTES);
    G729DEC_MEMPRINT("%d :: [ALLOC]  %p\n",__LINE__,pBufferHeader->pBuffer);

    if (pBufferHeader->pBuffer == NULL) {
        /* Free previously allocated memory before bailing */
        if (pBufferHeader) {
            free(pBufferHeader);
            pBufferHeader = NULL;
        }
        eError = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    pBufferHeader->pBuffer += CACHE_ALIGNMENT;

    if (nPortIndex == G729DEC_INPUT_PORT) {
        pBufferHeader->nInputPortIndex = nPortIndex;
        pBufferHeader->nOutputPortIndex = -1; 
        pComponentPrivate->pInputBufferList->pBufHdr[pComponentPrivate->pInputBufferList->numBuffers] = pBufferHeader;
        pComponentPrivate->pInputBufferList->bBufferPending[pComponentPrivate->pInputBufferList->numBuffers] = 0;
        G729DEC_DPRINT("pComponentPrivate->pInputBufferList->pBufHdr[%d] = %p\n",pComponentPrivate->pInputBufferList->numBuffers,pComponentPrivate->pInputBufferList->pBufHdr[pComponentPrivate->pInputBufferList->numBuffers]);
        pComponentPrivate->pInputBufferList->bufferOwner[pComponentPrivate->pInputBufferList->numBuffers++] = 1;
        if (pComponentPrivate->pInputBufferList->numBuffers == pPortDef->nBufferCountActual) {
            G729DEC_DPRINT("Setting pPortDef->bPopulated = OMX_TRUE for input port\n");
            pPortDef->bPopulated = OMX_TRUE;
        }
    }
    else if (nPortIndex == G729DEC_OUTPUT_PORT) {
        pBufferHeader->nInputPortIndex = -1;
        pBufferHeader->nOutputPortIndex = nPortIndex; 
        pComponentPrivate->pOutputBufferList->pBufHdr[pComponentPrivate->pOutputBufferList->numBuffers] = pBufferHeader;
        pComponentPrivate->pOutputBufferList->bBufferPending[pComponentPrivate->pOutputBufferList->numBuffers] = 0;
        pComponentPrivate->pOutputBufferList->bufferOwner[pComponentPrivate->pOutputBufferList->numBuffers++] = 1;
        if (pComponentPrivate->pOutputBufferList->numBuffers == pPortDef->nBufferCountActual) {
            G729DEC_DPRINT("Setting pPortDef->bPopulated = OMX_TRUE for input port\n");
            pPortDef->bPopulated = OMX_TRUE;
        }
    }
    else {
        eError = OMX_ErrorBadPortIndex;
        goto EXIT;
    }

    if((pComponentPrivate->pPortDef[G729DEC_OUTPUT_PORT]->bPopulated == pComponentPrivate->pPortDef[G729DEC_OUTPUT_PORT]->bEnabled)&&
       (pComponentPrivate->pPortDef[G729DEC_INPUT_PORT]->bPopulated == pComponentPrivate->pPortDef[G729DEC_INPUT_PORT]->bEnabled) &&
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
    OMX_G729MALLOC_STRUCT(pBufferHeader->pInputPortPrivate, G729DEC_BufParamStruct);
    ((G729DEC_BufParamStruct*)pBufferHeader->pInputPortPrivate)->bNoUseDefaults = OMX_FALSE; /* setting a flag to use defaults until client says otherwise */
    pBufferHeader->pPlatformPrivate = pComponentPrivate;
    pBufferHeader->nAllocLen = nSizeBytes;
    pBufferHeader->nVersion.s.nVersionMajor = G729DEC_MAJOR_VER;
    pBufferHeader->nVersion.s.nVersionMinor = G729DEC_MINOR_VER;
    pComponentPrivate->nVersion = pBufferHeader->nVersion.nVersion;
    pBufferHeader->nSize = sizeof(OMX_BUFFERHEADERTYPE);
    *pBuffer = pBufferHeader;

#ifdef __PERF_INSTRUMENTATION__
    PERF_ReceivedBuffer(pComponentPrivate->pPERF,
                        (*pBuffer)->pBuffer, nSizeBytes,
                        PERF_ModuleMemory);
#endif

    if (pComponentPrivate->bEnableCommandPending && pPortDef->bPopulated) {
        SendCommand (pComponentPrivate->pHandle,
                     OMX_CommandPortEnable,
                     pComponentPrivate->bEnableCommandParam,NULL);
    }

 EXIT:
    G729DEC_DPRINT("AllocateBuffer returning %d\n",eError);
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
    G729DEC_COMPONENT_PRIVATE * pComponentPrivate = NULL;
    OMX_BUFFERHEADERTYPE* buff = NULL;
    OMX_U8* tempBuff = NULL;
    OMX_S16 i =0;
    OMX_S16 inputIndex = -1;
    OMX_S16 outputIndex = -1;
    OMX_COMPONENTTYPE *pHandle = NULL;


    pComponentPrivate = (G729DEC_COMPONENT_PRIVATE *)
        (((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);
    pHandle = (OMX_COMPONENTTYPE *) pComponentPrivate->pHandle;
    G729DEC_DPRINT("Entering FreeBuffer\n");
    if (nPortIndex == G729DEC_INPUT_PORT){
        for (i=0; i < MAX_NUM_OF_BUFS; i++)
        {
            buff = pComponentPrivate->pInputBufferList->pBufHdr[i];
            if (buff == pBuffer)
            {
                G729DEC_DPRINT("Found matching input buffer\n");
                G729DEC_DPRINT("buff = %p\n", buff);
                G729DEC_DPRINT("pBuffer = %p\n", pBuffer);
                inputIndex = i;
                break;
            }
            else
            {
                G729DEC_DPRINT("This is not a match\n");
                G729DEC_DPRINT("buff = %p\n", buff);
                G729DEC_DPRINT("pBuffer = %p\n", pBuffer);
            }
        }
    }
    else if (nPortIndex == G729DEC_OUTPUT_PORT){
        for (i=0; i < MAX_NUM_OF_BUFS; i++)
        {
            buff = pComponentPrivate->pOutputBufferList->pBufHdr[i];
            if (buff == pBuffer)
            {
                G729DEC_DPRINT("Found matching output buffer\n");
                G729DEC_DPRINT("buff = %p\n", buff);
                G729DEC_DPRINT("pBuffer = %p\n", pBuffer);
                outputIndex = i;
                break;
            }
            else
            {
                G729DEC_DPRINT("This is not a match\n");
                G729DEC_DPRINT("buff = %p\n", buff);
                G729DEC_DPRINT("pBuffer = %p\n", pBuffer);
            }
        }
    }
    if (inputIndex != -1)
    {
        if (pComponentPrivate->pInputBufferList->bufferOwner[inputIndex] == 1)
        {
#ifdef __PERF_INSTRUMENTATION__
            PERF_SendingBuffer(pComponentPrivate->pPERF,
                               pComponentPrivate->pInputBufferList->pBufHdr[inputIndex]->pBuffer, 
                               pComponentPrivate->pInputBufferList->pBufHdr[inputIndex]->nAllocLen,
                               PERF_ModuleMemory );
#endif
            tempBuff = pComponentPrivate->pInputBufferList->pBufHdr[inputIndex]->pBuffer;
            if (tempBuff != NULL)
            {
                tempBuff -= CACHE_ALIGNMENT;
                OMX_G729MEMFREE_STRUCT(tempBuff);
            }
            OMX_G729MEMFREE_STRUCT(pComponentPrivate->pInputBufferList->pBufHdr[inputIndex]->pInputPortPrivate);
        }
        OMX_G729MEMFREE_STRUCT(pComponentPrivate->pInputBufferList->pBufHdr[inputIndex]);
        pComponentPrivate->pInputBufferList->numBuffers--;
        if (pComponentPrivate->pInputBufferList->numBuffers <
            pComponentPrivate->pPortDef[G729DEC_INPUT_PORT]->nBufferCountMin)
        {
            pComponentPrivate->pPortDef[G729DEC_INPUT_PORT]->bPopulated = OMX_FALSE;
        }
        if(pComponentPrivate->pPortDef[G729DEC_INPUT_PORT]->bEnabled &&
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
#ifdef __PERF_INSTRUMENTATION__
            PERF_SendingBuffer(pComponentPrivate->pPERF,
                               pComponentPrivate->pOutputBufferList->pBufHdr[outputIndex]->pBuffer, 
                               pComponentPrivate->pOutputBufferList->pBufHdr[outputIndex]->nAllocLen,
                               PERF_ModuleMemory );
#endif         
            tempBuff = pComponentPrivate->pOutputBufferList->pBufHdr[outputIndex]->pBuffer;
            if (tempBuff != NULL)
            {
                tempBuff -= CACHE_ALIGNMENT;
                OMX_G729MEMFREE_STRUCT(tempBuff);
            }
            OMX_G729MEMFREE_STRUCT(pComponentPrivate->pOutputBufferList->pBufHdr[outputIndex]->pInputPortPrivate);
        }
        OMX_G729MEMFREE_STRUCT(pComponentPrivate->pOutputBufferList->pBufHdr[outputIndex]);

        pComponentPrivate->pOutputBufferList->numBuffers--;
        if (pComponentPrivate->pOutputBufferList->numBuffers <
            pComponentPrivate->pPortDef[G729DEC_OUTPUT_PORT]->nBufferCountMin)
        {
            pComponentPrivate->pPortDef[G729DEC_OUTPUT_PORT]->bPopulated = OMX_FALSE;
        }
        if(pComponentPrivate->pPortDef[G729DEC_OUTPUT_PORT]->bEnabled &&
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
        G729DEC_EPRINT("OMX_ErrorBadParameter.\n");
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
    if (pComponentPrivate->bDisableCommandPending &&(pComponentPrivate->pInputBufferList->numBuffers + pComponentPrivate->pOutputBufferList->numBuffers == 0))
    {
        SendCommand (pComponentPrivate->pHandle, OMX_CommandPortDisable,
                     pComponentPrivate->bDisableCommandParam,NULL);
    }
    G729DEC_DPRINT("Exiting. Returning = 0x%x\n", eError);
    return eError;    

}


static OMX_ERRORTYPE UseBuffer (
                                OMX_IN OMX_HANDLETYPE hComponent,
                                OMX_INOUT OMX_BUFFERHEADERTYPE** ppBufferHdr,
                                OMX_IN OMX_U32 nPortIndex,
                                OMX_IN OMX_PTR pAppPrivate,
                                OMX_IN OMX_U32 nSizeBytes,
                                OMX_IN OMX_U8* pBuffer)
{
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef = NULL;
    G729DEC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_BUFFERHEADERTYPE *pBufferHeader = NULL;

    pComponentPrivate = (G729DEC_COMPONENT_PRIVATE *)
        (((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);

    pPortDef = ((G729DEC_COMPONENT_PRIVATE*) 
                pComponentPrivate)->pPortDef[nPortIndex];
    G729DEC_DPRINT("pPortDef->bPopulated = %d\n",pPortDef->bPopulated);
    if(!pPortDef->bEnabled) 
    {
        G729DEC_DPRINT ("%d :: In AllocateBuffer\n", __LINE__);
        eError = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }
    if(nSizeBytes != pPortDef->nBufferSize || pPortDef->bPopulated)
    {
        G729DEC_DPRINT ("%d :: In AllocateBuffer\n", __LINE__);
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }
    OMX_G729MALLOC_STRUCT(pBufferHeader, OMX_BUFFERHEADERTYPE);
    if (nPortIndex == G729DEC_OUTPUT_PORT) {
        pBufferHeader->nInputPortIndex = -1;
        pBufferHeader->nOutputPortIndex = nPortIndex; 
        pComponentPrivate->pOutputBufferList->pBufHdr[pComponentPrivate->pOutputBufferList->numBuffers] = pBufferHeader;
        pComponentPrivate->pOutputBufferList->bBufferPending[pComponentPrivate->pOutputBufferList->numBuffers] = 0;
        pComponentPrivate->pOutputBufferList->bufferOwner[pComponentPrivate->pOutputBufferList->numBuffers++] = 0;
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

    if((pComponentPrivate->pPortDef[G729DEC_OUTPUT_PORT]->bPopulated == pComponentPrivate->pPortDef[G729DEC_OUTPUT_PORT]->bEnabled)&&
       (pComponentPrivate->pPortDef[G729DEC_INPUT_PORT]->bPopulated == pComponentPrivate->pPortDef[G729DEC_INPUT_PORT]->bEnabled) &&
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
    OMX_G729MALLOC_STRUCT(pBufferHeader->pInputPortPrivate, G729DEC_BufParamStruct);
    ((G729DEC_BufParamStruct*)pBufferHeader->pInputPortPrivate)->bNoUseDefaults = OMX_FALSE; /* setting a flag to use defaults until client says otherwise */
    pBufferHeader->pPlatformPrivate = pComponentPrivate;
    pBufferHeader->nAllocLen = nSizeBytes;
    pBufferHeader->nVersion.s.nVersionMajor = G729DEC_MAJOR_VER;
    pBufferHeader->nVersion.s.nVersionMinor = G729DEC_MINOR_VER;
    pComponentPrivate->nVersion = pBufferHeader->nVersion.nVersion;
    pBufferHeader->pBuffer = pBuffer;
    pBufferHeader->nSize = sizeof(OMX_BUFFERHEADERTYPE);
    *ppBufferHdr = pBufferHeader;
    G729DEC_DPRINT("pBufferHeader = %p\n",pBufferHeader);
    if (pComponentPrivate->bEnableCommandPending && pPortDef->bPopulated) {
        SendCommand (pComponentPrivate->pHandle,
                     OMX_CommandPortEnable,
                     pComponentPrivate->bEnableCommandParam,NULL);
    }
 EXIT:
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
                                       OMX_OUT OMX_INDEXTYPE *pIndexType)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    if(!(strcmp(cParameterName,"OMX.TI.index.config.g729headerinfo")))
    {
        *pIndexType = OMX_IndexCustomG729DecHeaderInfoConfig;
    }
    else if(!(strcmp(cParameterName,"OMX.TI.index.config.g729.datapath"))) 
    {
        *pIndexType = OMX_IndexCustomG729DecDataPath;
    }
         
    else {
        eError = OMX_ErrorBadParameter;
    }
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
