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
 * $Id: ml_stored_data.c 5641 2011-06-14 02:10:02Z mcaramello $
 *
 *****************************************************************************/

/**
 * @defgroup ML_STORED_DATA
 *
 * @{
 *      @file     ml_stored_data.c
 *      @brief    functions for reading and writing stored data sets.
 *                Typically, these functions process stored calibration data.
 */

#include "ml_stored_data.h"
#include "ml.h"
#include "mlFIFO.h"
#include "mltypes.h"
#include "mlinclude.h"
#include "compass.h"
#include "dmpKey.h"
#include "dmpDefault.h"
#include "mlstates.h"
#include "checksum.h"
#include "mlsupervisor.h"

#include "mlsl.h"
#include "mlos.h"

#include "log.h"
#undef MPL_LOG_TAG
#define MPL_LOG_TAG "MPL-storeload"

/*
    Typedefs
*/
typedef inv_error_t(*tMLLoadFunc) (unsigned char *, unsigned short);

/*
    Globals
*/
extern struct inv_obj_t inv_obj;

/*
    Debugging Definitions
    set LOADCAL_DEBUG and/or STORECAL_DEBUG to 1 print the fields
    being read or stored in human-readable format.
    When set to 0, the compiler will optimize and remove the printouts
    from the code.
*/
#define LOADCAL_DEBUG    0
#define STORECAL_DEBUG   0

#define LOADCAL_LOG(...)                        \
    if(LOADCAL_DEBUG)                           \
        MPL_LOGI("LOADCAL: " __VA_ARGS__)
#define STORECAL_LOG(...)                       \
    if(STORECAL_DEBUG)                          \
        MPL_LOGI("STORECAL: " __VA_ARGS__)

/**
 *  @brief  Duplicate of the inv_temp_comp_find_bin function in the libmpl
 *          advanced algorithms library. To remove cross-dependency, for now,
 *          we reimplement the same function here.
 *  @param  temp
 *              the temperature (1 count == 1 degree C).
 */
int FindTempBin(float temp)
{
    int bin = (int)((temp - MIN_TEMP) / TEMP_PER_BIN);
    if (bin < 0)
        bin = 0;
    if (bin > BINS - 1)
        bin = BINS - 1;
    return bin;
}

/**
 * @brief   Returns the length of the <b>MPL internal calibration data</b>.
 *          Should be called before allocating the memory required to store
 *          this data to a file.
 *          This function returns the total size required to store the cal
 *          data including the header (4 bytes) and the checksum (2 bytes).
 *
 *  @pre    Must be in INV_STATE_DMP_OPENED state.
 *          inv_dmp_open() or inv_dmp_stop() must have been called.
 *          inv_dmp_start() and inv_dmp_close() must have <b>NOT</b>
 *          been called.
 *
 * @return  the length of the internal calibrated data format.
 */
unsigned int inv_get_cal_length(void)
{
    INVENSENSE_FUNC_START;
    unsigned int length;
    length = INV_CAL_HDR_LEN +  // header
        BINS * PTS_PER_BIN * 4 * 4 + BINS * 4 * 2 + // gyro cal
        INV_CAL_ACCEL_LEN +     // accel cal
        INV_CAL_COMPASS_LEN +   // compass cal
        INV_CAL_CHK_LEN;        // checksum
    return length;
}

/**
 *  @brief  Loads a type 0 set of calibration data.
 *          It parses a binary data set containing calibration data.
 *          The binary data set is intended to be loaded from a file.
 *          This calibrations data format stores values for (in order of
 *          appearance) :
 *          - temperature,
 *          - gyro biases for X, Y, Z axes.
 *          This calibration data would normally be produced by the MPU Self
 *          Test and its size is 18 bytes (header and checksum included).
 *          Calibration format type 0 is currently <b>NOT</b> used and
 *          is substituted by type 5 : inv_load_cal_V5().
 *
 *  @note   This calibration data format is obsoleted and no longer supported
 *          by the rest of the MPL
 *
 *  @pre    inv_dmp_open()
 *          @ifnot MPL_MF
 *              or inv_open_low_power_pedometer()
 *              or inv_eis_open_dmp()
 *          @endif
 *          must have been called.
 *
 *  @param  calData
 *              A pointer to an array of bytes to be parsed.
 *  @param  len
 *              the length of the calibration
 *
 *  @return INV_SUCCESS if successful, a non-zero error code otherwise.
 */
inv_error_t inv_load_cal_V0(unsigned char *calData, unsigned short len)
{
    INVENSENSE_FUNC_START;
    const short expLen = 18;
    long newGyroData[3] = { 0, 0, 0 };
    float newTemp;
    int bin;

    LOADCAL_LOG("Entering inv_load_cal_V0\n");

    if (len != expLen) {
        MPL_LOGE("Calibration data type 1 must be %d bytes long\n", expLen);
        return INV_ERROR_FILE_READ;
    }

    newTemp = (float)inv_decode_temperature(
                (unsigned short)calData[6] * 256 + calData[7]) / 65536.f;
    LOADCAL_LOG("newTemp = %f\n", newTemp);

    newGyroData[0] = ((long)calData[8]) * 256 + ((long)calData[9]);
    if (newGyroData[0] > 32767L)
        newGyroData[0] -= 65536L;
    LOADCAL_LOG("newGyroData[0] = %ld\n", newGyroData[0]);
    newGyroData[1] = ((long)calData[10]) * 256 + ((long)calData[11]);
    if (newGyroData[1] > 32767L)
        newGyroData[1] -= 65536L;
    LOADCAL_LOG("newGyroData[2] = %ld\n", newGyroData[2]);
    newGyroData[2] = ((long)calData[12]) * 256 + ((long)calData[13]);
    if (newGyroData[2] > 32767L)
        newGyroData[2] -= 65536L;
    LOADCAL_LOG("newGyroData[2] = %ld\n", newGyroData[2]);

    bin = FindTempBin(newTemp);
    LOADCAL_LOG("bin = %d", bin);

    inv_obj.temp_data[bin][inv_obj.temp_ptrs[bin]] = newTemp;
    LOADCAL_LOG("temp_data[%d][%d] = %f",
                bin, inv_obj.temp_ptrs[bin],
                inv_obj.temp_data[bin][inv_obj.temp_ptrs[bin]]);
    inv_obj.x_gyro_temp_data[bin][inv_obj.temp_ptrs[bin]] =
        ((float)newGyroData[0]) / 65536.f;
    LOADCAL_LOG("x_gyro_temp_data[%d][%d] = %f\n",
                bin, inv_obj.temp_ptrs[bin],
                inv_obj.x_gyro_temp_data[bin][inv_obj.temp_ptrs[bin]]);
    inv_obj.y_gyro_temp_data[bin][inv_obj.temp_ptrs[bin]] =
        ((float)newGyroData[0]) / 65536.f;
    LOADCAL_LOG("y_gyro_temp_data[%d][%d] = %f\n",
                bin, inv_obj.temp_ptrs[bin],
                inv_obj.y_gyro_temp_data[bin][inv_obj.temp_ptrs[bin]]);
    inv_obj.z_gyro_temp_data[bin][inv_obj.temp_ptrs[bin]] =
        ((float)newGyroData[0]) / 65536.f;
    LOADCAL_LOG("z_gyro_temp_data[%d][%d] = %f\n",
                bin, inv_obj.temp_ptrs[bin],
                inv_obj.z_gyro_temp_data[bin][inv_obj.temp_ptrs[bin]]);

    inv_obj.temp_ptrs[bin] = (inv_obj.temp_ptrs[bin] + 1) % PTS_PER_BIN;
    LOADCAL_LOG("temp_ptrs[%d] = %d\n", bin, inv_obj.temp_ptrs[bin]);

    if (inv_obj.temp_ptrs[bin] == 0)
        inv_obj.temp_valid_data[bin] = TRUE;
    LOADCAL_LOG("temp_valid_data[%d] = %ld\n",
                bin, inv_obj.temp_valid_data[bin]);

    inv_obj.got_no_motion_bias = TRUE;
    LOADCAL_LOG("got_no_motion_bias = 1\n");
    inv_obj.cal_loaded_flag = TRUE;
    LOADCAL_LOG("cal_loaded_flag = 1\n");

    LOADCAL_LOG("Exiting inv_load_cal_V0\n");
    return INV_SUCCESS;
}

/**
 *  @brief  Loads a type 1 set of calibration data.
 *          It parses a binary data set containing calibration data.
 *          The binary data set is intended to be loaded from a file.
 *          This calibrations data format stores values for (in order of
 *          appearance) :
 *          - temperature,
 *          - gyro biases for X, Y, Z axes,
 *          - accel biases for X, Y, Z axes.
 *          This calibration data would normally be produced by the MPU Self
 *          Test and its size is 24 bytes (header and checksum included).
 *          Calibration format type 1 is currently <b>NOT</b> used and
 *          is substituted by type 5 : inv_load_cal_V5().
 *
 *  @note   In order to successfully work, the gyro bias must be stored
 *          expressed in 250 dps full scale (131.072 sensitivity). Other full
 *          scale range will produce unpredictable results in the gyro biases.
 *
 *  @pre    inv_dmp_open()
 *          @ifnot MPL_MF
 *              or inv_open_low_power_pedometer()
 *              or inv_eis_open_dmp()
 *          @endif
 *          must have been called.
 *
 *  @param  calData
 *              A pointer to an array of bytes to be parsed.
 *  @param  len
 *              the length of the calibration
 *
 *  @return INV_SUCCESS if successful, a non-zero error code otherwise.
 */
