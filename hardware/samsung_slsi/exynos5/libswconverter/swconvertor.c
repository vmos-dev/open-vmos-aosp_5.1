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
 * @file    swconvertor.c
 *
 * @brief   SEC_OMX specific define. It support MFC 6.x tiled.
 *
 * @author  ShinWon Lee (shinwon.lee@samsung.com)
 *
 * @version 1.0
 *
 * @history
 *   2012.02.01 : Create
 */

#include "stdio.h"
#include "stdlib.h"
#include "swconverter.h"

/* 2D Configurable tiled memory access (TM)
 * Return the linear address from tiled position (x, y) */
unsigned int Tile2D_To_Linear(
    unsigned int width,
    unsigned int height,
    unsigned int xpos,
    unsigned int ypos,
    int crFlag)
{
    int  tileNumX;
    int  tileX, tileY;
    int  tileAddr;
    int  offset;
    int  addr;

    width = ((width + 15) / 16) * 16;
    tileNumX = width / 16;

    /* crFlag - 0: Y plane, 1: CbCr plane */
    if (crFlag == 0) {
        tileX = xpos / 16;
        tileY = ypos / 16;
        tileAddr = tileY * tileNumX + tileX;
        offset = (ypos & 15) * 16 + (xpos & 15);
        addr = (tileAddr << 8) | offset;
    } else {
        tileX = xpos / 16;
        tileY = ypos / 8;
        tileAddr = tileY * tileNumX + tileX;
        offset = (ypos & 7) * 16 + (xpos & 15);
        addr = (tileAddr << 7) | offset;
    }

    return addr;
}

/*
 * De-interleaves src to dest1, dest2
 *
 * @param dest1
 *   Address of de-interleaved data[out]
 *
 * @param dest2
 *   Address of de-interleaved data[out]
 *
 * @param src
 *   Address of interleaved data[in]
 *
 * @param src_size
 *   Size of interleaved data[in]
 */
void csc_deinterleave_memcpy(
    unsigned char *dest1,
    unsigned char *dest2,
    unsigned char *src,
    unsigned int src_size)
{
    unsigned int i = 0;
    for(i=0; i<src_size/2; i++) {
        dest1[i] = src[i*2];
        dest2[i] = src[i*2+1];
    }
}

/*
 * Interleaves src1, src2 to dest
 *
 * @param dest
 *   Address of interleaved data[out]
 *
 * @param src1
 *   Address of de-interleaved data[in]
 *
 * @param src2
 *   Address of de-interleaved data[in]
 *
 * @param src_size
 *   Size of de-interleaved data[in]
 */
void csc_interleave_memcpy(
    unsigned char *dest,
    unsigned char *src1,
    unsigned char *src2,
    unsigned int src_size)
{
    unsigned int i = 0;
    for(i=0; i<src_size; i++) {
        dest[i*2] = src1[i];
        dest[i*2+1] = src2[i];
    }
}

/*
 * Converts tiled data to linear for mfc 6.x tiled
 * 1. y of nv12t to y of yuv420p
 * 2. y of nv12t to y of yuv420s
 *
 * @param dst
 *   y address of yuv420[out]
 *
 * @param src
 *   y address of nv12t[in]
 *
 * @param yuv420_width
 *   real width of yuv420[in]
 *   it should be even
 *
 * @param yuv420_height
 *   real height of yuv420[in]
 *   it should be even.
 *
 */
