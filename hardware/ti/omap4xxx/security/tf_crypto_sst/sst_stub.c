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

#ifdef __ANDROID32__
#include <stddef.h>
#endif

#include <stdlib.h>
#include <string.h>

#define SST_EXPORTS
#define EXCLUDE_SERVICE_SYSTEM_SST_BASIC_TYPES
#include "sst.h"

/* Included for the TEE management */
#include "pkcs11_internal.h"


static TEEC_Session g_SSTSession;
static bool g_bSSTInitialized = false;


/* ------------------------------------------------------------------------
            TEEC -> SST error code translation
  ------------------------------------------------------------------------- */
static SST_ERROR static_SSTConvertErrorCode(TEEC_Result nError)
{
   switch (nError)
   {
      case TEEC_SUCCESS:
         return SST_SUCCESS;
      case SST_ERROR_BAD_PARAMETERS:
      case SST_ERROR_ACCESS_DENIED:
      case SST_ERROR_ACCESS_CONFLICT:
      case SST_ERROR_CORRUPTED:
      case SST_ERROR_NO_SPACE:
      case SST_ERROR_ITEM_NOT_FOUND:
      case SST_ERROR_OUT_OF_MEMORY:
      case SST_ERROR_OVERFLOW:
         return nError;
      default:
         return SST_ERROR_GENERIC;
   }
}

static TEEC_Session* static_SSTGetSession(void)
{
   if (g_bSSTInitialized)
   {
      return &g_SSTSession;
   }

   return NULL;
}

SST_ERROR SST_EXPORT_API SSTInit(void)
{
   TEEC_Result          nTeeError = TEEC_SUCCESS;
   TEEC_Operation       sOperation;
   uint8_t              nParamType3 = TEEC_NONE;
   void*                pSignatureFile = NULL;
   uint32_t             nSignatureFileLen = 0;
   uint32_t             nLoginType;

   stubMutexLock();
   if (g_bSSTInitialized)
   {
      /* SST library already initialized */
      nTeeError = TEEC_SUCCESS;
      goto end;
   }

   nTeeError = stubInitializeContext();
   if (nTeeError != TEEC_SUCCESS)
   {
      goto end;
   }

   /* Check if there is a signature file.
    * If yes, send it in param3, otherwise use LOGIN_APPLICATION
    */
   nTeeError =  TEEC_ReadSignatureFile(&pSignatureFile, &nSignatureFileLen);
   if (nTeeError == TEEC_ERROR_ITEM_NOT_FOUND)
   {
      nLoginType = TEEC_LOGIN_USER_APPLICATION;
   }
   else
   {
       if (nTeeError != TEEC_SUCCESS)
       {
           goto end;
       }
       sOperation.params[3].tmpref.buffer = pSignatureFile;
       sOperation.params[3].tmpref.size   = nSignatureFileLen;
       nParamType3 = TEEC_MEMREF_TEMP_INPUT;
       nLoginType = TEEC_LOGIN_AUTHENTICATION;
   }

   sOperation.paramTypes = TEEC_PARAM_TYPES(TEEC_NONE, TEEC_NONE, TEEC_NONE, nParamType3);
   nTeeError = TEEC_OpenSession(&g_sContext,
                             &g_SSTSession,              /* OUT session */
                             &SERVICE_UUID,              /* destination UUID */
                             nLoginType,                 /* connectionMethod */
                             NULL,                       /* connectionData */
                             &sOperation,                /* IN OUT operation */
                             NULL                        /* OUT returnOrigin, optional */
                             );
   if (nTeeError != TEEC_SUCCESS)
   {
      goto end_finalize_context;
   }

   g_bSSTInitialized = true;
   stubMutexUnlock();
   return SST_SUCCESS;

end_finalize_context:
   stubFinalizeContext();
end:
   stubMutexUnlock();
   return static_SSTConvertErrorCode(nTeeError);
}

SST_ERROR SST_EXPORT_API SSTTerminate(void)
{
   stubMutexLock();
   if (g_bSSTInitialized)
   {
      TEEC_CloseSession(&g_SSTSession);
      stubFinalizeContext();
      g_bSSTInitialized = false;
   }
   /* else if not intialized => success too */
   stubMutexUnlock();
   return SST_SUCCESS;
}


