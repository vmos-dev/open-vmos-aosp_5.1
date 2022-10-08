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
 * $Id: mlFIFO.c 5653 2011-06-16 21:06:55Z nroyer $
 *
 *******************************************************************************/

/**
 *   @defgroup MLFIFO
 *   @brief Motion Library - FIFO Driver.
 *          The FIFO API Interface.
 *
 *   @{
 *       @file mlFIFO.c
 *       @brief FIFO Interface.
**/

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
#include "mlFIFO.h"
#include "mlFIFOHW.h"
#include "dmpKey.h"
#include "mlMathFunc.h"
#include "ml.h"
#include "mldl.h"
#include "mldl_cfg.h"
#include "mlstates.h"
#include "mlsupervisor.h"
#include "mlos.h"
#include "mlmath.h"
#include "accel.h"

#include "log.h"
#undef MPL_LOG_TAG
#define MPL_LOG_TAG "MPL-fifo"

#define FIFO_DEBUG 0

#define REF_QUATERNION             (0)
#define REF_GYROS                  (REF_QUATERNION + 4)
#define REF_CONTROL                (REF_GYROS + 3)
#define REF_RAW                    (REF_CONTROL + 4)
#define REF_RAW_EXTERNAL           (REF_RAW + 8)
#define REF_ACCEL                  (REF_RAW_EXTERNAL + 6)
#define REF_QUANT_ACCEL            (REF_ACCEL + 3)
#define REF_QUATERNION_6AXIS       (REF_QUANT_ACCEL + INV_MAX_NUM_ACCEL_SAMPLES)
#define REF_EIS                    (REF_QUATERNION_6AXIS + 4)
#define REF_DMP_PACKET             (REF_EIS + 3)
#define REF_GARBAGE                (REF_DMP_PACKET + 1)
#define REF_LAST                   (REF_GARBAGE + 1)

long fifo_scale[REF_LAST] = {
    (1L << 30), (1L << 30), (1L << 30), (1L << 30), // Quaternion
    // 2^(16+30)/((2^30)*((3.14159265358/180)/200)/2)
    1501974482L, 1501974482L, 1501974482L,  // Gyro
    (1L << 30), (1L << 30), (1L << 30), (1L << 30), // Control
    (1L << 14),                 // Temperature
    (1L << 14), (1L << 14), (1L << 14), // Raw Gyro
    (1L << 14), (1L << 14), (1L << 14), (0),    // Raw Accel, plus padding
    (1L << 14), (1L << 14), (1L << 14), // Raw External
    (1L << 14), (1L << 14), (1L << 14), // Raw External
    (1L << 16), (1L << 16), (1L << 16), // Accel
    (1L << 30), (1L << 30), (1L << 30), (1L << 30), // Quant Accel
    (1L << 30), (1L << 30), (1L << 30), (1L << 30), //Quant Accel
    (1L << 30), (1L << 30), (1L << 30), (1L << 30), // Quaternion 6 Axis
    (1L << 30), (1L << 30), (1L << 30), // EIS
    (1L << 30),                 // Packet
    (1L << 30),                 // Garbage
};

// The scale factors for tap need to match the number in fifo_scale above.
// fifo_base_offset below may also need to be changed if this is not 8
#if INV_MAX_NUM_ACCEL_SAMPLES != 8
#error  INV_MAX_NUM_ACCEL_SAMPLES must be defined to 8
#endif

#define CONFIG_QUAT                (0)
#define CONFIG_GYROS               (CONFIG_QUAT + 1)
#define CONFIG_CONTROL_DATA        (CONFIG_GYROS + 1)
#define CONFIG_TEMPERATURE         (CONFIG_CONTROL_DATA + 1)
#define CONFIG_RAW_DATA            (CONFIG_TEMPERATURE + 1)
#define CONFIG_RAW_EXTERNAL        (CONFIG_RAW_DATA + 1)
#define CONFIG_ACCEL               (CONFIG_RAW_EXTERNAL + 1)
#define CONFIG_DMP_QUANT_ACCEL     (CONFIG_ACCEL + 1)
#define CONFIG_EIS                 (CONFIG_DMP_QUANT_ACCEL + 1)
#define CONFIG_DMP_PACKET_NUMBER   (CONFIG_EIS + 1)
#define CONFIG_FOOTER              (CONFIG_DMP_PACKET_NUMBER + 1)
#define NUMFIFOELEMENTS            (CONFIG_FOOTER + 1)

const int fifo_base_offset[NUMFIFOELEMENTS] = {
    REF_QUATERNION * 4,
    REF_GYROS * 4,
    REF_CONTROL * 4,
    REF_RAW * 4,
    REF_RAW * 4 + 4,
    REF_RAW_EXTERNAL * 4,
    REF_ACCEL * 4,
    REF_QUANT_ACCEL * 4,
    REF_EIS * 4,
    REF_DMP_PACKET * 4,
    REF_GARBAGE * 4
};

struct fifo_obj {
    void (*fifo_process_cb) (void);
    long decoded[REF_LAST];
    long decoded_accel[INV_MAX_NUM_ACCEL_SAMPLES][ACCEL_NUM_AXES];
    int offsets[REF_LAST * 4];
    int cache;
    uint_fast8_t gyro_source;
    unsigned short fifo_rate;
    unsigned short sample_step_size_ms;
    uint_fast16_t fifo_packet_size;
    uint_fast16_t data_config[NUMFIFOELEMENTS];
    unsigned char reference_count[REF_LAST];
    long acc_bias_filt[3];
    float acc_filter_coef;
    long gravity_cache[3];
};

static struct fifo_obj fifo_obj;

#define FIFO_CACHE_TEMPERATURE 1
#define FIFO_CACHE_GYRO 2
#define FIFO_CACHE_GRAVITY_BODY 4
#define FIFO_CACHE_ACC_BIAS 8

struct fifo_rate_obj {
    // These describe callbacks happening everytime a FIFO block is processed
    int_fast8_t num_cb;
    HANDLE mutex;
    inv_obj_func fifo_process_cb[MAX_HIGH_RATE_PROCESSES];
    int priority[MAX_HIGH_RATE_PROCESSES];
};

struct fifo_rate_obj fifo_rate_obj;

/** Sets accuracy to be one of 0, INV_32_BIT, or INV_16_BIT. Looks up old 
 *  accuracy if needed.
 */
static uint_fast16_t inv_set_fifo_accuracy(uint_fast16_t elements,
                                           uint_fast16_t accuracy,
                                           uint_fast8_t configOffset)
{
    if (elements) {
        if (!accuracy)
            accuracy = fifo_obj.data_config[configOffset];
        else if (accuracy & INV_16_BIT)
            if ((fifo_obj.data_config[configOffset] & INV_32_BIT))
                accuracy = INV_32_BIT;  // 32-bits takes priority
            else
                accuracy = INV_16_BIT;
        else
            accuracy = INV_32_BIT;
    } else {
        accuracy = 0;
    }

    return accuracy;
}

/** Adjusts (len) Reference Counts, at offset (refOffset). If increment is 0, 
 * the reference counts are subtracted, otherwise they are incremented for each
 * bit set in element. The value returned are the elements that should be sent
 * out as data through the FIFO.
*/
static uint_fast16_t inv_set_fifo_reference(uint_fast16_t elements,
                                            uint_fast16_t increment,
                                            uint_fast8_t refOffset,
                                            uint_fast8_t len)
{
    uint_fast8_t kk;

    if (increment == 0) {
        for (kk = 0; kk < len; ++kk) {
            if ((elements & 1)
                && (fifo_obj.reference_count[kk + refOffset] > 0)) {
                fifo_obj.reference_count[kk + refOffset]--;
            }
            elements >>= 1;
        }
    } else {
        for (kk = 0; kk < len; ++kk) {
            if (elements & 1)
                fifo_obj.reference_count[kk + refOffset]++;
            elements >>= 1;
        }
    }
    elements = 0;
    for (kk = 0; kk < len; ++kk) {
        if (fifo_obj.reference_count[kk + refOffset] > 0)
            elements |= (1 << kk);
    }
    return elements;
}

/**
 * @param[in] accuracy INV_16_BIT or INV_32_BIT when constructing data to send
 *  out the FIFO, 0 when removing from the FIFO.
 */
static inv_error_t inv_construct3_fifo(unsigned char *regs,
                                       uint_fast16_t elements,
                                       uint_fast16_t accuracy,
                                       uint_fast8_t refOffset,
                                       unsigned short key,
                                       uint_fast8_t configOffset)
{
    int_fast8_t kk;
    inv_error_t result;

    elements = inv_set_fifo_reference(elements, accuracy, refOffset, 3);
    accuracy = inv_set_fifo_accuracy(elements, accuracy, configOffset);

    if (accuracy & INV_16_BIT) {
        regs[0] = DINAF8 + 2;
    }

    fifo_obj.data_config[configOffset] = elements | accuracy;

    for (kk = 0; kk < 3; ++kk) {
        if ((elements & 1) == 0)
            regs[kk + 1] = DINAA0 + 3;
        elements >>= 1;
    }

    result = inv_set_mpu_memory(key, 4, regs);

    return result;
}

/** 
 * @internal
 * Puts footer on FIFO data.
 */
