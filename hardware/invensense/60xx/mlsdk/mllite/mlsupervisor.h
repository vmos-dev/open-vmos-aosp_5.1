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
 * $Id: mlsupervisor.h 5629 2011-06-11 03:13:08Z mcaramello $
 *
 *****************************************************************************/

#ifndef __INV_SUPERVISOR_H__
#define __INV_SUPERVISOR_H__

#include "mltypes.h"
#ifdef INV_INCLUDE_LEGACY_HEADERS
#include "mlsupervisor_legacy.h"
#endif

// The value of inv_get_gyro_sum_of_sqr is scaled such the (1 dps)^2 = 2^this_number
// this number must be >=0 and even.
#define GYRO_MAG_SQR_SHIFT 6
// The value of inv_accel_sum_of_sqr is scaled such that (1g)^2 = 2^this_number
#define ACC_MAG_SQR_SHIFT 16

#define CAL_RUN             0
#define CAL_RESET           1
#define CAL_CHANGED_DATA    2
#define CAL_RESET_TIME      3
#define CAL_ADD_DATA        4
#define CAL_COMBINE         5

#define P_INIT  100000

#define SF_NORMAL           0
#define SF_DISTURBANCE      1
#define SF_FAST_SETTLE      2
#define SF_SLOW_SETTLE      3
#define SF_STARTUP_SETTLE   4
#define SF_UNCALIBRATED     5

struct inv_supervisor_cb_obj {
    void (*accel_compass_fusion_func) (double magFB);
     inv_error_t(*progressive_no_motion_supervisor_func) (unsigned long
                                                          deltaTime);
     inv_error_t(*sensor_fusion_advanced_func) (double *magFB,
                                                unsigned long deltaTime);
    void (*reset_advanced_compass_func) (void);
    void (*supervisor_reset_func) (void);
};

inv_error_t inv_reset_compass_calibration(void);
void inv_init_sensor_fusion_supervisor(void);
inv_error_t inv_accel_compass_supervisor(void);
inv_error_t inv_pressure_supervisor(void);

#endif // __INV_SUPERVISOR_H__

