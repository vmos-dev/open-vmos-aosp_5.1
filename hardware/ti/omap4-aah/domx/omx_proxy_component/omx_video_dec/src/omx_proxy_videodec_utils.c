/*
 * Copyright (c) 2010, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*==============================================================
 *! Revision History
 *! ============================
 *! 21-Oct-2011 Rajesh vandanapu sarthav@ti.com: Initial Version
 *================================================================*/

/******************************************************************
 *   INCLUDE FILES
 ******************************************************************/
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "omx_proxy_common.h"
#include <timm_osal_interfaces.h>
#include "OMX_TI_IVCommon.h"
#include "OMX_TI_Video.h"
#include "OMX_TI_Index.h"

#ifdef ENABLE_RAW_BUFFERS_DUMP_UTILITY
#define LOG_TAG "OMXPROXYVIDEODEC"
#include <fcntl.h>
#include <cutils/properties.h>
#include <utils/Log.h>
#include <stdlib.h>
#include <errno.h>
#endif

#define COMPONENT_NAME "OMX.TI.DUCATI1.VIDEO.DECODER"
/* needs to be specific for every configuration wrapper */

/* DEFINITIONS for parsing the config information & sequence header for WMV*/
#define VIDDEC_GetUnalignedDword( pb, dw ) \
    (dw) = ((OMX_U32) *(pb + 3) << 24) + \
        ((OMX_U32) *(pb + 2) << 16) + \
        ((OMX_U16) *(pb + 1) << 8) + *pb;

#define VIDDEC_GetUnalignedDwordEx( pb, dw )   VIDDEC_GetUnalignedDword( pb, dw ); (pb) += sizeof(OMX_U32);
#define VIDDEC_LoadDWORD( dw, p )  VIDDEC_GetUnalignedDwordEx( p, dw )
#define VIDDEC_MAKEFOURCC(ch0, ch1, ch2, ch3) \
    ((OMX_U32)(OMX_U8)(ch0) | ((OMX_U32)(OMX_U8)(ch1) << 8) |   \
    ((OMX_U32)(OMX_U8)(ch2) << 16) | ((OMX_U32)(OMX_U8)(ch3) << 24 ))

#define VIDDEC_FOURCC(ch0, ch1, ch2, ch3)  VIDDEC_MAKEFOURCC(ch0, ch1, ch2, ch3)

#define FOURCC_WMV3     VIDDEC_FOURCC('W','M','V','3')
#define FOURCC_WMV2     VIDDEC_FOURCC('W','M','V','2')
#define FOURCC_WMV1     VIDDEC_FOURCC('W','M','V','1')
#define FOURCC_WVC1     VIDDEC_FOURCC('W','V','C','1')

#define CSD_POSITION    51 /*Codec Specific Data position on the "stream propierties object"(ASF spec)*/

typedef struct VIDDEC_WMV_RCV_struct {
    OMX_U32 nNumFrames : 24;
    OMX_U32 nFrameType : 8;
    OMX_U32 nID : 32;
    OMX_U32 nStructData : 32;   //STRUCT_C
    OMX_U32 nVertSize;          //STRUCT_A-1
    OMX_U32 nHorizSize;         //STRUCT_A-2
    OMX_U32 nID2 : 32;
    OMX_U32 nSequenceHdr : 32;   //STRUCT_B
} VIDDEC_WMV_RCV_struct;

typedef struct VIDDEC_WMV_VC1_struct {
    OMX_U32 nNumFrames  : 24;
    OMX_U32 nFrameType  : 8;
    OMX_U32 nID         : 32;
    OMX_U32 nStructData : 32;   //STRUCT_C
    OMX_U32 nVertSize;          //STRUCT_A-1
    OMX_U32 nHorizSize;         //STRUCT_A-2
    OMX_U32 nID2        : 32;
    OMX_U32 nSequenceHdr : 32;   //STRUCT_B
} VIDDEC_WMV_VC1_struct;


