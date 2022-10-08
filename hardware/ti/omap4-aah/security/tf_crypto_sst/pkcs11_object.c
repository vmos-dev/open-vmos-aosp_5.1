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

#include "pkcs11_internal.h"

/* ------------------------------------------------------------------------
Internal Functions
------------------------------------------------------------------------- */
/**
* Checks the following pre-conditions:
* - cryptoki is initialized,
* - hSession is valid (primary and/or secondary),
* - the user is logged in.
*
* And updates handle values:
*  IN/OUT : phSession
*           IN  = Cryptoki external handle
*           OUT = TFAPI handle = primary cryptoki session handle
*  OUT    : phSecSession16Msb
*           OUT = 0 for a primary session or
*                 the secondary cryptoki session handle in the 16 MSB bits
*/
static CK_RV static_checkPreConditionsAndUpdateHandles(
         CK_SESSION_HANDLE*   phSession,
         uint32_t*            phCommandIDAndSession,
         PPKCS11_PRIMARY_SESSION_CONTEXT* ppSession)
{
   bool  bIsPrimarySession;

   /* Check Cryptoki is initialized */
   if (!g_bCryptokiInitialized)
   {
      return CKR_CRYPTOKI_NOT_INITIALIZED;
   }

   if (phSession == NULL)
   {
      return CKR_SESSION_HANDLE_INVALID;
   }

   /* Check that the session is valid */
   if (!ckInternalSessionIsOpenedEx(*phSession, &bIsPrimarySession))
   {
      return CKR_SESSION_HANDLE_INVALID;
   }
   /* previous check is fine, then update session handles */
   if (bIsPrimarySession)
   {
      PPKCS11_PRIMARY_SESSION_CONTEXT pSession =
         (PPKCS11_PRIMARY_SESSION_CONTEXT)(*phSession);

      *phSession = pSession->hCryptoSession;
      *phCommandIDAndSession = (pSession->hCryptoSession<<16)|(*phCommandIDAndSession&0x00007FFF);
      *ppSession = pSession;
   }
   else
   {
      PPKCS11_SECONDARY_SESSION_CONTEXT pSecSession =
         (PPKCS11_SECONDARY_SESSION_CONTEXT)(*phSession);

      *phSession = pSecSession->pPrimarySession->hCryptoSession;
      *phCommandIDAndSession = (pSecSession->hSecondaryCryptoSession<<16)|(1<<15)|(*phCommandIDAndSession&0x00007FFF);
      *ppSession = pSecSession->pPrimarySession;
   }

   return CKR_OK;
}

/* Add up the sizes of the items and values in an attribute template.
 */
static CK_RV static_analyzeTemplate(
   uint32_t *const pDataOffset,
   uint32_t *const pBufferSize,
   const CK_ATTRIBUTE *const pTemplate,
   CK_ULONG const ulCount)
{
   CK_ULONG i;
   uint32_t nItemsSize;
   uint32_t nValuesSize = 0;

   nItemsSize = sizeof(uint32_t); /* for the number of attributes */
   if (ulCount == 0)
   {
      /* There are zero attributes, so the buffer will only contain the size word. */
      *pDataOffset += nItemsSize;
      *pBufferSize += nItemsSize;
      return CKR_OK;
   }
   nItemsSize += sizeof(INPUT_TEMPLATE_ITEM) * ulCount; /*for the attribute items*/

   /* Add up the attribute value sizes, taking the 4-byte alignment into account. */
   for (i = 0; i < ulCount; i++)
   {
      if (*pBufferSize + nValuesSize > 0x40000000)
      {
         /* Offsets above 0x40000000 aren't supported. */
         return CKR_DEVICE_ERROR;
      }
      nValuesSize += PKCS11_GET_SIZE_WITH_ALIGNMENT(pTemplate[i].ulValueLen);
   }

   *pDataOffset += nItemsSize;
   *pBufferSize += nItemsSize + nValuesSize;
   return CKR_OK;
}

static void static_copyTemplate(
   uint8_t *const pBuffer,
   uint32_t const nParamIndex,
   uint8_t **const ppAttributeCursor,
   uint8_t **const ppDataCursor,
   const CK_ATTRIBUTE *const pTemplate,
   CK_ULONG const ulCount)
{
   INPUT_TEMPLATE_ITEM  sItem;
   CK_ULONG i;
   *(uint32_t*)(*ppAttributeCursor) = ulCount;
   *ppAttributeCursor += sizeof(uint32_t);
   for (i = 0; i < ulCount; i++)
   {
      sItem.attributeType     = pTemplate[i].type;
      /* dataOffset = 0 means NULL buffer */
      sItem.dataOffset        = ((pTemplate[i].pValue == NULL) ? 0 :
                                 *ppDataCursor - pBuffer);
      sItem.dataParamIndex    = nParamIndex; /* The parameter where we store the data (0 to 3) */
      sItem.dataValueLen      = pTemplate[i].ulValueLen;
      /* Copy the item */
      memcpy(*ppAttributeCursor, &sItem, sizeof(INPUT_TEMPLATE_ITEM));
      *ppAttributeCursor += sizeof(INPUT_TEMPLATE_ITEM);
      if (pTemplate[i].pValue != NULL)
      {
         /* Copy the data */
         memcpy(*ppDataCursor, pTemplate[i].pValue, pTemplate[i].ulValueLen);
         /* Next data will be stored just after the previous one but aligned on 4 bytes */
         *ppDataCursor += PKCS11_GET_SIZE_WITH_ALIGNMENT(pTemplate[i].ulValueLen);
      }
   }
}

/******************************************/
/* The buffer must be freed by the caller */
/******************************************/
static CK_RV static_encodeTwoTemplates(
   uint8_t**         ppBuffer,
   uint32_t *        pBufferSize,
   const uint32_t    nParamIndex,
   const CK_ATTRIBUTE* pTemplate1,
   CK_ULONG          ulCount1,
   const CK_ATTRIBUTE* pTemplate2,
   CK_ULONG          ulCount2)
{
   uint8_t* pBuffer = NULL;
   uint32_t nBufferSize = 0;
   uint32_t nDataOffset = 0;
   uint8_t *pAttributeCursor;
   uint8_t *pDataCursor;
   CK_RV nErrorCode;

   nErrorCode = static_analyzeTemplate(&nDataOffset, &nBufferSize, pTemplate1, ulCount1);
   if (nErrorCode != CKR_OK) return nErrorCode;
   nErrorCode = static_analyzeTemplate(&nDataOffset, &nBufferSize, pTemplate2, ulCount2);
   if (nErrorCode != CKR_OK) return nErrorCode;

   pBuffer = malloc(nBufferSize);
   if (pBuffer == NULL) return CKR_DEVICE_MEMORY;
   memset(pBuffer, 0, nBufferSize);

   pAttributeCursor = pBuffer;
   pDataCursor = pBuffer + nDataOffset;
   static_copyTemplate(pBuffer, nParamIndex,
                       &pAttributeCursor, &pDataCursor,
                       pTemplate1, ulCount1);
   static_copyTemplate(pBuffer, nParamIndex,
                       &pAttributeCursor, &pDataCursor,
                       pTemplate2, ulCount2);

   *ppBuffer      = pBuffer;
   *pBufferSize   = nBufferSize;
   return CKR_OK;
}

/******************************************/
/* The buffer must be freed by the caller */
/******************************************/
static CK_RV static_encodeTemplate(
   uint8_t**         ppBuffer,
   uint32_t*         pBufferSize,
   const uint32_t    nParamIndex,
   CK_ATTRIBUTE*     pTemplate,
   CK_ULONG          ulCount)
{
   uint8_t* pBuffer = NULL;
   uint32_t nBufferSize = 0;
   uint32_t nDataOffset = 0;
   uint8_t *pAttributeCursor;
   uint8_t *pDataCursor;
   CK_RV nErrorCode;

   if (pTemplate == NULL || ulCount == 0)
   {
      *ppBuffer = NULL;
      *pBufferSize = 0;
      return CKR_OK;
   }

   nErrorCode = static_analyzeTemplate(&nDataOffset, &nBufferSize, pTemplate, ulCount);
   if (nErrorCode != CKR_OK) return nErrorCode;

   pBuffer = malloc(nBufferSize);
   if (pBuffer == NULL) return CKR_DEVICE_MEMORY;

   pAttributeCursor = pBuffer;
   pDataCursor = pBuffer + nDataOffset;
   static_copyTemplate(pBuffer, nParamIndex,
                       &pAttributeCursor, &pDataCursor,
                       pTemplate, ulCount);

   *ppBuffer      = pBuffer;
   *pBufferSize   = nBufferSize;
   return CKR_OK;
}
/* ----------------------------------------------------------------------- */

