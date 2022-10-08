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
 *  @file  omx_proxy_common.c
 *         This file contains methods that provides the functionality for
 *         the OpenMAX1.1 DOMX Framework OMX Common Proxy .
 *
 *  @path \WTSD_DucatiMMSW\framework\domx\omx_proxy_common\src
 *
 *  @rev 1.0
 */

/*==============================================================
 *! Revision History
 *! ============================
 *! 29-Mar-2010 Abhishek Ranka : Revamped DOMX implementation
 *!
 *! 19-August-2009 B Ravi Kiran ravi.kiran@ti.com: Initial Version
 *================================================================*/

/* ------compilation control switches ----------------------------------------*/
#define TILER_BUFF
#define ALLOCATE_TILER_BUFFER_IN_PROXY
// This has been enabled enbled only in Android.mk
// #define ENABLE_GRALLOC_BUFFER
/******************************************************************
 *   INCLUDE FILES
 ******************************************************************/
/* ----- system and platform files ----------------------------*/
#include <string.h>

#include "timm_osal_memory.h"
#include "timm_osal_mutex.h"
#include "OMX_TI_Common.h"
#include "OMX_TI_Index.h"
#include "OMX_TI_Core.h"
/*-------program files ----------------------------------------*/
#include "omx_proxy_common.h"
#include "omx_rpc.h"
#include "omx_rpc_stub.h"
#include "omx_rpc_utils.h"
#include "OMX_TI_IVCommon.h"
#include "profile.h"

#ifdef ALLOCATE_TILER_BUFFER_IN_PROXY

#ifdef USE_ION
#include <unistd.h>
#include <ion/ion.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/eventfd.h>
#include <fcntl.h>
#include <linux/rpmsg_omx.h>
#include <errno.h>
#endif

#endif

#ifdef  ENABLE_GRALLOC_BUFFERS
#include "native_handle.h"
#include "hal_public.h"
#endif

#ifdef TILER_BUFF
#define PortFormatIsNotYUV 0

static OMX_ERRORTYPE _RPC_IsProxyComponent(OMX_HANDLETYPE hComponent,
    OMX_BOOL * bIsProxy);
OMX_ERRORTYPE RPC_UTIL_GetStride(OMX_COMPONENTTYPE * hRemoteComp,
    OMX_U32 nPortIndex, OMX_U32 * nStride);
OMX_ERRORTYPE RPC_UTIL_GetNumLines(OMX_COMPONENTTYPE * hComp,
    OMX_U32 nPortIndex, OMX_U32 * nNumOfLines);

#endif

#ifdef ALLOCATE_TILER_BUFFER_IN_PROXY

static OMX_ERRORTYPE PROXY_UseBuffer(OMX_IN OMX_HANDLETYPE hComponent,
    OMX_INOUT OMX_BUFFERHEADERTYPE ** ppBufferHdr,
    OMX_IN OMX_U32 nPortIndex, OMX_IN OMX_PTR pAppPrivate,
    OMX_IN OMX_U32 nSizeBytes, OMX_IN OMX_U8 * pBuffer);
#endif

#define LINUX_PAGE_SIZE 4096
#define MAXNAMESIZE 128

#define CORE_MAX 4
#define CORE_CHIRON 3
#define CORE_SYSM3 2
#define CORE_APPM3 1
#define CORE_TESLA 0

#define MAX_CORENAME_LENGTH 32
char Core_Array[][MAX_CORENAME_LENGTH] =
    { "TESLA", "DUCATI1", "DUCATI0", "CHIRON" };

/******************************************************************
 *   MACROS - LOCAL
 ******************************************************************/

#ifdef USE_ION


RPC_OMX_ERRORTYPE RPC_RegisterBuffer(OMX_HANDLETYPE hRPCCtx, int fd,
				     OMX_PTR *handle1, OMX_PTR *handle2,
				     PROXY_BUFFER_TYPE proxyBufferType)
{
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	int status;
	RPC_OMX_CONTEXT *pRPCCtx = (RPC_OMX_CONTEXT *) hRPCCtx;

	if ((fd < 0) || (handle1 ==  NULL) ||
	    ((proxyBufferType == GrallocPointers) && (handle2 ==  NULL))) {
		eRPCError = RPC_OMX_ErrorBadParameter;
		goto EXIT;
	}

	if (proxyBufferType != GrallocPointers) {
		struct ion_fd_data ion_data;

		ion_data.fd = fd;
		ion_data.handle = NULL;
		status = ioctl(pRPCCtx->fd_omx, OMX_IOCIONREGISTER, &ion_data);
		if (status < 0) {
			DOMX_ERROR("RegisterBuffer ioctl call failed");
			eRPCError = RPC_OMX_ErrorInsufficientResources;
			goto EXIT;
		}
		if (ion_data.handle)
			*handle1 = ion_data.handle;
	} else {
#ifdef OMX_IOCPVRREGISTER
		struct omx_pvr_data pvr_data;

		pvr_data.fd = fd;
		memset(pvr_data.handles, 0x0, sizeof(pvr_data.handles));
		status = ioctl(pRPCCtx->fd_omx, OMX_IOCPVRREGISTER, &pvr_data);
		if (status < 0) {
			if (errno == ENOTTY) {
				DOMX_DEBUG("OMX_IOCPVRREGISTER not supported with current kernel version");
			} else {
				DOMX_ERROR("RegisterBuffer ioctl call failed");
				eRPCError = RPC_OMX_ErrorInsufficientResources;
			}
			goto EXIT;
		}

		if (pvr_data.handles[0])
			*handle1 = pvr_data.handles[0];
		if (pvr_data.handles[1])
			*handle2 = pvr_data.handles[1];
#endif
	}


 EXIT:
	return eRPCError;
}

RPC_OMX_ERRORTYPE RPC_UnRegisterBuffer(OMX_HANDLETYPE hRPCCtx, OMX_PTR handle)
{
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	int status;
	struct ion_fd_data data;
	RPC_OMX_CONTEXT *pRPCCtx = (RPC_OMX_CONTEXT *) hRPCCtx;

	if (handle ==  NULL) {
		eRPCError = RPC_OMX_ErrorBadParameter;
		goto EXIT;
	}

	data.handle = handle;
	status = ioctl(pRPCCtx->fd_omx, OMX_IOCIONUNREGISTER, &data);
	if (status < 0) {
		eRPCError = RPC_OMX_ErrorInsufficientResources;
		goto EXIT;
	}

 EXIT:
	return eRPCError;
}

static OMX_ERRORTYPE PROXY_AllocateBufferIonCarveout(PROXY_COMPONENT_PRIVATE *pCompPrv,
						 size_t len, struct ion_handle **handle)
{
	int fd;
	int ret;
	struct ion_handle *temp;
	size_t stride;

	ret = ion_alloc(pCompPrv->ion_fd, len, 0x1000, 1 << ION_HEAP_TYPE_CARVEOUT, &temp);

       if (ret || ((int)temp == -ENOMEM)) {
               ret = ion_alloc_tiler(pCompPrv->ion_fd, len, 1, TILER_PIXEL_FMT_PAGE,
                       OMAP_ION_HEAP_TILER_MASK, &temp, &stride);
       }

       if (ret || ((int)temp == -ENOMEM)) {
               DOMX_ERROR("FAILED to allocate buffer of size=%d. ret=0x%x",len, ret);
               return OMX_ErrorInsufficientResources;
       }

	if (ret)
		return OMX_ErrorInsufficientResources;
	*handle = temp;
	return OMX_ErrorNone;
}
#endif

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
OMX_ERRORTYPE PROXY_EventHandler(OMX_HANDLETYPE hComponent,
    OMX_PTR pAppData, OMX_EVENTTYPE eEvent, OMX_U32 nData1, OMX_U32 nData2,
    OMX_PTR pEventData)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	PROXY_COMPONENT_PRIVATE *pCompPrv = NULL;
	OMX_COMPONENTTYPE *hComp = (OMX_COMPONENTTYPE *) hComponent;
	OMX_PTR pTmpData = NULL;

	pCompPrv = (PROXY_COMPONENT_PRIVATE *) hComp->pComponentPrivate;

	PROXY_require((hComp->pComponentPrivate != NULL),
	    OMX_ErrorBadParameter,
	    "This is fatal error, processing cant proceed - please debug");

	DOMX_ENTER
	    ("hComponent=%p, pCompPrv=%p, eEvent=%p, nData1=%p, nData2=%p, pEventData=%p",
	    hComponent, pCompPrv, eEvent, nData1, nData2, pEventData);

	switch (eEvent)
	{
#if 0	// This feature is currently not supported, so kept in if(0) to be supported in the future
	case OMX_TI_EventBufferRefCount:
		DOMX_DEBUG("Received Ref Count Event");
		/*nData1 will be pBufferHeader, nData2 will be present count. Need to find local
		   buffer header for nData1 which is remote buffer header */

		PROXY_assert((nData1 != 0), OMX_ErrorBadParameter,
		    "Received NULL buffer header from OMX component");

		/*find local buffer header equivalent */
		for (count = 0; count < pCompPrv->nTotalBuffers; ++count)
		{
			if (pCompPrv->tBufList[count].pBufHeaderRemote ==
			    nData1)
			{
				pLocalBufHdr =
				    pCompPrv->tBufList[count].pBufHeader;
				pLocalBufHdr->pBuffer =
				    (OMX_U8 *) pCompPrv->tBufList[count].
				    pBufferActual;
				break;
			}
		}
		PROXY_assert((count != pCompPrv->nTotalBuffers),
		    OMX_ErrorBadParameter,
		    "Received invalid-buffer header from OMX component");

		/*update local buffer header */
		nData1 = (OMX_U32) pLocalBufHdr;
		break;
#endif
	case OMX_EventMark:
		DOMX_DEBUG("Received Mark Event");
		PROXY_assert((pEventData != NULL), OMX_ErrorUndefined,
		    "MarkData corrupted");
		pTmpData = pEventData;
		pEventData =
		    ((PROXY_MARK_DATA *) pEventData)->pMarkDataActual;
		TIMM_OSAL_Free(pTmpData);
		break;

	default:
		break;
	}

      EXIT:
	if (eError == OMX_ErrorNone)
	{
		pCompPrv->tCBFunc.EventHandler(hComponent,
		    pCompPrv->pILAppData, eEvent, nData1, nData2, pEventData);
	} else if (pCompPrv)
	{
		pCompPrv->tCBFunc.EventHandler(hComponent,
		    pCompPrv->pILAppData, OMX_EventError, eError, 0, NULL);
	}

	DOMX_EXIT("eError: %d", eError);
	return OMX_ErrorNone;
}

/* ===========================================================================*/
/**
 * @name PROXY_EmptyBufferDone()
 * @brief
 * @param
 * @return OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
static OMX_ERRORTYPE PROXY_EmptyBufferDone(OMX_HANDLETYPE hComponent,
    OMX_U32 remoteBufHdr, OMX_U32 nfilledLen, OMX_U32 nOffset, OMX_U32 nFlags)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	PROXY_COMPONENT_PRIVATE *pCompPrv = NULL;
	OMX_COMPONENTTYPE *hComp = (OMX_COMPONENTTYPE *) hComponent;
	OMX_U16 count;
	OMX_BUFFERHEADERTYPE *pBufHdr = NULL;

	PROXY_require((hComp->pComponentPrivate != NULL),
	    OMX_ErrorBadParameter,
	    "This is fatal error, processing cant proceed - please debug");

	pCompPrv = (PROXY_COMPONENT_PRIVATE *) hComp->pComponentPrivate;

	DOMX_ENTER
	    ("hComponent=%p, pCompPrv=%p, remoteBufHdr=%p, nFilledLen=%d, nOffset=%d, nFlags=%08x",
	    hComponent, pCompPrv, remoteBufHdr, nfilledLen, nOffset, nFlags);

	for (count = 0; count < pCompPrv->nTotalBuffers; ++count)
	{
		if (pCompPrv->tBufList[count].pBufHeaderRemote ==
		    remoteBufHdr)
		{
			pBufHdr = pCompPrv->tBufList[count].pBufHeader;
			pBufHdr->nFilledLen = nfilledLen;
			pBufHdr->nOffset = nOffset;
			pBufHdr->nFlags = nFlags;
			/* Setting mark info to NULL. This would always be
			   NULL in EBD, whether component has propagated the
			   mark or has generated mark event */
			pBufHdr->hMarkTargetComponent = NULL;
			pBufHdr->pMarkData = NULL;
			break;
		}
	}
	PROXY_assert((count != pCompPrv->nTotalBuffers),
	    OMX_ErrorBadParameter,
	    "Received invalid-buffer header from OMX component");

	KPI_OmxCompBufferEvent(KPI_BUFFER_EBD, hComponent, &(pCompPrv->tBufList[count]));

      EXIT:
	if (eError == OMX_ErrorNone)
	{
		pCompPrv->tCBFunc.EmptyBufferDone(hComponent,
		    pCompPrv->pILAppData, pBufHdr);
	} else if (pCompPrv)
	{
		pCompPrv->tCBFunc.EventHandler(hComponent,
		    pCompPrv->pILAppData, OMX_EventError, eError, 0, NULL);
	}

	DOMX_EXIT("eError: %d", eError);
	return OMX_ErrorNone;
}

