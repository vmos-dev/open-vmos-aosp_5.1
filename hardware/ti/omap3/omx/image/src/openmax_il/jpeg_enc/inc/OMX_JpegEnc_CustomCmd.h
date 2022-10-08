
/*
 * Copyright (C) Texas Instruments - http://www.ti.com/
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
/* =============================================================================
*             Texas Instruments OMAP(TM) Platform Software
*  (c) Copyright Texas Instruments, Incorporated.  All Rights Reserved.
*
*  Use of this software is controlled by the terms and conditions found
*  in the license agreement under which this software has been supplied.
* =========================================================================== */
/**
* @file OMX_JpegEnc_CustomCmd.h
*
* This is an header file for an Jpeg encoder that is fully compliant with the OMX specification.
* This is the file that the application that uses OMX would include in its code.
*
* @path $(CSLPATH)\
* 
* @rev 0.1
*/
/* -------------------------------------------------------------------------- */

#ifndef OMX_JPEGENC_CUSTOMCMD_H
#define OMX_JPEGENC_CUSTOMCMD_H


/*------- Program Header Files ----------------------------------------*/
#include <OMX_Component.h>

/*------- Structures ----------------------------------------*/


typedef struct JPEGENC_CUSTOM_HUFFMAN_TABLE {

      OMX_U32 lum_dc_vlc[12];
      OMX_U32 lum_ac_vlc[16][16];
      OMX_U32 chm_dc_vlc[12];
      OMX_U32 chm_ac_vlc[16][16];
      OMX_U8  lum_dc_codelens[16];
      OMX_U16 lum_dc_ncodes;
      OMX_U8  lum_dc_symbols[12];
      OMX_U16 lum_dc_nsymbols;
      OMX_U8  lum_ac_codelens[16];
      OMX_U16 lum_ac_ncodes;
      OMX_U8  lum_ac_symbols[162];
      OMX_U16 lum_ac_nsymbols;
      OMX_U8  chm_dc_codelens[16];
      OMX_U16 chm_dc_ncodes;
      OMX_U8  chm_dc_symbols[12];
      OMX_U16 chm_dc_nsymbols;
      OMX_U8  chm_ac_codelens[16];
      OMX_U16 chm_ac_ncodes;
      OMX_U8  chm_ac_symbols[162];
      OMX_U16 chm_ac_nsymbols;
}JPEGENC_CUSTOM_HUFFMAN_TABLE;

typedef struct JPEGENC_CUSTOM_HUFFMANTTABLETYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    JPEGENC_CUSTOM_HUFFMAN_TABLE sHuffmanTable;
}JPEGENC_CUSTOM_HUFFMANTTABLETYPE;

typedef struct JPEG_APPTHUMB_MARKER { 
	OMX_BOOL bMarkerEnabled;		/* Boolean flag to enable/disable this marker on its whole */
	OMX_U8 *pMarkerBuffer;			/* This pointer must point to the marker buffer allocated by application */
	OMX_U32 nMarkerSize;			/* This variable holds the size of the marker buffer */
	OMX_U32 nThumbnailWidth;          /* This variable holds the thumbnail's width value (0 = No thumbnail) */
	OMX_U32 nThumbnailHeight;	        /* This variable holds the thumbnail's height value (0 = No thumbnail) */
} JPEG_APPTHUMB_MARKER;

typedef struct JPEG_APP13_MARKER { 
	OMX_BOOL bMarkerEnabled;		/* Boolean flag to enable/disable this marker on its whole */
	OMX_U8 *pMarkerBuffer;			/* This pointer must point to the marker buffer allocated by application */
	OMX_U32 nMarkerSize;			/* This variable holds the size of the marker buffer */
} JPEG_APP13_MARKER;

#endif /* OMX_JPEGENC_CUSTOMCMD_H */

