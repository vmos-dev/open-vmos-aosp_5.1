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
#include "ml_load_dmp.h"
#include "ml_sysfs_helper.h"
#include "authenticate.h"
#include "mlos.h"

#define DMP_CODE_SIZE (3060)
#define POLL_TIME     (2000) // 2sec

// settings
static int accel_only = false;
static int test_motion = false;
static int test_flick = false;
static int test_pedometer = false;
static int test_orientation = false;
int verbose = false;

// paths
char *dev_dir_name, *buf_dir_name;

// all the DMP features supported
enum {
    FEAT_TAP = 0,
    FEAT_ORIENTATION,
    FEAT_DISPLAY_ORIENTATION,
    FEAT_MOTION,
    FEAT_FLICK,

    FEAT_NUM,
} features;

typedef void (*handler_t) (int data);

struct dmp_feat_t {
    int enabled;
    struct pollfd *pollfd;
    char *sysfs_name;
    handler_t phandler;
};

static struct dmp_feat_t dmp_feat[FEAT_NUM] = {{0}};
static struct pollfd pollfds[FEAT_NUM];
static int pollfds_used = 0;

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

/**
 * size_from_channelarray() - calculate the storage size of a scan
 * @channels: the channel info array
 * @num_channels: size of the channel info array
 *
 * Has the side effect of filling the channels[i].location values used
 * in processing the buffer output.
 */
int size_from_channelarray(struct iio_channel_info *channels, int num_channels)
{
    int bytes = 0;
    int i = 0;
    while (i < num_channels) {
        if (bytes % channels[i].bytes == 0)
            channels[i].location = bytes;
        else
            channels[i].location = bytes - bytes%channels[i].bytes
                + channels[i].bytes;
        bytes = channels[i].location + channels[i].bytes;
        i++;
    }
    return bytes;
}

void print2byte(int input, struct iio_channel_info *info)
{
    /* shift before conversion to avoid sign extension
       of left aligned data */
    input = input >> info->shift;
    if (info->is_signed) {
        int16_t val = input;
        val &= (1 << info->bits_used) - 1;
        val = (int16_t)(val << (16 - info->bits_used)) >>
            (16 - info->bits_used);
        /*printf("%d, %05f, scale=%05f", val,
               (float)(val + info->offset)*info->scale, info->scale);*/
        printf("%d, ", val);

    } else {
        uint16_t val = input;
        val &= (1 << info->bits_used) - 1;
        printf("%05f ", ((float)val + info->offset)*info->scale);
    }
}

/**
 * process_scan() - print out the values in SI units
 * @data:        pointer to the start of the scan
 * @infoarray:        information about the channels. Note
 *  size_from_channelarray must have been called first to fill the
 *  location offsets.
 * @num_channels:    the number of active channels
 */
void process_scan(char *data, struct iio_channel_info *infoarray, 
          int num_channels)
{
    int k;
    //char *tmp;
    for (k = 0; k < num_channels; k++) {
        switch (infoarray[k].bytes) {
            /* only a few cases implemented so far */
        case 2:
            print2byte(*(uint16_t *)(data + infoarray[k].location),
                   &infoarray[k]);
            //tmp = data + infoarray[k].location;
            break;
        case 4:
            if (infoarray[k].is_signed) {
                int32_t val = *(int32_t *)(data + infoarray[k].location);
                if ((val >> infoarray[k].bits_used) & 1)
                    val = (val & infoarray[k].mask) | ~infoarray[k].mask;
                /* special case for timestamp */
                printf(" %d ", val);
            }
            break;
        case 8:
            if (infoarray[k].is_signed) {
                int64_t val = *(int64_t *)(data + infoarray[k].location);
                if ((val >> infoarray[k].bits_used) & 1)
                    val = (val & infoarray[k].mask) | ~infoarray[k].mask;
                /* special case for timestamp */
                if (infoarray[k].scale == 1.0f &&
                    infoarray[k].offset == 0.0f)
                    printf(" %lld", val);
                else
                    printf("%05f ", ((float)val + infoarray[k].offset)
                            * infoarray[k].scale);
            }
            break;
        default:
            break;
        }
    }
    printf("\n");
}

/*
    Enablers for the gestures
*/

