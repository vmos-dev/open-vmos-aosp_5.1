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

#include "tee_client_api.h"
#include "schannel6_protocol.h"
#include "s_error.h"
#include "s_version.h"

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <linux/limits.h>
#include <time.h>
#include <sys/time.h>

/*
 * SCX_VERSION_INFORMATION_BUFFER structure description
 * Description of the sVersionBuffer handed over from user space to kernel space
 * This field is filled after an IOCTL call and handed back to user space
 */
typedef struct
{
   uint8_t sDriverDescription[65];
   uint8_t sSecureWorldDescription[65];
} SCX_VERSION_INFORMATION_BUFFER;


/* The IOCTLs to the driver */
#define IOCTL_SCX_GET_VERSION \
         _IO('z', 0)

#define IOCTL_SCX_EXCHANGE \
         _IOWR('z', 1, SCHANNEL6_COMMAND)

#define IOCTL_SCX_GET_DESCRIPTION \
         _IOR('z', 2, SCX_VERSION_INFORMATION_BUFFER)


/* Expected driver interface version. */
#define SM_DRIVER_VERSION 0x04000000

#define SCX_DEFAULT_DEVICE_NAME "tf_driver"

#define SCX_PARAM_TYPE_GET(nParamTypes, i) (((nParamTypes) >> (4*i)) & 0xF)

#define VAR_NOT_USED(variable)  do{(void)(variable);}while(0);

#define SIZE_4KB  0x1000
#define SIZE_1MB  0x100000

/* ------------------------------------------------------------------------ */
/*    UTILS                                                                 */
/* ------------------------------------------------------------------------ */
#ifdef NDEBUG
/* Compile-out the traces */
#define TRACE_ERROR(...)
#define TRACE_WARNING(...)
#define TRACE_INFO(...)
#else
static void TRACE_ERROR(const char* format, ...)
{
   va_list ap;
   va_start(ap, format);
   fprintf(stderr, "TRACE: ERROR: ");
   vfprintf(stderr, format, ap);
   fprintf(stderr, "\n");
   va_end(ap);
}

static void TRACE_WARNING(const char* format, ...)
{
   va_list ap;
   va_start(ap, format);
   fprintf(stderr, "TRACE: WARNING: ");
   vfprintf(stderr, format, ap);
   fprintf(stderr, "\n");
   va_end(ap);
}

static void TRACE_INFO(const char* format, ...)
{
   va_list ap;
   va_start(ap, format);
   fprintf(stderr, "TRACE: ");
   vfprintf(stderr, format, ap);
   fprintf(stderr, "\n");
   va_end(ap);
}
#endif /* NDEBUG */


/*
 * ====================================================
 *                 Internal functions
 * =====================================================
*/

static void scxYield(void)
{
   sleep(0);
}

/* ------------------------------------------------------------------------ */


/*
 * Exchange a message with the Secure World
 * by calling the ioctl command of the linux driver
 *
 * @param pContext
 * @param pCommand a SChannel command message that must have been filled except for the operation parameters
 * @param pAnswer  a placeholder for the SChannel answer
 * @param pOperation a TEEC_Operation structure that contains the operation parameters (and types)
 *                   and is updated with the SChannel answer data as appropriate. This parameter is
 *                   used only for the open and invoke operations
 */
