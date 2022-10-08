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


#ifndef SYSTEM_EXTRAS_TESTS_SDCARD_TESTCASE_H_
#define SYSTEM_EXTRAS_TESTS_SDCARD_TESTCASE_H_

#include <stdlib.h>
#include "stopwatch.h"
#include "sysutil.h"

namespace android_test {

// Class to group test parameters and implementation.
// Takes care of forking child processes and wait for them.

class TestCase {
  public:
    enum Type {UNKNOWN_TEST, WRITE, READ, OPEN_CREATE, READ_WRITE, TRAVERSE};
    enum Pipe {READ_FROM_CHILD = 0, WRITE_TO_PARENT, READ_FROM_PARENT, WRITE_TO_CHILD};
    enum Sync {NO_SYNC, FSYNC, SYNC};

    // Reads takes less time than writes. This is a basic
    // approximation of how much longer the read tasks must run to
    // terminate roughly at the same time as the write tasks.
    const static int kReadWriteFactor = 5;

    TestCase(const char *appName);

    ~TestCase();

    size_t iter() const { return mIter; }
    void setIter(size_t iter);

    size_t nproc() const { return mNproc; }
    void setNproc(size_t val) { mNproc = val; }

    size_t dataSize() const { return mDataSize; }
    void setDataSize(size_t val) { mDataSize = val; }

    size_t chunkSize() const { return mChunkSize; }
    void setChunkSize(size_t val) { mChunkSize = val; }

    size_t treeDepth() const { return mTreeDepth; }
    void setTreeDepth(size_t val) { mTreeDepth = val; }

    bool newFairSleepers() const { return mNewFairSleepers; }
    void setNewFairSleepers(bool val) {
        mNewFairSleepers = val;
        android::setNewFairSleepers(val);
    }

    bool normalizedSleepers() const { return mNormalizedSleepers; }
    void setNormalizedSleepers(bool val) {
        mNormalizedSleepers = val;
        android::setNormalizedSleepers(val);
    }

    Sync sync() const { return mSync; }
    void setSync(Sync s);
    const char *syncAsStr() const;

    bool cpuScaling() const { return mCpuScaling; }
    void setCpuScaling() { mCpuScaling = true; }

    bool truncateToSize() const { return mTruncateToSize; }
    void setTruncateToSize() { mTruncateToSize = true; }

    int fadvise() { return mFadvice; }
    void setFadvise(const char *advice);
    const char *fadviseAsStr() const;

    // Print the samples.
    void setDump() { StopWatch::setPrintRawMode(true); }

    StopWatch *testTimer() { return mTestTimer; }
    StopWatch *openTimer() { return mOpenTimer; }
    StopWatch *readTimer() { return mReadTimer; }
    StopWatch *writeTimer() { return mWriteTimer; }
    StopWatch *syncTimer() { return mSyncTimer; }
    StopWatch *truncateTimer() { return mTruncateTimer; }
    StopWatch *traverseTimer() { return mTraverseTimer; }

    // Fork the children, run the test and wait for them to complete.
    bool runTest();

    void signalParentAndWait() {
        if (!android::writePidAndWaitForReply(mIpc[WRITE_TO_PARENT], mIpc[READ_FROM_PARENT])) {
            exit(1);
        }
    }

    void createTimers();
    bool setTypeFromName(const char *test_name);
    Type type() const { return mType; }
    pid_t pid() const { return mPid; }
    const char *name() const { return mName; }

    // This is set to the function that will actually do the test when
    // the command line arguments have been parsed. The function will
    // be run in one or more child(ren) process(es).
    bool (*mTestBody)(TestCase *);
private:
    const char *mAppName;
    size_t mDataSize;
    size_t mChunkSize;
    size_t mTreeDepth;
    size_t mIter;
    size_t mNproc;
    pid_t mPid;
    char mName[80];
    Type mType;

    bool mDump;  // print the raw values instead of a human friendly report.
    bool mCpuScaling;  // true, do not turn off cpu scaling.
    Sync mSync;
    int mFadvice;
    // When new files are created, truncate them to the final size.
    bool mTruncateToSize;

    bool mNewFairSleepers;
    bool mNormalizedSleepers;

    // IPC
    //        Parent               Child(ren)
    // ---------------------------------------
    // 0: read from child          closed
    // 1: closed                   write to parent
    // 2: closed                   read from parent
    // 3: write to child           closed
    int mIpc[4];

    StopWatch *mTestTimer;  // Used to time the test overall.
    StopWatch *mOpenTimer;  // Used to time the open calls.
    StopWatch *mReadTimer;  // Used to time the read calls.
    StopWatch *mWriteTimer;  // Used to time the write calls.
    StopWatch *mSyncTimer;  // Used to time the sync/fsync calls.
    StopWatch *mTruncateTimer;  // Used to time the ftruncate calls.
    StopWatch *mTraverseTimer;  // Used to time each traversal.
};

}  // namespace android_test

#endif  // SYSTEM_EXTRAS_TESTS_SDCARD_TESTCASE_H_
