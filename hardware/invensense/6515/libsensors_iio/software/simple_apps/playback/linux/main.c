/*******************************************************************************
 * Copyright (c) 2012 InvenSense Corporation, All Rights Reserved.
 ******************************************************************************/

/*******************************************************************************
 *
 * $Id: main.c 6146 2011-10-04 18:33:51Z jcalizo $
 *
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "invensense.h"
#include "invensense_adv.h"
#include "and_constructor.h"
#include "ml_math_func.h"
#include "datalogger_outputs.h"

#include "console_helper.h"

#include "mlos.h"
#include "mlsl.h"

#include "testsupport.h"

#include "log.h"
#undef MPL_LOG_TAG
#define MPL_LOG_TAG "MPL-playback"

/*
    Defines & Macros
*/
#define UNPACK_3ELM_ARRAY(a)    (a)[0], (a)[1], (a)[2]
#define UNPACK_4ELM_ARRAY(a)    UNPACK_3ELM_ARRAY(a), (a)[3]
#define COMPONENT_NAME_MAX_LEN  (30)
#define DEF_NAME(x)             (#x)

#define PRINT_ON_CONSOLE(...)                   \
    if (print_on_screen)                        \
        printf(__VA_ARGS__)
#define PRINT_ON_FILE(...)                      \
    if(stream_file)                             \
        fprintf(stream_file, __VA_ARGS__)

#define PRINT(...)                              \
    PRINT_ON_CONSOLE(__VA_ARGS__);              \
    PRINT_ON_FILE(__VA_ARGS__)
#define PRINT_FLOAT(width, prec, data)          \
    PRINT_ON_CONSOLE("%+*.*f",                  \
                     width, prec, data);        \
    PRINT_ON_FILE("%+f", data)
#define PRINT_INT(width, data)                  \
    PRINT_ON_CONSOLE("%+*d", width, data);      \
    PRINT_ON_FILE("%+d", data);
#define PRINT_LONG(width, data)                 \
    PRINT_ON_CONSOLE("%+*ld", width, data);     \
    PRINT_ON_FILE("%+ld", data);

#define PRINT_3ELM_ARRAY_FLOAT(w, p, data)      \
    PRINT_FLOAT(w, p, data[0]);                 \
    PRINT(", ");                                \
    PRINT_FLOAT(w, p, data[1]);                 \
    PRINT(", ");                                \
    PRINT_FLOAT(w, p, data[2]);                 \
    PRINT(", ");
#define PRINT_4ELM_ARRAY_FLOAT(w, p, data)      \
    PRINT_3ELM_ARRAY_FLOAT(w, p, data);         \
    PRINT_FLOAT(w, p, data[3]);                 \
    PRINT(", ");

#define PRINT_3ELM_ARRAY_LONG(w, data)          \
    PRINT_LONG(w, data[0]);                     \
    PRINT(", ");                                \
    PRINT_LONG(w, data[1]);                     \
    PRINT(", ");                                \
    PRINT_LONG(w, data[2]);                     \
    PRINT(", ");
#define PRINT_4ELM_ARRAY_LONG(w, data)          \
    PRINT_3ELM_ARRAY_LONG(w, data);             \
    PRINT_LONG(w, data[3]);                     \
    PRINT(", ");

#define PRINT_3ELM_ARRAY_INT(w, data)           \
    PRINT_INT(w, data[0]);                      \
    PRINT(", ");                                \
    PRINT_INT(w, data[1]);                      \
    PRINT(", ");                                \
    PRINT_INT(w, data[2]);                      \
    PRINT(", ");
#define PRINT_4ELM_ARRAY_INT(w, data)           \
    PRINT_3ELM_ARRAY_LONG(w, data);             \
    PRINT_INT(w, data[3]);                      \
    PRINT(", ");


#define CASE_NAME(CODE)                         \
    case CODE:                                  \
        return #CODE

