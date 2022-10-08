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

#ifndef INVENSENSE_INV_FIFO_H__
#define INVENSENSE_INV_FIFO_H__

#include "mltypes.h"
#include "mlinclude.h"
#include "ml.h"
#ifdef INV_INCLUDE_LEGACY_HEADERS
#include "mlFIFO_legacy.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

    /**************************************************************************/
    /*  Elements                                                              */
    /**************************************************************************/

#define INV_ELEMENT_1                    (0x0001)
#define INV_ELEMENT_2                    (0x0002)
#define INV_ELEMENT_3                    (0x0004)
#define INV_ELEMENT_4                    (0x0008)
#define INV_ELEMENT_5                    (0x0010)
#define INV_ELEMENT_6                    (0x0020)
#define INV_ELEMENT_7                    (0x0040)
#define INV_ELEMENT_8                    (0x0080)

#define INV_ALL                          (0xFFFF)
#define INV_ELEMENT_MASK                 (0x00FF)

    /**************************************************************************/
    /*  Accuracy                                                              */
    /**************************************************************************/

#define INV_16_BIT                       (0x0100)
#define INV_32_BIT                       (0x0200)
#define INV_ACCURACY_MASK                (0x0300)

    /**************************************************************************/
    /*  Accuracy                                                              */
    /**************************************************************************/

#define INV_GYRO_FROM_RAW                (0x00)
#define INV_GYRO_FROM_QUATERNION         (0x01)

    /**************************************************************************/
    /*  High Rate Proceses                                                    */
    /**************************************************************************/

#define MAX_HIGH_RATE_PROCESSES 16

    /**************************************************************************/
    /*  Prototypes                                                            */
    /**************************************************************************/

    inv_error_t inv_set_fifo_rate(unsigned short fifoRate);
    unsigned short inv_get_fifo_rate(void);
    int_fast16_t inv_get_sample_step_size_ms(void);
    int_fast16_t inv_get_sample_frequency(void);
    long inv_decode_temperature(short tempReg);

    // Register callbacks after a packet of FIFO data is processed
    inv_error_t inv_register_fifo_rate_process(inv_obj_func func, int priority);
    inv_error_t inv_unregister_fifo_rate_process(inv_obj_func func);
    inv_error_t inv_run_fifo_rate_processes(void);

    // Setup FIFO for various output
    inv_error_t inv_send_quaternion(uint_fast16_t accuracy);
    inv_error_t inv_send_gyro(uint_fast16_t elements, uint_fast16_t accuracy);
    inv_error_t inv_send_accel(uint_fast16_t elements, uint_fast16_t accuracy);
    inv_error_t inv_send_linear_accel(uint_fast16_t elements,
                                      uint_fast16_t accuracy);
    inv_error_t inv_send_linear_accel_in_world(uint_fast16_t elements,
                                               uint_fast16_t accuracy);
    inv_error_t inv_send_cntrl_data(uint_fast16_t elements,
                                    uint_fast16_t accuracy);
    inv_error_t inv_send_sensor_data(uint_fast16_t elements,
                                     uint_fast16_t accuracy);
    inv_error_t inv_send_external_sensor_data(uint_fast16_t elements,
                                              uint_fast16_t accuracy);
    inv_error_t inv_send_gravity(uint_fast16_t elements,
                                 uint_fast16_t accuracy);
    inv_error_t inv_send_packet_number(uint_fast16_t accuracy);
    inv_error_t inv_send_quantized_accel(uint_fast16_t elements,
                                         uint_fast16_t accuracy);
    inv_error_t inv_send_eis(uint_fast16_t elements, uint_fast16_t accuracy);

    // Get Fixed Point data from FIFO
    inv_error_t inv_get_accel(long *data);
    inv_error_t inv_get_quaternion(long *data);
    inv_error_t inv_get_6axis_quaternion(long *data);
    inv_error_t inv_get_relative_quaternion(long *data);
    inv_error_t inv_get_gyro(long *data);
    inv_error_t inv_set_linear_accel_filter_coef(float coef);
    inv_error_t inv_get_linear_accel(long *data);
    inv_error_t inv_get_linear_accel_in_world(long *data);
    inv_error_t inv_get_gyro_and_accel_sensor(long *data);
    inv_error_t inv_get_gyro_sensor(long *data);
    inv_error_t inv_get_cntrl_data(long *data);
    inv_error_t inv_get_temperature(long *data);
    inv_error_t inv_get_gravity(long *data);
    inv_error_t inv_get_unquantized_accel(long *data);
    inv_error_t inv_get_quantized_accel(long *data);
    inv_error_t inv_get_external_sensor_data(long *data, int size);
    inv_error_t inv_get_eis(long *data);

    // Get Floating Point data from FIFO
    inv_error_t inv_get_accel_float(float *data);
    inv_error_t inv_get_quaternion_float(float *data);

    inv_error_t inv_process_fifo_packet(const unsigned char *dmpData);
    inv_error_t inv_read_and_process_fifo(int_fast8_t numPackets,
                                          int_fast8_t * processed);

    inv_error_t inv_set_fifo_processed_callback(void (*func) (void));

    inv_error_t inv_init_fifo_param(void);
    inv_error_t inv_close_fifo(void);
    inv_error_t inv_set_gyro_data_source(uint_fast8_t source);
    inv_error_t inv_decode_quantized_accel(void);
    unsigned long inv_get_gyro_sum_of_sqr(void);
    unsigned long inv_accel_sum_of_sqr(void);
    void inv_override_quaternion(float *q);
#ifdef UMPL
    bool isUmplDataInFIFO(void);
    void setUmplDataInFIFOFlag(bool flag);
#endif
    uint_fast16_t inv_get_fifo_packet_size(void);
#ifdef __cplusplus
}
#endif
#endif                          // INVENSENSE_INV_FIFO_H__
