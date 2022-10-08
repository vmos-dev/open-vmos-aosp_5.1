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
/*******************************************************************************
 *
 * $Id: compass.h 5629 2011-06-11 03:13:08Z mcaramello $
 *
 *******************************************************************************/

#ifndef COMPASS_H
#define COMPASS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "mltypes.h"
#include "mpu.h"
#ifdef INV_INCLUDE_LEGACY_HEADERS
#include "compass_legacy.h"
#endif
    /* ------------ */
    /* - Defines. - */
    /* ------------ */

#define YAS_MAX_FILTER_LEN (20)
#define YAS_DEFAULT_FILTER_LEN (20)
#define YAS_DEFAULT_FILTER_THRESH (300) /* 300 nT */
#define YAS_DEFAULT_FILTER_NOISE (2000 * 2000) /* standard deviation 2000 nT */

    /* --------------- */
    /* - Structures. - */
    /* --------------- */

struct yas_adaptive_filter {
    int num;
    int index;
    int len;
    float noise;
    float sequence[YAS_MAX_FILTER_LEN];
};

struct yas_thresh_filter {
    float threshold;
    float last;
};

typedef struct {
    struct yas_adaptive_filter adap_filter[3];
    struct yas_thresh_filter thresh_filter[3];
} yas_filter_handle_t;

typedef struct {
    int (*init)(yas_filter_handle_t *t);
    int (*update)(yas_filter_handle_t *t, float *input, float *output);
} yas_filter_if_s;

    /* --------------------- */
    /* - Function p-types. - */
    /* --------------------- */

    unsigned char inv_compass_present(void);
    unsigned char inv_get_compass_slave_addr(void);
    inv_error_t inv_get_compass_data(long *data);
    inv_error_t inv_set_compass_bias(long *bias);
    unsigned short inv_get_compass_id(void);
    inv_error_t inv_set_compass_offset(void);
    inv_error_t inv_compass_check_range(void);
    inv_error_t inv_compass_write_reg(unsigned char reg, unsigned char val);
    inv_error_t inv_compass_read_reg(unsigned char reg, unsigned char *val);
    inv_error_t inv_compass_read_scale(long *val);

    int yas_filter_init(yas_filter_if_s *f);

#ifdef __cplusplus
}
#endif
#endif                          // COMPASS_H
