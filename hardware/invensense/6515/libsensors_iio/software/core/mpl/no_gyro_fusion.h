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

#ifndef MLDMP_NOGYROFUSION_H__
#define MLDMP_NOGYROFUSION_H__
#include "mltypes.h"

#ifdef __cplusplus
extern "C" {
#endif

    inv_error_t inv_enable_no_gyro_fusion(void);
    inv_error_t inv_disable_no_gyro_fusion(void);
    inv_error_t inv_start_no_gyro_fusion(void);
    inv_error_t inv_start_no_gyro_fusion(void);
    inv_error_t inv_init_no_gyro_fusion(void);
    int inv_verify_no_gyro_fusion_data(float *data);

#ifdef __cplusplus
}
#endif


#endif // MLDMP_NOGYROFUSION_H__
