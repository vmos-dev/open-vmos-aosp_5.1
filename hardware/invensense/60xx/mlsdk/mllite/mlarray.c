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
/******************************************************************************
 *
 * $Id: mlarray.c 5085 2011-04-08 22:25:14Z phickey $
 *
 *****************************************************************************/

/**
 *  @defgroup ML
 *  @{
 *      @file   mlarray.c
 *      @brief  APIs to read different data sets from FIFO.
 */

/* ------------------ */
/* - Include Files. - */
/* ------------------ */
#include "ml.h"
#include "mltypes.h"
#include "mlinclude.h"
#include "mlMathFunc.h"
#include "mlmath.h"
#include "mlstates.h"
#include "mlFIFO.h"
#include "mlsupervisor.h"
#include "mldl.h"
#include "dmpKey.h"
#include "compass.h"

/**
 *  @brief  inv_get_gyro is used to get the most recent gyroscope measurement.
 *          The argument array elements are ordered X,Y,Z.
 *          The values are scaled at 1 dps = 2^16 LSBs.
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 3 cells long </b>.
 *
 *  @return Zero if the command is successful; an ML error code otherwise.
 */

 /* inv_get_gyro implemented in mlFIFO.c */

/**
 *  @brief  inv_get_accel is used to get the most recent
 *          accelerometer measurement.
 *          The argument array elements are ordered X,Y,Z.
 *          The values are scaled in units of g (gravity),
 *          where 1 g = 2^16 LSBs.
 *
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 3 cells long </b>.
 *
 *  @return Zero if the command is successful; an ML error code otherwise.
 */
 /* inv_get_accel implemented in mlFIFO.c */

/**
 *  @brief  inv_get_temperature is used to get the most recent
 *          temperature measurement.
 *          The argument array should only have one element.
 *          The value is in units of deg C (degrees Celsius), where
 *          2^16 LSBs = 1 deg C.
 *
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to the data to be passed back to the user.
 *
 *  @return Zero if the command is successful; an ML error code otherwise.
 */
 /* inv_get_temperature implemented in mlFIFO.c */

/**
 *  @brief  inv_get_rot_mat is used to get the rotation matrix
 *          representation of the current sensor fusion solution.
 *          The array format will be R11, R12, R13, R21, R22, R23, R31, R32,
 *          R33, representing the matrix:
 *          <center>R11 R12 R13</center>
 *          <center>R21 R22 R23</center>
 *          <center>R31 R32 R33</center>
 *          Values are scaled, where 1.0 = 2^30 LSBs.
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 9 cells long</b>.
 *
 *  @return Zero if the command is successful; an ML error code otherwise.
 */
inv_error_t inv_get_rot_mat(long *data)
{
    inv_error_t result = INV_SUCCESS;
    long qdata[4];
    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    inv_get_quaternion(qdata);
    inv_quaternion_to_rotation(qdata, data);

    return result;
}

/**
 *  @brief  inv_get_quaternion is used to get the quaternion representation
 *          of the current sensor fusion solution.
 *          The values are scaled where 1.0 = 2^30 LSBs.
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 4 cells long </b>.
 *
 *  @return INV_SUCCESS if the command is successful; an ML error code otherwise.
 */
 /* inv_get_quaternion implemented in mlFIFO.c */

/**
 *  @brief  inv_get_linear_accel is used to get an estimate of linear
 *          acceleration, based on the most recent accelerometer measurement
 *          and sensor fusion solution.
 *          The values are scaled where 1 g (gravity) = 2^16 LSBs.
 *
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 3 cells long </b>.
 *
 *  @return Zero if the command is successful; an ML error code otherwise.
 */
 /* inv_get_linear_accel implemented in mlFIFO.c */

/**
 *  @brief  inv_get_linear_accel_in_world is used to get an estimate of
 *          linear acceleration, in the world frame,  based on the most
 *          recent accelerometer measurement and sensor fusion solution.
 *          The values are scaled where 1 g (gravity) = 2^16 LSBs.
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 3 cells long</b>.
 *
 *  @return Zero if the command is successful; an ML error code otherwise.
 */
 /* inv_get_linear_accel_in_world implemented in mlFIFO.c */

/**
 *  @brief  inv_get_gravity is used to get an estimate of the body frame
 *          gravity vector, based on the most recent sensor fusion solution.
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 3 cells long</b>.
 *
 *  @return Zero if the command is successful; an ML error code otherwise.
 */
 /* inv_get_gravity implemented in mlFIFO.c */

/**
 *  @internal
 *  @brief  inv_get_angular_velocity is used to get an estimate of the body
 *          frame angular velocity, which is computed from the current and
 *          previous sensor fusion solutions.
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 3 cells long </b>.
 *
 *  @return Zero if the command is successful; an ML error code otherwise.
 */
inv_error_t inv_get_angular_velocity(long *data)
{

    return INV_ERROR_FEATURE_NOT_IMPLEMENTED;
    /* not implemented. old (invalid) implementation:
       if ( inv_get_state() < INV_STATE_DMP_OPENED )
       return INV_ERROR_SM_IMPROPER_STATE;

       if (NULL == data) {
       return INV_ERROR_INVALID_PARAMETER;
       }
       data[0] = inv_obj.ang_v_body[0];
       data[1] = inv_obj.ang_v_body[1];
       data[2] = inv_obj.ang_v_body[2];

       return result;
     */
}

/**
 *  @brief  inv_get_euler_angles is used to get the Euler angle representation
 *          of the current sensor fusion solution.
 *          Euler angles may follow various conventions. This function is equivelant
 *          to inv_get_euler_angles_x, refer to inv_get_euler_angles_x for more
 *          information.
 *
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 3 cells long </b>.
 *
 *  @return Zero if the command is successful; an ML error code otherwise.
 */
inv_error_t inv_get_euler_angles(long *data)
{
    return inv_get_euler_angles_x(data);
}

/**
 *  @brief  inv_get_euler_angles_x is used to get the Euler angle representation
 *          of the current sensor fusion solution.
 *          Euler angles are returned according to the X convention.
 *          This is typically the convention used for mobile devices where the X
 *          axis is the width of the screen, Y axis is the height, and Z the
 *          depth. In this case roll is defined as the rotation around the X
 *          axis of the device.
 *          <TABLE>
 *          <TR><TD>Element </TD><TD><b>Euler angle</b></TD><TD><b>Rotation about </b></TD></TR>
 *          <TR><TD> 0      </TD><TD>Roll              </TD><TD>X axis                </TD></TR>
 *          <TR><TD> 1      </TD><TD>Pitch             </TD><TD>Y axis                </TD></TR>
 *          <TR><TD> 2      </TD><TD>Yaw               </TD><TD>Z axis                </TD></TR>
 *          </TABLE>
 *
 *          Values are scaled where 1.0 = 2^16.
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 3 cells long </b>.
 *
 *  @return Zero if the command is successful; an ML error code otherwise.
 */
inv_error_t inv_get_euler_angles_x(long *data)
{
    inv_error_t result = INV_SUCCESS;
    float rotMatrix[9];
    float tmp;
    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    result = inv_get_rot_mat_float(rotMatrix);
    tmp = rotMatrix[6];
    if (tmp > 1.0f) {
        tmp = 1.0f;
    }
    if (tmp < -1.0f) {
        tmp = -1.0f;
    }
    data[0] =
        (long)((float)
               (atan2f(rotMatrix[7], rotMatrix[8]) * 57.29577951308) *
               65536L);
    data[1] = (long)((float)((double)asin(tmp) * 57.29577951308) * 65536L);
    data[2] =
        (long)((float)
               (atan2f(rotMatrix[3], rotMatrix[0]) * 57.29577951308) *
               65536L);
    return result;
}

/**
 *  @brief  inv_get_euler_angles_y is used to get the Euler angle representation
 *          of the current sensor fusion solution.
 *          Euler angles are returned according to the Y convention.
 *          This convention is typically used in augmented reality applications,
 *          where roll is defined as the rotation around the axis along the
 *          height of the screen of a mobile device, namely the Y axis.
 *          <TABLE>
 *          <TR><TD>Element </TD><TD><b>Euler angle</b></TD><TD><b>Rotation about </b></TD></TR>
 *          <TR><TD> 0      </TD><TD>Roll              </TD><TD>Y axis                </TD></TR>
 *          <TR><TD> 1      </TD><TD>Pitch             </TD><TD>X axis                </TD></TR>
 *          <TR><TD> 2      </TD><TD>Yaw               </TD><TD>Z axis                </TD></TR>
 *          </TABLE>
 *
 *          Values are scaled where 1.0 = 2^16.
 *
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 3 cells long</b>.
 *
 *  @return Zero if the command is successful; an ML error code otherwise.
 */
