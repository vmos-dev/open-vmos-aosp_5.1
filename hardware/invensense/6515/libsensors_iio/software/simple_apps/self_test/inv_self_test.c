/**
 *  Self Test application for Invensense's MPU6xxx/MPU9xxx.
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

#include "invensense.h"
#include "ml_math_func.h"
#include "storage_manager.h"
#include "ml_stored_data.h"
#include "ml_sysfs_helper.h"
#include "data_builder.h"

#ifndef ABS
#define ABS(x)(((x) >= 0) ? (x) : -(x))
#endif

#define DEBUG_PRINT     /* Uncomment to print Gyro & Accel read from Driver */

#define MAX_SYSFS_NAME_LEN     (100)
#define MAX_SYSFS_ATTRB        (sizeof(struct sysfs_attrbs) / sizeof(char*))
#define IIO_SYSFS_PATH         "/sys/bus/iio/devices/iio:device0"
#define IIO_HUB_NAME           "inv_hub"

#define GYRO_PASS_STATUS_BIT    0x01
#define ACCEL_PASS_STATUS_BIT   0x02
#define COMPASS_PASS_STATUS_BIT 0x04

typedef union {
    long l; 
    int i;
} bias_dtype;

char *sysfs_names_ptr;
struct sysfs_attrbs {
    char *name;
    char *enable;
    int enable_value;
    char *power_state;
    int power_state_value;
    char *dmp_on;
    int dmp_on_value;
    char *self_test;
    char *temperature;
    char *gyro_enable;
    int gyro_enable_value;
    char *gyro_x_calibbias;
    char *gyro_y_calibbias;
    char *gyro_z_calibbias;
    char *gyro_scale;
    char *gyro_x_offset;
    char *gyro_y_offset;
    char *gyro_z_offset;
    char *accel_enable;
    int accel_enable_value;
    char *accel_x_calibbias;
    char *accel_y_calibbias;
    char *accel_z_calibbias;
    char *accel_scale;
    char *accel_x_offset;
    char *accel_y_offset;
    char *accel_z_offset;
    char *compass_enable;
    int compass_enable_value;
} mpu;

static struct inv_db_save_t save_data;
static bool write_biases_immediately = false;

/** This function receives the data that was stored in non-volatile memory
    between power off */
static inv_error_t inv_db_load_func(const unsigned char *data)
{
    memcpy(&save_data, data, sizeof(save_data));
    return INV_SUCCESS;
}

/** This function returns the data to be stored in non-volatile memory between
    power off */
static inv_error_t inv_db_save_func(unsigned char *data)
{
    memcpy(data, &save_data, sizeof(save_data));
    return INV_SUCCESS;
}

/** read a sysfs entry that represents an integer */
int read_sysfs_int(char *filename, int *var)
{
    int res=0;
    FILE *fp;

    fp = fopen(filename, "r");
    if (fp != NULL) {
        fscanf(fp, "%d\n", var);
        fclose(fp);
    } else {
        MPL_LOGE("inv_self_test: ERR open file to read");
        res= -1;
    }
    return res;
}

/** write a sysfs entry that represents an integer */
int write_sysfs_int(char *filename, int data)
{
    int res = 0;
    FILE *fp;

    fp = fopen(filename, "w");
    if (fp != NULL) {
        fprintf(fp, "%d\n", data);
        fclose(fp);
    } else {
        MPL_LOGE("inv_self_test: ERR open file to write");
        res= -1;
    }
    return res;
}

int android_hub(void)
{
    char dev_name[8];
    FILE *fp;

    fp = fopen(mpu.name, "r");
    fgets(dev_name, 8, fp);
    fclose (fp);
    
    if (!strncmp(dev_name, IIO_HUB_NAME, sizeof(IIO_HUB_NAME)))
        return true;
    else
        return false;
}

