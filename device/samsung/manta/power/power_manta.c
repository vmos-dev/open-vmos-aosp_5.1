/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <linux/time.h>
#include <stdbool.h>
//#define LOG_NDEBUG 0

#define LOG_TAG "MantaPowerHAL"
#include <utils/Log.h>

#include <hardware/hardware.h>
#include <hardware/power.h>

#define BOOSTPULSE_PATH "/sys/devices/system/cpu/cpufreq/interactive/boostpulse"
#define BOOST_PATH "/sys/devices/system/cpu/cpufreq/interactive/boost"
#define CPU_MAX_FREQ_PATH "/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq"
//BOOST_PULSE_DURATION and BOOT_PULSE_DURATION_STR should always be in sync
#define BOOST_PULSE_DURATION 1000000
#define BOOST_PULSE_DURATION_STR "1000000"
#define NSEC_PER_SEC 1000000000
#define USEC_PER_SEC 1000000
#define NSEC_PER_USEC 100
#define LOW_POWER_MAX_FREQ "800000"
#define NORMAL_MAX_FREQ "1700000"

struct manta_power_module {
    struct power_module base;
    pthread_mutex_t lock;
    int boostpulse_fd;
    int boostpulse_warned;
    const char *touchscreen_power_path;
};

static unsigned int vsync_count;
static struct timespec last_touch_boost;
static bool touch_boost;
static bool low_power_mode = false;

static void sysfs_write(const char *path, char *s)
{
    char buf[80];
    int len;
    int fd = open(path, O_WRONLY);

    if (fd < 0) {
        strerror_r(errno, buf, sizeof(buf));
        ALOGE("Error opening %s: %s\n", path, buf);
        return;
    }

    len = write(fd, s, strlen(s));
    if (len < 0) {
        strerror_r(errno, buf, sizeof(buf));
        ALOGE("Error writing to %s: %s\n", path, buf);
    }

    close(fd);
}

static void init_touchscreen_power_path(struct manta_power_module *manta)
{
    char buf[80];
    const char dir[] = "/sys/devices/platform/s3c2440-i2c.3/i2c-3/3-004a/input";
    const char filename[] = "enabled";
    DIR *d;
    struct dirent *de;
    char *path;
    int pathsize;

    d = opendir(dir);
    if (d == NULL) {
        strerror_r(errno, buf, sizeof(buf));
        ALOGE("Error opening directory %s: %s\n", dir, buf);
        return;
    }
    while ((de = readdir(d)) != NULL) {
        if (strncmp("input", de->d_name, 5) == 0) {
            pathsize = strlen(dir) + strlen(de->d_name) + sizeof(filename) + 2;
            path = malloc(pathsize);
            if (path == NULL) {
                strerror_r(errno, buf, sizeof(buf));
                ALOGE("Out of memory: %s\n", buf);
                return;
            }
            snprintf(path, pathsize, "%s/%s/%s", dir, de->d_name, filename);
            manta->touchscreen_power_path = path;
            goto done;
        }
    }
    ALOGE("Error failed to find input dir in %s\n", dir);
done:
    closedir(d);
}

static void power_init(struct power_module *module)
{
    struct manta_power_module *manta = (struct manta_power_module *) module;
    struct dirent **namelist;
    int n;

    sysfs_write("/sys/devices/system/cpu/cpufreq/interactive/timer_rate",
                "20000");
    sysfs_write("/sys/devices/system/cpu/cpufreq/interactive/timer_slack",
                "20000");
    sysfs_write("/sys/devices/system/cpu/cpufreq/interactive/min_sample_time",
                "40000");
    sysfs_write("/sys/devices/system/cpu/cpufreq/interactive/hispeed_freq",
                "1000000");
    sysfs_write("/sys/devices/system/cpu/cpufreq/interactive/go_hispeed_load",
                "99");
    sysfs_write("/sys/devices/system/cpu/cpufreq/interactive/target_loads", "70 1200000:70 1300000:75 1400000:80 1500000:99");
    sysfs_write("/sys/devices/system/cpu/cpufreq/interactive/above_hispeed_delay",
                "80000");
    sysfs_write("/sys/devices/system/cpu/cpufreq/interactive/boostpulse_duration",
                BOOST_PULSE_DURATION_STR);
    sysfs_write("/sys/devices/system/cpu/cpufreq/interactive/io_is_busy", "1");

    init_touchscreen_power_path(manta);
}

