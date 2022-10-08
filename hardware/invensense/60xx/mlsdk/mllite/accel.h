/*
 $License:
   Copyright 2011 InvenSense, Inc.

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
  $
 */
/*******************************************************************************
 *
 * $Id: accel.h 4580 2011-01-22 03:19:23Z prao $
 *
 *******************************************************************************/

#ifndef ACCEL_H
#define ACCEL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "mltypes.h"
#include "mpu.h"
#ifdef INV_INCLUDE_LEGACY_HEADERS
#include "accel_legacy.h"
#endif

    /* ------------ */
    /* - Defines. - */
    /* ------------ */

    /* --------------- */
    /* - Structures. - */
    /* --------------- */

    /* --------------------- */
    /* - Function p-types. - */
    /* --------------------- */

    unsigned char inv_accel_present(void);
    unsigned char inv_get_slave_addr(void);
    inv_error_t inv_get_accel_data(long *data);
    unsigned short inv_get_accel_id(void);

#ifdef __cplusplus
}
#endif
#endif                          // ACCEL_H
