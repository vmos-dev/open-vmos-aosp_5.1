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
 * $Id: mlstates.h 5629 2011-06-11 03:13:08Z mcaramello $
 *
 *******************************************************************************/

#ifndef INV_STATES_H__
#define INV_STATES_H__

#include "mltypes.h"
#ifdef INV_INCLUDE_LEGACY_HEADERS
#include "mlstates_legacy.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* See inv_state_transition for the transition mask */
#define INV_STATE_SERIAL_CLOSED      (0)
#define INV_STATE_SERIAL_OPENED      (1)
#define INV_STATE_DMP_OPENED         (2)
#define INV_STATE_DMP_STARTED        (3)
#define INV_STATE_DMP_STOPPED        (INV_STATE_DMP_OPENED)
#define INV_STATE_DMP_CLOSED         (INV_STATE_SERIAL_OPENED)

#define INV_STATE_NAME(x)            (#x)

    typedef inv_error_t(*state_change_callback_t) (unsigned char newState);

    char *inv_state_name(unsigned char x);
    inv_error_t inv_state_transition(unsigned char newState);
    unsigned char inv_get_state(void);
    inv_error_t inv_register_state_callback(state_change_callback_t callback);
    inv_error_t inv_unregister_state_callback(state_change_callback_t callback);
    inv_error_t inv_run_state_callbacks(unsigned char newState);

#ifdef __cplusplus
}
#endif
#endif                          // INV_STATES_H__
