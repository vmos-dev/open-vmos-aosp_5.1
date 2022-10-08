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
 * $Id: ml_mputest.c 5641 2011-06-14 02:10:02Z mcaramello $
 *
 *****************************************************************************/

/**
 *  @defgroup MPU_SELF_TEST
 *  @brief  C wrapper to integrate the MPU Self Test wrapper in MPL.
 *          Provides ML name compliant naming and an additional API that
 *          automates the suspension of normal MPL operations, runs the test,
 *          and resume.
 *
 *  @{
 *      @file   ml_mputest.c
 *      @brief  C wrapper to integrate the MPU Self Test wrapper in MPL.
 *              The main logic of the test and APIs can be found in mputest.c
 */

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

#include "ml_mputest.h"

#include "mlmath.h"
#include "mlinclude.h"
#include "ml.h"
#include "mlstates.h"
#include "mldl.h"
#include "mldl_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
    Globals
*/
extern struct mldl_cfg *mputestCfgPtr;
extern signed char g_z_sign;

/*
    Prototypes
*/
extern inv_error_t inv_factory_calibrate(void *mlsl_handle,
                                         uint_fast8_t provide_result);

/**
 *  @brief  An MPL wrapper for the main MPU Self Test API inv_factory_calibrate().
 *          See inv_factory_calibrate() function for more details.
 *
 *  @pre    inv_dmp_open() <b>must</b> have been called to populate the mldl_cfg
 *          data structure.
 *          On Windows, SetupPlatform() is also a madatory pre condition to
 *          ensure the accelerometer is properly configured before running the
 *          test.
 *
 *  @param  mlsl_handle
 *              serial interface handle to allow serial communication with the
 *              device, both gyro and accelerometer.
 *  @param  provide_result
 *              If 1:
 *              perform and analyze the offset, drive frequency, and noise
 *              calculation and compare it against set thresholds. Report
 *              also the final result using a bit-mask like error code as
 *              described in the inv_test_gyro_xxxx() functions.
 *              When 0:
 *              skip the noise and drive frequency calculation  and pass/fail
 *              assessment. It simply calculates the gyro and accel biases.
 *              NOTE: for MPU6050 devices, this parameter is currently
 *              ignored.
 *
 *  @return INV_SUCCESS or first non-zero error code otherwise.
 */
inv_error_t inv_self_test_factory_calibrate(void *mlsl_handle,
                                            unsigned char provide_result)
{
    INVENSENSE_FUNC_START;
    inv_error_t firstError = INV_SUCCESS;
    inv_error_t result;
    unsigned char initState = inv_get_state();

    if (initState < INV_STATE_DMP_OPENED) {
        MPL_LOGE("Self Test cannot run before inv_dmp_open()\n");
        return INV_ERROR_SM_IMPROPER_STATE;
    }

    /* obtain a pointer to the 'struct mldl_cfg' data structure. */
    mputestCfgPtr = inv_get_dl_config();

    if(initState == INV_STATE_DMP_STARTED) {
        result = inv_dmp_stop();
        ERROR_CHECK_FIRST(firstError, result);
    }

    result = inv_factory_calibrate(mlsl_handle, provide_result);
    ERROR_CHECK_FIRST(firstError, result);

    if(initState == INV_STATE_DMP_STARTED) {
        result = inv_dmp_start();
        ERROR_CHECK_FIRST(firstError, result);
    }

    return firstError;
}

/**
 *  @brief  Runs the MPU test at MPL runtime.
 *          If the DMP is operating, stops the DMP temporarely,
 *          runs the MPU Self Test, and re-starts the DMP.
 *
 *  @return INV_SUCCESS or a non-zero error code otherwise.
 */
inv_error_t inv_self_test_run(void)
{
#ifdef CONFIG_MPU_SENSORS_MPU3050
    return inv_self_test_factory_calibrate(inv_get_serial_handle(), TRUE);
#else
    return inv_self_test_factory_calibrate(inv_get_serial_handle(), FALSE);
#endif
}

/**
 *  @brief  Runs the MPU test for bias correction only at MPL runtime.
 *          If the DMP is operating, stops the DMP temporarely,
 *          runs the bias calculation routines, and re-starts the DMP.
 *
 *  @return INV_SUCCESS or a non-zero error code otherwise.
 */
inv_error_t inv_self_test_bias_only(void)
{
    return inv_self_test_factory_calibrate(inv_get_serial_handle(), FALSE);
}

/**
 *  @brief  Set the orientation of the acceleroemter Z axis as it will be
 *          expected when running the MPU Self Test.
 *          Specifies the orientation of the accelerometer Z axis : Z axis
 *          pointing upwards or downwards.
 *  @param  z_sign
 *              The sign of the accelerometer Z axis; valid values are +1 and
 *              -1 for +Z and -Z respectively.  Any other value will cause the
 *              setting to be ignored and an error code to be returned.
 *  @return INV_SUCCESS or a non-zero error code.
 */
inv_error_t inv_self_test_set_accel_z_orient(signed char z_sign)
{
    if (z_sign != +1 && z_sign != -1) {
        return INV_ERROR_INVALID_PARAMETER;
    }
    g_z_sign = z_sign;
    return INV_SUCCESS;
}


#ifdef __cplusplus
}
#endif

/**
 *  @}
 */

