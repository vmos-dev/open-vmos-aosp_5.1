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
 * $Id: mlsupervisor.c 5637 2011-06-14 01:13:53Z mcaramello $
 *
 *****************************************************************************/

/**
 *  @defgroup   ML_SUPERVISOR
 *  @brief      Basic sensor fusion supervisor functionalities.
 *
 *  @{
 *      @file   mlsupervisor.c
 *      @brief  Basic sensor fusion supervisor functionalities.
 */

#include "ml.h"
#include "mldl.h"
#include "mldl_cfg.h"
#include "mltypes.h"
#include "mlinclude.h"
#include "compass.h"
#include "pressure.h"
#include "dmpKey.h"
#include "dmpDefault.h"
#include "mlstates.h"
#include "mlFIFO.h"
#include "mlFIFOHW.h"
#include "mlMathFunc.h"
#include "mlsupervisor.h"
#include "mlmath.h"

#include "mlsl.h"
#include "mlos.h"

#include <log.h>
#undef MPL_LOG_TAG
#define MPL_LOG_TAG "MPL-sup"

static unsigned long lastCompassTime = 0;
static unsigned long polltime = 0;
static unsigned long coiltime = 0;
static int accCount = 0;
static int compassCalStableCount = 0;
static int compassCalCount = 0;
static int coiltimerstart = 0;
static unsigned long disturbtime = 0;
static int disturbtimerstart = 0;

static yas_filter_if_s f;
static yas_filter_handle_t handle;

#define SUPERVISOR_DEBUG 0

struct inv_supervisor_cb_obj ml_supervisor_cb = { 0 };

/**
 *  @brief  This initializes all variables that should be reset on
 */
void inv_init_sensor_fusion_supervisor(void)
{
    lastCompassTime = 0;
    polltime = 0;
    inv_obj.acc_state = SF_STARTUP_SETTLE;
    accCount = 0;
    compassCalStableCount = 0;
    compassCalCount = 0;

    yas_filter_init(&f);
    f.init(&handle);

#if defined CONFIG_MPU_SENSORS_MPU6050A2 || \
	defined CONFIG_MPU_SENSORS_MPU6050B1
    if (inv_compass_present()) {
        struct mldl_cfg *mldl_cfg = inv_get_dl_config();
        if (mldl_cfg->pdata->compass.bus == EXT_SLAVE_BUS_SECONDARY) {
            (void)inv_send_external_sensor_data(INV_ELEMENT_1 | INV_ELEMENT_2 | INV_ELEMENT_3, INV_16_BIT);
        }
    }
#endif

    if (ml_supervisor_cb.supervisor_reset_func != NULL) {
        ml_supervisor_cb.supervisor_reset_func();
    }
}

static int MLUpdateCompassCalibration3DOF(int command, long *data,
                                          unsigned long deltaTime)
{
    INVENSENSE_FUNC_START;
    int retValue = INV_SUCCESS;
    static float m[10][10] = { {0} };
    float mInv[10][10] = { {0} };
    float mTmp[10][10] = { {0} };
    static float xTransY[4] = { 0 };
    float magSqr = 0;
    float inpData[3] = { 0 };
    int i, j;
    int a, b;
    float d;

    switch (command) {
    case CAL_ADD_DATA:
        inpData[0] = (float)data[0];
        inpData[1] = (float)data[1];
        inpData[2] = (float)data[2];
        m[0][0] += (-2 * inpData[0]) * (-2 * inpData[0]);
        m[0][1] += (-2 * inpData[0]) * (-2 * inpData[1]);
        m[0][2] += (-2 * inpData[0]) * (-2 * inpData[2]);
        m[0][3] += (-2 * inpData[0]);
        m[1][0] += (-2 * inpData[1]) * (-2 * inpData[0]);
        m[1][1] += (-2 * inpData[1]) * (-2 * inpData[1]);
        m[1][2] += (-2 * inpData[1]) * (-2 * inpData[2]);
        m[1][3] += (-2 * inpData[1]);
        m[2][0] += (-2 * inpData[2]) * (-2 * inpData[0]);
        m[2][1] += (-2 * inpData[2]) * (-2 * inpData[1]);
        m[2][2] += (-2 * inpData[2]) * (-2 * inpData[2]);
        m[2][3] += (-2 * inpData[2]);
        m[3][0] += (-2 * inpData[0]);
        m[3][1] += (-2 * inpData[1]);
        m[3][2] += (-2 * inpData[2]);
        m[3][3] += 1.0f;
        magSqr =
            inpData[0] * inpData[0] + inpData[1] * inpData[1] +
            inpData[2] * inpData[2];
        xTransY[0] += (-2 * inpData[0]) * magSqr;
        xTransY[1] += (-2 * inpData[1]) * magSqr;
        xTransY[2] += (-2 * inpData[2]) * magSqr;
        xTransY[3] += magSqr;
        break;
    case CAL_RUN:
        a = 4;
        b = a;
        for (i = 0; i < b; i++) {
            for (j = 0; j < b; j++) {
                a = b;
                inv_matrix_det_inc(&m[0][0], &mTmp[0][0], &a, i, j);
                mInv[j][i] = SIGNM(i + j) * inv_matrix_det(&mTmp[0][0], &a);
            }
        }
        a = b;
        d = inv_matrix_det(&m[0][0], &a);
        if (d == 0) {
            return INV_ERROR;
        }
        for (i = 0; i < 3; i++) {
            float tmp = 0;
            for (j = 0; j < 4; j++) {
                tmp += mInv[j][i] / d * xTransY[j];
            }
            inv_obj.compass_test_bias[i] =
                -(long)(tmp * inv_obj.compass_sens / 16384.0f);
        }
        break;
    case CAL_RESET:
        for (i = 0; i < 4; i++) {
            for (j = 0; j < 4; j++) {
                m[i][j] = 0;
            }
            xTransY[i] = 0;
        }
    default:
        break;
    }
    return retValue;
}

