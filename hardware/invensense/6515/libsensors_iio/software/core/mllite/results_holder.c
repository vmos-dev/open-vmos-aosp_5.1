/*
 $License:
    Copyright (C) 2011-2012 InvenSense Corporation, All Rights Reserved.
    See included License.txt for License information.
 $
 */
/**
 *   @defgroup  Results_Holder results_holder
 *   @brief     Motion Library - Results Holder
 *              Holds the data for MPL
 *
 *   @{
 *       @file results_holder.c
 *       @brief Results Holder for HAL.
 */

#include <string.h>

#include "results_holder.h"
#include "ml_math_func.h"
#include "mlmath.h"
#include "start_manager.h"
#include "data_builder.h"
#include "message_layer.h"
#include "log.h"

struct results_t {
    float nav_quat[4];
    float game_quat[4];
    long gam_quat[4];
    float geomag_quat[4];
    long accel_quat[4];
    inv_time_t nav_timestamp;
    inv_time_t gam_timestamp;
    inv_time_t geomag_timestamp;
    long mag_scale[3]; /**< scale factor to apply to magnetic field reading */
    long compass_correction[4]; /**< quaternion going from gyro,accel quaternion to 9 axis */
    long geomag_compass_correction[4]; /**< quaternion going from accel quaternion to geomag sensor fusion */
    int acc_state; /**< Describes accel state */
    int got_accel_bias; /**< Flag describing if accel bias is known */
    long compass_bias_error[3]; /**< Error Squared */
    unsigned char motion_state;
    unsigned int motion_state_counter; /**< Incremented for each no motion event in a row */
    long compass_count; /**< compass state internal counter */
    int got_compass_bias; /**< Flag describing if compass bias is known */
    int large_mag_field; /**< Flag describing if there is a large magnetic field */
    int compass_state; /**< Internal compass state */
    long status;
    struct inv_sensor_cal_t *sensor;
    float quat_confidence_interval;
    float geo_mag_confidence_interval;
    struct local_field_t mag_local_field;
    struct local_field_t mpl_compass_cal;
    int quat_validity;
#ifdef WIN32
    long last_quat[4];
#endif
};
static struct results_t rh;

/** @internal
* Store a quaternion more suitable for gaming. This quaternion is often determined
* using only gyro and accel.
* @param[in] quat Length 4, Quaternion scaled by 2^30
* @param[in] timestamp Timestamp of the 6-axis quaternion
*/
void inv_store_gaming_quaternion(const long *quat, inv_time_t timestamp)
{
    rh.status |= INV_6_AXIS_QUAT_SET;
    memcpy(&rh.gam_quat, quat, sizeof(rh.gam_quat));
    rh.gam_timestamp = timestamp;
}

/** @internal
* Store a 9-axis quaternion.
* @param[in] quat Length 4 in floating-point numbers
* @param[in] timestamp Timestamp of the 9-axis quaternion
*/
void inv_store_nav_quaternion(const float *quat, inv_time_t timestamp)
{
    memcpy(&rh.nav_quat, quat, sizeof(rh.nav_quat));
    rh.nav_timestamp = timestamp;
}

/** @internal
* Store a 6-axis geomagnetic quaternion.
* @param[in] quat Length 4 in floating-point numbers
* @param[in] timestamp Timestamp of the geomag quaternion
*/
void inv_store_geomag_quaternion(const float *quat, inv_time_t timestamp)
{
    memcpy(&rh.geomag_quat, quat, sizeof(rh.geomag_quat));
    rh.geomag_timestamp = timestamp;
}

/** @internal
* Store a floating-point quaternion more suitable for gaming.
* @param[in] quat Length 4 in floating-point numbers
* @param[in] timestamp Timestamp of the 6-axis quaternion
*/
void inv_store_game_quaternion(const float *quat, inv_time_t timestamp)
{
    rh.status |= INV_6_AXIS_QUAT_SET;
    memcpy(&rh.game_quat, quat, sizeof(rh.game_quat));
    rh.gam_timestamp = timestamp;
}

