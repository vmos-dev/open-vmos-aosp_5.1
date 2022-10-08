/* Industrialio buffer test code.
 *
 * Copyright (c) 2012 Invensense Inc.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * Command line parameters
 * stress_iio -d time1 -e time2
 */

#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>
#include <linux/types.h>
#include <string.h>
#include <poll.h>
#include <pthread.h>
#include "iio_utils.h"
#include "ml_load_dmp.h"
#include "ml_sysfs_helper.h"
#include "authenticate.h"

pthread_mutex_t data_switch_lock = PTHREAD_MUTEX_INITIALIZER;

static int has_compass = 0;
static int has_pressure = 0;
static int enable_random_delay = 0;
static int enable_delay = 10;
static int disable_delay = 10;
static int enable_motion_on = 0;
static int final_output_rate;
static int first_flag;
static char dmp_path[200];

static int dev_num;
static char *dev_dir_name;
static char *buf_dir_name;
static char *scan_el_dir;
static int gyro_data_is_enabled, accel_data_is_enabled, compass_data_is_enabled, quaternion_data_is_enabled;
static int accel_engine_is_on;

struct dmp_struct {
	char fname[100];
	void (*action)(struct dmp_struct *, int);
};

static void HandleTap(struct dmp_struct *dmp, int tap);
static void sipmle_print(struct dmp_struct *dmp, int d){
	printf("%s:%d\n", dmp->fname, d);
}

static void handle_smd() {
	printf("write wake lock for SMD\n");
	//write_sysfs_string_and_verify("wake_lock", "/sys/power/", "hack");
}

static void pedo_print()
{
	struct timespec aa;
	unsigned long long t;

	clock_gettime(CLOCK_REALTIME, &aa);
	t = (unsigned long long)aa.tv_sec * 1000000000 + aa.tv_nsec;
	printf("steps=%lld, time=%lld, system=%lld\n",
		read_sysfs_poslonglong("pedometer_steps", dev_dir_name),
		read_sysfs_poslonglong("pedometer_time", dev_dir_name),
		t);
}

struct dmp_struct event_file[] = {
#if 1
	{
		.fname = "event_tap",
		.action = HandleTap,
	},
#endif
	{
		.fname = "event_smd",
		.action = handle_smd,
	},
	{
		.fname = "event_accel_motion",
		.action = sipmle_print,
	},
	{
		.fname = "event_pedometer",
		.action = pedo_print,
	},
};

static void HandleTap(struct dmp_struct *dmp, int tap)
{
    int tap_dir = tap/8;
    int tap_num = tap%8 + 1;

    switch (tap_dir) {
        case 1:
            printf("INV_TAP_AXIS_X_POS\n");
            break;
        case 2:
            printf("INV_TAP_AXIS_X_NEG\n");
            break;
        case 3:
            printf("INV_TAP_AXIS_Y_POS\n");
            break;
        case 4:
            printf("INV_TAP_AXIS_Y_NEG\n");
            break;
        case 5:
            printf("INV_TAP_AXIS_Z_POS\n");
            break;
        case 6:
            printf("INV_TAP_AXIS_Z_NEG\n");
            break;
        default:
            break;
    }
    printf("Tap number: %d\n", tap_num);
}
#define DMP_CODE_SIZE 2799
static char dmp_img[DMP_CODE_SIZE];
static void verify_img(){
    FILE *fp;
    int i;
    char dmp_path[] = "/sys/bus/iio/devices/iio:device0/dmp_firmware";

    printf("saving image\n");
    if ((fp = fopen(dmp_path, "rb")) < 0 ) {
        perror("dmp fail");
    }

    i = fread(dmp_img, 1, DMP_CODE_SIZE, fp);
    printf("Result=%d\n", i);
    fclose(fp);
    fp = fopen("/dev/read_img.h", "wt");
    fprintf(fp, "unsigned char rec[]={\n");
    for(i=0; i<DMP_CODE_SIZE; i++) {
      fprintf(fp, "0x%02x, ", dmp_img[i]);
      //printf( "0x%02x, ", dmp_img[i]);
      if(((i+1)%16) == 0) {
        fprintf(fp, "\n");
        //printf("\n");
      }
    }
    fprintf(fp, "};\n ");
    fclose(fp);
    printf("saving image Done\n");
}

