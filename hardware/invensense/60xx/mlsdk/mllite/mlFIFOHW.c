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
 * $Id: mlFIFOHW.c 5653 2011-06-16 21:06:55Z nroyer $
 *
 *******************************************************************************/

/** 
 *  @defgroup MLFIFO_HW 
 *  @brief  Motion Library - FIFO HW Driver.
 *          Provides facilities to interact with the FIFO.
 *
 *  @{
 *      @file   mlFIFOHW.c
 *      @brief  The Motion Library Fifo Hardware Layer.
 *
 */

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
#include "mlFIFOHW.h"
#include "ml.h"
#include "mldl.h"
#include "mldl_cfg.h"

#include "mlsl.h"

#include "log.h"
#undef MPL_LOG_TAG
#define MPL_LOG_TAG "MPL-fifo"

/*
    Defines
*/

#define _fifoDebug(x)           //{x}

/*
    Typedefs
*/

struct fifo_hw_obj {
    short fifoCount;
    inv_error_t fifoError;
    unsigned char fifoOverflow;
    unsigned char fifoResetOnOverflow;
};

/*
    Global variables
*/
const unsigned char gFifoFooter[FIFO_FOOTER_SIZE] = { 0xB2, 0x6A };

/*
    Static variables
*/
static struct fifo_hw_obj fifo_objHW;

/*
    Definitions
*/

/**
 *  @brief  Initializes the internal FIFO data structure.
 */
void inv_init_fifo_hardare(void)
{
    memset(&fifo_objHW, 0, sizeof(fifo_objHW));
    fifo_objHW.fifoResetOnOverflow = TRUE;
}

/**
 *  @internal
 *  @brief  used to get the FIFO data.
 *  @param  length  
 *              Number of bytes to read from the FIFO.
 *  @param  buffer  
 *              the bytes of FIFO data.
 *              Note that this buffer <b>must</b> be large enough
 *              to store and additional trailing FIFO footer when 
 *              expected.  The callers must make sure enough space
 *              is allocated.
 *  @return number of valid bytes of data.
**/
uint_fast16_t inv_get_fifo(uint_fast16_t length, unsigned char *buffer)
{
    INVENSENSE_FUNC_START;
    inv_error_t result;
    uint_fast16_t inFifo;
    uint_fast16_t toRead;
    int_fast8_t kk;

    toRead = length - FIFO_FOOTER_SIZE + fifo_objHW.fifoCount;
    /*---- make sure length is correct ----*/
    if (length > MAX_FIFO_LENGTH || toRead > length || NULL == buffer) {
        fifo_objHW.fifoError = INV_ERROR_INVALID_PARAMETER;
        return 0;
    }

    result = inv_get_fifo_length(&inFifo);
    if (INV_SUCCESS != result) {
        fifo_objHW.fifoError = result;
        return 0;
    }
    // fifo_objHW.fifoCount is the footer size left in the buffer, or 
    //      0 if this is the first time reading the fifo since it was reset
    if (inFifo < length + fifo_objHW.fifoCount) {
        fifo_objHW.fifoError = INV_SUCCESS;
        return 0;
    }
    // if a trailing fifo count is expected - start storing data 2 bytes before
    result =
        inv_read_fifo(fifo_objHW.fifoCount >
                      0 ? buffer : buffer + FIFO_FOOTER_SIZE, toRead);
    if (INV_SUCCESS != result) {
        fifo_objHW.fifoError = result;
        return 0;
    }
    // Make sure the fifo didn't overflow before or during the read
    result = inv_serial_read(inv_get_serial_handle(), inv_get_mpu_slave_addr(),
                             MPUREG_INT_STATUS, 1, &fifo_objHW.fifoOverflow);
    if (INV_SUCCESS != result) {
        fifo_objHW.fifoError = result;
        return 0;
    }

    if (fifo_objHW.fifoOverflow & BIT_INT_STATUS_FIFO_OVERLOW) {
        MPL_LOGV("Resetting Fifo : Overflow\n");
        inv_reset_fifo();
        fifo_objHW.fifoError = INV_ERROR_FIFO_OVERFLOW;
        return 0;
    }

    /* Check the Footer value to give us a chance at making sure data 
     * didn't get corrupted */
    for (kk = 0; kk < fifo_objHW.fifoCount; ++kk) {
        if (buffer[kk] != gFifoFooter[kk]) {
            MPL_LOGV("Resetting Fifo : Invalid footer : 0x%02x 0x%02x\n",
                     buffer[0], buffer[1]);
            _fifoDebug(char out[200];
                       MPL_LOGW("fifoCount : %d\n", fifo_objHW.fifoCount);
                       sprintf(out, "0x");
                       for (kk = 0; kk < (int)toRead; kk++) {
                       sprintf(out, "%s%02X", out, buffer[kk]);}
                       MPL_LOGW("%s\n", out);)
                inv_reset_fifo();
            fifo_objHW.fifoError = INV_ERROR_FIFO_FOOTER;
            return 0;
        }
    }

    if (fifo_objHW.fifoCount == 0) {
        fifo_objHW.fifoCount = FIFO_FOOTER_SIZE;
    }

    return length - FIFO_FOOTER_SIZE;
}

