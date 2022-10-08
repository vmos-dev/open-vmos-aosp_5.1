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
 * @file        ExynosVideoDecoder.c
 * @brief
 * @author      Jinsung Yang (jsgood.yang@samsung.com)
 * @version     1.0.0
 * @history
 *   2012.01.15: Initial Version
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <pthread.h>

#include <sys/poll.h>

#include "ion.h"
#include "ExynosVideoApi.h"
#include "ExynosVideoDec.h"
#include "OMX_Core.h"

/* #define LOG_NDEBUG 0 */
#define LOG_TAG "ExynosVideoDecoder"
#include <utils/Log.h>

#define MAX_OUTPUTBUFFER_COUNT 32

/*
 * [Common] __CodingType_To_V4L2PixelFormat
 */
static unsigned int __CodingType_To_V4L2PixelFormat(ExynosVideoCodingType codingType)
{
    unsigned int pixelformat = V4L2_PIX_FMT_H264;

    switch (codingType) {
    case VIDEO_CODING_AVC:
        pixelformat = V4L2_PIX_FMT_H264;
        break;
    case VIDEO_CODING_MPEG4:
        pixelformat = V4L2_PIX_FMT_MPEG4;
        break;
    case VIDEO_CODING_VP8:
        pixelformat = V4L2_PIX_FMT_VP8;
        break;
    case VIDEO_CODING_H263:
        pixelformat = V4L2_PIX_FMT_H263;
        break;
    case VIDEO_CODING_VC1:
        pixelformat = V4L2_PIX_FMT_VC1_ANNEX_G;
        break;
    case VIDEO_CODING_VC1_RCV:
        pixelformat = V4L2_PIX_FMT_VC1_ANNEX_L;
        break;
    case VIDEO_CODING_MPEG2:
        pixelformat = V4L2_PIX_FMT_MPEG2;
        break;
    default:
        pixelformat = V4L2_PIX_FMT_H264;
        break;
    }

    return pixelformat;
}

/*
 * [Common] __ColorFormatType_To_V4L2PixelFormat
 */
static unsigned int __ColorFormatType_To_V4L2PixelFormat(ExynosVideoColorFormatType colorFormatType)
{
    unsigned int pixelformat = V4L2_PIX_FMT_NV12M;

    switch (colorFormatType) {
    case VIDEO_COLORFORMAT_NV12_TILED:
        pixelformat = V4L2_PIX_FMT_NV12MT_16X16;
        break;
    case VIDEO_COLORFORMAT_NV21:
        pixelformat = V4L2_PIX_FMT_NV21M;
        break;
    case VIDEO_COLORFORMAT_NV12:
    default:
        pixelformat = V4L2_PIX_FMT_NV12M;
        break;
    }

    return pixelformat;
}

/*
 * [Decoder OPS] Init
 */
static void *MFC_Decoder_Init(int nMemoryType)
{
    ExynosVideoDecContext *pCtx     = NULL;
    pthread_mutex_t       *pMutex   = NULL;
    int                    needCaps = (V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_VIDEO_OUTPUT | V4L2_CAP_STREAMING);

    pCtx = (ExynosVideoDecContext *)malloc(sizeof(*pCtx));
    if (pCtx == NULL) {
        ALOGE("%s: Failed to allocate decoder context buffer", __func__);
        goto EXIT_ALLOC_FAIL;
    }

    memset(pCtx, 0, sizeof(*pCtx));

    pCtx->hDec = exynos_v4l2_open_devname(VIDEO_DECODER_NAME, O_RDWR, 0);
    if (pCtx->hDec < 0) {
        ALOGE("%s: Failed to open decoder device", __func__);
        goto EXIT_OPEN_FAIL;
    }

    if (!exynos_v4l2_querycap(pCtx->hDec, needCaps)) {
        ALOGE("%s: Failed to querycap", __func__);
        goto EXIT_QUERYCAP_FAIL;
    }

    pCtx->bStreamonInbuf = VIDEO_FALSE;
    pCtx->bStreamonOutbuf = VIDEO_FALSE;

    pCtx->nMemoryType = nMemoryType;

    pMutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    if (pMutex == NULL) {
        ALOGE("%s: Failed to allocate mutex about input buffer", __func__);
        goto EXIT_QUERYCAP_FAIL;
    }
    if (pthread_mutex_init(pMutex, NULL) != 0) {
        free(pMutex);
        goto EXIT_QUERYCAP_FAIL;
    }
    pCtx->pInMutex = (void*)pMutex;

    pMutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    if (pMutex == NULL) {
        ALOGE("%s: Failed to allocate mutex about output buffer", __func__);
        goto EXIT_QUERYCAP_FAIL;
    }
    if (pthread_mutex_init(pMutex, NULL) != 0) {
        free(pMutex);
        goto EXIT_QUERYCAP_FAIL;
    }
    pCtx->pOutMutex = (void*)pMutex;

    pCtx->hIONHandle = (void*)ion_client_create();
    pCtx->nPrivateDataShareFD = ion_alloc((ion_client)pCtx->hIONHandle,
              sizeof(PrivateDataShareBuffer) * VIDEO_BUFFER_MAX_NUM, 0, ION_HEAP_SYSTEM_MASK, ION_FLAG_CACHED);
    pCtx->nPrivateDataShareAddress =
              ion_map(pCtx->nPrivateDataShareFD, sizeof(PrivateDataShareBuffer) * VIDEO_BUFFER_MAX_NUM, 0);
    memset(pCtx->nPrivateDataShareAddress, -1, sizeof(PrivateDataShareBuffer) * VIDEO_BUFFER_MAX_NUM);

    return (void *)pCtx;

EXIT_QUERYCAP_FAIL:
    if (pCtx->pInMutex != NULL) {
        pthread_mutex_destroy(pCtx->pInMutex);
        free(pCtx->pInMutex);
    }

    if (pCtx->pOutMutex != NULL) {
        pthread_mutex_destroy(pCtx->pOutMutex);
        free(pCtx->pOutMutex);
    }

    close(pCtx->hDec);

EXIT_OPEN_FAIL:
    free(pCtx);

EXIT_ALLOC_FAIL:
    return NULL;
}

/*
 * [Decoder OPS] Finalize
 */
static ExynosVideoErrorType MFC_Decoder_Finalize(void *pHandle)
{
    ExynosVideoDecContext *pCtx         = (ExynosVideoDecContext *)pHandle;
    ExynosVideoPlane      *pVideoPlane  = NULL;
    pthread_mutex_t       *pMutex       = NULL;
    ExynosVideoErrorType   ret          = VIDEO_ERROR_NONE;
    int i, j;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    ion_unmap(pCtx->nPrivateDataShareAddress, sizeof(PrivateDataShareBuffer) * VIDEO_BUFFER_MAX_NUM);
    ion_free(pCtx->nPrivateDataShareFD);
    ion_client_destroy((ion_client)pCtx->hIONHandle);

    if (pCtx->pOutMutex != NULL) {
        pMutex = (pthread_mutex_t*)pCtx->pOutMutex;
        pthread_mutex_destroy(pMutex);
        free(pMutex);
        pCtx->pOutMutex = NULL;
    }

    if (pCtx->pInMutex != NULL) {
        pMutex = (pthread_mutex_t*)pCtx->pInMutex;
        pthread_mutex_destroy(pMutex);
        free(pMutex);
        pCtx->pInMutex = NULL;
    }

    if (pCtx->bShareInbuf == VIDEO_FALSE) {
        for (i = 0; i < pCtx->nInbufs; i++) {
            for (j = 0; j < VIDEO_DECODER_INBUF_PLANES; j++) {
                pVideoPlane = &pCtx->pInbuf[i].planes[j];
                if (pVideoPlane->addr != NULL) {
                    munmap(pVideoPlane->addr, pVideoPlane->allocSize);
                    pVideoPlane->addr = NULL;
                    pVideoPlane->allocSize = 0;
                    pVideoPlane->dataSize = 0;
                }

                pCtx->pInbuf[i].pGeometry = NULL;
                pCtx->pInbuf[i].bQueued = VIDEO_FALSE;
                pCtx->pInbuf[i].bRegistered = VIDEO_FALSE;
            }
        }
    }

    if (pCtx->bShareOutbuf == VIDEO_FALSE) {
        for (i = 0; i < pCtx->nOutbufs; i++) {
            for (j = 0; j < VIDEO_DECODER_OUTBUF_PLANES; j++) {
                pVideoPlane = &pCtx->pOutbuf[i].planes[j];
                if (pVideoPlane->addr != NULL) {
                    munmap(pVideoPlane->addr, pVideoPlane->allocSize);
                    pVideoPlane->addr = NULL;
                    pVideoPlane->allocSize = 0;
                    pVideoPlane->dataSize = 0;
                }

                pCtx->pOutbuf[i].pGeometry = NULL;
                pCtx->pOutbuf[i].bQueued = VIDEO_FALSE;
                pCtx->pOutbuf[i].bRegistered = VIDEO_FALSE;
            }
        }
    }

    if (pCtx->pInbuf != NULL)
        free(pCtx->pInbuf);

    if (pCtx->pOutbuf != NULL)
        free(pCtx->pOutbuf);

    if (pCtx->hDec > 0)
        close(pCtx->hDec);

    free(pCtx);

EXIT:
    return ret;
}

