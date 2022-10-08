/*--------------------------------------------------------------------------
Copyright (c) 2010-2012, Code Aurora Forum. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of Code Aurora nor
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

#include "extra_data_handler.h"

extra_data_handler::extra_data_handler()
{
   rbsp_buf = (OMX_U8 *) calloc(1,100);
   memset(&frame_packing_arrangement,0,sizeof(frame_packing_arrangement));
   frame_packing_arrangement.cancel_flag = 1;
   pack_sei = false;
   sei_payload_type = -1;
}

extra_data_handler::~extra_data_handler()
{
  if(rbsp_buf) {
    free(rbsp_buf);
    rbsp_buf = NULL;
  }
}

OMX_U32 extra_data_handler::d_u(OMX_U32 num_bits)
{
  OMX_U32 rem_bits = num_bits, bins = 0, shift = 0;

  while(rem_bits >= bit_ptr) {
    DEBUG_PRINT_LOW("\nIn %s() bit_ptr/byte_ptr :%d/%d/%x", __func__, bit_ptr,
      byte_ptr, rbsp_buf[byte_ptr]);
    bins <<= shift;
    shift = (8-bit_ptr);
    bins |= ((rbsp_buf[byte_ptr] << shift) & 0xFF) >> shift;
    rem_bits -= bit_ptr;
    bit_ptr = 8;
    byte_ptr ++;
  }
  DEBUG_PRINT_LOW("\nIn %s() bit_ptr/byte_ptr :%d/%d/%x", __func__, bit_ptr,
    byte_ptr, rbsp_buf[byte_ptr]);

  if (rem_bits) {
    bins <<= rem_bits;
    bins |= ((rbsp_buf[byte_ptr] << (8-bit_ptr)) & 0xFF) >> (8-rem_bits);
    bit_ptr -= rem_bits;
	if (bit_ptr == 0) {
       bit_ptr = 8;
	   byte_ptr++;
	}
  }
  DEBUG_PRINT_LOW("\nIn %s() bit_ptr/byte_ptr :%d/%d/%x", __func__, bit_ptr,
    byte_ptr, rbsp_buf[byte_ptr]);

  DEBUG_PRINT_LOW("\nIn %s() bin/num_bits : %x/%d", __func__, bins, num_bits);
  return bins;
}

OMX_U32 extra_data_handler::d_ue()
{
    OMX_S32 lead_zeros = -1;
    OMX_U32 symbol, bit;
    do{
        bit = d_u(1);
        lead_zeros++;
    }while (!bit);

    symbol = ((1 << lead_zeros) - 1) + d_u(lead_zeros);

    DEBUG_PRINT_LOW("\nIn %s() symbol : %d", __func__,symbol);
    return symbol;
}

OMX_U32 extra_data_handler::parse_frame_pack(OMX_U32 payload_size)
{
  frame_packing_arrangement.id = d_ue();
  frame_packing_arrangement.cancel_flag = d_u(1);
  if(!frame_packing_arrangement.cancel_flag) {
     frame_packing_arrangement.type = d_u(7);
     frame_packing_arrangement.quincunx_sampling_flag = d_u(1);
     frame_packing_arrangement.content_interpretation_type = d_u(6);
     frame_packing_arrangement.spatial_flipping_flag = d_u(1);
     frame_packing_arrangement.frame0_flipped_flag = d_u(1);
     frame_packing_arrangement.field_views_flag = d_u(1);
     frame_packing_arrangement.current_frame_is_frame0_flag = d_u(1);
     frame_packing_arrangement.frame0_self_contained_flag = d_u(1);
     frame_packing_arrangement.frame1_self_contained_flag = d_u(1);

     if(!frame_packing_arrangement.quincunx_sampling_flag &&
        frame_packing_arrangement.type != 5) {
        frame_packing_arrangement.frame0_grid_position_x = d_u(4);
        frame_packing_arrangement.frame0_grid_position_y = d_u(4);
        frame_packing_arrangement.frame1_grid_position_x = d_u(4);
        frame_packing_arrangement.frame1_grid_position_y = d_u(4);
     }
     frame_packing_arrangement.reserved_byte = d_u(8);
     frame_packing_arrangement.repetition_period = d_ue();
   }
   frame_packing_arrangement.extension_flag = d_u(1);

   return 1;
}

OMX_S32 extra_data_handler::parse_rbsp(OMX_U8 *buf, OMX_U32 len)
{
   OMX_U32 i = 3, j=0, startcode;
   OMX_U32 nal_unit_type, nal_ref_idc, forbidden_zero_bit;

   bit_ptr  = 8;
   byte_ptr = 0;

   startcode =  buf[0] << 16 | buf[1] <<8 | buf[2];

   if (!startcode) {
       startcode |= buf[i++];
   }
   if(startcode != H264_START_CODE) {
       DEBUG_PRINT_ERROR("\nERROR: In %s() Start code not found", __func__);
       return -1;
   }
   forbidden_zero_bit = (buf[i] & 0x80) >>7;
   if(forbidden_zero_bit) {
       DEBUG_PRINT_ERROR("\nERROR: In %s() Non-zero forbidden bit", __func__);
       return -1;
   }
   nal_ref_idc = (buf[i] & 0x60) >>5;
   DEBUG_PRINT_LOW("\nIn %s() nal_ref_idc ; %d", __func__, nal_ref_idc);

   nal_unit_type = (buf[i++] & 0x1F);

   while(i<len) {
     if(!(buf[i] + buf[i+1]) && (buf[i+2] == H264_EMULATION_BYTE) &&
		(i+2 < len)) {
        rbsp_buf[j++] = buf[i++];
        rbsp_buf[j++] = buf[i++];
        i++;
     } else
        rbsp_buf[j++] = buf[i++];
   }
   return nal_unit_type;
}
OMX_S32 extra_data_handler::parse_sei(OMX_U8 *buffer, OMX_U32 buffer_length)
{
  OMX_U32 nal_unit_type, payload_type = 0, payload_size = 0;
  OMX_U32 marker = 0, pad = 0xFF;

  nal_unit_type = parse_rbsp(buffer, buffer_length);

  if (nal_unit_type != NAL_TYPE_SEI) {
     DEBUG_PRINT_ERROR("\nERROR: In %s() - Non SEI NAL ", __func__);
     return -1;
  } else {

    while(rbsp_buf[byte_ptr] == 0xFF)
      payload_type += rbsp_buf[byte_ptr++];
    payload_type += rbsp_buf[byte_ptr++];

    DEBUG_PRINT_LOW("\nIn %s() payload_type : %u", __func__, payload_type);

    while(rbsp_buf[byte_ptr] == 0xFF)
      payload_size += rbsp_buf[byte_ptr++];
    payload_size += rbsp_buf[byte_ptr++];

    DEBUG_PRINT_LOW("\nIn %s() payload_size : %u", __func__, payload_size);

    switch(payload_type) {
      case SEI_PAYLOAD_FRAME_PACKING_ARRANGEMENT:
        DEBUG_PRINT_LOW("\nIn %s() Frame Packing SEI ", __func__);
        parse_frame_pack(payload_size);
      break;
      default:
        DEBUG_PRINT_LOW("\nINFO: In %s() Not Supported SEI NAL ", __func__);
      break;
    }
  }
  if(bit_ptr != 8) {
    marker = d_u(1);
    if(marker) {
      if(bit_ptr != 8) {
	 pad = d_u(bit_ptr);
	 if(pad) {
	   DEBUG_PRINT_ERROR("\nERROR: In %s() padding Bits Error in SEI",
	     __func__);
           return -1;
	 }
      }
    } else {
      DEBUG_PRINT_ERROR("\nERROR: In %s() Marker Bit Error in SEI",
        __func__);
        return -1;
    }
  }
  DEBUG_PRINT_LOW("\nIn %s() payload_size : %u/%u", __func__,
    payload_size, byte_ptr);
  return 1;
}
/*======================================================================
  Slice Information will be available as below (each line is of 4 bytes)
  | number of slices |
  | 1st slice offset |
  | 1st slice size   |
  | ..               |
  | Nth slice offset |
  | Nth slice size   |
======================================================================*/
OMX_S32 extra_data_handler::parse_sliceinfo(
  OMX_BUFFERHEADERTYPE *pBufHdr, OMX_OTHER_EXTRADATATYPE *pExtra)
{
  OMX_U32 slice_offset = 0, slice_size = 0, total_size = 0;
  OMX_U8 *pBuffer = (OMX_U8 *)pBufHdr->pBuffer;
  OMX_U32 *data = (OMX_U32 *)pExtra->data;
  OMX_U32 num_slices = *data;
  DEBUG_PRINT_HIGH("number of slices = %d", num_slices);
  if ((4 + num_slices * 8) != (OMX_U32)pExtra->nDataSize) {
    DEBUG_PRINT_ERROR("unknown error in slice info extradata");
    return -1;
  }
  for (int i = 0; i < num_slices; i++) {
    slice_offset = (OMX_U32)(*(data + (i*2 + 1)));
    if ((*(pBuffer + slice_offset + 0) != 0x00) ||
        (*(pBuffer + slice_offset + 1) != 0x00) ||
        (*(pBuffer + slice_offset + 2) != 0x00) ||
        (*(pBuffer + slice_offset + 3) != H264_START_CODE)) {
      DEBUG_PRINT_ERROR("found 0x%x instead of start code at offset[%d] "
        "for slice[%d]", (OMX_U32)(*(OMX_U32 *)(pBuffer + slice_offset)),
        slice_offset, i);
      return -1;
    }
    if (slice_offset != total_size) {
      DEBUG_PRINT_ERROR("offset of slice number %d is not correct "
          "or previous slice size is not correct", i);
      return -1;
    }
    slice_size = (OMX_U32)(*(data + (i*2 + 2)));
    total_size += slice_size;
    DEBUG_PRINT_HIGH("slice number %d offset/size = %d/%d",
        i, slice_offset, slice_size);
  }
  if (pBufHdr->nFilledLen != total_size) {
    DEBUG_PRINT_ERROR("frame_size[%d] is not equal to "
       "total slices size[%d]", pBufHdr->nFilledLen, total_size);
    return -1;
  }
  return 0;
}

