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

//#define LOG_NDEBUG 0
#define LOG_TAG "intel_m4v_config_parser.cpp"
#include <utils/Log.h>


#include "intel_m4v_config_parser.h"
#include "oscl_mem.h"
#include "oscl_dll.h"
OSCL_DLL_ENTRY_POINT_DEFAULT()

#define PV_CLZ(A,B) while (((B) & 0x8000) == 0) {(B) <<=1; A++;}

static const uint32 mask[33] =
{
    0x00000000, 0x00000001, 0x00000003, 0x00000007,
    0x0000000f, 0x0000001f, 0x0000003f, 0x0000007f,
    0x000000ff, 0x000001ff, 0x000003ff, 0x000007ff,
    0x00000fff, 0x00001fff, 0x00003fff, 0x00007fff,
    0x0000ffff, 0x0001ffff, 0x0003ffff, 0x0007ffff,
    0x000fffff, 0x001fffff, 0x003fffff, 0x007fffff,
    0x00ffffff, 0x01ffffff, 0x03ffffff, 0x07ffffff,
    0x0fffffff, 0x1fffffff, 0x3fffffff, 0x7fffffff,
    0xffffffff
};

int32 LocateFrameHeader(uint8 *ptr, int32 size)
{
    int32 count = 0;
    int32 i = size;

    if (size < 1)
    {
        return 0;
    }
    while (i--)
    {
        if ((count > 1) && (*ptr == 0x01))
        {
            i += 2;
            break;
        }

        if (*ptr++)
            count = 0;
        else
            count++;
    }
    return (size - (i + 1));
}

void movePointerTo(mp4StreamType *psBits, int32 pos)
{
    uint32 byte_pos;
    if (pos < 0)
    {
        pos = 0;
    }

    byte_pos = pos >> 3;

    if (byte_pos > (psBits->numBytes - psBits->bytePos))
    {
        byte_pos = (psBits->numBytes - psBits->bytePos);
    }

    psBits->bytePos = byte_pos & -4;
    psBits->dataBitPos = psBits->bytePos << 3;
    FlushBits(psBits, ((pos & 0x7) + ((byte_pos & 0x3) << 3)));
}

int16 SearchNextM4VFrame(mp4StreamType *psBits)
{
    int16 status = 0;
    uint8 *ptr;
    int32 i;
    uint32 initial_byte_aligned_position = (psBits->dataBitPos + 7) >> 3;

    ptr = psBits->data + initial_byte_aligned_position;

    i = LocateFrameHeader(ptr, psBits->numBytes - initial_byte_aligned_position);
    if (psBits->numBytes <= initial_byte_aligned_position + i)
    {
        status = -1;
    }
    (void)movePointerTo(psBits, ((i + initial_byte_aligned_position) << 3)); /* ptr + i */
    return status;
}

OSCL_EXPORT_REF int16 iGetM4VConfigInfo(uint8 *buffer, int32 length, int32 *width, int32 *height, int32 *display_width, int32 *display_height)
{
    int16 status;
    mp4StreamType psBits;
    psBits.data = buffer;
    psBits.numBytes = length;
    psBits.bitBuf = 0;
    psBits.bitPos = 32;
    psBits.bytePos = 0;
    psBits.dataBitPos = 0;
    *width = *height = *display_height = *display_width = 0;

    if (length == 0)
    {
        return MP4_INVALID_VOL_PARAM;
    }
    int32 profilelevel = 0; // dummy value discarded here
    status = iDecodeVOLHeader(&psBits, width, height, display_width, display_height, &profilelevel);
    return status;
}

