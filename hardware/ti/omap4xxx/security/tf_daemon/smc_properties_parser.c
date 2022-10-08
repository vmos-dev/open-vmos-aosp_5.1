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
#include <assert.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#if defined(_WIN32_WCE)
#include "os_wm.h"
#else
#include <errno.h>
#endif

#include "smc_properties_parser.h"
#include "lib_manifest2.h"
#include "s_error.h"

/* ---------------------------------------------------------------------------------
   Defines
   ---------------------------------------------------------------------------------*/

#define STRUE                             "true"
#define SFALSE                            "false"

#if defined(_WIN32_WCE)
#define GET_LAST_ERR GetLastError()
#else
#define GET_LAST_ERR  errno
#endif

#if defined (LINUX) || defined (__SYMBIAN32__) || defined (__ANDROID32__)
#define STRICMP strcasecmp
#elif defined(_WIN32_WCE)
#define STRICMP _stricmp
#else
#define STRICMP stricmp
#endif


/* ---------------------------------------------------------------------------------
   Logs and Traces.
   ---------------------------------------------------------------------------------*/
#ifdef __SYMBIAN32__
#include "os_symbian.h"
#elif NDEBUG
/* Compile-out the traces */
#define TRACE_ERROR(...)
#define TRACE_WARNING(...)
#define TRACE_INFO(...)
#else
#include <stdarg.h>
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

/* ---------------------------------------------------------------------------------
   private functions.
   ---------------------------------------------------------------------------------*/

static NODE* static_listFindNodeElement(NODE* pList,char* pName,bool bIsCaseSensitive)
{
   int32_t nCmp;

   assert(pName!=NULL);

   while (pList!=NULL)
   {
      if (bIsCaseSensitive)
      {
         nCmp=strcmp(pName,pList->pName);
      }
      else
      {
         nCmp=STRICMP(pName,pList->pName);
      }
      if (nCmp>0)
      {
         pList=pList->pRight;
      }
      else if (nCmp<0)
      {
         pList=pList->pLeft;
      }
      else
      {
         break;
      }
   }
   return pList;
}


static S_RESULT static_listSortedAddNode(NODE* pList,NODE* pNode)
{
   int32_t nCmp;

   do {
      nCmp=strcmp(pNode->pName,pList->pName);
      if (nCmp>0)
      {
         if (pList->pRight!=NULL)
         {
            pList=pList->pRight;
         }
         else
         {
            pList->pRight=pNode;
            /* update linked list */
            pNode->pPrevious=pList;
            pNode->pNext=pList->pNext;
            if (pList->pNext!=NULL)
            {
               pList->pNext->pPrevious=pNode;
            }
            pList->pNext=pNode;
            return S_SUCCESS;
         }
      }
      else if (nCmp<0)
      {
         if (pList->pLeft!=NULL)
         {
            pList=pList->pLeft;
         }
         else
         {
            pList->pLeft=pNode;
            /* update linked list */
            pNode->pNext=pList;
            pNode->pPrevious=pList->pPrevious;
            if (pList->pPrevious!=NULL)
            {
               pList->pPrevious->pNext=pNode;
            }
            pList->pPrevious=pNode;
            return S_SUCCESS;
         }
      }
   } while (nCmp!=0);

   TRACE_ERROR("%s already exist !\n",pNode->pName);
   return S_ERROR_ITEM_EXISTS;
}


static S_RESULT SMCPropListSortedAdd(LIST* pList,NODE* pNode)
{
   S_RESULT nResult;

   assert(pList!=NULL && pNode!=NULL);

   if (pNode->pName==NULL)
   {
	   TRACE_ERROR("Trying to insert a NULL node name !\n");
      return S_ERROR_BAD_PARAMETERS;
   }

   if (pList->pRoot==NULL)
   {
      pList->pRoot=pNode;
      pList->pFirst=pNode;
      return S_SUCCESS;
   }
   else
   {
      nResult=static_listSortedAddNode(pList->pRoot,pNode);
      /* update the first node of the linked list */
      if (nResult==S_SUCCESS && pNode->pPrevious==NULL)
      {
         pList->pFirst=pNode;
      }
   }
   return nResult;
}


static NODE* SMCPropListFindElement(LIST* pList,char* pName,bool bIsCaseSensitive)
{
   if (pList->pRoot!=NULL)
   {
      return static_listFindNodeElement(pList->pRoot,pName,bIsCaseSensitive);
   }
   return NULL;
}


