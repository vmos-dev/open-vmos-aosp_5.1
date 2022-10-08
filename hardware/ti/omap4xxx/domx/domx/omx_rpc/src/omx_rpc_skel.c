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
 *  @file  omx_rpc_skel.c
 *         This file contains methods that provides the functionality for
 *         the OpenMAX1.1 DOMX Framework RPC Skeleton implementations.
 *
 *  @path \WTSD_DucatiMMSW\framework\domx\omx_rpc\src
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

/******************************************************************
 *   INCLUDE FILES
 ******************************************************************/

#include <stdio.h>
#include <string.h>

#include <timm_osal_interfaces.h>
#include <OMX_TI_Common.h>

#include "omx_rpc.h"
#include "omx_proxy_common.h"
#include "omx_rpc_skel.h"
#include "omx_rpc_utils.h"
#include "omx_rpc_internal.h"

/* FUNCTION LIST*/
/*
RPC_OMX_ERRORTYPE RPC_SKEL_EmptyBufferDone(uint32_t size, uint32_t *data)
RPC_OMX_ERRORTYPE RPC_SKEL_FillBufferDone(uint32_t size, uint32_t *data)
RPC_OMX_ERRORTYPE RPC_SKEL_EventHandler(uint32_t size, uint32_t *data)
*/