static TEEC_Result scxExchangeMessage(
   IN  TEEC_Context*     pContext,
   IN  SCHANNEL6_COMMAND* pCommand,
   OUT SCHANNEL6_ANSWER*  pAnswer,
   IN TEEC_Operation* pOperation)
{
   TEEC_Result nResult = TEEC_SUCCESS;

   TRACE_INFO("scxExchangeMessage[0x%X]\n",pContext);

   if (pOperation != NULL)
   {
      /* Determine message parameters from operation parameters */
      uint32_t i;
      SCHANNEL6_COMMAND_PARAM* pSCXParams;

      /* Note that nParamType is at the same position in an open and an invoke message */
      pCommand->sHeader.nMessageInfo = pOperation->paramTypes;

      if (pCommand->sHeader.nMessageType == SCX_OPEN_CLIENT_SESSION)
      {
         pSCXParams = pCommand->sOpenClientSession.sParams;
      }
      else
      {
         /* An invoke-command */
         pSCXParams = pCommand->sInvokeClientCommand.sParams;
      }

      for (i = 0; i < 4; i++)
      {
         uint32_t nTEECParamType = SCX_PARAM_TYPE_GET(pOperation->paramTypes, i);
         TEEC_Parameter*  pTEECParam = &pOperation->params[i];
         SCHANNEL6_COMMAND_PARAM* pSCXParam = &pSCXParams[i];

         if (nTEECParamType & SCX_PARAM_TYPE_MEMREF_FLAG)
         {
            if (nTEECParamType & SCX_PARAM_TYPE_REGISTERED_MEMREF_FLAG)
            {
               /* A registered memref */
               pSCXParam->sMemref.hBlock  = pTEECParam->memref.parent->imp._hBlock;
               if (nTEECParamType == TEEC_MEMREF_WHOLE)
               {
                  /* A memref on the whole shared memory */
                  /* Set the direction from the shared memory flags */
                  pCommand->sInvokeClientCommand.nParamTypes |=
                     (pTEECParam->memref.parent->flags & (SCX_PARAM_TYPE_INPUT_FLAG | SCX_PARAM_TYPE_OUTPUT_FLAG))
                     << (4*i);
                  pSCXParam->sMemref.nSize   = pTEECParam->memref.parent->size;
                  pSCXParam->sMemref.nOffset = 0;
               }
               else
               {
                  /* A partial memref */
                  pSCXParam->sMemref.nSize   = pTEECParam->memref.size;
                  pSCXParam->sMemref.nOffset = pTEECParam->memref.offset;
               }
            }
            else
            {
               /* A temporary memref */
               /* Set nOffset to the address in the client. This allows the server
                 to allocate a block with the same alignment and also to
                 detect a NULL tmpref.
               */
               pSCXParam->sTempMemref.nOffset = (uint32_t)pTEECParam->tmpref.buffer;
               pSCXParam->sTempMemref.nDescriptor = (uint32_t)pTEECParam->tmpref.buffer;
               pSCXParam->sTempMemref.nSize = pTEECParam->tmpref.size;
            }
         }
         else if (nTEECParamType & SCX_PARAM_TYPE_INPUT_FLAG)
         {
            /* An input value */
            pSCXParam->sValue.a = pTEECParam->value.a;
            pSCXParam->sValue.b = pTEECParam->value.b;
         }
      }
   }

   pCommand->sHeader.nOperationID = (uint32_t)pAnswer;

   nResult = ioctl((S_HANDLE)pContext->imp._hConnection, IOCTL_SCX_EXCHANGE, pCommand);
   if (nResult != S_SUCCESS)
   {
      TRACE_INFO("scxExchangeMessage[0x%X]: Ioctl returned error: 0x%x (0x%x - %d)\n",pContext,nResult,errno,errno);
      switch(errno)
      {
         case ENOMEM:
            nResult=TEEC_ERROR_OUT_OF_MEMORY;
            break;
         case EACCES:
            nResult=TEEC_ERROR_ACCESS_DENIED;
            break;
         default:
            nResult=TEEC_ERROR_COMMUNICATION;
            break;
      }
   }

   if (pOperation != NULL)
   {
      /* Update the operation parameters from the answer message */
      uint32_t   i;
      SCHANNEL6_ANSWER_PARAM *  pSCXAnswers;
      if (pAnswer->sHeader.nMessageType == SCX_OPEN_CLIENT_SESSION)
      {
         /* Open session */
         pSCXAnswers = pAnswer->sOpenClientSession.sAnswers;
      }
      else
      {
         /* Invoke case */
         pSCXAnswers = pAnswer->sInvokeClientCommand.sAnswers;
      }

      for (i = 0; i < 4; i++)
      {
         uint32_t   nSCXParamType;
         nSCXParamType = SCX_GET_PARAM_TYPE(pCommand->sHeader.nMessageInfo, i);
         if (nSCXParamType & SCX_PARAM_TYPE_OUTPUT_FLAG)
         {
            if (nSCXParamType & SCX_PARAM_TYPE_MEMREF_FLAG)
            {
               /* Trick: the size field is at the same position in a memref or a tmpref */
               pOperation->params[i].memref.size = pSCXAnswers[i].sSize.nSize;
            }
            else
            {
               /* An output value */
               pOperation->params[i].value.a = pSCXAnswers[i].sValue.a;
               pOperation->params[i].value.b = pSCXAnswers[i].sValue.b;
            }
         }
      }
   }

   return nResult;
}