OMX_U32 extra_data_handler::parse_extra_data(OMX_BUFFERHEADERTYPE *buf_hdr)
{
  OMX_OTHER_EXTRADATATYPE *extra_data = (OMX_OTHER_EXTRADATATYPE *)
    ((unsigned)(buf_hdr->pBuffer + buf_hdr->nOffset +
    buf_hdr->nFilledLen + 3)&(~3));

  DEBUG_PRINT_LOW("\nIn %s() symbol : %u", __func__,buf_hdr->nFlags);

  if (buf_hdr->nFlags & OMX_BUFFERFLAG_EXTRADATA) {
    while(extra_data && (OMX_U8*)extra_data < (buf_hdr->pBuffer +
      buf_hdr->nAllocLen) && extra_data->eType != VDEC_EXTRADATA_NONE) {
      DEBUG_PRINT_LOW("\nExtra data type(%x) Extra data size(%u)",
        extra_data->eType, extra_data->nDataSize);
      if (extra_data->eType == VDEC_EXTRADATA_SEI) {
         parse_sei(extra_data->data, extra_data->nDataSize);
      }
      else if (extra_data->eType == VEN_EXTRADATA_QCOMFILLER) {
         DEBUG_PRINT_HIGH("Extradata Qcom Filler found, skip %d bytes",
            extra_data->nSize);
      }
      else if (extra_data->eType == VEN_EXTRADATA_SLICEINFO) {
         DEBUG_PRINT_HIGH("Extradata SliceInfo of size %d found, "
            "parsing it", extra_data->nDataSize);
         parse_sliceinfo(buf_hdr, extra_data);
      }
      extra_data = (OMX_OTHER_EXTRADATATYPE *) (((OMX_U8 *) extra_data) +
        extra_data->nSize);
    }
  }
  return 1;
}