#define CALL_CHECK_N_PRINT(f) {                                     \
    MPL_LOGI("\n");                                                 \
    MPL_LOGI("################################################\n"); \
    MPL_LOGI("# %s\n", #f);                                         \
    MPL_LOGI("################################################\n"); \
    MPL_LOGI("\n");                                                 \
    CALL_N_CHECK(f);                                                \
}

/*
    Types
*/
/* A badly named enum type to track state of user input for tracker menu. */
typedef enum {
    STATE_SELECT_A_TRACKER,
    STATE_SET_TRACKER_STATE,    /* I'm running out of ideas here. */
    STATE_COUNT
} user_state_t;

/* bias trackers. */
typedef enum {
    BIAS_FROM_NO_MOTION,
    FAST_NO_MOTION,
    BIAS_FROM_GRAVITY,
    BIAS_FROM_TEMPERATURE,
    BIAS_FROM_LPF,
    DEAD_ZONE,
    NUM_TRACKERS
} bias_t;

enum comp_ids {
    TIME = 0,
    CALIBRATED_GYROSCOPE,
    CALIBRATED_ACCELEROMETER,
    CALIBRATED_COMPASS,
    RAW_GYROSCOPE,
    RAW_GYROSCOPE_BODY,
    RAW_ACCELEROMETER,
    RAW_COMPASS,
    QUATERNION_9_AXIS,
    QUATERNION_6_AXIS,
    GRAVITY,
    HEADING,
    COMPASS_BIAS_ERROR,
    COMPASS_STATE,
    TEMPERATURE,
    TEMP_COMP_SLOPE,
    LINEAR_ACCELERATION,
    ROTATION_VECTOR,
    MOTION_STATE,

    NUM_OF_IDS
};

struct component_list {
    char name[COMPONENT_NAME_MAX_LEN];
    int order;
};

/*
    Globals
*/
static int print_on_screen = true;
static int one_time_print = true;
static FILE *stream_file = NULL;
static unsigned long sample_count = 0;
static int enabled_9x = true;

signed char g_gyro_orientation[9] = {1, 0, 0, 0, 1, 0, 0, 0, 1};
signed char g_accel_orientation[9] = {1, 0, 0, 0, 1, 0, 0, 0, 1};
signed char g_compass_orientation[9] = {-1, 0, 0, 0, 1, 0, 0, 0, -1};

#ifdef WIN32
static double pc_freq;
static __int64 counter_start;
#else
static inv_time_t counter_start;
#endif

struct component_list components[NUM_OF_IDS];

/*
    Prototypes
*/
void print_tracker_states(bias_t tracker);

/*
    Callbacks
*/
/*--- motion / no motion callback function ---*/
void check_motion_event(void)
{
    long msg = inv_get_message_level_0(1);
    if (msg) {
        if (msg & INV_MSG_MOTION_EVENT) {
            MPL_LOGI("################################################\n");
            MPL_LOGI("## Motion\n");
            MPL_LOGI("################################################\n");
        }
        if (msg & INV_MSG_NO_MOTION_EVENT) {
            MPL_LOGI("################################################\n");
            MPL_LOGI("## No Motion\n");
            MPL_LOGI("################################################\n");
        }
    }
}

/* number to string coversion */
char *compass_state_name(char* out, int state)
{
    switch(state) {
        CASE_NAME(SF_NORMAL);
        CASE_NAME(SF_DISTURBANCE);
        CASE_NAME(SF_FAST_SETTLE);
        CASE_NAME(SF_SLOW_SETTLE);
        CASE_NAME(SF_STARTUP_SETTLE);
        CASE_NAME(SF_UNCALIBRATED);
    }

    #define UNKNOWN_ERROR_CODE 1234
    return ERROR_NAME(UNKNOWN_ERROR_CODE);
}