static void inv_set_rate()
{
	int ret;

	printf("set rate \n");
	ret = write_sysfs_int_and_verify("accel_rate", dev_dir_name, 5);
	ret = write_sysfs_int_and_verify("gyro_rate", dev_dir_name, 5);
	if (has_compass)
		ret = write_sysfs_int_and_verify("compass_rate", dev_dir_name, 10);
	if (has_pressure)
		ret = write_sysfs_int_and_verify("pressure_rate", dev_dir_name, 30);
	ret = write_sysfs_int_and_verify("ped_q_rate", dev_dir_name, 5);
	ret = write_sysfs_int_and_verify("six_axes_q_rate", dev_dir_name, 5);
	ret = write_sysfs_int_and_verify("three_axes_q_rate", dev_dir_name, 5);
}


static int setup_offset_and_bias()
{
	int ret;

	ret = write_sysfs_int_and_verify("in_accel_x_offset", dev_dir_name, 0);
	if (ret < 0)
		printf("write accel x offset failed.\n");
	ret = write_sysfs_int_and_verify("in_accel_y_offset", dev_dir_name, 0);
	if (ret < 0)
		printf("write accel y offset failed.\n");
	ret = write_sysfs_int_and_verify("in_accel_z_offset", dev_dir_name, 0);
	if (ret < 0)
		printf("write accel z offset failed.\n");

	ret = write_sysfs_int_and_verify("in_anglvel_x_offset", dev_dir_name, 0);
	if (ret < 0)
		printf("write accel x offset failed.\n");
	ret = write_sysfs_int_and_verify("in_anglvel_y_offset", dev_dir_name, 0);
	if (ret < 0)
		printf("write accel y offset failed.\n");
	ret = write_sysfs_int_and_verify("in_anglvel_z_offset", dev_dir_name, 0);
	if (ret < 0)
		printf("write accel z offset failed.\n");

	ret = write_sysfs_int_and_verify("in_accel_x_dmp_bias", dev_dir_name, 0);
	if (ret < 0)
		printf("write accel x offset failed.\n");
	ret = write_sysfs_int_and_verify("in_accel_y_dmp_bias", dev_dir_name, 0);
	if (ret < 0)
		printf("write accel y offset failed.\n");
	ret = write_sysfs_int_and_verify("in_accel_z_dmp_bias", dev_dir_name, 0);
	if (ret < 0)
		printf("write accel z offset failed.\n");

	ret = write_sysfs_int_and_verify("in_anglvel_x_dmp_bias", dev_dir_name, 0);
	if (ret < 0)
		printf("write gyro x offset failed.\n");
	ret = write_sysfs_int_and_verify("in_anglvel_y_dmp_bias", dev_dir_name, 0);
	if (ret < 0)
		printf("write gyro y offset failed.\n");
	ret = write_sysfs_int_and_verify("in_anglvel_z_dmp_bias", dev_dir_name, 0);
	if (ret < 0)
		printf("write gyro z offset failed.\n");

	return 0;
}

