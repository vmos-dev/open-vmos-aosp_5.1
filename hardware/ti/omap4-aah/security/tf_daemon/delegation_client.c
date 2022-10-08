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

#if defined(ANDROID)
#include <stddef.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * When porting to a new OS, insert here the appropriate include files
 */
#include <sys/stat.h>
#include <errno.h>
#include <sys/types.h>
#include <fcntl.h>

#if defined(LINUX) || defined(ANDROID)
#include <unistd.h>
#include <sys/resource.h>


#if defined(ANDROID)
/* fdatasync does not exist on Android */
#define fdatasync fsync
#else
/*
 * http://linux.die.net/man/2/fsync
 * The function fdatasync seems to be absent of the header file
 * in some distributions
 */
int fdatasync(int fd);
#endif /* ANDROID */
#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include <semaphore.h>
#define PATH_SEPARATOR '/'
#endif /* LINUX || ANDROID */

#ifdef WIN32
#include <windows.h>
#include <io.h>
#define PATH_SEPARATOR '\\'
#endif

#ifdef __SYMBIAN32__
#include <unistd.h>
#include "os_symbian.h"
#define PATH_SEPARATOR '\\'
#endif

#include <stdarg.h>
#include <assert.h>

#include "service_delegation_protocol.h"

#include "s_version.h"
#include "s_error.h"
#include "tee_client_api.h"

/* You can define the preprocessor constant SUPPORT_DELEGATION_EXTENSION
   if you want to pass extended options in a configuration file (option '-c').
   It is up to you to define the format of this configuration file and the
   extended option in the source file delegation_client_extension.c. You can
   use extended options, e.g., to control the name of each partition file. */
#ifdef SUPPORT_DELEGATION_EXTENSION
#include "delegation_client_extension.h"
#endif

#ifdef TFSW_FDM_ANDROID
#include <android/log.h>
#endif

/*----------------------------------------------------------------------------
 * Design notes
 * ============
 *
 * This implementation of the delegation daemon supports the protocol
 * specified in the Product Reference Manual ("Built-in Services Protocols Specification")
 *
 *----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
 * Defines and structures
 *----------------------------------------------------------------------------*/
#define ECHANGE_BUFFER_INSTRUCTIONS_NB 1000

#define DEFAULT_WORKSPACE_SIZE (128*1024)

/* A single shared memory block is used to contain the administrative data, the
   instruction buffer and the workspace. The size of the instruction buffer is
   fixed, but the size of workspace can be configured using the "-workspaceSize"
   command-line option. */
typedef struct
{
   DELEGATION_ADMINISTRATIVE_DATA sAdministrativeData;
   uint32_t                       sInstructions[ECHANGE_BUFFER_INSTRUCTIONS_NB];
   uint8_t                        sWorkspace[1/*g_nWorkspaceSize*/];
} DELEGATION_EXCHANGE_BUFFER;

#ifdef SUPPORT_RPMB_PARTITION
typedef struct
{
   uint8_t pDummy[196];
   uint8_t pMAC[32];
   uint8_t pData[256];
   uint8_t pNonce[16];
   uint32_t nMC;
   uint16_t nAddr;
   uint16_t nBlockCount;
   uint16_t nResult;
   uint16_t nReqOrResp;
} DELEGATION_RPMB_MESSAGE;
#endif

#define MD_VAR_NOT_USED(variable)  do{(void)(variable);}while(0);

#define MD_INLINE __inline

/* ----------------------------------------------
   Traces and logs

   On Linux, traces and logs go either to the console (stderr) or to the syslog.
   When the daemon is started, the logs go to the console. Once and if the daemon
   is detached, the logs go to syslog.

   On other systems, traces and logs go systematically to stderr

   The difference between traces and logs is that traces are compiled out
   in release builds whereas logs are visible to the customer.

   -----------------------------------------------*/
#if defined(LINUX) || (defined ANDROID)

static bool bDetached = false;

static MD_INLINE void LogError(const char* format, ...)
{
   va_list ap;
   va_start(ap, format);
   if (bDetached)
   {
      vsyslog(LOG_ERR, format, ap);
   }
   else
   {
#ifdef TFSW_FDM_ANDROID
	   __android_log_vprint(ANDROID_LOG_ERROR   , "TF Daemon", format, ap);
#else
      fprintf(stderr, "ERROR: ");
      vfprintf(stderr, format, ap);
      fprintf(stderr, "\n");
#endif
   }
   va_end(ap);
}

static MD_INLINE void LogWarning(const char* format, ...)
{
   va_list ap;
   va_start(ap, format);
   if (bDetached)
   {
      vsyslog(LOG_WARNING, format, ap);
   }
   else
   {
#ifdef TFSW_FDM_ANDROID
	   __android_log_vprint(ANDROID_LOG_WARN   , "TF Daemon", format, ap);
#else
      fprintf(stderr, "WARNING: ");
      vfprintf(stderr, format, ap);
      fprintf(stderr, "\n");
#endif
   }
   va_end(ap);
}
static MD_INLINE void LogInfo(const char* format, ...)
{
   va_list ap;
   va_start(ap, format);
   if (bDetached)
   {
      vsyslog(LOG_INFO, format, ap);
   }
   else
   {
#ifdef TFSW_FDM_ANDROID
	   __android_log_vprint(ANDROID_LOG_INFO   , "TF Daemon", format, ap);
#else
      vfprintf(stderr, format, ap);
      fprintf(stderr, "\n");
#endif
   }
   va_end(ap);
}

static MD_INLINE void TRACE_ERROR(const char* format, ...)
{
#ifndef NDEBUG
   va_list ap;
   va_start(ap, format);
   if (bDetached)
   {
      vsyslog(LOG_ERR, format, ap);
   }
   else
   {
#ifdef TFSW_FDM_ANDROID
	   __android_log_vprint(ANDROID_LOG_ERROR   , "TF Daemon", format, ap);
#else
      fprintf(stderr, "TRACE: ERROR: ");
      vfprintf(stderr, format, ap);
      fprintf(stderr, "\n");
#endif
   }
   va_end(ap);
#else
   MD_VAR_NOT_USED(format);
#endif /* NDEBUG */
}

