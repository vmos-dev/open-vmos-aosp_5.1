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

/*
    Includes, Defines, and Macros
*/

#undef MPL_LOG_NDEBUG
#define MPL_LOG_NDEBUG 0 /* turn to 0 to enable verbose logging */

#include "log.h"
#undef MPL_LOG_TAG
#define MPL_LOG_TAG "MPL-playback"

#include "and_constructor.h"
#include "mlos.h"
#include "invensense.h"
#include "invensense_adv.h"

/*
    Typedef
*/
struct inv_construct_t {
    int product; /**< Gyro Product Number */
    int debug_mode;
    int last_mode;
    FILE *file;
    int dmp_version;
    int left_in_buffer;
#define FIFO_READ_SIZE 100
    unsigned char fifo_data[FIFO_READ_SIZE];
    int gyro_enable;
    int accel_enable;
    int compass_enable;
    int quat_enable;
};

/*
    Globals
*/
static struct inv_construct_t inv_construct = {0};
static void (*s_func_cb)(void);
static char playback_filename[101] = "/data/playback.bin";
struct fifo_dmp_config fifo_dmp_cfg = {0};

/*
    Functions
*/
void inv_set_playback_filename(char *filename, int length)
{
    if (length > 100) {
        MPL_LOGE("Error : file name and path too long, 100 characters limit\n");
        return;
    }
    strncpy(playback_filename, filename, length);
}

inv_error_t inv_constructor_setup(void)
{
    unsigned short orient;
    extern signed char g_gyro_orientation[9];
    extern signed char g_accel_orientation[9];
    extern signed char g_compass_orientation[9];
    float scale = 2.f;
    long sens;

    // gyro setup
    orient = inv_orientation_matrix_to_scalar(g_gyro_orientation);
    inv_set_gyro_orientation_and_scale(orient, 2000L << 15);

    // accel setup
    orient = inv_orientation_matrix_to_scalar(g_accel_orientation);
    scale = 2.f;
    sens = (long)(scale * (1L << 15));
    inv_set_accel_orientation_and_scale(orient, sens);

    // compass setup
    orient = inv_orientation_matrix_to_scalar(g_compass_orientation);
    // scale is the max value of the compass in micro Tesla.
    scale = 5000.f;
    sens = (long)(scale * (1L << 15));
    inv_set_compass_orientation_and_scale(orient, sens);

    return INV_SUCCESS;
}

inv_error_t inv_set_fifo_processed_callback(void (*func_cb)(void))
{
    s_func_cb = func_cb;
    return INV_SUCCESS;
}

void int32_to_long(int32_t in[], long out[], int length)
{
    int ii;
    for (ii = 0; ii < length; ii++)
        out[ii] = (long)in[ii];
}

