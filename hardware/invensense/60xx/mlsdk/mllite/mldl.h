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
 * $Id: mldl.h 5639 2011-06-14 01:23:05Z nroyer $
 *
 *******************************************************************************/

#ifndef MLDL_H
#define MLDL_H

#include "mltypes.h"
#include "mlsl.h"
#include <linux/mpu.h>
#include "mldl_cfg.h"
#ifdef INV_INCLUDE_LEGACY_HEADERS
#include "mldl_legacy.h"
#endif

    /* ------------ */
    /* - Defines. - */
    /* ------------ */

typedef enum _DEVICE_CONFIG {
    DEVICE_MPU_ACCEL = 0,
    DEVICE_MPU,
    NUM_OF_DEVICES
} DEVICE_CONFIG;

#define SERIAL_I2C                  0
#define SERIAL_SPI                  1

#define DATAMODE_MANUAL             0   // Manual data mode
#define DATAMODE_AUTO               1   // Auto data mode

#define DATASRC_IMMEDIATE           0   // Return data immediately
#define DATASRC_WHENREADY           1   // Only return data when new data is available
#define DATASRC_FIFO                2   // Use FIFO for data

#define SENSOR_DATA_GYROX           0x0001
#define SENSOR_DATA_GYROY           0x0002
#define SENSOR_DATA_GYROZ           0x0004
#define SENSOR_DATA_ACCELX          0x0008
#define SENSOR_DATA_ACCELY          0x0010
#define SENSOR_DATA_ACCELZ          0x0020
#define SENSOR_DATA_AUX1            0x0040
#define SENSOR_DATA_AUX2            0x0080
#define SENSOR_DATA_AUX3            0x0100
#define SENSOR_DATA_TEMP            0x0200

#define INTPIN_MPU                  0

#define INTLOGIC_HIGH               0
#define INTLOGIC_LOW                1

#define INTTYPE_PUSHPULL            0
#define INTTYPE_OPENDRAIN           1

#define INTLATCH_DISABLE            0
#define INTLATCH_ENABLE             1

#define MPUINT_MPU_READY            0x04
#define MPUINT_DMP_DONE             0x02
#define MPUINT_DATA_READY           0x01

#define INTLATCHCLEAR_READSTATUS    0
#define INTLATCHCLEAR_ANYREAD       1

#define DMP_DONTRUN                 0
#define DMP_RUN                     1

    /*---- defines for external interrupt status (via external call into library) ----*/
#define INT_CLEAR                   0
#define INT_TRIGGERED               1

typedef enum {
    INTSRC_MPU = 0,
    INTSRC_AUX1,
    INTSRC_AUX2,
    INTSRC_AUX3,
    INTSRC_TIMER,
    INTSRC_UNKNOWN,
    INTSRC_MPU_FIFO,
    INTSRC_MPU_MOTION,
    NUM_OF_INTSOURCES,
} INT_SOURCE;

    /* --------------- */
    /* - Structures. - */
    /* --------------- */

    /* --------------- */
    /* - Variables.  - */
    /* --------------- */

    /* --------------------- */
    /* - Function p-types. - */
    /* --------------------- */
#ifdef __cplusplus
extern "C" {
#endif

    inv_error_t inv_dl_open(void *mlslHandle);
    inv_error_t inv_dl_close(void);

    inv_error_t inv_dl_start(unsigned long sensors);
    inv_error_t inv_dl_stop(unsigned long sensors);

    struct mldl_cfg *inv_get_dl_config(void);

    inv_error_t inv_init_requested_sensors(unsigned long sensors);
    unsigned char inv_get_mpu_slave_addr(void);
    inv_error_t inv_get_dl_ctrl_dmp(unsigned char enableRun,
                                    unsigned char enableFIFO);
    inv_error_t inv_get_dl_cfg_int(unsigned char triggers);
    inv_error_t inv_dl_cfg_sampling(unsigned char lpf, unsigned char divider);
    inv_error_t inv_set_full_scale(float fullScale);
    inv_error_t inv_set_external_sync(unsigned char extSync);
    inv_error_t inv_set_ignore_system_suspend(unsigned char ignore);
    inv_error_t inv_clock_source(unsigned char clkSource);
    inv_error_t inv_get_mpu_memory(unsigned short key,
                                   unsigned short length,
                                   unsigned char *buffer);
    inv_error_t inv_set_mpu_memory(unsigned short key,
                                   unsigned short length,
                                   const unsigned char *buffer);
    inv_error_t inv_load_dmp(const unsigned char *buffer,
                             unsigned short length,
                             unsigned short startAddress);

    unsigned char inv_get_slicon_rev(void);
    inv_error_t inv_set_offsetTC(const unsigned char *tc);
    inv_error_t inv_set_offset(const short *offset);

    /* Functions for setting and retrieving the DMP memory */
    inv_error_t inv_get_mpu_memory_original(unsigned short key,
                                            unsigned short length,
                                            unsigned char *buffer);
    void inv_set_get_address(unsigned short (*func) (unsigned short key));
    unsigned short inv_dl_get_address(unsigned short key);
    uint_fast8_t inv_dmpkey_supported(unsigned short key);

    inv_error_t inv_get_interrupt_status(unsigned char intPin,
                                         unsigned char *value);
    unsigned char inv_get_interrupt_trigger(unsigned char index);
    void inv_clear_interrupt_trigger(unsigned char index);
    inv_error_t inv_interrupt_handler(unsigned char intSource);

    /** Only exposed for testing purposes */
    inv_error_t inv_set_mpu_memory_one_bank(unsigned char bank,
                                            unsigned short memAddr,
                                            unsigned short length,
                                            const unsigned char *buffer);
    inv_error_t inv_get_mpu_memory_one_bank(unsigned char bank,
                                            unsigned char memAddr,
                                            unsigned short length,
                                            unsigned char *buffer);
#ifdef __cplusplus
}
#endif
#endif                          // MLDL_H
