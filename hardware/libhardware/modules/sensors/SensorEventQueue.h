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

#ifndef SENSOREVENTQUEUE_H_
#define SENSOREVENTQUEUE_H_

#include <hardware/sensors.h>
#include <pthread.h>

/*
 * Fixed-size circular queue, with an API developed around the sensor HAL poll() method.
 * Poll() takes a pointer to a buffer, which is written by poll() before it returns.
 * This class can provide a pointer to a spot in its internal buffer for poll() to
 * write to, instead of using an intermediate buffer and a memcpy.
 *
 * Thread safety:
 * Reading can be done safely after grabbing the mutex lock, while poll() writing in a separate
 * thread without a mutex lock. But there can only be one writer at a time.
 */
class SensorEventQueue {
    int mCapacity;
    int mStart; // start of readable region
    int mSize; // number of readable items
    sensors_event_t* mData;
    pthread_cond_t mSpaceAvailableCondition;

public:
    SensorEventQueue(int capacity);
    ~SensorEventQueue();

    // Returns length of region, between zero and min(capacity, requestedLength). If there is any
    // writable space, it will return a region of at least one. Because it must return
    // a pointer to a contiguous region, it may return smaller regions as we approach the end of
    // the data array.
    // Only call while holding the lock.
    // The region is not marked internally in any way. Subsequent calls may return overlapping
    // regions. This class expects there to be exactly one writer at a time.
    int getWritableRegion(int requestedLength, sensors_event_t** out);

    // After writing to the region returned by getWritableRegion(), call this to indicate how
    // many records were actually written.
    // This increases size() by count.
    // Only call while holding the lock.
    void markAsWritten(int count);

    // Gets the number of readable records.
    // Only call while holding the lock.
    int getSize();

    // Returns pointer to the first readable record, or NULL if size() is zero.
    // Only call this while holding the lock.
    sensors_event_t* peek();

    // This will decrease the size by one, freeing up the oldest readable event's slot for writing.
    // Only call while holding the lock.
    void dequeue();

    // Blocks until space is available. No-op if there is already space.
    // Returns true if it had to wait.
    bool waitForSpace(pthread_mutex_t* mutex);
};

#endif // SENSOREVENTQUEUE_H_
