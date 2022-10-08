/*
 * Copyright (C) 2014 The Android Open Source Project
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

#define LOG_TAG "healthd-flounder"
#include <healthd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cutils/klog.h>
#include <sys/types.h>
#include <sys/sysinfo.h>

/* Nominal voltage for ENERGY_COUNTER computation */
#define VOLTAGE_NOMINAL 3.7

#define POWER_SUPPLY_SUBSYSTEM "power_supply"
#define POWER_SUPPLY_SYSFS_PATH "/sys/class/" POWER_SUPPLY_SUBSYSTEM

#define MAX17050_PATH POWER_SUPPLY_SYSFS_PATH "/battery"
#define CHARGE_COUNTER_EXT_PATH MAX17050_PATH "/charge_counter_ext"

#define PALMAS_VOLTAGE_MONITOR_PATH POWER_SUPPLY_SYSFS_PATH "/palmas_voltage_monitor"
#define VOLTAGE_MONITOR_PATH PALMAS_VOLTAGE_MONITOR_PATH "/device/voltage_monitor"

#define BATTERY_FULL 100
#define BATTERY_LOW 15
#define BATTERY_CRITICAL_LOW_MV (3000)
#define BATTERY_DEAD_MV (2800)
#define NORMAL_MAX_SOC_DEC (2)
#define CRITICAL_LOW_FORCE_SOC_DROP (6)
#define UPDATE_PERIOD_MINIMUM_S (55)

using namespace android;
static bool first_update_done;
static int lasttime_soc;
static unsigned int flounder_monitor_voltage;
static long last_update_time;
static bool force_decrease;

static int read_sysfs(const char *path, char *buf, size_t size) {
    char *cp = NULL;

    int fd = open(path, O_RDONLY, 0);
    if (fd == -1) {
        KLOG_ERROR(LOG_TAG, "Could not open '%s'\n", path);
        return -1;
    }

    ssize_t count = TEMP_FAILURE_RETRY(read(fd, buf, size));
    if (count > 0)
        cp = (char *)memrchr(buf, '\n', count);

    if (cp)
        *cp = '\0';
    else
        buf[0] = '\0';

    close(fd);
    return count;
}

static void write_sysfs(const char *path, char *s)
{
    char buf[80];
    int len;
    int fd = open(path, O_WRONLY);

    if (fd < 0) {
        strerror_r(errno, buf, sizeof(buf));
        KLOG_ERROR(LOG_TAG, "Error opening %s: %s\n", path, buf);
        return;
    }

    len = write(fd, s, strlen(s));
    if (len < 0) {
        strerror_r(errno, buf, sizeof(buf));
        KLOG_ERROR(LOG_TAG, "Error writing to %s: %s\n", path, buf);
    }

    close(fd);
}

static int64_t get_int64_field(const char *path) {
    const int SIZE = 21;
    char buf[SIZE];

    int64_t value = 0;
    if (read_sysfs(path, buf, SIZE) > 0) {
        value = strtoll(buf, NULL, 0);
    }
    return value;
}

static void flounder_status_check(struct BatteryProperties *props)
{
    if (props->batteryStatus == BATTERY_STATUS_UNKNOWN)
        props->batteryStatus = BATTERY_STATUS_DISCHARGING;
    else if (props->batteryStatus == BATTERY_STATUS_FULL &&
        props->batteryLevel < BATTERY_FULL)
        props->batteryStatus = BATTERY_STATUS_CHARGING;
}

static void flounder_health_check(struct BatteryProperties *props)
{
    if (props->batteryLevel >= BATTERY_FULL)
        props->batteryHealth = BATTERY_HEALTH_GOOD;
    else if (props->batteryLevel < BATTERY_LOW)
        props->batteryHealth = BATTERY_HEALTH_DEAD;
    else
        props->batteryHealth = BATTERY_HEALTH_GOOD;
}

