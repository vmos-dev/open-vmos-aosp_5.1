
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
* @file OMX_AacEncoder.c
*
* This file implements OMX Component for AAC Encoder that
* is fully compliant with the OMX Audio specification 1.5.
*
* @path  $(CSLPATH)\
*
* @rev  1.0
*/
/* ----------------------------------------------------------------------------
*!
*! Revision History
*! ===================================
*! 13-Enc-2005 mf:  Initial Version. Change required per OMAPSWxxxxxxxxx
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

/*-------program files ----------------------------------------*/
#include "LCML_DspCodec.h"

#ifndef UNDER_CE
#ifdef DSP_RENDERING_ON
#include <AudioManagerAPI.h>
#endif
#endif
#ifdef RESOURCE_MANAGER_ENABLED
#include <ResourceManagerProxyAPI.h>
#endif
#include "OMX_AacEncoder.h"
#define  AAC_ENC_ROLE "audio_encoder.aac"
#include "OMX_AacEnc_Utils.h"
#include <TIDspOmx.h>

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

static OMX_ERRORTYPE SetCallbacks(OMX_HANDLETYPE hComp, OMX_CALLBACKTYPE* pCallBacks, OMX_PTR pAppData);
static OMX_ERRORTYPE GetComponentVersion(OMX_HANDLETYPE hComp, OMX_STRING pComponentName, OMX_VERSIONTYPE* pComponentVersion, OMX_VERSIONTYPE* pSpecVersion, OMX_UUIDTYPE* pComponentUUID);
static OMX_ERRORTYPE SendCommand(OMX_HANDLETYPE hComp, OMX_COMMANDTYPE nCommand, OMX_U32 nParam,OMX_PTR pCmdData);
static OMX_ERRORTYPE GetParameter(OMX_HANDLETYPE hComp, OMX_INDEXTYPE nParamIndex, OMX_PTR ComponentParamStruct);
static OMX_ERRORTYPE SetParameter(OMX_HANDLETYPE hComp, OMX_INDEXTYPE nParamIndex, OMX_PTR ComponentParamStruct);
static OMX_ERRORTYPE GetConfig(OMX_HANDLETYPE hComp, OMX_INDEXTYPE nConfigIndex, OMX_PTR pComponentConfigStructure);
static OMX_ERRORTYPE SetConfig(OMX_HANDLETYPE hComp, OMX_INDEXTYPE nConfigIndex, OMX_PTR pComponentConfigStructure);
static OMX_ERRORTYPE EmptyThisBuffer(OMX_HANDLETYPE hComp, OMX_BUFFERHEADERTYPE* pBuffer);
static OMX_ERRORTYPE FillThisBuffer(OMX_HANDLETYPE hComp, OMX_BUFFERHEADERTYPE* pBuffer);
static OMX_ERRORTYPE GetState(OMX_HANDLETYPE hComp, OMX_STATETYPE* pState);
static OMX_ERRORTYPE ComponentTunnelRequest(OMX_HANDLETYPE hComp, OMX_U32 nPort, OMX_HANDLETYPE hTunneledComp, OMX_U32 nTunneledPort, OMX_TUNNELSETUPTYPE* pTunnelSetup);
static OMX_ERRORTYPE ComponentDeInit(OMX_HANDLETYPE pHandle);
static OMX_ERRORTYPE AllocateBuffer(OMX_IN OMX_HANDLETYPE hComponent, OMX_INOUT OMX_BUFFERHEADERTYPE** pBuffer, OMX_IN OMX_U32 nPortIndex, OMX_IN OMX_PTR pAppPrivate, OMX_IN OMX_U32 nSizeBytes);
static OMX_ERRORTYPE FreeBuffer(OMX_IN  OMX_HANDLETYPE hComponent, OMX_IN OMX_U32 nPortIndex, OMX_IN OMX_BUFFERHEADERTYPE* pBuffer);
static OMX_ERRORTYPE UseBuffer(OMX_IN OMX_HANDLETYPE hComponent, OMX_INOUT OMX_BUFFERHEADERTYPE** ppBufferHdr, OMX_IN OMX_U32 nPortIndex, OMX_IN OMX_PTR pAppPrivate, OMX_IN OMX_U32 nSizeBytes, OMX_IN OMX_U8* pBuffer);
static OMX_ERRORTYPE GetExtensionIndex(OMX_IN  OMX_HANDLETYPE hComponent, OMX_IN  OMX_STRING cParameterName, OMX_OUT OMX_INDEXTYPE* pIndexType); 
static OMX_ERRORTYPE ComponentRoleEnum( OMX_IN OMX_HANDLETYPE hComponent,
                                                  OMX_OUT OMX_U8 *cRole,
                                                  OMX_IN OMX_U32 nIndex);


#ifdef DSP_RENDERING_ON

/* interface with audio manager*/
#define FIFO1 "/dev/fifo.1"
#define FIFO2 "/dev/fifo.2"

int Aacenc_fdwrite, Aacenc_fdread;

#ifndef UNDER_CE
AM_COMMANDDATATYPE cmd_data;
#endif

