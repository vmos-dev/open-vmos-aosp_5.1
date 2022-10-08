/**
 * @file     morpho_image_data.h
 * @brief    画像データの構造体定義
 * @version  1.0.0
 * @date     2008-06-09
 *
 * Copyright (C) 2006-2012 Morpho, Inc.
 */

#ifndef MORPHO_IMAGE_DATA_H
#define MORPHO_IMAGE_DATA_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct{
    void * y;               /**< Y画像の先頭ポインタ */
    void * u;               /**< U画像の先頭ポインタ */
    void * v;               /**< V画像の先頭ポインタ */
} morpho_ImageYuvPlanar;

typedef struct{
    void * y;               /**< Y画像の先頭ポインタ */
    void * uv;              /**< UV画像の先頭ポインタ */
} morpho_ImageYuvSemiPlanar;

/** 画像データ. */
typedef struct {
    int width;              /**< 幅 */
    int height;             /**< 高さ */
    union{
        void * p;           /**< 画像データの先頭ポインタ */
        morpho_ImageYuvPlanar planar;
        morpho_ImageYuvSemiPlanar semi_planar;
    } dat;
} morpho_ImageData;

#ifdef __cplusplus
}
#endif

#endif /* #ifndef MORPHO_IMAGE_DATA_H */
