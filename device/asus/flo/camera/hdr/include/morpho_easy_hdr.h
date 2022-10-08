/*******************************************************************
 * morpho_easy_hdr.h
 * [CP932/CRLF] { あ 符号化方式自動判定用 }
 *------------------------------------------------------------------
 * Copyright (C) 2010-2012 Morpho,Inc.
 *******************************************************************/

#ifndef MORPHO_EASY_HDR_H
#define MORPHO_EASY_HDR_H

/*******************************************************************/

#include "morpho_api.h"
#include "morpho_error.h"
#include "morpho_image_data.h"
#include "morpho_rect_int.h"

/*******************************************************************/

#define MORPHO_EASY_HDR_VER "Morpho EasyHDR Ver.2.0.1 2012/07/18"

/*-----------------------------------------------------------------*/

/* (input-limitaion) */

#define MORPHO_EASY_HDR_MIN_IMAGE_WIDTH    100
#define MORPHO_EASY_HDR_MAX_IMAGE_WIDTH   8192
#define MORPHO_EASY_HDR_MIN_IMAGE_HEIGHT   100
#define MORPHO_EASY_HDR_MAX_IMAGE_HEIGHT  8192
#define MORPHO_EASY_HDR_MIN_NIMAGES   2
#define MORPHO_EASY_HDR_MAX_NIMAGES  10

/*-----------------------------------------------------------------*/

/* (parameter) */

#define MORPHO_EASY_HDR_DISABLED 0
#define MORPHO_EASY_HDR_ENABLED  1

#define MORPHO_EASY_HDR_IMAGE_ALIGNMENT_DEFAULT  MORPHO_EASY_HDR_ENABLED

#define MORPHO_EASY_HDR_GHOST_REMOVAL_DEFAULT  MORPHO_EASY_HDR_ENABLED

#define MORPHO_EASY_HDR_AUTO_SCALING_DEFAULT  MORPHO_EASY_HDR_ENABLED

#define MORPHO_EASY_HDR_FACE_DETECTION_DEFAULT  MORPHO_EASY_HDR_ENABLED

#define MORPHO_EASY_HDR_FAIL_SOFT_MERGING_DEFAULT  MORPHO_EASY_HDR_ENABLED

#define MORPHO_EASY_HDR_GHOST_DETECTION_SENSITIVITY_LEVEL_MIN      0
#define MORPHO_EASY_HDR_GHOST_DETECTION_SENSITIVITY_LEVEL_MAX     10
#define MORPHO_EASY_HDR_GHOST_DETECTION_SENSITIVITY_LEVEL_DEFAULT  7

#define MORPHO_EASY_HDR_MERGE_SMOOTHNESS_LEVEL_MIN      0
#define MORPHO_EASY_HDR_MERGE_SMOOTHNESS_LEVEL_MAX     10
#define MORPHO_EASY_HDR_MERGE_SMOOTHNESS_LEVEL_DEFAULT  6

#define MORPHO_EASY_HDR_MERGE_PARAM_MIN        0
#define MORPHO_EASY_HDR_MERGE_PARAM_MAX      255
#define MORPHO_EASY_HDR_MERGE_PARAM1_DEFAULT   0
#define MORPHO_EASY_HDR_MERGE_PARAM2_DEFAULT 128
#define MORPHO_EASY_HDR_MERGE_PARAM3_DEFAULT   0
#define MORPHO_EASY_HDR_MERGE_PARAM4_DEFAULT 255

#define MORPHO_EASY_HDR_RELIABLE_RECT_RATE_THRESHOLD_MIN       0
#define MORPHO_EASY_HDR_RELIABLE_RECT_RATE_THRESHOLD_MAX     100
#define MORPHO_EASY_HDR_RELIABLE_RECT_RATE_THRESHOLD_DEFAULT  80

#define MORPHO_EASY_HDR_GHOST_RATE_THRESHOLD_MIN       0
#define MORPHO_EASY_HDR_GHOST_RATE_THRESHOLD_MAX     100
#define MORPHO_EASY_HDR_GHOST_RATE_THRESHOLD_DEFAULT  90

#define MORPHO_EASY_HDR_CC_OFFSET_MIN          0
#define MORPHO_EASY_HDR_CC_OFFSET_MAX        255
#define MORPHO_EASY_HDR_CC_Y_OFFSET_DEFAULT    0
#define MORPHO_EASY_HDR_CC_C_OFFSET_DEFAULT    0