inv_error_t inv_load_cal_V1(unsigned char *calData, unsigned short len)
{
    INVENSENSE_FUNC_START;
    const short expLen = 24;
    long newGyroData[3] = {0, 0, 0};
    long accelBias[3] = {0, 0, 0};
    float gyroBias[3] = {0, 0, 0};
    float newTemp = 0;
    int bin;

    LOADCAL_LOG("Entering inv_load_cal_V1\n");

    if (len != expLen) {
        MPL_LOGE("Calibration data type 1 must be %d bytes long\n", expLen);
        return INV_ERROR_FILE_READ;
    }

    newTemp = (float)inv_decode_temperature(
                (unsigned short)calData[6] * 256 + calData[7]) / 65536.f;
    LOADCAL_LOG("newTemp = %f\n", newTemp);

    newGyroData[0] = ((long)calData[8]) * 256 + ((long)calData[9]);
    if (newGyroData[0] > 32767L)
        newGyroData[0] -= 65536L;
    LOADCAL_LOG("newGyroData[0] = %ld\n", newGyroData[0]);
    newGyroData[1] = ((long)calData[10]) * 256 + ((long)calData[11]);
    if (newGyroData[1] > 32767L)
        newGyroData[1] -= 65536L;
    LOADCAL_LOG("newGyroData[1] = %ld\n", newGyroData[1]);
    newGyroData[2] = ((long)calData[12]) * 256 + ((long)calData[13]);
    if (newGyroData[2] > 32767L)
        newGyroData[2] -= 65536L;
    LOADCAL_LOG("newGyroData[2] = %ld\n", newGyroData[2]);

    bin = FindTempBin(newTemp);
    LOADCAL_LOG("bin = %d\n", bin);

    gyroBias[0] = ((float)newGyroData[0]) / 131.072f;
    gyroBias[1] = ((float)newGyroData[1]) / 131.072f;
    gyroBias[2] = ((float)newGyroData[2]) / 131.072f;
    LOADCAL_LOG("gyroBias[0] = %f\n", gyroBias[0]);
    LOADCAL_LOG("gyroBias[1] = %f\n", gyroBias[1]);
    LOADCAL_LOG("gyroBias[2] = %f\n", gyroBias[2]);

    inv_obj.temp_data[bin][inv_obj.temp_ptrs[bin]] = newTemp;
    LOADCAL_LOG("temp_data[%d][%d] = %f",
                bin, inv_obj.temp_ptrs[bin],
                inv_obj.temp_data[bin][inv_obj.temp_ptrs[bin]]);
    inv_obj.x_gyro_temp_data[bin][inv_obj.temp_ptrs[bin]] = gyroBias[0];
    LOADCAL_LOG("x_gyro_temp_data[%d][%d] = %f\n",
                bin, inv_obj.temp_ptrs[bin],
                inv_obj.x_gyro_temp_data[bin][inv_obj.temp_ptrs[bin]]);
    inv_obj.y_gyro_temp_data[bin][inv_obj.temp_ptrs[bin]] = gyroBias[1];
    LOADCAL_LOG("y_gyro_temp_data[%d][%d] = %f\n",
                bin, inv_obj.temp_ptrs[bin],
                inv_obj.y_gyro_temp_data[bin][inv_obj.temp_ptrs[bin]]);
    inv_obj.z_gyro_temp_data[bin][inv_obj.temp_ptrs[bin]] = gyroBias[2];
    LOADCAL_LOG("z_gyro_temp_data[%d][%d] = %f\n",
                bin, inv_obj.temp_ptrs[bin],
                inv_obj.z_gyro_temp_data[bin][inv_obj.temp_ptrs[bin]]);

    inv_obj.temp_ptrs[bin] = (inv_obj.temp_ptrs[bin] + 1) % PTS_PER_BIN;
    LOADCAL_LOG("temp_ptrs[%d] = %d\n", bin, inv_obj.temp_ptrs[bin]);

    if (inv_obj.temp_ptrs[bin] == 0)
        inv_obj.temp_valid_data[bin] = TRUE;
    LOADCAL_LOG("temp_valid_data[%d] = %ld\n",
                bin, inv_obj.temp_valid_data[bin]);

    /* load accel biases and apply immediately */
    accelBias[0] = ((long)calData[14]) * 256 + ((long)calData[15]);
    if (accelBias[0] > 32767)
        accelBias[0] -= 65536L;
    accelBias[0] = (long)((long long)accelBias[0] * 65536L *
                          inv_obj.accel_sens / 1073741824L);
    LOADCAL_LOG("accelBias[0] = %ld\n", accelBias[0]);
    accelBias[1] = ((long)calData[16]) * 256 + ((long)calData[17]);
    if (accelBias[1] > 32767)
        accelBias[1] -= 65536L;
    accelBias[1] = (long)((long long)accelBias[1] * 65536L *
                          inv_obj.accel_sens / 1073741824L);
    LOADCAL_LOG("accelBias[1] = %ld\n", accelBias[1]);
    accelBias[2] = ((long)calData[18]) * 256 + ((int)calData[19]);
    if (accelBias[2] > 32767)
        accelBias[2] -= 65536L;
    accelBias[2] = (long)((long long)accelBias[2] * 65536L *
                          inv_obj.accel_sens / 1073741824L);
    LOADCAL_LOG("accelBias[2] = %ld\n", accelBias[2]);
    if (inv_set_array(INV_ACCEL_BIAS, accelBias)) {
        LOG_RESULT_LOCATION(inv_set_array(INV_ACCEL_BIAS, accelBias));
        return inv_set_array(INV_ACCEL_BIAS, accelBias);
    }

    inv_obj.got_no_motion_bias = TRUE;
    LOADCAL_LOG("got_no_motion_bias = 1\n");
    inv_obj.cal_loaded_flag = TRUE;
    LOADCAL_LOG("cal_loaded_flag = 1\n");

    LOADCAL_LOG("Exiting inv_load_cal_V1\n");
    return INV_SUCCESS;
}

/**
 *  @brief  Loads a type 2 set of calibration data.
 *          It parses a binary data set containing calibration data.
 *          The binary data set is intended to be loaded from a file.
 *          This calibrations data format stores values for (in order of
 *          appearance) :
 *          - temperature compensation : temperature data points,
 *          - temperature compensation : gyro biases data points for X, Y,
 *              and Z axes.
 *          - accel biases for X, Y, Z axes.
 *          This calibration data is produced internally by the MPL and its
 *          size is 2222 bytes (header and checksum included).
 *          Calibration format type 2 is currently <b>NOT</b> used and
 *          is substituted by type 4 : inv_load_cal_V4().
 *
 *  @pre    inv_dmp_open()
 *          @ifnot MPL_MF
 *              or inv_open_low_power_pedometer()
 *              or inv_eis_open_dmp()
 *          @endif
 *          must have been called.
 *
 *  @param  calData
 *              A pointer to an array of bytes to be parsed.
 *  @param  len
 *              the length of the calibration
 *
 *  @return INV_SUCCESS if successful, a non-zero error code otherwise.
 */