/* ===========================================================================*/
/**
 * @name PROXY_FillBufferDone()
 * @brief
 * @param
 * @return OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
OMX_ERRORTYPE PROXY_FillBufferDone(OMX_HANDLETYPE hComponent,
    OMX_U32 remoteBufHdr, OMX_U32 nfilledLen, OMX_U32 nOffset, OMX_U32 nFlags,
    OMX_TICKS nTimeStamp, OMX_HANDLETYPE hMarkTargetComponent,
    OMX_PTR pMarkData)
{

	OMX_ERRORTYPE eError = OMX_ErrorNone;
	PROXY_COMPONENT_PRIVATE *pCompPrv = NULL;
	OMX_COMPONENTTYPE *hComp = (OMX_COMPONENTTYPE *) hComponent;
	OMX_U16 count;
	OMX_BUFFERHEADERTYPE *pBufHdr = NULL;

	PROXY_require((hComp->pComponentPrivate != NULL),
	    OMX_ErrorBadParameter,
	    "This is fatal error, processing cant proceed - please debug");

	pCompPrv = (PROXY_COMPONENT_PRIVATE *) hComp->pComponentPrivate;

	DOMX_ENTER
	    ("hComponent=%p, pCompPrv=%p, remoteBufHdr=%p, nFilledLen=%d, nOffset=%d, nFlags=%08x",
	    hComponent, pCompPrv, remoteBufHdr, nfilledLen, nOffset, nFlags);

	for (count = 0; count < pCompPrv->nTotalBuffers; ++count)
	{
		if (pCompPrv->tBufList[count].pBufHeaderRemote ==
		    remoteBufHdr)
		{
			pBufHdr = pCompPrv->tBufList[count].pBufHeader;
			pBufHdr->nFilledLen = nfilledLen;
			pBufHdr->nOffset = nOffset;
			pBufHdr->nFlags = nFlags;
			pBufHdr->nTimeStamp = nTimeStamp;
			if (pMarkData != NULL)
			{
				/*Update mark info in the buffer header */
				pBufHdr->pMarkData =
				    ((PROXY_MARK_DATA *)
				    pMarkData)->pMarkDataActual;
				pBufHdr->hMarkTargetComponent =
				    ((PROXY_MARK_DATA *)
				    pMarkData)->hComponentActual;
				TIMM_OSAL_Free(pMarkData);
			}
			break;
		}
	}
	PROXY_assert((count != pCompPrv->nTotalBuffers),
	    OMX_ErrorBadParameter,
	    "Received invalid-buffer header from OMX component");

	KPI_OmxCompBufferEvent(KPI_BUFFER_FBD, hComponent, &(pCompPrv->tBufList[count]));

      EXIT:
	if (eError == OMX_ErrorNone)
	{
		pCompPrv->tCBFunc.FillBufferDone(hComponent,
		    pCompPrv->pILAppData, pBufHdr);
	} else if (pCompPrv)
	{
		pCompPrv->tCBFunc.EventHandler(hComponent,
		    pCompPrv->pILAppData, OMX_EventError, eError, 0, NULL);
	}

	DOMX_EXIT("eError: %d", eError);
	return OMX_ErrorNone;
}

/* ===========================================================================*/
/**
 * @name PROXY_EmptyThisBuffer()
 * @brief
 * @param void
 * @return OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
OMX_ERRORTYPE PROXY_EmptyThisBuffer(OMX_HANDLETYPE hComponent,
    OMX_BUFFERHEADERTYPE * pBufferHdr)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone, eCompReturn = OMX_ErrorNone;
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	PROXY_COMPONENT_PRIVATE *pCompPrv;
	OMX_COMPONENTTYPE *hComp = (OMX_COMPONENTTYPE *) hComponent;
	OMX_U32 count = 0;
	OMX_COMPONENTTYPE *pMarkComp = NULL;
	PROXY_COMPONENT_PRIVATE *pMarkCompPrv = NULL;
	OMX_PTR pMarkData = NULL;
	OMX_BOOL bFreeMarkIfError = OMX_FALSE;
	OMX_BOOL bIsProxy = OMX_FALSE , bMapBuffer;

	PROXY_require(pBufferHdr != NULL, OMX_ErrorBadParameter, NULL);
	PROXY_require(hComp->pComponentPrivate != NULL, OMX_ErrorBadParameter,
	    NULL);
	PROXY_CHK_VERSION(pBufferHdr, OMX_BUFFERHEADERTYPE);

	pCompPrv = (PROXY_COMPONENT_PRIVATE *) hComp->pComponentPrivate;

	DOMX_ENTER
	    ("hComponent=%p, pCompPrv=%p, nFilledLen=%d, nOffset=%d, nFlags=%08x",
	    hComponent, pCompPrv, pBufferHdr->nFilledLen,
	    pBufferHdr->nOffset, pBufferHdr->nFlags);

	/*First find the index of this buffer header to retrieve remote buffer header */
	for (count = 0; count < pCompPrv->nTotalBuffers; count++)
	{
		if (pCompPrv->tBufList[count].pBufHeader == pBufferHdr)
		{
			DOMX_DEBUG("Buffer Index of Match %d ", count);
			break;
		}
	}
	PROXY_assert((count != pCompPrv->nTotalBuffers),
	    OMX_ErrorBadParameter,
	    "Could not find the remote header in buffer list");

	if (pBufferHdr->hMarkTargetComponent != NULL)
	{
		pMarkComp =
		    (OMX_COMPONENTTYPE *) (pBufferHdr->hMarkTargetComponent);
		/* Check is mark comp is proxy */
		eError = _RPC_IsProxyComponent(pMarkComp, &bIsProxy);
		PROXY_assert(eError == OMX_ErrorNone, eError, "");

		/*Replacing original mark data with proxy specific structure */
		pMarkData = pBufferHdr->pMarkData;
		pBufferHdr->pMarkData =
		    TIMM_OSAL_Malloc(sizeof(PROXY_MARK_DATA), TIMM_OSAL_TRUE,
		    0, TIMMOSAL_MEM_SEGMENT_INT);
		PROXY_assert(pBufferHdr->pMarkData != NULL,
		    OMX_ErrorInsufficientResources, "Malloc failed");
		bFreeMarkIfError = OMX_TRUE;
		((PROXY_MARK_DATA *) (pBufferHdr->
			pMarkData))->hComponentActual = pMarkComp;
		((PROXY_MARK_DATA *) (pBufferHdr->
			pMarkData))->pMarkDataActual = pMarkData;

		/* If proxy comp then replace hMarkTargetComponent with remote
		   handle */
		if (bIsProxy)
		{
			pMarkCompPrv = pMarkComp->pComponentPrivate;
			PROXY_assert(pMarkCompPrv != NULL,
			    OMX_ErrorBadParameter, NULL);

			/* Replacing with remote component handle */
			pBufferHdr->hMarkTargetComponent =
			    ((RPC_OMX_CONTEXT *) pMarkCompPrv->hRemoteComp)->
			    hActualRemoteCompHandle;
		}
	}

	bMapBuffer =
		pCompPrv->proxyPortBuffers[pBufferHdr->nInputPortIndex].proxyBufferType ==
			EncoderMetadataPointers;

	KPI_OmxCompBufferEvent(KPI_BUFFER_ETB, hComponent, &(pCompPrv->tBufList[count]));

	eRPCError =
	    RPC_EmptyThisBuffer(pCompPrv->hRemoteComp, pBufferHdr,
	    pCompPrv->tBufList[count].pBufHeaderRemote, &eCompReturn,bMapBuffer);

	PROXY_checkRpcError();

      EXIT:
	/*If ETB is about to return an error then this means that buffer has not
	   been accepted by the component. Thus the allocated mark data will be
	   lost so free it here. Also replace original mark data in the header */
	if ((eError != OMX_ErrorNone) && bFreeMarkIfError)
	{
		pBufferHdr->hMarkTargetComponent =
		    ((PROXY_MARK_DATA *) (pBufferHdr->
			pMarkData))->hComponentActual;
		pMarkData =
		    ((PROXY_MARK_DATA *) (pBufferHdr->
			pMarkData))->pMarkDataActual;
		TIMM_OSAL_Free(pBufferHdr->pMarkData);
		pBufferHdr->pMarkData = pMarkData;
	}

	DOMX_EXIT("eError: %d", eError);
	return eError;
}


/* ===========================================================================*/
/**
 * @name PROXY_FillThisBuffer()
 * @brief
 * @param void
 * @return OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
OMX_ERRORTYPE PROXY_FillThisBuffer(OMX_HANDLETYPE hComponent,
    OMX_BUFFERHEADERTYPE * pBufferHdr)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone, eCompReturn = OMX_ErrorNone;
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	PROXY_COMPONENT_PRIVATE *pCompPrv;
	OMX_COMPONENTTYPE *hComp = (OMX_COMPONENTTYPE *) hComponent;
	OMX_U32 count = 0;

	PROXY_require(pBufferHdr != NULL, OMX_ErrorBadParameter, NULL);
	PROXY_require(hComp->pComponentPrivate != NULL, OMX_ErrorBadParameter,
	    NULL);
	PROXY_CHK_VERSION(pBufferHdr, OMX_BUFFERHEADERTYPE);

	pCompPrv = (PROXY_COMPONENT_PRIVATE *) hComp->pComponentPrivate;

	DOMX_ENTER
	    ("hComponent = %p, pCompPrv = %p, nFilledLen = %d, nOffset = %d, nFlags = %08x",
	    hComponent, pCompPrv, pBufferHdr->nFilledLen,
	    pBufferHdr->nOffset, pBufferHdr->nFlags);

	/*First find the index of this buffer header to retrieve remote buffer header */
	for (count = 0; count < pCompPrv->nTotalBuffers; count++)
	{
		if (pCompPrv->tBufList[count].pBufHeader == pBufferHdr)
		{
			DOMX_DEBUG("Buffer Index of Match %d ", count);
			break;
		}
	}
	PROXY_assert((count != pCompPrv->nTotalBuffers),
	    OMX_ErrorBadParameter,
	    "Could not find the remote header in buffer list");

	KPI_OmxCompBufferEvent(KPI_BUFFER_FTB, hComponent, &(pCompPrv->tBufList[count]));

	eRPCError = RPC_FillThisBuffer(pCompPrv->hRemoteComp, pBufferHdr,
	    pCompPrv->tBufList[count].pBufHeaderRemote, &eCompReturn);

	PROXY_checkRpcError();

      EXIT:
	DOMX_EXIT("eError: %d", eError);
	return eError;
}


/* ===========================================================================*/
/**
 * @name PROXY_AllocateBuffer()
 * @brief
 * @param void
 * @return OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
OMX_ERRORTYPE PROXY_AllocateBuffer(OMX_IN OMX_HANDLETYPE hComponent,
    OMX_INOUT OMX_BUFFERHEADERTYPE ** ppBufferHdr,
    OMX_IN OMX_U32 nPortIndex,
    OMX_IN OMX_PTR pAppPrivate, OMX_IN OMX_U32 nSizeBytes)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	PROXY_COMPONENT_PRIVATE *pCompPrv = NULL;
	OMX_COMPONENTTYPE *hComp = (OMX_COMPONENTTYPE *) hComponent;
	OMX_U32 currentBuffer = 0, i = 0;
	OMX_BOOL bSlotFound = OMX_FALSE;
#ifdef USE_ION
	struct ion_handle *handle = NULL;
	OMX_PTR pIonMappedBuffer = NULL;
#endif

#ifdef ALLOCATE_TILER_BUFFER_IN_PROXY
	// Do the tiler allocations in Proxy and call use buffers on Ducati.

	//Round Off the size to allocate and map to next page boundary.
	OMX_U32 nSize = (nSizeBytes + LINUX_PAGE_SIZE - 1) & ~(LINUX_PAGE_SIZE - 1);
	OMX_U32 nStride = 0;
	OMX_U8* pMemptr = NULL;
	OMX_CONFIG_RECTTYPE tParamRect;
	OMX_PARAM_PORTDEFINITIONTYPE tParamPortDef;

	PROXY_require((hComp->pComponentPrivate != NULL),
	    OMX_ErrorBadParameter, NULL);
	PROXY_require(ppBufferHdr != NULL, OMX_ErrorBadParameter,
	    "Pointer to buffer header is NULL");

	pCompPrv = (PROXY_COMPONENT_PRIVATE *) hComp->pComponentPrivate;

	DOMX_ENTER
	    ("hComponent = %p, pCompPrv = %p, nPortIndex = %p, pAppPrivate = %p, nSizeBytes = %d",
	    hComponent, pCompPrv, nPortIndex, pAppPrivate, nSizeBytes);

	/*Pick up 1st empty slot */
	/*The same empty spot will be picked up by the subsequent
	Use buffer call to fill in the corresponding buffer
	Buffer header in the list */

	bSlotFound = OMX_FALSE;
	for (i = 0; i < pCompPrv->nTotalBuffers; i++)
	{
		if (pCompPrv->tBufList[i].pBufHeader == NULL)
		{
			currentBuffer = i;
			bSlotFound = OMX_TRUE;
			break;
		}
	}

	if (bSlotFound == OMX_FALSE)
	{
		currentBuffer = pCompPrv->nTotalBuffers;
	}

	/*To find whether buffer is 2D or 1D */
	eError =
	    RPC_UTIL_GetStride(pCompPrv->hRemoteComp, nPortIndex, &nStride);
	PROXY_assert(eError == OMX_ErrorNone, eError,
	    "Failed to get stride of component");

	if (nStride == LINUX_PAGE_SIZE && \
			pCompPrv->proxyPortBuffers[nPortIndex].proxyBufferType != EncoderMetadataPointers) //Allocate 2D buffer
	{
#if USE_ION
		DOMX_ERROR ("Tiler 2d port buffers not implemented");
		eError = OMX_ErrorNotImplemented;
		goto EXIT;
#endif
	}
