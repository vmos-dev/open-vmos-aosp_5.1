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

/*----------------------------------------------------------------------------
 * Notes
 * =====
 *
 * This file is an extension to the delegation client for SMC omap3. It
 * permits to manage two differences with the original client:
 *  - command line parsing to take into account a file as input
 *  - associate a specific path to each partition
 *
 *----------------------------------------------------------------------------*/
#include <stdarg.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "s_version.h"
#include "s_error.h"
#include "s_type.h"
#include "smc_properties.h"

/*
 * This function :
 *    1) parses the configuration file of the SMC
 *    2) checks that all mandatory partition name are present and valid
 *    3) associates partition names to one of the 16 partition IDs
 *    4) Mapping is as follow:
 *          -- partition 0 <--> File System
 *          -- partition 1 <--> Keystore System
 *          -- partition 2 <--> Keystore User
 *          -- partition 15 <--> Super Partition
 *
 * It returns 0 in case of success and the error code requested by tf_daemon otherwise.
 */
int parseCommandLineExtension(char* configurationFileName, char* partitionNames[16])
{
   int error = 0;
   uint32_t i = 0;
   char * tmpChar = NULL;

   if (configurationFileName == NULL)
   {
      return 1;
   }

   error = smcPropertiesParse(configurationFileName);
   if ( error != 0 )
   {
      return error;
   }

   /* Allocate SMC partition file Name */
   for(i=0; i<15; i++)
   {
      partitionNames[i] = NULL;
   }


   /* File System File Name */
   tmpChar = smcGetPropertyAsString(FILE_SYSTEM_FILE_NAME);
   if ( tmpChar == NULL )
   {
      printf("Main system partition storage file name is missing.\n");
      goto return_error;
   }
   partitionNames[0] = malloc (strlen(tmpChar) + 1);
   sprintf(partitionNames[0], "%s", tmpChar);

   /* Keystore System File Name */
   tmpChar = smcGetPropertyAsString(KEYSTORE_SYSTEM_FILE_NAME);
   if ( tmpChar == NULL )
   {
      printf("Main system partition storage file name is missing.\n");
      goto return_error;
   }
   partitionNames[1] = malloc (strlen(tmpChar) + 1);
   sprintf(partitionNames[1], "%s", tmpChar);

   /* Keystore User File Name */
   tmpChar = smcGetPropertyAsString(KEYSTORE_USER_FILE_NAME);
   if ( tmpChar == NULL )
   {
      printf("Main system partition storage file name is missing.\n");
      goto return_error;
   }
   partitionNames[2] = malloc (strlen(tmpChar) + 1);
   sprintf(partitionNames[2], "%s", tmpChar);

   /* Super Partition File Name */
   tmpChar = smcGetPropertyAsString(SUPER_PARTITION_FILE_NAME);
   if ( tmpChar == NULL )
   {
      printf("Main system partition storage file name is missing.\n");
      goto return_error;
   }
   partitionNames[15] = malloc (strlen(tmpChar) + 1);
   sprintf(partitionNames[15], "%s", tmpChar);

   /* return Success */
   return 0;

return_error:
   for (i=0; i<15; i++)
   {
      if (partitionNames[i] != NULL) free(partitionNames[i]);
   }
   return 1;
}
