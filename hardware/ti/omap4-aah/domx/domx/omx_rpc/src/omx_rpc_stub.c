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
 *  @file  omx_rpc_stub.c
 *         This file contains methods that provides the functionality for
 *         the OpenMAX1.1 DOMX Framework RPC Stub implementations.
 *
 *  @path \WTSD_DucatiMMSW\framework\domx\omx_rpc\src
 *
 *  @rev 1.0
 */

/*==============================================================
 *! Revision History
 *! ============================
 *! 30-Apr-2010 Abhishek Ranka : Fixed GetExtension issue
 *!
 *! 29-Mar-2010 Abhishek Ranka : Revamped DOMX implementation
 *!
 *! 19-August-2009 B Ravi Kiran ravi.kiran@ti.com: Initial Version
 *================================================================*/
/******************************************************************
 *   INCLUDE FILES
 ******************************************************************/
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "omx_rpc.h"
#include "omx_rpc_utils.h"
#include "omx_proxy_common.h"
#include "omx_rpc_stub.h"
#include <OMX_TI_Common.h>
#include <timm_osal_interfaces.h>

#include <linux/rpmsg_omx.h>
#include "rpmsg_omx_defs.h"

/******************************************************************
 *   EXTERNS
 ******************************************************************/

/******************************************************************
 *   MACROS - LOCAL
 ******************************************************************/

//#define RPC_MSGPIPE_SIZE (4)
#define RPC_MSG_SIZE_FOR_PIPE (sizeof(OMX_PTR))

/* When this is defined ETB/FTB calls are made in sync mode. Undefining will
 * result in these calls being sent via async mode. Sync mode leads to correct
 * functionality as per OMX spec but has a slight performance penalty. Async
 * mode sacrifices strict adherence to spec for some gain in performance. */
#define RPC_SYNC_MODE


#define RPC_getPacket(nPacketSize, pPacket) do { \
    pPacket = TIMM_OSAL_Malloc(nPacketSize, TIMM_OSAL_TRUE, 0, TIMMOSAL_MEM_SEGMENT_INT); \
    RPC_assert(pPacket != NULL, RPC_OMX_ErrorInsufficientResources, \
           "Error Allocating RCM Message Frame"); \
    TIMM_OSAL_Memset(pPacket, 0, nPacketSize); \
    } while(0)

#define RPC_freePacket(pPacket) do { \
    if(pPacket != NULL) TIMM_OSAL_Free(pPacket); \
    } while(0)

#define RPC_sendPacket_sync(hCtx, pPacket, nPacketSize, nFxnIdx, pRetPacket, nSize) do { \
    status = write(hCtx->fd_omx, pPacket, nPacketSize); \
    RPC_freePacket(pPacket); \
    pPacket = NULL; \
    if(status < 0 && errno == ENXIO) {  \
         RPC_assert(0, RPC_OMX_ErrorHardware, "Write failed - Ducati in faulty state"); \
    }  \
    if(status != (signed)nPacketSize) { \
        DOMX_ERROR("Write failed returning status = 0x%x",status); \
        RPC_assert(0, RPC_OMX_ErrorUndefined, "Write failed"); \
    }  \
    eError = TIMM_OSAL_ReadFromPipe(hCtx->pMsgPipe[nFxnIdx], &pRetPacket, \
        RPC_MSG_SIZE_FOR_PIPE, (TIMM_OSAL_U32 *)(&nSize), TIMM_OSAL_SUSPEND); \
    RPC_assert(eError == TIMM_OSAL_ERR_NONE, eError, \
        "Read failed"); \
    } while(0)

/*Set bit 31 on fxn idx as it is static function*/
#define RPC_initPacket(pPacket, pOmxPacket, pData, nFxnIdx, nPacketSize) do { \
    pOmxPacket = (struct omx_packet *)pPacket; \
    pData = pOmxPacket->data; \
    pOmxPacket->desc |= OMX_DESC_MSG << OMX_DESC_TYPE_SHIFT; \
    pOmxPacket->msg_id = 0; \
    pOmxPacket->flags = OMX_POOLID_JOBID_DEFAULT; \
    pOmxPacket->fxn_idx = (nFxnIdx | 0x80000000); \
    pOmxPacket->result = 0; \
    pOmxPacket->data_size = nPacketSize; \
    } while(0)

//Async to be checked later - most probably same as sync but without the read
#if 0

#define RPC_sendPacket_async(HRCM, pPacket, nFxnIdx) do { \
    pPacket->nFxnIdx = nFxnIdx; \
    status = RcmClient_execCmd(HRCM, pPacket); \
    if(status < 0) { \
    RPC_freePacket(HRCM, pPacket); \
    pPacket = NULL; \
    RPC_assert(0, RPC_OMX_RCM_ErrorExecFail, \
           "RcmClient_exec failed"); \
    } \
    } while(0)

#define RPC_checkAsyncErrors(rcmHndl, pPacket) do { \
    status = RcmClient_checkForError(rcmHndl, &pPacket); \
    if(status < 0) { \
        RPC_freePacket(rcmHndl, pPacket); \
        pPacket = NULL; \
    } \
    RPC_assert(status >= 0, RPC_OMX_RCM_ClientFail, \
        "Async error check returned error"); \
    } while(0)

#endif
/* ===========================================================================*/
/**
 * @name RPC_GetHandle()
 * @brief Remote invocation stub for OMX_GetHandle
 * @param hRPCCtx [OUT]         : The RPC context handle.
 * @param cComponentName [IN]   : Name of the component that is to be created
 *                                on Remote core.
 * @param pAppData [IN]         : The AppData passed by the user.
 * @param pCallBacks [IN]       : The callback pointers passed by the caller.
 * @param eCompReturn [OUT]     : This is return value returned by the remote
 *                                component.
 * @return RPC_OMX_ErrorNone = Successful
 */
/* ===========================================================================*/
RPC_OMX_ERRORTYPE RPC_GetHandle(OMX_HANDLETYPE hRPCCtx,
    OMX_STRING cComponentName, OMX_PTR pAppData,
    OMX_CALLBACKTYPE * pCallBacks, OMX_ERRORTYPE * eCompReturn)
{
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	TIMM_OSAL_ERRORTYPE eError = TIMM_OSAL_ERR_NONE;
	OMX_U32 nPacketSize = RPC_PACKET_SIZE;
	TIMM_OSAL_PTR pPacket = NULL, pRetPacket = NULL, pData =
	    NULL, pRetData = NULL;
	OMX_U32 nPos = 0, nSize = 0, nOffset = 0;
	RPC_OMX_CONTEXT *hCtx = hRPCCtx;
	OMX_HANDLETYPE hComp = NULL;
	OMX_HANDLETYPE hActualComp = NULL;
	OMX_S32 status = 0;
	RPC_OMX_FXN_IDX_TYPE nFxnIdx;
	struct omx_packet *pOmxPacket = NULL;

	DOMX_ENTER("");
	DOMX_DEBUG("RPC_GetHandle: Recieved GetHandle request from %s",
	    cComponentName);

	nFxnIdx = RPC_OMX_FXN_IDX_GET_HANDLE;
	RPC_getPacket(nPacketSize, pPacket);
	RPC_initPacket(pPacket, pOmxPacket, pData, nFxnIdx, nPacketSize);

	DOMX_DEBUG("Packing data");
	/*No buffer mapping required */
	RPC_SETFIELDVALUE(pData, nPos, RPC_OMX_MAP_INFO_NONE,
	    RPC_OMX_MAP_INFO_TYPE);
	RPC_SETFIELDVALUE(pData, nPos, nOffset, OMX_U32);

	RPC_SETFIELDCOPYGEN(pData, nPos, cComponentName,
	    OMX_MAX_STRINGNAME_SIZE);
	RPC_SETFIELDVALUE(pData, nPos, pAppData, OMX_PTR);

	DOMX_DEBUG("Sending data");
	RPC_sendPacket_sync(hCtx, pPacket, nPacketSize, nFxnIdx, pRetPacket,
	    nSize);

	*eCompReturn = (OMX_ERRORTYPE) (((struct omx_packet *) pRetPacket)->result);

	if (*eCompReturn == OMX_ErrorNone)
	{
		pRetData = ((struct omx_packet *) pRetPacket)->data;
		RPC_GETFIELDVALUE(pRetData, nPos, hComp, OMX_HANDLETYPE);
		DOMX_DEBUG("Remote Handle 0x%x", hComp);
		hCtx->hRemoteHandle = hComp;
		/* The handle received above is used for all communications
		   with the remote component but is not the actual component
		   handle (it is actually the rpc context handle which
		   contains lot of other info). The handle recd. below is the
		   actual remote component handle. This is used at present for
		   mark buffer implementation since in that case it is not
		   feasible to send the context handle */
		RPC_GETFIELDVALUE(pRetData, nPos, hActualComp,
		    OMX_HANDLETYPE);
		DOMX_DEBUG("Actual Remote Comp Handle 0x%x", hActualComp);
		hCtx->hActualRemoteCompHandle = hActualComp;
	}

    /* Save context information */
    hCtx->pAppData = pAppData;

      EXIT:
	if (pPacket)
		RPC_freePacket(pPacket);
	if (pRetPacket && *eCompReturn != OMX_ErrorHardware)
		RPC_freePacket(pRetPacket);

	DOMX_EXIT("");
	return eRPCError;
}