inv_error_t inv_get_euler_angles_y(long *data)
{
    inv_error_t result = INV_SUCCESS;
    float rotMatrix[9];
    float tmp;
    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    result = inv_get_rot_mat_float(rotMatrix);
    tmp = rotMatrix[7];
    if (tmp > 1.0f) {
        tmp = 1.0f;
    }
    if (tmp < -1.0f) {
        tmp = -1.0f;
    }
    data[0] =
        (long)((float)
               (atan2f(rotMatrix[8], rotMatrix[6]) * 57.29577951308f) *
               65536L);
    data[1] = (long)((float)((double)asin(tmp) * 57.29577951308) * 65536L);
    data[2] =
        (long)((float)
               (atan2f(rotMatrix[4], rotMatrix[1]) * 57.29577951308f) *
               65536L);
    return result;
}

/**  @brief  inv_get_euler_angles_z is used to get the Euler angle representation
 *          of the current sensor fusion solution.
 *          This convention is mostly used in application involving the use
 *          of a camera, typically placed on the back of a mobile device, that
 *          is along the Z axis.  In this convention roll is defined as the
 *          rotation around the Z axis.
 *          Euler angles are returned according to the Y convention.
 *          <TABLE>
 *          <TR><TD>Element </TD><TD><b>Euler angle</b></TD><TD><b>Rotation about </b></TD></TR>
 *          <TR><TD> 0      </TD><TD>Roll              </TD><TD>Z axis                </TD></TR>
 *          <TR><TD> 1      </TD><TD>Pitch             </TD><TD>X axis                </TD></TR>
 *          <TR><TD> 2      </TD><TD>Yaw               </TD><TD>Y axis                </TD></TR>
 *          </TABLE>
 *
 *          Values are scaled where 1.0 = 2^16.
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 3 cells long</b>.
 *
 *  @return INV_SUCCESS if the command is successful; an error code otherwise.
 */

inv_error_t inv_get_euler_angles_z(long *data)
{
    inv_error_t result = INV_SUCCESS;
    float rotMatrix[9];
    float tmp;
    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    result = inv_get_rot_mat_float(rotMatrix);
    tmp = rotMatrix[8];
    if (tmp > 1.0f) {
        tmp = 1.0f;
    }
    if (tmp < -1.0f) {
        tmp = -1.0f;
    }
    data[0] =
        (long)((float)
               (atan2f(rotMatrix[6], rotMatrix[7]) * 57.29577951308) *
               65536L);
    data[1] = (long)((float)((double)asin(tmp) * 57.29577951308) * 65536L);
    data[2] =
        (long)((float)
               (atan2f(rotMatrix[5], rotMatrix[2]) * 57.29577951308) *
               65536L);
    return result;
}

/**
 *  @brief  inv_get_gyro_temp_slope is used to get is used to get the temperature
 *          compensation algorithm's estimate of the gyroscope bias temperature
 *          coefficient.
 *          The argument array elements are ordered X,Y,Z.
 *          Values are in units of dps per deg C (degrees per second per degree
 *          Celcius). Values are scaled so that 1 dps per deg C = 2^16 LSBs.
 *          Please refer to the provided "9-Axis Sensor Fusion
 *          Application Note" document.
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 3 cells long </b>.
 *
 *  @return Zero if the command is successful; an ML error code otherwise.
 */
inv_error_t inv_get_gyro_temp_slope(long *data)
{
    inv_error_t result = INV_SUCCESS;
    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }
    if (inv_params_obj.bias_mode & INV_LEARN_BIAS_FROM_TEMPERATURE) {
        data[0] = (long)(inv_obj.x_gyro_coef[1] * 65536.0f);
        data[1] = (long)(inv_obj.y_gyro_coef[1] * 65536.0f);
        data[2] = (long)(inv_obj.z_gyro_coef[1] * 65536.0f);
    } else {
        data[0] = inv_obj.temp_slope[0];
        data[1] = inv_obj.temp_slope[1];
        data[2] = inv_obj.temp_slope[2];
    }
    return result;
}

/**
 *  @brief  inv_get_gyro_bias is used to get the gyroscope biases.
 *          The argument array elements are ordered X,Y,Z.
 *          The values are scaled such that 1 dps = 2^16 LSBs.
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 3 cells long</b>.
 *
 *  @return Zero if the command is successful; an ML error code otherwise.
 */
inv_error_t inv_get_gyro_bias(long *data)
{
    inv_error_t result = INV_SUCCESS;
    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }
    data[0] = inv_obj.gyro_bias[0];
    data[1] = inv_obj.gyro_bias[1];
    data[2] = inv_obj.gyro_bias[2];

    return result;
}

/**
 *  @brief  inv_get_accel_bias is used to get the accelerometer baises.
 *          The argument array elements are ordered X,Y,Z.
 *          The values are scaled such that 1 g (gravity) = 2^16 LSBs.
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 3 cells long </b>.
 *
 *  @return Zero if the command is successful; an ML error code otherwise.
 */
inv_error_t inv_get_accel_bias(long *data)
{
    inv_error_t result = INV_SUCCESS;
    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }
    data[0] = inv_obj.accel_bias[0];
    data[1] = inv_obj.accel_bias[1];
    data[2] = inv_obj.accel_bias[2];

    return result;
}

/**
 *  @cond MPL
 *  @brief  inv_get_mag_bias is used to get Magnetometer Bias
 *
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 3 cells long </b>.
 *
 *  @return Zero if the command is successful; an ML error code otherwise.
 *  @endcond
 */
inv_error_t inv_get_mag_bias(long *data)
{
    inv_error_t result = INV_SUCCESS;
    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }
    data[0] =
        inv_obj.compass_bias[0] +
        (long)((long long)inv_obj.init_compass_bias[0] * inv_obj.compass_sens /
               16384);
    data[1] =
        inv_obj.compass_bias[1] +
        (long)((long long)inv_obj.init_compass_bias[1] * inv_obj.compass_sens /
               16384);
    data[2] =
        inv_obj.compass_bias[2] +
        (long)((long long)inv_obj.init_compass_bias[2] * inv_obj.compass_sens /
               16384);

    return result;
}

/**
 *  @brief  inv_get_gyro_and_accel_sensor is used to get the most recent set of all sensor data.
 *          The argument array elements are ordered gyroscope X,Y, and Z,
 *          accelerometer X, Y, and Z, and magnetometer X,Y, and Z.
 *          \if UMPL The magnetometer elements are not populated in UMPL. \endif
 *          The gyroscope and accelerometer data is not scaled or offset, it is
 *          copied directly from the sensor registers.
 *          In the case of accelerometers with 8-bit output resolution, the data
 *          is scaled up to match the 2^14 = 1 g typical represntation of +/- 2 g
 *          full scale range
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 9 cells long</b>.
 *
 *  @return Zero if the command is successful; an ML error code otherwise.
 */
 /* inv_get_gyro_and_accel_sensor implemented in mlFIFO.c */

/**
 *  @cond MPL
 *  @brief  inv_get_mag_raw_data is used to get Raw magnetometer data.
 *
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 3 cells long </b>.
 *
 *  @return Zero if the command is successful; an ML error code otherwise.
 *  @endcond
 */
inv_error_t inv_get_mag_raw_data(long *data)
{
    inv_error_t result = INV_SUCCESS;
    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    data[0] = inv_obj.compass_sensor_data[0];
    data[1] = inv_obj.compass_sensor_data[1];
    data[2] = inv_obj.compass_sensor_data[2];

    return result;
}

/**
 *  @cond MPL
 *  @brief  inv_get_magnetometer is used to get magnetometer data.
 *
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 3 cells long</b>.
 *
 *  @return Zero if the command is successful; an ML error code otherwise.
 *  @endcond
 */
inv_error_t inv_get_magnetometer(long *data)
{
    inv_error_t result = INV_SUCCESS;
    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    data[0] = inv_obj.compass_sensor_data[0] + inv_obj.init_compass_bias[0];
    data[1] = inv_obj.compass_sensor_data[1] + inv_obj.init_compass_bias[1];
    data[2] = inv_obj.compass_sensor_data[2] + inv_obj.init_compass_bias[2];

    return result;
}