static void flounder_voltage_monitor_check(struct BatteryProperties *props)
{
    unsigned int monitor_voltage = 0;
    int vcell_mv;
    char voltage[10];

    if (props->batteryStatus != BATTERY_STATUS_CHARGING &&
        props->batteryStatus != BATTERY_STATUS_FULL && props->batteryLevel > 0) {
        vcell_mv = props->batteryVoltage;
        if (vcell_mv > BATTERY_CRITICAL_LOW_MV)
            monitor_voltage = BATTERY_CRITICAL_LOW_MV;
        else if (vcell_mv > BATTERY_DEAD_MV)
            monitor_voltage = BATTERY_DEAD_MV;
    }

    if (monitor_voltage != flounder_monitor_voltage) {
        snprintf(voltage, sizeof(voltage), "%d", monitor_voltage);
        write_sysfs(VOLTAGE_MONITOR_PATH, voltage);
        flounder_monitor_voltage = monitor_voltage;
    }
}

static void flounder_soc_adjust(struct BatteryProperties *props)
{
    int soc_decrease;
    int soc, vcell_mv;
    struct sysinfo info;
    long uptime = 0;
    int ret;

    ret = sysinfo(&info);
    if (ret) {
       KLOG_ERROR(LOG_TAG, "Fail to get sysinfo!!\n");
       uptime = last_update_time;
    } else
       uptime = info.uptime;

    if (!first_update_done) {
        if (props->batteryLevel >= BATTERY_FULL) {
            props->batteryLevel = BATTERY_FULL - 1;
            lasttime_soc = BATTERY_FULL - 1;
        } else {
            lasttime_soc = props->batteryLevel;
        }
        last_update_time = uptime;
        first_update_done = true;
    }

    if (props->batteryStatus == BATTERY_STATUS_FULL)
        soc = BATTERY_FULL;
    else if (props->batteryLevel >= BATTERY_FULL &&
             lasttime_soc < BATTERY_FULL)
        soc = BATTERY_FULL - 1;
    else
        soc = props->batteryLevel;

    if (props->batteryLevel > BATTERY_FULL)
        props->batteryLevel = BATTERY_FULL;
    else if (props->batteryLevel < 0)
        props->batteryLevel = 0;

    vcell_mv = props->batteryVoltage;
    if (props->batteryStatus == BATTERY_STATUS_DISCHARGING ||
        props->batteryStatus == BATTERY_STATUS_NOT_CHARGING ||
        props->batteryStatus == BATTERY_STATUS_UNKNOWN) {
        if (vcell_mv >= BATTERY_CRITICAL_LOW_MV) {
            force_decrease = false;
            soc_decrease = lasttime_soc - soc;
            if (soc_decrease < 0) {
                soc = lasttime_soc;
                goto done;
            }

            if (uptime > last_update_time &&
                uptime - last_update_time <= UPDATE_PERIOD_MINIMUM_S) {
                soc = lasttime_soc;
                goto done;
            }

            if (soc_decrease < 0)
                soc_decrease = 0;
            else if (soc_decrease > NORMAL_MAX_SOC_DEC)
                soc_decrease = NORMAL_MAX_SOC_DEC;

            soc = lasttime_soc - soc_decrease;
        } else if (vcell_mv < BATTERY_DEAD_MV) {
            soc = 0;
        } else {
            if (force_decrease &&
                uptime > last_update_time &&
                uptime - last_update_time <= UPDATE_PERIOD_MINIMUM_S) {
                soc = lasttime_soc;
                goto done;
            }

            soc_decrease = CRITICAL_LOW_FORCE_SOC_DROP;
            if (lasttime_soc <= soc_decrease)
               soc = 0;
            else
                soc = lasttime_soc - soc_decrease;
            force_decrease = true;
        }
    } else {
        force_decrease = false;
        if (soc > lasttime_soc)
            soc = lasttime_soc + 1;
    }
    last_update_time = uptime;
done:
    props->batteryLevel = lasttime_soc = soc;
}

static void flounder_bat_monitor(struct BatteryProperties *props)
{
    flounder_soc_adjust(props);
    flounder_health_check(props);
    flounder_status_check(props);
    flounder_voltage_monitor_check(props);
}

int healthd_board_battery_update(struct BatteryProperties *props)
{

    flounder_bat_monitor(props);

    // return 0 to log periodic polled battery status to kernel log
    return 0;
}

static int flounder_energy_counter(int64_t *energy)
{
    *energy = get_int64_field(CHARGE_COUNTER_EXT_PATH) * VOLTAGE_NOMINAL;
    return 0;
}

void healthd_board_init(struct healthd_config *config)
{
    config->energyCounter = flounder_energy_counter;
}
