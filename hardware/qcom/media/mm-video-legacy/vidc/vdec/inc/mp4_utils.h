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
#ifndef MP4_UTILS_H
#define MP4_UTILS_H
#include "OMX_Core.h"
#include "OMX_QCOMExtns.h"
typedef signed long long int64;
typedef unsigned int uint32;   /* Unsigned 32 bit value */
typedef unsigned short uint16;   /* Unsigned 16 bit value */
typedef unsigned char uint8;   /* Unsigned 8  bit value */

typedef int int32;   /* Signed 32 bit value */
typedef signed short int16;   /* Signed 16 bit value */
typedef signed char int8;   /* Signed 8  bit value */

typedef unsigned char byte;   /* Unsigned 8  bit value type. */
#define SIMPLE_PROFILE_LEVEL0            0x08
#define SIMPLE_PROFILE_LEVEL1            0x01
#define SIMPLE_PROFILE_LEVEL2            0x02
#define SIMPLE_PROFILE_LEVEL3            0x03
#define SIMPLE_PROFILE_LEVEL4A            0x04
#define SIMPLE_PROFILE_LEVEL5            0x05
#define SIMPLE_PROFILE_LEVEL6            0x06
#define SIMPLE_PROFILE_LEVEL0B           0x09

#define SIMPLE_SCALABLE_PROFILE_LEVEL0                  0x10
#define SIMPLE_SCALABLE_PROFILE_LEVEL1                  0x11
#define SIMPLE_SCALABLE_PROFILE_LEVEL2                  0x12

#define SIMPLE_SCALABLE_PROFILE_LEVEL0  0x10
#define SIMPLE_SCALABLE_PROFILE_LEVEL1  0x11
#define SIMPLE_SCALABLE_PROFILE_LEVEL2  0x12
#define ADVANCED_SIMPLE_PROFILE_LEVEL0  0xF0
#define ADVANCED_SIMPLE_PROFILE_LEVEL1  0xF1
#define ADVANCED_SIMPLE_PROFILE_LEVEL2  0xF2
#define ADVANCED_SIMPLE_PROFILE_LEVEL3  0xF3
#define ADVANCED_SIMPLE_PROFILE_LEVEL4  0xF4
#define ADVANCED_SIMPLE_PROFILE_LEVEL5  0xF5

#define VISUAL_OBJECT_SEQUENCE_START_CODE   0x000001B0
#define MP4ERROR_SUCCESS     0

#define VIDEO_OBJECT_LAYER_START_CODE_MASK  0xFFFFFFF0
#define VIDEO_OBJECT_LAYER_START_CODE       0x00000120
#define VOP_START_CODE_MASK                 0xFFFFFFFF
#define VOP_START_CODE                      0x000001B6
#define GOV_START_CODE                      0x000001B3
#define SHORT_HEADER_MASK                   0xFFFFFC00
#define SHORT_HEADER_START_MARKER           0x00008000
#define SHORT_HEADER_START_CODE             0x00008000
#define SPARK1_START_CODE                   0x00008400
#define MPEG4_SHAPE_RECTANGULAR               0x00
#define EXTENDED_PAR                        0xF
#define SHORT_VIDEO_START_MARKER         0x20
#define MP4_INVALID_VOL_PARAM   (0x0001)   // unsupported VOL parameter
#define MP4ERROR_UNSUPPORTED_UFEP                   -1068
#define MP4ERROR_UNSUPPORTED_SOURCE_FORMAT          -1069
#define MASK(x) (0xFFFFFFFF >> (32 - (x)))
#define VISUAL_OBJECT_TYPE_VIDEO_ID         0x1
#define VISUAL_OBJECT_START_CODE            0x000001B5
#define VIDEO_OBJECT_START_CODE_MASK        0xFFFFFFE0
#define VIDEO_OBJECT_START_CODE             0x00000100