static S_RESULT SMCPropYacc(uint8_t* pBuffer, uint32_t nBufferLength,
                     CONF_FILE* pConfFile)
{
   S_RESULT nError=S_SUCCESS;
   LIST *pPublicPropertyList=NULL;
   LIST *pPrivatePropertyList=NULL;
   PROPERTY* pProperty=NULL;
   SERVICE_SECTION* pServSection;
   SERVICE_SECTION* pPreviousService=NULL;

   uint8_t* pName;
   uint32_t nNameLength;
   uint8_t* pValue;
   uint32_t nValueLength;
   char* pNameZ = NULL;
   char* pValueZ = NULL;
   LIB_MANIFEST2_CONTEXT sParserContext;
   char serviceManifestName[1024];

   sParserContext.pManifestName = "Configuration File";
   sParserContext.pManifestContent = pBuffer;
   sParserContext.nManifestLength = nBufferLength;
   sParserContext.nType = LIB_MANIFEST2_TYPE_SOURCE_WITH_SECTIONS;

   libManifest2InitContext(&sParserContext);

   while (true)
   {
      nError = libManifest2GetNextItem(
         &sParserContext,
         &pName,
         &nNameLength,
         &pValue,
         &nValueLength);
      if (nError == S_ERROR_ITEM_NOT_FOUND)
      {
         /* End of parsing */
         nError = S_SUCCESS;
         break;
      }
      else if (nError != S_SUCCESS)
      {
         /* Error */
         goto error;
      }

      /* Duplicate name and value in as zero-terminated strings */
      /* Unclean: those strings are not going to be deallocated
         This is not a problem because this code is run in a tool
      */
      pNameZ = malloc(nNameLength+1);
      if (pNameZ == NULL)
      {
         nError = S_ERROR_OUT_OF_MEMORY;
         goto error;
      }
      memcpy(pNameZ, pName, nNameLength);
      pNameZ[nNameLength] = 0;

      if (pValue == NULL)
      {
         /* It's a section */
         if (STRICMP(pNameZ, SYSTEM_SECTION_NAME) == 0)
         {
            free(pNameZ);
            pPublicPropertyList=&pConfFile->sSystemSectionPropertyList;
         }
         else
         {
            pServSection=(SERVICE_SECTION*)SMCPropListFindElement(
               &pConfFile->sDriverSectionList,
               pNameZ,
               false);
            if (pServSection==NULL)
            {
               pServSection=(SERVICE_SECTION*)SMCPropListFindElement(
                     &pConfFile->sPreinstalledSectionList,
                     pNameZ,
                     false);
            }
            if (pServSection==NULL)
            {
               pServSection=(SERVICE_SECTION*)SMCPropListFindElement(
                  &pConfFile->sSectionList,
                  pNameZ,
                  false);
               if (pServSection==NULL)
               {
                  nError=S_ERROR_ITEM_NOT_FOUND;
                  goto error;
               }
            }
            free(pNameZ);

            pServSection->inSCF=true;
            if (pPreviousService!=NULL)
            {
               pPreviousService->pNextInSCF=pServSection;
            }
            else
            {
               pConfFile->pFirstSectionInSCF=pServSection;
            }
            pPreviousService=pServSection;

            pPublicPropertyList=&pServSection->sPublicPropertyList;
            pPrivatePropertyList=&pServSection->sPrivatePropertyList;
         }
      }
      else
      {
         /* It's a property definition */
         pValueZ = malloc(nValueLength+1);
         if (pValueZ == NULL)
         {
            nError = S_ERROR_OUT_OF_MEMORY;
            goto error;
         }
         memcpy(pValueZ, pValue, nValueLength);
         pValueZ[nValueLength] = 0;

         pProperty=(PROPERTY*)malloc(sizeof(PROPERTY));
         if (pProperty==NULL)
         {
            nError=S_ERROR_OUT_OF_MEMORY;
            goto error;
         }
         memset(pProperty, 0x00, sizeof(PROPERTY));
         pProperty->sNode.pName=pNameZ;

         pProperty->pValue=pValueZ;

         if (pPrivatePropertyList==NULL)
         {
            nError=SMCPropListSortedAdd(pPublicPropertyList,(NODE*)pProperty);
            if (nError!=S_SUCCESS)
            {
               goto error;
            }
         }
         else
         {
            if ((nValueLength > strlen(CONFIG_PROPERTY_NAME)) &&
                (memcmp(pProperty->sNode.pName, CONFIG_PROPERTY_NAME, strlen(CONFIG_PROPERTY_NAME)) == 0))
            {
               nError=SMCPropListSortedAdd(pPrivatePropertyList,(NODE*)pProperty);
            }
            else
            {
               nError=SMCPropListSortedAdd(pPublicPropertyList,(NODE*)pProperty);
            }
            if (nError!=S_SUCCESS)
            {
               goto error;
            }
         }
      }
   }

error:
   if (nError!=S_SUCCESS)
   {
      switch (nError)
      {
      case S_ERROR_BAD_FORMAT:
         /* Error message already output */
         break;
      case S_ERROR_WRONG_SIGNATURE:
         TRACE_ERROR("Configuration file: wrong service UUID: %s\n", pValueZ);
         break;
      case S_ERROR_OUT_OF_MEMORY:
	  TRACE_ERROR("Out of memory\n");
         break;
      case S_ERROR_ITEM_NOT_FOUND:
	  TRACE_ERROR("Configuration file: service \"%s\" not found\n", pNameZ);
         break;
      }
   }
   return nError;
}


