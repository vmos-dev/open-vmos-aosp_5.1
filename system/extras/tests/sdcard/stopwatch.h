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

#ifndef ANDROID_NATIVETEST_SYSTEM_EXTRAS_TESTS_SDCARD_STOPWATCH_H_
#define ANDROID_NATIVETEST_SYSTEM_EXTRAS_TESTS_SDCARD_STOPWATCH_H_

#include <stdlib.h>
#include <stdio.h>
#include <time.h>

namespace android_test {

// StopWatch class to collect execution statistics.
//
// Once the watch has been created, start and stop can be called to
// capture an event duration.
//
// On completion, use 'sprint' to retrieve the data.
//
// If StopWatch::setPrintRawMode(true) has been called, the raw
// samples also are printed.
// The print method is thread safe to avoid mixing the result of
// watches on different threads. For processes, use different files
// that you concat after the run.
//
// If the time measure is associated with some volume of data, use
// setMbytes, the print method will compute the average throughput
// based on that value.
//
// To capture the time accurately and since the runs are not too long,
// we collect the raw start and stop time in an array that get
// processed once all the measurements have been done.
//
// Typical Usage:
// ==============
//
//  StopWatch watch("my name", 20);
//
//  for (int i = 0; i < 20; ++i) {
//    watch.start();
//    doMyStuff();
//    watch.stop();
//  }
//  char buffer[4096];
//  char *str = buffer;
//  size_t size = sizeof(buffer);
//  watch.sprint(&str, &size);
//

class StopWatch {
  public:
    // Time of the snapshot and its nature (start of the interval or end of it).
    struct Measurement {
        struct timespec mTime;
        bool mIsStart;
    };
    static const size_t kUseDefaultCapacity = 20;

    // Create a stop watch. Default capacity == 2 * interval_nb
    // @param name To be used when the results are displayed. No
    //             spaces, use _ instead.
    // @param capacity Hint about the number of sampless that will be
    //                 measured (1 sample == 1 start + 1 stop). Used
    //                 to size the internal storage, when the capacity
    //                 is reached, it is doubled.
    StopWatch(const char *name, size_t capacity = kUseDefaultCapacity);
    ~StopWatch();

    // A StopWatch instance measures time intervals. Use setDataSize
    // if some volume of data is processed during these intervals, to
    // get the average throughput (in kbytes/s) printed.
    void setDataSize(size_t size_in_bytes) { mSizeKbytes = size_in_bytes / 1000; }

    // Starts and stops the timer. The time between the 2 calls is an
    // interval whose duration will be reported in sprint.
    void start();
    void stop();

    // Print a summary of the measurement and optionaly the raw data.
    // The summary is commented out using a leading '#'.  The raw data
    // is a pair (time, duration). The 1st sample is always at time
    // '0.0'.
    // @param str[inout] On entry points to the begining of a buffer
    // where to write the data. On exit points pass the last byte
    // written.
    // @param size[inout] On entry points to the size of the buffer
    // pointed by *str. On exit *size is the amount of free space left
    // in the buffer. If there was not enough space the data is truncated
    // and a warning is printed.
    void sprint(char **str, size_t *size);

    // @return true if at least one interval was timed.
    bool used() const { return mUsed; }

    // Affects all the timers. Instructs all the timers to print the
    // raw data as well as the summary.
    static void setPrintRawMode(bool printRaw);

  private:
    void checkCapacity();
    double timespecToDouble(const struct timespec& time);
    void printAverageMinMax(char **str, size_t *size);
    void printThroughput(char **str, size_t *size);
    // Allocate mDeltas and fill it in. Search for the min and max.
    void processSamples();

    char *const mName;  // Name of the test.
    struct timespec mStart;
    size_t mNum; // # of intervals == # of start() calls.
    struct Measurement *mData;
    size_t mDataLen;
    size_t mCapacity;
    int mSizeKbytes;

    bool mAlreadyPrinted;
    bool mPrintRaw;

    double mDuration;
    double mDeviation;
    double mMinDuration;
    size_t mMinIdx;
    double mMaxDuration;
    size_t mMaxIdx;
    double *mDeltas;

    bool mUsed;
};

}  // namespace android_test

#endif  // ANDROID_NATIVETEST_SYSTEM_EXTRAS_TESTS_SDCARD_STOPWATCH_H_
