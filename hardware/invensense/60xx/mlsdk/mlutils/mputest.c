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
 * $Id: mputest.c 5637 2011-06-14 01:13:53Z mcaramello $
 *
 *****************************************************************************/

/*
 *  MPU Self Test functions
 *  Version 2.4
 *  May 13th, 2011
 */

/**
 *  @defgroup MPU_SELF_TEST
 *  @brief  MPU Self Test functions
 *
 *  These functions provide an in-site test of the MPU 3xxx chips. The main
 *      entry point is the inv_mpu_test function.
 *  This runs the tests (as described in the accompanying documentation) and
 *      writes a configuration file containing initial calibration data.
 *  inv_mpu_test returns INV_SUCCESS if the chip passes the tests.
 *  Otherwise, an error code is returned.
 *  The functions in this file rely on MLSL and MLOS: refer to the MPL
 *      documentation for more information regarding the system interface
 *      files.
 *
 *  @{
 *      @file   mputest.c
 *      @brief  MPU Self Test routines for assessing gyro sensor status
 *              after surface mount has happened on the target host platform.
 */

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#ifdef LINUX
#include <unistd.h>
#endif

#include "mpu.h"
#include "mldl.h"
#include "mldl_cfg.h"
#include "accel.h"
#include "mlFIFO.h"
#include "slave.h"
#include "ml.h"
#include "ml_stored_data.h"
#include "checksum.h"

#include "mlsl.h"
#include "mlos.h"

#include "log.h"
#undef MPL_LOG_TAG
#define MPL_LOG_TAG "MPL-mpust"

#ifdef __cplusplus
extern "C" {
#endif

/*
    Defines
*/

#define VERBOSE_OUT 0

/*#define TRACK_IDS*/

/*--- Test parameters defaults. See set_test_parameters for more details ---*/

#define DEF_MPU_ADDR             (0x68)        /* I2C address of the mpu     */

#if (USE_SENSE_PATH_TEST == 1)                 /* gyro full scale dps        */
#define DEF_GYRO_FULLSCALE       (2000)
#else
#define DEF_GYRO_FULLSCALE       (250)
#endif

#define DEF_GYRO_SENS            (32768.f / DEF_GYRO_FULLSCALE)
                                               /* gyro sensitivity LSB/dps   */
#define DEF_PACKET_THRESH        (75)          /* 600 ms / 8ms / sample      */
#define DEF_TOTAL_TIMING_TOL     (.03f)        /* 3% = 2 pkts + 1% proc tol. */
#define DEF_BIAS_THRESH          (40 * DEF_GYRO_SENS)
                                               /* 40 dps in LSBs             */
#define DEF_RMS_THRESH           (0.4f * DEF_GYRO_SENS)
                                               /* 0.4 dps-rms in LSB-rms     */
#define DEF_SP_SHIFT_THRESH_CUST (.05f)        /* 5%                         */
#define DEF_TEST_TIME_PER_AXIS   (600)         /* ms of time spent collecting
                                                  data for each axis,
                                                  multiple of 600ms          */
#define DEF_N_ACCEL_SAMPLES      (20)          /* num of accel samples to
                                                  average from, if applic.   */

#define ML_INIT_CAL_LEN          (36)          /* length in bytes of
                                                  calibration data file      */

/*
    Macros
*/

#define CHECK_TEST_ERROR(x)                                                 \
    if (x) {                                                                \
        MPL_LOGI("error %d @ %s|%d\n", x, __func__, __LINE__);              \
        return (-1);                                                        \
    }

#define SHORT_TO_TEMP_C(shrt)         (((shrt+13200.f)/280.f)+35.f)

#define USHORT_TO_CHARS(chr,shrt)     (chr)[0]=(unsigned char)(shrt>>8);    \
                                      (chr)[1]=(unsigned char)(shrt);

#define UINT_TO_CHARS(chr,ui)         (chr)[0]=(unsigned char)(ui>>24);     \
                                      (chr)[1]=(unsigned char)(ui>>16);     \
                                      (chr)[2]=(unsigned char)(ui>>8);      \
                                      (chr)[3]=(unsigned char)(ui);

#define FLOAT_TO_SHORT(f)             (                                     \
                                        (fabs(f-(short)f)>=0.5) ? (         \
                                            ((short)f)+(f<0?(-1):(+1))) :   \
                                            ((short)f)                      \
                                      )

#define CHARS_TO_SHORT(d)             ((((short)(d)[0])<<8)+(d)[1])
#define CHARS_TO_SHORT_SWAPPED(d)     ((((short)(d)[1])<<8)+(d)[0])

#define ACCEL_UNPACK(d) d[0], d[1], d[2], d[3], d[4], d[5]

#define CHECK_NACKS(d)  (                               \
                         d[0]==0xff && d[1]==0xff &&    \
                         d[2]==0xff && d[3]==0xff &&    \
                         d[4]==0xff && d[5]==0xff       \
                        )

/*
    Prototypes
*/

static inv_error_t test_get_data(
                    void *mlsl_handle,
                    struct mldl_cfg *mputestCfgPtr,
                    short *vals);

/*
    Types
*/
typedef struct {
    float gyro_sens;
    int gyro_fs;
    int packet_thresh;
    float total_timing_tol;
    int bias_thresh;
    float rms_threshSq;
    float sp_shift_thresh;
    unsigned int test_time_per_axis;
    unsigned short accel_samples;
} tTestSetup;

/*
    Global variables
*/
static unsigned char dataout[20];
static unsigned char dataStore[ML_INIT_CAL_LEN];

static tTestSetup test_setup = {
    DEF_GYRO_SENS,
    DEF_GYRO_FULLSCALE,
    DEF_PACKET_THRESH,
    DEF_TOTAL_TIMING_TOL,
    (int)DEF_BIAS_THRESH,
    DEF_RMS_THRESH * DEF_RMS_THRESH,
    DEF_SP_SHIFT_THRESH_CUST,
    DEF_TEST_TIME_PER_AXIS,
    DEF_N_ACCEL_SAMPLES
};

static float adjGyroSens;
static char a_name[3][2] = {"X", "Y", "Z"};

/*
    NOTE :  modify get_slave_descr parameter below to reflect
                the DEFAULT accelerometer in use. The accelerometer in use
                can be modified at run-time using the inv_test_setup_accel API.
    NOTE :  modify the expected z axis orientation (Z axis pointing
                upward or downward)
*/

signed char g_z_sign = +1;
struct mldl_cfg *mputestCfgPtr = NULL;

