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
*   @file  timm_osal_pipes.c
*   This file contains methods that provides the functionality
*   for creating/using Nucleus pipes.
*
*  @path \
*
*/
/* -------------------------------------------------------------------------- */
/* =========================================================================
 *!
 *! Revision History
 *! ===================================
 *! 07-Nov-2008 Maiya ShreeHarsha: Linux specific changes
 *! 0.1: Created the first draft version, ksrini@ti.com
 * ========================================================================= */

/******************************************************************************
* Includes
******************************************************************************/

#include "timm_osal_types.h"
#include "timm_osal_error.h"
#include "timm_osal_memory.h"
#include "timm_osal_trace.h"

#include <unistd.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

/**
* TIMM_OSAL_PIPE structure define the OSAL pipe
*/
typedef struct TIMM_OSAL_PIPE
{
	int pfd[2];
	TIMM_OSAL_U32 pipeSize;
	TIMM_OSAL_U32 messageSize;
	TIMM_OSAL_U8 isFixedMessage;
	int messageCount;
	int totalBytesInPipe;
} TIMM_OSAL_PIPE;


/******************************************************************************
* Function Prototypes
******************************************************************************/

/* ========================================================================== */
/**
* @fn TIMM_OSAL_CreatePipe function
*
*
*/
/* ========================================================================== */

TIMM_OSAL_ERRORTYPE TIMM_OSAL_CreatePipe(TIMM_OSAL_PTR * pPipe,
    TIMM_OSAL_U32 pipeSize,
    TIMM_OSAL_U32 messageSize, TIMM_OSAL_U8 isFixedMessage)
{
	TIMM_OSAL_ERRORTYPE bReturnStatus = TIMM_OSAL_ERR_UNKNOWN;
	TIMM_OSAL_PIPE *pHandle = TIMM_OSAL_NULL;

	pHandle =
	    (TIMM_OSAL_PIPE *) TIMM_OSAL_Malloc(sizeof(TIMM_OSAL_PIPE), 0, 0,
	    0);

	if (TIMM_OSAL_NULL == pHandle)
	{
		bReturnStatus = TIMM_OSAL_ERR_ALLOC;
		goto EXIT;
	}
	TIMM_OSAL_Memset(pHandle, 0x0, sizeof(TIMM_OSAL_PIPE));

	pHandle->pfd[0] = -1;
	pHandle->pfd[1] = -1;
	if (SUCCESS != pipe(pHandle->pfd))
	{
		TIMM_OSAL_Error("Pipe failed: %s!!!", strerror(errno));
		goto EXIT;
	}

	pHandle->pipeSize = pipeSize;
	pHandle->messageSize = messageSize;
	pHandle->isFixedMessage = isFixedMessage;
	pHandle->messageCount = 0;
	pHandle->totalBytesInPipe = 0;

	*pPipe = (TIMM_OSAL_PTR) pHandle;

	bReturnStatus = TIMM_OSAL_ERR_NONE;


	return bReturnStatus;
EXIT:
	TIMM_OSAL_Free(pHandle);
	return bReturnStatus;
}



/* ========================================================================== */
/**
* @fn TIMM_OSAL_DeletePipe function
*
*
*/
/* ========================================================================== */

TIMM_OSAL_ERRORTYPE TIMM_OSAL_DeletePipe(TIMM_OSAL_PTR pPipe)
{
	TIMM_OSAL_ERRORTYPE bReturnStatus = TIMM_OSAL_ERR_NONE;

	TIMM_OSAL_PIPE *pHandle = (TIMM_OSAL_PIPE *) pPipe;

	if (TIMM_OSAL_NULL == pHandle)
	{
		bReturnStatus = TIMM_OSAL_ERR_PARAMETER;
		goto EXIT;
	}

	if (SUCCESS != close(pHandle->pfd[0]))
	{
		TIMM_OSAL_Error("Delete_Pipe Read fd failed!!!");
		bReturnStatus = TIMM_OSAL_ERR_UNKNOWN;
	}
	if (SUCCESS != close(pHandle->pfd[1]))
	{
		TIMM_OSAL_Error("Delete_Pipe Write fd failed!!!");
		bReturnStatus = TIMM_OSAL_ERR_UNKNOWN;
	}

	TIMM_OSAL_Free(pHandle);
EXIT:
	return bReturnStatus;
}



