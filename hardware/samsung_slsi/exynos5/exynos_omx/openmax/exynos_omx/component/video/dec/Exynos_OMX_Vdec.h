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
 * @file        Exynos_OMX_Vdec.h
 * @brief
 * @author      SeungBeom Kim (sbcrux.kim@samsung.com)
 *              HyeYeon Chung (hyeon.chung@samsung.com)
 *              Yunji Kim (yunji.kim@samsung.com)
 * @version     2.0.0
 * @history
 *   2012.02.20 : Create
 */

#ifndef EXYNOS_OMX_VIDEO_DECODE
#define EXYNOS_OMX_VIDEO_DECODE

#include "OMX_Component.h"
#include "Exynos_OMX_Def.h"
#include "Exynos_OSAL_Queue.h"
#include "Exynos_OMX_Baseport.h"
#include "Exynos_OMX_Basecomponent.h"
#include "ExynosVideoApi.h"

#define MAX_VIDEO_INPUTBUFFER_NUM           5
#define MAX_VIDEO_OUTPUTBUFFER_NUM          2

#define DEFAULT_FRAME_WIDTH                 176
#define DEFAULT_FRAME_HEIGHT                144

#define DEFAULT_VIDEO_INPUT_BUFFER_SIZE    (DEFAULT_FRAME_WIDTH * DEFAULT_FRAME_HEIGHT) * 2
#define DEFAULT_VIDEO_OUTPUT_BUFFER_SIZE   (DEFAULT_FRAME_WIDTH * DEFAULT_FRAME_HEIGHT * 3) / 2

#define MFC_INPUT_BUFFER_NUM_MAX            3
#define DEFAULT_MFC_INPUT_BUFFER_SIZE       1920 * 1080 * 3 / 2

#define MFC_OUTPUT_BUFFER_NUM_MAX           16 * 2
#define DEFAULT_MFC_OUTPUT_YBUFFER_SIZE     1920 * 1080
#define DEFAULT_MFC_OUTPUT_CBUFFER_SIZE     1920 * 1080 / 2

#define INPUT_PORT_SUPPORTFORMAT_NUM_MAX    1
#define OUTPUT_PORT_SUPPORTFORMAT_NUM_MAX   4

#define EXTRA_DPB_NUM                       5

#define MFC_INPUT_BUFFER_PLANE              1
#define MFC_OUTPUT_BUFFER_PLANE             2

#define MAX_OUTPUTBUFFER_NUM_DYNAMIC        0 /* Dynamic number of metadata buffer */

#define MAX_DISPLAY_EXTRA_BUFFER            2

typedef struct
{
    void *pAddrY;
    void *pAddrC;
} CODEC_DEC_ADDR_INFO;

typedef struct _CODEC_DEC_BUFFER
{
    void *pVirAddr[MAX_BUFFER_PLANE];   /* virtual address   */
    int   bufferSize[MAX_BUFFER_PLANE]; /* buffer alloc size */
    int   fd[MAX_BUFFER_PLANE];         /* buffer FD */
    int   dataSize;                     /* total data length */
} CODEC_DEC_BUFFER;

typedef struct _DECODE_CODEC_EXTRA_BUFFERINFO
{
    /* For Decode Output */
    OMX_U32 imageWidth;
    OMX_U32 imageHeight;
    OMX_COLOR_FORMATTYPE ColorFormat;
    PrivateDataShareBuffer PDSB;
} DECODE_CODEC_EXTRA_BUFFERINFO;

