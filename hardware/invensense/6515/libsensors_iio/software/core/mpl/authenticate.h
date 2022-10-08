/*
 $License:
    Copyright (C) 2011-2012 InvenSense Corporation, All Rights Reserved.
    See included License.txt for License information.
 $
 */

/*******************************************************************************
 *
 * $Id$
 *
 ******************************************************************************/

#ifndef MLDMP_AUTHENTICATE_H__
#define MLDMP_AUTHENTICATE_H__

#include "invensense.h"

inv_error_t inv_check_key(void);

#define INV_9AXIS_QUAT_VALID              0x10
#define INV_9AXIS_QUAT_FALSE              0x20
#define INV_6AXIS_QUAT_VALID              0x40
#define INV_6AXIS_QUAT_FALSE              0x80
#define INV_6AXIS_QUAT_NO_DMP             0x100
#define INV_6AXIS_GEOMAG_QUAT_VALID       0x200
#define INV_6AXIS_GEOMAG_QUAT_FALSE       0x400

#endif  /* MLDMP_AUTHENTICATE_H__ */
