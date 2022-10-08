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
#include "mlmath.h"
#include "mlMathFunc.h"
#include "mlinclude.h"

/** 
 * Performs one iteration of the filter, generating a new y(0) 
 *         1     / N                /  N             \\
 * y(0) = ---- * |SUM b(k) * x(k) - | SUM a(k) * y(k)|| for N = length
 *        a(0)   \k=0               \ k=1            //
 * 
 * The filters A and B should be (sizeof(long) * state->length).
 * The state variables x and y should be (sizeof(long) * (state->length - 1))
 *
 * The state variables x and y should be initialized prior to the first call
 * 
 * @param state Contains the length of the filter, filter coefficients and
 *              filter state variables x and y.
 * @param x New input into the filter.
 */
void inv_filter_long(struct filter_long *state, long x)
{
    const long *b = state->b;
    const long *a = state->a;
    long length = state->length;
    long long tmp;
    int ii;

    /* filter */
    tmp = (long long)x *(b[0]);
    for (ii = 0; ii < length - 1; ii++) {
        tmp += ((long long)state->x[ii] * (long long)(b[ii + 1]));
    }
    for (ii = 0; ii < length - 1; ii++) {
        tmp -= ((long long)state->y[ii] * (long long)(a[ii + 1]));
    }
    tmp /= (long long)a[0];

    /* Delay */
    for (ii = length - 2; ii > 0; ii--) {
        state->y[ii] = state->y[ii - 1];
        state->x[ii] = state->x[ii - 1];
    }
    /* New values */
    state->y[0] = (long)tmp;
    state->x[0] = x;
}

/** Performs a multiply and shift by 29. These are good functions to write in assembly on
 * with devices with small memory where you want to get rid of the long long which some
 * assemblers don't handle well
 * @param[in] a 
 * @param[in] b
 * @return ((long long)a*b)>>29
*/
long inv_q29_mult(long a, long b)
{
    long long temp;
    long result;
    temp = (long long)a *b;
    result = (long)(temp >> 29);
    return result;
}

/** Performs a multiply and shift by 30. These are good functions to write in assembly on
 * with devices with small memory where you want to get rid of the long long which some
 * assemblers don't handle well
 * @param[in] a 
 * @param[in] b
 * @return ((long long)a*b)>>30
*/
long inv_q30_mult(long a, long b)
{
    long long temp;
    long result;
    temp = (long long)a *b;
    result = (long)(temp >> 30);
    return result;
}

void inv_q_mult(const long *q1, const long *q2, long *qProd)
{
    INVENSENSE_FUNC_START;
    qProd[0] = (long)(((long long)q1[0] * q2[0] - (long long)q1[1] * q2[1] -
                       (long long)q1[2] * q2[2] -
                       (long long)q1[3] * q2[3]) >> 30);
    qProd[1] =
        (int)(((long long)q1[0] * q2[1] + (long long)q1[1] * q2[0] +
               (long long)q1[2] * q2[3] - (long long)q1[3] * q2[2]) >> 30);
    qProd[2] =
        (long)(((long long)q1[0] * q2[2] - (long long)q1[1] * q2[3] +
                (long long)q1[2] * q2[0] + (long long)q1[3] * q2[1]) >> 30);
    qProd[3] =
        (long)(((long long)q1[0] * q2[3] + (long long)q1[1] * q2[2] -
                (long long)q1[2] * q2[1] + (long long)q1[3] * q2[0]) >> 30);
}

void inv_q_add(long *q1, long *q2, long *qSum)
{
    INVENSENSE_FUNC_START;
    qSum[0] = q1[0] + q2[0];
    qSum[1] = q1[1] + q2[1];
    qSum[2] = q1[2] + q2[2];
    qSum[3] = q1[3] + q2[3];
}

void inv_q_normalize(long *q)
{
    INVENSENSE_FUNC_START;
    double normSF = 0;
    int i;
    for (i = 0; i < 4; i++) {
        normSF += ((double)q[i]) / 1073741824L * ((double)q[i]) / 1073741824L;
    }
    if (normSF > 0) {
        normSF = 1 / sqrt(normSF);
        for (i = 0; i < 4; i++) {
            q[i] = (int)((double)q[i] * normSF);
        }
    } else {
        q[0] = 1073741824L;
        q[1] = 0;
        q[2] = 0;
        q[3] = 0;
    }
}

