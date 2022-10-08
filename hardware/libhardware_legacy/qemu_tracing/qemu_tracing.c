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
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

// This is the pathname to the sysfs file that enables and disables
// tracing on the qemu emulator.
#define SYS_QEMU_TRACE_STATE "/sys/qemu_trace/state"


// This is the pathname to the sysfs file that adds new (address, symbol)
// pairs to the trace.
#define SYS_QEMU_TRACE_SYMBOL "/sys/qemu_trace/symbol"

// The maximum length of a symbol name
#define MAX_SYMBOL_NAME_LENGTH (4 * 1024)

// Allow space in the buffer for the address plus whitespace.
#define MAX_BUF_SIZE (MAX_SYMBOL_NAME_LENGTH + 20)

// return 0 on success, or an error if the qemu driver cannot be opened
int qemu_start_tracing()
{
    int fd = open(SYS_QEMU_TRACE_STATE, O_WRONLY);
    if (fd < 0)
        return fd;
    write(fd, "1\n", 2);
    close(fd);
    return 0;
}

int qemu_stop_tracing()
{
    int fd = open(SYS_QEMU_TRACE_STATE, O_WRONLY);
    if (fd < 0)
        return fd;
    write(fd, "0\n", 2);
    close(fd);
    return 0;
}

int qemu_add_mapping(unsigned int addr, const char *name)
{
    char buf[MAX_BUF_SIZE];

    if (strlen(name) > MAX_SYMBOL_NAME_LENGTH)
        return EINVAL;
    int fd = open(SYS_QEMU_TRACE_SYMBOL, O_WRONLY);
    if (fd < 0)
        return fd;
    sprintf(buf, "%x %s\n", addr, name);
    write(fd, buf, strlen(buf));
    close(fd);
    return 0;
}

int qemu_remove_mapping(unsigned int addr)
{
    char buf[MAX_BUF_SIZE];

    int fd = open(SYS_QEMU_TRACE_SYMBOL, O_WRONLY);
    if (fd < 0)
        return fd;
    sprintf(buf, "%x\n", addr);
    write(fd, buf, strlen(buf));
    close(fd);
    return 0;
}
