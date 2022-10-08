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
 * $Id: mlarray_legacy.c $
 *
 *****************************************************************************/

/** 
 *  @defgroup MLArray_Legacy 
 *  @brief  Legacy Motion Library Array APIs.
 *          The Motion Library Array APIs provide the user access to the
 *          Motion Library state. These Legacy APIs provide access to
 *          individual state arrays using a data set name as the first
 *          argument to the API. This format has been replaced by unique
 *          named APIs for each data set, found in the MLArray group.
 *
 *  @{
 *      @file   mlarray_legacy.c
 *      @brief  The Legacy Motion Library Array APIs.
 */

#include "ml.h"
#include "mltypes.h"
#include "mlinclude.h"
#include "mlFIFO.h"
#include "mldl_cfg.h"

/**
 *  @brief  inv_get_array is used to get an array of processed motion sensor data.
 *          inv_get_array can be used to retrieve various data sets. Certain data
 *          sets require functions to be enabled using MLEnable in order to be
 *          valid.
 *
 *          The available data sets are:
 *
 *          - INV_ROTATION_MATRIX
 *          - INV_QUATERNION
 *          - INV_EULER_ANGLES_X
 *          - INV_EULER_ANGLES_Y
 *          - INV_EULER_ANGLES_Z
 *          - INV_EULER_ANGLES
 *          - INV_LINEAR_ACCELERATION
 *          - INV_LINEAR_ACCELERATION_WORLD
 *          - INV_GRAVITY
 *          - INV_ANGULAR_VELOCITY
 *          - INV_RAW_DATA
 *          - INV_GYROS
 *          - INV_ACCELS
 *          - INV_MAGNETOMETER
 *          - INV_GYRO_BIAS
 *          - INV_ACCEL_BIAS
 *          - INV_MAG_BIAS 
 *          - INV_HEADING
 *          - INV_MAG_BIAS_ERROR
 *          - INV_PRESSURE
 *
 *          Please refer to the documentation of inv_get_float_array() for a 
 *          description of these data sets.
 *
 *  @pre    MLDmpOpen() or MLDmpPedometerStandAloneOpen()
 *          must have been called.
 *
 *  @param  dataSet     
 *              A constant specifying an array of data processed by the
 *              motion processor.
 *  @param  data        
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 9 cells long at least</b>.
 *
 *  @return Zero if the command is successful; an ML error code otherwise.
 */
inv_error_t inv_get_array(int dataSet, long *data)
{
    inv_error_t result;
    switch (dataSet) {
    case INV_GYROS:
        result = inv_get_gyro(data);
        break;
    case INV_ACCELS:
        result = inv_get_accel(data);
        break;
    case INV_TEMPERATURE:
        result = inv_get_temperature(data);
        break;
    case INV_ROTATION_MATRIX:
        result = inv_get_rot_mat(data);
        break;
    case INV_QUATERNION:
        result = inv_get_quaternion(data);
        break;
    case INV_LINEAR_ACCELERATION:
        result = inv_get_linear_accel(data);
        break;
    case INV_LINEAR_ACCELERATION_WORLD:
        result = inv_get_linear_accel_in_world(data);
        break;
    case INV_GRAVITY:
        result = inv_get_gravity(data);
        break;
    case INV_ANGULAR_VELOCITY:
        result = inv_get_angular_velocity(data);
        break;
    case INV_EULER_ANGLES:
        result = inv_get_euler_angles(data);
        break;
    case INV_EULER_ANGLES_X:
        result = inv_get_euler_angles_x(data);
        break;
    case INV_EULER_ANGLES_Y:
        result = inv_get_euler_angles_y(data);
        break;
    case INV_EULER_ANGLES_Z:
        result = inv_get_euler_angles_z(data);
        break;
    case INV_GYRO_TEMP_SLOPE:
        result = inv_get_gyro_temp_slope(data);
        break;
    case INV_GYRO_BIAS:
        result = inv_get_gyro_bias(data);
        break;
    case INV_ACCEL_BIAS:
        result = inv_get_accel_bias(data);
        break;
    case INV_MAG_BIAS:
        result = inv_get_mag_bias(data);
        break;
    case INV_RAW_DATA:
        result = inv_get_gyro_and_accel_sensor(data);
        break;
    case INV_MAG_RAW_DATA:
        result = inv_get_mag_raw_data(data);
        break;
    case INV_MAGNETOMETER:
        result = inv_get_magnetometer(data);
        break;
    case INV_PRESSURE:
        result = inv_get_pressure(data);
        break;
    case INV_HEADING:
        result = inv_get_heading(data);
        break;
    case INV_GYRO_CALIBRATION_MATRIX:
        result = inv_get_gyro_cal_matrix(data);
        break;
    case INV_ACCEL_CALIBRATION_MATRIX:
        result = inv_get_accel_cal_matrix(data);
        break;
    case INV_MAG_CALIBRATION_MATRIX:
        result = inv_get_mag_cal_matrix(data);
        break;
    case INV_MAG_BIAS_ERROR:
        result = inv_get_mag_bias_error(data);
        break;
    case INV_MAG_SCALE:
        result = inv_get_mag_scale(data);
        break;
    case INV_LOCAL_FIELD:
        result = inv_get_local_field(data);
        break;
    case INV_RELATIVE_QUATERNION:
        result = inv_get_relative_quaternion(data);
        break;
    default:
        return INV_ERROR_INVALID_PARAMETER;
        break;
    }
    return result;
}

