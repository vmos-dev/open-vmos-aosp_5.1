/**
 * Copyright(c) 2012 Trusted Logic.   All rights reserved.
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

#ifndef __LIB_UUID_H__
#define __LIB_UUID_H__


#include "s_type.h"


#ifdef __cplusplus
extern "C" {
#endif
#if 0
}  /* balance curly quotes */
#endif

/**
 * LIB_UUID is deprecated use S_UUID instead.
 * @deprecated
 */
typedef S_UUID LIB_UUID;
/**
 * LIB_UUID_STRING_SIZE is deprecated use UUID_STRING_SIZE instead.
 * @deprecated
 */
#define LIB_UUID_STRING_SIZE  36

/**
 * Defines the UUID string size in characters
 *
 * E.g. "f81d4fae-7dec-11d0-a765-00a0c91e6bf6"
 **/
#define UUID_STRING_SIZE  36

/**
 * Converts the string representation of an UUID to the binary representation as
 * a S_UUID type. The binary UUID structure must be provided by the caller.
 *
 * @param   pIdentifierString  The UTF-8 representation of the identifier. This
 *          string does not need to be zero terminated. The decoder reads only
 *          the {UUID_STRING_SIZE} first bytes.
 *
 * @param   pIdentifier  The identifer structure receiving the binary value of
 *          the identifier.
 *
 * @return  TRUE in case of success, FALSE if the string does not conform to the
 *          syntax of UUID as defined in RFC 4122
 *          (http://www.ietf.org/rfc/rfc4122.txt)
 **/
bool libUUIDFromString(
      IN  const uint8_t* pIdentifierString,
      OUT S_UUID* pIdentifier);

/**
 * Converts the binary representation of an UUID to the string representation.
 *
 * @param   pIdentifier  The identifer structure with the binary value of the
 *          identifier.
 *
 * @param   pIdentifierString  The buffer receiving the UTF-8 representation of
 *          the identifier. This string is not zero terminated. The encoder
 *          writes only the first {UUID_STRING_SIZE} bytes.
 *
 **/
void libUUIDToString(
      IN  const S_UUID* pIdentifier,
      OUT uint8_t* pIdentifierString);

/**
 * Generates an UUID from the specified MD5 hash value, as specified in section
 * 4.3, Algorithm for Creating a Name-Based UUID, of RFC 4122.
 *
 * This function assumes that the hash value is 128-bit long.
 *
 * @param pHashData A pointer to the first byte of the MD5 hash data. Only the
 * first 16 bytes of this hash data will be used to generate the UUID.
 *
 * @param pIdentifier A pointer to the placeholder receiving the generated
 * identifier.
 **/
void libUUIDFromMD5Hash(
      IN  const uint8_t* pHashData,
      OUT S_UUID* pIdentifier);

/**
 * Generates an UUID from the specified SHA-1 hash value, as specified in
 * section 4.3, Algorithm for Creating a Name-Based UUID, of RFC 4122.
 *
 * This function assumes that the hash value is 128-bit long.
 *
 * @param pHashData A pointer to the first byte of the SHA-1 hash data. Only the
 * first 16 bytes of this hash data will be used to generate the UUID.
 *
 * @param pIdentifier A pointer to the placeholder receiving the generated
 * identifier.
 **/
void libUUIDFromSHA1Hash(
      IN  const uint8_t* pHashData,
      OUT S_UUID* pIdentifier);

/**
 * Checks if an identifier is the nil identifier as specified in RFC 4122.
 *
 * @param   pIdentifier  The identifier to check.
 *
 * @return  TRUE if the identifier is the nil identifier, FALSE otherwise.
 **/
bool libUUIDIsNil(
      IN  const S_UUID* pIdentifier);

/**
 * Sets an identifier to the nil value as specified in RFC 4122.
 *
 * @param   pIdentifier  The identifier to set to nil.
 **/
void libUUIDSetToNil(
      OUT S_UUID* pIdentifier);

#if 0
{  /* balance curly quotes */
#endif
#ifdef __cplusplus
}  /* closes extern "C" */
#endif


#endif  /* !defined(__LIB_UUID_H__) */