// name: iDecodeVOLHeader
// Purpose: decode VOL header
// return:  error code
OSCL_EXPORT_REF int16 iDecodeVOLHeader(mp4StreamType *psBits, int32 *width, int32 *height, int32 *display_width, int32 *display_height, int32 *profilelevel)
{
    int16 iErrorStat;
    uint32 codeword;
    int32 time_increment_resolution, nbits_time_increment;
    int32 i, j;

    *profilelevel = 0x0000FFFF; // init to some invalid value. When this value is returned, then no profilelevel info is available

    ShowBits(psBits, 32, &codeword);

    if (codeword == VISUAL_OBJECT_SEQUENCE_START_CODE)
    {
        //DV: this is the wrong way to skip bits, use FLush or Read psBits->dataBitPos += 32;
        ReadBits(psBits, 32, &codeword); // skip 32 bits of the Start code

        ReadBits(psBits, 8, &codeword);

        // record profile and level
        *profilelevel = (int) codeword;

        ShowBits(psBits, 32, &codeword);
        if (codeword == USER_DATA_START_CODE)
        {
            iErrorStat = DecodeUserData(psBits);
            if (iErrorStat) return MP4_INVALID_VOL_PARAM;
        }


        ReadBits(psBits, 32, &codeword);
        if (codeword != VISUAL_OBJECT_START_CODE) return MP4_INVALID_VOL_PARAM;

        /*  is_visual_object_identifier            */
        ReadBits(psBits, 1, &codeword);

        if (codeword)
        {
            /* visual_object_verid                            */
            ReadBits(psBits, 4, &codeword);
            /* visual_object_priority                         */
            ReadBits(psBits, 3, &codeword);
        }
        /* visual_object_type                                 */
        ReadBits(psBits, 4, &codeword);

        if (codeword == 1)
        { /* video_signal_type */
            ReadBits(psBits, 1, &codeword);
            if (codeword == 1)
            {
                /* video_format */
                ReadBits(psBits, 3, &codeword);
                /* video_range  */
                ReadBits(psBits, 1, &codeword);
                /* color_description */
                ReadBits(psBits, 1, &codeword);
                if (codeword == 1)
                {
                    /* color_primaries */
                    ReadBits(psBits, 8, &codeword);;
                    /* transfer_characteristics */
                    ReadBits(psBits, 8, &codeword);
                    /* matrix_coefficients */
                    ReadBits(psBits, 8, &codeword);
                }
            }
        }
        else
        {
            int16 status = 0;
            do
            {
                /* Search for VOL_HEADER */
                status = SearchNextM4VFrame(psBits); /* search 0x00 0x00 0x01 */
                if (status != 0)
                    return MP4_INVALID_VOL_PARAM;

                status = ReadBits(psBits, VOL_START_CODE_LENGTH, &codeword);
            }
            while ((codeword != VOL_START_CODE) && (status == 0));
            goto decode_vol;
        }
        /* next_start_code() */
        ByteAlign(psBits);

        ShowBits(psBits, 32, &codeword);
        if (codeword == USER_DATA_START_CODE)
        {
            iErrorStat = DecodeUserData(psBits);
            if (iErrorStat) return MP4_INVALID_VOL_PARAM;
        }
        ShowBits(psBits, 27, &codeword);
    }
    else
    {
        ShowBits(psBits, 27, &codeword);
    }

    if (codeword == VO_START_CODE)
    {

        ReadBits(psBits, 32, &codeword);

        /* video_object_layer_start_code                   */
        ReadBits(psBits, 28, &codeword);
        if (codeword != VOL_START_CODE)
        {
            if (psBits->dataBitPos >= (psBits->numBytes << 3))
            {
                return SHORT_HEADER_MODE; /* SH */
            }
            else
            {
                int16 status = 0;
                do
                {
                    /* Search for VOL_HEADER */
                    status = SearchNextM4VFrame(psBits); /* search 0x00 0x00 0x01 */
                    if (status != 0)
                        return MP4_INVALID_VOL_PARAM;

                    status = ReadBits(psBits, VOL_START_CODE_LENGTH, &codeword);
                }
                while ((codeword != VOL_START_CODE) && (status == 0));
                goto decode_vol;
            }
        }
decode_vol:

        uint32 vol_id;

        /* vol_id (4 bits) */
        ReadBits(psBits, 4, & vol_id);

        // RandomAccessibleVOLFlag
        ReadBits(psBits, 1, &codeword);

        //Video Object Type Indication
        ReadBits(psBits, 8, &codeword);
        if (codeword != 1)
        {
            return MP4_INVALID_VOL_PARAM;
        }

        // is_object_layer_identifier
        ReadBits(psBits, 1, &codeword);

        if (codeword)
        {
            ReadBits(psBits, 4, &codeword);
            ReadBits(psBits, 3, &codeword);
        }

        // aspect ratio
        ReadBits(psBits, 4, &codeword);

        if (codeword == 0xF)
        {
            // Extended Parameter
            /* width */
            ReadBits(psBits, 8, &codeword);
            /* height */
            ReadBits(psBits, 8, &codeword);
        }

        ReadBits(psBits, 1, &codeword);

        if (codeword)
        {
            ReadBits(psBits, 2, &codeword);
            if (codeword != 1)
            {
                return MP4_INVALID_VOL_PARAM;
            }

            ReadBits(psBits, 1, &codeword);

            if (!codeword)
            {
                return MP4_INVALID_VOL_PARAM;
            }

            ReadBits(psBits, 1, &codeword);
            if (codeword)   /* if (vbv_parameters) {}, page 36 */
            {
                ReadBits(psBits, 15, &codeword);
                ReadBits(psBits, 1, &codeword);
                if (codeword != 1)
                    return MP4_INVALID_VOL_PARAM;

                ReadBits(psBits, 15, &codeword);
                ReadBits(psBits, 1, &codeword);
                if (codeword != 1)
                    return MP4_INVALID_VOL_PARAM;


                ReadBits(psBits, 19, &codeword);
                if (!(codeword & 0x8))
                    return MP4_INVALID_VOL_PARAM;

                ReadBits(psBits, 11, &codeword);
                ReadBits(psBits, 1, &codeword);
                if (codeword != 1)
                    return MP4_INVALID_VOL_PARAM;

                ReadBits(psBits, 15, &codeword);
                ReadBits(psBits, 1, &codeword);
                if (codeword != 1)
                    return MP4_INVALID_VOL_PARAM;
            }

        }

        ReadBits(psBits, 2, &codeword);

        if (codeword != 0)
        {
            return MP4_INVALID_VOL_PARAM;
        }

        ReadBits(psBits, 1, &codeword);
        if (codeword != 1)
            return MP4_INVALID_VOL_PARAM;

        ReadBits(psBits, 16, &codeword);
        time_increment_resolution = codeword;


        ReadBits(psBits, 1, &codeword);
        if (codeword != 1)
            return MP4_INVALID_VOL_PARAM;



        ReadBits(psBits, 1, &codeword);

        if (codeword && time_increment_resolution > 2)
        {
            i = time_increment_resolution - 1;
            j = 1;
            while (i >>= 1)
            {
                j++;
            }
            nbits_time_increment = j;

            ReadBits(psBits, nbits_time_increment, &codeword);
        }

        ReadBits(psBits, 1, &codeword);
        if (codeword != 1)
            return MP4_INVALID_VOL_PARAM;

        /* this should be 176 for QCIF */
        ReadBits(psBits, 13, &codeword);
        *display_width = (int32)codeword;
        ReadBits(psBits, 1, &codeword);
        if (codeword != 1)
            return MP4_INVALID_VOL_PARAM;

        /* this should be 144 for QCIF */
        ReadBits(psBits, 13, &codeword);
        *display_height = (int32)codeword;

        *width = (*display_width + 15) & -16;
        *height = (*display_height + 15) & -16;
    }
    else
    {
        /* SHORT_HEADER */
        ShowBits(psBits, SHORT_VIDEO_START_MARKER_LENGTH, &codeword);
        if (codeword == SHORT_VIDEO_START_MARKER)
        {
            iDecodeShortHeader(psBits, width, height, display_width, display_height);
        }
        else
        {
            int16 status = 0;
            do
            {
                /* Search for VOL_HEADER */
                status = SearchNextM4VFrame(psBits); /* search 0x00 0x00 0x01 */
                if (status != 0)
                    return MP4_INVALID_VOL_PARAM;

                status = ReadBits(psBits, VOL_START_CODE_LENGTH, &codeword);
            }
            while ((codeword != VOL_START_CODE) && (status == 0));
            goto decode_vol;
        }
    }
    return 0;
}



