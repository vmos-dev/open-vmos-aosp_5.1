/*
 * Copyright (C) 2009 The Android Open Source Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "sysutil.h"

namespace {
const int kError = -1;
// Max number of retries on EAGAIN and EINTR. Totally arbitrary.
const int kMaxAttempts = 8;

// How long to wait after a cache purge. A few seconds (arbitrary).
const int kCachePurgeSleepDuration = 2; // seconds

const bool kSilentIfMissing = false;

const char *kKernelVersion = "/proc/version";
const char *kScalingGovernorFormat = "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_governor";
const char *kDropCaches = "/proc/sys/vm/drop_caches";
const char *kSchedFeatures = "/sys/kernel/debug/sched_features";
const char *kNewFairSleepers = "NEW_FAIR_SLEEPERS";
const char *kNoNewFairSleepers = "NO_NEW_FAIR_SLEEPERS";
const char *kNormalizedSleepers = "NORMALIZED_SLEEPER"; // no 's' at the end
const char *kNoNormalizedSleepers = "NO_NORMALIZED_SLEEPER";

const char *kDebugfsWarningMsg = "Did you 'adb root; adb shell mount -t debugfs none /sys/kernel/debug' ?";

// TODO: Surely these file utility functions must exist already. A
// quick grep did not turn up anything. Look harder later.

void printErrno(const char *msg, const char *filename)
{
    fprintf(stderr, "# %s %s %d %s\n", msg, filename, errno, strerror(errno));
}

// Read a C-string from a file.  If the buffer is too short, an error
// message will be printed on stderr.
// @param filename Of the file to read.
// @param start    Buffer where the data should be written to.
// @param size     The size of the buffer pointed by str. Must be >= 1.
// @return The number of characters read (not including the trailing'\0' used
//         to end the string) or -1 if there was an error.
int readStringFromFile(const char *filename, char *const start, size_t size, bool must_exist=true)
{
    if (NULL == start || size == 0)
    {
        return 0;
    }
    char *end = start;
    int fd = open(filename, O_RDONLY);

    if (fd < 0)
    {
        if (ENOENT != errno || must_exist)
        {
            printErrno("Failed to open", filename);
        }
        return kError;
    }

    bool eof = false;
    bool error = false;
    int attempts = 0;

    --size; // reserve space for trailing '\0'

    while (size > 0 && !error && !eof && attempts < kMaxAttempts)
    {
        ssize_t s;

        s = read(fd, end, size);

        if (s < 0)
        {
            error = EAGAIN != errno && EINTR != errno;
            if (error)
            {
                printErrno("Failed to read", filename);
            }
        }
        else if (0 == s)
        {
            eof = true;
        }
        else
        {
            end += s;
            size -= s;
        }
        ++attempts;
    }

    close(fd);

    if (error)
    {
        return kError;
    }
    else
    {
        *end = '\0';
        if (!eof)
        {
            fprintf(stderr, "Buffer too small for %s\n", filename);
        }
        return end - start;
    }
}

// Write a C string ('\0' terminated) to a file.
//
int writeStringToFile(const char *filename, const char *start, bool must_exist=true)
{
    int fd = open(filename, O_WRONLY);


    if (fd < 0)
    {
        if (ENOENT != errno || must_exist)
        {
            printErrno("Failed to open", filename);
        }
        return kError;
    }

    const size_t len = strlen(start);
    size_t size = len;
    bool error = false;
    int attempts = 0;

    while (size > 0 && !error && attempts < kMaxAttempts)
    {
        ssize_t s = write(fd, start, size);

        if (s < 0)
        {
            error = EAGAIN != errno && EINTR != errno;
            if (error)
            {
                printErrno("Failed to write", filename);
            }
        }
        else
        {
            start += s;
            size -= s;
        }
        ++attempts;
    }
    close(fd);

    if (error)
    {
        return kError;
    }
    else
    {
        if (size > 0)
        {
            fprintf(stderr, "Partial write to %s (%d out of %d)\n",
                    filename, size, len);
        }
        return len - size;
    }
}

int writeIntToFile(const char *filename, long value)
{
    char buffer[16] = {0,};
    sprintf(buffer, "%ld", value);
    return writeStringToFile(filename, buffer);
}

// @return a message describing the reason why the child exited. The
// message is in a shared buffer, not thread safe, erased by
// subsequent calls.
const char *reasonChildExited(int status)
{
    static char buffer[80];

    if (WIFEXITED(status))
    {
        snprintf(buffer, sizeof(buffer), "ok (%d)",  WEXITSTATUS(status));
    }
    else if (WIFSIGNALED(status))
    {
        snprintf(buffer, sizeof(buffer), "signaled (%d %s)",  WTERMSIG(status), strsignal(WTERMSIG(status)));
    }
    else
    {
        snprintf(buffer, sizeof(buffer), "stopped?");
    }
    return buffer;
}


}  // anonymous namespace

namespace android {

int kernelVersion(char *str, size_t size)
{
    return readStringFromFile(kKernelVersion, str, size);
}

int pidOutOfMemoryAdj()
{
    char filename[FILENAME_MAX];

    snprintf(filename, sizeof(filename), "/proc/%d/oom_adj", getpid());

    char value[16];
    if (readStringFromFile(filename, value, sizeof(value)) == -1)
    {
        return -127;
    }
    else
    {
        return atoi(value);
    }
}

void setPidOutOfMemoryAdj(int level)
{
    char filename[FILENAME_MAX];

    snprintf(filename, sizeof(filename), "/proc/%d/oom_adj", getpid());
    writeIntToFile(filename, level);
}

void disableCpuScaling()
{
    for (int cpu = 0; cpu < 16; ++cpu) // 16 cores mobile phones, abestos pockets recommended.
    {
        char governor[FILENAME_MAX];
        sprintf(governor, kScalingGovernorFormat, cpu);

        if (writeStringToFile(governor, "performance", kSilentIfMissing) < 0)
        {
            if (cpu > 0 && errno == ENOENT)
            {
                break;  // cpu1 or above not found, ok since we have cpu0.
            }
            fprintf(stderr, "Failed to write to scaling governor file for cpu %d: %d %s",
                    cpu, errno, strerror(errno));
            break;
        }
    }
}

int schedFeatures(char *str, size_t size)
{
    return readStringFromFile(kSchedFeatures, str, size);
}

bool newFairSleepers()
{
    char value[256] = {0,};

    if (readStringFromFile(kSchedFeatures, value, sizeof(value)) == -1)
    {
        printErrno(kDebugfsWarningMsg, kSchedFeatures);
        return false;
    }
    return strstr(value, "NO_NEW_FAIR_SLEEPERS") == NULL;
}

void setNewFairSleepers(bool on)
{
    int retcode;

    if (on)
    {
        retcode = writeStringToFile(kSchedFeatures, kNewFairSleepers);
    }
    else
    {
        retcode = writeStringToFile(kSchedFeatures, kNoNewFairSleepers);
    }
    if (retcode < 0)
    {
        fprintf(stderr, "# %s\n", kDebugfsWarningMsg);
    }
}

bool normalizedSleepers()
{
    char value[256] = {0,};

    if (readStringFromFile(kSchedFeatures, value, sizeof(value)) == -1)
    {
        printErrno(kDebugfsWarningMsg, kSchedFeatures);
        return false;
    }
    return strstr(value, "NO_NEW_FAIR_SLEEPERS") == NULL;
}

void setNormalizedSleepers(bool on)
{
    int retcode;

    if (on)
    {
        retcode = writeStringToFile(kSchedFeatures, kNormalizedSleepers);
    }
    else
    {
        retcode = writeStringToFile(kSchedFeatures, kNoNormalizedSleepers);
    }
    if (retcode < 0)
    {
        fprintf(stderr, "# %s\n", kDebugfsWarningMsg);
    }
}

pid_t forkOrExit()
{
    pid_t childpid = fork();

    if (-1 == childpid)
    {
        fprintf(stderr, "Fork failed: %d %s", errno, strerror(errno));
        exit(EXIT_FAILURE);
    }
    return childpid;
}

void waitForChildrenOrExit(int num)
{
    while (num > 0)
    {
        int status;
        pid_t pid = wait(&status);
        if (-1 == pid)
        {
            fprintf(stderr, "Wait failed\n");
        }
        else
        {
            if (!WIFEXITED(status))
            {
                fprintf(stderr, "Child pid %d did not exit cleanly %s\n",
                        pid, reasonChildExited(status));
                exit(EXIT_FAILURE);
            }
        }
        --num;
    }
}

// Sync and cache cleaning functions.  In the old hpux days I was told
// to always call *sync twice. The same advice seems to be still true
// today so *sync is called twice.
// Also we wait 'a little' to give a chance to background threads to
// purge their caches.
void syncAndDropCaches(int code)
{
    sync();
    sync();
    writeIntToFile(kDropCaches, code);
    sleep(kCachePurgeSleepDuration);
}


void fsyncAndDropCaches(int fd, int code)
{
    fsync(fd);
    fsync(fd);
    writeIntToFile(kDropCaches, code);
    sleep(kCachePurgeSleepDuration);
}


void resetDirectory(const char *directory)
{
    DIR *dir = opendir(directory);

    if (NULL != dir)
    {
        struct dirent *entry;
        char name_buffer[PATH_MAX];

        while((entry = readdir(dir)))
        {
            if (0 == strcmp(entry->d_name, ".")
                || 0 == strcmp(entry->d_name, "..")
                || 0 == strcmp(entry->d_name, "lost+found"))
            {
                continue;
            }
            strcpy(name_buffer, directory);
            strcat(name_buffer, "/");
            strcat(name_buffer, entry->d_name);
            unlink(name_buffer);
        }
        closedir(dir);
    } else {
        mkdir(directory, S_IRWXU);
    }
}


// IPC
bool writePidAndWaitForReply(int writefd, int readfd)
{
    if (writefd > readfd)
    {
        fprintf(stderr, "Called with args in wrong order!!\n");
        return false;
    }
    pid_t pid = getpid();
    char *start = reinterpret_cast<char *>(&pid);
    size_t size = sizeof(pid);
    bool error = false;
    int attempts = 0;

    while (size > 0 && !error && attempts < kMaxAttempts)
    {
        ssize_t s = write(writefd, start, size);

        if (s < 0)
        {
            error = EAGAIN != errno && EINTR != errno;
            if (error)
            {
                printErrno("Failed to write", "parent");
            }
        }
        else
        {
            start += s;
            size -= s;
        }
        ++attempts;
    }

    if (error || 0 != size)
    {
        return false;
    }

    bool eof = false;
    char dummy;
    size = sizeof(dummy);
    error = false;
    attempts = 0;

    while (size > 0 && !error && !eof && attempts < kMaxAttempts)
    {
        ssize_t s;

        s = read(readfd, &dummy, size);

        if (s < 0)
        {
            error = EAGAIN != errno && EINTR != errno;
            if (error)
            {
                printErrno("Failed to read", "parent");
            }
        }
        else if (0 == s)
        {
            eof = true;
        }
        else
        {
            size -= s;
        }
        ++attempts;
    }
    if (error || 0 != size)
    {
        return false;
    }
    return true;
}



bool waitForChildrenAndSignal(int mProcessNb, int readfd, int writefd)
{
    if (readfd > writefd)
    {
        fprintf(stderr, "Called with args in wrong order!!\n");
        return false;
    }

    bool error;
    int attempts;
    size_t size;

    for (int p = 0; p < mProcessNb; ++p)
    {
        bool eof = false;
        pid_t pid;
        char *end = reinterpret_cast<char *>(&pid);

        error = false;
        attempts = 0;
        size = sizeof(pid);

        while (size > 0 && !error && !eof && attempts < kMaxAttempts)
        {
            ssize_t s;

            s = read(readfd, end, size);

            if (s < 0)
            {
                error = EAGAIN != errno && EINTR != errno;
                if (error)
                {
                    printErrno("Failed to read", "child");
                }
            }
            else if (0 == s)
            {
                eof = true;
            }
            else
            {
                end += s;
                size -= s;
            }
            ++attempts;
        }

        if (error || 0 != size)
        {
            return false;
        }
    }

    for (int p = 0; p < mProcessNb; ++p)
    {
        char dummy;

        error = false;
        attempts = 0;
        size = sizeof(dummy);

        while (size > 0 && !error && attempts < kMaxAttempts)
        {
            ssize_t s = write(writefd, &dummy, size);

            if (s < 0)
            {
                error = EAGAIN != errno && EINTR != errno;
                if (error)
                {
                    printErrno("Failed to write", "child");
                }
            }
            else
            {
                size -= s;
            }
            ++attempts;
        }

        if (error || 0 != size)
        {
            return false;
        }
    }
    return true;
}

}  // namespace android