static inv_error_t inv_set_footer(void)
{
    unsigned char regs = DINA30;
    uint_fast8_t tmp_count;
    int_fast8_t i, j;
    int offset;
    int result;
    int *fifo_offsets_ptr = fifo_obj.offsets;

    fifo_obj.fifo_packet_size = 0;
    for (i = 0; i < NUMFIFOELEMENTS; i++) {
        tmp_count = 0;
        offset = fifo_base_offset[i];
        for (j = 0; j < 8; j++) {
            if ((fifo_obj.data_config[i] >> j) & 0x0001) {
#ifndef BIG_ENDIAN
                // Special Case for Byte Ordering on Accel Data
                if ((i == CONFIG_RAW_DATA) && (j > 2)) {
                    tmp_count += 2;
                    switch (inv_get_dl_config()->accel->endian) {
                    case EXT_SLAVE_BIG_ENDIAN:
                        *fifo_offsets_ptr++ = offset + 3;
                        *fifo_offsets_ptr++ = offset + 2;
                        break;
                    case EXT_SLAVE_LITTLE_ENDIAN:
                        *fifo_offsets_ptr++ = offset + 2;
                        *fifo_offsets_ptr++ = offset + 3;
                        break;
                    case EXT_SLAVE_FS8_BIG_ENDIAN:
                        if (j == 3) {
                            // Throw this byte away
                            *fifo_offsets_ptr++ =
                                fifo_base_offset[CONFIG_FOOTER];
                            *fifo_offsets_ptr++ = offset + 3;
                        } else if (j == 4) {
                            *fifo_offsets_ptr++ = offset + 3;
                            *fifo_offsets_ptr++ = offset + 7;
                        } else {
                            // Throw these byte away
                            *fifo_offsets_ptr++ =
                                fifo_base_offset[CONFIG_FOOTER];
                            *fifo_offsets_ptr++ =
                                fifo_base_offset[CONFIG_FOOTER];
                        }
                        break;
                    case EXT_SLAVE_FS16_BIG_ENDIAN:
                        if (j == 3) {
                            // Throw this byte away
                            *fifo_offsets_ptr++ =
                                fifo_base_offset[CONFIG_FOOTER];
                            *fifo_offsets_ptr++ = offset + 3;
                        } else if (j == 4) {
                            *fifo_offsets_ptr++ = offset - 2;
                            *fifo_offsets_ptr++ = offset + 3;
                        } else {
                            *fifo_offsets_ptr++ = offset - 2;
                            *fifo_offsets_ptr++ = offset + 3;
                        }
                        break;
                    default:
                        return INV_ERROR;    // Bad value on ordering
                    }
                } else {
                    tmp_count += 2;
                    *fifo_offsets_ptr++ = offset + 3;
                    *fifo_offsets_ptr++ = offset + 2;
                    if (fifo_obj.data_config[i] & INV_32_BIT) {
                        *fifo_offsets_ptr++ = offset + 1;
                        *fifo_offsets_ptr++ = offset;
                        tmp_count += 2;
                    }
                }
#else
                // Big Endian Platform
                // Special Case for Byte Ordering on Accel Data
                if ((i == CONFIG_RAW_DATA) && (j > 2)) {
                    tmp_count += 2;
                    switch (inv_get_dl_config()->accel->endian) {
                    case EXT_SLAVE_BIG_ENDIAN:
                        *fifo_offsets_ptr++ = offset + 2;
                        *fifo_offsets_ptr++ = offset + 3;
                        break;
                    case EXT_SLAVE_LITTLE_ENDIAN:
                        *fifo_offsets_ptr++ = offset + 3;
                        *fifo_offsets_ptr++ = offset + 2;
                        break;
                    case EXT_SLAVE_FS8_BIG_ENDIAN:
                        if (j == 3) {
                            // Throw this byte away
                            *fifo_offsets_ptr++ =
                                fifo_base_offset[CONFIG_FOOTER];
                            *fifo_offsets_ptr++ = offset;
                        } else if (j == 4) {
                            *fifo_offsets_ptr++ = offset;
                            *fifo_offsets_ptr++ = offset + 4;
                        } else {
                            // Throw these bytes away
                            *fifo_offsets_ptr++ =
                                fifo_base_offset[CONFIG_FOOTER];
                            *fifo_offsets_ptr++ =
                                fifo_base_offset[CONFIG_FOOTER];
                        }
                        break;
                    case EXT_SLAVE_FS16_BIG_ENDIAN:
                        if (j == 3) {
                            // Throw this byte away
                            *fifo_offsets_ptr++ =
                                fifo_base_offset[CONFIG_FOOTER];
                            *fifo_offsets_ptr++ = offset;
                        } else if (j == 4) {
                            *fifo_offsets_ptr++ = offset - 3;
                            *fifo_offsets_ptr++ = offset;
                        } else {
                            *fifo_offsets_ptr++ = offset - 3;
                            *fifo_offsets_ptr++ = offset;
                        }
                        break;
                    default:
                        return INV_ERROR;    // Bad value on ordering
                    }
                } else {
                    tmp_count += 2;
                    *fifo_offsets_ptr++ = offset;
                    *fifo_offsets_ptr++ = offset + 1;
                    if (fifo_obj.data_config[i] & INV_32_BIT) {
                        *fifo_offsets_ptr++ = offset + 2;
                        *fifo_offsets_ptr++ = offset + 3;
                        tmp_count += 2;
                    }
                }

#endif
            }
            offset += 4;
        }
        fifo_obj.fifo_packet_size += tmp_count;
    }
    if (fifo_obj.data_config[CONFIG_FOOTER] == 0 &&
        fifo_obj.fifo_packet_size > 0) {
        // Add footer
        result = inv_set_mpu_memory(KEY_CFG_16, 1, &regs);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
        fifo_obj.data_config[CONFIG_FOOTER] = 0x0001 | INV_16_BIT;
        fifo_obj.fifo_packet_size += 2;
    } else if (fifo_obj.data_config[CONFIG_FOOTER] &&
               (fifo_obj.fifo_packet_size == 2)) {
        // Remove Footer
        regs = DINAA0 + 3;
        result = inv_set_mpu_memory(KEY_CFG_16, 1, &regs);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
        fifo_obj.data_config[CONFIG_FOOTER] = 0;
        fifo_obj.fifo_packet_size = 0;
    }

    return INV_SUCCESS;
}

inv_error_t inv_decode_quantized_accel(void)
{
    int kk;
    int fifoRate = inv_get_fifo_rate();

    if (!fifo_obj.data_config[CONFIG_DMP_QUANT_ACCEL])
        return INV_ERROR_FEATURE_NOT_ENABLED;

    for (kk = (INV_MAX_NUM_ACCEL_SAMPLES - (fifoRate + 1));
         kk < INV_MAX_NUM_ACCEL_SAMPLES; kk++) {
        union {
            unsigned int u10:10;
            signed int s10:10;
        } temp;

        union {
            uint32_t u32;
            int32_t s32;
        } value;

        value.u32 = fifo_obj.decoded[REF_QUANT_ACCEL + kk];
        // unquantize this samples.  
        // They are stored as x * 2^20 + y * 2^10 + z
        // Z
        temp.u10 = value.u32 & 0x3ff;
        value.s32 -= temp.s10;
        fifo_obj.decoded_accel[kk][2] = temp.s10 * 64;
        // Y
        value.s32 = value.s32 / 1024;
        temp.u10 = value.u32 & 0x3ff;
        value.s32 -= temp.s10;
        fifo_obj.decoded_accel[kk][1] = temp.s10 * 64;
        // X
        value.s32 = value.s32 / 1024;
        temp.u10 = value.u32 & 0x3ff;
        fifo_obj.decoded_accel[kk][0] = temp.s10 * 64;
    }
    return INV_SUCCESS;
}

static inv_error_t inv_state_change_fifo(unsigned char newState)
{
    inv_error_t result = INV_SUCCESS;
    unsigned char regs[4];
    struct mldl_cfg *mldl_cfg = inv_get_dl_config();

    /* Don't reset the fifo on a fifo rate change */
    if ((mldl_cfg->requested_sensors & INV_DMP_PROCESSOR) &&
        (newState != inv_get_state()) && (inv_dmpkey_supported(KEY_D_1_178))) {
        /* Delay output on restart by 50ms due to warm up time of gyros */

        short delay = (short)-((50 / inv_get_sample_step_size_ms()) + 1);
        inv_init_fifo_hardare();
        inv_int16_to_big8(delay, regs);
        result = inv_set_mpu_memory(KEY_D_1_178, 2, regs);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
    }

    if (INV_STATE_DMP_STARTED == newState) {
        if (inv_dmpkey_supported(KEY_D_1_128)) {
            double tmp;
            tmp = (0x20000000L * M_PI) / (fifo_obj.fifo_rate + 1);
            if (tmp > 0x40000000L)
                tmp = 0x40000000L;
            (void)inv_int32_to_big8((long)tmp, regs);
            result = inv_set_mpu_memory(KEY_D_1_128, sizeof(long), regs);
            if (result) {
                LOG_RESULT_LOCATION(result);
                return result;
            }
            result = inv_reset_fifo();
            if (result) {
                LOG_RESULT_LOCATION(result);
                return result;
            }
        }
    }
    return result;
}

/**
 * @internal
 * @brief get the FIFO packet size
 * @return the FIFO packet size
 */
uint_fast16_t inv_get_fifo_packet_size(void)
{
    return fifo_obj.fifo_packet_size;
}

/**
 *  @brief  Initializes all the internal static variables for 
 *          the FIFO module.
 *  @note   Should be called by the initialization routine such 
 *          as inv_dmp_open().
 *  @return INV_SUCCESS if successful, a non-zero error code otherwise. 
 */
inv_error_t inv_init_fifo_param(void)
{
    inv_error_t result;
    memset(&fifo_obj, 0, sizeof(struct fifo_obj));
    fifo_obj.decoded[REF_QUATERNION] = 1073741824L; // Set to Identity
    inv_set_linear_accel_filter_coef(0.f);
    fifo_obj.fifo_rate = 20;
    fifo_obj.sample_step_size_ms = 100;
    memset(&fifo_rate_obj, 0, sizeof(struct fifo_rate_obj));
    result = inv_create_mutex(&fifo_rate_obj.mutex);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    result = inv_register_state_callback(inv_state_change_fifo);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    return result;
}

