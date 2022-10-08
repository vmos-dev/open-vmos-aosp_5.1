/*
 * Copyright (c) 2010, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 *  @file  omx_proxy_videodecoder.c
 *         This file contains methods that provides the functionality for
 *         the OpenMAX1.1 DOMX Framework Tunnel Proxy component.
 *********************************************************************************************
 This is the proxy specific wrapper that passes the component name to the generic proxy init()
 The proxy wrapper also does some runtime/static time onfig on per proxy basis
 This is a thin wrapper that is called when componentiit() of the proxy is called
 static OMX_ERRORTYPE PROXY_Wrapper_init(OMX_HANDLETYPE hComponent, OMX_PTR pAppData);
 this layer gets called first whenever a proxy's get handle is called
 ************************************************************************************************
 *  @path WTSD_DucatiMMSW\omx\omx_il_1_x\omx_proxy_component\src
 *
 *  @rev 1.0
 */

/*==============================================================
 *! Revision History
 *! ============================
 *! 20-August-2010 Sarthak Aggarwal sarthak@ti.com: Initial Version
 *================================================================*/

/******************************************************************
 *   INCLUDE FILES
 ******************************************************************/
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "omx_proxy_common.h"
#include <timm_osal_interfaces.h>
#include "OMX_TI_IVCommon.h"
#include "OMX_TI_Video.h"
#include "OMX_TI_Index.h"

#ifdef ENABLE_RAW_BUFFERS_DUMP_UTILITY
#define LOG_TAG "OMXPROXYVIDEODEC"
#include <fcntl.h>
#include <cutils/properties.h>
#include <utils/Log.h>
#endif

#define COMPONENT_NAME "OMX.TI.DUCATI1.VIDEO.DECODER"
/* needs to be specific for every configuration wrapper */

#ifdef USE_ENHANCED_PORTRECONFIG
//Define port indices in video decoder proxy
#define OMX_VIDEODECODER_INPUT_PORT 0
#define OMX_VIDEODECODER_OUTPUT_PORT 1
#endif

#ifdef SET_STRIDE_PADDING_FROM_PROXY

#define LINUX_PAGE_SIZE (4 * 1024)
#define TOTAL_DEC_PORTS 2
#define HAL_NV12_PADDED_PIXEL_FORMAT (OMX_TI_COLOR_FormatYUV420PackedSemiPlanar - OMX_COLOR_FormatVendorStartUnused)

static OMX_ERRORTYPE RPC_UTIL_SetStrideAndPadding(OMX_COMPONENTTYPE * hRemoteComp, PROXY_COMPONENT_PRIVATE * pCompPrv);

OMX_ERRORTYPE PROXY_VIDDEC_SendCommand(OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_COMMANDTYPE eCmd,
    OMX_IN OMX_U32 nParam, OMX_IN OMX_PTR pCmdData);

OMX_ERRORTYPE PROXY_VIDDEC_EventHandler(OMX_HANDLETYPE hComponent,
    OMX_PTR pAppData, OMX_EVENTTYPE eEvent, OMX_U32 nData1, OMX_U32 nData2,
    OMX_PTR pEventData);

#endif //SET_STRIDE_PADDING_FROM_PROXY

OMX_ERRORTYPE PROXY_VIDDEC_GetExtensionIndex(OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_STRING cParameterName, OMX_OUT OMX_INDEXTYPE * pIndexType);

#ifdef ANDROID_QUIRK_CHANGE_PORT_VALUES

OMX_ERRORTYPE PROXY_VIDDEC_GetParameter(OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE nParamIndex, OMX_INOUT OMX_PTR pParamStruct);

OMX_ERRORTYPE PROXY_VIDDEC_SetParameter(OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE nParamIndex, OMX_INOUT OMX_PTR pParamStruct);

#endif

#ifdef ANDROID_QUIRK_LOCK_BUFFER
#include <hardware/gralloc.h>
#include <hardware/hardware.h>
#include "hal_public.h"

OMX_ERRORTYPE PROXY_VIDDEC_FillThisBuffer(OMX_HANDLETYPE hComponent, OMX_BUFFERHEADERTYPE * pBufferHdr);
OMX_ERRORTYPE PROXY_VIDDEC_FillBufferDone(OMX_HANDLETYPE hComponent,
    OMX_U32 remoteBufHdr, OMX_U32 nfilledLen, OMX_U32 nOffset, OMX_U32 nFlags,
    OMX_TICKS nTimeStamp, OMX_HANDLETYPE hMarkTargetComponent,
    OMX_PTR pMarkData);

