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
 * $Id: ml.c 5642 2011-06-14 02:26:20Z mcaramello $
 *
 *****************************************************************************/

/**
 *  @defgroup ML
 *  @brief  Motion Library APIs.
 *          The Motion Library processes gyroscopes, accelerometers, and
 *          compasses to provide a physical model of the movement for the
 *          sensors.
 *          The results of this processing may be used to control objects
 *          within a user interface environment, detect gestures, track 3D
 *          movement for gaming applications, and analyze the blur created
 *          due to hand movement while taking a picture.
 *
 *  @{
 *      @file   ml.c
 *      @brief  The Motion Library APIs.
 */

/* ------------------ */
/* - Include Files. - */
/* ------------------ */

#include <string.h>

#include "ml.h"
#include "mldl.h"
#include "mltypes.h"
#include "mlinclude.h"
#include "compass.h"
#include "dmpKey.h"
#include "dmpDefault.h"
#include "mlstates.h"
#include "mlFIFO.h"
#include "mlFIFOHW.h"
#include "mlMathFunc.h"
#include "mlsupervisor.h"
#include "mlmath.h"
#include "mlcontrol.h"
#include "mldl_cfg.h"
#include "mpu.h"
#include "accel.h"
#include "mlos.h"
#include "mlsl.h"
#include "mlos.h"
#include "mlBiasNoMotion.h"
#include "log.h"
#undef MPL_LOG_TAG
#define MPL_LOG_TAG "MPL-ml"

#define ML_MOT_TYPE_NONE 0
#define ML_MOT_TYPE_NO_MOTION 1
#define ML_MOT_TYPE_MOTION_DETECTED 2

#define ML_MOT_STATE_MOVING 0
#define ML_MOT_STATE_NO_MOTION 1
#define ML_MOT_STATE_BIAS_IN_PROG 2

#define _mlDebug(x)             //{x}

/* Global Variables */
const unsigned char mlVer[] = { INV_VERSION };

struct inv_params_obj inv_params_obj = {
    INV_BIAS_UPDATE_FUNC_DEFAULT,   // bias_mode
    INV_ORIENTATION_MASK_DEFAULT,   // orientation_mask
    INV_PROCESSED_DATA_CALLBACK_DEFAULT,    // fifo_processed_func
    INV_ORIENTATION_CALLBACK_DEFAULT,   // orientation_cb_func
    INV_MOTION_CALLBACK_DEFAULT,    // motion_cb_func
    INV_STATE_SERIAL_CLOSED     // starting state
};

struct inv_obj_t inv_obj;
void *g_mlsl_handle;

typedef struct {
    // These describe callbacks happening everythime a DMP interrupt is processed
    int_fast8_t numInterruptProcesses;
    // Array of function pointers, function being void function taking void
    inv_obj_func processInterruptCb[MAX_INTERRUPT_PROCESSES];

} tMLXCallbackInterrupt;        // MLX_callback_t

tMLXCallbackInterrupt mlxCallbackInterrupt;

/* --------------- */
/* -  Functions. - */
/* --------------- */

inv_error_t inv_freescale_sensor_fusion_16bit(unsigned short orient);
inv_error_t inv_freescale_sensor_fusion_8bit(unsigned short orient);
unsigned short inv_orientation_matrix_to_scalar(const signed char *mtx);

/**
 *  @brief  Open serial connection with the MPU device.
 *          This is the entry point of the MPL and must be
 *          called prior to any other function call.
 *
 *  @param  port     System handle for 'port' MPU device is found on.
 *                   The significance of this parameter varies by
 *                   platform. It is passed as 'port' to MLSLSerialOpen.
 *
 *  @return INV_SUCCESS or error code.
 */
inv_error_t inv_serial_start(char const *port)
{
    INVENSENSE_FUNC_START;
    inv_error_t result;

    if (inv_get_state() >= INV_STATE_SERIAL_OPENED)
        return INV_SUCCESS;

    result = inv_state_transition(INV_STATE_SERIAL_OPENED);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    result = inv_serial_open(port, &g_mlsl_handle);
    if (INV_SUCCESS != result) {
        (void)inv_state_transition(INV_STATE_SERIAL_CLOSED);
    }

    return result;
}

/**
 *  @brief  Close the serial communication.
 *          This function needs to be called explicitly to shut down
 *          the communication with the device.  Calling inv_dmp_close()
 *          won't affect the established serial communication.
 *  @return INV_SUCCESS; non-zero error code otherwise.
 */
inv_error_t inv_serial_stop(void)
{
    INVENSENSE_FUNC_START;
    inv_error_t result = INV_SUCCESS;

    if (inv_get_state() == INV_STATE_SERIAL_CLOSED)
        return INV_SUCCESS;

    result = inv_state_transition(INV_STATE_SERIAL_CLOSED);
    if (INV_SUCCESS != result) {
        MPL_LOGE("State Transition Failure in %s: %d\n", __func__, result);
    }
    result = inv_serial_close(g_mlsl_handle);
    if (INV_SUCCESS != result) {
        MPL_LOGE("Unable to close Serial Handle %s: %d\n", __func__, result);
    }
    return result;
}

/**
 *  @brief  Get the serial file handle to the device.
 *  @return The serial file handle.
 */
void *inv_get_serial_handle(void)
{
    INVENSENSE_FUNC_START;
    return g_mlsl_handle;
}

/**
 *  @brief  apply the choosen orientation and full scale range
 *          for gyroscopes, accelerometer, and compass.
 *  @return INV_SUCCESS if successful, a non-zero code otherwise.
 */
inv_error_t inv_apply_calibration(void)
{
    INVENSENSE_FUNC_START;
    signed char gyroCal[9] = { 0 };
    signed char accelCal[9] = { 0 };
    signed char magCal[9] = { 0 };
    float gyroScale = 2000.f;
    float accelScale = 0.f;
    float magScale = 0.f;

    inv_error_t result;
    int ii;
    struct mldl_cfg *mldl_cfg = inv_get_dl_config();

    for (ii = 0; ii < 9; ii++) {
        gyroCal[ii] = mldl_cfg->pdata->orientation[ii];
        if (NULL != mldl_cfg->accel){
            accelCal[ii] = mldl_cfg->pdata->accel.orientation[ii];
        }
        if (NULL != mldl_cfg->compass){
            magCal[ii] = mldl_cfg->pdata->compass.orientation[ii];
        }
    }

    switch (mldl_cfg->full_scale) {
    case MPU_FS_250DPS:
        gyroScale = 250.f;
        break;
    case MPU_FS_500DPS:
        gyroScale = 500.f;
        break;
    case MPU_FS_1000DPS:
        gyroScale = 1000.f;
        break;
    case MPU_FS_2000DPS:
        gyroScale = 2000.f;
        break;
    default:
        MPL_LOGE("Unrecognized full scale setting for gyros : %02X\n",
                 mldl_cfg->full_scale);
        return INV_ERROR_INVALID_PARAMETER;
        break;
    }

    if (NULL != mldl_cfg->accel){
        RANGE_FIXEDPOINT_TO_FLOAT(mldl_cfg->accel->range, accelScale);
        inv_obj.accel_sens = (long)(accelScale * 65536L);
    /* sensitivity adjustment, typically = 2 (for +/- 2 gee) */
#if defined CONFIG_MPU_SENSORS_MPU6050A2 || \
    defined CONFIG_MPU_SENSORS_MPU6050B1
    inv_obj.accel_sens /= 32768 / mldl_cfg->accel_sens_trim;
#else
    inv_obj.accel_sens /= 2;
#endif
    }
    if (NULL != mldl_cfg->compass){
        RANGE_FIXEDPOINT_TO_FLOAT(mldl_cfg->compass->range, magScale);
        inv_obj.compass_sens = (long)(magScale * 32768);
    }

    if (inv_get_state() == INV_STATE_DMP_OPENED) {
        result = inv_set_gyro_calibration(gyroScale, gyroCal);
        if (INV_SUCCESS != result) {
            MPL_LOGE("Unable to set Gyro Calibration\n");
            return result;
        }
        if (NULL != mldl_cfg->accel){
            result = inv_set_accel_calibration(accelScale, accelCal);
            if (INV_SUCCESS != result) {
                MPL_LOGE("Unable to set Accel Calibration\n");
                return result;
            }
        }
        if (NULL != mldl_cfg->compass){
            result = inv_set_compass_calibration(magScale, magCal);
            if (INV_SUCCESS != result) {
                MPL_LOGE("Unable to set Mag Calibration\n");
                return result;
            }
        }
    }
    return INV_SUCCESS;
}

