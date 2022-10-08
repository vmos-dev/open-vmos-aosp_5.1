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
*  @file profile.h
*  The header file defines the trace events definitions.
*  @path platform\hardware\ti\domx\domx\profiling\inc\
*
*/
/* -------------------------------------------------------------------------- */
/* =========================================================================
 *!
 *! Revision History
 *! ===================================
 *! 1.0: Created the first draft version
 * ========================================================================= */

#ifndef _PROFILE_H_
#define _PROFILE_H_

#ifdef __cplusplus
extern "C"
{
#endif /* #ifdef __cplusplus */

#include <OMX_Types.h>
#include <OMX_Component.h>

#include "omx_rpc_utils.h"
#include "omx_proxy_common.h"

enum KPI_BUFFER_EVENT {
        KPI_BUFFER_ETB = 1,
        KPI_BUFFER_FTB = 2,
        KPI_BUFFER_EBD = 3,
        KPI_BUFFER_FBD = 4
};

/**
 * OMX monitoring component init. Registers component
 */
void KPI_OmxCompInit(OMX_HANDLETYPE hComponent);

/**
 * OMX monitoring component deinit. Unregisters component
 */
void KPI_OmxCompDeinit(OMX_HANDLETYPE hComponent);

/**
 * OMA monitoring buffer event trace. Traces FTB/ETB/FBD/EBD event
 */
void KPI_OmxCompBufferEvent(enum KPI_BUFFER_EVENT event, OMX_HANDLETYPE hComponent, PROXY_BUFFER_INFO* pBuffer);

#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */

#endif /* #ifndef _PROFILE_H_ */
