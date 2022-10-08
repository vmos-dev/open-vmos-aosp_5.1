
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
 * @file OMX_G726Decoder.c
 *
 * This file implements OpenMAX (TM) 1.0 Specific APIs and its functionality
 * that is fully compliant with the Khronos OpenMAX (TM) 1.0 Specification
 *
 * @path  $(CSLPATH)\OMAPSW_MPU\linux\audio\src\openmax_il\g726_dec\src
 *
 * @rev  1.0
 */
/* ----------------------------------------------------------------------------
 *!
 *! Revision History
 *! ===================================
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
#endif

#include <pthread.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <dbapi.h>

/*------- Program Header Files -----------------------------------------------*/

#include "LCML_DspCodec.h"
#include "OMX_G726Dec_Utils.h"
#include <TIDspOmx.h>

#ifdef DSP_RENDERING_ON
#include <AudioManagerAPI.h>
#endif

#ifdef RESOURCE_MANAGER_ENABLED
#include <ResourceManagerProxyAPI.h>
#endif

#ifdef DSP_RENDERING_ON

#define FIFO1 "/dev/fifo.1"
#define FIFO2 "/dev/fifo.2"
#define PERMS 0666

AM_COMMANDDATATYPE cmd_data;
int G726d_fdwrite = 0, G726d_fdread = 0;
int errno;
#endif

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

/* ================================================================================= * */
/**
 * @fn OMX_ComponentInit() function is called by OMX Core to initialize the component
 * with default values of the component. Before calling this function OMX_Init
 * must have been called.
 *
 * @param *hComp This is component handle allocated by the OMX core.
 *
 * @pre          OMX_Init should be called by application.
 *
 * @post         Component has initialzed with default values.
 *
 *  @return      OMX_ErrorNone = Successful Inirialization of the component\n
 *               OMX_ErrorInsufficientResources = Not enough memory
 *
 *  @see          G726Dec_StartCompThread()
 */
/* ================================================================================ * */
OMX_ERRORTYPE OMX_ComponentInit (OMX_HANDLETYPE hComp)
{

    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE*) hComp;
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef_ip = NULL, *pPortDef_op = NULL;
    OMX_AUDIO_PARAM_PORTFORMATTYPE *pPortFormat = NULL;
    OMX_AUDIO_PARAM_PCMMODETYPE  *G726_op = NULL;
    OMX_AUDIO_PARAM_G726TYPE *G726_ip = NULL;
    G726DEC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    G726D_AUDIODEC_PORT_TYPE *pCompPort = NULL;
    G726D_BUFFERLIST *pTemp = NULL;
    int i=0;

    G726DEC_DPRINT ("Entering OMX_ComponentInit\n");

    G726D_OMX_CONF_CHECK_CMD(pHandle,1,1)

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
    pHandle->AllocateBuffer =  AllocateBuffer;
    pHandle->FreeBuffer = FreeBuffer;
    pHandle->UseBuffer = UseBuffer;
    pHandle->GetExtensionIndex = GetExtensionIndex;
    pHandle->ComponentRoleEnum = ComponentRoleEnum;

    G726D_OMX_MALLOC(pHandle->pComponentPrivate,G726DEC_COMPONENT_PRIVATE);

    pComponentPrivate = pHandle->pComponentPrivate;
    pComponentPrivate->pHandle = pHandle;

    G726D_OMX_MALLOC(pCompPort, G726D_AUDIODEC_PORT_TYPE);
    pComponentPrivate->pCompPort[G726D_INPUT_PORT] =  pCompPort;

    G726D_OMX_MALLOC(pCompPort, G726D_AUDIODEC_PORT_TYPE);
    pComponentPrivate->pCompPort[G726D_OUTPUT_PORT] = pCompPort;
    G726D_OMX_MALLOC(pTemp, G726D_BUFFERLIST);
    pComponentPrivate->pInputBufferList = pTemp;

    G726D_OMX_MALLOC(pTemp, G726D_BUFFERLIST);
    pComponentPrivate->pOutputBufferList = pTemp;

    pComponentPrivate->pInputBufferList->numBuffers = 0;
    pComponentPrivate->pOutputBufferList->numBuffers = 0;

    for (i=0; i < MAX_NUM_OF_BUFS; i++) {
        pComponentPrivate->pInputBufferList->pBufHdr[i] = NULL;
        pComponentPrivate->pOutputBufferList->pBufHdr[i] = NULL;
        pComponentPrivate->arrTickCount[i] = 0;
        pComponentPrivate->arrTimestamp[i] = 0;
    }

    pComponentPrivate->bufAlloced = 0;

    G726D_OMX_MALLOC(pComponentPrivate->sPortParam, OMX_PORT_PARAM_TYPE);
    OMX_CONF_INIT_STRUCT(pComponentPrivate->sPortParam, OMX_PORT_PARAM_TYPE);
    G726D_OMX_MALLOC(pComponentPrivate->pPriorityMgmt, OMX_PRIORITYMGMTTYPE);
    OMX_CONF_INIT_STRUCT(pComponentPrivate->pPriorityMgmt, OMX_PRIORITYMGMTTYPE);
    pComponentPrivate->sPortParam->nPorts = 0x2;
    pComponentPrivate->sPortParam->nStartPortNumber = 0x0;
    pComponentPrivate->G726Params = NULL;
    pComponentPrivate->PcmParams = NULL;

    G726D_OMX_MALLOC(G726_ip, OMX_AUDIO_PARAM_G726TYPE);
    G726D_OMX_MALLOC(G726_op, OMX_AUDIO_PARAM_PCMMODETYPE);

    pComponentPrivate->G726Params = G726_ip;
    pComponentPrivate->PcmParams = G726_op;
    pComponentPrivate->bIsEOFSent = 0;
    pComponentPrivate->dasfmode = 0;
    pComponentPrivate->packingType = 0;
    pComponentPrivate->bCompThreadStarted = 0;
    pComponentPrivate->bExitCompThrd = 0;

    pComponentPrivate->bInitParamsInitialized = 0;
    pComponentPrivate->pMarkBuf = NULL;
    pComponentPrivate->pMarkData = NULL;
    pComponentPrivate->nEmptyBufferDoneCount = 0;
    pComponentPrivate->nEmptyThisBufferCount = 0;
    pComponentPrivate->nFillBufferDoneCount = 0;
    pComponentPrivate->nFillThisBufferCount = 0;
    pComponentPrivate->strmAttr = NULL;
    pComponentPrivate->bDisableCommandParam = 0;
    pComponentPrivate->IpBufindex = 0;
    pComponentPrivate->OpBufindex = 0;
    pComponentPrivate->nUnhandledFillThisBuffers=0;
    pComponentPrivate->nUnhandledEmptyThisBuffers = 0;
    pComponentPrivate->bFlushOutputPortCommandPending = OMX_FALSE;
    pComponentPrivate->bFlushInputPortCommandPending = OMX_FALSE;
    pComponentPrivate->bPreempted = OMX_FALSE;

    /* Initialize device string to the default value */
    G726D_OMX_MALLOC_SIZE(pComponentPrivate->sDeviceString,(100*sizeof(OMX_STRING)),OMX_STRING);
    strcpy((char*)pComponentPrivate->sDeviceString,"/eteedn:i0:o0/codec\0");
    

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

    for (i=0; i < MAX_NUM_OF_BUFS; i++) {
        pComponentPrivate->pInputBufHdrPending[i] = NULL;
        pComponentPrivate->pOutputBufHdrPending[i] = NULL;
    }

    for (i=0; i < MAX_NUM_OF_BUFS; i++) {
        pComponentPrivate->pOutBufHdrWhilePaused[i] = NULL;
    }
    pComponentPrivate->nPendingOutPausedBufs = 0;

    pComponentPrivate->nInvalidFrameCount = 0;
    pComponentPrivate->bDisableCommandPending = 0;

    pComponentPrivate->numPendingBuffers = 0;
    pComponentPrivate->bNoIdleOnStop= OMX_FALSE;
    pComponentPrivate->bDspStoppedWhileExecuting = OMX_FALSE;
    pComponentPrivate->bIdleCommandPending = OMX_FALSE;
    pComponentPrivate->nOutStandingFillDones = 0;
    G726D_OMX_MALLOC(pPortDef_ip, OMX_PARAM_PORTDEFINITIONTYPE);
    G726D_OMX_MALLOC(pPortDef_op, OMX_PARAM_PORTDEFINITIONTYPE);

    pComponentPrivate->pPortDef[G726D_INPUT_PORT] = pPortDef_ip;
    pComponentPrivate->pPortDef[G726D_OUTPUT_PORT] = pPortDef_op;

    /* Set input port defaults */
    pPortDef_ip->nSize                                  = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
    pPortDef_ip->nPortIndex                         = G726D_INPUT_PORT;
    pPortDef_ip->eDir                               = OMX_DirInput;
    pPortDef_ip->nBufferCountActual                 = G726D_NUM_INPUT_BUFFERS;
    pPortDef_ip->nBufferCountMin                    = G726D_NUM_INPUT_BUFFERS;
    pPortDef_ip->nBufferSize                        = G726D_INPUT_BUFFER_SIZE;
    pPortDef_ip->bEnabled                           = OMX_TRUE;
    pPortDef_ip->bPopulated                         = OMX_FALSE;
    pPortDef_ip->eDomain                            = OMX_PortDomainAudio;
    pPortDef_ip->format.audio.eEncoding             = OMX_AUDIO_CodingG726;
    pPortDef_ip->format.audio.cMIMEType             = NULL;
    pPortDef_ip->format.audio.pNativeRender         = NULL;
    pPortDef_ip->format.audio.bFlagErrorConcealment = OMX_FALSE;

    /* Set input port defaults */
    pPortDef_op->nSize                                  = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
    pPortDef_op->nPortIndex                         = G726D_OUTPUT_PORT;
    pPortDef_op->eDir                               = OMX_DirOutput;
    pPortDef_op->nBufferCountMin                    = G726D_NUM_OUTPUT_BUFFERS;
    pPortDef_op->nBufferCountActual                 = G726D_NUM_OUTPUT_BUFFERS;
    pPortDef_op->nBufferSize                        = G726D_OUTPUT_BUFFER_SIZE;
    pPortDef_op->bEnabled                           = OMX_TRUE;
    pPortDef_op->bPopulated                         = OMX_FALSE;
    pPortDef_op->eDomain                            = OMX_PortDomainAudio;
    pPortDef_op->format.audio.eEncoding             = OMX_AUDIO_CodingPCM;
    pPortDef_op->format.audio.cMIMEType             = NULL;
    pPortDef_op->format.audio.pNativeRender         = NULL;
    pPortDef_op->format.audio.bFlagErrorConcealment = OMX_FALSE;

    G726D_OMX_MALLOC(pComponentPrivate->pCompPort[G726D_INPUT_PORT]->pPortFormat, 
                     OMX_AUDIO_PARAM_PORTFORMATTYPE);
    G726D_OMX_MALLOC(pComponentPrivate->pCompPort[G726D_OUTPUT_PORT]->pPortFormat, 
                     OMX_AUDIO_PARAM_PORTFORMATTYPE);
    OMX_CONF_INIT_STRUCT(pComponentPrivate->pCompPort[G726D_INPUT_PORT]->pPortFormat, 
                         OMX_AUDIO_PARAM_PORTFORMATTYPE);
    OMX_CONF_INIT_STRUCT(pComponentPrivate->pCompPort[G726D_OUTPUT_PORT]->pPortFormat, 
                         OMX_AUDIO_PARAM_PORTFORMATTYPE);


    /* Set input port format defaults */
    pPortFormat = pComponentPrivate->pCompPort[G726D_INPUT_PORT]->pPortFormat;
    OMX_CONF_INIT_STRUCT(pPortFormat, OMX_AUDIO_PARAM_PORTFORMATTYPE);
    pPortFormat->nPortIndex         = G726D_INPUT_PORT;
    pPortFormat->nIndex             = OMX_IndexParamAudioPcm;
    pPortFormat->eEncoding          = OMX_AUDIO_CodingPCM;

    /* Set output port format defaults */
    pPortFormat = pComponentPrivate->pCompPort[G726D_OUTPUT_PORT]->pPortFormat;
    OMX_CONF_INIT_STRUCT(pPortFormat, OMX_AUDIO_PARAM_PORTFORMATTYPE);
    pPortFormat->nPortIndex         = G726D_OUTPUT_PORT;
    pPortFormat->nIndex             = OMX_IndexParamAudioPcm;
    pPortFormat->eEncoding          = OMX_AUDIO_CodingPCM;

    /* Set input port parameters */
    OMX_CONF_INIT_STRUCT(G726_ip, OMX_AUDIO_PARAM_G726TYPE);
    G726_ip->nPortIndex               = G726D_INPUT_PORT;
    G726_ip->nSize                    = sizeof(OMX_AUDIO_PARAM_G726TYPE);
    G726_ip->nVersion.s.nVersionMajor = G726DEC_MAJOR_VER;
    G726_ip->nVersion.s.nVersionMinor = G726DEC_MINOR_VER;
    G726_ip->nChannels                = 1; /* mono */
    G726_ip->eG726Mode            = OMX_AUDIO_G726Mode32;   /*most common used for default */ 

    /* Set output port parameters */
    OMX_CONF_INIT_STRUCT(G726_op, OMX_AUDIO_PARAM_PCMMODETYPE);
    G726_op->nPortIndex               = G726D_OUTPUT_PORT;
    G726_op->nSamplingRate            = 16000;
    G726_op->nChannels                = OMX_AUDIO_ChannelModeMono;
    G726_op->nSize                    = sizeof(OMX_AUDIO_PARAM_PCMMODETYPE);
    G726_op->nVersion.s.nVersionMajor = 0x1;
    G726_op->nVersion.s.nVersionMinor = 0x1;
    G726_op->nBitPerSample            = 16;
    G726_op->eNumData                 = OMX_NumericalDataUnsigned;
    G726_op->eEndian                  = OMX_EndianLittle;
    G726_op->bInterleaved             = OMX_FALSE;
    G726_op->ePCMMode                 = OMX_AUDIO_PCMModeLinear;

    pComponentPrivate->bBufferIsAllocated = 1;
    pComponentPrivate->bPortDefsAllocated = 1;

