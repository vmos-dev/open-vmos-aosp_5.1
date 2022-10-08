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
 * $Id: mldl.c 5653 2011-06-16 21:06:55Z nroyer $
 *
 *****************************************************************************/

/**
 *  @defgroup   MLDL 
 *  @brief      Motion Library - Driver Layer.
 *              The Motion Library Driver Layer provides the intrface to the 
 *              system drivers that are used by the Motion Library.
 *
 *  @{
 *      @file   mldl.c
 *      @brief  The Motion Library Driver Layer.
 */

/* ------------------ */
/* - Include Files. - */
/* ------------------ */

#include <string.h>

#include "mpu.h"
#if defined CONFIG_MPU_SENSORS_MPU6050A2
#    include "mpu6050a2.h"
#elif defined CONFIG_MPU_SENSORS_MPU6050B1
#    include "mpu6050b1.h"
#elif defined CONFIG_MPU_SENSORS_MPU3050
#  include "mpu3050.h"
#else
#error Invalid or undefined CONFIG_MPU_SENSORS_MPUxxxx
#endif
#include "mldl.h"
#include "mldl_cfg.h"
#include "compass.h"
#include "mlsl.h"
#include "mlos.h"
#include "mlinclude.h"
#include "ml.h"
#include "dmpKey.h"
#include "mlFIFOHW.h"
#include "compass.h"
#include "pressure.h"

#include "log.h"
#undef MPL_LOG_TAG
#define MPL_LOG_TAG "MPL-mldl"

#define _mldlDebug(x)           //{x}

/* --------------------- */
/* -    Variables.     - */
/* --------------------- */

#define MAX_LOAD_WRITE_SIZE (MPU_MEM_BANK_SIZE/2)   /* 128 */

/*---- structure containing control variables used by MLDL ----*/
static struct mldl_cfg mldlCfg;
struct ext_slave_descr gAccel;
struct ext_slave_descr gCompass;
struct ext_slave_descr gPressure;
struct mpu_platform_data gPdata;
static void *sMLSLHandle;
int_fast8_t intTrigger[NUM_OF_INTSOURCES];

/*******************************************************************************
 * Functions for accessing the DMP memory via keys
 ******************************************************************************/

unsigned short (*sGetAddress) (unsigned short key) = NULL;
static const unsigned char *localDmpMemory = NULL;
static unsigned short localDmpMemorySize = 0;

/**
 *  @internal
 *  @brief Sets the function to use to convert keys to addresses. This
 *         will changed for each DMP code loaded.
 *  @param func
 *              Function used to convert keys to addresses.
 *  @endif
 */
void inv_set_get_address(unsigned short (*func) (unsigned short key))
{
    INVENSENSE_FUNC_START;
    _mldlDebug(MPL_LOGV("setGetAddress %d", (int)func);
        )
        sGetAddress = func;
}

/**
 *  @internal
 *  @brief  Check if the feature is supported in the currently loaded
 *          DMP code basing on the fact that the key is assigned a
 *          value or not.
 *  @param  key     the DMP key
 *  @return whether the feature associated with the key is supported
 *          or not.
 */
uint_fast8_t inv_dmpkey_supported(unsigned short key)
{
    unsigned short memAddr;

    if (sGetAddress == NULL) {
        MPL_LOGE("%s : sGetAddress is NULL\n", __func__);
        return FALSE;
    }

    memAddr = sGetAddress(key);
    if (memAddr >= 0xffff) {
        MPL_LOGV("inv_set_mpu_memory unsupported key\n");
        return FALSE;
    }

    return TRUE;
}

/**
 *  @internal
 *  @brief  used to get the specified number of bytes from the original
 *          MPU memory location specified by the key.
 *          Reads the specified number of bytes from the MPU location
 *          that was used to program the MPU specified by the key. Each
 *          set of code specifies a function that changes keys into
 *          addresses. This function is set with setGetAddress().
 *
 *  @param  key     The key to use when looking up the address.
 *  @param  length  Number of bytes to read.
 *  @param  buffer  Result for data.
 *
 *  @return INV_SUCCESS if the command is successful, INV_ERROR otherwise. The key
 *          not corresponding to a memory address will result in INV_ERROR.
 *  @endif
 */