/* ------------------------------------------------------------------------ */

static void* scxAllocateSharedMemory(
   IN uint32_t nLength)
{
   if (nLength == 0)
   {
      /* This is valid, although we don't want to call mmap.
         Just return a dummy non-NULL pointer */
      return (void*)0x10;
   }
   else
   {
      return mmap(
         0,nLength,
         PROT_READ | PROT_WRITE,
         MAP_SHARED | MAP_ANONYMOUS,
         0,0);
   }
}

/* ------------------------------------------------------------------------ */

static void scxReleaseSharedMemory(IN void* pBuffer,
                                   IN uint32_t nLength)
{
   if (nLength == 0)
   {
      return;
   }
   if (munmap(pBuffer, nLength)!= 0)
   {
       TRACE_WARNING("scxReleaseSharedMemory returned 0x%x \n",errno);
   }
}
/* ------------------------------------------------------------------------ */

uint64_t scxGetCurrentTime(void)
{
   uint64_t currentTime = 0;
   struct timeval now;

   gettimeofday(&now,NULL);
   currentTime = now.tv_sec;
   currentTime = (currentTime * 1000) + (now.tv_usec / 1000);

   return currentTime;
}
/* ------------------------------------------------------------------------ */

/*
 * ====================================================
 *                TEE Client API
 * =====================================================
*/

/**
 * Get a time-limit equal to now + relative timeout expressed in milliseconds.
 **/
void TEEC_GetTimeLimit(
    TEEC_Context*    sContext,
    uint32_t         nTimeout,
    TEEC_TimeLimit*  sTimeLimit)
{
   uint64_t nTimeLimit = 0;
   VAR_NOT_USED(sContext);

   TRACE_INFO("TEEC_GetTimeLimit(0x%X, %u ms)", sContext, nTimeout);

   if (nTimeout == 0xFFFFFFFF )
   {
      /* Infinite timeout */
      nTimeLimit = SCTIME_INFINITE;
   }
   else
   {
       nTimeLimit = scxGetCurrentTime() + nTimeout;
   }
   TRACE_INFO("GetTimeLimit %ld\n",nTimeLimit);
   memcpy(sTimeLimit, &nTimeLimit, sizeof(TEEC_TimeLimit));
}

//-----------------------------------------------------------------------------------------------------
TEEC_Result TEEC_InitializeContext(
    const char*   pDeviceName,
    TEEC_Context* pContext)
{

  TEEC_Result nError = TEEC_SUCCESS;
   S_HANDLE hDriver   = S_HANDLE_NULL;
   char sFullDeviceName[PATH_MAX];
   uint32_t nVersion;

   if(pDeviceName == NULL)
   {
      pDeviceName = SCX_DEFAULT_DEVICE_NAME;
   }
   strcpy(sFullDeviceName, "/dev/");
   strcat(sFullDeviceName, pDeviceName);

   hDriver = open(sFullDeviceName, O_RDWR, 0);

   if (hDriver == (uint32_t)-1)
   {
      TRACE_ERROR("scxOpen: open() failed 0x%x\n", errno);
      switch(errno)
      {
         case ENOMEM:
            nError = TEEC_ERROR_OUT_OF_MEMORY;
            goto error;
         case EINTR:
             break;
         default:
            nError = TEEC_ERROR_COMMUNICATION;
            goto error;
      }
   }
   fcntl(hDriver, F_SETFD, FD_CLOEXEC);
   nVersion = ioctl(hDriver, IOCTL_SCX_GET_VERSION);
   if (nVersion != SM_DRIVER_VERSION)
   {
      TRACE_ERROR("scxOpen: Not expected driver version: 0x%x instead of 0x%x\n", nVersion,SM_DRIVER_VERSION);
      switch(errno)
      {
         case ENOMEM:
            nError=TEEC_ERROR_OUT_OF_MEMORY;
            break;
         default:
            nError=TEEC_ERROR_COMMUNICATION;
            break;
      }
      close(hDriver);
   }
error:
   if(nError == TEEC_SUCCESS)
   {
       pContext->imp._hConnection = hDriver;
   }
   else
   {
      TRACE_ERROR("scxOpen failed 0x%x\n", nError);
      pContext->imp._hConnection = 0;
   }

   return nError;
}

