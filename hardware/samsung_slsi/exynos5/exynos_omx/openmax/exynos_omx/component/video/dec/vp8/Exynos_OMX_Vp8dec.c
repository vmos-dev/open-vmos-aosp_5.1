/*
 *
 * Copyright 2012 Samsung Electronics S.LSI Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * @file        Exynos_OMX_Vp8dec.c
 * @brief
 * @author      Satish Kumar Reddy (palli.satish@samsung.com)
 * @author      SeungBeom Kim (sbcrux.kim@samsung.com)
 * @version     2.0.0
 * @history
 *   2012.02.20 : Create
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Exynos_OMX_Macros.h"
#include "Exynos_OMX_Basecomponent.h"
#include "Exynos_OMX_Baseport.h"
#include "Exynos_OMX_Vdec.h"
#include "Exynos_OSAL_ETC.h"
#include "Exynos_OSAL_Semaphore.h"
#include "Exynos_OSAL_Thread.h"
#include "library_register.h"
#include "Exynos_OMX_Vp8dec.h"
#include "ExynosVideoApi.h"
#include "Exynos_OSAL_SharedMemory.h"
#include "Exynos_OSAL_Event.h"

#ifdef USE_ANB
#include "Exynos_OSAL_Android.h"
#endif

/* To use CSC_METHOD_HW in EXYNOS OMX, gralloc should allocate physical memory using FIMC */
/* It means GRALLOC_USAGE_HW_FIMC1 should be set on Native Window usage */
#include "csc.h"

#undef  EXYNOS_LOG_TAG
#define EXYNOS_LOG_TAG    "EXYNOS_VP8_DEC"
#define EXYNOS_LOG_OFF
//#define EXYNOS_TRACE_ON
#include "Exynos_OSAL_Log.h"

#define VP8_DEC_NUM_OF_EXTRA_BUFFERS 7

//#define FULL_FRAME_SEARCH /* Full frame search not support*/

static OMX_ERRORTYPE GetCodecInputPrivateData(OMX_PTR codecBuffer, void *pVirtAddr, OMX_U32 *dataSize)
{
    OMX_ERRORTYPE       ret = OMX_ErrorNone;

EXIT:
    return ret;
}

static OMX_ERRORTYPE GetCodecOutputPrivateData(OMX_PTR codecBuffer, void *addr[], int size[])
{
    OMX_ERRORTYPE       ret = OMX_ErrorNone;
    ExynosVideoBuffer  *pCodecBuffer;

    if (codecBuffer == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pCodecBuffer = (ExynosVideoBuffer *)codecBuffer;

    if (addr != NULL) {
        addr[0] = pCodecBuffer->planes[0].addr;
        addr[1] = pCodecBuffer->planes[1].addr;
        addr[2] = pCodecBuffer->planes[2].addr;
    }

    if (size != NULL) {
        size[0] = pCodecBuffer->planes[0].allocSize;
        size[1] = pCodecBuffer->planes[1].allocSize;
        size[2] = pCodecBuffer->planes[2].allocSize;
    }

EXIT:
    return ret;
}

static int Check_VP8_Frame(
    OMX_U8   *pInputStream,
    int       buffSize,
    OMX_U32   flag,
    OMX_BOOL  bPreviousFrameEOF,
    OMX_BOOL *pbEndOfFrame)
{
    /* Uncompressed data Chunk comprises a common
    (for key frames and interframes) 3-byte frame tag that
    contains four fields
    - 1-bit frame type (0 - key frame, 1 - inter frame)
    - 3-bit version number (0 - 3 are defined as four different
                                      profiles with different decoding complexity)
    - 1-bit show_frame flag ( 0 - current frame not for display,
                                          1 - current frame is for dispaly)
    - 19-bit field - size of the first data partition in bytes

    Key Frames : frame tag followed by 7 bytes of uncompressed
    data
    3-bytes : Start code (byte 0: 0x9d,byte 1: 0x01,byte 2: 0x2a)
    Next 4-bytes: Width & height, Horizontal and vertical scale information
    16 bits      :     (2 bits Horizontal Scale << 14) | Width (14 bits)
    16 bits      :     (2 bits Vertical Scale << 14) | Height (14 bits)
    */
    int width, height;
    int horizSscale, vertScale;

    FunctionIn();

    *pbEndOfFrame = OMX_TRUE;

    /*Check for Key frame*/
    if (!(pInputStream[0] & 0x01)){
        /* Key Frame  Start code*/
        if (pInputStream[3] != 0x9d || pInputStream[4] != 0x01 || pInputStream[5]!=0x2a) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, " VP8 Key Frame Start Code not Found");
            *pbEndOfFrame = OMX_FALSE;
        }
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, " VP8 Found Key Frame Start Code");
        width = (pInputStream[6] | (pInputStream[7] << 8)) & 0x3fff;
        horizSscale = pInputStream[7] >> 6;
        height = (pInputStream[8] | (pInputStream[9] << 8)) & 0x3fff;
        vertScale = pInputStream[9] >> 6;
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "width = %d, height = %d, horizSscale = %d, vertScale = %d", width, height, horizSscale, vertScale);
    }

    FunctionOut();
    return buffSize;
}

OMX_BOOL Check_VP8_StartCode(
    OMX_U8     *pInputStream,
    OMX_U32     streamSize)
{
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "streamSize: %d",streamSize);
    if (streamSize < 3) {
        return OMX_FALSE;
    }

    if (!(pInputStream[0] & 0x01)){
        /* Key Frame  Start code*/
        if (pInputStream[3] != 0x9d || pInputStream[4] != 0x01 || pInputStream[5]!=0x2a) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, " VP8 Key Frame Start Code not Found");
            return OMX_FALSE;
        }
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, " VP8 Found Key Frame Start Code");
    }

    return OMX_TRUE;
}

OMX_ERRORTYPE VP8CodecOpen(EXYNOS_VP8DEC_HANDLE *pVp8Dec)
{
    OMX_ERRORTYPE            ret        = OMX_ErrorNone;
    ExynosVideoDecOps       *pDecOps    = NULL;
    ExynosVideoDecBufferOps *pInbufOps  = NULL;
    ExynosVideoDecBufferOps *pOutbufOps = NULL;

    FunctionIn();

    if (pVp8Dec == NULL) {
        ret = OMX_ErrorBadParameter;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorBadParameter, Line:%d", __LINE__);
        goto EXIT;
    }

    /* alloc ops structure */
    pDecOps    = (ExynosVideoDecOps *)Exynos_OSAL_Malloc(sizeof(ExynosVideoDecOps));
    pInbufOps  = (ExynosVideoDecBufferOps *)Exynos_OSAL_Malloc(sizeof(ExynosVideoDecBufferOps));
    pOutbufOps = (ExynosVideoDecBufferOps *)Exynos_OSAL_Malloc(sizeof(ExynosVideoDecBufferOps));

    if ((pDecOps == NULL) || (pInbufOps == NULL) || (pOutbufOps == NULL)) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to allocate decoder ops buffer");
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    pVp8Dec->hMFCVp8Handle.pDecOps    = pDecOps;
    pVp8Dec->hMFCVp8Handle.pInbufOps  = pInbufOps;
    pVp8Dec->hMFCVp8Handle.pOutbufOps = pOutbufOps;

    /* function pointer mapping */
    pDecOps->nSize    = sizeof(ExynosVideoDecOps);
    pInbufOps->nSize  = sizeof(ExynosVideoDecBufferOps);
    pOutbufOps->nSize = sizeof(ExynosVideoDecBufferOps);

    Exynos_Video_Register_Decoder(pDecOps, pInbufOps, pOutbufOps);

    /* check mandatory functions for decoder ops */
    if ((pDecOps->Init == NULL) || (pDecOps->Finalize == NULL) ||
        (pDecOps->Get_ActualBufferCount == NULL) || (pDecOps->Set_FrameTag == NULL) ||
        (pDecOps->Get_FrameTag == NULL)) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Mandatory functions must be supplied");
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    /* check mandatory functions for buffer ops */
    if ((pInbufOps->Setup == NULL) || (pOutbufOps->Setup == NULL) ||
        (pInbufOps->Run == NULL) || (pOutbufOps->Run == NULL) ||
        (pInbufOps->Stop == NULL) || (pOutbufOps->Stop == NULL) ||
        (pInbufOps->Enqueue == NULL) || (pOutbufOps->Enqueue == NULL) ||
        (pInbufOps->Dequeue == NULL) || (pOutbufOps->Dequeue == NULL)) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Mandatory functions must be supplied");
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    /* alloc context, open, querycap */
    pVp8Dec->hMFCVp8Handle.hMFCHandle = pVp8Dec->hMFCVp8Handle.pDecOps->Init(V4L2_MEMORY_DMABUF);
    if (pVp8Dec->hMFCVp8Handle.hMFCHandle == NULL) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to allocate context buffer");
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    ret = OMX_ErrorNone;

EXIT:
    if (ret != OMX_ErrorNone) {
        if (pDecOps != NULL) {
            Exynos_OSAL_Free(pDecOps);
            pVp8Dec->hMFCVp8Handle.pDecOps = NULL;
        }
        if (pInbufOps != NULL) {
            Exynos_OSAL_Free(pInbufOps);
            pVp8Dec->hMFCVp8Handle.pInbufOps = NULL;
        }
        if (pOutbufOps != NULL) {
            Exynos_OSAL_Free(pOutbufOps);
            pVp8Dec->hMFCVp8Handle.pOutbufOps = NULL;
        }
    }

    FunctionOut();

    return ret;
}

