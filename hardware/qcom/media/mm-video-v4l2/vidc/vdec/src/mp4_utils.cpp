/*--------------------------------------------------------------------------
Copyright (c) 2010 - 2013, The Linux Foundation. All rights reserved.

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
#include "mp4_utils.h"
//#include "omx_vdec.h"
#include "vidc_debug.h"
# include <stdio.h>
#ifdef _ANDROID_
extern "C" {
#include<utils/Log.h>
}
#endif//_ANDROID_

MP4_Utils::MP4_Utils()
{
    m_SrcWidth = 0;
    m_SrcHeight = 0;
    vop_time_resolution = 0;
    vop_time_found = false;

}
MP4_Utils::~MP4_Utils()
{
}

uint32 MP4_Utils::read_bit_field(posInfoType * posPtr, uint32 size)
{
    uint8 *bits = &posPtr->bytePtr[0];
    uint32 bitBuf =
        (bits[0] << 24) | (bits[1] << 16) | (bits[2] << 8) | bits[3];

    uint32 value = (bitBuf >> (32 - posPtr->bitPos - size)) & MASK(size);

    /* Update the offset in preparation for next field    */
    posPtr->bitPos += size;

    while (posPtr->bitPos >= 8) {
        posPtr->bitPos -= 8;
        posPtr->bytePtr++;
    }

    return value;
}
    static uint8 *find_code
(uint8 * bytePtr, uint32 size, uint32 codeMask, uint32 referenceCode)
{
    uint32 code = 0xFFFFFFFF;

    for (uint32 i = 0; i < size; i++) {
        code <<= 8;
        code |= *bytePtr++;

        if ((code & codeMask) == referenceCode) {
            return bytePtr;
        }
    }

    DEBUG_PRINT_LOW("Unable to find code 0x%x", referenceCode);
    return NULL;
}
bool MP4_Utils::parseHeader(mp4StreamType * psBits)
{
    uint32 profile_and_level_indication = 0;
    uint8 VerID = 1; /* default value */
    long hxw = 0;

    m_posInfo.bitPos = 0;
    m_posInfo.bytePtr = psBits->data;
    m_dataBeginPtr = psBits->data;

    m_posInfo.bytePtr = find_code(m_posInfo.bytePtr,4,
            MASK(32),VOP_START_CODE);

    if (m_posInfo.bytePtr) {
        return false;
    }

    m_posInfo.bitPos = 0;
    m_posInfo.bytePtr = psBits->data;
    m_dataBeginPtr = psBits->data;
    m_posInfo.bytePtr = find_code(m_posInfo.bytePtr,4,
            MASK(32),GOV_START_CODE);

    if (m_posInfo.bytePtr) {
        return false;
    }

    m_posInfo.bitPos = 0;
    m_posInfo.bytePtr = psBits->data;
    m_dataBeginPtr = psBits->data;
    /* parsing Visual Object Seqence(VOS) header */
    m_posInfo.bytePtr = find_code(m_posInfo.bytePtr,
            psBits->numBytes,
            MASK(32),
            VISUAL_OBJECT_SEQUENCE_START_CODE);

    if ( m_posInfo.bytePtr == NULL ) {
        m_posInfo.bitPos  = 0;
        m_posInfo.bytePtr = psBits->data;
    } else {
        uint32 profile_and_level_indication = read_bit_field (&m_posInfo, 8);
    }

    /* parsing Visual Object(VO) header*/
    /* note: for now, we skip over the user_data */
    m_posInfo.bytePtr = find_code(m_posInfo.bytePtr,psBits->numBytes,
            MASK(32),VISUAL_OBJECT_START_CODE);

    if (m_posInfo.bytePtr == NULL) {
        m_posInfo.bitPos = 0;
        m_posInfo.bytePtr = psBits->data;
    } else {
        uint32 is_visual_object_identifier = read_bit_field (&m_posInfo, 1);

        if ( is_visual_object_identifier ) {
            /* visual_object_verid*/
            read_bit_field (&m_posInfo, 4);
            /* visual_object_priority*/
            read_bit_field (&m_posInfo, 3);
        }

        /* visual_object_type*/
        uint32 visual_object_type = read_bit_field (&m_posInfo, 4);

        if ( visual_object_type != VISUAL_OBJECT_TYPE_VIDEO_ID ) {
            return false;
        }

        /* skipping video_signal_type params*/
        /*parsing Video Object header*/
        m_posInfo.bytePtr = find_code(m_posInfo.bytePtr,psBits->numBytes,
                VIDEO_OBJECT_START_CODE_MASK,VIDEO_OBJECT_START_CODE);

        if ( m_posInfo.bytePtr == NULL ) {
            return false;
        }
    }

    /* parsing Video Object Layer(VOL) header */
    m_posInfo.bitPos = 0;
    m_posInfo.bytePtr = find_code(m_posInfo.bytePtr,
            psBits->numBytes,
            VIDEO_OBJECT_LAYER_START_CODE_MASK,
            VIDEO_OBJECT_LAYER_START_CODE);

    if ( m_posInfo.bytePtr == NULL ) {
        m_posInfo.bitPos = 0;
        m_posInfo.bytePtr = psBits->data;
    }

    // 1 -> random accessible VOL
    read_bit_field(&m_posInfo, 1);

    uint32 video_object_type_indication = read_bit_field (&m_posInfo, 8);

    if ( (video_object_type_indication != SIMPLE_OBJECT_TYPE) &&
            (video_object_type_indication != SIMPLE_SCALABLE_OBJECT_TYPE) &&
            (video_object_type_indication != CORE_OBJECT_TYPE) &&
            (video_object_type_indication != ADVANCED_SIMPLE) &&
            (video_object_type_indication != RESERVED_OBJECT_TYPE) &&
            (video_object_type_indication != MAIN_OBJECT_TYPE)) {
        return false;
    }

    /* is_object_layer_identifier*/
    uint32 is_object_layer_identifier = read_bit_field (&m_posInfo, 1);

    if (is_object_layer_identifier) {
        uint32 video_object_layer_verid = read_bit_field (&m_posInfo, 4);
        uint32 video_object_layer_priority = read_bit_field (&m_posInfo, 3);
        VerID = (unsigned char)video_object_layer_verid;
    }

    /* aspect_ratio_info*/
    uint32 aspect_ratio_info = read_bit_field (&m_posInfo, 4);

    if ( aspect_ratio_info == EXTENDED_PAR ) {
        /* par_width*/
        read_bit_field (&m_posInfo, 8);
        /* par_height*/
        read_bit_field (&m_posInfo, 8);
    }

    /* vol_control_parameters */
    uint32 vol_control_parameters = read_bit_field (&m_posInfo, 1);

    if ( vol_control_parameters ) {
        /* chroma_format*/
        uint32 chroma_format = read_bit_field (&m_posInfo, 2);

        if ( chroma_format != 1 ) {
            return false;
        }

        /* low_delay*/
        uint32 low_delay = read_bit_field (&m_posInfo, 1);
        /* vbv_parameters (annex D)*/
        uint32 vbv_parameters = read_bit_field (&m_posInfo, 1);

        if ( vbv_parameters ) {
            /* first_half_bitrate*/
            uint32 first_half_bitrate = read_bit_field (&m_posInfo, 15);
            uint32 marker_bit = read_bit_field (&m_posInfo, 1);

            if ( marker_bit != 1) {
                return false;
            }

            /* latter_half_bitrate*/
            uint32 latter_half_bitrate = read_bit_field (&m_posInfo, 15);
            marker_bit = read_bit_field (&m_posInfo, 1);

            if ( marker_bit != 1) {
                return false;
            }

            uint32 VBVPeakBitRate = (first_half_bitrate << 15) + latter_half_bitrate;
            /* first_half_vbv_buffer_size*/
            uint32 first_half_vbv_buffer_size = read_bit_field (&m_posInfo, 15);
            marker_bit = read_bit_field (&m_posInfo, 1);

            if ( marker_bit != 1) {
                return false;
            }

            /* latter_half_vbv_buffer_size*/
            uint32 latter_half_vbv_buffer_size = read_bit_field (&m_posInfo, 3);
            uint32 VBVBufferSize = (first_half_vbv_buffer_size << 3) + latter_half_vbv_buffer_size;
            /* first_half_vbv_occupancy*/
            uint32 first_half_vbv_occupancy = read_bit_field (&m_posInfo, 11);
            marker_bit = read_bit_field (&m_posInfo, 1);

            if ( marker_bit != 1) {
                return false;
            }

            /* latter_half_vbv_occupancy*/
            uint32 latter_half_vbv_occupancy = read_bit_field (&m_posInfo, 15);
            marker_bit = read_bit_field (&m_posInfo, 1);

            if ( marker_bit != 1) {
                return false;
            }
        }/* vbv_parameters*/
    }/*vol_control_parameters*/

    /* video_object_layer_shape*/
    uint32 video_object_layer_shape = read_bit_field (&m_posInfo, 2);
    uint8 VOLShape = (unsigned char)video_object_layer_shape;

    if ( VOLShape != MPEG4_SHAPE_RECTANGULAR ) {
        return false;
    }

    /* marker_bit*/
    uint32 marker_bit = read_bit_field (&m_posInfo, 1);

    if ( marker_bit != 1 ) {
        return false;
    }

    /* vop_time_increment_resolution*/
    uint32 vop_time_increment_resolution = read_bit_field (&m_posInfo, 16);
    vop_time_resolution = vop_time_increment_resolution;
    vop_time_found = true;
    return true;
}

