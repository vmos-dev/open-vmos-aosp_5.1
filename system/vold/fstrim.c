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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <limits.h>
#include <linux/fs.h>
#include <time.h>
#include <fs_mgr.h>
#include <pthread.h>
#define LOG_TAG "fstrim"
#include "cutils/log.h"
#include "hardware_legacy/power.h"

/* These numbers must match what the MountService specified in
 * frameworks/base/services/java/com/android/server/EventLogTags.logtags
 */
#define LOG_FSTRIM_START  2755
#define LOG_FSTRIM_FINISH 2756

#define FSTRIM_WAKELOCK "dofstrim"

#define UNUSED __attribute__((unused))

/* From a would-be kernel header */
#define FIDTRIM         _IOWR('f', 128, struct fstrim_range)    /* Deep discard trim */

static unsigned long long get_boot_time_ms(void)
{
    struct timespec t;
    unsigned long long time_ms;

    t.tv_sec = 0;
    t.tv_nsec = 0;
    clock_gettime(CLOCK_BOOTTIME, &t);
    time_ms = (t.tv_sec * 1000LL) + (t.tv_nsec / 1000000);

    return time_ms;
}

static void *do_fstrim_filesystems(void *thread_arg)
{
    int i;
    int fd;
    int ret = 0;
    struct fstrim_range range = { 0 };
    struct stat sb;
    extern struct fstab *fstab;
    int deep_trim = !!thread_arg;

    SLOGI("Starting fstrim work...\n");

    /* Log the start time in the event log */
    LOG_EVENT_LONG(LOG_FSTRIM_START, get_boot_time_ms());

    for (i = 0; i < fstab->num_entries; i++) {
        /* Skip raw partitions */
        if (!strcmp(fstab->recs[i].fs_type, "emmc") ||
            !strcmp(fstab->recs[i].fs_type, "mtd")) {
            continue;
        }
        /* Skip read-only filesystems */
        if (fstab->recs[i].flags & MS_RDONLY) {
            continue;
        }
        if (fs_mgr_is_voldmanaged(&fstab->recs[i])) {
            continue; /* Should we trim fat32 filesystems? */
        }

        if (stat(fstab->recs[i].mount_point, &sb) == -1) {
            SLOGE("Cannot stat mount point %s\n", fstab->recs[i].mount_point);
            ret = -1;
            continue;
        }
        if (!S_ISDIR(sb.st_mode)) {
            SLOGE("%s is not a directory\n", fstab->recs[i].mount_point);
            ret = -1;
            continue;
        }

        fd = open(fstab->recs[i].mount_point, O_RDONLY);
        if (fd < 0) {
            SLOGE("Cannot open %s for FITRIM\n", fstab->recs[i].mount_point);
            ret = -1;
            continue;
        }

        memset(&range, 0, sizeof(range));
        range.len = ULLONG_MAX;
        SLOGI("Invoking %s ioctl on %s", deep_trim ? "FIDTRIM" : "FITRIM", fstab->recs[i].mount_point);

        ret = ioctl(fd, deep_trim ? FIDTRIM : FITRIM, &range);
        if (ret) {
            SLOGE("%s ioctl failed on %s (error %d/%s)", deep_trim ? "FIDTRIM" : "FITRIM", fstab->recs[i].mount_point, errno, strerror(errno));
            ret = -1;
        } else {
            SLOGI("Trimmed %llu bytes on %s\n", range.len, fstab->recs[i].mount_point);
        }
        close(fd);
    }

    /* Log the finish time in the event log */
    LOG_EVENT_LONG(LOG_FSTRIM_FINISH, get_boot_time_ms());

    SLOGI("Finished fstrim work.\n");

    /* Release the wakelock that let us work */
    release_wake_lock(FSTRIM_WAKELOCK);

    return (void *)(uintptr_t)ret;
}

int fstrim_filesystems(int deep_trim)
{
    pthread_t t;
    int ret;

    /* Get a wakelock as this may take a while, and we don't want the
     * device to sleep on us.
     */
    acquire_wake_lock(PARTIAL_WAKE_LOCK, FSTRIM_WAKELOCK);

    /* Depending on the emmc chip and size, this can take upwards
     * of a few minutes.  If done in the same thread as the caller
     * of this function, that would block vold from accepting any
     * commands until the trim is finished.  So start another thread
     * to do the work, and return immediately.
     *
     * This function should not be called more than once per day, but
     * even if it is called a second time before the first one finishes,
     * the kernel will "do the right thing" and split the work between
     * the two ioctls invoked in separate threads.
     */
    ret = pthread_create(&t, NULL, do_fstrim_filesystems, (void *)(intptr_t)deep_trim);
    if (ret) {
        SLOGE("Cannot create thread to do fstrim");
        return ret;
    }

    ret = pthread_detach(t);
    if (ret) {
        SLOGE("Cannot detach thread doing fstrim");
        return ret;
    }

    return 0;
}