inv_error_t inv_playback(void)
{
    inv_rd_dbg_states type;
    inv_time_t ts;
    int32_t buffer[4];
    short gyro[3];
    size_t r = 1;
    int32_t orientation;
    int32_t sensitivity, sample_rate_us = 0;

    // Check to make sure we were request to playback
    if (inv_construct.debug_mode != RD_PLAYBACK) {
        MPL_LOGE("%s|%s|%d error: debug_mode != RD_PLAYBACK\n",
                 __FILE__, __func__, __LINE__);
        return INV_ERROR;
    }

    if (inv_construct.file == NULL) {
        inv_construct.file = fopen(playback_filename, "rb");
        if (!inv_construct.file) {
            MPL_LOGE("Error : cannot find or open playback file '%s'\n",
                     playback_filename);
            return INV_ERROR_FILE_OPEN;
        }
    }

    while (1) {
        r = fread(&type, sizeof(type), 1, inv_construct.file);
        if (r == 0) {
            MPL_LOGV("read 0 bytes, PLAYBACK file closed\n");
            inv_construct.debug_mode = RD_NO_DEBUG;
            fclose(inv_construct.file);
            break;
        }
        //MPL_LOGV("TYPE : %d, %d\n", type);
        switch (type) {
        case PLAYBACK_DBG_TYPE_GYRO:
            r = fread(gyro, sizeof(gyro[0]), 3, inv_construct.file);
            r = fread(&ts, sizeof(ts), 1, inv_construct.file);
            inv_build_gyro(gyro, ts);
            MPL_LOGV("PLAYBACK_DBG_TYPE_GYRO, %+d, %+d, %+d, %+lld\n",
                     gyro[0], gyro[1], gyro[2], ts);
            break;
        case PLAYBACK_DBG_TYPE_ACCEL:
        {
            long accel[3];
            r = fread(buffer, sizeof(buffer[0]), 3, inv_construct.file);
            r = fread(&ts, sizeof(ts), 1, inv_construct.file);
            int32_to_long(buffer, accel, 3);
            inv_build_accel(accel, 0, ts);
            MPL_LOGV("PLAYBACK_DBG_TYPE_ACCEL, %+d, %+d, %+d, %lld\n",
                     buffer[0], buffer[1], buffer[2], ts);
            break;
        }
        case PLAYBACK_DBG_TYPE_COMPASS:
        {
            long compass[3];
            r = fread(buffer, sizeof(buffer[0]), 3, inv_construct.file);
            r = fread(&ts, sizeof(ts), 1, inv_construct.file);
            int32_to_long(buffer, compass, 3);
            inv_build_compass(compass, 0, ts);
            MPL_LOGV("PLAYBACK_DBG_TYPE_COMPASS, %+d, %+d, %+d, %lld\n",
                     buffer[0], buffer[1], buffer[2], ts);
            break;
        }
        case PLAYBACK_DBG_TYPE_TEMPERATURE:
            r = fread(buffer, sizeof(buffer[0]), 1, inv_construct.file);
            r = fread(&ts, sizeof(ts), 1, inv_construct.file);
            inv_build_temp(buffer[0], ts);
            MPL_LOGV("PLAYBACK_DBG_TYPE_TEMPERATURE, %+d, %lld\n",
                     buffer[0], ts);
            break;
        case PLAYBACK_DBG_TYPE_QUAT:
        {
            long quat[4];
            r = fread(buffer, sizeof(buffer[0]), 4, inv_construct.file);
            r = fread(&ts, sizeof(ts), 1, inv_construct.file);
            int32_to_long(buffer, quat, 4);
            inv_build_quat(quat, INV_BIAS_APPLIED, ts);
            MPL_LOGV("PLAYBACK_DBG_TYPE_QUAT, %+d, %+d, %+d, %+d, %lld\n",
                     buffer[0], buffer[1], buffer[2], buffer[3], ts);
            break;
        }
        case PLAYBACK_DBG_TYPE_EXECUTE:
            MPL_LOGV("PLAYBACK_DBG_TYPE_EXECUTE\n");
            inv_execute_on_data();
            if (s_func_cb)
                s_func_cb();
            //done = 1;
            break;

        case PLAYBACK_DBG_TYPE_G_ORIENT:
            MPL_LOGV("PLAYBACK_DBG_TYPE_G_ORIENT\n");
            r = fread(&orientation, sizeof(orientation), 1, inv_construct.file);
            r = fread(&sensitivity, sizeof(sensitivity), 1, inv_construct.file);
            inv_set_gyro_orientation_and_scale(orientation, sensitivity);
            break;
        case PLAYBACK_DBG_TYPE_A_ORIENT:
            MPL_LOGV("PLAYBACK_DBG_TYPE_A_ORIENT\n");
            r = fread(&orientation, sizeof(orientation), 1, inv_construct.file);
            r = fread(&sensitivity, sizeof(sensitivity), 1, inv_construct.file);
            inv_set_accel_orientation_and_scale(orientation, sensitivity);
            break;
        case PLAYBACK_DBG_TYPE_C_ORIENT:
            MPL_LOGV("PLAYBACK_DBG_TYPE_C_ORIENT\n");
            r = fread(&orientation, sizeof(orientation), 1, inv_construct.file);
            r = fread(&sensitivity, sizeof(sensitivity), 1, inv_construct.file);
            inv_set_compass_orientation_and_scale(orientation, sensitivity);
            break;

        case PLAYBACK_DBG_TYPE_G_SAMPLE_RATE:
            r = fread(&sample_rate_us, sizeof(sample_rate_us), 
                      1, inv_construct.file);
            inv_set_gyro_sample_rate(sample_rate_us);
            MPL_LOGV("PLAYBACK_DBG_TYPE_G_SAMPLE_RATE => %d\n",
                     sample_rate_us);
            break;
        case PLAYBACK_DBG_TYPE_A_SAMPLE_RATE:
            r = fread(&sample_rate_us, sizeof(sample_rate_us), 
                      1, inv_construct.file);
            inv_set_accel_sample_rate(sample_rate_us);
            MPL_LOGV("PLAYBACK_DBG_TYPE_A_SAMPLE_RATE => %d\n",
                     sample_rate_us);
            break;
        case PLAYBACK_DBG_TYPE_C_SAMPLE_RATE:
            r = fread(&sample_rate_us, sizeof(sample_rate_us), 
                      1, inv_construct.file);
            inv_set_compass_sample_rate(sample_rate_us);
            MPL_LOGV("PLAYBACK_DBG_TYPE_C_SAMPLE_RATE => %d\n",
                     sample_rate_us);
            break;

        case PLAYBACK_DBG_TYPE_GYRO_OFF:
            MPL_LOGV("PLAYBACK_DBG_TYPE_GYRO_OFF\n");
            inv_gyro_was_turned_off();
            break;
        case PLAYBACK_DBG_TYPE_ACCEL_OFF:
            MPL_LOGV("PLAYBACK_DBG_TYPE_ACCEL_OFF\n");
            inv_accel_was_turned_off();
            break;
        case PLAYBACK_DBG_TYPE_COMPASS_OFF:
            MPL_LOGV("PLAYBACK_DBG_TYPE_COMPASS_OFF\n");
            inv_compass_was_turned_off();
            break;
        case PLAYBACK_DBG_TYPE_QUAT_OFF:
            MPL_LOGV("PLAYBACK_DBG_TYPE_QUAT_OFF\n");
            inv_quaternion_sensor_was_turned_off();
            break;

        case PLAYBACK_DBG_TYPE_Q_SAMPLE_RATE:
            MPL_LOGV("PLAYBACK_DBG_TYPE_Q_SAMPLE_RATE\n");
            r = fread(&sample_rate_us, sizeof(sample_rate_us), 
                      1, inv_construct.file);
            inv_set_quat_sample_rate(sample_rate_us);
            break;
        default:
            //MPL_LOGV("PLAYBACK file closed\n");
            fclose(inv_construct.file);
            MPL_LOGE("%s|%s|%d error: unrecognized log type '%d', "
                     "PLAYBACK file closed\n",
                     __FILE__, __func__, __LINE__, type);
            return INV_ERROR;
        }
    }
    msleep(1);

    inv_construct.debug_mode = RD_NO_DEBUG;
    fclose(inv_construct.file);

    return INV_SUCCESS;
}