static void setup_dmp(char *dev_path){
	char sysfs_path[200];
	int  ret;
	FILE *fd;
	sprintf(sysfs_path, "%s", dev_path);
	printf("sysfs: %s\n", sysfs_path);
	ret = write_sysfs_int_and_verify("power_state", sysfs_path, 1);
	if (ret < 0)
		return;

	ret = write_sysfs_int("in_accel_scale", dev_path, 0);
	if (ret < 0)
		return;
	ret = write_sysfs_int("in_anglvel_scale", dev_path, 3);
	if (ret < 0)
		return;
	ret = write_sysfs_int("sampling_frequency", sysfs_path, 200);
	if (ret < 0)
		return;
	ret = write_sysfs_int_and_verify("firmware_loaded", sysfs_path, 0);
	if (ret < 0)
		return;
	sprintf(dmp_path, "%s/dmp_firmware", dev_path);
	if ((fd = fopen(dmp_path, "wb")) < 0 ) {
		perror("dmp fail");
	}
	inv_load_dmp(fd);
	fclose(fd);
	printf("firmware_loaded=%d\n", read_sysfs_posint("firmware_loaded", sysfs_path));
	ret = write_sysfs_int_and_verify("dmp_on", sysfs_path, 1);
	if (ret < 0)
		return;
	ret = write_sysfs_int_and_verify("dmp_int_on", sysfs_path, 1);
	if (ret < 0)
		return;
	/* selelct which event to enable and interrupt on/off here */
	//enable_glu(sysfs_path, 0);
//	ret = write_sysfs_int_and_verify("tap_on", sysfs_path, 0);
//	if (ret < 0)
//		return;
	ret = write_sysfs_int_and_verify("pedometer_int_on", sysfs_path, 1);
	ret = write_sysfs_int_and_verify("pedometer_on", sysfs_path, 1);

	ret = write_sysfs_int_and_verify("dmp_event_int_on", sysfs_path, 1);
		write_sysfs_int64("pedometer_steps", sysfs_path, 0);
		write_sysfs_int64("pedometer_time", sysfs_path, 0);
	if (ret < 0)
		return;

	ret = setup_offset_and_bias();

	return;
}

#if 0
static char reg_dump_arr[2000];
static int inv_do_reg_dump(void)
{
	char reg_dump_name[100];
	int fd, i;

	sprintf(reg_dump_name, "%s/reg_dump", dev_dir_name);
	printf("%s\n", reg_dump_name);
	fd = open(reg_dump_name, O_RDONLY);
	pread(fd, reg_dump_arr, 2000, 0);
	close(fd);
	for ( i = 0; i < 2000; i++) {
		printf("%c", reg_dump_arr[i]);
		//if((i+1)%16 == 0)
			//printf("\n");
	}
	return 0;
}
#endif

static void *get_dmp_event(void *param) {
	char file_name[100];
	int i;
	int data;
	char d[4];
	FILE *fp;
	struct pollfd pfd[ARRAY_SIZE(event_file)];

	printf("get DMP event: %s\n", dev_dir_name);
	while(1) {
		for (i = 0; i < ARRAY_SIZE(event_file); i++) {
			sprintf(file_name, "%s/%s", dev_dir_name, event_file[i].fname);
			pfd[i].fd = open(file_name, O_RDONLY | O_NONBLOCK);
			pfd[i].events = POLLPRI|POLLERR;
			pfd[i].revents = 0;
			read(pfd[i].fd, d, 4);
		}

		poll(pfd, ARRAY_SIZE(event_file), -1);
		for (i = 0; i < ARRAY_SIZE(event_file); i++) {
			close(pfd[i].fd);
		}

		for (i=0; i< ARRAY_SIZE(pfd); i++) {
			if(pfd[i].revents != 0) {
				sprintf(file_name, "%s/%s", dev_dir_name, event_file[i].fname);
				fp = fopen(file_name, "rt");
				fscanf(fp, "%d\n", &data);
				event_file[i].action(&event_file[i], data);
			}
		}
	}

	return 0;
}

static int enable_gyro(int on){
	int ret;
	ret = write_sysfs_int_and_verify("gyro_enable", dev_dir_name, on);
	if (ret < 0)
		printf("write gyro_enable failed\n");

	return ret;
}

static int enable_gyro_output(int on){
	int ret;
	gyro_data_is_enabled = on;
	ret = write_sysfs_int_and_verify("gyro_fifo_enable", dev_dir_name, on);
	if (ret < 0)
		printf("write gyro_fifo_enable failed\n");

	return ret;
}

static int enable_compass(int on){
	int ret;

	compass_data_is_enabled = on;
	ret = write_sysfs_int_and_verify("compass_enable", dev_dir_name, on);
	if (ret < 0)
		printf("write gyro_enable failed\n");

	return ret;
}

static int enable_pressure(int on){
	int ret;

	ret = write_sysfs_int_and_verify("pressure_enable", dev_dir_name, on);
	if (ret < 0)
		printf("write pressure_enable failed\n");

	return ret;
}