inv_error_t inv_load_cal_V2(unsigned char *calData, unsigned short len)
{
    INVENSENSE_FUNC_START;
    const short expLen = 2222;
    long accel_bias[3];
    int ptr = INV_CAL_HDR_LEN;

    int i, j;
    long long tmp;

    LOADCAL_LOG("Entering inv_load_cal_V2\n");

    if (len != expLen) {
        MPL_LOGE("Calibration data type 2 must be %d bytes long (got %d)\n",
                 expLen, len);
        return INV_ERROR_FILE_READ;
    }

    for (i = 0; i < BINS; i++) {
        inv_obj.temp_ptrs[i] = 0;
        inv_obj.temp_ptrs[i] += 16777216L * ((long)calData[ptr++]);
        inv_obj.temp_ptrs[i] += 65536L * ((long)calData[ptr++]);
        inv_obj.temp_ptrs[i] += 256 * ((int)calData[ptr++]);
        inv_obj.temp_ptrs[i] += (int)calData[ptr++];
        LOADCAL_LOG("temp_ptrs[%d] = %d\n", i, inv_obj.temp_ptrs[i]);
    }
    for (i = 0; i < BINS; i++) {
        inv_obj.temp_valid_data[i] = 0;
        inv_obj.temp_valid_data[i] += 16777216L * ((long)calData[ptr++]);
        inv_obj.temp_valid_data[i] += 65536L * ((long)calData[ptr++]);
        inv_obj.temp_valid_data[i] += 256 * ((int)calData[ptr++]);
        inv_obj.temp_valid_data[i] += (int)calData[ptr++];
        LOADCAL_LOG("temp_valid_data[%d] = %ld\n",
                    i, inv_obj.temp_valid_data[i]);
    }

    for (i = 0; i < BINS; i++) {
        for (j = 0; j < PTS_PER_BIN; j++) {
            tmp = 0;
            tmp += 16777216LL * (long long)calData[ptr++];
            tmp += 65536LL * (long long)calData[ptr++];
            tmp += 256LL * (long long)calData[ptr++];
            tmp += (long long)calData[ptr++];
            if (tmp > 2147483648LL) {
                tmp -= 4294967296LL;
            }
            inv_obj.temp_data[i][j] = ((float)tmp) / 65536.0f;
            LOADCAL_LOG("temp_data[%d][%d] = %f\n",
                        i, j, inv_obj.temp_data[i][j]);
        }

    }
    for (i = 0; i < BINS; i++) {
        for (j = 0; j < PTS_PER_BIN; j++) {
            tmp = 0;
            tmp += 16777216LL * (long long)calData[ptr++];
            tmp += 65536LL * (long long)calData[ptr++];
            tmp += 256LL * (long long)calData[ptr++];
            tmp += (long long)calData[ptr++];
            if (tmp > 2147483648LL) {
                tmp -= 4294967296LL;
            }
            inv_obj.x_gyro_temp_data[i][j] = ((float)tmp) / 65536.0f;
            LOADCAL_LOG("x_gyro_temp_data[%d][%d] = %f\n",
                        i, j, inv_obj.x_gyro_temp_data[i][j]);
        }
    }
    for (i = 0; i < BINS; i++) {
        for (j = 0; j < PTS_PER_BIN; j++) {
            tmp = 0;
            tmp += 16777216LL * (long long)calData[ptr++];
            tmp += 65536LL * (long long)calData[ptr++];
            tmp += 256LL * (long long)calData[ptr++];
            tmp += (long long)calData[ptr++];
            if (tmp > 2147483648LL) {
                tmp -= 4294967296LL;
            }
            inv_obj.y_gyro_temp_data[i][j] = ((float)tmp) / 65536.0f;
            LOADCAL_LOG("y_gyro_temp_data[%d][%d] = %f\n",
                        i, j, inv_obj.y_gyro_temp_data[i][j]);
        }
    }
    for (i = 0; i < BINS; i++) {
        for (j = 0; j < PTS_PER_BIN; j++) {
            tmp = 0;
            tmp += 16777216LL * (long long)calData[ptr++];
            tmp += 65536LL * (long long)calData[ptr++];
            tmp += 256LL * (long long)calData[ptr++];
            tmp += (long long)calData[ptr++];
            if (tmp > 2147483648LL) {
                tmp -= 4294967296LL;
            }
            inv_obj.z_gyro_temp_data[i][j] = ((float)tmp) / 65536.0f;
            LOADCAL_LOG("z_gyro_temp_data[%d][%d] = %f\n",
                        i, j, inv_obj.z_gyro_temp_data[i][j]);
        }
    }

    /* read the accel biases */
    for (i = 0; i < 3; i++) {
        uint32_t t = 0;
        t += 16777216UL * ((uint32_t) calData[ptr++]);
        t += 65536UL * ((uint32_t) calData[ptr++]);
        t += 256u * ((uint32_t) calData[ptr++]);
        t += (uint32_t) calData[ptr++];
        accel_bias[i] = (int32_t) t;
        LOADCAL_LOG("accel_bias[%d] = %ld\n", i, accel_bias[i]);
    }

    if (inv_set_array(INV_ACCEL_BIAS, accel_bias)) {
        LOG_RESULT_LOCATION(inv_set_array(INV_ACCEL_BIAS, accel_bias));
        return inv_set_array(INV_ACCEL_BIAS, accel_bias);
    }

    inv_obj.got_no_motion_bias = TRUE;
    LOADCAL_LOG("got_no_motion_bias = 1\n");
    inv_obj.cal_loaded_flag = TRUE;
    LOADCAL_LOG("cal_loaded_flag = 1\n");

    LOADCAL_LOG("Exiting inv_load_cal_V2\n");
    return INV_SUCCESS;
}

/**
 *  @brief  Loads a type 3 set of calibration data.
 *          It parses a binary data set containing calibration data.
 *          The binary data set is intended to be loaded from a file.
 *          This calibrations data format stores values for (in order of
 *          appearance) :
 *          - temperature compensation : temperature data points,
 *          - temperature compensation : gyro biases data points for X, Y,
 *              and Z axes.
 *          - accel biases for X, Y, Z axes.
 *          - compass biases for X, Y, Z axes and bias tracking algorithm
 *              mock-up.
 *          This calibration data is produced internally by the MPL and its
 *          size is 2429 bytes (header and checksum included).
 *          Calibration format type 3 is currently <b>NOT</b> used and
 *          is substituted by type 4 : inv_load_cal_V4().

 *  @pre    inv_dmp_open()
 *          @ifnot MPL_MF
 *              or inv_open_low_power_pedometer()
 *              or inv_eis_open_dmp()
 *          @endif
 *          must have been called.
 *
 *  @param  calData
 *              A pointer to an array of bytes to be parsed.
 *  @param  len
 *              the length of the calibration
 *
 *  @return INV_SUCCESS if successful, a non-zero error code otherwise.
 */