OMX_ERRORTYPE VP8CodecClose(EXYNOS_VP8DEC_HANDLE *pVp8Dec)
{
    OMX_ERRORTYPE            ret        = OMX_ErrorNone;
    void                    *hMFCHandle = NULL;
    ExynosVideoDecOps       *pDecOps    = NULL;
    ExynosVideoDecBufferOps *pInbufOps  = NULL;
    ExynosVideoDecBufferOps *pOutbufOps = NULL;

    FunctionIn();

    if (pVp8Dec == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    hMFCHandle = pVp8Dec->hMFCVp8Handle.hMFCHandle;
    pDecOps    = pVp8Dec->hMFCVp8Handle.pDecOps;
    pInbufOps  = pVp8Dec->hMFCVp8Handle.pInbufOps;
    pOutbufOps = pVp8Dec->hMFCVp8Handle.pOutbufOps;

    if (hMFCHandle != NULL) {
        pDecOps->Finalize(hMFCHandle);
        pVp8Dec->hMFCVp8Handle.hMFCHandle = NULL;
    }
    if (pOutbufOps != NULL) {
        Exynos_OSAL_Free(pOutbufOps);
        pVp8Dec->hMFCVp8Handle.pOutbufOps = NULL;
    }
    if (pInbufOps != NULL) {
        Exynos_OSAL_Free(pInbufOps);
        pVp8Dec->hMFCVp8Handle.pInbufOps = NULL;
    }
    if (pDecOps != NULL) {
        Exynos_OSAL_Free(pDecOps);
        pVp8Dec->hMFCVp8Handle.pDecOps = NULL;
    }

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE VP8CodecStart(OMX_COMPONENTTYPE *pOMXComponent, OMX_U32 nPortIndex)
{
    OMX_ERRORTYPE            ret = OMX_ErrorNone;
    void                    *hMFCHandle = NULL;
    ExynosVideoDecOps       *pDecOps    = NULL;
    ExynosVideoDecBufferOps *pInbufOps  = NULL;
    ExynosVideoDecBufferOps *pOutbufOps = NULL;
    EXYNOS_OMX_VIDEODEC_COMPONENT *pVideoDec = NULL;
    EXYNOS_VP8DEC_HANDLE   *pVp8Dec = NULL;

    FunctionIn();

    if (pOMXComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pVideoDec = (EXYNOS_OMX_VIDEODEC_COMPONENT *)((EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate)->hComponentHandle;
    if (pVideoDec == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pVp8Dec = (EXYNOS_VP8DEC_HANDLE *)pVideoDec->hCodecHandle;
    if (pVp8Dec == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    hMFCHandle = pVp8Dec->hMFCVp8Handle.hMFCHandle;
    pDecOps    = pVp8Dec->hMFCVp8Handle.pDecOps;
    pInbufOps  = pVp8Dec->hMFCVp8Handle.pInbufOps;
    pOutbufOps = pVp8Dec->hMFCVp8Handle.pOutbufOps;

    if (nPortIndex == INPUT_PORT_INDEX)
        pInbufOps->Run(hMFCHandle);
    else if (nPortIndex == OUTPUT_PORT_INDEX)
        pOutbufOps->Run(hMFCHandle);

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE VP8CodecStop(OMX_COMPONENTTYPE *pOMXComponent, OMX_U32 nPortIndex)
{
    OMX_ERRORTYPE            ret = OMX_ErrorNone;
    void                    *hMFCHandle = NULL;
    ExynosVideoDecOps       *pDecOps    = NULL;
    ExynosVideoDecBufferOps *pInbufOps  = NULL;
    ExynosVideoDecBufferOps *pOutbufOps = NULL;
    EXYNOS_OMX_VIDEODEC_COMPONENT *pVideoDec = NULL;
    EXYNOS_VP8DEC_HANDLE   *pVp8Dec = NULL;

    FunctionIn();

    if (pOMXComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pVideoDec = (EXYNOS_OMX_VIDEODEC_COMPONENT *)((EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate)->hComponentHandle;
    if (pVideoDec == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pVp8Dec = (EXYNOS_VP8DEC_HANDLE *)pVideoDec->hCodecHandle;
    if (pVp8Dec == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    hMFCHandle = pVp8Dec->hMFCVp8Handle.hMFCHandle;
    pDecOps    = pVp8Dec->hMFCVp8Handle.pDecOps;
    pInbufOps  = pVp8Dec->hMFCVp8Handle.pInbufOps;
    pOutbufOps = pVp8Dec->hMFCVp8Handle.pOutbufOps;

    if ((nPortIndex == INPUT_PORT_INDEX) && (pInbufOps != NULL)) {
        pInbufOps->Stop(hMFCHandle);
    } else if ((nPortIndex == OUTPUT_PORT_INDEX) && (pOutbufOps != NULL)) {
        EXYNOS_OMX_BASECOMPONENT *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
        EXYNOS_OMX_BASEPORT *pExynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
        pOutbufOps->Stop(hMFCHandle);
        if (pExynosOutputPort->bufferProcessType == BUFFER_SHARE)
            pOutbufOps->Clear_RegisteredBuffer(hMFCHandle);
    }
    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE VP8CodecOutputBufferProcessRun(OMX_COMPONENTTYPE *pOMXComponent, OMX_U32 nPortIndex)
{
    OMX_ERRORTYPE            ret = OMX_ErrorNone;
    void                    *hMFCHandle = NULL;
    ExynosVideoDecOps       *pDecOps    = NULL;
    ExynosVideoDecBufferOps *pInbufOps  = NULL;
    ExynosVideoDecBufferOps *pOutbufOps = NULL;
    EXYNOS_OMX_VIDEODEC_COMPONENT *pVideoDec = NULL;
    EXYNOS_VP8DEC_HANDLE   *pVp8Dec = NULL;

    FunctionIn();

    if (pOMXComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pVideoDec = (EXYNOS_OMX_VIDEODEC_COMPONENT *)((EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate)->hComponentHandle;
    if (pVideoDec == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pVp8Dec = (EXYNOS_VP8DEC_HANDLE *)pVideoDec->hCodecHandle;
    if (pVp8Dec == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    hMFCHandle = pVp8Dec->hMFCVp8Handle.hMFCHandle;
    pDecOps    = pVp8Dec->hMFCVp8Handle.pDecOps;
    pInbufOps  = pVp8Dec->hMFCVp8Handle.pInbufOps;
    pOutbufOps = pVp8Dec->hMFCVp8Handle.pOutbufOps;

    if (nPortIndex == INPUT_PORT_INDEX) {
        if (pVp8Dec->bSourceStart == OMX_FALSE) {
            Exynos_OSAL_SignalSet(pVp8Dec->hSourceStartEvent);
            Exynos_OSAL_SleepMillisec(0);
        }
    }

    if (nPortIndex == OUTPUT_PORT_INDEX) {
        if (pVp8Dec->bDestinationStart == OMX_FALSE) {
            Exynos_OSAL_SignalSet(pVp8Dec->hDestinationStartEvent);
            Exynos_OSAL_SleepMillisec(0);
        }
    }

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE VP8CodecRegistCodecBuffers(
    OMX_COMPONENTTYPE   *pOMXComponent,
    OMX_U32              nPortIndex,
    OMX_U32              nBufferCnt)
{
    OMX_ERRORTYPE                    ret                = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT        *pExynosComponent   = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT   *pVideoDec          = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_VP8DEC_HANDLE            *pVp8Dec            = (EXYNOS_VP8DEC_HANDLE *)((EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
    void                            *hMFCHandle         = pVp8Dec->hMFCVp8Handle.hMFCHandle;
    CODEC_DEC_BUFFER               **ppCodecBuffer      = NULL;
    ExynosVideoDecBufferOps         *pBufOps            = NULL;
    ExynosVideoPlane                *pPlanes            = NULL;

    OMX_U32 nPlaneCnt = 0;
    int i, j;

    FunctionIn();

    if (nPortIndex == INPUT_PORT_INDEX) {
        ppCodecBuffer   = &(pVideoDec->pMFCDecInputBuffer[0]);
        nPlaneCnt       = MFC_INPUT_BUFFER_PLANE;
        pBufOps         = pVp8Dec->hMFCVp8Handle.pInbufOps;
    } else {
        ppCodecBuffer   = &(pVideoDec->pMFCDecOutputBuffer[0]);
        nPlaneCnt       = MFC_OUTPUT_BUFFER_PLANE;
        pBufOps         = pVp8Dec->hMFCVp8Handle.pOutbufOps;
    }

    pPlanes = (ExynosVideoPlane *)Exynos_OSAL_Malloc(sizeof(ExynosVideoPlane) * nPlaneCnt);
    if (pPlanes == NULL) {
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    /* Register buffer */
    for (i = 0; i < nBufferCnt; i++) {
        for (j = 0; j < nPlaneCnt; j++) {
            pPlanes[j].addr         = ppCodecBuffer[i]->pVirAddr[j];
            pPlanes[j].fd           = ppCodecBuffer[i]->fd[j];
            pPlanes[j].allocSize    = ppCodecBuffer[i]->bufferSize[j];
        }

        if (pBufOps->Register(hMFCHandle, pPlanes, nPlaneCnt) != VIDEO_ERROR_NONE) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "PORT[%d]: Failed to Register buffer", nPortIndex);
            ret = OMX_ErrorInsufficientResources;
            Exynos_OSAL_Free(pPlanes);
            goto EXIT;
        }
    }

    Exynos_OSAL_Free(pPlanes);

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE VP8CodecReconfigAllBuffers(
    OMX_COMPONENTTYPE   *pOMXComponent,
    OMX_U32              nPortIndex)
{
    OMX_ERRORTYPE                    ret                = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT        *pExynosComponent   = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT   *pVideoDec          = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_OMX_BASEPORT             *pExynosPort        = &pExynosComponent->pExynosPort[nPortIndex];
    EXYNOS_VP8DEC_HANDLE            *pVp8Dec            = (EXYNOS_VP8DEC_HANDLE *)pVideoDec->hCodecHandle;
    void                            *hMFCHandle         = pVp8Dec->hMFCVp8Handle.hMFCHandle;
    ExynosVideoDecBufferOps         *pBufferOps         = NULL;

    FunctionIn();

    if ((nPortIndex == INPUT_PORT_INDEX) &&
        (pVp8Dec->bSourceStart == OMX_TRUE)) {
        ret = OMX_ErrorNotImplemented;
        goto EXIT;
    } else if ((nPortIndex == OUTPUT_PORT_INDEX) &&
               (pVp8Dec->bDestinationStart == OMX_TRUE)) {
        pBufferOps  = pVp8Dec->hMFCVp8Handle.pOutbufOps;

        if (pExynosPort->bufferProcessType & BUFFER_COPY) {
            /**********************************/
            /* Codec Buffer Free & Unregister */
            /**********************************/
            Exynos_Free_CodecBuffers(pOMXComponent, OUTPUT_PORT_INDEX);
            Exynos_CodecBufferReset(pExynosComponent, OUTPUT_PORT_INDEX);
            pBufferOps->Clear_RegisteredBuffer(hMFCHandle);
            pBufferOps->Cleanup_Buffer(hMFCHandle);
        } else if (pExynosPort->bufferProcessType & BUFFER_SHARE) {
            /**********************************/
            /* Codec Buffer Unregister */
            /**********************************/
            pBufferOps->Clear_RegisteredBuffer(hMFCHandle);
            pBufferOps->Cleanup_Buffer(hMFCHandle);
        }
        /******************************************************/
        /* V4L2 Destnation Setup for DPB Buffer Number Change */
        /******************************************************/
        VP8CodecDstSetup(pOMXComponent);
        pVideoDec->bReconfigDPB = OMX_FALSE;

        Exynos_ResolutionUpdate(pOMXComponent);
    } else {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE VP8CodecEnQueueAllBuffer(OMX_COMPONENTTYPE *pOMXComponent, OMX_U32 nPortIndex)
{
    OMX_ERRORTYPE                  ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT *pVideoDec = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_VP8DEC_HANDLE         *pVp8Dec = (EXYNOS_VP8DEC_HANDLE *)((EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
    void                          *hMFCHandle = pVp8Dec->hMFCVp8Handle.hMFCHandle;
    EXYNOS_OMX_BASEPORT           *pExynosInputPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT           *pExynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    int i, nOutbufs;

    ExynosVideoDecOps       *pDecOps    = pVp8Dec->hMFCVp8Handle.pDecOps;
    ExynosVideoDecBufferOps *pInbufOps  = pVp8Dec->hMFCVp8Handle.pInbufOps;
    ExynosVideoDecBufferOps *pOutbufOps = pVp8Dec->hMFCVp8Handle.pOutbufOps;

    FunctionIn();

    if ((nPortIndex != INPUT_PORT_INDEX) && (nPortIndex != OUTPUT_PORT_INDEX)) {
        ret = OMX_ErrorBadPortIndex;
        goto EXIT;
    }

    if ((nPortIndex == INPUT_PORT_INDEX) &&
        (pVp8Dec->bSourceStart == OMX_TRUE)) {
        Exynos_CodecBufferReset(pExynosComponent, INPUT_PORT_INDEX);

        for (i = 0; i < MFC_INPUT_BUFFER_NUM_MAX; i++) {
            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "pVideoDec->pMFCDecInputBuffer[%d]: 0x%x", i, pVideoDec->pMFCDecInputBuffer[i]);
            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "pVideoDec->pMFCDecInputBuffer[%d]->pVirAddr[0]: 0x%x", i, pVideoDec->pMFCDecInputBuffer[i]->pVirAddr[0]);

            Exynos_CodecBufferEnQueue(pExynosComponent, INPUT_PORT_INDEX, pVideoDec->pMFCDecInputBuffer[i]);
        }

        pInbufOps->Clear_Queue(hMFCHandle);
    } else if ((nPortIndex == OUTPUT_PORT_INDEX) &&
               (pVp8Dec->bDestinationStart == OMX_TRUE)) {
        OMX_U32 dataLen[MFC_OUTPUT_BUFFER_PLANE] = {0, 0};
        ExynosVideoBuffer *pBuffer = NULL;

        Exynos_CodecBufferReset(pExynosComponent, OUTPUT_PORT_INDEX);

        nOutbufs = pDecOps->Get_ActualBufferCount(hMFCHandle);
        nOutbufs += EXTRA_DPB_NUM;
        for (i = 0; i < nOutbufs; i++) {
            Exynos_CodecBufferEnQueue(pExynosComponent, OUTPUT_PORT_INDEX, pVideoDec->pMFCDecOutputBuffer[i]);
        }
        pOutbufOps->Clear_Queue(hMFCHandle);
    } else {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE VP8CodecCheckResolutionChange(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE                  ret                = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent   = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT *pVideoDec          = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_VP8DEC_HANDLE          *pVp8Dec            = (EXYNOS_VP8DEC_HANDLE *)pVideoDec->hCodecHandle;
    void                          *hMFCHandle         = pVp8Dec->hMFCVp8Handle.hMFCHandle;
    EXYNOS_OMX_BASEPORT           *pInputPort         = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT           *pOutputPort        = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    ExynosVideoDecOps             *pDecOps            = pVp8Dec->hMFCVp8Handle.pDecOps;
    ExynosVideoDecBufferOps       *pOutbufOps         = pVp8Dec->hMFCVp8Handle.pOutbufOps;

    OMX_CONFIG_RECTTYPE *pCropRectangle = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE *pInputPortDefinition = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE *pOutputPortDefinition = NULL;

    FunctionIn();
    /* get geometry for output */
    Exynos_OSAL_Memset(&pVp8Dec->hMFCVp8Handle.codecOutbufConf, 0, sizeof(ExynosVideoGeometry));
    if (pOutbufOps->Get_Geometry(hMFCHandle, &pVp8Dec->hMFCVp8Handle.codecOutbufConf) != VIDEO_ERROR_NONE) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to get geometry for parsed header info");
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    /* get dpb count */
    pVp8Dec->hMFCVp8Handle.maxDPBNum = pDecOps->Get_ActualBufferCount(hMFCHandle);
    if (pVideoDec->bThumbnailMode == OMX_FALSE)
        pVp8Dec->hMFCVp8Handle.maxDPBNum += EXTRA_DPB_NUM;
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "Vp8CodecSetup nOutbufs: %d", pVp8Dec->hMFCVp8Handle.maxDPBNum);

    pVp8Dec->hMFCVp8Handle.bConfiguredMFCSrc = OMX_TRUE;

    if (pVideoDec->bReconfigDPB != OMX_TRUE) {
        pCropRectangle = &(pOutputPort->cropRectangle);
        pInputPortDefinition = &(pInputPort->portDefinition);
        pOutputPortDefinition = &(pOutputPort->portDefinition);
    } else {
        pCropRectangle = &(pOutputPort->newCropRectangle);
        pInputPortDefinition = &(pInputPort->newPortDefinition);
        pOutputPortDefinition = &(pOutputPort->newPortDefinition);
    }

    pCropRectangle->nTop     = pVp8Dec->hMFCVp8Handle.codecOutbufConf.cropRect.nTop;
    pCropRectangle->nLeft    = pVp8Dec->hMFCVp8Handle.codecOutbufConf.cropRect.nLeft;
    pCropRectangle->nWidth   = pVp8Dec->hMFCVp8Handle.codecOutbufConf.cropRect.nWidth;
    pCropRectangle->nHeight  = pVp8Dec->hMFCVp8Handle.codecOutbufConf.cropRect.nHeight;

    if (pOutputPort->bufferProcessType & BUFFER_COPY) {
        if ((pVideoDec->bReconfigDPB) ||
            (pInputPort->portDefinition.format.video.nFrameWidth != pVp8Dec->hMFCVp8Handle.codecOutbufConf.nFrameWidth) ||
            (pInputPort->portDefinition.format.video.nFrameHeight != pVp8Dec->hMFCVp8Handle.codecOutbufConf.nFrameHeight)) {
            pInputPortDefinition->format.video.nFrameWidth  = pVp8Dec->hMFCVp8Handle.codecOutbufConf.nFrameWidth;
            pInputPortDefinition->format.video.nFrameHeight = pVp8Dec->hMFCVp8Handle.codecOutbufConf.nFrameHeight;
            pInputPortDefinition->format.video.nStride      = ((pVp8Dec->hMFCVp8Handle.codecOutbufConf.nFrameWidth + 15) & (~15));
            pInputPortDefinition->format.video.nSliceHeight = ((pVp8Dec->hMFCVp8Handle.codecOutbufConf.nFrameHeight + 15) & (~15));
            pOutputPortDefinition->nBufferCountActual       = pOutputPort->portDefinition.nBufferCountActual;
            pOutputPortDefinition->nBufferCountMin          = pOutputPort->portDefinition.nBufferCountMin;
            if (pVideoDec->bReconfigDPB != OMX_TRUE)
                Exynos_UpdateFrameSize(pOMXComponent);
            pOutputPort->exceptionFlag = NEED_PORT_DISABLE;

            /** Send Port Settings changed call back **/
            (*(pExynosComponent->pCallbacks->EventHandler))
                (pOMXComponent,
                 pExynosComponent->callbackData,
                 OMX_EventPortSettingsChanged, /* The command was completed */
                 OMX_DirOutput, /* This is the port index */
                 0,
                 NULL);
        }
    } else if (pOutputPort->bufferProcessType & BUFFER_SHARE) {
        if ((pVideoDec->bReconfigDPB) ||
            (pInputPort->portDefinition.format.video.nFrameWidth != pVp8Dec->hMFCVp8Handle.codecOutbufConf.nFrameWidth) ||
            (pInputPort->portDefinition.format.video.nFrameHeight != pVp8Dec->hMFCVp8Handle.codecOutbufConf.nFrameHeight) ||
            (pOutputPort->portDefinition.nBufferCountActual != pVp8Dec->hMFCVp8Handle.maxDPBNum)) {
            pInputPortDefinition->format.video.nFrameWidth  = pVp8Dec->hMFCVp8Handle.codecOutbufConf.nFrameWidth;
            pInputPortDefinition->format.video.nFrameHeight = pVp8Dec->hMFCVp8Handle.codecOutbufConf.nFrameHeight;
            pInputPortDefinition->format.video.nStride      = ((pVp8Dec->hMFCVp8Handle.codecOutbufConf.nFrameWidth + 15) & (~15));
            pInputPortDefinition->format.video.nSliceHeight = ((pVp8Dec->hMFCVp8Handle.codecOutbufConf.nFrameHeight + 15) & (~15));
            pOutputPortDefinition->nBufferCountActual       = pVp8Dec->hMFCVp8Handle.maxDPBNum;
            pOutputPortDefinition->nBufferCountMin          = pVp8Dec->hMFCVp8Handle.maxDPBNum;
            if (pVideoDec->bReconfigDPB != OMX_TRUE)
                Exynos_UpdateFrameSize(pOMXComponent);
            pOutputPort->exceptionFlag = NEED_PORT_DISABLE;

            /** Send Port Settings changed call back **/
            (*(pExynosComponent->pCallbacks->EventHandler))
                (pOMXComponent,
                 pExynosComponent->callbackData,
                 OMX_EventPortSettingsChanged, /* The command was completed */
                 OMX_DirOutput, /* This is the port index */
                 0,
                 NULL);
        }
    }
    if ((pVideoDec->bReconfigDPB == OMX_TRUE) ||
        (pVp8Dec->hMFCVp8Handle.codecOutbufConf.nFrameWidth != pVp8Dec->hMFCVp8Handle.codecOutbufConf.cropRect.nWidth) ||
        (pVp8Dec->hMFCVp8Handle.codecOutbufConf.nFrameHeight != pVp8Dec->hMFCVp8Handle.codecOutbufConf.cropRect.nHeight)) {
        /* Check Crop */
        pInputPortDefinition->format.video.nFrameWidth  = pVp8Dec->hMFCVp8Handle.codecOutbufConf.nFrameWidth;
        pInputPortDefinition->format.video.nFrameHeight = pVp8Dec->hMFCVp8Handle.codecOutbufConf.nFrameHeight;
        pInputPortDefinition->format.video.nStride      = ((pVp8Dec->hMFCVp8Handle.codecOutbufConf.nFrameWidth + 15) & (~15));
        pInputPortDefinition->format.video.nSliceHeight = ((pVp8Dec->hMFCVp8Handle.codecOutbufConf.nFrameHeight + 15) & (~15));
        if (pVideoDec->bReconfigDPB != OMX_TRUE)
            Exynos_UpdateFrameSize(pOMXComponent);

        /** Send crop info call back **/
        (*(pExynosComponent->pCallbacks->EventHandler))
            (pOMXComponent,
             pExynosComponent->callbackData,
             OMX_EventPortSettingsChanged, /* The command was completed */
             OMX_DirOutput, /* This is the port index */
             OMX_IndexConfigCommonOutputCrop,
             NULL);
    }

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE VP8CodecSrcSetup(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *pSrcInputData)
{
    OMX_ERRORTYPE                  ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT *pVideoDec = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_VP8DEC_HANDLE          *pVp8Dec = (EXYNOS_VP8DEC_HANDLE *)((EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
    void                          *hMFCHandle = pVp8Dec->hMFCVp8Handle.hMFCHandle;
    EXYNOS_OMX_BASEPORT           *pExynosInputPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT           *pExynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    OMX_U32                     oneFrameSize = pSrcInputData->dataLen;

    ExynosVideoDecOps       *pDecOps    = pVp8Dec->hMFCVp8Handle.pDecOps;
    ExynosVideoDecBufferOps *pInbufOps  = pVp8Dec->hMFCVp8Handle.pInbufOps;
    ExynosVideoDecBufferOps *pOutbufOps = pVp8Dec->hMFCVp8Handle.pOutbufOps;
    ExynosVideoGeometry      bufferConf;
    OMX_U32                  inputBufferNumber = 0;
    int i;

    FunctionIn();

    if ((oneFrameSize <= 0) && (pSrcInputData->nFlags & OMX_BUFFERFLAG_EOS)) {
        OMX_BUFFERHEADERTYPE *OMXBuffer = NULL;
        OMXBuffer = Exynos_OutputBufferGetQueue_Direct(pExynosComponent);
        if (OMXBuffer == NULL) {
            ret = OMX_ErrorUndefined;
            goto EXIT;
        }

        OMXBuffer->nTimeStamp = pSrcInputData->timeStamp;
        OMXBuffer->nFlags = pSrcInputData->nFlags;
        Exynos_OMX_OutputBufferReturn(pOMXComponent, OMXBuffer);

        ret = OMX_ErrorNone;
        goto EXIT;
    }

    if (pVideoDec->bThumbnailMode == OMX_TRUE)
        pDecOps->Set_IFrameDecoding(hMFCHandle);

    /* input buffer info */
    Exynos_OSAL_Memset(&bufferConf, 0, sizeof(bufferConf));
    bufferConf.eCompressionFormat = VIDEO_CODING_VP8;
    pInbufOps->Set_Shareable(hMFCHandle);
    if (pExynosInputPort->bufferProcessType == BUFFER_SHARE) {
        bufferConf.nSizeImage = pExynosInputPort->portDefinition.format.video.nFrameWidth
                                * pExynosInputPort->portDefinition.format.video.nFrameHeight * 3 / 2;
        inputBufferNumber = MAX_VIDEO_INPUTBUFFER_NUM;
    } else if ((pExynosInputPort->bufferProcessType & BUFFER_COPY) == BUFFER_COPY) {
        bufferConf.nSizeImage = DEFAULT_MFC_INPUT_BUFFER_SIZE;
        inputBufferNumber = MFC_INPUT_BUFFER_NUM_MAX;
    }

    /* should be done before prepare input buffer */
    if (pInbufOps->Enable_Cacheable(hMFCHandle) != VIDEO_ERROR_NONE) {
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    /* set input buffer geometry */
    if (pInbufOps->Set_Geometry(hMFCHandle, &bufferConf) != VIDEO_ERROR_NONE) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to set geometry for input buffer");
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    /* setup input buffer */
    if (pInbufOps->Setup(hMFCHandle, inputBufferNumber) != VIDEO_ERROR_NONE) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to setup input buffer");
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    if ((pExynosInputPort->bufferProcessType & BUFFER_COPY) == BUFFER_COPY) {
        ret = VP8CodecRegistCodecBuffers(pOMXComponent, INPUT_PORT_INDEX, MFC_INPUT_BUFFER_NUM_MAX);
        if (ret != OMX_ErrorNone)
            goto EXIT;
    } else if (pExynosInputPort->bufferProcessType == BUFFER_SHARE) {
        /* Register input buffer */
        for (i = 0; i < pExynosInputPort->portDefinition.nBufferCountActual; i++) {
            ExynosVideoPlane plane;
            plane.addr = pExynosInputPort->extendBufferHeader[i].OMXBufferHeader->pBuffer;
            plane.allocSize = pExynosInputPort->extendBufferHeader[i].OMXBufferHeader->nAllocLen;
            plane.fd = pExynosInputPort->extendBufferHeader[i].buf_fd[0];
            if (pInbufOps->Register(hMFCHandle, &plane, MFC_INPUT_BUFFER_PLANE) != VIDEO_ERROR_NONE) {
                Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to Register input buffer");
                ret = OMX_ErrorInsufficientResources;
                goto EXIT;
            }
        }
    }

    /* set output geometry */
    Exynos_OSAL_Memset(&bufferConf, 0, sizeof(bufferConf));
    pVp8Dec->hMFCVp8Handle.MFCOutputColorType = bufferConf.eColorFormat = VIDEO_COLORFORMAT_NV12_TILED;
    if (pOutbufOps->Set_Geometry(hMFCHandle, &bufferConf) != VIDEO_ERROR_NONE) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to set geometry for output buffer");
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    /* input buffer enqueue for header parsing */
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "oneFrameSize: %d", oneFrameSize);
    if (pInbufOps->Enqueue(hMFCHandle, (unsigned char **)&pSrcInputData->buffer.singlePlaneBuffer.dataBuffer,
                        (unsigned int *)&oneFrameSize, MFC_INPUT_BUFFER_PLANE, pSrcInputData->bufferHeader) != VIDEO_ERROR_NONE) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to enqueue input buffer for header parsing");
//        ret = OMX_ErrorInsufficientResources;
        ret = (OMX_ERRORTYPE)OMX_ErrorCodecInit;
        goto EXIT;
    }

    /* start header parsing */
    if (pInbufOps->Run(hMFCHandle) != VIDEO_ERROR_NONE) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to run input buffer for header parsing");
        ret = OMX_ErrorCodecInit;
        goto EXIT;
    }

    VP8CodecCheckResolutionChange(pOMXComponent);

    Exynos_OSAL_SleepMillisec(0);
    ret = OMX_ErrorInputDataDecodeYet;
    VP8CodecStop(pOMXComponent, INPUT_PORT_INDEX);

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE VP8CodecDstSetup(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE                  ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT *pVideoDec = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_VP8DEC_HANDLE         *pVp8Dec = (EXYNOS_VP8DEC_HANDLE *)((EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
    void                          *hMFCHandle = pVp8Dec->hMFCVp8Handle.hMFCHandle;
    EXYNOS_OMX_BASEPORT           *pExynosInputPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT           *pExynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];

    ExynosVideoDecOps       *pDecOps    = pVp8Dec->hMFCVp8Handle.pDecOps;
    ExynosVideoDecBufferOps *pInbufOps  = pVp8Dec->hMFCVp8Handle.pInbufOps;
    ExynosVideoDecBufferOps *pOutbufOps = pVp8Dec->hMFCVp8Handle.pOutbufOps;

    int i, nOutbufs;

    FunctionIn();

    if ((pExynosOutputPort->bufferProcessType & BUFFER_COPY) == BUFFER_COPY) {
        /* BUFFER_COPY case, get dpb count */
        nOutbufs = pVp8Dec->hMFCVp8Handle.maxDPBNum;

        /* should be done before prepare output buffer */
        if (pOutbufOps->Enable_Cacheable(hMFCHandle) != VIDEO_ERROR_NONE) {
            ret = OMX_ErrorInsufficientResources;
            goto EXIT;
        }
    } else {
        /*BUFFER_SHERE case, get dpb count */
        nOutbufs = pExynosOutputPort->portDefinition.nBufferCountActual;
    }
    if (pOutbufOps->Enable_DynamicDPB(hMFCHandle) != VIDEO_ERROR_NONE) {
        ret = OMX_ErrorUndefined;
        goto EXIT;
    }

    pOutbufOps->Set_Shareable(hMFCHandle);
    if (pOutbufOps->Setup(hMFCHandle, MAX_OUTPUTBUFFER_NUM_DYNAMIC) != VIDEO_ERROR_NONE) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to setup output buffer");
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    ExynosVideoPlane planes[MFC_OUTPUT_BUFFER_PLANE];
    OMX_U32 nAllocLen[MFC_OUTPUT_BUFFER_PLANE] = {0, 0};
    OMX_U32 dataLen[MFC_OUTPUT_BUFFER_PLANE] = {0, 0};
    int plane;

    nAllocLen[0] = calc_plane(pVp8Dec->hMFCVp8Handle.codecOutbufConf.nFrameWidth,
                        pVp8Dec->hMFCVp8Handle.codecOutbufConf.nFrameHeight);
    nAllocLen[1] = calc_plane(pVp8Dec->hMFCVp8Handle.codecOutbufConf.nFrameWidth,
                        pVp8Dec->hMFCVp8Handle.codecOutbufConf.nFrameHeight >> 1);

    if ((pExynosOutputPort->bufferProcessType & BUFFER_COPY) == BUFFER_COPY) {
        ret = Exynos_Allocate_CodecBuffers(pOMXComponent, OUTPUT_PORT_INDEX, nOutbufs, nAllocLen);
        if (ret != OMX_ErrorNone)
            goto EXIT;
        for (i = 0; i < nOutbufs; i++) {
            /* Enqueue output buffer */
            pOutbufOps->ExtensionEnqueue(hMFCHandle,
                            (unsigned char **)pVideoDec->pMFCDecOutputBuffer[i]->pVirAddr,
                            (unsigned char **)pVideoDec->pMFCDecOutputBuffer[i]->fd,
                            (unsigned int *)pVideoDec->pMFCDecOutputBuffer[i]->bufferSize,
                            (unsigned int *)dataLen, MFC_OUTPUT_BUFFER_PLANE, NULL);
        }

        if (pOutbufOps->Run(hMFCHandle) != VIDEO_ERROR_NONE) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to run output buffer");
            ret = OMX_ErrorInsufficientResources;
            goto EXIT;
        }
    } else if (pExynosOutputPort->bufferProcessType == BUFFER_SHARE) {
        /*************/
        /*    TBD    */
        /*************/
#ifdef USE_ANB
        if ((pExynosOutputPort->bIsANBEnabled == OMX_FALSE) &&
            (pExynosOutputPort->bStoreMetaData == OMX_FALSE)) {
            ret = OMX_ErrorNotImplemented;
            goto EXIT;
        }
#else
        ret = OMX_ErrorNotImplemented;
        goto EXIT;
#endif
    }

    pVp8Dec->hMFCVp8Handle.bConfiguredMFCDst = OMX_TRUE;

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_VP8Dec_GetParameter(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nParamIndex,
    OMX_INOUT OMX_PTR     pComponentParameterStructure)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;

    FunctionIn();

    if (hComponent == NULL || pComponentParameterStructure == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = Exynos_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }
    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    if (pExynosComponent->currentState == OMX_StateInvalid ) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    switch (nParamIndex) {
    case OMX_IndexParamStandardComponentRole:
    {
        OMX_PARAM_COMPONENTROLETYPE *pComponentRole = (OMX_PARAM_COMPONENTROLETYPE *)pComponentParameterStructure;
        ret = Exynos_OMX_Check_SizeVersion(pComponentRole, sizeof(OMX_PARAM_COMPONENTROLETYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        Exynos_OSAL_Strcpy((char *)pComponentRole->cRole, EXYNOS_OMX_COMPONENT_VP8_DEC_ROLE);
    }
        break;
    case OMX_IndexParamVideoErrorCorrection:
    {
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pDstErrorCorrectionType = (OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pSrcErrorCorrectionType = NULL;
        EXYNOS_VP8DEC_HANDLE                *pVp8Dec                 = NULL;

        ret = Exynos_OMX_Check_SizeVersion(pDstErrorCorrectionType, sizeof(OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pDstErrorCorrectionType->nPortIndex != INPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pVp8Dec = (EXYNOS_VP8DEC_HANDLE *)((EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
        pSrcErrorCorrectionType = &pVp8Dec->errorCorrectionType[INPUT_PORT_INDEX];

        pDstErrorCorrectionType->bEnableHEC = pSrcErrorCorrectionType->bEnableHEC;
        pDstErrorCorrectionType->bEnableResync = pSrcErrorCorrectionType->bEnableResync;
        pDstErrorCorrectionType->nResynchMarkerSpacing = pSrcErrorCorrectionType->nResynchMarkerSpacing;
        pDstErrorCorrectionType->bEnableDataPartitioning = pSrcErrorCorrectionType->bEnableDataPartitioning;
        pDstErrorCorrectionType->bEnableRVLC = pSrcErrorCorrectionType->bEnableRVLC;
    }
        break;
    default:
        ret = Exynos_OMX_VideoDecodeGetParameter(hComponent, nParamIndex, pComponentParameterStructure);
        break;
    }
EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_VP8Dec_SetParameter(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nIndex,
    OMX_IN OMX_PTR        pComponentParameterStructure)
{
    OMX_ERRORTYPE           ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;

    FunctionIn();

    if (hComponent == NULL || pComponentParameterStructure == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = Exynos_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }
    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    if (pExynosComponent->currentState == OMX_StateInvalid ) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    switch (nIndex) {
    case OMX_IndexParamStandardComponentRole:
    {
        OMX_PARAM_COMPONENTROLETYPE *pComponentRole = (OMX_PARAM_COMPONENTROLETYPE*)pComponentParameterStructure;

        ret = Exynos_OMX_Check_SizeVersion(pComponentRole, sizeof(OMX_PARAM_COMPONENTROLETYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if ((pExynosComponent->currentState != OMX_StateLoaded) && (pExynosComponent->currentState != OMX_StateWaitForResources)) {
            ret = OMX_ErrorIncorrectStateOperation;
            goto EXIT;
        }

        if (!Exynos_OSAL_Strcmp((char*)pComponentRole->cRole, EXYNOS_OMX_COMPONENT_VP8_DEC_ROLE)) {
            pExynosComponent->pExynosPort[INPUT_PORT_INDEX].portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingVPX;
        } else {
            ret = OMX_ErrorBadParameter;
            goto EXIT;
        }
    }
        break;
    case OMX_IndexParamVideoErrorCorrection:
    {
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pSrcErrorCorrectionType = (OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pDstErrorCorrectionType = NULL;
        EXYNOS_VP8DEC_HANDLE                *pVp8Dec                 = NULL;

        ret = Exynos_OMX_Check_SizeVersion(pSrcErrorCorrectionType, sizeof(OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pSrcErrorCorrectionType->nPortIndex != INPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pVp8Dec = (EXYNOS_VP8DEC_HANDLE *)((EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
        pDstErrorCorrectionType = &pVp8Dec->errorCorrectionType[INPUT_PORT_INDEX];

        pDstErrorCorrectionType->bEnableHEC = pSrcErrorCorrectionType->bEnableHEC;
        pDstErrorCorrectionType->bEnableResync = pSrcErrorCorrectionType->bEnableResync;
        pDstErrorCorrectionType->nResynchMarkerSpacing = pSrcErrorCorrectionType->nResynchMarkerSpacing;
        pDstErrorCorrectionType->bEnableDataPartitioning = pSrcErrorCorrectionType->bEnableDataPartitioning;
        pDstErrorCorrectionType->bEnableRVLC = pSrcErrorCorrectionType->bEnableRVLC;
    }
        break;
    default:
        ret = Exynos_OMX_VideoDecodeSetParameter(hComponent, nIndex, pComponentParameterStructure);
        break;
    }
EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_VP8Dec_GetConfig(
    OMX_HANDLETYPE hComponent,
    OMX_INDEXTYPE  nIndex,
    OMX_PTR        pComponentConfigStructure)
{
    OMX_ERRORTYPE             ret              = OMX_ErrorNone;
    OMX_COMPONENTTYPE        *pOMXComponent    = NULL;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;

    FunctionIn();

    if (hComponent == NULL || pComponentConfigStructure == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = Exynos_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }
    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    if (pExynosComponent->currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    switch (nIndex) {
    default:
        ret = Exynos_OMX_VideoDecodeGetConfig(hComponent, nIndex, pComponentConfigStructure);
        break;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_VP8Dec_SetConfig(
    OMX_HANDLETYPE hComponent,
    OMX_INDEXTYPE  nIndex,
    OMX_PTR        pComponentConfigStructure)
{
    OMX_ERRORTYPE             ret              = OMX_ErrorNone;
    OMX_COMPONENTTYPE        *pOMXComponent    = NULL;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;

    FunctionIn();

    if (hComponent == NULL || pComponentConfigStructure == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = Exynos_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }
    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    if (pExynosComponent->currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    switch (nIndex) {
    default:
        ret = Exynos_OMX_VideoDecodeSetConfig(hComponent, nIndex, pComponentConfigStructure);
        break;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_VP8Dec_GetExtensionIndex(
    OMX_IN  OMX_HANDLETYPE  hComponent,
    OMX_IN  OMX_STRING      cParameterName,
    OMX_OUT OMX_INDEXTYPE  *pIndexType)
{
    OMX_ERRORTYPE             ret              = OMX_ErrorNone;
    OMX_COMPONENTTYPE        *pOMXComponent    = NULL;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = Exynos_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }
    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    if ((cParameterName == NULL) || (pIndexType == NULL)) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if (pExynosComponent->currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    ret = Exynos_OMX_VideoDecodeGetExtensionIndex(hComponent, cParameterName, pIndexType);

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_VP8Dec_ComponentRoleEnum(
    OMX_HANDLETYPE hComponent,
    OMX_U8        *cRole,
    OMX_U32        nIndex)
{
    OMX_ERRORTYPE             ret              = OMX_ErrorNone;
    OMX_COMPONENTTYPE        *pOMXComponent    = NULL;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;

    FunctionIn();

    if ((hComponent == NULL) || (cRole == NULL)) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if (nIndex == (MAX_COMPONENT_ROLE_NUM-1)) {
        Exynos_OSAL_Strcpy((char *)cRole, EXYNOS_OMX_COMPONENT_VP8_DEC_ROLE);
        ret = OMX_ErrorNone;
    } else {
        ret = OMX_ErrorNoMore;
    }

EXIT:
    FunctionOut();

    return ret;
}

/* MFC Init */
OMX_ERRORTYPE Exynos_VP8Dec_Init(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE                  ret               = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent  = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT *pVideoDec         = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_OMX_BASEPORT           *pExynosInputPort  = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT           *pExynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    EXYNOS_VP8DEC_HANDLE          *pVp8Dec           = (EXYNOS_VP8DEC_HANDLE *)pVideoDec->hCodecHandle;
    OMX_PTR                        hMFCHandle        = pVp8Dec->hMFCVp8Handle.hMFCHandle;

    ExynosVideoDecOps       *pDecOps    = NULL;
    ExynosVideoDecBufferOps *pInbufOps  = NULL;
    ExynosVideoDecBufferOps *pOutbufOps = NULL;

    CSC_METHOD csc_method = CSC_METHOD_SW;
    int i, plane;

    FunctionIn();

    pVp8Dec->hMFCVp8Handle.bConfiguredMFCSrc = OMX_FALSE;
    pVp8Dec->hMFCVp8Handle.bConfiguredMFCDst = OMX_FALSE;
    pExynosComponent->bUseFlagEOF = OMX_TRUE;
    pExynosComponent->bSaveFlagEOS = OMX_FALSE;
    pExynosComponent->bBehaviorEOS = OMX_FALSE;

    /* H.264 Codec Open */
    ret = VP8CodecOpen(pVp8Dec);
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    pDecOps    = pVp8Dec->hMFCVp8Handle.pDecOps;
    pInbufOps  = pVp8Dec->hMFCVp8Handle.pInbufOps;
    pOutbufOps = pVp8Dec->hMFCVp8Handle.pOutbufOps;

    if ((pExynosInputPort->bufferProcessType & BUFFER_COPY) == BUFFER_COPY) {
        OMX_U32 nPlaneSize[MFC_INPUT_BUFFER_PLANE] = {DEFAULT_MFC_INPUT_BUFFER_SIZE};
        Exynos_OSAL_SemaphoreCreate(&pExynosInputPort->codecSemID);
        Exynos_OSAL_QueueCreate(&pExynosInputPort->codecBufferQ, MAX_QUEUE_ELEMENTS);
        ret = Exynos_Allocate_CodecBuffers(pOMXComponent, INPUT_PORT_INDEX, MFC_INPUT_BUFFER_NUM_MAX, nPlaneSize);
        if (ret != OMX_ErrorNone)
            goto EXIT;

        for (i = 0; i < MFC_INPUT_BUFFER_NUM_MAX; i++)
            Exynos_CodecBufferEnQueue(pExynosComponent, INPUT_PORT_INDEX, pVideoDec->pMFCDecInputBuffer[i]);
    } else if (pExynosInputPort->bufferProcessType == BUFFER_SHARE) {
        /*************/
        /*    TBD    */
        /*************/
        /* Does not require any actions. */
    }

    if ((pExynosOutputPort->bufferProcessType & BUFFER_COPY) == BUFFER_COPY) {
        Exynos_OSAL_SemaphoreCreate(&pExynosOutputPort->codecSemID);
        Exynos_OSAL_QueueCreate(&pExynosOutputPort->codecBufferQ, MAX_QUEUE_ELEMENTS);
    } else if (pExynosOutputPort->bufferProcessType == BUFFER_SHARE) {
        /*************/
        /*    TBD    */
        /*************/
        /* Does not require any actions. */
    }

    pVp8Dec->bSourceStart = OMX_FALSE;
    Exynos_OSAL_SignalCreate(&pVp8Dec->hSourceStartEvent);
    pVp8Dec->bDestinationStart = OMX_FALSE;
    Exynos_OSAL_SignalCreate(&pVp8Dec->hDestinationStartEvent);

    Exynos_OSAL_Memset(pExynosComponent->timeStamp, -19771003, sizeof(OMX_TICKS) * MAX_TIMESTAMP);
    Exynos_OSAL_Memset(pExynosComponent->nFlags, 0, sizeof(OMX_U32) * MAX_FLAGS);
    pVp8Dec->hMFCVp8Handle.indexTimestamp = 0;
    pVp8Dec->hMFCVp8Handle.outputIndexTimestamp = 0;

    pExynosComponent->getAllDelayBuffer = OMX_FALSE;

#if 0//defined(USE_CSC_GSCALER)
    csc_method = CSC_METHOD_HW; //in case of Use ION buffer.
#endif
    pVideoDec->csc_handle = csc_init(csc_method);
    if (pVideoDec->csc_handle == NULL) {
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    pVideoDec->csc_set_format = OMX_FALSE;

EXIT:
    FunctionOut();

    return ret;
}

/* MFC Terminate */
OMX_ERRORTYPE Exynos_VP8Dec_Terminate(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT *pVideoDec = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_OMX_BASEPORT      *pExynosInputPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT      *pExynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    EXYNOS_VP8DEC_HANDLE    *pVp8Dec = (EXYNOS_VP8DEC_HANDLE *)((EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
    OMX_PTR                hMFCHandle = pVp8Dec->hMFCVp8Handle.hMFCHandle;

    ExynosVideoDecOps       *pDecOps    = pVp8Dec->hMFCVp8Handle.pDecOps;
    ExynosVideoDecBufferOps *pInbufOps  = pVp8Dec->hMFCVp8Handle.pInbufOps;
    ExynosVideoDecBufferOps *pOutbufOps = pVp8Dec->hMFCVp8Handle.pOutbufOps;

    int i, plane;

    FunctionIn();

    if (pVideoDec->csc_handle != NULL) {
        csc_deinit(pVideoDec->csc_handle);
        pVideoDec->csc_handle = NULL;
    }

    Exynos_OSAL_SignalTerminate(pVp8Dec->hDestinationStartEvent);
    pVp8Dec->hDestinationStartEvent = NULL;
    pVp8Dec->bDestinationStart = OMX_FALSE;
    Exynos_OSAL_SignalTerminate(pVp8Dec->hSourceStartEvent);
    pVp8Dec->hSourceStartEvent = NULL;
    pVp8Dec->bSourceStart = OMX_FALSE;

    if ((pExynosOutputPort->bufferProcessType & BUFFER_COPY) == BUFFER_COPY) {
        Exynos_Free_CodecBuffers(pOMXComponent, OUTPUT_PORT_INDEX);
        Exynos_OSAL_QueueTerminate(&pExynosOutputPort->codecBufferQ);
        Exynos_OSAL_SemaphoreTerminate(pExynosOutputPort->codecSemID);
    } else if (pExynosOutputPort->bufferProcessType == BUFFER_SHARE) {
        /*************/
        /*    TBD    */
        /*************/
        /* Does not require any actions. */
    }

    if ((pExynosInputPort->bufferProcessType & BUFFER_COPY) == BUFFER_COPY) {
        Exynos_Free_CodecBuffers(pOMXComponent, INPUT_PORT_INDEX);
        Exynos_OSAL_QueueTerminate(&pExynosInputPort->codecBufferQ);
        Exynos_OSAL_SemaphoreTerminate(pExynosInputPort->codecSemID);
    } else if (pExynosInputPort->bufferProcessType == BUFFER_SHARE) {
        /*************/
        /*    TBD    */
        /*************/
        /* Does not require any actions. */
    }
    VP8CodecClose(pVp8Dec);

    Exynos_ResetAllPortConfig(pOMXComponent);

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_VP8Dec_SrcIn(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *pSrcInputData)
{
    OMX_ERRORTYPE               ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT *pVideoDec = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_VP8DEC_HANDLE         *pVp8Dec = (EXYNOS_VP8DEC_HANDLE *)((EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
    void                          *hMFCHandle = pVp8Dec->hMFCVp8Handle.hMFCHandle;
    EXYNOS_OMX_BASEPORT *pExynosInputPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT *pExynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    OMX_U32  oneFrameSize = pSrcInputData->dataLen;
    ExynosVideoDecOps       *pDecOps    = pVp8Dec->hMFCVp8Handle.pDecOps;
    ExynosVideoDecBufferOps *pInbufOps  = pVp8Dec->hMFCVp8Handle.pInbufOps;
    ExynosVideoDecBufferOps *pOutbufOps = pVp8Dec->hMFCVp8Handle.pOutbufOps;
    ExynosVideoErrorType codecReturn = VIDEO_ERROR_NONE;
    int i;

    FunctionIn();

    if (pVp8Dec->hMFCVp8Handle.bConfiguredMFCSrc == OMX_FALSE) {
        ret = VP8CodecSrcSetup(pOMXComponent, pSrcInputData);
        goto EXIT;
    }
    if (pVp8Dec->hMFCVp8Handle.bConfiguredMFCDst == OMX_FALSE) {
        ret = VP8CodecDstSetup(pOMXComponent);
    }

    if ((Check_VP8_StartCode(pSrcInputData->buffer.singlePlaneBuffer.dataBuffer, oneFrameSize) == OMX_TRUE) ||
        ((pSrcInputData->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS)) {
        pExynosComponent->timeStamp[pVp8Dec->hMFCVp8Handle.indexTimestamp] = pSrcInputData->timeStamp;
        pExynosComponent->nFlags[pVp8Dec->hMFCVp8Handle.indexTimestamp] = pSrcInputData->nFlags;
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "input timestamp %lld us (%.2f secs), Tag: %d, nFlags: 0x%x", pSrcInputData->timeStamp, pSrcInputData->timeStamp / 1E6, pVp8Dec->hMFCVp8Handle.indexTimestamp, pSrcInputData->nFlags);
        pDecOps->Set_FrameTag(hMFCHandle, pVp8Dec->hMFCVp8Handle.indexTimestamp);
        pVp8Dec->hMFCVp8Handle.indexTimestamp++;
        pVp8Dec->hMFCVp8Handle.indexTimestamp %= MAX_TIMESTAMP;

        /* queue work for input buffer */
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "oneFrameSize: %d, bufferHeader: 0x%x, dataBuffer: 0x%x", oneFrameSize, pSrcInputData->bufferHeader, pSrcInputData->buffer.singlePlaneBuffer.dataBuffer);
        codecReturn = pInbufOps->Enqueue(hMFCHandle, (unsigned char **)&pSrcInputData->buffer.singlePlaneBuffer.dataBuffer,
                                    (unsigned int *)&oneFrameSize, MFC_INPUT_BUFFER_PLANE, pSrcInputData->bufferHeader);
        if (codecReturn != VIDEO_ERROR_NONE) {
            ret = (OMX_ERRORTYPE)OMX_ErrorCodecDecode;
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s : %d", __FUNCTION__, __LINE__);
            goto EXIT;
        }
        VP8CodecStart(pOMXComponent, INPUT_PORT_INDEX);
        if (pVp8Dec->bSourceStart == OMX_FALSE) {
            pVp8Dec->bSourceStart = OMX_TRUE;
            Exynos_OSAL_SignalSet(pVp8Dec->hSourceStartEvent);
            Exynos_OSAL_SleepMillisec(0);
        }
        if (pVp8Dec->bDestinationStart == OMX_FALSE) {
            pVp8Dec->bDestinationStart = OMX_TRUE;
            Exynos_OSAL_SignalSet(pVp8Dec->hDestinationStartEvent);
            Exynos_OSAL_SleepMillisec(0);
        }
    }

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_VP8Dec_SrcOut(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *pSrcOutputData)
{
    OMX_ERRORTYPE                  ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT *pVideoDec = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_VP8DEC_HANDLE         *pVp8Dec = (EXYNOS_VP8DEC_HANDLE *)((EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
    void                          *hMFCHandle = pVp8Dec->hMFCVp8Handle.hMFCHandle;
    EXYNOS_OMX_BASEPORT     *pExynosInputPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    ExynosVideoDecOps       *pDecOps    = pVp8Dec->hMFCVp8Handle.pDecOps;
    ExynosVideoDecBufferOps *pInbufOps  = pVp8Dec->hMFCVp8Handle.pInbufOps;
    ExynosVideoBuffer       *pVideoBuffer;

    FunctionIn();

    pVideoBuffer = pInbufOps->Dequeue(hMFCHandle);

    pSrcOutputData->dataLen       = 0;
    pSrcOutputData->usedDataLen   = 0;
    pSrcOutputData->remainDataLen = 0;
    pSrcOutputData->nFlags    = 0;
    pSrcOutputData->timeStamp = 0;

    if (pVideoBuffer == NULL) {
        pSrcOutputData->buffer.singlePlaneBuffer.dataBuffer = NULL;
        pSrcOutputData->allocSize  = 0;
        pSrcOutputData->pPrivate = NULL;
        pSrcOutputData->bufferHeader = NULL;
    } else {
        pSrcOutputData->buffer.singlePlaneBuffer.dataBuffer = pVideoBuffer->planes[0].addr;
        pSrcOutputData->buffer.singlePlaneBuffer.fd = pVideoBuffer->planes[0].fd;
        pSrcOutputData->allocSize  = pVideoBuffer->planes[0].allocSize;

        if ((pExynosInputPort->bufferProcessType & BUFFER_COPY) == BUFFER_COPY) {
            int i = 0;
            while (pSrcOutputData->buffer.singlePlaneBuffer.dataBuffer != pVideoDec->pMFCDecInputBuffer[i]->pVirAddr[0]) {
                if (i >= MFC_INPUT_BUFFER_NUM_MAX) {
                    Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Can not find buffer");
                    ret = (OMX_ERRORTYPE)OMX_ErrorCodecDecode;
                    goto EXIT;
                }
                i++;
            }
            pVideoDec->pMFCDecInputBuffer[i]->dataSize = 0;
            pSrcOutputData->pPrivate = pVideoDec->pMFCDecInputBuffer[i];
        }

        /* For Share Buffer */
        pSrcOutputData->bufferHeader = (OMX_BUFFERHEADERTYPE*)pVideoBuffer->pPrivate;
    }

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_VP8Dec_DstIn(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *pDstInputData)
{
    OMX_ERRORTYPE                  ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT *pVideoDec = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_VP8DEC_HANDLE         *pVp8Dec = (EXYNOS_VP8DEC_HANDLE *)((EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
    void                          *hMFCHandle = pVp8Dec->hMFCVp8Handle.hMFCHandle;
    EXYNOS_OMX_BASEPORT *pExynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    ExynosVideoDecOps       *pDecOps    = pVp8Dec->hMFCVp8Handle.pDecOps;
    ExynosVideoDecBufferOps *pOutbufOps = pVp8Dec->hMFCVp8Handle.pOutbufOps;
    OMX_U32 dataLen[MFC_OUTPUT_BUFFER_PLANE] = {0,};
    ExynosVideoErrorType codecReturn = VIDEO_ERROR_NONE;

    FunctionIn();

    if (pDstInputData->buffer.multiPlaneBuffer.dataBuffer[0] == NULL) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to find input buffer");
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "%s : %d => ADDR[0]: 0x%x, ADDR[1]: 0x%x, FD[0]:%d, FD[1]:%d", __FUNCTION__, __LINE__,
                                        pDstInputData->buffer.multiPlaneBuffer.dataBuffer[0],
                                        pDstInputData->buffer.multiPlaneBuffer.dataBuffer[1],
                                        pDstInputData->buffer.multiPlaneBuffer.fd[0],
                                        pDstInputData->buffer.multiPlaneBuffer.fd[1]);

    OMX_U32 nAllocLen[VIDEO_BUFFER_MAX_PLANES] = {0, 0, 0};
    nAllocLen[0] = pExynosOutputPort->portDefinition.format.video.nFrameWidth * pExynosOutputPort->portDefinition.format.video.nFrameHeight;
    nAllocLen[1] = pExynosOutputPort->portDefinition.format.video.nFrameWidth * pExynosOutputPort->portDefinition.format.video.nFrameHeight / 2;

    codecReturn = pOutbufOps->ExtensionEnqueue(hMFCHandle,
                                (unsigned char **)pDstInputData->buffer.multiPlaneBuffer.dataBuffer,
                                (unsigned int **)pDstInputData->buffer.multiPlaneBuffer.fd,
                                (unsigned int *)nAllocLen, (unsigned int *)dataLen,
                                MFC_OUTPUT_BUFFER_PLANE, pDstInputData->bufferHeader);
    if (codecReturn != VIDEO_ERROR_NONE) {
        if (codecReturn != VIDEO_ERROR_WRONGBUFFERSIZE) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s : %d", __FUNCTION__, __LINE__);
            ret = (OMX_ERRORTYPE)OMX_ErrorCodecDecode;
        }
        goto EXIT;
    }
    VP8CodecStart(pOMXComponent, OUTPUT_PORT_INDEX);

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_VP8Dec_DstOut(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *pDstOutputData)
{
    OMX_ERRORTYPE                  ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT *pVideoDec = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_VP8DEC_HANDLE         *pVp8Dec = (EXYNOS_VP8DEC_HANDLE *)((EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
    void                          *hMFCHandle = pVp8Dec->hMFCVp8Handle.hMFCHandle;
    EXYNOS_OMX_BASEPORT *pExynosInputPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT *pExynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    ExynosVideoDecOps       *pDecOps    = pVp8Dec->hMFCVp8Handle.pDecOps;
    ExynosVideoDecBufferOps *pOutbufOps = pVp8Dec->hMFCVp8Handle.pOutbufOps;
    ExynosVideoBuffer       *pVideoBuffer;
    ExynosVideoBuffer        videoBuffer;
    ExynosVideoFrameStatusType displayStatus = VIDEO_FRAME_STATUS_UNKNOWN;
    ExynosVideoGeometry *bufferGeometry;
    DECODE_CODEC_EXTRA_BUFFERINFO *pBufferInfo = NULL;
    OMX_S32 indexTimestamp = 0;
    int plane;

    FunctionIn();

    if (pVp8Dec->bDestinationStart == OMX_FALSE) {
        ret = OMX_ErrorNone;
        goto EXIT;
    }

    while (1) {
        Exynos_OSAL_Memset(&videoBuffer, 0, sizeof(ExynosVideoBuffer));
        if (pOutbufOps->ExtensionDequeue(hMFCHandle, &videoBuffer) == VIDEO_ERROR_NONE) {
            pVideoBuffer = &videoBuffer;
        } else {
            pVideoBuffer = NULL;
            ret = OMX_ErrorNone;
            goto EXIT;
        }
        displayStatus = pVideoBuffer->displayStatus;
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "displayStatus: 0x%x", displayStatus);

        if ((displayStatus == VIDEO_FRAME_STATUS_DISPLAY_DECODING) ||
            (displayStatus == VIDEO_FRAME_STATUS_DISPLAY_ONLY) ||
            (displayStatus == VIDEO_FRAME_STATUS_CHANGE_RESOL) ||
            (displayStatus == VIDEO_FRAME_STATUS_DECODING_FINISHED) ||
            (CHECK_PORT_BEING_FLUSHED(pExynosOutputPort))) {
            if (pVideoBuffer != NULL) {
                ret = OMX_ErrorNone;
                break;
            } else {
                ret = OMX_ErrorUndefined;
                break;
            }
        }
    }

    if ((pVideoDec->bThumbnailMode == OMX_FALSE) &&
        (displayStatus == VIDEO_FRAME_STATUS_CHANGE_RESOL)) {
        if (pVideoDec->bReconfigDPB != OMX_TRUE) {
            pExynosOutputPort->exceptionFlag = NEED_PORT_FLUSH;
            pVideoDec->bReconfigDPB = OMX_TRUE;
            VP8CodecCheckResolutionChange(pOMXComponent);
            pVideoDec->csc_set_format = OMX_FALSE;
        }
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "VIDEO_FRAME_STATUS_CHANGE_RESOL");
        ret = OMX_ErrorNone;
        goto EXIT;
    }

    if (ret != OMX_ErrorNone)
        goto EXIT;

    pVp8Dec->hMFCVp8Handle.outputIndexTimestamp++;
    pVp8Dec->hMFCVp8Handle.outputIndexTimestamp %= MAX_TIMESTAMP;

    pDstOutputData->allocSize = pDstOutputData->dataLen = 0;
    for (plane = 0; plane < MFC_OUTPUT_BUFFER_PLANE; plane++) {
        pDstOutputData->buffer.multiPlaneBuffer.dataBuffer[plane] = pVideoBuffer->planes[plane].addr;
        pDstOutputData->buffer.multiPlaneBuffer.fd[plane] = pVideoBuffer->planes[plane].fd;
        pDstOutputData->allocSize += pVideoBuffer->planes[plane].allocSize;
        pDstOutputData->dataLen +=  pVideoBuffer->planes[plane].dataSize;
    }
    pDstOutputData->usedDataLen = 0;
    pDstOutputData->pPrivate = pVideoBuffer;
    if (pExynosOutputPort->bufferProcessType & BUFFER_COPY) {
        int i = 0;
        pDstOutputData->pPrivate = NULL;

        for (i = 0; i < MFC_OUTPUT_BUFFER_NUM_MAX; i++) {
            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "pVideoDec->pMFCDecOutputBuffer[%d]:0x%x", i, pVideoDec->pMFCDecOutputBuffer[i]);
            if ((pVideoDec->pMFCDecOutputBuffer[i] != NULL) &&
                (pDstOutputData->buffer.multiPlaneBuffer.dataBuffer[0] ==
                 pVideoDec->pMFCDecOutputBuffer[i]->pVirAddr[0])) {
                pDstOutputData->pPrivate = pVideoDec->pMFCDecOutputBuffer[i];
                Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "%s : %d => ADDR[0]: 0x%x, ADDR[1]: 0x%x, FD[0]:%d, FD[1]:%d", __FUNCTION__, __LINE__,
                                    pVideoDec->pMFCDecOutputBuffer[i]->pVirAddr[0],
                                    pVideoDec->pMFCDecOutputBuffer[i]->pVirAddr[1],
                                    pVideoDec->pMFCDecOutputBuffer[i]->fd[0],
                                    pVideoDec->pMFCDecOutputBuffer[i]->fd[1]);
                break;
            }
        }

        if (pDstOutputData->pPrivate == NULL) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Can not find buffer");
            ret = (OMX_ERRORTYPE)OMX_ErrorCodecDecode;
            goto EXIT;
        }
    }
    /* For Share Buffer */
    pDstOutputData->bufferHeader = (OMX_BUFFERHEADERTYPE *)pVideoBuffer->pPrivate;

    pBufferInfo = (DECODE_CODEC_EXTRA_BUFFERINFO *)pDstOutputData->extInfo;
    bufferGeometry = &pVp8Dec->hMFCVp8Handle.codecOutbufConf;
    pBufferInfo->imageWidth = bufferGeometry->nFrameWidth;
    pBufferInfo->imageHeight = bufferGeometry->nFrameHeight;
    Exynos_OSAL_Memcpy(&pBufferInfo->PDSB, &pVideoBuffer->PDSB, sizeof(PrivateDataShareBuffer));
    switch (bufferGeometry->eColorFormat) {
    case VIDEO_COLORFORMAT_NV12:
        pBufferInfo->ColorFormat = OMX_COLOR_FormatYUV420SemiPlanar;
        break;
    case VIDEO_COLORFORMAT_NV12_TILED:
    default:
        pBufferInfo->ColorFormat = OMX_SEC_COLOR_FormatNV12Tiled;
        break;
    }

    indexTimestamp = pDecOps->Get_FrameTag(hMFCHandle);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "out indexTimestamp: %d", indexTimestamp);
    if ((indexTimestamp < 0) || (indexTimestamp >= MAX_TIMESTAMP)) {
        if ((pExynosComponent->checkTimeStamp.needSetStartTimeStamp != OMX_TRUE) &&
            (pExynosComponent->checkTimeStamp.needCheckStartTimeStamp != OMX_TRUE)) {
            pDstOutputData->timeStamp = pExynosComponent->timeStamp[pVp8Dec->hMFCVp8Handle.outputIndexTimestamp];
            pDstOutputData->nFlags = pExynosComponent->nFlags[pVp8Dec->hMFCVp8Handle.outputIndexTimestamp];
            pExynosComponent->nFlags[pVp8Dec->hMFCVp8Handle.outputIndexTimestamp] = 0x00;
            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "missing out indexTimestamp: %d", indexTimestamp);
        } else {
            pDstOutputData->timeStamp = 0x00;
            pDstOutputData->nFlags = 0x00;
        }
    } else {
        /* For timestamp correction. if mfc support frametype detect */
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "disp_pic_frame_type: %d", pVideoBuffer->frameType);
#ifdef NEED_TIMESTAMP_REORDER
        if ((pVideoBuffer->frameType == VIDEO_FRAME_I)) {
            pDstOutputData->timeStamp = pExynosComponent->timeStamp[indexTimestamp];
            pDstOutputData->nFlags = pExynosComponent->nFlags[indexTimestamp];
            pExynosComponent->nFlags[indexTimestamp] = 0x00;
            pVp8Dec->hMFCVp8Handle.outputIndexTimestamp = indexTimestamp;
        } else {
            pDstOutputData->timeStamp = pExynosComponent->timeStamp[pVp8Dec->hMFCVp8Handle.outputIndexTimestamp];
            pDstOutputData->nFlags = pExynosComponent->nFlags[pVp8Dec->hMFCVp8Handle.outputIndexTimestamp];
            pExynosComponent->nFlags[pVp8Dec->hMFCVp8Handle.outputIndexTimestamp] = 0x00;
        }
#else
        pDstOutputData->timeStamp = pExynosComponent->timeStamp[indexTimestamp];
        pDstOutputData->nFlags = pExynosComponent->nFlags[indexTimestamp];
        pExynosComponent->nFlags[indexTimestamp] = 0x00;
#endif
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "timestamp %lld us (%.2f secs), indexTimestamp: %d, nFlags: 0x%x", pDstOutputData->timeStamp, pDstOutputData->timeStamp / 1E6, indexTimestamp, pDstOutputData->nFlags);
    }

    if ((displayStatus == VIDEO_FRAME_STATUS_DECODING_FINISHED) ||
        ((pDstOutputData->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS)) {
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "displayStatus:%d, nFlags0x%x", displayStatus, pDstOutputData->nFlags);
        pDstOutputData->remainDataLen = 0;
        if (((pDstOutputData->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS) &&
            (pExynosComponent->bBehaviorEOS == OMX_TRUE)) {
            pDstOutputData->remainDataLen = bufferGeometry->nFrameWidth * bufferGeometry->nFrameHeight * 3 / 2;
            pExynosComponent->bBehaviorEOS = OMX_FALSE;
        }
    } else {
        pDstOutputData->remainDataLen = bufferGeometry->nFrameWidth * bufferGeometry->nFrameHeight * 3 / 2;
    }

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_VP8Dec_srcInputBufferProcess(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *pSrcInputData)
{
    OMX_ERRORTYPE             ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_VP8DEC_HANDLE    *pVp8Dec = (EXYNOS_VP8DEC_HANDLE *)((EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
    EXYNOS_OMX_BASEPORT      *pExynosInputPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];

    FunctionIn();

    if ((!CHECK_PORT_ENABLED(pExynosInputPort)) || (!CHECK_PORT_POPULATED(pExynosInputPort))) {
        ret = OMX_ErrorNone;
        goto EXIT;
    }
    if (OMX_FALSE == Exynos_Check_BufferProcess_State(pExynosComponent, INPUT_PORT_INDEX)) {
        ret = OMX_ErrorNone;
        goto EXIT;
    }

    ret = Exynos_VP8Dec_SrcIn(pOMXComponent, pSrcInputData);
    if ((ret != OMX_ErrorNone) && (ret != OMX_ErrorInputDataDecodeYet)) {
        pExynosComponent->pCallbacks->EventHandler((OMX_HANDLETYPE)pOMXComponent,
                                                pExynosComponent->callbackData,
                                                OMX_EventError, ret, 0, NULL);
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_VP8Dec_srcOutputBufferProcess(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *pSrcOutputData)
{
    OMX_ERRORTYPE             ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_VP8DEC_HANDLE    *pVp8Dec = (EXYNOS_VP8DEC_HANDLE *)((EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
    EXYNOS_OMX_BASEPORT     *pExynosInputPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];

    FunctionIn();

    if ((!CHECK_PORT_ENABLED(pExynosInputPort)) || (!CHECK_PORT_POPULATED(pExynosInputPort))) {
        ret = OMX_ErrorNone;
        goto EXIT;
    }

    if ((pExynosInputPort->bufferProcessType & BUFFER_COPY) == BUFFER_COPY) {
        if (OMX_FALSE == Exynos_Check_BufferProcess_State(pExynosComponent, INPUT_PORT_INDEX)) {
            ret = OMX_ErrorNone;
            goto EXIT;
        }
    }
    if ((pVp8Dec->bSourceStart == OMX_FALSE) &&
       (!CHECK_PORT_BEING_FLUSHED(pExynosInputPort))) {
        Exynos_OSAL_SignalWait(pVp8Dec->hSourceStartEvent, DEF_MAX_WAIT_TIME);
        Exynos_OSAL_SignalReset(pVp8Dec->hSourceStartEvent);
    }

    ret = Exynos_VP8Dec_SrcOut(pOMXComponent, pSrcOutputData);
    if ((ret != OMX_ErrorNone) &&
        (pExynosComponent->currentState == OMX_StateExecuting)) {
        pExynosComponent->pCallbacks->EventHandler((OMX_HANDLETYPE)pOMXComponent,
                                                pExynosComponent->callbackData,
                                                OMX_EventError, ret, 0, NULL);
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_VP8Dec_dstInputBufferProcess(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *pDstInputData)
{
    OMX_ERRORTYPE             ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_VP8DEC_HANDLE    *pVp8Dec = (EXYNOS_VP8DEC_HANDLE *)((EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
    EXYNOS_OMX_BASEPORT      *pExynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];

    FunctionIn();

    if ((!CHECK_PORT_ENABLED(pExynosOutputPort)) || (!CHECK_PORT_POPULATED(pExynosOutputPort))) {
        ret = OMX_ErrorNone;
        goto EXIT;
    }
    if (OMX_FALSE == Exynos_Check_BufferProcess_State(pExynosComponent, OUTPUT_PORT_INDEX)) {
        ret = OMX_ErrorNone;
        goto EXIT;
    }
    if (pExynosOutputPort->bufferProcessType == BUFFER_SHARE) {
        if ((pVp8Dec->bDestinationStart == OMX_FALSE) &&
           (!CHECK_PORT_BEING_FLUSHED(pExynosOutputPort))) {
            Exynos_OSAL_SignalWait(pVp8Dec->hDestinationStartEvent, DEF_MAX_WAIT_TIME);
            Exynos_OSAL_SignalReset(pVp8Dec->hDestinationStartEvent);
        }
    }
    if (pVp8Dec->hMFCVp8Handle.bConfiguredMFCDst == OMX_TRUE) {
        ret = Exynos_VP8Dec_DstIn(pOMXComponent, pDstInputData);
        if (ret != OMX_ErrorNone) {
            pExynosComponent->pCallbacks->EventHandler((OMX_HANDLETYPE)pOMXComponent,
                                                pExynosComponent->callbackData,
                                                OMX_EventError, ret, 0, NULL);
        }
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_VP8Dec_dstOutputBufferProcess(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *pDstOutputData)
{
    OMX_ERRORTYPE             ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_VP8DEC_HANDLE    *pVp8Dec = (EXYNOS_VP8DEC_HANDLE *)((EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
    EXYNOS_OMX_BASEPORT     *pExynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];

    FunctionIn();

    if ((!CHECK_PORT_ENABLED(pExynosOutputPort)) || (!CHECK_PORT_POPULATED(pExynosOutputPort))) {
        ret = OMX_ErrorNone;
        goto EXIT;
    }
    if (OMX_FALSE == Exynos_Check_BufferProcess_State(pExynosComponent, OUTPUT_PORT_INDEX)) {
        ret = OMX_ErrorNone;
        goto EXIT;
    }

    if ((pExynosOutputPort->bufferProcessType & BUFFER_COPY) == BUFFER_COPY) {
        if ((pVp8Dec->bDestinationStart == OMX_FALSE) &&
           (!CHECK_PORT_BEING_FLUSHED(pExynosOutputPort))) {
            Exynos_OSAL_SignalWait(pVp8Dec->hDestinationStartEvent, DEF_MAX_WAIT_TIME);
            Exynos_OSAL_SignalReset(pVp8Dec->hDestinationStartEvent);
        }
    }
    ret = Exynos_VP8Dec_DstOut(pOMXComponent, pDstOutputData);
    if ((ret != OMX_ErrorNone) &&
        (pExynosComponent->currentState == OMX_StateExecuting)) {
        pExynosComponent->pCallbacks->EventHandler((OMX_HANDLETYPE)pOMXComponent,
                                                pExynosComponent->callbackData,
                                                OMX_EventError, ret, 0, NULL);
    }

EXIT:
    FunctionOut();

    return ret;
}

OSCL_EXPORT_REF OMX_ERRORTYPE Exynos_OMX_ComponentInit(
    OMX_HANDLETYPE  hComponent,
    OMX_STRING      componentName)
{
    OMX_ERRORTYPE                    ret                = OMX_ErrorNone;
    OMX_COMPONENTTYPE               *pOMXComponent      = NULL;
    EXYNOS_OMX_BASECOMPONENT        *pExynosComponent   = NULL;
    EXYNOS_OMX_BASEPORT             *pExynosPort        = NULL;
    EXYNOS_OMX_VIDEODEC_COMPONENT   *pVideoDec          = NULL;
    EXYNOS_VP8DEC_HANDLE            *pVp8Dec            = NULL;
    int i = 0;

    FunctionIn();

    if ((hComponent == NULL) || (componentName == NULL)) {
        ret = OMX_ErrorBadParameter;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorBadParameter, Line:%d", __LINE__);
        goto EXIT;
    }
    if (Exynos_OSAL_Strcmp(EXYNOS_OMX_COMPONENT_VP8_DEC, componentName) != 0) {
        ret = OMX_ErrorBadParameter;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorBadParameter, componentName:%s, Line:%d", componentName, __LINE__);
        goto EXIT;
    }

    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = Exynos_OMX_VideoDecodeComponentInit(pOMXComponent);
    if (ret != OMX_ErrorNone) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_Error, Line:%d", __LINE__);
        goto EXIT;
    }
    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    pExynosComponent->codecType = HW_VIDEO_DEC_CODEC;

    pExynosComponent->componentName = (OMX_STRING)Exynos_OSAL_Malloc(MAX_OMX_COMPONENT_NAME_SIZE);
    if (pExynosComponent->componentName == NULL) {
        Exynos_OMX_VideoDecodeComponentDeinit(pOMXComponent);
        ret = OMX_ErrorInsufficientResources;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
        goto EXIT;
    }
    Exynos_OSAL_Memset(pExynosComponent->componentName, 0, MAX_OMX_COMPONENT_NAME_SIZE);

    pVp8Dec = Exynos_OSAL_Malloc(sizeof(EXYNOS_VP8DEC_HANDLE));
    if (pVp8Dec == NULL) {
        Exynos_OMX_VideoDecodeComponentDeinit(pOMXComponent);
        ret = OMX_ErrorInsufficientResources;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
        goto EXIT;
    }
    Exynos_OSAL_Memset(pVp8Dec, 0, sizeof(EXYNOS_VP8DEC_HANDLE));
    pVideoDec = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    pVideoDec->hCodecHandle = (OMX_HANDLETYPE)pVp8Dec;

    Exynos_OSAL_Strcpy(pExynosComponent->componentName, EXYNOS_OMX_COMPONENT_VP8_DEC);

    /* Set componentVersion */
    pExynosComponent->componentVersion.s.nVersionMajor = VERSIONMAJOR_NUMBER;
    pExynosComponent->componentVersion.s.nVersionMinor = VERSIONMINOR_NUMBER;
    pExynosComponent->componentVersion.s.nRevision     = REVISION_NUMBER;
    pExynosComponent->componentVersion.s.nStep         = STEP_NUMBER;
    /* Set specVersion */
    pExynosComponent->specVersion.s.nVersionMajor = VERSIONMAJOR_NUMBER;
    pExynosComponent->specVersion.s.nVersionMinor = VERSIONMINOR_NUMBER;
    pExynosComponent->specVersion.s.nRevision     = REVISION_NUMBER;
    pExynosComponent->specVersion.s.nStep         = STEP_NUMBER;

    /* Input port */
    pExynosPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    pExynosPort->portDefinition.format.video.nFrameWidth = DEFAULT_FRAME_WIDTH;
    pExynosPort->portDefinition.format.video.nFrameHeight= DEFAULT_FRAME_HEIGHT;
    pExynosPort->portDefinition.format.video.nStride = 0; /*DEFAULT_FRAME_WIDTH;*/
    pExynosPort->portDefinition.format.video.nSliceHeight = 0;
    pExynosPort->portDefinition.nBufferSize = DEFAULT_VIDEO_INPUT_BUFFER_SIZE;
    pExynosPort->portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingVPX;
    Exynos_OSAL_Memset(pExynosPort->portDefinition.format.video.cMIMEType, 0, MAX_OMX_MIMETYPE_SIZE);
    Exynos_OSAL_Strcpy(pExynosPort->portDefinition.format.video.cMIMEType, "video/x-vnd.on2.vp8");
    pExynosPort->portDefinition.format.video.pNativeRender = 0;
    pExynosPort->portDefinition.format.video.bFlagErrorConcealment = OMX_FALSE;
    pExynosPort->portDefinition.format.video.eColorFormat = OMX_COLOR_FormatUnused;
    pExynosPort->portDefinition.bEnabled = OMX_TRUE;
    pExynosPort->bufferProcessType = BUFFER_SHARE;
    pExynosPort->portWayType = WAY2_PORT;

    /* Output port */
    pExynosPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    pExynosPort->portDefinition.format.video.nFrameWidth = DEFAULT_FRAME_WIDTH;
    pExynosPort->portDefinition.format.video.nFrameHeight= DEFAULT_FRAME_HEIGHT;
    pExynosPort->portDefinition.format.video.nStride = 0; /*DEFAULT_FRAME_WIDTH;*/
    pExynosPort->portDefinition.format.video.nSliceHeight = 0;
    pExynosPort->portDefinition.nBufferSize = DEFAULT_VIDEO_OUTPUT_BUFFER_SIZE;
    pExynosPort->portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
    Exynos_OSAL_Memset(pExynosPort->portDefinition.format.video.cMIMEType, 0, MAX_OMX_MIMETYPE_SIZE);
    Exynos_OSAL_Strcpy(pExynosPort->portDefinition.format.video.cMIMEType, "raw/video");
    pExynosPort->portDefinition.format.video.pNativeRender = 0;
    pExynosPort->portDefinition.format.video.bFlagErrorConcealment = OMX_FALSE;
    pExynosPort->portDefinition.format.video.eColorFormat = OMX_COLOR_FormatYUV420Planar;
    pExynosPort->portDefinition.bEnabled = OMX_TRUE;
    pExynosPort->bufferProcessType = BUFFER_COPY | BUFFER_ANBSHARE;
    pExynosPort->portWayType = WAY2_PORT;

    pOMXComponent->GetParameter      = &Exynos_VP8Dec_GetParameter;
    pOMXComponent->SetParameter      = &Exynos_VP8Dec_SetParameter;
    pOMXComponent->GetConfig         = &Exynos_VP8Dec_GetConfig;
    pOMXComponent->SetConfig         = &Exynos_VP8Dec_SetConfig;
    pOMXComponent->GetExtensionIndex = &Exynos_VP8Dec_GetExtensionIndex;
    pOMXComponent->ComponentRoleEnum = &Exynos_VP8Dec_ComponentRoleEnum;
    pOMXComponent->ComponentDeInit   = &Exynos_OMX_ComponentDeinit;

    pExynosComponent->exynos_codec_componentInit      = &Exynos_VP8Dec_Init;
    pExynosComponent->exynos_codec_componentTerminate = &Exynos_VP8Dec_Terminate;

    pVideoDec->exynos_codec_srcInputProcess  = &Exynos_VP8Dec_srcInputBufferProcess;
    pVideoDec->exynos_codec_srcOutputProcess = &Exynos_VP8Dec_srcOutputBufferProcess;
    pVideoDec->exynos_codec_dstInputProcess  = &Exynos_VP8Dec_dstInputBufferProcess;
    pVideoDec->exynos_codec_dstOutputProcess = &Exynos_VP8Dec_dstOutputBufferProcess;

    pVideoDec->exynos_codec_start            = &VP8CodecStart;
    pVideoDec->exynos_codec_stop             = &VP8CodecStop;
    pVideoDec->exynos_codec_bufferProcessRun = &VP8CodecOutputBufferProcessRun;
    pVideoDec->exynos_codec_enqueueAllBuffer = &VP8CodecEnQueueAllBuffer;

    pVideoDec->exynos_checkInputFrame                 = &Check_VP8_Frame;
    pVideoDec->exynos_codec_getCodecInputPrivateData  = &GetCodecInputPrivateData;
    pVideoDec->exynos_codec_getCodecOutputPrivateData = &GetCodecOutputPrivateData;
    pVideoDec->exynos_codec_reconfigAllBuffers        = &VP8CodecReconfigAllBuffers;

    pVideoDec->hSharedMemory = Exynos_OSAL_SharedMemory_Open();
    if (pVideoDec->hSharedMemory == NULL) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
        Exynos_OSAL_Free(pVp8Dec);
        pVp8Dec = ((EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle = NULL;
        Exynos_OMX_VideoDecodeComponentDeinit(pOMXComponent);
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    pExynosComponent->currentState = OMX_StateLoaded;

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_ComponentDeinit(
    OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE                ret                = OMX_ErrorNone;
    OMX_COMPONENTTYPE           *pOMXComponent      = NULL;
    EXYNOS_OMX_BASECOMPONENT    *pExynosComponent   = NULL;
    EXYNOS_OMX_VIDEODEC_COMPONENT *pVideoDec = NULL;
    EXYNOS_VP8DEC_HANDLE        *pVp8Dec            = NULL;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    pVideoDec = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;

    Exynos_OSAL_SharedMemory_Close(pVideoDec->hSharedMemory);

    Exynos_OSAL_Free(pExynosComponent->componentName);
    pExynosComponent->componentName = NULL;

    pVp8Dec = (EXYNOS_VP8DEC_HANDLE *)pVideoDec->hCodecHandle;
    if (pVp8Dec != NULL) {
        Exynos_OSAL_Free(pVp8Dec);
        pVp8Dec = pVideoDec->hCodecHandle = NULL;
    }

    ret = Exynos_OMX_VideoDecodeComponentDeinit(pOMXComponent);
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}
