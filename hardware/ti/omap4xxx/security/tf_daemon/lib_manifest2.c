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
#include "lib_manifest2.h"
#include <string.h>

#define CHAR_CR  (uint8_t)0x0D
#define CHAR_LF  (uint8_t)0x0A
#define CHAR_TAB (uint8_t)0x09

#ifdef LIB_TOOL_IMPLEMENTATION
#include "exos_trace.h"
#define LOG_ERROR(pContext, msg, ...) log_error("%s - line %d: " msg, pContext->pManifestName, pContext->nLine, __VA_ARGS__)
static void log_error(const char* msg, ...)
{
   va_list arg_list;
   va_start(arg_list, msg);
   exosTraceVPrintf("LIB_MANIFEST2", EXOS_TRACE_ORG_APPLI, K_PRINT_ERROR_LOG, msg, &arg_list);
   va_end(arg_list);
}
#else
/* No error messages on the target */
#ifdef __SYMBIAN32__
#define LOG_ERROR(pContext...)
#else
#define LOG_ERROR(...)
#endif
#endif

void libManifest2InitContext(
   LIB_MANIFEST2_CONTEXT* pContext)
{
   pContext->nOffset = 0;
   pContext->nLine = 1;
   pContext->nSectionStartOffset = 0;
}


#define CHARACTER_NAME_FIRST      1
#define CHARACTER_NAME_SUBSEQUENT 2
#define CHARACTER_SECTION_NAME    3

static bool static_checkCharacter(uint8_t x, uint32_t nType)
{
   /* [A-Za-z0-9] is acceptable for everyone */
   if (x  >= (uint8_t)'a' && x <= (uint8_t)'z')
   {
      return true;
   }
   if (x >=(uint8_t)'A' && x <= (uint8_t)'Z')
   {
      return true;
   }
   if (x >= (uint8_t)'0' && x <= (uint8_t)'9')
   {
      return true;
   }
   if (nType == CHARACTER_NAME_FIRST)
   {
      return false;
   }
   /* Subsequent property name or section name characters can be [_.-] */
   if (x == (uint8_t)'_' || x == (uint8_t)'.' || x == (uint8_t)'-')
   {
      return true;
   }
   if (nType == CHARACTER_NAME_SUBSEQUENT)
   {
      return false;
   }
   /* Space is also allowed in section names */
   if (x == (uint8_t)' ')
   {
      return true;
   }
   return false;
}

static bool static_sectionNameEqualCaseInsensitive(
   uint8_t* pName1,
   uint32_t nName1Length,
   uint8_t* pName2,
   uint32_t nName2Length)
{
   uint32_t i;
   if (nName1Length != nName2Length)
   {
      return false;
   }
   for (i = 0; i < nName1Length; i++)
   {
      uint8_t x1 = pName1[i];
      uint8_t x2 = pName2[i];

      /* This code assumes the characters have been checked before */

      if ((x1 & ~0x20) != (x2 & ~0x20))
      {
         return false;
      }
   }
   return true;
}