void csc_tiled_to_linear_y(
    unsigned char *y_dst,
    unsigned char *y_src,
    unsigned int width,
    unsigned int height)
{
    unsigned int i, j, k;
    unsigned int aligned_width, aligned_height;
    unsigned int tiled_width;
    unsigned int src_offset, dst_offset;

    aligned_height = height & (~0xF);
    aligned_width = width & (~0xF);
    tiled_width = ((width + 15) >> 4) << 4;

    for (i = 0; i < aligned_height; i = i + 16) {
        for (j = 0; j<aligned_width; j = j + 16) {
            src_offset = (tiled_width * i) + (j << 4);
            dst_offset = width * i + j;
            for (k = 0; k < 8; k++) {
                memcpy(y_dst + dst_offset, y_src + src_offset, 16);
                src_offset += 16;
                dst_offset += width;
                memcpy(y_dst + dst_offset, y_src + src_offset, 16);
                src_offset += 16;
                dst_offset += width;
            }
        }
        if (aligned_width != width) {
            src_offset = (tiled_width * i) + (j << 4);
            dst_offset = width * i + j;
            for (k = 0; k < 8; k++) {
                memcpy(y_dst + dst_offset, y_src + src_offset, width - j);
                src_offset += 16;
                dst_offset += width;
                memcpy(y_dst + dst_offset, y_src + src_offset, width - j);
                src_offset += 16;
                dst_offset += width;
            }
        }
    }

    if (aligned_height != height) {
        for (j = 0; j<aligned_width; j = j + 16) {
            src_offset = (tiled_width * i) + (j << 4);
            dst_offset = width * i + j;
            for (k = 0; k < height - aligned_height; k = k + 2) {
                memcpy(y_dst + dst_offset, y_src + src_offset, 16);
                src_offset += 16;
                dst_offset += width;
                memcpy(y_dst + dst_offset, y_src + src_offset, 16);
                src_offset += 16;
                dst_offset += width;
            }
        }
        if (aligned_width != width) {
            src_offset = (tiled_width * i) + (j << 4);
            dst_offset = width * i + j;
            for (k = 0; k < height - aligned_height; k = k + 2) {
                memcpy(y_dst + dst_offset, y_src + src_offset, width - j);
                src_offset += 16;
                dst_offset += width;
                memcpy(y_dst + dst_offset, y_src + src_offset, width - j);
                src_offset += 16;
                dst_offset += width;
            }
        }
    }
}

/*
 * Converts tiled data to linear for mfc 6.x tiled
 * 1. uv of nv12t to uv of yuv420s
 *
 * @param dst
 *   uv address of yuv420s[out]
 *
 * @param src
 *   uv address of nv12t[in]
 *
 * @param yuv420_width
 *   real width of yuv420s[in]
 *
 * @param yuv420_height
 *   real height of yuv420s[in]
 *
 */
void csc_tiled_to_linear_uv(
    unsigned char *uv_dst,
    unsigned char *uv_src,
    unsigned int width,
    unsigned int height)
{
    unsigned int i, j, k;
    unsigned int aligned_width, aligned_height;
    unsigned int tiled_width;
    unsigned int src_offset, dst_offset;

    aligned_height = height & (~0x7);
    aligned_width = width & (~0xF);
    tiled_width = ((width + 15) >> 4) << 4;

    for (i = 0; i < aligned_height; i = i + 8) {
        for (j = 0; j<aligned_width; j = j + 16) {
            src_offset = (tiled_width * i) + (j << 3);
            dst_offset = width * i + j;
            for (k = 0; k < 4; k++) {
                memcpy(uv_dst + dst_offset, uv_src + src_offset, 16);
                src_offset += 16;
                dst_offset += width;
                memcpy(uv_dst + dst_offset, uv_src + src_offset, 16);
                src_offset += 16;
                dst_offset += width;
            }
        }
        if (aligned_width != width) {
            src_offset = (tiled_width * i) + (j << 3);
            dst_offset = width * i + j;
            for (k = 0; k < 4; k++) {
                memcpy(uv_dst + dst_offset, uv_src + src_offset, width - j);
                src_offset += 16;
                dst_offset += width;
                memcpy(uv_dst + dst_offset, uv_src + src_offset, width - j);
                src_offset += 16;
                dst_offset += width;
            }
        }
    }

    if (aligned_height != height) {
        for (j = 0; j<aligned_width; j = j + 16) {
            src_offset = (tiled_width * i) + (j << 3);
            dst_offset = width * i + j;
            for (k = 0; k < height - aligned_height; k = k + 1) {
                memcpy(uv_dst + dst_offset, uv_src + src_offset, 16);
                src_offset += 16;
                dst_offset += width;
            }
        }
        if (aligned_width != width) {
            src_offset = (tiled_width * i) + (j << 3);
            dst_offset = width * i + j;
            for (k = 0; k < height - aligned_height; k = k + 1) {
                memcpy(uv_dst + dst_offset, uv_src + src_offset, width - j);
                src_offset += 16;
                dst_offset += width;
            }
        }
    }
}