inv_error_t inv_load_cal_V3(unsigned char *calData, unsigned short len)
{
    INVENSENSE_FUNC_START;
    union doubleToLongLong {
        double db;
        unsigned long long ll;
    } dToLL;

    const short expLen = 2429;
    long bias[3];
    int i, j;
    int ptr = INV_CAL_HDR_LEN;
    long long tmp;

    LOADCAL_LOG("Entering inv_load_cal_V3\n");

    if (len != expLen) {
        MPL_LOGE("Calibration data type 3 must be %d bytes long (got %d)\n",
                 expLen, len);
        return INV_ERROR_FILE_READ;
    }

    for (i = 0; i < BINS; i++) {
        inv_obj.temp_ptrs[i] = 0;
        inv_obj.temp_ptrs[i] += 16777216L * ((long)calData[ptr++]);
        inv_obj.temp_ptrs[i] += 65536L * ((long)calData[ptr++]);
        inv_obj.temp_ptrs[i] += 256 * ((int)calData[ptr++]);
        inv_obj.temp_ptrs[i] += (int)calData[ptr++];
        LOADCAL_LOG("temp_ptrs[%d] = %d\n", i, inv_obj.temp_ptrs[i]);
    }
    for (i = 0; i < BINS; i++) {
        inv_obj.temp_valid_data[i] = 0;
        inv_obj.temp_valid_data[i] += 16777216L * ((long)calData[ptr++]);
        inv_obj.temp_valid_data[i] += 65536L * ((long)calData[ptr++]);
        inv_obj.temp_valid_data[i] += 256 * ((int)calData[ptr++]);
        inv_obj.temp_valid_data[i] += (int)calData[ptr++];
        LOADCAL_LOG("temp_valid_data[%d] = %ld\n",
                    i, inv_obj.temp_valid_data[i]);
    }

    for (i = 0; i < BINS; i++) {
        for (j = 0; j < PTS_PER_BIN; j++) {
            tmp = 0;
            tmp += 16777216LL * (long long)calData[ptr++];
            tmp += 65536LL * (long long)calData[ptr++];
            tmp += 256LL * (long long)calData[ptr++];
            tmp += (long long)calData[ptr++];
            if (tmp > 2147483648LL) {
                tmp -= 4294967296LL;
            }
            inv_obj.temp_data[i][j] = ((float)tmp) / 65536.0f;
            LOADCAL_LOG("temp_data[%d][%d] = %f\n",
                        i, j, inv_obj.temp_data[i][j]);
        }
    }

    for (i = 0; i < BINS; i++) {
        for (j = 0; j < PTS_PER_BIN; j++) {
            tmp = 0;
            tmp += 16777216LL * (long long)calData[ptr++];
            tmp += 65536LL * (long long)calData[ptr++];
            tmp += 256LL * (long long)calData[ptr++];
            tmp += (long long)calData[ptr++];
            if (tmp > 2147483648LL) {
                tmp -= 4294967296LL;
            }
            inv_obj.x_gyro_temp_data[i][j] = ((float)tmp) / 65536.0f;
            LOADCAL_LOG("x_gyro_temp_data[%d][%d] = %f\n",
                        i, j, inv_obj.x_gyro_temp_data[i][j]);
        }
    }
    for (i = 0; i < BINS; i++) {
        for (j = 0; j < PTS_PER_BIN; j++) {
            tmp = 0;
            tmp += 16777216LL * (long long)calData[ptr++];
            tmp += 65536LL * (long long)calData[ptr++];
            tmp += 256LL * (long long)calData[ptr++];
            tmp += (long long)calData[ptr++];
            if (tmp > 2147483648LL) {
                tmp -= 4294967296LL;
            }
            inv_obj.y_gyro_temp_data[i][j] = ((float)tmp) / 65536.0f;
            LOADCAL_LOG("y_gyro_temp_data[%d][%d] = %f\n",
                        i, j, inv_obj.y_gyro_temp_data[i][j]);
        }
    }
    for (i = 0; i < BINS; i++) {
        for (j = 0; j < PTS_PER_BIN; j++) {
            tmp = 0;
            tmp += 16777216LL * (long long)calData[ptr++];
            tmp += 65536LL * (long long)calData[ptr++];
            tmp += 256LL * (long long)calData[ptr++];
            tmp += (long long)calData[ptr++];
            if (tmp > 2147483648LL) {
                tmp -= 4294967296LL;
            }
            inv_obj.z_gyro_temp_data[i][j] = ((float)tmp) / 65536.0f;
            LOADCAL_LOG("z_gyro_temp_data[%d][%d] = %f\n",
                        i, j, inv_obj.z_gyro_temp_data[i][j]);
        }
    }

    /* read the accel biases */
    for (i = 0; i < 3; i++) {
        uint32_t t = 0;
        t += 16777216UL * ((uint32_t) calData[ptr++]);
        t += 65536UL * ((uint32_t) calData[ptr++]);
        t += 256u * ((uint32_t) calData[ptr++]);
        t += (uint32_t) calData[ptr++];
        bias[i] = (int32_t) t;
        LOADCAL_LOG("accel_bias[%d] = %ld\n", i, bias[i]);
    }
    if (inv_set_array(INV_ACCEL_BIAS, bias)) {
        LOG_RESULT_LOCATION(inv_set_array(INV_ACCEL_BIAS, bias));
        return inv_set_array(INV_ACCEL_BIAS, bias);
    }

    /* read the compass biases */
    inv_obj.got_compass_bias = (int)calData[ptr++];
    inv_obj.got_init_compass_bias = (int)calData[ptr++];
    inv_obj.compass_state = (int)calData[ptr++];

    for (i = 0; i < 3; i++) {
        uint32_t t = 0;
        t += 16777216UL * ((uint32_t) calData[ptr++]);
        t += 65536UL * ((uint32_t) calData[ptr++]);
        t += 256u * ((uint32_t) calData[ptr++]);
        t += (uint32_t) calData[ptr++];
        inv_obj.compass_bias_error[i] = (int32_t) t;
        LOADCAL_LOG("compass_bias_error[%d] = %ld\n", i,
                    inv_obj.compass_bias_error[i]);
    }
    for (i = 0; i < 3; i++) {
        uint32_t t = 0;
        t += 16777216UL * ((uint32_t) calData[ptr++]);
        t += 65536UL * ((uint32_t) calData[ptr++]);
        t += 256u * ((uint32_t) calData[ptr++]);
        t += (uint32_t) calData[ptr++];
        inv_obj.init_compass_bias[i] = (int32_t) t;
        LOADCAL_LOG("init_compass_bias[%d] = %ld\n", i,
                    inv_obj.init_compass_bias[i]);
    }
    for (i = 0; i < 3; i++) {
        uint32_t t = 0;
        t += 16777216UL * ((uint32_t) calData[ptr++]);
        t += 65536UL * ((uint32_t) calData[ptr++]);
        t += 256u * ((uint32_t) calData[ptr++]);
        t += (uint32_t) calData[ptr++];
        inv_obj.compass_bias[i] = (int32_t) t;
        LOADCAL_LOG("compass_bias[%d] = %ld\n", i, inv_obj.compass_bias[i]);
    }
    for (i = 0; i < 18; i++) {
        uint32_t t = 0;
        t += 16777216UL * ((uint32_t) calData[ptr++]);
        t += 65536UL * ((uint32_t) calData[ptr++]);
        t += 256u * ((uint32_t) calData[ptr++]);
        t += (uint32_t) calData[ptr++];
        inv_obj.compass_peaks[i] = (int32_t) t;
        LOADCAL_LOG("compass_peaks[%d] = %d\n", i, inv_obj.compass_peaks[i]);
    }
    for (i = 0; i < 3; i++) {
        dToLL.ll = 0;
        dToLL.ll += 72057594037927936ULL * ((unsigned long long)calData[ptr++]);
        dToLL.ll += 281474976710656ULL * ((unsigned long long)calData[ptr++]);
        dToLL.ll += 1099511627776ULL * ((unsigned long long)calData[ptr++]);
        dToLL.ll += 4294967296LL * ((unsigned long long)calData[ptr++]);
        dToLL.ll += 16777216ULL * ((unsigned long long)calData[ptr++]);
        dToLL.ll += 65536ULL * ((unsigned long long)calData[ptr++]);
        dToLL.ll += 256LL * ((unsigned long long)calData[ptr++]);
        dToLL.ll += (unsigned long long)calData[ptr++];

        inv_obj.compass_bias_v[i] = dToLL.db;
        LOADCAL_LOG("compass_bias_v[%d] = %lf\n", i, inv_obj.compass_bias_v[i]);
    }
    for (i = 0; i < 9; i++) {
        dToLL.ll = 0;
        dToLL.ll += 72057594037927936ULL * ((unsigned long long)calData[ptr++]);
        dToLL.ll += 281474976710656ULL * ((unsigned long long)calData[ptr++]);
        dToLL.ll += 1099511627776ULL * ((unsigned long long)calData[ptr++]);
        dToLL.ll += 4294967296LL * ((unsigned long long)calData[ptr++]);
        dToLL.ll += 16777216ULL * ((unsigned long long)calData[ptr++]);
        dToLL.ll += 65536ULL * ((unsigned long long)calData[ptr++]);
        dToLL.ll += 256LL * ((unsigned long long)calData[ptr++]);
        dToLL.ll += (unsigned long long)calData[ptr++];

        inv_obj.compass_bias_ptr[i] = dToLL.db;
        LOADCAL_LOG("compass_bias_ptr[%d] = %lf\n", i,
                    inv_obj.compass_bias_ptr[i]);
    }

    inv_obj.got_no_motion_bias = TRUE;
    LOADCAL_LOG("got_no_motion_bias = 1\n");
    inv_obj.cal_loaded_flag = TRUE;
    LOADCAL_LOG("cal_loaded_flag = 1\n");

    LOADCAL_LOG("Exiting inv_load_cal_V3\n");
    return INV_SUCCESS;
}

/**
 *  @brief  Loads a type 4 set of calibration data.
 *          It parses a binary data set containing calibration data.
 *          The binary data set is intended to be loaded from a file.
 *          This calibrations data format stores values for (in order of
 *          appearance) :
 *          - temperature compensation : temperature data points,
 *          - temperature compensation : gyro biases data points for X, Y,
 *              and Z axes.
 *          - accel biases for X, Y, Z axes.
 *          - compass biases for X, Y, Z axes, compass scale, and bias
 *              tracking algorithm  mock-up.
 *          This calibration data is produced internally by the MPL and its
 *          size is 2777 bytes (header and checksum included).
 *          Calibration format type 4 is currently used and
 *          substitutes type 2 (inv_load_cal_V2()) and 3 (inv_load_cal_V3()).
 *
 *  @pre    inv_dmp_open()
 *          @ifnot MPL_MF
 *              or inv_open_low_power_pedometer()
 *              or inv_eis_open_dmp()
 *          @endif
 *          must have been called.
 *
 *  @param  calData
 *              A pointer to an array of bytes to be parsed.
 *  @param  len
 *              the length of the calibration
 *
 *  @return INV_SUCCESS if successful, a non-zero error code otherwise.
 */
