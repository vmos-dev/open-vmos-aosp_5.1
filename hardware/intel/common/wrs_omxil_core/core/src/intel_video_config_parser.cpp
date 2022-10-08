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

#include "intel_video_config_parser.h"
#include "intel_m4v_config_parser.h"
#include "oscl_mem.h"

#include "oscl_dll.h"

#define GetUnalignedWord( pb, w ) \
            (w) = ((uint16) *(pb + 1) << 8) + *pb;

#define GetUnalignedDword( pb, dw ) \
            (dw) = ((uint32) *(pb + 3) << 24) + \
                   ((uint32) *(pb + 2) << 16) + \
                   ((uint16) *(pb + 1) << 8) + *pb;

#define GetUnalignedWordEx( pb, w )     GetUnalignedWord( pb, w ); (pb) += sizeof(uint16);
#define GetUnalignedDwordEx( pb, dw )   GetUnalignedDword( pb, dw ); (pb) += sizeof(uint32);
#define GetUnalignedQwordEx( pb, qw )   GetUnalignedQword( pb, qw ); (pb) += sizeof(uint64);

#define LoadBYTE( b, p )    b = *(uint8 *)p;  p += sizeof( uint8 )

#define LoadWORD( w, p )    GetUnalignedWordEx( p, w )
#define LoadDWORD( dw, p )  GetUnalignedDwordEx( p, dw )
#define LoadQWORD( qw, p )  GetUnalignedQwordEx( p, qw )

#ifndef MAKEFOURCC_WMC
#define MAKEFOURCC_WMC(ch0, ch1, ch2, ch3) \
        ((uint32)(uint8)(ch0) | ((uint32)(uint8)(ch1) << 8) |   \
        ((uint32)(uint8)(ch2) << 16) | ((uint32)(uint8)(ch3) << 24 ))

#define mmioFOURCC_WMC(ch0, ch1, ch2, ch3)  MAKEFOURCC_WMC(ch0, ch1, ch2, ch3)
#endif

#define FOURCC_WMV2     mmioFOURCC_WMC('W','M','V','2')
#define FOURCC_WMV3     mmioFOURCC_WMC('W','M','V','3')
#define FOURCC_WMVA     mmioFOURCC_WMC('W','M','V','A')
#define FOURCC_WVC1     mmioFOURCC_WMC('W','V','C','1')
//For WMVA
#define ASFBINDING_SIZE                   1   // size of ASFBINDING is 1 byte
#define FOURCC_MP42     mmioFOURCC_WMC('M','P','4','2')
#define FOURCC_MP43     mmioFOURCC_WMC('M','P','4','3')
#define FOURCC_mp42     mmioFOURCC_WMC('m','p','4','2')
#define FOURCC_mp43     mmioFOURCC_WMC('m','p','4','3')


OSCL_DLL_ENTRY_POINT_DEFAULT()

int32 GetNAL_Config(uint8** bitstream, int32* size);

