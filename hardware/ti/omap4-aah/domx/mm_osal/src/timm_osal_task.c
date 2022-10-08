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
*   @file  timm_osal_task.c
*   This file contains methods that provides the functionality
*   for creating/destroying tasks.
*
*  @path \
*
*/
/* -------------------------------------------------------------------------- */
/* =========================================================================
 *!
 *! Revision History
 *! ===================================
 *! 21-Oct-2008 Maiya ShreeHarsha: Linux specific changes
 *! 0.1: Created the first draft version, ksrini@ti.com
 * ========================================================================= */

/******************************************************************************
* Includes
******************************************************************************/

#include <stdio.h>
#include <pthread.h>		/*for POSIX calls */
#include <sched.h>		/*for sched structure */
#include <unistd.h>



#include "timm_osal_types.h"
#include "timm_osal_trace.h"
#include "timm_osal_error.h"
#include "timm_osal_memory.h"
#include "timm_osal_task.h"




/**
* TIMM_OSAL_TASK describe the different task information
*/
typedef struct TIMM_OSAL_TASK
{
	pthread_t threadID;	/*SHM check */
	/* To set the priority and stack size */
	pthread_attr_t ThreadAttr;	/*For setting the priority and stack size */
    /** Name of the task */
	/*    TIMM_OSAL_S8  name[8];*//* eight character plus null char */
    /** Pointer to the task stack memory */
/*    TIMM_OSAL_PTR stackPtr;*/
    /** Size of the task stack */
/*    TIMM_OSAL_S32 stackSize;*/
	/*parameters to the task */
	TIMM_OSAL_U32 uArgc;
	TIMM_OSAL_PTR pArgv;
    /** task priority */
/*    TIMM_OSAL_S32 priority;*/
    /** flag to check if task got created */
	TIMM_OSAL_BOOL isCreated;
} TIMM_OSAL_TASK;


/******************************************************************************
* Function Prototypes
******************************************************************************/


/* ========================================================================== */
/**
* @fn TIMM_OSAL_CreateTask function
*
* @see
*/
/* ========================================================================== */

