
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
#ifndef OMX_WBAMR_DEC_UTILS__H
#define OMX_WBAMR_DEC_UTILS__H

#include <OMX_Component.h>
#include "OMX_TI_Common.h"
#include "OMX_WbAmrDecoder.h"
#ifdef RESOURCE_MANAGER_ENABLED
#include <ResourceManagerProxyAPI.h>
#endif


/* ======================================================================= */
/**
 * @def    WBAMR_DEC__XXX_VER    Component version
 */
/* ======================================================================= */
#define WBAMR_DEC_MAJOR_VER 0xF1
#define WBAMR_DEC_MINOR_VER 0xF2
/* ======================================================================= */
/**
 * @def    WBAMR_DEC_NOT_USED    Defines a value for "don't care" parameters
 */
/* ======================================================================= */
#define WBAMR_DEC_NOT_USED 10
/* ======================================================================= */
/**
 * @def    WBAMR_DEC_NORMAL_BUFFER    Defines the flag value with all flags turned off
 */
/* ======================================================================= */
#define WBAMR_DEC_NORMAL_BUFFER 0
/* ======================================================================= */
/**
 * @def    OMX_WBAMR_DEC_DEFAULT_SEGMENT    Default segment ID for the LCML
 */
/* ======================================================================= */
#define OMX_WBAMR_DEC_DEFAULT_SEGMENT (0)
/* ======================================================================= */
/**
 * @def    OMX_WBAMR_DEC_SN_TIMEOUT    Timeout value for the socket node
 */
/* ======================================================================= */
#define OMX_WBAMR_DEC_SN_TIMEOUT (-1)
/* ======================================================================= */
/**
 * @def    OMX_WBAMR_DEC_SN_PRIORITY   Priority for the socket node
 */
/* ======================================================================= */
#define OMX_WBAMR_DEC_SN_PRIORITY (10)
/* ======================================================================= */
/**
 * @def    OMX_WBAMR_DEC_NUM_DLLS   number of DLL's
 */
 /* =================================================================================== */
/*
* Different Frame sizes
*/
/* ================================================================================== */
#define  WBAMR_DEC_FRAME_SIZE_18	18
#define  WBAMR_DEC_FRAME_SIZE_23  	23
#define  WBAMR_DEC_FRAME_SIZE_24  	24
#define  WBAMR_DEC_FRAME_SIZE_33  	33
#define  WBAMR_DEC_FRAME_SIZE_37  	37
#define  WBAMR_DEC_FRAME_SIZE_41  	41
#define  WBAMR_DEC_FRAME_SIZE_47  	47
#define  WBAMR_DEC_FRAME_SIZE_51  	51
#define  WBAMR_DEC_FRAME_SIZE_59  	59
#define  WBAMR_DEC_FRAME_SIZE_61	61
#define  WBAMR_DEC_FRAME_SIZE_6		6
#define  WBAMR_DEC_FRAME_SIZE_1		1
#define  WBAMR_DEC_FRAME_SIZE_0		0

#define WBAMRDEC_APP_ID 100

 
 /* ======================================================================= */
/**
 * @def    INPUT_WBAMRDEC_BUFFER_SIZE_IF2   Default input buffer size IF2
 *
 */
/* ======================================================================= */
#define INPUT_WBAMRDEC_BUFFER_SIZE_IF2 61

/* ======================================================================= */
/** WBAMRENC_MimeMode  format types
*
*  @param  WBAMRENC_MIMEMODE				MIME
*
*  @param  WBAMRENC_NONMIMEMODE				WBAMR mode
*
*/
/* ======================================================================= */
enum WBAMRDEC_MimeMode {
	WBAMRDEC_FORMATCONFORMANCE = 0,
	WBAMRDEC_MIMEMODE, 
	WBAMRDEC_IF2
};
 
/* ======================================================================= */
#define OMX_WBAMR_DEC_NUM_DLLS (2)
/* ======================================================================= */
/**
 * @def    WBAMR_DEC_USN_DLL_NAME   USN DLL name
 */
/* ======================================================================= */
#ifdef UNDER_CE
#define WBAMR_DEC_USN_DLL_NAME "\\windows\\usn.dll64P"
#else
#define WBAMR_DEC_USN_DLL_NAME "usn.dll64P"
#endif

/* ======================================================================= */
/**
 * @def    WBAMR_DEC_DLL_NAME   WB AMR Decoder socket node dll name
 */