static CK_RV static_C_CallInit(
   uint32_t            nCommandID,
   CK_SESSION_HANDLE   hSession,
   const CK_MECHANISM* pMechanism,
   CK_OBJECT_HANDLE    hKey)
{
   TEEC_Result    teeErr;
   uint32_t       nErrorOrigin;
   TEEC_Operation sOperation;
   CK_RV          nErrorCode = CKR_OK;
   uint32_t       nCommandIDAndSession = nCommandID;
   uint32_t       nParamType2 = TEEC_NONE;
   PPKCS11_PRIMARY_SESSION_CONTEXT pSession;

   nErrorCode = static_checkPreConditionsAndUpdateHandles(&hSession, &nCommandIDAndSession, &pSession);
   if (nErrorCode != CKR_OK)
   {
      return nErrorCode;
   }
   if (pMechanism == NULL)
   {
      return CKR_ARGUMENTS_BAD;
   }

   memset(&sOperation, 0, sizeof(TEEC_Operation));
   sOperation.params[0].value.a = (uint32_t)pMechanism->mechanism;
   if (nCommandID != SERVICE_SYSTEM_PKCS11_C_DIGESTINIT_COMMAND_ID)
   {
      sOperation.params[0].value.b = (uint32_t)hKey;

   }
   sOperation.params[1].tmpref.buffer = (uint8_t*)pMechanism->pParameter;
   sOperation.params[1].tmpref.size   = (uint32_t)pMechanism->ulParameterLen;

   /* Specific case of RSA OAEP */
   if (((nCommandID == SERVICE_SYSTEM_PKCS11_C_ENCRYPTINIT_COMMAND_ID)
      ||(nCommandID == SERVICE_SYSTEM_PKCS11_C_DECRYPTINIT_COMMAND_ID))
      && (pMechanism->mechanism == CKM_RSA_PKCS_OAEP)
      && (pMechanism->pParameter != NULL))
   {
      /* Add the source buffer of the RSA OAEP mechanism parameters */
      nParamType2 = TEEC_MEMREF_TEMP_INPUT;
      sOperation.params[2].tmpref.buffer = (uint8_t*)((CK_RSA_PKCS_OAEP_PARAMS_PTR)(pMechanism->pParameter))->pSourceData;
      sOperation.params[2].tmpref.size   = (uint32_t) ((CK_RSA_PKCS_OAEP_PARAMS_PTR)(pMechanism->pParameter))->ulSourceDataLen;
   }
   sOperation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_MEMREF_TEMP_INPUT, nParamType2, TEEC_NONE);
   teeErr = TEEC_InvokeCommand(   &pSession->sSession,
                                  nCommandIDAndSession,        /* commandID */
                                  &sOperation,                 /* IN OUT operation */
                                  &nErrorOrigin                /* OUT returnOrigin, optional */
                                 );
   nErrorCode = (nErrorOrigin == TEEC_ORIGIN_TRUSTED_APP ?
                  teeErr :
                  ckInternalTeeErrorToCKError(teeErr));

   return nErrorCode;
}

/* ----------------------------------------------------------------------- */
/**
* If bSend, the pData buffer is sent to the service.
* If bResult, a buffer is received, the convention described in
* PKCS11 Section 11.2 applies for pResult and pulResultLen.
* Specific function used for single operation
*/
static CK_RV static_C_CallForSingle(
   uint32_t             nCommandID,
   CK_SESSION_HANDLE    hSession,
   const CK_BYTE*       pData,
   CK_ULONG             ulDataLen,
   CK_BYTE*             pResult,
   CK_ULONG*            pulResultLen,
   bool                 bSend,
   bool                 bReceive)
{
   TEEC_Result       teeErr;
   uint32_t          nErrorOrigin;
   TEEC_Operation    sOperation;
   CK_RV             nErrorCode = CKR_OK;
   uint32_t          nCommandIDAndSession = nCommandID;
   uint32_t          nParamType0 = TEEC_NONE;
   uint32_t          nParamType1 = TEEC_NONE;
   PPKCS11_PRIMARY_SESSION_CONTEXT pSession;

   nErrorCode = static_checkPreConditionsAndUpdateHandles(&hSession, &nCommandIDAndSession, &pSession);
   if (nErrorCode != CKR_OK)
   {
      return nErrorCode;
   }

   memset(&sOperation, 0, sizeof(TEEC_Operation));

   if (bSend)
   {
      nParamType0 = TEEC_MEMREF_TEMP_INPUT;
      sOperation.params[0].tmpref.buffer = (uint8_t*)pData;
      sOperation.params[0].tmpref.size   = (uint32_t)ulDataLen;
   }

   if (bReceive)
   {
      if (pulResultLen == NULL)
      {
         /* The P11 API Spec states that, in this case, the operation must be
            aborted and the error code CKR_ARGUMENTS_BAD must be returned. We
            achieve this result by sending an invalid parameter type */
         nParamType1 = TEEC_NONE;
      }
      else if (pResult == NULL)
      {
         /* If pResult is NULL, the caller only wants the buffer length.
            Send a NULL output memref */
         nParamType1 = TEEC_MEMREF_TEMP_OUTPUT;
         sOperation.params[1].tmpref.buffer = (uint8_t*)NULL;
      }
      else
      {
         /* send the result buffer information */
         nParamType1 = TEEC_MEMREF_TEMP_OUTPUT;
         sOperation.params[1].tmpref.buffer = (uint8_t*)pResult;
         sOperation.params[1].tmpref.size   = (uint32_t)*pulResultLen;
      }
   }

   sOperation.paramTypes = TEEC_PARAM_TYPES(nParamType0, nParamType1, TEEC_NONE, TEEC_NONE);
   teeErr = TEEC_InvokeCommand(&pSession->sSession,
                               nCommandIDAndSession,     /* commandID */
                               &sOperation,              /* IN OUT operation */
                               &nErrorOrigin             /* OUT returnOrigin, optional */
                              );
   if (teeErr != TEEC_SUCCESS)
   {
      nErrorCode = (nErrorOrigin == TEEC_ORIGIN_TRUSTED_APP ?
                     teeErr :
                     ckInternalTeeErrorToCKError(teeErr));
      goto end;
   }

   /* Success */
   nErrorCode = CKR_OK;

 end:
   if (bReceive)
   {
      if ((nErrorCode == CKR_OK) || (nErrorCode == CKR_BUFFER_TOO_SMALL))
      {
         /* The service has returned the actual result */
         /* The data is already in pResult, we get the returned length */
         *pulResultLen = sOperation.params[1].tmpref.size;
      }
   }

   return nErrorCode;
}