#ifndef LINUX
/**
 *  @internal
 *  @brief  usec precision sleep function.
 *  @param  number of micro seconds (us) to sleep for.
 */
static void usleep(unsigned long t)
{
    unsigned long start = inv_get_tick_count();
    while (inv_get_tick_count()-start < t / 1000);
}
#endif

/**
 *  @brief  Modify the self test limits from their default values.
 *
 *  @param  slave_addr
 *              the slave address the MPU device is setup to respond at.
 *              The default is DEF_MPU_ADDR = 0x68.
 *  @param  sensitivity
 *              the read sensitivity of the device in LSB/dps as it is trimmed.
 *              NOTE :  if using the self test as part of the MPL, the
 *                      sensitivity the different sensitivity trims are already
 *                      taken care of.
 *  @param  p_thresh
 *              number of packets expected to be received in a 600 ms period.
 *              Depends on the sampling frequency of choice (set by default to
 *              125 Hz) and low pass filter cut-off frequency selection (set
 *              to 42 Hz).
 *              The default is DEF_PACKET_THRESH = 75 packets.
 *  @param  total_time_tol
 *              time skew tolerance, taking into account imprecision in turning
 *              the FIFO on and off and the processor time imprecision (for
 *              1 GHz processor).
 *              The default is DEF_TOTAL_TIMING_TOL = 3 %, about 2 packets.
 *  @param  bias_thresh
 *              bias level threshold, the maximun acceptable no motion bias
 *              for a production quality part.
 *              The default is DEF_BIAS_THRESH = 40 dps.
 *  @param  rms_thresh
 *              the limit standard deviation (=~ RMS) set to assess whether
 *              the noise level on the part is acceptable.
 *              The default is DEF_RMS_THRESH = 0.2 dps-rms.
 *  @param  sp_shift_thresh
 *              the limit shift applicable to the Sense Path self test
 *              calculation.
 */
void inv_set_test_parameters(unsigned int slave_addr, float sensitivity,
                             int p_thresh, float total_time_tol,
                             int bias_thresh, float rms_thresh,
                             float sp_shift_thresh,
                             unsigned short accel_samples)
{
    mputestCfgPtr->addr = slave_addr;
    test_setup.gyro_sens = sensitivity;
    test_setup.gyro_fs = (int)(32768.f / sensitivity);
    test_setup.packet_thresh = p_thresh;
    test_setup.total_timing_tol = total_time_tol;
    test_setup.bias_thresh = bias_thresh;
    test_setup.rms_threshSq = rms_thresh * rms_thresh;
    test_setup.sp_shift_thresh = sp_shift_thresh;
    test_setup.accel_samples = accel_samples;
}

#define X   (0)
#define Y   (1)
#define Z   (2)

#ifdef CONFIG_MPU_SENSORS_MPU3050
/**
 *  @brief  Test the gyroscope sensor.
 *          Implements the core logic of the MPU Self Test.
 *          Produces the PASS/FAIL result. Loads the calculated gyro biases
 *          and temperature datum into the corresponding pointers.
 *  @param  mlsl_handle
 *              serial interface handle to allow serial communication with the
 *              device, both gyro and accelerometer.
 *  @param  gyro_biases
 *              output pointer to store the initial bias calculation provided
 *              by the MPU Self Test.  Requires 3 elements for gyro X, Y,
 *              and Z.
 *  @param  temp_avg
 *              output pointer to store the initial average temperature as
 *              provided by the MPU Self Test.
 *  @param  perform_full_test
 *              If 1:
 *              calculates offset, drive frequency, and noise and compare it
 *              against set thresholds.
 *              Report also the final result using a bit-mask like error code
 *              as explained in return value description.
 *              When 0:
 *              skip the noise and drive frequency calculation and pass/fail
 *              assessment; simply calculates the gyro biases.
 *
 *  @return 0 on success.
 *          On error, the return value is a bitmask representing:
 *          0, 1, 2     Failures with PLLs on X, Y, Z gyros respectively
 *                      (decimal values will be 1, 2, 4 respectively).
 *          3, 4, 5     Excessive offset with X, Y, Z gyros respectively
 *                      (decimal values will be 8, 16, 32 respectively).
 *          6, 7, 8     Excessive noise with X, Y, Z gyros respectively
 *                      (decimal values will be 64, 128, 256 respectively).
 *          9           If any of the RMS noise values is zero, it could be
 *                      due to a non-functional gyro or FIFO/register failure.
 *                      (decimal value will be 512).
 *                      (decimal values will be 1024, 2048, 4096 respectively).
 */
