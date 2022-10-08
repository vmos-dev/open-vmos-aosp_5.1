
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
* @file OMX_VPP.c
*
* This file implements OMX Component for VPP that 
* is  compliant with the OMX khronos 1.0.
*
* @path  $(CSLPATH)\
*
* @rev  1.0
*/
/* ---------------------------------------------------------------------------- 
*! 
*! Revision History 
*! ===================================
*! 17-april-2005 mf:  Initial Version. Change required per OMAPSWxxxxxxxxx
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
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <pthread.h>
#include <errno.h>
#endif

#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <dbapi.h>


/*-------program files ----------------------------------------*/
#include <OMX_Component.h>

#include "LCML_DspCodec.h"
#include "OMX_VPP.h"
#include "OMX_VPP_Utils.h"
#include "OMX_VPP_CompThread.h"

#ifdef __PERF_INSTRUMENTATION__
#include "perf.h"
#endif

#ifdef RESOURCE_MANAGER_ENABLED
#include <ResourceManagerProxyAPI.h>
#endif

#define VPP_CONTRAST_MIN     -100
#define VPP_CONTRAST_MAX     100
#define VPP_CONTRAST_OFFSET  100
#define VPP_CONTRAST_FACTOR  64/100
#define VPP_MAX_NAMESIZE     127


#define VPP_NUM_CUSTOM_PARAMS 9



/****************************************************************
*  EXTERNAL REFERENCES NOTE : only use if not found in header file
****************************************************************/
/*--------data declarations -----------------------------------*/


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

/* --------- Globals ------------ */
OMX_STRING cVPPName = "OMX.TI.VPP"; 

static VPP_CUSTOM_PARAM_DEFINITION sVPPCustomParams[VPP_NUM_CUSTOM_PARAMS] = {
    {"OMX.TI.VPP.Param.ZoomFactor", OMX_IndexCustomSetZoomFactor},
    {"OMX.TI.VPP.Param.ZoomLimit", OMX_IndexCustomSetZoomLimit},
    {"OMX.TI.VPP.Param.ZoomSpeed", OMX_IndexCustomSetZoomSpeed},
    {"OMX.TI.VPP.Param.ZoomXoffsetFromCenter16", OMX_IndexCustomSetZoomXoffsetFromCenter16},
    {"OMX.TI.VPP.Param.ZoomYoffsetFromCenter16", OMX_IndexCustomSetZoomYoffsetFromCenter16},
    {"OMX.TI.VPP.Param.FrostedGlassOvly", OMX_IndexCustomSetFrostedGlassOvly},
    {"OMX.TI.VPP.Param.VideoColorRange", OMX_IndexCustomVideoColorRange},
    {"OMX.TI.VPP.Param.RGB4ColorFormat", OMX_IndexCustomRGB4ColorFormat},
    {"OMX.TI.VPP.Config.InputSize",OMX_IndexCustomConfigInputSize}
    };

/*--------function prototypes ---------------------------------*/
static OMX_ERRORTYPE VPP_SetCallbacks (OMX_HANDLETYPE hComp, 
                                       OMX_CALLBACKTYPE* pCallBacks, 
                                       OMX_PTR pAppData);

static OMX_ERRORTYPE VPP_GetComponentVersion (OMX_HANDLETYPE   hComp, 
                                              OMX_STRING       szComponentName, 
                                              OMX_VERSIONTYPE* pComponentVersion,
                                              OMX_VERSIONTYPE* pSpecVersion, 
                                              OMX_UUIDTYPE*    pComponentUUID
                                              );

static OMX_ERRORTYPE VPP_SendCommand (OMX_IN OMX_HANDLETYPE  hComponent,
                                      OMX_IN OMX_COMMANDTYPE Cmd,
                                      OMX_IN OMX_U32         nParam,
                                      OMX_IN OMX_PTR         pCmdData
                                      );

static OMX_ERRORTYPE VPP_GetParameter(OMX_HANDLETYPE hComp, 
                                      OMX_INDEXTYPE nParamIndex, 
                                      OMX_PTR pComponentParameterStructure);

static OMX_ERRORTYPE VPP_SetParameter (OMX_HANDLETYPE hComp, 
                                       OMX_INDEXTYPE nParamIndex, 
                                       OMX_PTR ComponentParameterStructure);

static OMX_ERRORTYPE VPP_GetConfig (OMX_HANDLETYPE hComp, 
                                    OMX_INDEXTYPE nConfigIndex,   
                                    OMX_PTR ComponentConfigStructure);

static OMX_ERRORTYPE VPP_SetConfig (OMX_HANDLETYPE hComp, 
                                    OMX_INDEXTYPE nConfigIndex,   
                                    OMX_PTR ComponentConfigStructure);

static OMX_ERRORTYPE VPP_EmptyThisBuffer (OMX_HANDLETYPE hComp, OMX_BUFFERHEADERTYPE* pBufHdr);

static OMX_ERRORTYPE VPP_FillThisBuffer (OMX_HANDLETYPE hComp,OMX_BUFFERHEADERTYPE* pBufferHdr);

static OMX_ERRORTYPE VPP_GetState (OMX_HANDLETYPE hComp, OMX_STATETYPE* pState);

static OMX_ERRORTYPE VPP_ComponentTunnelRequest (OMX_HANDLETYPE hComp, 
                                                 OMX_U32 nPort, 
                                                 OMX_HANDLETYPE hTunneledComp, 
                                                 OMX_U32 nTunneledPort, 
                                                 OMX_INOUT  OMX_TUNNELSETUPTYPE* pTunnelSetup);

static OMX_ERRORTYPE VPP_ComponentDeInit(OMX_HANDLETYPE pHandle);

static OMX_ERRORTYPE VPP_UseBuffer(OMX_IN OMX_HANDLETYPE hComponent,
                                   OMX_INOUT OMX_BUFFERHEADERTYPE** ppBufferHdr,
                                   OMX_IN OMX_U32 nPortIndex,
                                   OMX_IN OMX_PTR pAppPrivate,
                                   OMX_IN OMX_U32 nSizeBytes,
                                   OMX_IN OMX_U8* pBuffer);


static OMX_ERRORTYPE VPP_AllocateBuffer(OMX_IN OMX_HANDLETYPE hComponent,
                                        OMX_INOUT OMX_BUFFERHEADERTYPE** ppBufferHdr,
                                        OMX_IN OMX_U32 nPortIndex,
                                        OMX_IN OMX_PTR pAppPrivate,
                                        OMX_IN OMX_U32 nSizeBytes);

static OMX_ERRORTYPE VPP_FreeBuffer(OMX_IN  OMX_HANDLETYPE hComponent,
                                    OMX_IN  OMX_U32 nPortIndex,
                                    OMX_IN  OMX_BUFFERHEADERTYPE* pBufHeader);

static OMX_ERRORTYPE VPP_GetExtensionIndex(OMX_IN OMX_HANDLETYPE hComponent, 
                                                OMX_IN OMX_STRING cParameterName, 
                                                OMX_OUT OMX_INDEXTYPE* pIndexType);


#ifdef KHRONOS_1_1
static OMX_ERRORTYPE ComponentRoleEnum(
                OMX_IN OMX_HANDLETYPE hComponent,
                OMX_OUT OMX_U8 *cRole,
                OMX_IN OMX_U32 nIndex);
#endif

/*-------------------------------------------------------------------*/
/**
  * AllocateBuffer() 
  *
  * Allocate a video driver buffer.
  *
  * @retval OMX_ErrorNone                    Successful operation.
  *         OMX_ErrorBadParameter            Invalid operation.    
  *         OMX_ErrorIncorrectStateOperation If called when port is disabled.
  **/
/*-------------------------------------------------------------------*/
OMX_ERRORTYPE VPP_AllocateBuffer(OMX_IN OMX_HANDLETYPE hComponent,
                                OMX_INOUT OMX_BUFFERHEADERTYPE** ppBufferHdr,
                                OMX_IN OMX_U32 nPortIndex,
                                OMX_IN OMX_PTR pAppPrivate,
                                OMX_IN OMX_U32 nSizeBytes)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE*) hComponent;
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef = NULL;
    VPP_COMPONENT_PRIVATE* pComponentPrivate = NULL;
    OMX_BUFFERHEADERTYPE *pBufferHdr = NULL;
    OMX_U8  *pBufferAligned = NULL;
    OMX_U8  *pBufferStart = NULL;
    OMX_U32 nCount = 0;
    OMX_DIRTYPE nDirection = OMX_DirMax;
    OMX_U32 nBufSize;


    OMX_CHECK_CMD(hComponent, ppBufferHdr, OMX_TRUE);
    
    pComponentPrivate = (VPP_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    
    VPP_DPRINT("VPP::Inside the AllocateBuffer portindex =%ld\n",nPortIndex);

    if (nPortIndex == pComponentPrivate->sCompPorts[nPortIndex].pPortDef.nPortIndex) {
        pPortDef = &(pComponentPrivate->sCompPorts[nPortIndex].pPortDef);
    }
    else {
        VPP_DPRINT("OMX_ErrorBadparameter AllocateBuffer!!\n");
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }

    if (pComponentPrivate->sCompPorts[nPortIndex].pPortDef.nBufferSize > nSizeBytes) {
        nBufSize   = pComponentPrivate->sCompPorts[nPortIndex].pPortDef.nBufferSize;
    } else {
        nBufSize = nSizeBytes;
    }
    nDirection = pComponentPrivate->sCompPorts[nPortIndex].pPortDef.eDir;

    nCount  = pComponentPrivate->sCompPorts[nPortIndex].nBufferCount;
    /* Allocate memory for all input buffer headers..
    * This memory pointer will be sent to LCML */
    OMX_MALLOC (pBufferHdr, sizeof(OMX_BUFFERHEADERTYPE));

    pComponentPrivate->sCompPorts[nPortIndex].pVPPBufHeader[nCount].pBufHeader     = pBufferHdr;
    pComponentPrivate->sCompPorts[nPortIndex].pVPPBufHeader[nCount].bSelfAllocated = OMX_TRUE;


    VPP_DPRINT("VPP::%d :: --------- Inside Ip Loop\n",__LINE__);
    VPP_DPRINT ("VPP::Inside the AllocateBuffer --.5   pBufferHdr =%p\n" ,  pBufferHdr);
    pBufferHdr->nSize      = sizeof(OMX_BUFFERHEADERTYPE);


    pBufferHdr->nAllocLen  = nBufSize;
    pBufferHdr->nFilledLen = 0;
    pBufferHdr->nVersion.s.nVersionMajor = VPP_MAJOR_VER;
    pBufferHdr->nVersion.s.nVersionMinor = VPP_MINOR_VER;
    
    /* TO CONDITION FOR INPUT AND OUTPUT PORT */
    VPP_DPRINT ("VPP::Inside the AllocateBuffer --1   pBufferHdr =%p\n" ,  pBufferHdr);
    if (nDirection == OMX_DirInput) {
        pBufferHdr->pInputPortPrivate  = &(pComponentPrivate->sCompPorts[nPortIndex].pPortDef);
        pBufferHdr->pOutputPortPrivate = NULL;
        pBufferHdr->nOutputPortIndex   = OMX_NOPORT;  
        pBufferHdr->nInputPortIndex    = nPortIndex;
        if(nPortIndex == OMX_VPP_INPUT_OVERLAY_PORT){
            /* Allocate buffer for overlay process only one buffer*/
            if(pComponentPrivate->RGBbuffer == NULL){
                OMX_MALLOC(pComponentPrivate->RGBbuffer, nBufSize);
            }
        }
    }
    else {
        pBufferHdr->pOutputPortPrivate = &(pComponentPrivate->sCompPorts[nPortIndex].pPortDef);
        pBufferHdr->pInputPortPrivate  = NULL;
        pBufferHdr->nOutputPortIndex   = nPortIndex;  
        pBufferHdr->nInputPortIndex    = OMX_NOPORT;  
    }
    VPP_DPRINT ("VPP::Inside the AllocateBuffer --2  pBufferHdr =%p\n" ,  pBufferHdr);

    pBufferHdr->pPlatformPrivate = pHandle->pComponentPrivate; 
    pBufferHdr->pAppPrivate      = pAppPrivate;
    pBufferHdr->pMarkData        = NULL;
    pBufferHdr->nTickCount       = 0;
    pBufferHdr->nTimeStamp     = 0;
 
    OMX_MALLOC(pBufferStart, nBufSize + 32 + 256);

    pBufferAligned = pBufferStart;
    while ((((int)pBufferAligned) & 0x1f) != 0)
    {
        pBufferAligned++;  
    }

    VPP_DPRINT ("VPP::Inside the AllocateBuffer pBuffer =%p\n",pBufferHdr);
    VPP_DPRINT ("VPP:: Inside the AllocateBuffer   pBuffer->pBuffer =%p\n" ,  pBufferHdr->pBuffer);
    VPP_DPRINT ("VPP::Inside the AllocateBuffer --3  pBuffer =%p\n",pBufferHdr);

    pBufferAligned            = ((OMX_U8*)pBufferAligned) +128;
    pBufferHdr->pBuffer            = pBufferAligned;
    pComponentPrivate->sCompPorts[nPortIndex].pVPPBufHeader[nCount].pBufferStart = pBufferStart;
    pComponentPrivate->sCompPorts[nPortIndex].nBufferCount++;

#ifdef __PERF_INSTRUMENTATION__
    PERF_ReceivedFrame(pComponentPrivate->pPERF,
                    pBufferHdr->pBuffer,
                    pBufferHdr->nAllocLen,
                    PERF_ModuleMemory);
#endif
    
    VPP_DPRINT ("VPP::Inside the AllocateBuffer ( nBufferCountActual  =%ld nBufferCount =%ld " ,
    pComponentPrivate->sCompPorts[nPortIndex].pPortDef.nBufferCountActual,
    pComponentPrivate->sCompPorts[nPortIndex].nBufferCount);  
    if (pComponentPrivate->sCompPorts[nPortIndex].pPortDef.nBufferCountActual == 
            pComponentPrivate->sCompPorts[nPortIndex].nBufferCount) {
        pPortDef->bPopulated = OMX_TRUE;
        VPP_InitBufferDataPropagation(pComponentPrivate, nPortIndex);
        VPP_DPRINT ("VPP::Inside the AllocateBuffer PORT populated\n" );  
    }

    pComponentPrivate->sCompPorts[nPortIndex].pVPPBufHeader[nCount].eBufferOwner = VPP_BUFFER_CLIENT;
    * ppBufferHdr =  pComponentPrivate->sCompPorts[nPortIndex].pVPPBufHeader[nCount].pBufHeader;
EXIT:
    if(eError != OMX_ErrorNone){
        OMX_FREE(pComponentPrivate->RGBbuffer);
    }
    return eError;
}