#endif
extern OMX_ERRORTYPE PrearrageEmptyThisBuffer(OMX_HANDLETYPE hComponent,
	OMX_BUFFERHEADERTYPE * pBufferHdr);

#ifdef ENABLE_RAW_BUFFERS_DUMP_UTILITY
extern void DumpVideoFrame(DebugFrame_Dump *frameInfo);
#endif

OMX_ERRORTYPE OMX_ProxyViddecInit(OMX_HANDLETYPE hComponent);

OMX_ERRORTYPE OMX_ComponentInit(OMX_HANDLETYPE hComponent)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_COMPONENTTYPE *pHandle = NULL;
	PROXY_COMPONENT_PRIVATE *pComponentPrivate = NULL;
	pHandle = (OMX_COMPONENTTYPE *) hComponent;

	DOMX_ENTER("");

	DOMX_DEBUG("Component name provided is %s", COMPONENT_NAME);

	pHandle->pComponentPrivate =
	    (PROXY_COMPONENT_PRIVATE *)
	    TIMM_OSAL_Malloc(sizeof(PROXY_COMPONENT_PRIVATE), TIMM_OSAL_TRUE,
	    0, TIMMOSAL_MEM_SEGMENT_INT);

	PROXY_assert(pHandle->pComponentPrivate != NULL,
	    OMX_ErrorInsufficientResources,
	    "ERROR IN ALLOCATING PROXY COMPONENT PRIVATE STRUCTURE");

	pComponentPrivate =
	    (PROXY_COMPONENT_PRIVATE *) pHandle->pComponentPrivate;

	TIMM_OSAL_Memset(pComponentPrivate, 0,
		sizeof(PROXY_COMPONENT_PRIVATE));

	pComponentPrivate->cCompName =
	    TIMM_OSAL_Malloc(MAX_COMPONENT_NAME_LENGTH * sizeof(OMX_U8),
	    TIMM_OSAL_TRUE, 0, TIMMOSAL_MEM_SEGMENT_INT);

	PROXY_assert(pComponentPrivate->cCompName != NULL,
	    OMX_ErrorInsufficientResources,
	    " Error in Allocating space for proxy component table");

	eError = OMX_ProxyViddecInit(hComponent);

#ifdef ENABLE_RAW_BUFFERS_DUMP_UTILITY
	if (eError == OMX_ErrorNone)
	{
		char value[PROPERTY_VALUE_MAX];
		property_get("debug.video.dumpframe", value, "0:0");
		/* -ve value for fromFrame would disable this automatically */
		pComponentPrivate->debugframeInfo.fromFrame = atoi(strtok(value, ":"));
		pComponentPrivate->debugframeInfo.toFrame = atoi(strtok(NULL, ":"));
		pComponentPrivate->debugframeInfo.runningFrame = pComponentPrivate->debugframeInfo.fromFrame;
	}
#endif
	EXIT:
	if (eError != OMX_ErrorNone)
	{
		DOMX_DEBUG("Error in Initializing Proxy");
		if (pComponentPrivate->cCompName != NULL)
		{
			TIMM_OSAL_Free(pComponentPrivate->cCompName);
			pComponentPrivate->cCompName = NULL;
		}
		if (pComponentPrivate != NULL)
		{
			TIMM_OSAL_Free(pComponentPrivate);
			pComponentPrivate = NULL;
		}
	}
		return eError;
}

OMX_ERRORTYPE OMX_ProxyViddecInit(OMX_HANDLETYPE hComponent)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_COMPONENTTYPE *pHandle = NULL;
	PROXY_COMPONENT_PRIVATE *pComponentPrivate = NULL;
	pHandle = (OMX_COMPONENTTYPE *) hComponent;
        OMX_TI_PARAM_ENHANCEDPORTRECONFIG tParamStruct;

#ifdef ANDROID_QUIRK_LOCK_BUFFER
	OMX_U32 err;
	hw_module_t const* module;
#endif
	DOMX_ENTER("");

	DOMX_DEBUG("Component name provided is %s", COMPONENT_NAME);

        pComponentPrivate =
            (PROXY_COMPONENT_PRIVATE *) pHandle->pComponentPrivate;

	// Copying component Name - this will be picked up in the proxy common
	PROXY_assert(strlen(COMPONENT_NAME) + 1 < MAX_COMPONENT_NAME_LENGTH,
	    OMX_ErrorInvalidComponentName,
	    "Length of component name is longer than the max allowed");
	TIMM_OSAL_Memcpy(pComponentPrivate->cCompName, COMPONENT_NAME,
	    strlen(COMPONENT_NAME) + 1);

	eError = OMX_ProxyCommonInit(hComponent);	// Calling Proxy Common Init()
	PROXY_assert(eError == OMX_ErrorNone, eError, "Proxy common init returned error");