/* ======================================================================= */
#ifdef UNDER_CE
#define WBAMR_DEC_DLL_NAME "\\windows\\wbamrdec_sn.dll64P"
#else
#define WBAMR_DEC_DLL_NAME "wbamrdec_sn.dll64P"
#endif

/* ===========================================================  */
/**
*  WBAMR_DEC_StartComponentThread()  Starts component thread
*
*
*  @param hComp			OMX Handle
*
*  @return OMX_ErrorNone = Successful
*          Other error code = fail
*
*/
/*================================================================== */
OMX_ERRORTYPE WBAMR_DEC_StartComponentThread(OMX_HANDLETYPE pHandle);
/* ===========================================================  */
/**
*  WBAMR_DEC_StopComponentThread()  Stops component thread
*
*
*  @param hComp			OMX Handle
*
*  @return OMX_ErrorNone = Successful
*          Other error code = fail
*
*/
/*================================================================== */
OMX_ERRORTYPE WBAMR_DEC_StopComponentThread(OMX_HANDLETYPE pHandle);
/* ===========================================================  */
/**
*  WBAMR_DEC_FreeCompResources()  Frees allocated memory
*
*
*  @param hComp			OMX Handle
*
*  @return OMX_ErrorNone = Successful
*          Other error code = fail
*
*/
/*================================================================== */
OMX_ERRORTYPE WBAMR_DEC_FreeCompResources(OMX_HANDLETYPE pComponent);
/* ===========================================================  */
/**
*  WBAMR_DEC_GetCorresponding_LCMLHeader()  Returns LCML header
* that corresponds to the given buffer
*
*  @param pComponentPrivate	Component private data
*
*  @return OMX_ErrorNone = Successful
*          Other error code = fail
*/
/*================================================================== */
OMX_ERRORTYPE WBAMR_DEC_GetCorresponding_LCMLHeader(WBAMR_DEC_COMPONENT_PRIVATE *pComponentPrivate,
                                          OMX_U8 *pBuffer,
                                          OMX_DIRTYPE eDir,
                                          LCML_WBAMR_DEC_BUFHEADERTYPE **ppLcmlHdr);
/* ===========================================================  */
/**
*  WBAMR_DEC_LCML_Callback() Callback from LCML
*
*  @param event		Codec Event
*
*  @param args		Arguments from LCML
*
*  @return OMX_ErrorNone = Successful
*          Other error code = fail
*/
/*================================================================== */
OMX_ERRORTYPE WBAMR_DEC_LCML_Callback (TUsnCodecEvent event,void * args [10]);
/* ===========================================================  */
/**
*  WBAMR_DEC_Fill_LCMLInitParams() Fills the parameters needed
* to initialize the LCML
*
*  @param pHandle OMX Handle
*
*  @param plcml_Init LCML initialization parameters
*
*  @return OMX_ErrorNone = Successful
*          Other error code = fail
*
*/
/*================================================================== */
OMX_ERRORTYPE WBAMR_DEC_Fill_LCMLInitParams(OMX_HANDLETYPE pHandle,
                  LCML_DSP *plcml_Init,OMX_U16 arr[]);
/* ===========================================================  */
/**
*  WBAMR_DEC_GetBufferDirection() Returns direction of pBufHeader
*
*  @param pBufHeader		Buffer header
*
*  @param eDir				Buffer direction
*
*  @param pComponentPrivate	Component private data
*
*  @return OMX_ErrorNone = Successful
*          Other error code = fail
*/
/*================================================================== */
OMX_ERRORTYPE WBAMR_DEC_GetBufferDirection(OMX_BUFFERHEADERTYPE *pBufHeader, OMX_DIRTYPE *eDir);
/* ===========================================================  */
/**
*  WBAMR_DEC_HandleCommand()  Handles commands sent via SendCommand()
*
*  @param pComponentPrivate	Component private data
*
*  @return OMX_ErrorNone = Successful
*          Other error code = fail
*  @return OMX_ErrorNone = Successful
*          Other error code = fail
*/
/*================================================================== */
OMX_U32 WBAMR_DEC_HandleCommand (WBAMR_DEC_COMPONENT_PRIVATE *pComponentPrivate);
/* ===========================================================  */
/**
*  WBAMR_DEC_HandleDataBuf_FromApp()  Handles data buffers received
* from the IL Client
*
*  @param pComponentPrivate	Component private data
*
*  @return OMX_ErrorNone = Successful
*          Other error code = fail
*  @return OMX_ErrorNone = Successful
*          Other error code = fail
*/
/*================================================================== */
OMX_ERRORTYPE WBAMR_DEC_HandleDataBuf_FromApp(OMX_BUFFERHEADERTYPE *pBufHeader,
        WBAMR_DEC_COMPONENT_PRIVATE *pComponentPrivate);
