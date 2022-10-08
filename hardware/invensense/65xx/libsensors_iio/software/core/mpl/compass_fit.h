/*
 $License:
    Copyright (C) 2011-2012 InvenSense Corporation, All Rights Reserved.
    See included License.txt for License information.
 $
 */

#ifndef INV_MLDMP_COMPASSFIT_H__
#define INV_MLDMP_COMPASSFIT_H__

#include "mltypes.h"

#ifdef __cplusplus
extern "C" {
#endif

void inv_init_compass_fit(void);
inv_error_t inv_enable_compass_fit(void);
inv_error_t inv_disable_compass_fit(void);
inv_error_t inv_start_compass_fit(void);
inv_error_t inv_stop_compass_fit(void);

#ifdef __cplusplus
}
#endif


#endif // INV_MLDMP_COMPASSFIT_H__