int inv_test_gyro_3050(void *mlsl_handle,
                       short gyro_biases[3], short *temp_avg,
                       uint_fast8_t perform_full_test)
{
    int retVal = 0;
    inv_error_t result;

    int total_count = 0;
    int total_count_axis[3] = {0, 0, 0};
    int packet_count;
    short x[DEF_TEST_TIME_PER_AXIS / 8 * 4] = {0};
    short y[DEF_TEST_TIME_PER_AXIS / 8 * 4] = {0};
    short z[DEF_TEST_TIME_PER_AXIS / 8 * 4] = {0};
    unsigned char regs[7];

    int temperature;
    float Avg[3];
    float RMS[3];
    int i, j, tmp;
    char tmpStr[200];

    temperature = 0;

    /* sample rate = 8ms */
    result = inv_serial_single_write(
                mlsl_handle, mputestCfgPtr->addr,
                MPUREG_SMPLRT_DIV, 0x07);
    CHECK_TEST_ERROR(result);

    regs[0] = 0x03; /* filter = 42Hz, analog_sample rate = 1 KHz */
    switch (DEF_GYRO_FULLSCALE) {
        case 2000:
            regs[0] |= 0x18;
            break;
        case 1000:
            regs[0] |= 0x10;
            break;
        case 500:
            regs[0] |= 0x08;
            break;
        case 250:
        default:
            regs[0] |= 0x00;
            break;
    }
    result = inv_serial_single_write(
                mlsl_handle, mputestCfgPtr->addr,
                MPUREG_DLPF_FS_SYNC, regs[0]);
    CHECK_TEST_ERROR(result);
    result = inv_serial_single_write(
                mlsl_handle, mputestCfgPtr->addr,
                MPUREG_INT_CFG, 0x00);
    CHECK_TEST_ERROR(result);

    /* 1st, timing test */
    for (j = 0; j < 3; j++) {

        MPL_LOGI("Collecting gyro data from %s gyro PLL\n", a_name[j]);

        /* turn on all gyros, use gyro X for clocking
           Set to Y and Z for 2nd and 3rd iteration */
        result = inv_serial_single_write(
                    mlsl_handle, mputestCfgPtr->addr,
                    MPUREG_PWR_MGM, j + 1);
        CHECK_TEST_ERROR(result);

        /* wait for 2 ms after switching clock source */
        usleep(2000);

        /* we will enable XYZ gyro in FIFO and nothing else */
        result = inv_serial_single_write(
                    mlsl_handle, mputestCfgPtr->addr,
                    MPUREG_FIFO_EN2, 0x00);
        CHECK_TEST_ERROR(result);
        /* enable/reset FIFO */
        result = inv_serial_single_write(
                    mlsl_handle, mputestCfgPtr->addr,
                    MPUREG_USER_CTRL, BIT_FIFO_EN | BIT_FIFO_RST);
        CHECK_TEST_ERROR(result);

        tmp = (int)(test_setup.test_time_per_axis / 600);
        while (tmp-- > 0) {
            /* enable XYZ gyro in FIFO and nothing else */
            result = inv_serial_single_write(mlsl_handle,
                        mputestCfgPtr->addr, MPUREG_FIFO_EN1,
                        BIT_GYRO_XOUT | BIT_GYRO_YOUT | BIT_GYRO_ZOUT);
            CHECK_TEST_ERROR(result);

            /* wait for 600 ms for data */
            usleep(600000);

            /* stop storing gyro in the FIFO */
            result = inv_serial_single_write(
                        mlsl_handle, mputestCfgPtr->addr,
                        MPUREG_FIFO_EN1, 0x00);
            CHECK_TEST_ERROR(result);

            /* Getting number of bytes in FIFO */
            result = inv_serial_read(
                           mlsl_handle, mputestCfgPtr->addr,
                           MPUREG_FIFO_COUNTH, 2, dataout);
            CHECK_TEST_ERROR(result);
            /* number of 6 B packets in the FIFO */
            packet_count = CHARS_TO_SHORT(dataout) / 6;
            sprintf(tmpStr, "Packet Count: %d - ", packet_count);

            if ( abs(packet_count - test_setup.packet_thresh)
                        <=      /* Within +/- total_timing_tol % range */
                     test_setup.total_timing_tol * test_setup.packet_thresh) {
                for (i = 0; i < packet_count; i++) {
                    /* getting FIFO data */
                    result = inv_serial_read_fifo(mlsl_handle,
                                mputestCfgPtr->addr, 6, dataout);
                    CHECK_TEST_ERROR(result);
                    x[total_count + i] = CHARS_TO_SHORT(&dataout[0]);
                    y[total_count + i] = CHARS_TO_SHORT(&dataout[2]);
                    z[total_count + i] = CHARS_TO_SHORT(&dataout[4]);
                    if (VERBOSE_OUT) {
                        MPL_LOGI("Gyros %-4d    : %+13d %+13d %+13d\n",
                                    total_count + i, x[total_count + i],
                                    y[total_count + i], z[total_count + i]);
                    }
                }
                total_count += packet_count;
                total_count_axis[j] += packet_count;
                sprintf(tmpStr, "%sOK", tmpStr);
            } else {
                if (perform_full_test)
                    retVal |= 1 << j;
                sprintf(tmpStr, "%sNOK - samples ignored", tmpStr);
            }
            MPL_LOGI("%s\n", tmpStr);
        }

        /* remove gyros from FIFO */
        result = inv_serial_single_write(
                    mlsl_handle, mputestCfgPtr->addr,
                    MPUREG_FIFO_EN1, 0x00);
        CHECK_TEST_ERROR(result);

        /* Read Temperature */
        result = inv_serial_read(mlsl_handle, mputestCfgPtr->addr,
                    MPUREG_TEMP_OUT_H, 2, dataout);
        CHECK_TEST_ERROR(result);
        temperature += (short)CHARS_TO_SHORT(dataout);
    }

    MPL_LOGI("\n");
    MPL_LOGI("Total %d samples\n", total_count);
    MPL_LOGI("\n");

    /* 2nd, check bias from X and Y PLL clock source */
    tmp = total_count != 0 ? total_count : 1;
    for (i = 0,
            Avg[X] = .0f, Avg[Y] = .0f, Avg[Z] = .0f;
         i < total_count; i++) {
        Avg[X] += 1.f * x[i] / tmp;
        Avg[Y] += 1.f * y[i] / tmp;
        Avg[Z] += 1.f * z[i] / tmp;
    }
    MPL_LOGI("bias          : %+13.3f %+13.3f %+13.3f (LSB)\n",
             Avg[X], Avg[Y], Avg[Z]);
    if (VERBOSE_OUT) {
        MPL_LOGI("              : %+13.3f %+13.3f %+13.3f (dps)\n",
                 Avg[X] / adjGyroSens,
                 Avg[Y] / adjGyroSens,
                 Avg[Z] / adjGyroSens);
    }
    if(perform_full_test) {
        for (j = 0; j < 3; j++) {
            if (fabs(Avg[j]) > test_setup.bias_thresh) {
                MPL_LOGI("%s-Gyro bias (%.0f) exceeded threshold "
                         "(threshold = %d)\n",
                         a_name[j], Avg[j], test_setup.bias_thresh);
                retVal |= 1 << (3+j);
            }
        }
    }

    /* 3rd, check RMS */
    if (perform_full_test) {
        for (i = 0,
                RMS[X] = 0.f, RMS[Y] = 0.f, RMS[Z] = 0.f;
             i < total_count; i++) {
            RMS[X] += (x[i] - Avg[X]) * (x[i] - Avg[X]);
            RMS[Y] += (y[i] - Avg[Y]) * (y[i] - Avg[Y]);
            RMS[Z] += (z[i] - Avg[Z]) * (z[i] - Avg[Z]);
        }
        for (j = 0; j < 3; j++) {
            if (RMS[j] > test_setup.rms_threshSq * total_count) {
                MPL_LOGI("%s-Gyro RMS (%.2f) exceeded threshold "
                         "(threshold = %.2f)\n",
                         a_name[j], sqrt(RMS[j] / total_count),
                         sqrt(test_setup.rms_threshSq));
                retVal |= 1 << (6+j);
            }
        }

        MPL_LOGI("RMS           : %+13.3f %+13.3f %+13.3f (LSB-rms)\n",
                    sqrt(RMS[X] / total_count),
                    sqrt(RMS[Y] / total_count),
                    sqrt(RMS[Z] / total_count));
        if (VERBOSE_OUT) {
            MPL_LOGI("RMS ^ 2       : %+13.3f %+13.3f %+13.3f\n",
                        RMS[X] / total_count,
                        RMS[Y] / total_count,
                        RMS[Z] / total_count);
        }

        if (RMS[X] == 0 || RMS[Y] == 0 || RMS[Z] == 0) {
            /*  If any of the RMS noise value returns zero,
                then we might have dead gyro or FIFO/register failure,
                the part is sleeping, or the part is not responsive */
            retVal |= 1 << 9;
        }
    }

    /* 4th, temperature average */
    temperature /= 3;
    if (VERBOSE_OUT)
        MPL_LOGI("Temperature   : %+13.3f %13s %13s (deg. C)\n",
                 SHORT_TO_TEMP_C(temperature), "", "");

    /* load into final storage */
    *temp_avg = (short)temperature;
    gyro_biases[X] = FLOAT_TO_SHORT(Avg[X]);
    gyro_biases[Y] = FLOAT_TO_SHORT(Avg[Y]);
    gyro_biases[Z] = FLOAT_TO_SHORT(Avg[Z]);

    return retVal;
}