//-----------------------------------------------------------------------------------------------------
void TEEC_FinalizeContext(TEEC_Context* pContext)
{
   TRACE_INFO("TEEC_FinalizeContext[0x%X]", pContext);

   if (pContext == NULL) return;

   close(pContext->imp._hConnection);
   pContext->imp._hConnection = 0;
}

//-----------------------------------------------------------------------------------------------------
TEEC_Result TEEC_OpenSession (
    TEEC_Context*    context,
    TEEC_Session*    session,      /* OUT */
    const TEEC_UUID* destination,  /* The trusted application UUID we want to open the session with */
    uint32_t         connectionMethod, /* LoginType*/
    void*            connectionData,  /* LoginData */
    TEEC_Operation*  operation,    /* payload. If operation is NULL then no data buffers are exchanged with the Trusted Application, and the operation cannot be cancelled by the Client Application */
    uint32_t*        errorOrigin)
{
  return TEEC_OpenSessionEx(context,
                            session,
                            NULL,
                            destination,
                            connectionMethod,
                            connectionData,
                            operation,
                            errorOrigin);
}

//-----------------------------------------------------------------------------------------------------
void TEEC_CloseSession (TEEC_Session* session)
{
   TEEC_Context*     context;
   SCHANNEL6_ANSWER  sAnswer;
   SCHANNEL6_COMMAND sCommand;
   if (session == NULL) return;
   context = session->imp._pContext;
   memset(&sCommand,0,sizeof(sCommand));
   sCommand.sHeader.nMessageType = SCX_CLOSE_CLIENT_SESSION;
   sCommand.sHeader.nMessageSize = (sizeof(SCHANNEL6_CLOSE_CLIENT_SESSION_COMMAND)
                                         - sizeof(SCHANNEL6_COMMAND_HEADER))/sizeof(uint32_t);
   sCommand.sCloseClientSession.hClientSession = session->imp._hClientSession;
   scxExchangeMessage(context, &sCommand, &sAnswer, NULL);
   /* we ignore the error code of scxExchangeMessage */
   session->imp._hClientSession = S_HANDLE_NULL;
   session->imp._pContext = NULL;
}

//-----------------------------------------------------------------------------------------------------
TEEC_Result TEEC_InvokeCommand(
    TEEC_Session*     session,
    uint32_t          commandID,
    TEEC_Operation*   operation,
    uint32_t*         errorOrigin)
{
    return TEEC_InvokeCommandEx(session,
                            NULL,
                            commandID,
                            operation,
                            errorOrigin);
}