#ifdef USE_ION
	else if (pCompPrv->bUseIon == OMX_TRUE)
	{
		eError = PROXY_AllocateBufferIonCarveout(pCompPrv, nSize, &handle);
		pCompPrv->tBufList[currentBuffer].pYBuffer = handle;
		if (pCompPrv->bMapIonBuffers == OMX_TRUE)
		{
			DOMX_DEBUG("before mapping, handle = %x, nSize = %d",handle,nSize);
			if (ion_map(pCompPrv->ion_fd, handle, nSize, PROT_READ | PROT_WRITE, MAP_SHARED, 0,
		                &pIonMappedBuffer, &(pCompPrv->tBufList[currentBuffer].mmap_fd)) < 0)
			{
				DOMX_ERROR("userspace mapping of ION buffers returned error");
				eError = OMX_ErrorInsufficientResources;
				goto EXIT;
			}
		} else {
			if (ion_share(pCompPrv->ion_fd, handle, &(pCompPrv->tBufList[currentBuffer].mmap_fd)) < 0) {
				DOMX_ERROR("ion_share failed !!! \n");
				goto EXIT;
			} else {
				DOMX_DEBUG("ion_share success pMemptr: 0x%x \n", pCompPrv->tBufList[currentBuffer].mmap_fd);
			}
		}
		pMemptr = pCompPrv->tBufList[currentBuffer].mmap_fd;
		DOMX_DEBUG ("Ion handle recieved = %x",handle);
		if (eError != OMX_ErrorNone)
			goto EXIT;
	}
#endif

	/*No need to increment Allocated buffers here.
	It will be done in the subsequent use buffer call below*/
	eError = PROXY_UseBuffer(hComponent, ppBufferHdr, nPortIndex, pAppPrivate, nSize, pMemptr);

	if(eError != OMX_ErrorNone) {
		DOMX_ERROR("PROXY_UseBuffer in PROXY_AllocateBuffer failed with error %d (0x%08x)", eError, eError);
#ifdef USE_ION
		ion_free(pCompPrv->ion_fd,
		         (struct ion_handle *)pCompPrv->tBufList[currentBuffer].pYBuffer);
		close(pCompPrv->tBufList[currentBuffer].mmap_fd);
#endif
		goto EXIT;
	}
	else {
#ifndef USE_ION
		pCompPrv->tBufList[currentBuffer].pYBuffer = pMemptr;
#endif
	}

#ifdef USE_ION
	if (pCompPrv->bUseIon == OMX_TRUE && pCompPrv->bMapIonBuffers == OMX_TRUE)
	{
		DOMX_DEBUG("before mapping, handle = %x, nSize = %d",handle,nSize);
		(*ppBufferHdr)->pBuffer = pIonMappedBuffer;
	} else {
		(*ppBufferHdr)->pBuffer = (OMX_U8 *)handle;
	}
#endif

#else
	//This code is the un-changed version of original implementation.
	OMX_BUFFERHEADERTYPE *pBufferHeader = NULL;
	OMX_U32 pBufHeaderRemote = 0;
	OMX_U32 currentBuffer = 0, i = 0;
	OMX_U8 *pBuffer = NULL;
	OMX_TI_PLATFORMPRIVATE *pPlatformPrivate = NULL;
	OMX_BOOL bSlotFound = OMX_FALSE;
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	OMX_ERRORTYPE eCompReturn = OMX_ErrorNone;

	PROXY_require((hComp->pComponentPrivate != NULL),
	    OMX_ErrorBadParameter, NULL);
	PROXY_require(ppBufferHdr != NULL, OMX_ErrorBadParameter,
	    "Pointer to buffer header is NULL");

	pCompPrv = (PROXY_COMPONENT_PRIVATE *) hComp->pComponentPrivate;

	DOMX_ENTER
	    ("hComponent = %p, pCompPrv = %p, nPortIndex = %p, pAppPrivate = %p, nSizeBytes = %d",
	    hComponent, pCompPrv, nPortIndex, pAppPrivate, nSizeBytes);

	/*Pick up 1st empty slot */
	for (i = 0; i < pCompPrv->nTotalBuffers; i++)
	{
		if (pCompPrv->tBufList[i].pBufHeader == 0)
		{
			currentBuffer = i;
			bSlotFound = OMX_TRUE;
			break;
		}
	}
	if (!bSlotFound)
	{
		currentBuffer = pCompPrv->nTotalBuffers;
	}

	DOMX_DEBUG("In AB, no. of buffers = %d", pCompPrv->nTotalBuffers);
	PROXY_assert((pCompPrv->nTotalBuffers < MAX_NUM_PROXY_BUFFERS),
	    OMX_ErrorInsufficientResources,
	    "Proxy cannot handle more than MAX buffers");

	//Allocating Local bufferheader to be maintained locally within proxy
	pBufferHeader =
	    (OMX_BUFFERHEADERTYPE *)
	    TIMM_OSAL_Malloc(sizeof(OMX_BUFFERHEADERTYPE), TIMM_OSAL_TRUE, 0,
	    TIMMOSAL_MEM_SEGMENT_INT);
	PROXY_assert((pBufferHeader != NULL), OMX_ErrorInsufficientResources,
	    "Allocation of Buffer Header structure failed");

	pPlatformPrivate =
	    (OMX_TI_PLATFORMPRIVATE *)
	    TIMM_OSAL_Malloc(sizeof(OMX_TI_PLATFORMPRIVATE), TIMM_OSAL_TRUE,
	    0, TIMMOSAL_MEM_SEGMENT_INT);
	PROXY_assert(pPlatformPrivate != NULL, OMX_ErrorInsufficientResources,
	    "Allocation of platform private structure failed");
	pBufferHeader->pPlatformPrivate = pPlatformPrivate;

	DOMX_DEBUG(" Calling RPC ");

	eRPCError =
	    RPC_AllocateBuffer(pCompPrv->hRemoteComp, &pBufferHeader,
	    nPortIndex, &pBufHeaderRemote, pAppPrivate, nSizeBytes,
	    &eCompReturn);

	PROXY_checkRpcError();

	DOMX_DEBUG("Allocate Buffer Successful");
	DOMX_DEBUG("Value of pBufHeaderRemote: %p   LocalBufferHdr :%p",
	    pBufHeaderRemote, pBufferHeader);

	pCompPrv->tBufList[currentBuffer].pBufHeader = pBufferHeader;
	pCompPrv->tBufList[currentBuffer].pBufHeaderRemote = pBufHeaderRemote;


	//keeping track of number of Buffers
	pCompPrv->nAllocatedBuffers++;

	if (pCompPrv->nTotalBuffers < pCompPrv->nAllocatedBuffers)
	{
		pCompPrv->nTotalBuffers = pCompPrv->nAllocatedBuffers;
	}

	*ppBufferHdr = pBufferHeader;

      EXIT:
	if (eError != OMX_ErrorNone)
	{
		if (pPlatformPrivate)
			TIMM_OSAL_Free(pPlatformPrivate);
		if (pBufferHeader)
			TIMM_OSAL_Free(pBufferHeader);
	}
	DOMX_EXIT("eError: %d", eError);
	return eError;
#endif //ALLOCATE_TILER_BUFFER_IN_PROXY

      EXIT:
	DOMX_EXIT("eError: %d", eError);
	return eError;
}


/* ===========================================================================*/
/**
 * @name PROXY_UseBuffer()
 * @brief
 * @param void
 * @return OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
static OMX_ERRORTYPE PROXY_UseBuffer(OMX_IN OMX_HANDLETYPE hComponent,
    OMX_INOUT OMX_BUFFERHEADERTYPE ** ppBufferHdr,
    OMX_IN OMX_U32 nPortIndex, OMX_IN OMX_PTR pAppPrivate,
    OMX_IN OMX_U32 nSizeBytes, OMX_IN OMX_U8 * pBuffer)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone, eCompReturn = OMX_ErrorNone;
	OMX_BUFFERHEADERTYPE *pBufferHeader = NULL;
	OMX_U32 pBufHeaderRemote = 0;
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	OMX_U32 currentBuffer = 0, i = 0, nStride = 0, nNumLines = 0;
	PROXY_COMPONENT_PRIVATE *pCompPrv = NULL;
	OMX_TI_PLATFORMPRIVATE *pPlatformPrivate = NULL;
	OMX_COMPONENTTYPE *hComp = (OMX_COMPONENTTYPE *) hComponent;
	OMX_BOOL bSlotFound = OMX_FALSE;
	OMX_PTR pAuxBuf0 = pBuffer;
	OMX_PTR pMappedMetaDataBuffer = NULL;
	OMX_TI_PARAM_METADATABUFFERINFO tMetaDataBuffer;
	OMX_U32 nBufferHeight = 0;
	OMX_CONFIG_RECTTYPE tParamRect;
	OMX_PARAM_PORTDEFINITIONTYPE tParamPortDef;

#ifdef USE_ION
	OMX_PTR pMetadataBuffer = NULL;
#endif

	PROXY_require((hComp->pComponentPrivate != NULL),
	    OMX_ErrorBadParameter, NULL);
	PROXY_require(pBuffer != NULL, OMX_ErrorBadParameter, "Pointer to buffer is NULL");
	PROXY_require(ppBufferHdr != NULL, OMX_ErrorBadParameter,
	    "Pointer to buffer header is NULL");

	pCompPrv = (PROXY_COMPONENT_PRIVATE *) hComp->pComponentPrivate;

	DOMX_ENTER
	    ("hComponent = %p, pCompPrv = %p, nPortIndex = %p, pAppPrivate = %p, nSizeBytes = %d, pBuffer = %p",
	    hComponent, pCompPrv, nPortIndex, pAppPrivate, nSizeBytes,
	    pBuffer);

	/*Pick up 1st empty slot */
	for (i = 0; i < pCompPrv->nTotalBuffers; i++)
	{
		if (pCompPrv->tBufList[i].pBufHeader == 0)
		{
			currentBuffer = i;
			bSlotFound = OMX_TRUE;
			break;
		}
	}
	if (!bSlotFound)
	{
		currentBuffer = pCompPrv->nTotalBuffers;
	}
	DOMX_DEBUG("In UB, no. of buffers = %d", pCompPrv->nTotalBuffers);

	PROXY_assert((pCompPrv->nTotalBuffers < MAX_NUM_PROXY_BUFFERS),
	    OMX_ErrorInsufficientResources,
	    "Proxy cannot handle more than MAX buffers");

	//Allocating Local bufferheader to be maintained locally within proxy
	pBufferHeader =
	    (OMX_BUFFERHEADERTYPE *)
	    TIMM_OSAL_Malloc(sizeof(OMX_BUFFERHEADERTYPE), TIMM_OSAL_TRUE, 0,
	    TIMMOSAL_MEM_SEGMENT_INT);
	PROXY_assert((pBufferHeader != NULL), OMX_ErrorInsufficientResources,
	    "Allocation of Buffer Header structure failed");

	pPlatformPrivate =
	    (OMX_TI_PLATFORMPRIVATE *)
	    TIMM_OSAL_Malloc(sizeof(OMX_TI_PLATFORMPRIVATE), TIMM_OSAL_TRUE,
	    0, TIMMOSAL_MEM_SEGMENT_INT);
	PROXY_assert(pPlatformPrivate != NULL, OMX_ErrorInsufficientResources,
	    "Allocation of platform private structure failed");
	TIMM_OSAL_Memset(pPlatformPrivate, 0, sizeof(OMX_TI_PLATFORMPRIVATE));

	pBufferHeader->pPlatformPrivate = pPlatformPrivate;
	((OMX_TI_PLATFORMPRIVATE *)pBufferHeader->pPlatformPrivate)->nSize = sizeof(OMX_TI_PLATFORMPRIVATE);