/* component ID to name convertion */
char *component_name(char *out, int comp_id)
{
    switch (comp_id) {
    CASE_NAME(TIME);
    CASE_NAME(CALIBRATED_GYROSCOPE);
    CASE_NAME(CALIBRATED_ACCELEROMETER);
    CASE_NAME(CALIBRATED_COMPASS);
    CASE_NAME(RAW_GYROSCOPE);
    CASE_NAME(RAW_GYROSCOPE_BODY);
    CASE_NAME(RAW_ACCELEROMETER);
    CASE_NAME(RAW_COMPASS);
    CASE_NAME(QUATERNION_9_AXIS);
    CASE_NAME(QUATERNION_6_AXIS);
    CASE_NAME(GRAVITY);
    CASE_NAME(HEADING);
    CASE_NAME(COMPASS_BIAS_ERROR);
    CASE_NAME(COMPASS_STATE);
    CASE_NAME(TEMPERATURE);
    CASE_NAME(TEMP_COMP_SLOPE);
    CASE_NAME(LINEAR_ACCELERATION);
    CASE_NAME(ROTATION_VECTOR);
    CASE_NAME(MOTION_STATE);
    }

    return "UNKNOWN";
}


#ifdef WIN32

/*
  Karthik Implementation.
  http://stackoverflow.com/questions/1739259/how-to-use-queryperformancecounter
*/
double get_counter(__int64 *counter_start, double *pc_freq)
{
    LARGE_INTEGER li;
    double x;
    QueryPerformanceCounter(&li);
    x = (double) (li.QuadPart - (*counter_start));
    x = x / (*pc_freq);
    return(x);
}

void start_counter(double *pc_freq, __int64 *counter_start)
{
    LARGE_INTEGER li;
    double x;
    if(!QueryPerformanceFrequency(&li))
        printf("QueryPerformanceFrequency failed!\n");
    x = (double)(li.QuadPart);
    *pc_freq = x / 1000.0;
    QueryPerformanceCounter(&li);
    *counter_start = li.QuadPart;
}

#else

unsigned long get_counter(void)
{
    return (inv_get_tick_count() - counter_start);
}

void start_counter(void)
{
    counter_start = inv_get_tick_count();
}

#endif

