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
#include "h264_utils.h"
#include "extra_data_handler.h"
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/time.h>
#ifdef _ANDROID_
#include <cutils/properties.h>
    extern "C"{
        #include<utils/Log.h>
    }

#endif

/* =======================================================================

                DEFINITIONS AND DECLARATIONS FOR MODULE

This section contains definitions for constants, macros, types, variables
and other items needed by this module.

========================================================================== */

#define SIZE_NAL_FIELD_MAX  4
#define BASELINE_PROFILE 66
#define MAIN_PROFILE     77
#define HIGH_PROFILE     100

#define MAX_SUPPORTED_LEVEL 32

RbspParser::RbspParser (const uint8 *_begin, const uint8 *_end)
: begin (_begin), end(_end), pos (- 1), bit (0),
cursor (0xFFFFFF), advanceNeeded (true)
{
}

// Destructor
/*lint -e{1540}  Pointer member neither freed nor zeroed by destructor
 * No problem
 */
RbspParser::~RbspParser () {}

// Return next RBSP byte as a word
uint32 RbspParser::next ()
{
    if (advanceNeeded) advance ();
    //return static_cast<uint32> (*pos);
    return static_cast<uint32> (begin[pos]);
}

// Advance RBSP decoder to next byte
void RbspParser::advance ()
{
    ++pos;
    //if (pos >= stop)
    if (begin + pos == end)
    {
        /*lint -e{730}  Boolean argument to function
         * I don't see a problem here
         */
        //throw false;
        ALOGV("H264Parser-->NEED TO THROW THE EXCEPTION...\n");
    }
    cursor <<= 8;
    //cursor |= static_cast<uint32> (*pos);
    cursor |= static_cast<uint32> (begin[pos]);
    if ((cursor & 0xFFFFFF) == 0x000003)
    {
        advance ();
    }
    advanceNeeded = false;
}

// Decode unsigned integer
uint32 RbspParser::u (uint32 n)
{
    uint32 i, s, x = 0;
    for (i = 0; i < n; i += s)
    {
        s = static_cast<uint32>STD_MIN(static_cast<int>(8 - bit),
            static_cast<int>(n - i));
        x <<= s;

        x |= ((next () >> ((8 - static_cast<uint32>(bit)) - s)) &
            ((1 << s) - 1));

        bit = (bit + s) % 8;
        if (!bit)
        {
            advanceNeeded = true;
        }
    }
    return x;
}

// Decode unsigned integer Exp-Golomb-coded syntax element
uint32 RbspParser::ue ()
{
    int leadingZeroBits = -1;
    for (uint32 b = 0; !b; ++leadingZeroBits)
    {
        b = u (1);
    }
    return ((1 << leadingZeroBits) - 1) +
        u (static_cast<uint32>(leadingZeroBits));
}

// Decode signed integer Exp-Golomb-coded syntax element
int32 RbspParser::se ()
{
    const uint32 x = ue ();
    if (!x) return 0;
    else if (x & 1) return static_cast<int32> ((x >> 1) + 1);
    else return - static_cast<int32> (x >> 1);
}

void H264_Utils::allocate_rbsp_buffer(uint32 inputBufferSize)
{
    m_rbspBytes = (byte *) calloc(1,inputBufferSize);
    m_prv_nalu.nal_ref_idc = 0;
    m_prv_nalu.nalu_type = NALU_TYPE_UNSPECIFIED;
}

H264_Utils::H264_Utils(): m_height(0),
                          m_width(0),
                          m_rbspBytes(NULL),
                          m_au_data (false)
{
    initialize_frame_checking_environment();
}

H264_Utils::~H264_Utils()
{
/*  if(m_pbits)
  {
    delete(m_pbits);
    m_pbits = NULL;
  }
*/
  if (m_rbspBytes)
  {
    free(m_rbspBytes);
    m_rbspBytes = NULL;
  }
}

/***********************************************************************/
/*
FUNCTION:
  H264_Utils::initialize_frame_checking_environment

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
void H264_Utils::initialize_frame_checking_environment()
{
  m_forceToStichNextNAL = false;
  m_au_data = false;
  m_prv_nalu.nal_ref_idc = 0;
  m_prv_nalu.nalu_type = NALU_TYPE_UNSPECIFIED;
}

/***********************************************************************/
/*
FUNCTION:
  H264_Utils::extract_rbsp

DESCRIPTION:
  Extract RBSP data from a NAL

INPUT/OUTPUT PARAMETERS:
  <In>
    buffer : buffer containing start code or nal length + NAL units
    buffer_length : the length of the NAL buffer
    start_code : If true, start code is detected,
                 otherwise size nal length is detected
    size_of_nal_length_field: size of nal length field

  <Out>
    rbsp_bistream : extracted RBSP bistream
    rbsp_length : the length of the RBSP bitstream
    nal_unit : decoded NAL header information

RETURN VALUE:
  boolean

SIDE EFFECTS:
  None.
*/
/***********************************************************************/

boolean H264_Utils::extract_rbsp(OMX_IN   OMX_U8  *buffer,
                                 OMX_IN   OMX_U32 buffer_length,
                                 OMX_IN   OMX_U32 size_of_nal_length_field,
                                 OMX_OUT  OMX_U8  *rbsp_bistream,
                                 OMX_OUT  OMX_U32 *rbsp_length,
                                 OMX_OUT  NALU    *nal_unit)
{
  byte coef1, coef2, coef3;
  uint32 pos = 0;
  uint32 nal_len = buffer_length;
  uint32 sizeofNalLengthField = 0;
  uint32 zero_count;
  boolean eRet = true;
  boolean start_code = (size_of_nal_length_field==0)?true:false;

  if(start_code) {
    // Search start_code_prefix_one_3bytes (0x000001)
    coef2 = buffer[pos++];
    coef3 = buffer[pos++];
    do {
      if(pos >= buffer_length)
      {
        ALOGE("ERROR: In %s() - line %d", __func__, __LINE__);
        return false;
      }

      coef1 = coef2;
      coef2 = coef3;
      coef3 = buffer[pos++];
    } while(coef1 || coef2 || coef3 != 1);
  }
  else if (size_of_nal_length_field)
  {
    /* This is the case to play multiple NAL units inside each access unit*/
    /* Extract the NAL length depending on sizeOfNALength field */
    sizeofNalLengthField = size_of_nal_length_field;
    nal_len = 0;
    while(size_of_nal_length_field--)
    {
      nal_len |= buffer[pos++]<<(size_of_nal_length_field<<3);
    }
    if (nal_len >= buffer_length)
    {
      ALOGE("ERROR: In %s() - line %d", __func__, __LINE__);
      return false;
    }
  }

  if (nal_len > buffer_length)
  {
    ALOGE("ERROR: In %s() - line %d", __func__, __LINE__);
    return false;
  }
  if(pos + 1 > (nal_len + sizeofNalLengthField))
  {
    ALOGE("ERROR: In %s() - line %d", __func__, __LINE__);
    return false;
  }
  if (nal_unit->forbidden_zero_bit = (buffer[pos] & 0x80))
  {
    ALOGE("ERROR: In %s() - line %d", __func__, __LINE__);
  }
  nal_unit->nal_ref_idc   = (buffer[pos] & 0x60) >> 5;
  nal_unit->nalu_type = buffer[pos++] & 0x1f;
  ALOGV("\n@#@# Pos = %x NalType = %x buflen = %d",
      pos-1, nal_unit->nalu_type, buffer_length);
  *rbsp_length = 0;


  if( nal_unit->nalu_type == NALU_TYPE_EOSEQ ||
      nal_unit->nalu_type == NALU_TYPE_EOSTREAM)
    return (nal_len + sizeofNalLengthField);

  zero_count = 0;
  while (pos < (nal_len+sizeofNalLengthField))    //similar to for in p-42
   {
    if( zero_count == 2 ) {
      if( buffer[pos] == 0x03 ) {
        pos ++;
        zero_count = 0;
        continue;
      }
      if( buffer[pos] <= 0x01 ) {
        if( start_code ) {
          *rbsp_length -= 2;
          pos -= 2;
          return pos;
        }
      }
      zero_count = 0;
    }
    zero_count ++;
    if( buffer[pos] != 0 )
      zero_count = 0;

    rbsp_bistream[(*rbsp_length)++] = buffer[pos++];
  }

  return eRet;
}

