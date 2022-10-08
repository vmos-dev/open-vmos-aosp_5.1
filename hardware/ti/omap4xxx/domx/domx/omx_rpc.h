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
 *  @file  omx_rpc.h
 *  @brief This file contains methods that provides the functionality for
 *         the OpenMAX1.1 DOMX Framework RPC.
 *
 *  @path \WTSD_DucatiMMSW\framework\domx\omx_rpc\
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

#ifndef OMXRPC_H
#define OMXRPC_H

#ifdef __cplusplus
extern "C"
{
#endif				/* __cplusplus */

/******************************************************************
 *   INCLUDE FILES
 ******************************************************************/
#include <stdio.h>
#include <OMX_Types.h>
#include <OMX_Core.h>


/******************************************************************
 *   DATA TYPES
 ******************************************************************/


 /*******************************************************************************
* Enumerated Types
*******************************************************************************/
	typedef enum RPC_OMX_ERRORTYPE
	{
		RPC_OMX_ErrorNone = 0,

		/* OMX Error Mapped */
		RPC_OMX_ErrorInsufficientResources = 0x81000,
		RPC_OMX_ErrorUndefined = 0x81001,
		RPC_OMX_ErrorBadParameter = 0x81005,
		RPC_OMX_ErrorHardware = 0x81009,
		RPC_OMX_ErrorUnsupportedIndex = 0x8101A,
		RPC_OMX_ErrorTimeout = 0x81011,
		/* END OF OMX Error */

		/* RPC Specific Error - to depricate */
		RPC_OMX_ErrorUnknown = 0x70000,
		RPC_OMX_ErrorProccesorInit = 0x70001,
		RPC_OMX_InvalidRPCCmd = 0x70002,
		RPC_OMX_ErrorHLOS = 0x70003,
		RPC_OMX_ErrorInvalidMsg = 0x70004,

		/* RCM Specific */
		RPC_OMX_RCM_ErrorExecFail = 0x70005,
		RPC_OMX_RCM_ErrorExecDpcFail = 0x70006,
		RPC_OMX_RCM_ErrorTimeout = 0x70007,
		RPC_OMX_RCM_ServerFail = 0x70008,
		RPC_OMX_RCM_ClientFail = 0x70009,

	} RPC_OMX_ERRORTYPE;


/****************************************************************
 * PUBLIC DECLARATIONS Defined here, used elsewhere

****************************************************************/
/* ===========================================================================*/
/**
 * @name RPC_InstanceInit()
 * @brief RPC instance init is used to bring up a instance of a client - this should be ideally invokable from any core
 *        For this the parameters it would require are
 *        Heap ID - this needs to be configured at startup (CFG) and indicates the heaps available for a RCM client to pick from
 *        Server - this contains the RCM server name that the client should connect to
 *        rcmHndl - Contains the Client once the call is completed
 *        rcmParams -
 *        These values can be picked up from the RPC handle. But an unique identifier is required -Server
 * @param cComponentName  : Pointer to the Components Name that is requires the RCM client to be initialized
 * @return RPC_OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
	RPC_OMX_ERRORTYPE RPC_InstanceInit(OMX_STRING cComponentName,
	    OMX_HANDLETYPE * phRPCCtx);



/* ===========================================================================*/
/**
 * @name RPC_InstanceDeInit()
 * @brief This function Removes or deinitializes RCM client instances. This also manages the number of active users
 *        of a given RCM client
 * @param cComponentName  : Pointer to the Components Name that is active user of the RCM client to be deinitialized
 * @return RPC_OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
	RPC_OMX_ERRORTYPE RPC_InstanceDeInit(OMX_HANDLETYPE hRPCCtx);



#ifdef __cplusplus
}
#endif				/* __cplusplus */

#endif
