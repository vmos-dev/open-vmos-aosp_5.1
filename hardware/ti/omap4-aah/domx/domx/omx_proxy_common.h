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
 *  @file  omx_proxy_common.h
 *         This file contains methods that provides the functionality for
 *         the OpenMAX1.1 DOMX Framework OMX Common Proxy.
 *
 *  @path \WTSD_DucatiMMSW\framework\domx\omx_proxy_common\
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

#ifdef ANDROID_QUIRK_LOCK_BUFFER
#include <hardware/gralloc.h>
#endif

#ifndef OMX_PROXY_H
#define OMX_PROXY_H

#ifdef __cplusplus
extern "C"
{
#endif				/* __cplusplus */

/* ------compilation control switches ----------------------------------------*/

/******************************************************************
 *   INCLUDE FILES
 ******************************************************************/
/* ----- system and platform files ----------------------------*/
#include <OMX_Core.h>
/*-------program files ----------------------------------------*/
#include "omx_rpc.h"
#include "omx_rpc_internal.h"
#include "omx_rpc_utils.h"

/****************************************************************
 * PUBLIC DECLARATIONS Defined here, used elsewhere
 ****************************************************************/
/*--------data declarations -----------------------------------*/
/*OMX versions supported by DOMX*/
#define OMX_VER_MAJOR 0x1
#define OMX_VER_MINOR 0x1

#define MAX_NUM_PROXY_BUFFERS             100
#define MAX_COMPONENT_NAME_LENGTH         128
#define PROXY_MAXNUMOFPORTS               8

/******************************************************************
 *   MACROS - ASSERTS
 ******************************************************************/
#define PROXY_assert  PROXY_paramCheck
#define PROXY_require PROXY_paramCheck
#define PROXY_ensure  PROXY_paramCheck

#define PROXY_paramCheck(C, V, S) do {\
    if (!(C)) { eError = V;\
    if(S) DOMX_ERROR("failed check:" #C" - returning error: 0x%x - %s",V,S);\
    else DOMX_ERROR("failed check:" #C" - returning error: 0x%x",V); \
    goto EXIT; }\
    } while(0)

#define PROXY_CHK_VERSION(_pStruct_, _sName_) do { \
    PROXY_require((((_sName_ *)_pStruct_)->nSize == sizeof(_sName_)), \
                  OMX_ErrorBadParameter, "Incorrect nSize"); \
    PROXY_require(((((_sName_ *)_pStruct_)->nVersion.s.nVersionMajor == \
                  OMX_VER_MAJOR) && \
                  ((((_sName_ *)_pStruct_)->nVersion.s.nVersionMinor == \
                  OMX_VER_MINOR) || \
                  (((_sName_ *)_pStruct_)->nVersion.s.nVersionMinor == \
                  0))), \
                  OMX_ErrorVersionMismatch, NULL); \
    } while(0)

#define PROXY_checkRpcError() do { \
    if (eRPCError == RPC_OMX_ErrorNone) \
    { \
        DOMX_DEBUG("Corresponding RPC function executed successfully"); \
        eError = eCompReturn; \
        PROXY_assert((eError == OMX_ErrorNone) || (eError == OMX_ErrorNoMore), eError, "Error returned from OMX API in ducati"); \
    } else \
    { \
        DOMX_ERROR("RPC function returned error 0x%x", eRPCError); \
        switch (eRPCError) \
        { \
            case RPC_OMX_ErrorHardware: \
                eError = OMX_ErrorHardware; \
            break; \
            case RPC_OMX_ErrorInsufficientResources: \
                eError = OMX_ErrorInsufficientResources; \
            break; \
            case RPC_OMX_ErrorBadParameter: \
                eError = OMX_ErrorBadParameter; \
            break; \
            case RPC_OMX_ErrorUnsupportedIndex: \
                eError = OMX_ErrorUnsupportedIndex; \
            break; \
            case RPC_OMX_ErrorTimeout: \
                eError = OMX_ErrorTimeout; \
            break; \
            default: \
                eError = OMX_ErrorUndefined; \
        } \
        PROXY_assert((eError == OMX_ErrorNone), eError, "Error returned from OMX API in ducati"); \
    } \
} while(0)

	typedef OMX_ERRORTYPE(*PROXY_EMPTYBUFFER_DONE) (OMX_HANDLETYPE
	    hComponent, OMX_U32 remoteBufHdr, OMX_U32 nfilledLen,
	    OMX_U32 nOffset, OMX_U32 nFlags);

	typedef OMX_ERRORTYPE(*PROXY_FILLBUFFER_DONE) (OMX_HANDLETYPE
	    hComponent, OMX_U32 remoteBufHdr, OMX_U32 nfilledLen,
	    OMX_U32 nOffset, OMX_U32 nFlags, OMX_TICKS nTimeStamp,
	    OMX_HANDLETYPE hMarkTargetComponent, OMX_PTR pMarkData);

	typedef OMX_ERRORTYPE(*PROXY_EVENTHANDLER) (OMX_HANDLETYPE hComponent,
	    OMX_PTR pAppData, OMX_EVENTTYPE eEvent, OMX_U32 nData1,
	    OMX_U32 nData2, OMX_PTR pEventData);