/**
 *  @cond MPL
 *  @brief  inv_get_pressure is used to get Pressure data.
 *
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to data to be passed back to the user.
 *
 *  @return Zero if the command is successful; an ML error code otherwise.
 *  @endcond
 */
inv_error_t inv_get_pressure(long *data)
{
    inv_error_t result = INV_SUCCESS;
    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    data[0] = inv_obj.pressure;

    return result;
}

/**
 *  @cond MPL
 *  @brief  inv_get_heading is used to get heading from Rotation Matrix.
 *
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to data to be passed back to the user.
 *
 *  @return Zero if the command is successful; an ML error code otherwise.
 *  @endcond
 */

inv_error_t inv_get_heading(long *data)
{
    inv_error_t result = INV_SUCCESS;
    float rotMatrix[9];
    float tmp;
    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }
    result = inv_get_rot_mat_float(rotMatrix);
    if ((rotMatrix[7] < 0.707) && (rotMatrix[7] > -0.707)) {
        tmp =
            (float)(atan2f(rotMatrix[4], rotMatrix[1]) * 57.29577951308 -
                    90.0f);
    } else {
        tmp =
            (float)(atan2f(rotMatrix[5], rotMatrix[2]) * 57.29577951308 +
                    90.0f);
    }
    if (tmp < 0) {
        tmp += 360.0f;
    }
    data[0] = (long)((360 - tmp) * 65536.0f);

    return result;
}

/**
 *  @brief  inv_get_gyro_cal_matrix is used to get the gyroscope
 *          calibration matrix. The gyroscope calibration matrix defines the relationship
 *          between the gyroscope sensor axes and the sensor fusion solution axes.
 *          Calibration matrix data members will have a value of 1, 0, or -1.
 *          The matrix has members
 *          <center>C11 C12 C13</center>
 *          <center>C21 C22 C23</center>
 *          <center>C31 C32 C33</center>
 *          The argument array elements are ordered C11, C12, C13, C21, C22, C23, C31, C32, C33.
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 9 cells long</b>.
 *
 *  @return Zero if the command is successful; an ML error code otherwise.
 */
inv_error_t inv_get_gyro_cal_matrix(long *data)
{
    inv_error_t result = INV_SUCCESS;
    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    data[0] = inv_obj.gyro_cal[0];
    data[1] = inv_obj.gyro_cal[1];
    data[2] = inv_obj.gyro_cal[2];
    data[3] = inv_obj.gyro_cal[3];
    data[4] = inv_obj.gyro_cal[4];
    data[5] = inv_obj.gyro_cal[5];
    data[6] = inv_obj.gyro_cal[6];
    data[7] = inv_obj.gyro_cal[7];
    data[8] = inv_obj.gyro_cal[8];

    return result;
}

/**
 *  @brief  inv_get_accel_cal_matrix is used to get the accelerometer
 *          calibration matrix.
 *          Calibration matrix data members will have a value of 1, 0, or -1.
 *          The matrix has members
 *          <center>C11 C12 C13</center>
 *          <center>C21 C22 C23</center>
 *          <center>C31 C32 C33</center>
 *          The argument array elements are ordered C11, C12, C13, C21, C22, C23, C31, C32, C33.
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 9 cells long</b>.
 *
 *  @return Zero if the command is successful; an ML error code otherwise.
 */
inv_error_t inv_get_accel_cal_matrix(long *data)
{
    inv_error_t result = INV_SUCCESS;
    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    data[0] = inv_obj.accel_cal[0];
    data[1] = inv_obj.accel_cal[1];
    data[2] = inv_obj.accel_cal[2];
    data[3] = inv_obj.accel_cal[3];
    data[4] = inv_obj.accel_cal[4];
    data[5] = inv_obj.accel_cal[5];
    data[6] = inv_obj.accel_cal[6];
    data[7] = inv_obj.accel_cal[7];
    data[8] = inv_obj.accel_cal[8];

    return result;
}

/**
 *  @cond MPL
 *  @brief  inv_get_mag_cal_matrix is used to get magnetometer calibration matrix.
 *
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 9 cells long at least</b>.
 *
 *  @return Zero if the command is successful; an ML error code otherwise.
 *  @endcond
 */
inv_error_t inv_get_mag_cal_matrix(long *data)
{
    inv_error_t result = INV_SUCCESS;
    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    data[0] = inv_obj.compass_cal[0];
    data[1] = inv_obj.compass_cal[1];
    data[2] = inv_obj.compass_cal[2];
    data[3] = inv_obj.compass_cal[3];
    data[4] = inv_obj.compass_cal[4];
    data[5] = inv_obj.compass_cal[5];
    data[6] = inv_obj.compass_cal[6];
    data[7] = inv_obj.compass_cal[7];
    data[8] = inv_obj.compass_cal[8];

    return result;
}

/**
 *  @cond MPL
 *  @brief  inv_get_mag_bias_error is used to get magnetometer Bias error.
 *
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 3 cells long at least</b>.
 *
 *  @return Zero if the command is successful; an ML error code otherwise.
 *  @endcond
 */
inv_error_t inv_get_mag_bias_error(long *data)
{
    inv_error_t result = INV_SUCCESS;
    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }
    if (inv_obj.large_field == 0) {
        data[0] = inv_obj.compass_bias_error[0];
        data[1] = inv_obj.compass_bias_error[1];
        data[2] = inv_obj.compass_bias_error[2];
    } else {
        data[0] = P_INIT;
        data[1] = P_INIT;
        data[2] = P_INIT;
    }

    return result;
}

/**
 *  @cond MPL
 *  @brief  inv_get_mag_scale is used to get magnetometer scale.
 *
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 3 cells long at least</b>.
 *
 *  @return Zero if the command is successful; an ML error code otherwise.
 *  @endcond
 */
inv_error_t inv_get_mag_scale(long *data)
{
    inv_error_t result = INV_SUCCESS;
    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    data[0] = inv_obj.compass_scale[0];
    data[1] = inv_obj.compass_scale[1];
    data[2] = inv_obj.compass_scale[2];

    return result;
}

/**
 *  @cond MPL
 *  @brief  inv_get_local_field is used to get local magnetic field data.
 *
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 3 cells long at least</b>.
 *
 *  @return Zero if the command is successful; an ML error code otherwise.
 *  @endcond
 */
inv_error_t inv_get_local_field(long *data)
{
    inv_error_t result = INV_SUCCESS;
    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    data[0] = inv_obj.local_field[0];
    data[1] = inv_obj.local_field[1];
    data[2] = inv_obj.local_field[2];

    return result;
}

/**
 *  @cond MPL
 *  @brief  inv_get_relative_quaternion is used to get relative quaternion.
 *
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 4 cells long at least</b>.
 *
 *  @return Zero if the command is successful; an ML error code otherwise.
 *  @endcond
 */
/* inv_get_relative_quaternion implemented in mlFIFO.c */

/**
 *  @brief  inv_get_gyro_float is used to get the most recent gyroscope measurement.
 *          The argument array elements are ordered X,Y,Z.
 *          The values are in units of dps (degrees per second).
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 3 cells long</b>.
 *
 *  @return INV_SUCCESS if the command is successful; an error code otherwise.
 */
inv_error_t inv_get_gyro_float(float *data)
{
    INVENSENSE_FUNC_START;

    inv_error_t result = INV_SUCCESS;
    long ldata[3];

    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }
    result = inv_get_gyro(ldata);
    data[0] = (float)ldata[0] / 65536.0f;
    data[1] = (float)ldata[1] / 65536.0f;
    data[2] = (float)ldata[2] / 65536.0f;

    return result;
}

/**
 *  @internal
 *  @brief  inv_get_angular_velocity_float is used to get an array of three data points representing the angular
 *          velocity as derived from <b>both</b> gyroscopes and accelerometers.
 *          This requires that ML_SENSOR_FUSION be enabled, to fuse data from
 *          the gyroscope and accelerometer device, appropriately scaled and
 *          oriented according to the respective mounting matrices.
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 3 cells long</b>.
 *
 *  @return INV_SUCCESS if the command is successful; an error code otherwise.
 */
inv_error_t inv_get_angular_velocity_float(float *data)
{
    return INV_ERROR_FEATURE_NOT_IMPLEMENTED;
    /* not implemented. old (invalid) implementation:
       return inv_get_gyro_float(data);
     */
}

