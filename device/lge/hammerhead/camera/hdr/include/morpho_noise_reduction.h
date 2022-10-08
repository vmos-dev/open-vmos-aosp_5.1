//====================================================================
// morpho_noise_reduction.h
// [SJIS/CRLF] { あ 符号化方式自動判定用 }
//
// Copyright(c) 2006-2012 Morpho,Inc.
//====================================================================

#ifndef MORPHO_NOISE_REDUCTION_H
# define MORPHO_NOISE_REDUCTION_H

//--------------------------------------------------------------------

# include "morpho_api.h"
# include "morpho_error.h"
# include "morpho_image_data.h"
# include "morpho_motion_data.h"
# include "morpho_rect_int.h"

//--------------------------------------------------------------------

# ifdef __cplusplus
extern "C" {
# endif

//====================================================================

/** バージョン文字列 */
# define MORPHO_NOISE_REDUCTION_VERSION "Morpho Noise Reduction Ver.0.9.0 2012/08/09"

//--------------------------------------------------------------------
/** ノイズ除去器 */
typedef struct
{
    void *p; /**< 内部構造体へのポインタ */
} morpho_NoiseReduction;

//--------------------------------------------------------------------

/**
 * バージョン文字列を取得
 *
 * @return バージョン文字列(MORPHO_IMAGE_STABILIZER_VERSION)
 */
MORPHO_API(const char *)
morpho_NoiseReduction_getVersion(void);

/**
 * ノイズ除去処理に必要なメモリサイズを取得
 * 指定できるフォーマットはTRMを参照。
 *
 * @param[in] width  入力画像の幅
 * @param[in] height 入力画像の高さ
 * @param[in] format 画像フォーマット文字列
 * @return 必要なメモリサイズ(byte)
 */
MORPHO_API(int)
morpho_NoiseReduction_getBufferSize(
    int width,
    int height,
    const char *format);

/**
 * ノイズ除去器の初期化
 *
 * @param[out] reducer  ノイズ除去器
 * @param[out] buffer      ノイズ除去器に割り当てるメモリへのポインタ
 * @param[in]  buffer_size ノイズ除去器に割り当てるメモリのサイズ.
 * @return エラーコード(morpho_error.h)
 */
MORPHO_API(int)
morpho_NoiseReduction_initialize(
    morpho_NoiseReduction *reducer,
    void *buffer,
    int buffer_size);

/**
 * ノイズ除去器のクリーンアップ
 *
 * @param[in,out] reducer ノイズ除去器
 * @return エラーコード(morpho_error.h)
 */
MORPHO_API(int)
morpho_NoiseReduction_finalize(
    morpho_NoiseReduction *reducer);

/**
 * ノイズ除去処理: 処理開始
 * 出力画像(output_image)は1枚目の入力画像と同じでも良い
 *
 * @param[in,out] reducer    ノイズ除去器
 * @param[out]    output_image  出力画像
 * @return エラーコード(morpho_error.h)
 */
MORPHO_API(int)
morpho_NoiseReduction_start(
    morpho_NoiseReduction *reducer,
    morpho_ImageData *output_image);

/**
 * ノイズ除去処理: ノイズ除去
 *
 * @param[in,out] reducer   ノイズ除去器
 * @param[out]    input_image  出力画像
 * @return エラーコード(morpho_error.h)
 */
MORPHO_API(int)
morpho_NoiseReduction_reduceNoise(
    morpho_NoiseReduction *reducer,
    morpho_ImageData *input_image);

/**
 * 画像フォーマットを取得
 * initialize()実行後に取得可能
 * バッファサイズは32以上とすること
 *
 * @param[in,out] reducer ノイズ除去器
 * @param[out] format 画像フォーマット文字列が格納される
 * @param[in] buffer_size バッファサイズ
 * @return エラーコード(morpho_error.h)
 */
MORPHO_API(int)
morpho_NoiseReduction_getImageFormat(
    morpho_NoiseReduction *reducer,
    char *format,
    const int buffer_size);

/**
 * 輝度ノイズ除去強度レベルを取得
 * initialize()実行後に取得可能
 *
 * @param[in,out] reducer ノイズ除去器
 * @param[out] level 輝度ノイズ除去強度レベルが格納される
 * @return エラーコード(morpho_error.h)
 */
MORPHO_API(int)
morpho_NoiseReduction_getLumaNoiseReductionLevel(
    morpho_NoiseReduction *reducer,
    int *level);

/**
 * クロマノイズ除去強度レベルを取得
 * initialize()実行後に取得可能
 *
 * @param[in,out] reducer ノイズ除去器
 * @param[out] level クロマノイズ除去強度レベルが格納される
 * @return エラーコード(morpho_error.h)
 */
MORPHO_API(int)
morpho_NoiseReduction_getChromaNoiseReductionLevel(
    morpho_NoiseReduction *reducer,
    int *level);

/**
 * 画像フォーマットを設定
 * initialize()実行後かつstart()実行前に設定可能
 * 指定できるフォーマットはTRMを参照。
 *
 * @param[in,out] reducer ノイズ除去器
 * @param[in] format 画像フォーマット文字列
 * @return エラーコード(morpho_error.h)
 */
MORPHO_API(int)
morpho_NoiseReduction_setImageFormat(
    morpho_NoiseReduction *reducer,
    const char *format);

/**
 * 輝度ノイズ除去強度レベルを設定
 * initialize()実行後かつstart()実行前に設定可能
 *
 * @param[in,out] reducer ノイズ除去器
 * @param[in] level 輝度ノイズ除去強度レベル(0-7)
 * @return エラーコード(morpho_error.h)
 */
MORPHO_API(int)
morpho_NoiseReduction_setLumaNoiseReductionLevel(
    morpho_NoiseReduction *reducer,
    int level);

/**
 * クロマノイズ除去強度レベルを設定
 * initialize()実行後かつstart()実行前に設定可能
 *
 * @param[in,out] reducer ノイズ除去器
 * @param[in] level クロマノイズ除去強度レベル(0-7)
 * @return エラーコード(morpho_error.h)
 */
MORPHO_API(int)
morpho_NoiseReduction_setChromaNoiseReductionLevel(
    morpho_NoiseReduction *reducer,
    int level);

//====================================================================

# ifdef __cplusplus
} // extern "C"
# endif

//--------------------------------------------------------------------

#endif // !MORPHO_IMAGE_STABILIZER3_H

//====================================================================
// [EOF]
