/*
 $License:
    Copyright (C) 2014 InvenSense Corporation, All Rights Reserved.
 $
 */

/**
 *  @brief    Provides helpful file IO wrappers for use with sysfs.
 *  @details  Based on Jonathan Cameron's @e iio_utils.h.
 */

#ifndef _INV_SYSFS_UTILS_H_
#define _INV_SYSFS_UTILS_H_

/**
 *  struct inv_sysfs_names_s - Files needed by user applications.
 *  @buffer:		Ring buffer attached to FIFO.
 *  @enable:		Turns on HW-to-ring buffer flow.
 *  @raw_data:		Raw data from registers.
 *  @temperature:	Temperature data from register.
 *  @fifo_rate:		FIFO rate/ODR.
 *  @power_state:	Power state (this is a five-star comment).
 *  @fsr:		Full-scale range.
 *  @lpf:		Digital low pass filter.
 *  @scale:		LSBs / dps (or LSBs / Gs).
 *  @temp_scale:	LSBs / degrees C.
 *  @temp_offset:	Offset in LSBs.
 */
struct inv_sysfs_names_s {

	//Sysfs for ITG3500 & MPU6050
	const char *buffer;
	const char *enable;
	const char *raw_data;		//Raw Gyro data
	const char *temperature;
	const char *fifo_rate;
	const char *power_state;
	const char *fsr;
	const char *lpf;
	const char *scale;			//Gyro scale
	const char *temp_scale;
	const char *temp_offset;
	const char *self_test;
	//Starting Sysfs available for MPU6050 only
	const char *accel_en;
	const char *accel_fifo_en;
	const char *accel_fs;
	const char *clock_source;
	const char *early_suspend_en;
	const char *firmware_loaded;
	const char *gyro_en;
	const char *gyro_fifo_en;
	const char *key;
	const char *raw_accel;
	const char *reg_dump;
	const char *tap_on;
	const char *dmp_firmware;
};

/* File IO. Typically won't be called directly by user application, but they'll
 * be here for your enjoyment.
 */
int inv_sysfs_write(char *filename, long data);
int inv_sysfs_read(char *filename, long num_bytes, char *data);

/* Helper APIs to extract specific data. */
int inv_read_buffer(int fd, long *data, long long *timestamp);
int inv_read_raw(const struct inv_sysfs_names_s *names, long *data, 
		 long long *timestamp);
int inv_read_temperature_raw(const struct inv_sysfs_names_s *names, short *data,
			     long long *timestamp);
int inv_read_fifo_rate(const struct inv_sysfs_names_s *names, short *data);
int inv_read_power_state(const struct inv_sysfs_names_s *names, char *data);
int inv_read_scale(const struct inv_sysfs_names_s *names, float *data);
int inv_read_temp_scale(const struct inv_sysfs_names_s *names, short *data);
int inv_read_temp_offset(const struct inv_sysfs_names_s *names, short *data);
int inv_write_fifo_rate(const struct inv_sysfs_names_s *names, short data);
int inv_write_buffer_enable(const struct inv_sysfs_names_s *names, char data);
int inv_write_power_state(const struct inv_sysfs_names_s *names, char data);

/* Scaled data. */
int inv_read_q16(const struct inv_sysfs_names_s *names, long *data,
                 long long *timestamp);
int inv_read_temp_q16(const struct inv_sysfs_names_s *names, long *data,
                      long long *timestamp);


#endif  /* #ifndef _INV_SYSFS_UTILS_H_ */