/**
 *  @brief  inv_get_accel_float is used to get the most recent accelerometer measurement.
 *          The argument array elements are ordered X,Y,Z.
 *          The values are in units of g (gravity).
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 3 cells long</b>.
 *
 *  @return INV_SUCCESS if the command is successful; an error code otherwise.
 */
 /* inv_get_accel_float implemented in mlFIFO.c */

/**
 *  @brief  inv_get_temperature_float is used to get the most recent
 *          temperature measurement.
 *          The argument array should only have one element.
 *          The value is in units of deg C (degrees Celsius).
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to data to be passed back to the user.
 *
 *  @return Zero if the command is successful; an ML error code otherwise.
 */
inv_error_t inv_get_temperature_float(float *data)
{
    INVENSENSE_FUNC_START;

    inv_error_t result = INV_SUCCESS;
    long ldata[1];

    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    result = inv_get_temperature(ldata);
    data[0] = (float)ldata[0] / 65536.0f;

    return result;
}

/**
 *  @brief  inv_get_rot_mat_float is used to get an array of nine data points representing the rotation
 *          matrix generated from all available sensors.
 *          The array format will be R11, R12, R13, R21, R22, R23, R31, R32,
 *          R33, representing the matrix:
 *          <center>R11 R12 R13</center>
 *          <center>R21 R22 R23</center>
 *          <center>R31 R32 R33</center>
 *          <b>Please refer to the "9-Axis Sensor Fusion Application Note" document,
 *          section 7 "Sensor Fusion Output", for details regarding rotation
 *          matrix output</b>.
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 9 cells long at least</b>.
 *
 *  @return INV_SUCCESS if the command is successful; an error code otherwise.
 */
inv_error_t inv_get_rot_mat_float(float *data)
{
    INVENSENSE_FUNC_START;

    inv_error_t result = INV_SUCCESS;

    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }
    {
        long qdata[4], rdata[9];
        inv_get_quaternion(qdata);
        inv_quaternion_to_rotation(qdata, rdata);
        data[0] = (float)rdata[0] / 1073741824.0f;
        data[1] = (float)rdata[1] / 1073741824.0f;
        data[2] = (float)rdata[2] / 1073741824.0f;
        data[3] = (float)rdata[3] / 1073741824.0f;
        data[4] = (float)rdata[4] / 1073741824.0f;
        data[5] = (float)rdata[5] / 1073741824.0f;
        data[6] = (float)rdata[6] / 1073741824.0f;
        data[7] = (float)rdata[7] / 1073741824.0f;
        data[8] = (float)rdata[8] / 1073741824.0f;
    }

    return result;
}

/**
 *  @brief  inv_get_quaternion_float is used to get the quaternion representation
 *          of the current sensor fusion solution.
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 4 cells long</b>.
 *
 *  @return INV_SUCCESS if the command is successful; an ML error code otherwise.
 */
 /* inv_get_quaternion_float implemented in mlFIFO.c */

/**
 *  @brief  inv_get_linear_accel_float is used to get an estimate of linear
 *          acceleration, based on the most recent accelerometer measurement
 *          and sensor fusion solution.
 *          The values are in units of g (gravity).
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 3 cells long</b>.
 *
 *  @return INV_SUCCESS if the command is successful; an error code otherwise.
 */
inv_error_t inv_get_linear_accel_float(float *data)
{
    INVENSENSE_FUNC_START;

    inv_error_t result = INV_SUCCESS;
    long ldata[3];

    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    result = inv_get_linear_accel(ldata);
    data[0] = (float)ldata[0] / 65536.0f;
    data[1] = (float)ldata[1] / 65536.0f;
    data[2] = (float)ldata[2] / 65536.0f;

    return result;
}

/**
 *  @brief  inv_get_linear_accel_in_world_float is used to get an estimate of
 *          linear acceleration, in the world frame,  based on the most
 *          recent accelerometer measurement and sensor fusion solution.
 *          The values are in units of g (gravity).
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 3 cells long</b>.
 *
 *  @return INV_SUCCESS if the command is successful; an error code otherwise.
 */
inv_error_t inv_get_linear_accel_in_world_float(float *data)
{
    INVENSENSE_FUNC_START;

    inv_error_t result = INV_SUCCESS;
    long ldata[3];

    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    result = inv_get_linear_accel_in_world(ldata);
    data[0] = (float)ldata[0] / 65536.0f;
    data[1] = (float)ldata[1] / 65536.0f;
    data[2] = (float)ldata[2] / 65536.0f;

    return result;
}

/**
 *  @brief  inv_get_gravity_float is used to get an estimate of the body frame
 *          gravity vector, based on the most recent sensor fusion solution.
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 3 cells long at least</b>.
 *
 *  @return INV_SUCCESS if the command is successful; an error code otherwise.
 */
inv_error_t inv_get_gravity_float(float *data)
{
    INVENSENSE_FUNC_START;

    inv_error_t result = INV_SUCCESS;
    long ldata[3];

    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }
    result = inv_get_gravity(ldata);
    data[0] = (float)ldata[0] / 65536.0f;
    data[1] = (float)ldata[1] / 65536.0f;
    data[2] = (float)ldata[2] / 65536.0f;

    return result;
}

/**
 *  @brief  inv_get_gyro_cal_matrix_float is used to get the gyroscope
 *          calibration matrix. The gyroscope calibration matrix defines the relationship
 *          between the gyroscope sensor axes and the sensor fusion solution axes.
 *          Calibration matrix data members will have a value of 1.0, 0, or -1.0.
 *          The matrix has members
 *          <center>C11 C12 C13</center>
 *          <center>C21 C22 C23</center>
 *          <center>C31 C32 C33</center>
 *          The argument array elements are ordered C11, C12, C13, C21, C22, C23, C31, C32, C33.
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 9 cells long</b>.
 *
 *  @return INV_SUCCESS if the command is successful; an error code otherwise.
 */
inv_error_t inv_get_gyro_cal_matrix_float(float *data)
{
    INVENSENSE_FUNC_START;

    inv_error_t result = INV_SUCCESS;

    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    data[0] = (float)inv_obj.gyro_cal[0] / 1073741824.0f;
    data[1] = (float)inv_obj.gyro_cal[1] / 1073741824.0f;
    data[2] = (float)inv_obj.gyro_cal[2] / 1073741824.0f;
    data[3] = (float)inv_obj.gyro_cal[3] / 1073741824.0f;
    data[4] = (float)inv_obj.gyro_cal[4] / 1073741824.0f;
    data[5] = (float)inv_obj.gyro_cal[5] / 1073741824.0f;
    data[6] = (float)inv_obj.gyro_cal[6] / 1073741824.0f;
    data[7] = (float)inv_obj.gyro_cal[7] / 1073741824.0f;
    data[8] = (float)inv_obj.gyro_cal[8] / 1073741824.0f;

    return result;
}

/**
 *  @brief  inv_get_accel_cal_matrix_float is used to get the accelerometer
 *          calibration matrix.
 *          Calibration matrix data members will have a value of 1.0, 0, or -1.0.
 *          The matrix has members
 *          <center>C11 C12 C13</center>
 *          <center>C21 C22 C23</center>
 *          <center>C31 C32 C33</center>
 *          The argument array elements are ordered C11, C12, C13, C21, C22, C23, C31, C32, C33.
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 9 cells long</b>.
 *
 *  @return INV_SUCCESS if the command is successful; an error code otherwise.
 */

inv_error_t inv_get_accel_cal_matrix_float(float *data)
{
    INVENSENSE_FUNC_START;

    inv_error_t result = INV_SUCCESS;

    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    data[0] = (float)inv_obj.accel_cal[0] / 1073741824.0f;
    data[1] = (float)inv_obj.accel_cal[1] / 1073741824.0f;
    data[2] = (float)inv_obj.accel_cal[2] / 1073741824.0f;
    data[3] = (float)inv_obj.accel_cal[3] / 1073741824.0f;
    data[4] = (float)inv_obj.accel_cal[4] / 1073741824.0f;
    data[5] = (float)inv_obj.accel_cal[5] / 1073741824.0f;
    data[6] = (float)inv_obj.accel_cal[6] / 1073741824.0f;
    data[7] = (float)inv_obj.accel_cal[7] / 1073741824.0f;
    data[8] = (float)inv_obj.accel_cal[8] / 1073741824.0f;

    return result;
}

