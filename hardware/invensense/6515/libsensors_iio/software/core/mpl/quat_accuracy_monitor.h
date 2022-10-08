/*
	quat_accuracy_monitor.h
 $License:
    Copyright (C) 2011-2012 InvenSense Corporation, All Rights Reserved.
    See included License.txt for License information.
 $
 */

/*******************************************************************************
 *
 * $Id:$
 *
 ******************************************************************************/

#ifndef QUAT_ACCURARCY_MONITOR_H__
#define QUAT_ACCURARCY_MONITOR_H__

#include "mltypes.h"

#ifdef __cplusplus
extern "C" {
#endif
enum accuracy_signal_type_e {
    TYPE_NAV_QUAT,
    TYPE_GAM_QUAT,
    TYPE_NAV_QUAT_ADVANCED,
    TYPE_GAM_QUAT_ADVANCED,
    TYPE_NAV_QUAT_BASIC,
    TYPE_GAM_QUAT_BASIC,
    TYPE_MAG,
    TYPE_GYRO,
    TYPE_ACCEL,
};

inv_error_t inv_init_quat_accuracy_monitor(void);

void set_accuracy_threshold(enum accuracy_signal_type_e type, double threshold);
double get_accuracy_threshold(enum accuracy_signal_type_e type);
void set_accuracy_weight(enum accuracy_signal_type_e type, int weight);
int get_accuracy_weight(enum accuracy_signal_type_e type);

int8_t get_accuracy_accuracy(enum accuracy_signal_type_e type);

void inv_reset_quat_accuracy(void);
double get_6axis_correction_term(void);
double get_9axis_correction_term(void);
int get_9axis_accuracy_state();

void set_6axis_error_average(double value);
double get_6axis_error_bound(void);
double get_compass_correction(void);
double get_9axis_error_bound(void);

float get_confidence_interval(void);
void set_compass_uncertainty(float value);

inv_error_t inv_enable_quat_accuracy_monitor(void);
inv_error_t inv_disable_quat_accuracy_monitor(void);
inv_error_t inv_start_quat_accuracy_monitor(void);
inv_error_t inv_stop_quat_accuracy_monitor(void);

double get_compassNgravity(void);
double get_init_compassNgravity(void);

float inv_heading_accuracy_check(float orient[3], float *heading, int8_t *accuracy);

#ifdef __cplusplus
}
#endif

#endif // QUAT_ACCURARCY_MONITOR_H__
