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
 *  @file  omx_rpc_skel.h
 *         This file contains methods that provides the functionality for
 *         the OpenMAX1.1 DOMX Framework RPC.
 *
 *  @path \WTSD_DucatiMMSW\framework\domx\omx_rpc\inc
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


#ifndef OMX_RPC_SKELH
#define OMX_RPC_SKELH

#ifdef __cplusplus
extern "C"
{
#endif				/* __cplusplus */

/******************************************************************
 *   INCLUDE FILES
 ******************************************************************/

#include "../inc/omx_rpc_internal.h"
#include "../inc/omx_rpc_stub.h"

	RPC_OMX_ERRORTYPE RPC_SKEL_EmptyBufferDone(void *data);
	RPC_OMX_ERRORTYPE RPC_SKEL_FillBufferDone(void *data);
	RPC_OMX_ERRORTYPE RPC_SKEL_EventHandler(void *data);

/*Empty SKEL*/
	RPC_OMX_ERRORTYPE RPC_SKEL_GetHandle(uint32_t size, uint32_t * data);
	RPC_OMX_ERRORTYPE RPC_SKEL_SetParameter(uint32_t size,
	    uint32_t * data);
	RPC_OMX_ERRORTYPE RPC_SKEL_GetParameter(uint32_t size,
	    uint32_t * data);
	RPC_OMX_ERRORTYPE RPC_SKEL_FreeHandle(uint32_t size, uint32_t * data);
	RPC_OMX_ERRORTYPE RPC_SKEL_EmptyThisBuffer(uint32_t size,
	    uint32_t * data);
	RPC_OMX_ERRORTYPE RPC_SKEL_FillThisBuffer(uint32_t size,
	    uint32_t * data);
	RPC_OMX_ERRORTYPE RPC_SKEL_UseBuffer(uint32_t size, uint32_t * data);
	RPC_OMX_ERRORTYPE RPC_SKEL_FreeBuffer(uint32_t size, uint32_t * data);
	RPC_OMX_ERRORTYPE RPC_SKEL_SetConfig(uint32_t size, uint32_t * data);
	RPC_OMX_ERRORTYPE RPC_SKEL_GetConfig(uint32_t size, uint32_t * data);
	RPC_OMX_ERRORTYPE RPC_SKEL_GetState(uint32_t size, uint32_t * data);
	RPC_OMX_ERRORTYPE RPC_SKEL_SendCommand(uint32_t size,
	    uint32_t * data);
	RPC_OMX_ERRORTYPE RPC_SKEL_GetComponentVersion(uint32_t size,
	    uint32_t * data);
	RPC_OMX_ERRORTYPE RPC_SKEL_GetExtensionIndex(uint32_t size,
	    uint32_t * data);
	RPC_OMX_ERRORTYPE RPC_SKEL_AllocateBuffer(uint32_t size,
	    uint32_t * data);
	RPC_OMX_ERRORTYPE RPC_SKEL_ComponentTunnelRequest(uint32_t size,
	    uint32_t * data);

#ifdef __cplusplus
}
#endif
#endif
