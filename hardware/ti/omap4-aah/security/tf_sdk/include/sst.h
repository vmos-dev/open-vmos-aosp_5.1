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

#ifndef __SST_H__
#define __SST_H__

#ifdef __cplusplus
extern "C" {
#endif


#include "s_type.h"
#include "s_error.h"

#ifdef SST_EXPORTS
#define SST_EXPORT_API S_DLL_EXPORT
#else
#define SST_EXPORT_API S_DLL_IMPORT
#endif

/* -----------------------------------------------------------------------------
                          Service SST Types
----------------------------------------------------------------------------- */

#ifndef EXCLUDE_SERVICE_SST_TYPES

/** The SST_ERROR type */
typedef uint32_t  SST_ERROR;

/** The SST_HANDLE type */
typedef uint32_t  SST_HANDLE;

typedef struct SST_FILE_INFO
{
   char* pName;
   uint32_t nSize;
} SST_FILE_INFO;

#endif /* EXCLUDE_SERVICE_SST_TYPES */

typedef enum
{
   SST_SEEK_SET = 0,
   SST_SEEK_CUR,
   SST_SEEK_END
} SST_WHENCE;


/* -----------------------------------------------------------------------------
                          Constants
----------------------------------------------------------------------------- */

#ifndef EXCLUDE_SERVICE_SST_CONSTANTS

/** The Invalid SST_FILE_HANDLE */
#define SST_NULL_HANDLE     0

/* Legacy constant name */
#define SST_HANDLE_INVALID    SST_NULL_HANDLE

/** Max length for file names */
#define SST_MAX_FILENAME 0x40

/** Maximum pointer position in a file */
#define SST_MAX_FILE_POSITION    0xFFFFFFFF

/** File access modes */
#define SST_O_READ               0x00000001
#define SST_O_WRITE              0x00000002
#define SST_O_WRITE_META         0x00000004
#define SST_O_SHARE_READ         0x00000010
#define SST_O_SHARE_WRITE        0x00000020
#define SST_O_CREATE             0x00000200
#define SST_O_EXCLUSIVE          0x00000400


#endif /* EXCLUDE_SERVICE_SST_CONSTANTS */


/* -----------------------------------------------------------------------------
                        Functions
----------------------------------------------------------------------------- */

#ifndef EXCLUDE_SERVICE_SST_FUNCTIONS

SST_ERROR SST_EXPORT_API SSTInit(void);

SST_ERROR SST_EXPORT_API SSTTerminate(void);

SST_ERROR SST_EXPORT_API SSTOpen(const char*       pFilename,
                                 uint32_t    nFlags,
                                 uint32_t    nReserved,
                                 SST_HANDLE* phFile);

SST_ERROR SST_EXPORT_API SSTCloseHandle(SST_HANDLE  hFile);

SST_ERROR SST_EXPORT_API SSTWrite(SST_HANDLE hFile,
                                  const uint8_t*   pBuffer,
                                  uint32_t   nSize);

SST_ERROR SST_EXPORT_API SSTRead(SST_HANDLE  hFile,
                                 uint8_t*    pBuffer,
                                 uint32_t    nSize,
                                 uint32_t*   pnCount);

SST_ERROR SST_EXPORT_API SSTSeek(SST_HANDLE  hFile,
                                 int32_t     nOffset,
                                 SST_WHENCE  eWhence);

SST_ERROR SST_EXPORT_API SSTTell(SST_HANDLE  hFile,
                                 uint32_t*   pnPos);

SST_ERROR SST_EXPORT_API SSTGetSize(const char*        pFilename,
                                    uint32_t*    pnSize);

SST_ERROR SST_EXPORT_API SSTEof( SST_HANDLE   hFile,
                                 bool*        pbEof);

SST_ERROR SST_EXPORT_API SSTCloseAndDelete(SST_HANDLE hFile);

SST_ERROR SST_EXPORT_API SSTTruncate(  SST_HANDLE  hFile,
                                       uint32_t    nSize);

SST_ERROR SST_EXPORT_API SSTRename(SST_HANDLE hFile,
                                   const char* pNewFilename);

SST_ERROR SST_EXPORT_API SSTEnumerationStart(const char*     pFilenamePattern,
                                             uint32_t  nReserved1,
                                             uint32_t  nReserved2,
                                             SST_HANDLE* phFileEnumeration);

SST_ERROR SST_EXPORT_API SSTEnumerationCloseHandle(SST_HANDLE hFileEnumeration);

SST_ERROR SST_EXPORT_API SSTEnumerationGetNext(SST_HANDLE      hFileEnumeration,
                                               SST_FILE_INFO** ppFileInfo);

SST_ERROR SST_EXPORT_API SSTDestroyFileInfo(SST_FILE_INFO*   pFileInfo);

#endif /* EXCLUDE_SERVICE_SST_FUNCTIONS */

#ifdef __cplusplus
}
#endif

#endif /* __SST_H__ */