/* ===========================================================================*/
/**
 * @name RPC_FreeHandle()
 * @brief Remote invocation stub for OMX_FreeHandle
 * @param hRPCCtx [IN]      : The RPC context handle.
 * @param eCompReturn [OUT] : Return value returned by the remote component.
 * @return RPC_OMX_ErrorNone = Successful
 */
/* ===========================================================================*/
RPC_OMX_ERRORTYPE RPC_FreeHandle(OMX_HANDLETYPE hRPCCtx,
    OMX_ERRORTYPE * eCompReturn)
{
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	TIMM_OSAL_ERRORTYPE eError = TIMM_OSAL_ERR_NONE;
	OMX_U32 nPacketSize = RPC_PACKET_SIZE;
	TIMM_OSAL_PTR pPacket = NULL, pRetPacket = NULL, pData = NULL;
	OMX_S32 status = 0;
	RPC_OMX_FXN_IDX_TYPE nFxnIdx;
	OMX_U32 nPos = 0, nSize = 0, nOffset = 0;
	RPC_OMX_CONTEXT *hCtx = hRPCCtx;
	OMX_HANDLETYPE hComp = hCtx->hRemoteHandle;
	struct omx_packet *pOmxPacket = NULL;

	DOMX_ENTER("");

	nFxnIdx = RPC_OMX_FXN_IDX_FREE_HANDLE;
	RPC_getPacket(nPacketSize, pPacket);
	RPC_initPacket(pPacket, pOmxPacket, pData, nFxnIdx, nPacketSize);

	/*No buffer mapping required */
	RPC_SETFIELDVALUE(pData, nPos, RPC_OMX_MAP_INFO_NONE,
	    RPC_OMX_MAP_INFO_TYPE);
	RPC_SETFIELDVALUE(pData, nPos, nOffset, OMX_U32);

	RPC_SETFIELDVALUE(pData, nPos, hComp, OMX_HANDLETYPE);

	RPC_sendPacket_sync(hCtx, pPacket, nPacketSize, nFxnIdx, pRetPacket,
	    nSize);

	*eCompReturn = (OMX_ERRORTYPE) (((struct omx_packet *) pRetPacket)->result);

      EXIT:
	if (pPacket)
		RPC_freePacket(pPacket);
	if (pRetPacket && *eCompReturn != OMX_ErrorHardware)
		RPC_freePacket(pRetPacket);

	DOMX_EXIT("");
	return eRPCError;

}



/* ===========================================================================*/
/**
 * @name RPC_SetParameter()
 * @brief Remote invocation stub for OMX_SetParameter
 * @param hRPCCtx [IN]      : The RPC context handle.
 * @param nParamIndex [IN]  : Same as nParamIndex received at the proxy.
 * @param pCompParam [IN]   : Same as pCompParam recieved at the proxy.
 * @param eCompReturn [OUT] : Return value returned by the remote component.
 * @return RPC_OMX_ErrorNone = Successful
 */
/* ===========================================================================*/
RPC_OMX_ERRORTYPE RPC_SetParameter(OMX_HANDLETYPE hRPCCtx,
    OMX_INDEXTYPE nParamIndex, OMX_PTR pCompParam,
    OMX_PTR pLocBufNeedMap, OMX_U32 nNumOfLocalBuf, OMX_ERRORTYPE * eCompReturn)
{

	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	TIMM_OSAL_ERRORTYPE eError = TIMM_OSAL_ERR_NONE;
	TIMM_OSAL_PTR pPacket = NULL, pRetPacket = NULL, pData = NULL;
	OMX_U32 nPacketSize = RPC_PACKET_SIZE;
	RPC_OMX_FXN_IDX_TYPE nFxnIdx;
	OMX_U32 nPos = 0, nSize = 0, nOffset = 0;
	OMX_S32 status = 0;
	RPC_OMX_CONTEXT *hCtx = hRPCCtx;
	OMX_HANDLETYPE hComp = hCtx->hRemoteHandle;
	OMX_U32 structSize = 0;
	struct omx_packet *pOmxPacket = NULL;

	nFxnIdx = RPC_OMX_FXN_IDX_SET_PARAMETER;
	RPC_getPacket(nPacketSize, pPacket);
	RPC_initPacket(pPacket, pOmxPacket, pData, nFxnIdx, nPacketSize);

	if (pLocBufNeedMap != NULL && (pLocBufNeedMap - pCompParam) >= 0 ) {
		if (nNumOfLocalBuf == 1) {
			RPC_SETFIELDVALUE(pData, nPos, RPC_OMX_MAP_INFO_ONE_BUF,
				RPC_OMX_MAP_INFO_TYPE);
		}
		else if (nNumOfLocalBuf == 2) {
			RPC_SETFIELDVALUE(pData, nPos, RPC_OMX_MAP_INFO_TWO_BUF,
				RPC_OMX_MAP_INFO_TYPE);
		}
		nOffset = (pLocBufNeedMap - pCompParam) +
			sizeof(RPC_OMX_MAP_INFO_TYPE) + sizeof(OMX_U32) +
			sizeof(OMX_HANDLETYPE) + sizeof(OMX_INDEXTYPE);
	} else {
		/*No buffer mapping required */
		RPC_SETFIELDVALUE(pData, nPos, RPC_OMX_MAP_INFO_NONE,
			RPC_OMX_MAP_INFO_TYPE);
	}

	RPC_SETFIELDVALUE(pData, nPos, nOffset, OMX_U32);

	RPC_SETFIELDVALUE(pData, nPos, hComp, OMX_HANDLETYPE);
	RPC_SETFIELDVALUE(pData, nPos, nParamIndex, OMX_INDEXTYPE);
	structSize = RPC_UTIL_GETSTRUCTSIZE(pCompParam);
	RPC_SETFIELDCOPYGEN(pData, nPos, pCompParam, structSize);

	RPC_sendPacket_sync(hCtx, pPacket, nPacketSize, nFxnIdx, pRetPacket,
	    nSize);

	*eCompReturn = (OMX_ERRORTYPE) (((struct omx_packet *) pRetPacket)->result);

      EXIT:
	if (pPacket)
		RPC_freePacket(pPacket);
	if (pRetPacket && *eCompReturn != OMX_ErrorHardware)
		RPC_freePacket(pRetPacket);

	DOMX_EXIT("");
	return eRPCError;
}



/* ===========================================================================*/
/**
 * @name RPC_GetParameter()
 * @brief Remote invocation stub for OMX_GetParameter
 * @param hRPCCtx [IN]      : The RPC context handle.
 * @param nParamIndex [IN]  : Same as nParamIndex received at the proxy.
 * @param pCompParam [IN]   : Same as pCompParam recieved at the proxy.
 * @param eCompReturn [OUT] : Return value returned by the remote component.
 * @return RPC_OMX_ErrorNone = Successful
 */
/* ===========================================================================*/
RPC_OMX_ERRORTYPE RPC_GetParameter(OMX_HANDLETYPE hRPCCtx,
    OMX_INDEXTYPE nParamIndex, OMX_PTR pCompParam,
    OMX_PTR pLocBufNeedMap, OMX_ERRORTYPE * eCompReturn)
{
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	TIMM_OSAL_ERRORTYPE eError = TIMM_OSAL_ERR_NONE;
	TIMM_OSAL_PTR pPacket = NULL, pRetPacket = NULL, pData =
	    NULL, pRetData = NULL;
	OMX_U32 nPacketSize = RPC_PACKET_SIZE;
	RPC_OMX_FXN_IDX_TYPE nFxnIdx;
	OMX_U32 nPos = 0, nSize = 0, nDataOffset = 0, nOffset = 0;
	OMX_S32 status = 0;
	RPC_OMX_CONTEXT *hCtx = hRPCCtx;
	OMX_HANDLETYPE hComp = hCtx->hRemoteHandle;
	OMX_U32 structSize = 0;
	struct omx_packet *pOmxPacket = NULL;

	DOMX_ENTER("");

	nFxnIdx = RPC_OMX_FXN_IDX_GET_PARAMETER;
	RPC_getPacket(nPacketSize, pPacket);
	RPC_initPacket(pPacket, pOmxPacket, pData, nFxnIdx, nPacketSize);

	if (pLocBufNeedMap != NULL && (pLocBufNeedMap - pCompParam) >= 0 ) {
		RPC_SETFIELDVALUE(pData, nPos, RPC_OMX_MAP_INFO_ONE_BUF,
			RPC_OMX_MAP_INFO_TYPE);
		nOffset = (pLocBufNeedMap - pCompParam) +
			sizeof(RPC_OMX_MAP_INFO_TYPE) + sizeof(OMX_U32) +
			sizeof(OMX_HANDLETYPE) + sizeof(OMX_INDEXTYPE);
	} else {
		/*No buffer mapping required */
		RPC_SETFIELDVALUE(pData, nPos, RPC_OMX_MAP_INFO_NONE,
			RPC_OMX_MAP_INFO_TYPE);
	}

	RPC_SETFIELDVALUE(pData, nPos, nOffset, OMX_U32);

	RPC_SETFIELDVALUE(pData, nPos, hComp, OMX_HANDLETYPE);
	RPC_SETFIELDVALUE(pData, nPos, nParamIndex, OMX_INDEXTYPE);
	nDataOffset = nPos;
	structSize = RPC_UTIL_GETSTRUCTSIZE(pCompParam);
	RPC_SETFIELDCOPYGEN(pData, nPos, pCompParam, structSize);

	RPC_sendPacket_sync(hCtx, pPacket, nPacketSize, nFxnIdx, pRetPacket,
	    nSize);

	*eCompReturn = (OMX_ERRORTYPE) (((struct omx_packet *) pRetPacket)->result);

	if (*eCompReturn == OMX_ErrorNone)
	{
		pRetData = ((struct omx_packet *) pRetPacket)->data;
		/*pCompParam is returned in the same location in which it was sent */
		RPC_GETFIELDCOPYGEN(pRetData, nDataOffset, pCompParam,
		    structSize);
	}

      EXIT:
	if (pPacket)
		RPC_freePacket(pPacket);
	//In case of Error Hardware this packet gets freed in omx_rpc.c
	if (pRetPacket && *eCompReturn != OMX_ErrorHardware)
		RPC_freePacket(pRetPacket);

	DOMX_EXIT("");
	return eRPCError;
}