/*
 * Converts tiled data to linear for mfc 6.x tiled
 * 1. uv of nt12t to uv of yuv420p
 *
 * @param u_dst
 *   u address of yuv420p[out]
 *
 * @param v_dst
 *   v address of yuv420p[out]
 *
 * @param uv_src
 *   uv address of nt12t[in]
 *
 * @param yuv420_width
 *   real width of yuv420p[in]
 *
 * @param yuv420_height
 *   real height of yuv420p[in]
 */
void csc_tiled_to_linear_uv_deinterleave(
    unsigned char *u_dst,
    unsigned char *v_dst,
    unsigned char *uv_src,
    unsigned int width,
    unsigned int height)
{
    unsigned int i, j, k;
    unsigned int aligned_width, aligned_height;
    unsigned int tiled_width;
    unsigned int src_offset, dst_offset;

    aligned_height = height & (~0x7);
    aligned_width = width & (~0xF);
    tiled_width = ((width + 15) >> 4) << 4;

    for (i = 0; i < aligned_height; i = i + 8) {
        for (j = 0; j<aligned_width; j = j + 16) {
            src_offset = (tiled_width * i) + (j << 3);
            dst_offset = (width >> 1) * i + (j >> 1);
            for (k = 0; k < 4; k++) {
                csc_deinterleave_memcpy(u_dst + dst_offset, v_dst + dst_offset,
                                        uv_src + src_offset, 16);
                src_offset += 16;
                dst_offset += width >> 1;
                csc_deinterleave_memcpy(u_dst + dst_offset, v_dst + dst_offset,
                                        uv_src + src_offset, 16);
                src_offset += 16;
                dst_offset += width >> 1;
            }
        }
        if (aligned_width != width) {
            src_offset = (tiled_width * i) + (j << 3);
            dst_offset = (width >> 1) * i + (j >> 1);
            for (k = 0; k < 4; k++) {
                csc_deinterleave_memcpy(u_dst + dst_offset, v_dst + dst_offset,
                                        uv_src + src_offset, width - j);
                src_offset += 16;
                dst_offset += width >> 1;
                csc_deinterleave_memcpy(u_dst + dst_offset, v_dst + dst_offset,
                                        uv_src + src_offset, width - j);
                src_offset += 16;
                dst_offset += width >> 1;
            }
        }
    }
    if (aligned_height != height) {
        for (j = 0; j<aligned_width; j = j + 16) {
            src_offset = (tiled_width * i) + (j << 3);
            dst_offset = (width >> 1) * i + (j >> 1);
            for (k = 0; k < height - aligned_height; k = k + 1) {
                csc_deinterleave_memcpy(u_dst + dst_offset, v_dst + dst_offset,
                                        uv_src + src_offset, 16);
                src_offset += 16;
                dst_offset += width >> 1;
            }
        }
        if (aligned_width != width) {
            src_offset = (tiled_width * i) + (j << 3);
            dst_offset = (width >> 1) * i + (j >> 1);
            for (k = 0; k < height - aligned_height; k = k + 1) {
                csc_deinterleave_memcpy(u_dst + dst_offset, v_dst + dst_offset,
                                        uv_src + src_offset, width - j);
                src_offset += 16;
                dst_offset += width >> 1;
            }
        }
    }
}