/**
 *  @brief  Close the FIFO usage.
 *  @return INV_SUCCESS if successful, a non-zero error code otherwise.
 */
inv_error_t inv_close_fifo(void)
{
    inv_error_t result;
    inv_error_t first = INV_SUCCESS;
    result = inv_unregister_state_callback(inv_state_change_fifo);
    ERROR_CHECK_FIRST(first, result);
    result = inv_destroy_mutex(fifo_rate_obj.mutex);
    ERROR_CHECK_FIRST(first, result);
    memset(&fifo_rate_obj, 0, sizeof(struct fifo_rate_obj));
    return first;
}

/** 
 * Set the gyro source to output to the fifo
 * 
 * @param source The source.  One of 
 * - INV_GYRO_FROM_RAW
 * - INV_GYRO_FROM_QUATERNION
 * 
 * @return INV_SUCCESS or non-zero error code;
 */
inv_error_t inv_set_gyro_data_source(uint_fast8_t source)
{
    if (source != INV_GYRO_FROM_QUATERNION && source != INV_GYRO_FROM_RAW) {
        return INV_ERROR_INVALID_PARAMETER;
    }

    fifo_obj.gyro_source = source;
    return INV_SUCCESS;
}

/** 
 *  @brief  Reads and processes FIFO data. Also handles callbacks when data is
 *          ready.
 *  @param  numPackets 
 *              Number of FIFO packets to try to read. You should
 *              use a large number here, such as 100, if you want to read all
 *              the full packets in the FIFO, which is typical operation.
 *  @param  processed
 *              The number of FIFO packets processed. This may be incremented
 *              even if high rate processes later fail.
 *  @return INV_SUCCESS if successful, a non-zero error code otherwise.
 */
inv_error_t inv_read_and_process_fifo(int_fast8_t numPackets,
                                      int_fast8_t * processed)
{
    int_fast8_t packet;
    inv_error_t result = INV_SUCCESS;
    uint_fast16_t read;
    struct mldl_cfg *mldl_cfg = inv_get_dl_config();
    int kk;

    if (NULL == processed)
        return INV_ERROR_INVALID_PARAMETER;

    *processed = 0;
    if (fifo_obj.fifo_packet_size == 0)
        return result;          // Nothing to read

    for (packet = 0; packet < numPackets; ++packet) {
        if (mldl_cfg->requested_sensors & INV_DMP_PROCESSOR) {
            unsigned char footer_n_data[MAX_FIFO_LENGTH + FIFO_FOOTER_SIZE];
            unsigned char *buf = &footer_n_data[FIFO_FOOTER_SIZE];
            read = inv_get_fifo((uint_fast16_t) fifo_obj.fifo_packet_size,
                                footer_n_data);
            if (0 == read ||
                read != fifo_obj.fifo_packet_size - FIFO_FOOTER_SIZE) {
                result = inv_get_fifo_status();
                if (INV_SUCCESS != result) {
                    memset(fifo_obj.decoded, 0, sizeof(fifo_obj.decoded));
                }
                return result;
            }

            result = inv_process_fifo_packet(buf);
            if (result) {
                LOG_RESULT_LOCATION(result);
                return result;
            }
        } else if (inv_accel_present()) {
            long data[ACCEL_NUM_AXES];
            result = inv_get_accel_data(data);
            if (result == INV_ERROR_ACCEL_DATA_NOT_READY) {
                return INV_SUCCESS;
            }
            if (result) {
                LOG_RESULT_LOCATION(result);
                return result;
            }

            memset(fifo_obj.decoded, 0, sizeof(fifo_obj.decoded));
            fifo_obj.cache = 0;
            for (kk = 0; kk < ACCEL_NUM_AXES; ++kk) {
                fifo_obj.decoded[REF_RAW + 4 + kk] =
                    inv_q30_mult((data[kk] << 16),
                                 fifo_scale[REF_RAW + 4 + kk]);
                fifo_obj.decoded[REF_ACCEL + kk] =
                    inv_q30_mult((data[kk] << 15), fifo_scale[REF_ACCEL + kk]);
                fifo_obj.decoded[REF_ACCEL + kk] -=
                    inv_obj.scaled_accel_bias[kk];
            }
        }
        // The buffer was processed correctly, so increment even if
        // other processes fail later, which will return an error
        *processed = *processed + 1;

        if ((fifo_obj.fifo_rate < INV_MAX_NUM_ACCEL_SAMPLES) &&
            fifo_obj.data_config[CONFIG_DMP_QUANT_ACCEL]) {
            result = inv_decode_quantized_accel();
            if (result) {
                LOG_RESULT_LOCATION(result);
                return result;
            }
        }

        if (fifo_obj.data_config[CONFIG_QUAT]) {
            result = inv_accel_compass_supervisor();
            if (result) {
                LOG_RESULT_LOCATION(result);
                return result;
            }
        }

        result = inv_pressure_supervisor();
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }

        // Callbacks now that we have a buffer of data ready
        result = inv_run_fifo_rate_processes();
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }

    }
    return result;
}

/**
 *  @brief  inv_set_fifo_processed_callback is used to set a processed data callback
 *          function.  inv_set_fifo_processed_callback sets a user defined callback
 *          function that triggers when all the decoding has been finished by
 *          the motion processing engines. It is called before other bigger 
 *          processing engines to allow lower latency for the user.
 *
 *  @pre    inv_dmp_open() 
 *          @ifnot MPL_MF 
 *              or inv_open_low_power_pedometer() 
 *              or inv_eis_open_dmp()
 *          @endif
 *          and inv_dmp_start() 
 *          must <b>NOT</b> have been called.
 *
 *  @param  func    A user defined callback function.
 *
 *  @return INV_SUCCESS if successful, or non-zero error code otherwise.
 */
inv_error_t inv_set_fifo_processed_callback(void (*func) (void))
{
    INVENSENSE_FUNC_START;

    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    fifo_obj.fifo_process_cb = func;

    return INV_SUCCESS;
}

/**
 * @internal
 * @brief   Process data from the dmp read via the fifo.  Takes a buffer 
 *          filled with bytes read from the DMP FIFO. 
 *          Currently expects only the 16 bytes of quaternion data. 
 *          Calculates the motion parameters from that data and stores the 
 *          results in an internal data structure.
 * @param[in,out]   dmpData     Pointer to the buffer containing the fifo data.
 * @return  INV_SUCCESS or error code.
**/
inv_error_t inv_process_fifo_packet(const unsigned char *dmpData)
{
    INVENSENSE_FUNC_START;
    int N, kk;
    unsigned char *p;

    p = (unsigned char *)(&fifo_obj.decoded);
    N = fifo_obj.fifo_packet_size;
    if (N > sizeof(fifo_obj.decoded))
        return INV_ERROR_ASSERTION_FAILURE;

    memset(&fifo_obj.decoded, 0, sizeof(fifo_obj.decoded));

    for (kk = 0; kk < N; ++kk) {
        p[fifo_obj.offsets[kk]] = *dmpData++;
    }

    // If multiplies are much greater cost than if checks, you could check
    // to see if fifo_scale is non-zero first, or equal to (1L<<30)
    for (kk = 0; kk < REF_LAST; ++kk) {
        fifo_obj.decoded[kk] =
            inv_q30_mult(fifo_obj.decoded[kk], fifo_scale[kk]);
    }

    memcpy(&fifo_obj.decoded[REF_QUATERNION_6AXIS],
           &fifo_obj.decoded[REF_QUATERNION], 4 * sizeof(long));

    inv_obj.flags[INV_PROCESSED_DATA_READY] = 1;
    fifo_obj.cache = 0;

    return INV_SUCCESS;
}

/** Converts 16-bit temperature data as read from temperature register
* into Celcius scaled by 2^16.
*/
long inv_decode_temperature(short tempReg)
{
#if defined CONFIG_MPU_SENSORS_MPU6050A2
    // Celcius = 35 + (T + 3048.7)/325.9
    return 2906830L + inv_q30_mult((long)tempReg << 16, 3294697L);
#endif
#if defined CONFIG_MPU_SENSORS_MPU6050B1
    // Celcius = 35 + (T + 927.4)/360.6
    return 2462307L + inv_q30_mult((long)tempReg << 16, 2977653L);
#endif
#if defined CONFIG_MPU_SENSORS_MPU3050
    // Celcius = 35 + (T + 13200)/280
    return 5383314L + inv_q30_mult((long)tempReg << 16, 3834792L);
#endif
}

/**  @internal
* Returns the temperature in hardware units. The scaling may change from part to part.
*/
inv_error_t inv_get_temperature_raw(short *data)
{
    if (data == NULL)
        return INV_ERROR_INVALID_PARAMETER;

    if (!fifo_obj.data_config[CONFIG_TEMPERATURE]) {
        inv_error_t result;
        unsigned char regs[2];
        if ((fifo_obj.cache & FIFO_CACHE_TEMPERATURE) == 0) {
            if (FIFO_DEBUG)
                MPL_LOGI("Fetching the temperature from the registers\n");
            fifo_obj.cache |= FIFO_CACHE_TEMPERATURE;
            result = inv_serial_read(inv_get_serial_handle(),
                                inv_get_mpu_slave_addr(), MPUREG_TEMP_OUT_H, 2,
                                regs);
            if (result) {
                LOG_RESULT_LOCATION(result);
                return result;
            }
            fifo_obj.decoded[REF_RAW] = ((short)regs[0] << 8) | (regs[1]);
        }
    }
    *data = (short)fifo_obj.decoded[REF_RAW];
    return INV_SUCCESS;
}

/** 
 *  @brief      Returns 1-element vector of temperature. It is read from the hardware if it
 *              doesn't exist in the FIFO.
 *  @param[out] data    1-element vector of temperature
 *  @return     0 on success or an error code.
 */