/* ===========================================================================*/
/**
 * @name RPC_SetConfig()
 * @brief Remote invocation stub for OMX_SetConfig
 * @param hRPCCtx [IN]      : The RPC context handle.
 * @param nConfigIndex [IN] : Same as nConfigIndex received at the proxy.
 * @param pCompConfig [IN]  : Same as pCompConfig recieved at the proxy.
 * @param eCompReturn [OUT] : Return value returned by the remote component.
 * @return RPC_OMX_ErrorNone = Successful
 */
/* ===========================================================================*/
RPC_OMX_ERRORTYPE RPC_SetConfig(OMX_HANDLETYPE hRPCCtx,
    OMX_INDEXTYPE nConfigIndex, OMX_PTR pCompConfig,
    OMX_PTR pLocBufNeedMap, OMX_ERRORTYPE * eCompReturn)
{
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	TIMM_OSAL_ERRORTYPE eError = TIMM_OSAL_ERR_NONE;
	TIMM_OSAL_PTR pPacket = NULL, pRetPacket = NULL, pData = NULL;
	OMX_U32 nPacketSize = RPC_PACKET_SIZE;
	RPC_OMX_FXN_IDX_TYPE nFxnIdx;
	OMX_U32 nPos = 0, nSize = 0, nOffset = 0;
	OMX_S32 status = 0;
	RPC_OMX_CONTEXT *hCtx = hRPCCtx;
	OMX_HANDLETYPE hComp = hCtx->hRemoteHandle;
	OMX_U32 structSize = 0;
	struct omx_packet *pOmxPacket = NULL;

	DOMX_ENTER("");

	nFxnIdx = RPC_OMX_FXN_IDX_SET_CONFIG;
	RPC_getPacket(nPacketSize, pPacket);
	RPC_initPacket(pPacket, pOmxPacket, pData, nFxnIdx, nPacketSize);

	if (pLocBufNeedMap != NULL && (pLocBufNeedMap - pCompConfig) >= 0 ) {
		RPC_SETFIELDVALUE(pData, nPos, RPC_OMX_MAP_INFO_ONE_BUF,
			RPC_OMX_MAP_INFO_TYPE);
		nOffset = (pLocBufNeedMap - pCompConfig) +
			sizeof(RPC_OMX_MAP_INFO_TYPE) + sizeof(OMX_U32) +
			sizeof(OMX_HANDLETYPE) + sizeof(OMX_INDEXTYPE);
	} else {
		/*No buffer mapping required */
		RPC_SETFIELDVALUE(pData, nPos, RPC_OMX_MAP_INFO_NONE,
			RPC_OMX_MAP_INFO_TYPE);
	}

	RPC_SETFIELDVALUE(pData, nPos, nOffset, OMX_U32);

	RPC_SETFIELDVALUE(pData, nPos, hComp, OMX_HANDLETYPE);
	RPC_SETFIELDVALUE(pData, nPos, nConfigIndex, OMX_INDEXTYPE);
	structSize = RPC_UTIL_GETSTRUCTSIZE(pCompConfig);
	RPC_SETFIELDCOPYGEN(pData, nPos, pCompConfig, structSize);

	RPC_sendPacket_sync(hCtx, pPacket, nPacketSize, nFxnIdx, pRetPacket,
	    nSize);
	*eCompReturn = (OMX_ERRORTYPE) (((struct omx_packet *) pRetPacket)->result);

      EXIT:
	if (pPacket)
		RPC_freePacket(pPacket);
	if (pRetPacket && *eCompReturn != OMX_ErrorHardware)
		RPC_freePacket(pRetPacket);

	DOMX_EXIT("");
	return eRPCError;
}

/* ===========================================================================*/
/**
 * @name RPC_GetConfig()
 * @brief Remote invocation stub for OMX_GetConfig
 * @param hRPCCtx [IN]      : The RPC context handle.
 * @param nConfigIndex [IN] : Same as nConfigIndex received at the proxy.
 * @param pCompConfig [IN]  : Same as pCompConfig recieved at the proxy.
 * @param eCompReturn [OUT] : Return value returned by the remote component.
 * @return RPC_OMX_ErrorNone = Successful
 */
/* ===========================================================================*/
RPC_OMX_ERRORTYPE RPC_GetConfig(OMX_HANDLETYPE hRPCCtx,
    OMX_INDEXTYPE nConfigIndex, OMX_PTR pCompConfig,
    OMX_PTR pLocBufNeedMap, OMX_ERRORTYPE * eCompReturn)
{
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	TIMM_OSAL_ERRORTYPE eError = TIMM_OSAL_ERR_NONE;
	TIMM_OSAL_PTR pPacket = NULL, pRetPacket = NULL, pData =
		NULL, pRetData = NULL;
	OMX_U32 nPacketSize = RPC_PACKET_SIZE;
	RPC_OMX_FXN_IDX_TYPE nFxnIdx;
	OMX_U32 nPos = 0, nSize = 0, nOffset = 0, nDataOffset = 0;
	OMX_S32 status = 0;
	RPC_OMX_CONTEXT *hCtx = hRPCCtx;
	OMX_HANDLETYPE hComp = hCtx->hRemoteHandle;
	OMX_U32 structSize = 0;
	struct omx_packet *pOmxPacket = NULL;

	DOMX_ENTER("");

	nFxnIdx = RPC_OMX_FXN_IDX_GET_CONFIG;
	RPC_getPacket(nPacketSize, pPacket);
	RPC_initPacket(pPacket, pOmxPacket, pData, nFxnIdx, nPacketSize);

	if (pLocBufNeedMap != NULL && (pLocBufNeedMap - pCompConfig) >= 0 ) {
		RPC_SETFIELDVALUE(pData, nPos, RPC_OMX_MAP_INFO_ONE_BUF,
			RPC_OMX_MAP_INFO_TYPE);
		nOffset = (pLocBufNeedMap - pCompConfig) +
			sizeof(RPC_OMX_MAP_INFO_TYPE) + sizeof(OMX_U32) +
			sizeof(OMX_HANDLETYPE) + sizeof(OMX_INDEXTYPE);
	} else {
		/*No buffer mapping required */
		RPC_SETFIELDVALUE(pData, nPos, RPC_OMX_MAP_INFO_NONE,
			RPC_OMX_MAP_INFO_TYPE);
	}

	RPC_SETFIELDVALUE(pData, nPos, nOffset, OMX_U32);

	RPC_SETFIELDVALUE(pData, nPos, hComp, OMX_HANDLETYPE);
	RPC_SETFIELDVALUE(pData, nPos, nConfigIndex, OMX_INDEXTYPE);
	nDataOffset = nPos;
	structSize = RPC_UTIL_GETSTRUCTSIZE(pCompConfig);
	RPC_SETFIELDCOPYGEN(pData, nPos, pCompConfig, structSize);

	RPC_sendPacket_sync(hCtx, pPacket, nPacketSize, nFxnIdx, pRetPacket,
	    nSize);

	*eCompReturn = (OMX_ERRORTYPE) (((struct omx_packet *) pRetPacket)->result);

	if (*eCompReturn == RPC_OMX_ErrorNone)
	{
		pRetData = ((struct omx_packet *) pRetPacket)->data;
		/*pCompParam is returned in the same location in which it was sent */
		RPC_GETFIELDCOPYGEN(pRetData, nDataOffset, pCompConfig,
		    structSize);
	}

      EXIT:
	if (pPacket)
		RPC_freePacket(pPacket);
	if (pRetPacket && *eCompReturn != OMX_ErrorHardware)
		RPC_freePacket(pRetPacket);

	DOMX_EXIT("");
	return eRPCError;
}



/* ===========================================================================*/
/**
 * @name RPC_SendCommand()
 * @brief Remote invocation stub for OMX_SendCommand
 * @param hRPCCtx [IN]      : The RPC context handle.
 * @param eCmd [IN]         : Same as eCmd received at the proxy.
 * @param nParam [IN]       : Same as nParam recieved at the proxy.
 * @param pCmdData [IN]     : Same as pCmdData recieved at the proxy.
 * @param eCompReturn [OUT] : Return value returned by the remote component.
 * @return RPC_OMX_ErrorNone = Successful
 */
