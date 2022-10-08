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
 * @file        Exynos_OMX_Venc.c
 * @brief
 * @author      SeungBeom Kim (sbcrux.kim@samsung.com)
 *              Yunji Kim (yunji.kim@samsung.com)
 * @version     2.0.0
 * @history
 *   2012.02.20 : Create
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Exynos_OMX_Macros.h"
#include "Exynos_OSAL_Event.h"
#include "Exynos_OMX_Venc.h"
#include "Exynos_OMX_VencControl.h"
#include "Exynos_OMX_Basecomponent.h"
#include "Exynos_OSAL_Thread.h"
#include "Exynos_OSAL_Semaphore.h"
#include "Exynos_OSAL_SharedMemory.h"
#include "Exynos_OSAL_Mutex.h"
#include "Exynos_OSAL_ETC.h"
#include "csc.h"

#ifdef USE_STOREMETADATA
#include <system/window.h>
#include "Exynos_OSAL_Android.h"
#endif

#undef  EXYNOS_LOG_TAG
#define EXYNOS_LOG_TAG    "EXYNOS_VIDEO_ENC"
#define EXYNOS_LOG_OFF
//#define EXYNOS_TRACE_ON
#include "Exynos_OSAL_Log.h"


inline void Exynos_UpdateFrameSize(OMX_COMPONENTTYPE *pOMXComponent)
{
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_BASEPORT      *exynosInputPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT      *exynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];

    if ((exynosOutputPort->portDefinition.format.video.nFrameWidth !=
            exynosInputPort->portDefinition.format.video.nFrameWidth) ||
        (exynosOutputPort->portDefinition.format.video.nFrameHeight !=
            exynosInputPort->portDefinition.format.video.nFrameHeight)) {
        OMX_U32 width = 0, height = 0;

        exynosOutputPort->portDefinition.format.video.nFrameWidth =
            exynosInputPort->portDefinition.format.video.nFrameWidth;
        exynosOutputPort->portDefinition.format.video.nFrameHeight =
            exynosInputPort->portDefinition.format.video.nFrameHeight;
        width = exynosOutputPort->portDefinition.format.video.nStride =
            exynosInputPort->portDefinition.format.video.nStride;
        height = exynosOutputPort->portDefinition.format.video.nSliceHeight =
            exynosInputPort->portDefinition.format.video.nSliceHeight;

        if (width && height)
            exynosOutputPort->portDefinition.nBufferSize = (width * height * 3) / 2;
    }

    return;
}

void Exynos_Free_CodecBuffers(
    OMX_COMPONENTTYPE   *pOMXComponent,
    OMX_U32              nPortIndex)
{
    OMX_ERRORTYPE                    ret                = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT        *pExynosComponent   = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEOENC_COMPONENT   *pVideoEnc          = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    CODEC_ENC_BUFFER               **ppCodecBuffer      = NULL;

    OMX_U32 nBufferCnt = 0, nPlaneCnt = 0;
    int i, j;

    FunctionIn();

    if (nPortIndex == INPUT_PORT_INDEX) {
        ppCodecBuffer = &(pVideoEnc->pMFCEncInputBuffer[0]);
        nBufferCnt = MFC_INPUT_BUFFER_NUM_MAX;
        nPlaneCnt = MFC_INPUT_BUFFER_PLANE;
    } else {
        ppCodecBuffer = &(pVideoEnc->pMFCEncOutputBuffer[0]);
        nBufferCnt = MFC_OUTPUT_BUFFER_NUM_MAX;
        nPlaneCnt = MFC_OUTPUT_BUFFER_PLANE;
    }

    for (i = 0; i < nBufferCnt; i++) {
        if (ppCodecBuffer[i] != NULL) {
            for (j = 0; j < nPlaneCnt; j++) {
                if (ppCodecBuffer[i]->pVirAddr[j] != NULL)
                    Exynos_OSAL_SharedMemory_Free(pVideoEnc->hSharedMemory, ppCodecBuffer[i]->pVirAddr[j]);
            }

            Exynos_OSAL_Free(ppCodecBuffer[i]);
            ppCodecBuffer[i] = NULL;
        }
    }

    FunctionOut();
}

OMX_ERRORTYPE Exynos_Allocate_CodecBuffers(
    OMX_COMPONENTTYPE   *pOMXComponent,
    OMX_U32              nPortIndex,
    OMX_U32              nBufferCnt,
    OMX_U32              nPlaneSize[MFC_OUTPUT_BUFFER_PLANE])
{
    OMX_ERRORTYPE                    ret                = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT        *pExynosComponent   = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEOENC_COMPONENT   *pVideoEnc          = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    MEMORY_TYPE                      eMemoryType        = SYSTEM_MEMORY;
    CODEC_ENC_BUFFER               **ppCodecBuffer      = NULL;

    OMX_U32 nPlaneCnt = 0;
    int i, j;

    FunctionIn();

    if (nPortIndex == INPUT_PORT_INDEX) {
        ppCodecBuffer = &(pVideoEnc->pMFCEncInputBuffer[0]);
        nPlaneCnt = MFC_INPUT_BUFFER_PLANE;
    } else {
        ppCodecBuffer = &(pVideoEnc->pMFCEncOutputBuffer[0]);
        nPlaneCnt = MFC_OUTPUT_BUFFER_PLANE;
#ifdef USE_CSC_HW
        eMemoryType = NORMAL_MEMORY;
#endif
    }

    for (i = 0; i < nBufferCnt; i++) {
        ppCodecBuffer[i] = (CODEC_ENC_BUFFER *)Exynos_OSAL_Malloc(sizeof(CODEC_ENC_BUFFER));
        if (ppCodecBuffer[i] == NULL) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to Alloc codec buffer");
            ret = OMX_ErrorInsufficientResources;
            goto EXIT;
        }
        Exynos_OSAL_Memset(ppCodecBuffer[i], 0, sizeof(CODEC_ENC_BUFFER));

        for (j = 0; j < nPlaneCnt; j++) {
            ppCodecBuffer[i]->pVirAddr[j] =
                (void *)Exynos_OSAL_SharedMemory_Alloc(pVideoEnc->hSharedMemory, nPlaneSize[j], eMemoryType);
            if (ppCodecBuffer[i]->pVirAddr[j] == NULL) {
                Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to Alloc plane");
                ret = OMX_ErrorInsufficientResources;
                goto EXIT;
            }

            ppCodecBuffer[i]->fd[j] =
                Exynos_OSAL_SharedMemory_VirtToION(pVideoEnc->hSharedMemory, ppCodecBuffer[i]->pVirAddr[j]);
            ppCodecBuffer[i]->bufferSize[j] = nPlaneSize[j];
            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "PORT[%d]: pMFCCodecBuffer[%d]->pVirAddr[%d]: 0x%x", nPortIndex, i, j, ppCodecBuffer[i]->pVirAddr[j]);
        }

        ppCodecBuffer[i]->dataSize = 0;
    }

    return OMX_ErrorNone;

EXIT:
    Exynos_Free_CodecBuffers(pOMXComponent, nPortIndex);

    FunctionOut();

    return ret;
}

OMX_BOOL Exynos_Check_BufferProcess_State(EXYNOS_OMX_BASECOMPONENT *pExynosComponent, OMX_U32 nPortIndex)
{
    OMX_BOOL ret = OMX_FALSE;

    if ((pExynosComponent->currentState == OMX_StateExecuting) &&
        (pExynosComponent->pExynosPort[nPortIndex].portState == OMX_StateIdle) &&
        (pExynosComponent->transientState != EXYNOS_OMX_TransStateExecutingToIdle) &&
        (pExynosComponent->transientState != EXYNOS_OMX_TransStateIdleToExecuting)) {
        ret = OMX_TRUE;
    } else {
        ret = OMX_FALSE;
    }

    return ret;
}