/*===========================================================================
FUNCTION:
  H264_Utils::iSNewFrame

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
bool H264_Utils::isNewFrame(OMX_BUFFERHEADERTYPE *p_buf_hdr,
                            OMX_IN OMX_U32 size_of_nal_length_field,
                            OMX_OUT OMX_BOOL &isNewFrame)
{
    NALU nal_unit;
    uint16 first_mb_in_slice = 0;
    OMX_IN OMX_U32 numBytesInRBSP = 0;
    OMX_IN OMX_U8 *buffer = p_buf_hdr->pBuffer;
    OMX_IN OMX_U32 buffer_length = p_buf_hdr->nFilledLen;
    bool eRet = true;

    ALOGV("isNewFrame: buffer %p buffer_length %d "
        "size_of_nal_length_field %d\n", buffer, buffer_length,
        size_of_nal_length_field);

    if ( false == extract_rbsp(buffer, buffer_length, size_of_nal_length_field,
                               m_rbspBytes, &numBytesInRBSP, &nal_unit) )
    {
        ALOGE("ERROR: In %s() - extract_rbsp() failed", __func__);
        isNewFrame = OMX_FALSE;
        eRet = false;
    }
    else
    {
      nalu_type = nal_unit.nalu_type;
      switch (nal_unit.nalu_type)
      {
        case NALU_TYPE_IDR:
        case NALU_TYPE_NON_IDR:
        {
          ALOGV("\n AU Boundary with NAL type %d ",nal_unit.nalu_type);
          if (m_forceToStichNextNAL)
          {
            isNewFrame = OMX_FALSE;
          }
          else
          {
            RbspParser rbsp_parser(m_rbspBytes, (m_rbspBytes+numBytesInRBSP));
            first_mb_in_slice = rbsp_parser.ue();

            if((!first_mb_in_slice) || /*(slice.prv_frame_num != slice.frame_num ) ||*/
               ( (m_prv_nalu.nal_ref_idc != nal_unit.nal_ref_idc) && ( nal_unit.nal_ref_idc * m_prv_nalu.nal_ref_idc == 0 ) ) ||
               /*( ((m_prv_nalu.nalu_type == NALU_TYPE_IDR) && (nal_unit.nalu_type == NALU_TYPE_IDR)) && (slice.idr_pic_id != slice.prv_idr_pic_id) ) || */
               ( (m_prv_nalu.nalu_type != nal_unit.nalu_type ) && ((m_prv_nalu.nalu_type == NALU_TYPE_IDR) || (nal_unit.nalu_type == NALU_TYPE_IDR)) ) )
            {
              //ALOGV("Found a New Frame due to NALU_TYPE_IDR/NALU_TYPE_NON_IDR");
              isNewFrame = OMX_TRUE;
            }
            else
            {
              isNewFrame = OMX_FALSE;
            }
          }
          m_au_data = true;
          m_forceToStichNextNAL = false;
          break;
        }
        case NALU_TYPE_SPS:
        case NALU_TYPE_PPS:
        case NALU_TYPE_SEI:
        {
          ALOGV("\n Non-AU boundary with NAL type %d", nal_unit.nalu_type);
          if(m_au_data)
          {
            isNewFrame = OMX_TRUE;
            m_au_data = false;
          }
          else
          {
            isNewFrame =  OMX_FALSE;
          }

          m_forceToStichNextNAL = true;
          break;
        }
        case NALU_TYPE_ACCESS_DELIM:
        case NALU_TYPE_UNSPECIFIED:
        case NALU_TYPE_EOSEQ:
        case NALU_TYPE_EOSTREAM:
        default:
        {
          isNewFrame =  OMX_FALSE;
          // Do not update m_forceToStichNextNAL
          break;
        }
      } // end of switch
    } // end of if
    m_prv_nalu = nal_unit;
    ALOGV("get_h264_nal_type - newFrame value %d\n",isNewFrame);
    return eRet;
}

void perf_metrics::start()
{
  if (!active)
  {
    start_time = get_act_time();
    active = true;
  }
}

void perf_metrics::stop()
{
  OMX_U64 stop_time = get_act_time();
  if (active)
  {
    proc_time += (stop_time - start_time);
    active = false;
  }
}

void perf_metrics::end(OMX_U32 units_cntr)
{
  stop();
  ALOGV("--> Processing time : [%.2f] Sec", (float)proc_time / 1e6);
  if (units_cntr)
  {
    ALOGV("--> Avrg proc time  : [%.2f] mSec", proc_time / (float)(units_cntr * 1e3));
  }
}

void perf_metrics::reset()
{
  start_time = 0;
  proc_time = 0;
  active = false;
}

OMX_U64 perf_metrics::get_act_time()
{
  struct timeval act_time = {0, 0};
  gettimeofday(&act_time, NULL);
  return (act_time.tv_usec + act_time.tv_sec * 1e6);
}

OMX_U64 perf_metrics::processing_time_us()
{
  return proc_time;
}

h264_stream_parser::h264_stream_parser()
{
  reset();
#ifdef PANSCAN_HDLR
  panscan_hdl = new panscan_handler();
  if (!panscan_hdl)
  {
    ALOGE("ERROR: Panscan hdl was not allocated!");
  }
  else if (!panscan_hdl->initialize(10))
  {
    ALOGE("ERROR: Allocating memory for panscan!");
    delete panscan_hdl;
    panscan_hdl = NULL;
  }
#else
  memset(&panscan_param, 0, sizeof(panscan_param));
  panscan_param.rect_id = NO_PAN_SCAN_BIT;
#endif
}

h264_stream_parser::~h264_stream_parser()
{
#ifdef PANSCAN_HDLR
  if (panscan_hdl)
  {
    delete panscan_hdl;
    panscan_hdl = NULL;
  }
#endif
}

void h264_stream_parser::reset()
{
  curr_32_bit = 0;
  bits_read = 0;
  zero_cntr = 0;
  emulation_code_skip_cntr = 0;
  emulation_sc_enabled = true;
  bitstream = NULL;
  bitstream_bytes = 0;
  memset(&vui_param, 0, sizeof(vui_param));
  vui_param.fixed_fps_prev_ts = LLONG_MAX;
  memset(&sei_buf_period, 0, sizeof(sei_buf_period));
  memset(&sei_pic_timing, 0, sizeof(sei_pic_timing));
  memset(&frame_packing_arrangement,0,sizeof(frame_packing_arrangement));
  frame_packing_arrangement.cancel_flag = 1;
  mbaff_flag = 0;
}

void h264_stream_parser::init_bitstream(OMX_U8* data, OMX_U32 size)
{
  bitstream = data;
  bitstream_bytes = size;
  curr_32_bit = 0;
  bits_read = 0;
  zero_cntr = 0;
  emulation_code_skip_cntr = 0;
}