/** @internal
* Store a quaternion computed from accelerometer correction. This quaternion is 
* determined * using only accel, and used for geomagnetic fusion. 
* @param[in] quat Length 4, Quaternion scaled by 2^30
*/
void inv_store_accel_quaternion(const long *quat, inv_time_t timestamp)
{
   // rh.status |= INV_6_AXIS_QUAT_SET;
    memcpy(&rh.accel_quat, quat, sizeof(rh.accel_quat));
    rh.geomag_timestamp = timestamp;
}
/** @internal
* Sets the quaternion adjustment from 6 axis (accel, gyro) to 9 axis quaternion.
* @param[in] data Quaternion Adjustment
* @param[in] timestamp Timestamp of when this is valid
*/
void inv_set_compass_correction(const long *data, inv_time_t timestamp)
{
    rh.status |= INV_COMPASS_CORRECTION_SET;
    memcpy(rh.compass_correction, data, sizeof(rh.compass_correction));
    rh.nav_timestamp = timestamp;
}

/** @internal
* Sets the quaternion adjustment from 3 axis (accel) to 6 axis (with compass) quaternion.
* @param[in] data Quaternion Adjustment
* @param[in] timestamp Timestamp of when this is valid
*/
void inv_set_geomagnetic_compass_correction(const long *data, inv_time_t timestamp)
{
    rh.status |= INV_GEOMAGNETIC_CORRECTION_SET;
    memcpy(rh.geomag_compass_correction, data, sizeof(rh.geomag_compass_correction));
    rh.geomag_timestamp = timestamp;
}

/** @internal
* Gets the quaternion adjustment from 6 axis (accel, gyro) to 9 axis quaternion.
* @param[out] data Quaternion Adjustment
* @param[out] timestamp Timestamp of when this is valid
*/
void inv_get_compass_correction(long *data, inv_time_t *timestamp)
{
    memcpy(data, rh.compass_correction, sizeof(rh.compass_correction));
    *timestamp = rh.nav_timestamp;
}

/** @internal
* Gets the quaternion adjustment from 3 axis (accel) to 6 axis (with compass) quaternion.
* @param[out] data Quaternion Adjustment
* @param[out] timestamp Timestamp of when this is valid
*/
void inv_get_geomagnetic_compass_correction(long *data, inv_time_t *timestamp)
{
    memcpy(data, rh.geomag_compass_correction, sizeof(rh.geomag_compass_correction));
    *timestamp = rh.geomag_timestamp;
}

/** Returns non-zero if there is a large magnetic field. See inv_set_large_mag_field() for setting this variable.
 * @return Returns non-zero if there is a large magnetic field.
 */
int inv_get_large_mag_field()
{
    return rh.large_mag_field;
}

/** Set to non-zero if there as a large magnetic field. See inv_get_large_mag_field() for getting this variable.
 * @param[in] state value to set for magnetic field strength. Should be non-zero if it is large.
 */
void inv_set_large_mag_field(int state)
{
    rh.large_mag_field = state;
}

/** Gets the accel state set by inv_set_acc_state()
 * @return accel state.
 */
int inv_get_acc_state()
{
    return rh.acc_state;
}

/** Sets the accel state. See inv_get_acc_state() to get the value.
 * @param[in] state value to set accel state to.
 */
void inv_set_acc_state(int state)
{
    rh.acc_state = state;
    return;
}

/** Returns the motion state
* @param[out] cntr Number of previous times a no motion event has occured in a row.
* @return Returns INV_SUCCESS if successful or an error code if not.
*/
int inv_get_motion_state(unsigned int *cntr)
{
    *cntr = rh.motion_state_counter;
    return rh.motion_state;
}

/** Sets the motion state
 * @param[in] state motion state where INV_NO_MOTION is not moving
 *            and INV_MOTION is moving.
 */
void inv_set_motion_state(unsigned char state)
{
    long set;
    if (state == rh.motion_state) {
        if (state == INV_NO_MOTION) {
            rh.motion_state_counter++;
        } else {
            rh.motion_state_counter = 0;
        }
        return;
    }
    rh.motion_state_counter = 0;
    rh.motion_state = state;
    /* Equivalent to set = state, but #define's may change. */
    if (state == INV_MOTION)
        set = INV_MSG_MOTION_EVENT;
    else
        set = INV_MSG_NO_MOTION_EVENT;
    inv_set_message(set, (INV_MSG_MOTION_EVENT | INV_MSG_NO_MOTION_EVENT), 0);
}

