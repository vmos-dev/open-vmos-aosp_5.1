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
 * $Id: pressure.h 4092 2010-11-17 23:49:22Z kkeal $
 *
 *******************************************************************************/

#ifndef PRESSURE_H
#define PRESSURE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "mltypes.h"
#include "mpu.h"
#ifdef INV_INCLUDE_LEGACY_HEADERS
#include "pressure_legacy.h"
#endif

    /* ------------ */
    /* - Defines. - */
    /* ------------ */

#define USE_PRESSURE_BMA                    0

#define PRESSURE_SLAVEADDR_INVALID          0x00
#define PRESSURE_SLAVEADDR_BMA085           0x77

/*
    Define default pressure to use if no selection is made
*/
#if USE_PRESSURE_BMA
#define DEFAULT_PRESSURE_TYPE              PRESSURE_ID_BMA
#endif

    /* --------------- */
    /* - Structures. - */
    /* --------------- */

    /* --------------------- */
    /* - Function p-types. - */
    /* --------------------- */

    unsigned char inv_pressure_present(void);
    unsigned char inv_get_pressure_slave_addr(void);
    inv_error_t inv_suspend_pressure(void);
    inv_error_t inv_resume_presure(void);
    inv_error_t inv_get_pressure_data(long *data);
    unsigned short inv_get_pressure_id(void);

#ifdef __cplusplus
}
#endif
#endif                          // PRESSURE_H