/*
 * Converts linear data to tiled
 * It supports mfc 6.x tiled
 * 1. y of yuv420 to y of nv12t
 *
 * @param dst
 *   y address of nv12t[out]
 *
 * @param src
 *   y address of yuv420[in]
 *
 * @param yuv420_width
 *   real width of yuv420[in]
 *   it should be even
 *
 * @param yuv420_height
 *   real height of yuv420[in]
 *   it should be even.
 *
 */
void csc_linear_to_tiled_y(
    unsigned char *y_dst,
    unsigned char *y_src,
    unsigned int width,
    unsigned int height)
{

}

/*
 * Converts and interleaves linear data to tiled
 * It supports mfc 6.x tiled
 * 1. uv of nv12t to uv of yuv420
 *
 * @param dst
 *   uv address of nv12t[out]
 *
 * @param src
 *   u address of yuv420[in]
 *
 * @param src
 *   v address of yuv420[in]
 *
 * @param yuv420_width
 *   real width of yuv420[in]
 *
 * @param yuv420_height
 *   real height of yuv420[in]
 *
 */
void csc_linear_to_tiled_uv(
    unsigned char *uv_dst,
    unsigned char *u_src,
    unsigned char *v_src,
    unsigned int width,
    unsigned int height)
{

}

void Tile2D_To_YUV420(unsigned char *Y_plane, unsigned char *Cb_plane, unsigned char *Cr_plane,
                        unsigned int y_addr, unsigned int c_addr, unsigned int width, unsigned int height)
{
    int x, y, j, k, l;
    int out_of_width, actual_width;
    unsigned int base_addr, data;

    // y: 0, 16, 32, ...
    for (y = 0; y < height; y += 16) {
        // x: 0, 16, 32, ...
        for (x = 0; x < width; x += 16) {
            out_of_width = (x + 16) > width ? 1 : 0;
            base_addr = y_addr + Tile2D_To_Linear(width, height, x, y, 0);

            for (k = 0; (k < 16) && ((y + k) < height); k++) {
                actual_width = out_of_width ? ((width%4)?((width%16) / 4 + 1) : ((width%16) / 4)) : 4;
                for (l = 0; l < actual_width; l++) {
                    data = *((unsigned int*)(base_addr + 16*k + l*4));
                    for (j = 0; (j < 4) && (x + l*4 + j) < width; j++) {
                        Y_plane[(y+k)*width + x + l*4 +j] = (data>>(8*j))&0xff;
                    }
                }
            }
        }
    }

    for (y = 0; y < height/2; y += 8) {
        for (x = 0; x < width; x += 16) {
            out_of_width = (x + 16) > width ? 1 : 0;
            base_addr = c_addr + Tile2D_To_Linear(width, height/2, x, y, 1);
            for (k = 0; (k < 8) && ((y+k) < height/2); k++) {
                actual_width = out_of_width ? ((width%4) ? ((width%16) / 4 + 1) : ((width%16) / 4)) : 4;
                for (l = 0; l < actual_width; l++) {
                    data = *((unsigned int*)(base_addr + 16*k + l*4));
                    for (j = 0; (j < 2) && (x/2 + l*2 +j) < width/2; j++) {
                        Cb_plane[(y+k)*width/2 + x/2 + l*2 +j] = (data>> (8*2*j))&0xff;
                        Cr_plane[(y+k)*width/2 + x/2 + l*2 +j] = (data>>(8*2*j+8))&0xff;
                    }
                }
            }
        }
    }
}

/*
 * Converts RGB565 to YUV420P
 *
 * @param y_dst
 *   Y plane address of YUV420P[out]
 *
 * @param u_dst
 *   U plane address of YUV420P[out]
 *
 * @param v_dst
 *   V plane address of YUV420P[out]
 *
 * @param rgb_src
 *   Address of RGB565[in]
 *
 * @param width
 *   Width of RGB565[in]
 *
 * @param height
 *   Height of RGB565[in]
 */