/* ===========================================================================*/
RPC_OMX_ERRORTYPE RPC_SendCommand(OMX_HANDLETYPE hRPCCtx,
    OMX_COMMANDTYPE eCmd, OMX_U32 nParam, OMX_PTR pCmdData,
    OMX_ERRORTYPE * eCompReturn)
{
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	TIMM_OSAL_ERRORTYPE eError = TIMM_OSAL_ERR_NONE;
	TIMM_OSAL_PTR pPacket = NULL, pRetPacket = NULL, pData = NULL;
	OMX_U32 nPacketSize = RPC_PACKET_SIZE;
	RPC_OMX_FXN_IDX_TYPE nFxnIdx;
	OMX_U32 nPos = 0, nSize = 0, nOffset = 0;
	OMX_S32 status = 0;
	RPC_OMX_CONTEXT *hCtx = hRPCCtx;
	OMX_HANDLETYPE hComp = hCtx->hRemoteHandle;
	OMX_U32 structSize = 0;
	struct omx_packet *pOmxPacket = NULL;

	DOMX_ENTER("");

	nFxnIdx = RPC_OMX_FXN_IDX_SEND_CMD;
	RPC_getPacket(nPacketSize, pPacket);
	RPC_initPacket(pPacket, pOmxPacket, pData, nFxnIdx, nPacketSize);

	/*No buffer mapping required */
	RPC_SETFIELDVALUE(pData, nPos, RPC_OMX_MAP_INFO_NONE,
	    RPC_OMX_MAP_INFO_TYPE);
	RPC_SETFIELDVALUE(pData, nPos, nOffset, OMX_U32);

	RPC_SETFIELDVALUE(pData, nPos, hComp, OMX_HANDLETYPE);
	RPC_SETFIELDVALUE(pData, nPos, eCmd, OMX_COMMANDTYPE);
	RPC_SETFIELDVALUE(pData, nPos, nParam, OMX_U32);

	if (pCmdData != NULL && eCmd == OMX_CommandMarkBuffer)
	{
		/*The RPC_UTIL_GETSTRUCTSIZE will not work here since OMX_MARKTYPE structure
		   does not have nSize field */
		structSize = sizeof(OMX_MARKTYPE);
		RPC_SETFIELDCOPYGEN(pData, nPos, pCmdData, structSize);
	} else if (pCmdData != NULL)
	{
		structSize = RPC_UTIL_GETSTRUCTSIZE(pCmdData);
		RPC_SETFIELDCOPYGEN(pData, nPos, pCmdData, structSize);
	}

	RPC_sendPacket_sync(hCtx, pPacket, nPacketSize, nFxnIdx, pRetPacket,
	    nSize);

	*eCompReturn = (OMX_ERRORTYPE) (((struct omx_packet *) pRetPacket)->result);

      EXIT:
	if (pPacket)
		RPC_freePacket(pPacket);
	if (pRetPacket && *eCompReturn != OMX_ErrorHardware)
		RPC_freePacket(pRetPacket);

	DOMX_EXIT("");
	return eRPCError;
}



/* ===========================================================================*/
/**
 * @name RPC_GetState()
 * @brief Remote invocation stub for OMX_GetState
 * @param hRPCCtx [IN]      : The RPC context handle.
 * @param pState [OUT]      : State variable, to be filled in and returned by
 *                            the remote component..
 * @param eCompReturn [OUT] : Return value returned by the remote component.
 * @return RPC_OMX_ErrorNone = Successful
 */
/* ===========================================================================*/
RPC_OMX_ERRORTYPE RPC_GetState(OMX_HANDLETYPE hRPCCtx, OMX_STATETYPE * pState,
    OMX_ERRORTYPE * eCompReturn)
{
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	TIMM_OSAL_ERRORTYPE eError = TIMM_OSAL_ERR_NONE;
	TIMM_OSAL_PTR pPacket = NULL, pRetPacket = NULL, pData =
	    NULL, pRetData = NULL;
	OMX_U32 nPacketSize = RPC_PACKET_SIZE;
	RPC_OMX_FXN_IDX_TYPE nFxnIdx;
	OMX_U32 nPos = 0, nSize = 0, nOffset = 0;
	OMX_S32 status = 0;
	RPC_OMX_CONTEXT *hCtx = hRPCCtx;
	OMX_HANDLETYPE hComp = hCtx->hRemoteHandle;
	struct omx_packet *pOmxPacket = NULL;

	DOMX_ENTER("");

	nFxnIdx = RPC_OMX_FXN_IDX_GET_STATE;
	RPC_getPacket(nPacketSize, pPacket);
	RPC_initPacket(pPacket, pOmxPacket, pData, nFxnIdx, nPacketSize);

	/*No buffer mapping required */
	RPC_SETFIELDVALUE(pData, nPos, RPC_OMX_MAP_INFO_NONE,
	    RPC_OMX_MAP_INFO_TYPE);
	RPC_SETFIELDVALUE(pData, nPos, nOffset, OMX_U32);

	RPC_SETFIELDVALUE(pData, nPos, hComp, OMX_HANDLETYPE);

	RPC_sendPacket_sync(hCtx, pPacket, nPacketSize, nFxnIdx, pRetPacket,
	    nSize);

	*eCompReturn = (OMX_ERRORTYPE) (((struct omx_packet *) pRetPacket)->result);

	if (*eCompReturn == OMX_ErrorNone)
	{
		pRetData = ((struct omx_packet *) pRetPacket)->data;
		RPC_GETFIELDCOPYTYPE(pRetData, nPos, pState, OMX_STATETYPE);
	}

      EXIT:
	if (pPacket)
		RPC_freePacket(pPacket);
	if (pRetPacket && *eCompReturn != OMX_ErrorHardware)
		RPC_freePacket(pRetPacket);

	DOMX_EXIT("");
	return eRPCError;
}



/* ===========================================================================*/
/**
 * @name RPC_GetComponentVersion()
 * @brief Remote invocation stub for OMX_GetComponentVersion
 * @param hRPCCtx [IN]            : The RPC context handle.
 * @param pComponentName [OUT]    : Component name, to be filled in and returned
 *                                  by the remote component.
 * @param pComponentVersion [OUT] : Component version info, to be filled in and
 *                                  returned by the remote component.
 * @param pSpecVersion [OUT]      : Spec version info, to be filled in and
 *                                  returned by the remote component.
 * @param pComponentUUID [OUT]    : Component UUID info, to be filled in and
 *                                  returned by the remote component (optional).
 * @param eCompReturn [OUT]       : Return value returned by the remote
 *                                  component.
 * @return RPC_OMX_ErrorNone = Successful
 */
/* ===========================================================================*/
RPC_OMX_ERRORTYPE RPC_GetComponentVersion(OMX_HANDLETYPE hRPCCtx,
    OMX_STRING pComponentName, OMX_VERSIONTYPE * pComponentVersion,
    OMX_VERSIONTYPE * pSpecVersion, OMX_UUIDTYPE * pComponentUUID,
    OMX_ERRORTYPE * eCompReturn)
{
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	OMX_U32 nPacketSize = RPC_PACKET_SIZE;
	TIMM_OSAL_ERRORTYPE eError = TIMM_OSAL_ERR_NONE;
	TIMM_OSAL_PTR pPacket = NULL, pRetPacket = NULL, pData =
	    NULL, pRetData = NULL;
	OMX_S32 status = 0;
	RPC_OMX_FXN_IDX_TYPE nFxnIdx;
	OMX_U32 nPos = 0, nSize = 0, nOffset = 0;
	RPC_OMX_CONTEXT *hCtx = hRPCCtx;
	OMX_HANDLETYPE hComp = hCtx->hRemoteHandle;
	struct omx_packet *pOmxPacket = NULL;

	DOMX_ENTER("");

	nFxnIdx = RPC_OMX_FXN_IDX_GET_VERSION;
	RPC_getPacket(nPacketSize, pPacket);
	RPC_initPacket(pPacket, pOmxPacket, pData, nFxnIdx, nPacketSize);

	/*No buffer mapping required */
	RPC_SETFIELDVALUE(pData, nPos, RPC_OMX_MAP_INFO_NONE,
	    RPC_OMX_MAP_INFO_TYPE);
	RPC_SETFIELDVALUE(pData, nPos, nOffset, OMX_U32);

	RPC_SETFIELDVALUE(pData, nPos, hComp, OMX_HANDLETYPE);

	RPC_sendPacket_sync(hCtx, pPacket, nPacketSize, nFxnIdx, pRetPacket,
	    nSize);

	*eCompReturn = (OMX_ERRORTYPE) (((struct omx_packet *) pRetPacket)->result);

	if (*eCompReturn == OMX_ErrorNone)
	{
		pRetData = ((struct omx_packet *) pRetPacket)->data;
		RPC_GETFIELDCOPYGEN(pRetData, nPos, pComponentName,
		    OMX_MAX_STRINGNAME_SIZE);
		RPC_GETFIELDCOPYTYPE(pRetData, nPos, pComponentVersion,
		    OMX_VERSIONTYPE);
		RPC_GETFIELDCOPYTYPE(pRetData, nPos, pSpecVersion,
		    OMX_VERSIONTYPE);
		memcpy(pComponentUUID,(OMX_UUIDTYPE *)( (OMX_U32)pRetData + nPos), sizeof(OMX_UUIDTYPE));
	}

      EXIT:
	if (pPacket)
		RPC_freePacket(pPacket);
	if (pRetPacket && *eCompReturn != OMX_ErrorHardware)
		RPC_freePacket(pRetPacket);

	return eRPCError;
}