S_RESULT static_readFile(const char* pFilename, void** ppFile, uint32_t* pnFileLength)
{
   S_RESULT nResult = S_SUCCESS;
   long nFilesize;
   FILE* pFile = NULL;
   void *pBuff = NULL;

   // open file and get its size...
   if ((pFile = fopen(pFilename, "rb")) == NULL)
   {
      TRACE_ERROR("static_readFile: fopen(%s) failed [%d]", pFilename, GET_LAST_ERR);
	   nResult = S_ERROR_ITEM_NOT_FOUND;
	   return nResult;
   }
   if (fseek(pFile, 0, SEEK_END) != 0)
   {
      TRACE_ERROR("static_readFile: fseek(%s) failed [%d]", pFilename, GET_LAST_ERR);
	   nResult = S_ERROR_UNDERLYING_OS;
	   goto error;
   }
   nFilesize = ftell(pFile);
   if (nFilesize < 0)
   {
      TRACE_ERROR("static_readFile: ftell(%s) failed [%d]", pFilename, GET_LAST_ERR);
	   nResult = S_ERROR_UNDERLYING_OS;
	   goto error;
   }
   rewind(pFile);

   // allocate the buffer
   pBuff = malloc(nFilesize + 1);
   if (pBuff == NULL)
   {
      TRACE_ERROR("static_readFile: out of memory");
      nResult = S_ERROR_OUT_OF_MEMORY;
      goto error;
   }

   // read the file
   if (fread(pBuff, sizeof(uint8_t), (size_t)nFilesize, pFile) != (size_t)nFilesize)
   {
      TRACE_ERROR("static_readFile: fread failed [%d]", GET_LAST_ERR);
      nResult = S_ERROR_UNDERLYING_OS;
      goto error;
   }
   ((char*)pBuff)[nFilesize] = 0;

   *ppFile = pBuff;
   *pnFileLength = nFilesize;
   return S_SUCCESS;

error:
   if (pBuff != NULL)
      free(pBuff);
   fclose(pFile);

   *ppFile = NULL;
   *pnFileLength = 0;
   return nResult;
}





/* ---------------------------------------------------------------------------------
   API functions.
   ---------------------------------------------------------------------------------*/

char* SMCPropGetSystemProperty(CONF_FILE* pConfFile, char* pPropertyName)
{
   PROPERTY* pProperty;

   pProperty=(PROPERTY*)SMCPropListFindElement(
      &pConfFile->sSystemSectionPropertyList,
      pPropertyName,
      true);
   if (pProperty!=NULL)
   {
      return pProperty->pValue;
   }
   return NULL;
}

uint32_t SMCPropGetSystemPropertyAsInt(CONF_FILE* pConfFile, char* pPropertyName)
{
   uint32_t nValue;
   char* pValue=SMCPropGetSystemProperty(pConfFile,pPropertyName);

   if (libString2GetStringAsInt(pValue, &nValue) == S_SUCCESS)
   {
      return nValue;
   }
   return 0;
}


S_RESULT SMCPropParseConfigFile(char* pConfigFilename,CONF_FILE* pConfFile)
{
   S_RESULT nError=S_SUCCESS;
   void* pFile;
   uint32_t nFileLength;
   bool bReuseManifest;

   assert(pConfFile!=NULL);

   TRACE_INFO("Processing configuration file '%s'", pConfigFilename);

   if(pConfigFilename != NULL)
   {
      nError=static_readFile(pConfigFilename,&pFile,&nFileLength);
      if (nError!=S_SUCCESS)
      {
         goto error;
      }
      bReuseManifest = true;
   }
   else
   {
      assert(0);
   }

   nError=SMCPropYacc(pFile,nFileLength,pConfFile);

   if(pConfigFilename != NULL)
   {
      free(pFile);
   }

error:
   return nError;
}