void h264_stream_parser::parse_vui(bool vui_in_extradata)
{
  OMX_U32 value = 0;
  ALOGV("parse_vui: IN");
  if (vui_in_extradata)
    while (!extract_bits(1) && more_bits()); // Discard VUI enable flag
  if (!more_bits())
    return;

  vui_param.aspect_ratio_info_present_flag = extract_bits(1); //aspect_ratio_info_present_flag
  if (vui_param.aspect_ratio_info_present_flag)
  {
      ALOGV("Aspect Ratio Info present!");
      aspect_ratio_info();
  }

  if (extract_bits(1)) //overscan_info_present_flag
    extract_bits(1); //overscan_appropriate_flag
  if (extract_bits(1)) //video_signal_type_present_flag
  {
    extract_bits(3); //video_format
    extract_bits(1); //video_full_range_flag
    if (extract_bits(1)) //colour_description_present_flag
    {
      extract_bits(8); //colour_primaries
      extract_bits(8); //transfer_characteristics
      extract_bits(8); //matrix_coefficients
    }
  }
  if (extract_bits(1)) //chroma_location_info_present_flag
  {
    uev(); //chroma_sample_loc_type_top_field
    uev(); //chroma_sample_loc_type_bottom_field
  }
  vui_param.timing_info_present_flag = extract_bits(1);
  if (vui_param.timing_info_present_flag)
  {
    vui_param.num_units_in_tick = extract_bits(32);
    vui_param.time_scale = extract_bits(32);
    vui_param.fixed_frame_rate_flag = extract_bits(1);
    ALOGV("Timing info present in VUI!");
    ALOGV("  num units in tick  : %u", vui_param.num_units_in_tick);
    ALOGV("  time scale         : %u", vui_param.time_scale);
    ALOGV("  fixed frame rate   : %u", vui_param.fixed_frame_rate_flag);
  }
  vui_param.nal_hrd_parameters_present_flag = extract_bits(1);
  if (vui_param.nal_hrd_parameters_present_flag)
  {
    ALOGV("nal hrd params present!");
    hrd_parameters(&vui_param.nal_hrd_parameters);
  }
  vui_param.vcl_hrd_parameters_present_flag = extract_bits(1);
  if (vui_param.vcl_hrd_parameters_present_flag)
  {
    ALOGV("vcl hrd params present!");
    hrd_parameters(&vui_param.vcl_hrd_parameters);
  }
  if (vui_param.nal_hrd_parameters_present_flag ||
      vui_param.vcl_hrd_parameters_present_flag)
    vui_param.low_delay_hrd_flag = extract_bits(1);
  vui_param.pic_struct_present_flag = extract_bits(1);
  ALOGV("pic_struct_present_flag : %u", vui_param.pic_struct_present_flag);
  if (extract_bits(1)) //bitstream_restriction_flag
  {
    extract_bits(1); //motion_vectors_over_pic_boundaries_flag
    uev(); //max_bytes_per_pic_denom
    uev(); //max_bits_per_mb_denom
    uev(); //log2_max_mv_length_vertical
    uev(); //log2_max_mv_length_horizontal
    uev(); //num_reorder_frames
    uev(); //max_dec_frame_buffering
  }
  ALOGV("parse_vui: OUT");
}

void h264_stream_parser::aspect_ratio_info()
{
  ALOGV("aspect_ratio_info: IN");
  OMX_U32  aspect_ratio_idc = 0;
  OMX_U32  aspect_ratio_x = 0;
  OMX_U32  aspect_ratio_y = 0;
  aspect_ratio_idc = extract_bits(8); //aspect_ratio_idc
  switch (aspect_ratio_idc)
  {
    case 1:
      aspect_ratio_x = 1;
      aspect_ratio_y = 1;
      break;
    case 2:
      aspect_ratio_x = 12;
      aspect_ratio_y = 11;
      break;
    case 3:
      aspect_ratio_x = 10;
      aspect_ratio_y = 11;
      break;
    case 4:
      aspect_ratio_x = 16;
      aspect_ratio_y = 11;
      break;
    case 5:
      aspect_ratio_x = 40;
      aspect_ratio_y = 33;
      break;
    case 6:
      aspect_ratio_x = 24;
      aspect_ratio_y = 11;
      break;
    case 7:
      aspect_ratio_x = 20;
      aspect_ratio_y = 11;
      break;
    case 8:
      aspect_ratio_x = 32;
      aspect_ratio_y = 11;
      break;
    case 9:
      aspect_ratio_x = 80;
      aspect_ratio_y = 33;
      break;
    case 10:
      aspect_ratio_x = 18;
      aspect_ratio_y = 11;
      break;
    case 11:
      aspect_ratio_x = 15;
      aspect_ratio_y = 11;
      break;
    case 12:
      aspect_ratio_x = 64;
      aspect_ratio_y = 33;
      break;
    case 13:
      aspect_ratio_x = 160;
      aspect_ratio_y = 99;
      break;
    case 14:
      aspect_ratio_x = 4;
      aspect_ratio_y = 3;
      break;
    case 15:
      aspect_ratio_x = 3;
      aspect_ratio_y = 2;
      break;
    case 16:
      aspect_ratio_x = 2;
      aspect_ratio_y = 1;
      break;
    case 255:
      aspect_ratio_x = extract_bits(16); //sar_width
      aspect_ratio_y = extract_bits(16); //sar_height
      break;
    default:
      ALOGV("-->aspect_ratio_idc: Reserved Value ");
      break;
  }
  ALOGV("-->aspect_ratio_idc        : %u", aspect_ratio_idc);
  ALOGV("-->aspect_ratio_x          : %u", aspect_ratio_x);
  ALOGV("-->aspect_ratio_y          : %u", aspect_ratio_y);
  vui_param.aspect_ratio_info.aspect_ratio_idc = aspect_ratio_idc;
  vui_param.aspect_ratio_info.aspect_ratio_x = aspect_ratio_x;
  vui_param.aspect_ratio_info.aspect_ratio_y = aspect_ratio_y;
  ALOGV("aspect_ratio_info: OUT");
}

void h264_stream_parser::hrd_parameters(h264_hrd_param *hrd_param)
{
  int idx;
  ALOGV("hrd_parameters: IN");
  hrd_param->cpb_cnt = uev() + 1;
  hrd_param->bit_rate_scale = extract_bits(4);
  hrd_param->cpb_size_scale = extract_bits(4);
  ALOGV("-->cpb_cnt        : %u", hrd_param->cpb_cnt);
  ALOGV("-->bit_rate_scale : %u", hrd_param->bit_rate_scale);
  ALOGV("-->cpb_size_scale : %u", hrd_param->cpb_size_scale);
  if (hrd_param->cpb_cnt > MAX_CPB_COUNT)
  {
    ALOGV("ERROR: Invalid hrd_param->cpb_cnt [%u]!", hrd_param->cpb_cnt);
    return;
  }
  for (idx = 0; idx < hrd_param->cpb_cnt && more_bits(); idx++)
  {
    hrd_param->bit_rate_value[idx] = uev() + 1;
    hrd_param->cpb_size_value[idx] = uev() + 1;
    hrd_param->cbr_flag[idx] = extract_bits(1);
    ALOGV("-->bit_rate_value [%d] : %u", idx, hrd_param->bit_rate_value[idx]);
    ALOGV("-->cpb_size_value [%d] : %u", idx, hrd_param->cpb_size_value[idx]);
    ALOGV("-->cbr_flag       [%d] : %u", idx, hrd_param->cbr_flag[idx]);
  }
  hrd_param->initial_cpb_removal_delay_length = extract_bits(5) + 1;
  hrd_param->cpb_removal_delay_length = extract_bits(5) + 1;
  hrd_param->dpb_output_delay_length = extract_bits(5) + 1;
  hrd_param->time_offset_length = extract_bits(5);
  ALOGV("-->initial_cpb_removal_delay_length : %u", hrd_param->initial_cpb_removal_delay_length);
  ALOGV("-->cpb_removal_delay_length         : %u", hrd_param->cpb_removal_delay_length);
  ALOGV("-->dpb_output_delay_length          : %u", hrd_param->dpb_output_delay_length);
  ALOGV("-->time_offset_length               : %u", hrd_param->time_offset_length);
  ALOGV("hrd_parameters: OUT");
}

