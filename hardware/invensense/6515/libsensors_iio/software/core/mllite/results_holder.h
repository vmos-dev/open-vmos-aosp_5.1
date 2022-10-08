/*
 $License:
    Copyright (C) 2011-2012 InvenSense Corporation, All Rights Reserved.
    See included License.txt for License information.
 $
 */
#include "mltypes.h"

#ifndef INV_RESULTS_HOLDER_H__
#define INV_RESULTS_HOLDER_H__

#ifdef __cplusplus
extern "C" {
#endif

#define INV_MOTION                       0x0001
#define INV_NO_MOTION                    0x0002



    /**************************************************************************/
    /*  The value of inv_get_gyro_sum_of_sqr is scaled such the (1 dps)^2 =   */
    /*  2^GYRO_MAG_SQR_SHIFT. This number must be >=0 and even.               */
    /*  The value of inv_accel_sum_of_sqr is scaled such that (1g)^2 =        */
    /*  2^ACC_MAG_SQR_SHIFT                                                   */
    /**************************************************************************/
#define ACC_MAG_SQR_SHIFT 16

enum compass_local_field_e {
    // status for user input earth magnetic local field
    LOCAL_FILED_NOT_SET_BY_USER                     = 0,
    LOCAL_FILED_SET_BY_USER                         = 1,

    // status for mpl calibrated based magnetical field
    LOCAL_FILED_NOT_SET_BY_USER_BUT_SET_BY_MPL      = 2,
    LOCAL_FIELD_SET_BUT_NOT_MATCH_WITH_MPL          = 3,
    LOCAL_FIELD_SET_MATCH_WITH_MPL                  = 4,
};

struct local_field_t {
    float intensity;  // radius
    float inclination; // dip angle angle degree
    float declination; // yaw deviation angle from true north, eastward as positive
    enum compass_local_field_e mpl_match_status;
};

// earth magnetic field access API
enum compass_local_field_e inv_get_local_field_status(void);
void inv_set_local_field_status(enum compass_local_field_e status);

void inv_set_earth_magnetic_local_field_parameter(struct local_field_t *parameters);
void inv_get_earth_magnetic_local_field_parameter(struct local_field_t *parameters);

// mpl calibrated magnetic field access API
enum compass_local_field_e inv_get_mpl_mag_field_status(void);
void inv_set_mpl_mag_field_status(enum compass_local_field_e status);

inv_error_t inv_set_mpl_magnetic_local_field_parameter(struct local_field_t *parameters);
void inv_get_mpl_magnetic_local_field_parameter(struct local_field_t *parameters);

// quaternion store API
void inv_store_gaming_quaternion(const long *quat, inv_time_t timestamp);
void inv_store_accel_quaternion(const long *quat, inv_time_t timestamp);
void inv_store_nav_quaternion(const float *quat, inv_time_t timestamp);
void inv_store_game_quaternion(const float *quat, inv_time_t timestamp);
void inv_store_geomag_quaternion(const float *quat, inv_time_t timestamp);

// States
#define SF_NORMAL 0
#define SF_UNCALIBRATED 1
#define SF_STARTUP_SETTLE 2
#define SF_FAST_SETTLE 3
#define SF_DISTURBANCE 4
#define SF_SLOW_SETTLE 5

// These 2 status bits are used to control when the 9 axis quaternion is updated
#define INV_COMPASS_CORRECTION_SET 1
#define INV_6_AXIS_QUAT_SET 2
#define INV_GEOMAGNETIC_CORRECTION_SET 4

int inv_get_acc_state();
void inv_set_acc_state(int state);
int inv_get_motion_state(unsigned int *cntr);
void inv_set_motion_state(unsigned char state);
inv_error_t inv_get_gravity(long *data);
inv_error_t inv_get_gravity_6x(long *data);
inv_error_t inv_get_6axis_quaternion(long *data, inv_time_t *timestamp);
inv_error_t inv_get_quaternion(long *data);
inv_error_t inv_get_quaternion_float(float *data);
inv_error_t inv_get_6axis_quaternion_float(float *data, inv_time_t *timestamp);
inv_error_t inv_get_geomagnetic_quaternion_float(float *data, inv_time_t *timestamp);
#ifdef WIN32
inv_error_t inv_get_last_quaternion(long *data);
inv_error_t inv_set_last_quaternion(long *data);
#endif
void inv_get_quaternion_set(long *data, int *accuracy, inv_time_t *timestamp);
inv_error_t inv_get_accel_quaternion(long *data);
inv_error_t inv_get_geomagnetic_quaternion(long *data, inv_time_t *timestamp);
void inv_set_geomagnetic_compass_correction(const long *data, inv_time_t timestamp);
void inv_get_geomagnetic_compass_correction(long *data, inv_time_t *timestamp);
inv_error_t inv_get_result_holder_status(long *rh_status);
inv_error_t inv_set_result_holder_status(long rh_status);
inv_error_t inv_get_quaternion_validity(int *value);
inv_error_t inv_set_quaternion_validity(int value);

// set magnetic field by location
inv_error_t inv_set_local_magnetic_field(float intensity, float inclination, float declination);

inv_error_t inv_enable_results_holder();
inv_error_t inv_init_results_holder(void);

/* Magnetic Field Parameters*/
void inv_set_mag_scale(const long *data);
void inv_get_mag_scale(long *data);
void inv_set_compass_correction(const long *data, inv_time_t timestamp);
void inv_get_compass_correction(long *data, inv_time_t *timestamp);
int inv_got_compass_bias();
void inv_set_compass_bias_found(int state);
int inv_get_large_mag_field();
void inv_set_large_mag_field(int state);
void inv_set_compass_state(int state);
int inv_get_compass_state();
void inv_set_compass_bias_error(const long *bias_error);
void inv_get_compass_bias_error(long *bias_error);
inv_error_t inv_get_linear_accel(long *data);
inv_error_t inv_get_accel(long *data);
inv_error_t inv_get_accel_float(float *data);
inv_error_t inv_get_gyro_float(float *data);
inv_error_t inv_get_linear_accel_float(float *data);
void inv_set_heading_confidence_interval(float ci);
float inv_get_heading_confidence_interval(void);

void inv_set_accel_compass_confidence_interval(float ci);
float inv_get_accel_compass_confidence_interval(void);

int inv_got_accel_bias();
void inv_set_accel_bias_found(int state);


#ifdef __cplusplus
}
#endif

#endif // INV_RESULTS_HOLDER_H__