#ifdef ENABLE_GRALLOC_BUFFERS

	if(pCompPrv->proxyPortBuffers[nPortIndex].proxyBufferType == GrallocPointers)
	{
		//Extracting buffer pointer from the gralloc buffer
		pAuxBuf0 = (OMX_U8 *)(((IMG_native_handle_t*)pBuffer)->fd[0]);
	}
#endif

	DOMX_DEBUG("Preparing buffer to Remote Core...");
	pBufferHeader->pBuffer = pBuffer;
	/*To find whether buffer is 2D or 1D */
	eError =
	    RPC_UTIL_GetStride(pCompPrv->hRemoteComp, nPortIndex, &nStride);
	PROXY_assert(eError == OMX_ErrorNone, eError,
	    "Failed to get stride of component");
	if (nStride == LINUX_PAGE_SIZE)
	{
		// Change this to extract UV pointer from gralloc handle once new gralloc interface is available
		/*2D buffer, assume NV12 format */
		eError =
		    RPC_UTIL_GetNumLines(pCompPrv->hRemoteComp, nPortIndex,
		    &nNumLines);
		PROXY_assert(eError == OMX_ErrorNone, eError,
		    "Failed to get num of lines");

		if(pCompPrv->proxyPortBuffers[nPortIndex].proxyBufferType == GrallocPointers)
		{
			((OMX_TI_PLATFORMPRIVATE *) pBufferHeader->pPlatformPrivate)->
				pAuxBuf1 = (OMX_U8 *)(((IMG_native_handle_t*)pBuffer)->fd[1]);
		}

		if(pCompPrv->proxyPortBuffers[nPortIndex].proxyBufferType == EncoderMetadataPointers)
		{
			((OMX_TI_PLATFORMPRIVATE *) pBufferHeader->pPlatformPrivate)->
				pAuxBuf1 = NULL;
		}
		if(pCompPrv->proxyPortBuffers[nPortIndex].proxyBufferType == BufferDescriptorVirtual2D)
		{
			pAuxBuf0 = (OMX_U8 *)(((OMX_TI_BUFFERDESCRIPTOR_TYPE*)pBuffer)->pBuf[0]);

			((OMX_TI_PLATFORMPRIVATE *) pBufferHeader->pPlatformPrivate)->
				pAuxBuf1 = (OMX_U8 *)(((OMX_TI_BUFFERDESCRIPTOR_TYPE*)pBuffer)->pBuf[1]);
		}
	}

	/*Initializing Structure */
	tMetaDataBuffer.nSize = sizeof(OMX_TI_PARAM_METADATABUFFERINFO);
	tMetaDataBuffer.nVersion.s.nVersionMajor = OMX_VER_MAJOR;
	tMetaDataBuffer.nVersion.s.nVersionMinor = OMX_VER_MINOR;
	tMetaDataBuffer.nVersion.s.nRevision = 0x0;
	tMetaDataBuffer.nVersion.s.nStep = 0x0;
	tMetaDataBuffer.nPortIndex = nPortIndex;
	eError = PROXY_GetParameter(hComponent, (OMX_INDEXTYPE)OMX_TI_IndexParamMetaDataBufferInfo, (OMX_PTR)&tMetaDataBuffer);
	PROXY_assert(eError == OMX_ErrorNone, eError,
	    "Get Parameter for Metadata infor failed");

	DOMX_DEBUG("Metadata size = %d",tMetaDataBuffer.nMetaDataSize);

	if(tMetaDataBuffer.bIsMetaDataEnabledOnPort)
	{
#ifdef USE_ION
		((OMX_TI_PLATFORMPRIVATE *)pBufferHeader->pPlatformPrivate)->nMetaDataSize =
			(tMetaDataBuffer.nMetaDataSize + LINUX_PAGE_SIZE - 1) & ~(LINUX_PAGE_SIZE -1);
		eError = PROXY_AllocateBufferIonCarveout(pCompPrv, ((OMX_TI_PLATFORMPRIVATE *)pBufferHeader->pPlatformPrivate)->nMetaDataSize,
			(struct ion_handle **)(&(((OMX_TI_PLATFORMPRIVATE *)pBufferHeader->pPlatformPrivate)->pMetaDataBuffer)));
		pCompPrv->tBufList[currentBuffer].pMetaDataBuffer = ((OMX_TI_PLATFORMPRIVATE *)pBufferHeader->
			pPlatformPrivate)->pMetaDataBuffer;
		DOMX_DEBUG("Metadata buffer ion handle = %d",((OMX_TI_PLATFORMPRIVATE *)pBufferHeader->pPlatformPrivate)->pMetaDataBuffer);
		if (pCompPrv->bMapIonBuffers == OMX_TRUE) {
			if (ion_map(pCompPrv->ion_fd, pPlatformPrivate->pMetaDataBuffer,
				    pPlatformPrivate->nMetaDataSize,
				    PROT_READ | PROT_WRITE, MAP_SHARED, 0,
				    &pMetadataBuffer,
				    &(pCompPrv->tBufList[currentBuffer].mmap_fd_metadata_buff)) < 0)
			{
				DOMX_ERROR("userspace mapping of ION metadata buffers returned error");
				eError = OMX_ErrorInsufficientResources;
				goto EXIT;
			}
		}
#endif
	}

#ifdef USE_ION
	{
		// Need to register buffers when using ion and rpmsg
		eRPCError = RPC_RegisterBuffer(pCompPrv->hRemoteComp, pAuxBuf0,
					   &pCompPrv->tBufList[currentBuffer].pRegisteredAufBux0,
					   &pCompPrv->tBufList[currentBuffer].pRegisteredAufBux1,
					   pCompPrv->proxyPortBuffers[nPortIndex].proxyBufferType);
		PROXY_checkRpcError();
		if (pCompPrv->tBufList[currentBuffer].pRegisteredAufBux0)
			pAuxBuf0 = pCompPrv->tBufList[currentBuffer].pRegisteredAufBux0;
		if (pCompPrv->tBufList[currentBuffer].pRegisteredAufBux1)
			pPlatformPrivate->pAuxBuf1 = pCompPrv->tBufList[currentBuffer].pRegisteredAufBux1;

		if (pPlatformPrivate->pMetaDataBuffer != NULL)
		{
			int fd = -1;
			if (pCompPrv->bMapIonBuffers == OMX_TRUE) {
				fd = pCompPrv->tBufList[currentBuffer].mmap_fd_metadata_buff;
			} else {
				ion_share (pCompPrv->ion_fd, pPlatformPrivate->pMetaDataBuffer, &fd);
				pCompPrv->tBufList[currentBuffer].mmap_fd_metadata_buff = fd;
			}
			eRPCError = RPC_RegisterBuffer(pCompPrv->hRemoteComp, fd,
					   &pCompPrv->tBufList[currentBuffer].pRegisteredAufBux2, NULL, IONPointers);
			PROXY_checkRpcError();
			if (pCompPrv->tBufList[currentBuffer].pRegisteredAufBux2)
				pPlatformPrivate->pMetaDataBuffer = pCompPrv->tBufList[currentBuffer].pRegisteredAufBux2;
		}
	}
#endif

	eRPCError = RPC_UseBuffer(pCompPrv->hRemoteComp, &pBufferHeader, nPortIndex,
		pAppPrivate, nSizeBytes, pAuxBuf0, &pBufHeaderRemote, &eCompReturn);

	PROXY_checkRpcError();

	DOMX_DEBUG("Use Buffer Successful");
	DOMX_DEBUG
	    ("Value of pBufHeaderRemote: %p LocalBufferHdr :%p, LocalBuffer :%p",
	    pBufHeaderRemote, pBufferHeader, pBufferHeader->pBuffer);

#ifdef USE_ION
	if (pCompPrv->bUseIon == OMX_TRUE && pCompPrv->bMapIonBuffers == OMX_TRUE && tMetaDataBuffer.bIsMetaDataEnabledOnPort)
	{
		((OMX_TI_PLATFORMPRIVATE *)pBufferHeader->pPlatformPrivate)->pMetaDataBuffer = pMetadataBuffer;
		//ion_free(pCompPrv->ion_fd, handleToMap);
		memset(((OMX_TI_PLATFORMPRIVATE *)pBufferHeader->pPlatformPrivate)->pMetaDataBuffer,
			0x0, tMetaDataBuffer.nMetaDataSize);
	}
#endif

	//Storing details of pBufferHeader/Mapped/Actual buffer address locally.
	pCompPrv->tBufList[currentBuffer].pBufHeader = pBufferHeader;
	pCompPrv->tBufList[currentBuffer].pBufHeaderRemote = pBufHeaderRemote;

	//keeping track of number of Buffers
	pCompPrv->nAllocatedBuffers++;
	if (pCompPrv->nTotalBuffers < pCompPrv->nAllocatedBuffers)
		pCompPrv->nTotalBuffers = pCompPrv->nAllocatedBuffers;

	DOMX_DEBUG("Updating no. of buffer to %d", pCompPrv->nTotalBuffers);

	*ppBufferHdr = pBufferHeader;

      EXIT:
	if (eError != OMX_ErrorNone)
	{
		if (pPlatformPrivate)
			TIMM_OSAL_Free(pPlatformPrivate);
		if (pBufferHeader)
			TIMM_OSAL_Free(pBufferHeader);
	}
	DOMX_EXIT("eError: %d", eError);
	return eError;
}

