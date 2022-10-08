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

/*
 * This header file corresponds to V1.0 of the GlobalPlatform 
 * TEE Client API Specification
 */
#ifndef   __TEE_CLIENT_API_H__
#define   __TEE_CLIENT_API_H__

#include "s_type.h"
#include "s_error.h"

#ifndef TEEC_EXPORT
#define TEEC_EXPORT
#endif

/* The header tee_client_api_imp.h must define implementation-dependent
   types, constants and macros.
   
   The implementation-dependent types are:
     - TEEC_Context_IMP
     - TEEC_Session_IMP
     - TEEC_SharedMemory_IMP
     - TEEC_Operation_IMP

   The implementation-dependent constants are:
     - TEEC_CONFIG_SHAREDMEM_MAX_SIZE  
   The implementation-dependent macros are:
     - TEEC_PARAM_TYPES
*/
#include "tee_client_api_imp.h"

/* Type definitions */
typedef struct TEEC_Context
{
   TEEC_Context_IMP imp;
} TEEC_Context;

typedef struct TEEC_Session
{
   TEEC_Session_IMP imp; 
} TEEC_Session;

typedef struct TEEC_SharedMemory
{
    void*    buffer;
    size_t   size;
    uint32_t flags;
    TEEC_SharedMemory_IMP imp;
} TEEC_SharedMemory;

typedef struct
{
    void*     buffer;
    size_t    size;
} TEEC_TempMemoryReference;

typedef struct
{
    TEEC_SharedMemory * parent;
    size_t    size;
    size_t    offset;
} TEEC_RegisteredMemoryReference;

typedef struct
{
    uint32_t   a;
    uint32_t   b;
} TEEC_Value;

typedef union
{
   TEEC_TempMemoryReference        tmpref;
   TEEC_RegisteredMemoryReference  memref;
   TEEC_Value                      value;
} TEEC_Parameter;

typedef struct TEEC_Operation
{
    volatile uint32_t    started;
    uint32_t             paramTypes;
    TEEC_Parameter       params[4];   
    
    TEEC_Operation_IMP   imp;
} TEEC_Operation;


#define TEEC_ORIGIN_API                      0x00000001
#define TEEC_ORIGIN_COMMS                    0x00000002
#define TEEC_ORIGIN_TEE                      0x00000003
#define TEEC_ORIGIN_TRUSTED_APP              0x00000004

#define TEEC_MEM_INPUT                       0x00000001
#define TEEC_MEM_OUTPUT                      0x00000002

#define TEEC_NONE                     0x0
#define TEEC_VALUE_INPUT              0x1
#define TEEC_VALUE_OUTPUT             0x2
#define TEEC_VALUE_INOUT              0x3
#define TEEC_MEMREF_TEMP_INPUT        0x5
#define TEEC_MEMREF_TEMP_OUTPUT       0x6
#define TEEC_MEMREF_TEMP_INOUT        0x7
#define TEEC_MEMREF_WHOLE             0xC
#define TEEC_MEMREF_PARTIAL_INPUT     0xD
#define TEEC_MEMREF_PARTIAL_OUTPUT    0xE
#define TEEC_MEMREF_PARTIAL_INOUT     0xF

#define TEEC_LOGIN_PUBLIC                    0x00000000
#define TEEC_LOGIN_USER                      0x00000001
#define TEEC_LOGIN_GROUP                     0x00000002
#define TEEC_LOGIN_APPLICATION               0x00000004
#define TEEC_LOGIN_USER_APPLICATION          0x00000005
#define TEEC_LOGIN_GROUP_APPLICATION         0x00000006

TEEC_Result TEEC_EXPORT TEEC_InitializeContext(
    const char*   name,
    TEEC_Context* context);

void TEEC_EXPORT TEEC_FinalizeContext(
    TEEC_Context* context);

TEEC_Result TEEC_EXPORT TEEC_RegisterSharedMemory(
    TEEC_Context*      context,
    TEEC_SharedMemory* sharedMem);

TEEC_Result TEEC_EXPORT TEEC_AllocateSharedMemory(
    TEEC_Context*      context,
    TEEC_SharedMemory* sharedMem);

void TEEC_EXPORT TEEC_ReleaseSharedMemory (
    TEEC_SharedMemory* sharedMem);

TEEC_Result TEEC_EXPORT TEEC_OpenSession (
    TEEC_Context*    context,
    TEEC_Session*    session,
    const TEEC_UUID* destination,
    uint32_t         connectionMethod,
    void*            connectionData,
    TEEC_Operation*  operation,
    uint32_t*        errorOrigin);

void TEEC_EXPORT TEEC_CloseSession (
    TEEC_Session* session);

TEEC_Result TEEC_EXPORT TEEC_InvokeCommand(
    TEEC_Session*     session,
    uint32_t          commandID,
    TEEC_Operation*   operation,
    uint32_t*         errorOrigin);

void TEEC_EXPORT TEEC_RequestCancellation(
    TEEC_Operation* operation);

#include "tee_client_api_ex.h"

#endif /* __TEE_CLIENT_API_H__ */
