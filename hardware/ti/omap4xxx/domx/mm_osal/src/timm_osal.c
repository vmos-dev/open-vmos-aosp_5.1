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
*   @file  tiimm_osal.c
*   This file contains methods that provides the functionality
*   initializing/deinitializing the osal.
*
*  @path \
*
*/
/* -------------------------------------------------------------------------- */
/* =========================================================================
 *!
 *! Revision History
 *! ===================================
 *! 20-Oct-2008 Maiya ShreeHarsha:Linux specific changes
 *! 0.1: Created the first draft version, ksrini@ti.com
 * ========================================================================= */

/******************************************************************************
* Includes
******************************************************************************/
#include "timm_osal_types.h"
#include "timm_osal_error.h"
#include "timm_osal_memory.h"
/*#include "timm_osal_trace.h"*/


/******************************************************************************
* Function Prototypes
******************************************************************************/

/****************************************************************
*  PRIVATE DECLARATIONS  : only used in this file
****************************************************************/
/*--------data declarations -----------------------------------*/


/* ========================================================================== */
/**
* @fn TIMM_OSAL_Init function initilize the osal with initial settings.
*
* @return  TIMM_OSAL_ERR_NONE if successful
*               !TIMM_OSAL_ERR_NONE if an error occurs
*/
/* ========================================================================== */

TIMM_OSAL_ERRORTYPE TIMM_OSAL_Init(void)
{
	TIMM_OSAL_ERRORTYPE bReturnStatus = TIMM_OSAL_ERR_NONE;
	return bReturnStatus;
}



/* ========================================================================== */
/**
* @fn TIMM_OSAL_Init function de-initilize the osal.
*
* @return  TIMM_OSAL_ERR_NONE if successful
*               !TIMM_OSAL_ERR_NONE if an error occurs
*/
/* ========================================================================== */

TIMM_OSAL_ERRORTYPE TIMM_OSAL_Deinit(void)
{
	TIMM_OSAL_ERRORTYPE bReturnStatus = TIMM_OSAL_ERR_NONE;
	return bReturnStatus;
}