/* ===========================================================================*/
/**
 * @name RPC_GetExtensionIndex()
 * @brief Remote invocation stub for OMX_GetExtensionIndex
 * @param hRPCCtx [IN]        : The RPC context handle.
 * @param cParameterName [IN] : The parameter name sent by the user.
 * @param pIndexType [OUT]    : Index type returned by the remote component.
 * @param eCompReturn [OUT]   : Return value returned by the remote component.
 * @return RPC_OMX_ErrorNone = Successful
 */
/* ===========================================================================*/
RPC_OMX_ERRORTYPE RPC_GetExtensionIndex(OMX_HANDLETYPE hRPCCtx,
    OMX_STRING cParameterName, OMX_INDEXTYPE * pIndexType,
    OMX_ERRORTYPE * eCompReturn)
{

	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	OMX_U32 nPos = 0, nSize = 0, nOffset = 0;
	RPC_OMX_CONTEXT *hCtx = hRPCCtx;
	OMX_HANDLETYPE hComp = hCtx->hRemoteHandle;
	OMX_U32 nPacketSize = RPC_PACKET_SIZE;
	TIMM_OSAL_ERRORTYPE eError = TIMM_OSAL_ERR_NONE;
	TIMM_OSAL_PTR pPacket = NULL, pRetPacket = NULL, pData =
	    NULL, pRetData = NULL;
	OMX_S32 status = 0;
	RPC_OMX_FXN_IDX_TYPE nFxnIdx;
	struct omx_packet *pOmxPacket = NULL;

	nFxnIdx = RPC_OMX_FXN_IDX_GET_EXT_INDEX;

	RPC_getPacket(nPacketSize, pPacket);
	RPC_initPacket(pPacket, pOmxPacket, pData, nFxnIdx, nPacketSize);

	/*No buffer mapping required */
	RPC_SETFIELDVALUE(pData, nPos, RPC_OMX_MAP_INFO_NONE,
	    RPC_OMX_MAP_INFO_TYPE);
	RPC_SETFIELDVALUE(pData, nPos, nOffset, OMX_U32);

	RPC_SETFIELDVALUE(pData, nPos, hComp, OMX_HANDLETYPE);
	RPC_SETFIELDCOPYGEN(pData, nPos, cParameterName,
	    OMX_MAX_STRINGNAME_SIZE);

	RPC_sendPacket_sync(hCtx, pPacket, nPacketSize, nFxnIdx, pRetPacket,
	    nSize);

	*eCompReturn = (OMX_ERRORTYPE) (((struct omx_packet *) pRetPacket)->result);

	if (*eCompReturn == OMX_ErrorNone)
	{
		pRetData = ((struct omx_packet *) pRetPacket)->data;
		RPC_GETFIELDCOPYTYPE(pRetData, nPos, pIndexType,
		    OMX_INDEXTYPE);
	}

      EXIT:
	if (pPacket)
		RPC_freePacket(pPacket);
	if (pRetPacket && *eCompReturn != OMX_ErrorHardware)
		RPC_freePacket(pRetPacket);

	return eRPCError;

}

/* ***************************** DATA APIs ******************************** */

/* ===========================================================================*/
/**
 * @name RPC_AllocateBuffer()
 * @brief Remote invocation stub for OMX_AllcateBuffer()
 * @param size   : Size of the incoming RCM message (parameter used in the RCM alloc call)
 * @param *data  : Pointer to the RCM message/buffer received
 * @return RPC_OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
RPC_OMX_ERRORTYPE RPC_AllocateBuffer(OMX_HANDLETYPE hRPCCtx,
    OMX_INOUT OMX_BUFFERHEADERTYPE ** ppBufferHdr, OMX_IN OMX_U32 nPortIndex,
    OMX_U32 * pBufHeaderRemote, OMX_PTR pAppPrivate, OMX_U32 nSizeBytes,
    OMX_ERRORTYPE * eCompReturn)
{
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	TIMM_OSAL_ERRORTYPE eError = TIMM_OSAL_ERR_NONE;
	TIMM_OSAL_PTR pPacket = NULL, pRetPacket = NULL, pData =
	    NULL, pRetData = NULL;
	OMX_U32 nPacketSize = RPC_PACKET_SIZE;
	RPC_OMX_FXN_IDX_TYPE nFxnIdx;
	OMX_U32 nPos = 0, nSize = 0, nOffset = 0;
	OMX_S32 status = 0;
	RPC_OMX_CONTEXT *hCtx = hRPCCtx;
	OMX_HANDLETYPE hComp = hCtx->hRemoteHandle;
	OMX_TI_PLATFORMPRIVATE *pPlatformPrivate = NULL;
	OMX_BUFFERHEADERTYPE *pBufferHdr = *ppBufferHdr;
	struct omx_packet *pOmxPacket = NULL;

	DOMX_ENTER("");

	nFxnIdx = RPC_OMX_FXN_IDX_ALLOCATE_BUFFER;
	RPC_getPacket(nPacketSize, pPacket);
	RPC_initPacket(pPacket, pOmxPacket, pData, nFxnIdx, nPacketSize);

	/*No buffer mapping required */
	RPC_SETFIELDVALUE(pData, nPos, RPC_OMX_MAP_INFO_NONE,
	    RPC_OMX_MAP_INFO_TYPE);
	RPC_SETFIELDVALUE(pData, nPos, nOffset, OMX_U32);

	RPC_SETFIELDVALUE(pData, nPos, hComp, OMX_HANDLETYPE);
	RPC_SETFIELDVALUE(pData, nPos, nPortIndex, OMX_U32);
	RPC_SETFIELDVALUE(pData, nPos, pAppPrivate, OMX_PTR);
	RPC_SETFIELDVALUE(pData, nPos, nSizeBytes, OMX_U32);

	RPC_sendPacket_sync(hCtx, pPacket, nPacketSize, nFxnIdx, pRetPacket,
	    nSize);

	*eCompReturn = (OMX_ERRORTYPE) (((struct omx_packet *) pRetPacket)->result);

	if (*eCompReturn == OMX_ErrorNone)
	{
		pRetData = ((struct omx_packet *) pRetPacket)->data;
		RPC_GETFIELDVALUE(pRetData, nPos, *pBufHeaderRemote, OMX_U32);
		//save platform private before overwriting
		pPlatformPrivate = (*ppBufferHdr)->pPlatformPrivate;
		//RPC_GETFIELDCOPYTYPE(pData, nPos, pBufferHdr, OMX_BUFFERHEADERTYPE);
		/*Copying each field of the header separately due to padding issues in
		   the buffer header structure */
		RPC_GETFIELDVALUE(pRetData, nPos, pBufferHdr->nSize, OMX_U32);
		RPC_GETFIELDVALUE(pRetData, nPos, pBufferHdr->nVersion,
		    OMX_VERSIONTYPE);
		RPC_GETFIELDVALUE(pRetData, nPos, pBufferHdr->pBuffer,
		    OMX_U8 *);
		RPC_GETFIELDVALUE(pRetData, nPos, pBufferHdr->nAllocLen,
		    OMX_U32);
		RPC_GETFIELDVALUE(pRetData, nPos, pBufferHdr->nFilledLen,
		    OMX_U32);
		RPC_GETFIELDVALUE(pRetData, nPos, pBufferHdr->nOffset,
		    OMX_U32);
		RPC_GETFIELDVALUE(pRetData, nPos, pBufferHdr->pAppPrivate,
		    OMX_PTR);
		RPC_GETFIELDVALUE(pRetData, nPos,
		    pBufferHdr->pPlatformPrivate, OMX_PTR);
		RPC_GETFIELDVALUE(pRetData, nPos,
		    pBufferHdr->pInputPortPrivate, OMX_PTR);
		RPC_GETFIELDVALUE(pRetData, nPos,
		    pBufferHdr->pOutputPortPrivate, OMX_PTR);
		RPC_GETFIELDVALUE(pRetData, nPos,
		    pBufferHdr->hMarkTargetComponent, OMX_HANDLETYPE);
		RPC_GETFIELDVALUE(pRetData, nPos, pBufferHdr->pMarkData,
		    OMX_PTR);
		RPC_GETFIELDVALUE(pRetData, nPos, pBufferHdr->nTickCount,
		    OMX_U32);
		RPC_GETFIELDVALUE(pRetData, nPos, pBufferHdr->nTimeStamp,
		    OMX_TICKS);
		RPC_GETFIELDVALUE(pRetData, nPos, pBufferHdr->nFlags,
		    OMX_U32);
		RPC_GETFIELDVALUE(pRetData, nPos,
		    pBufferHdr->nInputPortIndex, OMX_U32);
		RPC_GETFIELDVALUE(pRetData, nPos,
		    pBufferHdr->nOutputPortIndex, OMX_U32);

		(*ppBufferHdr)->pPlatformPrivate = pPlatformPrivate;

#ifdef TILER_BUFF
		DOMX_DEBUG(" Copying plat pvt. ");
		//if (offset != 0)
		{
			RPC_GETFIELDCOPYTYPE(pRetData, nPos,
			    (OMX_TI_PLATFORMPRIVATE
				*) ((*ppBufferHdr)->pPlatformPrivate),
			    OMX_TI_PLATFORMPRIVATE);
			DOMX_DEBUG("Done copying plat pvt., aux buf = 0x%x",
			    ((OMX_TI_PLATFORMPRIVATE
				    *) ((*ppBufferHdr)->pPlatformPrivate))->
			    pAuxBuf1);
		}
#endif

	} else
	{
		//DOMX_DEBUG("OMX Error received: 0x%x",
		//  pRPCMsg->msgHeader.nOMXReturn);
	}

      EXIT:
	if (pPacket)
		RPC_freePacket(pPacket);
	if (pRetPacket && *eCompReturn != OMX_ErrorHardware)
		RPC_freePacket(pRetPacket);

	DOMX_EXIT("");
	return eRPCError;
}

