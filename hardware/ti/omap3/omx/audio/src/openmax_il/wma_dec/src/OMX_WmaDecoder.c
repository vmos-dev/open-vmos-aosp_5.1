
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

/* ==============================================================================
 *             Texas Instruments OMAP (TM) Platform Software
 *  (c) Copyright Texas Instruments, Incorporated.  All Rights Reserved.
 *
 *  Use of this software is controlled by the terms and conditions found
 *  in the license agreement under which this software has been supplied.
 * ============================================================================ */
/**
 * @file OMX_WmaDecoder.c
 *
 * This file implements OMX Component for WMA decoder that
 * is fully compliant with the OMX Audio specification 1.5.
 *
 * @path  $(CSLPATH)\
 *
 * @rev  0.1
 */
/* ----------------------------------------------------------------------------
 *!
 *! Revision History
 *! ===================================
 *! 12-Sept-2005 mf:  Initial Version. Change required per OMAPSWxxxxxxxxx
 *! to provide _________________.
 *!
 * ============================================================================= */


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
#include "OMX_WmaDec_Utils.h"
/* interface with audio manager */
#define FIFO1 "/dev/fifo.1"
#define FIFO2 "/dev/fifo.2"

#ifdef DSP_RENDERING_ON
AM_COMMANDDATATYPE cmd_data;
int fdwrite, fdread;
int errno;
OMX_U32 streamID;
#endif
#define WMA_DEC_ROLE "audio_decoder.wma"

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
                                       OMX_OUT OMX_INDEXTYPE* pIndexType);