#define PERMS 0666
int errno;
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

    OMX_ERRORTYPE rm_error = OMX_ErrorNone;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef_ip = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef_op = NULL;
    AACENC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    OMX_AUDIO_PARAM_AACPROFILETYPE *aac_ip = NULL;
    OMX_AUDIO_PARAM_AACPROFILETYPE *aac_op = NULL;
    OMX_CONF_CHECK_CMD(hComp, 1, 1);                              /* checking for NULL pointers */
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE*) hComp;
    OMX_AUDIO_PARAM_PCMMODETYPE *aac_pcm_ip = NULL;
    OMX_AUDIO_PARAM_PCMMODETYPE *aac_pcm_op = NULL;
    int i;

    /*Set the all component function pointer to the handle*/
    pHandle->SetCallbacks           = SetCallbacks;
    pHandle->GetComponentVersion    = GetComponentVersion;
    pHandle->SendCommand            = SendCommand;
    pHandle->GetParameter           = GetParameter;
    pHandle->GetExtensionIndex      = GetExtensionIndex;
    pHandle->SetParameter           = SetParameter;
    pHandle->GetConfig              = GetConfig;
    pHandle->SetConfig              = SetConfig;
    pHandle->GetState               = GetState;
    pHandle->EmptyThisBuffer        = EmptyThisBuffer;
    pHandle->FillThisBuffer         = FillThisBuffer;
    pHandle->ComponentTunnelRequest = ComponentTunnelRequest;
    pHandle->ComponentDeInit        = ComponentDeInit;
    pHandle->AllocateBuffer         = AllocateBuffer;
    pHandle->FreeBuffer             = FreeBuffer;
    pHandle->UseBuffer              = UseBuffer;
    pHandle->ComponentRoleEnum      = ComponentRoleEnum;
    
    OMX_MALLOC_GENERIC(pHandle->pComponentPrivate, AACENC_COMPONENT_PRIVATE);
    ((AACENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate)->pHandle = pHandle;
    
    /* Initialize component data structures to default values */
    ((AACENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate)->sPortParam.nPorts = 0x2;
    ((AACENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate)->sPortParam.nStartPortNumber = 0x0;


    /* ---------start of OMX_AUDIO_PARAM_AACPROFILETYPE --------- */

    OMX_MALLOC_GENERIC(aac_ip, OMX_AUDIO_PARAM_AACPROFILETYPE);
    OMX_MALLOC_GENERIC(aac_op, OMX_AUDIO_PARAM_AACPROFILETYPE);

    ((AACENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate)->aacParams[INPUT_PORT] = aac_ip;
    ((AACENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate)->aacParams[OUTPUT_PORT] = aac_op;
        
    aac_op->nSize               = sizeof(OMX_AUDIO_PARAM_AACPROFILETYPE);
    aac_op->nChannels           = 2;
    aac_op->nSampleRate         = 44100;
    aac_op->eAACProfile         = OMX_AUDIO_AACObjectLC;
    aac_op->eAACStreamFormat    = OMX_AUDIO_AACStreamFormatMP2ADTS;  /* For khronos only : should  be MP4ADTS*/
    aac_op->nBitRate            = 128000;
    aac_op->eChannelMode        = OMX_AUDIO_ChannelModeStereo;
    aac_op->nPortIndex          = 1;
    aac_op->nFrameLength        = 0;
    aac_op->nAudioBandWidth     = 0;
    
    /* ---------end of MX_AUDIO_PARAM_AACPROFILETYPE --------- */


    /* ---------start of OMX_AUDIO_PARAM_PCMMODETYPE --------- */

    OMX_MALLOC_GENERIC(aac_pcm_ip, OMX_AUDIO_PARAM_PCMMODETYPE);
    OMX_MALLOC_GENERIC(aac_pcm_op, OMX_AUDIO_PARAM_PCMMODETYPE);

    aac_pcm_ip->nSize               = sizeof(OMX_AUDIO_PARAM_PCMMODETYPE);
    aac_pcm_ip->nBitPerSample       = 16;        /*Will be remapped for SN. 16:2,  24:3*/
    aac_pcm_ip->nPortIndex          = 0;
    aac_pcm_ip->nChannels           = 1;         /*Will be remapped for SN.  0:mono, 1:stereo*/
    aac_pcm_ip->eNumData            = OMX_NumericalDataSigned;  
    aac_pcm_ip->nSamplingRate       = 8000;
    aac_pcm_ip->ePCMMode            = OMX_AUDIO_PCMModeLinear; 
    aac_pcm_ip->bInterleaved        = OMX_TRUE; 

    ((AACENC_COMPONENT_PRIVATE*) pHandle->pComponentPrivate)->pcmParam[INPUT_PORT] = aac_pcm_ip;
    ((AACENC_COMPONENT_PRIVATE*) pHandle->pComponentPrivate)->pcmParam[OUTPUT_PORT] = aac_pcm_op;

    /* ---------end of OMX_AUDIO_PARAM_PCMMODETYPE --------- */


    pComponentPrivate = pHandle->pComponentPrivate;
    OMX_DBG_INIT(pComponentPrivate->dbg, "OMX_DBG_AACENC");

#ifdef ANDROID /* leave this now, we may need them later. */
    pComponentPrivate->iPVCapabilityFlags.iIsOMXComponentMultiThreaded = OMX_TRUE;
    pComponentPrivate->iPVCapabilityFlags.iOMXComponentNeedsNALStartCode = OMX_FALSE; 
    pComponentPrivate->iPVCapabilityFlags.iOMXComponentSupportsExternalOutputBufferAlloc = OMX_FALSE; 
    pComponentPrivate->iPVCapabilityFlags.iOMXComponentSupportsExternalInputBufferAlloc = OMX_FALSE;
    pComponentPrivate->iPVCapabilityFlags.iOMXComponentSupportsMovableInputBuffers = OMX_FALSE; 
    pComponentPrivate->iPVCapabilityFlags.iOMXComponentSupportsPartialFrames = OMX_TRUE;
    pComponentPrivate->iPVCapabilityFlags.iOMXComponentCanHandleIncompleteFrames = OMX_TRUE;
#endif
    
#ifdef __PERF_INSTRUMENTATION__
    pComponentPrivate->pPERF = PERF_Create(PERF_FOURCC('A','A','E','_'),
                                               PERF_ModuleLLMM |
                                               PERF_ModuleAudioDecode);
#endif

    OMX_MALLOC_GENERIC(pComponentPrivate->pInputBufferList, BUFFERLIST);

    OMX_PRBUFFER2(pComponentPrivate->dbg, "AACENC: pInputBufferList %p\n ", pComponentPrivate->pInputBufferList);
    pComponentPrivate->pInputBufferList->numBuffers = 0; /* initialize number of buffers */

    OMX_MALLOC_GENERIC(pComponentPrivate->pOutputBufferList, BUFFERLIST);
    pComponentPrivate->pOutputBufferList->numBuffers = 0; /* initialize number of buffers */
    OMX_PRBUFFER2(pComponentPrivate->dbg, "AACENC: pOutputBufferList %p\n ", pComponentPrivate->pOutputBufferList);

    for (i=0; i < MAX_NUM_OF_BUFS; i++) 
    {
        pComponentPrivate->pOutputBufferList->pBufHdr[i] = NULL;
        pComponentPrivate->pInputBufferList->pBufHdr[i]  = NULL;
        pComponentPrivate->tickcountBufIndex[i] = 0;
        pComponentPrivate->timestampBufIndex[i]  = 0;
    }

    pComponentPrivate->IpBufindex = 0;
    pComponentPrivate->OpBufindex = 0;

    OMX_MALLOC_SIZE(pComponentPrivate->sDeviceString, 100*sizeof(OMX_STRING), void);
    
    /* Initialize device string to the default value */
    strcpy((char*)pComponentPrivate->sDeviceString,"/rtmdn:i2:o1/codec\0");

    /*Safety value for frames per output buffer ( for Khronos)  */
    pComponentPrivate->FramesPer_OutputBuffer = 1; 
    pComponentPrivate->CustomConfiguration = OMX_FALSE;
    
    pComponentPrivate->dasfmode                     = 0;
    pComponentPrivate->unNumChannels                = 2;
    pComponentPrivate->ulSamplingRate               = 44100;
    pComponentPrivate->unBitrate                    = 128000;
    pComponentPrivate->nObjectType                  = 2;
    pComponentPrivate->bitRateMode                  = 0;
    pComponentPrivate->File_Format                  = 2;
    pComponentPrivate->EmptybufferdoneCount         = 0;
    pComponentPrivate->EmptythisbufferCount         = 0;
    pComponentPrivate->FillbufferdoneCount          = 0;
    pComponentPrivate->FillthisbufferCount          = 0;

    pComponentPrivate->bPortDefsAllocated           = 0;
    pComponentPrivate->bCompThreadStarted           = 0;
    pComponentPrivate->bPlayCompleteFlag            = 0;
  OMX_PRINT2(pComponentPrivate->dbg, "%d :: AACENC: pComponentPrivate->bPlayCompleteFlag = %ld\n",__LINE__,pComponentPrivate->bPlayCompleteFlag);
    pComponentPrivate->strmAttr                     = NULL;
    pComponentPrivate->pMarkBuf                     = NULL;
    pComponentPrivate->pMarkData                    = NULL;
    pComponentPrivate->pParams                      = NULL;
    pComponentPrivate->ptAlgDynParams               = NULL;
    pComponentPrivate->LastOutputBufferHdrQueued    = NULL;
    pComponentPrivate->bDspStoppedWhileExecuting    = OMX_FALSE;
    pComponentPrivate->nOutStandingEmptyDones       = 0;
    pComponentPrivate->nOutStandingFillDones        = 0;
    pComponentPrivate->bPauseCommandPending         = OMX_FALSE;
    pComponentPrivate->bEnableCommandPending        = 0;
    pComponentPrivate->bDisableCommandPending       = 0;
    pComponentPrivate->nNumOutputBufPending         = 0;
    pComponentPrivate->bLoadedCommandPending        = OMX_FALSE;
    pComponentPrivate->bFirstOutputBuffer = 1;

    pComponentPrivate->nUnhandledFillThisBuffers=0;
    pComponentPrivate->nUnhandledEmptyThisBuffers = 0;
    
    pComponentPrivate->bFlushOutputPortCommandPending = OMX_FALSE;
    pComponentPrivate->bFlushInputPortCommandPending = OMX_FALSE;

    pComponentPrivate->nNumInputBufPending          = 0;
    pComponentPrivate->nNumOutputBufPending         = 0;

    pComponentPrivate->PendingInPausedBufs          = 0;
    pComponentPrivate->PendingOutPausedBufs         = 0;

    /* Port format type */
    pComponentPrivate->sOutPortFormat.eEncoding     = OMX_AUDIO_CodingAAC;
    pComponentPrivate->sOutPortFormat.nIndex        = 0;/*OMX_IndexParamAudioAac;*/
    pComponentPrivate->sOutPortFormat.nPortIndex    = OUTPUT_PORT;

    pComponentPrivate->sInPortFormat.eEncoding      = OMX_AUDIO_CodingPCM;
    pComponentPrivate->sInPortFormat.nIndex         = 1;/*OMX_IndexParamAudioPcm;  */
    pComponentPrivate->sInPortFormat.nPortIndex     = INPUT_PORT;

    /*flags that control LCML closing*/
    pComponentPrivate->ptrLibLCML                   = NULL;
    pComponentPrivate->bGotLCML                     = OMX_FALSE;
    pComponentPrivate->bCodecDestroyed              = OMX_FALSE;
    

    /* initialize role name */
    strcpy((char *)pComponentPrivate->componentRole.cRole, "audio_encoder.aac");
    
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

    pthread_mutex_init(&bufferReturned_mutex, NULL);
    pthread_cond_init (&bufferReturned_condition, NULL);
#else
    OMX_CreateEvent(&(pComponentPrivate->AlloBuf_event));
    pComponentPrivate->AlloBuf_waitingsignal = 0;
    
    OMX_CreateEvent(&(pComponentPrivate->InLoaded_event));
    pComponentPrivate->InLoaded_readytoidle = 0;
    
    OMX_CreateEvent(&(pComponentPrivate->InIdle_event));
    pComponentPrivate->InIdle_goingtoloaded = 0;
#endif
        
    /* port definition, input port */
    OMX_MALLOC_GENERIC(pPortDef_ip, OMX_PARAM_PORTDEFINITIONTYPE);
    OMX_PRCOMM2(pComponentPrivate->dbg, "AACENC: pPortDef_ip %p \n",pPortDef_ip );


    /* port definition, output port */
    OMX_MALLOC_GENERIC(pPortDef_op, OMX_PARAM_PORTDEFINITIONTYPE);
    OMX_PRCOMM2(pComponentPrivate->dbg, "AACENC: pPortDef_op %p,  size: %x \n",pPortDef_op, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));

    
    ((AACENC_COMPONENT_PRIVATE*) pHandle->pComponentPrivate)->pPortDef[INPUT_PORT] = pPortDef_ip;
    ((AACENC_COMPONENT_PRIVATE*) pHandle->pComponentPrivate)->pPortDef[OUTPUT_PORT] = pPortDef_op;

    
    pPortDef_ip->nSize                  = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
    pPortDef_ip->nPortIndex             = 0x0;
    pPortDef_ip->nBufferCountActual     = NUM_AACENC_INPUT_BUFFERS;
    pPortDef_ip->nBufferCountMin        = NUM_AACENC_INPUT_BUFFERS;
    pPortDef_ip->eDir                   = OMX_DirInput;
    pPortDef_ip->bEnabled               = OMX_TRUE;
    pPortDef_ip->nBufferSize            = INPUT_AACENC_BUFFER_SIZE;
    pPortDef_ip->nBufferAlignment       = DSP_CACHE_ALIGNMENT;
    pPortDef_ip->bPopulated             = 0;
    pPortDef_ip->format.audio.eEncoding =OMX_AUDIO_CodingPCM;  
    pPortDef_ip->eDomain                = OMX_PortDomainAudio;
    
    pPortDef_op->nSize                  = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
    pPortDef_op->nPortIndex             = 0x1;
    pPortDef_op->nBufferCountActual     = NUM_AACENC_OUTPUT_BUFFERS;
    pPortDef_op->nBufferCountMin        = NUM_AACENC_OUTPUT_BUFFERS;
    pPortDef_op->eDir                   = OMX_DirOutput;
    pPortDef_op->bEnabled               = OMX_TRUE;
    pPortDef_op->nBufferSize            = OUTPUT_AACENC_BUFFER_SIZE;
    pPortDef_op->nBufferAlignment       = DSP_CACHE_ALIGNMENT;
    pPortDef_op->bPopulated             = 0;
    pPortDef_op->format.audio.eEncoding = OMX_AUDIO_CodingAAC;   
    pPortDef_op->eDomain                = OMX_PortDomainAudio;

    pComponentPrivate->bIsInvalidState = OMX_FALSE;

    pComponentPrivate->bPreempted = OMX_FALSE; 

#ifdef RESOURCE_MANAGER_ENABLED
    /* start Resource Manager Proxy */
    eError = RMProxy_NewInitalize();
    if (eError != OMX_ErrorNone) 
    {
        OMX_ERROR4(pComponentPrivate->dbg, "%d :: Error returned from loading ResourceManagerProxy thread\n",__LINE__);
        goto EXIT;
    }
    pComponentPrivate->rmproxyCallback.RMPROXY_Callback = (void *) AACENC_ResourceManagerCallback;
    rm_error = RMProxy_NewSendCommand(pHandle, RMProxy_RequestResource, OMX_AAC_Encoder_COMPONENT,AACENC_CPU_USAGE, 3456, &(pComponentPrivate->rmproxyCallback));
    if (rm_error != OMX_ErrorNone) {
        RMProxy_NewSendCommand(pHandle, RMProxy_FreeResource, OMX_AAC_Encoder_COMPONENT, 0, 3456, NULL);
        RMProxy_Deinitalize();
        eError = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
#endif
#ifndef UNDER_CE
#ifdef DSP_RENDERING_ON

    /* start Audio Manager to get streamId */
    if((Aacenc_fdwrite=open(FIFO1,O_WRONLY))<0) 
    {
        OMX_ERROR4(pComponentPrivate->dbg, "%d :: [AAC Encoder Component] - failure to open WRITE pipe\n",__LINE__);
    }

    if((Aacenc_fdread=open(FIFO2,O_RDONLY))<0) 
    {
        OMX_ERROR4(pComponentPrivate->dbg, "%d :: [AAC Encoder Component] - failure to open READ pipe\n",__LINE__);
    }
#endif
#endif

    eError = AACENC_StartComponentThread(pHandle);
    if (eError != OMX_ErrorNone) 
    {
      OMX_ERROR4(pComponentPrivate->dbg, "%d :: Error returned from the Component\n",__LINE__);
      goto EXIT;
    }


#ifdef __PERF_INSTRUMENTATION__
    PERF_ThreadCreated(pComponentPrivate->pPERF, pComponentPrivate->ComponentThread,
                       PERF_FOURCC('A','A','E','T'));
#endif

    if(pthread_mutex_init(&pComponentPrivate->mutexStateChangeRequest, NULL)) {
       return OMX_ErrorUndefined;
    }

    if(pthread_cond_init (&pComponentPrivate->StateChangeCondition, NULL)) {
       return OMX_ErrorUndefined;
    }

    pComponentPrivate->nPendingStateChangeRequests = 0;


EXIT:
    if (pComponentPrivate != NULL) {
        OMX_PRINT2(pComponentPrivate->dbg, "%d :: AACENC: Exiting OMX_ComponentInit\n", __LINE__);
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
    AACENC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    OMX_CONF_CHECK_CMD(pComponent,1,1);                 /* Checking for NULL pointers:  pAppData is NULL for Khronos */
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE*)pComponent;
    
    pComponentPrivate = (AACENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    OMX_PRINT1(pComponentPrivate->dbg, "%d :: AACENC: Entering SetCallbacks\n", __LINE__);
    if (pCallBacks == NULL) 
    {
        OMX_ERROR4(pComponentPrivate->dbg, "%d :: Error: About to return OMX_ErrorBadParameter\n",__LINE__);
        eError = OMX_ErrorBadParameter;
        OMX_ERROR2(pComponentPrivate->dbg, "%d :: Received the empty callbacks from the application\n",__LINE__);
        goto EXIT;
    }

    /*Copy the callbacks of the application to the component private*/
    memcpy (&(pComponentPrivate->cbInfo), pCallBacks, sizeof(OMX_CALLBACKTYPE));
    /*copy the application private data to component memory */
    pHandle->pApplicationPrivate = pAppData;
    pComponentPrivate->curState = OMX_StateLoaded;

EXIT:
    if (pComponentPrivate != NULL) {
	OMX_PRINT1(pComponentPrivate->dbg, "%d :: AACENC: Exiting SetCallbacks\n", __LINE__);
    }
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
    AACENC_COMPONENT_PRIVATE *pComponentPrivate = (AACENC_COMPONENT_PRIVATE *) pHandle->pComponentPrivate;

    OMX_PRINT1(pComponentPrivate->dbg, "%d :: AACENC: Entering GetComponentVersion\n", __LINE__);

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
        memcpy(pComponentVersion, &(pComponentPrivate->ComponentVersion.s), sizeof(pComponentPrivate->ComponentVersion.s));
    }
    else 
    {
        eError = OMX_ErrorBadParameter;
    }

EXIT:

    OMX_PRINT1(pComponentPrivate->dbg, "%d :: AACENC: Exiting GetComponentVersion\n", __LINE__);
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
    AACENC_COMPONENT_PRIVATE *pCompPrivate = NULL;
    OMX_CONF_CHECK_CMD(phandle,1,1);        /*NOTE: Cmd,  pCmdData, nParam  are  NULL  for khronos*/
    int nRet = 0;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)phandle;

    pCompPrivate = (AACENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;

#ifdef _ERROR_PROPAGATION__
    if (pCompPrivate->curState == OMX_StateInvalid)
    {
        eError = OMX_ErrorInvalidState;
        goto EXIT;
    }   
#else
    OMX_PRINT1(pCompPrivate->dbg, "%d :: AACENC: Entering SendCommand()\n", __LINE__);
    if(pCompPrivate->curState == OMX_StateInvalid) 
    {
           OMX_PRINT1(pCompPrivate->dbg, "%d :: AACENC: Inside SendCommand\n",__LINE__);
           eError = OMX_ErrorInvalidState;
           OMX_ERROR4(pCompPrivate->dbg, "%d :: Error Notofication Sent to App\n",__LINE__);
           pCompPrivate->cbInfo.EventHandler(pHandle, pHandle->pApplicationPrivate,
                                             OMX_EventError, 
                                             OMX_ErrorInvalidState,
                                             OMX_TI_ErrorMinor,
                                             "Invalid State");
           goto EXIT;
    }
#endif

#ifdef __PERF_INSTRUMENTATION__
    PERF_SendingCommand(pCompPrivate->pPERF,Cmd,
                        (Cmd == OMX_CommandMarkBuffer) ? ((OMX_U32) pCmdData) : nParam,
                        PERF_ModuleComponent);
#endif

    switch(Cmd) 
    {
        case OMX_CommandStateSet:
            if (nParam == OMX_StateLoaded) 
            {
                pCompPrivate->bLoadedCommandPending = OMX_TRUE;
            }
       
            OMX_PRINT1(pCompPrivate->dbg, "%d :: AACENC: Inside SendCommand\n",__LINE__);
            OMX_PRSTATE1(pCompPrivate->dbg, "%d :: AACENC: pCompPrivate->curState = %d\n",__LINE__,pCompPrivate->curState);
            if(pCompPrivate->curState == OMX_StateLoaded)
            {
                if((nParam == OMX_StateExecuting) || (nParam == OMX_StatePause))
                {
                    pCompPrivate->cbInfo.EventHandler(pHandle,
                                                      pHandle->pApplicationPrivate,
                                                      OMX_EventError,
                                                      OMX_ErrorIncorrectStateTransition,
                                                      OMX_TI_ErrorMinor,
                                                      NULL);
                    goto EXIT;
                }

                if(nParam == OMX_StateInvalid)
                {
                    OMX_PRINT1(pCompPrivate->dbg, "%d :: AACENC: Inside SendCommand\n",__LINE__);
                    OMX_PRSTATE2(pCompPrivate->dbg, "AACENC: State changed to OMX_StateInvalid Line %d\n",__LINE__);
                    pCompPrivate->curState = OMX_StateInvalid;
                    pCompPrivate->cbInfo.EventHandler(pHandle,
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
                OMX_PRINT1(pCompPrivate->dbg, "%d :: IAACENC: nside SendCommand\n",__LINE__);
                if(nParam > 1 && nParam != -1) 
                {
                    eError = OMX_ErrorBadPortIndex;
                    goto EXIT;
                }
                break;
            
        case OMX_CommandPortDisable:
                OMX_PRDSP2(pCompPrivate->dbg, "%d :: AACENC: Inside SendCommand OMX_CommandPortDisable\n",__LINE__);
                break;
        
        case OMX_CommandPortEnable:
                OMX_PRDSP2(pCompPrivate->dbg, "%d :: AACENC: Inside SendCommand OMX_CommandPortEnable\n",__LINE__);
                break;
        
        case OMX_CommandMarkBuffer:
                OMX_PRDSP2(pCompPrivate->dbg, "%d :: AACENC: Inside SendCommand OMX_CommandMarkBuffer\n",__LINE__);
                if (nParam > 0) 
                {
                    eError = OMX_ErrorBadPortIndex;
                    goto EXIT;
                }
                break;
        
        default:
                OMX_ERROR4(pCompPrivate->dbg, "%d :: Error: Command Received Default error\n",__LINE__);
                pCompPrivate->cbInfo.EventHandler(pHandle, pHandle->pApplicationPrivate,
                                                  OMX_EventError,
                                                  OMX_ErrorBadParameter,
                                                  OMX_TI_ErrorMinor,
                                                  "Invalid Command");
                break;

    }

    OMX_PRINT1(pCompPrivate->dbg, "%d :: AACENC: Inside SendCommand\n",__LINE__);
    nRet = write (pCompPrivate->cmdPipe[1], &Cmd, sizeof(Cmd));
    OMX_PRCOMM2(pCompPrivate->dbg, "%d :: AACENC: Cmd pipe has been writen. nRet = %d  \n",__LINE__,nRet);
    OMX_PRCOMM2(pCompPrivate->dbg, "%d :: AACENC: pCompPrivate->cmdPipe[1] = %d  \n",__LINE__,pCompPrivate->cmdPipe[1]);
    OMX_PRCOMM2(pCompPrivate->dbg, "%d :: AACENC: &Cmd = %p  \n",__LINE__,&Cmd);

    if (nRet == -1) 
    {
        OMX_PRINT1(pCompPrivate->dbg, "%d :: AACENC: Inside SendCommand\n",__LINE__);
        eError = OMX_ErrorInsufficientResources;
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
    

    if (nRet == -1) {
        OMX_ERROR4(pCompPrivate->dbg, "%d :: OMX_ErrorInsufficientResources from SendCommand",__LINE__);
        if(Cmd == OMX_CommandStateSet) {
           if(RemoveStateTransition(pCompPrivate, OMX_FALSE) != OMX_ErrorNone) {
              return OMX_ErrorUndefined;
           }
        }
        return OMX_ErrorInsufficientResources;
    }
    
EXIT:
    if (pCompPrivate != NULL) {
	OMX_PRINT1(pCompPrivate->dbg, "%d :: AACENC: Exiting SendCommand()\n", __LINE__);
    }
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

static OMX_ERRORTYPE GetParameter (OMX_HANDLETYPE hComp, OMX_INDEXTYPE nParamIndex, OMX_PTR ComponentParameterStructure)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    AACENC_COMPONENT_PRIVATE  *pComponentPrivate = NULL;
    OMX_CONF_CHECK_CMD(hComp, 1, 1);
    OMX_PARAM_PORTDEFINITIONTYPE *pParameterStructure = NULL;

    pComponentPrivate = (AACENC_COMPONENT_PRIVATE *)(((OMX_COMPONENTTYPE*)hComp)->pComponentPrivate);
    OMX_PRINT1 (pComponentPrivate->dbg, "%d :: AACENC: Entering GetParameter\n", __LINE__);
    
    pParameterStructure = (OMX_PARAM_PORTDEFINITIONTYPE*)ComponentParameterStructure;
    if (pParameterStructure == NULL) 
    {
        OMX_ERROR4(pComponentPrivate->dbg, "%d :: Error: OMX_ErrorBadParameter Inside the GetParameter Line\n",__LINE__);
        eError = OMX_ErrorBadParameter;
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
        pComponentPrivate->cbInfo.EventHandler(hComp, 
                                                ((OMX_COMPONENTTYPE *)hComp)->pApplicationPrivate,
                                                OMX_EventError, 
                                                OMX_ErrorIncorrectStateOperation, 
                                                OMX_TI_ErrorMinor,
                                                "Invalid State");
        OMX_ERROR4(pComponentPrivate->dbg, "%d :: AACENC: Inside the GetParameter Line\n",__LINE__);
    }
#endif  
    switch(nParamIndex)
    {

        case OMX_IndexParamAudioInit:
             memcpy(ComponentParameterStructure, &pComponentPrivate->sPortParam, sizeof(OMX_PORT_PARAM_TYPE));
             break;
        
        case OMX_IndexParamPortDefinition:
            if(((OMX_PARAM_PORTDEFINITIONTYPE *)(ComponentParameterStructure))->nPortIndex == pComponentPrivate->pPortDef[INPUT_PORT]->nPortIndex) 
            {
                OMX_PRDSP2(pComponentPrivate->dbg, "%d :: AACENC: Inside the GetParameter Line\n",__LINE__);
                memcpy(ComponentParameterStructure,pComponentPrivate->pPortDef[INPUT_PORT], sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
            } 
            else if(((OMX_PARAM_PORTDEFINITIONTYPE *)(ComponentParameterStructure))->nPortIndex == pComponentPrivate->pPortDef[OUTPUT_PORT]->nPortIndex) 
            {
                memcpy(ComponentParameterStructure, pComponentPrivate->pPortDef[OUTPUT_PORT], sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
                OMX_PRDSP1(pComponentPrivate->dbg, "%d :: AACENC: Inside the GetParameter \n",__LINE__);
            } 
            else 
            {
                OMX_ERROR4(pComponentPrivate->dbg, "%d :: Error: BadPortIndex Inside the GetParameter \n",__LINE__);
                eError = OMX_ErrorBadPortIndex;
            }
            break;

        case OMX_IndexParamAudioPortFormat:
            if(((OMX_AUDIO_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nPortIndex == pComponentPrivate->pPortDef[INPUT_PORT]->nPortIndex) 
            {
                if(((OMX_AUDIO_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nIndex > pComponentPrivate->sInPortFormat.nIndex) 
                {
                    OMX_ERROR4(pComponentPrivate->dbg, "%d :: Error: ErrorNoMore Inside the GetParameter Line\n",__LINE__);
                    eError = OMX_ErrorNoMore;
                } 
                else 
                {
                    OMX_PRDSP2(pComponentPrivate->dbg, "%d :: AACENC: About to copy Inside GetParameter \n",__LINE__);
                    memcpy(ComponentParameterStructure, &pComponentPrivate->sInPortFormat, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
                }
            }
            else if(((OMX_AUDIO_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nPortIndex == pComponentPrivate->pPortDef[OUTPUT_PORT]->nPortIndex) 
            {
                    if(((OMX_AUDIO_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nIndex > pComponentPrivate->sOutPortFormat.nIndex) 
                    {
                        OMX_ERROR4(pComponentPrivate->dbg, "%d :: Error: ErrorNoMore Inside the GetParameter Line\n",__LINE__);
                        eError = OMX_ErrorNoMore;
                    } 
                    else 
                    {
                        OMX_PRDSP2(pComponentPrivate->dbg, "%d :: AACENC: About to copy Inside GetParameter \n",__LINE__);
                        memcpy(ComponentParameterStructure, &pComponentPrivate->sOutPortFormat, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
                    }
            } 
            else 
            {
                OMX_ERROR4(pComponentPrivate->dbg, "%d :: Error: BadPortIndex Inside the GetParameter Line\n",__LINE__);
                eError = OMX_ErrorBadPortIndex;
            }
            break;

        case OMX_IndexParamAudioAac:
            OMX_PRDSP2(pComponentPrivate->dbg, "AACENC: OMX_IndexParamAudioAac : nPortIndex = %d\n", (int)((OMX_AUDIO_PARAM_AACPROFILETYPE *)(ComponentParameterStructure))->nPortIndex);
            OMX_PRCOMM2(pComponentPrivate->dbg, "AACENC: acParams[INPUT_PORT]->nPortIndex = %d\n", (int)pComponentPrivate->aacParams[INPUT_PORT]->nPortIndex);
            OMX_PRCOMM2(pComponentPrivate->dbg, "AACENC: acParams[OUPUT_PORT]->nPortIndex = %d\n", (int)pComponentPrivate->aacParams[OUTPUT_PORT]->nPortIndex);
            
            if(((OMX_AUDIO_PARAM_AACPROFILETYPE *)(ComponentParameterStructure))->nPortIndex == pComponentPrivate->aacParams[INPUT_PORT]->nPortIndex) 
            {

                if (pComponentPrivate->CustomConfiguration ) /* For Testapp: An  Index  was providded. Getting the required Structure */
                                                             /* The flag is set in Setconfig() function*/
                {
                    OMX_PRCOMM2(pComponentPrivate->dbg, "AACENC: OMX_IndexParamAudioAac :input port \n");
                    memcpy(ComponentParameterStructure,pComponentPrivate->aacParams[INPUT_PORT], sizeof(OMX_AUDIO_PARAM_AACPROFILETYPE));
                    
                }
                else            /*for Khronos:  Getting the default structure (Ouput) for an  index not providded*/
                {
                    OMX_PRCOMM2(pComponentPrivate->dbg, "AACENC: OMX_IndexParamAudioAac :output port \n");
                    memcpy(ComponentParameterStructure, pComponentPrivate->aacParams[OUTPUT_PORT], sizeof(OMX_AUDIO_PARAM_AACPROFILETYPE));
                }
            } 
            else if(((OMX_AUDIO_PARAM_AACPROFILETYPE *)(ComponentParameterStructure))->nPortIndex == pComponentPrivate->aacParams[OUTPUT_PORT]->nPortIndex) 
            {
                OMX_PRCOMM2(pComponentPrivate->dbg, "AACENC: OMX_IndexParamAudioAac :output port \n");
                memcpy(ComponentParameterStructure, pComponentPrivate->aacParams[OUTPUT_PORT], sizeof(OMX_AUDIO_PARAM_AACPROFILETYPE));

            } 
            else 
            {
                OMX_ERROR4(pComponentPrivate->dbg, "%d :: Error: BadPortIndex Inside the GetParameter Line\n",__LINE__);
                eError = OMX_ErrorBadPortIndex;
            }
            break;


        case OMX_IndexParamAudioPcm:
            if(((OMX_AUDIO_PARAM_AACPROFILETYPE *)(ComponentParameterStructure))->nPortIndex == pComponentPrivate->pcmParam[INPUT_PORT]->nPortIndex) 
            {
                memcpy(ComponentParameterStructure,pComponentPrivate->pcmParam[INPUT_PORT], sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
                OMX_PRCOMM2(pComponentPrivate->dbg, "AACENC: OMX_IndexParamAudioPcm :input port \n");
            } 
            else if(((OMX_AUDIO_PARAM_AACPROFILETYPE *)(ComponentParameterStructure))->nPortIndex == pComponentPrivate->pcmParam[OUTPUT_PORT]->nPortIndex) 
            {
                memcpy(ComponentParameterStructure, pComponentPrivate->pcmParam[OUTPUT_PORT], sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
                OMX_PRCOMM2(pComponentPrivate->dbg, "AACENC: OMX_IndexParamAudioPcm :output port \n");
            } 
            else 
            {
                eError = OMX_ErrorBadPortIndex;
            }
            break;

        case OMX_IndexParamCompBufferSupplier:
         if(((OMX_PARAM_BUFFERSUPPLIERTYPE *)(ComponentParameterStructure))->nPortIndex == OMX_DirInput) 
         {
             OMX_PRBUFFER2(pComponentPrivate->dbg, "AACENC: GetParameter OMX_IndexParamCompBufferSupplier \n");
             
         }
         else if(((OMX_PARAM_BUFFERSUPPLIERTYPE *)(ComponentParameterStructure))->nPortIndex == OMX_DirOutput) 
         {
             OMX_PRBUFFER2(pComponentPrivate->dbg, "AACENC: GetParameter OMX_IndexParamCompBufferSupplier \n");
         } 
         else 
         {
             OMX_ERROR4(pComponentPrivate->dbg, "%d :: Error: OMX_ErrorBadPortIndex from GetParameter",__LINE__);
             eError = OMX_ErrorBadPortIndex;
         } 
         
            break;

#ifdef ANDROID
    case (OMX_INDEXTYPE) PV_OMX_COMPONENT_CAPABILITY_TYPE_INDEX:
    {
	OMX_PRDSP2(pComponentPrivate->dbg, "Entering PV_OMX_COMPONENT_CAPABILITY_TYPE_INDEX::%d\n", __LINE__);
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
            

        case OMX_IndexParamPriorityMgmt:
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
    if (pComponentPrivate != NULL) {
	OMX_PRINT1(pComponentPrivate->dbg, "%d :: AACENC: Exiting GetParameter:: %x  error :: %x \n", __LINE__, nParamIndex, eError);
    }
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
    AACENC_COMPONENT_PRIVATE  *pComponentPrivate = NULL;
    OMX_CONF_CHECK_CMD(hComp, 1, 1);
    pComponentPrivate = (AACENC_COMPONENT_PRIVATE *)(((OMX_COMPONENTTYPE*)hComp)->pComponentPrivate);
    OMX_PARAM_COMPONENTROLETYPE  *pRole = NULL;
    OMX_PARAM_BUFFERSUPPLIERTYPE sBufferSupplier;

    OMX_PRINT1(pComponentPrivate->dbg, "%d :: AACENC: Entering the SetParameter()\n",__LINE__);
    OMX_PRINT2(pComponentPrivate->dbg, "%d :: AACENC: Inside the SetParameter nParamIndex = %x\n",__LINE__, nParamIndex);

    if (pCompParam == NULL) 
    {
        eError = OMX_ErrorBadParameter;
        OMX_ERROR4(pComponentPrivate->dbg, "%d :: Error: About to return OMX_ErrorBadParameter \n",__LINE__);
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
            {
                OMX_AUDIO_PARAM_PORTFORMATTYPE *pComponentParam = (OMX_AUDIO_PARAM_PORTFORMATTYPE *)pCompParam;
                OMX_PRINT2(pComponentPrivate->dbg, "%d :: AACENC: pCompParam = index %d\n",__LINE__,(int)pComponentParam->nIndex);
                OMX_PRCOMM2(pComponentPrivate->dbg, "%d :: AACENC: pCompParam = nportindex  %d\n",__LINE__,(int)pComponentParam->nPortIndex);
                /* for input port */
                if (pComponentParam->nPortIndex == 0) 
                {
                    OMX_PRINT2(pComponentPrivate->dbg, "%d :: AACENC: OMX_IndexParamAudioPortFormat - index 0 \n",__LINE__);
                   
                } 
                else if (pComponentParam->nPortIndex == 1) 
                {
                    OMX_PRINT2(pComponentPrivate->dbg, "%d :: AACENC: OMX_IndexParamAudioPortFormat - index 1 \n",__LINE__);                    
                }
               else 
               {
                    OMX_ERROR4(pComponentPrivate->dbg, "%d :: Error: Wrong Port Index Parameter\n", __LINE__);
                    OMX_ERROR4(pComponentPrivate->dbg, "%d :: Error: About to return OMX_ErrorBadParameter\n",__LINE__);
                    eError = OMX_ErrorBadParameter;
                    goto EXIT;
               }
               break;
            }
        
        case OMX_IndexParamAudioAac:
            OMX_PRDSP2(pComponentPrivate->dbg, "%d :: AACENC: Inside the SetParameter - OMX_IndexParamAudioAac\n", __LINE__);
            if(((OMX_AUDIO_PARAM_AACPROFILETYPE *)(pCompParam))->nPortIndex ==
                                     pComponentPrivate->pPortDef[OUTPUT_PORT]->nPortIndex) 
            {
                OMX_CONF_CHECK_CMD(pComponentPrivate->aacParams[OUTPUT_PORT], 1, 1);
                memcpy(pComponentPrivate->aacParams[OUTPUT_PORT], pCompParam,
                                               sizeof(OMX_AUDIO_PARAM_AACPROFILETYPE));
                OMX_PRDSP2(pComponentPrivate->dbg, "AACENC: nSampleRate %ld\n",pComponentPrivate->aacParams[OUTPUT_PORT]->nSampleRate);

                /* check support stereo record support at DASF */ 
                if(pComponentPrivate->aacParams[OUTPUT_PORT]->nChannels ==2)
                {
#ifndef UNDER_CE
#ifdef DSP_RENDERING_ON

                    /* inform Audio Manager support DASF stereo record */
                    cmd_data.hComponent = hComp;
                    cmd_data.AM_Cmd = AM_CommandMixerStereoRecordSupport;
                    cmd_data.param1 = OMX_TRUE;
                    if((write(Aacenc_fdwrite, &cmd_data, sizeof(cmd_data)))<0) 
                    {
                        OMX_PRCOMM2(pComponentPrivate->dbg, "[AAC Enc Component] - send command to audio manager\n");
                    }
#endif
#endif
                }
            } 
            else if(((OMX_AUDIO_PARAM_AACPROFILETYPE *)(pCompParam))->nPortIndex ==
                                     pComponentPrivate->pPortDef[INPUT_PORT]->nPortIndex) 
            {
                OMX_CONF_CHECK_CMD(pComponentPrivate->aacParams[INPUT_PORT], 1, 1);
                memcpy(pComponentPrivate->aacParams[INPUT_PORT], pCompParam,
                                               sizeof(OMX_AUDIO_PARAM_AACPROFILETYPE));
            }
            else 
            {
                eError = OMX_ErrorBadPortIndex;
                OMX_ERROR4(pComponentPrivate->dbg, "%d :: Error in SetParameter - OMX_IndexParamAudioAac = %x\n", __LINE__, eError);
            }
        break;

        case OMX_IndexParamAudioPcm:
            if(((OMX_AUDIO_PARAM_PCMMODETYPE *)(pCompParam))->nPortIndex ==
                    pComponentPrivate->pcmParam[INPUT_PORT]->nPortIndex) 
            {
                memcpy(pComponentPrivate->pcmParam[INPUT_PORT],pCompParam,
                       sizeof(OMX_AUDIO_PARAM_PCMMODETYPE)
                      );
            } 
            else if(((OMX_AUDIO_PARAM_PCMMODETYPE *)(pCompParam))->nPortIndex ==
                    pComponentPrivate->pcmParam[OUTPUT_PORT]->nPortIndex) 
            {
                memcpy(pComponentPrivate->pcmParam[OUTPUT_PORT],pCompParam,
                       sizeof(OMX_AUDIO_PARAM_PCMMODETYPE)
                      );

            } 
            else 
            {
                eError = OMX_ErrorBadPortIndex;
            }
        break;
        
        case OMX_IndexParamPortDefinition:
            if(((OMX_PARAM_PORTDEFINITIONTYPE *)(pCompParam))->nPortIndex == pComponentPrivate->pPortDef[INPUT_PORT]->nPortIndex) 
            {
                memcpy(pComponentPrivate->pPortDef[INPUT_PORT], pCompParam, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
            }
            else if(((OMX_PARAM_PORTDEFINITIONTYPE *)(pCompParam))->nPortIndex == pComponentPrivate->pPortDef[OUTPUT_PORT]->nPortIndex) 
            {
                memcpy(pComponentPrivate->pPortDef[OUTPUT_PORT], pCompParam, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));

            }
            else 
            {
                OMX_ERROR4(pComponentPrivate->dbg, "%d :: Error: Wrong Port Index Parameter\n", __LINE__);
                OMX_ERROR4(pComponentPrivate->dbg, "%d :: Error: About to return OMX_ErrorBadParameter\n",__LINE__);
                eError = OMX_ErrorBadPortIndex;
                goto EXIT;
            }
            break;

        case OMX_IndexParamPriorityMgmt:
            if (pComponentPrivate->curState != OMX_StateLoaded) 
            {
                eError = OMX_ErrorIncorrectStateOperation;
                OMX_ERROR4(pComponentPrivate->dbg, "%d :: Error: About to return OMX_ErrorIncorrectStateOperation\n",__LINE__);
            }
            break;

        case OMX_IndexParamStandardComponentRole:
            if (pCompParam) 
            {
                pRole = (OMX_PARAM_COMPONENTROLETYPE *)pCompParam;
                memcpy(&(pComponentPrivate->componentRole), (void *)pRole, sizeof(OMX_PARAM_COMPONENTROLETYPE));
            } 
            else 
            {
                eError = OMX_ErrorBadParameter;
            }
            break;

        
         case OMX_IndexParamCompBufferSupplier:
            if(((OMX_PARAM_PORTDEFINITIONTYPE *)(pCompParam))->nPortIndex ==
                                    pComponentPrivate->pPortDef[INPUT_PORT]->nPortIndex) 
            {
                    OMX_PRBUFFER1(pComponentPrivate->dbg, "AACENC: SetParameter OMX_IndexParamCompBufferSupplier \n");
                                   sBufferSupplier.eBufferSupplier = OMX_BufferSupplyInput;
                                   memcpy(&sBufferSupplier, pCompParam, sizeof(OMX_PARAM_BUFFERSUPPLIERTYPE));                                 
                    
            }
            else if(((OMX_PARAM_PORTDEFINITIONTYPE *)(pCompParam))->nPortIndex ==
                              pComponentPrivate->pPortDef[OUTPUT_PORT]->nPortIndex) 
            {
                OMX_PRBUFFER1(pComponentPrivate->dbg, "AACENC: SetParameter OMX_IndexParamCompBufferSupplier \n");
                sBufferSupplier.eBufferSupplier = OMX_BufferSupplyOutput;
                memcpy(&sBufferSupplier, pCompParam, sizeof(OMX_PARAM_BUFFERSUPPLIERTYPE));
            } 
            else 
            {
                OMX_ERROR4(pComponentPrivate->dbg, "%d:: Error: OMX_ErrorBadPortIndex from SetParameter",__LINE__);
                eError = OMX_ErrorBadPortIndex;
            }
            break;


        
            
        default:
            break;
    }
EXIT:
    if (pComponentPrivate != NULL) {
	OMX_PRINT1(pComponentPrivate->dbg, "%d :: AACENC: Exiting SetParameter:: %x\n", __LINE__, nParamIndex);
	OMX_PRINT1(pComponentPrivate->dbg, "%d :: AACENC: Exiting the SetParameter() returned eError = %d\n", __LINE__, eError);
    }
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

static OMX_ERRORTYPE GetConfig (OMX_HANDLETYPE hComp, OMX_INDEXTYPE nConfigIndex, OMX_PTR ComponentConfigStructure)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    
    AACENC_COMPONENT_PRIVATE *pComponentPrivate =  NULL;
    TI_OMX_STREAM_INFO *streamInfo = NULL;

    OMX_MALLOC_SIZE(streamInfo, sizeof(TI_OMX_STREAM_INFO), void);
    OMXDBG_PRINT(stderr, PRINT, 1, 0, "AACENC: streamInfo %p \n",streamInfo);
    
    pComponentPrivate = (AACENC_COMPONENT_PRIVATE *)
            (((OMX_COMPONENTTYPE*)hComp)->pComponentPrivate);

#ifdef _ERROR_PROPAGATION__
    if (pComponentPrivate->curState == OMX_StateInvalid)
    {
        eError = OMX_ErrorInvalidState;
        goto EXIT;
    }   
#endif

    if(nConfigIndex == OMX_IndexCustomAacEncStreamIDConfig)
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
  *  SetConfig() Sets the configuration of the component
  *
  * @param hComp         handle for this instance of the component
  * @param nConfigIndex
  * @param ComponentConfigStructure
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  **/
/*-------------------------------------------------------------------*/

static OMX_ERRORTYPE SetConfig (OMX_HANDLETYPE hComp, OMX_INDEXTYPE nConfigIndex, OMX_PTR ComponentConfigStructure)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
 /*   OMX_CONF_CHECK_CMD(hComp,1,ComponentConfigStructure);*/
    OMX_COMPONENTTYPE* pHandle = (OMX_COMPONENTTYPE*)hComp;
    AACENC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    TI_OMX_DSP_DEFINITION* pDspDefinition = NULL;
    OMX_U16 FramesPerOutBuf =0  ;
    OMX_U16* ptrFramesPerOutBuf =NULL ;
    TI_OMX_DATAPATH dataPath;
    OMX_S16 *customFlag = NULL;
    
    
#ifdef DSP_RENDERING_ON
    OMX_AUDIO_CONFIG_VOLUMETYPE *pGainStructure = NULL;
#endif
    OMXDBG_PRINT(stderr, PRINT, 1, 0, "%d :: AACENC: Entering SetConfig\n", __LINE__);
    if (pHandle == NULL) 
    {
        OMXDBG_PRINT(stderr, ERROR, 4, 0, "%d :: AACENC: Invalid HANDLE OMX_ErrorBadParameter \n",__LINE__);
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pComponentPrivate = (AACENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;

#ifdef _ERROR_PROPAGATION__
    if (pComponentPrivate->curState == OMX_StateInvalid)
    {
        eError = OMX_ErrorInvalidState;
        goto EXIT;
    }   
#endif

    switch (nConfigIndex) 
    {
        case OMX_IndexCustomAacEncHeaderInfoConfig:
        {
            pDspDefinition = (TI_OMX_DSP_DEFINITION *)ComponentConfigStructure;
            if (pDspDefinition == NULL) 
            {
                eError = OMX_ErrorBadParameter;
                OMX_ERROR4(pComponentPrivate->dbg, "%d :: Error: OMX_ErrorBadParameter from SetConfig\n",__LINE__);
                goto EXIT;
            }            
            pComponentPrivate->dasfmode = pDspDefinition->dasfMode;
            OMX_PRDSP2(pComponentPrivate->dbg, "AACENC: pComponentPrivate->dasfmode = %d\n",(int)pComponentPrivate->dasfmode);
            pComponentPrivate->bitRateMode = pDspDefinition->aacencHeaderInfo->bitratemode;
            OMX_PRINT2(pComponentPrivate->dbg, "AACENC: pComponentPrivate->bitRateMode = %d\n",(int)pComponentPrivate->bitRateMode);
        pComponentPrivate->streamID = pDspDefinition->streamId;
            break;
        }  

        case OMX_IndexCustomAacEncFramesPerOutBuf:
        {   
            ptrFramesPerOutBuf = (OMX_U16*)ComponentConfigStructure;
            if (ptrFramesPerOutBuf == NULL)
            {
                eError = OMX_ErrorBadParameter;
                OMX_ERROR4(pComponentPrivate->dbg, "%d :: Error: OMX_ErrorBadParameter from SetConfig\n",__LINE__);
		goto EXIT;
            }
            FramesPerOutBuf = *ptrFramesPerOutBuf;
            pComponentPrivate->FramesPer_OutputBuffer= FramesPerOutBuf;
            OMX_PRINT1(pComponentPrivate->dbg, "AACENC: pComponentPrivate->FramesPer_OutputBuffer = %d \n",pComponentPrivate->FramesPer_OutputBuffer);


            pComponentPrivate->CustomConfiguration = OMX_TRUE;  /* Flag specific to test app */
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

            if((write(Aacenc_fdwrite, &cmd_data, sizeof(cmd_data)))<0)
            {   
                OMX_ERROR4(pComponentPrivate->dbg, "[AAC encoder] - fail to send command to audio manager\n");
                OMX_PRINT2(pComponentPrivate->dbg, "[AAC encoder] - fail to send command to audio manager\n");
            }
            else
            {
                OMX_PRMGR2(pComponentPrivate->dbg, "[AAC encoder] - ok to send command to audio manager\n");
            }
#endif
            break;

        case  OMX_IndexCustomAacEncDataPath:
            customFlag = (OMX_S16*)ComponentConfigStructure; 
            if (customFlag == NULL) 
            {
                eError = OMX_ErrorBadParameter;
                goto EXIT;
            }
            dataPath = *customFlag;
            switch(dataPath) 
            {
                case DATAPATH_APPLICATION:
                    OMX_MMMIXER_DATAPATH(pComponentPrivate->sDeviceString, RENDERTYPE_ENCODER, pComponentPrivate->streamID);
                break;

                case DATAPATH_APPLICATION_RTMIXER:
                    strcpy((char*)pComponentPrivate->sDeviceString,(char*)RTM_STRING_ENCODER);
                break;

                default:
                break;
                    
            }
        break;
    case OMX_IndexCustomDebug: 
	    OMX_DBG_SETCONFIG(pComponentPrivate->dbg, ComponentConfigStructure);
	    break;
        
        default:
/*          eError = OMX_ErrorUnsupportedIndex; */
        break;
    }
EXIT:
    if (pComponentPrivate != NULL) {
	OMX_PRINT1(pComponentPrivate->dbg, "%d :: AACENC: Exiting SetConfig\n", __LINE__);
	OMX_PRINT1(pComponentPrivate->dbg, "%d :: AACENC: Returning = 0x%x\n", __LINE__, eError);
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

static OMX_ERRORTYPE GetState (OMX_HANDLETYPE hComponent, OMX_STATETYPE* pState)
{
    OMX_ERRORTYPE eError                        = OMX_ErrorNone;
    OMX_COMPONENTTYPE* pHandle = NULL;
    AACENC_COMPONENT_PRIVATE* pComponentPrivate = NULL;
    struct timespec abs_time = {0,0};
    int nPendingStateChangeRequests = 0;
    int ret = 0;
    /* Set to sufficiently high value */
    int mutex_timeout = 3;

    if(hComponent == NULL || pState == NULL) {
        return OMX_ErrorBadParameter;
    }

    pHandle = (OMX_COMPONENTTYPE*)hComponent;
    pComponentPrivate = (AACENC_COMPONENT_PRIVATE*)pHandle->pComponentPrivate;

    /* Retrieve current state */
    if (pHandle && pHandle->pComponentPrivate) {
        /* Check for any pending state transition requests */
        pthread_mutex_lock(&pComponentPrivate->mutexStateChangeRequest);
        while (nPendingStateChangeRequests != 0) {
           /* Wait for component to complete state transition */
           clock_gettime(CLOCK_REALTIME, &abs_time);
           abs_time.tv_sec += mutex_timeout;
           abs_time.tv_nsec = 0;
           ret = pthread_cond_timedwait(&(pComponentPrivate->StateChangeCondition),
                                        &(pComponentPrivate->mutexStateChangeRequest),
                                        &abs_time);
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
    AACENC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    OMX_CONF_CHECK_CMD(pComponent,pBuffer,1);
    int ret = 0;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef = NULL;

    pComponentPrivate = (AACENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    OMX_PRINT1(pComponentPrivate->dbg, "%d :: AACENC: Entering EmptyThisBuffer\n", __LINE__);

    pPortDef = ((AACENC_COMPONENT_PRIVATE*) 
                    pComponentPrivate)->pPortDef[INPUT_PORT];

#ifdef _ERROR_PROPAGATION__
    if (pComponentPrivate->curState == OMX_StateInvalid)
    {
        eError = OMX_ErrorInvalidState;
        goto EXIT;
    }   
#endif

#ifdef __PERF_INSTRUMENTATION__
        PERF_ReceivedFrame(pComponentPrivate->pPERF,pBuffer->pBuffer, pBuffer->nFilledLen,
                           PERF_ModuleHLMM);
#endif

    OMX_PRBUFFER1(pComponentPrivate->dbg, "AACENC: pBuffer->nSize %d \n",(int)pBuffer->nSize);
    OMX_PRBUFFER1(pComponentPrivate->dbg, "AACENC: size OMX_BUFFERHEADERTYPE %d \n",sizeof(OMX_BUFFERHEADERTYPE));
    
    if(!pPortDef->bEnabled) 
    {
        eError = OMX_ErrorIncorrectStateOperation;
        OMX_ERROR4(pComponentPrivate->dbg, "%d :: Error: About to return OMX_ErrorIncorrectStateOperation\n",__LINE__);
        goto EXIT;
    }
    if (pBuffer == NULL) 
    {
        eError = OMX_ErrorBadParameter;
        OMX_ERROR4(pComponentPrivate->dbg, "%d :: Error: About to return OMX_ErrorBadParameter\n",__LINE__);
        goto EXIT;
    }
    if (pBuffer->nSize != sizeof(OMX_BUFFERHEADERTYPE)) 
    {
        OMX_ERROR4(pComponentPrivate->dbg, "%d :: Error: About to return OMX_ErrorBadParameter\n",__LINE__);
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }


    if (pBuffer->nVersion.nVersion != pComponentPrivate->nVersion) 
    {
        eError = OMX_ErrorVersionMismatch;
        goto EXIT;
    }

    if (pBuffer->nInputPortIndex != INPUT_PORT) 
    {
        OMX_ERROR4(pComponentPrivate->dbg, "%d :: Error: About to return OMX_ErrorBadPortIndex\n",__LINE__);
        eError  = OMX_ErrorBadPortIndex;
        goto EXIT;
    }
    OMX_PRBUFFER2(pComponentPrivate->dbg, "\n------------------------------------------\n\n");
    OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: AACENC: Component Sending Filled ip buff %p  to Component Thread\n", __LINE__,pBuffer);
    OMX_PRBUFFER2(pComponentPrivate->dbg, "\n------------------------------------------\n\n");

    pComponentPrivate->pMarkData = pBuffer->pMarkData;
    pComponentPrivate->hMarkTargetComponent = pBuffer->hMarkTargetComponent;

    pComponentPrivate->nUnhandledEmptyThisBuffers++;

    ret = write (pComponentPrivate->dataPipe[1], &pBuffer, sizeof(OMX_BUFFERHEADERTYPE*));
    if (ret == -1) 
    {
        OMX_ERROR4(pComponentPrivate->dbg, "%d :: Error in Writing to the Data pipe\n", __LINE__);
        eError = OMX_ErrorHardware;
        goto EXIT;
    } else{
        AACENC_IncrementBufferCounterByOne(&bufferReturned_mutex, &pComponentPrivate->EmptythisbufferCount);
    }
EXIT:
    if (pComponentPrivate != NULL) {
	OMX_PRINT1(pComponentPrivate->dbg, "%d :: AACENC: Exiting EmptyThisBuffer\n", __LINE__);
    }
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
    AACENC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    OMX_CONF_CHECK_CMD(pComponent,pBuffer,1);
    int ret = 0;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef = NULL;

    pComponentPrivate = (AACENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    OMX_PRINT1(pComponentPrivate->dbg, "%d :: AACENC: Entering FillThisBuffer\n", __LINE__);

    OMX_PRBUFFER2(pComponentPrivate->dbg, "\n------------------------------------------\n\n");
    OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: AACENC: Component Sending Emptied op buff %p to Component Thread\n",__LINE__,pBuffer);
    OMX_PRBUFFER2(pComponentPrivate->dbg, "\n------------------------------------------\n\n");
    pPortDef = ((AACENC_COMPONENT_PRIVATE*) 
                    pComponentPrivate)->pPortDef[OUTPUT_PORT];

#ifdef _ERROR_PROPAGATION__
    if (pComponentPrivate->curState == OMX_StateInvalid)
    {
        eError = OMX_ErrorInvalidState;
        goto EXIT;
    }   
#endif

#ifdef __PERF_INSTRUMENTATION__
    PERF_ReceivedFrame(pComponentPrivate->pPERF,pBuffer->pBuffer,0,PERF_ModuleHLMM);
#endif

    if(!pPortDef->bEnabled) 
    {
        eError = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }
    if (pBuffer == NULL) 
    {
        eError = OMX_ErrorBadParameter;
        OMX_ERROR4(pComponentPrivate->dbg, " %d :: Error: About to return OMX_ErrorBadParameter\n",__LINE__);
        goto EXIT;
    }
    if (pBuffer->nSize != sizeof(OMX_BUFFERHEADERTYPE)) 
    {
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }

    if (pBuffer->nVersion.nVersion != pComponentPrivate->nVersion) 
    {
        eError = OMX_ErrorVersionMismatch;
        goto EXIT;
    }

    if (pBuffer->nOutputPortIndex != OUTPUT_PORT) 
    {
        eError  = OMX_ErrorBadPortIndex;
        goto EXIT;
    }
    pBuffer->nFilledLen = 0;
    /*Filling the Output buffer with zero */
#ifndef UNDER_CE    
    /*memset(pBuffer->pBuffer, 0, pBuffer->nAllocLen);*/
#endif    

    OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: AACENC: pComponentPrivate->pMarkBuf = %p\n",__LINE__, pComponentPrivate->pMarkBuf);
    OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: AACENC: pComponentPrivate->pMarkData = %p\n",__LINE__, pComponentPrivate->pMarkData);
    if(pComponentPrivate->pMarkBuf) 
    {
        pBuffer->hMarkTargetComponent = pComponentPrivate->pMarkBuf->hMarkTargetComponent;
        pBuffer->pMarkData = pComponentPrivate->pMarkBuf->pMarkData;
        pComponentPrivate->pMarkBuf = NULL;
    }

    if (pComponentPrivate->pMarkData) 
    {
        pBuffer->hMarkTargetComponent = pComponentPrivate->hMarkTargetComponent;
        pBuffer->pMarkData = pComponentPrivate->pMarkData;
        pComponentPrivate->pMarkData = NULL;
    }

    pComponentPrivate->nUnhandledFillThisBuffers++;

    ret = write (pComponentPrivate->dataPipe[1], &pBuffer, sizeof (OMX_BUFFERHEADERTYPE*));
    if (ret == -1) 
    {
        OMX_ERROR4(pComponentPrivate->dbg, "%d :: Error in Writing to the Data pipe\n", __LINE__);
        eError = OMX_ErrorHardware;
        goto EXIT;
    } else {
        AACENC_IncrementBufferCounterByOne(&bufferReturned_mutex, &pComponentPrivate->FillthisbufferCount);
    }
EXIT:
    if (pComponentPrivate != NULL) {
	OMX_PRINT1(pComponentPrivate->dbg, "%d :: AACENC: Exiting FillThisBuffer error= %d \n", __LINE__, eError);
    }
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
    struct OMX_TI_Debug dbg = {0};
    AACENC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    OMX_CONF_CHECK_CMD(pHandle,1,1);
    OMX_COMPONENTTYPE *pComponent = (OMX_COMPONENTTYPE *)pHandle;
    pComponentPrivate = (AACENC_COMPONENT_PRIVATE *)pComponent->pComponentPrivate;
    dbg = pComponentPrivate->dbg;

    OMX_PRINT1(dbg, "%d :: AACENC: ComponentDeInit\n",__LINE__);
    OMX_PRDSP2(dbg, "AACENC:  LCML %p \n",pComponentPrivate->ptrLibLCML);

#ifdef __PERF_INSTRUMENTATION__
    PERF_Boundary(pComponentPrivate->pPERF,PERF_BoundaryStart | PERF_BoundaryCleanup);
#endif

#ifndef UNDER_CE
#ifdef DSP_RENDERING_ON
    close(Aacenc_fdwrite);
    close(Aacenc_fdread);
#endif
#endif

#ifdef RESOURCE_MANAGER_ENABLED
    eError = RMProxy_NewSendCommand(pHandle, RMProxy_FreeResource, OMX_AAC_Encoder_COMPONENT, 0, 3456, NULL);
    if (eError != OMX_ErrorNone) 
    {
         OMX_ERROR4(dbg, "%d :: Error returned from destroy ResourceManagerProxy thread\n",
                                                        __LINE__);
    }

    eError = RMProxy_Deinitalize();
    if (eError != OMX_ErrorNone) 
    {
         OMX_ERROR4(dbg, "%d :: Error returned from destroy ResourceManagerProxy thread\n",__LINE__);
    }
#endif

#ifdef SWAT_ANALYSIS
    SWATAPI_ReleaseHandle(pComponentPrivate->pSwatInfo->pSwatApiHandle);
    SWAT_Boundary(pComponentPrivate->pSwatInfo->pSwatObjHandle,
                          pComponentPrivate->pSwatInfo->ctUC,
                          SWAT_BoundaryComplete | SWAT_BoundaryCleanup);
    OMX_PRINT2(dbg, "%d :: AACENC: Instrumentation: SWAT_BoundaryComplete Done\n",__LINE__);
    SWAT_Done(pComponentPrivate->pSwatInfo->pSwatObjHandle);
#endif

    OMX_PRINT2(dbg, "%d :: AACENC: Inside ComponentDeInit point A \n",__LINE__);
    pComponentPrivate->bIsThreadstop = 1;
    eError = AACENC_StopComponentThread(pHandle);
    OMX_PRINT2(dbg, "%d :: AACENC: Inside ComponentDeInit Point B \n",__LINE__);
    /* Wait for thread to exit so we can get the status into "error" */

    /* close the pipe handles */
    AACENC_FreeCompResources(pHandle);

#ifdef __PERF_INSTRUMENTATION__
        PERF_Boundary(pComponentPrivate->pPERF,
                      PERF_BoundaryComplete | PERF_BoundaryCleanup);
        PERF_Done(pComponentPrivate->pPERF);
#endif
    
    OMX_MEMFREE_STRUCT(pComponentPrivate->pInputBufferList);
    OMX_MEMFREE_STRUCT(pComponentPrivate->pOutputBufferList);
    OMX_PRBUFFER2(dbg, "%d :: AACENC: After AACENC_FreeCompResources\n",__LINE__);
    OMX_PRBUFFER2(dbg, "%d :: AACENC: [FREE] %p\n",__LINE__,pComponentPrivate);

    pthread_mutex_destroy(&pComponentPrivate->mutexStateChangeRequest);
    pthread_cond_destroy(&pComponentPrivate->StateChangeCondition);

    if (pComponentPrivate->sDeviceString != NULL) 
    {
        OMX_MEMFREE_STRUCT(pComponentPrivate->sDeviceString);
    }
    
    /* CLose LCML .      - Note:  Need to handle better - */ 
    if ((pComponentPrivate->ptrLibLCML != NULL && pComponentPrivate->bGotLCML) &&
        (pComponentPrivate->bCodecDestroyed))
    {
        OMX_PRDSP2(dbg, "AACENC: About to Close LCML %p \n",pComponentPrivate->ptrLibLCML);
        dlclose( pComponentPrivate->ptrLibLCML  );   
        pComponentPrivate->ptrLibLCML = NULL;
        OMX_PRDSP2(dbg, "AACENC: Closed LCML \n");  

        pComponentPrivate->bCodecDestroyed = OMX_FALSE;     /* restoring flags */
        pComponentPrivate->bGotLCML        = OMX_FALSE;
    }
    
    OMX_MEMFREE_STRUCT(pComponentPrivate);
    pComponentPrivate = NULL;
    
EXIT:
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
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef= NULL;
    AACENC_COMPONENT_PRIVATE *pComponentPrivate= NULL;
    
    BUFFERLIST *pBufferList = NULL;
    OMX_BUFFERHEADERTYPE *pBufferHeader = NULL;
    OMX_CONF_CHECK_CMD(hComponent,pBuffer,1); 

    pComponentPrivate = (AACENC_COMPONENT_PRIVATE *)
            (((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);
    OMX_PRINT1(pComponentPrivate->dbg, "%d :: AACENC: Entering AllocateBuffer\n", __LINE__);
    
    if (nPortIndex != INPUT_PORT && nPortIndex != OUTPUT_PORT)
    {
        OMX_ERROR4(pComponentPrivate->dbg, "%d :: AllocateBuffer: Error - Unknown port index %ld\n",__LINE__, nPortIndex);
        return OMX_ErrorBadPortIndex;
    }
    pPortDef = ((AACENC_COMPONENT_PRIVATE*)
                    pComponentPrivate)->pPortDef[nPortIndex];

#ifdef _ERROR_PROPAGATION__
    if (pComponentPrivate->curState == OMX_StateInvalid)
    {
        eError = OMX_ErrorInvalidState;
        goto EXIT;
    }   
#endif

#ifdef __PERF_INSTRUMENTATION__
    PERF_ReceivedBuffer(pComponentPrivate->pPERF,(*pBuffer)->pBuffer,nSizeBytes,PERF_ModuleMemory);
#endif
     if (pPortDef->bPopulated)
    {
        OMX_ERROR4(pComponentPrivate->dbg, "%d :: AllocateBuffer - Error: port (%ld) already populated\n", __LINE__, nPortIndex);
        eError = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }
    // FIXME:
    // Should we remove the following while loop and just check make sure that
    // pPortDef->bEnabled == true, as we do in UseBuffer()?
    OMX_PRCOMM2(pComponentPrivate->dbg, "%d :: AACENC: pPortDef = %p\n", __LINE__, pPortDef);
    OMX_PRCOMM2(pComponentPrivate->dbg, "%d :: AACENC: pPortDef->bEnabled = %d\n", __LINE__, pPortDef->bEnabled);
    while (1) 
    {
        if(pPortDef->bEnabled) 
        {
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
    if (nSizeBytes != pPortDef->nBufferSize)
    {
        OMX_ERROR4(pComponentPrivate->dbg, "%d :: Error: nSizeBytes(%ld) != nBufferSize(%ld)\n", __LINE__, nSizeBytes, pPortDef->nBufferSize);
        eError = OMX_ErrorBadParameter;
        goto EXIT;
     }

    OMX_MALLOC_GENERIC(pBufferHeader, OMX_BUFFERHEADERTYPE);
    OMX_PRBUFFER2(pComponentPrivate->dbg, "AACENC: pBufferHeader %p\n",pBufferHeader);

    OMX_MALLOC_SIZE_DSPALIGN(pBufferHeader->pBuffer, nSizeBytes, OMX_U8);

    OMX_PRBUFFER2(pComponentPrivate->dbg, "AACENC: pBufferHeader->pbuffer %p to %p \n",pBufferHeader->pBuffer,(pBufferHeader->pBuffer + sizeof(pBufferHeader->pBuffer)) );

    if (nPortIndex == INPUT_PORT) 
    {
        pBufferList = pComponentPrivate->pInputBufferList;
        pBufferHeader->nInputPortIndex = nPortIndex;
        pBufferHeader->nOutputPortIndex = -1;
    }
    else
     {
         pBufferList = pComponentPrivate->pOutputBufferList;
         pBufferHeader->nInputPortIndex = -1;
         pBufferHeader->nOutputPortIndex = nPortIndex;
    }
    pBufferList->pBufHdr[pBufferList->numBuffers] = pBufferHeader;
    pBufferList->bBufferPending[pBufferList->numBuffers] = 0;
    pBufferList->bufferOwner[pBufferList->numBuffers++] = 1;
    if (pBufferList->numBuffers == pPortDef->nBufferCountActual)
    {  
       pPortDef->bPopulated = OMX_TRUE;
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
    pBufferHeader->nVersion.s.nVersionMajor = AACENC_MAJOR_VER;
    pBufferHeader->nVersion.s.nVersionMinor = AACENC_MINOR_VER;
    pComponentPrivate->nVersion = pBufferHeader->nVersion.nVersion;
    pBufferHeader->nSize = sizeof(OMX_BUFFERHEADERTYPE);
    *pBuffer = pBufferHeader;
    if (pComponentPrivate->bEnableCommandPending && pPortDef->bPopulated) 
    {
        SendCommand (pComponentPrivate->pHandle,OMX_CommandPortEnable,pComponentPrivate->nEnableCommandParam,NULL);
    }

#ifdef __PERF_INSTRUMENTATION__
        PERF_ReceivedBuffer(pComponentPrivate->pPERF,(*pBuffer)->pBuffer, nSizeBytes,PERF_ModuleMemory);
#endif

EXIT:
    if (pComponentPrivate != NULL) {
	OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: AACENC:  AllocateBuffer returning eError =  %d\n", __LINE__, eError);

	OMX_PRBUFFER2(pComponentPrivate->dbg, "AACENC: pBufferHeader = %p\n", pBufferHeader);
	if (pBufferHeader != NULL) {
	    OMX_PRBUFFER2(pComponentPrivate->dbg, "AACENC: pBufferHeader->pBuffer = %p\n", pBufferHeader->pBuffer);
	}
    }
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
    OMX_CONF_CHECK_CMD(hComponent,1,pBuffer);
    OMX_BUFFERHEADERTYPE* buffHdr = NULL;
    OMX_U8* tempBuff = NULL;
    OMX_U32 i = 0;
    int bufferIndex = -1;
    BUFFERLIST *pBuffList = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDef = NULL;
    AACENC_COMPONENT_PRIVATE *pComponentPrivate = ((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate;

    OMX_PRINT1 (pComponentPrivate->dbg, "%d :: AACENC: FreeBuffer for port index %ld\n", __LINE__, nPortIndex);
    
    if (nPortIndex != INPUT_PORT && nPortIndex != OUTPUT_PORT)
    {
        OMX_ERROR4(pComponentPrivate->dbg, "%d :: AACENC: Error - Unknown port index %ld\n",__LINE__, nPortIndex);
        return OMX_ErrorBadPortIndex;
    }
    
    pBuffList = ((nPortIndex == INPUT_PORT)? pComponentPrivate->pInputBufferList: pComponentPrivate->pOutputBufferList);
    pPortDef = pComponentPrivate->pPortDef[nPortIndex];
    for (i = 0; i < pPortDef->nBufferCountActual; ++i)

    {
        buffHdr = pBuffList->pBufHdr[i];
        if (buffHdr == pBuffer) 
        {
            OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: AACENC: Found matching %s buffer\n",__LINE__, nPortIndex == INPUT_PORT? "input": "output");
            OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: AACENC: buffHdr = %p\n",__LINE__,buffHdr);
            OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: AACENC: pBuffer = %p\n",__LINE__,pBuffer);
            bufferIndex = i;
            break;
        }
        else 
        {
            OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: AACENC: This is not a match\n",__LINE__);
            OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: AACENC: buffHdr = %p\n",__LINE__,buffHdr);
            OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: AACENC: pBuffer = %p\n",__LINE__,pBuffer);
        }
    }


    if (bufferIndex == -1) 
    {
        OMX_ERROR4(pComponentPrivate->dbg, "%d :: AACENC: Error - could not find match for buffer %p\n",__LINE__, pBuffer);
        return OMX_ErrorBadParameter;
    }
    
    if (pBuffList->bufferOwner[bufferIndex] == 1)
    {
            OMX_MEMFREE_STRUCT_DSPALIGN(buffHdr->pBuffer, OMX_U8);
#ifdef __PERF_INSTRUMENTATION__
            PERF_SendingBuffer(pComponentPrivate->pPERF,
                               pBufHdr->pBuffer, 
                               pBufHdr->nAllocLen,
                               PERF_ModuleMemory);
#endif
    }
    OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: AACENC: [FREE] %p\n",__LINE__, buffHdr);
    OMX_MEMFREE_STRUCT(buffHdr);
    pBuffList->pBufHdr[bufferIndex] = NULL;
    pBuffList->numBuffers--;
    OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: AACENC: numBuffers = %d \n",__LINE__, pBuffList->numBuffers);
    if (pBuffList->numBuffers < pPortDef->nBufferCountActual)
    {
        pPortDef->bPopulated = OMX_FALSE;
    }

    if (pPortDef->bEnabled &&
        pComponentPrivate->bLoadedCommandPending == OMX_FALSE &&
        (pComponentPrivate->curState == OMX_StateIdle ||
         pComponentPrivate->curState == OMX_StateExecuting ||
         pComponentPrivate->curState == OMX_StatePause))
    {
       OMX_PRCOMM1(pComponentPrivate->dbg, "%d :: AACENC: PortUnpopulated\n",__LINE__);
       pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                              pComponentPrivate->pHandle->pApplicationPrivate,
                                              OMX_EventError,
                                              OMX_ErrorPortUnpopulated,
                                              OMX_TI_ErrorMinor,
                                              "Port Unpopulated");
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

    if (pComponentPrivate->bDisableCommandPending && (pComponentPrivate->pInputBufferList->numBuffers + pComponentPrivate->pOutputBufferList->numBuffers == 0)) 
    {
        SendCommand (pComponentPrivate->pHandle,OMX_CommandPortDisable,pComponentPrivate->bDisableCommandParam,NULL);
    }

EXIT:
    OMX_PRINT1(pComponentPrivate->dbg, "%d :: AACENC: Exiting FreeBuffer\n", __LINE__);
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
static OMX_ERRORTYPE UseBuffer (OMX_IN OMX_HANDLETYPE hComponent,
            OMX_INOUT OMX_BUFFERHEADERTYPE** ppBufferHdr,
            OMX_IN OMX_U32 nPortIndex,
            OMX_IN OMX_PTR pAppPrivate,
            OMX_IN OMX_U32 nSizeBytes,
            OMX_IN OMX_U8* pBuffer)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef= NULL;
    AACENC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    OMX_BUFFERHEADERTYPE *pBufferHeader = NULL;
    BUFFERLIST *pBufferList = NULL;
    OMX_CONF_CHECK_CMD(hComponent,ppBufferHdr,pBuffer);

    OMXDBG_PRINT(stderr, PRINT, 1, 0, "%d :: AACENC: Entering UseBuffer\n", __LINE__);
    OMXDBG_PRINT(stderr, BUFFER, 2, 0, "%d :: AACENC: pBuffer = %p\n", __LINE__,pBuffer);
    if (nPortIndex != INPUT_PORT && nPortIndex != OUTPUT_PORT)
    {
        OMX_ERROR4(pComponentPrivate->dbg, "UseBuffer: Error - Unknown port index %ld", nPortIndex);
        return OMX_ErrorBadPortIndex;
    }

    pComponentPrivate = (AACENC_COMPONENT_PRIVATE *)
            (((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);

#ifdef _ERROR_PROPAGATION__
    if (pComponentPrivate->curState == OMX_StateInvalid)
    {
        eError = OMX_ErrorInvalidState;
        goto EXIT;
    }
    
#endif

#ifdef __PERF_INSTRUMENTATION__
        PERF_ReceivedBuffer(pComponentPrivate->pPERF,pBuffer, nSizeBytes,
                            PERF_ModuleHLMM);
#endif

    pPortDef = ((AACENC_COMPONENT_PRIVATE*)
                    pComponentPrivate)->pPortDef[nPortIndex];

    if(!pPortDef->bEnabled || pPortDef->bPopulated) 
    {
        if (!pPortDef->bEnabled)
        {
            OMX_ERROR4(pComponentPrivate->dbg, "%d :: UseBuffer - Error: port (%ld) not enabled\n", __LINE__, nPortIndex);
        }
        if (pPortDef->bPopulated)
        {
            OMX_ERROR4(pComponentPrivate->dbg, "%d :: UseBuffer - Error: port (%ld) already populated\n", __LINE__, nPortIndex);
        }
        eError = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }

    OMX_PRINT2(pComponentPrivate->dbg, "AACENC: nSizeBytes = %ld\n",nSizeBytes);
    OMX_PRBUFFER1(pComponentPrivate->dbg, "AACENC: pPortDef->nBufferSize = %ld\n",pPortDef->nBufferSize);
    OMX_PRBUFFER1(pComponentPrivate->dbg, "AACENC: pPortDef->bPopulated = %d\n",pPortDef->bPopulated);
    if(nSizeBytes != pPortDef->nBufferSize) 
    {
        OMX_ERROR4(pComponentPrivate->dbg, "%d :: Error: nSizeBytes(%ld) != nBufferSize(%ld)\n", __LINE__, nSizeBytes, pPortDef->nBufferSize);
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }

    OMX_MALLOC_GENERIC(pBufferHeader, OMX_BUFFERHEADERTYPE);

    OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: AACENC: [ALLOC] %p\n",__LINE__,pBufferHeader);

    if (nPortIndex == OUTPUT_PORT) 
    {
        pBufferList = pComponentPrivate->pOutputBufferList;
        pBufferHeader->nInputPortIndex = -1;
        pBufferHeader->nOutputPortIndex = nPortIndex;
    } else {         
        pBufferList = pComponentPrivate->pInputBufferList;
        pBufferHeader->nInputPortIndex = nPortIndex;
        pBufferHeader->nOutputPortIndex = -1;
    }
    pBufferList->pBufHdr[pBufferList->numBuffers] = pBufferHeader;
    pBufferList->bBufferPending[pBufferList->numBuffers] = 0;
    pBufferList->bufferOwner[pBufferList->numBuffers++] = 0;
    if (pBufferList->numBuffers == pPortDef->nBufferCountActual)
    {
         pPortDef->bPopulated = OMX_TRUE;
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
    pBufferHeader->nVersion.s.nVersionMajor = AACENC_MAJOR_VER;
    pBufferHeader->nVersion.s.nVersionMinor = AACENC_MINOR_VER;
    pComponentPrivate->nVersion = pBufferHeader->nVersion.nVersion;
    pBufferHeader->pBuffer = pBuffer;
    pBufferHeader->nSize = sizeof(OMX_BUFFERHEADERTYPE);
    *ppBufferHdr = pBufferHeader;
    OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: AACENC: pBufferHeader = %p\n",__LINE__,pBufferHeader);

    if (pComponentPrivate->bEnableCommandPending) 
    {
        SendCommand (pComponentPrivate->pHandle,OMX_CommandPortEnable,pComponentPrivate->nEnableCommandParam,NULL);
    }

EXIT:
    if (pComponentPrivate != NULL) {
	OMX_PRINT1(pComponentPrivate->dbg, "AACENC: [UseBuffer] pBufferHeader = %p\n", pBufferHeader);
	if (pBufferHeader != NULL) {
	    OMX_PRINT1(pComponentPrivate->dbg, "AACENC: [UseBuffer] pBufferHeader->pBuffer = %p\n", pBufferHeader->pBuffer);
	}
    }

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

    OMXDBG_PRINT(stderr, PRINT, 1, 0, "AACENC: GetExtensionIndex\n");
    if (!(strcmp(cParameterName,"OMX.TI.index.config.aacencHeaderInfo"))) 
    {
        *pIndexType = OMX_IndexCustomAacEncHeaderInfoConfig;
    }
    else if(!(strcmp(cParameterName,"OMX.TI.index.config.aacencstreamIDinfo"))) 
    {
        *pIndexType = OMX_IndexCustomAacEncStreamIDConfig;
    }
    else if(!(strcmp(cParameterName,"OMX.TI.index.config.aac.datapath"))) 
    {
        *pIndexType = OMX_IndexCustomAacEncDataPath;
    }
    else if (!(strcmp(cParameterName,"OMX.TI.index.config.aacencframesPerOutBuf")))
    {
        *pIndexType =OMX_IndexCustomAacEncFramesPerOutBuf;
    }
    else if (!(strcmp(cParameterName,"OMX.TI.AAC.Encode.Debug")))
    {
        *pIndexType =OMX_IndexCustomDebug;
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
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    AACENC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    pComponentPrivate = (AACENC_COMPONENT_PRIVATE *)(((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);
    if(nIndex == 0)
    {
      OMX_CONF_CHECK_CMD(cRole, 1, 1);
      memcpy(cRole, &pComponentPrivate->componentRole.cRole, sizeof(OMX_U8) * OMX_MAX_STRINGNAME_SIZE); 
    }
    else 
    {
      eError = OMX_ErrorNoMore;
      goto EXIT;
    }

EXIT:  
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