/* ----------------------------------------------------------------------- */
/**
* If bSend, the pData buffer is sent to the service.
* If bResult, a buffer is received, the convention described in
* PKCS11 Section 11.2 applies for pResult and pulResultLen.
* Specific function only used for update operations
*/
static CK_RV static_C_CallUpdate(
   uint32_t             nCommandID,
   CK_SESSION_HANDLE    hSession,
   const CK_BYTE*       pData,
   CK_ULONG             ulDataLen,
   CK_BYTE*             pResult,
   CK_ULONG*            pulResultLen,
   bool                 bSend,
   bool                 bReceive)
{
   TEEC_Result       teeErr;
   uint32_t          nErrorOrigin;
   TEEC_Operation    sOperation;
   CK_RV             nErrorCode = CKR_OK;
   uint32_t          nResultLen = 0;
   uint32_t          nCommandIDAndSession = nCommandID;
   uint32_t          nParamType0 = TEEC_NONE;
   uint32_t          nParamType1 = TEEC_NONE;
   PPKCS11_PRIMARY_SESSION_CONTEXT pSession;

   nErrorCode = static_checkPreConditionsAndUpdateHandles(&hSession, &nCommandIDAndSession, &pSession);
   if (nErrorCode != CKR_OK)
   {
      return nErrorCode;
   }

   if (pulResultLen != NULL)
   {
      nResultLen = *pulResultLen;
   }

   memset(&sOperation, 0, sizeof(TEEC_Operation));

   if (bSend)
   {
      nParamType0 = TEEC_MEMREF_TEMP_INPUT;
      sOperation.params[0].tmpref.buffer = (void*)pData;
      sOperation.params[0].tmpref.size   = ulDataLen;
   }

   if (bReceive)
   {
      if (pulResultLen == NULL)
      {
         /* The P11 API Spec states that, in this case, the operation must be
            aborted and the error code CKR_ARGUMENTS_BAD must be returned. We
            achieve this result by setting an invalid parameter type */
         nParamType1 = TEEC_NONE;
      }
      else if (pResult == NULL)
      {
         /* If pResult is NULL, the caller only wants the output buffer length.
            Pass a NULL output ref */
         nParamType1 = TEEC_MEMREF_TEMP_OUTPUT;
         sOperation.params[1].tmpref.buffer = NULL;
      }
      else
      {
         /* send the result buffer information */
         nParamType1 = TEEC_MEMREF_TEMP_OUTPUT;
         sOperation.params[1].tmpref.buffer = pResult;
         sOperation.params[1].tmpref.size   = (uint32_t)*pulResultLen;
      }
   }

   sOperation.paramTypes = TEEC_PARAM_TYPES(nParamType0, nParamType1, TEEC_NONE, TEEC_NONE);
   teeErr = TEEC_InvokeCommand(   &pSession->sSession,
                                  nCommandIDAndSession,        /* commandID */
                                  &sOperation,                 /* IN OUT operation */
                                  &nErrorOrigin                /* OUT returnOrigin, optional */
                                 );
   if (teeErr != TEEC_SUCCESS)
   {
      nErrorCode = (nErrorOrigin == TEEC_ORIGIN_TRUSTED_APP ?
                     teeErr :
                     ckInternalTeeErrorToCKError(teeErr));
      goto end;
   }

   /* Success */
   nErrorCode = CKR_OK;

 end:
   if (bReceive)
   {
      if ((nErrorCode == CKR_OK) || (nErrorCode == CKR_BUFFER_TOO_SMALL))
      {
         /* The service has returned the actual result */
         /* The data is already in pResult, we get the returned length */
         *pulResultLen = sOperation.params[1].tmpref.size;
      }
   }

   return nErrorCode;
}

/* Splits the buffer pData in chunks of nChunkSize size
 * and calls static_C_CallUpdate for each chunk
 */
static CK_RV static_C_CallSplitUpdate(
                           uint32_t           nCommandID,
                           CK_SESSION_HANDLE  hSession,
                           const CK_BYTE*     pData,
                           CK_ULONG           ulDataLen,
                           CK_BYTE*           pResult,
                           CK_ULONG*          pulResultLen,
                           bool               bSend,
                           bool               bReceive,
                           uint32_t           nChunkSize)
{
   CK_RV nErrorCode;
   CK_ULONG nPartDataLen;
   CK_ULONG nPartResultLen = 0;
   CK_ULONG ulResultLen = 0;
   bool bIsSymOperation = false;

   if (pulResultLen != NULL)
   {
      ulResultLen = *pulResultLen;
      /* Check wether the operation is a symetrical or asymetrical */
      if (*pulResultLen == ulDataLen)
      {
         bIsSymOperation = true;
      }
      *pulResultLen = 0;
   }

   while (ulDataLen > 0)
   {
      nPartDataLen = (ulDataLen <= nChunkSize ?
                        ulDataLen : nChunkSize);
      if (bIsSymOperation)
      {
         /* update the result only if it is a symetric operation */
         nPartResultLen = (ulResultLen <= nChunkSize ?
                               ulResultLen : nChunkSize);
      }
      else
      {
         nPartResultLen = ulResultLen;
      }

      nErrorCode =  static_C_CallUpdate(
                                 nCommandID,
                                 hSession,
                                 pData,
                                 nPartDataLen,
                                 pResult,
                                 &nPartResultLen,
                                 bSend,
                                 bReceive);
      if (nErrorCode != CKR_OK)
      {
         return nErrorCode;
      }

      ulDataLen -= nPartDataLen;
      pData += nPartDataLen;

      if (pResult != NULL)
      {
         ulResultLen -= nPartResultLen;
         pResult += nPartResultLen;
      }

      if ((pulResultLen != NULL) && (bIsSymOperation))
      {
         *pulResultLen += nPartResultLen;
      }
   }
   return CKR_OK;
}

/* Decides whether to split or not the inout/output buffer into chunks
*/
static CK_RV static_C_Call_CallForUpdate(
                           uint32_t           nCommandID,
                           CK_SESSION_HANDLE  hSession,
                           const CK_BYTE*     pData,
                           CK_ULONG           ulDataLen,
                           CK_BYTE*           pResult,
                           CK_ULONG*          pulResultLen,
                           bool               bSend,
                           bool               bReceive)
{
   CK_RV                   nErrorCode;
   uint32_t                nChunkSize;

   TEEC_ImplementationLimits  limits;

   if (!g_bCryptokiInitialized)
   {
      return CKR_CRYPTOKI_NOT_INITIALIZED;
   }

   TEEC_GetImplementationLimits(&limits);

   /* We can split the buffer in chunks of fixed size.
      No matter of the start address of the buffer,
      a safe size would be TotalNumberOfPages - 1
   */
   nChunkSize = limits.tmprefMaxSize - limits.pageSize;

   if (ulDataLen > nChunkSize)
   {
      /* inoutMaxSize = 0  means unlimited size */
       nErrorCode = static_C_CallSplitUpdate(nCommandID,
                                 hSession,
                                 pData,
                                 ulDataLen,
                                 pResult,
                                 pulResultLen,
                                 bSend,
                                 bReceive,
                                 nChunkSize);
   }
   else
   {
      nErrorCode = static_C_CallUpdate(nCommandID,
                                 hSession,
                                 pData,
                                 ulDataLen,
                                 pResult,
                                 pulResultLen,
                                 bSend,
                                 bReceive);
   }
   return nErrorCode;

}

/* ------------------------------------------------------------------------
Public Functions
------------------------------------------------------------------------- */

CK_RV PKCS11_EXPORT C_CreateObject(
   CK_SESSION_HANDLE    hSession,    /* the session's handle */
   const CK_ATTRIBUTE*  pTemplate,   /* the object's template */
   CK_ULONG             ulCount,     /* attributes in template */
   CK_OBJECT_HANDLE*    phObject)    /* receives new object's handle. */
{
   TEEC_Result          teeErr;
   uint32_t             nErrorOrigin;
   TEEC_Operation       sOperation;
   CK_RV                nErrorCode = CKR_OK;
   PPKCS11_PRIMARY_SESSION_CONTEXT pSession;
   uint32_t             nCommandIDAndSession = SERVICE_SYSTEM_PKCS11_C_CREATEOBJECT_COMMAND_ID;
   uint8_t*             pBuffer = NULL;
   uint32_t             nBufferSize = 0;

   if ( pTemplate == NULL || phObject == NULL )
   {
      return CKR_ARGUMENTS_BAD;
   }

   nErrorCode = static_checkPreConditionsAndUpdateHandles(&hSession, &nCommandIDAndSession, &pSession);
   if (nErrorCode != CKR_OK)
   {
      return nErrorCode;
   }

   nErrorCode = static_encodeTemplate(&pBuffer, &nBufferSize, 0, (CK_ATTRIBUTE*)pTemplate, ulCount); /* Sets the template on the param 0 */
   if (nErrorCode != CKR_OK)
   {
      return nErrorCode;
   }

   memset(&sOperation, 0, sizeof(TEEC_Operation));
   sOperation.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT, TEEC_VALUE_OUTPUT, TEEC_NONE, TEEC_NONE);
   sOperation.params[0].tmpref.buffer = pBuffer;
   sOperation.params[0].tmpref.size   = nBufferSize;
   teeErr = TEEC_InvokeCommand(   &pSession->sSession,
                                  nCommandIDAndSession,        /* commandID */
                                  &sOperation,                 /* IN OUT operation */
                                  &nErrorOrigin                /* OUT returnOrigin, optional */
                                 );
   free(pBuffer);

   if (teeErr != TEEC_SUCCESS)
   {
      nErrorCode = (nErrorOrigin == TEEC_ORIGIN_TRUSTED_APP ?
                     teeErr :
                     ckInternalTeeErrorToCKError(teeErr));
      goto end;
   }

   *phObject = sOperation.params[1].value.a;

   /* Success */
   nErrorCode = CKR_OK;