int enable_flick(char *p, int on)
{
    int ret;
    printf("flick:%s\n", p);
    ret = write_sysfs_int_and_verify("flick_int_on", p, on);
    if (ret < 0)
        return ret;
    ret = write_sysfs_int_and_verify("flick_upper", p, 3147790);
    if (ret < 0)
        return ret;
    ret = write_sysfs_int_and_verify("flick_lower", p, -3147790);
    if (ret < 0)
        return ret;
    ret = write_sysfs_int_and_verify("flick_counter", p, 50);
    if (ret < 0)
        return ret;
    ret = write_sysfs_int_and_verify("flick_message_on", p, 0);
    if (ret < 0)
        return ret;
    ret = write_sysfs_int_and_verify("flick_axis", p, 0);
    if (ret < 0)
        return ret;

    return 0;
}

void verify_img(char *dmp_path)
{
    FILE *fp;
    int i;
    char dmp_img[DMP_CODE_SIZE];

    if ((fp = fopen(dmp_path, "rb")) < 0) {
        perror("dmp fail");
    }
    i = fread(dmp_img, 1, DMP_CODE_SIZE, fp);
    printf("Result=%d\n", i);
    fclose(fp);
    fp = fopen("/dev/read_img.h", "wt");
    fprintf(fp, "char rec[]={\n");
    for(i = 0; i < DMP_CODE_SIZE; i++) {
        fprintf(fp, "0x%02x, ", dmp_img[i]);
        if(((i + 1) % 16) == 0) {
            fprintf(fp, "\n");
        }
    }
    fprintf(fp, "};\n ");
    fclose(fp);
}

int setup_dmp(char *dev_path, int p_event)
{
    char dmp_path[100];
    int ret;
    FILE *fd;

    printf("INFO: sysfs path=%s\n", dev_path);

    ret = write_sysfs_int_and_verify("power_state", dev_path, 1);
    if (ret < 0)
        return ret;

    ret = write_sysfs_int("in_accel_scale", dev_path, 0);
    if (ret < 0)
        return ret;
    ret = write_sysfs_int("in_anglvel_scale", dev_path, 3);
    if (ret < 0)
        return ret;
    ret = write_sysfs_int("sampling_frequency", dev_path, 200);
    if (ret < 0)
        return ret;
    ret = write_sysfs_int_and_verify("firmware_loaded", dev_path, 0);
    if (ret < 0)
        return ret;

    sprintf(dmp_path, "%s/dmp_firmware", dev_path);
    if ((fd = fopen(dmp_path, "wb")) < 0 ) {
        perror("dmp fail");
    }    
    inv_load_dmp(fd);
    fclose(fd);

    printf("INFO: firmware_loaded=%d\n",
           read_sysfs_posint("firmware_loaded", dev_path));

    // set accel offsets
    //ret = write_sysfs_int_and_verify("in_accel_x_offset", 
    //                                 dev_path, 0xabcd0000);
    //if (ret < 0)
    //    return ret;
    //ret = write_sysfs_int_and_verify("in_accel_y_offset", 
    //                                 dev_path, 0xffff0000);
    //if (ret < 0)
    //    return ret;
    //ret = write_sysfs_int_and_verify("in_accel_z_offset", 
    //                                 dev_path, 0xcdef0000);
    //if (ret < 0)
    //    return ret;

    ret = write_sysfs_int_and_verify("dmp_on", dev_path, 1);
    if (ret < 0)
        return ret;
    ret = write_sysfs_int_and_verify("dmp_int_on", dev_path, 1);
    if (ret < 0)
        return ret;

    /* select which event to enable and interrupt on/off here */
    if (test_flick) {
        ret = enable_flick(dev_path, 1);
        if (ret < 0)
            return ret;
    }

    /*
    ret = write_sysfs_int_and_verify("tap_on", dev_path, 1);
    if (ret < 0)
        return ret;
    */

    /*ret = write_sysfs_int_and_verify("display_orientation_on",
                                     dev_path, 1);
    if (ret < 0)
        return ret;*/
    if (test_orientation) {
        ret = write_sysfs_int_and_verify("orientation_on", dev_path, 1);
        if (ret < 0)
            return ret;
    }
   /* ret = write_sysfs_int_and_verify("dmp_output_rate", dev_path, 25);
    if (ret < 0)
        return ret;*/
    ret = write_sysfs_int_and_verify("dmp_event_int_on", dev_path, p_event);
    if (ret < 0)
        return ret;

    //verify_img(dmp_path);
    return 0;
}