OMX_ERRORTYPE Exynos_Input_CodecBufferToData(EXYNOS_OMX_BASECOMPONENT *pExynosComponent, OMX_PTR codecBuffer, EXYNOS_OMX_DATA *pData)
{
    OMX_ERRORTYPE                  ret = OMX_ErrorNone;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    CODEC_ENC_BUFFER *pInputCodecBuffer = (CODEC_ENC_BUFFER*)codecBuffer;

    pData->buffer.multiPlaneBuffer.dataBuffer[0] = pInputCodecBuffer->pVirAddr[0];
    pData->buffer.multiPlaneBuffer.dataBuffer[1] = pInputCodecBuffer->pVirAddr[1];
    pData->buffer.multiPlaneBuffer.fd[0] = pInputCodecBuffer->fd[0];
    pData->buffer.multiPlaneBuffer.fd[1] = pInputCodecBuffer->fd[1];
    pData->allocSize     = pInputCodecBuffer->bufferSize[0] + pInputCodecBuffer->bufferSize[1];
    pData->dataLen       = pInputCodecBuffer->dataSize;
    pData->usedDataLen   = 0;
    pData->remainDataLen = pInputCodecBuffer->dataSize;

    pData->nFlags        = 0;
    pData->timeStamp     = 0;
    pData->pPrivate      = codecBuffer;
    pData->bufferHeader  = NULL;

    return ret;
}

OMX_ERRORTYPE Exynos_Output_CodecBufferToData(EXYNOS_OMX_BASECOMPONENT *pExynosComponent, OMX_PTR codecBuffer, EXYNOS_OMX_DATA *pData)
{
    OMX_ERRORTYPE                  ret = OMX_ErrorNone;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    OMX_PTR pSrcBuf;
    OMX_U32 allocSize;

    pVideoEnc->exynos_codec_getCodecOutputPrivateData(codecBuffer, &pSrcBuf, &allocSize);
    pData->buffer.singlePlaneBuffer.dataBuffer = pSrcBuf;
    pData->allocSize     = allocSize;
    pData->dataLen       = 0;
    pData->usedDataLen   = 0;
    pData->remainDataLen = 0;

    pData->nFlags        = 0;
    pData->timeStamp     = 0;
    pData->pPrivate      = codecBuffer;
    pData->bufferHeader  = NULL;

    return ret;
}

void Exynos_Wait_ProcessPause(EXYNOS_OMX_BASECOMPONENT *pExynosComponent, OMX_U32 nPortIndex)
{
    EXYNOS_OMX_BASEPORT *exynosOMXInputPort  = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT *exynosOMXOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT *exynosOMXPort = NULL;

    FunctionIn();

    exynosOMXPort = &pExynosComponent->pExynosPort[nPortIndex];

    if (((pExynosComponent->currentState == OMX_StatePause) ||
        (pExynosComponent->currentState == OMX_StateIdle) ||
        (pExynosComponent->transientState == EXYNOS_OMX_TransStateLoadedToIdle) ||
        (pExynosComponent->transientState == EXYNOS_OMX_TransStateExecutingToIdle)) &&
        (pExynosComponent->transientState != EXYNOS_OMX_TransStateIdleToLoaded) &&
        (!CHECK_PORT_BEING_FLUSHED(exynosOMXPort))) {
        Exynos_OSAL_SignalWait(pExynosComponent->pExynosPort[nPortIndex].pauseEvent, DEF_MAX_WAIT_TIME);
        Exynos_OSAL_SignalReset(pExynosComponent->pExynosPort[nPortIndex].pauseEvent);
    }

    FunctionOut();

    return;
}

OMX_BOOL Exynos_CSC_InputData(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *srcInputData)
{
    OMX_BOOL                       ret = OMX_FALSE;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_OMX_BASEPORT   *exynosInputPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_DATABUFFER *inputUseBuffer = &exynosInputPort->way.port2WayDataBuffer.inputDataBuffer;
    OMX_U32                nFrameWidth = exynosInputPort->portDefinition.format.video.nFrameWidth;
    OMX_U32                nFrameHeight = exynosInputPort->portDefinition.format.video.nFrameHeight;
    OMX_COLOR_FORMATTYPE   eColorFormat = exynosInputPort->portDefinition.format.video.eColorFormat;
    OMX_BYTE               checkInputStream = NULL;
    OMX_BOOL               flagEOS = OMX_FALSE;

    FunctionIn();

    checkInputStream = inputUseBuffer->bufferHeader->pBuffer;

    CODEC_ENC_BUFFER *codecInputBuffer = (CODEC_ENC_BUFFER *)srcInputData->pPrivate;
    codecInputBuffer->dataSize = ((nFrameWidth * nFrameHeight) * 3) / 2;

    unsigned int csc_src_color_format = omx_2_hal_pixel_format((unsigned int)OMX_COLOR_FormatYUV420SemiPlanar);
    unsigned int csc_dst_color_format = omx_2_hal_pixel_format((unsigned int)OMX_COLOR_FormatYUV420SemiPlanar);
    CSC_METHOD csc_method = CSC_METHOD_SW;
    unsigned int cacheable = 1;

    void *pSrcBuf[3] = {NULL, };
    void *pSrcFd[3] = {NULL, };
    void *pDstBuf[3] = {NULL, };
    void *pDstFd[3] = {NULL, };

    CSC_ERRORCODE cscRet = CSC_ErrorNone;

    pSrcBuf[0]  = checkInputStream;
    pSrcBuf[1]  = checkInputStream + (nFrameWidth * nFrameHeight);
    pSrcBuf[2]  = checkInputStream + (((nFrameWidth * nFrameHeight) * 5) / 4);

    pDstBuf[0] = srcInputData->buffer.multiPlaneBuffer.dataBuffer[0];
    pDstBuf[1] = srcInputData->buffer.multiPlaneBuffer.dataBuffer[1];
    pDstBuf[2] = srcInputData->buffer.multiPlaneBuffer.dataBuffer[2];

#ifdef USE_METADATABUFFERTYPE
    OMX_PTR ppBuf[MAX_BUFFER_PLANE];

    /* kMetadataBufferTypeGrallocSource */
    if (exynosInputPort->bStoreMetaData == OMX_TRUE) {
        /* ARGB8888 converted to YUV420SemiPlanar */
        csc_src_color_format = omx_2_hal_pixel_format((unsigned int)OMX_COLOR_Format32bitARGB8888);
        csc_dst_color_format = omx_2_hal_pixel_format((unsigned int)OMX_COLOR_FormatYUV420SemiPlanar);

        Exynos_OSAL_GetInfoFromMetaData((OMX_BYTE)inputUseBuffer->bufferHeader->pBuffer, ppBuf);
        if (eColorFormat == OMX_COLOR_FormatAndroidOpaque) {
            ExynosVideoPlane planes[MAX_BUFFER_PLANE];
            OMX_U32 stride;
            int i;

            csc_src_color_format = omx_2_hal_pixel_format((unsigned int)Exynos_OSAL_GetANBColorFormat(ppBuf[0]));
            Exynos_OSAL_LockANBHandle((OMX_U32)ppBuf[0], nFrameWidth, nFrameHeight, OMX_COLOR_FormatAndroidOpaque, &stride, planes);

#if defined(USE_CSC_GSCALER) || defined(USE_CSC_G2D)
            csc_method = CSC_METHOD_HW;
#endif
            pSrcBuf[0] = planes[0].addr;
            pSrcFd[0] = (void *)planes[0].fd;
            for (i = 0; i < 3; i++)
                pDstFd[i] = (void *)srcInputData->buffer.multiPlaneBuffer.fd[i];
        }
    } else
#endif
    {
        switch (eColorFormat) {
        case OMX_COLOR_FormatYUV420Planar:
            /* YUV420Planar converted to YUV420Semiplanar (interleaved UV plane) as per MFC spec.*/
            csc_src_color_format = omx_2_hal_pixel_format((unsigned int)OMX_COLOR_FormatYUV420Planar);
            csc_dst_color_format = omx_2_hal_pixel_format((unsigned int)OMX_COLOR_FormatYUV420SemiPlanar);
            break;
        case OMX_COLOR_FormatYUV420SemiPlanar:
        case OMX_SEC_COLOR_FormatNV12Tiled:
        case OMX_SEC_COLOR_FormatNV21Linear:
            /* Just copied to MFC input buffer */
            csc_src_color_format = omx_2_hal_pixel_format((unsigned int)OMX_COLOR_FormatYUV420SemiPlanar);
            csc_dst_color_format = omx_2_hal_pixel_format((unsigned int)OMX_COLOR_FormatYUV420SemiPlanar);
            break;
        default:
            break;
        }
    }

    csc_set_method(
        pVideoEnc->csc_handle,
        csc_method);
    csc_set_src_format(
        pVideoEnc->csc_handle,  /* handle */
        nFrameWidth,                  /* width */
        nFrameHeight,                 /* height */
        0,                      /* crop_left */
        0,                      /* crop_right */
        nFrameWidth,                  /* crop_width */
        nFrameHeight,                 /* crop_height */
        csc_src_color_format,   /* color_format */
        cacheable);             /* cacheable */
    csc_set_dst_format(
        pVideoEnc->csc_handle,  /* handle */
        nFrameWidth,                  /* width */
        nFrameHeight,                 /* height */
        0,                      /* crop_left */
        0,                      /* crop_right */
        nFrameWidth,                  /* crop_width */
        nFrameHeight,                 /* crop_height */
        csc_dst_color_format,   /* color_format */
        cacheable);             /* cacheable */
    if (csc_method == CSC_METHOD_SW) {
        csc_set_src_buffer(
            pVideoEnc->csc_handle,  /* handle */
            pSrcBuf);
        csc_set_dst_buffer(
            pVideoEnc->csc_handle,  /* handle */
            pDstBuf);
    } else {
        csc_set_src_buffer(
            pVideoEnc->csc_handle,  /* handle */
            pSrcFd);
        csc_set_dst_buffer(
            pVideoEnc->csc_handle,  /* handle */
            pDstFd);
    }
    cscRet = csc_convert(pVideoEnc->csc_handle);
    if (cscRet != CSC_ErrorNone)
        ret = OMX_FALSE;
    else
        ret = OMX_TRUE;

#ifdef USE_METADATABUFFERTYPE
    if (exynosInputPort->bStoreMetaData == OMX_TRUE) {
        Exynos_OSAL_UnlockANBHandle((OMX_U32)ppBuf[0]);
    }
#endif

EXIT:
    FunctionOut();

    return ret;
}

