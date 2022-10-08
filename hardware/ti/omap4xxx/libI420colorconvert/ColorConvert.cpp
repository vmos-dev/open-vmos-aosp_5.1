/*
 * Copyright (C) 2011 The Android Open Source Project
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

#include <II420ColorConverter.h>
#include <OMX_IVCommon.h>
#include <string.h>

static int getDecoderOutputFormat() {
    return OMX_TI_COLOR_FormatYUV420PackedSemiPlanar;
}

static int convertDecoderOutputToI420(
    void* srcBits, int srcWidth, int srcHeight, ARect srcRect, void* dstBits) {

    const uint8_t *pSrc_y = (const uint8_t *)srcBits +
        srcWidth * srcRect.top + srcRect.left;
    const uint8_t *pSrc_uv = (const uint8_t *)pSrc_y +
        srcWidth * (srcHeight - srcRect.top / 2);

    int dstWidth = srcRect.right - srcRect.left + 1;
    int dstHeight = srcRect.bottom - srcRect.top + 1;
    size_t dst_y_size = dstWidth * dstHeight;
    size_t dst_uv_stride = dstWidth / 2;
    size_t dst_uv_size = dstWidth / 2 * dstHeight / 2;
    uint8_t *pDst_y = (uint8_t *)dstBits;
    uint8_t *pDst_u = pDst_y + dst_y_size;
    uint8_t *pDst_v = pDst_u + dst_uv_size;

    for (int y = 0; y < dstHeight; ++y) {
        memcpy(pDst_y, pSrc_y, dstWidth);
        pSrc_y += srcWidth;
        pDst_y += dstWidth;
    }

    size_t tmp = (dstWidth + 1) / 2;
    for (int y = 0; y < (dstHeight + 1) / 2; ++y) {
        for (size_t x = 0; x < tmp; ++x) {
            pDst_u[x] = pSrc_uv[2 * x];
            pDst_v[x] = pSrc_uv[2 * x + 1];
        }
        pSrc_uv += srcWidth;
        pDst_u += dst_uv_stride;
        pDst_v += dst_uv_stride;
    }
    return 0;
}

static int getEncoderInputFormat() {
    return OMX_TI_COLOR_FormatYUV420PackedSemiPlanar;
}

static int convertI420ToEncoderInput(
    void* srcBits, int srcWidth, int srcHeight,
    int dstWidth, int dstHeight, ARect dstRect,
    void* dstBits) {
    uint8_t *pSrc_y = (uint8_t*) srcBits;
    uint8_t *pDst_y = (uint8_t*) dstBits;
    for(int i=0; i < srcHeight; i++) {
        memcpy(pDst_y, pSrc_y, srcWidth);
        pSrc_y += srcWidth;
        pDst_y += dstWidth;
    }
    uint8_t* pSrc_u = (uint8_t*)srcBits + (srcWidth * srcHeight);
    uint8_t* pSrc_v = (uint8_t*)pSrc_u + (srcWidth / 2) * (srcHeight / 2);
    uint8_t* pDst_uv  = (uint8_t*)dstBits + dstWidth * dstHeight;

    for(int i=0; i < srcHeight / 2; i++) {
        for(int j=0, k=0; j < srcWidth / 2; j++, k+=2) {
            pDst_uv[k] = pSrc_u[j];
            pDst_uv[k+1] = pSrc_v[j];
        }
        pDst_uv += dstWidth;
        pSrc_u += srcWidth / 2;
        pSrc_v += srcWidth / 2;
    }
    return 0;
}

static int getEncoderInputBufferInfo(
    int actualWidth, int actualHeight,
    int* encoderWidth, int* encoderHeight,
    ARect* encoderRect, int* encoderBufferSize) {

    *encoderWidth = actualWidth;
    *encoderHeight = actualHeight;
    encoderRect->left = 0;
    encoderRect->top = 0;
    encoderRect->right = actualWidth - 1;
    encoderRect->bottom = actualHeight - 1;
    *encoderBufferSize = (actualWidth * actualHeight * 3 / 2);

    return 0;
}

extern "C" void getI420ColorConverter(II420ColorConverter *converter) {
    converter->getDecoderOutputFormat = getDecoderOutputFormat;
    converter->convertDecoderOutputToI420 = convertDecoderOutputToI420;
    converter->getEncoderInputFormat = getEncoderInputFormat;
    converter->convertI420ToEncoderInput = convertI420ToEncoderInput;
    converter->getEncoderInputBufferInfo = getEncoderInputBufferInfo;
}