#else /* CONFIG_MPU_SENSORS_MPU3050 */

/**
 *  @brief  Test the gyroscope sensor.
 *          Implements the core logic of the MPU Self Test but does not provide
 *          a PASS/FAIL output as in the MPU-3050 implementation.
 *  @param  mlsl_handle
 *              serial interface handle to allow serial communication with the
 *              device, both gyro and accelerometer.
 *  @param  gyro_biases
 *              output pointer to store the initial bias calculation provided
 *              by the MPU Self Test.  Requires 3 elements for gyro X, Y,
 *              and Z.
 *  @param  temp_avg
 *              output pointer to store the initial average temperature as
 *              provided by the MPU Self Test.
 *
 *  @return 0 on success.
 *          A non-zero error code on error.
 */
int inv_test_gyro_6050(void *mlsl_handle,
                       short gyro_biases[3], short *temp_avg)
{
    inv_error_t result;

    int total_count = 0;
    int total_count_axis[3] = {0, 0, 0};
    int packet_count;
    short x[DEF_TEST_TIME_PER_AXIS / 8 * 4] = {0};
    short y[DEF_TEST_TIME_PER_AXIS / 8 * 4] = {0};
    short z[DEF_TEST_TIME_PER_AXIS / 8 * 4] = {0};
    unsigned char regs[7];

    int temperature = 0;
    float Avg[3];
    int i, j, tmp;
    char tmpStr[200];

    /* sample rate = 8ms */
    result = inv_serial_single_write(
                mlsl_handle, mputestCfgPtr->addr,
                MPUREG_SMPLRT_DIV, 0x07);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    regs[0] = 0x03; /* filter = 42Hz, analog_sample rate = 1 KHz */
    switch (DEF_GYRO_FULLSCALE) {
        case 2000:
            regs[0] |= 0x18;
            break;
        case 1000:
            regs[0] |= 0x10;
            break;
        case 500:
            regs[0] |= 0x08;
            break;
        case 250:
        default:
            regs[0] |= 0x00;
            break;
    }
    result = inv_serial_single_write(
                mlsl_handle, mputestCfgPtr->addr,
                MPUREG_CONFIG, regs[0]);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    result = inv_serial_single_write(
                mlsl_handle, mputestCfgPtr->addr,
                MPUREG_INT_ENABLE, 0x00);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    /* 1st, timing test */
    for (j = 0; j < 3; j++) {
        MPL_LOGI("Collecting gyro data from %s gyro PLL\n", a_name[j]);

        /* turn on all gyros, use gyro X for clocking
           Set to Y and Z for 2nd and 3rd iteration */
#ifdef CONFIG_MPU_SENSORS_MPU6050A2
        result = inv_serial_single_write(
                    mlsl_handle, mputestCfgPtr->addr,
                    MPUREG_PWR_MGMT_1, BITS_PWRSEL | (j + 1));
#else
        result = inv_serial_single_write(
                    mlsl_handle, mputestCfgPtr->addr,
                    MPUREG_PWR_MGMT_1, j + 1);
#endif
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }

        /* wait for 2 ms after switching clock source */
        usleep(2000);

        /* enable/reset FIFO */
        result = inv_serial_single_write(
                    mlsl_handle, mputestCfgPtr->addr,
                    MPUREG_USER_CTRL, BIT_FIFO_EN | BIT_FIFO_RST);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }

        tmp = (int)(test_setup.test_time_per_axis / 600);
        while (tmp-- > 0) {
            /* enable XYZ gyro in FIFO and nothing else */
            result = inv_serial_single_write(mlsl_handle,
                        mputestCfgPtr->addr, MPUREG_FIFO_EN,
                        BIT_GYRO_XOUT | BIT_GYRO_YOUT | BIT_GYRO_ZOUT);
            if (result) {
                LOG_RESULT_LOCATION(result);
                return result;
            }

            /* wait for 600 ms for data */
            usleep(600000);
            /* stop storing gyro in the FIFO */
            result = inv_serial_single_write(
                        mlsl_handle, mputestCfgPtr->addr,
                        MPUREG_FIFO_EN, 0x00);
            if (result) {
                LOG_RESULT_LOCATION(result);
                return result;
            }
            /* Getting number of bytes in FIFO */
            result = inv_serial_read(
                           mlsl_handle, mputestCfgPtr->addr,
                           MPUREG_FIFO_COUNTH, 2, dataout);
            if (result) {
                LOG_RESULT_LOCATION(result);
                return result;
            }
            /* number of 6 B packets in the FIFO */
            packet_count = CHARS_TO_SHORT(dataout) / 6;
            sprintf(tmpStr, "Packet Count: %d - ", packet_count);

            if (abs(packet_count - test_setup.packet_thresh)
                        <=      /* Within +/- total_timing_tol % range */
                     test_setup.total_timing_tol * test_setup.packet_thresh) {
                for (i = 0; i < packet_count; i++) {
                    /* getting FIFO data */
                    result = inv_serial_read_fifo(mlsl_handle,
                                mputestCfgPtr->addr, 6, dataout);
                    if (result) {
                        LOG_RESULT_LOCATION(result);
                        return result;
                    }
                    x[total_count + i] = CHARS_TO_SHORT(&dataout[0]);
                    y[total_count + i] = CHARS_TO_SHORT(&dataout[2]);
                    z[total_count + i] = CHARS_TO_SHORT(&dataout[4]);
                    if (VERBOSE_OUT) {
                        MPL_LOGI("Gyros %-4d    : %+13d %+13d %+13d\n",
                                    total_count + i, x[total_count + i],
                                    y[total_count + i], z[total_count + i]);
                    }
                }
                total_count += packet_count;
                total_count_axis[j] += packet_count;
                sprintf(tmpStr, "%sOK", tmpStr);
            } else {
                sprintf(tmpStr, "%sNOK - samples ignored", tmpStr);
            }
            MPL_LOGI("%s\n", tmpStr);
        }

        /* remove gyros from FIFO */
        result = inv_serial_single_write(
                    mlsl_handle, mputestCfgPtr->addr,
                    MPUREG_FIFO_EN, 0x00);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }

        /* Read Temperature */
        result = inv_serial_read(mlsl_handle, mputestCfgPtr->addr,
                    MPUREG_TEMP_OUT_H, 2, dataout);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
        temperature += (short)CHARS_TO_SHORT(dataout);
    }

    MPL_LOGI("\n");
    MPL_LOGI("Total %d samples\n", total_count);
    MPL_LOGI("\n");

    /* 2nd, check bias from X and Y PLL clock source */
    tmp = total_count != 0 ? total_count : 1;
    for (i = 0,
            Avg[X] = .0f, Avg[Y] = .0f, Avg[Z] = .0f;
         i < total_count; i++) {
        Avg[X] += 1.f * x[i] / tmp;
        Avg[Y] += 1.f * y[i] / tmp;
        Avg[Z] += 1.f * z[i] / tmp;
    }
    MPL_LOGI("bias          : %+13.3f %+13.3f %+13.3f (LSB)\n",
             Avg[X], Avg[Y], Avg[Z]);
    if (VERBOSE_OUT) {
        MPL_LOGI("              : %+13.3f %+13.3f %+13.3f (dps)\n",
                 Avg[X] / adjGyroSens,
                 Avg[Y] / adjGyroSens,
                 Avg[Z] / adjGyroSens);
    }

    temperature /= 3;
    if (VERBOSE_OUT)
        MPL_LOGI("Temperature   : %+13.3f %13s %13s (deg. C)\n",
                 SHORT_TO_TEMP_C(temperature), "", "");

    /* load into final storage */
    *temp_avg = (short)temperature;
    gyro_biases[X] = FLOAT_TO_SHORT(Avg[X]);
    gyro_biases[Y] = FLOAT_TO_SHORT(Avg[Y]);
    gyro_biases[Z] = FLOAT_TO_SHORT(Avg[Z]);

    return INV_SUCCESS;
}