OMX_BOOL Exynos_Preprocessor_InputData(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *srcInputData)
{
    OMX_BOOL                      ret = OMX_FALSE;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_OMX_BASEPORT   *exynosInputPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_DATABUFFER *inputUseBuffer = &exynosInputPort->way.port2WayDataBuffer.inputDataBuffer;
    OMX_U32                nFrameWidth = exynosInputPort->portDefinition.format.video.nFrameWidth;
    OMX_U32                nFrameHeight = exynosInputPort->portDefinition.format.video.nFrameHeight;
    OMX_COLOR_FORMATTYPE   eColorFormat = exynosInputPort->portDefinition.format.video.eColorFormat;
    OMX_U32                copySize = 0;
    OMX_BYTE               checkInputStream = NULL;
    OMX_U32                checkInputStreamLen = 0;
    OMX_BOOL               flagEOS = OMX_FALSE;

    FunctionIn();

    if ((exynosInputPort->bufferProcessType & BUFFER_COPY) == BUFFER_COPY) {
        if ((srcInputData->buffer.multiPlaneBuffer.dataBuffer[0] == NULL) ||
            (srcInputData->pPrivate == NULL)) {
            ret = OMX_FALSE;
            goto EXIT;
        }
    }

    if (inputUseBuffer->dataValid == OMX_TRUE) {
        if (exynosInputPort->bufferProcessType == BUFFER_SHARE) {
            Exynos_Shared_BufferToData(inputUseBuffer, srcInputData, ONE_PLANE);
#ifdef USE_METADATABUFFERTYPE
            if (exynosInputPort->bStoreMetaData == OMX_TRUE) {
                OMX_PTR ppBuf[MAX_BUFFER_PLANE];
                OMX_U32 allocSize[MAX_BUFFER_PLANE];
                int     plane = 0;

                if (inputUseBuffer->dataLen <= 0) {
                    if (!(inputUseBuffer->nFlags & OMX_BUFFERFLAG_EOS)) {
                        Exynos_InputBufferReturn(pOMXComponent, inputUseBuffer);

                        /* reset dataBuffer */
                        Exynos_ResetDataBuffer(inputUseBuffer);
                    } else {
                        /* Make EOS Buffer for MFC Processing scheme */
                        /* Use ION Allocator */
                        /*Alloc Y-Buffer */
                        allocSize[0] = ALIGN(nFrameWidth, 16) * ALIGN(nFrameHeight, 16);
                        srcInputData->buffer.multiPlaneBuffer.dataBuffer[0] = (void *)Exynos_OSAL_SharedMemory_Alloc(pVideoEnc->hSharedMemory, allocSize[0], NORMAL_MEMORY);
                        srcInputData->buffer.multiPlaneBuffer.fd[0] = Exynos_OSAL_SharedMemory_VirtToION(pVideoEnc->hSharedMemory, srcInputData->buffer.multiPlaneBuffer.dataBuffer[0]);
                        /*Alloc C-Buffer */
                        allocSize[1] = ALIGN(allocSize[0] / 2, 256);
                        srcInputData->buffer.multiPlaneBuffer.dataBuffer[1] = (void *)Exynos_OSAL_SharedMemory_Alloc(pVideoEnc->hSharedMemory, allocSize[1], NORMAL_MEMORY);
                        srcInputData->buffer.multiPlaneBuffer.fd[1] = Exynos_OSAL_SharedMemory_VirtToION(pVideoEnc->hSharedMemory, srcInputData->buffer.multiPlaneBuffer.dataBuffer[1]);
                        /* input buffers are 2 plane. */
                        srcInputData->buffer.multiPlaneBuffer.dataBuffer[2] = NULL;
                        srcInputData->buffer.multiPlaneBuffer.fd[2] = -1;
                    }
                } else {
                    Exynos_OSAL_GetInfoFromMetaData((OMX_BYTE)inputUseBuffer->bufferHeader->pBuffer, ppBuf);

                    if (eColorFormat == OMX_COLOR_FormatAndroidOpaque) {
                        ExynosVideoPlane planes[MAX_BUFFER_PLANE];
                        OMX_U32 stride;

                        Exynos_OSAL_LockANBHandle((OMX_U32)ppBuf[0], nFrameWidth, nFrameHeight, OMX_COLOR_FormatYUV420SemiPlanar, &stride, planes);

                        srcInputData->buffer.multiPlaneBuffer.fd[0] = planes[0].fd;
                        srcInputData->buffer.multiPlaneBuffer.fd[1] = planes[1].fd;
                    } else {
                        /* kMetadataBufferTypeCameraSource */
                        srcInputData->buffer.multiPlaneBuffer.fd[0] = ppBuf[0];
                        srcInputData->buffer.multiPlaneBuffer.fd[1] = ppBuf[1];
                    }
                    allocSize[0] = nFrameWidth * nFrameHeight;
                    allocSize[1] = nFrameWidth * nFrameHeight >> 1;

                    for (plane = 0; plane < MFC_INPUT_BUFFER_PLANE; plane++) {
                        srcInputData->buffer.multiPlaneBuffer.dataBuffer[plane] =
                            Exynos_OSAL_SharedMemory_IONToVirt(pVideoEnc->hSharedMemory, srcInputData->buffer.multiPlaneBuffer.fd[plane]);
                        if(srcInputData->buffer.multiPlaneBuffer.dataBuffer[plane] == NULL) {
                            srcInputData->buffer.multiPlaneBuffer.dataBuffer[plane] =
                                Exynos_OSAL_SharedMemory_Map(pVideoEnc->hSharedMemory, allocSize[plane], srcInputData->buffer.multiPlaneBuffer.fd[plane]);
                        }
                    }
                    /* input buffers are 2 plane. */
                    srcInputData->buffer.multiPlaneBuffer.dataBuffer[2] = NULL;
                    srcInputData->buffer.multiPlaneBuffer.fd[2] = -1;
                }
            }
#endif
            /* reset dataBuffer */
            Exynos_ResetDataBuffer(inputUseBuffer);
        } else if ((exynosInputPort->bufferProcessType & BUFFER_COPY) == BUFFER_COPY) {
            checkInputStream = inputUseBuffer->bufferHeader->pBuffer + inputUseBuffer->usedDataLen;
            checkInputStreamLen = inputUseBuffer->remainDataLen;

            pExynosComponent->bUseFlagEOF = OMX_TRUE;

            if (checkInputStreamLen == 0) {
                inputUseBuffer->nFlags |= OMX_BUFFERFLAG_EOS;
                flagEOS = OMX_TRUE;
            }

            copySize = checkInputStreamLen;
            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "exynos_checkInputFrame : OMX_TRUE");

            if (((srcInputData->allocSize) - (srcInputData->dataLen)) >= copySize) {
                if ((copySize > 0) || (inputUseBuffer->nFlags & OMX_BUFFERFLAG_EOS)) {
                    ret = OMX_TRUE;
                    if (copySize > 0)
                        ret = Exynos_CSC_InputData(pOMXComponent, srcInputData);
                    if (ret) {
                        inputUseBuffer->dataLen -= copySize;
                        inputUseBuffer->remainDataLen -= copySize;
                        inputUseBuffer->usedDataLen += copySize;

                        srcInputData->dataLen += copySize;
                        srcInputData->remainDataLen += copySize;

                        srcInputData->timeStamp = inputUseBuffer->timeStamp;
                        srcInputData->nFlags = inputUseBuffer->nFlags;
                        srcInputData->bufferHeader = inputUseBuffer->bufferHeader;
                    } else {
                        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Exynos_CSC_InputData() failure");
                        pExynosComponent->pCallbacks->EventHandler((OMX_HANDLETYPE)pOMXComponent,
                                pExynosComponent->callbackData, OMX_EventError,
                                OMX_ErrorUndefined, 0, NULL );
                    }
                } else {
                    ret = OMX_FALSE;
                }
            } else {
                Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "input codec buffer is smaller than decoded input data size Out Length");
                pExynosComponent->pCallbacks->EventHandler((OMX_HANDLETYPE)pOMXComponent,
                                                        pExynosComponent->callbackData,
                                                        OMX_EventError, OMX_ErrorUndefined, 0, NULL);
                ret = OMX_FALSE;
            }

            if (((exynosInputPort->bStoreMetaData == OMX_TRUE) && (eColorFormat == OMX_COLOR_FormatAndroidOpaque)) ||
                (exynosInputPort->bStoreMetaData == OMX_FALSE)) {
                Exynos_InputBufferReturn(pOMXComponent, inputUseBuffer);
            } else {
                inputUseBuffer->dataValid = OMX_TRUE;
            }
        }

        if ((srcInputData->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS) {
            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "bSaveFlagEOS : OMX_TRUE");
            pExynosComponent->bSaveFlagEOS = OMX_TRUE;
            if (srcInputData->dataLen != 0)
                pExynosComponent->bBehaviorEOS = OMX_TRUE;
        }

        if ((pExynosComponent->checkTimeStamp.needSetStartTimeStamp == OMX_TRUE) &&
            (srcInputData->nFlags & OMX_BUFFERFLAG_CODECCONFIG) != OMX_BUFFERFLAG_CODECCONFIG) {
            pExynosComponent->checkTimeStamp.needCheckStartTimeStamp = OMX_TRUE;
            pExynosComponent->checkTimeStamp.startTimeStamp = srcInputData->timeStamp;
            pExynosComponent->checkTimeStamp.nStartFlags = srcInputData->nFlags;
            pExynosComponent->checkTimeStamp.needSetStartTimeStamp = OMX_FALSE;
            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "first frame timestamp after seeking %lld us (%.2f secs)",
                            srcInputData->timeStamp, srcInputData->timeStamp / 1E6);
        }

        ret = OMX_TRUE;
    }