/*******************************************************************************
* Structures
*******************************************************************************/
/*===============================================================*/
/** PROXY_BUFFER_INFO        : This structure maintains a table of A9 and
 *                             Ducati side buffers and headers.
 *
 * @param pBufHeader         : This is a pointer to the A9 bufferheader.
 *
 * @param pBufHeaderRemote   : This is pointer to Ducati side bufferheader.
 *
 * @param pRegisteredAufBux0
 * @param pRegisteredAufBux1
 * @param pRegisteredAufBux2 : These are pointers to buffers registered with rpc driver
 *                             They will assigned when registering and used when
 *                             unregistering the buffer
 */
/*===============================================================*/
	typedef struct PROXY_BUFFER_INFO
	{
		OMX_BUFFERHEADERTYPE *pBufHeader;
		OMX_U32 pBufHeaderRemote;
		OMX_PTR pYBuffer;
		OMX_PTR pMetaDataBuffer;
#ifdef USE_ION
		int mmap_fd;
		int mmap_fd_metadata_buff;
		OMX_PTR pRegisteredAufBux0;
		OMX_PTR pRegisteredAufBux1;
		OMX_PTR pRegisteredAufBux2;
#endif
	} PROXY_BUFFER_INFO;

/*===============================================================*/
/** PROXY_BUFFER_TYPE        : This enumeration tells the type of buffer pointers coming to OMX in 
				UseBuffer call.
 */
/*===============================================================*/
	typedef enum PROXY_BUFFER_TYPE
	{
		VirtualPointers,   /*Used when buffer pointers come from the normal A9 virtual space */
		GrallocPointers,   /*Used when buffer pointers come from Gralloc allocations */
		IONPointers,       /*Used when buffer pointers come from ION allocations */
		EncoderMetadataPointers,		/*Used when buffer pointers come from Stagefright in camcorder usecase */
		BufferDescriptorVirtual2D          /*Virtual unpacked buffers passed via OMX_TI_BUFFERDESCRIPTOR_TYPE */
	} PROXY_BUFFER_TYPE;

/*===============================================================*/
/** PROXY_BUFFER_TYPE        : This enumeration tells the type of buffer pointers coming to OMX in
                               UseBuffer call.
 */
/*===============================================================*/
	typedef struct PROXY_PORT_TYPE
	{
		PROXY_BUFFER_TYPE proxyBufferType;   /*Used when buffer pointers come from the normal A9 virtual space */
		OMX_U32 IsBuffer2D;   /*Used when buffer pointers come from Gralloc allocations */
	} PROXY_PORT_TYPE;

#ifdef ENABLE_RAW_BUFFERS_DUMP_UTILITY
/*===============================================================*/
/** DebugFrame_Dump     : Structure holding the info about frames to dump
 *  @param fromFrame: From which frame to start dumping
 *  @param toFrame:  till which frame to dump
 *  @param frame_width: Width of the frame
 *  @param frame_height: Height of the frame
 *  @param padded_width: Width of the buffer
 *  @param padded_height: Height of the buffer
 *  @param stride: Stride of the Buffer
 *  @param runningFrame: running counter to track the frames
 */
/*===============================================================*/
	typedef struct DebugFrame_Dump
	{
		OMX_S32 fromFrame;
		OMX_S32 toFrame;
		OMX_U32 frame_width;
		OMX_U32 frame_height;
		OMX_U32 frame_xoffset;
		OMX_U32	frame_yoffset;
		OMX_U32 stride;
		OMX_S32 runningFrame;
		OMX_U32 *y_uv[2];
	}DebugFrame_Dump;
#endif