inv_error_t inv_get_temperature(long *data)
{
    short tr;
    inv_error_t result;

    if (data == NULL)
        return INV_ERROR_INVALID_PARAMETER;
    result = inv_get_temperature_raw(&tr);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    data[0] = inv_decode_temperature(tr);
    return INV_SUCCESS;
}

/**
 *  @brief  Get the Decoded Accel Data.
 *  @param  data
 *              a buffer to store the quantized data.
 *  @return INV_SUCCESS if successful, a non-zero error code otherwise.
 */
inv_error_t inv_get_unquantized_accel(long *data)
{
    int ii, kk;
    if (data == NULL)
        return INV_ERROR_INVALID_PARAMETER;

    if (!fifo_obj.data_config[CONFIG_DMP_QUANT_ACCEL])
        return INV_ERROR_FEATURE_NOT_ENABLED;

    for (ii = 0; ii < INV_MAX_NUM_ACCEL_SAMPLES; ii++) {
        for (kk = 0; kk < ACCEL_NUM_AXES; kk++) {
            data[ii * ACCEL_NUM_AXES + kk] = fifo_obj.decoded_accel[ii][kk];
        }
    }

    return INV_SUCCESS;
}

/**
 *  @brief  Get the Quantized Accel data algorithm output from the FIFO.
 *  @param  data
 *              a buffer to store the quantized data.
 *  @return INV_SUCCESS if successful, a non-zero error code otherwise.
 */
inv_error_t inv_get_quantized_accel(long *data)
{
    int ii;
    if (data == NULL)
        return INV_ERROR_INVALID_PARAMETER;

    if (!fifo_obj.data_config[CONFIG_DMP_QUANT_ACCEL])
        return INV_ERROR_FEATURE_NOT_ENABLED;

    for (ii = 0; ii < INV_MAX_NUM_ACCEL_SAMPLES; ii++) {
        data[ii] = fifo_obj.decoded[REF_QUANT_ACCEL + ii];
    }

    return INV_SUCCESS;
}

/** This gets raw gyro data. The data is taken from the FIFO if it was put in the FIFO
*  and it is read from the registers if it was not put into the FIFO. The data is
*  cached till the next FIFO processing block time.
* @param[out] data Length 3, Gyro data
*/
inv_error_t inv_get_gyro_sensor(long *data)
{
    if (data == NULL)
        return INV_ERROR_INVALID_PARAMETER;
    if ((fifo_obj.data_config[CONFIG_RAW_DATA] & 7) != 7) {
        inv_error_t result;
        unsigned char regs[6];
        if ((fifo_obj.cache & FIFO_CACHE_GYRO) == 0) {
            fifo_obj.cache |= FIFO_CACHE_GYRO;
            result =
                inv_serial_read(inv_get_serial_handle(),
                                inv_get_mpu_slave_addr(), MPUREG_GYRO_XOUT_H, 6,
                                regs);
            if (result) {
                LOG_RESULT_LOCATION(result);
                return result;
            }
            fifo_obj.decoded[REF_RAW + 1] =
                (((long)regs[0]) << 24) | (((long)regs[1]) << 16);
            fifo_obj.decoded[REF_RAW + 2] =
                (((long)regs[2]) << 24) | (((long)regs[3]) << 16);
            fifo_obj.decoded[REF_RAW + 3] =
                (((long)regs[4]) << 24) | (((long)regs[5]) << 16);

            // Temperature starts at location 0, Gyro at location 1.
            fifo_obj.decoded[REF_RAW + 1] =
                inv_q30_mult(fifo_obj.decoded[REF_RAW + 1],
                             fifo_scale[REF_RAW + 1]);
            fifo_obj.decoded[REF_RAW + 2] =
                inv_q30_mult(fifo_obj.decoded[REF_RAW + 2],
                             fifo_scale[REF_RAW + 2]);
            fifo_obj.decoded[REF_RAW + 3] =
                inv_q30_mult(fifo_obj.decoded[REF_RAW + 3],
                             fifo_scale[REF_RAW + 3]);
        }
        data[0] = fifo_obj.decoded[REF_RAW + 1];
        data[1] = fifo_obj.decoded[REF_RAW + 2];
        data[2] = fifo_obj.decoded[REF_RAW + 3];
    } else {
        long data2[6];
        inv_get_gyro_and_accel_sensor(data2);
        data[0] = data2[0];
        data[1] = data2[1];
        data[2] = data2[2];
    }
    return INV_SUCCESS;
}

/** 
 *  @brief      Returns 6-element vector of gyro and accel data
 *  @param[out] data    6-element vector of gyro and accel data
 *  @return     0 on success or an error code.
 */
inv_error_t inv_get_gyro_and_accel_sensor(long *data)
{
    int ii;
    if (data == NULL)
        return INV_ERROR_INVALID_PARAMETER;

    if (!fifo_obj.data_config[CONFIG_RAW_DATA])
        return INV_ERROR_FEATURE_NOT_ENABLED;

    for (ii = 0; ii < (GYRO_NUM_AXES + ACCEL_NUM_AXES); ii++) {
        data[ii] = fifo_obj.decoded[REF_RAW + 1 + ii];
    }

    return INV_SUCCESS;
}

/** 
 *  @brief      Returns 3-element vector of external sensor
 *  @param[out] data    3-element vector of external sensor
 *  @return     0 on success or an error code.
 */
inv_error_t inv_get_external_sensor_data(long *data, int size)
{
#if defined CONFIG_MPU_SENSORS_MPU6050A2 || \
	defined CONFIG_MPU_SENSORS_MPU6050B1
    int ii;
    if (data == NULL)
        return INV_ERROR_INVALID_PARAMETER;

    if (!fifo_obj.data_config[CONFIG_RAW_EXTERNAL])
        return INV_ERROR_FEATURE_NOT_ENABLED;

    for (ii = 0; ii < size && ii < 6; ii++) {
        data[ii] = fifo_obj.decoded[REF_RAW_EXTERNAL + ii];
    }

    return INV_SUCCESS;
#else
    memset(data, 0, COMPASS_NUM_AXES * sizeof(long));
    return INV_ERROR_FEATURE_NOT_IMPLEMENTED;
#endif
}

/** 
 *  Sends accelerometer data to the FIFO.
 *
 *  @param[in] elements Which of the 3 elements to send. Use INV_ALL for 3 axis
 *            or INV_ELEMENT_1, INV_ELEMENT_2, INV_ELEMENT_3 or'd together
 *            for a subset.
 *
 * @param[in] accuracy Set to INV_32_BIT for 32-bit data, or INV_16_BIT for 16
 *            bit data. Set to zero to remove it from the FIFO.
 */
inv_error_t inv_send_accel(uint_fast16_t elements, uint_fast16_t accuracy)
{
    INVENSENSE_FUNC_START;
    unsigned char regs[4] = { DINAF8 + 1, DINA28, DINA30, DINA38 };
    inv_error_t result;
    int kk;

    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    result = inv_construct3_fifo(regs, elements, accuracy, REF_ACCEL,
                                 KEY_CFG_12, CONFIG_ACCEL);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    for (kk = 0; kk < ACCEL_NUM_AXES; kk++) {
        fifo_scale[REF_ACCEL + kk] = 2 * inv_obj.accel_sens;
    }

    return inv_set_footer();
}

/** 
 * Sends control data to the FIFO. Control data is a 4 length vector of 32-bits.
 *
 *  @param[in] elements Which of the 4 elements to send. Use INV_ALL for all
 *            or INV_ELEMENT_1, INV_ELEMENT_2, INV_ELEMENT_3, INV_ELEMENT_4 or'd
 *             together for a subset.
 *
 *  @param[in] accuracy Set to INV_32_BIT for 32-bit data, or INV_16_BIT for 16
 *             bit data. Set to zero to remove it from the FIFO.
 */
inv_error_t inv_send_cntrl_data(uint_fast16_t elements, uint_fast16_t accuracy)
{
    INVENSENSE_FUNC_START;
    int_fast8_t kk;
    inv_error_t result;
    unsigned char regs[5] = { DINAF8 + 1, DINA20, DINA28, DINA30, DINA38 };

    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    elements = inv_set_fifo_reference(elements, accuracy, REF_CONTROL, 4);
    accuracy = inv_set_fifo_accuracy(elements, accuracy, CONFIG_CONTROL_DATA);

    if (accuracy & INV_16_BIT) {
        regs[0] = DINAF8 + 2;
    }

    fifo_obj.data_config[CONFIG_CONTROL_DATA] = elements | accuracy;

    for (kk = 0; kk < 4; ++kk) {
        if ((elements & 1) == 0)
            regs[kk + 1] = DINAA0 + 3;
        elements >>= 1;
    }

    result = inv_set_mpu_memory(KEY_CFG_1, 5, regs);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    return inv_set_footer();
}

/** 
 * Adds a rolling counter to the fifo packet.  When used with the footer
 * the data comes out the first time:
 * 
 * @code
 * <data0><data1>...<dataN><PacketNum0><PacketNum1>
 * @endcode
 * for every other packet it is
 *
 * @code
 * <FifoFooter0><FifoFooter1><data0><data1>...<dataN><PacketNum0><PacketNum1>
 * @endcode
 *
 * This allows for scanning of the fifo for packets
 * 
 * @return INV_SUCCESS or error code
 */
inv_error_t inv_send_packet_number(uint_fast16_t accuracy)
{
    INVENSENSE_FUNC_START;
    inv_error_t result;
    unsigned char regs;
    uint_fast16_t elements;

    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    elements = inv_set_fifo_reference(1, accuracy, REF_DMP_PACKET, 1);
    if (elements & 1) {
        regs = DINA28;
        fifo_obj.data_config[CONFIG_DMP_PACKET_NUMBER] =
            INV_ELEMENT_1 | INV_16_BIT;
    } else {
        regs = DINAF8 + 3;
        fifo_obj.data_config[CONFIG_DMP_PACKET_NUMBER] = 0;
    }
    result = inv_set_mpu_memory(KEY_CFG_23, 1, &regs);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    return inv_set_footer();
}