#ifdef DSP_RENDERING_ON
    if((G726d_fdwrite=open(FIFO1,O_WRONLY))<0) {
        G726DEC_EPRINT("[G726 Component] - failure to open WRITE pipe\n");
        eError = OMX_ErrorHardware;
    }
    if((G726d_fdread=open(FIFO2,O_RDONLY))<0) {
        G726DEC_EPRINT("[G726 Component] - failure to open READ pipe\n");
        eError = OMX_ErrorHardware;
    }
#endif

#ifdef RESOURCE_MANAGER_ENABLED
    eError = RMProxy_NewInitalize();
    if (eError != OMX_ErrorNone) {
        G726DEC_DPRINT ("Error returned from loading ResourceManagerProxy thread\n");
        goto EXIT;
    }
#endif

    eError = G726Dec_StartCompThread(pHandle);
    if (eError != OMX_ErrorNone) {
        G726DEC_EPRINT ("Error returned from the Component\n");
        goto EXIT;
    }

 EXIT:
    if(OMX_ErrorNone != eError) {
        G726DEC_EPRINT(":: ************* ERROR: Freeing Other Malloced Resources\n");
        G726D_OMX_FREE(pPortDef_ip);
        G726D_OMX_FREE(pPortDef_op);
        G726D_OMX_FREE(G726_ip);
        G726D_OMX_FREE(G726_op);
        G726D_OMX_FREE(pTemp);
    }
    G726DEC_DPRINT ("Exiting OMX_ComponentInit\n");
    return eError;
}


/* ================================================================================= * */
/**
 * @fn SendCommand() function receives all the commands from the application.
 *
 * @param phandle This is component handle.
 *
 * @param Cmd    This is commnad set that came from application.
 *
 * @param nParam This is commnad of the commands set that came from application.
 *
 * @param G726dData This is command data that came with command.
 *
 * @pre          OMX_Init should be called by application.
 *
 * @post         None
 *
 * @return      OMX_ErrorNone = Successful Inirialization of the component\n
 *              OMX_ErrorBadPortIndex = Bad port index specified by application.
 */
/* ================================================================================ * */
static OMX_ERRORTYPE SendCommand (OMX_HANDLETYPE phandle,
                                  OMX_COMMANDTYPE Cmd,
                                  OMX_U32 nParam,OMX_PTR pCmdData)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    int nRet = 0;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)phandle;
    G726DEC_COMPONENT_PRIVATE *pCompPrivate = NULL;

    G726D_OMX_CONF_CHECK_CMD(pHandle,1,1);
    pCompPrivate = (G726DEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;

    G726DEC_DPRINT("G726DEC: Entered SendCommand\n");
    if(pCompPrivate->curState == OMX_StateInvalid){
        G726DEC_DPRINT("G726DEC: Error Notofication Sent to App\n");
#if 0
        pCompPrivate->cbInfo.EventHandler (pHandle, 
                                           pHandle->pApplicationPrivate,
                                           OMX_EventError, 
                                           OMX_ErrorInvalidState,0,
                                           "Invalid State");
#endif
        G726D_OMX_ERROR_EXIT(eError, OMX_ErrorInvalidState,"OMX_ErrorInvalidState");
    }

    switch(Cmd) {
    case OMX_CommandStateSet:
        G726DEC_DPRINT("G726DEC: Entered switch - Command State Set\n");
        if (nParam == OMX_StateLoaded) {
            pCompPrivate->bLoadedCommandPending = OMX_TRUE;
        }
        if(pCompPrivate->curState == OMX_StateLoaded) {
            G726DEC_DPRINT("G726DEC: Entered switch - curState == OMX_StateLoaded\n");
            if((nParam == OMX_StateExecuting) || (nParam == OMX_StatePause)) {
                G726DEC_DPRINT("G726DEC: Entered switch - nParam == StatExecuting \
|| OMX_StatePause\n");
                pCompPrivate->cbInfo.EventHandler (pHandle,
                                                   pHandle->pApplicationPrivate,
                                                   OMX_EventError,
                                                   OMX_ErrorIncorrectStateTransition,
                                                   0,
                                                   NULL);
                G726DEC_DPRINT("Incorrect St Tr fm Loaded to Executing By App\n");
                goto EXIT;
            }
            if(nParam == OMX_StateInvalid) {
                pCompPrivate->curState = OMX_StateInvalid;
                pCompPrivate->cbInfo.EventHandler (pHandle,
                                                   pHandle->pApplicationPrivate,
                                                   OMX_EventError,
                                                   OMX_ErrorInvalidState,
                                                   0,
                                                   NULL);
                G726DEC_DPRINT("Incorrect State Tr from Loaded to Invalid by Application\n");
                goto EXIT;
            }
        }
        break;
    case OMX_CommandFlush:
        G726DEC_DPRINT("G726DEC: Entered switch - Command Flush\n");
        if(nParam > 1 && nParam != -1) {
            G726D_OMX_ERROR_EXIT(eError,OMX_ErrorBadPortIndex,"OMX_ErrorBadPortIndex");
        }
        break;
    case OMX_CommandPortDisable:
        break;
    case OMX_CommandPortEnable:
        G726DEC_DPRINT("G726DEC: Entered switch - Command Port Enable/Disbale\n");
        break;
    case OMX_CommandMarkBuffer:
        G726DEC_DPRINT("G726DEC: Entered switch - Command Mark Buffer\n");

        if(nParam > 0) {
            G726D_OMX_ERROR_EXIT(eError,OMX_ErrorBadPortIndex,"OMX_ErrorBadPortIndex");
        }
        break;
    default:
        G726DEC_DPRINT("G726DEC: Command Received Default error\n");
        pCompPrivate->cbInfo.EventHandler(pHandle, pHandle->pApplicationPrivate,
                                          OMX_EventError,
                                          OMX_ErrorUndefined,0,
                                          "Invalid Command");
        break;
    }


    nRet = write (pCompPrivate->cmdPipe[1], &Cmd, sizeof(Cmd));
    if (nRet == -1) {
        G726D_OMX_ERROR_EXIT(eError,OMX_ErrorHardware,"write failed: OMX_ErrorHardware");
    }

    if (Cmd == OMX_CommandMarkBuffer) {
        nRet = write (pCompPrivate->cmdDataPipe[1], &pCmdData,
                      sizeof(OMX_PTR));
        if (nRet == -1) {
            G726D_OMX_ERROR_EXIT(eError,OMX_ErrorHardware,"write failed: OMX_ErrorHardware");
        }
    }
    else {
        nRet = write (pCompPrivate->cmdDataPipe[1], &nParam,
                      sizeof(OMX_U32));
        if (nRet == -1) {
            G726D_OMX_ERROR_EXIT(eError,OMX_ErrorHardware,"write failed: OMX_ErrorHardware");
        }
    }


#ifdef DSP_RENDERING_ON
    if(Cmd == OMX_CommandStateSet && nParam == OMX_StateExecuting){
        if(pCompPrivate->acdnmode == 1) {
            /* enable COMP for ACDN1 */
            cmd_data.hComponent = pHandle;
            cmd_data.AM_Cmd = AM_CommandACDN1CompressorMode;
            cmd_data.param1 = ECOMPRESSORENABLE;  /* mode  */
            cmd_data.param2 = 0;
            cmd_data.streamID = 0;

            if((write(G726d_fdwrite, &cmd_data, sizeof(cmd_data)))<0)
                G726DEC_DPRINT("[G726 decoder] - fail to send command to audio manager\n");

            /* enable DL EQ for ACDN0 */
            cmd_data.hComponent = pHandle;
            cmd_data.AM_Cmd = AM_CommandACDN0EQDLLMode;
            cmd_data.param1 = EEQ_DLENABLE;  /* mode  */
            cmd_data.param2 = 0;
            cmd_data.streamID = 0;

            if((write(G726d_fdwrite, &cmd_data, sizeof(cmd_data)))<0)
                G726DEC_DPRINT("[G726 decoder] - fail to send command to audio manager\n");
        }else {
            /* enable Tee device command*/
            cmd_data.hComponent = pHandle;
            cmd_data.AM_Cmd = AM_CommandTDNDownlinkMode;
            cmd_data.param1 = 0;
            cmd_data.param2 = 0;
            cmd_data.streamID = 0;

            if((write(G726d_fdwrite, &cmd_data, sizeof(cmd_data)))<0) {
                eError = OMX_ErrorHardware;
                goto EXIT;
            }
        }
    }
#endif

 EXIT:
    return eError;
}



