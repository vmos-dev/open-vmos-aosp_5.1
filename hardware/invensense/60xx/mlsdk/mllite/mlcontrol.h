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
 * $RCSfile: mlcontrol.h,v $
 *
 * $Date: 2011-06-10 20:13:08 -0700 (Fri, 10 Jun 2011) $
 *
 * $Revision: 5629 $
 *
 *******************************************************************************/

/*******************************************************************************/
/** @defgroup INV_CONTROL

    The Control processes gyroscopes and accelerometers to provide control 
    signals that can be used in user interfaces to manipulate objects such as 
    documents, images, cursors, menus, etc.
    
    @{
        @file mlcontrol.h
        @brief Header file for the Control Library.
*/
/******************************************************************************/
#ifndef MLCONTROL_H
#define MLCONTROL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "mltypes.h"
#include "ml.h"
#ifdef INV_INCLUDE_LEGACY_HEADERS
#include "mlcontrol_legacy.h"
#endif

    /* ------------ */
    /* - Defines. - */
    /* ------------ */

    /*******************************************************************************/
    /* Control Signals.                                                            */
    /*******************************************************************************/

#define INV_CONTROL_1                    0x0001
#define INV_CONTROL_2                    0x0002
#define INV_CONTROL_3                    0x0004
#define INV_CONTROL_4                    0x0008

    /*******************************************************************************/
    /* Control Functions.                                                          */
    /*******************************************************************************/

#define INV_GRID                         0x0001 // Indicates that the user will be controlling a system that
    //   has discrete steps, such as icons, menu entries, pixels, etc.
#define INV_SMOOTH                       0x0002 // Indicates that noise from unintentional motion should be filtered out.
#define INV_DEAD_ZONE                    0x0004 // Indicates that a dead zone should be used, below which sensor data is set to zero.
#define INV_HYSTERESIS                   0x0008 // Indicates that, when INV_GRID is selected, hysteresis should be used to prevent
    //   the control signal from switching rapidly across elements of the grid.</dd>

    /*******************************************************************************/
    /* Integral reset options.                                                     */
    /*******************************************************************************/

#define INV_NO_RESET                     0x0000
#define INV_RESET                        0x0001

    /*******************************************************************************/
    /* Data select options.                                                        */
    /*******************************************************************************/

#define INV_CTRL_SIGNAL                  0x0000
#define INV_CTRL_GRID_NUM                0x0001

    /*******************************************************************************/
    /* Control Axis.                                                               */
    /*******************************************************************************/
#define INV_CTRL_PITCH                   0x0000 // (INV_PITCH >> 1)
#define INV_CTRL_ROLL                    0x0001 // (INV_ROLL  >> 1)
#define INV_CTRL_YAW                     0x0002 // (INV_YAW   >> 1)

    /*******************************************************************************/
    /* control_params structure default values.                                   */
    /*******************************************************************************/