OSCL_EXPORT_REF
int16 iDecodeShortHeader(mp4StreamType *psBits,
                         int32 *width,
                         int32 *height,
                         int32 *display_width,
                         int32 *display_height)
{
    uint32 codeword;
    int32   extended_PTYPE = 0;
    int32 UFEP = 0;
    int32 custom_PFMT = 0;

    ShowBits(psBits, 22, &codeword);

    if (codeword !=  0x20)
    {
        return MP4_INVALID_VOL_PARAM;
    }
    FlushBits(psBits, 22);
    ReadBits(psBits, 8, &codeword);

    ReadBits(psBits, 1, &codeword);
    if (codeword == 0) return MP4_INVALID_VOL_PARAM;

    ReadBits(psBits, 1, &codeword);
    if (codeword == 1) return MP4_INVALID_VOL_PARAM;

    ReadBits(psBits, 1, &codeword);
    if (codeword == 1) return MP4_INVALID_VOL_PARAM;

    ReadBits(psBits, 1, &codeword);
    if (codeword == 1) return MP4_INVALID_VOL_PARAM;

    ReadBits(psBits, 1, &codeword);
    if (codeword == 1) return MP4_INVALID_VOL_PARAM;

    /* source format */
    ReadBits(psBits, 3, &codeword);
    switch (codeword)
    {
        case 1:
            *width = 128;
            *height = 96;
            break;

        case 2:
            *width = 176;
            *height = 144;
            break;

        case 3:
            *width = 352;
            *height = 288;
            break;

        case 4:
            *width = 704;
            *height = 576;
            break;

        case 5:
            *width = 1408;
            *height = 1152;
            break;

        case 7:
            extended_PTYPE = 1;
            break;
        default:
            /* Msg("H.263 source format not legal\n"); */
            return MP4_INVALID_VOL_PARAM;
    }

    if (extended_PTYPE == 0)
    {
        *display_width = *width;
        *display_height = *height;
        return 0;
    }
    /* source format */
    ReadBits(psBits, 3, &codeword);
    UFEP = codeword;
    if (UFEP == 1)
    {
        ReadBits(psBits, 3, &codeword);
        switch (codeword)
        {
            case 1:
                *width = 128;
                *height = 96;
                break;

            case 2:
                *width = 176;
                *height = 144;
                break;

            case 3:
                *width = 352;
                *height = 288;
                break;

            case 4:
                *width = 704;
                *height = 576;
                break;

            case 5:
                *width = 1408;
                *height = 1152;
                break;

            case 6:
                custom_PFMT = 1;
                break;
            default:
                /* Msg("H.263 source format not legal\n"); */
                return MP4_INVALID_VOL_PARAM;
        }
        if (custom_PFMT == 0)
        {
            *display_width = *width;
            *display_height = *height;
            return 0;
        }
        ReadBits(psBits, 1, &codeword);
        ReadBits(psBits, 1, &codeword);
        if (codeword) return MP4_INVALID_VOL_PARAM;
        ReadBits(psBits, 1, &codeword);
        if (codeword) return MP4_INVALID_VOL_PARAM;
        ReadBits(psBits, 1, &codeword);
        if (codeword) return MP4_INVALID_VOL_PARAM;
        ReadBits(psBits, 3, &codeword);
        ReadBits(psBits, 3, &codeword);
        if (codeword) return MP4_INVALID_VOL_PARAM;             /* RPS, ISD, AIV */
        ReadBits(psBits, 1, &codeword);
        ReadBits(psBits, 4, &codeword);
        if (codeword != 8) return MP4_INVALID_VOL_PARAM;
    }
    if (UFEP == 0 || UFEP == 1)
    {
        ReadBits(psBits, 3, &codeword);
        if (codeword > 1) return MP4_INVALID_VOL_PARAM;
        ReadBits(psBits, 1, &codeword);
        if (codeword) return MP4_INVALID_VOL_PARAM;
        ReadBits(psBits, 1, &codeword);
        if (codeword) return MP4_INVALID_VOL_PARAM;
        ReadBits(psBits, 1, &codeword);
        ReadBits(psBits, 3, &codeword);
        if (codeword != 1) return MP4_INVALID_VOL_PARAM;
    }
    else
    {
        return MP4_INVALID_VOL_PARAM;
    }
    ReadBits(psBits, 1, &codeword);
    if (codeword) return MP4_INVALID_VOL_PARAM; /* CPM */
    if (custom_PFMT == 1 && UFEP == 1)
    {
        ReadBits(psBits, 4, &codeword);
        if (codeword == 0) return MP4_INVALID_VOL_PARAM;
        if (codeword == 0xf)
        {
            ReadBits(psBits, 8, &codeword);
            ReadBits(psBits, 8, &codeword);
        }
        ReadBits(psBits, 9, &codeword);
        *display_width = (codeword + 1) << 2;
        *width = (*display_width + 15) & -16;
        ReadBits(psBits, 1, &codeword);
        if (codeword != 1) return MP4_INVALID_VOL_PARAM;
        ReadBits(psBits, 9, &codeword);
        if (codeword == 0) return MP4_INVALID_VOL_PARAM;
        *display_height = codeword << 2;
        *height = (*display_height + 15) & -16;
    }

    return 0;
}