EXIT:

    FunctionOut();

    return ret;
}

OMX_BOOL Exynos_Postprocess_OutputData(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *dstOutputData)
{
    OMX_BOOL                  ret = OMX_FALSE;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_BASEPORT      *exynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    EXYNOS_OMX_DATABUFFER    *outputUseBuffer = &exynosOutputPort->way.port2WayDataBuffer.outputDataBuffer;
    OMX_U32                   copySize = 0;

    FunctionIn();

    if (exynosOutputPort->bufferProcessType == BUFFER_SHARE) {
        if (Exynos_Shared_DataToBuffer(dstOutputData, outputUseBuffer) == OMX_ErrorNone)
            outputUseBuffer->dataValid = OMX_TRUE;
    }

    if (outputUseBuffer->dataValid == OMX_TRUE) {
        if (pExynosComponent->checkTimeStamp.needCheckStartTimeStamp == OMX_TRUE) {
            if (pExynosComponent->checkTimeStamp.startTimeStamp == dstOutputData->timeStamp){
                pExynosComponent->checkTimeStamp.startTimeStamp = -19761123;
                pExynosComponent->checkTimeStamp.nStartFlags = 0x0;
                pExynosComponent->checkTimeStamp.needSetStartTimeStamp = OMX_FALSE;
                pExynosComponent->checkTimeStamp.needCheckStartTimeStamp = OMX_FALSE;
            } else {
                Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "garbage frame drop after flush");
                ret = OMX_TRUE;
                goto EXIT;
            }
        } else if (pExynosComponent->checkTimeStamp.needSetStartTimeStamp == OMX_TRUE) {
            ret = OMX_TRUE;
            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "input buffer has not come after flush.");
            goto EXIT;
        }

        if ((exynosOutputPort->bufferProcessType & BUFFER_COPY) == BUFFER_COPY) {
            if (dstOutputData->remainDataLen <= (outputUseBuffer->allocSize - outputUseBuffer->dataLen)) {
                copySize = dstOutputData->remainDataLen;
                if (copySize > 0)
                    Exynos_OSAL_Memcpy((outputUseBuffer->bufferHeader->pBuffer + outputUseBuffer->dataLen),
                                       (dstOutputData->buffer.singlePlaneBuffer.dataBuffer + dstOutputData->usedDataLen),
                                       copySize);
                outputUseBuffer->dataLen += copySize;
                outputUseBuffer->remainDataLen += copySize;
                outputUseBuffer->nFlags = dstOutputData->nFlags;
                outputUseBuffer->timeStamp = dstOutputData->timeStamp;

                ret = OMX_TRUE;

                if ((outputUseBuffer->remainDataLen > 0) ||
                    (outputUseBuffer->nFlags & OMX_BUFFERFLAG_EOS)) {
                    Exynos_OutputBufferReturn(pOMXComponent, outputUseBuffer);
                }
            } else {
                Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "output buffer is smaller than encoded data size Out Length");
                pExynosComponent->pCallbacks->EventHandler((OMX_HANDLETYPE)pOMXComponent,
                                                        pExynosComponent->callbackData,
                                                        OMX_EventError, OMX_ErrorUndefined, 0, NULL);
                ret = OMX_FALSE;
            }
        } else if (exynosOutputPort->bufferProcessType == BUFFER_SHARE) {
            if ((outputUseBuffer->remainDataLen > 0) ||
                ((outputUseBuffer->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS) ||
                (CHECK_PORT_BEING_FLUSHED(exynosOutputPort)))
                Exynos_OutputBufferReturn(pOMXComponent, outputUseBuffer);
        }
    } else {
        ret = OMX_FALSE;
    }

