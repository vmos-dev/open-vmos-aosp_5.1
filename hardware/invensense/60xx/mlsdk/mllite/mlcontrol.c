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
 * $Id: mlcontrol.c 5641 2011-06-14 02:10:02Z mcaramello $
 *
 *******************************************************************************/

/**
 *  @defgroup   CONTROL
 *  @brief      Motion Library - Control Engine.
 *              The Control Library processes gyroscopes, accelerometers, and 
 *              compasses to provide control signals that can be used in user 
 *              interfaces.
 *              These signals can be used to manipulate objects such as documents,
 *              images, cursors, menus, etc.
 *
 *  @{
 *      @file   mlcontrol.c
 *      @brief  The Control Library.
 *
 */

/* ------------------ */
/* - Include Files. - */
/* ------------------ */

#include "mltypes.h"
#include "mlinclude.h"
#include "mltypes.h"
#include "ml.h"
#include "mlos.h"
#include "mlsl.h"
#include "mldl.h"
#include "mlcontrol.h"
#include "dmpKey.h"
#include "mlstates.h"
#include "mlFIFO.h"
#include "string.h"

/* - Global Vars. - */
struct control_params cntrl_params = {
    {
     MLCTRL_SENSITIVITY_0_DEFAULT,
     MLCTRL_SENSITIVITY_1_DEFAULT,
     MLCTRL_SENSITIVITY_2_DEFAULT,
     MLCTRL_SENSITIVITY_3_DEFAULT}, // sensitivity
    MLCTRL_FUNCTIONS_DEFAULT,   // functions
    {
     MLCTRL_PARAMETER_ARRAY_0_DEFAULT,
     MLCTRL_PARAMETER_ARRAY_1_DEFAULT,
     MLCTRL_PARAMETER_ARRAY_2_DEFAULT,
     MLCTRL_PARAMETER_ARRAY_3_DEFAULT}, // parameterArray
    {
     MLCTRL_PARAMETER_AXIS_0_DEFAULT,
     MLCTRL_PARAMETER_AXIS_1_DEFAULT,
     MLCTRL_PARAMETER_AXIS_2_DEFAULT,
     MLCTRL_PARAMETER_AXIS_3_DEFAULT},  // parameterAxis
    {
     MLCTRL_GRID_THRESHOLD_0_DEFAULT,
     MLCTRL_GRID_THRESHOLD_1_DEFAULT,
     MLCTRL_GRID_THRESHOLD_2_DEFAULT,
     MLCTRL_GRID_THRESHOLD_3_DEFAULT},  // gridThreshold
    {
     MLCTRL_GRID_MAXIMUM_0_DEFAULT,
     MLCTRL_GRID_MAXIMUM_1_DEFAULT,
     MLCTRL_GRID_MAXIMUM_2_DEFAULT,
     MLCTRL_GRID_MAXIMUM_3_DEFAULT},    // gridMaximum
    MLCTRL_GRID_CALLBACK_DEFAULT    // gridCallback
};

/* - Extern Vars. - */
struct control_obj cntrl_obj;
extern const unsigned char *dmpConfig1;

/* -------------- */
/* - Functions. - */
/* -------------- */

/**
 *  @brief  inv_set_control_sensitivity is used to set the sensitivity for a control
 *          signal.
 *
 *  @pre    inv_dmp_open() Must be called with MLDmpDefaultOpen() or 
 *          inv_open_low_power_pedometer().
 *
 *  @param controlSignal    Indicates which control signal is being modified.
 *                          Must be one of:
 *                          - INV_CONTROL_1,
 *                          - INV_CONTROL_2,
 *                          - INV_CONTROL_3 or
 *                          - INV_CONTROL_4.
 *
 *  @param sensitivity      The sensitivity of the control signal.
 *
 *  @return error code
 */