/* ------------------------------------------------------------------------
                           Other API Functions
------------------------------------------------------------------------- */


/* Check that the input filename is well-formed */
static SST_ERROR static_SSTCheckFileName(const char* pName)
{
   uint32_t i;
   char     c;

   if (pName == NULL)
   {
      return SST_ERROR_BAD_PARAMETERS;
   }

   for (i = 0; i <= SST_MAX_FILENAME; i++)
   {
      c = pName[i];
      if (c == 0)
      {
         /* End of the string */
         return SST_SUCCESS;
      }

      if (c == '/' || c == '\\')
      {
         /* Invalid character */
         return SST_ERROR_BAD_PARAMETERS;
      }

      if (c < 0x20 || c >= 0x7F)
      {
         /* Filename contains illegal characters */
         return SST_ERROR_BAD_PARAMETERS;
      }
   }
   /* Filename is too long. Zero terminator not found */
   return SST_ERROR_BAD_PARAMETERS;
}

static SST_ERROR static_SSTCheckPattern(
      const char* pFilenamePattern)
{
   uint32_t i;
   if(pFilenamePattern == NULL)
   {
      return S_SUCCESS;
   }

   /**
    * Check Forbidden characters.
    */
   for (i = 0; pFilenamePattern[i] != 0; i++)
   {
      if(pFilenamePattern[i] < 0x20 )
      {
         return S_ERROR_BAD_PARAMETERS;
      }
      else if(pFilenamePattern[i] == 0x2F ) /* '/' */
      {
         return S_ERROR_BAD_PARAMETERS;
      }
      else if(pFilenamePattern[i] == 0x5C ) /* '\' */
      {
         /**
          * Must be directly followed by asterisk character or question-mark
          * character.
          */
         if (! ((pFilenamePattern[i+1] == '*' ||
                   pFilenamePattern[i+1] == '?')))
         {
            return S_ERROR_BAD_PARAMETERS;
         }
      }
      else if(pFilenamePattern[i] >= 0x7F )
      {
         return S_ERROR_BAD_PARAMETERS;
      }
   }

   return S_SUCCESS;
}



SST_ERROR SST_EXPORT_API SSTOpen(const char* pFilename,
                                 uint32_t    nFlags,
                                 uint32_t    nReserved,
                                 SST_HANDLE* phFile)
{
   TEEC_Session*     pSession;
   TEEC_Result       nError;
   TEEC_Operation    sOperation;
   uint32_t          nReturnOrigin;
   SST_ERROR         nErrorCode = SST_SUCCESS;

   if (phFile == NULL || nReserved != 0)
   {
      return SST_ERROR_BAD_PARAMETERS;
   }

   *phFile = SST_HANDLE_INVALID;

   nErrorCode = static_SSTCheckFileName(pFilename);
   if (nErrorCode != SST_SUCCESS)
   {
      return nErrorCode;
   }

   pSession = static_SSTGetSession();
   if (pSession == NULL)
   {
      return SST_ERROR_GENERIC;
   }

   sOperation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INOUT, TEEC_MEMREF_TEMP_INPUT, TEEC_NONE, TEEC_NONE);
   sOperation.params[0].value.a = 1;      /* Private storage */
   sOperation.params[0].value.b = nFlags; /* Access flags */
   sOperation.params[1].tmpref.buffer = (void*)pFilename;
   sOperation.params[1].tmpref.size   = strlen(pFilename);
   nError = TEEC_InvokeCommand(pSession,
                               SERVICE_SYSTEM_SST_OPEN_COMMAND_ID,   /* commandID */
                               &sOperation,                 /* IN OUT operation */
                               &nReturnOrigin               /* OUT returnOrigin, optional */
                              );
   if (nError == TEEC_SUCCESS)
   {
      *phFile = (SST_HANDLE)sOperation.params[0].value.a;
   }

   return static_SSTConvertErrorCode(nError);
}