static MD_INLINE void TRACE_WARNING(const char* format, ...)
{
#ifndef NDEBUG
   va_list ap;
   va_start(ap, format);
   if (bDetached)
   {
      vsyslog(LOG_WARNING, format, ap);
   }
   else
   {
#ifdef TFSW_FDM_ANDROID
	   __android_log_vprint(ANDROID_LOG_WARN   , "TF Daemon", format, ap);
#else
      fprintf(stderr, "TRACE: WARNING: ");
      vfprintf(stderr, format, ap);
      fprintf(stderr, "\n");
#endif
   }
   va_end(ap);
#else
   MD_VAR_NOT_USED(format);
#endif /* NDEBUG */
}

static MD_INLINE void TRACE_INFO(const char* format, ...)
{
#ifndef NDEBUG
   va_list ap;
   va_start(ap, format);
   if (bDetached)
   {
      vsyslog(LOG_DEBUG, format, ap);
   }
   else
   {
#ifdef TFSW_FDM_ANDROID
	   __android_log_vprint(ANDROID_LOG_INFO   , "TF Daemon", format, ap);
#else
      fprintf(stderr, "TRACE: ");
      vfprintf(stderr, format, ap);
      fprintf(stderr, "\n");
#endif
   }
   va_end(ap);
#else
   MD_VAR_NOT_USED(format);
#endif /* NDEBUG */
}
#elif defined __SYMBIAN32__
/* defined in os_symbian.h */

#elif defined NO_LOG_NO_TRACE
static MD_INLINE void LogError(const char* format, ...)
{
   MD_VAR_NOT_USED(format);
}
static MD_INLINE void LogWarning(const char* format, ...)
{
   MD_VAR_NOT_USED(format);
}
static MD_INLINE void LogInfo(const char* format, ...)
{
   MD_VAR_NOT_USED(format);
}

static MD_INLINE void TRACE_ERROR(const char* format, ...)
{
   MD_VAR_NOT_USED(format);
}

static MD_INLINE void TRACE_WARNING(const char* format, ...)
{
   MD_VAR_NOT_USED(format);
}

static MD_INLINE void TRACE_INFO(const char* format, ...)
{
   MD_VAR_NOT_USED(format);
}

#else
/* !defined(LINUX) || !defined(ANDROID) */

static MD_INLINE void LogError(const char* format, ...)
{
   va_list ap;
   va_start(ap, format);
   fprintf(stderr, "ERROR: ");
   vfprintf(stderr, format, ap);
   fprintf(stderr, "\n");
   va_end(ap);
}
static MD_INLINE void LogWarning(const char* format, ...)
{
   va_list ap;
   va_start(ap, format);
   fprintf(stderr, "WARNING: ");
   vfprintf(stderr, format, ap);
   fprintf(stderr, "\n");
   va_end(ap);
}
static MD_INLINE void LogInfo(const char* format, ...)
{
   va_list ap;
   va_start(ap, format);
   vfprintf(stderr, format, ap);
   fprintf(stderr, "\n");
   va_end(ap);
}

static MD_INLINE void TRACE_ERROR(const char* format, ...)
{
#ifndef NDEBUG
   va_list ap;
   va_start(ap, format);
   fprintf(stderr, "TRACE: ERROR: ");
   vfprintf(stderr, format, ap);
   fprintf(stderr, "\n");
   va_end(ap);
#else
   MD_VAR_NOT_USED(format);
#endif /* NDEBUG */
}

static MD_INLINE void TRACE_WARNING(const char* format, ...)
{
#ifndef NDEBUG
   va_list ap;
   va_start(ap, format);
   fprintf(stderr, "TRACE: WARNING: ");
   vfprintf(stderr, format, ap);
   fprintf(stderr, "\n");
   va_end(ap);
#else
   MD_VAR_NOT_USED(format);
#endif /* NDEBUG */
}

static MD_INLINE void TRACE_INFO(const char* format, ...)
{
#ifndef NDEBUG
   va_list ap;
   va_start(ap, format);
   fprintf(stderr, "TRACE: ");
   vfprintf(stderr, format, ap);
   fprintf(stderr, "\n");
   va_end(ap);
#else
   MD_VAR_NOT_USED(format);
#endif /* NDEBUG */
}
#endif /* defined(LINUX) || defined(ANDROID) */

/*----------------------------------------------------------------------------
 * Globals
 *----------------------------------------------------------------------------*/
/* The sector size */
static uint32_t g_nSectorSize;

/* The workspace size */
static uint32_t g_nWorkspaceSize = DEFAULT_WORKSPACE_SIZE;

/* UUID of the delegation service */
static const TEEC_UUID g_sServiceId = SERVICE_DELEGATION_UUID;

/* pWorkspaceBuffer points to the workspace buffer shared with the secure
   world to transfer the sectors in the READ and WRITE instructions  */
static uint8_t* g_pWorkspaceBuffer;
static DELEGATION_EXCHANGE_BUFFER * g_pExchangeBuffer;
TEEC_SharedMemory sExchangeSharedMem;
/*
   The absolute path name for each of the 16 possible partitions.
 */
static char* g_pPartitionNames[16];

/* The file context for each of the 16 possible partitions. An entry
   in this array is NULL if the corresponding partition is currently not opened
 */
static FILE* g_pPartitionFiles[16];

/*----------------------------------------------------------------------------
 * Utilities functions
 *----------------------------------------------------------------------------*/
static void printUsage(void)
{
   LogInfo("usage : tf_daemon [options]");
   LogInfo("where [options] are:");
   LogInfo("-h --help  Display help.");
#ifdef SUPPORT_DELEGATION_EXTENSION
   LogInfo("-c <conf>  Configuration file path.");
#else
   /* If the compilation parameter SUPPORT_DELEGATION_EXTENSION is not set, each
      partition is stored as a file within the base dir */
   LogInfo("-storageDir <baseDir>  Set the directory where the data will be stored; this directory");
   LogInfo("           must be writable and executable (this parameter is mandatory)");
#endif
   LogInfo("-d         Turns on debug mode.  If not specified, the daemon will fork itself");
   LogInfo("           and get detached from the console.");
#ifndef SUPPORT_DELEGATION_EXTENSION
   LogInfo("-workspaceSize <integer>  Set the size in bytes of the workspace. Must be greater or equal to 8 sectors.");
   LogInfo("           (default is 128KB)");
#endif
}

