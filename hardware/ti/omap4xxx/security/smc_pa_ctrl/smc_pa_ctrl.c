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

#include "s_type.h"
#include "s_version.h"
#include "smc_pa_ctrl_os.h"


/*---------------------------------------------------------------------------
 * Utility Functions
 *---------------------------------------------------------------------------*/

static void printUsage(bool bSuccess)
{
#ifdef BOOT_TIME_PA
   printf("usage : smc_boot_pa_ctrl <options> [command]\n");
   printf("   Options:\n");
   printf("      -h, --help: Print this help\n");
   printf("   Commands:\n");
   printf("      -c <conf> : Configuration file path\n");
   printf("      start <pa_file_path>: load and start the SMC PA\n");
   printf("\n");
#else
   printf("usage : smc_pa_ctrl <options> [command]\n");
   printf("   Options:\n");
   printf("      -h, --help: Print this help\n");
   printf("   Commands:\n");
   printf("      -c <conf> : Configuration file path\n");
   printf("      start <pa_file_path>: load and start the SMC PA\n");
   printf("      stop: stop the SMC PA\n");
   printf("\n");
#endif

   exit(bSuccess ? 0 : 1);
}



/*---------------------------------------------------------------------------
 * Application Entry-Point
 *---------------------------------------------------------------------------*/

int main(int argc, char *argv[])
{
   char* pPAFileName = NULL;
   char* pConfFileName = NULL;
   int   nCommand = SCX_SMC_PA_CTRL_NONE;
   int   nStatus = 0;

#ifdef BOOT_TIME_PA
   printf("SMC BOOT PA Control\n");
   printf(S_VERSION_STRING "\n");
#else
   printf("SMC PA Control\n");
   printf(S_VERSION_STRING "\n");
#endif

   /* Skip program name */
   argv ++;
   argc --;

   while (argc != 0)
   {
      if (argv[0][0] == '-')
      {
         /*
          * This is an option
          */

         if ((strcmp(argv[0], "--help") == 0) || (strcmp(argv[0], "-h") == 0))
         {
            printUsage(true);
         }
         else if (strcmp(argv[0], "-c") == 0)
         {
            /* Next argument */
            argc --;
            argv ++;

            if (argc <= 0)
            {
               printf("Missing argument for the option '-c'\n\n");
               printUsage(false);
            }

            pConfFileName = malloc(strlen(argv[0]) + 1);
            if (pConfFileName == NULL)
            {
               printf("Out of memory\n");
               exit(2);
            }

            strcpy(pConfFileName, argv[0]);
         }
         else
         {
            printf("Invalid option [%s]\n\n", argv[0]);
            printUsage(false);
         }
      }
      else
      {
         /*
          * This is a command
          */
         if (strcmp(argv[0], "start") == 0)
         {
            /* Next argument */
            argc --;
            argv ++;

            if (argc <= 0)
            {
               printf("Missing argument for the command 'start'\n\n");
               printUsage(false);
            }

            pPAFileName = malloc(strlen(argv[0]) + 1);
            if (pPAFileName == NULL)
            {
               printf("Out of memory\n");
               exit(2);
            }

            strcpy(pPAFileName, argv[0]);

            nCommand = SCX_SMC_PA_CTRL_START;
         }
#ifndef BOOT_TIME_PA
         else if (strcmp(argv[0], "stop") == 0)
         {
            nCommand = SCX_SMC_PA_CTRL_STOP;
         }
#endif
         else
         {
            printf("Invalid command [%s]\n\n", argv[0]);
            printUsage(false);
         }
      }

      argc --;
      argv ++;
   }

   switch (nCommand)
   {
      case SCX_SMC_PA_CTRL_START:
         /*
          * Load and execute the SMC PA
          */

         if (pConfFileName == NULL)
         {
            printf("Configuration file path is missing !\n");
            printUsage(false);
         }

         nStatus = smcPAStart(pPAFileName, pConfFileName);
         break;

#ifndef BOOT_TIME_PA
      case SCX_SMC_PA_CTRL_STOP:
         /*
          * Stop the SMC PA
          */

         if (pConfFileName != NULL)
         {
            printf("Configuration file cannot be used with the 'stop' command\n\n");
            printUsage(false);
         }

         nStatus = smcPAStop();
         break;
#endif

      default:
         printf("No command specified\n\n");
         printUsage(false);
         break;
   }

   free(pPAFileName);
   free(pConfFileName);

   return nStatus;
}
