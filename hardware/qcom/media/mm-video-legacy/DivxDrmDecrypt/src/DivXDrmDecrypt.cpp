/*--------------------------------------------------------------------------
Copyright (c) 2010-2012, Code Aurora Forum. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of Code Aurora nor
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

#include "DivXDrmDecrypt.h"
#include <dlfcn.h>  // for dlopen/dlclose

//#define LOG_NDEBUG 0
#define LOG_TAG "DivXDrmDecrypt"
#ifdef _ANDROID_
#include <utils/Log.h>
#else
#include <stdio.h>
#define ALOGE(fmt, args...) fprintf(stderr, fmt, ##args)
#endif /* _ANDROID_ */

static const char* DIVX_DRM_SHIM_LIB = "libSHIMDivxDrm.so";

void* getDecryptHandle() {
    static void* decryptLib = NULL;
    static bool  decryptLibOpened = false;

    if(decryptLibOpened) {
        return decryptLib;
    }

    decryptLib = ::dlopen(DIVX_DRM_SHIM_LIB, RTLD_NOW);
    decryptLibOpened = true;

    if (decryptLib == NULL) {
        ALOGE("Failed to open DIVX_DRM_SHIM_LIB \n");
    }

    return decryptLib;
}

DivXDrmDecryptFactory DrmDecryptFactoryFunction() {
    static DivXDrmDecryptFactory drmDecryptFactoryFunction = NULL;
    static bool alreadyTriedToFindFactoryFunction = false;

    if(alreadyTriedToFindFactoryFunction) {
        return drmDecryptFactoryFunction;
    }

    void *pDecryptLib = getDecryptHandle();
    if (pDecryptLib == NULL) {
        return NULL;
    }

    drmDecryptFactoryFunction = (DivXDrmDecryptFactory) dlsym(pDecryptLib, MEDIA_CREATE_DIVX_DRM_DECRYPT);
    alreadyTriedToFindFactoryFunction = true;

    if(!drmDecryptFactoryFunction) {
        ALOGE(" dlsym for DrmDecrypt factory function failed \n");
    }

    return drmDecryptFactoryFunction;
}



DivXDrmDecrypt* DivXDrmDecrypt::Create() {
    DivXDrmDecryptFactory drmCreateFunc = DrmDecryptFactoryFunction();
    if( drmCreateFunc == NULL ) {
        return NULL;
    }

    DivXDrmDecrypt* decrypt = drmCreateFunc();
    if( decrypt == NULL ) {
        ALOGE(" failed to instantiate DrmDecoder \n");
    }
    return decrypt;
}

