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
 * $Id:$
 *
 ******************************************************************************/

#ifndef INV_STORED_DATA_H
#define INV_STORED_DATA_H

#ifdef __cplusplus
extern "C" {
#endif

/*
    Includes.
*/

#include "mltypes.h"

/*
    Defines
*/
#define INV_CAL_ACCEL_LEN    (12)
#define INV_CAL_COMPASS_LEN  (555 + 5)
#define INV_CAL_HDR_LEN      (6)
#define INV_CAL_CHK_LEN      (4)

/*
    APIs
*/
    inv_error_t inv_load_calibration(void);
    inv_error_t inv_store_calibration(void);

/*
    Other prototypes
*/
    inv_error_t inv_load_cal(unsigned char *calData);
    inv_error_t inv_store_cal(unsigned char *calData, int length);
    unsigned int inv_get_cal_length(void);

#ifdef __cplusplus
}
#endif
#endif                          /* INV_STORED_DATA_H */
