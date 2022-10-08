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
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/poll.h>
#include <stdint.h>

#include "frameparser.h"
#include "vidc_debug.h"

#ifdef _ANDROID_
extern "C" {
#include<utils/Log.h>
}
#endif//_ANDROID_

static unsigned char H264_mask_code[4] = {0xFF,0xFF,0xFF,0xFF};
static unsigned char H264_start_code[4] = {0x00,0x00,0x00,0x01};

static unsigned char MPEG4_start_code[4] = {0x00,0x00,0x01,0xB6};
static unsigned char MPEG4_mask_code[4] = {0xFF,0xFF,0xFF,0xFF};

static unsigned char H263_start_code[4] = {0x00,0x00,0x80,0x00};
static unsigned char H263_mask_code[4] = {0xFF,0xFF,0xFC,0x00};

static unsigned char VC1_AP_start_code[4] = {0x00,0x00,0x01,0x0C};
static unsigned char VC1_AP_mask_code[4] = {0xFF,0xFF,0xFF,0xFC};

static unsigned char MPEG2_start_code[4] = {0x00, 0x00, 0x01, 0x00};
static unsigned char MPEG2_mask_code[4] = {0xFF, 0xFF, 0xFF, 0xFF};

frame_parse::frame_parse():mutils(NULL),
    parse_state(A0),
    start_code(NULL),
    mask_code(NULL),
    last_byte_h263(0),
    last_byte(0),
    header_found(false),
    skip_frame_boundary(false),
    state_nal(NAL_LENGTH_ACC),
    nal_length(0),
    accum_length(0),
    bytes_tobeparsed(0)
{
}

frame_parse::~frame_parse ()
{
    if (mutils)
        delete mutils;

    mutils = NULL;
}

int frame_parse::init_start_codes (codec_type codec_type_parse)
{
    /*Check if Codec Type is proper and we are in proper state*/
    if (codec_type_parse > CODEC_TYPE_MAX || parse_state != A0) {
        return -1;
    }

    switch (codec_type_parse) {
        case CODEC_TYPE_MPEG4:
            start_code = MPEG4_start_code;
            mask_code = MPEG4_mask_code;
            break;
        case CODEC_TYPE_H263:
            start_code = H263_start_code;
            mask_code = H263_mask_code;
            break;
        case CODEC_TYPE_H264:
        case CODEC_TYPE_HEVC:
            start_code = H264_start_code;
            mask_code = H264_mask_code;
            break;
        case CODEC_TYPE_VC1:
            start_code = VC1_AP_start_code;
            mask_code = VC1_AP_mask_code;
            break;
        case CODEC_TYPE_MPEG2:
            start_code = MPEG2_start_code;
            mask_code = MPEG2_mask_code;
            break;
#ifdef _MSM8974_
        case CODEC_TYPE_VP8:
            break;
#endif
        default:
            return -1;
    }

    return 1;
}


int frame_parse::init_nal_length (unsigned int nal_len)
{
    if (nal_len == 0 || nal_len > 4 || state_nal != NAL_LENGTH_ACC) {
        return -1;
    }

    nal_length = nal_len;

    return 1;
}

