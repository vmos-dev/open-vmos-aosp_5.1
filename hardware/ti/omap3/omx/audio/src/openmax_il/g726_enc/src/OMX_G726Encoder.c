
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
* @file OMX_G726Encoder.c
*
* This file implements OpenMAX (TM) 1.0 Specific APIs and its functionality
* that is fully compliant with the Khronos OpenMAX (TM) 1.0 Specification
*
* @path  $(CSLPATH)\OMAPSW_MPU\linux\audio\src\openmax_il\g726_enc\src
*
* @rev  1.0
*/
/* ----------------------------------------------------------------------------
*!
*! Revision History
*! ===================================
*! Gyancarlo Garcia: Initial Verision
*! 05-Oct-2007
*!
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

#include "OMX_G726Enc_Utils.h"

/****************************************************************
*  EXTERNAL REFERENCES NOTE : only use if not found in header file
****************************************************************/
/*--------data declarations -----------------------------------*/

/*--------function prototypes ---------------------------------*/

/****************************************************************
*  PUBLIC DECLARATIONS Defined here, used elsewhere
****************************************************************/
/*--------data declarations --------------------------------*/

/*--------function prototypes ---------------------------------*/

/****************************************************************
*  PRIVATE DECLARATIONS Defined here, used only here
****************************************************************/
/*--------data declarations -----------------------------------*/

/*--------function prototypes ---------------------------------*/
#ifdef G726ENC_DEBUGMEM
extern void * DebugMalloc(int line, char *s, int size);
extern int DebugFree(void *dp, int line, char *s);