/** Sets the compass sensitivity
 * @param[in] data Length 3, sensitivity for each compass axis
 *  scaled such that 1.0 = 2^30.
 */
void inv_set_mag_scale(const long *data)
{
    memcpy(rh.mag_scale, data, sizeof(rh.mag_scale));
}

/** Gets the compass sensitivity
 * @param[out] data Length 3, sensitivity for each compass axis
 *  scaled such that 1.0 = 2^30.
 */
void inv_get_mag_scale(long *data)
{
    memcpy(data, rh.mag_scale, sizeof(rh.mag_scale));
}

/** Gets gravity vector
 * @param[out] data gravity vector in body frame scaled such that 1.0 = 2^30.
 * @return Returns INV_SUCCESS if successful or an error code if not.
 */
inv_error_t inv_get_gravity(long *data)
{
    long ldata[4];
    inv_error_t result = inv_get_quaternion(ldata);

    data[0] =
        inv_q29_mult(ldata[1], ldata[3]) - inv_q29_mult(ldata[2], ldata[0]);
    data[1] =
        inv_q29_mult(ldata[2], ldata[3]) + inv_q29_mult(ldata[1], ldata[0]);
    data[2] =
        (inv_q29_mult(ldata[3], ldata[3]) + inv_q29_mult(ldata[0], ldata[0])) -
        1073741824L;

    return result;
}

/** Returns a quaternion based only on accel.
 * @param[out] data 3-axis  accel quaternion scaled such that 1.0 = 2^30.
 * @return Returns INV_SUCCESS if successful or an error code if not.
 */
inv_error_t inv_get_accel_quaternion(long *data)
{
    memcpy(data, rh.accel_quat, sizeof(rh.accel_quat));
    return INV_SUCCESS;
}
inv_error_t inv_get_gravity_6x(long *data)
{
    data[0] =
        inv_q29_mult(rh.gam_quat[1], rh.gam_quat[3]) - inv_q29_mult(rh.gam_quat[2], rh.gam_quat[0]);
    data[1] =
        inv_q29_mult(rh.gam_quat[2], rh.gam_quat[3]) + inv_q29_mult(rh.gam_quat[1], rh.gam_quat[0]);
    data[2] =
        (inv_q29_mult(rh.gam_quat[3], rh.gam_quat[3]) + inv_q29_mult(rh.gam_quat[0], rh.gam_quat[0])) -
        1073741824L;
    return INV_SUCCESS;
}
/** Returns a quaternion based only on gyro and accel.
 * @param[out] data 6-axis  gyro and accel quaternion scaled such that 1.0 = 2^30.
 * @return Returns INV_SUCCESS if successful or an error code if not.
 */
inv_error_t inv_get_6axis_quaternion(long *data, inv_time_t *timestamp)
{
    data[0] = (long)MIN(MAX(rh.game_quat[0] * ((float)(1L << 30)), -2147483648.), 2147483647.);
    data[1] = (long)MIN(MAX(rh.game_quat[1] * ((float)(1L << 30)), -2147483648.), 2147483647.);
    data[2] = (long)MIN(MAX(rh.game_quat[2] * ((float)(1L << 30)), -2147483648.), 2147483647.);
    data[3] = (long)MIN(MAX(rh.game_quat[3] * ((float)(1L << 30)), -2147483648.), 2147483647.);
    *timestamp = rh.gam_timestamp;
    return INV_SUCCESS;
}

/** Returns a quaternion.
 * @param[out] data 9-axis quaternion scaled such that 1.0 = 2^30.
 * @return Returns INV_SUCCESS if successful or an error code if not.
 */
inv_error_t inv_get_quaternion(long *data)
{
    data[0] = (long)MIN(MAX(rh.nav_quat[0] * ((float)(1L << 30)), -2147483648.), 2147483647.);
    data[1] = (long)MIN(MAX(rh.nav_quat[1] * ((float)(1L << 30)), -2147483648.), 2147483647.);
    data[2] = (long)MIN(MAX(rh.nav_quat[2] * ((float)(1L << 30)), -2147483648.), 2147483647.);
    data[3] = (long)MIN(MAX(rh.nav_quat[3] * ((float)(1L << 30)), -2147483648.), 2147483647.);

    return INV_SUCCESS;
}

