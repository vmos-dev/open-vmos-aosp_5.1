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


#ifndef __SMC_PROPERTIES_PARSER_H__
#define __SMC_PROPERTIES_PARSER_H__



#include "s_type.h"

#ifdef __cplusplus
extern "C" {
#endif


/* ---------------------------------------------------------------------------------
   Defines
   ---------------------------------------------------------------------------------*/

#define SYSTEM_SECTION_NAME               "Global"
#define CONFIG_SERVICE_ID_PROPERTY_NAME   "config.s.serviceID"
#define CONFIG_PROPERTY_NAME              "config."



/* ---------------------------------------------------------------------------------
   types definition
   ---------------------------------------------------------------------------------*/

typedef struct NODE
{
   struct NODE* pLeft;
   struct NODE* pRight;
   struct NODE* pNext;
   struct NODE* pPrevious;
   char* pName;
} NODE;

typedef struct
{
   NODE* pRoot;
   NODE* pFirst;
} LIST;

typedef struct
{
   NODE sNode;
   char* pValue;
   bool bChecked; /* Whether it has been checked that this property is allowed */
} PROPERTY;

typedef struct
{
   NODE sNode;
   struct S_PROPERTY* pProperty;
} S_PROPERTY_NODE;

typedef struct SERVICE_SECTION
{
   NODE sNode;
   bool inSCF;
   struct SERVICE_SECTION* pNextInSCF; /* next section defined in config file */
   S_UUID sUUID;
   uint32_t nFlags;
   char* pComment;
   void* pFileInfo;     /* used to retreive filename and MD5 hash (optional) */
   LIST sPublicPropertyList;
   LIST sPrivatePropertyList;
} SERVICE_SECTION;

typedef struct
{
   char* pComment;
   LIST sSystemSectionPropertyList;
   SERVICE_SECTION* pFirstSectionInSCF; /* first section defined in config file */
   LIST sDriverSectionList;
   LIST sPreinstalledSectionList;
   LIST sSectionList;
} CONF_FILE;



/* ---------------------------------------------------------------------------------
   Prototypes
   ---------------------------------------------------------------------------------*/

uint32_t SMCPropStringToInt           (char* pValue);
char*    SMCPropGetSystemProperty     (CONF_FILE* pConfFile, char* pPropertyName);
uint32_t SMCPropGetSystemPropertyAsInt(CONF_FILE* pConfFile, char* pPropertyName);
S_RESULT SMCPropParseConfigFile       (char* pConfigFilename,CONF_FILE* pConfFile);


#ifdef __cplusplus
}
#endif

#endif /* __SMC_PROPERTIES_PARSER_H__ */