int16 ShowBits(
    mp4StreamType *pStream,           /* Input Stream */
    uint8 ucNBits,          /* nr of bits to read */
    uint32 *pulOutData      /* output target */
)
{
    uint8 *bits;
    uint32 dataBitPos = pStream->dataBitPos;
    uint32 bitPos = pStream->bitPos;
    uint32 dataBytePos;

    uint i;

    if (ucNBits > (32 - bitPos))    /* not enough bits */
    {
        dataBytePos = dataBitPos >> 3; /* Byte Aligned Position */
        bitPos = dataBitPos & 7; /* update bit position */
        if (dataBytePos > pStream->numBytes - 4)
        {
            pStream->bitBuf = 0;
            for (i = 0; i < pStream->numBytes - dataBytePos; i++)
            {
                pStream->bitBuf |= pStream->data[dataBytePos+i];
                pStream->bitBuf <<= 8;
            }
            pStream->bitBuf <<= 8 * (3 - i);
        }
        else
        {
            bits = &pStream->data[dataBytePos];
            pStream->bitBuf = (bits[0] << 24) | (bits[1] << 16) | (bits[2] << 8) | bits[3];
        }
        pStream->bitPos = bitPos;
    }

    bitPos += ucNBits;

    *pulOutData = (pStream->bitBuf >> (32 - bitPos)) & mask[(uint16)ucNBits];


    return 0;
}