void h264_stream_parser::parse_sei()
{
  OMX_U32 value = 0, processed_bytes = 0;
  OMX_U8 *sei_msg_start = bitstream;
  OMX_U32 sei_unit_size = bitstream_bytes;
  ALOGV("@@parse_sei: IN sei_unit_size(%u)", sei_unit_size);
  while ((processed_bytes + 2) < sei_unit_size && more_bits())
  {
    init_bitstream(sei_msg_start + processed_bytes, sei_unit_size - processed_bytes);
    ALOGV("-->NALU_TYPE_SEI");
    OMX_U32 payload_type = 0, payload_size = 0, aux = 0;
    do {
      value = extract_bits(8);
      payload_type += value;
      processed_bytes++;
    } while (value == 0xFF);
    ALOGV("-->payload_type   : %u", payload_type);
    do {
      value = extract_bits(8);
      payload_size += value;
      processed_bytes++;
    } while (value == 0xFF);
    ALOGV("-->payload_size   : %u", payload_size);
    if (payload_size > 0)
    {
      switch (payload_type)
      {
        case BUFFERING_PERIOD:
          sei_buffering_period();
        break;
        case PIC_TIMING:
          sei_picture_timing();
        break;
        case PAN_SCAN_RECT:
          sei_pan_scan();
        break;
        case SEI_PAYLOAD_FRAME_PACKING_ARRANGEMENT:
          parse_frame_pack();
        break;
        default:
          ALOGV("-->SEI payload type [%u] not implemented! size[%u]", payload_type, payload_size);
      }
    }
    processed_bytes += (payload_size + emulation_code_skip_cntr);
    ALOGV("-->SEI processed_bytes[%u]", processed_bytes);
  }
  ALOGV("@@parse_sei: OUT");
}

void h264_stream_parser::sei_buffering_period()
{
  int idx;
  OMX_U32 value = 0;
  h264_hrd_param *hrd_param = NULL;
  ALOGV("@@sei_buffering_period: IN");
  value = uev(); // seq_parameter_set_id
  ALOGV("-->seq_parameter_set_id : %u", value);
  if (value > 31)
  {
    ALOGV("ERROR: Invalid seq_parameter_set_id [%u]!", value);
    return;
  }
  sei_buf_period.is_valid = false;
  if (vui_param.nal_hrd_parameters_present_flag)
  {
    hrd_param = &vui_param.nal_hrd_parameters;
    if (hrd_param->cpb_cnt > MAX_CPB_COUNT)
    {
      ALOGV("ERROR: Invalid hrd_param->cpb_cnt [%u]!", hrd_param->cpb_cnt);
      return;
    }
    for (idx = 0; idx < hrd_param->cpb_cnt ; idx++)
    {
      sei_buf_period.is_valid = true;
      sei_buf_period.initial_cpb_removal_delay[idx] = extract_bits(hrd_param->initial_cpb_removal_delay_length);
      sei_buf_period.initial_cpb_removal_delay_offset[idx] = extract_bits(hrd_param->initial_cpb_removal_delay_length);
      ALOGV("-->initial_cpb_removal_delay        : %u", sei_buf_period.initial_cpb_removal_delay[idx]);
      ALOGV("-->initial_cpb_removal_delay_offset : %u", sei_buf_period.initial_cpb_removal_delay_offset[idx]);
    }
  }
  if (vui_param.vcl_hrd_parameters_present_flag)
  {
    hrd_param = &vui_param.vcl_hrd_parameters;
    if (hrd_param->cpb_cnt > MAX_CPB_COUNT)
    {
      ALOGV("ERROR: Invalid hrd_param->cpb_cnt [%u]!", hrd_param->cpb_cnt);
      return;
    }
    for (idx = 0; idx < hrd_param->cpb_cnt ; idx++)
    {
      sei_buf_period.is_valid = true;
      sei_buf_period.initial_cpb_removal_delay[idx] = extract_bits(hrd_param->initial_cpb_removal_delay_length);
      sei_buf_period.initial_cpb_removal_delay_offset[idx] = extract_bits(hrd_param->initial_cpb_removal_delay_length);
      ALOGV("-->initial_cpb_removal_delay        : %u", sei_buf_period.initial_cpb_removal_delay[idx]);
      ALOGV("-->initial_cpb_removal_delay_offset : %u", sei_buf_period.initial_cpb_removal_delay_offset[idx]);
    }
  }
  sei_buf_period.au_cntr = 0;
  ALOGV("@@sei_buffering_period: OUT");
}