OSCL_EXPORT_REF int16 intel_video_config_parser(pvVideoConfigParserInputs *aInputs, pvVideoConfigParserOutputs *aOutputs)
{
    if (aInputs->iMimeType == PVMF_MIME_M4V) //m4v
    {
        mp4StreamType psBits;
        psBits.data = aInputs->inPtr;
        if (aInputs->inBytes <= 0)
        {
            return -1;
        }
        psBits.numBytes = aInputs->inBytes;
        psBits.bytePos = 0;
        psBits.bitBuf = 0;
        psBits.dataBitPos = 0;
        psBits.bitPos = 32;

        int32 width, height, display_width, display_height = 0;
        int32 profile_level = 0;
        int16 retval = 0;
        retval = iDecodeVOLHeader(&psBits, &width, &height, &display_width, &display_height, &profile_level);
        if (retval != 0)
        {
            return retval;
        }
        aOutputs->width  = (uint32)display_width;
        aOutputs->height = (uint32)display_height;
        aOutputs->profile = (uint32)profile_level; // for mp4, profile/level info is packed
        aOutputs->level = 0;
    }
    else if (aInputs->iMimeType == PVMF_MIME_H2631998 ||
             aInputs->iMimeType == PVMF_MIME_H2632000)//h263
    {
        // Nothing to do for h263
        aOutputs->width  = 0;
        aOutputs->height = 0;
        aOutputs->profile = 0;
        aOutputs->level = 0;
    }
    else if (aInputs->iMimeType == PVMF_MIME_H264_VIDEO ||
             aInputs->iMimeType == PVMF_MIME_H264_VIDEO_MP4) //avc
    {
        int32 width, height, display_width, display_height = 0;
        int32 profile_idc, level_idc = 0;

        uint8 *tp = aInputs->inPtr;

        if (aInputs->inBytes > 1)
        {
            if (tp[0] == 0 && tp[1] == 0)
            {
                // ByteStream Format
                uint8* tmp_ptr = tp;
                uint8* buffer_begin = tp;
                int16 length = 0;
                int initbufsize = aInputs->inBytes;
                int tConfigSize = 0;
                do
                {
                    tmp_ptr += length;
                    length = GetNAL_Config(&tmp_ptr, &initbufsize);
                    buffer_begin[0] = length & 0xFF;
                    buffer_begin[1] = (length >> 8) & 0xFF;
                    oscl_memcpy(buffer_begin + 2, tmp_ptr, length);
                    buffer_begin += (length + 2);
                    tConfigSize += (length + 2);
                }
                while (initbufsize > 0);
            }
        }

        // check codec info and get settings
        int16 retval;
        retval = iGetAVCConfigInfo(tp,
                                   aInputs->inBytes,
                                   (int*) & width,
                                   (int*) & height,
                                   (int*) & display_width,
                                   (int*) & display_height,
                                   (int*) & profile_idc,
                                   (int*) & level_idc);
        if (retval != 0)
        {
            return retval;
        }
        aOutputs->width  = (uint32)display_width;
        aOutputs->height = (uint32)display_height;
        aOutputs->profile = (uint32)profile_idc;
        aOutputs->level = (uint32) level_idc;
    }
    else if (aInputs->iMimeType == PVMF_MIME_WMV) //wmv
    {
        uint32 dwdat;
        uint16 wdat;
        uint8 bdat;
        uint8 *pData = aInputs->inPtr;

        LoadDWORD(dwdat, pData); // Window width
        LoadDWORD(dwdat, pData); // Window height
        LoadBYTE(bdat, pData);
        LoadWORD(wdat, pData);  // Size of image info.

        // BITMAPINFOHEADER
        LoadDWORD(dwdat, pData); // Size of BMAPINFOHDR
        LoadDWORD(dwdat, pData);
        aOutputs->width = dwdat;
        LoadDWORD(dwdat, pData);
        aOutputs->height = dwdat;

        /* WMV1 and WMV2 are not supported. Rejected here. */
        /* Code to Check if comp is WMV1 then return not supported */
        pData += 4;
        LoadDWORD(dwdat, pData);
        uint32 NewCompression = dwdat;
        uint32 NewSeqHeader = 0;
        uint32 NewProfile = 0;
        // in case of WMV store "Compression Type as Level"
        aOutputs->level = NewCompression;

        if (NewCompression != FOURCC_WMV2 &&
                NewCompression != FOURCC_WMV3 &&
                NewCompression != FOURCC_WVC1 &&
                NewCompression != FOURCC_WMVA &&
                NewCompression != FOURCC_MP42 &&
                NewCompression != FOURCC_MP43 &&
                NewCompression != FOURCC_mp42 &&
                NewCompression != FOURCC_mp43)
        {
            return -1;
        }
        // get profile etc.
        // Check sequence header
        switch (NewCompression)
        {
            case FOURCC_WMV3:
            {
                // start fresh
                pData = aInputs->inPtr;
                pData += (11 + 40); //sizeof(BITMAPINFOHEADER); // position to sequence header

                LoadDWORD(dwdat, pData);
                NewSeqHeader = dwdat; // this is little endian read sequence header

                NewProfile = (NewSeqHeader & 0xC0) >> 6; // 0 - simple , 1- main, 3 - complex, 2-forbidden

                break;
            }
            case FOURCC_WMVA:
            {
                pData = aInputs->inPtr;
                pData += (11 + 40 + ASFBINDING_SIZE); //sizeof(BITMAPINFOHEADER); // position to sequence header

                LoadDWORD(dwdat, pData); // prefix  // this is little endian read sequence header
                LoadDWORD(dwdat, pData);
                NewSeqHeader = dwdat;

                NewProfile = (NewSeqHeader & 0xC0) >> 6; // this must be 3
            }
            break;

            default:

                NewProfile = 0;
                break;
        }

        aOutputs->profile = NewProfile;

    }
    else
    {
        return -1;
    }
    return 0;
}


/* This function finds a nal from the SC's, moves the bitstream pointer to the beginning of the NAL unit, returns the
    size of the NAL, and at the same time, updates the remaining size in the bitstream buffer that is passed in */
int32 GetNAL_Config(uint8** bitstream, int32* size)
{
    int i = 0;
    int j;
    uint8* nal_unit = *bitstream;
    int count = 0;

    /* find SC at the beginning of the NAL */
    while (nal_unit[i++] == 0 && i < *size)
    {
    }

    if (nal_unit[i-1] == 1)
    {
        *bitstream = nal_unit + i;
    }
    else
    {
        j = *size;
        *size = 0;
        return j;  // no SC at the beginning, not supposed to happen
    }

    j = i;

    /* found the SC at the beginning of the NAL, now find the SC at the beginning of the next NAL */
    while (i < *size)
    {
        if (count == 2 && nal_unit[i] == 0x01)
        {
            i -= 2;
            break;
        }

        if (nal_unit[i])
            count = 0;
        else
            count++;
        i++;
    }

    *size -= i;
    return (i - j);
}