inv_error_t inv_get_mpu_memory_original(unsigned short key,
                                        unsigned short length,
                                        unsigned char *buffer)
{
    unsigned short offset;

    if (sGetAddress == NULL) {
        return INV_ERROR_NOT_OPENED;
    }

    offset = sGetAddress(key);
    if (offset >= localDmpMemorySize || (offset + length) > localDmpMemorySize) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    memcpy(buffer, &localDmpMemory[offset], length);

    return INV_SUCCESS;
}

unsigned short inv_dl_get_address(unsigned short key)
{
    unsigned short offset;
    if (sGetAddress == NULL) {
        return INV_ERROR_NOT_OPENED;
    }

    offset = sGetAddress(key);
    return offset;
}

/* ---------------------- */
/* -  Static Functions. - */
/* ---------------------- */

/**
 *  @brief  Open the driver layer and resets the internal
 *          gyroscope, accelerometer, and compass data
 *          structures.
 *  @param  mlslHandle
 *              the serial handle.
 *  @return INV_SUCCESS if successful, a non-zero error code otherwise.
 */
inv_error_t inv_dl_open(void *mlslHandle)
{
    inv_error_t result;
    memset(&mldlCfg, 0, sizeof(mldlCfg));
    memset(intTrigger, INT_CLEAR, sizeof(intTrigger));

    sMLSLHandle = mlslHandle;

    mldlCfg.addr  = 0x68; /* default incase the driver doesn't set it */
    mldlCfg.accel = &gAccel;
    mldlCfg.compass = &gCompass;
    mldlCfg.pressure = &gPressure;
    mldlCfg.pdata = &gPdata;

    result = (inv_error_t) inv_mpu_open(&mldlCfg, sMLSLHandle,
                                        sMLSLHandle, sMLSLHandle, sMLSLHandle);
    return result;
}

/**
 *  @brief  Closes/Cleans up the ML Driver Layer.
 *          Put the device in sleep mode.
 *  @return INV_SUCCESS or non-zero error code.
 */
inv_error_t inv_dl_close(void)
{
    INVENSENSE_FUNC_START;
    inv_error_t result = INV_SUCCESS;

    result = (inv_error_t) inv_mpu_suspend(&mldlCfg,
                                           sMLSLHandle,
                                           sMLSLHandle,
                                           sMLSLHandle,
                                           sMLSLHandle, 
                                           INV_ALL_SENSORS);

    result = (inv_error_t) inv_mpu_close(&mldlCfg, sMLSLHandle,
                                         sMLSLHandle, sMLSLHandle, sMLSLHandle);
    /* Clear all previous settings */
    memset(&mldlCfg, 0, sizeof(mldlCfg));
    sMLSLHandle = NULL;
    sGetAddress = NULL;
    return result;
}

/**
 * @brief Sets the requested_sensors
 *
 * Accessor to set the requested_sensors field of the mldl_cfg structure.
 * Typically set at initialization.
 *
 * @param sensors
 * Bitfield of the sensors that are going to be used.  Combination of the
 * following:
 *  - INV_X_GYRO
 *  - INV_Y_GYRO
 *  - INV_Z_GYRO
 *  - INV_DMP_PROCESSOR
 *  - INV_X_ACCEL
 *  - INV_Y_ACCEL
 *  - INV_Z_ACCEL
 *  - INV_X_COMPASS
 *  - INV_Y_COMPASS
 *  - INV_Z_COMPASS
 *  - INV_X_PRESSURE
 *  - INV_Y_PRESSURE
 *  - INV_Z_PRESSURE
 *  - INV_THREE_AXIS_GYRO
 *  - INV_THREE_AXIS_ACCEL
 *  - INV_THREE_AXIS_COMPASS
 *  - INV_THREE_AXIS_PRESSURE
 *
 * @return INV_SUCCESS or non-zero error code
 */
inv_error_t inv_init_requested_sensors(unsigned long sensors)
{
    mldlCfg.requested_sensors = sensors;

    return INV_SUCCESS;
}

/**
 * @brief Starts the DMP running
 *
 * Resumes the sensor if any of the sensor axis or components are requested
 *
 * @param sensors
 * Bitfield of the sensors to turn on.  Combination of the following:
 *  - INV_X_GYRO
 *  - INV_Y_GYRO
 *  - INV_Z_GYRO
 *  - INV_DMP_PROCESSOR
 *  - INV_X_ACCEL
 *  - INV_Y_ACCEL
 *  - INV_Z_ACCEL
 *  - INV_X_COMPASS
 *  - INV_Y_COMPASS
 *  - INV_Z_COMPASS
 *  - INV_X_PRESSURE
 *  - INV_Y_PRESSURE
 *  - INV_Z_PRESSURE
 *  - INV_THREE_AXIS_GYRO
 *  - INV_THREE_AXIS_ACCEL
 *  - INV_THREE_AXIS_COMPASS
 *  - INV_THREE_AXIS_PRESSURE
 *
 * @return INV_SUCCESS or non-zero error code
 */
