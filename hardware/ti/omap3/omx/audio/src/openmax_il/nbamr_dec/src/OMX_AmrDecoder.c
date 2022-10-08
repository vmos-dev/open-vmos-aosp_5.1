
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
* ============================================================================ */
/**
* @file OMX_AmrDecoder.c
*
* This file implements OMX Component for AMR decoder that
* is fully compliant with the OMX Audio specification.
*
* @path  $(CSLPATH)\
*
* @rev  0.1
*/
/* ----------------------------------------------------------------------------*/
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
#include <dlfcn.h>

#ifdef DSP_RENDERING_ON
#include <AudioManagerAPI.h>
#endif

#ifdef RESOURCE_MANAGER_ENABLED
#include <ResourceManagerProxyAPI.h>
#endif

/*-------program files ----------------------------------------*/
#include <OMX_Component.h>
#include <TIDspOmx.h>

#include "OMX_AmrDecoder.h"
#include "OMX_AmrDec_Utils.h"

/* Log for Android system*/
#include <utils/Log.h>

#define AMRNB_DEC_ROLE "audio_decoder.amrnb"


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
        
#ifdef DSP_RENDERING_ON            
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
  *         OMX_ErrorInsufficientResources If the newmalloc fails
  **/
/*-------------------------------------------------------------------*/
OMX_ERRORTYPE OMX_ComponentInit (OMX_HANDLETYPE hComp)
{
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef_ip, *pPortDef_op;
    AMRDEC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    OMX_AUDIO_PARAM_AMRTYPE *amr_ip;
    OMX_AUDIO_PARAM_PCMMODETYPE *amr_op;
    OMX_ERRORTYPE error = OMX_ErrorNone;
    OMX_ERRORTYPE eError = OMX_ErrorNone;   
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE*) hComp;
    OMX_S16 i;

    OMXDBG_PRINT(stderr, PRINT, 1, 0, "%d ::OMX_ComponentInit\n", __LINE__);

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
    OMX_MALLOC_GENERIC(pHandle->pComponentPrivate, AMRDEC_COMPONENT_PRIVATE); 

    ((AMRDEC_COMPONENT_PRIVATE *)
                pHandle->pComponentPrivate)->pHandle = pHandle;

   /* Initialize component data structures to default values */
    ((AMRDEC_COMPONENT_PRIVATE *)
                 pHandle->pComponentPrivate)->sPortParam.nPorts = 0x2;
    ((AMRDEC_COMPONENT_PRIVATE *)
                 pHandle->pComponentPrivate)->sPortParam.nStartPortNumber = 0x0;

    error = OMX_ErrorNone;

    OMX_MALLOC_GENERIC(amr_ip , OMX_AUDIO_PARAM_AMRTYPE); 
    OMX_MALLOC_GENERIC(amr_op , OMX_AUDIO_PARAM_PCMMODETYPE);


    ((AMRDEC_COMPONENT_PRIVATE *)
                 pHandle->pComponentPrivate)->amrParams[NBAMRDEC_INPUT_PORT] = amr_ip;
    ((AMRDEC_COMPONENT_PRIVATE *)
                 pHandle->pComponentPrivate)->amrParams[NBAMRDEC_OUTPUT_PORT] = (OMX_AUDIO_PARAM_AMRTYPE*)amr_op;

    pComponentPrivate = pHandle->pComponentPrivate;
    OMX_DBG_INIT(pComponentPrivate->dbg, "OMX_DBG_NBAMRDEC");

#ifdef __PERF_INSTRUMENTATION__
    pComponentPrivate->pPERF = PERF_Create(PERF_FOURCC('N','B','D','_'),
                                           PERF_ModuleLLMM |
                                           PERF_ModuleAudioDecode);
