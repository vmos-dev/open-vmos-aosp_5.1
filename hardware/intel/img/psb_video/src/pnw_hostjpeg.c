/*
 * Copyright (c) 2011 Intel Corporation. All Rights Reserved.
 * Copyright (c) Imagination Technologies Limited, UK
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Elaine Wang <elaine.wang@intel.com>
 *
 */

/*#include <string.h>*/
#include <stdio.h>

#include <pnw_hostcode.h>
#include <pnw_hostjpeg.h>
#include "psb_drv_debug.h"

#define TRACK_FREE(ptr)                 free(ptr)
#define TRACK_MALLOC(ptr)               malloc(ptr)
#define TRACK_DEVICE_MEMORY_INIT
#define TRACK_DEVICE_MEMORY_SHOW
#define TIMER_INIT
#define TIMER_START(...)
#define TIMER_END(...)
#define TIMER_CLOSE
#define TIMER_CAPTURE(...)


//#include <stdio.h>
//#include <memory.h>
//#include <stdlib.h>
//#include <stdlib.h>
//#include "malloc.h"
//#include "mtxmain.h"

//#include "mtxccb.h"
//#include "topaz_tests.h"
//#include "mvea_regs.h"
//#include "mtx_regs.h"
//#include "defs.h"
//#include "mtxencode.h"
//#include "topaz_vlc_regs.h"
//#include "mtxheader.h"
//#include "apiinternal.h"

//#ifdef FIXME
//extern void JPEGInitialiseHardware();
//#endif

#define JPEG_INCLUDE_NONAPI 1



/////////////////////////////////////////////////////////////////////////////////////
// BMP Reading Header Stuff
/////////////////////////////////////////////////////////////////////////////////////


const IMG_UINT8 gQuantLuma[QUANT_TABLE_SIZE_BYTES] = {
    16, 11, 10, 16, 24, 40, 51, 61,
    12, 12, 14, 19, 26, 58, 60, 55,
    14, 13, 16, 24, 40, 57, 69, 56,
    14, 17, 22, 29, 51, 87, 80, 62,
    18, 22, 37, 56, 68, 109, 103, 77,
    24, 35, 55, 64, 81, 104, 113, 92,
    49, 64, 78, 87, 103, 121, 120, 101,
    72, 92, 95, 98, 112, 100, 103, 99
};

/*const IMG_UINT8 DEBUG_gQuantLumaForCoverage[QUANT_TABLE_SIZE_BYTES] =
{
    16, 17, 21, 22, 30, 31, 22, 45,
    18, 20, 23, 29, 32, 29, 44, 46,
    19, 24, 28, 33, 36, 43, 47, 58,
    25, 27, 34, 29, 42, 48, 57, 59,
    26, 34, 37, 41, 49, 56 , 60 , 67,
    36, 35, 40, 50, 55, 61 , 66 , 68,
    37, 39, 51, 54, 62 , 65 , 69 , 72 ,
    38, 52, 53, 63, 64 , 70 , 71 , 73
};*/

int DEBUG_giUsegQuantLumaForCoverageTable = 0;

/*****************************************************************************/
/*  \brief   gQuantChroma                                                    */
/*                                                                           */
/*  Contains the data that needs to be sent in the marker segment of an      */
/*  interchange format JPEG stream or an abbreviated format table            */
/*  specification data stream.                                               */
/*  Quantizer table for the chrominance component                            */
/*****************************************************************************/
const IMG_UINT8 gQuantChroma[QUANT_TABLE_SIZE_BYTES] = {
    17, 18, 24, 47, 99, 99, 99, 99,
    18, 21, 26, 66, 99, 99, 99, 99,
    24, 26, 56, 99, 99, 99, 99, 99,
    47, 66, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99
};

/*****************************************************************************/
/*  \brief   gZigZag                                                         */
/*                                                                           */
/*  Zigzag scan pattern                                                      */
/*****************************************************************************/
const IMG_UINT8 gZigZag[] = {
    0,  1,  8, 16,  9,  2,  3, 10,
    17, 24, 32, 25, 18, 11,  4,  5,
    12, 19, 26, 33, 40, 48, 41, 34,
    27, 20, 13,  6,  7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36,
    29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46,
    53, 60, 61, 54, 47, 55, 62, 63
};

/*****************************************************************************/
/*  \brief   gMarkerDataLumaDc                                               */
/*                                                                           */
/*  Contains the data that needs to be sent in the marker segment of an      */
/*  interchange format JPEG stream or an abbreviated format table            */
/*  specification data stream.                                               */
/*  Specifies the huffman table used for encoding the luminance DC           */
/*  coefficient differences. The table represents Table K.3 of               */
/*  IS0/IEC 10918-1:1994(E)                                                  */
/*****************************************************************************/
const IMG_UINT8 gMarkerDataLumaDc[] = {
    //TcTh  Li
    0x00, 0x00, 0x01, 0x05, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    // Vi
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B
};

/*****************************************************************************/
/*  \brief   gMarkerDataLumaAc                                               */
/*                                                                           */
/*  Contains the data that needs to be sent in the marker segment of an      */
/*  interchange format JPEG stream or an abbreviated format table            */
/*  specification data stream.                                               */
/*  Specifies the huffman table used for encoding the luminance AC           */
/*  coefficients. The table represents Table K.5 of IS0/IEC 10918-1:1994(E)  */
/*****************************************************************************/
const IMG_UINT8 gMarkerDataLumaAc[] = {
    // TcTh  Li
    0x10, 0x00, 0x02, 0x01, 0x03, 0x03, 0x02, 0x04, 0x03, 0x05, 0x05, 0x04,
    0x04, 0x00, 0x00, 0x01, 0x7D,
    // Vi
    0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12, 0x21, 0x31, 0x41, 0x06,
    0x13, 0x51, 0x61, 0x07, 0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xA1, 0x08,
    0x23, 0x42, 0xB1, 0xC1, 0x15, 0x52, 0xD1, 0xF0, 0x24, 0x33, 0x62, 0x72,
    0x82, 0x09, 0x0A, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x25, 0x26, 0x27, 0x28,
    0x29, 0x2A, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x43, 0x44, 0x45,
    0x46, 0x47, 0x48, 0x49, 0x4A, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
    0x5A, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x73, 0x74, 0x75,
    0x76, 0x77, 0x78, 0x79, 0x7A, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
    0x8A, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0xA2, 0xA3,
    0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6,
    0xB7, 0xB8, 0xB9, 0xBA, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9,
    0xCA, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xE1, 0xE2,
    0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xF1, 0xF2, 0xF3, 0xF4,
    0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA
};

/*****************************************************************************/
/*  \brief   gMarkerDataChromaDc                                             */
/*                                                                           */
/*  Contains the data that needs to be sent in the marker segment of an      */
/*  interchange format JPEG stream or an abbreviated format table            */
/*  specification data stream.                                               */
/*  Specifies the huffman table used for encoding the chrominance DC         */
/*  coefficient differences. The table represents Table K.4 of               */
/*  IS0/IEC 10918-1:1994(E)                                                  */
/*****************************************************************************/
const IMG_UINT8 gMarkerDataChromaDc[] = {
    // TcTh Li
    0x01, 0x00, 0x03, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00,

    // Vi
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B
};

/*****************************************************************************/
/*  \brief   gMarkerDataChromaAc                                             */
/*                                                                           */
/*  Contains the data that needs to be sent in the marker segment of an      */
/*  interchange format JPEG stream or an abbreviated format table            */
/*  specification data stream.                                               */
/*  Specifies the huffman table used for encoding the chrominance AC         */
/*  coefficients. The table represents Table K.6 of IS0/IEC 10918-1:1994(E)  */
/*****************************************************************************/
const IMG_UINT8 gMarkerDataChromaAc[] = {
    // TcTh
    0x11, 0x00, 0x02, 0x01, 0x02, 0x04, 0x04, 0x03, 0x04, 0x07, 0x05, 0x04,
    0x04, 0x00, 0x01, 0x02, 0x77,

    // Vi
    0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21, 0x31, 0x06, 0x12, 0x41,
    0x51, 0x07, 0x61, 0x71, 0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91,
    0xA1, 0xB1, 0xC1, 0x09, 0x23, 0x33, 0x52, 0xF0, 0x15, 0x62, 0x72, 0xD1,
    0x0A, 0x16, 0x24, 0x34, 0xE1, 0x25, 0xF1, 0x17, 0x18, 0x19, 0x1A, 0x26,
    0x27, 0x28, 0x29, 0x2A, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x43, 0x44,
    0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
    0x59, 0x5A, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x73, 0x74,
    0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
    0x88, 0x89, 0x8A, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A,
    0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4,
    0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7,
    0xC8, 0xC9, 0xCA, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA,
    0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xF2, 0xF3, 0xF4,
    0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA
};

/*****************************************************************************/
/*  \brief   gLumaDCCode                                                     */
/*                                                                           */
/*  Contains the code words to encode the luminance DC coefficient           */
/*  differences. The code words are mentioned in table K.3 of                */
/*  ISO/IEC 10918-1:1994(E)                                                  */
/*****************************************************************************/
const IMG_UINT16 gLumaDCCode[] = {
    0x0000, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006,
    0x000E, 0x001E, 0x003E, 0x007E, 0x00FE, 0x01FE
};

/*****************************************************************************/
/*  \brief   gLumaDCSize                                                     */
/*                                                                           */
/*  Contains the code length of the code words that are used to encode the   */
/*  luminance DC coefficient differences. The code lengths are mentioned in  */
/*  table K.3 of ISO/IEC 10918-1:1994(E)                                     */
/*****************************************************************************/
const IMG_UINT8 gLumaDCSize[] = {
    0x0002, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003,
    0x0004, 0x0005, 0x0006, 0x0007, 0x0008, 0x0009
};

/*****************************************************************************/
/*  \brief   gChromaDCCode                                                   */
/*                                                                           */
/*  Contains the code words to encode the chrominance DC coefficient         */
/*  differences. The code words are mentioned in table K.4 of                */
/*  ISO/IEC 10918-1:1994(E)                                                  */
/*****************************************************************************/
const IMG_UINT16 gChromaDCCode[] = {
    0x0000, 0x0001, 0x0002, 0x0006, 0x000E, 0x001E,
    0x003E, 0x007E, 0x00FE, 0x01FE, 0x03FE, 0x07FE
};

/*****************************************************************************/
/*  \brief   gChromaDCSize                                                   */
/*                                                                           */
/*  Contains the code length of the code words that are used to encode the   */
/*  chrominance DC coefficient differences. The code lengths are mentioned in*/
/*  table K.4 of ISO/IEC 10918-1:1994(E)                                     */
/*****************************************************************************/
const IMG_UINT8 gChromaDCSize[] = {
    0x0002, 0x0002, 0x0002, 0x0003, 0x0004, 0x0005,
    0x0006, 0x0007, 0x0008, 0x0009, 0x000A, 0x000B
};