/* ===========================================================  */
/**
*  WBAMR_DEC_GetLCMLHandle()  Get the handle to the LCML
*
*
*  @return OMX_ErrorNone = Successful
*          Other error code = fail
*/
/*================================================================== */
OMX_HANDLETYPE WBAMR_DEC_GetLCMLHandle(WBAMR_DEC_COMPONENT_PRIVATE *pComponentPrivate);
/* ===========================================================  */
/**
*  WBAMR_DEC_FreeLCMLHandle()  Frees the handle to the LCML
*
*
*  @return OMX_ErrorNone = Successful
*          Other error code = fail
*/
/*================================================================== */
OMX_ERRORTYPE WBAMR_DEC_FreeLCMLHandle(WBAMR_DEC_COMPONENT_PRIVATE *pComponentPrivate);
/* ===========================================================  */
/**
*  WBAMR_DEC_CleanupInitParams()  Starts component thread
*
*  @param pComponent		OMX Handle
*
*  @return OMX_ErrorNone = Successful
*          Other error code = fail
*/
/*================================================================== */
OMX_ERRORTYPE WBAMR_DEC_CleanupInitParams(OMX_HANDLETYPE pComponent);
/* ===========================================================  */
/**
*  WBAMR_DEC_SetPending()  Called when the component queues a buffer
* to the LCML
*
*  @param pComponentPrivate		Component private data
*
*  @param pBufHdr				Buffer header
*
*  @param eDir					Direction of the buffer
*
*  @return None
*/
/*================================================================== */
void WBAMR_DEC_SetPending(WBAMR_DEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_BUFFERHEADERTYPE *pBufHdr, OMX_DIRTYPE eDir, OMX_U32 lineNumber);
/* ===========================================================  */
/**
*  WBAMR_DEC_ClearPending()  Called when a buffer is returned
* from the LCML
*
*  @param pComponentPrivate		Component private data
*
*  @param pBufHdr				Buffer header
*
*  @param eDir					Direction of the buffer
*
*  @return None
*/
/*================================================================== */
void WBAMR_DEC_ClearPending(WBAMR_DEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_BUFFERHEADERTYPE *pBufHdr, OMX_DIRTYPE eDir, OMX_U32 lineNumber) ;
/* ===========================================================  */
/**
*  WMADEC_IsPending()
*
*
*  @param pComponentPrivate		Component private data
*
*  @return OMX_ErrorNone = Successful
*          Other error code = fail
*/
/*================================================================== */
OMX_U32 WBAMR_DEC_IsPending(WBAMR_DEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_BUFFERHEADERTYPE *pBufHdr, OMX_DIRTYPE eDir);
/* ===========================================================  */
/**
*  WMADEC_Fill_LCMLInitParamsEx()  Fills the parameters needed
* to initialize the LCML without recreating the socket node
*
*  @param pComponent			OMX Handle
*
*  @return None
*/

/*================================================================== */
OMX_ERRORTYPE WBAMR_DEC_Fill_LCMLInitParamsEx(OMX_HANDLETYPE pComponent);
/* ===========================================================  */
/**
*  WMADEC_IsValid() Returns whether a buffer is valid
*
*
*  @param pComponentPrivate		Component private data
*
*  @param pBuffer				Data buffer
*
*  @param eDir					Buffer direction
*
*  @return OMX_True = Valid
*          OMX_False= Invalid
*/
/*================================================================== */
OMX_U32 WBAMR_DEC_IsValid(WBAMR_DEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_U8 *pBuffer, OMX_DIRTYPE eDir) ;

#ifdef RESOURCE_MANAGER_ENABLED
void WBAMRDEC_ResourceManagerCallback(RMPROXY_COMMANDDATATYPE cbData);
#endif

OMX_ERRORTYPE OMX_DmmMap(DSP_HPROCESSOR ProcHandle, int size, void* pArmPtr, DMM_BUFFER_OBJ* pDmmBuf, struct OMX_TI_Debug dbg);

OMX_ERRORTYPE OMX_DmmUnMap(DSP_HPROCESSOR ProcHandle, void* pMapPtr, void* pResPtr, struct OMX_TI_Debug dbg);

void WBAMRDEC_HandleUSNError (WBAMR_DEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_U32 arg);

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