int16 FlushBits(
    mp4StreamType *pStream,           /* Input Stream */
    uint8 ucNBits                      /* number of bits to flush */
)
{
    uint8 *bits;
    uint32 dataBitPos = pStream->dataBitPos;
    uint32 bitPos = pStream->bitPos;
    uint32 dataBytePos;


    if ((dataBitPos + ucNBits) > (uint32)(pStream->numBytes << 3))
        return (-2); // Buffer over run

    dataBitPos += ucNBits;
    bitPos     += ucNBits;

    if (bitPos > 32)
    {
        dataBytePos = dataBitPos >> 3;    /* Byte Aligned Position */
        bitPos = dataBitPos & 7; /* update bit position */
        bits = &pStream->data[dataBytePos];
        pStream->bitBuf = (bits[0] << 24) | (bits[1] << 16) | (bits[2] << 8) | bits[3];
    }

    pStream->dataBitPos = dataBitPos;
    pStream->bitPos     = bitPos;

    return 0;
}

int16 ReadBits(
    mp4StreamType *pStream,           /* Input Stream */
    uint8 ucNBits,                     /* nr of bits to read */
    uint32 *pulOutData                 /* output target */
)
{
    uint8 *bits;
    uint32 dataBitPos = pStream->dataBitPos;
    uint32 bitPos = pStream->bitPos;
    uint32 dataBytePos;


    if ((dataBitPos + ucNBits) > (pStream->numBytes << 3))
    {
        *pulOutData = 0;
        return (-2); // Buffer over run
    }

    //  dataBitPos += ucNBits;

    if (ucNBits > (32 - bitPos))    /* not enough bits */
    {
        dataBytePos = dataBitPos >> 3;    /* Byte Aligned Position */
        bitPos = dataBitPos & 7; /* update bit position */
        bits = &pStream->data[dataBytePos];
        pStream->bitBuf = (bits[0] << 24) | (bits[1] << 16) | (bits[2] << 8) | bits[3];
    }

    pStream->dataBitPos += ucNBits;
    pStream->bitPos      = (unsigned char)(bitPos + ucNBits);

    *pulOutData = (pStream->bitBuf >> (32 - pStream->bitPos)) & mask[(uint16)ucNBits];

    return 0;
}



int16 ByteAlign(
    mp4StreamType *pStream           /* Input Stream */
)
{
    uint8 *bits;
    uint32 dataBitPos = pStream->dataBitPos;
    uint32 bitPos = pStream->bitPos;
    uint32 dataBytePos;
    uint32 leftBits;

    leftBits =  8 - (dataBitPos & 0x7);
    if (leftBits == 8)
    {
        if ((dataBitPos + 8) > (uint32)(pStream->numBytes << 3))
            return (-2); // Buffer over run
        dataBitPos += 8;
        bitPos += 8;
    }
    else
    {
        dataBytePos = dataBitPos >> 3;
        dataBitPos += leftBits;
        bitPos += leftBits;
    }


    if (bitPos > 32)
    {
        dataBytePos = dataBitPos >> 3;    /* Byte Aligned Position */
        bits = &pStream->data[dataBytePos];
        pStream->bitBuf = (bits[0] << 24) | (bits[1] << 16) | (bits[2] << 8) | bits[3];
    }

    pStream->dataBitPos = dataBitPos;
    pStream->bitPos     = bitPos;

    return 0;
}

int16 DecodeUserData(mp4StreamType *pStream)
{

    uint32 codeword;
    int16 iErrorStat;

    iErrorStat = ReadBits(pStream, 32, &codeword);
    if (iErrorStat) return iErrorStat;
    iErrorStat = ShowBits(pStream, 24, &codeword);
    if (iErrorStat) return iErrorStat;

    while (codeword != 1)
    {
        /* Discard user data for now. */
        iErrorStat = ReadBits(pStream, 8, &codeword);
        if (iErrorStat) return iErrorStat;
        iErrorStat = ShowBits(pStream, 24, &codeword);
        if (iErrorStat) return iErrorStat;
    }
    return 0;
}


