/*--------------------------------------------------------------------------
Copyright (c) 2009, The Linux Foundation. All rights reserved.

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
/*============================================================================
                            O p e n M A X   w r a p p e r s
                             O p e n  M A X   C o r e

*//** @file qc_omx_common.h
  This module contains the definitions of the OpenMAX core.

*//*========================================================================*/

#ifndef QC_OMX_COMMON_H
#define QC_OMX_COMMON_H


#include <stdio.h>           // Standard IO
#include "OMX_Core.h"        // OMX API
#include "OMX_QCOMExtns.h"   // OMX API

#define OMX_CORE_MAX_CMP                1 // MAX Components supported
#define OMX_CORE_MAX_CMP_ROLES          1 // MAX Roles per component
#define OMX_SPEC_VERSION       0x00000101 // OMX Version

#ifdef __cplusplus
extern "C" {
#endif

typedef void * (*create_qc_omx_component)(void);

#ifdef _ANDROID_
#define LOG_TAG "QC_CORE"
#endif
#include "qc_omx_msg.h"

#ifdef __cplusplus
}
#endif

#endif