int frame_parse::parse_sc_frame ( OMX_BUFFERHEADERTYPE *source,
        OMX_BUFFERHEADERTYPE *dest ,
        OMX_U32 *partialframe)
{
    OMX_U8 *pdest = NULL,*psource = NULL, match_found = FALSE, is_byte_match = 0;
    OMX_U32 dest_len =0, source_len = 0, temp_len = 0;
    OMX_U32 parsed_length = 0,i=0;
    int residue_byte = 0;

    if (source == NULL || dest == NULL || partialframe == NULL) {
        return -1;
    }

    /*Calculate how many bytes are left in source and destination*/
    dest_len = dest->nAllocLen - (dest->nFilledLen + dest->nOffset);
    psource = source->pBuffer + source->nOffset;
    pdest = dest->pBuffer + (dest->nFilledLen + dest->nOffset);
    source_len = source->nFilledLen;

    /*Need Minimum Start Code size for destination to copy atleast Start code*/
    if ((start_code == H263_start_code && dest_len < 3) ||
            (start_code != H263_start_code && dest_len < 4) || (source_len == 0)) {
        DEBUG_PRINT_LOW("FrameParser: dest_len %u source_len %u",(unsigned int)dest_len, (unsigned int)source_len);

        if (source_len == 0 && (source->nFlags & 0x01)) {
            DEBUG_PRINT_LOW("FrameParser: EOS rxd!! Notify it as a complete frame");
            *partialframe = 0;
            return 1;
        }

        DEBUG_PRINT_LOW("FrameParser: Bitstream Parsing error");
        return -1;
    }

    /*Check if State of the previous find is a Start code*/
    if (parse_state == A4 || parse_state == A5) {
        /*Check for minimun size should be 4*/
        dest->nFlags = source->nFlags;
        dest->nTimeStamp = source->nTimeStamp;

        if (start_code == H263_start_code) {
            memcpy (pdest,start_code,2);
            pdest[2] = last_byte_h263;
            dest->nFilledLen += 3;
            pdest += 3;
        } else {
            memcpy (pdest,start_code,4);

            if (start_code == VC1_AP_start_code
                    || start_code == MPEG4_start_code
                    || start_code == MPEG2_start_code) {
                pdest[3] = last_byte;
                update_skip_frame();
            }

            dest->nFilledLen += 4;
            pdest += 4;
        }

        parse_state = A0;
    }

    /*Entry State Machine*/
    while ( source->nFilledLen > 0 && parse_state != A0
            && parse_state != A4 && parse_state != A5 && dest_len > 0
          ) {
        //printf ("In the Entry Loop");
        switch (parse_state) {
            case A3:
                parse_additional_start_code(psource,&parsed_length);

                if (parse_state == A4) {
                    source->nFilledLen--;
                    source->nOffset++;
                    psource++;
                    break;
                }

                /*If fourth Byte is matching then start code is found*/
                if ((*psource & mask_code [3]) == start_code [3]) {
                    parse_state = A4;
                    last_byte =  *psource;
                    source->nFilledLen--;
                    source->nOffset++;
                    psource++;
                } else if ((start_code [1] == start_code [0]) && (start_code [2]  == start_code [1])) {
                    parse_state = A2;
                    memcpy (pdest,start_code,1);
                    pdest++;
                    dest->nFilledLen++;
                    dest_len--;
                } else if (start_code [2] == start_code [0]) {
                    parse_state = A1;
                    memcpy (pdest,start_code,2);
                    pdest += 2;
                    dest->nFilledLen += 2;
                    dest_len -= 2;
                } else {
                    parse_state = A0;
                    memcpy (pdest,start_code,3);
                    pdest += 3;
                    dest->nFilledLen +=3;
                    dest_len -= 3;
                }

                break;

            case A2:
                is_byte_match = ((*psource & mask_code [2]) == start_code [2]);
                match_found = FALSE;

                if (start_code == H263_start_code) {
                    if (is_byte_match) {
                        last_byte_h263 = *psource;
                        parse_state = A5;
                        match_found = TRUE;
                    }
                } else if (start_code == H264_start_code &&
                        (*psource & mask_code [3]) == start_code [3]) {
                    parse_state = A5;
                    match_found = TRUE;
                } else {
                    if (is_byte_match) {
                        parse_state = A3;
                        match_found = TRUE;
                    }
                }

                if (match_found) {
                    source->nFilledLen--;
                    source->nOffset++;
                    psource++;
                } else if (start_code [1] == start_code [0]) {
                    parse_state = A1;
                    memcpy (pdest,start_code,1);
                    dest->nFilledLen +=1;
                    dest_len--;
                    pdest++;
                } else {
                    parse_state = A0;
                    memcpy (pdest,start_code,2);
                    dest->nFilledLen +=2;
                    dest_len -= 2;
                    pdest += 2;
                }

                break;

            case A1:

                if ((*psource & mask_code [1]) == start_code [1]) {
                    parse_state = A2;
                    source->nFilledLen--;
                    source->nOffset++;
                    psource++;
                } else {
                    memcpy (pdest,start_code,1);
                    dest->nFilledLen +=1;
                    pdest++;
                    dest_len--;
                    parse_state = A0;
                }

                break;
            case A4:
            case A0:
            case A5:
                break;
        }

        dest_len = dest->nAllocLen - (dest->nFilledLen + dest->nOffset);
    }

    if (parse_state == A4 || parse_state == A5) {
        *partialframe = 0;
        check_skip_frame_boundary(partialframe);
        DEBUG_PRINT_LOW("FrameParser: Parsed Len = %u", (unsigned int)dest->nFilledLen);
        return 1;
    }

    /*Partial Frame is true*/
    *partialframe = 1;

    /*Calculate how many bytes are left in source and destination*/
    dest_len = dest->nAllocLen - (dest->nFilledLen + dest->nOffset);
    psource = source->pBuffer + source->nOffset;
    pdest = dest->pBuffer + (dest->nFilledLen + dest->nOffset);
    source_len = source->nFilledLen;

    temp_len = (source_len < dest_len)?source_len:dest_len;

    /*Check if entry state machine consumed source or destination*/
    if (temp_len == 0) {
        return 1;
    }

    /*Parsing State Machine*/
    while  (parsed_length < temp_len) {
        switch (parse_state) {
            case A0:

                if ((psource [parsed_length] & mask_code [0])  == start_code[0]) {
                    parse_state = A1;
                }

                parsed_length++;
                break;
            case A1:

                if ((psource [parsed_length] & mask_code [1]) == start_code [1]) {
                    parsed_length++;
                    parse_state = A2;
                } else {
                    parse_state = A0;
                }

                break;
            case A2:
                is_byte_match = ((psource[parsed_length] & mask_code [2]) == start_code [2]);
                match_found = FALSE;

                if (start_code == H263_start_code) {
                    if (is_byte_match) {
                        last_byte_h263 = psource[parsed_length];
                        parse_state = A5;
                        match_found = TRUE;
                    }
                } else if (start_code == H264_start_code &&
                        (psource[parsed_length] & mask_code [3]) == start_code [3]) {
                    parse_state = A5;
                    match_found = TRUE;
                } else {
                    if (is_byte_match) {
                        parse_state = A3;
                        match_found = TRUE;
                    }
                }

                if (match_found) {
                    parsed_length++;
                } else if (start_code [1] == start_code [0]) {
                    parse_state = A1;
                } else {
                    parse_state = A0;
                }

                break;
            case A3:
                parse_additional_start_code(psource,&parsed_length);

                if (parse_state == A4) break;

                if ((psource [parsed_length] & mask_code [3]) == start_code [3]) {
                    last_byte = psource [parsed_length];
                    parsed_length++;
                    parse_state = A4;
                } else if ((start_code [1] == start_code [0]) && (start_code [2] == start_code [1])) {
                    parse_state = A2;
                } else if (start_code [2] == start_code [0]) {
                    parse_state = A1;
                } else {
                    parse_state = A0;
                }

                break;
            case A4:
            case A5:
                break;
        }

        /*Found the code break*/
        if (parse_state == A4 || parse_state == A5) {
            break;
        }
    }

    /*Exit State Machine*/
    psource = source->pBuffer + source->nOffset;
    OMX_U32 bytes_to_skip = 0;
    switch (parse_state) {
        case A5:
            *partialframe = 0;
            check_skip_frame_boundary(partialframe);
            bytes_to_skip = 3;
            break;
        case A4:
            *partialframe = 0;
            check_skip_frame_boundary(partialframe);
            bytes_to_skip = 4;
            break;
        case A3:
            if (source->nFlags & OMX_BUFFERFLAG_EOS) {
                bytes_to_skip = 0;
            } else {
                bytes_to_skip = 3;
            }
            break;
        case A2:
            if (source->nFlags & OMX_BUFFERFLAG_EOS) {
                bytes_to_skip = 0;
            } else {
                bytes_to_skip = 2;
            }
            break;
        case A1:
            if (source->nFlags & OMX_BUFFERFLAG_EOS) {
                bytes_to_skip = 0;
            } else {
                bytes_to_skip = 1;
            }
            break;
        case A0:
            bytes_to_skip = 0;
            break;
    }

    if (source->nFilledLen < parsed_length) {
        DEBUG_PRINT_ERROR ("FATAL Error");
        return -1;
    }

    if (parsed_length > bytes_to_skip) {
        memcpy (pdest, psource, (parsed_length-bytes_to_skip));
        dest->nFilledLen += (parsed_length-bytes_to_skip);
    }

    source->nFilledLen -= parsed_length;
    source->nOffset += parsed_length;

    return 1;
}


