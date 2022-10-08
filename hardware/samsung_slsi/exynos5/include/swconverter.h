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
 * @file    swconverter.h
 * @brief   SEC_OMX specific define. It support MFC 6.x tiled.
 *   NV12T(tiled) layout:
 *   Each element is not pixel. It is 64x32 pixel block.
 *   uv pixel block is interleaved as u v u v u v ...
 *   y1    y2    y7    y8    y9    y10   y15   y16
 *   y3    y4    y5    y6    y11   y12   y13   y14
 *   y17   y18   y23   y24   y25   y26   y31   y32
 *   y19   y20   y21   y22   y27   y28   y29   y30
 *   uv1   uv2   uv7   uv8   uv9   uv10  uv15  uv16
 *   uv3   uv4   uv5   uv6   uv11  uv12  uv13  uv14
 *   YUV420Planar(linear) layout:
 *   Each element is not pixel. It is 64x32 pixel block.
 *   y1    y2    y3    y4    y5    y6    y7    y8
 *   y9    y10   y11   y12   y13   y14   y15   y16
 *   y17   y18   y19   y20   y21   y22   y23   y24
 *   y25   y26   y27   y28   y29   y30   y31   y32
 *   u1    u2    u3    u4    u5    u6    u7    u8
 *   v1    v2    v3    v4    v5    v6    v7    v8
 *   YUV420Semiplanar(linear) layout:
 *   Each element is not pixel. It is 64x32 pixel block.
 *   uv pixel block is interleaved as u v u v u v ...
 *   y1    y2    y3    y4    y5    y6    y7    y8
 *   y9    y10   y11   y12   y13   y14   y15   y16
 *   y17   y18   y19   y20   y21   y22   y23   y24
 *   y25   y26   y27   y28   y29   y30   y31   y32
 *   uv1   uv2   uv3   uv4   uv5   uv6   uv7   uv8
 *   uv9   uv10  uv11  uv12  uv13  uv14  uv15  uv16
 * @author  ShinWon Lee (shinwon.lee@samsung.com)
 * @version 1.0
 * @history
 *   2012.02.01 : Create
 */

#ifndef SW_CONVERTOR_H_
#define SW_CONVERTOR_H_

/*--------------------------------------------------------------------------------*/
/* Format Conversion API                                                          */
/*--------------------------------------------------------------------------------*/
/* C Code */
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
    unsigned int src_size);

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
    unsigned int src_size);

/*
 * Converts tiled data to linear
 * It supports mfc 6.x tiled
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
    unsigned int height);

/*
 * Converts tiled data to linear
 * It supports mfc 6.x tiled
 * 1. uv of nv12t to y of yuv420s
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
    unsigned int height);

/*
 * Converts tiled data to linear
 * It supports mfc 6.x tiled
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
    unsigned int height);

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
    unsigned int height);

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
    unsigned int height);

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
    int height);

/*
 * Converts RGB565 to YUV420S
 *
 * @param y_dst
 *   Y plane address of YUV420S[out]
 *
 * @param uv_dst
 *   UV plane address of YUV420S[out]
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
    int height);

/*
 * Converts ARGB8888 to YUV420P
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
    unsigned int height);

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
    unsigned int height);

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
void csc_deinterleave_memcpy_neon(
    unsigned char *dest1,
    unsigned char *dest2,
    unsigned char *src,
    unsigned int src_size);

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
void csc_interleave_memcpy_neon(
    unsigned char *dest,
    unsigned char *src1,
    unsigned char *src2,
    unsigned int src_size);

/*
 * Converts tiled data to linear for mfc 6.x
 * 1. Y of NV12T to Y of YUV420P
 * 2. Y of NV12T to Y of YUV420S
 *
 * @param dst
 *   Y address of YUV420[out]
 *
 * @param src
 *   Y address of NV12T[in]
 *
 * @param yuv420_width
 *   real width of YUV420[in]
 *
 * @param yuv420_height
 *   Y: real height of YUV420[in]
 *
 */
void csc_tiled_to_linear_y_neon(
    unsigned char *y_dst,
    unsigned char *y_src,
    unsigned int width,
    unsigned int height);

/*
 * Converts tiled data to linear for mfc 6.x
 * 1. UV of NV12T to Y of YUV420S
 *
 * @param u_dst
 *   UV plane address of YUV420P[out]
 *
 * @param nv12t_src
 *   Y or UV plane address of NV12T[in]
 *
 * @param yuv420_width
 *   real width of YUV420[in]
 *
 * @param yuv420_height
 *   (real height)/2 of YUV420[in]
 */
void csc_tiled_to_linear_uv_neon(
    unsigned char *uv_dst,
    unsigned char *uv_src,
    unsigned int width,
    unsigned int height);

/*
 * Converts tiled data to linear for mfc 6.x
 * Deinterleave src to u_dst, v_dst
 * 1. UV of NV12T to Y of YUV420P
 *
 * @param u_dst
 *   U plane address of YUV420P[out]
 *
 * @param v_dst
 *   V plane address of YUV420P[out]
 *
 * @param nv12t_src
 *   Y or UV plane address of NV12T[in]
 *
 * @param yuv420_width
 *   real width of YUV420[in]
 *
 * @param yuv420_height
 *   (real height)/2 of YUV420[in]
 */
void csc_tiled_to_linear_uv_deinterleave_neon(
    unsigned char *u_dst,
    unsigned char *v_dst,
    unsigned char *uv_src,
    unsigned int width,
    unsigned int height);

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
void csc_linear_to_tiled_y_neon(
    unsigned char *y_dst,
    unsigned char *y_src,
    unsigned int width,
    unsigned int height);

/*
 * Converts and interleave linear data to tiled
 * It supports mfc 6.x tiled
 * 1. uv of nv12t to uv of yuv420
 *
 * @param dst
 *   uv address of yuv420[out]
 *
 * @param src
 *   uv address of nv12t[in]
 *
 * @param yuv420_width
 *   real width of yuv420[in]
 *
 * @param yuv420_height
 *   real height of yuv420[in]
 *
 */
void csc_linear_to_tiled_uv_neon(
    unsigned char *uv_dst,
    unsigned char *uv_src,
    unsigned int width,
    unsigned int height);

void csc_ARGB8888_to_YUV420SP_NEON(
    unsigned char *y_dst,
    unsigned char *uv_dst,
    unsigned char *rgb_src,
    unsigned int width,
    unsigned int height);

#endif /*COLOR_SPACE_CONVERTOR_H_*/
