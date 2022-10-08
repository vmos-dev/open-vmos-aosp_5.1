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


/******************************************************************
 *   INCLUDE FILES
 ******************************************************************/
/* ----- system and platform files ----------------------------*/
#include <errno.h>
//#include <errno-base.h>

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include <OMX_Types.h>
#include <timm_osal_interfaces.h>
#include <timm_osal_trace.h>


/*-------program files ----------------------------------------*/
#include "omx_rpc.h"
#include "omx_proxy_common.h"
#include "omx_rpc_stub.h"
#include "omx_rpc_skel.h"
#include "omx_rpc_internal.h"
#include "omx_rpc_utils.h"

#include "rpmsg_omx_defs.h"

#define RPC_MSGPIPE_SIZE (4)
#define RPC_MSG_SIZE_FOR_PIPE (sizeof(OMX_PTR))
#define MAX_ATTEMPTS 15

#define RPC_getPacket(nPacketSize, pPacket) do { \
    pPacket = TIMM_OSAL_Malloc(nPacketSize, TIMM_OSAL_TRUE, 0, TIMMOSAL_MEM_SEGMENT_INT); \
    RPC_assert(pPacket != NULL, RPC_OMX_ErrorInsufficientResources, \
           "Error Allocating RCM Message Frame"); \
    } while(0)

#define RPC_freePacket(pPacket) do { \
    if(pPacket != NULL) TIMM_OSAL_Free(pPacket); \
    } while(0)

OMX_U8 pBufferError[RPC_PACKET_SIZE];

void *RPC_CallbackThread(void *data);


/* ===========================================================================*/
/**
* @name RPC_InstanceInit()
* @brief RPC instance init is used for initialization. It is called once for
*        every instance of an OMX component.
* @param cComponentName [IN] : Name of the component.
* @param phRPCCtx [OUT] : RPC Context structure - to be filled in and returned
* @return RPC_OMX_ErrorNone = Successful
*/
/* ===========================================================================*/

RPC_OMX_ERRORTYPE RPC_InstanceInit(OMX_STRING cComponentName,
    OMX_HANDLETYPE * phRPCCtx)
{
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	RPC_OMX_CONTEXT *pRPCCtx = NULL;
	OMX_S32 status = 0;
	struct omx_conn_req sReq = { .name = "OMX" };
	TIMM_OSAL_ERRORTYPE eError = TIMM_OSAL_ERR_NONE;
	OMX_U32 i = 0, nAttempts = 0;

	*(RPC_OMX_CONTEXT **) phRPCCtx = NULL;

	//pthread_t cbThread;

//	sReq.name = "OMX";

	/*pRPCCtx contains context information for each instance */
	pRPCCtx =
	    (RPC_OMX_CONTEXT *) TIMM_OSAL_Malloc(sizeof(RPC_OMX_CONTEXT),
	    TIMM_OSAL_TRUE, 0, TIMMOSAL_MEM_SEGMENT_INT);
	RPC_assert(pRPCCtx != NULL, RPC_OMX_ErrorInsufficientResources,
	    "Malloc failed");
	TIMM_OSAL_Memset(pRPCCtx, 0, sizeof(RPC_OMX_CONTEXT));

	/*Assuming that open maintains an internal count for multi instance */
	DOMX_DEBUG("Calling open on the device");
	while (1)
	{
		pRPCCtx->fd_omx = open("/dev/rpmsg-omx1", O_RDWR);
		if(pRPCCtx->fd_omx >= 0 || errno != ENOENT || nAttempts == MAX_ATTEMPTS)
			break;
		DOMX_DEBUG("errno from open= %d, REATTEMPTING OPEN!!!!",errno);
		nAttempts++;
		usleep(1000000);
	}
	if(pRPCCtx->fd_omx < 0)
	{
		DOMX_ERROR("Can't open device, errorno from open = %d",errno);
		eError = RPC_OMX_ErrorInsufficientResources;
		goto EXIT;
	}
	DOMX_DEBUG("Open was successful, pRPCCtx->fd_omx = %d",
	    pRPCCtx->fd_omx);
//AD
//      strncpy(sReq.name, cComponentName, OMX_MAX_STRINGNAME_SIZE);

	DOMX_DEBUG("Calling ioctl");
	status = ioctl(pRPCCtx->fd_omx, OMX_IOCCONNECT, &sReq);
	RPC_assert(status >= 0, RPC_OMX_ErrorInsufficientResources,
	    "Can't connect");

	for (i = 0; i < RPC_OMX_MAX_FUNCTION_LIST; i++)
	{
		eError =
		    TIMM_OSAL_CreatePipe(&(pRPCCtx->pMsgPipe[i]),
		    RPC_MSGPIPE_SIZE, RPC_MSG_SIZE_FOR_PIPE, 1);
		RPC_assert(eError == TIMM_OSAL_ERR_NONE,
		    RPC_OMX_ErrorInsufficientResources,
		    "Pipe creation failed");
	}

	DOMX_DEBUG("Creating event fd");
	pRPCCtx->fd_killcb = eventfd(0, 0);
	RPC_assert(pRPCCtx->fd_killcb >= 0,
	    RPC_OMX_ErrorInsufficientResources, "Can't create kill fd");
	/*Create a listener/server thread to listen for Ducati callbacks */
	DOMX_DEBUG("Create listener thread");
	status =
	    pthread_create(&(pRPCCtx->cbThread), NULL, RPC_CallbackThread,
	    pRPCCtx);
	RPC_assert(status == 0, RPC_OMX_ErrorInsufficientResources,
	    "Can't create cb thread");

      EXIT:
	if (eRPCError != RPC_OMX_ErrorNone)
	{
		RPC_InstanceDeInit(pRPCCtx);
	}
	else
	{
		*(RPC_OMX_CONTEXT **) phRPCCtx = pRPCCtx;
	}
	return eRPCError;
}