int frame_parse::parse_h264_nallength (OMX_BUFFERHEADERTYPE *source,
        OMX_BUFFERHEADERTYPE *dest ,
        OMX_U32 *partialframe)
{
    OMX_U8 *pdest = NULL,*psource = NULL;
    OMX_U32 dest_len =0, source_len = 0, temp_len = 0,parsed_length = 0;

    if (source == NULL || dest == NULL || partialframe == NULL) {
        return -1;
    }

    /*Calculate the length's*/
    dest_len = dest->nAllocLen - (dest->nFilledLen + dest->nOffset);
    source_len = source->nFilledLen;

    if (dest_len < 4 || nal_length == 0) {
        DEBUG_PRINT_LOW("FrameParser: NAL Parsing Error! dest_len %u "
                "nal_length %u", (unsigned int)dest_len, nal_length);
        return -1;
    }

    if (source_len == 0 ) {
        if (source->nFlags & OMX_BUFFERFLAG_EOS) {
            DEBUG_PRINT_LOW("FrameParser: EOS rxd for nallength!!"
                " Notify it as a complete frame");
            *partialframe = 0;
            return 1;
        } else {
            DEBUG_PRINT_ERROR("FrameParser: NAL Parsing Error!"
                "Buffer recieved with source_len = %u and with"
                "flags %u", (unsigned int)source_len, (unsigned int)source->nFlags);
            return -1;
        }
    }

    *partialframe = 1;
    temp_len = (source_len < dest_len)?source_len:dest_len;
    psource = source->pBuffer + source->nOffset;
    pdest = dest->pBuffer + (dest->nFilledLen + dest->nOffset);

    /* Find the Bytes to Accumalte*/
    if (state_nal == NAL_LENGTH_ACC) {
        while (parsed_length < temp_len ) {
            bytes_tobeparsed |= (((OMX_U32)(*psource))) << (((nal_length-accum_length-1) << 3));

            /*COPY THE DATA FOR C-SIM TO BE REOMVED ON TARGET*/
            //*pdest = *psource;
            accum_length++;
            source->nFilledLen--;
            source->nOffset++;
            psource++;
            //dest->nFilledLen++;
            //pdest++;
            parsed_length++;

            if (accum_length == nal_length) {
                accum_length = 0;
                state_nal = NAL_PARSING;
                memcpy (pdest,H264_start_code,4);
                dest->nFilledLen += 4;
                break;
            }
        }
    }

    dest_len = dest->nAllocLen - (dest->nFilledLen + dest->nOffset);
    source_len = source->nFilledLen;
    temp_len = (source_len < dest_len)?source_len:dest_len;

    psource = source->pBuffer + source->nOffset;
    pdest = dest->pBuffer + (dest->nFilledLen + dest->nOffset);

    dest->nTimeStamp = source->nTimeStamp;
    dest->nFlags = source->nFlags;

    /*Already in Parsing state go ahead and copy*/
    if (state_nal == NAL_PARSING && temp_len > 0) {
        if (temp_len < bytes_tobeparsed) {
            memcpy (pdest,psource,temp_len);
            dest->nFilledLen += temp_len;
            source->nOffset += temp_len;
            source->nFilledLen -= temp_len;
            bytes_tobeparsed -= temp_len;
        } else {
            memcpy (pdest,psource,bytes_tobeparsed);
            temp_len -= bytes_tobeparsed;
            dest->nFilledLen += bytes_tobeparsed;
            source->nOffset += bytes_tobeparsed;
            source->nFilledLen -= bytes_tobeparsed;
            bytes_tobeparsed = 0;
        }
    }

    if (bytes_tobeparsed == 0 && state_nal == NAL_PARSING) {
        *partialframe = 0;
        state_nal = NAL_LENGTH_ACC;
    }

    return 1;
}