#define MORPHO_EASY_HDR_CC_GAIN_MIN        100
#define MORPHO_EASY_HDR_CC_GAIN_MAX       2000
#define MORPHO_EASY_HDR_CC_Y_GAIN_DEFAULT 1000
#define MORPHO_EASY_HDR_CC_C_GAIN_DEFAULT 1000

#define MORPHO_EASY_HDR_CC_GAMMA_MIN        100
#define MORPHO_EASY_HDR_CC_GAMMA_MAX       2000
#define MORPHO_EASY_HDR_CC_Y_GAMMA_DEFAULT 1000
#define MORPHO_EASY_HDR_CC_C_GAMMA_DEFAULT 1000

/*-----------------------------------------------------------------*/

/* (merge-status) */

#define MORPHO_EASY_HDR_OK                             0x00000000
#define MORPHO_EASY_HDR_ERROR_IMAGE_ALIGNMENT_FAILURE  0x00000001
#define MORPHO_EASY_HDR_ERROR_EXP_ESTIMATION_FAILURE   0x00000002
#define MORPHO_EASY_HDR_ERROR_MOSTLY_GHOST             0x00000004
#define MORPHO_EASY_HDR_ERROR_INTERNAL                 0x80000000

/*******************************************************************/

typedef struct _morpho_EasyHDR morpho_EasyHDR;
typedef struct _morpho_EasyHDR_Callback morpho_EasyHDR_Callback;

/*-----------------------------------------------------------------*/

/** EasyHDR */
struct _morpho_EasyHDR
{
    void *p; /**< 内部構造体へのポインタ */
};

/** EasyHDR Callback (for multi-thread processing) */
struct _morpho_EasyHDR_Callback
{
    void *p; /**< コールバック関数の第一引数として渡される値 */

    void * (* thread_create )(void *p, int index, void *(*start_routine)(void *arg), void *arg);
    int    (* thread_destroy)(void *p, void *thread);
    int    (* thread_join   )(void *p, void *thread, void **value_ptr);

    void * (* mutex_create )(void *p);
    int    (* mutex_destroy)(void *p, void *mutex);
    int    (* mutex_lock   )(void *p, void *mutex);
    int    (* mutex_trylock)(void *p, void *mutex);
    int    (* mutex_unlock )(void *p, void *mutex);

    void * (* cond_create   )(void *p);
    int    (* cond_destroy  )(void *p, void *cond);
    int    (* cond_wait     )(void *p, void *cond, void *lock);
    int    (* cond_signal   )(void *p, void *cond);
    int    (* cond_broadcast)(void *p, void *cond);
};

/*******************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif

/*-----------------------------------------------------------------*/

/**
 * バージョン文字列を取得
 *
 * @return バージョン文字列(MORPHO_EASY_HDR_VER)
 */
MORPHO_API(char const *)
morpho_EasyHDR_getVersion(void);

/*-----------------------------------------------------------------*/

/**
 * 必要なメモリサイズを取得
 *
 * @param[in]  max_width   入力画像の最大幅
 * @param[in]  max_height  入力画像の最大高さ
 * @param[in]  format      画像フォーマット文字列
 *
 * @return 必要なメモリサイズ(byte)
 */
MORPHO_API(int)
morpho_EasyHDR_getBufferSize(
    int max_width,
    int max_height,
    char const *format);

/**
 * 初期化
 *
 * 使用スレッド数に0以下の値を設定した場合、
 * 分割実行を行う。
 *
 * 使用スレッド数に1以上の値を設定した場合、
 * 一括実行を行う。
 *
 * 使用スレッド数に2以上の値を設定した場合、
 * マルチスレッドによる並列実行(一括実行)を行う。
 * callback に適切な値を設定する必要あり。
 *
 * 【実行状態の遷移】
 *     ?_UNKNOWN → 0_INITIALIZED
 *
 * @param[in,out]  p            EasyHDR インスタンス
 * @param[out]     buffer       EasyHDRに割り当てるメモリへのポインタ
 * @param[in]      buffer_size  EasyHDRに割り当てるメモリのサイズ
 * @param[in]      nthreads     使用スレッド数 (コア数)
 * @param[in]      callback     コールバック関数群
 *
 * @return エラーコード (see morpho_error.h)
 */