#ifdef WIN32
/** Returns the last 9-axis quaternion genearted by MPL, so that
 * it can be written to the DMP.
 * @param[out] data 9-axis quaternion scaled such that 1.0 = 2^30.
 * @return Returns INV_SUCCESS if successful or an error code if not.
 */
inv_error_t inv_get_last_quaternion(long *data)
{
    memcpy(data, rh.last_quat, sizeof(rh.last_quat));
    return INV_SUCCESS;
}

/** Saves the last 9-axis quaternion generated by MPL.
 * @param[in] data 9-axis quaternion data.
 */
inv_error_t inv_set_last_quaternion(long *data)
{
    memcpy(rh.last_quat, data, sizeof(rh.last_quat));
    return INV_SUCCESS;
}
#endif

/** Returns the status of the result holder.
 * @param[out] rh_status Result holder status.
 * @return Returns INV_SUCCESS if successful or an error code if not.
 */
inv_error_t inv_get_result_holder_status(long *rh_status)
{
    *rh_status = rh.status;
    return INV_SUCCESS;
}

/** Set the status of the result holder.
 * @return Returns INV_SUCCESS if successful or an error code if not.
 */
inv_error_t inv_set_result_holder_status(long rh_status)
{
    rh.status = rh_status;
    return INV_SUCCESS;
}

/** Returns the status of the authenticity of the quaternion data.
 * @param[out] value Authenticity of the quaternion data.
 * @return Returns INV_SUCCESS if successful or an error code if not.
 */
inv_error_t inv_get_quaternion_validity(int *value)
{
    *value = rh.quat_validity;
    return INV_SUCCESS;
}

/** Set the status of the authenticity of the quaternion data.
 * @return Returns INV_SUCCESS if successful or an error code if not.
 */
inv_error_t inv_set_quaternion_validity(int value)
{
    rh.quat_validity = value;
    return INV_SUCCESS;
}

/** Returns a quaternion based only on compass and accel.
 * @param[out] data 6-axis  compass and accel quaternion scaled such that 1.0 = 2^30.
 * @return Returns INV_SUCCESS if successful or an error code if not.
 */
inv_error_t inv_get_geomagnetic_quaternion(long *data, inv_time_t *timestamp)
{
    data[0] = (long)MIN(MAX(rh.geomag_quat[0] * ((float)(1L << 30)), -2147483648.), 2147483647.);
    data[1] = (long)MIN(MAX(rh.geomag_quat[1] * ((float)(1L << 30)), -2147483648.), 2147483647.);
    data[2] = (long)MIN(MAX(rh.geomag_quat[2] * ((float)(1L << 30)), -2147483648.), 2147483647.);
    data[3] = (long)MIN(MAX(rh.geomag_quat[3] * ((float)(1L << 30)), -2147483648.), 2147483647.);
    *timestamp = rh.geomag_timestamp;
    return INV_SUCCESS;
}

/** Returns a quaternion.
 * @param[out] data 9-axis quaternion.
 * @return Returns INV_SUCCESS if successful or an error code if not.
 */
inv_error_t inv_get_quaternion_float(float *data)
{
    memcpy(data, rh.nav_quat, sizeof(rh.nav_quat));
    return INV_SUCCESS;
}

/** Returns a quaternion based only on gyro and accel.
 * @param[out] data 6-axis  gyro and accel quaternion.
 * @return Returns INV_SUCCESS if successful or an error code if not.
 */
inv_error_t inv_get_6axis_quaternion_float(float *data, inv_time_t *timestamp)
{
    memcpy(data, rh.game_quat, sizeof(rh.game_quat));
    *timestamp = rh.gam_timestamp;
    return INV_SUCCESS;
}

/** Returns a quaternion based only on compass and accel.
 * @param[out] data 6-axis  compass and accel quaternion.
 * @return Returns INV_SUCCESS if successful or an error code if not.
 */