static int enable_quaternion(int on) {
	int ret;
	ret = write_sysfs_int_and_verify("ped_q_on", dev_dir_name, on);
	if (ret < 0)
		printf("write quaternion_on failed\n");
	ret = write_sysfs_int_and_verify("six_axes_q_on", dev_dir_name, on);
	if (ret < 0)
		printf("write quaternion_on failed\n");
	ret = write_sysfs_int_and_verify("three_axes_q_on", dev_dir_name, on);
	if (ret < 0)
		printf("write quaternion_on failed\n");

	return ret;
}
static int enable_step_detector(int on) {
	int ret;

	ret = write_sysfs_int_and_verify("step_detector_on", dev_dir_name, on);
	if (ret < 0)
		printf("write step detector on failed\n");
}
static int enable_step_indicator(int on) {
	int ret;

	ret = write_sysfs_int_and_verify("step_indicator_on", dev_dir_name, on);
	if (ret < 0)
		printf("write step indicator on failed\n");
}

static int enable_accel(int on){
	int ret;
	accel_data_is_enabled = on;
	accel_engine_is_on = on;
	ret = write_sysfs_int_and_verify("accel_enable", dev_dir_name, on);
	if (ret < 0)
		printf("write accel_enable failed\n");
	ret = write_sysfs_int_and_verify("accel_fifo_enable", dev_dir_name, on);
	if (ret < 0)
		printf("write accel_fifo_enable failed\n");

	return ret;
}
static int enable_accel_output(int on) {
	int ret;
	accel_data_is_enabled = on;

	ret = write_sysfs_int_and_verify("accel_fifo_enable", dev_dir_name, on);
	if (ret < 0)
		printf("write accel_fifo_enable failed\n");

	return ret;
}

static int enable_enable(int on){
	int ret;

	if (0 == on) {
		//pthread_mutex_lock(&data_switch_lock);
	}
	ret = write_sysfs_int_and_verify("master_enable", dev_dir_name, on);
	if (ret < 0)
		printf("write enable failed\n");

	if (on) {
		//pthread_mutex_unlock(&data_switch_lock);
	}

	return 0;
}
static int write_dmp_event(int on) {
	int ret;
	ret = write_sysfs_int_and_verify("dmp_event_int_on", dev_dir_name, on);
	if (ret < 0)
		printf("write dmp_event_int_on failed\n");
	return 0;
}