/**
 *  @cond MPL
 *  @brief  inv_get_mag_cal_matrix_float is used to get an array of nine data points
 *			representing the calibration matrix for the compass:
 *          <center>C11 C12 C13</center>
 *          <center>C21 C22 C23</center>
 *          <center>C31 C32 C33</center>
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 9 cells long at least</b>.
 *
 *  @return INV_SUCCESS if the command is successful; an error code otherwise.
 *  @endcond
 */
inv_error_t inv_get_mag_cal_matrix_float(float *data)
{
    INVENSENSE_FUNC_START;

    inv_error_t result = INV_SUCCESS;

    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    data[0] = (float)inv_obj.compass_cal[0] / 1073741824.0f;
    data[1] = (float)inv_obj.compass_cal[1] / 1073741824.0f;
    data[2] = (float)inv_obj.compass_cal[2] / 1073741824.0f;
    data[3] = (float)inv_obj.compass_cal[3] / 1073741824.0f;
    data[4] = (float)inv_obj.compass_cal[4] / 1073741824.0f;
    data[5] = (float)inv_obj.compass_cal[5] / 1073741824.0f;
    data[6] = (float)inv_obj.compass_cal[6] / 1073741824.0f;
    data[7] = (float)inv_obj.compass_cal[7] / 1073741824.0f;
    data[8] = (float)inv_obj.compass_cal[8] / 1073741824.0f;
    return result;
}

/**
 *  @brief  inv_get_gyro_temp_slope_float is used to get the temperature
 *          compensation algorithm's estimate of the gyroscope bias temperature
 *          coefficient.
 *          The argument array elements are ordered X,Y,Z.
 *          Values are in units of dps per deg C (degrees per second per degree
 *          Celcius)
 *          Please refer to the provided "9-Axis Sensor Fusion
 *          Application Note" document.
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 3 cells long </b>.
 *
 *  @return INV_SUCCESS if the command is successful; an error code otherwise.
 */
inv_error_t inv_get_gyro_temp_slope_float(float *data)
{
    INVENSENSE_FUNC_START;

    inv_error_t result = INV_SUCCESS;

    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    if (inv_params_obj.bias_mode & INV_LEARN_BIAS_FROM_TEMPERATURE) {
        data[0] = inv_obj.x_gyro_coef[1];
        data[1] = inv_obj.y_gyro_coef[1];
        data[2] = inv_obj.z_gyro_coef[1];
    } else {
        data[0] = (float)inv_obj.temp_slope[0] / 65536.0f;
        data[1] = (float)inv_obj.temp_slope[1] / 65536.0f;
        data[2] = (float)inv_obj.temp_slope[2] / 65536.0f;
    }

    return result;
}

/**
 *  @brief  inv_get_gyro_bias_float is used to get the gyroscope biases.
 *          The argument array elements are ordered X,Y,Z.
 *          The values are in units of dps (degrees per second).

 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 3 cells long</b>.
 *
 *  @return INV_SUCCESS if the command is successful; an error code otherwise.
 */
inv_error_t inv_get_gyro_bias_float(float *data)
{
    INVENSENSE_FUNC_START;

    inv_error_t result = INV_SUCCESS;

    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    data[0] = (float)inv_obj.gyro_bias[0] / 65536.0f;
    data[1] = (float)inv_obj.gyro_bias[1] / 65536.0f;
    data[2] = (float)inv_obj.gyro_bias[2] / 65536.0f;

    return result;
}

/**
 *  @brief  inv_get_accel_bias_float is used to get the accelerometer baises.
 *          The argument array elements are ordered X,Y,Z.
 *          The values are in units of g (gravity).
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 3 cells long</b>.
 *
 *  @return INV_SUCCESS if the command is successful; an error code otherwise.
 */
inv_error_t inv_get_accel_bias_float(float *data)
{
    INVENSENSE_FUNC_START;

    inv_error_t result = INV_SUCCESS;

    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    data[0] = (float)inv_obj.accel_bias[0] / 65536.0f;
    data[1] = (float)inv_obj.accel_bias[1] / 65536.0f;
    data[2] = (float)inv_obj.accel_bias[2] / 65536.0f;

    return result;
}

/**
 *  @cond MPL
 *  @brief  inv_get_mag_bias_float is used to get an array of three data points representing
 *			the compass biases.
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 3 cells long</b>.
 *
 *  @return INV_SUCCESS if the command is successful; an error code otherwise.
 *  @endcond
 */
inv_error_t inv_get_mag_bias_float(float *data)
{
    INVENSENSE_FUNC_START;

    inv_error_t result = INV_SUCCESS;

    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    data[0] =
        ((float)
         (inv_obj.compass_bias[0] +
          (long)((long long)inv_obj.init_compass_bias[0] *
                 inv_obj.compass_sens / 16384))) / 65536.0f;
    data[1] =
        ((float)
         (inv_obj.compass_bias[1] +
          (long)((long long)inv_obj.init_compass_bias[1] *
                 inv_obj.compass_sens / 16384))) / 65536.0f;
    data[2] =
        ((float)
         (inv_obj.compass_bias[2] +
          (long)((long long)inv_obj.init_compass_bias[2] *
                 inv_obj.compass_sens / 16384))) / 65536.0f;

    return result;
}

/**
 *  @brief  inv_get_gyro_and_accel_sensor_float is used to get the most recent set of all sensor data.
 *          The argument array elements are ordered gyroscope X,Y, and Z,
 *          accelerometer X, Y, and Z, and magnetometer X,Y, and Z.
 *          \if UMPL The magnetometer elements are not populated in UMPL. \endif
 *          The gyroscope and accelerometer data is not scaled or offset, it is
 *          copied directly from the sensor registers, and cast as a float.
 *          In the case of accelerometers with 8-bit output resolution, the data
 *          is scaled up to match the 2^14 = 1 g typical represntation of +/- 2 g
 *          full scale range

 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 9 cells long</b>.
 *
 *  @return INV_SUCCESS if the command is successful; an error code otherwise.
 */
inv_error_t inv_get_gyro_and_accel_sensor_float(float *data)
{
    INVENSENSE_FUNC_START;

    inv_error_t result = INV_SUCCESS;
    long ldata[6];

    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    result = inv_get_gyro_and_accel_sensor(ldata);
    data[0] = (float)ldata[0];
    data[1] = (float)ldata[1];
    data[2] = (float)ldata[2];
    data[3] = (float)ldata[3];
    data[4] = (float)ldata[4];
    data[5] = (float)ldata[5];
    data[6] = (float)inv_obj.compass_sensor_data[0];
    data[7] = (float)inv_obj.compass_sensor_data[1];
    data[8] = (float)inv_obj.compass_sensor_data[2];

    return result;
}

/**
 *  @brief  inv_get_euler_angles_x is used to get the Euler angle representation
 *          of the current sensor fusion solution.
 *          Euler angles are returned according to the X convention.
 *          This is typically the convention used for mobile devices where the X
 *          axis is the width of the screen, Y axis is the height, and Z the
 *          depth. In this case roll is defined as the rotation around the X
 *          axis of the device.
 *          <TABLE>
 *          <TR><TD>Element </TD><TD><b>Euler angle</b></TD><TD><b>Rotation about </b></TD></TR>
 *          <TR><TD> 0      </TD><TD>Roll              </TD><TD>X axis                </TD></TR>
 *          <TR><TD> 1      </TD><TD>Pitch             </TD><TD>Y axis                </TD></TR>
 *          <TR><TD> 2      </TD><TD>Yaw               </TD><TD>Z axis                </TD></TR>
 *
           </TABLE>
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 3 cells long</b>.
 *
 *  @return INV_SUCCESS if the command is successful; an error code otherwise.
 */
inv_error_t inv_get_euler_angles_x_float(float *data)
{
    INVENSENSE_FUNC_START;

    inv_error_t result = INV_SUCCESS;
    float rotMatrix[9];
    float tmp;

    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    result = inv_get_rot_mat_float(rotMatrix);
    tmp = rotMatrix[6];
    if (tmp > 1.0f) {
        tmp = 1.0f;
    }
    if (tmp < -1.0f) {
        tmp = -1.0f;
    }
    data[0] =
        (float)(atan2f(rotMatrix[7],
                       rotMatrix[8]) * 57.29577951308);
    data[1] = (float)((double)asin(tmp) * 57.29577951308);
    data[2] =
        (float)(atan2f(rotMatrix[3], rotMatrix[0]) * 57.29577951308);

    return result;
}