MORPHO_API(int)
morpho_EasyHDR_initialize(
    morpho_EasyHDR *p,
    void *buffer,
    int buffer_size,
    int nthreads,
    morpho_EasyHDR_Callback const *callback);

/**
 * クリーンアップ
 * initialize() 実行後に実行可能
 *
 * @param[in,out]  p  EasyHDR インスタンス
 *
 * @return エラーコード (see morpho_error.h)
 */
MORPHO_API(int)
morpho_EasyHDR_finalize(
    morpho_EasyHDR *p);

/*-----------------------------------------------------------------*/

/**
 * 合成の開始・実行
 * setImageFormat() 実行後に実行可能
 *
 * 【実行状態の遷移 (一括実行時)】
 *     0_INITIALIZED → (1_PROCESSING) → 0_INITIALIZED (処理完了)
 *                                     → 2_SUSPENDED   (suspend()呼び出し)
 *
 * 【実行状態の遷移 (分割実行時)】
 *     0_INITIALIZED → 3_PAUSED      (処理中)
 *                   → 0_INITIALIZED (処理完了)
 *
 * @param[in,out]  p             EasyHDR インスタンス
 * @param[out]     output_image  結果画像 (「1枚目」の入力画像を指定可能)
 * @param[in,out]  input_images  入力画像群 (エンジンによって書き換えられる)
 * @param[in]      nimages       入力画像の数
 *
 * @return エラーコード (see morpho_error.h)
 */
MORPHO_API(int)
morpho_EasyHDR_merge(
    morpho_EasyHDR *p,
    morpho_ImageData *output_image,
    morpho_ImageData *input_images[],
    int nimages);

/*-----------------------------------------------------------------*/

/**
 * 合成の継続実行
 *
 * merge() 実行後に実行可能
 *
 * 分割実行時(initialize() で nthreads に 0 を指定したとき)のみ有効
 *
 * 【実行状態の遷移 (分割実行時)】
 *     3_PAUSED → 3_PAUSED      (処理中)
 *              → 0_INITIALIZED (処理完了)
 *
 * @param[in,out]  p  EasyHDR インスタンス
 *
 * @return エラーコード (see morpho_error.h)
 */
MORPHO_API(int)
morpho_EasyHDR_process(
    morpho_EasyHDR *p);

/*-----------------------------------------------------------------*/

/**
 * 合成の中断 (別コンテキストからの呼び出しによる)
 * merge() 実行中に実行可能
 *
 * 【実行状態の遷移 (一括実行時)】
 *     1_PROCESSING → 2_SUSPENDED
 *
 * @param[in,out]  p  EasyHDR インスタンス
 *
 * @return エラーコード (see morpho_error.h)
 */
MORPHO_API(int)
morpho_EasyHDR_suspend(
    morpho_EasyHDR *p);

/**
 * 合成の再開
 * suspend() 実行後に実行可能
 *
 * 【実行状態の遷移 (一括実行時)】
 *     2_SUSPENDED → (1_PROCESSING) → 0_INITIALIZED (処理完了)
 *                                   → 2_SUSPENDED   (suspend()呼び出し)
 *
 * @param[in,out]  p  EasyHDR インスタンス
 *
 * @return エラーコード (see morpho_error.h)
 */
MORPHO_API(int)
morpho_EasyHDR_resume(
    morpho_EasyHDR *p);

/*-----------------------------------------------------------------*/

/**
 * 画像フォーマットの設定
 * initialize() 実行後に実行可能
 *
 * @param[in,out]  p       EasyHDR インスタンス
 * @param[in]      format  画像フォーマットをあらわす文字列
 *
 * @return エラーコード (see morpho_error.h)
 */
MORPHO_API(int)
morpho_EasyHDR_setImageFormat(
    morpho_EasyHDR *p,
    char const *format);

/**
 * 画像フォーマットの取得
 * setImageFormat() 実行後に実行可能
 *
 * @param[in,out]  p            EasyHDR インスタンス
 * @param[out]     buffer       画像フォーマットをあらわす文字列が格納されるバッファ
 * @param[in]      buffer_size  バッファのサイズ(終端文字含む)
 *
 * @return エラーコード (see morpho_error.h)
 */
MORPHO_API(int)
morpho_EasyHDR_getImageFormat(
    morpho_EasyHDR *p,
    char *buffer,
    int buffer_size);

/*-----------------------------------------------------------------*/

