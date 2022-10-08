
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
* @file OMX_Mp3Decoder.c
*
* This file implements OpenMAX (TM) 1.0 Specific APIs and its functionality
* that is fully compliant with the Khronos OpenMAX (TM) 1.0 Specification
*
* @path  $(CSLPATH)\OMAPSW_MPU\linux\audio\src\openmax_il\mp3_dec\src
*
* @rev  1.0
*/
/* ----------------------------------------------------------------------------
*!
*! Revision History
*! ===================================
*! 21-sept-2006 bk: updated some review findings for alpha release
*! 24-Aug-2006 bk: Khronos OpenMAX (TM) 1.0 Conformance tests some more
*! 18-July-2006 bk: Khronos OpenMAX (TM) 1.0 Conformance tests validated for few cases
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
#endif
#include <pthread.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <dbapi.h>

/*------- Program Header Files -----------------------------------------------*/

#include "LCML_DspCodec.h"
#include "OMX_Mp3Dec_Utils.h"
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
int mp3d_fdwrite, mp3d_fdread;
int errno;
#endif

/* define component role */
#define MP3_DEC_ROLE "audio_decoder.mp3"

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
                                       OMX_OUT OMX_INDEXTYPE *pIndexType);

static OMX_ERRORTYPE ComponentRoleEnum(OMX_IN OMX_HANDLETYPE hComponent,
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
*  @see          Mp3Dec_StartCompThread()
*/
/* ================================================================================ * */
OMX_ERRORTYPE OMX_ComponentInit (OMX_HANDLETYPE hComp)
{
	

    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE*) hComp;
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef_ip = NULL, *pPortDef_op = NULL;
    OMX_AUDIO_PARAM_PORTFORMATTYPE *pPortFormat = NULL;
    OMX_AUDIO_PARAM_MP3TYPE *mp3_ip = NULL; 
    OMX_AUDIO_PARAM_PCMMODETYPE *mp3_op = NULL;
    MP3DEC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    MP3D_AUDIODEC_PORT_TYPE *pCompPort = NULL;
    MP3D_BUFFERLIST *pTemp = NULL;
    int i=0;

    OMXDBG_PRINT(stderr, PRINT, 1, 0, "Entering OMX_ComponentInit\n");

    MP3D_OMX_CONF_CHECK_CMD(pHandle,1,1);

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
    OMX_MALLOC_GENERIC(pHandle->pComponentPrivate,MP3DEC_COMPONENT_PRIVATE);

    pComponentPrivate = pHandle->pComponentPrivate;
    pComponentPrivate->pHandle = pHandle;
    OMX_DBG_INIT(pComponentPrivate->dbg, "OMX_DBG_MP3DEC");

#ifdef __PERF_INSTRUMENTATION__
    pComponentPrivate->pPERF = PERF_Create(PERF_FOURCC('M','P','3','D'),
                                           PERF_ModuleLLMM |
                                           PERF_ModuleAudioDecode);
#endif

    pComponentPrivate->iPVCapabilityFlags.iIsOMXComponentMultiThreaded = OMX_TRUE; 
    pComponentPrivate->iPVCapabilityFlags.iOMXComponentNeedsNALStartCode = OMX_FALSE; 
    pComponentPrivate->iPVCapabilityFlags.iOMXComponentSupportsExternalOutputBufferAlloc = OMX_FALSE;
    pComponentPrivate->iPVCapabilityFlags.iOMXComponentSupportsExternalInputBufferAlloc = OMX_FALSE; 
    pComponentPrivate->iPVCapabilityFlags.iOMXComponentSupportsMovableInputBuffers = OMX_FALSE; 
    /* Next capabilities are set to false in order to ease issues
       regarding 1 sec of audio sound repeated while FFW/RW(OMAPS00200511) */
    pComponentPrivate->iPVCapabilityFlags.iOMXComponentSupportsPartialFrames = OMX_FALSE;
    pComponentPrivate->iPVCapabilityFlags.iOMXComponentCanHandleIncompleteFrames = OMX_FALSE;


    OMX_MALLOC_GENERIC(pCompPort, MP3D_AUDIODEC_PORT_TYPE);
    pComponentPrivate->pCompPort[MP3D_INPUT_PORT] =  pCompPort;

    OMX_MALLOC_GENERIC(pCompPort, MP3D_AUDIODEC_PORT_TYPE);
    pComponentPrivate->pCompPort[MP3D_OUTPUT_PORT] = pCompPort;
    OMX_MALLOC_GENERIC(pTemp, MP3D_BUFFERLIST);
    pComponentPrivate->pInputBufferList = pTemp;

    OMX_MALLOC_GENERIC(pTemp, MP3D_BUFFERLIST);
    pComponentPrivate->pOutputBufferList = pTemp;

    pComponentPrivate->pInputBufferList->numBuffers = 0;
    pComponentPrivate->pOutputBufferList->numBuffers = 0;

    for (i=0; i < MP3D_MAX_NUM_OF_BUFS; i++) {
        pComponentPrivate->pInputBufferList->pBufHdr[i] = NULL;
        pComponentPrivate->pOutputBufferList->pBufHdr[i] = NULL;
    }   

    pComponentPrivate->bufAlloced = 0;

    OMX_MALLOC_GENERIC(pComponentPrivate->sPortParam, OMX_PORT_PARAM_TYPE);
    OMX_CONF_INIT_STRUCT(pComponentPrivate->sPortParam, OMX_PORT_PARAM_TYPE);
    OMX_MALLOC_GENERIC(pComponentPrivate->pPriorityMgmt, OMX_PRIORITYMGMTTYPE);
    OMX_CONF_INIT_STRUCT(pComponentPrivate->pPriorityMgmt, OMX_PRIORITYMGMTTYPE);
    pComponentPrivate->sPortParam->nPorts = NUM_OF_PORTS;
    pComponentPrivate->sPortParam->nStartPortNumber = 0x0;
    pComponentPrivate->mp3Params = NULL;

    pComponentPrivate->pcmParams = NULL;

    OMX_MALLOC_GENERIC(mp3_ip,OMX_AUDIO_PARAM_MP3TYPE);
    OMX_MALLOC_GENERIC(mp3_op,OMX_AUDIO_PARAM_PCMMODETYPE);


    pComponentPrivate->mp3Params = mp3_ip;
    pComponentPrivate->pcmParams = mp3_op;

    pComponentPrivate->dasfmode = 0;
    pComponentPrivate->frameMode = 0;
    pComponentPrivate->bBufferIsAllocated = 0;
    pComponentPrivate->bPortDefsAllocated = 0;
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
    pComponentPrivate->bEnableCommandParam = 0;
    pComponentPrivate->bIsInvalidState = OMX_FALSE;
    pComponentPrivate->sDeviceString = newmalloc(100*sizeof(OMX_STRING));
    pComponentPrivate->IpBufindex = 0;
    pComponentPrivate->OpBufindex = 0;
    pComponentPrivate->nNumInputBufPending = 0;
    pComponentPrivate->nNumOutputBufPending = 0;
    pComponentPrivate->nNumOutputBufPause = 0;
    pComponentPrivate->SendAfterEOS = 0;    
    pComponentPrivate->nHandledFillThisBuffers=0;
    pComponentPrivate->nHandledEmptyThisBuffers = 0;
    pComponentPrivate->bFlushOutputPortCommandPending = OMX_FALSE;
    pComponentPrivate->bFlushInputPortCommandPending = OMX_FALSE;

    pComponentPrivate->bPreempted = OMX_FALSE; 
    pComponentPrivate->first_buff = 0;
    pComponentPrivate->first_TS = 0;
    pComponentPrivate->temp_TS = 0;
    pComponentPrivate->lastout = NULL;

    //bConfigData flag is used to indicate if we need to parse the frame header 
    pComponentPrivate->bConfigData = 1;
    pComponentPrivate->reconfigInputPort = 0;
    pComponentPrivate->reconfigOutputPort = 1; //set the initial value to true if you expect to do port config...

    /* Initialize device string to the default value */
    strcpy((char*)pComponentPrivate->sDeviceString,"/eteedn:i0:o0/codec\0");

    for (i=0; i < MP3D_MAX_NUM_OF_BUFS; i++) {
        pComponentPrivate->pInputBufHdrPending[i] = NULL;
        pComponentPrivate->pOutputBufHdrPending[i] = NULL;
    }



    pComponentPrivate->nInvalidFrameCount = 0;
    pComponentPrivate->bDisableCommandPending = 0;
    pComponentPrivate->bEnableCommandPending = 0;
    
    pComponentPrivate->numPendingBuffers = 0;
    pComponentPrivate->bNoIdleOnStop= OMX_FALSE;
    pComponentPrivate->bDspStoppedWhileExecuting = OMX_FALSE;
    pComponentPrivate->sOutPortFormat.eEncoding = OMX_AUDIO_CodingPCM;


    /* initialize role name */
    strcpy((char*)pComponentPrivate->componentRole.cRole, MP3_DEC_ROLE);

#ifndef UNDER_CE
    pthread_mutex_init(&pComponentPrivate->AlloBuf_mutex, NULL);
    pthread_cond_init (&pComponentPrivate->AlloBuf_threshold, NULL);
    pComponentPrivate->AlloBuf_waitingsignal = 0;

    pthread_mutex_init(&pComponentPrivate->codecStop_mutex, NULL);
    pthread_cond_init (&pComponentPrivate->codecStop_threshold, NULL);
    pComponentPrivate->codecStop_waitingsignal = 0;

    pthread_mutex_init(&pComponentPrivate->codecFlush_mutex, NULL);
    pthread_cond_init (&pComponentPrivate->codecFlush_threshold, NULL);
    pComponentPrivate->codecFlush_waitingsignal = 0;
            
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

    pComponentPrivate->pPortDef[MP3D_INPUT_PORT] = pPortDef_ip;
    pComponentPrivate->pPortDef[MP3D_OUTPUT_PORT] = pPortDef_op;

    /* Set input port defaults */
    pPortDef_ip->nSize                              = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
    pPortDef_ip->nPortIndex                         = MP3D_INPUT_PORT;
    pPortDef_ip->eDir                               = OMX_DirInput;
    pPortDef_ip->nBufferCountActual                 = MP3D_NUM_INPUT_BUFFERS;
    pPortDef_ip->nBufferCountMin                    = MP3D_NUM_INPUT_BUFFERS;
    pPortDef_ip->nBufferSize                        = MP3D_INPUT_BUFFER_SIZE;
    pPortDef_ip->nBufferAlignment                   = DSP_CACHE_ALIGNMENT;
    pPortDef_ip->bEnabled                           = OMX_TRUE;
    pPortDef_ip->bPopulated                         = OMX_FALSE;
    pPortDef_ip->eDomain                            = OMX_PortDomainAudio;
    pPortDef_ip->format.audio.eEncoding             = OMX_AUDIO_CodingMP3;
    pPortDef_ip->format.audio.cMIMEType             = NULL;
    pPortDef_ip->format.audio.pNativeRender         = NULL;
    pPortDef_ip->format.audio.bFlagErrorConcealment = OMX_FALSE;


    /* Set output port defaults */
    pPortDef_op->nSize                              = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
    pPortDef_op->nPortIndex                         = MP3D_OUTPUT_PORT;
    pPortDef_op->eDir                               = OMX_DirOutput;
    pPortDef_op->nBufferCountMin                    = MP3D_NUM_OUTPUT_BUFFERS;
    pPortDef_op->nBufferCountActual                 = MP3D_NUM_OUTPUT_BUFFERS;
    pPortDef_op->nBufferSize                        = MP3D_OUTPUT_BUFFER_SIZE;
    pPortDef_op->nBufferAlignment                   = DSP_CACHE_ALIGNMENT;
    pPortDef_op->bEnabled                           = OMX_TRUE;
    pPortDef_op->bPopulated                         = OMX_FALSE;
    pPortDef_op->eDomain                            = OMX_PortDomainAudio;
    pPortDef_op->format.audio.eEncoding             = OMX_AUDIO_CodingPCM; 
    pPortDef_op->format.audio.cMIMEType             = NULL;
    pPortDef_op->format.audio.pNativeRender         = NULL;
    pPortDef_op->format.audio.bFlagErrorConcealment = OMX_FALSE;

    OMX_MALLOC_GENERIC(pComponentPrivate->pCompPort[MP3D_INPUT_PORT]->pPortFormat, OMX_AUDIO_PARAM_PORTFORMATTYPE);
    OMX_MALLOC_GENERIC(pComponentPrivate->pCompPort[MP3D_OUTPUT_PORT]->pPortFormat, OMX_AUDIO_PARAM_PORTFORMATTYPE);
    OMX_CONF_INIT_STRUCT(pComponentPrivate->pCompPort[MP3D_INPUT_PORT]->pPortFormat, OMX_AUDIO_PARAM_PORTFORMATTYPE);
    OMX_CONF_INIT_STRUCT(pComponentPrivate->pCompPort[MP3D_OUTPUT_PORT]->pPortFormat, OMX_AUDIO_PARAM_PORTFORMATTYPE);


    /* Set input port format defaults */
    pPortFormat = pComponentPrivate->pCompPort[MP3D_INPUT_PORT]->pPortFormat;
    OMX_CONF_INIT_STRUCT(pPortFormat, OMX_AUDIO_PARAM_PORTFORMATTYPE);
    pPortFormat->nPortIndex         = MP3D_INPUT_PORT;
    pPortFormat->nIndex             = OMX_IndexParamAudioMp3;
    pPortFormat->eEncoding          = OMX_AUDIO_CodingMP3;

    /* Set output port format defaults */
    pPortFormat = pComponentPrivate->pCompPort[MP3D_OUTPUT_PORT]->pPortFormat;
    OMX_CONF_INIT_STRUCT(pPortFormat, OMX_AUDIO_PARAM_PORTFORMATTYPE);
    pPortFormat->nPortIndex         = MP3D_OUTPUT_PORT;
    pPortFormat->nIndex             = OMX_IndexParamAudioPcm;
    pPortFormat->eEncoding          = OMX_AUDIO_CodingPCM;

    /* MP3 format defaults */
    OMX_CONF_INIT_STRUCT(mp3_ip, OMX_AUDIO_PARAM_MP3TYPE);
    mp3_ip->nPortIndex = MP3D_INPUT_PORT;
    mp3_ip->nSampleRate = 44100;
    mp3_ip->nChannels = MP3D_STEREO_STREAM;
    mp3_ip->eChannelMode = OMX_AUDIO_ChannelModeStereo;
    mp3_ip->nAudioBandWidth = 0;
    mp3_ip->eFormat = OMX_AUDIO_MP3StreamFormatMP1Layer3;

    /* PCM format defaults */
    OMX_CONF_INIT_STRUCT(mp3_op, OMX_AUDIO_PARAM_PCMMODETYPE);
    mp3_op->nPortIndex = MP3D_OUTPUT_PORT;
    mp3_op->nBitPerSample = 16;
    mp3_op->nChannels = MP3D_STEREO_STREAM;
    mp3_op->nSamplingRate = 44100;
    mp3_op->eNumData= OMX_NumericalDataSigned; 
    mp3_op->ePCMMode = OMX_AUDIO_PCMModeLinear; 
    mp3_op->bInterleaved = OMX_TRUE;
    
    pComponentPrivate->bPortDefsAllocated = 1;

#ifdef DSP_RENDERING_ON
    if((mp3d_fdwrite=open(FIFO1,O_WRONLY))<0) {
        OMX_ERROR4(pComponentPrivate->dbg, "[MP3 Component] - failure to open WRITE pipe\n");
        eError = OMX_ErrorHardware;
    }

    if((mp3d_fdread=open(FIFO2,O_RDONLY))<0) {
        OMX_ERROR4(pComponentPrivate->dbg, "[MP3 Component] - failure to open READ pipe\n");
        eError = OMX_ErrorHardware;

    }
#endif

#ifdef RESOURCE_MANAGER_ENABLED
    eError = RMProxy_NewInitalize();
    /*eError = RMProxy_Initalize();*/
    if (eError != OMX_ErrorNone) {
        OMX_ERROR4(pComponentPrivate->dbg, ":Error returned from loading ResourceManagerProxy thread\n");
        goto EXIT;
    }
#endif

    eError = Mp3Dec_StartCompThread(pHandle);
    if (eError != OMX_ErrorNone) {
        OMX_ERROR4(pComponentPrivate->dbg, "::Error returned from the Component\n");
        goto EXIT;
    }
#ifdef __PERF_INSTRUMENTATION__
    PERF_ThreadCreated(pComponentPrivate->pPERF, pComponentPrivate->ComponentThread,
                       PERF_FOURCC('M','P','3','T'));
#endif
 EXIT:
    if(OMX_ErrorNone != eError) {
        OMX_ERROR4(pComponentPrivate->dbg, "%d :: ************* ERROR: Freeing Other Malloced Resources\n",__LINE__);
        OMX_MEMFREE_STRUCT(pPortDef_ip);
        OMX_MEMFREE_STRUCT(pPortDef_op);
        OMX_MEMFREE_STRUCT(mp3_ip);
        OMX_MEMFREE_STRUCT(mp3_op);
        OMX_MEMFREE_STRUCT(pComponentPrivate);
        OMX_MEMFREE_STRUCT(pTemp);
    }
    OMX_PRINT1(pComponentPrivate->dbg, ":: Exiting OMX_ComponentInit\n");
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
* @param pCmdData This is command data that came with command.
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
    int nRet;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)phandle;
    MP3DEC_COMPONENT_PRIVATE *pCompPrivate = NULL;

    MP3D_OMX_CONF_CHECK_CMD(pHandle,1,1)
        pCompPrivate = (MP3DEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    
#ifdef _ERROR_PROPAGATION__
    if (pCompPrivate->curState == OMX_StateInvalid){
        eError = OMX_ErrorInvalidState;
        goto EXIT;
    }   
#else
    OMX_PRINT1(pCompPrivate->dbg, ":: MP3DEC: Entered SendCommand\n");
    if(pCompPrivate->curState == OMX_StateInvalid){
        MP3D_OMX_ERROR_EXIT(eError, OMX_ErrorInvalidState,"OMX_ErrorInvalidState");
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
        OMX_PRSTATE2(pCompPrivate->dbg, " MP3DEC: Entered switch - Command State Set\n");
        if (nParam == OMX_StateLoaded) {
            pCompPrivate->bLoadedCommandPending = OMX_TRUE;
        }
        if(pCompPrivate->curState == OMX_StateLoaded) {
            OMX_PRSTATE2(pCompPrivate->dbg, " MP3DEC: Entered switch - curState == OMX_StateLoaded\n");
            if((nParam == OMX_StateExecuting) || (nParam == OMX_StatePause)) {
                OMX_PRDSP1(pCompPrivate->dbg, ":: MP3DEC: Entered switch - nParam == StatExecuting || OMX_StatePause\n");
                pCompPrivate->cbInfo.EventHandler (pHandle,
                                                   pHandle->pApplicationPrivate,
                                                   OMX_EventError,
                                                   OMX_ErrorIncorrectStateTransition,
                                                   OMX_TI_ErrorMinor,
                                                   NULL);
                OMX_ERROR4(pCompPrivate->dbg, ":: Incorrect St Tr fm Loaded to Executing By App\n");
                goto EXIT;
            }

            if(nParam == OMX_StateInvalid) {
                pCompPrivate->curState = OMX_StateInvalid;
                pCompPrivate->cbInfo.EventHandler (
                                                   pHandle,
                                                   pHandle->pApplicationPrivate,
                                                   OMX_EventError,
                                                   OMX_ErrorInvalidState,
                                                   OMX_TI_ErrorMinor,
                                                   NULL);
                OMX_ERROR4(pCompPrivate->dbg, ":: Incorrect State Tr from Loaded to Invalid by Application\n");
                goto EXIT;
            }
        }
        break;
    case OMX_CommandFlush:
        if(nParam > 1 && nParam != -1) {
            MP3D_OMX_ERROR_EXIT(eError,OMX_ErrorBadPortIndex,"OMX_ErrorBadPortIndex");
        }
        break;
    case OMX_CommandPortDisable:
        break;
    case OMX_CommandPortEnable:
        OMX_PRDSP2(pCompPrivate->dbg, ":: MP3DEC: Entered switch - Command Port Enable/Disbale\n");
        break;
    case OMX_CommandMarkBuffer:
        OMX_PRBUFFER2(pCompPrivate->dbg, ":: MP3DEC: Entered switch - Command Mark Buffer\n");

        if(nParam > 0) {
            MP3D_OMX_ERROR_EXIT(eError,OMX_ErrorBadPortIndex,"OMX_ErrorBadPortIndex");
        }

        break;
    default:
        OMX_PRDSP2(pCompPrivate->dbg, " :: MP3DEC: Entered switch - Default\n");
        OMX_ERROR2(pCompPrivate->dbg, ":: MP3DEC: Command Received Default \
                                                      error\n");
        pCompPrivate->cbInfo.EventHandler(pHandle, 
                                          pHandle->pApplicationPrivate,
                                          OMX_EventError,
                                          OMX_ErrorUndefined,
                                          OMX_TI_ErrorMinor,
                                          "Invalid Command");
        break;
    }


    nRet = write (pCompPrivate->cmdPipe[1], &Cmd, sizeof(Cmd));
    if (nRet == -1) {
        MP3D_OMX_ERROR_EXIT(eError,OMX_ErrorHardware,"write failed: OMX_ErrorHardware");
    }


    if (Cmd == OMX_CommandMarkBuffer) {
        nRet = write (pCompPrivate->cmdDataPipe[1], &pCmdData,
                      sizeof(OMX_PTR));
        if (nRet == -1) {
            MP3D_OMX_ERROR_EXIT(eError,OMX_ErrorHardware,"write failed: OMX_ErrorHardware");
        }
    }
    else {
        nRet = write (pCompPrivate->cmdDataPipe[1], &nParam,
                      sizeof(OMX_U32));
        if (nRet == -1) {
            MP3D_OMX_ERROR_EXIT(eError,OMX_ErrorHardware,"write failed: OMX_ErrorHardware");
        }
    }
    OMX_PRINT2(pCompPrivate->dbg, ":: MP3DEC:SendCommand - nRet = %d\n",nRet);


#ifdef DSP_RENDERING_ON
    if(Cmd == OMX_CommandStateSet && nParam == OMX_StateExecuting) {
        /* enable Tee device command*/
        cmd_data.hComponent = pHandle;
        cmd_data.AM_Cmd = AM_CommandTDNDownlinkMode;
        cmd_data.param1 = 0;
        cmd_data.param2 = 0;
        cmd_data.streamID = 0;

        if((write(mp3d_fdwrite, &cmd_data, sizeof(cmd_data)))<0) {
            eError = OMX_ErrorHardware;
            goto EXIT;
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
    MP3DEC_COMPONENT_PRIVATE  *pComponentPrivate = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE *pParameterStructure;
    /*    OMX_PARAM_BUFFERSUPPLIERTYPE *pBufferSupplier;*/

    pParameterStructure = (OMX_PARAM_PORTDEFINITIONTYPE*)ComponentParameterStructure;

    MP3D_OMX_CONF_CHECK_CMD(hComp,1,1);
    pComponentPrivate = (MP3DEC_COMPONENT_PRIVATE *)(((OMX_COMPONENTTYPE*)hComp)->pComponentPrivate);
    MP3D_OMX_CONF_CHECK_CMD(pComponentPrivate, ComponentParameterStructure, 1)


        OMX_PRINT1(pComponentPrivate->dbg, ":: Entering the GetParameter\n");
    if (pParameterStructure == NULL) {
        eError = OMX_ErrorBadParameter;
        OMX_ERROR4(pComponentPrivate->dbg, ":: OMX_ErrorBadPortIndex from GetParameter");
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
        goto EXIT;
    }
#endif  

    switch(nParamIndex){
    case OMX_IndexParamAudioInit:
        OMX_PRDSP2(pComponentPrivate->dbg, ":: GetParameter OMX_IndexParamAudioInit\n");
        MP3D_OMX_CONF_CHECK_CMD(pComponentPrivate->sPortParam, 1, 1);
        memcpy(ComponentParameterStructure, pComponentPrivate->sPortParam, sizeof(OMX_PORT_PARAM_TYPE));
        break;
    case OMX_IndexParamPortDefinition:
        OMX_PRDSP2(pComponentPrivate->dbg, ": GetParameter OMX_IndexParamPortDefinition \n");
        if(((OMX_PARAM_PORTDEFINITIONTYPE *)(ComponentParameterStructure))->nPortIndex ==
           pComponentPrivate->pPortDef[MP3D_INPUT_PORT]->nPortIndex) {
            memcpy(ComponentParameterStructure, pComponentPrivate->pPortDef[MP3D_INPUT_PORT], 
                   sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
        } else if(((OMX_PARAM_PORTDEFINITIONTYPE *)(ComponentParameterStructure))->nPortIndex ==
                  pComponentPrivate->pPortDef[MP3D_OUTPUT_PORT]->nPortIndex) {
            memcpy(ComponentParameterStructure, pComponentPrivate->pPortDef[MP3D_OUTPUT_PORT], 
                   sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
        } else {
            OMX_ERROR4(pComponentPrivate->dbg, ":: OMX_ErrorBadPortIndex from GetParameter \n");
            eError = OMX_ErrorBadPortIndex;
        }
        break;
    case OMX_IndexParamAudioPortFormat:
        OMX_PRDSP2(pComponentPrivate->dbg, ":: GetParameter OMX_IndexParamAudioPortFormat \n");
        if(((OMX_AUDIO_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nPortIndex ==
           pComponentPrivate->pPortDef[MP3D_INPUT_PORT]->nPortIndex) {
            if(((OMX_AUDIO_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nIndex >
               pComponentPrivate->pCompPort[MP3D_INPUT_PORT]->pPortFormat->nPortIndex) {
                eError = OMX_ErrorNoMore;
            }
            else {
                memcpy(ComponentParameterStructure, pComponentPrivate->pCompPort[MP3D_INPUT_PORT]->pPortFormat, 
                       sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
            }
        }
        else if(((OMX_AUDIO_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nPortIndex ==
                pComponentPrivate->pPortDef[MP3D_OUTPUT_PORT]->nPortIndex){
            if(((OMX_AUDIO_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nIndex >
               pComponentPrivate->pCompPort[MP3D_OUTPUT_PORT]->pPortFormat->nPortIndex) {
                eError = OMX_ErrorNoMore;
            }
            else {
                memcpy(ComponentParameterStructure, pComponentPrivate->pCompPort[MP3D_OUTPUT_PORT]->pPortFormat, 
                       sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
            }
        }
        else {
            OMX_ERROR4(pComponentPrivate->dbg, ":: OMX_ErrorBadPortIndex from GetParameter \n");
            eError = OMX_ErrorBadPortIndex;
        }

        break;

    case OMX_IndexParamAudioMp3:
        OMX_PRDSP2(pComponentPrivate->dbg, " :: GetParameter OMX_IndexParamAudioMp3 \n");
        if(((OMX_AUDIO_PARAM_MP3TYPE *)(ComponentParameterStructure))->nPortIndex ==
           pComponentPrivate->mp3Params->nPortIndex) {
            memcpy(ComponentParameterStructure, pComponentPrivate->mp3Params, sizeof(OMX_AUDIO_PARAM_MP3TYPE));
        } else {
            OMX_ERROR4(pComponentPrivate->dbg, " :: OMX_ErrorBadPortIndex from GetParameter \n");
            eError = OMX_ErrorBadPortIndex;
        }

        break;

    case OMX_IndexParamPriorityMgmt:
        OMX_PRDSP2(pComponentPrivate->dbg, " :: GetParameter OMX_IndexParamPriorityMgmt \n");
        MP3D_OMX_CONF_CHECK_CMD(pComponentPrivate->pPriorityMgmt, 1, 1);
        memcpy(ComponentParameterStructure, pComponentPrivate->pPriorityMgmt, sizeof(OMX_PRIORITYMGMTTYPE));

        break;

    case OMX_IndexParamAudioPcm:
        OMX_PRDSP2(pComponentPrivate->dbg, " :: GetParameter OMX_IndexParamAudioPcm \n");
        if(((OMX_AUDIO_PARAM_PCMMODETYPE *)(ComponentParameterStructure))->nPortIndex ==
           pComponentPrivate->pcmParams->nPortIndex) {
            memcpy(ComponentParameterStructure,pComponentPrivate->pcmParams, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
        }else{
            OMX_PRINT1(pComponentPrivate->dbg, " :: OMX_ErrorBadPortIndex from GetParameter \n");
            eError = OMX_ErrorBadPortIndex;
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
            OMX_ERROR2(pComponentPrivate->dbg, ":: OMX_ErrorBadPortIndex from GetParameter");
            eError = OMX_ErrorBadPortIndex;
        } 
                
        break;

    case OMX_IndexParamVideoInit:
        break;

    case OMX_IndexParamImageInit:
        break;

    case OMX_IndexParamOtherInit:
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
        OMX_PRDSP2(pComponentPrivate->dbg, "%d :: Copying PV_OMX_COMPONENT_CAPABILITY_TYPE_INDEX\n", __LINE__);
        memcpy(pCap_flags, &(pComponentPrivate->iPVCapabilityFlags), sizeof(PV_OMXComponentCapabilityFlagsType));
	eError = OMX_ErrorNone;
    }
    break;
#endif

    default:
        OMX_ERROR4(pComponentPrivate->dbg, " :: OMX_ErrorUnsupportedIndex GetParameter \n");
        eError = OMX_ErrorUnsupportedIndex;
        break;
    }
 EXIT:
    if (pComponentPrivate != NULL) {
	 OMX_PRINT1(pComponentPrivate->dbg, " :: Exiting GetParameter\n");
	 OMX_PRINT1(pComponentPrivate->dbg, " :: Returning = 0x%x\n", eError);
    }
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
static OMX_ERRORTYPE SetParameter (OMX_HANDLETYPE hComp,
                                   OMX_INDEXTYPE nParamIndex,
                                   OMX_PTR pCompParam)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    MP3DEC_COMPONENT_PRIVATE  *pComponentPrivate = NULL;
    OMX_AUDIO_PARAM_PORTFORMATTYPE* pComponentParam = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE *pComponentParamPort = NULL;
    OMX_AUDIO_PARAM_MP3TYPE *pCompMp3Param = NULL;
    OMX_PARAM_COMPONENTROLETYPE  *pRole;
    OMX_PARAM_BUFFERSUPPLIERTYPE sBufferSupplier;
    OMX_AUDIO_PARAM_PCMMODETYPE* pPcmPort = NULL;


    MP3D_OMX_CONF_CHECK_CMD(hComp,1,1) 
        pComponentPrivate = (MP3DEC_COMPONENT_PRIVATE *)(((OMX_COMPONENTTYPE*)hComp)->pComponentPrivate);
    MP3D_OMX_CONF_CHECK_CMD(pComponentPrivate, pCompParam, 1); 
    
#ifdef _ERROR_PROPAGATION__
    if (pComponentPrivate->curState == OMX_StateInvalid){
        eError = OMX_ErrorInvalidState;
        goto EXIT;
    }   
#endif

    OMX_PRINT1(pComponentPrivate->dbg, " :: Entering the SetParameter\n");
    if (pComponentPrivate->curState != OMX_StateLoaded) {
        eError = OMX_ErrorIncorrectStateOperation;
        OMX_ERROR4(pComponentPrivate->dbg, " :: OMX_ErrorIncorrectStateOperation from SetParameter");
        goto EXIT;
    }

    switch(nParamIndex) {
    case OMX_IndexParamAudioPortFormat:
        OMX_PRDSP1(pComponentPrivate->dbg, ":: SetParameter OMX_IndexParamAudioPortFormat \n");
        pComponentParam = (OMX_AUDIO_PARAM_PORTFORMATTYPE *)pCompParam;
        if ( pComponentParam->nPortIndex == pComponentPrivate->pCompPort[MP3D_INPUT_PORT]->pPortFormat->nPortIndex ) {
            memcpy(pComponentPrivate->pCompPort[MP3D_INPUT_PORT]->pPortFormat, pComponentParam, 
                   sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
        } else if ( pComponentParam->nPortIndex == pComponentPrivate->pCompPort[MP3D_OUTPUT_PORT]->pPortFormat->nPortIndex ) {
            memcpy(pComponentPrivate->pCompPort[MP3D_OUTPUT_PORT]->pPortFormat, pComponentParam, 
                   sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
        } else {
            OMX_ERROR4(pComponentPrivate->dbg, ":: OMX_ErrorBadPortIndex from SetParameter");
            eError = OMX_ErrorBadPortIndex;
        }

        break;
    case OMX_IndexParamAudioMp3:
        OMX_PRDSP2(pComponentPrivate->dbg, " :: SetParameter OMX_IndexParamAudioMp3 \n");
        pCompMp3Param = (OMX_AUDIO_PARAM_MP3TYPE *)pCompParam;
        if(pCompMp3Param->nPortIndex == MP3D_INPUT_PORT) {
            MP3D_OMX_CONF_CHECK_CMD(pComponentPrivate->mp3Params, 1, 1);
            memcpy(pComponentPrivate->mp3Params,pCompMp3Param,sizeof(OMX_AUDIO_PARAM_MP3TYPE));
        } 
        else {
            OMX_ERROR4(pComponentPrivate->dbg, ":: OMX_ErrorBadPortIndex from SetParameter");
            eError = OMX_ErrorBadPortIndex;
        }
        break;
    case OMX_IndexParamPortDefinition:
        pComponentParamPort = (OMX_PARAM_PORTDEFINITIONTYPE *)pCompParam;
        if (pComponentParamPort->nPortIndex == 0) {
            if (pComponentParamPort->eDir != OMX_DirInput) {
                OMX_ERROR4(pComponentPrivate->dbg, ":: Invalid input buffer Direction\n");
                eError = OMX_ErrorBadParameter;
                goto EXIT;
            }
            if (pComponentParamPort->format.audio.eEncoding != OMX_AUDIO_CodingMP3) {
                OMX_ERROR4(pComponentPrivate->dbg, ":: Invalid format Parameter\n");
                eError = OMX_ErrorBadParameter;
                goto EXIT;
            }
        } else if (pComponentParamPort->nPortIndex == 1) {
            if (pComponentParamPort->eDir != OMX_DirOutput) {
                OMX_ERROR4(pComponentPrivate->dbg, " :: Invalid Output buffer Direction\n");
                eError = OMX_ErrorBadParameter;
                goto EXIT;
            }
            if (pComponentParamPort->format.audio.eEncoding != OMX_AUDIO_CodingPCM) {
                OMX_ERROR4(pComponentPrivate->dbg, ":: Invalid format Parameter\n");
                eError = OMX_ErrorBadParameter;
                goto EXIT;
            }
        } else {
            OMX_ERROR4(pComponentPrivate->dbg, ":: OMX_ErrorBadPortIndex from SetParameter");
            eError = OMX_ErrorBadPortIndex;
        }
        OMX_PRDSP2(pComponentPrivate->dbg, ":: SetParameter OMX_IndexParamPortDefinition \n");
        if(((OMX_PARAM_PORTDEFINITIONTYPE *)(pCompParam))->nPortIndex ==
           pComponentPrivate->pPortDef[MP3D_INPUT_PORT]->nPortIndex) {
            OMX_PRINT1(pComponentPrivate->dbg, ":: SetParameter OMX_IndexParamPortDefinition \n");
            memcpy(pComponentPrivate->pPortDef[MP3D_INPUT_PORT], pCompParam,sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
        }
        else if(((OMX_PARAM_PORTDEFINITIONTYPE *)(pCompParam))->nPortIndex ==
                pComponentPrivate->pPortDef[MP3D_OUTPUT_PORT]->nPortIndex) {
            OMX_PRDSP1(pComponentPrivate->dbg, ":: SetParameter OMX_IndexParamPortDefinition \n");
            memcpy(pComponentPrivate->pPortDef[MP3D_OUTPUT_PORT], pCompParam, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
        }
        else {
            OMX_ERROR4(pComponentPrivate->dbg, ":: OMX_ErrorBadPortIndex from SetParameter");
            eError = OMX_ErrorBadPortIndex;
        }
        break;
    case OMX_IndexParamPriorityMgmt:
        OMX_PRDSP2(pComponentPrivate->dbg, ":: SetParameter OMX_IndexParamPriorityMgmt \n");
        MP3D_OMX_CONF_CHECK_CMD(pComponentPrivate->pPriorityMgmt, 1, 1);
        memcpy(pComponentPrivate->pPriorityMgmt, (OMX_PRIORITYMGMTTYPE*)pCompParam, sizeof(OMX_PRIORITYMGMTTYPE));
        break;

    case OMX_IndexParamAudioInit:
        OMX_PRDSP2(pComponentPrivate->dbg, ":: SetParameter OMX_IndexParamAudioInit \n");
        MP3D_OMX_CONF_CHECK_CMD(pComponentPrivate->sPortParam, 1, 1);
        memcpy(pComponentPrivate->sPortParam, (OMX_PORT_PARAM_TYPE*)pCompParam, sizeof(OMX_PORT_PARAM_TYPE));
        break;

    case OMX_IndexParamStandardComponentRole:
        if (pCompParam) {
            pRole = (OMX_PARAM_COMPONENTROLETYPE *)pCompParam;
            memcpy(&(pComponentPrivate->componentRole), (void *)pRole, sizeof(OMX_PARAM_COMPONENTROLETYPE));
        } else {
            OMX_ERROR4(pComponentPrivate->dbg, ":: OMX_ErrorBadPortIndex from SetParameter");
            eError = OMX_ErrorBadParameter;
        }
        break;

    case OMX_IndexParamAudioPcm:
        OMX_PRDSP2(pComponentPrivate->dbg, " :: SetParameter OMX_IndexParamAudioPcm \n");
        pPcmPort= (OMX_AUDIO_PARAM_PCMMODETYPE *)pCompParam;
        if(pPcmPort->nPortIndex == MP3D_OUTPUT_PORT){
            MP3D_OMX_CONF_CHECK_CMD(pComponentPrivate->pcmParams, 1, 1);
            memcpy(pComponentPrivate->pcmParams, pPcmPort, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
        }else{
            OMX_ERROR4(pComponentPrivate->dbg, ":: OMX_ErrorBadPortIndex from SetParameter");
            eError = OMX_ErrorBadParameter;
        }
        break;

    case OMX_IndexParamCompBufferSupplier:
        /*  eError = OMX_ErrorBadPortIndex; *//*remove for StdAudioDecoderTest, leave for other tests*/
        if(((OMX_PARAM_PORTDEFINITIONTYPE *)(pCompParam))->nPortIndex ==
           pComponentPrivate->pPortDef[MP3D_INPUT_PORT]->nPortIndex) {
            OMX_PRDSP2(pComponentPrivate->dbg, ":: SetParameter OMX_IndexParamCompBufferSupplier \n");
            sBufferSupplier.eBufferSupplier = OMX_BufferSupplyInput;
            memcpy(&sBufferSupplier, pCompParam, sizeof(OMX_PARAM_BUFFERSUPPLIERTYPE));                                 
                    
        }
        else if(((OMX_PARAM_PORTDEFINITIONTYPE *)(pCompParam))->nPortIndex ==
                pComponentPrivate->pPortDef[MP3D_OUTPUT_PORT]->nPortIndex) {
            OMX_PRBUFFER2(pComponentPrivate->dbg, ":: SetParameter OMX_IndexParamCompBufferSupplier \n");
            sBufferSupplier.eBufferSupplier = OMX_BufferSupplyOutput;
            memcpy(&sBufferSupplier, pCompParam, sizeof(OMX_PARAM_BUFFERSUPPLIERTYPE));
        } 
        else {
            OMX_ERROR4(pComponentPrivate->dbg, ":: OMX_ErrorBadPortIndex from SetParameter");
            eError = OMX_ErrorBadPortIndex;
        }
        break;

    case OMX_IndexParamVideoInit:
    case OMX_IndexParamImageInit:
    case OMX_IndexParamOtherInit:
        eError = OMX_ErrorNone;
        break;

    default:
        OMX_ERROR4(pComponentPrivate->dbg, ":: SetParameter OMX_ErrorUnsupportedIndex \n");
        eError = OMX_ErrorUnsupportedIndex; 
        break;
    }
 EXIT:
    if (pComponentPrivate != NULL) {
	 OMX_PRINT1(pComponentPrivate->dbg, ":: Exiting SetParameter\n");
	 OMX_PRINT1(pComponentPrivate->dbg, ":: Returning = 0x%x\n", eError);
    }
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
    MP3DEC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
#ifdef DSP_RENDERING_ON
    OMX_AUDIO_CONFIG_MUTETYPE *pMuteStructure = NULL;
    OMX_AUDIO_CONFIG_VOLUMETYPE *pVolumeStructure = NULL;
#endif
    int *customFlag = NULL;
    TI_OMX_DSP_DEFINITION *configData;
    int flagValue=0;
    OMX_S16* deviceString = NULL;
    TI_OMX_DATAPATH dataPath;

    MP3D_OMX_CONF_CHECK_CMD(pHandle,1,1) 
        pComponentPrivate = (MP3DEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    MP3D_OMX_CONF_CHECK_CMD(pComponentPrivate,1,1) 

#ifdef _ERROR_PROPAGATION__
        if (pComponentPrivate->curState == OMX_StateInvalid){
            eError = OMX_ErrorInvalidState;
            goto EXIT;
        }   
#endif
    
    OMX_PRINT1(pComponentPrivate->dbg, ":: Entering SetConfig\n");
    if (pHandle == NULL) {
        OMX_ERROR4(pComponentPrivate->dbg, " :: Invalid HANDLE OMX_ErrorBadParameter \n");
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }

    switch (nConfigIndex) {
        /* set mute/unmute for playback stream */
    case OMX_IndexConfigAudioMute:
#ifdef DSP_RENDERING_ON
        pMuteStructure = (OMX_AUDIO_CONFIG_MUTETYPE *)ComponentConfigStructure;
        OMX_PRDSP2(pComponentPrivate->dbg, "Set Mute/Unmute for playback stream\n");
        cmd_data.hComponent = hComp;
        if(pMuteStructure->bMute == OMX_TRUE)
            {
                OMX_PRINT2(pComponentPrivate->dbg, "Mute the playback stream\n");
                cmd_data.AM_Cmd = AM_CommandStreamMute;
            }
        else
            {
                OMX_PRINT2(pComponentPrivate->dbg, "unMute the playback stream\n");
                cmd_data.AM_Cmd = AM_CommandStreamUnMute;
            }
        cmd_data.param1 = 0;
        cmd_data.param2 = 0;
        cmd_data.streamID = pComponentPrivate->streamID;

        if((write(mp3d_fdwrite, &cmd_data, sizeof(cmd_data)))<0)
            {   
                OMX_ERROR2(pComponentPrivate->dbg, "[MP3 decoder] - fail to send command to audio manager\n");
            }
#endif
        break;
        /* set volume for playback stream */
    case OMX_IndexConfigAudioVolume:
#ifdef DSP_RENDERING_ON
        pVolumeStructure = (OMX_AUDIO_CONFIG_VOLUMETYPE *)ComponentConfigStructure;
        OMX_PRDSP2(pComponentPrivate->dbg, "Set volume for playback stream\n");
        cmd_data.hComponent = hComp;
        cmd_data.AM_Cmd = AM_CommandSWGain;
        cmd_data.param1 = pVolumeStructure->sVolume.nValue;
        cmd_data.param2 = 0;
        cmd_data.streamID = pComponentPrivate->streamID;

        if((write(mp3d_fdwrite, &cmd_data, sizeof(cmd_data)))<0)
            {   
                OMX_ERROR4(pComponentPrivate->dbg, "[MP3 decoder] - fail to send command to audio manager\n");
            }
#endif
        break;
    case OMX_IndexCustomMp3DecHeaderInfoConfig:
        configData = (TI_OMX_DSP_DEFINITION*)ComponentConfigStructure;


        if (configData == NULL) {
            eError = OMX_ErrorBadParameter;
            OMX_ERROR4(pComponentPrivate->dbg, "%d :: OMX_ErrorBadParameter from SetConfig\n",__LINE__);
            goto EXIT;
        }
        if (configData->dasfMode == 0) {
            pComponentPrivate->dasfmode = 0;
        }
        else if (configData->dasfMode == 1) {
            pComponentPrivate->dasfmode = 1;
        }
        else if (configData->dasfMode == 2) {
            pComponentPrivate->dasfmode = 1;
        }
        pComponentPrivate->frameMode = configData->framemode;
        pComponentPrivate->streamID = configData->streamId;
        
        if(configData->mpeg1_layer2 == 1)
            pComponentPrivate->mpeg1_layer2 = 1;
        else
            pComponentPrivate->mpeg1_layer2 = 0;
        
        
        break;
    case  OMX_IndexCustomMp3DecDataPath:
        deviceString = (OMX_S16*)ComponentConfigStructure;
        if (deviceString == NULL) {
            eError = OMX_ErrorBadParameter;
            goto EXIT;
        }
        dataPath = *deviceString;
        switch(dataPath) {
        case DATAPATH_APPLICATION:
            /* strcpy((char*)pComponentPrivate->sDeviceString,(char*)ETEEDN_STRING);*/
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
    case MP3D_OMX_IndexCustomModeDasfConfig:
        OMX_PRDSP2(pComponentPrivate->dbg, ":: OMX_IndexCustomModeDasfConfig \n");
        customFlag = (int*)ComponentConfigStructure;
        if (customFlag == NULL) {
            eError = OMX_ErrorBadParameter;
            OMX_ERROR4(pComponentPrivate->dbg, ":: OMX_ErrorBadParameter from SetConfig\n");
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
            
        break;
    case MP3D_OMX_IndexCustomMode16_24bit:
        OMX_PRDSP2(pComponentPrivate->dbg, " :: OMX_IndexCustomMode16_24bit \n");
        customFlag = (int*)ComponentConfigStructure;
        if (customFlag == NULL) {
            eError = OMX_ErrorBadParameter;
            OMX_ERROR4(pComponentPrivate->dbg, ":: OMX_ErrorBadParameter from SetConfig\n");
            goto EXIT;
        }
        pComponentPrivate->nOpBit = *customFlag;
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
	 OMX_PRINT1(pComponentPrivate->dbg, ":: Exiting SetConfig\n");
	 OMX_PRINT1(pComponentPrivate->dbg, " :: Returning = 0x%x\n", eError);
    }
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

    MP3DEC_COMPONENT_PRIVATE *pComponentPrivate = NULL;


    MP3D_OMX_CONF_CHECK_CMD(pHandle,1,1);

    pComponentPrivate = (MP3DEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;

    MP3D_OMX_CONF_CHECK_CMD(pComponentPrivate,1,1);

    OMX_PRINT1(pComponentPrivate->dbg, "Entering SetCallbacks\n");

    MP3D_OMX_CONF_CHECK_CMD(pCallBacks, pCallBacks->EventHandler, pCallBacks->EmptyBufferDone);
    MP3D_OMX_CONF_CHECK_CMD(pCallBacks->FillBufferDone, 1, 1);

    memcpy (&(pComponentPrivate->cbInfo), pCallBacks, sizeof(OMX_CALLBACKTYPE));
    pHandle->pApplicationPrivate = pAppData;
    OMX_PRSTATE2(pComponentPrivate->dbg, "****************** Component State Set to Loaded\n\n");
    pComponentPrivate->curState = OMX_StateLoaded;
 EXIT:
    if (pComponentPrivate != NULL) {
	 OMX_PRINT1(pComponentPrivate->dbg, ":: Exiting SetCallbacks\n");
    }
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
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE*) hComp;
#ifdef _ERROR_PROPAGATION__ 
    MP3DEC_COMPONENT_PRIVATE *pComponentPrivate = (MP3DEC_COMPONENT_PRIVATE *) pHandle->pComponentPrivate;
#endif
    eError = OMX_ErrorNotImplemented;
    
#ifdef _ERROR_PROPAGATION__
    if (pComponentPrivate->curState == OMX_StateInvalid){
        eError = OMX_ErrorInvalidState;
        goto EXIT;
    }   
#endif

    OMX_PRINT1(pComponentPrivate->dbg, ":: Entering GetComponentVersion\n");
    OMX_PRINT2(pComponentPrivate->dbg, ":: Inside  GetComponentVersion\n");
    OMX_PRINT1(pComponentPrivate->dbg, " :: Exiting GetComponentVersion\n");
 EXIT:   
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
    
    MP3DEC_COMPONENT_PRIVATE *pComponentPrivate;
    TI_OMX_STREAM_INFO *streamInfo;
    
    pComponentPrivate = (MP3DEC_COMPONENT_PRIVATE *)(((OMX_COMPONENTTYPE*)hComp)->pComponentPrivate);
        
#ifdef _ERROR_PROPAGATION__
    if (pComponentPrivate->curState == OMX_StateInvalid){
        eError = OMX_ErrorInvalidState;
        goto EXIT;
    }   
#endif
    
    OMX_MALLOC_GENERIC(streamInfo,TI_OMX_STREAM_INFO);
    
    if(nConfigIndex == OMX_IndexCustomMp3DecStreamInfoConfig) {
        streamInfo->streamId = pComponentPrivate->streamID;
        MP3D_OMX_CONF_CHECK_CMD(ComponentConfigStructure, 1, 1);
        memcpy(ComponentConfigStructure,streamInfo,sizeof(TI_OMX_STREAM_INFO));
    } else if(nConfigIndex == OMX_IndexCustomDebug) {
	OMX_DBG_GETCONFIG(pComponentPrivate->dbg, ComponentConfigStructure);
    }
    OMX_MEMFREE_STRUCT(streamInfo);
    
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
	OMXDBG_PRINT(stderr, ERROR, 4, 0, " :: About to exit with bad parameter\n");
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }

    MP3D_OMX_CONF_CHECK_CMD(pHandle,1,1);

    if (pHandle && pHandle->pComponentPrivate) {
        *pState =  ((MP3DEC_COMPONENT_PRIVATE*)
                    pHandle->pComponentPrivate)->curState;
        /*OMX_PRINT1(pComponentPrivate->dbg, " :: curState = %d\n",(int)*pState);*/
    } else {
	OMXDBG_PRINT(stderr, STATE, 2, 0, "Component State Set to Loaded\n\n");
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
    MP3DEC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef;
    int ret=0;
    
    MP3D_OMX_CONF_CHECK_CMD(pHandle,1,1);
    pComponentPrivate = (MP3DEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    MP3D_OMX_CONF_CHECK_CMD(pComponentPrivate,1,1) ;

    pPortDef = ((MP3DEC_COMPONENT_PRIVATE*)pComponentPrivate)->pPortDef[MP3D_INPUT_PORT];

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
        eError = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }

    if (pBuffer == NULL) {
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }

    if (pBuffer->nSize != sizeof(OMX_BUFFERHEADERTYPE)) {
        MP3D_OMX_ERROR_EXIT(eError,OMX_ErrorBadParameter,"Bad Size");
    }

    if (pBuffer->nInputPortIndex != MP3D_INPUT_PORT) {
        MP3D_OMX_ERROR_EXIT(eError,OMX_ErrorBadPortIndex,"OMX_ErrorBadPortIndex");
    }


    if (pBuffer->nVersion.nVersion != pComponentPrivate->nVersion) {
        MP3D_OMX_ERROR_EXIT(eError,OMX_ErrorVersionMismatch,"OMX_ErrorVersionMismatch");
    }

    if(pComponentPrivate->curState != OMX_StateExecuting && pComponentPrivate->curState != OMX_StatePause &&
       pComponentPrivate->curState != OMX_StateIdle) {
        eError = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }

    OMX_PRCOMM2(pComponentPrivate->dbg, "------------------------------------------\n");
    OMX_PRCOMM2(pComponentPrivate->dbg, "Sending Filled IN buff %p\n",pBuffer);
    OMX_PRCOMM2(pComponentPrivate->dbg, "------------------------------------------\n");

    if (pComponentPrivate->bBypassDSP == 0) {
        pComponentPrivate->app_nBuf--;
    }

    pComponentPrivate->pMarkData = pBuffer->pMarkData;
    pComponentPrivate->hMarkTargetComponent = pBuffer->hMarkTargetComponent;

    ret = write (pComponentPrivate->dataPipe[1], &pBuffer,
                 sizeof(OMX_BUFFERHEADERTYPE*));
    if (ret == -1) {
        MP3D_OMX_ERROR_EXIT(eError,OMX_ErrorHardware,"write failed: OMX_ErrorHardware");
    }else{
        pComponentPrivate->nEmptyThisBufferCount++;
    }
    
 EXIT:
    if (pComponentPrivate != NULL) {
	 OMX_PRINT1(pComponentPrivate->dbg, " :: Exiting EmptyThisBuffer\n");
    }
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
    MP3DEC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    int nRet=0;
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef;

    MP3D_OMX_CONF_CHECK_CMD(pHandle,1,1);
    pComponentPrivate = (MP3DEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    MP3D_OMX_CONF_CHECK_CMD(pComponentPrivate,1,1);

    pPortDef = ((MP3DEC_COMPONENT_PRIVATE*) pComponentPrivate)->pPortDef[MP3D_OUTPUT_PORT];
    
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
        eError = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }

    if (pBuffer == NULL) {
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }

    if (pBuffer->nSize != sizeof(OMX_BUFFERHEADERTYPE)) {
        OMX_ERROR4(pComponentPrivate->dbg, " :: FillThisBuffer: Bad Size\n");
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }

    if (pBuffer->nOutputPortIndex != MP3D_OUTPUT_PORT) {
        OMX_ERROR4(pComponentPrivate->dbg, " :: EmptyThisBuffer: BadPortIndex\n");
        eError  = OMX_ErrorBadPortIndex;
        goto EXIT;
    }

    OMX_PRBUFFER2(pComponentPrivate->dbg, "::pBuffer->nVersion.nVersion:%ld\n",pBuffer->nVersion.nVersion);
    OMX_PRBUFFER2(pComponentPrivate->dbg, "::pComponentPrivate->nVersion:%ld\n",pComponentPrivate->nVersion);

    if (pBuffer->nVersion.nVersion != pComponentPrivate->nVersion) {
        OMX_ERROR4(pComponentPrivate->dbg, ":: FillThisBuffer: BufferHeader Version Mismatch\n");
        OMX_ERROR4(pComponentPrivate->dbg, ":pBuffer->nVersion.nVersion:%ld\n",pBuffer->nVersion.nVersion);
        OMX_ERROR4(pComponentPrivate->dbg, "::pComponentPrivate->nVersion:%ld\n",pComponentPrivate->nVersion);

        eError = OMX_ErrorVersionMismatch;
        goto EXIT;
    }

    if(pComponentPrivate->curState != OMX_StateExecuting && pComponentPrivate->curState != OMX_StatePause) {
        eError = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }

    pBuffer->nFilledLen = 0;

    if (pComponentPrivate->bBypassDSP == 0) {
        pComponentPrivate->app_nBuf--;
    }

    if(pComponentPrivate->pMarkBuf){
        OMX_PRBUFFER2(pComponentPrivate->dbg, "FillThisBuffer Line \n");
        pBuffer->hMarkTargetComponent = pComponentPrivate->pMarkBuf->hMarkTargetComponent;
        pBuffer->pMarkData = pComponentPrivate->pMarkBuf->pMarkData;
        pComponentPrivate->pMarkBuf = NULL;
    }

    if (pComponentPrivate->pMarkData) {
        OMX_PRBUFFER2(pComponentPrivate->dbg, "FillThisBuffer Line \n");
        pBuffer->hMarkTargetComponent = pComponentPrivate->hMarkTargetComponent;
        pBuffer->pMarkData = pComponentPrivate->pMarkData;
        pComponentPrivate->pMarkData = NULL;
    }

    OMX_PRCOMM2(pComponentPrivate->dbg, "------------------------------------------\n");
    OMX_PRCOMM2(pComponentPrivate->dbg, "Sending Emptied OUT buff %p\n",pBuffer);
    OMX_PRCOMM2(pComponentPrivate->dbg, "------------------------------------------\n");

    nRet = write (pComponentPrivate->dataPipe[1], &pBuffer,
                  sizeof (OMX_BUFFERHEADERTYPE*));
    if (nRet == -1) {
        MP3D_OMX_ERROR_EXIT(eError,OMX_ErrorHardware,"write failed: OMX_ErrorHardware");
    }else{
        pComponentPrivate->nFillThisBufferCount++;
    }

 EXIT:
    if (pComponentPrivate != NULL) {
	 OMX_PRINT1(pComponentPrivate->dbg, ": Exiting FillThisBuffer\n");
    }
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
* @post        This function should clean or newfree as much resources as
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
    MP3DEC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    int k=0, k2 = 0;
    struct OMX_TI_Debug dbg = {0};

    MP3D_OMX_CONF_CHECK_CMD(pComponent,1,1)
        pComponentPrivate = (MP3DEC_COMPONENT_PRIVATE *)pComponent->pComponentPrivate;
    MP3D_OMX_CONF_CHECK_CMD(pComponentPrivate,1,1)
    dbg = pComponentPrivate->dbg;
#ifdef __PERF_INSTRUMENTATION__
        PERF_Boundary(pComponentPrivate->pPERF,
                      PERF_BoundaryStart | PERF_BoundaryCleanup);
#endif
    
#ifdef DSP_RENDERING_ON
    close(mp3d_fdwrite);
    close(mp3d_fdread);
#endif

#ifdef RESOURCE_MANAGER_ENABLED
    /*eError1 = RMProxy_SendCommand(pHandle, RMProxy_FreeResource, OMX_MP3_Decoder_COMPONENT, 0, NULL);*/
    eError1 = RMProxy_NewSendCommand(pHandle, RMProxy_FreeResource, OMX_MP3_Decoder_COMPONENT, 0, 3456, NULL);
    if (eError1 != OMX_ErrorNone) {
        OMX_ERROR4(dbg, "%d ::Error returned from destroy ResourceManagerProxy thread\n",__LINE__);
    }

    eError1 = RMProxy_Deinitalize();
    if (eError1 != OMX_ErrorNone) {
        OMX_ERROR4(dbg, ":: First Error in ComponentDeinit: From RMProxy_Deinitalize\n");
        eError = eError1;
    }
#endif

    pComponentPrivate->bExitCompThrd = 1;
    write (pComponentPrivate->cmdPipe[1], &pComponentPrivate->bExitCompThrd, sizeof(OMX_U16));
    k = pthread_join(pComponentPrivate->ComponentThread, (void*)&k2);
    if(0 != k) {
        if (OMX_ErrorNone == eError) {
            OMX_ERROR4(dbg, ":: First Error in ComponentDeinit: From pthread_join\n");
            eError = OMX_ErrorHardware;
        }
    } 
    eError1 = MP3DEC_FreeCompResources(pHandle);
    if (OMX_ErrorNone != eError1) {
        if (OMX_ErrorNone == eError) {
            OMX_ERROR4(dbg, ":: First Error in ComponentDeinit: From FreeCompResources\n");
            eError = eError1;
        }
    }

#ifdef __PERF_INSTRUMENTATION__
    PERF_Boundary(pComponentPrivate->pPERF,
                  PERF_BoundaryComplete | PERF_BoundaryCleanup);
    PERF_Done(pComponentPrivate->pPERF);
#endif
    OMX_MEMFREE_STRUCT(pComponentPrivate->sDeviceString);

    OMX_PRBUFFER2(dbg, ":: Freeing: pComponentPrivate = %p\n",pComponentPrivate);
    OMX_PRINT1(dbg, "::*********** ComponentDeinit is Done************** \n");
 EXIT:
    OMX_DBG_CLOSE(dbg);
    OMX_MEMFREE_STRUCT(pComponentPrivate);
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
    OMXDBG_PRINT(stderr, PRINT, 1, 0, ":: Entering ComponentTunnelRequest\n");
    OMXDBG_PRINT(stderr, PRINT, 2, 0, " :: Inside   ComponentTunnelRequest\n");
    eError = OMX_ErrorNotImplemented;
    OMXDBG_PRINT(stderr, PRINT, 1, 0, " :: Exiting ComponentTunnelRequest\n");
    return eError;
}

static void waitForAlloBufThreshold(
        MP3DEC_COMPONENT_PRIVATE *pComponentPrivate) {
#ifndef UNDER_CE
    pthread_mutex_lock(&pComponentPrivate->AlloBuf_mutex);
    pComponentPrivate->AlloBuf_waitingsignal = 1;
    while (pComponentPrivate->AlloBuf_waitingsignal) {
        pthread_cond_wait(
                &pComponentPrivate->AlloBuf_threshold,
                &pComponentPrivate->AlloBuf_mutex);
    }
    pthread_mutex_unlock(&pComponentPrivate->AlloBuf_mutex);
#else
    // I am fairly sure this will suffer from similar issues without
    // proper mutex protection and a loop under WinCE...
    pComponentPrivate->AlloBuf_waitingsignal = 1;
    OMX_WaitForEvent(&(pComponentPrivate->AlloBuf_event));
#endif
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
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef;
    MP3DEC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_BUFFERHEADERTYPE *pBufferHeader = NULL;


    MP3D_OMX_CONF_CHECK_CMD(hComponent,1,1);
    pComponentPrivate = (MP3DEC_COMPONENT_PRIVATE *)(((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);
    OMX_PRINT1 (pComponentPrivate->dbg,"Entering AllocateBuffer\n");
    
    MP3D_OMX_CONF_CHECK_CMD(pComponentPrivate, 1, 1);
#ifdef _ERROR_PROPAGATION__
    if (pComponentPrivate->curState == OMX_StateInvalid){
        eError = OMX_ErrorInvalidState;
        goto EXIT;
    }   
#endif

    pPortDef = ((MP3DEC_COMPONENT_PRIVATE*)pComponentPrivate)->pPortDef[nPortIndex];

    MP3D_OMX_CONF_CHECK_CMD(pPortDef, 1, 1);

    if(!pPortDef->bEnabled){
        waitForAlloBufThreshold(pComponentPrivate);
    }

    OMX_MALLOC_GENERIC(pBufferHeader, OMX_BUFFERHEADERTYPE);
    memset((pBufferHeader), 0x0, sizeof(OMX_BUFFERHEADERTYPE));

    /* This extra 256 bytes memory is required to avoid DSP caching issues */
    OMX_MALLOC_SIZE_DSPALIGN(pBufferHeader->pBuffer, nSizeBytes, OMX_U8);
    OMX_PRBUFFER2(pComponentPrivate->dbg, ": Malloced = %p\n",pBufferHeader->pBuffer);
    pBufferHeader->nVersion.nVersion = MP3DEC_BUFHEADER_VERSION;

    OMX_PRBUFFER2(pComponentPrivate->dbg, "********************************************\n");
    OMX_PRBUFFER2(pComponentPrivate->dbg, " :: Allocated BufHeader %p Buffer = %p, on port %ld\n",pBufferHeader,
                  pBufferHeader->pBuffer, nPortIndex);

    OMX_PRBUFFER2(pComponentPrivate->dbg, " :: Ip Num = %ld\n", pComponentPrivate->pInputBufferList->numBuffers);
    OMX_PRBUFFER2(pComponentPrivate->dbg, " :: Op Num = %ld\n", pComponentPrivate->pOutputBufferList->numBuffers);
    OMX_PRBUFFER2(pComponentPrivate->dbg, "********************************************\n");

    pBufferHeader->pAppPrivate = pAppPrivate;
    pBufferHeader->pPlatformPrivate = pComponentPrivate;
    pBufferHeader->nAllocLen = nSizeBytes;

#ifdef __PERF_INSTRUMENTATION__
    PERF_ReceivedBuffer(pComponentPrivate->pPERF,
                        (*pBuffer)->pBuffer, nSizeBytes,
                        PERF_ModuleMemory);
#endif
    
    if (nPortIndex == MP3D_INPUT_PORT) {
        pBufferHeader->nInputPortIndex = nPortIndex;
        pBufferHeader->nOutputPortIndex = -1;
        pComponentPrivate->pInputBufferList->pBufHdr[pComponentPrivate->pInputBufferList->numBuffers] = pBufferHeader;
        pComponentPrivate->pInputBufferList->bBufferPending[pComponentPrivate->pInputBufferList->numBuffers] = 0;

        OMX_PRBUFFER2(pComponentPrivate->dbg, "pComponentPrivate->pInputBufferList->pBufHdr[%ld] = %p\n", pComponentPrivate->pInputBufferList->numBuffers,
                      pComponentPrivate->pInputBufferList->pBufHdr[pComponentPrivate->pInputBufferList->numBuffers]);

        pComponentPrivate->pInputBufferList->bufferOwner[pComponentPrivate->pInputBufferList->numBuffers++] = 1;

        OMX_PRBUFFER2(pComponentPrivate->dbg, "pComponentPrivate->pInputBufferList->numBuffers = %ld\n",pComponentPrivate->pInputBufferList->numBuffers);
        OMX_PRBUFFER2(pComponentPrivate->dbg, "pPortDef->nBufferCountMin = %ld\n",pPortDef->nBufferCountMin);

        if (pComponentPrivate->pInputBufferList->numBuffers == pPortDef->nBufferCountActual) {
            pPortDef->bPopulated = 1;
        }
    } else if (nPortIndex == MP3D_OUTPUT_PORT) {
        pBufferHeader->nInputPortIndex = -1;
        pBufferHeader->nOutputPortIndex = nPortIndex;
        OMX_MALLOC_GENERIC(pBufferHeader->pOutputPortPrivate,MP3DEC_BUFDATA);
        pComponentPrivate->pOutputBufferList->pBufHdr[pComponentPrivate->pOutputBufferList->numBuffers] = pBufferHeader;

        pComponentPrivate->pOutputBufferList->bBufferPending[pComponentPrivate->pOutputBufferList->numBuffers] = 0;


        OMX_PRBUFFER2(pComponentPrivate->dbg, "pComponentPrivate->pOutputBufferList->pBufHdr[%ld] = %p\n",pComponentPrivate->pOutputBufferList->numBuffers,pComponentPrivate->pOutputBufferList->pBufHdr[pComponentPrivate->pOutputBufferList->numBuffers]);

        pComponentPrivate->pOutputBufferList->bufferOwner[pComponentPrivate->pOutputBufferList->numBuffers++] = 1;

        if (pComponentPrivate->pOutputBufferList->numBuffers == pPortDef->nBufferCountActual) {
            pPortDef->bPopulated = 1;
        }

    } else {
        MP3D_OMX_ERROR_EXIT(eError,OMX_ErrorBadPortIndex,"OMX_ErrorBadPortIndex");
    }

    if((pComponentPrivate->pPortDef[MP3D_OUTPUT_PORT]->bPopulated == pComponentPrivate->pPortDef[MP3D_OUTPUT_PORT]->bEnabled)&&
       (pComponentPrivate->pPortDef[MP3D_INPUT_PORT]->bPopulated == pComponentPrivate->pPortDef[MP3D_INPUT_PORT]->bEnabled) &&
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
    pBufferHeader->nVersion.s.nVersionMajor = MP3DEC_MAJOR_VER;
    pBufferHeader->nVersion.s.nVersionMinor = MP3DEC_MINOR_VER;
    pComponentPrivate->nVersion = pBufferHeader->nVersion.nVersion;


    pBufferHeader->nSize = sizeof(OMX_BUFFERHEADERTYPE);

    *pBuffer = pBufferHeader;
    pComponentPrivate->bufAlloced = 1;
    
    if (pComponentPrivate->bEnableCommandPending && pPortDef->bPopulated) {
        SendCommand (pComponentPrivate->pHandle,
                     OMX_CommandPortEnable,
                     pComponentPrivate->bEnableCommandParam,NULL);
    }

 EXIT:
    if(OMX_ErrorNone != eError) {
	 if (pComponentPrivate != NULL) {
	     OMX_ERROR4(pComponentPrivate->dbg, "%d :: ************* ERROR: Freeing Other Malloced Resources\n", __LINE__);
	}
	if (pBufferHeader != NULL) {
	     OMX_MEMFREE_STRUCT_DSPALIGN(pBufferHeader->pBuffer, OMX_U8);
	     OMX_MEMFREE_STRUCT(pBufferHeader);
	}
    }
    return eError;
}

/* ================================================================================= * */
/**
* @fn FreeBuffer() This function newfrees the meomory of the buffer specified.
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
    MP3DEC_COMPONENT_PRIVATE * pComponentPrivate = NULL;
    OMX_U8* buff;
    int i;
    int inputIndex = -1;
    int outputIndex = -1;
    OMX_COMPONENTTYPE *pHandle;


    pComponentPrivate = (MP3DEC_COMPONENT_PRIVATE *) (((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);
    OMX_PRINT1(pComponentPrivate->dbg, ":: Entering FreeBuffer\n");

    pHandle = (OMX_COMPONENTTYPE *) pComponentPrivate->pHandle;
    OMX_PRINT2(pComponentPrivate->dbg, ":: pComponentPrivate = %p\n",pComponentPrivate);
    for (i=0; i < MP3D_MAX_NUM_OF_BUFS; i++) {
        buff = (OMX_U8 *)pComponentPrivate->pInputBufferList->pBufHdr[i];
        if (buff == (OMX_U8 *)pBuffer) {
            OMX_PRINT2(pComponentPrivate->dbg, "Found matching input buffer\n");
            OMX_PRBUFFER2(pComponentPrivate->dbg, "buff = %p\n",buff);
            OMX_PRBUFFER2(pComponentPrivate->dbg, "pBuffer = %p\n",pBuffer);
            inputIndex = i;
            break;
        }
    }

    for (i=0; i < MP3D_MAX_NUM_OF_BUFS; i++) {
        buff = (OMX_U8 *)pComponentPrivate->pOutputBufferList->pBufHdr[i];
        if (buff == (OMX_U8 *)pBuffer) {
            OMX_PRINT2(pComponentPrivate->dbg, "Found matching output buffer\n");
            OMX_PRBUFFER2(pComponentPrivate->dbg, "buff = %p\n",buff);
            OMX_PRBUFFER2(pComponentPrivate->dbg, "pBuffer = %p\n",pBuffer);
            outputIndex = i;
            break;
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
                           PERF_ModuleMemory);
#endif
        OMX_PRBUFFER2(pComponentPrivate->dbg, "Freeing: %p IP Buf Header\n\n",
                        pComponentPrivate->pInputBufferList->pBufHdr[inputIndex]);

        OMX_MEMFREE_STRUCT(pComponentPrivate->pInputBufferList->pBufHdr[inputIndex]);
        pComponentPrivate->pInputBufferList->pBufHdr[inputIndex] = NULL;
        pComponentPrivate->pInputBufferList->numBuffers--;

        if (pComponentPrivate->pInputBufferList->numBuffers <
            pComponentPrivate->pPortDef[MP3D_INPUT_PORT]->nBufferCountMin) {
            pComponentPrivate->pPortDef[MP3D_INPUT_PORT]->bPopulated = OMX_FALSE;
        }
        
#ifndef UNDER_CE        
        if(pComponentPrivate->pPortDef[MP3D_INPUT_PORT]->bEnabled &&
           pComponentPrivate->bLoadedCommandPending == OMX_FALSE &&
           !pComponentPrivate->reconfigInputPort &&
           (pComponentPrivate->curState == OMX_StateIdle ||
            pComponentPrivate->curState == OMX_StateExecuting ||
            pComponentPrivate->curState == OMX_StatePause)) {
                
            pComponentPrivate->cbInfo.EventHandler(pHandle, 
                                                   pHandle->pApplicationPrivate,
                                                   OMX_EventError, 
                                                   OMX_ErrorPortUnpopulated,
                                                   nPortIndex, 
                                                   NULL);
        }
                
#else
        if(pComponentPrivate->pPortDef[MP3D_INPUT_PORT]->bEnabled &&
           pComponentPrivate->bLoadedCommandPending == OMX_TRUE &&
           (pComponentPrivate->curState == OMX_StateIdle ||
            pComponentPrivate->curState == OMX_StateExecuting ||
            pComponentPrivate->curState == OMX_StatePause)) {

            pComponentPrivate->cbInfo.EventHandler(pHandle, 
                                                   pHandle->pApplicationPrivate,
                                                   OMX_EventError, 
                                                   OMX_ErrorPortUnpopulated,
                                                   nPortIndex, 
                                                   NULL);
        }
#endif                
    } else if (outputIndex != -1) {
        if (pComponentPrivate->pOutputBufferList->bBufferPending[outputIndex]) {
            pComponentPrivate->numPendingBuffers++;
        }

        if (pComponentPrivate->pOutputBufferList->bufferOwner[outputIndex] == 1) {
            OMX_MEMFREE_STRUCT_DSPALIGN(pComponentPrivate->pOutputBufferList->pBufHdr[outputIndex]->pBuffer, OMX_U8);
        }
#ifdef __PERF_INSTRUMENTATION__
        PERF_SendingBuffer(pComponentPrivate->pPERF,
                           pComponentPrivate->pOutputBufferList->pBufHdr[outputIndex]->pBuffer, 
                           pComponentPrivate->pOutputBufferList->pBufHdr[outputIndex]->nAllocLen,
                           PERF_ModuleMemory);
#endif
        OMX_PRBUFFER2(pComponentPrivate->dbg, " Freeing: %p OP Buf Header\n\n",
                        pComponentPrivate->pOutputBufferList->pBufHdr[outputIndex]);
        OMX_MEMFREE_STRUCT(pComponentPrivate->pOutputBufferList->pBufHdr[outputIndex]->pOutputPortPrivate);
        OMX_MEMFREE_STRUCT(pComponentPrivate->pOutputBufferList->pBufHdr[outputIndex]);
        pComponentPrivate->pOutputBufferList->pBufHdr[outputIndex] = NULL;
        pComponentPrivate->pOutputBufferList->numBuffers--;

        if (pComponentPrivate->pOutputBufferList->numBuffers <
            pComponentPrivate->pPortDef[MP3D_OUTPUT_PORT]->nBufferCountMin) {
            pComponentPrivate->pPortDef[MP3D_OUTPUT_PORT]->bPopulated = OMX_FALSE;
        }
#ifndef UNDER_CE        
        if(pComponentPrivate->pPortDef[MP3D_OUTPUT_PORT]->bEnabled &&
           pComponentPrivate->bLoadedCommandPending == OMX_FALSE &&
           !pComponentPrivate->reconfigOutputPort &&
           (pComponentPrivate->curState == OMX_StateIdle ||
            pComponentPrivate->curState == OMX_StateExecuting ||
            pComponentPrivate->curState == OMX_StatePause)) {

            pComponentPrivate->cbInfo.EventHandler(pHandle, 
                                                   pHandle->pApplicationPrivate,
                                                   OMX_EventError, 
                                                   OMX_ErrorPortUnpopulated,
                                                   nPortIndex, 
                                                   NULL);
        }
#else
        if(pComponentPrivate->pPortDef[MP3D_OUTPUT_PORT]->bEnabled &&
           pComponentPrivate->bLoadedCommandPending == OMX_TRUE &&
           (pComponentPrivate->curState == OMX_StateIdle ||
            pComponentPrivate->curState == OMX_StateExecuting ||
            pComponentPrivate->curState == OMX_StatePause)) {
            pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                   pHandle->pApplicationPrivate,
                                                   OMX_EventError, 
                                                   OMX_ErrorPortUnpopulated,
                                                   nPortIndex,
                                                   NULL);
        }
#endif
    } else {
        OMX_ERROR4(pComponentPrivate->dbg, ":Returning OMX_ErrorBadParameter\n");
        eError = OMX_ErrorBadParameter;
    }

    if ((!pComponentPrivate->pInputBufferList->numBuffers &&
         !pComponentPrivate->pOutputBufferList->numBuffers) &&
        pComponentPrivate->InIdle_goingtoloaded){
        pComponentPrivate->InIdle_goingtoloaded = 0;                  

        pthread_mutex_lock(&pComponentPrivate->InIdle_mutex);
        pthread_cond_signal(&pComponentPrivate->InIdle_threshold);
        pthread_mutex_unlock(&pComponentPrivate->InIdle_mutex);

    }
    
///
   if ((pComponentPrivate->bDisableCommandPending) &&
         (pComponentPrivate->pInputBufferList->numBuffers == 0))
    {
        OMX_PRCOMM2(pComponentPrivate->dbg, "calling command completed for input port disable\n");
        pComponentPrivate->bDisableCommandPending = 0;
        pComponentPrivate->cbInfo.EventHandler( pComponentPrivate->pHandle,
                                                pComponentPrivate->pHandle->pApplicationPrivate,
                                                        OMX_EventCmdComplete,
                                                        OMX_CommandPortDisable,
                                                        INPUT_PORT_MP3DEC,
                                                        NULL);
    }
    
  
  if ((pComponentPrivate->bDisableCommandPending) &&
         (pComponentPrivate->pOutputBufferList->numBuffers == 0))
    {
        OMX_PRCOMM2(pComponentPrivate->dbg, "calling command completed for output port disable\n");
        pComponentPrivate->bDisableCommandPending = 0;
        pComponentPrivate->cbInfo.EventHandler( pComponentPrivate->pHandle,
                                                pComponentPrivate->pHandle->pApplicationPrivate,
                                                OMX_EventCmdComplete,
                                                OMX_CommandPortDisable,
                                                OUTPUT_PORT_MP3DEC,
                                                NULL);
    } 



    pComponentPrivate->bufAlloced = 0;
    OMX_PRINT1(pComponentPrivate->dbg, ":: Exiting FreeBuffer\n");
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
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef;
    MP3DEC_COMPONENT_PRIVATE *pComponentPrivate;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_BUFFERHEADERTYPE *pBufferHeader;

    OMXDBG_PRINT(stderr, PRINT, 1, 0, ":: Entering UseBuffer\n");

    pComponentPrivate = (MP3DEC_COMPONENT_PRIVATE *)(((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);

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

    pPortDef = ((MP3DEC_COMPONENT_PRIVATE*)
                pComponentPrivate)->pPortDef[nPortIndex];

    MP3D_OMX_CONF_CHECK_CMD(pPortDef, 1, 1);
    if(!pPortDef->bEnabled) {
        waitForAlloBufThreshold(pComponentPrivate);
    }
    OMX_PRCOMM2(pComponentPrivate->dbg, ":: pPortDef = %p\n",pPortDef);
    OMX_PRCOMM2(pComponentPrivate->dbg, ":: pPortDef->bEnabled = %d\n",pPortDef->bEnabled);

    if(!pPortDef->bEnabled) {
        MP3D_OMX_ERROR_EXIT(eError,OMX_ErrorIncorrectStateOperation,
                            "Port is Disabled: OMX_ErrorIncorrectStateOperation");
    }
#ifndef UNDER_CE
    if(pPortDef->bPopulated) {
        MP3D_OMX_ERROR_EXIT(eError,OMX_ErrorBadParameter,
                            "Bad Size or Port Disabled : OMX_ErrorBadParameter");
    }
#endif  

    OMX_MALLOC_GENERIC(pBufferHeader, OMX_BUFFERHEADERTYPE);
    memset((pBufferHeader), 0x0, sizeof(OMX_BUFFERHEADERTYPE));

    if (nPortIndex == MP3D_OUTPUT_PORT) {
        pBufferHeader->nInputPortIndex = -1;
        pBufferHeader->nOutputPortIndex = nPortIndex;
        OMX_MALLOC_GENERIC(pBufferHeader->pOutputPortPrivate,MP3DEC_BUFDATA);
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

    if((pComponentPrivate->pPortDef[MP3D_OUTPUT_PORT]->bPopulated == pComponentPrivate->pPortDef[MP3D_OUTPUT_PORT]->bEnabled)&&
       (pComponentPrivate->pPortDef[MP3D_INPUT_PORT]->bPopulated == pComponentPrivate->pPortDef[MP3D_INPUT_PORT]->bEnabled) &&
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
    pBufferHeader->nVersion.s.nVersionMajor = MP3DEC_MAJOR_VER;
    pBufferHeader->nVersion.s.nVersionMinor = MP3DEC_MINOR_VER;
    pComponentPrivate->nVersion = pBufferHeader->nVersion.nVersion;
    pBufferHeader->pBuffer = pBuffer;
    pBufferHeader->nSize = sizeof(OMX_BUFFERHEADERTYPE);
    *ppBufferHdr = pBufferHeader;
    pComponentPrivate->bufAlloced = 1;

    if (pComponentPrivate->bEnableCommandPending){
        SendCommand (pComponentPrivate->pHandle,
                     OMX_CommandPortEnable,
                     pComponentPrivate->bEnableCommandParam,NULL);
    }

    OMX_PRINT1(pComponentPrivate->dbg, "pBufferHeader = %p\n",pBufferHeader);
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

    if(!(strcmp(cParameterName,"OMX.TI.index.config.mp3headerinfo"))) {
        *pIndexType = OMX_IndexCustomMp3DecHeaderInfoConfig;
    }
    else if (!(strcmp(cParameterName,"OMX.TI.index.config.mp3streamIDinfo"))) {
        *pIndexType = OMX_IndexCustomMp3DecStreamInfoConfig;
    }
    else if(!(strcmp(cParameterName,"OMX.TI.index.config.mp3.datapath"))){
        *pIndexType = OMX_IndexCustomMp3DecDataPath;
    }
    else if(!(strcmp(cParameterName,"OMX.TI.MP3.Decode.Debug"))){
        *pIndexType = OMX_IndexCustomDebug;
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
    MP3DEC_COMPONENT_PRIVATE *pComponentPrivate;
   
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    pComponentPrivate = (MP3DEC_COMPONENT_PRIVATE *)(((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);
    if (cRole == NULL){
        eError = OMX_ErrorBadParameter;
    }
    else if(nIndex == 0){
        memcpy(cRole, &pComponentPrivate->componentRole.cRole, sizeof(OMX_U8) * OMX_MAX_STRINGNAME_SIZE); 
        OMX_PRINT1(pComponentPrivate->dbg, "[ComponenetRoleEnum] cRole = %s\n",cRole);
    }
    else {
        eError = OMX_ErrorNoMore;
    }

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
        OMXDBG_PRINT(stderr, ERROR, 4, 0, "OMX_CreateEvent: OMX_ErrorBadParameter\n");
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
        OMXDBG_PRINT(stderr, ERROR, 4, 0, "OMX_CreateEvent: OMX_ErrorBadParameter\n");
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
        OMXDBG_PRINT(stderr, ERROR, 4, 0, "OMX_CreateEvent: OMX_ErrorBadParameter\n");
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
        OMXDBG_PRINT(stderr, ERROR, 4, 0, "OMX_CreateEvent: OMX_ErrorBadParameter\n");
        goto EXIT;
     }  
     CloseHandle(event->event);
EXIT:    
     return ret;
}
#endif