/**
 *  @brief  inv_get_float_array is used to get an array of processed motion sensor 
 *          data. inv_get_array can be used to retrieve various data sets. 
 *          Certain data sets require functions to be enabled using MLEnable 
 *          in order to be valid.
 *
 *          The available data sets are:
 *
 *          - INV_ROTATION_MATRIX :
 *          Returns an array of nine data points representing the rotation 
 *          matrix generated from all available sensors. 
 *          This requires that ML_SENSOR_FUSION be enabled.
 *          The array format will be R11, R12, R13, R21, R22, R23, R31, R32, 
 *          R33, representing the matrix:
 *          <center>R11 R12 R13</center>
 *          <center>R21 R22 R23</center>
 *          <center>R31 R32 R33</center>
 *          <b>Please refer to the "9-Axis Sensor Fusion Application Note" document, 
 *          section 7 "Sensor Fusion Output", for details regarding rotation 
 *          matrix output</b>.
 *
 *          - INV_QUATERNION :
 *          Returns an array of four data points representing the quaternion 
 *          generated from all available sensors. 
 *          This requires that ML_SENSOR_FUSION be enabled.
 *
 *          - INV_EULER_ANGLES_X :
 *          Returns an array of three data points representing roll, pitch, and 
 *          yaw using the X axis of the gyroscope, accelerometer, and compass as 
 *          reference axis. 
 *          This is typically the convention used for mobile devices where the X
 *          axis is the width of the screen, Y axis is the height, and Z the 
 *          depth. In this case roll is defined as the rotation around the X 
 *          axis of the device.
 *          The euler angles convention for this output is the following:
 *          <TABLE>
 *          <TR><TD><b>EULER ANGLE</b></TD><TD><b>ROTATION AROUND</b></TD></TR>
 *          <TR><TD>roll              </TD><TD>X axis                </TD></TR>
 *          <TR><TD>pitch             </TD><TD>Y axis                </TD></TR>
 *          <TR><TD>yaw               </TD><TD>Z axis                </TD></TR>
 *          </TABLE>
 *          INV_EULER_ANGLES_X corresponds to the INV_EULER_ANGLES output and is 
 *          therefore the default convention.
 *
 *          - INV_EULER_ANGLES_Y :
 *          Returns an array of three data points representing roll, pitch, and 
 *          yaw using the Y axis of the gyroscope, accelerometer, and compass as 
 *          reference axis. 
 *          This convention is typically used in augmented reality applications, 
 *          where roll is defined as the rotation around the axis along the 
 *          height of the screen of a mobile device, namely the Y axis.
 *          The euler angles convention for this output is the following:
 *          <TABLE>
 *          <TR><TD><b>EULER ANGLE</b></TD><TD><b>ROTATION AROUND</b></TD></TR>
 *          <TR><TD>roll              </TD><TD>Y axis                </TD></TR>
 *          <TR><TD>pitch             </TD><TD>X axis                </TD></TR>
 *          <TR><TD>yaw               </TD><TD>Z axis                </TD></TR>
 *          </TABLE>
 *
 *          - INV_EULER_ANGLES_Z :
 *          Returns an array of three data points representing roll, pitch, and 
 *          yaw using the Z axis of the gyroscope, accelerometer, and compass as 
 *          reference axis. 
 *          This convention is mostly used in application involving the use 
 *          of a camera, typically placed on the back of a mobile device, that 
 *          is along the Z axis.  In this convention roll is defined as the 
 *          rotation around the Z axis.
 *          The euler angles convention for this output is the following:
 *          <TABLE>
 *          <TR><TD><b>EULER ANGLE</b></TD><TD><b>ROTATION AROUND</b></TD></TR>
 *          <TR><TD>roll              </TD><TD>Z axis                </TD></TR>
 *          <TR><TD>pitch             </TD><TD>X axis                </TD></TR>
 *          <TR><TD>yaw               </TD><TD>Y axis                </TD></TR>
 *          </TABLE>
 *
 *          - INV_EULER_ANGLES :
 *          Returns an array of three data points representing roll, pitch, and 
 *          yaw corresponding to the INV_EULER_ANGLES_X output and it is 
 *          therefore the default convention for Euler angles.
 *          Please refer to the INV_EULER_ANGLES_X for a detailed description.
 *
 *          - INV_LINEAR_ACCELERATION :
 *          Returns an array of three data points representing the linear 
 *          acceleration as derived from both gyroscopes and accelerometers. 
 *          This requires that ML_SENSOR_FUSION be enabled.
 *
 *          - INV_LINEAR_ACCELERATION_WORLD :
 *          Returns an array of three data points representing the linear 
 *          acceleration in world coordinates, as derived from both gyroscopes 
 *          and accelerometers.
 *          This requires that ML_SENSOR_FUSION be enabled.
 *
 *          - INV_GRAVITY :
 *          Returns an array of three data points representing the direction 
 *          of gravity in body coordinates, as derived from both gyroscopes 
 *          and accelerometers.
 *          This requires that ML_SENSOR_FUSION be enabled.
 *
 *          - INV_ANGULAR_VELOCITY :
 *          Returns an array of three data points representing the angular 
 *          velocity as derived from <b>both</b> gyroscopes and accelerometers.
 *          This requires that ML_SENSOR_FUSION be enabled, to fuse data from
 *          the gyroscope and accelerometer device, appropriately scaled and 
 *          oriented according to the respective mounting matrices.
 *
 *          - INV_RAW_DATA :
 *          Returns an array of nine data points representing raw sensor data 
 *          of the gyroscope X, Y, Z, accelerometer X, Y, Z, and 
 *          compass X, Y, Z values.
 *          These values are not scaled and come out directly from the devices'
 *          sensor data output. In case of accelerometers with lower output 
 *          resolution, e.g 8-bit, the sensor data is scaled up to match the 
 *          2^14 = 1 gee typical representation for a +/- 2 gee full scale 
 *          range.
 *
 *          - INV_GYROS :
 *          Returns an array of three data points representing the X gyroscope,
 *          Y gyroscope, and Z gyroscope values.
 *          The values are not sensor fused with other sensor types data but
 *          reflect the orientation from the mounting matrices in use.
 *          The INV_GYROS values are scaled to ensure 1 dps corresponds to 2^16 
 *          codes.
 *
 *          - INV_ACCELS :
 *          Returns an array of three data points representing the X 
 *          accelerometer, Y accelerometer, and Z accelerometer values.
 *          The values are not sensor fused with other sensor types data but
 *          reflect the orientation from the mounting matrices in use.
 *          The INV_ACCELS values are scaled to ensure 1 gee corresponds to 2^16
 *          codes.
 *
 *          - INV_MAGNETOMETER :
 *          Returns an array of three data points representing the compass
 *          X, Y, and Z values.
 *          The values are not sensor fused with other sensor types data but
 *          reflect the orientation from the mounting matrices in use.
 *          The INV_MAGNETOMETER values are scaled to ensure 1 micro Tesla (uT) 
 *          corresponds to 2^16 codes.
 *
 *          - INV_GYRO_BIAS :
 *          Returns an array of three data points representing the gyroscope 
 *          biases.
 *
 *          - INV_ACCEL_BIAS :
 *          Returns an array of three data points representing the 
 *          accelerometer biases.
 *
 *          - INV_MAG_BIAS :
 *          Returns an array of three data points representing the compass 
 *          biases.
 *
 *          - INV_GYRO_CALIBRATION_MATRIX :
 *          Returns an array of nine data points representing the calibration 
 *          matrix for the gyroscopes:
 *          <center>C11 C12 C13</center>
 *          <center>C21 C22 C23</center>
 *          <center>C31 C32 C33</center>
 *
 *          - INV_ACCEL_CALIBRATION_MATRIX :
 *          Returns an array of nine data points representing the calibration 
 *          matrix for the accelerometers:
 *          <center>C11 C12 C13</center>
 *          <center>C21 C22 C23</center>
 *          <center>C31 C32 C33</center>
 *
 *          - INV_MAG_CALIBRATION_MATRIX :
 *          Returns an array of nine data points representing the calibration 
 *          matrix for the compass:
 *          <center>C11 C12 C13</center>
 *          <center>C21 C22 C23</center>
 *          <center>C31 C32 C33</center>
 *
 *          - INV_PRESSURE :
 *          Returns a single value representing the pressure in Pascal
 *
 *          - INV_HEADING : 
 *          Returns a single number representing the heading of the device 
 *          relative to the Earth, in which 0 represents North, 90 degrees 
 *          represents East, and so on. 
 *          The heading is defined as the direction of the +Y axis if the Y 
 *          axis is horizontal, and otherwise the direction of the -Z axis.
 *
 *          - INV_MAG_BIAS_ERROR :
 *          Returns an array of three numbers representing the current estimated
 *          error in the compass biases. These numbers are unitless and serve
 *          as rough estimates in which numbers less than 100 typically represent
 *          reasonably well calibrated compass axes.
 *
 *  @pre    MLDmpOpen() or MLDmpPedometerStandAloneOpen()
 *          must have been called.
 *
 *  @param  dataSet     
 *              A constant specifying an array of data processed by 
 *              the motion processor.
 *  @param  data        
 *              A pointer to an array to be passed back to the user.
 *              <b>Must be 9 cells long at least</b>.
 *
 *  @return INV_SUCCESS if the command is successful; an error code otherwise.
 */