inv_error_t inv_load_cal_V4(unsigned char *calData, unsigned short len)
{
    INVENSENSE_FUNC_START;
    union doubleToLongLong {
        double db;
        unsigned long long ll;
    } dToLL;

    const unsigned int expLen = 2782;
    long bias[3];
    int ptr = INV_CAL_HDR_LEN;
    int i, j;
    long long tmp;
    inv_error_t result;

    LOADCAL_LOG("Entering inv_load_cal_V4\n");

    if (len != expLen) {
        MPL_LOGE("Calibration data type 4 must be %d bytes long (got %d)\n",
                 expLen, len);
        return INV_ERROR_FILE_READ;
    }

    for (i = 0; i < BINS; i++) {
        inv_obj.temp_ptrs[i] = 0;
        inv_obj.temp_ptrs[i] += 16777216L * ((long)calData[ptr++]);
        inv_obj.temp_ptrs[i] += 65536L * ((long)calData[ptr++]);
        inv_obj.temp_ptrs[i] += 256 * ((int)calData[ptr++]);
        inv_obj.temp_ptrs[i] += (int)calData[ptr++];
        LOADCAL_LOG("temp_ptrs[%d] = %d\n", i, inv_obj.temp_ptrs[i]);
    }
    for (i = 0; i < BINS; i++) {
        inv_obj.temp_valid_data[i] = 0;
        inv_obj.temp_valid_data[i] += 16777216L * ((long)calData[ptr++]);
        inv_obj.temp_valid_data[i] += 65536L * ((long)calData[ptr++]);
        inv_obj.temp_valid_data[i] += 256 * ((int)calData[ptr++]);
        inv_obj.temp_valid_data[i] += (int)calData[ptr++];
        LOADCAL_LOG("temp_valid_data[%d] = %ld\n",
                    i, inv_obj.temp_valid_data[i]);
    }

    for (i = 0; i < BINS; i++) {
        for (j = 0; j < PTS_PER_BIN; j++) {
            tmp = 0;
            tmp += 16777216LL * (long long)calData[ptr++];
            tmp += 65536LL * (long long)calData[ptr++];
            tmp += 256LL * (long long)calData[ptr++];
            tmp += (long long)calData[ptr++];
            if (tmp > 2147483648LL) {
                tmp -= 4294967296LL;
            }
            inv_obj.temp_data[i][j] = ((float)tmp) / 65536.0f;
            LOADCAL_LOG("temp_data[%d][%d] = %f\n",
                        i, j, inv_obj.temp_data[i][j]);
        }
    }

    for (i = 0; i < BINS; i++) {
        for (j = 0; j < PTS_PER_BIN; j++) {
            tmp = 0;
            tmp += 16777216LL * (long long)calData[ptr++];
            tmp += 65536LL * (long long)calData[ptr++];
            tmp += 256LL * (long long)calData[ptr++];
            tmp += (long long)calData[ptr++];
            if (tmp > 2147483648LL) {
                tmp -= 4294967296LL;
            }
            inv_obj.x_gyro_temp_data[i][j] = ((float)tmp) / 65536.0f;
            LOADCAL_LOG("x_gyro_temp_data[%d][%d] = %f\n",
                        i, j, inv_obj.x_gyro_temp_data[i][j]);
        }
    }
    for (i = 0; i < BINS; i++) {
        for (j = 0; j < PTS_PER_BIN; j++) {
            tmp = 0;
            tmp += 16777216LL * (long long)calData[ptr++];
            tmp += 65536LL * (long long)calData[ptr++];
            tmp += 256LL * (long long)calData[ptr++];
            tmp += (long long)calData[ptr++];
            if (tmp > 2147483648LL) {
                tmp -= 4294967296LL;
            }
            inv_obj.y_gyro_temp_data[i][j] = ((float)tmp) / 65536.0f;
            LOADCAL_LOG("y_gyro_temp_data[%d][%d] = %f\n",
                        i, j, inv_obj.y_gyro_temp_data[i][j]);
        }
    }
    for (i = 0; i < BINS; i++) {
        for (j = 0; j < PTS_PER_BIN; j++) {
            tmp = 0;
            tmp += 16777216LL * (long long)calData[ptr++];
            tmp += 65536LL * (long long)calData[ptr++];
            tmp += 256LL * (long long)calData[ptr++];
            tmp += (long long)calData[ptr++];
            if (tmp > 2147483648LL) {
                tmp -= 4294967296LL;
            }
            inv_obj.z_gyro_temp_data[i][j] = ((float)tmp) / 65536.0f;
            LOADCAL_LOG("z_gyro_temp_data[%d][%d] = %f\n",
                        i, j, inv_obj.z_gyro_temp_data[i][j]);
        }
    }

    /* read the accel biases */
    for (i = 0; i < 3; i++) {
        uint32_t t = 0;
        t += 16777216UL * ((uint32_t) calData[ptr++]);
        t += 65536UL * ((uint32_t) calData[ptr++]);
        t += 256u * ((uint32_t) calData[ptr++]);
        t += (uint32_t) calData[ptr++];
        bias[i] = (int32_t) t;
        LOADCAL_LOG("accel_bias[%d] = %ld\n", i, bias[i]);
    }
    if (inv_set_array(INV_ACCEL_BIAS, bias)) {
        LOG_RESULT_LOCATION(inv_set_array(INV_ACCEL_BIAS, bias));
        return inv_set_array(INV_ACCEL_BIAS, bias);
    }

    /* read the compass biases */
    inv_reset_compass_calibration();

    inv_obj.got_compass_bias = (int)calData[ptr++];
    LOADCAL_LOG("got_compass_bias = %ld\n", inv_obj.got_compass_bias);
    inv_obj.got_init_compass_bias = (int)calData[ptr++];
    LOADCAL_LOG("got_init_compass_bias = %d\n", inv_obj.got_init_compass_bias);
    inv_obj.compass_state = (int)calData[ptr++];
    LOADCAL_LOG("compass_state = %ld\n", inv_obj.compass_state);

    for (i = 0; i < 3; i++) {
        uint32_t t = 0;
        t += 16777216UL * ((uint32_t) calData[ptr++]);
        t += 65536UL * ((uint32_t) calData[ptr++]);
        t += 256u * ((uint32_t) calData[ptr++]);
        t += (uint32_t) calData[ptr++];
        inv_obj.compass_bias_error[i] = (int32_t) t;
        LOADCAL_LOG("compass_bias_error[%d] = %ld\n", i,
                    inv_obj.compass_bias_error[i]);
    }
    for (i = 0; i < 3; i++) {
        uint32_t t = 0;
        t += 16777216UL * ((uint32_t) calData[ptr++]);
        t += 65536UL * ((uint32_t) calData[ptr++]);
        t += 256u * ((uint32_t) calData[ptr++]);
        t += (uint32_t) calData[ptr++];
        inv_obj.init_compass_bias[i] = (int32_t) t;
        LOADCAL_LOG("init_compass_bias[%d] = %ld\n", i,
                    inv_obj.init_compass_bias[i]);
    }
    for (i = 0; i < 3; i++) {
        uint32_t t = 0;
        t += 16777216UL * ((uint32_t) calData[ptr++]);
        t += 65536UL * ((uint32_t) calData[ptr++]);
        t += 256u * ((uint32_t) calData[ptr++]);
        t += (uint32_t) calData[ptr++];
        inv_obj.compass_bias[i] = (int32_t) t;
        LOADCAL_LOG("compass_bias[%d] = %ld\n", i, inv_obj.compass_bias[i]);
    }
    for (i = 0; i < 18; i++) {
        uint32_t t = 0;
        t += 16777216UL * ((uint32_t) calData[ptr++]);
        t += 65536UL * ((uint32_t) calData[ptr++]);
        t += 256u * ((uint32_t) calData[ptr++]);
        t += (uint32_t) calData[ptr++];
        inv_obj.compass_peaks[i] = (int32_t) t;
        LOADCAL_LOG("compass_peaks[%d] = %d\n", i, inv_obj.compass_peaks[i]);
    }
    for (i = 0; i < 3; i++) {
        dToLL.ll = 72057594037927936ULL * ((unsigned long long)calData[ptr++]);
        dToLL.ll += 281474976710656ULL * ((unsigned long long)calData[ptr++]);
        dToLL.ll += 1099511627776ULL * ((unsigned long long)calData[ptr++]);
        dToLL.ll += 4294967296LL * ((unsigned long long)calData[ptr++]);
        dToLL.ll += 16777216ULL * ((unsigned long long)calData[ptr++]);
        dToLL.ll += 65536ULL * ((unsigned long long)calData[ptr++]);
        dToLL.ll += 256LL * ((unsigned long long)calData[ptr++]);
        dToLL.ll += (unsigned long long)calData[ptr++];

        inv_obj.compass_bias_v[i] = dToLL.db;
        LOADCAL_LOG("compass_bias_v[%d] = %lf\n", i, inv_obj.compass_bias_v[i]);
    }
    for (i = 0; i < 9; i++) {
        dToLL.ll = 72057594037927936ULL * ((unsigned long long)calData[ptr++]);
        dToLL.ll += 281474976710656ULL * ((unsigned long long)calData[ptr++]);
        dToLL.ll += 1099511627776ULL * ((unsigned long long)calData[ptr++]);
        dToLL.ll += 4294967296LL * ((unsigned long long)calData[ptr++]);
        dToLL.ll += 16777216ULL * ((unsigned long long)calData[ptr++]);
        dToLL.ll += 65536ULL * ((unsigned long long)calData[ptr++]);
        dToLL.ll += 256LL * ((unsigned long long)calData[ptr++]);
        dToLL.ll += (unsigned long long)calData[ptr++];

        inv_obj.compass_bias_ptr[i] = dToLL.db;
        LOADCAL_LOG("compass_bias_ptr[%d] = %lf\n", i,
                    inv_obj.compass_bias_ptr[i]);
    }
    for (i = 0; i < 3; i++) {
        uint32_t t = 0;
        t += 16777216UL * ((uint32_t) calData[ptr++]);
        t += 65536UL * ((uint32_t) calData[ptr++]);
        t += 256u * ((uint32_t) calData[ptr++]);
        t += (uint32_t) calData[ptr++];
        inv_obj.compass_scale[i] = (int32_t) t;
        LOADCAL_LOG("compass_scale[%d] = %d\n", i, (int32_t) t);
    }
    for (i = 0; i < 6; i++) {
        dToLL.ll = 72057594037927936ULL * ((unsigned long long)calData[ptr++]);
        dToLL.ll += 281474976710656ULL * ((unsigned long long)calData[ptr++]);
        dToLL.ll += 1099511627776ULL * ((unsigned long long)calData[ptr++]);
        dToLL.ll += 4294967296LL * ((unsigned long long)calData[ptr++]);
        dToLL.ll += 16777216ULL * ((unsigned long long)calData[ptr++]);
        dToLL.ll += 65536ULL * ((unsigned long long)calData[ptr++]);
        dToLL.ll += 256LL * ((unsigned long long)calData[ptr++]);
        dToLL.ll += (unsigned long long)calData[ptr++];

        inv_obj.compass_prev_xty[i] = dToLL.db;
        LOADCAL_LOG("compass_prev_xty[%d] = %f\n", i, dToLL.db);
    }
    for (i = 0; i < 36; i++) {
        dToLL.ll = 72057594037927936ULL * ((unsigned long long)calData[ptr++]);
        dToLL.ll += 281474976710656ULL * ((unsigned long long)calData[ptr++]);
        dToLL.ll += 1099511627776ULL * ((unsigned long long)calData[ptr++]);
        dToLL.ll += 4294967296LL * ((unsigned long long)calData[ptr++]);
        dToLL.ll += 16777216ULL * ((unsigned long long)calData[ptr++]);
        dToLL.ll += 65536ULL * ((unsigned long long)calData[ptr++]);
        dToLL.ll += 256LL * ((unsigned long long)calData[ptr++]);
        dToLL.ll += (unsigned long long)calData[ptr++];

        inv_obj.compass_prev_m[i] = dToLL.db;
        LOADCAL_LOG("compass_prev_m[%d] = %f\n", i, dToLL.db);
    }

    /* Load the compass offset flag and values */
    inv_obj.flags[INV_COMPASS_OFFSET_VALID] = calData[ptr++];
    inv_obj.compass_offsets[0] = calData[ptr++];
    inv_obj.compass_offsets[1] = calData[ptr++];
    inv_obj.compass_offsets[2] = calData[ptr++];

    inv_obj.compass_accuracy = calData[ptr++];
    /* push the compass offset values to the device */
    result = inv_set_compass_offset();

    if (result == INV_SUCCESS) {
        if (inv_compass_check_range() != INV_SUCCESS) {
            MPL_LOGI("range check fail");
            inv_reset_compass_calibration();
            inv_obj.flags[INV_COMPASS_OFFSET_VALID] = 0;
            inv_set_compass_offset();
        }
    }

    inv_obj.got_no_motion_bias = TRUE;
    LOADCAL_LOG("got_no_motion_bias = 1\n");
    inv_obj.cal_loaded_flag = TRUE;
    LOADCAL_LOG("cal_loaded_flag = 1\n");

    LOADCAL_LOG("Exiting inv_load_cal_V4\n");
    return INV_SUCCESS;
}