#define MLCTRL_SENSITIVITY_0_DEFAULT           128
#define MLCTRL_SENSITIVITY_1_DEFAULT           128
#define MLCTRL_SENSITIVITY_2_DEFAULT           128
#define MLCTRL_SENSITIVITY_3_DEFAULT           128
#define MLCTRL_FUNCTIONS_DEFAULT                 0
#define MLCTRL_CONTROL_SIGNALS_DEFAULT           0
#define MLCTRL_PARAMETER_ARRAY_0_DEFAULT         0
#define MLCTRL_PARAMETER_ARRAY_1_DEFAULT         0
#define MLCTRL_PARAMETER_ARRAY_2_DEFAULT         0
#define MLCTRL_PARAMETER_ARRAY_3_DEFAULT         0
#define MLCTRL_PARAMETER_AXIS_0_DEFAULT          0
#define MLCTRL_PARAMETER_AXIS_1_DEFAULT          0
#define MLCTRL_PARAMETER_AXIS_2_DEFAULT          0
#define MLCTRL_PARAMETER_AXIS_3_DEFAULT          0
#define MLCTRL_GRID_THRESHOLD_0_DEFAULT          1
#define MLCTRL_GRID_THRESHOLD_1_DEFAULT          1
#define MLCTRL_GRID_THRESHOLD_2_DEFAULT          1
#define MLCTRL_GRID_THRESHOLD_3_DEFAULT          1
#define MLCTRL_GRID_MAXIMUM_0_DEFAULT            0
#define MLCTRL_GRID_MAXIMUM_1_DEFAULT            0
#define MLCTRL_GRID_MAXIMUM_2_DEFAULT            0
#define MLCTRL_GRID_MAXIMUM_3_DEFAULT            0
#define MLCTRL_GRID_CALLBACK_DEFAULT             0

    /* --------------- */
    /* - Structures. - */
    /* --------------- */

    /**************************************************************************/
    /* Control Parameters Structure.                                          */
    /**************************************************************************/

    struct control_params {
        // Sensitivity of control signal 1, 2, 3, and 4.
        unsigned short sensitivity[4];
        // Indicates what functions will be used. Can be a bitwise OR of INV_GRID,
        // ML_SMOOT, INV_DEAD_ZONE, and INV_HYSTERISIS.
        unsigned short functions;
        // Indicates which parameter array is being assigned to a control signal.
        // Must be one of INV_GYROS, INV_ANGULAR_VELOCITY, or
        // INV_ANGULAR_VELOCITY_WORLD.
        unsigned short parameterArray[4];
        // Indicates which axis of the parameter array will be used. Must be
        // INV_ROLL, INV_PITCH, or INV_YAW.
        unsigned short parameterAxis[4];
        // Threshold of the control signal at which the grid number will be
        // incremented or decremented.
        long gridThreshold[4];
        // Maximum grid number for the control signal.
        long gridMaximum[4];
        // User defined callback that will trigger when the grid location changes.
        void (*gridCallback) (
                                 // Indicates which control signal crossed a grid threshold. Must be
                                 // one of INV_CONTROL_1, INV_CONTROL_2, INV_CONTROL_3 or INV_CONTROL_4.
                                 unsigned short controlSignal,
                                 // An array of four numbers representing the grid number for each
                                 // control signal.
                                 long *gridNum,
                                 // An array of four numbers representing the change in grid number
                                 // for each control signal.
                                 long *gridChange);
    };

    struct control_obj {

        long gridNum[4];        // Current grid number for each control signal.
        long controlInt[4];     // Current data for each control signal.
        long lastGridNum[4];    // Previous grid number
        unsigned char controlDir[4];    // Direction of control signal
        long gridChange[4];     // Change in grid number

        long mlGridNumDMP[4];
        long gridNumOffset[4];
        long prevDMPGridNum[4];

    };

    /* --------------------- */
    /* - Function p-types. - */
    /* --------------------- */

    /**************************************************************************/
    /* ML Control Functions.                                                  */
    /**************************************************************************/

    unsigned short inv_get_control_params(struct control_params *params);
    unsigned short inv_set_control_params(struct control_params *params);

    /*API for handling control signals */
    inv_error_t inv_set_control_sensitivity(unsigned short controlSignal,
                                            long sensitivity);
    inv_error_t inv_set_control_func(unsigned short function);
    inv_error_t inv_get_control_signal(unsigned short controlSignal,
                                       unsigned short reset, long *data);
    inv_error_t inv_get_grid_num(unsigned short controlSignal,
                                 unsigned short reset, long *data);
    inv_error_t inv_set_grid_thresh(unsigned short controlSignal,
                                    long threshold);
    inv_error_t inv_set_grid_max(unsigned short controlSignal, long maximum);
    inv_error_t
        inv_set_grid_callback(void (*func)
                              (unsigned short controlSignal, long *gridNum,
                               long *gridChange));
    inv_error_t inv_set_control_data(unsigned short controlSignal,
                                     unsigned short parameterArray,
                                     unsigned short parameterNum);
    inv_error_t inv_get_control_data(long *controlSignal, long *gridNum,
                                     long *gridChange);
    inv_error_t inv_update_control(struct inv_obj_t *inv_obj);
    inv_error_t inv_enable_control(void);
    inv_error_t inv_disable_control(void);

#ifdef __cplusplus
}
#endif
#endif                          /* MLCONTROL_H */
