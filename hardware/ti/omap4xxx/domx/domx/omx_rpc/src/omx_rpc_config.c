#if 0

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
 *  @file  omx_rpc_config.c
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
 /* ----- system and platform files ---------------------------- */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <Std.h>

#include <OMX_Types.h>
#include <timm_osal_interfaces.h>
#include <timm_osal_trace.h>

#include <MultiProc.h>
#include <RcmClient.h>
#include <RcmServer.h>

/*-------program files ----------------------------------------*/
#include "omx_rpc.h"
#include "omx_rpc_stub.h"
#include "omx_rpc_skel.h"
#include "omx_rpc_internal.h"
#include "omx_rpc_utils.h"

extern Int32 RPC_MemFree(UInt32 * dataSize, UInt32 * data);
extern Int32 RPC_MemAlloc(UInt32 * dataSize, UInt32 * data);

/* contains configurations or structures to be passed to omx_rpc layer */
char rpcFxns[][MAX_FUNCTION_NAME_LENGTH] = {
	"RPC_SKEL_SetParameter",
	"RPC_SKEL_GetParameter",
	"RPC_SKEL_GetHandle",
	"RPC_SKEL_UseBuffer",

	"RPC_SKEL_FreeHandle",

	"RPC_SKEL_SetConfig",
	"RPC_SKEL_GetConfig",
	"RPC_SKEL_GetState",
	"RPC_SKEL_SendCommand",
	"RPC_SKEL_GetComponentVersion",
	"RPC_SKEL_GetExtensionIndex",
	"RPC_SKEL_FillThisBuffer",
	"RPC_SKEL_FillBufferDone",
	"RPC_SKEL_FreeBuffer",

	"RPC_SKEL_EmptyThisBuffer",
	"RPC_SKEL_EmptyBufferDone",
	"RPC_SKEL_EventHandler",
	"RPC_SKEL_AllocateBuffer",
	"RPC_SKEL_ComponentTunnelRequest",

	"MemMgr_Alloc",
	"MemMgr_Free"
};

rpcSkelArr rpcSkelFxns[] = {
	{RPC_SKEL_SetParameter},
	{RPC_SKEL_GetParameter},
	{RPC_SKEL_GetHandle},
	{RPC_SKEL_UseBuffer},
	{RPC_SKEL_FreeHandle},
	{RPC_SKEL_SetConfig},
	{RPC_SKEL_GetConfig},
	{RPC_SKEL_GetState},
	{RPC_SKEL_SendCommand},
	{RPC_SKEL_GetComponentVersion},
	{RPC_SKEL_GetExtensionIndex},
	{RPC_SKEL_FillThisBuffer},
	{RPC_SKEL_FillBufferDone},
	{RPC_SKEL_FreeBuffer},
	{RPC_SKEL_EmptyThisBuffer},
	{RPC_SKEL_EmptyBufferDone},
	{RPC_SKEL_EventHandler},
	{RPC_SKEL_AllocateBuffer},
	{RPC_SKEL_ComponentTunnelRequest},
	{RPC_MemAlloc},
	{RPC_MemFree}
};

#endif