//-----------------------------------------------------------------------------------------------------
/* Used to implement both register and allocate */
static TEEC_Result TEEC_RegisterSharedMemory0(
    TEEC_Context*      context,
    TEEC_SharedMemory* sharedMem)
{
   TEEC_Result nResult;
   SCHANNEL6_COMMAND sCommand;
   SCHANNEL6_ANSWER  sAnswer;

   TRACE_INFO("TEEC_RegisterSharedMemory0 (%p, %p)",context, sharedMem);
   memset(&sCommand, 0, sizeof(sCommand));

   sCommand.sRegisterSharedMemory.nMessageSize = (sizeof(SCHANNEL6_REGISTER_SHARED_MEMORY_COMMAND) - sizeof(SCHANNEL6_COMMAND_HEADER))/4;
   sCommand.sRegisterSharedMemory.nMessageType             = SCX_REGISTER_SHARED_MEMORY;
   sCommand.sRegisterSharedMemory.nMemoryFlags             = sharedMem->flags;
   sCommand.sRegisterSharedMemory.nSharedMemSize           = sharedMem->size;
   sCommand.sRegisterSharedMemory.nSharedMemStartOffset    = 0;
   sCommand.sRegisterSharedMemory.nSharedMemDescriptors[0] = (uint32_t)sharedMem->buffer;
   nResult = scxExchangeMessage(context,
                &sCommand,
                &sAnswer,
                NULL);
   if (nResult == TEEC_SUCCESS)
   {
       nResult = sAnswer.sRegisterSharedMemory.nErrorCode;
   }
   if (nResult == TEEC_SUCCESS)
   {
      sharedMem->imp._pContext = context;
      sharedMem->imp._hBlock = sAnswer.sRegisterSharedMemory.hBlock;
   }
   return nResult;
}


//-----------------------------------------------------------------------------------------------------
TEEC_Result TEEC_RegisterSharedMemory(
    TEEC_Context*      context,
    TEEC_SharedMemory* sharedMem)
{
   TRACE_INFO("TEEC_RegisterSharedMemory (%p)",context);
   sharedMem->imp._pContext = NULL;
   sharedMem->imp._hBlock = S_HANDLE_NULL;
   sharedMem->imp._bAllocated = false;
   return TEEC_RegisterSharedMemory0(context, sharedMem);
}

//-----------------------------------------------------------------------------------------------------
TEEC_Result TEEC_AllocateSharedMemory(
    TEEC_Context*      context,
    TEEC_SharedMemory* sharedMem)
{
   TEEC_Result nResult;
   TRACE_INFO("TEEC_AllocateSharedMemory (%p)",context);

   sharedMem->imp._pContext = NULL;
   sharedMem->imp._hBlock = S_HANDLE_NULL;
   sharedMem->buffer = scxAllocateSharedMemory(sharedMem->size);
   if (sharedMem->buffer == NULL)
   {
      return TEEC_ERROR_OUT_OF_MEMORY;
   }
   sharedMem->imp._bAllocated = true;
   nResult = TEEC_RegisterSharedMemory0(context, sharedMem);
   if (nResult != TEEC_SUCCESS)
   {
      scxReleaseSharedMemory(sharedMem->buffer,sharedMem->size);
      sharedMem->buffer = NULL;
   }
   return nResult;
}

//-----------------------------------------------------------------------------------------------------
void TEEC_ReleaseSharedMemory (
    TEEC_SharedMemory* sharedMem)
{
   SCHANNEL6_ANSWER  sAnswer;
   SCHANNEL6_COMMAND sMessage;
   TEEC_Context* context;

   context = (TEEC_Context *)sharedMem->imp._pContext;
   memset(&sMessage, 0, sizeof(SCHANNEL6_COMMAND));
   sMessage.sReleaseSharedMemory.nMessageType = SCX_RELEASE_SHARED_MEMORY;
   sMessage.sReleaseSharedMemory.nMessageSize = (sizeof(SCHANNEL6_RELEASE_SHARED_MEMORY_COMMAND)
                                    - sizeof(SCHANNEL6_COMMAND_HEADER))/sizeof(uint32_t);
   sMessage.sReleaseSharedMemory.hBlock = sharedMem->imp._hBlock;
   scxExchangeMessage(context,&sMessage, &sAnswer, NULL);
   if (sharedMem->imp._bAllocated)
   {
       scxReleaseSharedMemory(sharedMem->buffer,sharedMem->size);
       /* Update parameters:
       * In this case the Implementation MUST set the buffer and size fields of the sharedMem structure
       * to NULL and 0 respectively before returning.
       */
       sharedMem->buffer = NULL;
       sharedMem->size = 0;
   }
   sharedMem->imp._pContext = NULL;
   sharedMem->imp._hBlock = S_HANDLE_NULL;
}

