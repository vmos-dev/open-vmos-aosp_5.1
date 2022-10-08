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
 * $Id:$
 *
 *****************************************************************************/

#include "mlBiasNoMotion.h"
#include "ml.h"
#include "mlinclude.h"
#include "mlos.h"
#include "mlFIFO.h"
#include "dmpKey.h"
#include "accel.h"
#include "mlMathFunc.h"
#include "mldl.h"
#include "mlstates.h"
#include "mlSetGyroBias.h"

#include "log.h"
#undef MPL_LOG_TAG
#define MPL_LOG_TAG "MPL-BiasNoMot"


#define _mlDebug(x)             //{x}

/**
 *  @brief  inv_set_motion_callback is used to register a callback function that
 *          will trigger when a change of motion state is detected.
 *
 *  @pre    inv_dmp_open() 
 *          @ifnot MPL_MF 
 *              or inv_open_low_power_pedometer() 
 *              or inv_eis_open_dmp()
 *          @endif
 *          and inv_dmp_start()
 *          must <b>NOT</b> have been called.
 *
 *  @param  func    A user defined callback function accepting a
 *                  motion_state parameter, the new motion state.
 *                  May be one of INV_MOTION or INV_NO_MOTION.
 *  @return INV_SUCCESS if successful or Non-zero error code otherwise.
 */
inv_error_t inv_set_motion_callback(void (*func) (unsigned short motion_state))
{
    INVENSENSE_FUNC_START;

    if ((inv_get_state() != INV_STATE_DMP_OPENED) &&
        (inv_get_state() != INV_STATE_DMP_STARTED))
        return INV_ERROR_SM_IMPROPER_STATE;

    inv_params_obj.motion_cb_func = func;

    return INV_SUCCESS;
}

#if defined CONFIG_MPU_SENSORS_MPU6050A2 || \
    defined CONFIG_MPU_SENSORS_MPU6050B1
/** Turns on the feature to compute gyro bias from No Motion */
inv_error_t inv_turn_on_bias_from_no_motion()
{
    inv_error_t result;
    unsigned char regs[3] = { 0x0d, DINA35, 0x5d };
    inv_params_obj.bias_mode |= INV_BIAS_FROM_NO_MOTION;
    result = inv_set_mpu_memory(KEY_CFG_MOTION_BIAS, 3, regs);
    return result;
}

/** Turns off the feature to compute gyro bias from No Motion
*/
inv_error_t inv_turn_off_bias_from_no_motion()
{
    inv_error_t result;
    unsigned char regs[3] = { DINA90 + 8, DINA90 + 8, DINA90 + 8 };
    inv_params_obj.bias_mode &= ~INV_BIAS_FROM_NO_MOTION;
    result = inv_set_mpu_memory(KEY_CFG_MOTION_BIAS, 3, regs);
    return result;
}
#endif

inv_error_t inv_update_bias(void)
{
    INVENSENSE_FUNC_START;
    inv_error_t result;
    unsigned char regs[12];
    short bias[GYRO_NUM_AXES];

    if ((inv_params_obj.bias_mode & INV_BIAS_FROM_NO_MOTION)
        && inv_get_gyro_present()) {

        regs[0] = DINAA0 + 3;
        result = inv_set_mpu_memory(KEY_FCFG_6, 1, regs);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }

        result = inv_get_mpu_memory(KEY_D_1_244, 12, regs);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }

        inv_convert_bias(regs, bias);

        regs[0] = DINAA0 + 15;
        result = inv_set_mpu_memory(KEY_FCFG_6, 1, regs);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }

        result = inv_set_gyro_bias_in_hw_unit(bias, INV_SGB_NO_MOTION);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }

        result =
            inv_serial_read(inv_get_serial_handle(), inv_get_mpu_slave_addr(),
                            MPUREG_TEMP_OUT_H, 2, regs);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
        result = inv_set_mpu_memory(KEY_DMP_PREVPTAT, 2, regs);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }

        inv_obj.got_no_motion_bias = TRUE;
    }
    return INV_SUCCESS;
}