int save_n_write_sysfs_int(char *filename, int data, int *old_value)
{
    int res;
    res = read_sysfs_int(filename, old_value);
    if (res < 0) {
        return res;
    }
    //printf("saved %s = %d\n", filename, *old_value);
    res = write_sysfs_int(filename, data);
    if (res < 0) {
        return res;
    }
    return 0;
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
        MPL_LOGE("inv_self_test: couldn't alloc mem for sysfs paths");
        return -1;
    }

    // Setup IIO sysfs path & build MPU's sysfs paths
    sprintf(sysfs_path, "%s", IIO_SYSFS_PATH);

    sprintf(mpu.name, "%s%s", sysfs_path, "/name");
    sprintf(mpu.enable, "%s%s", sysfs_path, "/buffer/enable");
    sprintf(mpu.power_state, "%s%s", sysfs_path, "/power_state");
    sprintf(mpu.dmp_on,"%s%s", sysfs_path, "/dmp_on");
    sprintf(mpu.self_test, "%s%s", sysfs_path, "/self_test");
    sprintf(mpu.temperature, "%s%s", sysfs_path, "/temperature");

    sprintf(mpu.gyro_enable, "%s%s", sysfs_path, "/gyro_enable");
    sprintf(mpu.gyro_x_calibbias, "%s%s",
            sysfs_path, "/in_anglvel_x_calibbias");
    sprintf(mpu.gyro_y_calibbias, "%s%s",
            sysfs_path, "/in_anglvel_y_calibbias");
    sprintf(mpu.gyro_z_calibbias, "%s%s",
            sysfs_path, "/in_anglvel_z_calibbias");
    sprintf(mpu.gyro_scale,  "%s%s", sysfs_path, "/in_anglvel_self_test_scale");
    sprintf(mpu.gyro_x_offset, "%s%s", sysfs_path, "/in_anglvel_x_offset");
    sprintf(mpu.gyro_y_offset, "%s%s", sysfs_path, "/in_anglvel_y_offset");
    sprintf(mpu.gyro_z_offset, "%s%s", sysfs_path, "/in_anglvel_z_offset");

    sprintf(mpu.accel_enable, "%s%s", sysfs_path, "/accel_enable");
    sprintf(mpu.accel_x_calibbias, "%s%s", sysfs_path, "/in_accel_x_calibbias");
    sprintf(mpu.accel_y_calibbias, "%s%s", sysfs_path, "/in_accel_y_calibbias");
    sprintf(mpu.accel_z_calibbias, "%s%s", sysfs_path, "/in_accel_z_calibbias");
    sprintf(mpu.accel_scale,  "%s%s", sysfs_path, "/in_accel_self_test_scale");
    sprintf(mpu.accel_x_offset, "%s%s", sysfs_path, "/in_accel_x_offset");
    sprintf(mpu.accel_y_offset, "%s%s", sysfs_path, "/in_accel_y_offset");
    sprintf(mpu.accel_z_offset, "%s%s", sysfs_path, "/in_accel_z_offset");

    sprintf(mpu.compass_enable, "%s%s", sysfs_path, "/compass_enable");

#if 0
    // test print sysfs paths
    dptr = (char**)&mpu;
    for (i = 0; i < MAX_SYSFS_ATTRB; i++) {
        printf("inv_self_test: sysfs path: %s\n", *dptr++);
    }
#endif
    return 0;
}

void print_cal_file_content(struct inv_db_save_t *data)
{
    printf("------------------------------\n");
    printf("-- Calibration file content --\n");
    printf("   factory_gyro_bias[3]  = %+ld %+ld %+ld\n",
           data->factory_gyro_bias[0], data->factory_gyro_bias[1],
           data->factory_gyro_bias[2]);
    printf("   factory_accel_bias[3] = %+ld %+ld %+ld\n",
           data->factory_accel_bias[0], data->factory_accel_bias[1],
           data->factory_accel_bias[2]);
    printf("   compass_bias[3]      = %+ld %+ld %+ld\n",
           data->compass_bias[0], data->compass_bias[1], data->compass_bias[2]);
    printf("   gyro_temp            = %+ld\n", data->gyro_temp);
    printf("   gyro_bias_tc_set     = %+d\n", data->gyro_bias_tc_set);
    printf("   accel_temp           = %+ld\n", data->accel_temp);
    printf("   gyro_accuracy        = %d\n", data->gyro_accuracy);
    printf("   accel_accuracy       = %d\n", data->accel_accuracy);
    printf("   compass_accuracy     = %d\n", data->compass_accuracy);
    printf("------------------------------\n");
}

/*******************************************************************************
 *                       M a i n
 ******************************************************************************/