/**
 * 位置合わせ(手ぶれ補正)の有無の設定
 * initialize() 実行後に実行可能
 *
 * value:
 *   MOR_EASY_HDR_ENABLED  : 位置合わせあり
 *   MOR_EASY_HDR_DISABLED : 位置合わせなし
 *
 * @param[in,out]  p      EasyHDR インスタンス
 * @param[in]      value  設定値
 *
 * @return エラーコード (see morpho_error.h)
 */
MORPHO_API(int)
morpho_EasyHDR_setImageAlignmentStatus(
    morpho_EasyHDR *p,
    int value);

/**
 * 位置合わせ(手ぶれ補正)の有無の取得
 * initialize() 実行後に実行可能
 *
 * @param[in,out]  p      EasyHDR インスタンス
 * @param[out]     value  設定値格納先
 *
 * @return エラーコード (see morpho_error.h)
 */
MORPHO_API(int)
morpho_EasyHDR_getImageAlignmentStatus(
    morpho_EasyHDR *p,
    int *value);

/*-----------------------------------------------------------------*/

/**
 * ゴースト除去(被写体ぶれ補正)の有無の設定
 * initialize() 実行後に実行可能
 *
 * value:
 *   MOR_EASY_HDR_ENABLED  : ゴースト除去あり
 *   MOR_EASY_HDR_DISABLED : ゴースト除去なし
 *
 * @param[in,out]  p      EasyHDR インスタンス
 * @param[in]      value  設定値
 *
 * @return エラーコード (see morpho_error.h)
 */
MORPHO_API(int)
morpho_EasyHDR_setGhostRemovalStatus(
    morpho_EasyHDR *p,
    int value);

/**
 * ゴースト除去(被写体ぶれ補正)の有無の取得
 * initialize() 実行後に実行可能
 *
 * @param[in,out]  p      EasyHDR インスタンス
 * @param[out]     value  設定値格納先
 *
 * @return エラーコード (see morpho_error.h)
 */
MORPHO_API(int)
morpho_EasyHDR_getGhostRemovalStatus(
    morpho_EasyHDR *p,
    int *value);

/*-----------------------------------------------------------------*/

/**
 * 自動拡大(クリッピング)の有無の設定
 * initialize() 実行後に実行可能
 *
 * value:
 *   MOR_EASY_HDR_ENABLED  : 自動拡大あり
 *   MOR_EASY_HDR_DISABLED : 自動拡大なし
 *
 * @param[in,out]  p      EasyHDR インスタンス
 * @param[in]      value  設定値
 *
 * @return エラーコード (see morpho_error.h)
 */
MORPHO_API(int)
morpho_EasyHDR_setAutoScalingStatus(
    morpho_EasyHDR *p,
    int value);

/**
 * 自動拡大(クリッピング)の有無の取得
 * initialize() 実行後に実行可能
 *
 * @param[in,out]  p      EasyHDR インスタンス
 * @param[out]     value  設定値格納先
 *
 * @return エラーコード (see morpho_error.h)
 */
MORPHO_API(int)
morpho_EasyHDR_getAutoScalingStatus(
    morpho_EasyHDR *p,
    int *value);

/*-----------------------------------------------------------------*/

/**
 * 顔検出補正の有無の設定
 * initialize() 実行後に実行可能
 *
 * value:
 *   MOR_EASY_HDR_ENABLED  : 顔検出補正あり
 *   MOR_EASY_HDR_DISABLED : 顔検出補正なし
 *
 * @param[in,out]  p      EasyHDR インスタンス
 * @param[in]      value  設定値
 *
 * @return エラーコード (see morpho_error.h)
 */
MORPHO_API(int)
morpho_EasyHDR_setFaceDetectionStatus(
    morpho_EasyHDR *p,
    int value);

/**
 * 顔検出補正の有無の取得
 * initialize() 実行後に実行可能
 *
 * @param[in,out]  p      EasyHDR インスタンス
 * @param[out]     value  設定値格納先
 *
 * @return エラーコード (see morpho_error.h)
 */
MORPHO_API(int)
morpho_EasyHDR_getFaceDetectionStatus(
    morpho_EasyHDR *p,
    int *value);

/*-----------------------------------------------------------------*/

/**
 * Fail-soft-merging の有無の設定
 * initialize() 実行後に実行可能
 *
 * value:
 *   MOR_EASY_HDR_ENABLED  : Fail-soft-merging あり
 *   MOR_EASY_HDR_DISABLED : Fail-soft-merging なし
 *
 * @param[in,out]  p      EasyHDR インスタンス
 * @param[in]      value  設定値
 *
 * @return エラーコード (see morpho_error.h)
 */