OMX_ERRORTYPE PrearrageEmptyThisBuffer(OMX_HANDLETYPE hComponent,
    OMX_BUFFERHEADERTYPE * pBufferHdr)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    PROXY_COMPONENT_PRIVATE *pCompPrv = NULL;
    OMX_COMPONENTTYPE *hComp = (OMX_COMPONENTTYPE *) hComponent;
    OMX_U8* pBuffer = NULL;
    OMX_U8* pData = NULL;
    OMX_U32 nValue = 0;
    OMX_U32 nWidth = 0;
    OMX_U32 nHeight = 0;
    OMX_U32 nActualCompression = 0;
    OMX_U8* pCSD = NULL;
    OMX_U32 nSize_CSD = 0;
    DOMX_ENTER("");

    PROXY_assert(pBufferHdr != NULL, OMX_ErrorBadParameter, NULL);

    if (pBufferHdr->nFlags & OMX_BUFFERFLAG_CODECCONFIG){
        PROXY_assert(hComp->pComponentPrivate != NULL, OMX_ErrorBadParameter, NULL);

        pCompPrv = (PROXY_COMPONENT_PRIVATE *) hComp->pComponentPrivate;
        /* Get component role */
        OMX_PARAM_COMPONENTROLETYPE compRole;
        compRole.nSize = sizeof(OMX_PARAM_COMPONENTROLETYPE);
        compRole.nVersion.s.nVersionMajor = 1;
        compRole.nVersion.s.nVersionMinor = 1; //Ducati OMX version
        compRole.nVersion.s.nRevision = 0;
        compRole.nVersion.s.nStep = 0;

        eError = PROXY_GetParameter(hComp, OMX_IndexParamStandardComponentRole, &compRole);
        if(eError != OMX_ErrorNone){
            DOMX_ERROR("Error getting OMX_IndexParamStandardComponentRole");
        }

        if(!strcmp((char *)(compRole.cRole), "video_decoder.wmv")){
            pBuffer = pBufferHdr->pBuffer;

            VIDDEC_WMV_RCV_struct sStructRCV;

            DOMX_DEBUG("nFlags: %x", pBufferHdr->nFlags);

            pData = pBufferHdr->pBuffer + 15; /*Position to Width & Height*/
            VIDDEC_LoadDWORD(nValue, pData);
            nWidth = nValue;
            VIDDEC_LoadDWORD(nValue, pData);
            nHeight = nValue;

            pData += 4; /*Position to compression type*/
            VIDDEC_LoadDWORD(nValue, pData);
            nActualCompression = nValue;

            /*Seting pCSD to proper position*/
            pCSD = pBufferHdr->pBuffer;
            pCSD += CSD_POSITION;
            nSize_CSD = pBufferHdr->nFilledLen - CSD_POSITION;

            if(nActualCompression == FOURCC_WMV3){

                //From VC-1 spec: Table 265: Sequence Layer Data Structure
                sStructRCV.nNumFrames = 0xFFFFFF; /*Infinite frame number*/
                sStructRCV.nFrameType = 0xc5; /*0x85 is the value given by ASF to rcv converter*/
                sStructRCV.nID = 0x04; /*WMV3*/
                sStructRCV.nStructData = 0x018a3106; /*0x06318a01zero fill 0x018a3106*/
                sStructRCV.nVertSize = nHeight;
                sStructRCV.nHorizSize = nWidth;
                sStructRCV.nID2 = 0x0c; /* Fix value */
                sStructRCV.nSequenceHdr = 0x00002a9f; /* This value is not provided by parser, so giving a value from a video*/

                DOMX_DEBUG("initial: nStructData: %x", sStructRCV.nStructData);
                DOMX_DEBUG("pCSD = %x", (OMX_U32)*pCSD);

                sStructRCV.nStructData = (OMX_U32)pCSD[0] << 0  |
                    pCSD[1] << 8  |
                    pCSD[2] << 16 |
                    pCSD[3] << 24;

                DOMX_DEBUG("FINAL: nStructData: %x", sStructRCV.nStructData);

                //Copy RCV structure to actual buffer
                assert(pBufferHdr->nFilledLen < pBufferHdr->nAllocLen);
                pBufferHdr->nFilledLen = sizeof(VIDDEC_WMV_RCV_struct);
                TIMM_OSAL_Memcpy(pBufferHdr->pBuffer, (OMX_U8*)(&sStructRCV),
                    pBufferHdr->nFilledLen);

            }
            else if (nActualCompression == FOURCC_WVC1){
                DOMX_DEBUG("VC-1 Advance Profile prearrange");
                pBufferHdr->nOffset = pBufferHdr->nOffset+52;
                pBufferHdr->nFilledLen= pBufferHdr->nFilledLen-52;
            }
        }
    }

    EXIT:
    DOMX_EXIT("eError: %d", eError);

    return PROXY_EmptyThisBuffer(hComponent, pBufferHdr);
}