#ifdef ANDROID_QUIRK_CHANGE_PORT_VALUES
	pHandle->SetParameter = PROXY_VIDDEC_SetParameter;
        pHandle->GetParameter = PROXY_VIDDEC_GetParameter;
#endif
	pHandle->GetExtensionIndex = PROXY_VIDDEC_GetExtensionIndex;

#ifdef  SET_STRIDE_PADDING_FROM_PROXY
        pHandle->SendCommand = PROXY_VIDDEC_SendCommand;
	pComponentPrivate->proxyEventHandler = PROXY_VIDDEC_EventHandler;
	pComponentPrivate->IsLoadedState = OMX_TRUE;
#endif

#ifdef ANDROID_QUIRK_LOCK_BUFFER
        pComponentPrivate->proxyFillBufferDone = PROXY_VIDDEC_FillBufferDone;
	pHandle->FillThisBuffer = PROXY_VIDDEC_FillThisBuffer;

        err = hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &module);
        if (err == 0)
	{
                pComponentPrivate->grallocModule = (gralloc_module_t const *)module;
        }
	else
	{
                DOMX_ERROR("FATAL: gralloc api hw_get_module() returned error: Can't find \
		 %s module err = %x", GRALLOC_HARDWARE_MODULE_ID, err);
		eError = OMX_ErrorInsufficientResources;
		return eError;
	}
#endif

#ifdef USE_ENHANCED_PORTRECONFIG
	/*Initializing Structure */
	tParamStruct.nSize = sizeof(OMX_TI_PARAM_ENHANCEDPORTRECONFIG);
	tParamStruct.nVersion.s.nVersionMajor = OMX_VER_MAJOR;
	tParamStruct.nVersion.s.nVersionMinor = OMX_VER_MINOR;
	tParamStruct.nVersion.s.nRevision = 0x0;
	tParamStruct.nVersion.s.nStep = 0x0;
	tParamStruct.nPortIndex = OMX_VIDEODECODER_OUTPUT_PORT;
        tParamStruct.bUsePortReconfigForCrop = OMX_TRUE;
        tParamStruct.bUsePortReconfigForPadding = OMX_TRUE;

	eError = PROXY_SetParameter(hComponent,(OMX_INDEXTYPE)OMX_TI_IndexParamUseEnhancedPortReconfig,
	   &tParamStruct);
	PROXY_assert(eError == OMX_ErrorNone,
	    eError," Error in Proxy SetParameter for Enhanced port reconfig usage");
#endif
	/* This is required to handle WMV/VC-1 content */
	pHandle->EmptyThisBuffer = PrearrageEmptyThisBuffer;

      EXIT:
	return eError;
}

