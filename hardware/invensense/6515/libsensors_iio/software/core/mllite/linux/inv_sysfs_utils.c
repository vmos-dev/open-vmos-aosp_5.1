/*
 $License:
    Copyright (C) 2014 InvenSense Corporation, All Rights Reserved.
 $
 */

/**
 *  @brief    Provides helpful file IO wrappers for use with sysfs.
 *  @details  Based on Jonathan Cameron's @e iio_utils.h.
 */

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <stdint.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#include "inv_sysfs_utils.h"

/* General TODO list:
 * Select more reasonable string lengths or use fseek and malloc.
 */

/**
 *  inv_sysfs_write() - Write an integer to a file.
 *  @filename:	Path to file.
 *  @data:	Value to write to file.
 *  Returns number of bytes written or a negative error code.
 */
int inv_sysfs_write(char *filename, long data)
{
	FILE *fp;
	int count;

	if (!filename)
		return -1;
	fp = fopen(filename, "w");
	if (!fp)
		return -errno;
	count = fprintf(fp, "%ld", data);
	fclose(fp);
	return count;
}

/**
 *  inv_sysfs_read() - Read a string from a file.
 *  @filename:	Path to file.
 *  @num_bytes:	Number of bytes to read.
 *  @data:	Data from file.
 *  Returns number of bytes written or a negative error code.
 */
int inv_sysfs_read(char *filename, long num_bytes, char *data)
{
	FILE *fp;
	int count;

	if (!filename)
		return -1;
	fp = fopen(filename, "r");
	if (!fp)
		return -errno;
	count = fread(data, 1, num_bytes, fp);
	fclose(fp);
	return count;
}

/**
 *  inv_read_buffer() - Read data from ring buffer.
 *  @fd:	File descriptor for buffer file.
 *  @data:	Data in hardware units.
 *  @timestamp:	Time when data was read from device. Use NULL if unsupported.
 *  Returns number of bytes written or a negative error code.
 */
int inv_read_buffer(int fd, long *data, long long *timestamp)
{
	char str[35];
	int count;

	count = read(fd, str, sizeof(str));
	if (!count)
		return count;
	if (!timestamp)
		count = sscanf(str, "%ld%ld%ld", &data[0], &data[1], &data[2]);
	else
		count = sscanf(str, "%ld%ld%ld%lld", &data[0], &data[1],
			&data[2], timestamp);

	if (count < (timestamp?4:3))
		return -EAGAIN;
	return count;
}

/**
 *  inv_read_raw() - Read raw data.
 *  @names:	Names of sysfs files.
 *  @data:	Data in hardware units.
 *  @timestamp:	Time when data was read from device. Use NULL if unsupported.
 *  Returns number of bytes written or a negative error code.
 */
int inv_read_raw(const struct inv_sysfs_names_s *names, long *data, 
	long long *timestamp)
{
	char str[40];
	int count;

	count = inv_sysfs_read((char*)names->raw_data, sizeof(str), str);
	if (count < 0)
		return count;
	if (!timestamp)
		count = sscanf(str, "%ld%ld%ld", &data[0], &data[1], &data[2]);
	else
		count = sscanf(str, "%ld%ld%ld%lld", &data[0], &data[1],
			&data[2], timestamp);
	if (count < (timestamp?4:3))
		return -EAGAIN;
	return count;
}

/**
 *  inv_read_temperature_raw() - Read temperature.
 *  @names:	Names of sysfs files.
 *  @data:	Data in hardware units.
 *  @timestamp:	Time when data was read from device.
 *  Returns number of bytes written or a negative error code.
 */
int inv_read_temperature_raw(const struct inv_sysfs_names_s *names, short *data,
	long long *timestamp)
{
	char str[25];
	int count;

	count = inv_sysfs_read((char*)names->temperature, sizeof(str), str);
	if (count < 0)
		return count;
	count = sscanf(str, "%hd%lld", &data[0], timestamp);
	if (count < 2)
		return -EAGAIN;
	return count;
}

/**
 *  inv_read_fifo_rate() - Read fifo rate.
 *  @names:	Names of sysfs files.
 *  @data:	Fifo rate.
 *  Returns number of bytes written or a negative error code.
 */
int inv_read_fifo_rate(const struct inv_sysfs_names_s *names, short *data)
{
	char str[8];
	int count;

	count = inv_sysfs_read((char*)names->fifo_rate, sizeof(str), str);
	if (count < 0)
		return count;
	count = sscanf(str, "%hd", data);
	if (count < 1)
		return -EAGAIN;
	return count;
}

/**
 *  inv_read_power_state() - Read power state.
 *  @names:	Names of sysfs files.
 *  @data:	1 if device is on.
 *  Returns number of bytes written or a negative error code.
 */
