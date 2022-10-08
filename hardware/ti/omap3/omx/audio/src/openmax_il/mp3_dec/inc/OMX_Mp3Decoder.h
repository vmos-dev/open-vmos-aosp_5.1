
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
* ============================================================================
* */
/**
* @file OMX_MP3Decoder.h
*
* This is an header file for an audio MP3 decoder that is fully
* compliant with the OMX Audio specification.
* This the file that the application that uses OMX would include
* in its code.
*
* @path  $(CSLPATH)\OMAPSW_MPU\linux\audio\src\openmax_il\mp3_dec\inc\
*
* @rev 1.0
*/
/* --------------------------------------------------------------------------- * */

#ifndef OMX_MP3DECODER_H
#define OMX_MP3DECODER_H


#ifdef UNDER_CE
#include <windows.h>
#include <oaf_osal.h>
#include <omx_core.h>
#include <stdlib.h>
#else
#include <pthread.h>
#endif

#include <OMX_Component.h>
#include "LCML_DspCodec.h"

#ifndef UNDER_CE
#define MP3D_RM_MANAGER /* Enable to use Resource Manager functionality */
#else
#undef MP3D_RM_MANAGER /* Enable to use Resource Manager functionality */
#endif

/* #define SWAT_ANALYSIS */   /* Enable to use SWAT functionality */
/*#define DSP_RENDERING_ON*/ /* Enable to use DASF functionality */


/* #define MP3DEC_DEBUG */     /* See all debug statement of the component */
/*  #define MP3DEC_MEMDETAILS */  /* See memory details of the component */
/* #define MP3DEC_BUFDETAILS */  /* See buffers details of the component */
/* #define MP3DEC_STATEDETAILS */ /* See all state transitions of the component
*/
/* #define MP3DEC_SWATDETAILS  */ /* See SWAT debug statement of the component */


#define MP3_APP_ID  100 /* Defines MP3 Dec App ID, App must use this value */
#define MP3D_MAX_NUM_OF_BUFS 10 /* Max number of buffers used */
#define MP3D_NUM_INPUT_BUFFERS 2  /* Default number of input buffers */
#define MP3D_NUM_OUTPUT_BUFFERS 4 /* Default number of output buffers */


//#define MP3D_INPUT_BUFFER_SIZE  100 /* Default size of input buffer */
//#define MP3D_OUTPUT_BUFFER_SIZE 4608 /* Default size of output buffer */

#define MP3D_MONO_STREAM  1 /* Mono stream index */
#define MP3D_STEREO_INTERLEAVED_STREAM  2 /* Stereo Interleaved stream index */
#define MP3D_STEREO_NONINTERLEAVED_STREAM  3 /* Stereo Non-Interleaved stream index */

#define MP3D_STEREO_STREAM  2

#define NUM_OF_PORTS 2 /* Number of ports of component */

/*#define MP3DEC_MEMDEBUG*/
#undef MP3DEC_MEMDEBUG


typedef enum MP3D_COMP_PORT_TYPE {
    MP3D_INPUT_PORT = 0,
    MP3D_OUTPUT_PORT
}MP3D_COMP_PORT_TYPE;


typedef enum OMX_INDEXAUDIOTYPE {
	MP3D_OMX_IndexCustomMode16_24bit = 0xFF000001,
	MP3D_OMX_IndexCustomModeDasfConfig,
    OMX_IndexCustomMp3DecHeaderInfoConfig,
	OMX_IndexCustomMp3DecStreamInfoConfig,
    OMX_IndexCustomMp3DecDataPath,
    OMX_IndexCustomDebug
}OMX_INDEXAUDIOTYPE;

#endif /* OMX_MP3DECODER_H */
