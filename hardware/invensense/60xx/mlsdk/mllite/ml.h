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
 * $Id: ml.h 5653 2011-06-16 21:06:55Z nroyer $
 *
 *****************************************************************************/

/**
 *  @defgroup ML
 *  @brief  The Motion Library processes gyroscopes and accelerometers to
 *          provide a physical model of the movement of the sensors.
 *          The results of this processing may be used to control objects
 *          within a user interface environment, detect gestures, track 3D
 *          movement for gaming applications, and analyze the blur created
 *          due to hand movement while taking a picture.
 *
 *  @{
 *      @file ml.h
 *      @brief Header file for the Motion Library.
**/

#ifndef INV_H
#define INV_H

#ifdef __cplusplus
extern "C" {
#endif

#include "mltypes.h"
#include "mldmp.h"
#include "mlsl.h"
#include "mpu.h"
#ifdef INV_INCLUDE_LEGACY_HEADERS
#include "ml_legacy.h"
#endif

/* ------------ */
/* - Defines. - */
/* ------------ */

/* - Module defines. - */

/*************************************************************************/
/*  Motion Library Vesion                                                */
/*************************************************************************/

#define INV_VERSION_MAJOR                 4
#define INV_VERSION_MINOR                 0
#define INV_VERSION_SUB_MINOR             0

#define INV_VERSION_MAJOR_STR            "4"
#define INV_VERSION_MINOR_STR            "0"
#define INV_VERSION_SUB_MINOR_STR        "0"

#define INV_VERSION_NONE                 ""
#define INV_VERSION_PROTOTYPE            "ProtoA "
#define INV_VERSION_ENGINEERING          "EngA "
#define INV_VERSION_PRE_ALPHA            "Pre-Alpha "
#define INV_VERSION_ALPHA                "Alpha "
#define INV_VERSION_BETA                 "Beta "
#define INV_VERSION_PRODUCTION           "Production "

#ifndef INV_VERSION_TYPE
#define INV_VERSION_TYPE INV_VERSION_NONE
#endif

#define INV_VERSION  "InvenSense MPL" " " \
    "v" INV_VERSION_MAJOR_STR "." INV_VERSION_MINOR_STR "." INV_VERSION_SUB_MINOR_STR " " \
    INV_VERSION_TYPE \
    __DATE__ " " __TIME__

/*************************************************************************/
/*  Motion processing engines                                            */
/*************************************************************************/
#define INV_MOTION_DETECT                (0x0004)
#define INV_BIAS_UPDATE                  (0x0008)
#define INV_GESTURE                      (0x0020)
#define INV_CONTROL                      (0x0040)
#define INV_ORIENTATION                  (0x0100)
#define INV_PEDOMETER                    (0x0200)
#define INV_BASIC                        (INV_MOTION_DETECT | INV_BIAS_UPDATE)

/*************************************************************************/
/*  Data Source - Obsolete                                               */
/*************************************************************************/
#define INV_DATA_FIFO                    (0x1)
#define INV_DATA_POLL                    (0x2)

/*************************************************************************/
/*  Interrupt Source                                                     */
/*************************************************************************/
#define INV_INT_MOTION                   (0x01)
#define INV_INT_FIFO                     (0x02)
#define INV_INT_TAP                      (0x04)
#define INV_INT_ORIENTATION              (0x08)
#define INV_INT_SHAKE_PITCH              (0x10)
#define INV_INT_SHAKE_ROLL               (0x20)
#define INV_INT_SHAKE_YAW                (0x40)

/*************************************************************************/
/*  Bias update functions                                                */
/*************************************************************************/
#define INV_BIAS_FROM_NO_MOTION          0x0001
#define INV_BIAS_FROM_GRAVITY            0x0002
#define INV_BIAS_FROM_TEMPERATURE        0x0004
#define INV_BIAS_FROM_LPF                0x0008
#define INV_MAG_BIAS_FROM_MOTION         0x0010
#define INV_MAG_BIAS_FROM_GYRO           0x0020
#define INV_LEARN_BIAS_FROM_TEMPERATURE  0x0040
#define INV_AUTO_RESET_MAG_BIAS          0x0080
#define INV_REJECT_MAG_DISTURBANCE       0x0100
#define INV_PROGRESSIVE_NO_MOTION        0x0200
#define INV_BIAS_FROM_FAST_NO_MOTION     0x0400

/*************************************************************************/
/*  Euler angles and axis names                                          */
/*************************************************************************/
#define INV_X_AXIS                       (0x01)
#define INV_Y_AXIS                       (0x02)
#define INV_Z_AXIS                       (0x04)

#define INV_PITCH                        (INV_X_AXIS)
#define INV_ROLL                         (INV_Y_AXIS)
#define INV_YAW                          (INV_Z_AXIS)

/*************************************************************************/
/*  Sensor types                                                         */
/*************************************************************************/
#define INV_GYROS                        0x0001
#define INV_ACCELS                       0x0002

/*************************************************************************/
/*  Motion arrays                                                        */
/*************************************************************************/
#define INV_ROTATION_MATRIX              0x0003
#define INV_QUATERNION                   0x0004
#define INV_EULER_ANGLES                 0x0005
#define INV_LINEAR_ACCELERATION          0x0006
#define INV_LINEAR_ACCELERATION_WORLD    0x0007
#define INV_GRAVITY                      0x0008
#define INV_ANGULAR_VELOCITY             0x0009

#define INV_GYRO_CALIBRATION_MATRIX      0x000B
#define INV_ACCEL_CALIBRATION_MATRIX     0x000C
#define INV_GYRO_BIAS                    0x000D
#define INV_ACCEL_BIAS                   0x000E
#define INV_GYRO_TEMP_SLOPE              0x000F

#define INV_RAW_DATA                     0x0011
#define INV_DMP_TAP                      0x0012
#define INV_DMP_TAP2                     0x0021

#define INV_EULER_ANGLES_X               0x0013
#define INV_EULER_ANGLES_Y               0x0014
#define INV_EULER_ANGLES_Z               0x0015

#define INV_BIAS_UNCERTAINTY             0x0016
#define INV_DMP_PACKET_NUMBER            0x0017
#define INV_FOOTER                       0x0018

#define INV_CONTROL_DATA                 0x0019

#define INV_MAGNETOMETER                 0x001A
#define INV_PEDLBS                       0x001B
#define INV_MAG_RAW_DATA                 0x001C
#define INV_MAG_CALIBRATION_MATRIX       0x001D
#define INV_MAG_BIAS                     0x001E
#define INV_HEADING                      0x001F

#define INV_MAG_BIAS_ERROR               0x0020

#define INV_PRESSURE                     0x0021
#define INV_LOCAL_FIELD                  0x0022
#define INV_MAG_SCALE                    0x0023

#define INV_RELATIVE_QUATERNION          0x0024

#define SET_QUATERNION                                  0x0001
#define SET_GYROS                                       0x0002
#define SET_LINEAR_ACCELERATION                         0x0004
#define SET_GRAVITY                                     0x0008
#define SET_ACCELS                                      0x0010
#define SET_TAP                                         0x0020
#define SET_PEDLBS                                      0x0040
#define SET_LINEAR_ACCELERATION_WORLD                   0x0080
#define SET_CONTROL                                     0x0100
#define SET_PACKET_NUMBER                               0x4000
#define SET_FOOTER                                      0x8000

/*************************************************************************/
/*  Integral reset options                                               */
/*************************************************************************/
#define INV_NO_RESET                     0x0000
#define INV_RESET                        0x0001

/*************************************************************************/
/*  Motion states                                                        */
/*************************************************************************/
#define INV_MOTION                       0x0001
#define INV_NO_MOTION                    0x0002

/*************************************************************************/
/* Orientation and Gesture states                                        */
/*************************************************************************/
#define INV_STATE_IDLE                   (0)
#define INV_STATE_RUNNING                (1)

/*************************************************************************/
/* Gyroscope Temperature Compensation bins                               */
/*************************************************************************/
#define BINS                            (25)
#define PTS_PER_BIN                     (5)
#define MIN_TEMP                        (-40)
#define MAX_TEMP                        (+85)
#define TEMP_PER_BIN                    ((MAX_TEMP - MIN_TEMP) / BINS)

/*************************************************************************/
/*  Flags                                                                */
/*************************************************************************/
#define INV_RAW_DATA_READY               0x0001
#define INV_PROCESSED_DATA_READY         0x0002

#define INV_GOT_GESTURE                  0x0004

#define INV_MOTION_STATE_CHANGE          0x0006
#define INV_COMPASS_OFFSET_VALID         0x0007

/*************************************************************************/
/*  General                                                              */
/*************************************************************************/
#define INV_NONE                         (0x0000)
#define INV_INVALID_FIFO_RATE            (0xFFFF)

/*************************************************************************/
/*  ML Params Structure Default Values                                   */
/*************************************************************************/
#define INV_BIAS_UPDATE_FUNC_DEFAULT               (INV_BIAS_FROM_NO_MOTION | INV_BIAS_FROM_GRAVITY)
#define INV_ORIENTATION_MASK_DEFAULT               0x3f
#define INV_PROCESSED_DATA_CALLBACK_DEFAULT           0
#define INV_ORIENTATION_CALLBACK_DEFAULT              0
#define INV_MOTION_CALLBACK_DEFAULT                   0

/* ------------ */
/* - Defines. - */
/* ------------ */
/* Priority for various features */
#define INV_PRIORITY_BUS_ACCEL              100
#define INV_PRIORITY_EXTERNAL_SLAVE_MAG     110
#define INV_PRIORITY_FAST_NO_MOTION         120
#define INV_PRIORITY_BIAS_NO_MOTION         125
#define INV_PRIORITY_SET_GYRO_BIASES        150
#define INV_PRIORITY_TEMP_COMP              175
#define INV_PRIORITY_CONTROL                200
#define INV_PRIORITY_EIS                    300
#define INV_PRIORITY_ORIENTATION            400
#define INV_PRIORITY_PEDOMETER_FULLPOWER    500
#define INV_PRIORITY_NAVIGATION_PEDOMETER   600
#define INV_PRIORITY_GESTURE                700
#define INV_PRIORITY_GLYPH                  800

#define MAX_INTERRUPT_PROCESSES 5
/* Number of quantized accel samples */
#define INV_MAX_NUM_ACCEL_SAMPLES (8)

#define PRECISION 10000.f
#define RANGE_FLOAT_TO_FIXEDPOINT(range, x) {               \
    range.mantissa = (long)x;                               \
    range.fraction = (long)((float)(x-(long)x)*PRECISION);  \
}
#define RANGE_FIXEDPOINT_TO_FLOAT(range, x) {   \
    x = (float)(range.mantissa);                \
    x += ((float)range.fraction/PRECISION);     \
}

    /* --------------- */
    /* - Structures. - */
    /* --------------- */