bool MP4_Utils::is_notcodec_vop(unsigned char *pbuffer, unsigned int len)
{
    unsigned int index = 4,vop_bits=0;
    unsigned int temp = vop_time_resolution - 1;
    unsigned char vop_type=0,modulo_bit=0,not_coded=0;

    if (!vop_time_found || !pbuffer || len < 5) {
        return false;
    }

    if ((pbuffer[0] == 0) && (pbuffer[1] == 0) && (pbuffer[2] == 1) && (pbuffer[3] == 0xB6)) {
        while (temp) {
            vop_bits++;
            temp >>= 1;
        }

        vop_type = (pbuffer[index] & 0xc0) >> 6;
        unsigned bits_parsed = 2;

        do {
            modulo_bit = pbuffer[index]  & (1 << (7-bits_parsed));
            bits_parsed++;
            index += bits_parsed/8;
            bits_parsed = bits_parsed %8;

            if (index >= len) {
                return false;
            }
        } while (modulo_bit);

        bits_parsed++; //skip marker bit
        bits_parsed += vop_bits + 1;//Vop bit & Marker bits
        index += bits_parsed/8;

        if (index >= len) {
            return false;
        }

        bits_parsed = bits_parsed % 8;
        not_coded = pbuffer[index] & (1 << (7 - bits_parsed));

        if (!not_coded) {
            return true;
        }
    }

    return false;
}