inv_error_t inv_get_float_array(int dataSet, float *data)
{
    inv_error_t result;
    switch (dataSet) {
    case INV_GYROS:
        result = inv_get_gyro_float(data);
        break;
    case INV_ACCELS:
        result = inv_get_accel_float(data);
        break;
    case INV_TEMPERATURE:
        result = inv_get_temperature_float(data);
        break;
    case INV_ROTATION_MATRIX:
        result = inv_get_rot_mat_float(data);
        break;
    case INV_QUATERNION:
        result = inv_get_quaternion_float(data);
        break;
    case INV_LINEAR_ACCELERATION:
        result = inv_get_linear_accel_float(data);
        break;
    case INV_LINEAR_ACCELERATION_WORLD:
        result = inv_get_linear_accel_in_world_float(data);
        break;
    case INV_GRAVITY:
        result = inv_get_gravity_float(data);
        break;
    case INV_ANGULAR_VELOCITY:
        result = inv_get_angular_velocity_float(data);
        break;
    case INV_EULER_ANGLES:
        result = inv_get_euler_angles_float(data);
        break;
    case INV_EULER_ANGLES_X:
        result = inv_get_euler_angles_x_float(data);
        break;
    case INV_EULER_ANGLES_Y:
        result = inv_get_euler_angles_y_float(data);
        break;
    case INV_EULER_ANGLES_Z:
        result = inv_get_euler_angles_z_float(data);
        break;
    case INV_GYRO_TEMP_SLOPE:
        result = inv_get_gyro_temp_slope_float(data);
        break;
    case INV_GYRO_BIAS:
        result = inv_get_gyro_bias_float(data);
        break;
    case INV_ACCEL_BIAS:
        result = inv_get_accel_bias_float(data);
        break;
    case INV_MAG_BIAS:
        result = inv_get_mag_bias_float(data);
        break;
    case INV_RAW_DATA:
        result = inv_get_gyro_and_accel_sensor_float(data);
        break;
    case INV_MAG_RAW_DATA:
        result = inv_get_mag_raw_data_float(data);
        break;
    case INV_MAGNETOMETER:
        result = inv_get_magnetometer_float(data);
        break;
    case INV_PRESSURE:
        result = inv_get_pressure_float(data);
        break;
    case INV_HEADING:
        result = inv_get_heading_float(data);
        break;
    case INV_GYRO_CALIBRATION_MATRIX:
        result = inv_get_gyro_cal_matrix_float(data);
        break;
    case INV_ACCEL_CALIBRATION_MATRIX:
        result = inv_get_accel_cal_matrix_float(data);
        break;
    case INV_MAG_CALIBRATION_MATRIX:
        result = inv_get_mag_cal_matrix_float(data);
        break;
    case INV_MAG_BIAS_ERROR:
        result = inv_get_mag_bias_error_float(data);
        break;
    case INV_MAG_SCALE:
        result = inv_get_mag_scale_float(data);
        break;
    case INV_LOCAL_FIELD:
        result = inv_get_local_field_float(data);
        break;
    case INV_RELATIVE_QUATERNION:
        result = inv_get_relative_quaternion_float(data);
        break;
    default:
        return INV_ERROR_INVALID_PARAMETER;
        break;
    }
    return result;
}