/*****************************************************************************/
/*  \brief   gLumaACCode                                                     */
/*                                                                           */
/*  Contains the code words to encode the luminance AC coefficients. The     */
/*  code words are arrange in the increasing order of run followed by size as*/
/*  in table K.5 of ISO/IEC 10918-1:1994(E)                                  */
/*****************************************************************************/
const IMG_UINT16 gLumaACCode[] = {
    0x000A, 0x0000, 0x0001, 0x0004, 0x000B, 0x001A, 0x0078, 0x00F8, 0x03F6,
    0xFF82, 0xFF83, /* codes for run 0 */
    0x000C, 0x001B, 0x0079, 0x01F6, 0x07F6, 0xFF84, 0xFF85, 0xFF86,
    0xFF87, 0xFF88, /* codes for run 1 */
    0x001C, 0x00F9, 0x03F7, 0x0FF4, 0xFF89, 0xFF8A, 0xFF8b, 0xFF8C,
    0xFF8D, 0xFF8E, /* codes for run 2 */
    0x003A, 0x01F7, 0x0FF5, 0xFF8F, 0xFF90, 0xFF91, 0xFF92, 0xFF93,
    0xFF94, 0xFF95, /* codes for run 3 */
    0x003B, 0x03F8, 0xFF96, 0xFF97, 0xFF98, 0xFF99, 0xFF9A, 0xFF9B,
    0xFF9C, 0xFF9D, /* codes for run 4 */
    0x007A, 0x07F7, 0xFF9E, 0xFF9F, 0xFFA0, 0xFFA1, 0xFFA2, 0xFFA3,
    0xFFA4, 0xFFA5, /* codes for run 5 */
    0x007B, 0x0FF6, 0xFFA6, 0xFFA7, 0xFFA8, 0xFFA9, 0xFFAA, 0xFFAB,
    0xFFAC, 0xFFAD, /* codes for run 6 */
    0x00FA, 0x0FF7, 0xFFAE, 0xFFAF, 0xFFB0, 0xFFB1, 0xFFB2, 0xFFB3,
    0xFFB4, 0xFFB5, /* codes for run 7 */
    0x01F8, 0x7FC0, 0xFFB6, 0xFFB7, 0xFFB8, 0xFFB9, 0xFFBA, 0xFFBB,
    0xFFBC, 0xFFBD, /* codes for run 8 */
    0x01F9, 0xFFBE, 0xFFBF, 0xFFC0, 0xFFC1, 0xFFC2, 0xFFC3, 0xFFC4,
    0xFFC5, 0xFFC6, /* codes for run 9 */
    0x01FA, 0xFFC7, 0xFFC8, 0xFFC9, 0xFFCA, 0xFFCB, 0xFFCC, 0xFFCD,
    0xFFCE, 0xFFCF, /* codes for run A */
    0x03F9, 0xFFD0, 0xFFD1, 0xFFD2, 0xFFD3, 0xFFD4, 0xFFD5, 0xFFD6,
    0xFFD7, 0xFFD8, /* codes for run B */
    0x03FA, 0xFFD9, 0xFFDA, 0xFFDB, 0xFFDC, 0xFFDD, 0xFFDE, 0xFFDF,
    0xFFE0, 0xFFE1, /* codes for run C */
    0x07F8, 0xFFE2, 0xFFE3, 0xFFE4, 0xFFE5, 0xFFE6, 0xFFE7, 0xFFE8,
    0xFFE9, 0xFFEA, /* codes for run D */
    0xFFEB, 0xFFEC, 0xFFED, 0xFFEE, 0xFFEF, 0xFFF0, 0xFFF1, 0xFFF2,
    0xFFF3, 0xFFF4, /* codes for run E */
    0xFFF5, 0xFFF6, 0xFFF7, 0xFFF8, 0xFFF9, 0xFFFA, 0xFFFB, 0xFFFC, 0xFFFD,
    0xFFFE, 0x07F9  /* codes for run F */
};

/*****************************************************************************/
/*  \brief   gLumaACSize                                                     */
/*                                                                           */
/*  Contains the code length of the code words that are used to encode the   */
/*  luminance AC coefficients. The code lengths as in table K.5 of           */
/*  ISO/IEC 10918-1:1994(E), are arranged in the increasing order of run     */
/*  followed by size                                                         */
/*****************************************************************************/
const IMG_UINT8 gLumaACSize[] = {
    0x04, 0x02, 0x02, 0x03, 0x04, 0x05, 0x07, 0x08, 0x0A, 0x10, 0x10,/* run 0 */
    0x04, 0x05, 0x07, 0x09, 0x0B, 0x10, 0x10, 0x10, 0x10, 0x10,/* run 1 */
    0x05, 0x08, 0x0A, 0x0C, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,/* run 2 */
    0x06, 0x09, 0x0C, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,/* run 3 */
    0x06, 0x0A, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,/* run 4 */
    0x07, 0x0B, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,/* run 5 */
    0x07, 0x0C, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,/* run 6 */
    0x08, 0x0C, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,/* run 7 */
    0x09, 0x0F, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,/* run 8 */
    0x09, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,/* run 9 */
    0x09, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,/* run A */
    0x0A, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,/* run B */
    0x0A, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,/* run C */
    0x0B, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,/* run D */
    0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,/* run E */
    0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x0B /* run F */
};

/*****************************************************************************/
/*  \brief   gChromaACCode                                                   */
/*                                                                           */
/*  Contains the code words to encode the chromiannce AC coefficients. The   */
/*  code words are arrange in the increasing order of run followed by size   */
/*  as in table K.6 of ISO/IEC 10918-1:1994(E)                               */
/*****************************************************************************/
const IMG_UINT16 gChromaACCode[] = {
    0x0000, 0x0001, 0x0004, 0x000A, 0x0018, 0x0019, 0x0038, 0x0078, 0x01F4,
    0x03F6, 0x0FF4, /* codes for run 0 */
    0x000B, 0x0039, 0x00F6, 0x01F5, 0x07F6, 0x0FF5, 0xFF88, 0xFF89,
    0xFF8A, 0xFF8B, /* codes for run 1 */
    0x001A, 0x00F7, 0x03F7, 0x0FF6, 0x7FC2, 0xFF8C, 0xFF8D, 0xFF8E,
    0xFF8F, 0xFF90, /* codes for run 2 */
    0x001B, 0x00F8, 0x03F8, 0x0FF7, 0xFF91, 0xFF92, 0xFF93, 0xFF94,
    0xFF95, 0xFF96, /* codes for run 3 */
    0x003A, 0x01F6, 0xFF97, 0xFF98, 0xFF99, 0xFF9A, 0xFF9B, 0xFF9C,
    0xFF9D, 0xFF9E, /* codes for run 4 */
    0x003B, 0x03F9, 0xFF9F, 0xFFA0, 0xFFA1, 0xFFA2, 0xFFA3, 0xFFA4,
    0xFFA5, 0xFFA6, /* codes for run 5 */
    0x0079, 0x07F7, 0xFFA7, 0xFFA8, 0xFFA9, 0xFFAA, 0xFFAB, 0xFFAC,
    0xFFAD, 0xFFAE, /* codes for run 6 */
    0x007A, 0x07F8, 0xFFAF, 0xFFB0, 0xFFB1, 0xFFB2, 0xFFB3, 0xFFB4,
    0xFFB5, 0xFFB6, /* codes for run 7 */
    0x00F9, 0xFFB7, 0xFFB8, 0xFFB9, 0xFFBA, 0xFFBB, 0xFFBC, 0xFFBD,
    0xFFBE, 0xFFBF, /* codes for run 8 */
    0x01F7, 0xFFC0, 0xFFC1, 0xFFC2, 0xFFC3, 0xFFC4, 0xFFC5, 0xFFC6,
    0xFFC7, 0xFFC8, /* codes for run 9 */
    0x01F8, 0xFFC9, 0xFFCA, 0xFFCB, 0xFFCC, 0xFFCD, 0xFFCE, 0xFFCF,
    0xFFD0, 0xFFD1, /* codes for run A */
    0x01F9, 0xFFD2, 0xFFD3, 0xFFD4, 0xFFD5, 0xFFD6, 0xFFD7, 0xFFD8,
    0xFFD9, 0xFFDA, /* codes for run B */
    0x01FA, 0xFFDB, 0xFFDC, 0xFFDD, 0xFFDE, 0xFFDF, 0xFFE0, 0xFFE1,
    0xFFE2, 0xFFE3, /* codes for run C */
    0x07F9, 0xFFE4, 0xFFE5, 0xFFE6, 0xFFE7, 0xFFE8, 0xFFE9, 0xFFEA,
    0xFFEb, 0xFFEC, /* codes for run D */
    0x3FE0, 0xFFED, 0xFFEE, 0xFFEF, 0xFFF0, 0xFFF1, 0xFFF2, 0xFFF3,
    0xFFF4, 0xFFF5, /* codes for run E */
    0x7FC3, 0xFFF6, 0xFFF7, 0xFFF8, 0xFFF9, 0xFFFA, 0xFFFB, 0xFFFC, 0xFFFD,
    0xFFFE, 0x03FA  /* codes for run F */
};

/*****************************************************************************/
/*  \brief   gChromaACSize                                                   */
/*                                                                           */
/*  Contains the code length of the code words that are used to encode the   */
/*  chrominance AC coefficients. The code lengths as in table K.5 of         */
/*  ISO/IEC 10918-1:1994(E), are arranged in the increasing order of run     */
/*  followed by size                                                         */
/*****************************************************************************/
const IMG_UINT8 gChromaACSize[] = {
    0x02, 0x02, 0x03, 0x04, 0x05, 0x05, 0x06, 0x07, 0x09, 0x0A, 0x0C,/* run 0 */
    0x04, 0x06, 0x08, 0x09, 0x0B, 0x0C, 0x10, 0x10, 0x10, 0x10,/* run 1 */
    0x05, 0x08, 0x0A, 0x0C, 0x0F, 0x10, 0x10, 0x10, 0x10, 0x10,/* run 2 */
    0x05, 0x08, 0x0A, 0x0C, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,/* run 3 */
    0x06, 0x09, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,/* run 4 */
    0x06, 0x0A, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,/* run 5 */
    0x07, 0x0B, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,/* run 6 */
    0x07, 0x0B, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,/* run 7 */
    0x08, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,/* run 8 */
    0x09, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,/* run 9 */
    0x09, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,/* run A */
    0x09, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,/* run B */
    0x09, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,/* run C */
    0x0B, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,/* run D */
    0x0E, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,/* run E */
    0x0F, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x0A /* run F */
};

/*****************************************************************************/
/*  \brief   gSize                                                           */
/*                                                                           */
/*  Table used in the computation of 4 lease significant bits 'SSSS' as      */
/*  specified in table F.1 and F.2 of ISO/IEC 10918-1:1994(E). These values  */
/*  are used to computes the 4 least significant bits and are not exactly    */
/*  the values mentioned in the table                                        */
/*****************************************************************************/
const IMG_UINT8 gSize[] = {
    0,
    1,
    2, 2,
    3, 3, 3, 3,
    4, 4, 4, 4, 4, 4, 4, 4,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5
};

int customize_quantization_tables(unsigned char *luma_matrix,
                                  unsigned char *chroma_matrix,
                                  unsigned int ui32Quality)
{
    unsigned int uc_qVal;
    unsigned int uc_j;

    if((NULL == luma_matrix) || (NULL == chroma_matrix) || 
       (ui32Quality < 1) || (ui32Quality > 100))
        return 1;

    /* Compute luma quantization table */
    ui32Quality = (ui32Quality<50) ? (5000/ui32Quality) : (200-ui32Quality*2);
    for(uc_j=0; uc_j<QUANT_TABLE_SIZE_BYTES; ++uc_j) {
        uc_qVal = (gQuantLuma[uc_j] * ui32Quality + 50) / 100;
        uc_qVal =  (uc_qVal>0xFF)? 0xFF:uc_qVal;
        uc_qVal =  (uc_qVal<1)? 1:uc_qVal;
        luma_matrix[uc_j] = (unsigned char)uc_qVal;
    }

    /* Compute chroma quantization table */
    for(uc_j=0; uc_j<QUANT_TABLE_SIZE_BYTES; ++uc_j) {
        uc_qVal = (gQuantChroma[uc_j] * ui32Quality + 50) / 100;
        uc_qVal =  (uc_qVal>0xFF)? 0xFF:uc_qVal;
        uc_qVal =  (uc_qVal<1)? 1:uc_qVal;
        chroma_matrix[uc_j] = (unsigned char)uc_qVal;
    }

    return 0;
}

/*****************************************************************************/
/*                                                                           */
/* Function Name : fPutBitsToBuffer                                          */
/*                                                                           */
/* Description   : The function write a bit string into the stream           */
/*                                                                           */
/* Inputs        :                                                           */
/* BitStream     : Pointer to the stream context                             */
/* NoOfBits      : Size of the bit string                                    */
/* ActualBits    : Bit string to be written                                  */
/*                                                                           */
/* Outputs       : On return the function has written the bit string into    */
/*                 the stream                                                */
/*                                                                           */
/* Returns       : void                                                      */
/*                                                                           */
/* Revision History:                                                         */
/*                                                                           */
/* 14 12 2001 BG Creation                                                    */
/*                                                                           */
/*****************************************************************************/

