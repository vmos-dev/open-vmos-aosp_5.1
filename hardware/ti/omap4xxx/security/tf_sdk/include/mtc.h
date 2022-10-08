/**
 * Copyright(c) 2011 Trusted Logic.   All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name Trusted Logic nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ___MTC_H_INC___
#define ___MTC_H_INC___

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------------------------------------------------------------
   includes
------------------------------------------------------------------------------*/
#include "s_type.h"
#include "s_error.h"

/* Define MTC_EXPORTS during the build of mtc libraries. Do
 * not define it in applications.
 */

#ifdef MTC_EXPORTS
#define MTC_EXPORT S_DLL_EXPORT
#else
#define MTC_EXPORT S_DLL_IMPORT
#endif

/*------------------------------------------------------------------------------
   typedefs
------------------------------------------------------------------------------*/

typedef struct
{
   uint32_t nLow;
   uint32_t nHigh;
}
S_MONOTONIC_COUNTER_VALUE;

/*------------------------------------------------------------------------------
   defines
------------------------------------------------------------------------------*/

#define S_MONOTONIC_COUNTER_GLOBAL        0x00000000

/*------------------------------------------------------------------------------
   API
------------------------------------------------------------------------------*/

S_RESULT MTC_EXPORT SMonotonicCounterInit(void);

void MTC_EXPORT SMonotonicCounterTerminate(void);

S_RESULT MTC_EXPORT SMonotonicCounterOpen(
                 uint32_t nCounterIdentifier,
                 S_HANDLE* phCounter);

void MTC_EXPORT SMonotonicCounterClose(S_HANDLE hCounter);

S_RESULT MTC_EXPORT SMonotonicCounterGet(
                 S_HANDLE hCounter,
                 S_MONOTONIC_COUNTER_VALUE* psCurrentValue);

S_RESULT MTC_EXPORT SMonotonicCounterIncrement(
                 S_HANDLE hCounter,
                 S_MONOTONIC_COUNTER_VALUE* psNewValue);

#ifdef __cplusplus
}
#endif

#endif /*___MTC_H_INC___*/
