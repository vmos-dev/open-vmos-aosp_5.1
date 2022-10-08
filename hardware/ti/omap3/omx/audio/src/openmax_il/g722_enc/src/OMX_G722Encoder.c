
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
 * @file OMX_G722Encoder.c
 *
 * This file implements OMX Component for G722 encoder that
 * is fully compliant with the OMX Audio specification .
 *
 * @path  $(OMAPSW_MPU)\linux\audio\src\openmax_il\g722_enc\src
 *
 * @rev  0.1
 */
/* ----------------------------------------------------------------------------- 
 *! 
 *! Revision History 
 *! ===================================
 *! Date         Author(s)            Version  Description
 *! ---------    -------------------  -------  ---------------------------------
 *! 08-Mar-2007  A.Donjon             0.1      Code update for G722 ENCODER
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

#ifdef DSP_RENDERING_ON
#endif

/*-------program files ----------------------------------------*/
#include <OMX_Component.h>
#include "LCML_DspCodec.h"
#include "TIDspOmx.h"
#include "OMX_G722Encoder.h"
#include "OMX_G722Enc_Utils.h"



/* interface with audio manager */
#define G722ENC_FIFO1 "/dev/fifo.1"
#define G722ENC_FIFO2 "/dev/fifo.2"
#define G722ENC_PERMS 0666


int FillThisBufferCount=0;

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

/*int errno;*/

/* ================================================================================= */
/**
 * @fn OMX_ComponentInit() description for OMX_ComponentInit  
 OMX_ComponentInit().  
 Called when the client calls OMX_GetHandle(). Sets up the callbacks and initializes
 values in the component.
 *
 *  @see         OMX_Core.h
 */
