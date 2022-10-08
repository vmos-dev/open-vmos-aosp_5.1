/*
 * Copyright (C) 2013 The Android Open Source Project
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

#define LOG_TAG "healthd-manta"
#include <errno.h>
#include <fcntl.h>
#include <healthd.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <batteryservice/BatteryService.h>
#include <cutils/klog.h>
#include <sys/stat.h>
#include <sys/types.h>

/* Nominal voltage for ENERGY_COUNTER computation */
#define VOLTAGE_NOMINAL 3.7

#define POWER_SUPPLY_SUBSYSTEM "power_supply"
#define POWER_SUPPLY_SYSFS_PATH "/sys/class/" POWER_SUPPLY_SUBSYSTEM

#define DS2784_PATH POWER_SUPPLY_SYSFS_PATH "/ds2784-fuelgauge"
#define CHARGE_COUNTER_EXT_PATH DS2784_PATH "/charge_counter_ext"

using namespace android;

#define TEMP_HIGH_THRESHOLD     600     /* 60C */
#define TEMP_HIGH_RECOVERY      420     /* 42C */
#define TEMP_LOW_RECOVERY       0       /*  0C */
#define TEMP_LOW_THRESHOLD      -50     /* -5C */

#define FULL_CHARGING_TIME      (12 * 60 * 60)
#define RECHARGING_TIME         (2 * 60 * 60)
#define RECHARGING_VOLTAGE      (4250)

static bool manta_bat_recharging;
static time_t manta_bat_charging_start_time;

static unsigned int manta_bat_health = BATTERY_HEALTH_GOOD;
static unsigned int manta_bat_charging_status = BATTERY_STATUS_DISCHARGING;
static bool manta_bat_batterypresent;

static int charge_enabled_fd;

static void manta_bat_check_temp(struct BatteryProperties *props)
{
    if (props->chargerAcOnline == false &&
        props->chargerUsbOnline == false)
        return;

    if (props->batteryTemperature >= TEMP_HIGH_THRESHOLD) {
        if (manta_bat_health != BATTERY_HEALTH_OVERHEAT &&
            manta_bat_health != BATTERY_HEALTH_UNSPECIFIED_FAILURE) {
                KLOG_INFO(LOG_TAG,
                          "battery overheat (%d), charging unavailable\n",
                          props->batteryTemperature);
                manta_bat_health = BATTERY_HEALTH_OVERHEAT;
        }
    } else if (props->batteryTemperature <= TEMP_HIGH_RECOVERY &&
               props->batteryTemperature >= TEMP_LOW_RECOVERY) {
        if (manta_bat_health == BATTERY_HEALTH_OVERHEAT ||
            manta_bat_health == BATTERY_HEALTH_COLD) {
            KLOG_INFO(LOG_TAG,
                      "battery recovery (%d), charging available\n",
                      props->batteryTemperature);
            manta_bat_health = BATTERY_HEALTH_GOOD;
        }
    } else if (props->batteryTemperature <= TEMP_LOW_THRESHOLD) {
        if (manta_bat_health != BATTERY_HEALTH_COLD &&
            manta_bat_health != BATTERY_HEALTH_UNSPECIFIED_FAILURE) {
            KLOG_INFO(LOG_TAG,
                      "battery cold (%d), charging unavailable\n",
                      props->batteryTemperature);
            manta_bat_health = BATTERY_HEALTH_COLD;
        }
    }
}

static void manta_bat_set_charge_time(bool enable)
{
    if (enable && !manta_bat_charging_start_time)
        time(&manta_bat_charging_start_time);
    else if (!enable)
        manta_bat_charging_start_time = 0;
}