/* ========================================================================== */
/**
* @fn TIMM_OSAL_WriteToPipe function
*
*
*/
/* ========================================================================== */

TIMM_OSAL_ERRORTYPE TIMM_OSAL_WriteToPipe(TIMM_OSAL_PTR pPipe,
    void *pMessage, TIMM_OSAL_U32 size, TIMM_OSAL_S32 timeout)
{
	TIMM_OSAL_ERRORTYPE bReturnStatus = TIMM_OSAL_ERR_UNKNOWN;
	TIMM_OSAL_U32 lSizeWritten = -1;

	TIMM_OSAL_PIPE *pHandle = (TIMM_OSAL_PIPE *) pPipe;

	if (size == 0)
	{
		TIMM_OSAL_Error("0 size!!!");
		bReturnStatus = TIMM_OSAL_ERR_PARAMETER;
		goto EXIT;
	}
	lSizeWritten = write(pHandle->pfd[1], pMessage, size);

	if (lSizeWritten != size)
	{
		TIMM_OSAL_Error("Write of pipe failed!!!");
		bReturnStatus = TIMM_OSAL_ERR_PARAMETER;
		goto EXIT;
	}

	/*Update message count and size */
	pHandle->messageCount++;
	pHandle->totalBytesInPipe += size;

	bReturnStatus = TIMM_OSAL_ERR_NONE;

      EXIT:
	return bReturnStatus;
}



/* ========================================================================== */
/**
* @fn TIMM_OSAL_WriteToFrontOfPipe function
*
*
*/
/* ========================================================================== */

TIMM_OSAL_ERRORTYPE TIMM_OSAL_WriteToFrontOfPipe(TIMM_OSAL_PTR pPipe,
    void *pMessage, TIMM_OSAL_U32 size, TIMM_OSAL_S32 timeout)
{

	TIMM_OSAL_ERRORTYPE bReturnStatus = TIMM_OSAL_ERR_UNKNOWN;
	TIMM_OSAL_U32 lSizeWritten = -1;
	TIMM_OSAL_U32 lSizeRead = -1;
	TIMM_OSAL_PIPE *pHandle = (TIMM_OSAL_PIPE *) pPipe;
	TIMM_OSAL_U8 *tempPtr = NULL;


	/*First write to this pipe */
	if (size == 0)
	{
		bReturnStatus = TIMM_OSAL_ERR_PARAMETER;
		goto EXIT;
	}

	lSizeWritten = write(pHandle->pfd[1], pMessage, size);

	if (lSizeWritten != size)
	{
		bReturnStatus = TIMM_OSAL_ERR_PARAMETER;
		goto EXIT;
	}

	/*Update number of messages */
	pHandle->messageCount++;


	if (pHandle->messageCount > 1)
	{
		/*First allocate memory */
		tempPtr =
		    (TIMM_OSAL_U8 *) TIMM_OSAL_Malloc(pHandle->
		    totalBytesInPipe, 0, 0, 0);

		if (tempPtr == NULL)
		{
			bReturnStatus = TIMM_OSAL_ERR_PARAMETER;
			goto EXIT;
		}

		/*Read out of pipe */
		lSizeRead =
		    read(pHandle->pfd[0], tempPtr, pHandle->totalBytesInPipe);

		/*Write back to pipe */
		lSizeWritten =
		    write(pHandle->pfd[1], tempPtr,
		    pHandle->totalBytesInPipe);

		if (lSizeWritten != size)
		{
			bReturnStatus = TIMM_OSAL_ERR_PARAMETER;
			goto EXIT;
		}

		/*Update Total bytes in pipe */
		pHandle->totalBytesInPipe += size;
	}


      EXIT:
	TIMM_OSAL_Free(tempPtr);

	return bReturnStatus;

}



