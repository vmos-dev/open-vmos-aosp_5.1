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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifndef WIN32
#include <sys/ioctl.h>
#include <unistd.h>
#endif
#include <fcntl.h>
#include <errno.h>

#include "smc_pa_ctrl_os.h"
#include "s_type.h"

#ifndef BOOT_TIME_PA
#define SMC_DRIVER_NAME    "/dev/tf_ctrl"
#else
#define SMC_DRIVER_NAME    "/dev/supervisor"
#endif

#define IOCTL_SCX_SMC_PA_CTRL \
         _IOWR('z', 0xFF, SCX_SMC_PA_CTRL)


typedef struct
{
   uint32_t nPACommand;       /* SCX_PA_CTRL_xxx */

   /* For the SCX_SMC_PA_CTRL_START command only                           */
   uint32_t nPASize;          /* PA buffer size                            */
   uint8_t* pPABuffer;        /* PA buffer                                 */
   uint32_t nConfSize;        /* Configuration buffer size, including the  */
                              /* zero-terminating character (may be zero)  */
   uint8_t* pConfBuffer;      /* Configuration buffer, zero-terminated     */
                              /* string (may be NULL)                      */
} SCX_SMC_PA_CTRL;

static uint8_t* readLocalFile(const char* pFileName, uint32_t* pnBufferSize, bool bIsString)
{
   uint8_t* pBuffer = NULL;
   FILE*    pFile = NULL;
   uint32_t nBytesToAllocate;
   int      nBytesRead;
   int      nResult;

   struct stat statFile;

   *pnBufferSize = 0;

   if (stat(pFileName, &statFile) != 0)
   {
      printf("Cannot read '%s' !\n", pFileName);
      goto error;
   }

   nBytesToAllocate = statFile.st_size;

   if (bIsString)
   {
      /* Allocate enough room for the zero-terminated string */
      nBytesToAllocate ++;
   }

   pBuffer = (uint8_t*)malloc(nBytesToAllocate);
   if (pBuffer == NULL)
   {
      printf("Out of memory for the buffer [%u bytes] !\n", nBytesToAllocate);
      goto error;
   }

   pFile = fopen(pFileName, "rb");
   if (pFile == NULL)
   {
      printf("Cannot open '%s' !\n", pFileName);
      goto error;
   }

   nBytesRead = fread(pBuffer, 1, statFile.st_size, pFile);

   if (nBytesRead != statFile.st_size)
   {
      printf("Cannot read bytes from '%s' [%i] !\n", pFileName, nBytesRead);
      goto error;
   }

   nResult = fclose(pFile);

   pFile = NULL;

   if (nResult != 0)
   {
      printf("Cannot close '%s' !\n", pFileName);
      goto error;
   }

   if (bIsString)
   {
      /* Set the zero-terminated string */
      pBuffer[nBytesRead] = 0;
   }

   *pnBufferSize = nBytesToAllocate;

   return pBuffer;

   /*
    * Error handling.
    */

error:
   free(pBuffer);
   if (pFile != NULL)
   {
      fclose(pFile);
   }

   return NULL;
}




int smcPAStart(const char* pPAFileName, const char* pConfFileName)
{
   int fd = 0;
   int nStatus = 0;
   SCX_SMC_PA_CTRL paCtrl;

   memset(&paCtrl, 0, sizeof(SCX_SMC_PA_CTRL));
   paCtrl.nPACommand = SCX_SMC_PA_CTRL_START;

#ifdef BOOT_TIME_PA
   printf("Starting the SMC BOOT PA '%s'. Driver name : %s", pPAFileName, SMC_DRIVER_NAME);
#else
   printf("Starting the SMC PA '%s'", pPAFileName);
#endif
   if (pConfFileName != NULL)
   {
      printf(" with the Configuration file '%s'", pConfFileName);
   }
   else
   {
      printf("Configuration file is mandatory\n");
      nStatus  = -1;
      goto end;
   }
   printf("...\n");

   paCtrl.pPABuffer = readLocalFile(pPAFileName, &paCtrl.nPASize, false);
   if (paCtrl.pPABuffer == NULL)
   {
      nStatus  = -2;
      goto end;
   }

   paCtrl.pConfBuffer = readLocalFile(pConfFileName, &paCtrl.nConfSize, false);
   if (paCtrl.pConfBuffer == NULL)
   {
      nStatus  = -4;
      goto end;
   }

   #ifndef WIN32
   fd = open(SMC_DRIVER_NAME, O_RDWR, 0);
   #endif
   if (fd == -1)
   {
      nStatus = errno;
#ifdef BOOT_TIME_PA
      printf("Boot time driver open failed [%d] !\n", nStatus);
#else
      printf("SMC driver open failed [%d] !\n", nStatus);
#endif
      goto end;
   }

   #ifndef WIN32
   nStatus = ioctl(fd, IOCTL_SCX_SMC_PA_CTRL, &paCtrl);
   #endif
   if (nStatus != 0)
   {
      nStatus = errno;
#ifdef BOOT_TIME_PA
      printf("Starting the BOOT TIME PA failed [%d] !\n", nStatus);
#else
      printf("Starting the SMC PA failed [%d] !\n", nStatus);
#endif
      goto end;
   }

#ifdef BOOT_TIME_PA
   printf("Boot time PA '%s' has been launched successfully.\n", pPAFileName);
#else
   printf("Starting the SMC PA '%s': Done\n", pPAFileName);
#endif

end:
   if (fd != 0)
   {
      #ifndef WIN32
      close(fd);
      #endif
   }

   free(paCtrl.pPABuffer);
   free(paCtrl.pConfBuffer);

   return nStatus;
}

int smcPAStop(void)
{
   int fd = 0;
   int nStatus = 0;
   SCX_SMC_PA_CTRL paCtrl;

   memset(&paCtrl, 0, sizeof(SCX_SMC_PA_CTRL));
   paCtrl.nPACommand = SCX_SMC_PA_CTRL_STOP;

   printf("Stopping the SMC PA...\n");

   #ifndef WIN32
   fd = open(SMC_DRIVER_NAME, O_RDWR, 0);
   #endif
   if (fd == 0)
   {
      nStatus = errno;
      printf("SMC driver open failed [%d] !\n", nStatus);
      goto end;
   }

   #ifndef WIN32
   nStatus = ioctl(fd, IOCTL_SCX_SMC_PA_CTRL, &paCtrl);
   #endif
   if (nStatus != 0)
   {
      printf("Stopping the SMC PA failed [%d] !\n", nStatus);
      goto end;
   }

   printf("Stopping the SMC PA: Done\n");

end:

   if (fd != 0)
   {
      #ifndef WIN32
      close(fd);
      #endif
   }

   return nStatus;
}
