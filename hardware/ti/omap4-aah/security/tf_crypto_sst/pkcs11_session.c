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
 * The PKCS11 session handle is directly mapped on the
 * Trusted Foundations Software session handle (S_HANDLE).
 */

#include "pkcs11_internal.h"


/* ------------------------------------------------------------------------
                          Public Functions
------------------------------------------------------------------------- */


CK_RV PKCS11_EXPORT C_OpenSession(CK_SLOT_ID              slotID,        /* the slot's ID */
                                    CK_FLAGS              flags,         /* defined in CK_SESSION_INFO */
                                    CK_VOID_PTR           pApplication,  /* pointer passed to callback */
                                    CK_NOTIFY             Notify,        /* notification callback function */
                                    CK_SESSION_HANDLE*    phSession)     /* receives new session handle */
{
   CK_RV                   nErrorCode = CKR_OK;
   uint32_t                nErrorOrigin = TEEC_ORIGIN_API;
   TEEC_Result             nTeeError;
   TEEC_Operation          sOperation;
   PPKCS11_PRIMARY_SESSION_CONTEXT   pSession = NULL;
   PPKCS11_SECONDARY_SESSION_CONTEXT pSecondarySession = NULL;
   uint32_t                nLoginType;
   uint32_t                nLoginData = 0;
   void*                   pLoginData = NULL;
   bool                    bIsPrimarySession;
   char*                   pSignatureFile = NULL;
   uint32_t                nSignatureFileLen = 0;
   uint8_t                 nParamType3 = TEEC_NONE;

   /* Prevent the compiler from complaining about unused parameters */
   do{(void)pApplication;}while(0);
   do{(void)Notify;}while(0);

   if (phSession == NULL)
   {
      return CKR_ARGUMENTS_BAD;
   }

      /* Check Cryptoki is initialized */
   if (!g_bCryptokiInitialized)
   {
      return CKR_CRYPTOKI_NOT_INITIALIZED;
   }

   if ((flags & CKVF_OPEN_SUB_SESSION) == 0)
   {
      *phSession = CK_INVALID_HANDLE;

      /*
      * Allocate the session context
      */
      pSession = (PPKCS11_PRIMARY_SESSION_CONTEXT)malloc(sizeof(PKCS11_PRIMARY_SESSION_CONTEXT));
      if (pSession == NULL)
      {
         return CKR_DEVICE_MEMORY;
      }

      pSession->sHeader.nMagicWord  = PKCS11_SESSION_MAGIC;
      pSession->sHeader.nSessionTag = PKCS11_PRIMARY_SESSION_TAG;
      memset(&pSession->sSession, 0, sizeof(TEEC_Session));
      pSession->sSecondarySessionTable.pRoot = NULL_PTR;

      /* The structure must be initialized first (in a portable manner)
         to make it work on Win32 */
      memset(&pSession->sSecondarySessionTableMutex, 0,
               sizeof(pSession->sSecondarySessionTableMutex));
      libMutexInit(&pSession->sSecondarySessionTableMutex);

      switch (slotID)
      {
      case CKV_TOKEN_SYSTEM_SHARED:
      case CKV_TOKEN_USER_SHARED:
         nLoginType = TEEC_LOGIN_PUBLIC;
         break;

      case CKV_TOKEN_SYSTEM:
      case CKV_TOKEN_USER:
      default:
         nLoginType = TEEC_LOGIN_AUTHENTICATION;
         break;
      }

      /* Group tokens */
      if ((slotID >= 0x00010000) && (slotID <= 0x0002FFFF))
      {
         nLoginType = TEEC_LOGIN_GROUP;

         /* The 16 lower-order bits encode the group identifier */
         nLoginData = (uint32_t)slotID & 0x0000FFFF;
         pLoginData = (void*)&nLoginData;

         /* Update the slotID for the system / PKCS11 service */
         if ((slotID >= 0x00010000) && (slotID <= 0x0001FFFF))
         {
            /* System group token */
            slotID = 3;       /* CKV_TOKEN_SYSTEM_GROUP */
         }
         else  /* ((slotID >= 0x00020000) && (slotID <= 0x0002FFFF)) */
         {
            /* User group token */
            slotID = 0x4014;  /* CKV_TOKEN_USER_GROUP */
         }
      }

retry:
      memset(&sOperation, 0, sizeof(TEEC_Operation));

      if (nLoginType == TEEC_LOGIN_AUTHENTICATION)
      {
          nTeeError = TEEC_ReadSignatureFile((void **)&pSignatureFile, &nSignatureFileLen);
          if (nTeeError != TEEC_ERROR_ITEM_NOT_FOUND)
          {
              if (nTeeError != TEEC_SUCCESS)
              {
                  goto error;
              }

              sOperation.params[3].tmpref.buffer = pSignatureFile;
              sOperation.params[3].tmpref.size   = nSignatureFileLen;
              nParamType3 = TEEC_MEMREF_TEMP_INPUT;
          }
          else
          {
              /* No signature file found.
              * Should use LOGIN_APPLICATION for now
              * Can not use TEEC_LOGIN_AUTHENTICATION as this means that all .exe wil need a signature file
              * - a bit annoying for when passing the tests
              */
              nLoginType = TEEC_LOGIN_USER_APPLICATION;
          }
      }

      sOperation.paramTypes = TEEC_PARAM_TYPES(TEEC_NONE, TEEC_NONE, TEEC_NONE, nParamType3);
      nTeeError = TEEC_OpenSession(&g_sContext,
                                &pSession->sSession,        /* OUT session */
                                &SERVICE_UUID,              /* destination UUID */
                                nLoginType,                 /* connectionMethod */
                                pLoginData,                 /* connectionData */
                                &sOperation,                /* IN OUT operation */
                                NULL                        /* OUT returnOrigin, optional */
                                );
      if (nTeeError != TEEC_SUCCESS)
      {
         /* No need of the returnOrigin as this is not specific to P11 */

         if (  (nTeeError == TEEC_ERROR_NOT_SUPPORTED) &&
               (nLoginType == TEEC_LOGIN_AUTHENTICATION))
         {
            /* We could not open a session with the login TEEC_LOGIN_AUTHENTICATION */
            /* If it is not supported by the product, */
            /* retry with fallback to TEEC_LOGIN_USER_APPLICATION */
            nLoginType = TEEC_LOGIN_USER_APPLICATION;
            goto retry;
         }

         /* The ERROR_ACCESS_DENIED, if returned, will be converted into CKR_TOKEN_NOT_PRESENT
          * For the External Cryptographic API, this means that the authentication
          * of the calling application fails.
          */
         goto error;
      }

      memset(&sOperation, 0, sizeof(TEEC_Operation));
      sOperation.params[0].value.a = slotID;
      sOperation.params[0].value.b = flags;  /* access flags */
      sOperation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INOUT, TEEC_NONE, TEEC_NONE, TEEC_NONE);
      nTeeError = TEEC_InvokeCommand(&pSession->sSession,
                                  SERVICE_SYSTEM_PKCS11_C_OPEN_SESSION_COMMAND_ID & 0x00007FFF,
                                  &sOperation,              /* IN OUT operation */
                                  &nErrorOrigin             /* OUT returnOrigin, optional */
                                 );
      if (nTeeError != TEEC_SUCCESS)
      {
         goto error;
      }

      *phSession = (CK_SESSION_HANDLE)pSession;
      pSession->hCryptoSession = sOperation.params[0].value.a;

      return CKR_OK;
   }
   else
   {
      bool bResult;

      /* Check that {*phSession} is a valid primary session handle */
      if ((!ckInternalSessionIsOpenedEx(*phSession, &bIsPrimarySession)) ||
         (!bIsPrimarySession))
      {
         return CKR_SESSION_HANDLE_INVALID;
      }

      pSession = (PPKCS11_PRIMARY_SESSION_CONTEXT)(*phSession);

      /* allocate the secondary session context */
      pSecondarySession = (PKCS11_SECONDARY_SESSION_CONTEXT*)malloc(sizeof(PKCS11_SECONDARY_SESSION_CONTEXT));
      if (pSecondarySession == NULL)
      {
         return CKR_DEVICE_MEMORY;
      }
      pSecondarySession->sHeader.nMagicWord  = PKCS11_SESSION_MAGIC;
      pSecondarySession->sHeader.nSessionTag = PKCS11_SECONDARY_SESSION_TAG;
      pSecondarySession->pPrimarySession = pSession;

      libMutexLock(&pSession->sSecondarySessionTableMutex);
      bResult = libObjectHandle16Add(&pSession->sSecondarySessionTable,
                                    &pSecondarySession->sSecondarySessionNode);
      libMutexUnlock(&pSession->sSecondarySessionTableMutex);
      if (bResult == false)
      {
         free(pSecondarySession);
         return CKR_DEVICE_MEMORY;
      }

      memset(&sOperation, 0, sizeof(TEEC_Operation));
      sOperation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INOUT, TEEC_NONE, TEEC_NONE, TEEC_NONE);
      nTeeError = TEEC_InvokeCommand(&pSession->sSession,
                                  (pSession->hCryptoSession << 16) |
                                    (1 << 15) |
                                    (SERVICE_SYSTEM_PKCS11_C_OPEN_SESSION_COMMAND_ID & 0x00007FFF),
                                  &sOperation,                 /* IN OUT operation */
                                  &nErrorOrigin                /* OUT returnOrigin, optional */
                                 );
      if (nTeeError != TEEC_SUCCESS)
      {
         goto error;
      }

      *phSession = (CK_SESSION_HANDLE)pSecondarySession;
      pSecondarySession->hSecondaryCryptoSession = sOperation.params[0].value.a;

      return CKR_OK;
   }