/**
 *  @brief  Setup the DMP to handle the accelerometer endianess.
 *  @return INV_SUCCESS if successful, a non-zero error code otherwise.
 */
inv_error_t inv_apply_endian_accel(void)
{
    INVENSENSE_FUNC_START;
    unsigned char regs[4] = { 0 };
    struct mldl_cfg *mldl_cfg = inv_get_dl_config();
    int endian = mldl_cfg->accel->endian;

    if (mldl_cfg->pdata->accel.bus != EXT_SLAVE_BUS_SECONDARY) {
        endian = EXT_SLAVE_BIG_ENDIAN;
    }
    switch (endian) {
    case EXT_SLAVE_FS8_BIG_ENDIAN:
    case EXT_SLAVE_FS16_BIG_ENDIAN:
    case EXT_SLAVE_LITTLE_ENDIAN:
        regs[0] = 0;
        regs[1] = 64;
        regs[2] = 0;
        regs[3] = 0;
        break;
    case EXT_SLAVE_BIG_ENDIAN:
    default:
        regs[0] = 0;
        regs[1] = 0;
        regs[2] = 64;
        regs[3] = 0;
    }

    return inv_set_mpu_memory(KEY_D_1_236, 4, regs);
}

/**
 * @internal
 * @brief   Initialize MLX data.  This should be called to setup the mlx
 *          output buffers before any motion processing is done.
 */
void inv_init_ml(void)
{
    INVENSENSE_FUNC_START;
    int ii;
    long tmp[COMPASS_NUM_AXES];
    // Set all values to zero by default
    memset(&inv_obj, 0, sizeof(struct inv_obj_t));
    memset(&mlxCallbackInterrupt, 0, sizeof(tMLXCallbackInterrupt));

    inv_obj.compass_correction[0] = 1073741824L;
    inv_obj.compass_correction_relative[0] = 1073741824L;
    inv_obj.compass_disturb_correction[0] = 1073741824L;
    inv_obj.compass_correction_offset[0] = 1073741824L;
    inv_obj.relative_quat[0] = 1073741824L;

    //Not used with the ST accelerometer
    inv_obj.no_motion_threshold = 20;   // noMotionThreshold
    //Not used with the ST accelerometer
    inv_obj.motion_duration = 1536; // motionDuration

    inv_obj.motion_state = INV_MOTION;  // Motion state

    inv_obj.bias_update_time = 8000;
    inv_obj.bias_calc_time = 2000;

    inv_obj.internal_motion_state = ML_MOT_STATE_MOVING;

    inv_obj.start_time = inv_get_tick_count();

    inv_obj.compass_cal[0] = 322122560L;
    inv_obj.compass_cal[4] = 322122560L;
    inv_obj.compass_cal[8] = 322122560L;
    inv_obj.compass_sens = 322122560L;  // Should only change when the sensor full-scale range (FSR) is changed.

    for (ii = 0; ii < COMPASS_NUM_AXES; ii++) {
        inv_obj.compass_scale[ii] = 65536L;
        inv_obj.compass_test_scale[ii] = 65536L;
        inv_obj.compass_bias_error[ii] = P_INIT;
        inv_obj.init_compass_bias[ii] = 0;
        inv_obj.compass_asa[ii] = (1L << 30);
    }
    if (inv_compass_read_scale(tmp) == INV_SUCCESS) {
        for (ii = 0; ii < COMPASS_NUM_AXES; ii++)
            inv_obj.compass_asa[ii] = tmp[ii];
    }
    inv_obj.got_no_motion_bias = 0;
    inv_obj.got_compass_bias = 0;
    inv_obj.compass_state = SF_UNCALIBRATED;
    inv_obj.acc_state = SF_STARTUP_SETTLE;

    inv_obj.got_init_compass_bias = 0;
    inv_obj.resetting_compass = 0;

    inv_obj.external_slave_callback = NULL;
    inv_obj.compass_accuracy = 0;

    inv_obj.factory_temp_comp = 0;
    inv_obj.got_coarse_heading = 0;

    inv_obj.compass_bias_ptr[0] = P_INIT;
    inv_obj.compass_bias_ptr[4] = P_INIT;
    inv_obj.compass_bias_ptr[8] = P_INIT;

    inv_obj.gyro_bias_err = 1310720;

    inv_obj.accel_lpf_gain = 1073744L;
    inv_obj.no_motion_accel_threshold = 7000000L;
}

/**
 * @internal
 * @brief   This registers a function to be called for each time the DMP
 *          generates an an interrupt.
 *          It will be called after inv_register_fifo_rate_process() which gets called
 *          every time the FIFO data is processed.
 *          The FIFO does not have to be on for this callback.
 * @param func Function to be called when a DMP interrupt occurs.
 * @return INV_SUCCESS or non-zero error code.
 */
inv_error_t inv_register_dmp_interupt_cb(inv_obj_func func)
{
    INVENSENSE_FUNC_START;
    // Make sure we have not filled up our number of allowable callbacks
    if (mlxCallbackInterrupt.numInterruptProcesses <=
        MAX_INTERRUPT_PROCESSES - 1) {
        int kk;
        // Make sure we haven't registered this function already
        for (kk = 0; kk < mlxCallbackInterrupt.numInterruptProcesses; ++kk) {
            if (mlxCallbackInterrupt.processInterruptCb[kk] == func) {
                return INV_ERROR_INVALID_PARAMETER;
            }
        }
        // Add new callback
        mlxCallbackInterrupt.processInterruptCb[mlxCallbackInterrupt.
                                                numInterruptProcesses] = func;
        mlxCallbackInterrupt.numInterruptProcesses++;
        return INV_SUCCESS;
    }
    return INV_ERROR_MEMORY_EXAUSTED;
}

/**
 * @internal
 * @brief This unregisters a function to be called for each DMP interrupt.
 * @return INV_SUCCESS or non-zero error code.
 */
inv_error_t inv_unregister_dmp_interupt_cb(inv_obj_func func)
{
    INVENSENSE_FUNC_START;
    int kk, jj;
    // Make sure we haven't registered this function already
    for (kk = 0; kk < mlxCallbackInterrupt.numInterruptProcesses; ++kk) {
        if (mlxCallbackInterrupt.processInterruptCb[kk] == func) {
            // FIXME, we may need a thread block here
            for (jj = kk + 1; jj < mlxCallbackInterrupt.numInterruptProcesses;
                 ++jj) {
                mlxCallbackInterrupt.processInterruptCb[jj - 1] =
                    mlxCallbackInterrupt.processInterruptCb[jj];
            }
            mlxCallbackInterrupt.numInterruptProcesses--;
            return INV_SUCCESS;
        }
    }
    return INV_ERROR_INVALID_PARAMETER;
}