struct inv_obj_t {
        //Calibration parameters
        /* Raw sensor orientation */
        long gyro_bias[3];
        long accel_bias[3];
        long compass_bias[3];

        /* Cached values after orietnation is applied */
        long scaled_gyro_bias[3];
        long scaled_accel_bias[3];
        long scaled_compass_bias[3];

        long compass_scale[3];
        long compass_test_bias[3];
        long compass_test_scale[3];
        long compass_asa[3];
        long compass_offsets[3];

        long compass_bias_error[3];

        long got_no_motion_bias;
        long got_compass_bias;
        long compass_state;
        long large_field;
        long acc_state;

        long factory_temp_comp;
        long got_coarse_heading;
        long gyro_temp_bias[3];
        long prog_no_motion_bias[3];

        long accel_cal[9];
        // Deprecated, used gyro_orient
        long gyro_cal[GYRO_NUM_AXES * GYRO_NUM_AXES];
        long gyro_orient[GYRO_NUM_AXES * GYRO_NUM_AXES];
        long accel_sens;
        long compass_cal[9];
        long gyro_sens;
        long gyro_sf;
        long temp_slope[GYRO_NUM_AXES];
        long compass_sens;
        long temp_offset[GYRO_NUM_AXES];

        int cal_loaded_flag;

