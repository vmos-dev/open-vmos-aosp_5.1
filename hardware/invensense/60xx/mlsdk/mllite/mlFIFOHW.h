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
#ifndef INVENSENSE_INV_FIFO_HW_H__
#define INVENSENSE_INV_FIFO_HW_H__

#include "mpu.h"
#include "mltypes.h"
#include "mlinclude.h"
#ifdef INV_INCLUDE_LEGACY_HEADERS
#include "mlFIFOHW_legacy.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

    // This is the maximum amount of FIFO data we would read in one packet
#define MAX_FIFO_LENGTH             (256)
    // This is the hardware size of the FIFO
#define FIFO_FOOTER_SIZE            (2)

    uint_fast16_t inv_get_fifo(uint_fast16_t length, unsigned char *buffer);
    inv_error_t inv_get_fifo_status(void);
    inv_error_t inv_get_fifo_length(uint_fast16_t * len);
    short inv_get_fifo_count(void);
    inv_error_t inv_reset_fifo(void);
    void inv_init_fifo_hardare();
    inv_error_t inv_read_fifo(unsigned char *data, uint_fast16_t len);

#ifdef __cplusplus
}
#endif
#endif                          // INVENSENSE_INV_FIFO_HW_H__
