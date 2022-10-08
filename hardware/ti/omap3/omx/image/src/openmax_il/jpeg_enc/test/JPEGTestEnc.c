
/*
 *  Copyright 2001-2008 Texas Instruments - http://www.ti.com/
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 * limitations under the License.
 */
/* ====================================================================
*             Texas Instruments OMAP(TM) Platform Software
* (c) Copyright Texas Instruments, Incorporated. All Rights Reserved.
*
* Use of this software is controlled by the terms and conditions found
* in the license agreement under which this software has been supplied.
* ==================================================================== */
/**
* @file JPEGTestEnc.c
*
* This file implements OMX Component for JPEG encoder that
* is fully compliant with the OMX specification 1.5.
*
* @path  $(CSLPATH)\src
*
* @rev  0.1
*/
/* -------------------------------------------------------------------------------- */
/* ================================================================================
*!
*! Revision History
*! ===================================
*!
*! 22-May-2006 mf: Revisions appear in reverse chronological order;
*! that is, newest first.  The date format is dd-Mon-yyyy.
* ================================================================================= */

/*utilities includes */
#ifdef UNDER_CE
#include <windows.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h> 
#include <string.h> 
#include <sched.h> 
#include <unistd.h> 
#include <sys/types.h> 
#include <sys/select.h>
#include <time.h> 
//#include <mcheck.h>
#include <getopt.h>
#include <signal.h>

/* OMX includes */
#include <OMX_Component.h>
#include "OMX_JpegEnc_CustomCmd.h"
#include "JPEGTestEnc.h"
#include "OMX_JpegEnc_Utils.h"

/* DSP recovery includes */
#include <qosregistry.h>
#include <qosti.h>
#include <dbapi.h>
#include <DSPManager.h>
#include <DSPProcessor.h>
#include <DSPProcessor_OEM.h>

#define STRESS
#define NSTRESSCASES 1
#define STRESSMULTILOAD 1
#define NUM_OF_PORTS  2
#define NUM_OF_BUFFERSJPEG 1

#ifdef UNDER_CE
OMX_STRING StrJpegEncoder= "OMX.TI.IMAGE.JPEG.ENC"; 
#else
 OMX_STRING StrJpegEncoder= "OMX.TI.JPEG.Encoder";
#endif

OMX_U8 APPLICATION1_NOTHUMB[]={

/* 0 */0, 0, 0, 0, 0x45, 0x78, 0x69, 0x66, 0x00, 0x00,      /* Indicate Exif Data*/

/* 10 */ 0x49, 0x49,                                                  /* "Intel" type byte align*/

0x2A, 0x00,                                             /* Confirm "Intel" type byte align*/

/* 14 */ 0x08, 0x00, 0x00, 0x00,            /* Offset to first IFDc*/

0x06, 0x00,                     /* Number of IFD as 1*/

/* 21 */0x0f, 0x01,                     /* TAG: Make*/

0x02, 0x00,                    /* Type: Data format is 0x0002, ASCII */

0x0c, 0x00, 0x00, 0x00,  /* Count: number of chars*/

0x56, 0x00, 0x00, 0x00, /* Offset Make data*/

/* 33 */0x10, 0x01,                     /* TAG: Model*/

0x02, 0x00,                    /* Type: Data format is 0x0002, ASCII */

0x05, 0x00, 0x00, 0x00,  /* Count: number of chars*/

0x62, 0x00, 0x00, 0x00, /* Offset Model data*/

/*45*/0x12, 0x01,                     /* TAG: Orientation*/

0x03, 0x00,                    /* Type: Data format is 0x0003,  (short)*/

0x01, 0x00, 0x00, 0x00,  /* Count: number of chars*/

0x01, 0x00, 0x00, 0x00, /* 1 means normal orientation*/

/*57*/0x31, 0x01,                     /* TAG: Software*/

0x02, 0x00,                    /* Type: ASCII*/

0x08, 0x00, 0x00, 0x00,  /* Count: number of chars*/

0x67, 0x00, 0x00, 0x00, /* Offset*/

/*69*/0x3b, 0x01,                     /* TAG: Artist*/

0x02, 0x00,                    /* Type: ASCII*/

0x09, 0x00, 0x00, 0x00,  /* Count: number of chars*/

0x6f, 0x00, 0x00, 0x00, /* Offset*/

/* 81 */0x69, 0x87,                     /* Exif SubIFD*/

/* 83 */ 0x04, 0x00,                    /* Data format is 0x0004, ULInt*/

0x01, 0x00,  0x00, 0x00,            /* number of components is 1.*/

/*long integer data size is 4bytes/components, so total data length is 1x4=4bytes*/

/* 89 */ 0x78, 0x00,0x00, 0x00,             /* Offset of Exif data*/

/*93*/0x9E, 0x00,  0x00, 0x00,    /* Offset to next IFD. As we are saying only one directory( Number of IFD as 1) this indicate the offset of next IFD*/

/*97*/0x53, 0x61, 0x73, 0x6b, 0x65, 0x6e, 0x20, 0x26, 0x20, 0x54, 0x49, 0x00, /*Make*/

/*109*/0x4f, 0x4d, 0x41, 0x50,0x00, /*Model*/

/*114*/0x4f, 0x70, 0x65, 0x6e, 0x4d, 0x61, 0x78, 0x00, /*Software*/

/*122*/0x47, 0x65, 0x6f, 0x72, 0x67, 0x65, 0x20, 0x53, 0x00, /*Artist*/

/* exif ub-ID start Here */

/* 131 */ 0x03,  0x00,   /* Number of Exif ID*/

0x00, 0x90, /* Exif Version*/

0x07, 0x00,     /*Data format is 0x0007, undefined*/

0x04, 0x00, 0x00, 0x00,         /* number of components is 4.*/

/*Undefined data size is   1bytes/components, so total data length is 4x1=4bytes*/

0x30, 0x32, 0x32, 0x30, /* Exif version number 30 32 32 30 meaning 0220 (2.2)*/

/* next IFD start Here */

/*169*/0x03,  0x00,    /* Number of IFD1*/

/*171*/0x03,  0x01,    /* Compression  (0x0103)*/

0x03, 0x00,                     /*Data format is 0x0003 unsigned short,*/

0x01, 0x00,  0x00, 0x00,            /* number of components is 1.*/

/*unsigned short  data size is 2bytes/components, so total data length is 1x2=2bytes*/

/*183*/            0x01,  0x02,    /* JpegIFOffset  (0x0201)*/

                        0x04, 0x00,                     /* Data format is 0x0004, ULInt*/

                        0x01, 0x00,  0x00, 0x00,            /* number of components is 1.*/

/*195*/            0x02,  0x02,    /* JpegIFByteCount (0x0202)*/

                        0x04, 0x00,                     /* Data format is 0x0004, ULInt*/

                        0x01, 0x00,  0x00, 0x00,            /* number of components is 1.*/

/*203*/            0xff, 0xff,0xff, 0xff,  /* Legth of thumbnail data*/

};

OMX_U8 APPLICATION1_THUMB[]={
/* 0 */0, 0, 0, 0, 0x45, 0x78, 0x69, 0x66, 0x00, 0x00,      /* Indicate Exif Data*/
/* 10 */ 0x49, 0x49,                                                  /* "Intel" type byte align*/
0x2A, 0x00,                                             /* Confirm "Intel" type byte align*/
/* 14 */ 0x08, 0x00, 0x00, 0x00,            /* Offset to first IFDc*/
0x06, 0x00,                     /* Number of IFD as 1*/

/* 21 */0x0f, 0x01,                     /* TAG: Make*/
0x02, 0x00,                    /* Type: Data format is 0x0002, ASCII */
0x0c, 0x00, 0x00, 0x00,  /* Count: number of chars*/
0x56, 0x00, 0x00, 0x00, /* Offset Make data*/

/* 33 */0x10, 0x01,                     /* TAG: Model*/
0x02, 0x00,                    /* Type: Data format is 0x0002, ASCII */
0x05, 0x00, 0x00, 0x00,  /* Count: number of chars*/
0x62, 0x00, 0x00, 0x00, /* Offset Model data*/

/*45*/0x12, 0x01,                     /* TAG: Orientation*/
0x03, 0x00,                    /* Type: Data format is 0x0003,  (short)*/
0x01, 0x00, 0x00, 0x00,  /* Count: number of chars*/
0x01, 0x00, 0x00, 0x00, /* 1 means normal orientation*/

/*57*/0x31, 0x01,                     /* TAG: Software*/
0x02, 0x00,                    /* Type: ASCII*/
0x08, 0x00, 0x00, 0x00,  /* Count: number of chars*/
0x67, 0x00, 0x00, 0x00, /* Offset*/

/*69*/0x3b, 0x01,                     /* TAG: Artist*/
0x02, 0x00,                    /* Type: ASCII*/
0x09, 0x00, 0x00, 0x00,  /* Count: number of chars*/
0x6f, 0x00, 0x00, 0x00, /* Offset*/


/* 81 */0x69, 0x87,                     /* Exif SubIFD*/
/* 83 */ 0x04, 0x00,                    /* Data format is 0x0004, ULInt*/
0x01, 0x00,  0x00, 0x00,            /* number of components is 1.*/
/*long integer data size is 4bytes/components, so total data length is 1x4=4bytes*/
/* 89 */ 0x78, 0x00,0x00, 0x00,             /* Offset of Exif data*/

/*93*/0x9E, 0x00,  0x00, 0x00,    /* Offset to next IFD. As we are saying only one directory( Number of IFD as 1) this indicate the offset of next IFD*/

/*97*/0x53, 0x61, 0x73, 0x6b, 0x65, 0x6e, 0x20, 0x26, 0x20, 0x54, 0x49, 0x00, /*Make*/

/*109*/0x4f, 0x4d, 0x41, 0x50,0x00, /*Model*/

/*114*/0x4f, 0x70, 0x65, 0x6e, 0x4d, 0x61, 0x78, 0x00, /*Software*/

/*122*/0x47, 0x65, 0x6f, 0x72, 0x67, 0x65, 0x20, 0x53, 0x00, /*Artist*/

/* exif ub-ID start Here */
/* 131 */ 0x03,  0x00,   /* Number of Exif ID*/
0x00, 0x90, /* Exif Version*/
0x07, 0x00,     /*Data format is 0x0007, undefined*/
0x04, 0x00, 0x00, 0x00,         /* number of components is 4.*/
/*Undefined data size is   1bytes/components, so total data length is 4x1=4bytes*/
0x30, 0x32, 0x32, 0x30, /* Exif version number 30 32 32 30 meaning 0220 (2.2)*/

/*145*/0x02,  0xA0,    /* Exif Image Width  (0xA002)*/
0x04, 0x00,                     /* Data format is 0x0004, ULInt*/
0x01, 0x00,  0x00, 0x00,            /* number of components is 1.*/
/* 153 */ 0xB0, 0x00,0x00, 0x00,     /* Image width  , 0x00000280 i.e. 640*/

/*157*/0x03,  0xA0,    /* Exif Image Width  (0xA003)*/
0x04, 0x00,                     /* Data format is 0x0004, ULInt*/
0x01, 0x00,  0x00, 0x00,            /* number of components is 1.*/
/* 165 */ 0x90, 0x00,0x00, 0x00,     /* Image Height  , 0x000001E0 i.e. 480*/



/* next IFD start Here */
/*169*/0x03,  0x00,    /* Number of IFD1*/
/*171*/0x03,  0x01,    /* Compression  (0x0103)*/
0x03, 0x00,                     /*Data format is 0x0003 unsigned short,*/
0x01, 0x00,  0x00, 0x00,            /* number of components is 1.*/
/*unsigned short  data size is 2bytes/components, so total data length is 1x2=2bytes*/

0x06, 0x00,0x00, 0x00,  /* '6' means JPEG compression.*/
                        /*Shows compression method. 
                        o   '1' means no compression,
                        o    '6' means JPEG compression.*/
                        
/*183*/            0x01,  0x02,    /* JpegIFOffset  (0x0201)*/
                        0x04, 0x00,                     /* Data format is 0x0004, ULInt*/
                        0x01, 0x00,  0x00, 0x00,            /* number of components is 1.*/
/*191*/            0xc4, 0x00,0x00, 0x00,  /* Address 0f Thumbnail data*/
                        
/*195*/            0x02,  0x02,    /* JpegIFByteCount (0x0202)*/
                        0x04, 0x00,                     /* Data format is 0x0004, ULInt*/
                        0x01, 0x00,  0x00, 0x00,            /* number of components is 1.*/
/*203*/            0xff, 0xff,0xff, 0xff,  /* Legth of thumbnail data*/
};

