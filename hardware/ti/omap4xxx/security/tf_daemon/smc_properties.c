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
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <fcntl.h>

#if defined(_WIN32_WCE)
#include "os_wm.h"
#else
#include <sys/stat.h>
#endif

#include "smc_properties.h"
#include "smc_properties_parser.h"
#include "s_type.h"
#include "s_error.h"

#if defined(__SYMBIAN32__)
#include "smc_properties_symbian.h"
#endif


#define SECURE_CONFIG_FILE_HDR     66
#define SECURE_CONFIG_FILE_VERSION 01


#define SYSTEM_SECTION_NAME               "Global"

typedef enum {
   MANDATORY_FILE_SYSTEM_FILE_NAME,
   MANDATORY_KEYSTORE_SYSTEM_FILE_NAME,
   MANDATORY_KEYSTORE_USER_FILE_NAME,
   MANDATORY_SUPER_PARTITION_FILE_NAME,

   NB_MANDATORY_PROPS,
} MANDATORY_PROPS;


typedef enum
{
   STATE_NONE,
   STATE_DECIMAL,
   STATE_HEXA,
   STATE_BINARY
} INTEGER_FORMAT;

#if defined (LINUX) || defined(__ANDROID32__)
#define SEPARATOR_CHAR '/'

#elif defined (WIN32) || defined (__SYMBIAN32__) || defined (_WIN32_WCE)
#define SEPARATOR_CHAR '\\'

#else
#error SEPARATOR_CHAR not implemented...
#endif

#if defined(__SYMBIAN32__)
#define printf RDebugPrintf
#endif


/** the sturct that keep the data stored in the config file */
static CONF_FILE gConfFile;



/**
 * check the validity of a given path (is a directory, has the READ/WRITE access rights)
 * @param pPath the path we want to check
 * @return true if the path is OK, else false
 */
static bool checkFilePath(char *pPath)
{
   struct stat buf;
   uint32_t  result;
   char *pDir = pPath;

   // cobra & buffalo version : complete path (directory and filename) is given in the config file.
   // so we extract from this path the parent directory
   {
      uint32_t nSlashIndex = 0;
      uint32_t i = 0;
      while(pPath[i] != '\0')
      {
         if (pPath[i] == SEPARATOR_CHAR)
            nSlashIndex = i;
         i++;
      }
      pDir = malloc(sizeof(char) * nSlashIndex);
      if (pDir == NULL)
      {
         printf("Out of memory.");
         return false;
      }
      strncpy(pDir, pPath, nSlashIndex);
      pDir[nSlashIndex] = '\0';
   }

   /* check if file exists */
   result = stat(pDir, &buf);
   if(result != 0 )
   {
#if defined(__SYMBIAN32__)
      if (SymbianCheckFSDirectory(pDir) == -1)
      {
         printf("Cannot create directory : %s\n.", pDir);
         return false;
      }
#else
      printf("Unknown path : %s\n.", pDir);
      return false;
#endif
   }
   else
   {
      /* check it's a directory */
      if ((buf.st_mode & S_IFDIR) != S_IFDIR)
      {
         printf("Path %s doesn't point on a directory.\n", pDir);
         return false;
      }
#if (!defined(__SYMBIAN32__)) && (!defined(_WIN32_WCE)) && (!defined(__ANDROID32__))
      // TODO : under Symbian, Android and WM, check access right of a directory failed? I don't know why...
       /* check read access */
       if ((buf.st_mode & S_IREAD) != S_IREAD)
       {
          printf("Path %s doesn't have the READ access rights.\n", pDir);
          return false;
       }
       /* check write */
       if ((buf.st_mode & S_IWRITE) != S_IWRITE)
       {
          printf("Path %s doesn't have the WRITE access rights.\n", pDir);
          return false;
       }
#endif
   }

   return true;
}

/**
 * check properties (value, range...)
 * @param sConfFile struct where are stored the properties we will check
 * @return true if the check succeed, else false
 */