inv_error_t MLAccelMotionDetection(struct inv_obj_t *inv_obj)
{
    long gain;
    unsigned long timeChange;
    long rate;
    inv_error_t result;
    long accel[3], temp;
    long long accelMag;
    unsigned long currentTime;
    int kk;

    if (!inv_accel_present()) {
        return INV_SUCCESS;
    }

    currentTime = inv_get_tick_count();

    // We always run the accel low pass filter at the highest sample rate possible
    result = inv_get_accel(accel);
    if (result != INV_ERROR_FEATURE_NOT_ENABLED) {
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
        rate = inv_get_fifo_rate() * 5 + 5;
        if (rate > 200)
            rate = 200;

        gain = inv_obj->accel_lpf_gain * rate;
        timeChange = inv_get_fifo_rate();

        accelMag = 0;
        for (kk = 0; kk < ACCEL_NUM_AXES; ++kk) {
            inv_obj->accel_lpf[kk] =
                inv_q30_mult(((1L << 30) - gain), inv_obj->accel_lpf[kk]);
            inv_obj->accel_lpf[kk] += inv_q30_mult(gain, accel[kk]);
            temp = accel[0] - inv_obj->accel_lpf[0];
            accelMag += (long long)temp *temp;
        }

        if (accelMag > inv_obj->no_motion_accel_threshold) {
            inv_obj->no_motion_accel_time = currentTime;

            // Check for change of state
            if (!inv_get_gyro_present())
                inv_set_motion_state(INV_MOTION);

        } else if ((currentTime - inv_obj->no_motion_accel_time) >
                   5 * inv_obj->motion_duration) {
            // We have no motion according to accel
            // Check fsor change of state
            if (!inv_get_gyro_present())
                inv_set_motion_state(INV_NO_MOTION);
        }
    }
    return INV_SUCCESS;
}

/**
 * @internal
 * @brief   Manually update the motion/no motion status.  This is a 
 *          convienence function for implementations that do not wish to use 
 *          inv_update_data.  
 *          This function can be called periodically to check for the 
 *          'no motion' state and update the internal motion status and bias 
 *          calculations.
 */
inv_error_t MLPollMotionStatus(struct inv_obj_t * inv_obj)
{
    INVENSENSE_FUNC_START;
    unsigned char regs[3] = { 0 };
    unsigned short motionFlag = 0;
    unsigned long currentTime;
    inv_error_t result;

    result = MLAccelMotionDetection(inv_obj);

    currentTime = inv_get_tick_count();

    // If it is not time to poll for a no motion event, return
    if (((inv_obj->interrupt_sources & INV_INT_MOTION) == 0) &&
        ((currentTime - inv_obj->poll_no_motion) <= 1000))
        return INV_SUCCESS;

    inv_obj->poll_no_motion = currentTime;

#if defined CONFIG_MPU_SENSORS_MPU3050
    if (inv_get_gyro_present()
        && ((inv_params_obj.bias_mode & INV_BIAS_FROM_FAST_NO_MOTION) == 0)) {
        static long repeatBiasUpdateTime = 0;

        result = inv_get_mpu_memory(KEY_D_1_98, 2, regs);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }

        motionFlag = (unsigned short)regs[0] * 256 + (unsigned short)regs[1];

        _mlDebug(MPL_LOGV("motionFlag from RAM : 0x%04X\n", motionFlag);
            )
            if (motionFlag == inv_obj->motion_duration) {
            if (inv_obj->motion_state == INV_MOTION) {
                inv_update_bias();
                repeatBiasUpdateTime = inv_get_tick_count();

                regs[0] = DINAD8 + 1;
                regs[1] = DINA0C;
                regs[2] = DINAD8 + 2;
                result = inv_set_mpu_memory(KEY_CFG_18, 3, regs);
                if (result) {
                    LOG_RESULT_LOCATION(result);
                    return result;
                }

                regs[0] = 0;
                regs[1] = 5;
                result = inv_set_mpu_memory(KEY_D_1_106, 2, regs);
                if (result) {
                    LOG_RESULT_LOCATION(result);
                    return result;
                }

                //Trigger no motion callback
                inv_set_motion_state(INV_NO_MOTION);
            }
        }
        if (motionFlag == 5) {
            if (inv_obj->motion_state == INV_NO_MOTION) {
                regs[0] = DINAD8 + 2;
                regs[1] = DINA0C;
                regs[2] = DINAD8 + 1;
                result = inv_set_mpu_memory(KEY_CFG_18, 3, regs);
                if (result) {
                    LOG_RESULT_LOCATION(result);
                    return result;
                }

                regs[0] =
                    (unsigned char)((inv_obj->motion_duration >> 8) & 0xff);
                regs[1] = (unsigned char)(inv_obj->motion_duration & 0xff);
                result = inv_set_mpu_memory(KEY_D_1_106, 2, regs);
                if (result) {
                    LOG_RESULT_LOCATION(result);
                    return result;
                }

                //Trigger no motion callback
                inv_set_motion_state(INV_MOTION);
            }
        }
        if (inv_obj->motion_state == INV_NO_MOTION) {
            if ((inv_get_tick_count() - repeatBiasUpdateTime) > 4000) {
                inv_update_bias();
                repeatBiasUpdateTime = inv_get_tick_count();
            }
        }
    }
