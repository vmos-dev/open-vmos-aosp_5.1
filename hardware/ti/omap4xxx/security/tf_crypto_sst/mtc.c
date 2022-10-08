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

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define MTC_EXPORTS
#include "mtc.h"

/* Included for the TEE management */
#include "pkcs11_internal.h"


/*------------------------------------------------------------------------------
   Defines
------------------------------------------------------------------------------*/

/**
 * The magic word.
 */
#define MTC_SESSION_MAGIC  ( (uint32_t)0x4D544300 )   /* "MTC\0" */

/**
 * The MTC session context
 */
typedef struct
{
   /* Magic word, must be set to {MTC_SESSION_MAGIC}. */
   uint32_t    nMagicWord;

   /* MTC Identifier */
   uint32_t nCounterIdentifier;

   /* TEEC session and cryptoki session */
   TEEC_Session sSession;
   uint32_t     hCryptoSession;

} MTC_SESSION_CONTEXT;


static bool g_bMTCInitialized = false;


/*------------------------------------------------------------------------------
   Static functions
------------------------------------------------------------------------------*/

static S_RESULT static_getMonotonicCounter(S_HANDLE hCounter,
                                           S_MONOTONIC_COUNTER_VALUE* psValue,
                                           bool bIncrement)
{
   TEEC_Result          nError;
   TEEC_Operation       sOperation;
   MTC_SESSION_CONTEXT* pSession = NULL;
   uint32_t             nCommandID;

   if (!g_bMTCInitialized)
   {
      return S_ERROR_BAD_STATE;
   }

   pSession = (MTC_SESSION_CONTEXT *)hCounter;
   if ((pSession == NULL) || (pSession->nMagicWord != MTC_SESSION_MAGIC))
   {
      return S_ERROR_BAD_PARAMETERS;
   }

   if (bIncrement)
   {
      nCommandID = SERVICE_SYSTEM_PKCS11_INCREMENT_MTC_COMMAND_ID;
   }
   else
   {
      nCommandID = SERVICE_SYSTEM_PKCS11_GET_MTC_COMMAND_ID;
   }

   sOperation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INOUT, TEEC_NONE, TEEC_NONE, TEEC_NONE);
   sOperation.params[0].value.a = pSession->nCounterIdentifier;
   sOperation.params[0].value.b = 0;
   nError = TEEC_InvokeCommand(&pSession->sSession,
                            (pSession->hCryptoSession << 16 ) |
                              (nCommandID & 0x00007FFF),
                            &sOperation,
                            NULL);

   psValue->nLow  = sOperation.params[0].value.a;
   psValue->nHigh = sOperation.params[0].value.b;

   return nError;
}

/*------------------------------------------------------------------------------
   API
------------------------------------------------------------------------------*/

MTC_EXPORT S_RESULT SMonotonicCounterInit(void)
{
   TEEC_Result nTeeError;

   stubMutexLock();
   if (g_bMTCInitialized)
   {
      nTeeError = TEEC_SUCCESS;
   }
   else
   {
      nTeeError = stubInitializeContext();
      if (nTeeError == TEEC_SUCCESS)
      {
         g_bMTCInitialized = true;
      }
   }
   stubMutexUnlock();

   return nTeeError;
}

MTC_EXPORT void SMonotonicCounterTerminate(void)
{
   stubMutexLock();
   if (g_bMTCInitialized)
   {
      stubFinalizeContext();
      g_bMTCInitialized = false;
   }
   stubMutexUnlock();
}

MTC_EXPORT S_RESULT SMonotonicCounterOpen(
                 uint32_t nCounterIdentifier,
                 OUT S_HANDLE* phCounter)
{
   TEEC_Result                nError;
   TEEC_Operation             sOperation;
   MTC_SESSION_CONTEXT*       pSession = NULL;
   S_MONOTONIC_COUNTER_VALUE  nCounterValue;

   if (phCounter == NULL)
   {
      return S_ERROR_BAD_PARAMETERS;
   }

   *phCounter = S_HANDLE_NULL;

   if (!g_bMTCInitialized)
   {
      return S_ERROR_BAD_STATE;
   }

   if (nCounterIdentifier != S_MONOTONIC_COUNTER_GLOBAL)
   {
      return S_ERROR_ITEM_NOT_FOUND;
   }

   pSession = (MTC_SESSION_CONTEXT*)malloc(sizeof(MTC_SESSION_CONTEXT));
   if (pSession == NULL)
   {
      return S_ERROR_OUT_OF_MEMORY;
   }
   memset(pSession, 0, sizeof(MTC_SESSION_CONTEXT));
   pSession->nMagicWord = MTC_SESSION_MAGIC;

   /* Open a TEE session with the system service */
   nError = TEEC_OpenSession(&g_sContext,
                             &pSession->sSession,
                             &SERVICE_UUID,
                             TEEC_LOGIN_PUBLIC,
                             NULL,
                             NULL, /* No operation parameters */
                             NULL);
   if (nError != TEEC_SUCCESS)
   {
      goto error;
   }

   /* Open a cryptoki session */
   sOperation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INOUT, TEEC_NONE, TEEC_NONE, TEEC_NONE);
   sOperation.params[0].value.a = CKV_TOKEN_SYSTEM_SHARED;
   sOperation.params[0].value.b = CKF_RW_SESSION | CKF_SERIAL_SESSION;
   nError = TEEC_InvokeCommand(&pSession->sSession,
                               SERVICE_SYSTEM_PKCS11_C_OPEN_SESSION_COMMAND_ID & 0x00007FFF,
                               &sOperation,
                               NULL);
   if (nError != TEEC_SUCCESS)
   {
      TEEC_CloseSession(&pSession->sSession);
      goto error;
   }

   pSession->hCryptoSession = sOperation.params[0].value.a;
   pSession->nCounterIdentifier = nCounterIdentifier;

   nError = SMonotonicCounterGet((S_HANDLE)pSession, &nCounterValue);
   if (nError != TEEC_SUCCESS)
   {
      SMonotonicCounterClose((S_HANDLE)pSession);
      return nError;
   }

   *phCounter = (S_HANDLE)pSession;

   return TEEC_SUCCESS;

error:
   free(pSession);
   return nError;
}

MTC_EXPORT void SMonotonicCounterClose(S_HANDLE hCounter)
{
   MTC_SESSION_CONTEXT* pSession;

   if (!g_bMTCInitialized)
   {
      return;
   }

   pSession = (MTC_SESSION_CONTEXT *)hCounter;
   if ((pSession == NULL) || (pSession->nMagicWord != MTC_SESSION_MAGIC))
   {
      return;
   }

   (void)TEEC_InvokeCommand(&pSession->sSession,
                            (pSession->hCryptoSession << 16 ) |
                              (SERVICE_SYSTEM_PKCS11_C_CLOSE_SESSION_COMMAND_ID & 0x00007FFF),
                            NULL, /* No operation parameters */
                            NULL);

   TEEC_CloseSession(&pSession->sSession);
   free(pSession);
}

MTC_EXPORT S_RESULT SMonotonicCounterGet(
                 S_HANDLE hCounter,
                 S_MONOTONIC_COUNTER_VALUE* psCurrentValue)
{
   return static_getMonotonicCounter(hCounter, psCurrentValue, false);
}

MTC_EXPORT S_RESULT SMonotonicCounterIncrement(
                 S_HANDLE hCounter,
                 S_MONOTONIC_COUNTER_VALUE* psNewValue)
{
   return static_getMonotonicCounter(hCounter, psNewValue, true);
}