/*Set the fist 4 bytes to 0*/
OMX_U8 APPLICATION13[200] = {
    0x00, 0x00, 0x00, 0x00, /*We should set the first 4 bytes to 0 */
    0x50, 0x68, 0x6f, 0x74, 0x6f, 0x73, 0x68, 0x6f, 0x70, 0x20, 0x33, 0x2e, 0x30, 0x00,/*Photoshop header*/
    0x38, 0x42, 0x49, 0x4d, 0x04, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x4e, /* Initial marker*/
    /*IPTC Marker       TAG         Size of string*/
    0x1c,      0x02,       0x78,       0x00, 0x20, 
    0x54, 0x68, 0x69, 0x73,  0x20, 0x69, 0x73, 0x20, 0x74, 0x68, 0x65, 0x20, 0x63, 0x61, 0x70, 0x74, 0x69, 0x6f, 0x6e, 0x20, 0x6f, 0x66, 0x20, 0x74, 0x68, 0x65, 0x20, 0x69, 0x6d, 0x61, 0x67, 0x65, /*String of data (ASCII)*/
    0x1c, 0x02, 0x7a, 0x00, 0x16, 
    0x49, 0x27,0x6d, 0x20, 0x74, 0x68, 0x65, 0x20, 0x63, 0x61, 0x70, 0x74, 0x69, 0x6f, 0x6e, 0x20, 0x77, 0x72, 0x69, 0x74, 0x65, 0x72,
    0x1c, 0x02, 0x5a, 0x00, 0x09,
    0x4d, 0x6f, 0x6e, 0x74, 0x65, 0x72, 0x72, 0x65, 0x79,

};

OMX_U8 APPLICATION0[19]={0, 0, 0, 0, 
	0x4A, 0x46, 0x49, 0x46, 0x00, // JFIF Identifier
	0x01, 0x02, // Version
	0x00, // X and Y Unit Densities.
	0x00, 0x08, // Horizontal Pixel density
	0x00, 0x09, // Vertical Pixel density
	0x00,
	0x00,
	0x00,
};

OMX_U8 APPLICATION5[6]={0xff,0xff,0xff,0xff,0xff,0xff};
const OMX_U8 CustomLumaQuantizationTable[64]= { 
14, 10, 9, 14, 22, 30, 41, 51,
12, 12, 14, 19, 26, 58, 60, 55, 
14, 13, 16, 24, 40, 57, 69, 56, 
14, 17, 22, 29, 51, 87, 80, 62, 
18, 22, 37, 56, 68, 109, 103, 77, 
24, 35, 55, 64, 81, 104, 113, 92, 
49, 64, 78, 87, 103, 121, 120, 101, 
72, 92, 95, 98, 112, 100, 103, 99,
};

const OMX_U8 CustomChromaQuantizationTable[64]= { 
15, 16, 22, 35, 99, 99, 99, 99,
18, 21, 26, 66, 99, 99, 99, 99, 
24, 26, 56, 99, 99, 99, 99, 99, 
47, 66, 99, 99, 99, 99, 99, 99, 
99, 99, 99, 99, 99, 99, 99, 99, 
99, 99, 99, 99, 99, 99, 99, 99, 
99, 99, 99, 99, 99, 99, 99, 99, 
99, 99, 99, 99, 99, 99, 99, 99 
}; 