/**
 *  @internal
 *  @brief  Run the recorded interrupt process callbacks in the event
 *          of an interrupt.
 */
void inv_run_dmp_interupt_cb(void)
{
    int kk;
    for (kk = 0; kk < mlxCallbackInterrupt.numInterruptProcesses; ++kk) {
        if (mlxCallbackInterrupt.processInterruptCb[kk])
            mlxCallbackInterrupt.processInterruptCb[kk] (&inv_obj);
    }
}

/** @internal
* Resets the Motion/No Motion state which should be called at startup and resume.
*/
inv_error_t inv_reset_motion(void)
{
    unsigned char regs[8];
    inv_error_t result;

    inv_obj.motion_state = INV_MOTION;
    inv_obj.flags[INV_MOTION_STATE_CHANGE] = INV_MOTION;
    inv_obj.no_motion_accel_time = inv_get_tick_count();
#if defined CONFIG_MPU_SENSORS_MPU3050
    regs[0] = DINAD8 + 2;
    regs[1] = DINA0C;
    regs[2] = DINAD8 + 1;
    result = inv_set_mpu_memory(KEY_CFG_18, 3, regs);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
#else
#endif
    regs[0] = (unsigned char)((inv_obj.motion_duration >> 8) & 0xff);
    regs[1] = (unsigned char)(inv_obj.motion_duration & 0xff);
    result = inv_set_mpu_memory(KEY_D_1_106, 2, regs);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    memset(regs, 0, 8);
    result = inv_set_mpu_memory(KEY_D_1_96, 8, regs);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    result =
        inv_set_mpu_memory(KEY_D_0_96, 4, inv_int32_to_big8(0x40000000, regs));
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    inv_set_motion_state(INV_MOTION);
    return result;
}

/**
 * @internal
 * @brief   inv_start_bias_calc starts the bias calculation on the MPU.
 */
void inv_start_bias_calc(void)
{
    INVENSENSE_FUNC_START;

    inv_obj.suspend = 1;
}

/**
 * @internal
 * @brief   inv_stop_bias_calc stops the bias calculation on the MPU.
 */
void inv_stop_bias_calc(void)
{
    INVENSENSE_FUNC_START;

    inv_obj.suspend = 0;
}

/**
 *  @brief  inv_update_data fetches data from the fifo and updates the
 *          motion algorithms.
 *
 *  @pre    inv_dmp_open()
 *          @ifnot MPL_MF
 *              or inv_open_low_power_pedometer()
 *              or inv_eis_open_dmp()
 *          @endif
 *          and inv_dmp_start() must have been called.
 *
 *  @note   Motion algorithm data is constant between calls to inv_update_data
 *
 * @return
 * - INV_SUCCESS
 * - Non-zero error code
 */
inv_error_t inv_update_data(void)
{
    INVENSENSE_FUNC_START;
    inv_error_t result = INV_SUCCESS;
    int_fast8_t got, ftry;
    uint_fast8_t mpu_interrupt;
    struct mldl_cfg *mldl_cfg = inv_get_dl_config();

    if (inv_get_state() != INV_STATE_DMP_STARTED)
        return INV_ERROR_SM_IMPROPER_STATE;

    // Set the maximum number of FIFO packets we want to process
    if (mldl_cfg->requested_sensors & INV_DMP_PROCESSOR) {
        ftry = 100;             // Large enough to process all packets
    } else {
        ftry = 1;
    }

    // Go and process at most ftry number of packets, probably less than ftry
    result = inv_read_and_process_fifo(ftry, &got);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    // Process all interrupts
    mpu_interrupt = inv_get_interrupt_trigger(INTSRC_AUX1);
    if (mpu_interrupt) {
        inv_clear_interrupt_trigger(INTSRC_AUX1);
    }
    // Check if interrupt was from MPU
    mpu_interrupt = inv_get_interrupt_trigger(INTSRC_MPU);
    if (mpu_interrupt) {
        inv_clear_interrupt_trigger(INTSRC_MPU);
    }
    // Take care of the callbacks that want to be notified when there was an MPU interrupt
    if (mpu_interrupt) {
        inv_run_dmp_interupt_cb();
    }

    result = inv_get_fifo_status();
    return result;
}

/**
 *  @brief  inv_check_flag returns the value of a flag.
 *          inv_check_flag can be used to check a number of flags,
 *          allowing users to poll flags rather than register callback
 *          functions. If a flag is set to True when inv_check_flag is called,
 *          the flag is automatically reset.
 *          The flags are:
 *          - INV_RAW_DATA_READY
 *          Indicates that new raw data is available.
 *          - INV_PROCESSED_DATA_READY
 *          Indicates that new processed data is available.
 *          - INV_GOT_GESTURE
 *          Indicates that a gesture has been detected by the gesture engine.
 *          - INV_MOTION_STATE_CHANGE
 *          Indicates that a change has been made from motion to no motion,
 *          or vice versa.
 *
 *  @pre    inv_dmp_open()
 *          @ifnot MPL_MF
 *              or inv_open_low_power_pedometer()
 *              or inv_eis_open_dmp()
 *          @endif
 *          and inv_dmp_start() must have been called.
 *
 *  @param flag The flag to check.
 *
 * @return TRUE or FALSE state of the flag
 */
int inv_check_flag(int flag)
{
    INVENSENSE_FUNC_START;
    int flagReturn = inv_obj.flags[flag];

    inv_obj.flags[flag] = 0;
    return flagReturn;
}

/**
 *  @brief  Enable generation of the DMP interrupt when Motion or no-motion
 *          is detected
 *  @param on
 *          Boolean to turn the interrupt on or off.
 *  @return INV_SUCCESS or non-zero error code.
 */
inv_error_t inv_set_motion_interrupt(unsigned char on)
{
    INVENSENSE_FUNC_START;
    inv_error_t result;
    unsigned char regs[2] = { 0 };

    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (on) {
        result = inv_get_dl_cfg_int(BIT_DMP_INT_EN);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
        inv_obj.interrupt_sources |= INV_INT_MOTION;
    } else {
        inv_obj.interrupt_sources &= ~INV_INT_MOTION;
        if (!inv_obj.interrupt_sources) {
            result = inv_get_dl_cfg_int(0);
            if (result) {
                LOG_RESULT_LOCATION(result);
                return result;
            }
        }
    }

    if (on) {
        regs[0] = DINAFE;
    } else {
        regs[0] = DINAD8;
    }
    result = inv_set_mpu_memory(KEY_CFG_7, 1, regs);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    return result;
}

/**
 * Enable generation of the DMP interrupt when a FIFO packet is ready
 *
 * @param on Boolean to turn the interrupt on or off
 *
 * @return INV_SUCCESS or non-zero error code
 */
inv_error_t inv_set_fifo_interrupt(unsigned char on)
{
    INVENSENSE_FUNC_START;
    inv_error_t result;
    unsigned char regs[2] = { 0 };

    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (on) {
        result = inv_get_dl_cfg_int(BIT_DMP_INT_EN);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
        inv_obj.interrupt_sources |= INV_INT_FIFO;
    } else {
        inv_obj.interrupt_sources &= ~INV_INT_FIFO;
        if (!inv_obj.interrupt_sources) {
            result = inv_get_dl_cfg_int(0);
            if (result) {
                LOG_RESULT_LOCATION(result);
                return result;
            }
        }
    }

    if (on) {
        regs[0] = DINAFE;
    } else {
        regs[0] = DINAD8;
    }
    result = inv_set_mpu_memory(KEY_CFG_6, 1, regs);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    return result;
}