static TEEC_Result errno2serror(void)
{
   switch (errno)
   {
   case EINVAL:
      return S_ERROR_BAD_PARAMETERS;
   case EMFILE:
      return S_ERROR_NO_MORE_HANDLES;
   case ENOENT:
      return S_ERROR_ITEM_NOT_FOUND;
   case EEXIST:
      return S_ERROR_ITEM_EXISTS;
   case ENOSPC:
      return S_ERROR_STORAGE_NO_SPACE;
   case ENOMEM:
      return S_ERROR_OUT_OF_MEMORY;
   case EBADF:
   case EACCES:
   default:
      return S_ERROR_STORAGE_UNREACHABLE;
   }
}

/*
 * Check if the directory in parameter exists with Read/Write access
 * Return 0 in case of success and 1 otherwise.
 */
int static_checkStorageDirAndAccessRights(char * directoryName)
{
#ifdef __SYMBIAN32__
   /* it looks like stat is not working properly on Symbian
      Create and remove dummy file to check access rights */
   FILE *stream;
   char *checkAccess = NULL;

   if (directoryName == NULL)
   {
      LogError("Directory Name is NULL");
      return 1;
   }

   checkAccess = malloc(strlen(directoryName)+1/* \ */ +1 /* a */ + 1 /* 0 */);
   if (!checkAccess)
   {
      LogError("storageDir '%s' allocation error", directoryName);
      return 1;
   }
   sprintf(checkAccess,"%s\\a",directoryName);
   stream = fopen(checkAccess, "w+b");
   if (!stream)
   {
      LogError("storageDir '%s' is incorrect or cannot be reached", directoryName);
      return 1;
   }
   fclose(stream);
   unlink(checkAccess);
#else
   /* Non-Symbian OS: use stat */
   struct stat buf;
   int result = 0;

   if (directoryName == NULL)
   {
      LogError("Directory Name is NULL");
      return 1;
   }

   result = stat(directoryName, &buf);
   if (result == 0)
   {
      /* Storage dir exists. Check access rights */
#if defined(LINUX) || (defined ANDROID)
      if ((buf.st_mode & (S_IXUSR | S_IWUSR)) != (S_IXUSR | S_IWUSR))
      {
         LogError("storageDir '%s' does not have read-write access", directoryName);
         return 1;
      }
#endif
   }
   else if (errno == ENOENT)
   {
      LogError("storageDir '%s' does not exist", directoryName);
      return 1;
   }
   else
   {
      /* Another error */
      LogError("storageDir '%s' is incorrect or cannot be reached", directoryName);
      return 1;
   }
#endif
   return 0;
}



/*----------------------------------------------------------------------------
 * Instructions
 *----------------------------------------------------------------------------*/

/**
 * This function executes the DESTROY_PARTITION instruction
 *
 * @param nPartitionID: the partition identifier
 **/
static TEEC_Result partitionDestroy(uint32_t nPartitionID)
{
   TEEC_Result  nError = S_SUCCESS;

   if (g_pPartitionFiles[nPartitionID] != NULL)
   {
      /* The partition must not be currently opened */
      LogError("g_pPartitionFiles not NULL");
      return S_ERROR_BAD_STATE;
   }

   /* Try to erase the file */
#if defined(LINUX) || (defined ANDROID) || defined (__SYMBIAN32__)
   if (unlink(g_pPartitionNames[nPartitionID]) != 0)
#endif
#ifdef WIN32
   if (_unlink(g_pPartitionNames[nPartitionID]) != 0)
#endif
   {
      /* File in use or OS didn't allow the operation */
      nError = errno2serror();
   }

   return nError;
}

/**
 * This function executes the CREATE_PARTITION instruction. When successful,
 * it fills the g_pPartitionFiles[nPartitionID] slot.
 *
 * @param nPartitionID: the partition identifier
 **/
static TEEC_Result partitionCreate(uint32_t nPartitionID)
{
   uint32_t nError = S_SUCCESS;

   if (g_pPartitionFiles[nPartitionID] != NULL)
   {
      /* The partition is already opened */
      LogError("g_pPartitionFiles not NULL");
      return S_ERROR_BAD_STATE;
   }

   /* Create the file unconditionnally */
   LogInfo("Create storage file \"%s\"", g_pPartitionNames[nPartitionID]);
   g_pPartitionFiles[nPartitionID] = fopen(g_pPartitionNames[nPartitionID], "w+b");

   if (g_pPartitionFiles[nPartitionID] == NULL)
   {
      LogError("Cannot create storage file \"%s\"", g_pPartitionNames[nPartitionID]);
      nError = errno2serror();
      return nError;
   }

   return nError;
}

/**
 * This function executes the OPEN_PARTITION instruction. When successful,
 * it fills the g_pPartitionFiles[nPartitionID] slot and writes the partition
 * size in hResultEncoder
 *
 * @param nPartitionID: the partition identifier
 * @param pnPartitionSize: filled with the number of sectors in the partition
 **/
static TEEC_Result partitionOpen(uint32_t nPartitionID, uint32_t* pnPartitionSize)
{
   uint32_t nError = S_SUCCESS;

   if (g_pPartitionFiles[nPartitionID] != NULL)
   {
      /* No partition must be currently opened in the session */
      LogError("g_pPartitionFiles not NULL");
      return S_ERROR_BAD_STATE;
   }

   /* Open the file */
   g_pPartitionFiles[nPartitionID] = fopen(g_pPartitionNames[nPartitionID], "r+b");
   if (g_pPartitionFiles[nPartitionID] == NULL)
   {
      if (errno == ENOENT)
      {
         /* File does not exist */
         LogError("Storage file \"%s\" does not exist", g_pPartitionNames[nPartitionID]);
         nError = S_ERROR_ITEM_NOT_FOUND;
         return nError;
      }
      else
      {
         LogError("cannot open storage file \"%s\"", g_pPartitionNames[nPartitionID]);
         nError = errno2serror();
         return nError;
      }
   }
   /* Determine the current number of sectors */
   fseek(g_pPartitionFiles[nPartitionID], 0L, SEEK_END);
   *pnPartitionSize = ftell(g_pPartitionFiles[nPartitionID]) / g_nSectorSize;

   LogInfo("storage file \"%s\" successfully opened (size = %d KB (%d bytes))",
      g_pPartitionNames[nPartitionID],
      ((*pnPartitionSize) * g_nSectorSize) / 1024,
      ((*pnPartitionSize) * g_nSectorSize));

   return nError;
}


/**
 * This function executes the CLOSE_PARTITION instruction.
 * It closes the partition file.
 *
 * @param nPartitionID: the partition identifier
 **/
