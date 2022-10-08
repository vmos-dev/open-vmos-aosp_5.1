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

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <string.h>
#include <sys/stat.h>
#include <linux/fadvise.h>
#include <unistd.h>
#include <fts.h>

#include "stopwatch.h"
#include "sysutil.h"
#include "testcase.h"

// Stress test for the sdcard. Use this to generate some load on the
// sdcard and collect performance statistics. The output is either a
// human readable report or the raw timing samples that can be
// processed using another tool.
//
// Needs debugfs:
//   adb root;
//   adb shell mount -t debugfs none /sys/kernel/debug
//
// The following tests are defined (value of the --test flag):
//  write:       Open a file write some random data and close.
//  read:        Open a file read it and close.
//  read_write:  Combine readers and writers.
//  open_create: Open|create an non existing file.
//
// For each run you can control how many processes will run the test in
// parallel to simulate a real load (--procnb flag)
//
// For each process, the test selected will be executed many time to
// get a meaningful average/min/max (--iterations flag)
//
// Use --dump to also get the raw data.
//
// For read/write tests, size is the number of Kbytes to use.
//
// To build: mmm system/extras/tests/sdcard/Android.mk SDCARD_TESTS=1
//
// Examples:
// adb shell /system/bin/sdcard_perf_test --test=read --size=1000 --chunk-size=100 --procnb=1 --iterations=10 --dump > /tmp/data.txt
// adb shell /system/bin/sdcard_perf_test --test=write --size=1000 --chunk-size=100 --procnb=1 --iterations=100 --dump > /tmp/data.txt
//
// To watch the memory: cat /proc/meminfo
// If the phone crashes, look at /proc/last_kmsg on reboot.
//
// TODO: It would be cool if we could play with various fadvise()
// strategies in here to see how tweaking read-ahead changes things.
//

extern char *optarg;
extern int optind, opterr, optopt;

// TODO: No clue where fadvise is. Disabled for now.
#define FADVISE(fd, off, len, advice) (void)0

#ifndef min
#define min(a,b) (((a)>(b))?(b):(a))
#endif