EXIT:
    FunctionOut();

    return ret;
}

#ifdef USE_METADATABUFFERTYPE
OMX_ERRORTYPE Exynos_OMX_ExtensionSetup(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_OMX_BASEPORT      *exynosInputPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_DATABUFFER    *srcInputUseBuffer = &exynosInputPort->way.port2WayDataBuffer.inputDataBuffer;
    EXYNOS_OMX_DATA          *pSrcInputData = &exynosInputPort->processData;
    OMX_COLOR_FORMATTYPE      eColorFormat = exynosInputPort->portDefinition.format.video.eColorFormat;

    int i = 0;
    OMX_PTR ppBuf[MAX_BUFFER_PLANE];

    /* kMetadataBufferTypeGrallocSource */
    if (exynosInputPort->bStoreMetaData == OMX_TRUE) {
        Exynos_OSAL_GetInfoFromMetaData((OMX_BYTE)srcInputUseBuffer->bufferHeader->pBuffer, ppBuf);
        if (eColorFormat == OMX_COLOR_FormatAndroidOpaque) {
            pVideoEnc->ANBColorFormat = Exynos_OSAL_GetANBColorFormat(ppBuf[0]);
            if ((pVideoEnc->ANBColorFormat == OMX_COLOR_FormatYUV420SemiPlanar) ||
                (pVideoEnc->ANBColorFormat == OMX_SEC_COLOR_FormatNV12Tiled)) {
                exynosInputPort->bufferProcessType = BUFFER_SHARE;
            } else {
                exynosInputPort->bufferProcessType = BUFFER_COPY;
            }
        }
    }

    if ((exynosInputPort->bufferProcessType & BUFFER_COPY) == BUFFER_COPY) {
        OMX_U32 nPlaneSize[MFC_INPUT_BUFFER_PLANE] = {0, };
        nPlaneSize[0] = ALIGN_TO_16B(exynosInputPort->portDefinition.format.video.nFrameWidth)
                        * ALIGN_TO_16B(exynosInputPort->portDefinition.format.video.nFrameHeight);
        nPlaneSize[1] = ALIGN((nPlaneSize[0] / 2), 256);

        Exynos_OSAL_SemaphoreCreate(&exynosInputPort->codecSemID);
        Exynos_OSAL_QueueCreate(&exynosInputPort->codecBufferQ, MAX_QUEUE_ELEMENTS);

        ret = Exynos_Allocate_CodecBuffers(pOMXComponent, INPUT_PORT_INDEX, MFC_INPUT_BUFFER_NUM_MAX, nPlaneSize);
        if (ret != OMX_ErrorNone)
            goto EXIT;

        for (i = 0; i < MFC_INPUT_BUFFER_NUM_MAX; i++)
            Exynos_CodecBufferEnQueue(pExynosComponent, INPUT_PORT_INDEX, pVideoEnc->pMFCEncInputBuffer[i]);
    } else if (exynosInputPort->bufferProcessType == BUFFER_SHARE) {
        /*************/
        /*    TBD    */
        /*************/
        /* Does not require any actions. */
    }

EXIT:

    return ret;
}
#endif

