/**
 *   @defgroup  HAL_Outputs
 *   @brief     Motion Library - HAL Outputs
 *              Sets up common outputs for HAL
 *
 *   @{
 *       @file  datalogger_outputs.c
 *       @brief Windows 8 HAL outputs.
 */
 
#include <string.h>

#include "datalogger_outputs.h"
#include "ml_math_func.h"
#include "mlmath.h"
#include "start_manager.h"
#include "data_builder.h"
#include "results_holder.h"

/*
    Defines
*/
#define ACCEL_CONVERSION (0.000149637603759766f)

/*
    Types
*/
struct datalogger_output_s {
    int quat_accuracy;
    inv_time_t quat_timestamp;
    long quat[4];
    struct inv_sensor_cal_t sc;
};

/*
    Globals and Statics
*/
static struct datalogger_output_s dl_out;

/*
    Functions
*/

/**
 *  Raw (uncompensated) angular velocity (LSB) in chip frame.
 *  @param[out] values      raw angular velocity in LSB.
 *  @param[out] timestamp   Time when sensor was sampled.
 */
void inv_get_sensor_type_gyro_raw_short(short *values, inv_time_t *timestamp)
{
    struct inv_single_sensor_t *pg = &dl_out.sc.gyro;

    if (values)
        memcpy(values, &pg->raw, sizeof(short) * 3);
    if (timestamp)
        *timestamp = pg->timestamp;
}

/**
 *  Raw (uncompensated) angular velocity (degrees per second) in body frame.
 *  @param[out] values      raw angular velocity in dps.
 *  @param[out] timestamp   Time when sensor was sampled.
 */
void inv_get_sensor_type_gyro_raw_body_float(float *values,
        inv_time_t *timestamp)
{
    struct inv_single_sensor_t *pg = &dl_out.sc.gyro;
    long raw[3];
    long raw_body[3];

    raw[0] = (long) pg->raw[0] * (1L << 16);
    raw[1] = (long) pg->raw[1] * (1L << 16);
    raw[2] = (long) pg->raw[2] * (1L << 16);
    inv_convert_to_body_with_scale(pg->orientation, pg->sensitivity,
                                   raw, raw_body);
    if (values) {
        values[0] = inv_q16_to_float(raw_body[0]);
        values[1] = inv_q16_to_float(raw_body[1]);
        values[2] = inv_q16_to_float(raw_body[2]);
    }
    if (timestamp)
        *timestamp = pg->timestamp;
}

/**
 *  Angular velocity (degrees per second) in body frame.
 *  @param[out] values      Angular velocity in dps.
 *  @param[out] accuracy    0 (uncalibrated) to 3 (most accurate).
 *  @param[out] timestamp   Time when sensor was sampled.
 */
void inv_get_sensor_type_gyro_float(float *values, int8_t *accuracy,
        inv_time_t *timestamp)
{
    long gyro[3];
    inv_get_gyro_set(gyro, accuracy, timestamp);

    values[0] = (float)gyro[0] / 65536.f;
    values[1] = (float)gyro[1] / 65536.f;
    values[2] = (float)gyro[2] / 65536.f;
}

/**
 *  Raw (uncompensated) acceleration (LSB) in chip frame.
 *  @param[out] values      raw acceleration in LSB.
 *  @param[out] timestamp   Time when sensor was sampled.
 */
void inv_get_sensor_type_accel_raw_short(short *values, inv_time_t *timestamp)
{
    struct inv_single_sensor_t *pa = &dl_out.sc.accel;

    if (values)
        memcpy(values, &pa->raw, sizeof(short) * 3);
    if (timestamp)
        *timestamp = pa->timestamp;
}

/**
 *  Acceleration (g's) in body frame.
 *  Microsoft defines gravity as positive acceleration pointing towards the
 *  Earth.
 *  @param[out] values      Acceleration in g's.
 *  @param[out] accuracy    0 (uncalibrated) to 3 (most accurate).
 *  @param[out] timestamp   Time when sensor was sampled.
 */
void inv_get_sensor_type_accel_float(float *values, int8_t *accuracy,
        inv_time_t *timestamp)
{
    long accel[3];
    inv_get_accel_set(accel, accuracy, timestamp);

    values[0] = (float) -accel[0] / 65536.f;
    values[1] = (float) -accel[1] / 65536.f;
    values[2] = (float) -accel[2] / 65536.f;
}

/**
 *  Raw (uncompensated) compass magnetic field  (LSB) in chip frame.
 *  @param[out] values      raw magnetic field in LSB.
 *  @param[out] timestamp   Time when sensor was sampled.
 */
void inv_get_sensor_type_compass_raw_short(short *values, inv_time_t *timestamp)
{
    struct inv_single_sensor_t *pc = &dl_out.sc.compass;

    if (values)
        memcpy(values, &pc->raw, sizeof(short) * 3);
    if (timestamp)
        *timestamp = pc->timestamp;
}