static void random_delay(){
	int i;
	float bb;

	i = rand();
	bb = i * 200.0;
	bb = i * 10.0;
	i = 1 + (unsigned int)(bb/(RAND_MAX + 1.0));
	i *= 2;
	if (i%2) {
		printf("sleep %d ms\n", i);
		usleep(i*1000);
	} else {
		printf("sleep %d s\n", i);
		sleep(i);
	}

}
static void dmp_event_control(on){
	int ret;

	ret = 0;

//	ret = write_sysfs_int_and_verify("tap_on", dev_dir_name, on);
	ret = write_sysfs_int_and_verify("smd_enable", dev_dir_name, 1);
	if (ret < 0)
		return;
	inv_set_rate();

	//ret = write_sysfs_int_and_verify("batchmode_wake_fifo_full_on", dev_dir_name, 1);
	ret = write_sysfs_int_and_verify("batchmode_timeout", dev_dir_name, 10000);
	ret = write_sysfs_int_and_verify("batchmode_timeout", dev_dir_name, 0);
	//ret = write_sysfs_int_and_verify("smd_delay_threshold", dev_dir_name, 10);
	if (ret < 0)
		return;
	//ret = write_sysfs_int_and_verify("smd_threshold", dev_dir_name, 5000);
	if (ret < 0)
		return;
	//write_sysfs_int_and_verify("motion_lpa_duration", dev_dir_name, 1000);
	//write_sysfs_int_and_verify("motion_lpa_threshold", dev_dir_name, 200);
	write_sysfs_int_and_verify("dmp_on", dev_dir_name, 1);
	ret = write_sysfs_int_and_verify("sampling_frequency", dev_dir_name, 200);
	//write_sysfs_int_and_verify("motion_lpa_freq", dev_dir_name, 3);

}
void enable_motion(int on) {
	int ret;

	ret = write_sysfs_int_and_verify("motion_lpa_on", dev_dir_name, on);
	if (on) {
	        gyro_data_is_enabled = 0;
		compass_data_is_enabled = 0;
		quaternion_data_is_enabled = 0;
	}
}
bool g, a;
static int counter = 0;
static unsigned char data_rate[] = {5, 10, 15, 50, 100, 200};
static int run_enable_sequence()
{
	bool g, a, out;

	counter++;
	g = rand()%2;
	a = rand()%2;
	if (!g && !a)
		a = true;

	g = true;
//	g = false;
	a = true;
//	a = false;
	/*disable the master enable */
	enable_enable(0);
	if(g) {
		enable_gyro(1);
		if (rand()%2) {
			out = rand()%2;
			enable_quaternion(out);
			enable_gyro_output(!out);
		} else {
			enable_quaternion(1);
			enable_gyro_output(1);
		}
		enable_quaternion(1);
		enable_gyro_output(0);

	} else {
		enable_gyro(0);
		enable_gyro_output(0);
		enable_quaternion(0);
	}
	if(a) {
		enable_accel(1);
		enable_accel_output(0);
	} else {
		enable_accel(0);
		enable_accel_output(0);
	}
	if (has_compass) {
		if(rand()%2)
			enable_compass(1);
		else
			enable_compass(0);
		enable_compass(counter%2);
		enable_compass(0);
	}
	if (has_pressure) {
		if(rand()%2)
			enable_pressure(1);
		else
			enable_pressure(0);
		enable_pressure(counter%3);
		enable_pressure(0);
	}
	enable_step_detector(1);
	enable_step_indicator(1);
	enable_step_detector(0);
	enable_step_indicator(0);

	write_dmp_event(0);

	enable_motion(0);
	if (accel_engine_is_on)
		dmp_event_control(1);
	else
		dmp_event_control(0);
	first_flag = 1;
	/*enable the master enable */
	enable_enable(1);
	//enable_enable(0);
	//verify_img();
	//while(1);
	//write_sysfs_string_and_verify("wake_unlock", "/sys/power/", "hack");
	if (enable_random_delay)
		random_delay();
	else {
		printf("sleep %ds\n", enable_delay);
		sleep(enable_delay);
	}

	return 0;
}

static int run_disable_sequence() {
	enable_enable(0);

	enable_gyro(0);
	enable_accel(1);
	enable_quaternion(0);
	enable_accel_output(0);
	write_dmp_event(1);
	enable_motion(enable_motion_on);
	if (accel_engine_is_on)
		dmp_event_control(1);
	else
		dmp_event_control(0);

	enable_enable(1);
	if (enable_random_delay)
		random_delay();
	else {
		printf("sleep %ds\n", disable_delay);
		sleep(disable_delay);
	}

	if (has_pressure) {
		if(rand()%2)
			enable_pressure(1);
		else
			enable_pressure(0);
		enable_pressure(counter%3);
		enable_pressure(1);
	}

	return 0;
}
static int run_dmp_off() {
	bool g, a, out;

	counter++;
	g = rand()%2;
	a = rand()%2;
	if (!g && !a)
		a = true;

	g = true;
	a = true;
	a = false;
//	g = false;
	/*disable the master enable */
	enable_enable(0);
	if(g) {
		enable_gyro(1);
		if (rand()%2) {
			enable_gyro_output(!out);
		} else {
			enable_gyro_output(1);
		}
		enable_gyro_output(1);

	} else {
		enable_gyro(0);
		enable_gyro_output(0);
	}
	if(a) {
		enable_accel(1);
		enable_accel_output(1);
//		enable_accel_output(0);
	} else {
		enable_accel(0);
		enable_accel_output(0);
	}
	if (has_compass) {
		if(rand()%2)
			enable_compass(1);
		else
			enable_compass(0);
		enable_compass(counter%2);
		enable_compass(0);
	}
	if (has_pressure) {
		if(rand()%2)
			enable_pressure(1);
		else
			enable_pressure(0);
		enable_pressure(counter%3);
		enable_pressure(1);
	}
	printf("111111111111111\n");

	write_sysfs_int_and_verify("sampling_frequency", dev_dir_name,15);
	write_sysfs_int_and_verify("dmp_on", dev_dir_name, 0);
	first_flag = 1;
	/*enable the master enable */
	enable_enable(1);
	//sleep(2);

	return 0;
}
static void *control_switch(void *param)
{
	while(1) {
		run_enable_sequence();
		//run_dmp_off();
		printf("sleeping\n");
		//sleep(1000);
		run_disable_sequence();
	}
	return 0;
}

