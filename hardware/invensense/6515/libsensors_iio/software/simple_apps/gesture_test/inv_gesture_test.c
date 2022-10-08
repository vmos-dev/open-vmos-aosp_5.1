/**
 *  Gesture Test application for Invensense's MPU6/9xxx (w/ DMP).
 */

#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <features.h>
#include <dirent.h>
#include <string.h>
#include <poll.h>
#include <stddef.h>
#include <linux/input.h>
#include <time.h>
#include <linux/time.h>
#include <unistd.h>
#include <termios.h>

#include "invensense.h"
#include "ml_math_func.h"
#include "storage_manager.h"
#include "ml_stored_data.h"
#include "ml_sysfs_helper.h"
#include "mlos.h"

//#define DEBUG_PRINT   /* Uncomment to print Gyro & Accel read from Driver */

#define SUPPORT_SCREEN_ORIENTATION
//#define SUPPORT_TAP
//#define SUPPORT_ORIENTATION
#define SUPPORT_PEDOMETER
#define SUPPORT_SMD

#define MAX_SYSFS_NAME_LEN  (100)
#define MAX_SYSFS_ATTRB     (sizeof(struct sysfs_attrbs) / sizeof(char*))
#define IIO_SYSFS_PATH      "/sys/bus/iio/devices/iio:device0"
#define IIO_HUB_NAME        "inv_hub"

#define POLL_TIME           (2000) // 2sec

struct sysfs_attrbs {
    char *name;
    char *enable;
    char *power_state;
    char *dmp_on;
    char *dmp_int_on;
    char *dmp_firmware;
    char *firmware_loaded;
#ifdef SUPPORT_SCREEN_ORIENTATION
    char *event_display_orientation;
    char *display_orientation_on;
#endif
#ifdef SUPPORT_ORIENTATION
    char *event_orientation;
    char *orientation_on;
#endif
#ifdef SUPPORT_TAP
    char *event_tap;
    char *tap_min_count;
    char *tap_on;
    char *tap_threshold;
    char *tap_time;
#endif
#ifdef SUPPORT_PEDOMETER
    char *pedometer_on;
    char *pedometer_steps;
    char *pedometer_time;
#endif
#ifdef SUPPORT_SMD
    char *event_smd;
    char *smd_enable;
    char *smd_threshold;
    char *smd_delay_threshold;
    char *smd_delay_threshold2;
#endif
} mpu;

enum {
#ifdef SUPPORT_TAP
    FEAT_TAP,
#endif
#ifdef SUPPORT_SCREEN_ORIENTATION
    FEAT_SCREEN_ORIENTATION,
#endif
#ifdef SUPPORT_ORIENTATION
    FEAT_ORIENTATION,
#endif
#ifdef SUPPORT_PEDOMETER
    FEAT_PEDOMETER,
#endif
#ifdef SUPPORT_SMD
    FEAT_SMD,
#endif

    NUM_DMP_FEATS
};

char *sysfs_names_ptr;
#ifdef SUPPORT_PEDOMETER
unsigned long last_pedometer_poll = 0L;
unsigned long pedometer_poll_timeout = 500L; // .5 second
#endif
struct pollfd pfd[NUM_DMP_FEATS];
bool android_hub = false;   // flag to indicate true=Hub, false=non-hub

/*******************************************************************************
 *                       DMP Feature Supported Functions
 ******************************************************************************/

int read_sysfs_int(char *filename, int *var)
{
    int res=0;
    FILE *fp;

    fp = fopen(filename, "r");
    if (fp!=NULL) {
        fscanf(fp, "%d\n", var);
        fclose(fp);
    } else {
        printf("ERR open file to read: %s\n", filename);
        res= -1;
    }
    return res;
}

int write_sysfs_int(char *filename, int data)
{
    int res=0;
    FILE  *fp;

#ifdef DEBUG_PRINT
    printf("writing '%s' with '%d'\n", filename, data);
#endif

    fp = fopen(filename, "w");
    if (fp != NULL) {
        fprintf(fp, "%d\n", data);
        fclose(fp);
    } else {
        printf("ERR open file to write: %s\n", filename);
        res = -1;
    }
    return res;
}