end:
   return nErrorCode;
}

CK_RV PKCS11_EXPORT C_DestroyObject(
   CK_SESSION_HANDLE hSession,  /* the session's handle */
   CK_OBJECT_HANDLE  hObject)   /* the object's handle */
{
   TEEC_Result    teeErr;
   uint32_t       nErrorOrigin;
   TEEC_Operation sOperation;
   CK_RV       nErrorCode = CKR_OK;
   uint32_t    nCommandIDAndSession = SERVICE_SYSTEM_PKCS11_C_DESTROYOBJECT_COMMAND_ID;
   PPKCS11_PRIMARY_SESSION_CONTEXT pSession;

   nErrorCode = static_checkPreConditionsAndUpdateHandles(&hSession, &nCommandIDAndSession, &pSession);
   if (nErrorCode != CKR_OK)
   {
      return nErrorCode;
   }

   memset(&sOperation, 0, sizeof(TEEC_Operation));
   sOperation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_NONE, TEEC_NONE, TEEC_NONE);
   sOperation.params[0].value.a = (uint32_t)hObject;
   teeErr = TEEC_InvokeCommand(   &pSession->sSession,
                                  nCommandIDAndSession,        /* commandID */
                                  &sOperation,                 /* IN OUT operation */
                                  &nErrorOrigin                /* OUT returnOrigin, optional */
                                 );
   nErrorCode = (nErrorOrigin == TEEC_ORIGIN_TRUSTED_APP ?
                  teeErr :
                  ckInternalTeeErrorToCKError(teeErr));
   return nErrorCode;
}

CK_RV PKCS11_EXPORT C_GetAttributeValue(
   CK_SESSION_HANDLE hSession,   /* the session's handle */
   CK_OBJECT_HANDLE  hObject,    /* the object's handle */
   CK_ATTRIBUTE*     pTemplate,  /* specifies attributes, gets values */
   CK_ULONG          ulCount)    /* attributes in template */
{
   TEEC_Result       teeErr;
   uint32_t          nErrorOrigin;
   TEEC_Operation    sOperation;
   CK_RV             nErrorCode = CKR_OK;
   CK_RV             nFinalErrorCode = CKR_OK;
   uint32_t          i = 0;
   uint32_t          nCommandIDAndSession = SERVICE_SYSTEM_PKCS11_C_GETATTRIBUTEVALUE_COMMAND_ID;
   PPKCS11_PRIMARY_SESSION_CONTEXT pSession;

   nErrorCode = static_checkPreConditionsAndUpdateHandles(&hSession, &nCommandIDAndSession, &pSession);
   if (nErrorCode != CKR_OK)
   {
      return nErrorCode;
   }

   if (pTemplate == NULL)
   {
      return CKR_ARGUMENTS_BAD;
   }

   if (ulCount == 0)
   {
      return CKR_OK;
   }

   for (i = 0; i < ulCount; i++)
   {
      memset(&sOperation, 0, sizeof(TEEC_Operation));
      sOperation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_MEMREF_TEMP_OUTPUT, TEEC_NONE, TEEC_NONE);
      sOperation.params[0].value.a = (uint32_t)hObject;
      sOperation.params[0].value.b = (uint32_t)pTemplate[i].type;
      sOperation.params[1].tmpref.buffer = pTemplate[i].pValue;
      sOperation.params[1].tmpref.size   = pTemplate[i].ulValueLen;
      teeErr = TEEC_InvokeCommand(   &pSession->sSession,
                                     nCommandIDAndSession,        /* commandID */
                                     &sOperation,                 /* IN OUT operation */
                                     &nErrorOrigin                /* OUT returnOrigin, optional */
                                    );
      nErrorCode = (nErrorOrigin == TEEC_ORIGIN_TRUSTED_APP ?
                     teeErr :
                     ckInternalTeeErrorToCKError(teeErr));
      if (nErrorCode != CKR_OK)
      {
         if (  (nErrorCode == CKR_ATTRIBUTE_SENSITIVE) ||
               (nErrorCode == CKR_ATTRIBUTE_TYPE_INVALID) ||
               (nErrorCode == CKR_BUFFER_TOO_SMALL))
         {
            nFinalErrorCode = nErrorCode;
         }
         else
         {
            /* Not some of the special error codes: this is fatal */
            return nErrorCode;
         }
      }

      pTemplate[i].ulValueLen = sOperation.params[1].tmpref.size;
   }

   return nFinalErrorCode;
}

CK_RV PKCS11_EXPORT C_FindObjectsInit(
   CK_SESSION_HANDLE    hSession,   /* the session's handle */
   const CK_ATTRIBUTE*  pTemplate,  /* attribute values to match */
   CK_ULONG             ulCount)    /* attributes in search template */
{
   TEEC_Result    teeErr;
   uint32_t       nErrorOrigin;
   TEEC_Operation sOperation;
   CK_RV       nErrorCode = CKR_OK;
   PPKCS11_PRIMARY_SESSION_CONTEXT pSession;
   uint32_t    nCommandIDAndSession = SERVICE_SYSTEM_PKCS11_C_FINDOBJECTSINIT_COMMAND_ID;
   uint8_t*    pBuffer     = NULL;
   uint32_t    nBufferSize = 0;

   nErrorCode = static_checkPreConditionsAndUpdateHandles(&hSession, &nCommandIDAndSession, &pSession);
   if (nErrorCode != CKR_OK)
   {
      return nErrorCode;
   }

   nErrorCode = static_encodeTemplate(&pBuffer, &nBufferSize, 0, (CK_ATTRIBUTE*)pTemplate, ulCount); /* Sets the template on the param 0 */
   if (nErrorCode != CKR_OK)
   {
      return nErrorCode;
   }

   memset(&sOperation, 0, sizeof(TEEC_Operation));
   sOperation.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT, TEEC_NONE, TEEC_NONE, TEEC_NONE);
   sOperation.params[0].tmpref.buffer = pBuffer;
   sOperation.params[0].tmpref.size   = nBufferSize;
   teeErr = TEEC_InvokeCommand(   &pSession->sSession,
                                  nCommandIDAndSession,        /* commandID */
                                  &sOperation,                 /* IN OUT operation */
                                  &nErrorOrigin                /* OUT returnOrigin, optional */
                                 );
   free(pBuffer);

   nErrorCode = (nErrorOrigin == TEEC_ORIGIN_TRUSTED_APP ?
                  teeErr :
                  ckInternalTeeErrorToCKError(teeErr));
   return nErrorCode;
}


CK_RV PKCS11_EXPORT C_FindObjects(
   CK_SESSION_HANDLE hSession,          /* the session's handle */
   CK_OBJECT_HANDLE* phObject,          /* receives object handle array */
   CK_ULONG          ulMaxObjectCount,  /* max handles to be returned */
   CK_ULONG*         pulObjectCount)    /* actual number returned */
{
   TEEC_Result       teeErr;
   uint32_t          nErrorOrigin;
   TEEC_Operation    sOperation;
   CK_RV             nErrorCode = CKR_OK;
   PPKCS11_PRIMARY_SESSION_CONTEXT pSession;
   uint32_t          nCommandIDAndSession = SERVICE_SYSTEM_PKCS11_C_FINDOBJECTS_COMMAND_ID;

   if ( (phObject == NULL) || (pulObjectCount == NULL))
   {
      return CKR_ARGUMENTS_BAD;
   }

   *pulObjectCount = 0;

   nErrorCode = static_checkPreConditionsAndUpdateHandles(&hSession, &nCommandIDAndSession, &pSession);
   if (nErrorCode != CKR_OK)
   {
      return nErrorCode;
   }

   memset(&sOperation, 0, sizeof(TEEC_Operation));
   sOperation.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_OUTPUT, TEEC_NONE, TEEC_NONE, TEEC_NONE);
   sOperation.params[0].tmpref.buffer = (uint8_t*)phObject;
   sOperation.params[0].tmpref.size   = (uint32_t)ulMaxObjectCount * sizeof(uint32_t);

   teeErr = TEEC_InvokeCommand(   &pSession->sSession,
                                  nCommandIDAndSession,        /* commandID */
                                  &sOperation,                 /* IN OUT operation */
                                  &nErrorOrigin                /* OUT returnOrigin, optional */
                                 );

   if (teeErr != TEEC_SUCCESS)
   {
      nErrorCode = (nErrorOrigin == TEEC_ORIGIN_TRUSTED_APP ?
                     teeErr :
                     ckInternalTeeErrorToCKError(teeErr));
      return nErrorCode;
   }

   *pulObjectCount = sOperation.params[0].tmpref.size / sizeof(uint32_t);

   return CKR_OK;
}