/* ===========================================================================*/
/**
 * @name PROXY_FreeBuffer()
 * @brief
 * @param void
 * @return OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
OMX_ERRORTYPE PROXY_FreeBuffer(OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_U32 nPortIndex, OMX_IN OMX_BUFFERHEADERTYPE * pBufferHdr)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone, eCompReturn = OMX_ErrorNone;
	OMX_COMPONENTTYPE *hComp = (OMX_COMPONENTTYPE *) hComponent;
	PROXY_COMPONENT_PRIVATE *pCompPrv = NULL;
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone, eTmpRPCError =
	    RPC_OMX_ErrorNone;
	OMX_U32 count = 0, nStride = 0;
	OMX_U32 pBuffer = 0;
	OMX_PTR pMetaDataBuffer = NULL;
	OMX_PTR pAuxBuf0 = NULL;
	OMX_TI_PLATFORMPRIVATE * pPlatformPrivate = NULL;

	PROXY_require(pBufferHdr != NULL, OMX_ErrorBadParameter, NULL);
	PROXY_require(hComp->pComponentPrivate != NULL, OMX_ErrorBadParameter,
	    NULL);
	PROXY_CHK_VERSION(pBufferHdr, OMX_BUFFERHEADERTYPE);

	pCompPrv = (PROXY_COMPONENT_PRIVATE *) hComp->pComponentPrivate;

	DOMX_ENTER
	    ("hComponent = %p, pCompPrv = %p, nPortIndex = %p, pBufferHdr = %p, pBuffer = %p",
	    hComponent, pCompPrv, nPortIndex, pBufferHdr,
	    pBufferHdr->pBuffer);

	for (count = 0; count < pCompPrv->nTotalBuffers; count++)
	{
		if (pCompPrv->tBufList[count].pBufHeader == pBufferHdr)
		{
			DOMX_DEBUG("Buffer Index of Match %d", count);
			break;
		}
	}
	PROXY_assert((count != pCompPrv->nTotalBuffers),
	    OMX_ErrorBadParameter,
	    "Could not find the mapped address in component private buffer list");

	pBuffer = (OMX_U32)pBufferHdr->pBuffer;
    pAuxBuf0 = (OMX_PTR) pBuffer;

#ifdef ENABLE_GRALLOC_BUFFERS
	if (pCompPrv->proxyPortBuffers[nPortIndex].proxyBufferType == GrallocPointers)
	{
		//Extracting buffer pointer from the gralloc buffer
		pAuxBuf0 = (OMX_U8 *)(((IMG_native_handle_t*)pBuffer)->fd[0]);
	}
#endif

	/*To find whether buffer is 2D or 1D */
	/*Not having asserts from this point since even if error occurs during
	   unmapping/freeing, still trying to clean up as much as possible */
	eError =
	    RPC_UTIL_GetStride(pCompPrv->hRemoteComp, nPortIndex, &nStride);
	if (eError == OMX_ErrorNone && nStride == LINUX_PAGE_SIZE)
	{
		if (pCompPrv->proxyPortBuffers[nPortIndex].proxyBufferType == BufferDescriptorVirtual2D)
		{
			pAuxBuf0 = (OMX_U8 *)(((OMX_TI_BUFFERDESCRIPTOR_TYPE*)pBuffer)->pBuf[0]);
		}
	}

	/*Not having asserts from this point since even if error occurs during
	   unmapping/freeing, still trying to clean up as much as possible */

	if (pCompPrv->tBufList[count].pRegisteredAufBux0 != NULL)
		pAuxBuf0 = pCompPrv->tBufList[count].pRegisteredAufBux0;

	eRPCError =
	    RPC_FreeBuffer(pCompPrv->hRemoteComp, nPortIndex,
	    pCompPrv->tBufList[count].pBufHeaderRemote, (OMX_U32) pAuxBuf0,
	    &eCompReturn);

	if (eRPCError != RPC_OMX_ErrorNone)
		eTmpRPCError = eRPCError;

	pPlatformPrivate = (OMX_TI_PLATFORMPRIVATE *)(pCompPrv->tBufList[count].pBufHeader)->
			pPlatformPrivate;

	if (pCompPrv->tBufList[count].pBufHeader)
	{
#ifdef ALLOCATE_TILER_BUFFER_IN_PROXY
#ifdef USE_ION
		if(pCompPrv->tBufList[count].pYBuffer)
		{
        		if (pCompPrv->bUseIon == OMX_TRUE)
			{
				if(pCompPrv->bMapIonBuffers == OMX_TRUE && pBufferHdr->pBuffer)
				{
	                                munmap(pBufferHdr->pBuffer, pBufferHdr->nAllocLen);
        				close(pCompPrv->tBufList[count].mmap_fd);
				}
				ion_free(pCompPrv->ion_fd, pCompPrv->tBufList[count].pYBuffer);
				pCompPrv->tBufList[count].pYBuffer = NULL;
			}
		}
#endif
#endif
		pMetaDataBuffer = pPlatformPrivate->pMetaDataBuffer;
		if (pMetaDataBuffer)
		{
#ifdef USE_ION
        		if (pCompPrv->bUseIon == OMX_TRUE)
			{
				if(pCompPrv->bMapIonBuffers == OMX_TRUE)
				{
					munmap(pMetaDataBuffer, pPlatformPrivate->nMetaDataSize);
				}
				close(pCompPrv->tBufList[count].mmap_fd_metadata_buff);
				ion_free(pCompPrv->ion_fd, pCompPrv->tBufList[count].pMetaDataBuffer);
				pPlatformPrivate->pMetaDataBuffer = NULL;
			}
#endif
		}

#ifdef USE_ION
	{
		// Need to unregister buffers when using ion and rpmsg
		if (pCompPrv->tBufList[count].pRegisteredAufBux0 != NULL)
		{
			eTmpRPCError = RPC_UnRegisterBuffer(pCompPrv->hRemoteComp,
								pCompPrv->tBufList[count].pRegisteredAufBux0);
			if (eTmpRPCError != RPC_OMX_ErrorNone) {
				eRPCError = eTmpRPCError;
			}
		}

		if (pCompPrv->tBufList[count].pRegisteredAufBux1 != NULL)
		{
			eTmpRPCError = RPC_UnRegisterBuffer(pCompPrv->hRemoteComp,
								pCompPrv->tBufList[count].pRegisteredAufBux1);
			if (eTmpRPCError != RPC_OMX_ErrorNone) {
				eRPCError = eTmpRPCError;
			}
		}
		if (pCompPrv->tBufList[count].pRegisteredAufBux2 != NULL)
		{
			eTmpRPCError = RPC_UnRegisterBuffer(pCompPrv->hRemoteComp,
								pCompPrv->tBufList[count].pRegisteredAufBux2);
			if (eTmpRPCError != RPC_OMX_ErrorNone) {
				eRPCError = eTmpRPCError;
			}
		}
	}
#endif
		if (pCompPrv->tBufList[count].pBufHeader->pPlatformPrivate)
		{
			TIMM_OSAL_Free(pCompPrv->tBufList[count].pBufHeader->
			    pPlatformPrivate);
		}
		TIMM_OSAL_Free(pCompPrv->tBufList[count].pBufHeader);
		TIMM_OSAL_Memset(&(pCompPrv->tBufList[count]), 0,
		    sizeof(PROXY_BUFFER_INFO));
	}
	pCompPrv->nAllocatedBuffers--;

	PROXY_checkRpcError();

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
OMX_ERRORTYPE __PROXY_SetParameter(OMX_IN OMX_HANDLETYPE hComponent,
	OMX_IN OMX_INDEXTYPE nParamIndex, OMX_IN OMX_PTR pParamStruct,
	OMX_PTR pLocBufNeedMap, OMX_U32 nNumOfLocalBuf)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone, eCompReturn = OMX_ErrorNone;
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	PROXY_COMPONENT_PRIVATE *pCompPrv = NULL;
	OMX_COMPONENTTYPE *hComp = (OMX_COMPONENTTYPE *) hComponent;
	OMX_TI_PARAM_USEBUFFERDESCRIPTOR *ptBufDescParam = NULL;
#ifdef ENABLE_GRALLOC_BUFFERS
	OMX_TI_PARAMUSENATIVEBUFFER *pParamNativeBuffer = NULL;
#endif
#ifdef USE_ION
	OMX_PTR *pAuxBuf = pLocBufNeedMap;
	OMX_PTR pRegistered = NULL;
#endif

	PROXY_require((pParamStruct != NULL), OMX_ErrorBadParameter, NULL);
	PROXY_require((hComp->pComponentPrivate != NULL),
	    OMX_ErrorBadParameter, NULL);

	pCompPrv = (PROXY_COMPONENT_PRIVATE *) hComp->pComponentPrivate;

	DOMX_ENTER
		("hComponent = %p, pCompPrv = %p, nParamIndex = %d, pParamStruct = %p",
		hComponent, pCompPrv, nParamIndex, pParamStruct);

	switch(nParamIndex)
	{
#ifdef ENABLE_GRALLOC_BUFFERS
		case OMX_TI_IndexUseNativeBuffers:
		{
			//Add check version.
			pParamNativeBuffer = (OMX_TI_PARAMUSENATIVEBUFFER* )pParamStruct;
			if(pParamNativeBuffer->bEnable == OMX_TRUE)
			{
				pCompPrv->proxyPortBuffers[pParamNativeBuffer->nPortIndex].proxyBufferType = GrallocPointers;
				pCompPrv->proxyPortBuffers[pParamNativeBuffer->nPortIndex].IsBuffer2D = OMX_TRUE;
			} else
			{
				/* Reset to defaults */
				pCompPrv->proxyPortBuffers[pParamNativeBuffer->nPortIndex].proxyBufferType = VirtualPointers;
				pCompPrv->proxyPortBuffers[pParamNativeBuffer->nPortIndex].IsBuffer2D = OMX_FALSE;
			}

			break;
		}
#endif
		case OMX_TI_IndexUseBufferDescriptor:
		     ptBufDescParam = (OMX_TI_PARAM_USEBUFFERDESCRIPTOR *) pParamStruct;
		     if(ptBufDescParam->bEnabled == OMX_TRUE)
		     {
			     if(ptBufDescParam->eBufferType == OMX_TI_BufferTypeVirtual2D)
			     {
			         pCompPrv->proxyPortBuffers[ptBufDescParam->nPortIndex].proxyBufferType = BufferDescriptorVirtual2D;
			         pCompPrv->proxyPortBuffers[ptBufDescParam->nPortIndex].IsBuffer2D = OMX_TRUE;
		             }
		     }
		     else if(ptBufDescParam->bEnabled == OMX_FALSE)
		     {
			     /* Reset to defaults*/
			     pCompPrv->proxyPortBuffers[ptBufDescParam->nPortIndex].proxyBufferType = VirtualPointers;
			     pCompPrv->proxyPortBuffers[ptBufDescParam->nPortIndex].IsBuffer2D = OMX_FALSE;
		     }
			eRPCError =
				RPC_SetParameter(pCompPrv->hRemoteComp, nParamIndex, pParamStruct,
					pLocBufNeedMap, nNumOfLocalBuf, &eCompReturn);
		     break;
		default:
		{
#ifdef USE_ION
			if (pAuxBuf != NULL) {
				int fd = *((int*)pAuxBuf);
				if (fd > -1) {
					eRPCError = RPC_RegisterBuffer(pCompPrv->hRemoteComp, *((int*)pAuxBuf),
							   &pRegistered, NULL, IONPointers);
					PROXY_checkRpcError();
					if (pRegistered)
						*pAuxBuf = pRegistered;
				}
			}
#endif
			eRPCError =
				RPC_SetParameter(pCompPrv->hRemoteComp, nParamIndex, pParamStruct,
					pLocBufNeedMap, nNumOfLocalBuf, &eCompReturn);
#ifdef USE_ION
			PROXY_checkRpcError();
			if (pRegistered != NULL) {
				eRPCError = RPC_UnRegisterBuffer(pCompPrv->hRemoteComp, pRegistered);
				PROXY_checkRpcError();
			}
#endif
		}
	}

	PROXY_checkRpcError();

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
OMX_ERRORTYPE PROXY_SetParameter(OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE nParamIndex, OMX_IN OMX_PTR pParamStruct)
{
	return __PROXY_SetParameter(hComponent, nParamIndex, pParamStruct, NULL, 0);
}


