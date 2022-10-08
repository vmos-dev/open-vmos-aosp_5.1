
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
 * @file OMX_G722Decoder.c
 *
 * This file implements OpenMAX (TM) 1.0 Specific APIs and its functionality
 * that is fully compliant with the Khronos OpenMAX (TM) 1.0 Specification
 *
 * @path  $(CSLPATH)\OMAPSW_MPU\linux\audio\src\openmax_il\g722_dec\src
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
#include "OMX_G722Dec_Utils.h"
#include <TIDspOmx.h>

#ifdef DSP_RENDERING_ON
#include <AudioManagerAPI.h>
#endif

#ifdef DSP_RENDERING_ON

#define FIFO1 "/dev/fifo.1"
#define FIFO2 "/dev/fifo.2"
#define PERMS 0666

AM_COMMANDDATATYPE cmd_data;
int G722d_fdwrite = 0, G722d_fdread = 0;
int errno;
#endif
/* define component role */
#define G722_DEC_ROLE "audio_decoder.g722"

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
 *  @see          G722Dec_StartCompThread()
 */
/* ================================================================================ * */
OMX_ERRORTYPE OMX_ComponentInit (OMX_HANDLETYPE hComp)
{

    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE*) hComp;
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef_ip = NULL, *pPortDef_op = NULL;
    OMX_AUDIO_PARAM_PORTFORMATTYPE *pPortFormat = NULL;
    OMX_AUDIO_PARAM_PCMMODETYPE   *G722_ip = NULL, *G722_op = NULL;
    G722DEC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    G722D_AUDIODEC_PORT_TYPE *pCompPort = NULL;
    G722D_BUFFERLIST *pTemp = NULL;
    int i=0;

    G722DEC_DPRINT ("Entering OMX_ComponentInit\n");

    G722D_OMX_CONF_CHECK_CMD(pHandle,1,1)

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

    G722D_OMX_MALLOC(pHandle->pComponentPrivate,G722DEC_COMPONENT_PRIVATE);

    pComponentPrivate = pHandle->pComponentPrivate;
    pComponentPrivate->pHandle = pHandle;

    G722D_OMX_MALLOC(pCompPort, G722D_AUDIODEC_PORT_TYPE);
    pComponentPrivate->pCompPort[G722D_INPUT_PORT] =  pCompPort;

    G722D_OMX_MALLOC(pCompPort, G722D_AUDIODEC_PORT_TYPE);
    pComponentPrivate->pCompPort[G722D_OUTPUT_PORT] = pCompPort;
    G722D_OMX_MALLOC(pTemp, G722D_BUFFERLIST);
    pComponentPrivate->pInputBufferList = pTemp;

    G722D_OMX_MALLOC(pTemp, G722D_BUFFERLIST);
    pComponentPrivate->pOutputBufferList = pTemp;

    pComponentPrivate->pInputBufferList->numBuffers = 0;
    pComponentPrivate->pOutputBufferList->numBuffers = 0;

    for (i=0; i < MAX_NUM_OF_BUFS; i++) {
        pComponentPrivate->pInputBufferList->pBufHdr[i] = NULL;
        pComponentPrivate->pOutputBufferList->pBufHdr[i] = NULL;
        pComponentPrivate->arrTickCount[i] = 0; 
        pComponentPrivate->arrTimestamp[i] = 0; 
    } 
    pComponentPrivate->IpBufindex = 0; 
    pComponentPrivate->OpBufindex = 0;

    pComponentPrivate->bufAlloced = 0;

    G722D_OMX_MALLOC(pComponentPrivate->sPortParam, OMX_PORT_PARAM_TYPE);
    OMX_CONF_INIT_STRUCT(pComponentPrivate->sPortParam, OMX_PORT_PARAM_TYPE);
    G722D_OMX_MALLOC(pComponentPrivate->pPriorityMgmt, OMX_PRIORITYMGMTTYPE);
    OMX_CONF_INIT_STRUCT(pComponentPrivate->pPriorityMgmt, OMX_PRIORITYMGMTTYPE);
    pComponentPrivate->sPortParam->nPorts = 0x2;
    pComponentPrivate->sPortParam->nStartPortNumber = 0x0;
    pComponentPrivate->G722Params[G722D_INPUT_PORT] = NULL;
    pComponentPrivate->G722Params[G722D_OUTPUT_PORT] = NULL;

    G722D_OMX_MALLOC(G722_ip, OMX_AUDIO_PARAM_PCMMODETYPE);
    G722D_OMX_MALLOC(G722_op, OMX_AUDIO_PARAM_PCMMODETYPE);

    pComponentPrivate->G722Params[G722D_INPUT_PORT] = G722_ip;
    pComponentPrivate->G722Params[G722D_OUTPUT_PORT] = G722_op;

    pComponentPrivate->dasfmode = 0;
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

    /* Initialize device string to the default value */
    G722D_OMX_MALLOC_SIZE(pComponentPrivate->sDeviceString,(100*sizeof(OMX_STRING)),OMX_STRING);
    strcpy((char*)pComponentPrivate->sDeviceString,"/eteedn:i0:o0/codec\0");
    
    /* initialize role name */
    G722D_OMX_MALLOC(pComponentPrivate->componentRole,OMX_PARAM_COMPONENTROLETYPE);
    strcpy((char*)pComponentPrivate->componentRole->cRole, G722_DEC_ROLE);


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
        pComponentPrivate->arrTickCount[i] = 0; 
        pComponentPrivate->arrTimestamp[i] = 0; 
        pComponentPrivate->pOutBufHdrWhilePaused[i] = NULL;
    } 
    pComponentPrivate->IpBufindex = 0; 
    pComponentPrivate->OpBufindex = 0;

    pComponentPrivate->nPendingOutPausedBufs = 0;

    pComponentPrivate->nInvalidFrameCount = 0;
    pComponentPrivate->bDisableCommandPending = 0;

    pComponentPrivate->numPendingBuffers = 0;
    pComponentPrivate->bNoIdleOnStop= OMX_FALSE;
    pComponentPrivate->bDspStoppedWhileExecuting = OMX_FALSE;
    pComponentPrivate->bIdleCommandPending = OMX_FALSE;
    pComponentPrivate->nOutStandingFillDones = 0;
    G722D_OMX_MALLOC(pPortDef_ip, OMX_PARAM_PORTDEFINITIONTYPE);
    G722D_OMX_MALLOC(pPortDef_op, OMX_PARAM_PORTDEFINITIONTYPE);

    pComponentPrivate->pPortDef[G722D_INPUT_PORT] = pPortDef_ip;
    pComponentPrivate->pPortDef[G722D_OUTPUT_PORT] = pPortDef_op;

    /* Set input port defaults */
    pPortDef_ip->nSize                              = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
    pPortDef_ip->nPortIndex                         = G722D_INPUT_PORT;
    pPortDef_ip->eDir                               = OMX_DirInput;
    pPortDef_ip->nBufferCountActual                 = G722D_NUM_INPUT_BUFFERS;
    pPortDef_ip->nBufferCountMin                    = G722D_NUM_INPUT_BUFFERS;
    pPortDef_ip->nBufferSize                        = G722D_INPUT_BUFFER_SIZE;
    pPortDef_ip->bEnabled                           = OMX_TRUE;
    pPortDef_ip->bPopulated                         = OMX_FALSE;
    pPortDef_ip->eDomain                            = OMX_PortDomainAudio;
    pPortDef_ip->format.audio.eEncoding             = OMX_AUDIO_CodingPCM;
    pPortDef_ip->format.audio.cMIMEType             = NULL;
    pPortDef_ip->format.audio.pNativeRender         = NULL;
    pPortDef_ip->format.audio.bFlagErrorConcealment = OMX_FALSE;

    /* Set input port defaults */
    pPortDef_op->nSize                              = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
    pPortDef_op->nPortIndex                         = G722D_OUTPUT_PORT;
    pPortDef_op->eDir                               = OMX_DirOutput;
    pPortDef_op->nBufferCountMin                    = G722D_NUM_OUTPUT_BUFFERS;
    pPortDef_op->nBufferCountActual                 = G722D_NUM_OUTPUT_BUFFERS;
    pPortDef_op->nBufferSize                        = G722D_OUTPUT_BUFFER_SIZE;
    pPortDef_op->bEnabled                           = OMX_TRUE;
    pPortDef_op->bPopulated                         = OMX_FALSE;
    pPortDef_op->eDomain                            = OMX_PortDomainAudio;
    pPortDef_op->format.audio.eEncoding             = OMX_AUDIO_CodingPCM;
    pPortDef_op->format.audio.cMIMEType             = NULL;
    pPortDef_op->format.audio.pNativeRender         = NULL;
    pPortDef_op->format.audio.bFlagErrorConcealment = OMX_FALSE;

    G722D_OMX_MALLOC(pComponentPrivate->pCompPort[G722D_INPUT_PORT]->pPortFormat, 
                     OMX_AUDIO_PARAM_PORTFORMATTYPE);
    G722D_OMX_MALLOC(pComponentPrivate->pCompPort[G722D_OUTPUT_PORT]->pPortFormat, 
                     OMX_AUDIO_PARAM_PORTFORMATTYPE);
    OMX_CONF_INIT_STRUCT(pComponentPrivate->pCompPort[G722D_INPUT_PORT]->pPortFormat, 
                         OMX_AUDIO_PARAM_PORTFORMATTYPE);
    OMX_CONF_INIT_STRUCT(pComponentPrivate->pCompPort[G722D_OUTPUT_PORT]->pPortFormat, 
                         OMX_AUDIO_PARAM_PORTFORMATTYPE);


    /* Set input port format defaults */
    pPortFormat = pComponentPrivate->pCompPort[G722D_INPUT_PORT]->pPortFormat;
    OMX_CONF_INIT_STRUCT(pPortFormat, OMX_AUDIO_PARAM_PORTFORMATTYPE);
    pPortFormat->nPortIndex         = G722D_INPUT_PORT;
    pPortFormat->nIndex             = OMX_IndexParamAudioPcm;
    pPortFormat->eEncoding          = OMX_AUDIO_CodingPCM;

    /* Set output port format defaults */
    pPortFormat = pComponentPrivate->pCompPort[G722D_OUTPUT_PORT]->pPortFormat;
    OMX_CONF_INIT_STRUCT(pPortFormat, OMX_AUDIO_PARAM_PORTFORMATTYPE);
    pPortFormat->nPortIndex         = G722D_OUTPUT_PORT;
    pPortFormat->nIndex             = OMX_IndexParamAudioPcm;
    pPortFormat->eEncoding          = OMX_AUDIO_CodingPCM;

    /* Set input port parameters */
    OMX_CONF_INIT_STRUCT(G722_ip, OMX_AUDIO_PARAM_PCMMODETYPE);
    G722_ip->nPortIndex               = G722D_INPUT_PORT;
    G722_ip->nSize                    = sizeof(OMX_AUDIO_PARAM_PCMMODETYPE);
    G722_ip->nVersion.s.nVersionMajor = 0xF1;
    G722_ip->nVersion.s.nVersionMinor = 0xF2;
    G722_ip->nChannels                = 1; /* mono */
    G722_ip->nBitPerSample            = 8;
    G722_ip->eNumData                 = OMX_NumericalDataUnsigned;
    G722_ip->eEndian                  = OMX_EndianLittle;
    G722_ip->bInterleaved             = OMX_FALSE;
    G722_ip->ePCMMode                 = OMX_AUDIO_PCMModeLinear;
    G722_ip->nSamplingRate            = 64000;

    /* Set output port parameters */
    OMX_CONF_INIT_STRUCT(G722_op, OMX_AUDIO_PARAM_PCMMODETYPE);
    G722_op->nPortIndex               = G722D_OUTPUT_PORT;
    G722_op->nSamplingRate            = 16000;
    G722_op->nChannels                = OMX_AUDIO_ChannelModeMono;
    G722_op->nSize                    = sizeof(OMX_AUDIO_PARAM_PCMMODETYPE);
    G722_op->nVersion.s.nVersionMajor = 0xF1;
    G722_op->nVersion.s.nVersionMinor = 0xF2;
    G722_op->nBitPerSample            = 16;
    G722_op->eNumData                 = OMX_NumericalDataUnsigned;
    G722_op->eEndian                  = OMX_EndianLittle;
    G722_op->bInterleaved             = OMX_FALSE;
    G722_op->ePCMMode                 = OMX_AUDIO_PCMModeLinear;

    pComponentPrivate->bBufferIsAllocated = 1;
    pComponentPrivate->bPortDefsAllocated = 1;