CK_RV PKCS11_EXPORT C_FindObjectsFinal(CK_SESSION_HANDLE hSession) /* the session's handle */
{
   TEEC_Result    teeErr;
   uint32_t       nErrorOrigin;
   TEEC_Operation sOperation;
   CK_RV          nErrorCode = CKR_OK;
   uint32_t       nCommandIDAndSession = SERVICE_SYSTEM_PKCS11_C_FINDOBJECTSFINAL_COMMAND_ID;
   PPKCS11_PRIMARY_SESSION_CONTEXT pSession;

   nErrorCode = static_checkPreConditionsAndUpdateHandles(&hSession, &nCommandIDAndSession, &pSession);
   if (nErrorCode != CKR_OK)
   {
      return nErrorCode;
   }

   memset(&sOperation, 0, sizeof(TEEC_Operation));
   teeErr = TEEC_InvokeCommand(   &pSession->sSession,
                                  nCommandIDAndSession,        /* commandID */
                                  &sOperation,                 /* IN OUT operation */
                                  &nErrorOrigin                /* OUT returnOrigin, optional */
                                 );

   nErrorCode = (nErrorOrigin == TEEC_ORIGIN_TRUSTED_APP ?
                  teeErr :
                  ckInternalTeeErrorToCKError(teeErr));
   return nErrorCode;
}


CK_RV PKCS11_EXPORT C_DigestInit(
   CK_SESSION_HANDLE   hSession,   /* the session's handle */
   const CK_MECHANISM* pMechanism) /* the digesting mechanism */
{
   return static_C_CallInit(
      SERVICE_SYSTEM_PKCS11_C_DIGESTINIT_COMMAND_ID,
      hSession,
      pMechanism,
      CK_INVALID_HANDLE);
}

CK_RV PKCS11_EXPORT C_Digest(
   CK_SESSION_HANDLE hSession,     /* the session's handle */
   const CK_BYTE*    pData,        /* data to be digested */
   CK_ULONG          ulDataLen,    /* bytes of data to be digested */
   CK_BYTE*          pDigest,      /* receives the message digest */
   CK_ULONG*         pulDigestLen) /* receives byte length of digest */
{
   return static_C_CallForSingle(
      SERVICE_SYSTEM_PKCS11_C_DIGEST_COMMAND_ID,
      hSession,
      pData,
      ulDataLen,
      pDigest,
      pulDigestLen,
      TRUE,
      TRUE);
}

CK_RV PKCS11_EXPORT C_DigestUpdate(
   CK_SESSION_HANDLE hSession,  /* the session's handle */
   const CK_BYTE*    pPart,     /* data to be digested */
   CK_ULONG          ulPartLen) /* bytes of data to be digested */
{
   return static_C_Call_CallForUpdate(
      SERVICE_SYSTEM_PKCS11_C_DIGESTUPDATE_COMMAND_ID,
      hSession,
      pPart,
      ulPartLen,
      NULL,
      NULL,
      TRUE,
      FALSE);
}

CK_RV PKCS11_EXPORT C_DigestFinal(
   CK_SESSION_HANDLE hSession,  /* the session's handle */
   CK_BYTE*       pDigest,      /* receives the message digest */
   CK_ULONG*      pulDigestLen) /* receives byte count of digest */
{
   return static_C_Call_CallForUpdate(
      SERVICE_SYSTEM_PKCS11_C_DIGESTFINAL_COMMAND_ID,
      hSession,
      NULL,
      0,
      pDigest,
      pulDigestLen,
      FALSE,
      TRUE);
}


CK_RV PKCS11_EXPORT C_SignInit(
   CK_SESSION_HANDLE    hSession,    /* the session's handle */
   const CK_MECHANISM*  pMechanism,  /* the signature mechanism */
   CK_OBJECT_HANDLE     hKey)        /* handle of the signature key */
{
   return static_C_CallInit(
      SERVICE_SYSTEM_PKCS11_C_SIGNINIT_COMMAND_ID,
      hSession,
      pMechanism,
      hKey);
}

CK_RV PKCS11_EXPORT C_Sign(
   CK_SESSION_HANDLE hSession,        /* the session's handle */
   const CK_BYTE*    pData,           /* the data (digest) to be signed */
   CK_ULONG          ulDataLen,       /* count of bytes to be signed */
   CK_BYTE*          pSignature,      /* receives the signature */
   CK_ULONG*         pulSignatureLen) /* receives byte count of signature */
{
   return static_C_CallForSingle(
      SERVICE_SYSTEM_PKCS11_C_SIGN_COMMAND_ID,
      hSession,
      pData,
      ulDataLen,
      pSignature,
      pulSignatureLen,
      TRUE,
      TRUE);
}

CK_RV PKCS11_EXPORT C_SignUpdate(
   CK_SESSION_HANDLE hSession,  /* the session's handle */
   const CK_BYTE*    pPart,     /* the data (digest) to be signed */
   CK_ULONG          ulPartLen) /* count of bytes to be signed */
{
   return static_C_Call_CallForUpdate(
      SERVICE_SYSTEM_PKCS11_C_SIGNUPDATE_COMMAND_ID,
      hSession,
      pPart,
      ulPartLen,
      NULL,
      NULL,
      TRUE,
      FALSE);
}

CK_RV PKCS11_EXPORT C_SignFinal(
   CK_SESSION_HANDLE hSession,     /* the session's handle */
   CK_BYTE*       pSignature,      /* receives the signature */
   CK_ULONG*      pulSignatureLen) /* receives byte count of signature */
{
   return static_C_Call_CallForUpdate(
      SERVICE_SYSTEM_PKCS11_C_SIGNFINAL_COMMAND_ID,
      hSession,
      NULL,
      0,
      pSignature,
      pulSignatureLen,
      FALSE,
      TRUE);
}

CK_RV PKCS11_EXPORT C_EncryptInit(
   CK_SESSION_HANDLE   hSession,    /* the session's handle */
   const CK_MECHANISM* pMechanism,  /* the encryption mechanism */
   CK_OBJECT_HANDLE    hKey)        /* handle of encryption key */
{
   return static_C_CallInit(
      SERVICE_SYSTEM_PKCS11_C_ENCRYPTINIT_COMMAND_ID,
      hSession,
      pMechanism,
      hKey);
}

CK_RV PKCS11_EXPORT C_Encrypt(
   CK_SESSION_HANDLE hSession,            /* the session's handle */
   const CK_BYTE*    pData,               /* the plaintext data */
   CK_ULONG          ulDataLen,           /* bytes of plaintext data */
   CK_BYTE*          pEncryptedData,      /* receives encrypted data */
   CK_ULONG*         pulEncryptedDataLen) /* receives encrypted byte count */
{

   return static_C_CallForSingle(
      SERVICE_SYSTEM_PKCS11_C_ENCRYPT_COMMAND_ID,
      hSession,
      pData,
      ulDataLen,
      pEncryptedData,
      pulEncryptedDataLen,
      TRUE,
      TRUE);
}



