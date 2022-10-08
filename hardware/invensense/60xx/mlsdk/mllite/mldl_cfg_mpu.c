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
 * $Id: mldl_cfg_mpu.c 5653 2011-06-16 21:06:55Z nroyer $
 *
 *****************************************************************************/

/** 
 *  @addtogroup MLDL
 *
 *  @{
 *      @file   mldl_cfg_mpu.c
 *      @brief  The Motion Library Driver Layer.
 */

/* ------------------ */
/* - Include Files. - */
/* ------------------ */

#include <stddef.h>
#include "mldl_cfg.h"
#include "mlsl.h"
#include "mpu.h"

#ifdef LINUX
#include <sys/ioctl.h>
#endif

#include "log.h"
#undef MPL_LOG_TAG
#define MPL_LOG_TAG "MPL-mldl_cfg_mpu:"

/* --------------------- */
/* -    Variables.     - */
/* --------------------- */


/* ---------------------- */
/* -  Static Functions. - */
/* ---------------------- */
void mpu_print_cfg(struct mldl_cfg * mldl_cfg)
{
    struct mpu_platform_data   *pdata   = mldl_cfg->pdata;
    struct ext_slave_platform_data *accel   = &mldl_cfg->pdata->accel;
    struct ext_slave_platform_data *compass = &mldl_cfg->pdata->compass;
    struct ext_slave_platform_data *pressure = &mldl_cfg->pdata->pressure;

    MPL_LOGD("mldl_cfg.addr             = %02x\n", mldl_cfg->addr);
    MPL_LOGD("mldl_cfg.int_config       = %02x\n", mldl_cfg->int_config);
    MPL_LOGD("mldl_cfg.ext_sync         = %02x\n", mldl_cfg->ext_sync);
    MPL_LOGD("mldl_cfg.full_scale       = %02x\n", mldl_cfg->full_scale);
    MPL_LOGD("mldl_cfg.lpf              = %02x\n", mldl_cfg->lpf);
    MPL_LOGD("mldl_cfg.clk_src          = %02x\n", mldl_cfg->clk_src);
    MPL_LOGD("mldl_cfg.divider          = %02x\n", mldl_cfg->divider);
    MPL_LOGD("mldl_cfg.dmp_enable       = %02x\n", mldl_cfg->dmp_enable);
    MPL_LOGD("mldl_cfg.fifo_enable      = %02x\n", mldl_cfg->fifo_enable);
    MPL_LOGD("mldl_cfg.dmp_cfg1         = %02x\n", mldl_cfg->dmp_cfg1);
    MPL_LOGD("mldl_cfg.dmp_cfg2         = %02x\n", mldl_cfg->dmp_cfg2);
    MPL_LOGD("mldl_cfg.offset_tc[0]     = %02x\n", mldl_cfg->offset_tc[0]);
    MPL_LOGD("mldl_cfg.offset_tc[1]     = %02x\n", mldl_cfg->offset_tc[1]);
    MPL_LOGD("mldl_cfg.offset_tc[2]     = %02x\n", mldl_cfg->offset_tc[2]);
    MPL_LOGD("mldl_cfg.silicon_revision = %02x\n", mldl_cfg->silicon_revision);
    MPL_LOGD("mldl_cfg.product_id       = %02x\n", mldl_cfg->product_id);
    MPL_LOGD("mldl_cfg.gyro_sens_trim   = %02x\n", mldl_cfg->gyro_sens_trim);
#if defined CONFIG_MPU_SENSORS_MPU6050A2 || defined CONFIG_MPU_SENSORS_MPU6050B1
    MPL_LOGD("mldl_cfg.accel_sens_trim   = %02x\n", mldl_cfg->accel_sens_trim);
#endif

    if (mldl_cfg->accel) {
        MPL_LOGD("slave_accel->suspend      = %p\n", mldl_cfg->accel->suspend);
        MPL_LOGD("slave_accel->resume       = %p\n", mldl_cfg->accel->resume);
        MPL_LOGD("slave_accel->read         = %p\n", mldl_cfg->accel->read);
        MPL_LOGD("slave_accel->type         = %02x\n", mldl_cfg->accel->type);
        MPL_LOGD("slave_accel->read_reg     = %02x\n",
                 mldl_cfg->accel->read_reg);
        MPL_LOGD("slave_accel->read_len     = %02x\n",
                 mldl_cfg->accel->read_len);
        MPL_LOGD("slave_accel->endian       = %02x\n", mldl_cfg->accel->endian);
        MPL_LOGD("slave_accel->range.mantissa= %02x\n", (int)mldl_cfg->accel->range.mantissa);
        MPL_LOGD("slave_accel->range.fraction= %02x\n", (int)mldl_cfg->accel->range.fraction);
    } else {
        MPL_LOGD("slave_accel               = NULL\n");
    }

    if (mldl_cfg->compass) {
        MPL_LOGD("slave_compass->suspend    = %p\n", mldl_cfg->compass->suspend);
        MPL_LOGD("slave_compass->resume     = %p\n", mldl_cfg->compass->resume);
        MPL_LOGD("slave_compass->read       = %p\n", mldl_cfg->compass->read);
        MPL_LOGD("slave_compass->type       = %02x\n", mldl_cfg->compass->type);
        MPL_LOGD("slave_compass->read_reg   = %02x\n",
                 mldl_cfg->compass->read_reg);
        MPL_LOGD("slave_compass->read_len   = %02x\n",
                 mldl_cfg->compass->read_len);
        MPL_LOGD("slave_compass->endian     = %02x\n", mldl_cfg->compass->endian);
        MPL_LOGD("slave_compass->range.mantissa= %02x\n", (int)mldl_cfg->compass->range.mantissa);
        MPL_LOGD("slave_compass->range.fraction= %02x\n", (int)mldl_cfg->compass->range.fraction);
    } else {
        MPL_LOGD("slave_compass             = NULL\n");
    }

    if (mldl_cfg->pressure) {
        MPL_LOGD("slave_pressure->suspend    = %p\n", mldl_cfg->pressure->suspend);
        MPL_LOGD("slave_pressure->resume     = %p\n", mldl_cfg->pressure->resume);
        MPL_LOGD("slave_pressure->read       = %p\n", mldl_cfg->pressure->read);
        MPL_LOGD("slave_pressure->type       = %02x\n", mldl_cfg->pressure->type);
        MPL_LOGD("slave_pressure->read_reg   = %02x\n",
                 mldl_cfg->pressure->read_reg);
        MPL_LOGD("slave_pressure->read_len   = %02x\n",
                 mldl_cfg->pressure->read_len);
        MPL_LOGD("slave_pressure->endian     = %02x\n", mldl_cfg->pressure->endian);
        MPL_LOGD("slave_pressure->range.mantissa= %02x\n", (int)mldl_cfg->pressure->range.mantissa);
        MPL_LOGD("slave_pressure->range.fraction= %02x\n", (int)mldl_cfg->pressure->range.fraction);
    } else {
        MPL_LOGD("slave_pressure             = NULL\n");
    }

    MPL_LOGD("accel->get_slave_descr    = %p\n",accel->get_slave_descr);
    MPL_LOGD("accel->adapt_num          = %02x\n", accel->adapt_num);
    MPL_LOGD("accel->bus                = %02x\n", accel->bus);
    MPL_LOGD("accel->address            = %02x\n", accel->address);
    MPL_LOGD("accel->orientation        = \n"
             "                            %2d %2d %2d\n"
             "                            %2d %2d %2d\n"
             "                            %2d %2d %2d\n",
             accel->orientation[0],accel->orientation[1],accel->orientation[2],
             accel->orientation[3],accel->orientation[4],accel->orientation[5],
             accel->orientation[6],accel->orientation[7],accel->orientation[8]);
    MPL_LOGD("compass->get_slave_descr  = %p\n",compass->get_slave_descr);
    MPL_LOGD("compass->adapt_num        = %02x\n", compass->adapt_num);
    MPL_LOGD("compass->bus              = %02x\n", compass->bus);
    MPL_LOGD("compass->address          = %02x\n", compass->address);
    MPL_LOGD("compass->orientation      = \n"
             "                            %2d %2d %2d\n"
             "                            %2d %2d %2d\n"
             "                            %2d %2d %2d\n",
             compass->orientation[0],compass->orientation[1],compass->orientation[2],
             compass->orientation[3],compass->orientation[4],compass->orientation[5],
             compass->orientation[6],compass->orientation[7],compass->orientation[8]);
    MPL_LOGD("pressure->get_slave_descr  = %p\n",pressure->get_slave_descr);
    MPL_LOGD("pressure->adapt_num        = %02x\n", pressure->adapt_num);
    MPL_LOGD("pressure->bus              = %02x\n", pressure->bus);
    MPL_LOGD("pressure->address          = %02x\n", pressure->address);
    MPL_LOGD("pressure->orientation      = \n"
             "                            %2d %2d %2d\n"
             "                            %2d %2d %2d\n"
             "                            %2d %2d %2d\n",
             pressure->orientation[0],pressure->orientation[1],pressure->orientation[2],
             pressure->orientation[3],pressure->orientation[4],pressure->orientation[5],
             pressure->orientation[6],pressure->orientation[7],pressure->orientation[8]);
    
    MPL_LOGD("pdata->int_config         = %02x\n", pdata->int_config);
    MPL_LOGD("pdata->level_shifter      = %02x\n", pdata->level_shifter);
    MPL_LOGD("pdata->orientation        = \n"
             "                            %2d %2d %2d\n"
             "                            %2d %2d %2d\n"
             "                            %2d %2d %2d\n",
             pdata->orientation[0],pdata->orientation[1],pdata->orientation[2],
             pdata->orientation[3],pdata->orientation[4],pdata->orientation[5],
             pdata->orientation[6],pdata->orientation[7],pdata->orientation[8]);

    MPL_LOGD("Struct sizes: mldl_cfg: %zu, "
             "ext_slave_descr:%zu, mpu_platform_data:%zu: RamOffset: %zu\n",
             sizeof(struct mldl_cfg), sizeof(struct ext_slave_descr), 
             sizeof(struct mpu_platform_data), 
             offsetof(struct mldl_cfg, ram));
}