/**
 *  @brief  Loads a type 5 set of calibration data.
 *          It parses a binary data set containing calibration data.
 *          The binary data set is intended to be loaded from a file.
 *          This calibrations data format stores values for (in order of
 *          appearance) :
 *          - temperature,
 *          - gyro biases for X, Y, Z axes,
 *          - accel biases for X, Y, Z axes.
 *          This calibration data would normally be produced by the MPU Self
 *          Test and its size is 36 bytes (header and checksum included).
 *          Calibration format type 5 is produced by the MPU Self Test and
 *          substitutes the type 1 : inv_load_cal_V1().
 *
 *  @pre    inv_dmp_open()
 *          @ifnot MPL_MF
 *              or inv_open_low_power_pedometer()
 *              or inv_eis_open_dmp()
 *          @endif
 *          must have been called.
 *
 *  @param  calData
 *              A pointer to an array of bytes to be parsed.
 *  @param  len
 *              the length of the calibration
 *
 *  @return INV_SUCCESS if successful, a non-zero error code otherwise.
 */
inv_error_t inv_load_cal_V5(unsigned char *calData, unsigned short len)
{
    INVENSENSE_FUNC_START;
    const short expLen = 36;
    long accelBias[3] = { 0, 0, 0 };
    float gyroBias[3] = { 0, 0, 0 };

    int ptr = INV_CAL_HDR_LEN;
    unsigned short temp;
    float newTemp;
    int bin;
    int i;

    LOADCAL_LOG("Entering inv_load_cal_V5\n");

    if (len != expLen) {
        MPL_LOGE("Calibration data type 5 must be %d bytes long (got %d)\n",
                 expLen, len);
        return INV_ERROR_FILE_READ;
    }

    /* load the temperature */
    temp = (unsigned short)calData[ptr++] * (1L << 8);
    temp += calData[ptr++];
    newTemp = (float)inv_decode_temperature(temp) / 65536.f;
    LOADCAL_LOG("newTemp = %f\n", newTemp);

    /* load the gyro biases (represented in 2 ^ 16 == 1 dps) */
    for (i = 0; i < 3; i++) {
        int32_t tmp = 0;
        tmp += (int32_t) calData[ptr++] * (1L << 24);
        tmp += (int32_t) calData[ptr++] * (1L << 16);
        tmp += (int32_t) calData[ptr++] * (1L << 8);
        tmp += (int32_t) calData[ptr++];
        gyroBias[i] = (float)tmp / 65536.0f;
        LOADCAL_LOG("gyroBias[%d] = %f\n", i, gyroBias[i]);
    }
    /* find the temperature bin */
    bin = FindTempBin(newTemp);

    /* populate the temp comp data structure */
    inv_obj.temp_data[bin][inv_obj.temp_ptrs[bin]] = newTemp;
    LOADCAL_LOG("temp_data[%d][%d] = %f\n",
                bin, inv_obj.temp_ptrs[bin], newTemp);

    inv_obj.x_gyro_temp_data[bin][inv_obj.temp_ptrs[bin]] = gyroBias[0];
    LOADCAL_LOG("x_gyro_temp_data[%d][%d] = %f\n",
                bin, inv_obj.temp_ptrs[bin], gyroBias[0]);
    inv_obj.y_gyro_temp_data[bin][inv_obj.temp_ptrs[bin]] = gyroBias[1];
    LOADCAL_LOG("y_gyro_temp_data[%d][%d] = %f\n",
                bin, inv_obj.temp_ptrs[bin], gyroBias[1]);
    inv_obj.z_gyro_temp_data[bin][inv_obj.temp_ptrs[bin]] = gyroBias[2];
    LOADCAL_LOG("z_gyro_temp_data[%d][%d] = %f\n",
                bin, inv_obj.temp_ptrs[bin], gyroBias[2]);
    inv_obj.temp_ptrs[bin] = (inv_obj.temp_ptrs[bin] + 1) % PTS_PER_BIN;
    LOADCAL_LOG("temp_ptrs[%d] = %d\n", bin, inv_obj.temp_ptrs[bin]);

    if (inv_obj.temp_ptrs[bin] == 0)
        inv_obj.temp_valid_data[bin] = TRUE;
    LOADCAL_LOG("temp_valid_data[%d] = %ld\n",
                bin, inv_obj.temp_valid_data[bin]);

    /* load accel biases (represented in 2 ^ 16 == 1 gee)
       and apply immediately */
    for (i = 0; i < 3; i++) {
        int32_t tmp = 0;
        tmp += (int32_t) calData[ptr++] * (1L << 24);
        tmp += (int32_t) calData[ptr++] * (1L << 16);
        tmp += (int32_t) calData[ptr++] * (1L << 8);
        tmp += (int32_t) calData[ptr++];
        accelBias[i] = (long)tmp;
        LOADCAL_LOG("accelBias[%d] = %ld\n", i, accelBias[i]);
    }
    if (inv_set_array(INV_ACCEL_BIAS, accelBias)) {
        LOG_RESULT_LOCATION(inv_set_array(INV_ACCEL_BIAS, accelBias));
        return inv_set_array(INV_ACCEL_BIAS, accelBias);
    }

    inv_obj.got_no_motion_bias = TRUE;
    LOADCAL_LOG("got_no_motion_bias = 1\n");
    inv_obj.cal_loaded_flag = TRUE;
    LOADCAL_LOG("cal_loaded_flag = 1\n");

    LOADCAL_LOG("Exiting inv_load_cal_V5\n");
    return INV_SUCCESS;
}

/**
 * @brief   Loads a set of calibration data.
 *          It parses a binary data set containing calibration data.
 *          The binary data set is intended to be loaded from a file.
 *
 * @pre     inv_dmp_open()
 *          @ifnot MPL_MF
 *              or inv_open_low_power_pedometer()
 *              or inv_eis_open_dmp()
 *          @endif
 *          must have been called.
 *
 * @param   calData
 *              A pointer to an array of bytes to be parsed.
 *
 * @return  INV_SUCCESS if successful, a non-zero error code otherwise.
 */
