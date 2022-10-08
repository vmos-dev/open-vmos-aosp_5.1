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
 *  @file  omx_rpc_internal.h
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


#ifndef OMXRPC_INTERNAL_H
#define OMXRPC_INTERNAL_H

#ifdef __cplusplus
extern "C"
{
#endif				/* __cplusplus */

/******************************************************************
 *   INCLUDE FILES
 ******************************************************************/
/* ----- system and platform files ----------------------------*/
#include <pthread.h>

#include <OMX_Component.h>
#include <OMX_Core.h>
#include <OMX_Audio.h>
#include <OMX_Video.h>
#include <OMX_Types.h>
#include <OMX_Index.h>
#include <OMX_TI_Index.h>
#include <OMX_TI_Common.h>

/*-------program files ----------------------------------------*/
#include "omx_rpc.h"


/******************************************************************
 *   DEFINES - CONSTANTS
 ******************************************************************/
/* *********************** OMX RPC DEFINES***********************************/

/*This defines the maximum number of remote functions that can be registered*/
#define RPC_OMX_MAX_FUNCTION_LIST 21
/*Packet size for each message*/
#define RPC_PACKET_SIZE 0x12C



/*******************************************************************************
* Enumerated Types
*******************************************************************************/

	typedef enum OMX_RPC_CORE_TYPE
	{
		OMX_RPC_CORE_TESLA = 0,
		OMX_RPC_CORE_APPM3 = 1,
		OMX_RPC_CORE_SYSM3 = 2,
		RPC_CORE_CHIRON = 3,
		OMX_RPC_CORE_MAX = 4
	} OMX_RPC_CORE_TYPE;



/*===============================================================*/
/** RPC_OMX_FXN_IDX_TYPE   : A unique function index for each OMX function
 */
/*===============================================================*/
	typedef enum RPC_OMX_FXN_IDX_TYPE
	{

		RPC_OMX_FXN_IDX_GET_HANDLE = 0,
		RPC_OMX_FXN_IDX_SET_PARAMETER = 1,
		RPC_OMX_FXN_IDX_GET_PARAMETER = 2,
		RPC_OMX_FXN_IDX_USE_BUFFER = 3,
		RPC_OMX_FXN_IDX_FREE_HANDLE = 4,
		RPC_OMX_FXN_IDX_SET_CONFIG = 5,
		RPC_OMX_FXN_IDX_GET_CONFIG = 6,
		RPC_OMX_FXN_IDX_GET_STATE = 7,
		RPC_OMX_FXN_IDX_SEND_CMD = 8,
		RPC_OMX_FXN_IDX_GET_VERSION = 9,
		RPC_OMX_FXN_IDX_GET_EXT_INDEX = 10,
		RPC_OMX_FXN_IDX_FILLTHISBUFFER = 11,
		RPC_OMX_FXN_IDX_FILLBUFFERDONE = 12,
		RPC_OMX_FXN_IDX_FREE_BUFFER = 13,
		RPC_OMX_FXN_IDX_EMPTYTHISBUFFER = 14,
		RPC_OMX_FXN_IDX_EMPTYBUFFERDONE = 15,
		RPC_OMX_FXN_IDX_EVENTHANDLER = 16,
		RPC_OMX_FXN_IDX_ALLOCATE_BUFFER = 17,
		RPC_OMX_FXN_IDX_COMP_TUNNEL_REQUEST = 18,
		RPC_OMX_FXN_IDX_MAX = RPC_OMX_MAX_FUNCTION_LIST
	} RPC_OMX_FXN_IDX_TYPE;



/*===============================================================*/
/** RPC_OMX_MAP_INFO_TYPE   : Tells the no. of buffers that the kernel has to
                              map to remote core.
 */
/*===============================================================*/
	typedef enum RPC_OMX_MAP_INFO_TYPE
	{
		RPC_OMX_MAP_INFO_NONE = 0,
		RPC_OMX_MAP_INFO_ONE_BUF = 1,
		RPC_OMX_MAP_INFO_TWO_BUF = 2,
		RPC_OMX_MAP_INFO_THREE_BUF = 3,
		RPC_OMX_MAP_INFO_MAX = 0x7FFFFFFF
	} RPC_OMX_MAP_INFO_TYPE;



/*******************************************************************************
* STRUCTURES
*******************************************************************************/

/*===============================================================*/
/** RPC_OMX_CONTEXT                 : RPC context structure
 *
 *  @ param fd_omx                  : File descriptor corresponding to this
 *                                    instance on remote core.
 *  @ param fd_killcb               : File descriptor used to shut down the
 *                                    callback thread.
 *  @ param cbThread                : Callback thread.
 *  @ param pMsgPipe                : Array of message pipes. One for each OMX
 *                                    function. Used to post and receive the
 *                                    return messages of each function.
 *  @ param hRemoteHandle           : Handle to the context structure on the
 *                                    remote core.
 *  @ param hActualRemoteCompHandle : Actual component handle on remote core.
 *  @ param pAppData                : App data of RPC caller
 *
 */
/*===============================================================*/
	typedef struct RPC_OMX_CONTEXT
	{
		OMX_S32 fd_omx;
		OMX_S32 fd_killcb;
		pthread_t cbThread;
		OMX_PTR pMsgPipe[RPC_OMX_MAX_FUNCTION_LIST];
		OMX_HANDLETYPE hRemoteHandle;
		OMX_HANDLETYPE hActualRemoteCompHandle;
		OMX_PTR pAppData;
	} RPC_OMX_CONTEXT;

#ifdef __cplusplus
}
#endif
#endif