/**
 *  Magnetic heading/field strength in body frame.
 *  TODO: No difference between mag_north and true_north yet.
 *  @param[out] mag_north   Heading relative to magnetic north in degrees.
 *  @param[out] true_north  Heading relative to true north in degrees.
 *  @param[out] values      Field strength in milligauss.
 *  @param[out] accuracy    0 (uncalibrated) to 3 (most accurate).
 *  @param[out] timestamp   Time when sensor was sampled.
 */
void inv_get_sensor_type_compass_float(float *mag_north, float *true_north,
        float *values, int8_t *accuracy, inv_time_t *timestamp)
{
    long compass[3];
    long q00, q12, q22, q03, t1, t2;

    /* 1 uT = 10 milligauss. */
#define COMPASS_CONVERSION  (10 / 65536.f)
    inv_get_compass_set(compass, accuracy, timestamp);
    if (values) {
        values[0] = (float)compass[0]*COMPASS_CONVERSION;
        values[1] = (float)compass[1]*COMPASS_CONVERSION;
        values[2] = (float)compass[2]*COMPASS_CONVERSION;
    }

    /* TODO: Stolen from euler angle computation. Calculate this only once per
     * callback.
     */
    q00 = inv_q29_mult(dl_out.quat[0], dl_out.quat[0]);
    q12 = inv_q29_mult(dl_out.quat[1], dl_out.quat[2]);
    q22 = inv_q29_mult(dl_out.quat[2], dl_out.quat[2]);
    q03 = inv_q29_mult(dl_out.quat[0], dl_out.quat[3]);
    t1 = q12 - q03;
    t2 = q22 + q00 - (1L << 30);
    if (mag_north) {
        *mag_north = atan2f((float) t1, (float) t2) * 180.f / (float) M_PI;
        if (*mag_north < 0)
            *mag_north += 360;
    }
    if (true_north) {
        if (!mag_north) {
            *true_north = atan2f((float) t1, (float) t2) * 180.f / (float) M_PI;
            if (*true_north < 0)
                *true_north += 360;
        } else {
            *true_north = *mag_north;
        }
    }
}

#if 0
// put it back when we handle raw temperature
/**
 *  Raw temperature (LSB).
 *  @param[out] value       raw temperature in LSB (1 element).
 *  @param[out] timestamp   Time when sensor was sampled.
 */
void inv_get_sensor_type_temp_raw_short(short *value, inv_time_t *timestamp)
{
    struct inv_single_sensor_t *pt = &dl_out.sc.temp;
    if (value) {
        /* no raw temperature, temperature is only handled calibrated
        *value = pt->raw[0];
        */
        *value = pt->calibrated[0];
    }
    if (timestamp)
        *timestamp = pt->timestamp;
}
#endif

/**
 *  Temperature (degree C).
 *  @param[out] values      Temperature in degrees C.
 *  @param[out] timestamp   Time when sensor was sampled.
 */
void inv_get_sensor_type_temperature_float(float *value, inv_time_t *timestamp)
{
    struct inv_single_sensor_t *pt = &dl_out.sc.temp;
    long ltemp;
    if (timestamp)
        *timestamp = pt->timestamp;
    if (value) {
        /* no raw temperature, temperature is only handled calibrated
        ltemp = pt->raw[0];
        */
        ltemp = pt->calibrated[0];
        *value = (float) ltemp / (1L << 16);
    }
}

/**
 *  Quaternion in body frame.
 *  @e inv_get_sensor_type_quaternion_float outputs the data in the following
 *  order: X, Y, Z, W.
 *  TODO: Windows expects a discontinuity at 180 degree rotations. Will our
 *  convention be ok?
 *  @param[out] values      Quaternion normalized to one.
 *  @param[out] accuracy    0 (uncalibrated) to 3 (most accurate).
 *  @param[out] timestamp   Time when sensor was sampled.
 */
void inv_get_sensor_type_quat_float(float *values, int *accuracy,
                                    inv_time_t *timestamp)
{
    values[0] = dl_out.quat[0] / 1073741824.f;
    values[1] = dl_out.quat[1] / 1073741824.f;
    values[2] = dl_out.quat[2] / 1073741824.f;
    values[3] = dl_out.quat[3] / 1073741824.f;
    accuracy[0] = dl_out.quat_accuracy;
    timestamp[0] = dl_out.quat_timestamp;
}