/*
 * [Decoder OPS] Set Frame Tag
 */
static ExynosVideoErrorType MFC_Decoder_Set_FrameTag(
    void *pHandle,
    int   frameTag)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (exynos_v4l2_s_ctrl(pCtx->hDec, V4L2_CID_MPEG_MFC51_VIDEO_FRAME_TAG, frameTag) != 0) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Decoder OPS] Get Frame Tag
 */
static int MFC_Decoder_Get_FrameTag(void *pHandle)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    int frameTag = -1;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        goto EXIT;
    }

    exynos_v4l2_g_ctrl(pCtx->hDec, V4L2_CID_MPEG_MFC51_VIDEO_FRAME_TAG, &frameTag);

EXIT:
    return frameTag;
}

/*
 * [Decoder OPS] Get Buffer Count
 */
static int MFC_Decoder_Get_ActualBufferCount(void *pHandle)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    int bufferCount = -1;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        goto EXIT;
    }

    exynos_v4l2_g_ctrl(pCtx->hDec, V4L2_CID_MIN_BUFFERS_FOR_CAPTURE, &bufferCount);

EXIT:
    return bufferCount;
}

/*
 * [Decoder OPS] Set Display Delay
 */
static ExynosVideoErrorType MFC_Decoder_Set_DisplayDelay(
    void *pHandle,
    int   delay)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (exynos_v4l2_s_ctrl(pCtx->hDec, V4L2_CID_MPEG_MFC51_VIDEO_DECODER_H264_DISPLAY_DELAY, delay) != 0) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Decoder OPS] Set I-Frame Decoding
 */
static ExynosVideoErrorType MFC_Decoder_Set_IFrameDecoding(
    void *pHandle)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (exynos_v4l2_s_ctrl(pCtx->hDec, V4L2_CID_MPEG_MFC51_VIDEO_I_FRAME_DECODING, 1) != 0) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Decoder OPS] Enable Packed PB
 */
static ExynosVideoErrorType MFC_Decoder_Enable_PackedPB(void *pHandle)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (exynos_v4l2_s_ctrl(pCtx->hDec, V4L2_CID_MPEG_MFC51_VIDEO_PACKED_PB, 1) != 0) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Decoder OPS] Enable Loop Filter
 */
static ExynosVideoErrorType MFC_Decoder_Enable_LoopFilter(void *pHandle)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (exynos_v4l2_s_ctrl(pCtx->hDec, V4L2_CID_MPEG_VIDEO_DECODER_MPEG4_DEBLOCK_FILTER, 1) != 0) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Decoder OPS] Enable Slice Mode
 */
static ExynosVideoErrorType MFC_Decoder_Enable_SliceMode(void *pHandle)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (exynos_v4l2_s_ctrl(pCtx->hDec, V4L2_CID_MPEG_VIDEO_DECODER_SLICE_INTERFACE, 1) != 0) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Decoder OPS] Enable SEI Parsing
 */
static ExynosVideoErrorType MFC_Decoder_Enable_SEIParsing(void *pHandle)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (exynos_v4l2_s_ctrl(pCtx->hDec, V4L2_CID_MPEG_VIDEO_H264_SEI_FRAME_PACKING, 1) != 0) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Decoder OPS] Get Frame Packing information
 */
static ExynosVideoErrorType MFC_Decoder_Get_FramePackingInfo(
    void                    *pHandle,
    ExynosVideoFramePacking *pFramePacking)
{
    ExynosVideoDecContext   *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType     ret  = VIDEO_ERROR_NONE;

    struct v4l2_ext_control  ext_ctrl[FRAME_PACK_SEI_INFO_NUM];
    struct v4l2_ext_controls ext_ctrls;

    int seiAvailable, seiInfo, seiGridPos, i;
    unsigned int seiArgmtId;


    if ((pCtx == NULL) || (pFramePacking == NULL)) {
        ALOGE("%s: Video context info or FramePacking pointer must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    memset(pFramePacking, 0, sizeof(*pFramePacking));
    memset(ext_ctrl, 0, (sizeof(struct v4l2_ext_control) * FRAME_PACK_SEI_INFO_NUM));

    ext_ctrls.ctrl_class = V4L2_CTRL_CLASS_MPEG;
    ext_ctrls.count = FRAME_PACK_SEI_INFO_NUM;
    ext_ctrls.controls = ext_ctrl;
    ext_ctrl[0].id =  V4L2_CID_MPEG_VIDEO_H264_SEI_FP_AVAIL;
    ext_ctrl[1].id =  V4L2_CID_MPEG_VIDEO_H264_SEI_FP_ARRGMENT_ID;
    ext_ctrl[2].id =  V4L2_CID_MPEG_VIDEO_H264_SEI_FP_INFO;
    ext_ctrl[3].id =  V4L2_CID_MPEG_VIDEO_H264_SEI_FP_GRID_POS;

    if (exynos_v4l2_g_ext_ctrl(pCtx->hDec, &ext_ctrls) != 0) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

    seiAvailable = ext_ctrl[0].value;
    seiArgmtId = ext_ctrl[1].value;
    seiInfo = ext_ctrl[2].value;
    seiGridPos = ext_ctrl[3].value;

    pFramePacking->available = seiAvailable;
    pFramePacking->arrangement_id = seiArgmtId;

    pFramePacking->arrangement_cancel_flag = OPERATE_BIT(seiInfo, 0x1, 0);
    pFramePacking->arrangement_type = OPERATE_BIT(seiInfo, 0x3f, 1);
    pFramePacking->quincunx_sampling_flag = OPERATE_BIT(seiInfo, 0x1, 8);
    pFramePacking->content_interpretation_type = OPERATE_BIT(seiInfo, 0x3f, 9);
    pFramePacking->spatial_flipping_flag = OPERATE_BIT(seiInfo, 0x1, 15);
    pFramePacking->frame0_flipped_flag = OPERATE_BIT(seiInfo, 0x1, 16);
    pFramePacking->field_views_flag = OPERATE_BIT(seiInfo, 0x1, 17);
    pFramePacking->current_frame_is_frame0_flag = OPERATE_BIT(seiInfo, 0x1, 18);

    pFramePacking->frame0_grid_pos_x = OPERATE_BIT(seiGridPos, 0xf, 0);
    pFramePacking->frame0_grid_pos_y = OPERATE_BIT(seiGridPos, 0xf, 4);
    pFramePacking->frame1_grid_pos_x = OPERATE_BIT(seiGridPos, 0xf, 8);
    pFramePacking->frame1_grid_pos_y = OPERATE_BIT(seiGridPos, 0xf, 12);

EXIT:
    return ret;
}

/*
 * [Decoder Buffer OPS] Enable Cacheable (Input)
 */
static ExynosVideoErrorType MFC_Decoder_Enable_Cacheable_Inbuf(void *pHandle)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (exynos_v4l2_s_ctrl(pCtx->hDec, V4L2_CID_CACHEABLE, 2) != 0) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Decoder Buffer OPS] Enable Cacheable (Output)
 */
static ExynosVideoErrorType MFC_Decoder_Enable_Cacheable_Outbuf(void *pHandle)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (exynos_v4l2_s_ctrl(pCtx->hDec, V4L2_CID_CACHEABLE, 1) != 0) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Decoder Buffer OPS] Set Shareable Buffer (Input)
 */
static ExynosVideoErrorType MFC_Decoder_Set_Shareable_Inbuf(void *pHandle)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    pCtx->bShareInbuf = VIDEO_TRUE;

EXIT:
    return ret;
}

/*
 * [Decoder Buffer OPS] Set Shareable Buffer (Output)
 */
static ExynosVideoErrorType MFC_Decoder_Set_Shareable_Outbuf(void *pHandle)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    pCtx->bShareOutbuf = VIDEO_TRUE;

EXIT:
    return ret;
}

/*
 * [Decoder Buffer OPS] Get Buffer (Input)
 */
