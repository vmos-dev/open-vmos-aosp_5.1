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

#include <hardware/sensors.h>
#include <algorithm>
#include <pthread.h>
#include <cutils/log.h>

#include "SensorEventQueue.h"

SensorEventQueue::SensorEventQueue(int capacity) {
    mCapacity = capacity;

    mStart = 0;
    mSize = 0;
    mData = new sensors_event_t[mCapacity];
    pthread_cond_init(&mSpaceAvailableCondition, NULL);
}

SensorEventQueue::~SensorEventQueue() {
    delete[] mData;
    mData = NULL;
    pthread_cond_destroy(&mSpaceAvailableCondition);
}

int SensorEventQueue::getWritableRegion(int requestedLength, sensors_event_t** out) {
    if (mSize == mCapacity || requestedLength <= 0) {
        *out = NULL;
        return 0;
    }
    // Start writing after the last readable record.
    int firstWritable = (mStart + mSize) % mCapacity;

    int lastWritable = firstWritable + requestedLength - 1;

    // Don't go past the end of the data array.
    if (lastWritable > mCapacity - 1) {
        lastWritable = mCapacity - 1;
    }
    // Don't go into the readable region.
    if (firstWritable < mStart && lastWritable >= mStart) {
        lastWritable = mStart - 1;
    }
    *out = &mData[firstWritable];
    return lastWritable - firstWritable + 1;
}

void SensorEventQueue::markAsWritten(int count) {
    mSize += count;
}

int SensorEventQueue::getSize() {
    return mSize;
}

sensors_event_t* SensorEventQueue::peek() {
    if (mSize == 0) return NULL;
    return &mData[mStart];
}

void SensorEventQueue::dequeue() {
    if (mSize == 0) return;
    if (mSize == mCapacity) {
        pthread_cond_broadcast(&mSpaceAvailableCondition);
    }
    mSize--;
    mStart = (mStart + 1) % mCapacity;
}

// returns true if it waited, or false if it was a no-op.
bool SensorEventQueue::waitForSpace(pthread_mutex_t* mutex) {
    bool waited = false;
    while (mSize == mCapacity) {
        waited = true;
        pthread_cond_wait(&mSpaceAvailableCondition, mutex);
    }
    return waited;
}