/**
 * @brief   Get the current set of DMP interrupt sources.
 *          These interrupts are generated by the DMP and can be
 *          routed to the MPU interrupt line via internal
 *          settings.
 *
 *  @pre    inv_dmp_open()
 *          @ifnot MPL_MF
 *              or inv_open_low_power_pedometer()
 *              or inv_eis_open_dmp()
 *          @endif
 *          must have been called.
 *
 * @return  Currently enabled interrupt sources.  The possible interrupts are:
 *          - INV_INT_FIFO,
 *          - INV_INT_MOTION,
 *          - INV_INT_TAP
 */
int inv_get_interrupts(void)
{
    INVENSENSE_FUNC_START;

    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return 0;               // error

    return inv_obj.interrupt_sources;
}

/**
 *  @brief  Sets up the Accelerometer calibration and scale factor.
 *
 *          Please refer to the provided "9-Axis Sensor Fusion Application
 *          Note" document provided.  Section 5, "Sensor Mounting Orientation"
 *          offers a good coverage on the mounting matrices and explains how
 *          to use them.
 *
 *  @pre    inv_dmp_open()
 *          @ifnot MPL_MF
 *              or inv_open_low_power_pedometer()
 *              or inv_eis_open_dmp()
 *          @endif
 *          must have been called.
 *  @pre    inv_dmp_start() must <b>NOT</b> have been called.
 *
 *  @see    inv_set_gyro_calibration().
 *  @see    inv_set_compass_calibration().
 *
 *  @param[in]  range
 *                  The range of the accelerometers in g's. An accelerometer
 *                  that has a range of +2g's to -2g's should pass in 2.
 *  @param[in]  orientation
 *                  A 9 element matrix that represents how the accelerometers
 *                  are oriented with respect to the device they are mounted
 *                  in and the reference axis system.
 *                  A typical set of values are {1, 0, 0, 0, 1, 0, 0, 0, 1}.
 *                  This example corresponds to a 3 x 3 identity matrix.
 *
 *  @return INV_SUCCESS if successful; a non-zero error code otherwise.
 */
inv_error_t inv_set_accel_calibration(float range, signed char *orientation)
{
    INVENSENSE_FUNC_START;
    float cal[9];
    float scale = range / 32768.f;
    int kk;
    unsigned long sf;
    inv_error_t result;
    unsigned char regs[4] = { 0, 0, 0, 0 };
    struct mldl_cfg *mldl_cfg = inv_get_dl_config();

    if (inv_get_state() != INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    /* Apply zero g-offset values */
    if (ACCEL_ID_KXSD9 == mldl_cfg->accel->id) {
        regs[0] = 0x80;
        regs[1] = 0x0;
        regs[2] = 0x80;
        regs[3] = 0x0;
    }

    if (inv_dmpkey_supported(KEY_D_1_152)) {
        result = inv_set_mpu_memory(KEY_D_1_152, 4, &regs[0]);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
    }

    if (scale == 0) {
        inv_obj.accel_sens = 0;
    }

    if (mldl_cfg->accel->id) {
#if defined CONFIG_MPU_SENSORS_MPU6050A2 || \
    defined CONFIG_MPU_SENSORS_MPU6050B1
        unsigned char tmp[3] = { DINA0C, DINAC9, DINA2C };
#else
        unsigned char tmp[3] = { DINA4C, DINACD, DINA6C };
#endif
        struct mldl_cfg *mldl_cfg = inv_get_dl_config();
        unsigned char regs[3];
        unsigned short orient;

        for (kk = 0; kk < 9; ++kk) {
            cal[kk] = scale * orientation[kk];
            inv_obj.accel_cal[kk] = (long)(cal[kk] * (float)(1L << 30));
        }

        orient = inv_orientation_matrix_to_scalar(orientation);
        regs[0] = tmp[orient & 3];
        regs[1] = tmp[(orient >> 3) & 3];
        regs[2] = tmp[(orient >> 6) & 3];
        result = inv_set_mpu_memory(KEY_FCFG_2, 3, regs);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }

        regs[0] = DINA26;
        regs[1] = DINA46;
#if defined CONFIG_MPU_SENSORS_MPU3050 || defined CONFIG_MPU_SENSORS_MPU6050A2
        regs[2] = DINA66;
#else
        if (MPL_PROD_KEY(mldl_cfg->product_id, mldl_cfg->product_revision)
                == MPU_PRODUCT_KEY_B1_E1_5)
            regs[2] = DINA76;
        else
            regs[2] = DINA66;
#endif
        if (orient & 4)
            regs[0] |= 1;
        if (orient & 0x20)
            regs[1] |= 1;
        if (orient & 0x100)
            regs[2] |= 1;

        result = inv_set_mpu_memory(KEY_FCFG_7, 3, regs);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }

        if (mldl_cfg->accel->id == ACCEL_ID_MMA845X) {
            result = inv_freescale_sensor_fusion_16bit(orient);
            if (result) {
                LOG_RESULT_LOCATION(result);
                return result;
            }
        } else if (mldl_cfg->accel->id == ACCEL_ID_MMA8450) {
            result = inv_freescale_sensor_fusion_8bit(orient);
            if (result) {
                LOG_RESULT_LOCATION(result);
                return result;
            }
        }
    }

    if (inv_obj.accel_sens != 0) {
        sf = (1073741824L / inv_obj.accel_sens);
    } else {
        sf = 0;
    }
    regs[0] = (unsigned char)((sf >> 8) & 0xff);
    regs[1] = (unsigned char)(sf & 0xff);
    result = inv_set_mpu_memory(KEY_D_0_108, 2, regs);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    return result;
}

/**
 *  @brief  Sets up the Gyro calibration and scale factor.
 *
 *          Please refer to the provided "9-Axis Sensor Fusion Application
 *          Note" document provided.  Section 5, "Sensor Mounting Orientation"
 *          offers a good coverage on the mounting matrices and explains
 *          how to use them.
 *
 *  @pre    inv_dmp_open()
 *          @ifnot MPL_MF
 *              or inv_open_low_power_pedometer()
 *              or inv_eis_open_dmp()
 *          @endif
 *          must have been called.
 *  @pre    inv_dmp_start() must have <b>NOT</b> been called.
 *
 *  @see    inv_set_accel_calibration().
 *  @see    inv_set_compass_calibration().
 *
 *  @param[in]  range
 *                  The range of the gyros in degrees per second. A gyro
 *                  that has a range of +2000 dps to -2000 dps should pass in
 *                  2000.
 *  @param[in] orientation
 *                  A 9 element matrix that represents how the gyro are oriented
 *                  with respect to the device they are mounted in. A typical
 *                  set of values are {1, 0, 0, 0, 1, 0, 0, 0, 1}. This
 *                  example corresponds to a 3 x 3 identity matrix.
 *
 *  @return INV_SUCCESS if successful or Non-zero error code otherwise.
 */