/* ===========================================================================*/
/**
 * @name __PROXY_GetParameter()
 * @brief
 * @param void
 * @return OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
OMX_ERRORTYPE __PROXY_GetParameter(OMX_IN OMX_HANDLETYPE hComponent,
		OMX_IN OMX_INDEXTYPE nParamIndex, OMX_INOUT OMX_PTR pParamStruct,
		OMX_PTR pLocBufNeedMap)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone, eCompReturn = OMX_ErrorNone;
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	PROXY_COMPONENT_PRIVATE *pCompPrv = NULL;
	OMX_COMPONENTTYPE *hComp = (OMX_COMPONENTTYPE *) hComponent;
	OMX_TI_PARAM_USEBUFFERDESCRIPTOR *ptBufDescParam = NULL;
#ifdef USE_ION
	OMX_PTR *pAuxBuf = pLocBufNeedMap;
	OMX_PTR pRegistered = NULL;
#endif

	PROXY_require((pParamStruct != NULL), OMX_ErrorBadParameter, NULL);
	PROXY_assert((hComp->pComponentPrivate != NULL),
			OMX_ErrorBadParameter, NULL);

	pCompPrv = (PROXY_COMPONENT_PRIVATE *) hComp->pComponentPrivate;

	DOMX_ENTER
		("hComponent = %p, pCompPrv = %p, nParamIndex = %d, pParamStruct = %p",
		 hComponent, pCompPrv, nParamIndex, pParamStruct);

	switch(nParamIndex)
	{
		case OMX_TI_IndexUseBufferDescriptor:
		     ptBufDescParam = (OMX_TI_PARAM_USEBUFFERDESCRIPTOR *) pParamStruct;
		     if(pCompPrv->proxyPortBuffers[ptBufDescParam->nPortIndex].proxyBufferType == BufferDescriptorVirtual2D)
		     {
			     ptBufDescParam->bEnabled = OMX_TRUE;
			     ptBufDescParam->eBufferType = OMX_TI_BufferTypeVirtual2D;
		     }
		     else
		     {
			     ptBufDescParam->bEnabled = OMX_FALSE;
			     ptBufDescParam->eBufferType = OMX_TI_BufferTypeMax;
		     }
		     break;

		default:
		{
#ifdef USE_ION
			if (pAuxBuf != NULL) {
				int fd = *((int*)pAuxBuf);
				if (fd > -1) {
					eRPCError = RPC_RegisterBuffer(pCompPrv->hRemoteComp, *((int*)pAuxBuf),
							   &pRegistered, NULL, IONPointers);
					PROXY_checkRpcError();
					if (pRegistered)
						*pAuxBuf = pRegistered;
				}
			}
#endif
			eRPCError = RPC_GetParameter(pCompPrv->hRemoteComp, nParamIndex, pParamStruct,
				pLocBufNeedMap, &eCompReturn);
#ifdef USE_ION
			PROXY_checkRpcError();
			if (pRegistered != NULL) {
				eRPCError = RPC_UnRegisterBuffer(pCompPrv->hRemoteComp, pRegistered);
				PROXY_checkRpcError();
			}
#endif
		}
	}

	PROXY_checkRpcError();

EXIT:
	DOMX_EXIT("eError: %d index: 0x%x", eError, nParamIndex);
	return eError;
}

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
OMX_ERRORTYPE PROXY_GetParameter(OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE nParamIndex, OMX_INOUT OMX_PTR pParamStruct)
{
	return __PROXY_GetParameter(hComponent, nParamIndex, pParamStruct, NULL);
}

/* ===========================================================================*/
/**
 * @name __PROXY_GetConfig()
 * @brief
 * @param void
 * @return OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
OMX_ERRORTYPE __PROXY_GetConfig(OMX_HANDLETYPE hComponent,
	OMX_INDEXTYPE nConfigIndex, OMX_PTR pConfigStruct, OMX_PTR pLocBufNeedMap)
{
#ifdef USE_ION
	OMX_PTR *pAuxBuf = pLocBufNeedMap;
	OMX_PTR pRegistered = NULL;
#endif

	OMX_ERRORTYPE eError = OMX_ErrorNone, eCompReturn = OMX_ErrorNone;
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	PROXY_COMPONENT_PRIVATE *pCompPrv = NULL;
	OMX_COMPONENTTYPE *hComp = (OMX_COMPONENTTYPE *) hComponent;

	PROXY_require((pConfigStruct != NULL), OMX_ErrorBadParameter, NULL);
	PROXY_require((hComp->pComponentPrivate != NULL),
		OMX_ErrorBadParameter, NULL);

	pCompPrv = (PROXY_COMPONENT_PRIVATE *) hComp->pComponentPrivate;

	DOMX_ENTER("hComponent = %p, pCompPrv = %p, nConfigIndex = %d, "
				"pConfigStruct = %p",
				hComponent, pCompPrv, nConfigIndex,
				pConfigStruct);

#ifdef USE_ION
	if (pAuxBuf != NULL) {
		int fd = *((int*)pAuxBuf);
		if (fd > -1) {
			eRPCError = RPC_RegisterBuffer(pCompPrv->hRemoteComp, *((int*)pAuxBuf),
					   &pRegistered, NULL, IONPointers);
			PROXY_checkRpcError();
			if (pRegistered)
				*pAuxBuf = pRegistered;
		}
	}
#endif

	eRPCError =
		RPC_GetConfig(pCompPrv->hRemoteComp, nConfigIndex, pConfigStruct,
			pLocBufNeedMap, &eCompReturn);
#ifdef USE_ION
	PROXY_checkRpcError();
	if (pRegistered != NULL) {
		eRPCError = RPC_UnRegisterBuffer(pCompPrv->hRemoteComp, pRegistered);
		PROXY_checkRpcError();
	}
#endif

	PROXY_checkRpcError();

 EXIT:
	DOMX_EXIT("eError: %d", eError);
	return eError;
}

/* ===========================================================================*/
/**
 * @name PROXY_GetConfig()
 * @brief
 * @param void
 * @return OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
OMX_ERRORTYPE PROXY_GetConfig(OMX_HANDLETYPE hComponent,
	OMX_INDEXTYPE nConfigIndex, OMX_PTR pConfigStruct)
{
	return __PROXY_GetConfig(hComponent, nConfigIndex, pConfigStruct, NULL);
}

/* ===========================================================================*/
/**
 * @name __PROXY_SetConfig()
 * @brief
 * @param void
 * @return OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
OMX_ERRORTYPE __PROXY_SetConfig(OMX_IN OMX_HANDLETYPE hComponent,
	OMX_IN OMX_INDEXTYPE nConfigIndex, OMX_IN OMX_PTR pConfigStruct,
	OMX_PTR pLocBufNeedMap)
{
#ifdef USE_ION
	OMX_PTR *pAuxBuf = pLocBufNeedMap;
	OMX_PTR pRegistered = NULL;
#endif

	OMX_ERRORTYPE eError = OMX_ErrorNone, eCompReturn = OMX_ErrorNone;
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	PROXY_COMPONENT_PRIVATE *pCompPrv = NULL;
	OMX_COMPONENTTYPE *hComp = (OMX_COMPONENTTYPE *) hComponent;

	PROXY_require((pConfigStruct != NULL), OMX_ErrorBadParameter, NULL);

	PROXY_assert((hComp->pComponentPrivate != NULL),
		OMX_ErrorBadParameter, NULL);

	pCompPrv = (PROXY_COMPONENT_PRIVATE *) hComp->pComponentPrivate;

	DOMX_ENTER("hComponent = %p, pCompPrv = %p, nConfigIndex = %d, "
				"pConfigStruct = %p",
				hComponent, pCompPrv, nConfigIndex,
				pConfigStruct);

#ifdef USE_ION
	if (pAuxBuf != NULL) {
		int fd = *((int*)pAuxBuf);
		if (fd > -1) {
			eRPCError = RPC_RegisterBuffer(pCompPrv->hRemoteComp, *((int*)pAuxBuf),
					   &pRegistered, NULL, IONPointers);
			PROXY_checkRpcError();
			if (pRegistered)
				*pAuxBuf = pRegistered;
		}
	}
#endif

	eRPCError =
		RPC_SetConfig(pCompPrv->hRemoteComp, nConfigIndex, pConfigStruct,
			pLocBufNeedMap, &eCompReturn);

#ifdef USE_ION
	PROXY_checkRpcError();
	if (pRegistered != NULL) {
		eRPCError = RPC_UnRegisterBuffer(pCompPrv->hRemoteComp, pRegistered);
		PROXY_checkRpcError();
	}
#endif

	PROXY_checkRpcError();

      EXIT:
	DOMX_EXIT("eError: %d", eError);
	return eError;
}

/* ===========================================================================*/
/**
 * @name PROXY_SetConfig()
 * @brief
 * @param void
 * @return OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
OMX_ERRORTYPE PROXY_SetConfig(OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE nConfigIndex, OMX_IN OMX_PTR pConfigStruct)
{
	return __PROXY_SetConfig(hComponent, nConfigIndex, pConfigStruct, NULL);
}


/* ===========================================================================*/
/**
 * @name PROXY_GetState()
 * @brief
 * @param void
 * @return OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
static OMX_ERRORTYPE PROXY_GetState(OMX_IN OMX_HANDLETYPE hComponent,
    OMX_OUT OMX_STATETYPE * pState)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone, eCompReturn = OMX_ErrorNone;
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	OMX_COMPONENTTYPE *hComp = hComponent;
	PROXY_COMPONENT_PRIVATE *pCompPrv = NULL;

	PROXY_require((pState != NULL), OMX_ErrorBadParameter, NULL);
	PROXY_require((hComp->pComponentPrivate != NULL),
	    OMX_ErrorBadParameter, NULL);

	pCompPrv = (PROXY_COMPONENT_PRIVATE *) hComp->pComponentPrivate;

	DOMX_ENTER("hComponent = %p, pCompPrv = %p", hComponent, pCompPrv);

	eRPCError = RPC_GetState(pCompPrv->hRemoteComp, pState, &eCompReturn);

	DOMX_DEBUG("Returned from RPC_GetState, state: = %x", *pState);

	PROXY_checkRpcError();

      EXIT:
	if (eError == OMX_ErrorHardware)
	{
		*pState = OMX_StateInvalid;
		eError = OMX_ErrorNone;
		DOMX_DEBUG("Invalid state returned from RPC_GetState, state due to ducati in faulty state");
	}
	DOMX_EXIT("eError: %d", eError);
	return eError;
}



/* ===========================================================================*/
/**
 * @name PROXY_SendCommand()
 * @brief
 * @param void
 * @return OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
OMX_ERRORTYPE PROXY_SendCommand(OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_COMMANDTYPE eCmd,
    OMX_IN OMX_U32 nParam, OMX_IN OMX_PTR pCmdData)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone, eCompReturn = OMX_ErrorNone;
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	PROXY_COMPONENT_PRIVATE *pCompPrv = NULL;
	OMX_COMPONENTTYPE *hComp = (OMX_COMPONENTTYPE *) hComponent;
	OMX_COMPONENTTYPE *pMarkComp = NULL;
	PROXY_COMPONENT_PRIVATE *pMarkCompPrv = NULL;
	OMX_PTR pMarkData = NULL, pMarkToBeFreedIfError = NULL;
	OMX_BOOL bIsProxy = OMX_FALSE;

	PROXY_require((hComp->pComponentPrivate != NULL),
	    OMX_ErrorBadParameter, NULL);

	pCompPrv = (PROXY_COMPONENT_PRIVATE *) hComp->pComponentPrivate;

	DOMX_ENTER
	    ("hComponent = %p, pCompPrv = %p, eCmd = %d, nParam = %d, pCmdData = %p",
	    hComponent, pCompPrv, eCmd, nParam, pCmdData);

	if (eCmd == OMX_CommandMarkBuffer)
	{
		PROXY_require(pCmdData != NULL, OMX_ErrorBadParameter, NULL);
		pMarkComp = (OMX_COMPONENTTYPE *)
		    (((OMX_MARKTYPE *) pCmdData)->hMarkTargetComponent);
		PROXY_require(pMarkComp != NULL, OMX_ErrorBadParameter, NULL);

		/* To check if mark comp is a proxy or a real component */
		eError = _RPC_IsProxyComponent(pMarkComp, &bIsProxy);
		PROXY_assert(eError == OMX_ErrorNone, eError, "");

		/*Replacing original mark data with proxy specific structure */
		pMarkData = ((OMX_MARKTYPE *) pCmdData)->pMarkData;
		((OMX_MARKTYPE *) pCmdData)->pMarkData =
		    TIMM_OSAL_Malloc(sizeof(PROXY_MARK_DATA), TIMM_OSAL_TRUE,
		    0, TIMMOSAL_MEM_SEGMENT_INT);
		PROXY_assert(((OMX_MARKTYPE *) pCmdData)->pMarkData != NULL,
		    OMX_ErrorInsufficientResources, "Malloc failed");
		pMarkToBeFreedIfError =
		    ((OMX_MARKTYPE *) pCmdData)->pMarkData;
		((PROXY_MARK_DATA *) (((OMX_MARKTYPE *)
			    pCmdData)->pMarkData))->hComponentActual =
		    pMarkComp;
		((PROXY_MARK_DATA *) (((OMX_MARKTYPE *)
			    pCmdData)->pMarkData))->pMarkDataActual =
		    pMarkData;

		/* If it is proxy component then replace hMarkTargetComponent
		   with remote handle */
		if (bIsProxy)
		{
			pMarkCompPrv = pMarkComp->pComponentPrivate;
			PROXY_assert(pMarkCompPrv != NULL,
			    OMX_ErrorBadParameter, NULL);

			/* Replacing with remote component handle */
			((OMX_MARKTYPE *) pCmdData)->hMarkTargetComponent =
			    ((RPC_OMX_CONTEXT *) pMarkCompPrv->hRemoteComp)->
			    hActualRemoteCompHandle;
		}
	}

	eRPCError =
	    RPC_SendCommand(pCompPrv->hRemoteComp, eCmd, nParam, pCmdData,
	    &eCompReturn);

	if (eCmd == OMX_CommandMarkBuffer && bIsProxy)
	{
		/*Resetting to original values */
		((OMX_MARKTYPE *) pCmdData)->hMarkTargetComponent = pMarkComp;
		((OMX_MARKTYPE *) pCmdData)->pMarkData = pMarkData;
	}

	PROXY_checkRpcError();

      EXIT:
	/*If SendCommand is about to return an error then this means that the
	   command has not been accepted by the component. Thus the allocated mark data
	   will be lost so free it here */
	if ((eError != OMX_ErrorNone) && pMarkToBeFreedIfError)
	{
		TIMM_OSAL_Free(pMarkToBeFreedIfError);
	}
	DOMX_EXIT("eError: %d", eError);
	return eError;
}