#ifdef DSP_RENDERING_ON
    if((G722d_fdwrite=open(FIFO1,O_WRONLY))<0) {
        G722DEC_DPRINT("[G722 Component] - failure to open WRITE pipe\n");
        eError = OMX_ErrorHardware;
    }
    if((G722d_fdread=open(FIFO2,O_RDONLY))<0) {
        G722DEC_DPRINT("[G722 Component] - failure to open READ pipe\n");
        eError = OMX_ErrorHardware;
    }
#endif

    eError = G722Dec_StartCompThread(pHandle);
    if (eError != OMX_ErrorNone) {
        G722DEC_DPRINT ("Error returned from the Component\n");
        goto EXIT;
    }

 EXIT:
    if(OMX_ErrorNone != eError) {
        G722DEC_DPRINT(":: ************* ERROR: Freeing Other Malloced Resources\n");
        G722D_OMX_FREE(pPortDef_ip);
        G722D_OMX_FREE(pPortDef_op);
        G722D_OMX_FREE(G722_ip);
        G722D_OMX_FREE(G722_op);
        G722D_OMX_FREE(pTemp);
    }
    G722DEC_DPRINT ("Exiting OMX_ComponentInit\n");
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
 * @param G722dData This is command data that came with command.
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
    G722DEC_COMPONENT_PRIVATE *pCompPrivate = NULL;

    G722D_OMX_CONF_CHECK_CMD(pHandle,1,1);
    pCompPrivate = (G722DEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;

    G722DEC_DPRINT("G722DEC: Entered SendCommand\n");
    if(pCompPrivate->curState == OMX_StateInvalid){
        G722DEC_DPRINT("G722DEC: Error Notofication Sent to App\n");
        G722D_OMX_ERROR_EXIT(eError, OMX_ErrorInvalidState,"OMX_ErrorInvalidState");
    }

    switch(Cmd) {
    case OMX_CommandStateSet:
        G722DEC_DPRINT("G722DEC: Entered switch - Command State Set\n");
        if (nParam == OMX_StateLoaded) {
            pCompPrivate->bLoadedCommandPending = OMX_TRUE;
        }
        if(pCompPrivate->curState == OMX_StateLoaded) {
            G722DEC_DPRINT("G722DEC: Entered switch - curState == OMX_StateLoaded\n");
            if((nParam == OMX_StateExecuting) || (nParam == OMX_StatePause)) {
                G722DEC_DPRINT("G722DEC: Entered switch - nParam == StatExecuting \
|| OMX_StatePause\n");
                pCompPrivate->cbInfo.EventHandler (pHandle,
                                                   pHandle->pApplicationPrivate,
                                                   OMX_EventError,
                                                   OMX_ErrorIncorrectStateTransition,
                                                   0,
                                                   NULL);
                G722DEC_DPRINT("Incorrect St Tr fm Loaded to Executing By App\n");
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
                G722DEC_DPRINT("Incorrect State Tr from Loaded to Invalid by Application\n");
                goto EXIT;
            }
        }
        break;
    case OMX_CommandFlush:
        G722DEC_DPRINT("G722DEC: Entered switch - Command Flush\n");
        if(nParam > 1 && nParam != -1) {
            G722D_OMX_ERROR_EXIT(eError,OMX_ErrorBadPortIndex,"OMX_ErrorBadPortIndex");
        }
        break;
    case OMX_CommandPortDisable:
        break;
    case OMX_CommandPortEnable:
        G722DEC_DPRINT("G722DEC: Entered switch - Command Port Enable/Disbale\n");
        break;
    case OMX_CommandMarkBuffer:
        G722DEC_DPRINT("G722DEC: Entered switch - Command Mark Buffer\n");

        if(nParam > 0) {
            G722D_OMX_ERROR_EXIT(eError,OMX_ErrorBadPortIndex,"OMX_ErrorBadPortIndex");
        }
        break;
    default:
        G722DEC_DPRINT("G722DEC: Command Received Default error\n");
        pCompPrivate->cbInfo.EventHandler(pHandle, pHandle->pApplicationPrivate,
                                          OMX_EventError,
                                          OMX_ErrorUndefined,0,
                                          "Invalid Command");
        break;
    }


    nRet = write (pCompPrivate->cmdPipe[1], &Cmd, sizeof(Cmd));
    if (nRet == -1) {
        G722D_OMX_ERROR_EXIT(eError,OMX_ErrorHardware,"write failed: OMX_ErrorHardware");
    }

    if (Cmd == OMX_CommandMarkBuffer) {
        nRet = write (pCompPrivate->cmdDataPipe[1], &pCmdData,
                      sizeof(OMX_PTR));
        if (nRet == -1) {
            G722D_OMX_ERROR_EXIT(eError,OMX_ErrorHardware,"write failed: OMX_ErrorHardware");
        }
    }
    else {
        nRet = write (pCompPrivate->cmdDataPipe[1], &nParam,
                      sizeof(OMX_U32));
        if (nRet == -1) {
            G722D_OMX_ERROR_EXIT(eError,OMX_ErrorHardware,"write failed: OMX_ErrorHardware");
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

            if((write(G722d_fdwrite, &cmd_data, sizeof(cmd_data)))<0)
                G722DEC_DPRINT("[G722 decoder] - fail to send command to audio manager\n");

            /* enable DL EQ for ACDN0 */
            cmd_data.hComponent = pHandle;
            cmd_data.AM_Cmd = AM_CommandACDN0EQDLLMode;
            cmd_data.param1 = EEQ_DLENABLE;  /* mode  */
            cmd_data.param2 = 0;
            cmd_data.streamID = 0;

            if((write(G722d_fdwrite, &cmd_data, sizeof(cmd_data)))<0)
                G722DEC_DPRINT("[G722 decoder] - fail to send command to audio manager\n");
        }else {
            /* enable Tee device command*/
            cmd_data.hComponent = pHandle;
            cmd_data.AM_Cmd = AM_CommandTDNDownlinkMode;
            cmd_data.param1 = 0;
            cmd_data.param2 = 0;
            cmd_data.streamID = 0;

            if((write(G722d_fdwrite, &cmd_data, sizeof(cmd_data)))<0) {
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
    G722DEC_COMPONENT_PRIVATE  *pComponentPrivate = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE *pParameterStructure = NULL;


    G722DEC_DPRINT("Inside the GetParameter:: %x\n",nParamIndex);

    G722D_OMX_CONF_CHECK_CMD(hComp,1,1);
    pComponentPrivate = (G722DEC_COMPONENT_PRIVATE *)(((OMX_COMPONENTTYPE*)hComp)->pComponentPrivate);

    G722D_OMX_CONF_CHECK_CMD(pComponentPrivate, ComponentParameterStructure, 1);
    pParameterStructure = (OMX_PARAM_PORTDEFINITIONTYPE*)ComponentParameterStructure;

    G722DEC_DPRINT(":: Entering the GetParameter\n");

    if(pComponentPrivate->curState == OMX_StateInvalid) {
        eError = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }

    switch(nParamIndex){
    case OMX_IndexParamAudioInit:
        G722DEC_DPRINT(":: GetParameter OMX_IndexParamAudioInit\n");
        G722D_OMX_CONF_CHECK_CMD(pComponentPrivate->sPortParam, 1, 1);
        memcpy(ComponentParameterStructure, pComponentPrivate->sPortParam, sizeof(OMX_PORT_PARAM_TYPE));
        break;

    case OMX_IndexParamPortDefinition:
        G722DEC_DPRINT(": GetParameter OMX_IndexParamPortDefinition \n");
        if(((OMX_PARAM_PORTDEFINITIONTYPE *)(ComponentParameterStructure))->nPortIndex ==
           pComponentPrivate->pPortDef[G722D_INPUT_PORT]->nPortIndex) {
            memcpy(ComponentParameterStructure, pComponentPrivate->pPortDef[G722D_INPUT_PORT], 
                   sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
        } else if(((OMX_PARAM_PORTDEFINITIONTYPE *)(ComponentParameterStructure))->nPortIndex ==
                  pComponentPrivate->pPortDef[G722D_OUTPUT_PORT]->nPortIndex) {
            memcpy(ComponentParameterStructure, pComponentPrivate->pPortDef[G722D_OUTPUT_PORT], 
                   sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
        } else {
            G722DEC_DPRINT(":: OMX_ErrorBadPortIndex from GetParameter \n");
            eError = OMX_ErrorBadPortIndex;
        }
        break;

    case OMX_IndexParamAudioPortFormat:
        G722DEC_DPRINT(":: GetParameter OMX_IndexParamAudioPortFormat \n");
        if(((OMX_AUDIO_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nPortIndex ==
           pComponentPrivate->pPortDef[G722D_INPUT_PORT]->nPortIndex) {
            if(((OMX_AUDIO_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nIndex >
               pComponentPrivate->pCompPort[G722D_INPUT_PORT]->pPortFormat->nPortIndex) {
                eError = OMX_ErrorNoMore;
            }
            else {
                memcpy(ComponentParameterStructure, pComponentPrivate->pCompPort[G722D_INPUT_PORT]->pPortFormat, 
                       sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
            }
        }
        else if(((OMX_AUDIO_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nPortIndex ==
                pComponentPrivate->pPortDef[G722D_OUTPUT_PORT]->nPortIndex){
            if(((OMX_AUDIO_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nIndex >
               pComponentPrivate->pCompPort[G722D_OUTPUT_PORT]->pPortFormat->nPortIndex) {
                eError = OMX_ErrorNoMore;
            }
            else {
                memcpy(ComponentParameterStructure, pComponentPrivate->pCompPort[G722D_OUTPUT_PORT]->pPortFormat, 
                       sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
            }
        }
        else {
            G722DEC_DPRINT(":: OMX_ErrorBadPortIndex from GetParameter \n");
            eError = OMX_ErrorBadPortIndex;
        }
        break;

    case OMX_IndexParamAudioPcm:
        G722DEC_DPRINT(" :: GetParameter OMX_IndexParamAudioPcm \n");
        if(((OMX_AUDIO_PARAM_PCMMODETYPE *)(ComponentParameterStructure))->nPortIndex ==
           pComponentPrivate->G722Params[G722D_INPUT_PORT]->nPortIndex) {
            memcpy(ComponentParameterStructure, pComponentPrivate->G722Params[G722D_INPUT_PORT], 
                   sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
        } else if(((OMX_AUDIO_PARAM_PCMMODETYPE *)(ComponentParameterStructure))->nPortIndex ==
                  pComponentPrivate->G722Params[G722D_OUTPUT_PORT]->nPortIndex) {
            memcpy(ComponentParameterStructure, pComponentPrivate->G722Params[G722D_OUTPUT_PORT], 
                   sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
        } else {
            G722DEC_DPRINT(" :: OMX_ErrorBadPortIndex from GetParameter \n");
            eError = OMX_ErrorBadPortIndex;
        }
        break;

    case OMX_IndexParamPriorityMgmt:
        G722DEC_DPRINT(" :: GetParameter OMX_IndexParamPriorityMgmt \n");
        G722D_OMX_CONF_CHECK_CMD(pComponentPrivate->pPriorityMgmt, 1, 1);
        memcpy(ComponentParameterStructure, pComponentPrivate->pPriorityMgmt, sizeof(OMX_PRIORITYMGMTTYPE));
        break;

    case OMX_IndexParamVideoInit:
        break;
        
    case OMX_IndexParamImageInit:
        break;
        
    case OMX_IndexParamOtherInit:
        break;
    
    default:
        G722DEC_DPRINT(" :: OMX_ErrorUnsupportedIndex GetParameter \n");
        eError = OMX_ErrorUnsupportedIndex;
        break;
    }
 EXIT:
    G722DEC_DPRINT(" :: Exiting GetParameter\n");
    G722DEC_DPRINT(" :: Returning = 0x%x\n",eError);
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
    G722DEC_COMPONENT_PRIVATE  *pComponentPrivate = NULL;
    OMX_AUDIO_PARAM_PORTFORMATTYPE* pComponentParam = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE *pComponentParamPort = NULL;
    OMX_AUDIO_PARAM_PCMMODETYPE *pCompG722Param = NULL;
    OMX_PARAM_COMPONENTROLETYPE  *pRole = NULL;
    OMX_PARAM_BUFFERSUPPLIERTYPE sBufferSupplier;

    G722D_OMX_CONF_CHECK_CMD(hComponent,1,1)

        pComponentPrivate = (G722DEC_COMPONENT_PRIVATE *)(((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);
    G722D_OMX_CONF_CHECK_CMD(pComponentPrivate, pCompParam, 1);


    G722DEC_DPRINT(" :: Entering the SetParameter\n");
    if (pComponentPrivate->curState != OMX_StateLoaded) {
        eError = OMX_ErrorIncorrectStateOperation;
        G722DEC_DPRINT(" :: OMX_ErrorIncorrectStateOperation from SetParameter");
        goto EXIT;
    }

    switch(nIndex) {
    case OMX_IndexParamAudioPortFormat:
        G722DEC_DPRINT(":: SetParameter OMX_IndexParamAudioPortFormat \n");
        pComponentParam = (OMX_AUDIO_PARAM_PORTFORMATTYPE *)pCompParam;
        if ( pComponentParam->nPortIndex == pComponentPrivate->pCompPort[G722D_INPUT_PORT]->pPortFormat->nPortIndex ) {
            memcpy(pComponentPrivate->pCompPort[G722D_INPUT_PORT]->pPortFormat, 
                   pComponentParam, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
        } else if ( pComponentParam->nPortIndex == pComponentPrivate->pCompPort[G722D_OUTPUT_PORT]->pPortFormat->nPortIndex ) {
            memcpy(pComponentPrivate->pCompPort[G722D_OUTPUT_PORT]->pPortFormat, 
                   pComponentParam, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
        } else {
            G722DEC_DPRINT(":: OMX_ErrorBadPortIndex from SetParameter");
            eError = OMX_ErrorBadPortIndex;
        }
        break;

    case OMX_IndexParamAudioPcm:
        G722DEC_DPRINT(" :: SetParameter OMX_IndexParamAudioPcm \n");
        pCompG722Param = (OMX_AUDIO_PARAM_PCMMODETYPE *)pCompParam;

        /* This if condition is not yet well defined because khronos
         * test suite does not set the sampling frequency. For component
         * test application, it is meant to test that component returns
         * the error on invalid frequecy *
         if(pCompG722Param->nSamplingRate == INVALID_SAMPLING_FREQ) {
         eError = OMX_ErrorUnsupportedIndex;
         G722DEC_DPRINT("Unsupported SampleRate Given By the App\n");
         goto EXIT;
         }
        *****/
        
        if(pCompG722Param->nPortIndex == G722D_INPUT_PORT) { /* means Input port */
            memcpy(((G722DEC_COMPONENT_PRIVATE*)
                    pHandle->pComponentPrivate)->G722Params[G722D_INPUT_PORT], pCompG722Param, 
                   sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
        } else if (pCompG722Param->nPortIndex == G722D_OUTPUT_PORT) { /* means Output port */
            memcpy(((G722DEC_COMPONENT_PRIVATE *)
                    pHandle->pComponentPrivate)->G722Params[G722D_OUTPUT_PORT], pCompG722Param, 
                   sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
        }
        else {
            G722DEC_DPRINT(":: OMX_ErrorBadPortIndex from SetParameter");
            eError = OMX_ErrorBadPortIndex;
        }
        break;

    case OMX_IndexParamPortDefinition:
        pComponentParamPort = (OMX_PARAM_PORTDEFINITIONTYPE *)pCompParam;
        if (pComponentParamPort->nPortIndex == 0) {
            if (pComponentParamPort->eDir != OMX_DirInput) {
                G722DEC_DPRINT(":: Invalid input buffer Direction\n");
                eError = OMX_ErrorBadParameter;
                goto EXIT;
            }
            if (pComponentParamPort->format.audio.eEncoding != OMX_AUDIO_CodingPCM) {
                G722DEC_DPRINT(":: Invalid format Parameter\n");
                eError = OMX_ErrorBadParameter;
                goto EXIT;
            }
        } else if (pComponentParamPort->nPortIndex == 1) {
            if (pComponentParamPort->eDir != OMX_DirOutput) {
                G722DEC_DPRINT(" :: Invalid Output buffer Direction\n");
                eError = OMX_ErrorBadParameter;
                goto EXIT;
            }
            if (pComponentParamPort->format.audio.eEncoding != OMX_AUDIO_CodingPCM) {
                G722DEC_DPRINT(":: Invalid format Parameter\n");
                eError = OMX_ErrorBadParameter;
                goto EXIT;
            }
        } else {
            G722DEC_DPRINT(":: OMX_ErrorBadPortIndex from SetParameter");
            eError = OMX_ErrorBadPortIndex;
        }
        G722DEC_DPRINT(":: SetParameter OMX_IndexParamPortDefinition \n");
        if(((OMX_PARAM_PORTDEFINITIONTYPE *)(pCompParam))->nPortIndex ==
           pComponentPrivate->pPortDef[G722D_INPUT_PORT]->nPortIndex) {
            G722DEC_DPRINT(":: SetParameter OMX_IndexParamPortDefinition input \n");
            memcpy(pComponentPrivate->pPortDef[G722D_INPUT_PORT], pCompParam,
                   sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
        }
        else if(((OMX_PARAM_PORTDEFINITIONTYPE *)(pCompParam))->nPortIndex ==
                pComponentPrivate->pPortDef[G722D_OUTPUT_PORT]->nPortIndex) {
            G722DEC_DPRINT(":: SetParameter OMX_IndexParamPortDefinition output\n");
            memcpy(pComponentPrivate->pPortDef[G722D_OUTPUT_PORT], pCompParam, 
                   sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
        }
        else {
            G722DEC_DPRINT(":: OMX_ErrorBadPortIndex from SetParameter");
            eError = OMX_ErrorBadPortIndex;
        }
        break;

    case OMX_IndexParamPriorityMgmt:
        G722DEC_DPRINT(":: SetParameter OMX_IndexParamPriorityMgmt \n");
        G722D_OMX_CONF_CHECK_CMD(pComponentPrivate->pPriorityMgmt, 1, 1);
        memcpy(pComponentPrivate->pPriorityMgmt, (OMX_PRIORITYMGMTTYPE*)pCompParam, 
               sizeof(OMX_PRIORITYMGMTTYPE));
        break;

    case OMX_IndexParamAudioInit:
        G722DEC_DPRINT(":: SetParameter OMX_IndexParamAudioInit \n");
        G722D_OMX_CONF_CHECK_CMD(pComponentPrivate->sPortParam, 1, 1);
        memcpy(pComponentPrivate->sPortParam, (OMX_PORT_PARAM_TYPE*)pCompParam, 
               sizeof(OMX_PORT_PARAM_TYPE));
        break;

    case OMX_IndexParamStandardComponentRole:
        if (pCompParam) {
            pRole = (OMX_PARAM_COMPONENTROLETYPE *)pCompParam;
            G722D_OMX_CONF_CHECK_CMD(pComponentPrivate->componentRole, 1, 1);
            memcpy(pComponentPrivate->componentRole, (void *)pRole, sizeof(OMX_PARAM_COMPONENTROLETYPE));
        } else {
            eError = OMX_ErrorBadParameter;
        }
        break;

    case OMX_IndexParamCompBufferSupplier:
        if(((OMX_PARAM_PORTDEFINITIONTYPE *)(pCompParam))->nPortIndex ==
           pComponentPrivate->pPortDef[G722D_INPUT_PORT]->nPortIndex) {
            G722DEC_DPRINT(":: SetParameter OMX_IndexParamCompBufferSupplier \n");
            sBufferSupplier.eBufferSupplier = OMX_BufferSupplyInput;
            memcpy(&sBufferSupplier, pCompParam, sizeof(OMX_PARAM_BUFFERSUPPLIERTYPE));                                
                    
        }
        else if(((OMX_PARAM_PORTDEFINITIONTYPE *)(pCompParam))->nPortIndex ==
                pComponentPrivate->pPortDef[G722D_OUTPUT_PORT]->nPortIndex) {
            G722DEC_DPRINT(":: SetParameter OMX_IndexParamCompBufferSupplier \n");
            sBufferSupplier.eBufferSupplier = OMX_BufferSupplyOutput;
            memcpy(&sBufferSupplier, pCompParam, sizeof(OMX_PARAM_BUFFERSUPPLIERTYPE));
        } 
        else {
            G722DEC_DPRINT(":: OMX_ErrorBadPortIndex from SetParameter");
            eError = OMX_ErrorBadPortIndex;
        }
        break;

    default:
        G722DEC_DPRINT(":: SetParameter OMX_ErrorUnsupportedIndex \n");
        eError = OMX_ErrorUnsupportedIndex;
        break;
    }
     
 EXIT:
    G722DEC_DPRINT(":: Exiting SetParameter\n");
    G722DEC_DPRINT(":: Returning = 0x%x\n",eError);
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
    G722DEC_COMPONENT_PRIVATE *pComponentPrivate = NULL;


    G722D_OMX_CONF_CHECK_CMD(pHandle,1,1)

        pComponentPrivate = (G722DEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    G722D_OMX_CONF_CHECK_CMD(pComponentPrivate,1,1)

        G722DEC_DPRINT ("Entering SetCallbacks\n");

    G722D_OMX_CONF_CHECK_CMD(pCallBacks, pCallBacks->EventHandler, pCallBacks->EmptyBufferDone)
        G722D_OMX_CONF_CHECK_CMD(pCallBacks->FillBufferDone, 1, 1)

        memcpy (&(pComponentPrivate->cbInfo), pCallBacks, sizeof(OMX_CALLBACKTYPE));
    pHandle->pApplicationPrivate = pAppData;
    G722DEC_STATEPRINT("****************** Component State Set to Loaded\n\n");
    pComponentPrivate->curState = OMX_StateLoaded;

 EXIT:
    G722DEC_DPRINT ("Exiting SetCallbacks\n");
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
    G722DEC_COMPONENT_PRIVATE *pComponentPrivate = 
        (G722DEC_COMPONENT_PRIVATE *) pHandle->pComponentPrivate;

    /* Copy component version structure */
    if(pComponentVersion != NULL && pComponentName != NULL) {
        strcpy(pComponentName, pComponentPrivate->cComponentName);
        memcpy(pComponentVersion, &(pComponentPrivate->ComponentVersion.s), 
               sizeof(pComponentPrivate->ComponentVersion.s));
    }
    else {
        G722DEC_DPRINT("%d :: OMX_ErrorBadParameter from GetComponentVersion",__LINE__);
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
    G722DEC_DPRINT ("Entering GetConfig\n");
    G722DEC_DPRINT ("Inside   GetConfig\n");
    G722DEC_DPRINT ("Exiting  GetConfig\n");
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
    G722DEC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    OMX_AUDIO_CONFIG_MUTETYPE *pMuteStructure = NULL;
    OMX_AUDIO_CONFIG_VOLUMETYPE *pVolumeStructure = NULL;
    TI_OMX_DSP_DEFINITION* pDspDefinition = NULL;
    OMX_S16* deviceString = NULL;
    TI_OMX_DATAPATH dataPath;

    G722DEC_DPRINT ("Entering SetConfig\n");

    G722D_OMX_CONF_CHECK_CMD(hComp,ComponentConfigStructure,1)

        pComponentPrivate = (G722DEC_COMPONENT_PRIVATE *)
        (((OMX_COMPONENTTYPE*)hComp)->pComponentPrivate);

    G722D_OMX_CONF_CHECK_CMD(pComponentPrivate,1,1)

        switch (nConfigIndex){
        case OMX_IndexCustomG722DecHeaderInfoConfig:
            pDspDefinition = (TI_OMX_DSP_DEFINITION *)ComponentConfigStructure;
            pComponentPrivate->dasfmode = pDspDefinition->dasfMode;
            pComponentPrivate->streamID = pDspDefinition->streamId;
            break;

            /* set mute/unmute for playback stream */
        case OMX_IndexConfigAudioMute:
#ifdef DSP_RENDERING_ON
            pMuteStructure = (OMX_AUDIO_CONFIG_MUTETYPE *)ComponentConfigStructure;
            printf("Set Mute/Unmute for playback stream\n");
            cmd_data.hComponent = hComp;
            if(pMuteStructure->bMute == OMX_TRUE){
                printf("Mute the playback stream\n");
                cmd_data.AM_Cmd = AM_CommandStreamMute;
            }
            else{
                printf("unMute the playback stream\n");
                cmd_data.AM_Cmd = AM_CommandStreamUnMute;
            }
            cmd_data.param1 = 0;
            cmd_data.param2 = 0;
            cmd_data.streamID = pComponentPrivate->streamID;

            if((write(G722d_fdwrite, &cmd_data, sizeof(cmd_data)))<0){
                G722DEC_DPRINT("[G722 decoder] - fail to send command to audio manager\n");
            }
#endif
            break;

            /* set volume for playback stream */
        case OMX_IndexConfigAudioVolume:
#ifdef DSP_RENDERING_ON
            pVolumeStructure = (OMX_AUDIO_CONFIG_VOLUMETYPE *)ComponentConfigStructure;
            printf("Set volume for playback stream\n");
            cmd_data.hComponent = hComp;
            cmd_data.AM_Cmd = AM_CommandSWGain;
            cmd_data.param1 = pVolumeStructure->sVolume.nValue;
            cmd_data.param2 = 0;
            cmd_data.streamID = pComponentPrivate->streamID;

            if((write(G722d_fdwrite, &cmd_data, sizeof(cmd_data)))<0){
                G722DEC_DPRINT("[G722 decoder] - fail to send command to audio manager\n");
            }
#endif
            break;

        case  OMX_IndexCustomG722DecDataPath:
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

    G722DEC_DPRINT ("Exiting  SetConfig\n");
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

    G722DEC_DPRINT ("Entering GetState\n");

    if (!pState) {
        G722DEC_DPRINT (":: About to exit with bad parameter\n");
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }

    G722D_OMX_CONF_CHECK_CMD(pHandle,1,1)
        if (pHandle && pHandle->pComponentPrivate) {
            *pState =  ((G722DEC_COMPONENT_PRIVATE*)
                        pHandle->pComponentPrivate)->curState;
        } else {
            G722DEC_STATEPRINT("Component State Set to Loaded\n\n");
            *pState = OMX_StateLoaded;
        }

    eError = OMX_ErrorNone;

 EXIT:
    G722DEC_DPRINT (":: Exiting GetState\n");
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
    G722DEC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef = NULL;
    int ret=0;

    G722D_OMX_CONF_CHECK_CMD(pHandle,1,1)

        pComponentPrivate = (G722DEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;

    G722D_OMX_CONF_CHECK_CMD(pComponentPrivate,1,1)

        pPortDef = ((G722DEC_COMPONENT_PRIVATE*)
                    pComponentPrivate)->pPortDef[G722D_INPUT_PORT];


    if(!pPortDef->bEnabled) {
        G722D_OMX_ERROR_EXIT(eError,OMX_ErrorIncorrectStateOperation,
                             "OMX_ErrorIncorrectStateOperation");
    }

    if (pBuffer == NULL) {
        G722D_OMX_ERROR_EXIT(eError,OMX_ErrorBadParameter,"OMX_ErrorBadParameter");
    }

    if (pBuffer->nSize != sizeof(OMX_BUFFERHEADERTYPE)) {
        G722DEC_DPRINT(":: Error: Bad Size = %ld, Add: %p\n",
                       pBuffer->nSize,pBuffer);
        G722D_OMX_ERROR_EXIT(eError,OMX_ErrorBadParameter,"Bad Size");
    } 

    if (pBuffer->nInputPortIndex != G722D_INPUT_PORT) {
        G722D_OMX_ERROR_EXIT(eError,OMX_ErrorBadPortIndex,"OMX_ErrorBadPortIndex");
    }


    if (pBuffer->nVersion.nVersion != pComponentPrivate->nVersion) {
        G722D_OMX_ERROR_EXIT(eError,OMX_ErrorVersionMismatch,"OMX_ErrorVersionMismatch");
    }


    if(pComponentPrivate->curState != OMX_StateExecuting && 
       pComponentPrivate->curState != OMX_StatePause && 
       pComponentPrivate->curState != OMX_StateIdle) {
        G722D_OMX_ERROR_EXIT(eError,OMX_ErrorIncorrectStateOperation,
                             "OMX_ErrorIncorrectStateOperation");
    }


    G722DEC_DPRINT("\n------------------------------------------\n\n");
    G722DEC_DPRINT (":: Component Sending Filled ip buff %p \
                             to Component Thread\n",pBuffer);
    G722DEC_DPRINT("\n------------------------------------------\n\n");

    if (pComponentPrivate->bBypassDSP == 0) {
        pComponentPrivate->app_nBuf--;
    }

    pComponentPrivate->pMarkData = pBuffer->pMarkData;
    pComponentPrivate->hMarkTargetComponent = pBuffer->hMarkTargetComponent;

    ret = write (pComponentPrivate->dataPipe[1], &pBuffer,
                 sizeof(OMX_BUFFERHEADERTYPE*));
    if (ret == -1) {
        G722D_OMX_ERROR_EXIT(eError,OMX_ErrorHardware,"write failed: OMX_ErrorHardware");
    }

    pComponentPrivate->nEmptyThisBufferCount++;

 EXIT:
    G722DEC_DPRINT (":: Exiting EmptyThisBuffer, eError = %d\n",eError);
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
    G722DEC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    int nRet=0;
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef = NULL;

    G722DEC_DPRINT("\n------------------------------------------\n\n");
    G722DEC_DPRINT (" :: Component Sending Emptied op buff %p \
                             to Component Thread\n",pBuffer);
    G722DEC_DPRINT("\n------------------------------------------\n\n");


    G722D_OMX_CONF_CHECK_CMD(pHandle,1,1)

        pComponentPrivate = (G722DEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;

    G722D_OMX_CONF_CHECK_CMD(pComponentPrivate,1,1)


        pPortDef = ((G722DEC_COMPONENT_PRIVATE*)
                    pComponentPrivate)->pPortDef[G722D_OUTPUT_PORT];

    if(!pPortDef->bEnabled) {
        G722D_OMX_ERROR_EXIT(eError,OMX_ErrorIncorrectStateOperation,
                             "OMX_ErrorIncorrectStateOperation");
    }

    if (pBuffer == NULL) {
        G722D_OMX_ERROR_EXIT(eError,OMX_ErrorBadParameter,"OMX_ErrorBadParameter");
    }

    if (pBuffer->nSize != sizeof(OMX_BUFFERHEADERTYPE)) {
        G722D_OMX_ERROR_EXIT(eError,OMX_ErrorBadParameter,"OMX_ErrorBadParameter");
    }

    if (pBuffer->nOutputPortIndex != G722D_OUTPUT_PORT) {
        G722D_OMX_ERROR_EXIT(eError,OMX_ErrorBadPortIndex,"OMX_ErrorBadPortIndex");
    }

    G722DEC_DPRINT("::pBuffer->nVersion.nVersion:%ld\n",pBuffer->nVersion.nVersion);
    G722DEC_DPRINT("::pComponentPrivate->nVersion:%ld\n",pComponentPrivate->nVersion);
    if (pBuffer->nVersion.nVersion != pComponentPrivate->nVersion) {
        G722D_OMX_ERROR_EXIT(eError,OMX_ErrorVersionMismatch,"OMX_ErrorVersionMismatch");
    }

    if(pComponentPrivate->curState != OMX_StateExecuting && 
       pComponentPrivate->curState != OMX_StatePause &&
       pComponentPrivate->curState != OMX_StateIdle) {
        G722D_OMX_ERROR_EXIT(eError,OMX_ErrorIncorrectStateOperation,
                             "OMX_ErrorIncorrectStateOperation");
    }


    pBuffer->nFilledLen = 0;

    if (pComponentPrivate->bBypassDSP == 0) {
        pComponentPrivate->app_nBuf--;
    }

    if(pComponentPrivate->pMarkBuf){
        G722DEC_DPRINT("FillThisBuffer Line\n");
        pBuffer->hMarkTargetComponent = pComponentPrivate->pMarkBuf->hMarkTargetComponent;
        pBuffer->pMarkData = pComponentPrivate->pMarkBuf->pMarkData;
        pComponentPrivate->pMarkBuf = NULL;
    }

    if (pComponentPrivate->pMarkData) {
        G722DEC_DPRINT("FillThisBuffer Line\n");
        pBuffer->hMarkTargetComponent = pComponentPrivate->hMarkTargetComponent;
        pBuffer->pMarkData = pComponentPrivate->pMarkData;
        pComponentPrivate->pMarkData = NULL;
    }

    nRet = write (pComponentPrivate->dataPipe[1], &pBuffer,
                  sizeof (OMX_BUFFERHEADERTYPE*));
    if (nRet == -1) {
        G722D_OMX_ERROR_EXIT(eError,OMX_ErrorHardware,"write failed: OMX_ErrorHardware");
    }

    pComponentPrivate->nFillThisBufferCount++;

 EXIT:
    G722DEC_DPRINT (":: Exiting FillThisBuffer\n");
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
    G722DEC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    OMX_ERRORTYPE threadError = OMX_ErrorNone;
    int pthreadError = 0;

    G722DEC_DPRINT("ComponentDeInit\n");
    G722D_OMX_CONF_CHECK_CMD(pComponent,1,1)
        pComponentPrivate = (G722DEC_COMPONENT_PRIVATE *)pComponent->pComponentPrivate;
    G722D_OMX_CONF_CHECK_CMD(pComponentPrivate,1,1)

#ifdef DSP_RENDERING_ON
        close(G722d_fdwrite);
    close(G722d_fdread);
#endif

    pComponentPrivate->bExitCompThrd = 1;
    
    pthreadError = pthread_join(pComponentPrivate->ComponentThread, (void*)&threadError);
    if(0 != pthreadError) {
        G722DEC_DPRINT(":: First Error in ComponentDeinit: From pthread_join\n");
        eError = OMX_ErrorHardware;
        goto EXIT;
    }

    if (OMX_ErrorNone != threadError && OMX_ErrorNone != eError) {
        eError = OMX_ErrorInsufficientResources;
        G722DEC_DPRINT("%d :: Error while closing Component Thread\n",__LINE__);
        goto EXIT;
    }

    eError1 = G722DEC_FreeCompResources(pHandle);
    if (OMX_ErrorNone != eError1) {
        if (OMX_ErrorNone == eError) {
            G722DEC_DPRINT(":: First Error in ComponentDeinit: From FreeCompResources\n");
            eError = eError1;
        }
    }

    if (pComponentPrivate->sDeviceString != NULL) {
        G722D_OMX_FREE(pComponentPrivate->sDeviceString);
    }

    G722DEC_MEMPRINT(":: Freeing: pComponentPrivate = %p\n", pComponentPrivate);
    G722D_OMX_FREE(pComponentPrivate);
    G722DEC_DPRINT("::*********** ComponentDeinit is Done************** \n");

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
    G722DEC_DPRINT (":: Entering ComponentTunnelRequest\n");
    G722DEC_DPRINT (":: Inside   ComponentTunnelRequest\n");
    eError = OMX_ErrorNotImplemented;
    G722DEC_DPRINT (":: Exiting ComponentTunnelRequest\n");
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
    G722DEC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_BUFFERHEADERTYPE *pBufferHeader = NULL;


    G722DEC_DPRINT (":: Entering AllocateBuffer\n");
    G722D_OMX_CONF_CHECK_CMD(hComponent,1,1)
        pComponentPrivate = (G722DEC_COMPONENT_PRIVATE *)
        (((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);

    G722D_OMX_CONF_CHECK_CMD(pComponentPrivate, 1, 1)

        pPortDef = ((G722DEC_COMPONENT_PRIVATE*)
                    pComponentPrivate)->pPortDef[nPortIndex];

    G722D_OMX_CONF_CHECK_CMD(pPortDef, 1, 1)

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

    G722D_OMX_MALLOC(pBufferHeader, OMX_BUFFERHEADERTYPE);

    /* Needed for cache synchronization between ARM and DSP */
    G722D_OMX_MALLOC_SIZE(pBufferHeader->pBuffer,(nSizeBytes + 256),OMX_U8);
    pBufferHeader->pBuffer += 128;
    /*pBufferHeader->pBuffer = (OMX_U8 *)malloc(nSizeBytes + 256);
      if(NULL == pBufferHeader->pBuffer) {
      G722DEC_DPRINT(":: Malloc Failed\n");
      eError = OMX_ErrorInsufficientResources;
      goto EXIT;
      }
      memset((pBufferHeader), 0x0, sizeof(OMX_BUFFERHEADERTYPE));
      G722DEC_MEMPRINT(":: Malloced = %p\n",pBufferHeader->pBuffer);*/
    pBufferHeader->nVersion.nVersion = G722DEC_BUFHEADER_VERSION;


    G722DEC_DPRINT("********************************************\n");
    G722DEC_DPRINT(":: Allocated BufHeader %p Buffer = %p, on port %ld\n",
                   pBufferHeader,
                   pBufferHeader->pBuffer, nPortIndex);

    G722DEC_DPRINT(":: Ip Num = %ld\n",pComponentPrivate->pInputBufferList->numBuffers);
    G722DEC_DPRINT(":: Op Num = %ld\n",pComponentPrivate->pOutputBufferList->numBuffers);
    G722DEC_DPRINT("********************************************\n");


    pBufferHeader->pAppPrivate = pAppPrivate;
    pBufferHeader->pPlatformPrivate = pComponentPrivate;
    pBufferHeader->nAllocLen = nSizeBytes;

    if (nPortIndex == G722D_INPUT_PORT) {
        pBufferHeader->nInputPortIndex = nPortIndex;
        pBufferHeader->nOutputPortIndex = -1;
        pComponentPrivate->pInputBufferList->pBufHdr[pComponentPrivate->pInputBufferList->numBuffers] = pBufferHeader;
        pComponentPrivate->pInputBufferList->bBufferPending[pComponentPrivate->pInputBufferList->numBuffers] = 0;

        G722DEC_DPRINT("pComponentPrivate->pInputBufferList->pBufHdr[%ld] = %p\n",
                       pComponentPrivate->pInputBufferList->numBuffers,
                       pComponentPrivate->pInputBufferList->pBufHdr[pComponentPrivate->pInputBufferList->numBuffers]);

        pComponentPrivate->pInputBufferList->bufferOwner[pComponentPrivate->pInputBufferList->numBuffers++] = 1;

        G722DEC_DPRINT("pComponentPrivate->pInputBufferList->numBuffers = %ld\n",
                       pComponentPrivate->pInputBufferList->numBuffers);

        if (pComponentPrivate->pInputBufferList->numBuffers == pPortDef->nBufferCountActual) {
            pPortDef->bPopulated = 1;
        }
    } else if (nPortIndex == G722D_OUTPUT_PORT) {
        pBufferHeader->nInputPortIndex = -1;
        pBufferHeader->nOutputPortIndex = nPortIndex;
        pComponentPrivate->pOutputBufferList->pBufHdr[pComponentPrivate->pOutputBufferList->numBuffers] = pBufferHeader;
        pComponentPrivate->pOutputBufferList->bBufferPending[pComponentPrivate->pOutputBufferList->numBuffers] = 0;

        G722DEC_DPRINT("pComponentPrivate->pOutputBufferList->pBufHdr[%ld] = %p\n",
                       pComponentPrivate->pOutputBufferList->numBuffers,
                       pComponentPrivate->pOutputBufferList->pBufHdr[pComponentPrivate->pOutputBufferList->numBuffers]);

        pComponentPrivate->pOutputBufferList->bufferOwner[pComponentPrivate->pOutputBufferList->numBuffers++] = 1;

        G722DEC_DPRINT("pComponentPrivate->pOutputBufferList->numBuffers = %ld\n",
                       pComponentPrivate->pOutputBufferList->numBuffers);

        if (pComponentPrivate->pOutputBufferList->numBuffers == pPortDef->nBufferCountActual) {
            pPortDef->bPopulated = 1;
        }
    } else {
        G722D_OMX_ERROR_EXIT(eError,OMX_ErrorBadPortIndex,"OMX_ErrorBadPortIndex");
    }

    if((pComponentPrivate->pPortDef[G722D_OUTPUT_PORT]->bPopulated == 
        pComponentPrivate->pPortDef[G722D_OUTPUT_PORT]->bEnabled)&&
       (pComponentPrivate->pPortDef[G722D_INPUT_PORT]->bPopulated == 
        pComponentPrivate->pPortDef[G722D_INPUT_PORT]->bEnabled) &&
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

    pBufferHeader->nVersion.s.nVersionMajor = G722DEC_MAJOR_VER;
    pBufferHeader->nVersion.s.nVersionMinor = G722DEC_MINOR_VER;
    pComponentPrivate->nVersion = pBufferHeader->nVersion.nVersion;
    pBufferHeader->nSize = sizeof(OMX_BUFFERHEADERTYPE);

    *pBuffer = pBufferHeader;
    pComponentPrivate->bufAlloced = 1;
    G722DEC_DPRINT("%d:: A Buffer has been alloced\n", __LINE__);

 EXIT:
    if(OMX_ErrorNone != eError) {
        G722DEC_DPRINT("%d :: ************* ERROR: Freeing Other Malloced Resources\n",__LINE__);
	 if (NULL != pBufferHeader) {
	     G722D_OMX_FREE(pBufferHeader->pBuffer);
	     G722D_OMX_FREE(pBufferHeader);
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
    G722DEC_COMPONENT_PRIVATE * pComponentPrivate = NULL;
    OMX_U8* buff = NULL;
    int i = 0;
    int inputIndex = -1;
    int outputIndex = -1;
    OMX_COMPONENTTYPE *pHandle = NULL;

    G722DEC_DPRINT ("Entering FreeBuffer\n");
    pComponentPrivate = (G722DEC_COMPONENT_PRIVATE *) (((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);

    pHandle = (OMX_COMPONENTTYPE *) pComponentPrivate->pHandle;
    for (i=0; i < MAX_NUM_OF_BUFS; i++) {
        buff = (OMX_U8 *)pComponentPrivate->pInputBufferList->pBufHdr[i];
        if (buff == (OMX_U8 *)pBuffer) {
            G722DEC_DPRINT("Found matching input buffer\n");
            G722DEC_DPRINT("buff = %p\n",buff);
            G722DEC_DPRINT("pBuffer = %p\n",pBuffer);
            inputIndex = i;
            break;
        }else{
            G722DEC_DPRINT("%d:: No matching found for pBuffer %p\n",__LINE__,pBuffer);
        }
    }

    for (i=0; i < MAX_NUM_OF_BUFS; i++) {
        buff = (OMX_U8 *)pComponentPrivate->pOutputBufferList->pBufHdr[i];
        if (buff == (OMX_U8 *)pBuffer) {
            G722DEC_DPRINT("Found matching output buffer\n");
            G722DEC_DPRINT("buff = %p\n",buff);
            G722DEC_DPRINT("pBuffer = %p\n",pBuffer);
            outputIndex = i;
            break;
        }else{
            G722DEC_DPRINT("%d:: No matching found for pBuffer %p\n",__LINE__,pBuffer);
        }
    }

    if (inputIndex != -1) {
        if (pComponentPrivate->pInputBufferList->bufferOwner[inputIndex] == 1) {
            buff = pComponentPrivate->pInputBufferList->pBufHdr[inputIndex]->pBuffer;
            if(buff != 0){
                buff -= 128;
            }
            G722DEC_MEMPRINT("\n: Freeing:  %p IP Buffer\n",buff);
            G722D_OMX_FREE(buff);
        }

        G722DEC_MEMPRINT("Freeing: %p IP Buf Header\n\n",
                         pComponentPrivate->pInputBufferList->pBufHdr[inputIndex]);

        G722D_OMX_FREE(pComponentPrivate->pInputBufferList->pBufHdr[inputIndex]);
        pComponentPrivate->pInputBufferList->numBuffers--;

        if (pComponentPrivate->pInputBufferList->numBuffers <
            pComponentPrivate->pPortDef[G722D_INPUT_PORT]->nBufferCountMin) {
            pComponentPrivate->pPortDef[G722D_INPUT_PORT]->bPopulated = OMX_FALSE;
        }
        if(pComponentPrivate->pPortDef[G722D_INPUT_PORT]->bEnabled &&
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
            G722DEC_MEMPRINT("Freeing: %p OP Buffer\n",buff);
            G722D_OMX_FREE(buff);
        }

        G722DEC_MEMPRINT("Freeing: %p OP Buf Header\n\n",
                         pComponentPrivate->pOutputBufferList->pBufHdr[outputIndex]);

        G722D_OMX_FREE(pComponentPrivate->pOutputBufferList->pBufHdr[outputIndex]);
        pComponentPrivate->pOutputBufferList->numBuffers--;

        G722DEC_DPRINT("pComponentPrivate->pOutputBufferList->numBuffers = %ld\n",
                       pComponentPrivate->pOutputBufferList->numBuffers);
        G722DEC_DPRINT("pComponentPrivate->pPortDef[G722D_OUTPUT_PORT]->nBufferCountMin = %ld\n",
                       pComponentPrivate->pPortDef[G722D_OUTPUT_PORT]->nBufferCountMin);

        if (pComponentPrivate->pOutputBufferList->numBuffers <
            pComponentPrivate->pPortDef[G722D_OUTPUT_PORT]->nBufferCountMin) {
            pComponentPrivate->pPortDef[G722D_OUTPUT_PORT]->bPopulated = OMX_FALSE;
        }

        if(pComponentPrivate->pPortDef[G722D_OUTPUT_PORT]->bEnabled &&
           pComponentPrivate->bLoadedCommandPending == OMX_FALSE &&
           (pComponentPrivate->curState == OMX_StateIdle ||
            pComponentPrivate->curState == OMX_StateExecuting ||
            pComponentPrivate->curState == OMX_StatePause)) {
            pComponentPrivate->cbInfo.EventHandler(
                                                   pHandle, pHandle->pApplicationPrivate,
                                                   OMX_EventError, OMX_ErrorPortUnpopulated,nPortIndex, NULL);
        }
    } else {
        G722DEC_DPRINT("Returning OMX_ErrorBadParameter\n");
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

    G722DEC_DPRINT ("Exiting FreeBuffer\n");
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
    G722DEC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_BUFFERHEADERTYPE *pBufferHeader = NULL;

    G722DEC_DPRINT ("Entering UseBuffer\n");
    G722DEC_DPRINT ("pBuffer = %p\n",pBuffer);

    pComponentPrivate = (G722DEC_COMPONENT_PRIVATE *)
        (((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);

    pPortDef = ((G722DEC_COMPONENT_PRIVATE*)
                pComponentPrivate)->pPortDef[nPortIndex];
    G722DEC_DPRINT ("pPortDef = %p\n", pPortDef);
    G722DEC_DPRINT ("pPortDef->bEnabled = %d\n",pPortDef->bEnabled);

    if(!pPortDef->bEnabled) {
        G722D_OMX_ERROR_EXIT(eError,OMX_ErrorIncorrectStateOperation,
                             "Port is Disabled: OMX_ErrorIncorrectStateOperation");
    }
#if 1
    if(nSizeBytes != pPortDef->nBufferSize || pPortDef->bPopulated) {
        G722D_OMX_ERROR_EXIT(eError,OMX_ErrorBadParameter,
                             "Bad Size or Port Disabled : OMX_ErrorBadParameter");
    }
#endif

    G722D_OMX_MALLOC(pBufferHeader, OMX_BUFFERHEADERTYPE);

    if (nPortIndex == G722D_OUTPUT_PORT) {
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

    if((pComponentPrivate->pPortDef[G722D_OUTPUT_PORT]->bPopulated == 
        pComponentPrivate->pPortDef[G722D_OUTPUT_PORT]->bEnabled)&&
       (pComponentPrivate->pPortDef[G722D_INPUT_PORT]->bPopulated == 
        pComponentPrivate->pPortDef[G722D_INPUT_PORT]->bEnabled) &&
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

    pBufferHeader->nVersion.s.nVersionMajor = G722DEC_MAJOR_VER;
    pBufferHeader->nVersion.s.nVersionMinor = G722DEC_MINOR_VER;
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

    G722DEC_DPRINT("GetExtensionIndex\n");
    if (!(strcmp(cParameterName,"OMX.TI.index.config.g722headerinfo"))) {
        *pIndexType = OMX_IndexCustomG722DecHeaderInfoConfig;
        G722DEC_DPRINT("OMX_IndexCustomG722DecHeaderInfoConfig\n");
    }
    else if(!(strcmp(cParameterName,"OMX.TI.index.config.g722decstreamIDinfo"))){
        *pIndexType = OMX_IndexCustomG722DecStreamIDConfig;
        G722DEC_DPRINT("OMX_IndexCustomG722DecStreamIDConfig\n");
    }
    else if(!(strcmp(cParameterName,"OMX.TI.index.config.g722dec.datapath"))) {
        *pIndexType = OMX_IndexCustomG722DecDataPath;
        G722DEC_DPRINT("OMX_IndexCustomG722DecDataPath\n");
    } 
    else {
        eError = OMX_ErrorBadParameter;
    }

    G722DEC_DPRINT("Exiting GetExtensionIndex\n");
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
    /* This is a non standard component and does not support roles */
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