/*
    Handlers for the gestures
*/

void handle_flick(int flick)
{
    printf("flick=%x\n", flick);
}

void handle_display_orientation(int orient)
{
    printf("display_orientation=%x\n", orient);
}

void handle_motion(int motion)
{
    printf("motion=%x\n", motion);
}

void handle_orientation(int orient)
{
    printf("orientation=");
    if (orient & 0x01)
        printf("+X, ");
    if (orient & 0x02) 
        printf("-X, ");
    if (orient & 0x04) 
        printf("+Y, ");
    if (orient & 0x08) 
        printf("-Y, ");
    if (orient & 0x10) 
        printf("+Z, ");
    if (orient & 0x20) 
        printf("-Z, ");
    if (orient & 0x40)
        printf("flip");
    printf("\n");
}

void handle_tap(int tap)
{
    int tap_dir = tap / 8;
    int tap_num = tap % 8 + 1;

    printf("tap=");
    switch (tap_dir) {
        case 1:
            printf("+X, ");
            break;
        case 2:
            printf("-X, ");
            break;
        case 3:
            printf("+Y, ");
            break;
        case 4:
            printf("-Y, ");
            break;
        case 5:
            printf("+Z, ");
            break;
        case 6:
            printf("-Z, ");
            break;
        default:
            break;
    }
    printf("#%d\n", tap_num);
}

int handle_pedometer(int *got_event)
{
    static int last_pedometer_steps = -1;
    static long last_pedometer_time = -1;
    static unsigned long last_pedometer_poll = 0L;
    static unsigned long pedometer_poll_timeout = 500L; // .5 second

    unsigned long now;
    int pedometer_steps;
    long pedometer_time;

#ifdef DEBUG_PRINT
    printf("GT:Pedometer Handler\n");
#endif

    if ((now = inv_get_tick_count()) - last_pedometer_poll
            < pedometer_poll_timeout) {
        return 0;
    }
    last_pedometer_poll = now;

    pedometer_steps = read_sysfs_posint("pedometer_steps", dev_dir_name);
    pedometer_time = read_sysfs_posint("pedometer_time", dev_dir_name);

    if (last_pedometer_steps == -1 && last_pedometer_time == -1) {
        if (!*got_event)
            printf("\n");
        printf("p> pedometer=%d, %ld ",
               pedometer_steps, pedometer_time);
        if (pedometer_steps > 10 
                || pedometer_time > (pedometer_poll_timeout * 2))
            printf("(resumed)\n");
        else
            printf("\n");
        *got_event = true;
    } else if (last_pedometer_steps != pedometer_steps
                    || last_pedometer_time != pedometer_time) {
        if (!*got_event)
            printf("\n");
        printf("p> pedometer=%d, %ld\n", 
               pedometer_steps, pedometer_time);
        *got_event = true;
    }

    last_pedometer_steps = pedometer_steps;
    last_pedometer_time = pedometer_time;

    return 0;
}

/*
    Main processing functions
*/

void dump_dmp_event_struct(void)
{
#define VARVAL(f, v) printf("\t%s : " f "\n", #v, v);
    int i;

    printf("dmp_feat structure content:\n");
    for (i = 0; i < FEAT_NUM; i++) {
        printf("%d - ", i);
        VARVAL("%d", dmp_feat[i].enabled);
        VARVAL("%s", dmp_feat[i].sysfs_name);
        VARVAL("%p", dmp_feat[i].phandler);
        VARVAL("%p", dmp_feat[i].pollfd);
        if (dmp_feat[i].pollfd) {
            VARVAL("%d", dmp_feat[i].pollfd->events);
            VARVAL("%d", dmp_feat[i].pollfd->revents);
            VARVAL("%d", dmp_feat[i].pollfd->fd);
        }
    }
    printf("dmp_feat structure content:\n");
    for (i = 0; i < FEAT_NUM; i++) {
        printf("%d - ", i);
        VARVAL("%d", pollfds[i].fd);
        VARVAL("%d", pollfds[i].events);
        VARVAL("%d", pollfds[i].revents);
    }
    printf("end.\n");
}

