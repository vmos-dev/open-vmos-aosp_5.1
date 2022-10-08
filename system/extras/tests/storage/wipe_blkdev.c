/*
 * Copyright (C) 2011, 2012 The Android Open Source Project
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

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/fs.h>
#include <errno.h>

typedef unsigned long long u64;

#ifndef BLKDISCARD
#define BLKDISCARD _IO(0x12,119)
#endif

#ifndef BLKSECDISCARD
#define BLKSECDISCARD _IO(0x12,125)
#endif

static u64 get_block_device_size(int fd)
{
        u64 size = 0;
        int ret;

        ret = ioctl(fd, BLKGETSIZE64, &size);

        if (ret)
                return 0;

        return size;
}

static int wipe_block_device(int fd, u64 len, int secure)
{
    u64 range[2];
    int ret;
    int req;

    range[0] = 0;
    range[1] = len;
    if (secure) {
       req = BLKSECDISCARD;
    } else {
       req = BLKDISCARD;
    }

    ret = ioctl(fd, req, &range);
    if (ret < 0) {
        fprintf(stderr, "%s discard failed, errno = %d\n",
                secure ? "Secure" : "Nonsecure", errno);
    }

    return ret;
}

static void usage(void)
{
    fprintf(stderr, "Usage: wipe_blkdev [-s] <partition>\n");
    exit(1);
}

int main(int argc, char *argv[])
{
    int secure = 0;
    char *devname;
    int fd;
    u64 len;
    struct stat statbuf;
    int ret;

    if ((argc != 2) && (argc != 3)) {
        usage();
    }

    if (argc == 3) {
        if (!strcmp(argv[1], "-s")) {
            secure = 1;
            devname = argv[2];
        } else {
            usage();
        }
    } else {
        devname = argv[1];
    }

    fd = open(devname, O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "Cannot open device %s\n", devname);
        exit(1);
    }

    if (fstat(fd, &statbuf) < 0) {
        fprintf(stderr, "Cannot stat %s\n", devname);
        exit(1);
    }

    if (!S_ISBLK(statbuf.st_mode)) {
        fprintf(stderr, "%s is not a block device\n", devname);
        exit(1);
    }

    len = get_block_device_size(fd);

    if (! len) {
        fprintf(stderr, "Cannot get size of block device %s\n", devname);
        exit(1);
    }

    ret = wipe_block_device(fd, len, secure);

    close(fd);

    return ret;
}
