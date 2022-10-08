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
 */


/*
******************************************************************************
 Profile calculation masks
******************************************************************************/
#define iWMVA_MASK                              (0x08)
#define iWMV9_MASK                              (0x80)

/* system environment dependent switches */
//! Pack decoded bitplane bits into bytes (instead of 1-bit per byte)
//#define BITPLANE_PACKED_BYTES

//! Generate bitplane test vectors
#define BITPLANE_TEST_VECTORS   (0)

//! Measure bitplane decode time
#define BITPLANE_DECODE_TIME    (0)

//! Measure time spent parsing the picture header
#define PARSE_HEADER_TIME               (0)

//! Use VC1 reference decoder implementation for bitplane decoding
#define REFDEC_BITPLANE_DECODER         (0)

//! Interleave individual bitplanes into packed format
#define VC1_INTERLEAVED_BITPLANE        (1)

//! Use MSVDX hardware for bitplane decoding
#define VC1_BITPLANE_HARDWARE           (0)

/*****************************************************************************/
#if VC1_BITPLANE_HARDWARE
//! VC1_INTERLEAVED_BITPLANE must be set to 0 when using MSVDX hardware for bitplane decoding
#if VC1_INTERLEAVED_BITPLANE
#error VC1_INTERLEAVED_BITPLANE must not be defined together with VC1_BITPLANE_HARDWARE
#endif
#endif
/*****************************************************************************/

/*
Possible combinations for bitplane decoding operation:

+ To use the hardware bitplane decoder, define VC1_BITPLANE_HARDWARE in the
preprocessor definitions of um_drivers, set REFDEC_BITPLANE_DECODER to 1 and
VC1_INTERLEAVED_BITPLANE to 0.
+ To use the software bitplane decoder, don't define VC1_BITPLANE_HARDWARE.
There are two implementations of the decoder:
        - To use the VC1 reference decoder implementation, set
        REFDEC_BITPLANE_DECODER to 1.
        - Otherwise, set REFDEC_BITPLANE_DECODER to 0.
+ When using the software bitplane decoder, the data can be sent to
the hardware in two formats.
        - Set VC1_INTERLEAVED_BITPLANE to 1, if using the three-plane
        interleaved format.
        - Otherwise, set VC1_INTERLEAVED_BITPLANE to 0.
*/

/*!
******************************************************************************
 This enumeration defines PTYPE [All]
******************************************************************************/
typedef enum {
    WMF_PTYPE_I       = 0,      //!< I Picture
    WMF_PTYPE_P       = 1,      //!< P Picture
    WMF_PTYPE_B       = 2,      //!< B Picture
    WMF_PTYPE_BI      = 3,      //!< BI Picture
    WMF_PTYPE_SKIPPED = 4       //!< Skipped Picture

} WMF_ePTYPE;

/*!
******************************************************************************
 This enumeration defines the stream profile
******************************************************************************/
typedef enum {
    WMF_PROFILE_SIMPLE          = 0,    //!< Simple profile
    WMF_PROFILE_MAIN            = 1,    //!< Main profile
    WMF_PROFILE_ADVANCED        = 2,    //!< Advanced profile (VC1 Only)
    WMF_PROFILE_UNDEFINED   = 3,    //!< Undefined profile

} WMF_eProfile;

/*!
******************************************************************************
 This enumeration defines MVMODE [All] Tables 46-50 MVMODE and MVMODE2
******************************************************************************/
typedef enum {
    WMF_MVMODE_1MV                      = 0,    //!<    1 MV
    WMF_MVMODE_1MV_HALF_PEL_BILINEAR    = 1,    //!<    1 MV Half-pel bilinear
    WMF_MVMODE_1MV_HALF_PEL             = 2,    //!<    1 MV Half-pel
    WMF_MVMODE_MIXED_MV                 = 3,    //!<    Mixed MV
    WMF_MVMODE_INTENSITY_COMPENSATION   = 4,    //!<    Intensity Compensation
    WMF_MVMODE_QUARTER_PEL_BICUBIC      = 5,    //!<    Quarter pel bicubic

} WMF_eMVMODE;