void h264_stream_parser::sei_picture_timing()
{
  ALOGV("@@sei_picture_timing: IN");
  OMX_U32 time_offset_len = 0, cpb_removal_len = 24, dpb_output_len  = 24;
  OMX_U8 cbr_flag = 0;
  sei_pic_timing.is_valid = true;
  if (vui_param.nal_hrd_parameters_present_flag)
  {
    cpb_removal_len = vui_param.nal_hrd_parameters.cpb_removal_delay_length;
    dpb_output_len = vui_param.nal_hrd_parameters.dpb_output_delay_length;
    time_offset_len = vui_param.nal_hrd_parameters.time_offset_length;
    cbr_flag = vui_param.nal_hrd_parameters.cbr_flag[0];
  }
  else if (vui_param.vcl_hrd_parameters_present_flag)
  {
    cpb_removal_len = vui_param.vcl_hrd_parameters.cpb_removal_delay_length;
    dpb_output_len = vui_param.vcl_hrd_parameters.dpb_output_delay_length;
    time_offset_len = vui_param.vcl_hrd_parameters.time_offset_length;
    cbr_flag = vui_param.vcl_hrd_parameters.cbr_flag[0];
  }
  sei_pic_timing.cpb_removal_delay = extract_bits(cpb_removal_len);
  sei_pic_timing.dpb_output_delay = extract_bits(dpb_output_len);
  ALOGV("-->cpb_removal_len : %u", cpb_removal_len);
  ALOGV("-->dpb_output_len  : %u", dpb_output_len);
  ALOGV("-->cpb_removal_delay : %u", sei_pic_timing.cpb_removal_delay);
  ALOGV("-->dpb_output_delay  : %u", sei_pic_timing.dpb_output_delay);
  if (vui_param.pic_struct_present_flag)
  {
    sei_pic_timing.pic_struct = extract_bits(4);
    sei_pic_timing.num_clock_ts = 0;
    switch (sei_pic_timing.pic_struct)
    {
      case 0: case 1: case 2: sei_pic_timing.num_clock_ts = 1; break;
      case 3: case 4: case 7: sei_pic_timing.num_clock_ts = 2; break;
      case 5: case 6: case 8: sei_pic_timing.num_clock_ts = 3; break;
      default:
        ALOGE("sei_picture_timing: pic_struct invalid!");
    }
    ALOGV("-->num_clock_ts      : %u", sei_pic_timing.num_clock_ts);
    for (int i = 0; i < sei_pic_timing.num_clock_ts && more_bits(); i++)
    {
      sei_pic_timing.clock_ts_flag = extract_bits(1);
      if(sei_pic_timing.clock_ts_flag)
      {
        ALOGV("-->clock_timestamp present!");
        sei_pic_timing.ct_type = extract_bits(2);
        sei_pic_timing.nuit_field_based_flag = extract_bits(1);
        sei_pic_timing.counting_type = extract_bits(5);
        sei_pic_timing.full_timestamp_flag = extract_bits(1);
        sei_pic_timing.discontinuity_flag = extract_bits(1);
        sei_pic_timing.cnt_dropped_flag = extract_bits(1);
        sei_pic_timing.n_frames = extract_bits(8);
        ALOGV("-->f_timestamp_flg   : %u", sei_pic_timing.full_timestamp_flag);
        ALOGV("-->n_frames          : %u", sei_pic_timing.n_frames);
        sei_pic_timing.seconds_value = 0;
        sei_pic_timing.minutes_value = 0;
        sei_pic_timing.hours_value = 0;
        if (sei_pic_timing.full_timestamp_flag)
        {
          sei_pic_timing.seconds_value = extract_bits(6);
          sei_pic_timing.minutes_value = extract_bits(6);
          sei_pic_timing.hours_value = extract_bits(5);
        }
        else if (extract_bits(1))
        {
          ALOGV("-->seconds_flag enabled!");
          sei_pic_timing.seconds_value = extract_bits(6);
          if (extract_bits(1))
          {
            ALOGV("-->minutes_flag enabled!");
            sei_pic_timing.minutes_value = extract_bits(6);
            if (extract_bits(1))
            {
              ALOGV("-->hours_flag enabled!");
              sei_pic_timing.hours_value = extract_bits(5);
            }
          }
        }
        sei_pic_timing.time_offset = 0;
        if (time_offset_len > 0)
          sei_pic_timing.time_offset = iv(time_offset_len);
        ALOGV("-->seconds_value     : %u", sei_pic_timing.seconds_value);
        ALOGV("-->minutes_value     : %u", sei_pic_timing.minutes_value);
        ALOGV("-->hours_value       : %u", sei_pic_timing.hours_value);
        ALOGV("-->time_offset       : %d", sei_pic_timing.time_offset);
      }
    }
  }
  ALOGV("@@sei_picture_timing: OUT");
}

void h264_stream_parser::sei_pan_scan()
{
#ifdef _ANDROID_
  char property_value[PROPERTY_VALUE_MAX] = {0};
  OMX_S32 enable_panscan_log = 0;
  property_get("vidc.dec.debug.panframedata", property_value, "0");
  enable_panscan_log = atoi(property_value);
#endif
#ifdef PANSCAN_HDLR
  h264_pan_scan *pan_scan_param = panscan_hdl->get_free();
#else
  h264_pan_scan *pan_scan_param = &panscan_param;
#endif

  if (!pan_scan_param)
  {
    ALOGE("sei_pan_scan: ERROR: Invalid pointer!");
    return;
  }

  pan_scan_param->rect_id = uev();
  if (pan_scan_param->rect_id > 0xFF)
  {
    ALOGE("sei_pan_scan: ERROR: Invalid rect_id[%u]!", pan_scan_param->rect_id);
    pan_scan_param->rect_id = NO_PAN_SCAN_BIT;
    return;
  }

  pan_scan_param->rect_cancel_flag = extract_bits(1);

  if (pan_scan_param->rect_cancel_flag)
    pan_scan_param->rect_id = NO_PAN_SCAN_BIT;
  else
  {
    pan_scan_param->cnt = uev() + 1;
    if (pan_scan_param->cnt > MAX_PAN_SCAN_RECT)
    {
      ALOGE("sei_pan_scan: ERROR: Invalid num of rect [%u]!", pan_scan_param->cnt);
      pan_scan_param->rect_id = NO_PAN_SCAN_BIT;
      return;
    }

    for (int i = 0; i < pan_scan_param->cnt; i++)
    {
      pan_scan_param->rect_left_offset[i] = sev();
      pan_scan_param->rect_right_offset[i] = sev();
      pan_scan_param->rect_top_offset[i] = sev();
      pan_scan_param->rect_bottom_offset[i] = sev();

    }
    pan_scan_param->rect_repetition_period = uev();
#ifdef PANSCAN_HDLR
    if (pan_scan_param->rect_repetition_period > 1)
      // Repetition period is decreased by 2 each time panscan data is used
      pan_scan_param->rect_repetition_period *= 2;
#endif
#ifdef _ANDROID_
     if (enable_panscan_log)
     {
       print_pan_data(pan_scan_param);
     }
#endif
  }
}

void h264_stream_parser::print_pan_data(h264_pan_scan *pan_scan_param)
{
  ALOGV("@@print_pan_data: IN");

  ALOGV("-->rect_id            : %u", pan_scan_param->rect_id);
  ALOGV("-->rect_cancel_flag   : %u", pan_scan_param->rect_cancel_flag);

  ALOGV("-->cnt                : %u", pan_scan_param->cnt);

  for (int i = 0; i < pan_scan_param->cnt; i++)
  {
    ALOGV("-->rect_left_offset   : %d", pan_scan_param->rect_left_offset[i]);
    ALOGV("-->rect_right_offset  : %d", pan_scan_param->rect_right_offset[i]);
    ALOGV("-->rect_top_offset    : %d", pan_scan_param->rect_top_offset[i]);
    ALOGV("-->rect_bottom_offset : %d", pan_scan_param->rect_bottom_offset[i]);
  }
  ALOGV("-->repetition_period  : %u", pan_scan_param->rect_repetition_period);

  ALOGV("@@print_pan_data: OUT");
}

void h264_stream_parser::parse_sps()
{
  OMX_U32 value = 0, scaling_matrix_limit;
  ALOGV("@@parse_sps: IN");
  value = extract_bits(8); //profile_idc
  extract_bits(8); //constraint flags and reserved bits
  extract_bits(8); //level_idc
  uev(); //sps id
  if (value == 100 || value == 110 || value == 122 || value == 244 ||
      value ==  44 || value ==  83 || value ==  86 || value == 118)
  {
    if (uev() == 3) //chroma_format_idc
    {
      extract_bits(1); //separate_colour_plane_flag
      scaling_matrix_limit = 12;
    }
    else
      scaling_matrix_limit = 12;
    uev(); //bit_depth_luma_minus8
    uev(); //bit_depth_chroma_minus8
    extract_bits(1); //qpprime_y_zero_transform_bypass_flag
    if (extract_bits(1)) //seq_scaling_matrix_present_flag
      for (int i = 0; i < scaling_matrix_limit && more_bits(); i++)
      {
        if (extract_bits(1)) ////seq_scaling_list_present_flag[ i ]
          if (i < 6)
            scaling_list(16);
          else
            scaling_list(64);
      }
  }
  uev(); //log2_max_frame_num_minus4
  value = uev(); //pic_order_cnt_type
  if (value == 0)
    uev(); //log2_max_pic_order_cnt_lsb_minus4
  else if (value == 1)
  {
    extract_bits(1); //delta_pic_order_always_zero_flag
    sev(); //offset_for_non_ref_pic
    sev(); //offset_for_top_to_bottom_field
    value = uev(); // num_ref_frames_in_pic_order_cnt_cycle
    for (int i = 0; i < value; i++)
      sev(); //offset_for_ref_frame[ i ]
  }
  uev(); //max_num_ref_frames
  extract_bits(1); //gaps_in_frame_num_value_allowed_flag
  value = uev(); //pic_width_in_mbs_minus1
  value = uev(); //pic_height_in_map_units_minus1
  if (!extract_bits(1)) //frame_mbs_only_flag
    mbaff_flag = extract_bits(1); //mb_adaptive_frame_field_flag
  extract_bits(1); //direct_8x8_inference_flag
  if (extract_bits(1)) //frame_cropping_flag
  {
    uev(); //frame_crop_left_offset
    uev(); //frame_crop_right_offset
    uev(); //frame_crop_top_offset
    uev(); //frame_crop_bottom_offset
  }
  if (extract_bits(1)) //vui_parameters_present_flag
    parse_vui(false);
  ALOGV("@@parse_sps: OUT");
}

