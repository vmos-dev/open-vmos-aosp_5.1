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
*   @file  timm_osal_events.c
*   This file contains methods that provides the functionality
*   for creating/using events.
*
*  @path \
*
*/
/* -------------------------------------------------------------------------- */
/* =========================================================================
 *!
 *! Revision History
 *! ===================================
 *! 06-Nov-2008 Maiya ShreeHarsha: Linux specific changes
 *! 0.1: Created the first draft version, ksrini@ti.com
 * ========================================================================= */

/******************************************************************************
* Includes
******************************************************************************/
#include <stdio.h>
#include <pthread.h>		/*for POSIX calls */
#include <sys/time.h>
#include <errno.h>

#include "timm_osal_types.h"
#include "timm_osal_trace.h"
#include "timm_osal_error.h"
#include "timm_osal_memory.h"
#include "timm_osal_events.h"


typedef struct
{
	TIMM_OSAL_BOOL bSignaled;
	TIMM_OSAL_U32 eFlags;
	pthread_mutex_t mutex;
	pthread_cond_t condition;
} TIMM_OSAL_THREAD_EVENT;


/* ========================================================================== */
/**
* @fn TIMM_OSAL_EventCreate function
*
*
*/
/* ========================================================================== */
TIMM_OSAL_ERRORTYPE TIMM_OSAL_EventCreate(TIMM_OSAL_PTR * pEvents)
{
	TIMM_OSAL_ERRORTYPE bReturnStatus = TIMM_OSAL_ERR_UNKNOWN;
	TIMM_OSAL_THREAD_EVENT *plEvent = NULL;

	plEvent =
	    (TIMM_OSAL_THREAD_EVENT *)
	    TIMM_OSAL_Malloc(sizeof(TIMM_OSAL_THREAD_EVENT), 0, 0, 0);

	if (TIMM_OSAL_NULL == plEvent)
	{
		bReturnStatus = TIMM_OSAL_ERR_ALLOC;
		goto EXIT;
	}
	plEvent->bSignaled = TIMM_OSAL_FALSE;
	plEvent->eFlags = 0;

	if (SUCCESS != pthread_mutex_init(&(plEvent->mutex), NULL))
	{
		TIMM_OSAL_Error("Event Create:Mutex Init failed !");
		goto EXIT;	/*bReturnStatus = TIMM_OSAL_ERR_UNKNOWN */
	}

	if (SUCCESS != pthread_cond_init(&(plEvent->condition), NULL))
	{
		TIMM_OSAL_Error
		    ("Event Create:Conditional Variable  Init failed !");
		pthread_mutex_destroy(&(plEvent->mutex));
		/*TIMM_OSAL_Free(plEvent); */
	} else
	{
		*pEvents = (TIMM_OSAL_PTR) plEvent;
		bReturnStatus = TIMM_OSAL_ERR_NONE;
	}
      EXIT:
	if ((TIMM_OSAL_ERR_NONE != bReturnStatus) &&
	    (TIMM_OSAL_NULL != plEvent))
	{
		TIMM_OSAL_Free(plEvent);
	}
	return bReturnStatus;
}

/* ========================================================================== */
/**
* @fn TIMM_OSAL_EventDelete function
*
*
*/
/* ========================================================================== */
TIMM_OSAL_ERRORTYPE TIMM_OSAL_EventDelete(TIMM_OSAL_PTR pEvents)
{
	TIMM_OSAL_ERRORTYPE bReturnStatus = TIMM_OSAL_ERR_NONE;
	TIMM_OSAL_THREAD_EVENT *plEvent = (TIMM_OSAL_THREAD_EVENT *) pEvents;

	if (TIMM_OSAL_NULL == plEvent)
	{
		bReturnStatus = TIMM_OSAL_ERR_PARAMETER;
		goto EXIT;
	}

	if (SUCCESS != pthread_mutex_lock(&(plEvent->mutex)))
	{
		TIMM_OSAL_Error("Event Delete: Mutex Lock failed !");
		bReturnStatus = TIMM_OSAL_ERR_UNKNOWN;
	}
	if (SUCCESS != pthread_cond_destroy(&(plEvent->condition)))
	{
		TIMM_OSAL_Error
		    ("Event Delete: Conditional Variable Destroy failed !");
		bReturnStatus = TIMM_OSAL_ERR_UNKNOWN;
	}

	if (SUCCESS != pthread_mutex_unlock(&(plEvent->mutex)))
	{
		TIMM_OSAL_Error("Event Delete: Mutex Unlock failed !");
		bReturnStatus = TIMM_OSAL_ERR_UNKNOWN;
	}

	if (SUCCESS != pthread_mutex_destroy(&(plEvent->mutex)))
	{
		TIMM_OSAL_Error("Event Delete: Mutex Destory failed !");
		bReturnStatus = TIMM_OSAL_ERR_UNKNOWN;
	}

	TIMM_OSAL_Free(plEvent);
      EXIT:
	return bReturnStatus;
}