void init_dmp_event_fds(void)
{
    int i, j = 0;
    char file_name[100];

    for (i = 0; i < FEAT_NUM; i++) {
        if (!dmp_feat[i].enabled)
            continue;
        sprintf(file_name, "%s/%s", dev_dir_name, dmp_feat[i].sysfs_name);
        pollfds[j].fd = open(file_name, O_RDONLY | O_NONBLOCK);
        if (pollfds[j].fd < 0) {
            printf("Err: cannot open requested event file '%s'\n", file_name);
        } else {
            printf("INFO: opened event node '%s'\n", file_name);
        }
        pollfds[j].events = POLLPRI | POLLERR;
        pollfds[j].revents = 0;

        dmp_feat[i].pollfd = &pollfds[j];
        j++;
    }
}

void close_dmp_event_fds(void)
{
    int i;
    for (i = 0; i < pollfds_used; i++)
        close(pollfds[i].fd);
}

void poll_dmp_event_fds(void)
{
    int i;
    char d[4];
    static int got_event = 1;

    // read the pollable fds
    for (i = 0; i < pollfds_used; i++)
        read(pollfds[i].fd, d, 4);

    // poll
    if (got_event)
        printf("e> ");
    got_event = false;
    poll(pollfds, pollfds_used, POLL_TIME);

    for (i = 0; i < FEAT_NUM; i++) {
        if (!dmp_feat[i].enabled)
            continue;

        if (dmp_feat[i].pollfd->revents != 0) {
            char file_name[200];
            int data;

            sprintf(file_name, "%s/%s",
                    dev_dir_name, dmp_feat[i].sysfs_name);
            FILE *fp = fopen(file_name, "rt");
            if (!fp) {
                printf("Err:cannot open requested event file '%s'\n",
                       dmp_feat[i].sysfs_name);
                continue;
            }
            fscanf(fp, "%d\n", &data);
            fclose(fp);
            dmp_feat[i].pollfd->revents = 0;

            dmp_feat[i].phandler(data);
            got_event = true;
        }
    }

    if (test_pedometer) {
        /* pedometer is not event based, therefore we poll using a timer every
           pedometer_poll_timeout milliseconds */
        handle_pedometer(&got_event);
    }
}

/*
    Main
*/

