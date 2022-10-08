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

#ifndef _EXYNOS_VIDEO_DEC_H_
#define _EXYNOS_VIDEO_DEC_H_

/* Configurable */
#define VIDEO_DECODER_NAME              "s5p-mfc-dec"
#define VIDEO_DECODER_INBUF_SIZE        (1920 * 1080 * 3 / 2)
#define VIDEO_DECODER_INBUF_PLANES      1
#define VIDEO_DECODER_OUTBUF_PLANES     2
#define VIDEO_DECODER_POLL_TIMEOUT      25

#define OPERATE_BIT(x, mask, shift)     ((x & (mask << shift)) >> shift)
#define FRAME_PACK_SEI_INFO_NUM         4


typedef struct _ExynosVideoDecContext {
    int                  hDec;
    ExynosVideoBoolType  bShareInbuf;
    ExynosVideoBoolType  bShareOutbuf;
    ExynosVideoBuffer   *pInbuf;
    ExynosVideoBuffer   *pOutbuf;
    ExynosVideoGeometry  inbufGeometry;
    ExynosVideoGeometry  outbufGeometry;
    int                  nInbufs;
    int                  nOutbufs;
    ExynosVideoBoolType  bStreamonInbuf;
    ExynosVideoBoolType  bStreamonOutbuf;
    void                *pPrivate;
    void                *pInMutex;
    void                *pOutMutex;
    int                  nMemoryType;

    void                *hIONHandle;
    int                  nPrivateDataShareFD;
    void                *nPrivateDataShareAddress;
} ExynosVideoDecContext;

#endif /* _EXYNOS_VIDEO_DEC_H_ */
