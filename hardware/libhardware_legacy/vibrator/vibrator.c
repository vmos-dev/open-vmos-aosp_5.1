/*
 * Copyright (C) 2008 The Android Open Source Project
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
#include <hardware_legacy/vibrator.h>
#include "qemu.h"

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#define THE_DEVICE "/sys/class/timed_output/vibrator/enable"

int vibrator_exists()
{
    int fd;

#ifdef QEMU_HARDWARE
    if (qemu_check()) {
        return 1;
    }
#endif

    fd = open(THE_DEVICE, O_RDWR);
    if(fd < 0)
        return 0;
    close(fd);
    return 1;
}

static int sendit(int timeout_ms)
{
    int nwr, ret, fd;
    char value[20];

#ifdef QEMU_HARDWARE
    if (qemu_check()) {
        return qemu_control_command( "vibrator:%d", timeout_ms );
    }
#endif

    fd = open(THE_DEVICE, O_RDWR);
    if(fd < 0)
        return errno;

    nwr = sprintf(value, "%d\n", timeout_ms);
    ret = write(fd, value, nwr);

    close(fd);

    return (ret == nwr) ? 0 : -1;
}

int vibrator_on(int timeout_ms)
{
    /* constant on, up to maximum allowed time */
    return sendit(timeout_ms);
}

int vibrator_off()
{
    return sendit(0);
}