void get_sensor_data(char *d, short *sensor)
{
	int i;

	for (i = 0; i < 3; i++)
		sensor[i] = *(short *)(d + 2 + i * 2);
}

static void *read_data(void *param)
{
	char *buffer_access;
	char data[2048], *dptr, tmp[24];
	short sensor[3];
	int q[3], i, ind, left_over_size, buf_size;
	int ret, fp,read_size;
	unsigned short hdr;
	bool done_flag;
	struct timespec ts_1;
	unsigned long long t0, t1;
	int g_count, sq_count;

#define PRESSURE_HDR             0x8000
#define ACCEL_HDR                0x4000
#define GYRO_HDR                 0x2000
#define COMPASS_HDR              0x1000
#define COMPASS_HDR_2            0x1800
#define LPQUAT_HDR               0x0800
#define SIXQUAT_HDR              0x0400
#define PEDQUAT_HDR              0x0200
#define STEP_DETECTOR_HDR        0x0100
#define STEP_INDICATOR_HDR       0x0001
#define END_MARKER               0x0010
#define EMPTY_MARKER             0x0020

	printf("read_data Thread: %s\n", dev_dir_name);
	ret = asprintf(&scan_el_dir, FORMAT_SCAN_ELEMENTS_DIR, dev_dir_name);
	if (ret < 0)
		goto error_alloc_scan_el_dir;
	ret = asprintf(&buffer_access, "/dev/iio:device%d", dev_num);
	if (ret < 0)
		goto error_alloc_buffer_access;

	fp = open(buffer_access, O_RDONLY | O_NONBLOCK);
	if (fp == -1) { /*If it isn't there make the node */
		printf("Failed to open %s\n", buffer_access);
		ret = -errno;
		goto error_open_buffer_access;
	}
	ind = 0;

	clock_gettime(CLOCK_REALTIME, &ts_1);
	t0 = (unsigned long long)ts_1.tv_sec * 1000000000 + ts_1.tv_nsec;
	while(1) {

		clock_gettime(CLOCK_REALTIME, &ts_1);
		t1 = (unsigned long long)ts_1.tv_sec * 1000000000 + ts_1.tv_nsec;
		//printf("diff=%lld, a_count=%d, sq_count=%d\n", (t1-t0), g_count, sq_count);
		g_count = 0;
		sq_count = 0;
		t0 = t1;

		struct pollfd pfd = {
			.fd = fp,
			.events = POLLIN,
		};
		poll(&pfd, 1, -1);

		if (left_over_size > 0)
			memcpy(data, tmp, left_over_size);
		dptr = data + left_over_size;
		read_size = read(fp,  dptr, 2000);
		printf("readsize=%d, left_over_size=%d\n", read_size, left_over_size);
		if (read_size <= 0) {
			printf("Wrong size=%d\n", read_size);
			pthread_mutex_unlock(&data_switch_lock);
			continue;
		}
		ind = read_size + left_over_size;
		dptr = data;
		//printf("ind=%d\n", ind);
		buf_size = ind - (dptr - data);
		done_flag = false;

		while ((buf_size > 0) && (!done_flag)) {
			hdr = *((short *)(dptr));
			if ((hdr & 0xf) && (hdr != STEP_INDICATOR_HDR))
				printf("STEP$$$$$$$$$$$$$$$=%x  ", hdr);
			switch (hdr & (~0xf)) {
			case PRESSURE_HDR:
				if (buf_size >= 16) {
					get_sensor_data(dptr, sensor);
					dptr += 8;
					printf("PRESSURE:%d, %lld\n", (sensor[1] << 16) + (unsigned short)sensor[2],  *(long long *)dptr);
				} else
					done_flag = true;
				break;
			case ACCEL_HDR:
				if (buf_size >= 16) {
					get_sensor_data(dptr, sensor);
					dptr += 8;
					printf("A:%d, %d, %d,  %lld\n", sensor[0], sensor[1], sensor[2],   *(long long *)dptr);
				} else
					done_flag = true;
				break;
			case GYRO_HDR:
				if (buf_size >= 16) {
					get_sensor_data(dptr, sensor);
					dptr += 8;
					g_count++;
					printf("G:%d, %d, %d,  %lld\n", sensor[0], sensor[1], sensor[2],   *(long long *)dptr);
				} else
					done_flag = true;
				break;
			case COMPASS_HDR:
				if (buf_size >= 16) {
					get_sensor_data(dptr, sensor);
					dptr += 8;
					printf("M:%d, %d, %d,  %lld\n", sensor[0], sensor[1], sensor[2],   *(long long *)dptr);
				} else
					done_flag = true;
				break;
			case PEDQUAT_HDR:
				if (buf_size >= 16) {
					get_sensor_data(dptr, sensor);
					dptr += 8;
					printf("PED:%d, %d, %d,  %lld\n", sensor[0], sensor[1], sensor[2],   *(long long *)dptr);
				}  else
					done_flag = true;
				break;
			case LPQUAT_HDR:
				if (buf_size >= 24) {
					q[0] = *(int *)(dptr + 4);
					dptr += 8;
					q[1] = *(int *)(dptr);
					q[2] = *(int *)(dptr + 4);
					dptr += 8;
					printf("LPQ:%d, %d, %d,  %lld\n", q[0], q[1], q[2],   *(long long *)dptr);
				}  else
					done_flag = true;
				break;
			case SIXQUAT_HDR:
				if (buf_size >= 24) {
					q[0] = *(int *)(dptr + 4);
					dptr += 8;
					q[1] = *(int *)(dptr);
					q[2] = *(int *)(dptr + 4);
					dptr += 8;
					sq_count++;
					printf("SIXQ:%d, %d, %d,  %lld\n", q[0], q[1], q[2],   *(long long *)dptr);
				}  else
					done_flag = true;
				break;
			case STEP_DETECTOR_HDR:
				if (buf_size >= 16) {
					printf("STEP DETECTOR ");
					dptr += 8;
					printf(" %lld\n", *(long long *)dptr);
				}  else
					done_flag = true;

				break;
			default:
				if (hdr == EMPTY_MARKER) {
					printf("emptry marker !!!!!!!!!!!\n");
				} else if (hdr == END_MARKER) {
					printf("end marker !!!!!\n");
				} else if (hdr == COMPASS_HDR_2) {
					printf("bad compass data\n");
				} else {
					dptr +=8;
					printf("%lld\n", *(long long *)dptr);
				}
				break;
			}
			if (!done_flag)
				dptr += 8;
			buf_size = ind - (dptr - data);
		}
		if (ind - (dptr - data) > 0)
			memcpy(tmp, dptr, ind - (dptr - data));
		left_over_size = ind - (dptr - data);
	}
	close(fp);

error_open_buffer_access:
	free(buffer_access);
error_alloc_buffer_access:
	free(scan_el_dir);
error_alloc_scan_el_dir:

	return 0;
}