inv_error_t inv_set_gyro_calibration(float range, signed char *orientation)
{
    INVENSENSE_FUNC_START;

    struct mldl_cfg *mldl_cfg = inv_get_dl_config();
    int kk;
    float scale;
    inv_error_t result;

    unsigned char regs[12] = { 0 };
    unsigned char maxVal = 0;
    unsigned char tmpPtr = 0;
    unsigned char tmpSign = 0;
    unsigned char i = 0;
    int sf = 0;

    if (inv_get_state() != INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (mldl_cfg->gyro_sens_trim != 0) {
        /* adjust the declared range to consider the gyro_sens_trim
           of this part when different from the default (first dividend) */
        range *= (32768.f / 250) / mldl_cfg->gyro_sens_trim;
    }

    scale = range / 32768.f;    // inverse of sensitivity for the given full scale range
    inv_obj.gyro_sens = (long)(range * 32768L);

    for (kk = 0; kk < 9; ++kk) {
        inv_obj.gyro_cal[kk] = (long)(scale * orientation[kk] * (1L << 30));
        inv_obj.gyro_orient[kk] = (long)((double)orientation[kk] * (1L << 30));
    }

    {
#if defined CONFIG_MPU_SENSORS_MPU6050A2 || \
    defined CONFIG_MPU_SENSORS_MPU6050B1
        unsigned char tmpD = DINA4C;
        unsigned char tmpE = DINACD;
        unsigned char tmpF = DINA6C;
#else
        unsigned char tmpD = DINAC9;
        unsigned char tmpE = DINA2C;
        unsigned char tmpF = DINACB;
#endif
        regs[3] = DINA36;
        regs[4] = DINA56;
        regs[5] = DINA76;

        for (i = 0; i < 3; i++) {
            maxVal = 0;
            tmpSign = 0;
            if (inv_obj.gyro_orient[0 + 3 * i] < 0)
                tmpSign = 1;
            if (ABS(inv_obj.gyro_orient[1 + 3 * i]) >
                ABS(inv_obj.gyro_orient[0 + 3 * i])) {
                maxVal = 1;
                if (inv_obj.gyro_orient[1 + 3 * i] < 0)
                    tmpSign = 1;
            }
            if (ABS(inv_obj.gyro_orient[2 + 3 * i]) >
                ABS(inv_obj.gyro_orient[1 + 3 * i])) {
                tmpSign = 0;
                maxVal = 2;
                if (inv_obj.gyro_orient[2 + 3 * i] < 0)
                    tmpSign = 1;
            }
            if (maxVal == 0) {
                regs[tmpPtr++] = tmpD;
                if (tmpSign)
                    regs[tmpPtr + 2] |= 0x01;
            } else if (maxVal == 1) {
                regs[tmpPtr++] = tmpE;
                if (tmpSign)
                    regs[tmpPtr + 2] |= 0x01;
            } else {
                regs[tmpPtr++] = tmpF;
                if (tmpSign)
                    regs[tmpPtr + 2] |= 0x01;
            }
        }
        result = inv_set_mpu_memory(KEY_FCFG_1, 3, regs);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
        result = inv_set_mpu_memory(KEY_FCFG_3, 3, &regs[3]);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }

        //sf = (gyroSens) * (0.5 * (pi/180) / 200.0) * 16384
        inv_obj.gyro_sf =
            (long)(((long long)inv_obj.gyro_sens * 767603923LL) / 1073741824LL);
        result =
            inv_set_mpu_memory(KEY_D_0_104, 4,
                               inv_int32_to_big8(inv_obj.gyro_sf, regs));
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }

        if (inv_obj.gyro_sens != 0) {
            sf = (long)((long long)23832619764371LL / inv_obj.gyro_sens);
        } else {
            sf = 0;
        }

        result = inv_set_mpu_memory(KEY_D_0_24, 4, inv_int32_to_big8(sf, regs));
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
    }
    return INV_SUCCESS;
}

/**
 *  @brief  Sets up the Compass calibration and scale factor.
 *
 *          Please refer to the provided "9-Axis Sensor Fusion Application
 *          Note" document provided.  Section 5, "Sensor Mounting Orientation"
 *          offers a good coverage on the mounting matrices and explains
 *          how to use them.
 *
 *  @pre    inv_dmp_open()
 *          @ifnot MPL_MF
 *              or inv_open_low_power_pedometer()
 *              or inv_eis_open_dmp()
 *          @endif
 *          must have been called.
 *  @pre    inv_dmp_start() must have <b>NOT</b> been called.
 *
 *  @see    inv_set_gyro_calibration().
 *  @see    inv_set_accel_calibration().
 *
 *  @param[in] range
 *                  The range of the compass.
 *  @param[in] orientation
 *                  A 9 element matrix that represents how the compass is
 *                  oriented with respect to the device they are mounted in.
 *                  A typical set of values are {1, 0, 0, 0, 1, 0, 0, 0, 1}.
 *                  This example corresponds to a 3 x 3 identity matrix.
 *                  The matrix describes how to go from the chip mounting to
 *                  the body of the device.
 *
 *  @return INV_SUCCESS if successful or Non-zero error code otherwise.
 */
inv_error_t inv_set_compass_calibration(float range, signed char *orientation)
{
    INVENSENSE_FUNC_START;
    float cal[9];
    float scale = range / 32768.f;
    int kk;
    unsigned short compassId = 0;

    compassId = inv_get_compass_id();

    if ((compassId == COMPASS_ID_YAS529) || (compassId == COMPASS_ID_HMC5883)
        || (compassId == COMPASS_ID_LSM303M)) {
        scale /= 32.0f;
    }

    for (kk = 0; kk < 9; ++kk) {
        cal[kk] = scale * orientation[kk];
        inv_obj.compass_cal[kk] = (long)(cal[kk] * (float)(1L << 30));
    }

    inv_obj.compass_sens = (long)(scale * 1073741824L);

    if (inv_dmpkey_supported(KEY_CPASS_MTX_00)) {
        unsigned char reg0[4] = { 0, 0, 0, 0 };
        unsigned char regp[4] = { 64, 0, 0, 0 };
        unsigned char regn[4] = { 64 + 128, 0, 0, 0 };
        unsigned char *reg;
        int_fast8_t kk;
        unsigned short keyList[9] =
            { KEY_CPASS_MTX_00, KEY_CPASS_MTX_01, KEY_CPASS_MTX_02,
            KEY_CPASS_MTX_10, KEY_CPASS_MTX_11, KEY_CPASS_MTX_12,
            KEY_CPASS_MTX_20, KEY_CPASS_MTX_21, KEY_CPASS_MTX_22
        };

        for (kk = 0; kk < 9; ++kk) {

            if (orientation[kk] == 1)
                reg = regp;
            else if (orientation[kk] == -1)
                reg = regn;
            else
                reg = reg0;
            inv_set_mpu_memory(keyList[kk], 4, reg);
        }
    }

    return INV_SUCCESS;
}

/**
* @internal
* @brief Sets the Gyro Dead Zone based upon LPF filter settings and Control setup.
*/
inv_error_t inv_set_dead_zone(void)
{
    unsigned char reg;
    inv_error_t result;
    extern struct control_params cntrl_params;

    if (cntrl_params.functions & INV_DEAD_ZONE) {
        reg = 0x08;
    } else {
#if defined CONFIG_MPU_SENSORS_MPU6050A2 || \
    defined CONFIG_MPU_SENSORS_MPU6050B1
        reg = 0;
#else
        if (inv_params_obj.bias_mode & INV_BIAS_FROM_LPF) {
            reg = 0x2;
        } else {
            reg = 0;
        }
#endif
    }

    result = inv_set_mpu_memory(KEY_D_0_163, 1, &reg);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    return result;
}

