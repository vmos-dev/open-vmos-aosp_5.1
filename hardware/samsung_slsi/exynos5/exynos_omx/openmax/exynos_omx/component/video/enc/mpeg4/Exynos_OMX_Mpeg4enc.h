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
 * @file        Exynos_OMX_Mpeg4enc.h
 * @brief
 * @author      Yunji Kim (yunji.kim@samsung.com)
 * @version     2.0.0
 * @history
 *   2012.02.20 : Create
 */

#ifndef EXYNOS_OMX_MPEG4_ENC_COMPONENT
#define EXYNOS_OMX_MPEG4_ENC_COMPONENT

#include "Exynos_OMX_Def.h"
#include "OMX_Component.h"
#include "OMX_Video.h"

typedef enum _CODEC_TYPE
{
    CODEC_TYPE_H263,
    CODEC_TYPE_MPEG4
} CODEC_TYPE;

typedef struct _EXYNOS_MFC_MPEG4ENC_HANDLE
{
    OMX_HANDLETYPE             hMFCHandle;

    OMX_U32                    indexTimestamp;
    OMX_U32 outputIndexTimestamp;
    OMX_BOOL bConfiguredMFCSrc;
    OMX_BOOL bConfiguredMFCDst;
    CODEC_TYPE                 codecType;

    ExynosVideoDecOps *pEncOps;
    ExynosVideoDecBufferOps *pInbufOps;
    ExynosVideoDecBufferOps *pOutbufOps;
    ExynosVideoEncParam      encParam;
} EXYNOS_MFC_MPEG4ENC_HANDLE;

typedef struct _EXYNOS_MPEG4ENC_HANDLE
{
    /* OMX Codec specific */
    OMX_VIDEO_PARAM_H263TYPE  h263Component[ALL_PORT_NUM];
    OMX_VIDEO_PARAM_MPEG4TYPE mpeg4Component[ALL_PORT_NUM];
    OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE errorCorrectionType[ALL_PORT_NUM];

    /* SEC MFC Codec specific */
    EXYNOS_MFC_MPEG4ENC_HANDLE   hMFCMpeg4Handle;

    OMX_BOOL bSourceStart;
    OMX_BOOL bDestinationStart;
    OMX_HANDLETYPE hSourceStartEvent;
    OMX_HANDLETYPE hDestinationStartEvent;
} EXYNOS_MPEG4ENC_HANDLE;


#ifdef __cplusplus
extern "C" {
#endif

OSCL_EXPORT_REF OMX_ERRORTYPE Exynos_OMX_ComponentInit(OMX_HANDLETYPE hComponent, OMX_STRING componentName);
                OMX_ERRORTYPE Exynos_OMX_ComponentDeinit(OMX_HANDLETYPE hComponent);

#ifdef __cplusplus
};
#endif

#endif