SST_ERROR SST_EXPORT_API SSTCloseHandle(SST_HANDLE  hFile)
{
   TEEC_Session*     pSession;
   TEEC_Result        nError;
   TEEC_Operation    sOperation;
   uint32_t          nReturnOrigin;

   if (hFile == S_HANDLE_NULL)
   {
      return SST_SUCCESS;
   }

   pSession = static_SSTGetSession();
   if (pSession == NULL)
   {
      return SST_ERROR_GENERIC;
   }

   sOperation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_NONE, TEEC_NONE, TEEC_NONE);
   sOperation.params[0].value.a = hFile;
   nError = TEEC_InvokeCommand(pSession,
                               SERVICE_SYSTEM_SST_CLOSE_COMMAND_ID, /* commandID */
                               &sOperation,                  /* IN OUT operation */
                               &nReturnOrigin            /* OUT returnOrigin, optional */
                              );

   return static_SSTConvertErrorCode(nError);
}

SST_ERROR SST_EXPORT_API SSTWrite(SST_HANDLE       hFile,
                                  const uint8_t*   pBuffer,
                                  uint32_t         nSize)
{
   TEEC_Session*     pSession;
   TEEC_Result       nError;
   TEEC_Operation    sOperation;
   uint32_t          nReturnOrigin;

   if (pBuffer == NULL)
   {
      return SST_ERROR_BAD_PARAMETERS;
   }

   if (nSize == 0)
   {
      return SST_SUCCESS;
   }

   pSession = static_SSTGetSession();
   if (pSession == NULL)
   {
      return SST_ERROR_GENERIC;
   }

   sOperation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_MEMREF_TEMP_INPUT, TEEC_NONE, TEEC_NONE);
   sOperation.params[0].value.a       = hFile;
   sOperation.params[1].tmpref.buffer = (void*)pBuffer;
   sOperation.params[1].tmpref.size   = nSize;

   nError = TEEC_InvokeCommand(pSession,
                               SERVICE_SYSTEM_SST_WRITE_COMMAND_ID, /* commandID */
                               &sOperation,                  /* IN OUT operation */
                               &nReturnOrigin            /* OUT returnOrigin, optional */
                              );

   return static_SSTConvertErrorCode(nError);
}


SST_ERROR SST_EXPORT_API SSTRead(SST_HANDLE   hFile,
                                 uint8_t*     pBuffer,
                                 uint32_t     nSize,
                                 uint32_t*    pnCount)
{
   TEEC_Session*     pSession;
   TEEC_Result       nError;
   TEEC_Operation    sOperation;
   uint32_t          nReturnOrigin;

   if ((pBuffer == NULL) || (pnCount == NULL))
   {
      return SST_ERROR_BAD_PARAMETERS;
   }
   *pnCount = 0;

   pSession = static_SSTGetSession();
   if (pSession == NULL)
   {
      return SST_ERROR_GENERIC;
   }

   if (nSize == 0)
   {
      return SST_SUCCESS;
   }

   sOperation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_MEMREF_TEMP_OUTPUT, TEEC_NONE, TEEC_NONE);
   sOperation.params[0].value.a       = hFile;
   sOperation.params[1].tmpref.buffer = pBuffer;
   sOperation.params[1].tmpref.size   = nSize;

   nError = TEEC_InvokeCommand(pSession,
                               SERVICE_SYSTEM_SST_READ_COMMAND_ID, /* commandID */
                               &sOperation,                  /* IN OUT operation */
                               &nReturnOrigin            /* OUT returnOrigin, optional */
                              );

   *pnCount = sOperation.params[1].tmpref.size; /* The returned buffer size */
   return static_SSTConvertErrorCode(nError);
}

SST_ERROR SST_EXPORT_API SSTSeek(SST_HANDLE   hFile,
                                 int32_t     nOffset,
                                 SST_WHENCE   whence)
{
   TEEC_Session*     pSession;
   TEEC_Result       nError;
   TEEC_Operation    sOperation;
   uint32_t          nReturnOrigin;

   switch(whence)
   {
   case SST_SEEK_SET:
   case SST_SEEK_CUR:
   case SST_SEEK_END:
      break;
   default:
      return SST_ERROR_BAD_PARAMETERS;
   }

   pSession = static_SSTGetSession();
   if (pSession == NULL)
   {
      return SST_ERROR_GENERIC;
   }

   sOperation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_VALUE_INPUT, TEEC_NONE, TEEC_NONE);
   sOperation.params[0].value.a = hFile;
   sOperation.params[1].value.a = nOffset;
   sOperation.params[1].value.b = (uint32_t)whence;

   nError = TEEC_InvokeCommand(pSession,
                               SERVICE_SYSTEM_SST_SEEK_COMMAND_ID, /* commandID */
                               &sOperation,                  /* IN OUT operation */
                               &nReturnOrigin            /* OUT returnOrigin, optional */
                              );
   return static_SSTConvertErrorCode(nError);

}

