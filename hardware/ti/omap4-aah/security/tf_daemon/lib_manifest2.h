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

/*
 * This API allows parsing manifest files. A manifest is a text file that contains
 * a property set. A property is a name-value pair.
 *
 * The BNF syntax of a manifest file is :
 *
 * See Spec of Client Authentication
 *
 * Note: for each property, trailing spaces and tabs between the ':' separator
 * and the first character of the property value are discarded.
 */


#ifndef __LIB_MANIFEST2_H__
#define __LIB_MANIFEST2_H__

#include "s_error.h"
#include "s_type.h"

#ifdef __cplusplus
extern "C" {
#endif
#if 0
}  /* balance curly quotes */
#endif

/* The input file is a compiled manifest */
#define LIB_MANIFEST2_TYPE_COMPILED 1
/* The input file is a source manifest */
#define LIB_MANIFEST2_TYPE_SOURCE 2
/* The input file is a source manifest with sections */
#define LIB_MANIFEST2_TYPE_SOURCE_WITH_SECTIONS 3

typedef struct
{
   char* pManifestName;
   uint32_t nType;
   uint8_t* pManifestContent;
   uint32_t nManifestLength;
   uint32_t nOffset;
   uint32_t nLine;
   uint32_t nSectionStartOffset;
}
LIB_MANIFEST2_CONTEXT;

/* Must be used before libManifest2GetNextItem.
   The fields nType, pManifestContent, nManifestLength, and pManifestName (if applicable)
   must be filled-in before.
*/
void libManifest2InitContext(
   LIB_MANIFEST2_CONTEXT* pContext);

/**
 * Returns S_ITEM_NOT_FOUND for the last itel
 *
 * If type is LIB_MANIFEST2_TYPE_SOURCE, supports comments, multiple newlines, and leading BOM,
 * and checks that properties are not duplicated
 *
 * If type is LIB_MANIFEST2_TYPE_SOURCE_WITH_SECTIONS, returns *pValue == NULL for a section
 * Check that the section name contains only ASCII characters and that there is no duplicate
 * sections (same name, case insensitive)
 **/
S_RESULT libManifest2GetNextItem(
   LIB_MANIFEST2_CONTEXT* pContext,
   OUT uint8_t** ppName,
   OUT uint32_t* pNameLength,
   OUT uint8_t** ppValue,
   OUT uint32_t* pValueLength);

S_RESULT libManifest2CheckFormat(
   LIB_MANIFEST2_CONTEXT* pContext,
   uint32_t* pnItemCount);

#if 0
{  /* balance curly quotes */
#endif
#ifdef __cplusplus
}  /* closes extern "C" */
#endif

#endif  /* !defined(__LIB_MANIFEST_H__) */