CK_RV PKCS11_EXPORT C_EncryptUpdate(
   CK_SESSION_HANDLE hSession,           /* the session's handle */
   const CK_BYTE*    pPart,              /* the plaintext data */
   CK_ULONG          ulPartLen,          /* bytes of plaintext data */
   CK_BYTE*          pEncryptedPart,     /* receives encrypted data */
   CK_ULONG*         pulEncryptedPartLen)/* receives encrypted byte count */
{
   return static_C_Call_CallForUpdate(
      SERVICE_SYSTEM_PKCS11_C_ENCRYPTUPDATE_COMMAND_ID,
      hSession,
      pPart,
      ulPartLen,
      pEncryptedPart,
      pulEncryptedPartLen,
      TRUE,
      TRUE);
}

CK_RV PKCS11_EXPORT C_EncryptFinal(
   CK_SESSION_HANDLE hSession,             /* the session's handle */
   CK_BYTE*       pLastEncryptedPart,      /* receives encrypted last part */
   CK_ULONG*      pulLastEncryptedPartLen) /* receives byte count */
{
   return static_C_Call_CallForUpdate(
      SERVICE_SYSTEM_PKCS11_C_ENCRYPTFINAL_COMMAND_ID,
      hSession,
      NULL,
      0,
      pLastEncryptedPart,
      pulLastEncryptedPartLen,
      FALSE,
      TRUE);
}

CK_RV PKCS11_EXPORT C_DecryptInit(
   CK_SESSION_HANDLE   hSession,    /* the session's handle */
   const CK_MECHANISM* pMechanism,  /* the decryption mechanism */
   CK_OBJECT_HANDLE    hKey)        /* handle of the decryption key */
{
   return static_C_CallInit(
      SERVICE_SYSTEM_PKCS11_C_DECRYPTINIT_COMMAND_ID,
      hSession,
      pMechanism,
      hKey);
}

CK_RV PKCS11_EXPORT C_Decrypt(
   CK_SESSION_HANDLE hSession,           /* the session's handle */
   const CK_BYTE*    pEncryptedData,     /* input encrypted data */
   CK_ULONG          ulEncryptedDataLen, /* count of bytes of input */
   CK_BYTE*          pData,              /* receives decrypted output */
   CK_ULONG*         pulDataLen)         /* receives decrypted byte count */
{

   return static_C_CallForSingle(
      SERVICE_SYSTEM_PKCS11_C_DECRYPT_COMMAND_ID,
      hSession,
      pEncryptedData,
      ulEncryptedDataLen,
      pData,
      pulDataLen,
      TRUE,
      TRUE);
}

CK_RV PKCS11_EXPORT C_DecryptUpdate(
   CK_SESSION_HANDLE hSession,            /* the session's handle */
   const CK_BYTE*    pEncryptedPart,      /* input encrypted data */
   CK_ULONG          ulEncryptedPartLen,  /* count of bytes of input */
   CK_BYTE*          pPart,               /* receives decrypted output */
   CK_ULONG*         pulPartLen)          /* receives decrypted byte count */
{
   return static_C_Call_CallForUpdate(
      SERVICE_SYSTEM_PKCS11_C_DECRYPTUPDATE_COMMAND_ID,
      hSession,
      pEncryptedPart,
      ulEncryptedPartLen,
      pPart,
      pulPartLen,
      TRUE,
      TRUE);
}

CK_RV PKCS11_EXPORT C_DecryptFinal(
   CK_SESSION_HANDLE hSession,    /* the session's handle */
   CK_BYTE*       pLastPart,      /* receives decrypted output */
   CK_ULONG*      pulLastPartLen) /* receives decrypted byte count */
{
   return static_C_Call_CallForUpdate(
      SERVICE_SYSTEM_PKCS11_C_DECRYPTFINAL_COMMAND_ID,
      hSession,
      NULL,
      0,
      pLastPart,
      pulLastPartLen,
      FALSE,
      TRUE);
}


CK_RV PKCS11_EXPORT C_GenerateKey(
   CK_SESSION_HANDLE    hSession,    /* the session's handle */
   const CK_MECHANISM*  pMechanism,  /* the key generation mechanism */
   const CK_ATTRIBUTE*  pTemplate,   /* template for the new key */
   CK_ULONG             ulCount,     /* number of attributes in template */
   CK_OBJECT_HANDLE*    phKey)       /* receives handle of new key */
{
   TEEC_Result    teeErr;
   uint32_t       nErrorOrigin;
   TEEC_Operation sOperation;
   CK_RV       nErrorCode = CKR_OK;
   uint32_t    nCommandIDAndSession = SERVICE_SYSTEM_PKCS11_C_GENERATEKEY_COMMAND_ID;
   uint8_t*    pBuffer     = NULL;
   uint32_t    nBufferSize = 0;
   PPKCS11_PRIMARY_SESSION_CONTEXT pSession;

   if ((pMechanism == NULL) || (phKey == NULL) || (pTemplate == NULL))
   {
      return CKR_ARGUMENTS_BAD;
   }

   nErrorCode = static_checkPreConditionsAndUpdateHandles(&hSession, &nCommandIDAndSession, &pSession);
   if (nErrorCode != CKR_OK)
   {
      return nErrorCode;
   }

   nErrorCode = static_encodeTemplate(&pBuffer, &nBufferSize, 2, (CK_ATTRIBUTE*)pTemplate, ulCount);
   if (nErrorCode != CKR_OK)
   {
      return nErrorCode;
   }

   memset(&sOperation, 0, sizeof(TEEC_Operation));
   sOperation.params[0].value.a = (uint32_t)pMechanism->mechanism;
   sOperation.params[0].value.b = 0;
   sOperation.params[1].tmpref.buffer = pMechanism->pParameter;
   sOperation.params[1].tmpref.size = (uint32_t)pMechanism->ulParameterLen;
   sOperation.params[2].tmpref.buffer = pBuffer;
   sOperation.params[2].tmpref.size = nBufferSize;
   sOperation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INOUT, TEEC_MEMREF_TEMP_INPUT, TEEC_MEMREF_TEMP_INPUT, TEEC_NONE);
   teeErr = TEEC_InvokeCommand(   &pSession->sSession,
                                  nCommandIDAndSession,        /* commandID */
                                  &sOperation,                 /* IN OUT operation */
                                  &nErrorOrigin                /* OUT returnOrigin, optional */
                                 );
   free(pBuffer);

   if (teeErr != TEEC_SUCCESS)
   {
      nErrorCode = (nErrorOrigin == TEEC_ORIGIN_TRUSTED_APP ?
                     teeErr :
                     ckInternalTeeErrorToCKError(teeErr));
      return nErrorCode;
   }

   *phKey = sOperation.params[0].value.a;

   return CKR_OK;
}