static ExynosVideoErrorType MFC_Decoder_Get_Buffer_Inbuf(
    void               *pHandle,
    int                 nIndex,
    ExynosVideoBuffer **pBuffer)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        *pBuffer = NULL;
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (pCtx->nInbufs <= nIndex) {
        *pBuffer = NULL;
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    *pBuffer = (ExynosVideoBuffer *)&pCtx->pInbuf[nIndex];

EXIT:
    return ret;
}

/*
 * [Decoder Buffer OPS] Get Buffer (Output)
 */
static ExynosVideoErrorType MFC_Decoder_Get_Buffer_Outbuf(
    void               *pHandle,
    int                 nIndex,
    ExynosVideoBuffer **pBuffer)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        *pBuffer = NULL;
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (pCtx->nOutbufs <= nIndex) {
        *pBuffer = NULL;
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    *pBuffer = (ExynosVideoBuffer *)&pCtx->pOutbuf[nIndex];

EXIT:
    return ret;
}

/*
 * [Decoder Buffer OPS] Set Geometry (Input)
 */
static ExynosVideoErrorType MFC_Decoder_Set_Geometry_Inbuf(
    void                *pHandle,
    ExynosVideoGeometry *bufferConf)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    struct v4l2_format fmt;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (bufferConf == NULL) {
        ALOGE("%s: Buffer geometry must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    memset(&fmt, 0, sizeof(fmt));

    fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    fmt.fmt.pix_mp.pixelformat = __CodingType_To_V4L2PixelFormat(bufferConf->eCompressionFormat);
    fmt.fmt.pix_mp.plane_fmt[0].sizeimage = bufferConf->nSizeImage;

    if (exynos_v4l2_s_fmt(pCtx->hDec, &fmt) != 0) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

    memcpy(&pCtx->inbufGeometry, bufferConf, sizeof(pCtx->inbufGeometry));

EXIT:
    return ret;
}

/*
 * [Decoder Buffer OPS] Set Geometry (Output)
 */
static ExynosVideoErrorType MFC_Decoder_Set_Geometry_Outbuf(
    void                *pHandle,
    ExynosVideoGeometry *bufferConf)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    struct v4l2_format fmt;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (bufferConf == NULL) {
        ALOGE("%s: Buffer geometry must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    memset(&fmt, 0, sizeof(fmt));

    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    fmt.fmt.pix_mp.pixelformat = __ColorFormatType_To_V4L2PixelFormat(bufferConf->eColorFormat);

    if (exynos_v4l2_s_fmt(pCtx->hDec, &fmt) != 0) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

    memcpy(&pCtx->outbufGeometry, bufferConf, sizeof(pCtx->outbufGeometry));

EXIT:
    return ret;
}

/*
 * [Decoder Buffer OPS] Get Geometry (Output)
 */
static ExynosVideoErrorType MFC_Decoder_Get_Geometry_Outbuf(
    void                *pHandle,
    ExynosVideoGeometry *bufferConf)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    struct v4l2_format fmt;
    struct v4l2_crop   crop;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (bufferConf == NULL) {
        ALOGE("%s: Buffer geometry must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    memset(&fmt, 0, sizeof(fmt));
    memset(&crop, 0, sizeof(crop));

    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    if (exynos_v4l2_g_fmt(pCtx->hDec, &fmt) != 0) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

    crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    if (exynos_v4l2_g_crop(pCtx->hDec, &crop) != 0) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

    bufferConf->nFrameWidth = fmt.fmt.pix_mp.width;
    bufferConf->nFrameHeight = fmt.fmt.pix_mp.height;

    bufferConf->cropRect.nTop = crop.c.top;
    bufferConf->cropRect.nLeft = crop.c.left;
    bufferConf->cropRect.nWidth = crop.c.width;
    bufferConf->cropRect.nHeight = crop.c.height;

EXIT:
    return ret;
}

/*
 * [Decoder Buffer OPS] Setup (Input)
 */
static ExynosVideoErrorType MFC_Decoder_Setup_Inbuf(
    void         *pHandle,
    unsigned int  nBufferCount)
{
    ExynosVideoDecContext *pCtx         = (ExynosVideoDecContext *)pHandle;
    ExynosVideoPlane      *pVideoPlane  = NULL;
    ExynosVideoErrorType   ret          = VIDEO_ERROR_NONE;

    struct v4l2_requestbuffers req;
    struct v4l2_buffer         buf;
    struct v4l2_plane          planes[VIDEO_DECODER_INBUF_PLANES];
    int i;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (nBufferCount == 0) {
        ALOGE("%s: Buffer count must be greater than 0", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    ALOGV("%s: setting up inbufs (%d) shared=%s\n", __func__, nBufferCount,
          pCtx->bShareInbuf ? "true" : "false");

    memset(&req, 0, sizeof(req));

    req.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    req.count = nBufferCount;

    if (pCtx->bShareInbuf == VIDEO_TRUE)
        req.memory = pCtx->nMemoryType;
    else
        req.memory = V4L2_MEMORY_MMAP;

    if (exynos_v4l2_reqbufs(pCtx->hDec, &req) != 0) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

    if (req.count != nBufferCount) {
        ALOGE("%s: asked for %d, got %d\n", __func__, nBufferCount, req.count);
        ret = VIDEO_ERROR_NOMEM;
        goto EXIT;
    }

    pCtx->nInbufs = (int)req.count;

    pCtx->pInbuf = malloc(sizeof(*pCtx->pInbuf) * pCtx->nInbufs);
    if (pCtx->pInbuf == NULL) {
        ALOGE("Failed to allocate input buffer context");
        ret = VIDEO_ERROR_NOMEM;
        goto EXIT;
    }
    memset(pCtx->pInbuf, 0, sizeof(*pCtx->pInbuf) * pCtx->nInbufs);

    memset(&buf, 0, sizeof(buf));

    if (pCtx->bShareInbuf == VIDEO_FALSE) {
        buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.m.planes = planes;
        buf.length = VIDEO_DECODER_INBUF_PLANES;

        for (i = 0; i < pCtx->nInbufs; i++) {
            buf.index = i;
            if (exynos_v4l2_querybuf(pCtx->hDec, &buf) != 0) {
                ret = VIDEO_ERROR_APIFAIL;
                goto EXIT;
            }

            pVideoPlane = &pCtx->pInbuf[i].planes[0];

            pVideoPlane->addr = mmap(NULL,
                    buf.m.planes[0].length, PROT_READ | PROT_WRITE,
                    MAP_SHARED, pCtx->hDec, buf.m.planes[0].m.mem_offset);

            if (pVideoPlane->addr == MAP_FAILED) {
                ret = VIDEO_ERROR_MAPFAIL;
                goto EXIT;
            }

            pVideoPlane->allocSize = buf.m.planes[0].length;
            pVideoPlane->dataSize = 0;

            pCtx->pInbuf[i].pGeometry = &pCtx->inbufGeometry;
            pCtx->pInbuf[i].bQueued = VIDEO_FALSE;
            pCtx->pInbuf[i].bRegistered = VIDEO_TRUE;
        }
    }

    return ret;

EXIT:
    if ((pCtx != NULL) && (pCtx->pInbuf != NULL)) {
        if (pCtx->bShareInbuf == VIDEO_FALSE) {
            for (i = 0; i < pCtx->nInbufs; i++) {
                pVideoPlane = &pCtx->pInbuf[i].planes[0];
                if (pVideoPlane->addr == MAP_FAILED) {
                    pVideoPlane->addr = NULL;
                    break;
                }

                munmap(pVideoPlane->addr, pVideoPlane->allocSize);
            }
        }

        free(pCtx->pInbuf);
    }

    return ret;
}

/*
 * [Decoder Buffer OPS] Setup (Output)
 */
static ExynosVideoErrorType MFC_Decoder_Setup_Outbuf(
    void         *pHandle,
    unsigned int  nBufferCount)
{
    ExynosVideoDecContext *pCtx         = (ExynosVideoDecContext *)pHandle;
    ExynosVideoPlane      *pVideoPlane  = NULL;
    ExynosVideoErrorType   ret          = VIDEO_ERROR_NONE;

    struct v4l2_requestbuffers req;
    struct v4l2_buffer         buf;
    struct v4l2_plane          planes[VIDEO_DECODER_OUTBUF_PLANES];
    int i, j;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (nBufferCount == 0) {
        nBufferCount = MAX_OUTPUTBUFFER_COUNT;
        ALOGV("%s: Change buffer count %d", __func__, nBufferCount);
    }

    ALOGV("%s: setting up outbufs (%d) shared=%s\n", __func__, nBufferCount,
          pCtx->bShareOutbuf ? "true" : "false");

    memset(&req, 0, sizeof(req));

    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    req.count = nBufferCount;

    if (pCtx->bShareOutbuf == VIDEO_TRUE)
        req.memory = pCtx->nMemoryType;
    else
        req.memory = V4L2_MEMORY_MMAP;

    if (exynos_v4l2_reqbufs(pCtx->hDec, &req) != 0) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

    if (req.count != nBufferCount) {
        ALOGE("%s: asked for %d, got %d\n", __func__, nBufferCount, req.count);
        ret = VIDEO_ERROR_NOMEM;
        goto EXIT;
    }

    pCtx->nOutbufs = req.count;

    pCtx->pOutbuf = malloc(sizeof(*pCtx->pOutbuf) * pCtx->nOutbufs);
    if (pCtx->pOutbuf == NULL) {
        ALOGE("Failed to allocate output buffer context");
        ret = VIDEO_ERROR_NOMEM;
        goto EXIT;
    }
    memset(pCtx->pOutbuf, 0, sizeof(*pCtx->pOutbuf) * pCtx->nOutbufs);

    memset(&buf, 0, sizeof(buf));

    if (pCtx->bShareOutbuf == VIDEO_FALSE) {
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.m.planes = planes;
        buf.length = VIDEO_DECODER_OUTBUF_PLANES;

        for (i = 0; i < pCtx->nOutbufs; i++) {
            buf.index = i;
            if (exynos_v4l2_querybuf(pCtx->hDec, &buf) != 0) {
                ret = VIDEO_ERROR_APIFAIL;
                goto EXIT;
            }

            for (j = 0; j < VIDEO_DECODER_OUTBUF_PLANES; j++) {
                pVideoPlane = &pCtx->pOutbuf[i].planes[j];
                pVideoPlane->addr = mmap(NULL,
                        buf.m.planes[j].length, PROT_READ | PROT_WRITE,
                        MAP_SHARED, pCtx->hDec, buf.m.planes[j].m.mem_offset);

                if (pVideoPlane->addr == MAP_FAILED) {
                    ret = VIDEO_ERROR_MAPFAIL;
                    goto EXIT;
                }

                pVideoPlane->allocSize = buf.m.planes[j].length;
                pVideoPlane->dataSize = 0;
            }

            pCtx->pOutbuf[i].pGeometry = &pCtx->outbufGeometry;
            pCtx->pOutbuf[i].bQueued      = VIDEO_FALSE;
            pCtx->pOutbuf[i].bSlotUsed    = VIDEO_FALSE;
            pCtx->pOutbuf[i].nIndexUseCnt = 0;
            pCtx->pOutbuf[i].bRegistered  = VIDEO_TRUE;
        }
    }

    return ret;

EXIT:
    if ((pCtx != NULL) && (pCtx->pOutbuf != NULL)) {
        if (pCtx->bShareOutbuf == VIDEO_FALSE) {
            for (i = 0; i < pCtx->nOutbufs; i++) {
                for (j = 0; j < VIDEO_DECODER_OUTBUF_PLANES; j++) {
                    pVideoPlane = &pCtx->pOutbuf[i].planes[j];
                    if (pVideoPlane->addr == MAP_FAILED) {
                        pVideoPlane->addr = NULL;
                        break;
                    }

                    munmap(pVideoPlane->addr, pVideoPlane->allocSize);
                }
            }
        }

        free(pCtx->pOutbuf);
        pCtx->pOutbuf = NULL;
    }

    return ret;
}

/*
 * [Decoder Buffer OPS] Run (Input)
 */
static ExynosVideoErrorType MFC_Decoder_Run_Inbuf(void *pHandle)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (pCtx->bStreamonInbuf == VIDEO_FALSE) {
        if (exynos_v4l2_streamon(pCtx->hDec, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) != 0) {
            ALOGE("%s: Failed to streamon for input buffer", __func__);
            ret = VIDEO_ERROR_APIFAIL;
            goto EXIT;
        }
        pCtx->bStreamonInbuf = VIDEO_TRUE;
    }

EXIT:
    return ret;
}

/*
 * [Decoder Buffer OPS] Run (Output)
 */
static ExynosVideoErrorType MFC_Decoder_Run_Outbuf(void *pHandle)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (pCtx->bStreamonOutbuf == VIDEO_FALSE) {
        if (exynos_v4l2_streamon(pCtx->hDec, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) != 0) {
            ALOGE("%s: Failed to streamon for output buffer", __func__);
            ret = VIDEO_ERROR_APIFAIL;
            goto EXIT;
        }
        pCtx->bStreamonOutbuf = VIDEO_TRUE;
    }

EXIT:
    return ret;
}

/*
 * [Decoder Buffer OPS] Stop (Input)
 */
static ExynosVideoErrorType MFC_Decoder_Stop_Inbuf(void *pHandle)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;
    int i = 0;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (pCtx->bStreamonInbuf == VIDEO_TRUE) {
        if (exynos_v4l2_streamoff(pCtx->hDec, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) != 0) {
            ALOGE("%s: Failed to streamoff for input buffer", __func__);
            ret = VIDEO_ERROR_APIFAIL;
            goto EXIT;
        }
        pCtx->bStreamonInbuf = VIDEO_FALSE;
    }

    for (i = 0; i <  pCtx->nInbufs; i++) {
        pCtx->pInbuf[i].bQueued = VIDEO_FALSE;
    }

EXIT:
    return ret;
}

/*
 * [Decoder Buffer OPS] Stop (Output)
 */
static ExynosVideoErrorType MFC_Decoder_Stop_Outbuf(void *pHandle)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;
    int i = 0;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (pCtx->bStreamonOutbuf == VIDEO_TRUE) {
        if (exynos_v4l2_streamoff(pCtx->hDec, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) != 0) {
            ALOGE("%s: Failed to streamoff for output buffer", __func__);
            ret = VIDEO_ERROR_APIFAIL;
            goto EXIT;
        }
        pCtx->bStreamonOutbuf = VIDEO_FALSE;
    }

    for (i = 0; i < pCtx->nOutbufs; i++) {
        pCtx->pOutbuf[i].bQueued      = VIDEO_FALSE;
        pCtx->pOutbuf[i].bSlotUsed    = VIDEO_FALSE;
        pCtx->pOutbuf[i].nIndexUseCnt = 0;
    }

EXIT:
    return ret;
}

/*
 * [Decoder Buffer OPS] Wait (Input)
 */
static ExynosVideoErrorType MFC_Decoder_Wait_Inbuf(void *pHandle)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    struct pollfd poll_events;
    int poll_state;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    poll_events.fd = pCtx->hDec;
    poll_events.events = POLLOUT | POLLERR;
    poll_events.revents = 0;

    do {
        poll_state = poll((struct pollfd*)&poll_events, 1, VIDEO_DECODER_POLL_TIMEOUT);
        if (poll_state > 0) {
            if (poll_events.revents & POLLOUT) {
                break;
            } else {
                ALOGE("%s: Poll return error", __func__);
                ret = VIDEO_ERROR_POLL;
                break;
            }
        } else if (poll_state < 0) {
            ALOGE("%s: Poll state error", __func__);
            ret = VIDEO_ERROR_POLL;
            break;
        }
    } while (poll_state == 0);

EXIT:
    return ret;
}