void inv_q_invert(const long *q, long *qInverted)
{
    INVENSENSE_FUNC_START;
    qInverted[0] = q[0];
    qInverted[1] = -q[1];
    qInverted[2] = -q[2];
    qInverted[3] = -q[3];
}

void inv_q_multf(const float *q1, const float *q2, float *qProd)
{
    INVENSENSE_FUNC_START;
    qProd[0] = (q1[0] * q2[0] - q1[1] * q2[1] - q1[2] * q2[2] - q1[3] * q2[3]);
    qProd[1] = (q1[0] * q2[1] + q1[1] * q2[0] + q1[2] * q2[3] - q1[3] * q2[2]);
    qProd[2] = (q1[0] * q2[2] - q1[1] * q2[3] + q1[2] * q2[0] + q1[3] * q2[1]);
    qProd[3] = (q1[0] * q2[3] + q1[1] * q2[2] - q1[2] * q2[1] + q1[3] * q2[0]);
}

void inv_q_addf(float *q1, float *q2, float *qSum)
{
    INVENSENSE_FUNC_START;
    qSum[0] = q1[0] + q2[0];
    qSum[1] = q1[1] + q2[1];
    qSum[2] = q1[2] + q2[2];
    qSum[3] = q1[3] + q2[3];
}

void inv_q_normalizef(float *q)
{
    INVENSENSE_FUNC_START;
    float normSF = 0;
    float xHalf = 0;
    normSF = (q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3]);
    if (normSF < 2) {
        xHalf = 0.5f * normSF;
        normSF = normSF * (1.5f - xHalf * normSF * normSF);
        normSF = normSF * (1.5f - xHalf * normSF * normSF);
        normSF = normSF * (1.5f - xHalf * normSF * normSF);
        normSF = normSF * (1.5f - xHalf * normSF * normSF);
        q[0] *= normSF;
        q[1] *= normSF;
        q[2] *= normSF;
        q[3] *= normSF;
    } else {
        q[0] = 1.0;
        q[1] = 0.0;
        q[2] = 0.0;
        q[3] = 0.0;
    }
    normSF = (q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3]);
}

/** Performs a length 4 vector normalization with a square root.
* @param[in,out] vector to normalize. Returns [1,0,0,0] is magnitude is zero.
*/
void inv_q_norm4(float *q)
{
    float mag;
    mag = sqrtf(q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3]);
    if (mag) {
        q[0] /= mag;
        q[1] /= mag;
        q[2] /= mag;
        q[3] /= mag;
    } else {
        q[0] = 1.f;
        q[1] = 0.f;
        q[2] = 0.f;
        q[3] = 0.f;
    }
}

void inv_q_invertf(const float *q, float *qInverted)
{
    INVENSENSE_FUNC_START;
    qInverted[0] = q[0];
    qInverted[1] = -q[1];
    qInverted[2] = -q[2];
    qInverted[3] = -q[3];
}

/**
 * Converts a quaternion to a rotation matrix.
 * @param[in] quat 4-element quaternion in fixed point. One is 2^30.
 * @param[out] rot Rotation matrix in fixed point. One is 2^30. The
 *             First 3 elements of the rotation matrix, represent
 *             the first row of the matrix. Rotation matrix multiplied
 *             by a 3 element column vector transform a vector from Body
 *             to World.
 */
void inv_quaternion_to_rotation(const long *quat, long *rot)
{
    rot[0] =
        inv_q29_mult(quat[1], quat[1]) + inv_q29_mult(quat[0],
                                                      quat[0]) - 1073741824L;
    rot[1] = inv_q29_mult(quat[1], quat[2]) - inv_q29_mult(quat[3], quat[0]);
    rot[2] = inv_q29_mult(quat[1], quat[3]) + inv_q29_mult(quat[2], quat[0]);
    rot[3] = inv_q29_mult(quat[1], quat[2]) + inv_q29_mult(quat[3], quat[0]);
    rot[4] =
        inv_q29_mult(quat[2], quat[2]) + inv_q29_mult(quat[0],
                                                      quat[0]) - 1073741824L;
    rot[5] = inv_q29_mult(quat[2], quat[3]) - inv_q29_mult(quat[1], quat[0]);
    rot[6] = inv_q29_mult(quat[1], quat[3]) - inv_q29_mult(quat[2], quat[0]);
    rot[7] = inv_q29_mult(quat[2], quat[3]) + inv_q29_mult(quat[1], quat[0]);
    rot[8] =
        inv_q29_mult(quat[3], quat[3]) + inv_q29_mult(quat[0],
                                                      quat[0]) - 1073741824L;
}