/* processed data callback */
void fifo_callback(void)
{
    int print_on_screen_saved = print_on_screen;
    int i;

    /* one_time_print causes the data labels to be printed on screen */
    if (one_time_print) {
        print_on_screen = true;
    }
    for (i = 0; i < NUM_OF_IDS; i++) {
        if (components[TIME].order == i) {
            if (one_time_print) {
                PRINT("TIME,");
            } else {
#ifdef WIN32
                double time_ms;
                static int first_value = 0;
                if(first_value == 0){
                    first_value = 1;
                    start_counter(&pc_freq, &counter_start);
                    time_ms = 0;
                } else {
                    time_ms = get_counter(&counter_start, &pc_freq);
                }
                PRINT("%6.0f,   ", time_ms);
#else
                unsigned long time_ms;
                static int first_value = 0;
                if(first_value == 0){
                    first_value = 1;
                    start_counter();
                    time_ms = 0;
                } else {
                    time_ms = get_counter();
                }
                PRINT("%6ld,   ", time_ms);
#endif
            }
        } else if (components[CALIBRATED_GYROSCOPE].order == i) {
            if (one_time_print) {
                PRINT("CALIBRATED_GYROSCOPE_X,"
                      "CALIBRATED_GYROSCOPE_Y,"
                      "CALIBRATED_GYROSCOPE_Z,");
                /*
                PRINT("CALIBRATED_GYROSCOPE_X_AVERAGE,"
                      "CALIBRATED_GYROSCOPE_Y_AVERAGE,"
                      "CALIBRATED_GYROSCOPE_Z_AVERAGE,");
                */
            } else {
                /*
                #define window 20
                static int cnt = 0;
                static int valid = 0;
                static float gyro_keep[window][3];
                int kk, jj;
                float avg[3];
                */
                float gyro[3];
                inv_get_gyro_float(gyro);
                PRINT_3ELM_ARRAY_FLOAT(10, 5, gyro);
                PRINT("  ");
                /*
                memcpy(gyro_keep[cnt], gyro, sizeof(float) * 3);
                cnt= (cnt + 1) % window;
                if (cnt == window - 1)
                    valid = 1;
                if (valid) {
                    memset(avg, 0, sizeof(float) * 3);
                    jj = (cnt + 1) % window;
                    for (kk = 0; kk < window; kk++) {
                        avg[0] += gyro_keep[jj][0] / window;
                        avg[1] += gyro_keep[jj][1] / window;
                        avg[2] += gyro_keep[jj][2] / window;
                        jj = (jj + 1) % window;
                    }
                    PRINT("%+f, %+f, %+f,   ",
                          UNPACK_3ELM_ARRAY(avg));
                    PRINT_3ELM_ARRAY_FLOAT(10, 5, avg);
                    PRINT("  ");
                }
                */
            }
        } else if (components[CALIBRATED_ACCELEROMETER].order == i) {
            if (one_time_print) {
                PRINT("CALIBRATED_ACCELEROMETER_X,"
                      "CALIBRATED_ACCELEROMETER_Y,"
                      "CALIBRATED_ACCELEROMETER_Z,");
            } else {
                float accel[3];
                inv_get_accel_float(accel);
                PRINT_3ELM_ARRAY_FLOAT(10, 5, accel);
                PRINT("  ");
            }
        } else if (components[CALIBRATED_COMPASS].order == i) {
            if (one_time_print) {
                PRINT("CALIBRATED_COMPASS_X,"
                      "CALIBRATED_COMPASS_Y,"
                      "CALIBRATED_COMPASS_Z,");
            } else {
                long lcompass[3];
                float compass[3];
                inv_get_compass_set(lcompass, 0, 0);
                compass[0] = inv_q16_to_float(lcompass[0]);
                compass[1] = inv_q16_to_float(lcompass[1]);
                compass[2] = inv_q16_to_float(lcompass[2]);
                PRINT_3ELM_ARRAY_FLOAT(10, 5, compass);
                PRINT("  ");
            }
        } else if (components[RAW_GYROSCOPE].order == i) {
            if (one_time_print) {
                PRINT("RAW_GYROSCOPE_X,"
                      "RAW_GYROSCOPE_Y,"
                      "RAW_GYROSCOPE_Z,");
            } else {
                short raw[3];
                inv_get_sensor_type_gyro_raw_short(raw, NULL);
                PRINT_3ELM_ARRAY_INT(6, raw);
                PRINT("  ");
            }
        } else if (components[RAW_GYROSCOPE_BODY].order == i) {
            if (one_time_print) {
                PRINT("RAW_GYROSCOPE_BODY_X,"
                      "RAW_GYROSCOPE_BODY_Y,"
                      "RAW_GYROSCOPE_BODY_Z,");
            } else {
                float raw_body[3];
                inv_get_sensor_type_gyro_raw_body_float(raw_body, NULL);
                PRINT_3ELM_ARRAY_FLOAT(10, 5, raw_body);
                PRINT("  ");
            }
        } else if (components[RAW_ACCELEROMETER].order == i) {
            if (one_time_print) {
                PRINT("RAW_ACCELEROMETER_X,"
                      "RAW_ACCELEROMETER_Y,"
                      "RAW_ACCELEROMETER_Z,");
            } else {
                short raw[3];
                inv_get_sensor_type_accel_raw_short(raw, NULL);
                PRINT_3ELM_ARRAY_INT(6, raw);
                PRINT("  ");
            }
        } else if (components[RAW_COMPASS].order == i) {
            if (one_time_print) {
                PRINT("RAW_COMPASS_X,"
                      "RAW_COMPASS_Y,"
                      "RAW_COMPASS_Z,");
            } else {
                short raw[3];
                inv_get_sensor_type_compass_raw_short(raw, NULL);
                PRINT_3ELM_ARRAY_INT(6, raw);
                PRINT("  ");
            }
        } else if (components[QUATERNION_9_AXIS].order == i) {
            if (one_time_print) {
                PRINT("QUATERNION_9_AXIS_X,"
                      "QUATERNION_9_AXIS_Y,"
                      "QUATERNION_9_AXIS_Z,"
                      "QUATERNION_9_AXIS_w,");
            } else {
                float quat[4];
                inv_get_quaternion_float(quat);
                PRINT_4ELM_ARRAY_FLOAT(10, 5, quat);
                PRINT("  ");
            }
        } else if (components[QUATERNION_6_AXIS].order == i) {
            if (one_time_print) {
                PRINT("QUATERNION_6_AXIS_X,"
                      "QUATERNION_6_AXIS_Y,"
                      "QUATERNION_6_AXIS_Z,"
                      "QUATERNION_6_AXIS_w,");
            } else {
                float quat[4];
                long temp[4];
                int j;
                inv_time_t timestamp;
                inv_get_6axis_quaternion(temp, &timestamp);
                for (j = 0; j < 4; j++)
                    quat[j] = (float)temp[j] / (1 << 30);
                PRINT_4ELM_ARRAY_FLOAT(10, 5, quat);
                PRINT("  ");
            }
        } else if (components[HEADING].order == i) {
            if (one_time_print) {
                PRINT("HEADING,");
            } else {
                float heading[1];
                inv_get_sensor_type_compass_float(heading, NULL, NULL,
                                                  NULL, NULL);
                PRINT_FLOAT(10, 5, heading[0]);
                PRINT(",   ");
            }
        } else if (components[GRAVITY].order == i) {
            if (one_time_print) {
                PRINT("GRAVITY_X,"
                      "GRAVITY_Y,"
                      "GRAVITY_Z,");
            } else {
                float gravity[3];
                inv_get_sensor_type_gravity_float(gravity, NULL, NULL);
                PRINT_3ELM_ARRAY_FLOAT(10, 5, gravity);
                PRINT("  ");
            }
        } else if (components[COMPASS_STATE].order == i) {
            if (one_time_print) {
                PRINT("COMPASS_STATE,"
                      "GOT_COARSE_HEADING,");
            } else {
                PRINT_INT(1, inv_get_compass_state());
                PRINT(", ");
                PRINT_INT(1, inv_got_compass_bias());
                PRINT(", ");
            }
        } else if (components[COMPASS_BIAS_ERROR].order == i) {
            if (one_time_print) {
                PRINT("COMPASS_BIAS_ERROR_X,"
                      "COMPASS_BIAS_ERROR_Y,"
                      "COMPASS_BIAS_ERROR_Z,");
            } else {
                long mbe[3];
                inv_get_compass_bias_error(mbe);
                PRINT_3ELM_ARRAY_LONG(6, mbe);
            }
        } else if (components[TEMPERATURE].order == i) {
            if (one_time_print) {
                PRINT("TEMPERATURE,");
            } else {
                float temp;
                inv_get_sensor_type_temperature_float(&temp, NULL);
                PRINT_FLOAT(8, 4, temp);
                PRINT(",   ");
            }
        } else if (components[TEMP_COMP_SLOPE].order == i) {
            if (one_time_print) {
                PRINT("TEMP_COMP_SLOPE_X,"
                      "TEMP_COMP_SLOPE_Y,"
                      "TEMP_COMP_SLOPE_Z,");
            } else {
                long temp_slope[3];
                float temp_slope_f[3];
                (void)inv_get_gyro_ts(temp_slope);
                temp_slope_f[0] = inv_q16_to_float(temp_slope[0]);
                temp_slope_f[1] = inv_q16_to_float(temp_slope[1]);
                temp_slope_f[2] = inv_q16_to_float(temp_slope[2]);
                PRINT_3ELM_ARRAY_FLOAT(10, 5, temp_slope_f);
                PRINT("  ");
            }
        } else if (components[LINEAR_ACCELERATION].order == i) {
            if (one_time_print) {
                PRINT("LINEAR_ACCELERATION_X,"
                      "LINEAR_ACCELERATION_Y,"
                      "LINEAR_ACCELERATION_Z,");
            } else {
                float lin_acc[3];
                inv_get_linear_accel_float(lin_acc);
                PRINT_3ELM_ARRAY_FLOAT(10, 5, lin_acc);
                PRINT("  ");
            }
        } else if (components[ROTATION_VECTOR].order == i) {
            if (one_time_print) {
                PRINT("ROTATION_VECTOR_X,"
                      "ROTATION_VECTOR_Y,"
                      "ROTATION_VECTOR_Z,");
            } else {
                float rot_vec[3];
                inv_get_sensor_type_rotation_vector_float(rot_vec, NULL, NULL);
                PRINT_3ELM_ARRAY_FLOAT(10, 5, rot_vec);
                PRINT("  ");
            }
        } else if (components[MOTION_STATE].order == i) {
            if (one_time_print) {
                PRINT("MOTION_STATE,");
            } else {
                unsigned int counter;
                PRINT_INT(1, inv_get_motion_state(&counter));
                PRINT(",   ");
            }
        } else {
            ;
        }
    }
    PRINT("\n");

    if (one_time_print) {
        one_time_print = false;
        print_on_screen = print_on_screen_saved;
    }
    sample_count++;
}