/* ===========================================================================*/
/**
 * @name PROXY_GetComponentVersion()
 * @brief
 * @param void
 * @return OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
static OMX_ERRORTYPE PROXY_GetComponentVersion(OMX_IN OMX_HANDLETYPE
    hComponent, OMX_OUT OMX_STRING pComponentName,
    OMX_OUT OMX_VERSIONTYPE * pComponentVersion,
    OMX_OUT OMX_VERSIONTYPE * pSpecVersion,
    OMX_OUT OMX_UUIDTYPE * pComponentUUID)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone, eCompReturn = OMX_ErrorNone;
	PROXY_COMPONENT_PRIVATE *pCompPrv = NULL;
	OMX_COMPONENTTYPE *hComp = hComponent;
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;

	DOMX_ENTER("hComponent = %p, pCompPrv = %p", hComponent, pCompPrv);

	PROXY_require((hComp->pComponentPrivate != NULL),
	    OMX_ErrorBadParameter, NULL);
	PROXY_require(pComponentName != NULL, OMX_ErrorBadParameter, NULL);
	PROXY_require(pComponentVersion != NULL, OMX_ErrorBadParameter, NULL);
	PROXY_require(pSpecVersion != NULL, OMX_ErrorBadParameter, NULL);
	PROXY_require(pComponentUUID != NULL, OMX_ErrorBadParameter, NULL);

	pCompPrv = (PROXY_COMPONENT_PRIVATE *) hComp->pComponentPrivate;

	eRPCError = RPC_GetComponentVersion(pCompPrv->hRemoteComp,
	    pComponentName,
	    pComponentVersion, pSpecVersion, pComponentUUID, &eCompReturn);

	PROXY_checkRpcError();

      EXIT:
	DOMX_EXIT("eError: %d", eError);
	return eError;
}



/* ===========================================================================*/
/**
 * @name PROXY_GetExtensionIndex()
 * @brief
 * @param void
 * @return OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
OMX_ERRORTYPE PROXY_GetExtensionIndex(OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_STRING cParameterName, OMX_OUT OMX_INDEXTYPE * pIndexType)
{

	OMX_ERRORTYPE eError = OMX_ErrorNone, eCompReturn = OMX_ErrorNone;
	PROXY_COMPONENT_PRIVATE *pCompPrv = NULL;
	OMX_COMPONENTTYPE *hComp = hComponent;
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;

	PROXY_require((hComp->pComponentPrivate != NULL),
	    OMX_ErrorBadParameter, NULL);
	PROXY_require(cParameterName != NULL, OMX_ErrorBadParameter, NULL);
	PROXY_require(pIndexType != NULL, OMX_ErrorBadParameter, NULL);

	pCompPrv = (PROXY_COMPONENT_PRIVATE *) hComp->pComponentPrivate;

	DOMX_ENTER("hComponent = %p, pCompPrv = %p, cParameterName = %s",
	    hComponent, pCompPrv, cParameterName);


#ifdef ENABLE_GRALLOC_BUFFERS
	// Check for NULL Parameters
	PROXY_require((cParameterName != NULL && pIndexType != NULL),
	    OMX_ErrorBadParameter, NULL);

	// Ensure that String length is not greater than Max allowed length
	PROXY_require(strlen(cParameterName) <= 127, OMX_ErrorBadParameter, NULL);

	if(strcmp(cParameterName, "OMX.google.android.index.enableAndroidNativeBuffers") == 0)
	{
		// If Index type is 2D Buffer Allocated Dimension
		*pIndexType = (OMX_INDEXTYPE) OMX_TI_IndexUseNativeBuffers;
		goto EXIT;
	}
	else if (strcmp(cParameterName, "OMX.google.android.index.useAndroidNativeBuffer2") == 0)
	{
		//This is call just a dummy for android to support backward compatibility
		*pIndexType = (OMX_INDEXTYPE) NULL;
		goto EXIT;
	}
	else
	{
		eRPCError = RPC_GetExtensionIndex(pCompPrv->hRemoteComp,
                   cParameterName, pIndexType, &eCompReturn);

		PROXY_checkRpcError();
	}
#else
	eRPCError = RPC_GetExtensionIndex(pCompPrv->hRemoteComp,
	    cParameterName, pIndexType, &eCompReturn);

	PROXY_checkRpcError();
#endif

      EXIT:
	DOMX_EXIT("eError: %d", eError);
	return eError;
}




/* ===========================================================================*/
/**
 * @name PROXY_ComponentRoleEnum()
 * @brief
 * @param void
 * @return OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
static OMX_ERRORTYPE PROXY_ComponentRoleEnum(OMX_IN OMX_HANDLETYPE hComponent,
    OMX_OUT OMX_U8 * cRole, OMX_IN OMX_U32 nIndex)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;

	DOMX_ENTER("hComponent = %p", hComponent);
	DOMX_DEBUG(" EMPTY IMPLEMENTATION ");

	DOMX_EXIT("eError: %d", eError);
	return eError;
}


/* ===========================================================================*/
/**
 * @name PROXY_ComponentTunnelRequest()
 * @brief
 * @param void
 * @return OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
static OMX_ERRORTYPE PROXY_ComponentTunnelRequest(OMX_IN OMX_HANDLETYPE
    hComponent, OMX_IN OMX_U32 nPort, OMX_IN OMX_HANDLETYPE hTunneledComp,
    OMX_IN OMX_U32 nTunneledPort,
    OMX_INOUT OMX_TUNNELSETUPTYPE * pTunnelSetup)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;

	DOMX_ENTER("hComponent = %p", hComponent);
	DOMX_DEBUG(" EMPTY IMPLEMENTATION ");
	PROXY_COMPONENT_PRIVATE *pOutCompPrv = NULL;
	PROXY_COMPONENT_PRIVATE *pInCompPrv  = NULL;
	OMX_COMPONENTTYPE       *hOutComp    = hComponent;
	OMX_COMPONENTTYPE       *hInComp     = hTunneledComp;
	OMX_ERRORTYPE           eCompReturn = OMX_ErrorNone;
	RPC_OMX_ERRORTYPE       eRPCError    = RPC_OMX_ErrorNone;
	PROXY_assert((hOutComp->pComponentPrivate != NULL),
	    OMX_ErrorBadParameter, NULL);
	PROXY_assert((hInComp->pComponentPrivate != NULL),
	    OMX_ErrorBadParameter, NULL);

        //TBD
        //PROXY_assert(nPort != 1, OMX_ErrorBadParameter, NULL);
        //PROXY_assert(nTunnelPort != 0, OMX_ErrorBadParameter, NULL);
	pOutCompPrv = (PROXY_COMPONENT_PRIVATE *) hOutComp->pComponentPrivate;
	pInCompPrv  = (PROXY_COMPONENT_PRIVATE *) hInComp->pComponentPrivate;
	DOMX_ENTER("hOutComp=%p, pOutCompPrv=%p, hInComp=%p, pInCompPrv=%p, nOutPort=%d, nInPort=%d \n",
	        hOutComp, pOutCompPrv, hInComp, pInCompPrv, nPort, nTunneledPort);

	DOMX_INFO("PROXY_ComponentTunnelRequest:: hOutComp=%p, pOutCompPrv=%p, hInComp=%p, pInCompPrv=%p, nOutPort=%d, nInPort=%d \n ",
	        hOutComp, pOutCompPrv, hInComp, pInCompPrv, nPort, nTunneledPort);
       eRPCError = RPC_ComponentTunnelRequest(pOutCompPrv->hRemoteComp, nPort,
	        pInCompPrv->hRemoteComp, nTunneledPort, pTunnelSetup, &eCompReturn);
        DOMX_INFO("\nafter: RPC_ComponentTunnelRequest = 0x%x\n ", eRPCError);
        PROXY_checkRpcError();

EXIT:
	DOMX_EXIT("eError: %d", eError);
	return eError;
}


/* ===========================================================================*/
/**
 * @name PROXY_SetCallbacks()
 * @brief
 * @param void
 * @return OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
static OMX_ERRORTYPE PROXY_SetCallbacks(OMX_HANDLETYPE hComponent,
    OMX_CALLBACKTYPE * pCallBacks, OMX_PTR pAppData)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	PROXY_COMPONENT_PRIVATE *pCompPrv;
	OMX_COMPONENTTYPE *hComp = (OMX_COMPONENTTYPE *) hComponent;

	PROXY_require((pCallBacks != NULL), OMX_ErrorBadParameter, NULL);

	PROXY_assert((hComp->pComponentPrivate != NULL),
	    OMX_ErrorBadParameter, NULL);

	pCompPrv = (PROXY_COMPONENT_PRIVATE *) hComp->pComponentPrivate;

	DOMX_ENTER("hComponent = %p, pCompPrv = %p", hComponent, pCompPrv);

	/*Store App callback and data to proxy- managed by proxy */
	pCompPrv->tCBFunc = *pCallBacks;
	pCompPrv->pILAppData = pAppData;

      EXIT:
	DOMX_EXIT("eError: %d", eError);
	return eError;
}


/* ===========================================================================*/
/**
 * @name PROXY_UseEGLImage()
 * @brief : This returns error not implemented by default as no component
 *          implements this. In case there is a requiremet, support for this
 *          can be added later.
 *
 */
/* ===========================================================================*/
static OMX_ERRORTYPE PROXY_UseEGLImage(OMX_HANDLETYPE hComponent,
    OMX_BUFFERHEADERTYPE ** ppBufferHdr,
    OMX_U32 nPortIndex, OMX_PTR pAppPrivate, void *pBuffer)
{
	return OMX_ErrorNotImplemented;
}


/* ===========================================================================*/
/**
 * @name PROXY_ComponentDeInit()
 * @brief
 * @param void
 * @return OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
OMX_ERRORTYPE PROXY_ComponentDeInit(OMX_HANDLETYPE hComponent)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone, eCompReturn = OMX_ErrorNone;
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone, eTmpRPCError =
	    RPC_OMX_ErrorNone;
	PROXY_COMPONENT_PRIVATE *pCompPrv;
	OMX_COMPONENTTYPE *hComp = (OMX_COMPONENTTYPE *) hComponent;
	OMX_U32 count = 0, nStride = 0;
	OMX_PTR pMetaDataBuffer = NULL;

	DOMX_ENTER("hComponent = %p", hComponent);

	PROXY_assert((hComp->pComponentPrivate != NULL),
	    OMX_ErrorBadParameter, NULL);

	pCompPrv = (PROXY_COMPONENT_PRIVATE *) hComp->pComponentPrivate;

	ion_close(pCompPrv->ion_fd);

	for (count = 0; count < pCompPrv->nTotalBuffers; count++)
	{
		if (pCompPrv->tBufList[count].pBufHeader)
		{
#ifdef ALLOCATE_TILER_BUFFER_IN_PROXY
			if(pCompPrv->tBufList[count].pYBuffer)
			{
#ifdef USE_ION
				if(pCompPrv->tBufList[count].pYBuffer)
				{
					if (pCompPrv->bUseIon == OMX_TRUE)
					{
						if(pCompPrv->bMapIonBuffers == OMX_TRUE)
						{
					                munmap(pCompPrv->tBufList[count].pBufHeader->pBuffer, pCompPrv->tBufList[count].pBufHeader->nAllocLen);
							close(pCompPrv->tBufList[count].mmap_fd);
						}
						ion_free(pCompPrv->ion_fd, pCompPrv->tBufList[count].pYBuffer);
						pCompPrv->tBufList[count].pYBuffer = NULL;
					}
				}
#endif
			}
#endif
			pMetaDataBuffer = ((OMX_TI_PLATFORMPRIVATE *)(pCompPrv->tBufList[count].pBufHeader)->
				pPlatformPrivate)->pMetaDataBuffer;
			if (pMetaDataBuffer)
			{
#ifdef USE_ION
				if (pCompPrv->bUseIon == OMX_TRUE)
				{
					if(pCompPrv->bMapIonBuffers == OMX_TRUE)
					{
			                        munmap(pMetaDataBuffer, ((OMX_TI_PLATFORMPRIVATE *)(pCompPrv->tBufList[count].pBufHeader)->
								pPlatformPrivate)->nMetaDataSize);
					}
					close(pCompPrv->tBufList[count].mmap_fd_metadata_buff);
					ion_free(pCompPrv->ion_fd, pMetaDataBuffer);
					((OMX_TI_PLATFORMPRIVATE *)(pCompPrv->tBufList[count].pBufHeader)->
						pPlatformPrivate)->pMetaDataBuffer = NULL;
				}
#endif
			}
			if (pCompPrv->tBufList[count].pBufHeader->pPlatformPrivate)
			{
				TIMM_OSAL_Free(pCompPrv->tBufList[count].pBufHeader->
				    pPlatformPrivate);
			}
			TIMM_OSAL_Free(pCompPrv->tBufList[count].pBufHeader);
			TIMM_OSAL_Memset(&(pCompPrv->tBufList[count]), 0,
			    sizeof(PROXY_BUFFER_INFO));
		}
	}

	KPI_OmxCompDeinit(hComponent);

	eRPCError = RPC_FreeHandle(pCompPrv->hRemoteComp, &eCompReturn);
	if (eRPCError != RPC_OMX_ErrorNone)
		eTmpRPCError = eRPCError;

	eRPCError = RPC_InstanceDeInit(pCompPrv->hRemoteComp);
	if (eRPCError != RPC_OMX_ErrorNone)
		eTmpRPCError = eRPCError;

	if (pCompPrv->cCompName)
	{
		TIMM_OSAL_Free(pCompPrv->cCompName);
	}

	if (pCompPrv)
	{
		TIMM_OSAL_Free(pCompPrv);
	}

	eRPCError = eTmpRPCError;
	PROXY_checkRpcError();

      EXIT:
	DOMX_EXIT("eError: %d", eError);
	return eError;
}


/* ===========================================================================*/
/**
 * @name OMX_ProxyCommonInit()
 * @brief
 * @param void
 * @return OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
OMX_ERRORTYPE OMX_ProxyCommonInit(OMX_HANDLETYPE hComponent)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone, eCompReturn = OMX_ErrorNone;
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	PROXY_COMPONENT_PRIVATE *pCompPrv;
	OMX_COMPONENTTYPE *hComp = (OMX_COMPONENTTYPE *) hComponent;
	OMX_HANDLETYPE hRemoteComp = NULL;
        OMX_U32 i = 0;

	DOMX_ENTER("hComponent = %p", hComponent);

	TIMM_OSAL_UpdateTraceLevel();

	PROXY_require((hComp->pComponentPrivate != NULL),
	    OMX_ErrorBadParameter, NULL);

	pCompPrv = (PROXY_COMPONENT_PRIVATE *) hComp->pComponentPrivate;

	pCompPrv->nTotalBuffers = 0;
	pCompPrv->nAllocatedBuffers = 0;
	pCompPrv->proxyEmptyBufferDone = PROXY_EmptyBufferDone;
	pCompPrv->proxyFillBufferDone = PROXY_FillBufferDone;
	pCompPrv->proxyEventHandler = PROXY_EventHandler;

        for (i=0; i<PROXY_MAXNUMOFPORTS ; i++)
        {
              pCompPrv->proxyPortBuffers[i].proxyBufferType = VirtualPointers;
        }

	eRPCError = RPC_InstanceInit(pCompPrv->cCompName, &hRemoteComp);
	PROXY_assert(eRPCError == RPC_OMX_ErrorNone,
	    OMX_ErrorUndefined, "Error initializing RPC");
	PROXY_assert(hRemoteComp != NULL,
	    OMX_ErrorUndefined, "Error initializing RPC");

	//Send the proxy component handle for pAppData
	eRPCError =
	    RPC_GetHandle(hRemoteComp, pCompPrv->cCompName,
	    (OMX_PTR) hComponent, NULL, &eCompReturn);

	PROXY_checkRpcError();

	hComp->SetCallbacks = PROXY_SetCallbacks;
	hComp->ComponentDeInit = PROXY_ComponentDeInit;
	hComp->UseBuffer = PROXY_UseBuffer;
	hComp->GetParameter = PROXY_GetParameter;
	hComp->SetParameter = PROXY_SetParameter;
	hComp->EmptyThisBuffer = PROXY_EmptyThisBuffer;
	hComp->FillThisBuffer = PROXY_FillThisBuffer;
	hComp->GetComponentVersion = PROXY_GetComponentVersion;
	hComp->SendCommand = PROXY_SendCommand;
	hComp->GetConfig = PROXY_GetConfig;
	hComp->SetConfig = PROXY_SetConfig;
	hComp->GetState = PROXY_GetState;
	hComp->GetExtensionIndex = PROXY_GetExtensionIndex;
	hComp->FreeBuffer = PROXY_FreeBuffer;
	hComp->ComponentRoleEnum = PROXY_ComponentRoleEnum;
	hComp->AllocateBuffer = PROXY_AllocateBuffer;
	hComp->ComponentTunnelRequest = PROXY_ComponentTunnelRequest;
	hComp->UseEGLImage = PROXY_UseEGLImage;

	pCompPrv->hRemoteComp = hRemoteComp;

#ifdef USE_ION
	pCompPrv->bUseIon = OMX_TRUE;
	pCompPrv->bMapIonBuffers = OMX_TRUE;

	pCompPrv->ion_fd = ion_open();
	if(pCompPrv->ion_fd == 0)
	{
		DOMX_ERROR("ion_open failed!!!");
		return OMX_ErrorInsufficientResources;
	}
#endif

	KPI_OmxCompInit(hComponent);

      EXIT:
	if (eError != OMX_ErrorNone)
		RPC_InstanceDeInit(hRemoteComp);
	DOMX_EXIT("eError: %d", eError);

	return eError;
}



/* ===========================================================================*/
/**
 * @name RPC_UTIL_GetStride()
 * @brief Gets stride on this port. Used to determine whether buffer is 1D or 2D
 * @param hRemoteComp [IN]  : Remote component handle.
 * @param nPortIndex [IN]   : Port index.
 * @param nStride [OUT]     : Stride returned by the component.
 * @return OMX_ErrorNone = Successful
 */
