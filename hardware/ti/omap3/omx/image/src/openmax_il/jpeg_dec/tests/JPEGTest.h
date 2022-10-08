
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
/**
 * Global data used communicate state changes back to the applicaiton.  This probably 
 * should really be via a pipe or some other protected mechanism for better
 * reliability, but this is sufficient for a demo.
**/
/*Not used anymore in 2430*/
/*static volatile OMX_State gComponentState = OMX_STATE_INVALID;*/

#ifndef OMX_TESTDEC_H
#define OMX_TESTDEC_H


#ifndef UNDER_CE
#include <unistd.h>     
#include <signal.h>
#endif

#include <OMX_Core.h>   
#include <OMX_Types.h>	
#include <OMX_Image.h>
	
/* this implements the function for initializing the debug handle */

#define NUM_OF_BUFFERS     4


#define M_SOF0  0xC0            /* nStart Of Frame N*/
#define M_SOF1  0xC1            /* N indicates which compression process*/
#define M_SOF2  0xC2            /* Only SOF0-SOF2 are now in common use*/
#define M_SOF3  0xC3
#define M_SOF5  0xC5            /* NB: codes C4 and CC are NOT SOF markers*/
#define M_SOF6  0xC6
#define M_SOF7  0xC7
#define M_SOF9  0xC9
#define M_SOF10 0xCA
#define M_SOF11 0xCB
#define M_SOF13 0xCD
#define M_SOF14 0xCE
#define M_SOF15 0xCF
#define M_SOI   0xD8            /* nStart Of Image (beginning of datastream)*/
#define M_EOI   0xD9            /* End Of Image (end of datastream)*/
#define M_SOS   0xDA            /* nStart Of Scan (begins compressed data)*/
#define M_JFIF  0xE0            /* Jfif marker*/
#define M_EXIF  0xE1            /* Exif marker*/
#define M_COM   0xFE            /* COMment */
#define M_DQT   0xDB
#define M_DHT   0xC4
#define M_DRI   0xDD

#define DSP_MMU_FAULT_HANDLING

typedef struct IMAGE_INFO {
    int nWidth;
    int nHeight ;
	int format;
    int nProgressive;
    
} IMAGE_INFO;

#ifdef UNDER_CE
OMX_STRING StrJpegDecoder= "OMX.TI.IMAGE.JPEG.DEC";
#else
OMX_STRING StrJpegDecoder= "OMX.TI.JPEG.decoder";
#endif	


typedef struct JPEGD_EVENTPRIVATE {
	OMX_EVENTTYPE eEvent;
	OMX_PTR pAppData;
	OMX_PTR pEventInfo;
	OMX_U32 nData1;
	OMX_U32 nData2;
}JPEGD_EVENTPRIVATE;

typedef struct OMX_CUSTOM_IMAGE_DECODE_SECTION
{
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nMCURow;
    OMX_U32 nAU;
    OMX_BOOL bSectionsInput;
    OMX_BOOL bSectionsOutput;
}OMX_CUSTOM_IMAGE_DECODE_SECTION;


typedef struct OMX_CUSTOM_IMAGE_DECODE_SUBREGION
{
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nXOrg;         /*Sectional decoding: X origin*/
    OMX_U32 nYOrg;         /*Sectional decoding: Y origin*/
    OMX_U32 nXLength;      /*Sectional decoding: X lenght*/
    OMX_U32 nYLength;      /*Sectional decoding: Y lenght*/
}OMX_CUSTOM_IMAGE_DECODE_SUBREGION;

typedef struct OMX_CUSTOM_RESOLUTION 
{
	OMX_U32 nWidth;
	OMX_U32 nHeight;
} OMX_CUSTOM_RESOLUTION;

#endif /*OMX_TESTDEC_H*/
