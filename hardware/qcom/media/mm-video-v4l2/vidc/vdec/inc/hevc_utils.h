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


#ifndef HEVC_UTILS_H
#define HEVC_UTILS_H

/*========================================================================

                                 O p e n M M
         U t i l i t i e s   a n d   H e l p e r   R o u t i n e s

*//** @file HEVC_Utils.h
This module contains H264 video decoder utilities and helper routines.

*//*====================================================================== */

/* =======================================================================

                     INCLUDE FILES FOR MODULE

========================================================================== */
#include <stdio.h>
#include <utils/Log.h>
#include "Map.h"
#include "qtypes.h"
#include "OMX_Core.h"
#include "OMX_QCOMExtns.h"


class HEVC_Utils
{
public:
    HEVC_Utils();
    ~HEVC_Utils();

	enum {
		NAL_UNIT_CODED_SLICE_TRAIL_N,	// 0
		NAL_UNIT_CODED_SLICE_TRAIL_R,	// 1
		NAL_UNIT_CODED_SLICE_TSA_N,		// 2
		NAL_UNIT_CODED_SLICE_TLA,		// 3
		NAL_UNIT_CODED_SLICE_STSA_N,    // 4
		NAL_UNIT_CODED_SLICE_STSA_R,	// 5
		NAL_UNIT_CODED_SLICE_RADL_N,	// 6
		NAL_UNIT_CODED_SLICE_DLP,		// 7
		NAL_UNIT_CODED_SLICE_RASL_N,	// 8
		NAL_UNIT_CODED_SLICE_TFD,		// 9
		NAL_UNIT_RESERVED_10,
		NAL_UNIT_RESERVED_11,
		NAL_UNIT_RESERVED_12,
		NAL_UNIT_RESERVED_13,
		NAL_UNIT_RESERVED_14,
		NAL_UNIT_RESERVED_15,
		NAL_UNIT_CODED_SLICE_BLA,		// 16
		NAL_UNIT_CODED_SLICE_BLANT,		// 17
		NAL_UNIT_CODED_SLICE_BLA_N_LP,	// 18
		NAL_UNIT_CODED_SLICE_IDR,		// 19
		NAL_UNIT_CODED_SLICE_IDR_N_LP,	// 20
		NAL_UNIT_CODED_SLICE_CRA,		// 21
		NAL_UNIT_RESERVED_22,
		NAL_UNIT_RESERVED_23,
		NAL_UNIT_RESERVED_24,
		NAL_UNIT_RESERVED_25,
		NAL_UNIT_RESERVED_26,
		NAL_UNIT_RESERVED_27,

		NAL_UNIT_RESERVED_28,
		NAL_UNIT_RESERVED_29,
		NAL_UNIT_RESERVED_30,
		NAL_UNIT_RESERVED_31,

		NAL_UNIT_VPS,					// 32
		NAL_UNIT_SPS,					// 33
		NAL_UNIT_PPS,					// 34
		NAL_UNIT_ACCESS_UNIT_DELIMITER, // 35
		NAL_UNIT_EOS,					// 36
		NAL_UNIT_EOB,					// 37
		NAL_UNIT_FILLER_DATA,			// 38
		NAL_UNIT_SEI,					// 39 Prefix SEI
		NAL_UNIT_SEI_SUFFIX,			// 40 Suffix SEI

		NAL_UNIT_RESERVED_41,
		NAL_UNIT_RESERVED_42,
		NAL_UNIT_RESERVED_43,
		NAL_UNIT_RESERVED_44,
		NAL_UNIT_RESERVED_45,
		NAL_UNIT_RESERVED_46,
		NAL_UNIT_RESERVED_47,
		NAL_UNIT_UNSPECIFIED_48,
		NAL_UNIT_UNSPECIFIED_49,
		NAL_UNIT_UNSPECIFIED_50,
		NAL_UNIT_UNSPECIFIED_51,
		NAL_UNIT_UNSPECIFIED_52,
		NAL_UNIT_UNSPECIFIED_53,
		NAL_UNIT_UNSPECIFIED_54,
		NAL_UNIT_UNSPECIFIED_55,
		NAL_UNIT_UNSPECIFIED_56,
		NAL_UNIT_UNSPECIFIED_57,
		NAL_UNIT_UNSPECIFIED_58,
		NAL_UNIT_UNSPECIFIED_59,
		NAL_UNIT_UNSPECIFIED_60,
		NAL_UNIT_UNSPECIFIED_61,
		NAL_UNIT_UNSPECIFIED_62,
		NAL_UNIT_UNSPECIFIED_63,
		NAL_UNIT_INVALID,
	};


    void initialize_frame_checking_environment();
    bool isNewFrame(OMX_BUFFERHEADERTYPE *p_buf_hdr,
                    OMX_IN OMX_U32 size_of_nal_length_field,
                    OMX_OUT OMX_BOOL &isNewFrame);

private:

    bool              m_forceToStichNextNAL;
    bool              m_au_data;
    uint32 nalu_type;
};

#endif /* HEVC_UTILS_H */