#define RESERVED_OBJECT_TYPE                0x0
#define SIMPLE_OBJECT_TYPE                  0x1
#define SIMPLE_SCALABLE_OBJECT_TYPE         0x2
#define CORE_OBJECT_TYPE                    0x3
#define MAIN_OBJECT_TYPE                    0x4
#define N_BIT_OBJECT_TYPE                   0x5
#define BASIC_ANIMATED_2D_TEXTURE           0x6
#define ANIMATED_2D_MESH                    0x7
#define ADVANCED_SIMPLE                     0x11


#define SIMPLE_L1_MAX_VBVBUFFERSIZE 10  /* VBV Max Buffer size=10 (p. 498)  */
#define SIMPLE_L1_MAX_BITRATE       160 /* is is 64kpbs or 160 400bits/sec units */
#define SIMPLE_L2_MAX_VBVBUFFERSIZE 40  /* VBV Max Buffer size = 40 */
#define SIMPLE_L2_MAX_BITRATE       320 /* 320 400bps units = 128kpbs */
#define SIMPLE_L3_MAX_VBVBUFFERSIZE 40  /* VBV Max Buffer size = 40 */
#define SIMPLE_L3_MAX_BITRATE       960 /* 960 400bps units = 384kpbs */

/* The MP4 decoder currently supports Simple Profile@L3 */
#define MAX_VBVBUFFERSIZE (SIMPLE_L3_MAX_VBVBUFFERSIZE)
#define MAX_BITRATE       (SIMPLE_L3_MAX_BITRATE)

#define MAX_QUANTPRECISION 9
#define MIN_QUANTPRECISION 3

#define MP4_VGA_WIDTH             640
#define MP4_VGA_HEIGHT            480
#define MP4_WVGA_WIDTH            800
#define MP4_WVGA_HEIGHT           480
#define MP4_720P_WIDTH            1280
#define MP4_720P_HEIGHT           720

#define MP4_MAX_DECODE_WIDTH    MP4_720P_WIDTH
#define MP4_MAX_DECODE_HEIGHT   MP4_720P_HEIGHT

typedef struct {
   unsigned char *data;
   unsigned long int numBytes;
} mp4StreamType;

#define MAX_FRAMES_IN_CHUNK                 10
#define VOP_START_CODE                      0x000001B6
#define VOL_START_CODE                      0x000001B0

typedef enum VOPTYPE
{
  NO_VOP = -1, // bitstream contains no VOP.
  MPEG4_I_VOP = 0,   // bitstream contains an MPEG4 I-VOP
  MPEG4_P_VOP = 1,   // bitstream contains an MPEG4 P-VOP
  MPEG4_B_VOP = 2,   // bitstream contains an MPEG4 B-VOP
  MPEG4_S_VOP = 3,   // bitstream contains an MPEG4 S-VOP
} VOP_TYPE;

typedef struct
{
  uint32    timestamp_increment;
  uint32    offset;
  uint32    size;
  VOP_TYPE  vopType;
} mp4_frame_info_type;

void mp4_fill_aspect_ratio_info(struct vdec_aspectratioinfo *aspect_ratio_info,
                           OMX_QCOM_EXTRADATA_FRAMEINFO *frame_info);
class MP4_Utils {
private:
   struct posInfoType {
      uint8 *bytePtr;
      uint8 bitPos;
   };

   posInfoType m_posInfo;
   byte *m_dataBeginPtr;
   unsigned int vop_time_resolution;
   bool vop_time_found;
   uint16 m_SrcWidth, m_SrcHeight;   // Dimensions of the source clip
public:
    MP4_Utils();
   ~MP4_Utils();
   int16 populateHeightNWidthFromShortHeader(mp4StreamType * psBits);
   bool parseHeader(mp4StreamType * psBits);
   static uint32 read_bit_field(posInfoType * posPtr, uint32 size);
   bool is_notcodec_vop(unsigned char *pbuffer, unsigned int len);
};
#endif