/**
 *  @brief  used to set an array of motion sensor data.
 *          Handles the following data sets:
 *          - INV_GYRO_BIAS
 *          - INV_ACCEL_BIAS
 *          - INV_MAG_BIAS
 *          - INV_GYRO_TEMP_SLOPE
 *
 *          For more details about the use of the data sets
 *          please refer to the documentation of inv_set_float_array().
 *
 *          Please also refer to the provided "9-Axis Sensor Fusion 
 *          Application Note" document provided.
 *
 *  @pre    MLDmpOpen() or 
 *          MLDmpPedometerStandAloneOpen() 
 *  @pre    MLDmpStart() must <b>NOT</b> have been called.
 *
 *  @param  dataSet     A constant specifying an array of data.
 *  @param  data        A pointer to an array to be copied from the user.
 *
 *  @return INV_SUCCESS if successful; a non-zero error code otherwise.
 */
inv_error_t inv_set_array(int dataSet, long *data)
{
    INVENSENSE_FUNC_START;
    inv_error_t result;
    switch (dataSet) {
    case INV_GYRO_BIAS:
        result = inv_set_gyro_bias(data);
        break;
    case INV_ACCEL_BIAS:
        result = inv_set_accel_bias(data);
        break;
    case INV_MAG_BIAS:
        result = inv_set_mag_bias(data);
        break;
    case INV_GYRO_TEMP_SLOPE:
        result = inv_set_gyro_temp_slope(data);
        break;
    case INV_LOCAL_FIELD:
        result = inv_set_local_field(data);
        break;
    case INV_MAG_SCALE:
        result = inv_set_mag_scale(data);
        break;
    default:
        return INV_ERROR_INVALID_PARAMETER;
        break;
    }
    return result;
}