static ExynosVideoErrorType MFC_Decoder_Register_Inbuf(
    void             *pHandle,
    ExynosVideoPlane *planes,
    int               nPlanes)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;
    int nIndex, plane;

    if ((pCtx == NULL) || (planes == NULL) || (nPlanes != VIDEO_DECODER_INBUF_PLANES)) {
        ALOGE("%s: params must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    for (nIndex = 0; nIndex < pCtx->nInbufs; nIndex++) {
        if (pCtx->pInbuf[nIndex].bRegistered == VIDEO_FALSE) {
            for (plane = 0; plane < nPlanes; plane++) {
                pCtx->pInbuf[nIndex].planes[plane].addr = planes[plane].addr;
                pCtx->pInbuf[nIndex].planes[plane].allocSize = planes[plane].allocSize;
                pCtx->pInbuf[nIndex].planes[plane].fd = planes[plane].fd;
                ALOGV("%s: registered buf %d (addr=%p alloc_sz=%ld fd=%d)\n", __func__, nIndex,
                  planes[plane].addr, planes[plane].allocSize, planes[plane].fd);
            }
            pCtx->pInbuf[nIndex].bRegistered = VIDEO_TRUE;
            break;
        }
    }

    if (nIndex == pCtx->nInbufs) {
        ALOGE("%s: can not find non-registered input buffer", __func__);
        ret = VIDEO_ERROR_NOBUFFERS;
    }

EXIT:
    return ret;
}