/* ================================================================================= * */
/**
 * @fn GetParameter() function gets the various parameter values of the
 * component.
 *
 * @param hComp         This is component handle.
 *
 * @param nParamIndex   This is enumerate values which specifies what kind of
 *                      information is to be retreived form the component.
 *
 * @param ComponentParameterStructure      This is output argument which is
 *                                         filled by the component component
 *
 * @pre          The component should be in loaded state.
 *
 * @post         None
 *
 * @return      OMX_ErrorNone = Successful Inirialization of the component\n
 *              OMX_ErrorBadPortIndex = Bad port index specified by application.
 */
/* ================================================================================ * */
static OMX_ERRORTYPE GetParameter (OMX_HANDLETYPE hComp,
                                   OMX_INDEXTYPE nParamIndex,
                                   OMX_PTR ComponentParameterStructure)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    G726DEC_COMPONENT_PRIVATE  *pComponentPrivate = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE *pParameterStructure = NULL;


    G726DEC_DPRINT("Inside the GetParameter:: %x\n",nParamIndex);

    G726D_OMX_CONF_CHECK_CMD(hComp,1,1);
    pComponentPrivate = (G726DEC_COMPONENT_PRIVATE *)(((OMX_COMPONENTTYPE*)hComp)->pComponentPrivate);

    G726D_OMX_CONF_CHECK_CMD(pComponentPrivate, ComponentParameterStructure, 1);
    pParameterStructure = (OMX_PARAM_PORTDEFINITIONTYPE*)ComponentParameterStructure;

    G726DEC_DPRINT(":: Entering the GetParameter\n");

    if(pComponentPrivate->curState == OMX_StateInvalid) {
#if 0
        pComponentPrivate->cbInfo.EventHandler(hComp,
                                               ((OMX_COMPONENTTYPE *)hComp)->pApplicationPrivate,
                                               OMX_EventError,
                                               OMX_ErrorIncorrectStateOperation,
                                               0,
                                               NULL);
#endif
        eError = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }

    switch(nParamIndex){
    case OMX_IndexParamAudioInit:
        G726DEC_DPRINT(":: GetParameter OMX_IndexParamAudioInit\n");
        G726D_OMX_CONF_CHECK_CMD(pComponentPrivate->sPortParam, 1, 1);
        memcpy(ComponentParameterStructure, pComponentPrivate->sPortParam, sizeof(OMX_PORT_PARAM_TYPE));
        break;

    case OMX_IndexParamPortDefinition:
        G726DEC_DPRINT(": GetParameter OMX_IndexParamPortDefinition \n");
        if(((OMX_PARAM_PORTDEFINITIONTYPE *)(ComponentParameterStructure))->nPortIndex ==
           pComponentPrivate->pPortDef[G726D_INPUT_PORT]->nPortIndex) {
            memcpy(ComponentParameterStructure, pComponentPrivate->pPortDef[G726D_INPUT_PORT], 
                   sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
        } else if(((OMX_PARAM_PORTDEFINITIONTYPE *)(ComponentParameterStructure))->nPortIndex ==
                  pComponentPrivate->pPortDef[G726D_OUTPUT_PORT]->nPortIndex) {
            memcpy(ComponentParameterStructure, pComponentPrivate->pPortDef[G726D_OUTPUT_PORT], 
                   sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
        } else {
            G726DEC_DPRINT(":: OMX_ErrorBadPortIndex from GetParameter \n");
            eError = OMX_ErrorBadPortIndex;
        }
        break;

    case OMX_IndexParamAudioPortFormat:
        G726DEC_DPRINT(":: GetParameter OMX_IndexParamAudioPortFormat \n");
        if(((OMX_AUDIO_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nPortIndex ==
           pComponentPrivate->pPortDef[G726D_INPUT_PORT]->nPortIndex) {
            if(((OMX_AUDIO_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nIndex >
               pComponentPrivate->pCompPort[G726D_INPUT_PORT]->pPortFormat->nPortIndex) {
                eError = OMX_ErrorNoMore;
            }
            else {
                memcpy(ComponentParameterStructure, pComponentPrivate->pCompPort[G726D_INPUT_PORT]->pPortFormat, 
                       sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
            }
        }
        else if(((OMX_AUDIO_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nPortIndex ==
                pComponentPrivate->pPortDef[G726D_OUTPUT_PORT]->nPortIndex){
            if(((OMX_AUDIO_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nIndex >
               pComponentPrivate->pCompPort[G726D_OUTPUT_PORT]->pPortFormat->nPortIndex) {
                eError = OMX_ErrorNoMore;
            }
            else {
                memcpy(ComponentParameterStructure, pComponentPrivate->pCompPort[G726D_OUTPUT_PORT]->pPortFormat, 
                       sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
            }
        }
        else {
            G726DEC_DPRINT(":: OMX_ErrorBadPortIndex from GetParameter \n");
            eError = OMX_ErrorBadPortIndex;
        }
        break;

    case OMX_IndexParamAudioPcm:
        G726DEC_DPRINT(" :: GetParameter OMX_IndexParamAudioPcm \n");
        if(((OMX_AUDIO_PARAM_G726TYPE *)(ComponentParameterStructure))->nPortIndex ==
           pComponentPrivate->G726Params->nPortIndex) {
            memcpy(ComponentParameterStructure, pComponentPrivate->G726Params, 
                   sizeof(OMX_AUDIO_PARAM_G726TYPE));
        } else if(((OMX_AUDIO_PARAM_PCMMODETYPE *)(ComponentParameterStructure))->nPortIndex ==
                  pComponentPrivate->PcmParams->nPortIndex) {
            memcpy(ComponentParameterStructure, pComponentPrivate->PcmParams, 
                   sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
        } else {
            G726DEC_DPRINT(" :: OMX_ErrorBadPortIndex from GetParameter \n");
            eError = OMX_ErrorBadPortIndex;
        }
        break;

    case OMX_IndexParamPriorityMgmt:
        G726DEC_DPRINT(" :: GetParameter OMX_IndexParamPriorityMgmt \n");
        G726D_OMX_CONF_CHECK_CMD(pComponentPrivate->pPriorityMgmt, 1, 1);
        memcpy(ComponentParameterStructure, pComponentPrivate->pPriorityMgmt, sizeof(OMX_PRIORITYMGMTTYPE));
        break;

    default:
        G726DEC_DPRINT(" :: OMX_ErrorUnsupportedIndex GetParameter \n");
        eError = OMX_ErrorUnsupportedIndex;
        break;
    }
 EXIT:
    G726DEC_DPRINT(" :: Exiting GetParameter\n");
    G726DEC_DPRINT(" :: Returning = 0x%x\n",eError);
    return eError;
}


/* ================================================================================= * */
/**
 * @fn SetParameter() function sets the various parameter values of the
 * component.
 *
 * @param hComp         This is component handle.
 *
 * @param nParamIndex   This is enumerate values which specifies what kind of
 *                      information is to be set for the component.
 *
 * @param ComponentParameterStructure      This is input argument which contains
 *                                         the values to be set for the component.
 *
 * @pre          The component should be in loaded state.
 *
 * @post         None
 *
 * @return      OMX_ErrorNone = Successful Inirialization of the component\n
 *              OMX_ErrorBadPortIndex = Bad port index specified by application.
 */
/* ================================================================================ * */
static OMX_ERRORTYPE SetParameter (
                                   OMX_IN  OMX_HANDLETYPE hComponent,
                                   OMX_IN  OMX_INDEXTYPE nIndex,
                                   OMX_IN  OMX_PTR pCompParam)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE* pHandle= (OMX_COMPONENTTYPE*)hComponent;
    G726DEC_COMPONENT_PRIVATE  *pComponentPrivate = NULL;
    OMX_AUDIO_PARAM_PORTFORMATTYPE* pComponentParam = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE *pComponentParamPort = NULL;
    OMX_AUDIO_PARAM_PCMMODETYPE *pCompPcmParam = NULL;
    OMX_AUDIO_PARAM_G726TYPE *pCompG726Param = NULL;
    OMX_PARAM_COMPONENTROLETYPE  *pRole = NULL;
    OMX_PARAM_BUFFERSUPPLIERTYPE sBufferSupplier;

    G726D_OMX_CONF_CHECK_CMD(hComponent,1,1)

        pComponentPrivate = (G726DEC_COMPONENT_PRIVATE *)(((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);
    G726D_OMX_CONF_CHECK_CMD(pComponentPrivate, pCompParam, 1);


    G726DEC_DPRINT(" :: Entering the SetParameter\n");
    if (pComponentPrivate->curState != OMX_StateLoaded) {
        eError = OMX_ErrorIncorrectStateOperation;
        G726DEC_DPRINT(" :: OMX_ErrorIncorrectStateOperation from SetParameter");
        goto EXIT;
    }

    switch(nIndex) {
    case OMX_IndexParamAudioPortFormat:
        G726DEC_DPRINT(":: SetParameter OMX_IndexParamAudioPortFormat \n");
        pComponentParam = (OMX_AUDIO_PARAM_PORTFORMATTYPE *)pCompParam;
        if ( pComponentParam->nPortIndex == pComponentPrivate->pCompPort[G726D_INPUT_PORT]->pPortFormat->nPortIndex ) {
            memcpy(pComponentPrivate->pCompPort[G726D_INPUT_PORT]->pPortFormat, 
                   pComponentParam, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
        } else if ( pComponentParam->nPortIndex == pComponentPrivate->pCompPort[G726D_OUTPUT_PORT]->pPortFormat->nPortIndex ) {
            memcpy(pComponentPrivate->pCompPort[G726D_OUTPUT_PORT]->pPortFormat, 
                   pComponentParam, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
        } else {
            G726DEC_DPRINT(":: OMX_ErrorBadPortIndex from SetParameter");
            eError = OMX_ErrorBadPortIndex;
        }
        break;

    case OMX_IndexParamAudioPcm:
        G726DEC_DPRINT(" :: SetParameter OMX_IndexParamAudioPcm \n");
        pCompPcmParam = (OMX_AUDIO_PARAM_PCMMODETYPE *)pCompParam;

        /* This if condition is not yet well defined because khronos
         * test suite does not set the sampling frequency. For component
         * test application, it is meant to test that component returns
         * the error on invalid frequecy */

        if (pCompPcmParam->nPortIndex == G726D_OUTPUT_PORT) { /* means Output port */
            G726D_OMX_CONF_CHECK_CMD(((G726DEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate)->PcmParams, 1, 1);
            memcpy(((G726DEC_COMPONENT_PRIVATE *)
                    pHandle->pComponentPrivate)->PcmParams, pCompPcmParam, 
                   sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
        }
        else {
            G726DEC_DPRINT(":: OMX_ErrorBadPortIndex from SetParameter");
            eError = OMX_ErrorBadPortIndex;
        }
        break;

        /********/
    case OMX_IndexParamAudioG726:
        G726DEC_DPRINT(" :: SetParameter OMX_IndexParamAudioPcm \n");
        pCompG726Param = (OMX_AUDIO_PARAM_G726TYPE *)pCompParam;

        if(pCompG726Param->nPortIndex == G726D_INPUT_PORT) { /* means Input port */
            G726D_OMX_CONF_CHECK_CMD(((G726DEC_COMPONENT_PRIVATE*)pHandle->pComponentPrivate)->G726Params, 1, 1);
            memcpy(((G726DEC_COMPONENT_PRIVATE*)
                    pHandle->pComponentPrivate)->G726Params, pCompG726Param, 
                   sizeof(OMX_AUDIO_PARAM_G726TYPE));
        }
        else {
            G726DEC_DPRINT(":: OMX_ErrorBadPortIndex from SetParameter");
            eError = OMX_ErrorBadPortIndex;
        }
        break;
        /********/

    case OMX_IndexParamPortDefinition:
        pComponentParamPort = (OMX_PARAM_PORTDEFINITIONTYPE *)pCompParam;
        if (pComponentParamPort->nPortIndex == 0) {
            if (pComponentParamPort->eDir != OMX_DirInput) {
                G726DEC_DPRINT(":: Invalid input buffer Direction\n");
                eError = OMX_ErrorBadParameter;
                goto EXIT;
            }
            if (pComponentParamPort->format.audio.eEncoding != OMX_AUDIO_CodingG726) {
                G726DEC_DPRINT(":: Invalid format Parameter\n");
                eError = OMX_ErrorBadParameter;
                goto EXIT;
            }
        } else if (pComponentParamPort->nPortIndex == 1) {
            if (pComponentParamPort->eDir != OMX_DirOutput) {
                G726DEC_DPRINT(" :: Invalid Output buffer Direction\n");
                eError = OMX_ErrorBadParameter;
                goto EXIT;
            }
            if (pComponentParamPort->format.audio.eEncoding != OMX_AUDIO_CodingPCM) {
                G726DEC_DPRINT(":: Invalid format Parameter\n");
                eError = OMX_ErrorBadParameter;
                goto EXIT;
            }
        } else {
            G726DEC_DPRINT(":: OMX_ErrorBadPortIndex from SetParameter");
            eError = OMX_ErrorBadPortIndex;
        }
        G726DEC_DPRINT(":: SetParameter OMX_IndexParamPortDefinition \n");
        if(((OMX_PARAM_PORTDEFINITIONTYPE *)(pCompParam))->nPortIndex ==
           pComponentPrivate->pPortDef[G726D_INPUT_PORT]->nPortIndex) {
            G726DEC_DPRINT(":: SetParameter OMX_IndexParamPortDefinition input \n");
            memcpy(pComponentPrivate->pPortDef[G726D_INPUT_PORT], pCompParam,
                   sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
        }
        else if(((OMX_PARAM_PORTDEFINITIONTYPE *)(pCompParam))->nPortIndex ==
                pComponentPrivate->pPortDef[G726D_OUTPUT_PORT]->nPortIndex) {
            G726DEC_DPRINT(":: SetParameter OMX_IndexParamPortDefinition output\n");
            memcpy(pComponentPrivate->pPortDef[G726D_OUTPUT_PORT], pCompParam, 
                   sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
        }
        else {
            G726DEC_DPRINT(":: OMX_ErrorBadPortIndex from SetParameter");
            eError = OMX_ErrorBadPortIndex;
        }
        break;

    case OMX_IndexParamPriorityMgmt:
        G726DEC_DPRINT(":: SetParameter OMX_IndexParamPriorityMgmt \n");
        G726D_OMX_CONF_CHECK_CMD(pComponentPrivate->pPriorityMgmt, 1, 1);
        memcpy(pComponentPrivate->pPriorityMgmt, (OMX_PRIORITYMGMTTYPE*)pCompParam, 
               sizeof(OMX_PRIORITYMGMTTYPE));
        break;

    case OMX_IndexParamAudioInit:
        G726DEC_DPRINT(":: SetParameter OMX_IndexParamAudioInit \n");
        G726D_OMX_CONF_CHECK_CMD(pComponentPrivate->sPortParam, 1, 1);
        memcpy(pComponentPrivate->sPortParam, (OMX_PORT_PARAM_TYPE*)pCompParam, 
               sizeof(OMX_PORT_PARAM_TYPE));
        break;

    case OMX_IndexParamStandardComponentRole:
        if (pCompParam) {
            pRole = (OMX_PARAM_COMPONENTROLETYPE *)pCompParam;
            G726D_OMX_CONF_CHECK_CMD(pComponentPrivate->componentRole, 1, 1);
            memcpy(pComponentPrivate->componentRole, (void *)pRole, sizeof(OMX_PARAM_COMPONENTROLETYPE));
        } else {
            eError = OMX_ErrorBadParameter;
        }
        break;

    case OMX_IndexParamCompBufferSupplier:
        if(((OMX_PARAM_PORTDEFINITIONTYPE *)(pCompParam))->nPortIndex ==
           pComponentPrivate->pPortDef[G726D_INPUT_PORT]->nPortIndex) {
            G726DEC_DPRINT(":: SetParameter OMX_IndexParamCompBufferSupplier \n");
            sBufferSupplier.eBufferSupplier = OMX_BufferSupplyInput;
            memcpy(&sBufferSupplier, pCompParam, sizeof(OMX_PARAM_BUFFERSUPPLIERTYPE));                                
                    
        }
        else if(((OMX_PARAM_PORTDEFINITIONTYPE *)(pCompParam))->nPortIndex ==
                pComponentPrivate->pPortDef[G726D_OUTPUT_PORT]->nPortIndex) {
            G726DEC_DPRINT(":: SetParameter OMX_IndexParamCompBufferSupplier \n");
            sBufferSupplier.eBufferSupplier = OMX_BufferSupplyOutput;
            memcpy(&sBufferSupplier, pCompParam, sizeof(OMX_PARAM_BUFFERSUPPLIERTYPE));
        } 
        else {
            G726DEC_DPRINT(":: OMX_ErrorBadPortIndex from SetParameter");
            eError = OMX_ErrorBadPortIndex;
        }
        break;

    default:
        G726DEC_DPRINT(":: SetParameter OMX_ErrorUnsupportedIndex \n");
        eError = OMX_ErrorUnsupportedIndex;
        break;
    }
     
 EXIT:
    G726DEC_DPRINT(":: Exiting SetParameter\n");
    G726DEC_DPRINT(":: Returning = 0x%x\n",eError);
    return eError;

}


/* ================================================================================= * */
/**
 * @fn SetCallbacks() Sets application callbacks to the component
 *
 * @param pComponent  This is component handle.
 *
 * @param pCallBacks  Application callbacks
 *
 * @param pAppData    Application specified private data.
 *
 * @pre          None
 *
 * @post         None
 *
 * @return      OMX_ErrorNone = Successful Inirialization of the component
 *              OMX_ErrorBadParameter = If callback argument is NULL.
 */
/* ================================================================================ * */

static OMX_ERRORTYPE SetCallbacks (OMX_HANDLETYPE pComponent,
                                   OMX_CALLBACKTYPE* pCallBacks,
                                   OMX_PTR pAppData)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE*)pComponent;
    G726DEC_COMPONENT_PRIVATE *pComponentPrivate = NULL;


    G726D_OMX_CONF_CHECK_CMD(pHandle,1,1)

        pComponentPrivate = (G726DEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    G726D_OMX_CONF_CHECK_CMD(pComponentPrivate,1,1)

        G726DEC_DPRINT ("Entering SetCallbacks\n");

    G726D_OMX_CONF_CHECK_CMD(pCallBacks, pCallBacks->EventHandler, pCallBacks->EmptyBufferDone)
        G726D_OMX_CONF_CHECK_CMD(pCallBacks->FillBufferDone, 1, 1)

        memcpy (&(pComponentPrivate->cbInfo), pCallBacks, sizeof(OMX_CALLBACKTYPE));
    pHandle->pApplicationPrivate = pAppData;
    G726DEC_STATEPRINT("****************** Component State Set to Loaded\n\n");
    pComponentPrivate->curState = OMX_StateLoaded;

 EXIT:
    G726DEC_DPRINT ("Exiting SetCallbacks\n");
    return eError;
}


/* ================================================================================= * */
/**
 * @fn GetComponentVersion() Sets application callbacks to the component. Currently this
 * function is not implemented.
 *
 * @param hComp  This is component handle.
 *
 * @param pComponentName  This is component name.
 *
 * @param pComponentVersion  This output argument will contain the component
 *                           version when this function exits successfully.
 *
 * @param pSpecVersion    This is specification version.
 *
 * @param pComponentUUID  This specifies the UUID of the component.
 *
 * @pre          None
 *
 * @post         None
 *
 * @return      OMX_ErrorNone = Successful Inirialization of the component
 */
/* ================================================================================ * */
static OMX_ERRORTYPE GetComponentVersion (OMX_HANDLETYPE hComp,
                                          OMX_STRING pComponentName,
                                          OMX_VERSIONTYPE* pComponentVersion,
                                          OMX_VERSIONTYPE* pSpecVersion,
                                          OMX_UUIDTYPE* pComponentUUID)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    eError = OMX_ErrorNotImplemented;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE*) hComp;
    G726DEC_COMPONENT_PRIVATE *pComponentPrivate = 
        (G726DEC_COMPONENT_PRIVATE *) pHandle->pComponentPrivate;

    /* Copy component version structure */
    if(pComponentVersion != NULL && pComponentName != NULL) {
        strcpy(pComponentName, pComponentPrivate->cComponentName);
        memcpy(pComponentVersion, &(pComponentPrivate->ComponentVersion.s), 
               sizeof(pComponentPrivate->ComponentVersion.s));
    }
    else {
        G726DEC_DPRINT("%d :: OMX_ErrorBadParameter from GetComponentVersion",__LINE__);
        eError = OMX_ErrorBadParameter;
    }

    return eError;
}



/* ================================================================================= * */
/**
 * @fn GetConfig() gets the configuration of the component depending on the value
 * of nConfigINdex. This function is currently not implemented.
 *
 * @param hComp  This is component handle.
 *
 * @param nConfigIndex  This is config index to get the configuration of
 *                      component.
 *
 * @param ComponentConfigStructure This is configuration structure that is filled
 * by the component depending on the value of nConfigIndex.
 *
 * @pre          None
 *
 * @post         None
 *
 * @return      OMX_ErrorNone = Successful Inirialization of the component
 */
/* ================================================================================ * */
static OMX_ERRORTYPE GetConfig (OMX_HANDLETYPE hComp,
                                OMX_INDEXTYPE nConfigIndex,
                                OMX_PTR ComponentConfigStructure)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    G726DEC_DPRINT ("Entering GetConfig\n");
    G726DEC_DPRINT ("Inside   GetConfig\n");
    G726DEC_DPRINT ("Exiting  GetConfig\n");
    return eError;
}

/* ================================================================================= * */
/**
 * @fn SetConfig() Sets the configuration of the component depending on the value
 * of nConfigINdex.
 *
 * @param hComp  This is component handle.
 *
 * @param nConfigIndex  This is config index to get the configuration of
 *                      component.
 *
 * @param ComponentConfigStructure This is configuration structure that contains
 *                                 the information which the component has to
 *                                 configured with.
 *
 * @pre          None
 *
 * @post         None
 *
 * @return      OMX_ErrorNone = Successful Inirialization of the component
 */
/* ================================================================================ * */
static OMX_ERRORTYPE SetConfig (OMX_HANDLETYPE hComp,
                                OMX_INDEXTYPE nConfigIndex,
                                OMX_PTR ComponentConfigStructure)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    G726DEC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    OMX_AUDIO_CONFIG_MUTETYPE *pMuteStructure = NULL;
    OMX_AUDIO_CONFIG_VOLUMETYPE *pVolumeStructure = NULL;
    TI_OMX_DSP_DEFINITION* pDspDefinition = NULL;
    OMX_S16* deviceString = NULL;
    TI_OMX_DATAPATH dataPath;

    G726DEC_DPRINT ("Entering SetConfig\n");

    G726D_OMX_CONF_CHECK_CMD(hComp,ComponentConfigStructure,1)

        pComponentPrivate = (G726DEC_COMPONENT_PRIVATE *)
        (((OMX_COMPONENTTYPE*)hComp)->pComponentPrivate);

    G726D_OMX_CONF_CHECK_CMD(pComponentPrivate,1,1)

        switch (nConfigIndex){
        case OMX_IndexCustomG726DecHeaderInfoConfig:
            pDspDefinition = (TI_OMX_DSP_DEFINITION *)ComponentConfigStructure;
            pComponentPrivate->dasfmode = pDspDefinition->dasfMode;
            pComponentPrivate->streamID = pDspDefinition->streamId;
            pComponentPrivate->packingType = pDspDefinition->packingType;
            break;

            /* set mute/unmute for playback stream */
        case OMX_IndexConfigAudioMute:
#ifdef DSP_RENDERING_ON
            pMuteStructure = (OMX_AUDIO_CONFIG_MUTETYPE *)ComponentConfigStructure;
            G726DEC_DPRINT("Set Mute/Unmute for playback stream\n");
            cmd_data.hComponent = hComp;
            if(pMuteStructure->bMute == OMX_TRUE){
                G726DEC_DPRINT("Mute the playback stream\n");
                cmd_data.AM_Cmd = AM_CommandStreamMute;
            }
            else{
                G726DEC_DPRINT("unMute the playback stream\n");
                cmd_data.AM_Cmd = AM_CommandStreamUnMute;
            }
            cmd_data.param1 = 0;
            cmd_data.param2 = 0;
            cmd_data.streamID = pComponentPrivate->streamID;

            if((write(G726d_fdwrite, &cmd_data, sizeof(cmd_data)))<0){
                G726DEC_DPRINT("[G726 decoder] - fail to send command to audio manager\n");
            }
#endif
            break;

            /* set volume for playback stream */
        case OMX_IndexConfigAudioVolume:
#ifdef DSP_RENDERING_ON
            pVolumeStructure = (OMX_AUDIO_CONFIG_VOLUMETYPE *)ComponentConfigStructure;
            G726DEC_DPRINT("Set volume for playback stream\n");
            cmd_data.hComponent = hComp;
            cmd_data.AM_Cmd = AM_CommandSWGain;
            cmd_data.param1 = pVolumeStructure->sVolume.nValue;
            cmd_data.param2 = 0;
            cmd_data.streamID = pComponentPrivate->streamID;

            if((write(G726d_fdwrite, &cmd_data, sizeof(cmd_data)))<0){
                G726DEC_DPRINT("[G726 decoder] - fail to send command to audio manager\n");
            }
#endif
            break;

        case  OMX_IndexCustomG726DecDataPath:
            deviceString = (OMX_S16*)ComponentConfigStructure;
            if (deviceString == NULL) {
                eError = OMX_ErrorBadParameter;
                goto EXIT;
            }

            dataPath = (TI_OMX_DATAPATH)*deviceString;

            switch(dataPath) {
            case DATAPATH_APPLICATION:
                /*strcpy((char*)pComponentPrivate->sDeviceString,(char*)ETEEDN_STRING);*/
                OMX_MMMIXER_DATAPATH(pComponentPrivate->sDeviceString, RENDERTYPE_DECODER, 
                                     pComponentPrivate->streamID);
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

        default:
            eError = OMX_ErrorUnsupportedIndex;
            break;
        }

    G726DEC_DPRINT ("Exiting  SetConfig\n");
 EXIT:

    return eError;
}

/* ================================================================================= * */
/**
 * @fn GetState() Gets the current state of the component.
 *
 * @param pComponent  This is component handle.
 *
 * @param pState      This is the output argument that contains the state of the
 *                    component.
 *
 * @pre          None
 *
 * @post         None
 *
 * @return      OMX_ErrorNone = Successful Inirialization of the component
 *              OMX_ErrorBadParameter = if output argument is NULL.
 */
/* ================================================================================ * */
static OMX_ERRORTYPE GetState (OMX_HANDLETYPE pComponent, OMX_STATETYPE* pState)
{
    OMX_ERRORTYPE eError = OMX_ErrorUndefined;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;

    if (!pState) {
        G726DEC_DPRINT (":: About to exit with bad parameter\n");
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }

    G726D_OMX_CONF_CHECK_CMD(pHandle,1,1)
        if (pHandle && pHandle->pComponentPrivate) {
            *pState =  ((G726DEC_COMPONENT_PRIVATE*)
                        pHandle->pComponentPrivate)->curState;
        } else {
            G726DEC_STATEPRINT("Component State Set to Loaded\n\n");
            *pState = OMX_StateLoaded;
        }

    eError = OMX_ErrorNone;

 EXIT:
    return eError;
}

/* ================================================================================= * */
/**
 * @fn EmptyThisBuffer() This function is used by application to sent the filled
 * input buffers to the component.
 *
 * @param pComponent  This is component handle.
 *
 * @param pBuffer     This is poiter to the buffer header that come from the
 *                    application.
 *
 * @pre          None
 *
 * @post         None
 *
 * @return      OMX_ErrorNone = Successful exit of the function
 *              OMX_ErrorBadParameter =  Bad input argument
 *              OMX_ErrorBadPortIndex = Bad port index supplied by the
 *              application
 */
/* ================================================================================ * */
static OMX_ERRORTYPE EmptyThisBuffer (OMX_HANDLETYPE pComponent,
                                      OMX_BUFFERHEADERTYPE* pBuffer)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    G726DEC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef = NULL;
    int ret=0;

    G726D_OMX_CONF_CHECK_CMD(pHandle,1,1)

        pComponentPrivate = (G726DEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;

    G726D_OMX_CONF_CHECK_CMD(pComponentPrivate,1,1)

        pPortDef = ((G726DEC_COMPONENT_PRIVATE*)
                    pComponentPrivate)->pPortDef[G726D_INPUT_PORT];


    if(!pPortDef->bEnabled) {
        G726D_OMX_ERROR_EXIT(eError,OMX_ErrorIncorrectStateOperation,
                             "OMX_ErrorIncorrectStateOperation");
    }

    if (pBuffer == NULL) {
        G726D_OMX_ERROR_EXIT(eError,OMX_ErrorBadParameter,"OMX_ErrorBadParameter");
    }

    if (pBuffer->nSize != sizeof(OMX_BUFFERHEADERTYPE)) {
        G726DEC_EPRINT(":: Error: Bad Size = %ld, Add: %p\n",
                       pBuffer->nSize,pBuffer);
        G726D_OMX_ERROR_EXIT(eError,OMX_ErrorBadParameter,"Bad Size");
    } 

    if (pBuffer->nInputPortIndex != G726D_INPUT_PORT) {
        G726D_OMX_ERROR_EXIT(eError,OMX_ErrorBadPortIndex,"OMX_ErrorBadPortIndex");
    }


    if (pBuffer->nVersion.nVersion != pComponentPrivate->nVersion) {
        G726D_OMX_ERROR_EXIT(eError,OMX_ErrorVersionMismatch,"OMX_ErrorVersionMismatch");
    }


    if(pComponentPrivate->curState != OMX_StateExecuting && 
       pComponentPrivate->curState != OMX_StatePause && 
       pComponentPrivate->curState != OMX_StateIdle) {
        G726D_OMX_ERROR_EXIT(eError,OMX_ErrorIncorrectStateOperation,
                             "OMX_ErrorIncorrectStateOperation");
    }


    G726DEC_DPRINT("\n------------------------------------------\n\n");
    G726DEC_DPRINT (":: Component Sending Filled ip buff %p \
                             to Component Thread\n",pBuffer);
    G726DEC_DPRINT("\n------------------------------------------\n\n");

    if (pComponentPrivate->bBypassDSP == 0) {
        pComponentPrivate->app_nBuf--;
    }

    pComponentPrivate->pMarkData = pBuffer->pMarkData;
    pComponentPrivate->hMarkTargetComponent = pBuffer->hMarkTargetComponent;
    pComponentPrivate->nUnhandledEmptyThisBuffers++;
    ret = write (pComponentPrivate->dataPipe[1], &pBuffer,
                 sizeof(OMX_BUFFERHEADERTYPE*));
    if (ret == -1) {
        G726D_OMX_ERROR_EXIT(eError,OMX_ErrorHardware,"write failed: OMX_ErrorHardware");
    }

    pComponentPrivate->nEmptyThisBufferCount++;

 EXIT:
    G726DEC_DPRINT (":: Exiting EmptyThisBuffer, eError = %d\n",eError);
    return eError;
}

/* ================================================================================= * */
/**
 * @fn FillThisBuffer() This function is used by application to sent the empty
 * output buffers to the component.
 *
 * @param pComponent  This is component handle.
 *
 * @param pBuffer     This is poiter to the output buffer header that come from the
 *                    application.
 *
 * @pre          None
 *
 * @post         None
 *
 * @return      OMX_ErrorNone = Successful exit of the function
 *              OMX_ErrorBadParameter =  Bad input argument
 *              OMX_ErrorBadPortIndex = Bad port index supplied by the
 *              application
 */
/* ================================================================================ * */
static OMX_ERRORTYPE FillThisBuffer (OMX_HANDLETYPE pComponent,
                                     OMX_BUFFERHEADERTYPE* pBuffer)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    G726DEC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    int nRet=0;
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef = NULL;

    G726DEC_DPRINT("\n------------------------------------------\n\n");
    G726DEC_DPRINT (" :: Component Sending Emptied op buff %p \
                             to Component Thread\n",pBuffer);
    G726DEC_DPRINT("\n------------------------------------------\n\n");


    G726D_OMX_CONF_CHECK_CMD(pHandle,1,1)

        pComponentPrivate = (G726DEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;

    G726D_OMX_CONF_CHECK_CMD(pComponentPrivate,1,1)


        pPortDef = ((G726DEC_COMPONENT_PRIVATE*)
                    pComponentPrivate)->pPortDef[G726D_OUTPUT_PORT];

    if(!pPortDef->bEnabled) {
        G726D_OMX_ERROR_EXIT(eError,OMX_ErrorIncorrectStateOperation,
                             "OMX_ErrorIncorrectStateOperation");
    }

    if (pBuffer == NULL) {
        G726D_OMX_ERROR_EXIT(eError,OMX_ErrorBadParameter,"OMX_ErrorBadParameter");
    }

    if (pBuffer->nSize != sizeof(OMX_BUFFERHEADERTYPE)) {
        G726D_OMX_ERROR_EXIT(eError,OMX_ErrorBadParameter,"OMX_ErrorBadParameter");
    }

    if (pBuffer->nOutputPortIndex != G726D_OUTPUT_PORT) {
        G726D_OMX_ERROR_EXIT(eError,OMX_ErrorBadPortIndex,"OMX_ErrorBadPortIndex");
    }

    G726DEC_DPRINT("::pBuffer->nVersion.nVersion:%ld\n",pBuffer->nVersion.nVersion);
    G726DEC_DPRINT("::pComponentPrivate->nVersion:%ld\n",pComponentPrivate->nVersion);
    if (pBuffer->nVersion.nVersion != pComponentPrivate->nVersion) {
        G726D_OMX_ERROR_EXIT(eError,OMX_ErrorVersionMismatch,"OMX_ErrorVersionMismatch");
    }

    if(pComponentPrivate->curState != OMX_StateExecuting && 
       pComponentPrivate->curState != OMX_StatePause &&
       pComponentPrivate->curState != OMX_StateIdle) {
        G726D_OMX_ERROR_EXIT(eError,OMX_ErrorIncorrectStateOperation,
                             "OMX_ErrorIncorrectStateOperation");
    }


    pBuffer->nFilledLen = 0;

    if (pComponentPrivate->bBypassDSP == 0) {
        pComponentPrivate->app_nBuf--;
    }

    if(pComponentPrivate->pMarkBuf){
        G726DEC_DPRINT("FillThisBuffer Line\n");
        pBuffer->hMarkTargetComponent = pComponentPrivate->pMarkBuf->hMarkTargetComponent;
        pBuffer->pMarkData = pComponentPrivate->pMarkBuf->pMarkData;
        pComponentPrivate->pMarkBuf = NULL;
    }

    if (pComponentPrivate->pMarkData) {
        G726DEC_DPRINT("FillThisBuffer Line\n");
        pBuffer->hMarkTargetComponent = pComponentPrivate->hMarkTargetComponent;
        pBuffer->pMarkData = pComponentPrivate->pMarkData;
        pComponentPrivate->pMarkData = NULL;
    }
    pComponentPrivate->nUnhandledFillThisBuffers++;
    nRet = write (pComponentPrivate->dataPipe[1], &pBuffer,
                  sizeof (OMX_BUFFERHEADERTYPE*));
    if (nRet == -1) {
        G726D_OMX_ERROR_EXIT(eError,OMX_ErrorHardware,"write failed: OMX_ErrorHardware");
    }

    pComponentPrivate->nFillThisBufferCount++;

 EXIT:
    G726DEC_DPRINT (":: Exiting FillThisBuffer\n");
    return eError;
}

/* ================================================================================= * */
/**
 * @fn ComponentDeInit() This function deinitializes the component. It is called
 * from OMX Core, not by application. Albeit, Application does call
 * OMX_FreeHandle of OMX Core and which in turn calls this function.
 *
 * @param pHandle  This is component handle.
 *
 * @pre          None
 *
 * @post        This function should clean or free as much resources as
 *              possible.
 *
 * @return      OMX_ErrorNone = On Success
 *              Appropriate error number in case any error happens.
 */
/* ================================================================================ * */
static OMX_ERRORTYPE ComponentDeInit(OMX_HANDLETYPE pHandle)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_ERRORTYPE eError1 = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pComponent = (OMX_COMPONENTTYPE *)pHandle;
    G726DEC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    OMX_ERRORTYPE threadError = OMX_ErrorNone;
    int pthreadError = 0;

    G726DEC_DPRINT("ComponentDeInit\n");
    G726D_OMX_CONF_CHECK_CMD(pComponent,1,1)
        pComponentPrivate = (G726DEC_COMPONENT_PRIVATE *)pComponent->pComponentPrivate;
    G726D_OMX_CONF_CHECK_CMD(pComponentPrivate,1,1)

#ifdef DSP_RENDERING_ON
        close(G726d_fdwrite);
    close(G726d_fdread);
#endif

#ifdef RESOURCE_MANAGER_ENABLED
    /*  eError = RMProxy_SendCommand(pHandle, RMProxy_FreeResource, 
        OMX_PCM_Decoder_COMPONENT, 0, NULL); */
    eError = RMProxy_NewSendCommand(pHandle, RMProxy_FreeResource, OMX_PCM_Decoder_COMPONENT, 0, 3456,NULL);                                 
    if (eError != OMX_ErrorNone) {
        G726DEC_EPRINT ("%d ::Error returned from destroy ResourceManagerProxy thread\n",
                        __LINE__);
    }

    eError1 = RMProxy_Deinitalize();
    if (eError1 != OMX_ErrorNone) {
        G726DEC_EPRINT(":: First Error in ComponentDeinit: From RMProxy_Deinitalize\n");
        eError = eError1;
    }

    /*eError = RMProxy_SendCommand(pHandle, RMProxy_FreeResource, 
      OMX_G726_Decoder_COMPONENT, 0, NULL);
      if (eError != OMX_ErrorNone) {
      G726DEC_EPRINT ("%d ::Error returned from destroy ResourceManagerProxy thread\n",
      __LINE__);
      }

      eError1 = RMProxy_Deinitalize();
      if (eError1 != OMX_ErrorNone) {
      G726DEC_EPRINT(":: First Error in ComponentDeinit: From RMProxy_Deinitalize\n");
      eError = eError1;
      }*/
#endif

    pComponentPrivate->bExitCompThrd = 1;
    
    pthreadError = pthread_join(pComponentPrivate->ComponentThread, (void*)&threadError);
    if(0 != pthreadError) {
        G726DEC_EPRINT(":: First Error in ComponentDeinit: From pthread_join\n");
        eError = OMX_ErrorHardware;
        goto EXIT;
    }

    if (OMX_ErrorNone != threadError && OMX_ErrorNone != eError) {
        eError = OMX_ErrorInsufficientResources;
        G726DEC_EPRINT("%d :: Error while closing Component Thread\n",__LINE__);
        goto EXIT;
    }

    eError1 = G726DEC_FreeCompResources(pHandle);
    if (OMX_ErrorNone != eError1) {
        if (OMX_ErrorNone == eError) {
            G726DEC_EPRINT(":: First Error in ComponentDeinit: From FreeCompResources\n");
            eError = eError1;
        }
    }

    if (pComponentPrivate->sDeviceString != NULL) {
        G726D_OMX_FREE(pComponentPrivate->sDeviceString);
    }

    G726DEC_MEMPRINT(":: Freeing: pComponentPrivate = %p\n", pComponentPrivate);
    G726D_OMX_FREE(pComponentPrivate);
    G726DEC_DPRINT("::*********** ComponentDeinit is Done************** \n");

 EXIT:
    return eError;
}


/* ================================================================================= * */
/**
 * @fn ComponentTunnelRequest() This function estabilishes the tunnel between two
 * components. This is not implemented currently.
 *
 * @param hComp  Handle of this component.
 *
 * @param nPort Port of this component on which tunneling has to be done.
 *
 * @param hTunneledComp Handle of the component with which tunnel has to be
 *                      established.
 *
 * @param nTunneledPort Port of the tunneling component.
 *
 * @param pTunnelSetup Tunnel Setuup parameters.
 *
 * @pre          None
 *
 * @post        None
 *
 * @return      OMX_ErrorNone = On Success
 *              Appropriate error number in case any error happens.
 */
/* ================================================================================ * */
static OMX_ERRORTYPE ComponentTunnelRequest (OMX_HANDLETYPE hComp,
                                             OMX_U32 nPort, OMX_HANDLETYPE hTunneledComp,
                                             OMX_U32 nTunneledPort,
                                             OMX_TUNNELSETUPTYPE* pTunnelSetup)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    G726DEC_DPRINT (":: Entering ComponentTunnelRequest\n");
    G726DEC_DPRINT (":: Inside   ComponentTunnelRequest\n");
    eError = OMX_ErrorNotImplemented;
    G726DEC_DPRINT (":: Exiting ComponentTunnelRequest\n");
    return eError;
}



/* ================================================================================= * */
/**
 * @fn AllocateBuffer() This function allocated the memory for the buffer onm
 * request from application.
 *
 * @param hComponent  Handle of this component.
 *
 * @param pBuffer  Pointer to the buffer header.
 *
 * @param nPortIndex  Input port or Output port
 *
 * @param pAppPrivate Application private data.
 *
 * @param nSizeBytes Size of the buffer that is to be allocated.
 *
 * @pre          None
 *
 * @post        Requested buffer should get the memory allocated.
 *
 * @return      OMX_ErrorNone = On Success
 *              OMX_ErrorBadPortIndex = Bad port index from app
 */
/* ================================================================================ * */
static OMX_ERRORTYPE AllocateBuffer (OMX_IN OMX_HANDLETYPE hComponent,
                                     OMX_INOUT OMX_BUFFERHEADERTYPE** pBuffer,
                                     OMX_IN OMX_U32 nPortIndex,
                                     OMX_IN OMX_PTR pAppPrivate,
                                     OMX_IN OMX_U32 nSizeBytes)
{
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef = NULL;
    G726DEC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_BUFFERHEADERTYPE *pBufferHeader = NULL;


    G726DEC_DPRINT (":: Entering AllocateBuffer\n");
    G726D_OMX_CONF_CHECK_CMD(hComponent,1,1)
        pComponentPrivate = (G726DEC_COMPONENT_PRIVATE *)
        (((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);

    G726D_OMX_CONF_CHECK_CMD(pComponentPrivate, 1, 1)

        pPortDef = ((G726DEC_COMPONENT_PRIVATE*)
                    pComponentPrivate)->pPortDef[nPortIndex];

    G726D_OMX_CONF_CHECK_CMD(pPortDef, 1, 1)

        if (!pPortDef->bEnabled) {
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

    G726D_OMX_MALLOC(pBufferHeader, OMX_BUFFERHEADERTYPE);

    /* Needed for cache synchronization between ARM and DSP */
    G726D_OMX_MALLOC_SIZE(pBufferHeader->pBuffer,(nSizeBytes + 256),OMX_U8);
    pBufferHeader->pBuffer += 128;
    pBufferHeader->nVersion.nVersion = G726DEC_BUFHEADER_VERSION;


    G726DEC_DPRINT("********************************************\n");
    G726DEC_DPRINT(":: Allocated BufHeader %p Buffer = %p, on port %ld\n",
                   pBufferHeader,
                   pBufferHeader->pBuffer, nPortIndex);

    G726DEC_DPRINT(":: Ip Num = %ld\n",pComponentPrivate->pInputBufferList->numBuffers);
    G726DEC_DPRINT(":: Op Num = %ld\n",pComponentPrivate->pOutputBufferList->numBuffers);
    G726DEC_DPRINT("********************************************\n");


    pBufferHeader->pAppPrivate = pAppPrivate;
    pBufferHeader->pPlatformPrivate = pComponentPrivate;
    pBufferHeader->nAllocLen = nSizeBytes;

    if (nPortIndex == G726D_INPUT_PORT) {
        pBufferHeader->nInputPortIndex = nPortIndex;
        pBufferHeader->nOutputPortIndex = -1;
        pComponentPrivate->pInputBufferList->pBufHdr[pComponentPrivate->pInputBufferList->numBuffers] = pBufferHeader;
        pComponentPrivate->pInputBufferList->bBufferPending[pComponentPrivate->pInputBufferList->numBuffers] = 0;

        G726DEC_DPRINT("pComponentPrivate->pInputBufferList->pBufHdr[%ld] = %p\n",
                       pComponentPrivate->pInputBufferList->numBuffers,
                       pComponentPrivate->pInputBufferList->pBufHdr[pComponentPrivate->pInputBufferList->numBuffers]);

        pComponentPrivate->pInputBufferList->bufferOwner[pComponentPrivate->pInputBufferList->numBuffers++] = 1;

        G726DEC_DPRINT("pComponentPrivate->pInputBufferList->numBuffers = %ld\n",
                       pComponentPrivate->pInputBufferList->numBuffers);

        if (pComponentPrivate->pInputBufferList->numBuffers == pPortDef->nBufferCountActual) {
            pPortDef->bPopulated = 1;
        }
    } else if (nPortIndex == G726D_OUTPUT_PORT) {
        pBufferHeader->nInputPortIndex = -1;
        pBufferHeader->nOutputPortIndex = nPortIndex;
        pComponentPrivate->pOutputBufferList->pBufHdr[pComponentPrivate->pOutputBufferList->numBuffers] = pBufferHeader;
        pComponentPrivate->pOutputBufferList->bBufferPending[pComponentPrivate->pOutputBufferList->numBuffers] = 0;

        G726DEC_DPRINT("pComponentPrivate->pOutputBufferList->pBufHdr[%ld] = %p\n",
                       pComponentPrivate->pOutputBufferList->numBuffers,
                       pComponentPrivate->pOutputBufferList->pBufHdr[pComponentPrivate->pOutputBufferList->numBuffers]);

        pComponentPrivate->pOutputBufferList->bufferOwner[pComponentPrivate->pOutputBufferList->numBuffers++] = 1;

        G726DEC_DPRINT("pComponentPrivate->pOutputBufferList->numBuffers = %ld\n",
                       pComponentPrivate->pOutputBufferList->numBuffers);

        if (pComponentPrivate->pOutputBufferList->numBuffers == pPortDef->nBufferCountActual) {
            pPortDef->bPopulated = 1;
        }
    } else {
        G726D_OMX_ERROR_EXIT(eError,OMX_ErrorBadPortIndex,"OMX_ErrorBadPortIndex");
    }

    if((pComponentPrivate->pPortDef[G726D_OUTPUT_PORT]->bPopulated == 
        pComponentPrivate->pPortDef[G726D_OUTPUT_PORT]->bEnabled)&&
       (pComponentPrivate->pPortDef[G726D_INPUT_PORT]->bPopulated == 
        pComponentPrivate->pPortDef[G726D_INPUT_PORT]->bEnabled) &&
       (pComponentPrivate->InLoaded_readytoidle)) {
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

    pBufferHeader->nVersion.s.nVersionMajor = G726DEC_MAJOR_VER;
    pBufferHeader->nVersion.s.nVersionMinor = G726DEC_MINOR_VER;
    pComponentPrivate->nVersion = pBufferHeader->nVersion.nVersion;
    pBufferHeader->nSize = sizeof(OMX_BUFFERHEADERTYPE);

    *pBuffer = pBufferHeader;
    pComponentPrivate->bufAlloced = 1;
    G726DEC_DPRINT(":: A Buffer has been alloced\n");

 EXIT:
    if(OMX_ErrorNone != eError) {
        G726DEC_DPRINT("%d :: ************* ERROR: Freeing Other Malloced Resources\n",__LINE__);
	 if (pBufferHeader != NULL) {
	     G726D_OMX_FREE(pBufferHeader->pBuffer);
	     G726D_OMX_FREE(pBufferHeader);
	 }
    }

    return eError;
}

/* ================================================================================= * */
/**
 * @fn FreeBuffer() This function frees the meomory of the buffer specified.
 *
 * @param hComponent  Handle of this component.
 *
 * @param nPortIndex  Input port or Output port
 *
 * @param pBuffer  Pointer to the buffer header.
 *
 * @pre          None
 *
 * @post        Requested buffer should get the memory allocated.
 *
 * @return      OMX_ErrorNone = On Success
 *              OMX_ErrorBadPortIndex = Bad port index from app
 */
/* ================================================================================ * */
static OMX_ERRORTYPE FreeBuffer(
                                OMX_IN  OMX_HANDLETYPE hComponent,
                                OMX_IN  OMX_U32 nPortIndex,
                                OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    G726DEC_COMPONENT_PRIVATE * pComponentPrivate = NULL;
    OMX_U8* buff = NULL;
    int i = 0;
    int inputIndex = -1;
    int outputIndex = -1;
    OMX_COMPONENTTYPE *pHandle = 0;

    G726DEC_DPRINT ("Entering FreeBuffer\n");

    pComponentPrivate = (G726DEC_COMPONENT_PRIVATE *) 
        (((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);

    pHandle = (OMX_COMPONENTTYPE *) pComponentPrivate->pHandle;
    for (i=0; i < MAX_NUM_OF_BUFS; i++) {
        buff = (OMX_U8 *)pComponentPrivate->pInputBufferList->pBufHdr[i];
        if (buff == (OMX_U8 *)pBuffer) {
            G726DEC_DPRINT("Found matching input buffer\n");
            G726DEC_DPRINT("buff = %p\n",buff);
            G726DEC_DPRINT("pBuffer = %p\n",pBuffer);
            inputIndex = i;
            break;
        }else{
            G726DEC_DPRINT("%d:: No matching found for pBuffer %p\n",__LINE__,pBuffer);
        }
    }

    for (i=0; i < MAX_NUM_OF_BUFS; i++) {
        buff = (OMX_U8 *)pComponentPrivate->pOutputBufferList->pBufHdr[i];
        if (buff == (OMX_U8 *)pBuffer) {
            G726DEC_DPRINT("Found matching output buffer\n");
            G726DEC_DPRINT("buff = %p\n",buff);
            G726DEC_DPRINT("pBuffer = %p\n",pBuffer);
            outputIndex = i;
            break;
        }else{
            G726DEC_DPRINT("%d:: No matching found for pBuffer %p\n",__LINE__,pBuffer);
        }
    }


    if (inputIndex != -1) {
        if (pComponentPrivate->pInputBufferList->bufferOwner[inputIndex] == 1) {
            buff = pComponentPrivate->pInputBufferList->pBufHdr[inputIndex]->pBuffer;
            if(buff != 0){
                buff -= 128;
            }
            G726DEC_MEMPRINT("\n: Freeing:  %p IP Buffer\n",buff);
            G726D_OMX_FREE(buff);
        }

        G726DEC_MEMPRINT("Freeing: %p IP Buf Header\n\n",
                         pComponentPrivate->pInputBufferList->pBufHdr[inputIndex]);

        G726D_OMX_FREE(pComponentPrivate->pInputBufferList->pBufHdr[inputIndex]);
        pComponentPrivate->pInputBufferList->numBuffers--;

        if (pComponentPrivate->pInputBufferList->numBuffers <
            pComponentPrivate->pPortDef[G726D_INPUT_PORT]->nBufferCountMin) {
            pComponentPrivate->pPortDef[G726D_INPUT_PORT]->bPopulated = OMX_FALSE;
        }
        if(pComponentPrivate->pPortDef[G726D_INPUT_PORT]->bEnabled &&
           pComponentPrivate->bLoadedCommandPending == OMX_FALSE &&
           (pComponentPrivate->curState == OMX_StateIdle ||
            pComponentPrivate->curState == OMX_StateExecuting ||
            pComponentPrivate->curState == OMX_StatePause)) {

            pComponentPrivate->cbInfo.EventHandler(
                                                   pHandle, pHandle->pApplicationPrivate,
                                                   OMX_EventError, OMX_ErrorPortUnpopulated,nPortIndex, NULL);
        }
    } else if (outputIndex != -1) {
        if (pComponentPrivate->pOutputBufferList->bBufferPending[outputIndex]) {
            pComponentPrivate->numPendingBuffers++;
        }
        if (pComponentPrivate->pOutputBufferList->bufferOwner[outputIndex] == 1) {
            buff = pComponentPrivate->pOutputBufferList->pBufHdr[outputIndex]->pBuffer;
            if(buff != 0){
                buff -= 128;
            }
            G726DEC_MEMPRINT("Freeing: %p OP Buffer\n",buff);
            G726D_OMX_FREE(buff);
        }

        G726DEC_MEMPRINT("Freeing: %p OP Buf Header\n\n",
                         pComponentPrivate->pOutputBufferList->pBufHdr[outputIndex]);

        G726D_OMX_FREE(pComponentPrivate->pOutputBufferList->pBufHdr[outputIndex]);
        pComponentPrivate->pOutputBufferList->numBuffers--;

        G726DEC_DPRINT("pComponentPrivate->pOutputBufferList->numBuffers = %ld\n",
                       pComponentPrivate->pOutputBufferList->numBuffers);
        G726DEC_DPRINT("pComponentPrivate->pPortDef[G726D_OUTPUT_PORT]->nBufferCountMin = %ld\n",
                       pComponentPrivate->pPortDef[G726D_OUTPUT_PORT]->nBufferCountMin);

        if (pComponentPrivate->pOutputBufferList->numBuffers <
            pComponentPrivate->pPortDef[G726D_OUTPUT_PORT]->nBufferCountMin) {
            pComponentPrivate->pPortDef[G726D_OUTPUT_PORT]->bPopulated = OMX_FALSE;
        }

        if(pComponentPrivate->pPortDef[G726D_OUTPUT_PORT]->bEnabled &&
           pComponentPrivate->bLoadedCommandPending == OMX_FALSE &&
           (pComponentPrivate->curState == OMX_StateIdle ||
            pComponentPrivate->curState == OMX_StateExecuting ||
            pComponentPrivate->curState == OMX_StatePause)) {
            pComponentPrivate->cbInfo.EventHandler(
                                                   pHandle, pHandle->pApplicationPrivate,
                                                   OMX_EventError, OMX_ErrorPortUnpopulated,nPortIndex, NULL);
        }
    } else {
        G726DEC_DPRINT("Returning OMX_ErrorBadParameter\n");
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

    pComponentPrivate->bufAlloced = 0;
    if (pComponentPrivate->bDisableCommandPending && (pComponentPrivate->pInputBufferList->numBuffers + 
                                                      pComponentPrivate->pOutputBufferList->numBuffers == 0)) {
        SendCommand (pComponentPrivate->pHandle,OMX_CommandPortDisable,
                     pComponentPrivate->bDisableCommandParam,NULL);
    }

    G726DEC_DPRINT ("Exiting FreeBuffer\n");
    return eError;
}


/* ================================================================================= * */
/**
 * @fn UseBuffer() This function is called by application when app allocated the
 * memory for the buffer and sends it to application for use of component.
 *
 * @param hComponent  Handle of this component.
 *
 * @param ppBufferHdr  Double pointer to the buffer header.
 *
 * @param nPortIndex  Input port or Output port
 *
 * @param pAppPrivate Application private data.
 *
 * @param nSizeBytes Size of the buffer that is to be allocated.
 *
 * @param pBuffer    Pointer to data buffer which was allocated by the
 * application.
 *
 * @pre          None
 *
 * @post        None
 *
 * @return      OMX_ErrorNone = On Success
 *              OMX_ErrorBadPortIndex = Bad port index from app
 */
/* ================================================================================ * */
static OMX_ERRORTYPE UseBuffer (
                                OMX_IN OMX_HANDLETYPE hComponent,
                                OMX_INOUT OMX_BUFFERHEADERTYPE** ppBufferHdr,
                                OMX_IN OMX_U32 nPortIndex,
                                OMX_IN OMX_PTR pAppPrivate,
                                OMX_IN OMX_U32 nSizeBytes,
                                OMX_IN OMX_U8* pBuffer)
{
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef = NULL;
    G726DEC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_BUFFERHEADERTYPE *pBufferHeader = NULL;

    G726DEC_DPRINT ("Entering UseBuffer\n");
    G726DEC_DPRINT ("pBuffer = %p\n",pBuffer);

    pComponentPrivate = (G726DEC_COMPONENT_PRIVATE *)
        (((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);

    pPortDef = ((G726DEC_COMPONENT_PRIVATE*)
                pComponentPrivate)->pPortDef[nPortIndex];
    G726DEC_DPRINT ("pPortDef = %p\n", pPortDef);
    G726DEC_DPRINT ("pPortDef->bEnabled = %d\n",pPortDef->bEnabled);

    if(!pPortDef->bEnabled) {
        G726D_OMX_ERROR_EXIT(eError,OMX_ErrorIncorrectStateOperation,
                             "Port is Disabled: OMX_ErrorIncorrectStateOperation");
    }
#if 1
    if(nSizeBytes != pPortDef->nBufferSize || pPortDef->bPopulated) {
        G726D_OMX_ERROR_EXIT(eError,OMX_ErrorBadParameter,
                             "Bad Size or Port Disabled : OMX_ErrorBadParameter");
    }
#endif

#if 0
    if(pPortDef->bPopulated) {
        G726D_OMX_ERROR_EXIT(eError,OMX_ErrorBadParameter,
                             "Bad Size or Port Disabled : OMX_ErrorBadParameter");
    }
#endif

    G726D_OMX_MALLOC(pBufferHeader, OMX_BUFFERHEADERTYPE);

    if (nPortIndex == G726D_OUTPUT_PORT) {
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

    if((pComponentPrivate->pPortDef[G726D_OUTPUT_PORT]->bPopulated == 
        pComponentPrivate->pPortDef[G726D_OUTPUT_PORT]->bEnabled)&&
       (pComponentPrivate->pPortDef[G726D_INPUT_PORT]->bPopulated == 
        pComponentPrivate->pPortDef[G726D_INPUT_PORT]->bEnabled) &&
       (pComponentPrivate->InLoaded_readytoidle)) {
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

    pBufferHeader->nVersion.s.nVersionMajor = G726DEC_MAJOR_VER;
    pBufferHeader->nVersion.s.nVersionMinor = G726DEC_MINOR_VER;
    pComponentPrivate->nVersion = pBufferHeader->nVersion.nVersion;

    pBufferHeader->pBuffer = pBuffer;
    pBufferHeader->nSize = sizeof(OMX_BUFFERHEADERTYPE);
    *ppBufferHdr = pBufferHeader;
    pComponentPrivate->bufAlloced = 1;

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

    G726DEC_DPRINT("GetExtensionIndex\n");
    if (!(strcmp(cParameterName,"OMX.TI.index.config.g726headerinfo"))) {
        *pIndexType = OMX_IndexCustomG726DecHeaderInfoConfig;
        G726DEC_DPRINT("OMX_IndexCustomG726DecHeaderInfoConfig\n");
    }
    else if(!(strcmp(cParameterName,"OMX.TI.index.config.g726decstreamIDinfo"))){
        *pIndexType = OMX_IndexCustomG726DecStreamIDConfig;
        G726DEC_DPRINT("OMX_IndexCustomG726DecStreamIDConfig\n");
    }
    else if(!(strcmp(cParameterName,"OMX.TI.index.config.g726dec.datapath"))) {
        *pIndexType = OMX_IndexCustomG726DecDataPath;
        G726DEC_DPRINT("OMX_IndexCustomG726DecDataPath\n");
    } 
    else {
        eError = OMX_ErrorBadParameter;
    }

    G726DEC_DPRINT("Exiting GetExtensionIndex\n");
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
    OMX_ERRORTYPE eError = OMX_ErrorNotImplemented;

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
        MP3DEC_EPRINT("OMX_CreateEvent: OMX_ErrorBadParameter\n");
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
        MP3DEC_EPRINT("OMX_CreateEvent: OMX_ErrorBadParameter\n");
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
        MP3DEC_EPRINT("OMX_CreateEvent: OMX_ErrorBadParameter\n");
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
        MP3DEC_EPRINT("OMX_CreateEvent: OMX_ErrorBadParameter\n");
        goto EXIT;
    }  
    CloseHandle(event->event);
 EXIT:    
    return ret;
}
#endif