#define SafeMalloc(x) DebugMalloc(__LINE__,__FILE__,x)
#define SafeFree(z) DebugFree(z,__LINE__,__FILE__)
#else
#define SafeMalloc(x) calloc(1,x)
#define SafeFree(z) free(z)
#endif


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
    G726ENC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
	OMX_AUDIO_PARAM_G726TYPE *G726_op = NULL;
	OMX_AUDIO_PARAM_G726TYPE *G726_ip = NULL;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE*) hComp;
    G726ENC_PORT_TYPE *pCompPort = NULL;
    OMX_AUDIO_PARAM_PORTFORMATTYPE *pPortFormat = NULL;
	int i = 0;

	G726ENC_DPRINT("%d :: Entering OMX_ComponentInit\n", __LINE__);
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
	OMX_NBMALLOC_STRUCT(pHandle->pComponentPrivate, G726ENC_COMPONENT_PRIVATE);
	memset(pHandle->pComponentPrivate, 0x0, sizeof(G726ENC_COMPONENT_PRIVATE));

    ((G726ENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate)->pHandle = pHandle;
	pComponentPrivate = pHandle->pComponentPrivate;

    OMX_NBMALLOC_STRUCT(pCompPort, G726ENC_PORT_TYPE);
	pComponentPrivate->pCompPort[G726ENC_INPUT_PORT] = pCompPort;

    OMX_NBMALLOC_STRUCT(pCompPort, G726ENC_PORT_TYPE);
	pComponentPrivate->pCompPort[G726ENC_OUTPUT_PORT] = pCompPort;

	OMX_NBMALLOC_STRUCT(pComponentPrivate->sPortParam, OMX_PORT_PARAM_TYPE);
    OMX_NBCONF_INIT_STRUCT(pComponentPrivate->sPortParam, OMX_PORT_PARAM_TYPE);

    /* Initialize sPortParam data structures to default values */
  	pComponentPrivate->sPortParam->nPorts = 0x2;
	pComponentPrivate->sPortParam->nStartPortNumber = 0x0;

	/* Malloc and Set pPriorityMgmt defaults */
    OMX_NBMALLOC_STRUCT(pComponentPrivate->sPriorityMgmt, OMX_PRIORITYMGMTTYPE);
	OMX_NBCONF_INIT_STRUCT(pComponentPrivate->sPriorityMgmt, OMX_PRIORITYMGMTTYPE);

	/* Initialize sPriorityMgmt data structures to default values */
	pComponentPrivate->sPriorityMgmt->nGroupPriority = -1;
	pComponentPrivate->sPriorityMgmt->nGroupID = -1;

	OMX_NBMALLOC_STRUCT(G726_op, OMX_AUDIO_PARAM_G726TYPE);
    OMX_NBCONF_INIT_STRUCT(G726_op, OMX_AUDIO_PARAM_G726TYPE);
	pComponentPrivate->G726Params[G726ENC_OUTPUT_PORT] = G726_op;
    G726_op->nPortIndex = G726ENC_OUTPUT_PORT;

	OMX_NBMALLOC_STRUCT(G726_ip, OMX_AUDIO_PARAM_G726TYPE);
    OMX_NBCONF_INIT_STRUCT(G726_ip, OMX_AUDIO_PARAM_G726TYPE);
	pComponentPrivate->G726Params[G726ENC_INPUT_PORT] = G726_ip;
    G726_ip->nPortIndex = G726ENC_INPUT_PORT;

    /* malloc and initialize number of input buffers */
    OMX_NBMALLOC_STRUCT(pComponentPrivate->pInputBufferList, G726ENC_BUFFERLIST);
	pComponentPrivate->pInputBufferList->numBuffers = 0;

    /* malloc and initialize number of output buffers */
    OMX_NBMALLOC_STRUCT(pComponentPrivate->pOutputBufferList, G726ENC_BUFFERLIST);
	pComponentPrivate->pOutputBufferList->numBuffers = 0;

	for (i=0; i < G726ENC_MAX_NUM_OF_BUFS; i++) {
		pComponentPrivate->pOutputBufferList->pBufHdr[i] = NULL;
		pComponentPrivate->pInputBufferList->pBufHdr[i] = NULL;
		pComponentPrivate->arrTickCount[i] = 0;
		pComponentPrivate->arrTimestamp[i] = 0;		
	}
	pComponentPrivate->IpBufindex = 0;
	pComponentPrivate->OpBufindex = 0;	

    /* Set input port defaults */
	OMX_NBMALLOC_STRUCT(pPortDef_ip, OMX_PARAM_PORTDEFINITIONTYPE);
	OMX_NBCONF_INIT_STRUCT(pPortDef_ip, OMX_PARAM_PORTDEFINITIONTYPE);
    pComponentPrivate->pPortDef[G726ENC_INPUT_PORT] = pPortDef_ip;

    pPortDef_ip->nPortIndex                         = G726ENC_INPUT_PORT;
    pPortDef_ip->eDir                               = OMX_DirInput;
    pPortDef_ip->nBufferCountActual                 = G726ENC_NUM_INPUT_BUFFERS;
    pPortDef_ip->nBufferCountMin                    = G726ENC_NUM_INPUT_BUFFERS;
    pPortDef_ip->nBufferSize                        = G726ENC_INPUT_BUFFER_SIZE_DASF;
    pPortDef_ip->bEnabled                           = OMX_TRUE;
    pPortDef_ip->bPopulated                         = OMX_FALSE;
    pPortDef_ip->eDomain                            = OMX_PortDomainAudio;
	pPortDef_ip->format.audio.eEncoding             = OMX_AUDIO_CodingPCM;
	pPortDef_ip->format.audio.pNativeRender         = NULL;
	pPortDef_ip->format.audio.bFlagErrorConcealment = OMX_FALSE;

 	/* Set output port defaults */
	OMX_NBMALLOC_STRUCT(pPortDef_op, OMX_PARAM_PORTDEFINITIONTYPE);
	OMX_NBCONF_INIT_STRUCT(pPortDef_op, OMX_PARAM_PORTDEFINITIONTYPE);
    pComponentPrivate->pPortDef[G726ENC_OUTPUT_PORT] = pPortDef_op;

    pPortDef_op->nPortIndex                         = G726ENC_OUTPUT_PORT;
    pPortDef_op->eDir                               = OMX_DirOutput;
    pPortDef_op->nBufferCountMin                    = G726ENC_NUM_OUTPUT_BUFFERS;
    pPortDef_op->nBufferCountActual                 = G726ENC_NUM_OUTPUT_BUFFERS;
    pPortDef_op->nBufferSize                        = G726ENC_OUTPUT_BUFFER_SIZE;
    pPortDef_op->bEnabled                           = OMX_TRUE;
    pPortDef_op->bPopulated                         = OMX_FALSE;
    pPortDef_op->eDomain                            = OMX_PortDomainAudio;
	pPortDef_op->format.audio.eEncoding             = OMX_AUDIO_CodingG726;
	pPortDef_op->format.audio.pNativeRender         = NULL;
	pPortDef_op->format.audio.bFlagErrorConcealment = OMX_FALSE;

    OMX_NBMALLOC_STRUCT(pComponentPrivate->pCompPort[G726ENC_INPUT_PORT]->pPortFormat, OMX_AUDIO_PARAM_PORTFORMATTYPE);
    OMX_NBCONF_INIT_STRUCT(pComponentPrivate->pCompPort[G726ENC_INPUT_PORT]->pPortFormat, OMX_AUDIO_PARAM_PORTFORMATTYPE);
    /* Set input port format defaults */
    pPortFormat = pComponentPrivate->pCompPort[G726ENC_INPUT_PORT]->pPortFormat;
    OMX_NBCONF_INIT_STRUCT(pPortFormat, OMX_AUDIO_PARAM_PORTFORMATTYPE);
    pPortFormat->nPortIndex         = G726ENC_INPUT_PORT;
    pPortFormat->nIndex             = OMX_IndexParamAudioPcm;
    pPortFormat->eEncoding 			= OMX_AUDIO_CodingPCM;

    OMX_NBMALLOC_STRUCT(pComponentPrivate->pCompPort[G726ENC_OUTPUT_PORT]->pPortFormat, OMX_AUDIO_PARAM_PORTFORMATTYPE);
    OMX_NBCONF_INIT_STRUCT(pComponentPrivate->pCompPort[G726ENC_OUTPUT_PORT]->pPortFormat, OMX_AUDIO_PARAM_PORTFORMATTYPE);
    /* Set output port format defaults */
    pPortFormat = pComponentPrivate->pCompPort[G726ENC_OUTPUT_PORT]->pPortFormat;
    OMX_NBCONF_INIT_STRUCT(pPortFormat, OMX_AUDIO_PARAM_PORTFORMATTYPE);
    pPortFormat->nPortIndex         = G726ENC_OUTPUT_PORT;
    pPortFormat->nIndex             = OMX_IndexParamAudioPcm;
    pPortFormat->eEncoding 			= OMX_AUDIO_CodingG726;

    G726ENC_DPRINT("%d :: Setting dasf & acdn and MultiFrame modes to 0\n",__LINE__);
    pComponentPrivate->dasfMode = 0;
    pComponentPrivate->acdnMode = 0;
    pComponentPrivate->rtpMode = 0;
    pComponentPrivate->bPortDefsAllocated = 0;
    pComponentPrivate->bCompThreadStarted = 0;
    pComponentPrivate->pParams = NULL;

	pComponentPrivate->bInitParamsInitialized = 0;
	pComponentPrivate->pMarkBuf = NULL;
	pComponentPrivate->pMarkData = NULL;
	pComponentPrivate->nEmptyBufferDoneCount = 0;
	pComponentPrivate->nEmptyThisBufferCount = 0;
	pComponentPrivate->nFillBufferDoneCount = 0;
	pComponentPrivate->nFillThisBufferCount = 0;
	pComponentPrivate->strmAttr = NULL;
	pComponentPrivate->bDisableCommandParam = 0;


	for (i=0; i < G726ENC_MAX_NUM_OF_BUFS; i++) {
		pComponentPrivate->pInputBufHdrPending[i] = NULL;
		pComponentPrivate->pOutputBufHdrPending[i] = NULL;
	}

	pComponentPrivate->nNumInputBufPending = 0;
	pComponentPrivate->nNumOutputBufPending = 0;
	pComponentPrivate->bDisableCommandPending = 0;
    pComponentPrivate->bNoIdleOnStop= OMX_FALSE;
    pComponentPrivate->nOutStandingFillDones = 0;
    pComponentPrivate->bDspStoppedWhileExecuting = OMX_FALSE;

    pComponentPrivate->G726FrameSize[0] = 24;
    pComponentPrivate->G726FrameSize[1] = 20;
    pComponentPrivate->G726FrameSize[2] = 4;
    pComponentPrivate->G726FrameSize[3] = 1;
    pComponentPrivate->lastOutBufArrived = NULL;
    pComponentPrivate->LastBufSent = 0;
    pComponentPrivate->bPreempted = OMX_FALSE;    

    strcpy((char*)pComponentPrivate->componentRole.cRole, "audio_encoder.g726"); 
    pComponentPrivate->sDeviceString = SafeMalloc(100*sizeof(OMX_STRING));
    if (pComponentPrivate->sDeviceString == NULL) {
	G726ENC_DPRINT("%d :: OMX_ErrorInsufficientResources", __LINE__);
	eError = OMX_ErrorInsufficientResources;
	goto EXIT;
    }
    /* Initialize device string to the default value */
    strcpy((char*)pComponentPrivate->sDeviceString,":srcul/codec\0");
     
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
#endif

    eError = G726ENC_StartComponentThread(pHandle);
    G726ENC_DPRINT("%d :: OMX_ComponentInit\n", __LINE__);
    if (eError != OMX_ErrorNone) {
   	  G726ENC_DPRINT("%d :: Error returned from the Component\n",__LINE__);
   	  goto EXIT;
    }

#ifdef RESOURCE_MANAGER_ENABLED
	eError = RMProxy_NewInitalize();
    G726ENC_DPRINT("%d :: OMX_ComponentInit\n", __LINE__);
	if (eError != OMX_ErrorNone) {
		G726ENC_DPRINT("%d :: Error returned from loading ResourceManagerProxy thread\n",__LINE__);
		goto EXIT;
	}
#endif

EXIT:
	G726ENC_DPRINT("%d :: Exiting OMX_ComponentInit\n", __LINE__);
	G726ENC_DPRINT("%d :: Returning = 0x%x\n",__LINE__,eError);
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

    G726ENC_COMPONENT_PRIVATE *pComponentPrivate =
                    (G726ENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
	G726ENC_DPRINT("%d :: Entering SetCallbacks\n", __LINE__);
    if (pCallBacks == NULL) {
        eError = OMX_ErrorBadParameter;
        G726ENC_DPRINT("%d :: Received the empty callbacks from the application\n",__LINE__);
        goto EXIT;
    }

    /*Copy the callbacks of the application to the component private*/
    memcpy (&(pComponentPrivate->cbInfo), pCallBacks, sizeof(OMX_CALLBACKTYPE));

    /*copy the application private data to component memory */
    pHandle->pApplicationPrivate = pAppData;

    pComponentPrivate->curState = OMX_StateLoaded;

EXIT:
	G726ENC_DPRINT("%d :: Exiting SetCallbacks\n", __LINE__);
	G726ENC_DPRINT("%d :: Returning = 0x%x\n",__LINE__,eError);
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
    G726ENC_COMPONENT_PRIVATE *pComponentPrivate = (G726ENC_COMPONENT_PRIVATE *) pHandle->pComponentPrivate;
	G726ENC_DPRINT("%d :: Entering GetComponentVersion\n", __LINE__);
	/* Copy component version structure */
	if(pComponentVersion != NULL && pComponentName != NULL) {
		strcpy(pComponentName, pComponentPrivate->cComponentName);
		memcpy(pComponentVersion, &(pComponentPrivate->ComponentVersion.s), sizeof(pComponentPrivate->ComponentVersion.s));
	}
	else {
		G726ENC_DPRINT("%d :: OMX_ErrorBadParameter from GetComponentVersion",__LINE__);
		eError = OMX_ErrorBadParameter;
	}

    G726ENC_DPRINT("%d :: Exiting GetComponentVersion\n", __LINE__);
	G726ENC_DPRINT("%d :: Returning = 0x%x\n",__LINE__,eError);
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
    G726ENC_COMPONENT_PRIVATE *pCompPrivate =
             (G726ENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    int nRet = 0;
	G726ENC_DPRINT("%d :: Entering SendCommand()\n", __LINE__);
    if(pCompPrivate->curState == OMX_StateInvalid) {
	    eError = OMX_ErrorInvalidState;
	    G726ENC_DPRINT("%d :: Error OMX_ErrorInvalidState Sent to App\n",__LINE__);
        goto EXIT;
    }

    switch(Cmd) {
        case OMX_CommandStateSet:
   			G726ENC_DPRINT("%d :: OMX_CommandStateSet SendCommand\n",__LINE__);
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
   					G726ENC_DPRINT("%d :: OMX_CommandStateSet SendCommand\n",__LINE__);
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
   			G726ENC_DPRINT("%d :: OMX_CommandFlush SendCommand\n",__LINE__);
			if(nParam > 1 && nParam != -1) {
				eError = OMX_ErrorBadPortIndex;
				G726ENC_DPRINT("%d :: OMX_ErrorBadPortIndex from SendCommand",__LINE__);
				goto EXIT;
			}
	        break;
        case OMX_CommandPortDisable:
   			G726ENC_DPRINT("%d :: OMX_CommandPortDisable SendCommand\n",__LINE__);
            break;
        case OMX_CommandPortEnable:
   			G726ENC_DPRINT("%d :: OMX_CommandPortEnable SendCommand\n",__LINE__);
            break;
        case OMX_CommandMarkBuffer:
   			G726ENC_DPRINT("%d :: OMX_CommandMarkBuffer SendCommand\n",__LINE__);
            if (nParam > 0) {
                eError = OMX_ErrorBadPortIndex;
       		    G726ENC_DPRINT("%d :: OMX_ErrorBadPortIndex from SendCommand",__LINE__);
                goto EXIT;
            }
            break;
        default:
            G726ENC_DPRINT("%d :: Command Received Default eError\n",__LINE__);
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
       G726ENC_DPRINT("%d :: OMX_ErrorInsufficientResources from SendCommand",__LINE__);
	   goto EXIT;
	}

    if (Cmd == OMX_CommandMarkBuffer) {
        nRet = write(pCompPrivate->cmdDataPipe[1], &pCmdData,
                            sizeof(OMX_PTR));
    } else {
        nRet = write(pCompPrivate->cmdDataPipe[1], &nParam,
                            sizeof(OMX_U32));
    }
    if (nRet == -1) {
		G726ENC_DPRINT("%d :: OMX_ErrorInsufficientResources from SendCommand",__LINE__);
        eError = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

EXIT:
 	G726ENC_DPRINT("%d :: Exiting SendCommand()\n", __LINE__);
	G726ENC_DPRINT("%d :: Returning = 0x%x\n",__LINE__,eError);
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
    G726ENC_COMPONENT_PRIVATE  *pComponentPrivate = NULL;
	OMX_PARAM_PORTDEFINITIONTYPE *pParameterStructure = NULL;

    pComponentPrivate = (G726ENC_COMPONENT_PRIVATE *)(((OMX_COMPONENTTYPE*)hComp)->pComponentPrivate);
	pParameterStructure = (OMX_PARAM_PORTDEFINITIONTYPE*)ComponentParameterStructure;
	G726ENC_DPRINT("%d :: Entering the GetParameter\n",__LINE__);
	if (pParameterStructure == NULL) {
		eError = OMX_ErrorBadParameter;
		G726ENC_DPRINT("%d :: OMX_ErrorBadPortIndex from GetParameter",__LINE__);
		goto EXIT;
	}

    if(pComponentPrivate->curState == OMX_StateInvalid) {
		eError = OMX_ErrorIncorrectStateOperation;
		G726ENC_DPRINT("%d :: OMX_ErrorIncorrectStateOperation from GetParameter",__LINE__);
		goto EXIT;
    }

    switch(nParamIndex){
        case OMX_IndexParamAudioInit:
	    if (pComponentPrivate->sPortParam == NULL) {
	        eError = OMX_ErrorBadParameter;
                break;
	    }
            G726ENC_DPRINT("%d :: GetParameter OMX_IndexParamAudioInit \n",__LINE__);
            memcpy(ComponentParameterStructure, pComponentPrivate->sPortParam, sizeof(OMX_PORT_PARAM_TYPE));
            break;

        case OMX_IndexParamPortDefinition:
            G726ENC_DPRINT("%d :: GetParameter OMX_IndexParamPortDefinition \n",__LINE__);
            if(((OMX_PARAM_PORTDEFINITIONTYPE *)(ComponentParameterStructure))->nPortIndex ==
                pComponentPrivate->pPortDef[G726ENC_INPUT_PORT]->nPortIndex) {
                    memcpy(ComponentParameterStructure, pComponentPrivate->pPortDef[G726ENC_INPUT_PORT], sizeof(OMX_PARAM_PORTDEFINITIONTYPE)); 
            }
            else if(((OMX_PARAM_PORTDEFINITIONTYPE *)(ComponentParameterStructure))->nPortIndex ==
									  pComponentPrivate->pPortDef[G726ENC_OUTPUT_PORT]->nPortIndex) {
                memcpy(ComponentParameterStructure, pComponentPrivate->pPortDef[G726ENC_OUTPUT_PORT], sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
            } 
            else {
                G726ENC_DPRINT("%d :: OMX_ErrorBadPortIndex from GetParameter \n",__LINE__);
                eError = OMX_ErrorBadPortIndex;
            }
			break;

        case OMX_IndexParamAudioPortFormat:
            G726ENC_DPRINT("%d :: GetParameter OMX_IndexParamAudioPortFormat \n",__LINE__);
            if(((OMX_AUDIO_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nPortIndex ==
                pComponentPrivate->pPortDef[G726ENC_INPUT_PORT]->nPortIndex) {
                    if(((OMX_AUDIO_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nIndex >
                      pComponentPrivate->pCompPort[G726ENC_INPUT_PORT]->pPortFormat->nPortIndex) {
                        eError = OMX_ErrorNoMore;
					}
					else {
						memcpy(ComponentParameterStructure, pComponentPrivate->pCompPort[G726ENC_INPUT_PORT]->pPortFormat, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
					}
				}
				else if(((OMX_AUDIO_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nPortIndex ==
							pComponentPrivate->pPortDef[G726ENC_OUTPUT_PORT]->nPortIndex){
					if(((OMX_AUDIO_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nIndex >
												 pComponentPrivate->pCompPort[G726ENC_OUTPUT_PORT]->pPortFormat->nPortIndex) {
						eError = OMX_ErrorNoMore;
					}
					else {
						memcpy(ComponentParameterStructure, pComponentPrivate->pCompPort[G726ENC_OUTPUT_PORT]->pPortFormat, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
					}
				}
				else {
					G726ENC_DPRINT("%d :: OMX_ErrorBadPortIndex from GetParameter \n",__LINE__);
					eError = OMX_ErrorBadPortIndex;
				}
			break;

		case OMX_IndexParamAudioPcm:
            G726ENC_DPRINT("%d :: GetParameter OMX_IndexParamAudioG726 \n",__LINE__);
            if(((OMX_AUDIO_PARAM_PCMMODETYPE *)(ComponentParameterStructure))->nPortIndex ==
						pComponentPrivate->G726Params[G726ENC_OUTPUT_PORT]->nPortIndex) {
                memcpy(ComponentParameterStructure, pComponentPrivate->G726Params[G726ENC_OUTPUT_PORT], sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));

            } 
            else {
                G726ENC_DPRINT("%d :: OMX_ErrorBadPortIndex from GetParameter \n",__LINE__);
                eError = OMX_ErrorBadPortIndex;
            }
			break;

            case OMX_IndexParamPriorityMgmt:
	        if (pComponentPrivate->sPriorityMgmt == NULL) {
                    eError = OMX_ErrorBadParameter;
		    break;
		}
                G726ENC_DPRINT("%d :: GetParameter OMX_IndexParamPriorityMgmt \n",__LINE__);
                memcpy(ComponentParameterStructure, pComponentPrivate->sPriorityMgmt, sizeof(OMX_PRIORITYMGMTTYPE));
                break;

            case OMX_IndexParamVideoInit:
                break;
        
            case OMX_IndexParamImageInit:
                break;
        
            case OMX_IndexParamOtherInit:
                break;

		default:
            G726ENC_DPRINT("%d :: OMX_ErrorUnsupportedIndex GetParameter \n",__LINE__);
            eError = OMX_ErrorUnsupportedIndex;
			break;
    }
EXIT:
    G726ENC_DPRINT("%d :: Exiting GetParameter\n",__LINE__);
	G726ENC_DPRINT("%d :: Returning = 0x%x\n",__LINE__,eError);
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
    G726ENC_COMPONENT_PRIVATE  *pComponentPrivate = NULL;
    OMX_AUDIO_PARAM_PORTFORMATTYPE* pComponentParam = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE *pComponentParamPort = NULL;
    OMX_AUDIO_PARAM_G726TYPE *pCompG726Param = NULL;

        pComponentPrivate = (G726ENC_COMPONENT_PRIVATE *)(((OMX_COMPONENTTYPE*)hComp)->pComponentPrivate);


	G726ENC_DPRINT("%d :: Entering the SetParameter\n",__LINE__);
	if (pCompParam == NULL) {
		eError = OMX_ErrorBadParameter;
		G726ENC_DPRINT("%d :: OMX_ErrorBadParameter from SetParameter",__LINE__);
		goto EXIT;
	}
	if (pComponentPrivate->curState != OMX_StateLoaded) {
		eError = OMX_ErrorIncorrectStateOperation;
		G726ENC_DPRINT("%d :: OMX_ErrorIncorrectStateOperation from SetParameter",__LINE__);
		goto EXIT;
	}

    switch(nParamIndex) {
        case OMX_IndexParamAudioPortFormat:
				G726ENC_DPRINT("%d :: SetParameter OMX_IndexParamAudioPortFormat \n",__LINE__);
				pComponentParam = (OMX_AUDIO_PARAM_PORTFORMATTYPE *)pCompParam;
				if ( pComponentParam->nPortIndex == pComponentPrivate->pCompPort[G726ENC_INPUT_PORT]->pPortFormat->nPortIndex ) {
					memcpy(pComponentPrivate->pCompPort[G726ENC_INPUT_PORT]->pPortFormat, pComponentParam, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
				} else if ( pComponentParam->nPortIndex == pComponentPrivate->pCompPort[G726ENC_OUTPUT_PORT]->pPortFormat->nPortIndex ) {
					memcpy(pComponentPrivate->pCompPort[G726ENC_OUTPUT_PORT]->pPortFormat, pComponentParam, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
				} else {
					G726ENC_DPRINT("%d :: OMX_ErrorBadPortIndex from SetParameter",__LINE__);
					eError = OMX_ErrorBadPortIndex;
				}
            break;
        case OMX_IndexParamAudioG726:
                G726ENC_DPRINT("%d :: SetParameter OMX_IndexParamAudioG726 \n",__LINE__);
                pCompG726Param = (OMX_AUDIO_PARAM_G726TYPE *)pCompParam;
                if (pCompG726Param->nPortIndex == OMX_DirOutput) {
	            if (((G726ENC_COMPONENT_PRIVATE *)
		        pHandle->pComponentPrivate)->G726Params[G726ENC_OUTPUT_PORT] == NULL) {
	                eError = OMX_ErrorBadParameter;
                        break;
	            }
                    memcpy(((G726ENC_COMPONENT_PRIVATE *)
                            pHandle->pComponentPrivate)->G726Params[G726ENC_OUTPUT_PORT], pCompG726Param, sizeof(OMX_AUDIO_PARAM_G726TYPE));
                }
                else if (pCompG726Param->nPortIndex == OMX_DirInput) {
		    if (((G726ENC_COMPONENT_PRIVATE *)
		        pHandle->pComponentPrivate)->G726Params[G726ENC_INPUT_PORT] == NULL) {
	                eError = OMX_ErrorBadParameter;
                        break;
	            }
                    memcpy(((G726ENC_COMPONENT_PRIVATE *)
                            pHandle->pComponentPrivate)->G726Params[G726ENC_INPUT_PORT], pCompG726Param, sizeof(OMX_AUDIO_PARAM_G726TYPE));
                }
				else {
					G726ENC_DPRINT("%d :: OMX_ErrorBadPortIndex from SetParameter",__LINE__);
					eError = OMX_ErrorBadPortIndex;
				}
            break;
        case OMX_IndexParamPortDefinition:

                pComponentParamPort = (OMX_PARAM_PORTDEFINITIONTYPE *)pCompParam;
		G726ENC_DPRINT("%d :: SetParameter OMX_IndexParamPortDefinition \n",__LINE__);
		if(((OMX_PARAM_PORTDEFINITIONTYPE *)(pCompParam))->nPortIndex == pComponentPrivate->pPortDef[G726ENC_INPUT_PORT]->nPortIndex) {
			G726ENC_DPRINT("%d :: SetParameter OMX_IndexParamPortDefinition \n",__LINE__);
			memcpy(pComponentPrivate->pPortDef[G726ENC_INPUT_PORT], pCompParam, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
		}
		else if(((OMX_PARAM_PORTDEFINITIONTYPE *)(pCompParam))->nPortIndex == pComponentPrivate->pPortDef[G726ENC_OUTPUT_PORT]->nPortIndex) {
  		        G726ENC_DPRINT("%d :: SetParameter OMX_IndexParamPortDefinition \n",__LINE__);
			memcpy(pComponentPrivate->pPortDef[G726ENC_OUTPUT_PORT], pCompParam, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
		     }
		     else {
		           G726ENC_DPRINT("%d :: OMX_ErrorBadPortIndex from SetParameter",__LINE__);
			   eError = OMX_ErrorBadPortIndex;
			  }
        	break;
		case OMX_IndexParamPriorityMgmt:
		       if (pComponentPrivate->sPriorityMgmt == NULL) {
                           eError = OMX_ErrorBadParameter;
			   break;
		       }
				G726ENC_DPRINT("%d :: SetParameter OMX_IndexParamPriorityMgmt \n",__LINE__);
	        	memcpy(pComponentPrivate->sPriorityMgmt, (OMX_PRIORITYMGMTTYPE*)pCompParam, sizeof(OMX_PRIORITYMGMTTYPE));
			break;

	    case OMX_IndexParamAudioInit:
		       if (pComponentPrivate->sPortParam == NULL) {
                           eError = OMX_ErrorBadParameter;
			   break;
		       }
				G726ENC_DPRINT("%d :: SetParameter OMX_IndexParamAudioInit \n",__LINE__);
				memcpy(pComponentPrivate->sPortParam, (OMX_PORT_PARAM_TYPE*)pCompParam, sizeof(OMX_PORT_PARAM_TYPE));
		    break;
        default:
				G726ENC_DPRINT("%d :: SetParameter OMX_ErrorUnsupportedIndex \n",__LINE__);
				eError = OMX_ErrorUnsupportedIndex;
            break;
    }
EXIT:
	G726ENC_DPRINT("%d :: Exiting SetParameter\n",__LINE__);
	G726ENC_DPRINT("%d :: Returning = 0x%x\n",__LINE__,eError);
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
    TI_OMX_STREAM_INFO *streamInfo = NULL;
    OMX_COMPONENTTYPE* pHandle = (OMX_COMPONENTTYPE*)hComp;
    G726ENC_COMPONENT_PRIVATE *pComponentPrivate =
                         (G726ENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;

    if(nConfigIndex == OMX_IndexCustomG726ENCStreamIDConfig){
	    OMX_NBMALLOC_STRUCT(streamInfo, TI_OMX_STREAM_INFO);
        if(streamInfo == NULL){
		    eError = OMX_ErrorBadParameter;
		    goto EXIT;
	    }
		streamInfo->streamId = pComponentPrivate->streamID;
		memcpy(ComponentConfigStructure,streamInfo,sizeof(TI_OMX_STREAM_INFO));
    	OMX_NBMEMFREE_STRUCT(streamInfo);
	}
EXIT:
    G726ENC_DPRINT("%d :: Entering GetConfig\n", __LINE__);
    G726ENC_DPRINT("%d :: Exiting GetConfig\n", __LINE__);
	G726ENC_DPRINT("%d :: Returning = 0x%x\n",__LINE__,eError);
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
    G726ENC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
	OMX_COMPONENTTYPE* pHandle = (OMX_COMPONENTTYPE*)hComp;
	TI_OMX_DSP_DEFINITION *pTiDspDefinition = NULL;
    TI_OMX_DATAPATH dataPath; 
    OMX_S16 *customFlag = NULL;
    OMX_AUDIO_CONFIG_VOLUMETYPE *pGainStructure = NULL;
    OMX_U32 fdwrite = 0;
	
#ifdef DSP_RENDERING_ON
    AM_COMMANDDATATYPE cmd_data;
#endif        
	G726ENC_DPRINT("%d :: Entering SetConfig\n", __LINE__);
	if (pHandle == NULL) {
		G726ENC_DPRINT ("%d :: Invalid HANDLE OMX_ErrorBadParameter \n",__LINE__);
		eError = OMX_ErrorBadParameter;
		goto EXIT;
	}

	pComponentPrivate = (G726ENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
	switch (nConfigIndex) {

		case OMX_IndexCustomG726ENCModeConfig: 
			pTiDspDefinition = (TI_OMX_DSP_DEFINITION*)ComponentConfigStructure;
			if (pTiDspDefinition == NULL) {
				eError = OMX_ErrorBadParameter;
				G726ENC_DPRINT("%d :: OMX_ErrorBadParameter from SetConfig\n",__LINE__);
			 	goto EXIT;
			 }			
			pComponentPrivate->dasfMode = pTiDspDefinition->dasfMode;
			pComponentPrivate->acdnMode = pTiDspDefinition->acousticMode;
			pComponentPrivate->rtpMode = pTiDspDefinition->rtpMode;
			pComponentPrivate->streamID = pTiDspDefinition->streamId;
			break;
       case  OMX_IndexCustomG726ENCDataPath:
            customFlag = (OMX_S16*)ComponentConfigStructure;
            dataPath = *customFlag;            
            switch(dataPath) {
                case DATAPATH_APPLICATION:
                    G726ENC_DPRINT("------>Stream ID on SetConfig %d\n", (int)pComponentPrivate->streamID);
                    OMX_MMMIXER_DATAPATH(pComponentPrivate->sDeviceString, RENDERTYPE_ENCODER, pComponentPrivate->streamID);
                    break;

                case DATAPATH_APPLICATION_RTMIXER:
                    strcpy((char*)pComponentPrivate->sDeviceString,(char*)RTM_STRING_ENCODER);
                    break;
                    
                case DATAPATH_ACDN:
                     strcpy((char*)pComponentPrivate->sDeviceString,(char*)ACDN_STRING_ENCODER);
                     break;
                default:
                     break;
                    
            }
			break;
		case OMX_IndexConfigAudioVolume:
#ifdef DSP_RENDERING_ON
            if((fdwrite=open(FIFO1,O_WRONLY))<0) {
     	          G726ENC_DPRINT("%d :: [NBAMRE Encoder Component] - failure to open WRITE pipe\n",__LINE__);
            }
            else{
			     pGainStructure = (OMX_AUDIO_CONFIG_VOLUMETYPE *)ComponentConfigStructure;
			     cmd_data.hComponent = hComp;
			     cmd_data.AM_Cmd = AM_CommandSWGain;
			     cmd_data.param1 = pGainStructure->sVolume.nValue;
			     cmd_data.param2 = 0;
			     cmd_data.streamID = pComponentPrivate->streamID;

      			 if((write(fdwrite, &cmd_data, sizeof(cmd_data)))<0)
		    	 {	
				     G726ENC_DPRINT("[G726 encoder] - fail to send command to audio manager\n");
			     }
			     else
			     {
				     G726ENC_DPRINT("[G726 encoder] - ok to send command to audio manager\n");
			     }
			     close(fdwrite);
          }
#endif			
		default:
			eError = OMX_ErrorUnsupportedIndex;
		break;
	}
EXIT:
	G726ENC_DPRINT("%d :: Exiting SetConfig\n", __LINE__);
	G726ENC_DPRINT("%d :: Returning = 0x%x\n",__LINE__,eError);
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
	G726ENC_DPRINT("%d :: Entering GetState\n", __LINE__);
    if (!pState) {
        eError = OMX_ErrorBadParameter;
		G726ENC_DPRINT("%d :: OMX_ErrorBadParameter from GetState\n",__LINE__);
        goto EXIT;
    }

    if (pHandle && pHandle->pComponentPrivate) {
        *pState =  ((G726ENC_COMPONENT_PRIVATE*)
                                     pHandle->pComponentPrivate)->curState;
    } else {
        *pState = OMX_StateLoaded;
    }
    eError = OMX_ErrorNone;

EXIT:
	G726ENC_DPRINT("%d :: Exiting GetState\n", __LINE__);
	G726ENC_DPRINT("%d :: Returning = 0x%x\n",__LINE__,eError);
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
    G726ENC_COMPONENT_PRIVATE *pComponentPrivate =
                         (G726ENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    pPortDef = ((G726ENC_COMPONENT_PRIVATE*)pComponentPrivate)->pPortDef[G726ENC_INPUT_PORT];

	G726ENC_DPRINT("%d :: Entering EmptyThisBuffer\n", __LINE__);
    if (pBuffer == NULL) {
        eError = OMX_ErrorBadParameter;
		G726ENC_DPRINT("%d :: About to return OMX_ErrorBadParameter\n",__LINE__);
        goto EXIT;
    }

	if (pBuffer->nSize != sizeof(OMX_BUFFERHEADERTYPE)) {
		eError = OMX_ErrorBadParameter;
		G726ENC_DPRINT("%d :: About to return OMX_ErrorBadParameter\n",__LINE__);
		goto EXIT;
	}

	if (!pPortDef->bEnabled) {
		eError  = OMX_ErrorIncorrectStateOperation;
		G726ENC_DPRINT("%d :: About to return OMX_ErrorIncorrectStateOperation\n",__LINE__);
		goto EXIT;
	}

	if (pBuffer->nVersion.nVersion != pComponentPrivate->nVersion) {
		eError = OMX_ErrorVersionMismatch;
		G726ENC_DPRINT("%d :: About to return OMX_ErrorVersionMismatch\n",__LINE__);
		goto EXIT;
	}

	if (pBuffer->nInputPortIndex != G726ENC_INPUT_PORT) {
		eError  = OMX_ErrorBadPortIndex;
		G726ENC_DPRINT("%d :: About to return OMX_ErrorBadPortIndex\n",__LINE__);
		goto EXIT;
	}

	if (pComponentPrivate->curState != OMX_StateExecuting && pComponentPrivate->curState != OMX_StatePause) {
		eError= OMX_ErrorIncorrectStateOperation;
		G726ENC_DPRINT("%d :: About to return OMX_ErrorIncorrectStateOperation\n",__LINE__);
		goto EXIT;
	}


    G726ENC_DPRINT("----------------------------------------------------------------\n");
    G726ENC_DPRINT("%d :: Comp Sending Filled ip buff = %p to CompThread\n",__LINE__,pBuffer);
    G726ENC_DPRINT("----------------------------------------------------------------\n");



    pComponentPrivate->pMarkData = pBuffer->pMarkData;
    pComponentPrivate->hMarkTargetComponent = pBuffer->hMarkTargetComponent;

    ret = write (pComponentPrivate->dataPipe[1], &pBuffer, sizeof(OMX_BUFFERHEADERTYPE*));
    if (ret == -1) {
        G726ENC_DPRINT("%d :: Error in Writing to the Data pipe\n", __LINE__);
        eError = OMX_ErrorHardware;
        goto EXIT;
    }
	pComponentPrivate->nEmptyThisBufferCount++;
EXIT:
	G726ENC_DPRINT("%d :: Exiting EmptyThisBuffer\n", __LINE__);
	G726ENC_DPRINT("%d :: Returning = 0x%x\n",__LINE__,eError);
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
    G726ENC_COMPONENT_PRIVATE *pComponentPrivate =
                         (G726ENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
	pPortDef = ((G726ENC_COMPONENT_PRIVATE*)pComponentPrivate)->pPortDef[G726ENC_OUTPUT_PORT];
	G726ENC_DPRINT("%d :: Entering FillThisBuffer\n", __LINE__);
    G726ENC_DPRINT("------------------------------------------------------------------\n");
    G726ENC_DPRINT("%d :: Comp Sending Emptied op buff = %p to CompThread\n",__LINE__,pBuffer);
    G726ENC_DPRINT("------------------------------------------------------------------\n");
    if (pBuffer == NULL) {
        eError = OMX_ErrorBadParameter;
		G726ENC_DPRINT(" %d :: About to return OMX_ErrorBadParameter\n",__LINE__);
        goto EXIT;
    }

	if (pBuffer->nSize != sizeof(OMX_BUFFERHEADERTYPE)) {
		eError = OMX_ErrorBadParameter;
		G726ENC_DPRINT(" %d :: About to return OMX_ErrorBadParameter\n",__LINE__);
		goto EXIT;
	}

	if (!pPortDef->bEnabled) {
		eError  = OMX_ErrorIncorrectStateOperation;
		G726ENC_DPRINT("%d :: About to return OMX_ErrorIncorrectStateOperation\n",__LINE__);
		goto EXIT;
	}

	if (pBuffer->nVersion.nVersion != pComponentPrivate->nVersion) {
		eError = OMX_ErrorVersionMismatch;
		G726ENC_DPRINT(" %d :: About to return OMX_ErrorVersionMismatch\n",__LINE__);
		goto EXIT;
	}

	if (pBuffer->nOutputPortIndex != G726ENC_OUTPUT_PORT) {
		eError  = OMX_ErrorBadPortIndex;
		G726ENC_DPRINT(" %d :: About to return OMX_ErrorBadPortIndex\n",__LINE__);
		goto EXIT;
	}

    if(pComponentPrivate->curState != OMX_StateExecuting && pComponentPrivate->curState != OMX_StatePause) {
        eError = OMX_ErrorIncorrectStateOperation;
		G726ENC_DPRINT("%d :: About to return OMX_ErrorIncorrectStateOperation\n",__LINE__);
		goto EXIT;
	}

    pBuffer->nFilledLen = 0;
    /*Filling the Output buffer with zero */
    memset(pBuffer->pBuffer, 0, pBuffer->nAllocLen);


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
    ret = write (pComponentPrivate->dataPipe[1], &pBuffer, sizeof (OMX_BUFFERHEADERTYPE*));
    if (ret == -1) {
        G726ENC_DPRINT("%d :: Error in Writing to the Data pipe\n", __LINE__);
        eError = OMX_ErrorHardware;
        goto EXIT;
    }
	pComponentPrivate->nFillThisBufferCount++;
EXIT:
	G726ENC_DPRINT("%d :: Exiting FillThisBuffer\n", __LINE__);
	G726ENC_DPRINT("%d :: Returning = 0x%x\n",__LINE__,eError);
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
    G726ENC_COMPONENT_PRIVATE *pComponentPrivate =
                         (G726ENC_COMPONENT_PRIVATE *)pComponent->pComponentPrivate;
	G726ENC_DPRINT("%d :: Entering ComponentDeInit\n", __LINE__);

#ifdef RESOURCE_MANAGER_ENABLED
    eError = RMProxy_NewSendCommand(pHandle, RMProxy_FreeResource, OMX_G726_Encoder_COMPONENT, 0, 3456,NULL);
    if (eError != OMX_ErrorNone) {
         G726ENC_DPRINT ("%d ::OMX_G726_Encoder.c :: Error returned from destroy ResourceManagerProxy thread\n",
                                                        __LINE__);
    }
	eError = RMProxy_Deinitalize();
	if (eError != OMX_ErrorNone) {
         G726ENC_DPRINT("%d :: Error from RMProxy_Deinitalize\n",__LINE__);
         goto EXIT;
    }
#endif

    pComponentPrivate->bIsStopping = 1;
    eError = G726ENC_StopComponentThread(pHandle);
	if (eError != OMX_ErrorNone) {
         G726ENC_DPRINT("%d :: Error from G726ENC_StopComponentThread\n",__LINE__);
         goto EXIT;
    }
    /* Wait for thread to exit so we can get the status into "eError" */
    /* close the pipe handles */
    eError = G726ENC_FreeCompResources(pHandle);
	if (eError != OMX_ErrorNone) {
         G726ENC_DPRINT("%d :: Error from G726ENC_FreeCompResources\n",__LINE__);
         goto EXIT;
    }

#ifndef UNDER_CE
    pthread_mutex_destroy(&pComponentPrivate->InLoaded_mutex);
    pthread_cond_destroy(&pComponentPrivate->InLoaded_threshold);

    pthread_mutex_destroy(&pComponentPrivate->InIdle_mutex);
    pthread_cond_destroy(&pComponentPrivate->InIdle_threshold);

    pthread_mutex_destroy(&pComponentPrivate->AlloBuf_mutex);
    pthread_cond_destroy(&pComponentPrivate->AlloBuf_threshold);
#endif
    if (pComponentPrivate->sDeviceString != NULL) {
        SafeFree(pComponentPrivate->sDeviceString);
    }
	OMX_NBMEMFREE_STRUCT(pComponentPrivate);
EXIT:
    G726ENC_DPRINT("%d :: Exiting ComponentDeInit\n", __LINE__);
	G726ENC_DPRINT("%d :: Returning = 0x%x\n",__LINE__,eError);
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
    G726ENC_DPRINT("%d :: Entering ComponentTunnelRequest\n", __LINE__);
    G726ENC_DPRINT("%d :: Exiting ComponentTunnelRequest\n", __LINE__);
	G726ENC_DPRINT("%d :: Returning = 0x%x\n",__LINE__,eError);
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
    G726ENC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_BUFFERHEADERTYPE *pBufferHeader = NULL;

    pComponentPrivate = (G726ENC_COMPONENT_PRIVATE *)
            (((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);

    pPortDef = ((G726ENC_COMPONENT_PRIVATE*)pComponentPrivate)->pPortDef[nPortIndex];
    G726ENC_DPRINT("%d :: Entering AllocateBuffer\n", __LINE__);
    G726ENC_DPRINT("%d :: pPortDef = %p\n", __LINE__,pPortDef);
    G726ENC_DPRINT("%d :: pPortDef->bEnabled = %d\n", __LINE__,pPortDef->bEnabled);

    if(!pPortDef->bEnabled) {					
                pComponentPrivate->AlloBuf_waitingsignal = 1;
#ifndef UNDER_CE
                pthread_mutex_lock(&pComponentPrivate->AlloBuf_mutex);
                pthread_cond_wait(&pComponentPrivate->AlloBuf_threshold, &pComponentPrivate->AlloBuf_mutex);
                pthread_mutex_unlock(&pComponentPrivate->AlloBuf_mutex);
#endif
	}

	OMX_NBMALLOC_STRUCT(pBufferHeader, OMX_BUFFERHEADERTYPE);
        memset(pBufferHeader, 0x0, sizeof(OMX_BUFFERHEADERTYPE));

	pBufferHeader->pBuffer = (OMX_U8 *)SafeMalloc(nSizeBytes + 256);
	G726ENC_MEMPRINT("%d :: ALLOCATING MEMORY = %p\n",__LINE__,pBufferHeader->pBuffer);
	if (pBufferHeader->pBuffer == NULL) {
		/* Free previously allocated memory before bailing */
		if (pBufferHeader) {
			SafeFree(pBufferHeader);
			pBufferHeader = NULL;
		}
		eError = OMX_ErrorInsufficientResources;
		goto EXIT;
	}
    pBufferHeader->pBuffer += 128;

	if (nPortIndex == G726ENC_INPUT_PORT) {
                pBufferHeader->nInputPortIndex = nPortIndex;
		pBufferHeader->nOutputPortIndex = -1;
		pComponentPrivate->pInputBufferList->pBufHdr[pComponentPrivate->pInputBufferList->numBuffers] = pBufferHeader;
		pComponentPrivate->pInputBufferList->bBufferPending[pComponentPrivate->pInputBufferList->numBuffers] = 0;
		pComponentPrivate->pInputBufferList->bufferOwner[pComponentPrivate->pInputBufferList->numBuffers++] = 1;
		if (pComponentPrivate->pInputBufferList->numBuffers == pPortDef->nBufferCountActual) {
			pPortDef->bPopulated = OMX_TRUE;
			G726ENC_DPRINT("%d :: pPortDef->bPopulated = %d\n", __LINE__, pPortDef->bPopulated);
		}
	}
	else if (nPortIndex == G726ENC_OUTPUT_PORT) {
                pBufferHeader->nInputPortIndex = -1;
		pBufferHeader->nOutputPortIndex = nPortIndex;
		pComponentPrivate->pOutputBufferList->pBufHdr[pComponentPrivate->pOutputBufferList->numBuffers] = pBufferHeader;
		pComponentPrivate->pOutputBufferList->bBufferPending[pComponentPrivate->pOutputBufferList->numBuffers] = 0;
		pComponentPrivate->pOutputBufferList->bufferOwner[pComponentPrivate->pOutputBufferList->numBuffers++] = 1;
		if (pComponentPrivate->pOutputBufferList->numBuffers == pPortDef->nBufferCountActual) {
			pPortDef->bPopulated = OMX_TRUE;
		    G726ENC_DPRINT("%d :: pPortDef->bPopulated = %d\n", __LINE__, pPortDef->bPopulated);
		}
	}
	else {
		eError = OMX_ErrorBadPortIndex;
		G726ENC_DPRINT(" %d :: About to return OMX_ErrorBadPortIndex\n",__LINE__);
		goto EXIT;
	}
    if ((!pComponentPrivate->dasfMode                                 &&     /*File Mode*/
         pComponentPrivate->pPortDef[G726ENC_OUTPUT_PORT]->bPopulated &&
         pComponentPrivate->pPortDef[G726ENC_OUTPUT_PORT]->bEnabled   &&
         pComponentPrivate->pPortDef[G726ENC_INPUT_PORT]->bPopulated  &&
         pComponentPrivate->pPortDef[G726ENC_INPUT_PORT]->bEnabled    &&
         pComponentPrivate->InLoaded_readytoidle)      
         || 
        (pComponentPrivate->dasfMode                                   &&    /*Dasf Mode*/
         pComponentPrivate->pPortDef[G726ENC_OUTPUT_PORT]->bPopulated  &&  
         pComponentPrivate->pPortDef[G726ENC_OUTPUT_PORT]->bEnabled    &&
         pComponentPrivate->InLoaded_readytoidle)){
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
 	pBufferHeader->nVersion.s.nVersionMajor = G726ENC_MAJOR_VER;
    pBufferHeader->nVersion.s.nVersionMinor = G726ENC_MINOR_VER;
	pComponentPrivate->nVersion = pBufferHeader->nVersion.nVersion;
	pBufferHeader->nSize = sizeof(OMX_BUFFERHEADERTYPE);
    *pBuffer = pBufferHeader;
EXIT:
	G726ENC_DPRINT("%d :: Exiting AllocateBuffer\n",__LINE__);
	G726ENC_DPRINT("%d :: Returning = 0x%x\n",__LINE__,eError);
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
    G726ENC_COMPONENT_PRIVATE * pComponentPrivate = NULL;
    OMX_BUFFERHEADERTYPE* buff = NULL;
	OMX_U8* tempBuff = NULL;
	int i = 0;
	int inputIndex = -1;
	int outputIndex = -1;
    OMX_COMPONENTTYPE *pHandle = NULL;

    pComponentPrivate = (G726ENC_COMPONENT_PRIVATE *)
            					(((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);

	pHandle = (OMX_COMPONENTTYPE *) pComponentPrivate->pHandle;

	G726ENC_DPRINT("%d :: Entering FreeBuffer\n", __LINE__);
	for (i=0; i < G726ENC_MAX_NUM_OF_BUFS; i++) {
		buff = pComponentPrivate->pInputBufferList->pBufHdr[i];
		if (buff == pBuffer) {
			G726ENC_DPRINT("%d :: Found matching input buffer\n",__LINE__);
			G726ENC_DPRINT("%d :: buff = %p\n",__LINE__,buff);
			G726ENC_DPRINT("%d :: pBuffer = %p\n",__LINE__,pBuffer);
			inputIndex = i;
			break;
		}
		else {
			G726ENC_DPRINT("%d :: This is not a match\n",__LINE__);
			G726ENC_DPRINT("%d :: buff = %p\n",__LINE__,buff);
			G726ENC_DPRINT("%d :: pBuffer = %p\n",__LINE__,pBuffer);
		}
	}

	for (i=0; i < G726ENC_MAX_NUM_OF_BUFS; i++) {
		buff = pComponentPrivate->pOutputBufferList->pBufHdr[i];
		if (buff == pBuffer) {
			G726ENC_DPRINT("%d :: Found matching output buffer\n",__LINE__);
			G726ENC_DPRINT("%d :: buff = %p\n",__LINE__,buff);
			G726ENC_DPRINT("%d :: pBuffer = %p\n",__LINE__,pBuffer);
			outputIndex = i;
			break;
		}
		else {
			G726ENC_DPRINT("%d :: This is not a match\n",__LINE__);
			G726ENC_DPRINT("%d :: buff = %p\n",__LINE__,buff);
			G726ENC_DPRINT("%d :: pBuffer = %p\n",__LINE__,pBuffer);
		}
	}


	if (inputIndex != -1) {
		if (pComponentPrivate->pInputBufferList->bufferOwner[inputIndex] == 1) {
			tempBuff = pComponentPrivate->pInputBufferList->pBufHdr[inputIndex]->pBuffer;
			tempBuff -= 128;
			OMX_NBMEMFREE_STRUCT(tempBuff);
		}

		OMX_NBMEMFREE_STRUCT(pComponentPrivate->pInputBufferList->pBufHdr[inputIndex]);

			pComponentPrivate->pInputBufferList->numBuffers--;
			if (pComponentPrivate->pInputBufferList->numBuffers <
			pComponentPrivate->pPortDef[G726ENC_INPUT_PORT]->nBufferCountMin) {

			pComponentPrivate->pPortDef[G726ENC_INPUT_PORT]->bPopulated = OMX_FALSE;
			}
		if(pComponentPrivate->pPortDef[G726ENC_INPUT_PORT]->bEnabled &&
				(pComponentPrivate->curState == OMX_StateIdle ||
				pComponentPrivate->curState == OMX_StateExecuting ||
				pComponentPrivate->curState == OMX_StatePause)) {
			pComponentPrivate->cbInfo.EventHandler(	pHandle,
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
			tempBuff -= 128;
			OMX_NBMEMFREE_STRUCT(tempBuff);
		}
		OMX_NBMEMFREE_STRUCT(pComponentPrivate->pOutputBufferList->pBufHdr[outputIndex]);

		pComponentPrivate->pOutputBufferList->numBuffers--;
		if (pComponentPrivate->pOutputBufferList->numBuffers <
			pComponentPrivate->pPortDef[G726ENC_OUTPUT_PORT]->nBufferCountMin) {
			pComponentPrivate->pPortDef[G726ENC_OUTPUT_PORT]->bPopulated = OMX_FALSE;
		}
		if(pComponentPrivate->pPortDef[G726ENC_OUTPUT_PORT]->bEnabled &&
			(pComponentPrivate->curState == OMX_StateIdle ||
			pComponentPrivate->curState == OMX_StateExecuting ||
			pComponentPrivate->curState == OMX_StatePause)) {
			pComponentPrivate->cbInfo.EventHandler( pHandle,
													pHandle->pApplicationPrivate,
													OMX_EventError,
													OMX_ErrorPortUnpopulated,
													nPortIndex,
													NULL);
		}
	}
	else {
		G726ENC_DPRINT("%d :: Returning OMX_ErrorBadParameter\n",__LINE__);
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
#endif
       }
		if (pComponentPrivate->bDisableCommandPending) {
			SendCommand (pComponentPrivate->pHandle,OMX_CommandPortDisable,pComponentPrivate->bDisableCommandParam,NULL);
		}
    G726ENC_DPRINT("%d :: Exiting FreeBuffer\n", __LINE__);
	G726ENC_DPRINT("%d :: Returning = 0x%x\n",__LINE__,eError);
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
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef = NULL;
    G726ENC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_BUFFERHEADERTYPE *pBufferHeader = NULL;

    pComponentPrivate = (G726ENC_COMPONENT_PRIVATE *)
            (((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);

    pPortDef = ((G726ENC_COMPONENT_PRIVATE*)pComponentPrivate)->pPortDef[nPortIndex];
    G726ENC_DPRINT("%d :: Entering UseBuffer\n", __LINE__);
	G726ENC_DPRINT("%d :: pPortDef->bPopulated = %d \n",__LINE__,pPortDef->bPopulated);

    if(!pPortDef->bEnabled) {
		G726ENC_DPRINT("%d :: About to return OMX_ErrorIncorrectStateOperation\n",__LINE__);
        eError = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }

    if(nSizeBytes != pPortDef->nBufferSize || pPortDef->bPopulated) {
		G726ENC_DPRINT("%d :: About to return OMX_ErrorBadParameter\n",__LINE__);
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }

	OMX_NBMALLOC_STRUCT(pBufferHeader, OMX_BUFFERHEADERTYPE);
    memset((pBufferHeader), 0x0, sizeof(OMX_BUFFERHEADERTYPE));

    if (nPortIndex == G726ENC_OUTPUT_PORT) {
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
    
if ((!pComponentPrivate->dasfMode                                   &&     /*File Mode*/
         pComponentPrivate->pPortDef[G726ENC_OUTPUT_PORT]->bPopulated &&
         pComponentPrivate->pPortDef[G726ENC_OUTPUT_PORT]->bEnabled   &&
         pComponentPrivate->pPortDef[G726ENC_INPUT_PORT]->bPopulated  && 
         pComponentPrivate->pPortDef[G726ENC_INPUT_PORT]->bEnabled    &&
         pComponentPrivate->InLoaded_readytoidle)      
         || 
        (pComponentPrivate->dasfMode                                     &&    /*Dasf Mode*/
         pComponentPrivate->pPortDef[G726ENC_OUTPUT_PORT]->bPopulated  &&  
         pComponentPrivate->pPortDef[G726ENC_OUTPUT_PORT]->bEnabled    &&
         pComponentPrivate->InLoaded_readytoidle)){
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
    pBufferHeader->nVersion.s.nVersionMajor = G726ENC_MAJOR_VER;
    pBufferHeader->nVersion.s.nVersionMinor = G726ENC_MINOR_VER;
	pComponentPrivate->nVersion = pBufferHeader->nVersion.nVersion;
	pBufferHeader->pBuffer = pBuffer;
	pBufferHeader->nSize = sizeof(OMX_BUFFERHEADERTYPE);
    *ppBufferHdr = pBufferHeader;
EXIT:
    G726ENC_DPRINT("%d :: Exiting UseBuffer\n", __LINE__);
	G726ENC_DPRINT("%d :: Returning = 0x%x\n",__LINE__,eError);
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

	G726ENC_DPRINT("GetExtensionIndex\n");
	if (!(strcmp(cParameterName,"OMX.TI.index.config.tispecific"))) {
		*pIndexType = OMX_IndexCustomG726ENCModeConfig;
		G726ENC_DPRINT("OMX.TI.index.config.tispecific\n");
	}
	else if (!(strcmp(cParameterName,"OMX.TI.index.config.G726.streamIDinfo"))) {
		*pIndexType = OMX_IndexCustomG726ENCStreamIDConfig;
		G726ENC_DPRINT("OMX.TI.index.config.G726.streamIDinfo\n");
	}
	else if (!(strcmp(cParameterName,"OMX.TI.index.config.G726.datapath"))) {
		*pIndexType = OMX_IndexCustomG726ENCDataPath;
		G726ENC_DPRINT("OMX.TI.index.config.G726.datapath\n");
	}
	else {
		eError = OMX_ErrorBadParameter;
	}

	G726ENC_DPRINT("Exiting GetExtensionIndex\n");
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
    G726ENC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    pComponentPrivate = (G726ENC_COMPONENT_PRIVATE *)(((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);

    if(nIndex == 0){
      if (cRole == NULL) {
          eError = OMX_ErrorBadParameter;
      }
      else {
          memcpy(cRole, &pComponentPrivate->componentRole.cRole, sizeof(OMX_U8) * OMX_MAX_STRINGNAME_SIZE); 
          G726ENC_DPRINT("::::In ComponenetRoleEnum: cRole is set to %s\n",cRole);
      }
    }
    else {
      eError = OMX_ErrorNoMore;
    	}
    return eError;
}