inv_error_t inv_dl_start(unsigned long sensors)
{
    INVENSENSE_FUNC_START;
    inv_error_t result = INV_SUCCESS;

    mldlCfg.requested_sensors = sensors;
    result = inv_mpu_resume(&mldlCfg,
                            sMLSLHandle,
                            sMLSLHandle,
                            sMLSLHandle,
                            sMLSLHandle,
                            sensors);
    return result;
}

/**
 * @brief Stops the DMP running and puts it in low power as requested
 *
 * Suspends each sensor according to the bitfield, if all axis and components
 * of the sensor is off.
 *
 * @param sensors Bitfiled of the sensors to leave on.  Combination of the
 * following:
 *  - INV_X_GYRO
 *  - INV_Y_GYRO
 *  - INV_Z_GYRO
 *  - INV_X_ACCEL
 *  - INV_Y_ACCEL
 *  - INV_Z_ACCEL
 *  - INV_X_COMPASS
 *  - INV_Y_COMPASS
 *  - INV_Z_COMPASS
 *  - INV_X_PRESSURE
 *  - INV_Y_PRESSURE
 *  - INV_Z_PRESSURE
 *  - INV_THREE_AXIS_GYRO
 *  - INV_THREE_AXIS_ACCEL
 *  - INV_THREE_AXIS_COMPASS
 *  - INV_THREE_AXIS_PRESSURE
 *
 *
 * @return INV_SUCCESS or non-zero error code
 */
inv_error_t inv_dl_stop(unsigned long sensors)
{
    INVENSENSE_FUNC_START;
    inv_error_t result = INV_SUCCESS;

    result = inv_mpu_suspend(&mldlCfg,
                             sMLSLHandle,
                             sMLSLHandle,
                             sMLSLHandle,
                             sMLSLHandle,
                             sensors);
    return result;
}

/**
 *  @brief  Get a pointer to the internal data structure
 *          storing the configuration for the MPU, the accelerometer
 *          and the compass in use.
 *  @return a pointer to the data structure of type 'struct mldl_cfg'.
 */
struct mldl_cfg *inv_get_dl_config(void)
{
    return &mldlCfg;
}

/**
 *  @brief   Query the MPU slave address.
 *  @return  The 7-bit mpu slave address.
 */
unsigned char inv_get_mpu_slave_addr(void)
{
    INVENSENSE_FUNC_START;
    return mldlCfg.addr;
}

/**
 *  @internal
 * @brief   MLDLCfgDMP configures the Digital Motion Processor internal to
 *          the MPU. The DMP can be enabled or disabled and the start address
 *          can be set.
 *
 * @param   enableRun   Enables the DMP processing if set to TRUE.
 * @param   enableFIFO  Enables DMP output to the FIFO if set to TRUE.
 * @param   startAddress start address
 *
 * @return  Zero if the command is successful, an error code otherwise.
*/
inv_error_t inv_get_dl_ctrl_dmp(unsigned char enableRun,
                                unsigned char enableFIFO)
{
    INVENSENSE_FUNC_START;

    mldlCfg.dmp_enable = enableRun;
    mldlCfg.fifo_enable = enableFIFO;
    mldlCfg.gyro_needs_reset = TRUE;

    return INV_SUCCESS;
}

