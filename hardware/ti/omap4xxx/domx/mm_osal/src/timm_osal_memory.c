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
*   @file  timm_osal_memory.c
*   This file contains methods that provides the functionality
*   for allocating/deallocating memory.
*
*  @path \
*
*/
/* -------------------------------------------------------------------------- */
/* =========================================================================
 *!
 *! Revision History
 *! ===================================
 *!23-Oct-2008 Maiya ShreeHarsha: Linux specific changes
 *!0.1: Created the first draft version, ksrini@ti.com
 * ========================================================================= */

/******************************************************************************
* Includes
******************************************************************************/

#include <string.h>
#include <malloc.h>

#ifdef __KERNEL__
#include <linux/types.h>
#else
#include <stdint.h>
#endif


#include "timm_osal_types.h"
#include "timm_osal_trace.h"
#include "timm_osal_error.h"
#include "timm_osal_memory.h"



static TIMM_OSAL_U32 gMallocCounter = 0;

/******************************************************************************
* Function Prototypes
******************************************************************************/

/* ========================================================================== */
/**
* @fn TIMM_OSAL_createMemoryPool function
*
* @see
*/
/* ========================================================================== */
TIMM_OSAL_ERRORTYPE TIMM_OSAL_CreateMemoryPool(void)
{
	TIMM_OSAL_ERRORTYPE bReturnStatus = TIMM_OSAL_ERR_NONE;
	return bReturnStatus;
}



/* ========================================================================== */
/**
* @fn TIMM_OSAL_DeleteMemoryPool function
*
* @see
*/
/* ========================================================================== */

TIMM_OSAL_ERRORTYPE TIMM_OSAL_DeleteMemoryPool(void)
{
	TIMM_OSAL_ERRORTYPE bReturnStatus = TIMM_OSAL_ERR_NONE;
	return bReturnStatus;

}



/* ========================================================================== */
/**
* @fn TIMM_OSAL_Malloc function
*
* @see
*/
/* ========================================================================== */
TIMM_OSAL_PTR TIMM_OSAL_Malloc(TIMM_OSAL_U32 size,
    TIMM_OSAL_BOOL bBlockContiguous,
    TIMM_OSAL_U32 unBlockAlignment, TIMMOSAL_MEM_SEGMENTID tMemSegId)
{

	TIMM_OSAL_PTR pData = TIMM_OSAL_NULL;

#ifdef HAVE_MEMALIGN
	if (0 == unBlockAlignment)
	{
		pData = malloc((size_t) size);
	} else
	{
		pData = memalign((size_t) unBlockAlignment, (size_t) size);
	}
#else
	if (0 != unBlockAlignment)
	{
		TIMM_OSAL_Warning
		    ("Memory Allocation:Not done for specified nBufferAlignment. Alignment of 0 will be used");

	}
	pData = malloc((size_t) size);	/*size_t is long long */
#endif
	if (TIMM_OSAL_NULL == pData)
	{
		TIMM_OSAL_Error("Malloc failed!!!");
	} else
	{
		/* Memory Allocation was successfull */
		gMallocCounter++;
	}


	return pData;
}

/* ========================================================================== */
/**
* @fn TIMM_OSAL_Free function ....
*
* @see
*/
/* ========================================================================== */

void TIMM_OSAL_Free(TIMM_OSAL_PTR pData)
{
	if (TIMM_OSAL_NULL == pData)
	{
		/*TIMM_OSAL_Warning("TIMM_OSAL_Free called on NULL pointer"); */
		goto EXIT;
	}

	free(pData);
	pData = NULL;
	gMallocCounter--;
      EXIT:
	return;
}

/* ========================================================================== */
/**
* @fn TIMM_OSAL_Memset function ....
*
* @see
*/
/* ========================================================================== */

TIMM_OSAL_ERRORTYPE TIMM_OSAL_Memset(TIMM_OSAL_PTR pBuffer,
    TIMM_OSAL_U8 uValue, TIMM_OSAL_U32 uSize)
{
	TIMM_OSAL_ERRORTYPE bReturnStatus = TIMM_OSAL_ERR_UNKNOWN;

	memset((void *)pBuffer, (int)uValue, (size_t) uSize);
	bReturnStatus = TIMM_OSAL_ERR_NONE;

	return bReturnStatus;
}

/* ========================================================================== */
/**
* @fn TIMM_OSAL_Memcmp function ....
*
* @see
*/
/* ========================================================================== */

TIMM_OSAL_S32 TIMM_OSAL_Memcmp(TIMM_OSAL_PTR pBuffer1, TIMM_OSAL_PTR pBuffer2,
    TIMM_OSAL_U32 uSize)
{
	TIMM_OSAL_S32 result = memcmp(pBuffer1, pBuffer2, uSize);

	if (result > 0)
	{
		return 1;
	} else if (result < 0)
	{
		return -1;
	}

	return 0;
}

/* ========================================================================== */
/**
* @fn TIMM_OSAL_Memcpy function ....
*
* @see
*/
/* ========================================================================== */

TIMM_OSAL_ERRORTYPE TIMM_OSAL_Memcpy(TIMM_OSAL_PTR pBufDst,
    TIMM_OSAL_PTR pBufSrc, TIMM_OSAL_U32 uSize)
{
	TIMM_OSAL_ERRORTYPE bReturnStatus = TIMM_OSAL_ERR_UNKNOWN;

	memcpy(pBufDst, pBufSrc, uSize);
	bReturnStatus = TIMM_OSAL_ERR_NONE;

	return bReturnStatus;
}

/* ========================================================================== */
/**
* @fn TIMM_OSAL_GetMemCounter function ....
*
* @see
*/
/* ========================================================================== */

TIMM_OSAL_U32 TIMM_OSAL_GetMemCounter(void)
{

	return gMallocCounter;
}