TIMM_OSAL_ERRORTYPE TIMM_OSAL_CreateTask(TIMM_OSAL_PTR * pTask,
    TIMM_OSAL_TaskProc pFunc,
    TIMM_OSAL_U32 uArgc,
    TIMM_OSAL_PTR pArgv,
    TIMM_OSAL_U32 uStackSize, TIMM_OSAL_U32 uPriority, TIMM_OSAL_S8 * pName)
{

	TIMM_OSAL_ERRORTYPE bReturnStatus = TIMM_OSAL_ERR_UNKNOWN;
	TIMM_OSAL_TASK *pHandle = TIMM_OSAL_NULL;
	struct sched_param sched;
	size_t stackSize;
	*pTask = TIMM_OSAL_NULL;


	/*Task structure allocation */
	pHandle =
	    (TIMM_OSAL_TASK *) TIMM_OSAL_Malloc(sizeof(TIMM_OSAL_TASK), 0, 0,
	    0);
	if (pHandle == TIMM_OSAL_NULL)
	{
		bReturnStatus = TIMM_OSAL_ERR_ALLOC;
		goto EXIT;
	}

	/* Initial cleaning of the task structure */
	TIMM_OSAL_Memset((TIMM_OSAL_PTR) pHandle, 0, sizeof(TIMM_OSAL_TASK));

	/*Arguments for task */
	pHandle->uArgc = uArgc;
	pHandle->pArgv = pArgv;

	pHandle->isCreated = TIMM_OSAL_FALSE;


	if (SUCCESS != pthread_attr_init(&pHandle->ThreadAttr))
	{
		/*TIMM_OSAL_Error("Task Init Attr Init failed!"); */
		goto EXIT;
	}
	/* Updation of the priority and the stack size */

	if (SUCCESS != pthread_attr_getschedparam(&pHandle->ThreadAttr,
		&sched))
	{
		/*TIMM_OSAL_Error("Task Init Get Sched Params failed!"); */
		goto EXIT;
	}

	sched.sched_priority = uPriority;	/* relative to the default priority */
	if (SUCCESS != pthread_attr_setschedparam(&pHandle->ThreadAttr,
		&sched))
	{
		/*TIMM_OSAL_Error("Task Init Set Sched Paramsfailed!"); */
		goto EXIT;
	}

	/*First get the default stack size */
	if (SUCCESS != pthread_attr_getstacksize(&pHandle->ThreadAttr,
		&stackSize))
	{
		/*TIMM_OSAL_Error("Task Init Set Stack Size failed!"); */
		goto EXIT;
	}

	/*Check if requested stack size is larger than the current default stack size */
	if (uStackSize > stackSize)
	{
		stackSize = uStackSize;
		if (SUCCESS != pthread_attr_setstacksize(&pHandle->ThreadAttr,
			stackSize))
		{
			/*TIMM_OSAL_Error("Task Init Set Stack Size failed!"); */
			goto EXIT;
		}
	}



	if (SUCCESS != pthread_create(&pHandle->threadID,
		&pHandle->ThreadAttr, pFunc, pArgv))
	{
		/*TIMM_OSAL_Error ("Create_Task failed !"); */
		goto EXIT;
	}


	/* Task was successfully created */
	pHandle->isCreated = TIMM_OSAL_TRUE;
	*pTask = (TIMM_OSAL_PTR) pHandle;
	bReturnStatus = TIMM_OSAL_ERR_NONE;
    /**pTask = (TIMM_OSAL_PTR *)pHandle;*/

      EXIT:
/*    if((TIMM_OSAL_ERR_NONE != bReturnStatus) && (TIMM_OSAL_NULL != pHandle)) {
       TIMM_OSAL_Free (pHandle->stackPtr);*/
	if ((TIMM_OSAL_ERR_NONE != bReturnStatus))
	{
		TIMM_OSAL_Free(pHandle);
	}
	return bReturnStatus;

}

/* ========================================================================== */
/**
* @fn TIMM_OSAL_DeleteTask
*
* @see
*/
/* ========================================================================== */

TIMM_OSAL_ERRORTYPE TIMM_OSAL_DeleteTask(TIMM_OSAL_PTR pTask)
{
	TIMM_OSAL_ERRORTYPE bReturnStatus = TIMM_OSAL_ERR_UNKNOWN;

	TIMM_OSAL_TASK *pHandle = (TIMM_OSAL_TASK *) pTask;
	void *retVal;

	if ((NULL == pHandle) || (TIMM_OSAL_TRUE != pHandle->isCreated))
	{
		/* this task was never created */
		bReturnStatus = TIMM_OSAL_ERR_PARAMETER;
		goto EXIT;
	}
	if (pthread_attr_destroy(&pHandle->ThreadAttr))
	{
		/*TIMM_OSAL_Error("Delete_Task failed !"); */
		goto EXIT;
	}
	if (pthread_join(pHandle->threadID, &retVal))
	{
		/*TIMM_OSAL_Error("Delete_Task failed !"); */
		goto EXIT;
		/*	bReturnStatus = TIMM_OSAL_ERR_CREATE(TIMM_OSAL_ERR, TIMM_OSAL_COMP_TASK, status);*//*shm to be done */
	}
	bReturnStatus = TIMM_OSAL_ERR_NONE;
	TIMM_OSAL_Free(pHandle);
      EXIT:
	return bReturnStatus;
}


TIMM_OSAL_ERRORTYPE TIMM_OSAL_SleepTask(TIMM_OSAL_U32 mSec)
{
	TIMM_OSAL_S32 nReturn = 0;

#ifdef _POSIX_VERSION_1_
	usleep(1000 * mSec);
#else
	nReturn = usleep(1000 * mSec);
#endif
	if (nReturn == 0)
		return TIMM_OSAL_ERR_NONE;
	else
		return TIMM_OSAL_ERR_UNKNOWN;
}