static TEEC_Result partitionClose(uint32_t nPartitionID)
{
   if (g_pPartitionFiles[nPartitionID] == NULL)
   {
      /* The partition is currently not opened */
      return S_ERROR_BAD_STATE;
   }
   fclose(g_pPartitionFiles[nPartitionID]);
   g_pPartitionFiles[nPartitionID] = NULL;
   return S_SUCCESS;
}

/**
 * This function executes the READ instruction.
 *
 * @param nPartitionID: the partition identifier
 * @param nSectorIndex: the index of the sector to read
 * @param nWorkspaceOffset: the offset in the workspace where the sector must be written
 **/
static TEEC_Result partitionRead(uint32_t nPartitionID, uint32_t nSectorIndex, uint32_t nWorkspaceOffset)
{
   FILE* pFile;

   TRACE_INFO(">Partition %1X: read sector 0x%08X into workspace at offset 0x%08X",
      nPartitionID, nSectorIndex, nWorkspaceOffset);

   pFile = g_pPartitionFiles[nPartitionID];

   if (pFile == NULL)
   {
      /* The partition is not opened */
      return S_ERROR_BAD_STATE;
   }

   if (fseek(pFile, nSectorIndex*g_nSectorSize, SEEK_SET) != 0)
   {
      LogError("fseek error: %s", strerror(errno));
      return errno2serror();
   }

   if (fread(g_pWorkspaceBuffer + nWorkspaceOffset,
             g_nSectorSize, 1,
             pFile) != 1)
   {
      if (feof(pFile))
      {
         LogError("fread error: End-Of-File detected");
         return S_ERROR_ITEM_NOT_FOUND;
      }
      LogError("fread error: %s", strerror(errno));
      return errno2serror();
   }

   return S_SUCCESS;
}

#ifdef SUPPORT_RPMB_PARTITION
static TEEC_Result rpmbRead(DELEGATION_RPMB_INSTRUCTION *pInstruction)
{
   DELEGATION_RPMB_MESSAGE* pMessages;
   uint32_t nNbMsg, nIndex;

   nNbMsg = g_nSectorSize >> 8;
   pMessages = (DELEGATION_RPMB_MESSAGE*)malloc(nNbMsg * sizeof(DELEGATION_RPMB_MESSAGE));
   if (pMessages == NULL)
   {
      return S_ERROR_OUT_OF_MEMORY;
   }
   memset(pMessages,0,nNbMsg * sizeof(DELEGATION_RPMB_MESSAGE));

   for (nIndex=0;nIndex<nNbMsg;nIndex++)
   {
      memcpy(pMessages[nIndex].pNonce , pInstruction->pNonce, 16);
      pMessages[nIndex].nAddr = pInstruction->nAddr;
      pMessages[nIndex].nBlockCount = pInstruction->nBlockCount;
      pMessages[nIndex].nReqOrResp = 0x0004;
   }
   memcpy(pMessages[nNbMsg-1].pMAC,pInstruction->nMAC,32);

   /* TODO: send to the RPMB driver */

   memcpy(pInstruction->pNonce,pMessages[0].pNonce , 16);
   pInstruction->nAddr = pMessages[0].nAddr;
   pInstruction->nBlockCount = pMessages[0].nBlockCount;
   for (nIndex=0;nIndex<nNbMsg;nIndex++)
   {
      memcpy(g_pWorkspaceBuffer + pInstruction->nWorkspaceOffset[nIndex],pMessages[nIndex].pData,256);
   }
   memcpy(pInstruction->nMAC, pMessages[nNbMsg-1].pMAC,32);
   pInstruction->nResult=pMessages[nNbMsg-1].nResult;

   free(pMessages);

   return S_SUCCESS;
}
#endif
/**
 * This function executes the WRITE instruction.
 *
 * @param nPartitionID: the partition identifier
 * @param nSectorIndex: the index of the sector to read
 * @param nWorkspaceOffset: the offset in the workspace where the sector must be read
 **/
static TEEC_Result partitionWrite(uint32_t nPartitionID, uint32_t nSectorIndex, uint32_t nWorkspaceOffset)
{
   FILE* pFile;

   TRACE_INFO(">Partition %1X: write sector 0x%X from workspace at offset 0x%X",
      nPartitionID, nSectorIndex, nWorkspaceOffset);

   pFile = g_pPartitionFiles[nPartitionID];

   if (pFile == NULL)
   {
      /* The partition is not opened */
      return S_ERROR_BAD_STATE;
   }

   if (fseek(pFile, nSectorIndex*g_nSectorSize, SEEK_SET) != 0)
   {
      LogError("fseek error: %s", strerror(errno));
      return errno2serror();
   }

   if (fwrite(g_pWorkspaceBuffer + nWorkspaceOffset,
              g_nSectorSize, 1,
              pFile) != 1)
   {
      LogError("fread error: %s", strerror(errno));
      return errno2serror();
   }
   return S_SUCCESS;
}

#ifdef SUPPORT_RPMB_PARTITION
static TEEC_Result rpmbWrite(DELEGATION_RPMB_INSTRUCTION *pInstruction)
{
   DELEGATION_RPMB_MESSAGE* pMessages;
   uint32_t nNbMsg, nIndex;

   nNbMsg = g_nSectorSize >> 8;
   pMessages = (DELEGATION_RPMB_MESSAGE*)malloc(nNbMsg * sizeof(DELEGATION_RPMB_MESSAGE));
   if (pMessages == NULL)
   {
      return S_ERROR_OUT_OF_MEMORY;
   }
   memset(pMessages,0,nNbMsg * sizeof(DELEGATION_RPMB_MESSAGE));

   for (nIndex=0;nIndex<nNbMsg;nIndex++)
   {
      memcpy(pMessages[nIndex].pData,g_pWorkspaceBuffer + pInstruction->nWorkspaceOffset[nIndex],256);
      pMessages[nIndex].nMC = pInstruction->nMC;
      pMessages[nIndex].nAddr = pInstruction->nAddr;
      pMessages[nIndex].nBlockCount = pInstruction->nBlockCount;
      pMessages[nIndex].nReqOrResp = 0x0003;
   }
   memcpy(pMessages[nNbMsg-1].pMAC,pInstruction->nMAC,32);

   /* TODO: send to the RPMB driver */

   pInstruction->nAddr = pMessages[0].nAddr;
   pInstruction->nMC = pMessages[0].nMC;
   memcpy(pInstruction->nMAC, pMessages[nNbMsg-1].pMAC,32);
   pInstruction->nResult=pMessages[nNbMsg-1].nResult;

   free(pMessages);

   return S_SUCCESS;
}
#endif
/**
 * This function executes the SET_SIZE instruction.
 *
 * @param nPartitionID: the partition identifier
 * @param nNewSectorCount: the new sector count
 **/