OMX_U32 extra_data_handler::get_frame_pack_data(
	OMX_QCOM_FRAME_PACK_ARRANGEMENT *frame_pack)
{
   DEBUG_PRINT_LOW("\n%s:%d get frame data", __func__, __LINE__);
   memcpy(&frame_pack->id,&frame_packing_arrangement.id,
   FRAME_PACK_SIZE*sizeof(OMX_U32));
   return 1;
}

OMX_U32 extra_data_handler::set_frame_pack_data(OMX_QCOM_FRAME_PACK_ARRANGEMENT
   *frame_pack)
{
   DEBUG_PRINT_LOW("\n%s:%d set frame data", __func__, __LINE__);
   memcpy(&frame_packing_arrangement.id, &frame_pack->id,
     FRAME_PACK_SIZE*sizeof(OMX_U32));
   pack_sei = true;
   sei_payload_type = SEI_PAYLOAD_FRAME_PACKING_ARRANGEMENT;
   return 1;
}

OMX_U32 extra_data_handler::e_u(OMX_U32 symbol, OMX_U32 num_bits)
{
   OMX_U32 rem_bits = num_bits, shift;

   DEBUG_PRINT_LOW("\n%s bin  : %x/%d", __func__, symbol, num_bits);

   while(rem_bits >= bit_ptr) {
     shift = rem_bits - bit_ptr;
     rbsp_buf[byte_ptr] |= (symbol >> shift);
     symbol = (symbol << (32 - shift)) >> (32 - shift);
     rem_bits -= bit_ptr;
     DEBUG_PRINT_LOW("\n%sstream byte/rem_bits %x/%d", __func__,
     rbsp_buf[byte_ptr], rem_bits);
     byte_ptr ++;
     bit_ptr = 8;
   }

   if(rem_bits) {
     shift = bit_ptr - rem_bits;
     rbsp_buf[byte_ptr] |= (symbol << shift);
     bit_ptr -= rem_bits;
     DEBUG_PRINT_LOW("\n%s 2 stream byte/rem_bits %x", __func__,
       rbsp_buf[byte_ptr], rem_bits);
     if(bit_ptr == 0) {
       bit_ptr = 8;
       byte_ptr++;
     }
   }
   return 1;
}

