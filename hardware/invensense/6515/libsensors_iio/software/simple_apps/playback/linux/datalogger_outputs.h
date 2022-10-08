/**
 *   @defgroup  HAL_Outputs
 *   @brief     Motion Library - HAL Outputs
 *              Sets up common outputs for HAL
 *
 *   @{
 *       @file  datalogger_outputs.h
 *       @brief Windows 8 HAL outputs.
 */

#ifndef _DATALOGGER_OUTPUTS_H_
#define _DATALOGGER_OUTPUTS_H_

#include "mltypes.h"

#ifdef __cplusplus
extern "C" {
#endif

/* calibrated data */
void inv_get_sensor_type_temperature_float(float *value,
        inv_time_t *timestamp);
void inv_get_sensor_type_gyro_float(float *values, int8_t *accuracy,
        inv_time_t *timestamp);
void inv_get_sensor_type_accel_float(float *values, int8_t *accuracy,
        inv_time_t *timestamp);
void inv_get_sensor_type_compass_float(float *mag_north, float *true_north,
        float *values, int8_t *accuracy, inv_time_t *timestamp);
void inv_get_sensor_type_quat_float(float *values, int *accuracy,
        inv_time_t *timestamp);
void inv_get_sensor_type_gravity_float(float *values, int *accuracy,
        inv_time_t * timestamp);
void inv_get_sensor_type_rotation_vector_float(float *values, int *accuracy,
        inv_time_t * timestamp);

/* uncalibrated data */
void inv_get_sensor_type_gyro_raw_short(short *values,
        inv_time_t *timestamp);
void inv_get_sensor_type_gyro_raw_body_float(float *values,
        inv_time_t *timestamp);
void inv_get_sensor_type_accel_raw_short(short *values,
        inv_time_t *timestamp);
void inv_get_sensor_type_compass_raw_short(short *values,
        inv_time_t *timestamp);

/* enabler/disabler APIs */
inv_error_t inv_enable_datalogger_outputs(void);
inv_error_t inv_disable_datalogger_outputs(void);
inv_error_t inv_init_datalogger_outputs(void);
inv_error_t inv_start_datalogger_outputs(void);
inv_error_t inv_stop_datalogger_outputs(void);

#ifdef __cplusplus
}
#endif

#endif  /* #ifndef _DATALOGGER_OUTPUTS_H_ */

/**
 *  @}
 */