static TEEC_Result partitionSetSize(uint32_t nPartitionID, uint32_t nNewSectorCount)
{
   FILE* pFile;
   uint32_t nCurrentSectorCount;

   pFile = g_pPartitionFiles[nPartitionID];

   if (pFile==NULL)
   {
      /* The partition is not opened */
      return S_ERROR_BAD_STATE;
   }

   /* Determine the current size of the partition */
   if (fseek(pFile, 0, SEEK_END) != 0)
   {
      LogError("fseek error: %s", strerror(errno));
      return errno2serror();
   }
   nCurrentSectorCount = ftell(pFile) / g_nSectorSize;

   if (nNewSectorCount > nCurrentSectorCount)
   {
      uint32_t nAddedBytesCount;
      /* Enlarge the partition file. Make sure we actually write
         some non-zero data into the new sectors. Otherwise, some file-system
         might not really reserve the storage space but use a
         sparse representation. In this case, a subsequent write instruction
         could fail due to out-of-space, which we want to avoid. */
      nAddedBytesCount = (nNewSectorCount-nCurrentSectorCount)*g_nSectorSize;
      while (nAddedBytesCount)
      {
         if (fputc(0xA5, pFile)!=0xA5)
         {
            return errno2serror();
         }
         nAddedBytesCount--;
      }
   }
   else if (nNewSectorCount < nCurrentSectorCount)
   {
      int result = 0;
      /* Truncate the partition file */
#if defined(LINUX) || (defined ANDROID)
      result = ftruncate(fileno(pFile),nNewSectorCount * g_nSectorSize);
#endif
#if defined (__SYMBIAN32__)
	  LogError("No truncate available in Symbian C API");
#endif
#ifdef WIN32
      result = _chsize(_fileno(pFile),nNewSectorCount * g_nSectorSize);
#endif
      if (result)
      {
         return errno2serror();
      }
   }
   return S_SUCCESS;
}

/**
 * This function executes the SYNC instruction.
 *
 * @param pPartitionID: the partition identifier
 **/
static TEEC_Result partitionSync(uint32_t nPartitionID)
{
   TEEC_Result nError = S_SUCCESS;
   int result;

   FILE* pFile = g_pPartitionFiles[nPartitionID];

   if (pFile == NULL)
   {
      /* The partition is not currently opened */
      return S_ERROR_BAD_STATE;
   }

   /* First make sure that the data in the stdio buffers
      is flushed to the file descriptor */
   result=fflush(pFile);
   if (result)
   {
      nError=errno2serror();
      goto end;
   }
   /* Then synchronize the file descriptor with the file-system */

#if defined(LINUX) || (defined ANDROID)
   result=fdatasync(fileno(pFile));
#endif
#if defined (__SYMBIAN32__)
   result=fsync(fileno(pFile));
#endif
#ifdef WIN32
   result=_commit(_fileno(pFile));
#endif
   if (result)
   {
      nError=errno2serror();
   }

end:
   return nError;
}

/**
 * This function executes the NOTIFY instruction.
 *
 * @param pMessage the message string
 * @param nMessageType the type of messages
 **/
static void notify(const wchar_t* pMessage, uint32_t nMessageType)
{
   switch (nMessageType)
   {
   case DELEGATION_NOTIFY_TYPE_ERROR:
      LogError("%ls", pMessage);
      break;
   case DELEGATION_NOTIFY_TYPE_WARNING:
      LogWarning("%ls", pMessage);
      break;
   case DELEGATION_NOTIFY_TYPE_DEBUG:
      LogInfo("DEBUG: %ls", pMessage);
      break;
   case DELEGATION_NOTIFY_TYPE_INFO:
   default:
      LogInfo("%ls", pMessage);
      break;
   }
}

/*----------------------------------------------------------------------------
 * Session main function
 *----------------------------------------------------------------------------*/

/*
 * This function runs a session opened on the delegation service. It fetches
 * instructions and execute them in a loop. It never returns, but may call
 * exit when instructed to shutdown by the service
 */