/* ===========================================================================*/
/**
 * @name PROXY_VIDDEC_GetExtensionIndex()
 * @brief
 * @param void
 * @return OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
OMX_ERRORTYPE PROXY_VIDDEC_GetExtensionIndex(OMX_IN OMX_HANDLETYPE hComponent,
		OMX_IN OMX_STRING cParameterName, OMX_OUT OMX_INDEXTYPE * pIndexType)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	PROXY_COMPONENT_PRIVATE *pCompPrv = NULL;
	OMX_COMPONENTTYPE *hComp = (OMX_COMPONENTTYPE *) hComponent;

	PROXY_require((hComp->pComponentPrivate != NULL), OMX_ErrorBadParameter, NULL);
	PROXY_require(cParameterName != NULL, OMX_ErrorBadParameter, NULL);
	PROXY_require(pIndexType != NULL, OMX_ErrorBadParameter, NULL);

	DOMX_ENTER("hComponent = %p, cParameterName = %p", hComponent, cParameterName);

#ifdef ENABLE_GRALLOC_BUFFERS
	// Ensure that String length is not greater than Max allowed length
	PROXY_require(strlen(cParameterName) <= 127, OMX_ErrorBadParameter, NULL);

	if (strcmp(cParameterName, "OMX.google.android.index.getAndroidNativeBufferUsage") == 0)
	{
		*pIndexType = (OMX_INDEXTYPE) OMX_TI_IndexAndroidNativeBufferUsage;
	}
	else
	{
		eError = PROXY_GetExtensionIndex(hComponent, cParameterName, pIndexType);
		PROXY_assert(eError == OMX_ErrorNone,
		    eError," Error in PROXY_GetExtensionIndex");
	}
#else
	eError = PROXY_GetExtensionIndex(hComponent, cParameterName, pIndexType);
	PROXY_assert(eError == OMX_ErrorNone,
		eError," Error in PROXY_GetExtensionIndex");
#endif
	EXIT:
	DOMX_EXIT("eError: %d", eError);
	return eError;
}

#ifdef  ANDROID_QUIRK_CHANGE_PORT_VALUES

/* ===========================================================================*/
/**
 * @name PROXY_GetParameter()
 * @brief
 * @param void
 * @return OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
OMX_ERRORTYPE PROXY_VIDDEC_GetParameter(OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE nParamIndex, OMX_INOUT OMX_PTR pParamStruct)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	PROXY_COMPONENT_PRIVATE *pCompPrv = NULL;
	OMX_COMPONENTTYPE *hComp = (OMX_COMPONENTTYPE *) hComponent;
	OMX_PARAM_PORTDEFINITIONTYPE* pPortDef = NULL;
	OMX_VIDEO_PARAM_PORTFORMATTYPE* pPortParam = NULL;
	OMX_TI_PARAMNATIVEBUFFERUSAGE *pUsage = NULL;

	PROXY_require((pParamStruct != NULL), OMX_ErrorBadParameter, NULL);
	PROXY_assert((hComp->pComponentPrivate != NULL),
	    OMX_ErrorBadParameter, NULL);

	pCompPrv = (PROXY_COMPONENT_PRIVATE *) hComp->pComponentPrivate;

	DOMX_ENTER
	    ("hComponent = %p, pCompPrv = %p, nParamIndex = %d, pParamStruct = %p",
	    hComponent, pCompPrv, nParamIndex, pParamStruct);

#ifdef ENABLE_GRALLOC_BUFFERS
	if( nParamIndex == OMX_TI_IndexAndroidNativeBufferUsage)
	{
		pUsage = (OMX_TI_PARAMNATIVEBUFFERUSAGE*)pParamStruct;
		if(pCompPrv->proxyPortBuffers[pUsage->nPortIndex].proxyBufferType == GrallocPointers)
		{
			PROXY_CHK_VERSION(pParamStruct, OMX_TI_PARAMNATIVEBUFFERUSAGE);
			pUsage->nUsage = GRALLOC_USAGE_HW_RENDER;
#ifdef ENABLE_RAW_BUFFERS_DUMP_UTILITY
			pUsage->nUsage |= GRALLOC_USAGE_SW_READ_RARELY;
#endif
			goto EXIT;
		}
	}
#endif
	eError = PROXY_GetParameter(hComponent,nParamIndex, pParamStruct);
	PROXY_assert(eError == OMX_ErrorNone,
		    eError," Error in Proxy GetParameter");

	if( nParamIndex == OMX_IndexParamPortDefinition)
	{
		PROXY_CHK_VERSION(pParamStruct, OMX_PARAM_PORTDEFINITIONTYPE);
		pPortDef = (OMX_PARAM_PORTDEFINITIONTYPE *)pParamStruct;
		if(pPortDef->format.video.eColorFormat == OMX_COLOR_FormatYUV420PackedSemiPlanar)
		{
			if(pCompPrv->proxyPortBuffers[pPortDef->nPortIndex].proxyBufferType == GrallocPointers)
			{
				pPortDef->format.video.eColorFormat = HAL_NV12_PADDED_PIXEL_FORMAT;
			}
			else
			{
				pPortDef->format.video.eColorFormat = OMX_TI_COLOR_FormatYUV420PackedSemiPlanar;
			}
		}
	}
	else if ( nParamIndex == OMX_IndexParamVideoPortFormat)
	{
		PROXY_CHK_VERSION(pParamStruct, OMX_VIDEO_PARAM_PORTFORMATTYPE);
		pPortParam = (OMX_VIDEO_PARAM_PORTFORMATTYPE *)pParamStruct;
		if(pPortParam->eColorFormat == OMX_COLOR_FormatYUV420PackedSemiPlanar)
		{
			if(pCompPrv->proxyPortBuffers[pPortParam->nPortIndex].proxyBufferType == GrallocPointers)
			{
				pPortParam->eColorFormat = HAL_NV12_PADDED_PIXEL_FORMAT;
			}
			else
			{
				pPortParam->eColorFormat = OMX_TI_COLOR_FormatYUV420PackedSemiPlanar;
			}
		}
	}

      EXIT:
	DOMX_EXIT("eError: %d", eError);
	return eError;
}

/* ===========================================================================*/
/**
 * @name PROXY_SetParameter()
 * @brief
 * @param void
 * @return OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
OMX_ERRORTYPE PROXY_VIDDEC_SetParameter(OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE nParamIndex, OMX_IN OMX_PTR pParamStruct)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	PROXY_COMPONENT_PRIVATE *pCompPrv = NULL;
	OMX_COMPONENTTYPE *hComp = (OMX_COMPONENTTYPE *) hComponent;
	OMX_PARAM_PORTDEFINITIONTYPE* pPortDef = (OMX_PARAM_PORTDEFINITIONTYPE *)pParamStruct;
	OMX_VIDEO_PARAM_PORTFORMATTYPE* pPortParams = (OMX_VIDEO_PARAM_PORTFORMATTYPE *)pParamStruct;

	PROXY_require((pParamStruct != NULL), OMX_ErrorBadParameter, NULL);
	PROXY_require((hComp->pComponentPrivate != NULL),
	    OMX_ErrorBadParameter, NULL);

	pCompPrv = (PROXY_COMPONENT_PRIVATE *) hComp->pComponentPrivate;
	DOMX_ENTER
	    ("hComponent = %p, pCompPrv = %p, nParamIndex = %d, pParamStruct = %p",
	    hComponent, pCompPrv, nParamIndex, pParamStruct);
	if(nParamIndex == OMX_IndexParamPortDefinition)
	{
		if(pPortDef->format.video.eColorFormat == OMX_TI_COLOR_FormatYUV420PackedSemiPlanar
			|| pPortDef->format.video.eColorFormat == HAL_NV12_PADDED_PIXEL_FORMAT)
		{
			pPortDef->format.video.eColorFormat = OMX_COLOR_FormatYUV420PackedSemiPlanar;
		}
	}
	else if(nParamIndex == OMX_IndexParamVideoPortFormat)
	{
		if(pPortParams->eColorFormat == OMX_TI_COLOR_FormatYUV420PackedSemiPlanar
			|| pPortParams->eColorFormat == HAL_NV12_PADDED_PIXEL_FORMAT)
		{
			pPortParams->eColorFormat = OMX_COLOR_FormatYUV420PackedSemiPlanar;
		}
	}

	eError = PROXY_SetParameter(hComponent, nParamIndex, pParamStruct);
	PROXY_assert(eError == OMX_ErrorNone,
		    eError," Error in Proxy SetParameter");

	EXIT:
	DOMX_EXIT("eError: %d", eError);
	return eError;
}

#endif

#ifdef SET_STRIDE_PADDING_FROM_PROXY
/* ===========================================================================*/
/**
 * @name PROXY_VIDDEC_SendCommand()
 * @brief
 * @return OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
OMX_ERRORTYPE PROXY_VIDDEC_SendCommand(OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_COMMANDTYPE eCmd,
    OMX_IN OMX_U32 nParam, OMX_IN OMX_PTR pCmdData)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	PROXY_COMPONENT_PRIVATE *pCompPrv = NULL;
	OMX_COMPONENTTYPE *hComp = (OMX_COMPONENTTYPE *) hComponent;

	PROXY_require((hComp->pComponentPrivate != NULL),
	    OMX_ErrorBadParameter, NULL);

	pCompPrv = (PROXY_COMPONENT_PRIVATE *) hComp->pComponentPrivate;

	DOMX_ENTER
	    ("hComponent = %p, pCompPrv = %p, eCmd = %d, nParam = %d, pCmdData = %p",
	    hComponent, pCompPrv, eCmd, nParam, pCmdData);

	if(eCmd == OMX_CommandStateSet)
	{
		//Set appropriate stride before Loaded to Idle transition.
		if((OMX_STATETYPE)nParam == OMX_StateIdle && pCompPrv->IsLoadedState == OMX_TRUE)
		{
			eError = RPC_UTIL_SetStrideAndPadding(hComponent, pCompPrv);
			PROXY_require(eError == OMX_ErrorNone, eError,
                               "Stride and padding setting from proxy returned");
			pCompPrv->IsLoadedState = OMX_FALSE;
		}
	}
	else if(eCmd == OMX_CommandPortEnable)
	{
		if(nParam == OMX_ALL || nParam == OMX_VIDEODECODER_OUTPUT_PORT)
		{
			eError = RPC_UTIL_SetStrideAndPadding(hComponent, pCompPrv);
			PROXY_require(eError == OMX_ErrorNone, eError,
                               "Stride and padding setting from proxy returned");
		}
	}

	eError =
	    PROXY_SendCommand(hComponent, eCmd, nParam, pCmdData);

      EXIT:
	DOMX_EXIT("eError: %d", eError);
	return eError;
}

/* ===========================================================================*/
/**
 * @name PROXY_EventHandler()
 * @brief
 * @param void
 * @return OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
OMX_ERRORTYPE PROXY_VIDDEC_EventHandler(OMX_HANDLETYPE hComponent,
    OMX_PTR pAppData, OMX_EVENTTYPE eEvent, OMX_U32 nData1, OMX_U32 nData2,
    OMX_PTR pEventData)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	PROXY_COMPONENT_PRIVATE *pCompPrv = NULL;
	OMX_COMPONENTTYPE *hComp = (OMX_COMPONENTTYPE *) hComponent;

	pCompPrv = (PROXY_COMPONENT_PRIVATE *) hComp->pComponentPrivate;

	PROXY_require((hComp->pComponentPrivate != NULL),
	    OMX_ErrorBadParameter,
	    "This is fatal error, processing cant proceed - please debug");

	DOMX_ENTER
	    ("hComponent=%p, pCompPrv=%p, eEvent=%p, nData1=%p, nData2=%p, pEventData=%p",
	    hComponent, pCompPrv, eEvent, nData1, nData2, pEventData);

	if((eEvent ==  OMX_EventCmdComplete) && ((OMX_COMMANDTYPE)nData1 == OMX_CommandStateSet))
	{
		if((OMX_STATETYPE)nData2 == OMX_StateLoaded)
		{
			pCompPrv->IsLoadedState = OMX_TRUE;
		}
	}
	eError = PROXY_EventHandler(hComponent, pAppData, eEvent, nData1, nData2, pEventData);

	EXIT:
		return eError;
}

/* ===========================================================================*/
/**
 * @name RPC_UTIL_RPC_UTIL_SetStrideAndPadding()
 * @brief Gets stride on this port. Used to set stride on OMX to tell whether buffer is 1D or 2D
 * @param hRemoteComp [IN]  : Remote component handle.
 * @return OMX_ErrorNone = Successful
 */
