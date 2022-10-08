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
*  @file timm_osal_types.h
*  The timm_osal_types header file defines the primative osal type definitions.
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

#ifndef _TIMM_OSAL_TYPES_H_
#define _TIMM_OSAL_TYPES_H_

#ifdef __cplusplus
extern "C"
{
#endif				/* __cplusplus */



#ifdef __KERNEL__
#include <linux/types.h>
#else
#include <stdint.h>
#endif

	typedef int8_t TIMM_OSAL_S8;	/*INT8 */
	typedef int16_t TIMM_OSAL_S16;	/*INT16 */
	typedef int32_t TIMM_OSAL_S32;	/*INT32 */

#define TIMM_OSAL_INT8_MIN 0xFF
#define TIMM_OSAL_INT8_MAX 0x7F

#define TIMM_OSAL_INT16_MIN 0xFFFF
#define TIMM_OSAL_INT16_MAX 0x7FFF

#define TIMM_OSAL_INT32_MIN 0xFFFFFFFF
#define TIMM_OSAL_INT32_MAX 0x7FFFFFFF

	typedef uint8_t TIMM_OSAL_U8;	/*UINT8 */
	typedef uint16_t TIMM_OSAL_U16;	/*UINT16 */
	typedef uint32_t TIMM_OSAL_U32;	/*UINT32 */

#define TIMM_OSAL_UINT8_MIN 0
#define TIMM_OSAL_UINT8_MAX 0xFF

#define TIMM_OSAL_UINT16_MIN 0
#define TIMM_OSAL_UINT16_MAX 0xFFFF

#define TIMM_OSAL_UINT32_MIN 0
#define TIMM_OSAL_UINT32_MAX 0xFFFFFFFF


	typedef char TIMM_OSAL_CHAR;
	  /*CHAR*/ typedef void *TIMM_OSAL_HANDLE;
	typedef void *TIMM_OSAL_PTR;

	typedef enum TIMM_OSAL_BOOL
	{
		TIMM_OSAL_FALSE = 0,
		TIMM_OSAL_TRUE = !TIMM_OSAL_FALSE,
		TIMM_OSAL_BOOL_MAX = 0x7FFFFFFF
	} TIMM_OSAL_BOOL;

#define TIMM_OSAL_SUSPEND     0xFFFFFFFFUL
#define TIMM_OSAL_NO_SUSPEND  0x0
#define TIMM_OSAL_TIMED_OUT   0x7FFFFFFFUL


#define SUCCESS 0
#define NO_SUCCESS -1

#define ERROR 1
/*
#define TRUE 0
#define FALSE 1
*/
#define URGENT_MESSAGE 2
#define NORMAL_MESSAGE 1


#define TIMM_OSAL_NULL 0

#ifdef __cplusplus
}
#endif				/* __cplusplus */

#endif				/* _TIMM_OSAL_TYPES_H_ */
