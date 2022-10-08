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
 *  @file  omx_rpc_utils.c
 *         This file contains methods that provides the functionality for
 *         the OpenMAX1.1 DOMX Framework RPC.
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
#include <string.h>
#include <stdio.h>

#include "omx_rpc.h"
#include "omx_rpc_internal.h"
#include "omx_rpc_utils.h"
#include <MultiProc.h>
#include <ProcMgr.h>

extern char rcmservertable[MAX_PROC][MAX_SERVER_NAME_LENGTH];
extern char Core_Array[MAX_PROC][MAX_CORENAME_LENGTH];

/* ===========================================================================*/
/**
 * @name EMPTY-STUB
 * @brief
 * @param
 * @return
 *
 */
/* ===========================================================================*/
RPC_OMX_ERRORTYPE RPC_UTIL_GetTargetServerName(OMX_STRING ComponentName,
    OMX_STRING ServerName)
{
	OMX_U8 count = 0;
	OMX_U8 i = 0;
	OMX_U8 servertable_idx = 0;
	OMX_STRING str;
	char Core[MAX_CORENAME_LENGTH];

	DOMX_ENTER("");
	DOMX_DEBUG(" Calling Component Name %s", ComponentName);

	while (*ComponentName != '\0')
	{
		if (*ComponentName == '.')
		{
			count++;
			if (count == 2)
			{
				ComponentName++;
				str = ComponentName;

				while (*str != '.')
				{
					Core[i] = *str;
					i++;
					str++;
				}
				Core[i] = '\0';
				break;
			}

		}

		ComponentName++;
	}

	DOMX_DEBUG(" CORE NAME RECOVERED: %s", Core);
	DOMX_DEBUG
	    ("____________________________________________________________");
	DOMX_DEBUG("Recovering Server Table Index");
	for (i = 0; i < CORE_MAX; i++)
	{

		if (strcmp(Core, Core_Array[i]) == 0)
		{
			servertable_idx = i;
			DOMX_DEBUG("Recovered Server table index = %d", i);
			break;
		}
	}
	strncpy(ServerName, (OMX_STRING) rcmservertable[servertable_idx],
	    MAX_SERVER_NAME_LENGTH);
	DOMX_DEBUG(" ServerName recovered = %s", ServerName);

	return RPC_OMX_ErrorNone;
}

/* ===========================================================================*/
/**
 * @name EMPTY-STUB
 * @brief
 * @param
 * @return
 *
 */
/* ===========================================================================*/
RPC_OMX_ERRORTYPE RPC_UTIL_GetLocalServerName(OMX_STRING ComponentName,
    OMX_STRING * ServerName)
{
/* Implementation returns only current core ID - But this is a place holder to abstract out the
default server and other additional servers available on the current core. This additional servers
should be available in the RPC global that is indexed using the calling component name*/
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	OMX_U8 servertable_idx = 0;

	servertable_idx = MultiProc_getId(NULL);	//This can be replace with the mechanism to obtain new addition rcm servers
	if (MultiProc_INVALIDID != servertable_idx)
		*ServerName = rcmservertable[servertable_idx];
	else
		eRPCError = RPC_OMX_ErrorUndefined;

	return eRPCError;
}



/* ===========================================================================*/
/**
 * @name RPC_UTIL_GenerateLocalServerName()
 * @brief This function generates a server name to be used while creating the
 *        RCM server. The name is based on the pid so that eacj process gets a
 *        unique server name.
 * @param cServerName - This is filled in and returned with a valid value.
 * @return RPC_OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
RPC_OMX_ERRORTYPE RPC_UTIL_GenerateLocalServerName(OMX_STRING cServerName)
{
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	OMX_S32 pid = 0;
	OMX_U32 nProcId = 0;

	pid = getpid();
	/*Using pid as the server name, thus limiting to only 1 server per process.
	   This limitaion is partly enforce by syslink as well since max size of server
	   name is 32 so only pid can fit into the name */
	sprintf(cServerName, "%ld", pid);

	nProcId = MultiProc_getId(NULL);
	if (MultiProc_INVALIDID != nProcId)
		/*Fill the server table with the newly generated name */
		strncpy(rcmservertable[nProcId], cServerName,
		    MAX_SERVER_NAME_LENGTH);
	else
		eRPCError = RPC_OMX_ErrorUndefined;

	return eRPCError;
}