void h264_stream_parser::scaling_list(OMX_U32 size_of_scaling_list)
{
  OMX_S32 last_scale = 8, next_scale = 8, delta_scale;
  for (int j = 0; j < size_of_scaling_list; j++)
  {
    if (next_scale != 0)
    {
      delta_scale = sev();
      next_scale = (last_scale + delta_scale + 256) % 256;
    }
    last_scale = (next_scale == 0)? last_scale : next_scale;
  }
}

OMX_U32 h264_stream_parser::extract_bits(OMX_U32 n)
{
  OMX_U32 value = 0;
  if (n > 32)
  {
    ALOGE("ERROR: extract_bits limit to 32 bits!");
    return value;
  }
  value = curr_32_bit >> (32 - n);
  if (bits_read < n)
  {
    n -= bits_read;
    read_word();
    value |= (curr_32_bit >> (32 - n));
    if (bits_read < n)
    {
      ALOGV("ERROR: extract_bits underflow!");
      value >>= (n - bits_read);
      n = bits_read;
    }
  }
  bits_read -= n;
  curr_32_bit <<= n;
  return value;
}

void h264_stream_parser::read_word()
{
  curr_32_bit = 0;
  bits_read = 0;
  while (bitstream_bytes && bits_read < 32)
  {
    if (*bitstream == EMULATION_PREVENTION_THREE_BYTE &&
        zero_cntr >= 2 && emulation_sc_enabled)
    {
      ALOGV("EMULATION_PREVENTION_THREE_BYTE: Skip 0x03 byte aligned!");
      emulation_code_skip_cntr++;
    }
    else
    {
      curr_32_bit <<= 8;
      curr_32_bit |= *bitstream;
      bits_read += 8;
    }
    if (*bitstream == 0)
      zero_cntr++;
    else
      zero_cntr = 0;
    bitstream++;
    bitstream_bytes--;
  }
  curr_32_bit <<= (32 - bits_read);
}

OMX_U32 h264_stream_parser::uev()
{
  OMX_U32 lead_zero_bits = 0, code_num = 0;
  while(!extract_bits(1) && more_bits())
    lead_zero_bits++;
  code_num = lead_zero_bits == 0 ? 0 :
    (1 << lead_zero_bits) - 1 + extract_bits(lead_zero_bits);
  return code_num;
}

bool h264_stream_parser::more_bits()
{
  return (bitstream_bytes > 0 || bits_read > 0);
}

OMX_S32 h264_stream_parser::sev()
{
  OMX_U32 code_num = uev();
  OMX_S32 ret;
  ret = (code_num + 1) >> 1;
  return ((code_num & 1) ? ret : -ret);
}

OMX_S32 h264_stream_parser::iv(OMX_U32 n_bits)
{
  OMX_U32 code_num = extract_bits(n_bits);
  OMX_S32 ret = (code_num >> (n_bits - 1))? (-1)*(~(code_num & ~(0x1 << (n_bits - 1))) + 1) : code_num;
  return ret;
}

OMX_U32 h264_stream_parser::get_nal_unit_type(OMX_U32 *nal_unit_type)
{
  OMX_U32 value = 0, consumed_bytes = 3;
  *nal_unit_type = NALU_TYPE_UNSPECIFIED;
  ALOGV("-->get_nal_unit_type: IN");
  value = extract_bits(24);
  while (value != 0x00000001 && more_bits())
  {
    value <<= 8;
    value |= extract_bits(8);
    consumed_bytes++;
  }
  if (value != 0x00000001)
  {
    ALOGE("ERROR in get_nal_unit_type: Start code not found!");
  }
  else
  {
    if (extract_bits(1)) // forbidden_zero_bit
    {
      ALOGE("WARNING: forbidden_zero_bit should be zero!");
    }
    value = extract_bits(2);
    ALOGV("-->nal_ref_idc    : %x", value);
    *nal_unit_type = extract_bits(5);
    ALOGV("-->nal_unit_type  : %x", *nal_unit_type);
    consumed_bytes++;
    if (consumed_bytes > 5)
    {
      ALOGE("-->WARNING: Startcode was found after the first 4 bytes!");
    }
  }
  ALOGV("-->get_nal_unit_type: OUT");
  return consumed_bytes;
}

OMX_S64 h264_stream_parser::calculate_buf_period_ts(OMX_S64 timestamp)
{
  OMX_S64 clock_ts = timestamp;
  ALOGV("calculate_ts(): IN");
  if (sei_buf_period.au_cntr == 0)
    clock_ts = sei_buf_period.reference_ts = timestamp;
  else if (sei_pic_timing.is_valid && VALID_TS(sei_buf_period.reference_ts))
  {
    clock_ts = sei_buf_period.reference_ts + sei_pic_timing.cpb_removal_delay *
               1e6 * vui_param.num_units_in_tick / vui_param.time_scale;
  }
  sei_buf_period.au_cntr++;
  ALOGV("calculate_ts(): OUT");
  return clock_ts;
}

OMX_S64 h264_stream_parser::calculate_fixed_fps_ts(OMX_S64 timestamp, OMX_U32 DeltaTfiDivisor)
{
  if (VALID_TS(timestamp))
    vui_param.fixed_fps_prev_ts = timestamp;
  else if (VALID_TS(vui_param.fixed_fps_prev_ts))
    vui_param.fixed_fps_prev_ts += DeltaTfiDivisor * 1e6 *
                                   vui_param.num_units_in_tick / vui_param.time_scale;
  return vui_param.fixed_fps_prev_ts;
}