OMX_U32 extra_data_handler::e_ue(OMX_U32 symbol)
{
   OMX_U32 i, sym_len, sufix_len, info;
   OMX_U32 nn =(symbol + 1) >> 1;

   DEBUG_PRINT_LOW("\n%s bin  : %x", __func__, symbol);

   for(i=0; i < 33 && nn != 0; i++)
     nn >>= 1;

   sym_len = ((i << 1) + 1);
   info = symbol + 1 - (1 << i);
   sufix_len = (1 << (sym_len >>1));
   info = sufix_len | (info & (sufix_len - 1));
   e_u(info, sym_len);
   return 1;
}

OMX_U32 extra_data_handler::create_frame_pack()
{
   e_ue(frame_packing_arrangement.id);
   e_u(frame_packing_arrangement.cancel_flag, 1);
   if(!frame_packing_arrangement.cancel_flag) {
     e_u(frame_packing_arrangement.type, 7);
     e_u(frame_packing_arrangement.quincunx_sampling_flag, 1);
     e_u(frame_packing_arrangement.content_interpretation_type, 6);
     e_u(frame_packing_arrangement.spatial_flipping_flag, 1);
     e_u(frame_packing_arrangement.frame0_flipped_flag, 1);
     e_u(frame_packing_arrangement.field_views_flag, 1);
     e_u(frame_packing_arrangement.current_frame_is_frame0_flag, 1);
     e_u(frame_packing_arrangement.frame0_self_contained_flag, 1);
     e_u(frame_packing_arrangement.frame1_self_contained_flag, 1);
     if(!frame_packing_arrangement.quincunx_sampling_flag &&
        frame_packing_arrangement.type != 5) {
        e_u(frame_packing_arrangement.frame0_grid_position_x, 4);
        e_u(frame_packing_arrangement.frame0_grid_position_y, 4);
        e_u(frame_packing_arrangement.frame1_grid_position_x, 4);
        e_u(frame_packing_arrangement.frame1_grid_position_y, 4);
     }
     e_u(frame_packing_arrangement.reserved_byte, 8);
     e_ue(frame_packing_arrangement.repetition_period);
   }
   e_u(frame_packing_arrangement.extension_flag, 1);
   return 1;
}

