/*--------------------------------------------------------------------------
Copyright (c) 2013, The Linux Foundation. All rights reserved.

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

/*========================================================================

                      O p e n M M
         V i d e o   U t i l i t i e s

*//** @file VideoUtils.cpp
  This module contains utilities and helper routines.

@par EXTERNALIZED FUNCTIONS

@par INITIALIZATION AND SEQUENCING REQUIREMENTS
  (none)

*//*====================================================================== */

/* =======================================================================

                     INCLUDE FILES FOR MODULE

========================================================================== */
#include "hevc_utils.h"
#include "vidc_debug.h"
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/time.h>
#ifdef _ANDROID_
#include <cutils/properties.h>
#endif


/* =======================================================================

                DEFINITIONS AND DECLARATIONS FOR MODULE

This section contains definitions for constants, macros, types, variables
and other items needed by this module.

========================================================================== */

HEVC_Utils::HEVC_Utils()
{
    initialize_frame_checking_environment();
}

HEVC_Utils::~HEVC_Utils()
{
}

/***********************************************************************/
/*
FUNCTION:
HEVC_Utils::initialize_frame_checking_environment

DESCRIPTION:
Extract RBSP data from a NAL

INPUT/OUTPUT PARAMETERS:
None

RETURN VALUE:
boolean

SIDE EFFECTS:
None.
 */
/***********************************************************************/
void HEVC_Utils::initialize_frame_checking_environment()
{
    m_forceToStichNextNAL = false;
    m_au_data = false;
    nalu_type = NAL_UNIT_INVALID;
}

/*===========================================================================
FUNCTION:
HEVC_Utils::iSNewFrame

DESCRIPTION:
Returns true if NAL parsing successfull otherwise false.

INPUT/OUTPUT PARAMETERS:
<In>
buffer : buffer containing start code or nal length + NAL units
buffer_length : the length of the NAL buffer
start_code : If true, start code is detected,
otherwise size nal length is detected
size_of_nal_length_field: size of nal length field
<out>
isNewFrame: true if the NAL belongs to a differenet frame
false if the NAL belongs to a current frame

RETURN VALUE:
boolean  true, if nal parsing is successful
false, if the nal parsing has errors

SIDE EFFECTS:
None.
===========================================================================*/
bool HEVC_Utils::isNewFrame(OMX_BUFFERHEADERTYPE *p_buf_hdr,
        OMX_IN OMX_U32 size_of_nal_length_field,
        OMX_OUT OMX_BOOL &isNewFrame)
{
    OMX_IN OMX_U8 *buffer = p_buf_hdr->pBuffer;
    OMX_IN OMX_U32 buffer_length = p_buf_hdr->nFilledLen;
    byte bFirstSliceInPic = 0;

    byte coef1=1, coef2=0, coef3=0;
    uint32 pos = 0;
    uint32 nal_len = buffer_length;
    uint32 sizeofNalLengthField = 0;
    uint32 zero_count;
    boolean start_code = (size_of_nal_length_field==0)?true:false;

    if (start_code) {
        // Search start_code_prefix_one_3bytes (0x000001)
        coef2 = buffer[pos++];
        coef3 = buffer[pos++];

        do {
            if (pos >= buffer_length) {
                DEBUG_PRINT_ERROR("ERROR: In %s() - line %d", __func__, __LINE__);
                return false;
            }

            coef1 = coef2;
            coef2 = coef3;
            coef3 = buffer[pos++];
        } while (coef1 || coef2 || coef3 != 1);
    } else if (size_of_nal_length_field) {
        /* This is the case to play multiple NAL units inside each access unit*/
        /* Extract the NAL length depending on sizeOfNALength field */
        sizeofNalLengthField = size_of_nal_length_field;
        nal_len = 0;

        while (size_of_nal_length_field--) {
            nal_len |= buffer[pos++]<<(size_of_nal_length_field<<3);
        }

        if (nal_len >= buffer_length) {
            DEBUG_PRINT_ERROR("ERROR: In %s() - line %d", __func__, __LINE__);
            return false;
        }
    }

    if (nal_len > buffer_length) {
        DEBUG_PRINT_ERROR("ERROR: In %s() - line %d", __func__, __LINE__);
        return false;
    }

    if (pos + 2 > (nal_len + sizeofNalLengthField)) {
        DEBUG_PRINT_ERROR("ERROR: In %s() - line %d", __func__, __LINE__);
        return false;
    }

    nalu_type = (buffer[pos] & 0x7E)>>1 ;      //=== nal_unit_type

    DEBUG_PRINT_LOW("@#@# Pos = %x NalType = %x buflen = %u", pos-1, nalu_type, (unsigned int) buffer_length);

    isNewFrame =  OMX_FALSE;

    if (nalu_type == NAL_UNIT_VPS ||
            nalu_type == NAL_UNIT_SPS ||
            nalu_type == NAL_UNIT_PPS ||
            nalu_type == NAL_UNIT_SEI) {
        DEBUG_PRINT_LOW("Non-AU boundary with NAL type %d", nalu_type);

        if (m_au_data) {
            isNewFrame = OMX_TRUE;
            m_au_data = false;
        }

        m_forceToStichNextNAL = true;
    } else if (nalu_type <= NAL_UNIT_RESERVED_23) {
        DEBUG_PRINT_LOW("AU Boundary with NAL type %d ", nalu_type);

        if (!m_forceToStichNextNAL) {
            bFirstSliceInPic = ((buffer[pos+2] & 0x80)>>7);

            if (bFirstSliceInPic) {    //=== first_ctb_in_slice is only 1'b1  coded tree block
                DEBUG_PRINT_LOW("Found a New Frame due to 1st coded tree block");
                isNewFrame = OMX_TRUE;
            }
        }

        m_au_data = true;
        m_forceToStichNextNAL = false;
    }

    DEBUG_PRINT_LOW("get_HEVC_nal_type - newFrame value %d",isNewFrame);
    return true;
}

