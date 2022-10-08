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
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <sys/mount.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include <linux/kdev_t.h>

#define LOG_TAG "Vold"

#include <cutils/log.h>

#include <sysutils/SocketClient.h>
#include "Loop.h"
#include "Asec.h"

int Loop::dumpState(SocketClient *c) {
    int i;
    int fd;
    char filename[256];

    for (i = 0; i < LOOP_MAX; i++) {
        struct loop_info64 li;
        int rc;

        sprintf(filename, "/dev/block/loop%d", i);

        if ((fd = open(filename, O_RDWR)) < 0) {
            if (errno != ENOENT) {
                SLOGE("Unable to open %s (%s)", filename, strerror(errno));
            } else {
                continue;
            }
            return -1;
        }

        rc = ioctl(fd, LOOP_GET_STATUS64, &li);
        close(fd);
        if (rc < 0 && errno == ENXIO) {
            continue;
        }

        if (rc < 0) {
            SLOGE("Unable to get loop status for %s (%s)", filename,
                 strerror(errno));
            return -1;
        }
        char *tmp = NULL;
        asprintf(&tmp, "%s %d %lld:%lld %llu %lld:%lld %lld 0x%x {%s} {%s}", filename, li.lo_number,
                MAJOR(li.lo_device), MINOR(li.lo_device), li.lo_inode, MAJOR(li.lo_rdevice),
                        MINOR(li.lo_rdevice), li.lo_offset, li.lo_flags, li.lo_crypt_name,
                        li.lo_file_name);
        c->sendMsg(0, tmp, false);
        free(tmp);
    }
    return 0;
}

int Loop::lookupActive(const char *id, char *buffer, size_t len) {
    int i;
    int fd;
    char filename[256];

    memset(buffer, 0, len);

    for (i = 0; i < LOOP_MAX; i++) {
        struct loop_info64 li;
        int rc;

        sprintf(filename, "/dev/block/loop%d", i);

        if ((fd = open(filename, O_RDWR)) < 0) {
            if (errno != ENOENT) {
                SLOGE("Unable to open %s (%s)", filename, strerror(errno));
            } else {
                continue;
            }
            return -1;
        }

        rc = ioctl(fd, LOOP_GET_STATUS64, &li);
        close(fd);
        if (rc < 0 && errno == ENXIO) {
            continue;
        }

        if (rc < 0) {
            SLOGE("Unable to get loop status for %s (%s)", filename,
                 strerror(errno));
            return -1;
        }
        if (!strncmp((const char*) li.lo_crypt_name, id, LO_NAME_SIZE)) {
            break;
        }
    }

    if (i == LOOP_MAX) {
        errno = ENOENT;
        return -1;
    }
    strncpy(buffer, filename, len -1);
    return 0;
}

int Loop::create(const char *id, const char *loopFile, char *loopDeviceBuffer, size_t len) {
    int i;
    int fd;
    char filename[256];

    for (i = 0; i < LOOP_MAX; i++) {
        struct loop_info64 li;
        int rc;

        sprintf(filename, "/dev/block/loop%d", i);

        /*
         * The kernel starts us off with 8 loop nodes, but more
         * are created on-demand if needed.
         */
        mode_t mode = 0660 | S_IFBLK;
        unsigned int dev = (0xff & i) | ((i << 12) & 0xfff00000) | (7 << 8);
        if (mknod(filename, mode, dev) < 0) {
            if (errno != EEXIST) {
                SLOGE("Error creating loop device node (%s)", strerror(errno));
                return -1;
            }
        }

        if ((fd = open(filename, O_RDWR)) < 0) {
            SLOGE("Unable to open %s (%s)", filename, strerror(errno));
            return -1;
        }

        rc = ioctl(fd, LOOP_GET_STATUS64, &li);
        if (rc < 0 && errno == ENXIO)
            break;

        close(fd);

        if (rc < 0) {
            SLOGE("Unable to get loop status for %s (%s)", filename,
                 strerror(errno));
            return -1;
        }
    }

    if (i == LOOP_MAX) {
        SLOGE("Exhausted all loop devices");
        errno = ENOSPC;
        return -1;
    }

    strncpy(loopDeviceBuffer, filename, len -1);

    int file_fd;

    if ((file_fd = open(loopFile, O_RDWR)) < 0) {
        SLOGE("Unable to open %s (%s)", loopFile, strerror(errno));
        close(fd);
        return -1;
    }

    if (ioctl(fd, LOOP_SET_FD, file_fd) < 0) {
        SLOGE("Error setting up loopback interface (%s)", strerror(errno));
        close(file_fd);
        close(fd);
        return -1;
    }

    struct loop_info64 li;

    memset(&li, 0, sizeof(li));
    strlcpy((char*) li.lo_crypt_name, id, LO_NAME_SIZE);
    strlcpy((char*) li.lo_file_name, loopFile, LO_NAME_SIZE);

    if (ioctl(fd, LOOP_SET_STATUS64, &li) < 0) {
        SLOGE("Error setting loopback status (%s)", strerror(errno));
        close(file_fd);
        close(fd);
        return -1;
    }

    close(fd);
    close(file_fd);

    return 0;
}