/** Gravity vector (gee) in body frame.
* @param[out] values Gravity vector in body frame, length 3, (gee)
* @param[out] accuracy Accuracy of the measurment, 0 is least accurate,
                       while 3 is most accurate.
* @param[out] timestamp The timestamp for this sensor. Derived from the
                        timestamp sent to inv_build_accel().
*/
void inv_get_sensor_type_gravity_float(float *values, int *accuracy,
                                       inv_time_t * timestamp)
{
    struct inv_single_sensor_t *pa = &dl_out.sc.accel;

    if (values) {
        long lgravity[3];
        (void)inv_get_gravity(lgravity);
        values[0] = (float) lgravity[0] / (1L << 16);
        values[1] = (float) lgravity[1] / (1L << 16);
        values[2] = (float) lgravity[2] / (1L << 16);
    }
    if (accuracy)
        *accuracy = pa->accuracy;
    if (timestamp)
        *timestamp = pa->timestamp;
}

/**
* This corresponds to Sensor.TYPE_ROTATION_VECTOR.
* The rotation vector represents the orientation of the device as a combination
* of an angle and an axis, in which the device has rotated through an angle @f$\theta@f$
* around an axis {x, y, z}. <br>
* The three elements of the rotation vector are
* {x*sin(@f$\theta@f$/2), y*sin(@f$\theta@f$/2), z*sin(@f$\theta@f$/2)}, such that the magnitude of the rotation
* vector is equal to sin(@f$\theta@f$/2), and the direction of the rotation vector is
* equal to the direction of the axis of rotation.
*
* The three elements of the rotation vector are equal to the last three components of a unit quaternion
* {x*sin(@f$\theta@f$/2), y*sin(@f$\theta@f$/2), z*sin(@f$\theta@f$/2)>.
*
* Elements of the rotation vector are unitless. The x,y and z axis are defined in the same way as the acceleration sensor.
* The reference coordinate system is defined as a direct orthonormal basis, where:

    -X is defined as the vector product Y.Z (It is tangential to the ground at the device's current location and roughly points East).
    -Y is tangential to the ground at the device's current location and points towards the magnetic North Pole.
    -Z points towards the sky and is perpendicular to the ground.
* @param[out] values
* @param[out] accuracy Accuracy 0 to 3, 3 = most accurate
* @param[out] timestamp Timestamp. In (ns) for Android.
*/
void inv_get_sensor_type_rotation_vector_float(float *values, int *accuracy,
        inv_time_t * timestamp)
{
    if (accuracy)
        *accuracy = dl_out.quat_accuracy;
    if (timestamp)
        *timestamp = dl_out.quat_timestamp;
    if (values) {
        if (dl_out.quat[0] >= 0) {
            values[0] = dl_out.quat[1] * INV_TWO_POWER_NEG_30;
            values[1] = dl_out.quat[2] * INV_TWO_POWER_NEG_30;
            values[2] = dl_out.quat[3] * INV_TWO_POWER_NEG_30;
        } else {
            values[0] = -dl_out.quat[1] * INV_TWO_POWER_NEG_30;
            values[1] = -dl_out.quat[2] * INV_TWO_POWER_NEG_30;
            values[2] = -dl_out.quat[3] * INV_TWO_POWER_NEG_30;
        }
    }
}

/** Main callback to generate HAL outputs. Typically not called by library users. */
inv_error_t inv_generate_datalogger_outputs(struct inv_sensor_cal_t *sensor_cal)
{
    memcpy(&dl_out.sc, sensor_cal, sizeof(struct inv_sensor_cal_t));
    inv_get_quaternion_set(dl_out.quat, &dl_out.quat_accuracy,
                           &dl_out.quat_timestamp);
    return INV_SUCCESS;
}

/** Turns off generation of HAL outputs. */
inv_error_t inv_stop_datalogger_outputs(void)
{
    return inv_unregister_data_cb(inv_generate_datalogger_outputs);
}

/** Turns on generation of HAL outputs. This should be called after inv_stop_dl_outputs()
* to turn generation of HAL outputs back on. It is automatically called by inv_enable_dl_outputs().*/
inv_error_t inv_start_datalogger_outputs(void)
{
    return inv_register_data_cb(inv_generate_datalogger_outputs,
        INV_PRIORITY_HAL_OUTPUTS, INV_GYRO_NEW | INV_ACCEL_NEW | INV_MAG_NEW);
}

/** Initializes hal outputs class. This is called automatically by the
* enable function. It may be called any time the feature is enabled, but
* is typically not needed to be called by outside callers.
*/
inv_error_t inv_init_datalogger_outputs(void)
{
    memset(&dl_out, 0, sizeof(dl_out));
    return INV_SUCCESS;
}

/** Turns on creation and storage of HAL type results.
*/
inv_error_t inv_enable_datalogger_outputs(void)
{
    inv_error_t result;
    result = inv_init_datalogger_outputs();
    if (result)
        return result;
    return inv_register_mpl_start_notification(inv_start_datalogger_outputs);
}

/** Turns off creation and storage of HAL type results.
*/
inv_error_t inv_disable_datalogger_outputs(void)
{
    inv_stop_datalogger_outputs();
    return inv_unregister_mpl_start_notification(inv_start_datalogger_outputs);
}

/**
 * @}
 */