JPEGENC_CUSTOM_HUFFMAN_TABLE CustomHuffmanTable = 
{
    /* VLC Tables */
    /*Set 1 for Y Component */
    /*DC-Y*/
    /*const unsigned int JPEGENC_DCHUFFY[12] */
    /*Length[16]-Codeword[16]*/
    {
        0x00020000,
        0x00030002,
        0x00030003,
        0x00030004,
        0x00030005,
        0x00030006,
        0x0004000e,
        0x0005001e,
        0x0006003e,
        0x0007007e,
        0x000800fe,
        0x000901fe
    },

    /*AC-Y*/
    /*const unsigned int JPEGENC_ACHUFFY[16][16] */
    /*Length[16]-Codeword[16]*/
    {
        {
            0x0004000a, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
            0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
            0x00000000, 0x00000000, 0x00000000, 0x000b07f9
        },
        {
            0x00020000, 0x0004000c, 0x0005001c, 0x0006003a, 0x0006003b, 0x0007007a,
            0x0007007b, 0x000800fa, 0x000901f8, 0x000901f9, 0x000901fa, 0x000a03f9,
            0x000a03fa, 0x000b07f8, 0x0010ffeb, 0x0010fff5
        },
        {
            0x00020001, 0x0005001b, 0x000800f9, 0x000901f7, 0x000a03f8, 0x000b07f7,
            0x000c0ff6, 0x000c0ff7, 0x000f7fc0, 0x0010ffbe, 0x0010ffc7, 0x0010ffd0,
            0x0010ffd9, 0x0010ffe2, 0x0010ffec, 0x0010fff6
        },
        {            
            0x00030004, 0x00070079, 0x000a03f7, 0x000c0ff5, 0x0010ff96, 0x0010ff9e,
            0x0010ffa6, 0x0010ffae, 0x0010ffb6, 0x0010ffbf, 0x0010ffc8, 0x0010ffd1,
            0x0010ffda, 0x0010ffe3, 0x0010ffed, 0x0010fff7
        },
        {
            0x0004000b, 0x000901f6, 0x000c0ff4, 0x0010ff8f, 0x0010ff97, 0x0010ff9f,
            0x0010ffa7, 0x0010ffaf, 0x0010ffb7, 0x0010ffc0, 0x0010ffc9, 0x0010ffd2,
            0x0010ffdb, 0x0010ffe4, 0x0010ffee, 0x0010fff8
        },
        {
            0x0005001a, 0x000b07f6, 0x0010ff89, 0x0010ff90, 0x0010ff98, 0x0010ffa0,
            0x0010ffa8, 0x0010ffb0, 0x0010ffb8, 0x0010ffc1, 0x0010ffca, 0x0010ffd3,
            0x0010ffdc, 0x0010ffe5, 0x0010ffef, 0x0010fff9
        },
        {
            0x00070078, 0x0010ff84, 0x0010ff8a, 0x0010ff91, 0x0010ff99, 0x0010ffa1,
            0x0010ffa9, 0x0010ffb1, 0x0010ffb9, 0x0010ffc2, 0x0010ffcb, 0x0010ffd4,
            0x0010ffdd, 0x0010ffe6, 0x0010fff0, 0x0010fffa
        },
        {
            0x000800f8, 0x0010ff85, 0x0010ff8b, 0x0010ff92, 0x0010ff9a, 0x0010ffa2,
            0x0010ffaa, 0x0010ffb2, 0x0010ffba, 0x0010ffc3, 0x0010ffcc, 0x0010ffd5,
            0x0010ffde, 0x0010ffe7, 0x0010fff1, 0x0010fffb
        },
        {
            0x000a03f6, 0x0010ff86, 0x0010ff8c, 0x0010ff93, 0x0010ff9b, 0x0010ffa3,
            0x0010ffab, 0x0010ffb3, 0x0010ffbb, 0x0010ffc4, 0x0010ffcd, 0x0010ffd6,
            0x0010ffdf, 0x0010ffe8, 0x0010fff2, 0x0010fffc
        },
        {
            0x0010ff82, 0x0010ff87, 0x0010ff8d, 0x0010ff94, 0x0010ff9c, 0x0010ffa4,
            0x0010ffac, 0x0010ffb4, 0x0010ffbc, 0x0010ffc5, 0x0010ffce, 0x0010ffd7,
            0x0010ffe0, 0x0010ffe9, 0x0010fff3, 0x0010fffd
        },
        {
            0x0010ff83, 0x0010ff88, 0x0010ff8e, 0x0010ff95, 0x0010ff9d, 0x0010ffa5,
            0x0010ffad, 0x0010ffb5, 0x0010ffbd, 0x0010ffc6, 0x0010ffcf, 0x0010ffd8,
            0x0010ffe1, 0x0010ffea, 0x0010fff4, 0x0010fffe
        },
        {
            0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
            0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
            0x00000000, 0x00000000, 0x00000000, 0x00000000
        },
        {
            0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
            0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
            0x00000000, 0x00000000, 0x00000000, 0x00000000
        },
        {
            0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
            0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
            0x00000000, 0x00000000, 0x00000000, 0x00000000
        },
        {
            0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
            0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
            0x00000000, 0x00000000, 0x00000000, 0x00000000
        },
        {
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000
        }
    },

    /*Set 2 for U & V Component */
    /*DC-UV*/
    /*const unsigned int JPEGENC_DCHUFFUV[12] */
    /*Length[16]-Codeword[16]*/
    {
        0x00020000,
        0x00020001,
        0x00020002,
        0x00030006,
        0x0004000e,
        0x0005001e,
        0x0006003e,
        0x0007007e,
        0x000800fe,
        0x000901fe,
        0x000a03fe,
        0x000b07fe
    },

    /*AC-UV*/
    /*const unsigned int JPEGENC_ACHUFFUV[16][16] */
    /*Length[16]-Codeword[16]*/
    {
        {
            0x00020000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
            0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
            0x00000000, 0x00000000, 0x00000000, 0x000a03fa
        },
        {
            0x00020001, 0x0004000b, 0x0005001a, 0x0005001b, 0x0006003a, 0x0006003b,
            0x00070079, 0x0007007a, 0x000800f9, 0x000901f7, 0x000901f8, 0x000901f9,
            0x000901fa, 0x000b07f9, 0x000e3fe0, 0x000f7fc3
        },
        {            
            0x00030004, 0x00060039, 0x000800f7, 0x000800f8, 0x000901f6, 0x000a03f9,
            0x000b07f7, 0x000b07f8, 0x0010ffb7, 0x0010ffc0, 0x0010ffc9, 0x0010ffd2,
            0x0010ffdb, 0x0010ffe4, 0x0010ffed, 0x0010fff6
        },
        {
            0x0004000a, 0x000800f6, 0x000a03f7, 0x000a03f8, 0x0010ff97, 0x0010ff9f,
            0x0010ffa7, 0x0010ffaf, 0x0010ffb8, 0x0010ffc1, 0x0010ffca, 0x0010ffd3,
            0x0010ffdc, 0x0010ffe5, 0x0010ffee, 0x0010fff7
        },
        {
            0x00050018, 0x000901f5, 0x000c0ff6, 0x000c0ff7, 0x0010ff98, 0x0010ffa0,
            0x0010ffa8, 0x0010ffb0, 0x0010ffb9, 0x0010ffc2, 0x0010ffcb, 0x0010ffd4,
            0x0010ffdd, 0x0010ffe6, 0x0010ffef, 0x0010fff8
        },
        {
            0x00050019, 0x000b07f6, 0x000f7fc2, 0x0010ff91, 0x0010ff99, 0x0010ffa1,
            0x0010ffa9, 0x0010ffb1, 0x0010ffba, 0x0010ffc3, 0x0010ffcc, 0x0010ffd5,
            0x0010ffde, 0x0010ffe7, 0x0010fff0, 0x0010fff9
        },
        {
            0x00060038, 0x000c0ff5, 0x0010ff8c, 0x0010ff92, 0x0010ff9a, 0x0010ffa2,
            0x0010ffaa, 0x0010ffb2, 0x0010ffbb, 0x0010ffc4, 0x0010ffcd, 0x0010ffd6,
            0x0010ffdf, 0x0010ffe8, 0x0010fff1, 0x0010fffa
        },
        {
            0x00070078, 0x0010ff88, 0x0010ff8d, 0x0010ff93, 0x0010ff9b, 0x0010ffa3,
            0x0010ffab, 0x0010ffb3, 0x0010ffbc, 0x0010ffc5, 0x0010ffce, 0x0010ffd7,
            0x0010ffe0, 0x0010ffe9, 0x0010fff2, 0x0010fffb
        },
        {
            0x000901f4, 0x0010ff89, 0x0010ff8e, 0x0010ff94, 0x0010ff9c, 0x0010ffa4,
            0x0010ffac, 0x0010ffb4, 0x0010ffbd, 0x0010ffc6, 0x0010ffcf, 0x0010ffd8,
            0x0010ffe1, 0x0010ffea, 0x0010fff3, 0x0010fffc
        },
        {
            0x000a03f6, 0x0010ff8a, 0x0010ff8f, 0x0010ff95, 0x0010ff9d, 0x0010ffa5,
            0x0010ffad, 0x0010ffb5, 0x0010ffbe, 0x0010ffc7, 0x0010ffd0, 0x0010ffd9,
            0x0010ffe2, 0x0010ffeb, 0x0010fff4, 0x0010fffd
        },
        {
            0x000c0ff4, 0x0010ff8b, 0x0010ff90, 0x0010ff96, 0x0010ff9e, 0x0010ffa6,
            0x0010ffae, 0x0010ffb6, 0x0010ffbf, 0x0010ffc8, 0x0010ffd1, 0x0010ffda,
            0x0010ffe3, 0x0010ffec, 0x0010fff5, 0x0010fffe
        },
        {
            0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
            0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
            0x00000000, 0x00000000, 0x00000000, 0x00000000
        },
        {
            0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
            0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
            0x00000000, 0x00000000, 0x00000000, 0x00000000
        },
        {
            0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
            0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
            0x00000000, 0x00000000, 0x00000000, 0x00000000
        },
        {
            0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
            0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
            0x00000000, 0x00000000, 0x00000000, 0x00000000
        },
        {
            0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
            0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
            0x00000000, 0x00000000, 0x00000000, 0x00000000
        }
    },

    /* DHT Marker */
    /* lum_dc_codelens */
    {0, 1, 5, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0},
    /* lum_dc_ncodes */
    16,
    /* lum_dc_symbols */
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11},
    /* lum_dc_nsymbols */
    12,
    /* lum_ac_codelens */
    {0, 2, 1, 3, 3, 2, 4, 3, 5, 5, 4, 4, 0, 0, 1, 0x7d},
    /* lum_ac_ncodes */
    16,
    /* lum_ac_symbols */
    {
        0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12,
        0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07,
        0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xa1, 0x08,
        0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52, 0xd1, 0xf0,
        0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0a, 0x16,
        0x17, 0x18, 0x19, 0x1a, 0x25, 0x26, 0x27, 0x28,
        0x29, 0x2a, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
        0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
        0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
        0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
        0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
        0x7a, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
        0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
        0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
        0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6,
        0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5,
        0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4,
        0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2,
        0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea,
        0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
        0xf9, 0xfa
    },

    /* lum_ac_nsymbols */
    162,
    /* chm_dc_codelens */
    {0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0},
    /* chm_dc_ncodes */
    16,
    /* chm_dc_symbols */
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11},
    /* chm_dc_nsymbols */
    12,
    /* chm_ac_codelens */
    {0, 2, 1, 2, 4, 4, 3, 4, 7, 5, 4, 4, 0, 1, 2, 0x77},
    /* chm_ac_ncodes */
    16,
    /* chm_ac_symbols */
    {
        0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21, 
        0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71,
        0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91,
        0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33, 0x52, 0xf0,
        0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34,
        0xe1, 0x25, 0xf1, 0x17, 0x18, 0x19, 0x1a, 0x26,
        0x27, 0x28, 0x29, 0x2a, 0x35, 0x36, 0x37, 0x38,
        0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
        0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
        0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
        0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
        0x79, 0x7a, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
        0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96,
        0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5,
        0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4,
        0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3,
        0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2,
        0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda,
        0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9,
        0xea, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
        0xf9, 0xfa
    },
    /* chm_ac_nsymbols */
    162
};


typedef unsigned char uchar;
/**
 * Pipe used to communicate back to the main thread from the component thread;
**/
int IpBuf_Pipe[2];
int OpBuf_Pipe[2];
int Event_Pipe[2];

/* the FLAG when we need to DeInit the OMX */
int DEINIT_FLAG = 0;

/* Flag set when component is preempted */
int bPreempted=0;

/* Hardware error flag */
OMX_BOOL bError = OMX_FALSE;

/*function prototypes */
inline int maxint(int a, int b);
OMX_ERRORTYPE SetMarkers(OMX_HANDLETYPE pHandle, IMAGE_INFO *imageinfo, OMX_CONFIG_RECTTYPE sCrop, int nWidth, int nHeight);

#ifdef DSP_MMU_FAULT_HANDLING
int LoadBaseImage();
#endif

/*Routine to get the maximum of 2 integers */
inline int maxint(int a, int b)
{
    return(a>b) ? a : b;
}


/**
 * This method will wait for the component or mixer to get to the correct
 * state.  It is not a real implementation, but just simulates what a real
 * wait routine may do.
**/
static OMX_ERRORTYPE WaitForState(OMX_HANDLETYPE* pHandle,
                                  OMX_STATETYPE DesiredState)
{
	OMX_STATETYPE CurState = OMX_StateInvalid;
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	int nCnt = 0;
	OMX_COMPONENTTYPE *pComponent = (OMX_COMPONENTTYPE *)pHandle;

	PRINT("Inside Test Application WaitForState function\n");
	eError = pComponent->GetState(pHandle, &CurState);
	while ( (eError == OMX_ErrorNone) &&
	        (CurState != DesiredState)) {
		sched_yield();
		/*sleep(1);*/
		if ( nCnt++ == 0xFFFFFFFE ) {
			fprintf(stderr, "Press CTL-C to continue\n");
		}

		eError = pComponent->GetState(pHandle, &CurState);
		if (CurState == OMX_StateInvalid && DesiredState != OMX_StateInvalid) {
			eError = OMX_ErrorInvalidState;
			break;
		}
	}

	if ( eError != OMX_ErrorNone ) {
		PRINT("Error: Couldn't get state for component or sent to invalid state because of an error.\n");
		return eError;
	}
	return OMX_ErrorNone;
}

OMX_ERRORTYPE EventHandler(OMX_HANDLETYPE hComponent,OMX_PTR pAppData,OMX_EVENTTYPE eEvent,
                           OMX_U32 nData1, OMX_U32 data2, OMX_PTR pEventData)
{
    OMX_COMPONENTTYPE *pComponent = (OMX_COMPONENTTYPE *)hComponent;
    OMX_STATETYPE state;
    OMX_ERRORTYPE eError;
    JPEGE_EVENTPRIVATE MyEvent;

    MyEvent.eEvent = eEvent;
    MyEvent.nData1 = nData1;
    MyEvent.nData2 = data2;
    MyEvent.pAppData = pAppData;
    MyEvent.pEventInfo = pEventData;
    PRINT("Inside Test Application EventHandler function\n");
    eError = pComponent->GetState (hComponent, &state);

    if ( eError != OMX_ErrorNone ) {
        PRINT("Error: From JPEGENC_GetState\n");
    }
    switch ( eEvent ) {
        
        case OMX_EventCmdComplete:
            PRINT ("Component State Changed\n");
            break;
            
        case OMX_EventError:
                if (nData1 == OMX_ErrorHardware){
                    printf("\n\nAPP:: ErrorNotification received: Error Num = %p Severity = %ld String  = %s\n", (OMX_PTR)nData1, data2, (OMX_STRING)pEventData);
                    printf("\nAPP:: OMX_ErrorHardware. Deinitialization of the component....\n\n");
                    if(!bError) {
                        bError = OMX_TRUE;
                        write(Event_Pipe[1], &MyEvent, sizeof(JPEGE_EVENTPRIVATE));
                    }
                }
                else if(nData1 == OMX_ErrorResourcesPreempted) {
                    bPreempted = 1;
                    PRINT("APP:: OMX_ErrorResourcesPreempted !\n\n");
                }
                else if(nData1 == OMX_ErrorInvalidState) {
                    printf("\n\nAPP:: ErrorNotification received: Error Num = %p Severity = %ld String	= %s\n", (OMX_PTR)nData1, data2, (OMX_STRING)pEventData);
                    printf("\nAPP:: Invalid state\n\n");
                    if(!bError) {
                        bError = OMX_TRUE;
                        write(Event_Pipe[1], &MyEvent, sizeof(JPEGE_EVENTPRIVATE));
                    }
                }
                else if(nData1 == OMX_ErrorPortUnpopulated) {
                    printf("APP:: OMX_ErrorPortUnpopulated\n");
    		    bError = OMX_TRUE;
                }
                else if (nData1 == OMX_ErrorStreamCorrupt) {
                    printf("\n\nAPP:: ErrorNotification received: Error Num = %p Severity = %ld String	= %s\n", (OMX_PTR)nData1, data2, (OMX_STRING)pEventData);
                    printf("%s: Stream Corrupt (%ld %s)\n",__FUNCTION__,data2,(char*)pEventData);
                    if(!bError) {
    			bError = OMX_TRUE;
    			write(Event_Pipe[1], &MyEvent, sizeof(JPEGE_EVENTPRIVATE));
    		    }
                }
                else {
    		    bError = OMX_TRUE;
                    DEINIT_FLAG = 1;
                }
                
            break;
            
        case OMX_EventResourcesAcquired:
            bPreempted = 0;
            break;  
            
        case OMX_EventPortSettingsChanged:
        case OMX_EventBufferFlag:
            PRINT("Event Buffer Flag detected\n");
        case OMX_EventMax:
        case OMX_EventMark:
            break;
            
        default:
                PRINT("ErrorNotification received: Error Num %p: String :%s\n", (OMX_PTR)nData1, (OMX_STRING)pEventData);
    }

    return OMX_ErrorNone;
}