static int runSession(TEEC_Context* pContext, TEEC_Session* pSession, TEEC_Operation* pOperation)
{
   memset(&g_pExchangeBuffer->sAdministrativeData, 0x00, sizeof(g_pExchangeBuffer->sAdministrativeData));

   while (true)
   {
      S_RESULT    nError;
      TEEC_Result                      nTeeError;
      uint32_t                         nInstructionsIndex;
      uint32_t                         nInstructionsBufferSize = sizeof(g_pExchangeBuffer->sInstructions);

      pOperation->paramTypes = TEEC_PARAM_TYPES(
         TEEC_MEMREF_PARTIAL_INPUT,
         TEEC_MEMREF_PARTIAL_OUTPUT,
         TEEC_MEMREF_PARTIAL_INOUT,
         TEEC_NONE);
      pOperation->params[0].memref.parent = &sExchangeSharedMem;
      pOperation->params[0].memref.offset = offsetof(DELEGATION_EXCHANGE_BUFFER, sAdministrativeData);
      pOperation->params[0].memref.size   = sizeof(g_pExchangeBuffer->sAdministrativeData);

      pOperation->params[1].memref.parent = &sExchangeSharedMem;
      pOperation->params[1].memref.offset = offsetof(DELEGATION_EXCHANGE_BUFFER, sInstructions);
      pOperation->params[1].memref.size   = sizeof(g_pExchangeBuffer->sInstructions);

      pOperation->params[2].memref.parent = &sExchangeSharedMem;
      pOperation->params[2].memref.offset = offsetof(DELEGATION_EXCHANGE_BUFFER, sWorkspace);
      pOperation->params[2].memref.size   = g_nWorkspaceSize;

      nTeeError = TEEC_InvokeCommand(pSession,
                                  SERVICE_DELEGATION_GET_INSTRUCTIONS,   /* commandID */
                                  pOperation,     /* IN OUT operation */
                                  NULL             /* OUT errorOrigin, optional */
                                 );

      if (nTeeError != TEEC_SUCCESS)
      {
         LogError("TEEC_InvokeCommand error: 0x%08X", nTeeError);
         LogError("Daemon exits");
         exit(2);
      }

      if (pOperation->params[1].tmpref.size >  nInstructionsBufferSize)
      {
         /* Should not happen, probably an error from the service */
         pOperation->params[1].tmpref.size = 0;
      }

      /* Reset the operation results */
      nError = TEEC_SUCCESS;
      g_pExchangeBuffer->sAdministrativeData.nSyncExecuted = 0;
      memset(g_pExchangeBuffer->sAdministrativeData.nPartitionErrorStates, 0x00, sizeof(g_pExchangeBuffer->sAdministrativeData.nPartitionErrorStates));
      memset(g_pExchangeBuffer->sAdministrativeData.nPartitionOpenSizes, 0x00, sizeof(g_pExchangeBuffer->sAdministrativeData.nPartitionOpenSizes));

      /* Execute the instructions */
      nInstructionsIndex = 0;
      nInstructionsBufferSize = pOperation->params[1].tmpref.size;
      while (true)
      {
         DELEGATION_INSTRUCTION * pInstruction;
         uint32_t nInstructionID;
         pInstruction = (DELEGATION_INSTRUCTION *)(&g_pExchangeBuffer->sInstructions[nInstructionsIndex/4]);
         if (nInstructionsIndex + 4 <= nInstructionsBufferSize)
         {
            nInstructionID = pInstruction->sGeneric.nInstructionID;
            nInstructionsIndex+=4;
         }
         else
         {
            goto instruction_parse_end;
         }
         if ((nInstructionID & 0x0F) == 0)
         {
            /* Partition-independent instruction */
            switch (nInstructionID)
            {
            case DELEGATION_INSTRUCTION_SHUTDOWN:
               {
                  exit(0);
                  /* The implementation of the TF Client API will automatically
                     destroy the context and release any associated resource */
               }
            case DELEGATION_INSTRUCTION_NOTIFY:
               {
                  /* Parse the instruction parameters */
                  wchar_t  pMessage[100];
                  uint32_t nMessageType;
                  uint32_t nMessageSize;
                  memset(pMessage, 0, 100*sizeof(wchar_t));

                  if (nInstructionsIndex + 8 <= nInstructionsBufferSize)
                  {
                     nMessageType = pInstruction->sNotify.nMessageType;
                     nMessageSize = pInstruction->sNotify.nMessageSize;
                     nInstructionsIndex+=8;
                  }
                  else
                  {
                     goto instruction_parse_end;
                  }
                  if (nMessageSize > (99)*sizeof(wchar_t))
                  {
                     /* How to handle the error correctly in this case ? */
                     goto instruction_parse_end;
                  }
                  if (nInstructionsIndex + nMessageSize <= nInstructionsBufferSize)
                  {
                     memcpy(pMessage, &pInstruction->sNotify.nMessage[0], nMessageSize);
                     nInstructionsIndex+=nMessageSize;
                  }
                  else
                  {
                     goto instruction_parse_end;
                  }
                  /* Align the pInstructionsIndex on 4 bytes */
                  nInstructionsIndex = (nInstructionsIndex+3)&~3;
                  notify(pMessage, nMessageType);
                  break;
               }
            default:
               LogError("Unknown instruction identifier: %02X", nInstructionID);
               nError = S_ERROR_BAD_PARAMETERS;
               break;
            }
         }
         else
         {
            /* Partition-specific instruction */
            uint32_t nPartitionID = (nInstructionID & 0xF0) >> 4;
            if (g_pExchangeBuffer->sAdministrativeData.nPartitionErrorStates[nPartitionID] == S_SUCCESS)
            {
               /* Execute the instruction only if there is currently no
                  error on the partition */
               switch (nInstructionID & 0x0F)
               {
               case DELEGATION_INSTRUCTION_PARTITION_CREATE:
                  nError = partitionCreate(nPartitionID);
#ifdef SUPPORT_RPMB_PARTITION
                  if (nPartitionID == RPMB_PARTITION_ID)
                  {
                     /* TODO: get the Write counter */
                     pInstruction->sAuthRW.nMC = 0;
                  }
#endif
                  TRACE_INFO("INSTRUCTION: ID=0x%x pid=%d err=%d", (nInstructionID & 0x0F), nPartitionID, nError);
                  break;
               case DELEGATION_INSTRUCTION_PARTITION_OPEN:
                  {
                     uint32_t nPartitionSize = 0;
                     nError = partitionOpen(nPartitionID, &nPartitionSize);
                     TRACE_INFO("INSTRUCTION: ID=0x%x pid=%d pSize=%d err=%d", (nInstructionID & 0x0F), nPartitionID, nPartitionSize, nError);
                     if (nError == S_SUCCESS)
                     {
                        g_pExchangeBuffer->sAdministrativeData.nPartitionOpenSizes[nPartitionID] = nPartitionSize;
                     }
#ifdef SUPPORT_RPMB_PARTITION
                     if (nPartitionID == RPMB_PARTITION_ID)
                     {
                        /* TODO: get the Write counter */
                        pInstruction->sAuthRW.nMC = 0;
                     }
#endif
                     break;
                  }
               case DELEGATION_INSTRUCTION_PARTITION_READ:
#ifdef SUPPORT_RPMB_PARTITION
                  if (nPartitionID == RPMB_PARTITION_ID)
                  {
                     if (nInstructionsIndex + sizeof(DELEGATION_RPMB_INSTRUCTION)-sizeof(uint32_t) <= nInstructionsBufferSize)
                     {
                        nInstructionsIndex+=sizeof(DELEGATION_RPMB_INSTRUCTION)-sizeof(uint32_t);
                     }
                     else
                     {
                        goto instruction_parse_end;
                     }
                     nError = rpmbRead(&pInstruction->sAuthRW);
                     TRACE_INFO("INSTRUCTION: ID=0x%x pid=%d err=%d", (nInstructionID & 0x0F), nPartitionID, nError);
                     break;
                  }
                  else
#endif
                  {
                     /* Parse parameters */
                     uint32_t nSectorID;
                     uint32_t nWorkspaceOffset;
                     if (nInstructionsIndex + 8 <= nInstructionsBufferSize)
                     {
                        nSectorID        = pInstruction->sReadWrite.nSectorID;
                        nWorkspaceOffset = pInstruction->sReadWrite.nWorkspaceOffset;
                        nInstructionsIndex+=8;
                     }
                     else
                     {
                        goto instruction_parse_end;
                     }
                     nError = partitionRead(nPartitionID, nSectorID, nWorkspaceOffset);
                     TRACE_INFO("INSTRUCTION: ID=0x%x pid=%d sid=%d woff=%d err=%d", (nInstructionID & 0x0F), nPartitionID, nSectorID, nWorkspaceOffset, nError);
                     break;
                  }
               case DELEGATION_INSTRUCTION_PARTITION_WRITE:
#ifdef SUPPORT_RPMB_PARTITION
                  if (nPartitionID == RPMB_PARTITION_ID)
                  {
                     if (nInstructionsIndex + sizeof(DELEGATION_RPMB_INSTRUCTION)-sizeof(uint32_t) <= nInstructionsBufferSize)
                     {
                        nInstructionsIndex+=sizeof(DELEGATION_RPMB_INSTRUCTION)-sizeof(uint32_t);
                     }
                     else
                     {
                        goto instruction_parse_end;
                     }
                     nError = rpmbWrite(&pInstruction->sAuthRW);
                     TRACE_INFO("INSTRUCTION: ID=0x%x pid=%d err=%d", (nInstructionID & 0x0F), nPartitionID, nError);
                     break;
                  }
                  else
#endif
                  {
                     /* Parse parameters */
                     uint32_t nSectorID;
                     uint32_t nWorkspaceOffset;
                     if (nInstructionsIndex + 8 <= nInstructionsBufferSize)
                     {
                        nSectorID        = pInstruction->sReadWrite.nSectorID;
                        nWorkspaceOffset = pInstruction->sReadWrite.nWorkspaceOffset;
                        nInstructionsIndex+=8;
                     }
                     else
                     {
                        goto instruction_parse_end;
                     }
                     nError = partitionWrite(nPartitionID, nSectorID, nWorkspaceOffset);
                     TRACE_INFO("INSTRUCTION: ID=0x%x pid=%d sid=%d woff=%d err=%d", (nInstructionID & 0x0F), nPartitionID, nSectorID, nWorkspaceOffset, nError);
                     break;
                  }
               case DELEGATION_INSTRUCTION_PARTITION_SYNC:
                  nError = partitionSync(nPartitionID);
                  TRACE_INFO("INSTRUCTION: ID=0x%x pid=%d err=%d", (nInstructionID & 0x0F), nPartitionID, nError);
                  if (nError == S_SUCCESS)
                  {
                     g_pExchangeBuffer->sAdministrativeData.nSyncExecuted++;
                  }
                  break;
               case DELEGATION_INSTRUCTION_PARTITION_SET_SIZE:
                  {
                     uint32_t nNewSize;
                     if (nInstructionsIndex + 4 <= nInstructionsBufferSize)
                     {
                        nNewSize = pInstruction->sSetSize.nNewSize;
                        nInstructionsIndex+=4;
                     }
                     else
                     {
                        goto instruction_parse_end;
                     }
                     nError = partitionSetSize(nPartitionID, nNewSize);
                     TRACE_INFO("INSTRUCTION: ID=0x%x pid=%d nNewSize=%d err=%d", (nInstructionID & 0x0F), nPartitionID, nNewSize, nError);
                     break;
                  }
               case DELEGATION_INSTRUCTION_PARTITION_CLOSE:
                  nError = partitionClose(nPartitionID);
                  TRACE_INFO("INSTRUCTION: ID=0x%x pid=%d err=%d", (nInstructionID & 0x0F), nPartitionID, nError);
                  break;
               case DELEGATION_INSTRUCTION_PARTITION_DESTROY:
                  nError = partitionDestroy(nPartitionID);
                  TRACE_INFO("INSTRUCTION: ID=0x%x pid=%d err=%d", (nInstructionID & 0x0F), nPartitionID, nError);
                  break;
               }
               g_pExchangeBuffer->sAdministrativeData.nPartitionErrorStates[nPartitionID] = nError;
            }
         }
      }
instruction_parse_end:
      memset(pOperation, 0, sizeof(TEEC_Operation));
   }
}

