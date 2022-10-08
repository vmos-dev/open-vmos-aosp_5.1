
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
 * @file OMX_G711Encoder.c
 *
 * This file implements OpenMAX (TM) 1.0 Specific APIs and its functionality
 * that is fully compliant with the Khronos OpenMAX (TM) 1.0 Specification
 *
 * @path  $(CSLPATH)\OMAPSW_MPU\linux\audio\src\openmax_il\g711_enc\src
 *
 * @rev  1.0
 */
/* ----------------------------------------------------------------------------
 *!
 *! Revision History
 *! ===================================
 *! 12-Dec-2006: Initial Version
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
#endif

#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <dbapi.h>


/*-------program files ----------------------------------------*/
#include <OMX_Component.h>
//#include <encode_common_ti.h>

#include "OMX_G711Enc_Utils.h"

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


/* interface with audio manager*/
#define FIFO1 "/dev/fifo.1"
#define FIFO2 "/dev/fifo.2"
#define PERMS 0666

#define G711_ENC_ROLE "audio_encoder.g711"

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
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef_ip=NULL, *pPortDef_op=NULL;
    G711ENC_COMPONENT_PRIVATE *pComponentPrivate=NULL;
    OMX_AUDIO_PARAM_PCMMODETYPE *G711_op=NULL;
    OMX_AUDIO_PARAM_PCMMODETYPE *G711_ip=NULL;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE*) hComp;
    G711ENC_PORT_TYPE *pCompPort = NULL;
    OMX_AUDIO_PARAM_PORTFORMATTYPE *pInPortFormat = NULL;
    OMX_AUDIO_PARAM_PORTFORMATTYPE *pOutPortFormat = NULL;
    int i = 0;

    G711ENC_DPRINT("%d :: Entering OMX_ComponentInit\n", __LINE__);
    /*Set the all component function pointer to the handle */
    pHandle->SetCallbacks               = SetCallbacks;
    pHandle->GetComponentVersion        = GetComponentVersion;
    pHandle->SendCommand                = SendCommand;
    pHandle->GetParameter               = GetParameter;
    pHandle->SetParameter               = SetParameter;
    pHandle->GetConfig                  = GetConfig;
    pHandle->SetConfig                  = SetConfig;
    pHandle->GetState                   = GetState;
    pHandle->EmptyThisBuffer            = EmptyThisBuffer;
    pHandle->FillThisBuffer             = FillThisBuffer;
    pHandle->ComponentTunnelRequest     = ComponentTunnelRequest;
    pHandle->ComponentDeInit            = ComponentDeInit;
    pHandle->AllocateBuffer             = AllocateBuffer;
    pHandle->FreeBuffer                 = FreeBuffer;
    pHandle->UseBuffer                  = UseBuffer;
    pHandle->GetExtensionIndex          = GetExtensionIndex;
    pHandle->ComponentRoleEnum          = ComponentRoleEnum;    

    /*Allocate the memory for Component private data area */
    G711ENC_OMX_MALLOC_STRUCT(pHandle->pComponentPrivate, G711ENC_COMPONENT_PRIVATE);

    ((G711ENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate)->pHandle = pHandle;
    pComponentPrivate = pHandle->pComponentPrivate;

    G711ENC_OMX_MALLOC_STRUCT(pCompPort, G711ENC_PORT_TYPE);
    pComponentPrivate->pCompPort[G711ENC_INPUT_PORT] = pCompPort;

    G711ENC_OMX_MALLOC_STRUCT(pCompPort, G711ENC_PORT_TYPE);
    pComponentPrivate->pCompPort[G711ENC_OUTPUT_PORT] = pCompPort;

    G711ENC_OMX_MALLOC_STRUCT(pComponentPrivate->sPortParam, OMX_PORT_PARAM_TYPE);
    OMX_G711ENC_INIT_STRUCT(pComponentPrivate->sPortParam, OMX_PORT_PARAM_TYPE);

    /* Initialize sPortParam data structures to default values */
    pComponentPrivate->sPortParam->nPorts = 0x2;
    pComponentPrivate->sPortParam->nStartPortNumber = 0x0;

    /* Malloc and Set pPriorityMgmt defaults */
    G711ENC_OMX_MALLOC_STRUCT(pComponentPrivate->sPriorityMgmt, OMX_PRIORITYMGMTTYPE);
    OMX_G711ENC_INIT_STRUCT(pComponentPrivate->sPriorityMgmt, OMX_PRIORITYMGMTTYPE);

    /* Initialize sPriorityMgmt data structures to default values */
    pComponentPrivate->sPriorityMgmt->nGroupPriority = -1;
    pComponentPrivate->sPriorityMgmt->nGroupID = -1;

    G711ENC_OMX_MALLOC_STRUCT(G711_op, OMX_AUDIO_PARAM_PCMMODETYPE);
    OMX_G711ENC_INIT_STRUCT(G711_op, OMX_AUDIO_PARAM_PCMMODETYPE);
    pComponentPrivate->G711Params[G711ENC_OUTPUT_PORT] = G711_op;
    /*Input PCM format defaults */
    G711_op->nPortIndex = G711ENC_OUTPUT_PORT;
    G711_op->nChannels  = 1;
    G711_op->eNumData = OMX_NumericalDataUnsigned;
    G711_op->eEndian  = OMX_EndianLittle;
    G711_op->bInterleaved = OMX_FALSE;
    G711_op->nSamplingRate = 0;
    G711_op->nBitPerSample = 8;
    G711_op->ePCMMode = OMX_AUDIO_PCMModeLinear;

    G711ENC_OMX_MALLOC_STRUCT(G711_ip, OMX_AUDIO_PARAM_PCMMODETYPE);
    OMX_G711ENC_INIT_STRUCT(G711_ip, OMX_AUDIO_PARAM_PCMMODETYPE);
    pComponentPrivate->G711Params[G711ENC_INPUT_PORT] = G711_ip;
    /* Output PCM format defaults */
    G711_ip->nPortIndex = G711ENC_INPUT_PORT;
    G711_ip->nChannels  = 1;
    G711_ip->eNumData = OMX_NumericalDataUnsigned;
    G711_ip->eEndian  = OMX_EndianLittle;
    G711_ip->bInterleaved = OMX_FALSE;
    G711_ip->nSamplingRate = 8000;
    G711_ip->nBitPerSample = 16;
    G711_ip->ePCMMode = OMX_AUDIO_PCMModeLinear;

    /* Initialize number of input buffers */
    G711ENC_OMX_MALLOC_STRUCT(pComponentPrivate->pInputBufferList, G711ENC_BUFFERLIST);
    pComponentPrivate->pInputBufferList->numBuffers = 0;

    /* Initialize number of output buffers */
    G711ENC_OMX_MALLOC_STRUCT(pComponentPrivate->pOutputBufferList, G711ENC_BUFFERLIST);
    pComponentPrivate->pOutputBufferList->numBuffers = 0;

    for (i=0; i < G711ENC_MAX_NUM_OF_BUFS; i++) {
        pComponentPrivate->pOutputBufferList->pBufHdr[i] = NULL;
        pComponentPrivate->pInputBufferList->pBufHdr[i]  = NULL;
    }

    /* Set input port defaults */
    G711ENC_OMX_MALLOC_STRUCT(pPortDef_ip, OMX_PARAM_PORTDEFINITIONTYPE);
    OMX_G711ENC_INIT_STRUCT(pPortDef_ip, OMX_PARAM_PORTDEFINITIONTYPE);
    pComponentPrivate->pPortDef[G711ENC_INPUT_PORT] = pPortDef_ip;

    pPortDef_ip->nPortIndex                         = G711ENC_INPUT_PORT;
    pPortDef_ip->eDir                               = OMX_DirInput;
    pPortDef_ip->nBufferCountActual                 = G711ENC_NUM_INPUT_BUFFERS;
    pPortDef_ip->nBufferCountMin                    = G711ENC_NUM_INPUT_BUFFERS;
    pPortDef_ip->nBufferSize                        = G711ENC_INPUT_BUFFER_SIZE_DASF;
    pPortDef_ip->bEnabled                           = OMX_TRUE;
    pPortDef_ip->bPopulated                         = OMX_FALSE;
    pPortDef_ip->eDomain                            = OMX_PortDomainAudio;
    pPortDef_ip->format.audio.eEncoding             = OMX_AUDIO_CodingPCM;
    pPortDef_ip->format.audio.pNativeRender         = NULL;
    pPortDef_ip->format.audio.bFlagErrorConcealment = OMX_FALSE;

    /* Set output port defaults */
    G711ENC_OMX_MALLOC_STRUCT(pPortDef_op, OMX_PARAM_PORTDEFINITIONTYPE);
    OMX_G711ENC_INIT_STRUCT(pPortDef_op, OMX_PARAM_PORTDEFINITIONTYPE);
    pComponentPrivate->pPortDef[G711ENC_OUTPUT_PORT] = pPortDef_op;

    pPortDef_op->nPortIndex                         = G711ENC_OUTPUT_PORT;
    pPortDef_op->eDir                               = OMX_DirOutput;
    pPortDef_op->nBufferCountMin                    = G711ENC_NUM_OUTPUT_BUFFERS;
    pPortDef_op->nBufferCountActual                 = G711ENC_NUM_OUTPUT_BUFFERS;
    pPortDef_op->nBufferSize                        = G711ENC_OUTPUT_BUFFER_SIZE;
    pPortDef_op->bEnabled                           = OMX_TRUE;
    pPortDef_op->bPopulated                         = OMX_FALSE;
    pPortDef_op->eDomain                            = OMX_PortDomainAudio;
    pPortDef_op->format.audio.eEncoding             = OMX_AUDIO_CodingG711;
    pPortDef_op->format.audio.pNativeRender         = NULL;
    pPortDef_op->format.audio.bFlagErrorConcealment = OMX_FALSE;

    /* Set input port format defaults */
    G711ENC_OMX_MALLOC_STRUCT(pComponentPrivate->pCompPort[G711ENC_INPUT_PORT]->pPortFormat, OMX_AUDIO_PARAM_PORTFORMATTYPE);
    OMX_G711ENC_INIT_STRUCT(pComponentPrivate->pCompPort[G711ENC_INPUT_PORT]->pPortFormat, OMX_AUDIO_PARAM_PORTFORMATTYPE);

    pInPortFormat = pComponentPrivate->pCompPort[G711ENC_INPUT_PORT]->pPortFormat;
    OMX_G711ENC_INIT_STRUCT(pInPortFormat, OMX_AUDIO_PARAM_PORTFORMATTYPE);

    pInPortFormat->nPortIndex     = G711ENC_INPUT_PORT;
    pInPortFormat->nIndex         = OMX_IndexParamAudioPcm;
    pInPortFormat->eEncoding          = OMX_AUDIO_CodingPCM;

    /* Set output port format defaults */
    G711ENC_OMX_MALLOC_STRUCT(pComponentPrivate->pCompPort[G711ENC_OUTPUT_PORT]->pPortFormat, OMX_AUDIO_PARAM_PORTFORMATTYPE);
    OMX_G711ENC_INIT_STRUCT(pComponentPrivate->pCompPort[G711ENC_OUTPUT_PORT]->pPortFormat, OMX_AUDIO_PARAM_PORTFORMATTYPE);

    pOutPortFormat = pComponentPrivate->pCompPort[G711ENC_OUTPUT_PORT]->pPortFormat;
    OMX_G711ENC_INIT_STRUCT(pOutPortFormat, OMX_AUDIO_PARAM_PORTFORMATTYPE);

    pOutPortFormat->nPortIndex         = G711ENC_OUTPUT_PORT;
    pOutPortFormat->nIndex           = OMX_IndexParamAudioPcm;
    pOutPortFormat->eEncoding          = OMX_AUDIO_CodingG711;

    G711ENC_DPRINT("%d :: Setting dasf & acdn and MultiFrame modes to 0\n",__LINE__);
    pComponentPrivate->dasfMode = 0;
    pComponentPrivate->acdnMode = 0;
    pComponentPrivate->bPortDefsAllocated = 0;
    pComponentPrivate->bCompThreadStarted = 0;
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
    pComponentPrivate->bIdleCommandPending = 0;
    pComponentPrivate->bDisableCommandParam = 0;
    pComponentPrivate->bIsEOFSent = 0;
    pComponentPrivate->bBypassDSP = 0;
    pComponentPrivate->bEnableCommandParam = 0;
    pComponentPrivate->nRuntimeInputBuffers = 0;
    pComponentPrivate->nRuntimeOutputBuffers = 0;
    pComponentPrivate->nUnhandledEmptyThisBuffers = 0;
    pComponentPrivate->nUnhandledFillThisBuffers = 0;

    for (i=0; i < G711ENC_MAX_NUM_OF_BUFS; i++) {
        pComponentPrivate->pInputBufHdrPending[i] = NULL;
        pComponentPrivate->pOutputBufHdrPending[i] = NULL;
    }
    pComponentPrivate->nInvalidFrameCount = 0;
    pComponentPrivate->nNumInputBufPending = 0;
    pComponentPrivate->nNumOutputBufPending = 0;
    pComponentPrivate->bDisableCommandPending = 0;
    pComponentPrivate->bEnableCommandPending = 0;
    pComponentPrivate->bNoIdleOnStop= OMX_FALSE;
    pComponentPrivate->bIdleCommandPending = OMX_FALSE;
    pComponentPrivate->nOutStandingFillDones = 0;
    pComponentPrivate->bDspStoppedWhileExecuting = OMX_FALSE;
    pComponentPrivate->bPreempted = OMX_FALSE; 

    /* Default create phase parameters */
    /*pComponentPrivate->frametype = 0;
      pComponentPrivate->vaumode = 2;
      pComponentPrivate->vauthreshold = 0;
      pComponentPrivate->vaunumber = 0;
      pComponentPrivate->nmunoise = 2;
      pComponentPrivate->lporder = 5;*/

    /* initialize role name */
    strcpy((char*)pComponentPrivate->componentRole.cRole,G711_ENC_ROLE);

    G711ENC_OMX_MALLOC_SIZE(pComponentPrivate->sDeviceString,
                            100*sizeof(OMX_STRING),
                            OMX_STRING);
    /* Initialize device string to the default value */
    strcpy((char*)pComponentPrivate->sDeviceString,"/eteedn:i0:o0/codec\0");

    /* Initialize LMCL back up pointer*/
    pComponentPrivate->ptrLibLCML = NULL;

#ifndef UNDER_CE
    pthread_mutex_init(&pComponentPrivate->AlloBuf_mutex, NULL);
    pthread_cond_init (&pComponentPrivate->AlloBuf_threshold, NULL);
    pComponentPrivate->AlloBuf_waitingsignal = 0;

    pthread_mutex_init(&pComponentPrivate->InIdle_mutex, NULL);
    pthread_cond_init (&pComponentPrivate->InIdle_threshold, NULL);
    pComponentPrivate->InIdle_goingtoloaded = 0;

    pthread_mutex_init(&pComponentPrivate->InLoaded_mutex, NULL);
    pthread_cond_init (&pComponentPrivate->InLoaded_threshold, NULL);
    pComponentPrivate->InLoaded_readytoidle = 0;
#endif


#ifdef RESOURCE_MANAGER_ENABLED
    eError = RMProxy_NewInitalize();
    G711ENC_DPRINT("%d :: OMX_ComponentInit\n", __LINE__);
    if (eError != OMX_ErrorNone) {
        G711ENC_DPRINT("%d :: Error returned from loading ResourceManagerProxy thread\n",__LINE__);
        goto EXIT;
    }
#endif

    eError = G711ENC_StartComponentThread(pHandle);
    G711ENC_DPRINT("%d :: OMX_ComponentInit\n", __LINE__);
    if (eError != OMX_ErrorNone) {
        G711ENC_PRINT("%d :: Error returned from the Component\n",__LINE__);
        goto EXIT;
    }

#ifdef DSP_RENDERING_ON
    G711ENC_DPRINT("%d :: OMX_ComponentInit\n", __LINE__);
    if((pComponentPrivate->fdwrite=open(FIFO1,O_WRONLY))<0) {
        G711ENC_DPRINT("%d :: [G711E Component] - failure to open WRITE pipe\n",__LINE__);
    }
    G711ENC_DPRINT("%d :: OMX_ComponentInit\n", __LINE__);
    if((pComponentPrivate->fdread=open(FIFO2,O_RDONLY))<0) {
        G711ENC_DPRINT("%d :: [G711E Component] - failure to open READ pipe\n",__LINE__);
    }
    G711ENC_DPRINT("%d :: OMX_ComponentInit\n", __LINE__);
#endif

 EXIT:
    G711ENC_DPRINT("%d :: Exiting OMX_ComponentInit\n", __LINE__);
    G711ENC_DPRINT("%d :: Returning = 0x%x\n",__LINE__,eError);
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

    G711ENC_COMPONENT_PRIVATE *pComponentPrivate =
        (G711ENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    G711ENC_DPRINT("%d :: Entering SetCallbacks\n", __LINE__);
    if (pCallBacks == NULL) {
        eError = OMX_ErrorBadParameter;
        G711ENC_DPRINT("%d :: Received the empty callbacks from the application\n",__LINE__);
        goto EXIT;
    }

    /*Copy the callbacks of the application to the component private*/
    memcpy (&(pComponentPrivate->cbInfo), pCallBacks, sizeof(OMX_CALLBACKTYPE));

    /*copy the application private data to component memory */
    pHandle->pApplicationPrivate = pAppData;

    pComponentPrivate->curState = OMX_StateLoaded;

 EXIT:
    G711ENC_DPRINT("%d :: Exiting SetCallbacks\n", __LINE__);
    G711ENC_DPRINT("%d :: Returning = 0x%x\n",__LINE__,eError);
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
    G711ENC_COMPONENT_PRIVATE *pComponentPrivate = (G711ENC_COMPONENT_PRIVATE *) pHandle->pComponentPrivate;
    G711ENC_DPRINT("%d :: Entering GetComponentVersion\n", __LINE__);
    /* Copy component version structure */
    if(pComponentVersion != NULL && pComponentName != NULL) {
        strcpy(pComponentName, pComponentPrivate->cComponentName);
        memcpy(pComponentVersion, &(pComponentPrivate->ComponentVersion.s), sizeof(pComponentPrivate->ComponentVersion.s));
    }
    else {
        G711ENC_DPRINT("%d :: OMX_ErrorBadParameter from GetComponentVersion",__LINE__);
        eError = OMX_ErrorBadParameter;
    }

    G711ENC_DPRINT("%d :: Exiting GetComponentVersion\n", __LINE__);
    G711ENC_DPRINT("%d :: Returning = 0x%x\n",__LINE__,eError);
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
    G711ENC_COMPONENT_PRIVATE *pCompPrivate =
        (G711ENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    int nRet = 0;
    G711ENC_DPRINT("%d :: Entering SendCommand()\n", __LINE__);
    if(pCompPrivate->curState == OMX_StateInvalid) {
        eError = OMX_ErrorInvalidState;
        G711ENC_DPRINT("%d :: Error OMX_ErrorInvalidState Sent to App\n",__LINE__);
        goto EXIT;
    }

    switch(Cmd) {
    case OMX_CommandStateSet:
        G711ENC_DPRINT("%d :: OMX_CommandStateSet SendCommand\n",__LINE__);
        G711ENC_DPRINT("%d :: OMX_CommandStateSet nparam: %d \n",__LINE__,(int)nParam);
            
        if (nParam == OMX_StateLoaded) {
            pCompPrivate->bLoadedCommandPending = OMX_TRUE;
        }
        if(pCompPrivate->curState == OMX_StateLoaded) {
            if((nParam == OMX_StateExecuting) || (nParam == OMX_StatePause)) {
                pCompPrivate->cbInfo.EventHandler ( pHandle,
                                                    pHandle->pApplicationPrivate,
                                                    OMX_EventError,
                                                    OMX_ErrorIncorrectStateTransition,
                                                    0,
                                                    NULL);
                goto EXIT;
            }

            if(nParam == OMX_StateInvalid) {
                G711ENC_DPRINT("%d :: OMX_CommandStateSet SendCommand\n",__LINE__);
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
        G711ENC_DPRINT("%d :: OMX_CommandFlush SendCommand\n",__LINE__);
        if(nParam > 1 && nParam != -1) {
            eError = OMX_ErrorBadPortIndex;
            G711ENC_DPRINT("%d :: OMX_ErrorBadPortIndex from SendCommand",__LINE__);
            goto EXIT;
        }
        break;
    case OMX_CommandPortDisable:
        G711ENC_DPRINT("%d :: OMX_CommandPortDisable SendCommand\n",__LINE__);
        break;
    case OMX_CommandPortEnable:
        G711ENC_DPRINT("%d :: OMX_CommandPortEnable SendCommand\n",__LINE__);
        break;
    case OMX_CommandMarkBuffer:
        G711ENC_DPRINT("%d :: OMX_CommandMarkBuffer SendCommand\n",__LINE__);
        if (nParam > 0) {
            eError = OMX_ErrorBadPortIndex;
            G711ENC_DPRINT("%d :: OMX_ErrorBadPortIndex from SendCommand",__LINE__);
            goto EXIT;
        }
        break;
    default:
        G711ENC_DPRINT("%d :: Command Received Default eError\n",__LINE__);
        pCompPrivate->cbInfo.EventHandler ( pHandle,
                                            pHandle->pApplicationPrivate,
                                            OMX_EventError,
                                            OMX_ErrorUndefined,
                                            0,
                                            "Invalid Command");
        break;

    }

    nRet = write (pCompPrivate->cmdPipe[1], &Cmd, sizeof(Cmd));
    if (nRet == -1) {
        eError = OMX_ErrorInsufficientResources;
        G711ENC_PRINT("%d :: OMX_ErrorInsufficientResources from SendCommand",__LINE__);
        goto EXIT;
    }

    if (Cmd == OMX_CommandMarkBuffer) {
        nRet = write(pCompPrivate->cmdDataPipe[1], &pCmdData,sizeof(OMX_PTR));
    } else {
        nRet = write(pCompPrivate->cmdDataPipe[1], &nParam,sizeof(OMX_U32));
    }
    if (nRet == -1) {
        G711ENC_DPRINT("%d :: OMX_ErrorInsufficientResources from SendCommand",__LINE__);
        eError = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

 EXIT:
    G711ENC_DPRINT("%d :: Exiting SendCommand()\n", __LINE__);
    G711ENC_DPRINT("%d :: Returning = 0x%x\n",__LINE__,eError);
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
    G711ENC_COMPONENT_PRIVATE  *pComponentPrivate = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE *pParameterStructure = NULL;

    pComponentPrivate = (G711ENC_COMPONENT_PRIVATE *)(((OMX_COMPONENTTYPE*)hComp)->pComponentPrivate);
    pParameterStructure = (OMX_PARAM_PORTDEFINITIONTYPE*)ComponentParameterStructure;
    
    G711ENC_DPRINT("%d :: Entering the GetParameter\n",__LINE__);
    if (pParameterStructure == NULL) {
        eError = OMX_ErrorBadParameter;
        G711ENC_DPRINT("%d :: OMX_ErrorBadPortIndex from GetParameter",__LINE__);
        goto EXIT;
    }

    if(pComponentPrivate->curState == OMX_StateInvalid) {
        eError = OMX_ErrorIncorrectStateOperation;
        G711ENC_DPRINT("%d :: OMX_ErrorIncorrectStateOperation from GetParameter",__LINE__);
        goto EXIT;
    }

    switch(nParamIndex){
    case OMX_IndexParamAudioInit:
        if (pComponentPrivate->sPortParam == NULL) {
            eError = OMX_ErrorBadParameter;
	    break;
        }
        G711ENC_DPRINT("%d :: GetParameter OMX_IndexParamAudioInit \n",__LINE__);
        memcpy(ComponentParameterStructure, pComponentPrivate->sPortParam, sizeof(OMX_PORT_PARAM_TYPE));
        break;

    case OMX_IndexParamPortDefinition:
        G711ENC_DPRINT("%d :: GetParameter OMX_IndexParamPortDefinition \n",__LINE__);
        if(((OMX_PARAM_PORTDEFINITIONTYPE *)(ComponentParameterStructure))->nPortIndex ==
           pComponentPrivate->pPortDef[G711ENC_INPUT_PORT]->nPortIndex) {
            memcpy(ComponentParameterStructure, pComponentPrivate->pPortDef[G711ENC_INPUT_PORT], sizeof(OMX_PARAM_PORTDEFINITIONTYPE)); 
        }
        else if(((OMX_PARAM_PORTDEFINITIONTYPE *)(ComponentParameterStructure))->nPortIndex ==
                pComponentPrivate->pPortDef[G711ENC_OUTPUT_PORT]->nPortIndex) {
            memcpy(ComponentParameterStructure, pComponentPrivate->pPortDef[G711ENC_OUTPUT_PORT], sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
        } 
        else {
            G711ENC_DPRINT("%d :: OMX_ErrorBadPortIndex from GetParameter \n",__LINE__);
            eError = OMX_ErrorBadPortIndex;
        }
        break;

    case OMX_IndexParamAudioPortFormat:
        G711ENC_DPRINT("%d :: GetParameter OMX_IndexParamAudioPortFormat \n",__LINE__);
        if(((OMX_AUDIO_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nPortIndex ==
           pComponentPrivate->pPortDef[G711ENC_INPUT_PORT]->nPortIndex) {
            if(((OMX_AUDIO_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nIndex >
               pComponentPrivate->pCompPort[G711ENC_INPUT_PORT]->pPortFormat->nPortIndex) {
                eError = OMX_ErrorNoMore;
            }
            else {
                memcpy(ComponentParameterStructure, pComponentPrivate->pCompPort[G711ENC_INPUT_PORT]->pPortFormat, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
            }
        }
        else if(((OMX_AUDIO_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nPortIndex ==
                pComponentPrivate->pPortDef[G711ENC_OUTPUT_PORT]->nPortIndex){
            if(((OMX_AUDIO_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nIndex >
               pComponentPrivate->pCompPort[G711ENC_OUTPUT_PORT]->pPortFormat->nPortIndex) {
                eError = OMX_ErrorNoMore;
            }
            else {
                memcpy(ComponentParameterStructure, pComponentPrivate->pCompPort[G711ENC_OUTPUT_PORT]->pPortFormat, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
            }
        }
        else {
            G711ENC_DPRINT("%d :: OMX_ErrorBadPortIndex from GetParameter \n",__LINE__);
            eError = OMX_ErrorBadPortIndex;
        }
        break;

    case OMX_IndexParamAudioPcm:
        G711ENC_DPRINT("%d :: GetParameter OMX_IndexParamAudioG711 \n",__LINE__);
        if(((OMX_AUDIO_PARAM_PCMMODETYPE *)(ComponentParameterStructure))->nPortIndex ==
           pComponentPrivate->G711Params[G711ENC_OUTPUT_PORT]->nPortIndex) {
            memcpy(ComponentParameterStructure, pComponentPrivate->G711Params[G711ENC_OUTPUT_PORT], sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
        } 
        else if(((OMX_AUDIO_PARAM_PCMMODETYPE *)(ComponentParameterStructure))->nPortIndex ==
                pComponentPrivate->G711Params[G711ENC_INPUT_PORT]->nPortIndex) {
            memcpy(ComponentParameterStructure, pComponentPrivate->G711Params[G711ENC_INPUT_PORT], sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
        }
        else {
            G711ENC_DPRINT("%d :: OMX_ErrorBadPortIndex from GetParameter \n",__LINE__);
            eError = OMX_ErrorBadPortIndex;
        }
        break;
        
    case OMX_IndexParamCompBufferSupplier:
        if(((OMX_PARAM_BUFFERSUPPLIERTYPE *)(ComponentParameterStructure))->nPortIndex == OMX_DirInput) {
            G711ENC_DPRINT(":: GetParameter OMX_IndexParamCompBufferSupplier \n");
            /*  memcpy(ComponentParameterStructure, pBufferSupplier, sizeof(OMX_PARAM_BUFFERSUPPLIERTYPE)); */
        }
        else
            if(((OMX_PARAM_BUFFERSUPPLIERTYPE *)(ComponentParameterStructure))->nPortIndex == OMX_DirOutput) {
                G711ENC_DPRINT(":: GetParameter OMX_IndexParamCompBufferSupplier \n");
                /*memcpy(ComponentParameterStructure, pBufferSupplier, sizeof(OMX_PARAM_BUFFERSUPPLIERTYPE)); */
            } 
            else {
                G711ENC_DPRINT(":: OMX_ErrorBadPortIndex from GetParameter");
                eError = OMX_ErrorBadPortIndex;
            }
        break;

    case OMX_IndexParamPriorityMgmt:
        G711ENC_DPRINT("%d :: GetParameter OMX_IndexParamPriorityMgmt \n",__LINE__);
        if (pComponentPrivate->sPriorityMgmt == NULL) {
            eError = OMX_ErrorBadParameter;
	    break;
	}
        memcpy(ComponentParameterStructure, pComponentPrivate->sPriorityMgmt, sizeof(OMX_PRIORITYMGMTTYPE));
        break;
        
    case OMX_IndexParamVideoInit:
        break;
            
    case OMX_IndexParamImageInit:
        break;
        
    case OMX_IndexParamOtherInit:
        break;  
    
    default:
        G711ENC_DPRINT("%d :: OMX_ErrorUnsupportedIndex GetParameter \n",__LINE__);
        eError = OMX_ErrorUnsupportedIndex;
        break;
    }
 EXIT:
    G711ENC_DPRINT("%d :: Exiting GetParameter\n",__LINE__);
    G711ENC_DPRINT("%d :: Returning = 0x%x\n",__LINE__,eError);
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
    G711ENC_COMPONENT_PRIVATE  *pComponentPrivate = NULL;
    OMX_AUDIO_PARAM_PORTFORMATTYPE* pComponentParam = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE *pComponentParamPort = NULL;
    OMX_AUDIO_PARAM_PCMMODETYPE *pCompG711Param = NULL;
    OMX_PARAM_COMPONENTROLETYPE  *pRole = NULL;
    OMX_PARAM_BUFFERSUPPLIERTYPE sBufferSupplier;

    pComponentPrivate = (G711ENC_COMPONENT_PRIVATE *)(((OMX_COMPONENTTYPE*)hComp)->pComponentPrivate);


    G711ENC_DPRINT("%d :: Entering the SetParameter\n",__LINE__);
    if (pCompParam == NULL) {
        eError = OMX_ErrorBadParameter;
        G711ENC_DPRINT("%d :: OMX_ErrorBadParameter from SetParameter",__LINE__);
        goto EXIT;
    }
    if (pComponentPrivate->curState != OMX_StateLoaded) {
        eError = OMX_ErrorIncorrectStateOperation;
        G711ENC_DPRINT("%d :: OMX_ErrorIncorrectStateOperation from SetParameter",__LINE__);
        goto EXIT;
    }

    switch(nParamIndex) {
    case OMX_IndexParamAudioPortFormat:
        G711ENC_DPRINT("%d :: SetParameter OMX_IndexParamAudioPortFormat \n",__LINE__);
        pComponentParam = (OMX_AUDIO_PARAM_PORTFORMATTYPE *)pCompParam;
        if ( pComponentParam->nPortIndex == pComponentPrivate->pCompPort[G711ENC_INPUT_PORT]->pPortFormat->nPortIndex ) {
            memcpy(pComponentPrivate->pCompPort[G711ENC_INPUT_PORT]->pPortFormat, pComponentParam, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
        } else if ( pComponentParam->nPortIndex == pComponentPrivate->pCompPort[G711ENC_OUTPUT_PORT]->pPortFormat->nPortIndex ) {
            memcpy(pComponentPrivate->pCompPort[G711ENC_OUTPUT_PORT]->pPortFormat, pComponentParam, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
        } else {
            G711ENC_DPRINT("%d :: OMX_ErrorBadPortIndex from SetParameter",__LINE__);
            eError = OMX_ErrorBadPortIndex;
        }
        break;
    case OMX_IndexParamAudioPcm:

        G711ENC_DPRINT("%d :: SetParameter OMX_IndexParamAudioG711 \n",__LINE__);
        pCompG711Param = (OMX_AUDIO_PARAM_PCMMODETYPE *)pCompParam;
        if (pCompG711Param->nPortIndex == OMX_DirOutput) {
            if (((G711ENC_COMPONENT_PRIVATE *)
                    pHandle->pComponentPrivate)->G711Params[G711ENC_OUTPUT_PORT] == NULL) {
                eError = OMX_ErrorBadParameter;
		break;
            }  
            memcpy(((G711ENC_COMPONENT_PRIVATE *)
                    pHandle->pComponentPrivate)->G711Params[G711ENC_OUTPUT_PORT], pCompG711Param, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
        }
        else if (pCompG711Param->nPortIndex == OMX_DirInput) {
            if (((G711ENC_COMPONENT_PRIVATE *)
                    pHandle->pComponentPrivate)->G711Params[G711ENC_INPUT_PORT] == NULL) {
                eError = OMX_ErrorBadParameter;
		break;
            }  
            memcpy(((G711ENC_COMPONENT_PRIVATE *)
                    pHandle->pComponentPrivate)->G711Params[G711ENC_INPUT_PORT], pCompG711Param, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
        }
        else {
            G711ENC_DPRINT("%d :: OMX_ErrorBadPortIndex from SetParameter",__LINE__);
            eError = OMX_ErrorBadPortIndex;
        }
        break;
    case OMX_IndexParamPortDefinition:
        pComponentParamPort = (OMX_PARAM_PORTDEFINITIONTYPE *)pCompParam;
        G711ENC_DPRINT("%d :: SetParameter OMX_IndexParamPortDefinition \n",__LINE__);
        if(((OMX_PARAM_PORTDEFINITIONTYPE *)(pCompParam))->nPortIndex ==
           pComponentPrivate->pPortDef[G711ENC_INPUT_PORT]->nPortIndex) {
            G711ENC_DPRINT("%d :: SetParameter OMX_IndexParamPortDefinition \n",__LINE__);
            memcpy(pComponentPrivate->pPortDef[G711ENC_INPUT_PORT], pCompParam, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
            G711ENC_DPRINT("Input Count actual %d \n",(int)pComponentPrivate->pPortDef[G711ENC_INPUT_PORT]->nBufferCountActual);
        }
        else if(((OMX_PARAM_PORTDEFINITIONTYPE *)(pCompParam))->nPortIndex ==
                pComponentPrivate->pPortDef[G711ENC_OUTPUT_PORT]->nPortIndex) {
            G711ENC_DPRINT("%d :: SetParameter OMX_IndexParamPortDefinition \n",__LINE__);
            memcpy(pComponentPrivate->pPortDef[G711ENC_OUTPUT_PORT], pCompParam, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
            G711ENC_DPRINT("Output Count actual %d \n",(int)pComponentPrivate->pPortDef[G711ENC_INPUT_PORT]->nBufferCountActual);
        }
        else {
            G711ENC_DPRINT("%d :: OMX_ErrorBadPortIndex from SetParameter",__LINE__);
            eError = OMX_ErrorBadPortIndex;
        }
        break;
    case OMX_IndexParamPriorityMgmt:
	if (pComponentPrivate->sPriorityMgmt == NULL) {
            eError = OMX_ErrorBadParameter;
	    break;
	}
        G711ENC_DPRINT("%d :: SetParameter OMX_IndexParamPriorityMgmt \n",__LINE__);
        memcpy(pComponentPrivate->sPriorityMgmt, (OMX_PRIORITYMGMTTYPE*)pCompParam, sizeof(OMX_PRIORITYMGMTTYPE));
        break;

    case OMX_IndexParamAudioInit:
	if (pComponentPrivate->sPortParam == NULL) {
            eError = OMX_ErrorBadParameter;
	    break;
	}
        G711ENC_DPRINT("%d :: SetParameter OMX_IndexParamAudioInit \n",__LINE__);
        memcpy(pComponentPrivate->sPortParam, (OMX_PORT_PARAM_TYPE*)pCompParam, sizeof(OMX_PORT_PARAM_TYPE));
        break;

    case OMX_IndexParamStandardComponentRole:
        if (pCompParam) {
            pRole = (OMX_PARAM_COMPONENTROLETYPE *)pCompParam;
            memcpy(&(pComponentPrivate->componentRole), (void *)pRole, sizeof(OMX_PARAM_COMPONENTROLETYPE));
        } else {
            eError = OMX_ErrorBadParameter;
        }
        break;

    case OMX_IndexParamCompBufferSupplier:
        /*  eError = OMX_ErrorBadPortIndex; *//*remove for StdAudioDecoderTest, leave for other tests*/
        if(((OMX_PARAM_PORTDEFINITIONTYPE *)(pCompParam))->nPortIndex ==
           pComponentPrivate->pPortDef[G711ENC_INPUT_PORT]->nPortIndex) {
            G711ENC_DPRINT(":: SetParameter OMX_IndexParamCompBufferSupplier \n");
            sBufferSupplier.eBufferSupplier = OMX_BufferSupplyInput;
            memcpy(&sBufferSupplier, pCompParam, sizeof(OMX_PARAM_BUFFERSUPPLIERTYPE));                                  
                       
        }
        else if(((OMX_PARAM_PORTDEFINITIONTYPE *)(pCompParam))->nPortIndex ==
                pComponentPrivate->pPortDef[G711ENC_OUTPUT_PORT]->nPortIndex) {
            G711ENC_DPRINT(":: SetParameter OMX_IndexParamCompBufferSupplier \n");
            sBufferSupplier.eBufferSupplier = OMX_BufferSupplyOutput;
            memcpy(&sBufferSupplier, pCompParam, sizeof(OMX_PARAM_BUFFERSUPPLIERTYPE));
        } 
        else {
            G711ENC_DPRINT(":: OMX_ErrorBadPortIndex from SetParameter");
            eError = OMX_ErrorBadPortIndex;
        }
        break;
            
    default:
        G711ENC_DPRINT("%d :: SetParameter OMX_ErrorUnsupportedIndex \n",__LINE__);
        eError = OMX_ErrorUnsupportedIndex;
        break;
                
    }
 EXIT:
    G711ENC_DPRINT("%d :: Exiting SetParameter\n",__LINE__);
    G711ENC_DPRINT("%d :: Returning = 0x%x\n",__LINE__,eError);
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
    G711ENC_DPRINT("%d :: Entering GetConfig\n", __LINE__);
    G711ENC_DPRINT("%d :: Exiting GetConfig\n", __LINE__);
    G711ENC_DPRINT("%d :: Returning = 0x%x\n",__LINE__,eError);
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
    G711ENC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    OMX_COMPONENTTYPE* pHandle = (OMX_COMPONENTTYPE*)hComp;
    TI_OMX_DSP_DEFINITION *pTiDspDefinition = NULL;
    G711ENC_FTYPES *confFrameParams = NULL;
    TI_OMX_DATAPATH dataPath;
    OMX_S16 *customFlag = NULL;
    
    G711ENC_DPRINT("%d :: Entering SetConfig\n", __LINE__);
    if (pHandle == NULL) {
        G711ENC_DPRINT ("%d :: Invalid HANDLE OMX_ErrorBadParameter \n",__LINE__);
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pComponentPrivate = (G711ENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    G711ENC_DPRINT("Index: %x \n",nConfigIndex);
    switch (nConfigIndex) {

    case OMX_IndexCustomG711ENCModeConfig: 
        pTiDspDefinition = (TI_OMX_DSP_DEFINITION*)ComponentConfigStructure;
        memcpy(&(pComponentPrivate->tiOmxDspDefinition),pTiDspDefinition,sizeof(TI_OMX_DSP_DEFINITION));
        pComponentPrivate->dasfMode = pComponentPrivate->tiOmxDspDefinition.dasfMode;
        pComponentPrivate->acdnMode = pComponentPrivate->tiOmxDspDefinition.acousticMode;
        pComponentPrivate->streamID= pTiDspDefinition->streamId;

        G711ENC_DPRINT("%d :: dasfMode : %d \n",__LINE__,(int)pComponentPrivate->dasfMode);
        G711ENC_DPRINT("%d :: acdnMode : %d \n",__LINE__,(int)pComponentPrivate->acdnMode);
        G711ENC_DPRINT("%d :: streamID : %d \n",__LINE__,(int)pComponentPrivate->streamID);

        break;
    case OMX_IndexCustomG711EncFrameParams:

        confFrameParams = (G711ENC_FTYPES*)ComponentConfigStructure;
        pComponentPrivate->frametype = confFrameParams->FrameSizeType;
        pComponentPrivate->vaumode = confFrameParams->VAUMode;
        pComponentPrivate->vauthreshold = confFrameParams->VAUThresOffset;
        pComponentPrivate->vaunumber = confFrameParams->VAUNum;
        pComponentPrivate->nmunoise = confFrameParams->NMUNoise;
        pComponentPrivate->lporder = confFrameParams->LPOrder;
 
        break;  

                    
    case  OMX_IndexCustomG711EncDataPath:
        customFlag = (OMX_S16*)ComponentConfigStructure;
        if (customFlag == NULL) {
            G711ENC_DPRINT("%d :: OMX_ErrorBadParameter from SetConfig()  \n",__LINE__);
            eError = OMX_ErrorBadParameter;
            goto EXIT;
        }
        dataPath = *customFlag;
        switch(dataPath) {
        case DATAPATH_APPLICATION:
            OMX_MMMIXER_DATAPATH(pComponentPrivate->sDeviceString, RENDERTYPE_ENCODER, pComponentPrivate->streamID);
            break;

        case DATAPATH_APPLICATION_RTMIXER:
            strcpy((char*)pComponentPrivate->sDeviceString,(char*)RTM_STRING_ENCODER);
            break;

        case DATAPATH_ACDN:
            break;
        case DATAPATH_APPLICATION_TEE:
            break;
        }
        break;

    default:
        eError = OMX_ErrorUnsupportedIndex;
        G711ENC_DPRINT("%d :: OMX_ErrorUnsupportedIndex.\n",__LINE__);
        break;
    }
 EXIT:
    G711ENC_DPRINT("%d :: Exiting SetConfig\n", __LINE__);
    G711ENC_DPRINT("%d :: Returning = 0x%x\n",__LINE__,eError);
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
    G711ENC_DPRINT("%d :: Entering GetState\n", __LINE__);
    if (!pState) {
        eError = OMX_ErrorBadParameter;
        G711ENC_DPRINT("%d :: OMX_ErrorBadParameter from GetState\n",__LINE__);
        goto EXIT;
    }

    if (pHandle && pHandle->pComponentPrivate) {
        *pState =  ((G711ENC_COMPONENT_PRIVATE*)
                    pHandle->pComponentPrivate)->curState;
    } else {
        *pState = OMX_StateLoaded;
    }
    eError = OMX_ErrorNone;
    G711ENC_DPRINT("State = %d \n", (*pState));

 EXIT:
    G711ENC_DPRINT("%d :: Exiting GetState\n", __LINE__);
    G711ENC_DPRINT("%d :: Returning = 0x%x\n",__LINE__,eError);
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
    G711ENC_COMPONENT_PRIVATE *pComponentPrivate =
        (G711ENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    pPortDef = ((G711ENC_COMPONENT_PRIVATE*)pComponentPrivate)->pPortDef[G711ENC_INPUT_PORT];

    G711ENC_DPRINT("%d :: Entering EmptyThisBuffer\n", __LINE__);

    if (pBuffer == NULL) {
        eError = OMX_ErrorBadParameter;
        G711ENC_PRINT("%d :: About to return OMX_ErrorBadParameter\n",__LINE__);
        goto EXIT;
    }

    if (pBuffer->nSize != sizeof(OMX_BUFFERHEADERTYPE)) {
        eError = OMX_ErrorBadParameter;
        G711ENC_PRINT("%d :: About to return OMX_ErrorBadParameter\n",__LINE__);
        goto EXIT;
    }

    if (!pPortDef->bEnabled) {
        eError  = OMX_ErrorIncorrectStateOperation;
        G711ENC_PRINT("%d :: About to return OMX_ErrorIncorrectStateOperation\n",__LINE__);
        goto EXIT; 
    }

    if (pBuffer->nVersion.nVersion != pComponentPrivate->nVersion) {
        eError = OMX_ErrorVersionMismatch;
        G711ENC_PRINT("%d :: About to return OMX_ErrorVersionMismatch\n",__LINE__);
        goto EXIT;
    }

    if (pBuffer->nInputPortIndex != G711ENC_INPUT_PORT) {
        eError  = OMX_ErrorBadPortIndex;
        G711ENC_PRINT("%d :: About to return OMX_ErrorBadPortIndex\n",__LINE__);
        goto EXIT;
    }

    if (pComponentPrivate->curState != OMX_StateExecuting && 
        pComponentPrivate->curState != OMX_StatePause) {
        eError= OMX_ErrorIncorrectStateOperation;
        G711ENC_PRINT("%d :: current state: %d \n ",__LINE__,pComponentPrivate->curState);
        G711ENC_PRINT("%d :: About to return OMX_ErrorIncorrectStateOperation\n",__LINE__);
        goto EXIT;
    }


    G711ENC_DPRINT("----------------------------------------------------------------\n");
    G711ENC_DPRINT("%d :: Comp Sending Filled ip buff = %p to CompThread\n",__LINE__,pBuffer);
    G711ENC_DPRINT("----------------------------------------------------------------\n");

    pComponentPrivate->app_nBuf--;
    pComponentPrivate->pMarkData = pBuffer->pMarkData;
    pComponentPrivate->hMarkTargetComponent = pBuffer->hMarkTargetComponent;

    pComponentPrivate->nUnhandledEmptyThisBuffers++;

    ret = write (pComponentPrivate->dataPipe[1], &pBuffer, sizeof(OMX_BUFFERHEADERTYPE*));
    if (ret == -1) {
        G711ENC_PRINT("%d :: Error in Writing to the Data pipe\n", __LINE__);
        eError = OMX_ErrorHardware;
        goto EXIT;
    }
    pComponentPrivate->nEmptyThisBufferCount++;
 EXIT:
    G711ENC_DPRINT("%d :: Exiting EmptyThisBuffer\n", __LINE__);
    G711ENC_DPRINT("%d :: Returning = 0x%x\n",__LINE__,eError);
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
    G711ENC_COMPONENT_PRIVATE *pComponentPrivate =
        (G711ENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    pPortDef = ((G711ENC_COMPONENT_PRIVATE*)pComponentPrivate)->pPortDef[G711ENC_OUTPUT_PORT];
    G711ENC_DPRINT("%d :: Entering FillThisBuffer\n", __LINE__);
    G711ENC_DPRINT("------------------------------------------------------------------\n");
    G711ENC_DPRINT("%d :: Comp Sending Emptied op buff = %p to CompThread\n",__LINE__,pBuffer);
    G711ENC_DPRINT("------------------------------------------------------------------\n");
    if (pBuffer == NULL) {
        eError = OMX_ErrorBadParameter;
        G711ENC_DPRINT(" %d :: About to return OMX_ErrorBadParameter\n",__LINE__);
        goto EXIT;
    }

    if (pBuffer->nSize != sizeof(OMX_BUFFERHEADERTYPE)) {
        eError = OMX_ErrorBadParameter;
        G711ENC_DPRINT(" %d :: About to return OMX_ErrorBadParameter\n",__LINE__);
        goto EXIT;
    }

    if (!pPortDef->bEnabled) {
        eError  = OMX_ErrorIncorrectStateOperation;
        G711ENC_DPRINT("%d :: About to return OMX_ErrorIncorrectStateOperation\n",__LINE__);
        goto EXIT;
    }

    if (pBuffer->nVersion.nVersion != pComponentPrivate->nVersion) {
        eError = OMX_ErrorVersionMismatch;
        G711ENC_DPRINT(" %d :: About to return OMX_ErrorVersionMismatch\n",__LINE__);
        goto EXIT;
    }

    if (pBuffer->nOutputPortIndex != G711ENC_OUTPUT_PORT) {
        eError  = OMX_ErrorBadPortIndex;
        G711ENC_DPRINT(" %d :: About to return OMX_ErrorBadPortIndex\n",__LINE__);
        goto EXIT;
    }

    if(pComponentPrivate->curState != OMX_StateExecuting && 
       pComponentPrivate->curState != OMX_StatePause) {
        eError = OMX_ErrorIncorrectStateOperation;
        G711ENC_DPRINT("%d :: About to return OMX_ErrorIncorrectStateOperation\n",__LINE__);
        goto EXIT;
    }

    /*pBuffer->nFilledLen = 0;
      memset(pBuffer->pBuffer, 0, pBuffer->nAllocLen);*/

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

    pComponentPrivate->nUnhandledFillThisBuffers++;

    ret = write (pComponentPrivate->dataPipe[1], &pBuffer, sizeof (OMX_BUFFERHEADERTYPE*));
    if (ret == -1) {
        G711ENC_DPRINT("%d :: Error in Writing to the Data pipe\n", __LINE__);
        eError = OMX_ErrorHardware;
        goto EXIT;
    }
    pComponentPrivate->nFillThisBufferCount++;
 EXIT:
    G711ENC_DPRINT("%d :: Exiting FillThisBuffer\n", __LINE__);
    G711ENC_DPRINT("%d :: Returning = 0x%x\n",__LINE__,eError);
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
    G711ENC_COMPONENT_PRIVATE *pComponentPrivate =
        (G711ENC_COMPONENT_PRIVATE *)pComponent->pComponentPrivate;
    G711ENC_DPRINT("%d :: Entering ComponentDeInit\n", __LINE__);

#ifdef DSP_RENDERING_ON
    close(pComponentPrivate->fdwrite);
    close(pComponentPrivate->fdread);
#endif

#ifdef RESOURCE_MANAGER_ENABLED
    eError = RMProxy_NewSendCommand(pHandle, RMProxy_FreeResource, 
                                    OMX_G711_Encoder_COMPONENT, 
                                    0, 1234, NULL);
    if (eError != OMX_ErrorNone) {
        G711ENC_DPRINT ("%d ::Error returned from destroy ResourceManagerProxy thread\n",
                        __LINE__);
    }
    eError = RMProxy_Deinitalize();
    if (eError != OMX_ErrorNone) {
        G711ENC_DPRINT("%d :: Error from RMProxy_Deinitalize\n",__LINE__);
        goto EXIT;
    }
#endif

    pComponentPrivate->bIsThreadstop = 1;

    eError = G711ENC_StopComponentThread(pHandle);
    if (eError != OMX_ErrorNone) {
        G711ENC_DPRINT("%d :: Error from G711ENC_StopComponentThread\n",__LINE__);
        goto EXIT;
    }
    /* Wait for thread to exit so we can get the status into "eError" */
    /* close the pipe handles */
    eError = G711ENC_FreeCompResources(pHandle);
    if (eError != OMX_ErrorNone) {
        G711ENC_DPRINT("%d :: Error from G711ENC_FreeCompResources\n",__LINE__);
        goto EXIT;
    }

    OMX_G711ENC_MEMFREE_STRUCT(pComponentPrivate->sDeviceString);
    OMX_G711ENC_MEMFREE_STRUCT(pComponentPrivate);

 EXIT:
    G711ENC_DPRINT("%d :: Exiting ComponentDeInit\n", __LINE__);
    G711ENC_DPRINT("%d :: Returning = 0x%x\n",__LINE__,eError);
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
    G711ENC_DPRINT("%d :: Entering ComponentTunnelRequest\n", __LINE__);
    G711ENC_DPRINT("%d :: Exiting ComponentTunnelRequest\n", __LINE__);
    G711ENC_DPRINT("%d :: Returning = 0x%x\n",__LINE__,eError);
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
    G711ENC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_BUFFERHEADERTYPE *pBufferHeader = NULL;

    pComponentPrivate = (G711ENC_COMPONENT_PRIVATE *)
        (((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);

    pPortDef = ((G711ENC_COMPONENT_PRIVATE*)pComponentPrivate)->pPortDef[nPortIndex];
    G711ENC_DPRINT("%d :: Entering AllocateBuffer\n", __LINE__);
    G711ENC_DPRINT("%d :: pPortDef = %p\n", __LINE__,pPortDef);

    if(!pPortDef->bEnabled){
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

    G711ENC_DPRINT("%d :: pPortDef->bEnabled = %d\n", __LINE__,pPortDef->bEnabled);

    G711ENC_OMX_MALLOC_STRUCT(pBufferHeader, OMX_BUFFERHEADERTYPE);

    G711ENC_OMX_MALLOC_SIZE(pBufferHeader->pBuffer,
                            (nSizeBytes + DSP_CACHE_ALIGNMENT),
                            OMX_U8);

    pBufferHeader->pBuffer += EXTRA_BYTES;

    if (nPortIndex == G711ENC_INPUT_PORT) {
        pBufferHeader->nInputPortIndex = nPortIndex;
        pBufferHeader->nOutputPortIndex = -1;
        pComponentPrivate->pInputBufferList->pBufHdr[pComponentPrivate->pInputBufferList->numBuffers] = pBufferHeader;
        pComponentPrivate->pInputBufferList->bBufferPending[pComponentPrivate->pInputBufferList->numBuffers] = 0;
        pComponentPrivate->pInputBufferList->bufferOwner[pComponentPrivate->pInputBufferList->numBuffers++] = 1;
        G711ENC_DPRINT("%d ::Comparing INPUT numBuffers : pPortDef->nBufferCountActual  = %d : %d \n",__LINE__,
                       (int)pComponentPrivate->pInputBufferList->numBuffers,(int)pPortDef->nBufferCountActual);
        if (pComponentPrivate->pInputBufferList->numBuffers == pPortDef->nBufferCountActual) {
            pPortDef->bPopulated = OMX_TRUE;
            G711ENC_DPRINT("%d :: pPortDef->bPopulated = %d\n", __LINE__, pPortDef->bPopulated);
        }
    }
    else if (nPortIndex == G711ENC_OUTPUT_PORT) {
        pBufferHeader->nInputPortIndex = -1;
        pBufferHeader->nOutputPortIndex = nPortIndex;
        pComponentPrivate->pOutputBufferList->pBufHdr[pComponentPrivate->pOutputBufferList->numBuffers] = pBufferHeader;
        pComponentPrivate->pOutputBufferList->bBufferPending[pComponentPrivate->pOutputBufferList->numBuffers] = 0;
        pComponentPrivate->pOutputBufferList->bufferOwner[pComponentPrivate->pOutputBufferList->numBuffers++] = 1;
        G711ENC_DPRINT("%d :: Comparing OUTPUT numBuffers : pPortDef->nBufferCountActual  = %d : %d \n",__LINE__,
                       (int)pComponentPrivate->pOutputBufferList->numBuffers,(int)pPortDef->nBufferCountActual);
        if (pComponentPrivate->pOutputBufferList->numBuffers == pPortDef->nBufferCountActual) {
            pPortDef->bPopulated = OMX_TRUE;
            G711ENC_DPRINT("%d :: pPortDef->bPopulated = %d\n", __LINE__, pPortDef->bPopulated);
        }
    }
    else {
        eError = OMX_ErrorBadPortIndex;
        G711ENC_DPRINT(" %d :: About to return OMX_ErrorBadPortIndex\n",__LINE__);
        goto EXIT;
    }

    if((pComponentPrivate->pPortDef[G711ENC_OUTPUT_PORT]->bPopulated == 
        pComponentPrivate->pPortDef[G711ENC_OUTPUT_PORT]->bEnabled) &&
       (pComponentPrivate->pPortDef[G711ENC_INPUT_PORT]->bPopulated == 
        pComponentPrivate->pPortDef[G711ENC_INPUT_PORT]->bEnabled) &&
       (pComponentPrivate->InLoaded_readytoidle)){
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
    pBufferHeader->nVersion.s.nVersionMajor = G711ENC_MAJOR_VER;
    pBufferHeader->nVersion.s.nVersionMinor = G711ENC_MINOR_VER;
    pComponentPrivate->nVersion = pBufferHeader->nVersion.nVersion;
    pBufferHeader->nSize = sizeof(OMX_BUFFERHEADERTYPE);
    *pBuffer = pBufferHeader;

    if (pComponentPrivate->bEnableCommandPending && pPortDef->bPopulated) {
        SendCommand (pComponentPrivate->pHandle,
                     OMX_CommandPortEnable,
                     pComponentPrivate->nEnableCommandParam,
                     NULL);
    }

    
 EXIT:
    G711ENC_DPRINT("%d :: Exiting AllocateBuffer\n",__LINE__);
    G711ENC_DPRINT("%d :: Returning = 0x%x\n",__LINE__,eError);
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

static OMX_ERRORTYPE FreeBuffer(OMX_IN  OMX_HANDLETYPE hComponent,
                                OMX_IN  OMX_U32 nPortIndex,
                                OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    G711ENC_COMPONENT_PRIVATE * pComponentPrivate = NULL;
    OMX_BUFFERHEADERTYPE* buff = NULL;
    OMX_U8* tempBuff = NULL;
    int i = 0;
    int inputIndex = -1;
    int outputIndex = -1;
    OMX_COMPONENTTYPE *pHandle = NULL;

    pComponentPrivate = (G711ENC_COMPONENT_PRIVATE *)
        (((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);

    pHandle = (OMX_COMPONENTTYPE *) pComponentPrivate->pHandle;

    G711ENC_DPRINT("%d :: Entering FreeBuffer\n", __LINE__);
    for (i=0; i < G711ENC_MAX_NUM_OF_BUFS; i++) {
        buff = pComponentPrivate->pInputBufferList->pBufHdr[i];
        if (buff == pBuffer) {
            G711ENC_DPRINT("%d :: Found matching input buffer\n",__LINE__);
            G711ENC_DPRINT("%d :: buff = %p\n",__LINE__,buff);
            G711ENC_DPRINT("%d :: pBuffer = %p\n",__LINE__,pBuffer);
            inputIndex = i;
            break;
        }
        else {
            G711ENC_DPRINT("%d :: This is not a match\n",__LINE__);
            G711ENC_DPRINT("%d :: buff = %p\n",__LINE__,buff);
            G711ENC_DPRINT("%d :: pBuffer = %p\n",__LINE__,pBuffer);
        }
    }

    for (i=0; i < G711ENC_MAX_NUM_OF_BUFS; i++) {
        buff = pComponentPrivate->pOutputBufferList->pBufHdr[i];
        if (buff == pBuffer) {
            G711ENC_DPRINT("%d :: Found matching output buffer\n",__LINE__);
            G711ENC_DPRINT("%d :: buff = %p\n",__LINE__,buff);
            G711ENC_DPRINT("%d :: pBuffer = %p\n",__LINE__,pBuffer);
            outputIndex = i;
            break;
        }
        else {
            G711ENC_DPRINT("%d :: This is not a match\n",__LINE__);
            G711ENC_DPRINT("%d :: buff = %p\n",__LINE__,buff);
            G711ENC_DPRINT("%d :: pBuffer = %p\n",__LINE__,pBuffer);
        }
    }

    if (inputIndex != -1) {
        if (pComponentPrivate->pInputBufferList->bufferOwner[inputIndex] == 1) {
            tempBuff = pComponentPrivate->pInputBufferList->pBufHdr[inputIndex]->pBuffer;
            if (tempBuff != 0){
                tempBuff -= EXTRA_BYTES;
            }
            OMX_G711ENC_MEMFREE_STRUCT(tempBuff);
        }

        OMX_G711ENC_MEMFREE_STRUCT(pComponentPrivate->pInputBufferList->pBufHdr[inputIndex]);

        pComponentPrivate->pInputBufferList->numBuffers--;
        
        G711ENC_DPRINT("%d :: pComponentPrivate->pInputBufferList->numBuffers = %ld \n",__LINE__,pComponentPrivate->pInputBufferList->numBuffers);
        G711ENC_DPRINT("%d :: pComponentPrivate->pPortDef[G711ENC_INPUT_PORT]->nBufferCountMin = %ld \n",__LINE__,pComponentPrivate->pPortDef[G711ENC_INPUT_PORT]->nBufferCountMin);
        if (pComponentPrivate->pInputBufferList->numBuffers <
            pComponentPrivate->pPortDef[G711ENC_INPUT_PORT]->nBufferCountMin) {

            pComponentPrivate->pPortDef[G711ENC_INPUT_PORT]->bPopulated = OMX_FALSE;
        }
        if(pComponentPrivate->pPortDef[G711ENC_INPUT_PORT]->bEnabled &&
           pComponentPrivate->bLoadedCommandPending == OMX_FALSE &&
           (pComponentPrivate->curState == OMX_StateIdle ||
            pComponentPrivate->curState == OMX_StateExecuting ||
            pComponentPrivate->curState == OMX_StatePause)) {
            G711ENC_DPRINT("%d :: pPortDef[G711ENC_INPUT_PORT]->bPopulated = %d \n",
                           __LINE__,
                           pComponentPrivate->pPortDef[G711ENC_INPUT_PORT]->bPopulated);
            G711ENC_DPRINT("%d :: pComponentPrivate->curState = %d \n",
                           __LINE__,
                           pComponentPrivate->curState);
            pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                    pHandle->pApplicationPrivate,
                                                    OMX_EventError,
                                                    OMX_ErrorPortUnpopulated,
                                                    nPortIndex,
                                                    NULL);
        }
    }
    else if (outputIndex != -1) {
        if (pComponentPrivate->pOutputBufferList->bufferOwner[outputIndex] == 1) {
            tempBuff = pComponentPrivate->pOutputBufferList->pBufHdr[outputIndex]->pBuffer;
            if (tempBuff != 0){
                tempBuff -= EXTRA_BYTES;
            }
            OMX_G711ENC_MEMFREE_STRUCT(tempBuff);
        }
        OMX_G711ENC_MEMFREE_STRUCT(pComponentPrivate->pOutputBufferList->pBufHdr[outputIndex]);

        pComponentPrivate->pOutputBufferList->numBuffers--;
        G711ENC_DPRINT("\n%d :: pComponentPrivate->pOutputBufferList->numBuffers = %ld \n",
                       __LINE__,
                       pComponentPrivate->pOutputBufferList->numBuffers);
        G711ENC_DPRINT("\n%d :: pComponentPrivate->pPortDef[OUTPUT_PORT]->nBufferCountMin = %ld \n",
                       __LINE__,
                       pComponentPrivate->pPortDef[G711ENC_OUTPUT_PORT]->nBufferCountMin);

        if (pComponentPrivate->pOutputBufferList->numBuffers <
            pComponentPrivate->pPortDef[G711ENC_OUTPUT_PORT]->nBufferCountMin) {
            pComponentPrivate->pPortDef[G711ENC_OUTPUT_PORT]->bPopulated = OMX_FALSE;
        }
        if(pComponentPrivate->pPortDef[G711ENC_OUTPUT_PORT]->bEnabled &&
           pComponentPrivate->bLoadedCommandPending == OMX_FALSE &&
           (pComponentPrivate->curState == OMX_StateIdle ||
            pComponentPrivate->curState == OMX_StateExecuting ||
            pComponentPrivate->curState == OMX_StatePause)) {
            G711ENC_DPRINT("\n%d :: pPortDef[G711ENC_OUTPUT_PORT]->bPopulated = %d \n",
                           __LINE__,
                           pComponentPrivate->pPortDef[G711ENC_OUTPUT_PORT]->bPopulated);
            G711ENC_DPRINT("\n%d :: pComponentPrivate->curState = %d \n",
                           __LINE__,
                           pComponentPrivate->curState);
            pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                    pHandle->pApplicationPrivate,
                                                    OMX_EventError,
                                                    OMX_ErrorPortUnpopulated,
                                                    nPortIndex,
                                                    NULL);
        }
    }
    else {
        G711ENC_DPRINT("%d :: Returning OMX_ErrorBadParameter\n",__LINE__);
        eError = OMX_ErrorBadParameter;
    }
    if ((!pComponentPrivate->pInputBufferList->numBuffers &&
         !pComponentPrivate->pOutputBufferList->numBuffers) &&
        pComponentPrivate->InIdle_goingtoloaded){
        pComponentPrivate->InIdle_goingtoloaded = 0;                  
#ifndef UNDER_CE
        pthread_mutex_lock(&pComponentPrivate->InIdle_mutex);
        pthread_cond_signal(&pComponentPrivate->InIdle_threshold);
        pthread_mutex_unlock(&pComponentPrivate->InIdle_mutex);
#else
        OMX_SignalEvent(&(pComponentPrivate->InIdle_event));
#endif
    }

    if (pComponentPrivate->bDisableCommandPending && 
        (pComponentPrivate->pInputBufferList->numBuffers + 
         pComponentPrivate->pOutputBufferList->numBuffers == 0)) {

        SendCommand (pComponentPrivate->pHandle,
                     OMX_CommandPortDisable,
                     pComponentPrivate->bDisableCommandParam,
                     NULL);
    }
    G711ENC_DPRINT("%d :: Exiting FreeBuffer\n", __LINE__);
    G711ENC_DPRINT("%d :: Returning = 0x%x\n",__LINE__,eError);
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
static OMX_ERRORTYPE UseBuffer (OMX_IN OMX_HANDLETYPE hComponent,
                                OMX_INOUT OMX_BUFFERHEADERTYPE** ppBufferHdr,
                                OMX_IN OMX_U32 nPortIndex,
                                OMX_IN OMX_PTR pAppPrivate,
                                OMX_IN OMX_U32 nSizeBytes,
                                OMX_IN OMX_U8* pBuffer)
{
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef = NULL;
    G711ENC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_BUFFERHEADERTYPE *pBufferHeader = NULL;

    pComponentPrivate = (G711ENC_COMPONENT_PRIVATE *)
        (((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);

    pPortDef = ((G711ENC_COMPONENT_PRIVATE*)pComponentPrivate)->pPortDef[nPortIndex];
    G711ENC_DPRINT("%d :: Entering UseBuffer\n", __LINE__);
    G711ENC_DPRINT("%d :: pPortDef->bPopulated = %d \n",__LINE__,pPortDef->bPopulated);

    if(!pPortDef->bEnabled) {
        G711ENC_DPRINT("%d :: About to return OMX_ErrorIncorrectStateOperation\n",__LINE__);
        eError = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }

    if(nSizeBytes != pPortDef->nBufferSize || pPortDef->bPopulated) {
        G711ENC_DPRINT("%d :: About to return OMX_ErrorBadParameter\n",__LINE__);
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }

    G711ENC_OMX_MALLOC_STRUCT(pBufferHeader, OMX_BUFFERHEADERTYPE);

    if (nPortIndex == G711ENC_OUTPUT_PORT) {
        pBufferHeader->nInputPortIndex = -1;
        pBufferHeader->nOutputPortIndex = nPortIndex;
        pComponentPrivate->pOutputBufferList->pBufHdr[pComponentPrivate->pOutputBufferList->numBuffers] = pBufferHeader;
        pComponentPrivate->pOutputBufferList->bBufferPending[pComponentPrivate->pOutputBufferList->numBuffers] = 0;
        pComponentPrivate->pOutputBufferList->bufferOwner[pComponentPrivate->pOutputBufferList->numBuffers++] = 0;

        if (pComponentPrivate->pOutputBufferList->numBuffers == pPortDef->nBufferCountActual) {
            pPortDef->bPopulated = OMX_TRUE;
        }
        G711ENC_DPRINT("%d :: pPortDef->bPopulated = %d\n", __LINE__, pPortDef->bPopulated);
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
        G711ENC_DPRINT("%d :: pPortDef->bPopulated = %d\n", __LINE__, pPortDef->bPopulated);
    }

    if((pComponentPrivate->pPortDef[G711ENC_OUTPUT_PORT]->bPopulated == 
        pComponentPrivate->pPortDef[G711ENC_OUTPUT_PORT]->bEnabled)&&
       (pComponentPrivate->pPortDef[G711ENC_INPUT_PORT]->bPopulated == 
        pComponentPrivate->pPortDef[G711ENC_INPUT_PORT]->bEnabled) &&
       (pComponentPrivate->InLoaded_readytoidle)){
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
    pBufferHeader->nVersion.s.nVersionMajor = G711ENC_MAJOR_VER;
    pBufferHeader->nVersion.s.nVersionMinor = G711ENC_MINOR_VER;
    pComponentPrivate->nVersion = pBufferHeader->nVersion.nVersion;
    pBufferHeader->pBuffer = pBuffer;
    pBufferHeader->nSize = sizeof(OMX_BUFFERHEADERTYPE);
    *ppBufferHdr = pBufferHeader;
 EXIT:
    G711ENC_DPRINT("%d :: Exiting UseBuffer\n", __LINE__);
    G711ENC_DPRINT("%d :: Returning = 0x%x\n",__LINE__,eError);
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
static OMX_ERRORTYPE GetExtensionIndex( OMX_IN  OMX_HANDLETYPE hComponent,
                                        OMX_IN  OMX_STRING cParameterName,
                                        OMX_OUT OMX_INDEXTYPE* pIndexType) 
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    G711ENC_DPRINT("GetExtensionIndex\n");
    if (!(strcmp(cParameterName,"OMX.TI.index.config.tispecific"))) {
        *pIndexType = OMX_IndexCustomG711ENCModeConfig;
        G711ENC_DPRINT("OMX_IndexCustomG711ENCModeConfig\n");
    }
    else if(!(strcmp(cParameterName,"OMX.TI.index.config.g711.datapath"))) {
        *pIndexType = OMX_IndexCustomG711EncDataPath;
        G711ENC_DPRINT("OMX_IndexCustomG711EncDataPath\n");
    }
    
    else if(!strcmp(cParameterName,"OMX.TI.index.config.g711.frameparamters")){
        *pIndexType = OMX_IndexCustomG711EncFrameParams;
    } 
    
    else {
        eError = OMX_ErrorBadParameter;
    }

    G711ENC_DPRINT("pIndexType %x \n",*pIndexType);
    G711ENC_DPRINT("Exiting GetExtensionIndex\n");
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
static OMX_ERRORTYPE ComponentRoleEnum( OMX_IN OMX_HANDLETYPE hComponent,
                                        OMX_OUT OMX_U8 *cRole,
                                        OMX_IN OMX_U32 nIndex)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    eError = OMX_ErrorNotImplemented;

    return eError;
}