/**
 * Entry point for Sensor Fusion operations
 * @internal
 * @param magFB magnetormeter FB
 * @param accSF accelerometer SF
 */
static inv_error_t MLSensorFusionSupervisor(double *magFB, long *accSF,
                                            unsigned long deltaTime)
{
    static long prevCompassBias[3] = { 0 };
    static long magMax[3] = {
        -1073741824L,
        -1073741824L,
        -1073741824L
    };
    static long magMin[3] = {
        1073741824L,
        1073741824L,
        1073741824L
    };
    int gyroMag;
    long accMag;
    inv_error_t result;
    int i;

    if (ml_supervisor_cb.progressive_no_motion_supervisor_func != NULL) {
        ml_supervisor_cb.progressive_no_motion_supervisor_func(deltaTime);
    }

    gyroMag = inv_get_gyro_sum_of_sqr() >> GYRO_MAG_SQR_SHIFT;
    accMag = inv_accel_sum_of_sqr();

    // Scaling below assumes certain scaling.
#if ACC_MAG_SQR_SHIFT != 16
#error
#endif

    if (ml_supervisor_cb.sensor_fusion_advanced_func != NULL) {
        result = ml_supervisor_cb.sensor_fusion_advanced_func(magFB, deltaTime);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
    } else if (inv_params_obj.bias_mode & INV_MAG_BIAS_FROM_MOTION) {
        //Most basic compass calibration, used only with lite MPL
        if (inv_obj.resetting_compass == 1) {
            for (i = 0; i < 3; i++) {
                magMax[i] = -1073741824L;
                magMin[i] = 1073741824L;
                prevCompassBias[i] = 0;
            }
            compassCalStableCount = 0;
            compassCalCount = 0;
            inv_obj.resetting_compass = 0;
        }
        if ((gyroMag > 400) || (inv_get_gyro_present() == 0)) {
            if (compassCalStableCount < 1000) {
                for (i = 0; i < 3; i++) {
                    if (inv_obj.compass_sensor_data[i] > magMax[i]) {
                        magMax[i] = inv_obj.compass_sensor_data[i];
                    }
                    if (inv_obj.compass_sensor_data[i] < magMin[i]) {
                        magMin[i] = inv_obj.compass_sensor_data[i];
                    }
                }
                MLUpdateCompassCalibration3DOF(CAL_ADD_DATA,
                                               inv_obj.compass_sensor_data,
                                               deltaTime);
                compassCalStableCount += deltaTime;
                for (i = 0; i < 3; i++) {
                    if (magMax[i] - magMin[i] <
                        (long long)40 * 1073741824 / inv_obj.compass_sens) {
                        compassCalStableCount = 0;
                    }
                }
            } else {
                if (compassCalStableCount >= 1000) {
                    if (MLUpdateCompassCalibration3DOF
                        (CAL_RUN, inv_obj.compass_sensor_data,
                         deltaTime) == INV_SUCCESS) {
                        inv_obj.got_compass_bias = 1;
                        inv_obj.compass_accuracy = 3;
                        for(i=0; i<3; i++) {
                            inv_obj.compass_bias_error[i] = 35;
                        }
                        if (inv_obj.compass_state == SF_UNCALIBRATED)
                            inv_obj.compass_state = SF_STARTUP_SETTLE;
                        inv_set_compass_bias(inv_obj.compass_test_bias);
                    }
                    compassCalCount = 0;
                    compassCalStableCount = 0;
                    for (i = 0; i < 3; i++) {
                        magMax[i] = -1073741824L;
                        magMin[i] = 1073741824L;
                        prevCompassBias[i] = 0;
                    }
                    MLUpdateCompassCalibration3DOF(CAL_RESET,
                                                   inv_obj.compass_sensor_data,
                                                   deltaTime);
                }
            }
        }
        compassCalCount += deltaTime;
        if (compassCalCount > 20000) {
            compassCalCount = 0;
            compassCalStableCount = 0;
            for (i = 0; i < 3; i++) {
                magMax[i] = -1073741824L;
                magMin[i] = 1073741824L;
                prevCompassBias[i] = 0;
            }
            MLUpdateCompassCalibration3DOF(CAL_RESET,
                                           inv_obj.compass_sensor_data,
                                           deltaTime);
        }
    }

    if (inv_obj.acc_state != SF_STARTUP_SETTLE) {
        if (((accMag > 260000L) || (accMag < 6000)) || (gyroMag > 2250000L)) {
            inv_obj.acc_state = SF_DISTURBANCE; //No accels, fast swing
            accCount = 0;
        } else {
            if ((gyroMag < 400) && (accMag < 200000L) && (accMag > 26214)
                && ((inv_obj.acc_state == SF_DISTURBANCE)
                    || (inv_obj.acc_state == SF_SLOW_SETTLE))) {
                accCount += deltaTime;
                if (accCount > 0) {
                    inv_obj.acc_state = SF_FAST_SETTLE;
                    accCount = 0;
                }
            } else {
                if (inv_obj.acc_state == SF_DISTURBANCE) {
                    accCount += deltaTime;
                    if (accCount > 500) {
                        inv_obj.acc_state = SF_SLOW_SETTLE;
                        accCount = 0;
                    }
                } else if (inv_obj.acc_state == SF_SLOW_SETTLE) {
                    accCount += deltaTime;
                    if (accCount > 1000) {
                        inv_obj.acc_state = SF_NORMAL;
                        accCount = 0;
                    }
                } else if (inv_obj.acc_state == SF_FAST_SETTLE) {
                    accCount += deltaTime;
                    if (accCount > 250) {
                        inv_obj.acc_state = SF_NORMAL;
                        accCount = 0;
                    }
                }
            }
        }
    }
    if (inv_obj.acc_state == SF_STARTUP_SETTLE) {
        accCount += deltaTime;
        if (accCount > 50) {
            *accSF = 1073741824;    //Startup settling
            inv_obj.acc_state = SF_NORMAL;
            accCount = 0;
        }
    } else if ((inv_obj.acc_state == SF_NORMAL)
               || (inv_obj.acc_state == SF_SLOW_SETTLE)) {
        *accSF = inv_obj.accel_sens * 64;   //Normal
    } else if ((inv_obj.acc_state == SF_DISTURBANCE)) {
        *accSF = inv_obj.accel_sens * 64;   //Don't use accel (should be 0)
    } else if (inv_obj.acc_state == SF_FAST_SETTLE) {
        *accSF = inv_obj.accel_sens * 512;  //Amplify accel
    }
    if (!inv_get_gyro_present()) {
        *accSF = inv_obj.accel_sens * 128;
    }
    return INV_SUCCESS;
}