static ExynosVideoErrorType MFC_Decoder_Register_Outbuf(
    void             *pHandle,
    ExynosVideoPlane *planes,
    int               nPlanes)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;
    int nIndex, plane;

    if ((pCtx == NULL) || (planes == NULL) || (nPlanes != VIDEO_DECODER_OUTBUF_PLANES)) {
        ALOGE("%s: params must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    for (nIndex = 0; nIndex < pCtx->nOutbufs; nIndex++) {
        if (pCtx->pOutbuf[nIndex].bRegistered == VIDEO_FALSE) {
            for (plane = 0; plane < nPlanes; plane++) {
                pCtx->pOutbuf[nIndex].planes[plane].addr = planes[plane].addr;
                pCtx->pOutbuf[nIndex].planes[plane].allocSize = planes[plane].allocSize;
                pCtx->pOutbuf[nIndex].planes[plane].fd = planes[plane].fd;
            }
            pCtx->pOutbuf[nIndex].bRegistered = VIDEO_TRUE;
            ALOGV("%s: registered buf %d 0:(addr=%p alloc_sz=%d fd=%d) 1:(addr=%p alloc_sz=%d fd=%d)\n",
                  __func__, nIndex, planes[0].addr, planes[0].allocSize, planes[0].fd,
                  planes[1].addr, planes[1].allocSize, planes[1].fd);
            break;
        }
    }

    if (nIndex == pCtx->nOutbufs) {
        ALOGE("%s: can not find non-registered output buffer", __func__);
        ret = VIDEO_ERROR_NOBUFFERS;
    }

EXIT:
    return ret;
}

static ExynosVideoErrorType MFC_Decoder_Clear_RegisteredBuffer_Inbuf(void *pHandle)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;
    int nIndex;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    for (nIndex = 0; nIndex < pCtx->nInbufs; nIndex++) {
        pCtx->pInbuf[nIndex].planes[0].addr = NULL;
        pCtx->pInbuf[nIndex].bRegistered = VIDEO_FALSE;
    }

EXIT:
    return ret;
}

static ExynosVideoErrorType MFC_Decoder_Clear_RegisteredBuffer_Outbuf(void *pHandle)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;
    int nIndex;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    for (nIndex = 0; nIndex < pCtx->nOutbufs; nIndex++) {
        pCtx->pOutbuf[nIndex].planes[0].addr = NULL;
        pCtx->pOutbuf[nIndex].bRegistered = VIDEO_FALSE;
    }

EXIT:
    return ret;
}

/*
 * [Decoder Buffer OPS] Find (Input)
 */
static int MFC_Decoder_Find_Inbuf(
    void          *pHandle,
    unsigned char *pBuffer)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    int nIndex = -1;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        goto EXIT;
    }

    for (nIndex = 0; nIndex < pCtx->nInbufs; nIndex++) {
        if (pCtx->pInbuf[nIndex].bQueued == VIDEO_FALSE) {
            if ((pBuffer == NULL) ||
                (pCtx->pInbuf[nIndex].planes[0].addr == pBuffer))
                break;
        }
    }

    if (nIndex == pCtx->nInbufs)
        nIndex = -1;

EXIT:
    return nIndex;
}

/*
 * [Decoder Buffer OPS] Find (Outnput)
 */
static int MFC_Decoder_Find_Outbuf(
    void          *pHandle,
    unsigned char *pBuffer)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    int nIndex = -1;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        goto EXIT;
    }

    for (nIndex = 0; nIndex < pCtx->nOutbufs; nIndex++) {
        if (pCtx->pOutbuf[nIndex].bQueued == VIDEO_FALSE) {
            if ((pBuffer == NULL) ||
                (pCtx->pOutbuf[nIndex].planes[0].addr == pBuffer))
                break;
        }
    }

    if (nIndex == pCtx->nOutbufs)
        nIndex = -1;

EXIT:
    return nIndex;
}

/*
 * [Decoder Buffer OPS] Enqueue (Input)
 */