/******************************************************************************
 ******************************************************************************
 * Exported functions
 ******************************************************************************
 *****************************************************************************/

/** 
 * Initializes the pdata structure to defaults.
 *
 * Opens the device to read silicon revision, product id and whoami.  Leaves
 * the device in suspended state for low power.
 * 
 * @param mldl_cfg handle to the config structure
 * @param mlsl_handle handle to the mpu serial layer
 * @param accel_handle handle to the accel serial layer
 * @param compass_handle handle to the compass serial layer
 * @param pressure_handle handle to the pressure serial layer
 *
 * @return INV_SUCCESS if silicon revision, product id and woami are supported
 *         by this software.
 */
int inv_mpu_open(struct mldl_cfg *mldl_cfg, 
                 void *mlsl_handle,
                 void *accel_handle,
                 void *compass_handle,
                 void *pressure_handle)
{
    int result;
    result = ioctl((int)(uintptr_t)mlsl_handle, MPU_GET_MPU_CONFIG, mldl_cfg);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    result = inv_mpu_suspend(mldl_cfg, mlsl_handle, NULL, NULL, NULL,
                             INV_ALL_SENSORS);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    return result;
}

/** 
 * Stub for driver close.  Just verify that the devices are suspended
 * 
 * @param mldl_cfg handle to the config structure
 * @param mlsl_handle handle to the mpu serial layer
 * @param accel_handle handle to the accel serial layer
 * @param compass_handle handle to the compass serial layer
 * @param pressure_handle handle to the compass serial layer
 * 
 * @return INV_SUCCESS or non-zero error code
 */