/* ===========================================================================*/
OMX_ERRORTYPE RPC_UTIL_SetStrideAndPadding(OMX_COMPONENTTYPE * hComponent,PROXY_COMPONENT_PRIVATE * pCompPrv)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_PARAM_PORTDEFINITIONTYPE sPortDef;
	OMX_CONFIG_RECTTYPE tParamStruct;
	OMX_U32 nPortIndex = 0;

	for(nPortIndex=0; nPortIndex < TOTAL_DEC_PORTS ;nPortIndex++ )
	{
		/*Initializing Structure */
		sPortDef.nSize = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
		sPortDef.nVersion.s.nVersionMajor = OMX_VER_MAJOR;
		sPortDef.nVersion.s.nVersionMinor = OMX_VER_MINOR;
		sPortDef.nVersion.s.nRevision = 0x0;
		sPortDef.nVersion.s.nStep = 0x0;
		sPortDef.nPortIndex = nPortIndex;

		eError = PROXY_GetParameter(hComponent,OMX_IndexParamPortDefinition,
			   &sPortDef);
		PROXY_assert(eError == OMX_ErrorNone,
			    eError," Error in Proxy GetParameter for Port Def");

		if (sPortDef.eDomain == OMX_PortDomainVideo && sPortDef.format.video.eCompressionFormat == OMX_VIDEO_CodingUnused)
		{
			if(pCompPrv->proxyPortBuffers[nPortIndex].IsBuffer2D == OMX_TRUE)
			{
				sPortDef.format.video.nStride = LINUX_PAGE_SIZE;
			}
			else
			{
				tParamStruct.nSize = sizeof(OMX_CONFIG_RECTTYPE);
				tParamStruct.nVersion.s.nVersionMajor = OMX_VER_MAJOR;
				tParamStruct.nVersion.s.nVersionMinor = OMX_VER_MINOR;
				tParamStruct.nVersion.s.nRevision = 0x0;
				tParamStruct.nVersion.s.nStep = 0x0;
				tParamStruct.nPortIndex = nPortIndex;

				eError = PROXY_GetParameter(hComponent,(OMX_INDEXTYPE)OMX_TI_IndexParam2DBufferAllocDimension,
						  &tParamStruct);
				PROXY_assert(eError == OMX_ErrorNone,
					    eError," Error in Proxy GetParameter for 2D index");

				sPortDef.format.video.nStride = tParamStruct.nWidth;
			}
			eError = PROXY_SetParameter(hComponent,OMX_IndexParamPortDefinition,
				   &sPortDef);
			PROXY_assert(eError == OMX_ErrorNone,
				    eError," Error in Proxy SetParameter for Port Def");
		}
	}

      EXIT:
	return eError;
}