/* ===========================================================================*/
/**
 * @name RPC_SKEL_FillBufferDone()
 * @brief Skeleton for OMX_AllocateBuffer()
 * @param size   : Size of the incoming RCM message (parameter used in the RCM alloc call)
 * @param *data  : Pointer to the RCM message/buffer received
 * @return RPC_OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
RPC_OMX_ERRORTYPE RPC_SKEL_EmptyBufferDone(void *data)
{

	OMX_HANDLETYPE hComp;
	RPC_OMX_ERRORTYPE tRPCError = RPC_OMX_ErrorNone;
	OMX_U32 nfilledLen = 0;
	OMX_U32 nOffset = 0;
	OMX_U32 nFlags = 0;
	OMX_U32 bufferHdr;
	OMX_COMPONENTTYPE *pHandle = NULL;
	PROXY_COMPONENT_PRIVATE *pCompPrv = NULL;
	OMX_U32 nPos = 0;
	//RPC_OMX_MESSAGE *recdMsg;
	OMX_U8 *pMsgBody = NULL;
	//recdMsg = (RPC_OMX_MESSAGE *) (data);

	pMsgBody = data;	//&recdMsg->msgBody[0];

	DOMX_ENTER("");

	//Marshalled:[>hComp|>bufferHdr|>nFilledLen|>nOffset|>nFlags]

	RPC_GETFIELDVALUE(pMsgBody, nPos, hComp, OMX_HANDLETYPE);
	pHandle = (OMX_COMPONENTTYPE *) hComp;
	pCompPrv = (PROXY_COMPONENT_PRIVATE *) pHandle->pComponentPrivate;

	RPC_GETFIELDVALUE(pMsgBody, nPos, bufferHdr, OMX_U32);

	RPC_GETFIELDVALUE(pMsgBody, nPos, nfilledLen, OMX_U32);
	RPC_GETFIELDVALUE(pMsgBody, nPos, nOffset, OMX_U32);
	RPC_GETFIELDVALUE(pMsgBody, nPos, nFlags, OMX_U32);

	tRPCError =
	    pCompPrv->proxyEmptyBufferDone(hComp, bufferHdr, nfilledLen,
	    nOffset, nFlags);

	DOMX_EXIT("");
	return tRPCError;
}

/* ===========================================================================*/
/**
 * @name RPC_SKEL_FillBufferDone()
 * @brief Skeleton for OMX_AllocateBuffer()
 * @param size   : Size of the incoming RCM message (parameter used in the RCM alloc call)
 * @param *data  : Pointer to the RCM message/buffer received
 * @return RPC_OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
RPC_OMX_ERRORTYPE RPC_SKEL_FillBufferDone(void *data)
{

	OMX_HANDLETYPE hComp;
	RPC_OMX_ERRORTYPE tRPCError = RPC_OMX_ErrorNone;
	OMX_U32 nfilledLen = 0;
	OMX_U32 nOffset = 0;
	OMX_U32 nFlags = 0;
	OMX_TICKS nTimeStamp;
	OMX_U32 bufferHdr;
	OMX_COMPONENTTYPE *pHandle = NULL;
	PROXY_COMPONENT_PRIVATE *pCompPrv = NULL;
	OMX_U32 nPos = 0;
	//RPC_OMX_MESSAGE *recdMsg;
	OMX_U8 *pMsgBody = NULL;
	OMX_HANDLETYPE hMarkTargetComponent = NULL;
	OMX_PTR pMarkData = NULL;
	//recdMsg = (RPC_OMX_MESSAGE *) (data);

	pMsgBody = data;	//&recdMsg->msgBody[0];

	DOMX_ENTER("");

	//Marshalled:[>hComp|>bufferHdr|>nFilledLen|>nOffset|>nFlags|>nTimeStamp]

	RPC_GETFIELDVALUE(pMsgBody, nPos, hComp, OMX_HANDLETYPE);
	pHandle = (OMX_COMPONENTTYPE *) hComp;
	pCompPrv = (PROXY_COMPONENT_PRIVATE *) pHandle->pComponentPrivate;

	RPC_GETFIELDVALUE(pMsgBody, nPos, bufferHdr, OMX_U32);
	RPC_GETFIELDVALUE(pMsgBody, nPos, nfilledLen, OMX_U32);
	RPC_GETFIELDVALUE(pMsgBody, nPos, nOffset, OMX_U32);
	RPC_GETFIELDVALUE(pMsgBody, nPos, nFlags, OMX_U32);
	RPC_GETFIELDVALUE(pMsgBody, nPos, nTimeStamp, OMX_TICKS);
	RPC_GETFIELDVALUE(pMsgBody, nPos, hMarkTargetComponent,
	    OMX_HANDLETYPE);
	RPC_GETFIELDVALUE(pMsgBody, nPos, pMarkData, OMX_PTR);

	tRPCError =
	    pCompPrv->proxyFillBufferDone(hComp, bufferHdr, nfilledLen,
	    nOffset, nFlags, nTimeStamp, hMarkTargetComponent, pMarkData);

	DOMX_EXIT("");
	return tRPCError;
}



/* ===========================================================================*/
/**
 * @name RPC_SKEL_EventHandler()
 * @brief Skeleton for OMX_AllocateBuffer()
 * @param size   : Size of the incoming RCM message (parameter used in the RCM alloc call)
 * @param *data  : Pointer to the RCM message/buffer received
 * @return RPC_OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
RPC_OMX_ERRORTYPE RPC_SKEL_EventHandler(void *data)
{
	OMX_HANDLETYPE hComp;
	RPC_OMX_ERRORTYPE tRPCError = RPC_OMX_ErrorNone;
	OMX_COMPONENTTYPE *pHandle = NULL;
	PROXY_COMPONENT_PRIVATE *pCompPrv = NULL;
	OMX_EVENTTYPE eEvent;
	OMX_U32 nData1;
	OMX_U32 nData2;
	OMX_PTR pEventData;
	OMX_U32 nPos = 0;
	//RPC_OMX_MESSAGE *recdMsg;
	OMX_U8 *pMsgBody = NULL;
	//recdMsg = (RPC_OMX_MESSAGE *) (data);
	pMsgBody = data;	//&recdMsg->msgBody[0];

	DOMX_ENTER("");

	//Marshalled:[>hComp|>eEvent|>nData1|>nData2|>pEventData]

	RPC_GETFIELDVALUE(pMsgBody, nPos, hComp, OMX_HANDLETYPE);
	pHandle = (OMX_COMPONENTTYPE *) hComp;
	pCompPrv = (PROXY_COMPONENT_PRIVATE *) pHandle->pComponentPrivate;

	RPC_GETFIELDVALUE(pMsgBody, nPos, eEvent, OMX_EVENTTYPE);
	RPC_GETFIELDVALUE(pMsgBody, nPos, nData1, OMX_U32);
	RPC_GETFIELDVALUE(pMsgBody, nPos, nData2, OMX_U32);
	RPC_GETFIELDVALUE(pMsgBody, nPos, pEventData, OMX_PTR);

	DOMX_DEBUG
	    ("\n**__**Got eventhandler from Remote Core succesffuly - values are : **__**");
	DOMX_DEBUG
	    ("hComp :0x%x,eEvent: 0x%x,nData1: 0x%x,nData2: 0x%x,pEventData: 0x%x",
	    hComp, eEvent, nData1, nData2, pEventData);
	DOMX_DEBUG("Just Before Entering Proxy Event handler call");

	tRPCError =
	    pCompPrv->proxyEventHandler(hComp, pCompPrv->pILAppData, eEvent,
	    nData1, nData2, pEventData);

	DOMX_EXIT("");
	return tRPCError;

}

/* ===========================================================================*/
/**
 * @name EMPTY-SKEL
 * @brief
 * @param
 * @return
 *
 */
