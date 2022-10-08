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
 * $Id: mlsl_linux_mpu.c 5653 2011-06-16 21:06:55Z nroyer $
 *****************************************************************************/

/**
 *  @defgroup MLSL (Motion Library - Serial Layer)
 *  @brief  The Motion Library System Layer provides the Motion Library the
 *          interface to the system functions.
 *
 *  @{
 *      @file   mlsl_linux_mpu.c
 *      @brief  The Motion Library System Layer.
 *
 */

/* ------------------ */
/* - Include Files. - */
/* ------------------ */
#include <stdio.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <linux/fs.h>
#include <linux/i2c.h>
#include <string.h>
#include <signal.h>
#include <time.h>

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

#include "mlsl.h"
#include "mlos.h"
#include "mlmath.h"
#include "mlinclude.h"

#define MLCAL_ID      (0x0A0B0C0DL)
#define MLCAL_FILE    "/data/cal.bin"
#define MLCFG_ID      (0x01020304L)
#define MLCFG_FILE    "/data/cfg.bin"

#include <log.h>
#undef MPL_LOG_TAG
#define MPL_LOG_TAG "MPL-mlsl"

#ifndef I2CDEV
#define I2CDEV "/dev/mpu"
#endif

#define SERIAL_FULL_DEBUG (0)

/* --------------- */
/* - Prototypes. - */
/* --------------- */

/* ----------------------- */
/* -  Function Pointers. - */
/* ----------------------- */

/* --------------------------- */
/* - Global and Static vars. - */
/* --------------------------- */

/* ---------------- */
/* - Definitions. - */
/* ---------------- */

inv_error_t inv_serial_read_cfg(unsigned char *cfg, unsigned int len)
{
    FILE *fp;
    int bytesRead;

    fp = fopen(MLCFG_FILE, "rb");
    if (fp == NULL) {
        MPL_LOGE("Unable to open \"%s\" for read\n", MLCFG_FILE);
        return INV_ERROR_FILE_OPEN;
    }
    bytesRead = fread(cfg, 1, len, fp);
    if (bytesRead != len) {
        MPL_LOGE("bytes read (%d) don't match requested length (%d)\n",
                 bytesRead, len);
        return INV_ERROR_FILE_READ;
    }
    fclose(fp);

    if (((unsigned int)cfg[0] << 24 | cfg[1] << 16 | cfg[2] << 8 | cfg[3])
        != MLCFG_ID) {
        return INV_ERROR_ASSERTION_FAILURE;
    }

    return INV_SUCCESS;
}

inv_error_t inv_serial_write_cfg(unsigned char *cfg, unsigned int len)
{
    FILE *fp;
    int bytesWritten;
    unsigned char cfgId[4];

    fp = fopen(MLCFG_FILE,"wb");
    if (fp == NULL) {
        MPL_LOGE("Unable to open \"%s\" for write\n", MLCFG_FILE);
        return INV_ERROR_FILE_OPEN;
    }

    cfgId[0] = (unsigned char)(MLCFG_ID >> 24);
    cfgId[1] = (unsigned char)(MLCFG_ID >> 16);
    cfgId[2] = (unsigned char)(MLCFG_ID >> 8);
    cfgId[3] = (unsigned char)(MLCFG_ID);
    bytesWritten = fwrite(cfgId, 1, 4, fp);
    if (bytesWritten != 4) {
        MPL_LOGE("CFG ID could not be written on file\n");
        return INV_ERROR_FILE_WRITE;
    }

    bytesWritten = fwrite(cfg, 1, len, fp);
    if (bytesWritten != len) {
        MPL_LOGE("bytes write (%d) don't match requested length (%d)\n",
                 bytesWritten, len);
        return INV_ERROR_FILE_WRITE;
    }

    fclose(fp);

    return INV_SUCCESS;
}

inv_error_t inv_serial_read_cal(unsigned char *cal, unsigned int len)
{
    FILE *fp;
    int bytesRead;
    inv_error_t result = INV_SUCCESS;

    fp = fopen(MLCAL_FILE,"rb");
    if (fp == NULL) {
        MPL_LOGE("Cannot open file \"%s\" for read\n", MLCAL_FILE);
        return INV_ERROR_FILE_OPEN;
    }
    bytesRead = fread(cal, 1, len, fp);
    if (bytesRead != len) {
        MPL_LOGE("bytes read (%d) don't match requested length (%d)\n",
                 bytesRead, len);
        result = INV_ERROR_FILE_READ;
        goto read_cal_end;
    }

    /* MLCAL_ID not used
    if (((unsigned int)cal[0] << 24 | cal[1] << 16 | cal[2] << 8 | cal[3])
        != MLCAL_ID) {
        result = INV_ERROR_ASSERTION_FAILURE;
        goto read_cal_end;
    }
    */
read_cal_end:
    fclose(fp);
    return result;
}