static ExynosVideoErrorType MFC_Decoder_Enqueue_Inbuf(
    void          *pHandle,
    unsigned char *pBuffer[],
    unsigned int   dataSize[],
    int            nPlanes,
    void          *pPrivate)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;
    pthread_mutex_t       *pMutex = NULL;

    struct v4l2_plane  planes[VIDEO_DECODER_INBUF_PLANES];
    struct v4l2_buffer buf;
    int index, i;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (VIDEO_DECODER_INBUF_PLANES < nPlanes) {
        ALOGE("%s: Number of max planes : %d, nPlanes : %d", __func__,
                                    VIDEO_DECODER_INBUF_PLANES, nPlanes);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    memset(&buf, 0, sizeof(buf));

    buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    buf.m.planes = planes;
    buf.length = VIDEO_DECODER_INBUF_PLANES;

    pMutex = (pthread_mutex_t*)pCtx->pInMutex;
    pthread_mutex_lock(pMutex);
    index = MFC_Decoder_Find_Inbuf(pCtx, pBuffer[0]);
    if (index == -1) {
        pthread_mutex_unlock(pMutex);
        ALOGE("%s: Failed to get index", __func__);
        ret = VIDEO_ERROR_NOBUFFERS;
        goto EXIT;
    }

    buf.index = index;
    pCtx->pInbuf[buf.index].bQueued = VIDEO_TRUE;

    if (pCtx->bShareInbuf == VIDEO_TRUE) {
        buf.memory = pCtx->nMemoryType;
        for (i = 0; i < nPlanes; i++) {
            /* V4L2_MEMORY_USERPTR */
            buf.m.planes[i].m.userptr = (unsigned long)pBuffer[i];
            /* V4L2_MEMORY_DMABUF */
            buf.m.planes[i].m.fd = pCtx->pInbuf[index].planes[i].fd;
            buf.m.planes[i].length = pCtx->pInbuf[index].planes[i].allocSize;
            buf.m.planes[i].bytesused = dataSize[i];
            ALOGV("%s: shared inbuf(%d) plane(%d) addr=%p fd=%d len=%d used=%d\n", __func__,
                  index, i,
                  buf.m.planes[i].m.userptr,
                  buf.m.planes[i].m.fd,
                  buf.m.planes[i].length,
                  buf.m.planes[i].bytesused);
        }
    } else {
        buf.memory = V4L2_MEMORY_MMAP;
        for (i = 0; i < nPlanes; i++)
            buf.m.planes[i].bytesused = dataSize[i];
    }

    // FIXME: figure out why |pPrivate| may be NULL.
    if (pPrivate &&
            (((OMX_BUFFERHEADERTYPE *)pPrivate)->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS) {
        buf.flags |= V4L2_BUF_FLAG_LAST_FRAME;
        ALOGV("%s: OMX_BUFFERFLAG_EOS => LAST_FRAME: 0x%x", __func__,
              !!(buf.flags & V4L2_BUF_FLAG_LAST_FRAME));
    }

    pCtx->pInbuf[buf.index].pPrivate = pPrivate;

    pthread_mutex_unlock(pMutex);

    if (exynos_v4l2_qbuf(pCtx->hDec, &buf) != 0) {
        ALOGE("%s: Failed to enqueue input buffer", __func__);
        pthread_mutex_lock(pMutex);
        pCtx->pInbuf[buf.index].pPrivate = NULL;
        pCtx->pInbuf[buf.index].bQueued  = VIDEO_FALSE;
        pthread_mutex_unlock(pMutex);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Decoder Buffer OPS] Enqueue (Output)
 */
static ExynosVideoErrorType MFC_Decoder_Enqueue_Outbuf(
    void          *pHandle,
    unsigned char *pBuffer[],
    unsigned int   dataSize[],
    int            nPlanes,
    void          *pPrivate)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;
    pthread_mutex_t       *pMutex = NULL;

    struct v4l2_plane  planes[VIDEO_DECODER_OUTBUF_PLANES];
    struct v4l2_buffer buf;
    int i, index;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (VIDEO_DECODER_OUTBUF_PLANES < nPlanes) {
        ALOGE("%s: Number of max planes : %d, nPlanes : %d", __func__,
                                    VIDEO_DECODER_OUTBUF_PLANES, nPlanes);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    memset(&buf, 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    buf.m.planes = planes;
    buf.length = VIDEO_DECODER_OUTBUF_PLANES;

    pMutex = (pthread_mutex_t*)pCtx->pOutMutex;
    pthread_mutex_lock(pMutex);
    index = MFC_Decoder_Find_Outbuf(pCtx, pBuffer[0]);
    if (index == -1) {
        pthread_mutex_unlock(pMutex);
        ALOGE("%s: Failed to get index", __func__);
        ret = VIDEO_ERROR_NOBUFFERS;
        goto EXIT;
    }
    buf.index = index;
    pCtx->pOutbuf[buf.index].bQueued = VIDEO_TRUE;

    if (pCtx->bShareOutbuf == VIDEO_TRUE) {
        buf.memory = pCtx->nMemoryType;
        for (i = 0; i < nPlanes; i++) {
            /* V4L2_MEMORY_USERPTR */
            buf.m.planes[i].m.userptr = (unsigned long)pBuffer[i];
            /* V4L2_MEMORY_DMABUF */
            buf.m.planes[i].m.fd = pCtx->pOutbuf[index].planes[i].fd;
            buf.m.planes[i].length = pCtx->pOutbuf[index].planes[i].allocSize;
            buf.m.planes[i].bytesused = dataSize[i];
            ALOGV("%s: shared outbuf(%d) plane=%d addr=%p fd=%d len=%d used=%d\n", __func__,
                  index, i,
                  buf.m.planes[i].m.userptr,
                  buf.m.planes[i].m.fd,
                  buf.m.planes[i].length,
                  buf.m.planes[i].bytesused);
        }
    } else {
        ALOGV("%s: non-shared outbuf(%d)\n", __func__, index);
        buf.memory = V4L2_MEMORY_MMAP;
    }

    pCtx->pOutbuf[buf.index].pPrivate = pPrivate;

    pthread_mutex_unlock(pMutex);

    if (exynos_v4l2_qbuf(pCtx->hDec, &buf) != 0) {
        ALOGE("%s: Failed to enqueue output buffer", __func__);
        pthread_mutex_lock(pMutex);
        pCtx->pOutbuf[buf.index].pPrivate = NULL;
        pCtx->pOutbuf[buf.index].bQueued  = VIDEO_FALSE;
        pthread_mutex_unlock(pMutex);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Decoder Buffer OPS] Dequeue (Input)
 */
static ExynosVideoBuffer *MFC_Decoder_Dequeue_Inbuf(void *pHandle)
{
    ExynosVideoDecContext *pCtx     = (ExynosVideoDecContext *)pHandle;
    ExynosVideoBuffer     *pInbuf   = NULL;
    pthread_mutex_t       *pMutex = NULL;

    struct v4l2_buffer buf;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        goto EXIT;
    }

    if (pCtx->bStreamonInbuf == VIDEO_FALSE) {
        pInbuf = NULL;
        goto EXIT;
    }

    memset(&buf, 0, sizeof(buf));

    buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;

    if (pCtx->bShareInbuf == VIDEO_TRUE)
        buf.memory = pCtx->nMemoryType;
    else
        buf.memory = V4L2_MEMORY_MMAP;

    if (exynos_v4l2_dqbuf(pCtx->hDec, &buf) != 0) {
        pInbuf = NULL;
        goto EXIT;
    }

    pMutex = (pthread_mutex_t*)pCtx->pInMutex;
    pthread_mutex_lock(pMutex);

    pInbuf = &pCtx->pInbuf[buf.index];
    pCtx->pInbuf[buf.index].bQueued = VIDEO_FALSE;

    if (pCtx->bStreamonInbuf == VIDEO_FALSE)
        pInbuf = NULL;

    pthread_mutex_unlock(pMutex);

EXIT:
    return pInbuf;
}

/*
 * [Decoder Buffer OPS] Dequeue (Output)
 */
static ExynosVideoBuffer *MFC_Decoder_Dequeue_Outbuf(void *pHandle)
{
    ExynosVideoDecContext *pCtx    = (ExynosVideoDecContext *)pHandle;
    ExynosVideoBuffer     *pOutbuf = NULL;
    pthread_mutex_t       *pMutex = NULL;

    struct v4l2_buffer buf;
    int value, state;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        goto EXIT;
    }

    if (pCtx->bStreamonOutbuf == VIDEO_FALSE) {
        pOutbuf = NULL;
        goto EXIT;
    }

    memset(&buf, 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;

    if (pCtx->bShareOutbuf == VIDEO_TRUE)
        buf.memory = pCtx->nMemoryType;
    else
        buf.memory = V4L2_MEMORY_MMAP;

    /* HACK: pOutbuf return -1 means DECODING_ONLY for almost cases */
    if (exynos_v4l2_dqbuf(pCtx->hDec, &buf) != 0) {
        pOutbuf = NULL;
        goto EXIT;
    }

    if (pCtx->bStreamonOutbuf == VIDEO_FALSE) {
        pOutbuf = NULL;
        goto EXIT;
    }

    pMutex = (pthread_mutex_t*)pCtx->pOutMutex;
    pthread_mutex_lock(pMutex);

    pOutbuf = &pCtx->pOutbuf[buf.index];

    exynos_v4l2_g_ctrl(pCtx->hDec, V4L2_CID_MPEG_MFC51_VIDEO_DISPLAY_STATUS, &value);

    switch (value) {
    case 0:
        pOutbuf->displayStatus = VIDEO_FRAME_STATUS_DECODING_ONLY;
        break;
    case 1:
        pOutbuf->displayStatus = VIDEO_FRAME_STATUS_DISPLAY_DECODING;
        break;
    case 2:
        pOutbuf->displayStatus = VIDEO_FRAME_STATUS_DISPLAY_ONLY;
        break;
    case 3:
        exynos_v4l2_g_ctrl(pCtx->hDec, V4L2_CID_MPEG_MFC51_VIDEO_CHECK_STATE, &state);
        if (state == 1) /* Resolution is changed */
            pOutbuf->displayStatus = VIDEO_FRAME_STATUS_CHANGE_RESOL;
        else            /* Decoding is finished */
            pOutbuf->displayStatus = VIDEO_FRAME_STATUS_DECODING_FINISHED;
        break;
    default:
        pOutbuf->displayStatus = VIDEO_FRAME_STATUS_UNKNOWN;
        break;
    }

    switch (buf.flags & (0x7 << 3)) {
    case V4L2_BUF_FLAG_KEYFRAME:
        pOutbuf->frameType = VIDEO_FRAME_I;
        break;
    case V4L2_BUF_FLAG_PFRAME:
        pOutbuf->frameType = VIDEO_FRAME_P;
        break;
    case V4L2_BUF_FLAG_BFRAME:
        pOutbuf->frameType = VIDEO_FRAME_B;
        break;
    default:
        pOutbuf->frameType = VIDEO_FRAME_OTHERS;
        break;
    };

    pOutbuf->bQueued = VIDEO_FALSE;

    pthread_mutex_unlock(pMutex);

EXIT:
    return pOutbuf;
}

static ExynosVideoErrorType MFC_Decoder_Clear_Queued_Inbuf(void *pHandle)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;
    int i;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    for (i = 0; i < pCtx->nInbufs; i++) {
        pCtx->pInbuf[i].bQueued = VIDEO_FALSE;
    }

EXIT:
    return ret;
}

static ExynosVideoErrorType MFC_Decoder_Clear_Queued_Outbuf(void *pHandle)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;
    int i;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    for (i = 0; i < pCtx->nOutbufs; i++) {
        pCtx->pOutbuf[i].bQueued = VIDEO_FALSE;
    }

EXIT:
    return ret;
}

/*
 * [Decoder Buffer OPS] Cleanup Buffer (Input)
 */
static ExynosVideoErrorType MFC_Decoder_Cleanup_Buffer_Inbuf(void *pHandle)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    struct v4l2_requestbuffers req;
    int nBufferCount = 0;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    nBufferCount = 0; /* for clean-up */

    memset(&req, 0, sizeof(req));

    req.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    req.count = nBufferCount;

    if (pCtx->bShareInbuf == VIDEO_TRUE)
        req.memory = pCtx->nMemoryType;
    else
        req.memory = V4L2_MEMORY_MMAP;

    if (exynos_v4l2_reqbufs(pCtx->hDec, &req) != 0) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

    pCtx->nInbufs = (int)req.count;

    if (pCtx->pInbuf != NULL) {
        free(pCtx->pInbuf);
        pCtx->pInbuf = NULL;
    }

EXIT:
    return ret;
}

/*
 * [Decoder Buffer OPS] Cleanup Buffer (Output)
 */
static ExynosVideoErrorType MFC_Decoder_Cleanup_Buffer_Outbuf(void *pHandle)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    struct v4l2_requestbuffers req;
    int nBufferCount = 0;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    nBufferCount = 0; /* for clean-up */

    memset(&req, 0, sizeof(req));

    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    req.count = nBufferCount;

    if (pCtx->bShareOutbuf == VIDEO_TRUE)
        req.memory = pCtx->nMemoryType;
    else
        req.memory = V4L2_MEMORY_MMAP;

    if (exynos_v4l2_reqbufs(pCtx->hDec, &req) != 0) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

    pCtx->nOutbufs = (int)req.count;

    if (pCtx->pOutbuf != NULL) {
        free(pCtx->pOutbuf);
        pCtx->pOutbuf = NULL;
    }

