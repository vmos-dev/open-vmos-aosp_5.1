/*
 $License:
    Copyright (C) 2011-2012 InvenSense Corporation, All Rights Reserved.
    See included License.txt for License information.
 $
 */

/******************************************************************************
 *
 * $Id$
 *
 *****************************************************************************/

#ifndef MLDMP_FUSION9AXIS_H__
#define MLDMP_FUSION9AXIS_H__

#include "mltypes.h"

#ifdef __cplusplus
extern "C" {
#endif

    void inv_init_9x_fusion(void);
    inv_error_t inv_9x_fusion_state_change(unsigned char newState);
    inv_error_t inv_enable_9x_sensor_fusion(void);
    inv_error_t inv_disable_9x_sensor_fusion(void);
    inv_error_t inv_start_9x_sensor_fusion(void);
    inv_error_t inv_stop_9x_sensor_fusion(void);
    inv_error_t inv_9x_fusion_set_mag_fb(double fb);
    inv_error_t inv_9x_fusion_enable_jitter_reduction(int en);
    int inv_verify_9x_fusion_data(float *data);

    float inv_9x_sensor_fusion_get_correction_angle(void);
#ifdef __cplusplus
}
#endif


#endif // MLDMP_FUSION9AXIS_H__