inv_error_t inv_serial_write_cal(unsigned char *cal, unsigned int len)
{
    FILE *fp;
    int bytesWritten;
    inv_error_t result = INV_SUCCESS;

    fp = fopen(MLCAL_FILE,"wb");
    if (fp == NULL) {
        MPL_LOGE("Cannot open file \"%s\" for write\n", MLCAL_FILE);
        return INV_ERROR_FILE_OPEN;
    }
    bytesWritten = fwrite(cal, 1, len, fp);
    if (bytesWritten != len) {
        MPL_LOGE("bytes written (%d) don't match requested length (%d)\n",
                 bytesWritten, len);
        result = INV_ERROR_FILE_WRITE;
    }
    fclose(fp);
    return result;
}

inv_error_t inv_serial_get_cal_length(unsigned int *len)
{
    FILE *calFile;
    *len = 0;

    calFile = fopen(MLCAL_FILE, "rb");
    if (calFile == NULL) {
        MPL_LOGE("Cannot open file \"%s\" for read\n", MLCAL_FILE);
        return INV_ERROR_FILE_OPEN;
    }

    *len += (unsigned char)fgetc(calFile) << 24;
    *len += (unsigned char)fgetc(calFile) << 16;
    *len += (unsigned char)fgetc(calFile) << 8;
    *len += (unsigned char)fgetc(calFile);

    fclose(calFile);

    if (*len <= 0)
        return INV_ERROR_FILE_READ;

    return INV_SUCCESS;
}

inv_error_t inv_serial_open(char const *port, void **sl_handle)
{
    INVENSENSE_FUNC_START;

    if (NULL == port) {
        port = I2CDEV;
    }
    *sl_handle = (void*)(uintptr_t) open(port, O_RDWR);
    if((intptr_t)*sl_handle < 0) {
        /* ERROR HANDLING; you can check errno to see what went wrong */
        MPL_LOGE("inv_serial_open\n");
        MPL_LOGE("I2C Error %d: Cannot open Adapter %s\n", errno, port);
        return INV_ERROR_SERIAL_OPEN_ERROR;
    } else {
        MPL_LOGI("inv_serial_open: %s\n", port);
    }

    return INV_SUCCESS;
}

inv_error_t inv_serial_close(void *sl_handle)
{
    INVENSENSE_FUNC_START;

    close((int)(uintptr_t)sl_handle);

    return INV_SUCCESS;
}

inv_error_t inv_serial_reset(void *sl_handle)
{
    return INV_ERROR_FEATURE_NOT_IMPLEMENTED;
}

inv_error_t inv_serial_single_write(void *sl_handle,
                               unsigned char slaveAddr,
                               unsigned char registerAddr,
                               unsigned char data)
{
    unsigned char buf[2];
    buf[0] = registerAddr;
    buf[1] = data;
    return inv_serial_write(sl_handle, slaveAddr, 2, buf);
}

inv_error_t inv_serial_write(void *sl_handle,
                         unsigned char slaveAddr,
                         unsigned short length,
                         unsigned char const *data)
{
    INVENSENSE_FUNC_START;
    struct mpu_read_write msg;
    inv_error_t result;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    msg.address = 0; /* not used */
    msg.length  = length;
    msg.data    = (unsigned char*)data;

    if ((result = ioctl((int)(uintptr_t)sl_handle, MPU_WRITE, &msg))) {
        MPL_LOGE("I2C Error: could not write: R:%02x L:%d %d \n",
                 data[0], length, result);
       return result;
    } else if (SERIAL_FULL_DEBUG) {
        char data_buff[4096] = "";
        char strchar[3];
        int ii;
        for (ii = 0; ii < length; ii++) {
            snprintf(strchar, sizeof(strchar), "%02x", data[0]);
            strncat(data_buff, strchar, sizeof(data_buff));
        }
        MPL_LOGI("I2C Write Success %02x %02x: %s \n",
                 data[0], length, data_buff);
    }

    return INV_SUCCESS;
}

inv_error_t inv_serial_read(void *sl_handle,
                        unsigned char  slaveAddr,
                        unsigned char  registerAddr,
                        unsigned short length,
                        unsigned char  *data)
{
    INVENSENSE_FUNC_START;
    int result = INV_SUCCESS;
    struct mpu_read_write msg;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    msg.address = registerAddr;
    msg.length  = length;
    msg.data    = data;

    result = ioctl((int)(uintptr_t)sl_handle, MPU_READ, &msg);

    if (result != INV_SUCCESS) {
        MPL_LOGE("I2C Error %08x: could not read: R:%02x L:%d\n",
                 result, registerAddr, length);
        result = INV_ERROR_SERIAL_READ;
    } else if (SERIAL_FULL_DEBUG) {
        char data_buff[4096] = "";
        char strchar[3];
        int ii;
        for (ii = 0; ii < length; ii++) {
            snprintf(strchar, sizeof(strchar), "%02x", data[0]);
            strncat(data_buff, strchar, sizeof(data_buff));
        }
        MPL_LOGI("I2C Read  Success %02x %02x: %s \n",
                  registerAddr, length, data_buff);
    }

    return (inv_error_t) result;
}

