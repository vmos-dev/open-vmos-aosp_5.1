
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
 * @file OMX_G711Decoder.c
 *
 * This file implements OMX Component for G711 decoder that
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
#include <AudioManagerAPI.h>
#endif

#ifdef RESOURCE_MANAGER_ENABLED
#include <ResourceManagerProxyAPI.h>
#endif

/*-------program files ----------------------------------------*/
#include <OMX_Component.h>
#include <TIDspOmx.h>
#include "OMX_G711Dec_Utils.h"

#ifdef G711DEC_MEMDEBUG
extern void *arr[500] = {NULL};
extern int lines[500] = {0};
extern int bytes[500] = {0};
extern char file[500][50] = {""};
/*extern int ind=0;*/
#define newmalloc(x) mymalloc(__LINE__,__FILE__,x)
void * mymalloc(int line, char *s, int size);
#define newfree(z) myfree(z,__LINE__,__FILE__)
void myfree(void *dp, int line, char *s);
#else
#define newmalloc(x) malloc(x)
#define newfree(z) free(z)
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
#define G711_DEC_ROLE "audio_decoder.g711"
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
    G711DEC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    OMX_AUDIO_PARAM_PCMMODETYPE *g711_ip = NULL, *g711_op = NULL;
    G711DEC_PORT_TYPE *pCompPort = NULL;
    OMX_AUDIO_PARAM_PORTFORMATTYPE *pInPortFormatArr = NULL;
    OMX_AUDIO_PARAM_PORTFORMATTYPE *pOutPortFormatArr = NULL;    
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE*) hComp;
    OMX_S16 i = 0;

    G711DEC_DPRINT ("%d ::OMX_ComponentInit\n", __LINE__);

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
    G711D_OMX_MALLOC(pHandle->pComponentPrivate, G711DEC_COMPONENT_PRIVATE);

    ((G711DEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate)->pHandle = pHandle;

    /* Initialize component data structures to default values */
    ((G711DEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate)->sPortParam.nPorts = 0x2;
    ((G711DEC_COMPONENT_PRIVATE *)
     pHandle->pComponentPrivate)->sPortParam.nStartPortNumber = 0x0;

    eError = OMX_ErrorNone;

    G711D_OMX_MALLOC(g711_ip, OMX_AUDIO_PARAM_PCMMODETYPE);
    G711D_OMX_MALLOC(g711_op, OMX_AUDIO_PARAM_PCMMODETYPE);

    G711DEC_DPRINT ("%d ::LINE \n", __LINE__);

    ((G711DEC_COMPONENT_PRIVATE *)
     pHandle->pComponentPrivate)->g711Params[G711DEC_INPUT_PORT] = g711_ip;
    ((G711DEC_COMPONENT_PRIVATE *)
     pHandle->pComponentPrivate)->g711Params[G711DEC_OUTPUT_PORT] = g711_op;

    g711_ip->nPortIndex = G711DEC_INPUT_PORT;
    g711_op->nPortIndex = G711DEC_OUTPUT_PORT;
        
    G711DEC_DPRINT("%d Malloced g711Params[G711DEC_INPUT_PORT] = 0x%x\n",__LINE__,
                   (unsigned int)((G711DEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate)->g711Params[G711DEC_INPUT_PORT]);
    G711DEC_DPRINT("%d Malloced g711Params[G711DEC_OUTPUT_PORT] = 0x%x\n",__LINE__,
                   (unsigned int)((G711DEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate)->g711Params[G711DEC_OUTPUT_PORT]);


    pComponentPrivate = pHandle->pComponentPrivate;
    G711D_OMX_MALLOC(pComponentPrivate->pInputBufferList, G711DEC_BUFFERLIST);
    pComponentPrivate->pInputBufferList->numBuffers = 0; /* initialize number of buffers */
    G711D_OMX_MALLOC(pComponentPrivate->pOutputBufferList, G711DEC_BUFFERLIST);

    G711D_OMX_MALLOC(pCompPort, G711DEC_PORT_TYPE);
    pComponentPrivate->pCompPort[G711DEC_INPUT_PORT] = pCompPort;
    G711D_OMX_MALLOC(pCompPort, G711DEC_PORT_TYPE);
    pComponentPrivate->pCompPort[G711DEC_OUTPUT_PORT] = pCompPort;

    pComponentPrivate->pOutputBufferList->numBuffers = 0; /* initialize number of buffers */
    pComponentPrivate->bPlayCompleteFlag = 0;
    
    for (i=0; i < MAX_NUM_OF_BUFS; i++) {
        pComponentPrivate->pOutputBufferList->pBufHdr[i] = NULL;
        pComponentPrivate->pInputBufferList->pBufHdr[i] = NULL;
    }
    
    G711DEC_DPRINT("Setting dasfmode and mimemode to 0\n");
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
    pComponentPrivate->bEnableCommandParam = 0;
    pComponentPrivate->nRuntimeInputBuffers = 0;
    
    for (i=0; i < MAX_NUM_OF_BUFS; i++) {
        pComponentPrivate->pInputBufHdrPending[i] = NULL;
        pComponentPrivate->pOutputBufHdrPending[i] = NULL;
    }
    
    pComponentPrivate->bJustReenabled = 0;
    pComponentPrivate->nInvalidFrameCount = 0;
    pComponentPrivate->nNumInputBufPending = 0;
    pComponentPrivate->nNumOutputBufPending = 0;
    pComponentPrivate->bDisableCommandPending = 0;
    pComponentPrivate->bEnableCommandPending = 0;
    pComponentPrivate->nOutStandingFillDones = 0;
    pComponentPrivate->bStopSent = 0;
    pComponentPrivate->bBypassDSP = OMX_FALSE;
    pComponentPrivate->bNoIdleOnStop = OMX_FALSE;  
    pComponentPrivate->bPreempted = OMX_FALSE; 

    G711D_OMX_MALLOC_SIZE(pComponentPrivate->sDeviceString, 100*sizeof(OMX_STRING), OMX_STRING);
    /* initialize role name */
    strcpy((char*)pComponentPrivate->componentRole.cRole,G711_DEC_ROLE);
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
#else
    OMX_CreateEvent(&(pComponentPrivate->AlloBuf_event));
    pComponentPrivate->AlloBuf_waitingsignal = 0;
    
    OMX_CreateEvent(&(pComponentPrivate->InLoaded_event));
    pComponentPrivate->InLoaded_readytoidle = 0;
    
    OMX_CreateEvent(&(pComponentPrivate->InIdle_event));
    pComponentPrivate->InIdle_goingtoloaded = 0;
#endif
    /* Removing sleep() calls. Initialization.*/    
    G711D_OMX_MALLOC(pPortDef_ip, OMX_PARAM_PORTDEFINITIONTYPE);
    G711D_OMX_MALLOC(pPortDef_op, OMX_PARAM_PORTDEFINITIONTYPE);

    G711DEC_DPRINT ("%d ::pPortDef_ip = 0x%x\n", __LINE__,(unsigned int)pPortDef_ip);
    G711DEC_DPRINT ("%d ::pPortDef_op = 0x%x\n", __LINE__,(unsigned int)pPortDef_op);

    pComponentPrivate->pPortDef[G711DEC_INPUT_PORT] = pPortDef_ip;
    pComponentPrivate->pPortDef[G711DEC_OUTPUT_PORT] = pPortDef_op;

    pPortDef_ip->nSize                              = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
    pPortDef_ip->nPortIndex                         = 0x0;
    pPortDef_ip->nBufferCountActual                 = NUM_G711DEC_INPUT_BUFFERS;
    pPortDef_ip->nBufferCountMin                    = NUM_G711DEC_INPUT_BUFFERS;
    pPortDef_ip->eDir                               = OMX_DirInput;
    pPortDef_ip->bEnabled                           = OMX_TRUE;
    pPortDef_ip->nBufferSize                        = INPUT_G711DEC_BUFFER_SIZE;
    pPortDef_ip->bPopulated                         = 0;
    pPortDef_ip->eDomain                            = OMX_PortDomainAudio;
    pPortDef_ip->format.audio.eEncoding             = OMX_AUDIO_CodingG711;
    pPortDef_ip->format.audio.cMIMEType             = NULL;
    pPortDef_ip->format.audio.pNativeRender         = NULL;
    pPortDef_ip->format.audio.bFlagErrorConcealment = OMX_FALSE;
    
     
    pPortDef_op->nSize                              = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
    pPortDef_op->nPortIndex                         = 0x1;
    pPortDef_op->nBufferCountActual                 = NUM_G711DEC_OUTPUT_BUFFERS;
    pPortDef_op->nBufferCountMin                    = NUM_G711DEC_OUTPUT_BUFFERS;
    pPortDef_op->eDir                               = OMX_DirOutput;
    pPortDef_op->bEnabled                           = OMX_TRUE;
    pPortDef_op->nBufferSize                        = OUTPUT_G711DEC_BUFFER_SIZE;
    pPortDef_op->bPopulated                         = 0;
    pPortDef_op->eDomain                            = OMX_PortDomainAudio;
    pPortDef_op->format.audio.eEncoding             = OMX_AUDIO_CodingPCM;
    pPortDef_op->format.audio.cMIMEType             = NULL;
    pPortDef_op->format.audio.pNativeRender         = NULL;
    pPortDef_op->format.audio.bFlagErrorConcealment = OMX_FALSE;

    pComponentPrivate->sInPortFormat.nPortIndex    = G711DEC_INPUT_PORT;
    pComponentPrivate->sInPortFormat.nIndex        = OMX_IndexParamAudioPcm;
    pComponentPrivate->sInPortFormat.eEncoding     = OMX_AUDIO_CodingG711;

    pComponentPrivate->sOutPortFormat.nPortIndex    = G711DEC_OUTPUT_PORT;
    pComponentPrivate->sOutPortFormat.nIndex        = OMX_IndexParamAudioPcm;
    pComponentPrivate->sOutPortFormat.eEncoding     = OMX_AUDIO_CodingPCM;   

    G711D_OMX_MALLOC(pComponentPrivate->pCompPort[G711DEC_INPUT_PORT]->pPortFormat, OMX_AUDIO_PARAM_PORTFORMATTYPE);
    OMX_NBCONF_INIT_STRUCT(pComponentPrivate->pCompPort[G711DEC_INPUT_PORT]->pPortFormat, OMX_AUDIO_PARAM_PORTFORMATTYPE);
    /* Set input port format defaults */
    pInPortFormatArr = pComponentPrivate->pCompPort[G711DEC_INPUT_PORT]->pPortFormat;
    OMX_NBCONF_INIT_STRUCT(pInPortFormatArr, OMX_AUDIO_PARAM_PORTFORMATTYPE);
    pInPortFormatArr->nPortIndex         = G711DEC_INPUT_PORT;
    pInPortFormatArr->nIndex             = OMX_IndexParamAudioPcm;
    pInPortFormatArr->eEncoding          = OMX_AUDIO_CodingG711;

    G711D_OMX_MALLOC(pComponentPrivate->pCompPort[G711DEC_OUTPUT_PORT]->pPortFormat, OMX_AUDIO_PARAM_PORTFORMATTYPE);
    OMX_NBCONF_INIT_STRUCT(pComponentPrivate->pCompPort[G711DEC_OUTPUT_PORT]->pPortFormat, OMX_AUDIO_PARAM_PORTFORMATTYPE);
    /* Set output port format defaults */
    pOutPortFormatArr = pComponentPrivate->pCompPort[G711DEC_OUTPUT_PORT]->pPortFormat;
    OMX_NBCONF_INIT_STRUCT(pOutPortFormatArr, OMX_AUDIO_PARAM_PORTFORMATTYPE);
    pOutPortFormatArr->nPortIndex         = G711DEC_OUTPUT_PORT;
    pOutPortFormatArr->nIndex             = OMX_IndexParamAudioPcm;
    pOutPortFormatArr->eEncoding          = OMX_AUDIO_CodingPCM;

#ifdef RESOURCE_MANAGER_ENABLED
    G711DEC_DPRINT ("%d %s Initialize RM Proxy...\n", __LINE__,__FUNCTION__);
    eError = RMProxy_NewInitalize();
    G711DEC_DPRINT ("%d ::OMX_ComponentInit\n", __LINE__);
    if (eError != OMX_ErrorNone) {
        G711DEC_DPRINT ("%d ::Error returned from loading ResourceManagerProxy thread\n", __LINE__);
        if( eError == OMX_ErrorInsufficientResources) 
            G711DEC_PRINT("Error OMX_ErrorInsufficientResources\n");
        else
            G711DEC_PRINT("eError 0x%x\n", eError);        
        goto EXIT;
    }
#endif   

    eError = G711DEC_StartComponentThread(pHandle);
    G711DEC_DPRINT ("%d ::OMX_ComponentInit\n", __LINE__);
    if (eError != OMX_ErrorNone) {
        G711DEC_DPRINT ("%d ::Error returned from the Component\n", __LINE__);
        goto EXIT;
    }
    G711DEC_DPRINT ("%d ::OMX_ComponentInit\n", __LINE__);

#ifdef DSP_RENDERING_ON
   
    if((pComponentPrivate->fdwrite=open(FIFO1,O_WRONLY))<0) {
        G711DEC_DPRINT("[G711 Dec Component] - failure to open WRITE pipe\n");
    }

    if((pComponentPrivate->fdread=open(FIFO2,O_RDONLY))<0) {
        G711DEC_DPRINT("[G711 Dec Component] - failure to open READ pipe\n");
        goto EXIT;
    }
    G711DEC_DPRINT ("%d ::OMX_ComponentInit\n", __LINE__);

#endif
 EXIT:
    if(eError == OMX_ErrorInsufficientResources)
    {
        OMX_G711DECMEMFREE_STRUCT(g711_ip);
        OMX_G711DECMEMFREE_STRUCT(g711_op);
	 if (pComponentPrivate != NULL) {
	     OMX_G711DECMEMFREE_STRUCT(pComponentPrivate->pInputBufferList);
	     OMX_G711DECMEMFREE_STRUCT(pComponentPrivate->pOutputBufferList);
	     OMX_G711DECMEMFREE_STRUCT(pComponentPrivate->pCompPort[G711DEC_INPUT_PORT]->pPortFormat);
	     OMX_G711DECMEMFREE_STRUCT(pComponentPrivate->pCompPort[G711DEC_OUTPUT_PORT]->pPortFormat);
	     OMX_G711DECMEMFREE_STRUCT(pComponentPrivate->pCompPort[G711DEC_INPUT_PORT]);
	     OMX_G711DECMEMFREE_STRUCT(pComponentPrivate->pCompPort[G711DEC_OUTPUT_PORT]);
	     OMX_G711DECMEMFREE_STRUCT(pComponentPrivate->sDeviceString);
	 }
	 OMX_G711DECMEMFREE_STRUCT(pHandle->pComponentPrivate);
        OMX_G711DECMEMFREE_STRUCT(pPortDef_ip);
        OMX_G711DECMEMFREE_STRUCT(pPortDef_op);

    }
    G711DEC_DPRINT ("%d ::OMX_ComponentInit - returning %d\n", __LINE__,eError);
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

    G711DEC_COMPONENT_PRIVATE *pComponentPrivate =
        (G711DEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;

    if (pCallBacks == NULL) {
        G711DEC_DPRINT("About to return OMX_ErrorBadParameter on line %d\n",__LINE__);
        eError = OMX_ErrorBadParameter;
        G711DEC_DPRINT ("%d :: Received the empty callbacks from the application\n",__LINE__);
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
    G711DEC_DPRINT (stderr, "Inside the GetComponentVersion\n");
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
    G711DEC_COMPONENT_PRIVATE *pCompPrivate =
        (G711DEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;

    G711DEC_DPRINT ("%d:::Inside SendCommand\n", __LINE__);

    G711DEC_DPRINT("phandle = %p\n", phandle);
    G711DEC_DPRINT("pCompPrivate = %p\n", pCompPrivate);

    pCompPrivate->pHandle = phandle;

    if(pCompPrivate->curState == OMX_StateInvalid){
        G711DEC_DPRINT ("%d:::Inside SendCommand\n", __LINE__);
        eError = OMX_ErrorInvalidState;
        G711DEC_DPRINT("%d :: G711DEC: Error Notofication \
                                         Sent to App\n",__LINE__);
        pCompPrivate->cbInfo.EventHandler (pHandle, pHandle->pApplicationPrivate,
                                           OMX_EventError, OMX_ErrorInvalidState,
                                           0, "Invalid State");

        goto EXIT;
    }


    switch(Cmd) {
    case OMX_CommandStateSet:
        G711DEC_DPRINT ("%d:::Inside SendCommand\n",__LINE__);
        G711DEC_DPRINT ("%d:::pCompPrivate->curState = %d\n",
                        __LINE__,pCompPrivate->curState);
        if (nParam == OMX_StateLoaded) {
            pCompPrivate->bLoadedCommandPending = OMX_TRUE;
        }
        if(pCompPrivate->curState == OMX_StateLoaded) {
            if((nParam == OMX_StateExecuting) || (nParam == OMX_StatePause)) {
                pCompPrivate->cbInfo.EventHandler (pHandle,
                                                   pHandle->pApplicationPrivate,
                                                   OMX_EventError,
                                                   OMX_ErrorIncorrectStateTransition,
                                                   0, NULL);
                goto EXIT;
            }

            if(nParam == OMX_StateInvalid) {
                G711DEC_DPRINT ("%d:::Inside SendCommand\n",__LINE__);
                pCompPrivate->curState = OMX_StateInvalid;
                pCompPrivate->cbInfo.EventHandler (pHandle,
                                                   pHandle->pApplicationPrivate,
                                                   OMX_EventError,
                                                   OMX_ErrorInvalidState,
                                                   0, NULL);
                goto EXIT;
            }
        }
        break;
    case OMX_CommandFlush:
        G711DEC_DPRINT ("%d:::Inside SendCommand\n",__LINE__);
        if(nParam > 1 && nParam != -1) {
            eError = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        break;
    case OMX_CommandPortDisable:
        G711DEC_DPRINT ("%d:::Inside SendCommand\n",__LINE__);
        break;
    case OMX_CommandPortEnable:
        G711DEC_DPRINT ("%d:::Inside SendCommand\n",__LINE__);
        break;
    case OMX_CommandMarkBuffer:
        G711DEC_DPRINT ("%d:::Inside SendCommand\n",__LINE__);
        if (nParam > 0) {
            eError = OMX_ErrorBadPortIndex;
            goto EXIT;
        }
        break;
    default:
        G711DEC_DPRINT("%d :: G711DEC: Command Received Default \
                                                      error\n",__LINE__);
        pCompPrivate->cbInfo.EventHandler (pHandle, pHandle->pApplicationPrivate,
                                           OMX_EventError, OMX_ErrorUndefined,0,
                                           "Invalid Command");
        break;

    }

    G711DEC_DPRINT ("%d:::Inside SendCommand\n",__LINE__);
    nRet = write (pCompPrivate->cmdPipe[1], &Cmd, sizeof(Cmd));
    
    if (nRet == -1) {
        G711DEC_DPRINT ("%d:::Inside SendCommand\n",__LINE__);
        eError = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    if (Cmd == OMX_CommandMarkBuffer) {
        nRet = write(pCompPrivate->cmdDataPipe[1], &pCmdData, sizeof(OMX_PTR));
    }
    else {
        nRet = write(pCompPrivate->cmdDataPipe[1], &nParam, sizeof(OMX_U32));
    }

    G711DEC_DPRINT ("%d:::Inside SendCommand\n",__LINE__);
    G711DEC_DPRINT ("%d:::nRet = %d\n",__LINE__,nRet);
    if (nRet == -1) {
        G711DEC_DPRINT ("%d:::Inside SendCommand\n",__LINE__);
        eError = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

#ifdef DSP_RENDERING_ON
    if(Cmd == OMX_CommandStateSet && nParam == OMX_StateExecuting)
    {
        /* enable Tee device command*/
        cmd_data.hComponent = pHandle;
        cmd_data.AM_Cmd = AM_CommandTDNDownlinkMode;
        cmd_data.param1 = 0;
        cmd_data.param2 = 0;
        cmd_data.streamID = 0;
        if((write(pCompPrivate->fdwrite, &cmd_data, sizeof(cmd_data)))<0)
        {
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
    G711DEC_COMPONENT_PRIVATE  *pComponentPrivate = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE *pParameterStructure = NULL;

    G711DEC_DPRINT ("Inside the GetParameter Line %d\n",__LINE__);
    G711DEC_DPRINT("%d :: Inside the GetParameter:: %x\n",__LINE__,nParamIndex);

    pComponentPrivate = (G711DEC_COMPONENT_PRIVATE *)(((OMX_COMPONENTTYPE*)hComp)->pComponentPrivate);
    pParameterStructure = (OMX_PARAM_PORTDEFINITIONTYPE*)ComponentParameterStructure;

    if (pParameterStructure == NULL) {
        eError = OMX_ErrorBadParameter;
        goto EXIT;

    }

    G711DEC_DPRINT("pParameterStructure = %p\n",pParameterStructure);
    G711DEC_DPRINT ("Inside the GetParameter Line %d\n",__LINE__);

    if(pComponentPrivate->curState == OMX_StateInvalid) {
        pComponentPrivate->cbInfo.EventHandler(
                                               hComp,
                                               ((OMX_COMPONENTTYPE *)hComp)->pApplicationPrivate,
                                               OMX_EventError,
                                               OMX_ErrorIncorrectStateOperation,
                                               0, NULL);
                                
        G711DEC_DPRINT ("Inside the GetParameter Line %d\n",__LINE__);
    }
        
    G711DEC_DPRINT ("Inside the GetParameter Line %d\n",__LINE__);
        
    switch(nParamIndex){

    case OMX_IndexParamAudioInit:
        G711DEC_DPRINT ("Inside the GetParameter Line %d\n",__LINE__);
        G711DEC_DPRINT ("OMX_IndexParamAudioInit\n");
        memcpy(ComponentParameterStructure, 
               &pComponentPrivate->sPortParam, 
               sizeof(OMX_PORT_PARAM_TYPE));
        break;

    case OMX_IndexParamPortDefinition:
        G711DEC_DPRINT ("Inside the GetParameter Line %d\n",__LINE__);
        G711DEC_DPRINT ("pParameterStructure->nPortIndex = %d\n",(int)pParameterStructure->nPortIndex);
        G711DEC_DPRINT ("pComponentPrivate->pPortDef[G711DEC_INPUT_PORT]->nPortIndex = %d\n",(int)pComponentPrivate->pPortDef[G711DEC_INPUT_PORT]->nPortIndex);
                
        if(((OMX_PARAM_PORTDEFINITIONTYPE *)(ComponentParameterStructure))->nPortIndex ==
           pComponentPrivate->pPortDef[G711DEC_INPUT_PORT]->nPortIndex) 
        {
            G711DEC_DPRINT ("Inside the GetParameter Line %d\n",__LINE__);

            memcpy(ComponentParameterStructure,
                   pComponentPrivate->pPortDef[G711DEC_INPUT_PORT],
                   sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
                               
            G711DEC_DPRINT ("Inside the GetParameter Line %d\n",__LINE__);

        } else if(((OMX_PARAM_PORTDEFINITIONTYPE *)(ComponentParameterStructure))->nPortIndex ==
                  pComponentPrivate->pPortDef[G711DEC_OUTPUT_PORT]->nPortIndex) 
        {
            G711DEC_DPRINT ("Inside the GetParameter Line %d\n",__LINE__);

            memcpy(ComponentParameterStructure,
                   pComponentPrivate->pPortDef[G711DEC_OUTPUT_PORT],
                   sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
            G711DEC_DPRINT ("Inside the GetParameter Line %d\n",__LINE__);

        } else {
            G711DEC_DPRINT ("Inside the GetParameter Line %d\n",__LINE__);
            eError = OMX_ErrorBadPortIndex;
        }
        break;

    case OMX_IndexParamAudioPortFormat:
        G711DEC_DPRINT ("Inside the GetParameter Line %d\n",__LINE__);
        G711DEC_DPRINT ("((OMX_AUDIO_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nPortIndex = %d\n",(int)((OMX_AUDIO_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nPortIndex);
        G711DEC_DPRINT ("pComponentPrivate->sInPortFormat.nPortIndex= %d\n",(int)pComponentPrivate->sInPortFormat.nPortIndex );
        G711DEC_DPRINT ("pComponentPrivate->sOutPortFormat.nPortIndex= %d\n",(int)pComponentPrivate->sOutPortFormat.nPortIndex);
                
        if(((OMX_AUDIO_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nPortIndex ==
           pComponentPrivate->pPortDef[G711DEC_INPUT_PORT]->nPortIndex) 
        {

            G711DEC_DPRINT ("Inside the GetParameter Line %d\n",__LINE__);
                    
            if(((OMX_AUDIO_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nIndex >
               pComponentPrivate->sInPortFormat.nIndex)
            {
                eError = OMX_ErrorNoMore;
            }
                    
            else{
                memcpy(ComponentParameterStructure, &pComponentPrivate->sInPortFormat,
                       sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
            }
        }
        else if(((OMX_AUDIO_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nPortIndex ==
                pComponentPrivate->pPortDef[G711DEC_OUTPUT_PORT]->nPortIndex){
                
            G711DEC_DPRINT ("Inside the GetParameter Line %d\n",__LINE__);

            if(((OMX_AUDIO_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nIndex >
               pComponentPrivate->sOutPortFormat.nIndex)
            {
                G711DEC_DPRINT ("Inside the GetParameter Line %d\n",__LINE__);
                eError = OMX_ErrorNoMore;
            }
            else{
                G711DEC_DPRINT ("Inside the GetParameter Line %d\n",__LINE__);
                memcpy(ComponentParameterStructure, &pComponentPrivate->sOutPortFormat,
                       sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
            }
        }
        else{
            G711DEC_DPRINT ("Inside the GetParameter Line %d\n",__LINE__);
            eError = OMX_ErrorBadPortIndex;
        }
        break;

    case OMX_IndexParamAudioPcm:
        G711DEC_DPRINT("%d :: GetParameter OMX_IndexParamAudioPcmG711 \n",__LINE__);
        if(((OMX_AUDIO_PARAM_PCMMODETYPE *)(ComponentParameterStructure))->nPortIndex ==
           pComponentPrivate->g711Params[G711DEC_INPUT_PORT]->nPortIndex){
            G711DEC_DPRINT("%d :: GetParameter OMX_IndexParamAudioPcmG711 \n",__LINE__);
            memcpy(ComponentParameterStructure,
                   pComponentPrivate->g711Params[G711DEC_INPUT_PORT],
                   sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
                                
        } else if(((OMX_AUDIO_PARAM_PCMMODETYPE *)(ComponentParameterStructure))->nPortIndex ==
                  pComponentPrivate->g711Params[G711DEC_OUTPUT_PORT]->nPortIndex){
            G711DEC_DPRINT("%d :: GetParameter OMX_IndexParamAudioPcmG711 \n",__LINE__);
            memcpy(ComponentParameterStructure,
                   pComponentPrivate->g711Params[G711DEC_OUTPUT_PORT],
                   sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));

        } else {
            eError = OMX_ErrorBadPortIndex;
        }
        break;
            
    case OMX_IndexParamCompBufferSupplier:
        if(((OMX_PARAM_BUFFERSUPPLIERTYPE *)(ComponentParameterStructure))->nPortIndex == OMX_DirInput) {
            G711DEC_DPRINT(":: GetParameter OMX_IndexParamCompBufferSupplier \n");
        }
        else
            if(((OMX_PARAM_BUFFERSUPPLIERTYPE *)(ComponentParameterStructure))->nPortIndex == OMX_DirOutput) {
                G711DEC_DPRINT(":: GetParameter OMX_IndexParamCompBufferSupplier \n");
            } 
            else {
                G711DEC_DPRINT(":: OMX_ErrorBadPortIndex from GetParameter");
                eError = OMX_ErrorBadPortIndex;
            }
        break;

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
    G711DEC_DPRINT("%d :: Exiting GetParameter:: %x\n",__LINE__,nParamIndex);
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

static OMX_ERRORTYPE    SetParameter (OMX_HANDLETYPE hComp,
                                      OMX_INDEXTYPE nParamIndex,
                                      OMX_PTR pCompParam)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    OMX_COMPONENTTYPE* pHandle= (OMX_COMPONENTTYPE*)hComp;
    OMX_AUDIO_PARAM_PORTFORMATTYPE* pComponentParam = NULL;
    G711DEC_COMPONENT_PRIVATE  *pComponentPrivate = NULL;

    OMX_PARAM_COMPONENTROLETYPE  *pRole = NULL;
    OMX_PARAM_BUFFERSUPPLIERTYPE sBufferSupplier;
    pComponentPrivate = (G711DEC_COMPONENT_PRIVATE *)(((OMX_COMPONENTTYPE*)hComp)->pComponentPrivate);
    
    if (pCompParam == NULL) {
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }

    switch(nParamIndex) {
    case OMX_IndexParamAudioPortFormat:
        G711DEC_DPRINT("%d :: SetParameter OMX_IndexParamAudioPortFormat \n",__LINE__);
        pComponentParam = (OMX_AUDIO_PARAM_PORTFORMATTYPE *)pCompParam;
        if ( pComponentParam->nPortIndex == pComponentPrivate->pCompPort[G711DEC_INPUT_PORT]->pPortFormat->nPortIndex )
        {
            memcpy(pComponentPrivate->pCompPort[G711DEC_INPUT_PORT]->pPortFormat, pComponentParam, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
        } 
        else if ( pComponentParam->nPortIndex == pComponentPrivate->pCompPort[G711DEC_OUTPUT_PORT]->pPortFormat->nPortIndex )
        {
            memcpy(pComponentPrivate->pCompPort[G711DEC_OUTPUT_PORT]->pPortFormat, pComponentParam, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
        }
        else
        {
            G711DEC_DPRINT("%d :: OMX_ErrorBadPortIndex from SetParameter",__LINE__);
            eError = OMX_ErrorBadPortIndex;
        }
            
        break;
    case OMX_IndexParamAudioPcm:
        {
            OMX_AUDIO_PARAM_PCMMODETYPE *pCompG711Param =
                (OMX_AUDIO_PARAM_PCMMODETYPE *)pCompParam;

            /* Input port */
            if(pCompG711Param->nPortIndex == OMX_DirInput) {
                memcpy(((G711DEC_COMPONENT_PRIVATE*)
                        pHandle->pComponentPrivate)->g711Params[G711DEC_INPUT_PORT],
                       pCompG711Param, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));

            } else if (pCompG711Param->nPortIndex == OMX_DirOutput) {
                /* Output port */
                memcpy(((G711DEC_COMPONENT_PRIVATE *)
                        pHandle->pComponentPrivate)->g711Params[G711DEC_OUTPUT_PORT],
                       pCompG711Param, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
            }
            else {
                eError = OMX_ErrorBadPortIndex;
            }
        }
        break;
    case OMX_IndexParamPortDefinition:
        if(((OMX_PARAM_PORTDEFINITIONTYPE *)(pCompParam))->nPortIndex ==
           pComponentPrivate->pPortDef[G711DEC_INPUT_PORT]->nPortIndex) {

            memcpy(pComponentPrivate->pPortDef[G711DEC_INPUT_PORT],
                   pCompParam, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));

        }
        else if(((OMX_PARAM_PORTDEFINITIONTYPE *)(pCompParam))->nPortIndex ==
                pComponentPrivate->pPortDef[G711DEC_OUTPUT_PORT]->nPortIndex) {

            memcpy(pComponentPrivate->pPortDef[G711DEC_OUTPUT_PORT],
                   pCompParam,sizeof(OMX_PARAM_PORTDEFINITIONTYPE));

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
            
    case OMX_IndexParamCompBufferSupplier:
        if(((OMX_PARAM_PORTDEFINITIONTYPE *)(pCompParam))->nPortIndex ==
           pComponentPrivate->pPortDef[G711DEC_INPUT_PORT]->nPortIndex) {
            G711DEC_DPRINT(":: SetParameter OMX_IndexParamCompBufferSupplier \n");
            sBufferSupplier.eBufferSupplier = OMX_BufferSupplyInput;
            memcpy(&sBufferSupplier, pCompParam, 
                   sizeof(OMX_PARAM_BUFFERSUPPLIERTYPE));                                  
        }
        else if(((OMX_PARAM_PORTDEFINITIONTYPE *)(pCompParam))->nPortIndex ==
                pComponentPrivate->pPortDef[G711DEC_OUTPUT_PORT]->nPortIndex) {
            G711DEC_DPRINT(":: SetParameter OMX_IndexParamCompBufferSupplier \n");
            sBufferSupplier.eBufferSupplier = OMX_BufferSupplyOutput;
            memcpy(&sBufferSupplier, pCompParam, sizeof(OMX_PARAM_BUFFERSUPPLIERTYPE));
        } 
        else {
            G711DEC_DPRINT(":: OMX_ErrorBadPortIndex from SetParameter");
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

    G711DEC_COMPONENT_PRIVATE *pComponentPrivate = NULL;

    pComponentPrivate = (G711DEC_COMPONENT_PRIVATE *)
        (((OMX_COMPONENTTYPE*)hComp)->pComponentPrivate);
    if ((pComponentPrivate != NULL) && (ComponentConfigStructure != NULL))
        memcpy(ComponentConfigStructure,pComponentPrivate,sizeof(G711DEC_COMPONENT_PRIVATE));

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
    OMX_S16* deviceString = NULL;
    TI_OMX_DATAPATH dataPath;
    OMX_COMPONENTTYPE* pHandle = (OMX_COMPONENTTYPE*)hComp;
    G711DEC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    
    int *customFlag = NULL;
    TI_OMX_DSP_DEFINITION *configData = NULL; 
    G711DEC_FTYPES *confFrameParams = NULL;
    OMX_AUDIO_CONFIG_MUTETYPE *pMuteStructure = NULL;
    OMX_AUDIO_CONFIG_VOLUMETYPE *pVolumeStructure = NULL;

    G711DEC_DPRINT("%d :: Entering SetConfig\n", __LINE__);
    
    if (pHandle == NULL) {
        G711DEC_DPRINT ("%d :: Invalid HANDLE OMX_ErrorBadParameter \n",__LINE__);
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pComponentPrivate = (G711DEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    switch (nConfigIndex) {
    case  OMX_IndexCustomG711DecHeaderInfoConfig:
        {
            G711DEC_DPRINT("%d :: SetConfig OMX_IndexCustomG711DecHeaderInfoConfig \n",__LINE__);
            configData = (TI_OMX_DSP_DEFINITION*)ComponentConfigStructure;
            if (configData == NULL) {
                eError = OMX_ErrorBadParameter;
                G711DEC_DPRINT("%d :: OMX_ErrorBadParameter from SetConfig\n",__LINE__);
                goto EXIT;
            }
            pComponentPrivate->acdnmode = configData->acousticMode;
            pComponentPrivate->dasfmode = configData->dasfMode;
              
            if (pComponentPrivate->dasfmode ){
                pComponentPrivate->pPortDef[G711DEC_OUTPUT_PORT]->bEnabled = 0;
            }
              
            pComponentPrivate->streamID = configData->streamId;
            G711DEC_DPRINT("pComponentPrivate->acdnmode = %d\n",pComponentPrivate->acdnmode);
            G711DEC_DPRINT("pComponentPrivate->dasfmode = %d\n",pComponentPrivate->dasfmode);

            break;
        }
    case OMX_IndexCustomG711DecModeAcdnConfig:
        {
            G711DEC_DPRINT("%d :: SetConfig OMX_IndexCustomG711DecModeAcdnConfig \n",__LINE__);
            customFlag = (int*)ComponentConfigStructure;
            if (customFlag == NULL) {
                eError = OMX_ErrorBadParameter;
                G711DEC_DPRINT("%d :: OMX_ErrorBadParameter from SetConfig\n",__LINE__);
                goto EXIT;
            }
            pComponentPrivate->acdnmode = *customFlag;
            G711DEC_DPRINT("pComponentPrivate->acdnmode = %d\n",pComponentPrivate->acdnmode);
            break;
        }
    case OMX_IndexCustomG711DecModeDasfConfig:
        {
            G711DEC_DPRINT("%d :: SetConfig OMX_IndexCustomG711DecModeDasfConfig \n",__LINE__);
            customFlag = (int*)ComponentConfigStructure;
            if (customFlag == NULL) {
                eError = OMX_ErrorBadParameter;
                G711DEC_DPRINT("%d :: OMX_ErrorBadParameter from SetConfig\n",__LINE__);
                goto EXIT;
            }
            pComponentPrivate->dasfmode = *customFlag;
            G711DEC_DPRINT("pComponentPrivate->dasfmode = %d\n",pComponentPrivate->dasfmode);
            if (pComponentPrivate->dasfmode ){
                pComponentPrivate->pPortDef[G711DEC_OUTPUT_PORT]->bEnabled = 0;
            }
            break;
        }
        
    case OMX_IndexCustomG711DecFrameParams:
        {
            confFrameParams = (G711DEC_FTYPES*)ComponentConfigStructure;
            pComponentPrivate->ftype = confFrameParams->FrameSizeType;
            pComponentPrivate->nmulevel = confFrameParams->NmuNLvl;
            pComponentPrivate->noiselp = confFrameParams->NoiseLp;
            pComponentPrivate->dbmnoise = confFrameParams->dBmNoise;
            pComponentPrivate->packetlostc = confFrameParams->plc;
            break;  
        }
        
    case  OMX_IndexCustomG711DecDataPath:
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
              
#ifdef DSP_RENDERING_ON
    case OMX_IndexConfigAudioMute:
        {
            pMuteStructure = (OMX_AUDIO_CONFIG_MUTETYPE *)ComponentConfigStructure;
            G711DEC_PRINT("Set Mute/Unmute for playback stream\n");
            cmd_data.hComponent = hComp;
            if(pMuteStructure->bMute == OMX_TRUE)
            {
                G711DEC_PRINT("Mute the playback stream\n");
                cmd_data.AM_Cmd = AM_CommandStreamMute;
            }
            else
            {
                G711DEC_PRINT("unMute the playback stream\n");
                cmd_data.AM_Cmd = AM_CommandStreamUnMute;
            }
            cmd_data.param1 = 0;
            cmd_data.param2 = 0;
            cmd_data.streamID = pComponentPrivate->streamID;
            if((write(pComponentPrivate->fdwrite, &cmd_data, sizeof(cmd_data)))<0)
            {
                G711DEC_DPRINT("[G711 decoder] - fail to send Mute command to audio manager\n");
                G711DEC_PRINT("No pudo escribir al Pipe del Audio Manager\n");
            }

            break;
        }
    case OMX_IndexConfigAudioVolume:
        {
            pVolumeStructure = (OMX_AUDIO_CONFIG_VOLUMETYPE *)ComponentConfigStructure;
            G711DEC_PRINT("Set volume for playback stream\n");
            cmd_data.hComponent = hComp;
            cmd_data.AM_Cmd = AM_CommandSWGain;
            cmd_data.param1 = pVolumeStructure->sVolume.nValue;
            cmd_data.param2 = 0;
            cmd_data.streamID = pComponentPrivate->streamID;

            if((write(pComponentPrivate->fdwrite, &cmd_data, sizeof(cmd_data)))<0)
            {
                G711DEC_DPRINT("[G711 decoder] - fail to send Volume command to audio manager\n");
            }

            break;
        }
#endif
    default:
        eError = OMX_ErrorUnsupportedIndex;
        break;
    }
 EXIT:
    G711DEC_DPRINT("%d :: Exiting SetConfig\n", __LINE__);
    G711DEC_DPRINT("%d :: Returning = 0x%x\n",__LINE__,eError);
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
        G711DEC_DPRINT("About to return OMX_ErrorBadParameter on line %d\n",__LINE__);
        goto EXIT;
    }

    if (pHandle && pHandle->pComponentPrivate) {
        *pState =  ((G711DEC_COMPONENT_PRIVATE*)
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
    G711DEC_COMPONENT_PRIVATE *pComponentPrivate =
        (G711DEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
                         
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef = NULL;

    ssize_t ret = 0;
    pPortDef = ((G711DEC_COMPONENT_PRIVATE*)
                pComponentPrivate)->pPortDef[G711DEC_INPUT_PORT];

    if(!pPortDef->bEnabled) {
        G711DEC_DPRINT("About to return OMX_ErrorIncorrectStateOperation Line %d\n",__LINE__);
        eError = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }

    if (pBuffer == NULL) {
        eError = OMX_ErrorBadParameter;
        G711DEC_DPRINT("About to return OMX_ErrorBadParameter on line %d\n",__LINE__);
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

    if (pBuffer->nInputPortIndex != G711DEC_INPUT_PORT) {
        eError  = OMX_ErrorBadPortIndex;
        goto EXIT;
    }

    G711DEC_DPRINT("pComponentPrivate->curState = %d\n",pComponentPrivate->curState);
    
    if(pComponentPrivate->curState != OMX_StateExecuting && 
       pComponentPrivate->curState != OMX_StatePause) {
        G711DEC_DPRINT("About to return OMX_ErrorIncorrectStateOperation Line %d\n",__LINE__);
        eError = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }

    G711DEC_DPRINT("\n------------------------------------------\n\n");
    G711DEC_DPRINT ("%d :: Component Sending Filled ip buff %p \
                             to Component Thread\n",__LINE__,pBuffer);
    G711DEC_DPRINT("\n------------------------------------------\n\n");

    pComponentPrivate->app_nBuf--;

    pComponentPrivate->pMarkData = pBuffer->pMarkData;
    pComponentPrivate->hMarkTargetComponent = pBuffer->hMarkTargetComponent;

    ret = write (pComponentPrivate->dataPipe[1], &pBuffer,
                 sizeof(OMX_BUFFERHEADERTYPE*));
                                
    if (ret == -1) {
        G711DEC_DPRINT ("%d :: Error in Writing to the Data pipe\n", __LINE__);
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
    G711DEC_COMPONENT_PRIVATE *pComponentPrivate =
        (G711DEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
                         
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef = NULL;
    G711DEC_DPRINT("\n------------------------------------------\n\n");
    G711DEC_DPRINT ("%d :: Component Sending Emptied op buff %p \
                             to Component Thread\n",__LINE__,pBuffer);
    G711DEC_DPRINT("\n------------------------------------------\n\n");

    pPortDef = ((G711DEC_COMPONENT_PRIVATE*)
                pComponentPrivate)->pPortDef[G711DEC_OUTPUT_PORT];

    if(!pPortDef->bEnabled) {
        G711DEC_DPRINT("About to return OMX_ErrorIncorrectStateOperation Line %d\n",__LINE__);
        eError = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }

    if (pBuffer == NULL) {
        eError = OMX_ErrorBadParameter;
        G711DEC_DPRINT("About to return OMX_ErrorBadParameter on line %d\n",__LINE__);
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

    if (pBuffer->nOutputPortIndex != G711DEC_OUTPUT_PORT) {
        eError  = OMX_ErrorBadPortIndex;
        goto EXIT;
    }

    G711DEC_DPRINT("pComponentPrivate->curState = %d\n",pComponentPrivate->curState);
    
    if(pComponentPrivate->curState != OMX_StateExecuting && pComponentPrivate->curState != OMX_StatePause) {
        G711DEC_DPRINT("About to return OMX_ErrorIncorrectStateOperation Line %d\n",__LINE__);
        eError = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }

    G711DEC_DPRINT("FillThisBuffer Line %d\n",__LINE__);
    
    pBuffer->nFilledLen = 0;
    
    G711DEC_DPRINT("FillThisBuffer Line %d\n",__LINE__);
    
    /*Filling the Output buffer with zero */
#ifndef UNDER_CE
    memset (pBuffer->pBuffer,0,OUTPUT_G711DEC_BUFFER_SIZE);
#else
    memset (pBuffer->pBuffer,0,pComponentPrivate->pPortDef[G711DEC_OUTPUT_PORT]->nBufferSize);
#endif
    G711DEC_DPRINT("FillThisBuffer Line %d\n",__LINE__);
    
    pComponentPrivate->app_nBuf--;
    
    G711DEC_DPRINT("%d:Decrementing app_nBuf = %d\n",__LINE__,(int)pComponentPrivate->app_nBuf);
    G711DEC_DPRINT("pComponentPrivate->pMarkBuf = 0x%x\n",(int)pComponentPrivate->pMarkBuf);
    G711DEC_DPRINT("pComponentPrivate->pMarkData = 0x%x\n",(int)pComponentPrivate->pMarkData);
    
    if(pComponentPrivate->pMarkBuf){
        G711DEC_DPRINT("FillThisBuffer Line %d\n",__LINE__);
        pBuffer->hMarkTargetComponent = pComponentPrivate->pMarkBuf->hMarkTargetComponent;
        pBuffer->pMarkData = pComponentPrivate->pMarkBuf->pMarkData;
        pComponentPrivate->pMarkBuf = NULL;
    }

    if (pComponentPrivate->pMarkData) {
        G711DEC_DPRINT("FillThisBuffer Line %d\n",__LINE__);
        pBuffer->hMarkTargetComponent = pComponentPrivate->hMarkTargetComponent;
        pBuffer->pMarkData = pComponentPrivate->pMarkData;
        pComponentPrivate->pMarkData = NULL;
    }
    
    write (pComponentPrivate->dataPipe[1], &pBuffer, sizeof (OMX_BUFFERHEADERTYPE*));
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
    G711DEC_COMPONENT_PRIVATE *pComponentPrivate =
        (G711DEC_COMPONENT_PRIVATE *)pComponent->pComponentPrivate;

    G711DEC_DPRINT ("%d ::ComponentDeInit\n",__LINE__);

#ifdef DSP_RENDERING_ON
    close(pComponentPrivate->fdwrite);
    close(pComponentPrivate->fdread);
#endif

    OMX_G711DECMEMFREE_STRUCT(pComponentPrivate->sDeviceString);

#ifdef RESOURCE_MANAGER_ENABLED
    RMProxy_NewSendCommand(pHandle, RMProxy_FreeResource, 
                           OMX_G711_Decoder_COMPONENT, 
                           0, 1234, NULL);
    if (eError != OMX_ErrorNone) {
        G711DEC_DPRINT ("%d ::Error returned from destroy ResourceManagerProxy thread\n",
                        __LINE__);
    }
    eError = RMProxy_Deinitalize();
    if (eError != OMX_ErrorNone) {
        G711DEC_DPRINT ("%d ::Error returned from destroy ResourceManagerProxy thread\n",
                        __LINE__);
    }
#endif
    G711DEC_DPRINT ("%d ::ComponentDeInit\n",__LINE__);

    G711DEC_DPRINT ("%d ::ComponentDeInit\n",__LINE__);
    pComponentPrivate->bIsStopping = 1;
    eError = G711DEC_StopComponentThread(pHandle);
    G711DEC_DPRINT ("%d ::ComponentDeInit\n",__LINE__);
    /* Wait for thread to exit so we can get the status into "error" */

    /* close the pipe handles */
    G711DEC_FreeCompResources(pHandle);
    G711DEC_DPRINT ("%d ::After G711DEC_FreeCompResources\n",__LINE__);
    OMX_G711DECMEMFREE_STRUCT(pComponentPrivate);
    G711DEC_DPRINT ("%d ::After free(pComponentPrivate)\n",__LINE__);

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
    G711DEC_DPRINT (stderr, "Inside the ComponentTunnelRequest\n");
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
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef = NULL;
    G711DEC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_BUFFERHEADERTYPE *pBufferHeader = NULL;

    pComponentPrivate = (G711DEC_COMPONENT_PRIVATE *)
        (((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);

    pPortDef = ((G711DEC_COMPONENT_PRIVATE*)pComponentPrivate)->pPortDef[nPortIndex];
    
    G711DEC_DPRINT ("%d :: pPortDef = 0x%x\n", __LINE__,(unsigned int)pPortDef);
    G711DEC_DPRINT("%d :: pPortDef->bEnabled = %d\n", __LINE__,pPortDef->bEnabled);
    G711DEC_DPRINT("pPortDef->bEnabled = %d\n", pPortDef->bEnabled);
    
    if(!pPortDef->bEnabled){
        G711DEC_DPRINT ("%d :: BLOCK!! AlloBuf_threshold\n", __LINE__);
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
    
    G711D_OMX_MALLOC(pBufferHeader, OMX_BUFFERHEADERTYPE);
    G711D_OMX_MALLOC_SIZE(pBufferHeader->pBuffer, (nSizeBytes + EXTRA_BUFFBYTES), OMX_U8 );

#ifdef UNDER_CE
    memset(pBufferHeader->pBuffer, 0, (nSizeBytes + EXTRA_BUFFBYTES));
#endif

    pBufferHeader->pBuffer += CACHE_ALIGNMENT;

    if (nPortIndex == G711DEC_INPUT_PORT) {
        pBufferHeader->nInputPortIndex = nPortIndex;
        pBufferHeader->nOutputPortIndex = -1;
        pComponentPrivate->pInputBufferList->pBufHdr[pComponentPrivate->pInputBufferList->numBuffers] = pBufferHeader;
        pComponentPrivate->pInputBufferList->bBufferPending[pComponentPrivate->pInputBufferList->numBuffers] = 0;
        G711DEC_DPRINT("pComponentPrivate->pInputBufferList->pBufHdr[%d] = %p \n",(int)pComponentPrivate->pInputBufferList->pBufHdr[pComponentPrivate->pInputBufferList->numBuffers]);
        pComponentPrivate->pInputBufferList->bufferOwner[pComponentPrivate->pInputBufferList->numBuffers++] = 1;
        G711DEC_DPRINT("Allocate Buffer Line %d\n",__LINE__);
        G711DEC_DPRINT("pComponentPrivate->pInputBufferList->numBuffers = %d\n",(int)pComponentPrivate->pInputBufferList->numBuffers);
        G711DEC_DPRINT("pPortDef->nBufferCountMin = %d\n",(int)pPortDef->nBufferCountMin);
        if (pComponentPrivate->pInputBufferList->numBuffers == pPortDef->nBufferCountActual) {
            G711DEC_DPRINT("Setting pPortDef->bPopulated = OMX_TRUE for input port\n");
            pPortDef->bPopulated = OMX_TRUE;
        }
    }
    else if (nPortIndex == G711DEC_OUTPUT_PORT) {
        pBufferHeader->nInputPortIndex = -1;
        pBufferHeader->nOutputPortIndex = nPortIndex;
        pComponentPrivate->pOutputBufferList->pBufHdr[pComponentPrivate->pOutputBufferList->numBuffers] = pBufferHeader;
        pComponentPrivate->pOutputBufferList->bBufferPending[pComponentPrivate->pOutputBufferList->numBuffers] = 0;
        G711DEC_DPRINT("pComponentPrivate->pOutputBufferList->pBufHdr[%d] = %p \n",(int)pComponentPrivate->pOutputBufferList->pBufHdr[pComponentPrivate->pOutputBufferList->numBuffers]);
        pComponentPrivate->pOutputBufferList->bufferOwner[pComponentPrivate->pOutputBufferList->numBuffers++] = 1;
        if (pComponentPrivate->pOutputBufferList->numBuffers == pPortDef->nBufferCountActual) {
            G711DEC_DPRINT("Setting pPortDef->bPopulated = OMX_TRUE for input port\n");
            pPortDef->bPopulated = OMX_TRUE;
        }
    }
    else {
        eError = OMX_ErrorBadPortIndex;
        goto EXIT;
    }
    if((pComponentPrivate->pPortDef[G711DEC_OUTPUT_PORT]->bPopulated == 
        pComponentPrivate->pPortDef[G711DEC_OUTPUT_PORT]->bEnabled) &&
       (pComponentPrivate->pPortDef[G711DEC_INPUT_PORT]->bPopulated == 
        pComponentPrivate->pPortDef[G711DEC_INPUT_PORT]->bEnabled) &&
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
    pBufferHeader->nVersion.s.nVersionMajor = G711DEC_MAJOR_VER;
    pBufferHeader->nVersion.s.nVersionMinor = G711DEC_MINOR_VER;
    pComponentPrivate->nVersion = pBufferHeader->nVersion.nVersion;


    pBufferHeader->nSize = sizeof(OMX_BUFFERHEADERTYPE);
    *pBuffer = pBufferHeader;

    if (pComponentPrivate->bEnableCommandPending && pPortDef->bPopulated) 
    {
        SendCommand(pComponentPrivate->pHandle,
                    OMX_CommandPortEnable,
                    pComponentPrivate->bEnableCommandParam,
                    NULL);
    }
    
 EXIT:
    G711DEC_DPRINT("AllocateBuffer returning %d\n",eError);
    return eError;
}

/*------------------------------------------------------------------*/
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
    G711DEC_COMPONENT_PRIVATE * pComponentPrivate = NULL;
    OMX_BUFFERHEADERTYPE* buff = NULL;
    OMX_U8* tempBuff = NULL;
    OMX_S16 i = 0;
    OMX_S16 inputIndex = -1;
    OMX_S16 outputIndex = -1;
    OMX_COMPONENTTYPE *pHandle = NULL;

    pComponentPrivate = (G711DEC_COMPONENT_PRIVATE *)
        (((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);

    pHandle = (OMX_COMPONENTTYPE *) pComponentPrivate->pHandle;
    for (i=0; i < MAX_NUM_OF_BUFS; i++) {
        buff = pComponentPrivate->pInputBufferList->pBufHdr[i];
        if (buff == pBuffer) {
            G711DEC_DPRINT("Found matching input buffer\n");
            G711DEC_DPRINT("buff = %p\n",buff);
            G711DEC_DPRINT("pBuffer = %p\n",pBuffer);
            inputIndex = i;
            break;
        }
        else {
            G711DEC_DPRINT("This is not a match\n");
            G711DEC_DPRINT("buff = %p\n",buff);
            G711DEC_DPRINT("pBuffer = %p\n",pBuffer);
        }
    }

    for (i=0; i < MAX_NUM_OF_BUFS; i++) {
        buff = pComponentPrivate->pOutputBufferList->pBufHdr[i];
        if (buff == pBuffer) {
            G711DEC_DPRINT("Found matching output buffer\n");
            G711DEC_DPRINT("buff = %p\n",buff);
            G711DEC_DPRINT("pBuffer = %p\n",pBuffer);
            outputIndex = i;
            break;
        }
        else {
            G711DEC_DPRINT("This is not a match\n");
            G711DEC_DPRINT("buff = %p\n",buff);
            G711DEC_DPRINT("pBuffer = %p\n",pBuffer);
        }
    }


    if (inputIndex != -1) {
        if (pComponentPrivate->pInputBufferList->bufferOwner[inputIndex] == 1) {
            tempBuff = pComponentPrivate->pInputBufferList->pBufHdr[inputIndex]->pBuffer;
            tempBuff -= CACHE_ALIGNMENT;
            OMX_G711DECMEMFREE_STRUCT(tempBuff);
        }
        OMX_G711DECMEMFREE_STRUCT(pComponentPrivate->pInputBufferList->pBufHdr[inputIndex]);
        pComponentPrivate->pInputBufferList->numBuffers--;
        G711DEC_DPRINT("INPUT :: numBuffers %d, nBufferCountActual %d\n",
                       (int)pComponentPrivate->pInputBufferList->numBuffers,
                       (int)pComponentPrivate->pPortDef[G711DEC_INPUT_PORT]->nBufferCountActual);
        if (pComponentPrivate->pInputBufferList->numBuffers <
            pComponentPrivate->pPortDef[G711DEC_INPUT_PORT]->nBufferCountActual) {
            G711DEC_DPRINT("setting input port populated to OMX_FALSE\n");
            pComponentPrivate->pPortDef[G711DEC_INPUT_PORT]->bPopulated = OMX_FALSE;
        }
        if(pComponentPrivate->pPortDef[G711DEC_INPUT_PORT]->bEnabled &&
           pComponentPrivate->bLoadedCommandPending == OMX_FALSE &&
           (pComponentPrivate->curState == OMX_StateIdle ||
            pComponentPrivate->curState == OMX_StateExecuting ||
            pComponentPrivate->curState == OMX_StatePause)) 
        {
            pComponentPrivate->cbInfo.EventHandler(pHandle, 
                                                   pHandle->pApplicationPrivate,
                                                   OMX_EventError, 
                                                   OMX_ErrorPortUnpopulated,
                                                   nPortIndex, NULL);
        }
    }
    else if (outputIndex != -1) {
        if (pComponentPrivate->pOutputBufferList->bufferOwner[outputIndex] == 1) {
            tempBuff = pComponentPrivate->pOutputBufferList->pBufHdr[outputIndex]->pBuffer;
            tempBuff -= CACHE_ALIGNMENT;
            OMX_G711DECMEMFREE_STRUCT(tempBuff);
        }
            
        OMX_G711DECMEMFREE_STRUCT(pComponentPrivate->pOutputBufferList->pBufHdr[outputIndex]);
        pComponentPrivate->pOutputBufferList->numBuffers--;
        G711DEC_DPRINT("OUTPUT :: numBuffers %d, nBufferCountActual %d\n",
                       (int)pComponentPrivate->pInputBufferList->numBuffers,
                       (int)pComponentPrivate->pPortDef[G711DEC_INPUT_PORT]->nBufferCountActual);
            
        if (pComponentPrivate->pOutputBufferList->numBuffers <
            pComponentPrivate->pPortDef[G711DEC_OUTPUT_PORT]->nBufferCountActual) {
            G711DEC_DPRINT("setting output port populated to OMX_FALSE\n");
            pComponentPrivate->pPortDef[G711DEC_OUTPUT_PORT]->bPopulated = OMX_FALSE;
        }
            
        if(pComponentPrivate->pPortDef[G711DEC_OUTPUT_PORT]->bEnabled &&
           pComponentPrivate->bLoadedCommandPending == OMX_FALSE &&
           (pComponentPrivate->curState == OMX_StateIdle ||
            pComponentPrivate->curState == OMX_StateExecuting ||
            pComponentPrivate->curState == OMX_StatePause)) 
        {
            pComponentPrivate->cbInfo.EventHandler(
                                                   pHandle, pHandle->pApplicationPrivate,
                                                   OMX_EventError, OMX_ErrorPortUnpopulated,nPortIndex, NULL);
        }
    }
    else {
        G711DEC_DPRINT("%d::Returning OMX_ErrorBadParameter\n",__LINE__);
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
        
    G711DEC_DPRINT("pComponentPrivate->bDisableCommandPending = %d\n",(int)pComponentPrivate->bDisableCommandPending);
    if (pComponentPrivate->bDisableCommandPending) {
        if (pComponentPrivate->pInputBufferList->numBuffers + pComponentPrivate->pOutputBufferList->numBuffers == 0) {
            SendCommand (pComponentPrivate->pHandle,OMX_CommandPortDisable,pComponentPrivate->bDisableCommandParam,NULL);
        }
    }
    G711DEC_DPRINT ("%d :: Exiting FreeBuffer\n", __LINE__);
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
    G711DEC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_BUFFERHEADERTYPE *pBufferHeader = NULL;

    pComponentPrivate = (G711DEC_COMPONENT_PRIVATE *)
        (((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);

    pPortDef = ((G711DEC_COMPONENT_PRIVATE*)
                pComponentPrivate)->pPortDef[nPortIndex];
                        
    G711DEC_DPRINT("pPortDef->bPopulated = %d\n",pPortDef->bPopulated);

    if(!pPortDef->bEnabled) {
        G711DEC_DPRINT ("%d :: In AllocateBuffer\n", __LINE__);
        eError = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }

    if(nSizeBytes != pPortDef->nBufferSize || pPortDef->bPopulated) {
        G711DEC_DPRINT ("%d :: In AllocateBuffer\n", __LINE__);
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }

    G711D_OMX_MALLOC(pBufferHeader, OMX_BUFFERHEADERTYPE);
    
    if (nPortIndex == G711DEC_OUTPUT_PORT) {
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

    if((pComponentPrivate->pPortDef[G711DEC_OUTPUT_PORT]->bPopulated == pComponentPrivate->pPortDef[G711DEC_OUTPUT_PORT]->bEnabled)&&
       (pComponentPrivate->pPortDef[G711DEC_INPUT_PORT]->bPopulated == pComponentPrivate->pPortDef[G711DEC_INPUT_PORT]->bEnabled) &&
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
    
    pBufferHeader->pAppPrivate              =   pAppPrivate;
    pBufferHeader->pPlatformPrivate         =   pComponentPrivate;
    pBufferHeader->nAllocLen                =   nSizeBytes;
    pBufferHeader->nVersion.s.nVersionMajor =   G711DEC_MAJOR_VER;
    pBufferHeader->nVersion.s.nVersionMinor =   G711DEC_MINOR_VER;
    pComponentPrivate->nVersion             =   pBufferHeader->nVersion.nVersion;
    pBufferHeader->pBuffer                  =   pBuffer;
    pBufferHeader->nSize                    =   sizeof(OMX_BUFFERHEADERTYPE);
    *ppBufferHdr                            =   pBufferHeader;
    
    G711DEC_DPRINT("pBufferHeader = %p\n",pBufferHeader);

    if (pComponentPrivate->bEnableCommandPending && pPortDef->bPopulated) 
    {
        SendCommand(pComponentPrivate->pHandle, OMX_CommandPortEnable,
                    pComponentPrivate->bEnableCommandParam, NULL);
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
static OMX_ERRORTYPE GetExtensionIndex( OMX_IN  OMX_HANDLETYPE hComponent,
                                        OMX_IN  OMX_STRING cParameterName,
                                        OMX_OUT OMX_INDEXTYPE *pIndexType)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    if(!(strcmp(cParameterName,"OMX.TI.index.config.g711headerinfo"))) {
        *pIndexType = OMX_IndexCustomG711DecHeaderInfoConfig;
    }
    else if(!(strcmp(cParameterName,"OMX.TI.index.config.g711dec.datapath"))) 
    {
        *pIndexType = OMX_IndexCustomG711DecDataPath;
    } 
    else if(!strcmp(cParameterName,"OMX.TI.index.config.g711dec.frameparams"))
    {
        *pIndexType = OMX_IndexCustomG711DecFrameParams;
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
static OMX_ERRORTYPE ComponentRoleEnum( OMX_IN OMX_HANDLETYPE hComponent,
                                        OMX_OUT OMX_U8 *cRole,
                                        OMX_IN OMX_U32 nIndex)
{
    /*G711DEC_COMPONENT_PRIVATE *pComponentPrivate;*/
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    /* This is a non standard component and does not support roles */
    eError = OMX_ErrorNotImplemented;
    
    return eError;
}

#ifdef G711DEC_MEMDEBUG
void * mymalloc(int line, char *s, int size)
{
    void *p;    
    int e=0;
    p = malloc(size);
   
    if(p==NULL){
        G711DEC_PRINT("Memory not available\n");
        exit(1);
    }
    else{
        while((lines[e]!=0)&& (e<500) ){
            e++;
        }
        arr[e]=p;
        lines[e]=line;
        bytes[e]=size;
        strcpy(file[e],s);
        G711DEC_PRINT("Allocating %d bytes on address %p, line %d file %s pos %d\n", size, p, line, s, e);
        return p;
    }
}

void myfree(void *dp, int line, char *s){
    int q = 0;
    for(q=0;q<500;q++){
        if(arr[q]==dp){
            G711DEC_PRINT("Deleting %d bytes on address %p, line %d file %s\n", bytes[q],dp, line, s);
            free(dp);
            dp = NULL;
            lines[q]=0;
            strcpy(file[q],"");
            break;
        }            
    }    
    if(500==q)
        G711DEC_PRINT("\n\nPointer not found. Line:%d    File%s!!\n\n",line, s);
}
#endif