inv_error_t inv_set_control_sensitivity(unsigned short controlSignal,
                                        long sensitivity)
{
    INVENSENSE_FUNC_START;
    unsigned char regs[2];
    long finalSens = 0;
    inv_error_t result;

    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    finalSens = sensitivity * 100;
    if (finalSens > 16384) {
        finalSens = 16384;
    }
    regs[0] = (unsigned char)(finalSens / 256);
    regs[1] = (unsigned char)(finalSens % 256);
    switch (controlSignal) {
    case INV_CONTROL_1:
        result = inv_set_mpu_memory(KEY_D_0_224, 2, regs);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
        cntrl_params.sensitivity[0] = (unsigned short)sensitivity;
        break;
    case INV_CONTROL_2:
        result = inv_set_mpu_memory(KEY_D_0_228, 2, regs);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
        cntrl_params.sensitivity[1] = (unsigned short)sensitivity;
        break;
    case INV_CONTROL_3:
        result = inv_set_mpu_memory(KEY_D_0_232, 2, regs);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
        cntrl_params.sensitivity[2] = (unsigned short)sensitivity;
        break;
    case INV_CONTROL_4:
        result = inv_set_mpu_memory(KEY_D_0_236, 2, regs);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
        cntrl_params.sensitivity[3] = (unsigned short)sensitivity;
        break;
    default:
        break;
    }
    if (finalSens != sensitivity * 100) {
        return INV_ERROR_INVALID_PARAMETER;
    } else {
        return INV_SUCCESS;
    }
}

/**
 *  @brief  inv_set_control_func allows the user to choose how the sensor data will
 *          be processed in order to provide a control parameter.
 *          inv_set_control_func allows the user to choose which control functions
 *          will be incorporated in the sensor data processing.
 *          The control functions are:
 *          - INV_GRID
 *          Indicates that the user will be controlling a system that
 *          has discrete steps, such as icons, menu entries, pixels, etc.
 *          - INV_SMOOTH
 *          Indicates that noise from unintentional motion should be filtered out.
 *          - INV_DEAD_ZONE
 *          Indicates that a dead zone should be used, below which sensor
 *          data is set to zero.
 *          - INV_HYSTERESIS
 *          Indicates that, when INV_GRID is selected, hysteresis should
 *          be used to prevent the control signal from switching rapidly across
 *          elements of the grid.
 *
 *  @pre    inv_dmp_open() Must be called with MLDmpDefaultOpen() or 
 *          inv_open_low_power_pedometer().
 *
 *  @param  function    Indicates what functions will be used.
 *                      Can be a bitwise OR of several values.
 *
 *  @return Zero if the command is successful; an ML error code otherwise.
 */
inv_error_t inv_set_control_func(unsigned short function)
{
    INVENSENSE_FUNC_START;
    unsigned char regs[8] = { DINA06, DINA26,
        DINA46, DINA66,
        DINA0E, DINA2E,
        DINA4E, DINA6E
    };
    unsigned char i;
    inv_error_t result;

    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if ((function & INV_SMOOTH) == 0) {
        for (i = 0; i < 8; i++) {
            regs[i] = DINA80 + 3;
        }
    }
    result = inv_set_mpu_memory(KEY_CFG_4, 8, regs);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    cntrl_params.functions = function;
    result = inv_set_dead_zone();

    return result;
}

/**
 *  @brief  inv_get_control_signal is used to get the current control signal with
 *          high precision.
 *          inv_get_control_signal is used to acquire the current data of a control signal.
 *          If INV_GRID is being used, inv_get_grid_number will probably be preferrable.
 *
 *  @param  controlSignal   Indicates which control signal is being queried.
 *          Must be one of:
 *          - INV_CONTROL_1,
 *          - INV_CONTROL_2,
 *          - INV_CONTROL_3 or
 *          - INV_CONTROL_4.
 *
 *  @param  reset   Indicates whether the control signal should be reset to zero.
 *                  Options are INV_RESET or INV_NO_RESET
 *  @param  data    A pointer to the current control signal data.
 *
 *  @return Zero if the command is successful; an ML error code otherwise.
 */
inv_error_t inv_get_control_signal(unsigned short controlSignal,
                                   unsigned short reset, long *data)
{
    INVENSENSE_FUNC_START;

    if (inv_get_state() != INV_STATE_DMP_STARTED)
        return INV_ERROR_SM_IMPROPER_STATE;

    switch (controlSignal) {
    case INV_CONTROL_1:
        *data = cntrl_obj.controlInt[0];
        if (reset == INV_RESET) {
            cntrl_obj.controlInt[0] = 0;
        }
        break;
    case INV_CONTROL_2:
        *data = cntrl_obj.controlInt[1];
        if (reset == INV_RESET) {
            cntrl_obj.controlInt[1] = 0;
        }
        break;
    case INV_CONTROL_3:
        *data = cntrl_obj.controlInt[2];
        if (reset == INV_RESET) {
            cntrl_obj.controlInt[2] = 0;
        }
        break;
    case INV_CONTROL_4:
        *data = cntrl_obj.controlInt[3];
        if (reset == INV_RESET) {
            cntrl_obj.controlInt[3] = 0;
        }
        break;
    default:
        break;
    }
    return INV_SUCCESS;
}