/* ========================================================================== */
/**
* PROXY_COMPONENT_PRIVATE
*
*/
/* ========================================================================== */
	typedef struct PROXY_COMPONENT_PRIVATE
	{
		/* OMX Related Information */
		OMX_CALLBACKTYPE tCBFunc;
		OMX_PTR pILAppData;
		OMX_HANDLETYPE hRemoteComp;

		PROXY_BUFFER_INFO tBufList[MAX_NUM_PROXY_BUFFERS];
		PROXY_PORT_TYPE proxyPortBuffers[PROXY_MAXNUMOFPORTS];
		OMX_BOOL IsLoadedState;
		OMX_U32 nTotalBuffers;
		OMX_U32 nAllocatedBuffers;

		/* PROXY specific data - PROXY PRIVATE DATA */
		OMX_PTR pCompProxyPrv;
		char *cCompName;

		PROXY_EMPTYBUFFER_DONE proxyEmptyBufferDone;
		PROXY_FILLBUFFER_DONE proxyFillBufferDone;
		PROXY_EVENTHANDLER proxyEventHandler;

#ifdef ANDROID_QUIRK_LOCK_BUFFER
		gralloc_module_t const *grallocModule;
#endif
#ifdef USE_ION
		int ion_fd;
		OMX_BOOL bUseIon;
		OMX_BOOL bMapIonBuffers;
#endif
#ifdef ENABLE_RAW_BUFFERS_DUMP_UTILITY
		DebugFrame_Dump debugframeInfo;
#endif
		int secure_misc_drv_fd;
	} PROXY_COMPONENT_PRIVATE;



/*===============================================================*/
/** PROXY_MARK_DATA        : A pointer to this structure is sent as mark data to
 *                           the remote core when a MarkBuffer command is made.
 *
 * @param hComponentActual : This is the actual handle of the component to be
 *                           marked. When marked buffers come from the remote
 *                           to the local core then remote handle of the mark
 *                           component is replaced by this in the header.
 *
 * @param pMarkdataActual : This is the mark data set by the client.
 */
/*===============================================================*/
	typedef struct PROXY_MARK_DATA
	{
		OMX_HANDLETYPE hComponentActual;
		OMX_PTR pMarkDataActual;
	} PROXY_MARK_DATA;
/*******************************************************************************
* Functions
*******************************************************************************/
	OMX_ERRORTYPE OMX_ProxyCommonInit(OMX_HANDLETYPE hComponent);
	OMX_ERRORTYPE PROXY_GetParameter(OMX_IN OMX_HANDLETYPE hComponent,
	    OMX_IN OMX_INDEXTYPE nParamIndex, OMX_INOUT OMX_PTR pParamStruct);
	OMX_ERRORTYPE PROXY_SetParameter(OMX_IN OMX_HANDLETYPE hComponent,
	    OMX_IN OMX_INDEXTYPE nParamIndex, OMX_INOUT OMX_PTR pParamStruct);
	OMX_ERRORTYPE PROXY_EventHandler(OMX_HANDLETYPE hComponent,
	    OMX_PTR pAppData, OMX_EVENTTYPE eEvent, OMX_U32 nData1, OMX_U32 nData2,
	    OMX_PTR pEventData);
	OMX_ERRORTYPE PROXY_SendCommand(OMX_IN OMX_HANDLETYPE hComponent,
	    OMX_IN OMX_COMMANDTYPE eCmd,
	    OMX_IN OMX_U32 nParam, OMX_IN OMX_PTR pCmdData);
	OMX_ERRORTYPE PROXY_EmptyThisBuffer(OMX_HANDLETYPE hComponent,
	    OMX_BUFFERHEADERTYPE * pBufferHdr);
	OMX_ERRORTYPE PROXY_GetExtensionIndex(OMX_IN OMX_HANDLETYPE hComponent,
	    OMX_IN OMX_STRING cParameterName, OMX_OUT OMX_INDEXTYPE * pIndexType);
	OMX_ERRORTYPE PROXY_FillBufferDone(OMX_HANDLETYPE hComponent,
	OMX_U32 remoteBufHdr, OMX_U32 nfilledLen, OMX_U32 nOffset, OMX_U32 nFlags,
	    OMX_TICKS nTimeStamp, OMX_HANDLETYPE hMarkTargetComponent,
	    OMX_PTR pMarkData);
	OMX_ERRORTYPE PROXY_FillThisBuffer(OMX_HANDLETYPE hComponent,
	    OMX_BUFFERHEADERTYPE * pBufferHdr);
	OMX_ERRORTYPE PROXY_AllocateBuffer(OMX_IN OMX_HANDLETYPE hComponent,
	    OMX_INOUT OMX_BUFFERHEADERTYPE ** ppBufferHdr,OMX_IN OMX_U32 nPortIndex,
	    OMX_IN OMX_PTR pAppPrivate, OMX_IN OMX_U32 nSizeBytes);
	OMX_ERRORTYPE PROXY_FreeBuffer(OMX_IN OMX_HANDLETYPE hComponent,
	    OMX_IN OMX_U32 nPortIndex, OMX_IN OMX_BUFFERHEADERTYPE * pBufferHdr);
	OMX_ERRORTYPE PROXY_ComponentDeInit(OMX_HANDLETYPE hComponent);


#ifdef __cplusplus
}
#endif				/* __cplusplus */

#endif