void FillBufferDone (OMX_HANDLETYPE hComponent, OMX_PTR ptr,
                     OMX_BUFFERHEADERTYPE* pBuffHead)
{
    PRINT("Inside Test Application FillBufferDone function\n");
    write(OpBuf_Pipe[1], &pBuffHead, sizeof(pBuffHead));
}


void EmptyBufferDone(OMX_HANDLETYPE hComponent, OMX_PTR ptr,
                     OMX_BUFFERHEADERTYPE* pBuffer)
{
    PRINT("Inside Test Application EmptyBufferDone function\n");
    write(IpBuf_Pipe[1], &pBuffer, sizeof(pBuffer));
}

#ifdef UNDER_CE
int fill_data (OMX_BUFFERHEADERTYPE *pBufferPrivate, HANDLE fIn, int buffSize)
#else
int fill_data (OMX_BUFFERHEADERTYPE *pBuf, FILE *fIn, int buffSize)
#endif
{
    int nRead;
    OMX_U8 oneByte;    
    PRINT("Inside Test Application fill_data function\n");

#ifdef UNDER_CE
    ReadFile(fIn, pBuf->pBuffer, lSize, &nRead, NULL);
#else
    nRead = fread(pBuf->pBuffer,1, buffSize , fIn);
#endif  

    printf ("Buffer Size = %d. Read %d bytes from file. \n", (int) buffSize, (int)nRead);
    pBuf->nFilledLen = nRead;

    oneByte = fgetc(fIn);
    if (feof(fIn)){
        pBuf->nFlags = OMX_BUFFERFLAG_ENDOFFRAME;
        PRINT("Read full file...\n");        
        rewind(fIn);            
    }
    else{
        ungetc(oneByte, fIn);        
    }
    
    return nRead;
}

OMX_ERRORTYPE SetMarkers(OMX_HANDLETYPE pHandle, IMAGE_INFO *imageinfo, OMX_CONFIG_RECTTYPE sCrop, int nWidth, int nHeight) {

	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_INDEXTYPE nCustomIndex = OMX_IndexMax;
			
	/* Set APP0 Marker config (JFIF) */
	if (imageinfo->bAPP0) {

		JPEG_APPTHUMB_MARKER sAPP0;

		sAPP0.bMarkerEnabled = OMX_TRUE;
		
		/* set JFIF marker buffer */
		sAPP0.nMarkerSize = sizeof(APPLICATION0);
		sAPP0.pMarkerBuffer = APPLICATION0;
		sAPP0.nThumbnailWidth = imageinfo->nThumbnailWidth_app0;
		sAPP0.nThumbnailHeight = imageinfo->nThumbnailHeight_app0;
		
		eError = OMX_GetExtensionIndex(pHandle, "OMX.TI.JPEG.encoder.Config.APP0", (OMX_INDEXTYPE*)&nCustomIndex);
		if ( eError != OMX_ErrorNone ) {
			printf("%d::APP_Error at function call: %x\n", __LINE__, eError);
			eError = OMX_ErrorUndefined;
			goto EXIT;
		}
		eError = OMX_SetConfig(pHandle, nCustomIndex, &sAPP0); 
		if ( eError != OMX_ErrorNone ) {
			printf("%d::APP_Error at function call: %x\n", __LINE__, eError);
			eError = OMX_ErrorUndefined;
			goto EXIT;
		}

	}

	/* Set APP1 Marker config (EXIF) */
	if (imageinfo->bAPP1) {

		JPEG_APPTHUMB_MARKER sAPP1;

		sAPP1.bMarkerEnabled = OMX_TRUE;
		
		/* set JFIF marker buffer */
		sAPP1.nThumbnailWidth = imageinfo->nThumbnailWidth_app1;
		sAPP1.nThumbnailHeight = imageinfo->nThumbnailHeight_app1;

		/* if thumbnail is set, use APPLICATION structure with thumbnail */
		if(sAPP1.nThumbnailWidth > 0 && sAPP1.nThumbnailHeight > 0) {
			sAPP1.nMarkerSize = sizeof(APPLICATION1_THUMB);
			sAPP1.pMarkerBuffer = APPLICATION1_THUMB;
		}
		else {
			sAPP1.nMarkerSize = sizeof(APPLICATION1_NOTHUMB);
			sAPP1.pMarkerBuffer = APPLICATION1_NOTHUMB;
		}

		/*set crop */
		if (sCrop.nWidth != 0)
		{
			sAPP1.pMarkerBuffer[152] = sCrop.nWidth & 0xFF;
			sAPP1.pMarkerBuffer[153] = (sCrop.nWidth >> 8) & 0xFF;
		}
		else
		{
			sAPP1.pMarkerBuffer[152] = nWidth & 0xFF;
			sAPP1.pMarkerBuffer[153] = (nWidth >> 8) & 0xFF;
		}
		
		if (sCrop.nHeight != 0)
		{
			sAPP1.pMarkerBuffer[164] = sCrop.nHeight;
			sAPP1.pMarkerBuffer[165] = (sCrop.nHeight >> 8) & 0xFF;
		}
		else
		{
			sAPP1.pMarkerBuffer[164] = nHeight;
			sAPP1.pMarkerBuffer[165] = (nHeight >> 8) & 0xFF;
		}
		
		eError = OMX_GetExtensionIndex(pHandle, "OMX.TI.JPEG.encoder.Config.APP1", (OMX_INDEXTYPE*)&nCustomIndex);
		if ( eError != OMX_ErrorNone ) {
			printf("%d::APP_Error at function call: %x\n", __LINE__, eError);
			eError = OMX_ErrorUndefined;
			goto EXIT;
		}
		eError = OMX_SetConfig(pHandle, nCustomIndex, &sAPP1); 
		if ( eError != OMX_ErrorNone ) {
			printf("%d::APP_Error at function call: %x\n", __LINE__, eError);
			eError = OMX_ErrorUndefined;
			goto EXIT;
		}
	}

	/* Set APP5 Marker config	*/
	if (imageinfo->bAPP5) {

		JPEG_APPTHUMB_MARKER sAPP5;

		sAPP5.bMarkerEnabled = OMX_TRUE;
		
		/* set JFIF marker buffer */
		sAPP5.nMarkerSize = sizeof(APPLICATION5);
		sAPP5.pMarkerBuffer = APPLICATION5;
		sAPP5.nThumbnailWidth = imageinfo->nThumbnailWidth_app5;
		sAPP5.nThumbnailHeight = imageinfo->nThumbnailHeight_app5;
		
		eError = OMX_GetExtensionIndex(pHandle, "OMX.TI.JPEG.encoder.Config.APP5", (OMX_INDEXTYPE*)&nCustomIndex);
		if ( eError != OMX_ErrorNone ) {
			printf("%d::APP_Error at function call: %x\n", __LINE__, eError);
			eError = OMX_ErrorUndefined;
			goto EXIT;
		}
		eError = OMX_SetConfig(pHandle, nCustomIndex, &sAPP5); 
		if ( eError != OMX_ErrorNone ) {
			printf("%d::APP_Error at function call: %x\n", __LINE__, eError);
			eError = OMX_ErrorUndefined;
			goto EXIT;
		}

	}

	/* Set APP13 Marker config	*/
	if (imageinfo->bAPP13) {

		JPEG_APP13_MARKER sAPP13;

		sAPP13.bMarkerEnabled = OMX_TRUE;
		
		/* set JFIF marker buffer */
		sAPP13.nMarkerSize = sizeof(APPLICATION13);
		sAPP13.pMarkerBuffer = APPLICATION13;

		eError = OMX_GetExtensionIndex(pHandle, "OMX.TI.JPEG.encoder.Config.APP13", (OMX_INDEXTYPE*)&nCustomIndex);
		if ( eError != OMX_ErrorNone ) {
			printf("%d::APP_Error at function call: %x\n", __LINE__, eError);
			eError = OMX_ErrorUndefined;
			goto EXIT;
		}
		eError = OMX_SetConfig(pHandle, nCustomIndex, &sAPP13); 
		if ( eError != OMX_ErrorNone ) {
			printf("%d::APP_Error at function call: %x\n", __LINE__, eError);
			eError = OMX_ErrorUndefined;
			goto EXIT;
		}
	}

	/* set comment marker */
	if (imageinfo->nComment) {
		eError = OMX_GetExtensionIndex(pHandle, "OMX.TI.JPEG.encoder.Config.CommentFlag", (OMX_INDEXTYPE*)&nCustomIndex);
		if ( eError != OMX_ErrorNone ) {
			printf("%d::APP_Error at function call: %x\n", __LINE__, eError);
			eError = OMX_ErrorUndefined;
			goto EXIT;
		}
		eError = OMX_SetConfig(pHandle, nCustomIndex,  &(imageinfo->nComment));
		if ( eError != OMX_ErrorNone ) {
			printf("%d::APP_Error at function call: %x\n", __LINE__, eError);
			eError = OMX_ErrorUndefined;
			goto EXIT;
		}

		eError = OMX_GetExtensionIndex(pHandle, "OMX.TI.JPEG.encoder.Config.CommentString", (OMX_INDEXTYPE*)&nCustomIndex);
		if ( eError != OMX_ErrorNone ) {
			printf("%d::APP_Error at function call: %x\n", __LINE__, eError);
			eError = OMX_ErrorUndefined;
			goto EXIT;
		}
		eError = OMX_SetConfig(pHandle, nCustomIndex, imageinfo->pCommentString); 
		if ( eError != OMX_ErrorNone ) {
			printf("%d::APP_Error at function call: %x\n", __LINE__, eError);
			eError = OMX_ErrorUndefined;
			goto EXIT;
		}
	}
	
EXIT:
	return eError;
}