/* ===========================================================================*/
/**
 * @name RPC_UTIL_GetTargetCore()
 * @brief This function gets the target core id by parsing the component name.
 *        It is assumed that component names follow the convention
 *        <OMX>.<Company Name>.<Core Name>.<Domain>.<Component Details> with
 *        all characters in upper case for e.g. OMX.TI.DUCATI1.VIDEO.H264E
 * @param cComponentName - Name of the component.
 * @param nCoreId - Core ID, this is filled in and returned.
 * @return RPC_OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
RPC_OMX_ERRORTYPE RPC_UTIL_GetTargetCore(OMX_STRING cComponentName,
    OMX_U32 * nCoreId)
{
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	OMX_S8 cCoreName[MAX_SERVER_NAME_LENGTH] = { 0 };
	OMX_S32 ret = 0;
	OMX_U32 i = 0;

	ret =
	    sscanf(cComponentName, "%*[^'.'].%*[^'.'].%[^'.'].%*s",
	    cCoreName);
	RPC_assert(ret == 1, RPC_OMX_ErrorBadParameter,
	    "Incorrect component name");

	for (i = 0; i < CORE_MAX; i++)
	{
		if (strcmp((char *)cCoreName, Core_Array[i]) == 0)
			break;
	}
	RPC_assert(i < CORE_MAX, RPC_OMX_ErrorBadParameter,
	    "Unknown core name");
	*nCoreId = i;

      EXIT:
	return eRPCError;
}



/* ===========================================================================*/
/**
 * @name EMPTY-STUB
 * @brief  the Api creates the RCM client on the target core(where the component sits).
This happens in the context of the Default RCM server on the target core.
The RCM server name to connect for this client will be the default RCM server on the calling core.
This can be provided as an option as the name that you pass in the string server is used as the RCM server name
input to the client create call.
@Default_RcmServer - The default rcm server on the target core
@Server - The name of the server on the calling core to connect to
 * @param
 * @return
 *
 */
/* ===========================================================================*/
/*
RPC_OMX_ERRORTYPE RPC_GetTargetClient(OMX_STRING Default_RcmServer, OMX_STRING server, rcmhHndl)
{


}
*/

/* ===========================================================================*/
/**
 * @name EMPTY-STUB
 * @brief
 * @param
 * @return
 *
 */
/* ===========================================================================*/
RPC_OMX_ERRORTYPE RPC_MapBuffer(OMX_U32 mappedBuffer)
{
	DOMX_ENTER("");
	DOMX_DEBUG("Empty implementation ");
	//PlaceHolder for Mapping Buffers - Cuurently no implementation here
	return RPC_OMX_ErrorNone;
}

/* ===========================================================================*/
/**
 * @name EMPTY-STUB
 * @brief
 * @param
 * @return
 *
 */
/* ===========================================================================*/
RPC_OMX_ERRORTYPE RPC_UnMapBuffer(OMX_U32 mappedBuffer)
{
	DOMX_ENTER("");
	DOMX_DEBUG("Empty implementation ");
	//PlaceHolder for UnMapping Buffers - Cuurently no implementation here
	return RPC_OMX_ErrorNone;
}

/* ===========================================================================*/
/**
 * @name RPC_FlushBuffer
 * @brief Used to flush buffers from cache to memory. Used when buffers are
 *        being transferred across processor boundaries.
 * @param pBuffer       : Pointer to the data that has to be flushed.
 *        size          : Size of the data to be flushed.
 *        nTargetCoreId : Core to which buffer is being transferred.
 * @return RPC_OMX_ErrorNone      : Success.
 *         RPC_OMX_ErrorUndefined : Flush operation failed.
 */
/* ===========================================================================*/
RPC_OMX_ERRORTYPE RPC_FlushBuffer(OMX_U8 * pBuffer, OMX_U32 size,
    OMX_U32 nTargetCoreId)
{
	DOMX_ENTER("");
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	OMX_S32 nStatus = 0;

	DOMX_DEBUG("About to flush %d bytes", size);
	nStatus = ProcMgr_flushMemory((OMX_PTR) pBuffer, size,
	    (ProcMgr_ProcId) nTargetCoreId);
	RPC_assert(nStatus >= 0, RPC_OMX_ErrorUndefined,
	    "Cache flush failed");

      EXIT:
	return eRPCError;
}