void csc_RGB565_to_YUV420P(
    unsigned char *y_dst,
    unsigned char *u_dst,
    unsigned char *v_dst,
    unsigned char *rgb_src,
    int width,
    int height)
{
    unsigned int i, j;
    unsigned int tmp;

    unsigned int R, G, B;
    unsigned int Y, U, V;

    unsigned int offset1 = width * height;
    unsigned int offset2 = width/2 * height/2;

    unsigned short int *pSrc = (unsigned short int *)rgb_src;

    unsigned char *pDstY = (unsigned char *)y_dst;
    unsigned char *pDstU = (unsigned char *)u_dst;
    unsigned char *pDstV = (unsigned char *)v_dst;

    unsigned int yIndex = 0;
    unsigned int uIndex = 0;
    unsigned int vIndex = 0;

    for (j = 0; j < height; j++) {
        for (i = 0; i < width; i++) {
            tmp = pSrc[j * width + i];

            R = (tmp & 0x0000F800) >> 8;
            G = (tmp & 0x000007E0) >> 3;
            B = (tmp & 0x0000001F);
            B = B << 3;

            Y = ((66 * R) + (129 * G) + (25 * B) + 128);
            Y = Y >> 8;
            Y += 16;

            pDstY[yIndex++] = (unsigned char)Y;

            if ((j % 2) == 0 && (i % 2) == 0) {
                U = ((-38 * R) - (74 * G) + (112 * B) + 128);
                U = U >> 8;
                U += 128;
                V = ((112 * R) - (94 * G) - (18 * B) + 128);
                V = V >> 8;
                V += 128;

                pDstU[uIndex++] = (unsigned char)U;
                pDstV[vIndex++] = (unsigned char)V;
            }
        }
    }
}

/*
 * Converts RGB565 to YUV420SP
 *
 * @param y_dst
 *   Y plane address of YUV420SP[out]
 *
 * @param uv_dst
 *   UV plane address of YUV420SP[out]
 *
 * @param rgb_src
 *   Address of RGB565[in]
 *
 * @param width
 *   Width of RGB565[in]
 *
 * @param height
 *   Height of RGB565[in]
 */
void csc_RGB565_to_YUV420SP(
    unsigned char *y_dst,
    unsigned char *uv_dst,
    unsigned char *rgb_src,
    int width,
    int height)
{
    unsigned int i, j;
    unsigned int tmp;

    unsigned int R, G, B;
    unsigned int Y, U, V;

    unsigned int offset = width * height;

    unsigned short int *pSrc = (unsigned short int *)rgb_src;

    unsigned char *pDstY = (unsigned char *)y_dst;
    unsigned char *pDstUV = (unsigned char *)uv_dst;

    unsigned int yIndex = 0;
    unsigned int uvIndex = 0;

    for (j = 0; j < height; j++) {
        for (i = 0; i < width; i++) {
            tmp = pSrc[j * width + i];

            R = (tmp & 0x0000F800) >> 11;
            R = R * 8;
            G = (tmp & 0x000007E0) >> 5;
            G = G * 4;
            B = (tmp & 0x0000001F);
            B = B * 8;

            Y = ((66 * R) + (129 * G) + (25 * B) + 128);
            Y = Y >> 8;
            Y += 16;

            pDstY[yIndex++] = (unsigned char)Y;

            if ((j % 2) == 0 && (i % 2) == 0) {
                U = ((-38 * R) - (74 * G) + (112 * B) + 128);
                U = U >> 8;
                U += 128;
                V = ((112 * R) - (94 * G) - (18 * B) + 128);
                V = V >> 8;
                V += 128;

                pDstUV[uvIndex++] = (unsigned char)U;
                pDstUV[uvIndex++] = (unsigned char)V;
            }
        }
    }
}

/*
 * Converts RGB8888 to YUV420P
 *
 * @param y_dst
 *   Y plane address of YUV420P[out]
 *
 * @param u_dst
 *   U plane address of YUV420P[out]
 *
 * @param v_dst
 *   V plane address of YUV420P[out]
 *
 * @param rgb_src
 *   Address of ARGB8888[in]
 *
 * @param width
 *   Width of ARGB8888[in]
 *
 * @param height
 *   Height of ARGB8888[in]
 */