void fPutBitsToBuffer(STREAMTYPEW *BitStream, IMG_UINT8 NoOfBytes, IMG_UINT32 ActualBits)
{
    IMG_UINT8 ui8Lp;
    IMG_UINT8 *pui8S;

    pui8S = (IMG_UINT8 *)BitStream->Buffer;
    pui8S += BitStream->Offset;

    for (ui8Lp = NoOfBytes; ui8Lp > 0; ui8Lp--)
        *(pui8S++) = ((IMG_UINT8 *) & ActualBits)[ui8Lp-1];

    BitStream->Offset += NoOfBytes;
}



/***********************************************************************************
 * Function Name      : AllocateCodedDataBuffers
 * Inputs             :
 * Outputs            :
 * Returns            : PVRRC
 * Description        : Allocates Device memory for coded buffers and build BUFFERINFO array
 ************************************************************************************/

IMG_ERRORCODE AllocateCodedDataBuffers(TOPAZSC_JPEG_ENCODER_CONTEXT *pContext)
{
    IMG_UINT8 ui8Loop;

    for (ui8Loop = 0 ; ui8Loop < pContext->sScan_Encode_Info.ui8NumberOfCodedBuffers; ui8Loop ++)
        if (pContext->sScan_Encode_Info.aBufferTable[ui8Loop].pMemInfo == NULL) {
            pContext->sScan_Encode_Info.aBufferTable[ui8Loop].ui32DataBufferSizeBytes = ((DATA_BUFFER_SIZE(pContext->sScan_Encode_Info.ui32NumberMCUsToEncodePerScan) + sizeof(BUFFER_HEADER)) + 3) & ~3;
            pContext->sScan_Encode_Info.aBufferTable[ui8Loop].ui32DataBufferUsedBytes = 0;
            pContext->sScan_Encode_Info.aBufferTable[ui8Loop].i8MTXNumber = 0; // Indicates buffer is idle
            pContext->sScan_Encode_Info.aBufferTable[ui8Loop].ui16ScanNumber = 0; // Indicates buffer is idle
            pContext->sScan_Encode_Info.aBufferTable[ui8Loop].pMemInfo =
                (unsigned char *)pContext->jpeg_coded_buf.pMemInfo + PNW_JPEG_HEADER_MAX_SIZE + ui8Loop * pContext->ui32SizePerCodedBuffer;

        }

    return IMG_ERR_OK;
}


#if 0
/***********************************************************************************
 * Function Name      : FreeCodedDataBuffers
 * Inputs             :
 * Outputs            :
 * Returns            : PVRRC
 * Description        : Loops through our coded data buffers and frees them
 ************************************************************************************/

IMG_UINT32 FreeCodedDataBuffers(TOPAZSC_JPEG_ENCODER_CONTEXT *pContext)
{
    IMG_UINT8 ui8Loop;
    ui8Loop = pContext->sScan_Encode_Info.ui8NumberOfCodedBuffers;
    /* Spin through and remove */
    while (ui8Loop--)
        if (pContext->sScan_Encode_Info.aBufferTable[ui8Loop].pMemInfo != NULL) {
            /*      MMFreeDeviceMemory( &(pContext->sScan_Encode_Info.aBufferTable[ui8Loop].pMemInfo));*/
            pContext->sScan_Encode_Info.aBufferTable[ui8Loop].pMemInfo = NULL;
        }

    return 0;
}
#endif



void SetupMCUDetails(TOPAZSC_JPEG_ENCODER_CONTEXT *pContext,
                     const IMG_UINT32 uiComponentNumber ,
                     const IMG_UINT32 uiWidthBlocks ,
                     const IMG_UINT32 uiHeightBlocks,
                     const IMG_UINT32 uiLastRow,
                     const IMG_UINT32 uiLastCol)
{
    MCUCOMPONENT* pMCUComp;

    if (uiComponentNumber >=  MTX_MAX_COMPONENTS)
        return;

    pMCUComp = &pContext->pMTXSetup->MCUComponent[uiComponentNumber];

    pMCUComp->ui32WidthBlocks = uiWidthBlocks * 8;
    pMCUComp->ui32HeightBlocks = uiHeightBlocks * 8;

    pMCUComp->ui32XLimit = uiLastCol;
    pMCUComp->ui32YLimit = uiLastRow;

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "MCU Details %i : %ix%i  %i  %i\n", uiComponentNumber, uiWidthBlocks , uiHeightBlocks ,  uiLastCol , uiLastRow);

}


/*****************************************************************************/
/*                                                                           */
/* Function Name : InitializeJpegEncode                                      */
/*                                                                           */
/* Description   : The function initializes an instance of the JPEG encoder  */
/* Inputs/Outputs: Pointer to the JPEG Host context                                                      */
/*                 Pointer to a IMG_FRAME type which will be created to be filled        */
/*                      with the source image data                                                                               */
/*****************************************************************************/

IMG_ERRORCODE InitializeJpegEncode(TOPAZSC_JPEG_ENCODER_CONTEXT * pContext, object_surface_p __maybe_unused pTFrame)
{
    IMG_ERRORCODE rc = IMG_ERR_OK;
    IMG_UINT8  uc_i = 0;
    IMG_UINT16 ui16_height_min;
    IMG_UINT16 ui16_height_max;
    IMG_UINT16 ui16_width_min;
    IMG_UINT16 ui16_width_max;
    //IMG_UINT16 ui16_width;
    //IMG_UINT16 ui16_height;
    IMG_UINT32 uiBlockCount = 0;
    IMG_UINT16 ui16_comp_width, ui16_comp_height;
    IMG_UINT8  uc_h_scale_max = 0, uc_v_scale_max = 0, uc_h_scale = 0;
    IMG_UINT8  uc_v_scale;
    IMG_UINT16 ui16_height, ui16_width;
    context_ENC_p ctx = (context_ENC_p)pContext->ctx;

    /*************************************************************************/
    /* Determine the horizontal and the vertical scaling factor of each of   */
    /* the components of the image                                           */
    /*************************************************************************/

    /*FIXME: ONLY support NV12, YV12 here*/
    /*pTFrame->height isn't the real height of image, since vaCreateSurface
     * makes it aligned with 32*/
    ui16_height = pContext->ui32OutputHeight;
    ui16_width = pContext->ui32OutputWidth;
	
    switch (pContext->eFormat) {
    case IMG_CODEC_YV16: /*422 format*/
        ui16_height_min = ui16_height;
        ui16_height_max = ui16_height;
        ui16_width_min = ui16_width >> 1;
        ui16_width_max = ui16_width;
        break;
    case IMG_CODEC_NV12:
    case IMG_CODEC_IYUV:
    default:
        ui16_height_min = ui16_height >> 1;
        ui16_height_max = ui16_height;
        ui16_width_min = ui16_width >> 1;
        ui16_width_max = ui16_width;
        break;
    }
    /*ui16_height_min = ui16_width_min  = 65535;
    ui16_height_max = ui16_width_max  = 0;

    for(uc_i = 0; uc_i < pContext->pMTXSetup->ui32ComponentsInScan; uc_i++)
    {
    ui16_width  = pTFrame->Width;
    if(ui16_width_min > ui16_width)
    ui16_width_min  = ui16_width;
    if(ui16_width_max < ui16_width)
    ui16_width_max  = ui16_width;

    ui16_height = pTFrame->Height;
    if(ui16_height_min > ui16_height)
    ui16_height_min = ui16_height;
    if(ui16_height_max < ui16_height)
    ui16_height_max = ui16_height;
    }
    */
    /*********************************************************************/
    /* Determine the horizonal and the vertical sampling frequency of    */
    /* each of components in the image                                   */
    /*********************************************************************/

    uc_h_scale_max = (ui16_width_max + ui16_width_min - 1) /
                     ui16_width_min;
    uc_v_scale_max = (ui16_height_max + ui16_height_min - 1) /
                     ui16_height_min;

    for (uc_i = 0; uc_i < pContext->pMTXSetup->ui32ComponentsInScan; uc_i++) {

        /*      ui16_comp_width  = pTFrame->aui32ComponentInfo[uc_i].ui32Width;
                ui16_comp_height = pTFrame->aui32ComponentInfo[uc_i].ui32Height;*/
        /*Support NV12, YV12, YV16 here. uc_h/v_scale should be
         * 2x2(Y) or 1x1(U/V)*/
        if (0 == uc_i) {
            ui16_comp_width  = ui16_width;
            ui16_comp_height = ui16_height;
        } else {
            switch (pContext->eFormat) {
            case IMG_CODEC_YV16: /*422 format*/
                ui16_comp_width  = ui16_width >> 1;
                ui16_comp_height = ui16_height;
                break;
            case IMG_CODEC_NV12:
            case IMG_CODEC_IYUV:
            default:
                ui16_comp_width  = ui16_width >> 1;
                ui16_comp_height = ui16_height >> 1;
            }
        }

        uc_h_scale       = (ui16_comp_width *  uc_h_scale_max) /
                           ui16_width_max;
        uc_v_scale       = (ui16_comp_height * uc_v_scale_max) /
                           ui16_height_max;

        uiBlockCount += (uc_h_scale * uc_v_scale);

        switch (ISCHROMAINTERLEAVED(pContext->eFormat)) {
        case C_INTERLEAVE:
            // Chroma format is byte interleaved, as the engine runs using planar colour surfaces we need
            // to fool the engine into offsetting by 16 instead of 8
            if (uc_i > 0) { // if chroma, then double values
                uc_h_scale <<= 1;
                ui16_comp_width <<= 1;
            }
            break;
        case LC_UVINTERLEAVE:
            //Y0UY1V_8888 or Y0VY1U_8888 format
            ui16_comp_width <<= 1;
            break;
        case LC_VUINTERLEAVE:
            //Y0UY1V_8888 or Y0VY1U_8888 format
            ui16_comp_width <<= 1;
            break;
        default:
            break;
        }

        SetupMCUDetails(pContext, uc_i , uc_h_scale, uc_v_scale, ui16_comp_height, ui16_comp_width);

        if (uiBlockCount > BLOCKCOUNTMAXPERCOLOURPLANE) {
            return IMG_ERR_INVALID_SIZE;
        }

    }

    return rc;
}


/***********************************************************************************
  Function Name      : JPGEncodeBegin
Inputs             : hInstance,psJpegInfo,ui32Quality
Outputs            :
Returns            : PVRRC
Description        : Marks the begining of a JPEG encode and sets up encoder
 ************************************************************************************/

/*IMG_ERRORCODE JPGEncodeBegin(TOPAZSC_JPEG_ENCODER_CONTEXT *pContext, IMG_FRAME *pTFrame)
{
    IMG_UINT32 ReturnCode = 0;
    ReturnCode = InitializeJpegEncode(pContext, pTFrame);

    return ReturnCode;
} */