/** Converts a 32-bit long to a big endian byte stream */
unsigned char *inv_int32_to_big8(long x, unsigned char *big8)
{
    big8[0] = (unsigned char)((x >> 24) & 0xff);
    big8[1] = (unsigned char)((x >> 16) & 0xff);
    big8[2] = (unsigned char)((x >> 8) & 0xff);
    big8[3] = (unsigned char)(x & 0xff);
    return big8;
}

/** Converts a big endian byte stream into a 32-bit long */
long inv_big8_to_int32(const unsigned char *big8)
{
    long x;
    x = ((long)big8[0] << 24) | ((long)big8[1] << 16) | ((long)big8[2] << 8) |
        ((long)big8[3]);
    return x;
}

/** Converts a 16-bit short to a big endian byte stream */
unsigned char *inv_int16_to_big8(short x, unsigned char *big8)
{
    big8[0] = (unsigned char)((x >> 8) & 0xff);
    big8[1] = (unsigned char)(x & 0xff);
    return big8;
}

void inv_matrix_det_inc(float *a, float *b, int *n, int x, int y)
{
    int k, l, i, j;
    for (i = 0, k = 0; i < *n; i++, k++) {
        for (j = 0, l = 0; j < *n; j++, l++) {
            if (i == x)
                i++;
            if (j == y)
                j++;
            *(b + 10 * k + l) = *(a + 10 * i + j);
        }
    }
    *n = *n - 1;
}

void inv_matrix_det_incd(double *a, double *b, int *n, int x, int y)
{
    int k, l, i, j;
    for (i = 0, k = 0; i < *n; i++, k++) {
        for (j = 0, l = 0; j < *n; j++, l++) {
            if (i == x)
                i++;
            if (j == y)
                j++;
            *(b + 10 * k + l) = *(a + 10 * i + j);
        }
    }
    *n = *n - 1;
}

float inv_matrix_det(float *p, int *n)
{
    float d[10][10], sum = 0;
    int i, j, m;
    m = *n;
    if (*n == 2)
        return (*p ** (p + 11) - *(p + 1) ** (p + 10));
    for (i = 0, j = 0; j < m; j++) {
        *n = m;
        inv_matrix_det_inc(p, &d[0][0], n, i, j);
        sum =
            sum + *(p + 10 * i + j) * SIGNM(i + j) * inv_matrix_det(&d[0][0],
                                                                    n);
    }

    return (sum);
}

double inv_matrix_detd(double *p, int *n)
{
    double d[10][10], sum = 0;
    int i, j, m;
    m = *n;
    if (*n == 2)
        return (*p ** (p + 11) - *(p + 1) ** (p + 10));
    for (i = 0, j = 0; j < m; j++) {
        *n = m;
        inv_matrix_det_incd(p, &d[0][0], n, i, j);
        sum =
            sum + *(p + 10 * i + j) * SIGNM(i + j) * inv_matrix_detd(&d[0][0],
                                                                     n);
    }

    return (sum);
}

/** Wraps angle from (-M_PI,M_PI]
 * @param[in] ang Angle in radians to wrap
 * @return Wrapped angle from (-M_PI,M_PI]
 */
float inv_wrap_angle(float ang)
{
    if (ang > M_PI)
        return ang - 2 * (float)M_PI;
    else if (ang <= -(float)M_PI)
        return ang + 2 * (float)M_PI;
    else
        return ang;
}

/** Finds the minimum angle difference ang1-ang2 such that difference
 * is between [-M_PI,M_PI]
 * @param[in] ang1 
 * @param[in] ang2
 * @return angle difference ang1-ang2
 */
float inv_angle_diff(float ang1, float ang2)
{
    float d;
    ang1 = inv_wrap_angle(ang1);
    ang2 = inv_wrap_angle(ang2);
    d = ang1 - ang2;
    if (d > M_PI)
        d -= 2 * (float)M_PI;
    else if (d < -(float)M_PI)
        d += 2 * (float)M_PI;
    return d;
}