/* ===========================================================================*/
OMX_ERRORTYPE RPC_UTIL_GetStride(OMX_COMPONENTTYPE * hRemoteComp,
    OMX_U32 nPortIndex, OMX_U32 * nStride)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone, eCompReturn = OMX_ErrorNone;
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	OMX_PARAM_PORTDEFINITIONTYPE sPortDef;

	/*Initializing Structure */
	sPortDef.nSize = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
	sPortDef.nVersion.s.nVersionMajor = OMX_VER_MAJOR;
	sPortDef.nVersion.s.nVersionMinor = OMX_VER_MINOR;
	sPortDef.nVersion.s.nRevision = 0x0;
	sPortDef.nVersion.s.nStep = 0x0;
	sPortDef.nPortIndex = nPortIndex;

	eRPCError =
	    RPC_GetParameter(hRemoteComp, OMX_IndexParamPortDefinition,
	    (OMX_PTR) (&sPortDef), NULL, &eCompReturn);
	PROXY_checkRpcError();

	if (sPortDef.eDomain == OMX_PortDomainVideo)
	{
		*nStride = sPortDef.format.video.nStride;
	} else if (sPortDef.eDomain == OMX_PortDomainImage)
	{
		*nStride = sPortDef.format.image.nStride;
	} else if (sPortDef.eDomain == OMX_PortDomainMax && nPortIndex == 0)
	{
		/*Temp - just for testing sample */
		*nStride = LINUX_PAGE_SIZE;
	} else
	{
		*nStride = 0;
	}

      EXIT:
	return eError;
}



/* ===========================================================================*/
/**
 * @name RPC_UTIL_GetNumLines()
 * @brief
 * @param void
 * @return OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
OMX_ERRORTYPE RPC_UTIL_GetNumLines(OMX_COMPONENTTYPE * hRemoteComp,
    OMX_U32 nPortIndex, OMX_U32 * nNumOfLines)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_ERRORTYPE eCompReturn;
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	OMX_BOOL bUseEnhancedPortReconfig = OMX_FALSE;

	OMX_PARAM_PORTDEFINITIONTYPE portDef;
	OMX_CONFIG_RECTTYPE sRect;

	DOMX_ENTER("");

	/*initializing Structure */
	portDef.nSize = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
	portDef.nVersion.s.nVersionMajor = 0x1;
	portDef.nVersion.s.nVersionMinor = 0x1;
	portDef.nVersion.s.nRevision = 0x0;
	portDef.nVersion.s.nStep = 0x0;

	portDef.nPortIndex = nPortIndex;

	sRect.nSize = sizeof(OMX_CONFIG_RECTTYPE);
	sRect.nVersion.s.nVersionMajor = 0x1;
	sRect.nVersion.s.nVersionMinor = 0x1;
	sRect.nVersion.s.nRevision = 0x0;
	sRect.nVersion.s.nStep = 0x0;

	sRect.nPortIndex = nPortIndex;
	sRect.nLeft = 0;
	sRect.nTop = 0;
	sRect.nHeight = 0;
	sRect.nWidth = 0;

#ifdef USE_ENHANCED_PORTRECONFIG
	bUseEnhancedPortReconfig = OMX_TRUE;
#endif
	eRPCError = RPC_GetParameter(hRemoteComp,
	    OMX_TI_IndexParam2DBufferAllocDimension,
	    (OMX_PTR) & sRect, NULL, &eCompReturn);
	if (eRPCError == RPC_OMX_ErrorNone)
	{
		DOMX_DEBUG(" PROXY_UTIL Get Parameter Successful");
		eError = eCompReturn;
	} else
	{
		DOMX_ERROR("RPC_GetParameter returned error 0x%x", eRPCError);
		eError = OMX_ErrorUndefined;
		goto EXIT;
	}

	if (eCompReturn == OMX_ErrorNone && bUseEnhancedPortReconfig == OMX_FALSE)
	{
		*nNumOfLines = sRect.nHeight;
	} else if (eCompReturn == OMX_ErrorUnsupportedIndex || bUseEnhancedPortReconfig == OMX_TRUE)
	{
		eRPCError =
		    RPC_GetParameter(hRemoteComp,
		    OMX_IndexParamPortDefinition, (OMX_PTR) & portDef,
		    NULL, &eCompReturn);

		if (eRPCError == RPC_OMX_ErrorNone)
		{
			DOMX_DEBUG(" PROXY_UTIL Get Parameter Successful");
			eError = eCompReturn;
		} else
		{
			DOMX_ERROR("RPC_GetParameter returned error 0x%x",
			    eRPCError);
			eError = OMX_ErrorUndefined;
			goto EXIT;
		}

		if (eCompReturn == OMX_ErrorNone)
		{

			//start with 1 meaning 1D buffer
			*nNumOfLines = 1;

			if (portDef.eDomain == OMX_PortDomainVideo)
			{
				*nNumOfLines =
				    portDef.format.video.nFrameHeight;
				//DOMX_DEBUG("Port definition Type is video...");
				//DOMX_DEBUG("&&Colorformat is:%p", portDef.format.video.eColorFormat);
				//DOMX_DEBUG("nFrameHeight is:%d", portDef.format.video.nFrameHeight);
				//*nNumOfLines = portDef.format.video.nFrameHeight;

				//if((portDef.format.video.eColorFormat == OMX_COLOR_FormatYUV420PackedSemiPlanar) ||
				//  (portDef.format.video.eColorFormat == OMX_COLOR_FormatYUV420Planar))
				//{
				//DOMX_DEBUG("Setting FrameHeight as Number of lines...");
				//*nNumOfLines = portDef.format.video.nFrameHeight;
				//}
			} else if (portDef.eDomain == OMX_PortDomainImage)
			{
				DOMX_DEBUG
				    ("Image DOMAIN TILER SUPPORT for NV12 format only");
				*nNumOfLines =
				    portDef.format.image.nFrameHeight;
			} else if (portDef.eDomain == OMX_PortDomainAudio)
			{
				DOMX_DEBUG("Audio DOMAIN TILER SUPPORT");
			} else if (portDef.eDomain == OMX_PortDomainOther)
			{
				DOMX_DEBUG("Other DOMAIN TILER SUPPORT");
			} else
			{	//this is the sample component test
				//Temporary - just to get check functionality
				DOMX_DEBUG("Sample component TILER SUPPORT");
				*nNumOfLines = 4;
			}
		} else
		{
			DOMX_ERROR(" ERROR IN RECOVERING UV POINTER");
		}
	} else
	{
		DOMX_ERROR(" ERROR IN RECOVERING UV POINTER");
	}

	DOMX_DEBUG("Port Number: %d :: NumOfLines %d", nPortIndex,
	    *nNumOfLines);

      EXIT:
	DOMX_EXIT("eError: %d", eError);
	return eError;
}


/* ===========================================================================*/
/**
 * @name _RPC_IsProxyComponent()
 * @brief This function calls GetComponentVersion API on the component and
 *        based on the component name decidec whether the component is a proxy
 *        or real component. The naming component convention assumed is
 *        <OMX>.<Company Name>.<Core Name>.<Domain>.<Component Details> with
 *        all characters in upper case for e.g. OMX.TI.DUCATI1.VIDEO.H264E
 * @param hComponent [IN] : The component handle
 *        bIsProxy [OUT]  : Set to true is handle is for a proxy component
 * @return OMX_ErrorNone = Successful
 *
 **/
/* ===========================================================================*/
OMX_ERRORTYPE _RPC_IsProxyComponent(OMX_HANDLETYPE hComponent,
    OMX_BOOL * bIsProxy)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_S8 cComponentName[MAXNAMESIZE] = { 0 }
	, cCoreName[32] =
	{
	0};
	OMX_VERSIONTYPE sCompVer, sSpecVer;
	OMX_UUIDTYPE sCompUUID;
	OMX_U32 i = 0, ret = 0;

	eError =
	    OMX_GetComponentVersion(hComponent, (OMX_STRING) cComponentName,
	    &sCompVer, &sSpecVer, &sCompUUID);
	PROXY_assert(eError == OMX_ErrorNone, eError, "");
	ret =
	    sscanf((char *)cComponentName, "%*[^'.'].%*[^'.'].%[^'.'].%*s",
	    cCoreName);
	PROXY_assert(ret == 1, OMX_ErrorBadParameter,
	    "Incorrect component name");
	for (i = 0; i < CORE_MAX; i++)
	{
		if (strcmp((char *)cCoreName, Core_Array[i]) == 0)
			break;
	}
	PROXY_assert(i < CORE_MAX, OMX_ErrorBadParameter,
	    "Unknown core name");

	/* If component name indicates remote core, it means proxy
	   component */
	if ((i == CORE_SYSM3) || (i == CORE_APPM3) || (i == CORE_TESLA))
		*bIsProxy = OMX_TRUE;
	else
		*bIsProxy = OMX_FALSE;

      EXIT:
	return eError;
}