inv_error_t inv_get_geomagnetic_quaternion_float(float *data, inv_time_t *timestamp)
{
    memcpy(data, rh.geomag_quat, sizeof(rh.geomag_quat));
    *timestamp = rh.geomag_timestamp;
    return INV_SUCCESS;
}

/** Returns a quaternion with accuracy and timestamp.
 * @param[out] data 9-axis quaternion scaled such that 1.0 = 2^30.
 * @param[out] accuracy Accuracy of quaternion, 0-3, where 3 is most accurate.
 * @param[out] timestamp Timestamp of this quaternion in nanoseconds
 */
void inv_get_quaternion_set(long *data, int *accuracy, inv_time_t *timestamp)
{
    inv_get_quaternion(data);
    *timestamp = inv_get_last_timestamp();
    if (inv_get_compass_on()) {
        *accuracy = inv_get_mag_accuracy();
    } else if (inv_get_gyro_on()) {
        *accuracy = inv_get_gyro_accuracy();
    }else if (inv_get_accel_on()) {
        *accuracy = inv_get_accel_accuracy();
    } else {
        *accuracy = 0;
    }
}

/** Callback that gets called everytime there is new data. It is 
 * registered by inv_start_results_holder().
 * @param[in] sensor_cal New sensor data to process.
 * @return Returns INV_SUCCESS if successful or an error code if not.
 */
inv_error_t inv_generate_results(struct inv_sensor_cal_t *sensor_cal)
{
    rh.sensor = sensor_cal;
    return INV_SUCCESS;
}

/** Function to turn on this module. This is automatically called by
 *  inv_enable_results_holder(). Typically not called by users.
 * @return Returns INV_SUCCESS if successful or an error code if not.
 */
inv_error_t inv_start_results_holder(void)
{
    inv_register_data_cb(inv_generate_results, INV_PRIORITY_RESULTS_HOLDER,
        INV_GYRO_NEW | INV_ACCEL_NEW | INV_MAG_NEW);
    return INV_SUCCESS;
}

/** Initializes results holder. This is called automatically by the
* enable function inv_enable_results_holder(). It may be called any time the feature is enabled, but
* is typically not needed to be called by outside callers.
* @return Returns INV_SUCCESS if successful or an error code if not.
*/
inv_error_t inv_init_results_holder(void)
{
    memset(&rh, 0, sizeof(rh));
    rh.mag_scale[0] = 1L<<30;
    rh.mag_scale[1] = 1L<<30;
    rh.mag_scale[2] = 1L<<30;
    rh.compass_correction[0] = 1L<<30;
    rh.gam_quat[0] = 1L<<30;
    rh.nav_quat[0] = 1.;
    rh.geomag_quat[0] = 1.;
    rh.game_quat[0] = 1.;
    rh.accel_quat[0] = 1L<<30;
    rh.geomag_compass_correction[0] = 1L<<30;
    rh.quat_confidence_interval = (float)M_PI;
#ifdef WIN32
    rh.last_quat[0] = 1L<<30;
#endif


#ifdef TEST
    {
        struct local_field_t local_field;
        local_field.intensity = 48.0f;   // uT
        local_field.inclination = 60.0f; // degree
        local_field.declination = 0.0f;  // degree
        inv_set_earth_magnetic_local_field_parameter(&local_field);
    //    inv_set_local_field_status(LOCAL_FILED_NOT_SET_BY_USER);
        inv_set_local_field_status(LOCAL_FILED_SET_BY_USER);
   	    inv_set_local_magnetic_field(48.0f, 59.0f, 0.0f);

        // set default mpl calibration result for testing
        local_field.intensity = 50.0f;   // uT
        local_field.inclination = 59.0f; // degree
        local_field.declination = 0.0f;  // degree
        inv_set_mpl_magnetic_local_field_parameter(&local_field);
        inv_set_mpl_mag_field_status(LOCAL_FIELD_SET_BUT_NOT_MATCH_WITH_MPL);
    }
#endif

    return INV_SUCCESS;
}

/** Turns on storage of results.
*/
inv_error_t inv_enable_results_holder()
{
    inv_error_t result;
    result = inv_init_results_holder();
    if ( result ) {
        return result;
    }

    result = inv_register_mpl_start_notification(inv_start_results_holder);
    return result;
}