/* ===========================================================================*/
/**
* @name RPC_InstanceDeInit()
* @brief RPC instance deinit is used for tear down. It is called once when an
*        instance of OMX component is going down.
* @param phRPCCtx [IN] : RPC Context structure.
* @return RPC_OMX_ErrorNone = Successful
*/
/* ===========================================================================*/
RPC_OMX_ERRORTYPE RPC_InstanceDeInit(OMX_HANDLETYPE hRPCCtx)
{
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	TIMM_OSAL_ERRORTYPE eError = TIMM_OSAL_ERR_NONE;
	RPC_OMX_CONTEXT *pRPCCtx = (RPC_OMX_CONTEXT *) hRPCCtx;
	OMX_S32 status = 0;
	OMX_U32 i = 0;
	OMX_U64 nKillEvent = 1;

	RPC_assert(hRPCCtx != NULL, RPC_OMX_ErrorUndefined,
	    "NULL context handle supplied to RPC Deinit");

	if (pRPCCtx->fd_killcb)
	{
		status =
		    write(pRPCCtx->fd_killcb, &nKillEvent, sizeof(OMX_U64));
		if (status <= 0)
		{
			DOMX_ERROR
			    ("Write to kill fd failed - cb thread may not exit");
			eRPCError = RPC_OMX_ErrorUndefined;
		} else if (pRPCCtx->cbThread)
		{
			DOMX_DEBUG("Waiting for cb thread to exit");
			status = pthread_join(pRPCCtx->cbThread, NULL);
			if (status != 0)
			{
				DOMX_ERROR("Join for cb thread failed");
				eRPCError = RPC_OMX_ErrorUndefined;
			}
		}
		DOMX_DEBUG("Closing the kill fd");
		status = close(pRPCCtx->fd_killcb);
		pRPCCtx->fd_killcb = 0;
		if (status != 0)
		{
			DOMX_ERROR("Close failed on kill fd");
			eRPCError = RPC_OMX_ErrorUndefined;
		}
	}

	for (i = 0; i < RPC_OMX_MAX_FUNCTION_LIST; i++)
	{
		if (pRPCCtx->pMsgPipe[i])
		{
			eError = TIMM_OSAL_DeletePipe(pRPCCtx->pMsgPipe[i]);
			pRPCCtx->pMsgPipe[i] = NULL;
			if (eError != TIMM_OSAL_ERR_NONE)
			{
				DOMX_ERROR("Pipe deletion failed");
				eRPCError = RPC_OMX_ErrorUndefined;
			}
		}
	}

	DOMX_DEBUG("Closing the omx fd");
	if (pRPCCtx->fd_omx)
	{
		status = close(pRPCCtx->fd_omx);
		pRPCCtx->fd_omx = 0;
		if (status != 0)
		{
			DOMX_ERROR("Close failed on omx fd");
			eRPCError = RPC_OMX_ErrorUndefined;
		}
	}

	TIMM_OSAL_Free(pRPCCtx);

	EXIT:
		return eRPCError;
}