OMX_ERRORTYPE Exynos_OMX_SrcInputBufferProcess(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_OMX_BASEPORT      *exynosInputPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_DATABUFFER    *srcInputUseBuffer = &exynosInputPort->way.port2WayDataBuffer.inputDataBuffer;
    EXYNOS_OMX_DATA          *pSrcInputData = &exynosInputPort->processData;
    OMX_BOOL               bCheckInputData = OMX_FALSE;
    OMX_BOOL               bValidCodecData = OMX_FALSE;

    FunctionIn();

    while (!pVideoEnc->bExitBufferProcessThread) {
        Exynos_OSAL_SleepMillisec(0);
        Exynos_Wait_ProcessPause(pExynosComponent, INPUT_PORT_INDEX);

        while ((Exynos_Check_BufferProcess_State(pExynosComponent, INPUT_PORT_INDEX)) &&
               (!pVideoEnc->bExitBufferProcessThread)) {
            Exynos_OSAL_SleepMillisec(0);

            if (CHECK_PORT_BEING_FLUSHED(exynosInputPort))
                break;
            if (exynosInputPort->portState != OMX_StateIdle)
                break;

            Exynos_OSAL_MutexLock(srcInputUseBuffer->bufferMutex);
            if (pVideoEnc->bFirstInput == OMX_FALSE) {
                if ((exynosInputPort->bufferProcessType & BUFFER_COPY) == BUFFER_COPY) {
                    OMX_PTR codecBuffer;
                    if ((pSrcInputData->buffer.multiPlaneBuffer.dataBuffer[0] == NULL) || (pSrcInputData->pPrivate == NULL)) {
                        Exynos_CodecBufferDeQueue(pExynosComponent, INPUT_PORT_INDEX, &codecBuffer);
                        if (codecBuffer != NULL) {
                            Exynos_Input_CodecBufferToData(pExynosComponent, codecBuffer, pSrcInputData);
                        }
                        Exynos_OSAL_MutexUnlock(srcInputUseBuffer->bufferMutex);
                        break;
                    }
                }

                if (srcInputUseBuffer->dataValid == OMX_TRUE) {
                    bCheckInputData = Exynos_Preprocessor_InputData(pOMXComponent, pSrcInputData);
                } else {
                    bCheckInputData = OMX_FALSE;
                }
            }
            if ((bCheckInputData == OMX_FALSE) &&
                (!CHECK_PORT_BEING_FLUSHED(exynosInputPort))) {
                ret = Exynos_InputBufferGetQueue(pExynosComponent);
                if (ret != OMX_ErrorNone) {
                    Exynos_OSAL_MutexUnlock(srcInputUseBuffer->bufferMutex);
                    break;
                }
#ifdef USE_METADATABUFFERTYPE
                if ((pVideoEnc->bFirstInput == OMX_TRUE) &&
                    (!CHECK_PORT_BEING_FLUSHED(exynosInputPort))) {
                    Exynos_OMX_ExtensionSetup(hComponent);
                    pVideoEnc->bFirstInput = OMX_FALSE;
                }
#endif
                Exynos_OSAL_MutexUnlock(srcInputUseBuffer->bufferMutex);
                break;
            }

            if (CHECK_PORT_BEING_FLUSHED(exynosInputPort)) {
                Exynos_OSAL_MutexUnlock(srcInputUseBuffer->bufferMutex);
                break;
            }

            ret = pVideoEnc->exynos_codec_srcInputProcess(pOMXComponent, pSrcInputData);
            Exynos_ResetCodecData(pSrcInputData);
            Exynos_OSAL_MutexUnlock(srcInputUseBuffer->bufferMutex);
            if (ret == OMX_ErrorCodecInit)
                pVideoEnc->bExitBufferProcessThread = OMX_TRUE;
        }
    }

EXIT:

    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_SrcOutputBufferProcess(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_OMX_BASEPORT      *exynosInputPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_DATABUFFER    *srcOutputUseBuffer = &exynosInputPort->way.port2WayDataBuffer.outputDataBuffer;
    EXYNOS_OMX_DATA           srcOutputData; 

    FunctionIn();

    while (!pVideoEnc->bExitBufferProcessThread) {
        Exynos_OSAL_SleepMillisec(0);

        while (!pVideoEnc->bExitBufferProcessThread) {
            if ((exynosInputPort->bufferProcessType & BUFFER_COPY) == BUFFER_COPY) {
                if (Exynos_Check_BufferProcess_State(pExynosComponent, INPUT_PORT_INDEX) == OMX_FALSE)
                    break;
            }
            Exynos_OSAL_SleepMillisec(0);

            if (CHECK_PORT_BEING_FLUSHED(exynosInputPort))
                break;

            Exynos_OSAL_MutexLock(srcOutputUseBuffer->bufferMutex);
            ret = pVideoEnc->exynos_codec_srcOutputProcess(pOMXComponent, &srcOutputData);

            if (ret == OMX_ErrorNone) {
                if ((exynosInputPort->bufferProcessType & BUFFER_COPY) == BUFFER_COPY) {
                    OMX_PTR codecBuffer;
                    codecBuffer = srcOutputData.pPrivate;
                    if (codecBuffer != NULL)
                        Exynos_CodecBufferEnQueue(pExynosComponent, INPUT_PORT_INDEX, codecBuffer);
                }
                if (exynosInputPort->bufferProcessType == BUFFER_SHARE) {
                    Exynos_Shared_DataToBuffer(&srcOutputData, srcOutputUseBuffer);
                    Exynos_InputBufferReturn(pOMXComponent, srcOutputUseBuffer);
                }
                Exynos_ResetCodecData(&srcOutputData);
            }
            Exynos_OSAL_MutexUnlock(srcOutputUseBuffer->bufferMutex);
        }
    }

EXIT:

    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_DstInputBufferProcess(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_OMX_BASEPORT      *exynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    EXYNOS_OMX_DATABUFFER    *dstInputUseBuffer = &exynosOutputPort->way.port2WayDataBuffer.inputDataBuffer;
    EXYNOS_OMX_DATA           dstInputData;

    FunctionIn();

    while (!pVideoEnc->bExitBufferProcessThread) {
        Exynos_OSAL_SleepMillisec(0);

        while ((Exynos_Check_BufferProcess_State(pExynosComponent, OUTPUT_PORT_INDEX)) &&
               (!pVideoEnc->bExitBufferProcessThread)) {
            Exynos_OSAL_SleepMillisec(0);

            if ((CHECK_PORT_BEING_FLUSHED(exynosOutputPort)) ||
                (!CHECK_PORT_POPULATED(exynosOutputPort)))
                break;
            if (exynosOutputPort->portState != OMX_StateIdle)
                break;

            Exynos_OSAL_MutexLock(dstInputUseBuffer->bufferMutex);
            if ((exynosOutputPort->bufferProcessType & BUFFER_COPY) == BUFFER_COPY) {
                OMX_PTR codecBuffer;
                ret = Exynos_CodecBufferDeQueue(pExynosComponent, OUTPUT_PORT_INDEX, &codecBuffer);
                if (ret != OMX_ErrorNone) {
                    Exynos_OSAL_MutexUnlock(dstInputUseBuffer->bufferMutex);
                    break;
                }
                Exynos_Output_CodecBufferToData(pExynosComponent, codecBuffer, &dstInputData);
            }

            if (exynosOutputPort->bufferProcessType == BUFFER_SHARE) {
                if ((dstInputUseBuffer->dataValid != OMX_TRUE) &&
                    (!CHECK_PORT_BEING_FLUSHED(exynosOutputPort))) {
                    ret = Exynos_OutputBufferGetQueue(pExynosComponent);
                    if (ret != OMX_ErrorNone) {
                        Exynos_OSAL_MutexUnlock(dstInputUseBuffer->bufferMutex);
                        break;
                    }
                    Exynos_Shared_BufferToData(dstInputUseBuffer, &dstInputData, ONE_PLANE);
                    Exynos_ResetDataBuffer(dstInputUseBuffer);
                }
            }

            if (CHECK_PORT_BEING_FLUSHED(exynosOutputPort)) {
                Exynos_OSAL_MutexUnlock(dstInputUseBuffer->bufferMutex);
                break;
            }
            ret = pVideoEnc->exynos_codec_dstInputProcess(pOMXComponent, &dstInputData);

            Exynos_ResetCodecData(&dstInputData);
            Exynos_OSAL_MutexUnlock(dstInputUseBuffer->bufferMutex);
        }
    }

EXIT:

    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_DstOutputBufferProcess(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_OMX_BASEPORT      *exynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    EXYNOS_OMX_DATABUFFER    *dstOutputUseBuffer = &exynosOutputPort->way.port2WayDataBuffer.outputDataBuffer;
    EXYNOS_OMX_DATA          *pDstOutputData = &exynosOutputPort->processData;

    FunctionIn();

    while (!pVideoEnc->bExitBufferProcessThread) {
        Exynos_OSAL_SleepMillisec(0);
        Exynos_Wait_ProcessPause(pExynosComponent, OUTPUT_PORT_INDEX);

        while ((Exynos_Check_BufferProcess_State(pExynosComponent, OUTPUT_PORT_INDEX)) &&
               (!pVideoEnc->bExitBufferProcessThread)) {
            Exynos_OSAL_SleepMillisec(0);

            if (CHECK_PORT_BEING_FLUSHED(exynosOutputPort))
                break;

            Exynos_OSAL_MutexLock(dstOutputUseBuffer->bufferMutex);
            if ((exynosOutputPort->bufferProcessType & BUFFER_COPY) == BUFFER_COPY) {
                if ((dstOutputUseBuffer->dataValid != OMX_TRUE) &&
                    (!CHECK_PORT_BEING_FLUSHED(exynosOutputPort))) {
                    ret = Exynos_OutputBufferGetQueue(pExynosComponent);
                    if (ret != OMX_ErrorNone) {
                        Exynos_OSAL_MutexUnlock(dstOutputUseBuffer->bufferMutex);
                        break;
                    }
                }
            }

            if ((dstOutputUseBuffer->dataValid == OMX_TRUE) ||
                (exynosOutputPort->bufferProcessType == BUFFER_SHARE))
                ret = pVideoEnc->exynos_codec_dstOutputProcess(pOMXComponent, pDstOutputData);

            if (((ret == OMX_ErrorNone) && (dstOutputUseBuffer->dataValid == OMX_TRUE)) ||
                (exynosOutputPort->bufferProcessType == BUFFER_SHARE)) {
                Exynos_Postprocess_OutputData(pOMXComponent, pDstOutputData);
            }

            if ((exynosOutputPort->bufferProcessType & BUFFER_COPY) == BUFFER_COPY) {
                OMX_PTR codecBuffer;
                codecBuffer = pDstOutputData->pPrivate;
                if (codecBuffer != NULL) {
                    Exynos_CodecBufferEnQueue(pExynosComponent, OUTPUT_PORT_INDEX, codecBuffer);
                    pDstOutputData->pPrivate = NULL;
                }
            }

            /* reset outputData */
            Exynos_ResetCodecData(pDstOutputData);
            Exynos_OSAL_MutexUnlock(dstOutputUseBuffer->bufferMutex);
        }
    }

EXIT:

    FunctionOut();

    return ret;
}

static OMX_ERRORTYPE Exynos_OMX_SrcInputProcessThread(OMX_PTR threadData)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;
    EXYNOS_OMX_MESSAGE       *message = NULL;

    FunctionIn();

    if (threadData == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)threadData;
    ret = Exynos_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }
    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    Exynos_OMX_SrcInputBufferProcess(pOMXComponent);

    Exynos_OSAL_ThreadExit(NULL);

EXIT:
    FunctionOut();

    return ret;
}

static OMX_ERRORTYPE Exynos_OMX_SrcOutputProcessThread(OMX_PTR threadData)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;
    EXYNOS_OMX_MESSAGE       *message = NULL;

    FunctionIn();

    if (threadData == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)threadData;
    ret = Exynos_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }
    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    Exynos_OMX_SrcOutputBufferProcess(pOMXComponent);

    Exynos_OSAL_ThreadExit(NULL);