static void power_set_interactive(struct power_module *module, int on)
{
    struct manta_power_module *manta = (struct manta_power_module *) module;
    char buf[80];
    int ret;

    ALOGV("power_set_interactive: %d\n", on);

    /*
     * Lower maximum frequency when screen is off.  CPU 0 and 1 share a
     * cpufreq policy.
     */
    sysfs_write(CPU_MAX_FREQ_PATH,
                (!on || low_power_mode) ? LOW_POWER_MAX_FREQ : NORMAL_MAX_FREQ);

    sysfs_write(manta->touchscreen_power_path, on ? "Y" : "N");

    ALOGV("power_set_interactive: %d done\n", on);
}

static int boostpulse_open(struct manta_power_module *manta)
{
    char buf[80];

    pthread_mutex_lock(&manta->lock);

    if (manta->boostpulse_fd < 0) {
        manta->boostpulse_fd = open(BOOSTPULSE_PATH, O_WRONLY);

        if (manta->boostpulse_fd < 0) {
            if (!manta->boostpulse_warned) {
                strerror_r(errno, buf, sizeof(buf));
                ALOGE("Error opening %s: %s\n", BOOSTPULSE_PATH, buf);
                manta->boostpulse_warned = 1;
            }
        }
    }

    pthread_mutex_unlock(&manta->lock);
    return manta->boostpulse_fd;
}

static struct timespec timespec_diff(struct timespec lhs, struct timespec rhs)
{
    struct timespec result;
    if (rhs.tv_nsec > lhs.tv_nsec) {
        result.tv_sec = lhs.tv_sec - rhs.tv_sec - 1;
        result.tv_nsec = NSEC_PER_SEC + lhs.tv_nsec - rhs.tv_nsec;
    } else {
        result.tv_sec = lhs.tv_sec - rhs.tv_sec;
        result.tv_nsec = lhs.tv_nsec - rhs.tv_nsec;
    }
    return result;
}

static int check_boostpulse_on(struct timespec diff)
{
    long boost_ns = (BOOST_PULSE_DURATION * NSEC_PER_USEC) % NSEC_PER_SEC;
    long boost_s = BOOST_PULSE_DURATION / USEC_PER_SEC;

    if (diff.tv_sec == boost_s)
        return (diff.tv_nsec < boost_ns);
    return (diff.tv_sec < boost_s);
}

static void manta_power_hint(struct power_module *module, power_hint_t hint,
                             void *data)
{
    struct manta_power_module *manta = (struct manta_power_module *) module;
    struct timespec now, diff;
    char buf[80];
    int len;

    switch (hint) {
     case POWER_HINT_INTERACTION:
        if (boostpulse_open(manta) >= 0) {
            pthread_mutex_lock(&manta->lock);
            len = write(manta->boostpulse_fd, "1", 1);

            if (len < 0) {
                strerror_r(errno, buf, sizeof(buf));
                ALOGE("Error writing to %s: %s\n", BOOSTPULSE_PATH, buf);
            } else {
                clock_gettime(CLOCK_MONOTONIC, &last_touch_boost);
                touch_boost = true;
            }
            pthread_mutex_unlock(&manta->lock);
        }

        break;

     case POWER_HINT_VSYNC:
        pthread_mutex_lock(&manta->lock);
        if (data) {
            if (vsync_count < UINT_MAX)
                vsync_count++;
        } else {
            if (vsync_count)
                vsync_count--;
            if (vsync_count == 0 && touch_boost) {
                touch_boost = false;
                clock_gettime(CLOCK_MONOTONIC, &now);
                diff = timespec_diff(now, last_touch_boost);
                if (check_boostpulse_on(diff)) {
                    sysfs_write(BOOST_PATH, "0");
                }
            }
        }
        pthread_mutex_unlock(&manta->lock);
        break;

    case POWER_HINT_LOW_POWER:
        pthread_mutex_lock(&manta->lock);
        if (data)
            sysfs_write(CPU_MAX_FREQ_PATH, LOW_POWER_MAX_FREQ);
        else
            sysfs_write(CPU_MAX_FREQ_PATH, NORMAL_MAX_FREQ);
        low_power_mode = data;
        pthread_mutex_unlock(&manta->lock);
        break;
    default:
            break;
    }
}

static struct hw_module_methods_t power_module_methods = {
    .open = NULL,
};

struct manta_power_module HAL_MODULE_INFO_SYM = {
    .base = {
        .common = {
            .tag = HARDWARE_MODULE_TAG,
            .module_api_version = POWER_MODULE_API_VERSION_0_2,
            .hal_api_version = HARDWARE_HAL_API_VERSION,
            .id = POWER_HARDWARE_MODULE_ID,
            .name = "Manta Power HAL",
            .author = "The Android Open Source Project",
            .methods = &power_module_methods,
        },

        .init = power_init,
        .setInteractive = power_set_interactive,
        .powerHint = manta_power_hint,
    },

    .lock = PTHREAD_MUTEX_INITIALIZER,
    .boostpulse_fd = -1,
    .boostpulse_warned = 0,
};