OSCL_EXPORT_REF int16 iGetAVCConfigInfo(uint8 *buffer, int32 length, int32 *width, int32 *height, int32 *display_width, int32 *display_height, int32 *profile_idc, int32 *level_idc)
{
    int16 status;
    mp4StreamType psBits;
    uint16 sps_length, pps_length;
    int32 size;
    int32 i = 0;
    uint8* sps = NULL;
    uint8* temp = (uint8 *)OSCL_MALLOC(sizeof(uint8) * length);
    uint8* pps = NULL;


    if (temp)
    {
        sps = temp; // Make a copy of the original pointer to be freed later
        // Successfull allocation... copy input buffer
        oscl_memcpy(sps, buffer, length);
    }
    else
    {
        // Allocation failed
        return MP4_INVALID_VOL_PARAM;
    }

    if (length < 3)
    {
        OSCL_FREE(temp);
        return MP4_INVALID_VOL_PARAM;
    }

    *width = *height = *display_height = *display_width = 0;

    if (sps[0] == 0 && sps[1] == 0)
    {
        /* find SC at the beginning of the NAL */
        while (sps[i++] == 0 && i < length)
        {
        }

        if (sps[i-1] == 1)
        {
            sps += i;

            sps_length = 0;
            // search for the next start code
            while (!(sps[sps_length] == 0 && sps[sps_length+1] == 0 && sps[sps_length+2] == 1) &&
                    sps_length < length - i - 2)
            {
                sps_length++;
            }

            if (sps_length >= length - i - 2)
            {
                OSCL_FREE(temp);
                return MP4_INVALID_VOL_PARAM;
            }

            pps_length = length - i - sps_length - 3;
            pps = sps + sps_length + 3;
        }
        else
        {
            OSCL_FREE(temp);
            return MP4_INVALID_VOL_PARAM;
        }
    }
    else
    {
        sps_length = (uint16)(sps[1] << 8) | sps[0];
        sps += 2;
        pps = sps + sps_length;
        pps_length = (uint16)(pps[1] << 8) | pps[0];
        pps += 2;
    }

    if (sps_length + pps_length > length)
    {
        OSCL_FREE(temp);
        return MP4_INVALID_VOL_PARAM;
    }

    size = sps_length;

    Parser_EBSPtoRBSP(sps, &size);

    psBits.data = sps;
    psBits.numBytes = size;
    psBits.bitBuf = 0;
    psBits.bitPos = 32;
    psBits.bytePos = 0;
    psBits.dataBitPos = 0;

    LOGV("intel: --- call to DecodeSPS() begin ---\n");
    if (DecodeSPS(&psBits, width, height, display_width, display_height, profile_idc, level_idc))
    {
        OSCL_FREE(temp);
        return MP4_INVALID_VOL_PARAM;
    }
    LOGV("intel: --- call to DecodeSPS() end ---\n");

    // now do PPS
    size = pps_length;

    Parser_EBSPtoRBSP(pps, &size);
    psBits.data = pps;
    psBits.numBytes = size;
    psBits.bitBuf = 0;
    psBits.bitPos = 32;
    psBits.bytePos = 0;
    psBits.dataBitPos = 0;

#if 1
    
    return 0;
	
#else

    LOGV("intel: --- call to DecodePPS() begin ---\n");

    status = DecodePPS(&psBits);

    LOGV("intel: --- call to DecodePPS() end : status = %x ---\n", status);

    OSCL_FREE(temp);

    return status;
#endif
}

void scaling_list(mp4StreamType *psBits, uint32 sizeOfScalingList) {
    uint32 lastScale = 8;
    uint32 nextScale = 8;
    int32 delta_scale;
    uint32 j = 0;

    for (j = 0; j < sizeOfScalingList; j++) {
        if(nextScale != 0) {
          /* delta_scale */
          se_v(psBits, &delta_scale);
          nextScale = ( lastScale + delta_scale + 256 ) % 256 ;
        } 
        lastScale = ( nextScale == 0 ) ? lastScale : nextScale;
    } 
}