/** Sets state of if we know the accel bias.
 * @return return 1 if we know the accel bias, 0 if not.
 *            it is set with inv_set_accel_bias_found()
 */
int inv_got_accel_bias()
{
    return rh.got_accel_bias;
}

/** Sets whether we know the accel bias
 * @param[in] state Set to 1 if we know the accel bias. 
 *            Can be retrieved with inv_got_accel_bias()
 */
void inv_set_accel_bias_found(int state)
{
    rh.got_accel_bias = state;
}

/** Sets state of if we know the compass bias.
 * @return return 1 if we know the compass bias, 0 if not.
 *            it is set with inv_set_compass_bias_found()
 */
int inv_got_compass_bias()
{
    return rh.got_compass_bias;
}

/** Sets whether we know the compass bias
 * @param[in] state Set to 1 if we know the compass bias. 
 *            Can be retrieved with inv_got_compass_bias()
 */
void inv_set_compass_bias_found(int state)
{
    rh.got_compass_bias = state;
}

/** Sets the compass state.
 * @param[in] state Compass state. It can be retrieved with inv_get_compass_state().
 */
void inv_set_compass_state(int state)
{
    rh.compass_state = state;
}

/** Get's the compass state
 * @return the compass state that was set with inv_set_compass_state()
 */
int inv_get_compass_state()
{
    return rh.compass_state;
}

/** Set compass bias error. See inv_get_compass_bias_error()
 * @param[in] bias_error Set's how accurate we know the compass bias. It is the 
 * error squared.
 */
void inv_set_compass_bias_error(const long *bias_error)
{
    memcpy(rh.compass_bias_error, bias_error, sizeof(rh.compass_bias_error));
}

/** Get's compass bias error. See inv_set_compass_bias_error() for setting.
 * @param[out] bias_error Accuracy as to how well the compass bias is known. It is the error squared.
 */
void inv_get_compass_bias_error(long *bias_error)
{
    memcpy(bias_error, rh.compass_bias_error, sizeof(rh.compass_bias_error));
}

/**
 *  @brief      Returns 3-element vector of accelerometer data in body frame
 *                with gravity removed
 *  @param[out] data    3-element vector of accelerometer data in body frame
 *                with gravity removed
 *  @return     INV_SUCCESS if successful
 *              INV_ERROR_INVALID_PARAMETER if invalid input pointer
 */
inv_error_t inv_get_linear_accel(long *data)
{
    long gravity[3];

    if (data != NULL)
    {
        inv_get_accel_set(data, NULL, NULL);
        inv_get_gravity(gravity);
        data[0] -= gravity[0] >> 14;
        data[1] -= gravity[1] >> 14;
        data[2] -= gravity[2] >> 14;
        return INV_SUCCESS;
    }
    else {
        return INV_ERROR_INVALID_PARAMETER;
    }
}

/**
 *  @brief      Returns 3-element vector of accelerometer data in body frame
 *  @param[out] data    3-element vector of accelerometer data in body frame
 *  @return     INV_SUCCESS if successful
 *              INV_ERROR_INVALID_PARAMETER if invalid input pointer
 */
inv_error_t inv_get_accel(long *data)
{
    if (data != NULL) {
        inv_get_accel_set(data, NULL, NULL);
        return INV_SUCCESS;
    }
    else {
        return INV_ERROR_INVALID_PARAMETER;
    }
}

/**
 *  @brief      Returns 3-element vector of accelerometer float data
 *  @param[out] data    3-element vector of accelerometer float data
 *  @return     INV_SUCCESS if successful
 *              INV_ERROR_INVALID_PARAMETER if invalid input pointer
 */
inv_error_t inv_get_accel_float(float *data)
{
    long tdata[3];
    unsigned char i;

    if (data != NULL && !inv_get_accel(tdata)) {
        for (i = 0; i < 3; ++i) {
            data[i] = ((float)tdata[i] / (1L << 16));
        }
        return INV_SUCCESS;
    }
    else {
        return INV_ERROR_INVALID_PARAMETER;
    }
}

/**
 *  @brief      Returns 3-element vector of gyro float data
 *  @param[out] data    3-element vector of gyro float data
 *  @return     INV_SUCCESS if successful
 *              INV_ERROR_INVALID_PARAMETER if invalid input pointer
 */