/**
 * This function opens a new session to the delegation service.
 **/
static int createSession(TEEC_Context* pContext, TEEC_Session* pSession, TEEC_Operation* pOperation)
{
   TEEC_Result nError;
   uint32_t nExchangeBufferSize;

   memset(pOperation, 0, sizeof(TEEC_Operation));
   pOperation->paramTypes = TEEC_PARAM_TYPES(
      TEEC_VALUE_OUTPUT, TEEC_NONE, TEEC_NONE, TEEC_NONE);
   nError = TEEC_OpenSession(pContext,
                             pSession,                /* OUT session */
                             &g_sServiceId,           /* destination UUID */
                             TEEC_LOGIN_PRIVILEGED,   /* connectionMethod */
                             NULL,                    /* connectionData */
                             pOperation,              /* IN OUT operation */
                             NULL                     /* OUT errorOrigin, optional */
                             );
   if (nError != TEEC_SUCCESS)
   {
      LogError("Error on TEEC_OpenSession : 0x%x", nError);
      exit(2);
   }
   /* Read sector size */
   g_nSectorSize = pOperation->params[0].value.a;
   LogInfo("Sector Size: %d bytes", g_nSectorSize);

   /* Check sector size */
   if (!(g_nSectorSize == 512 || g_nSectorSize == 1024 || g_nSectorSize == 2048 || g_nSectorSize == 4096))
   {
      LogError("Incorrect sector size: terminating...");
      exit(2);
   }

   /* Check workspace size */
   if (g_nWorkspaceSize < 8 * g_nSectorSize)
   {
      g_nWorkspaceSize = 8 * g_nSectorSize;
      LogWarning("Workspace size too small, automatically set to %d bytes", g_nWorkspaceSize);
   }
   /* Compute the size of the exchange buffer */
   nExchangeBufferSize = sizeof(DELEGATION_EXCHANGE_BUFFER)-1+g_nWorkspaceSize;
   g_pExchangeBuffer  = (DELEGATION_EXCHANGE_BUFFER*)malloc(nExchangeBufferSize);
	 if (g_pExchangeBuffer == NULL)
   {
      LogError("Cannot allocate exchange buffer of %d bytes", nExchangeBufferSize);
      LogError("Now exiting...");
      exit(2);
   }
   g_pWorkspaceBuffer = (uint8_t*)g_pExchangeBuffer->sWorkspace;
   memset(g_pExchangeBuffer, 0x00, nExchangeBufferSize);
   memset(g_pPartitionFiles,0,16*sizeof(FILE*));

   /* Register the exchange buffer as a shared memory block */
   sExchangeSharedMem.buffer = g_pExchangeBuffer;
   sExchangeSharedMem.size   = nExchangeBufferSize;
   sExchangeSharedMem.flags  = TEEC_MEM_INPUT | TEEC_MEM_OUTPUT;
   nError = TEEC_RegisterSharedMemory(pContext, &sExchangeSharedMem);
   if (nError != TEEC_SUCCESS)
   {
      LogError("Error on TEEC_RegisterSharedMemory : 0x%x", nError);
      free(g_pExchangeBuffer);
      exit(2);
   }
   LogInfo("Daemon now connected");
   return 0;
}


