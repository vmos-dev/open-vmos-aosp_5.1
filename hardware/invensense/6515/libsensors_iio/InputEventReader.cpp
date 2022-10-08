/*
* Copyright (C) 2012 Invensense, Inc.
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

#define LOG_NDEBUG 0

#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <poll.h>

#include <sys/cdefs.h>
#include <sys/types.h>

#include <linux/input.h>

#include <cutils/log.h>

#include "InputEventReader.h"

/*****************************************************************************/

struct input_event;

InputEventCircularReader::InputEventCircularReader(size_t numEvents)
    : mBuffer(new input_event[numEvents * 2]),
      mBufferEnd(mBuffer + numEvents),
      mHead(mBuffer),
      mCurr(mBuffer),
      mFreeSpace(numEvents)
{
    mLastFd = -1;
}

InputEventCircularReader::~InputEventCircularReader()
{
    delete [] mBuffer;
}

#define INPUT_EVENT_DEBUG (0)
ssize_t InputEventCircularReader::fill(int fd)
{
    size_t numEventsRead = 0;
    mLastFd = fd;

    LOGV_IF(INPUT_EVENT_DEBUG, 
            "DEBUG:%s enter, fd=%d\n", __PRETTY_FUNCTION__, fd);
    if (mFreeSpace) {
        const ssize_t nread = read(fd, mHead, mFreeSpace * sizeof(input_event));
        if (nread < 0 || nread % sizeof(input_event)) {
            //LOGE("Partial event received nread=%d, required=%d", 
            //     nread, sizeof(input_event));
            //LOGE("FD trying to read is: %d");
            // we got a partial event!!
            if (INPUT_EVENT_DEBUG) {
                LOGV_IF(nread < 0, "DEBUG:%s exit nread < 0\n", 
                        __PRETTY_FUNCTION__);
                LOGV_IF(nread % sizeof(input_event), 
                        "DEBUG:%s exit nread %% sizeof(input_event)\n", 
                        __PRETTY_FUNCTION__);
            }
            return (nread < 0 ? -errno : -EINVAL);
        }

        numEventsRead = nread / sizeof(input_event);
        if (numEventsRead) {
            mHead += numEventsRead;
            mFreeSpace -= numEventsRead;
            if (mHead > mBufferEnd) {
                size_t s = mHead - mBufferEnd;
                memcpy(mBuffer, mBufferEnd, s * sizeof(input_event));
                mHead = mBuffer + s;
            }
        }
    }

    LOGV_IF(INPUT_EVENT_DEBUG, "DEBUG:%s exit, numEventsRead:%d\n", 
            __PRETTY_FUNCTION__, numEventsRead);
    return numEventsRead;
}

ssize_t InputEventCircularReader::readEvent(input_event const** events)
{
    *events = mCurr;
    ssize_t available = (mBufferEnd - mBuffer) - mFreeSpace;
    LOGV_IF(INPUT_EVENT_DEBUG, "DEBUG:%s fd:%d, available:%d\n", 
            __PRETTY_FUNCTION__, mLastFd, (int)available);
    return (available ? 1 : 0);
}

void InputEventCircularReader::next()
{
    mCurr++;
    mFreeSpace++;
    if (mCurr >= mBufferEnd) {
        mCurr = mBuffer;
    }
    ssize_t available = (mBufferEnd - mBuffer) - mFreeSpace;
    LOGV_IF(INPUT_EVENT_DEBUG, "DEBUG:%s fd:%d, still available:%d\n",
            __PRETTY_FUNCTION__, mLastFd, (int)available);
}