inv_error_t inv_load_cal(unsigned char *calData)
{
    INVENSENSE_FUNC_START;
    int calType = 0;
    int len = 0;
    int ptr;
    uint32_t chk = 0;
    uint32_t cmp_chk = 0;

    tMLLoadFunc loaders[] = {
        inv_load_cal_V0,
        inv_load_cal_V1,
        inv_load_cal_V2,
        inv_load_cal_V3,
        inv_load_cal_V4,
        inv_load_cal_V5
    };

    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    /* read the header (type and len)
       len is the total record length including header and checksum */
    len = 0;
    len += 16777216L * ((int)calData[0]);
    len += 65536L * ((int)calData[1]);
    len += 256 * ((int)calData[2]);
    len += (int)calData[3];

    calType = ((int)calData[4]) * 256 + ((int)calData[5]);
    if (calType > 5) {
        MPL_LOGE("Unsupported calibration file format %d. "
                 "Valid types 0..5\n", calType);
        return INV_ERROR_INVALID_PARAMETER;
    }

    /* check the checksum */
    chk = 0;
    ptr = len - INV_CAL_CHK_LEN;

    chk += 16777216L * ((uint32_t) calData[ptr++]);
    chk += 65536L * ((uint32_t) calData[ptr++]);
    chk += 256 * ((uint32_t) calData[ptr++]);
    chk += (uint32_t) calData[ptr++];

    cmp_chk = inv_checksum(calData + INV_CAL_HDR_LEN,
                           len - (INV_CAL_HDR_LEN + INV_CAL_CHK_LEN));

    if (chk != cmp_chk) {
        return INV_ERROR_CALIBRATION_CHECKSUM;
    }

    /* call the proper method to read in the data */
    return loaders[calType] (calData, len);
}

/**
 *  @brief  Stores a set of calibration data.
 *          It generates a binary data set containing calibration data.
 *          The binary data set is intended to be stored into a file.
 *
 *  @pre    inv_dmp_open()
 *
 *  @param  calData
 *              A pointer to an array of bytes to be stored.
 *  @param  length
 *              The amount of bytes available in the array.
 *
 *  @return INV_SUCCESS if successful, a non-zero error code otherwise.
 */
inv_error_t inv_store_cal(unsigned char *calData, int length)
{
    INVENSENSE_FUNC_START;
    int ptr = 0;
    int i = 0;
    int j = 0;
    long long tmp;
    uint32_t chk;
    long bias[3];
    //unsigned char state;
    union doubleToLongLong {
        double db;
        unsigned long long ll;
    } dToLL;

    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    STORECAL_LOG("Entering inv_store_cal\n");

    // length
    calData[0] = (unsigned char)((length >> 24) & 0xff);
    calData[1] = (unsigned char)((length >> 16) & 0xff);
    calData[2] = (unsigned char)((length >> 8) & 0xff);
    calData[3] = (unsigned char)(length & 0xff);
    STORECAL_LOG("calLen = %d\n", length);

    // calibration data format type
    calData[4] = 0;
    calData[5] = 4;
    STORECAL_LOG("calType = %d\n", (int)calData[4] * 256 + calData[5]);

    // data
    ptr = 6;
    for (i = 0; i < BINS; i++) {
        tmp = (int)inv_obj.temp_ptrs[i];
        calData[ptr++] = (unsigned char)((tmp >> 24) & 0xff);
        calData[ptr++] = (unsigned char)((tmp >> 16) & 0xff);
        calData[ptr++] = (unsigned char)((tmp >> 8) & 0xff);
        calData[ptr++] = (unsigned char)(tmp & 0xff);
        STORECAL_LOG("temp_ptrs[%d] = %lld\n", i, tmp);
    }

    for (i = 0; i < BINS; i++) {
        tmp = (int)inv_obj.temp_valid_data[i];
        calData[ptr++] = (unsigned char)((tmp >> 24) & 0xff);
        calData[ptr++] = (unsigned char)((tmp >> 16) & 0xff);
        calData[ptr++] = (unsigned char)((tmp >> 8) & 0xff);
        calData[ptr++] = (unsigned char)(tmp & 0xff);
        STORECAL_LOG("temp_valid_data[%d] = %lld\n", i, tmp);
    }
    for (i = 0; i < BINS; i++) {
        for (j = 0; j < PTS_PER_BIN; j++) {
            tmp = (long long)(inv_obj.temp_data[i][j] * 65536.0f);
            if (tmp < 0) {
                tmp += 4294967296LL;
            }
            calData[ptr++] = (unsigned char)((tmp >> 24) & 0xff);
            calData[ptr++] = (unsigned char)((tmp >> 16) & 0xff);
            calData[ptr++] = (unsigned char)((tmp >> 8) & 0xff);
            calData[ptr++] = (unsigned char)(tmp & 0xff);
            STORECAL_LOG("temp_data[%d][%d] = %f\n",
                         i, j, inv_obj.temp_data[i][j]);
        }
    }

    for (i = 0; i < BINS; i++) {
        for (j = 0; j < PTS_PER_BIN; j++) {
            tmp = (long long)(inv_obj.x_gyro_temp_data[i][j] * 65536.0f);
            if (tmp < 0) {
                tmp += 4294967296LL;
            }
            calData[ptr++] = (unsigned char)((tmp >> 24) & 0xff);
            calData[ptr++] = (unsigned char)((tmp >> 16) & 0xff);
            calData[ptr++] = (unsigned char)((tmp >> 8) & 0xff);
            calData[ptr++] = (unsigned char)(tmp & 0xff);
            STORECAL_LOG("x_gyro_temp_data[%d][%d] = %f\n",
                         i, j, inv_obj.x_gyro_temp_data[i][j]);
        }
    }
    for (i = 0; i < BINS; i++) {
        for (j = 0; j < PTS_PER_BIN; j++) {
            tmp = (long long)(inv_obj.y_gyro_temp_data[i][j] * 65536.0f);
            if (tmp < 0) {
                tmp += 4294967296LL;
            }
            calData[ptr++] = (unsigned char)((tmp >> 24) & 0xff);
            calData[ptr++] = (unsigned char)((tmp >> 16) & 0xff);
            calData[ptr++] = (unsigned char)((tmp >> 8) & 0xff);
            calData[ptr++] = (unsigned char)(tmp & 0xff);
            STORECAL_LOG("y_gyro_temp_data[%d][%d] = %f\n",
                         i, j, inv_obj.y_gyro_temp_data[i][j]);
        }
    }
    for (i = 0; i < BINS; i++) {
        for (j = 0; j < PTS_PER_BIN; j++) {
            tmp = (long long)(inv_obj.z_gyro_temp_data[i][j] * 65536.0f);
            if (tmp < 0) {
                tmp += 4294967296LL;
            }
            calData[ptr++] = (unsigned char)((tmp >> 24) & 0xff);
            calData[ptr++] = (unsigned char)((tmp >> 16) & 0xff);
            calData[ptr++] = (unsigned char)((tmp >> 8) & 0xff);
            calData[ptr++] = (unsigned char)(tmp & 0xff);
            STORECAL_LOG("z_gyro_temp_data[%d][%d] = %f\n",
                         i, j, inv_obj.z_gyro_temp_data[i][j]);
        }
    }

    inv_get_array(INV_ACCEL_BIAS, bias);

    /* write the accel biases */
    for (i = 0; i < 3; i++) {
        uint32_t t = (uint32_t) bias[i];
        calData[ptr++] = (unsigned char)((t >> 24) & 0xff);
        calData[ptr++] = (unsigned char)((t >> 16) & 0xff);
        calData[ptr++] = (unsigned char)((t >> 8) & 0xff);
        calData[ptr++] = (unsigned char)(t & 0xff);
        STORECAL_LOG("accel_bias[%d] = %ld\n", i, bias[i]);
    }

    /* write the compass calibration state */
    calData[ptr++] = (unsigned char)(inv_obj.got_compass_bias);
    STORECAL_LOG("got_compass_bias = %ld\n", inv_obj.got_compass_bias);
    calData[ptr++] = (unsigned char)(inv_obj.got_init_compass_bias);
    STORECAL_LOG("got_init_compass_bias = %d\n", inv_obj.got_init_compass_bias);
    if (inv_obj.compass_state == SF_UNCALIBRATED) {
        calData[ptr++] = SF_UNCALIBRATED;
    } else {
        calData[ptr++] = SF_STARTUP_SETTLE;
    }
    STORECAL_LOG("compass_state = %ld\n", inv_obj.compass_state);

    for (i = 0; i < 3; i++) {
        uint32_t t = (uint32_t) inv_obj.compass_bias_error[i];
        calData[ptr++] = (unsigned char)((t >> 24) & 0xff);
        calData[ptr++] = (unsigned char)((t >> 16) & 0xff);
        calData[ptr++] = (unsigned char)((t >> 8) & 0xff);
        calData[ptr++] = (unsigned char)(t & 0xff);
        STORECAL_LOG("compass_bias_error[%d] = %ld\n",
                     i, inv_obj.compass_bias_error[i]);
    }
    for (i = 0; i < 3; i++) {
        uint32_t t = (uint32_t) inv_obj.init_compass_bias[i];
        calData[ptr++] = (unsigned char)((t >> 24) & 0xff);
        calData[ptr++] = (unsigned char)((t >> 16) & 0xff);
        calData[ptr++] = (unsigned char)((t >> 8) & 0xff);
        calData[ptr++] = (unsigned char)(t & 0xff);
        STORECAL_LOG("init_compass_bias[%d] = %ld\n", i,
                     inv_obj.init_compass_bias[i]);
    }
    for (i = 0; i < 3; i++) {
        uint32_t t = (uint32_t) inv_obj.compass_bias[i];
        calData[ptr++] = (unsigned char)((t >> 24) & 0xff);
        calData[ptr++] = (unsigned char)((t >> 16) & 0xff);
        calData[ptr++] = (unsigned char)((t >> 8) & 0xff);
        calData[ptr++] = (unsigned char)(t & 0xff);
        STORECAL_LOG("compass_bias[%d] = %ld\n", i, inv_obj.compass_bias[i]);
    }
    for (i = 0; i < 18; i++) {
        uint32_t t = (uint32_t) inv_obj.compass_peaks[i];
        calData[ptr++] = (unsigned char)((t >> 24) & 0xff);
        calData[ptr++] = (unsigned char)((t >> 16) & 0xff);
        calData[ptr++] = (unsigned char)((t >> 8) & 0xff);
        calData[ptr++] = (unsigned char)(t & 0xff);
        STORECAL_LOG("compass_peaks[%d] = %d\n", i, inv_obj.compass_peaks[i]);
    }
    for (i = 0; i < 3; i++) {
        dToLL.db = inv_obj.compass_bias_v[i];
        calData[ptr++] = (unsigned char)((dToLL.ll >> 56) & 0xff);
        calData[ptr++] = (unsigned char)((dToLL.ll >> 48) & 0xff);
        calData[ptr++] = (unsigned char)((dToLL.ll >> 40) & 0xff);
        calData[ptr++] = (unsigned char)((dToLL.ll >> 32) & 0xff);
        calData[ptr++] = (unsigned char)((dToLL.ll >> 24) & 0xff);
        calData[ptr++] = (unsigned char)((dToLL.ll >> 16) & 0xff);
        calData[ptr++] = (unsigned char)((dToLL.ll >> 8) & 0xff);
        calData[ptr++] = (unsigned char)(dToLL.ll & 0xff);
        STORECAL_LOG("compass_bias_v[%d] = %lf\n", i,
                     inv_obj.compass_bias_v[i]);
    }
    for (i = 0; i < 9; i++) {
        dToLL.db = inv_obj.compass_bias_ptr[i];
        calData[ptr++] = (unsigned char)((dToLL.ll >> 56) & 0xff);
        calData[ptr++] = (unsigned char)((dToLL.ll >> 48) & 0xff);
        calData[ptr++] = (unsigned char)((dToLL.ll >> 40) & 0xff);
        calData[ptr++] = (unsigned char)((dToLL.ll >> 32) & 0xff);
        calData[ptr++] = (unsigned char)((dToLL.ll >> 24) & 0xff);
        calData[ptr++] = (unsigned char)((dToLL.ll >> 16) & 0xff);
        calData[ptr++] = (unsigned char)((dToLL.ll >> 8) & 0xff);
        calData[ptr++] = (unsigned char)(dToLL.ll & 0xff);
        STORECAL_LOG("compass_bias_ptr[%d] = %lf\n", i,
                     inv_obj.compass_bias_ptr[i]);
    }
    for (i = 0; i < 3; i++) {
        uint32_t t = (uint32_t) inv_obj.compass_scale[i];
        calData[ptr++] = (unsigned char)((t >> 24) & 0xff);
        calData[ptr++] = (unsigned char)((t >> 16) & 0xff);
        calData[ptr++] = (unsigned char)((t >> 8) & 0xff);
        calData[ptr++] = (unsigned char)(t & 0xff);
        STORECAL_LOG("compass_scale[%d] = %ld\n", i, inv_obj.compass_scale[i]);
    }
    for (i = 0; i < 6; i++) {
        dToLL.db = inv_obj.compass_prev_xty[i];
        calData[ptr++] = (unsigned char)((dToLL.ll >> 56) & 0xff);
        calData[ptr++] = (unsigned char)((dToLL.ll >> 48) & 0xff);
        calData[ptr++] = (unsigned char)((dToLL.ll >> 40) & 0xff);
        calData[ptr++] = (unsigned char)((dToLL.ll >> 32) & 0xff);
        calData[ptr++] = (unsigned char)((dToLL.ll >> 24) & 0xff);
        calData[ptr++] = (unsigned char)((dToLL.ll >> 16) & 0xff);
        calData[ptr++] = (unsigned char)((dToLL.ll >> 8) & 0xff);
        calData[ptr++] = (unsigned char)(dToLL.ll & 0xff);
        STORECAL_LOG("compass_prev_xty[%d] = %lf\n", i,
                     inv_obj.compass_prev_xty[i]);
    }
    for (i = 0; i < 36; i++) {
        dToLL.db = inv_obj.compass_prev_m[i];
        calData[ptr++] = (unsigned char)((dToLL.ll >> 56) & 0xff);
        calData[ptr++] = (unsigned char)((dToLL.ll >> 48) & 0xff);
        calData[ptr++] = (unsigned char)((dToLL.ll >> 40) & 0xff);
        calData[ptr++] = (unsigned char)((dToLL.ll >> 32) & 0xff);
        calData[ptr++] = (unsigned char)((dToLL.ll >> 24) & 0xff);
        calData[ptr++] = (unsigned char)((dToLL.ll >> 16) & 0xff);
        calData[ptr++] = (unsigned char)((dToLL.ll >> 8) & 0xff);
        calData[ptr++] = (unsigned char)(dToLL.ll & 0xff);
        STORECAL_LOG("compass_prev_m[%d] = %lf\n", i,
                     inv_obj.compass_prev_m[i]);
    }

    /* store the compass offsets and validity flag */
    calData[ptr++] = (unsigned char)inv_obj.flags[INV_COMPASS_OFFSET_VALID];
    calData[ptr++] = (unsigned char)inv_obj.compass_offsets[0];
    calData[ptr++] = (unsigned char)inv_obj.compass_offsets[1];
    calData[ptr++] = (unsigned char)inv_obj.compass_offsets[2];

    /* store the compass accuracy */
    calData[ptr++] = (unsigned char)(inv_obj.compass_accuracy);

    /* add a checksum */
    chk = inv_checksum(calData + INV_CAL_HDR_LEN,
                       length - (INV_CAL_HDR_LEN + INV_CAL_CHK_LEN));
    calData[ptr++] = (unsigned char)((chk >> 24) & 0xff);
    calData[ptr++] = (unsigned char)((chk >> 16) & 0xff);
    calData[ptr++] = (unsigned char)((chk >> 8) & 0xff);
    calData[ptr++] = (unsigned char)(chk & 0xff);

    STORECAL_LOG("Exiting inv_store_cal\n");
    return INV_SUCCESS;
}