/**
 * @brief   inv_get_dl_cfg_int configures the interrupt function on the specified pin.
 *          The basic interrupt signal characteristics can be set
 *          (i.e. active high/low, open drain/push pull, etc.) and the
 *          triggers can be set.
 *          Currently only INTPIN_MPU is supported.
 *
 * @param   triggers
 *              bitmask of triggers to enable for interrupt.
 *              The available triggers are:
 *              - BIT_MPU_RDY_EN
 *              - BIT_DMP_INT_EN
 *              - BIT_RAW_RDY_EN
 *
 * @return  Zero if the command is successful, an error code otherwise.
*/
inv_error_t inv_get_dl_cfg_int(unsigned char triggers)
{
    inv_error_t result = INV_SUCCESS;

#if defined CONFIG_MPU_SENSORS_MPU3050
    /* Mantis has 8 bits of interrupt config bits */
    if (triggers & !(BIT_MPU_RDY_EN | BIT_DMP_INT_EN | BIT_RAW_RDY_EN)) {
        return INV_ERROR_INVALID_PARAMETER;
    }
#endif

    mldlCfg.int_config = triggers;
    if (!mldlCfg.gyro_is_suspended) {
        result = inv_serial_single_write(sMLSLHandle, mldlCfg.addr,
                                         MPUREG_INT_CFG,
                                         (mldlCfg.int_config | mldlCfg.pdata->
                                          int_config));
    } else {
        mldlCfg.gyro_needs_reset = TRUE;
    }

    return result;
}

/**
 * @brief   configures the output sampling rate on the MPU.
 *          Three parameters control the sampling:
 *
 *          1) Low pass filter bandwidth, and
 *          2) output sampling divider.
 *
 *          The output sampling rate is determined by the divider and the low
 *          pass filter setting. If the low pass filter is set to
 *          'MPUFILTER_256HZ_NOLPF2', then the sample rate going into the
 *          divider is 8kHz; for all other settings it is 1kHz.
 *          The 8-bit divider will divide this frequency to get the resulting
 *          sample frequency.
 *          For example, if the filter setting is not 256Hz and the divider is
 *          set to 7, then the sample rate is as follows:
 *          sample rate = internal sample rate / div = 1kHz / 8 = 125Hz (or 8ms).
 *
 *          The low pass filter selection codes control both the cutoff frequency of
 *          the internal low pass filter and internal analog sampling rate. The
 *          latter, in turn, affects the final output sampling rate according to the
 *          sample rate divider settig.
 *              0 -> 256 Hz  cutoff BW, 8 kHz analog sample rate,
 *              1 -> 188 Hz  cutoff BW, 1 kHz analog sample rate,
 *              2 ->  98 Hz  cutoff BW, 1 kHz analog sample rate,
 *              3 ->  42 Hz  cutoff BW, 1 kHz analog sample rate,
 *              4 ->  20 Hz  cutoff BW, 1 kHz analog sample rate,
 *              5 ->  10 Hz  cutoff BW, 1 kHz analog sample rate,
 *              6 ->   5 Hz  cutoff BW, 1 kHz analog sample rate,
 *              7 -> 2.1 kHz cutoff BW, 8 kHz analog sample rate.
 *
 * @param   lpf         low pass filter,   0 to 7.
 * @param   divider     Output sampling rate divider, 0 to 255.
 *
 * @return  ML_SUCESS if successful; a non-zero error code otherwise.
**/
inv_error_t inv_dl_cfg_sampling(unsigned char lpf, unsigned char divider)
{
    /*---- do range checking ----*/
    if (lpf >= NUM_MPU_FILTER) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    mldlCfg.lpf = lpf;
    mldlCfg.divider = divider;
    mldlCfg.gyro_needs_reset = TRUE;

    return INV_SUCCESS;
}

/**
 *  @brief  set the full scale range for the gyros.
 *          The full scale selection codes correspond to:
 *              0 -> 250  dps,
 *              1 -> 500  dps,
 *              2 -> 1000 dps,
 *              3 -> 2000 dps.
 *          Full scale range affect the MPU's measurement
 *          sensitivity.
 *
 *  @param  fullScale
 *              the gyro full scale range in dps.
 *
 *  @return INV_SUCCESS or non-zero error code.
**/
inv_error_t inv_set_full_scale(float fullScale)
{
    if (fullScale == 250.f)
        mldlCfg.full_scale = MPU_FS_250DPS;
    else if (fullScale == 500.f)
        mldlCfg.full_scale = MPU_FS_500DPS;
    else if (fullScale == 1000.f)
        mldlCfg.full_scale = MPU_FS_1000DPS;
    else if (fullScale == 2000.f)
        mldlCfg.full_scale = MPU_FS_2000DPS;
    else {                      // not a valid setting
        MPL_LOGE("Invalid full scale range specification for gyros : %f\n",
                 fullScale);
        MPL_LOGE
            ("\tAvailable values : +/- 250 dps, +/- 500 dps, +/- 1000 dps, +/- 2000 dps\n");
        return INV_ERROR_INVALID_PARAMETER;
    }
    mldlCfg.gyro_needs_reset = TRUE;

    return INV_SUCCESS;
}