#if 0
/*****************************************************************************/
/*                                                                           */
/* Function Name : EncodeMarkerSegment                                       */
/*                                                                           */
/* Description   : Writes the marker segment of a JPEG stream according to   */
/*                 the syntax mentioned in section B.2.4 of                  */
/*                 ISO/IEC 10918-1:1994E                                     */
/*                                                                           */
/* Inputs        : Pointer to the JPEG Context and coded output buffer       */
/*                                                                           */
/* Processing    : Writes each of the following into the marker segment of   */
/*                 the JPEG stream                                           */
/*                 - luminance and chrominance quantization tables           */
/*                 - huffman tables used for encoding the luminance and      */
/*                   chrominance AC coefficients                             */
/*                 - huffman tables used for encoding the luminance and      */
/*                   chorimance  DC coefficient differences                  */
/*                                                                           */
/*                                                                           */
/*****************************************************************************/
IMG_UINT32 Legacy_EncodeMarkerSegment(LEGACY_JPEG_ENCODER_CONTEXT *pContext,
                                      IMG_UINT8 *puc_stream_buff)
{
    STREAMTYPEW s_streamW;
    IMG_UINT8 uc_i;

    s_streamW.Offset = 0;
    s_streamW.Buffer = puc_stream_buff;

    /* Writing the start of image marker */
    fPutBitsToBuffer(&s_streamW, 2, START_OF_IMAGE);

    /* Writing the quantization table for luminance into the stream */
    fPutBitsToBuffer(&s_streamW, 2, DQT_MARKER);

    fPutBitsToBuffer(&s_streamW, 3, LQPQ << 4); // 20 bits = LQPQ, 4 bits = 0 (Destination identifier for the luminance quantizer tables)

    for (uc_i = 0; uc_i < PELS_IN_BLOCK; uc_i++) {
        fPutBitsToBuffer(&s_streamW, 1, pContext->pvLowLevelEncContext->Qmatrix[0][gZigZag[uc_i]]);
    }

    /* Writing the quantization table for chrominance into the stream */
    fPutBitsToBuffer(&s_streamW, 2, DQT_MARKER);

    fPutBitsToBuffer(&s_streamW, 3, (LQPQ << 4) | 1); // 20 bits = LQPQ, 4 bits = 1 (Destination identifier for the chrominance quantizer tables)

    for (uc_i = 0; uc_i < PELS_IN_BLOCK; uc_i++) {
        fPutBitsToBuffer(&s_streamW, 1, pContext->pvLowLevelEncContext->Qmatrix[1][gZigZag[uc_i]]);
    }

    /* Writing the huffman tables for luminance dc coeffs */
    /* Write the DHT Marker */
    fPutBitsToBuffer(&s_streamW, 2, DHT_MARKER);
    fPutBitsToBuffer(&s_streamW, 2, LH_DC);
    for (uc_i = 0; uc_i < LH_DC - 2; uc_i++) {
        fPutBitsToBuffer(&s_streamW, 1, gMarkerDataLumaDc[uc_i]);
    }
    /* Writing the huffman tables for luminance ac coeffs */
    /* Write the DHT Marker */
    fPutBitsToBuffer(&s_streamW, 2, DHT_MARKER);
    fPutBitsToBuffer(&s_streamW, 2, LH_AC);
    for (uc_i = 0; uc_i < LH_AC - 2; uc_i++) {
        fPutBitsToBuffer(&s_streamW, 1, gMarkerDataLumaAc[uc_i]);
    }
    /* Writing the huffman tables for chrominance dc coeffs */
    fPutBitsToBuffer(&s_streamW, 2, DHT_MARKER);
    fPutBitsToBuffer(&s_streamW, 2, LH_DC);
    for (uc_i = 0; uc_i < LH_DC - 2; uc_i++) {
        fPutBitsToBuffer(&s_streamW, 1, gMarkerDataChromaDc[uc_i]);
    }
    /* Writing the huffman tables for luminance ac coeffs */
    /* Write the DHT Marker */
    fPutBitsToBuffer(&s_streamW, 2, DHT_MARKER);
    fPutBitsToBuffer(&s_streamW, 2, LH_AC);
    for (uc_i = 0; uc_i < LH_AC - 2; uc_i++) {
        fPutBitsToBuffer(&s_streamW, 1, gMarkerDataChromaAc[uc_i]);
    }


    return s_streamW.Offset;
}
#endif

/*****************************************************************************/
/*                                                                           */
/* Function Name : EncodeMarkerSegment                                       */
/*                                                                           */
/* Description   : Writes the marker segment of a JPEG stream according to   */
/*                 the syntax mentioned in section B.2.4 of                  */
/*                 ISO/IEC 10918-1:1994E                                     */
/*                                                                           */
/* Inputs        : Pointer to the JPEG Context and coded output buffer       */
/*                                                                           */
/* Processing    : Writes each of the following into the marker segment of   */
/*                 the JPEG stream                                           */
/*                 - luminance and chrominance quantization tables           */
/*                 - huffman tables used for encoding the luminance and      */
/*                   chrominance AC coefficients                             */
/*                 - huffman tables used for encoding the luminance and      */
/*                   chorimance  DC coefficient differences                  */
/*                                                                           */
/*****************************************************************************/

IMG_UINT32 EncodeMarkerSegment(TOPAZSC_JPEG_ENCODER_CONTEXT *pContext,
                               IMG_UINT8 *puc_stream_buff, IMG_BOOL bIncludeHuffmanTables)
{
    STREAMTYPEW s_streamW;
    IMG_UINT8 uc_i;

    s_streamW.Offset = 0;
    s_streamW.Buffer = puc_stream_buff;

    /* Writing the start of image marker */
    fPutBitsToBuffer(&s_streamW, 2, START_OF_IMAGE);

    /* Writing the quantization table for luminance into the stream */
    fPutBitsToBuffer(&s_streamW, 2, DQT_MARKER);

    fPutBitsToBuffer(&s_streamW, 3, LQPQ << 4); // 20 bits = LQPQ, 4 bits = 0 (Destination identifier for the luminance quantizer tables)

    IMG_ASSERT(PELS_IN_BLOCK <= QUANT_TABLE_SIZE_BYTES);
    for (uc_i = 0; uc_i < PELS_IN_BLOCK; uc_i++) {
        // Write zigzag ordered luma quantization values to our JPEG header
        fPutBitsToBuffer(&s_streamW, 1, pContext->psTablesBlock->aui8LumaQuantParams[gZigZag[uc_i]]);
    }

    /* Writing the quantization table for chrominance into the stream */
    fPutBitsToBuffer(&s_streamW, 2, DQT_MARKER);

    fPutBitsToBuffer(&s_streamW, 3, (LQPQ << 4) | 1); // 20 bits = LQPQ, 4 bits = 1 (Destination identifier for the chrominance quantizer tables)

    for (uc_i = 0; uc_i < PELS_IN_BLOCK; uc_i++) {
        // Write zigzag ordered chroma quantization values to our JPEG header
        fPutBitsToBuffer(&s_streamW, 1, pContext->psTablesBlock->aui8ChromaQuantParams[gZigZag[uc_i]]);
    }

    if (bIncludeHuffmanTables) {
        /* Writing the huffman tables for luminance dc coeffs */
        /* Write the DHT Marker */
        fPutBitsToBuffer(&s_streamW, 2, DHT_MARKER);
        fPutBitsToBuffer(&s_streamW, 2, LH_DC);
        for (uc_i = 0; uc_i < LH_DC - 2; uc_i++) {
            fPutBitsToBuffer(&s_streamW, 1, gMarkerDataLumaDc[uc_i]);
        }
        /* Writing the huffman tables for luminance ac coeffs */
        /* Write the DHT Marker */
        fPutBitsToBuffer(&s_streamW, 2, DHT_MARKER);
        fPutBitsToBuffer(&s_streamW, 2, LH_AC);
        for (uc_i = 0; uc_i < LH_AC - 2; uc_i++) {
            fPutBitsToBuffer(&s_streamW, 1, gMarkerDataLumaAc[uc_i]);
        }
        /* Writing the huffman tables for chrominance dc coeffs */
        fPutBitsToBuffer(&s_streamW, 2, DHT_MARKER);
        fPutBitsToBuffer(&s_streamW, 2, LH_DC);
        for (uc_i = 0; uc_i < LH_DC - 2; uc_i++) {
            fPutBitsToBuffer(&s_streamW, 1, gMarkerDataChromaDc[uc_i]);
        }
        /* Writing the huffman tables for luminance ac coeffs */
        /* Write the DHT Marker */
        fPutBitsToBuffer(&s_streamW, 2, DHT_MARKER);
        fPutBitsToBuffer(&s_streamW, 2, LH_AC);
        for (uc_i = 0; uc_i < LH_AC - 2; uc_i++) {
            fPutBitsToBuffer(&s_streamW, 1, gMarkerDataChromaAc[uc_i]);
        }
    }

    // Activate Restart markers
    if (pContext->sScan_Encode_Info.ui16CScan > 1) {
        // Only use restart intervals if we need them (ie. multiple Scan encode and/or parallel CB encode)
        fPutBitsToBuffer(&s_streamW, 2, 0xFFDD); //Marker header
        fPutBitsToBuffer(&s_streamW, 2, 4); // Byte size of marker (header not included)
        fPutBitsToBuffer(&s_streamW, 2, pContext->sScan_Encode_Info.ui32NumberMCUsToEncodePerScan); // Restart Interval (same as MCUs per buffer)
    }

    return s_streamW.Offset;
}

#if 0
/***********************************************************************************
  Function Name      : JPGEncodeMarker
Inputs             :    Pointer to JPEG Encoder context
Pointer to coded buffer stream
Pointer to pui32BytesWritten value
Outputs            : ui8BitStreamBuffer,pui32BytesWritten
Description        : Writes a JPEG marker segment to the bit stream
 ************************************************************************************/

IMG_UINT32 Legacy_JPGEncodeMarker(/*in */               LEGACY_JPEG_ENCODER_CONTEXT *pContext ,
        /*in */         IMG_UINT8* pui8BitStreamBuffer ,
        /*out*/         IMG_UINT32 *pui32BytesWritten)
{
#ifdef JPEG_VERBOSE
    printf("PVRJPGEncodeMarker");
#endif

    if (pContext->eCurrentActive != LEGACY_JPEG_API_CURRENT_ACTIVE_ENCODE) {
        /* Cannot be called outside begin and end*/
        return 2;
    }

    *pui32BytesWritten += Legacy_EncodeMarkerSegment(pContext, pui8BitStreamBuffer);

    return 0;
}
#endif

/***********************************************************************************
  Function Name      : JPGEncodeMarker
Inputs             :    Pointer to JPEG Encoder context
Pointer to coded buffer stream
Pointer to Byteswritten value
bIncludeHuffmanTables - Set to true to include the huffman tables in the stream
Outputs            : ui8BitStreamBuffer,pui32BytesWritten
Description        : Writes a JPEG marker segment to the bit stream
 ************************************************************************************/

IMG_UINT32 JPGEncodeMarker(/*in */              TOPAZSC_JPEG_ENCODER_CONTEXT *pContext ,
        /*in */         IMG_UINT8* pui8BitStreamBuffer ,
        /*out*/         IMG_UINT32 *pui32BytesWritten, IMG_BOOL bIncludeHuffmanTables)
{
#ifdef JPEG_VERBOSE
    printf("PVRJPGEncodeMarker");
#endif


    *pui32BytesWritten += EncodeMarkerSegment(pContext, pui8BitStreamBuffer + *pui32BytesWritten, bIncludeHuffmanTables);

    return 0;
}

#if 0
/*****************************************************************************/
/*                                                                           */
/* Function Name : EncodeFrameHeader                                         */
/*                                                                           */
/* Description   : Writes the frame header of a JPEG stream according to     */
/*                 the syntax mentioned in section B.2.2 of                  */
/*                 ISO/IEC 10918-1:1994E .                                   */
/*                                                                           */
/* Inputs        :                                                           */
/* ps_jpeg_params: Ptr to the JPEG encoder parameter context                 */
/* ps_jpeg_comp  : Ptr to the JPEG image component context                   */
/* puc_stream_buff:Ptr to the bistream buffer from where to start writing    */
/*                                                                           */
/* Processing    : Writes each of the following into the frame header of     */
/*                 the JPEG stream                                           */
/*                 - JPEG image component identifier                         */
/*                 - Horizontal and vertical sampling factor                 */
/*                 - Quantization table identifier for each of the components*/
/*                                                                           */
/* Returns       : Number of bytes written into the stream                   */
/*                                                                           */
/*****************************************************************************/

