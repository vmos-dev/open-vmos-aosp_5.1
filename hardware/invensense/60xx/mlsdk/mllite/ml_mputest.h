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
 * $Id: ml_mputest.h 5629 2011-06-11 03:13:08Z mcaramello $
 *
 *****************************************************************************/

#ifndef _INV_MPUTEST_H_
#define _INV_MPUTEST_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "mltypes.h"

/* user APIs */
inv_error_t inv_self_test_factory_calibrate(
                void *mlsl_handle, unsigned char provide_result);
inv_error_t inv_self_test_set_accel_z_orient(signed char z_sign);
inv_error_t inv_self_test_run(void);
inv_error_t inv_self_test_bias_only(void);

/* other functions */
#define inv_set_self_test_parameters inv_set_test_parameters

#ifdef __cplusplus
}
#endif

#endif /* _INV_MPUTEST_H_ */

