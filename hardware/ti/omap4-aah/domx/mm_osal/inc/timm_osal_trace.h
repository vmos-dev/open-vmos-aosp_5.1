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
*  @file timm_osal_trace.h
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

#ifndef _TIMM_OSAL_TRACES_H_
#define _TIMM_OSAL_TRACES_H_

#ifdef __cplusplus
extern "C"
{
#endif				/* __cplusplus */

/*******************************************************************************
* Traces
*******************************************************************************/


/******************************************************************************
* Debug Trace defines
******************************************************************************/

	typedef enum TIMM_OSAL_TRACEGRP_TYPE
	{
		TIMM_OSAL_TRACEGRP_SYSTEM = 1,
		TIMM_OSAL_TRACEGRP_OMXBASE = (1 << 1),
		TIMM_OSAL_TRACEGRP_DOMX = (1 << 2),
		TIMM_OSAL_TRACEGRP_OMXVIDEOENC = (1 << 3),
		TIMM_OSAL_TRACEGRP_OMXVIDEODEC = (1 << 4),
		TIMM_OSAL_TRACEGRP_OMXCAM = (1 << 5),
		TIMM_OSAL_TRACEGRP_OMXIMGDEC = (1 << 6),
		TIMM_OSAL_TRACEGRP_DRIVERS = (1 << 7),
		TIMM_OSAL_TRACEGRP_SIMCOPALGOS = (1 << 8)
	} TIMM_OSAL_TRACEGRP;

	typedef enum TIMM_OSAL_TRACE_LEVEL_TYPE
	{
		TIMM_OSAL_TRACE_LEVEL_ERROR = 1,
		TIMM_OSAL_TRACE_LEVEL_WARNING = 2,
		TIMM_OSAL_TRACE_LEVEL_PROFILING = 3,
		TIMM_OSAL_TRACE_LEVEL_INFO = 4,
		TIMM_OSAL_TRACE_LEVEL_DEBUG = 5,
		TIMM_OSAL_TRACE_LEVEL_ENTERING = 6,
		TIMM_OSAL_TRACE_LEVEL_EXITING = TIMM_OSAL_TRACE_LEVEL_ENTERING
	} TIMM_OSAL_TRACE_LEVEL;


/**
* The OSAL debug trace level can be set at runtime by defining the environment
* variable TIMM_OSAL_DEBUG_TRACE_LEVEL=<Level>.  The default level is 1
* The debug levels are:
* Level 0 - No trace
* Level 1 - Error   [Errors]
* Level 2 - Warning [Warnings that are useful to know about]
* Level 3 - Profiling [performance analysis trace that must not impact use case perf]
* Level 4 - Info    [General information]
* Level 5 - Debug   [most-commonly used statement for us developers]
* Level 6 - Trace   ["ENTERING <function>" and "EXITING <function>" statements]
*
* Example: if TIMM_OSAL_DEBUG_TRACE_LEVEL=3, then level 1,2 and 3 traces messages
* are enabled.
*/

/**
 * Information about the trace location/type, passed as a single pointer to
 * internal trace function.  Not part of the public API
 */
	typedef struct
	{
		const char *file;
		const char *function;
		const int line;
		const short level;
		const short tracegrp;	/* TIMM_OSAL_TRACEGRP */
	} __TIMM_OSAL_TRACE_LOCATION;


/**
 * Trace level update function.  Updates trace level if env variable
 * or Android property is set. Env variable has precedence over it
 */
	void TIMM_OSAL_UpdateTraceLevel(void);

/**
 * Trace implementation function.  Not part of public API.  Default
 * implementation uses printf(), but you can use LD_PRELOAD to plug in
 * alternative trace system at runtime.
 */
	void __TIMM_OSAL_TraceFunction(const __TIMM_OSAL_TRACE_LOCATION * loc,
	    const char *fmt, ...);

/**
 * Internal trace macro.  Not part of public API.
 */
#define __TIMM_OSAL_Trace(level, tracegrp, fmt, ...)                          \
    do {                                                                      \
        static const __TIMM_OSAL_TRACE_LOCATION loc = {                       \
                __FILE__, __FUNCTION__, __LINE__, (level), (tracegrp)         \
        };                                                                    \
        __TIMM_OSAL_TraceFunction(&loc, fmt"\n", ##__VA_ARGS__);              \
    } while(0)

/**
* TIMM_OSAL_Error() -- Fatal errors
*/
#define TIMM_OSAL_Error(fmt,...)  TIMM_OSAL_ErrorExt(TIMM_OSAL_TRACEGRP_SYSTEM, fmt, ##__VA_ARGS__)

/**
* TIMM_OSAL_Warning() -- Warnings that are useful to know about
*/
#define TIMM_OSAL_Warning(fmt,...)  TIMM_OSAL_WarningExt(TIMM_OSAL_TRACEGRP_SYSTEM, fmt, ##__VA_ARGS__)

/**
* TIMM_OSAL_Profiling() -- performance analysis trace that must not impact use case perf]
*/
#define TIMM_OSAL_Profiling(fmt,...)  TIMM_OSAL_ProfilingExt(TIMM_OSAL_TRACEGRP_SYSTEM, fmt, ##__VA_ARGS__)

/**
* TIMM_OSAL_Info() -- general information
*/
#define TIMM_OSAL_Info(fmt,...)  TIMM_OSAL_InfoExt(TIMM_OSAL_TRACEGRP_SYSTEM, fmt, ##__VA_ARGS__)

/**
* TIMM_OSAL_Debug() -- debug traces, most-commonly useful for developers
*/
#define TIMM_OSAL_Debug(fmt,...)  TIMM_OSAL_DebugExt(TIMM_OSAL_TRACEGRP_SYSTEM, fmt, ##__VA_ARGS__)

/**
* TIMM_OSAL_Entering() -- "ENTERING <function>" statements
* TIMM_OSAL_Exiting()  -- "EXITING <function>" statements
*/
#define TIMM_OSAL_Entering(fmt,...)  TIMM_OSAL_EnteringExt(TIMM_OSAL_TRACEGRP_SYSTEM, fmt, ##__VA_ARGS__)
#define TIMM_OSAL_Exiting(fmt,...)  TIMM_OSAL_ExitingExt(TIMM_OSAL_TRACEGRP_SYSTEM, fmt, ##__VA_ARGS__)

/*******************************************************************************
** New Trace to be used by Applications
*******************************************************************************/

/**
* TIMM_OSAL_ErrorExt() -- Fatal errors
*/
#define TIMM_OSAL_ErrorExt(tracegrp, fmt, ...)  __TIMM_OSAL_Trace(TIMM_OSAL_TRACE_LEVEL_ERROR, tracegrp, "ERROR: "fmt, ##__VA_ARGS__)

/**
* TIMM_OSAL_WarningExt() -- Warnings that are useful to know about
*/
#define TIMM_OSAL_WarningExt(tracegrp, fmt, ...)  __TIMM_OSAL_Trace(TIMM_OSAL_TRACE_LEVEL_WARNING, tracegrp, "WARNING: "fmt, ##__VA_ARGS__)

/**
* TIMM_OSAL_ProfilingExt() -- performance analysis trace that must not impact use case perf]
*/
#define TIMM_OSAL_ProfilingExt(tracegrp, fmt, ...)  __TIMM_OSAL_Trace(TIMM_OSAL_TRACE_LEVEL_PROFILING, tracegrp, "PROFILING: "fmt, ##__VA_ARGS__)

/**
* TIMM_OSAL_InfoExt() -- general information
*/
#define TIMM_OSAL_InfoExt(tracegrp, fmt, ...)  __TIMM_OSAL_Trace(TIMM_OSAL_TRACE_LEVEL_INFO, tracegrp, "INFO: "fmt, ##__VA_ARGS__)

/**
* TIMM_OSAL_DebugExt() -- most-commonly used statement for us developers
*/
#define TIMM_OSAL_DebugExt(tracegrp, fmt, ...)  __TIMM_OSAL_Trace(TIMM_OSAL_TRACE_LEVEL_DEBUG, tracegrp, "TRACE: "fmt, ##__VA_ARGS__)

/**
* TIMM_OSAL_EnteringExt() -- "ENTERING <function>" statements
* TIMM_OSAL_ExitingExt()  -- "EXITING <function>" statements
*/
#define TIMM_OSAL_EnteringExt(tracegrp, fmt, ...)  __TIMM_OSAL_Trace(TIMM_OSAL_TRACE_LEVEL_ENTERING, tracegrp, "ENTER: "fmt, ##__VA_ARGS__)
#define TIMM_OSAL_ExitingExt(tracegrp, fmt, ...)  __TIMM_OSAL_Trace(TIMM_OSAL_TRACE_LEVEL_EXITING, tracegrp, "EXIT: "fmt, ##__VA_ARGS__)


#ifdef __cplusplus
}
#endif				/* __cplusplus */

#endif				/* _TIMM_OSAL_TRACES_H_ */
