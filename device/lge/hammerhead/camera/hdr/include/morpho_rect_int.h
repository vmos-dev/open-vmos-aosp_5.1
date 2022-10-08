/**
 * @file     morpho_rect_int.h
 * @brief    矩形データの構造体定義
 * @version  1.0.0
 * @date     2008-06-09
 *
 * Copyright (C) 2006-2012 Morpho, Inc.
 */

#ifndef MORPHO_RECT_INT_H
#define MORPHO_RECT_INT_H

#ifdef __cplusplus
extern "C" {
#endif

/** 矩形データ. */
typedef struct {
    int sx; /**< left */
    int sy; /**< top */
    int ex; /**< right */
    int ey; /**< bottom */
} morpho_RectInt;

/** 矩形領域 rect の左上座標 (l,t) と右下座標 (r,b) を設定する. */
#define morpho_RectInt_setRect(rect,l,t,r,b) do { \
	(rect)->sx=(l);\
	(rect)->sy=(t);\
	(rect)->ex=(r);\
	(rect)->ey=(b);\
    } while(0)

#ifdef __cplusplus
}
#endif

#endif /* #ifndef MORPHO_RECT_INT_H */
