/*
 * Copyright (C) 2012 The Android Open Source Project
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
 * @file    csc.h
 *
 * @brief   color space convertion abstract header
 *
 * @author  Pyoungjae Jung (pjet.jung@samsung.com)
 *
 * @version 1.0
 *
 * @history
 *   2011.12.27 : Create
 */

#ifndef CSC_H
#define CSC_H

#ifdef __cplusplus
extern "C" {
#endif

#define CSC_MAX_PLANES 3

typedef enum _CSC_ERRORCODE {
    CSC_ErrorNone = 0,
    CSC_Error,
    CSC_ErrorNotInit,
    CSC_ErrorInvalidAddress,
    CSC_ErrorUnsupportFormat,
    CSC_ErrorNotImplemented
} CSC_ERRORCODE;

typedef enum _CSC_METHOD {
    CSC_METHOD_SW = 0,
    CSC_METHOD_HW
} CSC_METHOD;

typedef enum _CSC_HW_PROPERTY_TYPE {
    CSC_HW_PROPERTY_FIXED_NODE = 0,
    CSC_HW_PROPERTY_MODE_DRM,
    CSC_HW_PROPERTY_HW_TYPE,
} CSC_HW_PROPERTY_TYPE;

typedef enum _CSC_HW_TYPE {
    CSC_HW_TYPE_NONE = 0,
    CSC_HW_TYPE_FIMC,
    CSC_HW_TYPE_GSCALER,
    CSC_HW_TYPE_G2D,
} CSC_HW_TYPE;

/*
 * change hal pixel format to omx pixel format
 *
 * @param hal_format
 *   hal pixel format[in]
 *
 * @return
 *   omx pixel format
 */
unsigned int hal_2_omx_pixel_format(
    unsigned int hal_format);

/*
 * change omx pixel format to hal pixel format
 *
 * @param hal_format
 *   omx pixel format[in]
 *
 * @return
 *   hal pixel format
 */
unsigned int omx_2_hal_pixel_format(
    unsigned int omx_format);

/*
 * change hal pixel format to g2d color format
 *
 * @param hal_format
 *   hal pixel format[in]
 *
 * @return
 *   g2d color format
 */
unsigned int hal_2_g2d_color_format(unsigned int hal_format);

/*
 * change hal pixel format to g2d pixel order
 *
 * @param hal_format
 *   hal pixel format[in]
 *
 * @return
 *   g2d pixel order
 */
unsigned int hal_2_g2d_pixel_order(unsigned int hal_format);

/*
 * change hal pixel format to g2d "bpp" (actual bpp for RGB formats, 8 bpp for
 * YUV formats)
 *
 * @param hal_format
 *   hal pixel format[in]
 *
 * @return
 *   g2d bpp
 */
size_t hal_2_g2d_bpp(unsigned int hal_format);

/*
 * Init CSC handle
 *
 * @return
 *   csc handle
 */
void *csc_init(
    CSC_METHOD method);

/*
 * Deinit CSC handle
 *
 * @param handle
 *   CSC handle[in]
 *
 * @return
 *   error code
 */
CSC_ERRORCODE csc_deinit(
    void *handle);

/*
 * get color space converter method
 *
 * @param handle
 *   CSC handle[in]
 *
 * @param method
 *   CSC method[out]
 *
 * @return
 *   error code
 */
CSC_ERRORCODE csc_get_method(
    void           *handle,
    CSC_METHOD     *method);

/*
 * set color space converter method
 *
 * @param handle
 *   CSC handle[in]
 *
 * @param method
 *   CSC method[in]
 *
 * @return
 *   error code
 */
CSC_ERRORCODE csc_set_method(
    void           *handle,
    CSC_METHOD     method);

/*
 * Set hw property
 *
 * @param handle
 *   CSC handle[in]
 *
 * @param property
 *   csc hw property[in]
 *
 * @param value
 *   csc hw property value[in]
 *
 * @return
 *   csc handle
 */
CSC_ERRORCODE csc_set_hw_property(
    void                *handle,
    CSC_HW_PROPERTY_TYPE property,
    int                  value);

/*
 * Get source format.
 *
 * @param handle
 *   CSC handle[in]
 *
 * @param width
 *   address of image width[out]
 *
 * @param height
 *   address of image height[out]
 *
 * @param crop_left
 *   address of image left crop size[out]
 *
 * @param crop_top
 *   address of image top crop size[out]
 *
 * @param crop_width
 *   address of cropped image width[out]
 *
 * @param crop_height
 *   address of cropped image height[out]
 *
 * @param color_format
 *   address of source color format(HAL format)[out]
 *
 * @return
 *   error code
 */
CSC_ERRORCODE csc_get_src_format(
    void           *handle,
    unsigned int   *width,
    unsigned int   *height,
    unsigned int   *crop_left,
    unsigned int   *crop_top,
    unsigned int   *crop_width,
    unsigned int   *crop_height,
    unsigned int   *color_format,
    unsigned int   *cacheable);

/*
 * Set source format.
 * Don't call each converting time.
 * Pls call this function as below.
 *   1. first converting time
 *   2. format is changed
 *
 * @param handle
 *   CSC handle[in]
 *
 * @param width
 *   image width[in]
 *
 * @param height
 *   image height[in]
 *
 * @param crop_left
 *   image left crop size[in]
 *
 * @param crop_top
 *   image top crop size[in]
 *
 * @param crop_width
 *   cropped image width[in]
 *
 * @param crop_height
 *   cropped image height[in]
 *
 * @param color_format
 *   source color format(HAL format)[in]
 *
 * @return
 *   error code
 */
CSC_ERRORCODE csc_set_src_format(
    void           *handle,
    unsigned int    width,
    unsigned int    height,
    unsigned int    crop_left,
    unsigned int    crop_top,
    unsigned int    crop_width,
    unsigned int    crop_height,
    unsigned int    color_format,
    unsigned int    cacheable);

/*
 * Get destination format.
 *
 * @param handle
 *   CSC handle[in]
 *
 * @param width
 *   address of image width[out]
 *
 * @param height
 *   address of image height[out]
 *
 * @param crop_left
 *   address of image left crop size[out]
 *
 * @param crop_top
 *   address of image top crop size[out]
 *
 * @param crop_width
 *   address of cropped image width[out]
 *
 * @param crop_height
 *   address of cropped image height[out]
 *
 * @param color_format
 *   address of color format(HAL format)[out]
 *
 * @return
 *   error code
 */
CSC_ERRORCODE csc_get_dst_format(
    void           *handle,
    unsigned int   *width,
    unsigned int   *height,
    unsigned int   *crop_left,
    unsigned int   *crop_top,
    unsigned int   *crop_width,
    unsigned int   *crop_height,
    unsigned int   *color_format,
    unsigned int   *cacheable);

/*
 * Set destination format
 * Don't call each converting time.
 * Pls call this function as below.
 *   1. first converting time
 *   2. format is changed
 *
 * @param handle
 *   CSC handle[in]
 *
 * @param width
 *   image width[in]
 *
 * @param height
 *   image height[in]
 *
 * @param crop_left
 *   image left crop size[in]
 *
 * @param crop_top
 *   image top crop size[in]
 *
 * @param crop_width
 *   cropped image width[in]
 *
 * @param crop_height
 *   cropped image height[in]
 *
 * @param color_format
 *   destination color format(HAL format)[in]
 *
 * @return
 *   error code
 */
CSC_ERRORCODE csc_set_dst_format(
    void           *handle,
    unsigned int    width,
    unsigned int    height,
    unsigned int    crop_left,
    unsigned int    crop_top,
    unsigned int    crop_width,
    unsigned int    crop_height,
    unsigned int    color_format,
    unsigned int    cacheable);

/*
 * Setup source buffer
 * set_format func should be called before this this func.
 *
 * @param handle
 *   CSC handle[in]
 *
 * @param src_buffer
 *   source buffer pointer array[in]
 *
 * @param y
 *   y or RGB destination pointer[in]
 *
 * @param u
 *   u or uv destination pointer[in]
 *
 * @param v
 *   v or none destination pointer[in]
 *
 * @return
 *   error code
 */
CSC_ERRORCODE csc_set_src_buffer(
    void *handle,
    void *addr[CSC_MAX_PLANES]);

/*
 * Setup destination buffer
 *
 * @param handle
 *   CSC handle[in]
 *
 * @param y
 *   y or RGB destination pointer[in]
 *
 * @param u
 *   u or uv destination pointer[in]
 *
 * @param v
 *   v or none destination pointer[in]
 *
 * @return
 *   error code
 */
CSC_ERRORCODE csc_set_dst_buffer(
    void *handle,
    void *addr[CSC_MAX_PLANES]);

/*
 * Convert color space with presetup color format
 *
 * @param handle
 *   CSC handle[in]
 *
 * @return
 *   error code
 */
CSC_ERRORCODE csc_convert(
    void *handle);

#ifdef __cplusplus
}
#endif

#endif