static SST_ERROR SSTGetOffsetAndSize(SST_HANDLE   hFile, uint32_t* pnOffset, uint32_t* pnSize)
{
   TEEC_Session*     pSession;
   TEEC_Result       nError;
   TEEC_Operation    sOperation;
   uint32_t          nReturnOrigin;

   pSession = static_SSTGetSession();
   if (pSession == NULL)
   {
      return SST_ERROR_GENERIC;
   }

   if (pnOffset == NULL)
   {
      return SST_ERROR_BAD_PARAMETERS;
   }

   sOperation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INOUT, TEEC_NONE, TEEC_NONE, TEEC_NONE);
   sOperation.params[0].value.a = (uint32_t)hFile;

   nError = TEEC_InvokeCommand(pSession,
                               SERVICE_SYSTEM_SST_GET_OFFSET_AND_SIZE_COMMAND_ID, /* commandID */
                               &sOperation,                  /* IN OUT operation */
                               &nReturnOrigin            /* OUT returnOrigin, optional */
                              );

   if (pnOffset != NULL)
   {
      *pnOffset = sOperation.params[0].value.a;
   }
   if (pnSize != NULL)
   {
      *pnSize = sOperation.params[0].value.b;
   }
   return static_SSTConvertErrorCode(nError);

}

SST_ERROR SST_EXPORT_API SSTTell(SST_HANDLE   hFile,
                                 uint32_t*    pnPos)
{
   return SSTGetOffsetAndSize(hFile, pnPos, NULL);
}

SST_ERROR SST_EXPORT_API SSTGetSize(const char*  pFilename,
                                    uint32_t*    pnSize)
{
   TEEC_Session*     pSession;
   TEEC_Result       nError;
   TEEC_Operation    sOperation;
   uint32_t          nReturnOrigin;

   if ((pFilename == NULL) || (pnSize == NULL))
   {
      return SST_ERROR_BAD_PARAMETERS;
   }

   nError = static_SSTCheckFileName(pFilename);
   if (nError != SST_SUCCESS)
   {
      return nError;
   }

   pSession = static_SSTGetSession();
   if (pSession == NULL)
   {
      return SST_ERROR_GENERIC;
   }

   sOperation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INOUT, TEEC_MEMREF_TEMP_INPUT, TEEC_NONE, TEEC_NONE);
   sOperation.params[0].value.a = 1; /* private storage */
   sOperation.params[0].value.b = 0;
   sOperation.params[1].tmpref.buffer = (void*)pFilename;
   sOperation.params[1].tmpref.size   = strlen(pFilename);

   nError = TEEC_InvokeCommand(pSession,
                               SERVICE_SYSTEM_SST_GET_SIZE_COMMAND_ID, /* commandID */
                               &sOperation,                  /* IN OUT operation */
                               &nReturnOrigin            /* OUT returnOrigin, optional */
                              );

   *pnSize = sOperation.params[0].value.a;
   return static_SSTConvertErrorCode(nError);
}


SST_ERROR SST_EXPORT_API SSTEof( SST_HANDLE   hFile,
                                 bool*        pbEof)
{
   uint32_t nOffset;
   uint32_t nSize;
   SST_ERROR nError;
   if (pbEof == NULL)
      return SST_ERROR_BAD_PARAMETERS;
   nError = SSTGetOffsetAndSize(hFile, &nOffset, &nSize);
   if (nError == SST_SUCCESS)
   {
      if (nOffset >= nSize)
      {
         *pbEof = true;
      }
      else
      {
         *pbEof = false;
      }
   }
   return nError;
}

