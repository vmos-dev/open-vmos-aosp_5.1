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
 * $Id: mlstates.c 5629 2011-06-11 03:13:08Z mcaramello $
 *
 *******************************************************************************/

/** 
 *  @defgroup MLSTATES
 *  @brief  Basic state machine definition and support for the Motion Library.
 *
 *  @{
 *      @file mlstates.c
 *      @brief The Motion Library state machine definition.
 */

#define ML_C

/* ------------------ */
/* - Include Files. - */
/* ------------------ */

#include <stdio.h>
#include <string.h>

#include "mlstates.h"
#include "mltypes.h"
#include "mlinclude.h"
#include "ml.h"
#include "mlos.h"

#include <log.h>
#undef MPL_LOG_TAG
#define MPL_LOG_TAG "MPL-mlstates"

#define _stateDebug(x)          //{x}

#define MAX_STATE_CHANGE_PROCESSES (8)

struct state_callback_obj {
    int_fast8_t numStateChangeCallbacks;
    HANDLE mutex;
    state_change_callback_t stateChangeCallbacks[MAX_STATE_CHANGE_PROCESSES];
};

static struct state_callback_obj sStateChangeCallbacks = { 0 };

/* --------------- */
/* -  Functions. - */
/* --------------- */

static inv_error_t inv_init_state_callbacks(void)
{
    memset(&sStateChangeCallbacks, 0, sizeof(sStateChangeCallbacks));
    return inv_create_mutex(&sStateChangeCallbacks.mutex);
}

static inv_error_t MLStateCloseCallbacks(void)
{
    inv_error_t result;
    result = inv_destroy_mutex(sStateChangeCallbacks.mutex);
    memset(&sStateChangeCallbacks, 0, sizeof(sStateChangeCallbacks));
    return result;
}

/**
 *  @internal
 *  @brief  return a string containing the label assigned to the given state.
 *  @param  state   The state of which the label has to be returned.
 *  @return A string containing the state label.
**/
char *inv_state_name(unsigned char state)
{
    switch (state) {
    case INV_STATE_SERIAL_CLOSED:
        return INV_STATE_NAME(INV_STATE_SERIAL_CLOSED);
        break;
    case INV_STATE_SERIAL_OPENED:
        return INV_STATE_NAME(INV_STATE_SERIAL_OPENED);
        break;
    case INV_STATE_DMP_OPENED:
        return INV_STATE_NAME(INV_STATE_DMP_OPENED);
        break;
    case INV_STATE_DMP_STARTED:
        return INV_STATE_NAME(INV_STATE_DMP_STARTED);
        break;
    default:
        return NULL;
    }
}

/**
 *  @internal
 *  @brief  Perform a transition from the current state to newState.
 *          Check for the correctness of the transition.
 *          Print out an error message if the transition is illegal .
 *          This routine is also called if a certain normally constant parameters
 *          are changed such as the FIFO Rate.
 *  @param  newState    state we are transitioning to.
 *  @return  
**/
inv_error_t inv_state_transition(unsigned char newState)
{
    inv_error_t result = INV_SUCCESS;

    if (newState == INV_STATE_SERIAL_CLOSED) {
        // Always allow transition to closed
    } else if (newState == INV_STATE_SERIAL_OPENED) {
        inv_init_state_callbacks(); // Always allow first transition to start over
    } else if (((newState == INV_STATE_DMP_OPENED) &&
                ((inv_params_obj.state == INV_STATE_SERIAL_OPENED) ||
                 (inv_params_obj.state == INV_STATE_DMP_STARTED)))
               ||
               ((newState == INV_STATE_DMP_STARTED) &&
                (inv_params_obj.state == INV_STATE_DMP_OPENED))) {
        // Valid transitions but no special action required
    } else {
        // All other combinations are illegal
        MPL_LOGE("Error : illegal state transition from %s to %s\n",
                 inv_state_name(inv_params_obj.state),
                 inv_state_name(newState));
        result = INV_ERROR_SM_TRANSITION;
    }

    if (result == INV_SUCCESS) {
        _stateDebug(MPL_LOGV
                    ("ML State transition from %s to %s\n",
                     inv_state_name(inv_params_obj.state),
                     inv_state_name(newState)));
        result = inv_run_state_callbacks(newState);
        if (INV_SUCCESS == result && newState == INV_STATE_SERIAL_CLOSED) {
            MLStateCloseCallbacks();
        }
        inv_params_obj.state = newState;
    }
    return result;
}

