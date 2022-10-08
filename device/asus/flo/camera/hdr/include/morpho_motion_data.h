/**
 * @file     morpho_motion_data.h
 * @brief    動きデータの構造体定義
 * @version  1.0.0
 * @date     2008-06-09
 *
 * Copyright (C) 2006-2012 Morpho, Inc.
 */

#ifndef MORPHO_MOTION_DATA_H
#define MORPHO_MOTION_DATA_H

#ifdef __cplusplus
extern "C" {
#endif

/** 動きデータ. */
typedef struct {
    int v[6];  /**< 動きデータ */
    int fcode; /**< 成功:0 / 失敗:0以外（失敗した原因） */
} morpho_MotionData;

#ifdef __cplusplus
}
#endif

#endif /* #ifndef MORPHO_MOTION_DATA_H */
