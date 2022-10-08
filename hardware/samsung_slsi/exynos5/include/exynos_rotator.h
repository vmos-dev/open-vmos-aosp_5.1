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

/*!
 * \file      exynos_rotator.h
 * \brief     header file for exynos_rotator HAL
 * \author    Sunmi Lee (carrotsm.lee@samsung.com)
 * \date      2012/03/05
 *
 * <b>Revision History: </b>
 * - 2012/03/05 : Sunmi Lee (carrotsm.lee@samsung.com) \n
 *   Create
 *
 */

/*!
 * \defgroup exynos_rotator
 * \brief API for rotator
 * \addtogroup Exynos
 */

#ifndef _EXYNOS_ROTATOR_H_
#define _EXYNOS_ROTATOR_H_

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * Create librotator handle
 *
 * \ingroup exynos_rotator
 *
 * \return
 *   librotator handle
 */
void *exynos_rotator_create(void);

/*!
 * Destroy librotator handle
 *
 * \ingroup exynos_rotator
 *
 * \param handle
 *   librotator handle[in]
 */
void exynos_rotator_destroy(
    void *handle);

/*!
 * Set source format.
 *
 * \ingroup exynos_rotator
 *
 * \param handle
 *   librotator handle[in]
 *
 * \param width
 *   image width[in]
 *
 * \param height
 *   image height[in]
 *
 * \param crop_left
 *   image left crop size[in]
 *
 * \param crop_top
 *   image top crop size[in]
 *
 * \param crop_width
 *   cropped image width[in]
 *
 * \param crop_height
 *   cropped image height[in]
 *
 * \param v4l2_colorformat
 *   color format[in]
 *
 * \param cacheable
 *   ccacheable[in]
 *
 * \return
 *   error code
 */
int exynos_rotator_set_src_format(
    void        *handle,
    unsigned int width,
    unsigned int height,
    unsigned int crop_left,
    unsigned int crop_top,
    unsigned int crop_width,
    unsigned int crop_height,
    unsigned int v4l2_colorformat,
    unsigned int cacheable);

/*!
 * Set destination format.
 *
 * \ingroup exynos_rotator
 *
 * \param handle
 *   librotator handle[in]
 *
 * \param width
 *   image width[in]
 *
 * \param height
 *   image height[in]
 *
 * \param crop_left
 *   image left crop size[in]
 *
 * \param crop_top
 *   image top crop size[in]
 *
 * \param v4l2_colorformat
 *   color format[in]
 *
 * \param cacheable
 *   ccacheable[in]
 *
 * \return
 *   error code
 */
int exynos_rotator_set_dst_format(
    void        *handle,
    unsigned int width,
    unsigned int height,
    unsigned int crop_left,
    unsigned int crop_top,
    unsigned int v4l2_colorformat,
    unsigned int cacheable);

/*!
 * Set rotation.
 *
 * \ingroup exynos_rotator
 *
 * \param handle
 *   librotator handle[in]
 *
 * \param rotation
 *   image rotation. It should be multiple of 90[in]
 *
 * \return
 *   error code
 */
int exynos_rotator_set_rotation(
    void *handle,
    int   rotation);

/*!
 * Set source buffer
 *
 * \ingroup exynos_rotator
 *
 * \param handle
 *   librotator handle[in]
 *
 * \param addr
 *   buffer pointer array[in]
 *
 * \return
 *   error code
 */
int exynos_rotator_set_src_addr(
    void *handle,
    void *addr[3]);

/*!
 * Set destination buffer
 *
 * \ingroup exynos_rotator
 *
 * \param handle
 *   librotator handle[in]
 *
 * \param addr
 *   buffer pointer array[in]
 *
 * \return
 *   error code
 */
int exynos_rotator_set_dst_addr(
    void *handle,
    void *addr[3]);

/*!
 * Convert color space with presetup color format
 *
 * \ingroup exynos_rotator
 *
 * \param handle
 *   librotator handle[in]
 *
 * \return
 *   error code
 */
int exynos_rotator_convert(
    void *handle);

/*!
 * api for local path rotator. Not yet support.
 *
 * \ingroup exynos_rotator
 */
int exynos_rotator_connect(
    void *handle,
    void *hw);

/*!
 * api for local path rotator. Not yet support.
 *
 * \ingroup exynos_rotator
 */
int exynos_rotator_disconnect(
    void *handle,
    void *hw);

#ifdef __cplusplus
}
#endif

#endif /*EXYNOS_ROTATORALER_H_*/