/**
 * @file     morpho_image_data_ex.h
 * @brief    画像データの構造体定義
 * @version  1.0.0
 * @date     2010-03-30
 *
 * Copyright (C) 2010-2011 Morpho, Inc.
 */

#ifndef MORPHO_IMAGE_DATA_EX_H
#define MORPHO_IMAGE_DATA_EX_H

#include "morpho_image_data.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct{
    int y;
    int u;
    int v;
} morpho_ImageYuvPlanarPitch;

typedef struct{
    int y;
    int uv;
} morpho_ImageYuvSemiPlanarPitch;

/** 画像データ. */
typedef struct {
    int width;              /**< 幅 */
    int height;             /**< 高さ */
    union{
        void *p;            /**< 画像データの先頭ポインタ */
        morpho_ImageYuvPlanar planar;
        morpho_ImageYuvSemiPlanar semi_planar;
    } dat;
    union{
        int p;              /**< ラインの先頭から次のライン先頭までのバイト数 */
        morpho_ImageYuvPlanarPitch planar;
        morpho_ImageYuvSemiPlanarPitch semi_planar;
    } pitch;
} morpho_ImageDataEx;


#ifdef __cplusplus
}
#endif

#endif /* #ifndef MORPHO_IMAGE_DATA_EX_H */
