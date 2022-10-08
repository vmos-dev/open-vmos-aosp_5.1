/*--------------------------------------------------------------------------
Copyright (c) 2010-2013, The Linux Foundation. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of The Linux Foundation nor
      the names of its contributors may be used to endorse or promote
      products derived from this software without specific prior written
      permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
--------------------------------------------------------------------------*/

#ifndef __DIVXDRMDECRYPT_H__
#define __DIVXDRMDECRYPT_H__

#include <OMX_Core.h>

//Abstract base class of API to decrypt DRM content.
class DivXDrmDecrypt
{
    public:
        static DivXDrmDecrypt* Create();
        virtual OMX_ERRORTYPE Init() = 0;
        virtual OMX_ERRORTYPE Decrypt(OMX_BUFFERHEADERTYPE* buffer) = 0;
        inline virtual ~DivXDrmDecrypt() {}
};

//.so file should provide a function with the name createDivXDrmDecrypt with
//prototype of DivXDrmDecryptFactory.
static const char* MEDIA_CREATE_DIVX_DRM_DECRYPT = "createDivXDrmDecrypt";
typedef DivXDrmDecrypt* (*DivXDrmDecryptFactory)();

#endif //__DIVXDRMDECRYPT_H__
