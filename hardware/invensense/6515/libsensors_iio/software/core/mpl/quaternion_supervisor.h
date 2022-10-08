/*
 $License:
    Copyright (C) 2011-2012 InvenSense Corporation, All Rights Reserved.
    See included License.txt for License information.
 $
 */
#ifndef INV_QUATERNION_SUPERVISOR_H__
#define INV_QUATERNION_SUPERVISOR_H__

#include "mltypes.h"


#ifdef __cplusplus
extern "C" {
#endif

#define ACCELERATION_SQUARE_1P5G   9663676416LL
#define ACCELERATION_SQUARE_1P2G   6184752906LL
inv_error_t inv_enable_quaternion(void);
inv_error_t inv_disable_quaternion(void);
inv_error_t inv_init_quaternion(void);
inv_error_t inv_start_quaternion(void);
void inv_set_quaternion(long *quat);
int inv_verify_6x_fusion_data(float *data);

#ifdef __cplusplus
}
#endif

#endif // INV_QUATERNION_SUPERVISOR_H__