CK_RV PKCS11_EXPORT C_GenerateKeyPair(
   CK_SESSION_HANDLE    hSession,                    /* the session's handle */
   const CK_MECHANISM*  pMechanism,                  /* the key gen. mech. */
   const CK_ATTRIBUTE*  pPublicKeyTemplate,          /* pub. attr. template */
   CK_ULONG             ulPublicKeyAttributeCount,   /* # of pub. attrs. */
   const CK_ATTRIBUTE*  pPrivateKeyTemplate,         /* priv. attr. template */
   CK_ULONG             ulPrivateKeyAttributeCount,  /* # of priv. attrs. */
   CK_OBJECT_HANDLE*    phPublicKey,                 /* gets pub. key handle */
   CK_OBJECT_HANDLE*    phPrivateKey)                /* gets priv. key handle */
{
   TEEC_Result    teeErr;
   uint32_t       nErrorOrigin;
   TEEC_Operation sOperation;
   CK_RV       nErrorCode = CKR_OK;
   uint32_t    nCommandIDAndSession = SERVICE_SYSTEM_PKCS11_C_GENERATEKEYPAIR_COMMAND_ID;
   uint8_t*    pBuffer     = NULL;
   uint32_t    nBufferSize = 0;
   PPKCS11_PRIMARY_SESSION_CONTEXT pSession;

   if (  (pMechanism == NULL) ||
         (pPublicKeyTemplate == NULL) ||
         (phPublicKey== NULL) || (phPrivateKey== NULL))
   {
      return CKR_ARGUMENTS_BAD;
   }

   nErrorCode = static_checkPreConditionsAndUpdateHandles(&hSession, &nCommandIDAndSession, &pSession);
   if (nErrorCode != CKR_OK)
   {
      return nErrorCode;
   }

   nErrorCode = static_encodeTwoTemplates(&pBuffer, &nBufferSize, 2, pPublicKeyTemplate, ulPublicKeyAttributeCount, pPrivateKeyTemplate, ulPrivateKeyAttributeCount);
   if (nErrorCode != CKR_OK)
   {
      return nErrorCode;
   }

   memset(&sOperation, 0, sizeof(TEEC_Operation));
   sOperation.params[0].value.a = (uint32_t)pMechanism->mechanism;
   sOperation.params[0].value.b = 0;
   sOperation.params[1].tmpref.buffer = (uint8_t*)pMechanism->pParameter;
   sOperation.params[1].tmpref.size = (uint32_t)pMechanism->ulParameterLen;
   sOperation.params[2].tmpref.buffer = pBuffer;
   sOperation.params[2].tmpref.size = nBufferSize;
   sOperation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INOUT, TEEC_MEMREF_TEMP_INPUT, TEEC_MEMREF_TEMP_INPUT, TEEC_NONE);
   teeErr = TEEC_InvokeCommand(   &pSession->sSession,
                                  nCommandIDAndSession,        /* commandID */
                                  &sOperation,                 /* IN OUT operation */
                                  &nErrorOrigin                /* OUT returnOrigin, optional */
                                 );
   free(pBuffer);

   if (teeErr != TEEC_SUCCESS)
   {
      nErrorCode = (nErrorOrigin == TEEC_ORIGIN_TRUSTED_APP ?
                     teeErr :
                     ckInternalTeeErrorToCKError(teeErr));
      return nErrorCode;
   }

   *phPublicKey  = sOperation.params[0].value.a;
   *phPrivateKey = sOperation.params[0].value.b;

   return CKR_OK;
}

CK_RV PKCS11_EXPORT C_DeriveKey(
   CK_SESSION_HANDLE    hSession,          /* session's handle */
   const CK_MECHANISM*  pMechanism,        /* key deriv. mech. */
   CK_OBJECT_HANDLE     hBaseKey,          /* base key */
   const CK_ATTRIBUTE*  pTemplate,         /* new key template */
   CK_ULONG             ulAttributeCount,  /* template length */
   CK_OBJECT_HANDLE*    phKey)             /* gets new handle */
{
   TEEC_Result    teeErr;
   uint32_t       nErrorOrigin;
   TEEC_Operation sOperation;
   CK_RV       nErrorCode = CKR_OK;
   uint32_t    nCommandIDAndSession = SERVICE_SYSTEM_PKCS11_C_DERIVEKEY_COMMAND_ID;
   uint8_t*    pBuffer     = NULL;
   uint32_t    nBufferSize = 0;
   PPKCS11_PRIMARY_SESSION_CONTEXT pSession;

   if ((pMechanism == NULL) || (pTemplate == NULL) || (phKey == NULL))
   {
      return CKR_ARGUMENTS_BAD;
   }

   nErrorCode = static_checkPreConditionsAndUpdateHandles(&hSession, &nCommandIDAndSession, &pSession);
   if (nErrorCode != CKR_OK)
   {
      return nErrorCode;
   }

   nErrorCode = static_encodeTemplate(&pBuffer, &nBufferSize, 2, (CK_ATTRIBUTE*)pTemplate, ulAttributeCount);
   if (nErrorCode != CKR_OK)
   {
      return nErrorCode;
   }

   memset(&sOperation, 0, sizeof(TEEC_Operation));
   sOperation.params[0].value.a = (uint32_t)pMechanism->mechanism;
   sOperation.params[0].value.b = (uint32_t)hBaseKey;
   sOperation.params[1].tmpref.buffer = (uint8_t*)pMechanism->pParameter;
   sOperation.params[1].tmpref.size = (uint32_t)pMechanism->ulParameterLen;
   sOperation.params[2].tmpref.buffer = pBuffer;
   sOperation.params[2].tmpref.size = nBufferSize;
   sOperation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INOUT, TEEC_MEMREF_TEMP_INPUT, TEEC_MEMREF_TEMP_INPUT, TEEC_NONE);
   teeErr = TEEC_InvokeCommand(   &pSession->sSession,
                                  nCommandIDAndSession,        /* commandID */
                                  &sOperation,                 /* IN OUT operation */
                                  &nErrorOrigin                /* OUT returnOrigin, optional */
                                 );
   free(pBuffer);

   if (teeErr != TEEC_SUCCESS)
   {
      nErrorCode = (nErrorOrigin == TEEC_ORIGIN_TRUSTED_APP ?
                     teeErr :
                     ckInternalTeeErrorToCKError(teeErr));
      return nErrorCode;
   }

   *phKey = sOperation.params[0].value.a;

   return CKR_OK;
}

CK_RV PKCS11_EXPORT C_SeedRandom(
   CK_SESSION_HANDLE hSession,  /* the session's handle */
   const CK_BYTE*    pSeed,     /* the seed material */
   CK_ULONG          ulSeedLen) /* count of bytes of seed material */
{
   TEEC_Result    teeErr;
   uint32_t       nErrorOrigin;
   TEEC_Operation sOperation;
   CK_RV       nErrorCode = CKR_OK;
   uint32_t    nCommandIDAndSession = SERVICE_SYSTEM_PKCS11_C_SEEDRANDOM_COMMAND_ID;
   PPKCS11_PRIMARY_SESSION_CONTEXT pSession;

   nErrorCode = static_checkPreConditionsAndUpdateHandles(&hSession, &nCommandIDAndSession, &pSession);
   if (nErrorCode != CKR_OK)
   {
      return nErrorCode;
   }
   memset(&sOperation, 0, sizeof(TEEC_Operation));
   sOperation.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT, TEEC_NONE, TEEC_NONE, TEEC_NONE);
   sOperation.params[0].tmpref.buffer = (uint8_t*)pSeed;
   sOperation.params[0].tmpref.size   = (uint32_t)ulSeedLen;
   teeErr = TEEC_InvokeCommand(   &pSession->sSession,
                                  nCommandIDAndSession,        /* commandID */
                                  &sOperation,                 /* IN OUT operation */
                                  &nErrorOrigin                /* OUT returnOrigin, optional */
                                 );

   nErrorCode = (nErrorOrigin == TEEC_ORIGIN_TRUSTED_APP ?
                  teeErr :
                  ckInternalTeeErrorToCKError(teeErr));
   return nErrorCode;
}

CK_RV PKCS11_EXPORT C_GenerateRandom(
   CK_SESSION_HANDLE hSession,    /* the session's handle */
   CK_BYTE*          pRandomData,  /* receives the random data */
   CK_ULONG          ulRandomLen) /* number of bytes to be generated */
{
   TEEC_Result    teeErr;
   uint32_t       nErrorOrigin;
   TEEC_Operation sOperation;
   CK_RV       nErrorCode = CKR_OK;
   uint32_t    nCommandIDAndSession = SERVICE_SYSTEM_PKCS11_C_GENERATERANDOM_COMMAND_ID;
   PPKCS11_PRIMARY_SESSION_CONTEXT pSession;

   nErrorCode = static_checkPreConditionsAndUpdateHandles(&hSession, &nCommandIDAndSession, &pSession);
   if (nErrorCode != CKR_OK)
   {
      return nErrorCode;
   }

   do
   {
      CK_ULONG nArrayLength;
      nArrayLength = 1024;
      if (ulRandomLen < nArrayLength)
      {
         nArrayLength = ulRandomLen;
      }
      memset(&sOperation, 0, sizeof(TEEC_Operation));
      sOperation.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_OUTPUT, TEEC_NONE, TEEC_NONE, TEEC_NONE);
      sOperation.params[0].tmpref.buffer = (uint8_t*)pRandomData;
      sOperation.params[0].tmpref.size   = (uint32_t)nArrayLength;
      teeErr = TEEC_InvokeCommand(   &pSession->sSession,
                                     nCommandIDAndSession,        /* commandID */
                                     &sOperation,                 /* IN OUT operation */
                                     &nErrorOrigin                /* OUT returnOrigin, optional */
                                    );
      if (teeErr != TEEC_SUCCESS)
      {
         nErrorCode = (nErrorOrigin == TEEC_ORIGIN_TRUSTED_APP ?
                        teeErr :
                        ckInternalTeeErrorToCKError(teeErr));
         return nErrorCode;
      }

      ulRandomLen -= nArrayLength;
      pRandomData += nArrayLength;
      if (ulRandomLen == 0)
      {
         break;
      }
   }
   while(1);

   return CKR_OK;
}