void print_help(char *exename)
{
    printf(
        "\n"
        "Usage:\n"
        "\t%s [options]\n"
        "\n"
        "Options:\n"
        "        [-h|--help]          = shows this help\n"
        "        [-o|--output PREFIX] = to dump data on csv file whose file\n"
        "                               prefix is specified by the parameter,\n"
        "                               e.g. '<PREFIX>-<timestamp>.csv'\n"
        "        [-i|--input NAME]    = to read the provided playback.bin file\n"
        "        [-c|--comp C]        = enable the following components in the\n"
        "                               given order:\n"
        "                                 t = TIME\n"
        "                                 T = TEMPERATURE,\n"
        "                                 s = TEMP_COMP_SLOPE,\n"
        "                                 G = CALIBRATED_GYROSCOPE,\n"
        "                                 A = CALIBRATED_ACCELEROMETER,\n"
        "                                 C = CALIBRATED_COMPASS,\n"
        "                                 g = RAW_GYROSCOPE,\n"
        "                                 b = RAW_GYROSCOPE_BODY,\n"
        "                                 a = RAW_ACCELEROMETER,\n"
        "                                 c = RAW_COMPASS,\n"
        "                                 Q = QUATERNION_9_AXIS,\n"
        "                                 6 = QUATERNION_6_AXIS,\n"
        "                                 V = GRAVITY,\n"
        "                                 L = LINEAR_ACCELERATION,\n"
        "                                 R = ROTATION_VECTOR,\n"
        "                                 H = HEADING,\n"
        "                                 E = COMPASS_BIAS_ERROR,\n"
        "                                 S = COMPASS_STATE,\n"
        "                                 M = MOTION_STATE.\n"
        "\n"
        "Note on compass state values:\n"
        "    SF_NORMAL         = 0\n"
        "    SF_DISTURBANCE    = 1\n"
        "    SF_FAST_SETTLE    = 2\n"
        "    SF_SLOW_SETTLE    = 3\n"
        "    SF_STARTUP_SETTLE = 4\n"
        "    SF_UNCALIBRATED   = 5\n"
        "\n",
        exename);
}