/**
 *  @brief  used to set an array of motion sensor data.
 *          Handles various data sets:
 *          - INV_GYRO_BIAS
 *          - INV_ACCEL_BIAS
 *          - INV_MAG_BIAS
 *          - INV_GYRO_TEMP_SLOPE
 *
 *          Please refer to the provided "9-Axis Sensor Fusion Application
 *          Note" document provided.
 *
 *  @pre    MLDmpOpen() or 
 *          MLDmpPedometerStandAloneOpen() 
 *  @pre    MLDmpStart() must <b>NOT</b> have been called.
 *
 *  @param  dataSet     A constant specifying an array of data.
 *  @param  data        A pointer to an array to be copied from the user.
 *
 *  @return INV_SUCCESS if successful; a non-zero error code otherwise.
 */
inv_error_t inv_set_float_array(int dataSet, float *data)
{
    INVENSENSE_FUNC_START;
    inv_error_t result;

    switch (dataSet) {
    case INV_GYRO_TEMP_SLOPE:  // internal
        result = inv_set_gyro_temp_slope_float(data);
        break;
    case INV_GYRO_BIAS:        // internal
        result = inv_set_gyro_bias_float(data);
        break;
    case INV_ACCEL_BIAS:       // internal
        result = inv_set_accel_bias_float(data);
        break;
    case INV_MAG_BIAS:         // internal            
        result = inv_set_mag_bias_float(data);
        break;
    case INV_LOCAL_FIELD:      // internal     
        result = inv_set_local_field_float(data);
        break;
    case INV_MAG_SCALE:        // internal            
        result = inv_set_mag_scale_float(data);
        break;
    default:
        result = INV_ERROR_INVALID_PARAMETER;
        break;
    }

    return result;
}
