/*
 * Copyright (c) Invensense Inc. 2012
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
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
#include <termios.h>

#include "iio_utils.h"
#include "ml_sysfs_helper.h"
#include "mlos.h"

#define POLL_TIME (2000) // 2sec

// settings
int verbose = false;

// paths
char *dev_dir_name;

/**************************************************
   This _kbhit() function is courtesy of the web
***************************************************/
int _kbhit(void)
{
    static const int STDIN = 0;
    static bool initialized = false;

    if (!initialized) {
        // Use termios to turn off line buffering
        struct termios term;
        tcgetattr(STDIN, &term);
        term.c_lflag &= ~ICANON;
        tcsetattr(STDIN, TCSANOW, &term);
        setbuf(stdin, NULL);
        initialized = true;
    }

    int bytesWaiting;
    ioctl(STDIN, FIONREAD, &bytesWaiting);
    return bytesWaiting;
}

void get_sensor_data(char *d, short *sensor)
{
    int i;
    for (i = 0; i < 3; i++)
        sensor[i] = *(short *)(d + 2 + i * 2);
}

static int read_data(char *buffer_access)
{
#define PRESSURE_HDR             0x8000
#define ACCEL_HDR                0x4000
#define GYRO_HDR                 0x2000
#define COMPASS_HDR              0x1000
#define LPQUAT_HDR               0x0800
#define SIXQUAT_HDR              0x0400
#define PEDQUAT_HDR              0x0200
#define STEP_DETECTOR_HDR        0x0100

    static int left_over_size = 0;
    char data[1048], *dptr, tmp[24];
    short sensor[3];
    int q[3];
    int ret, i, ind, fp;
    int buf_size, read_size;
    unsigned short hdr;
    bool done_flag;

    fp = open(buffer_access, O_RDONLY | O_NONBLOCK);
    if (fp == -1) { /* if it isn't there make the node */
        printf("Failed to open %s\n", buffer_access);
        ret = -errno;
        goto error_read_data;
    }
    ind = 0;
    
    {
        struct pollfd pfd = {
            .fd = fp,
            .events = POLLIN,
        };
        poll(&pfd, 1, -1);

        if (left_over_size > 0)
            memcpy(data, tmp, left_over_size);
        dptr = data + left_over_size;

        read_size = read(fp,  dptr, 1024);
        if (read_size <= 0) {
            printf("Wrong size=%d\n", read_size);
            ret = -EINVAL;
            goto error_read_data;
        }

        ind = read_size + left_over_size;
        dptr = data;
        buf_size = ind - (dptr - data);
        done_flag = false;
        while ((buf_size > 0) && (!done_flag)) {
            hdr = *((short *)(dptr));
            if (hdr & 1)
                printf("STEP\n");

            switch (hdr & (~1)) {
            case PRESSURE_HDR:
                if (buf_size >= 16) {
                    get_sensor_data(dptr, sensor);
                    dptr += 8;
                    printf("PRESS, %d, %lld\n", (sensor[1] << 16) + (unsigned short)sensor[2], *(long long *)dptr);
                } else
                    done_flag = true;
                break;
            case ACCEL_HDR:
                if (buf_size >= 16) {
                    get_sensor_data(dptr, sensor);
                    dptr += 8;
                    printf("ACCEL, %d, %d, %d, %lld\n", sensor[0], sensor[1], sensor[2], *(long long *)dptr);
                } else
                    done_flag = true;
                break;
            case GYRO_HDR:
                if (buf_size >= 16) {
                    get_sensor_data(dptr, sensor);
                    dptr += 8;
                    printf("GYRO, %d, %d, %d, %lld\n", sensor[0], sensor[1], sensor[2], *(long long *)dptr);
                } else
                    done_flag = true;
                break;
            case COMPASS_HDR:
                if (buf_size >= 16) {
                    get_sensor_data(dptr, sensor);
                    dptr += 8;
                    printf("COMPASS, %d, %d, %d, %lld\n", sensor[0], sensor[1], sensor[2], *(long long *)dptr);
                } else
                    done_flag = true;
                break;
            case PEDQUAT_HDR:
                if (buf_size >= 16) {
                    get_sensor_data(dptr, sensor);
                    dptr += 8;
                    printf("LOW_RES_QUAT, %d, %d, %d, %lld\n", sensor[0], sensor[1], sensor[2], *(long long *)dptr);
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
                    printf("LPQ_3AXES, %d, %d, %d, %lld\n", q[0], q[1], q[2], *(long long *)dptr);
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
                    printf("LPQ_6AXES, %d, %d, %d, %lld\n", q[0], q[1], q[2], *(long long *)dptr);
                }  else
                    done_flag = true;
                break;
            case STEP_DETECTOR_HDR:
                if (buf_size >= 16) {
                    printf("STEP_DETECTOR, ");
                    dptr += 8;
                    printf("%lld\n", *(long long *)dptr);
                }  else
                    done_flag = true;

                break;
            default:
                printf("unknown, \n");
                for (i = 0; i < 8; i++)
                    printf("%02x, ", dptr[i]);
                printf("\n");
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

error_read_data:
    return ret;
}

/*
    Main
*/

int main(int argc, char **argv)
{
    unsigned long num_loops = -1;
    int ret, c, i;

    int dev_num;
    char *buffer_access;
    char chip_name[10];
    char *dummy;
    char device_name[10];
    char sysfs[100];

    // all output to stdout must be delivered immediately, no buffering
    setvbuf(stdout, NULL, _IONBF, 0);

    /* parse the command line parameters 
       TODO description
    */
    while ((c = getopt(argc, argv, "c:vh")) != -1) {
        switch (c) {
        case 'c':
            num_loops = strtoul(optarg, &dummy, 10);
            break;
        case 'v':
            verbose = true;
            break;
        case 'h':
            //print_help();
            // TODO write print_help helper function
            break;
        case '?':
            return -1;
        }
    }

    // get info about the device and driver
    inv_get_sysfs_path(sysfs);
    if (inv_get_chip_name(chip_name) != INV_SUCCESS) {
        printf("get chip name fail\n");
        exit(0);
    }
    printf("INFO: chip_name=%s\n", chip_name);

    for (i = 0; i < strlen(chip_name); i++)
        device_name[i] = tolower(chip_name[i]);
    device_name[strlen(chip_name)] = '\0';
    printf("INFO: device name=%s\n", device_name);

    /* Find the device requested */
    dev_num = find_type_by_name(device_name, "iio:device");
    if (dev_num < 0) {
        printf("Failed to find the %s\n", device_name);
        ret = -ENODEV;
        goto error_ret;
    }
    printf("INFO: iio device number=%d\n", dev_num);
    
    /* attempt to open non blocking the access dev */
    ret = asprintf(&buffer_access, "/dev/iio:device%d", dev_num);
    if (ret < 0) {
        ret = -ENOMEM;
        goto error_ret;
    }
    while (num_loops == -1 || num_loops--)
        read_data(buffer_access);
    free(buffer_access);

error_ret:
    return ret;
}
