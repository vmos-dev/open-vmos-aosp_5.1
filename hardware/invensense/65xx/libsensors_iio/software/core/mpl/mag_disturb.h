/*
 $License:
    Copyright (C) 2011-2012 InvenSense Corporation, All Rights Reserved.
    See included License.txt for License information.
 $
 */

#ifndef MLDMP_MAGDISTURB_H__
#define MLDMP_MAGDISTURB_H__

#include "mltypes.h"

#ifdef __cplusplus
extern "C" {
#endif

   // #define WIN_8

    int inv_check_magnetic_disturbance(unsigned long delta_time, const long *quat,
        const long *compass, const long *gravity);

    void inv_track_world_yaw_angle_angle(int mode, float currdip);

    inv_error_t inv_enable_magnetic_disturbance(void);
    inv_error_t inv_disable_magnetic_disturbance(void);
    int inv_get_magnetic_disturbance_state();

    inv_error_t inv_set_magnetic_disturbance(int time_ms);
    inv_error_t inv_init_magnetic_disturbance(void);

    void inv_enable_magnetic_disturbance_logging(void);
    void inv_disable_magnetic_disturbance_logging(void);

    float Mag3ofNormalizedLong(const long *x);
    float Mag2ofNormalizedLong(const long *x);
    float Mag2ofNormalizedFloat(const float *x);
    
    int inv_mag_disturb_get_detect_status_3D(void);
    void inv_mag_disturb_set_detect_status_3D(int status);

    int inv_mag_disturb_get_drop_heading_accuracy_status(void);
    void inv_mag_disturb_set_drop_heading_accuracy_status(int status);

    int inv_mag_disturb_get_detect_weak_status_3D(void);
    void inv_mag_disturb_set_detect_weak_status_3D(int status);

    int inv_mag_disturb_get_detect_world_yaw_angle_status(void);
    void inv_mag_disturb_set_detect_world_yaw_angle_status(int status);

    int inv_mag_disturb_get_detect_world_yaw_angle_confirm_status(void);
    void inv_mag_disturb_set_detect_world_yaw_angle_confirm_status(int status);

    float inv_mag_disturb_get_vector_radius_3D(void);
    void inv_mag_disturb_set_vector_radius_3D(float radius);

    void inv_mag_disturb_world_yaw_angle_init(void);
    void inv_mag_disturb_world_yaw_angle_process(struct inv_sensor_cal_t *obj);
    //enum mag_distub_state_e inv_mag_disturb_get_mar_world_yaw_angle_state(void);
    
    char inv_mag_disturb_get_mar_world_yaw_angle_detection_status(void);

    float inv_mag_disturb_world_yaw_angle_distortion_from_gyro_bias(struct inv_sensor_cal_t *obj);
    int inv_mag_disturb_get_dip_compassNgravity(struct inv_sensor_cal_t *data);

    float inv_mag_disturb_9x_quat_confidence_interval(struct inv_sensor_cal_t *obj);
    float inv_mag_disturb_geo_mag_confidence_interval(struct inv_sensor_cal_t *obj);

    float inv_mag_disturb_world_yaw_angle_distortion_from_accel_compass_bias(float accel_bias_error, float compass_bias_error);
    float inv_mag_disturb_world_yaw_angle_distortion_from_accel_compass_only(float accel_bias_error, float compass_bias_error);

    void inv_mag_disturb_all_confidence_interval_init(void);

    /************************/
    /* external API         */
    /************************/
    float inv_mag_disturb_get_magnitude_threshold(void);
    void inv_mag_disturb_set_magnitude_threshold(float radius);

    float inv_mag_disturb_get_time_threshold_detect(void);
    void inv_mag_disturb_set_time_threshold_detect(float time_seconds);

    float inv_mag_disturb_get_local_field_radius(void);
    void inv_mag_disturb_set_local_field_radius(float radius);

    float inv_mag_disturb_get_local_field_dip(void);
    void inv_mag_disturb_set_local_field_dip(float dip);

#ifdef __cplusplus
}
#endif


#endif // MLDMP_MAGDISTURB_H__