/**
 *  @brief  Send the computed gravity vectors into the FIFO.
 *          The gravity vectors can be retrieved from the FIFO via 
 *          inv_get_gravity(), to have the gravitation vector expressed
 *          in coordinates relative to the body.
 *
 *  Gravity is a derived vector derived from the quaternion.
 *  @param  elements
 *              the gravitation vectors components bitmask.
 *              To send all compoents use INV_ALL.
 *  @param  accuracy
 *              The number of bits the gravitation vector is expressed 
 *              into.
 *              Set to INV_32_BIT for 32-bit data, or INV_16_BIT for 16
 *              bit data. 
 *              Set to zero to remove it from the FIFO.
 *
 *  @return INV_SUCCESS if successful, a non-zero error code otherwise.
 */
inv_error_t inv_send_gravity(uint_fast16_t elements, uint_fast16_t accuracy)
{
    INVENSENSE_FUNC_START;
    inv_error_t result;

    result = inv_send_quaternion(accuracy);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    return inv_set_footer();
}

/** Sends gyro data to the FIFO. Gyro data is a 3 length vector
 *  of 32-bits. Should be called once after inv_dmp_open() and before inv_dmp_start().
 *  @param[in] elements Which of the 3 elements to send. Use INV_ALL for all of them
 *            or INV_ELEMENT_1, INV_ELEMENT_2, INV_ELEMENT_3 or'd together
 *            for a subset.
 *  @param[in] accuracy Set to INV_32_BIT for 32-bit data, or INV_16_BIT for 16
 *             bit data. Set to zero to remove it from the FIFO.
 */
inv_error_t inv_send_gyro(uint_fast16_t elements, uint_fast16_t accuracy)
{
    INVENSENSE_FUNC_START;
    unsigned char regs[4] = { DINAF8 + 1, DINA20, DINA28, DINA30 };
    inv_error_t result;

    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (fifo_obj.gyro_source == INV_GYRO_FROM_QUATERNION) {
        regs[0] = DINA90 + 5;
        result = inv_set_mpu_memory(KEY_CFG_GYRO_SOURCE, 1, regs);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
        regs[0] = DINAF8 + 1;
        regs[1] = DINA20;
        regs[2] = DINA28;
        regs[3] = DINA30;
    } else {
        regs[0] = DINA90 + 10;
        result = inv_set_mpu_memory(KEY_CFG_GYRO_SOURCE, 1, regs);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
        regs[0] = DINAF8 + 1;
        regs[1] = DINA28;
        regs[2] = DINA30;
        regs[3] = DINA38;
    }
    result = inv_construct3_fifo(regs, elements, accuracy, REF_GYROS,
                                 KEY_CFG_9, CONFIG_GYROS);

    return inv_set_footer();
}

/** Sends linear accelerometer data to the FIFO. 
 *
 *  Linear accelerometer data is a 3 length vector of 32-bits. It is the 
 *  acceleration in the body frame with gravity removed.
 * 
 * 
 *  @param[in] elements Which of the 3 elements to send. Use INV_ALL for all of
 *            them or INV_ELEMENT_1, INV_ELEMENT_2, INV_ELEMENT_3 or'd together
 *            for a subset.
 *
 *  NOTE: Elements is ignored if the fifo rate is < INV_MAX_NUM_ACCEL_SAMPLES
 *  @param[in] accuracy Set to INV_32_BIT for 32-bit data, or INV_16_BIT for 16
 *             bit data. Set to zero to remove it from the FIFO.
 */