        /* temperature compensation */
        float x_gyro_coef[3];
        float y_gyro_coef[3];
        float z_gyro_coef[3];
        float x_gyro_temp_data[BINS][PTS_PER_BIN];
        float y_gyro_temp_data[BINS][PTS_PER_BIN];
        float z_gyro_temp_data[BINS][PTS_PER_BIN];
        float temp_data[BINS][PTS_PER_BIN];
        int temp_ptrs[BINS];
        long temp_valid_data[BINS];

        long compass_correction[4];
        long compass_correction_relative[4];
        long compass_disturb_correction[4];
        long compass_correction_offset[4];
        long relative_quat[4];
        long local_field[3];
        long new_local_field;
        long sync_grav_body[3];
        int gyro_bias_err;

        double compass_bias_ptr[9];
        double compass_bias_v[3];
        double compass_prev_m[36];
        double compass_prev_xty[6];

        int compass_peaks[18];
        int all_sensors_no_motion;

        long init_compass_bias[3];

        int got_init_compass_bias;
        int resetting_compass;

        long ang_v_body[GYRO_NUM_AXES];
        unsigned char compass_raw_data[24]; /* Sensor data plus status etc */
        long compass_sensor_data[3]; /* Raw sensor data only */
        long compass_calibrated_data[3];
        long compass_test_calibrated_data[3];
        long pressure;
        inv_error_t (*external_slave_callback)(struct inv_obj_t *);
        int  compass_accuracy;
        int compass_overunder;