void csc_ARGB8888_to_YUV420P(
    unsigned char *y_dst,
    unsigned char *u_dst,
    unsigned char *v_dst,
    unsigned char *rgb_src,
    unsigned int width,
    unsigned int height)
{
    unsigned int i, j;
    unsigned int tmp;

    unsigned int R, G, B;
    unsigned int Y, U, V;

    unsigned int offset1 = width * height;
    unsigned int offset2 = width/2 * height/2;

    unsigned int *pSrc = (unsigned int *)rgb_src;

    unsigned char *pDstY = (unsigned char *)y_dst;
    unsigned char *pDstU = (unsigned char *)u_dst;
    unsigned char *pDstV = (unsigned char *)v_dst;

    unsigned int yIndex = 0;
    unsigned int uIndex = 0;
    unsigned int vIndex = 0;

    for (j = 0; j < height; j++) {
        for (i = 0; i < width; i++) {
            tmp = pSrc[j * width + i];

            R = (tmp & 0x00FF0000) >> 16;
            G = (tmp & 0x0000FF00) >> 8;
            B = (tmp & 0x000000FF);

            Y = ((66 * R) + (129 * G) + (25 * B) + 128);
            Y = Y >> 8;
            Y += 16;

            pDstY[yIndex++] = (unsigned char)Y;

            if ((j % 2) == 0 && (i % 2) == 0) {
                U = ((-38 * R) - (74 * G) + (112 * B) + 128);
                U = U >> 8;
                U += 128;
                V = ((112 * R) - (94 * G) - (18 * B) + 128);
                V = V >> 8;
                V += 128;

                pDstU[uIndex++] = (unsigned char)U;
                pDstV[vIndex++] = (unsigned char)V;
            }
        }
    }
}


/*
 * Converts ARGB8888 to YUV420S
 *
 * @param y_dst
 *   Y plane address of YUV420S[out]
 *
 * @param uv_dst
 *   UV plane address of YUV420S[out]
 *
 * @param rgb_src
 *   Address of ARGB8888[in]
 *
 * @param width
 *   Width of ARGB8888[in]
 *
 * @param height
 *   Height of ARGB8888[in]
 */
void csc_ARGB8888_to_YUV420SP(
    unsigned char *y_dst,
    unsigned char *uv_dst,
    unsigned char *rgb_src,
    unsigned int width,
    unsigned int height)
{
    unsigned int i, j;
    unsigned int tmp;

    unsigned int R, G, B;
    unsigned int Y, U, V;

    unsigned int offset = width * height;

    unsigned int *pSrc = (unsigned int *)rgb_src;

    unsigned char *pDstY = (unsigned char *)y_dst;
    unsigned char *pDstUV = (unsigned char *)uv_dst;

    unsigned int yIndex = 0;
    unsigned int uvIndex = 0;

    for (j = 0; j < height; j++) {
        for (i = 0; i < width; i++) {
            tmp = pSrc[j * width + i];

            R = (tmp & 0x00FF0000) >> 16;
            G = (tmp & 0x0000FF00) >> 8;
            B = (tmp & 0x000000FF);

            Y = ((66 * R) + (129 * G) + (25 * B) + 128);
            Y = Y >> 8;
            Y += 16;

            pDstY[yIndex++] = (unsigned char)Y;

            if ((j % 2) == 0 && (i % 2) == 0) {
                U = ((-38 * R) - (74 * G) + (112 * B) + 128);
                U = U >> 8;
                U += 128;
                V = ((112 * R) - (94 * G) - (18 * B) + 128);
                V = V >> 8;
                V += 128;

                pDstUV[uvIndex++] = (unsigned char)U;
                pDstUV[uvIndex++] = (unsigned char)V;
            }
        }
    }
}