static OMX_ERRORTYPE ComponentRoleEnum(
                                       OMX_IN OMX_HANDLETYPE hComponent,
                                       OMX_OUT OMX_U8 *cRole,
                                       OMX_IN OMX_U32 nIndex);

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
    WMADEC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    OMX_AUDIO_PARAM_WMATYPE *wma_ip = NULL; 
    OMX_AUDIO_PARAM_PCMMODETYPE *wma_op = NULL;
    RCA_HEADER *rcaheader=NULL;
      
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE*) hComp;
    int i;

    OMXDBG_PRINT(stderr, PRINT, 1, 0, "%d ::OMX_ComponentInit\n", __LINE__);

    /*Set the all component function pointer to the handle */
    pHandle->SetCallbacks = SetCallbacks;
    pHandle->GetComponentVersion = GetComponentVersion;
    pHandle->SendCommand = SendCommand;
    pHandle->GetParameter = GetParameter;
    pHandle->SetParameter = SetParameter;
    pHandle->GetConfig = GetConfig;
    pHandle->GetState = GetState;
    pHandle->EmptyThisBuffer = EmptyThisBuffer;
    pHandle->FillThisBuffer = FillThisBuffer;
    pHandle->ComponentTunnelRequest = ComponentTunnelRequest;
    pHandle->ComponentDeInit = ComponentDeInit;
    pHandle->AllocateBuffer = AllocateBuffer;
    pHandle->FreeBuffer = FreeBuffer;
    pHandle->UseBuffer = UseBuffer;
    pHandle->SetConfig = SetConfig;
    pHandle->GetExtensionIndex = GetExtensionIndex;
    pHandle->ComponentRoleEnum = ComponentRoleEnum;
  

 
    /*Allocate the memory for Component private data area */
    OMX_MALLOC_GENERIC(pHandle->pComponentPrivate, WMADEC_COMPONENT_PRIVATE);
    ((WMADEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate)->pHandle = pHandle;

    /* Initialize component data structures to default values */
    ((WMADEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate)->sPortParam.nPorts = 0x2;
    ((WMADEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate)->sPortParam.nStartPortNumber = 0x0;

    eError = OMX_ErrorNone;
 
    OMX_MALLOC_GENERIC(wma_ip, OMX_AUDIO_PARAM_WMATYPE);
    OMX_MALLOC_GENERIC(wma_op, OMX_AUDIO_PARAM_PCMMODETYPE);
    OMX_MALLOC_GENERIC(rcaheader, RCA_HEADER);
    
    ((WMADEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate)->wma_op=wma_op;
    ((WMADEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate)->rcaheader=rcaheader;

    ((WMADEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate)->wmaParams[INPUT_PORT] = wma_ip;
    ((WMADEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate)->wmaParams[OUTPUT_PORT] = (OMX_AUDIO_PARAM_WMATYPE*)wma_op;

 
    pComponentPrivate = pHandle->pComponentPrivate;
    OMX_MALLOC_GENERIC(pComponentPrivate->pInputBufferList, BUFFERLIST);
    OMX_DBG_INIT(pComponentPrivate->dbg, "OMX_DBG_WMADEC");
    
#ifdef __PERF_INSTRUMENTATION__
    OMX_PRDSP1(pComponentPrivate->dbg, "PERF %d :: OMX_WmaDecoder.c\n", __LINE__);
    pComponentPrivate->pPERF = PERF_Create(PERF_FOURCC('W','M','A','_'),
                                           PERF_ModuleLLMM |
                                           PERF_ModuleAudioDecode);
#endif

#ifdef ANDROID 
    pComponentPrivate->iPVCapabilityFlags.iIsOMXComponentMultiThreaded = OMX_TRUE;
    pComponentPrivate->iPVCapabilityFlags.iOMXComponentNeedsNALStartCode = OMX_FALSE;
    pComponentPrivate->iPVCapabilityFlags.iOMXComponentSupportsExternalOutputBufferAlloc = OMX_FALSE;
    pComponentPrivate->iPVCapabilityFlags.iOMXComponentSupportsExternalInputBufferAlloc = OMX_FALSE;
    pComponentPrivate->iPVCapabilityFlags.iOMXComponentSupportsMovableInputBuffers = OMX_FALSE; 
    pComponentPrivate->iPVCapabilityFlags.iOMXComponentSupportsPartialFrames = OMX_FALSE; 
    pComponentPrivate->iPVCapabilityFlags.iOMXComponentCanHandleIncompleteFrames = OMX_FALSE; 
#endif
    
    pComponentPrivate->pInputBufferList->numBuffers = 0; /* initialize number of buffers */
    pComponentPrivate->bDspStoppedWhileExecuting = OMX_FALSE;
    OMX_MALLOC_GENERIC(pComponentPrivate->pOutputBufferList, BUFFERLIST);
 
    pComponentPrivate->pOutputBufferList->numBuffers = 0; /* initialize number of buffers */
    OMX_MALLOC_GENERIC(pComponentPrivate->pHeaderInfo, WMA_HeadInfo);
    OMX_MALLOC_GENERIC(pComponentPrivate->pDspDefinition, TI_OMX_DSP_DEFINITION);
 
    /* Temporarily treat these as default values for Khronos tests */
    pComponentPrivate->pHeaderInfo->iPackets.dwLo           =   178 ;
    pComponentPrivate->pHeaderInfo->iPlayDuration.dwHi      =   0   ;
    pComponentPrivate->pHeaderInfo->iPlayDuration.dwLo      =   917760000 ;
    pComponentPrivate->pHeaderInfo->iMaxPacketSize          =   349 ;
    pComponentPrivate->pHeaderInfo->iStreamType.Data1       =   -127295936 ;
    pComponentPrivate->pHeaderInfo->iStreamType.Data2       =   23373 ;
    pComponentPrivate->pHeaderInfo->iStreamType.Data3       =   4559 ;
    pComponentPrivate->pHeaderInfo->iStreamType.Data4[0]    =   168 ;
    pComponentPrivate->pHeaderInfo->iStreamType.Data4[1]    =   253 ;
    pComponentPrivate->pHeaderInfo->iStreamType.Data4[2]    =   0   ;
    pComponentPrivate->pHeaderInfo->iStreamType.Data4[3]    =   128 ;
    pComponentPrivate->pHeaderInfo->iStreamType.Data4[4]    =   95  ;
    pComponentPrivate->pHeaderInfo->iStreamType.Data4[5]    =   92  ;
    pComponentPrivate->pHeaderInfo->iStreamType.Data4[6]    =   68  ;
    pComponentPrivate->pHeaderInfo->iStreamType.Data4[7]    =   43  ;
    pComponentPrivate->pHeaderInfo->iTypeSpecific           =   28  ;
    pComponentPrivate->pHeaderInfo->iStreamNum              =   1   ;
    pComponentPrivate->pHeaderInfo->iFormatTag              =   353 ;
    pComponentPrivate->pHeaderInfo->iBlockAlign             =   40  ;
    pComponentPrivate->pHeaderInfo->iSamplePerSec           =   8000;
    pComponentPrivate->pHeaderInfo->iAvgBytesPerSec         =   625 ;
    pComponentPrivate->pHeaderInfo->iChannel                =   1   ;
    pComponentPrivate->pHeaderInfo->iValidBitsPerSample     =   16  ;
    pComponentPrivate->pHeaderInfo->iSizeWaveHeader         =   10  ;
    pComponentPrivate->pHeaderInfo->iEncodeOptV             =   0   ;
    pComponentPrivate->pHeaderInfo->iValidBitsPerSample     =   16  ;
    pComponentPrivate->pHeaderInfo->iChannelMask            =   0   ;
    pComponentPrivate->pHeaderInfo->iSamplePerBlock         =   8704;
    

    pComponentPrivate->bConfigData = 0;  /* assume the first buffer received will contain only config data, need to use bufferFlag instead */
    pComponentPrivate->reconfigInputPort = 0; 
    pComponentPrivate->reconfigOutputPort = 0;
     
    for (i=0; i < MAX_NUM_OF_BUFS; i++) {
        pComponentPrivate->pOutputBufferList->pBufHdr[i] = NULL;
        pComponentPrivate->pInputBufferList->pBufHdr[i] = NULL;
    }
    pComponentPrivate->dasfmode = 0;
    pComponentPrivate->bPortDefsAllocated = 0;
    pComponentPrivate->bCompThreadStarted = 0;
    pComponentPrivate->bInitParamsInitialized = 0;
    pComponentPrivate->pMarkBuf = NULL;
    pComponentPrivate->pMarkData = NULL;
    pComponentPrivate->nEmptyBufferDoneCount = 0;
    pComponentPrivate->nEmptyThisBufferCount = 0;
    pComponentPrivate->strmAttr = NULL;
    pComponentPrivate->bDisableCommandParam = 0;
    pComponentPrivate->bEnableCommandParam = 0;
    
    pComponentPrivate->nUnhandledFillThisBuffers=0;
    pComponentPrivate->nUnhandledEmptyThisBuffers = 0;
    pComponentPrivate->SendAfterEOS = 0;
    
    pComponentPrivate->bFlushOutputPortCommandPending = OMX_FALSE;
    pComponentPrivate->bFlushInputPortCommandPending = OMX_FALSE;
    

    
    for (i=0; i < MAX_NUM_OF_BUFS; i++) {
        pComponentPrivate->pInputBufHdrPending[i] = NULL;
        pComponentPrivate->pOutputBufHdrPending[i] = NULL;
    }
    pComponentPrivate->nInvalidFrameCount = 0;
    pComponentPrivate->nNumInputBufPending = 0;
    pComponentPrivate->nNumOutputBufPending = 0;
    pComponentPrivate->bDisableCommandPending = 0;
    pComponentPrivate->bEnableCommandPending = 0;
    pComponentPrivate->bBypassDSP = OMX_FALSE;
    pComponentPrivate->bNoIdleOnStop= OMX_FALSE;
    pComponentPrivate->bIdleCommandPending = OMX_FALSE;
    pComponentPrivate->nOutStandingFillDones = 0;
    
    pComponentPrivate->sInPortFormat.eEncoding = OMX_AUDIO_CodingWMA;
    pComponentPrivate->sInPortFormat.nIndex = 0;
    pComponentPrivate->sInPortFormat.nPortIndex = INPUT_PORT;
    pComponentPrivate->bPreempted = OMX_FALSE; 
    
    pComponentPrivate->sOutPortFormat.eEncoding = OMX_AUDIO_CodingPCM;  /*chrisk*/
    pComponentPrivate->sOutPortFormat.nIndex = 1;
    pComponentPrivate->sOutPortFormat.nPortIndex = OUTPUT_PORT;
    
    OMX_MALLOC_SIZE(pComponentPrivate->sDeviceString, 100*sizeof(OMX_STRING), OMX_STRING);

    
    /* PCM format defaults */
    wma_op->nPortIndex = 1;
    wma_op->nChannels = 2; 
    wma_op->eNumData= OMX_NumericalDataSigned; 
    wma_op->nBitPerSample = 16;  
    wma_op->nSamplingRate = 44100;          
    wma_op->ePCMMode = OMX_AUDIO_PCMModeLinear; 
    

    /* WMA format defaults */
    wma_ip->nPortIndex = 0;
    wma_ip->nChannels = 2;
    wma_ip->nBitRate = 32000;
    wma_ip->nSamplingRate = 44100;
   
    /* initialize role name */
    strcpy((char *) pComponentPrivate->componentRole.cRole, WMA_DEC_ROLE);
    /* Initialize device string to the default value */
    strcpy((char*)pComponentPrivate->sDeviceString,"/eteedn:i0:o0/codec\0");

    /* Removing sleep() calls. Initialization.*/
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

    pthread_mutex_init(&pComponentPrivate->codecStop_mutex, NULL);
    pthread_cond_init (&pComponentPrivate->codecStop_threshold, NULL);
    pComponentPrivate->codecStop_waitingsignal = 0;

    pthread_mutex_init(&pComponentPrivate->codecFlush_mutex, NULL);
    pthread_cond_init (&pComponentPrivate->codecFlush_threshold, NULL);
    pComponentPrivate->codecFlush_waitingsignal = 0;
#else
    OMX_CreateEvent(&(pComponentPrivate->AlloBuf_event));
    pComponentPrivate->AlloBuf_waitingsignal = 0;
    
    OMX_CreateEvent(&(pComponentPrivate->InLoaded_event));
    pComponentPrivate->InLoaded_readytoidle = 0;
    
    OMX_CreateEvent(&(pComponentPrivate->InIdle_event));
    pComponentPrivate->InIdle_goingtoloaded = 0;
#endif
    /* Removing sleep() calls. Initialization.*/        
    
    OMX_MALLOC_GENERIC(pPortDef_ip, OMX_PARAM_PORTDEFINITIONTYPE);
    OMX_MALLOC_GENERIC(pPortDef_op,OMX_PARAM_PORTDEFINITIONTYPE );

    OMX_PRCOMM2(pComponentPrivate->dbg, "%d ::pPortDef_ip = %p\n", __LINE__,pPortDef_ip);
    OMX_PRCOMM2(pComponentPrivate->dbg, "%d ::pPortDef_op = %p\n", __LINE__,pPortDef_op);

    ((WMADEC_COMPONENT_PRIVATE*) pHandle->pComponentPrivate)->pPortDef[INPUT_PORT]
        = pPortDef_ip;

    ((WMADEC_COMPONENT_PRIVATE*) pHandle->pComponentPrivate)->pPortDef[OUTPUT_PORT]
        = pPortDef_op;

    pPortDef_ip->nPortIndex = 0x0;
    pPortDef_ip->nBufferCountActual = NUM_WMADEC_INPUT_BUFFERS;
    pPortDef_ip->nBufferCountMin = NUM_WMADEC_INPUT_BUFFERS;
    pPortDef_ip->eDir = OMX_DirInput;
    pPortDef_ip->bEnabled = OMX_TRUE;
    pPortDef_ip->nBufferAlignment = DSP_CACHE_ALIGNMENT;
    pPortDef_ip->nBufferSize = INPUT_WMADEC_BUFFER_SIZE;
    pPortDef_ip->bPopulated = 0;
    pPortDef_ip->eDomain = OMX_PortDomainAudio;
    pPortDef_ip->format.audio.eEncoding = OMX_AUDIO_CodingWMA;
    
    pPortDef_op->nPortIndex = 0x1;
    pPortDef_op->nBufferCountActual = NUM_WMADEC_OUTPUT_BUFFERS;
    pPortDef_op->nBufferCountMin = NUM_WMADEC_OUTPUT_BUFFERS;
    pPortDef_op->nBufferAlignment = DSP_CACHE_ALIGNMENT;
    pPortDef_op->eDir = OMX_DirOutput;
    pPortDef_op->bEnabled = OMX_TRUE;
    if(pComponentPrivate->pPortDef[OUTPUT_PORT]->nBufferSize == 0)
    {
        pPortDef_op->nBufferSize = OUTPUT_WMADEC_BUFFER_SIZE;
    }
    else
    {
        pPortDef_op->nBufferSize = pComponentPrivate->pPortDef[OUTPUT_PORT]->nBufferSize;
    }
    pPortDef_op->bPopulated = 0;
    pPortDef_op->eDomain = OMX_PortDomainAudio;
    pComponentPrivate->bIsInvalidState = OMX_FALSE;
    /*    sPortFormat->eEncoding = OMX_AUDIO_CodingPCM; */ /*chrisk*/
    pPortDef_op->format.audio.eEncoding = OMX_AUDIO_CodingPCM;  /*chrisk*/

#ifdef RESOURCE_MANAGER_ENABLED
    OMX_PRCOMM2(pComponentPrivate->dbg, "%d :: Initialize RM Proxy... \n", __LINE__);
    eError = RMProxy_NewInitalize();
    OMX_PRINT2(pComponentPrivate->dbg, "%d ::OMX_ComponentInit\n", __LINE__);
    if (eError != OMX_ErrorNone) {
        OMX_ERROR4(pComponentPrivate->dbg, "%d ::Error returned from loading ResourceManagerProxy thread\n",
                       __LINE__);
        goto EXIT;
    }
#endif  
    OMX_PRINT1(pComponentPrivate->dbg, "%d ::Start Component Thread \n", __LINE__);
    eError = WMADEC_StartComponentThread(pHandle);
    /*OMX_PRINT1(pComponentPrivate->dbg, "%d ::OMX_ComponentInit\n", __LINE__);*/
    if (eError != OMX_ErrorNone) {
        OMX_ERROR4(pComponentPrivate->dbg, "%d ::Error returned from the Component\n",
                       __LINE__);
        goto EXIT;
    }
    OMX_PRINT1(pComponentPrivate->dbg, "%d ::OMX_ComponentInit\n", __LINE__);


  
    
#ifdef DSP_RENDERING_ON
    OMX_PRINT1(pComponentPrivate->dbg, "%d ::OMX_ComponentInit\n", __LINE__);
    if((fdwrite=open(FIFO1,O_WRONLY))<0) {
        OMX_TRACE4(pComponentPrivate->dbg, "[WMA Dec Component] - failure to open WRITE pipe\n");
        eError = OMX_ErrorHardware;
    }

    OMX_PRINT1(pComponentPrivate->dbg, "%d ::OMX_ComponentInit\n", __LINE__);
    if((fdread=open(FIFO2,O_RDONLY))<0) {
        OMX_TRACE4(pComponentPrivate->dbg, "[WMA Dec Component] - failure to open READ pipe\n");
        eError = OMX_ErrorHardware;
    }
    OMX_PRINT1(pComponentPrivate->dbg, "%d ::OMX_ComponentInit\n", __LINE__);

#endif
#ifdef __PERF_INSTRUMENTATION__
    OMX_PRINT1(pComponentPrivate->dbg, "PERF %d :: OMX_WmaDecoder.c\n",__LINE__);
    PERF_ThreadCreated(pComponentPrivate->pPERF, pComponentPrivate->ComponentThread,
                       PERF_FOURCC('W','M','A','T'));
#endif
 EXIT:


    
    OMX_PRINT2(pComponentPrivate->dbg, "%d ::OMX_ComponentInit - returning %d\n", __LINE__,eError);
    if(eError == OMX_ErrorInsufficientResources)
    {
        OMX_MEMFREE_STRUCT(pPortDef_op);
        OMX_MEMFREE_STRUCT(pPortDef_ip);
        OMX_MEMFREE_STRUCT(pComponentPrivate->sDeviceString);
        OMX_MEMFREE_STRUCT(pComponentPrivate->pDspDefinition);
        OMX_MEMFREE_STRUCT(pComponentPrivate->pHeaderInfo);
        OMX_MEMFREE_STRUCT(pComponentPrivate->pOutputBufferList); 
        OMX_MEMFREE_STRUCT(pComponentPrivate->pInputBufferList);
        OMX_MEMFREE_STRUCT(wma_op);
        OMX_MEMFREE_STRUCT(wma_ip);
        OMX_MEMFREE_STRUCT(pHandle->pComponentPrivate);
        
    }
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

    WMADEC_COMPONENT_PRIVATE *pComponentPrivate =
        (WMADEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;

    if (pCallBacks == NULL) {
        OMX_PRINT1(pComponentPrivate->dbg, "About to return OMX_ErrorBadParameter on line %d\n",__LINE__);
        eError = OMX_ErrorBadParameter;
        OMX_ERROR4(pComponentPrivate->dbg, "%d :: Received the empty callbacks from the \
                application\n",__LINE__);
        goto EXIT;
    }

    /*Copy the callbacks of the application to the component private */
    memcpy (&(pComponentPrivate->cbInfo), pCallBacks, sizeof(OMX_CALLBACKTYPE));

    /*copy the application private data to component memory */
    pHandle->pApplicationPrivate = pAppData;

    pComponentPrivate->curState = OMX_StateLoaded;
#ifdef __PERF_INSTRUMENTATION__
    OMX_PRDSP1(pComponentPrivate->dbg, "PERF %d :: OMX_WmaDecoder.c\n",__LINE__);
    /*               PERF_Boundary(pComponentPrivate->pPERFcomp,PERF_BoundaryComplete | PERF_BoundaryCleanup);*/
#endif
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

    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE*) hComp;
    WMADEC_COMPONENT_PRIVATE *pComponentPrivate = (WMADEC_COMPONENT_PRIVATE *) pHandle->pComponentPrivate;
    eError = OMX_ErrorNotImplemented;
#ifdef _ERROR_PROPAGATION__
    if (pComponentPrivate->curState == OMX_StateInvalid){
        eError = OMX_ErrorInvalidState;
        goto EXIT;
    }   
#endif

 EXIT:
    OMX_PRINT1(pComponentPrivate->dbg, "Inside the GetComponentVersion\n");
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
    int nRet;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)phandle;
    WMADEC_COMPONENT_PRIVATE *pCompPrivate =
        (WMADEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;

#ifdef _ERROR_PROPAGATION__
    if (pCompPrivate->curState == OMX_StateInvalid){
        eError = OMX_ErrorInvalidState;
        goto EXIT;
    }   
#else
    OMX_PRINT1(pCompPrivate->dbg, "%d:::Inside SendCommand\n",__LINE__);
    if(pCompPrivate->curState == OMX_StateInvalid){
        eError = OMX_ErrorInvalidState;
        OMX_ERROR4(pCompPrivate->dbg, "%d :: WMADEC: Error Notofication Sent to App\n",__LINE__);
        pCompPrivate->cbInfo.EventHandler (
                                           pHandle, pHandle->pApplicationPrivate,
                                           OMX_EventError, OMX_ErrorInvalidState,0,
                                           "Invalid State");

        goto EXIT;
    }
#endif

#ifdef __PERF_INSTRUMENTATION__
    OMX_PRDSP1(pCompPrivate->dbg, "PERF %d :: OMX_WmaDecoder.c\n",__LINE__);
    PERF_SendingCommand(pCompPrivate->pPERF,
                        Cmd,
                        (Cmd == OMX_CommandMarkBuffer) ? ((OMX_U32) pCmdData) : nParam,
                        PERF_ModuleComponent);
#endif

    switch(Cmd) {
    case OMX_CommandStateSet:
        OMX_PRINT1(pCompPrivate->dbg, "%d:::Inside SendCommand\n",__LINE__);
        OMX_PRSTATE2(pCompPrivate->dbg, "%d:::pCompPrivate->curState = %d\n",__LINE__,pCompPrivate->curState);
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
                OMX_PRINT1(pCompPrivate->dbg, "%d:::Inside SendCommand\n",__LINE__);
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
        if(nParam > 1 && nParam != -1) {
            eError = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        break;
    case OMX_CommandPortDisable:
        break;
    case OMX_CommandPortEnable:
        break;
    case OMX_CommandMarkBuffer:
        if (nParam > 0) {
            eError = OMX_ErrorBadPortIndex;
            goto EXIT;
        }
        break;
    default:
        OMX_PRDSP2(pCompPrivate->dbg, "%d :: WMADEC: Command Received Default \
                                                      error\n",__LINE__);
        pCompPrivate->cbInfo.EventHandler (
                                           pHandle, pHandle->pApplicationPrivate,
                                           OMX_EventError,
                                           OMX_ErrorUndefined,0,
                                           "Invalid Command");
        break;

    }

    OMX_PRINT1(pCompPrivate->dbg, "%d:::Inside SendCommand\n",__LINE__);
    nRet = write (pCompPrivate->cmdPipe[1], &Cmd, sizeof(Cmd));
    if (nRet == -1) {
        OMX_PRINT1(pCompPrivate->dbg, "%d:::Inside SendCommand\n",__LINE__);
        eError = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    if (Cmd == OMX_CommandMarkBuffer) {
        nRet = write(pCompPrivate->cmdDataPipe[1],&pCmdData,sizeof(OMX_PTR));
    }
    else {
        nRet = write(pCompPrivate->cmdDataPipe[1], &nParam,
                     sizeof(OMX_U32));
    }

    OMX_PRINT1(pCompPrivate->dbg, "%d:::Inside SendCommand\n",__LINE__);
    OMX_PRINT2(pCompPrivate->dbg, "%d:::nRet = %d\n",__LINE__,nRet);
    if (nRet == -1) {
        OMX_ERROR4(pCompPrivate->dbg, "%d:::Inside SendCommand\n",__LINE__);
        eError = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
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
    WMADEC_COMPONENT_PRIVATE  *pComponentPrivate;
    OMX_PARAM_PORTDEFINITIONTYPE *pParameterStructure;
    /*OMX_PARAM_BUFFERSUPPLIERTYPE *pBufferSupplier;*/

    pComponentPrivate = (WMADEC_COMPONENT_PRIVATE *)(((OMX_COMPONENTTYPE*)hComp)->pComponentPrivate);
    pParameterStructure = (OMX_PARAM_PORTDEFINITIONTYPE*)ComponentParameterStructure;

    if (pParameterStructure == NULL) {
        eError = OMX_ErrorBadParameter;
        goto EXIT;

    }
    OMX_PRINT1(pComponentPrivate->dbg, "pParameterStructure = %p\n",pParameterStructure);
#ifdef _ERROR_PROPAGATION__
    if (pComponentPrivate->curState == OMX_StateInvalid){
        eError = OMX_ErrorInvalidState;
        goto EXIT;
    }   
#else
    if(pComponentPrivate->curState == OMX_StateInvalid) {
        pComponentPrivate->cbInfo.EventHandler(
                                               hComp,
                                               ((OMX_COMPONENTTYPE *)hComp)->pApplicationPrivate,
                                               OMX_EventError,
                                               OMX_ErrorIncorrectStateOperation, 
                                               0,
                                               NULL);
    
    }
#endif
    switch(nParamIndex){
    case OMX_IndexParamAudioInit:
    
        OMX_PRINT1(pComponentPrivate->dbg, "OMX_IndexParamAudioInit\n");
        memcpy(ComponentParameterStructure, &pComponentPrivate->sPortParam, sizeof(OMX_PORT_PARAM_TYPE));
        break;
    case OMX_IndexParamPortDefinition:
        if(((OMX_PARAM_PORTDEFINITIONTYPE *)(ComponentParameterStructure))->nPortIndex == 
           pComponentPrivate->pPortDef[INPUT_PORT]->nPortIndex) {
    

            memcpy(ComponentParameterStructure,
                   pComponentPrivate->pPortDef[INPUT_PORT], 
                   sizeof(OMX_PARAM_PORTDEFINITIONTYPE)
                   );
        } 
        else if(((OMX_PARAM_PORTDEFINITIONTYPE *)(ComponentParameterStructure))->nPortIndex == 
                pComponentPrivate->pPortDef[OUTPUT_PORT]->nPortIndex) {
    

            memcpy(ComponentParameterStructure, 
                   pComponentPrivate->pPortDef[OUTPUT_PORT], 
                   sizeof(OMX_PARAM_PORTDEFINITIONTYPE)
                   );
 
        } 
        else {
            eError = OMX_ErrorBadPortIndex;
        }
        break;

    case OMX_IndexParamAudioPortFormat:
        if(((OMX_AUDIO_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nPortIndex == 
           pComponentPrivate->pPortDef[INPUT_PORT]->nPortIndex) {

            if(((OMX_AUDIO_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nIndex > 
               pComponentPrivate->sInPortFormat.nIndex) {

                eError = OMX_ErrorNoMore;
            } 
            else {
                memcpy(ComponentParameterStructure, &pComponentPrivate->sInPortFormat, 
                       sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
            }
        }
        else if(((OMX_AUDIO_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nPortIndex == 
                pComponentPrivate->pPortDef[OUTPUT_PORT]->nPortIndex){

            
            if(((OMX_AUDIO_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nIndex > 
               pComponentPrivate->sOutPortFormat.nIndex) {
                
                eError = OMX_ErrorNoMore;
            } 
            else {
                
                memcpy(ComponentParameterStructure, &pComponentPrivate->sOutPortFormat, 
                       sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
            }
        } 
        else {
            
            eError = OMX_ErrorBadPortIndex;
        }
        break;

    case OMX_IndexParamPriorityMgmt:
        break;
    case OMX_IndexParamAudioWma:
        OMX_PRINT2(pComponentPrivate->dbg, "%d :: GetParameter OMX_IndexParamAudioWma \n",__LINE__);
        OMX_PRCOMM2(pComponentPrivate->dbg, "%d :: GetParameter nPortIndex %ld\n",__LINE__, ((OMX_AUDIO_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nPortIndex);
        OMX_PRCOMM2(pComponentPrivate->dbg, "%d :: GetParameter wmaParams->nPortIndex %ld\n",__LINE__, pComponentPrivate->wmaParams[INPUT_PORT]->nPortIndex);
        if(((OMX_AUDIO_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nPortIndex ==
           pComponentPrivate->wmaParams[INPUT_PORT]->nPortIndex) 
        {
            memcpy(ComponentParameterStructure, pComponentPrivate->wmaParams[INPUT_PORT], sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
        } 
        else if(((OMX_AUDIO_PARAM_PORTFORMATTYPE*)(ComponentParameterStructure))->nPortIndex ==
                pComponentPrivate->wmaParams[OUTPUT_PORT]->nPortIndex)
        {
            memcpy(ComponentParameterStructure, pComponentPrivate->wmaParams[OUTPUT_PORT], sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));

        }
        else
        {

            eError = OMX_ErrorBadPortIndex;
        }
        break;
    case OMX_IndexParamAudioPcm:
        memcpy(ComponentParameterStructure, 
               (OMX_AUDIO_PARAM_PCMMODETYPE*)pComponentPrivate->wma_op, 
               sizeof(OMX_AUDIO_PARAM_PCMMODETYPE)
               );                    
        break;
            
    case OMX_IndexParamCompBufferSupplier:
        if(((OMX_PARAM_BUFFERSUPPLIERTYPE *)(ComponentParameterStructure))->nPortIndex == OMX_DirInput) {
            OMX_PRBUFFER2(pComponentPrivate->dbg, ":: GetParameter OMX_IndexParamCompBufferSupplier \n");
            /*  memcpy(ComponentParameterStructure, pBufferSupplier, sizeof(OMX_PARAM_BUFFERSUPPLIERTYPE)); */
                    
        }
        else if(((OMX_PARAM_BUFFERSUPPLIERTYPE *)(ComponentParameterStructure))->nPortIndex == OMX_DirOutput) {
            OMX_PRBUFFER2(pComponentPrivate->dbg, ":: GetParameter OMX_IndexParamCompBufferSupplier \n"); 
            /*memcpy(ComponentParameterStructure, pBufferSupplier, sizeof(OMX_PARAM_BUFFERSUPPLIERTYPE)); */
        } 
        else {
            OMX_ERROR2(pComponentPrivate->dbg, ":: OMX_ErrorBadPortIndex from GetParameter");
            eError = OMX_ErrorBadPortIndex;
        }
        break;

    case OMX_IndexParamVideoInit:
        break;

    case OMX_IndexParamImageInit:
        break;

    case OMX_IndexParamOtherInit:
        /*#ifdef ANDROID
          OMX_PRINT1(pComponentPrivate->dbg, "%d :: Entering OMX_IndexParamVideoInit\n", __LINE__);
          OMX_PRINT1(pComponentPrivate->dbg, "%d :: Entering OMX_IndexParamImageInit/OtherInit\n", __LINE__);
          memcpy(ComponentParameterStructure,pComponentPrivate->sPortParam, sizeof(OMX_PORT_PARAM_TYPE));

        
          eError = OMX_ErrorNone;
          #else
          eError = OMX_ErrorUnsupportedIndex;
          #endif*/
        break;

#ifdef ANDROID
    case (OMX_INDEXTYPE) PV_OMX_COMPONENT_CAPABILITY_TYPE_INDEX:
        {
            OMX_PRDSP1(pComponentPrivate->dbg, "Entering PV_OMX_COMPONENT_CAPABILITY_TYPE_INDEX::%d\n", __LINE__);
            PV_OMXComponentCapabilityFlagsType* pCap_flags = (PV_OMXComponentCapabilityFlagsType *) ComponentParameterStructure;
            if (NULL == pCap_flags)
            {
                OMX_ERROR4(pComponentPrivate->dbg, "%d :: ERROR PV_OMX_COMPONENT_CAPABILITY_TYPE_INDEX\n", __LINE__);
                eError =  OMX_ErrorBadParameter;
                goto EXIT;
            }
            OMX_ERROR2(pComponentPrivate->dbg, "%d :: Copying PV_OMX_COMPONENT_CAPABILITY_TYPE_INDEX\n", __LINE__);
            memcpy(pCap_flags, &(pComponentPrivate->iPVCapabilityFlags), sizeof(PV_OMXComponentCapabilityFlagsType));
            eError = OMX_ErrorNone;
        }
        break;
#endif
    default:
        eError = OMX_ErrorUnsupportedIndex;
        break;
    }
 EXIT:
    OMX_PRINT1(pComponentPrivate->dbg, "%d :: Exiting GetParameter:: %x\n",__LINE__,nParamIndex);
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
    WMADEC_COMPONENT_PRIVATE  *pComponentPrivate;
    OMX_PARAM_PORTDEFINITIONTYPE *pComponentParam = NULL;
    OMX_PARAM_COMPONENTROLETYPE  *pRole;
    OMX_AUDIO_PARAM_PCMMODETYPE *wma_op;
    OMX_PARAM_BUFFERSUPPLIERTYPE sBufferSupplier;       
    pComponentPrivate = (WMADEC_COMPONENT_PRIVATE *)(((OMX_COMPONENTTYPE*)hComp)->pComponentPrivate);



    if (pCompParam == NULL) {
        eError = OMX_ErrorBadParameter;
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
        pComponentParam = (OMX_PARAM_PORTDEFINITIONTYPE *)pCompParam;

        /* 0 means Input port */
        if (pComponentParam->nPortIndex == 0) {
            if (pComponentParam->eDir != OMX_DirInput) {
                OMX_PRBUFFER4(pComponentPrivate->dbg, "%d :: Invalid input buffer Direction\n",__LINE__);
                eError = OMX_ErrorBadParameter;
                OMX_ERROR4(pComponentPrivate->dbg, "About to return OMX_ErrorBadParameter on line %d\n",__LINE__);
                goto EXIT;
            }
               
        } 
        else if (pComponentParam->nPortIndex == 1) {
            /* 1 means Output port */

            if (pComponentParam->eDir != OMX_DirOutput) {
                eError = OMX_ErrorBadParameter;
                OMX_ERROR4(pComponentPrivate->dbg, "About to return OMX_ErrorBadParameter on line %d\n",__LINE__);
                goto EXIT;
            }
            pComponentPrivate->pPortDef[OUTPUT_PORT]->nBufferSize = pComponentParam->nBufferSize;
            /*
              if (pComponentParam->nBufferSize != OUTPUT_WMADEC_BUFFER_SIZE) {
              eError = OMX_ErrorBadParameter;
              OMX_PRINT1(pComponentPrivate->dbg, "About to return OMX_ErrorBadParameter on line %d\n",__LINE__);
              goto EXIT;
              }

              if (pComponentParam->format.audio.eEncoding != OMX_AUDIO_CodingPCM) {
              eError = OMX_ErrorBadParameter;
              goto EXIT;
              } */
        }
        else {
            eError = OMX_ErrorBadParameter;
            goto EXIT;
        }
        break;
    case OMX_IndexParamAudioWma:
        {
            OMX_AUDIO_PARAM_WMATYPE *pCompWmaParam =
                (OMX_AUDIO_PARAM_WMATYPE *)pCompParam;

            /* 0 means Input port */
            if(pCompWmaParam->nPortIndex == 0) {
                OMX_PRCOMM2(pComponentPrivate->dbg, "pCompWmaParam->nPortIndex == 0\n");
                memcpy(((WMADEC_COMPONENT_PRIVATE*)
                        pHandle->pComponentPrivate)->wmaParams[INPUT_PORT],
                       pCompWmaParam, sizeof(OMX_AUDIO_PARAM_WMATYPE));

            } else if (pCompWmaParam->nPortIndex == 1) {
                pComponentPrivate->wmaParams[OUTPUT_PORT]->nSize = pCompWmaParam->nSize;
                pComponentPrivate->wmaParams[OUTPUT_PORT]->nPortIndex = pCompWmaParam->nPortIndex;
                pComponentPrivate->wmaParams[OUTPUT_PORT]->nBitRate = pCompWmaParam->nBitRate;
                pComponentPrivate->wmaParams[OUTPUT_PORT]->eFormat = pCompWmaParam->eFormat;
            }
            else {
                eError = OMX_ErrorBadPortIndex;
            }
        }
        break;
    case OMX_IndexParamPortDefinition:
        if(((OMX_PARAM_PORTDEFINITIONTYPE *)(pCompParam))->nPortIndex == 
           pComponentPrivate->pPortDef[INPUT_PORT]->nPortIndex) {

            memcpy(pComponentPrivate->pPortDef[INPUT_PORT], 
                   pCompParam,
                   sizeof(OMX_PARAM_PORTDEFINITIONTYPE)
                   );

        } 
        else if(((OMX_PARAM_PORTDEFINITIONTYPE *)(pCompParam))->nPortIndex == 
                pComponentPrivate->pPortDef[OUTPUT_PORT]->nPortIndex) {

            memcpy(pComponentPrivate->pPortDef[OUTPUT_PORT], 
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
        }
        break;
    case OMX_IndexParamStandardComponentRole:
        if (pCompParam) {
            pRole = (OMX_PARAM_COMPONENTROLETYPE *)pCompParam;
            memcpy(&(pComponentPrivate->componentRole), (void *)pRole, sizeof(OMX_PARAM_COMPONENTROLETYPE));
        } else {
            eError = OMX_ErrorBadParameter;
        }
        break;

    case OMX_IndexParamAudioPcm:
        if(pCompParam){
            wma_op = (OMX_AUDIO_PARAM_PCMMODETYPE *)pCompParam;
            memcpy(pComponentPrivate->wma_op, wma_op, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
        }
        else{
            eError = OMX_ErrorBadParameter;
        }
        break;

    case OMX_IndexParamCompBufferSupplier:
        if(((OMX_PARAM_PORTDEFINITIONTYPE *)(pCompParam))->nPortIndex ==
           pComponentPrivate->pPortDef[INPUT_PORT]->nPortIndex) {
            OMX_PRINT2(pComponentPrivate->dbg, ":: SetParameter OMX_IndexParamCompBufferSupplier \n");
            sBufferSupplier.eBufferSupplier = OMX_BufferSupplyInput;
            memcpy(&sBufferSupplier, pCompParam, sizeof(OMX_PARAM_BUFFERSUPPLIERTYPE));                                                            
        }
        else if(((OMX_PARAM_PORTDEFINITIONTYPE *)(pCompParam))->nPortIndex ==
                pComponentPrivate->pPortDef[OUTPUT_PORT]->nPortIndex) {
            OMX_PRINT2(pComponentPrivate->dbg, ":: SetParameter OMX_IndexParamCompBufferSupplier \n");
            sBufferSupplier.eBufferSupplier = OMX_BufferSupplyOutput;
            memcpy(&sBufferSupplier, pCompParam, sizeof(OMX_PARAM_BUFFERSUPPLIERTYPE));
        } 
        else {
            OMX_ERROR2(pComponentPrivate->dbg, ":: OMX_ErrorBadPortIndex from SetParameter");
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

    WMADEC_COMPONENT_PRIVATE *pComponentPrivate;
    TI_OMX_STREAM_INFO *streamInfo;

    OMX_MALLOC_GENERIC(streamInfo, TI_OMX_STREAM_INFO);
    pComponentPrivate = (WMADEC_COMPONENT_PRIVATE *)
        (((OMX_COMPONENTTYPE*)hComp)->pComponentPrivate);

    OMX_PRINT1(pComponentPrivate->dbg, "Inside   GetConfig\n");

#ifdef _ERROR_PROPAGATION__
    if (pComponentPrivate->curState == OMX_StateInvalid){
        eError = OMX_ErrorInvalidState;
        goto EXIT;
    }   
#endif

    if(nConfigIndex == OMX_IndexCustomWmaDecStreamIDConfig)
    {
        /* copy component info */
        streamInfo->streamId = pComponentPrivate->streamID;
        memcpy(ComponentConfigStructure,streamInfo,sizeof(TI_OMX_STREAM_INFO));
    } else if (nConfigIndex == OMX_IndexCustomDebug) {
	OMX_DBG_GETCONFIG(pComponentPrivate->dbg, ComponentConfigStructure);
    }

 EXIT:
    if(streamInfo)
    {
        free(streamInfo);
        streamInfo = NULL;
    }
    OMX_PRINT1(pComponentPrivate->dbg, "Exiting  GetConfig\n");
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
    WMADEC_COMPONENT_PRIVATE *pComponentPrivate;
    WMA_HeadInfo* headerInfo = NULL;
    TI_OMX_DSP_DEFINITION* pDspDefinition = (TI_OMX_DSP_DEFINITION*)ComponentConfigStructure;
    /*int flagValue = 0;*/
    OMX_S16* deviceString;
    TI_OMX_DATAPATH dataPath;    
    OMXDBG_PRINT(stderr, PRINT, 1, 0, "%d :: Entering SetConfig\n", __LINE__);
    if (pHandle == NULL) {
        OMXDBG_PRINT(stderr, ERROR, 4, 0, "%d :: Invalid HANDLE OMX_ErrorBadParameter \n",__LINE__);
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pComponentPrivate =  (WMADEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    OMX_PRSTATE1(pComponentPrivate->dbg, "Set Config %d\n",__LINE__);

#ifdef _ERROR_PROPAGATION__
    if (pComponentPrivate->curState == OMX_StateInvalid){
        eError = OMX_ErrorInvalidState;
        goto EXIT;
    }   
#endif

    switch (nConfigIndex) {
    case OMX_IndexCustomWMADECHeaderInfoConfig:
        memcpy(pComponentPrivate->pDspDefinition,pDspDefinition,sizeof(TI_OMX_DSP_DEFINITION));
        headerInfo = pDspDefinition->wmaHeaderInfo;
        memcpy(pComponentPrivate->pHeaderInfo,headerInfo,sizeof(WMA_HeadInfo));
        if(pComponentPrivate->pDspDefinition->dasfMode == 0){
            pComponentPrivate->dasfmode = 0;
        }
        else if (pComponentPrivate->pDspDefinition->dasfMode == 1) {
            pComponentPrivate->dasfmode = 1;
        }
        else if(pComponentPrivate->pDspDefinition->dasfMode == 2) {
            pComponentPrivate->dasfmode = 1;
        }
        pComponentPrivate->streamID = pDspDefinition->streamId;
        break;
    case  OMX_IndexCustomWmaDecDataPath:
        deviceString = (OMX_S16*)ComponentConfigStructure;
        if (deviceString == NULL)
        {
            eError = OMX_ErrorBadParameter;
            goto EXIT;
        }

        dataPath = *deviceString;
        switch(dataPath) 
        {
        case DATAPATH_APPLICATION:
            OMX_MMMIXER_DATAPATH(pComponentPrivate->sDeviceString, RENDERTYPE_DECODER, pComponentPrivate->streamID);
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
    case OMX_IndexCustomDebug: 
	OMX_DBG_SETCONFIG(pComponentPrivate->dbg, ComponentConfigStructure);
	break;
    default:
        eError = OMX_ErrorUnsupportedIndex;
        break;
    }
 EXIT:
    OMX_PRINT1(pComponentPrivate->dbg, "%d :: Exiting SetConfig\n", __LINE__);
    OMX_PRINT1(pComponentPrivate->dbg, "%d :: Returning = 0x%x\n",__LINE__,eError);
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
        OMXDBG_PRINT(stderr, ERROR, 4, 0, "About to return OMX_ErrorBadParameter on line %d\n",__LINE__);
        goto EXIT;
    }

    if (pHandle && pHandle->pComponentPrivate) {
        *pState =  ((WMADEC_COMPONENT_PRIVATE*)
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
    WMADEC_COMPONENT_PRIVATE *pComponentPrivate =
        (WMADEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef;
    int ret=0;
    pPortDef = ((WMADEC_COMPONENT_PRIVATE*)
                pComponentPrivate)->pPortDef[INPUT_PORT];
#ifdef _ERROR_PROPAGATION__
    if (pComponentPrivate->curState == OMX_StateInvalid){
        eError = OMX_ErrorInvalidState;
        goto EXIT;
    }   
#endif
#ifdef __PERF_INSTRUMENTATION__
    OMX_PRINT1(pComponentPrivate->dbg, "PERF %d :: OMX_WmaDecoder.c\n",__LINE__);
    PERF_ReceivedFrame(pComponentPrivate->pPERF,
                       pBuffer->pBuffer,
                       pBuffer->nFilledLen,
                       PERF_ModuleHLMM);
#endif
    if(!pPortDef->bEnabled) {
        eError = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }   

    if (pBuffer == NULL) {
        eError = OMX_ErrorBadParameter;
        OMX_ERROR4(pComponentPrivate->dbg, "About to return OMX_ErrorBadParameter on line %d\n",__LINE__);
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
    
    if (pBuffer->nInputPortIndex != INPUT_PORT) {
        eError  = OMX_ErrorBadPortIndex;
        goto EXIT;
    }    

    if(pComponentPrivate->curState != OMX_StateExecuting && pComponentPrivate->curState != OMX_StatePause
       && pComponentPrivate->curState != OMX_StateIdle) {
        eError = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }

    OMX_PRBUFFER1(pComponentPrivate->dbg, "\n------------------------------------------\n\n");
    OMX_PRBUFFER1(pComponentPrivate->dbg, "%d :: Component Sending Filled ip buff %p to Component Thread\n",
                  __LINE__,pBuffer);
    OMX_PRBUFFER1(pComponentPrivate->dbg, "\n------------------------------------------\n\n");

    if (pComponentPrivate->bBypassDSP == 0) {
        pComponentPrivate->app_nBuf--;
    }

    pComponentPrivate->pMarkData = pBuffer->pMarkData;
    pComponentPrivate->hMarkTargetComponent = pBuffer->hMarkTargetComponent;

    pComponentPrivate->nUnhandledEmptyThisBuffers++;
 
    ret = write (pComponentPrivate->dataPipe[1], &pBuffer, sizeof(OMX_BUFFERHEADERTYPE*));

    if (ret == -1) {
        OMX_TRACE4(pComponentPrivate->dbg, "%d :: Error in Writing to the Data pipe\n", __LINE__);
        eError = OMX_ErrorHardware;
        goto EXIT;
    }

    pComponentPrivate->nEmptyThisBufferCount++;
 EXIT:
    return eError;
}   
/**-------------------------------------------------------------------*
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
    WMADEC_COMPONENT_PRIVATE *pComponentPrivate =
        (WMADEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef;
    int nRet=0;
    
    OMX_PRBUFFER1(pComponentPrivate->dbg, "\n------------------------------------------\n\n");
    OMX_PRBUFFER1(pComponentPrivate->dbg, "%d :: Component Sending Emptied op buff %p to Component Thread\n",
                   __LINE__,pBuffer);
    OMX_PRBUFFER1(pComponentPrivate->dbg, "\n------------------------------------------\n\n");

    pPortDef = ((WMADEC_COMPONENT_PRIVATE*) 
                pComponentPrivate)->pPortDef[OUTPUT_PORT];
#ifdef _ERROR_PROPAGATION__
    if (pComponentPrivate->curState == OMX_StateInvalid){
        eError = OMX_ErrorInvalidState;
        goto EXIT;
    }   
#endif
#ifdef __PERF_INSTRUMENTATION__
    OMX_PRDSP1(pComponentPrivate->dbg, "PERF %d :: OMX_WmaDecoder.c\n",__LINE__);
    PERF_ReceivedFrame(pComponentPrivate->pPERF,
                       pBuffer->pBuffer,
                       0,
                       PERF_ModuleHLMM);
#endif

    if(!pPortDef->bEnabled) {
        eError = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }

    if (pBuffer == NULL) {
        eError = OMX_ErrorBadParameter;
        OMX_ERROR4(pComponentPrivate->dbg, "About to return OMX_ErrorBadParameter on line %d\n",__LINE__);
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

    if (pBuffer->nOutputPortIndex != OUTPUT_PORT) {
        eError  = OMX_ErrorBadPortIndex;
        goto EXIT;
    }

    if(pComponentPrivate->curState != OMX_StateExecuting && pComponentPrivate->curState != OMX_StatePause) {
        eError = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }

    OMX_PRBUFFER1(pComponentPrivate->dbg, "FillThisBuffer Line %d\n",__LINE__);
    pBuffer->nFilledLen = 0;
    OMX_PRBUFFER1(pComponentPrivate->dbg, "FillThisBuffer Line %d\n",__LINE__);
    /*Filling the Output buffer with zero */
    
    memset (pBuffer->pBuffer,0,pBuffer->nAllocLen);
    OMX_PRBUFFER1(pComponentPrivate->dbg, "FillThisBuffer Line %d\n",__LINE__);

    OMX_PRBUFFER1(pComponentPrivate->dbg, "pComponentPrivate->pMarkBuf = %p\n",pComponentPrivate->pMarkBuf);
    OMX_PRBUFFER1(pComponentPrivate->dbg, "pComponentPrivate->pMarkData = %p\n",pComponentPrivate->pMarkData);
    if(pComponentPrivate->pMarkBuf){
        OMX_PRBUFFER1(pComponentPrivate->dbg, "FillThisBuffer Line %d\n",__LINE__);
        pBuffer->hMarkTargetComponent = pComponentPrivate->pMarkBuf->hMarkTargetComponent;
        pBuffer->pMarkData = pComponentPrivate->pMarkBuf->pMarkData;
        pComponentPrivate->pMarkBuf = NULL;
    }

    if (pComponentPrivate->pMarkData) {
        OMX_PRBUFFER1(pComponentPrivate->dbg, "FillThisBuffer Line %d\n",__LINE__);
        pBuffer->hMarkTargetComponent = pComponentPrivate->hMarkTargetComponent;
        pBuffer->pMarkData = pComponentPrivate->pMarkData;
        pComponentPrivate->pMarkData = NULL;
    }
    pComponentPrivate->nUnhandledFillThisBuffers++;
    nRet = write (pComponentPrivate->dataPipe[1], &pBuffer,
                  sizeof (OMX_BUFFERHEADERTYPE*));
    if (nRet == -1) {
        OMX_ERROR4(pComponentPrivate->dbg, "%d :: Error in Writing to the Data pipe\n", __LINE__);
        eError = OMX_ErrorHardware;
        goto EXIT;
    }    

 EXIT:
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
    WMADEC_COMPONENT_PRIVATE *pComponentPrivate =
        (WMADEC_COMPONENT_PRIVATE *)pComponent->pComponentPrivate;
    struct OMX_TI_Debug dbg;
    dbg = pComponentPrivate->dbg;


    OMX_PRINT1(dbg, "%d ::ComponentDeInit\n",__LINE__);
    
#ifdef __PERF_INSTRUMENTATION__
    OMX_PRDSP1(dbg, "PERF %d :: OMX_WmaDecoder.c\n",__LINE__);
    PERF_Boundary(pComponentPrivate->pPERF,
                  PERF_BoundaryStart | PERF_BoundaryCleanup);
#endif
    
#ifdef DSP_RENDERING_ON 
    close(fdwrite);
    close(fdread);
#endif

#ifdef RESOURCE_MANAGER_ENABLED  
    eError =  RMProxy_NewSendCommand(pHandle, RMProxy_FreeResource, 
                                     OMX_WMA_Decoder_COMPONENT, 0, 1234, NULL);
    if (eError != OMX_ErrorNone) {
        OMX_ERROR4(dbg, "%d ::OMX_AmrDecoder.c :: Error returned from destroy ResourceManagerProxy thread\n",
                       __LINE__);
    }
    eError = RMProxy_Deinitalize();
    if (eError != OMX_ErrorNone) {
        OMX_ERROR4(dbg, "%d ::Error returned from destroy ResourceManagerProxy thread\n",
                       __LINE__);
    }
#endif    
    OMX_PRINT1(dbg, "%d ::ComponentDeInit\n",__LINE__);
    pComponentPrivate->bIsStopping = 1;
    eError = WMADEC_StopComponentThread(pHandle);
    OMX_PRINT1(dbg, "%d ::ComponentDeInit\n",__LINE__);
    /* Wait for thread to exit so we can get the status into "error" */

    /* close the pipe handles */
    WMADEC_FreeCompResources(pHandle);
    OMX_PRINT1(dbg, "%d ::After WMADEC_FreeCompResources\n",__LINE__);
    OMX_MEMFREE_STRUCT(pComponentPrivate->pHeaderInfo);
    OMX_MEMFREE_STRUCT(pComponentPrivate->pDspDefinition);
    OMX_MEMFREE_STRUCT(pComponentPrivate->pInputBufferList);
    OMX_MEMFREE_STRUCT(pComponentPrivate->pOutputBufferList);
    OMX_MEMFREE_STRUCT(pComponentPrivate->sDeviceString);
    OMX_MEMFREE_STRUCT(pComponentPrivate->rcaheader);
#ifdef __PERF_INSTRUMENTATION__
    OMX_PRDSP1(dbg, "PERF %d :: OMX_WmaDecoder.c\n",__LINE__);
    PERF_Boundary(pComponentPrivate->pPERF,
                  PERF_BoundaryComplete | PERF_BoundaryCleanup);
    PERF_Done(pComponentPrivate->pPERF);
#endif
    OMX_MEMFREE_STRUCT(pComponentPrivate);
    OMX_PRINT1(dbg, "%d ::After free(pComponentPrivate)\n",__LINE__);
    OMX_DBG_CLOSE(dbg);
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
    OMXDBG_PRINT(stderr, PRINT, 1, 0, "===========================Inside the ComponentTunnelRequest==============================\n");
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMXDBG_PRINT(stderr, ERROR, 4, 0, "Inside the ComponentTunnelRequest\n");
    eError = OMX_ErrorNotImplemented;
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
    WMADEC_COMPONENT_PRIVATE *pComponentPrivate;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_BUFFERHEADERTYPE *pBufferHeader;

    pComponentPrivate = (WMADEC_COMPONENT_PRIVATE *)
        (((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);

    pPortDef = ((WMADEC_COMPONENT_PRIVATE*) 
                pComponentPrivate)->pPortDef[nPortIndex];

#ifdef _ERROR_PROPAGATION__
    if (pComponentPrivate->curState == OMX_StateInvalid){
        eError = OMX_ErrorInvalidState;
        goto EXIT;
    }   
#endif

    OMX_PRCOMM2(pComponentPrivate->dbg, "%d :: pPortDef = %p\n", __LINE__,pPortDef);
    OMX_PRCOMM2(pComponentPrivate->dbg, "%d :: pPortDef->bEnabled = %d\n", __LINE__,pPortDef->bEnabled);

    OMX_PRDSP2(pComponentPrivate->dbg, "pPortDef->bEnabled = %d\n", pPortDef->bEnabled);

    if(!pPortDef->bEnabled) {
        pComponentPrivate->AlloBuf_waitingsignal = 1;
#ifndef UNDER_CE
        pthread_mutex_lock(&pComponentPrivate->AlloBuf_mutex); 
        pthread_cond_wait(&pComponentPrivate->AlloBuf_threshold, &pComponentPrivate->AlloBuf_mutex);
        pthread_mutex_unlock(&pComponentPrivate->AlloBuf_mutex);
#else
        OMX_WaitForEvent(&(pComponentPrivate->AlloBuf_event));
#endif

    }

    OMX_MALLOC_GENERIC(pBufferHeader, OMX_BUFFERHEADERTYPE);

    memset(pBufferHeader, 0x0, sizeof(OMX_BUFFERHEADERTYPE));

    OMX_MALLOC_SIZE_DSPALIGN(pBufferHeader->pBuffer, nSizeBytes, OMX_U8);
    if(NULL == pBufferHeader->pBuffer) {
	    OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: Malloc Failed\n",__LINE__);
        if (pBufferHeader) {
            OMX_MEMFREE_STRUCT(pBufferHeader);
        }
        goto EXIT;
    }

    if (nPortIndex == INPUT_PORT)
    {
        pBufferHeader->nInputPortIndex = nPortIndex;
        pBufferHeader->nOutputPortIndex = -1; 
        pComponentPrivate->pInputBufferList->pBufHdr[pComponentPrivate->pInputBufferList->numBuffers] = pBufferHeader;
        pComponentPrivate->pInputBufferList->bBufferPending[pComponentPrivate->pInputBufferList->numBuffers] = 0;
        OMX_PRBUFFER1(pComponentPrivate->dbg, "pComponentPrivate->pInputBufferList->pBufHdr[%d] = %p\n",pComponentPrivate->pInputBufferList->numBuffers,pComponentPrivate->pInputBufferList->pBufHdr[pComponentPrivate->pInputBufferList->numBuffers]);
        pComponentPrivate->pInputBufferList->bufferOwner[pComponentPrivate->pInputBufferList->numBuffers++] = 1;
        OMX_PRBUFFER1(pComponentPrivate->dbg, "Allocate Buffer Line %d\n",__LINE__);
        OMX_PRBUFFER1(pComponentPrivate->dbg, "pComponentPrivate->pInputBufferList->numBuffers = %d\n",pComponentPrivate->pInputBufferList->numBuffers);
        OMX_PRBUFFER1(pComponentPrivate->dbg, "pPortDef->nBufferCountMin = %ld\n",pPortDef->nBufferCountMin);
        if (pComponentPrivate->pInputBufferList->numBuffers == pPortDef->nBufferCountActual) {
            pPortDef->bPopulated = OMX_TRUE;
        }
    }
    else if (nPortIndex == OUTPUT_PORT) {
        pBufferHeader->nInputPortIndex = -1;
        pBufferHeader->nOutputPortIndex = nPortIndex; 
        pComponentPrivate->pOutputBufferList->pBufHdr[pComponentPrivate->pOutputBufferList->numBuffers] = pBufferHeader;
        pComponentPrivate->pOutputBufferList->bBufferPending[pComponentPrivate->pOutputBufferList->numBuffers] = 0;
        OMX_PRBUFFER1(pComponentPrivate->dbg, "pComponentPrivate->pOutputBufferList->pBufHdr[%d] = %p\n",pComponentPrivate->pOutputBufferList->numBuffers,pComponentPrivate->pOutputBufferList->pBufHdr[pComponentPrivate->pOutputBufferList->numBuffers]);
        pComponentPrivate->pOutputBufferList->bufferOwner[pComponentPrivate->pOutputBufferList->numBuffers++] = 1;
        if (pComponentPrivate->pOutputBufferList->numBuffers == pPortDef->nBufferCountActual) {
            pPortDef->bPopulated = OMX_TRUE;
        }
    }
    else {
        eError = OMX_ErrorBadPortIndex;
        goto EXIT;
    }
    
    if((pComponentPrivate->pPortDef[OUTPUT_PORT]->bPopulated == pComponentPrivate->pPortDef[OUTPUT_PORT]->bEnabled)&&
       (pComponentPrivate->pPortDef[INPUT_PORT]->bPopulated == pComponentPrivate->pPortDef[INPUT_PORT]->bEnabled) &&
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
    pBufferHeader->nVersion.s.nVersionMajor = WMADEC_MAJOR_VER;
    pBufferHeader->nVersion.s.nVersionMinor = WMADEC_MINOR_VER;
    pComponentPrivate->nVersion = pBufferHeader->nVersion.nVersion;


    pBufferHeader->nSize = sizeof(OMX_BUFFERHEADERTYPE);
    OMX_PRINT2(pComponentPrivate->dbg, "Line %d\n",__LINE__); 
    *pBuffer = pBufferHeader;

    if (pComponentPrivate->bEnableCommandPending && pPortDef->bPopulated) {
        SendCommand (pComponentPrivate->pHandle,
                     OMX_CommandPortEnable,
                     pComponentPrivate->bEnableCommandParam,NULL);
    }
    
#ifdef __PERF_INSTRUMENTATION__
    OMX_PRDSP1(pComponentPrivate->dbg, "PERF %d :: OMX_WmaDecoder.c\n",__LINE__);
    PERF_ReceivedBuffer(pComponentPrivate->pPERF,
                        (*pBuffer)->pBuffer, nSizeBytes,
                        PERF_ModuleMemory);
#endif

 EXIT:
    OMX_PRINT1(pComponentPrivate->dbg, "AllocateBuffer returning %d\n",eError);
    return eError;
}


/* ================================================================================= */
/**
 * @fn FreeBuffer() description for FreeBuffer  
 FreeBuffer().  
 Called by the OMX IL client to free a buffer. 
 *
 *  @see         OMX_Core.h
 */
/* ================================================================================ */
static OMX_ERRORTYPE FreeBuffer(
                                OMX_IN  OMX_HANDLETYPE hComponent,
                                OMX_IN  OMX_U32 nPortIndex,
                                OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer)
{

    OMX_ERRORTYPE eError = OMX_ErrorNone;
    WMADEC_COMPONENT_PRIVATE * pComponentPrivate = NULL;
    OMX_BUFFERHEADERTYPE* buff;
    int i;
    int inputIndex = -1;
    int outputIndex = -1;
    OMX_COMPONENTTYPE *pHandle;

    pComponentPrivate = (WMADEC_COMPONENT_PRIVATE *)
        (((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);

    pHandle = (OMX_COMPONENTTYPE *) pComponentPrivate->pHandle;
    for (i=0; i < MAX_NUM_OF_BUFS; i++) {
        buff = pComponentPrivate->pInputBufferList->pBufHdr[i];
        if (buff == pBuffer) {
            OMX_PRBUFFER2(pComponentPrivate->dbg, "Found matching input buffer\n");
            OMX_PRBUFFER2(pComponentPrivate->dbg, "buff = %p\n",buff);
            OMX_PRBUFFER2(pComponentPrivate->dbg, "pBuffer = %p\n",pBuffer);
            inputIndex = i;
            break;
        }
        else {
            OMX_PRBUFFER2(pComponentPrivate->dbg, "This is not a match\n");
            OMX_PRBUFFER2(pComponentPrivate->dbg, "buff = %p\n",buff);
            OMX_PRBUFFER2(pComponentPrivate->dbg, "pBuffer = %p\n",pBuffer);
        }
    }

    for (i=0; i < MAX_NUM_OF_BUFS; i++) {
        buff = pComponentPrivate->pOutputBufferList->pBufHdr[i];
        if (buff == pBuffer) {
            OMX_PRBUFFER2(pComponentPrivate->dbg, "Found matching output buffer\n");
            OMX_PRBUFFER2(pComponentPrivate->dbg, "buff = %p\n",buff);
            OMX_PRBUFFER2(pComponentPrivate->dbg, "pBuffer = %p\n",pBuffer);
            outputIndex = i;
            break;
        }
        else {
            OMX_PRBUFFER2(pComponentPrivate->dbg, "This is not a match\n");
            OMX_PRBUFFER2(pComponentPrivate->dbg, "buff = %p\n",buff);
            OMX_PRBUFFER2(pComponentPrivate->dbg, "pBuffer = %p\n",pBuffer);
        }
    }


    if (inputIndex != -1) {
        if (pComponentPrivate->pInputBufferList->bufferOwner[inputIndex] == 1) {
            OMX_MEMFREE_STRUCT_DSPALIGN(pComponentPrivate->pInputBufferList->pBufHdr[inputIndex]->pBuffer, OMX_U8);
        }
#ifdef __PERF_INSTRUMENTATION__
        OMX_PRDSP1(pComponentPrivate->dbg, "PERF %d :: OMX_WmaDecoder.c\n",__LINE__);
        PERF_SendingBuffer(pComponentPrivate->pPERF,
                           pComponentPrivate->pInputBufferList->pBufHdr[inputIndex]->pBuffer, 
                           pComponentPrivate->pInputBufferList->pBufHdr[inputIndex]->nAllocLen,
                           PERF_ModuleMemory);
#endif
        OMX_PRBUFFER2(pComponentPrivate->dbg, "%d:[FREE] %p\n",__LINE__,pComponentPrivate->pInputBufferList->pBufHdr[inputIndex]);
        OMX_MEMFREE_STRUCT(pComponentPrivate->pInputBufferList->pBufHdr[inputIndex]);
        pComponentPrivate->pInputBufferList->numBuffers--;
            
        if (pComponentPrivate->pInputBufferList->numBuffers < 
            pComponentPrivate->pPortDef[INPUT_PORT]->nBufferCountMin) {
    
            pComponentPrivate->pPortDef[INPUT_PORT]->bPopulated = OMX_FALSE;
        }
            
        if(pComponentPrivate->pPortDef[INPUT_PORT]->bEnabled && 
           pComponentPrivate->bLoadedCommandPending == OMX_FALSE &&
           (pComponentPrivate->curState == OMX_StateIdle || 
            pComponentPrivate->curState == OMX_StateExecuting || 
            pComponentPrivate->curState == OMX_StatePause)) {
            pComponentPrivate->cbInfo.EventHandler(
                                                   pHandle, pHandle->pApplicationPrivate,
                                                   OMX_EventError, OMX_ErrorPortUnpopulated,nPortIndex, NULL);
        }
    }
    else if (outputIndex != -1) {
        if (pComponentPrivate->pOutputBufferList->bufferOwner[outputIndex] == 1) {
            OMX_MEMFREE_STRUCT_DSPALIGN(pComponentPrivate->pOutputBufferList->pBufHdr[outputIndex]->pBuffer, OMX_U8);
        }
            
#ifdef __PERF_INSTRUMENTATION__
        OMX_PRDSP1(pComponentPrivate->dbg, "PERF %d :: OMX_WmaDecoder.c\n",__LINE__);
        PERF_SendingBuffer(pComponentPrivate->pPERF,
                           pComponentPrivate->pOutputBufferList->pBufHdr[outputIndex]->pBuffer, 
                           pComponentPrivate->pOutputBufferList->pBufHdr[outputIndex]->nAllocLen,
                           PERF_ModuleMemory);
#endif

        OMX_PRBUFFER2(pComponentPrivate->dbg, "%d:[FREE] %p\n",__LINE__,pComponentPrivate->pOutputBufferList->pBufHdr[outputIndex]);
        OMX_MEMFREE_STRUCT(pComponentPrivate->pOutputBufferList->pBufHdr[outputIndex]);
        pComponentPrivate->pOutputBufferList->numBuffers--;
    
        if (pComponentPrivate->pOutputBufferList->numBuffers < 
            pComponentPrivate->pPortDef[OUTPUT_PORT]->nBufferCountMin) {
            pComponentPrivate->pPortDef[OUTPUT_PORT]->bPopulated = OMX_FALSE;
        }
        if(pComponentPrivate->pPortDef[OUTPUT_PORT]->bEnabled && 
           pComponentPrivate->bLoadedCommandPending == OMX_FALSE &&
           !pComponentPrivate->reconfigOutputPort &&
           (pComponentPrivate->curState == OMX_StateIdle || 
            pComponentPrivate->curState == OMX_StateExecuting || 
            pComponentPrivate->curState == OMX_StatePause)) {
            pComponentPrivate->cbInfo.EventHandler(
                                                   pHandle, pHandle->pApplicationPrivate,
                                                   OMX_EventError, OMX_ErrorPortUnpopulated,nPortIndex, NULL);
        }
    }
    else {
        OMX_ERROR2(pComponentPrivate->dbg, "%d::Returning OMX_ErrorBadParameter\n",__LINE__);
        eError = OMX_ErrorBadParameter;
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

    /* Removing sleep() calls.  There are no allocated buffers. */
#if 0
    if (pComponentPrivate->bDisableCommandPending && 
        (pComponentPrivate->pInputBufferList->numBuffers + 
         pComponentPrivate->pOutputBufferList->numBuffers == 0)) {
        if (pComponentPrivate->pInputBufferList->numBuffers + 
            pComponentPrivate->pOutputBufferList->numBuffers == 0) {
            SendCommand (pComponentPrivate->pHandle,OMX_CommandPortDisable,
                         pComponentPrivate->bDisableCommandParam,NULL);
        }
    }
#else
    if (pComponentPrivate->bDisableCommandPending && 
        (pComponentPrivate->pInputBufferList->numBuffers == 0)) {
        pComponentPrivate->bDisableCommandPending = OMX_FALSE;
        pComponentPrivate->cbInfo.EventHandler( pComponentPrivate->pHandle,
                                                pComponentPrivate->pHandle->pApplicationPrivate,
                                                OMX_EventCmdComplete,
                                                OMX_CommandPortDisable,
                                                INPUT_PORT,
                                                NULL);

    }
    if (pComponentPrivate->bDisableCommandPending && 
        (pComponentPrivate->pOutputBufferList->numBuffers == 0)) {
        pComponentPrivate->bDisableCommandPending = OMX_FALSE;
        pComponentPrivate->cbInfo.EventHandler( pComponentPrivate->pHandle,
                                                pComponentPrivate->pHandle->pApplicationPrivate,
                                                OMX_EventCmdComplete,
                                                OMX_CommandPortDisable,
                                                OUTPUT_PORT,
                                                NULL);

    }

#endif

    OMX_PRINT1(pComponentPrivate->dbg, "%d :: Exiting FreeBuffer\n", __LINE__);
    return eError;
}


/* ================================================================================= */
/**
 * @fn UseBuffer() description for UseBuffer  
 UseBuffer().  
 Called by the OMX IL client to pass a buffer to be used.   
 *
 *  @see         OMX_Core.h
 */
/* ================================================================================ */
static OMX_ERRORTYPE UseBuffer (
                                OMX_IN OMX_HANDLETYPE hComponent,
                                OMX_INOUT OMX_BUFFERHEADERTYPE** ppBufferHdr,
                                OMX_IN OMX_U32 nPortIndex,
                                OMX_IN OMX_PTR pAppPrivate,
                                OMX_IN OMX_U32 nSizeBytes,
                                OMX_IN OMX_U8* pBuffer)
{

    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef;
    WMADEC_COMPONENT_PRIVATE *pComponentPrivate;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_BUFFERHEADERTYPE *pBufferHeader;

    pComponentPrivate = (WMADEC_COMPONENT_PRIVATE *)
        (((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);

#ifdef _ERROR_PROPAGATION__
    if (pComponentPrivate->curState == OMX_StateInvalid){
        eError = OMX_ErrorInvalidState;
        goto EXIT;
    }
    
#endif

#ifdef __PERF_INSTRUMENTATION__
    OMX_PRDSP1(pComponentPrivate->dbg, "PERF %d :: OMX_WmaDecoder.c\n",__LINE__);
    PERF_ReceivedBuffer(pComponentPrivate->pPERF,
                        pBuffer, nSizeBytes,
                        PERF_ModuleHLMM);
#endif

    pPortDef = ((WMADEC_COMPONENT_PRIVATE*) 
                pComponentPrivate)->pPortDef[nPortIndex];
    if(!pPortDef->bEnabled){
        pthread_mutex_lock(&pComponentPrivate->AlloBuf_mutex); 
        pComponentPrivate->AlloBuf_waitingsignal = 1;
        //wait for the port to be enabled before we accept buffers
        pthread_cond_wait(&pComponentPrivate->AlloBuf_threshold, &pComponentPrivate->AlloBuf_mutex);
        pthread_mutex_unlock(&pComponentPrivate->AlloBuf_mutex);
    }
    OMX_PRCOMM1(pComponentPrivate->dbg, "%d :: pPortDef = %p\n", __LINE__,pPortDef);
    OMX_PRCOMM1(pComponentPrivate->dbg, "%d :: pPortDef->bEnabled = %d\n", __LINE__,pPortDef->bEnabled);

    OMX_PRINT1(pComponentPrivate->dbg, "Line %d\n",__LINE__); 
    if(!pPortDef->bEnabled) {
        OMX_ERROR4(pComponentPrivate->dbg, "%d :: In AllocateBuffer\n", __LINE__);
        eError = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }

    OMX_PRINT1(pComponentPrivate->dbg, "Line %d\n",__LINE__); 
    OMX_PRINT1(pComponentPrivate->dbg, "pPortDef->bPopulated =%d\n",pPortDef->bPopulated);
    OMX_PRINT1(pComponentPrivate->dbg, "nSizeBytes =%ld\n",nSizeBytes);
    OMX_PRBUFFER1(pComponentPrivate->dbg, "pPortDef->nBufferSize =%ld\n",pPortDef->nBufferSize);

#ifndef UNDER_CE
    if(pPortDef->bPopulated) {
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }
#endif  

    OMX_PRINT1(pComponentPrivate->dbg, "Line %d\n",__LINE__); 
    OMX_MALLOC_GENERIC(pBufferHeader, OMX_BUFFERHEADERTYPE);
    OMX_PRBUFFER2(pComponentPrivate->dbg, "%d:[ALLOC] %p\n",__LINE__,pBufferHeader);

    if (pBufferHeader == 0) {
        eError = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    memset((pBufferHeader), 0x0, sizeof(OMX_BUFFERHEADERTYPE));

    OMX_PRINT1(pComponentPrivate->dbg, "Line %d\n",__LINE__); 
    if (nPortIndex == OUTPUT_PORT) {
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

    if((pComponentPrivate->pPortDef[OUTPUT_PORT]->bPopulated == pComponentPrivate->pPortDef[OUTPUT_PORT]->bEnabled)&&
       (pComponentPrivate->pPortDef[INPUT_PORT]->bPopulated == pComponentPrivate->pPortDef[INPUT_PORT]->bEnabled) &&
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
    
    OMX_PRINT1(pComponentPrivate->dbg, "Line %d\n",__LINE__); 
    pBufferHeader->pAppPrivate = pAppPrivate;
    pBufferHeader->pPlatformPrivate = pComponentPrivate;
    pBufferHeader->nAllocLen = nSizeBytes;
    pBufferHeader->nVersion.s.nVersionMajor = WMADEC_MAJOR_VER;
    pBufferHeader->nVersion.s.nVersionMinor = WMADEC_MINOR_VER;
    pComponentPrivate->nVersion = pBufferHeader->nVersion.nVersion;
    pBufferHeader->pBuffer = pBuffer;
    pBufferHeader->nSize = sizeof(OMX_BUFFERHEADERTYPE);
    OMX_PRINT1(pComponentPrivate->dbg, "Line %d\n",__LINE__); 
    *ppBufferHdr = pBufferHeader;
    OMX_PRBUFFER1(pComponentPrivate->dbg, "pBufferHeader = %p\n",pBufferHeader);
    if (pComponentPrivate->bEnableCommandPending && pPortDef->bPopulated){
        OMX_PRCOMM2(pComponentPrivate->dbg, "Sending command before exiting usbuffer\n");
        SendCommand (pComponentPrivate->pHandle,
                     OMX_CommandPortEnable,
                     pComponentPrivate->bEnableCommandParam,NULL);
    }    
    OMX_PRINT1(pComponentPrivate->dbg, "exiting Use buffer\n");      
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
                                       OMX_OUT OMX_INDEXTYPE* pIndexType) 
{

    OMX_ERRORTYPE eError = OMX_ErrorNone;

    OMXDBG_PRINT(stderr, PRINT, 1, 0, "GetExtensionIndex\n");
    if (!(strcmp(cParameterName,"OMX.TI.index.config.wmaheaderinfo"))) {
        *pIndexType = OMX_IndexCustomWMADECHeaderInfoConfig;
        OMXDBG_PRINT(stderr, DSP, 2, 0, "OMX_IndexCustomWMADECHeaderInfoConfig\n");
    }
    else if(!(strcmp(cParameterName,"OMX.TI.index.config.wmastreamIDinfo"))) 
    {
        *pIndexType = OMX_IndexCustomWmaDecStreamIDConfig;
        
    }
    else if(!(strcmp(cParameterName,"OMX.TI.index.config.wmadec.datapath"))) 
    {
        *pIndexType = OMX_IndexCustomWmaDecDataPath;
    }    
    else if(!(strcmp(cParameterName,"OMX.TI.WMA.Decode.Debug"))) 
    {
	*pIndexType = OMX_IndexCustomDebug;
    }    
    else {
        eError = OMX_ErrorBadParameter;
    }

    OMXDBG_PRINT(stderr, PRINT, 1, 0, "Exiting GetExtensionIndex\n");
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
    WMADEC_COMPONENT_PRIVATE *pComponentPrivate;
    
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    pComponentPrivate = (WMADEC_COMPONENT_PRIVATE *)(((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);

    if(nIndex == 0){
        memcpy(cRole, &pComponentPrivate->componentRole.cRole, sizeof(OMX_U8) * OMX_MAX_STRINGNAME_SIZE); 
        OMX_PRINT1(pComponentPrivate->dbg, "::::In ComponenetRoleEnum: cRole is set to %s\n",cRole);
    }
    else {
        eError = OMX_ErrorNoMore;
    }
    return eError;
};


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