/* ===========================================================================*/
RPC_OMX_ERRORTYPE RPC_SKEL_GetHandle(uint32_t size, uint32_t * data)
{
	return RPC_OMX_ErrorNone;
}

RPC_OMX_ERRORTYPE RPC_SKEL_SetParameter(uint32_t size, uint32_t * data)
{
	return RPC_OMX_ErrorNone;
}

RPC_OMX_ERRORTYPE RPC_SKEL_GetParameter(uint32_t size, uint32_t * data)
{
	return RPC_OMX_ErrorNone;
}

RPC_OMX_ERRORTYPE RPC_SKEL_FreeHandle(uint32_t size, uint32_t * data)
{
	return RPC_OMX_ErrorNone;
}

RPC_OMX_ERRORTYPE RPC_SKEL_EmptyThisBuffer(uint32_t size, uint32_t * data)
{
	return RPC_OMX_ErrorNone;
}

RPC_OMX_ERRORTYPE RPC_SKEL_FillThisBuffer(uint32_t size, uint32_t * data)
{
	return RPC_OMX_ErrorNone;
}

RPC_OMX_ERRORTYPE RPC_SKEL_UseBuffer(uint32_t size, uint32_t * data)
{
	return RPC_OMX_ErrorNone;
}

RPC_OMX_ERRORTYPE RPC_SKEL_FreeBuffer(uint32_t size, uint32_t * data)
{
	return RPC_OMX_ErrorNone;
}

RPC_OMX_ERRORTYPE RPC_SKEL_SetConfig(uint32_t size, uint32_t * data)
{
	return RPC_OMX_ErrorNone;
}

RPC_OMX_ERRORTYPE RPC_SKEL_GetConfig(uint32_t size, uint32_t * data)
{
	return RPC_OMX_ErrorNone;
}

RPC_OMX_ERRORTYPE RPC_SKEL_GetState(uint32_t size, uint32_t * data)
{
	return RPC_OMX_ErrorNone;
}

RPC_OMX_ERRORTYPE RPC_SKEL_SendCommand(uint32_t size, uint32_t * data)
{
	return RPC_OMX_ErrorNone;
}

RPC_OMX_ERRORTYPE RPC_SKEL_GetComponentVersion(uint32_t size, uint32_t * data)
{
	return RPC_OMX_ErrorNone;
}

RPC_OMX_ERRORTYPE RPC_SKEL_GetExtensionIndex(uint32_t size, uint32_t * data)
{
	return RPC_OMX_ErrorNone;
}

RPC_OMX_ERRORTYPE RPC_SKEL_AllocateBuffer(uint32_t size, uint32_t * data)
{
	return RPC_OMX_ErrorNone;
}

RPC_OMX_ERRORTYPE RPC_SKEL_ComponentTunnelRequest(uint32_t size,
    uint32_t * data)
{
	return RPC_OMX_ErrorNone;
}
