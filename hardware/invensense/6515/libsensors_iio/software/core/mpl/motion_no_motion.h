/*
 $License:
    Copyright (C) 2011-2012 InvenSense Corporation, All Rights Reserved.
    See included License.txt for License information.
 $
 */
#ifndef INV_MOTION_NO_MOTION_H__
#define INV_MOTION_NO_MOTION_H__

#include "mltypes.h"
#include "invensense.h"

#ifdef __cplusplus
extern "C" {
#endif

inv_error_t inv_enable_motion_no_motion(void);
inv_error_t inv_disable_motion_no_motion(void);
inv_error_t inv_init_motion_no_motion(void);
inv_error_t inv_start_motion_no_motion(void);
inv_error_t inv_stop_motion_no_motion(void);

inv_error_t inv_set_no_motion_time(long time_ms);
inv_time_t motion_nomot_get_gyro_bias_update_time(void);
void motion_nomot_set_gyro_bias_update_time(struct inv_sensor_cal_t *sensor_cal);

#ifdef __cplusplus
}
#endif

#endif // INV_MOTION_NO_MOTION_H__