/* ========================================================================== */
/**
* @fn TIMM_OSAL_ReadFromPipe function
*
*
*/
/* ========================================================================== */

TIMM_OSAL_ERRORTYPE TIMM_OSAL_ReadFromPipe(TIMM_OSAL_PTR pPipe,
    void *pMessage,
    TIMM_OSAL_U32 size, TIMM_OSAL_U32 * actualSize, TIMM_OSAL_S32 timeout)
{
	TIMM_OSAL_ERRORTYPE bReturnStatus = TIMM_OSAL_ERR_UNKNOWN;
	TIMM_OSAL_U32 lSizeRead = -1;
	TIMM_OSAL_PIPE *pHandle = (TIMM_OSAL_PIPE *) pPipe;

	if (size == 0)
	{
		TIMM_OSAL_Error("nRead size has error!!!");
		bReturnStatus = TIMM_OSAL_ERR_PARAMETER;
		goto EXIT;
	}
	if ((pHandle->messageCount == 0) && (timeout == TIMM_OSAL_NO_SUSPEND))
	{
		/*If timeout is 0 and pipe is empty, return error */
		TIMM_OSAL_Error("Pipe is empty!!!");
		bReturnStatus = TIMM_OSAL_ERR_PIPE_EMPTY;
		goto EXIT;
	}
	if ((timeout !=TIMM_OSAL_NO_SUSPEND) &&
	    (timeout != (TIMM_OSAL_S32)TIMM_OSAL_SUSPEND))
	{
		TIMM_OSAL_Warning("Only infinite or no timeouts \
			supported. Going to read with infinite timeout now");
	}
	/*read blocks infinitely until message is available */
	*actualSize = lSizeRead = read(pHandle->pfd[0], pMessage, size);
	if (0 == lSizeRead)
	{
		TIMM_OSAL_Error("EOF reached or no data in pipe!!!");
		bReturnStatus = TIMM_OSAL_ERR_PARAMETER;
		goto EXIT;
	}

	bReturnStatus = TIMM_OSAL_ERR_NONE;

	pHandle->messageCount--;
	pHandle->totalBytesInPipe -= size;

      EXIT:
	return bReturnStatus;

}



/* ========================================================================== */
/**
* @fn TIMM_OSAL_ClearPipe function
*
*
*/
/* ========================================================================== */

TIMM_OSAL_ERRORTYPE TIMM_OSAL_ClearPipe(TIMM_OSAL_PTR pPipe)
{
	TIMM_OSAL_ERRORTYPE bReturnStatus = TIMM_OSAL_ERR;

	TIMM_OSAL_Warning("This function is currently not implemented");

	return bReturnStatus;
}



/* ========================================================================== */
/**
* @fn TIMM_OSAL_IsPipeReady function
*
*
*/
/* ========================================================================== */

TIMM_OSAL_ERRORTYPE TIMM_OSAL_IsPipeReady(TIMM_OSAL_PTR pPipe)
{
	TIMM_OSAL_ERRORTYPE bReturnStatus = TIMM_OSAL_ERR;
	TIMM_OSAL_PIPE *pHandle = (TIMM_OSAL_PIPE *) pPipe;

	if (pHandle->messageCount <= 0)
	{
		bReturnStatus = TIMM_OSAL_ERR_NOT_READY;
	} else
	{
		bReturnStatus = TIMM_OSAL_ERR_NONE;
	}

	return bReturnStatus;

}



/* ========================================================================== */
/**
* @fn TIMM_OSAL_GetPipeReadyMessageCount function
*
*
*/
/* ========================================================================== */

TIMM_OSAL_ERRORTYPE TIMM_OSAL_GetPipeReadyMessageCount(TIMM_OSAL_PTR pPipe,
    TIMM_OSAL_U32 * count)
{
	TIMM_OSAL_ERRORTYPE bReturnStatus = TIMM_OSAL_ERR_NONE;
	TIMM_OSAL_PIPE *pHandle = (TIMM_OSAL_PIPE *) pPipe;

	*count = pHandle->messageCount;
	return bReturnStatus;

}
