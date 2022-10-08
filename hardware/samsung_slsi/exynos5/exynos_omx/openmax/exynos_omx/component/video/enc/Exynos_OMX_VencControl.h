/*
 *
 * Copyright 2012 Samsung Electronics S.LSI Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * @file        Exynos_OMX_VencControl.h
 * @brief
 * @author      SeungBeom Kim (sbcrux.kim@samsung.com)
 * @version     2.0.0
 * @history
 *   2012.02.20 : Create
 */

#ifndef EXYNOS_OMX_VIDEO_ENCODECONTROL
#define EXYNOS_OMX_VIDEO_ENCODECONTROL

#include "OMX_Component.h"
#include "Exynos_OMX_Def.h"
#include "Exynos_OSAL_Queue.h"
#include "Exynos_OMX_Baseport.h"
#include "Exynos_OMX_Basecomponent.h"


#ifdef __cplusplus
extern "C" {
#endif

OMX_ERRORTYPE Exynos_OMX_UseBuffer(
    OMX_IN OMX_HANDLETYPE            hComponent,
    OMX_INOUT OMX_BUFFERHEADERTYPE **ppBufferHdr,
    OMX_IN OMX_U32                   nPortIndex,
    OMX_IN OMX_PTR                   pAppPrivate,
    OMX_IN OMX_U32                   nSizeBytes,
    OMX_IN OMX_U8                   *pBuffer);
OMX_ERRORTYPE Exynos_OMX_AllocateBuffer(
    OMX_IN OMX_HANDLETYPE            hComponent,
    OMX_INOUT OMX_BUFFERHEADERTYPE **ppBuffer,
    OMX_IN OMX_U32                   nPortIndex,
    OMX_IN OMX_PTR                   pAppPrivate,
    OMX_IN OMX_U32                   nSizeBytes);
OMX_ERRORTYPE Exynos_OMX_FreeBuffer(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_U32        nPortIndex,
    OMX_IN OMX_BUFFERHEADERTYPE *pBufferHdr);
OMX_ERRORTYPE Exynos_OMX_AllocateTunnelBuffer(
    EXYNOS_OMX_BASEPORT *pOMXBasePort,
    OMX_U32           nPortIndex);
OMX_ERRORTYPE Exynos_OMX_FreeTunnelBuffer(
    EXYNOS_OMX_BASEPORT *pOMXBasePort,
    OMX_U32           nPortIndex);
OMX_ERRORTYPE Exynos_OMX_ComponentTunnelRequest(
    OMX_IN  OMX_HANDLETYPE hComp,
    OMX_IN OMX_U32         nPort,
    OMX_IN OMX_HANDLETYPE  hTunneledComp,
    OMX_IN OMX_U32         nTunneledPort,
    OMX_INOUT OMX_TUNNELSETUPTYPE *pTunnelSetup);
OMX_ERRORTYPE Exynos_OMX_VideoEncodeGetParameter(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nParamIndex,
    OMX_INOUT OMX_PTR     ComponentParameterStructure);
OMX_ERRORTYPE Exynos_OMX_VideoEncodeSetParameter(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nIndex,
    OMX_IN OMX_PTR        ComponentParameterStructure);
OMX_ERRORTYPE Exynos_OMX_VideoEncodeGetConfig(
    OMX_HANDLETYPE hComponent,
    OMX_INDEXTYPE nIndex,
    OMX_PTR pComponentConfigStructure);
OMX_ERRORTYPE Exynos_OMX_VideoEncodeSetConfig(
    OMX_HANDLETYPE hComponent,
    OMX_INDEXTYPE nIndex,
    OMX_PTR pComponentConfigStructure);
OMX_ERRORTYPE Exynos_OMX_VideoEncodeGetExtensionIndex(
    OMX_IN OMX_HANDLETYPE  hComponent,
    OMX_IN OMX_STRING      cParameterName,
    OMX_OUT OMX_INDEXTYPE *pIndexType);
OMX_ERRORTYPE Exynos_InputBufferReturn(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATABUFFER *dataBuffer);
OMX_ERRORTYPE Exynos_OutputBufferReturn(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATABUFFER *dataBuffer);
OMX_ERRORTYPE Exynos_OMX_BufferFlush(OMX_COMPONENTTYPE *pOMXComponent, OMX_S32 nPortIndex, OMX_BOOL bEvent);
#ifdef __cplusplus
}
#endif

#endif