/**
 *  @brief  Entry point for software sensor fusion operations.
 *          Manages hardware interaction, calls sensor fusion supervisor for
 *          bias calculation.
 *  @return error code. INV_SUCCESS if no error occurred.
 */
inv_error_t inv_accel_compass_supervisor(void)
{
    inv_error_t result;
    int adjustSensorFusion = 0;
    long accSF = 1073741824;
    static double magFB = 0;
    long magSensorData[3];
    float fcin[3];
    float fcout[3];
    

    if (inv_compass_present()) {    /* check for compass data */
        int i, j;
        long long tmp[3] = { 0 };
        long long tmp64 = 0;
        unsigned long ctime = inv_get_tick_count();
        if (((inv_get_compass_id() == COMPASS_ID_AK8975) &&
             ((ctime - polltime) > 20)) ||
            (polltime == 0 || ((ctime - polltime) > 20))) { // 50Hz max
            if (SUPERVISOR_DEBUG) {
                MPL_LOGV("Fetch compass data from inv_process_fifo_packet\n");
                MPL_LOGV("delta time = %ld\n", ctime - polltime);
            }
            polltime = ctime;
            result = inv_get_compass_data(magSensorData);
            /* external slave wants the data even if there is an error */
            if (result && !inv_obj.external_slave_callback) {
                if (SUPERVISOR_DEBUG) {
                    MPL_LOGW("inv_get_compass_data returned %d\n", result);
                }
            } else {
                unsigned long compassTime = inv_get_tick_count();
                unsigned long deltaTime = 1;

                if (result == INV_SUCCESS) {
                    if (lastCompassTime != 0) {
                        deltaTime = compassTime - lastCompassTime;
                    }
                    lastCompassTime = compassTime;
                    adjustSensorFusion = 1;
                    if (inv_obj.got_init_compass_bias == 0) {
                        inv_obj.got_init_compass_bias = 1;
                        for (i = 0; i < 3; i++) {
                            inv_obj.init_compass_bias[i] = magSensorData[i];
                        }
                    }
                    for (i = 0; i < 3; i++) {
                        inv_obj.compass_sensor_data[i] = (long)magSensorData[i];
                        inv_obj.compass_sensor_data[i] -=
                            inv_obj.init_compass_bias[i];
                        tmp[i] = ((long long)inv_obj.compass_sensor_data[i])
                            * inv_obj.compass_sens / 16384;
                        tmp[i] -= inv_obj.compass_bias[i];
                        tmp[i] = (tmp[i] * inv_obj.compass_scale[i]) / 65536L;
                    }
                    for (i = 0; i < 3; i++) {
                        tmp64 = 0;
                        for (j = 0; j < 3; j++) {
                            tmp64 += (long long) tmp[j] *
                                inv_obj.compass_cal[i * 3 + j];
                        }
                        inv_obj.compass_calibrated_data[i] =
                            (long) (tmp64 / inv_obj.compass_sens);
                    }
                    //Additions:
                    if ((inv_obj.compass_state == 1) &&
                            (inv_obj.compass_overunder == 0)) {
                        if (disturbtimerstart == 0) {
                            disturbtimerstart = 1;
                            disturbtime = ctime;
                        }
                    } else {
                        disturbtimerstart = 0;
                    }

                    if (inv_obj.compass_overunder) {
                        if (coiltimerstart == 0) {
                            coiltimerstart = 1;
                            coiltime = ctime;
                        }
                    }
                    if (coiltimerstart == 1) {
                        if (ctime - coiltime > 3000) {
                            inv_obj.flags[INV_COMPASS_OFFSET_VALID] = 0;
                            inv_set_compass_offset();
                            inv_reset_compass_calibration();
                            coiltimerstart = 0;
                        }
                    }

                    if (disturbtimerstart == 1) {
                        if (ctime - disturbtime > 10000) {
                            inv_obj.flags[INV_COMPASS_OFFSET_VALID] = 0;
                            inv_set_compass_offset();
                            inv_reset_compass_calibration();
                            MPL_LOGI("Compass reset! inv_reset_compass_calibration \n");
                            disturbtimerstart = 0;
                        }
                    }
                }
                if (inv_obj.external_slave_callback) {
                    result = inv_obj.external_slave_callback(&inv_obj);
                    if (result) {
                        LOG_RESULT_LOCATION(result);
                        return result;
                    }
                }

#ifdef APPLY_COMPASS_FILTER
                if (inv_get_compass_id() == COMPASS_ID_YAS530)
                {
                    fcin[0] = 1000*((float)inv_obj.compass_calibrated_data[0] /65536.f);
                    fcin[1] = 1000*((float)inv_obj.compass_calibrated_data[1] /65536.f);
                    fcin[2] = 1000*((float)inv_obj.compass_calibrated_data[2] /65536.f);

                    f.update(&handle, fcin, fcout);

                    inv_obj.compass_calibrated_data[0] = (long)(fcout[0]*65536.f/1000.f);
                    inv_obj.compass_calibrated_data[1] = (long)(fcout[1]*65536.f/1000.f);
                    inv_obj.compass_calibrated_data[2] = (long)(fcout[2]*65536.f/1000.f);
                }
#endif

                if (SUPERVISOR_DEBUG) {
                    MPL_LOGI("RM : %+10.6f %+10.6f %+10.6f\n",
                             (float)inv_obj.compass_calibrated_data[0] /
                             65536.f,
                             (float)inv_obj.compass_calibrated_data[1] /
                             65536.f,
                             (float)inv_obj.compass_calibrated_data[2] /
                             65536.f);
                }
                magFB = 1.0;
                adjustSensorFusion = 1;
                result = MLSensorFusionSupervisor(&magFB, &accSF, deltaTime);
                if (result) {
                    LOG_RESULT_LOCATION(result);
                    return result;
                }
            }
        }
    } else {
        //No compass, but still modify accel
        unsigned long ctime = inv_get_tick_count();
        if (polltime == 0 || ((ctime - polltime) > 80)) {   // at the beginning AND every 1/8 second
            unsigned long deltaTime = 1;
            adjustSensorFusion = 1;
            magFB = 0;
            if (polltime != 0) {
                deltaTime = ctime - polltime;
            }
            MLSensorFusionSupervisor(&magFB, &accSF, deltaTime);
            polltime = ctime;
        }
    }
    if (adjustSensorFusion == 1) {
        unsigned char regs[4];
        static int prevAccSF = 1;

        if (accSF != prevAccSF) {
            regs[0] = (unsigned char)((accSF >> 24) & 0xff);
            regs[1] = (unsigned char)((accSF >> 16) & 0xff);
            regs[2] = (unsigned char)((accSF >> 8) & 0xff);
            regs[3] = (unsigned char)(accSF & 0xff);
            result = inv_set_mpu_memory(KEY_D_0_96, 4, regs);
            if (result) {
                LOG_RESULT_LOCATION(result);
                return result;
            }
            prevAccSF = accSF;
        }
    }

    if (ml_supervisor_cb.accel_compass_fusion_func != NULL)
        ml_supervisor_cb.accel_compass_fusion_func(magFB);

    return INV_SUCCESS;
}