namespace  {
using android::kernelVersion;
using android::kMinKernelVersionBufferSize;
using android::schedFeatures;
using android::kMinSchedFeaturesBufferSize;
using android_test::StopWatch;
using android::writePidAndWaitForReply;
using android::waitForChildrenAndSignal;
using android::waitForChildrenOrExit;
using android_test::TestCase;

const char *kAppName = "sdcard_perf_test";
const char *kTestDir = "/sdcard/perf";
const bool kVerbose = false;

// Used by getopt to parse the command line.
struct option long_options[] = {
    {"size", required_argument, 0, 's'},
    {"chunk-size", required_argument, 0, 'S'},
    {"depth", required_argument, 0, 'D'},
    {"iterations",  required_argument, 0, 'i'},
    {"procnb",  required_argument, 0, 'p'},
    {"test",  required_argument, 0, 't'},
    {"dump",  no_argument, 0, 'd'},
    {"cpu-scaling",  no_argument, 0, 'c'},
    {"sync",  required_argument, 0, 'f'},
    {"truncate", no_argument, 0, 'e'},
    {"no-new-fair-sleepers", no_argument, 0, 'z'},
    {"no-normalized-sleepers", no_argument, 0, 'Z'},
    {"fadvise", required_argument, 0, 'a'},
    {"help", no_argument, 0, 'h'},
    {0, 0, 0, 0},
};

void usage()
{
    printf("sdcard_perf_test --test=write|read|read_write|open_create|traverse [options]\n\n"
           "  -t --test:        Select the test.\n"
           "  -s --size:        Size in kbytes of the data.\n"
           "  -S --chunk-size:  Size of a chunk. Default to size ie 1 chunk.\n"
           "                    Data will be written/read using that chunk size.\n"
           "  -D --depth:       Depth of directory tree to create for traversal.\n",
           "  -i --iterations:  Number of time a process should carry its task.\n"
           "  -p --procnb:      Number of processes to use.\n"
           "  -d --dump:        Print the raw timing on stdout.\n"
           "  -c --cpu-scaling: Enable cpu scaling.\n"
           "  -s --sync: fsync|sync Use fsync or sync in write test. Default: no sync call.\n"
           "  -e --truncate:    Truncate to size the file on creation.\n"
           "  -z --no-new-fair-sleepers: Turn them off. You need to mount debugfs.\n"
           "  -Z --no-normalized-sleepers: Turn them off. You need to mount debugfs.\n"
           "  -a --fadvise:     Specify an fadvise policy (not supported).\n"
           );
}

// Print command line, pid, kernel version, OOM adj and scheduler.
void printHeader(int argc, char **argv, const TestCase& testCase)
{
    printf("# Command: ");
    for (int i = 0; i < argc; ++i)
    {
        printf("%s ", argv[i]);
    }
    printf("\n");

    printf("# Pid: %d\n", getpid());

    {
        char buffer[kMinKernelVersionBufferSize] = {0, };
        if (kernelVersion(buffer, sizeof(buffer)) > 0)
        {
            printf("# Kernel: %s", buffer);
        }
    }

    // Earlier on, running this test was crashing the phone. It turned
    // out that it was using too much memory but its oom_adj value was
    // -17 which means disabled. -16 is the system_server and 0 is
    // typically what applications run at. The issue is that adb runs
    // at -17 and so is this test. We force oom_adj to 0 unless the
    // oom_adj option has been used.
    // TODO: We talked about adding an option to control oom_adj, not
    // sure if we still need that.
    int oomAdj = android::pidOutOfMemoryAdj();

    printf("# Oom_adj: %d ", oomAdj);
    if (oomAdj < 0)
    {
        android::setPidOutOfMemoryAdj(0);
        printf("adjuted to %d", android::pidOutOfMemoryAdj());
    }
    printf("\n");

    {
        char buffer[kMinSchedFeaturesBufferSize] = {0, };
        if (schedFeatures(buffer, sizeof(buffer)) > 0)
        {
            printf("# Sched features: %s", buffer);
        }
    }
    printf("# Fadvise: %s\n", testCase.fadviseAsStr());
}

// Remove all the files under kTestDir and clear the caches.
void cleanup() {
    android::resetDirectory(kTestDir);
    android::syncAndDropCaches();  // don't pollute runs.
}

// @param argc, argv have a wild guess.
// @param[out] testCase Structure built from the cmd line args.
void parseCmdLine(int argc, char **argv, TestCase *testCase)\
{
    int c;

    while(true)
    {
        // getopt_long stores the option index here.
        int option_index = 0;

        c = getopt_long (argc, argv,
                         "hS:s:D:i:p:t:dcf:ezZa:",
                         long_options,
                         &option_index);
        // Detect the end of the options.
        if (c == -1) break;

        switch (c)
        {
            case 's':
                testCase->setDataSize(atoi(optarg) * 1024);
                break;
            case 'S':
                testCase->setChunkSize(atoi(optarg) * 1024);
                break;
            case 'D': // tree depth
                testCase->setTreeDepth(atoi(optarg));
                break;
            case 'i':
                testCase->setIter(atoi(optarg));
                printf("# Iterations: %d\n", testCase->iter());
                break;
            case 'p':
                testCase->setNproc(atoi(optarg));
                printf("# Proc nb: %d\n", testCase->nproc());
                break;
            case 't':
                testCase->setTypeFromName(optarg);
                printf("# Test name %s\n", testCase->name());
                break;
            case 'd':
                testCase->setDump();
                break;
            case 'c':
                printf("# Cpu scaling is on\n");
                testCase->setCpuScaling();
                break;
            case 'f':
                if (strcmp("sync", optarg) == 0) {
                    testCase->setSync(TestCase::SYNC);
                } else if (strcmp("fsync", optarg) == 0) {
                    testCase->setSync(TestCase::FSYNC);
                }
                break;
            case 'e':  // e for empty
                printf("# Will truncate to size\n");
                testCase->setTruncateToSize();
                break;
            case 'z':  // no new fair sleepers
                testCase->setNewFairSleepers(false);
                break;
            case 'Z':  // no normalized sleepers
                testCase->setNormalizedSleepers(false);
                break;
            case 'a':  // fadvise
                testCase->setFadvise(optarg);
                break;
            case 'h':
                usage();
                exit(0);
            default:
                fprintf(stderr, "Unknown option %s\n", optarg);
                exit(EXIT_FAILURE);
        }
    }
}

// ----------------------------------------------------------------------
// READ TEST

// Read a file.  We use a new file each time to avoid any caching
// effect that would happen if we were reading the same file each
// time.
// @param chunk buffer large enough where the chunk read are written.
// @param idx iteration number.
// @param testCase has all the timers and paramters to run the test.

bool readData(char *const chunk, const int idx, TestCase *testCase)
{
    char filename[80] = {'\0',};

    sprintf(filename, "%s/file-%d-%d", kTestDir, idx, getpid());

    testCase->openTimer()->start();
    int fd = open(filename, O_RDONLY);
    testCase->openTimer()->stop();

    if (fd < 0)
    {
        fprintf(stderr, "Open read only failed.");
        return false;
    }
    FADVISE(fd, 0, 0, testCase->fadvise());

    size_t left = testCase->dataSize();
    pid_t *pid = (pid_t*)chunk;
    while (left > 0)
    {
        char *dest = chunk;
        size_t chunk_size = testCase->chunkSize();

        if (chunk_size > left)
        {
            chunk_size = left;
            left = 0;
        }
        else
        {
            left -= chunk_size;
        }

        testCase->readTimer()->start();
        while (chunk_size > 0)
        {
            ssize_t s = read(fd, dest, chunk_size);
            if (s < 0)
            {
                fprintf(stderr, "Failed to read.\n");
                close(fd);
                return false;
            }
            chunk_size -= s;
            dest += s;
        }
        testCase->readTimer()->stop();
    }
    close(fd);
    if (testCase->pid() != *pid)
    {
        fprintf(stderr, "Wrong pid found @ read block %x != %x\n", testCase->pid(), *pid);
        return false;
    }
    else
    {
        return true;
    }
}


bool testRead(TestCase *testCase) {
    // Setup the testcase by writting some dummy files.
    const size_t size = testCase->dataSize();
    size_t chunk_size = testCase->chunkSize();
    char *const chunk = new char[chunk_size];

    memset(chunk, 0xaa, chunk_size);
    *((pid_t *)chunk) = testCase->pid(); // write our pid at the beginning of each chunk

    size_t iter = testCase->iter();

    // since readers are much faster we increase the number of
    // iteration to last longer and have concurrent read/write
    // thoughout the whole test.
    if (testCase->type() == TestCase::READ_WRITE)
    {
        iter *= TestCase::kReadWriteFactor;
    }

    for (size_t i = 0; i < iter; ++i)
    {
        char filename[80] = {'\0',};

        sprintf(filename, "%s/file-%d-%d", kTestDir, i, testCase->pid());
        int fd = open(filename, O_RDWR | O_CREAT, S_IRWXU);

        size_t left = size;
        while (left > 0)
        {
            if (chunk_size > left)
            {
                chunk_size = left;
            }
            ssize_t written = write(fd, chunk, chunk_size);
            if (written < 0)
            {
                fprintf(stderr, "Write failed %d %s.", errno, strerror(errno));
                return false;
            }
            left -= written;
        }
        close(fd);
    }
    if (kVerbose) printf("Child %d all chunk written\n", testCase->pid());

    android::syncAndDropCaches();
    testCase->signalParentAndWait();

    // Start the read test.
    testCase->testTimer()->start();
    for (size_t i = 0; i < iter; ++i)
    {
        if (!readData(chunk, i, testCase))
        {
            return false;
        }
    }
    testCase->testTimer()->stop();

    delete [] chunk;
    return true;
}

// ----------------------------------------------------------------------
// WRITE TEST

bool writeData(const char *const chunk, const int idx, TestCase *testCase) {
    char filename[80] = {'\0',};

    sprintf(filename, "%s/file-%d-%d", kTestDir, idx, getpid());
    testCase->openTimer()->start();
    int fd = open(filename, O_RDWR | O_CREAT, S_IRWXU);  // no O_TRUNC, see header comment
    testCase->openTimer()->stop();

    if (fd < 0)
    {
        fprintf(stderr, "Open write failed.");
        return false;
    }
    FADVISE(fd, 0, 0, testCase->fadvise());

    if (testCase->truncateToSize())
    {
        testCase->truncateTimer()->start();
        ftruncate(fd, testCase->dataSize());
        testCase->truncateTimer()->stop();
    }

    size_t left = testCase->dataSize();
    while (left > 0)
    {
        const char *dest = chunk;
        size_t chunk_size = testCase->chunkSize();

        if (chunk_size > left)
        {
            chunk_size = left;
            left = 0;
        }
        else
        {
            left -= chunk_size;
        }


        testCase->writeTimer()->start();
        while (chunk_size > 0)
        {
            ssize_t s = write(fd, dest, chunk_size);
            if (s < 0)
            {
                fprintf(stderr, "Failed to write.\n");
                close(fd);
                return false;
            }
            chunk_size -= s;
            dest += s;
        }
        testCase->writeTimer()->stop();
    }

    if (TestCase::FSYNC == testCase->sync())
    {
        testCase->syncTimer()->start();
        fsync(fd);
        testCase->syncTimer()->stop();
    }
    else if (TestCase::SYNC == testCase->sync())
    {
        testCase->syncTimer()->start();
        sync();
        testCase->syncTimer()->stop();
    }
    close(fd);
    return true;
}

bool testWrite(TestCase *testCase)
{
    const size_t size = testCase->dataSize();
    size_t chunk_size = testCase->chunkSize();
    char *data = new char[chunk_size];

    memset(data, 0xaa, chunk_size);
    // TODO: write the pid at the beginning like in the write
    // test. Have a mode where we check the write was correct.
    testCase->signalParentAndWait();

    testCase->testTimer()->start();
    for (size_t i = 0; i < testCase->iter(); ++i)
    {
        if (!writeData(data, i, testCase))
        {
            return false;
        }
    }
    testCase->testTimer()->stop();
    delete [] data;
    return true;
}


// ----------------------------------------------------------------------
// READ WRITE

// Mix of read and write test. Even PID run the write test. Odd PID
// the read test. Not fool proof but work most of the time.
bool testReadWrite(TestCase *testCase)
{
    if (getpid() & 0x1) {
        return testRead(testCase);
    } else {
        return testWrite(testCase);
    }
}

// ----------------------------------------------------------------------
// OPEN CREATE TEST

bool testOpenCreate(TestCase *testCase)
{
    char filename[80] = {'\0',};

    testCase->signalParentAndWait();
    testCase->testTimer()->start();

    for (size_t i = 0; i < testCase->iter(); ++i)
    {
        sprintf(filename, "%s/file-%d-%d", kTestDir, i, testCase->pid());

        int fd = open(filename, O_RDWR | O_CREAT, S_IRWXU);
        FADVISE(fd, 0, 0, testCase->fadvise());

        if (testCase->truncateToSize())
        {
            ftruncate(fd, testCase->dataSize());
        }
        if (fd < 0)
        {
            return false;
        }
        close(fd);
    }
    testCase->testTimer()->stop();
    return true;
}

bool writeTestFile(TestCase *testCase, const char* filename) {
    int fd = open(filename, O_RDWR | O_CREAT, S_IRWXU);
    if (fd < 0) {
        fprintf(stderr, "open() failed: %s\n", strerror(errno));
        return false;
    }

    bool res = false;

    char * const chunk = new char[testCase->chunkSize()];
    memset(chunk, 0xaa, testCase->chunkSize());

    size_t left = testCase->dataSize();
    while (left > 0) {
        char *dest = chunk;
        size_t chunk_size = testCase->chunkSize();

        if (chunk_size > left) {
            chunk_size = left;
            left = 0;
        } else {
            left -= chunk_size;
        }

        while (chunk_size > 0) {
            ssize_t s = write(fd, dest, chunk_size);
            if (s < 0) {
                fprintf(stderr, "write() failed: %s\n", strerror(errno));
                goto fail;
            }
            chunk_size -= s;
            dest += s;
        }
    }

    res = true;
fail:
    close(fd);
    delete[] chunk;
    return res;
}

// ----------------------------------------------------------------------
// TRAVERSE

#define MAX_PATH 512

// Creates a directory tree that is both deep and wide, and times
// traversal using fts_open().
bool testTraverse(TestCase *testCase) {
    char path[MAX_PATH];
    char filepath[MAX_PATH];
    strcpy(path, kTestDir);

    // Generate a deep directory hierarchy
    size_t depth = testCase->treeDepth();
    for (size_t i = 0; i < depth; i++) {
        // Go deeper by appending onto current path
        snprintf(path + strlen(path), MAX_PATH - strlen(path), "/dir%d", i);
        mkdir(path, S_IRWXU);

        // Create some files at this depth
        strcpy(filepath, path);
        int pathlen = strlen(path);
        char* nameStart = filepath + pathlen;
        for (size_t j = 0; j < depth; j++) {
            snprintf(nameStart, MAX_PATH - pathlen, "/file%d", j);
            writeTestFile(testCase, filepath);
        }
    }

    testCase->signalParentAndWait();
    testCase->testTimer()->start();

    // Now traverse structure
    size_t iter = testCase->iter();
    for (size_t i = 0; i < iter; i++) {
        testCase->traverseTimer()->start();

        FTS *ftsp;
        if ((ftsp = fts_open((char **) &kTestDir, FTS_LOGICAL | FTS_XDEV, NULL)) == NULL) {
            fprintf(stderr, "fts_open() failed: %s\n", strerror(errno));
            return false;
        }

        // Count discovered files
        int dirs = 0, files = 0;

        FTSENT *curr;
        while ((curr = fts_read(ftsp)) != NULL) {
            switch (curr->fts_info) {
            case FTS_D:
                dirs++;
                break;
            case FTS_F:
                files++;
                break;
            }
        }

        fts_close(ftsp);

        testCase->traverseTimer()->stop();

        int expectedDirs = depth + 1;
        if (expectedDirs != dirs) {
            fprintf(stderr, "expected %d dirs, but found %d\n", expectedDirs, dirs);
            return false;
        }

        int expectedFiles = depth * depth;
        if (expectedFiles != files) {
            fprintf(stderr, "expected %d files, but found %d\n", expectedFiles, files);
            return false;
        }
    }

    testCase->testTimer()->stop();
    return true;
}

}  // anonymous namespace

