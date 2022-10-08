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
 * This header file contains extensions to the TEE Client API that are
 * specific to the Trusted Foundations implementations
 */
#ifndef   __TEE_CLIENT_API_EX_H__
#define   __TEE_CLIENT_API_EX_H__

/* Implementation defined Login types  */
#define TEEC_LOGIN_AUTHENTICATION      0x80000000
#define TEEC_LOGIN_PRIVILEGED          0x80000002

/* Type definitions */

typedef struct
{
   uint32_t x;
   uint32_t y;
}
TEEC_TimeLimit;

typedef struct
{
   char apiDescription[65];
   char commsDescription[65];
   char TEEDescription[65];
}
TEEC_ImplementationInfo;

typedef struct
{
   uint32_t pageSize;
   uint32_t tmprefMaxSize;
   uint32_t sharedMemMaxSize;
   uint32_t nReserved3;
   uint32_t nReserved4;
   uint32_t nReserved5;
   uint32_t nReserved6;
   uint32_t nReserved7;
} 
TEEC_ImplementationLimits;

void TEEC_EXPORT TEEC_GetImplementationInfo(
   TEEC_Context*            context,
   TEEC_ImplementationInfo* description);

void TEEC_EXPORT TEEC_GetImplementationLimits(
   TEEC_ImplementationLimits* limits);

void TEEC_EXPORT TEEC_GetTimeLimit(
    TEEC_Context*    context,
    uint32_t         timeout,
    TEEC_TimeLimit*  timeLimit);

TEEC_Result TEEC_EXPORT TEEC_OpenSessionEx (
    TEEC_Context*         context,
    TEEC_Session*         session,
    const TEEC_TimeLimit* timeLimit,
    const TEEC_UUID*      destination,
    uint32_t              connectionMethod,
    void*                 connectionData,
    TEEC_Operation*       operation,
    uint32_t*             errorOrigin);

TEEC_Result TEEC_EXPORT TEEC_InvokeCommandEx(
    TEEC_Session*         session,
    const TEEC_TimeLimit* timeLimit,
    uint32_t              commandID,
    TEEC_Operation*       operation,
    uint32_t*             errorOrigin);

TEEC_Result TEEC_EXPORT TEEC_ReadSignatureFile(
   void**    ppSignatureFile,
   uint32_t* pnSignatureFileLength);

#endif /* __TEE_CLIENT_API_EX_H__ */