inv_error_t inv_send_linear_accel(uint_fast16_t elements,
                                  uint_fast16_t accuracy)
{
    INVENSENSE_FUNC_START;
    inv_error_t result;
    unsigned char state = inv_get_state();

    if (state < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    result = inv_send_gravity(elements, accuracy);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    result = inv_send_accel(elements, accuracy);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    return inv_set_footer();
}

/** Sends linear world accelerometer data to the FIFO. Linear world
 *  accelerometer data is a 3 length vector of 32-bits. It is the acceleration
 *  in the world frame with gravity removed. Should be called once after
 *  inv_dmp_open() and before inv_dmp_start().
 *
 *  @param[in] elements Which of the 3 elements to send. Use INV_ALL for all of 
 *             them or INV_ELEMENT_1, INV_ELEMENT_2, INV_ELEMENT_3 or'd together
 *             for a subset.
 *  @param[in] accuracy Set to INV_32_BIT for 32-bit data, or INV_16_BIT for 16
 *             bit data.
 */
inv_error_t inv_send_linear_accel_in_world(uint_fast16_t elements,
                                           uint_fast16_t accuracy)
{
    INVENSENSE_FUNC_START;
    inv_error_t result;

    result = inv_send_linear_accel(INV_ALL, accuracy);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    result = inv_send_quaternion(accuracy);

    return inv_set_footer();
}

/** Sends quaternion data to the FIFO. Quaternion data is a 4 length vector
 *   of 32-bits. Should be called once after inv_dmp_open() and before inv_dmp_start().
 * @param[in] accuracy Set to INV_32_BIT for 32-bit data, or INV_16_BIT for 16
 *            bit data.
 */
inv_error_t inv_send_quaternion(uint_fast16_t accuracy)
{
    INVENSENSE_FUNC_START;
    unsigned char regs[5] = { DINAF8 + 1, DINA20, DINA28,
        DINA30, DINA38
    };
    uint_fast16_t elements, kk;
    inv_error_t result;

    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    elements = inv_set_fifo_reference(0xf, accuracy, REF_QUATERNION, 4);
    accuracy = inv_set_fifo_accuracy(elements, accuracy, CONFIG_QUAT);

    if (accuracy & INV_16_BIT) {
        regs[0] = DINAF8 + 2;
    }

    fifo_obj.data_config[CONFIG_QUAT] = elements | accuracy;

    for (kk = 0; kk < 4; ++kk) {
        if ((elements & 1) == 0)
            regs[kk + 1] = DINAA0 + 3;
        elements >>= 1;
    }

    result = inv_set_mpu_memory(KEY_CFG_8, 5, regs);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    return inv_set_footer();
}

/** Sends raw data to the FIFO. 
 *  Should be called once after inv_dmp_open() and before inv_dmp_start().
 *  @param[in] elements Which of the 7 elements to send. Use INV_ALL for all of them
 *            or INV_ELEMENT_1, INV_ELEMENT_2, INV_ELEMENT_3 ... INV_ELEMENT_7 or'd together
 *            for a subset. The first element is temperature, the next 3 are gyro data,
 *            and the last 3 accel data.
 *  @param  accuracy
 *              The element's accuracy, can be INV_16_BIT, INV_32_BIT, or 0 to turn off.
 *  @return 0 if successful, a non-zero error code otherwise.
 */
inv_error_t inv_send_sensor_data(uint_fast16_t elements, uint_fast16_t accuracy)
{
    int result;
#if defined CONFIG_MPU_SENSORS_MPU6050A2 || \
	defined CONFIG_MPU_SENSORS_MPU6050B1
    unsigned char regs[7] = { DINAA0 + 3, DINAA0 + 3, DINAA0 + 3,
        DINAA0 + 3, DINAA0 + 3, DINAA0 + 3,
        DINAA0 + 3
    };

    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (accuracy)
        accuracy = INV_16_BIT;

    elements = inv_set_fifo_reference(elements, accuracy, REF_RAW, 7);

    if (elements & 1)
        fifo_obj.data_config[CONFIG_TEMPERATURE] = 1 | INV_16_BIT;
    else
        fifo_obj.data_config[CONFIG_TEMPERATURE] = 0;
    if (elements & 0x7e)
        fifo_obj.data_config[CONFIG_RAW_DATA] =
            (0x3f & (elements >> 1)) | INV_16_BIT;
    else
        fifo_obj.data_config[CONFIG_RAW_DATA] = 0;

    if (elements & INV_ELEMENT_1) {
        regs[0] = DINACA;
    }
    if (elements & INV_ELEMENT_2) {
        regs[1] = DINBC4;
    }
    if (elements & INV_ELEMENT_3) {
        regs[2] = DINACC;
    }
    if (elements & INV_ELEMENT_4) {
        regs[3] = DINBC6;
    }
    if (elements & INV_ELEMENT_5) {
        regs[4] = DINBC0;
    }
    if (elements & INV_ELEMENT_6) {
        regs[5] = DINAC8;
    }
    if (elements & INV_ELEMENT_7) {
        regs[6] = DINBC2;
    }
    result = inv_set_mpu_memory(KEY_CFG_15, 7, regs);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    return inv_set_footer();

#else
    INVENSENSE_FUNC_START;
    unsigned char regs[4] = { DINAA0 + 3,
        DINAA0 + 3,
        DINAA0 + 3,
        DINAA0 + 3
    };

    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (accuracy)
        accuracy = INV_16_BIT;

    elements = inv_set_fifo_reference(elements, accuracy, REF_RAW, 7);

    if (elements & 0x03) {
        elements |= 0x03;
        regs[0] = DINA20;
    }
    if (elements & 0x0C) {
        elements |= 0x0C;
        regs[1] = DINA28;
    }
    if (elements & 0x30) {
        elements |= 0x30;
        regs[2] = DINA30;
    }
    if (elements & 0x40) {
        elements |= 0xC0;
        regs[3] = DINA38;
    }

    result = inv_set_mpu_memory(KEY_CFG_15, 4, regs);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    if (elements & 0x01)
        fifo_obj.data_config[CONFIG_TEMPERATURE] = 1 | INV_16_BIT;
    else
        fifo_obj.data_config[CONFIG_TEMPERATURE] = 0;
    if (elements & 0xfe)
        fifo_obj.data_config[CONFIG_RAW_DATA] =
            (0x7f & (elements >> 1)) | INV_16_BIT;
    else
        fifo_obj.data_config[CONFIG_RAW_DATA] = 0;

    return inv_set_footer();
#endif
}

/** Sends raw external data to the FIFO. 
 *  Should be called once after inv_dmp_open() and before inv_dmp_start().
 *  @param[in] elements Which of the 3 elements to send. Use INV_ALL for all of them
 *            or INV_ELEMENT_1, INV_ELEMENT_2, INV_ELEMENT_3 or'd together
 *            for a subset. 
 *  @param[in] accuracy INV_16_BIT to send data, 0 to stop sending this data.
 *            Sending and Stop sending are reference counted, so data actually
 *            stops when the reference reaches zero.
 */
inv_error_t inv_send_external_sensor_data(uint_fast16_t elements,
                                          uint_fast16_t accuracy)
{
#if defined CONFIG_MPU_SENSORS_MPU6050A2 || \
	defined CONFIG_MPU_SENSORS_MPU6050B1
    int result;
    unsigned char regs[6] = { DINAA0 + 3, DINAA0 + 3,
                              DINAA0 + 3, DINAA0 + 3,
                              DINAA0 + 3, DINAA0 + 3 };

    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (accuracy)
        accuracy = INV_16_BIT;

    elements = inv_set_fifo_reference(elements, accuracy, REF_RAW_EXTERNAL, 6);

    if (elements)
        fifo_obj.data_config[CONFIG_RAW_EXTERNAL] = elements | INV_16_BIT;
    else
        fifo_obj.data_config[CONFIG_RAW_EXTERNAL] = 0;

    if (elements & INV_ELEMENT_1) {
        regs[0] = DINBC2;
    }
    if (elements & INV_ELEMENT_2) {
        regs[1] = DINACA;
    }
    if (elements & INV_ELEMENT_3) {
        regs[2] = DINBC4;
    }
    if (elements & INV_ELEMENT_4) {
        regs[3] = DINBC0;
    }
    if (elements & INV_ELEMENT_5) {
        regs[4] = DINAC8;
    }
    if (elements & INV_ELEMENT_6) {
        regs[5] = DINACC;
    }

    result = inv_set_mpu_memory(KEY_CFG_EXTERNAL, sizeof(regs), regs);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    return inv_set_footer();

#else
    return INV_ERROR_FEATURE_NOT_IMPLEMENTED;    // Feature not supported
#endif
}

/**
 *  @brief  Send the Quantized Acceleromter data into the FIFO.  The data can be
 *          retrieved using inv_get_quantized_accel() or inv_get_unquantized_accel().
 *
 *  To be useful this should be set to fifo_rate + 1 if less than
 *  INV_MAX_NUM_ACCEL_SAMPLES, otherwise it doesn't work.
 *
 *  @param  elements
 *              the components bitmask.
 *              To send all compoents use INV_ALL.
 *
 *  @param  accuracy
 *              Use INV_32_BIT for 32-bit data or INV_16_BIT for 
 *              16-bit data.
 *              Set to zero to remove it from the FIFO.
 *
 *  @return INV_SUCCESS if successful, a non-zero error code otherwise.
 */
inv_error_t inv_send_quantized_accel(uint_fast16_t elements,
                                     uint_fast16_t accuracy)
{
    INVENSENSE_FUNC_START;
    unsigned char regs[5] = { DINAF8 + 1, DINA20, DINA28,
        DINA30, DINA38
    };
    unsigned char regs2[4] = { DINA20, DINA28,
        DINA30, DINA38
    };
    inv_error_t result;
    int_fast8_t kk;
    int_fast8_t ii;

    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    elements = inv_set_fifo_reference(elements, accuracy, REF_QUANT_ACCEL, 8);

    if (elements) {
        fifo_obj.data_config[CONFIG_DMP_QUANT_ACCEL] = (elements) | INV_32_BIT;
    } else {
        fifo_obj.data_config[CONFIG_DMP_QUANT_ACCEL] = 0;
    }

    for (kk = 0; kk < INV_MAX_NUM_ACCEL_SAMPLES; ++kk) {
        fifo_obj.decoded[REF_QUANT_ACCEL + kk] = 0;
        for (ii = 0; ii < ACCEL_NUM_AXES; ii++) {
            fifo_obj.decoded_accel[kk][ii] = 0;
        }
    }

    for (kk = 0; kk < 4; ++kk) {
        if ((elements & 1) == 0)
            regs[kk + 1] = DINAA0 + 3;
        elements >>= 1;
    }

    result = inv_set_mpu_memory(KEY_CFG_TAP0, 5, regs);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    for (kk = 0; kk < 4; ++kk) {
        if ((elements & 1) == 0)
            regs2[kk] = DINAA0 + 3;
        elements >>= 1;
    }

    result = inv_set_mpu_memory(KEY_CFG_TAP4, 4, regs2);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    return inv_set_footer();
}

inv_error_t inv_send_eis(uint_fast16_t elements, uint_fast16_t accuracy)
{
    INVENSENSE_FUNC_START;
    int_fast8_t kk;
    unsigned char regs[3] = { DINA28, DINA30, DINA38 };
    inv_error_t result;

    if (inv_get_state() < INV_STATE_DMP_OPENED)
        return INV_ERROR_SM_IMPROPER_STATE;

    if (accuracy) {
        accuracy = INV_32_BIT;
    }

    elements = inv_set_fifo_reference(elements, accuracy, REF_EIS, 3);
    accuracy = inv_set_fifo_accuracy(elements, accuracy, CONFIG_EIS);

    fifo_obj.data_config[CONFIG_EIS] = elements | accuracy;

    for (kk = 0; kk < 3; ++kk) {
        if ((elements & 1) == 0)
            regs[kk] = DINAA0 + 7;
        elements >>= 1;
    }

    result = inv_set_mpu_memory(KEY_P_EIS_FIFO_XSHIFT, 3, regs);

    return inv_set_footer();
}

/** 
 * @brief       Returns 3-element vector of accelerometer data in body frame.
 *
 * @param[out]  data    3-element vector of accelerometer data in body frame.
 *                      One gee = 2^16.
 *  @return     0 on success or an error code.
 */
inv_error_t inv_get_accel(long *data)
{
    int kk;
    struct mldl_cfg *mldl_cfg = inv_get_dl_config();

    if (data == NULL)
        return INV_ERROR_INVALID_PARAMETER;

    if ((!fifo_obj.data_config[CONFIG_ACCEL] &&
         (mldl_cfg->requested_sensors & INV_DMP_PROCESSOR))
        ||
        (!(mldl_cfg->requested_sensors & INV_DMP_PROCESSOR) &&
         !inv_accel_present()))
        return INV_ERROR_FEATURE_NOT_ENABLED;

    for (kk = 0; kk < ACCEL_NUM_AXES; ++kk) {
        data[kk] = fifo_obj.decoded[REF_ACCEL + kk];
    }

    return INV_SUCCESS;
}

/** 
 *  @brief      Returns 4-element quaternion vector derived from 6-axis or 
 *  9-axis if 9-axis was implemented. 6-axis is gyros and accels. 9-axis is
 *  gyros, accel and compass.
 *
 *  @param[out] data    4-element quaternion vector. One is scaled to 2^30.
 *  @return     0 on success or an error code.
 */
inv_error_t inv_get_quaternion(long *data)
{
    int kk;

    if (data == NULL)
        return INV_ERROR_INVALID_PARAMETER;

    if (!fifo_obj.data_config[CONFIG_QUAT])
        return INV_ERROR_FEATURE_NOT_ENABLED;

    for (kk = 0; kk < 4; ++kk) {
        data[kk] = fifo_obj.decoded[REF_QUATERNION + kk];
    }

    return INV_SUCCESS;
}

/** 
 *  @brief      Returns 4-element quaternion vector derived from 6 
 *              axis sensors (gyros and accels).
 *  @param[out] data    
 *                  4-element quaternion vector. One is scaled to 2^30.
 *  @return     0 on success or an error code.
 */
inv_error_t inv_get_6axis_quaternion(long *data)
{
    int kk;
    if (data == NULL)
        return INV_ERROR_INVALID_PARAMETER;

    if (!fifo_obj.data_config[CONFIG_QUAT])
        return INV_ERROR_FEATURE_NOT_ENABLED;

    for (kk = 0; kk < 4; ++kk) {
        data[kk] = fifo_obj.decoded[REF_QUATERNION_6AXIS + kk];
    }

    return INV_SUCCESS;
}

inv_error_t inv_get_relative_quaternion(long *data)
{
    if (data == NULL)
        return INV_ERROR;
    data[0] = inv_obj.relative_quat[0];
    data[1] = inv_obj.relative_quat[1];
    data[2] = inv_obj.relative_quat[2];
    data[3] = inv_obj.relative_quat[3];
    return INV_SUCCESS;
}

/** 
 *  @brief  Returns 3-element vector of gyro data in body frame.
 *  @param[out] data    
 *              3-element vector of gyro data in body frame 
 *              with gravity removed. One degree per second = 2^16.
 *  @return 0 on success or an error code.
 */
inv_error_t inv_get_gyro(long *data)
{
    int kk;
    if (data == NULL)
        return INV_ERROR_INVALID_PARAMETER;

    if (fifo_obj.data_config[CONFIG_GYROS]) {
        for (kk = 0; kk < 3; ++kk) {
            data[kk] = fifo_obj.decoded[REF_GYROS + kk];
        }
        return INV_SUCCESS;
    } else {
        return INV_ERROR_FEATURE_NOT_ENABLED;
    }
}

/**
 *  @brief  Get the 3-element gravity vector from the FIFO expressed
 *          in coordinates relative to the body frame.
 *  @param  data    
 *              3-element vector of gravity in body frame.
 *  @return 0 on success or an error code.
 */
inv_error_t inv_get_gravity(long *data)
{
    long quat[4];
    int ii;
    inv_error_t result;

    if (data == NULL)
        return INV_ERROR_INVALID_PARAMETER;

    if (!fifo_obj.data_config[CONFIG_QUAT])
        return INV_ERROR_FEATURE_NOT_ENABLED;

    if ((fifo_obj.cache & FIFO_CACHE_GRAVITY_BODY) == 0) {
        fifo_obj.cache |= FIFO_CACHE_GRAVITY_BODY;

        // Compute it from Quaternion
        result = inv_get_quaternion(quat);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }

        data[0] =
            inv_q29_mult(quat[1], quat[3]) - inv_q29_mult(quat[2], quat[0]);
        data[1] =
            inv_q29_mult(quat[2], quat[3]) + inv_q29_mult(quat[1], quat[0]);
        data[2] =
            (inv_q29_mult(quat[3], quat[3]) + inv_q29_mult(quat[0], quat[0])) -
            1073741824L;

        for (ii = 0; ii < ACCEL_NUM_AXES; ii++) {
            data[ii] >>= 14;
            fifo_obj.gravity_cache[ii] = data[ii];
        }
    } else {
        data[0] = fifo_obj.gravity_cache[0];
        data[1] = fifo_obj.gravity_cache[1];
        data[2] = fifo_obj.gravity_cache[2];
    }

    return INV_SUCCESS;
}