#endif /* CONFIG_MPU_SENSORS_MPU3050 */

#ifdef TRACK_IDS
/**
 *  @internal
 *  @brief  Retrieve the unique MPU device identifier from the internal OTP
 *          bank 0 memory.
 *  @param  mlsl_handle
 *              serial interface handle to allow serial communication with the
 *              device, both gyro and accelerometer.
 *  @return 0 on success, a non-zero error code from the serial layer on error.
 */
static inv_error_t test_get_mpu_id(void *mlsl_handle)
{
    inv_error_t result;
    unsigned char otp0[8];


    result =
        inv_serial_read_mem(mlsl_handle, mputestCfgPtr->addr,
            (BIT_PRFTCH_EN | BIT_CFG_USER_BANK | MPU_MEM_OTP_BANK_0) << 8 |
            0x00, 6, otp0);
    if (result)
        goto close;

    MPL_LOGI("\n");
    MPL_LOGI("DIE_ID   : %06X\n",
                ((int)otp0[1] << 8 | otp0[0]) & 0x1fff);
    MPL_LOGI("WAFER_ID : %06X\n",
                (((int)otp0[2] << 8 | otp0[1]) & 0x03ff ) >> 5);
    MPL_LOGI("A_LOT_ID : %06X\n",
                ( ((int)otp0[4] << 16 | (int)otp0[3] << 8 |
                otp0[2]) & 0x3ffff) >> 2);
    MPL_LOGI("W_LOT_ID : %06X\n",
                ( ((int)otp0[5] << 8 | otp0[4]) & 0x3fff) >> 2);
    MPL_LOGI("WP_ID    : %06X\n",
                ( ((int)otp0[6] << 8 | otp0[5]) & 0x03ff) >> 7);
    MPL_LOGI("REV_ID   : %06X\n", otp0[6] >> 2);
    MPL_LOGI("\n");

close:
    result =
        inv_serial_single_write(mlsl_handle, mputestCfgPtr->addr, MPUREG_BANK_SEL, 0x00);
    return result;
}
#endif /* TRACK_IDS */

/**
 *  @brief  If requested via inv_test_setup_accel(), test the accelerometer biases
 *          and calculate the necessary bias correction.
 *  @param  mlsl_handle
 *              serial interface handle to allow serial communication with the
 *              device, both gyro and accelerometer.
 *  @param  bias
 *              output pointer to store the initial bias calculation provided
 *              by the MPU Self Test.  Requires 3 elements to store accel X, Y,
 *              and Z axis bias.
 *  @param  perform_full_test
 *              If 1:
 *              calculates offsets and noise and compare it against set
 *              thresholds. The final exist status will reflect if any of the
 *              value is outside of the expected range.
 *              When 0;
 *              skip the noise calculation and pass/fail assessment; simply
 *              calculates the accel biases.
 *
 *  @return 0 on success. A non-zero error code on error.
 */
