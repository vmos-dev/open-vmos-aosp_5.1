
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
* @file OMX_JpegDecoder.c
*
* This file implements OMX Component for JPEG decoder
*
* @patth $(CSLPATH)\jpeg_dec\src\OMX_JpegDecoder.c
*
* @rev 0.2
*/

/****************************************************************
 *  INCLUDE FILES
*****************************************************************/

/* -----------System and Platform Files ------------------------*/

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

/* -----------------Program Header Files ---------------------------*/

#include "OMX_JpegDec_Utils.h"

#ifdef RESOURCE_MANAGER_ENABLED
    #include <ResourceManagerProxyAPI.h>
#endif

/*----------------------Global-----------------------------------*/
OMX_STRING cJPEGdecName = "OMX.TI.JPEG.decoder";


/* ---------------------------- Fucntion prototypes -----------------*/

static OMX_ERRORTYPE SetCallbacks_JPEGDec(OMX_HANDLETYPE hComp,
                                          OMX_CALLBACKTYPE* pCallBacks,
                                          OMX_PTR pAppData);
static OMX_ERRORTYPE GetComponentVersion_JPEGDec(OMX_HANDLETYPE hComp,
                                                 OMX_STRING pComponentName,
                                                 OMX_VERSIONTYPE* pComponentVersion,
                                                 OMX_VERSIONTYPE* pSpecVersion,
                                                 OMX_UUIDTYPE* pComponentUUID);
static OMX_ERRORTYPE SendCommand_JPEGDec(OMX_HANDLETYPE hComponent,
                                         OMX_COMMANDTYPE Cmd,
                                         OMX_U32 nParam,
                                         OMX_PTR pCmdData);
static OMX_ERRORTYPE GetParameter_JPEGDec(OMX_IN OMX_HANDLETYPE hComponent,
                                          OMX_IN OMX_INDEXTYPE nParamIndex,
                                          OMX_INOUT OMX_PTR ComponentParameterStructure);
static OMX_ERRORTYPE SetParameter_JPEGDec(OMX_HANDLETYPE hComp,
                                          OMX_INDEXTYPE nParamIndex,
                                          OMX_PTR ComponentParameterStructure);
static OMX_ERRORTYPE GetConfig_JPEGDec(OMX_HANDLETYPE hComp,
                                       OMX_INDEXTYPE nConfigIndex,
                                       OMX_PTR ComponentConfigStructure);
static OMX_ERRORTYPE SetConfig_JPEGDec(OMX_HANDLETYPE hComp,
                                       OMX_INDEXTYPE nConfigIndex,
                                       OMX_PTR ComponentConfigStructure);
static OMX_ERRORTYPE EmptyThisBuffer(OMX_HANDLETYPE hComp,
                                     OMX_BUFFERHEADERTYPE* pBuffer);

static OMX_ERRORTYPE FillThisBuffer_JPEGDec(OMX_HANDLETYPE hComp,
                                            OMX_BUFFERHEADERTYPE* pBuffer);

static OMX_ERRORTYPE GetState(OMX_HANDLETYPE hComp,
                              OMX_STATETYPE* pState);
static OMX_ERRORTYPE ComponentTunnelRequest_JPEGDec(OMX_IN OMX_HANDLETYPE hComp,
                                                    OMX_IN OMX_U32 nPort,
                                                    OMX_IN OMX_HANDLETYPE hTunneledComp,
                                                    OMX_IN OMX_U32 nTunneledPort,
                                                    OMX_INOUT  OMX_TUNNELSETUPTYPE* pTunnelSetup);
static OMX_ERRORTYPE UseBuffer_JPEGDec(OMX_IN OMX_HANDLETYPE hComponent,
                                                   OMX_INOUT OMX_BUFFERHEADERTYPE** ppBufferHdr,
                                                   OMX_IN OMX_U32 nPortIndex,
                                                   OMX_IN OMX_PTR pAppPrivate,
                                                   OMX_IN OMX_U32 nSizeBytes,
                                                   OMX_IN OMX_U8* pBuffer) ;
static OMX_ERRORTYPE FreeBuffer_JPEGDec(OMX_IN OMX_HANDLETYPE hComponent,
                                        OMX_IN OMX_U32 nPortIndex,
                                        OMX_IN OMX_BUFFERHEADERTYPE* pBuffer);
static OMX_ERRORTYPE Allocate_DSPResources_JPEGDec(OMX_IN JPEGDEC_COMPONENT_PRIVATE *pComponentPrivate,
                                                   OMX_IN OMX_U32 nPortIndex);
static OMX_ERRORTYPE VerifyTunnelConnection_JPEGDec(JPEGDEC_PORT_TYPE *pPort,
                                                    OMX_HANDLETYPE hTunneledComp,
                                                    OMX_PARAM_PORTDEFINITIONTYPE* pPortDef);
static  OMX_ERRORTYPE AllocateBuffer_JPEGDec(OMX_IN OMX_HANDLETYPE hComponent,
                                             OMX_INOUT OMX_BUFFERHEADERTYPE** pBuffer,
                                             OMX_IN OMX_U32 nPortIndex,
                                             OMX_IN OMX_PTR pAppPrivate,
                                             OMX_IN OMX_U32 nSizeBytes);
static void JPEGDEC_InitBufferFlagTrack(JPEGDEC_COMPONENT_PRIVATE *pComponentPrivate,
										OMX_U32 nPortIndex);
#ifdef KHRONOS_1_1
static OMX_ERRORTYPE ComponentRoleEnum(OMX_IN OMX_HANDLETYPE hComponent,
                                       OMX_OUT OMX_U8 *cRole,
                                       OMX_IN OMX_U32 nIndex);
#endif

static OMX_ERRORTYPE GetExtensionIndex_JPEGDec(OMX_IN OMX_HANDLETYPE hComponent, 
                                                OMX_IN OMX_STRING cParameterName, 
                                                OMX_OUT OMX_INDEXTYPE* pIndexType);

/* -------------------------- Function Implementation ------------------ */


/* ========================================================================== */
/**
 * @fn AllocateBuffer_JPEGDec - Implements allocatebuffer functionality
 * @param hComponent - Component handle.
 * @param pBuffHead - pointer to buffer header structure
 * @param nPortIndex - Port index
 * @param pAppPrivate - pointer to application private data
 * @param nSizeBytes - size of the buffer to allocate
 * @return: OMX_ERRORTYPE
 *          OMX_ErrorNone on success
 *          !OMX_ErrorNone on any failure
 */
