/**
 * @file     morpho_error.h
 * @brief    エラーコードの定義
 * @version  1.0.0
 * @date     2008-06-09
 *
 * Copyright (C) 2006-2012 Morpho, Inc.
 */

#ifndef MORPHO_ERROR_H
#define MORPHO_ERROR_H

/** エラーコード .*/
#define MORPHO_OK                   (0x00000000)  /**< 成功 */
#define MORPHO_DOPROCESS            (0x00000001)  /**< 処理中 */
#define MORPHO_CANCELED             (0x00000002)  /**< キャンセルされた */
#define MORPHO_SUSPENDED            (0x00000008)  /**< 中断された */

#define MORPHO_ERROR_GENERAL_ERROR  (0x80000000)  /**< 一般的なエラー. */
#define MORPHO_ERROR_PARAM          (0x80000001)  /**< 引数が不正. */
#define MORPHO_ERROR_STATE          (0x80000002)  /**< 内部状態や関数呼出順序が不正. */
#define MORPHO_ERROR_MALLOC         (0x80000004)  /**< メモリアロケーションエラー. */
#define MORPHO_ERROR_IO             (0x80000008)  /**< 入出力エラー. */
#define MORPHO_ERROR_UNSUPPORTED    (0x80000010)  /**< 機能をサポートしていない. */
#define MORPHO_ERROR_NOTFOUND       (0x80000020)  /**< 検索対象が見つからない */
#define MORPHO_ERROR_INTERNAL       (0x80000040)  /**< 内部エラー. */
#define MORPHO_ERROR_UNKNOWN        (0xC0000000)  /**< 上記以外のエラー. */

#endif /* #ifndef MORPHO_ERROR_H */
