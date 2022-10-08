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

#define LOG_TAG "CHARGER_TOUCH"
#include <hardware_legacy/uevent.h>
#include <utils/Log.h>
#include <fcntl.h>

#define POWER_SUPPLY_PATH "/sys/class/power_supply"
#define TOUCH_PATH "/sys/devices/virtual/input/lge_touch/charger"

const char* WIRELESS = "change@/devices/platform/bq51051b_wlc/power_supply/wireless";
const char* USB = "change@/devices/platform/msm_ssbi.0/pm8921-core/pm8921-charger/power_supply/usb";

enum {
    NO_CHARGER,
    CHARGER_WIRELESS,
    CHARGER_USB,
    CHARGER_AC
};

static void read_path(const char* path, char* buf, size_t size)
{
    int fd = open(path, O_RDONLY, 0);
    int count;

    if (fd < 0) {
        ALOGE("Could not open %s", path);
        return;
    }
    count = read(fd, buf, size);

    close(fd);
}

static void write_path(int type)
{
    int fd = open(TOUCH_PATH, O_RDWR, 0);
    char buf[2];

    if (fd < 0) {
        ALOGE("Could not open %s", TOUCH_PATH);
        return;
    }

    snprintf(buf, sizeof(buf), "%d", type);
    write(fd, buf, 1);

    close(fd);
}

static void handle_uevent(const char* udata)
{
    const char *str = udata;
    char path[PATH_MAX];
    char wlc[2], usb[2], ac[2];
    int type = NO_CHARGER;

    memset(wlc, 0, 2);
    memset(usb, 0, 2);
    memset(ac, 0, 2);

    if (!strncmp(str, WIRELESS, strlen(WIRELESS))) {
        snprintf(path, sizeof(path), "%s/wireless/online", POWER_SUPPLY_PATH);
        read_path(path, wlc, 1);
        if (!strncmp(wlc, "1", 1))
            type = CHARGER_WIRELESS;

        ALOGE("Type: %d", type);
        write_path(type);
    } else if (!strncmp(str, USB, strlen(USB))) {
        snprintf(path, sizeof(path), "%s/usb/online", POWER_SUPPLY_PATH);
        read_path(path, usb, 1);

        snprintf(path, sizeof(path), "%s/pm8921-dc/online", POWER_SUPPLY_PATH);
        read_path(path, ac, 1);

        if (!strncmp(usb, "1", 1)) {
            type = CHARGER_USB;
        } else if (!strncmp(ac, "1", 1)) {
            type = CHARGER_AC;
        }

        ALOGE("Type: %d", type);
        write_path(type);
    }

}

static void event_loop(void)
{
    int len = 0;
    static char udata[4096];
    memset(udata, 0, sizeof(udata));

    uevent_init();

    while (1) {
        len = uevent_next_event(udata, sizeof(udata) - 2);
        handle_uevent(udata);
    }
}

int main()
{
    event_loop();
    return 0;
}
