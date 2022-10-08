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
#ifndef PV_M4V_CONFIG_PARSER_H_INCLUDED
#define PV_M4V_CONFIG_PARSER_H_INCLUDED

#include "oscl_base.h"
#include "oscl_types.h"

#define USE_LATER 0  // for some code that will be needed in the future

#define MP4_INVALID_VOL_PARAM -1
#define SHORT_HEADER_MODE -4


#define VISUAL_OBJECT_SEQUENCE_START_CODE   0x01B0
#define VISUAL_OBJECT_SEQUENCE_END_CODE     0x01B1
#define VISUAL_OBJECT_START_CODE   0x01B5
#define VO_START_CODE           0x8
#define VO_HEADER_LENGTH        32
#define VOL_START_CODE 0x12
#define VOL_START_CODE_LENGTH 28

#define GROUP_START_CODE    0x01B3
#define GROUP_START_CODE_LENGTH  32

#define VOP_ID_CODE_LENGTH      5
#define VOP_TEMP_REF_CODE_LENGTH    16

#define USER_DATA_START_CODE        0x01B2
#define USER_DATA_START_CODE_LENGTH 32

#define SHORT_VIDEO_START_MARKER        0x20
#define SHORT_VIDEO_START_MARKER_LENGTH  22

typedef struct
{
    uint8 *data;
    uint32 numBytes;
    uint32 bytePos;
    uint32 bitBuf;
    uint32 dataBitPos;
    uint32  bitPos;
} mp4StreamType;


int16 ShowBits(
    mp4StreamType *pStream,
    uint8 ucNBits,
    uint32 *pulOutData
);

int16 FlushBits(
    mp4StreamType *pStream,
    uint8 ucNBits
);

int16 ReadBits(
    mp4StreamType *pStream,
    uint8 ucNBits,
    uint32 *pulOutData
);



int16 ByteAlign(
    mp4StreamType *pStream
);


OSCL_IMPORT_REF int16 iDecodeVOLHeader(mp4StreamType *psBits, int32 *width, int32 *height, int32 *, int32 *, int32 *profilelevel);
OSCL_IMPORT_REF int16 iGetM4VConfigInfo(uint8 *buffer, int32 length, int32 *width, int32 *height, int32 *, int32 *);
int16 DecodeUserData(mp4StreamType *pStream);
OSCL_IMPORT_REF int16 iDecodeShortHeader(mp4StreamType *psBits, int32 *width, int32 *height, int32 *, int32 *);
OSCL_IMPORT_REF int16 iGetAVCConfigInfo(uint8 *buffer, int32 length, int32 *width, int32 *height, int32 *, int32 *, int32 *profile, int32 *level);

int32 FindNAL(uint8** nal_pnt, uint8* buffer, int32 length);
int16 DecodeSPS(mp4StreamType *psBits, int32 *width, int32 *height, int32 *display_width, int32 *display_height, int32 *profile_idc, int32 *level_idc);
#if USE_LATER
int32 DecodeHRD(mp4StreamType *psBits);
int32 DecodeVUI(mp4StreamType *psBits);
#endif
int32 DecodePPS(mp4StreamType *psBits);

void ue_v(mp4StreamType *psBits, uint32 *codeNum);
void se_v(mp4StreamType *psBits, int32 *value);
void Parser_EBSPtoRBSP(uint8 *nal_unit, int32 *size);

#endif //PV_M4V_CONFIG_PARSER_H_INCLUDED