int inv_read_power_state(const struct inv_sysfs_names_s *names, char *data)
{
	char str[2];
	int count;

	count = inv_sysfs_read((char*)names->power_state, sizeof(str), str);
	if (count < 0)
		return count;
	count = sscanf(str, "%hd", (short*)data);
	if (count < 1)
		return -EAGAIN;
	return count;
}

/**
 *  inv_read_scale() - Read scale.
 *  @names:	Names of sysfs files.
 *  @data:	1 if device is on.
 *  Returns number of bytes written or a negative error code.
 */
int inv_read_scale(const struct inv_sysfs_names_s *names, float *data)
{
	char str[5];
	int count;

	count = inv_sysfs_read((char*)names->scale, sizeof(str), str);
	if (count < 0)
		return count;
	count = sscanf(str, "%f", data);
	if (count < 1)
		return -EAGAIN;
	return count;
}

/**
 *  inv_read_temp_scale() - Read temperature scale.
 *  @names:	Names of sysfs files.
 *  @data:	1 if device is on.
 *  Returns number of bytes written or a negative error code.
 */
int inv_read_temp_scale(const struct inv_sysfs_names_s *names, short *data)
{
	char str[4];
	int count;

	count = inv_sysfs_read((char*)names->temp_scale, sizeof(str), str);
	if (count < 0)
		return count;
	count = sscanf(str, "%hd", data);
	if (count < 1)
		return -EAGAIN;
	return count;
}

/**
 *  inv_read_temp_offset() - Read temperature offset.
 *  @names:	Names of sysfs files.
 *  @data:	1 if device is on.
 *  Returns number of bytes written or a negative error code.
 */
int inv_read_temp_offset(const struct inv_sysfs_names_s *names, short *data)
{
	char str[4];
	int count;

	count = inv_sysfs_read((char*)names->temp_offset, sizeof(str), str);
	if (count < 0)
		return count;
	count = sscanf(str, "%hd", data);
	if (count < 1)
		return -EAGAIN;
	return count;
}

/**
 *  inv_read_q16() - Get data as q16 fixed point.
 *  @names:	Names of sysfs files.
 *  @data:	1 if device is on.
 *  @timestamp:	Time when data was read from device.
 *  Returns number of bytes written or a negative error code.
 */
int inv_read_q16(const struct inv_sysfs_names_s *names, long *data,
	long long *timestamp)
{
	int count;
	short raw[3];
	float scale;
	count = inv_read_raw(names, (long*)raw, timestamp);
	count += inv_read_scale(names, &scale);
	data[0] = (long)(raw[0] * (65536.f / scale));
	data[1] = (long)(raw[1] * (65536.f / scale));
	data[2] = (long)(raw[2] * (65536.f / scale));
	return count;
}

/**
 *  inv_read_q16() - Get temperature data as q16 fixed point.
 *  @names:	Names of sysfs files.
 *  @data:	1 if device is on.
 *  @timestamp:	Time when data was read from device.
 *  Returns number of bytes read or a negative error code.
 */
int inv_read_temp_q16(const struct inv_sysfs_names_s *names, long *data,
	long long *timestamp)
{
	int count = 0;
	short raw;
	static short scale, offset;
	static unsigned char first_read = 1;

	if (first_read) {
		count += inv_read_temp_scale(names, &scale);
		count += inv_read_temp_offset(names, &offset);
		first_read = 0;
	}
	count += inv_read_temperature_raw(names, &raw, timestamp);
	data[0] = (long)((35 + ((float)(raw - offset) / scale)) * 65536.f);

	return count;
}

/**
 *  inv_write_fifo_rate() - Write fifo rate.
 *  @names:	Names of sysfs files.
 *  @data:	Fifo rate.
 *  Returns number of bytes written or a negative error code.
 */
int inv_write_fifo_rate(const struct inv_sysfs_names_s *names, short data)
{
	return inv_sysfs_write((char*)names->fifo_rate, (long)data);
}

/**
 *  inv_write_buffer_enable() - Enable/disable buffer in /dev.
 *  @names:	Names of sysfs files.
 *  @data:	Fifo rate.
 *  Returns number of bytes written or a negative error code.
 */
int inv_write_buffer_enable(const struct inv_sysfs_names_s *names, char data)
{
	return inv_sysfs_write((char*)names->enable, (long)data);
}

/**
 *  inv_write_power_state() - Turn device on/off.
 *  @names:	Names of sysfs files.
 *  @data:	1 to turn on.
 *  Returns number of bytes written or a negative error code.
 */
int inv_write_power_state(const struct inv_sysfs_names_s *names, char data)
{
	return inv_sysfs_write((char*)names->power_state, (long)data);
}