/* ========================================================================== */
/**
* @fn TIMM_OSAL_EventSet function
*
*
*/
/* ========================================================================== */
TIMM_OSAL_ERRORTYPE TIMM_OSAL_EventSet(TIMM_OSAL_PTR pEvents,
    TIMM_OSAL_U32 uEventFlags, TIMM_OSAL_EVENT_OPERATION eOperation)
{

	TIMM_OSAL_ERRORTYPE bReturnStatus = TIMM_OSAL_ERR_UNKNOWN;
	TIMM_OSAL_THREAD_EVENT *plEvent = (TIMM_OSAL_THREAD_EVENT *) pEvents;

	if (TIMM_OSAL_NULL == plEvent)
	{
		bReturnStatus = TIMM_OSAL_ERR_PARAMETER;
		goto EXIT;
	}

	if (SUCCESS != pthread_mutex_lock(&(plEvent->mutex)))
	{
		TIMM_OSAL_Error("Event Set: Mutex Lock failed !");
		bReturnStatus = TIMM_OSAL_ERR_UNKNOWN;
		goto EXIT;
	}

	switch (eOperation)
	{
	case TIMM_OSAL_EVENT_AND:
		plEvent->eFlags = plEvent->eFlags & uEventFlags;
		break;
	case TIMM_OSAL_EVENT_OR:
		plEvent->eFlags = plEvent->eFlags | uEventFlags;
		break;
	default:
		TIMM_OSAL_Error("Event Set: Bad eOperation !");
		bReturnStatus = TIMM_OSAL_ERR_PARAMETER;
		pthread_mutex_unlock(&plEvent->mutex);
		goto EXIT;
	}

	plEvent->bSignaled = TIMM_OSAL_TRUE;

	if (SUCCESS != pthread_cond_signal(&plEvent->condition))
	{
		TIMM_OSAL_Error
		    ("Event Set: Condition Variable Signal failed !");
		bReturnStatus = TIMM_OSAL_ERR_UNKNOWN;
		pthread_mutex_unlock(&plEvent->mutex);
		goto EXIT;
	}

	if (SUCCESS != pthread_mutex_unlock(&plEvent->mutex))
	{
		TIMM_OSAL_Error("Event Set: Mutex Unlock failed !");
		bReturnStatus = TIMM_OSAL_ERR_UNKNOWN;
	} else
		bReturnStatus = TIMM_OSAL_ERR_NONE;

      EXIT:
	return bReturnStatus;


}

