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
*   @file  timm_osal_trace.c
*   This file contains methods that provides the functionality
*   for logging errors/warings/information/etc.
*
*  @path \
*
*/
/* -------------------------------------------------------------------------- */
/* =========================================================================
 *!
 *! Revision History
 *! ===================================
 *!
 * ========================================================================= */

/******************************************************************************
* Includes
******************************************************************************/

/*#include "typedefs.h"*/
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "timm_osal_trace.h"

#ifdef _Android
#define LOG_TAG "DOMX"
#include <utils/Log.h>
#include <cutils/properties.h>
#endif

/**
* The OSAL debug trace detail can be set at compile time by defining the flag
* TIMM_OSAL_DEBUG_TRACE_DETAIL=<Details>
* detail - 0 - no detail
*          1 - function name
*          2 - function name, line number
* Prefix is added to every debug trace message
*/
#ifndef TIMM_OSAL_DEBUG_TRACE_DETAIL
#define TIMM_OSAL_DEBUG_TRACE_DETAIL 2
#endif

#define DEFAULT_TRACE_LEVEL TIMM_OSAL_TRACE_LEVEL_ERROR

static int trace_level = -1;

/* strip out leading ../ stuff that happens to __FILE__ for out-of-tree builds */
static const char *simplify_path(const char *file)
{
	while (file)
	{
		char c = file[0];
		if ((c != '.') && (c != '/') && (c != '\\'))
			break;
		file++;
	}
	return file;
}

void TIMM_OSAL_UpdateTraceLevel(void)
{
	char *val = getenv("TIMM_OSAL_DEBUG_TRACE_LEVEL");

	if (val)
	{
		trace_level = strtol(val, NULL, 0);
	}
	else
	{
#ifdef _Android
		char value[PROPERTY_VALUE_MAX];
		int val;

		property_get("debug.domx.trace_level", value, "0");
		val = atoi(value);
		if ( (!val) || (val < 0) )
		{
			trace_level = DEFAULT_TRACE_LEVEL;
		}
		else
			trace_level = val;
#else
		trace_level = DEFAULT_TRACE_LEVEL;
#endif
	}
}

void __TIMM_OSAL_TraceFunction(const __TIMM_OSAL_TRACE_LOCATION * loc,
    const char *fmt, ...)
{
	if (trace_level == -1)
	{
		char *val = getenv("TIMM_OSAL_DEBUG_TRACE_LEVEL");
		trace_level =
		    val ? strtol(val, NULL, 0) : DEFAULT_TRACE_LEVEL;
	}

	if (trace_level >= loc->level)
	{
		va_list ap;

		va_start(ap, fmt);	/* make ap point to first arg after 'fmt' */

#ifdef _Android

#if 0 // Original for reference
#if ( TIMM_OSAL_DEBUG_TRACE_DETAIL > 1 )
		ALOGD("%s:%d\t%s()\t", simplify_path(loc->file), loc->line,
		    loc->function);
#endif
#else // Prints function_name for ERROR, WARNING and ENTRY/EXIT
	if ( (loc->level == TIMM_OSAL_TRACE_LEVEL_ERROR) || (loc->level == TIMM_OSAL_TRACE_LEVEL_WARNING) || (loc->level == TIMM_OSAL_TRACE_LEVEL_ENTERING) )
		ALOGD("%s:%d\t%s()\t", simplify_path(loc->file), loc->line,
		    loc->function);
#endif

		char string[1000];
		vsprintf(string, fmt, ap);
		ALOGD("%s",string);

#else

#if 0 // Original for reference
#if ( TIMM_OSAL_DEBUG_TRACE_DETAIL > 1 )
		printf("%s:%d\t%s()\t", simplify_path(loc->file), loc->line,
		    loc->function);
#endif
#else // Prints function_name for ERROR, WARNING and ENTRY/EXIT
		if ( (loc->level == 1) || (loc->level == 2) || (loc->level == 5) )
			printf("%s:%d\t%s()\t", simplify_path(loc->file), loc->line,
		    loc->function);
#endif

		vprintf(fmt, ap);

#endif

		va_end(ap);
	}
}