/**
 *  @brief  Entry point for software sensor fusion operations.
 *          Manages hardware interaction, calls sensor fusion supervisor for
 *          bias calculation.
 *  @return INV_SUCCESS or non-zero error code on error.
 */
inv_error_t inv_pressure_supervisor(void)
{
    long pressureSensorData[1];
    static unsigned long pressurePolltime = 0;
    if (inv_pressure_present()) {   /* check for pressure data */
        unsigned long ctime = inv_get_tick_count();
        if ((pressurePolltime == 0 || ((ctime - pressurePolltime) > 80))) { //every 1/8 second
            if (SUPERVISOR_DEBUG) {
                MPL_LOGV("Fetch pressure data\n");
                MPL_LOGV("delta time = %ld\n", ctime - pressurePolltime);
            }
            pressurePolltime = ctime;
            if (inv_get_pressure_data(&pressureSensorData[0]) == INV_SUCCESS) {
                inv_obj.pressure = pressureSensorData[0];
            }
        }
    }
    return INV_SUCCESS;
}

/**
 *  @brief  Resets the magnetometer calibration algorithm.
 *  @return INV_SUCCESS if successful, or non-zero error code otherwise.
 */
inv_error_t inv_reset_compass_calibration(void)
{
    if (inv_params_obj.bias_mode & INV_MAG_BIAS_FROM_GYRO) {
        if (ml_supervisor_cb.reset_advanced_compass_func != NULL)
            ml_supervisor_cb.reset_advanced_compass_func();
    }
    MLUpdateCompassCalibration3DOF(CAL_RESET, inv_obj.compass_sensor_data, 1);

    inv_obj.compass_bias_error[0] = P_INIT;
    inv_obj.compass_bias_error[1] = P_INIT;
    inv_obj.compass_bias_error[2] = P_INIT;
    inv_obj.compass_accuracy = 0;

    inv_obj.got_compass_bias = 0;
    inv_obj.got_init_compass_bias = 0;
    inv_obj.compass_state = SF_UNCALIBRATED;
    inv_obj.resetting_compass = 1;

    return INV_SUCCESS;
}

/**
 *  @}
 */
