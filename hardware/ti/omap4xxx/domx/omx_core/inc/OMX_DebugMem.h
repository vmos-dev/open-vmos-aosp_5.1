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

#ifndef OMX_DEBUGMEM_H
#define OMX_DEBUGMEM_H

#include <stdlib.h> // for size_t

#if OMX_DEBUG
void* OMX_DebugMem_calloc(size_t num, size_t size,
                           const char* file, const char* func, int line);
void* OMX_DebugMem_malloc(size_t size,
                           const char* file, const char* func, int line);
void* OMX_DebugMem_realloc(void *ptr, size_t size,
                           const char* file, const char* func, int line);
void  OMX_DebugMem_free(void* ptr,
                         const char* file, const char* func, int line);
int   OMX_DebugMem_validate(void *ptr,
                         const char* file, const char* func, int line);

void OMX_DebugMem_dump(const char *file, const char *func, int line);
#endif

#define OMX_MASK_MEMORY    0x00010000 /** One of the reserved bits from OMX_MASK_USERMASK */

#define MEM_DEBUG_HANDLE_DESCR "DebugMemory"

#if OMX_DEBUG
    #define malloc(x)    OMX_DebugMem_malloc(x,__FILE__,__FUNCTION__,__LINE__)
    #define calloc(n,s)  OMX_DebugMem_calloc(n,s,__FILE__,__FUNCTION__,__LINE__)
    #define realloc(x,s) OMX_DebugMem_realloc(x,s,__FILE__,__FUNCTION__,__LINE__)
    #define free(x)      OMX_DebugMem_free(x,__FILE__,__FUNCTION__ ,__LINE__)
    #define validate(x)  OMX_DebugMem_validate(x, __FILE__,__FUNCTION__ ,__LINE__)
    #define dump()      OMX_DebugMem_dump( __FILE__,__FUNCTION__ ,__LINE__)
#else
    #define validate(x)
    #define dump()
#endif


#endif