static S_RESULT static_libManifest2GetNextItemInternal(
   LIB_MANIFEST2_CONTEXT* pContext,
   OUT uint8_t** ppName,
   OUT uint32_t* pNameLength,
   OUT uint8_t** ppValue,
   OUT uint32_t* pValueLength)
{
   S_RESULT nResult = S_ERROR_BAD_FORMAT;
   uint8_t* pCurrent = pContext->pManifestContent + pContext->nOffset;
   uint8_t* pEnd = pContext->pManifestContent + pContext->nManifestLength;
   uint8_t* pLastNonWhitespaceChar;
   uint32_t nCurrentSequenceCount;
   uint32_t nCurrentChar;

   if (pContext->nType != LIB_MANIFEST2_TYPE_COMPILED)
   {
      /* Skip leading BOM if we're at the start */
      if (pCurrent == pContext->pManifestContent)
      {
         /* We're at the start. Skip leading BOM if present */
         /* Note that the UTF-8 encoding of the BOM marker is EF BB BF */
         if (pContext->nManifestLength >= 3
             && pCurrent[0] == 0xEF
             && pCurrent[1] == 0xBB
             && pCurrent[2] == 0xBF)
         {
            pCurrent += 3;
         }
      }
      /* Skip comments and newlines */
      while (pCurrent < pEnd)
      {
         if (*pCurrent == (uint8_t)'#')
         {
            /* This is the start of a comment. Skip until end of line or end of file */
            pCurrent++;
            while (pCurrent < pEnd && *pCurrent != CHAR_LF && *pCurrent != CHAR_CR)
            {
               if (*pCurrent == 0)
               {
                  LOG_ERROR(pContext, "NUL character forbidden");
                  goto error;
               }
               pCurrent++;
            }
         }
         else if (*pCurrent == CHAR_CR)
         {
            /* Check if a LF follows */
            pCurrent++;
            if (pCurrent < pEnd && *pCurrent == CHAR_LF)
            {
               pCurrent++;
            }
            pContext->nLine++;
         }
         else if (*pCurrent == CHAR_LF)
         {
            pCurrent++;
            pContext->nLine++;
         }
         else if (*pCurrent == ' ' || *pCurrent == '\t')
         {
            /* this is the start of a all-whitespace line */
            /* NOTE: this is not allowed by the current spec: spec update needed */
            pCurrent++;
            while (pCurrent < pEnd)
            {
               if (*pCurrent == CHAR_LF || *pCurrent == CHAR_CR)
               {
                  /* End-of-line reached */
                  break;
               }
               if (! (*pCurrent == ' ' || *pCurrent == '\t'))
               {
                  LOG_ERROR(pContext, "A line starting with whitespaces must contain only whitespaces. Illegal character: 0x%02X", *pCurrent);
                  goto error;
               }
               pCurrent++;
            }
         }
         else
         {
            break;
         }
      }
   }

   if (pCurrent >= pEnd)
   {
      /* No more properties */
      nResult = S_ERROR_ITEM_NOT_FOUND;
      goto error;
   }

   if (pContext->nType == LIB_MANIFEST2_TYPE_SOURCE_WITH_SECTIONS)
   {
      if (*pCurrent == '[')
      {
         /* This is a section descriptor */
         pCurrent++;
         *ppName = pCurrent;
         *ppValue = NULL;
         *pValueLength = 0;
         while (true)
         {
            if (pCurrent >= pEnd)
            {
               LOG_ERROR(pContext, "EOF reached within a section name");
               goto error;
            }
            if (*pCurrent == ']')
            {
               /* End of section name */
               *pNameLength = pCurrent - *ppName;
               pCurrent++;

               /* Skip spaces and tabs. Note that this is a deviation from the current spec
                 (see SWIS). Spec must be updated */
               while (pCurrent < pEnd)
               {
                  if (*pCurrent == ' ' || *pCurrent == '\t')
                  {
                     pCurrent++;
                  }
                  else if (*pCurrent == CHAR_CR || *pCurrent == CHAR_LF)
                  {
                     /* End of line */
                     break;
                  }
                  else
                  {
                     LOG_ERROR(pContext, "Non-space character follows a sectino header: 0x02X", *pCurrent);
                  }
               }
               pContext->nOffset = pCurrent - pContext->pManifestContent;
               pContext->nSectionStartOffset = pContext->nOffset;
               return S_SUCCESS;
            }
            /* Check section name character */
            if (!static_checkCharacter(*pCurrent, CHARACTER_SECTION_NAME))
            {
               LOG_ERROR(pContext, "Invalid character for a section name: 0x%02X", *pCurrent);
               goto error;
            }
            pCurrent++;
         }
      }

      if (pContext->nSectionStartOffset == 0)
      {
         /* No section has been found yet. This is a bad format */
         LOG_ERROR(pContext, "Property found outside any section");
         goto error;
      }
   }

   *ppName = pCurrent;

   /* Check first character of name is in [A-Za-z0-9] */
   if (!static_checkCharacter(*pCurrent, CHARACTER_NAME_FIRST))
   {
      LOG_ERROR(pContext, "Invalid first character for a property name: 0x%02X", *pCurrent);
      goto error;
   }
   pCurrent++;
   pLastNonWhitespaceChar = pCurrent;
   while (true)
   {
      if (pCurrent == pEnd)
      {
         LOG_ERROR(pContext, "EOF reached within a property name");
         goto error;
      }
      if (*pCurrent == ':')
      {
         /* Colon reached */
         break;
      }
      if (pContext->nType != LIB_MANIFEST2_TYPE_COMPILED)
      {
         /* In source manifest, allow space characters before the colon.
            This is a deviation from the spec. Spec must be updated */
         if (*pCurrent == ' ' || *pCurrent == '\t')
         {
            pCurrent++;
            continue;
         }
      }
      if (!static_checkCharacter(*pCurrent, CHARACTER_NAME_SUBSEQUENT))
      {
         LOG_ERROR(pContext, "Invalid character for a property name: 0x%02X", *pCurrent);
         goto error;
      }
      if (pContext->nType != LIB_MANIFEST2_TYPE_COMPILED)
      {
         /* Even in a source manifest, property name cannot contain spaces! */
         if (pCurrent != pLastNonWhitespaceChar)
         {
            LOG_ERROR(pContext, "Property name cannot contain spaces");
            goto error;
         }
      }
      pCurrent++;
      pLastNonWhitespaceChar = pCurrent;
   }
   *pNameLength = pLastNonWhitespaceChar - *ppName;
   pCurrent++;
   /* Skip spaces and tabs on the right of the colon */
   while (pCurrent < pEnd && (*pCurrent == ' ' || *pCurrent == '\t'))
   {
      pCurrent++;
   }
   *ppValue = pCurrent;
   pLastNonWhitespaceChar = pCurrent-1;

   nCurrentSequenceCount = 0;
   nCurrentChar = 0;

   while (pCurrent < pEnd)
   {
      uint32_t x;
      x = *pCurrent;
      if ((x & 0x80) == 0)
      {
         if (nCurrentSequenceCount != 0)
         {
            /* We were expecting a 10xxxxxx byte: ill-formed UTF-8 */
            LOG_ERROR(pContext, "Invalid UTF-8 sequence");
            goto error;
         }
         else if (x == 0)
         {
            /* The null character is forbidden */
            LOG_ERROR(pContext, "NUL character forbidden");
            goto error;
         }
         /* We have a well-formed Unicode character */
         nCurrentChar = x;
      }
      else if ((x & 0xC0) == 0xC0)
      {
         /* Start of a sequence */
         if (nCurrentSequenceCount != 0)
         {
            /* We were expecting a 10xxxxxx byte: ill-formed UTF-8 */
            LOG_ERROR(pContext, "Invalid UTF-8 sequence");
            goto error;
         }
         else if ((x & 0xE0) == 0xC0)
         {
            /* 1 byte follows */
            nCurrentChar = x & 0x1F;
            nCurrentSequenceCount = 1;
            if ((x & 0x1E) == 0)
            {
               /* Illegal UTF-8: overlong encoding of character in the [0x00-0x7F] range
                  (must use 1-byte encoding, not a 2-byte encoding) */
               LOG_ERROR(pContext, "Invalid UTF-8 sequence");
               goto error;
            }
         }
         else if ((x & 0xF0) == 0xE0)
         {
            /* 2 bytes follow */
            nCurrentChar = x & 0x0F;
            nCurrentSequenceCount = 2;
         }
         else if ((x & 0xF8) == 0xF0)
         {
            /* 3 bytes follow */
            nCurrentChar = x & 0x07;
            nCurrentSequenceCount = 3;
         }
         else
         {
            /* Illegal start of sequence */
            LOG_ERROR(pContext, "Invalid UTF-8 sequence");
            goto error;
         }
      }
      else if ((x & 0xC0) == 0x80)
      {
         /* Continuation byte */
         if (nCurrentSequenceCount == 0)
         {
            /* We were expecting a sequence start, not a continuation byte */
            LOG_ERROR(pContext, "Invalid UTF-8 sequence");
            goto error;
         }
         else
         {
            if (nCurrentSequenceCount == 2)
            {
               /* We're in a 3-byte sequence, check that we're not using an overlong sequence */
               if (nCurrentChar == 0 && (x & 0x20) == 0)
               {
                  /* The character starts with at least 5 zero bits, so has fewer than 11 bits. It should
                     have used a 2-byte sequence, not a 3-byte sequence */
                  LOG_ERROR(pContext, "Invalid UTF-8 sequence");
                  goto error;
               }
            }
            else if (nCurrentSequenceCount == 3)
            {
               if (nCurrentChar == 0 && (x & 0x30) == 0)
               {
                  /* The character starts with at least 5 zero bits, so has fewer than 16 bits. It should
                     have used a 3-byte sequence, not a 4-byte sequence */
                  LOG_ERROR(pContext, "Invalid UTF-8 sequence");
                  goto error;
               }
            }
            nCurrentSequenceCount--;
            nCurrentChar = (nCurrentChar << 6) | (x & 0x3F);
         }
      }
      else
      {
         /* Illegal byte */
         LOG_ERROR(pContext, "Invalid UTF-8 sequence");
         goto error;
      }
      if (nCurrentSequenceCount == 0)
      {
         /* nCurrentChar contains the current Unicode character */
         /* check character */
         if ((nCurrentChar >= 0xD800 && nCurrentChar < 0xE000) || nCurrentChar >= 0x110000)
         {
            /* Illegal code point */
            LOG_ERROR(pContext, "Invalid UTF-8 code point 0x%X", nCurrentChar);
            goto error;
         }

         if (*pCurrent == CHAR_CR)
         {
            if (pContext->nType == LIB_MANIFEST2_TYPE_COMPILED)
            {
               /* Check if a LF follows */
               pCurrent++;
               if (pCurrent < pEnd && *pCurrent == CHAR_LF)
               {
                  pCurrent++;
               }
               pContext->nLine++;
            }
            goto end;
         }
         else if (*pCurrent == CHAR_LF)
         {
            if (pContext->nType == LIB_MANIFEST2_TYPE_COMPILED)
            {
               pCurrent++;
               pContext->nLine++;
            }
            goto end;
         }
      }
      if (*pCurrent != ' ' && *pCurrent != CHAR_TAB)
      {
         /* It's a non-whitespace char */
         pLastNonWhitespaceChar = pCurrent;
      }
      pCurrent++;
   }

   /* Hit the end of the manifest; Check that we're not in the middle of a sequence */
   if (nCurrentSequenceCount != 0)
   {
      LOG_ERROR(pContext, "File ends in the middle of an UTF-8 sequence");
      goto error;
   }

end:

   *pValueLength = pLastNonWhitespaceChar - *ppValue + 1;
   pContext->nOffset = pCurrent - pContext->pManifestContent;

   return S_SUCCESS;

error:
   *ppName = NULL;
   *pNameLength = 0;
   *ppValue = NULL;
   *pValueLength = 0;
   return nResult;
}