void h264_stream_parser::parse_frame_pack()
{
#ifdef _ANDROID_
  char property_value[PROPERTY_VALUE_MAX] = {0};
  OMX_S32 enable_framepack_log = 0;

  property_get("vidc.dec.debug.panframedata", property_value, "0");
  enable_framepack_log = atoi(property_value);
#endif
  ALOGV("\n%s:%d parse_frame_pack", __func__, __LINE__);

  frame_packing_arrangement.id = uev();

  frame_packing_arrangement.cancel_flag = extract_bits(1);
  if(!frame_packing_arrangement.cancel_flag) {
     frame_packing_arrangement.type = extract_bits(7);
     frame_packing_arrangement.quincunx_sampling_flag = extract_bits(1);
     frame_packing_arrangement.content_interpretation_type = extract_bits(6);
     frame_packing_arrangement.spatial_flipping_flag = extract_bits(1);
     frame_packing_arrangement.frame0_flipped_flag = extract_bits(1);
     frame_packing_arrangement.field_views_flag = extract_bits(1);
     frame_packing_arrangement.current_frame_is_frame0_flag = extract_bits(1);
     frame_packing_arrangement.frame0_self_contained_flag = extract_bits(1);
     frame_packing_arrangement.frame1_self_contained_flag = extract_bits(1);

     if(!frame_packing_arrangement.quincunx_sampling_flag &&
        frame_packing_arrangement.type != 5) {
        frame_packing_arrangement.frame0_grid_position_x = extract_bits(4);
        frame_packing_arrangement.frame0_grid_position_y = extract_bits(4);
        frame_packing_arrangement.frame1_grid_position_x = extract_bits(4);
        frame_packing_arrangement.frame1_grid_position_y = extract_bits(4);
     }
     frame_packing_arrangement.reserved_byte = extract_bits(8);
     frame_packing_arrangement.repetition_period = uev();
   }
   frame_packing_arrangement.extension_flag = extract_bits(1);

#ifdef _ANDROID_
   if (enable_framepack_log)
   {
     print_frame_pack();
   }
#endif
}

void h264_stream_parser::print_frame_pack()
{
  ALOGV("\n ## frame_packing_arrangement.id = %u", frame_packing_arrangement.id);
  ALOGV("\n ## frame_packing_arrangement.cancel_flag = %u",
                       frame_packing_arrangement.cancel_flag);
  if(!frame_packing_arrangement.cancel_flag)
  {
    ALOGV("\n ## frame_packing_arrangement.type = %u",
                         frame_packing_arrangement.type);
    ALOGV("\n ## frame_packing_arrangement.quincunx_sampling_flag = %u",
                         frame_packing_arrangement.quincunx_sampling_flag);
    ALOGV("\n ## frame_packing_arrangement.content_interpretation_type = %u",
                         frame_packing_arrangement.content_interpretation_type);
    ALOGV("\n ## frame_packing_arrangement.spatial_flipping_flag = %u",
                         frame_packing_arrangement.spatial_flipping_flag);
    ALOGV("\n ## frame_packing_arrangement.frame0_flipped_flag = %u",
                         frame_packing_arrangement.frame0_flipped_flag);
    ALOGV("\n ## frame_packing_arrangement.field_views_flag = %u",
                         frame_packing_arrangement.field_views_flag);
    ALOGV("\n ## frame_packing_arrangement.current_frame_is_frame0_flag = %u",
                         frame_packing_arrangement.current_frame_is_frame0_flag);
    ALOGV("\n ## frame_packing_arrangement.frame0_self_contained_flag = %u",
                         frame_packing_arrangement.frame0_self_contained_flag);
    ALOGV("\n ## frame_packing_arrangement.frame1_self_contained_flag = %u",
                         frame_packing_arrangement.frame1_self_contained_flag);
    ALOGV("\n ## frame_packing_arrangement.reserved_byte = %u",
                         frame_packing_arrangement.reserved_byte);
    ALOGV("\n ## frame_packing_arrangement.repetition_period = %u",
                         frame_packing_arrangement.repetition_period);
    ALOGV("\n ## frame_packing_arrangement.extension_flag = %u",
                         frame_packing_arrangement.extension_flag);
  }
}
/* API'S EXPOSED TO OMX COMPONENT */

void h264_stream_parser::get_frame_pack_data(
  OMX_QCOM_FRAME_PACK_ARRANGEMENT *frame_pack)
{
   ALOGV("\n%s:%d get frame data", __func__, __LINE__);
   memcpy(&frame_pack->id,&frame_packing_arrangement.id,
   FRAME_PACK_SIZE*sizeof(OMX_U32));
   return;
}


bool h264_stream_parser::is_mbaff()
{
   ALOGV("\n%s:%d MBAFF flag=%d", __func__, __LINE__,mbaff_flag);
   return mbaff_flag;
}

void h264_stream_parser::get_frame_rate(OMX_U32 *frame_rate)
{
  if (vui_param.num_units_in_tick != 0)
    *frame_rate = vui_param.time_scale / (2 * vui_param.num_units_in_tick);
}

void h264_stream_parser::parse_nal(OMX_U8* data_ptr, OMX_U32 data_len, OMX_U32 nal_type, bool enable_emu_sc)
{
  OMX_U32 nal_unit_type = NALU_TYPE_UNSPECIFIED, cons_bytes = 0;
  ALOGV("parse_nal(): IN nal_type(%lu)", nal_type);
  if (!data_len)
    return;
  init_bitstream(data_ptr, data_len);
  emulation_sc_enabled = enable_emu_sc;
  if (nal_type != NALU_TYPE_VUI)
  {
    cons_bytes = get_nal_unit_type(&nal_unit_type);
    if (nal_type != nal_unit_type && nal_type != NALU_TYPE_UNSPECIFIED)
    {
      ALOGV("Unexpected nal_type(%x) expected(%x)", nal_unit_type, nal_type);
      return;
    }
  }
  switch (nal_type)
  {
    case NALU_TYPE_SPS:
      if (more_bits())
        parse_sps();
#ifdef PANSCAN_HDLR
      panscan_hdl->get_free();
#endif
    break;
    case NALU_TYPE_SEI:
      init_bitstream(data_ptr + cons_bytes, data_len - cons_bytes);
      parse_sei();
    break;
    case NALU_TYPE_VUI:
      parse_vui(true);
    break;
    default:
      ALOGV("nal_unit_type received : %lu", nal_type);
  }
  ALOGV("parse_nal(): OUT");
}

#ifdef PANSCAN_HDLR
void h264_stream_parser::update_panscan_data(OMX_S64 timestamp)
{
  panscan_hdl->update_last(timestamp);
}
#endif

void h264_stream_parser::fill_aspect_ratio_info(OMX_QCOM_ASPECT_RATIO *dest_aspect_ratio)
{
  if(dest_aspect_ratio && vui_param.aspect_ratio_info_present_flag)
  {
    dest_aspect_ratio->aspectRatioX = vui_param.aspect_ratio_info.aspect_ratio_x;
    dest_aspect_ratio->aspectRatioY = vui_param.aspect_ratio_info.aspect_ratio_y;
  }
}

void h264_stream_parser::fill_pan_scan_data(OMX_QCOM_PANSCAN *dest_pan_scan, OMX_S64 timestamp)
{
#ifdef PANSCAN_HDLR
  h264_pan_scan *pan_scan_param = panscan_hdl->get_populated(timestamp);
#else
  h264_pan_scan *pan_scan_param = &panscan_param;
#endif
  if (pan_scan_param)
    if (!(pan_scan_param->rect_id & NO_PAN_SCAN_BIT))
    {
      PRINT_PANSCAN_PARAM(*pan_scan_param);
      dest_pan_scan->numWindows = pan_scan_param->cnt;
      for (int i = 0; i < dest_pan_scan->numWindows; i++)
      {
        dest_pan_scan->window[i].x = pan_scan_param->rect_left_offset[i];
        dest_pan_scan->window[i].y = pan_scan_param->rect_top_offset[i];
        dest_pan_scan->window[i].dx = pan_scan_param->rect_right_offset[i];
        dest_pan_scan->window[i].dy = pan_scan_param->rect_bottom_offset[i];
      }
#ifndef PANSCAN_HDLR
      if (pan_scan_param->rect_repetition_period == 0)
        pan_scan_param->rect_id = NO_PAN_SCAN_BIT;
      else if (pan_scan_param->rect_repetition_period > 1)
        pan_scan_param->rect_repetition_period =
        (pan_scan_param->rect_repetition_period == 2)? 0 :
        (pan_scan_param->rect_repetition_period - 1);
#endif
    }
    else
      pan_scan_param->rect_repetition_period = 0;
}