/**
 *  @brief  inv_get_euler_angles_float is used to get an array of three data points three data points
 *			representing roll, pitch, and yaw corresponding to the INV_EULER_ANGLES_X output and it is
 *          therefore the default convention for Euler angles.
 *          Please refer to the INV_EULER_ANGLES_X for a detailed description.
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 3 cells long</b>.
 *
 *  @return INV_SUCCESS if the command is successful; an error code otherwise.
 */
inv_error_t inv_get_euler_angles_float(float *data)
{
    return inv_get_euler_angles_x_float(data);
}

/**  @brief  inv_get_euler_angles_y_float is used to get the Euler angle representation
 *          of the current sensor fusion solution.
 *          Euler angles are returned according to the Y convention.
 *          This convention is typically used in augmented reality applications,
 *          where roll is defined as the rotation around the axis along the
 *          height of the screen of a mobile device, namely the Y axis.
 *          <TABLE>
 *          <TR><TD>Element </TD><TD><b>Euler angle</b></TD><TD><b>Rotation about </b></TD></TR>
 *          <TR><TD> 0      </TD><TD>Roll              </TD><TD>Y axis                </TD></TR>
 *          <TR><TD> 1      </TD><TD>Pitch             </TD><TD>X axis                </TD></TR>
 *          <TR><TD> 2      </TD><TD>Yaw               </TD><TD>Z axis                </TD></TR>
 *          </TABLE>
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 3 cells long</b>.
 *
 *  @return INV_SUCCESS if the command is successful; an error code otherwise.
 */
inv_error_t inv_get_euler_angles_y_float(float *data)
{
    INVENSENSE_FUNC_START;

    inv_error_t result = INV_SUCCESS;
    float rotMatrix[9];
    float tmp;

    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    result = inv_get_rot_mat_float(rotMatrix);
    tmp = rotMatrix[7];
    if (tmp > 1.0f) {
        tmp = 1.0f;
    }
    if (tmp < -1.0f) {
        tmp = -1.0f;
    }
    data[0] =
        (float)(atan2f(rotMatrix[8], rotMatrix[6]) * 57.29577951308);
    data[1] = (float)((double)asin(tmp) * 57.29577951308);
    data[2] =
        (float)(atan2f(rotMatrix[4], rotMatrix[1]) * 57.29577951308);

    return result;
}

/**  @brief  inv_get_euler_angles_z_float is used to get the Euler angle representation
 *          of the current sensor fusion solution.
 *          This convention is mostly used in application involving the use
 *          of a camera, typically placed on the back of a mobile device, that
 *          is along the Z axis.  In this convention roll is defined as the
 *          rotation around the Z axis.
 *          Euler angles are returned according to the Y convention.
 *          <TABLE>
 *          <TR><TD>Element </TD><TD><b>Euler angle</b></TD><TD><b>Rotation about </b></TD></TR>
 *          <TR><TD> 0      </TD><TD>Roll              </TD><TD>Z axis                </TD></TR>
 *          <TR><TD> 1      </TD><TD>Pitch             </TD><TD>X axis                </TD></TR>
 *          <TR><TD> 2      </TD><TD>Yaw               </TD><TD>Y axis                </TD></TR>
 *          </TABLE>
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 3 cells long</b>.
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 3 cells long</b>.
 *
 *  @return INV_SUCCESS if the command is successful; an error code otherwise.
 */
inv_error_t inv_get_euler_angles_z_float(float *data)
{
    INVENSENSE_FUNC_START;

    inv_error_t result = INV_SUCCESS;
    float rotMatrix[9];
    float tmp;

    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    result = inv_get_rot_mat_float(rotMatrix);
    tmp = rotMatrix[8];
    if (tmp > 1.0f) {
        tmp = 1.0f;
    }
    if (tmp < -1.0f) {
        tmp = -1.0f;
    }
    data[0] =
        (float)(atan2f(rotMatrix[6], rotMatrix[7]) * 57.29577951308);
    data[1] = (float)((double)asin(tmp) * 57.29577951308);
    data[2] =
        (float)(atan2f(rotMatrix[5], rotMatrix[2]) * 57.29577951308);

    return result;
}

/**
 *  @cond MPL
 *  @brief  inv_get_mag_raw_data_float is used to get Raw magnetometer data
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 3 cells long</b>.
 *
 *  @return INV_SUCCESS if the command is successful; an error code otherwise.
 *  @endcond
 */
inv_error_t inv_get_mag_raw_data_float(float *data)
{
    INVENSENSE_FUNC_START;

    inv_error_t result = INV_SUCCESS;

    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    data[0] =
        (float)(inv_obj.compass_sensor_data[0] + inv_obj.init_compass_bias[0]);
    data[1] =
        (float)(inv_obj.compass_sensor_data[1] + inv_obj.init_compass_bias[1]);
    data[2] =
        (float)(inv_obj.compass_sensor_data[2] + inv_obj.init_compass_bias[2]);

    return result;
}

/**
 *  @cond MPL
 *  @brief  inv_get_magnetometer_float is used to get magnetometer data
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 3 cells long</b>.
 *
 *  @return INV_SUCCESS if the command is successful; an error code otherwise.
 *  @endcond
 */
inv_error_t inv_get_magnetometer_float(float *data)
{
    INVENSENSE_FUNC_START;

    inv_error_t result = INV_SUCCESS;

    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    data[0] = (float)inv_obj.compass_calibrated_data[0] / 65536.0f;
    data[1] = (float)inv_obj.compass_calibrated_data[1] / 65536.0f;
    data[2] = (float)inv_obj.compass_calibrated_data[2] / 65536.0f;

    return result;
}

/**
 *  @cond MPL
 *  @brief  inv_get_pressure_float is used to get a single value representing the pressure in Pascal
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to the data to be passed back to the user.
 *
 *  @return INV_SUCCESS if the command is successful; an error code otherwise.
 *  @endcond
 */
inv_error_t inv_get_pressure_float(float *data)
{
    INVENSENSE_FUNC_START;

    inv_error_t result = INV_SUCCESS;

    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    data[0] = (float)inv_obj.pressure;

    return result;
}

/**
 *  @cond MPL
 *  @brief  inv_get_heading_float is used to get single number representing the heading of the device
 *          relative to the Earth, in which 0 represents North, 90 degrees
 *          represents East, and so on.
 *          The heading is defined as the direction of the +Y axis if the Y
 *          axis is horizontal, and otherwise the direction of the -Z axis.
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to the data to be passed back to the user.
 *
 *  @return INV_SUCCESS if the command is successful; an error code otherwise.
 *  @endcond
 */
inv_error_t inv_get_heading_float(float *data)
{
    INVENSENSE_FUNC_START;

    inv_error_t result = INV_SUCCESS;
    float rotMatrix[9];
    float tmp;

    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    inv_get_rot_mat_float(rotMatrix);
    if ((rotMatrix[7] < 0.707) && (rotMatrix[7] > -0.707)) {
        tmp =
            (float)(atan2f(rotMatrix[4], rotMatrix[1]) * 57.29577951308 -
                    90.0f);
    } else {
        tmp =
            (float)(atan2f(rotMatrix[5], rotMatrix[2]) * 57.29577951308 +
                    90.0f);
    }
    if (tmp < 0) {
        tmp += 360.0f;
    }
    data[0] = 360 - tmp;

    return result;
}

/**
 *  @cond MPL
 *  @brief  inv_get_mag_bias_error_float is used to get an array of three numbers representing
 *			the current estimated error in the compass biases. These numbers are unitless and serve
 *          as rough estimates in which numbers less than 100 typically represent
 *          reasonably well calibrated compass axes.
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 3 cells long</b>.
 *
 *  @return INV_SUCCESS if the command is successful; an error code otherwise.
 *  @endcond
 */
inv_error_t inv_get_mag_bias_error_float(float *data)
{
    INVENSENSE_FUNC_START;

    inv_error_t result = INV_SUCCESS;

    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    if (inv_obj.large_field == 0) {
        data[0] = (float)inv_obj.compass_bias_error[0];
        data[1] = (float)inv_obj.compass_bias_error[1];
        data[2] = (float)inv_obj.compass_bias_error[2];
    } else {
        data[0] = (float)P_INIT;
        data[1] = (float)P_INIT;
        data[2] = (float)P_INIT;
    }

    return result;
}