char *output_filename_datetimestamp(char *out)
{
    time_t t;
    struct tm *now;
    t = time(NULL);
    now = localtime(&t);

    sprintf(out,
            "%02d%02d%02d_%02d%02d%02d.csv",
            now->tm_year - 100, now->tm_mon + 1, now->tm_mday,
            now->tm_hour, now->tm_min, now->tm_sec);
    return out;
}

int components_parser(char pname[], char requested[], int length)
{
    int j;

    /* forcibly disable all */
    for (j = 0; j < NUM_OF_IDS; j++)
        components[j].order = -1;

    /* parse each character one a time */
    for (j = 0; j < length; j++) {
        switch (requested[j]) {
        case 'T':
            components[TEMPERATURE].order = j;
            break;
        case 'G':
            components[CALIBRATED_GYROSCOPE].order = j;
            break;
        case 'A':
            components[CALIBRATED_ACCELEROMETER].order = j;
            break;
        case 'g':
            components[RAW_GYROSCOPE].order = j;
            break;
        case 'b':
            components[RAW_GYROSCOPE_BODY].order = j;
            break;
        case 'a':
            components[RAW_ACCELEROMETER].order = j;
            break;
        case 'Q':
            components[QUATERNION_9_AXIS].order = j;
            break;
        case '6':
            components[QUATERNION_6_AXIS].order = j;
            break;
        case 'V':
            components[GRAVITY].order = j;
            break;
        case 'L':
            components[LINEAR_ACCELERATION].order = j;
            break;
        case 'R':
            components[ROTATION_VECTOR].order = j;
            break;

        /* these components don't need to be enabled */
        case 't':
            components[TIME].order = j;
            break;
        case 'C':
            components[CALIBRATED_COMPASS].order = j;
            break;
        case 'c':
            components[RAW_COMPASS].order = j;
            break;
        case 'H':
            components[HEADING].order = j;
            break;
        case 'E':
            components[COMPASS_BIAS_ERROR].order = j;
            break;
        case 'S':
            components[COMPASS_STATE].order = j;
            break;
        case 'M':
            components[MOTION_STATE].order = j;
            break;

        default:
            MPL_LOGI("Error : unrecognized component '%c'\n",
                     requested[j]);
            print_help(pname);
            return 1;
            break;
        }
    }
    return 0;
}