static void manta_bat_enable_charging(bool enable)
{
    int ret;
    char val[20];

    if (enable && (manta_bat_health != BATTERY_HEALTH_GOOD)) {
        manta_bat_charging_status = BATTERY_STATUS_NOT_CHARGING;
        return;
    }

    if (charge_enabled_fd < 0)
        goto err;

    snprintf(val, sizeof(val), "%d", enable);
    ret = write(charge_enabled_fd, val, strlen(val));
    if (ret == -1) {
        KLOG_ERROR(LOG_TAG, "charge_enabled write error; errno=%d\n", errno);
        goto err;
    }

    manta_bat_set_charge_time(enable);
    KLOG_INFO(LOG_TAG, "battery charge enable=%d\n", enable);
    return;

err:
    if (enable)
        manta_bat_charging_status = BATTERY_STATUS_NOT_CHARGING;
}

static bool manta_bat_charge_timeout(time_t timeout)
{
    if (!manta_bat_charging_start_time)
        return false;

    return time(NULL) >= manta_bat_charging_start_time + timeout;
}

static void manta_bat_set_full(void)
{
    KLOG_INFO(LOG_TAG, "battery full\n");
    manta_bat_charging_status = BATTERY_STATUS_FULL;
    manta_bat_enable_charging(false);
    manta_bat_recharging = false;
}

static void manta_bat_charging_timer(struct BatteryProperties *props)
{
    if (!manta_bat_charging_start_time &&
        manta_bat_charging_status == BATTERY_STATUS_CHARGING) {
        KLOG_WARNING("battery charging timer not started, starting\n");
        manta_bat_enable_charging(true);
        manta_bat_recharging = true;
    } else if (!manta_bat_charging_start_time) {
        return;
    }

    if (manta_bat_charge_timeout(manta_bat_recharging ?
                                 RECHARGING_TIME : FULL_CHARGING_TIME)) {
        KLOG_INFO(LOG_TAG, "battery charging timer expired\n");
        if (props->batteryVoltage > RECHARGING_VOLTAGE &&
            props->batteryLevel == 100) {
            manta_bat_set_full();
        } else {
            manta_bat_enable_charging(false);
            manta_bat_recharging = false;
            manta_bat_charging_start_time = 0;
        }
    }
}

static void manta_bat_check_charge_source_changed(
    struct BatteryProperties *props)
{
    if (props->chargerUsbOnline || props->chargerAcOnline) {
        if (manta_bat_charging_status == BATTERY_STATUS_CHARGING ||
            (manta_bat_charging_status == BATTERY_STATUS_FULL &&
             manta_bat_recharging))
            return;

        /*
         * If charging status indicates a charger was already
         * connected prior to this and the status is something
         * other than charging ("full" or "not-charging"), leave
         * the status alone.
         */
        if (manta_bat_charging_status == BATTERY_STATUS_DISCHARGING ||
            manta_bat_charging_status == BATTERY_STATUS_UNKNOWN)
            manta_bat_charging_status = BATTERY_STATUS_CHARGING;

        /*
         * Don't re-enable charging if the battery is full and we
         * are not actively re-charging it, or if "not-charging"
         * status is set.
         */
        if (!(manta_bat_charging_status == BATTERY_STATUS_FULL
              && !manta_bat_recharging) &&
            manta_bat_charging_status != BATTERY_STATUS_NOT_CHARGING)
            manta_bat_enable_charging(true);
    } else if (manta_bat_charging_status == BATTERY_STATUS_CHARGING ||
               manta_bat_charging_status == BATTERY_STATUS_NOT_CHARGING ||
               manta_bat_charging_status == BATTERY_STATUS_FULL) {
        manta_bat_charging_status = BATTERY_STATUS_DISCHARGING;
        manta_bat_enable_charging(false);
        manta_bat_health = BATTERY_HEALTH_GOOD;
        manta_bat_recharging = false;
        manta_bat_charging_start_time = 0;
    }
}