/* ========================================================================== */
/**
* @fn TIMM_OSAL_EventRetrieve function
*
*Spurious  wakeups  from  the  pthread_cond_timedwait() or pthread_cond_wait() functions  may  occur.
*
*A representative sequence for using condition variables is shown below
*
*Thread A (Retrieve Events)							|Thread B (Set Events)
*------------------------------------------------------------------------------------------------------------
*1) Do work up to the point where a certain condition 	|1)Do work
*  must occur (such as "count" must reach a specified 	|2)Lock associated mutex
*  value)											|3)Change the value of the global variable
*2) Lock associated mutex and check value of a global 	|  that Thread-A is waiting upon.
*  variable										|4)Check value of the global Thread-A wait
*3) Call pthread_cond_wait() to perform a blocking wait 	|  variable. If it fulfills the desired
*  for signal from Thread-B. Note that a call to 			|  condition, signal Thread-A.
*  pthread_cond_wait() automatically and atomically 		|5)Unlock mutex.
*  unlocks the associated mutex variable so that it can 	|6)Continue
*  be used by Thread-B.							|
*4) When signalled, wake up. Mutex is automatically and 	|
*  atomically locked.								|
*5) Explicitly unlock mutex							|
*6) Continue										|
*
*/
/* ========================================================================== */
TIMM_OSAL_ERRORTYPE TIMM_OSAL_EventRetrieve(TIMM_OSAL_PTR pEvents,
    TIMM_OSAL_U32 uRequestedEvents,
    TIMM_OSAL_EVENT_OPERATION eOperation,
    TIMM_OSAL_U32 * pRetrievedEvents, TIMM_OSAL_U32 uTimeOutMsec)
{
	TIMM_OSAL_ERRORTYPE bReturnStatus = TIMM_OSAL_ERR_UNKNOWN;
	struct timespec timeout;
	struct timeval now;
	TIMM_OSAL_U32 timeout_us;
	TIMM_OSAL_U32 isolatedFlags;
	int status = -1;
	int and_operation;
	TIMM_OSAL_THREAD_EVENT *plEvent = (TIMM_OSAL_THREAD_EVENT *) pEvents;

	if (TIMM_OSAL_NULL == plEvent)
	{
		bReturnStatus = TIMM_OSAL_ERR_PARAMETER;
		goto EXIT;
	}

	/* Lock the mutex for access to the eFlags global variable */
	if (SUCCESS != pthread_mutex_lock(&(plEvent->mutex)))
	{
		TIMM_OSAL_Error("Event Retrieve: Mutex Lock failed !");
		bReturnStatus = TIMM_OSAL_ERR_UNKNOWN;
		goto EXIT;
	}

	/*Check the eOperation and put it in a variable */
	and_operation = ((TIMM_OSAL_EVENT_AND == eOperation) ||
	    (TIMM_OSAL_EVENT_AND_CONSUME == eOperation));

	/* Isolate the flags. The & operation is suffice for an TIMM_OSAL_EVENT_OR eOperation */
	isolatedFlags = plEvent->eFlags & uRequestedEvents;

	/*Check if it is the AND operation. If yes then, all the flags must match */
	if (and_operation)
	{
		isolatedFlags = (isolatedFlags == uRequestedEvents);
	}


	if (isolatedFlags)
	{

		/*We have got required combination of the eFlags bits and will return it back */
		*pRetrievedEvents = plEvent->eFlags;
		bReturnStatus = TIMM_OSAL_ERR_NONE;
	} else
	{

		/*Required combination of bits is not yet available */
		if (TIMM_OSAL_NO_SUSPEND == uTimeOutMsec)
		{
			*pRetrievedEvents = 0;
			bReturnStatus = TIMM_OSAL_ERR_NONE;
		}

		else if (TIMM_OSAL_SUSPEND == uTimeOutMsec)
		{

			/*Wait till we get the required combination of bits. We we get the required
			 *bits then we go out of the while loop
			 */
			while (!isolatedFlags)
			{

				/*Wait on the conditional variable for another thread to set the eFlags and signal */
				pthread_cond_wait(&(plEvent->condition),
				    &(plEvent->mutex));

				/* eFlags set by some thread. Now, isolate the flags.
				 * The & operation is suffice for an TIMM_OSAL_EVENT_OR eOperation
				 */
				isolatedFlags =
				    plEvent->eFlags & uRequestedEvents;

				/*Check if it is the AND operation. If yes then, all the flags must match */
				if (and_operation)
				{
					isolatedFlags =
					    (isolatedFlags ==
					    uRequestedEvents);
				}
			}

			/* Obtained the requested combination of bits on eFlags */
			*pRetrievedEvents = plEvent->eFlags;
			bReturnStatus = TIMM_OSAL_ERR_NONE;

		} else
		{

			/* Calculate uTimeOutMsec in terms of the absolute time. uTimeOutMsec is in milliseconds */
			gettimeofday(&now, NULL);
			timeout_us = now.tv_usec + 1000 * uTimeOutMsec;
			timeout.tv_sec = now.tv_sec + timeout_us / 1000000;
			timeout.tv_nsec = (timeout_us % 1000000) * 1000;

			while (!isolatedFlags)
			{

				/* Wait till uTimeOutMsec for a thread to signal on the conditional variable */
				status =
				    pthread_cond_timedwait(&(plEvent->
					condition), &(plEvent->mutex),
				    &timeout);

				/*Timedout or error and returned without being signalled */
				if (SUCCESS != status)
				{
					if (ETIMEDOUT == status)
						bReturnStatus =
						    TIMM_OSAL_ERR_NONE;
					*pRetrievedEvents = 0;
					break;
				}

				/* eFlags set by some thread. Now, isolate the flags.
				 * The & operation is suffice for an TIMM_OSAL_EVENT_OR eOperation
				 */
				isolatedFlags =
				    plEvent->eFlags & uRequestedEvents;

				/*Check if it is the AND operation. If yes then, all the flags must match */
				if (and_operation)
				{
					isolatedFlags =
					    (isolatedFlags ==
					    uRequestedEvents);
				}

			}
		}
	}

	/*If we have got the required combination of bits, we will have to reset the eFlags if CONSUME is mentioned
	 *in the eOperations
	 */
	if (isolatedFlags && ((eOperation == TIMM_OSAL_EVENT_AND_CONSUME) ||
		(eOperation == TIMM_OSAL_EVENT_OR_CONSUME)))
	{
		plEvent->eFlags = 0;
	}

	/*Manually unlock the mutex */
	if (SUCCESS != pthread_mutex_unlock(&(plEvent->mutex)))
	{
		TIMM_OSAL_Error("Event Retrieve: Mutex Unlock failed !");
		bReturnStatus = TIMM_OSAL_ERR_UNKNOWN;
	}

      EXIT:
	return bReturnStatus;

}