int inv_test_accel(void *mlsl_handle,
                   short *bias, long gravity,
                   uint_fast8_t perform_full_test)
{
    int i;

    short *p_vals;
    float x = 0.f, y = 0.f, z = 0.f, zg = 0.f;
    float RMS[3];
    float accelRmsThresh = 1000000.f; /* enourmous so that the test always
                                         passes - future deployment */
    unsigned short res;
    unsigned long orig_requested_sensors;
    struct mpu_platform_data *mputestPData = mputestCfgPtr->pdata;

    p_vals = (short*)inv_malloc(sizeof(short) * 3 * test_setup.accel_samples);

    /* load the slave descr from the getter */
    if (mputestPData->accel.get_slave_descr == NULL) {
        MPL_LOGI("\n");
        MPL_LOGI("No accelerometer configured\n");
        return 0;
    }
    if (mputestCfgPtr->accel == NULL) {
        MPL_LOGI("\n");
        MPL_LOGI("No accelerometer configured\n");
        return 0;
    }

    /* resume the accel */
    orig_requested_sensors = mputestCfgPtr->requested_sensors;
    mputestCfgPtr->requested_sensors = INV_THREE_AXIS_ACCEL | INV_THREE_AXIS_GYRO;
    res = inv_mpu_resume(mputestCfgPtr,
                         mlsl_handle, NULL, NULL, NULL,
                         mputestCfgPtr->requested_sensors);
    if(res != INV_SUCCESS)
        goto accel_error;

    /* wait at least a sample cycle for the
       accel data to be retrieved by MPU */
    inv_sleep(inv_mpu_get_sampling_period_us(mputestCfgPtr) / 1000);

    /* collect the samples  */
    for(i = 0; i < test_setup.accel_samples; i++) {
        short *vals = &p_vals[3 * i];
        if (test_get_data(mlsl_handle, mputestCfgPtr, vals)) {
            goto accel_error;
        }
        x += 1.f * vals[X] / test_setup.accel_samples;
        y += 1.f * vals[Y] / test_setup.accel_samples;
        z += 1.f * vals[Z] / test_setup.accel_samples;
    }

    mputestCfgPtr->requested_sensors = orig_requested_sensors;
    res = inv_mpu_suspend(mputestCfgPtr,
                          mlsl_handle, NULL, NULL, NULL,
                          INV_ALL_SENSORS);
    if (res != INV_SUCCESS)
        goto accel_error;

    MPL_LOGI("Accel biases  : %+13.3f %+13.3f %+13.3f (LSB)\n", x, y, z);
    if (VERBOSE_OUT) {
        MPL_LOGI("Accel biases  : %+13.3f %+13.3f %+13.3f (gee)\n",
                    x / gravity, y / gravity, z / gravity);
    }

    bias[0] = FLOAT_TO_SHORT(x);
    bias[1] = FLOAT_TO_SHORT(y);
    zg = z - g_z_sign * gravity;
    bias[2] = FLOAT_TO_SHORT(zg);

    MPL_LOGI("Accel correct.: %+13d %+13d %+13d (LSB)\n",
             bias[0], bias[1], bias[2]);
    if (VERBOSE_OUT) {
        MPL_LOGI("Accel correct.: "
               "%+13.3f %+13.3f %+13.3f (gee)\n",
                    1.f * bias[0] / gravity,
                    1.f * bias[1] / gravity,
                    1.f * bias[2] / gravity);
    }

    if (perform_full_test) {
        /* accel RMS - for now the threshold is only indicative */
        for (i = 0,
                 RMS[X] = 0.f, RMS[Y] = 0.f, RMS[Z] = 0.f;
             i <  test_setup.accel_samples; i++) {
            short *vals = &p_vals[3 * i];
            RMS[X] += (vals[X] - x) * (vals[X] - x);
            RMS[Y] += (vals[Y] - y) * (vals[Y] - y);
            RMS[Z] += (vals[Z] - z) * (vals[Z] - z);
        }
        for (i = 0; i < 3; i++) {
            if (RMS[i] >  accelRmsThresh * accelRmsThresh
                            * test_setup.accel_samples) {
                MPL_LOGI("%s-Accel RMS (%.2f) exceeded threshold "
                         "(threshold = %.2f)\n",
                         a_name[i], sqrt(RMS[i] / test_setup.accel_samples),
                         accelRmsThresh);
                goto accel_error;
            }
        }
        MPL_LOGI("RMS           : %+13.3f %+13.3f %+13.3f (LSB-rms)\n",
                 sqrt(RMS[X] / DEF_N_ACCEL_SAMPLES),
                 sqrt(RMS[Y] / DEF_N_ACCEL_SAMPLES),
                 sqrt(RMS[Z] / DEF_N_ACCEL_SAMPLES));
    }

    return 0; /* success */

accel_error:  /* error */
    bias[0] = bias[1] = bias[2] = 0;
    return 1;
}

/**
 *  @brief  an user-space substitute of the power management function(s)
 *          in mldl_cfg.c for self test usage.
 *          Wake up and sleep the device, whether that is MPU3050, MPU6050A2,
 *          or MPU6050B1.
 *  @param  mlsl_handle
 *              a file handle for the serial communication port used to
 *              communicate with the MPU device.
 *  @param  power_level
 *              the power state to change the device into. Currently only 0 or
 *              1 are supported, for sleep and full-power respectively.
 *  @return 0 on success; a non-zero error code on error.
 */
static inv_error_t inv_device_power_mgmt(void *mlsl_handle,
                                         uint_fast8_t power_level)
{
    inv_error_t result;
    static unsigned char pwr_mgm;
    unsigned char b;

    if (power_level != 0 && power_level != 1) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    if (power_level) {
        result = inv_serial_read(
                    mlsl_handle, mputestCfgPtr->addr,
                    MPUREG_PWR_MGM, 1, &pwr_mgm);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }

        /* reset */
        result = inv_serial_single_write(
                    mlsl_handle, mputestCfgPtr->addr,
                    MPUREG_PWR_MGM, pwr_mgm | BIT_H_RESET);
#ifndef CONFIG_MPU_SENSORS_MPU6050A2
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
#endif
        inv_sleep(5);

        /* re-read power mgmt reg -
           it may have reset after H_RESET is applied */
        result = inv_serial_read(
                    mlsl_handle, mputestCfgPtr->addr,
                    MPUREG_PWR_MGM, 1, &b);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }

        /* wake up */
#ifdef CONFIG_MPU_SENSORS_MPU6050A2
        if ((b & BITS_PWRSEL) != BITS_PWRSEL) {
            result = inv_serial_single_write(
                        mlsl_handle, mputestCfgPtr->addr,
                        MPUREG_PWR_MGM, BITS_PWRSEL);
            if (result) {
                LOG_RESULT_LOCATION(result);
                return result;
            }
        }
#else
        if (pwr_mgm & BIT_SLEEP) {
            result = inv_serial_single_write(
                        mlsl_handle, mputestCfgPtr->addr,
                        MPUREG_PWR_MGM, 0x00);
            if (result) {
                LOG_RESULT_LOCATION(result);
                return result;
            }
        }
#endif
        inv_sleep(60);

#if defined(CONFIG_MPU_SENSORS_MPU6050A2) || \
    defined(CONFIG_MPU_SENSORS_MPU6050B1)
        result = inv_serial_single_write(
                    mlsl_handle, mputestCfgPtr->addr,
                    MPUREG_INT_PIN_CFG, BIT_BYPASS_EN);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