/* ========================================================================== */
static OMX_ERRORTYPE AllocateBuffer_JPEGDec(OMX_IN OMX_HANDLETYPE hComponent,
                                            OMX_INOUT OMX_BUFFERHEADERTYPE** pBuffHead,
                                            OMX_IN OMX_U32 nPortIndex,
                                            OMX_IN OMX_PTR pAppPrivate,
                                            OMX_IN OMX_U32 nSizeBytes)
{
    /*VALIDATE INPUT ARGUMENTS*/
    OMX_COMPONENTTYPE *pHandle = NULL;
    JPEGDEC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDef = NULL;
    JPEGDEC_PORT_TYPE* pCompPort;
    OMX_U8 nBufferCount = -1;
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    OMX_CHECK_PARAM(hComponent);
    pHandle = (OMX_COMPONENTTYPE *)hComponent;
    pComponentPrivate = (JPEGDEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    JPEGDEC_OMX_CONF_CHECK_CMD(pComponentPrivate, 1, 1);

    pPortDef = pComponentPrivate->pCompPort[nPortIndex]->pPortDef;
    nBufferCount = pComponentPrivate->pCompPort[nPortIndex]->nBuffCount;
    pCompPort = pComponentPrivate->pCompPort[nPortIndex];

    OMX_PRINT1(pComponentPrivate->dbg, "Entering function AllocateBuffer_JPEGDec\n");

    OMX_PRBUFFER1(pComponentPrivate->dbg, "In AllocateBuffer_JPEGDec %d %lu %lu\n", 
	    nBufferCount, pPortDef->nBufferCountActual, nPortIndex);

    if (nBufferCount >= pPortDef->nBufferCountActual) {
        eError = OMX_ErrorInsufficientResources;
        OMX_PRBUFFER4(pComponentPrivate->dbg, " try to allocate more buffers that the port supports\n");
	goto EXIT;
    }

    if (nPortIndex == JPEGDEC_INPUT_PORT) {
        OMX_MALLOC(pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pBufferPrivate[nBufferCount]->pBufferHdr, sizeof(OMX_BUFFERHEADERTYPE));
        OMX_CONF_INIT_STRUCT(pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pBufferPrivate[nBufferCount]->pBufferHdr, OMX_BUFFERHEADERTYPE);
        OMX_PRBUFFER1(pComponentPrivate->dbg, "pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pBufferPrivate[nBufferCount]->pBufferHdr = %p\n", pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pBufferPrivate[nBufferCount]->pBufferHdr);
        pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pBufferPrivate[nBufferCount]->pBufferHdr->nOutputPortIndex = OMX_NOPORT;
        pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pBufferPrivate[nBufferCount]->pBufferHdr->nInputPortIndex = pPortDef->nPortIndex;
        pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pBufferPrivate[nBufferCount]->pBufferHdr->nSize = sizeof(OMX_BUFFERHEADERTYPE);
        pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pBufferPrivate[nBufferCount]->pBufferHdr->nVersion = pPortDef->nVersion;
        pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pBufferPrivate[nBufferCount]->pBufferHdr->pAppPrivate = pAppPrivate;
        pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pBufferPrivate[nBufferCount]->pBufferHdr->nAllocLen = nSizeBytes;
        pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pBufferPrivate[nBufferCount]->bAllocbyComponent = OMX_TRUE;

        *pBuffHead = pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pBufferPrivate[nBufferCount]->pBufferHdr;

    }
    else if (nPortIndex == JPEGDEC_OUTPUT_PORT) {

        OMX_MALLOC(pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pBufferPrivate[nBufferCount]->pBufferHdr, sizeof(OMX_BUFFERHEADERTYPE));
        OMX_CONF_INIT_STRUCT(pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pBufferPrivate[nBufferCount]->pBufferHdr, OMX_BUFFERHEADERTYPE);
        OMX_PRBUFFER1(pComponentPrivate->dbg, "pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pBufferPrivate[nBuffCount]->pBufferHdr = %p\n", pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pBufferPrivate[nBufferCount]->pBufferHdr);
        pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pBufferPrivate[nBufferCount]->pBufferHdr->nOutputPortIndex = pPortDef->nPortIndex;
        pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pBufferPrivate[nBufferCount]->pBufferHdr->nInputPortIndex = OMX_NOPORT;
        pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pBufferPrivate[nBufferCount]->pBufferHdr->nSize = sizeof(OMX_BUFFERHEADERTYPE);
        pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pBufferPrivate[nBufferCount]->pBufferHdr->nVersion = pPortDef->nVersion;
        pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pBufferPrivate[nBufferCount]->pBufferHdr->pAppPrivate = pAppPrivate;
        pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pBufferPrivate[nBufferCount]->pBufferHdr->nAllocLen = nSizeBytes;
        pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pBufferPrivate[nBufferCount]->bAllocbyComponent = OMX_TRUE;

        *pBuffHead = pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pBufferPrivate[nBufferCount]->pBufferHdr;

    }
    else {
	eError = OMX_ErrorBadPortIndex;
	goto EXIT;
    }

    if (nPortIndex == JPEGDEC_INPUT_PORT) {
        OMX_MALLOC(pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pBufferPrivate[nBufferCount]->pBufferHdr->pBuffer, nSizeBytes + 256);
        pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pBufferPrivate[nBufferCount]->pBufferHdr->pBuffer = (pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pBufferPrivate[nBufferCount]->pBufferHdr->pBuffer) + 128;

#ifdef __PERF_INSTRUMENTATION__
        PERF_ReceivedFrame(pComponentPrivate->pPERF,
                           pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pBufferPrivate[nBufferCount]->pBufferHdr->pBuffer,
                           nSizeBytes,
                           PERF_ModuleMemory);
#endif

    }
    else if (nPortIndex == JPEGDEC_OUTPUT_PORT) {
        OMX_MALLOC(pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pBufferPrivate[nBufferCount]->pBufferHdr->pBuffer, nSizeBytes + 256);
        pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pBufferPrivate[nBufferCount]->pBufferHdr->pBuffer = pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pBufferPrivate[nBufferCount]->pBufferHdr->pBuffer + 128;

#ifdef __PERF_INSTRUMENTATION__
        PERF_ReceivedFrame(pComponentPrivate->pPERF,
                           pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pBufferPrivate[nBufferCount]->pBufferHdr->pBuffer,
                           nSizeBytes,
                           PERF_ModuleMemory);
#endif

    }
    else {
	eError = OMX_ErrorBadPortIndex;
	goto EXIT;
    }

    eError = Allocate_DSPResources_JPEGDec(pComponentPrivate, nPortIndex);
    if (eError != OMX_ErrorNone)    {
	OMX_PRDSP4(pComponentPrivate->dbg, "OMX_ErrorInsufficientResources\n");
	eError = OMX_ErrorInsufficientResources;
	goto EXIT;
    }

    if (nPortIndex == JPEGDEC_INPUT_PORT) {
        pComponentPrivate->pCompPort[nPortIndex]->pBufferPrivate[nBufferCount]->pBufferHdr->pInputPortPrivate =
            pComponentPrivate->pCompPort[nPortIndex]->pBufferPrivate[nBufferCount];
   }
    else {
        pComponentPrivate->pCompPort[nPortIndex]->pBufferPrivate[nBufferCount]->pBufferHdr->pOutputPortPrivate =
            pComponentPrivate->pCompPort[nPortIndex]->pBufferPrivate[nBufferCount];
    }

    if (pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->hTunnelComponent != NULL) {
            pComponentPrivate->pCompPort[nPortIndex]->pBufferPrivate[nBufferCount]->eBufferOwner = JPEGDEC_BUFFER_TUNNEL_COMPONENT;
        }
    else {
            pComponentPrivate->pCompPort[nPortIndex]->pBufferPrivate[nBufferCount]->eBufferOwner = JPEGDEC_BUFFER_CLIENT;
        }

    pComponentPrivate->pCompPort[nPortIndex]->nBuffCount++;
    if (pComponentPrivate->pCompPort[nPortIndex]->nBuffCount == pPortDef->nBufferCountActual) {
            pPortDef->bPopulated = OMX_TRUE;
            JPEGDEC_InitBufferFlagTrack(pComponentPrivate, nPortIndex);
            pthread_mutex_lock(&(pComponentPrivate->mJpegDecMutex));
            pthread_cond_signal(&(pComponentPrivate->sPortPopulated_cond));
            pthread_mutex_unlock(&(pComponentPrivate->mJpegDecMutex));
	    OMX_PRINT2(pComponentPrivate->dbg, " Port [%lu] Populated!\n", nPortIndex); 
    }

EXIT:
    return eError;
}   /* End of AllocateBuffer_JPEGDec */


/* ========================================================================== */
/**
 * @fn FreeBuffer_JPEGDec - Implements freebuffer functionality
 * @param hComponent - Component handle.
 * @param nPortIndex - Port index
 * @param pBuffHead - pointer to buffer header structure
 * @return: OMX_ERRORTYPE
 *          OMX_ErrorNone on success
 *          !OMX_ErrorNone on any failure
 */
/* ========================================================================== */
static OMX_ERRORTYPE FreeBuffer_JPEGDec(OMX_IN OMX_HANDLETYPE hComponent,
                                        OMX_IN OMX_U32 nPortIndex,
                                        OMX_IN OMX_BUFFERHEADERTYPE* pBuffHead)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pHandle = NULL;
    JPEGDEC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDef = NULL;
    JPEGDEC_BUFFER_PRIVATE* pBuffPrivate = NULL;
    OMX_U8* pTemp;
    OMX_U8 nBufferCount = -1;

    OMX_CHECK_PARAM(hComponent);
    OMX_CHECK_PARAM(pBuffHead);

    pHandle = (OMX_COMPONENTTYPE *)hComponent;
    pComponentPrivate = (JPEGDEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;

    JPEGDEC_OMX_CONF_CHECK_CMD(pComponentPrivate, 1, 1);  

    if (nPortIndex != 1 && nPortIndex != 0) {
	    eError = OMX_ErrorBadPortIndex;
	    goto PRINT_EXIT;
    }

    OMX_PRBUFFER2(pComponentPrivate->dbg, "JPEG-D: Entering funtion FreeBuffer_JPEGDec %lu %p\n", nPortIndex, pBuffHead);
    
    if(pComponentPrivate->nCurState == OMX_StateLoaded){
        eError = OMX_ErrorIncorrectStateOperation;
	goto PRINT_EXIT;
    }
    pPortDef = pComponentPrivate->pCompPort[nPortIndex]->pPortDef;
    nBufferCount = pComponentPrivate->pCompPort[nPortIndex]->nBuffCount;
    OMX_PRBUFFER1(pComponentPrivate->dbg, "Trying to Free Header %p\nBuffer %p\n", pBuffHead, pBuffHead->pBuffer);

    if (nPortIndex == JPEGDEC_INPUT_PORT) {

        pBuffPrivate = pBuffHead->pInputPortPrivate;
        if (pBuffPrivate->pUALGParams) {
            pTemp = (OMX_U8*)(pBuffPrivate->pUALGParams);
            pTemp -= 128;
            (pBuffPrivate->pUALGParams) = (JPEGDEC_UAlgInBufParamStruct *)pTemp;
            OMX_FREE(pBuffPrivate->pUALGParams);
            pBuffPrivate->pUALGParams = NULL;
        }
    }
    else if (nPortIndex == JPEGDEC_OUTPUT_PORT) {

        pBuffPrivate = pBuffHead->pOutputPortPrivate;
        if (pBuffPrivate->pUALGParams) {
            pTemp = (OMX_U8*)(pBuffPrivate->pUALGParams);
            pTemp -= 128;
            (pBuffPrivate->pUALGParams) = (JPEGDEC_UAlgOutBufParamStruct *)pTemp;
            OMX_FREE(pBuffPrivate->pUALGParams);
            pBuffPrivate->pUALGParams = NULL;
        }
    }
    else {
        eError = OMX_ErrorBadPortIndex; 
	goto PRINT_EXIT;
    }

    if (pBuffPrivate->bAllocbyComponent == OMX_TRUE) {
        if (pBuffHead->pBuffer) {

#ifdef __PERF_INSTRUMENTATION__
            PERF_SendingFrame(pComponentPrivate->pPERF,
                              pBuffHead->pBuffer,
                              pBuffHead->nFilledLen,
                              PERF_ModuleMemory);
#endif

            OMX_PRBUFFER1(pComponentPrivate->dbg, "INTERNAL BUFFER USED , trying to free it.\n");
            pBuffHead->pBuffer -= 128;
            OMX_FREE(pBuffHead->pBuffer);
            pBuffHead->pBuffer = NULL;
        }
    }

    if (pBuffHead) {
        OMX_FREE(pBuffHead);
        pBuffHead = NULL;
    }

        
    OMX_PRSTATE2(pComponentPrivate->dbg, "Current state is %d and To State is %d\n", pComponentPrivate->nCurState, pComponentPrivate->nToState);
        if ( pPortDef->bEnabled && 
            ((pComponentPrivate->nCurState == OMX_StateIdle && pComponentPrivate->nToState != OMX_StateLoaded) ||
            (pComponentPrivate->nCurState == OMX_StateExecuting  || 
            pComponentPrivate->nCurState == OMX_StatePause)) ) {                                      

            pComponentPrivate->cbInfo.EventHandler (pComponentPrivate->pHandle, 
                                                    pComponentPrivate->pHandle->pApplicationPrivate,
                                                    OMX_EventError,
                                                    OMX_ErrorPortUnpopulated, 
                                                    OMX_TI_ErrorMinor,
                                                    "Port Unpopulated");
        }

    nBufferCount = --pComponentPrivate->pCompPort[nPortIndex]->nBuffCount;

    OMX_PRBUFFER1(pComponentPrivate->dbg, "nBufferCount %d\n", nBufferCount);
    if (nBufferCount == 0) {
        pComponentPrivate->pCompPort[nPortIndex]->pPortDef->bPopulated = OMX_FALSE;
        OMX_PRBUFFER2(pComponentPrivate->dbg, "port %lu is %u\n", nPortIndex, pComponentPrivate->pCompPort[nPortIndex]->pPortDef->bPopulated);
        pthread_mutex_lock(&(pComponentPrivate->mJpegDecMutex));
        pthread_cond_signal(&(pComponentPrivate->sPortPopulated_cond));
        pthread_mutex_unlock(&(pComponentPrivate->mJpegDecMutex));
    }

    if ((!pPortDef->bEnabled) &&
        (!pComponentPrivate->pCompPort[nPortIndex]->pPortDef->bPopulated)) {
        if ((nPortIndex == 0) && (!pComponentPrivate->bInportDisableComplete)) {
            pComponentPrivate->bInportDisableComplete = OMX_TRUE;
            pComponentPrivate->cbInfo.EventHandler (pComponentPrivate->pHandle,
                                                pComponentPrivate->pHandle->pApplicationPrivate,
                                                OMX_EventCmdComplete,
                                                OMX_CommandPortDisable,
                                                0,
                                                NULL);              
        }   
        if ((nPortIndex == 1) && (!pComponentPrivate->bOutportDisableComplete)) {
            pComponentPrivate->bOutportDisableComplete = OMX_TRUE;
            pComponentPrivate->cbInfo.EventHandler (pComponentPrivate->pHandle,
                                                pComponentPrivate->pHandle->pApplicationPrivate,
                                                OMX_EventCmdComplete,
                                                OMX_CommandPortDisable,
                                                1,
                                                NULL);              
        }
    }
PRINT_EXIT: 
    OMX_PRINT1(pComponentPrivate->dbg, "Exit from FreeBuffer function error = %x\n", eError);
EXIT: 
    return eError;
}   /* End of FreeBuffer_JPEGDec */


/* ========================================================================== */
/**
 * @fn UseBuffer_JPEGDec - Implements usebuffer functionality
 * @param hComponent - Component handle.
 * @param pBuffHead - pointer to buffer header structure
 * @param nPortIndex - Port index
 * @param pAppPrivate - pointer to application private data
 * @param nSizeBytes - size of the buffer
 * @param pBuffer - pointer to the buffer
 * @return: OMX_ERRORTYPE
 *          OMX_ErrorNone on success
 *          !OMX_ErrorNone on any failure
 */
/* ========================================================================== */
static OMX_ERRORTYPE UseBuffer_JPEGDec(OMX_IN OMX_HANDLETYPE hComponent,
                                        OMX_INOUT OMX_BUFFERHEADERTYPE** ppBufferHdr,
                                        OMX_IN OMX_U32 nPortIndex,
                                        OMX_IN OMX_PTR pAppPrivate,
                                        OMX_IN OMX_U32 nSizeBytes,
                                        OMX_IN OMX_U8* pBuffer)
{
    OMX_COMPONENTTYPE *pHandle = NULL;
    JPEGDEC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDef = NULL;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_HANDLETYPE hTunnelComponent;
    OMX_U8 nBufferCount = -1;

    pHandle = (OMX_COMPONENTTYPE *)hComponent;
    pComponentPrivate = (JPEGDEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    JPEGDEC_OMX_CONF_CHECK_CMD(pComponentPrivate, 1, 1); 

    pPortDef = pComponentPrivate->pCompPort[nPortIndex]->pPortDef;
    nBufferCount = pComponentPrivate->pCompPort[nPortIndex]->nBuffCount;
    hTunnelComponent = pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->hTunnelComponent;

    OMX_PRINT1(pComponentPrivate->dbg, "Entering funtion UseBuffer_JPEGDec\n");

#ifdef __PERF_INSTRUMENTATION__
    PERF_ReceivedFrame(pComponentPrivate->pPERF,
                       pBuffer,
                       nSizeBytes,
                       PERF_ModuleHLMM);
#endif


    if (nPortIndex == JPEGDEC_INPUT_PORT) {
        OMX_MALLOC(pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pBufferPrivate[nBufferCount]->pBufferHdr, sizeof(OMX_BUFFERHEADERTYPE));
        *ppBufferHdr = pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pBufferPrivate[nBufferCount]->pBufferHdr;
    }
    else if (nPortIndex == JPEGDEC_OUTPUT_PORT) {
        OMX_MALLOC(pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pBufferPrivate[nBufferCount]->pBufferHdr, sizeof(OMX_BUFFERHEADERTYPE));
        *ppBufferHdr = pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pBufferPrivate[nBufferCount]->pBufferHdr;
    }
    else {
	eError = OMX_ErrorBadPortIndex;
	goto EXIT;
    }
    pComponentPrivate->pCompPort[nPortIndex]->pBufferPrivate[nBufferCount]->bAllocbyComponent = OMX_FALSE;

    /* set relevant fields */
    (*ppBufferHdr)->nSize = sizeof(OMX_BUFFERHEADERTYPE);
    (*ppBufferHdr)->nVersion = pPortDef->nVersion;
    (*ppBufferHdr)->pBuffer = pBuffer;
    (*ppBufferHdr)->nAllocLen = nSizeBytes;
    (*ppBufferHdr)->pAppPrivate = pAppPrivate;

    if (hTunnelComponent != NULL) {
        /* set direction dependent fields */
        if (pPortDef->eDir == OMX_DirInput) {
            (*ppBufferHdr)->nInputPortIndex = nPortIndex;
            (*ppBufferHdr)->nOutputPortIndex = pComponentPrivate->pCompPort[nPortIndex]->nTunnelPort;

            pComponentPrivate->pCompPort[nPortIndex]->pBufferPrivate[nBufferCount]->pBufferHdr = (*ppBufferHdr);
        }
        else {
            (*ppBufferHdr)->nOutputPortIndex   = nPortIndex;
            (*ppBufferHdr)->nInputPortIndex    = pComponentPrivate->pCompPort[nPortIndex]->nTunnelPort;

            pComponentPrivate->pCompPort[nPortIndex]->pBufferPrivate[nBufferCount]->pBufferHdr = (*ppBufferHdr);
        }
    }
    else {
        if (nPortIndex == JPEGDEC_INPUT_PORT) {
            pComponentPrivate->pCompPort[nPortIndex]->pBufferPrivate[nBufferCount]->pBufferHdr->nInputPortIndex  = JPEGDEC_INPUT_PORT;
            pComponentPrivate->pCompPort[nPortIndex]->pBufferPrivate[nBufferCount]->pBufferHdr->nOutputPortIndex = OMX_NOPORT;
        }
        else {
            pComponentPrivate->pCompPort[nPortIndex]->pBufferPrivate[nBufferCount]->pBufferHdr->nInputPortIndex  = OMX_NOPORT;
            pComponentPrivate->pCompPort[nPortIndex]->pBufferPrivate[nBufferCount]->pBufferHdr->nOutputPortIndex = JPEGDEC_OUTPUT_PORT;
        }
    }
    if (nPortIndex == JPEGDEC_INPUT_PORT) {
        pComponentPrivate->pCompPort[nPortIndex]->pBufferPrivate[nBufferCount]->pBufferHdr->pInputPortPrivate =
                                                     pComponentPrivate->pCompPort[nPortIndex]->pBufferPrivate[nBufferCount];
    }
    else {
        pComponentPrivate->pCompPort[nPortIndex]->pBufferPrivate[nBufferCount]->pBufferHdr->pOutputPortPrivate =
                                                     pComponentPrivate->pCompPort[nPortIndex]->pBufferPrivate[nBufferCount];
    }

    eError = Allocate_DSPResources_JPEGDec(pComponentPrivate, nPortIndex);
    if (eError != OMX_ErrorNone) {
	OMX_PRDSP4(pComponentPrivate->dbg, "Error: Allocate_DSPResources_JPEGDec failed\n");
	eError = OMX_ErrorInsufficientResources;
	goto EXIT;
    }
    /* increment buffer count */

    pComponentPrivate->pCompPort[nPortIndex]->nBuffCount++;
    if (pComponentPrivate->pCompPort[nPortIndex]->nBuffCount == pPortDef->nBufferCountActual) {
        pPortDef->bPopulated = OMX_TRUE;
        JPEGDEC_InitBufferFlagTrack(pComponentPrivate, nPortIndex);
        pthread_mutex_lock(&(pComponentPrivate->mJpegDecMutex));
        pthread_cond_signal(&(pComponentPrivate->sPortPopulated_cond));
        pthread_mutex_unlock(&(pComponentPrivate->mJpegDecMutex));
    }
EXIT:
    return eError;
}   /* end of UseBuffer_JPEGDec */



/* ========================================================================== */
/**
 * @fn OMX_ComponentInit - Updates the component function pointer to the handle.
 *  Sets default parameters.
 * @param hComponent - Component handle.
 * @return: OMX_ERRORTYPE
 *          OMX_ErrorNone on success
 *          !OMX_ErrorNone on any failure
 */
/* ========================================================================== */
OMX_ERRORTYPE OMX_ComponentInit(OMX_HANDLETYPE hComponent)
{

    OMX_COMPONENTTYPE *pHandle = NULL;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    JPEGDEC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    OMX_U8 i = 0;
    OMX_CHECK_PARAM(hComponent);
    pHandle = (OMX_COMPONENTTYPE *)hComponent;

    LinkedList_Create(&AllocList);
    
    OMX_MALLOC(pHandle->pComponentPrivate, sizeof(JPEGDEC_COMPONENT_PRIVATE));

    pComponentPrivate = (JPEGDEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;

    JPEGDEC_OMX_CONF_CHECK_CMD(pComponentPrivate, 1, 1);

    OMX_DBG_INIT(pComponentPrivate->dbg, "OMX_DBG_JPEGDEC");

    OMX_PRINT1(pComponentPrivate->dbg, "Entering funtion OMX_ComponentInit\n");

#ifdef __PERF_INSTRUMENTATION__
    pComponentPrivate->pPERF = PERF_Create(PERF_FOURS("JPD "), PERF_ModuleLLMM | PERF_ModuleImageDecode);
    pComponentPrivate->pPERFcomp = NULL;
#endif

    ((JPEGDEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate)->pHandle = pHandle;
    pComponentPrivate->nCurState = OMX_StateLoaded;
    pComponentPrivate->ComponentVersion.s.nVersionMajor = 0x01;
    pComponentPrivate->ComponentVersion.s.nVersionMinor = 0x00;
    pComponentPrivate->ComponentVersion.s.nRevision = 0x00;
    pComponentPrivate->ComponentVersion.s.nStep = 0x00;
    OMX_MALLOC(pComponentPrivate->cComponentName, COMP_MAX_NAMESIZE + 1);        
    strncpy(pComponentPrivate->cComponentName, cJPEGdecName, COMP_MAX_NAMESIZE);

    pHandle->SetCallbacks = SetCallbacks_JPEGDec;
    pHandle->GetComponentVersion = GetComponentVersion_JPEGDec;
    pHandle->SendCommand = SendCommand_JPEGDec;
    pHandle->GetParameter = GetParameter_JPEGDec;
    pHandle->SetParameter = SetParameter_JPEGDec;
    pHandle->GetConfig = GetConfig_JPEGDec;
    pHandle->SetConfig = SetConfig_JPEGDec;
    pHandle->GetState = GetState;
    pHandle->EmptyThisBuffer = EmptyThisBuffer;
    pHandle->FillThisBuffer = FillThisBuffer_JPEGDec;
    pHandle->ComponentTunnelRequest = ComponentTunnelRequest_JPEGDec;
    pHandle->ComponentDeInit = ComponentDeInit;
    pHandle->UseBuffer = UseBuffer_JPEGDec;
    pHandle->FreeBuffer =  FreeBuffer_JPEGDec;
    pHandle->AllocateBuffer = AllocateBuffer_JPEGDec;
    pHandle->GetExtensionIndex      = GetExtensionIndex_JPEGDec;
#ifdef KHRONOS_1_1
    pHandle->ComponentRoleEnum = ComponentRoleEnum;
#endif

    /* Allocate memory for component data structures */
    OMX_MALLOC(pComponentPrivate->pPortParamType, sizeof(OMX_PORT_PARAM_TYPE));
    OMX_MALLOC(pComponentPrivate->pPriorityMgmt, sizeof(OMX_PRIORITYMGMTTYPE));
    OMX_MALLOC(pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT], sizeof(JPEGDEC_PORT_TYPE));
    OMX_MALLOC(pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT], sizeof(JPEGDEC_PORT_TYPE));
    OMX_MALLOC(pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pParamBufSupplier, sizeof(OMX_PARAM_BUFFERSUPPLIERTYPE));
    OMX_MALLOC(pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pParamBufSupplier, sizeof(OMX_PARAM_BUFFERSUPPLIERTYPE));
    OMX_MALLOC(pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pPortDef, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
    OMX_MALLOC(pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pPortDef, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
    OMX_MALLOC(pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pPortFormat, sizeof(OMX_IMAGE_PARAM_PORTFORMATTYPE));
    OMX_MALLOC(pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pPortFormat, sizeof(OMX_IMAGE_PARAM_PORTFORMATTYPE));
    OMX_MALLOC(pComponentPrivate->pScalePrivate, sizeof(OMX_CONFIG_SCALEFACTORTYPE)); /* Scale Factor */
    OMX_MALLOC(pComponentPrivate->pSectionDecode, sizeof(OMX_CUSTOM_IMAGE_DECODE_SECTION));
    OMX_MALLOC(pComponentPrivate->pSubRegionDecode, sizeof(OMX_CUSTOM_IMAGE_DECODE_SUBREGION));

#ifdef KHRONOS_1_1
    OMX_MALLOC(pComponentPrivate->pAudioPortType, sizeof(OMX_PORT_PARAM_TYPE));
    OMX_CONF_INIT_STRUCT(pComponentPrivate->pAudioPortType, OMX_PORT_PARAM_TYPE);
    OMX_MALLOC(pComponentPrivate->pVideoPortType, sizeof(OMX_PORT_PARAM_TYPE));
    OMX_CONF_INIT_STRUCT(pComponentPrivate->pVideoPortType, OMX_PORT_PARAM_TYPE);
    OMX_MALLOC(pComponentPrivate->pOtherPortType, sizeof(OMX_PORT_PARAM_TYPE));
    OMX_CONF_INIT_STRUCT(pComponentPrivate->pOtherPortType, OMX_PORT_PARAM_TYPE);
    OMX_MALLOC(pComponentPrivate->pCompRole, sizeof(OMX_PARAM_COMPONENTROLETYPE));
    OMX_CONF_INIT_STRUCT(pComponentPrivate->pCompRole,OMX_PARAM_COMPONENTROLETYPE);
    OMX_MALLOC(pComponentPrivate->pQuantTable, sizeof(OMX_IMAGE_PARAM_QUANTIZATIONTABLETYPE));
    OMX_CONF_INIT_STRUCT(pComponentPrivate->pQuantTable,OMX_IMAGE_PARAM_QUANTIZATIONTABLETYPE);
    pComponentPrivate->pQuantTable->eQuantizationTable = OMX_IMAGE_QuantizationTableLuma;
    OMX_MALLOC(pComponentPrivate->pHuffmanTable, sizeof(OMX_IMAGE_PARAM_HUFFMANTTABLETYPE));
    OMX_CONF_INIT_STRUCT(pComponentPrivate->pHuffmanTable,OMX_IMAGE_PARAM_HUFFMANTTABLETYPE);
    pComponentPrivate->pHuffmanTable->eHuffmanTable = OMX_IMAGE_HuffmanTableAC;

    pComponentPrivate->pAudioPortType->nPorts = 0;
    pComponentPrivate->pAudioPortType->nStartPortNumber = -1;
    pComponentPrivate->pVideoPortType->nPorts = 0;
    pComponentPrivate->pVideoPortType->nStartPortNumber = -1;
    pComponentPrivate->pOtherPortType->nPorts = 0;
    pComponentPrivate->pOtherPortType->nStartPortNumber = -1;
    
#endif
    for (i = 0;i<NUM_OF_BUFFERS;i++) {
        OMX_MALLOC(pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pBufferPrivate[i], sizeof(JPEGDEC_BUFFER_PRIVATE));
        pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pBufferPrivate[i]->pBufferHdr = NULL;
    }

    for (i = 0;i<NUM_OF_BUFFERS;i++) {
        OMX_MALLOC(pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pBufferPrivate[i], sizeof(JPEGDEC_BUFFER_PRIVATE));
        pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pBufferPrivate[i]->pBufferHdr = NULL;
    }

    pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->hTunnelComponent = NULL;
    pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->hTunnelComponent = NULL;
    pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->nBuffCount = 0;
    pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->nBuffCount = 0;
    pComponentPrivate->nProgressive = 0;
    pComponentPrivate->nInputFrameWidth = 1600;
    pComponentPrivate->nOutputColorFormat = OMX_COLOR_Format32bitARGB8888;
    pComponentPrivate->nProfileID = 0;
    pComponentPrivate->nIsLCMLActive = 0;
    pComponentPrivate->pDllHandle = NULL;
    pComponentPrivate->pLCML = NULL;
    pComponentPrivate->pMarkData = NULL;
    pComponentPrivate->pMarkBuf = NULL;
    pComponentPrivate->hMarkTargetComponent = NULL;
    pComponentPrivate->pHandle->pApplicationPrivate = (OMX_PTR *)200;
    pComponentPrivate->nInPortIn = 0;
    pComponentPrivate->nOutPortOut = 0;
    pComponentPrivate->bPreempted = OMX_FALSE;

/*Slide decoding defaults*/
    OMX_CONF_INIT_STRUCT(pComponentPrivate->pSectionDecode, OMX_CUSTOM_IMAGE_DECODE_SECTION);
    pComponentPrivate->pSectionDecode->nMCURow = 0;
    pComponentPrivate->pSectionDecode->nAU = 0;
    pComponentPrivate->pSectionDecode->bSectionsInput = 0;
    pComponentPrivate->pSectionDecode->bSectionsOutput = 0;

    OMX_CONF_INIT_STRUCT(pComponentPrivate->pSubRegionDecode, OMX_CUSTOM_IMAGE_DECODE_SUBREGION);
    pComponentPrivate->pSubRegionDecode->nXOrg = 0;
    pComponentPrivate->pSubRegionDecode->nYOrg = 0;
    pComponentPrivate->pSubRegionDecode->nXLength = 0;
    pComponentPrivate->pSubRegionDecode->nYLength = 0;


    /* Set pPortParamType defaults */
    OMX_CONF_INIT_STRUCT(pComponentPrivate->pPortParamType, OMX_PORT_PARAM_TYPE);
    pComponentPrivate->pPortParamType->nPorts = NUM_OF_PORTS;
    pComponentPrivate->pPortParamType->nStartPortNumber = 0x0;

    /* Set pInPortDef defaults */
    OMX_CONF_INIT_STRUCT(pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
    pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pPortDef->nPortIndex = JPEGDEC_INPUT_PORT;
    pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pPortDef->eDir = OMX_DirInput;
    pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pPortDef->nBufferCountActual = NUM_OF_BUFFERS;
    pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pPortDef->nBufferCountMin         = 1;
    pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pPortDef->nBufferSize             = 70532;
    pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pPortDef->bEnabled = OMX_TRUE;
    pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pPortDef->bPopulated = OMX_FALSE;
    pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pPortDef->eDomain = OMX_PortDomainImage;
    pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pPortDef->format.image.cMIMEType = "JPEGDEC";
    pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pPortDef->format.image.pNativeRender = NULL;
    pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pPortDef->format.image.nFrameWidth        = 640 ; /* 128*/ 
    pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pPortDef->format.image.nFrameHeight       = 480;  /*96 */
    pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pPortDef->format.image.nStride = -1;
    pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pPortDef->format.image.nSliceHeight = -1;
    pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pPortDef->format.image.bFlagErrorConcealment = OMX_FALSE;
    pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pPortDef->format.image.eCompressionFormat         =  OMX_IMAGE_CodingJPEG;   
    pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pPortDef->format.image.eColorFormat           =  OMX_COLOR_FormatUnused; 

    /* Set pOutPortDef defaults */
    OMX_CONF_INIT_STRUCT(pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
    pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pPortDef->nPortIndex = JPEGDEC_OUTPUT_PORT;
    pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pPortDef->eDir = OMX_DirOutput;
    pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pPortDef->nBufferCountMin        =  1; 
    pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pPortDef->nBufferCountActual         = NUM_OF_BUFFERS;
    pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pPortDef->nBufferSize            =  (640*480*2);
    pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pPortDef->bEnabled = OMX_TRUE;
    pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pPortDef->bPopulated = OMX_FALSE;
    pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pPortDef->eDomain = OMX_PortDomainImage;
    pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pPortDef->format.image.cMIMEType = "JPEGDEC";
    pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pPortDef->format.image.pNativeRender = NULL;
    pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pPortDef->format.image.nFrameWidth       =    640;
    pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pPortDef->format.image.nFrameHeight  = 480;
    pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pPortDef->format.image.nStride = -1;
    pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pPortDef->format.image.nSliceHeight = -1;
    pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pPortDef->format.image.bFlagErrorConcealment = OMX_FALSE;
    pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pPortDef->format.image.eCompressionFormat = OMX_IMAGE_CodingUnused;
    pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pPortDef->format.image.eColorFormat          =  OMX_COLOR_FormatYUV420PackedPlanar; 
    /* added for vpp tunneling */
    pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pPortDef->format.video.eColorFormat = OMX_COLOR_FormatYCbYCr;


    /* Set pInPortFormat defaults */
    OMX_CONF_INIT_STRUCT(pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pPortFormat, OMX_IMAGE_PARAM_PORTFORMATTYPE);
    pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pPortFormat->nPortIndex = JPEGDEC_INPUT_PORT;
    pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pPortFormat->nIndex = 0x0;
    pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pPortFormat->eCompressionFormat   =OMX_IMAGE_CodingJPEG; 
    pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pPortFormat->eColorFormat         = OMX_COLOR_FormatUnused; 

    /* Set pOutPortFormat defaults */
    OMX_CONF_INIT_STRUCT(pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pPortFormat, OMX_IMAGE_PARAM_PORTFORMATTYPE);
    pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pPortFormat->nPortIndex = JPEGDEC_OUTPUT_PORT;
    pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pPortFormat->nIndex = 0x0;
    pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pPortFormat->eCompressionFormat = OMX_IMAGE_CodingUnused;
    pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pPortFormat->eColorFormat        = OMX_COLOR_FormatYUV420PackedPlanar;

    /* Set pPriorityMgmt defaults */
    OMX_CONF_INIT_STRUCT(pComponentPrivate->pPriorityMgmt, OMX_PRIORITYMGMTTYPE);
    pComponentPrivate->pPriorityMgmt->nGroupPriority = -1;
    pComponentPrivate->pPriorityMgmt->nGroupID = -1;


    /* Set pInBufSupplier defaults */
    OMX_CONF_INIT_STRUCT(pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pParamBufSupplier, OMX_PARAM_BUFFERSUPPLIERTYPE);
    pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pParamBufSupplier->nPortIndex = JPEGDEC_INPUT_PORT;
    pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pParamBufSupplier->eBufferSupplier = OMX_BufferSupplyInput;

    /* Set pOutBufSupplier defaults */
    OMX_CONF_INIT_STRUCT(pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pParamBufSupplier, OMX_PARAM_BUFFERSUPPLIERTYPE);
    pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pParamBufSupplier->nPortIndex = JPEGDEC_OUTPUT_PORT;
    pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pParamBufSupplier->eBufferSupplier = OMX_BufferSupplyInput;

	/* Set default value for Max width & Height*/
	pComponentPrivate->sMaxResolution.nWidth = JPGDEC_SNTEST_MAX_WIDTH;
	pComponentPrivate->sMaxResolution.nHeight = JPGDEC_SNTEST_MAX_HEIGHT;

    /*Initialize Component mutex*/
    if (pthread_mutex_init(&(pComponentPrivate->mJpegDecMutex), NULL) != 0)
    {
	OMX_TRACE4(pComponentPrivate->dbg, "Error at Initialize fill this buffer mutex condition");
	eError = OMX_ErrorHardware;
	goto EXIT;
    }
    
    /* initialize a condition variable to its default value */ 
    if (pthread_cond_init(&(pComponentPrivate->sStop_cond), NULL) != 0)
    {
	OMX_TRACE4(pComponentPrivate->dbg, "Error at Initialize fill this buffer mutex condition");
	eError = OMX_ErrorHardware;
	goto EXIT;
    }

#ifdef KHRONOS_1_1
    strcpy((char *)pComponentPrivate->componentRole.cRole, "image_decoder.jpeg");
#endif


#ifdef RESOURCE_MANAGER_ENABLED
    /* load the ResourceManagerProxy thread */
    eError = RMProxy_NewInitalizeEx(OMX_COMPONENTTYPE_IMAGE);
    if (eError != OMX_ErrorNone) {
	OMX_PRMGR4(pComponentPrivate->dbg, "Error returned from loading ResourceManagerProxy thread: %x\n", eError);                
	goto EXIT;
    }
#endif

    eError= Start_ComponentThreadJpegDec(pHandle);


EXIT:
    if(eError != OMX_ErrorNone){
        /* LinkedList_DisplayAll (&AllocList); */
        OMX_FREEALL();
        LinkedList_Destroy(&AllocList);
    }
    return eError;
}   /* End of OMX_ComponentInit */
/* ========================================================================== */
/**
 * @fn SetCallbacks_JPEGDec - Sets application callbacks to the component
 * @param hComponent - Component handle.
 * @param pCallBacks - application callbacks
 * @param pAppData - pointer to application Data
 * @return: OMX_ERRORTYPE
 *          OMX_ErrorNone on success
 *          !OMX_ErrorNone on any failure
 */
/* ========================================================================== */
static OMX_ERRORTYPE SetCallbacks_JPEGDec(OMX_HANDLETYPE pComponent,
                                          OMX_CALLBACKTYPE* pCallBacks,
                                          OMX_PTR pAppData)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pHandle = NULL;
    JPEGDEC_COMPONENT_PRIVATE *pComponentPrivate = NULL;

    OMX_CHECK_PARAM(pComponent);

    pHandle = (OMX_COMPONENTTYPE*)pComponent;
    pComponentPrivate = (JPEGDEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;

    JPEGDEC_OMX_CONF_CHECK_CMD(pComponentPrivate, 1, 1); 

    OMX_PRINT1(pComponentPrivate->dbg, "Entering funtion SetCallbacks_JPEGDec\n");

    if (pCallBacks == NULL) {
        eError = OMX_ErrorBadParameter;
        OMX_ERROR5(pComponentPrivate->dbg, "Entering funtion SetCallbacks_JPEGDec\n"); 
        goto EXIT;
    }

    /* Copy the callbacks of the application to the component private  */
    OMX_MEMCPY_CHECK(&(pComponentPrivate->cbInfo));
    memcpy(&(pComponentPrivate->cbInfo), pCallBacks, sizeof(OMX_CALLBACKTYPE));


    /* copy the application private data to component memory */
    pHandle->pApplicationPrivate = pAppData;
    pComponentPrivate->nCurState = OMX_StateLoaded;

EXIT:
    return eError;
}   /* End of SetCallbacks_JPEGDec */



/* ========================================================================== */
/**
 * @fn GetComponentVersion_JPEGDec - This method will get the component Version.
 * @param hComp - handle for this instance of the component
 * @param pComponentName  - component name
 * @param pComponentVersion - component version
 * @param pSpecVersion - OMX specification version
 * @param pComponentUUID - UUID of the component
 * @return: OMX_ERRORTYPE
 *          OMX_ErrorNone on success
 *          !OMX_ErrorNone on any failure
 */
/* ========================================================================== */
static OMX_ERRORTYPE GetComponentVersion_JPEGDec(OMX_HANDLETYPE hComp,
                                                 OMX_STRING szComponentName,
                                                 OMX_VERSIONTYPE* pComponentVersion,
                                                 OMX_VERSIONTYPE* pSpecVersion,
                                                 OMX_UUIDTYPE* pComponentUUID)
{
    OMX_ERRORTYPE          eError            = OMX_ErrorNone;
    OMX_COMPONENTTYPE    * pHandle           = NULL;
    JPEGDEC_COMPONENT_PRIVATE* pComponentPrivate = NULL;
    OMX_U8 *pTemp = NULL;

    if (!hComp || !szComponentName || !pComponentVersion || !pSpecVersion || !pComponentUUID) {
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pHandle = (OMX_COMPONENTTYPE*)hComp;
    if (!pHandle->pComponentPrivate) {
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pComponentPrivate = (JPEGDEC_COMPONENT_PRIVATE*)pHandle->pComponentPrivate;

    JPEGDEC_OMX_CONF_CHECK_CMD(pComponentPrivate, 1, 1); 

    OMX_PRINT1(pComponentPrivate->dbg, "Entering funtion GetComponentVersion_JPEGDec\n");

    strncpy(szComponentName, pComponentPrivate->cComponentName, COMP_MAX_NAMESIZE);
    pTemp = memcpy(pComponentVersion, &(pComponentPrivate->ComponentVersion.s), sizeof(pComponentPrivate->ComponentVersion.s));
    if(pTemp == NULL){
        eError = OMX_ErrorUndefined;
        goto EXIT;
    }
    pTemp = memcpy(pSpecVersion, &(pComponentPrivate->SpecVersion.s), sizeof(pComponentPrivate->SpecVersion.s));
    if(pTemp == NULL){
        eError = OMX_ErrorUndefined;
        goto EXIT;
    }
EXIT:
    return eError;
}/* End of GetComponentVersion_JPEGDec */

/* ========================================================================== */
/**
 * @fn SendCommand_JPEGDec - Sends command from application to component
 * @param hComponent - handle for this instance of the component
 * @param Cmd - Command to be sent
 * @param nParam - Command parameters
 * @param pCmdData - Additional Command data
 * @return: OMX_ERRORTYPE
 *          OMX_ErrorNone on success
 *          !OMX_ErrorNone on any failure
 */
/* ========================================================================== */
static OMX_ERRORTYPE SendCommand_JPEGDec(OMX_HANDLETYPE hComponent,
                                         OMX_COMMANDTYPE Cmd,
                                         OMX_U32 nParam,
                                         OMX_PTR pCmdData)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pHandle = NULL;
    JPEGDEC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDefIn= NULL;
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDefOut = NULL;
    OMX_MARKTYPE* pMarkType = NULL;
    OMX_COMMANDTYPE eCmd = OMX_CustomCommandStopThread;
    int nRet = 0;

    OMX_CHECK_PARAM(hComponent);
    pHandle = (OMX_COMPONENTTYPE *)hComponent;
    pComponentPrivate = (JPEGDEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    JPEGDEC_OMX_CONF_CHECK_CMD(pComponentPrivate, 1, 1);

    pPortDefIn = pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pPortDef;
    pPortDefOut = pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pPortDef;

    OMX_PRINT1(pComponentPrivate->dbg, "Entering funtion SendCommand_JPEGDec %u %lu\n", Cmd, nParam);

    if (pComponentPrivate->nCurState == OMX_StateInvalid) {
	OMX_PRSTATE4(pComponentPrivate->dbg, "INVALID STATE\n"); 
        eError = OMX_ErrorInvalidState;
        goto EXIT;
    }
    switch (Cmd) {

    case OMX_CommandStateSet:
        eCmd = OMX_CommandStateSet;
            pComponentPrivate->nToState = nParam;
            OMX_PRSTATE2(pComponentPrivate->dbg, "To state %d\n", pComponentPrivate->nToState);
        break;

    case OMX_CommandFlush:
        if ((nParam != JPEGDEC_INPUT_PORT) &&
            (nParam != JPEGDEC_OUTPUT_PORT) &&
	    ((int)nParam != -1)) {
            eError = OMX_ErrorBadPortIndex;
            goto EXIT;
        }
        eCmd = OMX_CommandFlush;
        OMX_PRBUFFER2(pComponentPrivate->dbg, "get OMX_CommandFlush\n");
        break;

    case OMX_CommandPortDisable:
	    OMX_PRBUFFER2(pComponentPrivate->dbg, "Get port disable command %lu\n", nParam);
	if (nParam == JPEGDEC_INPUT_PORT  || (int)nParam == -1) {
            pPortDefIn->bEnabled = OMX_FALSE;
            pComponentPrivate->bInportDisableComplete = OMX_FALSE;
            OMX_PRBUFFER2(pComponentPrivate->dbg, "pPortDefIn->bEnabled %d\n", pPortDefIn->bEnabled);
        }
        if (nParam == JPEGDEC_OUTPUT_PORT  || (int)nParam == -1) {
                pPortDefOut->bEnabled = OMX_FALSE;
                pComponentPrivate->bOutportDisableComplete = OMX_FALSE;
                OMX_PRBUFFER2(pComponentPrivate->dbg, "pPortDefIn->bEnabled %d\n", pPortDefIn->bEnabled);
        }
        if ((nParam != JPEGDEC_INPUT_PORT) && (nParam != JPEGDEC_OUTPUT_PORT) && ((int)nParam != -1)) {
            eError = OMX_ErrorBadParameter;
            OMX_PRBUFFER4(pComponentPrivate->dbg, "OMX_ErrorBadParameter\n");
            goto EXIT;
        }

        if (!pPortDefIn->bPopulated) {
            pComponentPrivate->bInportDisableComplete = OMX_TRUE;
            OMX_PRBUFFER2(pComponentPrivate->dbg, "JPEGDEC_INPUT_PORT: OMX_CommandPortDisable\n");
            pComponentPrivate->cbInfo.EventHandler (pComponentPrivate->pHandle,
                                               pComponentPrivate->pHandle->pApplicationPrivate,
                                               OMX_EventCmdComplete,
                                               OMX_CommandPortDisable,
                                               JPEGDEC_INPUT_PORT,
                                               NULL);
        }

        if (!pPortDefOut->bPopulated) {
            pComponentPrivate->bOutportDisableComplete = OMX_TRUE;
            OMX_PRBUFFER1(pComponentPrivate->dbg, "JPEGDEC_OUTPUT_PORT : OMX_CommandPortDisable\n");
            pComponentPrivate->cbInfo.EventHandler (pComponentPrivate->pHandle,
                            pComponentPrivate->pHandle->pApplicationPrivate,
                            OMX_EventCmdComplete,
                            OMX_CommandPortDisable,
                            JPEGDEC_OUTPUT_PORT,
                            NULL);          
        }
        eCmd = OMX_CommandPortDisable;
        break;

    case OMX_CommandPortEnable:
	if (nParam == JPEGDEC_INPUT_PORT  || (int)nParam == -1) {
                pPortDefIn->bEnabled = OMX_TRUE;
        }
        if (nParam == JPEGDEC_OUTPUT_PORT  || (int)nParam == -1) {
                    pPortDefOut->bEnabled = OMX_TRUE;
        }
        if ((nParam != JPEGDEC_INPUT_PORT) && (nParam != JPEGDEC_OUTPUT_PORT) && ((int)nParam != -1)) {
            eError = OMX_ErrorBadParameter;
            goto EXIT;
        }
        eCmd = OMX_CommandPortEnable;
        break;

    case OMX_CommandMarkBuffer:
	    OMX_PRBUFFER2(pComponentPrivate->dbg, "JPEGDEC: Entered switch - Command Mark Buffer\n");
        eCmd = OMX_CommandMarkBuffer;
        /* we can only mark buffers on input port */
        if ( nParam != JPEGDEC_INPUT_PORT && nParam != JPEGDEC_OUTPUT_PORT){
            eError = OMX_ErrorBadPortIndex;
            goto EXIT;
        }
        pMarkType =(OMX_MARKTYPE*)pCmdData;
        pComponentPrivate->pMarkData = pMarkType->pMarkData;
        pComponentPrivate->hMarkTargetComponent = pMarkType->hMarkTargetComponent;
        pComponentPrivate->nMarkPort = nParam;
        break;

    default:
        break;
    }

    OMX_PRINT1(pComponentPrivate->dbg, "JPEGDEC: eCmd = %d\n",(int)eCmd);

#ifdef __PERF_INSTRUMENTATION__
    PERF_SendingCommand(pComponentPrivate->pPERF,
                        eCmd, (eCmd == OMX_CommandMarkBuffer) ? (OMX_U32) pCmdData : nParam,
                        PERF_ModuleComponent);
#endif

    nRet = write(pComponentPrivate->nCmdPipe[1], &eCmd, sizeof(eCmd));
    if (nRet == -1) {
	    OMX_PRCOMM4(pComponentPrivate->dbg, "Failed to write into nCmdPipe\n");
    }

    /* this needs to be fixed */
    /* port index is redundant for mark buffer as we have 1 input port */
    if (eCmd == OMX_CommandMarkBuffer) {
        nRet = write(pComponentPrivate->nCmdDataPipe[1], &pCmdData, sizeof(OMX_PTR));
        if (nRet == -1) {
		OMX_PRCOMM4(pComponentPrivate->dbg, "Failed to write into nCmdDataPipe, eCmd = MarkBuf\n");
        }
    }
    else {
        nRet = write(pComponentPrivate->nCmdDataPipe[1], &nParam, sizeof(nParam));
        OMX_PRCOMM2(pComponentPrivate->dbg, "write param %lu to pipe\n", nParam);
        if (nRet == -1) {
		OMX_PRCOMM4(pComponentPrivate->dbg, "Failed to write into nCmdDataPipe\n");
        }
    }

EXIT:
    return eError;
}   /* End of SendCommand_JPEGDec*/



/* ========================================================================== */
/**
 * @fn GetParameter_JPEGDec - Gets Parameters from the Component.
 *  This method will update parameters from the component to the app.
 * @param hComponent - handle for this instance of the component
 * @param nParamIndex - Component Index Port
 * @param ComponentParameterStructure - Component Parameter Structure
 * @return: OMX_ERRORTYPE
 *          OMX_ErrorNone on success
 *          !OMX_ErrorNone on any failure
 */
/* ========================================================================== */
static OMX_ERRORTYPE GetParameter_JPEGDec(OMX_IN OMX_HANDLETYPE hComponent,
                                          OMX_IN OMX_INDEXTYPE nParamIndex,
                                          OMX_INOUT OMX_PTR ComponentParameterStructure)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pComp = NULL;
    JPEGDEC_COMPONENT_PRIVATE *pComponentPrivate = NULL;

    OMX_CHECK_PARAM(hComponent);
    OMX_CHECK_PARAM(ComponentParameterStructure);
    pComp = (OMX_COMPONENTTYPE *)hComponent;
    pComponentPrivate = (JPEGDEC_COMPONENT_PRIVATE*)pComp->pComponentPrivate;

    JPEGDEC_OMX_CONF_CHECK_CMD(pComponentPrivate, 1, 1); 

    OMX_PRINT1(pComponentPrivate->dbg, "Entering funtion GetParameter_JPEGDec\n"); 

    if (pComponentPrivate->nCurState == OMX_StateInvalid) {
        eError = OMX_ErrorIncorrectStateOperation;
        goto PRINT_EXIT;
    }

    switch (nParamIndex) {
#ifdef KHRONOS_1_1
        case OMX_IndexParamAudioInit:
            memcpy(ComponentParameterStructure, pComponentPrivate->pAudioPortType, sizeof(OMX_PORT_PARAM_TYPE));
            OMX_MEMCPY_CHECK(ComponentParameterStructure);
            
            break;

        case OMX_IndexParamVideoInit:
            memcpy(ComponentParameterStructure, pComponentPrivate->pVideoPortType, sizeof(OMX_PORT_PARAM_TYPE));
            OMX_MEMCPY_CHECK(ComponentParameterStructure);
                    
            break;

        case OMX_IndexParamOtherInit:
            memcpy(ComponentParameterStructure, pComponentPrivate->pOtherPortType, sizeof(OMX_PORT_PARAM_TYPE));
            OMX_MEMCPY_CHECK(ComponentParameterStructure);
                    
            break;
#endif
    case OMX_IndexParamImageInit:
        memcpy(ComponentParameterStructure, pComponentPrivate->pPortParamType, sizeof(OMX_PORT_PARAM_TYPE));
        OMX_MEMCPY_CHECK(ComponentParameterStructure);
        break;

    case OMX_IndexParamPortDefinition:
		OMX_PRINT1(pComponentPrivate->dbg, "get param %lu\n", ((OMX_PARAM_PORTDEFINITIONTYPE *)(ComponentParameterStructure))->nPortIndex);
		OMX_PRINT1(pComponentPrivate->dbg, " port 0 enable: %d\n", pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pPortDef->bEnabled);
        OMX_PRINT1(pComponentPrivate->dbg, " port 1 enable: %d\n", pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pPortDef->bEnabled);
       if (((OMX_PARAM_PORTDEFINITIONTYPE *)(ComponentParameterStructure))->nPortIndex == pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pPortDef->nPortIndex) {
            memcpy(ComponentParameterStructure, pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pPortDef, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
        }
        else if (((OMX_PARAM_PORTDEFINITIONTYPE *)(ComponentParameterStructure))->nPortIndex == pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pPortDef->nPortIndex) {
            memcpy(ComponentParameterStructure, pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pPortDef, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
        }
        else {
            eError = OMX_ErrorBadPortIndex;
        }
        break;

    case OMX_IndexParamImagePortFormat:
        if (((OMX_IMAGE_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nPortIndex ==  pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pPortFormat->nPortIndex) {
            if (((OMX_IMAGE_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nIndex > pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pPortFormat->nIndex) {
                eError = OMX_ErrorNoMore;
            }
            else {
                memcpy(ComponentParameterStructure, pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pPortFormat, sizeof(OMX_IMAGE_PARAM_PORTFORMATTYPE));
            }
        }
        else if (((OMX_IMAGE_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nPortIndex == pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pPortFormat->nPortIndex) {
            if (((OMX_IMAGE_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nIndex > pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pPortFormat->nIndex) {
                eError = OMX_ErrorNoMore;
            }
            else {
                memcpy(ComponentParameterStructure, pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pPortFormat, sizeof(OMX_IMAGE_PARAM_PORTFORMATTYPE));

            }
        }
        else {
            eError = OMX_ErrorBadPortIndex;
        }
        break;

    case OMX_IndexParamPriorityMgmt:
        memcpy(ComponentParameterStructure, pComponentPrivate->pPriorityMgmt, sizeof(OMX_PRIORITYMGMTTYPE));
        break;

    case OMX_IndexParamCompBufferSupplier:
    {
        OMX_PARAM_BUFFERSUPPLIERTYPE *pBuffSupplierParam = (OMX_PARAM_BUFFERSUPPLIERTYPE *)ComponentParameterStructure;
        if (pBuffSupplierParam->nPortIndex == 1) {
            pBuffSupplierParam->eBufferSupplier = pComponentPrivate->pCompPort[pBuffSupplierParam->nPortIndex]->pParamBufSupplier->eBufferSupplier;
        }
        else if (pBuffSupplierParam->nPortIndex == 0) {
            pBuffSupplierParam->eBufferSupplier = pComponentPrivate->pCompPort[pBuffSupplierParam->nPortIndex]->pParamBufSupplier->eBufferSupplier;
        }
        else {
            eError = OMX_ErrorBadPortIndex;
        }
        break;
    }

#ifdef KHRONOS_1_1

    case OMX_IndexParamQuantizationTable:
            {
                memcpy(ComponentParameterStructure, pComponentPrivate->pQuantTable, sizeof(OMX_IMAGE_PARAM_QUANTIZATIONTABLETYPE));
                OMX_MEMCPY_CHECK(ComponentParameterStructure);
        }
    break;

    case OMX_IndexParamHuffmanTable:
                    {
                memcpy(ComponentParameterStructure, pComponentPrivate->pHuffmanTable, sizeof(OMX_IMAGE_PARAM_HUFFMANTTABLETYPE));
                OMX_MEMCPY_CHECK(ComponentParameterStructure);
        }
    break;

#endif
    case OMX_IndexCustomSectionDecode:
        {
            memcpy(ComponentParameterStructure, pComponentPrivate->pSectionDecode, sizeof(OMX_CUSTOM_IMAGE_DECODE_SECTION));
            OMX_MEMCPY_CHECK(ComponentParameterStructure);
        }
    break;

    case OMX_IndexCustomSubRegionDecode:
        {
            memcpy(ComponentParameterStructure, pComponentPrivate->pSubRegionDecode, sizeof(OMX_CUSTOM_IMAGE_DECODE_SUBREGION));
            OMX_MEMCPY_CHECK(ComponentParameterStructure);
        }
    break;

	case OMX_IndexCustomSetMaxResolution:
		{
            memcpy(ComponentParameterStructure, &(pComponentPrivate->sMaxResolution), sizeof(OMX_CUSTOM_RESOLUTION));
            OMX_MEMCPY_CHECK(ComponentParameterStructure);
		}
		break;

    default:
        eError = OMX_ErrorUnsupportedIndex;
        break;
    }

PRINT_EXIT:
    OMX_PRINT1(pComponentPrivate->dbg, "EXIT\n");
EXIT:
    return eError;
}   /* End of GetParameter_JPEGDec */



/* ========================================================================== */
/**
 * @fn SetParameter_JPEGDec - Sets the parameters sent by the Application and
 *  sets it to the component
 * @param hComponent - handle for this instance of the component
 * @param nParamIndex - Component Index Port
 * @param ComponentParameterStructure - Component Parameter Structure to set
 * @return: OMX_ERRORTYPE
 *          OMX_ErrorNone on success
 *          !OMX_ErrorNone on any failure
 */
/* ========================================================================== */
static OMX_ERRORTYPE SetParameter_JPEGDec(OMX_HANDLETYPE hComponent,
                                          OMX_INDEXTYPE nParamIndex,
                                          OMX_PTR pCompParam)
{
    OMX_COMPONENTTYPE* pHandle = NULL;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    JPEGDEC_COMPONENT_PRIVATE * pComponentPrivate = NULL;

    OMX_CHECK_PARAM(hComponent);
    OMX_CHECK_PARAM(pCompParam);

    pHandle = (OMX_COMPONENTTYPE*)hComponent;
    pComponentPrivate = (JPEGDEC_COMPONENT_PRIVATE*)pHandle->pComponentPrivate;

    JPEGDEC_OMX_CONF_CHECK_CMD(pComponentPrivate, 1, 1);

    OMX_PRINT1(pComponentPrivate->dbg, "Entering funtion SetParameter_JPEGDec\n");

    if (pComponentPrivate->nCurState != OMX_StateLoaded) {
        eError = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }

    switch (nParamIndex) {
#ifdef KHRONOS_1_1
    case OMX_IndexParamAudioPortFormat:
        {
            OMX_AUDIO_PARAM_PORTFORMATTYPE* pComponentParam = (OMX_AUDIO_PARAM_PORTFORMATTYPE *)pCompParam;
            if ( pComponentParam->nPortIndex == pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pPortFormat->nPortIndex ) {
                memcpy(pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pPortFormat, pComponentParam, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
                OMX_MEMCPY_CHECK(pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pPortFormat);
            } 
            else if ( pComponentParam->nPortIndex == pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pPortFormat->nPortIndex ) {
                memcpy(pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pPortFormat, pComponentParam, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
                OMX_MEMCPY_CHECK(pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pPortFormat);
            } 
            else {
                eError = OMX_ErrorBadPortIndex;
            }

            break;
        }

    case OMX_IndexParamVideoPortFormat:
        {
            OMX_VIDEO_PARAM_PORTFORMATTYPE* pComponentParam = (OMX_VIDEO_PARAM_PORTFORMATTYPE *)pCompParam;
            if ( pComponentParam->nPortIndex == pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pPortFormat->nPortIndex ) {
                memcpy(pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pPortFormat, pComponentParam, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
                OMX_MEMCPY_CHECK(pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pPortFormat);
            } 
            else if ( pComponentParam->nPortIndex == pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pPortFormat->nPortIndex ) {
                memcpy(pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pPortFormat, pComponentParam, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
                OMX_MEMCPY_CHECK(pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pPortFormat);
            } 
            else {
                eError = OMX_ErrorBadPortIndex;
            }

            break;
        }

    case OMX_IndexParamOtherPortFormat:
        {
            OMX_OTHER_PARAM_PORTFORMATTYPE* pComponentParam = (OMX_OTHER_PARAM_PORTFORMATTYPE *)pCompParam;
            if ( pComponentParam->nPortIndex == pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pPortFormat->nPortIndex ) {
                memcpy(pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pPortFormat, pComponentParam, sizeof(OMX_OTHER_PARAM_PORTFORMATTYPE));
                OMX_MEMCPY_CHECK(pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pPortFormat);
            } 
            else if ( pComponentParam->nPortIndex == pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pPortFormat->nPortIndex ) {
                memcpy(pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pPortFormat, pComponentParam, sizeof(OMX_OTHER_PARAM_PORTFORMATTYPE));
                OMX_MEMCPY_CHECK(pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pPortFormat);
            } 
            else {
                eError = OMX_ErrorBadPortIndex;
            }

            break;
        }
#endif
    case OMX_IndexParamImagePortFormat:
    {
        OMX_IMAGE_PARAM_PORTFORMATTYPE* pComponentParam = (OMX_IMAGE_PARAM_PORTFORMATTYPE *)pCompParam;
        if (pComponentParam->nPortIndex == pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pPortFormat->nPortIndex) {
            OMX_MEMCPY_CHECK(pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pPortFormat);
            memcpy(pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pPortFormat, pComponentParam, sizeof(OMX_IMAGE_PARAM_PORTFORMATTYPE));
        }
        else if (pComponentParam->nPortIndex == pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pPortFormat->nPortIndex) {
            OMX_MEMCPY_CHECK(pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pPortFormat);
            memcpy(pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pPortFormat, pComponentParam, sizeof(OMX_IMAGE_PARAM_PORTFORMATTYPE));
        }
        else {
            eError = OMX_ErrorBadPortIndex;
        }
        break;
    }

    case OMX_IndexParamImageInit:
        OMX_MEMCPY_CHECK(pComponentPrivate->pPortParamType);
        memcpy(pComponentPrivate->pPortParamType, (OMX_PORT_PARAM_TYPE*)pCompParam, sizeof(OMX_PORT_PARAM_TYPE));
        break;

    case OMX_IndexParamPortDefinition:
    {
        OMX_PARAM_PORTDEFINITIONTYPE* pComponentParam = (OMX_PARAM_PORTDEFINITIONTYPE *)pCompParam;
        if (pComponentParam->nPortIndex == pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pPortDef->nPortIndex) {
            OMX_MEMCPY_CHECK(pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pPortDef);
            memcpy(pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pPortDef, pComponentParam, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
        }
        else if (pComponentParam->nPortIndex == pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pPortDef->nPortIndex) {
            OMX_MEMCPY_CHECK(pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pPortDef);
            memcpy(pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pPortDef, pComponentParam, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
        }
        else {
            eError = OMX_ErrorBadPortIndex;
        }
        break;
    }

    case OMX_IndexParamPriorityMgmt:
        OMX_MEMCPY_CHECK(pComponentPrivate->pPriorityMgmt);
        memcpy(pComponentPrivate->pPriorityMgmt, (OMX_PRIORITYMGMTTYPE*)pCompParam, sizeof(OMX_PRIORITYMGMTTYPE));
        break;

    case OMX_IndexParamCompBufferSupplier:
    {
        OMX_PARAM_BUFFERSUPPLIERTYPE *pBuffSupplierParam = (OMX_PARAM_BUFFERSUPPLIERTYPE *)pCompParam;

        if (pBuffSupplierParam->nPortIndex == 1)    {
            /* Copy parameters to input port buffer supplier type */
            pComponentPrivate->pCompPort[pBuffSupplierParam->nPortIndex]->pParamBufSupplier->eBufferSupplier= pBuffSupplierParam->eBufferSupplier;
        }
        else if (pBuffSupplierParam->nPortIndex == 0) {
            pComponentPrivate->pCompPort[pBuffSupplierParam->nPortIndex]->pParamBufSupplier->eBufferSupplier = pBuffSupplierParam->eBufferSupplier;
        }
        else {
            eError = OMX_ErrorBadPortIndex;
            break;
        }
        break;
    }
#ifdef KHRONOS_1_1

    case OMX_IndexParamQuantizationTable:
            {
                OMX_IMAGE_PARAM_QUANTIZATIONTABLETYPE* pQuantizationTable = (OMX_IMAGE_PARAM_QUANTIZATIONTABLETYPE*)pCompParam;
                OMX_MEMCPY_CHECK(pQuantizationTable);
                OMX_MEMCPY_CHECK(pComponentPrivate->pQuantTable);
                memcpy(pComponentPrivate->pQuantTable, pQuantizationTable, sizeof(OMX_IMAGE_PARAM_QUANTIZATIONTABLETYPE));
        }
    break;

    case OMX_IndexParamStandardComponentRole:
            {
                OMX_PARAM_COMPONENTROLETYPE* pRole = (OMX_PARAM_COMPONENTROLETYPE *)pCompParam;
                OMX_MEMCPY_CHECK(pRole);
                OMX_MEMCPY_CHECK(pComponentPrivate->pCompRole);
                memcpy(pComponentPrivate->pCompRole, pRole, sizeof(OMX_PARAM_COMPONENTROLETYPE ));
        }
    break;
        

    case OMX_IndexParamHuffmanTable:
                    {
                OMX_IMAGE_PARAM_HUFFMANTTABLETYPE* pHuffmanTable = (OMX_IMAGE_PARAM_HUFFMANTTABLETYPE *)pCompParam;
                OMX_MEMCPY_CHECK(pHuffmanTable);
                OMX_MEMCPY_CHECK(pComponentPrivate->pHuffmanTable);
                memcpy(pComponentPrivate->pHuffmanTable, pHuffmanTable, sizeof(OMX_IMAGE_PARAM_HUFFMANTTABLETYPE ));
        }
    break;

#endif

    case OMX_IndexCustomSectionDecode:
        {
            OMX_CUSTOM_IMAGE_DECODE_SECTION* pSectionDecode = pCompParam;
            OMX_MEMCPY_CHECK(pSectionDecode);
            OMX_MEMCPY_CHECK(pComponentPrivate->pSectionDecode);
            memcpy(pComponentPrivate->pSectionDecode, pSectionDecode, sizeof(OMX_CUSTOM_IMAGE_DECODE_SECTION));
        }
    break;

    case OMX_IndexCustomSubRegionDecode:
        {
            OMX_CUSTOM_IMAGE_DECODE_SUBREGION* pSubRegionDecode = pCompParam;
            OMX_MEMCPY_CHECK(pSubRegionDecode);
            OMX_MEMCPY_CHECK(pComponentPrivate->pSubRegionDecode);
            memcpy(pComponentPrivate->pSubRegionDecode, pSubRegionDecode, sizeof(OMX_CUSTOM_IMAGE_DECODE_SUBREGION));
        }
    break;
    
	case OMX_IndexCustomSetMaxResolution:
		{
			OMX_MEMCPY_CHECK(pCompParam);
            memcpy(&(pComponentPrivate->sMaxResolution), pCompParam, sizeof(OMX_CUSTOM_RESOLUTION));
		}
		break;

    
    default:
        eError = OMX_ErrorUnsupportedIndex;
        break;
    }

EXIT:
    return eError;
}   /* End of SetParameter_JPEGDec */



/* ========================================================================== */
/**
 * @fn GetConfig_JPEGDec - Gets Configuration data from the Component
 * @param hComp - handle for this instance of the component
 * @param nConfigIndex - Component Config Index Port
 * @param ComponentConfigStructure - Component Config Structure
 * @return: OMX_ERRORTYPE
 *          OMX_ErrorNone on success
 *          !OMX_ErrorNone on any failure
 */
/* ========================================================================== */
static OMX_ERRORTYPE GetConfig_JPEGDec(OMX_HANDLETYPE hComp,
                                       OMX_INDEXTYPE nConfigIndex,
                                       OMX_PTR ComponentConfigStructure)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE* pHandle = NULL;
    JPEGDEC_COMPONENT_PRIVATE * pComponentPrivate = NULL;

    OMX_CHECK_PARAM(hComp);
    OMX_CHECK_PARAM(ComponentConfigStructure);
    pHandle = (OMX_COMPONENTTYPE*)hComp;
    pComponentPrivate = (JPEGDEC_COMPONENT_PRIVATE*)pHandle->pComponentPrivate;

    JPEGDEC_OMX_CONF_CHECK_CMD(pComponentPrivate, 1, 1); 

    OMX_PRINT1(pComponentPrivate->dbg, "Entering funtion GetConfig_JPEGDec\n"); 

    switch (nConfigIndex) {

    case OMX_IndexConfigCommonScale:
    {
        int scale=0;
        OMX_CONFIG_SCALEFACTORTYPE* pScaleFactor = (OMX_CONFIG_SCALEFACTORTYPE *)ComponentConfigStructure;
        memcpy(pScaleFactor, pComponentPrivate->pScalePrivate, sizeof(OMX_CONFIG_SCALEFACTORTYPE));

        scale =(int)pScaleFactor->xWidth;

        switch (scale)
        {
        case (0):
            pScaleFactor->xWidth = 100;
            break;

        case (1):
            pScaleFactor->xWidth = 50;
            break;

        case (2):
            pScaleFactor->xWidth = 25;
            break;

        case (3):
            pScaleFactor->xWidth = 12;
            break;

        case (4):
            pScaleFactor->xWidth = 200;
            break;

        case (5):
            pScaleFactor->xWidth = 400;
            break;

        case (6):
            pScaleFactor->xWidth = 800;
            break;

        default:
            pScaleFactor->xWidth = 100;
            break;
        }
        break;
    }

    case OMX_IndexCustomProgressiveFactor:
    {
        int *pnProgressiveFlag = (int*)ComponentConfigStructure;
        pnProgressiveFlag = (int*)pComponentPrivate->nProgressive;
        break;
    }

    case OMX_IndexCustomDebug: 
    {
	OMX_DBG_GETCONFIG(pComponentPrivate->dbg, ComponentConfigStructure);
	break;
    }

    default:
        eError = OMX_ErrorUnsupportedIndex;
        break;
    }

EXIT:
    return eError;
}   /* End of GetConfig_JPEGDec */



/* ========================================================================== */
/**
 * @fn SetConfig_JPEGDec - Sends command to set new configuration
 * @param hComp - handle for this instance of the component
 * @param nConfigIndex - Component Config Index Port
 * @param ComponentConfigStructure - Component Config Structure
 * @return: OMX_ERRORTYPE
 *          OMX_ErrorNone on success
 *          !OMX_ErrorNone on any failure
 */
/* ========================================================================== */
static OMX_ERRORTYPE SetConfig_JPEGDec(OMX_HANDLETYPE hComp,
                                       OMX_INDEXTYPE nConfigIndex,
                                       OMX_PTR ComponentConfigStructure)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE* pHandle = NULL;
    JPEGDEC_COMPONENT_PRIVATE * pComponentPrivate = NULL;

    OMX_CHECK_PARAM(hComp);
    OMX_CHECK_PARAM(ComponentConfigStructure);
    pHandle = (OMX_COMPONENTTYPE*)hComp;
    pComponentPrivate = (JPEGDEC_COMPONENT_PRIVATE*)pHandle->pComponentPrivate;

    JPEGDEC_OMX_CONF_CHECK_CMD(pComponentPrivate, 1, 1); 

    OMX_PRINT1(pComponentPrivate->dbg, "Entering funtion SetConfig_JPEGDec\n"); 

    switch (nConfigIndex) {

    case OMX_IndexConfigCommonScale:
    {
        int scale=0;
        OMX_CONFIG_SCALEFACTORTYPE* pScaleFactor = (OMX_CONFIG_SCALEFACTORTYPE *)ComponentConfigStructure;
        scale =(int)pScaleFactor->xWidth;

        switch (scale)
        {
        case (800):
            pScaleFactor->xWidth = 6;
            break;
            
        case (400):
            pScaleFactor->xWidth = 5;
            break;
            
        case (200):
            pScaleFactor->xWidth = 4;
            break;
            
        case (100):
            pScaleFactor->xWidth = 0;
            break;

        case (50):
            pScaleFactor->xWidth = 1;
            break;

        case (25):
            pScaleFactor->xWidth = 2;
            break;

        case (12):
            pScaleFactor->xWidth = 3;
            break;

        default:
            pScaleFactor->xWidth = 0;
            break;
        }

        OMX_MEMCPY_CHECK(pComponentPrivate->pScalePrivate);
        memcpy(pComponentPrivate->pScalePrivate, pScaleFactor, sizeof(OMX_CONFIG_SCALEFACTORTYPE));
        break;
    }

    case OMX_IndexCustomProgressiveFactor:
    {
        int *pnProgressive = (int*)ComponentConfigStructure;
        pComponentPrivate->nProgressive = (int)*pnProgressive;
        break;
    }

    case OMX_IndexCustomInputFrameWidth:
    {
        int *pnInputFrameWidth = (int*)ComponentConfigStructure;
        pComponentPrivate->nInputFrameWidth = (int)*pnInputFrameWidth;
        break;
    }

    case OMX_IndexCustomOutputColorFormat:
    {
        int *pnOutputColorFormat = (int*)ComponentConfigStructure;
        pComponentPrivate->nOutputColorFormat = (int)*pnOutputColorFormat;
        break;
    }

    case OMX_IndexCustomDebug:
    {
	OMX_DBG_SETCONFIG(pComponentPrivate->dbg, ComponentConfigStructure);
	break;
    }
    default:
        eError = OMX_ErrorUnsupportedIndex;
        break;
    }
EXIT:
    return eError;
}   /* End of SetConfig_JPEGDec */



/* ========================================================================== */
/**
 * @fn GetState - Gets OMX Component State
 * @param hComp - handle for this instance of the component
 * @param pState - pointer to store State
 * @return: OMX_ERRORTYPE
 *          OMX_ErrorNone on success
 *          !OMX_ErrorNone on any failure
 */
/* ========================================================================== */
static OMX_ERRORTYPE GetState(OMX_HANDLETYPE pComponent,
                              OMX_STATETYPE* pState)
{
    OMX_ERRORTYPE eError = 80001012;
    OMX_COMPONENTTYPE *pHandle = NULL;
    OMX_CHECK_PARAM(pComponent);
    pHandle = (OMX_COMPONENTTYPE *)pComponent;
    JPEGDEC_COMPONENT_PRIVATE * pComponentPrivate = NULL;
    pComponentPrivate = (JPEGDEC_COMPONENT_PRIVATE*)pHandle->pComponentPrivate; 

    JPEGDEC_OMX_CONF_CHECK_CMD(pComponentPrivate, 1, 1);  

    OMX_PRINT1(pComponentPrivate->dbg, "Entering funtion GetState\n"); 

    if (!pState) {
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if (pHandle && pHandle->pComponentPrivate) {
        *pState = ((JPEGDEC_COMPONENT_PRIVATE*)pHandle->pComponentPrivate)->nCurState;
    }
    else {
        *pState = OMX_StateLoaded;
    }

    eError = OMX_ErrorNone;

EXIT:
    return eError;
}   /* End of GetState */


/* ========================================================================== */
/**
 * @fn EmptyThisBuffer - Send Input Buffers.   * The application uses this to
 *  send the input buffers filled with data to the input port of the component.
 * @param hComponent - handle for this instance of the component
 * @param pBuffHead - Pointer to data filled
 * @return: OMX_ERRORTYPE
 *          OMX_ErrorNone on success
 *          !OMX_ErrorNone on any failure
 */
/* ========================================================================== */
static OMX_ERRORTYPE EmptyThisBuffer(OMX_HANDLETYPE hComponent,
                                     OMX_BUFFERHEADERTYPE* pBuffHead)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pHandle = NULL;
    JPEGDEC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    JPEGDEC_BUFFER_PRIVATE* pBuffPrivate;
    int nRet;
    int i;

    OMX_CHECK_PARAM(hComponent);
    pHandle = (OMX_COMPONENTTYPE *)hComponent;
    pComponentPrivate = (JPEGDEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;

    JPEGDEC_OMX_CONF_CHECK_CMD(pComponentPrivate, 1, 1);

    OMX_PRINT1(pComponentPrivate->dbg, "Entering funtion EmptyThisBuffer %p\n", pBuffHead);

    if ( pComponentPrivate->nCurState != OMX_StateExecuting && 
        pComponentPrivate->nCurState != OMX_StatePause && 
        pComponentPrivate->nCurState != OMX_StateIdle) {
        eError= OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }

    if (pBuffHead == NULL) {
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }

    if (pBuffHead->nSize != sizeof(OMX_BUFFERHEADERTYPE)) {
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if (!pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pPortDef->bEnabled) {
        eError = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }

    pBuffPrivate = pBuffHead->pInputPortPrivate;

    OMX_PRBUFFER1(pComponentPrivate->dbg, "pBuffHead->nAllocLen = %d\n",(int)pBuffHead->nAllocLen);
    OMX_PRBUFFER1(pComponentPrivate->dbg, "pBuffHead->nFilledLen = %d\n",(int)pBuffHead->nFilledLen);
    OMX_PRBUFFER1(pComponentPrivate->dbg, "pBuffHead->nFlags = %d\n",(int)pBuffHead->nFlags);

    if ((pBuffHead->nVersion.s.nVersionMajor != 0x1) ||
        (pBuffHead->nVersion.s.nVersionMinor != 0x0) ||
        (pBuffHead->nVersion.s.nRevision != 0x0) ||
        (pBuffHead->nVersion.s.nStep != 0x0)) {

        eError= OMX_ErrorVersionMismatch;
        goto EXIT;
    }

    if (pBuffHead->nInputPortIndex != 0x0) {
        eError= OMX_ErrorBadPortIndex;
        goto EXIT;
    }
    pComponentPrivate->nInPortIn ++;

    if (pBuffHead->nFlags & OMX_BUFFERFLAG_EOS) {
	for (i = 0; i < (int)pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pPortDef->nBufferCountActual; i ++) {
            if (pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->sBufferFlagTrack[i].buffer_id == 0xFFFFFFFF) 
            {
	      OMX_PRBUFFER1(pComponentPrivate->dbg, "record buffer id in array %d\n", i);
              pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->sBufferFlagTrack[i].flag = pBuffHead->nFlags;
              pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->sBufferFlagTrack[i].buffer_id = pComponentPrivate->nInPortIn;
              break;
            }
        }
    }

     /* mark the first buffer from input port after receiving mark buffer command */
     if (pComponentPrivate->nMarkPort == JPEGDEC_INPUT_PORT) {
         if (pComponentPrivate->pMarkData) {
	    OMX_PRBUFFER2(pComponentPrivate->dbg, "get mark buffer command, mark buffer %p\n", pBuffHead);
            pBuffHead->pMarkData = pComponentPrivate->pMarkData;
            pBuffHead->hMarkTargetComponent = pComponentPrivate->hMarkTargetComponent;
            pComponentPrivate->pMarkData = NULL;
            pComponentPrivate->hMarkTargetComponent = NULL;
         }
     }

     /* if a buffer from input port is marked, record this port # in the buffer queue */
     if (pBuffHead->pMarkData) {
	 for (i = 0; i < (int)pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pPortDef->nBufferCountActual; i ++) {
             if (pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->sBufferMarkTrack[i].buffer_id == 0xFFFFFFFF)
             {
                JPEGDEC_BUFFERMARK_TRACK *pMarkTrack;
                pMarkTrack = &(pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->sBufferMarkTrack[i]);
                pMarkTrack->buffer_id = pComponentPrivate->nInPortIn;
                pMarkTrack->pMarkData = pBuffHead->pMarkData;
                pMarkTrack->hMarkTargetComponent = pBuffHead->hMarkTargetComponent;
                OMX_PRBUFFER2(pComponentPrivate->dbg, "mark buffer at ID %lu\n", pComponentPrivate->nInPortIn);
                break;
             }
         }
     }


     pBuffPrivate->bReadFromPipe = OMX_FALSE;
#ifdef __PERF_INSTRUMENTATION__
    PERF_ReceivedFrame(pComponentPrivate->pPERF,
                       pBuffHead->pBuffer,
                       pBuffHead->nFilledLen,
                       PERF_ModuleHLMM);
#endif


    pBuffPrivate->eBufferOwner = JPEGDEC_BUFFER_COMPONENT_IN;

    nRet = write (pComponentPrivate->nFilled_inpBuf_Q[1], &(pBuffHead), sizeof(pBuffHead));
    if (nRet == -1) {
	    OMX_PRCOMM4(pComponentPrivate->dbg, "Error while writing to the nFilled_inpBuf_Q pipe\n");
    }

EXIT:
    return eError;
}   /* End of EmptyThisBuffer */



/* ========================================================================== */
/**
 * @fn FillThisBuffer_JPEGDec - Send Output Buffers. The application uses this
 *  to send the empty output buffers to the output port of the component.
 * @param hComponent - handle for this instance of the component
 * @param pBuffHead - Pointer to buffer header
 * @return: OMX_ERRORTYPE
 *          OMX_ErrorNone on success
 *          !OMX_ErrorNone on any failure
 */
/* ========================================================================== */
static OMX_ERRORTYPE FillThisBuffer_JPEGDec(OMX_HANDLETYPE pComponent,
                                            OMX_BUFFERHEADERTYPE* pBuffHead)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pHandle = NULL;
    JPEGDEC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    JPEGDEC_BUFFER_PRIVATE* pBuffPrivate = NULL;
    int nRet;

    if (pBuffHead == NULL) {
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }

    OMX_CHECK_PARAM(pComponent);
    pHandle = (OMX_COMPONENTTYPE *)pComponent;
    pComponentPrivate = (JPEGDEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    pBuffPrivate = pBuffHead->pOutputPortPrivate;

    JPEGDEC_OMX_CONF_CHECK_CMD(pComponentPrivate, 1, 1);

    OMX_PRINT1(pComponentPrivate->dbg, "Entering funtion FillThisBuffer_JPEGDec\n"); 

    if ( pBuffHead->nSize != sizeof(OMX_BUFFERHEADERTYPE) ) {

        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }

    if (pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pPortDef->bEnabled == OMX_FALSE) {
        eError = OMX_ErrorIncorrectStateOperation;
        OMX_PRSTATE4(pComponentPrivate->dbg, "eError = OMX_ErrorIncorrectStateOperation\n");
        goto EXIT;
        }



    if ((pBuffHead->nVersion.s.nVersionMajor != 0x1) ||
        (pBuffHead->nVersion.s.nVersionMinor != 0x0) ||
        (pBuffHead->nVersion.s.nRevision != 0x0) ||
        (pBuffHead->nVersion.s.nStep != 0x0)) {

        eError= OMX_ErrorVersionMismatch;
        goto EXIT;
    }

    OMX_PRBUFFER1(pComponentPrivate->dbg, "pBuffHead->nOutputPortIndex %lu\n", pBuffHead->nOutputPortIndex);

    if (pBuffHead->nOutputPortIndex != 0x1) {
            eError = OMX_ErrorBadPortIndex;
            goto EXIT;
    }

    pBuffHead->nFilledLen = 0;

    if ( pComponentPrivate->nCurState != OMX_StateExecuting && 
        pComponentPrivate->nCurState != OMX_StatePause &&
        pComponentPrivate->nCurState != OMX_StateIdle) {
        eError= OMX_ErrorIncorrectStateOperation;
        goto EXIT;

    }

#ifdef __PERF_INSTRUMENTATION__
    PERF_ReceivedFrame(pComponentPrivate->pPERF,
                       pBuffHead->pBuffer,
                       pBuffHead->nFilledLen,
                       PERF_ModuleHLMM);
#endif

        pBuffPrivate->bReadFromPipe = OMX_FALSE;
    pBuffPrivate->eBufferOwner = JPEGDEC_BUFFER_COMPONENT_IN;
    nRet = write (pComponentPrivate->nFree_outBuf_Q[1], &(pBuffHead), sizeof (pBuffHead));
    if (nRet == -1) {
	    OMX_PRCOMM4(pComponentPrivate->dbg, "Error while writing in nFree_outBuf_Q pipe\n");
    }

EXIT:
    return eError;
}   /* End of FillThisBuffer_JPEGDec */


/* ========================================================================== */
/**
 * @fn ComponentDeInit - Deinitialize Component. This method will clean all
 *  resources in the component
 * @param hComponent - handle for this instance of the component
 * @return: OMX_ERRORTYPE
 *          OMX_ErrorNone on success
 *          !OMX_ErrorNone on any failure
 */
/* ========================================================================== */
OMX_ERRORTYPE ComponentDeInit(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pHandle = NULL;
    JPEGDEC_COMPONENT_PRIVATE *pComponentPrivate = NULL; 
    struct OMX_TI_Debug dbg;

    OMX_DBG_INIT_BASE(dbg);
    OMX_CHECK_PARAM(hComponent);
    pHandle = (OMX_COMPONENTTYPE *)hComponent;

    pComponentPrivate = (JPEGDEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;          
    JPEGDEC_OMX_CONF_CHECK_CMD(pComponentPrivate, 1, 1);

    dbg = pComponentPrivate->dbg;    

    OMX_PRINT1(dbg, "Entering funtion ComponentDeInit\n");

#ifdef __PERF_INSTRUMENTATION__
    PERF_Boundary(pComponentPrivate->pPERF,
	    PERF_BoundaryComplete | PERF_BoundaryCleanup);
    PERF_Done(pComponentPrivate->pPERF);
#endif

    Free_ComponentResourcesJpegDec(pHandle->pComponentPrivate);

#ifdef RESOURCE_MANAGER_ENABLED
    eError= RMProxy_NewSendCommand(pHandle,  RMProxy_FreeResource, OMX_JPEG_Decoder_COMPONENT, 0, 3456, NULL);
    if (eError != OMX_ErrorNone) {
        OMX_PRMGR4(dbg, "Cannot Free RMProxy Resources\n");                    
    }
    eError = RMProxy_DeinitalizeEx(OMX_COMPONENTTYPE_IMAGE);
    if (eError != OMX_ErrorNone) {
	OMX_PRMGR4(dbg, "Error returned from destroy ResourceManagerProxy thread\n");
    }
#endif

EXIT:
    OMX_PRINT1(dbg, "Error from Component DeInit = %d\n",eError);
    OMX_DBG_CLOSE(dbg);
    return eError;
} /* End of ComponentDeInit */



/* ========================================================================== */
/**
 * @fn VerifyTunnelConnection_JPEGDec - This function verifies the tunnel connection
 * @param pPort - port info
 * @param hTunneledComp - handle of the component to tunnel
 * @param pPortDef - pointer to the port definition.
 * @return: OMX_ERRORTYPE
 *          OMX_ErrorNone on success
 *          !OMX_ErrorNone on any failure
 */
/* ========================================================================== */
static OMX_ERRORTYPE VerifyTunnelConnection_JPEGDec(JPEGDEC_PORT_TYPE *pPort,
                                                    OMX_HANDLETYPE hTunneledComp,
                                                    OMX_PARAM_PORTDEFINITIONTYPE* pPortDef)
{
    /* 1.4 Check if input port is compatible with output port */
    OMX_PARAM_PORTDEFINITIONTYPE sPortDef;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pHandle = NULL;
    JPEGDEC_COMPONENT_PRIVATE *pComponentPrivate = NULL; 
    pHandle = (OMX_COMPONENTTYPE *)hTunneledComp; 
    pComponentPrivate = (JPEGDEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;

    JPEGDEC_OMX_CONF_CHECK_CMD(pComponentPrivate, 1, 1); 
    OMX_PRINT1(pComponentPrivate->dbg, "Entering funtion VerifyTunnelConnection_JPEGDec\n");

    sPortDef.nSize = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
    sPortDef.nVersion.s.nVersionMajor = 0x01;
    sPortDef.nVersion.s.nVersionMinor = 0x00;
    sPortDef.nPortIndex = pPort->nTunnelPort;

    eError = OMX_GetParameter(hTunneledComp, OMX_IndexParamPortDefinition, &sPortDef);

    if (eError != OMX_ErrorNone) {
	    OMX_PRCOMM4(pComponentPrivate->dbg, "Error \n");
        return eError;
    }

    switch (pPortDef->eDomain) {

    case OMX_PortDomainOther:
        if (sPortDef.format.other.eFormat!= pPortDef->format.other.eFormat) {
            pPort->hTunnelComponent = 0;
            pPort->nTunnelPort = 0;
            return OMX_ErrorPortsNotCompatible;
        }
        break;

    case OMX_PortDomainAudio:
        if (sPortDef.format.audio.eEncoding != pPortDef->format.audio.eEncoding) {
            pPort->hTunnelComponent = 0;
            pPort->nTunnelPort = 0;
            return OMX_ErrorPortsNotCompatible;
        }
        break;

    case OMX_PortDomainVideo:
        if (sPortDef.format.video.eCompressionFormat != pPortDef->format.video.eCompressionFormat) {
            pPort->hTunnelComponent = 0;
            pPort->nTunnelPort = 0;
            return OMX_ErrorPortsNotCompatible;
        }
        break;

    case OMX_PortDomainImage:
        if (sPortDef.format.image.eCompressionFormat != pPortDef->format.image.eCompressionFormat) {
            pPort->hTunnelComponent = 0;
            pPort->nTunnelPort = 0;
            return OMX_ErrorPortsNotCompatible;
        }
        break;

    default:
        pPort->hTunnelComponent = 0;
        pPort->nTunnelPort = 0;
        return OMX_ErrorPortsNotCompatible; /* Our current port is not set up correctly */
    }
EXIT:
    return eError;
}   /* End of VerifyTunnelConnection_JPEGDec */



/* ========================================================================== */
/**
 * @fn ComponentTunnelRequest_JPEGDec - Set a tunnel between two OMx components
 * @param hComponent - handle for this instance of the component
 * @param nPort - Index Port
 * @param hTunneledComp - handle for the istance of the tunneled component
 * @param nTunneledPort - index port of the tunneled component
 * @param pTunnelSetup - Tunnel setup type
 * @return: OMX_ERRORTYPE
 *          OMX_ErrorNone on success
 *          !OMX_ErrorNone on any failure
 */
/* ========================================================================== */
static OMX_ERRORTYPE ComponentTunnelRequest_JPEGDec(OMX_IN OMX_HANDLETYPE hComponent,
                                                    OMX_IN OMX_U32 nPort,
                                                    OMX_IN OMX_HANDLETYPE hTunneledComp,
                                                    OMX_IN OMX_U32 nTunneledPort,
                                                    OMX_INOUT OMX_TUNNELSETUPTYPE* pTunnelSetup)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pHandle = NULL;
    JPEGDEC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    OMX_PARAM_BUFFERSUPPLIERTYPE sBufferSupplier;
    JPEGDEC_PORT_TYPE *pPort = NULL;

    OMX_CHECK_PARAM(hComponent);
    pHandle = (OMX_COMPONENTTYPE *)hComponent;
    pComponentPrivate = (JPEGDEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;

    JPEGDEC_OMX_CONF_CHECK_CMD(pComponentPrivate, 1, 1); 

    OMX_PRINT1(pComponentPrivate->dbg, "Entering funtion ComponentTunnelRequest_JPEGDec\n");
    OMX_PRINT1(pComponentPrivate->dbg, "nPort = %lu\n",nPort); 

    pPort = pComponentPrivate->pCompPort[nPort];
    if (pTunnelSetup == NULL || hTunneledComp == 0) {
        /* cancel previous tunnel */
        pPort->hTunnelComponent = 0;
        pPort->nTunnelPort = 0;
        OMX_PRBUFFER2(pComponentPrivate->dbg, "OMX_BufferSupplyUnspecified\n");
        pPort->pParamBufSupplier->eBufferSupplier = OMX_BufferSupplyUnspecified;
    }
    else {
        if (pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pPortDef->eDir != OMX_DirInput && pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pPortDef->eDir != OMX_DirOutput) {
           
	    OMX_PRBUFFER4(pComponentPrivate->dbg, "OMX_ErrorBadParameter\n");
            return OMX_ErrorBadParameter;
        }

        /* Check if the other component is developed by TI */
        if(IsTIOMXComponent(hTunneledComp) != OMX_TRUE) 
        {
	    OMX_PRCOMM4(pComponentPrivate->dbg, "OMX_ErrorTunnelingUnsupported\n");
        eError = OMX_ErrorTunnelingUnsupported;
        goto EXIT;
        }

        pPort->hTunnelComponent = hTunneledComp;
        pPort->nTunnelPort = nTunneledPort;

        if (pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pPortDef && pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pPortDef->nPortIndex == nPort) {
            /* Component is the output (source of data) */
            pTunnelSetup->eSupplier = pPort->pParamBufSupplier->eBufferSupplier;
        }
        else {
            /* Component is the input (sink of data) */
            eError = VerifyTunnelConnection_JPEGDec(pPort, hTunneledComp, pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pPortDef);
            if (OMX_ErrorNone != eError)    {
		OMX_PRCOMM5(pComponentPrivate->dbg, "########### Error !! PP VerifyTunnelConnection_JPEGDec failed\n");
                /* Invalid connection formats. Return eError */
                return OMX_ErrorPortsNotCompatible;
            }
            /* If specified obey output port's preferences. Otherwise choose output */
            pPort->pParamBufSupplier->eBufferSupplier = pTunnelSetup->eSupplier;
            if (OMX_BufferSupplyUnspecified == pPort->pParamBufSupplier->eBufferSupplier) {
                pPort->pParamBufSupplier->eBufferSupplier = pTunnelSetup->eSupplier = OMX_BufferSupplyOutput;
            }

            /* Tell the output port who the supplier is */
            sBufferSupplier.nSize = sizeof(sBufferSupplier);
            sBufferSupplier.nVersion.s.nVersionMajor = 0x01;
            sBufferSupplier.nVersion.s.nVersionMinor = 0x00;
            sBufferSupplier.nPortIndex = nTunneledPort;
            sBufferSupplier.eBufferSupplier = pPort->pParamBufSupplier->eBufferSupplier;
            eError = OMX_SetParameter(hTunneledComp, OMX_IndexParamCompBufferSupplier, &sBufferSupplier);
            eError = OMX_GetParameter(hTunneledComp, OMX_IndexParamCompBufferSupplier, &sBufferSupplier);

            if (sBufferSupplier.eBufferSupplier != pPort->pParamBufSupplier->eBufferSupplier) {

                return OMX_ErrorUndefined;
            }
        }
    }
EXIT:
    return eError;
}   /* End of ComponentTunnelRequest_JPEGDec */


/* ========================================================================== */
/**
 * @fn Allocate_DSPResources_JPEGDec - Allocate the Ialg structure for each port's
 * buffer header.
 * @param hComponent - handle for this instance of the component
 * @param nPort - Index Port
 * @return: OMX_ERRORTYPE
 *          OMX_ErrorNone on success
 *          !OMX_ErrorNone on any failure
 */
/* ========================================================================== */
static OMX_ERRORTYPE Allocate_DSPResources_JPEGDec(OMX_IN JPEGDEC_COMPONENT_PRIVATE *pComponentPrivate,
                                                   OMX_IN OMX_U32 nPortIndex)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    void *pUalgOutParams;
    void *pUalgInpParams;
    OMX_U8* pTemp;
    OMX_U8 nBufferCount = -1;

    JPEGDEC_OMX_CONF_CHECK_CMD(pComponentPrivate, 1, 1);

    OMX_PRINT1(pComponentPrivate->dbg, "Entering funtion Allocate_DSPResources_JPEGDec\n");

    nBufferCount = pComponentPrivate->pCompPort[nPortIndex]->nBuffCount;
    if (nPortIndex == JPEGDEC_INPUT_PORT) {
        OMX_MALLOC(pUalgInpParams, sizeof(JPEGDEC_UAlgInBufParamStruct) + 256);
        pTemp = (OMX_U8*)pUalgInpParams;
        pTemp += 128;
        pUalgInpParams = pTemp;
        (pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pBufferPrivate[nBufferCount]->pUALGParams) = (JPEGDEC_UAlgInBufParamStruct *)(pUalgInpParams);
    }
    else if (nPortIndex == JPEGDEC_OUTPUT_PORT) {
        OMX_MALLOC(pUalgOutParams, sizeof(JPEGDEC_UAlgOutBufParamStruct) + 256);
        pTemp = (OMX_U8*)pUalgOutParams;
        pTemp += 128;
        pUalgOutParams = pTemp;
        (pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pBufferPrivate[nBufferCount]->pUALGParams) = (JPEGDEC_UAlgOutBufParamStruct *)(pUalgOutParams);
    }
    else {
        eError = OMX_ErrorBadPortIndex;
        goto EXIT;
    }

EXIT:
    return eError;
} /* End of Allocate_DSPResources_JPEGDec */




static void JPEGDEC_InitBufferFlagTrack(
    JPEGDEC_COMPONENT_PRIVATE *pComponentPrivate, 
    OMX_U32 nPortIndex)

{
    JPEGDEC_PORT_TYPE *pPortType = NULL;
    int i;
        
     pPortType = pComponentPrivate->pCompPort[nPortIndex];

    /* assume  pPortType->pPortDef->nBufferCountActual <= NUM_OF_BUFFERSJPEG */
     for (i = 0; i < (int)pPortType->pPortDef->nBufferCountActual; i ++) {
        pPortType->sBufferFlagTrack[i].flag = 0;
        pPortType->sBufferFlagTrack[i].buffer_id = 0xFFFFFFFF;
        pPortType->sBufferMarkTrack[i].buffer_id = 0xFFFFFFFF;
        pPortType->sBufferMarkTrack[i].pMarkData = NULL;
    }
}



#ifdef KHRONOS_1_1
/* ========================================================================== */
/**
 * @fn ComponentRoleEnum -
 * @param hComponent - handle for this instance of the component
 * @param cRole - role performed by the component
 * @param nIndex - Index
 * @return: OMX_ERRORTYPE
 *          OMX_ErrorNone on success
 *          !OMX_ErrorNone on any failure
 */
/* ========================================================================== */
static OMX_ERRORTYPE ComponentRoleEnum(OMX_IN OMX_HANDLETYPE hComponent,
                                       OMX_OUT OMX_U8 *cRole,
                                       OMX_IN OMX_U32 nIndex)
{
    JPEGDEC_COMPONENT_PRIVATE *pComponentPrivate;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    pComponentPrivate = (JPEGDEC_COMPONENT_PRIVATE *)(((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);

    if(nIndex == 0){
        strncpy((char *)cRole, (char *)pComponentPrivate->componentRole.cRole, sizeof(OMX_U8) * OMX_MAX_STRINGNAME_SIZE);
    }
    else {
        eError = OMX_ErrorNoMore;
    }

    return eError;
}
#endif



/*-------------------------------------------------------------------*/
/**
  * GetExtensionIndex_JpegDec() 
  *
  * Free a video driver buffer.
  *
  * @retval OMX_ErrorNone                    Successful operation.
  *         OMX_ErrorBadParameter            Invalid operation.    
  *         OMX_ErrorIncorrectStateOperation If called when port is disabled.
  **/
/*-------------------------------------------------------------------*/
OMX_ERRORTYPE GetExtensionIndex_JPEGDec(OMX_IN OMX_HANDLETYPE hComponent, OMX_IN OMX_STRING cParameterName, OMX_OUT OMX_INDEXTYPE* pIndexType)
{
    OMX_U16 nIndex;
    OMX_ERRORTYPE eError = OMX_ErrorUndefined;
    JPEGDEC_CUSTOM_PARAM_DEFINITION sJpegDecCustomParams[] = {
    {"OMX.TI.JPEG.decoder.Config.ProgressiveFactor", OMX_IndexCustomProgressiveFactor},
    {"OMX.TI.JPEG.decoder.Config.InputFrameWidth", OMX_IndexCustomInputFrameWidth},
    {"OMX.TI.JPEG.decoder.Config.OutputColorFormat", OMX_IndexCustomOutputColorFormat},
    {"OMX.TI.JPEG.decoder.Param.SectionDecode", OMX_IndexCustomSectionDecode},
    {"OMX.TI.JPEG.decoder.Param.SubRegionDecode", OMX_IndexCustomSubRegionDecode},
    {"OMX.TI.JPEG.decoder.Param.SetMaxResolution", OMX_IndexCustomSetMaxResolution},
    {"OMX.TI.JPEG.decoder.Debug", OMX_IndexCustomDebug},
    {"",0x0}
    };

    /* Check parameter validity */    
    OMX_CHECK_PARAM(hComponent);
    OMX_CHECK_PARAM(pIndexType);
    *pIndexType = OMX_IndexMax;

    for (nIndex = 0; strlen((const char*)sJpegDecCustomParams[nIndex].cCustomParamName); nIndex++){
        if (!strcmp((const char*)cParameterName, (const char*)(&(sJpegDecCustomParams[nIndex].cCustomParamName)))){
            *pIndexType = sJpegDecCustomParams[nIndex].nCustomParamIndex;
            eError = OMX_ErrorNone;
            break;
        }
    }

    if(*pIndexType == OMX_IndexMax){
         eError = OMX_ErrorUnsupportedIndex;
    }

    
EXIT:
    return eError;
}