/**
 * @brief   This function sets the external sync for the MPU sampling.
 *          It can be synchronized on the LSB of any of the gyros, any of the
 *          external accels, or on the temp readings.
 *
 * @param   extSync External sync selection, 0 to 7.
 * @return  Zero if the command is successful; an error code otherwise.
**/
inv_error_t inv_set_external_sync(unsigned char extSync)
{
    INVENSENSE_FUNC_START;

    /*---- do range checking ----*/
    if (extSync >= NUM_MPU_EXT_SYNC) {
        return INV_ERROR_INVALID_PARAMETER;
    }
    mldlCfg.ext_sync = extSync;
    mldlCfg.gyro_needs_reset = TRUE;

    return INV_SUCCESS;
}

inv_error_t inv_set_ignore_system_suspend(unsigned char ignore)
{
    INVENSENSE_FUNC_START;

    mldlCfg.ignore_system_suspend = ignore;

    return INV_SUCCESS;
}

/**
 * @brief   inv_clock_source function sets the clock source for the MPU gyro
 *          processing.
 *          The source can be any of the following:
 *          - Internal 8MHz oscillator,
 *          - PLL with X gyro as reference,
 *          - PLL with Y gyro as reference,
 *          - PLL with Z gyro as reference,
 *          - PLL with external 32.768Mhz reference, or
 *          - PLL with external 19.2MHz reference
 *
 *          For best accuracy and timing, it is highly recommended to use one
 *          of the gyros as the clock source; however this gyro must be
 *          enabled to use its clock (see 'MLDLPowerMgmtMPU()').
 *
 * @param   clkSource   Clock source selection.
 *                      Can be one of:
 *                      - CLK_INTERNAL,
 *                      - CLK_PLLGYROX,
 *                      - CLK_PLLGYROY,
 *                      - CLK_PLLGYROZ,
 *                      - CLK_PLLEXT32K, or
 *                      - CLK_PLLEXT19M.
 *
 * @return  Zero if the command is successful; an error code otherwise.
**/
inv_error_t inv_clock_source(unsigned char clkSource)
{
    INVENSENSE_FUNC_START;

    /*---- do range checking ----*/
    if (clkSource >= NUM_CLK_SEL) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    mldlCfg.clk_src = clkSource;
    mldlCfg.gyro_needs_reset = TRUE;

    return INV_SUCCESS;
}

/**
 *  @brief  Set the Temperature Compensation offset.
 *  @param  tc
 *              a pointer to the temperature compensations offset
 *              for the 3 gyro axes.
 *  @return INV_SUCCESS if successful, a non-zero error code otherwise.
 */
inv_error_t inv_set_offsetTC(const unsigned char *tc)
{
    int ii;
    inv_error_t result;

    for (ii = 0; ii < ARRAY_SIZE(mldlCfg.offset_tc); ii++) {
        mldlCfg.offset_tc[ii] = tc[ii];
    }

    if (!mldlCfg.gyro_is_suspended) {
#ifdef CONFIG_MPU_SENSORS_MPU3050
        result = inv_serial_single_write(sMLSLHandle, mldlCfg.addr,
                                         MPUREG_XG_OFFS_TC, tc[0]);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
        result = inv_serial_single_write(sMLSLHandle, mldlCfg.addr,
                                         MPUREG_YG_OFFS_TC, tc[1]);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
        result = inv_serial_single_write(sMLSLHandle, mldlCfg.addr,
                                         MPUREG_ZG_OFFS_TC, tc[2]);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
#else
        unsigned char reg;
        result = inv_serial_single_write(sMLSLHandle, mldlCfg.addr,
                                         MPUREG_XG_OFFS_TC,
                                         ((mldlCfg.offset_tc[0] << 1)
                                          & BITS_XG_OFFS_TC));
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
        reg = ((mldlCfg.offset_tc[1] << 1) & BITS_YG_OFFS_TC);
#ifdef CONFIG_MPU_SENSORS_MPU6050B1
        if (mldlCfg.pdata->level_shifter)
            reg |= BIT_I2C_MST_VDDIO;
#endif
        result = inv_serial_single_write(sMLSLHandle, mldlCfg.addr,
                                         MPUREG_YG_OFFS_TC, reg);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
        result = inv_serial_single_write(sMLSLHandle, mldlCfg.addr,
                                         MPUREG_ZG_OFFS_TC,
                                         ((mldlCfg.offset_tc[2] << 1)
                                          & BITS_ZG_OFFS_TC));
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
#endif
    } else {
        mldlCfg.gyro_needs_reset = TRUE;
    }
    return INV_SUCCESS;
}