#ifdef ENABLE_RAW_BUFFERS_DUMP_UTILITY
/**
* Usage#
* By default this feature is kept disabled to avoid security leaks.
*
* (1) Uncomment the below 2 lines from Android.mk
*     #LOCAL_CFLAGS += -DENABLE_RAW_BUFFERS_DUMP_UTILITY
*     #LOCAL_SHARED_LIBRARIES += libcutils
*     And rebuild the omx proxy common component
*
* (2) Before start playback, make sure that "data" folder has r/w
*   permissions. For this, execute the below
*   mount -o rw,remount -t ext3 /dev/block/mmcblk0p1 /data/
*   chmod 777 /data/
*
* (3) Set the property for number of frames to dump
*  eg: setprop debug.video.dumpframe 10:20
*     would dump frames from 10 to 20.
*
* (4) Pull the frames to PC over adb
*    adb pull /data/frame_10.txt
*
* (5) Analyse on PC tools.
*/

/*
* Method to convert NV12 to YUV420p for PC analysis
*/
static void convertNV12ToYuv420(DebugFrame_Dump *frameInfo, void *dst)
{
	int stride = 4096; /* ARM Page size = 4k */
	uint32_t ybuf_offset = frameInfo->frame_yoffset * stride + frameInfo->frame_xoffset;
	uint8_t* p1y = (uint8_t*)frameInfo->y_uv[0] + ybuf_offset;
	uint8_t* p2y = (uint8_t*) dst;
	int i, j, j1;
	int width = frameInfo->frame_width;
	int height = frameInfo->frame_height;

	LOGD("Coverting NV-12 to YUV420p Width[%d], Height[%d] and Stride[%d] offset[%d]",
	width, height, stride, ybuf_offset);

	/* copy y-buffer, almost bytewise copy, except for stride jumps.*/
	for(i=0;i<height;i++)
	{
		/* copy whole row of Y pixels. source and desination will point to new row each time.*/
		memcpy(p2y+i*width, p1y+i*stride, width);
	}

	/** copy uv buffers
	* rearrange from  packed planar [uvuvuv] to planar [uuu][vvvv] packages pixel wise
	* calculate the offset for UV buffer
	*/
	uint32_t UV_offset = frameInfo->frame_xoffset +
                             (frameInfo->frame_yoffset * stride)/2;

	const uint8_t* p1uv = (uint8_t*)frameInfo->y_uv[1] + UV_offset;

	uint8_t* p2u = ((uint8_t*) dst + (width * height));
	uint8_t* p2v = ((uint8_t*) p2u + ((width/2) * (height/2)));
	for(i=0;(i < height/2);i++)
	{
		for(j=0,j1=0;(j< width/2);j++,j1+=2)
		{
			p2u[j] = p1uv[j1];
			p2v[j] = p1uv[j1+1];
		}
		p1uv+=stride;
		p2u+=width/2;
		p2v+=width/2;
	}
}

void DumpVideoFrame(DebugFrame_Dump *frameInfo)
{
	/* First convert the frame to 420p and then write to SD Card */
	OMX_U32 framesize = (frameInfo->frame_width *
                             frameInfo->frame_height * 3) / 2;
	OMX_U8* localbuffer = malloc(framesize);
	if (localbuffer == NULL)
	{
		LOGE("NO HEAP");
		goto EXIT;
	}
	convertNV12ToYuv420(frameInfo, localbuffer);
	int filedes = -1;
	char framenumber[100];
	sprintf(framenumber, "/data/frame_%ld.txt", frameInfo->runningFrame);
	LOGD("file path %s",framenumber);
	filedes = open(framenumber, O_CREAT | O_WRONLY | O_SYNC | O_TRUNC, 0777);
	if(filedes < 0)
	{
		LOGE("\n!!!!!!!!!Error in file open!!!!!!!! [%d][%s]\n", filedes, strerror(errno));
		goto EXIT;
	}
	int ret = write (filedes, (void*)localbuffer, framesize);
	if (ret < (int)framesize)
	{
		LOGE("File Write Failed");
	}
EXIT:
	if (localbuffer)
	{
		free(localbuffer);
		localbuffer = NULL;
	}
	if (filedes > 0)
	{
		close(filedes);
	}
}

#endif