#endif


        pComponentPrivate->iPVCapabilityFlags.iIsOMXComponentMultiThreaded = OMX_TRUE;
        pComponentPrivate->iPVCapabilityFlags.iOMXComponentSupportsExternalOutputBufferAlloc = OMX_FALSE;
        pComponentPrivate->iPVCapabilityFlags.iOMXComponentSupportsExternalInputBufferAlloc = OMX_FALSE;
        pComponentPrivate->iPVCapabilityFlags.iOMXComponentSupportsMovableInputBuffers = OMX_FALSE;
        pComponentPrivate->iPVCapabilityFlags.iOMXComponentSupportsPartialFrames = OMX_FALSE;
        pComponentPrivate->iPVCapabilityFlags.iOMXComponentNeedsNALStartCode = OMX_FALSE;
        pComponentPrivate->iPVCapabilityFlags.iOMXComponentCanHandleIncompleteFrames = OMX_FALSE;


    OMX_MALLOC_GENERIC(pComponentPrivate->pInputBufferList, NBAMRDEC_BUFFERLIST); 
    pComponentPrivate->pInputBufferList->numBuffers = 0; /* initialize number of buffers */
    OMX_MALLOC_GENERIC(pComponentPrivate->pOutputBufferList, NBAMRDEC_BUFFERLIST); 
    OMX_MALLOC_GENERIC(pComponentPrivate->pPriorityMgmt, OMX_PRIORITYMGMTTYPE);
         
    pComponentPrivate->pOutputBufferList->numBuffers = 0; /* initialize number of buffers */
    pComponentPrivate->bPlayCompleteFlag = 0;
    for (i=0; i < MAX_NUM_OF_BUFS; i++) {
        pComponentPrivate->pOutputBufferList->pBufHdr[i] = NULL;
        pComponentPrivate->pInputBufferList->pBufHdr[i] = NULL;
    }
    OMX_PRINT2(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c ::Setting dasfmode and mimemode to 0\n",__LINE__);
    pComponentPrivate->dasfmode = 0;
    pComponentPrivate->mimemode = 0;
    pComponentPrivate->bPortDefsAllocated = 0;
    pComponentPrivate->bCompThreadStarted = 0;
    pComponentPrivate->nHoldLength = 0;
    pComponentPrivate->pHoldBuffer = NULL;
    pComponentPrivate->bInitParamsInitialized = 0;
    
    pComponentPrivate->amrMimeBytes[0] = FRAME_SIZE_13;
    pComponentPrivate->amrMimeBytes[1] = FRAME_SIZE_14;
    pComponentPrivate->amrMimeBytes[2] = FRAME_SIZE_16;
    pComponentPrivate->amrMimeBytes[3] = FRAME_SIZE_18;
    pComponentPrivate->amrMimeBytes[4] = FRAME_SIZE_20;
    pComponentPrivate->amrMimeBytes[5] = FRAME_SIZE_21;
    pComponentPrivate->amrMimeBytes[6] = FRAME_SIZE_27;
    pComponentPrivate->amrMimeBytes[7] = FRAME_SIZE_32;
    pComponentPrivate->amrMimeBytes[8] = FRAME_SIZE_6;
    pComponentPrivate->amrMimeBytes[9] = FRAME_SIZE_0;
    pComponentPrivate->amrMimeBytes[10] = FRAME_SIZE_0;
    pComponentPrivate->amrMimeBytes[11] = FRAME_SIZE_0;
    pComponentPrivate->amrMimeBytes[12] = FRAME_SIZE_0;
    pComponentPrivate->amrMimeBytes[13] = FRAME_SIZE_0;
    pComponentPrivate->amrMimeBytes[14] = FRAME_SIZE_0;
    pComponentPrivate->amrMimeBytes[15] = FRAME_SIZE_1;
    
    /*IF2 Pointer declarations*/
    pComponentPrivate->amrIF2Bytes[0] = FRAME_SIZE_13;
    pComponentPrivate->amrIF2Bytes[1] = FRAME_SIZE_14;
    pComponentPrivate->amrIF2Bytes[2] = FRAME_SIZE_16;
    pComponentPrivate->amrIF2Bytes[3] = FRAME_SIZE_18;
    pComponentPrivate->amrIF2Bytes[4] = FRAME_SIZE_19;
    pComponentPrivate->amrIF2Bytes[5] = FRAME_SIZE_21;
    pComponentPrivate->amrIF2Bytes[6] = FRAME_SIZE_26;
    pComponentPrivate->amrIF2Bytes[7] = FRAME_SIZE_31;
    pComponentPrivate->amrIF2Bytes[8] = FRAME_SIZE_6;
    pComponentPrivate->amrIF2Bytes[9] = FRAME_SIZE_0;
    pComponentPrivate->amrIF2Bytes[10] = FRAME_SIZE_0;
    pComponentPrivate->amrIF2Bytes[11] = FRAME_SIZE_0;
    pComponentPrivate->amrIF2Bytes[12] = FRAME_SIZE_0;
    pComponentPrivate->amrIF2Bytes[13] = FRAME_SIZE_0;
    pComponentPrivate->amrIF2Bytes[14] = FRAME_SIZE_0;
    pComponentPrivate->amrIF2Bytes[15] = FRAME_SIZE_1;
    
    pComponentPrivate->pMarkBuf = NULL;
    pComponentPrivate->pMarkData = NULL;
    pComponentPrivate->nEmptyBufferDoneCount = 0;
    pComponentPrivate->nEmptyThisBufferCount = 0;
    pComponentPrivate->nFillBufferDoneCount = 0;
    pComponentPrivate->nFillThisBufferCount = 0;
    pComponentPrivate->strmAttr = NULL;
/*  pComponentPrivate->bIdleCommandPending = 0; */
    pComponentPrivate->bDisableCommandParam = 0;
    pComponentPrivate->bEnableCommandParam = 0;

    pComponentPrivate->IpBufindex = 0;
    pComponentPrivate->OpBufindex = 0;
    pComponentPrivate->ptrLibLCML = NULL;
    pComponentPrivate->PendingPausedBufs = 0;

    pComponentPrivate->nUnhandledFillThisBuffers=0;
    pComponentPrivate->nUnhandledEmptyThisBuffers = 0;
    pComponentPrivate->SendAfterEOS = 0;
    
    pComponentPrivate->bFlushOutputPortCommandPending = OMX_FALSE;
    pComponentPrivate->bFlushInputPortCommandPending = OMX_FALSE;
    
    pComponentPrivate->first_buff = 0;
    pComponentPrivate->first_TS = 0;
    pComponentPrivate->temp_TS = 0;

    for (i=0; i < MAX_NUM_OF_BUFS; i++) {
        pComponentPrivate->pInputBufHdrPending[i] = NULL;
        pComponentPrivate->pOutputBufHdrPending[i] = NULL;
        pComponentPrivate->arrTickCount[i] = 0;
        pComponentPrivate->arrBufIndex[i] = 0;
    }

    pComponentPrivate->IpBufindex = 0;
    pComponentPrivate->OpBufindex = 0;
            
    pComponentPrivate->nNumInputBufPending = 0;
    pComponentPrivate->nNumOutputBufPending = 0;
    pComponentPrivate->bDisableCommandPending = 0;
    pComponentPrivate->bEnableCommandPending = 0;
    pComponentPrivate->nOutStandingFillDones = 0;
    pComponentPrivate->bStopSent=0;
    pComponentPrivate->bBypassDSP = OMX_FALSE;
    pComponentPrivate->bNoIdleOnStop = OMX_FALSE;
    pComponentPrivate->pParams = NULL;
    pComponentPrivate->LastOutbuf = NULL;
    pComponentPrivate->bPreempted = OMX_FALSE;    
    pComponentPrivate->using_rtsp = 0;
    
    OMX_MALLOC_SIZE(pComponentPrivate->sDeviceString, (100*sizeof(char)),OMX_STRING);
    strcpy((char*)pComponentPrivate->sDeviceString,"/eteedn:i0:o0/codec\0");

    
/* Set input port format defaults */
    pComponentPrivate->sInPortFormat.nPortIndex         = NBAMRDEC_INPUT_PORT;
    pComponentPrivate->sInPortFormat.nIndex             = OMX_IndexParamAudioAmr;    
    pComponentPrivate->sInPortFormat.eEncoding          = OMX_AUDIO_CodingAMR;


/* Set output port format defaults */
    pComponentPrivate->sOutPortFormat.nPortIndex         = NBAMRDEC_OUTPUT_PORT;
    pComponentPrivate->sOutPortFormat.nIndex             = OMX_IndexParamAudioPcm;
    pComponentPrivate->sOutPortFormat.eEncoding          = OMX_AUDIO_CodingPCM;

    amr_ip->nPortIndex = OMX_DirInput;
    amr_ip->nChannels = 1;
    amr_ip->nBitRate = 8000;
    amr_ip->eAMRBandMode = OMX_AUDIO_AMRBandModeNB0;
    amr_ip->eAMRDTXMode = OMX_AUDIO_AMRDTXModeOff;
    amr_ip->eAMRFrameFormat = OMX_AUDIO_AMRFrameFormatConformance;
    amr_ip->nSize = sizeof (OMX_AUDIO_PARAM_AMRTYPE);

    /* PCM format defaults - These values are required to pass StdAudioDecoderTest*/
    amr_op->nPortIndex = OMX_DirOutput;
    amr_op->nChannels = 1; 
    amr_op->eNumData= OMX_NumericalDataSigned;
    amr_op->nSamplingRate = NBAMRDEC_SAMPLING_FREQUENCY;
    amr_op->nBitPerSample = 16;
    amr_op->ePCMMode = OMX_AUDIO_PCMModeLinear;

    strcpy((char*)pComponentPrivate->componentRole.cRole, "audio_decoder.amrnb");    
    
    /* Removing sleep() calls. Initialization.*/
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
#else
    OMX_CreateEvent(&(pComponentPrivate->AlloBuf_event));
    pComponentPrivate->AlloBuf_waitingsignal = 0;

    OMX_CreateEvent(&(pComponentPrivate->InLoaded_event));
    pComponentPrivate->InLoaded_readytoidle = 0;

    OMX_CreateEvent(&(pComponentPrivate->InIdle_event));
    pComponentPrivate->InIdle_goingtoloaded = 0;
#endif
    OMX_MALLOC_GENERIC(pPortDef_ip, OMX_PARAM_PORTDEFINITIONTYPE);
    OMX_MALLOC_GENERIC(pPortDef_op, OMX_PARAM_PORTDEFINITIONTYPE);

    ((AMRDEC_COMPONENT_PRIVATE*) pHandle->pComponentPrivate)->pPortDef[NBAMRDEC_INPUT_PORT]
                                                              = pPortDef_ip;

    ((AMRDEC_COMPONENT_PRIVATE*) pHandle->pComponentPrivate)->pPortDef[NBAMRDEC_OUTPUT_PORT]
                                                            = pPortDef_op;
/* Define Input Port Definition*/
    pPortDef_ip->eDomain = OMX_PortDomainAudio;
    pPortDef_ip->nSize = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
    pPortDef_ip->nPortIndex = 0x0;
    pPortDef_ip->nBufferCountActual = NUM_NBAMRDEC_INPUT_BUFFERS;
    pPortDef_ip->nBufferCountMin = NUM_NBAMRDEC_INPUT_BUFFERS;
    pPortDef_ip->eDir = OMX_DirInput;
    pPortDef_ip->bEnabled = OMX_TRUE;
    pPortDef_ip->nBufferSize = IP_BUFFERSIZE;
    pPortDef_ip->nBufferAlignment = CACHE_ALIGNMENT;
    pPortDef_ip->bPopulated = 0;   
    pPortDef_ip->format.audio.eEncoding = OMX_AUDIO_CodingAMR;

/* Define Output Port Definition*/
    pPortDef_op->eDomain = OMX_PortDomainAudio;
    pPortDef_op->nSize = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
    pPortDef_op->nPortIndex = 0x1;
    pPortDef_op->nBufferCountActual = NUM_NBAMRDEC_OUTPUT_BUFFERS;
    pPortDef_op->nBufferCountMin = NUM_NBAMRDEC_OUTPUT_BUFFERS;
    pPortDef_op->nBufferAlignment = CACHE_ALIGNMENT;
    pPortDef_op->eDir = OMX_DirOutput;
    pPortDef_op->bEnabled = OMX_TRUE;
    pPortDef_op->nBufferSize = OUTPUT_NBAMRDEC_BUFFER_SIZE;
    pPortDef_op->bPopulated = 0;
    pPortDef_op->format.audio.eEncoding = OMX_AUDIO_CodingPCM;

    /*sPortFormat->eEncoding = OMX_AUDIO_CodingPCM;*/
    pComponentPrivate->bIsInvalidState = OMX_FALSE;

    

#ifdef RESOURCE_MANAGER_ENABLED
    error = RMProxy_NewInitalize();
    OMX_PRINT2(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c ::OMX_ComponentInit\n", __LINE__);
    if (error != OMX_ErrorNone) {
        OMX_ERROR4(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c ::Error returned from loading ResourceManagerProxy thread\n",
                                                        __LINE__);
        goto EXIT;
    }
#endif

error = NBAMRDEC_StartComponentThread(pHandle);
    OMX_PRINT2(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c ::OMX_ComponentInit\n", __LINE__);
    if (error != OMX_ErrorNone) {
        OMX_ERROR4(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c ::Error returned from the Component\n",
                                                     __LINE__);
        goto EXIT;
    }
    OMX_PRINT2(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c ::OMX_ComponentInit\n", __LINE__);


#ifdef DSP_RENDERING_ON
    OMX_PRINT2(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c ::OMX_ComponentInit\n", __LINE__);
    if((pComponentPrivate->fdwrite=open(FIFO1,O_WRONLY))<0) {
        OMX_ERROR4(pComponentPrivate->dbg, "[NBAMR Dec Component] - failure to open WRITE pipe\n");
    }

    OMX_PRINT2(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c ::OMX_ComponentInit\n", __LINE__);
    if((pComponentPrivate->fdread=open(FIFO2,O_RDONLY))<0) {
        OMX_ERROR4(pComponentPrivate->dbg, "[NBAMR Dec Component] - failure to open READ pipe\n");
        goto EXIT;
    }
#endif
#ifdef __PERF_INSTRUMENTATION__
    PERF_ThreadCreated(pComponentPrivate->pPERF, pComponentPrivate->ComponentThread,
                       PERF_FOURCC('N','B','D','T'));
#endif

EXIT:
    if (pComponentPrivate != NULL) {
	 OMX_PRINT1(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c ::OMX_ComponentInit - returning %d\n", __LINE__, error);
	 OMX_PRINT2(pComponentPrivate->dbg, "%s: OUT", __FUNCTION__);
    }
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

    AMRDEC_COMPONENT_PRIVATE *pComponentPrivate =
                    (AMRDEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;

    if (pCallBacks == NULL) {
        OMX_PRINT1(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c :: About to return OMX_ErrorBadParameter\n",__LINE__);
        eError = OMX_ErrorBadParameter;
        OMX_ERROR4(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c :: Received the empty callbacks from the \
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
   OMX_ERRORTYPE eError = OMX_ErrorNone;

    eError = OMX_ErrorNotImplemented;
    OMXDBG_PRINT(stderr, PRINT, 1, 0, "Inside the GetComponentVersion\n");
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
    ssize_t nRet;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)phandle;
    AMRDEC_COMPONENT_PRIVATE *pCompPrivate =
             (AMRDEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;

    OMX_PRINT1(pCompPrivate->dbg, "%d ::OMX_AmrDecoder.c ::Inside SendCommand\n",__LINE__);

    OMX_PRDSP1(pCompPrivate->dbg, "%d ::OMX_AmrDecoder.c ::phandle = %p\n",__LINE__,phandle);
    OMX_PRINT2(pCompPrivate->dbg, "%d ::OMX_AmrDecoder.c ::pCompPrivate = %p\n",__LINE__,pCompPrivate);

    pCompPrivate->pHandle = phandle;

#ifdef _ERROR_PROPAGATION__
    if (pCompPrivate->curState == OMX_StateInvalid){
        eError = OMX_ErrorInvalidState;
        goto EXIT;
    }
#else
    if(pCompPrivate->curState == OMX_StateInvalid){
        OMX_PRCOMM2(pCompPrivate->dbg, "%d ::OMX_AmrDecoder.c ::Inside SendCommand\n",__LINE__);
        eError = OMX_ErrorInvalidState;
        OMX_ERROR4(pCompPrivate->dbg, "%d ::OMX_AmrDecoder.c :: AMRDEC: Error Notofication \
                                         Sent to App\n",__LINE__);
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
    PERF_SendingCommand(pCompPrivate->pPERF, Cmd,
            (Cmd == OMX_CommandMarkBuffer) ? ((OMX_U32) pCmdData) : nParam,
            PERF_ModuleComponent);
#endif


    switch(Cmd) {
        case OMX_CommandStateSet:
            OMX_PRCOMM1(pCompPrivate->dbg, "%d ::OMX_AmrDecoder.c ::Inside SendCommand\n",__LINE__);
            OMX_PRSTATE2(pCompPrivate->dbg, "%d ::OMX_AmrDecoder.c ::pCompPrivate->curState = %d\n",__LINE__,pCompPrivate->curState);
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
                                     OMX_TI_ErrorMinor,
                                     NULL);
                    goto EXIT;
                }

                if(nParam == OMX_StateInvalid) {
                    OMX_PRCOMM1(pCompPrivate->dbg, "%d ::OMX_AmrDecoder.c ::Inside SendCommand\n",__LINE__);
                    pCompPrivate->curState = OMX_StateInvalid;
                    pCompPrivate->cbInfo.EventHandler (
                                     pHandle,
                                     pHandle->pApplicationPrivate,
                                     OMX_EventError,
                                     OMX_ErrorInvalidState,
                                     OMX_TI_ErrorMinor,
                                     NULL);
                    goto EXIT;
                }
            }
            break;
        case OMX_CommandFlush:
            OMX_PRCOMM1(pCompPrivate->dbg, "%d ::OMX_AmrDecoder.c ::Inside SendCommand\n",__LINE__);
            if(nParam > 1 && nParam != -1) {
                eError = OMX_ErrorBadPortIndex;
                goto EXIT;
            }

            break;
        case OMX_CommandPortDisable:
    OMX_PRCOMM1(pCompPrivate->dbg, "%d ::OMX_AmrDecoder.c ::Inside SendCommand\n",__LINE__);
            break;
        case OMX_CommandPortEnable:
    OMX_PRCOMM1(pCompPrivate->dbg, "%d ::OMX_AmrDecoder.c ::Inside SendCommand\n",__LINE__);
            break;
        case OMX_CommandMarkBuffer:
    OMX_PRCOMM1(pCompPrivate->dbg, "%d ::OMX_AmrDecoder.c ::Inside SendCommand\n",__LINE__);
            if (nParam > 0) {
                eError = OMX_ErrorBadPortIndex;
                goto EXIT;
            }
            break;
    default:
            OMX_ERROR2(pCompPrivate->dbg, "%d ::OMX_AmrDecoder.c :: AMRDEC: Command Received Default \
                                                      error\n",__LINE__);
            pCompPrivate->cbInfo.EventHandler (pHandle, 
                                                pHandle->pApplicationPrivate,
                                                OMX_EventError,
                                                OMX_ErrorUndefined,
                                                OMX_TI_ErrorMinor,
                                                "Invalid Command");
            break;

    }

    OMX_PRINT2(pCompPrivate->dbg, "%d ::OMX_AmrDecoder.c ::Inside SendCommand\n",__LINE__);
    nRet = write (pCompPrivate->cmdPipe[1], &Cmd, sizeof(Cmd));
    if (nRet == -1) {
        OMX_ERROR4(pCompPrivate->dbg, "%d ::OMX_AmrDecoder.c ::Inside SendCommand\n",__LINE__);
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

    OMX_PRINT2(pCompPrivate->dbg, "%d ::OMX_AmrDecoder.c ::Inside SendCommand\n",__LINE__);
    OMX_PRINT2(pCompPrivate->dbg, "%d ::OMX_AmrDecoder.c ::nRet = %ld\n",__LINE__,nRet);
    if (nRet == -1) {
        OMX_ERROR4(pCompPrivate->dbg, "%d ::OMX_AmrDecoder.c ::Inside SendCommand\n",__LINE__);
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
    OMX_PRINT1(pCompPrivate->dbg, "Returning from SendCommand\n");
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
    AMRDEC_COMPONENT_PRIVATE  *pComponentPrivate;
    OMX_PARAM_PORTDEFINITIONTYPE *pParameterStructure;



    pComponentPrivate = (AMRDEC_COMPONENT_PRIVATE *)(((OMX_COMPONENTTYPE*)hComp)->pComponentPrivate);
    OMX_PRINT1 (pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c :: Inside the GetParameter:: %x\n",__LINE__,nParamIndex);
    pParameterStructure = (OMX_PARAM_PORTDEFINITIONTYPE*)ComponentParameterStructure;

    if (pParameterStructure == NULL) {
        eError = OMX_ErrorBadParameter;
        goto EXIT;

    }
    OMX_PRINT2(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c ::pParameterStructure = %p\n",__LINE__,pParameterStructure);


    OMX_PRINT2(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c ::Inside the GetParameter Line\n",__LINE__);
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
                            OMX_TI_ErrorMinor,
                            NULL);
    OMX_PRINT2(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c ::ide the GetParameter\n",__LINE__);
    }
#endif
    OMX_PRINT2(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c ::Inside the GetParameter\n",__LINE__);
    switch(nParamIndex){
                case OMX_IndexParamAudioInit:
                        OMX_PRMGR1(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c ::OMX_IndexParamAudioInit\n",__LINE__);
                        memcpy(ComponentParameterStructure, &pComponentPrivate->sPortParam, sizeof(OMX_PORT_PARAM_TYPE));
                        break;

                case OMX_IndexParamPortDefinition:
                        OMX_PRINT2(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c ::pParameterStructure->nPortIndex = %ld\n",__LINE__,pParameterStructure->nPortIndex);
                        OMX_PRCOMM2(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c ::pComponentPrivate->pPortDef[NBAMRDEC_INPUT_PORT]->nPortIndex = %ld\n",__LINE__,pComponentPrivate->pPortDef[NBAMRDEC_INPUT_PORT]->nPortIndex);
                        if(((OMX_PARAM_PORTDEFINITIONTYPE *)(ComponentParameterStructure))->nPortIndex == 
                                pComponentPrivate->pPortDef[NBAMRDEC_INPUT_PORT]->nPortIndex) {
                                        OMX_PRINT2(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c ::Inside the GetParameter\n",__LINE__);
                                        memcpy(ComponentParameterStructure,pComponentPrivate->pPortDef[NBAMRDEC_INPUT_PORT], 
                                                sizeof(OMX_PARAM_PORTDEFINITIONTYPE));                                       
                        } 
                        else if(((OMX_PARAM_PORTDEFINITIONTYPE *)(ComponentParameterStructure))->nPortIndex == 
                                pComponentPrivate->pPortDef[NBAMRDEC_OUTPUT_PORT]->nPortIndex) {
                                        OMX_PRINT2(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c ::Inside the GetParameter\n",__LINE__);
                                        memcpy(ComponentParameterStructure, pComponentPrivate->pPortDef[NBAMRDEC_OUTPUT_PORT], 
                                                sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
                                } 
                                else {
                                        eError = OMX_ErrorBadPortIndex;
                                }
                        break;
                
                case OMX_IndexParamAudioPortFormat:         
            OMX_PRCOMM2(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c ::((OMX_AUDIO_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nPortIndex = %ld\n",__LINE__,((OMX_AUDIO_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nPortIndex);
            OMX_PRCOMM2(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c ::pComponentPrivate->sInPortFormat.nPortIndex= %ld\n",__LINE__,pComponentPrivate->sInPortFormat.nPortIndex);
            OMX_PRCOMM2(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c ::pComponentPrivate->sOutPortFormat.nPortIndex= %ld\n",__LINE__,pComponentPrivate->sOutPortFormat.nPortIndex);
            if(((OMX_AUDIO_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nPortIndex == pComponentPrivate->pPortDef[NBAMRDEC_INPUT_PORT]->nPortIndex) {
                if(((OMX_AUDIO_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nIndex > pComponentPrivate->sInPortFormat.nPortIndex) {
                                        eError = OMX_ErrorNoMore;
                                } 
            else {
                memcpy(ComponentParameterStructure, &pComponentPrivate->sInPortFormat,
                    sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
                                }
                        }
                        else if(((OMX_AUDIO_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nPortIndex == pComponentPrivate->pPortDef[NBAMRDEC_OUTPUT_PORT]->nPortIndex){
                                        OMX_PRCOMM1(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c ::Inside the GetParameter\n",__LINE__);
                                        if(((OMX_AUDIO_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nIndex > pComponentPrivate->sOutPortFormat.nPortIndex) {
                                                OMX_ERROR1(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c ::Inside the GetParameter\n",__LINE__);
                                                eError = OMX_ErrorNoMore;
                                        } 
                                        else {
                                                OMX_PRINT2(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c ::Inside the GetParameter\n",__LINE__);
                                                memcpy(ComponentParameterStructure, &pComponentPrivate->sOutPortFormat, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
                                        }
                                } 
        else {
            OMX_PRINT2(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c ::Inside the GetParameter\n",__LINE__);
                        eError = OMX_ErrorBadPortIndex;
                        }                
                break;

      case OMX_IndexParamAudioAmr:
                       if(((OMX_PARAM_PORTDEFINITIONTYPE *)(ComponentParameterStructure))->nPortIndex == 
                                pComponentPrivate->pPortDef[NBAMRDEC_INPUT_PORT]->nPortIndex) {
                                        OMX_PRINT2(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c ::Inside the GetParameter\n",__LINE__);
                                        memcpy(ComponentParameterStructure,pComponentPrivate->amrParams[NBAMRDEC_INPUT_PORT], 
                                                sizeof(OMX_AUDIO_PARAM_AMRTYPE));                                       
                        } 
                        else if(((OMX_PARAM_PORTDEFINITIONTYPE *)(ComponentParameterStructure))->nPortIndex == 
                                pComponentPrivate->pPortDef[NBAMRDEC_OUTPUT_PORT]->nPortIndex) {
                                        OMX_PRINT2(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c ::Inside the GetParameter\n",__LINE__);
                                        memcpy(ComponentParameterStructure, pComponentPrivate->amrParams[NBAMRDEC_OUTPUT_PORT], 
                                                sizeof(OMX_AUDIO_PARAM_AMRTYPE));
                                } 
                                else {
                                        eError = OMX_ErrorBadPortIndex;
                                }
                        break;

        case OMX_IndexParamAudioPcm:
                if(((OMX_AUDIO_PARAM_AMRTYPE *)(ComponentParameterStructure))->nPortIndex == NBAMRDEC_OUTPUT_PORT){
                      memcpy(ComponentParameterStructure, pComponentPrivate->amrParams[NBAMRDEC_OUTPUT_PORT], sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
                }
                else {
                      eError = OMX_ErrorBadPortIndex;
               }
               break;

             
        case OMX_IndexParamPriorityMgmt:
	     if (NULL == pComponentPrivate->pPriorityMgmt) {
	         eError = OMX_ErrorBadParameter;
	     }
             else {
                  memcpy(ComponentParameterStructure, pComponentPrivate->pPriorityMgmt, sizeof(OMX_PRIORITYMGMTTYPE));
	     }
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
                    OMX_PRINT2(pComponentPrivate->dbg, ":: OMX_ErrorBadPortIndex from GetParameter");
                    eError = OMX_ErrorBadPortIndex;
                }
                break;
             

        case OMX_IndexParamVideoInit:
                break;

         case OMX_IndexParamImageInit:
                break;

         case OMX_IndexParamOtherInit:
                break;

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
            OMX_PRDSP2(pComponentPrivate->dbg, "%d :: Copying PV_OMX_COMPONENT_CAPABILITY_TYPE_INDEX\n", __LINE__);
            memcpy(pCap_flags, &(pComponentPrivate->iPVCapabilityFlags), sizeof(PV_OMXComponentCapabilityFlagsType));
        eError = OMX_ErrorNone;
        }
                break;

        default:
            eError = OMX_ErrorUnsupportedIndex;
        
        break;
    }
EXIT:
    OMX_PRINT1(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c :: Exiting GetParameter:: %x\n",__LINE__,nParamIndex);
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

static OMX_ERRORTYPE SetParameter (OMX_HANDLETYPE hComp, OMX_INDEXTYPE nParamIndex, OMX_PTR pCompParam)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_BOOL temp_bEnabled, temp_bPopulated;
    OMX_COMPONENTTYPE* pHandle= (OMX_COMPONENTTYPE*)hComp;
    AMRDEC_COMPONENT_PRIVATE  *pComponentPrivate;
    OMX_PARAM_COMPONENTROLETYPE  *pRole;
    OMX_AUDIO_PARAM_PCMMODETYPE *amr_op;
    OMX_PARAM_BUFFERSUPPLIERTYPE sBufferSupplier;
    
    pComponentPrivate = (AMRDEC_COMPONENT_PRIVATE *)(((OMX_COMPONENTTYPE*)hComp)->pComponentPrivate);

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
            {
                OMX_AUDIO_PARAM_PORTFORMATTYPE *pComponentParam =
                                     (OMX_AUDIO_PARAM_PORTFORMATTYPE *)pCompParam;

                    OMX_PRCOMM2(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c ::pComponentParam->nPortIndex = %ld\n",__LINE__,pComponentParam->nPortIndex);
                    

                /* 0 means Input port */
                if (pComponentParam->nPortIndex == 0) {                  
                                                
                    memcpy(&pComponentPrivate->sInPortFormat, pComponentParam, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
                    
                } else if (pComponentParam->nPortIndex == 1) {
                    /* 1 means Output port */
                    memcpy(&pComponentPrivate->sOutPortFormat, pComponentParam, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));                    
                    
               }else {
                   OMX_ERROR4(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c :: Wrong Port Index Parameter\n", __LINE__);
                    OMX_ERROR4(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c :: About to return OMX_ErrorBadParameter\n", __LINE__);
                   eError = OMX_ErrorBadParameter;
                        OMX_PRINT2(pComponentPrivate->dbg, "WARNING: %s    %d\n", __FILE__,__LINE__);                   
                   goto EXIT;
               }
            }
            break;
        case OMX_IndexParamAudioAmr:
            {
                OMX_AUDIO_PARAM_AMRTYPE *pCompAmrParam = (OMX_AUDIO_PARAM_AMRTYPE *)pCompParam;

               if (OMX_AUDIO_AMRFrameFormatConformance == pCompAmrParam->eAMRFrameFormat)
                         pComponentPrivate->mimemode = 0;
               else if (OMX_AUDIO_AMRFrameFormatIF2 == pCompAmrParam->eAMRFrameFormat)
                         pComponentPrivate->mimemode = 2;
               else if (OMX_AUDIO_AMRFrameFormatRTPPayload == pCompAmrParam->eAMRFrameFormat) {
                         pComponentPrivate->mimemode = 1;
                         pComponentPrivate->using_rtsp=1;
               }
               else
                         pComponentPrivate->mimemode = 1; /*MIME Format*/

              pComponentPrivate->iAmrMode = pCompAmrParam->eAMRDTXMode;


                /* 0 means Input port */
                if(pCompAmrParam->nPortIndex == 0) {
                    memcpy(((AMRDEC_COMPONENT_PRIVATE*)
                            pHandle->pComponentPrivate)->amrParams[NBAMRDEC_INPUT_PORT],
                            pCompAmrParam, sizeof(OMX_AUDIO_PARAM_AMRTYPE));

                } else if (pCompAmrParam->nPortIndex == 1) {
                    /* 1 means Output port */
                    memcpy(((AMRDEC_COMPONENT_PRIVATE *)
                            pHandle->pComponentPrivate)->amrParams[NBAMRDEC_OUTPUT_PORT],
                            pCompAmrParam, sizeof(OMX_AUDIO_PARAM_AMRTYPE));
                }
                else {
                    eError = OMX_ErrorBadPortIndex;
                }
            }
            break;
        case OMX_IndexParamPortDefinition:
            if(((OMX_PARAM_PORTDEFINITIONTYPE *)(pCompParam))->nPortIndex ==
                                pComponentPrivate->pPortDef[NBAMRDEC_INPUT_PORT]->nPortIndex) {
                temp_bEnabled = pComponentPrivate->pPortDef[NBAMRDEC_INPUT_PORT]->bEnabled;
                temp_bPopulated = pComponentPrivate->pPortDef[NBAMRDEC_INPUT_PORT]->bPopulated;
                memcpy(pComponentPrivate->pPortDef[NBAMRDEC_INPUT_PORT],
                        pCompParam,
                        sizeof(OMX_PARAM_PORTDEFINITIONTYPE)
                      );
                 pComponentPrivate->pPortDef[NBAMRDEC_INPUT_PORT]->bEnabled = temp_bEnabled;
                 pComponentPrivate->pPortDef[NBAMRDEC_INPUT_PORT]->bPopulated = temp_bPopulated;

            }
            else if(((OMX_PARAM_PORTDEFINITIONTYPE *)(pCompParam))->nPortIndex ==
                              pComponentPrivate->pPortDef[NBAMRDEC_OUTPUT_PORT]->nPortIndex) {
                temp_bEnabled = pComponentPrivate->pPortDef[NBAMRDEC_OUTPUT_PORT]->bEnabled;
                temp_bPopulated = pComponentPrivate->pPortDef[NBAMRDEC_OUTPUT_PORT]->bPopulated;
                memcpy(pComponentPrivate->pPortDef[NBAMRDEC_OUTPUT_PORT],
                        pCompParam,
                       sizeof(OMX_PARAM_PORTDEFINITIONTYPE)
                      );
                 pComponentPrivate->pPortDef[NBAMRDEC_OUTPUT_PORT]->bEnabled = temp_bEnabled;
                 pComponentPrivate->pPortDef[NBAMRDEC_OUTPUT_PORT]->bPopulated = temp_bPopulated;
            }
            else {
                eError = OMX_ErrorBadPortIndex;
            }
        break;
        case OMX_IndexParamPriorityMgmt:
            if (pComponentPrivate->curState == OMX_StateIdle ){
                 eError = OMX_ErrorIncorrectStateOperation;
                 break;
            }
	    if (NULL == pComponentPrivate->pPriorityMgmt) {
	        eError = OMX_ErrorBadParameter;
	    }
	    else {
	        memcpy(pComponentPrivate->pPriorityMgmt, (OMX_PRIORITYMGMTTYPE*)pCompParam, sizeof(OMX_PRIORITYMGMTTYPE));
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
                 amr_op = (OMX_AUDIO_PARAM_PCMMODETYPE *)pCompParam;
                 memcpy(pComponentPrivate->amrParams[NBAMRDEC_OUTPUT_PORT], amr_op, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
        }
        else{
            eError = OMX_ErrorBadParameter;
        }
        break;

        case OMX_IndexParamCompBufferSupplier:             
            if(((OMX_PARAM_PORTDEFINITIONTYPE *)(pCompParam))->nPortIndex ==
                                    pComponentPrivate->pPortDef[NBAMRDEC_INPUT_PORT]->nPortIndex) {
                    OMX_PRBUFFER2(pComponentPrivate->dbg, ":: SetParameter OMX_IndexParamCompBufferSupplier \n");
                                   sBufferSupplier.eBufferSupplier = OMX_BufferSupplyInput;
                                   memcpy(&sBufferSupplier, pCompParam, sizeof(OMX_PARAM_BUFFERSUPPLIERTYPE));                                 
                    
                }
                else if(((OMX_PARAM_PORTDEFINITIONTYPE *)(pCompParam))->nPortIndex ==
                                  pComponentPrivate->pPortDef[NBAMRDEC_OUTPUT_PORT]->nPortIndex) {
                    OMX_PRBUFFER2(pComponentPrivate->dbg, ":: SetParameter OMX_IndexParamCompBufferSupplier \n");
                    sBufferSupplier.eBufferSupplier = OMX_BufferSupplyOutput;
                    memcpy(&sBufferSupplier, pCompParam, sizeof(OMX_PARAM_BUFFERSUPPLIERTYPE));
                } 
                else {
                    OMX_PRINT2(pComponentPrivate->dbg, ":: OMX_ErrorBadPortIndex from SetParameter");
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

    AMRDEC_COMPONENT_PRIVATE *pComponentPrivate;
    TI_OMX_STREAM_INFO *streamInfo;

    OMX_MALLOC_GENERIC(streamInfo, TI_OMX_STREAM_INFO);

    pComponentPrivate = (AMRDEC_COMPONENT_PRIVATE *)
            (((OMX_COMPONENTTYPE*)hComp)->pComponentPrivate);

#ifdef _ERROR_PROPAGATION__
    if (pComponentPrivate->curState == OMX_StateInvalid){
        eError = OMX_ErrorInvalidState;
        goto EXIT;
    }
#endif
    if (NULL == ComponentConfigStructure) {
        eError = OMX_ErrorBadParameter;
	goto EXIT;
    }
    if(nConfigIndex == OMX_IndexCustomNbAmrDecStreamIDConfig)
    {
        /* copy component info */
        streamInfo->streamId = pComponentPrivate->streamID;
        memcpy(ComponentConfigStructure,streamInfo,sizeof(TI_OMX_STREAM_INFO));
    }
    else if(nConfigIndex == OMX_IndexCustomDebug)
    {
        OMX_DBG_GETCONFIG(pComponentPrivate->dbg, ComponentConfigStructure);
    }

EXIT:
    OMX_MEMFREE_STRUCT(streamInfo);
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
    if (pHandle == NULL) {
	 OMXDBG_PRINT(stderr, ERROR, 2, 0, "%d ::OMX_AmrDecoder.c :: About to return OMX_ErrorBadParameter\n", __LINE__);
	 eError = OMX_ErrorBadParameter;
	 goto EXIT;
    }
    AMRDEC_COMPONENT_PRIVATE *pComponentPrivate =
                         (AMRDEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    OMX_S16 *customFlag = NULL;
    TI_OMX_DSP_DEFINITION *configData;
    int flagValue=0;
     TI_OMX_DATAPATH dataPath;
#ifdef DSP_RENDERING_ON
    OMX_AUDIO_CONFIG_MUTETYPE *pMuteStructure = NULL;
    OMX_AUDIO_CONFIG_VOLUMETYPE *pVolumeStructure = NULL;
#endif
    OMX_PRINT1(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c :: Entering SetConfig\n", __LINE__);

#ifdef _ERROR_PROPAGATION__
    if (pComponentPrivate->curState == OMX_StateInvalid){
        eError = OMX_ErrorInvalidState;
        goto EXIT;
    }
#endif

    switch (nConfigIndex) {
        case  OMX_IndexCustomNbAmrDecHeaderInfoConfig:
        {
              OMX_PRDSP2(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c :: SetConfig OMX_IndexCustomNbAmrDecHeaderInfoConfig \n",__LINE__);
              configData = (TI_OMX_DSP_DEFINITION*)ComponentConfigStructure;
              if (configData == NULL) {
                eError = OMX_ErrorBadParameter;
                OMX_ERROR4(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c :: OMX_ErrorBadParameter from SetConfig\n",__LINE__);
                goto EXIT;
              }
              pComponentPrivate->acdnmode = configData->acousticMode;
              if (configData->dasfMode == 0) {
                  pComponentPrivate->dasfmode = 0;
              }
              else if (configData->dasfMode == 1) {
                  pComponentPrivate->dasfmode = 1;
              }
              else if (configData->dasfMode == 2) {
                  pComponentPrivate->dasfmode = 1;
              }
              if (pComponentPrivate->dasfmode ){
                    pComponentPrivate->pPortDef[NBAMRDEC_OUTPUT_PORT]->bEnabled = 0;
              }

              pComponentPrivate->streamID = configData->streamId;

              break;
        }
        case  OMX_IndexCustomNbAmrDecDataPath:
            customFlag = (OMX_S16*)ComponentConfigStructure;
            if (customFlag == NULL) {
                eError = OMX_ErrorBadParameter;
                goto EXIT;
            }

            dataPath = *customFlag;

            switch(dataPath) {
                case DATAPATH_APPLICATION:
                    /*strcpy((char*)pComponentPrivate->sDeviceString,(char*)ETEEDN_STRING);*/
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
        case OMX_IndexCustomNbAmrDecModeEfrConfig:
        {
            OMX_PRINT2(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c :: SetConfig OMX_IndexCustomNbAmrDecModeEfrConfig \n",__LINE__);
            customFlag = (OMX_S16*)ComponentConfigStructure;
            if (customFlag == NULL) {
                eError = OMX_ErrorBadParameter;
                OMX_ERROR4(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c :: OMX_ErrorBadParameter from SetConfig\n",__LINE__);
                goto EXIT;
            }
            pComponentPrivate->iAmrMode = *customFlag;
            break;
        }
        case OMX_IndexCustomNbAmrDecModeDasfConfig:
        {
            OMX_PRDSP2(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c :: SetConfig OMX_IndexCustomNbAmrDecModeDasfConfig \n",__LINE__);
            customFlag = (OMX_S16*)ComponentConfigStructure;
            if (customFlag == NULL) {
                eError = OMX_ErrorBadParameter;
                OMX_ERROR4(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c :: OMX_ErrorBadParameter from SetConfig\n",__LINE__);
                goto EXIT;
            }

            flagValue = *customFlag;
            if (flagValue == 0) {
                pComponentPrivate->dasfmode = 0;
            }
            else if (flagValue == 1) {
                pComponentPrivate->dasfmode = 1;
            }
            else if (flagValue == 2) {
                pComponentPrivate->dasfmode = 1;
            }
            OMX_PRDSP2(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c ::pComponentPrivate->dasfmode = %d\n",__LINE__,pComponentPrivate->dasfmode);
            if (pComponentPrivate->dasfmode ){
               pComponentPrivate->pPortDef[NBAMRDEC_OUTPUT_PORT]->bEnabled = 0;
            }
            break;
        }
        case OMX_IndexCustomNbAmrDecModeMimeConfig:
        {
            OMX_PRINT2(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c :: SetConfig OMX_IndexCustomNbAmrDecModeMimeConfig \n",__LINE__);
            customFlag = (OMX_S16*)ComponentConfigStructure;
            if (customFlag == NULL) 
            {
                eError = OMX_ErrorBadParameter;
                OMX_ERROR4(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c :: OMX_ErrorBadParameter from SetConfig\n",__LINE__);
                goto EXIT;
            }
            pComponentPrivate->mimemode = *customFlag;
            OMX_PRDSP2(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c ::pComponentPrivate->mimemode = %d\n",__LINE__,pComponentPrivate->mimemode);
            break;
        }
        case OMX_IndexCustomNbAmrDecNextFrameLost:
        {
             pComponentPrivate->bFrameLost=OMX_TRUE;
             break;
        }
        case OMX_IndexConfigAudioMute:
        {
#ifdef DSP_RENDERING_ON
            pMuteStructure = (OMX_AUDIO_CONFIG_MUTETYPE *)ComponentConfigStructure;
             OMX_PRSTATE2(pComponentPrivate->dbg, "Set Mute/Unmute for playback stream\n"); 
             cmd_data.hComponent = hComp;
             if(pMuteStructure->bMute == OMX_TRUE)
             {
                 OMX_PRSTATE2(pComponentPrivate->dbg, "Mute the playback stream\n");
                 cmd_data.AM_Cmd = AM_CommandStreamMute;
             }
             else
             {
                 OMX_PRSTATE2(pComponentPrivate->dbg, "unMute the playback stream\n");
                 cmd_data.AM_Cmd = AM_CommandStreamUnMute;
             }
             cmd_data.param1 = 0;
             cmd_data.param2 = 0;
             cmd_data.streamID = pComponentPrivate->streamID;
             if((write(pComponentPrivate->fdwrite, &cmd_data, sizeof(cmd_data)))<0)
             {
                 OMX_ERROR4(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c ::[NBAMR decoder] - fail to send Mute command to audio manager\n",__LINE__);
                 OMX_ERROR4(pComponentPrivate->dbg, "Failed to write the Audio Manager output pipe\n");
             }

             break;
#endif
        }
        case OMX_IndexConfigAudioVolume:
        {
#ifdef DSP_RENDERING_ON
             pVolumeStructure = (OMX_AUDIO_CONFIG_VOLUMETYPE *)ComponentConfigStructure;
             OMX_PRSTATE2(pComponentPrivate->dbg, "Set volume for playback stream\n"); 
             cmd_data.hComponent = hComp;
             cmd_data.AM_Cmd = AM_CommandSWGain;
             cmd_data.param1 = pVolumeStructure->sVolume.nValue;
             cmd_data.param2 = 0;
             cmd_data.streamID = pComponentPrivate->streamID;

             if((write(pComponentPrivate->fdwrite, &cmd_data, sizeof(cmd_data)))<0)
             {
                 OMX_ERROR4(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c ::[NBAMR decoder] - fail to send Volume command to audio manager\n",__LINE__);
             }

             break;
#endif
        }

    case OMX_IndexCustomDebug: 
         OMX_DBG_SETCONFIG(pComponentPrivate->dbg, ComponentConfigStructure);
         break;

        default:
            eError = OMX_ErrorUnsupportedIndex;
        break;
    }
EXIT:
    OMX_PRINT1(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c :: Exiting SetConfig\n", __LINE__);
    OMX_PRINT1(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c :: Returning = 0x%x\n",__LINE__,eError);
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
        OMXDBG_PRINT(stderr, ERROR, 4, 0, "%d ::OMX_AmrDecoder.c :: About to return OMX_ErrorBadParameter\n",__LINE__);
        goto EXIT;
    }

    if (pHandle && pHandle->pComponentPrivate) {
        *pState =  ((AMRDEC_COMPONENT_PRIVATE*)
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
    AMRDEC_COMPONENT_PRIVATE *pComponentPrivate =
                         (AMRDEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef;

    ssize_t ret;
    pPortDef = ((AMRDEC_COMPONENT_PRIVATE*)
                    pComponentPrivate)->pPortDef[NBAMRDEC_INPUT_PORT];


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

    if(!pPortDef->bEnabled) {
        OMX_ERROR4(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c :: About to return OMX_ErrorIncorrectStateOperation\n",__LINE__);
        eError = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }

    if (pBuffer == NULL) {
        eError = OMX_ErrorBadParameter;
        OMX_ERROR4(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c :: About to return OMX_ErrorBadParameter\n",__LINE__);
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

    if (pBuffer->nInputPortIndex != NBAMRDEC_INPUT_PORT) {
        eError  = OMX_ErrorBadPortIndex;
        goto EXIT;
    }

    OMX_PRSTATE2(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c :: pComponentPrivate->curState = %d\n",__LINE__,pComponentPrivate->curState);
    if(pComponentPrivate->curState != OMX_StateExecuting && pComponentPrivate->curState != OMX_StatePause) {
        OMX_ERROR4(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c :: About to return OMX_ErrorIncorrectStateOperation\n",__LINE__);
        eError = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }



    OMX_PRBUFFER2(pComponentPrivate->dbg, "\n------------------------------------------\n\n");
    OMX_PRBUFFER2(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c :: Component Sending Filled ip buff %p \
                             to Component Thread\n",__LINE__,pBuffer);
    OMX_PRBUFFER2(pComponentPrivate->dbg, "\n------------------------------------------\n\n");


    pComponentPrivate->app_nBuf--;

    pComponentPrivate->pMarkData = pBuffer->pMarkData;
    pComponentPrivate->hMarkTargetComponent = pBuffer->hMarkTargetComponent;
    
    pComponentPrivate->nUnhandledEmptyThisBuffers++;

    ret = write (pComponentPrivate->dataPipe[1], &pBuffer,
                                       sizeof(OMX_BUFFERHEADERTYPE*));
    if (ret == -1) {
        OMX_ERROR4(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c :: Error in Writing to the Data pipe\n", __LINE__);
        eError = OMX_ErrorHardware;
        goto EXIT;
    }
    pComponentPrivate->nEmptyThisBufferCount++;

EXIT:
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
    AMRDEC_COMPONENT_PRIVATE *pComponentPrivate =
                         (AMRDEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef;
    OMX_PRBUFFER2(pComponentPrivate->dbg, "\n------------------------------------------\n\n");
    OMX_PRBUFFER2(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c :: Component Sending Emptied op buff %p \
                             to Component Thread\n",__LINE__,pBuffer);
    OMX_PRBUFFER2(pComponentPrivate->dbg, "\n------------------------------------------\n\n");

    pPortDef = ((AMRDEC_COMPONENT_PRIVATE*)
                    pComponentPrivate)->pPortDef[NBAMRDEC_OUTPUT_PORT];

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

    if(!pPortDef->bEnabled) {
        OMX_ERROR4(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c :: About to return OMX_ErrorIncorrectStateOperation\n",__LINE__);
        eError = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }

    if (pBuffer == NULL) {
        eError = OMX_ErrorBadParameter;
        OMX_ERROR4(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c :: About to return OMX_ErrorBadParameter\n",__LINE__);
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

    if (pBuffer->nOutputPortIndex != NBAMRDEC_OUTPUT_PORT) {
        eError  = OMX_ErrorBadPortIndex;
        goto EXIT;
    }

    OMX_PRSTATE2(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c :: pComponentPrivate->curState = %d\n",__LINE__,pComponentPrivate->curState);
    if(pComponentPrivate->curState != OMX_StateExecuting && pComponentPrivate->curState != OMX_StatePause) {
        OMX_PRINT2(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c :: About to return OMX_ErrorIncorrectStateOperation\n",__LINE__);
        eError = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }

    OMX_PRBUFFER2(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c :: FillThisBuffer\n",__LINE__);
    pComponentPrivate->app_nBuf--;
    OMX_PRBUFFER2(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c :: Decrementing app_nBuf = %ld\n",__LINE__,pComponentPrivate->app_nBuf);

    OMX_PRBUFFER2(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c :: pComponentPrivate->pMarkBuf = 0x%p\n",__LINE__,pComponentPrivate->pMarkBuf);
    OMX_PRBUFFER2(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c :: pComponentPrivate->pMarkData = 0x%p\n",__LINE__,pComponentPrivate->pMarkData);
    if(pComponentPrivate->pMarkBuf){
        OMX_PRBUFFER2(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c :: FillThisBuffer\n",__LINE__);
        pBuffer->hMarkTargetComponent = pComponentPrivate->pMarkBuf->hMarkTargetComponent;
        pBuffer->pMarkData = pComponentPrivate->pMarkBuf->pMarkData;
        pComponentPrivate->pMarkBuf = NULL;
    }

    if (pComponentPrivate->pMarkData) {
        OMX_PRBUFFER2(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c :: FillThisBuffer\n",__LINE__);
        pBuffer->hMarkTargetComponent = pComponentPrivate->hMarkTargetComponent;
        pBuffer->pMarkData = pComponentPrivate->pMarkData;
        pComponentPrivate->pMarkData = NULL;
    }

    pComponentPrivate->nUnhandledFillThisBuffers++;

    write (pComponentPrivate->dataPipe[1], &pBuffer,
                                      sizeof (OMX_BUFFERHEADERTYPE*));
    pComponentPrivate->nFillThisBufferCount++;

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
    AMRDEC_COMPONENT_PRIVATE *pComponentPrivate =
                         (AMRDEC_COMPONENT_PRIVATE *)pComponent->pComponentPrivate;
    struct OMX_TI_Debug dbg;
    dbg = pComponentPrivate->dbg;

    OMX_PRINT1(dbg, "%d ::OMX_AmrDecoder.c ::ComponentDeInit\n",__LINE__);

#ifdef __PERF_INSTRUMENTATION__
    PERF_Boundary(pComponentPrivate->pPERF,
                  PERF_BoundaryStart | PERF_BoundaryCleanup);
#endif
#ifdef DSP_RENDERING_ON
    close(pComponentPrivate->fdwrite);
    close(pComponentPrivate->fdread);
#endif
#ifdef RESOURCE_MANAGER_ENABLED
    eError = RMProxy_NewSendCommand(pHandle, RMProxy_FreeResource, OMX_NBAMR_Decoder_COMPONENT, 0, 3456,NULL);
    if (eError != OMX_ErrorNone) {
         OMX_ERROR4(dbg, "%d ::OMX_AmrDecoder.c :: Error returned from destroy ResourceManagerProxy thread\n",
                                                        __LINE__);
    }
    eError = RMProxy_Deinitalize();
    if (eError != OMX_ErrorNone) {
         OMX_ERROR4(dbg, "%d ::OMX_AmrDecoder.c :: Error from RMProxy_Deinitalize\n",
                                                        __LINE__);
    }
/*RM END*/
#endif
    OMX_PRINT2(dbg, "%d ::OMX_AmrDecoder.c ::ComponentDeInit\n",__LINE__);
    pComponentPrivate->bIsStopping = 1;
    eError = NBAMRDEC_StopComponentThread(pHandle);
    OMX_PRINT2(dbg, "%d ::OMX_AmrDecoder.c ::ComponentDeInit\n",__LINE__);
    /* Wait for thread to exit so we can get the status into "error" */

    OMX_MEMFREE_STRUCT(pComponentPrivate->pInputBufferList);

    OMX_MEMFREE_STRUCT(pComponentPrivate->pOutputBufferList);
    
    /* close the pipe handles */
    NBAMRDEC_FreeCompResources(pHandle);
    
#ifdef __PERF_INSTRUMENTATION__
    PERF_Boundary(pComponentPrivate->pPERF,
                  PERF_BoundaryComplete | PERF_BoundaryCleanup);
    PERF_Done(pComponentPrivate->pPERF);
#endif
    OMX_MEMFREE_STRUCT(pComponentPrivate->sDeviceString);
    OMX_PRINT1(dbg, "%d ::After NBAMRDEC_FreeCompResources\n",__LINE__);
    OMX_MEMFREE_STRUCT(pComponentPrivate);
      
    OMX_PRINT1(dbg, "%d ::After OMX_MEMFREE_STRUCT(pComponentPrivate)\n",__LINE__);
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
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMXDBG_PRINT(stderr, PRINT, 1, 0, "Inside the ComponentTunnelRequest\n");
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
    AMRDEC_COMPONENT_PRIVATE *pComponentPrivate;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_BUFFERHEADERTYPE *pBufferHeader;

    pComponentPrivate = (AMRDEC_COMPONENT_PRIVATE *)
            (((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);
    OMX_PRINT1 (pComponentPrivate->dbg, "%s: ALLOCATE BUFFER", __FUNCTION__);
    pPortDef = ((AMRDEC_COMPONENT_PRIVATE*)
                    pComponentPrivate)->pPortDef[nPortIndex];
#ifdef _ERROR_PROPAGATION__
    if (pComponentPrivate->curState == OMX_StateInvalid){
    OMX_PRBUFFER1(pComponentPrivate->dbg, "AllocateBuffer %d\n",__LINE__);

        eError = OMX_ErrorInvalidState;
        goto EXIT;
    }
#endif
    OMX_PRCOMM2(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c :: pPortDef = 0x%p\n", __LINE__,pPortDef);
    OMX_PRCOMM2(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c :: pPortDef->bEnabled = %d\n", __LINE__,pPortDef->bEnabled);


    OMX_PRCOMM2(pComponentPrivate->dbg, "pPortDef->bEnabled = %d\n", pPortDef->bEnabled);
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

    OMX_MALLOC_SIZE_DSPALIGN(pBufferHeader->pBuffer, nSizeBytes,OMX_U8);

    if (nPortIndex == NBAMRDEC_INPUT_PORT) {
        pBufferHeader->nInputPortIndex = nPortIndex;
        pBufferHeader->nOutputPortIndex = -1;
        pComponentPrivate->pInputBufferList->pBufHdr[pComponentPrivate->pInputBufferList->numBuffers] = pBufferHeader;
        pComponentPrivate->pInputBufferList->bBufferPending[pComponentPrivate->pInputBufferList->numBuffers] = 0;
        OMX_PRBUFFER2(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c :: pComponentPrivate->pInputBufferList->pBufHdr[%d] = %p\n",__LINE__,pComponentPrivate->pInputBufferList->numBuffers,pComponentPrivate->pInputBufferList->pBufHdr[pComponentPrivate->pInputBufferList->numBuffers]);
        pComponentPrivate->pInputBufferList->bufferOwner[pComponentPrivate->pInputBufferList->numBuffers++] = 1;
        OMX_PRBUFFER2(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c :: Allocate Buffer Line \n",__LINE__);
        OMX_PRBUFFER2(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c :: pComponentPrivate->pInputBufferList->numBuffers = %d\n",__LINE__,pComponentPrivate->pInputBufferList->numBuffers);
        OMX_PRBUFFER2(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c :: pPortDef->nBufferCountMin = %ld\n",__LINE__,pPortDef->nBufferCountMin);
        if (pComponentPrivate->pInputBufferList->numBuffers == pPortDef->nBufferCountActual) {
            OMX_PRCOMM2(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c :: Setting pPortDef->bPopulated = OMX_TRUE for input port\n",__LINE__);
            pPortDef->bPopulated = OMX_TRUE;
        }
    }
    else if (nPortIndex == NBAMRDEC_OUTPUT_PORT) {
                pBufferHeader->nInputPortIndex = -1;
        pBufferHeader->nOutputPortIndex = nPortIndex; 
        OMX_MALLOC_GENERIC(pBufferHeader->pOutputPortPrivate, NBAMRDEC_BUFDATA);
        pComponentPrivate->pOutputBufferList->pBufHdr[pComponentPrivate->pOutputBufferList->numBuffers] = pBufferHeader;
        pComponentPrivate->pOutputBufferList->bBufferPending[pComponentPrivate->pOutputBufferList->numBuffers] = 0;
        OMX_PRBUFFER2(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c :: pComponentPrivate->pOutputBufferList->pBufHdr[%d] = %p\n",__LINE__,pComponentPrivate->pOutputBufferList->numBuffers,pComponentPrivate->pOutputBufferList->pBufHdr[pComponentPrivate->pOutputBufferList->numBuffers]);
        pComponentPrivate->pOutputBufferList->bufferOwner[pComponentPrivate->pOutputBufferList->numBuffers++] = 1;
                if (pComponentPrivate->pOutputBufferList->numBuffers == pPortDef->nBufferCountActual) {
            OMX_PRCOMM2(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c :: Setting pPortDef->bPopulated = OMX_TRUE for input port\n",__LINE__);
            pPortDef->bPopulated = OMX_TRUE;
        }
    }
    else {
        eError = OMX_ErrorBadPortIndex;
        goto EXIT;
    }
    /* Removing sleep() calls. Input buffer enabled and populated as well as output buffer. */
    if((pComponentPrivate->pPortDef[NBAMRDEC_OUTPUT_PORT]->bPopulated == pComponentPrivate->pPortDef[NBAMRDEC_OUTPUT_PORT]->bEnabled)&&
       (pComponentPrivate->pPortDef[NBAMRDEC_INPUT_PORT]->bPopulated == pComponentPrivate->pPortDef[NBAMRDEC_INPUT_PORT]->bEnabled) &&
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
    pBufferHeader->nVersion.s.nVersionMajor = AMRDEC_MAJOR_VER;
    pBufferHeader->nVersion.s.nVersionMinor = AMRDEC_MINOR_VER;
    pComponentPrivate->nVersion = pBufferHeader->nVersion.nVersion;


    pBufferHeader->nSize = sizeof(OMX_BUFFERHEADERTYPE);
    OMX_PRINT2(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c :: \n",__LINE__);
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
    OMX_PRINT1(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c :: AllocateBuffer returning %d\n",__LINE__,eError);
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
    AMRDEC_COMPONENT_PRIVATE * pComponentPrivate = NULL;
    OMX_BUFFERHEADERTYPE* buff;
    OMX_S16 i;
    OMX_S16 inputIndex = -1;
    OMX_S16 outputIndex = -1;
    OMX_COMPONENTTYPE *pHandle;

    pComponentPrivate = (AMRDEC_COMPONENT_PRIVATE *)
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
#ifdef __PERF_INSTRUMENTATION__
            PERF_SendingBuffer(pComponentPrivate->pPERF,
                 pComponentPrivate->pInputBufferList->pBufHdr[inputIndex]->pBuffer,
                 pComponentPrivate->pInputBufferList->pBufHdr[inputIndex]->nAllocLen,
                 PERF_ModuleMemory);
#endif
            if (pComponentPrivate->pInputBufferList->bufferOwner[inputIndex] == 1) {
                OMX_MEMFREE_STRUCT_DSPALIGN(pComponentPrivate->pInputBufferList->pBufHdr[inputIndex]->pBuffer, OMX_U8);
            }
            OMX_MEMFREE_STRUCT(pComponentPrivate->pInputBufferList->pBufHdr[inputIndex]);
            pComponentPrivate->pInputBufferList->numBuffers--;
            if (pComponentPrivate->pInputBufferList->numBuffers <
                pComponentPrivate->pPortDef[NBAMRDEC_INPUT_PORT]->nBufferCountActual) {
                OMX_PRCOMM2(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c ::setting input port populated to OMX_FALSE\n",__LINE__);
                pComponentPrivate->pPortDef[NBAMRDEC_INPUT_PORT]->bPopulated = OMX_FALSE;
            }
            if(pComponentPrivate->pPortDef[NBAMRDEC_INPUT_PORT]->bEnabled && 
               pComponentPrivate->bLoadedCommandPending == OMX_FALSE &&
                (pComponentPrivate->curState == OMX_StateIdle || 
                 pComponentPrivate->curState == OMX_StateExecuting || 
                 pComponentPrivate->curState == OMX_StatePause)) {
                 pComponentPrivate->cbInfo.EventHandler(
                        pHandle, pHandle->pApplicationPrivate,
                        OMX_EventError, OMX_ErrorPortUnpopulated,OMX_TI_ErrorMinor, "Input Port Unpopulated");
            }
        }
        else if (outputIndex != -1) {
#ifdef __PERF_INSTRUMENTATION__
                PERF_SendingBuffer(pComponentPrivate->pPERF,
                       pComponentPrivate->pOutputBufferList->pBufHdr[outputIndex]->pBuffer,
                       pComponentPrivate->pOutputBufferList->pBufHdr[outputIndex]->nAllocLen,
                       PERF_ModuleMemory);
#endif
            if (pComponentPrivate->pOutputBufferList->bufferOwner[outputIndex] == 1) {
                OMX_MEMFREE_STRUCT_DSPALIGN(pComponentPrivate->pOutputBufferList->pBufHdr[outputIndex]->pBuffer,OMX_U8);
            }
            OMX_MEMFREE_STRUCT(pComponentPrivate->pOutputBufferList->pBufHdr[outputIndex]->pOutputPortPrivate);
            OMX_MEMFREE_STRUCT(pComponentPrivate->pOutputBufferList->pBufHdr[outputIndex]);
            pComponentPrivate->pOutputBufferList->pBufHdr[outputIndex] = NULL;
            pComponentPrivate->pOutputBufferList->numBuffers--;

            if (pComponentPrivate->pOutputBufferList->numBuffers <
                pComponentPrivate->pPortDef[NBAMRDEC_OUTPUT_PORT]->nBufferCountActual) {
                OMX_PRCOMM2(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c ::setting output port populated to OMX_FALSE\n",__LINE__);
                pComponentPrivate->pPortDef[NBAMRDEC_OUTPUT_PORT]->bPopulated = OMX_FALSE;
            }
            if(pComponentPrivate->pPortDef[NBAMRDEC_OUTPUT_PORT]->bEnabled &&
               pComponentPrivate->bLoadedCommandPending == OMX_FALSE &&
               (pComponentPrivate->curState == OMX_StateIdle || 
                pComponentPrivate->curState == OMX_StateExecuting || 
                pComponentPrivate->curState == OMX_StatePause)) {
                pComponentPrivate->cbInfo.EventHandler(
                        pHandle, pHandle->pApplicationPrivate,
                        OMX_EventError, OMX_ErrorPortUnpopulated,OMX_TI_ErrorMinor, "Output Port Unpopulated");
            }
        }
        else {
            OMX_ERROR4(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c ::Returning OMX_ErrorBadParameter\n",__LINE__);
            eError = OMX_ErrorBadParameter;
        }
       /* Removing sleep() calls.  There are no allocated buffers. */
       if ((!pComponentPrivate->pInputBufferList->numBuffers &&
            !pComponentPrivate->pOutputBufferList->numBuffers) &&
            pComponentPrivate->InIdle_goingtoloaded)
       {
#ifndef UNDER_CE
           pthread_mutex_lock(&pComponentPrivate->InIdle_mutex);
           pthread_cond_signal(&pComponentPrivate->InIdle_threshold);
           pthread_mutex_unlock(&pComponentPrivate->InIdle_mutex);
#else
           OMX_SignalEvent(&(pComponentPrivate->InIdle_event));
#endif

pComponentPrivate->InIdle_goingtoloaded = 0;

       }
        OMX_PRINT2(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c ::pComponentPrivate->bDisableCommandPending = %ld\n",__LINE__,pComponentPrivate->bDisableCommandPending);
/*      if (pComponentPrivate->bDisableCommandPending) {*/
if (pComponentPrivate->bDisableCommandPending && (pComponentPrivate->pInputBufferList->numBuffers + pComponentPrivate->pOutputBufferList->numBuffers == 0)) {
            if (pComponentPrivate->pInputBufferList->numBuffers + pComponentPrivate->pOutputBufferList->numBuffers == 0) {
                SendCommand (pComponentPrivate->pHandle,OMX_CommandPortDisable,pComponentPrivate->bDisableCommandParam,NULL);
            }
        }
    OMX_PRINT1(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c :: Exiting FreeBuffer\n", __LINE__);
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
    AMRDEC_COMPONENT_PRIVATE *pComponentPrivate;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_BUFFERHEADERTYPE *pBufferHeader;

    pComponentPrivate = (AMRDEC_COMPONENT_PRIVATE *)
            (((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);

#ifdef _ERROR_PROPAGATION__
    if (pComponentPrivate->curState == OMX_StateInvalid){
        eError = OMX_ErrorInvalidState;
        goto EXIT;
    }

#endif

    pPortDef = ((AMRDEC_COMPONENT_PRIVATE*)
                    pComponentPrivate)->pPortDef[nPortIndex];
    OMX_PRCOMM2(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c ::pPortDef->bPopulated = %d\n",__LINE__,pPortDef->bPopulated);

    if(!pPortDef->bEnabled) {
        OMX_ERROR4(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c :: In AllocateBuffer\n", __LINE__);
        eError = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }

    /*if(nSizeBytes != pPortDef->nBufferSize || pPortDef->bPopulated) {
        OMX_PRINT1(pComponentPrivate->dbg, "%d ::OMX_AmrDecoder.c :: In AllocateBuffer\n", __LINE__);
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }*/

    OMX_MALLOC_GENERIC(pBufferHeader, OMX_BUFFERHEADERTYPE); 

    if (nPortIndex == NBAMRDEC_OUTPUT_PORT) {
        pBufferHeader->nInputPortIndex = -1;
        pBufferHeader->nOutputPortIndex = nPortIndex;
        //pBufferHeader->pOutputPortPrivate = (NBAMRDEC_BUFDATA*) newmalloc(sizeof(NBAMRDEC_BUFDATA));
        OMX_MALLOC_GENERIC(pBufferHeader->pOutputPortPrivate, NBAMRDEC_BUFDATA);
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
    /* Removing sleep() calls. All enabled buffers are populated. */
    if((pComponentPrivate->pPortDef[NBAMRDEC_OUTPUT_PORT]->bPopulated == pComponentPrivate->pPortDef[NBAMRDEC_OUTPUT_PORT]->bEnabled)&&
       (pComponentPrivate->pPortDef[NBAMRDEC_INPUT_PORT]->bPopulated == pComponentPrivate->pPortDef[NBAMRDEC_INPUT_PORT]->bEnabled) &&
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
    /* Removing sleep() calls.  */

    pBufferHeader->pAppPrivate = pAppPrivate;
    pBufferHeader->pPlatformPrivate = pComponentPrivate;
    pBufferHeader->nAllocLen = nSizeBytes;
    pBufferHeader->nVersion.s.nVersionMajor = AMRDEC_MAJOR_VER;
    pBufferHeader->nVersion.s.nVersionMinor = AMRDEC_MINOR_VER;
    pComponentPrivate->nVersion = pBufferHeader->nVersion.nVersion;
    pBufferHeader->pBuffer = pBuffer;
    pBufferHeader->nSize = sizeof(OMX_BUFFERHEADERTYPE);
    *ppBufferHdr = pBufferHeader;
    OMX_PRBUFFER2(pComponentPrivate->dbg, "pBufferHeader = %p\n",pBufferHeader);

    if (pComponentPrivate->bEnableCommandPending){
        SendCommand (pComponentPrivate->pHandle,
                     OMX_CommandPortEnable,
                     pComponentPrivate->bEnableCommandParam,NULL);
    }

EXIT:
#ifdef __PERF_INSTRUMENTATION__
    PERF_ReceivedBuffer(pComponentPrivate->pPERF,
                        pBuffer, nSizeBytes,
                        PERF_ModuleHLMM);
#endif
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

    if(!(strcmp(cParameterName,"OMX.TI.index.config.nbamrheaderinfo")))
    {
        *pIndexType = OMX_IndexCustomNbAmrDecHeaderInfoConfig;
    }
    else if(!(strcmp(cParameterName,"OMX.TI.index.config.nbamrstreamIDinfo")))
    {
        *pIndexType = OMX_IndexCustomNbAmrDecStreamIDConfig;
    }
    else if(!(strcmp(cParameterName,"OMX.TI.index.config.nbamr.datapath"))) 
    {
        *pIndexType = OMX_IndexCustomNbAmrDecDataPath;
    }
    else if(!(strcmp(cParameterName,"OMX.TI.index.config.nbamr.framelost")))
    {
        *pIndexType = OMX_IndexCustomNbAmrDecNextFrameLost;
    }
    else if(!(strcmp(cParameterName,"OMX.TI.AMR.Decode.Debug")))
    {
        *pIndexType = OMX_IndexCustomDebug;
    }
    else
    {
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
    AMRDEC_COMPONENT_PRIVATE *pComponentPrivate;
    
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    pComponentPrivate = (AMRDEC_COMPONENT_PRIVATE *)(((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);
    if(nIndex == 0){
      if (NULL == cRole) {
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

#ifdef NBAMRDEC_DEBUGMEM
void * mymalloc(int line, char *s, int size)
{
   void *p;
   int e=0;
   p = malloc(size);
   if(p==NULL){
       OMXDBG_PRINT(stderr, ERROR, 4, 0, "Memory not  available\n");
       /* exit(1); */
       }
   else{
         while((lines[e]!=0)&& (e<500) ){
              e++;
         }
         arr[e]=p;
         lines[e]=line;
         bytes[e]=size;
         strcpy(file[e],s);
         OMXDBG_PRINT(stderr, ERROR, 4, 0, "Allocating %d bytes on address %p, line %d file %s\n", size, p, line, s);
   }
   return p;
}

int myfree(void *dp, int line, char *s){
    int q;
    for(q=0;q<500;q++){
        if(arr[q]==dp){
           OMXDBG_PRINT(stderr, ERROR, 4, 0, "Deleting %d bytes on address %p, line %d file %s\n", bytes[q],dp, line, s);
           free(dp);
           dp = NULL;
           lines[q]=0;
           strcpy(file[q],"");
           break;
        }
     }
     if(500==q)
         OMXDBG_PRINT(stderr, ERROR, 4, 0, "\n\n%p Pointer not found. Line:%d    File%s!!\n\n",dp, line, s);
}
#endif