typedef struct _EXYNOS_OMX_VIDEODEC_COMPONENT
{
    OMX_HANDLETYPE hCodecHandle;
    OMX_BOOL bThumbnailMode;
    OMX_BOOL bFirstFrame;
    CODEC_DEC_BUFFER *pMFCDecInputBuffer[MFC_INPUT_BUFFER_NUM_MAX];
    CODEC_DEC_BUFFER *pMFCDecOutputBuffer[MFC_OUTPUT_BUFFER_NUM_MAX];

    /* Buffer Process */
    OMX_BOOL       bExitBufferProcessThread;
    OMX_HANDLETYPE hSrcInputThread;
    OMX_HANDLETYPE hSrcOutputThread;
    OMX_HANDLETYPE hDstInputThread;
    OMX_HANDLETYPE hDstOutputThread;

    /* Shared Memory Handle */
    OMX_HANDLETYPE hSharedMemory;

    /* For DRM Play */
    OMX_BOOL bDRMPlayerMode;

    /* For Reconfiguration DPB */
    OMX_BOOL bReconfigDPB;

    /* CSC handle */
    OMX_PTR csc_handle;
    OMX_U32 csc_set_format;

    OMX_HANDLETYPE hRefHandle;

    OMX_ERRORTYPE (*exynos_codec_srcInputProcess) (OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *pInputData);
    OMX_ERRORTYPE (*exynos_codec_srcOutputProcess) (OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *pInputData);
    OMX_ERRORTYPE (*exynos_codec_dstInputProcess) (OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *pOutputData);
    OMX_ERRORTYPE (*exynos_codec_dstOutputProcess) (OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *pOutputData);

    OMX_ERRORTYPE (*exynos_codec_start)(OMX_COMPONENTTYPE *pOMXComponent, OMX_U32 nPortIndex);
    OMX_ERRORTYPE (*exynos_codec_stop)(OMX_COMPONENTTYPE *pOMXComponent, OMX_U32 nPortIndex);
    OMX_ERRORTYPE (*exynos_codec_bufferProcessRun)(OMX_COMPONENTTYPE *pOMXComponent, OMX_U32 nPortIndex);
    OMX_ERRORTYPE (*exynos_codec_enqueueAllBuffer)(OMX_COMPONENTTYPE *pOMXComponent, OMX_U32 nPortIndex);

    int (*exynos_checkInputFrame) (OMX_U8 *pInputStream, OMX_U32 buffSize, OMX_U32 flag,
                                   OMX_BOOL bPreviousFrameEOF, OMX_BOOL *pbEndOfFrame);
    OMX_ERRORTYPE (*exynos_codec_getCodecInputPrivateData) (OMX_PTR codecBuffer, OMX_PTR addr, OMX_U32 *size);
    OMX_ERRORTYPE (*exynos_codec_getCodecOutputPrivateData) (OMX_PTR codecBuffer, OMX_PTR addr[], OMX_U32 size[]);
    OMX_ERRORTYPE (*exynos_codec_reconfigAllBuffers) (OMX_COMPONENTTYPE *pOMXComponent, OMX_U32 nPortIndex);
} EXYNOS_OMX_VIDEODEC_COMPONENT;

#ifdef __cplusplus
extern "C" {
#endif

int calc_plane(int width, int height);
inline void Exynos_UpdateFrameSize(OMX_COMPONENTTYPE *pOMXComponent);
OMX_BOOL Exynos_Check_BufferProcess_State(EXYNOS_OMX_BASECOMPONENT *pExynosComponent, OMX_U32 nPortIndex);
OMX_ERRORTYPE Exynos_Input_CodecBufferToData(EXYNOS_OMX_BASECOMPONENT *pExynosComponent, OMX_PTR codecBuffer, EXYNOS_OMX_DATA *pData);
OMX_ERRORTYPE Exynos_Output_CodecBufferToData(EXYNOS_OMX_BASECOMPONENT *pExynosComponent, OMX_PTR codecBuffer, EXYNOS_OMX_DATA *pData);

OMX_ERRORTYPE Exynos_OMX_SrcInputBufferProcess(OMX_HANDLETYPE hComponent);
OMX_ERRORTYPE Exynos_OMX_SrcOutputBufferProcess(OMX_HANDLETYPE hComponent);
OMX_ERRORTYPE Exynos_OMX_DstInputBufferProcess(OMX_HANDLETYPE hComponent);
OMX_ERRORTYPE Exynos_OMX_DstOutputBufferProcess(OMX_HANDLETYPE hComponent);
OMX_ERRORTYPE Exynos_OMX_VideoDecodeComponentInit(OMX_IN OMX_HANDLETYPE hComponent);
OMX_ERRORTYPE Exynos_OMX_VideoDecodeComponentDeinit(OMX_IN OMX_HANDLETYPE hComponent);

#ifdef __cplusplus
}
#endif

#endif