inv_error_t inv_serial_write_mem(void *sl_handle,
                            unsigned char mpu_addr,
                            unsigned short memAddr,
                            unsigned short length,
                            const unsigned char *data)
{
    INVENSENSE_FUNC_START;
    struct mpu_read_write msg;
    int result;

    msg.address = memAddr;
    msg.length  = length;
    msg.data    = (unsigned char *)data;

    result = ioctl((int)(uintptr_t)sl_handle, MPU_WRITE_MEM, &msg);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    } else if (SERIAL_FULL_DEBUG) {
        char data_buff[4096] = "";
        char strchar[3];
        int ii;
        for (ii = 0; ii < length; ii++) {
            snprintf(strchar, sizeof(strchar), "%02x", data[0]);
            strncat(data_buff, strchar, sizeof(data_buff));
        }
        MPL_LOGI("I2C WriteMem Success %04x %04x: %s \n",
                 memAddr, length, data_buff);
    }
    return INV_SUCCESS;
}

inv_error_t inv_serial_read_mem(void *sl_handle,
                           unsigned char  mpu_addr,
                           unsigned short memAddr,
                           unsigned short length,
                           unsigned char  *data)
{
    INVENSENSE_FUNC_START;
    struct mpu_read_write msg;
    int result;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    msg.address = memAddr;
    msg.length  = length;
    msg.data    = data;

    result = ioctl((int)(uintptr_t)sl_handle, MPU_READ_MEM, &msg);
    if (result != INV_SUCCESS) {
        MPL_LOGE("I2C Error %08x: could not read memory: A:%04x L:%d\n",
                 result, memAddr, length);
        return INV_ERROR_SERIAL_READ;
    } else if (SERIAL_FULL_DEBUG) {
        char data_buff[4096] = "";
        char strchar[3];
        int ii;
        for (ii = 0; ii < length; ii++) {
            snprintf(strchar, sizeof(strchar), "%02x", data[0]);
            strncat(data_buff, strchar, sizeof(data_buff));
        }
        MPL_LOGI("I2C ReadMem Success %04x %04x: %s\n",
                 memAddr, length, data_buff);
    }
    return INV_SUCCESS;
}

inv_error_t inv_serial_write_fifo(void *sl_handle,
                             unsigned char mpu_addr,
                             unsigned short length,
                             const unsigned char *data)
{
    INVENSENSE_FUNC_START;
    struct mpu_read_write msg;
    int result;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    msg.address = 0; /* Not used */
    msg.length  = length;
    msg.data    = (unsigned char *)data;

    result = ioctl((int)(uintptr_t)sl_handle, MPU_WRITE_FIFO, &msg);
    if (result != INV_SUCCESS) {
        MPL_LOGE("I2C Error: could not write fifo: %02x %02x\n",
                  MPUREG_FIFO_R_W, length);
        return INV_ERROR_SERIAL_WRITE;
    } else if (SERIAL_FULL_DEBUG) {
        char data_buff[4096] = "";
        char strchar[3];
        int ii;
        for (ii = 0; ii < length; ii++) {
            snprintf(strchar, sizeof(strchar), "%02x", data[0]);
            strncat(data_buff, strchar, sizeof(data_buff));
        }
        MPL_LOGI("I2C Write Success %02x %02x: %s\n",
                 MPUREG_FIFO_R_W, length, data_buff);
    }
    return (inv_error_t) result;
}

inv_error_t inv_serial_read_fifo(void *sl_handle,
                            unsigned char  mpu_addr,
                            unsigned short length,
                            unsigned char  *data)
{
    INVENSENSE_FUNC_START;
    struct mpu_read_write msg;
    int result;

    if (NULL == data) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    msg.address = MPUREG_FIFO_R_W; /* Not used */
    msg.length  = length;
    msg.data    = data;

    result = ioctl((int)(uintptr_t)sl_handle, MPU_READ_FIFO, &msg);
    if (result != INV_SUCCESS) {
        MPL_LOGE("I2C Error %08x: could not read fifo: R:%02x L:%d\n",
                 result, MPUREG_FIFO_R_W, length);
        return INV_ERROR_SERIAL_READ;
    } else if (SERIAL_FULL_DEBUG) {
        char data_buff[4096] = "";
        char strchar[3];
        int ii;
        for (ii = 0; ii < length; ii++) {
            snprintf(strchar, sizeof(strchar), "%02x", data[0]);
            strncat(data_buff, strchar, sizeof(data_buff));
        }
        MPL_LOGI("I2C ReadFifo Success %02x %02x: %s\n",
                 MPUREG_FIFO_R_W, length, data_buff);
    }
    return INV_SUCCESS;
}

/**
 *  @}
 */