#endif
    } else {
        /* restore the power state the part was found in */
#ifdef CONFIG_MPU_SENSORS_MPU6050A2
        if ((pwr_mgm & BITS_PWRSEL) != BITS_PWRSEL) {
            result = inv_serial_single_write(
                        mlsl_handle, mputestCfgPtr->addr,
                        MPUREG_PWR_MGM, pwr_mgm);
            if (result) {
                LOG_RESULT_LOCATION(result);
                return result;
            }
        }
#else
        if (pwr_mgm & BIT_SLEEP) {
            result = inv_serial_single_write(
                        mlsl_handle, mputestCfgPtr->addr,
                        MPUREG_PWR_MGM, pwr_mgm);
            if (result) {
                LOG_RESULT_LOCATION(result);
                return result;
            }
        }
#endif
    }

    return INV_SUCCESS;
}

/**
 *  @brief  The main entry point of the MPU Self Test, triggering the run of
 *          the single tests, for gyros and accelerometers.
 *          Prepares the MPU for the test, taking the device out of low power
 *          state if necessary, switching the MPU secondary I2C interface into
 *          bypass mode and restoring the original power state at the end of
 *          the test.
 *          This function is also responsible for encoding the output of each
 *          test in the correct format as it is stored on the file/medium of
 *          choice (according to inv_serial_write_cal() function).
 *          The format needs to stay perfectly consistent with the one expected
 *          by the corresponding loader in ml_stored_data.c; currectly the
 *          loaded in use is inv_load_cal_V1 (record type 1 - initial
 *          calibration).
 *
 *  @param  mlsl_handle
 *              serial interface handle to allow serial communication with the
 *              device, both gyro and accelerometer.
 *  @param  provide_result
 *              If 1:
 *              perform and analyze the offset, drive frequency, and noise
 *              calculation and compare it against set threshouds. Report
 *              also the final result using a bit-mask like error code as
 *              described in the inv_test_gyro() function.
 *              When 0:
 *              skip the noise and drive frequency calculation and pass/fail
 *              assessment. It simply calculates the gyro and accel biases.
 *              NOTE: for MPU6050 devices, this parameter is currently
 *              ignored.
 *
 *  @return 0 on success.  A non-zero error code on error.
 *          Propagates the errors from the tests up to the caller.
 */
int inv_mpu_test(void *mlsl_handle, uint_fast8_t provide_result)
{
    int result = 0;

    short temp_avg;
    short gyro_biases[3] = {0, 0, 0};
    short accel_biases[3] = {0, 0, 0};

    unsigned long testStart = inv_get_tick_count();
    long accelSens[3] = {0};
    int ptr;
    int tmp;
    long long lltmp;
    uint32_t chk;

    MPL_LOGI("Collecting %d groups of 600 ms samples for each axis\n",
                DEF_TEST_TIME_PER_AXIS / 600);
    MPL_LOGI("\n");

    result = inv_device_power_mgmt(mlsl_handle, TRUE);

#ifdef TRACK_IDS
    result = test_get_mpu_id(mlsl_handle);
    if (result != INV_SUCCESS) {
        MPL_LOGI("Could not read the device's unique ID\n");
        MPL_LOGI("\n");
        return result;
    }
#endif

    /* adjust the gyro sensitivity according to the gyro_sens_trim value */
    adjGyroSens = test_setup.gyro_sens * mputestCfgPtr->gyro_sens_trim / 131.f;
    test_setup.gyro_fs = (int)(32768.f / adjGyroSens);

    /* collect gyro and temperature data */
#ifdef CONFIG_MPU_SENSORS_MPU3050
    result = inv_test_gyro_3050(mlsl_handle,
                gyro_biases, &temp_avg, provide_result);
#else
    MPL_LOGW_IF(provide_result,
                "Self Test for MPU-6050 devices is for bias correction only: "
                "no test PASS/FAIL result will be provided\n");
    result = inv_test_gyro_6050(mlsl_handle, gyro_biases, &temp_avg);
#endif

    MPL_LOGI("\n");
    MPL_LOGI("Test time : %ld ms\n", inv_get_tick_count() - testStart);
    if (result)
        return result;

    /* collect accel data.  if this step is skipped,
       ensure the array still contains zeros. */
    if (mputestCfgPtr->accel != NULL) {
        float fs;
        RANGE_FIXEDPOINT_TO_FLOAT(mputestCfgPtr->accel->range, fs);
        accelSens[0] = (long)(32768L / fs);
        accelSens[1] = (long)(32768L / fs);
        accelSens[2] = (long)(32768L / fs);
#if defined CONFIG_MPU_SENSORS_MPU6050B1
        if (MPL_PROD_KEY(mputestCfgPtr->product_id,
                mputestCfgPtr->product_revision) == MPU_PRODUCT_KEY_B1_E1_5) {
            accelSens[2] /= 2;
        } else {
            unsigned short trim_corr = 16384 / mputestCfgPtr->accel_sens_trim;
            accelSens[0] /= trim_corr;
            accelSens[1] /= trim_corr;
            accelSens[2] /= trim_corr;
        }
#endif
    } else {
        /* would be 0, but 1 to avoid divide-by-0 below */
        accelSens[0] = accelSens[1] = accelSens[2] = 1;
    }
#ifdef CONFIG_MPU_SENSORS_MPU3050
    result = inv_test_accel(mlsl_handle, accel_biases, accelSens[2],
                            provide_result);
#else
    result = inv_test_accel(mlsl_handle, accel_biases, accelSens[2],
                            FALSE);
#endif
    if (result)
        return result;

    result = inv_device_power_mgmt(mlsl_handle, FALSE);
    if (result)
        return result;

    ptr = 0;
    dataStore[ptr++] = 0;       /* total len of factory cal */
    dataStore[ptr++] = 0;
    dataStore[ptr++] = 0;
    dataStore[ptr++] = ML_INIT_CAL_LEN;
    dataStore[ptr++] = 0;
    dataStore[ptr++] = 5;       /* record type 5 - initial calibration */

    tmp = temp_avg;             /* temperature */
    if (tmp < 0) tmp += 2 << 16;
    USHORT_TO_CHARS(&dataStore[ptr], tmp);
    ptr += 2;

    /* NOTE : 2 * test_setup.gyro_fs == 65536 / (32768 / test_setup.gyro_fs) */
    lltmp = (long)gyro_biases[0] * 2 * test_setup.gyro_fs; /* x gyro avg */
    if (lltmp < 0) lltmp += 1LL << 32;
    UINT_TO_CHARS(&dataStore[ptr], (uint32_t)lltmp);
    ptr += 4;
    lltmp = (long)gyro_biases[1] * 2 * test_setup.gyro_fs; /* y gyro avg */
    if (lltmp < 0) lltmp += 1LL << 32;
    UINT_TO_CHARS(&dataStore[ptr], (uint32_t)lltmp);
    ptr += 4;
    lltmp = (long)gyro_biases[2] * 2 * test_setup.gyro_fs; /* z gyro avg */
    if (lltmp < 0) lltmp += 1LL << 32;
    UINT_TO_CHARS(&dataStore[ptr], (uint32_t)lltmp);
    ptr += 4;

    lltmp = (long)accel_biases[0] * 65536L / accelSens[0]; /* x accel avg */
    if (lltmp < 0) lltmp += 1LL << 32;
    UINT_TO_CHARS(&dataStore[ptr], (uint32_t)lltmp);
    ptr += 4;
    lltmp = (long)accel_biases[1] * 65536L / accelSens[1]; /* y accel avg */
    if (lltmp < 0) lltmp += 1LL << 32;
    UINT_TO_CHARS(&dataStore[ptr], (uint32_t)lltmp);
    ptr += 4;
    lltmp = (long)accel_biases[2] * 65536L / accelSens[2]; /* z accel avg */
    if (lltmp < 0) lltmp += 1LL << 32;
    UINT_TO_CHARS(&dataStore[ptr], (uint32_t)lltmp);
    ptr += 4;

    /* add a checksum for data */
    chk = inv_checksum(
        dataStore + INV_CAL_HDR_LEN,
        ML_INIT_CAL_LEN - INV_CAL_HDR_LEN - INV_CAL_CHK_LEN);
    UINT_TO_CHARS(&dataStore[ptr], chk);
    ptr += 4;

    if (ptr != ML_INIT_CAL_LEN) {
        MPL_LOGI("Invalid calibration data length: exp %d, got %d\n",
                    ML_INIT_CAL_LEN, ptr);
        return -1;
    }

    return result;
}