void frame_parse::flush ()
{
    parse_state = A0;
    state_nal = NAL_LENGTH_ACC;
    accum_length = 0;
    bytes_tobeparsed = 0;
    header_found = false;
    skip_frame_boundary = false;
}

void frame_parse::parse_additional_start_code(OMX_U8 *psource,
        OMX_U32 *parsed_length)
{

    if (((start_code == MPEG4_start_code) ||
                (start_code == MPEG2_start_code)) &&
            psource &&
            parsed_length) {
        OMX_U32 index = *parsed_length;

        if ((start_code == MPEG4_start_code &&
                    (psource [index] & 0xF0) == 0x20) ||
                (start_code == MPEG2_start_code &&
                 psource [index] == 0xB3)) {
            if (header_found) {
                last_byte = psource [index];
                index++;
                parse_state = A4;
            } else
                header_found = true;
        }

        *parsed_length = index;
    }
}

void frame_parse::check_skip_frame_boundary(OMX_U32 *partialframe)
{
    if ((start_code == MPEG4_start_code ||
                start_code == MPEG2_start_code) &&
            partialframe) {

        *partialframe = 1;

        if (!skip_frame_boundary)
            *partialframe = 0;

        skip_frame_boundary = false;
    }
}

void frame_parse::update_skip_frame()
{
    if (((start_code == MPEG4_start_code) &&
                ((last_byte & 0xF0) == 0x20)) ||
            ((start_code == MPEG2_start_code) &&
             (last_byte == 0xB3))) {

        skip_frame_boundary = true;
    }
}