/**
 *  @brief  Set the gyro offset.
 *  @param  offset
 *              a pointer to the gyro offset for the 3 gyro axes. This is scaled
 *              as it would be written to the hardware registers.
 *  @return INV_SUCCESS if successful, a non-zero error code otherwise.
 */
inv_error_t inv_set_offset(const short *offset)
{
    inv_error_t result;
    unsigned char regs[7];
    int ii;
    long sf;

    sf = (2000L * 131) / mldlCfg.gyro_sens_trim;
    for (ii = 0; ii < ARRAY_SIZE(mldlCfg.offset); ii++) {
        // Record the bias in the units the register uses
        mldlCfg.offset[ii] = offset[ii];
        // Convert the bias to 1 dps = 1<<16
        inv_obj.gyro_bias[ii] = -offset[ii] * sf;
        regs[1 + ii * 2] = (unsigned char)(offset[ii] >> 8) & 0xff;
        regs[1 + ii * 2 + 1] = (unsigned char)(offset[ii] & 0xff);
    }

    if (!mldlCfg.gyro_is_suspended) {
        regs[0] = MPUREG_X_OFFS_USRH;
        result = inv_serial_write(sMLSLHandle, mldlCfg.addr, 7, regs);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
    } else {
        mldlCfg.gyro_needs_reset = TRUE;
    }
    return INV_SUCCESS;
}

/**
 *  @internal
 *  @brief  used to get the specified number of bytes in the specified MPU
 *          memory bank.
 *          The memory bank is one of the following:
 *          - MPUMEM_RAM_BANK_0,
 *          - MPUMEM_RAM_BANK_1,
 *          - MPUMEM_RAM_BANK_2, or
 *          - MPUMEM_RAM_BANK_3.
 *
 *  @param  bank    Memory bank to write.
 *  @param  memAddr Starting address for write.
 *  @param  length  Number of bytes to write.
 *  @param  buffer  Result for data.
 *
 *  @return zero if the command is successful, an error code otherwise.
 *  @endif
 */
inv_error_t
inv_get_mpu_memory_one_bank(unsigned char bank,
                            unsigned char memAddr,
                            unsigned short length, unsigned char *buffer)
{
    inv_error_t result;

    if ((bank >= MPU_MEM_NUM_RAM_BANKS) ||
        //(memAddr >= MPU_MEM_BANK_SIZE) || always 0, memAddr is an u_char, therefore limited to 255
        ((memAddr + length) > MPU_MEM_BANK_SIZE) || (NULL == buffer)) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    if (mldlCfg.gyro_is_suspended) {
        memcpy(buffer, &mldlCfg.ram[bank][memAddr], length);
        result = INV_SUCCESS;
    } else {
        result = inv_serial_read_mem(sMLSLHandle, mldlCfg.addr,
                                     ((bank << 8) | memAddr), length, buffer);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
    }

    return result;
}

/**
 *  @internal
 *  @brief  used to set the specified number of bytes in the specified MPU
 *          memory bank.
 *          The memory bank is one of the following:
 *          - MPUMEM_RAM_BANK_0,
 *          - MPUMEM_RAM_BANK_1,
 *          - MPUMEM_RAM_BANK_2, or
 *          - MPUMEM_RAM_BANK_3.
 *
 *  @param  bank    Memory bank to write.
 *  @param  memAddr Starting address for write.
 *  @param  length  Number of bytes to write.
 *  @param  buffer  Result for data.
 *
 *  @return zero if the command is successful, an error code otherwise.
 *  @endif
 */
inv_error_t inv_set_mpu_memory_one_bank(unsigned char bank,
                                        unsigned short memAddr,
                                        unsigned short length,
                                        const unsigned char *buffer)
{
    inv_error_t result = INV_SUCCESS;
    int different;

    if ((bank >= MPU_MEM_NUM_RAM_BANKS) || (memAddr >= MPU_MEM_BANK_SIZE) ||
        ((memAddr + length) > MPU_MEM_BANK_SIZE) || (NULL == buffer)) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    different = memcmp(&mldlCfg.ram[bank][memAddr], buffer, length);
    memcpy(&mldlCfg.ram[bank][memAddr], buffer, length);
    if (!mldlCfg.gyro_is_suspended) {
        result = inv_serial_write_mem(sMLSLHandle, mldlCfg.addr,
                                      ((bank << 8) | memAddr), length, buffer);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
    } else if (different) {
        mldlCfg.gyro_needs_reset = TRUE;
    }

    return result;
}