MORPHO_API(int)
morpho_EasyHDR_setFailSoftMergingStatus(
    morpho_EasyHDR *p,
    int value);

/**
 * Fail-soft-merging の有無の取得
 * initialize() 実行後に実行可能
 *
 * @param[in,out]  p      EasyHDR インスタンス
 * @param[out]     value  設定値格納先
 *
 * @return エラーコード (see morpho_error.h)
 */
MORPHO_API(int)
morpho_EasyHDR_getFailSoftMergingStatus(
    morpho_EasyHDR *p,
    int *value);

/*-----------------------------------------------------------------*/

/**
 * ゴースト判定感度レベルの設定
 * initialize() 実行後に実行可能
 *
 * @param[in,out]  p      EasyHDR インスタンス
 * @param[in]      value  設定値
 *
 * @return エラーコード (see morpho_error.h)
 */
MORPHO_API(int)
morpho_EasyHDR_setGhostDetectionSensitivityLevel(
    morpho_EasyHDR *p,
    int value);

/**
 * ゴースト判定感度レベルの取得
 * initialize() 実行後に実行可能
 *
 * @param[in,out]  p      EasyHDR インスタンス
 * @param[out]     value  設定値格納先
 *
 * @return エラーコード (see morpho_error.h)
 */
MORPHO_API(int)
morpho_EasyHDR_getGhostDetectionSensitivityLevel(
    morpho_EasyHDR *p,
    int *value);

/*-----------------------------------------------------------------*/

/**
 * 合成なめらかさの設定
 * initialize() 実行後に実行可能
 *
 * @param[in,out]  p      EasyHDR インスタンス
 * @param[in]      value  設定値
 *
 * @return エラーコード (see morpho_error.h)
 */
MORPHO_API(int)
morpho_EasyHDR_setMergeSmoothnessLevel(
    morpho_EasyHDR *p,
    int value);

/**
 * 合成なめらかさの取得
 * initialize() 実行後に実行可能
 *
 * @param[in,out]  p      EasyHDR インスタンス
 * @param[out]     value  設定値格納先
 *
 * @return エラーコード (see morpho_error.h)
 */
MORPHO_API(int)
morpho_EasyHDR_getMergeSmoothnessLevel(
    morpho_EasyHDR *p,
    int *value);

/*-----------------------------------------------------------------*/

/**
 * 合成パラメータの設定
 * initialize() 実行後に実行可能
 *
 * @param[in,out]  p       EasyHDR インスタンス
 * @param[in]      value1  設定値
 * @param[in]      value2  設定値
 * @param[in]      value3  設定値
 * @param[in]      value4  設定値
 *
 * @return エラーコード (see morpho_error.h)
 */
MORPHO_API(int)
morpho_EasyHDR_setMergeParameters(
    morpho_EasyHDR *p,
    int value1,
    int value2,
    int value3,
    int value4);

/**
 * 合成パラメータの取得
 * initialize() 実行後に実行可能
 *
 * @param[in,out]  p       EasyHDR インスタンス
 * @param[out]     value1  設定値格納先
 * @param[out]     value2  設定値格納先
 * @param[out]     value3  設定値格納先
 * @param[out]     value4  設定値格納先
 *
 * @return エラーコード (see morpho_error.h)
 */
MORPHO_API(int)
morpho_EasyHDR_getMergeParameters(
    morpho_EasyHDR *p,
    int *value1,
    int *value2,
    int *value3,
    int *value4);

/*-----------------------------------------------------------------*/

/**
 * 有効領域閾値の設定
 * initialize() 実行後に実行可能
 *
 * @param[in,out]  p     EasyHDR インスタンス
 * @param[in]      rate  設定値 (中央 rate % 矩形)
 *
 * @return エラーコード (see morpho_error.h)
 */
MORPHO_API(int)
morpho_EasyHDR_setReliableRectRateThreshold(
    morpho_EasyHDR *p,
    int rate);

/**
 * 有効領域閾値の取得
 * initialize() 実行後に実行可能
 *
 * @param[in,out]  p     EasyHDR インスタンス
 * @param[out]     rate  設定値格納先
 *
 * @return エラーコード (see morpho_error.h)
 */