//-----------------------------------------------------------------------------------------------------
void TEEC_RequestCancellation(TEEC_Operation* operation)
{
   uint32_t nOperationState;
   TEEC_Result       nResult;

   if (operation == NULL) return;

retry:
   nOperationState = operation->started;
   if (nOperationState == 2)
    {
      /* Operation already finished. Return immediately */
      return;
   }
   else if (nOperationState == 1)
   {
       /* Operation is in progress */
       TEEC_Context*     context;
       SCHANNEL6_ANSWER  sAnswer;
       SCHANNEL6_COMMAND sMessage;

       context = operation->imp._pContext;

       memset(&sMessage,0,sizeof(sMessage));
       sMessage.sHeader.nMessageType = SCX_CANCEL_CLIENT_OPERATION;
       sMessage.sHeader.nMessageSize = (sizeof(SCHANNEL6_CANCEL_CLIENT_OPERATION_COMMAND) - sizeof(SCHANNEL6_COMMAND_HEADER))/4;
       sMessage.sCancelClientOperation.hClientSession = operation->imp._hSession;
       sMessage.sCancelClientOperation.nCancellationID = (uint32_t)operation;
       nResult = scxExchangeMessage(context,&sMessage, &sAnswer, NULL);

       if (nResult != TEEC_SUCCESS)
       {
           /* Communication failure. Ignore the error: the operation is already cancelled anyway */
           return;
       }
       if (sAnswer.sCancelClientOperation.nErrorCode == S_SUCCESS)
       {
           /* Command was successfully cancelled */
           return;
       }
       /* Otherwise, the command has not yet reached the secure world or has already finished and we must retry */
   }
   /* This applies as well when nOperationState == 0. In this case, the operation has not yet
      started yet and we don't even have a pointer to the context */
   scxYield();
   goto retry;
}


//-----------------------------------------------------------------------------------------------------
TEEC_Result TEEC_ReadSignatureFile(
                                   void**    ppSignatureFile,
                                   uint32_t* pnSignatureFileLength)
{
   TEEC_Result nErrorCode = TEEC_SUCCESS;

   uint32_t      nBytesRead;
   uint32_t      nSignatureSize = 0;
   uint8_t*     pSignature = NULL;
   FILE*    pSignatureFile = NULL;
   char     sFileName[PATH_MAX + 1 + 5];  /* Allocate room for the signature extension */
   long     nFileSize;

   *pnSignatureFileLength = 0;
   *ppSignatureFile = NULL;

   if (realpath("/proc/self/exe", sFileName) == NULL)
   {
      TRACE_ERROR("TEEC_ReadSignatureFile: realpath failed [%d]", errno);
      return TEEC_ERROR_OS;
   }

   /* Work out the signature file name */
   strcat(sFileName, ".ssig");

   pSignatureFile = fopen(sFileName, "rb");
   if (pSignatureFile == NULL)
   {
      /* Signature doesn't exist */
       return TEEC_ERROR_ITEM_NOT_FOUND;
   }

   if (fseek(pSignatureFile, 0, SEEK_END) != 0)
   {
      TRACE_ERROR("TEEC_ReadSignatureFile: fseek(%s) failed [%d]",
                     sFileName, errno);
      nErrorCode = TEEC_ERROR_OS;
      goto error;
   }

   nFileSize = ftell(pSignatureFile);
   if (nFileSize < 0)
   {
      TRACE_ERROR("TEEC_ReadSignatureFile: ftell(%s) failed [%d]",
                     sFileName, errno);
      nErrorCode = TEEC_ERROR_OS;
      goto error;
   }

   nSignatureSize = (uint32_t)nFileSize;

   if (nSignatureSize != 0)
   {
      pSignature = malloc(nSignatureSize);
      if (pSignature == NULL)
      {
         TRACE_ERROR("TEEC_ReadSignatureFile: Heap - Out of memory for %u bytes",
                        nSignatureSize);
         nErrorCode = TEEC_ERROR_OUT_OF_MEMORY;
         goto error;
      }

      rewind(pSignatureFile);

      nBytesRead = fread(pSignature, 1, nSignatureSize, pSignatureFile);
      if (nBytesRead < nSignatureSize)
      {
         TRACE_ERROR("TEEC_ReadSignatureFile: fread failed [%d]", errno);
         nErrorCode = TEEC_ERROR_OS;
         goto error;
      }
   }

   fclose(pSignatureFile);

   *pnSignatureFileLength = nSignatureSize;
   *ppSignatureFile = pSignature;

   return S_SUCCESS;

error:
   fclose(pSignatureFile);
   free(pSignature);

   return nErrorCode;
}

