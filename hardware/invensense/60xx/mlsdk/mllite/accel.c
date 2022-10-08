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
 * $Id: accel.c 4595 2011-01-25 01:43:03Z mcaramello $
 *
 *******************************************************************************/

/** 
 *  @defgroup ACCELDL 
 *  @brief  Motion Library - Accel Driver Layer.
 *          Provides the interface to setup and handle an accel
 *          connected to either the primary or the seconday I2C interface 
 *          of the gyroscope.
 *
 *  @{
 *      @file   accel.c
 *      @brief  Accel setup and handling methods.
**/

/* ------------------ */
/* - Include Files. - */
/* ------------------ */

#include <string.h>

#include "ml.h"
#include "mlinclude.h"
#include "dmpKey.h"
#include "mlFIFO.h"
#include "mldl.h"
#include "mldl_cfg.h"
#include "mlMathFunc.h"
#include "mlsl.h"
#include "mlos.h"

#include "log.h"
#undef MPL_LOG_TAG
#define MPL_LOG_TAG "MPL-accel"

#define ACCEL_DEBUG 0

/* --------------------- */
/* - Global Variables. - */
/* --------------------- */

/* --------------------- */
/* - Static Variables. - */
/* --------------------- */

/* --------------- */
/* - Prototypes. - */
/* --------------- */

/* -------------- */
/* - Externs.   - */
/* -------------- */

/* -------------- */
/* - Functions. - */
/* -------------- */

/** 
 *  @brief  Used to determine if an accel is configured and
 *          used by the MPL.
 *  @return INV_SUCCESS if the accel is present.
 */
unsigned char inv_accel_present(void)
{
    INVENSENSE_FUNC_START;
    struct mldl_cfg *mldl_cfg = inv_get_dl_config();
    if (NULL != mldl_cfg->accel &&
        NULL != mldl_cfg->accel->resume &&
        mldl_cfg->requested_sensors & INV_THREE_AXIS_ACCEL)
        return TRUE;
    else
        return FALSE;
}

/**
 *  @brief   Query the accel slave address.
 *  @return  The 7-bit accel slave address.
 */
unsigned char inv_get_slave_addr(void)
{
    INVENSENSE_FUNC_START;
    struct mldl_cfg *mldl_cfg = inv_get_dl_config();
    if (NULL != mldl_cfg->pdata)
        return mldl_cfg->pdata->accel.address;
    else
        return 0;
}

/**
 *  @brief   Get the ID of the accel in use.
 *  @return  ID of the accel in use.
 */
unsigned short inv_get_accel_id(void)
{
    INVENSENSE_FUNC_START;
    struct mldl_cfg *mldl_cfg = inv_get_dl_config();
    if (NULL != mldl_cfg->accel) {
        return mldl_cfg->accel->id;
    }
    return ID_INVALID;
}

/**
 *  @brief  Get a sample of accel data from the device.
 *  @param  data
 *              the buffer to store the accel raw data for
 *              X, Y, and Z axes.
 *  @return INV_SUCCESS or a non-zero error code.
 */
inv_error_t inv_get_accel_data(long *data)
{
    struct mldl_cfg *mldl_cfg = inv_get_dl_config();
    inv_error_t result;
    unsigned char raw_data[2 * ACCEL_NUM_AXES];
    long tmp[ACCEL_NUM_AXES];
    int ii;
    signed char *mtx = mldl_cfg->pdata->accel.orientation;
    char accelId = mldl_cfg->accel->id;

    if (NULL == data)
        return INV_ERROR_INVALID_PARAMETER;

    if (mldl_cfg->accel->read_len > sizeof(raw_data))
        return INV_ERROR_ASSERTION_FAILURE;

    result = (inv_error_t) inv_mpu_read_accel(mldl_cfg,
                                              inv_get_serial_handle(),
                                              inv_get_serial_handle(),
                                              raw_data);
    if (result == INV_ERROR_ACCEL_DATA_NOT_READY) {
        return result;
    }
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    for (ii = 0; ii < ARRAY_SIZE(tmp); ii++) {
        if (EXT_SLAVE_LITTLE_ENDIAN == mldl_cfg->accel->endian) {
            tmp[ii] = (long)((signed char)raw_data[2 * ii + 1]) * 256;
            tmp[ii] += (long)((unsigned char)raw_data[2 * ii]);
        } else if ((EXT_SLAVE_BIG_ENDIAN == mldl_cfg->accel->endian) ||
                   (EXT_SLAVE_FS16_BIG_ENDIAN == mldl_cfg->accel->endian)) {
            tmp[ii] = (long)((signed char)raw_data[2 * ii]) * 256;
            tmp[ii] += (long)((unsigned char)raw_data[2 * ii + 1]);
            if (accelId == ACCEL_ID_KXSD9) {
                tmp[ii] = (long)((short)(((unsigned short)tmp[ii])
                                         + ((unsigned short)0x8000)));
            }
        } else if (EXT_SLAVE_FS8_BIG_ENDIAN == mldl_cfg->accel->endian) {
            tmp[ii] = (long)((signed char)raw_data[ii]) * 256;
        } else {
            result = INV_ERROR_FEATURE_NOT_IMPLEMENTED;
        }
    }

    for (ii = 0; ii < ARRAY_SIZE(tmp); ii++) {
        data[ii] = ((long)tmp[0] * mtx[3 * ii] +
                    (long)tmp[1] * mtx[3 * ii + 1] +
                    (long)tmp[2] * mtx[3 * ii + 2]);
    }

    //MPL_LOGI("ACCEL: %8ld, %8ld, %8ld\n", data[0], data[1], data[2]);
    return result;
}

/**
 *  @}
 */
