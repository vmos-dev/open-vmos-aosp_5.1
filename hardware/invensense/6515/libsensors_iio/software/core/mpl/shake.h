/*
 $License:
    Copyright (C) 2011-2012 InvenSense Corporation, All Rights Reserved.
    See included License.txt for License information.
 $
 */
#ifndef INV_SHAKE_H__
#define INV_SHAKE_H__

#include "mltypes.h"


#ifdef __cplusplus
extern "C" {
#endif

	/* ------------ */
    /* - Defines. - */
    /* ------------ */

    #define STATE_ZERO                             0
    #define STATE_INIT_1                           1
    #define STATE_INIT_2                           2
    #define STATE_DETECT                           3

	struct t_shake_config_params {
		long shake_time_min_ms;
		long shake_time_max_ms;
		long shake_time_min;
        long shake_time_max;
		unsigned char shake_time_set;
		long shake_time_saved;
        float shake_deriv_thr;
        int zero_cross_thr;
        float accel_delta_min;
        float accel_delta_max;
		unsigned char interp_enable;
	};

	struct t_shake_state_params {
		unsigned char state;
        float accel_peak_high;
        float accel_peak_low;
        float accel_range;
        int num_zero_cross;
        short curr_shake_time;
		int deriv_major_change;
		int deriv_major_sign;
        float accel_buffer[200];
        float delta_buffer[200];
	};

	struct t_shake_data_params {
		float accel_prev;
		float accel_curr;
		float delta_prev;
		float delta_curr;
		float delta_prev_buffer;
	};

	struct t_shake_results {
		//unsigned char shake_int;
		int shake_number;
	};

	struct t_shake_cb {
       void (*shake_callback)(struct t_shake_results *shake_results);            
    };


    /* --------------------- */
    /* - Function p-types. - */
    /* --------------------- */
	inv_error_t inv_enable_shake(void);
    inv_error_t inv_disable_shake(void);
    inv_error_t inv_init_shake(void);
    inv_error_t inv_start_shake(void);
	int inv_set_shake_cb(void (*callback)(struct t_shake_results *shake_results));
	void inv_config_shake_time_params(long sample_time_ms);
	void inv_set_shake_accel_delta_min(float accel_g);
	void inv_set_shake_accel_delta_max(float accel_g);
	void inv_set_shake_zero_cross_thresh(int num_zero_cross);
	void inv_set_shake_deriv_thresh(float shake_deriv_thresh);
	void inv_set_shake_time_min_ms(long time_ms);
	void inv_set_shake_time_max_ms(long time_ms);
	void inv_enable_shake_data_interpolation(unsigned char en);



#ifdef __cplusplus
}
#endif

#endif // INV_SHAKE__