IMG_UINT32 Legacy_EncodeFrameHeader(LEGACY_JPEGENC_ITTIAM_PARAMS *ps_jpeg_params,
                                    LEGACY_JPEGENC_ITTIAM_COMPONENT *ps_jpeg_comp,
                                    IMG_UINT8 *puc_stream_buff)
{
    STREAMTYPEW ps_streamW;
    IMG_UINT16 ui16_i;
    IMG_UINT8  uc_num_comp_in_img;

    uc_num_comp_in_img = ps_jpeg_comp->uc_num_comp_in_img;

    ps_streamW.Offset = 0;
    ps_streamW.Buffer = puc_stream_buff;


    if (ps_jpeg_params->uc_isAbbreviated != 0)
        fPutBitsToBuffer(&ps_streamW, 2, START_OF_IMAGE);

    /* Writing the frame header */
    fPutBitsToBuffer(&ps_streamW, 2, SOF_BASELINE_DCT);
    /* Frame header length */
    fPutBitsToBuffer(&ps_streamW, 2, 8 + 3 * uc_num_comp_in_img);
    /* Precision */
    fPutBitsToBuffer(&ps_streamW, 1, 8);
    /* Height : sample lines */
    fPutBitsToBuffer(&ps_streamW, 2, ps_jpeg_params->ui16_height);
    /* Width : samples per line */
    fPutBitsToBuffer(&ps_streamW, 2, ps_jpeg_params->ui16_width);
    /* Number of image components */
    fPutBitsToBuffer(&ps_streamW, 1, uc_num_comp_in_img);

    if (uc_num_comp_in_img > MAX_COMP_IN_SCAN)
        uc_num_comp_in_img = MAX_COMP_IN_SCAN;
    for (ui16_i = 0; ui16_i < uc_num_comp_in_img; ui16_i++) {
        /* Component identifier */
        fPutBitsToBuffer(&ps_streamW, 1, ps_jpeg_comp->puc_comp_id[ui16_i]);

        /* 4 bit Horizontal and 4 bit vertical sampling factors */
        fPutBitsToBuffer(&ps_streamW, 1, (ps_jpeg_comp->puc_horiz_scale[ui16_i] << 4) | ps_jpeg_comp->puc_vert_scale[ui16_i]);

        /* Quantization table destination selector */
        fPutBitsToBuffer(&ps_streamW, 1, ps_jpeg_comp->puc_q_table_id[ui16_i]);

        ps_jpeg_comp->CompIdtoIndex[ ps_jpeg_comp->puc_comp_id[ui16_i] ] = (IMG_UINT8) ui16_i;
    }

    //Use if you want start of scan (image data) to align to 32
    //fPutBitsToBuffer(&ps_streamW, 1, 0xFF);

    return ps_streamW.Offset;
}
#endif

/*****************************************************************************/
/*                                                                           */
/* Function Name : EncodeFrameHeader                                         */
/*                                                                           */
/* Description   : Writes the frame header of a JPEG stream according to     */
/*                 the syntax mentioned in section B.2.2 of                  */
/*                 ISO/IEC 10918-1:1994E .                                   */
/*                                                                           */
/* Inputs        :                                                           */
/*                                      pContext                        - Ptr to the JPEG encoder context        */
/*                                      puc_stream_buff         - Ptr to the bistream buffer from        */
/*                                                                                      where to start writing                   */
/*                                                                           */
/* Processing    : Writes each of the following into the frame header of     */
/*                 the JPEG stream                                           */
/*                 - JPEG image component identifier                         */
/*                 - Horizontal and vertical sampling factor                 */
/*                 - Quantization table identifier for each of the components*/
/*                                                                           */
/* Returns       : Number of bytes written into the stream                   */
/*                                                                           */
/*****************************************************************************/

IMG_UINT32 EncodeFrameHeader(TOPAZSC_JPEG_ENCODER_CONTEXT *pContext,
                             IMG_UINT8 *puc_stream_buff)
{
    STREAMTYPEW ps_streamW;
    IMG_UINT8  uc_num_comp_in_img;

    uc_num_comp_in_img = pContext->pMTXSetup->ui32ComponentsInScan;

    ps_streamW.Offset = 0;
    ps_streamW.Buffer = puc_stream_buff;


    //if(ps_jpeg_params->uc_isAbbreviated != 0)
    //   fPutBitsToBuffer(&ps_streamW, 2, START_OF_IMAGE);

    /* Writing the frame header */
    fPutBitsToBuffer(&ps_streamW, 2, SOF_BASELINE_DCT);
    /* Frame header length */
    fPutBitsToBuffer(&ps_streamW, 2, 8 + 3 * uc_num_comp_in_img);
    /* Precision */
    fPutBitsToBuffer(&ps_streamW, 1, 8);
    /* Height : sample lines */
    fPutBitsToBuffer(&ps_streamW, 2, pContext->ui32OutputHeight);
    /* Width : samples per line */
    fPutBitsToBuffer(&ps_streamW, 2, pContext->ui32OutputWidth);
    /* Number of image components */
    fPutBitsToBuffer(&ps_streamW, 1, uc_num_comp_in_img);

    //Luma Details
    /* Component identifier */
    fPutBitsToBuffer(&ps_streamW, 1, 1); //CompId 0 = 1, 1 = 2, 2 = 3
    fPutBitsToBuffer(&ps_streamW, 1, ((pContext->pMTXSetup->MCUComponent[0].ui32WidthBlocks >> 3) << 4) | (pContext->pMTXSetup->MCUComponent[0].ui32HeightBlocks >> 3));
    fPutBitsToBuffer(&ps_streamW, 1, 0); // 0 = Luma(0), 1,2 = Chroma(1)

    //Chroma Details
    if (pContext->pMTXSetup->ui32DataInterleaveStatus < C_INTERLEAVE) { //Chroma planar
        fPutBitsToBuffer(&ps_streamW, 1, 2); //CompId 0 = 1, 1 = 2, 2 = 3
        /* 4 bit Horizontal and 4 bit vertical sampling factors */
        fPutBitsToBuffer(&ps_streamW, 1, ((pContext->pMTXSetup->MCUComponent[1].ui32WidthBlocks >> 3) << 4) | (pContext->pMTXSetup->MCUComponent[1].ui32HeightBlocks >> 3));
        fPutBitsToBuffer(&ps_streamW, 1, 1); // 0 = Luma(0), 1,2 = Chroma(1)
        fPutBitsToBuffer(&ps_streamW, 1, 3); //CompId 0 = 1, 1 = 2, 2 = 3
        /* 4 bit Horizontal and 4 bit vertical sampling factors */
        fPutBitsToBuffer(&ps_streamW, 1, ((pContext->pMTXSetup->MCUComponent[2].ui32WidthBlocks >> 3) << 4) | (pContext->pMTXSetup->MCUComponent[2].ui32HeightBlocks >> 3));
        fPutBitsToBuffer(&ps_streamW, 1, 1); // 0 = Luma(0), 1,2 = Chroma(1)
    } else if (pContext->pMTXSetup->ui32DataInterleaveStatus == C_INTERLEAVE) { // Chroma Interleaved
        fPutBitsToBuffer(&ps_streamW, 1, 2); //CompId 0 = 1, 1 = 2, 2 = 3
        /* 4 bit Horizontal and 4 bit vertical sampling factors */
        fPutBitsToBuffer(&ps_streamW, 1, ((pContext->pMTXSetup->MCUComponent[1].ui32WidthBlocks >> 3) << 3) | (pContext->pMTXSetup->MCUComponent[1].ui32HeightBlocks >> 3));
        fPutBitsToBuffer(&ps_streamW, 1, 1); // 0 = Luma(0), 1,2 = Chroma(1)
        fPutBitsToBuffer(&ps_streamW, 1, 3); //CompId 0 = 1, 1 = 2, 2 = 3
        /* 4 bit Horizontal and 4 bit vertical sampling factors */
        fPutBitsToBuffer(&ps_streamW, 1, ((pContext->pMTXSetup->MCUComponent[2].ui32WidthBlocks >> 3) << 3) | (pContext->pMTXSetup->MCUComponent[2].ui32HeightBlocks >> 3));
        fPutBitsToBuffer(&ps_streamW, 1, 1); // 0 = Luma(0), 1,2 = Chroma(1)
    } else { //Chroma YUYV - Special case
        fPutBitsToBuffer(&ps_streamW, 1, 2); //CompId 0 = 1, 1 = 2, 2 = 3
        /* 4 bit Horizontal and 4 bit vertical sampling factors */
        fPutBitsToBuffer(&ps_streamW, 1, ((pContext->pMTXSetup->MCUComponent[1].ui32WidthBlocks >> 3) << 4) | (pContext->pMTXSetup->MCUComponent[1].ui32HeightBlocks >> 3));
        fPutBitsToBuffer(&ps_streamW, 1, 1); // 0 = Luma(0), 1,2 = Chroma(1)
        fPutBitsToBuffer(&ps_streamW, 1, 3); //CompId 0 = 1, 1 = 2, 2 = 3
        /* 4 bit Horizontal and 4 bit vertical sampling factors */
        fPutBitsToBuffer(&ps_streamW, 1, ((pContext->pMTXSetup->MCUComponent[2].ui32WidthBlocks >> 3) << 4) | (pContext->pMTXSetup->MCUComponent[2].ui32HeightBlocks >> 3));
        fPutBitsToBuffer(&ps_streamW, 1, 1); // 0 = Luma(0), 1,2 = Chroma(1)
    }


    //Use if you want start of scan (image data) to align to 32
    //fPutBitsToBuffer(&ps_streamW, 1, 0xFF);

    return ps_streamW.Offset;
}

#if 0
/***********************************************************************************
  Function Name      : JPGEncodeHeader
Inputs             : Pointer to JPEG Context
Outputs            : ui8BitStreamBuffer,pui32BytesWritten
Returns            : PVRRC
Description        : Writes a frame header to the bit stream
Max written 21 bytes
 ************************************************************************************/

IMG_UINT32 Legacy_JPGEncodeHeader(/*in */               LEGACY_JPEG_ENCODER_CONTEXT *pContext,
        /*out */        IMG_UINT8*              pui8BitStreamBuffer ,
        /*out*/         IMG_UINT32*             pui32BytesWritten)
{
#ifdef JPEG_VERBOSE
    printf("PVRJPGEncodeHeader");
#endif

    if (pContext->eCurrentActive != LEGACY_JPEG_API_CURRENT_ACTIVE_ENCODE) {
        /* Cannot be called outside begin and end*/
        return 2;
    }

    *pui32BytesWritten += Legacy_EncodeFrameHeader(&pContext->JPEGEncoderParams,
                          &pContext->sJPEGEncoderComp,
                          pui8BitStreamBuffer + *pui32BytesWritten);
    return 0;
}
#endif

/***********************************************************************************
  Function Name      : JPGEncodeHeader
Inputs             : Pointer to JPEG context
Outputs            : ui8BitStreamBuffer,pui32BytesWritten
Description        : Writes a frame header to the bit stream
Max written 21 bytes
 ************************************************************************************/

IMG_UINT32 JPGEncodeHeader(/*in */              TOPAZSC_JPEG_ENCODER_CONTEXT *pContext,
        /*out */        IMG_UINT8*              pui8BitStreamBuffer ,
        /*out*/         IMG_UINT32*             pui32BytesWritten)
{
#ifdef JPEG_VERBOSE
    printf("JPGEncodeHeader");
#endif

    *pui32BytesWritten += EncodeFrameHeader(pContext, pui8BitStreamBuffer + *pui32BytesWritten);

    return 0;
}


/***********************************************************************************
 * Function Name      : SetupIssueSetup
 * Inputs             :
 pContext - Pointer to JPEG context
 ui32ComponentsInScan - Number of components to be encoded in a scan
 aui8Planes - Array of 3 bytes indexing YUV colour planes
 pTFrame - Pointer to source data
 ui32TableA, ui32TableB - Quantization table index values
 * Outputs            :
 * Returns            : PVRRC
 * Description        : Issues the Setup structure to MTX
 ************************************************************************************/