OMX_S64 h264_stream_parser::process_ts_with_sei_vui(OMX_S64 timestamp)
{
  bool clock_ts_flag = false;
  OMX_S64 clock_ts = timestamp;
  OMX_U32 deltaTfiDivisor = 2;
  if (vui_param.timing_info_present_flag)
  {
    if (vui_param.pic_struct_present_flag)
    {
      if(sei_pic_timing.clock_ts_flag)
      {
        clock_ts = ((sei_pic_timing.hours_value * 60 + sei_pic_timing.minutes_value) * 60 + sei_pic_timing.seconds_value) * 1e6 +
                    (sei_pic_timing.n_frames * (vui_param.num_units_in_tick * (1 + sei_pic_timing.nuit_field_based_flag)) + sei_pic_timing.time_offset) *
                    1e6 / vui_param.time_scale;
        ALOGV("-->CLOCK TIMESTAMP   : %lld", clock_ts);
        clock_ts_flag = true;
      }
      if (vui_param.fixed_frame_rate_flag)
      {
        switch (sei_pic_timing.pic_struct)
        {
          case 1: case 2:         deltaTfiDivisor = 1; break;
          case 0: case 3: case 4: deltaTfiDivisor = 2; break;
          case 5: case 6:         deltaTfiDivisor = 3; break;
          case 7:                 deltaTfiDivisor = 4; break;
          case 8:                 deltaTfiDivisor = 6; break;
          default:
            ALOGE("process_ts_with_sei_vui: pic_struct invalid!");
        }
      }
    }
    if (!clock_ts_flag)
    {
      if (vui_param.fixed_frame_rate_flag)
        clock_ts = calculate_fixed_fps_ts(timestamp, deltaTfiDivisor);
      else if (sei_buf_period.is_valid)
        clock_ts = calculate_buf_period_ts(timestamp);
    }
  }
  else
  {
    ALOGV("NO TIMING information present in VUI!");
  }
  sei_pic_timing.is_valid = false; // SEI data is valid only for current frame
  return clock_ts;
}

#ifdef PANSCAN_HDLR

panscan_handler::panscan_handler() : panscan_data(NULL) {}

panscan_handler::~panscan_handler()
{
  if (panscan_data)
  {
    free(panscan_data);
    panscan_data = NULL;
  }
}

bool panscan_handler::initialize(int num_data)
{
  bool ret = false;
  if (!panscan_data)
  {
    panscan_data = (PANSCAN_NODE *) malloc (sizeof(PANSCAN_NODE) * num_data);
    if (panscan_data)
    {
      panscan_free.add_multiple(panscan_data, num_data);
      ret = true;
    }
  }
  else
  {
    ALOGE("ERROR: Old panscan memory must be freed to allocate new");
  }
  return ret;
}

h264_pan_scan *panscan_handler::get_free()
{
  h264_pan_scan *data = NULL;
  PANSCAN_NODE *panscan_node = panscan_used.watch_last();
  panscan_node = (!panscan_node || VALID_TS(panscan_node->start_ts))?
                 panscan_free.remove_first() :
                 panscan_used.remove_last();
  if (panscan_node)
  {
    panscan_node->start_ts = LLONG_MAX;
    panscan_node->end_ts = LLONG_MAX;
    panscan_node->pan_scan_param.rect_id = NO_PAN_SCAN_BIT;
    panscan_node->active = false;
    panscan_used.add_last(panscan_node);
    data = &panscan_node->pan_scan_param;
  }
  return data;
}

h264_pan_scan *panscan_handler::get_populated(OMX_S64 frame_ts)
{
  h264_pan_scan *data = NULL;
  PANSCAN_NODE *panscan_node = panscan_used.watch_first();
  while (panscan_node && !data)
  {
    if (VALID_TS(panscan_node->start_ts))
    {
      if (panscan_node->active && frame_ts < panscan_node->start_ts)
        panscan_node->start_ts = frame_ts;
      if (frame_ts >= panscan_node->start_ts)
        if (frame_ts < panscan_node->end_ts)
        {
          data = &panscan_node->pan_scan_param;
          panscan_node->active = true;
        }
        else
        {
          panscan_free.add_last(panscan_used.remove_first());
          panscan_node = panscan_used.watch_first();
        }
      else
        // Finish search if current timestamp has not reached
        // start timestamp of first panscan data.
        panscan_node = NULL;
    }
    else
    {
      // Only one panscan data is stored for clips
      // with invalid timestamps in every frame
      data = &panscan_node->pan_scan_param;
      panscan_node->active = true;
    }
  }
  if (data)
    if (data->rect_repetition_period == 0)
      panscan_free.add_last(panscan_used.remove_first());
    else if (data->rect_repetition_period > 1)
      data->rect_repetition_period -= 2;
  PRINT_PANSCAN_DATA(panscan_node);
  return data;
}

void panscan_handler::update_last(OMX_S64 frame_ts)
{
  PANSCAN_NODE *panscan_node = panscan_used.watch_last();
  if (panscan_node && !VALID_TS(panscan_node->start_ts))
  {
    panscan_node->start_ts = frame_ts;
    PRINT_PANSCAN_DATA(panscan_node);
    if (panscan_node->prev)
    {
      if (frame_ts < panscan_node->prev->end_ts)
        panscan_node->prev->end_ts = frame_ts;
      else if (!VALID_TS(frame_ts))
        panscan_node->prev->pan_scan_param.rect_repetition_period = 0;
      PRINT_PANSCAN_DATA(panscan_node->prev);
    }
  }
}

template <class NODE_STRUCT>
void omx_dl_list<NODE_STRUCT>::add_multiple(NODE_STRUCT *data_arr, int data_num)
{
  for (int idx = 0; idx < data_num; idx++)
    add_last(&data_arr[idx]);
}

template <class NODE_STRUCT>
NODE_STRUCT *omx_dl_list<NODE_STRUCT>::remove_first()
{
  NODE_STRUCT *data = head;
  if (head)
  {
    if (head->next)
    {
      head = head->next;
      head->prev = NULL;
    }
    else
      head = tail = NULL;
    data->next = data->prev = NULL;
  }
  return data;
}

template <class NODE_STRUCT>
NODE_STRUCT *omx_dl_list<NODE_STRUCT>::remove_last()
{
  NODE_STRUCT *data = tail;
  if (tail)
  {
    if (tail->prev)
    {
      tail = tail->prev;
      tail->next = NULL;
    }
    else
      head = tail = NULL;
    data->next = data->prev = NULL;
  }
  return data;
}

template <class NODE_STRUCT>
void omx_dl_list<NODE_STRUCT>::add_last(NODE_STRUCT* data_ptr)
{
  if (data_ptr)
  {
    data_ptr->next = NULL;
    data_ptr->prev = tail;
    if (tail)
    {
      tail->next = data_ptr;
      tail = data_ptr;
    }
    else
      head = tail = data_ptr;
  }
}

template <class NODE_STRUCT>
NODE_STRUCT* omx_dl_list<NODE_STRUCT>::watch_first()
{
  return head;
}

template <class NODE_STRUCT>
NODE_STRUCT* omx_dl_list<NODE_STRUCT>::watch_last()
{
  return tail;
}

#endif
