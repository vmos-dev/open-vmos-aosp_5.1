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

/******************************************************************************
 *
 * $Id$
 *
 *****************************************************************************/

#ifndef INV_SET_GYRO_BIAS__H__
#define INV_SET_GYRO_BIAS__H__

#include "mltypes.h"

#ifdef __cplusplus
extern "C" {
#endif

#define INV_SGB_NO_MOTION 4
#define INV_SGB_FAST_NO_MOTION 5
#define INV_SGB_TEMP_COMP 6

    inv_error_t inv_enable_set_bias(void);
    inv_error_t inv_disable_set_bias(void);
    inv_error_t inv_set_gyro_bias_in_hw_unit(const short *bias, int mode);
    inv_error_t inv_set_gyro_bias_in_dps(const long *bias, int mode);
    inv_error_t inv_set_gyro_bias_in_dps_float(const float *bias, int mode);
    void inv_convert_bias(const unsigned char *regs, short *bias);
    void inv_set_motion_state(int motion);

#ifdef __cplusplus
}
#endif
#endif                          // INV_SET_GYRO_BIAS__H__