void PrintUsage(void)
{
    printf("\ni.. Input File\n");
    printf("o.. Output File\n");
    printf("w.. Width Of Image\n");
    printf("h.. Height Of Image\n");
    printf("f.. Input Of Image:\n    1.- YUV 420 Planer\n    2.- YUV 422 Interleaved UYVY\n    3.- 32 bit RAW (RGB32)\n    4.- 16 bit RAW (RGB16)\n    5.- YUV 422 Interleaved YUYV\n");
    printf("z.. 420p to 422i conversion before encode \n");
    printf("q.. Quality Factor Of Image: 1 to 100\n");
    printf("b.. Exit Buffer: 1 o 2\n");
    printf("c.. Marker Comment: The comment string length should be less than 255 characters\n");
    printf("j.. Marker APP0: Contain Information about JFIF\n");
    printf("e.. Marker APP1: Contain Information about EXIF\n");
    printf("d.. Marker APP5: Contain Miscellaneous  Information\n");	
    printf("m.. Marker APP13: Contain Miscellaneous  Information\n");
    printf("x.. Width of Thumbnail of EXIF (default set to 88)\n");
    printf("y.. Height of Thumbnail of EXIF (default set to 72)\n");
    printf("s.. Width of Thumbnail of JFIF (default set to 88)\n");
    printf("k.. Height of Thumbnail of JFIF (default set to 72)\n");
    printf("n.. Width of Thumbnail of APP5 (default set to 88)\n");
    printf("p.. Height of Thumbnail of APP5 (default set to 72)\n");
    printf("t.. Type of Quantization Table  \n\t 0.-Default Quantization Tables \n\t 1.-Custom Luma And Choma Tables\n");
    printf("u.. Type of Huffman Table       \n\t 0.-Default Huffman Table        \n\t 1.-Custom Huffman Table\n");            
    printf("r.. No. of times to repeat\n");
    printf("v.. Crop width value\n");           
    printf("l.. Crop height value\n");           
    
    printf("\na.. Prints this information\n");           
    printf("\nExample: ./JPEGTestEnc_common -i patterns/JPGE_CONF_003.yuv -o output.jpg -w 176 -h 144 -f 2 -q 100 -b 1 -c JPEG  -j -e -m -x 100 -y 100 -r 1\n\n");           
}