/*-------------------------------------------------------------------*/
/**
  * FreeBuffer() 
  *
  * Free buffer allocated for VPP.
  *
  * @retval OMX_ErrorNone                    Successful operation.
  *         OMX_ErrorBadParameter            Invalid operation.    
  *         OMX_ErrorIncorrectStateOperation If called when port is disabled.
  **/
/*-------------------------------------------------------------------*/
OMX_ERRORTYPE VPP_FreeBuffer(OMX_IN  OMX_HANDLETYPE hComponent,
OMX_IN  OMX_U32 nPortIndex,
OMX_IN  OMX_BUFFERHEADERTYPE* pBufHeader)
{
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)hComponent;
    VPP_COMPONENT_PRIVATE *pMyData = NULL;
    OMX_U8  *pBufferStart = NULL;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U32 nCount = 0;

    OMX_CHECK_CMD(hComponent, pBufHeader, OMX_TRUE);

    pMyData = (VPP_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;

    OMX_CHECK_CMD(pMyData, OMX_TRUE, OMX_TRUE);
    if(!((nPortIndex == OMX_VPP_INPUT_PORT) ||
            (nPortIndex == OMX_VPP_INPUT_OVERLAY_PORT) ||
            (nPortIndex == OMX_VPP_RGB_OUTPUT_PORT)||
            (nPortIndex == OMX_VPP_YUV_OUTPUT_PORT))){
        OMX_SET_ERROR_BAIL(eError, OMX_ErrorBadParameter);
    }

    VPP_DPRINT("VPP.c: VPP_FreeBuffer port %d\n", nPortIndex);
    VPP_DPRINT("VPP:: # allocated buffers = %d\n", pMyData->sCompPorts[nPortIndex].nBufferCount);
    eError = VPP_IsValidBuffer(pBufHeader, pMyData, nPortIndex, &nCount);
    if(eError != OMX_ErrorNone){
        goto EXIT;
    }

    pBufferStart = pMyData->sCompPorts[nPortIndex].pVPPBufHeader[nCount].pBufferStart;

    VPP_DPRINT(" Free_ComponentResources --nPortIndex= %d, Header = %p \n", nPortIndex,
        pMyData->sCompPorts[nPortIndex].pVPPBufHeader[nCount].pBufHeader);

    if(pMyData->sCompPorts[nPortIndex].pVPPBufHeader[nCount].bSelfAllocated == OMX_TRUE) {
        VPP_DPRINT ("VPP::%d :: FreeBuffer --1\n",__LINE__);

        if (pBufHeader) {
            if (pBufHeader->pBuffer) {

#ifdef __PERF_INSTRUMENTATION__
                PERF_SendingFrame(pMyData->pPERF,
                                pBufHeader->pBuffer,
                                pBufHeader->nAllocLen,
                                PERF_ModuleMemory);
#endif
               VPP_DPRINT ("VPP::%d :: FreeBuffer --1.5\n",__LINE__);
                OMX_FREE(pBufferStart);
                pBufferStart = NULL;
                pBufHeader->pBuffer = NULL;
                VPP_DPRINT ("VPP::%d :: FreeBuffer --1.6\n",__LINE__);
            }
            OMX_FREE(pBufHeader);
            pBufHeader = NULL;
        }
    }
    else {
        if (pBufHeader) {

#ifdef __PERF_INSTRUMENTATION__
            PERF_SendingFrame(pMyData->pPERF,
                            pBufHeader->pBuffer,
                            pBufHeader->nAllocLen,
                            PERF_ModuleHLMM);
#endif

            OMX_FREE(pBufHeader);
            pBufHeader = NULL;
        }
    }

    pMyData->sCompPorts[nPortIndex].nBufferCount--;
	VPP_DPRINT("nBufferCount %d\n", pMyData->sCompPorts[nPortIndex].nBufferCount);
    if (pMyData->sCompPorts[nPortIndex].nBufferCount == 0) {
		VPP_DPRINT("port %d is unpopulated\n", nPortIndex);
        pMyData->sCompPorts[nPortIndex].pPortDef.bPopulated = OMX_FALSE;

    if (pMyData->sCompPorts[nPortIndex].pPortDef.bEnabled && 
            ((pMyData->curState == OMX_StateIdle && pMyData->toState != OMX_StateLoaded) || 
                pMyData->curState == OMX_StateExecuting || 
                pMyData->curState == OMX_StatePause)) {
					VPP_DPRINT("FreeBuf: cur state %d to %d\n", pMyData->curState, pMyData->toState);
        pMyData->cbInfo.EventHandler (pMyData->pHandle,
                    pMyData->pHandle->pApplicationPrivate, 
                    OMX_EventError, 
                    OMX_ErrorPortUnpopulated, 
                    OMX_TI_ErrorMinor,
                    "port unpopulated");
    }
    
    } 
 
	VPP_DPRINT("nPortIndex %d\n", nPortIndex);
	VPP_DPRINT("pPortDef->bEnabled %d pPortDef->bPopulated %d pMyData->bDisableIncomplete[nPortIndex] %d (%d)\n",
			pMyData->sCompPorts[nPortIndex].pPortDef.bEnabled, 
			pMyData->sCompPorts[nPortIndex].pPortDef.bPopulated, 
			pMyData->bDisableIncomplete[nPortIndex], 
			nPortIndex);


      if ((!pMyData->sCompPorts[nPortIndex].pPortDef.bEnabled) && 
		  (pMyData->sCompPorts[nPortIndex].pPortDef.bPopulated == OMX_FALSE)) {
			  VPP_DPRINT("VPP: %d\n", __LINE__);
		if (pMyData->bDisableIncomplete[nPortIndex] == OMX_TRUE) {
            pMyData->sCompPorts[nPortIndex].pPortDef.bEnabled = OMX_FALSE;
			pMyData->bDisableIncomplete[nPortIndex] = OMX_FALSE;
			VPP_DPRINT("send OMX_CommandPortDisable for port %d\n", nPortIndex);
		    pMyData->cbInfo.EventHandler (pMyData->pHandle,
												pMyData->pHandle->pApplicationPrivate,
												OMX_EventCmdComplete,
												OMX_CommandPortDisable,
												nPortIndex,
												NULL);				
      	}	
      }
	  

EXIT:  
    return eError;
}

/*----------------------------------------------------------------------------*/
/**
  *  UseBuffer() 
  *
  * 
  * 
  *
  * @param 
  * @param 
  * @param 
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  **/
/*----------------------------------------------------------------------------*/
OMX_ERRORTYPE VPP_UseBuffer(OMX_IN OMX_HANDLETYPE hComponent,
                            OMX_INOUT OMX_BUFFERHEADERTYPE** ppBufferHdr,
                            OMX_IN OMX_U32 nPortIndex,
                            OMX_IN OMX_PTR pAppPrivate,
                            OMX_IN OMX_U32 nSizeBytes,
                            OMX_IN OMX_U8* pBuffer)
{

    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)hComponent;
    VPP_COMPONENT_PRIVATE *pMyData = NULL; 
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U32 nCount = 0;
    OMX_BUFFERHEADERTYPE* pBufListObj = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef = NULL;


    OMX_CHECK_CMD(hComponent, ppBufferHdr, pBuffer);

    VPP_DPRINT("VPP::UseBuffer nPortIndex= %lu\n",nPortIndex);

    pMyData = (VPP_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;

    OMX_CHECK_CMD(pMyData, OMX_TRUE, OMX_TRUE);

    if (nPortIndex == pMyData->pInPortFormat->nPortIndex) {
        pPortDef = &(pMyData->sCompPorts[nPortIndex].pPortDef);
    }
    else if (nPortIndex == pMyData->pInPortOverlayFormat->nPortIndex) {
        pPortDef = &(pMyData->sCompPorts[nPortIndex].pPortDef);
    }
    else if (nPortIndex == pMyData->pOutPortRGBFormat->nPortIndex) {
        pPortDef = &(pMyData->sCompPorts[nPortIndex].pPortDef);
    }
    else if (nPortIndex == pMyData->pOutPortYUVFormat->nPortIndex) {
        pPortDef = &(pMyData->sCompPorts[nPortIndex].pPortDef);
    }
    else {
        OMX_SET_ERROR_BAIL(eError, OMX_ErrorBadParameter);
    }

    if (!pPortDef->bEnabled) {
        OMX_SET_ERROR_BAIL(eError, OMX_ErrorIncorrectStateOperation);
    }

    OMX_MALLOC(pBufListObj, sizeof(OMX_BUFFERHEADERTYPE));

    OMX_INIT_STRUCT((pBufListObj), OMX_BUFFERHEADERTYPE);
    pBufListObj->pBuffer          = pBuffer;
    pBufListObj->pPlatformPrivate = NULL;
    pBufListObj->nAllocLen        = nSizeBytes;
    if ((nPortIndex == pMyData->pInPortFormat->nPortIndex) ||
            (nPortIndex == pMyData->pInPortOverlayFormat->nPortIndex)) {
        pBufListObj->nInputPortIndex = nPortIndex;
        if (!pMyData->sCompPorts[nPortIndex].hTunnelComponent) {
            pBufListObj->nOutputPortIndex = OMX_NOPORT;
        } 
        else {
            pBufListObj->nOutputPortIndex = pMyData->sCompPorts[nPortIndex].nTunnelPort;
        }
        pBufListObj->pInputPortPrivate  = &(pMyData->sCompPorts[nPortIndex].pPortDef);
        pBufListObj->pOutputPortPrivate = NULL;

        if(nPortIndex == OMX_VPP_INPUT_OVERLAY_PORT){
            /* Allocate buffer for overlay process only one buffer*/
            if(pMyData->RGBbuffer == NULL){  
                OMX_MALLOC(pMyData->RGBbuffer, nSizeBytes);
            }
        }
    } 
    else if (( nPortIndex == pMyData->pOutPortRGBFormat->nPortIndex) ||
            ( nPortIndex == pMyData->pOutPortYUVFormat->nPortIndex)) {
        pBufListObj->nOutputPortIndex = nPortIndex;
        if (!pMyData->sCompPorts[nPortIndex].hTunnelComponent) {
            pBufListObj->nInputPortIndex = OMX_NOPORT;
        }
        else {
            pBufListObj->nInputPortIndex = pMyData->sCompPorts[nPortIndex].nTunnelPort;
        }
        pBufListObj->pInputPortPrivate = NULL;
        pBufListObj->pOutputPortPrivate = &(pMyData->sCompPorts[nPortIndex].pPortDef);
    }
    nCount = pMyData->sCompPorts[nPortIndex].nBufferCount;
    pMyData->sCompPorts[nPortIndex].pVPPBufHeader[nCount].pBufHeader = pBufListObj;
    pMyData->sCompPorts[nPortIndex].pVPPBufHeader[nCount].bSelfAllocated = OMX_FALSE;

    if (!pMyData->sCompPorts[nPortIndex].hTunnelComponent) {
        pMyData->sCompPorts[nPortIndex].pVPPBufHeader[nCount].eBufferOwner = VPP_BUFFER_CLIENT;
    }
    else{
         pMyData->sCompPorts[nPortIndex].pVPPBufHeader[nCount].eBufferOwner = VPP_BUFFER_CLIENT;
    }

    pMyData->sCompPorts[nPortIndex].nBufferCount++; 
    if (pMyData->sCompPorts[nPortIndex].nBufferCount == pPortDef->nBufferCountActual) {
        pPortDef->bPopulated = OMX_TRUE;
        VPP_InitBufferDataPropagation(pMyData, nPortIndex);
    }
    *ppBufferHdr = pBufListObj;

    VPP_DPRINT("In UseBuffer: pBufferHdr is %p, (int) %p, (out)%p \n", 
        *ppBufferHdr, 
        (pBufListObj->pInputPortPrivate),
        (pBufListObj->pOutputPortPrivate));

    VPP_DPRINT("VPP::Exit UseBuffer with Error=0x%X",eError);

#ifdef __PERF_INSTRUMENTATION__
    PERF_ReceivedFrame(pMyData->pPERF,
                    pBufListObj->pBuffer,
                    pBufListObj->nAllocLen,
                    PERF_ModuleHLMM);
#endif

EXIT: 
    return eError;
}

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
    OMX_ERRORTYPE eError       = OMX_ErrorUndefined;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE*) hComp;
    OMX_U8  colorKey[3]  = {3,5,250}; /*RGB*/
    VPP_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    OMX_U8* pTemp = NULL;

    OMX_CHECK_CMD(hComp, OMX_TRUE, OMX_TRUE);

    LinkedList_Create(&AllocList);

    /*Set the all component function pointer to the handle*/
    pHandle->SetCallbacks           = VPP_SetCallbacks;
    pHandle->GetComponentVersion    = VPP_GetComponentVersion;
    pHandle->SendCommand            = VPP_SendCommand;
    pHandle->GetParameter           = VPP_GetParameter;
    pHandle->SetParameter           = VPP_SetParameter;
    pHandle->GetConfig              = VPP_GetConfig;
    pHandle->SetConfig              = VPP_SetConfig;
    pHandle->GetState               = VPP_GetState;
    pHandle->EmptyThisBuffer        = VPP_EmptyThisBuffer;
    pHandle->FillThisBuffer         = VPP_FillThisBuffer;
    pHandle->ComponentTunnelRequest = VPP_ComponentTunnelRequest;
    pHandle->ComponentDeInit        = VPP_ComponentDeInit;
    pHandle->AllocateBuffer         = VPP_AllocateBuffer;
    pHandle->UseBuffer              = VPP_UseBuffer;
    pHandle->FreeBuffer             = VPP_FreeBuffer;
    pHandle->GetExtensionIndex      = VPP_GetExtensionIndex;
#ifdef KHRONOS_1_1
    pHandle->ComponentRoleEnum      = ComponentRoleEnum;
