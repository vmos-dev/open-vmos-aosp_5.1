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
 * @file        Exynos_OMX_Mp3dec.h
 * @brief
 * @author      Yunji Kim (yunji.kim@samsung.com)
 * @version     1.1.0
 * @history
 *   2012.02.28 : Create
 */

#ifndef EXYNOS_OMX_MP3_DEC_COMPONENT
#define EXYNOS_OMX_MP3_DEC_COMPONENT

#include "Exynos_OMX_Def.h"
#include "OMX_Component.h"

typedef struct _EXYNOS_SRP_MP3_HANDLE
{
    OMX_HANDLETYPE hSRPHandle;
    OMX_BOOL       bConfiguredSRP;
    OMX_BOOL       bSRPLoaded;
    OMX_BOOL       bSRPSendEOS;
    OMX_S32        returnCodec;
} EXYNOS_SRP_MP3_HANDLE;

typedef struct _EXYNOS_MP3_HANDLE
{
    /* OMX Codec specific */
    OMX_AUDIO_PARAM_MP3TYPE     mp3Param;
    OMX_AUDIO_PARAM_PCMMODETYPE pcmParam;

    /* SEC SRP Codec specific */
    EXYNOS_SRP_MP3_HANDLE      hSRPMp3Handle;
} EXYNOS_MP3_HANDLE;

#ifdef __cplusplus
extern "C" {
#endif

OSCL_EXPORT_REF OMX_ERRORTYPE Exynos_OMX_ComponentInit(OMX_HANDLETYPE hComponent, OMX_STRING componentName);
                OMX_ERRORTYPE Exynos_OMX_ComponentDeinit(OMX_HANDLETYPE hComponent);

#ifdef __cplusplus
};
#endif

#endif /* EXYNOS_OMX_MP3_DEC_COMPONENT */