OMX_S32 extra_data_handler::create_rbsp(OMX_U8 *buf, OMX_U32 nalu_type)
{
   OMX_U32 i, j = 7;
   for(i = 0;i < 3;i++)
   *buf++ = 0x00;
   *buf++ = H264_START_CODE;
   *buf++ = nalu_type;
   *buf++ = (sei_payload_type & 0x000000FF);
   *buf++ = byte_ptr - 1; //payload will contain 1 byte of rbsp_trailing_bits
                          //that shouldn't be taken into account

    for(i = 0;i < byte_ptr ;i += 2) {
      *buf++ = rbsp_buf[i];
       j++;
       if(i+1 < byte_ptr) {
           *buf++ = rbsp_buf[i+1];
           j++;
           if(!(rbsp_buf[i] + rbsp_buf[i+1])) {
               *buf++ = H264_EMULATION_BYTE;
               j++;
           }
       }
    }

    DEBUG_PRINT_LOW("\n%s rbsp length %d", __func__, j);
    return j;
}

OMX_U32 extra_data_handler::create_sei(OMX_U8 *buffer)
{
   OMX_U32 i, ret_val = 0;

   byte_ptr = 0;
   bit_ptr  = 8;

   if(sei_payload_type == SEI_PAYLOAD_FRAME_PACKING_ARRANGEMENT) {
     create_frame_pack();

     if(bit_ptr != 8) {
       e_u(1,1);
       if(bit_ptr != 8)
         e_u(0,bit_ptr);
     }

     //Payload will have been byte aligned by now,
     //insert the rbsp trailing bits
     e_u(1, 1);
     e_u(0, 7);

     ret_val = create_rbsp(buffer, NAL_TYPE_SEI);
   }

   pack_sei = false;
   sei_payload_type = -1;

   return ret_val;
}

OMX_U32 extra_data_handler::create_extra_data(OMX_BUFFERHEADERTYPE *buf_hdr)
{
   OMX_U8 *buffer = (OMX_U8 *) ((unsigned)(buf_hdr->pBuffer +
     buf_hdr->nOffset + buf_hdr->nFilledLen));
   OMX_U32 msg_size;

   DEBUG_PRINT_LOW("\n filled_len/orig_len %d/%d", buf_hdr->nFilledLen,
     buf_hdr->nAllocLen);

    if(buf_hdr->nFlags & OMX_BUFFERFLAG_CODECCONFIG) {
      DEBUG_PRINT_LOW("\n%s:%d create extra data with config", __func__,
        __LINE__);
      if(pack_sei) {
         msg_size = create_sei(buffer);
	 if( msg_size > 0)
           buf_hdr->nFilledLen += msg_size;
      }
    }
    DEBUG_PRINT_LOW("\n filled_len/orig_len %d/%d", buf_hdr->nFilledLen,
      buf_hdr->nAllocLen);
    return 1;
}

