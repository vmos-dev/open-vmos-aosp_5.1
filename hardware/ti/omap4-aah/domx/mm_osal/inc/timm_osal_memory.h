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

/*
*  @file timm_osal_memory.h
*  The osal header file defines
*  @path
*
*/
/* -------------------------------------------------------------------------- */
/* =========================================================================
 *!
 *! Revision History
 *! ===================================
 *! 0.1: Created the first draft version, ksrini@ti.com
 * ========================================================================= */

#ifndef _TIMM_OSAL_MEMORY_H_
#define _TIMM_OSAL_MEMORY_H_

#ifdef __cplusplus
extern "C"
{
#endif				/* __cplusplus */

/*******************************************************************************
* Includes
*******************************************************************************/

#include "timm_osal_types.h"
#include "timm_osal_error.h"


/* Enumeration Flag for Memory Segmenation Id */
	typedef enum TIMMOSAL_MEM_SEGMENTID
	{

		TIMMOSAL_MEM_SEGMENT_EXT = 0,
		TIMMOSAL_MEM_SEGMENT_INT,
		TIMMOSAL_MEM_SEGMENT_UNCACHED
	} TIMMOSAL_MEM_SEGMENTID;


/*******************************************************************************
* External interface
*******************************************************************************/

	TIMM_OSAL_ERRORTYPE TIMM_OSAL_CreateMemoryPool(void);

	TIMM_OSAL_ERRORTYPE TIMM_OSAL_DeleteMemoryPool(void);

	TIMM_OSAL_PTR TIMM_OSAL_Malloc(TIMM_OSAL_U32 size,
	    TIMM_OSAL_BOOL bBlockContiguous, TIMM_OSAL_U32 unBlockAlignment,
	    TIMMOSAL_MEM_SEGMENTID tMemSegId);

	void TIMM_OSAL_Free(TIMM_OSAL_PTR pData);

	TIMM_OSAL_ERRORTYPE TIMM_OSAL_Memset(TIMM_OSAL_PTR pBuffer,
	    TIMM_OSAL_U8 uValue, TIMM_OSAL_U32 uSize);

	TIMM_OSAL_S32 TIMM_OSAL_Memcmp(TIMM_OSAL_PTR pBuffer1,
	    TIMM_OSAL_PTR pBuffer2, TIMM_OSAL_U32 uSize);

	TIMM_OSAL_ERRORTYPE TIMM_OSAL_Memcpy(TIMM_OSAL_PTR pBufDst,
	    TIMM_OSAL_PTR pBufSrc, TIMM_OSAL_U32 uSize);

	TIMM_OSAL_U32 TIMM_OSAL_GetMemCounter(void);

#define TIMM_OSAL_MallocExtn(size, bBlockContiguous, unBlockAlignment, tMemSegId, hHeap) \
    TIMM_OSAL_Malloc(size, bBlockContiguous, unBlockAlignment, tMemSegId )

#ifdef __cplusplus
}
#endif				/* __cplusplus */

#endif				/* _TIMM_OSAL_DEFINES_H_ */