//-----------------------------------------------------------------------------------------------------
TEEC_Result TEEC_OpenSessionEx (
    TEEC_Context*         context,
    TEEC_Session*         session,      /* OUT */
    const TEEC_TimeLimit* timeLimit,
    const TEEC_UUID*      destination,  /* The trusted application UUID we want to open the session with */
    uint32_t              connectionMethod, /* LoginType*/
    void*                 connectionData,  /* LoginData */
    TEEC_Operation*       operation,    /* payload. If operation is NULL then no data buffers are exchanged with the Trusted Application, and the operation cannot be cancelled by the Client Application */
    uint32_t*             returnOrigin)
{
   TEEC_Result nError;
   uint32_t nReturnOrigin;
   SCHANNEL6_ANSWER  sAnswer;
   SCHANNEL6_COMMAND sCommand;

   memset(&sCommand, 0, sizeof(SCHANNEL6_COMMAND));

   sCommand.sHeader.nMessageType = SCX_OPEN_CLIENT_SESSION;
   sCommand.sHeader.nMessageSize = (sizeof(SCHANNEL6_OPEN_CLIENT_SESSION_COMMAND) - 20 -sizeof(SCHANNEL6_COMMAND_HEADER))/sizeof(uint32_t);
   if (timeLimit == NULL)
   {
      sCommand.sOpenClientSession.sTimeout = SCTIME_INFINITE;
   }
   else
   {
      sCommand.sOpenClientSession.sTimeout = *(uint64_t*)timeLimit;
   }
   sCommand.sOpenClientSession.sDestinationUUID   = *((S_UUID*)destination);
   sCommand.sOpenClientSession.nLoginType         = connectionMethod;
   if ((connectionMethod == TEEC_LOGIN_GROUP)||(connectionMethod == TEEC_LOGIN_GROUP_APPLICATION))
   {
       /* connectionData MUST point to a uint32_t which contains the group
       * which this Client Application wants to connect as. The Linux Driver
       * is responsible for securely ensuring that the Client Application
       * instance is actually a member of this group.
       */
       if (connectionData != NULL)
       {
           *(uint32_t*)sCommand.sOpenClientSession.sLoginData = *(uint32_t*)connectionData;
           sCommand.sHeader.nMessageSize += sizeof(uint32_t);
       }
   }
   sCommand.sOpenClientSession.nCancellationID    = (uint32_t)operation; // used for TEEC_RequestCancellation

   if (operation != NULL)
   {
       operation->imp._pContext = context;
       operation->imp._hSession = S_HANDLE_NULL;
       operation->started = 1;
   }

   nError = scxExchangeMessage(context, &sCommand, &sAnswer, operation);

   if (operation != NULL) operation->started = 2;

   if (nError != TEEC_SUCCESS)
   {
       nReturnOrigin = TEEC_ORIGIN_COMMS;
   }
   else
   {
       nError = sAnswer.sOpenClientSession.nErrorCode;
       nReturnOrigin = sAnswer.sOpenClientSession.nReturnOrigin;
   }

   if (returnOrigin != NULL) *returnOrigin = nReturnOrigin;

   if (nError == S_SUCCESS)
   {
       session->imp._hClientSession = sAnswer.sOpenClientSession.hClientSession;
       session->imp._pContext       = context;
   }

   return nError;
}