int16 DecodeSPS(mp4StreamType *psBits, int32 *width, int32 *height, int32 *display_width, int32 *display_height, int32 *profile_idc, int32 *level_idc)
{
    uint32 temp;
    int32 temp0;
    uint left_offset, right_offset, top_offset, bottom_offset;
    uint i;

    ReadBits(psBits, 8, &temp);

    if ((temp & 0x1F) != 7) {
          return MP4_INVALID_VOL_PARAM;
    }

    /* Read profile_idc */
    ReadBits(psBits, 8, &temp);
    *profile_idc = temp;

    /* Read Constraint_set0-3_flag */
    ReadBits(psBits, 1, &temp);
    ReadBits(psBits, 1, &temp);
    ReadBits(psBits, 1, &temp);
    ReadBits(psBits, 1, &temp);

    LOGV("intel: --- profile_idc = %d ---\n", temp);

    /* reserved_zero_4bits */
    ReadBits(psBits, 4, &temp);

    /* level_idc */ 
    ReadBits(psBits, 8, &temp);
    *level_idc = temp;

    if (temp > 51)
        return MP4_INVALID_VOL_PARAM;

    /* seq_parameter_set_id */
    ue_v(psBits, &temp);
    if(*profile_idc == 100 || *profile_idc == 110 || 
       *profile_idc == 122 || *profile_idc == 144 )
    {
    /* chroma_format_idc */ 
       ue_v(psBits, &temp);
       if(temp == 3) {
       /* residual_colour_transform_flag */
          ReadBits(psBits, 1, &temp);
       }
       /* bit_depth_luma_minus8 */
       ue_v(psBits, &temp);
  
       /* bit_depth_chroma_minus8 */
       ue_v(psBits, &temp);
   
       /* qpprime_y_zero_transform_bypass_flag */
       ReadBits(psBits, 1, &temp);

       /* seq_scaling_matrix_present_flag */
       ReadBits(psBits, 1, &temp);
 
       if(temp) {
           int i = 0;
           for(i = 0; i < 8; i++) {
               ReadBits(psBits, 1, &temp);
               if(temp) {
                  if(i < 6 ) {
                     scaling_list(psBits, 16);
                  } else {
                     scaling_list(psBits, 64);
                  }
              } 
           }
       }
    }

    /* log2_max_frame_num_minus4 */
    ue_v(psBits, &temp);

    /* pic_order_cnt_type */
    ue_v(psBits, &temp);

    if (temp == 0)
    {
        ue_v(psBits, &temp);
    }
    else if (temp == 1)
    {
        ReadBits(psBits, 1, &temp);
        se_v(psBits, &temp0);
        se_v(psBits, &temp0);
        ue_v(psBits, &temp);

        for (i = 0; i < temp; i++)
        {
            se_v(psBits, &temp0);
        }
    }

    /* num_ref_frames */
    ue_v(psBits, &temp);

    /* gaps_in_frame_num_value_allowed_flag */
    ReadBits(psBits, 1, &temp);


    /* pic_width_in_mbs_minus1 */
    ue_v(psBits, &temp);
    *display_width = *width = (temp + 1) << 4;

    /* pic_height_in_map_units_minus1 */
    ue_v(psBits, &temp);
    *display_height = *height = (temp + 1) << 4;


    LOGV("---intel: 1 *display_width = %d *display_height = %d\n", *display_width, *display_height);

    /* frame_mbs_only_flag */
    ReadBits(psBits, 1, &temp);

    if (!temp)
    {
       /* mb_adaptive_frame_field_flag */
        ReadBits(psBits,1, &temp);
    }

    /* direct_8x8_inference_flag */
    ReadBits(psBits, 1, &temp);
    
    /* frame_cropping_flag */
    ReadBits(psBits, 1, &temp);

    if (temp)
    {
        ue_v(psBits, (uint32*)&left_offset);
        ue_v(psBits, (uint32*)&right_offset);
        ue_v(psBits, (uint32*)&top_offset);
        ue_v(psBits, (uint32*)&bottom_offset);

        *display_width = *width - 2 * (right_offset + left_offset);
        *display_height = *height - 2 * (top_offset + bottom_offset);

        LOGV("---intel: 2 *display_width = %d *display_height = %d\n", *display_width, *display_height);
    }

    /*  no need to check further */
#if USE_LATER
    ReadBits(psBits, 1, &temp);
    if (temp)
    {
        if (!DecodeVUI(psBits))
        {
            return MP4_INVALID_VOL_PARAM;
        }
    }
#endif
    return 0; // return 0 for success
}

#if USE_LATER
/* unused for now */
int32 DecodeVUI(mp4StreamType *psBits)
{
    uint temp;
    uint32 temp32;
    uint aspect_ratio_idc, overscan_appopriate_flag, video_format, video_full_range_flag;
    int32 status;

    ReadBits(psBits, 1, &temp); /* aspect_ratio_info_present_flag */
    if (temp)
    {
        ReadBits(psBits, 8, &aspect_ratio_idc);
        if (aspect_ratio_idc == 255)
        {
            ReadBits(psBits, 16, &temp); /* sar_width */
            ReadBits(psBits, 16, &temp); /* sar_height */
        }
    }
    ReadBits(psBits, 1, &temp); /* overscan_info_present */
    if (temp)
    {
        ReadBits(psBits, 1, &overscan_appopriate_flag);
    }
    ReadBits(psBits, 1, &temp); /* video_signal_type_present_flag */
    if (temp)
    {
        ReadBits(psBits, 3, &video_format);
        ReadBits(psBits, 1, &video_full_range_flag);
        ReadBits(psBits, 1, &temp); /* colour_description_present_flag */
        if (temp)
        {
            ReadBits(psBits, 8, &temp); /* colour_primaries */
            ReadBits(psBits, 8, &temp); /* transfer_characteristics */
            ReadBits(psBits, 8, &temp); /* matrix coefficients */
        }
    }
    ReadBits(psBits, 1, &temp);/*   chroma_loc_info_present_flag */
    if (temp)
    {
        ue_v(psBits, &temp); /*  chroma_sample_loc_type_top_field */
        ue_v(psBits, &temp); /*  chroma_sample_loc_type_bottom_field */
    }

    ReadBits(psBits, 1, &temp); /*  timing_info_present_flag*/
    if (temp)
    {
        ReadBits(psBits, 32, &temp32); /*  num_unit_in_tick*/
        ReadBits(psBits, 32, &temp32); /*   time_scale */
        ReadBits(psBits, 1, &temp); /*  fixed_frame_rate_flag */
    }

    ReadBits(psBits, 1, &temp); /*  nal_hrd_parameters_present_flag */
    if (temp)
    {
        if (!DecodeHRD(psBits))
        {
            return 1;
        }
    }
    ReadBits(psBits, 1, &temp32); /*    vcl_hrd_parameters_present_flag*/
    if (temp32)
    {
        if (!DecodeHRD(psBits))
        {
            return 1;
        }
    }
    if (temp || temp32)
    {
        ReadBits(psBits, 1, &temp);     /*  low_delay_hrd_flag */
    }
    ReadBits(psBits, 1, &temp); /*  pic_struct_present_flag */
    status = ReadBits(psBits, 1, &temp); /* _restriction_flag */
    if (status != 0) // buffer overrun
    {
        return 1;
    }

    if (temp)
    {
        ReadBits(psBits, 1, &temp); /*  motion_vectors_over_pic_boundaries_flag */
        ue_v(psBits, &temp); /* max_bytes_per_pic_denom */
        ue_v(psBits, &temp); /* max_bits_per_mb_denom */
        ue_v(psBits, &temp); /* log2_max_mv_length_horizontal */
        ue_v(psBits, &temp); /* log2_max_mv_length_vertical */
        ue_v(psBits, &temp); /* num_reorder_frames */
        ue_v(psBits, &temp); /* max_dec_frame_buffering */
    }

    return 0; // 0 for success
}

