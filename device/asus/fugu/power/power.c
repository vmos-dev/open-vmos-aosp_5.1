/*
 * Copyright (c) 2014 Intel Corporation All Rights Reserved
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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

//#define LOG_NDEBUG 0
#define LOG_TAG "IntelPowerHAL"
#include <utils/Log.h>

#include <hardware/hardware.h>
#include <hardware/power.h>

#define BOOST_PULSE_SYSFS    "/sys/devices/system/cpu/cpufreq/interactive/boostpulse"
#define BOOST_FREQ_SYSFS     "/sys/devices/system/cpu/cpufreq/interactive/hispeed_freq"
#define BOOST_DURATION_SYSFS "/sys/devices/system/cpu/cpufreq/interactive/boostpulse_duration"

struct intel_power_module {
    struct power_module container;
    uint32_t pulse_duration;
    struct timespec last_boost_time; /* latest POWER_HINT_INTERACTION boost */
};

static ssize_t sysfs_write(char *path, char *s)
{
    char buf[80];
    ssize_t len;
    int fd = open(path, O_WRONLY);

    if (fd < 0) {
        strerror_r(errno, buf, sizeof(buf));
        ALOGE("Error opening %s: %s\n", path, buf);
        return -1;
    }

    if ((len = write(fd, s, strlen(s))) < 0) {
        strerror_r(errno, buf, sizeof(buf));
        ALOGE("Error writing to %s: %s\n", path, buf);
    }

    close(fd);
    ALOGV("wrote '%s' to %s", s, path);

    return len;
}

static ssize_t sysfs_read(char *path, char *s, int num_bytes)
{
    char buf[80];
    ssize_t count;
    int fd = open(path, O_RDONLY);

    if (fd < 0) {
        strerror_r(errno, buf, sizeof(buf));
        ALOGE("Error reading from %s: %s\n", path, buf);
        return -1;
    }

    if ((count = read(fd, s, (num_bytes - 1))) < 0) {
        strerror_r(errno, buf, sizeof(buf));
        ALOGE("Error reading from  %s: %s\n", path, buf);
    } else {
        if ((count >= 1) && (s[count-1] == '\n')) {
            s[count-1] = '\0';
        } else {
            s[count] = '\0';
        }
    }

    close(fd);
    ALOGV("read '%s' from %s", s, path);

    return count;
}

static void fugu_power_init(struct power_module *module)
{
    struct intel_power_module *mod = (struct intel_power_module *) module;
    char boost_freq[32];
    char boostpulse_duration[32];

    /* Keep default boost_freq for fugu => max freq */

    if (sysfs_read(BOOST_FREQ_SYSFS, boost_freq, 32) < 0) {
        strcpy(boost_freq, "?");
    }
    if (sysfs_read(BOOST_DURATION_SYSFS, boostpulse_duration, 32) < 0) {
        /* above should not fail but just in case it does use an arbitrary 20ms value */
        snprintf(boostpulse_duration, 32, "%d", 20000);
    }
    mod->pulse_duration = atoi(boostpulse_duration);
    /* initialize last_boost_time */
    clock_gettime(CLOCK_MONOTONIC, &mod->last_boost_time);

    ALOGI("init done: will boost CPU to %skHz for %dus on input events",
            boost_freq, mod->pulse_duration);
}

static void fugu_power_set_interactive(struct power_module *module, int on)
{
    struct dirent **device_list = NULL;

    ALOGI("setInteractive: on=%d", on);
    (void) module; /* unused */
    (void) on; /* unused */
}

static inline void timespec_sub(struct timespec *res, struct timespec *a, struct timespec *b)
{
    res->tv_sec = a->tv_sec - b->tv_sec;
    if (a->tv_sec >= b->tv_sec) {
        res->tv_nsec = a->tv_nsec - b->tv_nsec;
    } else {
        res->tv_nsec = 1000000000 - b->tv_nsec + a->tv_nsec;
        res->tv_sec--;
    }
}

static inline uint64_t timespec_to_us(struct timespec *t)
{
    return t->tv_sec * 1000000 + t->tv_nsec / 1000;
}

static void fugu_power_hint(struct power_module *module, power_hint_t hint, void *data)
{
    struct intel_power_module *mod = (struct intel_power_module *) module;
    char sysfs_val[80];
    struct timespec curr_time;
    struct timespec diff_time;
    uint64_t diff;

    (void) data;

    switch (hint) {
        case POWER_HINT_INTERACTION:
            clock_gettime(CLOCK_MONOTONIC, &curr_time);
            timespec_sub(&diff_time, &curr_time, &mod->last_boost_time);
            diff = timespec_to_us(&diff_time);

            ALOGV("POWER_HINT_INTERACTION: diff=%llu", diff);

            if (diff > mod->pulse_duration) {
                sysfs_write(BOOST_PULSE_SYSFS, "1");
                mod->last_boost_time = curr_time;
            }
            break;
        case POWER_HINT_VSYNC:
            break;
        default:
            break;
    }
}

static struct hw_module_methods_t power_module_methods = {
    .open = NULL,
};

struct intel_power_module HAL_MODULE_INFO_SYM = {
    container:{
        common: {
            tag: HARDWARE_MODULE_TAG,
            module_api_version: POWER_MODULE_API_VERSION_0_2,
            hal_api_version: HARDWARE_HAL_API_VERSION,
            id: POWER_HARDWARE_MODULE_ID,
            name: "Fugu Power HAL",
            author: "Intel",
            methods: &power_module_methods,
        },
        init: fugu_power_init,
        setInteractive: fugu_power_set_interactive,
        powerHint: fugu_power_hint,
    },
};