#endif

    /*Allocate the memory for Component private data area*/
   OMX_MALLOC(pHandle->pComponentPrivate, sizeof(VPP_COMPONENT_PRIVATE));

    ((VPP_COMPONENT_PRIVATE *)pHandle->pComponentPrivate)->pHandle = pHandle;

    pComponentPrivate = (VPP_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    
    /*Allcocating FrameStatus*/
    OMX_MALLOC(pComponentPrivate->pIpFrameStatus, sizeof(GPPToVPPInputFrameStatus) + 256);
    pTemp = ((OMX_U8*)(pComponentPrivate->pIpFrameStatus))+128;
    pComponentPrivate->pIpFrameStatus =  (GPPToVPPInputFrameStatus *)pTemp;
    OMX_MALLOC(pComponentPrivate->pOpYUVFrameStatus, sizeof(GPPToVPPOutputFrameStatus) + 256); 
    pTemp = ((OMX_U8*)(pComponentPrivate->pOpYUVFrameStatus))+128;
    pComponentPrivate->pOpYUVFrameStatus = (GPPToVPPOutputFrameStatus *)pTemp;
    OMX_MALLOC(pComponentPrivate->pOpRGBFrameStatus, sizeof(GPPToVPPOutputFrameStatus) + 256); 
    pTemp = ((OMX_U8*)(pComponentPrivate->pOpRGBFrameStatus))+128;
    pComponentPrivate->pOpRGBFrameStatus = (GPPToVPPOutputFrameStatus *)pTemp;

#ifdef KHRONOS_1_1
    strcpy((char *)pComponentPrivate->componentRole.cRole,"iv_renderer.yuv.overlay");
#endif 
    
    /*Init pIpFrameStatus*/
    /*Frame Width and Height*/
    pComponentPrivate->pIpFrameStatus->ulInWidth             = DEFAULT_WIDTH;
    pComponentPrivate->pIpFrameStatus->ulInHeight            = 220; /*Default value for StdCompRoleTest*/
    pComponentPrivate->pIpFrameStatus->ulCInOffset           = DEFAULT_WIDTH * 220;  /* offset of the C frame in the   *
                                                                    * buffer (equal to zero if there *
                                                                    * is no C frame)                 */
    /* crop */
    pComponentPrivate->pIpFrameStatus->ulInXstart            = 0;          
    pComponentPrivate->pIpFrameStatus->ulInXsize             = 0; /*176 Default value for StdCompRoleTest */
    pComponentPrivate->pIpFrameStatus->ulInYstart            = 0;          
    pComponentPrivate->pIpFrameStatus->ulInYsize             = 0; /* 220 Default value for StdCompRoleTest*/
    
    /* zoom*/
    pComponentPrivate->pIpFrameStatus->ulZoomFactor          = 1 << 10;       
    pComponentPrivate->pIpFrameStatus->ulZoomLimit           = 1 << 10;      
    pComponentPrivate->pIpFrameStatus->ulZoomSpeed           = 0;          
    
    pComponentPrivate->pIpFrameStatus->ulFrostedGlassOvly    = OMX_FALSE;        
    pComponentPrivate->pIpFrameStatus->ulLightChroma         = OMX_TRUE;          
    pComponentPrivate->pIpFrameStatus->ulLockedRatio         = OMX_FALSE;          
    pComponentPrivate->pIpFrameStatus->ulMirror              = OMX_FALSE;      
    pComponentPrivate->pIpFrameStatus->ulRGBRotation         = 0;    
    pComponentPrivate->pIpFrameStatus->ulYUVRotation         = 0;
    
    pComponentPrivate->pIpFrameStatus->ulContrastType        = 0;  
    pComponentPrivate->pIpFrameStatus->ulVideoGain           = 1 << 6;   /*Video Gain (contrast) in VGPOP ranges from 0 to 127, being 64 = Gain 1 (no contrast)*/
    
    pComponentPrivate->pIpFrameStatus->ulXoffsetFromCenter16 = 0;        
    pComponentPrivate->pIpFrameStatus->ulYoffsetFromCenter16 = 0;        
    pComponentPrivate->pIpFrameStatus->ulOutPitch = 0;  /*Not supported at OMX level*/
    pComponentPrivate->pIpFrameStatus->ulAlphaRGB = 0; /*Not supported at OMX level*/
    
    /*Init pComponentPrivate->pOpYUVFrameStatus */
    pComponentPrivate->pOpYUVFrameStatus->ulOutWidth            = DEFAULT_WIDTH;
    pComponentPrivate->pOpYUVFrameStatus->ulOutHeight           = DEFAULT_HEIGHT;
    pComponentPrivate->pOpYUVFrameStatus->ulCOutOffset          = 0;/*  Offset of the C frame in the buffer *
                                                                *   (equal to 0 if there is no C frame)*/

    /*Init pComponentPrivate->pOpRGBFrameStatus */
    pComponentPrivate->pOpRGBFrameStatus->ulOutWidth            = DEFAULT_WIDTH;
    pComponentPrivate->pOpRGBFrameStatus->ulOutHeight           = DEFAULT_HEIGHT;
    pComponentPrivate->pOpRGBFrameStatus->ulCOutOffset          = 0;/*  Offset of the C frame in the buffer *
                                                                *   (equal to 0 if there is no C frame)*/

#ifdef __PERF_INSTRUMENTATION__
    pComponentPrivate->pPERF = PERF_Create(PERF_FOURCC('V','P','P',' '),
                                PERF_ModuleLLMM |
                                PERF_ModuleVideoEncode | PERF_ModuleImageEncode |
                                PERF_ModuleVideoDecode | PERF_ModuleImageDecode);
#endif

    OMX_MALLOC(pComponentPrivate->pInPortFormat, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
    OMX_MALLOC(pComponentPrivate->pInPortOverlayFormat, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
    OMX_MALLOC(pComponentPrivate->pOutPortRGBFormat, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
    OMX_MALLOC(pComponentPrivate->pOutPortYUVFormat, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
    OMX_MALLOC(pComponentPrivate->pPriorityMgmt, sizeof(OMX_PRIORITYMGMTTYPE));
    OMX_MALLOC(pComponentPrivate->pPortParamTypeImage, sizeof(OMX_PORT_PARAM_TYPE));
    OMX_MALLOC(pComponentPrivate->pPortParamTypeAudio, sizeof(OMX_PORT_PARAM_TYPE));
    OMX_MALLOC(pComponentPrivate->pPortParamTypeVideo, sizeof(OMX_PORT_PARAM_TYPE));
    OMX_MALLOC(pComponentPrivate->pPortParamTypeOthers, sizeof(OMX_PORT_PARAM_TYPE));
    OMX_MALLOC(pComponentPrivate->pCrop, sizeof(OMX_CONFIG_RECTTYPE));

    OMX_MALLOC(pComponentPrivate->cComponentName,  VPP_MAX_NAMESIZE + 1);
    strncpy(pComponentPrivate->cComponentName, cVPPName, VPP_MAX_NAMESIZE);
    
    OMX_MALLOC(pComponentPrivate->colorKey, 3 * sizeof(OMX_U8));
    pTemp = memcpy (pComponentPrivate->colorKey, (OMX_U8 *)colorKey,(3 * sizeof(OMX_U8)));
    if(pTemp == NULL){
        eError = OMX_ErrorUndefined;
        goto EXIT;
    }
    
    OMX_MALLOC(pComponentPrivate->tVPPIOConf, sizeof(VPPIOConf)); 
    
    eError=VPP_Initialize_PrivateStruct(pComponentPrivate);
    if (eError != OMX_ErrorNone) {
        VPP_DPRINT ("VPP::Error=0x%X returned from VPP_Initialize_PrivateStruct\n",eError);
        goto EXIT;
    }
  
    /* load the ResourceManagerProxy thread*/
#ifdef RESOURCE_MANAGER_ENABLED
    eError = RMProxy_NewInitalizeEx(OMX_COMPONENTTYPE_VPP);
    if (eError != OMX_ErrorNone) {
        VPP_DPRINT ("VPP::%d::Error 0x%X returned from loading ResourceManagerProxy thread\n", __LINE__,eError);
        goto EXIT;
    }
#endif

    /* start the component thread */      
    eError = VPP_Start_ComponentThread(pHandle);
    if (eError != OMX_ErrorNone) {
        VPP_DPRINT ("VPP::Error=0x%X returned from Start_ComponentThread\n",eError);
        goto EXIT;
    }

    eError = OMX_ErrorNone;
    
EXIT:
    if(eError != OMX_ErrorNone){
        /* LinkedList_DisplayAll(&AllocList); */
        OMX_FREEALL();
        LinkedList_Destroy(&AllocList);
    }
    return eError;
}

/*-------------------------------------------------------------------*/
/**
  *  SetCallbacks() Sets application callbacks to the component
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
static OMX_ERRORTYPE VPP_SetCallbacks (OMX_HANDLETYPE pComponent,
                                        OMX_CALLBACKTYPE* pCallBacks, 
                                        OMX_PTR pAppData)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE*)pComponent;
    VPP_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    OMX_U8 *pTemp = NULL;

    OMX_CHECK_CMD(pComponent, pCallBacks, OMX_TRUE);

    pComponentPrivate = (VPP_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;

    /*Copy the callbacks of the application to the component private */
    pTemp = memcpy (&(pComponentPrivate->cbInfo), pCallBacks, sizeof(OMX_CALLBACKTYPE));
    if(pTemp == NULL){
        eError = OMX_ErrorUndefined;
        goto EXIT;
    }
    /*copy the application private data to component memory*/
    pHandle->pApplicationPrivate = pAppData;
    pComponentPrivate->curState = OMX_StateLoaded;

EXIT:
    return eError;
}

/*-------------------------------------------------------------------*/
/**
  *  GetComponentVersion() Sets application callbacks to the component
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
static OMX_ERRORTYPE VPP_GetComponentVersion (OMX_HANDLETYPE   hComp, 
                                              OMX_STRING       szComponentName, 
                                              OMX_VERSIONTYPE* pComponentVersion,
                                              OMX_VERSIONTYPE* pSpecVersion, 
                                              OMX_UUIDTYPE*    pComponentUUID)
{
    OMX_ERRORTYPE          eError            = OMX_ErrorNone;
    OMX_COMPONENTTYPE    * pHandle           = NULL;
    VPP_COMPONENT_PRIVATE* pComponentPrivate = NULL;
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
    pComponentPrivate = (VPP_COMPONENT_PRIVATE*)pHandle->pComponentPrivate;

    strncpy(szComponentName, pComponentPrivate->cComponentName, VPP_MAX_NAMESIZE);
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
static OMX_ERRORTYPE VPP_SendCommand (OMX_IN OMX_HANDLETYPE phandle, 
                                      OMX_IN OMX_COMMANDTYPE Cmd, 
                                      OMX_IN OMX_U32 nParam,
                                      OMX_IN OMX_PTR pCmdData) 
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    int nRet;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)phandle;
    VPP_COMPONENT_PRIVATE *pComponentPrivate = NULL; 
    OMX_MARKTYPE *pMarkType = NULL; 


    OMX_CHECK_CMD(phandle, OMX_TRUE, OMX_TRUE);
    pComponentPrivate = (VPP_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;

    if ( pComponentPrivate->curState == OMX_StateInvalid ) {
        VPP_DPRINT("VPP::%d :: INVALID ALREADY",__LINE__);
        eError = OMX_ErrorInvalidState;
        goto EXIT;
    }

    switch(Cmd) 
    {
    case OMX_CommandStateSet:
		  pComponentPrivate->toState = nParam;
		VPP_DPRINT("VPP:: OMX_CommandStateSet: tostate %d\n", nParam);
		if (nParam == OMX_StateIdle && pComponentPrivate->curState == OMX_StateExecuting) {
			pComponentPrivate->bIsStopping = OMX_TRUE;
			VPP_DPRINT("VPP:: Is stopping!!\n");
		}
        break;
    case OMX_CommandPortDisable:
        
        if ((nParam >= NUM_OF_VPP_PORTS) && (nParam != OMX_ALL)) { 
            eError = OMX_ErrorBadPortIndex;
            break;
        }
        else if(nParam != OMX_ALL) {  /*If only one port is requested might come from the application, then disable from here to avoid race condition*/
            VPP_DPRINT("set port %d as diabled\n", nParam);
			pComponentPrivate->sCompPorts[nParam].pPortDef.bEnabled=OMX_FALSE;
			if (pComponentPrivate->sCompPorts[nParam].pPortDef.bPopulated) {
			    pComponentPrivate->bDisableIncomplete[nParam] = OMX_TRUE;
			} else {
                pComponentPrivate->bDisableIncomplete[nParam] = OMX_FALSE;
		        pComponentPrivate->cbInfo.EventHandler (pComponentPrivate->pHandle,
												pComponentPrivate->pHandle->pApplicationPrivate,
												OMX_EventCmdComplete,
												OMX_CommandPortDisable,
												nParam,
												NULL);		
			}
		} else { /* nParam == 0xFFFFFFFF */
			int i;
			for (i = 0; i < NUM_OF_VPP_PORTS; i ++) {
				VPP_DPRINT("set port %d as disabled\n", i);
			    pComponentPrivate->sCompPorts[i].pPortDef.bEnabled=OMX_FALSE;
				if (pComponentPrivate->sCompPorts[i].pPortDef.bPopulated) {
				    pComponentPrivate->bDisableIncomplete[i] = OMX_TRUE;
				} else {
                    pComponentPrivate->bDisableIncomplete[i] = OMX_FALSE;
		            pComponentPrivate->cbInfo.EventHandler (pComponentPrivate->pHandle,
												pComponentPrivate->pHandle->pApplicationPrivate,
												OMX_EventCmdComplete,
												OMX_CommandPortDisable,
												i,
												NULL);	
				}
			}
		}
        break;
        
    case OMX_CommandPortEnable:
    case OMX_CommandFlush:
        /*if invalid port, send error, and don't write to any pipe*/
        if ((nParam >= NUM_OF_VPP_PORTS) && (nParam != OMX_ALL)) {
            eError = OMX_ErrorBadPortIndex;
            break;
        }
        break;
    case OMX_CommandMarkBuffer: 
        /* we can only mark buffers on input port */
        if (nParam > 1) {
            eError = OMX_ErrorBadPortIndex;
            break;
        }
        VPP_DPRINT("VPP:: OMX_CommandMarkBuffer\n");
        pMarkType = (OMX_MARKTYPE *)pCmdData;
        pComponentPrivate->pMarkData = pMarkType->pMarkData;
        pComponentPrivate->hMarkTargetComponent = pMarkType->hMarkTargetComponent;
        goto EXIT;


        break;
    case OMX_CommandMax:
        break;
    default:
        eError = OMX_ErrorUndefined;
        break;
    }
    if(eError != OMX_ErrorNone){
        goto EXIT;
    }
    /*Write to the command pipe*/
    nRet = write (pComponentPrivate->cmdPipe[1], &Cmd, sizeof(OMX_COMMANDTYPE));
    if (nRet == -1) {
        eError = OMX_ErrorHardware;
        goto EXIT;
    }

#ifdef __PERF_INSTRUMENTATION__
    PERF_SendingCommand(pComponentPrivate->pPERF,
        Cmd,
        Cmd == OMX_CommandMarkBuffer ? ((OMX_U32) pCmdData) : nParam,
        PERF_ModuleComponent);
#endif

        nRet = write (pComponentPrivate->nCmdDataPipe[1], &nParam, sizeof(OMX_U32));
        if (nRet == -1) {
            eError = OMX_ErrorHardware;
            goto EXIT;
        }
        
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
static OMX_ERRORTYPE VPP_GetParameter (OMX_HANDLETYPE hComp, 
                                       OMX_INDEXTYPE nParamIndex, 
                                       OMX_PTR pComponentParameterStructure)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE* pHandle= (OMX_COMPONENTTYPE*)hComp;
    VPP_COMPONENT_PRIVATE *pComponentPrivate = NULL; 
    OMX_U8 *pTemp = NULL;
    OMX_CHECK_CMD(hComp, pComponentParameterStructure, OMX_TRUE);
    pComponentPrivate = (VPP_COMPONENT_PRIVATE*)pHandle->pComponentPrivate;
    OMX_CHECK_CMD(pComponentPrivate, OMX_TRUE, OMX_TRUE);

    VPP_DPRINT ("VPP::Inside the GetParameter %lu\n",(OMX_U32)nParamIndex);
    if ( pComponentPrivate->curState == OMX_StateInvalid ) {
        OMX_SET_ERROR_BAIL(eError, OMX_ErrorIncorrectStateOperation);
    }
 
    switch(nParamIndex) 
    {
    case OMX_IndexParamImageInit:
        pTemp = memcpy(pComponentParameterStructure, 
                        ((VPP_COMPONENT_PRIVATE*) pHandle->pComponentPrivate)->pPortParamTypeImage, 
                        sizeof(OMX_PORT_PARAM_TYPE));
        if(pTemp == NULL){
            eError = OMX_ErrorUndefined;
            break;
        }
        break;
    case OMX_IndexParamAudioInit:
        pTemp = memcpy(pComponentParameterStructure, 
                        ((VPP_COMPONENT_PRIVATE*) pHandle->pComponentPrivate)->pPortParamTypeAudio, 
                        sizeof(OMX_PORT_PARAM_TYPE));
        if(pTemp == NULL){
            eError = OMX_ErrorUndefined;
            break;
        }
        break;
    case OMX_IndexParamVideoInit:
        pTemp = memcpy(pComponentParameterStructure,
                        ((VPP_COMPONENT_PRIVATE*) pHandle->pComponentPrivate)->pPortParamTypeVideo, 
                        sizeof(OMX_PORT_PARAM_TYPE));
        if(pTemp == NULL){
            eError = OMX_ErrorUndefined;
            break;
        }
        break;
    case OMX_IndexParamOtherInit:
        pTemp = memcpy(pComponentParameterStructure, 
                        ((VPP_COMPONENT_PRIVATE*) pHandle->pComponentPrivate)->pPortParamTypeOthers, 
                        sizeof(OMX_PORT_PARAM_TYPE));
        if(pTemp == NULL){
            eError = OMX_ErrorUndefined;
            break;
        }
        break;
    case OMX_IndexParamPortDefinition:
        {
            OMX_PARAM_PORTDEFINITIONTYPE *pComponentParam =(OMX_PARAM_PORTDEFINITIONTYPE *)pComponentParameterStructure;
            OMX_U32 portindex = pComponentParam->nPortIndex;
            if(portindex >= 0 && portindex < NUM_OF_VPP_PORTS){ /*The validation  should be done in two parts, if the portindex is a wrong number the next validation could generate a segmentation fault*/
				VPP_DPRINT ("VPP::Inside the GetParameter portindex = %d (%d)\n",(int)portindex, pComponentPrivate->sCompPorts[portindex].pPortDef.nPortIndex);
				if(portindex == pComponentPrivate->sCompPorts[portindex].pPortDef.nPortIndex){
	                pTemp = memcpy(pComponentParameterStructure, 
	                    &pComponentPrivate->sCompPorts[portindex].pPortDef, 
	                    sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
	                if(pTemp == NULL){
	                    eError = OMX_ErrorUndefined;
	                    break;
	                }
				}
				else{
					eError = OMX_ErrorBadPortIndex;
				}
            }
            else{
                eError = OMX_ErrorBadPortIndex;
            }
            break;
        }
    case OMX_IndexParamVideoPortFormat:
        {
            OMX_VIDEO_PARAM_PORTFORMATTYPE * pVidFmt = (OMX_VIDEO_PARAM_PORTFORMATTYPE *)pComponentParameterStructure;
            if (pVidFmt->nPortIndex == pComponentPrivate->pInPortFormat->nPortIndex) {
                if(pVidFmt->nIndex > pComponentPrivate->pInPortFormat->nIndex) {
                    eError = OMX_ErrorNoMore;
                }
                else {
                    pTemp = memcpy(pComponentParameterStructure, 
                        pComponentPrivate->pInPortFormat, 
                        sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
                    if(pTemp == NULL){
                        eError = OMX_ErrorUndefined;
                        break;
                    }
                }
            }
            else if(pVidFmt->nPortIndex == pComponentPrivate->pInPortOverlayFormat->nPortIndex) {
                if (pVidFmt->nIndex > pComponentPrivate->pInPortOverlayFormat->nIndex) {
                    eError = OMX_ErrorNoMore;
                }
                else {
                    pTemp = memcpy(pComponentParameterStructure, 
                        pComponentPrivate->pInPortOverlayFormat, 
                        sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
                    if(pTemp == NULL){
                        eError = OMX_ErrorUndefined;
                        break;
                    }
                }
            }
            else if (pVidFmt->nPortIndex == pComponentPrivate->pOutPortRGBFormat->nPortIndex) {
                if (pVidFmt->nIndex > pComponentPrivate->pOutPortRGBFormat->nIndex) {
                    eError = OMX_ErrorNoMore;
                }
                else {
                    pTemp = memcpy(pComponentParameterStructure, 
                        pComponentPrivate->pOutPortRGBFormat, 
                        sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
                    if(pTemp == NULL){
                        eError = OMX_ErrorUndefined;
                        break;
                    }
                }
            }
            else if (pVidFmt->nPortIndex == pComponentPrivate->pOutPortYUVFormat->nPortIndex) {
                if (pVidFmt->nIndex > pComponentPrivate->pOutPortYUVFormat->nIndex) {
                    eError = OMX_ErrorNoMore;
                }
                else {
                    pTemp = memcpy(pComponentParameterStructure, 
                        pComponentPrivate->pOutPortYUVFormat, 
                        sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
                    if(pTemp == NULL){
                        eError = OMX_ErrorUndefined;
                        break;
                    }
                }
            }
            else {
                eError = OMX_ErrorBadPortIndex;
            }
            break;
        }
    case OMX_IndexParamCompBufferSupplier:
        {
            OMX_PARAM_BUFFERSUPPLIERTYPE *pBuffSupplierParam = (OMX_PARAM_BUFFERSUPPLIERTYPE *)pComponentParameterStructure;
            VPP_DPRINT ("VPP::Inside the GetParameter portindex =%d\n" , (int)pBuffSupplierParam->nPortIndex);
            if (pBuffSupplierParam->nPortIndex == OMX_VPP_INPUT_PORT || 
                    pBuffSupplierParam->nPortIndex == OMX_VPP_INPUT_OVERLAY_PORT || 
                    pBuffSupplierParam->nPortIndex == OMX_VPP_RGB_OUTPUT_PORT || 
                    pBuffSupplierParam->nPortIndex == OMX_VPP_YUV_OUTPUT_PORT ) { 

                pBuffSupplierParam->eBufferSupplier = pComponentPrivate->sCompPorts[pBuffSupplierParam->nPortIndex].eSupplierSetting;
            }
            else {
                eError = OMX_ErrorBadPortIndex;
            }
            break;
        }
    case OMX_IndexParamPriorityMgmt:
        pTemp = memcpy(pComponentParameterStructure, 
                pComponentPrivate->pPriorityMgmt, 
                sizeof(OMX_PRIORITYMGMTTYPE));
        if(pTemp == NULL){
            eError = OMX_ErrorUndefined;
            break;
        }
        break;

    default:
        eError = OMX_ErrorUnsupportedIndex;
        break;
    }
EXIT:
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
static OMX_ERRORTYPE VPP_SetParameter (OMX_HANDLETYPE hComp, 
                                       OMX_INDEXTYPE nParamIndex,
                                       OMX_PTR pCompParam)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE* pHandle= (OMX_COMPONENTTYPE*)hComp;
    VPP_COMPONENT_PRIVATE *pComponentPrivate = NULL; 
    OMX_VIDEO_PORTDEFINITIONTYPE *pVidDef = NULL;
    OMX_U8 *pTemp = NULL;
#ifdef KHRONOS_1_1
    OMX_PARAM_COMPONENTROLETYPE  *pRole = NULL;
#endif
    OMX_CHECK_CMD(hComp, pCompParam, OMX_TRUE);

    pComponentPrivate = (VPP_COMPONENT_PRIVATE*)pHandle->pComponentPrivate;
    
    OMX_CHECK_CMD(pComponentPrivate, OMX_TRUE, OMX_TRUE);

    if (pComponentPrivate->curState != OMX_StateLoaded) {
        OMX_SET_ERROR_BAIL(eError, OMX_ErrorIncorrectStateOperation);
    }
    switch (nParamIndex) 
    {
    case OMX_IndexParamVideoPortFormat:
        {
            OMX_VIDEO_PARAM_PORTFORMATTYPE* pComponentParam = (OMX_VIDEO_PARAM_PORTFORMATTYPE *)pCompParam;
            if (pComponentParam->nPortIndex == pComponentPrivate->pInPortFormat->nPortIndex) {
                pTemp = memcpy(pComponentPrivate->pInPortFormat, 
                                            pComponentParam, 
                                            sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
                if(pTemp == NULL){
                    eError = OMX_ErrorUndefined;
                    break;
                }
                
            }
            else if (pComponentParam->nPortIndex == pComponentPrivate->pInPortOverlayFormat->nPortIndex) {
                pTemp = memcpy(pComponentPrivate->pInPortOverlayFormat, 
                                            pComponentParam, 
                                            sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
                if(pTemp == NULL){
                    eError = OMX_ErrorUndefined;
                    break;
                }
            }
            else if (pComponentParam->nPortIndex == pComponentPrivate->pOutPortRGBFormat->nPortIndex) {
                pTemp = memcpy(pComponentPrivate->pOutPortRGBFormat, 
                                            pComponentParam, 
                                            sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
                if(pTemp == NULL){
                    eError = OMX_ErrorUndefined;
                    goto EXIT;
                }
            }
            else if (pComponentParam->nPortIndex == pComponentPrivate->pOutPortYUVFormat->nPortIndex) {
                pTemp = memcpy(pComponentPrivate->pOutPortYUVFormat, 
                                            pComponentParam, 
                                            sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
                if(pTemp == NULL){
                    eError = OMX_ErrorUndefined;
                    break;
                }
            }
            else {
                eError = OMX_ErrorBadPortIndex;
            }
            break;
        }
    case OMX_IndexParamVideoInit:
        pTemp = memcpy(((VPP_COMPONENT_PRIVATE*) pHandle->pComponentPrivate)->pPortParamTypeVideo, 
                pCompParam, 
                sizeof(OMX_PORT_PARAM_TYPE));
        if(pTemp == NULL){
            eError = OMX_ErrorUndefined;
            break;
        }
        break;
    case OMX_IndexParamPortDefinition:
        {
            OMX_PARAM_PORTDEFINITIONTYPE *pComponentParam = (OMX_PARAM_PORTDEFINITIONTYPE *)pCompParam;
            OMX_U32 portIndex = pComponentParam->nPortIndex;
            if ((portIndex == OMX_VPP_INPUT_PORT) || (portIndex == OMX_VPP_INPUT_OVERLAY_PORT)) {
                if (pComponentParam->eDir != OMX_DirInput) {
                    VPP_DPRINT ("%d :: Invalid input buffer Direction\n", __LINE__);
                    eError = OMX_ErrorBadParameter;
                    break;
                }
                if (portIndex == OMX_VPP_INPUT_OVERLAY_PORT) {
                    if ((OMX_COLOR_Format24bitRGB888 != pComponentParam->format.video.eColorFormat) &&
                            (OMX_COLOR_FormatUnused != pComponentParam->format.video.eColorFormat)) {
                        eError = OMX_ErrorUnsupportedSetting;
                        break;
                    }
                }
                if (portIndex == OMX_VPP_INPUT_PORT) {
                    pComponentPrivate->pIpFrameStatus->ulInWidth = pComponentParam->format.video.nFrameWidth;
                    pComponentPrivate->pIpFrameStatus->ulInHeight = pComponentParam->format.video.nFrameHeight;
                    pComponentPrivate->pIpFrameStatus->ulCInOffset = 0;
                }
            }
            else if (portIndex == OMX_VPP_YUV_OUTPUT_PORT){
                if (pComponentParam->eDir != OMX_DirOutput) {
                    VPP_DPRINT ("VPP::%d :: Invalid Output buffer Direction\n", __LINE__);
                    eError = OMX_ErrorBadParameter;
                    break;
                }
                pComponentPrivate->pOpYUVFrameStatus->ulOutWidth  = pComponentParam->format.video.nFrameWidth;
                pComponentPrivate->pOpYUVFrameStatus->ulOutHeight = pComponentParam->format.video.nFrameHeight;
            }
            else if(portIndex == OMX_VPP_RGB_OUTPUT_PORT){
                if (pComponentParam->eDir != OMX_DirOutput) {
                    VPP_DPRINT ("VPP::%d :: Invalid Output buffer Direction\n", __LINE__);
                    eError = OMX_ErrorBadParameter;
                    break;
                }
                pComponentPrivate->pOpRGBFrameStatus->ulOutWidth  = pComponentParam->format.video.nFrameWidth;
                pComponentPrivate->pOpRGBFrameStatus->ulOutHeight = pComponentParam->format.video.nFrameHeight;
            }
            else {
                VPP_DPRINT ("VPP::%d :: Wrong Port Index Parameter\n", __LINE__);
                eError = OMX_ErrorBadPortIndex;
                break;
            }
            pTemp = memcpy (&(((VPP_COMPONENT_PRIVATE*)pHandle->pComponentPrivate)->sCompPorts[portIndex].pPortDef),
                    pComponentParam, 
                    sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
            if(pTemp == NULL){
                eError = OMX_ErrorUndefined;
                break;
            }

            /* update nBufferSize */
            pComponentPrivate->sCompPorts[portIndex].pPortDef.nBufferSize = 
                pComponentParam->format.video.nFrameWidth * pComponentParam->format.video.nFrameHeight;
            
            switch(pComponentPrivate->sCompPorts[portIndex].pPortDef.format.video.eColorFormat) {
                case OMX_COLOR_FormatYUV420PackedPlanar:
                    pComponentPrivate->sCompPorts[portIndex].pPortDef.nBufferSize*= 3;
                    pComponentPrivate->sCompPorts[portIndex].pPortDef.nBufferSize/= 2;
                    break;
                case OMX_COLOR_FormatCbYCrY:
                    pComponentPrivate->sCompPorts[portIndex].pPortDef.nBufferSize*= 2;
                    break;
                case OMX_COLOR_FormatYCbYCr:
                    pComponentPrivate->sCompPorts[portIndex].pPortDef.nBufferSize*= 2;
                    break;
                case OMX_COLOR_Format32bitARGB8888:
                    pComponentPrivate->sCompPorts[portIndex].pPortDef.nBufferSize*= 4;
                    break;
                case OMX_COLOR_Format24bitRGB888:
                    pComponentPrivate->sCompPorts[portIndex].pPortDef.nBufferSize*= 3;
                    break;
                case OMX_COLOR_Format16bitRGB565:
                    pComponentPrivate->sCompPorts[portIndex].pPortDef.nBufferSize*= 2;
                    break;
                case OMX_COLOR_Format12bitRGB444:
                    pComponentPrivate->sCompPorts[portIndex].pPortDef.nBufferSize*= 2;
                    break;
                case OMX_COLOR_Format8bitRGB332:
                    pComponentPrivate->sCompPorts[portIndex].pPortDef.nBufferSize*= 2;
                    break; 
                case OMX_COLOR_FormatL8:
                    break;
                case OMX_COLOR_FormatL4:
                    pComponentPrivate->sCompPorts[portIndex].pPortDef.nBufferSize/= 2;
                    break;
                case OMX_COLOR_FormatL2:
                    pComponentPrivate->sCompPorts[portIndex].pPortDef.nBufferSize/= 4; 
                    break;
                case  OMX_COLOR_FormatMonochrome:
                    pComponentPrivate->sCompPorts[portIndex].pPortDef.nBufferSize/= 8; 
                    break;  
                default:
                    pComponentPrivate->sCompPorts[portIndex].pPortDef.nBufferSize/= 2; 
                    break;
            }
 
            VPP_DPRINT("after setparam: %d\n", 
                ((VPP_COMPONENT_PRIVATE*)pHandle->pComponentPrivate)->sCompPorts[portIndex].pPortDef.nPortIndex);

            if (portIndex == OMX_VPP_YUV_OUTPUT_PORT) {
                pVidDef     = &(pComponentPrivate->sCompPorts[OMX_VPP_YUV_OUTPUT_PORT].pPortDef.format.video);
                if (pVidDef->eColorFormat == OMX_COLOR_FormatYUV420PackedPlanar) {
                    pComponentPrivate->pOpYUVFrameStatus->ulCOutOffset = 
                    pComponentPrivate->pOpYUVFrameStatus->ulOutWidth * pComponentPrivate->pOpYUVFrameStatus->ulOutHeight;
                }
                else { 
                    pComponentPrivate->pOpYUVFrameStatus->ulCOutOffset = 0; 
                }
            }
            else if (portIndex == OMX_VPP_INPUT_PORT) {
                pVidDef     = &(pComponentPrivate->sCompPorts[OMX_VPP_INPUT_PORT].pPortDef.format.video);
                if (pVidDef->eColorFormat == OMX_COLOR_FormatYUV420PackedPlanar) {
                    pComponentPrivate->pIpFrameStatus->ulCInOffset = 
                    pComponentPrivate->pIpFrameStatus->ulInWidth * pComponentPrivate->pIpFrameStatus->ulInHeight;         
                }
                else {
                    pComponentPrivate->pIpFrameStatus->ulCInOffset = 0; 
                }
            }
            break;
        }
    case OMX_IndexParamPriorityMgmt:
        pTemp = memcpy(pComponentPrivate->pPriorityMgmt, 
            (OMX_PRIORITYMGMTTYPE*)pCompParam, 
            sizeof(OMX_PRIORITYMGMTTYPE)); 
        if(pTemp == NULL){
            eError = OMX_ErrorUndefined;
            break;
        }
        break;
    case OMX_IndexParamCompBufferSupplier:
        {
            OMX_PARAM_BUFFERSUPPLIERTYPE *pBuffSupplierParam = (OMX_PARAM_BUFFERSUPPLIERTYPE *)pCompParam;
            /*Verify if it's a correct port index*/
            if ( pBuffSupplierParam->nPortIndex == OMX_VPP_INPUT_PORT || 
                    pBuffSupplierParam->nPortIndex == OMX_VPP_INPUT_OVERLAY_PORT || 
                    pBuffSupplierParam->nPortIndex == OMX_VPP_RGB_OUTPUT_PORT || 
                    pBuffSupplierParam->nPortIndex == OMX_VPP_YUV_OUTPUT_PORT ) { 
                /* Copy parameters to input port buffer supplier type */
                pComponentPrivate->sCompPorts[pBuffSupplierParam->nPortIndex].eSupplierSetting = 
                    pBuffSupplierParam->eBufferSupplier;
            }
            else {
                eError = OMX_ErrorBadPortIndex;
                break;
            }
            break;
        }
#ifdef KHRONOS_1_1
    case OMX_IndexParamStandardComponentRole:
        if (pCompParam) {
            pRole = (OMX_PARAM_COMPONENTROLETYPE *)pCompParam;
            memcpy(&(pComponentPrivate->componentRole), (void *)pRole, sizeof(OMX_PARAM_COMPONENTROLETYPE));
        } else {
            eError = OMX_ErrorBadParameter;
        }
        break;
#endif
    default:
        eError = OMX_ErrorUnsupportedIndex;
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
static OMX_ERRORTYPE VPP_GetConfig (OMX_HANDLETYPE hComp, 
OMX_INDEXTYPE nConfigIndex,
OMX_PTR ComponentConfigStructure)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE* pHandle= (OMX_COMPONENTTYPE*)hComp;
    VPP_COMPONENT_PRIVATE *pComponentPrivate = NULL;

    VPP_DPRINT ("VPP::Inside the GetConfig\n");

    OMX_CHECK_CMD(hComp, ComponentConfigStructure, OMX_TRUE);
    pComponentPrivate = (VPP_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;

    switch(nConfigIndex) 
    {  
    case OMX_IndexConfigCommonInputCrop :
        {
            OMX_CONFIG_RECTTYPE *crop = (OMX_CONFIG_RECTTYPE*)ComponentConfigStructure;
            crop->nLeft   = pComponentPrivate->pCrop->nLeft;
            crop->nWidth  = pComponentPrivate->pCrop->nWidth;
            crop->nTop    = pComponentPrivate->pCrop->nTop;
            crop->nHeight = pComponentPrivate->pCrop->nHeight;

            break;
        }
    case OMX_IndexConfigCommonRotate :/*On Rotation, the OMX_CONFIG_ROTATIONTYPE must indicate the port index, since VPP supports independent rotation on each port*/
        {
            OMX_CONFIG_ROTATIONTYPE *Rotate = (OMX_CONFIG_ROTATIONTYPE*)ComponentConfigStructure;
            if (Rotate->nPortIndex==OMX_VPP_RGB_OUTPUT_PORT) {
                Rotate->nRotation = pComponentPrivate->pIpFrameStatus->ulRGBRotation;
            }
            else if (Rotate->nPortIndex==OMX_VPP_YUV_OUTPUT_PORT) {
                Rotate->nRotation = pComponentPrivate->pIpFrameStatus->ulYUVRotation;
            }
            else if (Rotate->nPortIndex==OMX_VPP_INPUT_PORT ||Rotate->nPortIndex==OMX_VPP_INPUT_OVERLAY_PORT){
                Rotate->nRotation = pComponentPrivate->pIpFrameStatus->ulYUVRotation;
            }
            else {
                eError = OMX_ErrorBadParameter;
                break;
            }
            break;
        }
    case OMX_IndexConfigCommonMirror:
        {
            OMX_CONFIG_MIRRORTYPE *nMirror = (OMX_CONFIG_MIRRORTYPE*)ComponentConfigStructure;
            if(nMirror->nPortIndex > OMX_VPP_MAXPORT_NUM){
                eError = OMX_ErrorBadPortIndex;
                goto EXIT;
            }
            nMirror->eMirror = ((VPP_COMPONENT_PRIVATE*)pHandle->pComponentPrivate)->sCompPorts[nMirror->nPortIndex].eMirror;
            break;
        }
    case OMX_IndexConfigCommonScale:
        {
            OMX_CONFIG_SCALEFACTORTYPE *sScale = (OMX_CONFIG_SCALEFACTORTYPE*)ComponentConfigStructure;
            if(sScale->nPortIndex > OMX_VPP_MAXPORT_NUM){
                eError = OMX_ErrorBadPortIndex;
                goto EXIT;
            }

            sScale->xWidth = ((VPP_COMPONENT_PRIVATE*)pHandle->pComponentPrivate)->sScale.xWidth;
            sScale->xHeight = ((VPP_COMPONENT_PRIVATE*)pHandle->pComponentPrivate)->sScale.xHeight;
            break;
        }
    case OMX_IndexCustomConfigInputSize:
        {
            OMX_FRAMESIZETYPE *pInputSize = (OMX_FRAMESIZETYPE *)ComponentConfigStructure;
            VPP_COMPONENT_PRIVATE *pComponentPrivate = (VPP_COMPONENT_PRIVATE*)pHandle->pComponentPrivate;
            if((pInputSize->nPortIndex != OMX_VPP_INPUT_PORT) && (pInputSize->nPortIndex != OMX_VPP_INPUT_OVERLAY_PORT)){
                eError = OMX_ErrorBadParameter;
                goto EXIT;
            }

            if(pInputSize->nPortIndex == OMX_VPP_INPUT_PORT){
                pInputSize->nWidth = pComponentPrivate->pIpFrameStatus->ulInWidth;
                pInputSize->nHeight = pComponentPrivate->pIpFrameStatus->ulInHeight;
            }
            else if(pInputSize->nPortIndex == OMX_VPP_INPUT_OVERLAY_PORT){
                pInputSize->nWidth = pComponentPrivate->pIpFrameStatus->ulInWidth;
                pInputSize->nHeight = pComponentPrivate->pIpFrameStatus->ulInHeight;
            }
            break;
        }
    case OMX_IndexConfigCommonOutputSize:
        {
            OMX_FRAMESIZETYPE *pOutputSize = (OMX_FRAMESIZETYPE *)ComponentConfigStructure;
            VPP_COMPONENT_PRIVATE *pComponentPrivate = (VPP_COMPONENT_PRIVATE*)pHandle->pComponentPrivate;
            if((pOutputSize->nPortIndex != OMX_VPP_YUV_OUTPUT_PORT) && (pOutputSize->nPortIndex != OMX_VPP_RGB_OUTPUT_PORT)){
                eError = OMX_ErrorBadParameter;
                goto EXIT;
            }

            if(pOutputSize->nPortIndex == OMX_VPP_YUV_OUTPUT_PORT){
                pOutputSize->nWidth = pComponentPrivate->pOpYUVFrameStatus->ulOutWidth;
                pOutputSize->nHeight = pComponentPrivate->pOpYUVFrameStatus->ulOutHeight;
            }
            else if(pOutputSize->nPortIndex == OMX_VPP_RGB_OUTPUT_PORT){
                pOutputSize->nWidth = pComponentPrivate->pOpRGBFrameStatus->ulOutWidth;
                pOutputSize->nHeight = pComponentPrivate->pOpRGBFrameStatus->ulOutHeight;
            }
            
            break;
        }
    default:
        eError = OMX_ErrorUnsupportedIndex;
        break;
    }
EXIT:
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
static OMX_ERRORTYPE VPP_SetConfig (OMX_HANDLETYPE hComp, 
OMX_INDEXTYPE nConfigIndex,
OMX_PTR ComponentConfigStructure)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE* pHandle= (OMX_COMPONENTTYPE*)hComp;
    OMX_U8 *pTemp = NULL;

    OMX_CHECK_CMD(hComp, ComponentConfigStructure, OMX_TRUE);

    VPP_DPRINT ("VPP::Inside the SetConfig\n");

    switch(nConfigIndex) 
    {
    case OMX_IndexConfigCommonColorKey:
        {
            /*Already allocated in ComponentInit*/
            OMX_CONFIG_COLORKEYTYPE *transcolorkey = (OMX_CONFIG_COLORKEYTYPE*)ComponentConfigStructure;
            pTemp = memcpy (((VPP_COMPONENT_PRIVATE*)pHandle->pComponentPrivate)-> colorKey,
                ((OMX_U8 *) transcolorkey ->nARGBMask )+1,
                3 * sizeof(OMX_U8));
            if(pTemp == NULL){
                eError = OMX_ErrorUndefined;
                goto EXIT;
            }
            break;
        }
    case OMX_IndexConfigCommonInputCrop :
        {
            OMX_CONFIG_RECTTYPE *crop = (OMX_CONFIG_RECTTYPE*)ComponentConfigStructure;
            ((VPP_COMPONENT_PRIVATE*) pHandle->pComponentPrivate)->pIpFrameStatus->ulInXstart = crop->nLeft;
            ((VPP_COMPONENT_PRIVATE*) pHandle->pComponentPrivate)->pIpFrameStatus->ulInXsize  = crop->nWidth;
            ((VPP_COMPONENT_PRIVATE*) pHandle->pComponentPrivate)->pIpFrameStatus->ulInYstart = crop->nTop;
            ((VPP_COMPONENT_PRIVATE*) pHandle->pComponentPrivate)->pIpFrameStatus->ulInYsize  = crop->nHeight;
            /*StdcomponentRoleTest*/
            ((VPP_COMPONENT_PRIVATE*) pHandle->pComponentPrivate)->pCrop->nLeft = crop->nLeft;
            ((VPP_COMPONENT_PRIVATE*) pHandle->pComponentPrivate)->pCrop->nWidth = crop->nWidth;
            ((VPP_COMPONENT_PRIVATE*) pHandle->pComponentPrivate)->pCrop->nTop = crop->nTop;
            ((VPP_COMPONENT_PRIVATE*) pHandle->pComponentPrivate)->pCrop->nHeight = crop->nHeight;
            break;
        }
    case OMX_IndexConfigCommonContrast :
        {
            OMX_U32 nContrast;
            OMX_CONFIG_CONTRASTTYPE *contrast = (OMX_CONFIG_CONTRASTTYPE*)ComponentConfigStructure;
            
            if (contrast->nContrast < VPP_CONTRAST_MIN) {  VPP_DPRINT("Out of range value, setting Contrast to Minimum\n");
                contrast->nContrast = VPP_CONTRAST_MIN;
            }
            else if(contrast->nContrast > VPP_CONTRAST_MAX)
            {
                VPP_DPRINT("Out of range value, setting Contrast to Maximum\n");
                contrast->nContrast = VPP_CONTRAST_MAX;
            }
            
            /*Normalize for VGPOP range*/
            nContrast = (OMX_U32) ((contrast->nContrast+VPP_CONTRAST_OFFSET)*VPP_CONTRAST_FACTOR);
            
            ((VPP_COMPONENT_PRIVATE*) 
                pHandle->pComponentPrivate)->pIpFrameStatus->ulVideoGain = nContrast;
            break;
        }
    case OMX_IndexConfigCommonRotate :
        {
            OMX_CONFIG_ROTATIONTYPE *Rotate = (OMX_CONFIG_ROTATIONTYPE*)ComponentConfigStructure;
            VPP_DPRINT ("VPP::Inside the SetConfig in OMX_IndexConfigCommonRotate  \n");

            if (((Rotate->nPortIndex == OMX_VPP_RGB_OUTPUT_PORT) || (Rotate->nPortIndex == OMX_VPP_YUV_OUTPUT_PORT)) &&
                    (Rotate->nRotation == 0 || 
                        Rotate->nRotation == 90 || 
                        Rotate->nRotation == 180 || 
                        Rotate->nRotation == 270)) {
                if(Rotate->nPortIndex == OMX_VPP_RGB_OUTPUT_PORT){
                    ((VPP_COMPONENT_PRIVATE*) 
                    pHandle->pComponentPrivate)->pIpFrameStatus->ulRGBRotation = Rotate->nRotation;
                }
                else if (Rotate->nPortIndex == OMX_VPP_YUV_OUTPUT_PORT) {
                    ((VPP_COMPONENT_PRIVATE*) 
                        pHandle->pComponentPrivate)->pIpFrameStatus->ulYUVRotation = Rotate->nRotation;
                }
            }
            else if (((Rotate->nPortIndex == OMX_VPP_INPUT_PORT) || (Rotate->nPortIndex == OMX_VPP_INPUT_OVERLAY_PORT)) &&
                        (Rotate->nRotation == 0 || 
                        Rotate->nRotation == 90 || 
                        Rotate->nRotation == 180 || 
                        Rotate->nRotation == 270)) {
                    ((VPP_COMPONENT_PRIVATE*) 
                        pHandle->pComponentPrivate)->pIpFrameStatus->ulYUVRotation = Rotate->nRotation;
                }
            else{
                eError = OMX_ErrorBadParameter;
                goto EXIT;
            }
            break;
        }
    case OMX_IndexCustomSetZoomFactor :
        {
            OMX_U32 *nZoomfactor = (OMX_U32*)ComponentConfigStructure;
            
            ((VPP_COMPONENT_PRIVATE*) 
                pHandle->pComponentPrivate)->pIpFrameStatus->ulZoomFactor = *nZoomfactor;
            break;
        }
    case OMX_IndexCustomSetZoomLimit :
        {
            OMX_U32 *nZoomlimit = (OMX_U32*)ComponentConfigStructure;
            ((VPP_COMPONENT_PRIVATE*) 
                pHandle->pComponentPrivate)->pIpFrameStatus->ulZoomLimit = *nZoomlimit;
            
            break;
        }
    case OMX_IndexCustomSetZoomSpeed :
        {
            OMX_U32 *nZoomspeed = (OMX_U32*)ComponentConfigStructure;
            ((VPP_COMPONENT_PRIVATE*) 
                pHandle->pComponentPrivate)->pIpFrameStatus->ulZoomSpeed = *nZoomspeed;
            
            break;
        }
    case OMX_IndexCustomSetFrostedGlassOvly :
        {
            OMX_U32 *FrostedGlassOvly = (OMX_U32*)ComponentConfigStructure;
            ((VPP_COMPONENT_PRIVATE*) 
                pHandle->pComponentPrivate)->pIpFrameStatus->ulFrostedGlassOvly = *FrostedGlassOvly;

            break;
        }
    case OMX_IndexCustomSetZoomXoffsetFromCenter16 :
        {
            OMX_U32 *XoffsetFromCenter16 = (OMX_U32*)ComponentConfigStructure;
            ((VPP_COMPONENT_PRIVATE*) 
                pHandle->pComponentPrivate)->pIpFrameStatus->ulXoffsetFromCenter16 = *XoffsetFromCenter16;
            
            break;
        }
    case OMX_IndexCustomSetZoomYoffsetFromCenter16 :
        {
            OMX_U32 *YoffsetFromCenter16 = (OMX_U32*)ComponentConfigStructure;
            ((VPP_COMPONENT_PRIVATE*) 
                pHandle->pComponentPrivate)->pIpFrameStatus->ulYoffsetFromCenter16 = *YoffsetFromCenter16;
            
            break;
        }
    case OMX_IndexConfigCommonMirror:
        {
            /*Only RGB output mirroring supported*/
            OMX_CONFIG_MIRRORTYPE *nMirror = (OMX_CONFIG_MIRRORTYPE*)ComponentConfigStructure;
            OMX_S32 nMirrorRotation = 0;
            OMX_MIRRORTYPE eMirrorPrev = OMX_MirrorNone;

            if(nMirror->nPortIndex > OMX_VPP_MAXPORT_NUM){
                eError = OMX_ErrorBadParameter;
                goto EXIT;
            }
            
            eMirrorPrev = ((VPP_COMPONENT_PRIVATE*)pHandle->pComponentPrivate)->sCompPorts[nMirror->nPortIndex].eMirror;
            if(eMirrorPrev != OMX_MirrorNone){
                ((VPP_COMPONENT_PRIVATE*) 
                    pHandle->pComponentPrivate)->pIpFrameStatus->ulMirror = OMX_FALSE;
                if(eMirrorPrev == OMX_MirrorVertical|| eMirrorPrev == OMX_MirrorBoth){
                    nMirrorRotation = ((VPP_COMPONENT_PRIVATE*)pHandle->pComponentPrivate)->pIpFrameStatus->ulRGBRotation;
                    if(nMirrorRotation <= 90){
                        nMirrorRotation += 180;
                    }
                    else{
                        nMirrorRotation -= 180;
                    }
                    ((VPP_COMPONENT_PRIVATE*)pHandle->pComponentPrivate)->pIpFrameStatus->ulRGBRotation = nMirrorRotation;
                }
            }
            
            if (nMirror->eMirror == OMX_MirrorHorizontal){
                ((VPP_COMPONENT_PRIVATE*) pHandle->pComponentPrivate)
                    ->pIpFrameStatus->ulMirror = OMX_TRUE;
                ((VPP_COMPONENT_PRIVATE*)pHandle->pComponentPrivate)
                    ->sCompPorts[nMirror->nPortIndex].eMirror = OMX_MirrorHorizontal;
            }
            else if (nMirror->eMirror == OMX_MirrorVertical){
                nMirrorRotation = 180;
                ((VPP_COMPONENT_PRIVATE*) 
                    pHandle->pComponentPrivate)->pIpFrameStatus->ulMirror = OMX_TRUE;
                ((VPP_COMPONENT_PRIVATE*)pHandle->pComponentPrivate)
                    ->sCompPorts[nMirror->nPortIndex].eMirror = OMX_MirrorVertical;
                nMirrorRotation += ((VPP_COMPONENT_PRIVATE*)pHandle->pComponentPrivate)->pIpFrameStatus->ulRGBRotation;
                if (nMirrorRotation >= 360){
                    nMirrorRotation -= 180;
                }
                ((VPP_COMPONENT_PRIVATE*) 
                    pHandle->pComponentPrivate)->pIpFrameStatus->ulRGBRotation = nMirrorRotation;
            }
            else if (nMirror->eMirror == OMX_MirrorBoth) {
                ((VPP_COMPONENT_PRIVATE*)pHandle->pComponentPrivate)
                    ->sCompPorts[nMirror->nPortIndex].eMirror = OMX_MirrorBoth;
                nMirrorRotation = 180;
                nMirrorRotation += ((VPP_COMPONENT_PRIVATE*)pHandle->pComponentPrivate)->pIpFrameStatus->ulRGBRotation;
                if (nMirrorRotation >= 360){
                    nMirrorRotation -= 180;
                }
                ((VPP_COMPONENT_PRIVATE*) 
                    pHandle->pComponentPrivate)->pIpFrameStatus->ulRGBRotation = nMirrorRotation;
            }
            else if(nMirror->eMirror == OMX_MirrorNone){
                ((VPP_COMPONENT_PRIVATE*)pHandle->pComponentPrivate)
                    ->sCompPorts[nMirror->nPortIndex].eMirror = OMX_MirrorNone;
                }
            else {
                eError = OMX_ErrorUnsupportedSetting;
                goto EXIT;
            }

            break;
        }
    case OMX_IndexConfigCommonDithering:
        {
            OMX_CONFIG_DITHERTYPE *nDither = (OMX_CONFIG_DITHERTYPE *)ComponentConfigStructure;
            if (nDither->eDither == OMX_DitherErrorDiffusion && nDither->nPortIndex == OMX_VPP_RGB_OUTPUT_PORT) {
                ((VPP_COMPONENT_PRIVATE*) 
                    pHandle->pComponentPrivate)->pIpFrameStatus->ulDithering = OMX_TRUE;
            }
            else {
                eError = OMX_ErrorUnsupportedSetting;
                goto EXIT;
            }
            break;
        } 
    case OMX_IndexCustomVideoColorRange:
        {
            OMX_U32 *nColorRange = (OMX_U32*)ComponentConfigStructure;
            VPP_DPRINT ("VPP::Inside the SetConfig in OMX_IndexConfigCommonColorRange  \n");
            if ((*nColorRange == VGPOP_IN_16_235_OUT_16_235 || 
                    *nColorRange == VGPOP_IN_00_255_OUT_00_255 || 
                    *nColorRange == VGPOP_IN_00_255_OUT_16_235 || 
                    *nColorRange == VGPOP_IN_16_235_OUT_00_255)) {
                ((VPP_COMPONENT_PRIVATE*) 
                    pHandle->pComponentPrivate)->pIpFrameStatus->eIORange = *nColorRange;
            }
            else {
                eError = OMX_ErrorBadParameter;
                goto EXIT;
            }
            
            break;
        }
    case OMX_IndexConfigCommonScale:
        {
            OMX_CONFIG_SCALEFACTORTYPE *sScale = (OMX_CONFIG_SCALEFACTORTYPE*)ComponentConfigStructure;
            if(sScale->nPortIndex > OMX_VPP_MAXPORT_NUM){
                eError = OMX_ErrorBadParameter;
                goto EXIT;
            }

            ((VPP_COMPONENT_PRIVATE*)pHandle->pComponentPrivate)->sScale.xWidth = sScale->xWidth;
            ((VPP_COMPONENT_PRIVATE*)pHandle->pComponentPrivate)->sScale.xHeight = sScale->xHeight;

            break;
        }
    case OMX_IndexCustomConfigInputSize:
        {
            OMX_FRAMESIZETYPE *pInputSize = (OMX_FRAMESIZETYPE *)ComponentConfigStructure;
            VPP_COMPONENT_PRIVATE *pComponentPrivate = (VPP_COMPONENT_PRIVATE*)pHandle->pComponentPrivate;
            if((pInputSize->nPortIndex != OMX_VPP_INPUT_PORT) && (pInputSize->nPortIndex != OMX_VPP_INPUT_OVERLAY_PORT)){
                eError = OMX_ErrorBadParameter;
                goto EXIT;
            }

            if(pInputSize->nPortIndex == OMX_VPP_INPUT_PORT){
                pComponentPrivate->pIpFrameStatus->ulInWidth = pInputSize->nWidth;
                pComponentPrivate->pIpFrameStatus->ulInHeight = pInputSize->nHeight;
                if(pComponentPrivate->sCompPorts[OMX_VPP_INPUT_PORT].pPortDef.format.video.eColorFormat == OMX_COLOR_FormatYUV420PackedPlanar){
                     pComponentPrivate->pIpFrameStatus->ulCInOffset = 
                        pComponentPrivate->pIpFrameStatus->ulInWidth * pComponentPrivate->pIpFrameStatus->ulInHeight;
                }
                else{
                    pComponentPrivate->pIpFrameStatus->ulCInOffset = 0;
                }
            }
            else if(pInputSize->nPortIndex == OMX_VPP_INPUT_OVERLAY_PORT){
                eError = OMX_ErrorUnsupportedSetting;
                goto EXIT;
            }
            break;
        }
    case OMX_IndexConfigCommonOutputSize:
        {
            OMX_FRAMESIZETYPE *pOutputSize = (OMX_FRAMESIZETYPE *)ComponentConfigStructure;
            VPP_COMPONENT_PRIVATE *pComponentPrivate = (VPP_COMPONENT_PRIVATE*)pHandle->pComponentPrivate;
            if((pOutputSize->nPortIndex != OMX_VPP_YUV_OUTPUT_PORT) && (pOutputSize->nPortIndex != OMX_VPP_RGB_OUTPUT_PORT)){
                eError = OMX_ErrorBadParameter;
                goto EXIT;
            }

            if(pOutputSize->nPortIndex == OMX_VPP_YUV_OUTPUT_PORT){
                pComponentPrivate->pOpYUVFrameStatus->ulOutWidth = pOutputSize->nWidth;
                pComponentPrivate->pOpYUVFrameStatus->ulOutHeight = pOutputSize->nHeight;
                if(pComponentPrivate->sCompPorts[OMX_VPP_YUV_OUTPUT_PORT].pPortDef.format.video.eColorFormat == OMX_COLOR_FormatYUV420PackedPlanar){
                     pComponentPrivate->pOpYUVFrameStatus->ulCOutOffset = 
                        pComponentPrivate->pOpYUVFrameStatus->ulOutWidth * pComponentPrivate->pOpYUVFrameStatus->ulOutHeight;
                }
                else{
                    pComponentPrivate->pOpYUVFrameStatus->ulCOutOffset = 0;
                }
            }
            else if(pOutputSize->nPortIndex == OMX_VPP_RGB_OUTPUT_PORT){
                pComponentPrivate->pOpRGBFrameStatus->ulOutWidth            = pOutputSize->nWidth;
                pComponentPrivate->pOpRGBFrameStatus->ulOutHeight           = pOutputSize->nHeight;
                pComponentPrivate->pOpRGBFrameStatus->ulCOutOffset          = 0;
            }
            break;
        }
    default:
        eError = OMX_ErrorUnsupportedIndex;
        break;
    }
EXIT:
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
static OMX_ERRORTYPE VPP_GetState (OMX_HANDLETYPE pComponent, OMX_STATETYPE* pState)
{
    OMX_ERRORTYPE eError = OMX_ErrorUndefined;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;

    OMX_CHECK_CMD(pComponent, pState, OMX_TRUE);

    if (pHandle->pComponentPrivate) {
        *pState = ((VPP_COMPONENT_PRIVATE*) pHandle->pComponentPrivate)->curState;
    }
    else {
        eError = OMX_ErrorUndefined;
        goto EXIT;
    }
    eError = OMX_ErrorNone;

EXIT:
    return eError;
}

/*-------------------------------------------------------------------*/
/**
  *  EmptyThisBuffer() This callback is used to send the input buffer to
  *  component
  *
  * @param pComponent       handle for this instance of the component
  
  * @param pBuffer          buffer to be sent to codec
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  **/
/*-------------------------------------------------------------------*/
static OMX_ERRORTYPE VPP_EmptyThisBuffer (OMX_HANDLETYPE pComponent, OMX_BUFFERHEADERTYPE* pBufHdr)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    VPP_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE *portDef = NULL;
    VPP_BUFFERDATA_PROPAGATION* pDataProp = NULL;
    OMX_S16 nRet = 0;
    OMX_U32 nCount = 0;
    OMX_U16 i = 0;
    
    OMX_CHECK_CMD(pComponent, pBufHdr, OMX_TRUE);
    

    pComponentPrivate = (VPP_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;

#ifdef __PERF_INSTRUMENTATION__
    PERF_ReceivedFrame(pComponentPrivate->pPERF,
        pBufHdr->pBuffer,
        pBufHdr->nFilledLen,
        PERF_ModuleHLMM);
#endif

    VPP_DPRINT("VPP: EmptyThisBuffer() %p\n", pBufHdr);

    portDef = pBufHdr->pInputPortPrivate;

    if (pBufHdr->nInputPortIndex != OMX_VPP_INPUT_PORT && 
            pBufHdr->nInputPortIndex != OMX_VPP_INPUT_OVERLAY_PORT) {
        VPP_DPRINT("Error ! Incorrect input port index\n");
        eError = OMX_ErrorBadPortIndex;
        goto EXIT;
    }
    
    if (pComponentPrivate->curState != OMX_StateExecuting && pComponentPrivate->curState != OMX_StatePause) {
        eError = OMX_ErrorIncorrectStateOperation;
        VPP_DPRINT("VPP: Incorrect state. state = %d\n", pComponentPrivate->curState);
        goto EXIT;
    }
    if(pBufHdr->nInputPortIndex == OMX_VPP_INPUT_PORT &&
            !pComponentPrivate->sCompPorts[OMX_VPP_INPUT_PORT].pPortDef.bEnabled){
        VPP_DPRINT("Error (in VPP)! OMX_ErrorIncorrectStateOperation, %d\n", pComponentPrivate->curState);
        eError = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }
    else if(pBufHdr->nInputPortIndex == OMX_VPP_INPUT_OVERLAY_PORT &&
            !pComponentPrivate->sCompPorts[OMX_VPP_INPUT_OVERLAY_PORT].pPortDef.bEnabled){
        VPP_DPRINT("Error (in VPP)! OMX_ErrorIncorrectStateOperation, %d\n", pComponentPrivate->curState);
        eError = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }

    if (pBufHdr->nSize != sizeof(OMX_BUFFERHEADERTYPE)) {
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if ((pBufHdr->nVersion.s.nVersionMajor != VPP_MAJOR_VER) || 
            (pBufHdr->nVersion.s.nVersionMinor != VPP_MINOR_VER) ||            
            (pBufHdr->nVersion.s.nRevision != VPP_REVISION) || 
            (pBufHdr->nVersion.s.nStep != VPP_STEP)) {
        eError = OMX_ErrorVersionMismatch;
        goto EXIT;
    }

    eError = VPP_IsValidBuffer(pBufHdr, pComponentPrivate, portDef->nPortIndex, &nCount); 
    if (eError !=OMX_ErrorNone) {
        goto EXIT;
    }

	if (pComponentPrivate->toState == OMX_StateIdle && pComponentPrivate->curState == OMX_StateExecuting) {
		if(pComponentPrivate->sCompPorts[pBufHdr->nInputPortIndex].hTunnelComponent == NULL){
			VPP_DPRINT("Not right state, return buf %p\n", pBufHdr);
		        pComponentPrivate->cbInfo.EmptyBufferDone (pComponentPrivate->pHandle,
	                                    pComponentPrivate->pHandle->pApplicationPrivate,
	                                    pBufHdr
	                                    );
			goto EXIT;
		}
		else{
          if(pComponentPrivate->sCompPorts[portDef->nPortIndex].eSupplierSetting == OMX_BufferSupplyOutput){
            pComponentPrivate->sCompPorts[portDef->nPortIndex].pVPPBufHeader[nCount].eBufferOwner = VPP_BUFFER_TUNNEL_COMPONENT;
            VPP_DPRINT("VPP:: call to OMX_FillThisBuffer():: %d\n", __LINE__);
			eError = OMX_FillThisBuffer(
				pComponentPrivate->sCompPorts[pBufHdr->nInputPortIndex].hTunnelComponent,
                            pBufHdr);
        }
        else{
            pComponentPrivate->sCompPorts[portDef->nPortIndex].pVPPBufHeader[nCount].eBufferOwner = VPP_BUFFER_COMPONENT_IN;
        }
			goto EXIT;
		}
	}

        /*usmc  VPP-JPEG TUNNELING*/
    if((pComponentPrivate->bIsStopping == OMX_TRUE) && 
            (!pComponentPrivate->sCompPorts[OMX_VPP_INPUT_PORT].hTunnelComponent)) {
        pComponentPrivate->sCompPorts[OMX_VPP_INPUT_PORT].nReturnedBufferCount--; 
        goto EXIT;
    }/*USMC VPP-JPEG TUNNELING*/


    if(pBufHdr->nInputPortIndex == OMX_VPP_INPUT_PORT){
        pComponentPrivate->nInputFrame ++;
    }
    else{
        pComponentPrivate->nOverlayFrame ++;
    }

    if(pComponentPrivate->nInputFrame != pComponentPrivate->nOverlayFrame){
        if(pComponentPrivate->IsYUVdataout){
            pComponentPrivate->nInYUVBufferCount ++;
        }
        if(pComponentPrivate->IsRGBdataout){
            pComponentPrivate->nInRGBBufferCount ++;
        }
    }


    VPP_DPRINT("nInBufferCount %d, nInRGBBufferCount %d:: InputCount= %d, OverlayCount= %d\n ", pComponentPrivate->nInYUVBufferCount, pComponentPrivate->nInRGBBufferCount, pComponentPrivate->nInputFrame, pComponentPrivate->nOverlayFrame);
   if(pBufHdr->pMarkData == NULL){
        pBufHdr->pMarkData = pComponentPrivate->pMarkData;
        pBufHdr->hMarkTargetComponent = pComponentPrivate->hMarkTargetComponent;
        pComponentPrivate->pMarkData = NULL;
        pComponentPrivate->hMarkTargetComponent = NULL;
    }

    if ((pBufHdr->nFlags) || (pBufHdr->pMarkData) || (pBufHdr->nTickCount) || (pBufHdr->nTimeStamp)){
#ifdef  VPP_DEBUG
        if(pBufHdr->nInputPortIndex == OMX_VPP_INPUT_PORT){
            VPP_DPRINT("OMX_VPP_INPUT_PORT\n");
        }
        else{
            VPP_DPRINT("OMX_VPP_OVERLAY_PORT\n"); 
        }
        if(pBufHdr->nFlags & OMX_BUFFERFLAG_EOS){
            VPP_DPRINT("END OF STREAM DETECTED EmptyThis buffer\n");
        }
        if(pBufHdr->pMarkData){
            VPP_DPRINT("\nMarkDataDetected\n");
        }
        if((pBufHdr->nTickCount) || (pBufHdr->nTimeStamp)){
            VPP_DPRINT("\n nTickCount= %d,   nTimeStamp = %d\n\n", pBufHdr->nTickCount, pBufHdr->nTimeStamp);
        }
#endif

        for (i = 0; i < pComponentPrivate->sCompPorts[pBufHdr->nInputPortIndex].pPortDef.nBufferCountActual; i ++) {
            pDataProp = &(pComponentPrivate->sCompPorts[pBufHdr->nInputPortIndex].sBufferDataProp[i]);
            if (pDataProp->buffer_idYUV == 0xFFFFFFFF && pDataProp->buffer_idRGB == 0xFFFFFFFF) {
                pDataProp->flag = pBufHdr->nFlags;
                if(pComponentPrivate->IsYUVdataout){
                    pDataProp->buffer_idYUV= pComponentPrivate->nInYUVBufferCount;
                }
                if(pComponentPrivate->IsRGBdataout){
                    pDataProp->buffer_idRGB= pComponentPrivate->nInRGBBufferCount;
                }
                VPP_DPRINT("Record buff in array[%d] buffer_idYUV = %d, buffer_idRGB = %d\n, nFlags= %x", i, pDataProp->buffer_idYUV, pComponentPrivate->nInRGBBufferCount, pDataProp->flag);
                 /* mark the first buffer from input port after receiving mark buffer command */
                 if (pBufHdr->pMarkData) {
                    VPP_DPRINT("Get mark buffer command, mark buffer %p\n", pBufHdr);
                    pDataProp->pMarkData = pBufHdr->pMarkData;
                    pDataProp->hMarkTargetComponent = pBufHdr->hMarkTargetComponent;
                }
                 if((pBufHdr->nTickCount) || (pBufHdr->nTimeStamp)){
                    printf("Record TimeStamp= %Ld & nTickCount =%ld\n", pBufHdr->nTimeStamp, pBufHdr->nTickCount);
                    pDataProp->nTickCount = pBufHdr->nTickCount;
                    pDataProp->nTimeStamp = pBufHdr->nTimeStamp;
                 }
                break;
            }
        }
    }

    pComponentPrivate->sCompPorts[portDef->nPortIndex].pVPPBufHeader[nCount].pBufHeader->pBuffer = pBufHdr->pBuffer; /*Updating pBuffer*/
    pComponentPrivate->sCompPorts[portDef->nPortIndex].pVPPBufHeader[nCount].bHolding = OMX_TRUE;

    VPP_DPRINT("\n------------------------------------------\n\n");
    VPP_DPRINT ("%d :: Component Sending Filled ip buff %p \
                        to Component Thread\n",pBufHdr->nInputPortIndex, pBufHdr); 
    VPP_DPRINT("\n------------------------------------------\n\n");

#if 0
    FILE *fp;

    fp = fopen("mytestcvnew.raw", "w");
    fwrite(pBufHdr->pBuffer, 1, pBufHdr->nFilledLen, fp);
    fclose(fp);
#endif

    VPP_DPRINT("VPP get %d bytes of data from %p\n", pBufHdr->nFilledLen, pBufHdr->pBuffer);

    pComponentPrivate->sCompPorts[portDef->nPortIndex].pVPPBufHeader[nCount].eBufferOwner = VPP_BUFFER_COMPONENT_IN;
    nRet = write(pComponentPrivate->nFilled_iPipe[1],&pBufHdr, sizeof(OMX_BUFFERHEADERTYPE*));
    
    if (nRet == -1) {
        eError = OMX_ErrorHardware;
        goto EXIT;
    }

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
  * @param pBufferHdr       buffer to be sent to codec
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  **/
/*-------------------------------------------------------------------*/
static OMX_ERRORTYPE VPP_FillThisBuffer (OMX_HANDLETYPE pComponent, 
                                         OMX_BUFFERHEADERTYPE* pBufferHdr)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    VPP_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE *portDef = NULL;
    int nRet = 0;
    OMX_U32 nCount = 0;


    OMX_CHECK_CMD(pComponent, pBufferHdr, OMX_TRUE);

    VPP_DPRINT("\n------------------------------------------\n\n");
    VPP_DPRINT ("%d :: Component Sending Emptied op buff %p \
                            to Component Thread\n",__LINE__,pBufferHdr); 
    VPP_DPRINT("\n------------------------------------------\n\n");

	VPP_DPRINT("get output buffer %p (%p %p)\n", pBufferHdr, pBufferHdr->hMarkTargetComponent, pBufferHdr->pMarkData);

    pComponentPrivate = (VPP_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    portDef = pBufferHdr->pOutputPortPrivate; 

#ifdef __PERF_INSTRUMENTATION__
    PERF_ReceivedFrame(pComponentPrivate->pPERF,
        pBufferHdr->pBuffer,
        0,
        PERF_ModuleHLMM);
#endif

    if (pBufferHdr->nOutputPortIndex != OMX_VPP_YUV_OUTPUT_PORT && 
            pBufferHdr->nOutputPortIndex != OMX_VPP_RGB_OUTPUT_PORT) {
        VPP_DPRINT("Error ! Incorrect output port index\n");
        eError = OMX_ErrorBadPortIndex;
        goto EXIT;
    }

    if (pComponentPrivate->curState != OMX_StateExecuting && 
                pComponentPrivate->curState != OMX_StatePause &&
                pComponentPrivate->curState != OMX_StateIdle) {
        VPP_DPRINT("Error (in VPP)! OMX_ErrorIncorrectStateOperation, %d\n", pComponentPrivate->curState);
        eError = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }

    if(pBufferHdr->nOutputPortIndex == OMX_VPP_YUV_OUTPUT_PORT &&
            !pComponentPrivate->sCompPorts[OMX_VPP_YUV_OUTPUT_PORT].pPortDef.bEnabled){
        VPP_DPRINT("Error (in VPP)! OMX_ErrorIncorrectStateOperation, %d\n", pComponentPrivate->curState);
        eError = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }
    else if(pBufferHdr->nOutputPortIndex == OMX_VPP_RGB_OUTPUT_PORT &&
            !pComponentPrivate->sCompPorts[OMX_VPP_RGB_OUTPUT_PORT].pPortDef.bEnabled){
        VPP_DPRINT("Error (in VPP)! OMX_ErrorIncorrectStateOperation, %d\n", pComponentPrivate->curState);
        eError = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }


    if (pBufferHdr->nSize != sizeof(OMX_BUFFERHEADERTYPE)) {
        VPP_DPRINT("Error ! OMX_ErrorBadParameter\n");
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }

    if ((pBufferHdr->nVersion.s.nVersionMajor != VPP_MAJOR_VER) || 
            (pBufferHdr->nVersion.s.nVersionMinor != VPP_MINOR_VER) ||            
            (pBufferHdr->nVersion.s.nRevision != VPP_REVISION) || 
            (pBufferHdr->nVersion.s.nStep != VPP_STEP)) {
        eError = OMX_ErrorVersionMismatch;
        goto EXIT;
    }

    if ((pComponentPrivate->toState == OMX_StateIdle) && (pComponentPrivate->curState == OMX_StateExecuting || pComponentPrivate->curState == OMX_StatePause)) {
        VPP_DPRINT("VPP::to state is IDLE, return buf %p\n", pBufferHdr);
        if(pComponentPrivate->sCompPorts[portDef->nPortIndex].eSupplierSetting == OMX_BufferSupplyOutput){
                pComponentPrivate->sCompPorts[portDef->nPortIndex].pVPPBufHeader[nCount].eBufferOwner = VPP_BUFFER_COMPONENT_IN;
        }
        else{
            pComponentPrivate->sCompPorts[portDef->nPortIndex].pVPPBufHeader[nCount].eBufferOwner = VPP_BUFFER_CLIENT;
            pComponentPrivate->cbInfo.FillBufferDone(pComponentPrivate->pHandle,
                    pComponentPrivate->pHandle->pApplicationPrivate,
                    pBufferHdr);
        }
        goto EXIT;
    }

    pBufferHdr->nFilledLen = 0;

    eError = VPP_IsValidBuffer(pBufferHdr,pComponentPrivate,portDef->nPortIndex, &nCount); 
    if ( eError !=OMX_ErrorNone) {
        goto EXIT;
    }
    
    pComponentPrivate->sCompPorts[portDef->nPortIndex].pVPPBufHeader[nCount].pBufHeader->pBuffer = pBufferHdr->pBuffer; /*Updating pBuffer*/
    pComponentPrivate->sCompPorts[portDef->nPortIndex].pVPPBufHeader[nCount].bHolding = OMX_TRUE;
    VPP_DPRINT("VPP: fillthisbuffer: (%p) %d %d %d\n", pBufferHdr, portDef->nPortIndex, nCount, pComponentPrivate->sCompPorts[portDef->nPortIndex].pVPPBufHeader[nCount].bHolding);

    pBufferHdr->nFilledLen = 0;
    VPP_DPRINT ("%d :: Component Sending Emptied op buff  with index %d \
                            to Component Thread\n",__LINE__,pBufferHdr->nOutputPortIndex); 

    pthread_mutex_lock(&pComponentPrivate->buf_mutex);
    pComponentPrivate->sCompPorts[portDef->nPortIndex].pVPPBufHeader[nCount].eBufferOwner = VPP_BUFFER_COMPONENT_IN;
    pthread_mutex_unlock(&pComponentPrivate->buf_mutex);
    nRet = write(pComponentPrivate->nFree_oPipe[1],&pBufferHdr,sizeof(OMX_BUFFERHEADERTYPE*));

    if (nRet == -1) {
        VPP_DPRINT ("VPP::%d :: Error in Writing to the Data pipe\n", __LINE__);
        eError = OMX_ErrorHardware;
        goto EXIT;
    }

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
  *         
  **/
/*-------------------------------------------------------------------*/

static OMX_ERRORTYPE VPP_ComponentDeInit(OMX_HANDLETYPE pHandle)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    OMX_CHECK_CMD(pHandle, OMX_TRUE, OMX_TRUE);
    
    VPP_DPRINT (" IN ComponentDeInit \n");
    
    VPP_DPRINT ("VPP::Freeing OMX pComponentPrivate \n");
    eError = VPP_Free_ComponentResources(pHandle);
    if (eError != OMX_ErrorNone) {
        VPP_DPRINT ("VPP::Error While Stoping the Component Thread\n");
        goto EXIT;
    }
    VPP_DPRINT ("\n");

    /* load the ResourceManagerProxy thread*/
#ifdef RESOURCE_MANAGER_ENABLED
    eError = RMProxy_NewSendCommand(pHandle, RMProxy_FreeResource, OMX_VPP_COMPONENT, 0, 3456, NULL);
    if (eError != OMX_ErrorNone) {
        VPP_DPRINT ("%d ::Error returned from destroy ResourceManagerProxy thread\n",
        __LINE__);
    }
    eError = RMProxy_DeinitalizeEx(OMX_COMPONENTTYPE_VPP);
    if (eError != OMX_ErrorNone) {
        VPP_DPRINT ("VPP::%d ::Error returned from destroy ResourceManagerProxy thread\n",
        __LINE__);
    }
#endif

EXIT:
    return eError;
}

/*-------------------------------------------------------------------*/
/**
  *  VerifyTunnelConnection() 
  *
  * 
  * 
  *
  * @param         
  * @param    
  * @param 
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  **/
/*-------------------------------------------------------------------*/
OMX_ERRORTYPE VPP_VerifyTunnelConnection(VPP_PORT_TYPE *pPort, 
                                         OMX_HANDLETYPE hTunneledComp, 
                                         OMX_PARAM_PORTDEFINITIONTYPE* pPortDef)
{
    /* 1.4 Check if input port is compatible with output port */
    OMX_PARAM_PORTDEFINITIONTYPE MyPortDef ;
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    OMX_CHECK_CMD(pPort, hTunneledComp, pPortDef);

    MyPortDef.nSize = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);

    MyPortDef.nVersion.s.nVersionMajor = VPP_MAJOR_VER;
    MyPortDef.nVersion.s.nVersionMinor = VPP_MINOR_VER;

    MyPortDef.nPortIndex = pPort->nTunnelPort;
    eError = OMX_GetParameter(hTunneledComp, OMX_IndexParamPortDefinition, &MyPortDef);
    if (eError != OMX_ErrorNone) {
        VPP_DPRINT("VPP::Error 0x%X\n",eError);
        return eError;
    }

    switch(pPortDef->eDomain)
    {
    case OMX_PortDomainOther:
        if (MyPortDef.format.other.eFormat!= pPortDef->format.other.eFormat) {
            pPort->hTunnelComponent  = 0; 
            pPort->nTunnelPort       = 0;
            return OMX_ErrorPortsNotCompatible;
        }
        break;
    case OMX_PortDomainAudio:
        if (MyPortDef.format.audio.eEncoding != pPortDef->format.audio.eEncoding) {
            pPort->hTunnelComponent = 0; 
            pPort->nTunnelPort      = 0;
            return OMX_ErrorPortsNotCompatible;
        }
        break;
    case OMX_PortDomainVideo:
        VPP_DPRINT("my eColorFormat is %d, partner is %d\n", 
        MyPortDef.format.video.eColorFormat, 
        pPortDef->format.video.eColorFormat);
        /* The program should check the colorformat for tunneled components as the code shown here.
        * However, because of big-endian/little-endian issue, we just ignore the format checking 
        * as this moment
        if (MyPortDef.format.video.eColorFormat != pPortDef->format.video.eColorFormat) 
        {
            pPort->hTunnelComponent = 0; 
            pPort->nTunnelPort      = 0;
            return OMX_ErrorPortsNotCompatible;
        }
        */
        break;
    case OMX_PortDomainImage:
        if (MyPortDef.format.image.eCompressionFormat != pPortDef->format.image.eCompressionFormat) {
            pPort->hTunnelComponent = 0; 
            pPort->nTunnelPort      = 0;
            return OMX_ErrorPortsNotCompatible;
        }
        break;
    default: 
        pPort->hTunnelComponent     = 0;
        pPort->nTunnelPort          = 0;
        return OMX_ErrorPortsNotCompatible; /* Our current port is not set up correctly */
    }
EXIT:
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
  *         OMX_ErrorNotImplemented   
  **/
/*-------------------------------------------------------------------*/
static OMX_ERRORTYPE VPP_ComponentTunnelRequest (OMX_HANDLETYPE hComponent, 
                                                 OMX_U32 nPort, 
                                                 OMX_HANDLETYPE hTunneledComp, 
                                                 OMX_U32 nTunneledPort, 
                                                 OMX_INOUT  OMX_TUNNELSETUPTYPE* pTunnelSetup)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)hComponent;
    VPP_COMPONENT_PRIVATE *pComponentPrivate = NULL; 
    OMX_PARAM_BUFFERSUPPLIERTYPE sBufferSupplier;
    VPP_PORT_TYPE *pPort = NULL;
    
    OMX_CHECK_CMD(hComponent, OMX_TRUE, OMX_TRUE);

    pComponentPrivate = (VPP_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    pPort = &(pComponentPrivate->sCompPorts[nPort]);

    if (pTunnelSetup == NULL || hTunneledComp == 0) {
        /* cancel previous tunnel */
        pPort->hTunnelComponent = 0;
        pPort->nTunnelPort = 0;
        pPort->eSupplierSetting = OMX_BufferSupplyUnspecified;
        eError = OMX_ErrorNone;
        goto EXIT;
    }

    if (pComponentPrivate->sCompPorts[nPort].pPortDef.eDir != OMX_DirInput && 
            pComponentPrivate->sCompPorts[nPort].pPortDef.eDir != OMX_DirOutput) {
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }
    
    /* Check if the other component is developed by TI */
    if(IsTIOMXComponent(hTunneledComp) != OMX_TRUE) {
        VPP_DPRINT("OMX_ErrorTunnelingUnsupported\n");
        eError = OMX_ErrorTunnelingUnsupported;
        goto EXIT;
    }

    pPort->hTunnelComponent = hTunneledComp;
    pPort->nTunnelPort      = nTunneledPort;
    VPP_DPRINT("VPP comp = %x, tunneled comp = %x\n",(int)hComponent, (int)pPort->hTunnelComponent);

    if (pComponentPrivate->sCompPorts[nPort].pPortDef.eDir == OMX_DirOutput) {
        /* Component is the output (source of data) */
        pTunnelSetup->eSupplier = pPort->eSupplierSetting;
        VPP_DPRINT("VPP:: set output port supplier as OMX_BufferSupplyInput\n");
    }
    else { /* Component is the input (sink of data) */
        eError = VPP_VerifyTunnelConnection(pPort, hTunneledComp, &pComponentPrivate->sCompPorts[nPort].pPortDef);
        if (OMX_ErrorNone != eError) {
            VPP_DPRINT(" Error !! VPP VerifyTunnelConnection failed\n");
            /* Invalid connection formats. Return eError */
            return OMX_ErrorPortsNotCompatible;
        }
        /* If specified obey output port's preferences. Otherwise choose output */
        pPort->eSupplierSetting = pTunnelSetup->eSupplier;
        if (OMX_BufferSupplyUnspecified == pPort->eSupplierSetting) {
            pPort->eSupplierSetting = pTunnelSetup->eSupplier = OMX_BufferSupplyOutput;
        }

        /* Tell the output port who the supplier is */
        sBufferSupplier.nSize = sizeof(OMX_PARAM_BUFFERSUPPLIERTYPE);

        sBufferSupplier.nVersion.s.nVersionMajor = VPP_MAJOR_VER;
        sBufferSupplier.nVersion.s.nVersionMinor = VPP_MINOR_VER ;

        sBufferSupplier.nPortIndex      = nTunneledPort;
        sBufferSupplier.eBufferSupplier = pPort->eSupplierSetting;
        eError = OMX_SetParameter(hTunneledComp, OMX_IndexParamCompBufferSupplier, &sBufferSupplier);
        if(eError != OMX_ErrorNone){
            goto EXIT;
        }
    }
EXIT:
    return eError;
}

/*-------------------------------------------------------------------*/
/**
  * VPP_GetExtensionIndex() 
  *
  * Free a video driver buffer.
  *
  * @retval OMX_ErrorNone                    Successful operation.
  *         OMX_ErrorBadParameter            Invalid operation.    
  *         OMX_ErrorIncorrectStateOperation If called when port is disabled.
  **/
/*-------------------------------------------------------------------*/
OMX_ERRORTYPE VPP_GetExtensionIndex(OMX_IN OMX_HANDLETYPE hComponent, OMX_IN OMX_STRING cParameterName, OMX_OUT OMX_INDEXTYPE* pIndexType)
{
    int nIndex;
    OMX_ERRORTYPE eError = OMX_ErrorUndefined;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE*) hComponent;
    VPP_COMPONENT_PRIVATE* pComponentPrivate = NULL;

    /* Check parameter validity */    
    if (!pHandle) {
        eError = OMX_ErrorBadParameter; 
        goto EXIT; 
    }
        
    pComponentPrivate = (VPP_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;

    if (!pComponentPrivate) {
        eError = OMX_ErrorBadParameter; 
        goto EXIT; 
    }

    for (nIndex = 0; nIndex < VPP_NUM_CUSTOM_PARAMS; nIndex++) {
        if (!strcmp((const char *)cParameterName, (const char *)(&(sVPPCustomParams[nIndex].cCustomParamName)))) {
            *pIndexType = sVPPCustomParams[nIndex].nCustomParamIndex;
            eError = OMX_ErrorNone;
            break;
        }
    }
EXIT:
    return eError;
}

/*-------------------------------------------------------------------*/
/**
  *  ComponentRoleEnum()
  *
  *
  *
  *
  * @param
  * @param
  * @param
  *
  * @retval OMX_NoError              Success, ready to roll
  *         
  **/
/*-------------------------------------------------------------------*/
#ifdef KHRONOS_1_1

static OMX_ERRORTYPE ComponentRoleEnum(
        OMX_IN OMX_HANDLETYPE hComponent,
                OMX_OUT OMX_U8 *cRole,
                OMX_IN OMX_U32 nIndex)
{
    VPP_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    OMX_U8 *pTemp = NULL;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    pComponentPrivate = (VPP_COMPONENT_PRIVATE *)(((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);

    if(nIndex == 0){
        pTemp = memcpy(cRole, &(pComponentPrivate->componentRole.cRole), sizeof(OMX_U8) * OMX_MAX_STRINGNAME_SIZE - 1);
        if(pTemp == NULL){
            eError = OMX_ErrorUndefined;
            goto EXIT;
        }
    }
    else {
      eError = OMX_ErrorNoMore;
        }

EXIT:
    return eError;
}
#endif