EXIT:
    return ret;
}

/*
 * [Decoder Buffer OPS] FindIndex (Output)
 */
static int MFC_Decoder_FindEmpty_Outbuf(void *pHandle)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    int nIndex = -1;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        goto EXIT;
    }

    for (nIndex = 0; nIndex < pCtx->nOutbufs; nIndex++) {
        if ((pCtx->pOutbuf[nIndex].bQueued == VIDEO_FALSE) &&
            (pCtx->pOutbuf[nIndex].bSlotUsed == VIDEO_FALSE))
            break;
    }

    if (nIndex == pCtx->nOutbufs)
        nIndex = -1;

EXIT:
    return nIndex;
}

/*
 * [Decoder Buffer OPS] BufferIndexFree (Output)
 */
void MFC_Decoder_BufferIndexFree_Outbuf(void *pHandle, PrivateDataShareBuffer *pPDSB, int index)
{
    ExynosVideoDecContext *pCtx    = (ExynosVideoDecContext *)pHandle;
    int i, j;

    ALOGV("De-queue buf.index:%d", index);
    ALOGV("pOutbuf fd:%d", pCtx->pOutbuf[index].planes[0].fd);

    if (pCtx->pOutbuf[index].nIndexUseCnt == 0) {
        pCtx->pOutbuf[index].bSlotUsed = VIDEO_FALSE;
    }

    for (i = 0; pPDSB->dpbFD[i].fd > -1; i++) {
        ALOGV("pPDSB->dpbFD[%d].fd:%d", i, pPDSB->dpbFD[i].fd);
        for (j = 0; pCtx->nOutbufs > j; j++)
            if (pPDSB->dpbFD[i].fd == pCtx->pOutbuf[j].planes[0].fd) {
                if (pCtx->pOutbuf[j].bQueued == VIDEO_FALSE) {
                    if (pCtx->pOutbuf[j].nIndexUseCnt > 0)
                        pCtx->pOutbuf[j].nIndexUseCnt--;
                } else if(pCtx->pOutbuf[j].bQueued == VIDEO_TRUE) {
                    if (pCtx->pOutbuf[j].nIndexUseCnt > 1) {
                        /* The buffer being used as the reference buffer came again. */
                        pCtx->pOutbuf[j].nIndexUseCnt--;
                    } else {
                        /* Reference DPB buffer is internally reused. */
                    }
                }
                ALOGV("dec FD:%d, pCtx->pOutbuf[%d].nIndexUseCnt:%d", pPDSB->dpbFD[i].fd, j, pCtx->pOutbuf[j].nIndexUseCnt);
                if ((pCtx->pOutbuf[j].nIndexUseCnt == 0) &&
                    (pCtx->pOutbuf[j].bQueued == VIDEO_FALSE)) {
                    pCtx->pOutbuf[j].bSlotUsed = VIDEO_FALSE;
                }
            }
    }
    memset((char *)pPDSB, -1, sizeof(PrivateDataShareBuffer));

    return;
}

/*
 * [Decoder Buffer OPS] ExtensionEnqueue (Output)
 */