SST_ERROR SST_EXPORT_API SSTCloseAndDelete(SST_HANDLE  hFile)
{
   TEEC_Session*     pSession;
   TEEC_Result       nError;
   TEEC_Operation    sOperation;
   uint32_t          nReturnOrigin;

   pSession = static_SSTGetSession();
   if (pSession == NULL)
   {
      return SST_ERROR_GENERIC;
   }

   sOperation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_NONE, TEEC_NONE, TEEC_NONE);
   sOperation.params[0].value.a = hFile;

   nError = TEEC_InvokeCommand(pSession,
                               SERVICE_SYSTEM_SST_CLOSE_DELETE_COMMAND_ID, /* commandID */
                               &sOperation,                  /* IN OUT operation */
                               &nReturnOrigin            /* OUT returnOrigin, optional */
                              );
   return static_SSTConvertErrorCode(nError);
}

SST_ERROR SST_EXPORT_API SSTTruncate(SST_HANDLE hFile, uint32_t nLength)
{
   TEEC_Session*     pSession;
   TEEC_Result       nError;
   TEEC_Operation    sOperation;
   uint32_t          nReturnOrigin;

   pSession = static_SSTGetSession();
   if (pSession == NULL)
   {
      return SST_ERROR_GENERIC;
   }

   sOperation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_NONE, TEEC_NONE, TEEC_NONE);
   sOperation.params[0].value.a = hFile;
   sOperation.params[0].value.b = nLength;

   nError = TEEC_InvokeCommand(pSession,
                               SERVICE_SYSTEM_SST_TRUNCATE_COMMAND_ID, /* commandID */
                               &sOperation,                  /* IN OUT operation */
                               &nReturnOrigin            /* OUT returnOrigin, optional */
                              );
   return static_SSTConvertErrorCode(nError);
}

SST_ERROR SST_EXPORT_API SSTRename(SST_HANDLE hFile,
                                   const char* pNewFilename)
{
   TEEC_Session*     pSession;
   TEEC_Result       nError;
   TEEC_Operation    sOperation;
   uint32_t          nReturnOrigin;

   pSession = static_SSTGetSession();
   if (pSession == NULL)
   {
      return SST_ERROR_GENERIC;
   }

   if (pNewFilename == NULL)
   {
      return SST_ERROR_BAD_PARAMETERS;
   }

   nError = static_SSTCheckFileName(pNewFilename);
   if (nError != SST_SUCCESS)
   {
      return nError;
   }

   sOperation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_MEMREF_TEMP_INPUT, TEEC_NONE, TEEC_NONE);
   sOperation.params[0].value.a = hFile;
   sOperation.params[1].tmpref.buffer = (void*)pNewFilename;
   sOperation.params[1].tmpref.size   = strlen(pNewFilename);

   nError = TEEC_InvokeCommand(pSession,
                               SERVICE_SYSTEM_SST_RENAME_COMMAND_ID, /* commandID */
                               &sOperation,                  /* IN OUT operation */
                               &nReturnOrigin            /* OUT returnOrigin, optional */
                              );
      return static_SSTConvertErrorCode(nError);
}

SST_ERROR SST_EXPORT_API SSTEnumerationStart(const char* pFilenamePattern,
                                             uint32_t  nReserved1,
                                             uint32_t  nReserved2,
                                             SST_HANDLE* phFileEnumeration)
{
   TEEC_Session*     pSession;
   TEEC_Result       nError;
   TEEC_Operation    sOperation;
   uint32_t          nReturnOrigin;

   if (nReserved1!=0 || nReserved2!=0)
   {
      return SST_ERROR_BAD_PARAMETERS;
   }
   if (phFileEnumeration==NULL)
   {
      return SST_ERROR_BAD_PARAMETERS;
   }
   *phFileEnumeration = SST_HANDLE_INVALID;

   nError = static_SSTCheckPattern(pFilenamePattern);
   if (nError != SST_SUCCESS)
   {
      return nError;
   }

   pSession = static_SSTGetSession();
   if (pSession == NULL)
   {
      return SST_ERROR_GENERIC;
   }

   sOperation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INOUT, TEEC_MEMREF_TEMP_INPUT, TEEC_NONE, TEEC_NONE);
   sOperation.params[0].value.a = 1;      /* Private storage */
   sOperation.params[1].tmpref.buffer = (void*)pFilenamePattern;
   if (pFilenamePattern != NULL)
   {
      sOperation.params[1].tmpref.size   = strlen(pFilenamePattern);
   }
   else
   {
      sOperation.params[1].tmpref.size   = 0;
   }

   nError = TEEC_InvokeCommand(pSession,
                               SERVICE_SYSTEM_SST_ENUM_START_COMMAND_ID, /* commandID */
                               &sOperation,                  /* IN OUT operation */
                               &nReturnOrigin            /* OUT returnOrigin, optional */
                              );

   *phFileEnumeration = (SST_HANDLE)sOperation.params[0].value.a;
   return static_SSTConvertErrorCode(nError);
}

