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


#include <malloc.h>
#include <stdio.h>
#include <time.h>
#include "stopwatch.h"
#include <math.h>

#define SNPRINTF_OR_RETURN(str, size, format, ...) {                    \
        int len = snprintf((str), (size), (format), ## __VA_ARGS__);    \
        if (len < 0) return;                                            \
        if (len > static_cast<int>(size)) {                             \
            fprintf(stderr, "Not enough space\n");                      \
            return;                                                     \
        } else {                                                        \
            (size) -= len; (str) += len;                                \
        }                                                               \
    }

namespace {
const bool kVerbose = false;
bool printRaw = false;
}

namespace android_test {

StopWatch::StopWatch(const char *name, size_t capacity)
    : mName(strdup(name)), mNum(0), mData(NULL), mDataLen(0), mCapacity(capacity * 2),
      mSizeKbytes(0), mAlreadyPrinted(false), mPrintRaw(false),
      mDuration(0.0), mDeviation(0.0),
      mMinDuration(0.0), mMinIdx(0),
      mMaxDuration(0.0), mMaxIdx(0),
      mDeltas(NULL), mUsed(false)
{
    mStart.tv_sec = 0;
    mStart.tv_nsec = 0;
    mData = (Measurement *) malloc(mCapacity * sizeof(Measurement));
}

StopWatch::~StopWatch()
{
    if (mUsed && !mAlreadyPrinted)
    {
        fprintf(stderr, "Discarding data for %s\n", mName);
    }
    free(mData);
    free(mName);
    delete [] mDeltas;
}

void StopWatch::start()
{
    checkCapacity();
    clock_gettime(CLOCK_MONOTONIC, &mData[mDataLen].mTime);
    mData[mDataLen].mIsStart = true;
    if (!mUsed)
    {
        mStart = mData[mDataLen].mTime; // mDataLen should be 0
        mUsed = true;
    }
    ++mNum;
    ++mDataLen;
}

void StopWatch::stop()
{
    checkCapacity();
    clock_gettime(CLOCK_MONOTONIC, &mData[mDataLen].mTime);
    mData[mDataLen].mIsStart = false;
    ++mDataLen;
}

void StopWatch::setPrintRawMode(bool raw)
{
    printRaw = raw;
}


void StopWatch::sprint(char **str, size_t *size)
{
    if (kVerbose) fprintf(stderr, "printing\n");
    mAlreadyPrinted = true;
    if (0 == mDataLen)
    {
        return;
    }
    if (mDataLen > 0 && mData[mDataLen - 1].mIsStart)
    {
        stop();
    }
    if (kVerbose) SNPRINTF_OR_RETURN(*str, *size, "# Got %d samples for %s\n", mDataLen, mName);
    processSamples();

    SNPRINTF_OR_RETURN(*str, *size, "# StopWatch %s total/cumulative duration %f Samples: %d\n",
                       mName, mDuration, mNum);
    printThroughput(str, size);
    printAverageMinMax(str, size);

    if (printRaw)
    {
        // print comment header and summary values.

        SNPRINTF_OR_RETURN(*str, *size, "# Name Iterations  Duration Min MinIdx Max MaxIdx SizeKbytes\n");
        SNPRINTF_OR_RETURN(*str, *size, "%s %d %f %f %d %f %d %d\n", mName, mNum, mDuration,
                           mMinDuration, mMinIdx, mMaxDuration, mMaxIdx, mSizeKbytes);
        // print each duration sample
        for (size_t i = 0; i < mDataLen / 2; ++i)
        {
            long second = mData[i * 2].mTime.tv_sec - mStart.tv_sec;
            long nano = mData[i * 2].mTime.tv_nsec - mStart.tv_nsec;

            SNPRINTF_OR_RETURN(*str, *size, "%f %f\n", double(second) + double(nano) / 1.0e9, mDeltas[i]);
        }
    }

}

// Normally we should have enough capacity but if we have to
// reallocate the measurement buffer (e.g start and stop called more
// than once in an iteration) we let the user know. She should provide
// a capacity when building the StopWatch.
void StopWatch::checkCapacity()
{
    if (mDataLen >= mCapacity)
    {
        mCapacity *= 2;
        fprintf(stderr, "# Increased capacity to %d for %s. Measurement affected.\n",
                mCapacity, mName);
        mData = (Measurement *)realloc(mData, mCapacity * sizeof(Measurement));
    }
}


// Go over all the samples and compute the diffs between a start and
// stop pair. The diff is accumulated in mDuration and inserted in
// mDeltas.
// The min and max values for a diff are also tracked.
void StopWatch::processSamples()
{
    if (kVerbose) fprintf(stderr, "processing samples\n");
    size_t n = mDataLen / 2;
    mDeltas= new double[n];
    for (size_t i = 0; i < mDataLen; i += 2)   // even: start  odd: stop
    {
        long second = mData[i + 1].mTime.tv_sec - mData[i].mTime.tv_sec;
        long nano = mData[i + 1].mTime.tv_nsec - mData[i].mTime.tv_nsec;

        mDeltas[i / 2] = double(second) + double(nano) / 1.0e9;
    }

    for (size_t i = 0; i < n; ++i)
    {
        if (0 == i)
        {
            mMinDuration = mMaxDuration = mDeltas[i];
        }
        else
        {
            if (mMaxDuration < mDeltas[i])
            {
                mMaxDuration = mDeltas[i];
                mMaxIdx = i;
            }
            if (mMinDuration > mDeltas[i])
            {
                mMinDuration = mDeltas[i];
                mMinIdx = i;
            }
        }
        mDuration += mDeltas[i];
    }
    double avgDuration = mDuration / n;
    double diffSQ = 0.0;
    for (size_t i = 0; i < n; ++i)
    {
      diffSQ += pow((mDeltas[i] - avgDuration), 2.0);
    }
    mDeviation = sqrt(diffSQ / n);
}


double StopWatch::timespecToDouble(const struct timespec& time)
{
    double val = double(time.tv_nsec) / 1.0e9 + double(time.tv_sec);
    return val < 0.0 ? -val : val;  // sometimes 0.00 is -0.00
}


// If we have only 2 values, don't bother printing anything.
void StopWatch::printAverageMinMax(char **str, size_t *size)
{
    if (mDataLen > 2) // if there is only one sample, avg, min, max are trivial.
    {
        SNPRINTF_OR_RETURN(*str, *size, "# Average %s duration %f s/op\n", mName, mDuration / mNum);
        SNPRINTF_OR_RETURN(*str, *size, "# Standard deviation %s duration %f \n", mName, mDeviation);
        SNPRINTF_OR_RETURN(*str, *size, "# Min %s duration %f [%d]\n", mName, mMinDuration, mMinIdx);
        SNPRINTF_OR_RETURN(*str, *size, "# Max %s duration %f [%d]\n", mName, mMaxDuration, mMaxIdx);
    }
}

void StopWatch::printThroughput(char **str, size_t *size)
{
    if (0 != mSizeKbytes)
    {
        SNPRINTF_OR_RETURN(*str, *size, "# Size: %d Kbytes  Total: %d\n", mSizeKbytes, mNum);
        SNPRINTF_OR_RETURN(*str, *size, "# Speed %f Kbyte/s\n", double(mSizeKbytes) * mNum / mDuration);
    }
}
}  // namespace android_test
