/*
// Copyright (c) 2014 Intel Corporation 
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/
#ifndef UEVENT_OBSERVER_H
#define UEVENT_OBSERVER_H

#include <utils/KeyedVector.h>
#include <utils/String8.h>
#include <common/base/SimpleThread.h>

namespace android {
namespace intel {

typedef void (*UeventListenerFunc)(void *data);

class UeventObserver
{
public:
    UeventObserver();
    virtual ~UeventObserver();

public:
    bool initialize();
    void deinitialize();
    void start();
    void registerListener(const char *event, UeventListenerFunc func, void *data);

private:
    DECLARE_THREAD(UeventObserverThread, UeventObserver);
    void onUevent();

private:
    enum {
        UEVENT_MSG_LEN = 4096,
    };

    char mUeventMessage[UEVENT_MSG_LEN];
    int mUeventFd;
    int mExitRDFd;
    int mExitWDFd;
    struct UeventListener {
        UeventListenerFunc func;
        void *data;
    };
    KeyedVector<String8, UeventListener*> mListeners;
};

} // namespace intel
} // namespace android

#endif