/*!
******************************************************************************
 This enumeration defines FCM [Advanced Profile Only] Table 41 gFCM_VlcTable
******************************************************************************/
typedef enum {
    VC1_FCM_P       = 0,    //!< 0      Progressive
    VC1_FCM_FRMI    = 2,        //!< 10 Frame-Interlace
    VC1_FCM_FLDI    = 3,        //!< 11 Field-Interlace

} VC1_eFCM;

/*!
******************************************************************************
 This enumeration defines the BDU Start Code Suffixes \n
    0x00 - 0x09         SMPTE Reserved \n
    0x20 - 0x7F         SMPTE Reserved \n
    0x80  -0xFF         Forbidden
******************************************************************************/
typedef enum {
    VC1_SCS_ENDOFSEQU                           = 0x0A,         //!< End of sequence
    VC1_SCS_SLICE                                       = 0x0B,         //!< Slice
    VC1_SCS_FIELD                                       = 0x0C,         //!< Field
    VC1_SCS_PIC_LAYER                           = 0x0D,         //!< Frame
    VC1_SCS_ENTRYPNT_LAYER                      = 0x0E,         //!< Entry-point Header
    VC1_SCS_SEQ_LAYER                           = 0x0F,         //!< Sequence Header
    VC1_SCS_SLICELVL_USERDATA           = 0x1B,         //!< Slice Level User Data
    VC1_SCS_FIELDLVL_USERDATA           = 0x1C,         //!< Field Level User Data
    VC1_SCS_PICLVL_USERDATA                     = 0x1D,         //!< Frame Level User Data
    VC1_SCS_ENTRYPNTLVL_USERDATA        = 0x1E,         //!< Entry-point Level User Data
    VC1_SCS_SEQLVL_USERDATA                     = 0x1F,         //!< Sequence Level User Data

} VC1_eSCS;

/*! Test if picture type is a reference (I or P) */
#define PIC_TYPE_IS_REF(Type)   ((Type) == WMF_PTYPE_I || (Type) == WMF_PTYPE_P)

/*! Test if picture type is intra (I or BI) */
#define PIC_TYPE_IS_INTRA(Type) ((Type) == WMF_PTYPE_I || (Type) == WMF_PTYPE_BI)

/*! Test if picture type is inter (P or B) */
#define PIC_TYPE_IS_INTER(Type) ((Type) == WMF_PTYPE_P || (Type) == WMF_PTYPE_B)

//! Maximum number of VLC tables for MB/block layer decode
#define MAX_VLC_TABLES  (12)

#define COMPUTE_PULLBACK(s)     (VC1_MB_SIZE*((s+15)/VC1_MB_SIZE)*4 - 4)      /* 8.4.5.8 */

//! VC1 MB Parameter Stride
#define VC1_MB_PARAM_STRIDE     (128)

#define VC1_MAX_NUM_BITPLANES   (3)

#define CABAC_RAM_WIDTH_IN_BITS (16)

/*
******************************************************************************
 Frame Dimension Parameters
******************************************************************************/

//! Number of pixels in each MB dimension
#define VC1_MB_SIZE (16)

//! Maximum resolution in frame width (X)
#define VC1_MAX_X   (4096)      // 1920

//! Maximum resolution in frame height (Y)
#define VC1_MAX_Y   (2048)              // 1080

//! Maximum resolution of frame
#define VC1_MAX_RES                     (VC1_MAX_X*VC1_MAX_Y)

//! Maximum number of MBs in frame width (X)
#define VC1_MAX_NO_MBS_X        ((VC1_MAX_X + VC1_MB_SIZE - 1)/VC1_MB_SIZE)

//! Maximum number of MBs in frame height (Y)
#define VC1_MAX_NO_MBS_Y        ((VC1_MAX_Y + VC1_MB_SIZE - 1)/VC1_MB_SIZE)

//! Maximum number of MBs in frame
#define VC1_MAX_NO_MBS          ((VC1_MAX_RES + (VC1_MB_SIZE*VC1_MB_SIZE) - 1) /(VC1_MB_SIZE*VC1_MB_SIZE))

//! Maximum number of bytes in bitplane
#define BITPLANE_BYTES          VC1_MAX_NO_MBS

//! Maximum number of pan/scan windows per frame (VC1 Specification: 7.1.1.20)
#define VC1_MAX_PANSCAN_WINDOWS (4)