/**
 *  @internal
 *  @brief  To be moved in mlstates.c
**/
unsigned char inv_get_state(void)
{
    return (inv_params_obj.state);
}

/**
 * @internal
 * @brief   This registers a function to be called each time the state 
 *          changes. It may also be called when the FIFO Rate is changed.
 *          It will be called at the start of a state change before the
 *          state change has taken place. See Also inv_unregister_state_callback()
 *          The FIFO does not have to be on for this callback.
 * @param func Function to be called when a DMP interrupt occurs.
 * @return INV_SUCCESS or non-zero error code.
 */

inv_error_t inv_register_state_callback(state_change_callback_t callback)
{
    INVENSENSE_FUNC_START;
    int kk;
    inv_error_t result;

    result = inv_lock_mutex(sStateChangeCallbacks.mutex);
    if (INV_SUCCESS != result) {
        return result;
    }
    // Make sure we have not filled up our number of allowable callbacks
    if (sStateChangeCallbacks.numStateChangeCallbacks <
        MAX_STATE_CHANGE_PROCESSES) {
        // Make sure we haven't registered this function already
        for (kk = 0; kk < sStateChangeCallbacks.numStateChangeCallbacks; ++kk) {
            if (sStateChangeCallbacks.stateChangeCallbacks[kk] == callback) {
                result = INV_ERROR_INVALID_PARAMETER;
                break;
            }
        }

        if (INV_SUCCESS == result) {
            // Add new callback
            sStateChangeCallbacks.stateChangeCallbacks[sStateChangeCallbacks.
                                                       numStateChangeCallbacks]
                = callback;
            sStateChangeCallbacks.numStateChangeCallbacks++;
        }
    } else {
        result = INV_ERROR_MEMORY_EXAUSTED;
    }

    inv_unlock_mutex(sStateChangeCallbacks.mutex);
    return result;
}

/**
 * @internal
 * @brief   This unregisters a function to be called each time the state 
 *          changes. See Also inv_register_state_callback()
 *          The FIFO does not have to be on for this callback.
 * @return INV_SUCCESS or non-zero error code.
 */
inv_error_t inv_unregister_state_callback(state_change_callback_t callback)
{
    INVENSENSE_FUNC_START;
    int kk, jj;
    inv_error_t result;

    result = inv_lock_mutex(sStateChangeCallbacks.mutex);
    if (INV_SUCCESS != result) {
        return result;
    }
    // Make sure we haven't registered this function already
    result = INV_ERROR_INVALID_PARAMETER;
    for (kk = 0; kk < sStateChangeCallbacks.numStateChangeCallbacks; ++kk) {
        if (sStateChangeCallbacks.stateChangeCallbacks[kk] == callback) {
            for (jj = kk + 1;
                 jj < sStateChangeCallbacks.numStateChangeCallbacks; ++jj) {
                sStateChangeCallbacks.stateChangeCallbacks[jj - 1] =
                    sStateChangeCallbacks.stateChangeCallbacks[jj];
            }
            sStateChangeCallbacks.numStateChangeCallbacks--;
            result = INV_SUCCESS;
            break;
        }
    }

    inv_unlock_mutex(sStateChangeCallbacks.mutex);
    return result;
}

inv_error_t inv_run_state_callbacks(unsigned char newState)
{
    int kk;
    inv_error_t result;

    result = inv_lock_mutex(sStateChangeCallbacks.mutex);
    if (INV_SUCCESS != result) {
        MPL_LOGE("MLOsLockMutex returned %d\n", result);
        return result;
    }

    for (kk = 0; kk < sStateChangeCallbacks.numStateChangeCallbacks; ++kk) {
        if (sStateChangeCallbacks.stateChangeCallbacks[kk]) {
            result = sStateChangeCallbacks.stateChangeCallbacks[kk] (newState);
            if (INV_SUCCESS != result) {
                break;          // Can't return, must release mutex
            }
        }
    }

    inv_unlock_mutex(sStateChangeCallbacks.mutex);
    return result;
}