int inv_mpu_close(struct mldl_cfg *mldl_cfg, 
		  void *mlsl_handle,
		  void *accel_handle,
		  void *compass_handle,
		  void *pressure_handle)
{
    int result = INV_SUCCESS;

    result = inv_mpu_suspend(mldl_cfg, mlsl_handle, NULL, NULL, NULL,
                             INV_ALL_SENSORS);
    return result;
}

int inv_mpu_resume(struct mldl_cfg* mldl_cfg, 
                   void *mlsl_handle, 
                   void *accel_handle, 
                   void *compass_handle, 
                   void *pressure_handle,
                   unsigned long sensors)
{
    int result;
    
    mldl_cfg->requested_sensors = sensors;
    result = ioctl((int)(uintptr_t)mlsl_handle, MPU_SET_MPU_CONFIG, mldl_cfg);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    result = ioctl((int)(uintptr_t)mlsl_handle, MPU_RESUME, NULL);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    result = ioctl((int)(uintptr_t)mlsl_handle, MPU_GET_MPU_CONFIG, mldl_cfg);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    //MPL_LOGI("%s: Resuming to %04lx\n", __func__, mldl_cfg->requested_sensors);

    return result;
}


int inv_mpu_suspend(struct mldl_cfg *mldl_cfg, 
          	    void *mlsl_handle,
                    void *accel_handle,
                    void *compass_handle,
                    void *pressure_handle,
                    unsigned long sensors)
{
    int result;
    unsigned long requested = mldl_cfg->requested_sensors;

    mldl_cfg->requested_sensors = (~sensors) & INV_ALL_SENSORS;
    //MPL_LOGI("%s: suspending sensors to %04lx\n", __func__,
    //         mldl_cfg->requested_sensors);

    result = ioctl((int)(uintptr_t)mlsl_handle, MPU_SET_MPU_CONFIG, mldl_cfg);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    result = ioctl((int)(uintptr_t)mlsl_handle, MPU_SUSPEND, NULL);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    result = ioctl((int)(uintptr_t)mlsl_handle, MPU_GET_MPU_CONFIG, mldl_cfg);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    mldl_cfg->requested_sensors = requested;
    //MPL_LOGI("%s: Will resume next to %04lx\n", __func__, requested);

    return result;
}

/**
 * Send slave configuration information
 *
 * @param mldl_cfg pointer to the mldl configuration structure
 * @param gyro_handle handle to the gyro sensor
 * @param slave_handle handle to the slave sensor
 * @param slave slave description
 * @param pdata slave platform data
 * @param data where to store the read data
 *
 * @return 0 or non-zero error code
 */
