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
 * Implementation Notes:
 *
 * This API is NOT thread-safe. Indeed this Cryptoki implementation
 * only supports option 1 defined in PKCS#11, section 6.5.2:
 * "The application can specify that it will not be accessing the library concurrently
 * from multiple threads, and so the library need not worry about performing any type
 * of locking for the sake of thread-safety."
 */

#include "pkcs11_internal.h"

/* ------------------------------------------------------------------------
    System Service UUID
------------------------------------------------------------------------- */
const TEEC_UUID SERVICE_UUID = SERVICE_SYSTEM_UUID;

/* ------------------------------------------------------------------------
    Definition of the global TEE Context
------------------------------------------------------------------------- */
TEEC_Context g_sContext;
/* A mutex that protects the access to the global context and to the
   g_bContextRefCounter flag */
LIB_MUTEX g_sContextMutex = LIB_MUTEX_INITIALIZER;
/* Whether the context has already been initialized or not */
uint32_t  g_nContextRefCounter = 0;

bool g_bCryptokiInitialized = false;

/* ------------------------------------------------------------------------
   Internal global TEE context management
------------------------------------------------------------------------- */

void stubMutexLock(void)
{
   libMutexLock(&g_sContextMutex);
}

void stubMutexUnlock(void)
{
   libMutexUnlock(&g_sContextMutex);
}

/* This API must be protected by stubMutexLock/Unlock */
TEEC_Result stubInitializeContext(void)
{
   TEEC_Result nTeeError;

   if (g_nContextRefCounter)
   {
      g_nContextRefCounter ++;
      return TEEC_SUCCESS;
   }

   nTeeError = TEEC_InitializeContext(NULL, &g_sContext);
   if (nTeeError == TEEC_SUCCESS)
   {
      g_nContextRefCounter = 1;
   }

   return nTeeError;
}

/* This API must be protected by stubMutexLock/Unlock */
void stubFinalizeContext(void)
{
   if (g_nContextRefCounter > 0)
   {
      g_nContextRefCounter --;
   }

   if (g_nContextRefCounter == 0)
   {
      TEEC_FinalizeContext(&g_sContext);
      memset(&g_sContext, 0, sizeof(TEEC_Context));
   }
}


/* ------------------------------------------------------------------------
                          Internal monitor management
------------------------------------------------------------------------- */
/**
* Check that hSession is a valid primary session,
* or a valid secondary session attached to a valid primary session.
*
* input:
*   S_HANDLE hSession: the session handle to check
* output:
*   bool* pBoolIsPrimarySession: a boolean set to true if the session is primary,
*             set to false if the session if the session is secondary
*   returned boolean: set to true iff :
*              - either hSession is a valid primary session
*              - or hSession is a valid secondary session attached to a valid primary session
**/
bool ckInternalSessionIsOpenedEx(S_HANDLE hSession, bool* pBoolIsPrimarySession)
{
   PPKCS11_SESSION_CONTEXT_HEADER   pHeader = (PPKCS11_SESSION_CONTEXT_HEADER)hSession;
   PPKCS11_PRIMARY_SESSION_CONTEXT  pSession = NULL;

   if ((pHeader == NULL) || (pHeader->nMagicWord != PKCS11_SESSION_MAGIC))
   {
      return FALSE;
   }
   if (pHeader->nSessionTag == PKCS11_PRIMARY_SESSION_TAG) /* primary session */
   {
      pSession = (PPKCS11_PRIMARY_SESSION_CONTEXT)pHeader;

      *pBoolIsPrimarySession = true;

      /* check that primary session is valid */
      return (pSession->hCryptoSession != CK_INVALID_HANDLE);
   }
   else if (pHeader->nSessionTag == PKCS11_SECONDARY_SESSION_TAG) /*secondary session */
   {
      PPKCS11_SECONDARY_SESSION_CONTEXT pSecSession = (PPKCS11_SECONDARY_SESSION_CONTEXT)pHeader;

      *pBoolIsPrimarySession = false;

      /* check that primary session is still valid */
      pSession = pSecSession->pPrimarySession;
      if (  (pSession == NULL) ||
            (pSession->sHeader.nMagicWord != PKCS11_SESSION_MAGIC) ||
            (pSession->sHeader.nSessionTag != PKCS11_PRIMARY_SESSION_TAG))
      {
         return FALSE;
      }

      if (pSession->hCryptoSession == CK_INVALID_HANDLE)
      {
         return FALSE;
      }

      /* check that secondary session is valid */
      return (pSecSession->hSecondaryCryptoSession != CK_INVALID_HANDLE);
   }
   else
   {
     return FALSE;
   }
}

/* ------------------------------------------------------------------------
                          Internal error management
------------------------------------------------------------------------- */

CK_RV ckInternalTeeErrorToCKError(TEEC_Result nError)
{
   switch (nError)
   {
      case TEEC_SUCCESS:
         return CKR_OK;

      case TEEC_ERROR_BAD_PARAMETERS:
      case TEEC_ERROR_BAD_FORMAT:
         return CKR_ARGUMENTS_BAD;
      case TEEC_ERROR_OUT_OF_MEMORY:
         return CKR_HOST_MEMORY;
      case TEEC_ERROR_ACCESS_DENIED:
         return CKR_TOKEN_NOT_PRESENT;
      default:
         return CKR_DEVICE_ERROR;
   }
}

/* ------------------------------------------------------------------------
                          Public Functions
------------------------------------------------------------------------- */
CK_RV PKCS11_EXPORT C_Initialize(CK_VOID_PTR pInitArgs)
{
   CK_RV       nErrorCode;
   TEEC_Result nTeeError;

   if (pInitArgs != NULL_PTR)
   {
      return CKR_ARGUMENTS_BAD;
   }

   stubMutexLock();
   if (g_bCryptokiInitialized)
   {
      nErrorCode = CKR_CRYPTOKI_ALREADY_INITIALIZED;
   }
   else
   {
      nTeeError = stubInitializeContext();
      if (nTeeError == TEEC_SUCCESS)
      {
         g_bCryptokiInitialized = true;
      }
      nErrorCode = ckInternalTeeErrorToCKError(nTeeError);
   }
   stubMutexUnlock();

   return nErrorCode;
}

CK_RV PKCS11_EXPORT C_Finalize(CK_VOID_PTR pReserved)
{
   CK_RV nErrorCode;

   if (pReserved != NULL_PTR)
   {
      return CKR_ARGUMENTS_BAD;
   }

   stubMutexLock();
   if (g_bCryptokiInitialized)
   {
      stubFinalizeContext();
      g_bCryptokiInitialized = false;
      nErrorCode = CKR_OK;
   }
   else
   {
      nErrorCode = CKR_CRYPTOKI_NOT_INITIALIZED;
   }
   stubMutexUnlock();

   return nErrorCode;
}

static const CK_INFO sImplementationInfo =
{
   {2, 20},         /* cryptokiVersion, spec 2.20 */
   "Trusted Logic", /* manufacturerID */
   0,               /* flags */
   "PKCS#11",       /* libraryDescription */
   {3, 0}           /* libraryVersion */
};

CK_RV PKCS11_EXPORT C_GetInfo(CK_INFO_PTR pInfo)
{
   if (!g_bCryptokiInitialized)
   {
      return CKR_CRYPTOKI_NOT_INITIALIZED;
   }
   if (pInfo == NULL_PTR)
   {
      return CKR_ARGUMENTS_BAD;
   }

   memcpy(pInfo, &sImplementationInfo, sizeof(CK_INFO));
   return CKR_OK;
}