static bool smcPropertiesCheck(CONF_FILE sConfFile)
{
   NODE* pNext = NULL;
   char *pPropVal = NULL;
   bool bCheckResult = true;
   bool pMandatoryProps[NB_MANDATORY_PROPS];
   uint32_t i = 0;

   // reset properties table
   for (i=0; i<NB_MANDATORY_PROPS; i++)
      pMandatoryProps[i] = false;

   // check properties type and set MandatoryProps field to true (check later)
   pNext = sConfFile.sSystemSectionPropertyList.pFirst;
   while(pNext != NULL)
   {
      pPropVal = ((PROPERTY*)pNext)->pValue;

      //printf("Checking %s = %s.\n", pNext->pName, pPropVal);
      if(strcmp(pNext->pName, FILE_SYSTEM_FILE_NAME) == 0)
      {
         /* File System */
         bCheckResult = checkFilePath(pPropVal);
         pMandatoryProps[MANDATORY_FILE_SYSTEM_FILE_NAME] = true;
      }
      else if(strcmp(pNext->pName, KEYSTORE_SYSTEM_FILE_NAME) == 0)
      {
         bCheckResult = checkFilePath(pPropVal);
         pMandatoryProps[MANDATORY_KEYSTORE_SYSTEM_FILE_NAME] = true;
      }
      else if(strcmp(pNext->pName, KEYSTORE_USER_FILE_NAME) == 0)
      {
         bCheckResult = checkFilePath(pPropVal);
         pMandatoryProps[MANDATORY_KEYSTORE_USER_FILE_NAME] = true;
      }
      else if(strcmp(pNext->pName, SUPER_PARTITION_FILE_NAME) == 0)
      {
         bCheckResult = checkFilePath(pPropVal);
         pMandatoryProps[MANDATORY_SUPER_PARTITION_FILE_NAME] = true;
      }
      else
      {
         bCheckResult = true;
      }

      if (! bCheckResult)
      {
         printf("Property %s = %s. Bad value!!!\n", pNext->pName, pPropVal);
         return false;
      }
      pNext=pNext->pNext;
   }

   /* check all mandatory properties had been found */
   for (i=0; i<NB_MANDATORY_PROPS; i++)
   {
      if (!pMandatoryProps[i])
      {
         char *pMissingProp = NULL;
         switch(i){
            case MANDATORY_FILE_SYSTEM_FILE_NAME :
               pMissingProp = FILE_SYSTEM_FILE_NAME;
               break;
            case MANDATORY_KEYSTORE_SYSTEM_FILE_NAME :
               pMissingProp = KEYSTORE_SYSTEM_FILE_NAME;
               break;
            case MANDATORY_KEYSTORE_USER_FILE_NAME :
               pMissingProp = KEYSTORE_USER_FILE_NAME;
               break;
            case MANDATORY_SUPER_PARTITION_FILE_NAME :
               pMissingProp = SUPER_PARTITION_FILE_NAME;
               break;
         }
         printf("Mandatory property %s is missing.\n", pMissingProp);
         bCheckResult = false;
      }
   }

   return bCheckResult;
}



/**
 * parse the config file
 * @param configFile the path of the configuration file
 * @return 0 if succeed, else 1
 */
int smcPropertiesParse(const char *configFile)
{
   S_RESULT nResult = S_SUCCESS;

   // first : parse the config file
   memset(&gConfFile, 0x00, sizeof(CONF_FILE));
   nResult=SMCPropParseConfigFile((char *)configFile, &gConfFile);
   if (nResult!=S_SUCCESS)
   {
      printf("Parsing error in file %s : %x.\n", configFile, nResult);
      return 1;
   }

   // check properties
   if (!smcPropertiesCheck(gConfFile))
   {
      printf("Properties check failed.\n");
      return 1;
   }

   return 0;
}



/**
 * get the value of a property
 * @param pProp we are asking the value of this property
 * @return the value if found, else NULL
 */
char *smcGetPropertyAsString(char *pProp)
{
   return SMCPropGetSystemProperty(&gConfFile, pProp);
}


/**
 * get the value of a property
 * @param pProp we are asking the value of this property
 * @param pVal the value of the property
 * @return 0 if found, else 1 (and pVal set to 0)
 */
int smcGetPropertyAsInt(char *pProp, int *pVal)
{
   char *pStr = SMCPropGetSystemProperty(&gConfFile, pProp);
   if (pStr == NULL)
   {
      *pVal = 0;
      return 1;
   }
   if (libString2GetStringAsInt(pStr, (uint32_t*)pVal) == S_SUCCESS)
   {
      return 0;
   }
   *pVal = 0;
   return 1;
}
