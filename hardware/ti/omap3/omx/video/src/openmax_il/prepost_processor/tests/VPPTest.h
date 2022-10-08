
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
/* ==============================================================================
*             Texas Instruments OMAP (TM) Platform Software
*  (c) Copyright Texas Instruments, Incorporated.  All Rights Reserved.
*
*  Use of this software is controlled by the terms and conditions found
*  in the license agreement under which this software has been supplied.
* ============================================================================ */

/**
* @file OMX_PostProcTest.h
*
* Header file for PostProcessor OMX Component Test application.
*
* @path  $(CSLPATH)\post_proc\tests\OMX_PostProcTest.h
*
* @rev  0.1
*/

/* =================================================================================  
*! Revision History 
*! ===================================
*! 
*!
*! 02-Dec-2005 mf: Revisions appear in reverse chronological order; 
*! that is, newest first.  The date format is dd-Mon-yyyy.  
* ================================================================================= */

#ifndef VPP_TEST_H
#define VPP_TEST_H

/****************************************************************
*  INCLUDE FILES
****************************************************************/
#include <OMX_Core.h>       
#include <OMX_Video.h>	
#include <OMX_Types.h>	

#define MAX_FRAME_WIDTH      640 
#define MAX_FRAME_HEIGHT     480
#define MAX_422_YUV_BUF_SIZE (MAX_FRAME_WIDTH * MAX_FRAME_HEIGHT * 2)
#define MAX_420_YUV_BUF_SIZE (MAX_FRAME_WIDTH * MAX_FRAME_HEIGHT * 1.5)
#define MAX_RGB_565_BUF_SIZE (MAX_FRAME_WIDTH * MAX_FRAME_HEIGHT * 2)

#define QQCIF_FRAME_WIDTH       88 
#define QQCIF_FRAME_HEIGHT      72
#define QQCIF_422_YUV_BUF_SIZE  (QQCIF_FRAME_WIDTH * QQCIF_FRAME_HEIGHT * 2) 
#define QQCIF_420_YUV_BUF_SIZE  (QQCIF_FRAME_WIDTH * QQCIF_FRAME_HEIGHT * 1.5) 
#define QQCIF_RGB_565_BUF_SIZE  (QQCIF_FRAME_WIDTH * QQCIF_FRAME_HEIGHT * 2) 

#define QCIF_FRAME_WIDTH       176 
#define QCIF_FRAME_HEIGHT      144
#define QCIF_422_YUV_BUF_SIZE  (QCIF_FRAME_WIDTH * QCIF_FRAME_HEIGHT * 2) 
#define QCIF_420_YUV_BUF_SIZE  (QCIF_FRAME_WIDTH * QCIF_FRAME_HEIGHT * 1.5)
#define QCIF_RGB_565_BUF_SIZE  (QCIF_FRAME_WIDTH * QCIF_FRAME_HEIGHT * 2)  

#define QQVGA_FRAME_WIDTH       160
#define QQVGA_FRAME_HEIGHT      120
#define QQVGA_422_YUV_BUF_SIZE  (QQVGA_FRAME_WIDTH * QQVGA_FRAME_HEIGHT * 2)
#define QQVGA_420_YUV_BUF_SIZE  (QQVGA_FRAME_WIDTH * QQVGA_FRAME_HEIGHT * 1.5)
#define QQVGA_RGB_565_BUF_SIZE  (QQVGA_FRAME_WIDTH * QQVGA_FRAME_HEIGHT * 2)

#define QVGA_FRAME_WIDTH       320 
#define QVGA_FRAME_HEIGHT      240
#define QVGA_422_YUV_BUF_SIZE  (QVGA_FRAME_WIDTH * QVGA_FRAME_HEIGHT * 2) 
#define QVGA_420_YUV_BUF_SIZE  (QVGA_FRAME_WIDTH * QVGA_FRAME_HEIGHT * 1.5) 
#define QVGA_RGB_565_BUF_SIZE  (QVGA_FRAME_WIDTH * QVGA_FRAME_HEIGHT * 2) 

#define VGA_FRAME_WIDTH       640 
#define VGA_FRAME_HEIGHT      480
#define VGA_422_YUV_BUF_SIZE  (VGA_FRAME_WIDTH * VGA_FRAME_HEIGHT * 2) 
#define VGA_420_YUV_BUF_SIZE  (VGA_FRAME_WIDTH * VGA_FRAME_HEIGHT * 1.5)
#define VGA_RGB_565_BUF_SIZE  (VGA_FRAME_WIDTH * VGA_FRAME_HEIGHT * 2)  

#define CIF_FRAME_WIDTH       352 
#define CIF_FRAME_HEIGHT      288
#define CIF_422_YUV_BUF_SIZE  (CIF_FRAME_WIDTH * CIF_FRAME_HEIGHT * 2) 
#define CIF_420_YUV_BUF_SIZE  (CIF_FRAME_WIDTH * CIF_FRAME_HEIGHT * 1.5) 
#define CIF_RGB_565_BUF_SIZE  (CIF_FRAME_WIDTH * CIF_FRAME_HEIGHT * 2) 

#define MAX_POSTPROC_BUFFERS 4
#define MAX_MPEG4D_OUT_BUFFERS 1
#define MAX_VPP_BUFFERS 4

#define DSP_MMU_FAULT_HANDLING

#define OMX_CONF_INIT_STRUCT(_s_, _name_)  \
{ \
    memset((_s_), 0x0, sizeof(_name_));  \
    (_s_)->nSize = sizeof(_name_);    \
    (_s_)->nVersion.s.nVersionMajor = 1;  \
    (_s_)->nVersion.s.nVersionMinor = 0;  \
    (_s_)->nVersion.s.nRevision = 0;    \
    (_s_)->nVersion.s.nStep = 0; \
}

#if 1
    #define PRINTF(str,args...) printf("\n"str"\n",##args)
#else
    #define PRINTF(str,args...)
#endif

#define EXIT_IF_ERROR(_eError,_string)		\
{						\
    if(_eError != OMX_ErrorNone) {		\
	 printf(_string); \
        goto EXIT;		\
    	}                          \
}

#define CHK_COND(_predicate,_eError,_eCode,_string) \
{ \
    if(_predicate) { \
		_eError = _eCode; \
		printf(_string); \
		goto EXIT; \
    	} \
}

#define MALLOC(_ptr,_castType,_type,_eError) \
{                       \
  _ptr = (_castType *)malloc(sizeof(_type)); \
    if(!_ptr) { \
        _eError = OMX_ErrorInsufficientResources; \
	goto EXIT; \
    } \
}

#define MEMSET(_ps,_c,_n,_eError) \
{                                   \
  if(memset(_ps,_c,_n) != _ps) {           \
  	_eError = OMX_ErrorInsufficientResources; \
  	goto EXIT; \
  }    \
}

typedef struct _COMPONENT_PORTINDEX_DEF {
	int input_port;
	int overlay_port;
	int output_port;
	int yuvoutput_port;
	int rgboutput_port;
} COMPONENT_PORTINDEX_DEF;

#endif /* OMX_POSTPROC_TEST_H */
