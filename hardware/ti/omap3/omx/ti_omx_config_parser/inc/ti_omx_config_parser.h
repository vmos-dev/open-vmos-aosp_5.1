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
#ifndef TI_OMX_CONFIG_PARSER_H_INCLUDED
#define TI_OMX_CONFIG_PARSER_H_INCLUDED

#ifdef __cplusplus
extern "C"
{
    OSCL_IMPORT_REF OMX_BOOL TIOMXConfigParser(
        OMX_PTR aInputParameters,
        OMX_PTR aOutputParameters);

}
#endif


typedef struct
{
    OMX_U8* inPtr;             //pointer to codec configuration header
    OMX_U32 inBytes;           //length of codec configuration header
    OMX_STRING cComponentRole; //OMX component codec type
    OMX_STRING cComponentName;  //OMX component name
} OMXConfigParserInputs;

typedef struct
{
    OMX_U16 Channels;
    OMX_U16 BitsPerSample;
    OMX_U32 SamplesPerSec;
} AudioOMXConfigParserOutputs;

typedef struct
{
    OMX_U32 width;
    OMX_U32 height;
    OMX_U32 profile;
    OMX_U32 level;
} VideoOMXConfigParserOutputs;

#endif

