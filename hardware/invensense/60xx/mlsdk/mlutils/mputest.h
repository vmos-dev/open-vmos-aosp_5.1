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
 * $Id: mputest.h 4051 2010-11-19 04:51:58Z mcaramello $
 *
 *****************************************************************************/

#ifndef MPUTEST_H
#define MPUTEST_H

#ifdef __cplusplus
extern "C" {
#endif

#include "mlsl.h"
#include "mldl_cfg.h"
#include "mputest_legacy.h"

/* user facing APIs */
inv_error_t inv_factory_calibrate(void *mlsl_handle,
                                  uint_fast8_t provide_result);
void inv_set_test_parameters(unsigned int slave_addr, float sensitivity,
                             int p_thresh, float total_time_tol,
                             int bias_thresh, float rms_thresh,
                             float sp_shift_thresh,
                             unsigned short accel_samples);

/* additional functions */
int  inv_mpu_test(void *mlsl_handle, uint_fast8_t provide_result);


#ifdef __cplusplus
}
#endif

#endif /* MPUTEST_H */