/* ================================================================================ */
OMX_ERRORTYPE OMX_ComponentInit (OMX_HANDLETYPE hComp)
{

    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE*) hComp;
    OMX_AUDIO_PARAM_ADPCMTYPE *pcm_ip = NULL;
    OMX_AUDIO_PARAM_ADPCMTYPE *pcm_op = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef_ip, *pPortDef_op;
    OMX_AUDIO_PARAM_PORTFORMATTYPE *pInPortFormat = NULL;
    OMX_AUDIO_PARAM_PORTFORMATTYPE *pOutPortFormat = NULL;    
    G722ENC_COMPONENT_PRIVATE *pComponentPrivate = NULL;

    int i = 0;

    if (pHandle == NULL) {
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }

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
    pHandle->FreeBuffer = FreeBuffer;
    pHandle->AllocateBuffer = AllocateBuffer;
    pHandle->UseBuffer = UseBuffer;
    pHandle->GetExtensionIndex = GetExtensionIndex;
    pHandle->ComponentRoleEnum = ComponentRoleEnum;  

    /*Allocate the memory for Component private data area*/
    pHandle->pComponentPrivate = (G722ENC_COMPONENT_PRIVATE *)
        malloc (sizeof(G722ENC_COMPONENT_PRIVATE));
    /*Fix for OMAPS00129038*/
    G722ENC_MEMPRINT("%d:::[ALLOC] %p\n",__LINE__,pHandle->pComponentPrivate);
    if(pHandle->pComponentPrivate == NULL) {
        eError = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
#ifdef UNDER_CE
    memset (pHandle->pComponentPrivate,0,sizeof(G722ENC_COMPONENT_PRIVATE));
#endif

    ((G722ENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate)->pHandle = pHandle;
    ((G722ENC_COMPONENT_PRIVATE *)
     pHandle->pComponentPrivate)->sPortParam.nPorts = 0x2;
    ((G722ENC_COMPONENT_PRIVATE *)
     pHandle->pComponentPrivate)->sPortParam.nStartPortNumber = 0x0;
    ((G722ENC_COMPONENT_PRIVATE *)
     pHandle->pComponentPrivate)->pcmParams = NULL;
    ((G722ENC_COMPONENT_PRIVATE *)
     pHandle->pComponentPrivate)->g722Params = NULL;

    pcm_ip = (OMX_AUDIO_PARAM_ADPCMTYPE *)malloc(sizeof(OMX_AUDIO_PARAM_ADPCMTYPE));
    G722ENC_MEMPRINT("%d:::[ALLOC] %p\n",__LINE__,pcm_ip);
    if(NULL == pcm_ip) {
        /* Free previously allocated memory before bailing */
        if (pHandle->pComponentPrivate) {
            G722ENC_MEMPRINT("%d:::[FREE] %p\n",__LINE__,pHandle->pComponentPrivate);
            free(pHandle->pComponentPrivate);
            pHandle->pComponentPrivate = NULL;
        }
        eError = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    pcm_op = (OMX_AUDIO_PARAM_ADPCMTYPE *)malloc(sizeof(OMX_AUDIO_PARAM_ADPCMTYPE));
    G722ENC_MEMPRINT("%d:::[ALLOC] %p\n",__LINE__,pcm_op);
    if(NULL == pcm_op) {
        /* Free previously allocated memory before bailing */
        if (pHandle->pComponentPrivate) {
            G722ENC_MEMPRINT("%d:::[FREE] %p\n",__LINE__,pHandle->pComponentPrivate);
            free(pHandle->pComponentPrivate);
            pHandle->pComponentPrivate = NULL;
        }

        if (pcm_ip) {
            G722ENC_MEMPRINT("%d:::[FREE] %p\n",__LINE__,pcm_ip);
            free(pcm_ip);
            pcm_ip = NULL;
        }
        eError = OMX_ErrorInsufficientResources;
        goto EXIT;
    }


    ((G722ENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate)->pHandle = pHandle;
    ((G722ENC_COMPONENT_PRIVATE *)
     pHandle->pComponentPrivate)->pcmParams = pcm_ip;
    ((G722ENC_COMPONENT_PRIVATE *)
     pHandle->pComponentPrivate)->g722Params = pcm_op;

    pComponentPrivate = pHandle->pComponentPrivate;

    pcm_ip->nPortIndex = G722ENC_INPUT_PORT;
    pcm_op->nSampleRate = 0;  /* 0 represents 60kpbs */
    pcm_op->nPortIndex = G722ENC_OUTPUT_PORT;    

    /* Malloc and Set pPriorityMgmt defaults */
    OMX_G722MALLOC_STRUCT(pComponentPrivate->sPriorityMgmt, OMX_PRIORITYMGMTTYPE);
    OMX_G722CONF_INIT_STRUCT(pComponentPrivate->sPriorityMgmt, OMX_PRIORITYMGMTTYPE);
    /* Initialize sPriorityMgmt data structures to default values */
    pComponentPrivate->sPriorityMgmt->nGroupPriority = 0xDEADC0DE;
    pComponentPrivate->sPriorityMgmt->nGroupID = 0xF00DBEEF;

    OMX_G722MALLOC_STRUCT(pComponentPrivate->pInPortFormat, OMX_AUDIO_PARAM_PORTFORMATTYPE);
    OMX_G722CONF_INIT_STRUCT(pComponentPrivate->pInPortFormat, OMX_AUDIO_PARAM_PORTFORMATTYPE);
    /* Set input port format defaults */
    pInPortFormat = pComponentPrivate->pInPortFormat;
    OMX_G722CONF_INIT_STRUCT(pInPortFormat, OMX_AUDIO_PARAM_PORTFORMATTYPE);
    pInPortFormat->nPortIndex         = G722ENC_INPUT_PORT;
    pInPortFormat->nIndex             = OMX_IndexParamAudioPcm;
    pInPortFormat->eEncoding          = OMX_AUDIO_CodingADPCM;

    OMX_G722MALLOC_STRUCT(pComponentPrivate->pOutPortFormat, OMX_AUDIO_PARAM_PORTFORMATTYPE);
    OMX_G722CONF_INIT_STRUCT(pComponentPrivate->pOutPortFormat, OMX_AUDIO_PARAM_PORTFORMATTYPE);
    /* Set output port format defaults */
    pOutPortFormat = pComponentPrivate->pOutPortFormat;
    OMX_G722CONF_INIT_STRUCT(pOutPortFormat, OMX_AUDIO_PARAM_PORTFORMATTYPE);
    pOutPortFormat->nPortIndex         = G722ENC_OUTPUT_PORT;
    pOutPortFormat->nIndex             = OMX_IndexParamAudioPcm;
    pOutPortFormat->eEncoding          = OMX_AUDIO_CodingADPCM;

    /*pComponentPrivate->pInputBufferList = malloc(sizeof(G722ENC_BUFFERLIST));
      G722ENC_MEMPRINT("%d:::[ALLOC] %p\n",__LINE__,pComponentPrivate->pInputBufferList);*/
    OMX_G722MALLOC_STRUCT (pComponentPrivate->pInputBufferList, G722ENC_BUFFERLIST);
    if (pComponentPrivate->pInputBufferList == NULL) {
        /* Free previously allocated memory before bailing */
        if (pHandle->pComponentPrivate) {
            G722ENC_MEMPRINT("%d:::[FREE] %p\n",__LINE__,pHandle->pComponentPrivate);
            free(pHandle->pComponentPrivate);
            pHandle->pComponentPrivate = NULL;
        }

        if (pcm_ip) {
            G722ENC_MEMPRINT("%d:::[FREE] %p\n",__LINE__,pcm_ip);
            free(pcm_ip);
            pcm_ip = NULL;
        }

        if (pcm_op) {
            G722ENC_MEMPRINT("%d:::[FREE] %p\n",__LINE__,pcm_op);
            free(pcm_op);
            pcm_op = NULL;
        }
        eError = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    
    pComponentPrivate->sDeviceString = malloc(100*sizeof(OMX_STRING));
    if (pComponentPrivate->sDeviceString == NULL) {
	/* Free previously allocated memory before bailing */
	if (pcm_ip) {
	    G722ENC_MEMPRINT("%d:::[FREE] %p\n", __LINE__, pcm_ip);
	    free(pcm_ip);
	    pcm_ip = NULL;
	}

	if (pcm_op) {
	    G722ENC_MEMPRINT("%d:::[FREE] %p\n", __LINE__, pcm_op);
	    free(pcm_op);
	    pcm_op = NULL;
	}

	if (pComponentPrivate->pInputBufferList) {
	    G722ENC_MEMPRINT("%d:::[FREE] %p\n", __LINE__, pComponentPrivate->pInputBufferList);
	    free(pComponentPrivate->pInputBufferList);
	    pComponentPrivate->pInputBufferList = NULL;
	}

	if (pHandle->pComponentPrivate) {
	    G722ENC_MEMPRINT("%d:::[FREE] %p\n", __LINE__, pHandle->pComponentPrivate);
	    free(pHandle->pComponentPrivate);
	    pHandle->pComponentPrivate = NULL;
	}
	eError = OMX_ErrorInsufficientResources;
	goto EXIT;
    }
    memset (pComponentPrivate->sDeviceString, 0, 100*sizeof(OMX_STRING));
    strcpy((char*)pComponentPrivate->sDeviceString,"/eteedn:i0:o0/codec\0");
    pComponentPrivate->pInputBufferList->numBuffers = 0; /* initialize number of buffers */
    /*pComponentPrivate->pOutputBufferList = malloc(sizeof(G722ENC_BUFFERLIST));
      G722ENC_MEMPRINT("%d:::[ALLOC] %p\n",__LINE__,pComponentPrivate->pOutputBufferList);*/
    OMX_G722MALLOC_STRUCT (pComponentPrivate->pOutputBufferList, G722ENC_BUFFERLIST);

    if (pComponentPrivate->pOutputBufferList == NULL) {
        /* Free previously allocated memory before bailing */
        if (pHandle->pComponentPrivate) {
            G722ENC_MEMPRINT("%d:::[FREE] %p\n",__LINE__,pHandle->pComponentPrivate);
            free(pHandle->pComponentPrivate);
            pHandle->pComponentPrivate = NULL;
        }

        if (pcm_ip) {
            G722ENC_MEMPRINT("%d:::[FREE] %p\n",__LINE__,pcm_ip);
            free(pcm_ip);
            pcm_ip = NULL;
        }

        if (pcm_op) {
            G722ENC_MEMPRINT("%d:::[FREE] %p\n",__LINE__,pcm_op);
            free(pcm_op);
            pcm_op = NULL;
        }

        if (pComponentPrivate->pInputBufferList) {
            G722ENC_MEMPRINT("%d:::[FREE] %p\n",__LINE__,pComponentPrivate->pInputBufferList);
            free(pComponentPrivate->pInputBufferList);
            pComponentPrivate->pInputBufferList = NULL;
        }
        eError = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    pComponentPrivate->pOutputBufferList->numBuffers = 0; /* initialize number of buffers */
    for (i=0; i < G722ENC_MAX_NUM_OF_BUFS; i++) {
        pComponentPrivate->pOutputBufferList->pBufHdr[i] = NULL;
        pComponentPrivate->pInputBufferList->pBufHdr[i] = NULL;
        pComponentPrivate->arrTickCount[i] = 0; 
        pComponentPrivate->arrTimestamp[i] = 0; 
    } 
    pComponentPrivate->IpBufindex = 0; 
    pComponentPrivate->OpBufindex = 0;

    pComponentPrivate->dasfmode = 0;
    pComponentPrivate->rtmx = 0;    
    pComponentPrivate->bPortDefsAllocated = 0;
    pComponentPrivate->bCompThreadStarted = 0;
    pComponentPrivate->bLcmlHandleOpened = 0;
    pComponentPrivate->strmAttr = NULL;
    pComponentPrivate->bIdleCommandPending = 0;
    for (i=0; i < G722ENC_MAX_NUM_OF_BUFS; i++) {
        pComponentPrivate->pInputBufHdrPending[i] = NULL;
        pComponentPrivate->pOutputBufHdrPending[i] = NULL;
    }
    pComponentPrivate->nInvalidFrameCount = 0;
    pComponentPrivate->nNumInputBufPending = 0;
    pComponentPrivate->nNumOutputBufPending = 0;
    pComponentPrivate->bDisableCommandPending = 0;
    pComponentPrivate->bNoIdleOnStop= OMX_FALSE;
    pComponentPrivate->bIdleCommandPending = OMX_FALSE;
    pComponentPrivate->nOutStandingFillDones = 0;
    pComponentPrivate->bDisableCommandParam = 0;
    pComponentPrivate->pMarkBuf = NULL;
    pComponentPrivate->bStreamCtrlCalled = 0;
    pComponentPrivate->pParams = NULL;
    pComponentPrivate->bDspStoppedWhileExecuting = OMX_FALSE;
    pComponentPrivate->bPreempted = OMX_FALSE; 


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

    /*pPortDef_ip = (OMX_PARAM_PORTDEFINITIONTYPE *) malloc(sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
      G722ENC_MEMPRINT("%d:::[ALLOC] %p\n",__LINE__,pPortDef_ip);*/
    OMX_G722MALLOC_STRUCT (pPortDef_ip, OMX_PARAM_PORTDEFINITIONTYPE);
    if (pPortDef_ip == NULL) {
        G722ENC_DPRINT("%d :: malloc failed\n",__LINE__);
        /* Free previously allocated memory before bailing */

        if (pHandle->pComponentPrivate) {
            G722ENC_MEMPRINT("%d:::[FREE] %p\n",__LINE__,pHandle->pComponentPrivate);
            free(pHandle->pComponentPrivate);
            pHandle->pComponentPrivate = NULL;
        }

        if (pcm_ip) {
            G722ENC_MEMPRINT("%d:::[FREE] %p\n",__LINE__,pcm_ip);
            free(pcm_ip);
            pcm_ip = NULL;
        }

        if (pcm_op) {
            G722ENC_MEMPRINT("%d:::[FREE] %p\n",__LINE__,pcm_op);
            free(pcm_op);
            pcm_op = NULL;
        }

        if (pComponentPrivate->pInputBufferList) {
            G722ENC_MEMPRINT("%d:::[FREE] %p\n",__LINE__,pComponentPrivate->pInputBufferList);
            free(pComponentPrivate->pInputBufferList);
            pComponentPrivate->pInputBufferList = NULL;
        }

        if (pComponentPrivate->pOutputBufferList) {
            G722ENC_MEMPRINT("%d:::[FREE] %p\n",__LINE__,pComponentPrivate->pOutputBufferList);
            free(pComponentPrivate->pOutputBufferList);
            pComponentPrivate->pOutputBufferList = NULL;
        }
        eError = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    pPortDef_op = (OMX_PARAM_PORTDEFINITIONTYPE *)
        malloc(sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
    G722ENC_MEMPRINT("%d:::[ALLOC] %p\n",__LINE__,pPortDef_op);
    if (pPortDef_op == NULL) {
        G722ENC_DPRINT("%d :: malloc failed\n",__LINE__);
        /* Free previously allocated memory before bailing */

        if (pcm_ip) {
            G722ENC_MEMPRINT("%d:::[FREE] %p\n",__LINE__,pcm_ip);
            free(pcm_ip);
            pcm_ip = NULL;
        }

        if (pcm_op) {
            G722ENC_MEMPRINT("%d:::[FREE] %p\n",__LINE__,pcm_op);
            free(pcm_op);
            pcm_op = NULL;
        }

        if (pComponentPrivate->pInputBufferList) {
            G722ENC_MEMPRINT("%d:::[FREE] %p\n",__LINE__,pComponentPrivate->pInputBufferList);
            free(pComponentPrivate->pInputBufferList);
            pComponentPrivate->pInputBufferList = NULL;
        }

        if (pComponentPrivate->pOutputBufferList) {
            G722ENC_MEMPRINT("%d:::[FREE] %p\n",__LINE__,pComponentPrivate->pOutputBufferList);
            free(pComponentPrivate->pOutputBufferList);
            pComponentPrivate->pOutputBufferList = NULL;
        }

	if (pHandle->pComponentPrivate) {
	    G722ENC_MEMPRINT("%d:::[FREE] %p\n", __LINE__, pHandle->pComponentPrivate);
	    free(pHandle->pComponentPrivate);
	    pHandle->pComponentPrivate = NULL;
	}

        if (pPortDef_ip) {
            G722ENC_MEMPRINT("%d:::[FREE] %p\n",__LINE__,pPortDef_ip);
            free(pPortDef_ip);
            pPortDef_ip = NULL;
        }
        eError = OMX_ErrorInsufficientResources;
        goto EXIT;
    }


    ((G722ENC_COMPONENT_PRIVATE*) pHandle->pComponentPrivate)->pPortDef[G722ENC_INPUT_PORT]
        = pPortDef_ip;

    ((G722ENC_COMPONENT_PRIVATE*) pHandle->pComponentPrivate)->pPortDef[G722ENC_OUTPUT_PORT]
        = pPortDef_op;

    pPortDef_ip->nPortIndex = 0x0;
    pPortDef_ip->nBufferCountActual = G722ENC_NUM_INPUT_BUFFERS;
    pPortDef_ip->nBufferCountMin = G722ENC_NUM_INPUT_BUFFERS;
    pPortDef_ip->eDir = OMX_DirOutput;
    pPortDef_ip->bEnabled = OMX_TRUE;
    pPortDef_ip->bPopulated = OMX_FALSE;    
    pPortDef_ip->nBufferSize = G722ENC_INPUT_BUFFER_SIZE;
    pPortDef_ip->eDomain = OMX_PortDomainAudio;
    pPortDef_ip->format.audio.eEncoding = OMX_AUDIO_CodingADPCM;

    pPortDef_op->nPortIndex = 0x1;
    pPortDef_op->nBufferCountActual = G722ENC_NUM_OUTPUT_BUFFERS;
    pPortDef_op->nBufferCountMin = G722ENC_NUM_OUTPUT_BUFFERS;
    pPortDef_op->eDir = OMX_DirOutput;
    pPortDef_op->bEnabled = OMX_TRUE;
    pPortDef_op->bPopulated = OMX_FALSE;
    pPortDef_op->eDomain = OMX_PortDomainAudio;
    pPortDef_op->format.audio.eEncoding = OMX_AUDIO_CodingADPCM;
    pPortDef_op->nBufferSize = G722ENC_OUTPUT_BUFFER_SIZE_BYTES;

    pComponentPrivate->bPortDefsAllocated = 1;

    eError = G722Enc_StartCompThread(pHandle);
    if (eError != OMX_ErrorNone) {
        G722ENC_DPRINT ("%d ::Error returned from the Component\n",__LINE__);
        goto EXIT;
    }
    pComponentPrivate->bCompThreadStarted = 1;

#ifdef DSP_RENDERING_ON
    if((pComponentPrivate->fdwrite=open(G722ENC_FIFO1,O_WRONLY))<0) {
        G722ENC_DPRINT("[G722 Component] - failure to open WRITE pipe\n");
    }

    if((pComponentPrivate->fdread=open(G722ENC_FIFO2,O_RDONLY))<0) {
        G722ENC_DPRINT("[G722 Component] - failure to open READ pipe\n");
    }
    /* compose the data*/
    cmd_data.hComponent = pHandle;
    cmd_data.AM_Cmd = AM_CommandIsInputStreamAvailable;
    cmd_data.param1 = 0;

    if((write(pComponentPrivate->fdwrite, &cmd_data, sizeof(cmd_data)))<0) {
        G722ENC_DPRINT("[G722 Component] - send command to audio manager\n");
    }
    else {
        G722ENC_DPRINT("%d ::%s [G722 Component] - send Command=%d to audio manager\n", __LINE__, __FUNCTION__, cmd_data.AM_Cmd);
    }

    if((read(pComponentPrivate->fdread, &cmd_data, sizeof(cmd_data)))<0) {
        G722ENC_DPRINT("[G722 Component] - failure to get data from the audio manager\n");
    }
    else {
        G722ENC_DPRINT("[G722 Component] - got data back from audio manager, command=%d AND streamID=%d\n", cmd_data.AM_Cmd, (int) cmd_data.streamID);
    }

    if(cmd_data.streamID == 0) {
        /* no output stream available*/
        eError = OMX_ErrorInsufficientResources;
    }
    else {
        /* get stream ID from audio manager */
        pComponentPrivate->streamID = cmd_data.streamID;
        eError = OMX_ErrorNone;
    }
#endif

 EXIT:
    G722ENC_DPRINT("Leaving OMX_ComponentInit\n");
    return eError;
}


/* ================================================================================= */
/**
 * @fn SendCommand() description for SendCommand  
 SendCommand().  
 Send a command to the component.
 *
 *  @see         OMX_Core.h
 */
/* ================================================================================ */
static OMX_ERRORTYPE SendCommand (OMX_HANDLETYPE phandle,
                                  OMX_COMMANDTYPE Cmd,
                                  OMX_U32 nParam,OMX_PTR pCmdData)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    int nRet = 0;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)phandle;
    G722ENC_COMPONENT_PRIVATE *pCompPrivate =
        (G722ENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;

    G722ENC_DPRINT("Entered SendCommand\n");
    G722ENC_DPRINT("Cmd = %d\n",Cmd);
    G722ENC_DPRINT("nParam = %d\n",nParam);
    G722ENC_DPRINT("%d :: G722ENC: About to Check for Invalid State \n",__LINE__);
    if(pCompPrivate->curState == OMX_StateInvalid){
        eError = OMX_ErrorInvalidState;
        goto EXIT;
    }
    switch(Cmd) {
    case OMX_CommandStateSet:
        G722ENC_DPRINT("OMX_CommandStateSet SendCommand\n");
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
                G722ENC_DPRINT("OMX_CommandStateSet SendCommand\n");
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
        G722ENC_DPRINT("%d :: G722ENC: Entered switch - Command Flush\n",__LINE__);
        if(nParam > 1 && nParam != -1) {
            eError = OMX_ErrorBadPortIndex;
            goto EXIT;
        }
        break;

    case OMX_CommandPortDisable:
        G722ENC_DPRINT("%d :: G722ENC: Entered switch - Command Port Disable\n",__LINE__);
        break;

    case OMX_CommandPortEnable:
        G722ENC_DPRINT("%d :: G722ENC: Entered switch - Command Port Enable\n",__LINE__);
        break;

    case OMX_CommandMarkBuffer:
        G722ENC_DPRINT("%d :: G722ENC: Entered switch - Command Mark Buffer\n",__LINE__);
        if(nParam > 0) {
            eError = OMX_ErrorBadPortIndex;
            goto EXIT;
        }
        break;

    default:
        eError = OMX_ErrorBadParameter;
        goto EXIT;
        break;
    }

    nRet = write (pCompPrivate->cmdPipe[1], &Cmd,sizeof(Cmd));
    if (nRet == -1) {
        eError = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    if (Cmd == OMX_CommandMarkBuffer) {
        nRet = write (pCompPrivate->cmdDataPipe[1], &pCmdData,sizeof(OMX_PTR));
    }
    else {
        nRet = write (pCompPrivate->cmdDataPipe[1], &nParam,
                      sizeof(nParam));
    }

    G722ENC_DPRINT("%d :: G722ENC: SendCommand - nRet = %d\n",__LINE__,nRet);
    G722ENC_DPRINT("%d :: G722ENC: SendCommand - errno = %d\n",__LINE__,errno);
    if (nRet == -1) {
        eError = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

#ifdef DSP_RENDERING_ON

    /* add for Tee device control*/
    if(Cmd == OMX_CommandStateSet && nParam == OMX_StateExecuting) {
        if(pCompPrivate->teemode == 1) {
            /* enable acoustic supporting*/
            G722ENC_DPRINT("[G722 encoder] --- supporting TeeDN\n");
            G722ENC_DPRINT("[G722 encoder] --- Send Tee commnad\n");

            /* enable Tee device command*/
            cmd_data.hComponent = pHandle;
            /*
              AM_CommandTDNPlayMode           --- play mode only
              AM_CommandTDNLoopBackMode       --- loopback mode pnly
              AM_CommandTDNPlayLoopBackMode   --- loopback + playback mode
            */
            cmd_data.AM_Cmd = AM_CommandTDNPlayMode;
            cmd_data.param1 = 0;
            cmd_data.param2 = 0;
            cmd_data.streamID = 0;

            if((write(pCompPrivate->fdwrite, &cmd_data, sizeof(cmd_data)))<0) {
                G722ENC_DPRINT("[G722 encoder] - fail to send Tee command to audio manager\n");
                eError = OMX_ErrorHardware;
                goto EXIT;
            }
        }
        else {
            G722ENC_DPRINT("[G722 encoder] --- Normal DASF Mode\n");
        }
    }
#endif
 EXIT:
    G722ENC_DPRINT("%d :: G722ENC: Exiting SendCommand()\n",__LINE__);
    return eError;
}

static OMX_ERRORTYPE GetParameter (OMX_HANDLETYPE hComp,
                                   OMX_INDEXTYPE nParamIndex,
                                   OMX_PTR ComponentParameterStructure)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    G722ENC_COMPONENT_PRIVATE  *pComponentPrivate = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE *pParameterStructure  = NULL;

    G722ENC_DPRINT ("%d :: Entering the GetParameter():: %x\n",__LINE__,nParamIndex);

    pComponentPrivate = (G722ENC_COMPONENT_PRIVATE *)(((OMX_COMPONENTTYPE*)hComp)->pComponentPrivate);
    if (pComponentPrivate == NULL) {
	eError = OMX_ErrorBadParameter;
	goto EXIT;
    }

    pParameterStructure = (OMX_PARAM_PORTDEFINITIONTYPE *)ComponentParameterStructure;
    if (pParameterStructure == NULL) {
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }

    if(pComponentPrivate->curState == OMX_StateInvalid) {
        pComponentPrivate->cbInfo.EventHandler(
                                               hComp,
                                               ((OMX_COMPONENTTYPE *)hComp)->pApplicationPrivate,
                                               OMX_EventError,
                                               OMX_ErrorIncorrectStateOperation,
                                               0,
                                               NULL);
        goto EXIT;
    }

    switch(nParamIndex){
    case OMX_IndexParamAudioInit:
        G722ENC_DPRINT ("OMX_IndexParamAudioInit\n");
	memcpy(ComponentParameterStructure, &pComponentPrivate->sPortParam, sizeof(OMX_PORT_PARAM_TYPE));
        break;

    case OMX_IndexParamPortDefinition:
        if(((OMX_PARAM_PORTDEFINITIONTYPE *)(ComponentParameterStructure))->nPortIndex ==
           pComponentPrivate->pPortDef[G722ENC_INPUT_PORT]->nPortIndex) {
            memcpy(ComponentParameterStructure,
                   pComponentPrivate->pPortDef[G722ENC_INPUT_PORT],
                   sizeof(OMX_PARAM_PORTDEFINITIONTYPE)
                   );
        } 
        else if(((OMX_PARAM_PORTDEFINITIONTYPE *)(ComponentParameterStructure))->nPortIndex ==
                pComponentPrivate->pPortDef[G722ENC_OUTPUT_PORT]->nPortIndex) {
            memcpy(ComponentParameterStructure,
                   pComponentPrivate->pPortDef[G722ENC_OUTPUT_PORT],
                   sizeof(OMX_PARAM_PORTDEFINITIONTYPE)
                   );
        } 
        else {
            eError = OMX_ErrorBadPortIndex;
        }
        break;

    case OMX_IndexParamAudioPortFormat:
        if(((OMX_AUDIO_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nPortIndex ==
           pComponentPrivate->pInPortFormat->nPortIndex) {

            if(((OMX_AUDIO_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nIndex >
               pComponentPrivate->pInPortFormat->nIndex) {
                eError = OMX_ErrorNoMore;
            } 
            else {
                memcpy(ComponentParameterStructure, pComponentPrivate->pInPortFormat,
                       sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
            }
        }
        else if(((OMX_AUDIO_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nPortIndex ==
                pComponentPrivate->pPortDef[G722ENC_OUTPUT_PORT]->nPortIndex){
            if(((OMX_AUDIO_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nIndex >
               pComponentPrivate->pOutPortFormat->nIndex) {
                eError = OMX_ErrorNoMore;
            } 
            else {
                memcpy(ComponentParameterStructure, pComponentPrivate->pOutPortFormat,
                       sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
            }
        } 
        else {
            eError = OMX_ErrorBadPortIndex;
        }
        break;

    case OMX_IndexParamAudioAdpcm:
        if(((OMX_AUDIO_PARAM_ADPCMTYPE *)(ComponentParameterStructure))->nPortIndex ==
           G722ENC_INPUT_PORT) {
	    if (pComponentPrivate->pcmParams == NULL) {
                eError = OMX_ErrorBadParameter;
		break;
	    }
            memcpy(ComponentParameterStructure,
                   pComponentPrivate->pcmParams,
                   sizeof(OMX_AUDIO_PARAM_ADPCMTYPE)
                   );
        } 
        else if(((OMX_AUDIO_PARAM_ADPCMTYPE *)(ComponentParameterStructure))->nPortIndex ==
                G722ENC_OUTPUT_PORT) {
	    if (pComponentPrivate->g722Params == NULL) {
                eError = OMX_ErrorBadParameter;
		break;
	    }
            memcpy(ComponentParameterStructure,
                   pComponentPrivate->g722Params,
                   sizeof(OMX_AUDIO_PARAM_ADPCMTYPE)
                   );

        } 
        else {
            eError = OMX_ErrorBadPortIndex;
        }
        break;
        
    case OMX_IndexParamPriorityMgmt:
	if (pComponentPrivate->sPriorityMgmt == NULL) {
             eError = OMX_ErrorBadParameter;
	     break;
	}
        memcpy(ComponentParameterStructure,
               pComponentPrivate->sPriorityMgmt,
               sizeof(OMX_PRIORITYMGMTTYPE));
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
    G722ENC_DPRINT("%d :: Exiting GetParameter():: %x\n",__LINE__,nParamIndex);
    return eError;
}

/* ================================================================================= */
/**
 * @fn SetParameter() description for SetParameter  
 SendCommand().  
 Send an initialization parameter structure to a component. 
 *
 *  @see         OMX_Core.h
 */
/* ================================================================================ */

static OMX_ERRORTYPE SetParameter (
                                   OMX_IN  OMX_HANDLETYPE hComponent,
                                   OMX_IN  OMX_INDEXTYPE nIndex,
                                   OMX_IN  OMX_PTR ComponentParameterStructure)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_AUDIO_PARAM_PORTFORMATTYPE* pComponentParam = NULL;
    /*    OMX_PARAM_PORTDEFINITIONTYPE *pComponentParamPort = NULL;*/
    G722ENC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    pComponentPrivate = (G722ENC_COMPONENT_PRIVATE *)(((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);
    
    if (pComponentPrivate == NULL) {
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if (ComponentParameterStructure == NULL) {
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }
    switch(nIndex) {
    case OMX_IndexParamAudioInit:
        G722ENC_DPRINT ("%d :: Inside the SetParameter - OMX_IndexParamAudioInit\n", __LINE__);
        memcpy(&pComponentPrivate->sPortParam, ComponentParameterStructure, sizeof(OMX_PORT_PARAM_TYPE));
        break;

    case OMX_IndexParamPortDefinition:
        G722ENC_DPRINT ("%d :: Inside the SetParameter - OMX_IndexParamPortDefinition\n", __LINE__);
        if(((OMX_PARAM_PORTDEFINITIONTYPE *)(ComponentParameterStructure))->nPortIndex ==
           pComponentPrivate->pPortDef[G722ENC_INPUT_PORT]->nPortIndex) {

            memcpy(pComponentPrivate->pPortDef[G722ENC_INPUT_PORT],
                   ComponentParameterStructure, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));

        } 

        else if(((OMX_PARAM_PORTDEFINITIONTYPE *)(ComponentParameterStructure))->nPortIndex ==
                pComponentPrivate->pPortDef[G722ENC_OUTPUT_PORT]->nPortIndex) {

            memcpy(pComponentPrivate->pPortDef[G722ENC_OUTPUT_PORT],
                   ComponentParameterStructure, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));

        } 
        else {
            eError = OMX_ErrorBadPortIndex;
            G722ENC_DPRINT ("%d :: Error in SetParameter - OMX_IndexParamPortDefinition = %x\n", __LINE__, eError);
        }
        break;

    case OMX_IndexParamAudioPortFormat:
        G722ENC_DPRINT ("%d :: Inside the SetParameter - OMX_IndexParamAudioPortFormat\n", __LINE__);
        pComponentParam = (OMX_AUDIO_PARAM_PORTFORMATTYPE*)ComponentParameterStructure;
        if(pComponentParam->nPortIndex == pComponentPrivate->pInPortFormat->nPortIndex) {
            memcpy(pComponentPrivate->pInPortFormat, pComponentParam,
                   sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
        }
        else if(pComponentParam->nPortIndex == pComponentPrivate->pOutPortFormat->nPortIndex){
            memcpy(pComponentPrivate->pOutPortFormat,
                   pComponentParam, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
        } 
        else {
            eError = OMX_ErrorBadPortIndex;
            G722ENC_DPRINT ("%d :: Error in SetParameter - OMX_IndexParamAudioPortFormat = %x\n", __LINE__, eError);
        }
        break;

    case OMX_IndexParamAudioAdpcm:
        G722ENC_DPRINT ("%d :: Inside the SetParameter - OMX_IndexParamAudioPcm\n", __LINE__);
        if(((OMX_AUDIO_PARAM_ADPCMTYPE *)(ComponentParameterStructure))->nPortIndex ==
           pComponentPrivate->g722Params->nPortIndex) {
            memcpy(pComponentPrivate->g722Params, ComponentParameterStructure,
                   sizeof(OMX_AUDIO_PARAM_ADPCMTYPE));                                      
        } 
        else if(((OMX_AUDIO_PARAM_ADPCMTYPE *)(ComponentParameterStructure))->nPortIndex ==
                pComponentPrivate->pcmParams->nPortIndex)
        {
            memcpy(pComponentPrivate->pcmParams, ComponentParameterStructure,
                   sizeof(OMX_AUDIO_PARAM_ADPCMTYPE));
            if(pComponentPrivate->dasfmode==1){

#ifdef DSP_RENDERING_ON
                cmd_data.hComponent = hComponent;
                cmd_data.AM_Cmd = AM_CommandWarnSampleFreqChange;  
                cmd_data.param1 = pComponentPrivate->pcmParams->nSampleRate;
                if((write(pComponentPrivate->fdwrite, &cmd_data, sizeof(cmd_data)))<0) {
                    G722ENC_DPRINT("[G722 Enc Component] - send command to audio manager\n");
                }
                else {
                    G722ENC_DPRINT("%d:: %s [G722 Enc Component] - send Command=%d to audio manager\n", __LINE__, __FUNCTION__,  cmd_data.AM_Cmd);
                }
#endif

            }
        }
        else{
            eError = OMX_ErrorBadPortIndex;
            G722ENC_DPRINT ("%d :: Error in SetParameter - OMX_IndexParamAudioPcm = %x\n", __LINE__, eError);
        }

        break;
    case OMX_IndexParamPriorityMgmt:
        if (pComponentPrivate->curState == OMX_StateLoaded){
	    if (pComponentPrivate->sPriorityMgmt == NULL) {
                eError = OMX_ErrorBadParameter;
	        break;
	    }
            memcpy(pComponentPrivate->sPriorityMgmt,
                   (OMX_PRIORITYMGMTTYPE*)ComponentParameterStructure,
                   sizeof(OMX_PRIORITYMGMTTYPE));
        }
        else{
            eError = OMX_ErrorIncorrectStateOperation;
        }
        break;

    default:
        G722ENC_DPRINT("%d :: SetParameter: Default Case: \n",__LINE__);
        eError = OMX_ErrorBadParameter;
        break;
    }
 EXIT:
    G722ENC_DPRINT ("%d :: Exiting from SetParameter() eError = %x\n",
                    __LINE__, eError);

    return eError;
}

/* ================================================================================= */
/**
 * @fn SetParameter() description for SetParameter  
 SendCommand().  
 This method will update application callbacks to the component. So that component can 
 make use of those call back while sending buffers to the application. And also it 
 will copy the application private data to component memory
 *
 *  @see         OMX_Core.h
 */
/* ================================================================================ */
static OMX_ERRORTYPE SetCallbacks (OMX_HANDLETYPE pComponent,
                                   OMX_CALLBACKTYPE* pCallBacks,
                                   OMX_PTR pAppData)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE*)pComponent;

    G722ENC_COMPONENT_PRIVATE *pComponentPrivate =
        (G722ENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;


    if (pCallBacks == NULL) {
        eError = OMX_ErrorBadParameter;
        G722ENC_DPRINT ("%d :: Received the empty callbacks from the \
                application\n",__LINE__);
        goto EXIT;
    }

    /*Copy the callbacks of the application to the component private*/
    memcpy (&(pComponentPrivate->cbInfo), pCallBacks, sizeof(OMX_CALLBACKTYPE));


    if (!pComponentPrivate->cbInfo.EventHandler ||
        !pComponentPrivate->cbInfo.EmptyBufferDone ||
        !pComponentPrivate->cbInfo.FillBufferDone)  {
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }

    /*copy the application private data to component memory*/
    pHandle->pApplicationPrivate = pAppData;

    pComponentPrivate->curState = OMX_StateLoaded;


 EXIT:
    G722ENC_DPRINT ("%d :: Exiting SetCallbacks()\n", __LINE__);
    return eError;
}


/* ================================================================================= */
/**
 * @fn GetComponentVersion() description for GetComponentVersion  
 GetComponentVersion().  
 This method will will return information about the component.
 *
 *  @see         OMX_Core.h
 */
/* ================================================================================ */

static OMX_ERRORTYPE GetComponentVersion (OMX_HANDLETYPE hComp,
                                          OMX_STRING pComponentName,
                                          OMX_VERSIONTYPE* pComponentVersion,
                                          OMX_VERSIONTYPE* pSpecVersion,
                                          OMX_UUIDTYPE* pComponentUUID)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    G722ENC_DPRINT ("%d :: Entering GetComponentVersion\n", __LINE__);
    eError = OMX_ErrorNotImplemented;
    G722ENC_DPRINT ("%d :: Inside  GetComponentVersion\n", __LINE__);
    G722ENC_DPRINT ("%d :: Exiting GetComponentVersion\n", __LINE__);
    return eError;
}


/* ================================================================================= */
/**
 * @fn GetConfig() description for GetConfig  
 GetConfig().  
 This method  will get one of the configuration structures from a component. .
 *
 *  @see         OMX_Core.h
 */
/* ================================================================================ */
static OMX_ERRORTYPE GetConfig (OMX_HANDLETYPE hComp,
                                OMX_INDEXTYPE nConfigIndex,
                                OMX_PTR ComponentConfigStructure)
{
    
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    G722ENC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    TI_OMX_STREAM_INFO *streamInfo = NULL;
    pComponentPrivate = (G722ENC_COMPONENT_PRIVATE *)
        (((OMX_COMPONENTTYPE*)hComp)->pComponentPrivate);
    /*streamInfo = malloc(sizeof(TI_OMX_STREAM_INFO));*/
    OMX_G722MALLOC_STRUCT (streamInfo, TI_OMX_STREAM_INFO);
    if(streamInfo == NULL)
    {
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }
    
    if(nConfigIndex == OMX_IndexCustomG722EncStreamIDConfig)
    {
        /* copy component info */
        streamInfo->streamId = pComponentPrivate->streamID;
        memcpy(ComponentConfigStructure,streamInfo,sizeof(TI_OMX_STREAM_INFO));
    }


 EXIT:
    if(streamInfo)
    {
        G722ENC_MEMPRINT("%d:::[FREE] %p\n",__LINE__,streamInfo);
        free(streamInfo);
        streamInfo = NULL;
    }
    G722ENC_DPRINT ("Exiting GetConfig\n");
    return eError;
}
/* ================================================================================= */
/**
 * @fn SetConfig() description for SetConfig  
 SetConfig().  
 This method  will set one of the configuration structures from a component. .
 *
 *  @see         OMX_Core.h
 */
/* ================================================================================ */

static OMX_ERRORTYPE SetConfig (OMX_HANDLETYPE hComp,
                                OMX_INDEXTYPE nConfigIndex,
                                OMX_PTR ComponentConfigStructure)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    G722ENC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    OMX_COMPONENTTYPE* pHandle = (OMX_COMPONENTTYPE*)hComp;
    TI_OMX_DSP_DEFINITION* pDspDefinition = NULL;
    OMX_AUDIO_CONFIG_VOLUMETYPE *pGainStructure = NULL;
    OMX_S16 *customFlag = NULL;
    TI_OMX_DATAPATH dataPath;
    /*    OMX_S16* deviceString;*/
    G722ENC_DPRINT("%d :: Entering SetConfig\n", __LINE__);

    if (pHandle == NULL) {
        G722ENC_DPRINT ("%d :: Invalid HANDLE OMX_ErrorBadParameter \n",__LINE__);
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pComponentPrivate = (G722ENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    G722ENC_DPRINT("Entering setConfig Switch statements\n");
    switch (nConfigIndex) 
    {
    case OMX_IndexCustomG722EncModeConfig: 
        pDspDefinition = (TI_OMX_DSP_DEFINITION*)ComponentConfigStructure;
        memcpy(&(pComponentPrivate->tiOmxDspDefinition), pDspDefinition,
               sizeof(TI_OMX_DSP_DEFINITION));
        pComponentPrivate->dasfmode = pComponentPrivate->tiOmxDspDefinition.dasfMode;
        pComponentPrivate->streamID = pComponentPrivate->tiOmxDspDefinition.streamId;
        break;

    case OMX_IndexCustomG722EncStreamIDConfig:
        pDspDefinition = (TI_OMX_DSP_DEFINITION*)ComponentConfigStructure;
        pComponentPrivate->streamID = pDspDefinition->streamId;
        break;
            
    case OMX_IndexCustomG722EncHeaderInfoConfig:
        {
            pDspDefinition = (TI_OMX_DSP_DEFINITION *)ComponentConfigStructure;
            if (pDspDefinition == NULL) {
                eError = OMX_ErrorBadParameter;
                G722ENC_DPRINT("%d :: OMX_ErrorBadParameter from SetConfig\n",__LINE__);
                goto EXIT;
            }            
            pComponentPrivate->dasfmode = pDspDefinition->dasfMode;
            G722ENC_DPRINT("pComponentPrivate->dasfmode = %d\n",pComponentPrivate->dasfmode);
            pComponentPrivate->teemode = pDspDefinition->teeMode;
            G722ENC_DPRINT("pComponentPrivate->teemode = %d\n",pComponentPrivate->teemode);

            if (2 == pComponentPrivate->dasfmode)
            {
                pComponentPrivate->dasfmode--;
                pComponentPrivate->rtmx = 1; 
            }
            pComponentPrivate->streamID = pDspDefinition->streamId;
            break;
        }           
           
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
            G722ENC_DPRINT("[G722 encoder] - fail to send command to audio manager\n");
        }
            
        cmd_data.hComponent = hComp;
        cmd_data.AM_Cmd = AM_CommandRecordVolume;
             
        /* 100 is max value for T2 driver 
           T2 deiver value is not linear*/
        if(pGainStructure->sVolume.nValue > 4096)
            cmd_data.param1 = 100;          
        else
            cmd_data.param1 = 90;   

        cmd_data.param2 = 0;
        cmd_data.streamID = 0;

        if((write(pComponentPrivate->fdwrite, &cmd_data, sizeof(cmd_data)))<0)
        {   
            G722ENC_DPRINT("[G722 encoder] - fail to send command to audio manager\n");
        }
#endif
        break;

    case  OMX_IndexCustomG722EncDataPath:
        G722ENC_DPRINT("Entering OMX_IndexCustomG722EncDataPath statements\n");
        customFlag = (OMX_S16*)ComponentConfigStructure;
        if (customFlag == NULL) {
            eError = OMX_ErrorBadParameter;
            goto EXIT;
        }
        G722ENC_DPRINT("setting data path %d\n", __LINE__);
        dataPath = *customFlag;

        switch(dataPath) {
        case DATAPATH_APPLICATION:
            OMX_MMMIXER_DATAPATH(pComponentPrivate->sDeviceString, 
                                 RENDERTYPE_ENCODER, pComponentPrivate->streamID);
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
        break;
    }
 EXIT:
    G722ENC_DPRINT("%d :: Exiting SetConfig\n", __LINE__);
    G722ENC_DPRINT("%d :: Returning = 0x%x\n",__LINE__,eError);
    return eError;
}

/* ================================================================================= */
/**
 * @fn GetState() description for GetState  
 GetState().  
 This method  will get the current state of the component
 *
 *  @see         OMX_Core.h
 * 
 *  ACAs note: NORMALIZED
 */
/* ================================================================================ */
static OMX_ERRORTYPE GetState (OMX_HANDLETYPE pComponent, OMX_STATETYPE* pState)
{
    OMX_ERRORTYPE      eError  = OMX_ErrorUndefined;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *) pComponent;

    
    if (pHandle == NULL) {
        return (OMX_ErrorBadParameter);
    }
    
    if (pState == NULL){
        G722ENC_DPRINT("%d :: %s :: OMX_ErrorBadParameter\n", __LINE__,__FUNCTION__);
        return (eError);
    }

    if (pHandle && pHandle->pComponentPrivate) {
        *pState = ((G722ENC_COMPONENT_PRIVATE*)
                   pHandle->pComponentPrivate)->curState;
    } else {
        *pState = OMX_StateLoaded;
    }

    eError = OMX_ErrorNone;

    return eError;
}
/**/
/* ================================================================================= */
/**
 * @fn EmptyThisBuffer() description for EmptyThisBuffer  
 EmptyThisBuffer().  
 This method is called by the IL client to send a filled input buffer to the 
 component
 *
 *  @see         OMX_Core.h
 * 
 *  ACAs note: NORMALIZED
 */
/* ================================================================================ */
static OMX_ERRORTYPE EmptyThisBuffer (OMX_HANDLETYPE pComponent,
                                      OMX_BUFFERHEADERTYPE* pBuffer)
{
    OMX_ERRORTYPE                 eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE            *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    G722ENC_COMPONENT_PRIVATE    *pComponentPrivate = (G722ENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef = NULL;
    ssize_t ret = 0;

    pPortDef = ((G722ENC_COMPONENT_PRIVATE *)
                pComponentPrivate)->pPortDef[G722ENC_INPUT_PORT];


#ifdef _ERROR_PROPAGATION__
    if (pComponentPrivate->curState == OMX_StateInvalid){
        G722ENC_DPRINT("%d :: %s :: ErrorInvalidState \n", __LINE__,__FUNCTION__);
        return (OMX_ErrorStateInvalid);
    }
#endif

    if(!pPortDef->bEnabled){
        G722ENC_DPRINT("%d :: %s ::  OMX_ErrorIncorrectStateOperation\n", __LINE__,__FUNCTION__);
        return (OMX_ErrorIncorrectStateOperation);
    }

    if (pBuffer == NULL){
        G722ENC_DPRINT("%d :: %s ::  OMX_ErrorBadParameter\n", __LINE__,__FUNCTION__);
        return (OMX_ErrorBadParameter);
    }


    if (pBuffer->nSize != sizeof(OMX_BUFFERHEADERTYPE)) {
        G722ENC_DPRINT("%d :: %s ::  OMX_ErrorBadParameter\n", __LINE__,__FUNCTION__);
        return (OMX_ErrorBadParameter);
    }

    if (pBuffer->nVersion.nVersion != pComponentPrivate->nVersion){
        G722ENC_DPRINT("%d :: %s ::  OMX_ErrorVersionMismatch\n", __LINE__,__FUNCTION__);
        return (OMX_ErrorVersionMismatch);
    }

    if (pBuffer->nInputPortIndex != G722ENC_INPUT_PORT){
        G722ENC_DPRINT("%d :: %s ::  OMX_ErrorBadPortIndex\n", __LINE__,__FUNCTION__);
        return (OMX_ErrorBadPortIndex);
    }
    G722ENC_DPRINT("%d :: %s :: pComponentPrivate->curState = %d\n",__LINE__,
                   __FUNCTION__,pComponentPrivate->curState);
    if(pComponentPrivate->curState != OMX_StateExecuting && 
       pComponentPrivate->curState != OMX_StatePause) {
        G722ENC_DPRINT("%d :: %s ::  OMX_ErrorIncorrectStateOperation\n", __LINE__,__FUNCTION__);
        return (OMX_ErrorIncorrectStateOperation);
    }

    G722ENC_DPRINT("\n------------------------------------------\n\n");
    G722ENC_DPRINT ("%d :: %s :: Component Sending Filled ip buff %p \
                             to Component Thread\n",__LINE__,__FUNCTION__,
                    pBuffer);
    G722ENC_DPRINT("\n------------------------------------------\n\n");

    pComponentPrivate->app_nBuf--;

    pComponentPrivate->pMarkData = pBuffer->pMarkData;
    pComponentPrivate->hMarkTargetComponent = pBuffer->hMarkTargetComponent;
    
    ret = write (pComponentPrivate->dataPipe[1], &pBuffer,
                 sizeof(OMX_BUFFERHEADERTYPE*));
    if (ret == -1) {
        G722ENC_DPRINT("%d :: %s ::  OMX_ErrorHardware\n", __LINE__,__FUNCTION__);
        return (OMX_ErrorHardware);
    }
    
    /* iLBC, GSM Fr specific
       pComponentPrivate->nEmptyThisBufferCount++;
    */

    return eError;
}

/* ================================================================================= */
/**
 * @fn FillThisBuffer() description for FillThisBuffer  
 FillThisBuffer().  
 This method is called by the IL client to send an empty output buffer to the 
 component
 *
 *  @see         OMX_Core.h
 * 
 *  ACAs note: NORMALIZED
 */
/* ================================================================================ */
static OMX_ERRORTYPE FillThisBuffer (OMX_HANDLETYPE pComponent,
                                     OMX_BUFFERHEADERTYPE* pBuffer)
{
    OMX_ERRORTYPE                 eError            = OMX_ErrorNone;
    OMX_COMPONENTTYPE            *pHandle           = (OMX_COMPONENTTYPE *)pComponent;
    G722ENC_COMPONENT_PRIVATE    *pComponentPrivate = (G722ENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef = NULL;
    ssize_t ret = 0;

    pPortDef = ((G722ENC_COMPONENT_PRIVATE*)
                pComponentPrivate)->pPortDef [G722ENC_OUTPUT_PORT];

#ifdef _ERROR_PROPAGATION__
    if (pComponentPrivate->curState == OMX_StateInvalid){
        G722ENC_DPRINT("%d :: %s ::  OMX_ErrorInvalidState\n", __LINE__,__FUNCTION__);
        return (OMX_ErrorInvalidState);
    }
#endif
    if(!pPortDef->bEnabled){
        G722ENC_DPRINT("%d :: %s ::  OMX_ErrorIncorrectStateOperation\n", __LINE__,__FUNCTION__);
        return (OMX_ErrorIncorrectStateOperation);
    }
    if (pBuffer == NULL){
        G722ENC_DPRINT("%d :: %s ::  OMX_ErrorBadParameter\n", __LINE__,__FUNCTION__);
        return (OMX_ErrorBadParameter);
    }
    if (pBuffer->nSize != sizeof(OMX_BUFFERHEADERTYPE)){
        G722ENC_DPRINT("%d :: %s ::  OMX_ErrorBadParameter\n", __LINE__,__FUNCTION__);
        return (OMX_ErrorBadParameter);
    }
    if (pBuffer->nVersion.nVersion != pComponentPrivate->nVersion){
        G722ENC_DPRINT("%d :: %s ::  OMX_ErrorVersionMismatch\n", __LINE__,__FUNCTION__);
        return (OMX_ErrorVersionMismatch);
    }
    if (pBuffer->nOutputPortIndex != G722ENC_OUTPUT_PORT){
        G722ENC_DPRINT("%d :: %s ::  OMX_ErrorBadPortIndex\n", __LINE__,__FUNCTION__);
        return (OMX_ErrorBadPortIndex);
    }
    G722ENC_DPRINT("%d :: %s :: pComponentPrivate->curState = %d\n",__LINE__,
                   __FUNCTION__,pComponentPrivate->curState);

    if(pComponentPrivate->curState != OMX_StateExecuting && 
       pComponentPrivate->curState != OMX_StatePause){
        G722ENC_DPRINT("%d :: %s ::  OMX_ErrorIncorrectStateOperation\n", __LINE__,__FUNCTION__);
        return (OMX_ErrorIncorrectStateOperation);
    }
    G722ENC_DPRINT("\n------------------------------------------\n\n");
    G722ENC_DPRINT ("%d :: %s :: Component Sending Emptied op buff %p \
                             to Component Thread\n",__LINE__,__FUNCTION__,
                    pBuffer);
    G722ENC_DPRINT("\n------------------------------------------\n\n");

    pComponentPrivate->app_nBuf--;

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
    ret = write (pComponentPrivate->dataPipe[1], &pBuffer,
                 sizeof (OMX_BUFFERHEADERTYPE*));
    if (ret == -1){
        G722ENC_DPRINT("%d :: %s ::  OMX_ErrorHardware\n", __LINE__,__FUNCTION__);
        return (OMX_ErrorHardware);
    }
    /** iLBC, gsm fr specific
        pComponentPrivate->nFillThisBufferCount++;
    */

    return eError;
}

/* ================================================================================= */
/**
 * @fn ComponentDeInit() description for ComponentDeInit  
 ComponentDeInit().  
 Called by the OMX Core to destroy the component 
 *
 *  @see         OMX_Core.h
 */
/* ================================================================================ */
static OMX_ERRORTYPE ComponentDeInit(OMX_HANDLETYPE pHandle)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pComponent = (OMX_COMPONENTTYPE *)pHandle;
    G722ENC_COMPONENT_PRIVATE *pComponentPrivate =
        (G722ENC_COMPONENT_PRIVATE *)pComponent->pComponentPrivate;

    G722ENC_DPRINT("%d:::ComponentDeInit\n",__LINE__);

#ifdef DSP_RENDERING_ON
    /* inform audio manager to remove the streamID*/
    /* compose the data*/

    cmd_data.hComponent = pHandle;

    if (pComponentPrivate->g722Params->nSampleRate == 44100) {
        cmd_data.AM_Cmd = AM_CommandWarnSampleFreqChange;  
        cmd_data.param1 = G722ENC_AM_DEFAULT_RATE;
        if((write(pComponentPrivate->fdwrite, &cmd_data, sizeof(cmd_data)))<0) {
            G722ENC_DPRINT("[G722 Enc Component] - send command to audio manager\n");
        }
        else {
            G722ENC_DPRINT("%d:: %s [G722 Enc Component] - send Command=%d to audio manager\n", __LINE__, __FUNCTION__,  cmd_data.AM_Cmd);
        }
    }

    cmd_data.AM_Cmd = AM_Exit;

    if((write(pComponentPrivate->fdwrite, &cmd_data, sizeof(cmd_data)))<0)
        G722ENC_DPRINT("[G722 Component] - send command to audio manager\n");
    else
        G722ENC_DPRINT("%d:: %s [G722 Component] - send Command=%d to audio manager\n", __LINE__, __FUNCTION__,  cmd_data.AM_Cmd);

    close(pComponentPrivate->fdwrite);
    close(pComponentPrivate->fdread);
#endif

    if (eError != OMX_ErrorNone) {
        G722ENC_DPRINT ("%d ::Error returned from destroy\
                                ResourceManagerProxy thread\n", __LINE__);
        goto EXIT;
    }
    
    eError = G722ENC_StopComponentThread(pHandle);

    G722Enc_FreeCompResources(pComponent);
    G722ENC_MEMPRINT("%d:::[FREE] %p\n",__LINE__,pComponentPrivate);
    if (pComponentPrivate->pInputBufferList) {
        G722ENC_MEMPRINT("%d:::[FREE] %p\n",__LINE__,pComponentPrivate->pInputBufferList);
        free(pComponentPrivate->pInputBufferList);
        pComponentPrivate->pInputBufferList = NULL;
    }

    if (pComponentPrivate->pOutputBufferList) {
        G722ENC_MEMPRINT("%d:::[FREE] %p\n",__LINE__,pComponentPrivate->pOutputBufferList);
        free(pComponentPrivate->pOutputBufferList);
        pComponentPrivate->pOutputBufferList = NULL;
    }

    OMX_G722MEMFREE_STRUCT(pComponentPrivate);
       

 EXIT:
    return eError;
}

/* ================================================================================= */
/**
 * @fn ComponentTunnelRequest() description for ComponentTunnelRequest  
 ComponentTunnelRequest().  
 Will interact with another OpenMAX component to determine if tunneling 
 is possible and to set up the tunneling if it is possible. 
 *
 *  @see         OMX_Core.h
 */
/* ================================================================================ */
static OMX_ERRORTYPE ComponentTunnelRequest (OMX_HANDLETYPE hComp,
                                             OMX_U32 nPort, OMX_HANDLETYPE hTunneledComp,
                                             OMX_U32 nTunneledPort,
                                             OMX_TUNNELSETUPTYPE* pTunnelSetup)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    G722ENC_DPRINT ("%d :: Entering ComponentTunnelRequest\n", __LINE__);
    G722ENC_DPRINT ("%d :: Inside   ComponentTunnelRequest\n", __LINE__);
    eError = OMX_ErrorNotImplemented;
    G722ENC_DPRINT ("%d :: Exiting ComponentTunnelRequest\n", __LINE__);
    return eError;
}


/* ================================================================================= */
/**
 * @fn AllocateBuffer() description for AllocateBuffer  
 AllocateBuffer().  
 Called by the OMX IL client to allocate a buffer. 
 *
 *  @see         OMX_Core.h
 */
/* ================================================================================ */
static OMX_ERRORTYPE AllocateBuffer (OMX_IN OMX_HANDLETYPE hComponent,
                                     OMX_INOUT OMX_BUFFERHEADERTYPE** pBuffer,
                                     OMX_IN OMX_U32 nPortIndex,
                                     OMX_IN OMX_PTR pAppPrivate,
                                     OMX_IN OMX_U32 nSizeBytes)
{
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef = NULL;
    G722ENC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_BUFFERHEADERTYPE *pBufferHeader = NULL;

    G722ENC_DPRINT ("%d :: Entering AllocateBuffer\n", __LINE__);
    G722ENC_DPRINT ("%d :: pBuffer = 0x%x\n", __LINE__,pBuffer);

    pComponentPrivate = (G722ENC_COMPONENT_PRIVATE *)
        (((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);

    pPortDef = ((G722ENC_COMPONENT_PRIVATE*) 
                pComponentPrivate)->pPortDef[nPortIndex];
    G722ENC_DPRINT ("%d :: pPortDef = 0x%x\n", __LINE__,pPortDef);
    G722ENC_DPRINT ("%d :: pPortDef->bEnabled = %d\n", __LINE__,pPortDef->bEnabled);

    G722ENC_DPRINT ("pPortDef->bEnabled = %d\n", pPortDef->bEnabled);
    if(!(pPortDef->bEnabled))
    {
        pComponentPrivate->AlloBuf_waitingsignal = 1;
#ifndef UNDER_CE        
        G722ENC_DPRINT("\n\n AllocateBuffer: Waiting for signal.\n\n\n");
        pthread_mutex_lock(&pComponentPrivate->AlloBuf_mutex); 
        pthread_cond_wait(&pComponentPrivate->AlloBuf_threshold, &pComponentPrivate->AlloBuf_mutex);      
        pthread_mutex_unlock(&pComponentPrivate->AlloBuf_mutex);
        G722ENC_DPRINT("\n\n AllocateBuffer: Signal received!!!\n\n\n");
#else
        OMX_WaitForEvent(&(pComponentPrivate->AlloBuf_event));
#endif
    }
    /*pBufferHeader = (OMX_BUFFERHEADERTYPE*)malloc(sizeof(OMX_BUFFERHEADERTYPE));
      G722ENC_MEMPRINT("%d:::[ALLOC] %p\n",__LINE__,pBufferHeader);*/
    OMX_G722MALLOC_STRUCT (pBufferHeader, OMX_BUFFERHEADERTYPE);

    if (pBufferHeader == NULL) {
        G722ENC_DPRINT("pBufferHeader = %p - about to exit with OMX_ErrorInsufficientResources\n",pBufferHeader);
        eError = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    
    
    pBufferHeader->pBuffer = (OMX_U8 *)malloc(nSizeBytes + G722ENC_CACHE_ALIGN_MALLOC);
    
    G722ENC_MEMPRINT("%d:::[ALLOC] %p\n",__LINE__,pBufferHeader->pBuffer);
    memset (pBufferHeader->pBuffer, 0, nSizeBytes + G722ENC_CACHE_ALIGN_MALLOC);
    /*memset(pBufferHeader, 0x0, sizeof(OMX_BUFFERHEADERTYPE));*/

    pBufferHeader->pBuffer += G722ENC_CACHE_ALIGN_OFFSET;
    if(NULL == pBufferHeader->pBuffer) {
        G722ENC_DPRINT("%d :: Malloc Failed\n");
        /* Free previously allocated memory before bailing */

        if (pBufferHeader) {
            G722ENC_MEMPRINT("%d:::[FREE] %p\n",__LINE__,pBufferHeader);
            free(pBufferHeader);    
            pBufferHeader = NULL;
        }
        goto EXIT;
    }

    if (nPortIndex == G722ENC_INPUT_PORT) {
        pBufferHeader->nInputPortIndex = nPortIndex;
        pBufferHeader->nOutputPortIndex = -1; 
        pComponentPrivate->pInputBufferList->pBufHdr[pComponentPrivate->pInputBufferList->numBuffers] = pBufferHeader;
        G722ENC_DPRINT("pComponentPrivate->pInputBufferList->pBufHdr[%d] = %p\n",
                       pComponentPrivate->pInputBufferList->numBuffers,pComponentPrivate->pInputBufferList->pBufHdr[pComponentPrivate->pInputBufferList->numBuffers]);
        pComponentPrivate->pInputBufferList->bBufferPending[pComponentPrivate->pInputBufferList->numBuffers] = 0;
        pComponentPrivate->pInputBufferList->bufferOwner[pComponentPrivate->pInputBufferList->numBuffers++] = 1;
        G722ENC_DPRINT("Allocate Buffer Line %d\n",__LINE__);
        G722ENC_DPRINT("pComponentPrivate->pInputBufferList->numBuffers = %d\n",pComponentPrivate->pInputBufferList->numBuffers);
        G722ENC_DPRINT("pPortDef->nBufferCountMin = %d\n",pPortDef->nBufferCountMin);
        if (pComponentPrivate->pInputBufferList->numBuffers == pPortDef->nBufferCountActual) {
            pPortDef->bPopulated = OMX_TRUE;
        }
    }
    else if (nPortIndex == G722ENC_OUTPUT_PORT) {
        pBufferHeader->nInputPortIndex = -1;
        pBufferHeader->nOutputPortIndex = nPortIndex; 
        pComponentPrivate->pOutputBufferList->pBufHdr[pComponentPrivate->pOutputBufferList->numBuffers] = pBufferHeader;
        pComponentPrivate->pOutputBufferList->bBufferPending[pComponentPrivate->pOutputBufferList->numBuffers] = 0;
        G722ENC_DPRINT("pComponentPrivate->pOutputBufferList->pBufHdr[%d] = %p\n",pComponentPrivate->pOutputBufferList->numBuffers,pComponentPrivate->pOutputBufferList->pBufHdr[pComponentPrivate->pOutputBufferList->numBuffers]);
        pComponentPrivate->pOutputBufferList->bufferOwner[pComponentPrivate->pOutputBufferList->numBuffers++] = 1;
        if (pComponentPrivate->pOutputBufferList->numBuffers == pPortDef->nBufferCountActual) {
            pPortDef->bPopulated = OMX_TRUE;
        }

    }
    else {
        eError = OMX_ErrorBadPortIndex;
        goto EXIT;
    }
    

    if((pComponentPrivate->pPortDef[G722ENC_OUTPUT_PORT]->bPopulated == pComponentPrivate->pPortDef[G722ENC_OUTPUT_PORT]->bEnabled)&&
       (pComponentPrivate->pPortDef[G722ENC_INPUT_PORT]->bPopulated == pComponentPrivate->pPortDef[G722ENC_INPUT_PORT]->bEnabled) &&
       (pComponentPrivate->InLoaded_readytoidle))
    {
        pComponentPrivate->InLoaded_readytoidle = 0;  
#ifndef UNDER_CE                  
        pthread_mutex_lock(&pComponentPrivate->InLoaded_mutex);
        G722ENC_DPRINT("\n\n WAKE UP!! Allocate Buffer: buffers populated.\n\n");
        pthread_cond_signal(&pComponentPrivate->InLoaded_threshold);
        /* Sending signal. */ 
        G722ENC_DPRINT("\n\n WAKE UP!! Allocate Buffer: Sending signal from Loaded to Ilde state!\n\n");
        pthread_mutex_unlock(&pComponentPrivate->InLoaded_mutex);
#else
        OMX_SignalEvent(&(pComponentPrivate->InLoaded_event));                   

#endif
    }
    
    pBufferHeader->pAppPrivate = pAppPrivate;
    pBufferHeader->pPlatformPrivate = pComponentPrivate;
    pBufferHeader->nAllocLen = nSizeBytes;
    pBufferHeader->nVersion.s.nVersionMajor = G722ENC_MAJOR_VER;
    pBufferHeader->nVersion.s.nVersionMinor = G722ENC_MINOR_VER;
    pComponentPrivate->nVersion = pBufferHeader->nVersion.nVersion;
    pBufferHeader->nSize = sizeof(OMX_BUFFERHEADERTYPE);
    G722ENC_DPRINT("Line %d\n",__LINE__); 
    *pBuffer = pBufferHeader;
 EXIT:
    G722ENC_DPRINT ("pComponentPrivate->pInputBufferList->numBuffers = %d\n", pComponentPrivate->pInputBufferList->numBuffers);
    G722ENC_DPRINT ("pComponentPrivate->pOutputBufferList->numBuffers = %d\n", pComponentPrivate->pOutputBufferList->numBuffers);
    G722ENC_DPRINT ("pComponentPrivate->curState = %d\n", pComponentPrivate->curState);
    G722ENC_DPRINT("AllocateBuffer returning %d\n",eError);
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
    G722ENC_COMPONENT_PRIVATE * pComponentPrivate = NULL;
    OMX_BUFFERHEADERTYPE* buff = NULL;
    OMX_U8* tempBuff = NULL;
    int i = 0;
    int inputIndex = -1;
    int outputIndex = -1;
    OMX_COMPONENTTYPE *pHandle = NULL;

    pComponentPrivate = (G722ENC_COMPONENT_PRIVATE *)
        (((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);

    pHandle = (OMX_COMPONENTTYPE *) pComponentPrivate->pHandle;
    for (i=0; i < G722ENC_MAX_NUM_OF_BUFS; i++) {
        buff = pComponentPrivate->pInputBufferList->pBufHdr[i];
        if (buff == pBuffer) {
            G722ENC_DPRINT("Found matching input buffer\n");
            G722ENC_DPRINT("buff = %p\n",buff);
            G722ENC_DPRINT("pBuffer = %p\n",pBuffer);
            inputIndex = i;
            break;
        }
        else {
            G722ENC_DPRINT("This is not a match\n");
            G722ENC_DPRINT("buff = %p\n",buff);
            G722ENC_DPRINT("pBuffer = %p\n",pBuffer);
        }
    }

    for (i=0; i < G722ENC_MAX_NUM_OF_BUFS; i++) {
        buff = pComponentPrivate->pOutputBufferList->pBufHdr[i];
        if (buff == pBuffer) {
            G722ENC_DPRINT("Found matching output buffer\n");
            G722ENC_DPRINT("buff = %p\n",buff);
            G722ENC_DPRINT("pBuffer = %p\n",pBuffer);
            outputIndex = i;
            break;
        }
        else {
            G722ENC_DPRINT("This is not a match\n");
            G722ENC_DPRINT("buff = %p\n",buff);
            G722ENC_DPRINT("pBuffer = %p\n",pBuffer);
        }
    }


    if (inputIndex != -1) {
        if (pComponentPrivate->pInputBufferList->bufferOwner[inputIndex] == 1) {
            tempBuff = pComponentPrivate->pInputBufferList->pBufHdr[inputIndex]->pBuffer;
            if (tempBuff != NULL){
                tempBuff -= G722ENC_CACHE_ALIGN_OFFSET;}
            OMX_G722MEMFREE_STRUCT(tempBuff);
        }
        OMX_G722MEMFREE_STRUCT(pComponentPrivate->pInputBufferList->pBufHdr[inputIndex]);

        pComponentPrivate->pInputBufferList->numBuffers--;
        if (pComponentPrivate->pInputBufferList->numBuffers < 
            pComponentPrivate->pPortDef[G722ENC_INPUT_PORT]->nBufferCountActual) {
    
            pComponentPrivate->pPortDef[G722ENC_INPUT_PORT]->bPopulated = OMX_FALSE;
        }
        if(pComponentPrivate->pPortDef[G722ENC_INPUT_PORT]->bEnabled && 
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
            tempBuff = pComponentPrivate->pOutputBufferList->pBufHdr[outputIndex]->pBuffer;
            if (tempBuff != NULL){
                tempBuff -= G722ENC_CACHE_ALIGN_OFFSET;}
            OMX_G722MEMFREE_STRUCT(tempBuff);
        }
        OMX_G722MEMFREE_STRUCT(pComponentPrivate->pOutputBufferList->pBufHdr[outputIndex]);
        pComponentPrivate->pOutputBufferList->numBuffers--;
    
        if (pComponentPrivate->pOutputBufferList->numBuffers < 
            pComponentPrivate->pPortDef[G722ENC_OUTPUT_PORT]->nBufferCountActual) {
            pComponentPrivate->pPortDef[G722ENC_OUTPUT_PORT]->bPopulated = OMX_FALSE;
        }
        if(pComponentPrivate->pPortDef[G722ENC_OUTPUT_PORT]->bEnabled && 
           pComponentPrivate->bLoadedCommandPending == OMX_FALSE &&
           (pComponentPrivate->curState == OMX_StateIdle || 
            pComponentPrivate->curState == OMX_StateExecuting || 
            pComponentPrivate->curState == OMX_StatePause)) {
            pComponentPrivate->cbInfo.EventHandler(
                                                   pHandle, pHandle->pApplicationPrivate,
                                                   OMX_EventError, OMX_ErrorPortUnpopulated,nPortIndex, NULL);
        }
    }
    else {
        G722ENC_DPRINT("%d::Returning OMX_ErrorBadParameter\n",__LINE__);
        eError = OMX_ErrorBadParameter;
    }


    if ((!pComponentPrivate->pInputBufferList->numBuffers &&
         !pComponentPrivate->pOutputBufferList->numBuffers) &&
        pComponentPrivate->InIdle_goingtoloaded)
    {
        pComponentPrivate->InIdle_goingtoloaded = 0;                  
#ifndef UNDER_CE
        pthread_mutex_lock(&pComponentPrivate->InIdle_mutex);
        G722ENC_DPRINT("\n\n WAKE UP!! Free Buffer: No allocated buffers.\n\n");
        pthread_cond_signal(&pComponentPrivate->InIdle_threshold);
        /* Sending signal. */ 
        G722ENC_DPRINT("\n\n WAKE UP!! Free Buffer: Sending signal from Idle to Loaded state!\n\n");
        pthread_mutex_unlock(&pComponentPrivate->InIdle_mutex);
#else
        OMX_SignalEvent(&(pComponentPrivate->InIdle_event));
#endif
    }
        
    if (pComponentPrivate->bDisableCommandPending) {
        SendCommand (pComponentPrivate->pHandle,OMX_CommandPortDisable,pComponentPrivate->bDisableCommandParam,NULL);
    }
    G722ENC_DPRINT("%d :: Exiting FreeBuffer   error = %x\n", __LINE__, eError);
    G722ENC_DPRINT ("pComponentPrivate->pInputBufferList->numBuffers = %d\n", pComponentPrivate->pInputBufferList->numBuffers);
    G722ENC_DPRINT ("pComponentPrivate->pOutputBufferList->numBuffers = %d\n", pComponentPrivate->pOutputBufferList->numBuffers);
    G722ENC_DPRINT ("pComponentPrivate->curState = %d\n", pComponentPrivate->curState);
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
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef = NULL;
    G722ENC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_BUFFERHEADERTYPE *pBufferHeader = NULL;
    G722ENC_DPRINT ("%d :: Entering UseBuffer\n", __LINE__);
    G722ENC_DPRINT ("%d :: pBuffer = 0x%x\n", __LINE__,pBuffer);

    pComponentPrivate = (G722ENC_COMPONENT_PRIVATE *)
        (((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);

    pPortDef = ((G722ENC_COMPONENT_PRIVATE*) 
                pComponentPrivate)->pPortDef[nPortIndex];
    G722ENC_DPRINT ("%d :: pPortDef = 0x%x\n", __LINE__,pPortDef);
    G722ENC_DPRINT ("%d :: pPortDef = 0x%x\n", __LINE__,pPortDef);
    G722ENC_DPRINT ("%d :: pPortDef = 0x%x\n", __LINE__,pPortDef);
    G722ENC_DPRINT ("%d :: pPortDef = 0x%x\n", __LINE__,pPortDef);
    G722ENC_DPRINT ("%d :: pPortDef = 0x%x\n", __LINE__,pPortDef);
    G722ENC_DPRINT ("%d :: pPortDef->bEnabled = %d\n", __LINE__,pPortDef->bEnabled);

    G722ENC_DPRINT("Line %d\n",__LINE__); 
    if(!pPortDef->bEnabled) {
        G722ENC_DPRINT ("%d :: In UseBuffer\n", __LINE__);
        eError = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }

    G722ENC_DPRINT("Line %d\n",__LINE__); 
    G722ENC_DPRINT("nSizeBytes = %d\n",nSizeBytes);
    G722ENC_DPRINT("pPortDef->nBufferSize  = %d\n",pPortDef->nBufferSize );
    G722ENC_DPRINT("pPortDef->bPopulated  = %d\n",pPortDef->bPopulated);

    if(nSizeBytes != pPortDef->nBufferSize || pPortDef->bPopulated) {
        G722ENC_DPRINT ("%d :: In UseBuffer\n", __LINE__);
        G722ENC_DPRINT("About to return OMX_ErrorBadParameter on line %d\n",__LINE__);
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }

    G722ENC_DPRINT("Line %d\n",__LINE__); 
    /*pBufferHeader = (OMX_BUFFERHEADERTYPE*)malloc(sizeof(OMX_BUFFERHEADERTYPE));
      G722ENC_MEMPRINT("%d:::[ALLOC] %p\n",__LINE__,pBufferHeader);*/
    OMX_G722MALLOC_STRUCT (pBufferHeader, OMX_BUFFERHEADERTYPE);

    if (pBufferHeader == NULL) {
        eError = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    G722ENC_DPRINT("Line %d\n",__LINE__); 
    memset((pBufferHeader), 0x0, sizeof(OMX_BUFFERHEADERTYPE));

    G722ENC_DPRINT("Line %d\n",__LINE__); 
    if (nPortIndex == G722ENC_OUTPUT_PORT) {
        pBufferHeader->nInputPortIndex = -1;
        pBufferHeader->nOutputPortIndex = nPortIndex; 
        pComponentPrivate->pOutputBufferList->pBufHdr[pComponentPrivate->pOutputBufferList->numBuffers] = pBufferHeader;
        pComponentPrivate->pOutputBufferList->bBufferPending[pComponentPrivate->pOutputBufferList->numBuffers] = 0;
        pComponentPrivate->pOutputBufferList->bufferOwner[pComponentPrivate->pOutputBufferList->numBuffers++] = 0;
        G722ENC_DPRINT("pComponentPrivate->pOutputBufferList->numBuffers = %d\n",pComponentPrivate->pOutputBufferList->numBuffers);
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
    if((pComponentPrivate->pPortDef[G722ENC_OUTPUT_PORT]->bPopulated == pComponentPrivate->pPortDef[G722ENC_OUTPUT_PORT]->bEnabled)&&
       (pComponentPrivate->pPortDef[G722ENC_INPUT_PORT]->bPopulated == pComponentPrivate->pPortDef[G722ENC_INPUT_PORT]->bEnabled) &&
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
    G722ENC_DPRINT("Line %d\n",__LINE__); 
    pBufferHeader->pAppPrivate = pAppPrivate;
    pBufferHeader->pPlatformPrivate = pComponentPrivate;
    pBufferHeader->nAllocLen = nSizeBytes;
    pBufferHeader->nVersion.s.nVersionMajor = G722ENC_MAJOR_VER;
    pBufferHeader->nVersion.s.nVersionMinor = G722ENC_MINOR_VER;
    pComponentPrivate->nVersion = pBufferHeader->nVersion.nVersion;
    pBufferHeader->pBuffer = pBuffer;
    pBufferHeader->nSize = sizeof(OMX_BUFFERHEADERTYPE);
    G722ENC_DPRINT("Line %d\n",__LINE__); 
    *ppBufferHdr = pBufferHeader;
    G722ENC_DPRINT("pBufferHeader = %p\n",pBufferHeader);
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

    G722ENC_DPRINT("GetExtensionIndex\n");
    if (!(strcmp(cParameterName,"OMX.TI.index.config.g722headerinfo"))) {
        *pIndexType = OMX_IndexCustomG722EncHeaderInfoConfig;
        G722ENC_DPRINT("OMX_IndexCustomG722EncHeaderInfoConfig\n");
    }
    else if(!(strcmp(cParameterName,"OMX.TI.index.config.g722streamIDinfo"))) 
    {
        *pIndexType = OMX_IndexCustomG722EncStreamIDConfig;
        G722ENC_DPRINT("OMX_IndexCustomG722EncStreamIDConfig\n");
        
    }
    else if(!(strcmp(cParameterName,"OMX.TI.index.config.g722.datapath"))) 
    {
        *pIndexType = OMX_IndexCustomG722EncDataPath;
    }
    else {
        eError = OMX_ErrorBadParameter;
    }

    G722ENC_DPRINT("Exiting GetExtensionIndex\n");
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