/*----------------------------------------------------------------------------
 * Main
 *----------------------------------------------------------------------------*/

#ifdef INCLUDE_CLIENT_DELEGATION
int delegation_main(int argc, char* argv[])
#else
int main(int argc, char* argv[])
#endif
{
   TEEC_Result    nError;
   TEEC_Context   sContext;
   TEEC_Session   sSession;
   TEEC_Operation sOperation;
   bool        debug = false;

#ifndef SUPPORT_DELEGATION_EXTENSION
   char * baseDir = NULL;

   LogInfo("TFSW Normal-World Daemon");
#else
   LogInfo("TFSW Normal-World Ext Daemon");
#endif
   LogInfo(S_VERSION_STRING);
   LogInfo("");

   /* Skip program name */
   argv++;
   argc--;

   while (argc != 0)
   {
      if (strcmp(argv[0], "-d") == 0)
      {
         debug = true;
      }
#ifdef SUPPORT_DELEGATION_EXTENSION
      else if (strcmp(argv[0], "-c") == 0)
      {
         int error;
         argc--;
         argv++;
         if (argc == 0)
         {
            printUsage();
            return 1;
         }
         /* Note that the function parseCommandLineExtension can modify the
            content of the g_partitionNames array */
         error = parseCommandLineExtension(argv[0], g_pPartitionNames);
         if ( error != 0 )
         {
            printUsage();
            return error;
         }
      }
#else
      else if (strcmp(argv[0], "-storageDir") == 0)
      {
         uint32_t i = 0;
         argc--;
         argv++;
         if (argc == 0)
         {
            printUsage();
            return 1;
         }
         if (baseDir != NULL)
         {
            LogError("Only one storage directory may be specified");
            return 1;
         }
         baseDir = malloc(strlen(argv[0])+1); /* Zero-terminated string */
         if (baseDir == NULL)
         {
             LogError("Out of memory");
             return 2;
         }

         strcpy(baseDir, argv[0]);

         /* Set default names to the partitions */
         for ( i=0; i<16 ;i++ )
         {
            g_pPartitionNames[i] = NULL;
            g_pPartitionNames[i] = malloc(strlen(baseDir) + 1 /* separator */ + sizeof("Store_X.tf"));
            if (g_pPartitionNames[i] != NULL)
            {
               sprintf(g_pPartitionNames[i], "%s%cStore_%1X.tf", baseDir, PATH_SEPARATOR, i);
            }
            else
            {
               free(baseDir);
               i=0;
               while(g_pPartitionNames[i] != NULL) free(g_pPartitionNames[i++]);
               LogError("Out of memory");
               return 2;
            }
         }
      }
      else if (strcmp(argv[0], "-workspaceSize") == 0)
      {
         argc--;
         argv++;
         if (argc == 0)
         {
            printUsage();
            return 1;
         }
         g_nWorkspaceSize=atol(argv[0]);
      }
#endif /* ! SUPPORT_DELEGATION_EXTENSION */
      /*****************************************/
      else if (strcmp(argv[0], "--help") == 0 || strcmp(argv[0], "-h") == 0)
      {
         printUsage();
         return 0;
      }
      else
      {
         printUsage();
         return 1;
      }
      argc--;
      argv++;
   }

#ifndef SUPPORT_DELEGATION_EXTENSION
   if (baseDir == NULL)
   {
      LogError("-storageDir option is mandatory");
      return 1;
   }
   else
   {
      if (static_checkStorageDirAndAccessRights(baseDir) != 0)
      {
         return 1;
      }
   }
#endif /* #ifndef SUPPORT_DELEGATION_EXTENSION */

   /*
    * Detach the daemon from the console
    */

#if defined(LINUX) || (defined ANDROID)
   {
      /*
       * Turns this application into a daemon => fork off parent process, setup logging, ...
       */

      /* Our process ID and Session ID */
      pid_t pid, sid;

      if (!debug)
      {
         LogInfo("tf_daemon is detaching from console... Further traces go to syslog");
         /* Fork off the parent process */
         pid = fork();
         if (pid < 0)
         {
            LogError("daemon forking failed");
            return 1;
         }

         if (pid > 0)
         {
            /* parent */
            return 0;
         }
         bDetached = true;
      }

      /* Change the file mode mask */
      umask(0077);

      if (!debug)
      {
         /* Open any logs here */
         openlog("tf_daemon", 0, LOG_DAEMON);

         /* Detach from the console */
         sid = setsid();
         if (sid < 0)
         {
            /* Log the failure */
            LogError("daemon group creation failed");
            return 1;
         }
         /* Close out the standard file descriptors */
         close(STDIN_FILENO);
         close(STDOUT_FILENO);
         close(STDERR_FILENO);
      }
   }
   /* Change priority so that tf_driver.ko with no polling thread is faster */
   if (setpriority(PRIO_PROCESS, 0, 19)!=0)
   {
      LogError("Daemon cannot change priority");
      return 1;
   }

#endif

   TRACE_INFO("Sector size is %d", g_nSectorSize);

   LogInfo("tf_daemon - started");

   nError = TEEC_InitializeContext(NULL,  /* const char * name */
                                   &sContext);   /* TEEC_Context* context */
   if (nError != TEEC_SUCCESS)
   {
      LogError("TEEC_InitializeContext error: 0x%08X", nError);
      LogError("Now exiting...");
      exit(2);
   }

   /* Open a session */
   if(createSession(&sContext, &sSession, &sOperation) == 0)
   {
      /* Run the session. This should never return */
      runSession(&sContext, &sSession, &sOperation);
   }
   TEEC_FinalizeContext(&sContext);

   return 3;
}
