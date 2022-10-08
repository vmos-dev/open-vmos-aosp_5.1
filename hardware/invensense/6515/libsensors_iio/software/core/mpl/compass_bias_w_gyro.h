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

#ifndef MLDMP_COMPASSBIASWGYRO_H__
#define MLDMP_COMPASSBIASWGYRO_H__
#include "mltypes.h"

#ifdef __cplusplus
extern "C" {
#endif

    inv_error_t inv_enable_compass_bias_w_gyro();
    inv_error_t inv_disable_compass_bias_w_gyro();
    void inv_init_compass_bias_w_gyro();

#ifdef __cplusplus
}
#endif


#endif // MLDMP_COMPASSBIASWGYRO_H__