/* ===========================================================================*/
/**
 * @name RPC_InvalidateBuffer
 * @brief Used to flush buffers from cache to memory. Used when buffers are
 *        being transferred across processor boundaries.
 * @param pBuffer       : Pointer to the data that has to be flushed.
 *        size          : Size of the data to be flushed.
 *        nTargetCoreId : Core to which buffer is being transferred.
 * @return RPC_OMX_ErrorNone      : Success.
 *         RPC_OMX_ErrorUndefined : Invalidate operation failed.
 */
/* ===========================================================================*/
RPC_OMX_ERRORTYPE RPC_InvalidateBuffer(OMX_U8 * pBuffer, OMX_U32 size,
    OMX_U32 nTargetCoreId)
{
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	OMX_S32 nStatus = 0;
	DOMX_ENTER("");

	DOMX_DEBUG("About to invalidate %d bytes", size);
	nStatus = ProcMgr_invalidateMemory((OMX_PTR) pBuffer, size,
	    (ProcMgr_ProcId) nTargetCoreId);
	RPC_assert(nStatus >= 0, RPC_OMX_ErrorUndefined,
	    "Cache invalidate failed");

      EXIT:
	return eRPCError;
}



/* ===========================================================================*/
/**
 * @name RPC_Util_AcquireJobId
 * @brief Used to get a new job id from syslink - all messages sent with the
 *        same job id will be processed serially. Messages with different job id
 *        will be processed parallely.
 * @param hRPCCtx       : RPC context handle.
 *        pJobId        : Pointer to job id - this'll be filled in and returned.
 * @return RPC_OMX_ErrorNone      : Success.
 *         Any other              : Operation failed.
 */
/* ===========================================================================*/
RPC_OMX_ERRORTYPE RPC_Util_AcquireJobId(RPC_OMX_CONTEXT * hRPCCtx,
    OMX_U16 * pJobId)
{
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	OMX_S32 nStatus = 0;
	DOMX_ENTER("");

	nStatus =
	    RcmClient_acquireJobId(hRPCCtx->ClientHndl[RCM_DEFAULT_CLIENT],
	    pJobId);
	RPC_assert(nStatus >= 0, RPC_OMX_ErrorUndefined,
	    "Acquire job id failed");

      EXIT:
	DOMX_EXIT("eRPCError = %d", eRPCError);
	return eRPCError;
}



/* ===========================================================================*/
/**
 * @name RPC_Util_ReleaseJobId
 * @brief Used to release job id to syslink. Once release, this job id cannot be
 *        reused.
 * @param hRPCCtx       : RPC context handle.
 *        nJobId        : Job ID to be released.
 * @return RPC_OMX_ErrorNone      : Success.
 *         Any other              : Operation failed.
 */
/* ===========================================================================*/
RPC_OMX_ERRORTYPE RPC_Util_ReleaseJobId(RPC_OMX_CONTEXT * hRPCCtx,
    OMX_U16 nJobId)
{
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	OMX_S32 nStatus = 0;
	DOMX_ENTER("");

	nStatus =
	    RcmClient_releaseJobId(hRPCCtx->ClientHndl[RCM_DEFAULT_CLIENT],
	    nJobId);
	RPC_assert(nStatus >= 0, RPC_OMX_ErrorUndefined,
	    "Release job id failed");

      EXIT:
	DOMX_EXIT("eRPCError = %d", eRPCError);
	return eRPCError;
}



/* ===========================================================================*/
/**
 * @name RPC_Util_GetPoolId
 * @brief Used to get pool id. Messages sent with same pool id will get
 *        processed at the same priority on remote core.
 * @param hRPCCtx       : RPC context handle.
 *        pPoolId       : Pointer to pool id - this'll be filled in and
 *                        returned.
 * @return RPC_OMX_ErrorNone      : Success.
 *         Any other              : Operation failed.
 */
/* ===========================================================================*/
RPC_OMX_ERRORTYPE RPC_Util_GetPoolId(RPC_OMX_CONTEXT * hRPCCtx,
    OMX_U16 * pPoolId)
{
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	DOMX_ENTER("");

	*pPoolId = 0x8001;

//EXIT:
	DOMX_EXIT("eRPCError = %d", eRPCError);
	return eRPCError;
}