int inv_mpu_slave_read(struct mldl_cfg *mldl_cfg,
		void *gyro_handle,
		void *slave_handle,
		struct ext_slave_descr *slave,
		struct ext_slave_platform_data *pdata,
		unsigned char *data)
{
    int result;
    if (!mldl_cfg || !gyro_handle || !data || !slave) {
        LOG_RESULT_LOCATION(INV_ERROR_INVALID_PARAMETER);
        return INV_ERROR_INVALID_PARAMETER;
    }

    switch (slave->type) {
    case EXT_SLAVE_TYPE_ACCELEROMETER:
        result = ioctl((int)(uintptr_t)gyro_handle, MPU_READ_ACCEL, data);
        break;
    case EXT_SLAVE_TYPE_COMPASS:
        result = ioctl((int)(uintptr_t)gyro_handle, MPU_READ_COMPASS, data);
        break;
    case EXT_SLAVE_TYPE_PRESSURE:
        result = ioctl((int)(uintptr_t)gyro_handle, MPU_READ_PRESSURE, data);
        break;
    default:
        LOG_RESULT_LOCATION(INV_ERROR_INVALID_PARAMETER);
        return INV_ERROR_INVALID_PARAMETER;
        break;
    }

    return result;
}

/**
 * Send slave configuration information
 *
 * @param mldl_cfg pointer to the mldl configuration structure
 * @param gyro_handle handle to the gyro sensor
 * @param slave_handle handle to the slave sensor
 * @param data the data being sent
 * @param slave slave description
 * @param pdata slave platform data
 *
 * @return 0 or non-zero error code
 */
int inv_mpu_slave_config(struct mldl_cfg *mldl_cfg,
                     void *gyro_handle,
                     void *slave_handle,
                     struct ext_slave_config *data,
                     struct ext_slave_descr *slave,
                     struct ext_slave_platform_data *pdata)
{
    int result;
    if (!mldl_cfg || !data || !slave || !pdata || !slave) {
        LOG_RESULT_LOCATION(INV_ERROR_INVALID_PARAMETER);
        return INV_ERROR_INVALID_PARAMETER;
    }

    switch (slave->type) {
    case EXT_SLAVE_TYPE_ACCELEROMETER:
        result = ioctl((int)(uintptr_t)gyro_handle, MPU_CONFIG_ACCEL, data);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
        break;
    case EXT_SLAVE_TYPE_COMPASS:
        result = ioctl((int)(uintptr_t)gyro_handle, MPU_CONFIG_COMPASS, data);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
        break;
    case EXT_SLAVE_TYPE_PRESSURE:
        result = ioctl((int)(uintptr_t)gyro_handle, MPU_CONFIG_PRESSURE, data);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
        break;
    default:
        LOG_RESULT_LOCATION(INV_ERROR_INVALID_PARAMETER);
        return INV_ERROR_INVALID_PARAMETER;
        break;
    }

    return result;
}

/**
 * Request slave configuration information
 *
 * Use this specifically after requesting a slave configuration to see what the
 * slave accually accepted.
 *
 * @param mldl_cfg pointer to the mldl configuration structure
 * @param gyro_handle handle to the gyro sensor
 * @param slave_handle handle to the slave sensor
 * @param data the data being requested.
 * @param slave slave description
 * @param pdata slave platform data
 *
 * @return 0 or non-zero error code
 */
int inv_mpu_get_slave_config(struct mldl_cfg *mldl_cfg,
                         void *gyro_handle,
                         void *slave_handle,
                         struct ext_slave_config *data,
                         struct ext_slave_descr *slave,
                         struct ext_slave_platform_data *pdata)
{
    int result;
    if (!mldl_cfg || !data || !slave) {
        LOG_RESULT_LOCATION(INV_ERROR_INVALID_PARAMETER);
        return INV_ERROR_INVALID_PARAMETER;
    }
    switch (slave->type) {
    case EXT_SLAVE_TYPE_ACCELEROMETER:
        result = ioctl((int)(uintptr_t)gyro_handle, MPU_GET_CONFIG_ACCEL, data);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
        break;
    case EXT_SLAVE_TYPE_COMPASS:
        result = ioctl((int)(uintptr_t)gyro_handle, MPU_GET_CONFIG_COMPASS, data);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
        break;
    case EXT_SLAVE_TYPE_PRESSURE:
        result = ioctl((int)(uintptr_t)gyro_handle, MPU_GET_CONFIG_PRESSURE, data);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
        break;
    default:
        LOG_RESULT_LOCATION(INV_ERROR_INVALID_PARAMETER);
        return INV_ERROR_INVALID_PARAMETER;
        break;
    }
    return result;
}
/**
 *@}
 */
