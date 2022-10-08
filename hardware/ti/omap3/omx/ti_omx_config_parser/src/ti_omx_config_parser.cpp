/* ------------------------------------------------------------------
 * Copyright (C) 1998-2009 PacketVideo
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 * -------------------------------------------------------------------
 */

#define __USE_MISC
#include "oscl_stdstring.h"

// Use default DLL entry point
#ifndef OSCL_DLL_H_INCLUDED
#include "oscl_dll.h"
#endif

#ifndef PV_OMXDEFS_H_INCLUDED
#include "pv_omxdefs.h"
#endif

#ifndef PV_OMX_QUEUE_H_INCLUDED
#include "pv_omx_queue.h"
#endif

#ifndef OMX_Types_h
#include "omx_types.h"
#endif

#ifndef OSCL_BASE_INCLUDED_H
#include "oscl_base.h"
#endif

#ifndef OMX_Core_h
#include "omx_core.h"
#endif

#ifndef PV_AUDIO_CONFIG_PARSER_H
#include "pv_audio_config_parser.h"
#endif

#ifndef PV_VIDEO_CONFIG_PARSER_H
#include "pv_video_config_parser.h"
#endif

#include "ti_omx_config_parser.h"
#include "ti_video_config_parser.h"

OSCL_EXPORT_REF OMX_BOOL TIOMXConfigParser(
    OMX_PTR aInputParameters,
    OMX_PTR aOutputParameters)

{
    OMXConfigParserInputs* pInputs;
    pInputs = (OMXConfigParserInputs*) aInputParameters;

    if (NULL != pInputs->cComponentRole)
    {
        if (0 == oscl_strncmp(pInputs->cComponentRole, (OMX_STRING)"audio_decoder", oscl_strlen("audio_decoder")))
        {
            OMX_S32 Status;
            pvAudioConfigParserInputs aInputs;

            aInputs.inPtr = pInputs->inPtr;
            aInputs.inBytes = pInputs->inBytes;

            if (0 == oscl_strcmp(pInputs->cComponentRole, (OMX_STRING)"audio_decoder.wma"))
            {
                aInputs.iMimeType = PVMF_MIME_WMA;

            }
            else if (0 == oscl_strcmp(pInputs->cComponentRole, (OMX_STRING)"audio_decoder.aac"))
            {
                aInputs.iMimeType = PVMF_MIME_AAC_SIZEHDR;

            }
            else if (0 == oscl_strcmp(pInputs->cComponentRole, (OMX_STRING)"audio_decoder.amr"))
            {
                aInputs.iMimeType = PVMF_MIME_AMR;

            }
            else if (0 == oscl_strcmp(pInputs->cComponentRole, (OMX_STRING)"audio_decoder.amrnb"))
            {
                aInputs.iMimeType = PVMF_MIME_AMR;

            }
            else if (0 == oscl_strcmp(pInputs->cComponentRole, (OMX_STRING)"audio_decoder.amrwb"))
            {
                aInputs.iMimeType = PVMF_MIME_AMRWB;

            }
            else if (0 == oscl_strcmp(pInputs->cComponentRole, (OMX_STRING)"audio_decoder.mp3"))
            {
                aInputs.iMimeType = PVMF_MIME_MP3;

            }
            else
            {
                return OMX_FALSE;
            }

            Status = pv_audio_config_parser(&aInputs, (pvAudioConfigParserOutputs *)aOutputParameters);
            if (0 == Status)
            {
                return OMX_FALSE;
            }
        }
        else if (0 == oscl_strncmp(pInputs->cComponentRole, (OMX_STRING)"video_decoder", oscl_strlen("video_decoder")))
        {

            OMX_S32 Status;
            pvVideoConfigParserInputs aInputs;

            aInputs.inPtr = pInputs->inPtr;
            aInputs.inBytes = pInputs->inBytes;

            if (0 == oscl_strcmp(pInputs->cComponentRole, (OMX_STRING)"video_decoder.wmv"))
            {
                aInputs.iMimeType = PVMF_MIME_WMV;

            }
            else if (0 == oscl_strcmp(pInputs->cComponentRole, (OMX_STRING)"video_decoder.avc"))
            {
                aInputs.iMimeType = PVMF_MIME_H264_VIDEO;

            }
            else if (0 == oscl_strcmp(pInputs->cComponentRole, (OMX_STRING)"video_decoder.mpeg4"))
            {
                aInputs.iMimeType = PVMF_MIME_M4V;

            }
            else if (0 == oscl_strcmp(pInputs->cComponentRole, (OMX_STRING)"video_decoder.h263"))
            {
                aInputs.iMimeType = PVMF_MIME_H2632000;

            }
            else
            {
                return OMX_FALSE;
            }
            
            if ((aInputs.iMimeType == PVMF_MIME_M4V) || (aInputs.iMimeType == PVMF_MIME_H264_VIDEO))
            {
                Status = ti_video_config_parser((tiVideoConfigParserInputs *)&aInputs, (tiVideoConfigParserOutputs *)aOutputParameters, pInputs->cComponentName);
            }
            else
            {
                Status = pv_video_config_parser(&aInputs, (pvVideoConfigParserOutputs *)aOutputParameters);
            }
            if (0 != Status)
            {
                return OMX_FALSE;
            }
        }
        else
        {
            return OMX_FALSE;
        }

    }
    else
    {
        return OMX_FALSE;
    }

    return OMX_TRUE;
}

