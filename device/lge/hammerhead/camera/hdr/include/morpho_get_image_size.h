/**
 * @file     morpho_get_image_size.h
 * @brief    画像に必要なメモリサイズを取得する関数
 * @version  1.0.0
 * @date     2008-07-01
 *
 * Copyright (C) 2006-2012 Morpho, Inc.
 */

#ifndef MORPHO_GET_IMAGE_SIZE_H
#define MORPHO_GET_IMAGE_SIZE_H

#include "morpho_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 幅と高さとフォーマット名から、画像を格納するのに必要なメモリサイズを得る.
 *
 * @param width       幅
 * @param height      高さ
 * @param p_format    画像フォーマット文字列
 * @return            画像に必要なメモリサイズ
 */
#define morpho_getImageSize mor_noise_reduction_IF_getImageSize

MORPHO_API(int)
morpho_getImageSize(int width, int height, const char *p_format);

/**
 * Y画像データサイズを取得.
 *
 * @param width       幅
 * @param height      高さ
 * @param p_format    画像フォーマット文字列
 * @return            Y画像データサイズ
 */
#define morpho_getImageSizeY mor_noise_reduction_IF_getImageSizeY

MORPHO_API(int)
morpho_getImageSizeY(int width, int height, const char *p_format);

/**
 * U画像データサイズを取得.
 *
 * @param width       幅
 * @param height      高さ
 * @param p_format    画像フォーマット文字列
 * @return            U画像データサイズ
 */
#define morpho_getImageSizeU mor_noise_reduction_IF_getImageSizeU

MORPHO_API(int)
morpho_getImageSizeU(int width, int height, const char *p_format);

/**
 * V画像データサイズを取得.
 *
 * @param width       幅
 * @param height      高さ
 * @param p_format    画像フォーマット文字列
 * @return            V画像データサイズ
 */
#define morpho_getImageSizeV mor_noise_reduction_IF_getImageSizeV

MORPHO_API(int)
morpho_getImageSizeV(int width, int height, const char *p_format);

/**
 * UV画像データサイズを取得.
 *
 * @param width       幅
 * @param height      高さ
 * @param p_format    画像フォーマット文字列
 * @return            UV画像データサイズ
 */
#define morpho_getImageSizeUV mor_noise_reduction_IF_getImageSizeUV

MORPHO_API(int)
morpho_getImageSizeUV(int width, int height, const char *p_format);


#ifdef __cplusplus
} /* extern "C" { */
#endif

#endif /* MORPHO_GET_IMAGE_SIZE_H */
