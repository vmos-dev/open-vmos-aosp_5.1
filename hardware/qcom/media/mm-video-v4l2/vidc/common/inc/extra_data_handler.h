/*--------------------------------------------------------------------------
Copyright (c) 2010-2014, The Linux Foundation. All rights reserved.

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

#ifndef __EXTRA_DATA_HANDLER_H__
#define __EXTRA_DATA_HANDLER_H__

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "OMX_QCOMExtns.h"
#include<linux/msm_vidc_dec.h>
#include<linux/msm_vidc_enc.h>


#ifdef _ANDROID_
extern "C"{
#include<utils/Log.h>
}
#endif // _ANDROID_

#include "vidc_debug.h"
#define SEI_PAYLOAD_FRAME_PACKING_ARRANGEMENT 0x2D
#define H264_START_CODE 0x01
#define NAL_TYPE_SEI 0x06
#define VDEC_OMX_SEI 0x7F000007
#define FRAME_PACK_SIZE 18
#define H264_EMULATION_BYTE 0x03
class extra_data_handler
{
    public:
        extra_data_handler();
        ~extra_data_handler();
        OMX_U32 parse_extra_data(OMX_BUFFERHEADERTYPE *buf_hdr);
        OMX_U32 create_extra_data(OMX_BUFFERHEADERTYPE *buf_hdr);
        OMX_U32 get_frame_pack_data(OMX_QCOM_FRAME_PACK_ARRANGEMENT *frame_pack);
        OMX_U32 set_frame_pack_data(OMX_QCOM_FRAME_PACK_ARRANGEMENT *frame_pack);
    private:
        OMX_QCOM_FRAME_PACK_ARRANGEMENT frame_packing_arrangement;
        OMX_U8 *rbsp_buf;
        OMX_U32 bit_ptr;
        OMX_U32 byte_ptr;
        OMX_U32 pack_sei;
        OMX_U32 sei_payload_type;
        OMX_U32 d_u(OMX_U32 num_bits);
        OMX_U32 d_ue();
        OMX_U32 parse_frame_pack(void);
        OMX_S32 parse_rbsp(OMX_U8 *buf, OMX_U32 len);
        OMX_S32 parse_sei(OMX_U8 *buffer, OMX_U32 buffer_length);
        OMX_U32 e_u(OMX_U32 symbol, OMX_U32 num_bits);
        OMX_U32 e_ue(OMX_U32 symbol);
        OMX_U32 create_frame_pack();
        OMX_S32 create_rbsp(OMX_U8 *buf, OMX_U32 nalu_type);
        OMX_U32 create_sei(OMX_U8 *buffer);
        OMX_S32 parse_sliceinfo(OMX_BUFFERHEADERTYPE *pBufHdr,
                OMX_OTHER_EXTRADATATYPE *pExtra);
        OMX_S32 parse_ltrinfo(OMX_OTHER_EXTRADATATYPE *pExtra);
};

#endif