EXIT:
    FunctionOut();

    return ret;
}

static OMX_ERRORTYPE Exynos_OMX_DstInputProcessThread(OMX_PTR threadData)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;
    EXYNOS_OMX_MESSAGE       *message = NULL;

    FunctionIn();

    if (threadData == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)threadData;
    ret = Exynos_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }
    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    Exynos_OMX_DstInputBufferProcess(pOMXComponent);

    Exynos_OSAL_ThreadExit(NULL);

EXIT:
    FunctionOut();

    return ret;
}

static OMX_ERRORTYPE Exynos_OMX_DstOutputProcessThread(OMX_PTR threadData)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;
    EXYNOS_OMX_MESSAGE       *message = NULL;

    FunctionIn();

    if (threadData == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)threadData;
    ret = Exynos_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }
    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    Exynos_OMX_DstOutputBufferProcess(pOMXComponent);

    Exynos_OSAL_ThreadExit(NULL);

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_BufferProcess_Create(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;

    FunctionIn();

    pVideoEnc->bExitBufferProcessThread = OMX_FALSE;

    ret = Exynos_OSAL_ThreadCreate(&pVideoEnc->hDstOutputThread,
                 Exynos_OMX_DstOutputProcessThread,
                 pOMXComponent);
    if (ret == OMX_ErrorNone)
        ret = Exynos_OSAL_ThreadCreate(&pVideoEnc->hSrcOutputThread,
                     Exynos_OMX_SrcOutputProcessThread,
                     pOMXComponent);
    if (ret == OMX_ErrorNone)
        ret = Exynos_OSAL_ThreadCreate(&pVideoEnc->hDstInputThread,
                     Exynos_OMX_DstInputProcessThread,
                     pOMXComponent);
    if (ret == OMX_ErrorNone)
        ret = Exynos_OSAL_ThreadCreate(&pVideoEnc->hSrcInputThread,
                     Exynos_OMX_SrcInputProcessThread,
                     pOMXComponent);

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_BufferProcess_Terminate(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    OMX_S32                countValue = 0;
    unsigned int           i = 0;

    FunctionIn();

    pVideoEnc->bExitBufferProcessThread = OMX_TRUE;

    Exynos_OSAL_Get_SemaphoreCount(pExynosComponent->pExynosPort[INPUT_PORT_INDEX].bufferSemID, &countValue);
    if (countValue == 0)
        Exynos_OSAL_SemaphorePost(pExynosComponent->pExynosPort[INPUT_PORT_INDEX].bufferSemID);
    Exynos_OSAL_Get_SemaphoreCount(pExynosComponent->pExynosPort[INPUT_PORT_INDEX].codecSemID, &countValue);
    if (countValue == 0)
        Exynos_OSAL_SemaphorePost(pExynosComponent->pExynosPort[INPUT_PORT_INDEX].codecSemID);
    Exynos_OSAL_SignalSet(pExynosComponent->pExynosPort[INPUT_PORT_INDEX].pauseEvent);
    Exynos_OSAL_ThreadTerminate(pVideoEnc->hSrcInputThread);
    pVideoEnc->hSrcInputThread = NULL;

    Exynos_OSAL_Get_SemaphoreCount(pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX].bufferSemID, &countValue);
    if (countValue == 0)
        Exynos_OSAL_SemaphorePost(pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX].bufferSemID);
    Exynos_OSAL_Get_SemaphoreCount(pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX].codecSemID, &countValue);
    if (countValue == 0)
        Exynos_OSAL_SemaphorePost(pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX].codecSemID);
    Exynos_OSAL_SignalSet(pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX].pauseEvent);
    Exynos_OSAL_ThreadTerminate(pVideoEnc->hDstInputThread);
    pVideoEnc->hDstInputThread = NULL;

    pVideoEnc->exynos_codec_stop(pOMXComponent, INPUT_PORT_INDEX);
    pVideoEnc->exynos_codec_bufferProcessRun(pOMXComponent, INPUT_PORT_INDEX);
    Exynos_OSAL_SignalSet(pExynosComponent->pExynosPort[INPUT_PORT_INDEX].pauseEvent);
    Exynos_OSAL_ThreadTerminate(pVideoEnc->hSrcOutputThread);
    pVideoEnc->hSrcOutputThread = NULL;

    pVideoEnc->exynos_codec_stop(pOMXComponent, OUTPUT_PORT_INDEX);
    pVideoEnc->exynos_codec_bufferProcessRun(pOMXComponent, INPUT_PORT_INDEX);
    Exynos_OSAL_SignalSet(pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX].pauseEvent);
    Exynos_OSAL_ThreadTerminate(pVideoEnc->hDstOutputThread);
    pVideoEnc->hDstOutputThread = NULL;

    pExynosComponent->checkTimeStamp.needSetStartTimeStamp = OMX_FALSE;
    pExynosComponent->checkTimeStamp.needCheckStartTimeStamp = OMX_FALSE;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_VideoEncodeComponentInit(OMX_IN OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE                  ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE             *pOMXComponent = NULL;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent = NULL;
    EXYNOS_OMX_BASEPORT           *pExynosPort = NULL;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc = NULL;

    CSC_METHOD csc_method = CSC_METHOD_SW;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = Exynos_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_Error, Line:%d", __LINE__);
        goto EXIT;
    }

    ret = Exynos_OMX_BaseComponent_Constructor(pOMXComponent);
    if (ret != OMX_ErrorNone) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_Error, Line:%d", __LINE__);
        goto EXIT;
    }

    ret = Exynos_OMX_Port_Constructor(pOMXComponent);
    if (ret != OMX_ErrorNone) {
        Exynos_OMX_BaseComponent_Destructor(pOMXComponent);
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_Error, Line:%d", __LINE__);
        goto EXIT;
    }

    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;

    pVideoEnc = Exynos_OSAL_Malloc(sizeof(EXYNOS_OMX_VIDEOENC_COMPONENT));
    if (pVideoEnc == NULL) {
        Exynos_OMX_BaseComponent_Destructor(pOMXComponent);
        ret = OMX_ErrorInsufficientResources;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
        goto EXIT;
    }

    Exynos_OSAL_Memset(pVideoEnc, 0, sizeof(EXYNOS_OMX_VIDEOENC_COMPONENT));
    pExynosComponent->hComponentHandle = (OMX_HANDLETYPE)pVideoEnc;

    pExynosComponent->bSaveFlagEOS = OMX_FALSE;
    pExynosComponent->bBehaviorEOS = OMX_FALSE;

    pVideoEnc->bFirstInput  = OMX_TRUE;
    pVideoEnc->bFirstOutput = OMX_FALSE;
    pVideoEnc->configChange = OMX_FALSE;
    pVideoEnc->quantization.nQpI = 4; // I frame quantization parameter
    pVideoEnc->quantization.nQpP = 5; // P frame quantization parameter
    pVideoEnc->quantization.nQpB = 5; // B frame quantization parameter

    pVideoEnc->csc_handle = csc_init(csc_method);
    if (pVideoEnc->csc_handle == NULL) {
        Exynos_OSAL_Free(pVideoEnc);
        Exynos_OMX_BaseComponent_Destructor(pOMXComponent);
        ret = OMX_ErrorInsufficientResources;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
        goto EXIT;
    }
    pVideoEnc->csc_set_format = OMX_FALSE;