/* ===========================================================================*/
/**
* @name RPC_CallbackThread()
* @brief This is the entry function of the thread which keeps spinning, waiting
*        for messages from Ducati.
* @param data [IN] : The RPC Context structure is passed here.
* @return RPC_OMX_ErrorNone = Successful
*/
/* ===========================================================================*/
void *RPC_CallbackThread(void *data)
{
	OMX_PTR pBuffer = NULL;
	RPC_OMX_CONTEXT *pRPCCtx = (RPC_OMX_CONTEXT *) data;
	fd_set readfds;
	OMX_S32 maxfd = 0, status = 0;
	OMX_U32 nFxnIdx = 0, nPacketSize = RPC_PACKET_SIZE, nPos = 0;
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	TIMM_OSAL_ERRORTYPE eError = TIMM_OSAL_ERR_NONE;
	OMX_COMPONENTTYPE *hComp = NULL;
	PROXY_COMPONENT_PRIVATE *pCompPrv = NULL;
        OMX_PTR pBuff = pBufferError;

	maxfd =
	    (pRPCCtx->fd_killcb >
	    pRPCCtx->fd_omx ? pRPCCtx->fd_killcb : pRPCCtx->fd_omx) + 1;
	while (1)
	{
		FD_ZERO(&readfds);
		FD_SET(pRPCCtx->fd_omx, &readfds);
		FD_SET(pRPCCtx->fd_killcb, &readfds);

		DOMX_DEBUG("Waiting for messages from remote core");
		status = select(maxfd, &readfds, NULL, NULL, NULL);
		RPC_assert(status > 0, RPC_OMX_ErrorUndefined,
		    "select failed");

		if (FD_ISSET(pRPCCtx->fd_killcb, &readfds))
		{
			DOMX_DEBUG("Recd. kill message - exiting the thread");
			break;
		}

		if (FD_ISSET(pRPCCtx->fd_omx, &readfds))
		{
			DOMX_DEBUG("Recd. omx message");
			RPC_getPacket(nPacketSize, pBuffer);
			status = read(pRPCCtx->fd_omx, pBuffer, nPacketSize);
            if(status < 0)
            {
                if(errno == ENXIO)
                {
		    for(nFxnIdx = 0; nFxnIdx < RPC_OMX_MAX_FUNCTION_LIST; nFxnIdx++)
		    {
			((struct omx_packet *) pBufferError)->result = OMX_ErrorHardware;
			TIMM_OSAL_WriteToPipe(pRPCCtx->pMsgPipe[nFxnIdx], &pBuff, RPC_MSG_SIZE_FOR_PIPE, TIMM_OSAL_SUSPEND);
			if(eError != TIMM_OSAL_ERR_NONE)
				DOMX_ERROR("Write to pipe failed");
		    }
                    /*Indicate fatal error and exit*/
                    RPC_assert(0, RPC_OMX_ErrorHardware,
                    "Remote processor fatal error");
                }
                else
                {
                    RPC_assert(0, RPC_OMX_ErrorUndefined,
                    "read failed");
                }
            }

			nPos = 0;
			nFxnIdx = ((struct omx_packet *) pBuffer)->fxn_idx;
			/*Indices from static table will have bit 31 set */
			if (nFxnIdx & 0x80000000)
				nFxnIdx &= 0x0FFFFFFF;
			RPC_assert(nFxnIdx < RPC_OMX_MAX_FUNCTION_LIST,
			    RPC_OMX_ErrorUndefined,
			    "Bad function index recd");
			switch (nFxnIdx)
			{
			case RPC_OMX_FXN_IDX_EVENTHANDLER:
				RPC_SKEL_EventHandler(((struct omx_packet *)
					pBuffer)->data);
				RPC_freePacket(pBuffer);
				pBuffer = NULL;
				break;
			case RPC_OMX_FXN_IDX_EMPTYBUFFERDONE:
				RPC_SKEL_EmptyBufferDone(((struct omx_packet *)
					pBuffer)->data);
				RPC_freePacket(pBuffer);
				pBuffer = NULL;
				break;
			case RPC_OMX_FXN_IDX_FILLBUFFERDONE:
				RPC_SKEL_FillBufferDone(((struct omx_packet *)
					pBuffer)->data);
				RPC_freePacket(pBuffer);
				pBuffer = NULL;
				break;
			default:
				eError =
				    TIMM_OSAL_WriteToPipe(pRPCCtx->
				    pMsgPipe[nFxnIdx], &pBuffer,
				    RPC_MSG_SIZE_FOR_PIPE, TIMM_OSAL_SUSPEND);
				RPC_assert(eError == TIMM_OSAL_ERR_NONE,
				    RPC_OMX_ErrorUndefined,
				    "Write to pipe failed");
				break;
			}
		}
EXIT:
		if (eRPCError != RPC_OMX_ErrorNone)
		{
			//AD TODO: Send error CB to client and then go back in loop to wait for killfd
			if (pBuffer != NULL)
			{
				RPC_freePacket(pBuffer);
				pBuffer = NULL;
			}
			/*Report all hardware errors as fatal and exit from listener thread*/
			if (eRPCError == RPC_OMX_ErrorHardware)
			{
				/*Implicit detail: pAppData is proxy component handle updated during
                  RPC_GetHandle*/
				hComp = (OMX_COMPONENTTYPE *) pRPCCtx->pAppData;
                if(hComp != NULL)
				{
					pCompPrv = (PROXY_COMPONENT_PRIVATE *) hComp->pComponentPrivate;
                    /*Indicate fatal error. Users are expected to cleanup the OMX instance
                    to ensure all resources are cleaned up.*/
					pCompPrv->proxyEventHandler(hComp, pCompPrv->pILAppData, OMX_EventError,
												OMX_ErrorHardware, 0, NULL);
				}
				break;
			}
		}
	}
        return (void*)0;
}