#else                           // CONFIG_MPU_SENSORS_MPU3050
    if (inv_get_gyro_present()
        && ((inv_params_obj.bias_mode & INV_BIAS_FROM_FAST_NO_MOTION) == 0)) {
        result = inv_get_mpu_memory(KEY_D_1_98, 2, regs);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }

        motionFlag = (unsigned short)regs[0] * 256 + (unsigned short)regs[1];

        _mlDebug(MPL_LOGV("motionFlag from RAM : 0x%04X\n", motionFlag);
            )
            if (motionFlag > 0) {
            unsigned char biasReg[12];
            long biasTmp2[3], biasTmp[3];
            int i;

            if (inv_obj->last_motion != motionFlag) {
                result = inv_get_mpu_memory(KEY_D_2_96, 12, biasReg);

                for (i = 0; i < 3; i++) {
                    biasTmp2[i] = inv_big8_to_int32(&biasReg[i * 4]);
                }
                // Rotate bias vector by the transpose of the orientation matrix
                for (i = 0; i < 3; ++i) {
                    biasTmp[i] =
                        inv_q30_mult(biasTmp2[0],
                                     inv_obj->gyro_orient[i]) +
                        inv_q30_mult(biasTmp2[1],
                                     inv_obj->gyro_orient[i + 3]) +
                        inv_q30_mult(biasTmp2[2], inv_obj->gyro_orient[i + 6]);
                }
                inv_obj->gyro_bias[0] = inv_q30_mult(biasTmp[0], 1501974482L);
                inv_obj->gyro_bias[1] = inv_q30_mult(biasTmp[1], 1501974482L);
                inv_obj->gyro_bias[2] = inv_q30_mult(biasTmp[2], 1501974482L);
            }
            inv_set_motion_state(INV_NO_MOTION);
        } else {
            // We are in a motion state
            inv_set_motion_state(INV_MOTION);
        }
        inv_obj->last_motion = motionFlag;

    }
#endif                          // CONFIG_MPU_SENSORS_MPU3050
    return INV_SUCCESS;
}

inv_error_t inv_enable_bias_no_motion(void)
{
    inv_error_t result;
    inv_params_obj.bias_mode |= INV_BIAS_FROM_NO_MOTION;
    result =
        inv_register_fifo_rate_process(MLPollMotionStatus,
                                       INV_PRIORITY_BIAS_NO_MOTION);
#if defined CONFIG_MPU_SENSORS_MPU6050A2 || \
    defined CONFIG_MPU_SENSORS_MPU6050B1
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    result = inv_turn_on_bias_from_no_motion();
#endif
    return result;
}

inv_error_t inv_disable_bias_no_motion(void)
{
    inv_error_t result;
    inv_params_obj.bias_mode &= ~INV_BIAS_FROM_NO_MOTION;
    result = inv_unregister_fifo_rate_process(MLPollMotionStatus);
#if defined CONFIG_MPU_SENSORS_MPU6050A2 || \
    defined CONFIG_MPU_SENSORS_MPU6050B1
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    result = inv_turn_off_bias_from_no_motion();
#endif
    return result;
}