/**
 *  @brief  inv_get_grid_num is used to get the current grid location for a certain
 *          control signal.
 *          inv_get_grid_num is used to acquire the current grid location.
 *
 *  @pre    inv_dmp_open() Must be called with MLDmpDefaultOpen() or 
 *          inv_open_low_power_pedometer().
 *
 *  @param  controlSignal   Indicates which control signal is being queried.
 *          Must be one of:
 *          - INV_CONTROL_1,
 *          - INV_CONTROL_2,
 *          - INV_CONTROL_3 or
 *          - INV_CONTROL_4.
 *
 *  @param  reset   Indicates whether the control signal should be reset to zero.
 *                  Options are INV_RESET or INV_NO_RESET
 *  @param  data    A pointer to the current grid number.
 *
 *  @return Zero if the command is successful; an ML error code otherwise.
 */

inv_error_t inv_get_grid_num(unsigned short controlSignal, unsigned short reset,
                             long *data)
{
    INVENSENSE_FUNC_START;

    if (inv_get_state() != INV_STATE_DMP_STARTED)
        return INV_ERROR_SM_IMPROPER_STATE;

    switch (controlSignal) {
    case INV_CONTROL_1:
        *data = cntrl_obj.gridNum[0];
        if (reset == INV_RESET) {
            cntrl_obj.gridNum[0] = 0;
        }
        break;
    case INV_CONTROL_2:
        *data = cntrl_obj.gridNum[1];
        if (reset == INV_RESET) {
            cntrl_obj.gridNum[1] = 0;
        }
        break;
    case INV_CONTROL_3:
        *data = cntrl_obj.gridNum[2];
        if (reset == INV_RESET) {
            cntrl_obj.gridNum[2] = 0;
        }
        break;
    case INV_CONTROL_4:
        *data = cntrl_obj.gridNum[3];
        if (reset == INV_RESET) {
            cntrl_obj.gridNum[3] = 0;
        }
        break;
    default:
        break;
    }

    return INV_SUCCESS;
}

/**
 *  @brief  inv_set_grid_thresh is used to set the grid size for a control signal.
 *          inv_set_grid_thresh is used to adjust the size of the grid being controlled.
 *  @param  controlSignal   Indicates which control signal is being modified.
 *                          Must be one of:
 *                          - INV_CONTROL_1,
 *                          - INV_CONTROL_2,
 *                          - INV_CONTROL_3 and
 *                          - INV_CONTROL_4.
 *  @param  threshold       The threshold of the control signal at which the grid
 *                          number will be incremented or decremented.
 *  @return Zero if the command is successful; an ML error code otherwise.
 */

inv_error_t inv_set_grid_thresh(unsigned short controlSignal, long threshold)
{
    INVENSENSE_FUNC_START;

    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    switch (controlSignal) {
    case INV_CONTROL_1:
        cntrl_params.gridThreshold[0] = threshold;
        break;
    case INV_CONTROL_2:
        cntrl_params.gridThreshold[1] = threshold;
        break;
    case INV_CONTROL_3:
        cntrl_params.gridThreshold[2] = threshold;
        break;
    case INV_CONTROL_4:
        cntrl_params.gridThreshold[3] = threshold;
        break;
    default:
        return INV_ERROR_INVALID_PARAMETER;
        break;
    }

    return INV_SUCCESS;
}

/**
 *  @brief  inv_set_grid_max is used to set the maximum grid number for a control signal.
 *          inv_set_grid_max is used to adjust the maximum allowed grid number, above
 *          which the grid number will not be incremented.
 *          The minimum grid number is always zero.
 *
 *  @pre    inv_dmp_open() Must be called with MLDmpDefaultOpen() or 
 *          inv_open_low_power_pedometer().
 *
 *  @param controlSignal    Indicates which control signal is being modified.
 *                          Must be one of:
 *                          - INV_CONTROL_1,
 *                          - INV_CONTROL_2,
 *                          - INV_CONTROL_3 and
 *                          - INV_CONTROL_4.
 *
 *  @param  maximum         The maximum grid number for a control signal.
 *  @return Zero if the command is successful; an ML error code otherwise.
 */

