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
 * $Id: mldmp.c 5629 2011-06-11 03:13:08Z mcaramello $
 *
 *****************************************************************************/

/**
 * @addtogroup MLDMP
 *
 * @{
 *      @file     mldmp.c
 *      @brief    Shared functions between all the different DMP versions
**/

#include <stdio.h>

#include "mltypes.h"
#include "mlinclude.h"
#include "mltypes.h"
#include "ml.h"
#include "mldl_cfg.h"
#include "mldl.h"
#include "compass.h"
#include "mlSetGyroBias.h"
#include "mlsl.h"
#include "mlFIFO.h"
#include "mldmp.h"
#include "mlstates.h"
#include "dmpDefault.h"
#include "mlFIFOHW.h"
#include "mlsupervisor.h"

#include "log.h"
#undef MPL_LOG_TAG
#define MPL_LOG_TAG "MPL-dmp"

/**
 *  @brief  Open the default motion sensor engine.
 *          This function is used to open the default MPL engine, 
 *          featuring, for example, sensor fusion (6 axes and 9 axes), 
 *          sensor calibration, accelerometer data byte swapping, among 
 *          others.  
 *          Compare with the other provided engines.
 *
 *  @pre    inv_serial_start() must have been called to instantiate the serial 
 *          communication.
 *  
 *  Example:
 *  @code
 *    result = inv_dmp_open( );
 *    if (INV_SUCCESS != result) {
 *        // Handle the error case
 *    }
 *  @endcode
 *
 *  @return Zero on success; Error Code on any failure.
 *
 */
inv_error_t inv_dmp_open(void)
{
    INVENSENSE_FUNC_START;
    inv_error_t result;
    unsigned char state = inv_get_state();
    struct mldl_cfg *mldl_cfg;
    unsigned long requested_sensors;

    /*************************************************************
     * Common operations before calling DMPOpen
     ************************************************************/
    if (state == INV_STATE_DMP_OPENED)
        return INV_SUCCESS;

    if (state == INV_STATE_DMP_STARTED) {
        return inv_dmp_stop();
    }

    result = inv_state_transition(INV_STATE_DMP_OPENED);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    result = inv_dl_open(inv_get_serial_handle());
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
#ifdef ML_USE_DMP_SIM
    do {
        void setup_univ();
        setup_univ();           /* hijack the read and write paths 
                                   and re-direct them to the simulator */
    } while (0);
#endif

    result = inv_setup_dmp();
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    // Init vars.
    inv_init_ml();

    result = inv_init_fifo_param();
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    result = inv_enable_set_bias();
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    inv_init_fifo_hardare();
    mldl_cfg = inv_get_dl_config();
    requested_sensors = INV_THREE_AXIS_GYRO;
    if (mldl_cfg->accel && mldl_cfg->accel->resume)
        requested_sensors |= INV_THREE_AXIS_ACCEL;

    if (mldl_cfg->compass && mldl_cfg->compass->resume)
        requested_sensors |= INV_THREE_AXIS_COMPASS;

    if (mldl_cfg->pressure && mldl_cfg->pressure->resume)
        requested_sensors |= INV_THREE_AXIS_PRESSURE;

    result = inv_init_requested_sensors(requested_sensors);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    result = inv_apply_calibration();
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    if (NULL != mldl_cfg->accel){
        result = inv_apply_endian_accel();
    }

    return result;
}

/**
 *  @brief  Start the DMP.
 *
 *  @pre    inv_dmp_open() must have been called.
 * 
 *  @code
 *     result = inv_dmp_start();
 *     if (INV_SUCCESS != result) {
 *         // Handle the error case
 *     }
 *  @endcode
 *
 *  @return INV_SUCCESS if successful, or Non-zero error code otherwise.
 */
inv_error_t inv_dmp_start(void)
{
    INVENSENSE_FUNC_START;
    inv_error_t result;

    if (inv_get_state() == INV_STATE_DMP_STARTED)
        return INV_SUCCESS;

    result = inv_state_transition(INV_STATE_DMP_STARTED);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    inv_init_sensor_fusion_supervisor();
    result = inv_dl_start(inv_get_dl_config()->requested_sensors);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    /* This is done after the start since it will modify DMP memory, which 
     * will cause a full reset is most cases */
    result = inv_reset_motion();
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    return result;
}

/**
 *  @brief  Stops the DMP and puts it in low power.
 *
 *  @pre    inv_dmp_start() must have been called.
 * 
 *  @return INV_SUCCESS, Non-zero error code otherwise.
 */
inv_error_t inv_dmp_stop(void)
{
    INVENSENSE_FUNC_START;
    inv_error_t result;

    if (inv_get_state() == INV_STATE_DMP_OPENED)
        return INV_SUCCESS;

    result = inv_state_transition(INV_STATE_DMP_OPENED);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    result = inv_dl_stop(INV_ALL_SENSORS);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    return result;
}

/**
 *  @brief  Closes the motion sensor engine.
 *          Does not close the serial communication. To do that,
 *          call inv_serial_stop().
 *          After calling inv_dmp_close() another DMP module can be
 *          loaded in the MPL with the corresponding necessary 
 *          intialization and configurations, via any of the 
 *          MLDmpXXXOpen functions.
 *
 *  @pre    inv_dmp_open() must have been called.
 * 
 *  @code
 *     result = inv_dmp_close();
 *     if (INV_SUCCESS != result) {
 *         // Handle the error case
 *     }
 *  @endcode
 *
 *  @return INV_SUCCESS, Non-zero error code otherwise.
 */
inv_error_t inv_dmp_close(void)
{
    INVENSENSE_FUNC_START;
    inv_error_t result;
    inv_error_t firstError = INV_SUCCESS;

    if (inv_get_state() <= INV_STATE_DMP_CLOSED)
        return INV_SUCCESS;

    result = inv_disable_set_bias();
    ERROR_CHECK_FIRST(firstError, result);

    result = inv_dl_stop(INV_ALL_SENSORS);
    ERROR_CHECK_FIRST(firstError, result);

    result = inv_close_fifo();
    ERROR_CHECK_FIRST(firstError, result);

    result = inv_dl_close();
    ERROR_CHECK_FIRST(firstError, result);

    result = inv_state_transition(INV_STATE_SERIAL_OPENED);
    ERROR_CHECK_FIRST(firstError, result);

    return result;
}

/**
 *  @}
 */
