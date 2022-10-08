/*
 $License:
   Copyright 2011 InvenSense, Inc.

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
  $
 */
#ifndef INVENSENSE_INV_MATH_FUNC_H__
#define INVENSENSE_INV_MATH_FUNC_H__
#ifdef INV_INCLUDE_LEGACY_HEADERS
#include "mlMathFunc_legacy.h"
#endif

#define NUM_ROTATION_MATRIX_ELEMENTS (9)
#define ROT_MATRIX_SCALE_LONG  (1073741824)
#define ROT_MATRIX_SCALE_FLOAT (1073741824.0f)
#define ROT_MATRIX_LONG_TO_FLOAT( longval ) \
    ((float) ((longval) / ROT_MATRIX_SCALE_FLOAT ))
#define SIGNM(k)((int)(k)&1?-1:1)

#ifdef __cplusplus
extern "C" {
#endif
    struct filter_long {
        int length;
        const long *b;
        const long *a;
        long *x;
        long *y;
    };

    void inv_filter_long(struct filter_long *state, long x);
    long inv_q29_mult(long a, long b);
    long inv_q30_mult(long a, long b);
    void inv_q_mult(const long *q1, const long *q2, long *qProd);
    void inv_q_add(long *q1, long *q2, long *qSum);
    void inv_q_normalize(long *q);
    void inv_q_invert(const long *q, long *qInverted);
    void inv_q_multf(const float *q1, const float *q2, float *qProd);
    void inv_q_addf(float *q1, float *q2, float *qSum);
    void inv_q_normalizef(float *q);
    void inv_q_norm4(float *q);
    void inv_q_invertf(const float *q, float *qInverted);
    void inv_quaternion_to_rotation(const long *quat, long *rot);
    unsigned char *inv_int32_to_big8(long x, unsigned char *big8);
    long inv_big8_to_int32(const unsigned char *big8);
    unsigned char *inv_int16_to_big8(short x, unsigned char *big8);
    float inv_matrix_det(float *p, int *n);
    void inv_matrix_det_inc(float *a, float *b, int *n, int x, int y);
    double inv_matrix_detd(double *p, int *n);
    void inv_matrix_det_incd(double *a, double *b, int *n, int x, int y);
    float inv_wrap_angle(float ang);
    float inv_angle_diff(float ang1, float ang2);

#ifdef __cplusplus
}
#endif
#endif                          // INVENSENSE_INV_MATH_FUNC_H__