int main(int argc, char **argv)
{
    android_test::TestCase testCase(kAppName);

    cleanup();

    parseCmdLine(argc, argv, &testCase);
    printHeader(argc, argv, testCase);

    printf("# File size %d kbytes\n", testCase.dataSize() / 1024);
    printf("# Chunk size %d kbytes\n", testCase.chunkSize() / 1024);
    printf("# Sync: %s\n", testCase.syncAsStr());

    if (!testCase.cpuScaling())
    {
        android::disableCpuScaling();
    }

    switch(testCase.type()) {
        case TestCase::WRITE:
            testCase.mTestBody = testWrite;
            break;
        case TestCase::READ:
            testCase.mTestBody = testRead;
            break;
        case TestCase::OPEN_CREATE:
            testCase.mTestBody = testOpenCreate;
            break;
        case TestCase::READ_WRITE:
            testCase.mTestBody = testReadWrite;
            break;
        case TestCase::TRAVERSE:
            testCase.mTestBody = testTraverse;
            break;
        default:
            fprintf(stderr, "Unknown test type %s", testCase.name());
            exit(EXIT_FAILURE);
    }

    testCase.createTimers();

    bool ok;

    ok = testCase.runTest();

    cleanup();
    if (!ok)
    {
        printf("error %d %s", errno, strerror(errno));
        exit(EXIT_FAILURE);
    }
    else
    {
        exit(EXIT_SUCCESS);
    }
}