int main(int argc, char **argv)
{
    FILE *fptr;
    int self_test_status = 0;
    inv_error_t result;
    bias_dtype gyro_bias[3];
    bias_dtype gyro_scale;
    bias_dtype accel_bias[3];
    bias_dtype accel_scale;
    int scale_ratio;
    size_t packet_sz;
    int axis_sign = 1;
    unsigned char *buffer;
    long timestamp;
    long long temperature = 0;
    bool compass_present = true;
    int c;

    result= inv_init_sysfs_attributes();
    if (result)
        return -1;

    // Self-test for Non-Hub
    inv_init_storage_manager();

    // Register packet to be saved.
    result = inv_register_load_store(
                inv_db_load_func, inv_db_save_func,
                sizeof(save_data), INV_DB_SAVE_KEY);

    // Self-test for Android Hub
    if (android_hub() == true) {
        fptr = fopen(mpu.self_test, "r");
        if (fptr) {
            fscanf(fptr, "%d", &self_test_status);
            printf("\nSelf-Test:Hub:Self test result - "
                   "Gyro passed= %x, Accel passed= %x, Compass passed= %x\n",
                   (self_test_status & GYRO_PASS_STATUS_BIT), 
                   (self_test_status & ACCEL_PASS_STATUS_BIT) >> 1,
                   (self_test_status & COMPASS_PASS_STATUS_BIT) >> 2);
            fclose(fptr);
            result = 0;
        } else {
            printf("Hub-Self-Test:ERR-Couldn't invoke self-test\n");
            result = -1;
        }

        free(sysfs_names_ptr);
        return result;
    } 

    /* testing hook: if the command-line parameter is '-l' as in 'load',
       the system will read out the MLCAL_FILE */
    while ((c = getopt(argc, argv, "lw")) != -1) {
        switch (c) {
        case 'l':
            inv_get_mpl_state_size(&packet_sz);

            buffer = (unsigned char *)malloc(packet_sz + 10);
            if (buffer == NULL) {
                printf("Self-Test:Can't allocate buffer\n");
                return -1;
            }

            fptr= fopen(MLCAL_FILE, "rb");
            if (!fptr) {
                printf("Self-Test:ERR- Can't open cal file to read - %s\n",
                       MLCAL_FILE);
                result = -1;
            }
            fread(buffer, 1, packet_sz, fptr);
            fclose(fptr);

            result = inv_load_mpl_states(buffer, packet_sz);
            if (result) {
                printf("Self-Test:ERR - "
                       "Cannot load MPL states from cal file - error %d\n", 
                       result);
                free(buffer);
                return -1;
            }
            free(buffer);

            print_cal_file_content(&save_data);
            return 0;
            break;

        case 'w':
            write_biases_immediately = true;
            break;

        case '?':
            return -1;
        }
    }

    // Clear out data.
    memset(&save_data, 0, sizeof(save_data));
    memset(gyro_bias,0, sizeof(gyro_bias));
    memset(accel_bias,0, sizeof(accel_bias));

    // enable the device
    if (save_n_write_sysfs_int(mpu.power_state, 1, 
                               &mpu.power_state_value) < 0) {
        printf("Self-Test:ERR-Failed to set power_state=1\n");
    }
    if (save_n_write_sysfs_int(mpu.enable, 0, &mpu.enable_value) < 0) {
        printf("Self-Test:ERR-Failed to disable master enable\n");
    }
    if (save_n_write_sysfs_int(mpu.dmp_on, 0, &mpu.dmp_on_value) < 0) {
        printf("Self-Test:ERR-Failed to disable DMP\n");
    }
    if (save_n_write_sysfs_int(mpu.accel_enable, 1, 
                               &mpu.accel_enable_value) < 0) {
        printf("Self-Test:ERR-Failed to enable accel\n");
    }
    if (save_n_write_sysfs_int(mpu.gyro_enable, 1, 
                               &mpu.gyro_enable_value) < 0) {
        printf("Self-Test:ERR-Failed to enable gyro\n");
    }
    if (save_n_write_sysfs_int(mpu.compass_enable, 1, 
                               &mpu.compass_enable_value) < 0) {
#ifdef DEBUG_PRINT
        printf("Self-Test:ERR-Failed to enable compass\n");
#endif
        compass_present = false;
    } 

    fptr = fopen(mpu.self_test, "r");
    if (!fptr) {
        printf("Self-Test:ERR-Couldn't invoke self-test\n");
        result = -1;
        goto free_sysfs_storage;
    }

    // Invoke self-test 
    fscanf(fptr, "%d", &self_test_status);
    if (compass_present == true) {
        printf("Self-Test:Self test result- "
               "Gyro passed = %x, Accel passed = %x, Compass passed = %x\n",
               (self_test_status & GYRO_PASS_STATUS_BIT), 
               (self_test_status & ACCEL_PASS_STATUS_BIT) >> 1,
               (self_test_status & COMPASS_PASS_STATUS_BIT) >> 2);
    } else {
        printf("Self-Test:Self test result- "
               "Gyro passed = %x, Accel passed = %x\n",
               (self_test_status & GYRO_PASS_STATUS_BIT),
               (self_test_status & ACCEL_PASS_STATUS_BIT) >> 1);
    }
    fclose(fptr);

    if (self_test_status & GYRO_PASS_STATUS_BIT) {
        // Read Gyro Bias
        if (read_sysfs_int(mpu.gyro_x_calibbias, &gyro_bias[0].i) < 0 ||
            read_sysfs_int(mpu.gyro_y_calibbias, &gyro_bias[1].i) < 0 ||
            read_sysfs_int(mpu.gyro_z_calibbias, &gyro_bias[2].i) < 0 ||
            read_sysfs_int(mpu.gyro_scale, &gyro_scale.i) < 0) {
            memset(gyro_bias, 0, sizeof(gyro_bias));
            gyro_scale.i = 0;
            printf("Self-Test:Failed to read Gyro bias\n");
        } else {
            save_data.gyro_accuracy = 3;
#ifdef DEBUG_PRINT
            printf("Self-Test:Gyro bias[0..2] = [%d %d %d]\n", 
                   gyro_bias[0].i, gyro_bias[1].i, gyro_bias[2].i);
#endif
        }
    } else {
        printf("Self-Test:Failed Gyro self-test\n");
    }

    if (self_test_status & ACCEL_PASS_STATUS_BIT) {
        // Read Accel Bias
        if (read_sysfs_int(mpu.accel_x_calibbias, &accel_bias[0].i) < 0 ||
            read_sysfs_int(mpu.accel_y_calibbias, &accel_bias[1].i) < 0 ||
            read_sysfs_int(mpu.accel_z_calibbias, &accel_bias[2].i) < 0 ||
            read_sysfs_int(mpu.accel_scale, &accel_scale.i) < 0) {
            memset(accel_bias, 0, sizeof(accel_bias));
            accel_scale.i = 0;
            printf("Self-Test:Failed to read Accel bias\n");
        } else {
            save_data.accel_accuracy = 3;
#ifdef DEBUG_PRINT
            printf("Self-Test:Accel bias[0..2] = [%d %d %d]\n", 
                   accel_bias[0].i, accel_bias[1].i, accel_bias[2].i);
#endif
       }
    } else {
        printf("Self-Test:Failed Accel self-test\n");
    }

    if (!(self_test_status & (GYRO_PASS_STATUS_BIT | ACCEL_PASS_STATUS_BIT))) {
        printf("Self-Test:Failed Gyro and Accel self-test, "
               "nothing left to do\n");
        result = -1;
        goto restore_settings;
    }

    // Read temperature
    fptr = fopen(mpu.temperature, "r");
    if (fptr != NULL) {
        fscanf(fptr,"%lld %ld", &temperature, &timestamp);
        fclose(fptr);
    } else {
        printf("Self-Test:ERR-Couldn't read temperature\n");
    }

    /*
        When we read gyro bias from sysfs, the bias is in the raw units scaled by 
        1000 at the full scale used during self-test 
        (in_anglvel_self_test_scale).
        We store the biases in raw units (+/- 2000 dps full scale assumed) 
        scaled by 2^16 therefore the gyro_bias[] have to be divided by the 
        ratio of 2000 / gyro_scale to comply.
    */
    // TODO read this from the regular full scale in sysfs
    scale_ratio = 2000 / gyro_scale.i;
    save_data.factory_gyro_bias[0] = 
                        (long)(gyro_bias[0].l / 1000.f * 65536.f / scale_ratio);
    save_data.factory_gyro_bias[1] = 
                        (long)(gyro_bias[1].l / 1000.f * 65536.f / scale_ratio);
    save_data.factory_gyro_bias[2] = 
                        (long)(gyro_bias[2].l / 1000.f * 65536.f / scale_ratio);

    // Save temperature @ time stored.
    //  Temperature is in degrees Celsius scaled by 2^16
    save_data.gyro_temp = temperature * (1L << 16);
    save_data.gyro_bias_tc_set = true;
    save_data.accel_temp = save_data.gyro_temp;

    // When we read accel bias, the bias is in raw units scaled by 1000.
    // and it contains the gravity vector.
    // Find the orientation of the device, by looking for gravity
    int axis = 0;
    if (ABS(accel_bias[1].l) > ABS(accel_bias[0].l)) {
        axis = 1;
    }
    if (ABS(accel_bias[2].l) > ABS(accel_bias[axis].l)) {
        axis = 2;
    }
    if (accel_bias[axis].l < 0) {
        axis_sign = -1;
    }

    // Convert scaling from raw units scaled by 1000 to raw scaled by 2^16
    /*
        When we read accel bias from sysfs, the bias is in the raw units scaled
        by 1000 at the full scale used during self-test 
        (in_accel_self_test_scale).
        We store the biases in raw units (+/- 2 gee full scale assumed) 
        scaled by 2^16 therefore the accel_bias[] have to be multipled by the 
        ratio of accel_scale / 2 to comply.
    */
    // TODO read this from the regular full scale in sysfs
    scale_ratio = accel_scale.i / 2;
    save_data.factory_accel_bias[0] =
                    (long)(accel_bias[0].l / 1000.f * 65536.f * scale_ratio);
    save_data.factory_accel_bias[1] =
                    (long)(accel_bias[1].l / 1000.f * 65536.f * scale_ratio);
    save_data.factory_accel_bias[2] =
                    (long)(accel_bias[2].l / 1000.f * 65536.f * scale_ratio);

#ifdef DEBUG_PRINT
    printf("Self-Test:Saved Accel bias[0..2] = [%ld %ld %ld]\n",
           save_data.factory_accel_bias[0], save_data.factory_accel_bias[1],
           save_data.factory_accel_bias[2]);
#endif

    /*
        Remove gravity, gravity in raw units should be 16384 = 2^14 for a +/- 
        2 gee setting. The data has been saved in Hw units scaled to 2^16,
        so gravity needs to scale up accordingly.
    */
    /* gravity correction for accel_bias format */
    long gravity = axis_sign * (32768L / accel_scale.i) * 1000L;
    accel_bias[axis].l -= gravity; 
    /* gravity correction for saved_data.accel_bias format */
    gravity = axis_sign * (32768L / accel_scale.i) * 65536L;
#ifdef DEBUG_PRINT
    printf("Self-Test:Gravity = %ld\n", gravity);
#endif
    save_data.factory_accel_bias[axis] -= gravity;

    printf("Self-Test:Saved Accel bias (gravity removed) [0..2] = "
           "[%ld %ld %ld]\n",
           save_data.factory_accel_bias[0], save_data.factory_accel_bias[1],
           save_data.factory_accel_bias[2]);
    /* write the accel biases down to the hardware now */
    if (write_biases_immediately) {
        int offsets[3] = {0};
        /* NOTE: 16 is the gee scale of the dmp_bias settings */
        scale_ratio = 16 / accel_scale.i;
        /* NOTE: the 2 factor is to account the halved resolution for the 
                 accel offset registers */
        offsets[0] = -accel_bias[0].l / 1000 / 2 / scale_ratio;
        if (write_sysfs_int(mpu.accel_x_offset, offsets[0]) < 0) {
            printf("Self-Test:ERR-Failed to write %s\n", mpu.accel_x_offset);
        }
        offsets[1] = -accel_bias[1].l / 1000 / 2 / scale_ratio;
        if (write_sysfs_int(mpu.accel_y_offset, offsets[1]) < 0) {
            printf("Self-Test:ERR-Failed to write %s\n", mpu.accel_y_offset);
        }
        offsets[2] = -accel_bias[2].l / 1000 / 2 / scale_ratio;
        if (write_sysfs_int(mpu.accel_z_offset, offsets[2]) < 0) {
            printf("Self-Test:ERR-Failed to write %s\n", mpu.accel_z_offset);
        }
        printf("Self-Test:Written Accel offsets[0..2] = [%d %d %d]\n",
               offsets[0], offsets[1], offsets[2]);
    }

    printf("Self-Test:Saved Gyro bias[0..2] = [%ld %ld %ld]\n",
           save_data.factory_gyro_bias[0], save_data.factory_gyro_bias[1],
           save_data.factory_gyro_bias[2]);
    /* write the gyro biases down to the hardware now */
    if (write_biases_immediately) {
        int offsets[3] = {0};
        /* NOTE: 1000 is the dps scale of the offset registers */
        scale_ratio = 1000 / gyro_scale.i;
        offsets[0] = -gyro_bias[0].l / 1000 / scale_ratio;
        if (write_sysfs_int(mpu.gyro_x_offset, offsets[0]) < 0) {
            printf("Self-Test:ERR-Failed to write %s\n", mpu.gyro_x_offset);
        }
        offsets[1] = -gyro_bias[1].l / 1000 / scale_ratio;
        if (write_sysfs_int(mpu.gyro_y_offset, offsets[1]) < 0) {
            printf("Self-Test:ERR-Failed to write %s\n", mpu.gyro_y_offset);
        }
        offsets[2] = -gyro_bias[2].l / 1000 / scale_ratio;
        if (write_sysfs_int(mpu.gyro_z_offset, offsets[2]) < 0) {
            printf("Self-Test:ERR-Failed to write %s\n", mpu.gyro_z_offset);
        }
        printf("Self-Test:Written Gyro offsets[0..2] = [%d %d %d]\n",
               offsets[0], offsets[1], offsets[2]);
    }

    printf("Self-Test:Gyro temperature @ time stored %ld\n",
           save_data.gyro_temp);
    printf("Self-Test:Accel temperature @ time stored %ld\n",
           save_data.accel_temp);

    // Get size of packet to store.
    inv_get_mpl_state_size(&packet_sz);

    // Create place to store data
    buffer = (unsigned char *)malloc(packet_sz + 10);
    if (buffer == NULL) {
        printf("Self-Test:Can't allocate buffer\n");
        result = -1;
        goto free_sysfs_storage;
    }

    // Store the data
    result = inv_save_mpl_states(buffer, packet_sz);
    if (result) {
        result = -1;
    } else {
        fptr = fopen(MLCAL_FILE, "wb+");
        if (fptr != NULL) {
            fwrite(buffer, 1, packet_sz, fptr);
            fclose(fptr);
        } else {
            printf("Self-Test:ERR- Can't open calibration file to write - %s\n", 
                   MLCAL_FILE);
            result= -1;
        }
    }

    free(buffer);

restore_settings:
    if (write_sysfs_int(mpu.dmp_on, mpu.dmp_on_value) < 0) {
        printf("Self-Test:ERR-Failed to restore dmp_on\n");
    }
    if (write_sysfs_int(mpu.accel_enable, mpu.accel_enable_value) < 0) {
        printf("Self-Test:ERR-Failed to restore accel_enable\n");
    }
    if (write_sysfs_int(mpu.gyro_enable, mpu.gyro_enable_value) < 0) {
        printf("Self-Test:ERR-Failed to restore gyro_enable\n");
    }
    if (compass_present) {
        if (write_sysfs_int(mpu.compass_enable, mpu.compass_enable_value) < 0) {
            printf("Self-Test:ERR-Failed to restore compass_enable\n");
        }
    }
    if (write_sysfs_int(mpu.enable, mpu.enable_value) < 0) {
        printf("Self-Test:ERR-Failed to restore buffer/enable\n");
    }
    if (write_sysfs_int(mpu.power_state, mpu.power_state_value) < 0) {
        printf("Self-Test:ERR-Failed to restore power_state\n");
    }

free_sysfs_storage:
    free(sysfs_names_ptr);
    return result;
}