        unsigned short flags[8];
        unsigned short suspend;

        long no_motion_threshold;
        unsigned long motion_duration;

        unsigned short motion_state;

        unsigned short data_mode;
        unsigned short interrupt_sources;

        unsigned short bias_update_time;
        short last_motion;
        unsigned short bias_calc_time;

        unsigned char internal_motion_state;
        long start_time;

        long accel_lpf_gain;
        long accel_lpf[3];
        unsigned long poll_no_motion;
        long no_motion_accel_threshold;
        unsigned long no_motion_accel_time;
        inv_error_t(*mode_change_func) (unsigned long, unsigned long);
    };

    typedef inv_error_t(*inv_obj_func) (struct inv_obj_t *);

    extern struct inv_obj_t inv_obj;

    /* --------------------- */
    /* - Params Structure. - */
    /* --------------------- */

    struct inv_params_obj {

        unsigned short bias_mode;   // A function or bitwise OR of functions that determine how the gyroscope bias will be automatically updated.
        // Functions include INV_BIAS_FROM_NO_MOTION, INV_BIAS_FROM_GRAVITY, and INV_BIAS_FROM_TEMPERATURE.
        // The engine INV_BIAS_UPDATE must be enabled for these algorithms to run.

        unsigned short orientation_mask;    // Allows a user to register which orientations will trigger the user defined callback function.
        // The orientations are INV_X_UP, INV_X_DOWN, INV_Y_UP, INV_Y_DOWN, INV_Z_UP, and INV_Z_DOWN.
        // INV_ORIENTATION_ALL is equivalent to INV_X_UP | INV_X_DOWN | INV_Y_UP | INV_Y_DOWN | INV_Z_UP | INV_Z_DOWN.

        void (*fifo_processed_func) (void); // Callback function that triggers when all the processing has been finished by the motion processing engines.

        void (*orientation_cb_func) (unsigned short orient);    // Callback function that will run when a change of orientation is detected.
        // The new orientation. May be one of INV_X_UP, INV_X_DOWN, INV_Y_UP, INV_Y_DOWN, INV_Z_UP, or INV_Z_DOWN.

        void (*motion_cb_func) (unsigned short motion_state);   // Callback function that will run when a change of motion state is detected.
        // The new motion state. May be one of INV_MOTION, or INV_NO_MOTION.

        unsigned char state;

    };

    extern struct inv_params_obj inv_params_obj;
    /* --------------------- */
    /* - Function p-types. - */
    /* --------------------- */

    inv_error_t inv_serial_start(char const *port);
    inv_error_t inv_serial_stop(void);
    inv_error_t inv_set_mpu_sensors(unsigned long sensors);
    void *inv_get_serial_handle(void);

    /*API for handling the buffer */
    inv_error_t inv_update_data(void);

    /*API for handling polling */
    int inv_check_flag(int flag);

    /*API for setting bias update function */
    inv_error_t inv_set_bias_update(unsigned short biasFunction);