/**
 *  @brief  inv_set_bias_update is used to register which algorithms will be
 *          used to automatically reset the gyroscope bias.
 *          The engine INV_BIAS_UPDATE must be enabled for these algorithms to
 *          run.
 *
 *  @pre    inv_dmp_open()
 *          @ifnot MPL_MF
 *              or inv_open_low_power_pedometer()
 *              or inv_eis_open_dmp()
 *          @endif
 *          and inv_dmp_start()
 *          must <b>NOT</b> have been called.
 *
 *  @param  function    A function or bitwise OR of functions that determine
 *                      how the gyroscope bias will be automatically updated.
 *                      Functions include:
 *                      - INV_NONE or 0,
 *                      - INV_BIAS_FROM_NO_MOTION,
 *                      - INV_BIAS_FROM_GRAVITY,
 *                      - INV_BIAS_FROM_TEMPERATURE,
                    \ifnot UMPL
 *                      - INV_BIAS_FROM_LPF,
 *                      - INV_MAG_BIAS_FROM_MOTION,
 *                      - INV_MAG_BIAS_FROM_GYRO,
 *                      - INV_ALL.
 *                   \endif
 *  @return INV_SUCCESS if successful or Non-zero error code otherwise.
 */
inv_error_t inv_set_bias_update(unsigned short function)
{
    INVENSENSE_FUNC_START;
    unsigned char regs[4];
    long tmp[3] = { 0, 0, 0 };
    inv_error_t result = INV_SUCCESS;
    struct mldl_cfg *mldl_cfg = inv_get_dl_config();

    if (inv_get_state() != INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    /* do not allow progressive no motion bias tracker to run -
       it's not fully debugged */
    function &= ~INV_PROGRESSIVE_NO_MOTION; // FIXME, workaround
    MPL_LOGV("forcing disable of PROGRESSIVE_NO_MOTION bias tracker\n");

    // You must use EnableFastNoMotion() to get this feature
    function &= ~INV_BIAS_FROM_FAST_NO_MOTION;

    // You must use DisableFastNoMotion() to turn this feature off
    if (inv_params_obj.bias_mode & INV_BIAS_FROM_FAST_NO_MOTION)
        function |= INV_BIAS_FROM_FAST_NO_MOTION;

    /*--- remove magnetic components from bias tracking
          if there is no compass ---*/
    if (!inv_compass_present()) {
        function &= ~(INV_MAG_BIAS_FROM_GYRO | INV_MAG_BIAS_FROM_MOTION);
    } else {
        function &= ~(INV_BIAS_FROM_LPF);
    }

    // Enable function sets bias from no motion
    inv_params_obj.bias_mode = function & (~INV_BIAS_FROM_NO_MOTION);

    if (function & INV_BIAS_FROM_NO_MOTION) {
        inv_enable_bias_no_motion();
    } else {
        inv_disable_bias_no_motion();
    }

    if (inv_params_obj.bias_mode & INV_BIAS_FROM_LPF) {
        regs[0] = DINA80 + 2;
        regs[1] = DINA2D;
        regs[2] = DINA55;
        regs[3] = DINA7D;
    } else {
        regs[0] = DINA80 + 7;
        regs[1] = DINA2D;
        regs[2] = DINA35;
        regs[3] = DINA3D;
    }
    result = inv_set_mpu_memory(KEY_FCFG_5, 4, regs);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    result = inv_set_dead_zone();
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    if ((inv_params_obj.bias_mode & INV_BIAS_FROM_GRAVITY) &&
        !inv_compass_present()) {
        result = inv_set_gyro_data_source(INV_GYRO_FROM_QUATERNION);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
    } else {
        result = inv_set_gyro_data_source(INV_GYRO_FROM_RAW);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
    }

    inv_obj.factory_temp_comp = 0;  // FIXME, workaround
    if ((mldl_cfg->offset_tc[0] != 0) ||
        (mldl_cfg->offset_tc[1] != 0) || (mldl_cfg->offset_tc[2] != 0)) {
        inv_obj.factory_temp_comp = 1;
    }

    if (inv_obj.factory_temp_comp == 0) {
        if (inv_params_obj.bias_mode & INV_BIAS_FROM_TEMPERATURE) {
            result = inv_set_gyro_temp_slope(inv_obj.temp_slope);
            if (result) {
                LOG_RESULT_LOCATION(result);
                return result;
            }
        } else {
            result = inv_set_gyro_temp_slope(tmp);
            if (result) {
                LOG_RESULT_LOCATION(result);
                return result;
            }
        }
    } else {
        inv_params_obj.bias_mode &= ~INV_LEARN_BIAS_FROM_TEMPERATURE;
        MPL_LOGV("factory temperature compensation coefficients available - "
                 "disabling INV_LEARN_BIAS_FROM_TEMPERATURE\n");
    }

    /*---- hard requirement for using bias tracking BIAS_FROM_GRAVITY, relying on
           compass and accel data, is to have accelerometer data delivered in the
           FIFO ----*/
    if (((inv_params_obj.bias_mode & INV_BIAS_FROM_GRAVITY)
         && inv_compass_present())
        || (inv_params_obj.bias_mode & INV_MAG_BIAS_FROM_GYRO)
        || (inv_params_obj.bias_mode & INV_MAG_BIAS_FROM_MOTION)) {
        inv_send_accel(INV_ALL, INV_32_BIT);
        inv_send_gyro(INV_ALL, INV_32_BIT);
    }

    return result;
}

/**
 *  @brief  inv_get_motion_state is used to determine if the device is in
 *          a 'motion' or 'no motion' state.
 *          inv_get_motion_state returns INV_MOTION of the device is moving,
 *          or INV_NO_MOTION if the device is not moving.
 *
 *  @pre    inv_dmp_open()
 *          @ifnot MPL_MF
 *              or inv_open_low_power_pedometer()
 *              or inv_eis_open_dmp()
 *          @endif
 *          and inv_dmp_start()
 *          must have been called.
 *
 *  @return INV_SUCCESS if successful or Non-zero error code otherwise.
 */
int inv_get_motion_state(void)
{
    INVENSENSE_FUNC_START;
    return inv_obj.motion_state;
}

/**
 *  @brief  inv_set_no_motion_thresh is used to set the threshold for
 *          detecting INV_NO_MOTION
 *
 *  @pre    inv_dmp_open()
 *          @ifnot MPL_MF
 *              or inv_open_low_power_pedometer()
 *              or inv_eis_open_dmp()
 *          @endif
 *          must have been called.
 *
 *  @param  thresh  A threshold scaled in degrees per second.
 *
 *  @return INV_SUCCESS if successful or Non-zero error code otherwise.
 */
inv_error_t inv_set_no_motion_thresh(float thresh)
{
    inv_error_t result = INV_SUCCESS;
    unsigned char regs[4] = { 0 };
    long tmp;
    INVENSENSE_FUNC_START;

    tmp = (long)(thresh * thresh * 2.045f);
    if (tmp < 0) {
        return INV_ERROR;
    } else if (tmp > 8180000L) {
        return INV_ERROR;
    }
    inv_obj.no_motion_threshold = tmp;

    regs[0] = (unsigned char)(tmp >> 24);
    regs[1] = (unsigned char)((tmp >> 16) & 0xff);
    regs[2] = (unsigned char)((tmp >> 8) & 0xff);
    regs[3] = (unsigned char)(tmp & 0xff);

    result = inv_set_mpu_memory(KEY_D_1_108, 4, regs);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    result = inv_reset_motion();
    return result;
}

/**
 *  @brief  inv_set_no_motion_threshAccel is used to set the threshold for
 *          detecting INV_NO_MOTION with accelerometers when Gyros have
 *          been turned off
 *
 *  @pre    inv_dmp_open()
 *          @ifnot MPL_MF
 *              or inv_open_low_power_pedometer()
 *              or inv_eis_open_dmp()
 *          @endif
 *          must have been called.
 *
 *  @param  thresh  A threshold in g's scaled by 2^32
 *
 *  @return INV_SUCCESS if successful or Non-zero error code otherwise.
 */
inv_error_t inv_set_no_motion_threshAccel(long thresh)
{
    INVENSENSE_FUNC_START;

    inv_obj.no_motion_accel_threshold = thresh;

    return INV_SUCCESS;
}

/**
 *  @brief  inv_set_no_motion_time is used to set the time required for
 *          detecting INV_NO_MOTION
 *
 *  @pre    inv_dmp_open()
 *          @ifnot MPL_MF
 *              or inv_open_low_power_pedometer()
 *              or inv_eis_open_dmp()
 *          @endif
 *          must have been called.
 *
 *  @param  time    A time in seconds.
 *
 *  @return INV_SUCCESS if successful or Non-zero error code otherwise.
 */
inv_error_t inv_set_no_motion_time(float time)
{
    inv_error_t result = INV_SUCCESS;
    unsigned char regs[2] = { 0 };
    long tmp;

    INVENSENSE_FUNC_START;

    tmp = (long)(time * 200);
    if (tmp < 0) {
        return INV_ERROR;
    } else if (tmp > 65535L) {
        return INV_ERROR;
    }
    inv_obj.motion_duration = (unsigned short)tmp;

    regs[0] = (unsigned char)((inv_obj.motion_duration >> 8) & 0xff);
    regs[1] = (unsigned char)(inv_obj.motion_duration & 0xff);
    result = inv_set_mpu_memory(KEY_D_1_106, 2, regs);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    result = inv_reset_motion();
    return result;
}

/**
 *  @brief  inv_get_version is used to get the ML version.
 *
 *  @pre    inv_get_version can be called at any time.
 *
 *  @param  version     inv_get_version writes the ML version
 *                      string pointer to version.
 *
 *  @return INV_SUCCESS if successful or Non-zero error code otherwise.
 */
inv_error_t inv_get_version(unsigned char **version)
{
    INVENSENSE_FUNC_START;

    *version = (unsigned char *)mlVer;  //fixme we are wiping const

    return INV_SUCCESS;
}

/**
 * @brief Check for the presence of the gyro sensor.
 *
 * This is not a physical check but a logical check and the value can change
 * dynamically based on calls to inv_set_mpu_sensors().
 *
 * @return  TRUE if the gyro is enabled FALSE otherwise.
 */
int inv_get_gyro_present(void)
{
    return inv_get_dl_config()->requested_sensors & (INV_X_GYRO | INV_Y_GYRO |
                                                     INV_Z_GYRO);
}

static unsigned short inv_row_2_scale(const signed char *row)
{
    unsigned short b;

    if (row[0] > 0)
        b = 0;
    else if (row[0] < 0)
        b = 4;
    else if (row[1] > 0)
        b = 1;
    else if (row[1] < 0)
        b = 5;
    else if (row[2] > 0)
        b = 2;
    else if (row[2] < 0)
        b = 6;
    else
        b = 7;                  // error
    return b;
}

unsigned short inv_orientation_matrix_to_scalar(const signed char *mtx)
{
    unsigned short scalar;
    /*
       XYZ  010_001_000 Identity Matrix
       XZY  001_010_000
       YXZ  010_000_001
       YZX  000_010_001
       ZXY  001_000_010
       ZYX  000_001_010
     */

    scalar = inv_row_2_scale(mtx);
    scalar |= inv_row_2_scale(mtx + 3) << 3;
    scalar |= inv_row_2_scale(mtx + 6) << 6;

    return scalar;
}

/* Setups up the Freescale 16-bit accel for Sensor Fusion
* @param[in] orient A scalar representation of the orientation
*  that describes how to go from the Chip Orientation
*  to the Board Orientation often times called the Body Orientation in Invensense Documentation.
*  inv_orientation_matrix_to_scalar() will turn the transformation matrix into this scalar.
*/
inv_error_t inv_freescale_sensor_fusion_16bit(unsigned short orient)
{
    unsigned char rr[3];
    inv_error_t result;

    orient = orient & 0xdb;
    switch (orient) {
    default:
        // Typically 0x88
        rr[0] = DINACC;
        rr[1] = DINACF;
        rr[2] = DINA0E;
        break;
    case 0x50:
        rr[0] = DINACE;
        rr[1] = DINA0E;
        rr[2] = DINACD;
        break;
    case 0x81:
        rr[0] = DINACE;
        rr[1] = DINACB;
        rr[2] = DINA0E;
        break;
    case 0x11:
        rr[0] = DINACC;
        rr[1] = DINA0E;
        rr[2] = DINACB;
        break;
    case 0x42:
        rr[0] = DINA0A;
        rr[1] = DINACF;
        rr[2] = DINACB;
        break;
    case 0x0a:
        rr[0] = DINA0A;
        rr[1] = DINACB;
        rr[2] = DINACD;
        break;
    }
    result = inv_set_mpu_memory(KEY_FCFG_AZ, 3, rr);
    return result;
}

/* Setups up the Freescale 8-bit accel for Sensor Fusion
* @param[in] orient A scalar representation of the orientation
*  that describes how to go from the Chip Orientation
*  to the Board Orientation often times called the Body Orientation in Invensense Documentation.
*  inv_orientation_matrix_to_scalar() will turn the transformation matrix into this scalar.
*/
inv_error_t inv_freescale_sensor_fusion_8bit(unsigned short orient)
{
    unsigned char regs[27];
    inv_error_t result;
    uint_fast8_t kk;

    orient = orient & 0xdb;
    kk = 0;

    regs[kk++] = DINAC3;
    regs[kk++] = DINA90 + 14;
    regs[kk++] = DINAA0 + 9;
    regs[kk++] = DINA3E;
    regs[kk++] = DINA5E;
    regs[kk++] = DINA7E;

    regs[kk++] = DINAC2;
    regs[kk++] = DINAA0 + 9;
    regs[kk++] = DINA90 + 9;
    regs[kk++] = DINAF8 + 2;

    switch (orient) {
    default:
        // Typically 0x88
        regs[kk++] = DINACB;

        regs[kk++] = DINA54;
        regs[kk++] = DINA50;
        regs[kk++] = DINA50;
        regs[kk++] = DINA50;
        regs[kk++] = DINA50;
        regs[kk++] = DINA50;
        regs[kk++] = DINA50;
        regs[kk++] = DINA50;

        regs[kk++] = DINACD;
        break;
    case 0x50:
        regs[kk++] = DINACB;

        regs[kk++] = DINACF;

        regs[kk++] = DINA7C;
        regs[kk++] = DINA78;
        regs[kk++] = DINA78;
        regs[kk++] = DINA78;
        regs[kk++] = DINA78;
        regs[kk++] = DINA78;
        regs[kk++] = DINA78;
        regs[kk++] = DINA78;
        break;
    case 0x81:
        regs[kk++] = DINA2C;
        regs[kk++] = DINA28;
        regs[kk++] = DINA28;
        regs[kk++] = DINA28;
        regs[kk++] = DINA28;
        regs[kk++] = DINA28;
        regs[kk++] = DINA28;
        regs[kk++] = DINA28;

        regs[kk++] = DINACD;

        regs[kk++] = DINACB;
        break;
    case 0x11:
        regs[kk++] = DINA2C;
        regs[kk++] = DINA28;
        regs[kk++] = DINA28;
        regs[kk++] = DINA28;
        regs[kk++] = DINA28;
        regs[kk++] = DINA28;
        regs[kk++] = DINA28;
        regs[kk++] = DINA28;
        regs[kk++] = DINACB;
        regs[kk++] = DINACF;
        break;
    case 0x42:
        regs[kk++] = DINACF;
        regs[kk++] = DINACD;

        regs[kk++] = DINA7C;
        regs[kk++] = DINA78;
        regs[kk++] = DINA78;
        regs[kk++] = DINA78;
        regs[kk++] = DINA78;
        regs[kk++] = DINA78;
        regs[kk++] = DINA78;
        regs[kk++] = DINA78;
        break;
    case 0x0a:
        regs[kk++] = DINACD;

        regs[kk++] = DINA54;
        regs[kk++] = DINA50;
        regs[kk++] = DINA50;
        regs[kk++] = DINA50;
        regs[kk++] = DINA50;
        regs[kk++] = DINA50;
        regs[kk++] = DINA50;
        regs[kk++] = DINA50;

        regs[kk++] = DINACF;
        break;
    }

    regs[kk++] = DINA90 + 7;
    regs[kk++] = DINAF8 + 3;
    regs[kk++] = DINAA0 + 9;
    regs[kk++] = DINA0E;
    regs[kk++] = DINA0E;
    regs[kk++] = DINA0E;

    regs[kk++] = DINAF8 + 1;    // filler

    result = inv_set_mpu_memory(KEY_FCFG_FSCALE, kk, regs);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    return result;
}

/**
 * Controlls each sensor and each axis when the motion processing unit is
 * running.  When it is not running, simply records the state for later.
 *
 * NOTE: In this version only full sensors controll is allowed.  Independent
 * axis control will return an error.
 *
 * @param sensors Bit field of each axis desired to be turned on or off
 *
 * @return INV_SUCCESS or non-zero error code
 */
inv_error_t inv_set_mpu_sensors(unsigned long sensors)
{
    INVENSENSE_FUNC_START;
    unsigned char state = inv_get_state();
    struct mldl_cfg *mldl_cfg = inv_get_dl_config();
    inv_error_t result = INV_SUCCESS;
    unsigned short fifoRate;

    if (state < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (((sensors & INV_THREE_AXIS_ACCEL) != INV_THREE_AXIS_ACCEL) &&
        ((sensors & INV_THREE_AXIS_ACCEL) != 0)) {
        return INV_ERROR_FEATURE_NOT_IMPLEMENTED;
    }
    if (((sensors & INV_THREE_AXIS_ACCEL) != 0) &&
        (mldl_cfg->pdata->accel.get_slave_descr == 0)) {
        return INV_ERROR_SERIAL_DEVICE_NOT_RECOGNIZED;
    }

    if (((sensors & INV_THREE_AXIS_COMPASS) != INV_THREE_AXIS_COMPASS) &&
        ((sensors & INV_THREE_AXIS_COMPASS) != 0)) {
        return INV_ERROR_FEATURE_NOT_IMPLEMENTED;
    }
    if (((sensors & INV_THREE_AXIS_COMPASS) != 0) &&
        (mldl_cfg->pdata->compass.get_slave_descr == 0)) {
        return INV_ERROR_SERIAL_DEVICE_NOT_RECOGNIZED;
    }

    if (((sensors & INV_THREE_AXIS_PRESSURE) != INV_THREE_AXIS_PRESSURE) &&
        ((sensors & INV_THREE_AXIS_PRESSURE) != 0)) {
        return INV_ERROR_FEATURE_NOT_IMPLEMENTED;
    }
    if (((sensors & INV_THREE_AXIS_PRESSURE) != 0) &&
        (mldl_cfg->pdata->pressure.get_slave_descr == 0)) {
        return INV_ERROR_SERIAL_DEVICE_NOT_RECOGNIZED;
    }

    /* DMP was off, and is turning on */
    if (sensors & INV_DMP_PROCESSOR &&
        !(mldl_cfg->requested_sensors & INV_DMP_PROCESSOR)) {
        struct ext_slave_config config;
        long odr;
        config.key = MPU_SLAVE_CONFIG_ODR_RESUME;
        config.len = sizeof(long);
        config.apply = (state == INV_STATE_DMP_STARTED);
        config.data = &odr;

        odr = (inv_mpu_get_sampling_rate_hz(mldl_cfg) * 1000);
        result = inv_mpu_config_accel(mldl_cfg,
                                      inv_get_serial_handle(),
                                      inv_get_serial_handle(), &config);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }

        config.key = MPU_SLAVE_CONFIG_IRQ_RESUME;
        odr = MPU_SLAVE_IRQ_TYPE_NONE;
        result = inv_mpu_config_accel(mldl_cfg,
                                      inv_get_serial_handle(),
                                      inv_get_serial_handle(), &config);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
        inv_init_fifo_hardare();
    }

    if (inv_obj.mode_change_func) {
        result = inv_obj.mode_change_func(mldl_cfg->requested_sensors, sensors);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
    }

    /* Get the fifo rate before changing sensors so if we need to match it */
    fifoRate = inv_get_fifo_rate();
    mldl_cfg->requested_sensors = sensors;

    /* inv_dmp_start will turn the sensors on */
    if (state == INV_STATE_DMP_STARTED) {
        result = inv_dl_start(sensors);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
        result = inv_reset_motion();
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
        result = inv_dl_stop(~sensors);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
    }

    inv_set_fifo_rate(fifoRate);

    if (!(sensors & INV_DMP_PROCESSOR) && (sensors & INV_THREE_AXIS_ACCEL)) {
        struct ext_slave_config config;
        long data;

        config.len = sizeof(long);
        config.key = MPU_SLAVE_CONFIG_IRQ_RESUME;
        config.apply = (state == INV_STATE_DMP_STARTED);
        config.data = &data;
        data = MPU_SLAVE_IRQ_TYPE_DATA_READY;
        result = inv_mpu_config_accel(mldl_cfg,
                                      inv_get_serial_handle(),
                                      inv_get_serial_handle(), &config);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
    }

    return result;
}

void inv_set_mode_change(inv_error_t(*mode_change_func)
                         (unsigned long, unsigned long))
{
    inv_obj.mode_change_func = mode_change_func;
}

/**
* Mantis setup
*/
#ifdef CONFIG_MPU_SENSORS_MPU6050B1
inv_error_t inv_set_mpu_6050_config()
{
    long temp;
    inv_error_t result;
    unsigned char big8[4];
    unsigned char atc[4];
    long s[3], s2[3];
    int kk;
    struct mldl_cfg *mldlCfg = inv_get_dl_config();

    result = inv_serial_read(inv_get_serial_handle(), inv_get_mpu_slave_addr(),
                             0x0d, 4, atc);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    temp = atc[3] & 0x3f;
    if (temp >= 32)
        temp = temp - 64;
    temp = (temp << 21) | 0x100000;
    temp += (1L << 29);
    temp = -temp;
    result = inv_set_mpu_memory(KEY_D_ACT0, 4, inv_int32_to_big8(temp, big8));
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    for (kk = 0; kk < 3; ++kk) {
        s[kk] = atc[kk] & 0x3f;
        if (s[kk] > 32)
            s[kk] = s[kk] - 64;
        s[kk] *= 2516582L;
    }

    for (kk = 0; kk < 3; ++kk) {
        s2[kk] = mldlCfg->pdata->orientation[kk * 3] * s[0] +
            mldlCfg->pdata->orientation[kk * 3 + 1] * s[1] +
            mldlCfg->pdata->orientation[kk * 3 + 2] * s[2];
    }
    result = inv_set_mpu_memory(KEY_D_ACSX, 4, inv_int32_to_big8(s2[0], big8));
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    result = inv_set_mpu_memory(KEY_D_ACSY, 4, inv_int32_to_big8(s2[1], big8));
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    result = inv_set_mpu_memory(KEY_D_ACSZ, 4, inv_int32_to_big8(s2[2], big8));
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    return result;
}
#endif

/**
 * @}
 */