/* ===========================================================================*/
/**
 * @name RPC_UseBuffer()
 * @brief Remote invocation stub for OMX_AllcateBuffer()
 * @param hComp: This is the handle on the Remote core, the proxy will replace
                 it's handle with actual OMX Component handle that recides on the specified core
 * @param ppBufferHdr:
 * @param nPortIndex:
 * @param pAppPrivate:
 * @param eCompReturn: This is return value that will be supplied by Proxy to the caller.
 *                    This is actual return value returned by the Remote component
 * @return RPC_OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
RPC_OMX_ERRORTYPE RPC_UseBuffer(OMX_HANDLETYPE hRPCCtx,
    OMX_INOUT OMX_BUFFERHEADERTYPE ** ppBufferHdr, OMX_U32 nPortIndex,
    OMX_PTR pAppPrivate, OMX_U32 nSizeBytes, OMX_U8 * pBuffer,
    OMX_U32 * pBufHeaderRemote, OMX_ERRORTYPE * eCompReturn)
{

	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	TIMM_OSAL_ERRORTYPE eError = TIMM_OSAL_ERR_NONE;
	TIMM_OSAL_PTR pPacket = NULL, pRetPacket = NULL, pData =
	    NULL, pRetData = NULL;
	OMX_U32 nPacketSize = RPC_PACKET_SIZE;
	RPC_OMX_FXN_IDX_TYPE nFxnIdx;
	OMX_U32 nPos = 0, nSize = 0, nOffset = 0;
	OMX_S32 status = 0;
	RPC_OMX_CONTEXT *hCtx = hRPCCtx;
	OMX_HANDLETYPE hComp = hCtx->hRemoteHandle;
	OMX_TI_PLATFORMPRIVATE *pPlatformPrivate = NULL;
	OMX_BUFFERHEADERTYPE *pBufferHdr = *ppBufferHdr;
	struct omx_packet *pOmxPacket = NULL;
	RPC_OMX_MAP_INFO_TYPE eMapInfo = RPC_OMX_MAP_INFO_NONE;
	OMX_PTR pMetaDataBuffer = NULL;
	OMX_U32 a =32;

	DOMX_ENTER("");

	nFxnIdx = RPC_OMX_FXN_IDX_USE_BUFFER;
	RPC_getPacket(nPacketSize, pPacket);
	RPC_initPacket(pPacket, pOmxPacket, pData, nFxnIdx, nPacketSize);

	DOMX_DEBUG("Marshaling data");
	/*Buffer mapping required */
	eMapInfo = RPC_OMX_MAP_INFO_ONE_BUF;
	if (((OMX_TI_PLATFORMPRIVATE *) pBufferHdr->pPlatformPrivate)->
	    pAuxBuf1 != NULL)
		eMapInfo = RPC_OMX_MAP_INFO_TWO_BUF;
	if (((OMX_TI_PLATFORMPRIVATE *)pBufferHdr->pPlatformPrivate)->
	    pMetaDataBuffer != NULL)
		eMapInfo = RPC_OMX_MAP_INFO_THREE_BUF;

	/*Offset is the location of the buffer pointer from the start of the data packet */
	nOffset =
	    sizeof(RPC_OMX_MAP_INFO_TYPE) + sizeof(OMX_U32) +
	    sizeof(OMX_HANDLETYPE) + sizeof(OMX_U32) + sizeof(OMX_PTR) +
	    sizeof(OMX_U32);
	RPC_SETFIELDVALUE(pData, nPos, eMapInfo, RPC_OMX_MAP_INFO_TYPE);
	RPC_SETFIELDVALUE(pData, nPos, nOffset, OMX_U32);

	RPC_SETFIELDVALUE(pData, nPos, hComp, OMX_HANDLETYPE);
	RPC_SETFIELDVALUE(pData, nPos, nPortIndex, OMX_U32);
	RPC_SETFIELDVALUE(pData, nPos, pAppPrivate, OMX_PTR);
	RPC_SETFIELDVALUE(pData, nPos, nSizeBytes, OMX_U32);

	RPC_SETFIELDVALUE(pData, nPos, pBuffer, OMX_U32);
	DOMX_DEBUG("eMapInfo = %x",eMapInfo);
	if (eMapInfo >= RPC_OMX_MAP_INFO_TWO_BUF)
	{
		RPC_SETFIELDVALUE(pData, nPos,
		    ((OMX_TI_PLATFORMPRIVATE *) pBufferHdr->
			pPlatformPrivate)->pAuxBuf1, OMX_U32);
		DOMX_DEBUG("UV buffer fd= %d",((OMX_TI_PLATFORMPRIVATE *)pBufferHdr->pPlatformPrivate)->pAuxBuf1);
	}

	if (eMapInfo >= RPC_OMX_MAP_INFO_THREE_BUF)
	{
		RPC_SETFIELDVALUE(pData, nPos,
		    ((OMX_TI_PLATFORMPRIVATE *) pBufferHdr->
			pPlatformPrivate)->pMetaDataBuffer, OMX_U32);
		DOMX_DEBUG("Metadata buffer = %d",((OMX_TI_PLATFORMPRIVATE *)pBufferHdr->pPlatformPrivate)->pMetaDataBuffer);
	}

	DOMX_DEBUG("About to send packet");
	RPC_sendPacket_sync(hCtx, pPacket, nPacketSize, nFxnIdx, pRetPacket,
	    nSize);
	DOMX_DEBUG("Remote call returned");

	*eCompReturn = (OMX_ERRORTYPE) (((struct omx_packet *) pRetPacket)->result);

	if (*eCompReturn == OMX_ErrorNone)
	{
		pRetData = ((struct omx_packet *) pRetPacket)->data;
		RPC_GETFIELDVALUE(pRetData, nPos, *pBufHeaderRemote, OMX_U32);
		//save platform private before overwriting
		pPlatformPrivate = (*ppBufferHdr)->pPlatformPrivate;

		/*Copying each field of the header separately due to padding issues in
		   the buffer header structure */
		RPC_GETFIELDVALUE(pRetData, nPos, pBufferHdr->nSize, OMX_U32);
		RPC_GETFIELDVALUE(pRetData, nPos, pBufferHdr->nVersion,
		    OMX_VERSIONTYPE);
		/*
		   Do not expect buffer pointer - local buffer pointer is already
		   present in the local header.
		   RPC_GETFIELDVALUE(pRetData, nPos, pBufferHdr->pBuffer,
		   OMX_U8 *);
		 */
		RPC_GETFIELDVALUE(pRetData, nPos, pBufferHdr->nAllocLen,
		    OMX_U32);
		RPC_GETFIELDVALUE(pRetData, nPos, pBufferHdr->nFilledLen,
		    OMX_U32);
		RPC_GETFIELDVALUE(pRetData, nPos, pBufferHdr->nOffset,
		    OMX_U32);
		RPC_GETFIELDVALUE(pRetData, nPos, pBufferHdr->pAppPrivate,
		    OMX_PTR);
		//Do not expect PlatformPrivate from Ducati
		//RPC_GETFIELDVALUE(pRetData, nPos,pBufferHdr->pPlatformPrivate, OMX_PTR);
		RPC_GETFIELDVALUE(pRetData, nPos,
		    pBufferHdr->pInputPortPrivate, OMX_PTR);
		RPC_GETFIELDVALUE(pRetData, nPos,
		    pBufferHdr->pOutputPortPrivate, OMX_PTR);
		RPC_GETFIELDVALUE(pRetData, nPos,
		    pBufferHdr->hMarkTargetComponent, OMX_HANDLETYPE);
		RPC_GETFIELDVALUE(pRetData, nPos, pBufferHdr->pMarkData,
		    OMX_PTR);
		RPC_GETFIELDVALUE(pRetData, nPos, pBufferHdr->nTickCount,
		    OMX_U32);
		RPC_GETFIELDVALUE(pRetData, nPos, pBufferHdr->nTimeStamp,
		    OMX_TICKS);
		RPC_GETFIELDVALUE(pRetData, nPos, pBufferHdr->nFlags,
		    OMX_U32);
		RPC_GETFIELDVALUE(pRetData, nPos,
		    pBufferHdr->nInputPortIndex, OMX_U32);
		RPC_GETFIELDVALUE(pRetData, nPos,
		    pBufferHdr->nOutputPortIndex, OMX_U32);

		//Do not expect PlatformPrivate from Ducati
		/*
		(*ppBufferHdr)->pPlatformPrivate = pPlatformPrivate;

		DOMX_DEBUG(" Copying plat pvt. ");
		RPC_GETFIELDCOPYTYPE(pRetData, nPos,
		    (OMX_TI_PLATFORMPRIVATE *) ((*ppBufferHdr)->
			pPlatformPrivate), OMX_TI_PLATFORMPRIVATE);*/

		/*Resetting size as it might be diff on Ducati due to padding issues */
		pBufferHdr->nSize = sizeof(OMX_BUFFERHEADERTYPE);
	}

      EXIT:
	if (pPacket)
		RPC_freePacket(pPacket);
	if (pRetPacket && *eCompReturn != OMX_ErrorHardware)
		RPC_freePacket(pRetPacket);

	DOMX_EXIT("");
	return eRPCError;
}