static void manta_bat_monitor(struct BatteryProperties *props)
{
    unsigned int old_bat_health = manta_bat_health;

    if (manta_bat_batterypresent) {
        manta_bat_check_temp(props);
    } else {
         props->batteryTemperature = 42;  /* 4.2C */
         props->batteryVoltage = 4342;    /* 4342mV */
         props->batteryLevel = 42;        /* 42% */
    }

    if (props->batteryStatus == BATTERY_STATUS_FULL &&
        (manta_bat_charging_status == BATTERY_STATUS_CHARGING ||
         manta_bat_recharging)) {
            manta_bat_set_full();
    }

    manta_bat_check_charge_source_changed(props);

    switch (manta_bat_charging_status) {
    case BATTERY_STATUS_FULL:
        if (props->batteryVoltage < RECHARGING_VOLTAGE &&
            !manta_bat_recharging) {
            KLOG_INFO(LOG_TAG, "start recharging, v=%d\n",
                      props->batteryVoltage);
            manta_bat_recharging = true;
            manta_bat_enable_charging(true);
        }
        break;
    case BATTERY_STATUS_DISCHARGING:
        break;
    case BATTERY_STATUS_CHARGING:
        switch (manta_bat_health) {
        case BATTERY_HEALTH_OVERHEAT:
        case BATTERY_HEALTH_COLD:
        case BATTERY_HEALTH_OVER_VOLTAGE:
        case BATTERY_HEALTH_DEAD:
        case BATTERY_HEALTH_UNSPECIFIED_FAILURE:
            manta_bat_charging_status = BATTERY_STATUS_NOT_CHARGING;
            manta_bat_enable_charging(false);
            KLOG_INFO(LOG_TAG, "Not charging, health=%d\n",
                      manta_bat_health);
            break;
        default:
            break;
        }
        break;
    case BATTERY_STATUS_NOT_CHARGING:
        if (old_bat_health != BATTERY_HEALTH_GOOD &&
            manta_bat_health == BATTERY_HEALTH_GOOD) {
                KLOG_INFO(LOG_TAG, "battery health recovered\n");

                if (props->chargerUsbOnline || props->chargerAcOnline) {
                    manta_bat_enable_charging(true);
                    manta_bat_charging_status = BATTERY_STATUS_CHARGING;
                } else {
                    manta_bat_charging_status =
                            BATTERY_STATUS_DISCHARGING;
                }
        }
        break;
    default:
        break;
    }

    manta_bat_charging_timer(props);

    // set health and status according to our state, hardcode invariants
    props->batteryHealth = manta_bat_health;
    props->batteryStatus = manta_bat_charging_status;
    props->batteryTechnology = "Li-ion";
    props->batteryPresent = manta_bat_batterypresent;
}

int healthd_board_battery_update(struct BatteryProperties *props)
{
    manta_bat_monitor(props);

    // return 0 to log periodic polled battery status to kernel log
    return 0;
}

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

static int64_t get_int64_field(const char *path) {
    const int SIZE = 21;
    char buf[SIZE];

    int64_t value = 0;
    if (read_sysfs(path, buf, SIZE) > 0) {
        value = strtoll(buf, NULL, 0);
    }
    return value;
}

static int manta_energy_counter(int64_t *energy)
{
    *energy = get_int64_field(CHARGE_COUNTER_EXT_PATH) * VOLTAGE_NOMINAL;
    return 0;
}

void healthd_board_init(struct healthd_config *config)
{
    charge_enabled_fd = open(POWER_SUPPLY_SYSFS_PATH
                             "/manta-battery/charge_enabled", O_WRONLY);
    if (charge_enabled_fd == -1)
        KLOG_ERROR(LOG_TAG, "open manta-battery/charge_enabled failed"
                   "; errno=%d\n", errno);

    config->batteryCurrentNowPath = DS2784_PATH "/current_now";

    if (access(config->batteryCurrentNowPath.string(), R_OK) == 0) {
        manta_bat_batterypresent = true;
    } else {
        KLOG_INFO(LOG_TAG, "Missing battery, using fake battery data\n");
        config->batteryCurrentNowPath.clear();
    }

    config->energyCounter = manta_energy_counter;
}