#endif

#ifdef ANDROID_QUIRK_LOCK_BUFFER
/* ===========================================================================*/
/**
 * @name PROXY_VIDDEC_FillThisBuffer()
 * @brief Gets stride on this port. Used to set stride on OMX to tell whether buffer is 1D or 2D
 */
/* ===========================================================================*/
OMX_ERRORTYPE PROXY_VIDDEC_FillThisBuffer(OMX_HANDLETYPE hComponent, OMX_BUFFERHEADERTYPE * pBufferHdr)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone, eCompReturn = OMX_ErrorNone;
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	PROXY_COMPONENT_PRIVATE *pCompPrv;
	OMX_COMPONENTTYPE *hComp = (OMX_COMPONENTTYPE *) hComponent;
	OMX_U32 count = 0;
	IMG_native_handle_t*  grallocHandle;
	OMX_PARAM_PORTDEFINITIONTYPE sPortDef;

	PROXY_require(pBufferHdr != NULL, OMX_ErrorBadParameter, NULL);
	PROXY_require(hComp->pComponentPrivate != NULL, OMX_ErrorBadParameter,
	    NULL);
	PROXY_CHK_VERSION(pBufferHdr, OMX_BUFFERHEADERTYPE);

	pCompPrv = (PROXY_COMPONENT_PRIVATE *) hComp->pComponentPrivate;

	if(pCompPrv->proxyPortBuffers[OMX_VIDEODECODER_OUTPUT_PORT].proxyBufferType
			== GrallocPointers)
	{
		/* Lock the Gralloc buffer till it gets rendered completely */
		/* Extract the Gralloc handle from the Header and then call lock on that */
		/* Note# There is no error check for the pBufferHdr here*/
		grallocHandle = (IMG_native_handle_t*)pBufferHdr->pBuffer;

		/*Initializing Structure */
		sPortDef.nSize = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
		sPortDef.nVersion.s.nVersionMajor = OMX_VER_MAJOR;
		sPortDef.nVersion.s.nVersionMinor = OMX_VER_MINOR;
		sPortDef.nVersion.s.nRevision = 0x0;
		sPortDef.nVersion.s.nStep = 0x0;
		sPortDef.nPortIndex = OMX_VIDEODECODER_INPUT_PORT;
		eError = PROXY_GetParameter(hComponent,OMX_IndexParamPortDefinition,
				&sPortDef);
		PROXY_assert(eError == OMX_ErrorNone,
				eError," Error in Proxy GetParameter for Port Def");

#ifdef ENABLE_RAW_BUFFERS_DUMP_UTILITY
		/* Get the Video frame crop window */
		OMX_CONFIG_RECTTYPE rect;
		rect.nSize = sizeof(rect);
		rect.nVersion.s.nVersionMajor = OMX_VER_MAJOR;
		rect.nVersion.s.nVersionMinor = OMX_VER_MINOR;
		rect.nVersion.s.nRevision = 0x0;
		rect.nVersion.s.nStep = 0x0;
		rect.nPortIndex = OMX_VIDEODECODER_OUTPUT_PORT;

		eError = PROXY_GetConfig(hComponent, OMX_IndexConfigCommonOutputCrop, &rect);

		PROXY_assert(eError == OMX_ErrorNone,
			eError," Error while getting output crop");
		pCompPrv->debugframeInfo.frame_width = rect.nWidth;
		pCompPrv->debugframeInfo.frame_height = rect.nHeight;
		pCompPrv->debugframeInfo.frame_xoffset = rect.nLeft;
		pCompPrv->debugframeInfo.frame_yoffset = rect.nTop;
#endif
		pCompPrv->grallocModule->lock((gralloc_module_t const *) pCompPrv->grallocModule,
				(buffer_handle_t)grallocHandle, GRALLOC_USAGE_HW_RENDER,
				0,0,sPortDef.format.video.nFrameWidth, sPortDef.format.video.nFrameHeight,NULL);
	}

        eRPCError = PROXY_FillThisBuffer(hComponent, pBufferHdr);

	PROXY_assert(eError == OMX_ErrorNone,
	    eError," Error in Proxy SetParameter for Port Def");

      EXIT:
	DOMX_EXIT("eError: %d", eError);
	return eError;
}