/* ===========================================================================*/
/**
 * @name RPC_FreeBuffer()
 * @brief Remote invocation stub for OMX_AllcateBuffer()
 * @param size   : Size of the incoming RCM message (parameter used in the RCM alloc call)
 * @param *data  : Pointer to the RCM message/buffer received
 * @return RPC_OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
RPC_OMX_ERRORTYPE RPC_FreeBuffer(OMX_HANDLETYPE hRPCCtx,
    OMX_IN OMX_U32 nPortIndex, OMX_IN OMX_U32 BufHdrRemote, OMX_U32 pBuffer,
    OMX_ERRORTYPE * eCompReturn)
{
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	TIMM_OSAL_ERRORTYPE eError = TIMM_OSAL_ERR_NONE;
	TIMM_OSAL_PTR pPacket = NULL, pRetPacket = NULL, pData = NULL;
	OMX_U32 nPacketSize = RPC_PACKET_SIZE;
	RPC_OMX_FXN_IDX_TYPE nFxnIdx;
	OMX_U32 nPos = 0, nSize = 0, nOffset = 0;
	OMX_S32 status = 0;
	RPC_OMX_CONTEXT *hCtx = hRPCCtx;
	OMX_HANDLETYPE hComp = hCtx->hRemoteHandle;
	struct omx_packet *pOmxPacket = NULL;

	DOMX_ENTER("");

	nFxnIdx = RPC_OMX_FXN_IDX_FREE_BUFFER;
	RPC_getPacket(nPacketSize, pPacket);
	RPC_initPacket(pPacket, pOmxPacket, pData, nFxnIdx, nPacketSize);

	/*Offset is the location of the buffer pointer from the start of the data packet */
	nOffset =  sizeof(RPC_OMX_MAP_INFO_TYPE) + sizeof(OMX_U32) +
                   sizeof(OMX_HANDLETYPE) + sizeof(OMX_U32) + sizeof(OMX_U32);
	/*No buffer mapping required */
	RPC_SETFIELDVALUE(pData, nPos, RPC_OMX_MAP_INFO_ONE_BUF,
	    RPC_OMX_MAP_INFO_TYPE);
	RPC_SETFIELDVALUE(pData, nPos, nOffset, OMX_U32);

	RPC_SETFIELDVALUE(pData, nPos, hComp, OMX_HANDLETYPE);
	RPC_SETFIELDVALUE(pData, nPos, nPortIndex, OMX_U32);
	RPC_SETFIELDVALUE(pData, nPos, BufHdrRemote, OMX_U32);
	/* Buffer is being sent separately only to catch NULL buffer errors
	   in PA mode */
	RPC_SETFIELDVALUE(pData, nPos, pBuffer, OMX_U32);

	RPC_sendPacket_sync(hCtx, pPacket, nPacketSize, nFxnIdx, pRetPacket,
	    nSize);

	*eCompReturn = (OMX_ERRORTYPE) (((struct omx_packet *) pRetPacket)->result);

      EXIT:
	if (pPacket)
		RPC_freePacket(pPacket);
	if (pRetPacket && *eCompReturn != OMX_ErrorHardware)
		RPC_freePacket(pRetPacket);

	DOMX_EXIT("");
	return eRPCError;
}


/* ===========================================================================*/
/**
 * @name RPC_EmptyThisBuffer()
 * @brief
 * @param size   : Size of the incoming RCM message (parameter used in the RCM alloc call)
 * @param *data  : Pointer to the RCM message/buffer received
 * @return RPC_OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/

RPC_OMX_ERRORTYPE RPC_EmptyThisBuffer(OMX_HANDLETYPE hRPCCtx,
    OMX_BUFFERHEADERTYPE * pBufferHdr, OMX_U32 BufHdrRemote,
    OMX_ERRORTYPE * eCompReturn, OMX_BOOL bMapBuffer)
{
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	TIMM_OSAL_ERRORTYPE eError = TIMM_OSAL_ERR_NONE;
	OMX_U32 nPacketSize = RPC_PACKET_SIZE;
	RPC_OMX_FXN_IDX_TYPE nFxnIdx;
	OMX_U32 nPos = 0, nSize = 0, nOffset = 0;
	OMX_S32 status = 0;
	RPC_OMX_CONTEXT *hCtx = hRPCCtx;
	OMX_HANDLETYPE hComp = hCtx->hRemoteHandle;
	OMX_U8 *pAuxBuf1 = NULL;
	struct omx_packet *pOmxPacket = NULL;
	RPC_OMX_MAP_INFO_TYPE eMapInfo = RPC_OMX_MAP_INFO_NONE;
#ifdef RPC_SYNC_MODE
	TIMM_OSAL_PTR pPacket = NULL, pRetPacket = NULL, pData = NULL;
#endif

	DOMX_ENTER("");

	nFxnIdx = RPC_OMX_FXN_IDX_EMPTYTHISBUFFER;
	RPC_getPacket(nPacketSize, pPacket);
	RPC_initPacket(pPacket, pOmxPacket, pData, nFxnIdx, nPacketSize);

	if(bMapBuffer == OMX_TRUE)
	{
		pAuxBuf1 = ((OMX_TI_PLATFORMPRIVATE *) pBufferHdr->pPlatformPrivate)->pAuxBuf1;
		/*Buffer mapping required */
		if (((OMX_TI_PLATFORMPRIVATE *) pBufferHdr->pPlatformPrivate)->pAuxBuf1 == NULL)
			eMapInfo = RPC_OMX_MAP_INFO_ONE_BUF;
		else
			eMapInfo = RPC_OMX_MAP_INFO_TWO_BUF;
		/*Offset is the location of the buffer pointer from the start of the data packet */
		nOffset =
	    		sizeof(RPC_OMX_MAP_INFO_TYPE) + sizeof(OMX_U32) +
	   		sizeof(OMX_HANDLETYPE) + sizeof(OMX_BUFFERHEADERTYPE *) + 3*sizeof(OMX_U32) +
			sizeof(OMX_TICKS) + sizeof(OMX_HANDLETYPE) + sizeof(OMX_PTR) + 3*sizeof(OMX_U32);
	}

	RPC_SETFIELDVALUE(pData, nPos, eMapInfo, RPC_OMX_MAP_INFO_TYPE);

	RPC_SETFIELDVALUE(pData, nPos, nOffset, OMX_U32);

	RPC_SETFIELDVALUE(pData, nPos, hComp, OMX_HANDLETYPE);
	RPC_SETFIELDVALUE(pData, nPos,
	    (OMX_BUFFERHEADERTYPE *) BufHdrRemote, OMX_BUFFERHEADERTYPE *);

	RPC_SETFIELDVALUE(pData, nPos, pBufferHdr->nFilledLen, OMX_U32);
	RPC_SETFIELDVALUE(pData, nPos, pBufferHdr->nOffset, OMX_U32);
	RPC_SETFIELDVALUE(pData, nPos, pBufferHdr->nFlags, OMX_U32);
	RPC_SETFIELDVALUE(pData, nPos, pBufferHdr->nTimeStamp, OMX_TICKS);
	RPC_SETFIELDVALUE(pData, nPos, pBufferHdr->hMarkTargetComponent,
	    OMX_HANDLETYPE);
	RPC_SETFIELDVALUE(pData, nPos, pBufferHdr->pMarkData, OMX_PTR);
	RPC_SETFIELDVALUE(pData, nPos, pBufferHdr->nAllocLen, OMX_U32);
	RPC_SETFIELDVALUE(pData, nPos, pBufferHdr->nOutputPortIndex, OMX_U32);
	RPC_SETFIELDVALUE(pData, nPos, pBufferHdr->nInputPortIndex, OMX_U32);

	if(bMapBuffer == OMX_TRUE)
	{
		RPC_SETFIELDVALUE(pData, nPos, pBufferHdr->pBuffer, OMX_U32);
		if (eMapInfo == RPC_OMX_MAP_INFO_TWO_BUF)
		{
			RPC_SETFIELDVALUE(pData, nPos,
			    ((OMX_TI_PLATFORMPRIVATE *) pBufferHdr->pPlatformPrivate)->pAuxBuf1, OMX_U32);
		}
	}

	DOMX_DEBUG(" pBufferHdr = %x BufHdrRemote %x", pBufferHdr,
	    BufHdrRemote);