int Loop::destroyByDevice(const char *loopDevice) {
    int device_fd;

    device_fd = open(loopDevice, O_RDONLY);
    if (device_fd < 0) {
        SLOGE("Failed to open loop (%d)", errno);
        return -1;
    }

    if (ioctl(device_fd, LOOP_CLR_FD, 0) < 0) {
        SLOGE("Failed to destroy loop (%d)", errno);
        close(device_fd);
        return -1;
    }

    close(device_fd);
    return 0;
}

int Loop::destroyByFile(const char * /*loopFile*/) {
    errno = ENOSYS;
    return -1;
}

int Loop::createImageFile(const char *file, unsigned int numSectors) {
    int fd;

    if ((fd = creat(file, 0600)) < 0) {
        SLOGE("Error creating imagefile (%s)", strerror(errno));
        return -1;
    }

    if (ftruncate(fd, numSectors * 512) < 0) {
        SLOGE("Error truncating imagefile (%s)", strerror(errno));
        close(fd);
        return -1;
    }
    close(fd);
    return 0;
}

int Loop::resizeImageFile(const char *file, unsigned int numSectors) {
    int fd;

    if ((fd = open(file, O_RDWR)) < 0) {
        SLOGE("Error opening imagefile (%s)", strerror(errno));
        return -1;
    }

    SLOGD("Attempting to increase size of %s to %d sectors.", file, numSectors);

    if (fallocate(fd, 0, 0, numSectors * 512)) {
        if (errno == ENOSYS || errno == ENOTSUP) {
            SLOGW("fallocate not found. Falling back to ftruncate.");
            if (ftruncate(fd, numSectors * 512) < 0) {
                SLOGE("Error truncating imagefile (%s)", strerror(errno));
                close(fd);
                return -1;
            }
        } else {
            SLOGE("Error allocating space (%s)", strerror(errno));
            close(fd);
            return -1;
        }
    }
    close(fd);
    return 0;
}

int Loop::lookupInfo(const char *loopDevice, struct asec_superblock *sb, unsigned int *nr_sec) {
    int fd;
    struct asec_superblock buffer;

    if ((fd = open(loopDevice, O_RDONLY)) < 0) {
        SLOGE("Failed to open loopdevice (%s)", strerror(errno));
        destroyByDevice(loopDevice);
        return -1;
    }

    if (ioctl(fd, BLKGETSIZE, nr_sec)) {
        SLOGE("Failed to get loop size (%s)", strerror(errno));
        destroyByDevice(loopDevice);
        close(fd);
        return -1;
    }

    /*
     * Try to read superblock.
     */
    memset(&buffer, 0, sizeof(struct asec_superblock));
    if (lseek(fd, ((*nr_sec - 1) * 512), SEEK_SET) < 0) {
        SLOGE("lseek failed (%s)", strerror(errno));
        close(fd);
        destroyByDevice(loopDevice);
        return -1;
    }
    if (read(fd, &buffer, sizeof(struct asec_superblock)) != sizeof(struct asec_superblock)) {
        SLOGE("superblock read failed (%s)", strerror(errno));
        close(fd);
        destroyByDevice(loopDevice);
        return -1;
    }
    close(fd);

    /*
     * Superblock successfully read. Copy to caller's struct.
     */
    memcpy(sb, &buffer, sizeof(struct asec_superblock));
    return 0;
}
