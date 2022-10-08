/*
 $License:
    Copyright (C) 2012 InvenSense Corporation, All Rights Reserved.
 $
 */

/*******************************************************************************
 *
 * $Id:$
 *
 ******************************************************************************/

#ifndef INV_CONSTRUCTOR_H__
#define INV_CONSTRUCTOR_H__

#include "mltypes.h"
#include "data_builder.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PRECISION 10000.f
#define RANGE_FLOAT_TO_FIXEDPOINT(range, x) {               \
    range.mantissa = (long)x;                               \
    range.fraction = (long)((float)(x-(long)x)*PRECISION);  \
}
#define RANGE_FIXEDPOINT_TO_FLOAT(range, x) {   \
    x = (float)(range.mantissa);                \
    x += ((float)range.fraction/PRECISION);     \
}

struct fifo_dmp_config {
    unsigned char sample_divider;
    unsigned char fifo_divider;
    unsigned char mpl_divider;
};

inv_error_t inv_construct_and_push_data();
inv_error_t inv_set_fifo_processed_callback(void (*func_cb)(void));
inv_error_t inv_constructor_setup();
inv_error_t inv_constructor_start();
inv_error_t inv_constructor_init();
inv_error_t inv_constructor_default_enable();
void inv_set_debug_mode(rd_dbg_mode mode);
inv_error_t inv_playback();
void inv_set_playback_filename(char *filename, int length);
inv_error_t wait_for_and_process_interrupt();

inv_error_t inv_set_interrupt_word(unsigned long word);
inv_error_t inv_get_interrupt_word(unsigned long *data);
inv_error_t inv_set_gesture_enable(int word);
int inv_get_gesture_enable(void);
inv_error_t inv_set_fifo_rate(unsigned long fifo_rate);
inv_error_t inv_get_dmp_sample_divider(unsigned char *data);

#ifdef __cplusplus
}
#endif

#endif // INVENSENSE_INV_CONSTRUCTOR_H__