CK_RV PKCS11_EXPORT C_VerifyInit(
   CK_SESSION_HANDLE   hSession,    /* the session's handle */
   const CK_MECHANISM* pMechanism,  /* the verification mechanism */
   CK_OBJECT_HANDLE    hKey)        /* handle of the verification key */
{
   return static_C_CallInit(
      SERVICE_SYSTEM_PKCS11_C_VERIFYINIT_COMMAND_ID,
      hSession,
      pMechanism,
      hKey);
}

CK_RV PKCS11_EXPORT C_Verify(
   CK_SESSION_HANDLE hSession,       /* the session's handle */
   const CK_BYTE*    pData,          /* plaintext data (digest) to compare */
   CK_ULONG          ulDataLen,      /* length of data (digest) in bytes */
   CK_BYTE*          pSignature,     /* the signature to be verified */
   CK_ULONG          ulSignatureLen) /* count of bytes of signature */
{
   TEEC_Result    teeErr;
   uint32_t       nErrorOrigin;
   TEEC_Operation sOperation;
   CK_RV       nErrorCode = CKR_OK;
   uint32_t    nCommandIDAndSession = SERVICE_SYSTEM_PKCS11_C_VERIFY_COMMAND_ID;
   PPKCS11_PRIMARY_SESSION_CONTEXT pSession;

   nErrorCode = static_checkPreConditionsAndUpdateHandles(&hSession, &nCommandIDAndSession, &pSession);
   if (nErrorCode != CKR_OK)
   {
      return nErrorCode;
   }

   memset(&sOperation, 0, sizeof(TEEC_Operation));
   sOperation.params[0].tmpref.buffer = (uint8_t*)pData;
   sOperation.params[0].tmpref.size   = (uint32_t)ulDataLen;
   sOperation.params[1].tmpref.buffer = (uint8_t*)pSignature;
   sOperation.params[1].tmpref.size   = (uint32_t)ulSignatureLen;
   sOperation.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT, TEEC_MEMREF_TEMP_INPUT, TEEC_NONE, TEEC_NONE);
   teeErr = TEEC_InvokeCommand(   &pSession->sSession,
                                  nCommandIDAndSession,        /* commandID */
                                  &sOperation,                 /* IN OUT operation */
                                  &nErrorOrigin                /* OUT returnOrigin, optional */
                                 );
   nErrorCode = (nErrorOrigin == TEEC_ORIGIN_TRUSTED_APP ?
                  teeErr :
                  ckInternalTeeErrorToCKError(teeErr));
   return nErrorCode;
}

CK_RV PKCS11_EXPORT C_VerifyUpdate(
   CK_SESSION_HANDLE hSession,  /* the session's handle */
   const CK_BYTE*    pPart,     /* plaintext data (digest) to compare */
   CK_ULONG          ulPartLen) /* length of data (digest) in bytes */
{
   return static_C_Call_CallForUpdate(
      SERVICE_SYSTEM_PKCS11_C_VERIFYUPDATE_COMMAND_ID,
      hSession,
      pPart,
      ulPartLen,
      NULL,
      NULL,
      TRUE,
      FALSE);
}

CK_RV PKCS11_EXPORT C_VerifyFinal(
   CK_SESSION_HANDLE hSession,       /* the session's handle */
   const CK_BYTE*    pSignature,     /* the signature to be verified */
   CK_ULONG          ulSignatureLen) /* count of bytes of signature */
{
   return static_C_Call_CallForUpdate(
      SERVICE_SYSTEM_PKCS11_C_VERIFYFINAL_COMMAND_ID,
      hSession,
      pSignature,
      ulSignatureLen,
      NULL,
      NULL,
      TRUE,
      FALSE);
}

CK_RV PKCS11_EXPORT C_CloseObjectHandle(
   CK_SESSION_HANDLE hSession,  /* the session's handle */
   CK_OBJECT_HANDLE  hObject)   /* the object's handle */
{
   TEEC_Result    teeErr;
   uint32_t       nErrorOrigin;
   TEEC_Operation sOperation;
   CK_RV       nErrorCode = CKR_OK;
   uint32_t    nCommandIDAndSession = SERVICE_SYSTEM_PKCS11_C_CLOSEOBJECTHANDLE_COMMAND_ID;
   PPKCS11_PRIMARY_SESSION_CONTEXT pSession;

   nErrorCode = static_checkPreConditionsAndUpdateHandles(&hSession, &nCommandIDAndSession, &pSession);
   if (nErrorCode != CKR_OK)
   {
      return nErrorCode;
   }
   memset(&sOperation, 0, sizeof(TEEC_Operation));
   sOperation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_NONE, TEEC_NONE, TEEC_NONE);
   sOperation.params[0].value.a = (uint32_t)hObject;
   sOperation.params[0].value.b = 0;
   teeErr = TEEC_InvokeCommand(   &pSession->sSession,
                                  nCommandIDAndSession,        /* commandID */
                                  &sOperation,                 /* IN OUT operation */
                                  &nErrorOrigin                /* OUT returnOrigin, optional */
                                 );
   nErrorCode = (nErrorOrigin == TEEC_ORIGIN_TRUSTED_APP ?
                  teeErr :
                  ckInternalTeeErrorToCKError(teeErr));
   return nErrorCode;
}

CK_RV PKCS11_EXPORT C_CopyObject(
         CK_SESSION_HANDLE    hSession,    /* the session's handle */
         CK_OBJECT_HANDLE     hObject,     /* the source object's handle */
   const CK_ATTRIBUTE*        pTemplate,   /* the template of the copied object */
         CK_ULONG             ulCount,     /* the number of attributes of the template*/
         CK_OBJECT_HANDLE*    phNewObject) /* the copied object's handle */
{
   TEEC_Result    teeErr;
   uint32_t       nErrorOrigin;
   TEEC_Operation sOperation;
   CK_RV       nErrorCode = CKR_OK;
   uint32_t    nCommandIDAndSession = SERVICE_SYSTEM_PKCS11_C_COPYOBJECT_COMMAND_ID;
   uint8_t*    pBuffer     = NULL;
   uint32_t    nBufferSize = 0;
   PPKCS11_PRIMARY_SESSION_CONTEXT pSession;

   if ((pTemplate == NULL) || (phNewObject == NULL))
   {
      return CKR_ARGUMENTS_BAD;
   }

   nErrorCode = static_checkPreConditionsAndUpdateHandles(&hSession, &nCommandIDAndSession, &pSession);
   if (nErrorCode != CKR_OK)
   {
      return nErrorCode;
   }

   nErrorCode = static_encodeTemplate(&pBuffer, &nBufferSize, 1, (CK_ATTRIBUTE*)pTemplate, ulCount);
   if (nErrorCode != CKR_OK)
   {
      return nErrorCode;
   }

   memset(&sOperation, 0, sizeof(TEEC_Operation));
   sOperation.params[0].value.a = (uint32_t)hObject;
   sOperation.params[0].value.b = 0;
   sOperation.params[1].tmpref.buffer = pBuffer;
   sOperation.params[1].tmpref.size   = nBufferSize;
   sOperation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INOUT, TEEC_MEMREF_TEMP_INPUT, TEEC_NONE, TEEC_NONE);
   teeErr = TEEC_InvokeCommand(   &pSession->sSession,
                                  nCommandIDAndSession,        /* commandID */
                                  &sOperation,                 /* IN OUT operation */
                                  &nErrorOrigin                /* OUT returnOrigin, optional */
                                 );
   free(pBuffer);

   if (teeErr != TEEC_SUCCESS)
   {
      nErrorCode = (nErrorOrigin == TEEC_ORIGIN_TRUSTED_APP ?
                     teeErr :
                     ckInternalTeeErrorToCKError(teeErr));
      return nErrorCode;
   }

   *phNewObject = sOperation.params[0].value.a;

   return CKR_OK;
}