#ifdef RPC_SYNC_MODE
	RPC_sendPacket_sync(hCtx, pPacket, nPacketSize, nFxnIdx, pRetPacket,
	    nSize);

	*eCompReturn = (OMX_ERRORTYPE) (((struct omx_packet *) pRetPacket)->result);
#else
	RPC_sendPacket_async(hCtx->ClientHndl[RCM_DEFAULT_CLIENT], pPacket,
	    nFxnIdx);
	RPC_checkAsyncErrors(hCtx->ClientHndl[RCM_DEFAULT_CLIENT], pPacket);

	*eCompReturn = OMX_ErrorNone;
#endif

      EXIT:
	if (pPacket)
		RPC_freePacket(pPacket);
	if (pRetPacket && *eCompReturn != OMX_ErrorHardware)
		RPC_freePacket(pRetPacket);

	DOMX_EXIT("");
	return eRPCError;
}


/* ===========================================================================*/
/**
 * @name RPC_FillThisBuffer()
 * @brief Remote invocation stub for OMX_AllcateBuffer()
 * @param size   : Size of the incoming RCM message (parameter used in the RCM alloc call)
 * @param *data  : Pointer to the RCM message/buffer received
 * @return RPC_OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
RPC_OMX_ERRORTYPE RPC_FillThisBuffer(OMX_HANDLETYPE hRPCCtx,
    OMX_BUFFERHEADERTYPE * pBufferHdr, OMX_U32 BufHdrRemote,
    OMX_ERRORTYPE * eCompReturn)
{
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	TIMM_OSAL_ERRORTYPE eError = TIMM_OSAL_ERR_NONE;
	OMX_U32 nPacketSize = RPC_PACKET_SIZE;
	RPC_OMX_FXN_IDX_TYPE nFxnIdx;
	OMX_U32 nPos = 0, nSize = 0, nOffset = 0;
	OMX_S32 status = 0;
	RPC_OMX_CONTEXT *hCtx = hRPCCtx;
	OMX_HANDLETYPE hComp = hCtx->hRemoteHandle;
	OMX_U8 *pAuxBuf1 = NULL;
	struct omx_packet *pOmxPacket = NULL;
#ifdef RPC_SYNC_MODE
	TIMM_OSAL_PTR pPacket = NULL, pRetPacket = NULL, pData = NULL;
#endif

	DOMX_ENTER("");

	nFxnIdx = RPC_OMX_FXN_IDX_FILLTHISBUFFER;
	RPC_getPacket(nPacketSize, pPacket);
	RPC_initPacket(pPacket, pOmxPacket, pData, nFxnIdx, nPacketSize);

	/*No buffer mapping required */
	RPC_SETFIELDVALUE(pData, nPos, RPC_OMX_MAP_INFO_NONE,
	    RPC_OMX_MAP_INFO_TYPE);
	RPC_SETFIELDVALUE(pData, nPos, nOffset, OMX_U32);

	RPC_SETFIELDVALUE(pData, nPos, hComp, OMX_HANDLETYPE);
	RPC_SETFIELDVALUE(pData, nPos,
	    (OMX_BUFFERHEADERTYPE *) BufHdrRemote, OMX_BUFFERHEADERTYPE *);

	RPC_SETFIELDVALUE(pData, nPos, pBufferHdr->nFilledLen, OMX_U32);
	RPC_SETFIELDVALUE(pData, nPos, pBufferHdr->nOffset, OMX_U32);
	RPC_SETFIELDVALUE(pData, nPos, pBufferHdr->nFlags, OMX_U32);
	RPC_SETFIELDVALUE(pData, nPos, pBufferHdr->nAllocLen, OMX_U32);
	RPC_SETFIELDVALUE(pData, nPos, pBufferHdr->nOutputPortIndex, OMX_U32);
	RPC_SETFIELDVALUE(pData, nPos, pBufferHdr->nInputPortIndex, OMX_U32);

	DOMX_DEBUG(" pBufferHdr = %x BufHdrRemote %x", pBufferHdr,
	    BufHdrRemote);

#ifdef RPC_SYNC_MODE
	RPC_sendPacket_sync(hCtx, pPacket, nPacketSize, nFxnIdx, pRetPacket,
	    nSize);

	*eCompReturn = (OMX_ERRORTYPE) (((struct omx_packet *) pRetPacket)->result);

#else
	RPC_sendPacket_async(hCtx->ClientHndl[RCM_DEFAULT_CLIENT], pPacket,
	    nFxnIdx);
	RPC_checkAsyncErrors(hCtx->ClientHndl[RCM_DEFAULT_CLIENT], pPacket);

	*eCompReturn = OMX_ErrorNone;
#endif

      EXIT:
	if (pPacket)
		RPC_freePacket(pPacket);
	if (pRetPacket && *eCompReturn != OMX_ErrorHardware)
		RPC_freePacket(pRetPacket);

	DOMX_EXIT("");
	return eRPCError;
}


/* ***************************** EMPTY APIs ******************************** */

/* ===========================================================================*/
/**
 * @name EMPTY-STUB
 * @brief
 * @param
 * @return
 *
 */
/* ===========================================================================*/
OMX_ERRORTYPE RPC_EventHandler(OMX_HANDLETYPE hRPCCtx, OMX_PTR pAppData,
    OMX_EVENTTYPE eEvent, OMX_U32 nData1, OMX_U32 nData2, OMX_PTR pEventData)
{
	return RPC_OMX_ErrorNone;
}

OMX_ERRORTYPE RPC_EmptyBufferDone(OMX_HANDLETYPE hRPCCtx, OMX_PTR pAppData,
    OMX_BUFFERHEADERTYPE * pBuffer)
{
	return RPC_OMX_ErrorNone;
}

OMX_ERRORTYPE RPC_FillBufferDone(OMX_HANDLETYPE hRPCCtx, OMX_PTR pAppData,
    OMX_BUFFERHEADERTYPE * pBuffer)
{
	return RPC_OMX_ErrorNone;
}

RPC_OMX_ERRORTYPE RPC_ComponentTunnelRequest(OMX_HANDLETYPE hRPCCtx,
    OMX_IN OMX_U32 nPort, OMX_HANDLETYPE hTunneledRemoteHandle,
    OMX_U32 nTunneledPort, OMX_INOUT OMX_TUNNELSETUPTYPE * pTunnelSetup,
    OMX_ERRORTYPE * eCompReturn)
{
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	TIMM_OSAL_ERRORTYPE eError = TIMM_OSAL_ERR_NONE;
	OMX_U32 nPacketSize = RPC_PACKET_SIZE;
	RPC_OMX_CONTEXT *hCtx = hRPCCtx;
	OMX_HANDLETYPE hComp = hCtx->hRemoteHandle;
       RPC_OMX_CONTEXT   *hTunneledCtx    = hTunneledRemoteHandle;
        OMX_HANDLETYPE     hTunneledComp   = hTunneledCtx->hRemoteHandle;
	RPC_OMX_FXN_IDX_TYPE nFxnIdx;
	struct omx_packet *pOmxPacket = NULL;
	OMX_U32 nPos = 0, nSize = 0, nOffset = 0;
	OMX_S32 status = 0;
#ifdef RPC_SYNC_MODE
	TIMM_OSAL_PTR pPacket = NULL, pRetPacket = NULL, pData = NULL;
#endif

        printf(" Entering rpc:domx_stub.c:ComponentTunnelRequest\n");

	nFxnIdx = RPC_OMX_FXN_IDX_COMP_TUNNEL_REQUEST;
	RPC_getPacket(nPacketSize, pPacket);
	RPC_initPacket(pPacket, pOmxPacket, pData, nFxnIdx, nPacketSize);

        /*Pack the values into a packet*/
        //Marshalled:[>ParentComp|>ParentPort|>TunnelComp|>TunneledPort>TunnelSetup]
	RPC_SETFIELDVALUE(pData, nPos, RPC_OMX_MAP_INFO_NONE, RPC_OMX_MAP_INFO_TYPE);
        RPC_SETFIELDVALUE(pData, nPos, hComp, OMX_HANDLETYPE);
        RPC_SETFIELDVALUE(pData, nPos, nPort, OMX_U32);
        RPC_SETFIELDVALUE(pData, nPos, hTunneledComp, OMX_HANDLETYPE);
        RPC_SETFIELDVALUE(pData, nPos, nTunneledPort, OMX_U32);
        printf("\n after RPC_sendPacket_sync");
	RPC_sendPacket_sync(hCtx, pPacket, nPacketSize, nFxnIdx, pRetPacket,
	    nSize);

        printf("\n after RPC_sendPacket_sync: *eCompReturn : 0x%x\n", (OMX_ERRORTYPE) (((struct omx_packet *) pRetPacket)->result));
	*eCompReturn = (OMX_ERRORTYPE) (((struct omx_packet *) pRetPacket)->result);

      EXIT:
	if (pPacket)
		RPC_freePacket(pPacket);
	if (pRetPacket)
		RPC_freePacket(pRetPacket);

	DOMX_EXIT("");
	return eRPCError;

}
