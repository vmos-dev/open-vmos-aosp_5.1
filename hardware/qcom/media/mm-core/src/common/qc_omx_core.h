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

 This module contains the definitions of the OpenMAX core.

*//*========================================================================*/

#ifndef QC_OMX_CORE_H
#define QC_OMX_CORE_H

#include "qc_omx_common.h"        // OMX API
#include <string.h>

#define OMX_COMP_MAX_INST 16

typedef struct _omx_core_cb_type
{
  char*                         name;// Component name
  create_qc_omx_component     fn_ptr;// create instance fn ptr
  void*                         inst[OMX_COMP_MAX_INST];// Instance handle
  void*                so_lib_handle;// So Library handle
  char*                  so_lib_name;// so directory
  char* roles[OMX_CORE_MAX_CMP_ROLES];// roles played
}omx_core_cb_type;

typedef struct
{
    OMX_U32 width;
    OMX_U32 height;
    OMX_U32 profile;
    OMX_U32 level;
} VideoOMXConfigParserOutputs;


typedef struct
{
    OMX_U8* inPtr;             //pointer to codec configuration header
    OMX_U32 inBytes;           //length of codec configuration header
    OMX_STRING cComponentRole; //OMX component codec type
    OMX_STRING cComponentName;  //OMX component name
} OMXConfigParserInputs;

#endif