inv_error_t inv_get_gyro_float(float *data)
{
    long tdata[3];
    unsigned char i;

    if (data != NULL) {
        inv_get_gyro_set(tdata, NULL, NULL);
        for (i = 0; i < 3; ++i) {
            data[i] = ((float)tdata[i] / (1L << 16));
        }
        return INV_SUCCESS;
    }
    else {
        return INV_ERROR_INVALID_PARAMETER;
    }
}

/** Set 9 axis 95% heading confidence interval for quaternion
* @param[in] ci Confidence interval in radians.
*/
void inv_set_heading_confidence_interval(float ci)
{
    rh.quat_confidence_interval = ci;
}

/** Get 9 axis 95% heading confidence interval for quaternion
* @return Confidence interval in radians.
*/
float inv_get_heading_confidence_interval(void)
{
    return rh.quat_confidence_interval;
}

/** Set 6 axis (accel and compass) 95% heading confidence interval for quaternion
* @param[in] ci Confidence interval in radians.
*/
void inv_set_accel_compass_confidence_interval(float ci)
{
    rh.geo_mag_confidence_interval = ci;
}

/** Get 6 axis (accel and compass) 95% heading confidence interval for quaternion
* @return Confidence interval in radians.
*/
float inv_get_accel_compass_confidence_interval(void)
{
    return rh.geo_mag_confidence_interval;
}

/**
 *  @brief      Returns 3-element vector of linear accel float data
 *  @param[out] data    3-element vector of linear aceel float data
 *  @return     INV_SUCCESS if successful
 *              INV_ERROR_INVALID_PARAMETER if invalid input pointer
 */
inv_error_t inv_get_linear_accel_float(float *data)
{
    long tdata[3];
    unsigned char i;

    if (data != NULL && !inv_get_linear_accel(tdata)) {
        for (i = 0; i < 3; ++i) {
            data[i] = ((float)tdata[i] / (1L << 16));
        }
        return INV_SUCCESS;
    }
    else {
        return INV_ERROR_INVALID_PARAMETER;
    }
}

/**
 *  @brief      Returns the status of earth magnetic field local field parameters
 *  @param[out] N/A
 *  @return     status of local field, defined in enum compass_local_field_e
 */
enum compass_local_field_e inv_get_local_field_status(void)
{
    return rh.mag_local_field.mpl_match_status;
}

/** Set the status of earth magnetic field local field parameters
* @param[in] status of earth magnetic field local field parameters.
*/
void inv_set_local_field_status(enum compass_local_field_e status)
{
    rh.mag_local_field.mpl_match_status = status;
}

/** Set the parameters of earth magnetic field local field
* @param[in] the earth magnetic field local field parameters.
*/
void inv_set_earth_magnetic_local_field_parameter(struct local_field_t *parameters)
{
    rh.mag_local_field.intensity = parameters->intensity;        // radius
    rh.mag_local_field.inclination = parameters->inclination;    // dip angle
    rh.mag_local_field.declination = parameters->declination;    // yaw deviation angle from true north
    
    inv_set_local_field_status(LOCAL_FILED_SET_BY_USER);
}

/**
 *  @brief      Returns the parameters of earth magnetic field local field
 *  @param[out] the parameters of earth magnetic field local field
 *  @return     N/A
 */
void inv_get_earth_magnetic_local_field_parameter(struct local_field_t *parameters)
{
    parameters->intensity = rh.mag_local_field.intensity;        // radius
    parameters->inclination = rh.mag_local_field.inclination;    // dip angle
    parameters->declination = rh.mag_local_field.declination;    // yaw deviation angle from true north
    parameters->mpl_match_status = rh.mag_local_field.mpl_match_status;
}

/**
 *  @brief      Returns the status of mpl calibrated magnetic field local field parameters
 *  @param[out] N/A
 *  @return     status of local field, defined in enum compass_local_field_e
 */
enum compass_local_field_e inv_get_mpl_mag_field_status(void)
{
    return rh.mpl_compass_cal.mpl_match_status;
}

/** Set the status of mpl calibrated magnetic field local field parameters
* @param[in] status of earth magnetic field local field parameters.
*/
void inv_set_mpl_mag_field_status(enum compass_local_field_e status)
{
    rh.mpl_compass_cal.mpl_match_status = status;
}