MORPHO_API(int)
morpho_EasyHDR_getReliableRectRateThreshold(
    morpho_EasyHDR *p,
    int *rate);

/**
 * 有効領域の取得
 * initialize() 実行後に実行可能
 * (有効な値がセットされるのは merge() 後)
 *
 * @param[in,out]  p     EasyHDR インスタンス
 * @param[out]     rect  設定値格納先
 *
 * @return エラーコード (see morpho_error.h)
 */
MORPHO_API(int)
morpho_EasyHDR_getReliableRect(
    morpho_EasyHDR *p,
    morpho_RectInt *rect);

/*-----------------------------------------------------------------*/

/**
 * ゴースト割合閾値の設定
 * initialize() 実行後に実行可能
 *
 * @param[in,out]  p     EasyHDR インスタンス
 * @param[in]      rate  設定値
 *
 * @return エラーコード (see morpho_error.h)
 */
MORPHO_API(int)
morpho_EasyHDR_setGhostRateThreshold(
    morpho_EasyHDR *p,
    int rate);

/**
 * ゴースト割合閾値の取得
 * initialize() 実行後に実行可能
 *
 * @param[in,out]  p     EasyHDR インスタンス
 * @param[out]     rate  設定値格納先
 *
 * @return エラーコード (see morpho_error.h)
 */
MORPHO_API(int)
morpho_EasyHDR_getGhostRateThreshold(
    morpho_EasyHDR *p,
    int *rate);

/*-----------------------------------------------------------------*/

/**
 * 色補正パラメータの設定
 * initialize() 実行後に実行可能
 *
 * @param[in,out]  p         EasyHDR インスタンス
 * @param[in]      y_offset  設定値
 * @param[in]      y_gain    設定値
 * @param[in]      y_gamma   設定値
 * @param[in]      c_offset  設定値
 * @param[in]      c_gain    設定値
 * @param[in]      c_gamma   設定値
 *
 * @return エラーコード (see morpho_error.h)
 */
MORPHO_API(int)
morpho_EasyHDR_setColorCorrectionParameters(
    morpho_EasyHDR *p,
    int y_offset,
    int y_gain,
    int y_gamma,
    int c_offset,
    int c_gain,
    int c_gamma);

/**
 * 色補正パラメータの取得
 * initialize() 実行後に実行可能
 *
 * @param[in,out]  p         EasyHDR インスタンス
 * @param[out]     y_offset  設定値格納先
 * @param[out]     y_gain    設定値格納先
 * @param[out]     y_gamma   設定値格納先
 * @param[out]     c_offset  設定値格納先
 * @param[out]     c_gain    設定値格納先
 * @param[out]     c_gamma   設定値格納先
 *
 * @return エラーコード (see morpho_error.h)
 */
MORPHO_API(int)
morpho_EasyHDR_getColorCorrectionParameters(
    morpho_EasyHDR *p,
    int *y_offset,
    int *y_gain,
    int *y_gamma,
    int *c_offset,
    int *c_gain,
    int *c_gamma);

/*-----------------------------------------------------------------*/

/**
 * 合成ステータスの取得
 * initialize() 実行後に実行可能
 *
 * ステータスコード
 *   MORPHO_EASY_HDR_OK
 *   MORPHO_EASY_HDR_ERROR_*
 *
 * @param[in,out]  p  EasyHDR インスタンス
 *
 * @return ステータスコード (MORPHO_EASMORPHO_EASY_HDR_ERROR_
 */
MORPHO_API(int)
morpho_EasyHDR_getMergeStatus(
    morpho_EasyHDR *p);

/*-----------------------------------------------------------------*/

/**
 * サムネイルの作成 (出力画像の縮小)
 * morpho_EasyHDR_setImageFormat() 実行後に実行可能
 *
 * @param[in,out]  p                EasyHDR インスタンス
 * @param[out]     thumbnail_image  出力画像
 * @param[in]      output_image     入力画像
 *
 * @return エラーコード (see morpho_error.h)
 */
MORPHO_API(int)
morpho_EasyHDR_makeThumbnail(
    morpho_EasyHDR *p,
    morpho_ImageData *thumbnail_image,
    morpho_ImageData const *output_image);

/*-----------------------------------------------------------------*/

#ifdef __cplusplus
} /* extern "C" */
#endif

/*******************************************************************/

#endif /* !MORPHO_EASY_HDR_H */

/*******************************************************************/
/* [EOF] */