/**
 *  @internal
 *  @brief  used to get the specified number of bytes from the MPU location
 *          specified by the key.
 *          Reads the specified number of bytes from the MPU location
 *          specified by the key. Each set of code specifies a function
 *          that changes keys into addresses. This function is set with
 *          setGetAddress().
 *
 *  @param  key     The key to use when looking up the address.
 *  @param  length  Number of bytes to read.
 *  @param  buffer  Result for data.
 *
 *  @return INV_SUCCESS if the command is successful, INV_ERROR otherwise. The key
 *          not corresponding to a memory address will result in INV_ERROR.
 *  @endif
 */
inv_error_t inv_get_mpu_memory(unsigned short key,
                               unsigned short length, unsigned char *buffer)
{
    unsigned char bank;
    inv_error_t result;
    unsigned short memAddr;

    if (sGetAddress == NULL) {
        return INV_ERROR_NOT_OPENED;
    }

    memAddr = sGetAddress(key);
    if (memAddr >= 0xffff)
        return INV_ERROR_FEATURE_NOT_IMPLEMENTED;
    bank = memAddr >> 8;        // Get Bank
    memAddr &= 0xff;

    while (memAddr + length > MPU_MEM_BANK_SIZE) {
        // We cross a bank in the middle
        unsigned short sub_length = MPU_MEM_BANK_SIZE - memAddr;
        result = inv_get_mpu_memory_one_bank(bank, (unsigned char)memAddr,
                                             sub_length, buffer);
        if (INV_SUCCESS != result)
            return result;
        bank++;
        length -= sub_length;
        buffer += sub_length;
        memAddr = 0;
    }
    result = inv_get_mpu_memory_one_bank(bank, (unsigned char)memAddr,
                                         length, buffer);

    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    return result;
}

/**
 *  @internal
 *  @brief  used to set the specified number of bytes from the MPU location
 *          specified by the key.
 *          Set the specified number of bytes from the MPU location
 *          specified by the key. Each set of DMP code specifies a function
 *          that changes keys into addresses. This function is set with
 *          setGetAddress().
 *
 *  @param  key     The key to use when looking up the address.
 *  @param  length  Number of bytes to write.
 *  @param  buffer  Result for data.
 *
 *  @return INV_SUCCESS if the command is successful, INV_ERROR otherwise. The key
 *          not corresponding to a memory address will result in INV_ERROR.
 *  @endif
 */
inv_error_t inv_set_mpu_memory(unsigned short key,
                               unsigned short length,
                               const unsigned char *buffer)
{
    inv_error_t result = INV_SUCCESS;
    unsigned short memAddr;
    unsigned char bank;

    if (sGetAddress == NULL) {
        MPL_LOGE("MLDSetMemoryMPU sGetAddress is NULL\n");
        return INV_ERROR_INVALID_MODULE;
    }
    memAddr = sGetAddress(key);

    if (memAddr >= 0xffff) {
        MPL_LOGE("inv_set_mpu_memory unsupported key\n");
        return INV_ERROR_INVALID_MODULE; // This key not supported
    }

    bank = (unsigned char)(memAddr >> 8);
    memAddr &= 0xff;

    while (memAddr + length > MPU_MEM_BANK_SIZE) {
        // We cross a bank in the middle
        unsigned short sub_length = MPU_MEM_BANK_SIZE - memAddr;

        result = inv_set_mpu_memory_one_bank(bank, memAddr, sub_length, buffer);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }

        bank++;
        length -= sub_length;
        buffer += sub_length;
        memAddr = 0;
    }
    result = inv_set_mpu_memory_one_bank(bank, memAddr, length, buffer);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    return result;
}

/**
 *  @brief  Load the DMP with the given code and configuration.
 *  @param  buffer
 *              the DMP data.
 *  @param  length
 *              the length in bytes of the DMP data.
 *  @param  config
 *              the DMP configuration.
 *  @return INV_SUCCESS if successful, a non-zero error code otherwise.
 */
