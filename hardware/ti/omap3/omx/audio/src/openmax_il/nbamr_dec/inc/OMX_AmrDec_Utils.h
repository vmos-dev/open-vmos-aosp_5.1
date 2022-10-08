
/*
 * Copyright (C) Texas Instruments - http://www.ti.com/
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
/* =============================================================================
*             Texas Instruments OMAP(TM) Platform Software
*  (c) Copyright Texas Instruments, Incorporated.  All Rights Reserved.
*
*  Use of this software is controlled by the terms and conditions found
*  in the license agreement under which this software has been supplied.
* ============================================================================ */
/**
* @file OMX_AmrDec_Utils.h
*
* This is an header file for an audio PCM decoder that is fully
* compliant with the OMX Audio specification.
* This the header file with the utils necesary to for the NBAMR_DEC component.
* in its code.
*
* @path $(CSLPATH)\
*
* @rev 0.1
*/
/* --------------------------------------------------------------------------- */

#ifndef OMX_AMRDEC_UTILS__H
#define OMX_AMRDEC_UTILS__H

#include <OMX_Component.h>
#include "OMX_TI_Common.h"
#include "OMX_AmrDecoder.h"

/* ======================================================================= */
/**
 * @def    AMRDEC_MAJOR_VER              Define value for "major" version
 */
/* ======================================================================= */
#define  AMRDEC_MAJOR_VER 0xF1

/* ======================================================================= */
/**
 * @def    AMRDEC_MINOR_VER              Define value for "minor" version
 */
/* ======================================================================= */
#define  AMRDEC_MINOR_VER 0xF2

/* ======================================================================= */
/**
 * @def    NOT_USED                            Define a not used value
 */
/* ======================================================================= */
#define NOT_USED 10

/* ======================================================================= */
/**
 * @def    NORMAL_BUFFER                       Define a normal buffer value
 */
/* ======================================================================= */
#define NORMAL_BUFFER 0

/* ======================================================================= */
/**
 * @def    OMX_AMRDEC_DEFAULT_SEGMENT        Define the default segment
 */
/* ======================================================================= */
#define OMX_AMRDEC_DEFAULT_SEGMENT (0)

/* ======================================================================= */
/**
 * @def    OMX_AMRDEC_SN_TIMEOUT            Define a value for SN Timeout
 */
/* ======================================================================= */
#define OMX_AMRDEC_SN_TIMEOUT (-1)

/* ======================================================================= */
/**
 * @def    OMX_AMRDEC_SN_PRIORITY           Define a value for SN Priority
 */
/* ======================================================================= */
#define OMX_AMRDEC_SN_PRIORITY (10)

/* ======================================================================= */
/**
 * @def    OMX_AMRDEC_NUM_DLLS              Define a num of DLLS to be used
 */
/* ======================================================================= */
#define OMX_AMRDEC_NUM_DLLS (2)

/* ======================================================================= */
/**
 * @def    NBAMRDEC_USN_DLL_NAME             Path & Name of USN DLL to be used
 *                                           at initialization
 */
/* ======================================================================= */
#ifdef UNDER_CE
	#define NBAMRDEC_USN_DLL_NAME "\\windows\\usn.dll64P"
#else
	#define NBAMRDEC_USN_DLL_NAME "usn.dll64P"
#endif

/* ======================================================================= */
/**
 * @def    NBAMRDEC_USN_DLL_NAME             Path & Name of DLL to be useda
 *                                           at initialization
 */
/* ======================================================================= */
#ifdef UNDER_CE
	#define NBAMRDEC_DLL_NAME "\\windows\\nbamrdec_sn.dll64P"
#else
	#define NBAMRDEC_DLL_NAME "nbamrdec_sn.dll64P"
#endif

OMX_ERRORTYPE NBAMRDECGetCorresponding_LCMLHeader(AMRDEC_COMPONENT_PRIVATE *pComponentPrivate,
                                          OMX_U8 *pBuffer,
                                          OMX_DIRTYPE eDir,
                                          LCML_NBAMRDEC_BUFHEADERTYPE **ppLcmlHdr);
                                          
OMX_ERRORTYPE NBAMRDECLCML_Callback (TUsnCodecEvent event,void * args [10]);

OMX_ERRORTYPE NBAMRDECFill_LCMLInitParams(OMX_HANDLETYPE pHandle,
                  LCML_DSP *plcml_Init,OMX_U16 arr[]);

OMX_ERRORTYPE NBAMRDECGetBufferDirection(OMX_BUFFERHEADERTYPE *pBufHeader, OMX_DIRTYPE *eDir);
OMX_U32 NBAMRDECHandleCommand (AMRDEC_COMPONENT_PRIVATE *pComponentPrivate);

OMX_ERRORTYPE NBAMRDECHandleDataBuf_FromApp(OMX_BUFFERHEADERTYPE *pBufHeader,
        AMRDEC_COMPONENT_PRIVATE *pComponentPrivate);

void  AddHeader(BYTE **pFileBuf);    
void  ResetPtr(BYTE **pFileBuf);     
OMX_HANDLETYPE NBAMRDECGetLCMLHandle(AMRDEC_COMPONENT_PRIVATE *pComponentPrivate);
OMX_ERRORTYPE NBAMRDECFreeLCMLHandle(AMRDEC_COMPONENT_PRIVATE *pComponentPrivate);
OMX_ERRORTYPE NBAMRDEC_CleanupInitParams(OMX_HANDLETYPE pComponent);
void NBAMRDEC_SetPending(AMRDEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_BUFFERHEADERTYPE *pBufHdr, OMX_DIRTYPE eDir);
void NBAMRDEC_ClearPending(AMRDEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_BUFFERHEADERTYPE *pBufHdr, OMX_DIRTYPE eDir) ;
OMX_U32 NBAMRDEC_IsPending(AMRDEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_BUFFERHEADERTYPE *pBufHdr, OMX_DIRTYPE eDir);
OMX_ERRORTYPE NBAMRDECFill_LCMLInitParamsEx(OMX_HANDLETYPE pComponent);
OMX_U32 NBAMRDEC_IsValid(AMRDEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_U8 *pBuffer, OMX_DIRTYPE eDir) ;
OMX_ERRORTYPE OMX_DmmMap(DSP_HPROCESSOR ProcHandle, int size, void* pArmPtr, DMM_BUFFER_OBJ* pDmmBuf, struct OMX_TI_Debug dbg);
OMX_ERRORTYPE OMX_DmmUnMap(DSP_HPROCESSOR ProcHandle, void* pMapPtr, void* pResPtr, struct OMX_TI_Debug dbg);

#ifdef RESOURCE_MANAGER_ENABLED
void NBAMR_ResourceManagerCallback(RMPROXY_COMMANDDATATYPE cbData);
#endif

void NBAMRDEC_HandleUSNError (AMRDEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_U32 arg);
#ifdef UNDER_CE
	#ifndef _OMX_EVENT_
		#define _OMX_EVENT_
		typedef struct OMX_Event {
			HANDLE event;
		} OMX_Event;
	#endif
	int OMX_CreateEvent(OMX_Event *event);
	int OMX_SignalEvent(OMX_Event *event);
	int OMX_WaitForEvent(OMX_Event *event);
	int OMX_DestroyEvent(OMX_Event *event);
#endif

#endif