/**
 *  @cond MPL
 *  @brief  inv_get_mag_scale_float is used to get magnetometer scale.
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 3 cells long</b>.
 *
 *  @return INV_SUCCESS if the command is successful; an error code otherwise.
 *  @endcond
 */
inv_error_t inv_get_mag_scale_float(float *data)
{
    INVENSENSE_FUNC_START;

    inv_error_t result = INV_SUCCESS;

    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    data[0] = (float)inv_obj.compass_scale[0] / 65536.0f;
    data[1] = (float)inv_obj.compass_scale[1] / 65536.0f;
    data[2] = (float)inv_obj.compass_scale[2] / 65536.0f;

    return result;
}

/**
 *  @cond MPL
 *  @brief  inv_get_local_field_float is used to get local magnetic field data.
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 3 cells long</b>.
 *
 *  @return INV_SUCCESS if the command is successful; an error code otherwise.
 *  @endcond
 */
inv_error_t inv_get_local_field_float(float *data)
{
    INVENSENSE_FUNC_START;

    inv_error_t result = INV_SUCCESS;

    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    data[0] = (float)inv_obj.local_field[0] / 65536.0f;
    data[1] = (float)inv_obj.local_field[1] / 65536.0f;
    data[2] = (float)inv_obj.local_field[2] / 65536.0f;

    return result;
}

/**
 *  @cond MPL
 *  @brief  inv_get_relative_quaternion_float is used to get relative quaternion data.
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or MLDmpPedometerStandAloneOpen() \endif
 *          must have been called.
 *
 *  @param  data
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 4 cells long at least</b>.
 *
 *  @return INV_SUCCESS if the command is successful; an error code otherwise.
 *  @endcond
 */
inv_error_t inv_get_relative_quaternion_float(float *data)
{
    INVENSENSE_FUNC_START;

    inv_error_t result = INV_SUCCESS;

    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    data[0] = (float)inv_obj.relative_quat[0] / 1073741824.0f;
    data[1] = (float)inv_obj.relative_quat[1] / 1073741824.0f;
    data[2] = (float)inv_obj.relative_quat[2] / 1073741824.0f;
    data[3] = (float)inv_obj.relative_quat[3] / 1073741824.0f;

    return result;
}

/**
 * Returns the curren compass accuracy.
 *
 * - 0: Unknown: The accuracy is unreliable and compass data should not be used
 * - 1: Low: The compass accuracy is low.
 * - 2: Medium: The compass accuracy is medium.
 * - 3: High: The compas acurracy is high and can be trusted
 *
 * @param accuracy The accuracy level in the range 0-3
 *
 * @return ML_SUCCESS or non-zero error code
 */
inv_error_t inv_get_compass_accuracy(int *accuracy)
{
    if (inv_get_state() != INV_STATE_DMP_STARTED)
        return INV_ERROR_SM_IMPROPER_STATE;

    *accuracy = inv_obj.compass_accuracy;
    return INV_SUCCESS;
}

/**
 *  @brief  inv_set_gyro_bias is used to set the gyroscope bias.
 *          The argument array elements are ordered X,Y,Z.
 *          The values are scaled at 1 dps = 2^16 LSBs.
 *
 *          Please refer to the provided "9-Axis Sensor Fusion
 *          Application Note" document provided.
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or
 *          MLDmpPedometerStandAloneOpen() \endif
 *
 *  @param  data        A pointer to an array to be copied from the user.
 *
 *  @return INV_SUCCESS if successful; a non-zero error code otherwise.
 */
inv_error_t inv_set_gyro_bias(long *data)
{
    INVENSENSE_FUNC_START;
    inv_error_t result = INV_SUCCESS;
    long biasTmp;
    long sf = 0;
    short offset[GYRO_NUM_AXES];
    int i;
    struct mldl_cfg *mldl_cfg = inv_get_dl_config();

    if (mldl_cfg->gyro_sens_trim != 0) {
        sf = 2000 * 131 / mldl_cfg->gyro_sens_trim;
    } else {
        sf = 2000;
    }
    for (i = 0; i < GYRO_NUM_AXES; i++) {
        inv_obj.gyro_bias[i] = data[i];
        biasTmp = -inv_obj.gyro_bias[i] / sf;
        if (biasTmp < 0)
            biasTmp += 65536L;
        offset[i] = (short)biasTmp;
    }
    result = inv_set_offset(offset);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    return INV_SUCCESS;
}

/**
 *  @brief  inv_set_accel_bias is used to set the accelerometer bias.
 *          The argument array elements are ordered X,Y,Z.
 *          The values are scaled in units of g (gravity),
 *          where 1 g = 2^16 LSBs.
 *
 *          Please refer to the provided "9-Axis Sensor Fusion
 *          Application Note" document provided.
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or
 *          MLDmpPedometerStandAloneOpen() \endif
 *
 *  @param  data        A pointer to an array to be copied from the user.
 *
 *  @return INV_SUCCESS if successful; a non-zero error code otherwise.
 */
inv_error_t inv_set_accel_bias(long *data)
{
    INVENSENSE_FUNC_START;
    inv_error_t result = INV_SUCCESS;
    long biasTmp;
    int i, j;
    unsigned char regs[6];
    struct mldl_cfg *mldl_cfg = inv_get_dl_config();

    for (i = 0; i < ACCEL_NUM_AXES; i++) {
        inv_obj.accel_bias[i] = data[i];
        if (inv_obj.accel_sens != 0 && mldl_cfg && mldl_cfg->pdata) {
            long long tmp64;
            inv_obj.scaled_accel_bias[i] = 0;
            for (j = 0; j < ACCEL_NUM_AXES; j++) {
                inv_obj.scaled_accel_bias[i] +=
                    data[j] *
                    (long)mldl_cfg->pdata->accel.orientation[i * 3 + j];
            }
            tmp64 = (long long)inv_obj.scaled_accel_bias[i] << 13;
            biasTmp = (long)(tmp64 / inv_obj.accel_sens);
        } else {
            biasTmp = 0;
        }
        if (biasTmp < 0)
            biasTmp += 65536L;
        regs[2 * i + 0] = (unsigned char)(biasTmp / 256);
        regs[2 * i + 1] = (unsigned char)(biasTmp % 256);
    }
    result = inv_set_mpu_memory(KEY_D_1_8, 2, &regs[0]);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    result = inv_set_mpu_memory(KEY_D_1_10, 2, &regs[2]);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    result = inv_set_mpu_memory(KEY_D_1_2, 2, &regs[4]);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    return INV_SUCCESS;
}

/**
 *  @cond MPL
 *  @brief  inv_set_mag_bias is used to set Compass Bias
 *
 *          Please refer to the provided "9-Axis Sensor Fusion
 *          Application Note" document provided.
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or
 *          MLDmpPedometerStandAloneOpen() \endif
 *  @pre    MLDmpStart() must <b>NOT</b> have been called.
 *
 *  @param  data        A pointer to an array to be copied from the user.
 *
 *  @return INV_SUCCESS if successful; a non-zero error code otherwise.
 *  @endcond
 */
inv_error_t inv_set_mag_bias(long *data)
{
    INVENSENSE_FUNC_START;
    inv_error_t result = INV_SUCCESS;

    inv_set_compass_bias(data);
    inv_obj.init_compass_bias[0] = 0;
    inv_obj.init_compass_bias[1] = 0;
    inv_obj.init_compass_bias[2] = 0;
    inv_obj.got_compass_bias = 1;
    inv_obj.got_init_compass_bias = 1;
    inv_obj.compass_state = SF_STARTUP_SETTLE;

    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    return INV_SUCCESS;
}

/**
 *  @brief  inv_set_gyro_temp_slope is used to set the temperature
 *          compensation algorithm's estimate of the gyroscope bias temperature
 *          coefficient.
 *          The argument array elements are ordered X,Y,Z.
 *          Values are in units of dps per deg C (degrees per second per degree
 *          Celcius), and scaled such that 1 dps per deg C = 2^16 LSBs.
 *          Please refer to the provided "9-Axis Sensor Fusion
 *          Application Note" document.
 *
 *  @brief  inv_set_gyro_temp_slope is used to set Gyro temperature slope
 *
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or
 *          MLDmpPedometerStandAloneOpen() \endif
 *
 *  @param  data        A pointer to an array to be copied from the user.
 *
 *  @return INV_SUCCESS if successful; a non-zero error code otherwise.
 */