/**
 *  @brief  Used to query the status of the FIFO.
 *  @return INV_SUCCESS if the fifo is OK. An error code otherwise.
**/
inv_error_t inv_get_fifo_status(void)
{
    inv_error_t fifoError = fifo_objHW.fifoError;
    fifo_objHW.fifoError = 0;
    return fifoError;
}

/** 
 * @internal
 * @brief   Get the length from the fifo
 * 
 * @param[out] len amount of data currently stored in the fifo.
 * 
 * @return INV_SUCCESS or non-zero error code.
**/
inv_error_t inv_get_fifo_length(uint_fast16_t * len)
{
    INVENSENSE_FUNC_START;
    unsigned char fifoBuf[2];
    inv_error_t result;

    if (NULL == len)
        return INV_ERROR_INVALID_PARAMETER;

    /*---- read the 2 'count' registers and 
      burst read the data from the FIFO ----*/
    result = inv_serial_read(inv_get_serial_handle(), inv_get_mpu_slave_addr(),
                             MPUREG_FIFO_COUNTH, 2, fifoBuf);
    if (INV_SUCCESS != result) {
        MPL_LOGE("ReadBurst failed %d\n", result);
        inv_reset_fifo();
        fifo_objHW.fifoError = INV_ERROR_FIFO_READ_COUNT;
        *len = 0;
        return result;
    }

    *len = (uint_fast16_t) (fifoBuf[0] << 8);
    *len += (uint_fast16_t) (fifoBuf[1]);
    return result;
}

/**
 *  @brief  inv_get_fifo_count is used to get the number of bytes left in the FIFO.
 *          This function returns the stored value and does not access the hardware.  
 *          See inv_get_fifo_length().
 *  @return the number of bytes left in the FIFO
**/
short inv_get_fifo_count(void)
{
    return fifo_objHW.fifoCount;
}

/** 
 *  @internal
 *  @brief  Read data from the fifo
 * 
 *  @param[out] data Location to store the date read from the fifo
 *  @param[in] len   Amount of data to read out of the fifo
 * 
 *  @return INV_SUCCESS or non-zero error code
**/
inv_error_t inv_read_fifo(unsigned char *data, uint_fast16_t len)
{
    INVENSENSE_FUNC_START;
    inv_error_t result;
    result = inv_serial_read_fifo(inv_get_serial_handle(),
                                  inv_get_mpu_slave_addr(),
                                  (unsigned short)len, data);
    if (INV_SUCCESS != result) {
        MPL_LOGE("inv_serial_readBurst failed %d\n", result);
        inv_reset_fifo();
        fifo_objHW.fifoError = INV_ERROR_FIFO_READ_DATA;
        return result;
    }
    return result;
}

/**
 *  @brief  Clears the FIFO status and its content. 
 *  @note   Halt the DMP writing into the FIFO for the time 
 *          needed to reset the FIFO.
 *  @return INV_SUCCESS if successful, a non-zero error code otherwise.
 */
inv_error_t inv_reset_fifo(void)
{
    INVENSENSE_FUNC_START;
    int len = FIFO_HW_SIZE;
    unsigned char fifoBuf[2];
    unsigned char tries = 0;
    unsigned char userCtrlReg;
    inv_error_t result;
    struct mldl_cfg *mldl_cfg = inv_get_dl_config();

    fifo_objHW.fifoCount = 0;
    if (mldl_cfg->gyro_is_suspended)
        return INV_SUCCESS;

    result = inv_serial_read(inv_get_serial_handle(), inv_get_mpu_slave_addr(),
                             MPUREG_USER_CTRL, 1, &userCtrlReg);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    while (len != 0 && tries < 6) {
        result =
            inv_serial_single_write(inv_get_serial_handle(),
                                    inv_get_mpu_slave_addr(), MPUREG_USER_CTRL,
                                    ((userCtrlReg & (~BIT_FIFO_EN)) |
                                     BIT_FIFO_RST));
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
        result =
            inv_serial_read(inv_get_serial_handle(), inv_get_mpu_slave_addr(),
                            MPUREG_FIFO_COUNTH, 2, fifoBuf);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
        len = (unsigned short)fifoBuf[0] * 256 + (unsigned short)fifoBuf[1];
        tries++;
    }

    result =
        inv_serial_single_write(inv_get_serial_handle(),
                                inv_get_mpu_slave_addr(), MPUREG_USER_CTRL,
                                userCtrlReg);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    return INV_SUCCESS;
}

/**
 * @}
**/