/* unused for now */
int32 DecodeHRD(mp4StreamType *psBits)
{
    uint temp;
    uint cpb_cnt_minus1;
    uint i;
    int32 status;

    ue_v(psBits, &cpb_cnt_minus1);
    ReadBits(psBits, 4, &temp); /*  bit_rate_scale */
    ReadBits(psBits, 4, &temp); /*  cpb_size_scale */
    for (i = 0; i <= cpb_cnt_minus1; i++)
    {
        ue_v(psBits, &temp); /* bit_rate_value_minus1[i] */
        ue_v(psBits, &temp); /* cpb_size_value_minus1[i] */
        ue_v(psBits, &temp); /* cbr_flag[i] */
    }
    ReadBits(psBits, 5, &temp); /*  initial_cpb_removal_delay_length_minus1 */
    ReadBits(psBits, 5, &temp); /*  cpb_removal_delay_length_minus1 */
    ReadBits(psBits, 5, &temp); /*  dpb_output_delay_length_minus1 */
    status = ReadBits(psBits, 5, &temp); /* time_offset_length  */

    if (status != 0) // buffer overrun
    {
        return 1;
    }

    return 0; // 0 for success
}
#endif

// only check for entropy coding mode
int32 DecodePPS(mp4StreamType *psBits)
{
    uint32 temp, pic_parameter_set_id, seq_parameter_set_id, entropy_coding_mode_flag;

    ReadBits(psBits, 8, &temp);

    if ((temp & 0x1F) != 8) return MP4_INVALID_VOL_PARAM;

    ue_v(psBits, &pic_parameter_set_id);
    ue_v(psBits, &seq_parameter_set_id);

    ReadBits(psBits, 1, &entropy_coding_mode_flag);
    if (entropy_coding_mode_flag)
    {
        return 1;
    }

    return 0;
}

void ue_v(mp4StreamType *psBits, uint32 *codeNum)
{
    uint32 temp;
    uint tmp_cnt;
    int32 leading_zeros = 0;
    ShowBits(psBits, 16, &temp);

    tmp_cnt = temp  | 0x1;

    PV_CLZ(leading_zeros, tmp_cnt)

    if (leading_zeros < 8)
    {
        *codeNum = (temp >> (15 - (leading_zeros << 1))) - 1;

	FlushBits(psBits, (leading_zeros << 1) + 1);
	
    }
    else
    {
        ReadBits(psBits, (leading_zeros << 1) + 1, &temp);
        *codeNum = temp - 1;

    }

}


void se_v(mp4StreamType *psBits, int32 *value)
{
    int32 leadingZeros = 0;
    uint32 temp;

    OSCL_UNUSED_ARG(value);

    ReadBits(psBits, 1, &temp);
    while (!temp)
    {
        leadingZeros++;
        if (ReadBits(psBits, 1, &temp))
        {
            break;
        }
    }
    ReadBits(psBits, leadingZeros, &temp);
}

void Parser_EBSPtoRBSP(uint8 *nal_unit, int32 *size)
{
    int32 i, j;
    int32 count = 0;


    for (i = 0; i < *size; i++)
    {
        if (count == 2 && nal_unit[i] == 0x03)
        {
            break;
        }

        if (nal_unit[i])
            count = 0;
        else
            count++;
    }

    count = 0;
    j = i++;
    for (; i < *size; i++)
    {
        if (count == 2 && nal_unit[i] == 0x03)
        {
            i++;
            count = 0;
        }
        nal_unit[j] = nal_unit[i];
        if (nal_unit[i])
            count = 0;
        else
            count++;
        j++;
    }

    *size = j;
}