int main(int argc, char *argv[])
{
#ifndef INV_PLAYBACK_DBG
    MPL_LOGI("Error : this application was not compiled with the "
             "INV_PLAYBACK_DBG define and cannot work.\n");
    MPL_LOGI("        Recompile the libmllite libraries with #define "
             "INV_PLAYBACK_DBG uncommented\n");
    MPL_LOGI("        in 'software/core/mllite/data_builder.h'.\n");
    return INV_ERROR;
#endif

    inv_time_t start_time;
    double total_time;
    char req_component_list[50] = "tQGACH";
    char input_filename[101] = "/data/playback.bin";
    int i = 0;
    char *ver_str;
    /* flags */
    int use_nm_detection = true;

    /* make sure there is no buffering of the print messages */
    setvbuf(stdout, NULL, _IONBF, 0);

    /* forcibly disable all and populate the names */
    for (i = 0; i < NUM_OF_IDS; i++) {
        char str_tmp[COMPONENT_NAME_MAX_LEN];
        strcpy(components[i].name, component_name(str_tmp, i));
        components[i].order = -1;
    }

    CALL_N_CHECK( inv_get_version(&ver_str) );
    MPL_LOGI("\n");
    MPL_LOGI("%s\n", ver_str);
    MPL_LOGI("\n");

    for (i = 1; i < argc; i++) {
        if(strcmp(argv[i], "-h") == 0
            || strcmp(argv[i], "--help") == 0) {
            print_help(argv[0]);
            return INV_SUCCESS;

        } else if(strcmp(argv[i], "-o") == 0
            || strcmp(argv[i], "--output") == 0) {
            char output_filename[200];
            char end[50] = "";
            i++;

            snprintf(output_filename, sizeof(output_filename), "%s-%s",
                    argv[i], output_filename_datetimestamp(end));
            stream_file = fopen(output_filename, "w+");
            if (!stream_file) {
                printf("Unable to open file '%s'\n", output_filename);
                return INV_ERROR;
            }
            MPL_LOGI("-- Output on file '%s'\n", output_filename);

        } else if(strcmp(argv[i], "-i") == 0
            || strcmp(argv[i], "--input") == 0) {
            i++;
            strncpy(input_filename, argv[i], sizeof(input_filename));
            MPL_LOGI("-- Playing back file '%s'\n", input_filename);

        } else if(strcmp(argv[i], "-n") == 0
            || strcmp(argv[i], "--nm") == 0) {
            i++;
            if (!strcmp(argv[i], "none")) {
                use_nm_detection = 0;
            } else if (!strcmp(argv[i], "regular")) {
                use_nm_detection = 1;
            } else if (!strcmp(argv[i], "fast")) {
                use_nm_detection = 2;
            } else {
                MPL_LOGI("Error : unrecognized no-motion type '%s'\n",
                         argv[i]);
                return INV_ERROR_INVALID_PARAMETER;
            }
            MPL_LOGI("-- No motion algorithm : '%s', %d\n",
                     argv[i], use_nm_detection);

        } else if(strcmp(argv[i], "-9") == 0
            || strcmp(argv[i], "--nine") == 0) {
            MPL_LOGI("-- using 9 axis sensor fusion by default\n");
            enabled_9x = true;

        } else if(strcmp(argv[i], "-c") == 0
            || strcmp(argv[i], "--comp") == 0) {
            i++;
            strcpy(req_component_list, argv[i]);

        } else {
            MPL_LOGI("Unrecognized command-line parameter '%s'\n", argv[i]);
            return INV_ERROR_INVALID_PARAMETER;
        }
    }

    CALL_CHECK_N_RETURN_ERROR(
        components_parser(
            argv[0],
            req_component_list, strlen(req_component_list)));

    /* set up callbacks */
    CALL_N_CHECK(inv_set_fifo_processed_callback(fifo_callback));

    /* algorithm init */
    CALL_N_CHECK(inv_enable_quaternion());
    if (use_nm_detection == 1) {
        CALL_N_CHECK(inv_enable_motion_no_motion());
    } else if (use_nm_detection == 2) {
        CALL_N_CHECK(inv_enable_fast_nomot());
    }
    CALL_N_CHECK(inv_enable_gyro_tc());
    CALL_N_CHECK(inv_enable_in_use_auto_calibration());
    CALL_N_CHECK(inv_enable_no_gyro_fusion());
    CALL_N_CHECK(inv_enable_results_holder());
    if (enabled_9x) {
        CALL_N_CHECK(inv_enable_heading_from_gyro());
        CALL_N_CHECK(inv_enable_compass_bias_w_gyro());
        CALL_N_CHECK(inv_enable_vector_compass_cal());
        CALL_N_CHECK(inv_enable_9x_sensor_fusion());
    }

    CALL_CHECK_N_RETURN_ERROR(inv_enable_datalogger_outputs());
    CALL_CHECK_N_RETURN_ERROR(inv_constructor_start());

    /* load persistent data */
    {
        FILE *fc = fopen("invcal.bin", "rb");
        if (fc != NULL) {
            size_t sz, rd;
            inv_error_t result = 0;
            // Read amount of memory we need to hold data
            rd = fread(&sz, sizeof(size_t), 1, fc);
            if (rd == sizeof(size_t)) {
                unsigned char *cal_data = (unsigned char *)malloc(sizeof(sz));
                unsigned char *cal_ptr;
                cal_ptr = cal_data;
                *(size_t *)cal_ptr = sz;
                cal_ptr += sizeof(sz);
                /* read rest of the file */
                fread(cal_ptr, sz - sizeof(size_t), 1, fc);
                //result = inv_load_mpl_states(cal_data, sz);
                if (result) {
                    MPL_LOGE("Cal Load error\n");
                }
                MPL_LOGI("inv_load_mpl_states()\n");
                free(cal_data);
            } else {
                MPL_LOGE("Cal Load error with size read\n");
            }
            fclose(fc);
        }
    }

    sample_count = 0;
    start_time = inv_get_tick_count();

    /* playback data that was recorded */
    inv_set_playback_filename(input_filename, strlen(input_filename) + 1);
    inv_set_debug_mode(RD_PLAYBACK);
    CALL_N_CHECK(inv_playback());

    total_time = (1.0 * inv_get_tick_count() - start_time) / 1000;
    if (total_time > 0) {
        MPL_LOGI("\nPlayed back %ld samples in %.2f s (%.1f Hz)\n",
                 sample_count, total_time , 1.0 * sample_count / total_time);
    }

    if (stream_file)
        fclose(stream_file);

    return INV_SUCCESS;
}