S_RESULT libManifest2GetNextItem(
   LIB_MANIFEST2_CONTEXT* pContext,
   OUT uint8_t** ppName,
   OUT uint32_t* pNameLength,
   OUT uint8_t** ppValue,
   OUT uint32_t* pValueLength)
{
   if (pContext->nType == LIB_MANIFEST2_TYPE_COMPILED)
   {
      /* Don't check for duplicates in binary manifests */
      return static_libManifest2GetNextItemInternal(
         pContext,
         ppName,
         pNameLength,
         ppValue,
         pValueLength);
   }
   else
   {
      uint32_t nOriginalOffset = pContext->nOffset;
      uint32_t nOffset;
      uint32_t nLine;
      uint32_t nSectionStartOffset;
      S_RESULT nResult;
      uint8_t* pDupName;
      uint32_t nDupNameLength;
      uint8_t* pDupValue;
      uint32_t nDupValueLength;

      /* First get the item */
      nResult = static_libManifest2GetNextItemInternal(
         pContext,
         ppName,
         pNameLength,
         ppValue,
         pValueLength);
      if (nResult != S_SUCCESS)
      {
         return nResult;
      }
      /* Save the state of the parser */
      nOffset = pContext->nOffset;
      nLine = pContext->nLine;
      nSectionStartOffset = pContext->nSectionStartOffset;
      if (pContext->nType == LIB_MANIFEST2_TYPE_SOURCE)
      {
         pContext->nOffset = 0;
      }
      else if (*ppValue == NULL)
      {
         /* The item was a section header. Iterate on all section headers and
            check for duplicates */
         pContext->nOffset = 0;
      }
      else
      {
         if (nSectionStartOffset == 0)
         {
            LOG_ERROR(pContext, "Property definition outside any section");
            goto bad_format;
         }
         /* Iterate only on the properties in the section */
         pContext->nOffset = nSectionStartOffset;
      }
      while (pContext->nOffset < nOriginalOffset)
      {
         static_libManifest2GetNextItemInternal(
            pContext,
            &pDupName,
            &nDupNameLength,
            &pDupValue,
            &nDupValueLength);
         if (pContext->nType == LIB_MANIFEST2_TYPE_SOURCE_WITH_SECTIONS && *ppValue == NULL)
         {
            /* Check for duplicate section names */
            if (pDupValue == NULL
                &&
                static_sectionNameEqualCaseInsensitive(
                   *ppName,
                   *pNameLength,
                   pDupName,
                   nDupNameLength))
            {
               pContext->nOffset = nOffset;
               pContext->nLine = nLine;
               pContext->nSectionStartOffset = nSectionStartOffset;
               LOG_ERROR(pContext, "Duplicate section %.*s", nDupNameLength, pDupName);
               goto bad_format;
            }
         }
         else
         {
            /* Check for duplicate property name */
            if (nDupNameLength == *pNameLength &&
                memcmp(pDupName, *ppName, nDupNameLength) == 0)
            {
               /* Duplicated property */
               pContext->nOffset = nOffset;
               pContext->nLine = nLine;
               pContext->nSectionStartOffset = nSectionStartOffset;
               LOG_ERROR(pContext,"Duplicate property %.*s", nDupNameLength, pDupName);
               goto bad_format;
            }
         }
      }
      /* Everything's fine. restore context and exit  */
      /* Restore the context */
      pContext->nOffset = nOffset;
      pContext->nLine = nLine;
      pContext->nSectionStartOffset = nSectionStartOffset;

      return S_SUCCESS;
bad_format:
      *ppName = NULL;
      *pNameLength = 0;
      *ppValue = NULL;
      *pValueLength = 0;
      return S_ERROR_BAD_FORMAT;
   }
}


S_RESULT libManifest2CheckFormat(
   LIB_MANIFEST2_CONTEXT* pContext,
   uint32_t* pnItemCount)
{
   uint32_t nPropertyCount = 0;
   uint8_t* pName;
   uint32_t nNameLength;
   uint8_t* pValue;
   uint32_t nValueLength;
   S_RESULT nResult;

   pContext->nOffset = 0;
   pContext->nLine = 1;
   pContext->nSectionStartOffset = 0;

   while (true)
   {
      nResult = libManifest2GetNextItem(
         pContext,
         &pName,
         &nNameLength,
         &pValue,
         &nValueLength);
      if (nResult == S_ERROR_ITEM_NOT_FOUND)
      {
         if (pnItemCount != NULL)
         {
            *pnItemCount = nPropertyCount;
         }
         return S_SUCCESS;
      }
      if (nResult != S_SUCCESS)
      {
         return nResult;
      }
      nPropertyCount++;
   }
}