inv_error_t inv_set_gyro_temp_slope(long *data)
{
    INVENSENSE_FUNC_START;
    inv_error_t result = INV_SUCCESS;
    int i;
    long sf;
    unsigned char regs[3];

    inv_obj.factory_temp_comp = 1;
    inv_obj.temp_slope[0] = data[0];
    inv_obj.temp_slope[1] = data[1];
    inv_obj.temp_slope[2] = data[2];
    for (i = 0; i < GYRO_NUM_AXES; i++) {
        sf = -inv_obj.temp_slope[i] / 1118;
        if (sf > 127) {
            sf -= 256;
        }
        regs[i] = (unsigned char)sf;
    }
    result = inv_set_offsetTC(regs);

    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    return INV_SUCCESS;
}

/**
 *  @cond MPL
 *  @brief  inv_set_local_field is used to set local magnetic field
 *
 *          Please refer to the provided "9-Axis Sensor Fusion
 *          Application Note" document provided.
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or
 *          MLDmpPedometerStandAloneOpen() \endif
 *  @pre    MLDmpStart() must <b>NOT</b> have been called.
 *
 *  @param  data        A pointer to an array to be copied from the user.
 *
 *  @return INV_SUCCESS if successful; a non-zero error code otherwise.
 *  @endcond
 */
inv_error_t inv_set_local_field(long *data)
{
    INVENSENSE_FUNC_START;
    inv_error_t result = INV_SUCCESS;

    inv_obj.local_field[0] = data[0];
    inv_obj.local_field[1] = data[1];
    inv_obj.local_field[2] = data[2];
    inv_obj.new_local_field = 1;

    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    return INV_SUCCESS;
}

/**
 *  @cond MPL
 *  @brief  inv_set_mag_scale is used to set magnetometer scale
 *
 *          Please refer to the provided "9-Axis Sensor Fusion
 *          Application Note" document provided.
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or
 *          MLDmpPedometerStandAloneOpen() \endif
 *  @pre    MLDmpStart() must <b>NOT</b> have been called.
 *
 *  @param  data        A pointer to an array to be copied from the user.
 *
 *  @return INV_SUCCESS if successful; a non-zero error code otherwise.
 *  @endcond
 */
inv_error_t inv_set_mag_scale(long *data)
{
    INVENSENSE_FUNC_START;
    inv_error_t result = INV_SUCCESS;

    inv_obj.compass_scale[0] = data[0];
    inv_obj.compass_scale[1] = data[1];
    inv_obj.compass_scale[2] = data[2];

    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    return INV_SUCCESS;
}

/**
 *  @brief  inv_set_gyro_temp_slope_float is used to get the temperature
 *          compensation algorithm's estimate of the gyroscope bias temperature
 *          coefficient.
 *          The argument array elements are ordered X,Y,Z.
 *          Values are in units of dps per deg C (degrees per second per degree
 *          Celcius)

 *          Please refer to the provided "9-Axis Sensor Fusion
 *          Application Note" document provided.
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or
 *          MLDmpPedometerStandAloneOpen() \endif
 *
 *  @param  data        A pointer to an array to be copied from the user.
 *
 *  @return INV_SUCCESS if successful; a non-zero error code otherwise.
 */
inv_error_t inv_set_gyro_temp_slope_float(float *data)
{
    long arrayTmp[3];
    arrayTmp[0] = (long)(data[0] * 65536.f);
    arrayTmp[1] = (long)(data[1] * 65536.f);
    arrayTmp[2] = (long)(data[2] * 65536.f);
    return inv_set_gyro_temp_slope(arrayTmp);
}

/**
 *  @brief  inv_set_gyro_bias_float is used to set the gyroscope bias.
 *          The argument array elements are ordered X,Y,Z.
 *          The values are in units of dps (degrees per second).
 *
 *          Please refer to the provided "9-Axis Sensor Fusion
 *          Application Note" document provided.
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or
 *          MLDmpPedometerStandAloneOpen() \endif
 *  @pre    MLDmpStart() must <b>NOT</b> have been called.
 *
 *  @param  data        A pointer to an array to be copied from the user.
 *
 *  @return INV_SUCCESS if successful; a non-zero error code otherwise.
 */
inv_error_t inv_set_gyro_bias_float(float *data)
{
    long arrayTmp[3];
    arrayTmp[0] = (long)(data[0] * 65536.f);
    arrayTmp[1] = (long)(data[1] * 65536.f);
    arrayTmp[2] = (long)(data[2] * 65536.f);
    return inv_set_gyro_bias(arrayTmp);

}

/**
 *  @brief  inv_set_accel_bias_float is used to set the accelerometer bias.
 *          The argument array elements are ordered X,Y,Z.
 *          The values are in units of g (gravity).
 *
 *          Please refer to the provided "9-Axis Sensor Fusion
 *          Application Note" document provided.
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or
 *          MLDmpPedometerStandAloneOpen() \endif
 *  @pre    MLDmpStart() must <b>NOT</b> have been called.
 *
 *  @param  data        A pointer to an array to be copied from the user.
 *
 *  @return INV_SUCCESS if successful; a non-zero error code otherwise.
 */
inv_error_t inv_set_accel_bias_float(float *data)
{
    long arrayTmp[3];
    arrayTmp[0] = (long)(data[0] * 65536.f);
    arrayTmp[1] = (long)(data[1] * 65536.f);
    arrayTmp[2] = (long)(data[2] * 65536.f);
    return inv_set_accel_bias(arrayTmp);

}

/**
 *  @cond MPL
 *  @brief  inv_set_mag_bias_float is used to set compass bias
 *
 *          Please refer to the provided "9-Axis Sensor Fusion
 *          Application Note" document provided.
 *
 *  @pre    MLDmpOpen()\ifnot UMPL or
 *          MLDmpPedometerStandAloneOpen() \endif
 *  @pre    MLDmpStart() must <b>NOT</b> have been called.
 *
 *  @param  data        A pointer to an array to be copied from the user.
 *
 *  @return INV_SUCCESS if successful; a non-zero error code otherwise.
 *  @endcond
 */
inv_error_t inv_set_mag_bias_float(float *data)
{
    long arrayTmp[3];
    arrayTmp[0] = (long)(data[0] * 65536.f);
    arrayTmp[1] = (long)(data[1] * 65536.f);
    arrayTmp[2] = (long)(data[2] * 65536.f);
    return inv_set_mag_bias(arrayTmp);
}

/**
 *  @cond MPL
 *  @brief  inv_set_local_field_float is used to set local magnetic field
 *
 *          Please refer to the provided "9-Axis Sensor Fusion
 *          Application Note" document provided.
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or
 *          MLDmpPedometerStandAloneOpen() \endif
 *  @pre    MLDmpStart() must <b>NOT</b> have been called.
 *
 *  @param  data        A pointer to an array to be copied from the user.
 *
 *  @return INV_SUCCESS if successful; a non-zero error code otherwise.
 *  @endcond
 */
inv_error_t inv_set_local_field_float(float *data)
{
    long arrayTmp[3];
    arrayTmp[0] = (long)(data[0] * 65536.f);
    arrayTmp[1] = (long)(data[1] * 65536.f);
    arrayTmp[2] = (long)(data[2] * 65536.f);
    return inv_set_local_field(arrayTmp);
}

/**
 *  @cond MPL
 *  @brief  inv_set_mag_scale_float is used to set magnetometer scale
 *
 *          Please refer to the provided "9-Axis Sensor Fusion
 *          Application Note" document provided.
 *
 *  @pre    MLDmpOpen() \ifnot UMPL or
 *          MLDmpPedometerStandAloneOpen() \endif
 *  @pre    MLDmpStart() must <b>NOT</b> have been called.
 *
 *  @param  data        A pointer to an array to be copied from the user.
 *
 *  @return INV_SUCCESS if successful; a non-zero error code otherwise.
 *  @endcond
 */
inv_error_t inv_set_mag_scale_float(float *data)
{
    long arrayTmp[3];
    arrayTmp[0] = (long)(data[0] * 65536.f);
    arrayTmp[1] = (long)(data[1] * 65536.f);
    arrayTmp[2] = (long)(data[2] * 65536.f);
    return inv_set_mag_scale(arrayTmp);
}