error:
   nErrorCode = (nErrorOrigin == TEEC_ORIGIN_TRUSTED_APP ?
                  nTeeError :
                  ckInternalTeeErrorToCKError(nTeeError));

   if ((flags & CKVF_OPEN_SUB_SESSION) == 0)
   {
      libMutexDestroy(&pSession->sSecondarySessionTableMutex);
      free(pSession);
   }
   else
   {
      libMutexLock(&pSession->sSecondarySessionTableMutex);
      libObjectHandle16Remove(&pSession->sSecondarySessionTable,&pSecondarySession->sSecondarySessionNode);
      libMutexUnlock(&pSession->sSecondarySessionTableMutex);
      free(pSecondarySession);
   }

   return nErrorCode;
}

CK_RV PKCS11_EXPORT C_CloseSession(CK_SESSION_HANDLE hSession) /* the session's handle */
{
   CK_RV                   nErrorCode = CKR_OK;
   uint32_t                nErrorOrigin = TEEC_ORIGIN_API;
   TEEC_Result             nTeeError;
   TEEC_Operation          sOperation;
   bool                    bIsPrimarySession;

   /* Check Cryptoki is initialized */
   if (!g_bCryptokiInitialized)
   {
      return CKR_CRYPTOKI_NOT_INITIALIZED;
   }

   if (!ckInternalSessionIsOpenedEx(hSession, &bIsPrimarySession))
   {
      return CKR_SESSION_HANDLE_INVALID;
   }

   if (bIsPrimarySession)
   {
      LIB_OBJECT_NODE_HANDLE16*           pObject;
      PPKCS11_SECONDARY_SESSION_CONTEXT   pSecSession;
      PPKCS11_PRIMARY_SESSION_CONTEXT     pSession = (PPKCS11_PRIMARY_SESSION_CONTEXT)hSession;

      hSession = pSession->hCryptoSession;

      memset(&sOperation, 0, sizeof(TEEC_Operation));
      sOperation.paramTypes = TEEC_PARAM_TYPES(TEEC_NONE, TEEC_NONE, TEEC_NONE, TEEC_NONE);
      nTeeError = TEEC_InvokeCommand(&pSession->sSession,
                                  (pSession->hCryptoSession << 16 ) |
                                    (SERVICE_SYSTEM_PKCS11_C_CLOSE_SESSION_COMMAND_ID & 0x00007FFF),
                                  &sOperation,                 /* IN OUT operation */
                                  &nErrorOrigin                /* OUT returnOrigin, optional */
                                 );
      if (nTeeError != TEEC_SUCCESS)
      {
         goto end;
      }

      TEEC_CloseSession(&pSession->sSession);
      memset(&pSession->sSession, 0, sizeof(TEEC_Session));

      /* Free all secondary session contexts */
      libMutexLock(&pSession->sSecondarySessionTableMutex);
      pObject = libObjectHandle16RemoveOne(&pSession->sSecondarySessionTable);
      while (pObject != NULL)
      {
         /* find all secondary session contexts,
            and release associated resources */

         pSecSession = LIB_OBJECT_CONTAINER_OF(pObject, //ptr
                                               PKCS11_SECONDARY_SESSION_CONTEXT,//type
                                               sSecondarySessionNode);//member

         /* free secondary session context */
         free(pSecSession);

         pObject = libObjectHandle16RemoveOne(&pSession->sSecondarySessionTable);
      }
      libMutexUnlock(&pSession->sSecondarySessionTableMutex);

      libMutexDestroy(&pSession->sSecondarySessionTableMutex);

      /* free primary session context */
      free(pSession);
   }
   else
   {
      PPKCS11_SECONDARY_SESSION_CONTEXT pSecSession = (PPKCS11_SECONDARY_SESSION_CONTEXT)hSession;
      PPKCS11_PRIMARY_SESSION_CONTEXT   pSession;

      uint32_t nCommandID = ( (pSecSession->hSecondaryCryptoSession & 0xFFFF) << 16 ) |
                              (1 << 15) |
                              (SERVICE_SYSTEM_PKCS11_C_CLOSE_SESSION_COMMAND_ID & 0x00007FFF);

      /* every pre-check are fine, then, update the local handles */
      hSession = pSecSession->pPrimarySession->hCryptoSession;
      pSession = (PPKCS11_PRIMARY_SESSION_CONTEXT)(pSecSession->pPrimarySession);

      memset(&sOperation, 0, sizeof(TEEC_Operation));
      sOperation.paramTypes = TEEC_PARAM_TYPES(TEEC_NONE, TEEC_NONE, TEEC_NONE, TEEC_NONE);
      nTeeError = TEEC_InvokeCommand(&pSession->sSession,
                                  nCommandID,
                                  &sOperation,                 /* IN OUT operation */
                                  &nErrorOrigin                /* OUT returnOrigin, optional */
                                 );
      if (nTeeError != TEEC_SUCCESS)
      {
         goto end;
      }

      /* remove the object from the table */
      libMutexLock(&pSession->sSecondarySessionTableMutex);
      libObjectHandle16Remove(&pSecSession->pPrimarySession->sSecondarySessionTable, &pSecSession->sSecondarySessionNode);
      libMutexUnlock(&pSession->sSecondarySessionTableMutex);

      /* free secondary session context */
      free(pSecSession);
   }

end:
   nErrorCode = (nErrorOrigin == TEEC_ORIGIN_TRUSTED_APP ?
                  nTeeError :
                  ckInternalTeeErrorToCKError(nTeeError));
   return nErrorCode;
}


CK_RV PKCS11_EXPORT C_Login(CK_SESSION_HANDLE hSession,  /* the session's handle */
                              CK_USER_TYPE      userType,  /* the user type */
                              const CK_UTF8CHAR*   pPin,      /* the user's PIN */
                              CK_ULONG          ulPinLen)  /* the length of the PIN */
{
   /* Prevent the compiler from complaining about unused variables */
   do{(void)hSession;}while(0);
   do{(void)userType;}while(0);
   do{(void)pPin;}while(0);
   do{(void)ulPinLen;}while(0);

   return CKR_OK;
}

CK_RV PKCS11_EXPORT C_Logout(CK_SESSION_HANDLE hSession) /* the session's handle */
{
   do{(void)hSession;}while(0);

   return CKR_OK;
}