/** Turns on/off playback and record modes
* @param mode Turn on recording mode with RD_RECORD and turn off recording mode with
*             RD_NO_DBG. Turn on playback mode with RD_PLAYBACK.
*/
void inv_set_debug_mode(rd_dbg_mode mode)
{
#ifdef INV_PLAYBACK_DBG
    inv_construct.debug_mode = mode;
#endif
}

inv_error_t inv_constructor_start(void)
{
    inv_error_t result;
    unsigned char divider;
    //int gest_enabled = inv_get_gesture_enable();

    // start the software
    result = inv_start_mpl();
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }
    
    /*
    if (inv_construct.dmp_version == WIN8_DMP_VERSION) {
        int fifo_divider;
        divider = 4; // 4 means 200Hz DMP
        fifo_divider = 3;
        // Set Gyro Sample Rate in MPL in micro seconds
        inv_set_gyro_sample_rate(1000L*(divider+1)*(fifo_divider+1));

        // Set Gyro Sample Rate in MPL in micro seconds
        inv_set_quat_sample_rate(1000L*(divider+1)*(fifo_divider+1));

        // Set Compass Sample Rate in MPL in micro seconds
        inv_set_compass_sample_rate(1000L*(divider+1)*(fifo_divider+1));

        // Set Accel Sample Rate in MPL in micro seconds
        inv_set_accel_sample_rate(1000L*(divider+1)*(fifo_divider+1));
    } else if (gest_enabled) {
        int fifo_divider;
        unsigned char mpl_divider;

        inv_send_interrupt_word();
        inv_send_sensor_data(INV_ALL & INV_GYRO_ACC_MASK);
        inv_send_quaternion();

        divider = fifo_dmp_cfg.sample_divider;
        mpl_divider = fifo_dmp_cfg.mpl_divider;

        // Set Gyro Sample Rate in MPL in micro seconds
        inv_set_gyro_sample_rate(1000L*(mpl_divider+1));

        // Set Gyro Sample Rate in MPL in micro seconds
        inv_set_quat_sample_rate(1000L*(mpl_divider+1));

        // Set Compass Sample Rate in MPL in micro seconds
        inv_set_compass_sample_rate(1000L*(mpl_divider+1));

        // Set Accel Sample Rate in MPL in micro seconds
        inv_set_accel_sample_rate(1000L*(mpl_divider+1));
    } else 
    */
    {
        divider = 9;
        // set gyro sample sate in MPL in micro seconds
        inv_set_gyro_sample_rate(1000L*(divider+1));
        // set compass sample rate in MPL in micro seconds
        inv_set_compass_sample_rate(1000L*(divider+1));
        // set accel sample rate in MPL in micro seconds
        inv_set_accel_sample_rate(1000L*(divider+1));
    }

    // setup the scale factors and orientations and other parameters
    result = inv_constructor_setup();

    return result;
}

inv_error_t inv_constructor_default_enable()
{
    INV_ERROR_CHECK(inv_enable_quaternion());
    INV_ERROR_CHECK(inv_enable_fast_nomot());
    INV_ERROR_CHECK(inv_enable_heading_from_gyro());
    INV_ERROR_CHECK(inv_enable_compass_bias_w_gyro());
    INV_ERROR_CHECK(inv_enable_hal_outputs());
    INV_ERROR_CHECK(inv_enable_vector_compass_cal());
    INV_ERROR_CHECK(inv_enable_9x_sensor_fusion());
    INV_ERROR_CHECK(inv_enable_gyro_tc());
    INV_ERROR_CHECK(inv_enable_no_gyro_fusion());
    INV_ERROR_CHECK(inv_enable_in_use_auto_calibration());
    INV_ERROR_CHECK(inv_enable_magnetic_disturbance());
    return INV_SUCCESS;
}

/**
 * @}
 */