/** 
* @brief        Sets the filter coefficent used for computing the acceleration
*               bias which is used to compute linear acceleration.
* @param[in] coef   Fitler coefficient. 0. means no filter, a small number means
*                   a small cutoff frequency with an increasing number meaning 
*                   an increasing cutoff frequency.
*/
inv_error_t inv_set_linear_accel_filter_coef(float coef)
{
    fifo_obj.acc_filter_coef = coef;
    return INV_SUCCESS;
}

/** 
 *  @brief      Returns 3-element vector of accelerometer data in body frame
 *              with gravity removed.
 *  @param[out] data    3-element vector of accelerometer data in body frame
 *                      with gravity removed. One g = 2^16.
 *  @return     0 on success or an error code. data unchanged on error.
 */
inv_error_t inv_get_linear_accel(long *data)
{
    int kk;
    long grav[3];
    long la[3];
    inv_error_t result;

    if (data == NULL)
        return INV_ERROR_INVALID_PARAMETER;

    result = inv_get_gravity(grav);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    result = inv_get_accel(la);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    if ((fifo_obj.cache & FIFO_CACHE_ACC_BIAS) == 0) {
        fifo_obj.cache |= FIFO_CACHE_ACC_BIAS;

        for (kk = 0; kk < ACCEL_NUM_AXES; ++kk) {
            long x;
            x = la[kk] - grav[kk];
            fifo_obj.acc_bias_filt[kk] = (long)(x * fifo_obj.acc_filter_coef +
                                                fifo_obj.acc_bias_filt[kk] *
                                                (1.f -
                                                 fifo_obj.acc_filter_coef));
            data[kk] = x - fifo_obj.acc_bias_filt[kk];
        }
    } else {
        for (kk = 0; kk < ACCEL_NUM_AXES; ++kk) {
            data[kk] = la[kk] - grav[kk] - fifo_obj.acc_bias_filt[kk];
        }
    }
    return INV_SUCCESS;
}

/** 
 *  @brief      Returns 3-element vector of accelerometer data in world frame
 *              with gravity removed.
 *  @param[out] data    3-element vector of accelerometer data in world frame
 *                      with gravity removed. One g = 2^16.
 *  @return     0 on success or an error code.
 */
inv_error_t inv_get_linear_accel_in_world(long *data)
{
    int kk;
    if (data == NULL)
        return INV_ERROR_INVALID_PARAMETER;
    if (fifo_obj.data_config[CONFIG_ACCEL] && fifo_obj.data_config[CONFIG_QUAT]) {
        long wtemp[4], qi[4], wtemp2[4];
        wtemp[0] = 0;
        inv_get_linear_accel(&wtemp[1]);
        inv_q_mult(&fifo_obj.decoded[REF_QUATERNION], wtemp, wtemp2);
        inv_q_invert(&fifo_obj.decoded[REF_QUATERNION], qi);
        inv_q_mult(wtemp2, qi, wtemp);
        for (kk = 0; kk < 3; ++kk) {
            data[kk] = wtemp[kk + 1];
        }
        return INV_SUCCESS;
    } else {
        return INV_ERROR_FEATURE_NOT_ENABLED;
    }
}

/**
 *  @brief      Returns 4-element vector of control data.
 *  @param[out] data    4-element vector of control data.
 *  @return     0 for succes or an error code.
 */
inv_error_t inv_get_cntrl_data(long *data)
{
    int kk;
    if (data == NULL)
        return INV_ERROR_INVALID_PARAMETER;

    if (!fifo_obj.data_config[CONFIG_CONTROL_DATA])
        return INV_ERROR_FEATURE_NOT_ENABLED;

    for (kk = 0; kk < 4; ++kk) {
        data[kk] = fifo_obj.decoded[REF_CONTROL + kk];
    }

    return INV_SUCCESS;

}

/**
 *  @brief      Returns 3-element vector of EIS shfit data
 *  @param[out] data    3-element vector of EIS shift data.
 *  @return     0 for succes or an error code.
 */
inv_error_t inv_get_eis(long *data)
{
    int kk;
    if (data == NULL)
        return INV_ERROR_INVALID_PARAMETER;

    if (!fifo_obj.data_config[CONFIG_EIS])
        return INV_ERROR_FEATURE_NOT_ENABLED;

    for (kk = 0; kk < 3; ++kk) {
        data[kk] = fifo_obj.decoded[REF_EIS + kk];
    }

    return INV_SUCCESS;

}

/** 
 *  @brief      Returns 3-element vector of accelerometer data in body frame.
 *  @param[out] data    3-element vector of accelerometer data in body frame in g's.
 *  @return     0 for success or an error code.
 */
inv_error_t inv_get_accel_float(float *data)
{
    long lData[3];
    int kk;
    int result;

    if (data == NULL)
        return INV_ERROR_INVALID_PARAMETER;

    result = inv_get_accel(lData);
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    for (kk = 0; kk < ACCEL_NUM_AXES; ++kk) {
        data[kk] = lData[kk] / 65536.0f;
    }

    return INV_SUCCESS;
}

/** 
 *  @brief      Returns 4-element quaternion vector.
 *  @param[out] data    4-element quaternion vector.
 *  @return     0 on success, an error code otherwise.
 */
inv_error_t inv_get_quaternion_float(float *data)
{
    int kk;

    if (data == NULL)
        return INV_ERROR_INVALID_PARAMETER;

    if (!fifo_obj.data_config[CONFIG_QUAT])
        return INV_ERROR_FEATURE_NOT_ENABLED;

    for (kk = 0; kk < 4; ++kk) {
        data[kk] = fifo_obj.decoded[REF_QUATERNION + kk] / 1073741824.0f;
    }

    return INV_SUCCESS;
}

/**
 * @brief   Command the MPU to put data in the FIFO at a particular rate.
 *
 *          The DMP will add fifo entries every fifoRate + 1 MPU cycles.  For
 *          example if the MPU is running at 200Hz the following values apply:
 *
 *          <TABLE>
 *          <TR><TD>fifoRate</TD><TD>DMP Sample Rate</TD><TD>FIFO update frequency</TD></TR>
 *          <TR><TD>0</TD><TD>200Hz</TD><TD>200Hz</TD></TR>
 *          <TR><TD>1</TD><TD>200Hz</TD><TD>100Hz</TD></TR>
 *          <TR><TD>2</TD><TD>200Hz</TD><TD>50Hz</TD></TR>
 *          <TR><TD>4</TD><TD>200Hz</TD><TD>40Hz</TD></TR>
 *          <TR><TD>9</TD><TD>200Hz</TD><TD>20Hz</TD></TR>
 *          <TR><TD>19</TD><TD>200Hz</TD><TD>10Hz</TD></TR>
 *          </TABLE>
 *
 *          Note: if the DMP is running, (state == INV_STATE_DMP_STARTED)
 *          then inv_run_state_callbacks() will be called to allow features
 *          that depend upon fundamental constants to be updated.
 *
 *  @pre    inv_dmp_open() 
 *          @ifnot MPL_MF 
 *              or inv_open_low_power_pedometer() 
 *              or inv_eis_open_dmp()
 *          @endif
 *          and inv_dmp_start() 
 *          must <b>NOT</b> have been called.
 *
 * @param   fifoRate    Divider value - 1.  Output rate is 
 *          (DMP Sample Rate) / (fifoRate + 1).
 *
 * @return  INV_SUCCESS if successful, ML error code on any failure.
 */