static void inv_create_thread() {
	pthread_t thread_dmp_event, thread_read_data, thread_control;

	pthread_create(&thread_dmp_event, NULL, &get_dmp_event, (void *)dev_dir_name);
	pthread_create(&thread_read_data, NULL, &read_data, (void *)dev_dir_name);
	pthread_create(&thread_control, NULL, &control_switch, (void *)dev_dir_name);

	pthread_join(thread_dmp_event, NULL);
	pthread_join(thread_read_data, NULL);
	pthread_join(thread_control, NULL);
}

static int enable_enable_main(int on){
	int ret;

	printf("enable_enable: %s=%d\n", dev_dir_name, on);
	ret = write_sysfs_int_and_verify("enable", buf_dir_name, on);
	if (ret < 0)
		printf("write enable failed\n");

	return 0;
}

int main(int argc, char **argv)
{
	unsigned long buf_len = 240;

	int ret, c, i;

	char *trigger_name = NULL;

	int datardytrigger = 1;
	int trig_num;
	char *dummy;
	char chip_name[10];
	char device_name[10];
	char sysfs[100];

	gyro_data_is_enabled = 0;
	accel_data_is_enabled = 0;
	compass_data_is_enabled = 0;
	quaternion_data_is_enabled = 0;

	while ((c = getopt(argc, argv, "lcd:e:rmp")) != -1) {
		switch (c) {
		case 'c':
			has_compass = 1;
			break;
		case 'p':
			has_pressure = 1;
			break;
		case 'd':
			disable_delay = strtoul(optarg, &dummy, 10);
			break;
		case 'e':
			enable_delay = strtoul(optarg, &dummy, 10);
			break;
		case 'r':
			enable_random_delay = 1;
			break;
		case 'm':
			enable_motion_on = 1;
			break;
		case '?':
			return -1;
		}
	}

	inv_get_sysfs_path(sysfs);
	printf("sss:::%s\n", sysfs);
	if (inv_get_chip_name(chip_name) != INV_SUCCESS) {
		printf("get chip name fail\n");
		exit(0);
	}
	printf("chip_name=%s\n", chip_name);
	if (INV_SUCCESS != inv_check_key())
        	printf("key check fail\n");
	else
        	printf("key authenticated\n");

	for (i=0; i<strlen(chip_name); i++) {
		device_name[i] = tolower(chip_name[i]);
	}
	device_name[strlen(chip_name)] = '\0';
	printf("device name: %s\n", device_name);

	/* Find the device requested */
	dev_num = find_type_by_name(device_name, "iio:device");
	if (dev_num < 0) {
		printf("Failed to find the %s\n", device_name);
		ret = -ENODEV;
		goto error_ret;
	}
	printf("iio device number being used is %d\n", dev_num);
	asprintf(&dev_dir_name, "%siio:device%d", iio_dir, dev_num);
	printf("allco=%x\n", (int)dev_dir_name);
	if (trigger_name == NULL) {
		/*
		 * Build the trigger name. If it is device associated it's
		 * name is <device_name>_dev[n] where n matches the device
		 * number found above
		 */
		ret = asprintf(&trigger_name,
			       "%s-dev%d", device_name, dev_num);
		if (ret < 0) {
			ret = -ENOMEM;
			goto error_ret;
		}
	}
	/* Verify the trigger exists */
	trig_num = find_type_by_name(trigger_name, "trigger");
	if (trig_num < 0) {
		printf("Failed to find the trigger %s\n", trigger_name);
		ret = -ENODEV;
		goto error_free_triggername;
	}
	printf("iio trigger number being used is %d\n", trig_num);
	ret = asprintf(&buf_dir_name, "%siio:device%d/buffer", iio_dir, dev_num);
	if (ret < 0) {
		ret = -ENOMEM;
		goto error_free_triggername;
	}
	enable_enable_main(0);
	ret = write_sysfs_int_and_verify("power_state", dev_dir_name, 1);
	/*
	 * Parse the files in scan_elements to identify what channels are
	 * present
	 */
	ret = 0;
	setup_dmp(dev_dir_name);

	printf("%s %s\n", dev_dir_name, trigger_name);

	/* Set the device trigger to be the data rdy trigger found above */
	ret = write_sysfs_string_and_verify("trigger/current_trigger",
					dev_dir_name,
					trigger_name);
	if (ret < 0) {
		printf("Failed to write current_trigger file\n");
		goto error_free_buf_dir_name;
	}
	/* Setup ring buffer parameters */
	/* length must be even number because iio_store_to_sw_ring is expecting
		half pointer to be equal to the read pointer, which is impossible
		when buflen is odd number. This is actually a bug in the code */
	ret = write_sysfs_int("length", buf_dir_name, buf_len*2);
	if (ret < 0)
		goto exit_here;
	enable_enable_main(1);
	inv_create_thread();
exit_here:
error_free_buf_dir_name:
	free(buf_dir_name);
error_free_triggername:
	if (datardytrigger)
		free(trigger_name);
error_ret:
	return ret;
}
