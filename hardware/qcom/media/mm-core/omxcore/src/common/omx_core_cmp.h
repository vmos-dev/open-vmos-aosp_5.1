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

 OpenMAX Core Macros interface.

============================================================================*/

//////////////////////////////////////////////////////////////////////////////
//                             Include Files
//////////////////////////////////////////////////////////////////////////////
#ifndef OMX_CORE_CMP_H
#define OMX_CORE_CMP_H



#ifdef __cplusplus
extern "C" {
#endif


void * qc_omx_create_component_wrapper(OMX_PTR obj_ptr);


OMX_ERRORTYPE
qc_omx_component_init(OMX_IN OMX_HANDLETYPE hComp, OMX_IN OMX_STRING componentName);


OMX_ERRORTYPE
qc_omx_component_get_version(OMX_IN OMX_HANDLETYPE               hComp,
                             OMX_OUT OMX_STRING          componentName,
                             OMX_OUT OMX_VERSIONTYPE* componentVersion,
                             OMX_OUT OMX_VERSIONTYPE*      specVersion,
                             OMX_OUT OMX_UUIDTYPE*       componentUUID);

OMX_ERRORTYPE
qc_omx_component_send_command(OMX_IN OMX_HANDLETYPE hComp,
                              OMX_IN OMX_COMMANDTYPE  cmd,
                              OMX_IN OMX_U32       param1,
                              OMX_IN OMX_PTR      cmdData);

OMX_ERRORTYPE
qc_omx_component_get_parameter(OMX_IN OMX_HANDLETYPE     hComp,
                               OMX_IN OMX_INDEXTYPE paramIndex,
                               OMX_INOUT OMX_PTR     paramData);

OMX_ERRORTYPE
qc_omx_component_set_parameter(OMX_IN OMX_HANDLETYPE     hComp,
                               OMX_IN OMX_INDEXTYPE paramIndex,
                               OMX_IN OMX_PTR        paramData);

OMX_ERRORTYPE
qc_omx_component_get_config(OMX_IN OMX_HANDLETYPE      hComp,
          OMX_IN OMX_INDEXTYPE configIndex,
          OMX_INOUT OMX_PTR     configData);

OMX_ERRORTYPE
qc_omx_component_set_config(OMX_IN OMX_HANDLETYPE      hComp,
                            OMX_IN OMX_INDEXTYPE configIndex,
                            OMX_IN OMX_PTR        configData);

OMX_ERRORTYPE
qc_omx_component_get_extension_index(OMX_IN OMX_HANDLETYPE      hComp,
                                     OMX_IN OMX_STRING      paramName,
                                     OMX_OUT OMX_INDEXTYPE* indexType);

OMX_ERRORTYPE
qc_omx_component_get_state(OMX_IN OMX_HANDLETYPE  hComp,
                           OMX_OUT OMX_STATETYPE* state);

OMX_ERRORTYPE
qc_omx_component_tunnel_request(OMX_IN OMX_HANDLETYPE                hComp,
                                OMX_IN OMX_U32                        port,
                                OMX_IN OMX_HANDLETYPE        peerComponent,
                                OMX_IN OMX_U32                    peerPort,
                                OMX_INOUT OMX_TUNNELSETUPTYPE* tunnelSetup);

OMX_ERRORTYPE
qc_omx_component_use_buffer(OMX_IN OMX_HANDLETYPE                hComp,
                            OMX_INOUT OMX_BUFFERHEADERTYPE** bufferHdr,
                            OMX_IN OMX_U32                        port,
                            OMX_IN OMX_PTR                     appData,
                            OMX_IN OMX_U32                       bytes,
                            OMX_IN OMX_U8*                      buffer);


// qc_omx_component_allocate_buffer  -- API Call
OMX_ERRORTYPE
qc_omx_component_allocate_buffer(OMX_IN OMX_HANDLETYPE                hComp,
                                 OMX_INOUT OMX_BUFFERHEADERTYPE** bufferHdr,
                                 OMX_IN OMX_U32                        port,
                                 OMX_IN OMX_PTR                     appData,
                                 OMX_IN OMX_U32                       bytes);

OMX_ERRORTYPE
qc_omx_component_free_buffer(OMX_IN OMX_HANDLETYPE         hComp,
                             OMX_IN OMX_U32                 port,
                             OMX_IN OMX_BUFFERHEADERTYPE* buffer);

OMX_ERRORTYPE
qc_omx_component_empty_this_buffer(OMX_IN OMX_HANDLETYPE         hComp,
                                   OMX_IN OMX_BUFFERHEADERTYPE* buffer);

OMX_ERRORTYPE
qc_omx_component_fill_this_buffer(OMX_IN OMX_HANDLETYPE         hComp,
                                  OMX_IN OMX_BUFFERHEADERTYPE* buffer);

OMX_ERRORTYPE
qc_omx_component_set_callbacks(OMX_IN OMX_HANDLETYPE        hComp,
                               OMX_IN OMX_CALLBACKTYPE* callbacks,
                               OMX_IN OMX_PTR             appData);

OMX_ERRORTYPE
qc_omx_component_deinit(OMX_IN OMX_HANDLETYPE hComp);

OMX_ERRORTYPE
qc_omx_component_use_EGL_image(OMX_IN OMX_HANDLETYPE                hComp,
                               OMX_INOUT OMX_BUFFERHEADERTYPE** bufferHdr,
                               OMX_IN OMX_U32                        port,
                               OMX_IN OMX_PTR                     appData,
                               OMX_IN void*                      eglImage);

OMX_ERRORTYPE
qc_omx_component_role_enum(OMX_IN OMX_HANDLETYPE hComp,
                           OMX_OUT OMX_U8*        role,
                           OMX_IN OMX_U32        index);

#ifdef __cplusplus
}
#endif

#endif