inv_error_t inv_load_dmp(const unsigned char *buffer,
                         unsigned short length, unsigned short config)
{
    INVENSENSE_FUNC_START;

    inv_error_t result = INV_SUCCESS;
    unsigned short toWrite;
    unsigned short memAddr = 0;
    localDmpMemory = buffer;
    localDmpMemorySize = length;

    mldlCfg.dmp_cfg1 = (config >> 8);
    mldlCfg.dmp_cfg2 = (config & 0xff);

    while (length > 0) {
        toWrite = length;
        if (toWrite > MAX_LOAD_WRITE_SIZE)
            toWrite = MAX_LOAD_WRITE_SIZE;

        result =
            inv_set_mpu_memory_one_bank(memAddr >> 8, memAddr & 0xff, toWrite,
                                        buffer);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }

        buffer += toWrite;
        memAddr += toWrite;
        length -= toWrite;
    }

    return result;
}

/**
 *  @brief  Get the silicon revision ID.
 *  @return The silicon revision ID
 *          (0 will be read if inv_mpu_open returned an error)
 */
unsigned char inv_get_silicon_rev(void)
{
    return mldlCfg.silicon_revision;
}

/**
 *  @brief  Get the product revision ID.
 *  @return The product revision ID
 *          (0 will be read if inv_mpu_open returned an error)
 */
unsigned char inv_get_product_rev(void)
{
    return mldlCfg.product_revision;
}

/*******************************************************************************
 *******************************************************************************
 *******************************************************************************
 * @todo these belong with an interface to the kernel driver layer
 *******************************************************************************
 *******************************************************************************
 ******************************************************************************/

/**
 * @brief   inv_get_interrupt_status returns the interrupt status from the specified
 *          interrupt pin.
 * @param   intPin
 *              Currently only the value INTPIN_MPU is supported.
 * @param   status
 *              The available statuses are:
 *              - BIT_MPU_RDY_EN
 *              - BIT_DMP_INT_EN
 *              - BIT_RAW_RDY_EN
 *
 * @return  INV_SUCCESS or a non-zero error code.
 */
inv_error_t inv_get_interrupt_status(unsigned char intPin,
                                     unsigned char *status)
{
    INVENSENSE_FUNC_START;

    inv_error_t result;

    switch (intPin) {

    case INTPIN_MPU:
            /*---- return the MPU interrupt status ----*/
        result = inv_serial_read(sMLSLHandle, mldlCfg.addr,
                                 MPUREG_INT_STATUS, 1, status);
        break;

    default:
        result = INV_ERROR_INVALID_PARAMETER;
        break;
    }

    return result;
}

/**
 *  @brief   query the current status of an interrupt source.
 *  @param   srcIndex
 *              index of the interrupt source.
 *              Currently the only source supported is INTPIN_MPU.
 *
 *  @return  1 if the interrupt has been triggered.
 */
unsigned char inv_get_interrupt_trigger(unsigned char srcIndex)
{
    INVENSENSE_FUNC_START;
    return intTrigger[srcIndex];
}

/**
 * @brief clear the 'triggered' status for an interrupt source.
 * @param srcIndex
 *          index of the interrupt source.
 *          Currently only INTPIN_MPU is supported.
 */
void inv_clear_interrupt_trigger(unsigned char srcIndex)
{
    INVENSENSE_FUNC_START;
    intTrigger[srcIndex] = 0;
}

/**
 * @brief   inv_interrupt_handler function should be called when an interrupt is
 *          received.  The source parameter identifies which interrupt source
 *          caused the interrupt. Note that this routine should not be called
 *          directly from the interrupt service routine.
 *
 * @param   intSource   MPU, AUX1, AUX2, or timer. Can be one of: INTSRC_MPU, INTSRC_AUX1,
 *                      INTSRC_AUX2, or INT_SRC_TIMER.
 *
 * @return  Zero if the command is successful; an error code otherwise.
 */
inv_error_t inv_interrupt_handler(unsigned char intSource)
{
    INVENSENSE_FUNC_START;
    /*---- range check ----*/
    if (intSource >= NUM_OF_INTSOURCES) {
        return INV_ERROR;
    }

    /*---- save source of interrupt ----*/
    intTrigger[intSource] = INT_TRIGGERED;

#ifdef ML_USE_DMP_SIM
    if (intSource == INTSRC_AUX1 || intSource == INTSRC_TIMER) {
        MLSimHWDataInput();
    }
#endif

    return INV_SUCCESS;
}

/***************************/
        /**@}*//* end of defgroup */
/***************************/
