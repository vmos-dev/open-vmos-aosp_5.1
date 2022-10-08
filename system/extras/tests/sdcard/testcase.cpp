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

#include "testcase.h"
#include <hardware_legacy/power.h>  // wake lock
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <linux/fadvise.h>

namespace {
const bool kVerbose = false;
}

namespace android_test {

TestCase::TestCase(const char *appName)
    : mTestBody(NULL), mAppName(appName), mDataSize(1000 * 1000),
      mChunkSize(mDataSize), mTreeDepth(8), mIter(20), mNproc(1),
      mType(UNKNOWN_TEST),  mDump(false), mCpuScaling(false),
      mSync(NO_SYNC), mFadvice(POSIX_FADV_NORMAL), mTruncateToSize(false),
      mTestTimer(NULL)
{
    // Make sure the cpu and phone are fully awake. The
    // FULL_WAKE_LOCK was used by java apps and don't do
    // anything. Also the partial wake lock is a nop if the phone
    // is plugged in via USB.
    acquire_wake_lock(PARTIAL_WAKE_LOCK, mAppName);
    mPid = getpid();
    setNewFairSleepers(true);
    setNormalizedSleepers(true);
    if (pipe(mIpc) < 0)
    {
        fprintf(stderr, "pipe failed\n");
        exit(1);
    }
    if (pipe(mIpc + 2) < 0)
    {
        fprintf(stderr, "pipe failed\n");
        exit(1);
    }
}

TestCase::~TestCase()
{
    release_wake_lock(mAppName);
    for (int i = 0; i < 4; ++i) close(mIpc[i]);
    delete mTestTimer;
    delete mOpenTimer;
}


bool TestCase::runTest()
{
    if (UNKNOWN_TEST == mType)
    {
        fprintf(stderr, "No test set.");
        return false;
    }
    for (size_t p = 0; p < mNproc; ++p)
    {
        pid_t childpid = android::forkOrExit();

        if (0 == childpid) { // I am a child, run the test.
            mPid = getpid();
            close(mIpc[READ_FROM_CHILD]);
            close(mIpc[WRITE_TO_CHILD]);

            if (kVerbose) printf("Child pid: %d\n", mPid);
            if (!mTestBody(this)) {
                printf("Test failed\n");
            }
            char buffer[32000] = {0,};
            char *str = buffer;
            size_t size_left = sizeof(buffer);

            testTimer()->sprint(&str, &size_left);
            if(openTimer()->used()) openTimer()->sprint(&str, &size_left);
            if(readTimer()->used()) readTimer()->sprint(&str, &size_left);
            if(writeTimer()->used()) writeTimer()->sprint(&str, &size_left);
            if(syncTimer()->used()) syncTimer()->sprint(&str, &size_left);
            if(truncateTimer()->used()) truncateTimer()->sprint(&str, &size_left);
            if(traverseTimer()->used()) traverseTimer()->sprint(&str, &size_left);

            write(mIpc[TestCase::WRITE_TO_PARENT], buffer, str - buffer);


            close(mIpc[WRITE_TO_PARENT]);
            close(mIpc[READ_FROM_PARENT]);
            exit(EXIT_SUCCESS);
        }
    }
    // I am the parent process
    close(mIpc[WRITE_TO_PARENT]);
    close(mIpc[READ_FROM_PARENT]);

    // Block until all the children have reported for
    // duty. Unblock them so they start the work.
    if (!android::waitForChildrenAndSignal(mNproc, mIpc[READ_FROM_CHILD], mIpc[WRITE_TO_CHILD]))
    {
        exit(1);
    }

    // Process the output of each child.
    // TODO: handle EINTR
    char buffer[32000] = {0,};
    while(read(mIpc[READ_FROM_CHILD], buffer, sizeof(buffer)) != 0)
    {
        printf("%s", buffer);
        fflush(stdout);
        memset(buffer, 0, sizeof(buffer));
    }
    // Parent is waiting for children to exit.
    android::waitForChildrenOrExit(mNproc);
    return true;
}

void TestCase::setIter(size_t iter)
{
    mIter = iter;
}

void TestCase::createTimers()
{
    char total_time[80];

    snprintf(total_time, sizeof(total_time), "%s_total", mName);
    mTestTimer = new StopWatch(total_time, 1);
    mTestTimer->setDataSize(dataSize());

    mOpenTimer = new StopWatch("open", iter() * kReadWriteFactor);

    mReadTimer = new StopWatch("read", iter() * dataSize() / chunkSize() * kReadWriteFactor);
    mReadTimer->setDataSize(dataSize());

    mWriteTimer = new StopWatch("write", iter() * dataSize() / chunkSize());
    mWriteTimer->setDataSize(dataSize());

    mSyncTimer = new StopWatch("sync", iter());

    mTruncateTimer = new StopWatch("truncate", iter());

    mTraverseTimer = new StopWatch("traversal", iter());
}

bool TestCase::setTypeFromName(const char *test_name)
{
    strcpy(mName, test_name);
    if (strcmp(mName, "write") == 0) mType = WRITE;
    if (strcmp(mName, "read") == 0) mType = READ;
    if (strcmp(mName, "read_write") == 0) mType = READ_WRITE;
    if (strcmp(mName, "open_create") == 0) mType = OPEN_CREATE;
    if (strcmp(mName, "traverse") == 0) mType = TRAVERSE;

    return UNKNOWN_TEST != mType;
}

void TestCase::setSync(Sync s)
{
    mSync = s;
}

const char *TestCase::syncAsStr() const
{
    return mSync == NO_SYNC ? "disabled" : (mSync == FSYNC ? "fsync" : "sync");
}

void TestCase::setFadvise(const char *advice)
{
    mFadvice = POSIX_FADV_NORMAL;
    if (strcmp(advice, "sequential") == 0)
    {
        mFadvice = POSIX_FADV_SEQUENTIAL;
    }
    else if (strcmp(advice, "random") == 0)
    {
        mFadvice = POSIX_FADV_RANDOM;
    }
    else if (strcmp(advice, "noreuse") == 0)
    {
        mFadvice = POSIX_FADV_NOREUSE;
    }
    else if (strcmp(advice, "willneed") == 0)
    {
        mFadvice = POSIX_FADV_WILLNEED;
    }
    else if (strcmp(advice, "dontneed") == 0)
    {
        mFadvice = POSIX_FADV_DONTNEED;
    }
}

const char *TestCase::fadviseAsStr() const
{
    switch (mFadvice) {
        case POSIX_FADV_NORMAL: return "fadv_normal";
        case POSIX_FADV_SEQUENTIAL: return "fadv_sequential";
        case POSIX_FADV_RANDOM: return "fadv_random";
        case POSIX_FADV_NOREUSE: return "fadv_noreuse";
        case POSIX_FADV_WILLNEED: return "fadv_willneed";
        case POSIX_FADV_DONTNEED: return "fadv_dontneed";
        default: return "fadvice_unknown";
    }
}


}  // namespace android_test