int main(int argc, char **argv)
{
    unsigned long num_loops = 2;
    unsigned long timedelay = 100000;
    unsigned long buf_len = 128;

    int ret, c, i, j, toread;
    int fp;

    int num_channels;
    char *trigger_name = NULL;

    int datardytrigger = 1;
    char *data;
    int read_size;
    int dev_num, trig_num;
    char *buffer_access;
    int scan_size;
    int noevents = 0;
    int p_event = 0, nodmp = 0;
    char *dummy;
    char chip_name[10];
    char device_name[10];
    char sysfs[100];

    struct iio_channel_info *infoarray;

    // all output to stdout must be delivered immediately, no buffering
    setvbuf(stdout, NULL, _IONBF, 0);

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

    /* parse the command line parameters 
      -r means no DMP is enabled (raw) -> should be used for mpu3050.
       -p means no print of data 
       when using -p, 1 means orientation, 2 means tap, 3 means flick */
    while ((c = getopt(argc, argv, "l:w:c:premavt:")) != -1) {
        switch (c) {
        case 't':
            trigger_name = optarg;
            datardytrigger = 0;
            break;
        case 'e':
            noevents = 1;
            break;
        case 'p':
            p_event = 1;
            break;
        case 'r':
            nodmp = 1;
            break;
        case 'c':
            num_loops = strtoul(optarg, &dummy, 10);
            break;
        case 'w':
            timedelay = strtoul(optarg, &dummy, 10);
            break;
        case 'l':
            buf_len = strtoul(optarg, &dummy, 10);
            break;
        case 'm':
            test_motion = true;
            break;
        case 'a':
            accel_only = true;
            break;
        case 'v':
            verbose = true;
            break;
        case '?':
            return -1;
        }
    }

    pollfds_used = 0;

    // comment out/remove/if(0) the block corresponding to the feature 
    //  that you want to disable

    if (0) {
        struct dmp_feat_t f = {
            true, 
            NULL, 
            "event_tap",
            handle_tap
        };
        dmp_feat[pollfds_used] = f;
        pollfds_used++;
    }
    if (test_orientation) {
        struct dmp_feat_t f = {
            true, 
            NULL, 
            "event_orientation",
            handle_orientation
        };
        dmp_feat[pollfds_used] = f;
        pollfds_used++;
    }
    /*if (1) {
        struct dmp_feat_t f = {
            true, 
            NULL, 
            "event_display_orientation",
            handle_display_orientation
        };
        dmp_feat[pollfds_used] = f;
        pollfds_used++;
    }*/
    if (test_motion) {
        struct dmp_feat_t f = {
            true,
            NULL,
            "event_accel_motion",
            handle_motion
        };
        dmp_feat[pollfds_used] = f;
        pollfds_used++;
    }
    if (test_flick) {
        struct dmp_feat_t f = {
            true, 
            NULL, 
            "event_flick", 
            handle_flick
        };
        dmp_feat[pollfds_used] = f;
        pollfds_used++;
    }

    // debug
    printf("INFO\n");
    printf("INFO: Configured features:\n");
    for (i = 0; i < pollfds_used; i++)
        printf("INFO:   %d -> %s\n", i, dmp_feat[i].sysfs_name);
    printf("INFO\n");

    /* Find the device requested */
    dev_num = find_type_by_name(device_name, "iio:device");
    if (dev_num < 0) {
        printf("Failed to find the %s\n", device_name);
        ret = -ENODEV;
        goto error_ret;
    }
    printf("INFO: iio device number=%d\n", dev_num);
    asprintf(&dev_dir_name, "%siio:device%d", iio_dir, dev_num);
    if (trigger_name == NULL) {
        /*
         * Build the trigger name. If it is device associated it's
         * name is <device_name>_dev[n] where n matches the device
         * number found above
         */
        ret = asprintf(&trigger_name, "%s-dev%d", device_name, dev_num);
        if (ret < 0) {
            ret = -ENOMEM;
            goto error_ret;
        }
    }

   ret = write_sysfs_int_and_verify("master_enable", dev_dir_name, 0);
    if (ret < 0)
        return ret;
    ret = write_sysfs_int_and_verify("buffer/enable", dev_dir_name, 0);
    if (ret < 0)
        return ret;
    ret = write_sysfs_int_and_verify("power_state", dev_dir_name, 1);

    //
    //  motion interrupt in low power accel mode
    //
    if (test_motion) {
        ret = write_sysfs_int_and_verify("motion_lpa_on", dev_dir_name, 1);
        if (ret < 0)
            return ret;
        // magnitude threshold - range [0, 1020] in 32 mg increments
        ret = write_sysfs_int_and_verify("motion_lpa_threshold", dev_dir_name, 
                                         3 * 32);
        if (ret < 0)
            return ret;
        // duration in ms up to 2^16
      //  ret = write_sysfs_int_and_verify("motion_lpa_dur", dev_dir_name, 
        //                                 200 * 1);
        //if (ret < 0)
          //  return ret;
        // motion_lpa_freq: 0 for 1.25, 1 for 5, 2 for 20, 3 for 40 Hz update rate 
        //  of the low power accel mode. 
        //  The higher the rate, the better responsiveness of the motion interrupt.
        ret = write_sysfs_int("motion_lpa_freq", dev_dir_name, 2);
        if (ret < 0)
            return ret;
    } else {
        ret = write_sysfs_int_and_verify("motion_lpa_on", dev_dir_name, 0);
        if (ret < 0)
            return ret;
    }

    /* Verify the trigger exists */
    trig_num = find_type_by_name(trigger_name, "trigger");
    if (trig_num < 0) {
        printf("Failed to find the trigger %s\n", trigger_name);
        ret = -ENODEV;
        goto error_free_triggername;
    }
    printf("INFO: iio trigger number=%d\n", trig_num);

    if (!nodmp)
        setup_dmp(dev_dir_name, p_event);

    /*
     * Construct the directory name for the associated buffer.
     * As we know that the lis3l02dq has only one buffer this may
     * be built rather than found.
     */
    ret = asprintf(&buf_dir_name, "%siio:device%d/buffer", iio_dir, dev_num);
    if (ret < 0) {
        ret = -ENOMEM;
        goto error_free_triggername;
    }

    /* Set the device trigger to be the data rdy trigger found above */
    ret = write_sysfs_string_and_verify("trigger/current_trigger",
                    dev_dir_name,
                    trigger_name);
    if (ret < 0) {
        printf("Failed to write current_trigger file\n");
        goto error_free_buf_dir_name;
    }

    /* Setup ring buffer parameters
       length must be even number because iio_store_to_sw_ring is expecting 
       half pointer to be equal to the read pointer, which is impossible
       when buflen is odd number. This is actually a bug in the code */
    ret = write_sysfs_int("length", buf_dir_name, buf_len * 2);
    if (ret < 0)
        goto exit_here;

    // gyro
    if (accel_only) {
        ret = enable_anglvel_se(dev_dir_name, &infoarray, &num_channels, 0);
        if (ret < 0)
            return ret;
        ret = write_sysfs_int_and_verify("gyro_enable", dev_dir_name, 0);
        if (ret < 0)
            return ret;
    } else {
        ret = enable_anglvel_se(dev_dir_name, &infoarray, &num_channels, 1);
        if (ret < 0)
            return ret;
        ret = write_sysfs_int_and_verify("gyro_enable", dev_dir_name, 1);
        if (ret < 0)
            return ret;
    }

    // accel
    ret = enable_accel_se(dev_dir_name, &infoarray, &num_channels, 1);
    if (ret < 0)
        return ret;
    ret = write_sysfs_int_and_verify("accel_enable", dev_dir_name, 1);
    if (ret < 0)
        return ret;

    // quaternion
    if (!nodmp) {
        ret = enable_quaternion_se(dev_dir_name, &infoarray, &num_channels, 1);
        if (ret < 0)
            return ret;
        ret = write_sysfs_int_and_verify("three_axes_q_on", dev_dir_name, 1);
        if (ret < 0)
            return ret;
    } else {
        ret = enable_quaternion_se(dev_dir_name, &infoarray, &num_channels, 0);
        if (ret < 0)
            return ret;
        ret = write_sysfs_int_and_verify("dmp_on", dev_dir_name, 0);
        if (ret < 0)
            return ret;
    }

    //sprintf(dmp_path, "%s/dmp_firmware", dev_dir_name);
    //verify_img(dmp_path);

    ret = build_channel_array(dev_dir_name, &infoarray, &num_channels);
    if (ret) {
        printf("Problem reading scan element information\n");
        goto exit_here;
    }

    /* enable the buffer */
    ret = write_sysfs_int_and_verify("enable", buf_dir_name, 1);
    if (ret < 0)
        goto exit_here;
    scan_size = size_from_channelarray(infoarray, num_channels);
    data = malloc(scan_size * buf_len);
    if (!data) {
        ret = -ENOMEM;
        goto exit_here;
    }
/*ADDED*/
   ret = write_sysfs_int_and_verify("master_enable", dev_dir_name, 1);
    if (ret < 0)
        return ret;
    if (p_event) {

        /* polling events from the DMP */
        init_dmp_event_fds();
        while(!_kbhit())
            poll_dmp_event_fds();
        close_dmp_event_fds();

    } else {

        /* attempt to open non blocking the access dev */
        ret = asprintf(&buffer_access, "/dev/iio:device%d", dev_num);
        if (ret < 0) {
            ret = -ENOMEM;
            goto error_free_data;
        }
        fp = open(buffer_access, O_RDONLY | O_NONBLOCK);
        if (fp == -1) { /*If it isn't there make the node */
            printf("Failed to open %s\n", buffer_access);
            ret = -errno;
            goto error_free_buffer_access;
        }
        /* wait for events num_loops times */
        for (j = 0; j < num_loops; j++) {
            if (!noevents) {
                struct pollfd pfd = {
                    .fd = fp,
                    .events = POLLIN,
                };
                poll(&pfd, 1, -1);
                toread = 1;
                if (j % 128 == 0)
                    usleep(timedelay);

            } else {
                usleep(timedelay);
                toread = 1;
            }
            read_size = read(fp, data, toread * scan_size);
            if (read_size == -EAGAIN) {
                printf("nothing available\n");
                continue;
            }
            if (!p_event) {
                for (i = 0; i < read_size / scan_size; i++)
                    process_scan(data + scan_size * i, infoarray, num_channels);
            }
        }
        close(fp);
    }

error_free_buffer_access:
    free(buffer_access);
error_free_data:
    free(data);
exit_here:
    /* stop the ring buffer */
    ret = write_sysfs_int_and_verify("enable", buf_dir_name, 0);
    /* disable the dmp */
    if (p_event)
        ret = write_sysfs_int_and_verify("dmp_on", dev_dir_name, 0);

error_free_buf_dir_name:
    free(buf_dir_name);
error_free_triggername:
    if (datardytrigger)
        free(trigger_name);
error_ret:
    return ret;
}