IMG_UINT32 SetupIssueSetup(TOPAZSC_JPEG_ENCODER_CONTEXT *pContext, const IMG_UINT32 ui32ComponentsInScan, IMG_UINT8 __maybe_unused * aui8Planes,  object_surface_p pTFrame, const IMG_UINT32 ui32TableA , const IMG_UINT32 __maybe_unused ui32TableB)
{
    IMG_UINT32 ReturnCode = 0;
    IMG_INT32 i32Lp;
    //COMPONENTPLANE* pSrcPlane;
    IMG_UINT32 srf_buf_offset;
    context_ENC_p ctx = (context_ENC_p)pContext->ctx;
    pnw_cmdbuf_p cmdbuf = ctx->obj_context->pnw_cmdbuf;
    /*TOPAZSC_JPEG_ENCODER_CONTEXT *pContext = pEncContext->psJpegHC;*/

#ifdef JPEG_VERBOSE
    printf("\nSetupIssueSetup");
#endif

    pContext->pMTXSetup->ui32ComponentsInScan = ui32ComponentsInScan;
    pContext->pMTXSetup->ui32DataInterleaveStatus = ISCHROMAINTERLEAVED(pContext->eFormat);
    pContext->pMTXSetup->ui32TableA = ui32TableA;

    /*MMUpdateDeviceMemory( pContext->pMemInfoMTXSetup )*/;
    // we want to process the handles that have been placed into the buffer
    /*for(n=0;n<pContext->pMTXSetup->ui32ComponentsInScan;n++)
    {
    pSrcPlane = &pContext->pMTXSetup->ComponentPlane[n];
    TIMER_START(hardwareduration,"");
    MMDeviceMemWriteDeviceMemRef(pContext->pMemInfoMTXSetup->iu32MemoryRegionID, pContext->pMemInfoMTXSetup->hShadowMem, (IMG_UINT32)   ((IMG_BYTE*)&pSrcPlane->ui32PhysAddr - (IMG_BYTE*)pContext->pMTXSetup),TAL_NULL_MANGLER_ID,(IMG_HANDLE) pTFrame->psBuffer->pMemInfo->hShadowMem, pTFrame->aui32ComponentOffset[aui8Planes[n]]);
    TIMER_END("HW - MMDeviceMemWriteDeviceMemRef in SetupIssueSetup (hostjpeg.c)");
    }*/
    /*Support PL12/NV12, YV12/IYUV, YV16*/
    srf_buf_offset = pTFrame->psb_surface->buf.buffer_ofs;
    RELOC_PIC_PARAMS_PNW(&pContext->pMTXSetup->ComponentPlane[0].ui32PhysAddr, srf_buf_offset , &pTFrame->psb_surface->buf);
    switch (pContext->eFormat) {
    case IMG_CODEC_IYUV:
    case IMG_CODEC_PL8:
        RELOC_PIC_PARAMS_PNW(&pContext->pMTXSetup->ComponentPlane[1].ui32PhysAddr,
                             srf_buf_offset + pTFrame->psb_surface->stride * pTFrame->height,
                             &pTFrame->psb_surface->buf);
        RELOC_PIC_PARAMS_PNW(&pContext->pMTXSetup->ComponentPlane[2].ui32PhysAddr,
                             srf_buf_offset + pTFrame->psb_surface->stride * pTFrame->height
                             + (pTFrame->psb_surface->stride / 2) *(pTFrame->height / 2),
                             &pTFrame->psb_surface->buf);
        break;
    case IMG_CODEC_IMC2:
    case IMG_CODEC_PL12:
    case IMG_CODEC_NV12:
        RELOC_PIC_PARAMS_PNW(&pContext->pMTXSetup->ComponentPlane[1].ui32PhysAddr,
                             srf_buf_offset + pTFrame->psb_surface->stride * pTFrame->height,
                             &pTFrame->psb_surface->buf);
        //Byte interleaved surface, so need to force chroma to use single surface by fooling it into
        //thinking it's dealing with standard 8x8 planaerblocks
        RELOC_PIC_PARAMS_PNW(&pContext->pMTXSetup->ComponentPlane[2].ui32PhysAddr,
                             srf_buf_offset + pTFrame->psb_surface->stride * pTFrame->height
                             + 8,
                             &pTFrame->psb_surface->buf);
        break;
    case IMG_CODEC_YV16:
        /*V*/
        RELOC_PIC_PARAMS_PNW(&pContext->pMTXSetup->ComponentPlane[2].ui32PhysAddr,
                             srf_buf_offset + pTFrame->psb_surface->stride * pTFrame->height,
                             &pTFrame->psb_surface->buf);
        /*U*/
        RELOC_PIC_PARAMS_PNW(&pContext->pMTXSetup->ComponentPlane[1].ui32PhysAddr,
                             srf_buf_offset + pTFrame->psb_surface->stride * pTFrame->height
                             + (pTFrame->psb_surface->stride) *(pTFrame->height) / 2,
                             &pTFrame->psb_surface->buf);
        break;
    default:
        drv_debug_msg(VIDEO_DEBUG_ERROR, " Not supported FOURCC %x!\n", pContext->eFormat);
        return -1;

    }


    drv_debug_msg(VIDEO_DEBUG_GENERAL, "TOPAZ_PDUMP: ui32DataInterleaveStatus %x\n", pContext->pMTXSetup->ui32DataInterleaveStatus);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "TOPAZ_PDUMP: ui32TableA %x \n", pContext->pMTXSetup->ui32TableA);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "TOPAZ_PDUMP:ui32ComponentsInScan %x\n", pContext->pMTXSetup->ui32ComponentsInScan);
    for (i32Lp = 0; i32Lp < 3; i32Lp++) {
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "TOPAZ_PDUMP: ComponentPlane[%d]: 0x%x, %d, %d\n",
                                 i32Lp, pContext->pMTXSetup->ComponentPlane[i32Lp].ui32PhysAddr, pContext->pMTXSetup->ComponentPlane[i32Lp].ui32Stride, pContext->pMTXSetup->ComponentPlane[i32Lp].ui32Height);
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "TOPAZ_PDUMP: MCUComponent[%d]: %d, %d, %d, %d\n", i32Lp, pContext->pMTXSetup->MCUComponent[i32Lp].ui32WidthBlocks, pContext->pMTXSetup->MCUComponent[i32Lp].ui32HeightBlocks, pContext->pMTXSetup->MCUComponent[i32Lp].ui32XLimit, pContext->pMTXSetup->MCUComponent[i32Lp].ui32YLimit);
    }

    i32Lp = ctx->NumCores;
    while (--i32Lp > -1) {
        /*TOPAZ_InsertCommand(
                pEncContext,
                i32Lp,
                MTX_CMDID_SETUP,
                IMG_FALSE,
                IMG_NULL,
                pContext->pMemInfoMTXSetup );*/

        pnw_cmdbuf_insert_command_package(ctx->obj_context,
                                          i32Lp,
                                          MTX_CMDID_SETUP,
                                          &(ctx->obj_context->pnw_cmdbuf->header_mem),
                                          0);
    }

    return ReturnCode;
}

#if 0
IMG_UINT32 Legacy_JPGEncodeSOSHeader(LEGACY_JPEG_ENCODER_CONTEXT *pContext, IMG_CODED_BUFFER *pCBuffer)
{
    IMG_UINT32  ui32TableIndex ;
    IMG_UINT8 uc_comp_id, ui8Comp;
    STREAMTYPEW s_streamW;
    IMG_UINT8 *puc_stream_buff;

    puc_stream_buff = (IMG_UINT8 *)(pCBuffer->pMemInfo) + pCBuffer->ui32BytesWritten;

    s_streamW.Offset = 0;
    s_streamW.Buffer = puc_stream_buff;
    s_streamW.Limit = (pCBuffer->ui32Size - pCBuffer->ui32BytesWritten);

    /* Start of scan */
    fPutBitsToBuffer(&s_streamW, 2, START_OF_SCAN);
    /* Scan header length */
    fPutBitsToBuffer(&s_streamW, 2, 6 + (pContext->JPEGEncoderParams.uc_num_comp_in_scan << 1));
    /* Number of image components in scan */
    fPutBitsToBuffer(&s_streamW, 1, pContext->JPEGEncoderParams.uc_num_comp_in_scan);

    if (pContext->JPEGEncoderParams.uc_num_comp_in_scan > MAX_COMP_IN_SCAN)
        pContext->JPEGEncoderParams.uc_num_comp_in_scan = MAX_COMP_IN_SCAN;
    for (ui8Comp = 0; ui8Comp < pContext->JPEGEncoderParams.uc_num_comp_in_scan; ui8Comp++) {
        uc_comp_id = pContext->JPEGEncoderParams.puc_comp_id[ui8Comp];
        if (uc_comp_id >= MAX_COMP_IN_SCAN) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "Invalide component index %d\n", uc_comp_id);
            uc_comp_id = MAX_COMP_IN_SCAN - 1;
        }

        ui32TableIndex  = pContext->sJPEGEncoderComp.CompIdtoIndex[uc_comp_id];

        /* Scan component selector */
        fPutBitsToBuffer(&s_streamW, 1, uc_comp_id);

        /*4 Bits Dc entropy coding table destination selector */
        /*4 Bits Ac entropy coding table destination selector */
        ui32TableIndex %= 255;
        fPutBitsToBuffer(&s_streamW, 1, (pContext->sJPEGEncoderComp.puc_huff_table_id[ui32TableIndex] << 4) | pContext->sJPEGEncoderComp.puc_huff_table_id[ui32TableIndex]);
    }

    /* Start of spectral or predictor selection  */
    fPutBitsToBuffer(&s_streamW, 1, 0);
    /* End of spectral selection */
    fPutBitsToBuffer(&s_streamW, 1, 63);
    /*4 Bits Successive approximation bit position high (0)*/
    /*4 Bits Successive approximation bit position low or point transform (0)*/
    fPutBitsToBuffer(&s_streamW, 1, 0);

    pCBuffer->ui32BytesWritten += s_streamW.Offset;

    return 0;
}
#endif

/***********************************************************************************
 * Function Name      : JPGEncodeSOSHeader
 * Inputs             : PContext - Pointer to JPEG context
 * Inputs             : pui8BitStreamBuffer start of the coded output buffer
 * Inputs             : pui32BytesWritten pointer to offset into the coded output buffer at which to write
 * Outputs            : pui8BitStreamBuffer - Start of Scan header written to the coded buffer
 * Returns            : Bytes writtren
 * Description        : This function writes the Start of Scan Header
 ************************************************************************************/

IMG_UINT32 JPGEncodeSOSHeader(/*in */           TOPAZSC_JPEG_ENCODER_CONTEXT *pContext,
        /*out */        IMG_UINT8*              pui8BitStreamBuffer ,
        /*out*/         IMG_UINT32*             pui32BytesWritten)
{
    IMG_UINT8 uc_comp_id, ui8Comp;
    STREAMTYPEW s_streamW;

    s_streamW.Offset = 0;
    s_streamW.Buffer = pui8BitStreamBuffer + *pui32BytesWritten;

    /* Start of scan */
    fPutBitsToBuffer(&s_streamW, 2, START_OF_SCAN);
    /* Scan header length */
    fPutBitsToBuffer(&s_streamW, 2, 6 + (pContext->pMTXSetup->ui32ComponentsInScan << 1));
    /* Number of image components in scan */
    fPutBitsToBuffer(&s_streamW, 1, pContext->pMTXSetup->ui32ComponentsInScan);
    for (ui8Comp = 0; ui8Comp < pContext->pMTXSetup->ui32ComponentsInScan; ui8Comp++) {
        uc_comp_id = ui8Comp + 1;

        /* Scan component selector */
        fPutBitsToBuffer(&s_streamW, 1, uc_comp_id);

        /*4 Bits Dc entropy coding table destination selector */
        /*4 Bits Ac entropy coding table destination selector */
        fPutBitsToBuffer(&s_streamW, 1, ((ui8Comp != 0 ? 1 : 0) << 4) | (ui8Comp != 0 ? 1 : 0)); // Huffman table refs = 0 Luma 1 Chroma
    }

    /* Start of spectral or predictor selection  */
    fPutBitsToBuffer(&s_streamW, 1, 0);
    /* End of spectral selection */
    fPutBitsToBuffer(&s_streamW, 1, 63);
    /*4 Bits Successive approximation bit position high (0)*/
    /*4 Bits Successive approximation bit position low or point transform (0)*/
    fPutBitsToBuffer(&s_streamW, 1, 0);

    *pui32BytesWritten += s_streamW.Offset;

    return 0;
}