#if defined CONFIG_MPU_SENSORS_MPU6050A2 || \
    defined CONFIG_MPU_SENSORS_MPU6050B1
    inv_error_t inv_turn_on_bias_from_no_motion(void);
    inv_error_t inv_turn_off_bias_from_no_motion(void);
    inv_error_t inv_set_mpu_6050_config(void);
#endif

    /* Legacy functions for handling augmented data*/
    inv_error_t inv_get_array(int dataSet, long *data);
    inv_error_t inv_get_float_array(int dataSet, float *data);
    inv_error_t inv_set_array(int dataSet, long *data);
    inv_error_t inv_set_float_array(int dataSet, float *data);
    /* Individual functions for augmented data, per specific dataset */


    inv_error_t inv_get_gyro(long *data);
    inv_error_t inv_get_accel(long *data);
    inv_error_t inv_get_temperature(long *data);
    inv_error_t inv_get_temperature_raw(short *data);
    inv_error_t inv_get_rot_mat(long *data);
    inv_error_t inv_get_quaternion(long *data);
    inv_error_t inv_get_linear_accel(long *data);
    inv_error_t inv_get_linear_accel_in_world(long *data);
    inv_error_t inv_get_gravity(long *data);
    inv_error_t inv_get_angular_velocity(long *data);
    inv_error_t inv_get_euler_angles(long *data);
    inv_error_t inv_get_euler_angles_x(long *data);
    inv_error_t inv_get_euler_angles_y(long *data);
    inv_error_t inv_get_euler_angles_z(long *data);
    inv_error_t inv_get_gyro_temp_slope(long *data);
    inv_error_t inv_get_gyro_bias(long *data);
    inv_error_t inv_get_accel_bias(long *data);
    inv_error_t inv_get_mag_bias(long *data);
    inv_error_t inv_get_gyro_and_accel_sensor(long *data);
    inv_error_t inv_get_mag_raw_data(long *data);
    inv_error_t inv_get_magnetometer(long *data);
    inv_error_t inv_get_pressure(long *data);
    inv_error_t inv_get_heading(long *data);
    inv_error_t inv_get_gyro_cal_matrix(long *data);
    inv_error_t inv_get_accel_cal_matrix(long *data);
    inv_error_t inv_get_mag_cal_matrix(long *data);
    inv_error_t inv_get_mag_bias_error(long *data);
    inv_error_t inv_get_mag_scale(long *data);
    inv_error_t inv_get_local_field(long *data);
    inv_error_t inv_get_relative_quaternion(long *data);
    inv_error_t inv_get_gyro_float(float *data);
    inv_error_t inv_get_accel_float(float *data);
    inv_error_t inv_get_temperature_float(float *data);
    inv_error_t inv_get_rot_mat_float(float *data);
    inv_error_t inv_get_quaternion_float(float *data);
    inv_error_t inv_get_linear_accel_float(float *data);
    inv_error_t inv_get_linear_accel_in_world_float(float *data);
    inv_error_t inv_get_gravity_float(float *data);
    inv_error_t inv_get_angular_velocity_float(float *data);
    inv_error_t inv_get_euler_angles_float(float *data);
    inv_error_t inv_get_euler_angles_x_float(float *data);
    inv_error_t inv_get_euler_angles_y_float(float *data);
    inv_error_t inv_get_euler_angles_z_float(float *data);
    inv_error_t inv_get_gyro_temp_slope_float(float *data);
    inv_error_t inv_get_gyro_bias_float(float *data);
    inv_error_t inv_get_accel_bias_float(float *data);
    inv_error_t inv_get_mag_bias_float(float *data);
    inv_error_t inv_get_gyro_and_accel_sensor_float(float *data);
    inv_error_t inv_get_mag_raw_data_float(float *data);
    inv_error_t inv_get_magnetometer_float(float *data);
    inv_error_t inv_get_pressure_float(float *data);
    inv_error_t inv_get_heading_float(float *data);
    inv_error_t inv_get_gyro_cal_matrix_float(float *data);
    inv_error_t inv_get_accel_cal_matrix_float(float *data);
    inv_error_t inv_get_mag_cal_matrix_float(float *data);
    inv_error_t inv_get_mag_bias_error_float(float *data);
    inv_error_t inv_get_mag_scale_float(float *data);
    inv_error_t inv_get_local_field_float(float *data);
    inv_error_t inv_get_relative_quaternion_float(float *data);
    inv_error_t inv_get_compass_accuracy(int *accuracy);
    inv_error_t inv_set_gyro_bias(long *data);
    inv_error_t inv_set_accel_bias(long *data);
    inv_error_t inv_set_mag_bias(long *data);
    inv_error_t inv_set_gyro_temp_slope(long *data);
    inv_error_t inv_set_local_field(long *data);
    inv_error_t inv_set_mag_scale(long *data);
    inv_error_t inv_set_gyro_temp_slope_float(float *data);
    inv_error_t inv_set_gyro_bias_float(float *data);
    inv_error_t inv_set_accel_bias_float(float *data);
    inv_error_t inv_set_mag_bias_float(float *data);
    inv_error_t inv_set_local_field_float(float *data);
    inv_error_t inv_set_mag_scale_float(float *data);

    inv_error_t inv_apply_endian_accel(void);
    inv_error_t inv_apply_calibration(void);
    inv_error_t inv_set_gyro_calibration(float range, signed char *orientation);
    inv_error_t inv_set_accel_calibration(float range,
                                          signed char *orientation);
    inv_error_t inv_set_compass_calibration(float range,
                                            signed char *orientation);

    /*API for detecting change of state */
     inv_error_t
        inv_set_motion_callback(void (*func) (unsigned short motion_state));
    int inv_get_motion_state(void);

    /*API for getting ML version. */
    inv_error_t inv_get_version(unsigned char **version);

    inv_error_t inv_set_motion_interrupt(unsigned char on);
    inv_error_t inv_set_fifo_interrupt(unsigned char on);

    int inv_get_interrupts(void);

    /* Simulated DMP */
    int inv_get_gyro_present(void);

    inv_error_t inv_set_no_motion_time(float time);
    inv_error_t inv_set_no_motion_thresh(float thresh);
    inv_error_t inv_set_no_motion_threshAccel(long thresh);
    inv_error_t inv_reset_motion(void);

    inv_error_t inv_update_bias(void);
    inv_error_t inv_set_dead_zone(void);
    void inv_start_bias_calc(void);
    void inv_stop_bias_calc(void);

    // Private functions shared accross modules
    void inv_init_ml(void);

    inv_error_t inv_register_dmp_interupt_cb(inv_obj_func func);
    inv_error_t inv_unregister_dmp_interupt_cb(inv_obj_func func);
    void inv_run_dmp_interupt_cb(void);
    void inv_set_mode_change(inv_error_t(*mode_change_func)
                              (unsigned long, unsigned long));

#ifdef __cplusplus
}
#endif
#endif                          // INV_H
/**
 * @}
 */