#ifdef UNDER_CE
int _tmain(int argc, TCHAR **argv) 
#else
int main(int argc, char** argv)
#endif
{

#ifndef UNDER_CE
//mtrace();
#endif
    OMX_HANDLETYPE pHandle;
    OMX_U32 AppData = 100;

    OMX_CALLBACKTYPE JPEGCaBa = {(void *)EventHandler,
                     (void*) EmptyBufferDone,
                     (void*)FillBufferDone};
    int retval;
    int nWidth;
    int nHeight;
    int framesent = 0;
    int nRepeated = 0;
    int maxRepeat = 1;
    int inputformat;
    int qualityfactor;
    int buffertype;
    int bSetCustomHuffmanTable=0;
    int bSetCustomQuatizationTable=0;    
    sigset_t set;	

    OMX_STATETYPE state;
    OMX_COMPONENTTYPE *pComponent;
    IMAGE_INFO* imageinfo = NULL;
    OMX_PORT_PARAM_TYPE* pPortParamType = NULL;
    OMX_IMAGE_PARAM_QFACTORTYPE* pQfactorType = NULL;
    JPEGENC_CUSTOM_HUFFMANTTABLETYPE *pHuffmanTable = NULL;
    OMX_IMAGE_PARAM_QUANTIZATIONTABLETYPE *pQuantizationTable = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE* pInPortDef = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE* pOutPortDef = NULL;
    OMX_CONFIG_RECTTYPE sCrop;

    OMX_BOOL bConvertion_420pTo422i = OMX_FALSE;

#ifdef UNDER_CE
    TCHAR* szInFile = NULL;
    TCHAR* szOutFile = NULL; 
    HANDLE fIn = NULL;
    HANDLE fOut = NULL;
    DWORD dwWritten;
#else
    char* szInFile = NULL;
    char* szOutFile = NULL;

    FILE* fIn = NULL;
    FILE* fOut = NULL;
#endif  

    OMX_BUFFERHEADERTYPE* pInBuff[NUM_OF_BUFFERSJPEG];
    OMX_BUFFERHEADERTYPE* pOutBuff[NUM_OF_BUFFERSJPEG];
    int nCounter = 0;
    int fdmax;
    OMX_U8* pTemp;
    OMX_U8* pInBuffer[NUM_OF_BUFFERSJPEG];
    OMX_U8* pOutBuffer[NUM_OF_BUFFERSJPEG];
    OMX_BUFFERHEADERTYPE* pBuffer;
    OMX_BUFFERHEADERTYPE* pBuf;    
    int nRead;
    int done;
    OMX_S32 sJPEGEnc_CompID = 300;
    int nIndex1= 0;
    int nIndex2 = 0;
    int nframerecieved = 0;
    int nMultFactor = 0;
    int nHeightNew, nWidthNew;
    OMX_INDEXTYPE nCustomIndex = OMX_IndexMax;
    OMX_ERRORTYPE error = OMX_ErrorNone;

#ifdef STRESS
    int multiload = 0;
#endif

    int next_option;
    const char* const short_options = "i:o:w:h:f:q:b:c:x:y:s:k:t:u:r:v:l:n:p:ajemdvlz";
    const struct option long_options[] = 
    {
        { "InputFile",1, NULL, 'i' },
        { "OutputFile",1, NULL, 'o' },
        { "WidthImage",1, NULL, 'w' },
        { "HeightImage",1, NULL, 'h' },
        { "inputFormat",1, NULL, 'f' },
        { "QualityFactor",1, NULL, 'q' },
        { "Ext/IntBuffer",1, NULL, 'b' },
        { "MarkerComment",1, NULL, 'c' },
        { "EXIFwidthThumbnail",1, NULL, 'x' },
        { "EXIFheightThumbnail",1, NULL, 'y' },
        { "JFIFwidthThumbnail",1, NULL, 's' },
        { "JFIFheightThumbnail",1, NULL, 'k' },
        { "APP5heightThumbnail",1, NULL, 'p' },
        { "APP5widthThumbnail",1, NULL, 'n' },
        { "Repetition",1, NULL, 'r' },        
 	 {"QuantizationTable",1,NULL,'t'},		
	 {"HuffmanTable",1,NULL,'u'}, 	 
        { "help", 0, NULL, 'a' },
        { "MarkerAPP0",0,NULL,'j'},
        { "MarkerAPP1",0,NULL,'e'},
        { "MarkerAPP13",0,NULL,'m'},        
        { "MarkerAPP5",0,NULL,'d'},
        { "CroppedWidth",0,NULL,'v'},               
        { "CroppedHeight",0,NULL,'l'},                       
        { "420pTo422iConversion",0,NULL,'z'},
        { NULL, 0, NULL, 0 }                
    };

    MALLOC(pPortParamType, OMX_PORT_PARAM_TYPE);
    MALLOC(pQfactorType, OMX_IMAGE_PARAM_QFACTORTYPE);
    MALLOC(pQuantizationTable,OMX_IMAGE_PARAM_QUANTIZATIONTABLETYPE);
    MALLOC(pHuffmanTable, JPEGENC_CUSTOM_HUFFMANTTABLETYPE);    
    MALLOC(pInPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
    MALLOC(pOutPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
    MALLOC(imageinfo, IMAGE_INFO);
    
    /* Setting up default parameters */
    szOutFile="output.jpg";
    nWidth=176;
    nHeight=144;
    inputformat=1;
    qualityfactor=100;
    buffertype=1;   

    imageinfo->nThumbnailWidth_app0 = 0;
    imageinfo->nThumbnailHeight_app0 = 0;
    imageinfo->nThumbnailWidth_app1 = 0;
    imageinfo->nThumbnailHeight_app1 = 0;
    imageinfo->nThumbnailWidth_app5 = 0;
    imageinfo->nThumbnailHeight_app5 = 0;
    imageinfo->nThumbnailWidth_app13 = 0;
    imageinfo->nThumbnailHeight_app13 = 0;

    imageinfo->bAPP0 = OMX_FALSE;
    imageinfo->bAPP1 = OMX_FALSE;
    imageinfo->bAPP5 = OMX_FALSE;
    imageinfo->bAPP13 = OMX_FALSE;
    imageinfo->nComment = OMX_FALSE;
    imageinfo->pCommentString = NULL;

    sCrop.nTop = 0;
    sCrop.nLeft = 0;
    sCrop.nWidth = 0;
    sCrop.nHeight = 0;
    
    if (argc <= 1)
    {
        PrintUsage();
        return 0;
    }    
    
do
{
    next_option = getopt_long(argc, argv, short_options,long_options, NULL);    
    switch(next_option)
    {
        case 'a':
            PrintUsage();
            return 0;
            break;
    
        case 'i':
        szInFile=optarg; 
        break;
        
        case 'o':
        szOutFile=optarg;
        break;
        
        case 'w':
         nWidth = atoi(optarg);
        break;
    
        case 'h':
        nHeight=atoi(optarg);
        break;
    
        case 'f':
        inputformat=atoi(optarg);
        break;
    
        case 'z':
        bConvertion_420pTo422i = OMX_TRUE;
        PRINT("\n ********* bConvertion_420pTo422i is set to TRUE \n");
        break;

        case 'q':
        qualityfactor=atoi(optarg);
        break;
    
        case 'b':
        buffertype=atoi(optarg);
        break;  
    
        case 'c':
        imageinfo->nComment = 1;
        imageinfo->pCommentString  = (char *)optarg;
        break;

	/*EXIF */
		
        case 'e':
        imageinfo->bAPP1 = OMX_TRUE;
        break;      

        case 'x':
        imageinfo->nThumbnailWidth_app1 = atoi(optarg);
        break;
        
        case 'y':
        imageinfo->nThumbnailHeight_app1 = atoi(optarg); 
        break;      

	/* JFIF */
        case 'j':
        imageinfo->bAPP0 = OMX_TRUE;
        break;     

        case 's':
        imageinfo->nThumbnailWidth_app0 = atoi(optarg);
        break;
        
        case 'k':
        imageinfo->nThumbnailHeight_app0 = atoi(optarg);
        break;      
        

        case 'n':
        imageinfo->nThumbnailWidth_app5 = atoi(optarg);
        break;
        
        case 'p':
        imageinfo->nThumbnailHeight_app5 = atoi(optarg);
        break;      

        case 'm':
        imageinfo->bAPP13 = OMX_TRUE;
        break;

        case 'd':
        imageinfo->bAPP5 = OMX_TRUE;
        break;
		
        case 't':
        bSetCustomQuatizationTable=atoi(optarg);
        break;

        case 'u':
        bSetCustomHuffmanTable=atoi(optarg);
        break;
    
        case 'r':
        maxRepeat = atoi(optarg);
        break;

        case 'v':
        sCrop.nWidth = atoi(optarg);
        break;
        
        case 'l':
        sCrop.nHeight = atoi(optarg);
        break;        

    }   
}while (next_option != -1);

	printf("\n------------------------------------------------\n");
	printf("OMX JPEG Encoder Test App built on " __DATE__ ":" __TIME__ "\n");
	printf("------------------------------------------------\n");
	printf("Output File Name is %s \n" , szOutFile);    

#ifdef STRESS

	for (multiload = 0; multiload < STRESSMULTILOAD; multiload ++) {
	    printf("Stress Test: Iteration %d\n", multiload + 1);
#endif


#ifdef DSP_MMU_FAULT_HANDLING
/* LOAD BASE IMAGE FIRST TIME */
        LoadBaseImage();
#endif

	error = TIOMX_Init();
	if ( error != OMX_ErrorNone ) {
	    PRINT("Error returned by OMX_Init()\n");
	    goto EXIT;
	}

	printf("OMX_Init Successful!\n");

	error = TIOMX_GetHandle(&pHandle,StrJpegEncoder,(void *)&AppData, &JPEGCaBa);
	if ( (error != OMX_ErrorNone) || (pHandle == NULL) ) {
	    fprintf (stderr,"Error in Get Handle function\n");
	    goto EXIT;
	}

#ifdef STRESS
	OMX_U8 i;
	for(i=0; i < NSTRESSCASES; i++) {
	     printf("Stress test number %d\n",i);
#endif

	/* Create a pipe used to queue data from the callback. */
	retval = pipe(IpBuf_Pipe);
	if ( retval != 0 ) {
	    fprintf(stderr, "Error:Fill Data Pipe failed to open\n");
	    goto EXIT;
	}

	retval = pipe(OpBuf_Pipe);
	if ( retval != 0 ) {
	    fprintf(stderr, "Error:Empty Data Pipe failed to open\n");
	    goto EXIT;
	}

	retval = pipe(Event_Pipe);
	if ( retval != 0 ) {
	    fprintf(stderr, "Error:Empty Data Pipe failed to open\n");
	    goto EXIT;
	}

	PRINT("Input/Output Pipes created\n");

	/* save off the "max" of the handles for the selct statement */
	fdmax = maxint(IpBuf_Pipe[0], OpBuf_Pipe[0]);
	fdmax = maxint(Event_Pipe[0], fdmax);

#ifdef UNDER_CE
	fIn = CreateFile(szInFile, GENERIC_READ, 0,
	                       NULL,OPEN_EXISTING, 0, NULL);
	if (INVALID_HANDLE_VALUE == fIn)
	{
	    PRINT("Error:  failed to open the file %s for " \
	         "reading\n", szInFile);
	    goto EXIT;
	}
#else
	fIn = fopen(szInFile, "r");
	if ( fIn == NULL ) {
	    printf("Error: failed to open the file <%s> for reading\n",
	          szInFile);
	    goto EXIT;
	}
	PRINT(" File %s opened \n" , szInFile);    
#endif  


	error = OMX_GetParameter(pHandle, OMX_IndexParamImageInit, pPortParamType);
	if ( error != OMX_ErrorNone ) {
	    printf("%d::APP_Error at function call: %x\n", __LINE__, error);        
	    error = OMX_ErrorBadParameter;
	    goto EXIT;
	}

	nIndex1 = pPortParamType->nStartPortNumber;
	nIndex2 = nIndex1 + 1;
	pInPortDef->nPortIndex = nIndex1;
	error = OMX_GetParameter (pHandle, OMX_IndexParamPortDefinition, pInPortDef);
	if ( error != OMX_ErrorNone ) {
	    printf("%d::APP_Error at function call: %x\n", __LINE__, error);
	    error = OMX_ErrorBadParameter;
	    goto EXIT;
	}

	if (pInPortDef->eDir == nIndex1 ) {
	    pInPortDef->nPortIndex = nIndex1;
	}
	else {
	    pInPortDef->nPortIndex = nIndex2;
	}

	/* Set the component's OMX_PARAM_PORTDEFINITIONTYPE structure (input) */
	pInPortDef->nSize = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
	pInPortDef->nVersion.s.nVersionMajor = 0x1;
	pInPortDef->nVersion.s.nVersionMinor = 0x0;
	pInPortDef->nVersion.s.nRevision = 0x0;
	pInPortDef->nVersion.s.nStep = 0x0;
	pInPortDef->nPortIndex = 0x0;
	pInPortDef->eDir = OMX_DirInput;
	pInPortDef->nBufferCountActual = NUM_OF_BUFFERSJPEG;
	pInPortDef->nBufferCountMin = 1;
	pInPortDef->bEnabled = OMX_TRUE;
	pInPortDef->bPopulated = OMX_FALSE;
	pInPortDef->eDomain = OMX_PortDomainImage;

	/* OMX_IMAGE_PORTDEFINITION values for input port */
	pInPortDef->format.image.cMIMEType = "JPEGENC";
	pInPortDef->format.image.pNativeRender = NULL;
	pInPortDef->format.image.nFrameWidth = nWidth;
	pInPortDef->format.image.nFrameHeight = nHeight;
	pInPortDef->format.image.nSliceHeight = -1;
	pInPortDef->format.image.bFlagErrorConcealment = OMX_FALSE;

	if ( inputformat == 2) {
	     pInPortDef->format.image.eColorFormat =  OMX_COLOR_FormatCbYCrY;
	} 
	else if ( inputformat == 3) {
	     pInPortDef->format.image.eColorFormat = OMX_COLOR_Format32bitARGB8888;
	}
	else if (inputformat == 4) {
		pInPortDef->format.image.eColorFormat = OMX_COLOR_Format16bitRGB565;
	}
	else if ( inputformat == 5) {
	     pInPortDef->format.image.eColorFormat = OMX_COLOR_FormatYCbYCr;
	}
	else {
	    pInPortDef->format.image.eColorFormat = OMX_COLOR_FormatYUV420PackedPlanar;
	}

	pInPortDef->format.image.eCompressionFormat = OMX_IMAGE_CodingUnused;

	nMultFactor = (nWidth + 16 - 1)/16;
	nWidthNew = (int)(nMultFactor) * 16;

	nMultFactor = (nHeight + 16 - 1)/16;
	nHeightNew = (int)(nMultFactor) * 16;

	if (inputformat == 1) {
	    pInPortDef->nBufferSize = (nWidthNew * nHeightNew * 1.5);
	    if (pInPortDef->nBufferSize < 1600) {
	        pInPortDef->nBufferSize = 1600;
	    }
	}
	else if(inputformat == 3) {
		pInPortDef->nBufferSize = (nWidthNew * nHeightNew * 4);
	}
	else {
	    pInPortDef->nBufferSize = (nWidthNew * nHeightNew * 2);
	    if (pInPortDef->nBufferSize < 400) {
	        pInPortDef->nBufferSize = 400;
	    }
	}


	error = OMX_SetParameter (pHandle, OMX_IndexParamPortDefinition, pInPortDef);
	if ( error != OMX_ErrorNone ) {
	    printf("%d::APP_Error at function call: %x\n", __LINE__, error);        
	    error = OMX_ErrorBadParameter;
	    goto EXIT;
	}

	pOutPortDef->nPortIndex = nIndex2;
	error = OMX_GetParameter(pHandle, OMX_IndexParamPortDefinition, pOutPortDef);
	if ( error != OMX_ErrorNone ) {
	    printf("%d::APP_Error at function call: %x\n", __LINE__, error);        
	    error = OMX_ErrorBadParameter;
	    goto EXIT;
	}
	if (pOutPortDef->eDir == nIndex1 ) {
	    pOutPortDef->nPortIndex = nIndex1;
	}
	else {
	    pOutPortDef->nPortIndex = nIndex2;
	}
	/* Set the component's OMX_PARAM_PORTDEFINITIONTYPE structure (input) */

	pOutPortDef->nSize = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
	pOutPortDef->nVersion.s.nVersionMajor = 0x1;
	pOutPortDef->nVersion.s.nVersionMinor = 0x0;
	pOutPortDef->nVersion.s.nRevision = 0x0;
	pOutPortDef->nVersion.s.nStep = 0x0;
	pOutPortDef->nPortIndex = 0x1;
	pOutPortDef->eDir = OMX_DirInput;
	pOutPortDef->nBufferCountActual = NUM_OF_BUFFERSJPEG;
	pOutPortDef->nBufferCountMin = 1;
	pOutPortDef->bEnabled = OMX_TRUE;
	pOutPortDef->bPopulated = OMX_FALSE;
	pOutPortDef->eDomain = OMX_PortDomainImage;

	/* OMX_IMAGE_PORTDEFINITION values for input port */
	pOutPortDef->format.image.cMIMEType = "JPEGENC";
	pOutPortDef->format.image.pNativeRender = NULL; 
	pOutPortDef->format.image.nFrameWidth = nWidth;
	pOutPortDef->format.image.nFrameHeight = nHeight;
	pOutPortDef->format.image.nStride = -1; 
	pOutPortDef->format.image.nSliceHeight = -1;
	pOutPortDef->format.image.bFlagErrorConcealment = OMX_FALSE;

	/*Minimum buffer size requirement */
	pOutPortDef->nBufferSize = (nWidth*nHeight);
	if( qualityfactor < 10){
	    pOutPortDef->nBufferSize /=10;
	}
	else if (qualityfactor <100){
	    pOutPortDef->nBufferSize /= (100/qualityfactor);
	}

	/*Adding memory to include Thumbnail, comments & markers information and header (depends on the app)*/
	pOutPortDef->nBufferSize += 12288;


	if ( inputformat == 2 || inputformat == 3 || inputformat == 4 ) {
	     pOutPortDef->format.image.eColorFormat =  OMX_COLOR_FormatCbYCrY; 
	}
	else if ( inputformat == 1 && bConvertion_420pTo422i ) {
	    pOutPortDef->format.image.eColorFormat =  OMX_COLOR_FormatCbYCrY;
	}
	else {
	    pOutPortDef->format.image.eColorFormat = OMX_COLOR_FormatYUV420PackedPlanar;
	}

	if (imageinfo->bAPP1) {
	    pOutPortDef->format.image.eCompressionFormat = OMX_IMAGE_CodingEXIF;
	}
	else {
	    pOutPortDef->format.image.eCompressionFormat = OMX_IMAGE_CodingJPEG;
	}

	error = OMX_SetParameter (pHandle, OMX_IndexParamPortDefinition, pOutPortDef);
	if ( error != OMX_ErrorNone ) {
	    printf("%d::APP_Error at function call: %x\n", __LINE__, error);        
	    error = OMX_ErrorBadParameter;
	    goto EXIT;
	}

	pComponent = (OMX_COMPONENTTYPE *)pHandle;

	error = OMX_SetConfig(pHandle, OMX_IndexConfigCommonInputCrop, &sCrop);
	if ( error != OMX_ErrorNone ) {
	    printf("%d::APP_Error at function call: %x\n", __LINE__, error);            
	   error = OMX_ErrorBadParameter;
	   goto EXIT;
	}
    
	error = OMX_SetConfig(pHandle, OMX_IndexCustomColorFormatConvertion_420pTo422i, &bConvertion_420pTo422i);
	if ( error != OMX_ErrorNone ) {
	    printf("%d::APP_Error at function call: %x\n", __LINE__, error);
	   error = OMX_ErrorBadParameter;
	   goto EXIT;
	}

	if (bSetCustomQuatizationTable){

		pQuantizationTable->eQuantizationTable = OMX_IMAGE_QuantizationTableLuma;
		error = OMX_GetParameter(pHandle, OMX_IndexParamQuantizationTable, pQuantizationTable);
	    if ( error != OMX_ErrorNone ) {
	        printf("%d::APP_Error at function call: %x\n", __LINE__, error);            
	       error = OMX_ErrorBadParameter;
	       goto EXIT;
	    }

	    pTemp = (OMX_U8*)memcpy(pQuantizationTable->nQuantizationMatrix, CustomLumaQuantizationTable, sizeof(CustomLumaQuantizationTable));
	    if(pTemp == NULL){
	        printf("%d::APP_Error at function call\n", __LINE__);             
	        error = OMX_ErrorUndefined;
	        goto EXIT;
	    }

	    error = OMX_SetParameter(pHandle, OMX_IndexParamQuantizationTable, pQuantizationTable);
	    if ( error != OMX_ErrorNone ) {
	        printf("%d::APP_Error at function call: %x\n", __LINE__, error);             
	       error = OMX_ErrorBadParameter;
	       goto EXIT;
	    }

	    pQuantizationTable->eQuantizationTable = OMX_IMAGE_QuantizationTableChroma;
	    error = OMX_GetParameter(pHandle, OMX_IndexParamQuantizationTable, pQuantizationTable);
	    if ( error != OMX_ErrorNone ) {
	        printf("%d::APP_Error at function call: %x\n", __LINE__, error); 
	       error = OMX_ErrorBadParameter;
	       goto EXIT;
	    }

	    pTemp = (OMX_U8*)memcpy(pQuantizationTable->nQuantizationMatrix, CustomChromaQuantizationTable, sizeof(CustomChromaQuantizationTable));
	    if(pTemp == NULL){
	        error = OMX_ErrorUndefined;
	        goto EXIT;
	    }

	    error = OMX_SetParameter(pHandle, OMX_IndexParamQuantizationTable, pQuantizationTable);
	    if ( error != OMX_ErrorNone ) {
	        printf("%d::APP_Error at function call: %x\n", __LINE__, error);             
	       error = OMX_ErrorBadParameter;
	       goto EXIT;
	    }
	}

	if (bSetCustomHuffmanTable){

	    error = OMX_GetExtensionIndex(pHandle, "OMX.TI.JPEG.encoder.Config.HuffmanTable", (OMX_INDEXTYPE*)&nCustomIndex);
	    if ( error != OMX_ErrorNone ) {
	        printf("%d::APP_Error at function call: %x\n", __LINE__, error);             
	       error = OMX_ErrorBadParameter;
	       goto EXIT;
	    }

		error = OMX_GetParameter(pHandle, nCustomIndex, pHuffmanTable);
	    if ( error != OMX_ErrorNone ) {
	        printf("%d::APP_Error at function call: %x\n", __LINE__, error);             
	       error = OMX_ErrorBadParameter;
	       goto EXIT;
	    }

	    pTemp = (OMX_U8*)memcpy(&(pHuffmanTable->sHuffmanTable), &CustomHuffmanTable, sizeof(CustomHuffmanTable));
	    if(pTemp == NULL){
	        error = OMX_ErrorUndefined;
	        goto EXIT;
	    }

	    error = OMX_SetParameter(pHandle, nCustomIndex, pHuffmanTable);
	    if ( error != OMX_ErrorNone ) {
	        printf("%d::APP_Error at function call: %x\n", __LINE__, error);             
	       error = OMX_ErrorBadParameter;
	       goto EXIT;
	    }
	}


	pComponent = (OMX_COMPONENTTYPE *)pHandle;
	error = OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateIdle ,NULL);
	if ( error != OMX_ErrorNone ) {
	    fprintf (stderr,"Error from SendCommand-Idle(Init) State function\n");
	    goto EXIT;
	}

	if (buffertype == 1) {
	    
	    for (nCounter = 0; nCounter < NUM_OF_BUFFERSJPEG; nCounter++) {
	        pTemp=(OMX_U8*)malloc(pInPortDef->nBufferSize+256);
	        if(pTemp == NULL){
	            error = OMX_ErrorInsufficientResources;
	            goto EXIT;
	        }
	        pTemp+= 128;
	        pInBuffer[nCounter] = pTemp;
	        pTemp = NULL;

	        pTemp= (OMX_U8*)malloc(pOutPortDef->nBufferSize+256);
	        if(pTemp == NULL){
	            error = OMX_ErrorInsufficientResources;
	            goto EXIT;
	        }
	        pTemp+= 128;
	        pOutBuffer[nCounter] = pTemp;
	    }

	    for (nCounter = 0; nCounter < NUM_OF_BUFFERSJPEG; nCounter++) {
	        error = OMX_UseBuffer(pHandle, &pInBuff[nCounter], nIndex1, (void*)&sJPEGEnc_CompID, pInPortDef->nBufferSize,pInBuffer[nCounter]);  
	    }
	    for (nCounter = 0; nCounter < NUM_OF_BUFFERSJPEG; nCounter++) {
	        error = OMX_UseBuffer(pHandle, &pOutBuff[nCounter], nIndex2, (void*)&sJPEGEnc_CompID, pOutPortDef->nBufferSize,pOutBuffer[nCounter]); 
	    }
	}

	if (buffertype == 2) {
	    for (nCounter = 0; nCounter < NUM_OF_BUFFERSJPEG; nCounter++) {
	        error = OMX_AllocateBuffer(pHandle, &pInBuff[nCounter], nIndex1, (void *)&sJPEGEnc_CompID, pInPortDef->nBufferSize);
	    }
	    for (nCounter = 0; nCounter < NUM_OF_BUFFERSJPEG; nCounter++) {
	        error = OMX_AllocateBuffer(pHandle, &pOutBuff[nCounter], nIndex2, (void *)&sJPEGEnc_CompID, pOutPortDef->nBufferSize);
	    }
	}

	/* set markers */
	error = SetMarkers(pHandle, imageinfo, sCrop, nWidth, nHeight);
	if ( error != OMX_ErrorNone ) {
		fprintf (stderr,"Error from SetMarkers()  function\n");
		goto EXIT;
	}
	
	/**
	* wait for initialization to complete (as indicated by the statechange
	* callback saying that component has been loaded (and is therefore
	* initialized.  Note that you should probably handle GUI events
	* in the WaitForState method.
	**/

	PRINT("Waiting for state Idle\n");
	error = WaitForState(pHandle, OMX_StateIdle);
	if ( error != OMX_ErrorNone ) {
	    PRINT("Error: JpegEncoder->WaitForState has timed out or failed %X\n",
	          error);
	    goto EXIT;
	}

	pQfactorType->nSize = sizeof(OMX_IMAGE_PARAM_QFACTORTYPE);
	pQfactorType->nQFactor = (OMX_U32) qualityfactor;
	pQfactorType->nVersion.s.nVersionMajor = 0x1;
	pQfactorType->nVersion.s.nVersionMinor = 0x0;
	pQfactorType->nVersion.s.nRevision = 0x0;
	pQfactorType->nVersion.s.nStep = 0x0;
	pQfactorType->nPortIndex = 0x0;

	error = OMX_GetExtensionIndex(pHandle, "OMX.TI.JPEG.encoder.Config.QFactor", (OMX_INDEXTYPE*)&nCustomIndex);
	if ( error != OMX_ErrorNone ) {
	    printf("%d::APP_Error at function call: %x\n", __LINE__, error);
	    goto EXIT;
	}
	error = OMX_SetConfig (pHandle, nCustomIndex, pQfactorType);
	if ( error != OMX_ErrorNone ) {
	    printf("%d::APP_Error at function call: %x\n", __LINE__, error);
	    goto EXIT;
	}


	/**
	 * Open the file of data to be rendered.  Since this is a just sample
	 * application, the data is "rendered" to a test mixer.  So, the test
	 * file better contain data that can be printed to the terminal w/o
	 * problems or you will not be a happy camper.
	**/
	PRINT("Opening output jpg file\n");
#ifdef UNDER_CE
	fOut = CreateFile(szOutFile, GENERIC_WRITE, 0,
	                       NULL,CREATE_ALWAYS, 0, NULL);
	if (INVALID_HANDLE_VALUE == fOut){
	    PRINT("Error: failed to open the file <%s> for writing\n",
	        szOutFile);
	    goto EXIT;
	}
#else   
	fOut = fopen(szOutFile, "w");
	if ( fOut == NULL ) {
	    PRINT("Error: failed to open the file <%s> for writing\n",
	          szOutFile);
	    goto EXIT;
	}
#endif  

	error = OMX_SendCommand(pHandle,OMX_CommandStateSet, OMX_StateExecuting, NULL);
	if ( error != OMX_ErrorNone ) {
	    fprintf (stderr,"Error from SendCommand-Executing State function\n");
	    goto EXIT;
	}
	pComponent = (OMX_COMPONENTTYPE *)pHandle;

	/**
	 * wait for startup to complete (as indicated by the statechange
	 * callback saying that component has been loaded (and is therefore
	 * initialized.  Note that you should probably handle GUI events
	 * in the WaitForState method.
	**/
	    
	PRINT("Waiting for OMX_StateExcecuting\n");
	error = WaitForState(pHandle, OMX_StateExecuting);
	if ( error != OMX_ErrorNone ) {
	    PRINT("Error:  JpegEncoder->WaitForState has timed out or failed %X\n", error);
	    goto EXIT;
	}

#if 0
	if (imageinfo->nDRI) {
	    error = OMX_GetExtensionIndex(pHandle, "OMX.TI.JPEG.encoder.Config.DRI", (OMX_INDEXTYPE*)&nCustomIndex);
	    if ( error != OMX_ErrorNone ) {
	        printf("%d::APP_Error at function call: %x\n", __LINE__, error);
	        goto EXIT;
	    }
	    error = OMX_SetConfig(pHandle, nCustomIndex, &(imageinfo->nDRI));
	    if ( error != OMX_ErrorNone ) {
	        printf("%d::APP_Error at function call: %x\n", __LINE__, error);
	        goto EXIT;
	    }
	}
#endif
    
	/** Handle the component's requests for data until we run out of data.  Do this
	 * in a way that will allow the UI to continue to run (if there is a UI, which
	 * this sample application does NOT have)
	**/

	done = 0;
	pComponent->GetState(pHandle, &state);
	PRINT("Error is %d , cur state is %d ",error, state); 


	for (nCounter =0; nCounter<1 /*NUM_OF_BUFFERSJPEG*/; nCounter++) {
		nRead = fill_data(pInBuff[nCounter], fIn,pInPortDef->nBufferSize);
		pComponent->FillThisBuffer(pHandle, pOutBuff[nCounter]);
		pComponent->EmptyThisBuffer(pHandle, pInBuff[nCounter]);
		framesent++;
		PRINT("Sent Frame # %d\n", framesent);                    
		if (pInBuff[nCounter]->nFlags == OMX_BUFFERFLAG_ENDOFFRAME)
		    break;
	}

	while ((error == OMX_ErrorNone) && 
	        (state != OMX_StateIdle)) {

	        if (bPreempted)
	        {
			PRINT("Preempted - Forced tp Idle State - Waiting for OMX_StateIdle\n");
			error = WaitForState(pHandle, OMX_StateIdle);
			if ( error != OMX_ErrorNone ) {
				PRINT("Error:  JpegEncoder->WaitForState has timed out or failed %X",  error);
				goto EXIT;
			}
			break;
	        }

	        fd_set rfds;
		sigemptyset(&set);
		sigaddset(&set,SIGALRM);

	        FD_ZERO(&rfds);
	        FD_SET(IpBuf_Pipe[0], &rfds);
	        FD_SET(OpBuf_Pipe[0], &rfds);
	        FD_SET(Event_Pipe[0], &rfds);
	        retval = pselect(fdmax+1, &rfds, NULL, NULL, NULL,&set);
	        if ( retval == -1 ) {
#ifndef UNDER_CE            
			perror("select()");
#endif          
			fprintf (stderr, " : Error \n");
			break;
	        }	
	        else if ( retval == 0 ) {
			PRINT("App Timeout !!!!!!!!!!!\n");
	        }
			
        	/**
		* If FD_ISSET for Event_Pipe, there is an event remaining on the pipe.  Read it
		* and get act accordingly to the event.
		**/
	        if ( FD_ISSET(Event_Pipe[0], &rfds)) {
				
	            JPEGE_EVENTPRIVATE EventPrivate;
	            read(Event_Pipe[0], &EventPrivate, sizeof(JPEGE_EVENTPRIVATE));
				
	            switch(EventPrivate.eEvent) {
	                case OMX_EventError:	
				if(bError) {
					error = WaitForState(pHandle, OMX_StateInvalid);
					if (error != OMX_ErrorNone) {
						printf("APP:: Error:  JpegEncoder->WaitForState has timed out or failed %X\n", error);
						goto EXIT;
					}
					printf("APP:: Component is in Invalid state now.\n");
					goto EXIT;	
				}
				break;

	                case OMX_EventBufferFlag:
		                printf("APP:: EOS flag received\n");
		                break;

	                default:
		                printf("APP:: Non-error event rised. Event -> 0x%x\n", EventPrivate.eEvent);
		                break;
	            }
	        }

		/**
		* If FD_ISSET for IpBuf_Pipe, then there is an empty buffer available on the pipe.  Read it 
		* from the pipe, then re-fill the buffer and send it back.
		**/
		if ( FD_ISSET(IpBuf_Pipe[0], &rfds) && !DEINIT_FLAG ) {
			
			/*read buffer */
			read(IpBuf_Pipe[0], &pBuffer, sizeof(pBuffer));

			/* re-fill this buffer with data from JPEG file */
			nRead = fill_data(pBuffer, fIn,pInPortDef->nBufferSize);

			/* call EmptyThisBuffer (send buffer back to component */
			OMX_EmptyThisBuffer(pHandle, pBuffer);

			/*increment count */
			framesent++;
			PRINT("Sent Frame # %d\n", framesent);            
	        }

		/**
		* If FD_ISSET for OpBuf_Pipe, then there is a filled buffer available on the pipe.  Read it
		* and get the buffer data out, write it to a file and then re-empty the buffer and send
		* the buffer back to the component.
		**/
	        if (FD_ISSET(OpBuf_Pipe[0], &rfds)) {

			/* read buffer */
			read(OpBuf_Pipe[0], &pBuf, sizeof(pBuf));

			/* write data to a file, buffer is assumed to be emptied after this*/
#ifdef UNDER_CE
			WriteFile(fOut, pBuf->pBuffer, pBuf->nFilledLen, &dwWritten, NULL);
#else
			printf("APP:: Write into file %lu bytes (%d)\n",pBuf->nFilledLen, nframerecieved);
			fwrite(pBuf->pBuffer, 1,  (int)pBuf->nFilledLen, fOut);
			fflush(fOut);
#endif          
			/*increment count and validate for limits; call FillThisBuffer */
			nframerecieved++;
			nRepeated++;
			PRINT("\n%d***************%d***************%d***************%d***************%d\n", nRepeated, nRepeated, nRepeated, nRepeated, nRepeated);
			if (nRepeated >= maxRepeat) {                    
				DEINIT_FLAG = 1;
			}
			else {
				PRINT("Sending another output buffer\n");
				pComponent->FillThisBuffer(pHandle,pBuf);
			}
	        }

	        if (DEINIT_FLAG == 1) {
                    done = 1;
                    pHandle = (OMX_HANDLETYPE *) pComponent;
                    error = OMX_SendCommand(pHandle,OMX_CommandStateSet, OMX_StateIdle, NULL);
                    if ( error != OMX_ErrorNone ) {
                        fprintf (stderr,"APP:: Error from SendCommand-Idle(Stop) State function\n");
                        goto EXIT;
                    }

                    PRINT("Waiting for OMX_StateIdle\n");
                    error = WaitForState(pHandle, OMX_StateIdle);
                    if ( error != OMX_ErrorNone ) {
                        PRINT("Error:  JpegEncoder->WaitForState has timed out %X", error);
                        goto EXIT;
                    }

                    error = OMX_SendCommand(pHandle, OMX_CommandPortDisable, 0, NULL);
                    if ( error != OMX_ErrorNone ) {
                        PRINT("Error from SendCommand-PortDisable function\n");
                        goto EXIT;
                    }
	        }
        
	        if (done == 1) {
                    error = pComponent->GetState(pHandle, &state);
                    if ( error != OMX_ErrorNone ){
                        fprintf(stderr, "Warning:  JpegEncoder->JPEGENC_GetState has returned status %X\n", error);
                        goto EXIT;
                    }
                    PRINT("After GetState() in while loop.\n");
	        }
	}

	


#ifdef STRESS
      }
#endif

	printf("\nTest Completed Successfully! Deinitializing ... \n\n");

EXIT:

	printf("\nClosing application...\n");

	/**
	*  Freeing memory
	**/
	
/*close handles */
#ifdef UNDER_CE
	CloseHandle(fOut);
	CloseHandle(fIn);
#else
	fclose(fOut);
	rewind (fIn);
	fclose(fIn);
#endif  

	PRINT("Freeing memory from test app\n");
	FREE(pPortParamType);
	FREE(pQfactorType);
	FREE(pInPortDef);
	FREE(pOutPortDef);
	FREE(imageinfo);
	FREE(pQuantizationTable);
	FREE(pHuffmanTable);
    
	if( error != OMX_ErrorNone){
		if (buffertype == 1) {
			for (nCounter = 0; nCounter < NUM_OF_BUFFERSJPEG; nCounter++) {
				pOutBuffer[nCounter]-=128;
				pInBuffer[nCounter]-=128;
				FREE(pOutBuffer[nCounter]);
				FREE(pInBuffer[nCounter]);
			}
		}
		for (nCounter = 0; nCounter < NUM_OF_BUFFERSJPEG; nCounter++) {
		    error = OMX_FreeBuffer(pHandle, nIndex1, pInBuff[nCounter]);
		    if ( (error != OMX_ErrorNone) ) {
		        printf ("Error in OMX_FreeBuffer: %d\n", __LINE__);
		    }
		    error = OMX_FreeBuffer(pHandle, nIndex2, pOutBuff[nCounter]);
		    if ( (error != OMX_ErrorNone) ) {
		        printf ("Error in OMX_FreeBuffer: %d\n", __LINE__);
		    }
		}
	}

	error = TIOMX_FreeHandle(pHandle);
	if ( (error != OMX_ErrorNone) ) {
	    printf ("Error in Free Handle function\n");
	}

#ifdef DSP_MMU_FAULT_HANDLING

        if(bError) {
            LoadBaseImage();
        }

#endif
    
	error = TIOMX_Deinit();
	if ( error != OMX_ErrorNone ) {
	    printf("Error returned by OMX_DeInit()\n");
	}

#ifdef STRESS

} /* end of multiload loop */

#endif

#ifndef UNDER_CE
//muntrace();
#endif
    PRINT ("Free Handle returned Successfully = %x\n",error);
    return error;
}

#ifdef DSP_MMU_FAULT_HANDLING

int LoadBaseImage() {
    unsigned int uProcId = 0;	/* default proc ID is 0. */
    unsigned int index = 0;
    
    struct DSP_PROCESSORINFO dspInfo;
    DSP_HPROCESSOR hProc;
    DSP_STATUS status = DSP_SOK;
    unsigned int numProcs;
    char* argv[2];
   
    argv[0] = "/lib/dsp/baseimage.dof";
    
    status = (DBAPI)DspManager_Open(0, NULL);
    if (DSP_FAILED(status)) {
        printf("DSPManager_Open failed \n");
        return -1;
    } 
    while (DSP_SUCCEEDED(DSPManager_EnumProcessorInfo(index,&dspInfo,
        (unsigned int)sizeof(struct DSP_PROCESSORINFO),&numProcs))) {
        if ((dspInfo.uProcessorType == DSPTYPE_55) || 
            (dspInfo.uProcessorType == DSPTYPE_64)) {
            uProcId = index;
            status = DSP_SOK;
            break;
        }
        index++;
    }
    status = DSPProcessor_Attach(uProcId, NULL, &hProc);
    if (DSP_SUCCEEDED(status)) {
        status = DSPProcessor_Stop(hProc);
        if (DSP_SUCCEEDED(status)) {
            status = DSPProcessor_Load(hProc,1,(const char **)argv,NULL);
            if (DSP_SUCCEEDED(status)) {
                status = DSPProcessor_Start(hProc);
                if (DSP_SUCCEEDED(status)) {
                } 
                else {
                }
            } 
			else {
            }
            DSPProcessor_Detach(hProc);
        }
        else {
        }
    }
    else {
    }
    fprintf(stderr,"Baseimage Loaded\n");

    return 0;		
}
#endif