/**
 *  @brief  Load a calibration file.
 *
 *  @pre    Must be in INV_STATE_DMP_OPENED state.
 *          inv_dmp_open() or inv_dmp_stop() must have been called.
 *          inv_dmp_start() and inv_dmp_close() must have <b>NOT</b>
 *          been called.
 *
 *  @return 0 or error code.
 */
inv_error_t inv_load_calibration(void)
{
    unsigned char *calData;
    inv_error_t result;
    unsigned int length;

    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    result = inv_serial_get_cal_length(&length);
    if (result == INV_ERROR_FILE_OPEN) {
        MPL_LOGI("Calibration data not loaded\n");
        return INV_SUCCESS;
    }
    if (result || length <= 0) {
        MPL_LOGE("Could not get file calibration length - "
                 "error %d - aborting\n", result);
        return result;
    }
    calData = (unsigned char *)inv_malloc(length);
    if (!calData) {
        MPL_LOGE("Could not allocate buffer of %d bytes - "
                 "aborting\n", length);
        return INV_ERROR_MEMORY_EXAUSTED;
    }
    result = inv_serial_read_cal(calData, length);
    if (result) {
        MPL_LOGE("Could not read the calibration data from file - "
                 "error %d - aborting\n", result);
        goto free_mem_n_exit;

    }
    result = inv_load_cal(calData);
    if (result) {
        MPL_LOGE("Could not load the calibration data - "
                 "error %d - aborting\n", result);
        goto free_mem_n_exit;

    }



free_mem_n_exit:    
    inv_free(calData);
    return INV_SUCCESS;
}

/**
 *  @brief  Store runtime calibration data to a file
 *
 *  @pre    Must be in INV_STATE_DMP_OPENED state.
 *          inv_dmp_open() or inv_dmp_stop() must have been called.
 *          inv_dmp_start() and inv_dmp_close() must have <b>NOT</b>
 *          been called.
 *
 *  @return 0 or error code.
 */
inv_error_t inv_store_calibration(void)
{
    unsigned char *calData;
    inv_error_t result;
    unsigned int length;

    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    length = inv_get_cal_length();
    calData = (unsigned char *)inv_malloc(length);
    if (!calData) {
        MPL_LOGE("Could not allocate buffer of %d bytes - "
                 "aborting\n", length);
        return INV_ERROR_MEMORY_EXAUSTED;
    }
    result = inv_store_cal(calData, length);
    if (result) {
        MPL_LOGE("Could not store calibrated data on file - "
                 "error %d - aborting\n", result);
        goto free_mem_n_exit;

    }
    result = inv_serial_write_cal(calData, length);
    if (result) {
        MPL_LOGE("Could not write calibration data - " "error %d\n", result);
        goto free_mem_n_exit;

    }



free_mem_n_exit:
    inv_free(calData);
    return INV_SUCCESS;
}

/**
 *  @}
 */
