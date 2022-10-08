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

#ifndef ANDROID_INPUT_EVENT_READER_H
#define ANDROID_INPUT_EVENT_READER_H

#include <stdint.h>
#include <errno.h>
#include <sys/cdefs.h>
#include <sys/types.h>
#include "sensors.h"
#include "iio/events.h"

/*****************************************************************************/

struct iio_event_data;

class InputEventCircularReader
{
    struct iio_event_data* const mBuffer;
    struct iio_event_data* const mBufferEnd;
    struct iio_event_data* mHead;
    struct iio_event_data* mCurr;
    size_t mMaxEvents;
    size_t mFreeEvents;

public:
    InputEventCircularReader(size_t numEvents);
    ~InputEventCircularReader();
    ssize_t fill(int fd);
    bool readEvent(int fd, iio_event_data const** events);
    void next();
};

/*****************************************************************************/

#endif  /* ANDROID_INPUT_EVENT_READER_H */