/**
 *  @brief  The main test API. Runs the MPU Self Test and, if successful,
 *          stores the encoded initial calibration data on the final storage
 *          medium of choice (cfr. inv_serial_write_cal() and the MLCAL_FILE
 *          define in your mlsl implementation).
 *
 *  @param  mlsl_handle
 *              serial interface handle to allow serial communication with the
 *              device, both gyro and accelerometer.
 *  @param  provide_result
 *              If 1:
 *              perform and analyze the offset, drive frequency, and noise
 *              calculation and compare it against set threshouds. Report
 *              also the final result using a bit-mask like error code as
 *              described in the inv_test_gyro() function.
 *              When 0:
 *              skip the noise and drive frequency calculation and pass/fail
 *              assessment. It simply calculates the gyro and accel biases.
 *
 *  @return 0 on success or a non-zero error code from the callees on error.
 */
inv_error_t inv_factory_calibrate(void *mlsl_handle,
                                  uint_fast8_t provide_result)
{
    int result;

    result = inv_mpu_test(mlsl_handle, provide_result);
    if (provide_result) {
        MPL_LOGI("\n");
        if (result == 0) {
            MPL_LOGI("Test : PASSED\n");
        } else {
            MPL_LOGI("Test : FAILED %d/%04X - Biases NOT stored\n", result, result);
            return result; /* abort writing the calibration if the
                              test is not successful */
        }
        MPL_LOGI("\n");
    } else {
        MPL_LOGI("\n");
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
    }

    result = inv_serial_write_cal(dataStore, ML_INIT_CAL_LEN);
    if (result) {
        MPL_LOGI("Error : cannot write calibration on file - error %d\n",
            result);
        return result;
    }

    return INV_SUCCESS;
}



/* -----------------------------------------------------------------------
    accel interface functions
 -----------------------------------------------------------------------*/

/**
 *  @internal
 *  @brief  Reads data for X, Y, and Z axis from the accelerometer device.
 *          Used only if an accelerometer has been setup using the
 *          inv_test_setup_accel() API.
 *          Takes care of the accelerometer endianess according to how the
 *          device has been described in the corresponding accelerometer driver
 *          file.
 *  @param  mlsl_handle
 *              serial interface handle to allow serial communication with the
 *              device, both gyro and accelerometer.
 *  @param  slave
 *              a pointer to the descriptor of the slave accelerometer device
 *              in use. Contains the necessary information to operate, read,
 *              and communicate with the accelerometer device of choice.
 *              See the declaration of struct ext_slave_descr in mpu.h.
 *  @param  pdata
 *              a pointer to the platform info of the slave accelerometer
 *              device in use. Describes how the device is oriented and
 *              mounted on host platform's PCB.
 *  @param  vals
 *              output pointer to return the accelerometer's X, Y, and Z axis
 *              sensor data collected.
 *  @return 0 on success or a non-zero error code on error.
 */
static inv_error_t test_get_data(
                    void *mlsl_handle,
                    struct mldl_cfg *mputestCfgPtr,
                    short *vals)
{
    inv_error_t result;
    unsigned char data[20];
    struct ext_slave_descr *slave = mputestCfgPtr->accel;
#ifndef CONFIG_MPU_SENSORS_MPU3050
    struct ext_slave_platform_data *pdata = &mputestCfgPtr->pdata->accel;
#endif

#ifdef CONFIG_MPU_SENSORS_MPU3050
    result = inv_serial_read(mlsl_handle, mputestCfgPtr->addr, 0x23,
                             6, data);
#else
    result = inv_serial_read(mlsl_handle, pdata->address, slave->read_reg,
                            slave->read_len, data);
#endif
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    if (VERBOSE_OUT) {
        MPL_LOGI("Accel         :        0x%02X%02X        0x%02X%02X        0x%02X%02X (raw)\n",
            ACCEL_UNPACK(data));
    }

    if (CHECK_NACKS(data)) {
        MPL_LOGI("Error fetching data from the accelerometer : "
                 "all bytes read 0xff\n");
        return INV_ERROR_SERIAL_READ;
    }

    if (slave->endian == EXT_SLAVE_BIG_ENDIAN) {
        vals[0] = CHARS_TO_SHORT(&data[0]);
        vals[1] = CHARS_TO_SHORT(&data[2]);
        vals[2] = CHARS_TO_SHORT(&data[4]);
    } else {
        vals[0] = CHARS_TO_SHORT_SWAPPED(&data[0]);
        vals[1] = CHARS_TO_SHORT_SWAPPED(&data[2]);
        vals[2] = CHARS_TO_SHORT_SWAPPED(&data[4]);
    }

    if (VERBOSE_OUT) {
        MPL_LOGI("Accel         : %+13d %+13d %+13d (LSB)\n",
                 vals[0], vals[1], vals[2]);
    }
    return INV_SUCCESS;
}

#ifdef __cplusplus
}
#endif

/**
 *  @}
 */