SST_ERROR SST_EXPORT_API SSTEnumerationCloseHandle(SST_HANDLE hFileEnumeration)
{
   TEEC_Session*     pSession;
   TEEC_Result       nError;
   TEEC_Operation    sOperation;
   uint32_t          nReturnOrigin;

   pSession = static_SSTGetSession();
   if (pSession == NULL)
   {
      return SST_ERROR_GENERIC;
   }

   sOperation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_NONE, TEEC_NONE, TEEC_NONE);
   sOperation.params[0].value.a = hFileEnumeration;

   nError = TEEC_InvokeCommand(pSession,
                               SERVICE_SYSTEM_SST_ENUM_CLOSE_COMMAND_ID, /* commandID */
                               &sOperation,                  /* IN OUT operation */
                               &nReturnOrigin                /* OUT returnOrigin, optional */
                              );

   return static_SSTConvertErrorCode(nError);
}

SST_ERROR SST_EXPORT_API SSTEnumerationGetNext(SST_HANDLE      hFileEnumeration,
                                               SST_FILE_INFO**   ppFileInfo)

{
   TEEC_Session*     pSession;
   TEEC_Result       nError;
   TEEC_Operation    sOperation;
   uint32_t          nReturnOrigin;
   SST_FILE_INFO*    pInfo = NULL;
   char              sFilename[SST_MAX_FILENAME];

   if (ppFileInfo==NULL)
   {
      return SST_ERROR_BAD_PARAMETERS;
   }

   pSession = static_SSTGetSession();
   if (pSession == NULL)
   {
      return SST_ERROR_GENERIC;
   }

   sOperation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INOUT, TEEC_MEMREF_TEMP_OUTPUT, TEEC_NONE, TEEC_NONE);
   sOperation.params[0].value.a = hFileEnumeration;
   sOperation.params[1].tmpref.buffer = sFilename;
   sOperation.params[1].tmpref.size   = SST_MAX_FILENAME;

   nError = TEEC_InvokeCommand(pSession,
                               SERVICE_SYSTEM_SST_ENUM_GETNEXT_COMMAND_ID, /* commandID */
                               &sOperation,                  /* IN OUT operation */
                               &nReturnOrigin            /* OUT returnOrigin, optional */
                              );

   if (nError == TEEC_SUCCESS)
   {
      if (sOperation.params[1].tmpref.size <= SST_MAX_FILENAME)
      {
         pInfo = (SST_FILE_INFO*)malloc(sizeof(SST_FILE_INFO));
         if (pInfo == NULL)
         {
            return SST_ERROR_OUT_OF_MEMORY;
         }
         pInfo->pName = (char*)malloc(sOperation.params[1].tmpref.size+1);
         if (pInfo->pName == NULL)
         {
            free(pInfo);
            return SST_ERROR_OUT_OF_MEMORY;
         }
         memcpy(pInfo->pName, sFilename, sOperation.params[1].tmpref.size);
         /* Add zero terminator */
         pInfo->pName[sOperation.params[1].tmpref.size] = 0;
         pInfo->nSize = sOperation.params[0].value.b;
      }
   }
  *ppFileInfo = pInfo;
   return static_SSTConvertErrorCode(nError);
 }

SST_ERROR SST_EXPORT_API SSTDestroyFileInfo(SST_FILE_INFO*   pFileInfo)
{
   TEEC_Session*  pSession;

   pSession = static_SSTGetSession();
   if (pSession == NULL)
   {
      return SST_ERROR_GENERIC;
   }

   if (pFileInfo != NULL)
   {
      free(pFileInfo->pName);
      free(pFileInfo);
   }
   return SST_SUCCESS;
}