/* ===========================================================================*/
/**
 * @name PROXY_VIDDEC_FillBufferDone()
 * @brief Gets stride on this port. Used to set stride on OMX to tell whether buffer is 1D or 2D
 */
/* ===========================================================================*/
OMX_ERRORTYPE PROXY_VIDDEC_FillBufferDone(OMX_HANDLETYPE hComponent,
    OMX_U32 remoteBufHdr, OMX_U32 nfilledLen, OMX_U32 nOffset, OMX_U32 nFlags,
    OMX_TICKS nTimeStamp, OMX_HANDLETYPE hMarkTargetComponent,
    OMX_PTR pMarkData)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone, eCompReturn = OMX_ErrorNone;
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	PROXY_COMPONENT_PRIVATE *pCompPrv = NULL;
	OMX_COMPONENTTYPE *hComp = (OMX_COMPONENTTYPE *) hComponent;
	OMX_U32 count = 0;
	IMG_native_handle_t*  grallocHandle;

	PROXY_require((hComp->pComponentPrivate != NULL),
			OMX_ErrorBadParameter,
			"This is fatal error, processing cant proceed - please debug");

	pCompPrv = (PROXY_COMPONENT_PRIVATE *) hComp->pComponentPrivate;

	/* Lock the Gralloc buffer till it gets rendered completely */
	/* Extract the Gralloc handle from the Header and then call lock on that */
	/* Note# There is no error check for the pBufferHdr here*/

	if(pCompPrv->proxyPortBuffers[OMX_VIDEODECODER_OUTPUT_PORT].proxyBufferType
			== GrallocPointers) {
		for (count = 0; count < pCompPrv->nTotalBuffers; ++count)
		{
			if (pCompPrv->tBufList[count].pBufHeaderRemote == remoteBufHdr)
			{
				grallocHandle = (IMG_native_handle_t*)(pCompPrv->tBufList[count].pBufHeader)->pBuffer;
				break;
			}
		}

		PROXY_assert((count != pCompPrv->nTotalBuffers),
				OMX_ErrorBadParameter,
				"Received invalid-buffer header from OMX component");
		pCompPrv->grallocModule->unlock((gralloc_module_t const *) pCompPrv->grallocModule, (buffer_handle_t)grallocHandle);

#ifdef ENABLE_RAW_BUFFERS_DUMP_UTILITY
		ALOGV("frm[%u] to[%u] run[%u]", pCompPrv->debugframeInfo.fromFrame, pCompPrv->debugframeInfo.toFrame, pCompPrv->debugframeInfo.runningFrame);
		/* Fill buffer Done successed, hence start dumping if requested	*/
		OMX_BUFFERHEADERTYPE *pBufHdr = pCompPrv->tBufList[count].pBufHeader;
		if ((pCompPrv->debugframeInfo.fromFrame == 0) && (pCompPrv->debugframeInfo.runningFrame <= pCompPrv->debugframeInfo.toFrame))
		{
			/* Lock the buffer for SW read usage and then access it */
			pCompPrv->grallocModule->lock((gralloc_module_t const*) pCompPrv->grallocModule,
				(buffer_handle_t)grallocHandle,
				GRALLOC_USAGE_HW_RENDER | GRALLOC_USAGE_SW_READ_RARELY,
				pCompPrv->debugframeInfo.frame_xoffset,
                                pCompPrv->debugframeInfo.frame_yoffset,
				pCompPrv->debugframeInfo.frame_width,
                                pCompPrv->debugframeInfo.frame_height,
				(void*)pCompPrv->debugframeInfo.y_uv);

			DumpVideoFrame(&pCompPrv->debugframeInfo);

			pCompPrv->grallocModule->unlock((gralloc_module_t const *) pCompPrv->grallocModule,
							(buffer_handle_t)grallocHandle);
			pCompPrv->debugframeInfo.runningFrame++;
		}
		else if (pCompPrv->debugframeInfo.fromFrame > 0)
		{
			pCompPrv->debugframeInfo.fromFrame--;
		}
#endif
	}

	eRPCError = PROXY_FillBufferDone(hComponent,remoteBufHdr, nfilledLen, nOffset, nFlags,
		nTimeStamp, hMarkTargetComponent, pMarkData);

	PROXY_assert(eError == OMX_ErrorNone,
			eError," Error in PROXY FillBufferDone for Port Def");

EXIT:
	DOMX_EXIT("eError: %d", eError);
	return eError;
}

#endif

