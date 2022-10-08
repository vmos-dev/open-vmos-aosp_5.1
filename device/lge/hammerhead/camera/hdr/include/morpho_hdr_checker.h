/*******************************************************************
 * morpho_hdr_checker.h
 * [CP932/CRLF] { あ 符号化方式自動判定用 }
 *------------------------------------------------------------------
 * Copyright (C) 2011-2012 Morpho,Inc.
 *******************************************************************/

#ifndef MORPHO_HDR_CHECKER_H
#define MORPHO_HDR_CHECKER_H

/*******************************************************************/

#include "morpho_api.h"
#include "morpho_error.h"
#include "morpho_image_data.h"

/*******************************************************************/

#define MORPHO_HDR_CHECKER_VER "Morpho DR Checker Ver.1.1.0 2012/1/17"

/*-----------------------------------------------------------------*/

#define MORPHO_HDR_CHECKER_MIN_IMAGE_WIDTH     2
#define MORPHO_HDR_CHECKER_MAX_IMAGE_WIDTH  8192
#define MORPHO_HDR_CHECKER_MIN_IMAGE_HEIGHT    2
#define MORPHO_HDR_CHECKER_MAX_IMAGE_HEIGHT 8192

/*******************************************************************/

typedef struct _morpho_HDRChecker morpho_HDRChecker;

/* HDR指標評価器 */
struct _morpho_HDRChecker
{
    void *p; /**< 内部構造体へのポインタ */
};

/* 白飛び・黒つぶれ判定の敏感度 */
typedef enum {
    MORPHO_HDR_CHECKER_SENSITIVITY_SENSITIVE,
    MORPHO_HDR_CHECKER_SENSITIVITY_NORMAL,
    MORPHO_HDR_CHECKER_SENSITIVITY_INSENSITIVE,
} MORPHO_HDR_CHECKER_SENSITIVITY;

/*******************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/**
 * バージョン文字列を取得
 *
 * @return バージョン文字列(MORPHO_EASY_HDR_VER)
 */
MORPHO_API(const char*)
morpho_HDRChecker_getVersion(void);

/**
 * 必要なメモリサイズを取得
 *
 * @param[in] width  入力画像の幅
 * @param[in] height 入力画像の高さ
 * @param[in] format 入力画像のフォーマット
 * @return 必要なメモリサイズ(byte)
 */
MORPHO_API(int)
morpho_HDRChecker_getBufferSize(
    int width,
    int height,
    const char *format);

/**
 * 初期化
 *
 * @param[in,out] p           HDRCheckerインスタンス
 * @param[in]     buffer      HDRCheckerに割り当てるメモリへのポインタ
 * @param[in]     buffer_size HDRCheckerに割り当てるメモリのサイズ
 * @param[in]     width       入力画像の幅
 * @param[in]     height      入力画像の高さ
 * @param[in]     format      入力画像のフォーマット
 * @return エラーコード (see morpho_error.h)
 */
MORPHO_API(int)
morpho_HDRChecker_initialize(
    morpho_HDRChecker * const p,
    void * const buffer,
    const int buffer_size,
    const int width,
    const int height,
    const char *format);

/**
 * クリーンアップ
 * initialize()実行後に実行可能
 *
 * @param[in,out] p HDRCheckerインスタンス
 * @return エラーコード (see morpho_error.h)
 */
MORPHO_API(int)
morpho_HDRChecker_finalize(
    morpho_HDRChecker *p);

/*-----------------------------------------------------------------*/

/**
 * HDR指標計算の敏感性の設定
 * initialize()実行後に実行可能
 *
 * @param[in,out] p           HDRCheckerインスタンス
 * @param[in]     sensitivity 敏感性(MORPHO_HDR_CHECKER_SENSITIVIY列挙体で指定)
 * @return エラーコード (see morpho_error.h)
 */
MORPHO_API(int)
morpho_HDRChecker_setSensitivity(
    morpho_HDRChecker * const p,
    MORPHO_HDR_CHECKER_SENSITIVITY sensitivity);

/**
 * HDR指標計算の敏感性の取得
 * initialize()実行後に実行可能
 *
 * @param[in,out] p           HDRCheckerインスタンス
 * @param[out]    sensitivity 敏感性へのポインタ
 * @return エラーコード (see morpho_error.h)
 */
MORPHO_API(int)
morpho_HDRChecker_getSensitivity(
    morpho_HDRChecker * const p,
    MORPHO_HDR_CHECKER_SENSITIVITY *sensitivity);

/**
 * HDR指標の評価
 * initialize()実行後に実行可能
 *
 * @param[in,out] p      HDRCheckerインスタンス
 * @param[out]    result 評価結果を格納する配列(要素数4の配列)
 *                       要素が非ゼロの場合に対応する下記の露出の画像が必要と判定
 *                       {+2, +1, -1, -2}の順に判定結果が格納される
 * @param[in]     input_image 入力画像
 * @return エラーコード (see morpho_error.h)
 */
MORPHO_API(int)
morpho_HDRChecker_evaluate(
    morpho_HDRChecker * const p,
    int * const result,
    const morpho_ImageData * const input_image);

/*-----------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* MORPHO_HDR_CHECKER_H */