#if 0
/*****************************************************************************/
/*                                                                           */
/* Function Name : EncodeMJPEGAPP1Marker                                     */
/*                                                                           */
/* Description   : Writes a Quicktime MJPEG (A) marker                                       */
/*                                                                           */
/* Inputs        :                                                           */
/*                                      pContext                        - Ptr to the JPEG encoder context        */
/*                                      puc_stream_buff         - Ptr to the bistream buffer from        */
/*                                                                                      where to start writing                   */
/*                                                                           */
/* Processing    :                                                                                                                       */
/*                                                                           */
/* Returns       : Number of bytes written into the stream                   */
/*                                                                           */
/*****************************************************************************/
IMG_UINT32 Insert_QT_MJPEGA_APP1Marker(IMG_CODED_BUFFER *pCBuffer, IMG_UINT8 *pui8BitStreamBuffer, IMG_UINT32 ui32OffsetBytes, IMG_BOOL bIsFirstField)
{
    STREAMTYPEW ps_streamW;
#ifdef JPEG_VERBOSE
    printf("Insert_QT_MJPEGA_APP1Marker");
#endif

    ps_streamW.Offset = 0;
    ps_streamW.Buffer = &pui8BitStreamBuffer[ui32OffsetBytes];

    /* Writing the start of image marker */
    fPutBitsToBuffer(&ps_streamW, 2, START_OF_IMAGE);
    /* Writing the Motion-JPEG APP1 marker */
    fPutBitsToBuffer(&ps_streamW, 2, MJPEG_APP1);
    /* Marker content length */
    fPutBitsToBuffer(&ps_streamW, 2, 0x002A);
    /* Reserved, set to zero */
    fPutBitsToBuffer(&ps_streamW, 4, 0x00000000);
    /* Motion-JPEG tag = "mjpg"*/
    fPutBitsToBuffer(&ps_streamW, 4, 0x6D6A7067);
    /* Field size*/
    fPutBitsToBuffer(&ps_streamW, 4, pCBuffer->ui32BytesWritten + 2 - ui32OffsetBytes); // +2 because the EOI marker not written yet
    /* Padded Field size*/
    fPutBitsToBuffer(&ps_streamW, 4, pCBuffer->ui32BytesWritten + 2 - ui32OffsetBytes);
    /* Offset to next field*/
    if (bIsFirstField)
        fPutBitsToBuffer(&ps_streamW, 4, pCBuffer->ui32BytesWritten + 2 - ui32OffsetBytes); // Points to start of next field
    else
        fPutBitsToBuffer(&ps_streamW, 4, 0); // Set to zero for second field

    /* Quantization Table Offset*/
    // May be able to leave this at zero
    fPutBitsToBuffer(&ps_streamW, 4, H_QT_OFFSET);
    /* Huffman Table Offset*/
    // May be able to leave this at zero
    fPutBitsToBuffer(&ps_streamW, 4, H_HT_OFFSET);
    /* Start Of Frame Offset*/
    fPutBitsToBuffer(&ps_streamW, 4, H_SOF_OFFSET);
    /* Start Of Scan Offset*/
    fPutBitsToBuffer(&ps_streamW, 4, H_SOS_OFFSET);
    /* Start of data offset*/
    fPutBitsToBuffer(&ps_streamW, 4, H_SOI_OFFSET);

    return 0;
}
#endif

/* JPEG Start picture function. Sets up all context information, Quantization details, Header output and MTX ready for the main encode loop*/
IMG_ERRORCODE SetupJPEGTables(TOPAZSC_JPEG_ENCODER_CONTEXT * pContext, IMG_CODED_BUFFER *pCBuffer,  object_surface_p pTFrame)
{
    /*IMG_UINT32 rc = 0;
    const IMG_UINT8* StandardQuantLuma;
    const IMG_UINT8* StandardQuantChroma;
    IMG_UINT8* QuantLumaTableOnHost;
    IMG_UINT8* QuantChromaTableOnHost;
    JPEG_MTX_QUANT_TABLE * psQuantTables;*/
    IMG_UINT16 ui16Lp;
    IMG_UINT8 ui8Planes[MAX_COMP_IN_SCAN];
    /*IMG_ENC_CONTEXT * pEncContext = (IMG_ENC_CONTEXT*)hEncContext;*/
    /*TOPAZSC_JPEG_ENCODER_CONTEXT * pContext = pEncContext->psJpegHC;*/
    IMG_INT8 i;
    context_ENC_p ctx = (context_ENC_p)pContext->ctx;

    // Lets add a pointer from our context to the source surface structure (remember we still need to use GETBUFFER commands if we want to get shared image memory, but info data structures are on host)
    pContext->pSourceSurface = pTFrame;

    //Try setting MCU details from here
    /*JPGEncodeBegin(pContext, pTFrame);*/
    InitializeJpegEncode(pContext, pTFrame);

    // Special case for YUYV format
    if (ISCHROMAINTERLEAVED(pContext->eFormat) > C_INTERLEAVE)
        pContext->sScan_Encode_Info.ui32NumberMCUsX = (pContext->pMTXSetup->MCUComponent[0].ui32XLimit + ((pContext->pMTXSetup->MCUComponent[0].ui32WidthBlocks * 2) - 1)) / (pContext->pMTXSetup->MCUComponent[0].ui32WidthBlocks * 2);
    else
        pContext->sScan_Encode_Info.ui32NumberMCUsX = (pContext->pMTXSetup->MCUComponent[0].ui32XLimit + (pContext->pMTXSetup->MCUComponent[0].ui32WidthBlocks - 1)) / pContext->pMTXSetup->MCUComponent[0].ui32WidthBlocks;

    pContext->sScan_Encode_Info.ui32NumberMCUsY = (pContext->pMTXSetup->MCUComponent[0].ui32YLimit + (pContext->pMTXSetup->MCUComponent[0].ui32HeightBlocks - 1)) / pContext->pMTXSetup->MCUComponent[0].ui32HeightBlocks;
    pContext->sScan_Encode_Info.ui32NumberMCUsToEncode = pContext->sScan_Encode_Info.ui32NumberMCUsX * pContext->sScan_Encode_Info.ui32NumberMCUsY;

    pContext->sScan_Encode_Info.ui32NumberMCUsToEncodePerScan =
        JPEG_MCU_PER_SCAN(pContext->ui32OutputWidth, pContext->ui32OutputHeight, ctx->NumCores, pContext->eFormat);

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "MCUs To Encode %dx%d\n",
                             pContext->sScan_Encode_Info.ui32NumberMCUsX,
                             pContext->sScan_Encode_Info.ui32NumberMCUsY);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "Total MCU %d, per scan %d\n", pContext->sScan_Encode_Info.ui32NumberMCUsToEncode, pContext->sScan_Encode_Info.ui32NumberMCUsToEncodePerScan);

    //Allocate our coded data buffers here now we know the number of MCUs we're encoding
    /*Use slice parameter buffer*/
    if (AllocateCodedDataBuffers(pContext) != IMG_ERR_OK) return IMG_ERR_MEMORY;
    // Send Setup message to MTX to initialise it
    /*EncodeInitMTX(pEncContext);        Set predictors to zero */

    for (i = ctx->NumCores - 1; i >= 0; i--) {
        pnw_cmdbuf_insert_command_package(
            ((context_ENC_p)pContext->ctx)->obj_context,
            i,
            MTX_CMDID_RESET_ENCODE,
            NULL,
            0);
    }
    //CODE HERE SIMPLIFIED AS WE KNOW THERE WILL ONLY EVER BE 3 IMAGE COMPONENTS
    /* Set up Source panes for HW */
    // Reverse colour plane pointers fed to MTX when required by format

    switch (pContext->eFormat) {
    case IMG_CODEC_YV16:
        pContext->pMTXSetup->ComponentPlane[0].ui32Stride = pTFrame->psb_surface->stride;
        pContext->pMTXSetup->ComponentPlane[1].ui32Stride = pContext->pMTXSetup->ComponentPlane[0].ui32Stride / 2;
        pContext->pMTXSetup->ComponentPlane[2].ui32Stride = pContext->pMTXSetup->ComponentPlane[0].ui32Stride / 2;

        pContext->pMTXSetup->ComponentPlane[0].ui32Height = pTFrame->height;
        pContext->pMTXSetup->ComponentPlane[1].ui32Height = pTFrame->height;
        pContext->pMTXSetup->ComponentPlane[2].ui32Height = pTFrame->height;

        /*YV16's plane order is Y, V, U*/
        ui8Planes[0] = 0;
        ui8Planes[2] = 1;
        ui8Planes[1] = 2;
        break;
    case IMG_CODEC_IYUV:
    case IMG_CODEC_IMC2:
        /*case IMG_CODEC_422_IMC2:
            SetupYUVPlaneDetails(&(pContext->pMTXSetup->ComponentPlane[0]),
                    &(pTFrame->aui32ComponentInfo[0]));

            SetupYUVPlaneDetails(&(pContext->pMTXSetup->ComponentPlane[2]),
                    &(pTFrame->aui32ComponentInfo[1]));

            SetupYUVPlaneDetails(&(pContext->pMTXSetup->ComponentPlane[1]),
                    &(pTFrame->aui32ComponentInfo[2]));*/
        pContext->pMTXSetup->ComponentPlane[0].ui32Stride = pTFrame->psb_surface->stride;
        pContext->pMTXSetup->ComponentPlane[1].ui32Stride = pContext->pMTXSetup->ComponentPlane[0].ui32Stride / 2;
        pContext->pMTXSetup->ComponentPlane[2].ui32Stride = pContext->pMTXSetup->ComponentPlane[0].ui32Stride / 2;

        pContext->pMTXSetup->ComponentPlane[0].ui32Height = pTFrame->height;
        pContext->pMTXSetup->ComponentPlane[1].ui32Height = pTFrame->height / 2;
        pContext->pMTXSetup->ComponentPlane[2].ui32Height = pTFrame->height / 2;

        //ui8Planes[0]=0;ui8Planes[1]=2;ui8Planes[2]=1;
        /*IYUV don't need to swap UV color space. Not sure about IMC12*/
        ui8Planes[0] = 0;
        ui8Planes[1] = 1;
        ui8Planes[2] = 2;
        break;
    default:
        /*SetupYUVPlaneDetails(&(pContext->pMTXSetup->ComponentPlane[0]),
            &(pTFrame->aui32ComponentInfo[0]));

        SetupYUVPlaneDetails(&(pContext->pMTXSetup->ComponentPlane[1]),
            &(pTFrame->aui32ComponentInfo[1]));

        SetupYUVPlaneDetails(&(pContext->pMTXSetup->ComponentPlane[2]),
            &(pTFrame->aui32ComponentInfo[2]));*/
        /* It's for NV12*/
        pContext->pMTXSetup->ComponentPlane[0].ui32Stride = pTFrame->psb_surface->stride;
        pContext->pMTXSetup->ComponentPlane[1].ui32Stride = pContext->pMTXSetup->ComponentPlane[0].ui32Stride;
        pContext->pMTXSetup->ComponentPlane[2].ui32Stride = pContext->pMTXSetup->ComponentPlane[0].ui32Stride;

        pContext->pMTXSetup->ComponentPlane[0].ui32Height = pTFrame->height;
        pContext->pMTXSetup->ComponentPlane[1].ui32Height = pTFrame->height / 2;
        pContext->pMTXSetup->ComponentPlane[2].ui32Height = pTFrame->height / 2;

        ui8Planes[0] = 0;
        ui8Planes[1] = 1;
        ui8Planes[2] = 2;
        break;
    };


    SetupIssueSetup(pContext, pContext->pMTXSetup->ui32ComponentsInScan, ui8Planes, pTFrame, 0 , 1);

    if (pContext->pMTXSetup->ui32ComponentsInScan > MAX_COMP_IN_SCAN) {
        return IMG_ERR_UNDEFINED;
    }

    if ((pCBuffer->ui32Size - pCBuffer->ui32BytesWritten) < 9 + 6 + (4 *(IMG_UINT32)pContext->pMTXSetup->ui32ComponentsInScan)) {
        return  IMG_ERR_MEMORY;
    }

    // Reset Scan Encode structures - just in case
    for (ui16Lp = 0; ui16Lp < pContext->sScan_Encode_Info.ui8NumberOfCodedBuffers; ui16Lp++) {
        BUFFER_HEADER *pbh;

        pContext->sScan_Encode_Info.aBufferTable[ui16Lp].i8MTXNumber = 0; // Indicates buffer is idle
        pContext->sScan_Encode_Info.aBufferTable[ui16Lp].ui16ScanNumber = 0; // Indicates buffer is idle

        pContext->sScan_Encode_Info.aBufferTable[ui16Lp].ui32DataBufferUsedBytes = 0;

        //pbh = MMGetHostLinAddress(pContext->sScan_Encode_Info.aBufferTable[ui16Lp].pMemInfo );
        pbh = (BUFFER_HEADER *)(pContext->sScan_Encode_Info.aBufferTable[ui16Lp].pMemInfo);
        pbh->ui32BytesEncoded = 0;
        pbh->ui32BytesUsed = sizeof(BUFFER_HEADER);
    }

    //Prefill out MTXIdleTable with MTX references (0 is the Master, and will be the last sent)
    for (pContext->sScan_Encode_Info.ui8MTXIdleCnt = 0; pContext->sScan_Encode_Info.ui8MTXIdleCnt < ctx->NumCores; pContext->sScan_Encode_Info.ui8MTXIdleCnt++) {
        pContext->sScan_Encode_Info.aui8MTXIdleTable[pContext->sScan_Encode_Info.ui8MTXIdleCnt] = pContext->sScan_Encode_Info.ui8MTXIdleCnt + 1;
    }


    //Need to set up CB Output slicenumber to equal number of slices required to encode image
    // Set current CB scan to maximum scan number (will count down as scans are output)
    pContext->sScan_Encode_Info.ui16CScan = (pContext->sScan_Encode_Info.ui32NumberMCUsToEncode + (pContext->sScan_Encode_Info.ui32NumberMCUsToEncodePerScan - 1)) / pContext->sScan_Encode_Info.ui32NumberMCUsToEncodePerScan;
    pContext->sScan_Encode_Info.ui16ScansInImage = pContext->sScan_Encode_Info.ui16CScan;
    // Set next scan to send to MTX to maximum scan number
    pContext->sScan_Encode_Info.ui16SScan = pContext->sScan_Encode_Info.ui16CScan;
    pContext->ui32InitialCBOffset = 0;

    return IMG_ERR_OK;
}