#if defined(USE_CSC_GSCALER) && defined(USE_CSC_G2D)
#error USE_CSC_GSCALER and USE_CSC_G2D are mutually exclusive
#elif defined(USE_CSC_GSCALER)
    csc_set_hw_property(pVideoEnc->csc_handle, CSC_HW_PROPERTY_FIXED_NODE, CSC_GSCALER_IDX);
    csc_set_hw_property(pVideoEnc->csc_handle, CSC_HW_PROPERTY_HW_TYPE, CSC_HW_TYPE_GSCALER);
#elif defined(USE_CSC_G2D)
    csc_set_hw_property(pVideoEnc->csc_handle, CSC_HW_PROPERTY_HW_TYPE, CSC_HW_TYPE_G2D);
#endif

    pExynosComponent->bMultiThreadProcess = OMX_TRUE;

    /* Input port */
    pExynosPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    pExynosPort->portDefinition.nBufferCountActual = MAX_VIDEO_INPUTBUFFER_NUM;
    pExynosPort->portDefinition.nBufferCountMin = MAX_VIDEO_INPUTBUFFER_NUM;
    pExynosPort->portDefinition.nBufferSize = 0;
    pExynosPort->portDefinition.eDomain = OMX_PortDomainVideo;

    pExynosPort->portDefinition.format.video.cMIMEType = Exynos_OSAL_Malloc(MAX_OMX_MIMETYPE_SIZE);
    Exynos_OSAL_Strcpy(pExynosPort->portDefinition.format.video.cMIMEType, "raw/video");
    pExynosPort->portDefinition.format.video.pNativeRender = 0;
    pExynosPort->portDefinition.format.video.bFlagErrorConcealment = OMX_FALSE;
    pExynosPort->portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;

    pExynosPort->portDefinition.format.video.nFrameWidth = 0;
    pExynosPort->portDefinition.format.video.nFrameHeight= 0;
    pExynosPort->portDefinition.format.video.nStride = 0;
    pExynosPort->portDefinition.format.video.nSliceHeight = 0;
    pExynosPort->portDefinition.format.video.nBitrate = 64000;
    pExynosPort->portDefinition.format.video.xFramerate = (15 << 16);
    pExynosPort->portDefinition.format.video.eColorFormat = OMX_COLOR_FormatUnused;
    pExynosPort->portDefinition.format.video.pNativeWindow = NULL;
    pVideoEnc->eControlRate[INPUT_PORT_INDEX] = OMX_Video_ControlRateDisable;

    pExynosPort->bStoreMetaData = OMX_FALSE;

    /* Output port */
    pExynosPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    pExynosPort->portDefinition.nBufferCountActual = MAX_VIDEO_OUTPUTBUFFER_NUM;
    pExynosPort->portDefinition.nBufferCountMin = MAX_VIDEO_OUTPUTBUFFER_NUM;
    pExynosPort->portDefinition.nBufferSize = DEFAULT_VIDEO_OUTPUT_BUFFER_SIZE;
    pExynosPort->portDefinition.eDomain = OMX_PortDomainVideo;

    pExynosPort->portDefinition.format.video.cMIMEType = Exynos_OSAL_Malloc(MAX_OMX_MIMETYPE_SIZE);
    Exynos_OSAL_Strcpy(pExynosPort->portDefinition.format.video.cMIMEType, "raw/video");
    pExynosPort->portDefinition.format.video.pNativeRender = 0;
    pExynosPort->portDefinition.format.video.bFlagErrorConcealment = OMX_FALSE;
    pExynosPort->portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;

    pExynosPort->portDefinition.format.video.nFrameWidth = 0;
    pExynosPort->portDefinition.format.video.nFrameHeight= 0;
    pExynosPort->portDefinition.format.video.nStride = 0;
    pExynosPort->portDefinition.format.video.nSliceHeight = 0;
    pExynosPort->portDefinition.format.video.nBitrate = 64000;
    pExynosPort->portDefinition.format.video.xFramerate = (15 << 16);
    pExynosPort->portDefinition.format.video.eColorFormat = OMX_COLOR_FormatUnused;
    pExynosPort->portDefinition.format.video.pNativeWindow = NULL;
    pVideoEnc->eControlRate[OUTPUT_PORT_INDEX] = OMX_Video_ControlRateDisable;

    pOMXComponent->UseBuffer              = &Exynos_OMX_UseBuffer;
    pOMXComponent->AllocateBuffer         = &Exynos_OMX_AllocateBuffer;
    pOMXComponent->FreeBuffer             = &Exynos_OMX_FreeBuffer;
    pOMXComponent->ComponentTunnelRequest = &Exynos_OMX_ComponentTunnelRequest;

    pExynosComponent->exynos_AllocateTunnelBuffer = &Exynos_OMX_AllocateTunnelBuffer;
    pExynosComponent->exynos_FreeTunnelBuffer     = &Exynos_OMX_FreeTunnelBuffer;
    pExynosComponent->exynos_BufferProcessCreate    = &Exynos_OMX_BufferProcess_Create;
    pExynosComponent->exynos_BufferProcessTerminate = &Exynos_OMX_BufferProcess_Terminate;
    pExynosComponent->exynos_BufferFlush          = &Exynos_OMX_BufferFlush;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_VideoEncodeComponentDeinit(OMX_IN OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE                  ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE             *pOMXComponent = NULL;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent = NULL;
    EXYNOS_OMX_BASEPORT           *pExynosPort = NULL;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc = NULL;
    int                            i = 0;

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

    pVideoEnc = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;

    if (pVideoEnc->csc_handle != NULL) {
        csc_deinit(pVideoEnc->csc_handle);
        pVideoEnc->csc_handle = NULL;
    }

    Exynos_OSAL_Free(pVideoEnc);
    pExynosComponent->hComponentHandle = pVideoEnc = NULL;

    for(i = 0; i < ALL_PORT_NUM; i++) {
        pExynosPort = &pExynosComponent->pExynosPort[i];
        Exynos_OSAL_Free(pExynosPort->portDefinition.format.video.cMIMEType);
        pExynosPort->portDefinition.format.video.cMIMEType = NULL;
    }

    ret = Exynos_OMX_Port_Destructor(pOMXComponent);

    ret = Exynos_OMX_BaseComponent_Destructor(hComponent);

EXIT:
    FunctionOut();

    return ret;
}