static ExynosVideoErrorType MFC_Decoder_ExtensionEnqueue_Outbuf(
    void          *pHandle,
    unsigned char *pBuffer[],
    unsigned int  *pFd[],
    unsigned int   allocLen[],
    unsigned int   dataSize[],
    int            nPlanes,
    void          *pPrivate)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;
    pthread_mutex_t       *pMutex = NULL;

    struct v4l2_plane  planes[VIDEO_DECODER_OUTBUF_PLANES];
    struct v4l2_buffer buf;
    int state, index, i;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (VIDEO_DECODER_OUTBUF_PLANES < nPlanes) {
        ALOGE("%s: Number of max planes : %d, nPlanes : %d", __func__,
                                    VIDEO_DECODER_OUTBUF_PLANES, nPlanes);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    memset(&buf, 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    buf.m.planes = planes;
    buf.length = VIDEO_DECODER_OUTBUF_PLANES;

    pMutex = (pthread_mutex_t*)pCtx->pOutMutex;
    pthread_mutex_lock(pMutex);

    index = MFC_Decoder_Find_Outbuf(pCtx, pBuffer[0]);
    if (index == -1) {
        ALOGV("%s: Failed to find index", __func__);
        index = MFC_Decoder_FindEmpty_Outbuf(pCtx);
        if (index == -1) {
            pthread_mutex_unlock(pMutex);
            ALOGE("%s: Failed to get index", __func__);
            ret = VIDEO_ERROR_NOBUFFERS;
            goto EXIT;
        }
    }

    buf.index = index;
    ALOGV("En-queue index:%d pCtx->pOutbuf[buf.index].bQueued:%d, pFd[0]:%d",
           index, pCtx->pOutbuf[buf.index].bQueued, pFd[0]);
    pCtx->pOutbuf[buf.index].bQueued = VIDEO_TRUE;
    pCtx->pOutbuf[buf.index].bSlotUsed = VIDEO_TRUE;

    buf.memory = pCtx->nMemoryType;
    for (i = 0; i < nPlanes; i++) {
        /* V4L2_MEMORY_USERPTR */
        buf.m.planes[i].m.userptr = (unsigned long)pBuffer[i];
        /* V4L2_MEMORY_DMABUF */
        buf.m.planes[i].m.fd = pFd[i];
        buf.m.planes[i].length = allocLen[i];
        buf.m.planes[i].bytesused = dataSize[i];

        /* Temporary storage for Dequeue */
        pCtx->pOutbuf[buf.index].planes[i].addr = (unsigned long)pBuffer[i];
        pCtx->pOutbuf[buf.index].planes[i].fd = (unsigned int)pFd[i];
        pCtx->pOutbuf[buf.index].planes[i].allocSize = allocLen[i];

        ALOGV("%s: shared outbuf(%d) plane=%d addr=0x%x fd=%d len=%d used=%d\n",
              __func__, index, i,
              (void*)buf.m.planes[i].m.userptr, buf.m.planes[i].m.fd,
              buf.m.planes[i].length, buf.m.planes[i].bytesused);
    }

    pCtx->pOutbuf[buf.index].pPrivate = pPrivate;
    pCtx->pOutbuf[buf.index].nIndexUseCnt++;

    pthread_mutex_unlock(pMutex);

    if (exynos_v4l2_qbuf(pCtx->hDec, &buf) != 0) {
        pthread_mutex_lock(pMutex);
        pCtx->pOutbuf[buf.index].nIndexUseCnt--;
        pCtx->pOutbuf[buf.index].pPrivate = NULL;
        pCtx->pOutbuf[buf.index].bQueued  = VIDEO_FALSE;
        if (pCtx->pOutbuf[buf.index].nIndexUseCnt == 0)
            pCtx->pOutbuf[buf.index].bSlotUsed = VIDEO_FALSE;
        exynos_v4l2_g_ctrl(pCtx->hDec, V4L2_CID_MPEG_MFC51_VIDEO_CHECK_STATE, &state);
        pthread_mutex_unlock(pMutex);

        if (state == 1) {
            /* The case of Resolution is changed */
            ret = VIDEO_ERROR_WRONGBUFFERSIZE;
        } else {
            ALOGE("%s: Failed to enqueue output buffer", __func__);
            ret = VIDEO_ERROR_APIFAIL;
        }
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Decoder Buffer OPS] ExtensionDequeue (Output)
 */
static ExynosVideoErrorType MFC_Decoder_ExtensionDequeue_Outbuf(
    void              *pHandle,
    ExynosVideoBuffer *pVideoBuffer)
{
    ExynosVideoDecContext  *pCtx    = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType    ret     = VIDEO_ERROR_NONE;
    pthread_mutex_t        *pMutex = NULL;
    ExynosVideoBuffer      *pOutbuf = NULL;
    PrivateDataShareBuffer *pPDSB  = NULL;
    struct v4l2_buffer buf;
    int value, state, i, j;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (pCtx->bStreamonOutbuf == VIDEO_FALSE) {
        pOutbuf = NULL;
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

    memset(&buf, 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;

    if (pCtx->bShareOutbuf == VIDEO_TRUE)
        buf.memory = pCtx->nMemoryType;
    else
        buf.memory = V4L2_MEMORY_MMAP;

    /* HACK: pOutbuf return -1 means DECODING_ONLY for almost cases */
    if (exynos_v4l2_dqbuf(pCtx->hDec, &buf) != 0) {
        pOutbuf = NULL;
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

    pMutex = (pthread_mutex_t*)pCtx->pOutMutex;
    pthread_mutex_lock(pMutex);

    pOutbuf = &pCtx->pOutbuf[buf.index];
    exynos_v4l2_g_ctrl(pCtx->hDec, V4L2_CID_MPEG_MFC51_VIDEO_DISPLAY_STATUS, &value);

    switch (value) {
    case 0:
        pOutbuf->displayStatus = VIDEO_FRAME_STATUS_DECODING_ONLY;
        break;
    case 1:
        pOutbuf->displayStatus = VIDEO_FRAME_STATUS_DISPLAY_DECODING;
        break;
    case 2:
        pOutbuf->displayStatus = VIDEO_FRAME_STATUS_DISPLAY_ONLY;
        break;
    case 3:
        exynos_v4l2_g_ctrl(pCtx->hDec, V4L2_CID_MPEG_MFC51_VIDEO_CHECK_STATE, &state);
        if (state == 1) /* Resolution is changed */
            pOutbuf->displayStatus = VIDEO_FRAME_STATUS_CHANGE_RESOL;
        else            /* Decoding is finished */
            pOutbuf->displayStatus = VIDEO_FRAME_STATUS_DECODING_FINISHED;
        break;
    default:
        pOutbuf->displayStatus = VIDEO_FRAME_STATUS_UNKNOWN;
        break;
    }

    switch (buf.flags & (0x7 << 3)) {
    case V4L2_BUF_FLAG_KEYFRAME:
        pOutbuf->frameType = VIDEO_FRAME_I;
        break;
    case V4L2_BUF_FLAG_PFRAME:
        pOutbuf->frameType = VIDEO_FRAME_P;
        break;
    case V4L2_BUF_FLAG_BFRAME:
        pOutbuf->frameType = VIDEO_FRAME_B;
        break;
    default:
        pOutbuf->frameType = VIDEO_FRAME_OTHERS;
        break;
    };

    pPDSB = ((PrivateDataShareBuffer *)pCtx->nPrivateDataShareAddress) + buf.index;
    if (pCtx->pOutbuf[buf.index].bQueued == VIDEO_TRUE) {
        memcpy(pVideoBuffer, pOutbuf, sizeof(ExynosVideoBuffer));
        memcpy((char *)(&(pVideoBuffer->PDSB)), (char *)pPDSB, sizeof(PrivateDataShareBuffer));
    } else {
        ret = VIDEO_ERROR_NOBUFFERS;
        ALOGV("%s :: %d", __FUNCTION__, __LINE__);
    }

    pCtx->pOutbuf[buf.index].bQueued = VIDEO_FALSE;
    MFC_Decoder_BufferIndexFree_Outbuf(pHandle, pPDSB, buf.index);

    pthread_mutex_unlock(pMutex);

EXIT:
    return ret;
}

/*
 * [Decoder Buffer OPS] Enable Dynamic DPB
 */
static ExynosVideoErrorType MFC_Decoder_Enable_DynamicDPB(void *pHandle)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (exynos_v4l2_s_ctrl(pCtx->hDec, V4L2_CID_MPEG_MFC_SET_DYNAMIC_DPB_MODE, 1) != 0) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

    if (exynos_v4l2_s_ctrl(pCtx->hDec, V4L2_CID_MPEG_MFC_SET_USER_SHARED_HANDLE, pCtx->nPrivateDataShareFD) != 0) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Decoder OPS] Common
 */
static ExynosVideoDecOps defDecOps = {
    .nSize                  = 0,
    .Init                   = MFC_Decoder_Init,
    .Finalize               = MFC_Decoder_Finalize,
    .Set_DisplayDelay       = MFC_Decoder_Set_DisplayDelay,
    .Set_IFrameDecoding     = MFC_Decoder_Set_IFrameDecoding,
    .Enable_PackedPB        = MFC_Decoder_Enable_PackedPB,
    .Enable_LoopFilter      = MFC_Decoder_Enable_LoopFilter,
    .Enable_SliceMode       = MFC_Decoder_Enable_SliceMode,
    .Get_ActualBufferCount  = MFC_Decoder_Get_ActualBufferCount,
    .Set_FrameTag           = MFC_Decoder_Set_FrameTag,
    .Get_FrameTag           = MFC_Decoder_Get_FrameTag,
    .Enable_SEIParsing      = MFC_Decoder_Enable_SEIParsing,
    .Get_FramePackingInfo   = MFC_Decoder_Get_FramePackingInfo,
};

/*
 * [Decoder Buffer OPS] Input
 */
static ExynosVideoDecBufferOps defInbufOps = {
    .nSize                  = 0,
    .Enable_Cacheable       = MFC_Decoder_Enable_Cacheable_Inbuf,
    .Set_Shareable          = MFC_Decoder_Set_Shareable_Inbuf,
    .Get_Buffer             = NULL,
    .Set_Geometry           = MFC_Decoder_Set_Geometry_Inbuf,
    .Get_Geometry           = NULL,
    .Setup                  = MFC_Decoder_Setup_Inbuf,
    .Run                    = MFC_Decoder_Run_Inbuf,
    .Stop                   = MFC_Decoder_Stop_Inbuf,
    .Enqueue                = MFC_Decoder_Enqueue_Inbuf,
    .Enqueue_All            = NULL,
    .Dequeue                = MFC_Decoder_Dequeue_Inbuf,
    .Register               = MFC_Decoder_Register_Inbuf,
    .Clear_RegisteredBuffer = MFC_Decoder_Clear_RegisteredBuffer_Inbuf,
    .Clear_Queue            = MFC_Decoder_Clear_Queued_Inbuf,
    .Cleanup_Buffer         = MFC_Decoder_Cleanup_Buffer_Inbuf,
};

/*
 * [Decoder Buffer OPS] Output
 */
static ExynosVideoDecBufferOps defOutbufOps = {
    .nSize                  = 0,
    .Enable_Cacheable       = MFC_Decoder_Enable_Cacheable_Outbuf,
    .Set_Shareable          = MFC_Decoder_Set_Shareable_Outbuf,
    .Get_Buffer             = MFC_Decoder_Get_Buffer_Outbuf,
    .Set_Geometry           = MFC_Decoder_Set_Geometry_Outbuf,
    .Get_Geometry           = MFC_Decoder_Get_Geometry_Outbuf,
    .Setup                  = MFC_Decoder_Setup_Outbuf,
    .Run                    = MFC_Decoder_Run_Outbuf,
    .Stop                   = MFC_Decoder_Stop_Outbuf,
    .Enqueue                = MFC_Decoder_Enqueue_Outbuf,
    .Enqueue_All            = NULL,
    .Dequeue                = MFC_Decoder_Dequeue_Outbuf,
    .Register               = MFC_Decoder_Register_Outbuf,
    .Clear_RegisteredBuffer = MFC_Decoder_Clear_RegisteredBuffer_Outbuf,
    .Clear_Queue            = MFC_Decoder_Clear_Queued_Outbuf,
    .Cleanup_Buffer         = MFC_Decoder_Cleanup_Buffer_Outbuf,
    .ExtensionEnqueue       = MFC_Decoder_ExtensionEnqueue_Outbuf,
    .ExtensionDequeue       = MFC_Decoder_ExtensionDequeue_Outbuf,
    .Enable_DynamicDPB      = MFC_Decoder_Enable_DynamicDPB,
};

int Exynos_Video_Register_Decoder(
    ExynosVideoDecOps       *pDecOps,
    ExynosVideoDecBufferOps *pInbufOps,
    ExynosVideoDecBufferOps *pOutbufOps)
{
    ExynosVideoErrorType ret = VIDEO_ERROR_NONE;

    if ((pDecOps == NULL) || (pInbufOps == NULL) || (pOutbufOps == NULL)) {
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    defDecOps.nSize = sizeof(defDecOps);
    defInbufOps.nSize = sizeof(defInbufOps);
    defOutbufOps.nSize = sizeof(defOutbufOps);

    memcpy((char *)pDecOps + sizeof(pDecOps->nSize), (char *)&defDecOps + sizeof(defDecOps.nSize),
            pDecOps->nSize - sizeof(pDecOps->nSize));

    memcpy((char *)pInbufOps + sizeof(pInbufOps->nSize), (char *)&defInbufOps + sizeof(defInbufOps.nSize),
            pInbufOps->nSize - sizeof(pInbufOps->nSize));

    memcpy((char *)pOutbufOps + sizeof(pOutbufOps->nSize), (char *)&defOutbufOps + sizeof(defOutbufOps.nSize),
            pOutbufOps->nSize - sizeof(pOutbufOps->nSize));

EXIT:
    return ret;
}