#if 0
IMG_ERRORCODE Legacy_PrepareHeader(LEGACY_JPEG_ENCODER_CONTEXT * pContext, IMG_CODED_BUFFER *pCBuffer)
{
    IMG_ERRORCODE rc;
    IMG_UINT8 *ui8OutputBuffer;

    ui8OutputBuffer = pCBuffer->pMemInfo;
    //Lock our JPEG Coded buffer
    /* if (IMG_JPEG_GetBuffer(pCBuffer, (IMG_VOID **) &ui8OutputBuffer)!=IMG_ERR_OK)
     return IMG_ERR_SURFACE_LOCKED;*/

    pCBuffer->ui32BytesWritten = 0;
    *((IMG_UINT32*) ui8OutputBuffer) = 0;

    // JPGEncodeMarker - Currently misses out the APP0 header
    rc = Legacy_JPGEncodeMarker(pContext, (IMG_UINT8 *) ui8OutputBuffer,  &pCBuffer->ui32BytesWritten);
    if (rc) return rc;

    rc = Legacy_JPGEncodeHeader(pContext , (IMG_UINT8 *) ui8OutputBuffer ,  &pCBuffer->ui32BytesWritten);
    if (rc) return rc;

    //  JPGEncodeDRIMarker(pContext,pCBuffer);
    Legacy_JPGEncodeSOSHeader(pContext, pCBuffer);

    //IMG_JPEG_ReleaseBuffer( pCBuffer);


    return IMG_ERR_OK;
}
/* Send header will work for each type of header*/
#endif

/***********************************************************************************
 * Function Name      : PrepareHeader
 * Inputs             : Pointer to JPEG Context, Point to coded buffer
 * Outputs            :
 * Returns            : IMG_ERRORCODE
 * Description        : Writes required JPEG Header elements to the coded buffer
 ************************************************************************************/
IMG_ERRORCODE PrepareHeader(TOPAZSC_JPEG_ENCODER_CONTEXT * pContext, IMG_CODED_BUFFER *pCBuffer, IMG_UINT32 ui32StartOffset, IMG_BOOL bIncludeHuffmanTables)
{
    IMG_ERRORCODE rc;
    IMG_UINT8 *ui8OutputBuffer;

    //Lock our JPEG Coded buffer
    /*if (IMG_C_GetBuffer((IMG_HENC_CONTEXT) pContext, pCBuffer, (IMG_VOID **) &ui8OutputBuffer)!=IMG_ERR_OK)
    return IMG_ERR_SURFACE_LOCKED;*/

    ui8OutputBuffer = (IMG_UINT8 *)pCBuffer->pMemInfo;
    pCBuffer->ui32BytesWritten = ui32StartOffset;
    *((IMG_UINT32*) ui8OutputBuffer + pCBuffer->ui32BytesWritten) = 0;


    // JPGEncodeMarker - Currently misses out the APP0 header
    rc = JPGEncodeMarker(pContext, (IMG_UINT8 *) ui8OutputBuffer,  &pCBuffer->ui32BytesWritten, bIncludeHuffmanTables);
    if (rc) return rc;

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "Current bytes of coded buf used: %d\n", pCBuffer->ui32BytesWritten);
    rc = JPGEncodeHeader(pContext , (IMG_UINT8 *) ui8OutputBuffer ,  &pCBuffer->ui32BytesWritten);
    if (rc) return rc;

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "Current bytes of coded buf used: %d\n", pCBuffer->ui32BytesWritten);
    rc = JPGEncodeSOSHeader(pContext, (IMG_UINT8 *) ui8OutputBuffer, &pCBuffer->ui32BytesWritten);
    if (rc) return rc;

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "Current bytes of coded buf used: %d\n", pCBuffer->ui32BytesWritten);
    /*IMG_C_ReleaseBuffer((IMG_HENC_CONTEXT) pContext, pCBuffer);*/
    return IMG_ERR_OK;
}




/***********************************************************************************
 * Function Name      : IMG_JPEG_ISSUEBUFFERTOHW
 * Inputs             :
 * Outputs            : pui32MCUsToProccess
 * Returns            :
 * Description        : Issues a buffer to MTX to recieve coded data.
 *
 ************************************************************************************/

IMG_ERRORCODE IssueBufferToHW(TOPAZSC_JPEG_ENCODER_CONTEXT *pContext, TOPAZSC_JPEG_BUFFER_INFO* pWriteBuf, IMG_UINT16 ui16BCnt, IMG_UINT32 ui32NoMCUsToEncode, IMG_INT8 i8MTXNumber)
{
    MTX_ISSUE_BUFFERS *psBufferCmd;
    context_ENC_p ctx = (context_ENC_p)pContext->ctx;
    /*IMG_ENC_CONTEXT * pEncContext = (IMG_ENC_CONTEXT*)hContext;
    TOPAZSC_JPEG_ENCODER_CONTEXT *pContext = pEncContext->psJpegHC;*/

#ifdef JPEG_VERBOSE
    printf("\n**************************************************************************\n");
    printf("** HOST SENDING Scan:%i (%i MCUs) to MTX %i, using Buffer %i\n", pWriteBuf->ui16ScanNumber, ui32NoMCUsToEncode, i8MTXNumber - 1, ui16BCnt);
#endif
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "HOST SENDING Scan:%d (%d MCUs, offset %d MCUs)"
                             " to MTX %d, using Buffer %d\n",
                             pWriteBuf->ui16ScanNumber, ui32NoMCUsToEncode,
                             pContext->sScan_Encode_Info.ui32CurMCUsOffset,
                             i8MTXNumber, ui16BCnt);

    // Issue to MTX ////////////////////////////
    ASSERT(pWriteBuf->pMemInfo);

    // Lets send our input parameters using the device memory allocated for the coded output
    psBufferCmd = (MTX_ISSUE_BUFFERS *)(pWriteBuf->pMemInfo);

    // We've disabled size bound checking in firmware, so can use the full 31 bits for NoMCUsToEncode instead
    //psBufferCmd->ui32MCUCntAndResetFlag       = ( DATA_BUFFER_SIZE(pContext->sScan_Encode_Info.ui32NumberMCUsToEncodePerScan) << 16) | (ui32NoMCUsToEncode << 1) | 0x1;

    psBufferCmd->ui32MCUCntAndResetFlag = (ui32NoMCUsToEncode << 1) | 0x1;
    psBufferCmd->ui32CurrentMTXScanMCUPosition = pContext->sScan_Encode_Info.ui32CurMCUsOffset;
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "TOPAZ_PDUMP: ui32MCUCntAndResetFlag 0x%x\n", psBufferCmd->ui32MCUCntAndResetFlag);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "TOPAZ_PDUMP: ui32CurrentMTXScanMCUPosition 0x%x\n", psBufferCmd->ui32CurrentMTXScanMCUPosition);
    /* We do not need to do this but in order for params to match on HW we need to know whats in the buffer*/
    /*MMUpdateDeviceMemory(pWriteBuf->pMemInfo );*/

    /*TOPAZ_InsertCommand(
        pEncContext,
        (IMG_INT32) (i8MTXNumber-1),
        MTX_CMDID_ISSUEBUFF,
        IMG_FALSE,
        &pWriteBuf->ui32WriteBackVal,
        pWriteBuf->pMemInfo );*/
    pnw_cmdbuf_insert_command_package(ctx->obj_context,
                                      (IMG_INT32)(i8MTXNumber) ,
                                      MTX_CMDID_ISSUEBUFF,
                                      ctx->coded_buf->psb_buffer,
                                      ui16BCnt * pContext->ui32SizePerCodedBuffer + PNW_JPEG_HEADER_MAX_SIZE);

    return IMG_ERR_OK;
}

/***********************************************************************************
 * Function Name      : SubmitScanToMTX
 * Inputs             :
 *                                              pContext - Pointer to JPEG context
 *                                              ui16BCnt - Index of the buffer to associate with the MTX
 *                                              i8MTXNumber - The ID number of the MTX that will have a scan sent to it
 * Outputs            : Status update to current buffer, MTX is activated
 * Returns            : Errorcode
 * Description        : Associates the empty buffer with the inactive MTX and then
 *                                              starts a scan, with output to be sent to the buffer.
 ************************************************************************************/

IMG_ERRORCODE SubmitScanToMTX(TOPAZSC_JPEG_ENCODER_CONTEXT *pContext,
                              IMG_UINT16 ui16BCnt,
                              IMG_INT8 i8MTXNumber,
                              IMG_UINT32 ui32NoMCUsToEncode)
{
    /*IMG_ENC_CONTEXT * pEncContext = (IMG_ENC_CONTEXT*)hContext;
    TOPAZSC_JPEG_ENCODER_CONTEXT *pContext = pEncContext->psJpegHC;*/


#if 0
    // Submit a scan and buffer to the appropriate MTX unit
    IMG_UINT32 ui32NoMCUsToEncode;

    if (pContext->sScan_Encode_Info.ui16SScan == 0) {
        // Final scan, may need fewer MCUs than buffer size, calculate the remainder
        ui32NoMCUsToEncode = pContext->sScan_Encode_Info.ui32NumberMCUsToEncode % pContext->sScan_Encode_Info.ui32NumberMCUsToEncodePerScan;
        if (ui32NoMCUsToEncode == 0)    ui32NoMCUsToEncode = pContext->sScan_Encode_Info.ui32NumberMCUsToEncodePerScan;
    } else
        ui32NoMCUsToEncode = pContext->sScan_Encode_Info.ui32NumberMCUsToEncodePerScan;
#endif
    //Set the buffer returned size to -1
    pContext->sScan_Encode_Info.aBufferTable[ui16BCnt].ui32DataBufferUsedBytes = ((BUFFER_HEADER*)(pContext->sScan_Encode_Info.aBufferTable[ui16BCnt].pMemInfo))->ui32BytesUsed = -1; // Won't be necessary with SC Peek commands enabled
    IssueBufferToHW(pContext, &(pContext->sScan_Encode_Info.aBufferTable[ui16BCnt]), ui16BCnt, ui32NoMCUsToEncode, i8MTXNumber);

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "Submitting scan %i to MTX %i and Buffer %i\n", pContext->sScan_Encode_Info.aBufferTable[ui16BCnt].ui16ScanNumber, i8MTXNumber, ui16BCnt);
    return IMG_ERR_OK;
}


void pnw_jpeg_set_default_qmatix(unsigned char *pMemInfoTableBlock)
{
    JPEG_MTX_QUANT_TABLE *pQTable = (JPEG_MTX_QUANT_TABLE *)pMemInfoTableBlock;
    memcpy(pQTable->aui8LumaQuantParams, gQuantLuma, QUANT_TABLE_SIZE_BYTES);
    memcpy(pQTable->aui8ChromaQuantParams, gQuantChroma, QUANT_TABLE_SIZE_BYTES);
    return;
}
