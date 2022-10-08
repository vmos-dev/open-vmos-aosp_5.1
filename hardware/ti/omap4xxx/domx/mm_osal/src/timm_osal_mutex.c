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
*   @file  timm_osal_Mutexs.c
*   This file contains methods that provides the functionality
*
*  @path \
*
*/
/* -------------------------------------------------------------------------- */
/* =========================================================================
 *!
 *! Revision History
 *! ===================================
 *! 04-Nov-2008 Maiya ShreeHarsha: Linux specific changes
 *! 0.1: Created the first draft version, ksrini@ti.com
 * ========================================================================= */

/******************************************************************************
* Includes
******************************************************************************/


#include "timm_osal_types.h"
#include "timm_osal_trace.h"
#include "timm_osal_error.h"
#include "timm_osal_memory.h"
#include "timm_osal_semaphores.h"

#include <errno.h>

#include <pthread.h>
#include <sys/time.h>


/* ========================================================================== */
/**
* @fn TIMM_OSAL_MutexCreate function
*
*
*/
/* ========================================================================== */
TIMM_OSAL_ERRORTYPE TIMM_OSAL_MutexCreate(TIMM_OSAL_PTR * pMutex)
{
	TIMM_OSAL_ERRORTYPE bReturnStatus = TIMM_OSAL_ERR_UNKNOWN;
	pthread_mutex_t *plMutex = TIMM_OSAL_NULL;

	plMutex =
	    (pthread_mutex_t *) TIMM_OSAL_Malloc(sizeof(pthread_mutex_t), 0,
	    0, 0);
	if (TIMM_OSAL_NULL == plMutex)
	{
		bReturnStatus = TIMM_OSAL_ERR_ALLOC;
		goto EXIT;
	}

	/*if (SUCCESS != pthread_mutex_init(plMutex, pAttr)) */
	if (SUCCESS != pthread_mutex_init(plMutex, TIMM_OSAL_NULL))
	{
		/*TIMM_OSAL_Error ("Mutex Create failed !"); */
		/*goto EXIT; */
	} else
	{
	/**pMutex = (TIMM_OSAL_PTR *)plMutex;*/
		*pMutex = (TIMM_OSAL_PTR) plMutex;
		bReturnStatus = TIMM_OSAL_ERR_NONE;
	}
      EXIT:
	/*if((TIMM_OSAL_ERR_NONE != bReturnStatus)) {
	   TIMM_OSAL_Free(plMutex);
	   } */
	if ((TIMM_OSAL_ERR_NONE != bReturnStatus) &&
	    (TIMM_OSAL_NULL != plMutex))
	{
		TIMM_OSAL_Free(plMutex);
	}
	return bReturnStatus;

/**********************************************************/
/*return TIMM_OSAL_SemaphoreCreate(pMutex, 1);*/
/**********************************************************/
}

/* ========================================================================== */
/**
* @fn TIMM_OSAL_MutexDelete function
*
*
*/
/* ========================================================================== */
TIMM_OSAL_ERRORTYPE TIMM_OSAL_MutexDelete(TIMM_OSAL_PTR pMutex)
{
	TIMM_OSAL_ERRORTYPE bReturnStatus = TIMM_OSAL_ERR_NONE;
	pthread_mutex_t *plMutex = (pthread_mutex_t *) pMutex;

	if (plMutex == TIMM_OSAL_NULL)
	{
		bReturnStatus = TIMM_OSAL_ERR_PARAMETER;
		goto EXIT;
	}
	/*can we do away with if or with switch case */
	if (SUCCESS != pthread_mutex_destroy(plMutex))
	{
		/*TIMM_OSAL_Error("Delete Mutex failed !"); */
		bReturnStatus = TIMM_OSAL_ERR_UNKNOWN;;
	}

	TIMM_OSAL_Free(plMutex);
      EXIT:
	return bReturnStatus;
/**********************************************************/
/*return TIMM_OSAL_SemaphoreDelete(pMutex);*/
/**********************************************************/

}

/* ========================================================================== */
/**
* @fn TIMM_OSAL_MutexObtain function
*
*
*/
/* ========================================================================== */

TIMM_OSAL_ERRORTYPE TIMM_OSAL_MutexObtain(TIMM_OSAL_PTR pMutex,
    TIMM_OSAL_U32 uTimeOut)
{
	TIMM_OSAL_ERRORTYPE bReturnStatus = TIMM_OSAL_ERR_UNKNOWN;
	struct timespec abs_timeout;
	struct timeval ltime_now;
	TIMM_OSAL_U32 ltimenow_us;
	pthread_mutex_t *plMutex = (pthread_mutex_t *) pMutex;

	if (plMutex == TIMM_OSAL_NULL)
	{
		bReturnStatus = TIMM_OSAL_ERR_PARAMETER;
		goto EXIT;
	}

	if (TIMM_OSAL_SUSPEND == uTimeOut)
	{
		if (SUCCESS != pthread_mutex_lock(plMutex))
		{
			/*TIMM_OSAL_Error("Lock Mutex failed !"); */
			goto EXIT;
		}
	} else if (TIMM_OSAL_NO_SUSPEND == uTimeOut)
	{
		if (SUCCESS != pthread_mutex_trylock(plMutex))
		{
			/*TIMM_OSAL_Error("Lock Mutex failed !"); */
			goto EXIT;
		}
	} else
	{
		gettimeofday(&ltime_now, NULL);
		/*uTimeOut is assumed to be in milliseconds */
		ltimenow_us = ltime_now.tv_usec + 1000 * uTimeOut;
		abs_timeout.tv_sec = ltime_now.tv_sec + uTimeOut / 1000;
		abs_timeout.tv_nsec = (ltimenow_us % 1000000) * 1000;

#ifdef _POSIX_VERSION_1_
		if (SUCCESS != pthread_mutex_lock(plMutex))
		{		//Some Posix versions dont support timeout
#else
		if (SUCCESS != pthread_mutex_timedlock(plMutex, &abs_timeout))
		{
#endif
			/*TIMM_OSAL_Error("Lock Mutex failed !"); */
			goto EXIT;
		}
	}
	bReturnStatus = TIMM_OSAL_ERR_NONE;

      EXIT:
	return bReturnStatus;
/**********************************************************/
/*return TIMM_OSAL_SemaphoreObtain(pMutex, uTimeOut);*/
/**********************************************************/
}



/* ========================================================================== */
/**
* @fn TIMM_OSAL_MutexRelease function
*
*
*/
/* ========================================================================== */
TIMM_OSAL_ERRORTYPE TIMM_OSAL_MutexRelease(TIMM_OSAL_PTR pMutex)
{
	TIMM_OSAL_ERRORTYPE bReturnStatus = TIMM_OSAL_ERR_UNKNOWN;
	pthread_mutex_t *plMutex = (pthread_mutex_t *) pMutex;

	if (TIMM_OSAL_NULL == plMutex)
	{
		bReturnStatus = TIMM_OSAL_ERR_PARAMETER;
		goto EXIT;
	}

	if (SUCCESS != pthread_mutex_unlock(plMutex))
	{
		/*TIMM_OSAL_Error("Unlock Mutex failed !"); */
	} else
	{
		bReturnStatus = TIMM_OSAL_ERR_NONE;
	}
      EXIT:
	return bReturnStatus;
/**********************************************************/
/*return TIMM_OSAL_SemaphoreRelease(pMutex);*/
/**********************************************************/
}