//-----------------------------------------------------------------------------------------------------
TEEC_Result TEEC_InvokeCommandEx(
    TEEC_Session*         session,
    const TEEC_TimeLimit* timeLimit,
    uint32_t              commandID,
    TEEC_Operation*       operation,
    uint32_t*             returnOrigin)
{
   TEEC_Result nError;
   SCHANNEL6_ANSWER  sAnswer;
   SCHANNEL6_COMMAND sCommand;
   uint32_t    nReturnOrigin;
   TEEC_Context * context;

   context = (TEEC_Context *)session->imp._pContext;
   memset(&sCommand, 0, sizeof(SCHANNEL6_COMMAND));

   sCommand.sHeader.nMessageType = SCX_INVOKE_CLIENT_COMMAND;
   sCommand.sHeader.nMessageSize = (sizeof(SCHANNEL6_INVOKE_CLIENT_COMMAND_COMMAND) - sizeof(SCHANNEL6_COMMAND_HEADER))/sizeof(uint32_t);
   sCommand.sInvokeClientCommand.nClientCommandIdentifier = commandID;
    if (timeLimit == NULL)
   {
      sCommand.sInvokeClientCommand.sTimeout = SCTIME_INFINITE;
   }
   else
   {
      sCommand.sInvokeClientCommand.sTimeout = *(uint64_t*)timeLimit;
   }
   sCommand.sInvokeClientCommand.hClientSession     = session->imp._hClientSession;
   sCommand.sInvokeClientCommand.nCancellationID = (uint32_t)operation; // used for TEEC_RequestCancellation

   if (operation != NULL)
   {
      operation->imp._pContext = session->imp._pContext;
      operation->imp._hSession = session->imp._hClientSession;
      operation->started = 1;
   }

   nError = scxExchangeMessage(context, &sCommand, &sAnswer, operation);

   if (operation != NULL)
   {
      operation->started = 2;
      operation->imp._hSession = S_HANDLE_NULL;
      operation->imp._pContext = NULL;
   }

   if (nError != TEEC_SUCCESS)
   {
      nReturnOrigin = TEEC_ORIGIN_COMMS;
   }
   else
   {
       nError = sAnswer.sInvokeClientCommand.nErrorCode;
       nReturnOrigin = sAnswer.sInvokeClientCommand.nReturnOrigin;
    }

   if (returnOrigin != NULL) *returnOrigin = nReturnOrigin;

   return nError;

}

//-----------------------------------------------------------------------------------------------------
/*
 * Retrieves information about the implementation
 */
void TEEC_GetImplementationInfo(
                                TEEC_Context*            context,
                                TEEC_ImplementationInfo* description)
{
   TRACE_INFO("TEEC_GetImplementationInfo");

   memset(description, 0, sizeof(TEEC_ImplementationInfo));

   strcpy(description->apiDescription, S_VERSION_STRING);

   if (context != NULL)
   {
      SCX_VERSION_INFORMATION_BUFFER sInfoBuffer;
      uint32_t nResult;

      nResult = ioctl((S_HANDLE)context->imp._hConnection, IOCTL_SCX_GET_DESCRIPTION, &sInfoBuffer);
      if (nResult != S_SUCCESS)
      {
         TRACE_ERROR("TEEC_GetImplementationInfo[0x%X]: ioctl returned error: 0x%x ( %d)\n",context, nResult, errno);
         return;
      }

      memcpy(description->commsDescription, sInfoBuffer.sDriverDescription, 64);
      description->commsDescription[64] = 0;
      memcpy(description->TEEDescription, sInfoBuffer.sSecureWorldDescription, 64);
      description->TEEDescription[64] = 0;
   }
}

void TEEC_GetImplementationLimits(
   TEEC_ImplementationLimits* limits)
{
   memset(limits, 0, sizeof(TEEC_ImplementationLimits));

   /* A temp mem ref can not be mapped on more than 1Mb */
   limits->pageSize = SIZE_4KB;
   limits->tmprefMaxSize = SIZE_1MB;
   limits->sharedMemMaxSize = SIZE_1MB * 8;
 }