/**************************************************
    This _kbhit() function is courtesy of the web
***************************************************/
int _kbhit(void)
{
    static const int STDIN = 0;
    static bool initialized = false;

    if (! initialized) {
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

int inv_init_sysfs_attributes(void)
{
    unsigned char i = 0;
    char sysfs_path[MAX_SYSFS_NAME_LEN];
    char *sptr;
    char **dptr;

    sysfs_names_ptr =
            (char*)malloc(sizeof(char[MAX_SYSFS_ATTRB][MAX_SYSFS_NAME_LEN]));
    sptr = sysfs_names_ptr;
    if (sptr != NULL) {
        dptr = (char**)&mpu;
        do {
            *dptr++ = sptr;
            sptr += sizeof(char[MAX_SYSFS_NAME_LEN]);
        } while (++i < MAX_SYSFS_ATTRB);
    } else {
        printf("couldn't alloc mem for sysfs paths\n");
        return -1;
    }

    // get proper (in absolute/relative) IIO path & build MPU's sysfs paths
    inv_get_sysfs_path(sysfs_path);

    sprintf(mpu.name, "%s%s", sysfs_path, "/name");
    sprintf(mpu.enable, "%s%s", sysfs_path, "/buffer/enable");
    sprintf(mpu.power_state, "%s%s", sysfs_path, "/power_state");
    sprintf(mpu.dmp_on,"%s%s", sysfs_path, "/dmp_on");
    sprintf(mpu.dmp_int_on, "%s%s", sysfs_path, "/dmp_int_on");
    sprintf(mpu.dmp_firmware, "%s%s", sysfs_path, "/dmp_firmware");
    sprintf(mpu.firmware_loaded, "%s%s", sysfs_path, "/firmware_loaded");

#ifdef SUPPORT_SCREEN_ORIENTATION
    sprintf(mpu.event_display_orientation, "%s%s", 
            sysfs_path, "/event_display_orientation");
    sprintf(mpu.display_orientation_on, "%s%s", 
            sysfs_path, "/display_orientation_on");
#endif
#ifdef SUPPORT_ORIENTATION
    sprintf(mpu.event_orientation, "%s%s", sysfs_path, "/event_orientation");
    sprintf(mpu.orientation_on, "%s%s", sysfs_path, "/orientation_on");
#endif
#ifdef SUPPORT_TAP
    sprintf(mpu.event_tap, "%s%s", sysfs_path, "/event_tap");
    sprintf(mpu.tap_min_count, "%s%s", sysfs_path, "/tap_min_count");
    sprintf(mpu.tap_on, "%s%s", sysfs_path, "/tap_on");
    sprintf(mpu.tap_threshold, "%s%s", sysfs_path, "/tap_threshold");
    sprintf(mpu.tap_time, "%s%s", sysfs_path, "/tap_time");
#endif
#ifdef SUPPORT_PEDOMETER
    sprintf(mpu.pedometer_on, "%s%s", sysfs_path, "/dmp_on");
    sprintf(mpu.pedometer_steps, "%s%s", sysfs_path, "/pedometer_steps");
    sprintf(mpu.pedometer_time, "%s%s", sysfs_path, "/pedometer_time");
#endif
#ifdef SUPPORT_SMD
    sprintf(mpu.event_smd, "%s%s", sysfs_path, "/event_smd");
    sprintf(mpu.smd_enable, "%s%s", sysfs_path, "/smd_enable");
    sprintf(mpu.smd_threshold, "%s%s", sysfs_path, "/smd_threshold");
    sprintf(mpu.smd_delay_threshold, "%s%s", 
            sysfs_path, "/smd_delay_threshold");
    sprintf(mpu.smd_delay_threshold2, "%s%s", 
            sysfs_path, "/smd_delay_threshold2");
#endif

#if 0
    // test print sysfs paths
    dptr = (char**)&mpu;
    for (i = 0; i < MAX_SYSFS_ATTRB; i++) {
        MPL_LOGE("sysfs path: %s", *dptr++);
    }
#endif
    return 0;
}

int dmp_fw_loaded(void)
{
    int fw_loaded;
    if (read_sysfs_int(mpu.firmware_loaded, &fw_loaded) < 0)
        fw_loaded= 0;
    return fw_loaded;
}

int is_android_hub(void)
{
    char dev_name[8];
    FILE *fp;

    fp= fopen(mpu.name, "r");
    fgets(dev_name, 8, fp);
    fclose(fp);
    
    if (!strncmp(dev_name, IIO_HUB_NAME, sizeof(IIO_HUB_NAME))) {
        android_hub = true;
    }else {
        android_hub = false;
    }

    return 0;
}

/*
    Enablers for the gestures
*/

int master_enable(int en)
{
    if (write_sysfs_int(mpu.enable, en) < 0) {
        printf("GT:ERR-can't write 'buffer/enable'");
        return -1;
    }
    return 0;
}

#ifdef SUPPORT_TAP
int enable_tap(int en)
{
    if (write_sysfs_int(mpu.tap_on, en) < 0) {
        printf("GT:ERR-can't write 'tap_on'\n");
        return -1;
    }

    return 0;
}
#endif

/* Unnecessary: pedometer_on == dmp_on, which is always on 
#ifdef SUPPORT_PEDOMETER
int enable_pedometer(int en)
{
    if (write_sysfs_int(mpu.pedometer_on, en) < 0) {
        printf("GT:ERR-can't write 'pedometer_on'\n");
        return -1;
    }

    return 0;
}
#endif
*/

#ifdef SUPPORT_SCREEN_ORIENTATION
int enable_display_orientation(int en)
{
    if (write_sysfs_int(mpu.display_orientation_on, en) < 0) {
        printf("GT:ERR-can't write 'display_orientation_on'\n");
        return -1;
    }

    return 0;
}
#endif

#ifdef SUPPORT_ORIENTATION
int enable_orientation(int en)
{
    if (write_sysfs_int(mpu.orientation_on, en) < 0) {
        printf("GT:ERR-can't write 'orientation_on'\n");
        return -1;
    }

    return 0;
}
#endif

#ifdef SUPPORT_SMD
int enable_smd(int en)
{
    if (write_sysfs_int(mpu.smd_enable, en) < 0) {
        printf("GT:ERR-can't write 'smd_enable'\n");
        return -1;
    }
    return 0;
}
#endif

/*
    Handlers for the gestures
*/
#ifdef SUPPORT_TAP
int tap_handler(void)
{
    FILE *fp;
    int tap, tap_dir, tap_num;

    fp = fopen(mpu.event_tap, "rt");
    fscanf(fp, "%d\n", &tap);
    fclose(fp);

    tap_dir = tap/8;
    tap_num = tap%8 + 1;

#ifdef DEBUG_PRINT
    printf("GT:Tap Handler **\n");
    printf("Tap= %x\n", tap);
    printf("Tap Dir= %x\n", tap_dir);
    printf("Tap Num= %x\n", tap_num);
#endif

    switch (tap_dir) {
        case 1:
            printf("Tap Axis->X Pos, ");
            break;
        case 2:
            printf("Tap Axis->X Neg, ");
            break;
        case 3:
            printf("Tap Axis->Y Pos, ");
            break;
        case 4:
            printf("Tap Axis->Y Neg, ");
            break;
        case 5:
            printf("Tap Axis->Z Pos, ");
            break;
        case 6:
            printf("Tap Axis->Z Neg, ");
            break;
        default:
            printf("Tap Axis->Unknown, ");
            break;
    }
    printf("#%d\n", tap_num);

    return 0;
}
#endif

#ifdef SUPPORT_PEDOMETER
int pedometer_handler(void)
{
    FILE *fp;
    static int last_pedometer_steps = -1;
    static long last_pedometer_time = -1;
    int pedometer_steps;
    long pedometer_time;

#ifdef DEBUG_PRINT
    printf("GT:Pedometer Handler\n");
#endif

    fp = fopen(mpu.pedometer_steps, "rt");
    fscanf(fp, "%d\n", &pedometer_steps);
    fclose(fp);

    fp = fopen(mpu.pedometer_time, "rt");
    fscanf(fp, "%ld\n", &pedometer_time);
    fclose(fp);

    if (last_pedometer_steps == -1 && last_pedometer_time == -1) {
        printf("Pedometer Steps: %d Time: %ld ",
               pedometer_steps, pedometer_time);
        if (pedometer_steps > 10 
                || pedometer_time > (pedometer_poll_timeout * 2))
            printf("(resumed)\n");
        else
            printf("\n");
    } else if (last_pedometer_steps != pedometer_steps
                    || last_pedometer_time != pedometer_time) {
    printf("Pedometer Steps: %d Time: %ld\n", 
           pedometer_steps, pedometer_time);
    }

    last_pedometer_steps = pedometer_steps;
    last_pedometer_time = pedometer_time;

    return 0;
}
#endif

#ifdef SUPPORT_SCREEN_ORIENTATION
int display_orientation_handler(void)
{
    FILE *fp;
    int orient;

#ifdef DEBUG_PRINT
    printf("GT:Screen Orient Handler\n");
#endif

    fp = fopen(mpu.event_display_orientation, "rt");
    if (!fp) {
        printf("GT:Cannot open '%s'\n", mpu.event_display_orientation);
        return -1;
    }
    fscanf(fp, "%d\n", &orient);
    fclose(fp);

    printf("Screen Orient-> %d\n", orient);

    return 0;
}
#endif

#ifdef SUPPORT_ORIENTATION
int host_orientation_handler(void)
{
    FILE *fp;
    int orient;

    fp = fopen(mpu.event_orientation, "rt");
    fscanf(fp, "%d\n", &orient);
    fclose(fp);

#ifdef DEBUG_PRINT
    printf("GT:Reg Orient Handler\n");
#endif

    if (orient & 0x01)
        printf("Orient->X Up\n");
    if (orient & 0x02)
        printf("Orient->X Down\n");
    if (orient & 0x04)
        printf("Orient->Y Up\n");
    if (orient & 0x08)
        printf("Orient->Y Down\n");
    if (orient & 0x10)
        printf("Orient->Z Up\n");
    if (orient & 0x20)
        printf("Orient->Z Down\n");
    if (orient & 0x40)
        printf("Orient->Flip\n");

    return 0;
}
#endif

#ifdef SUPPORT_SMD
int smd_handler(void)
{
    FILE *fp;
    int smd;

    fp = fopen(mpu.event_smd, "rt");
    fscanf(fp, "%d\n", &smd);
    fclose(fp);

#ifdef DEBUG_PRINT
    printf("GT:SMD Handler\n");
#endif
    printf("SMD (%d)\n", smd);

    /* wait for the acceleration low pass filtered tail to die off -
       this is to prevent that the tail end of a 2nd event of above threhsold 
       motion be considered as also the 1st event for the next SM detection */
    inv_sleep(1000);

    /* re-enable to continue the detection */
    master_enable(0);
    enable_smd(1);
    master_enable(1);

    return 0;
}
#endif

int enable_dmp_features(int en)
{
    int res= -1;

    if (android_hub || dmp_fw_loaded()) {
        /* Currently there's no info regarding DMP's supported features/capabilities
           An error in enabling features below could be an indication of the feature
           not supported in current loaded DMP firmware */

        master_enable(0);
#ifdef SUPPORT_TAP
        enable_tap(en);
#endif
#ifdef SUPPORT_SCREEN_ORIENTATION
        enable_display_orientation(en);
#endif
#ifdef SUPPORT_ORIENTATION
        if (android_hub == false) {
            // Android Hub does not support 'regular' orientation feature
            enable_orientation(en);
        }
#endif
#ifdef SUPPORT_SMD
        enable_smd(en);
#endif
        master_enable(1);
        res = 0;

    } else {
        printf("GT:ERR-No DMP firmware\n");
        res= -1;
    }

    return res;
}

int init_fds(void)
{
    int i;

    for (i = 0; i < NUM_DMP_FEATS; i++) {
        switch(i) {
#ifdef SUPPORT_TAP
        case FEAT_TAP:
            pfd[i].fd = open(mpu.event_tap, O_RDONLY | O_NONBLOCK);
            break;
#endif
#ifdef SUPPORT_SCREEN_ORIENTATION
        case FEAT_SCREEN_ORIENTATION:
            pfd[i].fd = open(mpu.event_display_orientation,
                             O_RDONLY | O_NONBLOCK);
            break;
#endif
#ifdef SUPPORT_ORIENTATION
        case FEAT_ORIENTATION:
            pfd[i].fd = open(mpu.event_orientation, O_RDONLY | O_NONBLOCK);
            break;
#endif
#ifdef SUPPORT_SMD
        case FEAT_SMD:
            pfd[i].fd = open(mpu.event_smd, O_RDONLY | O_NONBLOCK);
            break;
#endif
        default:
            pfd[i].fd = -1;
        }

        pfd[i].events = POLLPRI|POLLERR,
        pfd[i].revents = 0;
    }

    return 0;
}

void parse_events(struct pollfd pfd[], int num_fds)
{
    int i;

    for (i = 0; i < num_fds; i++) {
        if(pfd[i].revents != 0) {
            switch(i) {
#ifdef SUPPORT_TAP
            case FEAT_TAP:
                tap_handler();
                break;
#endif
#ifdef SUPPORT_SCREEN_ORIENTATION
            case FEAT_SCREEN_ORIENTATION:
                display_orientation_handler();
                break;
#endif
#ifdef SUPPORT_ORIENTATION
            case FEAT_ORIENTATION:
                host_orientation_handler();
                break;
#endif
#ifdef SUPPORT_SMD
            case FEAT_SMD:
                smd_handler();
                break;
#endif
            default:
                printf("GT:ERR-unhandled/unrecognized gesture event");
                break;
            }
            pfd[i].revents = 0;   // no need: reset anyway
        }
    }

#ifdef SUPPORT_PEDOMETER
    {
        unsigned long now;
        // pedometer is not event based, therefore we poll using a timer every
        //  pedometer_poll_timeout milliseconds
        if ((now = inv_get_tick_count()) - last_pedometer_poll
                > pedometer_poll_timeout) {
            pedometer_handler();
            last_pedometer_poll = now;
        }
    }
#endif
}

int close_fds(void)
{
    int i;
    for (i = 0; i < NUM_DMP_FEATS; i++) {
        if (!pfd[i].fd)
            close(pfd[i].fd);
    }
    return 0;
}

/*******************************************************************************
 *                       M a i n
 ******************************************************************************/

int main(int argc, char **argv)
{
    char data[4];
    int i, res= 0;

    printf("\n"
           "****************************************************************\n"
           "*** NOTE:                                                    ***\n"
           "***       the HAL must be compiled with Low power quaternion ***\n"
           "***           and/or DMP screen orientation support.         ***\n"
           "***       'At least' one of the 4 Android virtual sensors    ***\n"
           "***           must be enabled.                               ***\n"
           "***                                                          ***\n"
           "*** Please perform gestures to see the output.               ***\n"
           "*** Press any key to stop the program.                       ***\n"
           "****************************************************************\n"
           "\n");

    res = inv_init_sysfs_attributes();
    if (res) {
        printf("GT:ERR-Can't allocate mem\n");
        return -1;
    }

    /* check if Android Hub */
    is_android_hub();

    /* init Fds to poll for gesture data */
    init_fds();

    /* on Gesture/DMP supported features */
    if (enable_dmp_features(1) < 0) {
        printf("GT:ERR-Can't enable Gestures\n");
        return -1;
    }

    do {
        for (i = 0; i < NUM_DMP_FEATS; i++)
            read(pfd[i].fd, data, 4);
        poll(pfd, NUM_DMP_FEATS, POLL_TIME);
        parse_events(pfd, NUM_DMP_FEATS);
    } while (!_kbhit());

    /* off Gesture/DMP supported features */
    if (enable_dmp_features(0) < 0) {
        printf("GT:ERR-Can't disable Gestures\n");
        return -1;
    }

    /* release resources */
    close_fds();
    if (sysfs_names_ptr)
        free(sysfs_names_ptr);

    return res;
}