/** Set the magnetic field local field struct object
* @param[in] status of earth magnetic field local field parameters.
*/
inv_error_t inv_set_local_magnetic_field(float intensity, float inclination, float declination)
{
	struct local_field_t local_field;
	local_field.intensity = intensity;  // radius
    local_field.inclination = inclination; // dip angle angle degree
    local_field.declination = declination; // yaw deviation angle from true north, eastward as positive
    local_field.mpl_match_status = LOCAL_FILED_SET_BY_USER;

	inv_set_earth_magnetic_local_field_parameter(&local_field);
	return 0;
}

/** Set the parameters of mpl calibrated magnetic field local field
*   This API is used by mpl only.
* @param[in] the earth magnetic field local field parameters.
 *  @return     INV_SUCCESS if successful
 *              INV_ERROR_INVALID_PARAMETER if invalid input pointer
 */
inv_error_t inv_set_mpl_magnetic_local_field_parameter(struct local_field_t *parameters)
{
    enum compass_local_field_e mpl_status;
    struct local_field_t local_field;
    inv_error_t status;

    rh.mpl_compass_cal.intensity = parameters->intensity;        // radius
    rh.mpl_compass_cal.inclination = parameters->inclination;    // dip angle
    rh.mpl_compass_cal.declination = parameters->declination;    // yaw deviation angle from true north

    mpl_status = inv_get_mpl_mag_field_status();
    inv_get_earth_magnetic_local_field_parameter(&local_field);

    status = INV_SUCCESS;

    switch (inv_get_local_field_status())  {
    case LOCAL_FILED_NOT_SET_BY_USER:
        if (mpl_status == LOCAL_FILED_NOT_SET_BY_USER)  {
            inv_set_mpl_mag_field_status(LOCAL_FILED_NOT_SET_BY_USER_BUT_SET_BY_MPL);
        } else {
            // illegal status
            status = INV_ERROR_INVALID_PARAMETER;
        }
        break;
    case LOCAL_FILED_SET_BY_USER:
        switch (mpl_status) {
            case LOCAL_FIELD_SET_BUT_NOT_MATCH_WITH_MPL:
            case LOCAL_FIELD_SET_MATCH_WITH_MPL:
                if  ( (ABS(local_field.intensity - parameters->intensity) < 5.0f) &&
                      (ABS(local_field.intensity - parameters->intensity) < 5.0f)  )  {
                    inv_set_mpl_mag_field_status(LOCAL_FIELD_SET_MATCH_WITH_MPL);
                } else {
                    inv_set_mpl_mag_field_status(LOCAL_FIELD_SET_BUT_NOT_MATCH_WITH_MPL);
                }
            break;
            case LOCAL_FILED_NOT_SET_BY_USER_BUT_SET_BY_MPL:
            // no status update            
            break;
            case LOCAL_FILED_NOT_SET_BY_USER:
            case LOCAL_FILED_SET_BY_USER:
            default:
            // illegal status
            status = INV_ERROR_INVALID_PARAMETER;
            break;
        }
        break;
    case LOCAL_FILED_NOT_SET_BY_USER_BUT_SET_BY_MPL:
    case LOCAL_FIELD_SET_BUT_NOT_MATCH_WITH_MPL:
    case LOCAL_FIELD_SET_MATCH_WITH_MPL:
    default:
        // illegal status
        status = INV_ERROR_INVALID_PARAMETER;
        break;
    }
    return status;
}

/**
 *  @brief      Returns the parameters of mpl calibrated magnetic field local field
 *  @param[out] the parameters of earth magnetic field local field
 *  @return     N/A
 */
void inv_get_mpl_magnetic_local_field_parameter(struct local_field_t *parameters)
{
    parameters->intensity = rh.mpl_compass_cal.intensity;        // radius
    parameters->inclination = rh.mpl_compass_cal.inclination;    // dip angle
    parameters->declination = rh.mpl_compass_cal.declination;    // yaw deviation angle from true north
    parameters->mpl_match_status = rh.mpl_compass_cal.mpl_match_status;
}

/**
 * @}
 */