inv_error_t inv_set_grid_max(unsigned short controlSignal, long maximum)
{
    INVENSENSE_FUNC_START;

    if (inv_get_state() != INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    switch (controlSignal) {
    case INV_CONTROL_1:
        cntrl_params.gridMaximum[0] = maximum;
        break;
    case INV_CONTROL_2:
        cntrl_params.gridMaximum[1] = maximum;
        break;
    case INV_CONTROL_3:
        cntrl_params.gridMaximum[2] = maximum;
        break;
    case INV_CONTROL_4:
        cntrl_params.gridMaximum[3] = maximum;
        break;
    default:
        return INV_ERROR_INVALID_PARAMETER;
        break;
    }

    return INV_SUCCESS;
}

/**
 *  @brief  GridCallback function pointer type, to be passed as argument of 
 *          inv_set_grid_callback.
 *
 *  @param  controlSignal   Indicates which control signal crossed a grid threshold.
 *                          Must be one of:
 *                          - INV_CONTROL_1,
 *                          - INV_CONTROL_2,
 *                          - INV_CONTROL_3 and
 *                          - INV_CONTROL_4.
 *
 *  @param  gridNumber  An array of four numbers representing the grid number for each
 *                      control signal.
 *  @param  gridChange  An array of four numbers representing the change in grid number
 *                      for each control signal.
**/
typedef void (*fpGridCb) (unsigned short controlSignal, long *gridNum,
                          long *gridChange);

/**
 *  @brief  inv_set_grid_callback is used to register a callback function that
 *          will trigger when the grid location changes.
 *          inv_set_grid_callback allows a user to define a callback function that will
 *          run when a control signal crosses a grid threshold.

 *  @pre    inv_dmp_open() Must be called with MLDmpDefaultOpen() or 
 *          inv_open_low_power_pedometer().  inv_dmp_start must <b>NOT</b> have 
 *          been called.
 *
 *  @param  func    A user defined callback function
 *  @return Zero if the command is successful; an ML error code otherwise.
**/
inv_error_t inv_set_grid_callback(fpGridCb func)
{
    INVENSENSE_FUNC_START;

    if (inv_get_state() != INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    cntrl_params.gridCallback = func;
    return INV_SUCCESS;
}

/**
 *  @brief  inv_set_control_data is used to assign physical parameters to control signals.
 *          inv_set_control_data allows flexibility in assigning physical parameters to
 *          control signals. For example, the user is allowed to use raw gyroscope data
 *          as an input to the control algorithm.
 *          Alternatively, angular velocity can be used, which combines gyroscopes and
 *          accelerometers to provide a more robust physical parameter. Finally, angular
 *          velocity in world coordinates can be used, providing a control signal in
 *          which pitch and yaw are provided relative to gravity.
 *
 *  @pre    inv_dmp_open() Must be called with MLDmpDefaultOpen() or 
 *          inv_open_low_power_pedometer().
 *
 *  @param  controlSignal   Indicates which control signal is being modified.
 *                          Must be one of:
 *                          - INV_CONTROL_1,
 *                          - INV_CONTROL_2,
 *                          - INV_CONTROL_3 or
 *                          - INV_CONTROL_4.
 *
 *  @param  parameterArray   Indicates which parameter array is being assigned to a
 *                          control signal. Must be one of:
 *                          - INV_GYROS,
 *                          - INV_ANGULAR_VELOCITY, or
 *
 *  @param  parameterAxis   Indicates which axis of the parameter array will be used.
 *                          Must be:
 *                          - INV_ROLL,
 *                          - INV_PITCH, or
 *                          - INV_YAW.
 */

inv_error_t inv_set_control_data(unsigned short controlSignal,
                                 unsigned short parameterArray,
                                 unsigned short parameterAxis)
{
    INVENSENSE_FUNC_START;
    unsigned char regs[2] = { DINA80 + 10, DINA20 };
    inv_error_t result;

    if (inv_get_state() != INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (parameterArray == INV_ANGULAR_VELOCITY) {
        regs[0] = DINA80 + 5;
        regs[1] = DINA00;
    }
    switch (controlSignal) {
    case INV_CONTROL_1:
        cntrl_params.parameterArray[0] = parameterArray;
        switch (parameterAxis) {
        case INV_PITCH:
            regs[1] += 0x02;
            cntrl_params.parameterAxis[0] = 0;
            break;
        case INV_ROLL:
            regs[1] = DINA22;
            cntrl_params.parameterAxis[0] = 1;
            break;
        case INV_YAW:
            regs[1] = DINA42;
            cntrl_params.parameterAxis[0] = 2;
            break;
        default:
            return INV_ERROR_INVALID_PARAMETER;
        }
        result = inv_set_mpu_memory(KEY_CFG_3, 2, regs);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
        break;
    case INV_CONTROL_2:
        cntrl_params.parameterArray[1] = parameterArray;
        switch (parameterAxis) {
        case INV_PITCH:
            regs[1] += DINA0E;
            cntrl_params.parameterAxis[1] = 0;
            break;
        case INV_ROLL:
            regs[1] += DINA2E;
            cntrl_params.parameterAxis[1] = 1;
            break;
        case INV_YAW:
            regs[1] += DINA4E;
            cntrl_params.parameterAxis[1] = 2;
            break;
        default:
            return INV_ERROR_INVALID_PARAMETER;
        }
        result = inv_set_mpu_memory(KEY_CFG_3B, 2, regs);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
        break;
    case INV_CONTROL_3:
        cntrl_params.parameterArray[2] = parameterArray;
        switch (parameterAxis) {
        case INV_PITCH:
            regs[1] += DINA0E;
            cntrl_params.parameterAxis[2] = 0;
            break;
        case INV_ROLL:
            regs[1] += DINA2E;
            cntrl_params.parameterAxis[2] = 1;
            break;
        case INV_YAW:
            regs[1] += DINA4E;
            cntrl_params.parameterAxis[2] = 2;
            break;
        default:
            return INV_ERROR_INVALID_PARAMETER;
        }
        result = inv_set_mpu_memory(KEY_CFG_3C, 2, regs);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
        break;
    case INV_CONTROL_4:
        cntrl_params.parameterArray[3] = parameterArray;
        switch (parameterAxis) {
        case INV_PITCH:
            regs[1] += DINA0E;
            cntrl_params.parameterAxis[3] = 0;
            break;
        case INV_ROLL:
            regs[1] += DINA2E;
            cntrl_params.parameterAxis[3] = 1;
            break;
        case INV_YAW:
            regs[1] += DINA4E;
            cntrl_params.parameterAxis[3] = 2;
            break;
        default:
            return INV_ERROR_INVALID_PARAMETER;
        }
        result = inv_set_mpu_memory(KEY_CFG_3D, 2, regs);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
        break;
    default:
        result = INV_ERROR_INVALID_PARAMETER;
        break;
    }
    return result;
}

/**
 *  @brief  inv_get_control_data is used to get the current control data.
 *
 *  @pre    inv_dmp_open() Must be called with MLDmpDefaultOpen() or 
 *          inv_open_low_power_pedometer().
 *
 *  @param  controlSignal   Indicates which control signal is being queried.
 *                          Must be one of:
 *                          - INV_CONTROL_1,
 *                          - INV_CONTROL_2,
 *                          - INV_CONTROL_3 or
 *                          - INV_CONTROL_4.
 *
 *  @param  gridNum     A pointer to pass gridNum info back to the user.
 *  @param  gridChange  A pointer to pass gridChange info back to the user.
 *
 *  @return Zero if the command is successful; an ML error code otherwise.
 */

inv_error_t inv_get_control_data(long *controlSignal, long *gridNum,
                                 long *gridChange)
{
    INVENSENSE_FUNC_START;
    int_fast8_t i = 0;

    if (inv_get_state() != INV_STATE_DMP_STARTED)
        return INV_ERROR_SM_IMPROPER_STATE;

    for (i = 0; i < 4; i++) {
        controlSignal[i] = cntrl_obj.controlInt[i];
        gridNum[i] = cntrl_obj.gridNum[i];
        gridChange[i] = cntrl_obj.gridChange[i];
    }
    return INV_SUCCESS;
}

/** 
 * @internal
 * @brief   Update the ML Control engine.  This function should be called 
 *          every time new data from the MPU becomes available.
 *          Control engine outputs are written to the cntrl_obj data 
 *          structure.
 * @return  INV_SUCCESS or an error code.
**/
inv_error_t inv_update_control(struct inv_obj_t * inv_obj)
{
    INVENSENSE_FUNC_START;
    unsigned char i;
    long gridTmp;
    long tmp;

    inv_get_cntrl_data(cntrl_obj.mlGridNumDMP);

    for (i = 0; i < 4; i++) {
        if (cntrl_params.functions & INV_GRID) {
            if (cntrl_params.functions & INV_HYSTERESIS) {
                cntrl_obj.mlGridNumDMP[i] += cntrl_obj.gridNumOffset[i];
            }
            cntrl_obj.mlGridNumDMP[i] =
                cntrl_obj.mlGridNumDMP[i] / 2 + 1073741824L;
            cntrl_obj.controlInt[i] =
                (cntrl_obj.mlGridNumDMP[i] %
                 (128 * cntrl_params.gridThreshold[i])) / 128;
            gridTmp =
                cntrl_obj.mlGridNumDMP[i] / (128 *
                                             cntrl_params.gridThreshold[i]);
            tmp = 1 + 16777216L / cntrl_params.gridThreshold[i];
            cntrl_obj.gridChange[i] = gridTmp - cntrl_obj.lastGridNum[i];
            if (cntrl_obj.gridChange[i] > tmp / 2) {
                cntrl_obj.gridChange[i] =
                    gridTmp - tmp - cntrl_obj.lastGridNum[i];
            } else if (cntrl_obj.gridChange[i] < -tmp / 2) {
                cntrl_obj.gridChange[i] =
                    gridTmp + tmp - cntrl_obj.lastGridNum[i];
            }
            if ((cntrl_params.functions & INV_HYSTERESIS)
                && (cntrl_obj.gridChange[i] != 0)) {
                if (cntrl_obj.gridChange[i] > 0) {
                    cntrl_obj.gridNumOffset[i] +=
                        128 * cntrl_params.gridThreshold[i];
                    cntrl_obj.controlInt[i] = cntrl_params.gridThreshold[i] / 2;
                }
                if (cntrl_obj.gridChange[i] < 0) {
                    cntrl_obj.gridNumOffset[i] -=
                        128 * cntrl_params.gridThreshold[i];
                    cntrl_obj.controlInt[i] = cntrl_params.gridThreshold[i] / 2;
                }
            }
            cntrl_obj.gridNum[i] += cntrl_obj.gridChange[i];
            if (cntrl_obj.gridNum[i] >= cntrl_params.gridMaximum[i]) {
                cntrl_obj.gridNum[i] = cntrl_params.gridMaximum[i];
                if (cntrl_obj.controlInt[i] >=
                    cntrl_params.gridThreshold[i] / 2) {
                    cntrl_obj.controlInt[i] = cntrl_params.gridThreshold[i] / 2;
                }
            } else if (cntrl_obj.gridNum[i] <= 0) {
                cntrl_obj.gridNum[i] = 0;
                if (cntrl_obj.controlInt[i] < cntrl_params.gridThreshold[i] / 2) {
                    cntrl_obj.controlInt[i] = cntrl_params.gridThreshold[i] / 2;
                }
            }
            cntrl_obj.lastGridNum[i] = gridTmp;
            if ((cntrl_params.gridCallback) && (cntrl_obj.gridChange[i] != 0)) {
                cntrl_params.gridCallback((INV_CONTROL_1 << i),
                                          cntrl_obj.gridNum,
                                          cntrl_obj.gridChange);
            }

        } else {
            cntrl_obj.controlInt[i] = cntrl_obj.mlGridNumDMP[i];
        }

    }

    return INV_SUCCESS;
}

/**
 * @brief Enables the INV_CONTROL engine.
 *
 * @note  This function replaces MLEnable(INV_CONTROL)
 *
 * @pre inv_dmp_open() with MLDmpDefaultOpen or MLDmpPedometerStandAlone() must 
 *      have been called.
 *
 * @return INV_SUCCESS or non-zero error code
 */
inv_error_t inv_enable_control(void)
{
    INVENSENSE_FUNC_START;

    if (inv_get_state() != INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    memset(&cntrl_obj, 0, sizeof(cntrl_obj));

    inv_register_fifo_rate_process(inv_update_control, INV_PRIORITY_CONTROL);   // fixme, someone needs to send control data to the fifo
    return INV_SUCCESS;
}

/**
 * @brief Disables the INV_CONTROL engine.
 *
 * @note  This function replaces MLDisable(INV_CONTROL)
 *
 * @pre inv_dmp_open() with MLDmpDefaultOpen or MLDmpPedometerStandAlone() must 
 *      have been called.
 *
 * @return INV_SUCCESS or non-zero error code
 */
inv_error_t inv_disable_control(void)
{
    INVENSENSE_FUNC_START;

    if (inv_get_state() < INV_STATE_DMP_STARTED)
        return INV_ERROR_SM_IMPROPER_STATE;

    return INV_SUCCESS;
}

/**
 * @}
 */