inv_error_t inv_set_fifo_rate(unsigned short fifoRate)
{
    INVENSENSE_FUNC_START;
    unsigned char regs[2];
    unsigned char state;
    inv_error_t result = INV_SUCCESS;
    struct mldl_cfg *mldl_cfg = inv_get_dl_config();

    state = inv_get_state();
    if (state != INV_STATE_DMP_OPENED && state != INV_STATE_DMP_STARTED)
        return INV_ERROR_SM_IMPROPER_STATE;

    fifo_obj.fifo_rate = fifoRate;

    if (mldl_cfg->requested_sensors & INV_DMP_PROCESSOR) {

        regs[0] = (unsigned char)((fifoRate >> 8) & 0xff);
        regs[1] = (unsigned char)(fifoRate & 0xff);

        result = inv_set_mpu_memory(KEY_D_0_22, 2, regs);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
        fifo_obj.sample_step_size_ms = 
            (unsigned short)(((long)fifoRate + 1) *
                             (inv_mpu_get_sampling_period_us
                              (mldl_cfg)) / 1000L);

        if (state == INV_STATE_DMP_STARTED)
            result = inv_run_state_callbacks(state);
    } else if (mldl_cfg->requested_sensors & INV_THREE_AXIS_ACCEL) {
        struct ext_slave_config config;
        long data;
        config.key = MPU_SLAVE_CONFIG_ODR_RESUME;
        config.len = sizeof(long);
        config.apply = (state == INV_STATE_DMP_STARTED);
        config.data = &data;
        data = (1000 * inv_mpu_get_sampling_rate_hz(mldl_cfg)) / (fifoRate + 1);

        /* Ask for the same frequency */
        result = inv_mpu_config_accel(mldl_cfg,
                                      inv_get_serial_handle(),
                                      inv_get_serial_handle(), &config);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
        result = inv_mpu_get_accel_config(mldl_cfg,
                                          inv_get_serial_handle(),
                                          inv_get_serial_handle(), &config);
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
        if(FIFO_DEBUG)
            MPL_LOGI("Actual ODR: %ld Hz\n", data / 1000);
        /* Record the actual frequency granted odr is in mHz */
        fifo_obj.sample_step_size_ms = (unsigned short)((1000L * 1000L) / data);
    }
    return result;
}

/**
 * @brief   Retrieve the current FIFO update divider - 1.
 *          See inv_set_fifo_rate() for how this value is used.
 *
 *          The fifo rate when there is no fifo is the equivilent divider when
 *          derived from the value set by SetSampleSteSizeMs()
 *          
 * @return  The value of the fifo rate divider or INV_INVALID_FIFO_RATE on error.
 */
unsigned short inv_get_fifo_rate(void)
{
    return fifo_obj.fifo_rate;
}

/**
 * @brief   Returns the step size for quaternion type data.
 *
 * Typically the data rate for each FIFO packet. When the gryos are sleeping
 * this value will return the last value set by SetSampleStepSizeMs()
 *
 * @return  step size for quaternion type data
 */
int_fast16_t inv_get_sample_step_size_ms(void)
{
    struct mldl_cfg *mldl_cfg = inv_get_dl_config();

    if (mldl_cfg->requested_sensors & INV_DMP_PROCESSOR)
        return (fifo_obj.fifo_rate + 1) *
            (inv_mpu_get_sampling_period_us(mldl_cfg) / 1000);
    else
        return fifo_obj.sample_step_size_ms;
}

/**
 * @brief   Returns the step size for quaternion type data.
 *
 * Typically the data rate for each FIFO packet. When the gryos are sleeping
 * this value will return the last value set by SetSampleStepSizeMs()
 *
 * @return  step size for quaternion type data
 */
int_fast16_t inv_get_sample_frequency(void)
{
    struct mldl_cfg *mldl_cfg = inv_get_dl_config();

    if (mldl_cfg->requested_sensors & INV_DMP_PROCESSOR)
        return (inv_mpu_get_sampling_rate_hz(mldl_cfg) /
                (fifo_obj.fifo_rate + 1));
    else
        return (1000 / fifo_obj.sample_step_size_ms);
}

/** 
 *  @brief  The gyro data magnitude squared : 
 *          (1 degree per second)^2 = 2^6 = 2^GYRO_MAG_SQR_SHIFT.
 *  @return the computed magnitude squared output of the gyroscope.
 */
unsigned long inv_get_gyro_sum_of_sqr(void)
{
    unsigned long gmag = 0;
    long temp;
    int kk;

    for (kk = 0; kk < 3; ++kk) {
        temp = fifo_obj.decoded[REF_GYROS + kk] >>
            (16 - (GYRO_MAG_SQR_SHIFT / 2));
        gmag += temp * temp;
    }

    return gmag;
}

/** 
 *  @brief  The gyro data magnitude squared:
 *          (1 g)^2 = 2^16 = 2^ACC_MAG_SQR_SHIFT.
 *  @return the computed magnitude squared output of the accelerometer.
 */
unsigned long inv_accel_sum_of_sqr(void)
{
    unsigned long amag = 0;
    long temp;
    int kk;
    long accel[3];
    inv_error_t result;

    result = inv_get_accel(accel);
    if (INV_SUCCESS != result) {
        return 0;
    }

    for (kk = 0; kk < 3; ++kk) {
        temp = accel[kk] >> (16 - (ACC_MAG_SQR_SHIFT / 2));
        amag += temp * temp;
    }
    return amag;
}

/**
 *  @internal
 */
void inv_override_quaternion(float *q)
{
    int kk;
    for (kk = 0; kk < 4; ++kk) {
        fifo_obj.decoded[REF_QUATERNION + kk] = (long)(q[kk] * (1L << 30));
    }
}

/**
 * @internal
 * @brief   This registers a function to be called for each set of 
 *          gyro/quaternion/rotation matrix/etc output.
 * @param[in] func The callback function to register
 * @param[in] priority The unique priority number of the callback. Lower numbers
 *            are called first.
 * @return  error code.
 */
inv_error_t inv_register_fifo_rate_process(inv_obj_func func, int priority)
{
    INVENSENSE_FUNC_START;
    inv_error_t result;
    int kk, nn;

    result = inv_lock_mutex(fifo_rate_obj.mutex);
    if (INV_SUCCESS != result) {
        return result;
    }
    // Make sure we haven't registered this function already
    // Or used the same priority
    for (kk = 0; kk < fifo_rate_obj.num_cb; ++kk) {
        if ((fifo_rate_obj.fifo_process_cb[kk] == func) ||
            (fifo_rate_obj.priority[kk] == priority)) {
            inv_unlock_mutex(fifo_rate_obj.mutex);
            return INV_ERROR_INVALID_PARAMETER;
        }
    }

    // Make sure we have not filled up our number of allowable callbacks
    if (fifo_rate_obj.num_cb <= MAX_HIGH_RATE_PROCESSES - 1) {
        kk = 0;
        if (fifo_rate_obj.num_cb != 0) {
            // set kk to be where this new callback goes in the array
            while ((kk < fifo_rate_obj.num_cb) &&
                   (fifo_rate_obj.priority[kk] < priority)) {
                kk++;
            }
            if (kk != fifo_rate_obj.num_cb) {
                // We need to move the others
                for (nn = fifo_rate_obj.num_cb; nn > kk; --nn) {
                    fifo_rate_obj.fifo_process_cb[nn] =
                        fifo_rate_obj.fifo_process_cb[nn - 1];
                    fifo_rate_obj.priority[nn] = fifo_rate_obj.priority[nn - 1];
                }
            }
        }
        // Add new callback
        fifo_rate_obj.fifo_process_cb[kk] = func;
        fifo_rate_obj.priority[kk] = priority;
        fifo_rate_obj.num_cb++;
    } else {
        result = INV_ERROR_MEMORY_EXAUSTED;
    }

    inv_unlock_mutex(fifo_rate_obj.mutex);
    return result;
}

/**
 * @internal
 * @brief   This unregisters a function to be called for each set of 
 *          gyro/quaternion/rotation matrix/etc output.
 * @return  error code.
 */
inv_error_t inv_unregister_fifo_rate_process(inv_obj_func func)
{
    INVENSENSE_FUNC_START;
    int kk, jj;
    inv_error_t result;

    result = inv_lock_mutex(fifo_rate_obj.mutex);
    if (INV_SUCCESS != result) {
        return result;
    }
    // Make sure we haven't registered this function already
    result = INV_ERROR_INVALID_PARAMETER;
    for (kk = 0; kk < fifo_rate_obj.num_cb; ++kk) {
        if (fifo_rate_obj.fifo_process_cb[kk] == func) {
            for (jj = kk + 1; jj < fifo_rate_obj.num_cb; ++jj) {
                fifo_rate_obj.fifo_process_cb[jj - 1] =
                    fifo_rate_obj.fifo_process_cb[jj];
                fifo_rate_obj.priority[jj - 1] =
                    fifo_rate_obj.priority[jj];
            }
            fifo_rate_obj.fifo_process_cb[fifo_rate_obj.num_cb - 1] = NULL;
            fifo_rate_obj.priority[fifo_rate_obj.num_cb - 1] = 0;
            fifo_rate_obj.num_cb--;
            result = INV_SUCCESS;
            break;
        }
    }

    inv_unlock_mutex(fifo_rate_obj.mutex);
    return result;

}
#ifdef UMPL
bool bFIFIDataAvailable = FALSE;
bool isUmplDataInFIFO(void)
{
    return bFIFIDataAvailable;
}
void setUmplDataInFIFOFlag(bool flag)
{
    bFIFIDataAvailable = flag;
}
#endif
inv_error_t inv_run_fifo_rate_processes(void)
{
    int kk;
    inv_error_t result, result2;

    result = inv_lock_mutex(fifo_rate_obj.mutex);
    if (INV_SUCCESS != result) {
        MPL_LOGE("MLOsLockMutex returned %d\n", result);
        return result;
    }
    // User callbacks take priority over the fifo_process_cb callback
    if (fifo_obj.fifo_process_cb)
        fifo_obj.fifo_process_cb();

    for (kk = 0; kk < fifo_rate_obj.num_cb; ++kk) {
        if (fifo_rate_obj.fifo_process_cb[kk]) {
            result2 = fifo_rate_obj.fifo_process_cb[kk] (&inv_obj);
            if (result == INV_SUCCESS)
#ifdef UMPL		 
	 setUmplDataInFIFOFlag(TRUE);
#endif
                result = result2;
        }
    }

    inv_unlock_mutex(fifo_rate_obj.mutex);
    return result;
}

/*********************/
         /** \}*//* defgroup */
/*********************/
